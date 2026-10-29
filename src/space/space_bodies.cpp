// E-08: what orbits and streams: a belt's rocks round the ship, a companion's gas drawn into a black hole, a giant's envelope
// stripped by a Wolf-Rayet star's wind, a comet's coma and tails
#include "space_internal.h"

// the rocks of a belt around the ship, hashed in co-rotating cells (StarSystem::beltRockAt): a point when
// under two pixels across, else a lumpy mesh (a sphere with fbm radii) tumbling slowly, lit by the star, depth-tested
// against the globes drawn before it
void SpaceRenderer::drawBeltRocks(Framebuffer& fb, const SpaceContext& c, int k) {
    const StarSystem& sys = *c.sys;
    const Belt& bl = sys.belts[k];
    Vec3 rel = c.shipPos - sys.star.pos;
    double r = std::sqrt(rel.x * rel.x + rel.z * rel.z), w = bl.outerKm - bl.innerKm;
    if (r < bl.innerKm - 0.1 * w || r > bl.outerKm + 0.1 * w || std::fabs(rel.y) > 0.03 * r + 2 * BELT_CELL_KM) return;   // the ship is not in this belt
    struct Seen { double d; BeltRock rk; };
    std::vector<Seen> seen;
    const double S = FB_SCALE;
    sys.forBeltRocksNear(k, c.shipPos, c.t, 4, 4, 1, [&](const BeltRock& rk) {
        Vec3 v = c.cam * (rk.pos - c.shipPos);
        if (v.z <= rk.radiusKm * 1.5) return;
        double d = length(v);
        double rpx = proj.f * rk.radiusKm / d;
        if (rpx < 0.35 * S) return;
        double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
        if (sx < -rpx * 2 - 4 || sy < -rpx * 2 - 4 || sx > FBW + rpx * 2 + 4 || sy > FBH + rpx * 2 + 4) return;
        seen.push_back({d, rk});
    });
    std::sort(seen.begin(), seen.end(), [](const Seen& a, const Seen& b) { return a.d > b.d; });
    Vec3 starDir = normalize(sys.star.pos - c.shipPos);   // the star is far: one direction serves every rock
    double lf = clampd(lightFactor(sys.star.luminosity, length(sys.star.pos - c.shipPos)), 0, 1.15);
    RasterParams rp; rp.bank = 20;   // bank 20: the rock ramp (see setupPalette)
    for (const Seen& sn : seen) {
        const BeltRock& rk = sn.rk;
        Vec3 v = c.cam * (rk.pos - c.shipPos);
        double rpx = proj.f * rk.radiusKm / sn.d;
        lastBeltRocks++;
        if (rpx < 1.6 * S) {   // a point, bright with its phase
            double phase = 0.5 + 0.5 * dot(normalize(c.shipPos - rk.pos), starDir);
            double val = clampd(14 + 34 * phase * lf + 8 * rpx / S, 6, 50);
            int x = (int)(proj.cx + proj.f * v.x / v.z), y = (int)(proj.cy - proj.f * v.y / v.z);
            float iz = (float)(1.0 / v.z);
            for (int dy = 0; dy < FB_SCALE; dy++)
                for (int dx = 0; dx < FB_SCALE; dx++) {
                    int px = x + dx, py = y + dy;
                    if (px < 0 || py < 0 || px >= FBW || py >= FBH) continue;
                    int o = py * FBW + px;
                    if (iz > fb.invz[o]) { fb.idx[o] = pix(20, val); fb.invz[o] = iz; }
                }
            continue;
        }
        lastBeltMeshes++;
        const int NA = rpx > 8 * S ? 12 : 6, NE = rpx > 8 * S ? 6 : 3;
        Mat3 rot = Mat3::axisAngle(rk.axis, rk.tumble);
        Vec3 pts[13][7];
        for (int e = 0; e <= NE; e++)
            for (int a = 0; a <= NA; a++) {
                double el = (e / (double)NE - 0.5) * PI, az = a * TAU / NA;
                Vec3 n = e == 0 ? Vec3(0, -1, 0) : (e == NE ? Vec3(0, 1, 0) : Vec3(std::cos(el) * std::cos(az), std::sin(el), std::cos(el) * std::sin(az)));
                double rr = rk.radiusKm * (1 + 0.38 * fbm3(n * 1.7 + Vec3(3.1, 1.7, 5.3), rk.seed, 3));
                pts[a][e] = rk.pos + rot * (n * rr);
            }
        auto face = [&](const Vec3* p, int n) {
            Vec3 nrm = normalize(cross(p[1] - p[0], p[n - 1] - p[0]));
            if (dot(nrm, p[0] - c.shipPos) > 0) return;   // faces away from the ship
            double light = 0.06 + 0.94 * std::max(0.0, dot(nrm, starDir)) * lf;
            double shade = 52 * std::pow(light, 0.6);
            RVert q[4];
            for (int i = 0; i < n; i++) { Vec3 vv = c.cam * (p[i] - c.shipPos); q[i].x = vv.x; q[i].y = vv.y; q[i].z = vv.z; q[i].shade = shade; }
            rasterPolygon(fb, q, n, rp, proj);
        };
        for (int e = 0; e < NE; e++)
            for (int a = 0; a < NA; a++) {
                if (e == 0) { Vec3 tri[3] = {pts[a][0], pts[a + 1][1], pts[a][1]}; face(tri, 3); }
                else if (e == NE - 1) { Vec3 tri[3] = {pts[a][e], pts[a + 1][e], pts[a][NE]}; face(tri, 3); }
                else { Vec3 quad[4] = {pts[a][e], pts[a + 1][e], pts[a + 1][e + 1], pts[a][e + 1]}; face(quad, 4); }
            }
    }
}

// S-06: a black hole's companion is being drawn out: a stream of its gas from the limb facing the hole to the edge of the
// disc, thin at the companion and flaring into the disc, bent back against the companion's motion (the Coriolis side) and
// wavering slowly, drawn before the companion's own disc in its bank. The hole's disc and shadow are drawn with the sun
void SpaceRenderer::drawAccretionStream(Framebuffer& fb, const SpaceContext& c, int bi, int bank) {
    const StarSystem& sys = *c.sys;
    const Body& b = sys.bodies[bi];
    Vec3 posW = sys.bodyPos(bi, c.t);
    Vec3 toHole = sys.star.pos - posW;
    double dist = length(toHole);
    if (dist < 1e-6) return;
    toHole = toHole / dist;
    Vec3 fwd = sys.bodyVel(bi, c.t);
    fwd = fwd - toHole * dot(fwd, toHole);
    double fl = length(fwd);
    fwd = fl > 1e-9 ? fwd / fl : Vec3(0, 1, 0);
    const double rc = b.radiusKm, rEnd = sys.star.radiusKm * 9.0, ph = (double)(b.seed % 5);
    const double span = std::max(dist - rc * 0.9 - rEnd, rc * 0.5);
    const int N = 40;
    for (int k = 0; k <= N; k++) {
        double f = (double)k / N;
        double wob = 0.06 * std::sin(f * 11.0 + c.t * 0.02 + ph);
        Vec3 p = posW + toHole * (rc * 0.9 + span * f) - fwd * (span * (0.22 * f * f + wob * f));
        Vec3 v = c.cam * (p - c.shipPos);
        if (v.z <= 1) continue;
        double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
        if (sx < -80 * FB_SCALE || sy < -80 * FB_SCALE || sx > FBW + 80 * FB_SCALE || sy > FBH + 80 * FB_SCALE) continue;
        double rKm = rc * (0.12 + 0.3 * (1 - f)) + rEnd * 0.12 * f;
        double rpx = std::max(proj.f * rKm / length(v), 1.0 * FB_SCALE);
        if (rpx > 80 * FB_SCALE) rpx = 80 * FB_SCALE;
        int a = (int)((9 + 9 * f) * (0.85 + 0.15 * std::sin(f * 23.0 - c.t * 0.05)) + 0.5);
        if (a < 1) continue;
        fb.glowDisc(sx, sy, rpx, rpx * 0.4, a, bank);
    }
}

// S-05: the envelope a Wolf-Rayet star's wind strips from a giant: a tail of its own cloud from the limb away from the star, four
// radii long, widening and thinning, curling back along the orbit and wavering slowly, in filaments along the flow. B-427: it was
// forty glow discs of up to 14 shades drawn before the globe; heaped near the root they reached the ramp's top (white) in a blob
// behind the giant, the sky's haze kept its own bank under them, and seen from the night side the tail streaming toward the
// camera was hidden by the globe. Now splats about half their half-width apart along the axis each carry the light of their
// stretch of the tube, so the side view is the profile's (24 shades at the root, a dim share of the lit face) and a view down
// the tail sums to a soft ceiling of 36; drawn after the globe, each splat behind whatever is nearer (the globe hides the root
// from the day side; from the night side the tail veils the globe), in the giant's bank where it outshines the pixel and the
// two banks dithered where neither does (one bank a pixel: the switch drew a hard outline where it crossed the band). In a
// sky its light is added by luminance (below). The filaments run across a splat by the axis's direction in the frame, faded as
// the tail turns end-on. The light is sampled once a block of 2 x 2 pixels at 3x and 4x, bands of rows in parallel, each block
// summed over the splats in their order (band-pinned)
void SpaceRenderer::drawStrippedTail(Framebuffer& fb, const SpaceContext& c, int bi, int bank) {
    const StarSystem& sys = *c.sys;
    const Body& b = sys.bodies[bi];
    const double dark = c.skyMode ? c.skyDark : 1.0;
    if (dark < 0.05) return;
    const Vec3 posW = sys.bodyPos(bi, c.t);
    const Vec3 away = normalize(posW - sys.star.pos);
    Vec3 back = -sys.bodyVel(bi, c.t);
    back = back - away * dot(back, away);
    const double bl = length(back);
    back = bl > 1e-9 ? back / bl : Vec3(0, 1, 0);
    const Vec3 side = cross(away, back);
    const double R = b.radiusKm, ph = (double)(b.seed % 7);
    auto axis = [&](double f) {
        const double wob = 0.25 * std::sin(f * 7.0 + c.t * 0.01 + ph) + 0.15 * std::sin(f * 13.0 - c.t * 0.016);
        return posW + away * (R * (1.0 + 4.0 * f * f)) + back * (R * 0.6 * f * f) + side * (R * wob * f);
    };
    const double B0 = 24.0 * dark, cap = 36.0 * dark, depthUnit = c.skyMode ? 1000.0 : 1.0;
    const int g = FB_SCALE >= 3 ? 2 : 1;   // the light sampled once a g x g block (at 3x and 4x the finest filament spans tens of pixels)
    constexpr int FN = 16;
    static const double BAYER4[16] = {0.5 / 16, 8.5 / 16, 2.5 / 16, 10.5 / 16, 12.5 / 16, 4.5 / 16, 14.5 / 16, 6.5 / 16,
                                      3.5 / 16, 11.5 / 16, 1.5 / 16, 9.5 / 16, 15.5 / 16, 7.5 / 16, 13.5 / 16, 5.5 / 16};
    struct Splat { double sx, sy, r, invR, invR2, z, inv2r, a, ux, uy; float fil[FN + 1]; int x0, x1, y0, y1; };
    std::vector<Splat> sp;
    for (double f = 0; f < 1.0;) {
        const double hw = R * (0.55 + 0.9 * f);
        const double speed = length(axis(std::min(f + 0.01, 1.0)) - axis(f)) / 0.01;
        const double df = std::min({0.1, 0.55 * hw / std::max(speed, 1e-9), 1.0 - f});
        if (df < 1e-6) break;
        const double fm = f + 0.5 * df, rKm = R * (0.55 + 0.9 * fm);
        const double ds = length(axis(f + df) - axis(f));
        f += df;
        const Vec3 P = axis(fm), v = c.cam * (P - c.shipPos);
        const double near = smoothstep(0.5, 2.0, v.z / rKm);   // a stretch round the camera is too near to see as a shape
        if (near <= 0) continue;
        Splat s;
        s.sx = proj.cx + proj.f * v.x / v.z; s.sy = proj.cy - proj.f * v.y / v.z;
        s.r = proj.f * rKm / v.z;
        if (s.r < 0.6 * g || s.sx + s.r < 0 || s.sy + s.r < 0 || s.sx - s.r > FBW || s.sy - s.r > FBH) continue;
        s.x0 = std::max(0, (int)std::floor(s.sx - s.r)); s.x1 = std::min(FBW - 1, (int)std::ceil(s.sx + s.r));
        s.y0 = std::max(0, (int)std::floor(s.sy - s.r)); s.y1 = std::min(FBH - 1, (int)std::ceil(s.sy + s.r));
        if (s.x0 > s.x1 || s.y0 > s.y1) continue;
        s.invR = 1.0 / s.r; s.invR2 = s.invR * s.invR; s.z = v.z; s.inv2r = 0.5 / rKm;
        s.a = B0 * std::pow(1.0 - fm, 1.3) * near * ds * 15.0 / (16.0 * rKm);   // a tube of radius rKm seen side-on: its column B
        const Vec3 v2 = c.cam * (axis(std::min(fm + 0.01, 1.0)) - c.shipPos);
        double tx = 1, ty = 0, along = 0;
        if (v2.z > 1e-6) {
            tx = (proj.cx + proj.f * v2.x / v2.z) - s.sx; ty = (proj.cy - proj.f * v2.y / v2.z) - s.sy;
            const double tl = std::sqrt(tx * tx + ty * ty), side3 = proj.f * length(v2 - v) / v.z;
            if (tl > 1e-9) { tx /= tl; ty /= tl; along = side3 > 1e-9 ? tl / side3 : 0; }
        }
        s.ux = -ty; s.uy = tx;
        const double amp = smoothstep(0.15, 0.5, along);   // end-on, a splat's across has no direction
        for (int j = 0; j <= FN; j++) {
            const double u = -1.0 + 2.0 * j / FN, q = fm * 1.6 - c.t * 0.004;
            const double n = 0.9 * gnoise2(u * (2.0 + 1.5 * fm) + ph * 3.7, q, b.seed ^ 0x7A11ULL) + 0.45 * gnoise2(u * (5.0 + 3.0 * fm), q * 2.0, b.seed ^ 0x7A12ULL);
            s.fil[j] = (float)std::exp(amp * 2.4 * n);
        }
        sp.push_back(s);
    }
    if (sp.empty()) return;
    int Y0 = FBH, Y1 = -1;
    for (const Splat& s : sp) { Y0 = std::min(Y0, s.y0); Y1 = std::max(Y1, s.y1); }
    // in a sky (the surface's palette) the sky's bank is black under shade 30 and steep above it, so the tail's light is added there
    // by luminance: the pixel's read from the palette, the tail's at most that of the bank's shade 40 (the lit side is drawn from 43
    // up), the sum written as the bank's shade of that luminance (the band's pass carries the sky's pixels into its bank by luminance)
    std::vector<float> intenLum;
    std::vector<int> lumInten;
    double lumTop = 0;
    if (c.skyMode) {
        double lm[64], run = 0;
        for (int k = 0; k < 64; k++) { const uint8_t* q = &fb.pal[(bank * 64 + k) * 3]; run = std::max(run, 0.2126 * q[0] + 0.7152 * q[1] + 0.0722 * q[2]); lm[k] = run; }
        intenLum.resize(INTEN_MASK + 1);
        for (int i = 0; i <= INTEN_MASK; i++) {
            const double sh = std::min(63.0, i / (double)INTEN_PER_SHADE);
            const int k = std::min(62, (int)sh);
            intenLum[i] = (float)(lm[k] + (lm[k + 1] - lm[k]) * (sh - k));
        }
        lumInten.resize(1025);
        for (int j = 0, k = 0; j <= 1024; j++) {
            const double L = j * (255.0 / 1024);
            while (k < 62 && lm[k + 1] < L) k++;
            const double f = lm[k + 1] > lm[k] ? clampd((L - lm[k]) / (lm[k + 1] - lm[k]), 0, 1) : 0.0;
            lumInten[j] = (int)((k + f) * INTEN_PER_SHADE + 0.5);
        }
        lumTop = lm[40];
    }
    parallelFor((Y1 - Y0) / g + 1, 4, [&](int bb, int be) {   // bands of g rows, each its own (band-pinned)
        std::vector<float> acc(FBW / g + 2), zb(FBW / g + 2);
        for (int band = bb; band < be; band++) {
            const int yb = Y0 + band * g, ye = std::min(yb + g, FBH) - 1;
            int xa = FBW, xb = -1;
            for (const Splat& s : sp) if (ye >= s.y0 && yb <= s.y1) { xa = std::min(xa, s.x0); xb = std::max(xb, s.x1); }
            if (xb < xa) continue;
            const int nk = (xb - xa) / g + 1;
            for (int k = 0; k < nk; k++) {   // a block's depth: its farthest pixel's (the tail's light reaches the limb, no dark seam)
                float iz = 3e38f;
                for (int y = yb; y <= ye; y++) for (int x = xa + k * g; x < std::min(xa + k * g + g, FBW); x++) iz = std::min(iz, fb.invz[y * FBW + x]);
                acc[k] = 0; zb[k] = iz > 1e-12f ? (float)(1.0 / (iz * depthUnit)) : 3e38f;
            }
            const double cy = yb + 0.5 * g;   // the blocks' centres
            for (const Splat& s : sp) {
                const double ry = cy - s.sy, ry2 = ry * ry * s.invR2;
                if (ry2 >= 1) continue;
                const double span = s.r * std::sqrt(1 - ry2);
                const int k0 = std::max(0, (int)std::ceil((s.sx - span - xa) / g - 0.5)), k1 = std::min(nk - 1, (int)std::floor((s.sx + span - xa) / g - 0.5));
                for (int k = k0; k <= k1; k++) {
                    const double rx = xa + (k + 0.5) * g - s.sx, d2 = rx * rx * s.invR2 + ry2;
                    if (d2 >= 1) continue;
                    const double vis = clampd(0.5 + ((double)zb[k] - s.z) * s.inv2r, 0, 1);
                    if (vis <= 0) continue;
                    const double fu = ((rx * s.ux + ry * s.uy) * s.invR + 1.0) * 0.5 * FN;
                    const int j = clampi((int)fu, 0, FN - 1);
                    const double fil = s.fil[j] + (s.fil[j + 1] - s.fil[j]) * clampd(fu - j, 0, 1);
                    acc[k] += (float)(s.a * (1 - d2) * (1 - d2) * fil * vis);
                }
            }
            for (int k = 0; k < nk; k++) {
                if (acc[k] <= 0) continue;
                const double T = cap * (1.0 - std::exp(-acc[k] / cap));
                const int add = (int)(T * INTEN_PER_SHADE + 0.5);
                if (add <= 0) continue;
                for (int y = yb; y <= ye; y++)
                    for (int x = xa + k * g; x < std::min(xa + k * g + g, FBW); x++) {
                        Pix& p = fb.idx[y * FBW + x];
                        const int v = intenOf(p);
                        if (c.skyMode) {
                            const double L = (bankOf(p) == bank ? intenLum[v] : 0.0) + lumTop * T / 36.0;
                            const int ni = lumInten[clampi((int)(L * (1024 / 255.0) + 0.5), 0, 1024)];
                            if (ni > v) p = pixI(bank, ni);
                            continue;
                        }
                        const double q = (double)add / (add + v), th = 0.3 + 0.4 * BAYER4[(y & 3) * 4 + (x & 3)];   // the banks dithered where neither outshines
                        p = pixI(q > th ? bank : bankOf(p), v + add);
                    }
            }
        }
    });
}

// M5-07/N0-01 (B-201): a comet's coma, its straight ion tail with knots streaming outward, a dust tail
// curving back along the orbit and, close up, jets from the sunlit side of the nucleus. The orbit itself
// is untouched (the sky stays honest); the internal motion is a deterministic function of the time.
double SpaceRenderer::cometKnot(int k, double t) {
    double ph = k * 0.2 + (k * 0.618034 - std::floor(k * 0.618034)) * 0.3 + t / 40.0;
    return ph - std::floor(ph);
}

void SpaceRenderer::drawCometTail(Framebuffer& fb, const SpaceContext& c, int bi, int bank, double nucleusRpx) {
    const StarSystem& sys = *c.sys;
    const Body& b = sys.bodies[bi];
    Vec3 posW = sys.bodyPos(bi, c.t);
    double dStar = length(posW - sys.star.pos);
    double activity = cometActivityAt(sys, dStar);
    if (activity < 0.02) return;
    double refKm = cometRefKm(sys);   // the tail's length
    double dark = c.skyMode ? c.skyDark : 1.0;
    if (dark < 0.05) return;
    Vec3 away = normalize(posW - sys.star.pos);
    Vec3 back = -sys.bodyVel(bi, c.t);
    back = back - away * dot(back, away);   // the part of "where it came from" that is not along the tail
    double bl = length(back);
    back = bl > 1e-9 ? back / bl : Vec3(0, 1, 0);
    double lenKm = activity * refKm * 0.5;
    double sqa = std::sqrt(activity);
    auto disc = [&](const Vec3& p, double rKm, double rMinPx, double add) {
        Vec3 v = c.cam * (p - c.shipPos);
        if (v.z <= 1) return;
        double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
        if (sx < -50 || sy < -50 || sx > FBW + 50 || sy > FBH + 50) return;
        double rpx = std::max(proj.f * rKm / length(v), rMinPx);
        if (rpx > 70 * FB_SCALE) rpx = 70 * FB_SCALE;
        int a = (int)(add * dark + 0.5);
        if (a < 1) return;
        fb.glowDisc(sx, sy, rpx, rpx * 0.4, a, bank);
    };
    // the coma, breathing a little
    double flicker = 1.0 + 0.10 * std::sin(c.t * 1.7) * std::sin(c.t * 0.61 + 1.3) + 0.05 * std::sin(c.t * 3.1);
    disc(posW, 6e4 * sqa, 2.0 * FB_SCALE, 30 * activity * flicker);
    // the ion tail: straight away from the star; five knots of brightness stream outward, accelerating
    const int N = 36;
    for (int k = 1; k <= N; k++) {
        double f = (double)k / N;
        double knots = 0;
        for (int j = 0; j < 5; j++) { double d = f - cometKnot(j, c.t); knots += std::exp(-d * d / (2 * 0.06 * 0.06)); }
        Vec3 p = posW + away * (lenKm * std::pow(f, 1.3));
        disc(p, (6e4 + 9e5 * f) * sqa, 1.0 * FB_SCALE, 18 * (1 - f) * (1 - f) * (0.82 + 0.36 * knots) * activity);
    }
    // the dust tail: heavier grains lag along the orbit, so it is shorter, wider, dimmer and curved
    const int N2 = 24;
    for (int k = 1; k <= N2; k++) {
        double g = (double)k / N2;
        Vec3 p = posW + away * (lenKm * 0.55 * g) + back * (lenKm * 0.28 * g * g);
        disc(p, (1.0e5 + 1.1e6 * g) * sqa, 1.0 * FB_SCALE, 9 * (1 - g) * (1 - g) * activity);
    }
    // close up: two to four jets vent from the sunlit hemisphere and bend back into the tail, knots
    // streaming out every 8 s; they turn with the nucleus
    if (nucleusRpx >= 3 * FB_SCALE) {
        Mat3 frame = sys.bodyFrame(bi, c.t);
        Mat3 inv = frame.transposed();
        Vec3 awayB = frame * away;
        int nj = 2 + (int)(b.seed % 3);
        for (int j = 0; j < nj; j++) {
            uint64_t hj = mix64(b.seed ^ (0x1E75ULL + (uint64_t)j * 0x9E37ULL));
            double lat = ((hj & 0xffff) / 65535.0 - 0.5) * 1.6, lon = ((hj >> 16) & 0xffff) / 65535.0 * TAU;
            Vec3 dB = StarSystem::bodyFromLatLon(lat, lon);
            if (dot(dB, awayB) > -0.15) dB = normalize(dB - awayB * (dot(dB, awayB) + 0.35));   // vents face the star
            Vec3 d = inv * dB;
            double strength = 0.6 + 0.4 * (((hj >> 32) & 0xff) / 255.0);
            for (int k = 0; k < 14; k++) {
                double s = (k + 0.5) / 14;
                double kn = 0;
                for (int q = 0; q < 3; q++) { double ph = q / 3.0 + j * 0.37 + c.t / 8.0; ph -= std::floor(ph); double dd = s - ph; kn += std::exp(-dd * dd / (2 * 0.06 * 0.06)); }
                Vec3 p = posW + d * (b.radiusKm * (1.02 + 3.0 * s)) + away * (b.radiusKm * 2.2 * s * s);
                disc(p, b.radiusKm * (0.10 + 0.35 * s), 1.0 * FB_SCALE, 12 * (1 - s) * (0.6 + 0.8 * kn) * strength * activity);
            }
        }
    }
}
