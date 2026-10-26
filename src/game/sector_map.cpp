// The sector map (SECTOR_MAP, M4-02): the ground round the explorer at six scales, its cursor, marks and waypoint
#include "game.h"
#include "peoples/plan.h"
#include <array>
#include "ui.h"
#include "core/rng.h"
#include "core/parallel.h"
#include <cmath>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

// the sector map: heights, water, the capsule, the buggy, the waypoint and you (R-414: M4-02's trail of where you walked is gone;
// the user: "remove these lines that show what routes you traveled - they seem to be a good idea but they confuse you a lot").
static const double SECTOR_HALF_W[6] = {1000, 4000, 16000, 64000, 256000, 1024000};   // metres (Game::SECTOR_LEVELS)
static const double SECTOR_DETAIL[6] = {16, 64, 512, 2048, 4096, 16384};

// O1-04: the widest level whose half-width stays within 1.2 radii of the world (a comet has two levels, a planet six)
int Game::sectorMaxLevel() const {
    int m = 0;
    for (int k = 1; k < SECTOR_LEVELS; k++) if (SECTOR_HALF_W[k] <= 1.2 * surf.site.R) m = k;
    return m;
}

// R-408: the map's cursor (the user: "make the surface N map more interactive so you can place arbitrary marker on something YOU
// see as a landmark and name it"): the arrows (Shift faster) and the mouse move it within the frame, C or Space bring it back to the
// explorer; Enter marks the place under it (the name's entry; on a mark: renames it), Delete twice removes the mark under it, M puts
// the waypoint there (on the waypoint: clears it), Tab goes to the next mark; +/- and the wheel zoom (the arrows did), R and Shift+R
// work the scanner as on the ground. B-412: Shift+Enter or Shift+click on a mark shows it over the view (the one shown), again hides it
void Game::updateSectorMap(const Input& in) {
    if (in.wasPressed(KEY_EQUAL) || in.wheel > 0) sectorZoom = std::max(0, sectorZoom - 1);
    if (in.wasPressed(KEY_MINUS) || in.wheel < 0) sectorZoom = std::min(sectorMaxLevel(), sectorZoom + 1);
    const double hw = SECTOR_HALF_W[std::min(sectorZoom, sectorMaxLevel())], mpp = 2 * hw / 200, hh = hw * 0.8;
    const double px = surf.player.x, pz = surf.player.z;
    const double sp = (in.shift() ? 3.0 : 1.0) * 0.6 * hw * lastRealDt;
    if (in.isDown(KEY_LEFT)) mapCurX -= sp;
    if (in.isDown(KEY_RIGHT)) mapCurX += sp;
    if (in.isDown(KEY_UP)) mapCurZ += sp;
    if (in.isDown(KEY_DOWN)) mapCurZ -= sp;
    mapCurX += in.mouseDx * 0.5 * settings.mouseSensitivity * mpp;
    mapCurZ -= in.mouseDy * 0.5 * settings.mouseSensitivity * mpp;
    if (in.wasPressed(KEY_C) || in.wasPressed(KEY_SPACE)) { mapCurX = px; mapCurZ = pz; }
    if (in.wasPressed(KEY_TAB)) {   // the next mark in the frame, nearest the explorer first
        std::vector<std::pair<double, const SiteMark*>> in_;
        for (const SiteMark& m : marksHere()) if (std::fabs(m.x - px) <= hw && std::fabs(m.z - pz) <= hh) in_.push_back({std::hypot(m.x - px, m.z - pz), &m});
        std::sort(in_.begin(), in_.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        if (in_.empty()) status("NO MARK IN THE FRAME - ZOOM OUT (-)", 3);
        else {
            size_t k = 0;
            for (size_t i = 0; i < in_.size(); i++) if (in_[i].second->index == mapTabMark) { k = (i + 1) % in_.size(); break; }
            mapTabMark = in_[k].second->index; mapCurX = in_[k].second->x; mapCurZ = in_[k].second->z;
        }
        audio.beep = BEEP_UI;
    }
    mapCurX = clampd(mapCurX, px - hw, px + hw); mapCurZ = clampd(mapCurZ, pz - hh, pz + hh);
    const int onMark = markNear(mapCurX, mapCurZ, 5 * mpp);
    if ((enterKey(in) || in.mousePressed[0]) && in.shift()) {
        if (onMark >= 0) showMark(onMark);
        else { refuse("NO MARK UNDER THE CURSOR", 2); }
    } else if (enterKey(in) || in.mousePressed[0]) { if (onMark >= 0) beginRenameMark(onMark); else beginMark(mapCurX, mapCurZ); return; }
    if (in.wasPressed(KEY_DELETE) || in.wasPressed(KEY_BACKSPACE)) {
        if (onMark >= 0 && onMark == mapDelMark && realTime < mapDelUntil) { removeMark(onMark); mapDelMark = -1; }
        else if (onMark >= 0) { mapDelMark = onMark; mapDelUntil = realTime + 3; refuse(fmt("REMOVE %s? DELETE AGAIN", trunc(markLabel(onMark), 24).c_str()), 3); }
        else { refuse("NO MARK UNDER THE CURSOR", 2); }
    }
    if (in.wasPressed(KEY_M) && !in.ctrl()) {
        if (surf.hasWaypoint && std::hypot(surf.wpX - mapCurX, surf.wpZ - mapCurZ) < 5 * mpp) { surf.hasWaypoint = false; status("WAYPOINT CLEARED", 3); }
        else { surf.hasWaypoint = true; surf.wpX = mapCurX; surf.wpZ = mapCurZ; status(std::hypot(mapCurX - px, mapCurZ - pz) < 2 * mpp ? std::string("WAYPOINT SET HERE") : fmt("WAYPOINT SET %s AWAY", metresString(std::hypot(mapCurX - px, mapCurZ - pz)).c_str()), 3); }
        audio.beep = BEEP_UI;
    }
    if (in.wasPressed(KEY_R) && !in.ctrl()) { if (in.shift()) scanToggleSkip(); else scanCycle(); }
    if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_N)) state = GameState::SURFACE;
}

void Game::renderSectorMap() {
    const int MW = 200, MH = 160, ox = 8, oy = 14;   // logical frame
    sectorZoom = std::min(sectorZoom, sectorMaxLevel());
    double hw = SECTOR_HALF_W[sectorZoom], mpp = 2 * hw / MW;   // metres per logical pixel
    double px = surf.player.x, pz = surf.player.z;
    const Body& body = sys.bodies[surf.site.body];
    // rebuild the height image when the view moved by a tenth, the zoom changed, or the site was re-initialised or
    // re-anchored (B-303: a new landing near the old one's coordinates showed the old landing's image)
    if (sectorImgZoom != sectorZoom || sectorImgEpoch != surf.siteEpoch || std::fabs(px - sectorImgX) > hw * 0.1 || std::fabs(pz - sectorImgZ) > hw * 0.1) {
        sectorImgZoom = sectorZoom; sectorImgEpoch = surf.siteEpoch; sectorImgX = px; sectorImgZ = pz;
        sectorImg.assign(sectorImgW * sectorImgH, 0);
        double cell = mpp * 2;   // one sample per 2x2 logical pixels
        double detail = SECTOR_DETAIL[sectorZoom];
        std::vector<float> hs(sectorImgW * sectorImgH);
        std::vector<TerrainVertex> tv(sectorImgW * sectorImgH);
        parallelFor(sectorImgH, 8, [&](int y0, int y1) {
            for (int y = y0; y < y1; y++)
                for (int x = 0; x < sectorImgW; x++) {
                    double wx = px + (x - sectorImgW / 2) * cell, wz = pz - (y - sectorImgH / 2) * cell;
                    tv[y * sectorImgW + x] = surf.site.sampleAt(wx, wz, detail);
                    hs[y * sectorImgW + x] = tv[y * sectorImgW + x].h;
                }
        });
        // the same colouring as the landing map (material ramps, exposure rule, hillshade from the north-west), so
        // the map you land by and the map you walk by agree
        MatRamp ramps[MAT_COUNT]; double lf; bool atmo;
        mapRampsFor(body, ramps, lf, atmo);
        auto hAt = [&](int x, int y) { x = clampi(x, 0, sectorImgW - 1); y = clampi(y, 0, sectorImgH - 1); return (double)hs[y * sectorImgW + x]; };
        double sum2 = 0; int cnt = 0;
        for (int y = 0; y < sectorImgH; y++) for (int x = 0; x < sectorImgW; x++) { double gx = hAt(x + 1, y) - hAt(x - 1, y), gy = hAt(x, y - 1) - hAt(x, y + 1); sum2 += gx * gx + gy * gy; cnt++; }
        double k = std::min(0.5 / (std::sqrt(sum2 / std::max(1, cnt)) + 1e-9), 0.5 / (0.03 * 2 * cell));   // a 3% grade at most for full shading
        const bool survey = sys.sunless();   // the survey look of a world with no sun, by the frame's heights
        double hLo = 0, hHi = 1; if (survey) surveyRange(hs.data(), hs.size(), hLo, hHi);
        for (int y = 0; y < sectorImgH; y++)
            for (int x = 0; x < sectorImgW; x++) {
                const TerrainVertex& v = tv[y * sectorImgW + x];
                double gx = hAt(x + 1, y) - hAt(x - 1, y), gy = hAt(x, y - 1) - hAt(x, y + 1);
                double shade = clampd(1.0 + k * 0.7071 * (gx + gy), 0.5, 1.5);
                int mat = v.material;
                if (v.water > -1e8f && v.h < v.water && !(v.material == MAT_SAND && surf.site.seaVertex(v))) mat = MAT_WATER;   // a tidal flat its wet sand
                sectorImg[y * sectorImgW + x] = mapColor(ramps, mat, v.albedo, lf, atmo, shade, survey ? surveyHeight(v.h, hLo, hHi) : -1);
            }
        if (surf.site.gen.tideRangeM > 0 && surf.site.hasWater) {   // the tide lines, the high water's pale along the flat's landward edge, the low water's dark
            const double S = surf.site.seaLevel;
            for (int y = 0; y + 1 < sectorImgH; y++)
                for (int x = 0; x + 1 < sectorImgW; x++) {
                    const double Rl = tideRangeAt(surf.site.gen, surf.site.unitAt(px + (x - sectorImgW / 2) * cell, pz - (y - sectorImgH / 2) * cell));
                    const double hi = S + 0.5 * Rl, lo = S - 0.5 * Rl, h0 = hAt(x, y), hr = hAt(x + 1, y), hd = hAt(x, y + 1);
                    if ((h0 - hi) * (hr - hi) < 0 || (h0 - hi) * (hd - hi) < 0) sectorImg[y * sectorImgW + x] = rgb(232, 226, 204);
                    else if ((h0 - lo) * (hr - lo) < 0 || (h0 - lo) * (hd - lo) < 0) sectorImg[y * sectorImgW + x] = rgb(40, 62, 120);
                }
        }
    }
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 200);
    int S = FB_SCALE;
    double shiftX = (px - sectorImgX) / mpp, shiftZ = (pz - sectorImgZ) / mpp;   // the image is a little stale between rebuilds
    // the image's column of every pixel column and its row of every pixel row, by the per-pixel expressions, then the rows on the pool
    static std::vector<int> ixs, iys;
    ixs.resize(MW * S); iys.resize(MH * S);
    for (int x = 0; x < MW * S; x++) ixs[x] = (int)((x / (double)S + shiftX) / 2);
    for (int y = 0; y < MH * S; y++) iys[y] = (int)((y / (double)S - shiftZ) / 2);
    parallelFor(MH * S, 64, [&](int yb, int ye) {
        for (int y = yb; y < ye; y++)
            for (int x = 0; x < MW * S; x++) {
                int ix = ixs[x], iy = iys[y];
                uint32_t col = (ix >= 0 && iy >= 0 && ix < sectorImgW && iy < sectorImgH) ? sectorImg[iy * sectorImgW + ix] : rgb(10, 10, 10);
                canvas.px[(size_t)(oy * S + y) * FBW + ox * S + x] = col;
            }
    });
    if (surf.localFront.on) {   // the nearest front's rain line across the map and its band behind it, hatched
        const LocalFront& F = surf.localFront;
        const double s0 = FRONT_RAIN_END * F.width;
        const uint32_t lineC = F.dust ? HUD_AMBER : HUD_CYAN;
        for (int y = 0; y < MH; y++)
            for (int x = 0; x < MW; x++) {
                double sA = F.ahead(px + (x - MW / 2 + 0.5) * mpp, pz - (y - MH / 2 + 0.5) * mpp);
                if (std::fabs(sA) <= 0.6 * mpp) fillRectRGB(canvas, ox + x, oy + y, ox + x, oy + y, lineC);
                else if (sA < 0 && sA > s0 && ((x + y) & 3) == 0) blendRectRGB(canvas, ox + x, oy + y, ox + x, oy + y, lineC, 110);
            }
    }
    drawRectRGB(canvas, ox - 1, oy - 1, ox + MW, oy + MH, HUD_DIM);
    auto mark = [&](double wx, double wz, int& sx, int& sy) { sx = ox + MW / 2 + (int)((wx - px) / mpp); sy = oy + MH / 2 - (int)((wz - pz) / mpp); return sx >= ox && sy >= oy && sx < ox + MW && sy < oy + MH; };
    int sx, sy;
    if (sectorZoom >= 2) {   // the sector grid with the sectors' names on the wide levels
        // the grid's step grows with the zoom (1, 2, 5, 10, 15, 30 degrees) so its lines stay at least 24 px apart;
        // the one-degree cells carry the sectors' names when they are wide enough, the coarser lines their latitude and longitude
        auto toPx = [&](double lat, double lon, int& gx, int& gy) { double x, z; surf.site.localAt(StarSystem::bodyFromLatLon(lat, lon), x, z); gx = ox + MW / 2 + (int)((x - px) / mpp); gy = oy + MH / 2 - (int)((z - pz) / mpp); };
        double pxPerDeg = DEG * surf.site.R / mpp;
        static const int STEPS[] = {1, 2, 5, 10, 15, 30};
        int step = 30;
        for (int s : STEPS) if (s * pxPerDeg >= 24) { step = s; break; }
        double latC = surf.env.latDeg * DEG, lonC = surf.env.lonDeg * DEG;
        double span = hw / surf.site.R * 1.1, lonSpan = span / std::max(std::cos(latC), 0.15);
        int j0 = (int)std::floor((latC - span) / DEG / step) * step, j1 = (int)std::ceil((latC + span) / DEG / step) * step;
        int k0 = (int)std::floor((lonC - lonSpan) / DEG / step) * step, k1 = (int)std::ceil((lonC + lonSpan) / DEG / step) * step;
        j0 = std::max(j0, -90); j1 = std::min(j1, 90);
        const uint32_t gridC = rgb(95, 95, 60);
        auto polyline = [&](int n, const std::function<void(int, int&, int&)>& pointAt) {
            int lx = 0, ly = 0; bool have = false;
            for (int i = 0; i <= n; i++) { int gx, gy; pointAt(i, gx, gy); bool in = gx >= ox && gx < ox + MW && gy >= oy && gy < oy + MH; if (have && in) drawLineRGB(canvas, lx, ly, gx, gy, gridC); lx = gx; ly = gy; have = in; }
        };
        for (int j = j0; j <= j1; j += step) polyline(24, [&](int i, int& gx, int& gy) { toPx(j * DEG, (k0 + (k1 - k0) * i / 24.0) * DEG, gx, gy); });
        for (int k = k0; k <= k1; k += step) polyline(24, [&](int i, int& gx, int& gy) { toPx((j0 + (j1 - j0) * i / 24.0) * DEG, k * DEG, gx, gy); });
        if (step == 1 && pxPerDeg >= 46) {
            for (int j = j0; j < j1; j++)
                for (int k = k0; k < k1; k++) {
                    int gx, gy; toPx((j + 0.5) * DEG, (k + 0.5) * DEG, gx, gy);
                    if (gx - 21 < ox || gx + 21 > ox + MW || gy - 3 < oy || gy + 4 > oy + MH) continue;
                    drawText(canvas, gx - 21, gy - 3, sectorName(j + 0.5, wrapAngle((k + 0.5) * DEG) / DEG).c_str(), HUD_DIM);
                }
        } else if (step > 1) {   // the coarse lines' values along the map's edges
            for (int j = j0; j <= j1; j += step) {
                int gx, gy; toPx(j * DEG, lonC, gx, gy);
                if (gy - 3 < oy || gy + 4 > oy + MH) continue;
                drawText(canvas, ox + 2, gy - 3, fmt("%d%s", std::abs(j), j >= 0 ? "N" : "S").c_str(), HUD_DIM);
            }
            for (int k = k0; k <= k1; k += step) {
                int gx, gy; toPx(latC, k * DEG, gx, gy);
                if (gx < ox + 2 || gx + 16 > ox + MW) continue;
                int kw = ((k % 360) + 360) % 360; if (kw > 180) kw -= 360;
                drawText(canvas, gx + 2, oy + 2, fmt("%d%s", std::abs(kw), kw >= 0 ? "E" : "W").c_str(), HUD_DIM);
            }
        }
    }
    if (mark(surf.capsuleX, surf.capsuleZ, sx, sy)) { drawRectRGB(canvas, sx - 2, sy - 2, sx + 2, sy + 2, HUD_AMBER); drawText(canvas, sx + 4, sy - 3, "CAPSULE", HUD_AMBER); }
    if (surf.buggy.deployed && mark(surf.buggy.x, surf.buggy.z, sx, sy)) { drawRectRGB(canvas, sx - 2, sy - 2, sx + 2, sy + 2, HUD_CYAN); drawText(canvas, sx + 4, sy - 3, "BUGGY", HUD_CYAN); }
    if (surf.drone.deployed && mark(surf.drone.x, surf.drone.z, sx, sy)) { drawRectRGB(canvas, sx - 2, sy - 2, sx + 2, sy + 2, HUD_CYAN); drawText(canvas, sx + 4, sy - 3, "DRONE", HUD_CYAN); }
    if (scanMode >= 0) {   // the scanner's echoes of its mode where it hears them (the marked left out unless taken in), the nearest 30
        std::vector<std::pair<double, const SurfaceView::ScanEcho*>> heard;
        for (const SurfaceView::ScanEcho& e : surf.echoes) if (e.lm.kind == scanMode && !(scanSkipMarked && echoMarked(e))) heard.push_back({(e.x - px) * (e.x - px) + (e.z - pz) * (e.z - pz), &e});
        std::sort(heard.begin(), heard.end(), [](const auto& a, const auto& b2) { return a.first < b2.first; });
        if (heard.size() > 30) heard.resize(30);
        for (size_t i = 0; i < heard.size(); i++) {
            double hx, hz; echoHeardAt(*heard[i].second, hx, hz);
            if (!mark(hx, hz, sx, sy)) continue;
            if (i == 0) { drawRectRGB(canvas, sx - 3, sy - 3, sx + 3, sy + 3, HUD_CYAN); fillRectRGB(canvas, sx - 1, sy - 1, sx + 1, sy + 1, HUD_CYAN); }
            else { fillRectRGB(canvas, sx - 1, sy - 1, sx + 1, sy + 1, rgb(0, 0, 0)); const Landmark& L = heard[i].second->lm; const bool big = L.kind == LM_RUIN ? (L.sub == SC_VILLAGE || L.sub == SC_TOWN || L.sub >= SC_CITY) : L.radiusM > 400; fillRectRGB(canvas, sx, sy, sx + (big ? 1 : 0), sy + (big ? 1 : 0), HUD_CYAN); }   // a bigger echo a bigger dot (a village, a town, T-01's cities)
        }
    }
    const int onMark = markNear(mapCurX, mapCurZ, 5 * mpp);
    {   // the explorer's marks (a friend's in cyan): the glyph of what they mark (a place: a small diamond), the names of the
        // nearest twelve where they do not run into one another; B-412: the one shown over the view boxed, its name first
        std::vector<std::pair<double, const SiteMark*>> shown;
        for (const SiteMark& m : marksHere()) if (mark(m.x, m.z, sx, sy)) shown.push_back({m.index == guide.shownMark ? -1.0 : (m.x - px) * (m.x - px) + (m.z - pz) * (m.z - pz), &m});
        std::sort(shown.begin(), shown.end(), [](const auto& a, const auto& b2) { return a.first < b2.first; });
        std::vector<std::array<int, 4>> boxes;
        for (size_t i = 0; i < shown.size(); i++) {
            const SiteMark& m = *shown[i].second; const SurfaceMark& gm = guide.marks[m.index];
            mark(m.x, m.z, sx, sy);
            const bool armed = m.index == mapDelMark && realTime < mapDelUntil;
            const uint32_t col = armed ? (((int)(realTime * 4) & 1) ? HUD_RED : HUD_WHITE) : (gm.lent ? HUD_CYAN : HUD_WHITE);
            if (gm.kind < LM_COUNT) drawText(canvas, sx - 2, sy - 3, LANDMARK_KIND_SYMBOLS[gm.kind], col);
            else { drawLineRGB(canvas, sx - 2, sy, sx, sy - 2, col); drawLineRGB(canvas, sx, sy - 2, sx + 2, sy, col); drawLineRGB(canvas, sx + 2, sy, sx, sy + 2, col); drawLineRGB(canvas, sx, sy + 2, sx - 2, sy, col); }
            if (m.index == onMark || m.index == guide.shownMark) drawRectRGB(canvas, sx - 4, sy - 5, sx + 4, sy + 4, m.index == onMark ? HUD_AMBER : col);
            if (i >= 12 || gm.name.empty()) continue;
            std::string nm = trunc(upper(gm.name), 14);
            int tw = textWidth(nm.c_str()), lx = sx + 5, ly = sy - 3;
            if (lx + tw > ox + MW - 1) lx = sx - 5 - tw;
            bool clear = lx >= ox && ly >= oy;
            for (const auto& b2 : boxes) if (clear && lx < b2[2] + 2 && lx + tw > b2[0] - 2 && ly < b2[3] + 1 && ly + 7 > b2[1] - 1) clear = false;
            if (!clear) continue;
            drawText(canvas, lx, ly, nm.c_str(), col); boxes.push_back({lx, ly, lx + tw, ly + 7});
        }
    }
    if (surf.hasWaypoint && mark(surf.wpX, surf.wpZ, sx, sy)) { drawLineRGB(canvas, sx - 3, sy, sx + 3, sy, HUD_WHITE); drawLineRGB(canvas, sx, sy - 3, sx, sy + 3, HUD_WHITE); drawText(canvas, sx + 4, sy - 3, "WAYPOINT", HUD_WHITE); }
    {   // you, with your heading
        int cx = ox + MW / 2, cy = oy + MH / 2;
        double hx = std::sin(surf.player.yaw), hz = std::cos(surf.player.yaw);
        drawLineRGB(canvas, cx, cy, cx + (int)(hx * 6), cy - (int)(hz * 6), HUD_WHITE);
        drawRectRGB(canvas, cx - 1, cy - 1, cx + 1, cy + 1, HUD_WHITE);
    }
    if (mark(mapCurX, mapCurZ, sx, sy)) {   // the cursor
        const uint32_t cc = HUD_AMBER;
        drawLineRGB(canvas, sx - 7, sy, sx - 3, sy, cc); drawLineRGB(canvas, sx + 3, sy, sx + 7, sy, cc);
        drawLineRGB(canvas, sx, sy - 7, sx, sy - 3, cc); drawLineRGB(canvas, sx, sy + 3, sx, sy + 7, cc);
    }
    const int tx = ox + MW + 6;
    drawText(canvas, tx, oy, "SECTOR MAP", HUD_AMBER);
    drawText(canvas, tx, oy + 9, fmt("%.0f KM WIDE", 2 * hw / 1000).c_str(), HUD_GREEN);
    drawText(canvas, tx, oy + 18, "+/- ZOOM N CLOSE", HUD_DIM);
    {   // the cursor: its way and distance from the explorer, its place, the mark or the waypoint under it
        const double cd = std::hypot(mapCurX - px, mapCurZ - pz);
        double clat, clon; surf.site.latLonAt(mapCurX, mapCurZ, clat, clon);
        const double la = clat / DEG, lo = wrapAngle(clon) / DEG;
        drawText(canvas, tx, oy + 31, "CURSOR", HUD_DIM);
        drawText(canvas, tx, oy + 40, cd < 2 * mpp ? "HERE" : fmt("%s %s", metresString(cd).c_str(), compassName(wrap2pi(std::atan2(mapCurX - px, mapCurZ - pz)))).c_str(), HUD_WHITE);
        drawText(canvas, tx, oy + 49, fmt("%.2f%s %.2f%s", std::fabs(la), la >= 0 ? "N" : "S", std::fabs(lo), lo >= 0 ? "E" : "W").c_str(), HUD_DIM);
        drawText(canvas, tx, oy + 58, fmt("S %s", sectorName(la, lo).c_str()).c_str(), HUD_CYAN);
        if (onMark >= 0) {
            drawText(canvas, tx, oy + 67, trunc(markLabel(onMark), 17).c_str(), guide.marks[onMark].lent ? HUD_CYAN : HUD_AMBER);
            drawText(canvas, tx, oy + 76, onMark == guide.shownMark ? "SHIFT+ENTER HIDES" : "SHIFT+ENTER SHOWS", HUD_DIM);   // over the view
        }
        else if (surf.hasWaypoint && std::hypot(surf.wpX - mapCurX, surf.wpZ - mapCurZ) < 5 * mpp) drawText(canvas, tx, oy + 67, "THE WAYPOINT", HUD_WHITE);
    }
    {   // the scanner
        if (scanMode < 0) drawText(canvas, tx, oy + 89, "R SCANNER OFF", HUD_DIM);
        else {
            drawText(canvas, tx, oy + 89, fmt("SCAN %s", SCAN_MODE_NAMES[scanMode]).c_str(), HUD_CYAN);   // the scanner's own names
            double hx, hz; const SurfaceView::ScanEcho* e = scanModeHere(scanMode) && surf.scanned ? scanNearest(hx, hz) : nullptr;
            if (!scanModeHere(scanMode)) drawText(canvas, tx, oy + 98, "NONE ON THIS WORLD", HUD_DIM);
            else if (!surf.scanned) drawText(canvas, tx, oy + 98, "SCANNING", HUD_DIM);
            else if (!e) drawText(canvas, tx, oy + 98, "NONE IN 40 KM", HUD_DIM);
            else {
                drawText(canvas, tx, oy + 98, fmt("%s %s", metresString(std::hypot(hx - px, hz - pz)).c_str(), compassName(wrap2pi(std::atan2(hx - px, hz - pz)))).c_str(), HUD_CYAN);
                drawText(canvas, tx, oy + 107, trunc(echoDetail(*e), 17).c_str(), HUD_DIM);
            }
            drawText(canvas, tx, oy + 116, scanSkipMarked ? "MARKED LEFT OUT" : "MARKED HEARD (*)", HUD_DIM);
        }
    }
    drawText(canvas, tx, oy + 127, fmt("ELEV %+.0f", surf.player.y).c_str(), HUD_DIM);
    for (const FrontForecast& f : forecast) {   // the next front, or the one overhead
        if (f.tClear <= t) continue;
        const uint32_t c = f.dust ? HUD_AMBER : HUD_CYAN;
        drawText(canvas, tx, oy + 144, f.dust ? "DUST FRONT" : "RAIN FRONT", c);
        drawText(canvas, tx, oy + 153, (f.tArrive <= t ? "ENDS " + countdownString(f.tClear - t) : "IN " + countdownString(f.tArrive - t)).c_str(), c);
        break;
    }
    drawText(canvas, ox, oy + MH + 4, "ENTER MARK  DEL REMOVE  M WAYPOINT  TAB NEXT  C HOME", HUD_DIM);
}
