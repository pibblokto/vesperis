// J-09: the overgrowth on the ground (IDEAS-seeded-worlds.md section 7): its structures a 16 m cell (`forGrowths`, a pure function of the
// cell that the drawing, the colliders and the trees read), the discs they stand in the way with, and their drawing in the overgrowth's bank
// with its tile: lit by the sun and the lamp, a wet one glossy, a glowing one's spots lit at night, a ciliated one's cilia waving
#include "surface_internal.h"

namespace {
struct ShapeWeight { int shape; double w; };
struct GrowthKit { double perCell; int n; ShapeWeight s[10]; };
// each type's structures (the shared ones, then its own) and how many stand in a cell at their fullest
const GrowthKit KITS[OG_COUNT] = {
        {0, 0, {}},
        {4.5, 10, {{GS_MOUND, 3}, {GS_RIDGE, 2}, {GS_BLISTER, 2}, {GS_SAC, 1}, {GS_FOLD, 2}, {GS_VENT, 1}, {GS_ARCH, 0.8}, {GS_TEETH, 0.8},
                   {GS_MEMBRANE, 0.8}, {GS_STALK, 0.5}}},                                                                       // flesh
        {5.5, 6, {{GS_CAP, 4}, {GS_SHELF, 2}, {GS_MOUND, 1}, {GS_STALK, 1.5}, {GS_FOLD, 1}, {GS_MEMBRANE, 0.5}}},             // fungus
        {4.0, 5, {{GS_VEINS, 4}, {GS_STALK, 2}, {GS_BLISTER, 1}, {GS_MOUND, 1.5}, {GS_FOLD, 0.5}}},                           // slime mould
        {3.5, 5, {{GS_MOUND, 2}, {GS_FOLD, 2}, {GS_VASE, 1}, {GS_STALK, 0.6}, {GS_HAIR, 0.5}}},                               // lichen
        {6.0, 6, {{GS_SPIRE, 3}, {GS_FOLD, 2}, {GS_VASE, 1}, {GS_POLYP, 2}, {GS_MOUND, 1}, {GS_STALK, 1}}},                   // coral
        {4.5, 4, {{GS_VASE, 4}, {GS_SPIRE, 1.5}, {GS_MOUND, 1.5}, {GS_VENT, 1.5}}},                                           // sponge
        {6.0, 4, {{GS_HAIR, 5}, {GS_MOUND, 2}, {GS_STALK, 1}, {GS_RIDGE, 1}}},                                                // hair
        {4.0, 5, {{GS_BLADDER, 4}, {GS_SAC, 2}, {GS_BLISTER, 2}, {GS_MOUND, 1}, {GS_STALK, 1}}},                              // bladders
        {1.2, 4, {{GS_MOUND, 2}, {GS_RIDGE, 1}, {GS_FOLD, 1}, {GS_BLISTER, 1}}},                                              // carpets
        {0.6, 1, {{GS_BLISTER, 1}}},                                                                                          // films
};
// each shape's girth or reach (r) and height (h), in metres at a middling grain, and its length along its heading (0: round)
struct ShapeSize { double r0, r1, h0, h1, len0, len1; };
const ShapeSize SIZES[GS_COUNT] = {
        {0.6, 2.8, 0.4, 1.8, 0, 0},       // mound
        {0.5, 1.4, 0.4, 1.2, 3, 7},       // ridge
        {0.3, 1.1, 0.25, 1.0, 0, 0},      // blister
        {0.35, 0.9, 0.7, 2.2, 0, 0},      // sac
        {0.06, 0.18, 0.8, 3.2, 0, 0},     // stalk
        {0.6, 1.6, 0.3, 0.9, 2.5, 6},     // fold
        {0.6, 1.8, 0.5, 1.8, 0, 0},       // vent
        {0.12, 0.35, 1.4, 3.6, 2, 5},     // arch (a rib's girth, its height, its span)
        {0.12, 0.35, 0.6, 2.0, 2, 5},     // teeth (a tooth's foot, its height, the row)
        {0.08, 0.15, 1.2, 3.0, 2, 4.5},   // membrane (a post's girth, its height, the span)
        {0.5, 1.2, 0.3, 1.0, 0, 0},       // polyps (the cluster's reach)
        {0.3, 2.2, 0.4, 3.5, 0, 0},       // cap (the cap's radius)
        {0.3, 0.7, 0.8, 2.2, 0, 0},       // shelf (the stump's girth)
        {0.05, 0.14, 0.05, 0.14, 3, 9},   // veins (their girth)
        {0.18, 0.7, 1.6, 7.0, 0, 0},      // spire
        {0.35, 1.3, 0.8, 2.8, 0, 0},      // vase
        {0.4, 1.4, 0.6, 2.6, 0, 0},       // hair (the tuft's reach, a strand's length)
        {0.3, 1.3, 1.0, 6.0, 0, 0},       // bladder (its radius, how high it floats)
};
int pickShape(const GrowthKit& k, double u) {
    double sum = 0;
    for (int i = 0; i < k.n; i++) sum += k.s[i].w;
    u *= sum;
    for (int i = 0; i < k.n; i++) { if (u < k.s[i].w) return k.s[i].shape; u -= k.s[i].w; }
    return k.s[k.n - 1].shape;
}
double reachOf(const GrowthInst& S) { return growthReachOf(S); }
// a row's teeth (four to nine), an arch's ribs (three to five), each rib's place across the heading, its span and its height
int teethOf(const GrowthInst& S) { return 4 + (int)(hash01(mix64(S.hs + 120)) * 5.99); }
int ribsOf(const GrowthInst& S) { return 3 + (int)(hash01(mix64(S.hs + 110)) * 2.99); }
double ribOffset(const GrowthInst& S, int i, int nr) { return (i - 0.5 * (nr - 1)) * (0.3 + 1.2 * S.r); }
double ribSpan(const GrowthInst& S, int i, int nr) { return S.len * (1 - 0.15 * std::fabs(i - 0.5 * (nr - 1)) / std::max(1.0, 0.5 * nr)); }
double ribHeight(const GrowthInst& S, int i, int nr) { return S.h * (1 - 0.1 * std::fabs(i - 0.5 * (nr - 1))); }
}   // namespace

int growthFeet(const GrowthInst& S, GrowthFoot* out, int max) {
    const double hx = std::sin(S.heading), hz = std::cos(S.heading);
    int n = 0;
    auto put = [&](double along, double across, double r) { if (n < max) out[n++] = {S.x + hx * along + hz * across, S.z + hz * along - hx * across, r}; };
    switch (S.shape) {
        case GS_RIDGE: case GS_FOLD: case GS_MEMBRANE: {   // along it, close enough to leave no gap
            const double rr = S.shape == GS_MEMBRANE ? 0.35 : S.r * 0.85;
            const int k = std::min(max - 1, std::max(2, (int)std::ceil(S.len / (1.6 * rr))));
            for (int i = 0; i <= k; i++) put(S.len * (-0.5 + (double)i / k), 0, rr);
            break;
        }
        case GS_TEETH: { const int nt = teethOf(S); for (int i = 0; i < nt; i++) put(S.len * (-0.5 + (i + 0.5) / nt), 0, S.r + 0.15); break; }
        case GS_ARCH: {   // each rib's two feet
            const int nr = ribsOf(S);
            for (int i = 0; i < nr; i++) { put(-0.5 * ribSpan(S, i, nr), ribOffset(S, i, nr), S.r * 1.3 + 0.15); put(0.5 * ribSpan(S, i, nr), ribOffset(S, i, nr), S.r * 1.3 + 0.15); }
            break;
        }
        case GS_CAP: put(0, 0, std::max(0.15, 0.22 * S.r) + 0.1); break;   // the stalk (the cap overhangs)
        case GS_STALK: put(0, 0, 2.4 * S.r); break;
        case GS_SHELF: put(0, 0, S.r * 1.4); break;
        case GS_SPIRE: put(0, 0, S.r * 0.9); break;
        default: put(0, 0, S.r * 0.85); break;
    }
    return n;
}

// how far a structure reaches from its foot over the ground (a cap's overhang, a shelf's), for its place in the cell and the trees
double growthReachOf(const GrowthInst& S) {
    if (S.len > 0) return 0.5 * S.len + S.r;
    if (S.shape == GS_CAP) return S.r;
    if (S.shape == GS_SHELF) return 2.2 * S.r;
    if (S.shape == GS_STALK) return 2.6 * S.r;
    if (S.shape == GS_BLADDER) return 0.3;   // (its tether's foot)
    return S.r;
}
// a long one's footprint a capsule along its heading, a round one's a disc
bool growthHolds(const GrowthInst& S, double x, double z, double margin) {
    const double dx = x - S.x, dz = z - S.z;
    if (S.len > 0) {
        const double hx = std::sin(S.heading), hz = std::cos(S.heading);
        const double along = clampd(dx * hx + dz * hz, -0.5 * S.len, 0.5 * S.len);
        const double ex = dx - hx * along, ez = dz - hz * along, rr = (S.shape == GS_ARCH ? 0.5 * S.len : S.r) + margin;
        return ex * ex + ez * ez < rr * rr;
    }
    const double rr = growthReachOf(S) + margin;
    return dx * dx + dz * dz < rr * rr;
}

namespace {
// KI-476: the structures' lattice, fixed to the world: an equal-angle cube map's cells about 16 m across (each face cut n by n on its two
// angles), so a structure stands where it stands from any landing. A unit vector's face (its largest axis and that axis's sign: 0..5) and its
// two angles on that face (-pi/4..pi/4), and back
constexpr double GROWTH_CELL_M = 16;
int latticeN(double radiusM) { return std::max(1, (int)std::lround(radiusM * (PI / 2) / GROWTH_CELL_M)); }
void toFace(const Vec3& u, int& face, double& al, double& be) {
    const double c[3] = {u.x, u.y, u.z};
    int k = 0;
    if (std::fabs(c[1]) > std::fabs(c[k])) k = 1;
    if (std::fabs(c[2]) > std::fabs(c[k])) k = 2;
    const double m = std::fabs(c[k]);
    face = 2 * k + (c[k] < 0 ? 1 : 0);
    al = std::atan(c[(k + 1) % 3] / m); be = std::atan(c[(k + 2) % 3] / m);
}
Vec3 fromFace(int face, double al, double be) {
    const int k = face / 2;
    double c[3];
    c[k] = face & 1 ? -1.0 : 1.0; c[(k + 1) % 3] = std::tan(al); c[(k + 2) % 3] = std::tan(be);
    return normalize(Vec3(c[0], c[1], c[2]));
}
}   // namespace

// a world cell's structures, placed once a landing (its middle and its two edges read into the local metres, each structure inside the cell by
// its own reach, none overlapping another), each in the local cell its middle lies in; what the landing lays (its settlements, roads, streets
// and works, the ground drawn, the water) is tested where they are asked for
const SurfaceView::GrowthCell& SurfaceView::growthCellOf(int face, int i, int j, int N) {
    const uint64_t key = ((uint64_t)face << 42) | ((uint64_t)i << 21) | (uint64_t)j;
    const auto it = growthLattice.find(key);
    if (it != growthLattice.end()) return it->second;
    GrowthCell& gc = growthLattice[key];
    const double d = (PI / 2) / N, al = -PI / 4 + (i + 0.5) * d, be = -PI / 4 + (j + 0.5) * d;
    double x0, z0, xa, za, xb, zb;
    site.localAt(fromFace(face, al, be), x0, z0);
    site.localAt(fromFace(face, al + 0.5 * d, be), xa, za);
    site.localAt(fromFace(face, al, be + 0.5 * d), xb, zb);
    const double ax = 2 * (xa - x0), az = 2 * (za - z0), bx = 2 * (xb - x0), bz = 2 * (zb - z0);   // a cell across along each angle
    gc.x = x0; gc.z = z0;
    int line = -1;   // KI-473: the line whose cover lies at its middle (its kit, its sizes, its colour)
    madeGrowthAt(site.gen.made, fromFace(face, al, be), site.gen.seed, line);
    if (line < 0 || line >= MADE_LINES || !site.gen.made.spread[line].growth.on()) line = site.gen.made.growthLine;
    const MadeGrowth& G = site.gen.made.spread[line].growth;
    const GrowthKit& kit = KITS[G.type > 0 && G.type < OG_COUNT ? G.type : OG_FLESH];
    if (kit.n == 0) return gc;
    const uint64_t h = mix64(key * 0x9E3779B97F4A7C15ULL ^ site.gen.seed ^ 0x6E0E51ULL);
    const double la = std::hypot(ax, az), lb = std::hypot(bx, bz), area = std::fabs(ax * bz - az * bx);
    const double grown = 0.35 + 0.65 * smoothstep(1, 10, G.age);              // its structures grow with its steps
    const double scale = (0.7 + 0.6 * G.grain) * grown;                         // a coarse grain's bigger
    const double many = (1.3 - 0.6 * G.grain) * (0.4 + 0.6 * smoothstep(1, 8, G.age));
    const int n = std::min(10, (int)(kit.perCell * many * (0.5 + hash01(h)) * area / (GROWTH_CELL_M * GROWTH_CELL_M)));
    const double phi = std::atan2(bx, bz);   // (the cell's second angle's heading in the local metres: a structure's heading turns with it)
    double px[10], pz[10], pr[10]; int np = 0;
    for (int q = 0; q < n; q++) {
        const uint64_t hr = mix64(h + 41 * (q + 1));
        GrowthInst S;
        S.line = line;
        S.shape = pickShape(kit, hash01(hr));
        const ShapeSize& z = SIZES[S.shape];
        S.r = (z.r0 + (z.r1 - z.r0) * std::pow(hash01(mix64(hr + 2)), 1.4)) * scale;
        S.h = (z.h0 + (z.h1 - z.h0) * hash01(mix64(hr + 3))) * scale;
        S.len = z.len1 > 0 ? (z.len0 + (z.len1 - z.len0) * hash01(mix64(hr + 4))) * scale : 0;
        S.heading = phi + hash01(mix64(hr + 5)) * TAU;
        S.hs = mix64(hr + 6);
        const double ext = std::min(7.5, reachOf(S));
        const double mu = std::min(0.5, ext / std::max(1e-6, la)), mv = std::min(0.5, ext / std::max(1e-6, lb));
        const double fu = mu + hash01(mix64(hr + 7)) * (1 - 2 * mu) - 0.5, fv = mv + hash01(mix64(hr + 8)) * (1 - 2 * mv) - 0.5;
        S.x = x0 + ax * fu + bx * fv; S.z = z0 + az * fu + bz * fv;
        bool clash = false;   // none overlapping another
        for (int k = 0; k < np && !clash; k++) clash = (S.x - px[k]) * (S.x - px[k]) + (S.z - pz[k]) * (S.z - pz[k]) < (ext + pr[k]) * (ext + pr[k]);
        if (clash) continue;
        S.collides = S.h >= STEP_M && S.shape != GS_VEINS && S.shape != GS_HAIR && S.shape != GS_BLADDER && S.shape != GS_POLYP;
        px[np] = S.x; pz[np] = S.z; pr[np] = ext; np++;
        gc.s.push_back(S);
    }
    return gc;
}

// the structures whose middles lie in a local 16 m cell, as the world's cells round its middle placed them (kept a landing)
const std::vector<GrowthInst>& SurfaceView::growthsOwned(int cx, int cz) {
    if (growthLatticeKey != site.gen.made.key || growthOwned.size() > 16384 || growthLattice.size() > 16384) {
        growthLattice.clear(); growthOwned.clear(); growthLatticeKey = site.gen.made.key;
    }
    const uint64_t key = ((uint64_t)(uint32_t)cx << 32) | (uint32_t)cz;
    const auto it = growthOwned.find(key);
    if (it != growthOwned.end()) return it->second;
    std::vector<GrowthInst>& out = growthOwned[key];
    const double cs = 16;
    int face; double al, be;
    toFace(site.unitAt(cx * cs + 8, cz * cs + 8), face, al, be);
    const int N = latticeN(site.R);
    const double d = (PI / 2) / N;
    const int i0 = (int)std::floor((al + PI / 4) / d), j0 = (int)std::floor((be + PI / 4) / d);
    for (int dj = -1; dj <= 1; dj++)
        for (int di = -1; di <= 1; di++) {
            const int i = i0 + di, j = j0 + dj;
            if (i < 0 || j < 0 || i >= N || j >= N) continue;   // (a cube's edge: its strip goes without)
            const GrowthCell& gc = growthCellOf(face, i, j, N);
            if (std::fabs(gc.x - (cx * cs + 8)) > 2 * cs || std::fabs(gc.z - (cz * cs + 8)) > 2 * cs) continue;
            for (const GrowthInst& S : gc.s) if ((int)std::floor(S.x / cs) == cx && (int)std::floor(S.z / cs) == cz) out.push_back(S);
        }
    return out;
}

bool SurfaceView::growthCellHolds(int cx, int cz) {
    const MadeWorld& m = site.gen.made;
    if (!m.grown() || m.growthLine < 0 || m.growthLine >= MADE_LINES) return false;
    for (int k = 0; k < 4; k++) if (!madeGrowthMaterial(site.lod0.at(cx + (k & 1), cz + (k >> 1)).material)) return false;
    const TerrainVertex& tv = site.lod0.at(cx, cz);
    return !(tv.water > -1e8f && tv.h < tv.water + 0.3);
}

void SurfaceView::forGrowths(int cx, int cz, const std::function<void(const GrowthInst&)>& fn) {
    if (!growthCellHolds(cx, cz)) return;
    const double cs = 16;
    const RuinCell* near[SETTLEMENTS_NEAR]; int nNear = -1;
    for (GrowthInst S : growthsOwned(cx, cz)) {
        const double ext = std::min(7.5, reachOf(S));
        if (nNear < 0) nNear = settlementsNear(cx * cs + 8, cz * cs + 8, near);
        { bool in = false; for (int q = 0; q < nNear && !in; q++) in = inSettlementBuilding(*near[q], S.x, S.z, 1.0 + ext); if (in) continue; }
        if (!site.roads.empty() && site.roadCover(S.x, S.z) > 0.3) continue;
        if (!site.marks.empty() && site.markCover(S.x, S.z) > 0.3) continue;
        if (!site.edits.empty() && site.editCover(S.x, S.z) > 0.3) continue;
        S.gy = site.groundHeight(S.x, S.z);
        if (S.gy < site.waterAt(S.x, S.z) + 0.1) continue;
        fn(S);
    }
}

int SurfaceView::growthAt(double x, double z, double margin) {
    int shape = -1;
    const int cx = (int)std::floor(x / 16), cz = (int)std::floor(z / 16);
    for (int dz = -1; dz <= 1; dz++)
        for (int dx = -1; dx <= 1; dx++)
            forGrowths(cx + dx, cz + dz, [&](const GrowthInst& S) { if (shape < 0 && growthHolds(S, x, z, margin)) shape = S.shape; });
    return shape;
}

void SurfaceView::growthColliders(const GrowthInst& S, std::vector<Collider>& out) const {
    if (!S.collides) return;
    GrowthFoot f[GROWTH_FEET];
    const int n = growthFeet(S, f, GROWTH_FEET);
    const double top = S.shape == GS_ARCH ? S.gy + 0.6 * S.h : S.gy + S.h;   // (an arch's legs: its crown overhead)
    for (int i = 0; i < n; i++) out.push_back({f[i].x, f[i].z, f[i].r, 0, S.gy - 0.2, top});
    if (S.shape == GS_ARCH && S.h < 2.2) out.push_back({S.x, S.z, 0.4 + 1.5 * S.r * ribsOf(S), 0, S.gy + S.h - 2 * S.r, S.gy + S.h + S.r});   // a crown low enough to meet the head
}

bool SurfaceView::growthVisible(const GrowthInst& S, double& px) const {
    const double dist = std::max(0.5, std::sqrt((S.x - camPos.x) * (S.x - camPos.x) + (S.z - camPos.z) * (S.z - camPos.z)));
    px = std::max(S.r, std::max(S.h, 0.5 * S.len)) / dist * proj.f / FB_SCALE;   // (its size in 1x pixels)
    return px >= (S.shape == GS_HAIR || S.shape == GS_VEINS ? 4.0 : 2.0);
}

double SurfaceView::growthShade(const Vec3& p, Vec3 n, const Vec3& half, double ambient, double sunUp, double lf, double boost, int line) const {
    const bool own = line >= 0 && line < MADE_LINES && growthLineBank[line] >= 0;
    const double wet = own ? growthLineWet[line] : growthWet;
    const Vec3& sd = env.sun.dirLocal;
    if (dot(n, camPos - p) < 0) n = -n;
    const double amb = ambient * (0.55 + 0.45 * std::max(0.0, n.y));
    double light = amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf;
    if (nLamps) light += (1 - amb) * lampAt(p, &n);
    const double l = std::min(1.2, light), q = std::sqrt(l);
    double shade = 44 * q * std::sqrt(std::sqrt(q)) + boost;   // (44 l^0.625, the boulders' curve without a pow)
    if (wet > 0.3 && sunUp > 0.05) {   // a wet one's gloss
        double c = std::max(0.0, dot(n, half));
        c *= c; c *= c; c *= c;   // (c^8, then c^24)
        const double sp = c * c * c * sunUp * lf;
        shade += sp > 0.5 ? 12 * wet : (sp > 0.2 ? 5 * wet : 0);
    }
    return shade;
}

double SurfaceView::growthGlowShade(uint64_t face, double t, int line) const {
    const double glow = line >= 0 && line < MADE_LINES && growthLineBank[line] >= 0 ? growthLineGlow[line] : growthGlow;
    if (glow <= 0.02 || hash01(mix64(face + 900)) >= 0.22) return 0;
    return (46 + 10 * std::sin(t * 0.9 + hash01(mix64(face + 7)) * TAU)) * glow;
}

void SurfaceView::drawGrowth(ShapeSink& out, const GrowthDraw& D, double t, double ambient, double sunUp, double lf, const GrainTexture* tile) const {
    const GrowthInst& S = D.S;
    const MadeWorld& m = site.gen.made;
    const MadeGrowth& G = m.spread[S.line >= 0 && S.line < MADE_LINES ? S.line : m.growthLine].growth;   // (KI-473: its own line's)
    const double dist = std::max(0.5, std::sqrt((S.x - camPos.x) * (S.x - camPos.x) + (S.z - camPos.z) * (S.z - camPos.z))), px = D.px;
    auto groundAt = [&](double x, double z) { return S.gy + (x - S.x) * D.sx + (z - S.z) * D.sz; };   // (the plane under it)
    const Vec3& sd = env.sun.dirLocal;
    const double fogD = env.fogDistance;
    const double fogK = 1 - std::exp(-dist / fogD);
    const double wind = clampd(env.windKnots / 25.0, 0, 1.5), wdx = std::sin(env.windDir), wdz = std::cos(env.windDir);
    const double hx = std::sin(S.heading), hz = std::cos(S.heading);   // along it
    const int bank = growthBankOf(S.line) >= 0 ? growthBankOf(S.line) : 0;
    RasterParams rp; rp.bank = bank; rp.grain = &grain; rp.grainScale = 3.0 * FB_SCALE; rp.grain2 = tile; rp.grain2Scale = 3.0;
    rp.grainWeight = clampd(1.0 - (dist - 30.0) / 120.0, 0.2, 1.0);
    const bool hardFacets = G.hard >= 2;
    const int NA = hardFacets ? (px > 12 ? 6 : 4) : (px > 30 ? 9 : (px > 10 ? 6 : 4));
    const bool far = px < 8;   // (a far one plain: its small parts left out)
    const Vec3 toCam0 = normalize(Vec3(camPos.x - S.x, camPos.y - S.gy, camPos.z - S.z)), half = normalize(toCam0 + sd);
    int faceNo = 0;
    // a point's shade by its normal, lit from its side toward the eye (a sheet's both, a cup's inside, a cap's underside), a wet one glossy
    auto lit = [&](const Vec3& p, Vec3 n, double boost) { return growthShade(p, n, half, ambient, sunUp, lf, boost, S.line); };
    const double glowNow = S.line >= 0 && S.line < MADE_LINES && growthLineBank[S.line] >= 0 ? growthLineGlow[S.line] : growthGlow;
    // a triangle shaded at its corners (smooth), a glowing one's spots lit at night, slowly pulsing
    auto tri3 = [&](const Vec3& p0, double s0, const Vec3& p1, double s1, const Vec3& p2, double s2) {
        double sh[3] = {s0, s1, s2};
        if (glowNow > 0.02 && hash01(mix64(S.hs + 900 + (uint64_t)faceNo)) < 0.22)
            for (double& v : sh) v = std::max(v, (46 + 10 * std::sin(t * 0.9 + hash01(mix64(S.hs + 7)) * TAU)) * glowNow);
        faceNo++;
        RVert tri[3]; const Vec3 ws[3] = {p0, p1, p2};
        for (int q = 0; q < 3; q++) {
            const Vec3 v = toView(ws[q].x, ws[q].y, ws[q].z);
            const double s = std::min(62.0, sh[q]);
            tri[q].x = v.x; tri[q].y = v.y; tri[q].z = v.z; tri[q].shade = s + (63 - s) * fogK; tri[q].u = ws[q].x + ws[q].y * 0.7; tri[q].v = ws[q].z + ws[q].y * 0.4;
        }
        out.polygon(tri, 3, rp);
    };
    auto face = [&](const Vec3& p0, const Vec3& p1, const Vec3& p2, double boost) {   // flat: a shelf's, a sheet's, a vein's
        Vec3 n = cross(p1 - p0, p2 - p0);
        const double nl = length(n);
        if (nl < 1e-9) return;
        n = n * (1.0 / nl);
        const double s = lit((p0 + p1 + p2) * (1.0 / 3.0), n, boost);
        tri3(p0, s, p1, s, p2, s);
    };
    auto line3 = [&](const Vec3& a, const Vec3& b, double shade, int th) {
        RVert va, vb; const Vec3 A = toView(a.x, a.y, a.z), B = toView(b.x, b.y, b.z);
        va.x = A.x; va.y = A.y; va.z = A.z; vb.x = B.x; vb.y = B.y; vb.z = B.z;
        va.shade = vb.shade = shade + (63 - shade) * fogK;
        out.line(va, vb, bank, true, th);
    };
    // a lathe round an axis from (x, y0, z) up: rings of (radius fraction, height fraction), stretched along the heading by `sx`, leaning by
    // (lx, lz) a metre up, each vertex wobbled by the hash (`wob`); the top closed to a point where the last ring's radius is 0
    struct Ring { double rf, yf; };
    auto lathe = [&](double x, double y0, double z, double r, double h, const Ring* rings, int nr, double sx, double lx, double lz, double wob,
                     uint64_t hw, double boost) {
        Vec3 pts[8][10];
        double shd[8][10];
        nr = std::min(nr, 8);
        const double a0 = hash01(mix64(hw + 1)) * TAU;
        for (int k = 0; k < nr; k++)
            for (int a = 0; a < NA; a++) {
                const double ang = a0 + a * TAU / NA + (hardFacets ? 0.25 * k : 0);
                const double w = 1 + wob * (hash01(mix64(hw + 10 + k * 16 + a)) - 0.5);
                const double ca = std::cos(ang) * sx, sa = std::sin(ang);
                const double rx = (ca * hx + sa * hz), rz = (ca * hz - sa * hx);   // the ellipse turned to the heading
                const double yy = rings[k].yf * h;
                pts[k][a] = Vec3(x + rx * r * rings[k].rf * w + lx * yy, y0 + yy, z + rz * r * rings[k].rf * w + lz * yy);
            }
        // each point's normal across its neighbours (out from the axis; a closed end's along it), its shade there: smooth, or a hard one's facets
        for (int k = 0; k < nr; k++)
            for (int a = 0; a < NA; a++) {
                const Vec3 ta = pts[k][(a + 1) % NA] - pts[k][(a + NA - 1) % NA], tk = pts[std::min(k + 1, nr - 1)][a] - pts[std::max(k - 1, 0)][a];
                Vec3 n = cross(ta, tk);
                const double yy = rings[k].yf * h;
                const Vec3 axis(x + lx * yy, y0 + yy, z + lz * yy);
                if (length(n) < 1e-9 || rings[k].rf <= 0.02) n = Vec3(0, rings[k].yf >= 0.5 ? 1 : -1, 0);
                else if (dot(n, pts[k][a] - axis) < 0) n = -n;
                shd[k][a] = lit(pts[k][a], normalize(n), boost);
            }
        for (int k = 0; k + 1 < nr; k++)
            for (int a = 0; a < NA; a++) {
                const int b = (a + 1) % NA;
                if (hardFacets) {   // (a hard one's flat facets)
                    if (rings[k + 1].rf <= 0) face(pts[k][a], pts[k][b], pts[k + 1][0], boost);
                    else { face(pts[k][a], pts[k][b], pts[k + 1][b], boost); face(pts[k][a], pts[k + 1][b], pts[k + 1][a], boost); }
                } else if (rings[k + 1].rf <= 0) tri3(pts[k][a], shd[k][a], pts[k][b], shd[k][b], pts[k + 1][0], shd[k + 1][0]);
                else {
                    tri3(pts[k][a], shd[k][a], pts[k][b], shd[k][b], pts[k + 1][b], shd[k + 1][b]);
                    tri3(pts[k][a], shd[k][a], pts[k + 1][b], shd[k + 1][b], pts[k + 1][a], shd[k + 1][a]);
                }
            }
    };
    auto cilia = [&](double x, double y, double z, double r, int count, double len) {   // short hairs off a crown, waving
        if (!G.hair || dist > 25) return;
        for (int i = 0; i < count; i++) {
            const double ang = hash01(mix64(S.hs + 300 + i)) * TAU, sway = 0.3 * std::sin(t * 4.0 + i * 1.7);
            const Vec3 a(x + std::cos(ang) * r, y, z + std::sin(ang) * r);
            line3(a, a + Vec3(std::cos(ang) * len * 0.6 + sway * len * 0.3, len * 0.8, std::sin(ang) * len * 0.6), 30, 1);
        }
    };
    const double wobble = hardFacets ? 0.12 : 0.3;
    switch (S.shape) {
        case GS_MOUND: {
            static const Ring R[] = {{1, -0.08}, {0.94, 0.35}, {0.68, 0.75}, {0.3, 0.96}, {0, 1}};
            lathe(S.x, S.gy, S.z, S.r, S.h, R, 5, 1, 0, 0, wobble, S.hs, 0);
            cilia(S.x, S.gy + S.h * 0.8, S.z, S.r * 0.5, 10, 0.25);
            break;
        }
        case GS_BLISTER: {
            static const Ring R[] = {{0.82, -0.05}, {1, 0.3}, {0.86, 0.7}, {0.45, 0.95}, {0, 1}};
            lathe(S.x, S.gy, S.z, S.r, S.h, R, 5, 1, 0, 0, wobble * 0.4, S.hs, 3);
            break;
        }
        case GS_SAC: {
            static const Ring R[] = {{0.4, -0.05}, {0.72, 0.25}, {1, 0.6}, {0.86, 0.88}, {0, 1}};
            const double sw = 0.04 * std::sin(t * 0.7 + hash01(S.hs) * TAU);   // swaying a little
            lathe(S.x, S.gy, S.z, S.r * (1 + 0.04 * std::sin(t * 1.1 + hash01(S.hs) * 9)), S.h, R, 5, 1, sw * wdx, sw * wdz, wobble * 0.5, S.hs, 2);
            break;
        }
        case GS_RIDGE: case GS_FOLD: {   // a long hump along its heading; a fold's sinuous, in a few humps along it
            static const Ring R[] = {{1, -0.08}, {0.9, 0.45}, {0.55, 0.85}, {0, 1}};
            const int humps = S.shape == GS_FOLD ? 3 : 1;
            for (int k = 0; k < humps; k++) {
                const double f = humps == 1 ? 0 : -0.33 + 0.33 * k, side = S.shape == GS_FOLD ? 0.35 * S.r * std::sin(k * 2.1 + hash01(S.hs) * 6) : 0;
                const double x = S.x + hx * S.len * f + hz * side, z = S.z + hz * S.len * f - hx * side;
                lathe(x, S.gy, z, S.r, S.h * (humps == 1 ? 1 : 0.8 + 0.2 * hash01(mix64(S.hs + k))), R, 4, (S.len / humps) / (2 * S.r) + 0.5, 0, 0, wobble,
                      mix64(S.hs + 20 + k), 0);
            }
            break;
        }
        case GS_VENT: {   // a cone with a crater, its lip breathing
            const double br = 1 + 0.1 * std::sin(t * GROWTH_BREATH_RATE + hash01(S.hs) * TAU);
            const Ring R[] = {{1, -0.08}, {0.75, 0.6}, {0.48 * br, 1}, {0.3 * br, 0.86}, {0, 0.7}};
            lathe(S.x, S.gy, S.z, S.r, S.h, R, 5, 1, 0, 0, wobble, S.hs, 0);
            break;
        }
        case GS_STALK: {   // a thin stalk with a knob (a sporangium's, a bud's)
            const double sw = wind * 0.05 * std::sin(t * 1.3 + hash01(S.hs) * 9);
            static const Ring R[] = {{1, -0.02}, {0.7, 0.86}, {2.4, 0.9}, {2.6, 0.97}, {1.4, 1.04}, {0, 1.06}};
            lathe(S.x, S.gy, S.z, S.r, S.h, R, 6, 1, sw * wdx, sw * wdz, wobble * 0.5, S.hs, 2);
            cilia(S.x + sw * wdx * S.h, S.gy + S.h, S.z + sw * wdz * S.h, S.r * 2.4, 8, 0.15);
            break;
        }
        case GS_CAP: {   // a cluster of one to three: a stalk and a cap over it
            const int nc = far ? 1 : 1 + (int)(hash01(mix64(S.hs + 50)) * 2.99);
            for (int c = 0; c < nc; c++) {
                const uint64_t hc = mix64(S.hs + 60 + c);
                const double f = c == 0 ? 1 : 0.4 + 0.4 * hash01(hc), ang = hash01(mix64(hc + 1)) * TAU;
                const double cr = S.r * f, ch = S.h * f, sr = std::max(0.06, 0.18 * cr);
                const double x = S.x + (c == 0 ? 0 : std::cos(ang) * S.r * 0.8), z = S.z + (c == 0 ? 0 : std::sin(ang) * S.r * 0.8);
                const double gy = c == 0 ? S.gy : groundAt(x, z);
                const double k = cr / sr;
                const Ring R[] = {{1, -0.02}, {0.85, 0.8}, {0.8, 0.82}, {k, 0.8}, {k * 0.9, 0.9}, {k * 0.45, 0.99}, {0, 1}};
                lathe(x, gy, z, sr, ch, R, 7, 1, 0, 0, wobble * 0.4, hc, 0);
            }
            break;
        }
        case GS_SHELF: {   // a stump stacked with half-discs of shelves
            static const Ring R[] = {{1, -0.05}, {0.9, 0.98}, {0, 1.0}};
            lathe(S.x, S.gy, S.z, S.r, S.h, R, 3, 1, 0, 0, wobble, S.hs, -4);
            const int ns = far ? 2 : 3 + (int)(hash01(mix64(S.hs + 70)) * 3.99);
            for (int i = 0; i < ns; i++) {
                const uint64_t hsh = mix64(S.hs + 80 + i);
                const double y = S.gy + S.h * (0.2 + 0.7 * hash01(hsh)), dir = hash01(mix64(hsh + 1)) * TAU, w = S.r * (0.8 + 0.9 * hash01(mix64(hsh + 2)));
                const Vec3 c(S.x + std::cos(dir) * S.r * 0.85, y, S.z + std::sin(dir) * S.r * 0.85);
                Vec3 prev;
                for (int k = 0; k <= 5; k++) {   // a fan of the half-disc's top, drooping to its rim
                    const double a = dir - 1.2 + 2.4 * k / 5.0;
                    const Vec3 p(c.x + std::cos(a) * w, y - 0.12 * w, c.z + std::sin(a) * w);
                    if (k > 0) face(c, prev, p, 4);
                    prev = p;
                }
            }
            break;
        }
        case GS_SPIRE: {   // a tall spiked cone; a coral's with branches off it
            static const Ring R[] = {{1, -0.05}, {0.62, 0.4}, {0.3, 0.75}, {0, 1}};
            lathe(S.x, S.gy, S.z, S.r, S.h, R, 4, 1, 0, 0, wobble * 0.6, S.hs, 0);
            if (G.type == OG_CORAL && !far) {
                const int nb = 2 + (int)(hash01(mix64(S.hs + 90)) * 2.99);
                for (int i = 0; i < nb; i++) {
                    const uint64_t hb = mix64(S.hs + 91 + i);
                    const double f = 0.25 + 0.4 * hash01(hb), ang = hash01(mix64(hb + 1)) * TAU, lean = 0.4 + 0.3 * hash01(mix64(hb + 2));
                    const double x = S.x + std::cos(ang) * S.r * 0.5 * (1 - f), z = S.z + std::sin(ang) * S.r * 0.5 * (1 - f);
                    lathe(x, S.gy + S.h * f, z, S.r * 0.45 * (1 - f), S.h * 0.45 * (1 - 0.5 * f), R, 4, 1, std::cos(ang) * lean, std::sin(ang) * lean, wobble * 0.5, hb, 0);
                }
            }
            break;
        }
        case GS_VASE: {   // a cup: its outer wall flaring to the lip, its inside down to a floor
            static const Ring R[] = {{0.55, -0.05}, {0.72, 0.4}, {1, 0.86}, {1.08, 1}, {0.88, 0.97}, {0.66, 0.65}, {0, 0.55}};
            static const Ring RF[] = {{0.55, -0.05}, {1, 0.86}, {0, 1}};   // (far: a closed cup)
            if (far) lathe(S.x, S.gy, S.z, S.r, S.h, RF, 3, 1, 0, 0, wobble * 0.5, S.hs, 0);
            else lathe(S.x, S.gy, S.z, S.r, S.h, R, 7, 1, 0, 0, wobble * 0.5, S.hs, 0);
            cilia(S.x, S.gy + S.h, S.z, S.r * 1.05, 12, 0.2);
            break;
        }
        case GS_POLYP: {   // a cluster of small stalks, each with a crown of tentacles waving
            const int np = far ? 2 : 3 + (int)(hash01(mix64(S.hs + 100)) * 4.99);
            for (int i = 0; i < np; i++) {
                const uint64_t hp = mix64(S.hs + 101 + i);
                const double ang = hash01(hp) * TAU, rr = S.r * std::sqrt(hash01(mix64(hp + 1))), hh = S.h * (0.5 + 0.5 * hash01(mix64(hp + 2)));
                const double x = S.x + std::cos(ang) * rr, z = S.z + std::sin(ang) * rr, gy = groundAt(x, z);
                static const Ring R[] = {{1, -0.02}, {0.8, 0.9}, {1.6, 1.0}, {0, 1.02}};
                lathe(x, gy, z, 0.04 + 0.05 * S.r, hh, R, 4, 1, 0, 0, 0.2, hp, 2);
                if (dist < 30 && !far)
                    for (int k = 0; k < 6; k++) {
                        const double a = k * TAU / 6 + hash01(hp) * 3, sway = 0.4 * std::sin(t * 2.2 + k + i);
                        const Vec3 top(x, gy + hh, z);
                        line3(top, top + Vec3(std::cos(a) * 0.12 + sway * 0.05, 0.1, std::sin(a) * 0.12), 36, 1);
                    }
            }
            break;
        }
        case GS_ARCH: {   // a ribcage: three to five ribs arching over, each a tube from foot to foot (`growthFeet`)
            const int nr = ribsOf(S);
            const int SEG = far ? 4 : (dist < 40 ? 8 : 5), SIDES = far ? 3 : (dist < 40 ? 5 : 3);
            for (int ri = 0; ri < nr; ri++) {
                if (far && ri != nr / 2) continue;   // (far: the middle rib)
                const double off = ribOffset(S, ri, nr), span = ribSpan(S, ri, nr), hh = ribHeight(S, ri, nr);   // the ribs side by side across the heading
                Vec3 path[9];
                for (int s = 0; s <= SEG; s++) {
                    const double f = (double)s / SEG, a = PI * f;
                    const double x = S.x + hx * span * (f - 0.5) + hz * off, z = S.z + hz * span * (f - 0.5) - hx * off;
                    path[s] = Vec3(x, S.gy - 0.1 + std::sin(a) * hh, z);
                }
                for (int s = 0; s < SEG; s++) {
                    const double rr0 = S.r * (1.3 - 0.6 * std::sin(PI * s / SEG)), rr1 = S.r * (1.3 - 0.6 * std::sin(PI * (s + 1) / SEG));
                    const Vec3 d = normalize(path[s + 1] - path[s]);
                    const Vec3 side = normalize(cross(d, Vec3(0, 1, 0)) + Vec3(1e-6, 0, 0)), upv = normalize(cross(side, d));
                    for (int k = 0; k < SIDES; k++) {   // (smooth: a point's normal is its offset from the rib's line)
                        const double a0 = k * TAU / SIDES, a1 = (k + 1) * TAU / SIDES;
                        const Vec3 o0 = side * std::cos(a0) + upv * std::sin(a0), o1 = side * std::cos(a1) + upv * std::sin(a1);
                        const Vec3 q00 = path[s] + o0 * rr0, q01 = path[s] + o1 * rr0, q11 = path[s + 1] + o1 * rr1, q10 = path[s + 1] + o0 * rr1;
                        const double s00 = lit(q00, o0, 4), s01 = lit(q01, o1, 4), s11 = lit(q11, o1, 4), s10 = lit(q10, o0, 4);
                        tri3(q00, s00, q01, s01, q11, s11);
                        tri3(q00, s00, q11, s11, q10, s10);
                    }
                }
            }
            break;
        }
        case GS_TEETH: {   // a row of curved cones, leaning back along the row (`growthFeet`)
            const int nt = teethOf(S);
            static const Ring R[] = {{1, -0.05}, {0.6, 0.5}, {0, 1}};
            for (int i = 0; i < nt; i++) {
                if (far && (i & 1)) continue;
                const uint64_t ht = mix64(S.hs + 121 + i);
                const double f = -0.5 + (i + 0.5) / nt, hh = S.h * (0.6 + 0.4 * hash01(ht)) * (1 - 0.5 * std::fabs(f));
                const double x = S.x + hx * S.len * f, z = S.z + hz * S.len * f;
                lathe(x, groundAt(x, z), z, S.r, hh, R, 3, 1, hx * 0.25, hz * 0.25, 0.15, ht, 6);
            }
            break;
        }
        case GS_MEMBRANE: {   // two posts and a sheet between them sagging toward the ground
            static const Ring R[] = {{1, -0.05}, {0.7, 1}, {0, 1.04}};
            const double x0 = S.x - hx * S.len * 0.5, z0 = S.z - hz * S.len * 0.5, x1 = S.x + hx * S.len * 0.5, z1 = S.z + hz * S.len * 0.5;
            const double g0 = groundAt(x0, z0), g1 = groundAt(x1, z1);
            lathe(x0, g0, z0, S.r, S.h, R, 3, 1, 0, 0, 0.2, S.hs, 0);
            lathe(x1, g1, z1, S.r, S.h * (0.85 + 0.3 * hash01(S.hs)), R, 3, 1, 0, 0, 0.2, mix64(S.hs + 1), 0);
            const int NX = far ? 2 : (dist < 40 ? 6 : 3), NY = far ? 1 : (dist < 40 ? 3 : 2);
            const double breathe = 0.08 * std::sin(t * 0.6 + hash01(S.hs) * 6);
            auto sheet = [&](int i, int j) {
                const double f = (double)i / NX, v = (double)j / NY, sag = std::sin(PI * f);
                const double top = (g0 + S.h * 0.95) * (1 - f) + (g1 + S.h * 0.85) * f - sag * S.h * 0.25, bot = (g0 + g1) * 0.5 + 0.15 * sag;
                return Vec3(x0 + (x1 - x0) * f + hz * sag * (0.3 + breathe) * v, top + (bot - top) * v, z0 + (z1 - z0) * f - hx * sag * (0.3 + breathe) * v);
            };
            for (int i = 0; i < NX; i++)
                for (int j = 0; j < NY; j++) { face(sheet(i, j), sheet(i + 1, j), sheet(i + 1, j + 1), 7); face(sheet(i, j), sheet(i + 1, j + 1), sheet(i, j + 1), 7); }
            break;
        }
        case GS_VEINS: {   // creeping veins over the ground, branching (a slime mould's: a main vein wandering on, a few branches off it)
            auto vein = [&](Vec3 p, double dir, double len, double girth, uint64_t hv, int depth, auto&& self) -> void {
                const int SEG = std::max(2, (int)(len / 0.8));
                for (int s = 0; s < SEG; s++) {
                    dir += 0.6 * (hash01(mix64(hv + s)) - 0.5);
                    Vec3 q(p.x + std::sin(dir) * len / SEG, 0, p.z + std::cos(dir) * len / SEG);
                    q.y = groundAt(q.x, q.z) + girth * 0.5;
                    const Vec3 side(std::cos(dir) * girth, 0, -std::sin(dir) * girth), up(0, girth * 0.8, 0);
                    face(p - side, q - side, q + up, 6); face(p - side, q + up, p + up, 6);
                    face(p + up, q + up, q + side, 6); face(p + up, q + side, p + side, 6);
                    if (depth < 1 && !far && hash01(mix64(hv + 50 + s)) < 0.3) self(q, dir + (hash01(mix64(hv + 60 + s)) < 0.5 ? -1 : 1) * 0.9, len * 0.4, girth * 0.6, mix64(hv + 70 + s), depth + 1, self);
                    p = q;
                }
            };
            Vec3 p0(S.x - hx * S.len * 0.5, 0, S.z - hz * S.len * 0.5);
            p0.y = groundAt(p0.x, p0.z) + S.r * 0.5;
            vein(p0, S.heading, S.len, S.r, S.hs, 0, vein);
            break;
        }
        case GS_HAIR: {   // a tuft of long strands waving with the wind, dark at the root, pale at the tip
            const int ns = far ? 5 : (dist < 20 ? 40 : (dist < 50 ? 20 : 10));
            const double sh = 14 + 14 * sunUp * lf, th = dist < 8 ? 3 : (dist < 25 ? 2 : 1);
            for (int i = 0; i < ns; i++) {
                const uint64_t hh = mix64(S.hs + 200 + i);
                const double ang = hash01(hh) * TAU, rr = S.r * std::sqrt(hash01(mix64(hh + 1))), len = S.h * (0.6 + 0.4 * hash01(mix64(hh + 2)));
                const double x = S.x + std::cos(ang) * rr, z = S.z + std::sin(ang) * rr, gy = groundAt(x, z);
                const double sway = (0.3 + wind) * (0.25 + 0.15 * std::sin(t * 1.6 + i * 0.7 + hash01(S.hs) * 6));
                Vec3 a(x, gy, z);
                for (int k = 1; k <= 3; k++) {   // three segments bending downwind, more at the tip
                    const double f = k / 3.0;
                    const Vec3 b(x + (wdx * sway * f * f + std::cos(ang) * 0.15 * f) * len, gy + len * f * (1 - 0.25 * sway * f), z + (wdz * sway * f * f + std::sin(ang) * 0.15 * f) * len);
                    line3(a, b, sh + 22 * f, (int)th);
                    a = b;
                }
            }
            break;
        }
        case GS_BLADDER: {   // a sphere floating on a tether, rising and sinking slowly
            const double ph = hash01(S.hs) * TAU, rise = 0.55 + 0.45 * std::sin(t * 0.05 + ph);
            const double y = S.gy + S.r + S.h * rise + 0.15 * std::sin(t * 0.7 + ph), dx = wdx * wind * 0.4 * S.h * rise, dz = wdz * wind * 0.4 * S.h * rise;
            static const Ring R[] = {{0.05, -1}, {0.72, -0.7}, {1, 0}, {0.72, 0.7}, {0, 1}};
            lathe(S.x + dx, y, S.z + dz, S.r, S.r, R, 5, 1, 0, 0, 0.1, S.hs, 4);
            line3(Vec3(S.x, S.gy, S.z), Vec3(S.x + dx, y - S.r, S.z + dz), 22, 1);
            break;
        }
        default: break;
    }
}

// J-10: the overgrowth's sounds round the explorer on foot: the nearest vent within 24 m (louder nearer), what creaks within 20 m (the tall:
// ribs, teeth, spires, stalks, membranes' posts, caps, shelves and vases; a soft overgrowth's half as much) and the hair within 15 m, both in the
// wind; a membrane across a ruin's opening crossed since (px0, pz0) tears. Each eased in and out as the ice's song
void SurfaceView::updateGrowthSongs(double dt, double px0, double pz0) {
    double vent = 0, creak = 0, hair = 0;
    const MadeWorld& m = site.gen.made;
    if (m.grown() && m.growthLine >= 0 && m.growthLine < MADE_LINES && !inVehicle() && site.atmosphere) {
        const MadeGrowth& G = m.spread[m.growthLine].growth;
        const double px = player.x, pz = player.z, wind = clampd(env.windKnots / 20.0, 0, 1);
        double nearVent = 1e9, tall = 0, hairs = 0;
        const int c0x = (int)std::floor(px / 16), c0z = (int)std::floor(pz / 16);
        for (int dz = -2; dz <= 2; dz++)
            for (int dx = -2; dx <= 2; dx++)
                forGrowths(c0x + dx, c0z + dz, [&](const GrowthInst& S) {
                    const double d = std::hypot(S.x - px, S.z - pz);
                    if (S.shape == GS_VENT) nearVent = std::min(nearVent, d);
                    else if (S.shape == GS_HAIR) hairs += std::max(0.0, 1 - d / 15);
                    else if (S.shape == GS_ARCH || S.shape == GS_TEETH || S.shape == GS_SPIRE || S.shape == GS_STALK || S.shape == GS_MEMBRANE || S.shape == GS_CAP ||
                             S.shape == GS_SHELF || S.shape == GS_VASE)
                        tall += std::max(0.0, 1 - d / 20);
                });
        vent = nearVent < 24 ? (1 - nearVent / 24) * (1 - nearVent / 24) : 0;
        creak = clampd(tall / 3.0, 0, 1) * (G.hard >= 1 ? 1.0 : 0.5) * (0.3 + 0.7 * wind);
        hair = clampd(hairs / 2.0, 0, 1) * (0.25 + 0.75 * wind);
        auto tears = [&](const Ruin& r, const RuinCell& c) {
            if (c.climb.empty() || std::hypot(r.x - px, r.z - pz) > r.reach + 10) return;
            for (const Form& f : c.elems[0]) {
                if (f.growth != FG_MEMBRANE || f.broken > 0) continue;   // (KI-479: one torn tears once)
                const double mx = r.x + f.x, mz = r.z + f.z;
                if (std::fabs(mx - px) > 4 || std::fabs(mz - pz) > 4) continue;
                const double nx = std::sin(f.heading), nz = std::cos(f.heading), ax = std::cos(f.heading), az = -std::sin(f.heading);
                const double s0 = (px0 - mx) * nx + (pz0 - mz) * nz, s1 = (px - mx) * nx + (pz - mz) * nz, along = (px - mx) * ax + (pz - mz) * az;
                const double gy = site.groundHeight(mx, mz);
                if ((s0 < 0) != (s1 < 0) && std::fabs(along) < f.hx + 0.2 && player.y < gy + f.y1 && player.y + 1.6 > gy + f.y0) {
                    tearCue = 0.8;
                    tornMembranes.insert(membraneKey(c, (int)f.p[3]));   // KI-479: torn for the landing (the cell's own copy marked; a regrown one reads the set)
                    const_cast<Form&>(f).broken = 1;
                }
            }
        };
        if (std::hypot(px - px0, pz - pz0) > 1e-4) {
            forNearbyRuins(px, pz, 1, [&](const Ruin& r, const RuinCell& c, int) { tears(r, c); });
            for (const RuinCell& c : placedCells) tears(c.r, c);   // (tests)
        }
    }
    const double k = 1 - std::exp(-dt / 0.8);
    ventSong += (vent - ventSong) * k; creakSong += (creak - creakSong) * k; hairSong += (hair - hairSong) * k;
    if (ventSong < 0.002 && vent == 0) ventSong = 0;
    if (creakSong < 0.002 && creak == 0) creakSong = 0;
    if (hairSong < 0.002 && hair == 0) hairSong = 0;
}
