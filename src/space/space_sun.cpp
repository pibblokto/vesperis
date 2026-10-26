// E-08: the suns: a star's disc and glow by angle (`drawSun`: the corona, the flares, the eclipses, a companion's), the lens
// flare, and a black hole's shadow and lens (`drawBlackHole`, drawn with the sun)
#include "space_internal.h"

// true when no pixel centre of the frame lies within `halfAngle` of a direction `theta` off the view's axis. The farthest pixel
// centre lies `cornerA` off the axis, and the angle between two directions is at least the difference of their angles to a third, so
// every pixel lies more than theta - cornerA from the direction (a margin of 1e-5 radians over the passes' own rounding)
static bool coneMissesFrame(const Proj& pj, double theta, double halfAngle) {
    const double mx = std::max(std::fabs(0.5 - pj.cx), std::fabs(FBW - 0.5 - pj.cx)), my = std::max(std::fabs(0.5 - pj.cy), std::fabs(FBH - 0.5 - pj.cy));
    const double cornerA = std::atan(std::sqrt(mx * mx + my * my) / pj.f);
    return theta - cornerA > halfAngle + 1e-5;
}

// B-308: the sun's disc and glow by angle. Each pixel's ray is compared with the star's direction, so the disc is right
// whatever its size and wherever its centre is. The old screen-space circle used the radius f tan(a) d/z, which grows
// without bound as the centre nears the camera plane: with the ship parked 3.5 radii from a companion star, a swing of
// the camera flooded the whole frame with the companion's colour. `dirView` is the unit direction to the star in view
// space, `angR` its angular radius. The rays of compact stars and the point of a tiny sun stay screen-space, drawn
// from the projected centre while it lies in front of the camera.
void SpaceRenderer::drawSun(Framebuffer& fb, const Star& star, const Vec3& dirView, double angR, double t,
                            double intensity, int bank, bool atmosphere, double atmosHaze, bool depthTest, const Proj& pj, const Vec3* discUp) {
    int cls = star.cls;
    if (cls == STAR_BLACK_HOLE) { drawBlackHole(fb, star, dirView, angR, t, intensity, bank, depthTest, pj, discUp); return; }   // no disc of light at all
    double glowMul = 1.0;
    double pulse = 1.0;
    double flare = starFlare(star, t);   // a red dwarf's flare brightens the disc and the corona for a minute
    if (cls == STAR_PULSAR) {
        double ph = std::fmod(t * star.pulseHz, 1.0);
        pulse = 0.15 + 0.85 * std::pow(std::max(0.0, std::cos(ph * TAU)), 12.0);
        glowMul = 0.6 + 1.4 * pulse;
    }
    if (cls == STAR_BLUE_GIANT) glowMul = 1.8;
    if (cls == STAR_RED_GIANT) glowMul = 0.6;
    if (cls == STAR_WHITE_DWARF) glowMul = 1.1;
    if (cls == STAR_ORANGE) glowMul = 0.9;
    if (cls == STAR_RED_DWARF) glowMul = 0.8 + 0.7 * flare;
    if (cls == STAR_BLUE_WHITE) glowMul = 1.5;
    if (cls == STAR_ORANGE_GIANT) glowMul = 0.7;
    if (cls == STAR_CARBON) glowMul = 1.4;   // the soot's haze: wide and dim (its corona is halved below)
    if (cls == STAR_NEUTRON) glowMul = 1.3;   // a point of white light in a soft steady halo, no rays, no beams
    if (cls == STAR_PROTOSTAR) glowMul = 2.0;   // the accretion cloud, a wide mottled glow round a soft disc
    if (cls == STAR_WOLF_RAYET) glowMul = 1.6;   // blinding, inside the ring of its shell (drawn in the glow's pass below)
    const double S = FB_SCALE, invf = 1.0 / pj.f;
    double rA = std::max(angR, 0.6 * S * invf);   // the old floor of 0.6 px, as an angle
    double coronaA = std::max(rA * (atmosphere ? 3.2 : 2.4), 5.0 * S * invf) * glowMul;
    double outerA = std::min(coronaA * (atmosphere ? 3.0 : 1.6) * (1.0 + atmosHaze), PI - 0.01);
    int addOuter = (int)(intensity * (atmosphere ? 14 : 8) * pulse * (cls == STAR_CARBON ? 0.7 : 1.0));
    int addCorona = (int)(intensity * 30 * pulse * (cls == STAR_CARBON ? 0.5 : 1.0) * (1 + 0.5 * flare));
    // the Wolf-Rayet star's shell of shed gas, a ring at five radii (four pixels at least) a seventh of that wide, sharp
    // outside and diffuse inside, filamentary, brighter on the side it drives into (a fixed direction by the seed); the box
    // and the glow's pass reach past it, so it costs no pass of its own. Its pattern is fixed in the world when the caller
    // gives the world's up (`discUp`), else in the view
    double ringA = 0, ringW = 0, bowAng = 0;
    Vec3 r1, r2;
    if (cls == STAR_WOLF_RAYET) {
        ringA = std::max(rA * 5.0, 4.0 * S * invf); ringW = ringA / 7.0;
        outerA = std::min(std::max(outerA, ringA + 3 * ringW), PI - 0.01);
        bowAng = unitFromHash(star.seed ^ 0x5E11ULL) * TAU;
        Vec3 up = discUp ? *discUp : Vec3(0, 1, 0);
        r1 = cross(dirView, up);
        if (length2(r1) < 1e-6) r1 = cross(dirView, Vec3(1, 0, 0));
        r1 = normalize(r1); r2 = cross(dirView, r1);
    }
    // the box: the projection of the cone of half-angle outerA round the star, or the whole frame when the cone
    // reaches the camera plane
    double theta = std::acos(clampd(dirView.z, -1, 1));
    bool centreFront = dirView.z > 1e-6;
    double sx = 0, sy = 0;
    if (centreFront) { sx = pj.cx + pj.f * dirView.x / dirView.z; sy = pj.cy - pj.f * dirView.y / dirView.z; }
    int x0 = 0, x1 = FBW - 1, y0 = 0, y1 = FBH - 1;
    bool glowPass = true;
    if (centreFront && theta + outerA < 85 * DEG) {
        double rb = pj.f * std::sin(outerA) / (std::cos(theta) * std::cos(theta + outerA)) + 2;
        x0 = std::max(0, (int)(sx - rb)); x1 = std::min(FBW - 1, (int)(sx + rb));
        y0 = std::max(0, (int)(sy - rb)); y1 = std::min(FBH - 1, (int)(sy + rb));
        if (x0 > x1 || y0 > y1) return;
    } else glowPass = !coneMissesFrame(pj, theta, outerA);   // the sun beside or behind the camera: the whole frame was walked for nothing
    bool disc = rA * pj.f >= 0.8;   // under 0.8 px the sun is a point
    // a tangent basis for the granulation pattern (6 units across the disc, as the old dx * ns)
    Vec3 e1 = cross(dirView, Vec3(0, 1, 0));
    if (length2(e1) < 1e-6) e1 = cross(dirView, Vec3(1, 0, 0));
    e1 = normalize(e1);
    Vec3 e2 = cross(dirView, e1);
    double cosOuter = std::cos(outerA);
    if (glowPass) parallelFor(y1 - y0 + 1, 32, [&](int rb, int re) {
        // what the loop reads, copied into the band's own locals. Read through the closure from drawSun's frame between the
        // pixels' stores, the loop ran five times slower under one alignment of the main thread's stack; the arithmetic is the same
        const double pcx = pj.cx, pcy = pj.cy, inv = invf, cosOut = cosOuter, rAng = rA, outer = outerA, corona = coronaA, tc = t;
        const double fl = flare, pu = pulse, inten = intensity, rgA = ringA, rgW = ringW, bowA = bowAng;
        const Vec3 dir = dirView, g1 = e1, g2 = e2, s1 = r1, s2 = r2;
        const int kind = cls, bk0 = bank, aOuter = addOuter, aCorona = addCorona, xa = x0, xb = x1, W = FBW;
        const bool depth = depthTest, isDisc = disc;
        const uint64_t seed = star.seed;
        const float* const zbuf = fb.invz.data();
        Pix* const pixels = fb.idx.data();
        for (int y = y0 + rb; y < y0 + re; y++) {
            for (int x = xa; x <= xb; x++) {
                Vec3 d((x + 0.5 - pcx) * inv, -(y + 0.5 - pcy) * inv, 1.0);
                double dl = std::sqrt(dot(d, d));
                double cosA = dot(d, dir) / dl;
                if (cosA <= cosOut) continue;
                int o = y * W + x;
                if (depth && zbuf[o] > 1e-12f) continue;
                double a = std::acos(clampd(cosA, -1, 1));
                Pix& p = pixels[o];
                if (isDisc && a < rAng) {
                    double q = a / rAng;
                    double mu = std::sqrt(std::max(0.0, 1.0 - q * q));
                    double limb = 0.55 + 0.45 * mu;
                    double px = dot(d, g1) / dl, py = dot(d, g2) / dl;
                    double pl = std::sqrt(px * px + py * py);
                    double k = pl > 1e-9 ? 6.0 * q / pl : 0.0;
                    double nx = px * k, ny = py * k;
                    double v;
                    if (kind == STAR_RED_GIANT || kind == STAR_ORANGE_GIANT || kind == STAR_CARBON) {   // the giants' mottled convection; the carbon star's disc dark under its soot
                        double n = gnoise2(nx * 1.5 + tc * 0.03, ny * 1.5, seed) * 0.5 +
                                   gnoise2(nx * 4 - tc * 0.05, ny * 4, seed + 3) * 0.3;
                        v = (kind == STAR_CARBON ? 42 : 50) + 14 * n;
                        v *= limb * 0.95 + 0.05;
                    } else if (kind == STAR_ORANGE || kind == STAR_YELLOW || kind == STAR_RED_DWARF) {
                        double n = gnoise2(nx * 3 + tc * 0.01, ny * 3, seed);
                        double spotFrom = kind == STAR_RED_DWARF ? 0.36 : 0.42;   // a red dwarf is spotted all over, and its flare whitens the disc
                        double spots = (kind != STAR_YELLOW && n > spotFrom) ? 18 * (n - spotFrom) / 0.3 : 0;
                        v = (58 + 4 * gnoise2(nx * 8, ny * 8 + tc * 0.02, seed + 1)) * limb - spots * 2.0 + 12 * fl;
                    } else if (kind == STAR_BLUE_GIANT || kind == STAR_BLUE_WHITE || kind == STAR_WOLF_RAYET) {   // the Wolf-Rayet star's disc is the blue giant's
                        v = 63 * (0.8 + 0.2 * mu);
                    } else if (kind == STAR_PULSAR) {
                        v = 40 + 23 * pu;
                    } else if (kind == STAR_PROTOSTAR) {   // a soft disc, strongly limb-darkened, its light through the dust
                        double n = gnoise2(nx * 2 + tc * 0.02, ny * 2, seed) * 0.5 + gnoise2(nx * 5, ny * 5 - tc * 0.03, seed + 3) * 0.3;
                        v = (47 + 8 * n) * (0.35 + 0.65 * mu);
                    } else {
                        v = 63 * (0.75 + 0.25 * mu);
                    }
                    v *= inten;
                    // the disc is never dimmer than the glow-lit sky round it (a dim low sun inside a saturated corona read
                    // as a ring: the glow saturated the sky at 63 while the disc itself sat at 45)
                    double rim = shadeOf(p) + (aOuter + aCorona) * (1.0 - 0.2 * 0.2);
                    if (v < rim) v = rim;
                    p = pix(bk0, clampd(v, 0, 63));
                    continue;
                }
                // the glow: an outer disc and the corona, both squared falloffs from the edge of the disc
                int add = 0;
                {
                    double t1 = a <= rAng ? 1.0 : 1.0 - (a - rAng) / (outer - rAng + 1e-9);
                    if (t1 > 0) add += (int)(aOuter * INTEN_PER_SHADE * t1 * t1 + 0.5);
                    double c0 = rAng * 0.8;
                    double t2 = a <= c0 ? 1.0 : 1.0 - (a - c0) / (corona - c0 + 1e-9);
                    if (t2 > 0) add += (int)(aCorona * INTEN_PER_SHADE * t2 * t2 + 0.5);
                }
                if (kind == STAR_WOLF_RAYET && a > rAng) {   // the shell's ring
                    double ex = (a - rgA) / rgW;
                    double ring = ex > 0 ? std::exp(-4.0 * ex * ex) : std::exp(-0.5 * ex * ex);
                    if (ring > 0.01) {
                        double px = dot(d, s1) / dl, py = dot(d, s2) / dl;
                        double ang = std::atan2(py, px);
                        double fil = 0.6 + 0.4 * gnoise2(std::cos(ang) * 4.5 + 3.1, std::sin(ang) * 4.5 + ex * 0.7, seed + 17);
                        double bow = 0.7 + 0.3 * std::cos(ang - bowA);
                        add += (int)(inten * 34 * ring * fil * bow * INTEN_PER_SHADE + 0.5);
                    }
                }
                if (add <= 0) continue;
                if (kind == STAR_PROTOSTAR && a > rAng) add = (int)(add * (0.6 + 0.4 * (0.5 + 0.5 * gnoise3(d / dl * 6.0, seed + 9))));   // the cloud's wisps
                int bk = intenOf(p) == 0 ? bk0 : bankOf(p);
                p = pixI(bk, intenOf(p) + add);
            }
        }
    });
    // the protostar's dust disc, seen edge-on from the plane of its worlds: a band of glow along the orbital plane through
    // the star, narrow at the star and flaring to 0.05 + 0.3 a (radians) at the angle a from it, gone 0.6 rad out, with the dark
    // lane of the disc's own shadow along its middle. Its own pass over the frame, skipping cheaply every pixel outside the strip
    if (cls == STAR_PROTOSTAR && discUp && !coneMissesFrame(pj, theta, 0.6)) {   // not when the strip's cone misses the frame
        const Vec3 n = *discUp;
        const double cosBand = std::cos(0.6);
        parallelFor(FBH, 32, [&](int rb, int re) {
            for (int y = rb; y < re; y++) {
                for (int x = 0; x < FBW; x++) {
                    Vec3 d((x + 0.5 - pj.cx) * invf, -(y + 0.5 - pj.cy) * invf, 1.0);
                    double il = 1.0 / std::sqrt(dot(d, d));
                    double hs = dot(d, n) * il;
                    if (hs > 0.3 || hs < -0.3) continue;
                    double cosA = dot(d, dirView) * il;
                    if (cosA <= cosBand) continue;
                    int o = y * FBW + x;
                    if (depthTest && fb.invz[o] > 1e-12f) continue;
                    double a = std::acos(clampd(cosA, -1, 1));
                    if (a <= rA) continue;
                    double hAng = std::asin(clampd(hs, -1, 1));
                    double hw = 0.05 + 0.3 * a, along = 1.0 - a / 0.6;
                    double g = std::exp(-(hAng * hAng) / (hw * hw)) * along;
                    double lw = 0.012 + 0.06 * a;
                    g *= 1 - 0.65 * std::exp(-(hAng * hAng) / (lw * lw)) * smoothstep(0.0, 0.08, a);
                    g *= 0.8 + 0.2 * (0.5 + 0.5 * gnoise3(d * il * 9.0, star.seed + 11));
                    int add = (int)(intensity * 24 * g * INTEN_PER_SHADE + 0.5);
                    if (add <= 0) continue;
                    Pix& p = fb.idx[o];
                    int bk = intenOf(p) == 0 ? bank : bankOf(p);
                    p = pixI(bk, intenOf(p) + add);
                }
            }
        });
    }
    if (!centreFront) return;
    // rays for compact stars, from the projected centre, outside the disc
    if (cls == STAR_WHITE_DWARF || cls == STAR_PULSAR || cls == STAR_BLUE_GIANT || cls == STAR_BLUE_WHITE || cls == STAR_WOLF_RAYET) {   // the blue-white star's cross of four; S-05: the Wolf-Rayet star's too
        double coronaPx = std::min(pj.f * std::tan(std::min(coronaA, 1.4)), (double)FBW);
        double rPx = pj.f * std::tan(std::min(rA, 1.4));
        double len = coronaPx * ((cls == STAR_BLUE_GIANT || cls == STAR_BLUE_WHITE || cls == STAR_WOLF_RAYET) ? 1.4 : 2.2) * (cls == STAR_PULSAR ? pulse : 1.0);
        int arms = (cls == STAR_WHITE_DWARF || cls == STAR_BLUE_WHITE || cls == STAR_WOLF_RAYET) ? 4 : (cls == STAR_BLUE_GIANT ? 3 : 2);
        double rot = cls == STAR_PULSAR ? t * 0.8 : 0.0;
        for (int a = 0; a < arms; a++) {
            double ang = rot + a * PI / arms * 2.0 / (arms == 4 ? 2.0 : 1.0);
            double dx = std::cos(ang), dy = std::sin(ang);
            int n = std::min((int)len, 2 * FBW);
            for (int i = std::max(1, (int)rPx); i < n; i++) {
                double f = 1.0 - (double)i / n;
                int x = (int)(sx + dx * i), y = (int)(sy + dy * i);
                int x2 = (int)(sx - dx * i), y2 = (int)(sy - dy * i);
                int add = (int)(40 * f * f * intensity);
                for (int k = 0; k < 2; k++) {
                    int px = k ? x2 : x, py = k ? y2 : y;
                    if (px < 0 || py < 0 || px >= FBW || py >= FBH) continue;
                    if (depthTest && fb.invz[py * FBW + px] > 1e-12f) continue;
                    Pix& p = fb.idx[py * FBW + px];
                    int bk = intenOf(p) == 0 ? bank : bankOf(p);
                    p = pixI(bk, intenOf(p) + toInten(add));
                }
            }
        }
    }
    if (!disc) {
        int x = (int)sx, y = (int)sy;
        for (int dy = 0; dy < FB_SCALE; dy++)
            for (int dx = 0; dx < FB_SCALE; dx++)
                if (x + dx >= 0 && y + dy >= 0 && x + dx < FBW && y + dy < FBH && !(depthTest && fb.invz[(y + dy) * FBW + x + dx] > 1e-12f)) fb.idx[(y + dy) * FBW + x + dx] = pix(bank, clampd(63 * intensity, 0, 63));
    }
}

void SpaceRenderer::drawLensFlare(Framebuffer& fb, double sx, double sy, double sunRadiusPx, double intensity, int bank, const Proj& pj) {
    double r = std::min(sunRadiusPx, 0.2 * FBW);   // B-308: a sun close to the camera plane projects to an absurd radius
    if (sx < -FBW * 0.15 || sx > FBW * 1.15 || sy < -FBH * 0.15 || sy > FBH * 1.15) return;
    double dx = pj.cx - sx, dy = pj.cy - sy;
    double S = FB_SCALE;
    const double ks[4] = {0.45, 0.8, 1.2, 1.6};
    const double rad[4] = {0.45, 0.8, 0.3, 1.1};
    const int add[4] = {6, 4, 5, 3};
    for (int i = 0; i < 4; i++) {
        double px = sx + dx * ks[i], py = sy + dy * ks[i];
        double rr = std::max(2.0 * S, r * rad[i]) + 2 * S;
        fb.glowDisc(px, py, rr, rr * 0.3, (int)(add[i] * intensity + 0.5), bank);
    }
    // a faint streak through the sun along the same line
    int n = (int)(std::sqrt(dx * dx + dy * dy) * 0.5);
    double nx = dx, ny = dy, nl = std::sqrt(nx * nx + ny * ny) + 1e-9;
    nx /= nl; ny /= nl;
    for (int i = -n; i <= n; i++) {
        double f = 1.0 - std::fabs((double)i) / (n + 1);
        int x = (int)(sx + nx * i * 2), y = (int)(sy + ny * i * 2);
        if (x < 0 || y < 0 || x >= FBW || y >= FBH) continue;
        Pix& p = fb.idx[y * FBW + x];
        if (fb.invz[y * FBW + x] > 1e-12f) continue;
        int bk = intenOf(p) == 0 ? bank : bankOf(p);
        p = pixI(bk, intenOf(p) + toInten(3.0 * f * f * intensity));
    }
}

// S-06: the black hole. No disc of light: a shadow of 2.6 radii (the photon ring; the class's radius stands for the Schwarzschild
// radius) in the sky, and behind it the stars and the band bent into arcs by a lens: a pixel at the angle th from the hole shows
// what lies at th - thE^2 / th along the same great circle (the other side when that is negative), with the Einstein angle
// thE = sqrt(R / D), the weak-field deflection of a source at infinity; the bend fades out toward the edge of the region (2.6 thE)
// so there is no seam. The bent directions are computed on a grid of one logical pixel and interpolated, the frame before the
// lens copied once (the bent rays read anywhere in it). The accretion disc lies in the plane of the worlds, a slab half a radius
// thick from 3 to 9 radii, bright and hot inside to a dim orange outside, brighter on the side that comes toward the viewer (the
// spin by the seed): its near half is seen along the straight ray and drawn over the shadow, its far half along the bent ray from
// the lens plane, so it arches over and under the shadow. Two in five blow a jet along the plane's normal, drawn before the lens
// so the shadow covers its root and the lens bends it. Rows in parallel; the whole frame when the cone reaches the camera plane
void SpaceRenderer::drawBlackHole(Framebuffer& fb, const Star& star, const Vec3& dirView, double angR, double t, double intensity,
                                  int bank, bool depthTest, const Proj& pj, const Vec3* discUp) {
    (void)t;
    const double S = FB_SCALE, invf = 1.0 / pj.f;
    const double rA = std::max(angR, 0.8 * S * invf);
    const double thE = std::sqrt(rA);
    const double regionA = std::min(2.6 * thE, 80 * DEG);
    const double shadowA = 2.6 * rA;
    const double cosRegion = std::cos(regionA), cosShadow = std::cos(shadowA);
    const double R = star.radiusKm, D = R / std::sin(std::max(angR, 1e-7));
    const Vec3 H = dirView * D;
    Vec3 n = discUp ? *discUp : Vec3(0, 1, 0);
    if (length2(n) < 1e-9) n = Vec3(0, 1, 0);
    n = normalize(n);
    const double spin = unitFromHash(star.seed ^ 0x5B1AULL) < 0.5 ? 1.0 : -1.0;
    const bool jet = unitFromHash(star.seed ^ 0x3E7AULL) < 0.4;
    const double rIn = 3.0 * R, rOut = 9.0 * R, half = 0.25 * R;
    const double thDisc = std::min(regionA, std::asin(std::min(1.0, rOut / D)) * 1.15 + 0.02);
    const double cosTest = std::cos(std::max(thDisc, std::min(regionA, 1.5 * thE)));
    // the box: the cone of regionA round the hole, or the whole frame when it reaches the camera plane
    double theta = std::acos(clampd(dirView.z, -1, 1));
    bool centreFront = dirView.z > 1e-6;
    double sx = 0, sy = 0;
    if (centreFront) { sx = pj.cx + pj.f * dirView.x / dirView.z; sy = pj.cy - pj.f * dirView.y / dirView.z; }
    int x0 = 0, x1 = FBW - 1, y0 = 0, y1 = FBH - 1;
    if (centreFront && theta + regionA < 85 * DEG) {
        double rb = pj.f * std::sin(regionA) / (std::cos(theta) * std::cos(theta + regionA)) + 2;
        x0 = std::max(0, (int)(sx - rb)); x1 = std::min(FBW - 1, (int)(sx + rb));
        y0 = std::max(0, (int)(sy - rb)); y1 = std::min(FBH - 1, (int)(sy + rb));
        if (x0 > x1 || y0 > y1) return;
    } else if (theta - regionA > 95 * DEG) return;
    // the jet, before the lens
    if (jet) {
        for (int side = -1; side <= 1; side += 2) {
            for (int k = 0; k < 20; k++) {
                double f = (double)k / 19;
                Vec3 p = H + n * (side * R * (4.0 + 26.0 * f));
                if (p.z <= 1) continue;
                double px = pj.cx + pj.f * p.x / p.z, py = pj.cy - pj.f * p.y / p.z;
                double rpx = std::max(pj.f * R * (0.5 + 1.2 * f) / length(p), 1.0 * S);
                if (rpx > 60 * S) rpx = 60 * S;
                int a = (int)(intensity * 9 * (1 - f) * (1 - f) + 0.5);
                if (a >= 1) fb.glowDisc(px, py, rpx, rpx * 0.3, a, bank, depthTest);
            }
        }
    }
    if (coneMissesFrame(pj, theta, regionA)) return;   // no pixel within the lens's region: the copy, the grid and the pass write nothing
    // the frame before the lens
    std::vector<Pix> before(fb.idx);
    // the bent directions on the grid (not unit: only their projection and their line are used)
    const int g = std::max(1, FB_SCALE);
    const int gw = (x1 - x0) / g + 2, gh = (y1 - y0) / g + 2;
    std::vector<float> gx(gw * gh), gy(gw * gh), gz(gw * gh);
    parallelFor(gh, 8, [&](int jb, int je) {
        for (int j = jb; j < je; j++) for (int i = 0; i < gw; i++) {
            Vec3 d((x0 + i * g + 0.5 - pj.cx) * invf, -(y0 + j * g + 0.5 - pj.cy) * invf, 1.0);
            Vec3 dn = d / std::sqrt(dot(d, d));
            double cosA = clampd(dot(dn, dirView), -1, 1);
            Vec3 u = dn - dirView * cosA;
            double ul = std::sqrt(dot(u, u));
            double th = std::atan2(ul, cosA);
            Vec3 out = dn;
            if (ul > 1e-9 && th > 1e-6) {
                u = u / ul;
                double thc = std::max(th, shadowA);
                double fade = smoothstep(regionA, 0.6 * regionA, thc);
                double ths = thc - fade * thE * thE / thc;
                out = dirView * std::cos(ths) + u * std::sin(ths);
            }
            gx[j * gw + i] = (float)out.x; gy[j * gw + i] = (float)out.y; gz[j * gw + i] = (float)out.z;
        }
    });
    // the disc along a ray from `o` in the direction `dd`: the brightest of the slab's three planes, hits before the hole's
    // transverse plane (the near half) or beyond it (the far half); 0 = miss
    auto discAlong = [&](const Vec3& o, const Vec3& dd, bool farHalf) -> double {
        double dn_ = dot(dd, n);
        if (std::fabs(dn_) < 1e-12) return 0;
        double ddl = std::sqrt(dot(dd, dd));
        double best = 0;
        for (int k = -1; k <= 1; k++) {
            double tt = (dot(H - o, n) + k * half) / dn_;
            if (tt <= 0) continue;
            Vec3 rel = o + dd * tt - H;
            double along = dot(rel, dirView);
            if (farHalf ? along < 0 : along > 0) continue;
            Vec3 rp = rel - n * dot(rel, n);
            double r = std::sqrt(dot(rp, rp));
            if (r < rIn || r > rOut) continue;
            double rr = (r - rIn) / (rOut - rIn);
            Vec3 vel = cross(n, rp / r) * spin;
            double dop = 1.0 + 0.6 * dot(vel, dd) * (-1.0 / ddl);
            double b = (1.0 - 0.72 * rr) * dop;
            if (b > best) best = b;
        }
        return best;
    };
    parallelFor(y1 - y0 + 1, 16, [&](int rb, int re) {
        for (int y = y0 + rb; y < y0 + re; y++) {
            int j = (y - y0) / g; double fy = (double)((y - y0) - j * g) / g;
            for (int x = x0; x <= x1; x++) {
                Vec3 d((x + 0.5 - pj.cx) * invf, -(y + 0.5 - pj.cy) * invf, 1.0);
                double dl = std::sqrt(dot(d, d));
                double cosA = dot(d, dirView) / dl;
                if (cosA <= cosRegion) continue;
                int o = y * FBW + x;
                if (depthTest && fb.invz[o] > 1e-12f) continue;
                Pix& p = fb.idx[o];
                Vec3 dn = d / dl;
                double b = 0;
                if (cosA > cosShadow) {
                    p = pixI(0, 0);
                    b = discAlong(Vec3(0, 0, 0), dn, false);
                } else {
                    int i = (x - x0) / g; double fx = (double)((x - x0) - i * g) / g;
                    int k = j * gw + i;
                    double w00 = (1 - fx) * (1 - fy), w10 = fx * (1 - fy), w01 = (1 - fx) * fy, w11 = fx * fy;
                    Vec3 ds(gx[k] * w00 + gx[k + 1] * w10 + gx[k + gw] * w01 + gx[k + gw + 1] * w11,
                            gy[k] * w00 + gy[k + 1] * w10 + gy[k + gw] * w01 + gy[k + gw + 1] * w11,
                            gz[k] * w00 + gz[k + 1] * w10 + gz[k + gw] * w01 + gz[k + gw + 1] * w11);
                    if (ds.z > 1e-6) {
                        int ix = (int)(pj.cx + pj.f * ds.x / ds.z), iy = (int)(pj.cy - pj.f * ds.y / ds.z);
                        p = (ix >= 0 && iy >= 0 && ix < FBW && iy < FBH) ? before[iy * FBW + ix] : pixI(0, 0);
                    } else p = pixI(0, 0);
                    if (cosA > cosTest) {
                        b = discAlong(Vec3(0, 0, 0), dn, false);
                        if (b <= 0) b = discAlong(dn * (D / cosA), ds, true);
                    }
                }
                if (b > 0) p = pix(bank, clampd(63 * intensity * b, 0, 63));
            }
        }
    });
}
