// E-08: the globes: a world's banks, a sphere's and a ring's screen box, the telescope's plate (W-06: built in blocks, read
// by the globe), `drawGlobe` (the disc, its rings, its clouds and fronts, its night, the sunless globe, the second sun), and
// the lights of a world in the dark
#include "space_internal.h"

// D-02: a globe with no sun lit by the band alone, in the band's own exposure: its albedo times the irradiance over pi (0.105 facing the
// core from the drifters near home, 0.012-0.034 facing away) taken back to the band's tone (the 0.7 power) and drawn at three times the
// band's 24 shades: a mid albedo's side toward the core 9, the far side 3 to 4, a dark rock's 2 to 4 (a black disc where the stars stop),
// under the band behind it (the plane 8-10, the bulge 21)
static constexpr double DARK_BAND_SHADES = 3 * BAND_SHADES;

// O0-02 (B-305): each of the two worlds with a disc owns six banks, so neither borrows the other's colours
int SpaceRenderer::globeBank(int bodyBank, int type, int material) {
    if (bodyBank != 2 && bodyBank != 3) return bodyBank;
    bool second = bodyBank == 3;
    switch (matFamilyOn(type, material)) {
        case FAM_WATER: return second ? 7 : 6;
        case FAM_FOREST: return second ? 16 : 11;
        case FAM_GRASS: return second ? 19 : 15;
        case FAM_SAND: return second ? 17 : 13;
        case FAM_SNOW: return second ? 18 : 14;
        default: return bodyBank;
    }
}

// B-312: the screen box of a sphere of radius R at the view-space centre c: the exact extents of its projection (the
// tangent planes through the camera's axes; on the plane z = 1 the extents are (a cz +- R sqrt(a^2 + cz^2 - R^2)) /
// (cz^2 - R^2) for a = cx and a = cy), or the whole frame when the sphere reaches the camera plane
static void sphereBox(const Vec3& c, double R, const Proj& pj, int& x0, int& x1, int& y0, int& y1) {
    x0 = 0; x1 = FBW - 1; y0 = 0; y1 = FBH - 1;
    double R2 = R * R;
    if (c.z <= 0 || c.z * c.z <= R2 * 1.0001) return;
    double d = c.z * c.z - R2;
    auto ext = [&](double a, double& lo, double& hi) {
        double s = std::sqrt(std::max(0.0, a * a + c.z * c.z - R2));
        lo = (a * c.z - R * s) / d; hi = (a * c.z + R * s) / d;
    };
    double xl, xh, yl, yh;
    ext(c.x, xl, xh); ext(c.y, yl, yh);
    x0 = std::max(0, (int)std::floor(pj.cx + pj.f * xl) - 1); x1 = std::min(FBW - 1, (int)std::ceil(pj.cx + pj.f * xh) + 1);
    y0 = std::max(0, (int)std::floor(pj.cy - pj.f * yh) - 1); y1 = std::min(FBH - 1, (int)std::ceil(pj.cy - pj.f * yl) + 1);
}

// B-312: the screen box of a ring's outer circle (radius ringR1 round c in the plane normal to axisV), from 48 points
// of it; any point at or behind the camera plane makes it the whole frame. The projection of a circle is an ellipse,
// convex, so the chords hug it within a fraction of a percent; a margin covers that
static void ringBox(const Vec3& c, const Vec3& axisV, double ringR1, const Proj& pj, int& x0, int& x1, int& y0, int& y1) {
    x0 = 0; x1 = FBW - 1; y0 = 0; y1 = FBH - 1;
    Vec3 u = normalize(cross(axisV, std::fabs(axisV.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0)));
    Vec3 v = cross(axisV, u);
    double xl = 1e18, xh = -1e18, yl = 1e18, yh = -1e18;
    for (int k = 0; k < 48; k++) {
        double a = k * TAU / 48;
        Vec3 p = c + (u * std::cos(a) + v * std::sin(a)) * ringR1;
        if (p.z <= ringR1 * 1e-4) return;
        double sx = pj.cx + pj.f * p.x / p.z, sy = pj.cy - pj.f * p.y / p.z;
        xl = std::min(xl, sx); xh = std::max(xh, sx); yl = std::min(yl, sy); yh = std::max(yh, sy);
    }
    double mx = (xh - xl) * 0.01 + 2, my = (yh - yl) * 0.01 + 2;
    x0 = std::max(0, (int)std::floor(xl - mx)); x1 = std::min(FBW - 1, (int)std::ceil(xh + mx));
    y0 = std::max(0, (int)std::floor(yl - my)); y1 = std::min(FBH - 1, (int)std::ceil(yh + my));
}

// ---------------------------------------------------------------------------
// W-06: the telescope's plate (see space_view.h)
// ---------------------------------------------------------------------------

constexpr int DetailPlate::BLOCKS[4];

double DetailPlate::progress() const {
    if (!valid()) return 0;
    if (done()) return 1;
    static const double cum[5] = {0, 1.0 / 64, 4.0 / 64, 16.0 / 64, 1.0};
    int L = BLOCKS[pass], bw = (W + L - 1) / L, bh = (H + L - 1) / L;
    return cum[pass] + (cum[pass + 1] - cum[pass]) * (double)next / std::max(1, bw * bh);
}

Vec3 SpaceRenderer::plateCellUnit(const DetailPlate& p, int i, int j) {
    double a = (i - p.W * 0.5 + 0.5) * p.texel, b = (j - p.H * 0.5 + 0.5) * p.texel, r2 = a * a + b * b;
    if (r2 >= 0.9999) return Vec3(0, 0, 0);
    return normalize(p.u0 * std::sqrt(1 - r2) + p.e1 * a + p.e2 * b);
}

namespace {
// a body-frame point turned about the spin axis (the jets' and the clouds' drift are turns of the lookup, the plate being fixed)
inline Vec3 rotZcs(const Vec3& v, double cs, double sn) { return Vec3(v.x * cs - v.y * sn, v.x * sn + v.y * cs, v.z); }
inline Vec3 rotZ(const Vec3& v, double ang) { return rotZcs(v, std::cos(ang), std::sin(ang)); }
// the plate's cell under a body-frame point: the cell coordinates (a cell's centre at integer fx, fy), the nearest cell,
// filled and on the sphere, else no hit
struct PlateHit { const DetailPlate* p = nullptr; double fx = 0, fy = 0; int in = 0, jn = 0; bool ok = false; };
inline PlateHit plateFind(const DetailPlate& p, const Vec3& bf) {
    PlateHit h;
    if (dot(bf, p.u0) <= 0.02) return h;
    double fx = dot(bf, p.e1) / p.texel + p.W * 0.5 - 0.5, fy = dot(bf, p.e2) / p.texel + p.H * 0.5 - 0.5;
    if (!(fx >= 0 && fy >= 0 && fx < p.W - 1 && fy < p.H - 1)) return h;
    h.fx = fx; h.fy = fy; h.in = (int)(fx + 0.5); h.jn = (int)(fy + 0.5);
    size_t o = (size_t)h.jn * p.W + h.in;
    if (!p.level[o] || p.material[o] == 255) return h;
    h.p = &p; h.ok = true;
    return h;
}
// the four samples round the point at the nearest cell's block step (a coarse pass interpolates between the blocks'
// own samples, so the picture is smooth at every pass), all filled and on the sphere, else false
inline bool plateQuad(const PlateHit& h, size_t o[4], double& tx, double& ty) {
    const DetailPlate& p = *h.p;
    int L = p.level[(size_t)h.jn * p.W + h.in];
    double u = h.fx / L, v = h.fy / L;
    int m = (int)std::floor(u), n = (int)std::floor(v);
    tx = u - m; ty = v - n;
    int i0 = m * L, i1 = i0 + L, j0 = n * L, j1 = j0 + L;
    if (i0 < 0 || j0 < 0 || i1 >= p.W || j1 >= p.H) return false;
    o[0] = (size_t)j0 * p.W + i0; o[1] = o[0] + L; o[2] = (size_t)j1 * p.W + i0; o[3] = o[2] + L;
    for (int k = 0; k < 4; k++) if (!p.level[o[k]] || p.material[o[k]] == 255) return false;
    return true;
}
inline double plateLerp4(const std::vector<uint8_t>& v, const size_t o[4], double tx, double ty) {
    double a = v[o[0]] + (v[o[1]] - v[o[0]]) * tx, b = v[o[2]] + (v[o[3]] - v[o[2]]) * tx;
    return (a + (b - a) * ty) / 255.0;
}
inline double plateAlbedo(const PlateHit& h) { size_t o[4]; double tx, ty; if (plateQuad(h, o, tx, ty)) return plateLerp4(h.p->albedo, o, tx, ty); return h.p->albedo[(size_t)h.jn * h.p->W + h.in] / 255.0; }
inline double plateCloud(const PlateHit& h) { size_t o[4]; double tx, ty; if (plateQuad(h, o, tx, ty)) return plateLerp4(h.p->cloud, o, tx, ty); return h.p->cloud[(size_t)h.jn * h.p->W + h.in] / 255.0; }
inline double plateVeg(const PlateHit& h) { return h.p->veg[(size_t)h.jn * h.p->W + h.in] / 255.0; }
inline int plateMaterial(const PlateHit& h) { return h.p->material[(size_t)h.jn * h.p->W + h.in]; }
// the slopes (rise over run) along e1 and e2 over the nearest cell's block step
inline void plateSlopes(const PlateHit& h, double& ga, double& gb) {
    const DetailPlate& p = *h.p;
    size_t on = (size_t)h.jn * p.W + h.in;
    int L = p.level[on];
    double h0 = p.height[on];
    auto hAt = [&](int i, int j) { size_t o = (size_t)j * p.W + i; return p.level[o] && p.material[o] != 255 ? (double)p.height[o] : h0; };
    int iL = std::max(0, h.in - L), iR = std::min(p.W - 1, h.in + L), jD = std::max(0, h.jn - L), jU = std::min(p.H - 1, h.jn + L);
    ga = iR > iL ? (hAt(iR, h.jn) - hAt(iL, h.jn)) / ((iR - iL) * p.texelM) : 0.0;
    gb = jU > jD ? (hAt(h.in, jU) - hAt(h.in, jD)) / ((jU - jD) * p.texelM) : 0.0;
}
}

// The clouds' drift in longitude at t, within a turn (F-10: wrapped once a frame, so that the globe's every cloud read, `lon + drift`,
// takes wrap2pi's one-turn path; unwrapped it grew with the clock, tens of turns and more in play, and each read was an fmod)
static double cloudDriftOf(const Body& b, double t) { return wrap2pi(t * TAU / (b.rotPeriod * 3.7) + 1.0); }

void SpaceRenderer::plateBegin(DetailPlate& p, int bi, uint64_t seed, int bucket, const Vec3& u0, const Vec3& e1, const Vec3& e2, double texel, double R, double cloudDrift, double halfA, double halfB) {
    plateBegun++;
    p = DetailPlate();
    p.body = bi; p.seed = seed; p.seasonBucket = bucket; p.u0 = u0; p.e1 = e1; p.e2 = e2;
    int W = (int)std::ceil(2 * halfA / texel) + 2, H = (int)std::ceil(2 * halfB / texel) + 2;
    const double CAP = 900000;   // cells: 2x's frame with its margin and an oblique view's stretch (3x and 4x read it at 2x's step); a wider field (the limb at a deep power) takes a coarser step
    if ((double)W * H > CAP) { double sc = std::sqrt(CAP / ((double)W * H)); texel /= sc; W = (int)std::ceil(2 * halfA / texel) + 2; H = (int)std::ceil(2 * halfB / texel) + 2; }
    W = std::max(8, (W + 7) / 8 * 8); H = std::max(8, (H + 7) / 8 * 8);
    p.W = W; p.H = H; p.texel = texel; p.texelM = texel * R * 1000.0;
    size_t n = (size_t)W * H;
    p.height.assign(n, 0.f); p.material.assign(n, 0); p.albedo.assign(n, 0); p.cloud.assign(n, 0); p.veg.assign(n, 0); p.level.assign(n, 0); p.exact.assign(n, 0);
    p.cloudDrift0 = cloudDrift;
    p.radiusM = 0.5 * std::sqrt((double)W * W + (double)H * H) * p.texelM;
}

// the build: the block passes in order, the blocks of a pass in rows, `budget` samples at most (0: all of it)
void SpaceRenderer::plateFill(DetailPlate& p, const BodyGen& g, int budget) {
    long long left = budget > 0 ? budget : (1LL << 40);   // B-424: long long (Windows' long is 32 bits: 1L << 40 left the photo's plate unread)
    const double Rm = g.R * 1000.0;
    if (!p.planRead && worldHadCivilisation(g)) {   // T-01: the plan's cores and ways in reach of the plate (it is built at the approach; until then not)
        if (std::shared_ptr<const WorldPlan> P = worldPlanIfReady(g)) {
            p.planRead = true;
            const double reach = p.radiusM + 3000;
            std::vector<const PlanPlace*> ps; planPlacesNear(*P, p.u0, reach, ps);
            for (const PlanPlace* q : ps) {
                const Culture cu = cultureOf(g, q->people);
                const RGB pc = cu.palette.colour[0];
                DetailPlate::PlanCore k; k.u = q->unit; k.r = q->radiusM; k.alb = clampd(0.25 + 0.5 * (0.3 * pc.r + 0.55 * pc.g + 0.15 * pc.b), 0.2, 0.7); k.standing = 0.75 - 0.45 * cu.decay;
                k.cosReach = std::cos(std::min(PI, (k.r + p.texelM) / Rm));
                p.cores.push_back(k);
            }
            if (p.texelM < 300) {   // a way shows where a texel is no more than ten times its width
                std::vector<PlanEdgeRef> es; planEdgesNear(*P, p.u0, reach, es);
                for (const PlanEdgeRef& r : es) {
                    if (r.e->kind == NK_SEA) continue;
                    const std::vector<Vec3>& way = planEdgeWay(g, *P, r);
                    const Culture cu = cultureOf(g, r.people);
                    static const double KD[NK_COUNT] = {0.2, 0.3, 0.5, 0.45, -0.15, -0.2, 0};   // darker (a railway's ballast), paler (a highway's slabs)
                    for (size_t k = 0; k + 1 < way.size(); k++) {
                        if (std::acos(clampd(dot(way[k], p.u0), -1, 1)) * Rm > reach + PLAN_LEG_KM * 1000) continue;
                        DetailPlate::PlanWay w; w.a = way[k]; w.b = way[k + 1]; w.half = netHalfWidth(r.e->kind, cu, r.e->trunk) * (r.e->kind == NK_MAGLEV ? 0.5 : 1.0); w.k = KD[clampi(r.e->kind, 0, NK_COUNT - 1)];
                        p.ways.push_back(w);
                    }
                }
            }
        }
    }
    const int rockMat = familyRep(g.type, FAM_ROCK);
    while (!p.done() && left > 0) {
        int L = DetailPlate::BLOCKS[p.pass];
        int bw = (p.W + L - 1) / L, bh = (p.H + L - 1) / L, nBlocks = bw * bh;
        if (p.next >= nBlocks) { p.pass++; p.next = 0; continue; }
        int count = (int)std::min<long long>(nBlocks - p.next, left), first = p.next;   // B-424: not min<long> (2^40 is 0 in Windows' long)
        std::atomic<long> taken{0};
        bool noDrain = p.noDrainage;
        // F-06: the blocks dealt 64 at a time (a block costs a sample, a copy or nothing: the threads' shares even out); the same
        // blocks land in the same frames whoever takes them
        parallelFor(count, 32, [&](int k0, int k1) {
            DrainageOff off(noDrain);
            long mine = 0;
            for (int k = k0; k < k1; k++) {
                int blk = first + k, i = (blk % bw) * L, j = (blk / bw) * L;
                size_t o = (size_t)j * p.W + i;
                if (!p.exact[o]) {
                    double a = (i - p.W * 0.5 + 0.5) * p.texel, b = (j - p.H * 0.5 + 0.5) * p.texel, r2 = a * a + b * b;
                    if (r2 < 0.9999) {
                        Vec3 u = normalize(p.u0 * std::sqrt(1 - r2) + p.e1 * a + p.e2 * b);
                        SurfaceSample s = sampleSurface(g, u, p.texelM);
                        double lat, lon; StarSystem::latLonFromBody(u, lat, lon);
                        double alb = s.albedo;
                        if (kindHas(g.type, KT_GAS_GIANT) && std::fabs(lat) > 76 * DEG) alb *= 1 + 0.14 * std::sin(6 * lon) * smoothstep(76 * DEG, 86 * DEG, std::fabs(lat));   // M9-10: the polar vortex, as the map has it
                        int mat = s.material;
                        for (const DetailPlate::PlanCore& k : p.cores) {   // T-01: a core's built ground, its share of the cell, less what the years took
                            if (dot(u, k.u) < k.cosReach - 1e-9) continue;   // F-07: surely out of reach (the angle's margin is far over acos's rounding)
                            const double d = std::acos(clampd(dot(u, k.u), -1, 1)) * Rm;
                            if (d > k.r + p.texelM) continue;
                            const double f = clampd((k.r - d) / p.texelM + 0.5, 0, 1) * std::min(1.0, k.r / std::max(1.0, p.texelM)) * k.standing;
                            alb += (k.alb - alb) * f;
                            if (f > 0.4 && mat != MAT_WATER) mat = rockMat;
                        }
                        for (const DetailPlate::PlanWay& w : p.ways) {   // the ways' beds: their share of the cell, darker or paler by their kind
                            const Vec3 ab = w.b - w.a; const double L2 = dot(ab, ab);
                            const double tt = L2 > 1e-24 ? clampd(dot(u - w.a, ab) / L2, 0, 1) : 0;
                            const double d = length(u - (w.a + ab * tt)) * Rm;
                            if (d > w.half + p.texelM) continue;
                            const double cover = clampd((w.half + 0.5 * p.texelM - d) / p.texelM, 0, std::min(1.0, 2 * w.half / p.texelM));
                            alb *= 1 - w.k * cover;
                        }
                        p.height[o] = (float)s.height; p.material[o] = (uint8_t)mat;
                        p.albedo[o] = (uint8_t)clampi((int)(alb * 255 + 0.5), 0, 255);
                        p.veg[o] = (uint8_t)clampi((int)(s.veg * 255 + 0.5), 0, 255);
                        p.cloud[o] = (uint8_t)clampi((int)(sampleCloudPattern(g, lon + p.cloudDrift0, lat) * 255 + 0.5), 0, 255);
                        mine++;
                    } else p.material[o] = 255;   // off the sphere
                    p.exact[o] = 1;
                }
                p.level[o] = (uint8_t)L;
                int i1 = std::min(i + L, p.W), j1 = std::min(j + L, p.H);
                for (int jj = j; jj < j1; jj++)
                    for (int ii = i; ii < i1; ii++) {
                        size_t q = (size_t)jj * p.W + ii;
                        if (q == o || p.exact[q]) continue;
                        p.height[q] = p.height[o]; p.material[q] = p.material[o]; p.albedo[q] = p.albedo[o]; p.cloud[q] = p.cloud[o]; p.veg[q] = p.veg[o]; p.level[q] = (uint8_t)L;
                    }
            }
            taken += mine;
        }, 64);
        p.samples += taken; plateSamplesFrame += taken; p.next += count; left -= count;
    }
}

// every frame: the plate the eyepiece needs for `c.detailBody` at the aim point's footprint; the current plate kept while it
// fits (the body, the season, the step within a quarter, the field inside it: the whole near hemisphere when it holds it,
// within forty degrees of its centre), else the other slot checked for a fit (a step out after a step in), else a new
// plate begun in the other slot (the old one staying as the fallback); then the build within the budget
void SpaceRenderer::updatePlate(const SpaceContext& c) {
    plateSamplesFrame = 0;
    int bi = c.detailBody;
    if (!c.sys || !c.sys->valid || c.skyMode || bi < 0 || bi >= (int)c.sys->bodies.size()) return;
    const StarSystem& sys = *c.sys;
    const Body& b = sys.bodies[bi];
    if (b.type == PT_COMPANION) return;
    Vec3 posW = sys.bodyPos(bi, c.t), cv = c.cam * (posW - c.shipPos);
    double dist = length(cv), R = b.radiusKm;
    if (cv.z <= 0 || dist <= R * 1.0001) return;
    double angR = std::asin(clampd(R / dist, 0, 1)), rpx = proj.f * std::tan(angR) * (dist / cv.z);
    double texelPx = FB_SCALE >= 3 ? FB_SCALE * 0.5 : 1.0;   // at 3x and 4x the plate is 2x's: the mush melts the rest
    if (rpx < 8 * texelPx) return;   // a dot: the map is enough
    // the aim point: where the central ray meets the sphere, else the point of the sphere nearest the ray (the limb beside the reticle)
    double perp2 = cv.x * cv.x + cv.y * cv.y;
    Vec3 nV; double dHit;
    if (perp2 < R * R) { double tHit = cv.z - std::sqrt(R * R - perp2); nV = (Vec3(0, 0, tHit) - cv) / R; dHit = tHit; }
    else { nV = normalize(Vec3(0, 0, cv.z) - cv); dHit = length(cv + nV * R); }
    Mat3 frame = sys.bodyFrame(bi, c.t), camT = c.cam.transposed();
    Vec3 u0 = normalize(frame * (camT * nV));
    double texel = (dHit / proj.f) * texelPx / R;   // a pixel's footprint at the aim point, in radii
    if (texel * R * 1000.0 >= 0.75 * TAU * R * 1000.0 / PlanetMap::W) return;   // the map's texel is finer than the eyepiece's: the map serves (a disc under about 80 px at 1x)
    // the axes along the screen's (the field then lies along the grid); a degenerate one (the limb beside the reticle seen
    // edge-on) falls back on the spin axis
    Vec3 ex = frame * (camT * Vec3(1, 0, 0)); ex = ex - u0 * dot(ex, u0);
    if (length(ex) < 0.3) { Vec3 ax = std::fabs(u0.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0); ex = cross(ax, u0); }
    Vec3 e1 = normalize(ex), e2 = cross(u0, e1);
    // the field's points on the sphere (the corners, the edges' middles, the centre): where the pixel's ray meets it, else
    // the point of the sphere nearest the ray (the limb beside it), in the body's frame; the plate must hold them all, so an
    // oblique view (the ground stretched along one axis) and the field's own turn are sized for, and a limb point counts
    // only up to two and a half fields (looking past the limb must not ask for the hemisphere at a fine step)
    struct FieldPt { Vec3 n; bool hit; };
    FieldPt fp[9]; int nfp = 0;
    const double pxs[3] = {0.5, FBW - 0.5, FBW * 0.5}, pys[3] = {0.5, FBH - 0.5, FBH * 0.5};
    for (int iy = 0; iy < 3; iy++)
        for (int ix = 0; ix < 3; ix++) {
            Vec3 d = normalize(Vec3((pxs[ix] - proj.cx) / proj.f, -(pys[iy] - proj.cy) / proj.f, 1.0));
            double tc = dot(d, cv), perp2 = dot(cv, cv) - tc * tc;
            bool hit = tc > 0 && perp2 < R * R;
            Vec3 nV = hit ? (d * (tc - std::sqrt(R * R - perp2)) - cv) / R : normalize(d * std::max(tc, 0.0) - cv);
            fp[nfp++] = {normalize(frame * (camT * nV)), hit};
        }
    double needA = 0.5 * FBW / texelPx * texel, needB = 0.5 * FBH / texelPx * texel;   // the field's half extent in radii, face on
    auto extent = [&](const Vec3& ea, const Vec3& eb, double& extA, double& extB) {
        extA = 0; extB = 0;
        for (int k = 0; k < nfp; k++) {
            double a = std::fabs(dot(fp[k].n, ea)), b = std::fabs(dot(fp[k].n, eb));
            if (!fp[k].hit) { a = std::min(a, 2.5 * needA); b = std::min(b, 2.5 * needB); }
            extA = std::max(extA, a); extB = std::max(extB, b);
        }
    };
    int bucket = bi < 64 ? seasonBucket[bi] : 0;
    const BodyGen& g = genFor(b);
    double cloudDrift = cloudDriftOf(b, c.t);
    auto fits = [&](const DetailPlate& p) {
        if (!p.valid() || p.body != bi || p.seed != b.seed || p.seasonBucket != bucket || std::fabs(p.texel / texel - 1) >= 0.25) return false;
        if (dot(u0, p.u0) <= 0.77) return false;   // the aim within forty degrees of the plate's centre
        double extA, extB; extent(p.e1, p.e2, extA, extB);
        if (extA > 0.5 * p.W * p.texel || extB > 0.5 * p.H * p.texel) return false;   // the field left the plate
        for (int k = 0; k < nfp; k++) if (fp[k].hit && dot(fp[k].n, p.u0) <= 0.05) return false;
        if (p.noDrainage && p.drainageWanted && drainageReady(g, p.u0, p.radiusM)) return false;   // the rivers' tiles are in: again with them
        return true;
    };
    if (!fits(plates[plateCur])) {
        if (fits(plates[1 - plateCur])) plateCur = 1 - plateCur;
        else {
            plateCur = 1 - plateCur;
            DetailPlate& q = plates[plateCur];
            double extA, extB; extent(e1, e2, extA, extB);
            plateBegin(q, bi, b.seed, bucket, u0, e1, e2, texel, R, cloudDrift, std::min(1.0, extA * 1.25 + 4 * texel), std::min(1.0, extB * 1.25 + 4 * texel));
            // the rivers and the lakes: their tiles for the plate's ground when the step is under the drainage's own limit and
            // the ground is a few tiles (a hemisphere at 1500 m a step would be a hundred); built now for a photo or a harness
            // frame, in the background otherwise (the plate sampled without them meanwhile and begun again when they are in)
            q.drainageWanted = typeHasDrainage(b.type) && q.texelM < 2048 && q.radiusM <= 200000 && drainageEnabled();
            q.noDrainage = !q.drainageWanted;   // a wider plate samples without the drainage outright: a sample would build its tile (a second each) on the spot
            if (q.drainageWanted && !drainageReady(g, u0, q.radiusM)) {
                if (c.detailBudget <= 0) drainagePrefetch(g, u0, q.radiusM, false);
                else { drainagePrefetch(g, u0, q.radiusM, true); q.noDrainage = true; }
            }
        }
    }
    plateFill(plates[plateCur], g, c.detailBudget);
}

std::string SpaceRenderer::plateInfo() const {
    const DetailPlate& p = plates[plateCur];
    if (!p.valid()) return "no plate";
    char buf[320];
    int L = p.done() ? 1 : DetailPlate::BLOCKS[p.pass];
    snprintf(buf, sizeof buf, "plate: body %d, %d x %d cells of %.0f m (%.3g radii), %s, %ld samples, %.0f%%%s", p.body, p.W, p.H, p.texelM, p.texel,
             p.done() ? "done" : (std::string("blocks of ") + std::to_string(L) + " " + std::to_string(p.next) + "/" + std::to_string(((p.W + L - 1) / L) * ((p.H + L - 1) / L))).c_str(),
             p.samples, 100 * p.progress(), p.noDrainage && p.drainageWanted ? ", no rivers yet" : "");
    std::string s = buf;
    if (p.samples > 0) {   // the heights' range and the slopes over the cells' own step (rise over run), for the harness
        double lo = 1e18, hi = -1e18, sl = 0; long n = 0;
        for (int j = 1; j < p.H - 1; j++)
            for (int i = 1; i < p.W - 1; i++) {
                size_t o = (size_t)j * p.W + i;
                if (!p.exact[o] || p.material[o] == 255) continue;
                lo = std::min(lo, (double)p.height[o]); hi = std::max(hi, (double)p.height[o]);
                int L = p.level[o];
                if (i + L < p.W && p.exact[(size_t)j * p.W + i + L] && p.material[(size_t)j * p.W + i + L] != 255) { sl += std::fabs(p.height[(size_t)j * p.W + i + L] - p.height[o]) / (L * p.texelM); n++; }
            }
        if (n) { snprintf(buf, sizeof buf, ", heights %.0f..%.0f m, mean slope %.3f", lo, hi, sl / n); s += buf; }
    }
    const DetailPlate& q = plates[1 - plateCur];
    if (q.valid() && q.body == p.body && q.seed == p.seed) { snprintf(buf, sizeof buf, "; the previous %d x %d at %.0f m, %.0f%%", q.W, q.H, q.texelM, 100 * q.progress()); s += buf; }
    return s;
}

void SpaceRenderer::drawGlobe(Framebuffer& fb, const SpaceContext& c, int bi, int bank, BodyScreenInfo& info) {
    const StarSystem& sys = *c.sys;
    const Body& b = sys.bodies[bi];
    Vec3 posW = sys.bodyPos(bi, c.t);
    Vec3 relW = posW - c.shipPos;
    Vec3 cv = c.cam * relW;             // view-space centre
    double dist = length(cv);
    info.body = bi;
    info.distKm = dist;
    info.inFront = cv.z > 0;
    double R = b.radiusKm;
    // O0-03 (B-304): a globe is skipped only when the whole sphere lies behind the camera plane. It used to be skipped as
    // soon as its centre did, so a huge parent in a moon's sky vanished the moment the camera looked down while its
    // limb still filled the top of the frame. With the centre behind, the rays are tested over the whole frame.
    if (cv.z + R <= 0) { info.radiusPx = 0; return; }
    double cz = cv.z;
    double angR = std::asin(clampd(R / dist, 0, 1));
    double rpx;
    if (cz > 1e-3 * R) {
        info.sx = proj.cx + proj.f * cv.x / cz;
        info.sy = proj.cy - proj.f * cv.y / cz;
        rpx = proj.f * std::tan(angR) * (dist / cz);
    } else {
        info.sx = proj.cx; info.sy = proj.cy; rpx = 1e9;   // the centre is behind or beside the camera: no projection, the box is the frame
    }
    info.radiusPx = rpx;
    if (b.type == PT_COMPANION) {
        // M5-01: the second sun; the surface draws its own (see SurfaceView::drawSky), space draws it here, depth-tested
        if (c.skyMode) { info.radiusPx = 0; return; }
        Star ks = sys.companionStar();
        double inten = clampd(0.6 + 0.4 * std::log10(1 + ks.luminosity), 0.7, 1.0);
        if (sys.star.cls == STAR_BLACK_HOLE) drawAccretionStream(fb, c, bi, 12);   // S-06: drawn out: its gas streams to the hole's disc, under its own disc
        drawSun(fb, ks, cv / dist, angR, c.t, inten, 12, false, 0, true, proj);   // B-308: by angle, whatever the size
        return;
    }
    StarSystem::Light light = sys.lightAt(posW, c.t);   // M5-01: bodies near the companion are lit by it
    Vec3 starDirW = light.dir;
    double lf = clampd(light.factor, 0, 1.15);
    if (b.type == PT_COMET) drawCometTail(fb, c, bi, c.skyMode ? 1 : bank, rpx);
    const KindDef& pt = KINDS[b.type];
    const bool gasGiant = kindHas(b.type, KT_GAS_GIANT), glows = kindHas(b.type, KT_GLOWS), deck = hasOpaqueDeck(b.type);   // P-00: once a globe, not a pixel
    // P-07: a made giant's class: the hot giants' night glow (a silicate giant's brighter toward the evening terminator, where its day's heat flows
    // over on the jet, dimmed and streaked toward the morning by the iron that condenses and rains out: Ehrenreich et al. 2020), the lightning of
    // a water cloud's only, a hazy giant's soft wide limb
    const int giantCls = KINDS[b.type].giantClass;
    const bool nightGlow = giantCls == GC_ALKALI || giantCls == GC_SILICATE, lightning = gasGiant && (giantCls == GC_NONE || giantCls == GC_WATER);
    const double limbHaze = giantCls == GC_DWARF ? 0.5 : (giantCls == GC_PUFF ? 0.8 : 0.0);
    // B-317: in a sky the depth is written in metres, the terrain's unit (it was the kilometre: a moon 20,000 km away read
    // as 20 km, so hills beyond that failed the depth test against it and the moon stood in the ground); in space the
    // kilometre stays, since the whole scene is in kilometres
    const double depthUnit = c.skyMode ? 1000.0 : 1.0;
    if (rpx < 1.2 * FB_SCALE && cz > 0) {
        // far body: a point like a bright star. In a sky (M5-09) it is a wandering star: a scale+1 square
        // that survives the mush, bright with phase, albedo and closeness, hidden where the sky is brighter
        Vec3 toShip = normalize(c.shipPos - posW);
        double phase = 0.5 + 0.5 * dot(toShip, starDirW);
        double v = c.skyMode ? clampd((30 + 26 * phase * pt.albedo * lf + 8 * (rpx / (1.2 * FB_SCALE))) * (0.35 + 0.65 * c.skyDark), 6, 60)
                             : clampd(18 + 30 * phase * pt.albedo * lf + 10 * (rpx / (1.2 * FB_SCALE)), 6, 55);
        if (sys.sunless() && !glows) {   // D-02: no sun: a dark world is no point of light (a molten one a faint ember; D-04: and a young interior's fields)
            const BodyGen& fg = genFor(b);
            if (!isLavaWorld(b.type) && !fg.hasTrait(TR_EMBERS)) return;
            v = c.skyMode ? 30 : 12;
        }
        if (glows) v = std::max(v, 40.0);
        if (giantCls == GC_SILICATE) v = std::max(v, 30.0);   // P-07: it glows
        if (b.type == PT_COMET) v = std::max(v, 36.0 * c.skyDark);
        int side = c.skyMode ? FB_SCALE + 1 : FB_SCALE;
        int x = (int)info.sx - (side - FB_SCALE) / 2, y = (int)info.sy - (side - FB_SCALE) / 2;
        for (int dy = 0; dy < side; dy++)
            for (int dx = 0; dx < side; dx++) {
                int px = x + dx, py = y + dy;
                if (px < 0 || py < 0 || px >= FBW || py >= FBH) continue;
                int o = py * FBW + px;
                if (shadeOf(fb.idx[o]) < v) { fb.idx[o] = pix(bank, v); fb.invz[o] = (float)(1.0 / (cz * depthUnit)); }
            }
        return;
    }
    const PlanetMap* mapPtr = mapIfReady(b);   // drawn flat until the background generation is done (M9-15)
    const bool haveMap = mapPtr != nullptr;
    static const PlanetMap emptyMap;
    const PlanetMap& map = haveMap ? *mapPtr : emptyMap;
    const BodyGen& bgen = genFor(b);
    Mat3 frame = sys.bodyFrame(bi, c.t);
    Mat3 camT = c.cam.transposed();
    Mat3 frameT = frame.transposed();
    // W-06: the telescope's plates for this body (the current, then the previous as the fallback), their axes in the world
    // and the weight of their relief shading (the map's is 5 over its 50 km, where the function's slopes are a few
    // thousandths; a plate's slopes at a few hundred metres are a tenth, so the weight runs from 2.5 there to 6 at 1.5 km)
    const DetailPlate* pl[2] = {nullptr, nullptr}; Vec3 plE1W[2], plE2W[2]; double plK[2] = {0, 0}; int npl = 0;
    if (bi == c.detailBody && !c.skyMode)
        for (int k = 0; k < 2; k++) {
            const DetailPlate& q = plates[k == 0 ? plateCur : 1 - plateCur];
            if (!q.valid() || q.body != bi || q.seed != b.seed || q.samples <= 0) continue;
            pl[npl] = &q; plE1W[npl] = frameT * q.e1; plE2W[npl] = frameT * q.e2; plK[npl] = 2.5 + 3.5 * smoothstep(200.0, 1500.0, q.texelM); npl++;
        }
    bool atmosphere = bodyHasAir(b);   // D-04: an air fallen as snow in the dark has no limb
    const bool ownCloudV = atmosphere && (deck || gasGiant);
    double bubble = atmosphere ? (giantCls == GC_PUFF ? 1.22 : (giantCls == GC_DWARF ? 1.07 : 1.045)) : 1.0;   // P-07: a hazy giant's air reaches far over its clouds
    const FrontMap* fmap = (atmosphere && !deck && worldHasFronts(b)) ? frontsFor(b, c.t) : nullptr;   // W-03: the fronts' bands
    const int frontCW = PlanetMap::W / FRONT_BLOCK, frontCH = PlanetMap::H / FRONT_BLOCK;
    const bool dustFronts = kindHas(b.type, KT_DUST);
    // B-312: the box is the exact screen extent of the sphere (its projection is a conic that reaches farther from the
    // projected centre the nearer the centre is to the edge of the view: a box of radius rpx round the centre cut the
    // limb off, and a huge parent vanished from a moon's sky as soon as its centre left the frame to the left or the
    // right), the whole frame when the sphere reaches the camera plane, and the projected outer circle of the rings
    int x0, x1, y0, y1;
    sphereBox(cv, R * bubble, proj, x0, x1, y0, y1);
    double ambient = atmosphere ? 0.10 : 0.05;
    // D-02: a system with no sun. No side faces a sun (every term of the sun's off: the shadows, the relief, the limb), the floor is the band's
    // light (`galaxyBandLight`: the hemisphere toward the bright band faintly lit, the core's way the brightest), a glowing drifter lights
    // its moons on the side facing it (W-04's earthshine, ungated) and the drifter's own glows stay: a brown dwarf's heat, lava, lightning
    // in a giant's night, the aurorae of a giant's or a brown dwarf's poles (bank 12: no second sun here)
    const bool dark = sys.sunless();
    if (dark && !c.skyMode && bandSHGen != bandGen) { galaxyBandLight(bandMap, bandSH); bandSHGen = bandGen; }
    if (dark) ambient = 0;   // the band's light is the floor (below)
    const bool aurorae = dark && !c.skyMode && b.drifter && kindHas(b.type, KT_GIANT) && auroraBank12;
    double auroraLat = 0, auroraW = 0, auroraV = 0;
    if (aurorae) {
        const uint64_t h = mix64(sys.star.seed ^ 0xA0DA2ULL);
        auroraLat = (66 + 10 * unitFromHash(h)) * DEG; auroraW = (1.6 + 1.4 * unitFromHash(mix64(h + 1))) * DEG;
        auroraV = 18 + 12 * clampd(drifterPower(sys.star) / 2, 0, 1);
    }
    // W-04: the earthshine: a moon's night side lit by its parent's day side (`bodyShineAt`, the light the ground's planetshine
    // reads at night), so a thin crescent shows the rest of its disc faintly and a moon's night seen from space is its parent's
    double shineP = 0; Vec3 shineDirW;
    if (b.parent >= 0 && b.parent < (int)sys.bodies.size()) {
        shineP = bodyShineAt(sys, b.parent, posW, c.t, lf);
        shineDirW = normalize(sys.bodyPos(b.parent, c.t) - posW);
    }
    double cloudDrift = cloudDriftOf(b, c.t);
    double plCs[2] = {1, 1}, plSn[2] = {0, 0};   // F-07: a plate's cloud turn where there is no jet (the angle as the pixel wrote it)
    for (int k = 0; k < npl; k++) { const double ang = 0.0 + cloudDrift - pl[k]->cloudDrift0; plCs[k] = std::cos(ang); plSn[k] = std::sin(ang); }
    double R2 = R * R;
    double c2 = dot(cv, cv);
    double rings = b.rings ? 1 : 0;
    Vec3 axisV = c.cam * b.spinAxis;
    const std::vector<float>* prof = b.rings ? &ringProfile(sys, bi) : nullptr;
    double ringR0 = b.ringInner * R, ringR1 = b.ringOuter * R;
    if (b.rings) {
        int rx0, rx1, ry0, ry1;
        ringBox(cv, axisV, ringR1, proj, rx0, rx1, ry0, ry1);
        x0 = std::min(x0, rx0); x1 = std::max(x1, rx1); y0 = std::min(y0, ry0); y1 = std::max(y1, ry1);
    }
    if (x0 > x1 || y0 > y1) return;
    // M1-08: moons and the parent cast shadows on this globe; rings shade it too. F-07: a body that can shade no lit point this
    // frame is left out (the loop would leave its shadow at 0): its shadow's cylinder passes wide of the globe (a point's distance
    // from the axis is at least the centre's less R), or it is behind the globe from the star and clear of it (a lit point's
    // `along` is under the centre's); the slack is far over the rounding of either side
    struct Occ { Vec3 pos; double r; };
    std::vector<Occ> occ;
    if (!c.skyMode)
        for (int j = 0; j < (int)sys.bodies.size(); j++) {
            if (j == bi) continue;
            if (sys.bodies[j].parent != bi && j != b.parent) continue;
            const Occ o{sys.bodyPos(j, c.t), sys.bodies[j].radiusKm};
            const Vec3 rc = o.pos - posW;
            const double alongC = dot(rc, starDirW), dc = length(rc), dpC = std::sqrt(std::max(0.0, length2(rc) - alongC * alongC));
            const double slack = 0.01 * o.r + 1e-6 * (dc + R);
            if (dpC - R > o.r + slack || (alongC <= 0 && dc - R > o.r + slack)) continue;
            occ.push_back(o);
        }
    double invf = 1.0 / proj.f;
    // N1-02: each material's bank on this globe (its family's, the accents' among them; a giant and a deck draw in the body's), once a globe
    int matBank[MAT_COUNT];
    for (int m = 0; m < MAT_COUNT; m++) matBank[m] = (kindHas(b.type, KT_GIANT) || deck) ? bank : globeBank(bank, b.type, m);
    // Q-02: the water's trait colours from the map (the first disc's in bank 31: a bloom's streaks, a lagoon, a pink lake), a milky sea's glow on
    // the night side in the water's own ramp (its pale upper stops)
    const bool tintMap = haveMap && !map.ltint.empty() && !c.skyMode;
    const bool tintBank31 = tintMap && bank == 2 && (bgen.hasTrait(TR_BLOOMS) || bgen.hasTrait(TR_REEFS) || bgen.hasTrait(TR_PINK_LAKES));
    const bool pinkLakes = bgen.hasTrait(TR_PINK_LAKES), milkyGlow = tintMap && bgen.hasTrait(TR_MILKY_SEAS);
    const bool waterLights = tintMap && bgen.hasTrait(TR_LIGHTS_IN_WATER);   // D-04: a dark ocean's shoals of light
    parallelFor(y1 - y0 + 1, 48, [&](int rb, int re) {
    for (int y = y0 + rb; y < y0 + re; y++) {
        for (int x = x0; x <= x1; x++) {
            Vec3 d((x + 0.5 - proj.cx) * invf, -(y + 0.5 - proj.cy) * invf, 1.0);
            double dl = length(d);
            d = d / dl;
            double tc = dot(d, cv);
            double disc = tc * tc - c2 + R2;
            int o = y * FBW + x;
            bool hitSphere = disc > 0 && tc > 0;
            double tHit = 0;
            double sphereVal = -1;
            if (hitSphere) {
                tHit = tc - std::sqrt(disc);
                Vec3 p = d * tHit;
                Vec3 nV = (p - cv) / R;
                Vec3 nW = camT * nV;
                double lit = dark ? -1.0 : dot(nW, starDirW);   // D-02: no sun
                if (lit > 0 && (!occ.empty() || rings)) {
                    Vec3 surfW = posW + nW * R;
                    double shadow = 0;
                    for (const Occ& o : occ) {
                        Vec3 rel = o.pos - surfW;
                        double along = dot(rel, starDirW);
                        if (along <= 0) continue;
                        double dp2 = length2(rel) - along * along;
                        double dp = std::sqrt(std::max(0.0, dp2));
                        if (dp < o.r) shadow = std::max(shadow, clampd((o.r - dp) / (o.r * 0.12), 0, 1));
                    }
                    if (rings) {
                        double denom = dot(starDirW, b.spinAxis);
                        if (std::fabs(denom) > 1e-6) {
                            double tr = dot(posW - surfW, b.spinAxis) / denom;
                            if (tr > 0) {
                                double rr = length(surfW + starDirW * tr - posW);
                                if (rr >= ringR0 && rr <= ringR1) shadow = std::max(shadow, 0.75 * (*prof)[clampi((int)((rr - ringR0) / (ringR1 - ringR0) * 255), 0, 255)]);
                            }
                        }
                    }
                    lit *= (1 - 0.92 * shadow);
                }
                double viewCos = -dot(nV, d);
                Vec3 bf = frame * nW;
                double lat = std::asin(clampd(bf.z, -1, 1));
                double lon = std::atan2(bf.y, bf.x);
                double jet = gasGiant ? giantJetTurn(b, bgen.bandCount, lat, c.t) : 0.0;   // M9-10 zonal jets (X-02: galaxy/probe.h, the descent reads the same)
                lon += jet;
                // W-06: the plate's cell under this point (the point turned by the jet, the plate being body-fixed): the
                // current plate first, the previous where the current has no cell yet, the map where neither has
                Vec3 bfG = jet != 0.0 ? rotZ(bf, jet) : bf;
                PlateHit ph; int phk = -1;
                for (int k = 0; k < npl && !ph.ok; k++) { ph = plateFind(*pl[k], bfG); if (ph.ok) phk = k; }
                double alb = ph.ok ? plateAlbedo(ph) : (haveMap ? map.albedoAt(lon, lat) : pt.albedo);
                int mat = ph.ok ? plateMaterial(ph) : (haveMap ? map.materialAt(lon, lat) : MAT_ROCK);
                int tintPb = -1; double glowW = 0;   // Q-02
                if ((tintBank31 || milkyGlow || waterLights) && mat == MAT_WATER) {
                    const int ti = map.texelIndex(lon, lat);
                    if (tintBank31 && std::max(map.lifeTintAt(ti), pinkLakes ? map.waterTintAt(ti) : 0.0) > 0.5) tintPb = 31;
                    if (milkyGlow || waterLights) glowW = map.waterTintAt(ti);
                }
                if ((ph.ok || haveMap) && gasGiant) {   // N5-03 (M9-10): storms grow and fade over about a day, band by band
                    double st = ph.ok ? plateVeg(ph) : map.veg[map.texelIndex(lon, lat)] / 255.0;
                    if (st > 0.01) {
                        double gain = 0.55 + 0.45 * std::sin(c.t / 86400.0 * 0.9 + lat * 4.0 + (double)(b.seed & 255) * 0.02);
                        alb = clampd(alb - 0.35 * st + 0.35 * st * gain, 0.22, 1.0);
                    }
                }
                if (!gasGiant && !c.skyMode && !dark) {
                    // M9-15 relief shading: slopes facing the star brighten, slopes facing away darken
                    // B-407 (2026-10-06): a surface rising toward the star tilts its normal away from it (a ramp rising east
                    // faces west), so the slope's term is taken off `lit`; M9-15 added it, and ridges were lit from the wrong side
                    if (ph.ok) {   // W-06: from the plate's heights along its axes, the rim's foreshortening taken out, held within +-0.7
                        double ga, gb; plateSlopes(ph, ga, gb);
                        double cc = clampd(dot(bfG, ph.p->u0), 0.05, 1.0);
                        lit = clampd(lit - clampd(plK[phk] * cc * (ga * dot(starDirW, plE1W[phk]) + gb * dot(starDirW, plE2W[phk])), -0.7, 0.7), -1, 1);
                    } else if (haveMap) {
                        Vec3 east = normalize(cross(b.spinAxis, nW)), north = cross(nW, east);
                        double sE = dot(starDirW, east), sN = dot(starDirW, north);
                        double dl = 0.7 * TAU / PlanetMap::W;
                        double hE = map.heightAt(lon + dl, lat) - map.heightAt(lon - dl, lat);
                        double hN = map.heightAt(lon, std::min(PI * 0.5, lat + dl)) - map.heightAt(lon, std::max(-PI * 0.5, lat - dl));
                        double run = 2 * dl * R * 1000.0;
                        lit = clampd(lit - 5.0 * (hE * sE + hN * sN) / run, -1, 1);
                    }
                }
                double shade = ambient + (1 - ambient) * smoothstep(-0.08, 0.25, lit) * std::max(0.0, lit * 0.6 + 0.4 * std::max(0.0, lit));
                shade = ambient + (1 - ambient) * clampd(lit * 1.1 + 0.05, 0, 1) * lf;
                // W-04: the parent's light past the terminator (it fades in over the first tenth, where the sun's own fades out)
                double earth = shineP > 0.002 && lit < 0 ? (1 - ambient) * 0.6 * shineP * std::max(0.0, dot(nW, shineDirW)) * smoothstep(0.0, -0.1, lit) : 0.0;
                if (earth > 0 && !c.skyMode) shade += earth;
                if (glows) shade = std::max(shade, bgen.selfGlow * (0.3 + 0.7 * alb) * (dark ? 0.55 + 0.45 * std::sqrt(clampd(viewCos, 0, 1)) : 1.0));   // M5-02: it glows on its own, bands and all (D-02: darkening to its limb, its glow the whole of it)
                double v = 0;
                if (!ownCloudV) {   // F-07: an opaque deck's and a giant's value is their clouds' (below), the ground's never read
                    v = exposureStop(alb, shade, atmosphere);   // N1-05: the ground's exposure rule
                    if (mat == MAT_LAVA) v = std::max(v, 50.0 + 10.0 * alb);
                }
                bool cloudy = false;
                if (atmosphere) {
                    double cl = 0.0; bool gotCl = false;
                    for (int k = 0; k < npl && !gotCl; k++) {   // W-06: the plate's cloud, the point turned by the drift since the build
                        PlateHit pc = plateFind(*pl[k], jet == 0.0 ? rotZcs(bf, plCs[k], plSn[k]) : rotZ(bf, jet + cloudDrift - pl[k]->cloudDrift0));
                        if (pc.ok) { cl = plateCloud(pc); gotCl = true; }
                    }
                    if (!gotCl) cl = haveMap ? map.cloudAt(lon + cloudDrift, lat) : 0.0;
                    if (fmap) {   // W-03: a front's band over the pattern (body-fixed moving features: no drift); a dust front is a haze in the ground's own bank
                        double u = (lon + PI) * (1.0 / TAU); if (u < 0) u += 1; else if (u >= 1) u -= 1;
                        int bx = std::min(frontCW - 1, (int)(u * frontCW)), by = clampi((int)((0.5 - lat / PI) * frontCH), 0, frontCH - 1);
                        if (fmap->coarse[(size_t)by * frontCW + bx]) {
                            double fc = frontMapAt(fmap->cloud, PlanetMap::W, PlanetMap::H, lon, lat);
                            if (dustFronts) v = v + (63.0 * 0.8 * shade - v) * fc * 0.7;
                            else cl = std::max(cl, fc);
                        }
                    }
                    if (deck) { v = 63.0 * (0.6 + 0.4 * cl) * shade; }
                    else if (gasGiant) { v = 63.0 * std::pow(alb, 0.7) * shade * 1.05; }
                    else { v = v + (63.0 * 0.92 * shade - v) * cl; cloudy = cl > 0.6; }
                    if (lit + 0.2 > 0) {   // F-07: else the term is +0 (the night side)
                        const double edge = 1.0 - clampd(viewCos, 0, 1), limb = edge * edge * edge;   // F-10: the cube as products
                        v += 14.0 * limb * clampd(lit + 0.2, 0, 1) * lf;
                        if (limbHaze > 0) v += (40.0 * clampd(lit + 0.25, 0, 1) * lf - v) * limbHaze * edge * edge;   // P-07: the haze pale over the limb
                    }
                }
                if (lightning && lit < -0.05 && !c.skyMode) {   // M9-10 lightning on the night side
                    if (unitFromHash(hash3i(x >> 2, y >> 2, (int64_t)(c.t * 4), b.seed)) < 0.0025) v = 58;
                }
                if (glowW > 0.2 && lit < 0.05 && !cloudy && !waterLights) v = std::max(v, (46.0 + 10.0 * glowW) * smoothstep(0.2, 0.6, glowW) * smoothstep(0.05, -0.15, lit));   // Q-02: a milky sea's glow on the night side, pale
                if (waterLights && glowW > 0.06) v = std::max(v, 37.0 + 35.0 * std::min(glowW, 0.2));   // D-04: the shoals' faint sheen, toward the light's own stops (from 44)
                if (nightGlow && lit < 0.12) {   // P-07: the hot giant's night side glowing (the ramp's low end), coming up through the terminator
                    const double night = smoothstep(0.12, -0.3, lit);
                    double glowV = 12.0 * (0.85 + 0.3 * (alb - 0.5));
                    if (giantCls == GC_SILICATE) {   // the night's longitude past the evening terminator (east of the substellar point along the turn)
                        const Vec3 ax = b.spinAxis, sp = normalize(starDirW - ax * dot(starDirW, ax)), np = nW - ax * dot(nW, ax);
                        double phi = std::atan2(dot(ax, cross(sp, np)) * (b.rotPeriod < 0 ? -1.0 : 1.0), dot(sp, np));   // + east of the substellar point
                        const double past = phi > 0 ? (phi - PI / 2) / PI : (phi + 1.5 * PI) / PI;   // 0 at the evening terminator .. 1 at the morning
                        const double rain = smoothstep(0.3, 0.6, past) * (0.55 + 0.45 * std::sin(lat * 26.0 + 3.0 * std::sin(lon * 3.0 + lat * 5.0)));
                        glowV = (11.0 + 5.0 * (1 - clampd(past, 0, 1))) * (1 - 0.6 * rain);
                    }
                    v = std::max(v, glowV * night);
                }
                if (dark && !c.skyMode) v = std::max(v, DARK_BAND_SHADES * std::pow(alb * galaxyBandIrradiance(bandSH, nW), BAND_GAMMA));   // D-02: the band's light
                sphereVal = clampd(v, 0, 63);
                int auroraPix = -1;   // D-02: the oval round each pole, its curtains brightening and fading along it
                if (aurorae) {
                    const double off = (std::fabs(lat) - auroraLat) / auroraW;
                    if (off > -3 && off < 3) {
                        const double curtain = 0.55 + 0.45 * std::sin(lon * 5 + c.t * 0.05 + (double)(b.seed & 63)) * std::sin(lon * 3 - c.t * 0.031 + (lat > 0 ? 0.0 : 1.7));
                        const double av = auroraV * std::exp(-off * off) * curtain;
                        if (av > sphereVal + 1) auroraPix = (int)av;
                    }
                }
                double iz = 1.0 / (tHit * d.z * depthUnit);
                if (c.skyMode) {
                    // lit parts map onto the bright, neutral top of the sky ramp; the dark side only occludes
                    if (iz > fb.invz[o]) {
                        double lv = sphereVal / 63.0;
                        // everything that faces the star is drawn as a solid disc; the night side only occludes,
                        // except a substellar object, whose glow shows through the night (M5-02)
                        if (glows) fb.idx[o] = pix(13, 16 + 28 * clampd(lv, 0, 1));   // bank 13: its own dull red glow
                        else if (lit > -0.03) fb.idx[o] = pix(bank, 43 + 20 * clampd(lv * 1.15, 0.05, 1.0));
                        else if (earth > 0.0005) {   // W-04: the earthshine, just over the black of the sky bank and under the lit side's 43, where it beats the sky there
                            double ev = 30 + 0.45 * exposureStop(alb, earth, false);
                            if (ev > shadeOf(fb.idx[o])) fb.idx[o] = pix(bank, std::min(42.0, ev));
                        }
                        fb.invz[o] = (float)iz;
                    }
                } else if (iz > fb.invz[o]) {
                    // N1-02: the material's family bank (oceans since M1-06); thick cloud goes to the snow bank so it reads white
                    int pb = matBank[cloudy ? MAT_CLOUD : mat];
                    if (tintPb >= 0 && !cloudy) pb = tintPb;   // Q-02
                    fb.idx[o] = auroraPix >= 0 ? pix(12, auroraPix) : pix(pb, sphereVal); fb.invz[o] = (float)iz;
                }
            } else if (atmosphere && !c.skyMode) {
                // glass bubble: thin bright rim outside the disc
                double tcc = tc;
                double dperp2 = c2 - tcc * tcc;
                double dperp = std::sqrt(std::max(0.0, dperp2));
                double rim = (dperp - R) / (R * (bubble - 1.0));
                if (rim < 1 && tcc > 0) {
                    Vec3 p = d * tcc;
                    Vec3 nW = camT * ((p - cv) / dperp);
                    double lit = clampd(dot(nW, starDirW) + 0.15, 0, 1);
                    double add = 26 * (1 - rim) * lit * lf;
                    Pix& px = fb.idx[o];
                    if (add > shadeOf(px)) px = pix(bank, add);
                }
            }
            if (rings) {
                // ring plane through cv with normal axisV
                double denom = dot(d, axisV);
                if (std::fabs(denom) > 1e-6) {
                    double tr = dot(cv, axisV) / denom;
                    if (tr > 0) {
                        Vec3 p = d * tr;
                        double rr = length(p - cv);
                        if (rr >= ringR0 && rr <= ringR1) {
                            double u = (rr - ringR0) / (ringR1 - ringR0);
                            double dens = (*prof)[clampi((int)(u * 255), 0, 255)];
                            if (dens > 0.02) {
                                Vec3 pW = camT * p + c.shipPos;
                                // shadow of the planet on the ring: ray from pW toward the star
                                Vec3 toStar = starDirW;
                                Vec3 rel = pW - posW;
                                double tcs = -dot(rel, toStar);
                                double closest2 = length2(rel + toStar * tcs);
                                double shadow = (tcs > 0 && closest2 < R2) ? 0.12 : 1.0;
                                double litRing = std::fabs(dot(b.spinAxis, starDirW)) * 0.7 + 0.3;
                                double v = 63.0 * dens * litRing * shadow * lf * 0.9;
                                if (c.skyMode) v = 36.0 + 27.0 * std::pow(clampd(dens * 1.3, 0, 1), 0.4) * (0.5 + 0.5 * litRing) * shadow * lf;   // M5-04/O0-01: rings in a sky read like a moon (the sky bank is black below 30)
                                double iz = 1.0 / (tr * d.z * depthUnit);
                                if (iz > fb.invz[o] || (!hitSphere)) {
                                    if (iz > fb.invz[o]) {
                                        // translucent: blend with what is behind
                                        double cur = shadeOf(fb.idx[o]);
                                        double a = clampd(dens * (c.skyMode ? 2.2 : 1.6), c.skyMode ? 0.4 : 0.25, 1.0);
                                        double nv = v * a + cur * (1 - a);
                                        fb.idx[o] = pix(bank, clampd(nv, 0, 63));
                                        fb.invz[o] = (float)iz;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    });
    if (!c.skyMode && (bgen.hasTrait(TR_LIGHTS_LEFT_ON) || (dark && bgen.hasTrait(TR_EMBERS) && !isLavaWorld(b.type)))) drawDarkLights(fb, c, bi, bank, posW, frameT, R, depthUnit, starDirW, dark);   // D-04
}

// D-04: the lights of a world in the dark, over its disc: a young interior's ember fields (TR_EMBERS: a spot of the lava ramp's glow each, as wide
// as the field, brighter in its middle, dimmer toward the limb) and a dead people's cities still lit (TR_LIGHTS_LEFT_ON: a point at each of its
// places, a capital or a city brighter than a town; on a world with a sun only on its night side); only on the side the eye sees and where
// nothing stands in front of the disc
void SpaceRenderer::drawDarkLights(Framebuffer& fb, const SpaceContext& c, int bi, int bank, const Vec3& posW, const Mat3& frameT, double R, double depthUnit, const Vec3& starDirW, bool dark) {
    const Body& b = c.sys->bodies[bi];
    const BodyGen& g = genFor(b);
    struct Light { Vec3 unit; double radiusKm, value; };
    std::vector<Light> lights;
    if (dark && g.hasTrait(TR_EMBERS) && !isLavaWorld(b.type)) {
        auto it = emberFields.find(b.seed);
        if (it == emberFields.end()) { std::vector<EmberField> f; emberFieldsOf(g, f); it = emberFields.emplace(b.seed, std::move(f)).first; }
        for (const EmberField& e : it->second) lights.push_back({e.unit, e.radiusKm * 0.8, 51.0 + 7.0 * e.strength});   // the lava ramp's glow (its stops from 46: a dull red to orange)
    }
    if (g.hasTrait(TR_LIGHTS_LEFT_ON)) {
        BodyGen tmp;
        if (std::shared_ptr<const WorldPlan> P = worldPlanIfReady(thenWorld(g, tmp)))
            for (int k = 0; k < 2; k++)
                for (const PlanPlace& pl : P->plans[k].places) lights.push_back({pl.unit, 0, pl.sclass == SC_CAPITAL ? 60.0 : (pl.sclass == SC_CITY ? 55.0 : 48.0)});
    }
    for (const Light& L : lights) {
        const Vec3 nW = frameT * L.unit, pW = posW + nW * R, toEye = c.shipPos - pW;
        const double eyeD = length(toEye), facing = dot(nW, toEye) / std::max(1e-9, eyeD);
        if (facing <= 0.02) continue;   // the far side, or edge-on
        if (!dark && dot(nW, starDirW) > -0.05) continue;   // a lit city's lights are lost in its day
        const Vec3 v = c.cam * (pW - c.shipPos);
        if (v.z <= 1e-6) continue;
        const double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
        const double rPx = proj.f * L.radiusKm / v.z * std::sqrt(facing);   // foreshortened toward the limb
        const double val = L.value - 4.0 * (1.0 - facing), iz = 1.0 / (v.z * depthUnit);   // a little dimmer toward the limb
        auto put = [&](int x, int y, double sv) {
            if (x < 0 || y < 0 || x >= FBW || y >= FBH) return;
            const int o = y * FBW + x;
            if (fb.invz[o] > iz * 1.02) return;   // a body in front of it
            if (sv > shadeOf(fb.idx[o])) fb.idx[o] = pix(bank, sv);
        };
        if (rPx < 1.5 * FB_SCALE) {   // a point: a square two pixels wider than the scale, so the frame's two passes of the mush keep its middle
            const int side = FB_SCALE + 2, x = (int)sx - (side - FB_SCALE) / 2, y = (int)sy - (side - FB_SCALE) / 2;
            for (int dy = 0; dy < side; dy++) for (int dx = 0; dx < side; dx++) put(x + dx, y + dy, val);
            continue;
        }
        const int x0 = (int)std::floor(sx - rPx), x1 = (int)std::ceil(sx + rPx), y0 = (int)std::floor(sy - rPx), y1 = (int)std::ceil(sy + rPx);
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) {
                const double dx = (x + 0.5 - sx) / rPx, dy = (y + 0.5 - sy) / rPx, r2 = dx * dx + dy * dy;
                if (r2 <= 1.0) put(x, y, val - 5.0 * r2);   // its middle the brightest (never under the glow's own stops)
            }
    }
}
