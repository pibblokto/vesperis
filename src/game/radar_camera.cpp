// The radar camera's picture (C-07): the sky through the ship's instrument, cold and full of static, and its readout
#include "radar_internal.h"
#include "ui.h"
#include "galaxy/planetmap.h"
#include "peoples/ruins.h"
#include "core/rng.h"
#include "core/parallel.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>

// the picture as the radar camera gives it: a cold phosphor (the palette toward blue-green, lifted off black), scanlines, a band
// rolling down, a vignette, and static by the clarity (noise on every pixel, snow in flecks) that thins as a signal comes through.
// with the gain the static and the snow thicken, the radio sky the receiver hears glows through the phosphor (the band's
// brightness, on a coarse grid of directions), the star field ghosts a faint double, the rolling band runs faster and the picture
// tears now and then (a band of rows thrown sideways for an eighth of a second)
void Game::radarCameraFeed(Framebuffer& fb) {
    const double gT = gainShare(radarGain());
    double statik = 1 - 0.8 * clampd(radar.clarity, 0, 1);
    for (int i = 0; i < BANKS * 64; i++) {
        uint8_t* c = fb.pal + i * 3;
        double lum = 0.3 * c[0] + 0.59 * c[1] + 0.11 * c[2];
        c[0] = (uint8_t)clampd(10 + lum * 0.55, 0, 255); c[1] = (uint8_t)clampd(14 + lum * 1.0, 0, 255); c[2] = (uint8_t)clampd(16 + lum * 0.92, 0, 255);
    }
    int S = FB_SCALE;
    static std::vector<float> vx, vy;
    if ((int)vx.size() != FBW) { vx.resize(FBW); for (int x = 0; x < FBW; x++) { double u = (x + 0.5) / FBW * 2 - 1; vx[x] = (float)(u * u); } }
    if ((int)vy.size() != FBH) { vy.resize(FBH); for (int y = 0; y < FBH; y++) { double u = (y + 0.5) / FBH * 2 - 1; vy[y] = (float)(u * u); } }
    // the radio sky: the band's brightness at every eighth pixel of the screen's grid, in intensity units (two shades at x1 where the
    // band is brightest, twelve at x32)
    const int cell = 8 * S, GW = FBW / cell + 2, GH = FBH / cell + 2;
    static std::vector<float> glow, colT; static std::vector<int> colI;
    if ((int)colI.size() != FBW || (int)colT.size() != FBW) { colI.resize(FBW); colT.resize(FBW); }
    for (int x = 0; x < FBW; x++) { double fx = (double)x / cell; colI[x] = std::min(GW - 2, (int)fx); colT[x] = (float)(fx - colI[x]); }   // the grid's column and the share across it, per pixel
    const std::vector<float>& band = spaceR.parkedBand();
    const double glowAmp = band.empty() ? 0 : INTEN_PER_SHADE * (2 + 10 * gT);
    if (glowAmp > 0) {
        glow.assign((size_t)GW * GH, 0.f);
        Mat3 camT = viewBasis().transposed(); double invf = 1.0 / spaceR.proj.f;
        for (int j = 0; j < GH; j++) for (int i = 0; i < GW; i++) {
            Vec3 d = normalize(Vec3((i * cell - spaceR.proj.cx) * invf, -(j * cell - spaceR.proj.cy) * invf, 1.0));
            glow[(size_t)j * GW + i] = (float)(glowAmp * sampleGalaxyBand(band, camT * d));
        }
    }
    // the ghost double and the tears read the picture as it was
    static std::vector<Pix> orig;
    const int dblX = 3 * S, dblY = S; const double dblK = 0.45 * gT;
    int tearY0 = -1, tearY1 = -1, tearShift = 0;
    if (gT > 0) {
        uint64_t h = mix64((uint64_t)(realTime * 8) ^ 0x7EA2ULL);
        if ((h >> 11) * (1.0 / 9007199254740992.0) < 0.004 + 0.035 * gT) {
            tearY0 = (int)((h >> 20) % (uint64_t)FBH); tearY1 = tearY0 + (6 + (int)((h >> 40) % 30)) * S;
            tearShift = (4 + (int)((h >> 50) % 16)) * S * ((h >> 63) ? 1 : -1);
        }
    }
    if (dblK > 0 || tearY0 >= 0) orig = fb.idx;
    int bandY = (int)(std::fmod(radar.rollPh, 1.0) * (FBH + 40 * S)) - 20 * S;   // rolls down the picture every 6 s at x1 (C-16: 2.4 s at x32)
    int noiseAmp = (int)(40 + 150 * statik + 50 * gT), snowOdds = (int)(2 + 40 * statik + 25 * gT);   // intensity units (32 a shade); flecks per 4096 pixels
    static std::vector<uint32_t> rowStart;   // the rows on the pool, each band from its first row's noise state (xorshiftRowStarts)
    xorshiftRowStarts((uint32_t)(realTime * 60) * 2654435761u + 0x51A7u, FBW, FBH, rowStart);
    parallelFor(FBH, 32, [&](int yb, int ye) {
        uint32_t rs = rowStart[yb];
        std::vector<float> rowGlow(glowAmp > 0 ? (size_t)GW : 0, 0.f);   // the radio sky along this row (the band's own)
        for (int y = yb; y < ye; y++) {
            double line = ((y / S) & 1) ? 0.8 : 1.0;
            double bnd = std::abs(y - bandY) < 10 * S ? 1.08 : 1.0;
            Pix* row = &fb.idx[(size_t)y * FBW];
            const bool tear = y >= tearY0 && y < tearY1;
            if (glowAmp > 0) {
                double fy = (double)y / cell; int j0 = std::min(GH - 2, (int)fy); double ty = fy - j0;
                for (int i = 0; i < GW; i++) rowGlow[i] = (float)(glow[(size_t)j0 * GW + i] * (1 - ty) + glow[(size_t)(j0 + 1) * GW + i] * ty);
            }
            for (int x = 0; x < FBW; x++) {
                rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
                int xs = tear ? std::max(0, std::min(FBW - 1, x - tearShift)) : x;
                Pix p = tear ? orig[(size_t)y * FBW + xs] : row[x];
                double base = intenOf(p);
                if (dblK > 0 && y >= dblY && xs >= dblX) base = std::max(base, dblK * intenOf(orig[(size_t)(y - dblY) * FBW + (xs - dblX)]));
                if (glowAmp > 0) { int i0 = colI[x]; float tx = colT[x]; base += rowGlow[i0] + (rowGlow[i0 + 1] - rowGlow[i0]) * tx; }
                double vig = 1 - 0.5 * std::max(0.0, (double)(vx[x] + vy[y]) - 0.4);
                int inten = (int)(base * line * bnd * vig) + (int)(rs % (2 * noiseAmp + 1)) - noiseAmp;
                if ((int)((rs >> 8) & 4095) < snowOdds) inten = std::max(inten, 32 * (28 + (int)((rs >> 20) % 24)));
                row[x] = pixI(bankOf(p), inten);
            }
        }
    });
}

// the camera's frame over the picture: the vehicles' brackets and channel, the ship's attitude as the camera's azimuth and
// elevation, the scope and the meter, the lock cone round the crosshair, the source's smudge within the middle lobe and its
// diamond once locked, the readout (the hint, the hold, or the lock: the system only)
void Game::renderRadarCamera() {
    const RadarState& r = radar;
    const uint32_t fc = HUD_WHITE, fd = HUD_DIM;
    int cx = UW / 2, cy = UH / 2;
    drawVisor(fd);
    {   // brackets inside the visor's
        int m = 14, l = 26;
        drawLineRGB(canvas, m, m, m + l, m, fc); drawLineRGB(canvas, m, m, m, m + l, fc);
        drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m - l, m, fc); drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m, m + l, fc);
        drawLineRGB(canvas, m, UH - 1 - m, m + l, UH - 1 - m, fc); drawLineRGB(canvas, m, UH - 1 - m, m, UH - 1 - m - l, fc);
        drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m - l, UH - 1 - m, fc); drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m, UH - 1 - m - l, fc);
    }
    drawTextShadow(canvas, 8, 16, "CAM 03 RADAR", fc, HUD_SHADOW);
    if ((int)(realTime * 2) & 1) drawTextShadow(canvas, 8 + textWidth("CAM 03 RADAR") + 6, 16, "\x07REC", HUD_RED, HUD_SHADOW);
    drawTextShadow(canvas, 8, 24, fmt("AZ %03.0f  EL %+03.0f", wrap2pi(ship.yaw) / DEG, ship.pitch / DEG).c_str(), fd, HUD_SHADOW);
    drawTextShadow(canvas, UW - 8 - textWidth(epocString().c_str()), 8, epocString().c_str(), HUD_GREEN, HUD_SHADOW);
    std::string sysName = sys.valid ? trunc(upper(starNameOf(sys.star)), 20) : "INTERSTELLAR SPACE";
    drawTextShadow(canvas, UW - 8 - textWidth(sysName.c_str()), 16, sysName.c_str(), fd, HUD_SHADOW);
    double pxPerRad = spaceR.proj.f / FB_SCALE;
    auto arc = [&](double rad, double from, double to, uint32_t col) {   // fractions of the turn from the top, clockwise
        int segs = 40; double a0 = -PI / 2 + from * TAU, a1 = -PI / 2 + to * TAU;
        int px = (int)std::round(cx + rad * std::cos(a0)), py = (int)std::round(cy + rad * std::sin(a0));
        for (int k = 1; k <= segs; k++) {
            double a = a0 + (a1 - a0) * k / segs;
            int x = (int)std::round(cx + rad * std::cos(a)), y = (int)std::round(cy + rad * std::sin(a));
            drawLineRGB(canvas, px, py, x, y, col); px = x; py = y;
        }
    };
    const double G = radarGain();
    {   // crosshair and the lock cone: dim while sweeping, cyan with a white arc filling while the hold lasts, amber once locked
        // (C-15: the cone narrows with the gain)
        drawLineRGB(canvas, cx - 10, cy, cx - 4, cy, fd); drawLineRGB(canvas, cx + 4, cy, cx + 10, cy, fd);
        drawLineRGB(canvas, cx, cy - 8, cx, cy - 3, fd); drawLineRGB(canvas, cx, cy + 3, cx, cy + 8, fd);
        double rad = pxPerRad * std::tan(lockCone(G));
        arc(rad, 0, 1, r.locked >= 0 ? HUD_AMBER : (r.holdT > 0 ? HUD_CYAN : fd));
        if (r.locked < 0 && r.holdT > 0) arc(rad + 2, 0, clampd(r.holdT / std::max(r.lockSecs, 0.1), 0, 1), HUD_WHITE);
    }
    {   // the scope: the meter's last four seconds, and the meter beside it
        int bx0 = cx - 80, bx1 = cx + 80, by0 = 36, by1 = 56;
        blendRectRGB(canvas, bx0 - 2, by0 - 2, bx1 + 14, by1 + 2, rgb(0, 0, 0), 150);
        drawRectRGB(canvas, bx0, by0, bx1, by1, fd);
        int h = by1 - by0 - 2;
        for (size_t i = 1; i < r.trace.size(); i++) {
            int x0 = bx0 + 1 + (int)(i - 1) + (158 - (int)r.trace.size()), x1 = x0 + 1;
            int y0 = by1 - 1 - (int)(clampd(r.trace[i - 1], 0, 1) * h), y1 = by1 - 1 - (int)(clampd(r.trace[i], 0, 1) * h);
            drawLineRGB(canvas, x0, y0, x1, y1, HUD_GREEN);
        }
        int mh = (int)(clampd(r.reading, 0, 1) * h);
        drawRectRGB(canvas, bx1 + 4, by0, bx1 + 12, by1, fd);
        if (mh > 0) fillRectRGB(canvas, bx1 + 5, by1 - 1 - mh, bx1 + 11, by1 - 1, r.overload ? HUD_RED : (r.locked >= 0 ? HUD_AMBER : HUD_GREEN));   // pinned red by an overload
        std::string right = r.locked >= 0 ? "LOCKED" : (r.overload ? "OVERLOAD" : (r.holdT > 0 && r.nothingT <= 0 ? "LOCKING" : "SWEEP"));   // a false rise's hold empties under SWEEP
        drawText(canvas, bx1 + 12 - textWidth(right.c_str()), by1 + 4, right.c_str(), r.locked >= 0 ? HUD_AMBER : (r.overload ? HUD_RED : fd));
        drawText(canvas, bx0, by1 + 4, "RECEIVER", fd);
        std::string gain = fmt("GAIN X%.0f", G);   // the dial, by the scope (the reach is the transmitter's: not printed)
        drawText(canvas, bx0 + textWidth("RECEIVER") + 8, by1 + 4, gain.c_str(), G > 1 ? HUD_CYAN : fd);
    }
    // the source: a smudge of static where the beam's middle lobe has it (found, not handed: nothing shows outside ten degrees),
    // the lock's diamond once locked; an edge mark when it is behind
    int src = r.locked >= 0 ? r.locked : (r.aim >= 0 ? r.aim : r.best);
    if (src >= 0 && src < (int)r.all.size()) {
        const Signal& s = r.all[src];
        Mat3 cam = viewBasis();
        Vec3 v = cam * (s.pos - ship.pos);
        double g = beamGain(r.angle[src]);
        if (v.z > 0) {
            int sx = (int)(UW / 2 + spaceR.proj.f * v.x / v.z / FB_SCALE), sy = (int)(UH / 2 - spaceR.proj.f * v.y / v.z / FB_SCALE);
            if (sx > 4 && sy > 4 && sx < UW - 4 && sy < UH - 4) {
                if (r.locked >= 0) {
                    drawLineRGB(canvas, sx - 5, sy, sx, sy - 5, HUD_AMBER); drawLineRGB(canvas, sx, sy - 5, sx + 5, sy, HUD_AMBER);
                    drawLineRGB(canvas, sx + 5, sy, sx, sy + 5, HUD_AMBER); drawLineRGB(canvas, sx, sy + 5, sx - 5, sy, HUD_AMBER);
                } else if (g > 0.35) {
                    double dens = (g - 0.35) / 0.65 * std::min(1.0, src < (int)r.level.size() ? r.level[src] : s.strength);   // by its level at this gain
                    uint32_t rs = (uint32_t)(realTime * 30) * 2654435761u + (uint32_t)src;
                    int rad = 4 + (int)(5 * dens);
                    for (int dy = -rad; dy <= rad; dy++) for (int dx = -rad; dx <= rad; dx++) {
                        if (dx * dx + dy * dy > rad * rad) continue;
                        rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
                        int x = sx + dx, y = sy + dy;
                        if (x < 0 || y < 0 || x >= canvas.w / canvas.scale || y >= canvas.h / canvas.scale) continue;
                        if ((rs & 255) < (uint32_t)(60 + 120 * dens) * (uint32_t)(rad * rad - dx * dx - dy * dy) / (uint32_t)(rad * rad)) fillRectRGB(canvas, x, y, x, y, HUD_GREEN);
                    }
                }
            }
        } else if (r.locked >= 0) { double ang = std::atan2(-v.y, v.x); drawText(canvas, UW / 2 + (int)(std::cos(ang) * 140) - 2, UH / 2 + (int)(std::sin(ang) * 85) - 3, "+", HUD_AMBER); }
    }
    // a false rise smudges the static as a source would where nothing outshines it within the middle lobe: the instrument
    // cannot tell, the hold can
    if (r.rise.on && r.locked < 0 && r.riseAngle < 10 * DEG && (src < 0 || r.riseGain > r.gain[src])) {
        Mat3 cam = viewBasis();
        Vec3 v = cam * r.rise.dir;
        double g = beamGain(r.riseAngle);
        if (v.z > 0 && g > 0.35) {
            int sx = (int)(UW / 2 + spaceR.proj.f * v.x / v.z / FB_SCALE), sy = (int)(UH / 2 - spaceR.proj.f * v.y / v.z / FB_SCALE);
            double dens = (g - 0.35) / 0.65 * std::min(1.0, r.rise.peak * 3) * (r.rise.spent ? 0.4 : 1.0);
            uint32_t rs = (uint32_t)(realTime * 30) * 2654435761u + 0x5EEDu;
            int rad = 4 + (int)(5 * dens);
            for (int dy = -rad; dy <= rad; dy++) for (int dx = -rad; dx <= rad; dx++) {
                if (dx * dx + dy * dy > rad * rad) continue;
                rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
                int x = sx + dx, y = sy + dy;
                if (x < 0 || y < 0 || x >= canvas.w / canvas.scale || y >= canvas.h / canvas.scale) continue;
                if ((rs & 255) < (uint32_t)(60 + 120 * dens) * (uint32_t)(rad * rad - dx * dx - dy * dy) / (uint32_t)(rad * rad)) fillRectRGB(canvas, x, y, x, y, HUD_GREEN);
            }
        }
    }
    // the readout: the system only
    std::string l1, l2, l3; uint32_t c1 = fd;
    if (r.locked >= 0 && r.locked < (int)r.all.size()) {
        const Signal& s = r.all[r.locked];
        c1 = HUD_AMBER;
        if (s.local) {
            l1 = "A SIGNAL FROM THIS SYSTEM";
            l2 = fmt("%s", trunc(upper(bodyNameOf(s.body)), 30).c_str());
            l3 = "ENTER APPROACHES IT";
        } else if (s.kind == SIG_ROGUE) {   // the distance and no star at the source, nothing more (the sound tells, the readout never does)
            Star full = s.star; if (full.name.empty()) placeInSector(s.star.sx, s.star.sy, s.star.sz, full, true);
            l1 = "NO STAR AT THE SOURCE";
            l2 = thousands(s.distLy) + " LY" + (guide.visited.count(starKeyOf(full)) ? "  VISITED" : "");
            l3 = "ENTER FLIES THERE";
        } else {   // thousands of light years at the top steps (the line within its 52 characters: the name gives way first)
            Star full = s.star; if (full.name.empty()) placeInSector(s.star.sx, s.star.sy, s.star.sz, full, true);
            std::string ly = thousands(s.distLy), tail = fmt(" (%s)  %s LY%s", STAR_CLASSES[full.cls].code, ly.c_str(), guide.visited.count(starKeyOf(full)) ? "  VISITED" : "");   // a loop back to a system seen says so
            l1 = "A SIGNAL FROM " + trunc(upper(starNameOf(full)), (size_t)clampi(52 - 14 - (int)tail.size(), 6, 16)) + tail;
            std::string sector = fmt("SECTOR %lld %lld %lld", (long long)s.star.sx, (long long)s.star.sy, (long long)s.star.sz);
            l2 = sector + " - THE SIGNAL IS " + ly + " YEARS OLD";
            if (l2.size() > 52) l2 = sector + " - " + ly + " YEARS OLD";
            l3 = "ENTER LOCKS THE STAR AND FLIES THERE";
        }
    } else if (r.overload) { l1 = "OVERLOADED - TURN THE GAIN DOWN"; c1 = HUD_RED; }   // backing the gain off is part of the play
    else if (r.nothingT > 0) { l1 = "NOTHING THERE"; c1 = fc; }   // a false rise held empties
    else if (r.holdT > 0) { l1 = fmt("LOCKING %.0f%% - HOLD STEADY", 100 * clampd(r.holdT / std::max(r.lockSecs, 0.1), 0, 1)); c1 = HUD_CYAN; }
    else if (r.gainStep > 0 && !r.deepReady) { l1 = "LISTENING DEEPER..."; c1 = HUD_CYAN; }   // the far scan, a slice a frame
    else if ((r.best >= 0 && r.bestGain > 0.06 && (G <= 1 || r.bestAngle < 10 * DEG)) || (r.rise.on && !r.rise.spent && r.riseGain > 0.06 && r.riseAngle < 10 * DEG)) { l1 = "A RISE - CENTRE IT AND HOLD TO LOCK"; c1 = HUD_GREEN; }   // over x1 the side lobes rise everywhere: the middle lobe's (C-16: a false rise's too)
    else l1 = "SWEEP THE SKY - A RISE ON THE METER IS A SIGNAL";
    drawTextCentered(canvas, cx, UH - 54, l1.c_str(), c1);
    if (!l2.empty()) drawTextCentered(canvas, cx, UH - 46, l2.c_str(), fc);
    if (!l3.empty()) drawTextCentered(canvas, cx, UH - 38, l3.c_str(), HUD_GREEN);
    drawTextShadow(canvas, 18, UH - 26, "+/- GAIN  B / ESC LEAVES THE CAMERA", fd, HUD_SHADOW);   // the telescope's place, clear of the bracket's upright
}
