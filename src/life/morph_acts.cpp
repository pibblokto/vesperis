// Z-02: what a specimen does (life/morph_draw.h's acts, IDEAS 1.6), each closed-form in the clock from its start: the forms of the life cycle
// (spore, cyst, ghost, fossil, a colony, the slug and the stalk), division and budding, a colony's daughters let out, sporulation and lysis,
// waking, encystment, engulfing, the bridge, the crowds that turn to light and to the field or glow, the gathering in spiral waves, the
// fruiting, the colony's branching front, a protocell's thread
#include "life/morph_shape_internal.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

const char* const ACT_NAMES[AN_COUNT] = {"LIVE", "DIVIDE", "BUD", "RELEASE", "SPORULATE", "WAKE", "ENCYST", "LYSE", "ENGULF", "BRIDGE", "TO LIGHT",
                                         "TO FIELD", "GLOW", "GATHER", "FRUIT", "FRONT", "THREAD"};

bool actSuits(const Morph& m, int act) {
    const int p = m.plan;
    if (m.state == SS_FOSSIL) return act == AN_LIVE;
    switch (act) {
        case AN_LIVE: return true;
        case AN_DIVIDE: return (m.cycle & LC_DIVIDING) && m.division != DV_BUDDING && m.division != DV_NONE && p != MP_GATHERING && p != MP_ROLLER &&
                               !(protocellPlan(p) && m.division == DV_FRAGMENTS);
        case AN_BUD: return m.division == DV_BUDDING;
        case AN_RELEASE: return p == MP_ROLLER && m.variant == 0;
        case AN_SPORULATE: return (m.cycle & LC_SPORULATING) && smallCellPlan(p) && p != MP_FILAMENT;
        case AN_WAKE: return (m.cycle & LC_WAKING) != 0;
        case AN_ENCYST: return (m.cycle & LC_DORMANT) && (largeCellPlan(p) || p == MP_GATHERING);
        case AN_LYSE: return (m.cycle & LC_DYING) && !colonyPlan(p) && p != MP_FILAMENT;
        case AN_ENGULF: return (m.way == LW_PREDATOR || m.way == LW_GRAZER) && (p == MP_AMOEBA || p == MP_TEST || p == MP_CILIATE || p == MP_FLAGELLATE || p == MP_GATHERING);
        case AN_BRIDGE: return (m.cycle & LC_EXCHANGING) && !colonyPlan(p) && p != MP_FILAMENT;
        case AN_TO_LIGHT: return (m.taxis & TX_LIGHT) && m.motion != MO_NONE && !colonyPlan(p);
        case AN_TO_FIELD: return (m.taxis & TX_FIELD) != 0;
        case AN_GLOW: return m.glows;
        case AN_GATHER: return p == MP_GATHERING;
        case AN_FRUIT: return p == MP_GATHERING && (m.cycle & LC_FRUITING);
        case AN_FRONT: return (m.cycle & LC_COLONY) && (smallCellPlan(p) || p == MP_BRANCHING);
        case AN_THREAD: return protocellPlan(p) && m.division == DV_FRAGMENTS;
        default: return false;
    }
}

double actSeconds(const Morph& m, int act) {
    const double u = unitFromHash(mix64(m.seed ^ 0xAC75ULL * (uint64_t)(act + 1)));
    switch (act) {
        case AN_LIVE: return 4;
        case AN_DIVIDE: return 30 + 30 * u;
        case AN_BUD: return 40 + 30 * u;
        case AN_RELEASE: return 60;
        case AN_SPORULATE: return 50 + 20 * u;
        case AN_WAKE: return 20 + 40 * u;   // the plan's 20-60 s
        case AN_ENCYST: return 30;
        case AN_LYSE: return 12;
        case AN_ENGULF: return 25;
        case AN_BRIDGE: return 40;
        case AN_TO_LIGHT: return 20;
        case AN_TO_FIELD: return 12;
        case AN_GLOW: return 30;
        case AN_GATHER: case AN_FRUIT: case AN_FRONT: return 120;
        case AN_THREAD: return 40;
        default: return 10;
    }
}

namespace morph {

double frontRadiusUm(const Morph& m) { return clampd(m.widthUm * 500, 400, 1500); }   // a colony of millimetres, the larger its cells the larger

namespace {
inline double h01(uint64_t h) { return unitFromHash(mix64(h)); }   // re-mixed: a shifted hash would span [0, 2^-n) alone
inline uint64_t hk(uint64_t seed, int64_t a, int64_t b = 0) { return hash2i(a, b, seed); }
double ease(double x) { x = clampd(x, 0, 1); return x * x * (3 - 2 * x); }
double ramp(double p, double a, double b) { return clampd((p - a) / (b - a), 0, 1); }

// a spore (or a large cell's cyst wall) at the builder's place: its coat smooth, spined, ridged or netted, phase-bright; waking: swelling,
// darkening, its coat split open at the front
void sporeForm(Build& B, double wake, double scale) {
    const Morph& m = B.m;
    const bool small = smallCellPlan(m.plan) || protocellPlan(m.plan) || (colonyPlan(m.plan) && m.plan != MP_GATHERING);
    const bool round = m.plan != MP_ROD && m.plan != MP_CURVED && m.plan != MP_SPIRAL && m.plan != MP_CORKSCREW && m.plan != MP_FILAMENT && m.plan != MP_STALKED;
    const double cellR = m.widthUm * 0.5;
    const double R = (small ? cellR * std::min(round ? 0.65 : 1.4, m.sporeSize * (round ? 0.75 : 1.2)) : cellR * (0.35 + 0.3 * m.sporeSize)) * scale * (1 + 0.25 * ease(wake / 0.3));
    Look L = cellLook(m, R);
    L.refract = (float)(1.0 - 0.9 * ease(wake / 0.3));
    L.phase = (float)(0.25 + 0.75 * ease(wake / 0.3));   // phase-bright while it sleeps, dark as it wakes
    L.density = small ? 0.5f : 0.45f;
    L.wall = (float)(R * (small ? 0.22 : 0.16));
    L.wallDensity = 0.7f;
    L.halo = 0; L.lum = 0; L.red = 0; L.stain = 0.25f;
    L.mark = m.coat == CW_RIDGED ? MK_RIBS : (m.coat == CW_NETTED ? MK_NET : MK_SMOOTH);
    L.marks = m.coat == CW_RIDGED ? 10 + (int)(m.seed % 9) : 14 + (int)(m.seed % 10);
    L.layout = m.coat == CW_RIDGED ? 3 : 0;
    if (m.pigment == PG_BROWN || m.pigment == PG_BLACK || m.pigment == PG_RED || m.pigment == PG_ORANGE) L.bank = pigmentBank(m.pigment);
    if (wake > 0.3) { L.gapPhi = 0; L.gapW = (float)(0.9 * ramp(wake, 0.3, 0.5)); }
    Body& b = B.radial(0, 0, 0, R, small && !round ? 1.3 + 0.3 * h01(m.seed >> 9) : (small ? 1.08 : 1.0), L);
    fillOutline(b, 0, 2, 2, 2, small ? 0.01 : 0.05, m.seed ^ 0x5B0EULL, B.t, 0);
    B.finish(b);
    if (m.coat == CW_SPINED) {
        const Body copy = b;
        const int n = 14 + (int)(m.seed % 14);
        for (int k = 0; k < n; k++) {
            P2 e, nr; edgeOf(copy, k * TAU / n, e, nr);
            pushStrand(B.out, {{e.x, e.y}, {e.x + nr.x * R * 0.3, e.y + nr.y * R * 0.3}}, (float)(R * 0.07), (float)(R * 0.02), ST_SPINE, L.bank, 1, B.group);
        }
    }
    if (!small) {   // a cyst's contents: dense granules and a nucleus
        CellCtx C; C.form = CF_CYST;
        InFrame F = frameOf(B, b, R * 0.8, R * 0.8);
        for (int k = 0; k < 40; k++) {
            const uint64_t q = hk(m.seed, 90, k);
            const P2 p = F.point(h01(q) * 1.6 - 0.8, h01(q >> 9) * 1.6 - 0.8);
            B.dot(p.x, p.y, R * 0.06, R * 0.06, 0, DOT_GRANULE, SB_FIELD, 1);
        }
        if (m.nucleus > 0) B.dot(0, 0, R * 0.25, R * 0.25, 0, DOT_NUCLEUS, SB_FIELD, 1);
    }
}

// the cyst: the cell rounded in a double wall, its outer one wrinkled
void cystForm(Build& B, const Pose& P, double thick) {
    const Morph& m = B.m;
    if (smallCellPlan(m.plan) || protocellPlan(m.plan)) { sporeForm(B, 0, 1); return; }
    const double R = m.widthUm * 0.5 * 0.7;
    Look outer = cellLook(m, R);
    outer.density = 0.08f; outer.wall = (float)(R * 0.12 * thick); outer.wallDensity = 0.6f; outer.mark = MK_SMOOTH; outer.lum = 0; outer.stain = 0;
    Body& o = B.radial(0, 0, 0, R * 1.12, 1, outer);
    fillOutline(o, 0, 2, 2, 2, 0.06, m.seed ^ 0xC757ULL, B.t, 0);
    B.finish(o);
    CellCtx C; C.form = CF_CYST; C.round = 1; C.limbs = 0; C.scale = 0.7;
    sporeForm(B, 0, 1.0);
    (void)P;
}

// a crowd of the species round the specimen's place: each its own seed, pose and place
template <class Place>
void crowd(Build& B, const Pose& P, int n, Place place) {
    const Specimen& sp = B.sp;
    for (int k = 0; k < n; k++) {
        Specimen one = sp; one.seed = mix64(sp.seed + (uint64_t)k * 0x9E3779B97F4A7C15ULL);
        double x, y, a; CellCtx C;
        place(k, one, x, y, a, C);
        Pose pk = poseAt(B.m, one, B.t);
        Build b2(B.m, one, B.t, B.pixel, B.out);
        b2.place(x, y, a, P.mirror);
        b2.group = k;
        buildCell(b2, pk, C);
    }
}

// the colony's branching front on a plate (Ben-Jacob et al. 1994): tips that split as they spread, drawn to the radius it has reached
void branchingFront(Build& B, double p) {
    const Morph& m = B.m;
    const double Rmax = frontRadiusUm(m), reach = Rmax * (0.12 + 0.88 * std::pow(p, 0.8));
    Look look = cellLook(m, Rmax * 0.01);
    look.density = 0.6f; look.wall = 0; look.mark = MK_SMOOTH; look.halo = 0;
    struct Tip { double x, y, a; uint64_t h; };
    std::vector<Tip> tips;
    const int n0 = 7 + (int)(h01(m.seed >> 3) * 5);
    for (int k = 0; k < n0; k++) tips.push_back({0, 0, k * TAU / n0, hk(m.seed, 101, k)});
    const double step = Rmax / 28;
    int segs = 0;
    for (int gen = 0; gen < 40 && !tips.empty() && segs < 700; gen++) {
        std::vector<Tip> next;
        for (const Tip& t : tips) {
            const double r0 = std::sqrt(t.x * t.x + t.y * t.y);
            if (r0 >= reach) continue;
            const double a = t.a + (h01(t.h) - 0.5) * 0.5;
            double nx = t.x + std::cos(a) * step, ny = t.y + std::sin(a) * step;
            const double r1 = std::sqrt(nx * nx + ny * ny);
            if (r1 > reach) { const double f = (reach - r0) / std::max(1e-9, r1 - r0); nx = t.x + (nx - t.x) * f; ny = t.y + (ny - t.y) * f; }
            const float w = (float)(Rmax * 0.012 * (1.2 - 0.6 * r0 / Rmax));
            B.tube({{t.x, t.y}, {nx, ny}}, {w, w}, look);
            segs++;
            const uint64_t h = mix64(t.h);
            if (h01(h >> 7) < 0.13 && (int)next.size() < 220) { next.push_back({nx, ny, a - 0.38, mix64(h + 1)}); next.push_back({nx, ny, a + 0.38, mix64(h + 2)}); }
            else next.push_back({nx, ny, a, h});
        }
        tips.swap(next);
    }
    Body& core = B.radial(0, 0, 0, Rmax * 0.08, 1, look);
    fillOutline(core, 0, 2, 2, 2, 0.1, m.seed, 0, 0);
    B.finish(core);
}
}   // namespace

void buildShape(const Specimen& sp, double t, double pixel, Shape& out) {
    out = Shape();
    if (!sp.m) return;
    const Morph& m = *sp.m;
    const double T = actSeconds(m, sp.act), tau = t - sp.t0;
    const double p = sp.act == AN_LIVE ? 0 : clampd(tau / T, 0, 1);
    Pose P = poseAt(m, sp, t);
    if (sp.act != AN_LIVE && sp.act != AN_TO_LIGHT && sp.act != AN_TO_FIELD && sp.act != AN_GATHER) { P.x = sp.x; P.y = sp.y; P.angle = sp.angle; }
    Build B(m, sp, t, pixel, out);
    B.place(P.x, P.y, P.angle, P.mirror);
    CellCtx C; C.form = sp.form;
    C.single = sp.act != AN_LIVE && sp.act != AN_GLOW && sp.act != AN_TO_LIGHT && sp.act != AN_TO_FIELD;
    // the forms of the life cycle
    switch (sp.form) {
        case CF_SPORE: if (sp.act == AN_WAKE) break; sporeForm(B, 0, 1); return;
        case CF_CYST: cystForm(B, P, 1); return;
        case CF_GHOST: C.ghost = 0.85; C.limbs = 0; buildCell(B, P, C); return;
        case CF_FOSSIL: C.limbs = 0; buildCell(B, P, C); return;
        case CF_DIVIDING: C.grow = 0.8; C.pinch = 0.75; C.limbs = 0.6; buildCell(B, P, C); return;
        case CF_SLUG: case CF_STALK:
            if (m.plan == MP_GATHERING) { buildCell(B, P, C); return; }
            C.form = CF_COLONY; [[fallthrough]];
        case CF_COLONY: {
            const int n = smallCellPlan(m.plan) ? 36 : 7;
            const double w = std::max(m.widthUm, m.lengthUm);
            C.form = CF_CELL;
            crowd(B, P, n, [&](int k, Specimen& one, double& x, double& y, double& a, CellCtx& c) {
                const double r = std::sqrt(k + 0.5) * w * (smallCellPlan(m.plan) ? 0.62 : 0.9), an = k * 2.39996 + h01(one.seed) * 0.4;
                x = P.x + r * std::cos(an); y = P.y + r * std::sin(an); a = h01(one.seed >> 9) * TAU; c.limbs = 0.5;
            });
            return;
        }
        default: break;
    }
    switch (sp.act) {
        case AN_DIVIDE: {
            if (m.division == DV_MULTIPLE && !smallCellPlan(m.plan)) {   // many at once: its contents cut into daughters, its wall gaping
                const double R = m.widthUm * 0.5;
                Look shell = cellLook(m, R); shell.density = 0.03f; shell.gapW = (float)(0.8 * ramp(p, 0.65, 0.85)); shell.ghost = (float)(0.6 * ramp(p, 0.8, 1));
                Body& b = B.radial(0, 0, 0, R, std::max(1.0, m.aspect * 0.8), shell);
                fillOutline(b, 0, 2, 2, 2, 0.02, m.seed, t, 0.05);
                B.finish(b);
                const int nd = p < 0.25 ? 1 : (p < 0.4 ? 2 : (p < 0.55 ? 4 : 8));
                for (int k = 0; k < nd; k++) {
                    const double a = k * TAU / nd + 0.3, out = R * (nd == 1 ? 0 : 0.45) + R * 2.2 * ease(ramp(p, 0.75, 1)) * (0.7 + 0.3 * h01(hk(m.seed, 111, k)));
                    Build b2 = B; b2.place(B.at(std::cos(a) * out, std::sin(a) * out).x, B.at(std::cos(a) * out, std::sin(a) * out).y, B.angle(a), P.mirror);
                    CellCtx c; c.scale = 1.0 / std::sqrt((double)nd) * 0.95; c.limbs = ramp(p, 0.75, 1); c.single = true;
                    b2.group = k;
                    buildCell(b2, P, c);
                }
                return;
            }
            if (smallCellPlan(m.plan) || protocellPlan(m.plan) || m.division == DV_FRAGMENTS) {
                C.grow = m.plan == MP_SPHERE || m.plan == MP_FILAMENT ? 0 : 0.95 * ease(p / 0.6);
                C.pinch = m.division == DV_FRAGMENTS ? ease(p / 0.5) : ease(ramp(p, 0.4, 0.88));
                C.part = m.division == DV_FRAGMENTS ? ease(ramp(p, 0.5, 1)) : 0.6 * ease(ramp(p, 0.88, 1));
                if (m.plan == MP_SPHERE) C.scale = 1 + 0.12 * ease(p / 0.5);
                if (protocellPlan(m.plan)) {   // a vesicle pinched in two
                    const double sep = m.widthUm * 0.5 * (0.2 + 0.9 * ease(ramp(p, 0.3, 1)));
                    for (int k = 0; k < 2; k++) {
                        Build b2 = B; const P2 c = B.at((k ? 1 : -1) * sep, 0); b2.place(c.x, c.y, P.angle, P.mirror); b2.group = k;
                        CellCtx c2; c2.scale = 1 - 0.2 * ease(p / 0.4); c2.single = true;
                        buildCell(b2, P, c2);
                    }
                    return;
                }
                buildCell(B, P, C);
                return;
            }
            // a large cell in two: across its axis (a ciliate's), along it (a flagellate's, an armoured one's), an amoeba's two lobes
            const bool along = m.plan == MP_FLAGELLATE || m.plan == MP_ARMOURED || m.plan == MP_SHELLED;
            const double L = m.lengthUm * 0.5, W = m.widthUm * 0.5;
            const double sep = (along ? W : L) * (0.25 + 0.8 * ease(p));
            for (int k = 0; k < 2; k++) {
                Build b2 = B;
                const double sd = k ? 1 : -1;
                const P2 c = along ? B.at(0, sd * sep) : B.at(sd * sep, 0);
                b2.place(c.x, c.y, P.angle + sd * 0.08 * ease(ramp(p, 0.6, 1)), P.mirror);
                b2.group = k;
                CellCtx c2; c2.scale = 0.78 + 0.1 * ease(ramp(p, 0.7, 1)); c2.limbs = k == 0 ? 1 : ramp(p, 0.3, 1); c2.single = true;
                if (m.plan == MP_AMOEBA) c2.round = 0.4 * (1 - ramp(p, 0.7, 1));
                buildCell(b2, P, c2);
            }
            return;
        }
        case AN_BUD: {
            C.bud = ease(p / 0.8);
            C.part = 1.5 * ease(ramp(p, 0.85, 1));
            buildCell(B, P, C);
            if (m.plan != MP_BUDDING) {   // a bud off a pole of any other
                const double R = m.widthUm * 0.5, br = R * (0.15 + 0.6 * C.bud);
                Look lk = cellLook(m, br);
                Body& b = B.radial(m.lengthUm * 0.5 + br * 0.85 + C.part * R, 0, 0, br, 1, lk);
                fillOutline(b, 0, 2, 2, 2, m.wander, m.seed ^ 0xB0D, t, 0.2);
                B.finish(b);
            }
            return;
        }
        case AN_RELEASE: C.grow = ease(p / 0.5); C.gape = ease(ramp(p, 0.55, 1)); C.ghost = 0.7 * ease(ramp(p, 0.8, 1)); buildCell(B, P, C); return;
        case AN_SPORULATE: {
            if (p < 0.75) { C.spore = p / 0.7; buildCell(B, P, C); return; }
            C.ghost = 0.9 * ease(ramp(p, 0.75, 0.95)); C.limbs = 0; C.spore = 1;
            buildCell(B, P, C);
            const bool rod = m.plan == MP_ROD || m.plan == MP_CURVED || m.plan == MP_SPIRAL || m.plan == MP_STALKED;
            const double s = !rod || m.sporeAt == 0 ? 0 : (m.sporeAt == 1 ? 0.55 : 0.82);
            Build b2 = B; const P2 c = B.at(s * m.lengthUm * 0.5 * 0.9, 0); b2.place(c.x, c.y, P.angle, P.mirror);
            sporeForm(b2, 0, 0.85);
            return;
        }
        case AN_WAKE: {
            if (p < 0.5) { sporeForm(B, p, 1); return; }
            sporeForm(B, std::max(0.5, p), 1);   // the empty coat
            const double g = ease(ramp(p, 0.5, 1));
            Build b2 = B; const P2 c = B.at(m.lengthUm * 0.5 * (0.2 + 1.2 * g), 0); b2.place(c.x, c.y, P.angle, P.mirror);
            CellCtx c2; c2.scale = 0.45 + 0.55 * g; c2.limbs = ramp(p, 0.8, 1); c2.single = true;
            buildCell(b2, P, c2);
            return;
        }
        case AN_ENCYST: {
            if (p < 0.65) { C.round = ease(p / 0.6); C.limbs = 1 - ease(p / 0.4); buildCell(B, P, C); return; }
            cystForm(B, P, ease(ramp(p, 0.65, 1)));
            return;
        }
        case AN_LYSE: {
            C.gape = ease(p / 0.3); C.ghost = 0.9 * ease(ramp(p, 0.2, 0.8)); C.limbs = 1 - ease(p / 0.3);
            buildCell(B, P, C);
            const double R = m.widthUm * 0.5, L = std::max(m.lengthUm * 0.5, R);
            for (int k = 0; k < 24; k++) {   // its contents spilling from the break
                const uint64_t q = hk(m.seed ^ sp.seed, 121, k);
                const double a = (h01(q) - 0.5) * 1.6, d = L * (0.8 + 1.6 * ease(ramp(p, 0.1, 1)) * (0.4 + 0.6 * h01(q >> 9)));
                B.dot(std::cos(a) * d, std::sin(a) * d, R * 0.08, R * 0.08, 0, DOT_DEBRIS, SB_FIELD, (float)(1 - 0.5 * p));
            }
            return;
        }
        case AN_ENGULF: {
            const Morph* prey = sp.other;
            const double R = m.widthUm * 0.5, L = R * m.aspect;
            C.toward = ease(p / 0.4) * (1 - ramp(p, 0.55, 0.7)); C.towardAngle = 0;
            buildCell(B, P, C);
            if (!prey) return;
            const double inside = ease(ramp(p, 0.35, 0.6)), digest = ramp(p, 0.6, 1);
            const double d = L * (1.45 - 0.9 * inside - 0.35 * digest);
            const double ps = clampd(R * 0.28 / std::max(1e-6, prey->bodyUm()), 0.05, 40.0) * (1 - 0.65 * digest);   // a prey of a size to be seen
            if (inside > 0.5) {
                const double vr = std::max(prey->bodyUm() * ps * 1.4, R * 0.08);
                B.dot(d, 0, vr, vr, 0, DOT_FOOD, SB_FIELD, 1);
            }
            Specimen ps2; ps2.m = prey; ps2.seed = sp.seed ^ 0x9AE7ULL; ps2.x = B.at(d, 0).x; ps2.y = B.at(d, 0).y; ps2.angle = P.angle + 1.2;
            Build b2(*prey, ps2, t, pixel, out);
            b2.place(ps2.x, ps2.y, ps2.angle, prey->mirrored);
            CellCtx c2; c2.scale = ps; c2.limbs = 1 - inside; c2.ghost = 0.8 * digest; c2.single = true;
            Pose pp = poseAt(*prey, ps2, t); pp.x = ps2.x; pp.y = ps2.y;
            buildCell(b2, pp, c2);
            return;
        }
        case AN_BRIDGE: {
            const double L = std::max(m.lengthUm, m.widthUm) * 0.5;
            const bool side = m.plan == MP_CILIATE;
            const double far = side ? m.widthUm * 1.4 : L * 2.6, near = side ? m.widthUm * 0.52 : L * 1.25;
            const double d = p < 0.3 ? far + (near - far) * ease(p / 0.3) : (p < 0.8 ? near : near + (far * 0.8 - near) * ease(ramp(p, 0.8, 1)));
            P2 ends[2];
            for (int k = 0; k < 2; k++) {
                Build b2 = B; const double sd = k ? 1 : -1;
                const P2 c = side ? B.at(0, sd * d) : B.at(sd * d, 0);
                b2.place(c.x, c.y, P.angle + (side ? 0 : (k ? PI : 0)), P.mirror);
                b2.group = k;
                ends[k] = side ? B.at(0, sd * (d - m.widthUm * 0.5)) : B.at(sd * (d - L), 0);
                CellCtx c2; c2.single = true;
                buildCell(b2, P, c2);
            }
            if (p > 0.25 && p < 0.85) pushStrand(out, {ends[0], ends[1]}, (float)(m.widthUm * (side ? 0.3 : 0.12)), (float)(m.widthUm * (side ? 0.3 : 0.12)), ST_BRIDGE, SB_FIELD, (float)(1 - ramp(p, 0.75, 0.85)), 0);
            return;
        }
        case AN_TO_LIGHT: case AN_TO_FIELD: {
            const double e = std::max(m.bodyUm(), m.widthUm), Rc = e * (sp.act == AN_TO_FIELD ? 3.5 : 6);
            const int n = sp.act == AN_TO_FIELD ? (smallCellPlan(m.plan) ? 14 : 7) : (smallCellPlan(m.plan) ? 22 : 9);
            const double field = sp.angle;
            crowd(B, P, n, [&](int k, Specimen& one, double& x, double& y, double& a, CellCtx&) {
                const uint64_t q = one.seed;
                const double r0 = Rc * std::sqrt(0.15 + 0.85 * h01(q)), a0 = h01(q >> 9) * TAU;
                const double sx = sp.x + r0 * std::cos(a0), sy = sp.y + r0 * std::sin(a0);
                if (sp.act == AN_TO_LIGHT) {   // to the spot at the specimen's place, crowding in it
                    const double g = ease(std::min(1.0, p * (0.8 + 0.5 * h01(q >> 17))));
                    const double ex = sp.x + e * 1.2 * std::cos(a0 * 3), ey = sp.y + e * 1.2 * std::sin(a0 * 3);
                    x = sx + (ex - sx) * g; y = sy + (ey - sy) * g;
                    a = std::atan2(ey - sy, ex - sx) + (g > 0.95 ? h01(q >> 23) * 2 : 0);
                } else {   // along the field, turning about when it is reversed halfway
                    const double head0 = h01(q >> 17) * TAU;
                    double hx = 0, hy = 0, head = head0;
                    for (int i = 0; i < 24; i++) {
                        const double pi = p * (i + 0.5) / 24;
                        const double f = pi < 0.5 ? field : field + PI;
                        const double since = pi < 0.5 ? pi : pi - 0.5;
                        const double from = pi < 0.5 ? head0 : field;
                        head = from + wrapAngle(f - from) * ease(since / 0.15);
                        hx += std::cos(head) * p / 24; hy += std::sin(head) * p / 24;
                    }
                    const double run = std::min(m.speed * actSeconds(m, sp.act), Rc * 2.0);
                    x = sx + hx * run; y = sy + hy * run; a = head;
                }
            });
            return;
        }
        case AN_GLOW: {
            const int n = (int)std::round(8 * std::pow(2.0, 3 * p));
            const double w = std::max(m.widthUm * 3, std::sqrt(m.widthUm * std::max(m.widthUm, m.lengthUm)));   // a long thin one's crowd a tangle
            crowd(B, P, n, [&](int k, Specimen& one, double& x, double& y, double& a, CellCtx& c) {
                const double r = std::sqrt(k + 0.5) * w * 0.9, an = k * 2.39996;
                x = sp.x + r * std::cos(an); y = sp.y + r * std::sin(an); a = h01(one.seed) * TAU;
                c.glow = ease(ramp(p, 0.55, 0.8));
            });
            return;
        }
        case AN_GATHER: {
            const double w = m.widthUm, Rg = w * 34;
            const int arms = 1 + (int)(h01(m.seed >> 5) * 2.99), n = 150;
            const double kspiral = 5.5;
            Wave wv; wv.c = {sp.x, sp.y}; wv.R = Rg * 1.1; wv.k = kspiral / Rg * TAU * 0.25; wv.arms = arms; wv.phase = t * TAU / 6.0; wv.strength = (float)(0.8 * (1 - ramp(p, 0.8, 1)));
            out.waves.push_back(wv);
            crowd(B, P, n, [&](int k, Specimen& one, double& x, double& y, double& a, CellCtx& c) {
                const uint64_t q = one.seed;
                const double r0 = Rg * std::sqrt(0.05 + 0.95 * h01(q)), th0 = h01(q >> 9) * TAU;
                const double pulse = 0.03 * std::sin(TAU * (8 * p - 2.5 * r0 / Rg));
                const double g = clampd(p * (1.1 - 0.2 * r0 / Rg) + pulse, 0, 1);
                const double r = std::max(w * 0.6 * std::sqrt(h01(q >> 17)) * 4, r0 * (1 - ease(g)));
                double best = th0, bd = 1e9;
                for (int j = 0; j < arms; j++) { const double arm = j * TAU / arms + kspiral * r / Rg; const double d = std::fabs(wrapAngle(arm - th0)); if (d < bd) { bd = d; best = arm; } }
                const double th = th0 + wrapAngle(best - th0) * ease(std::min(1.0, g * 2.2));
                x = sp.x + r * std::cos(th); y = sp.y + r * std::sin(th);
                a = th + PI;
                c.limbs = 1;
            });
            return;
        }
        case AN_FRUIT: {
            if (p < 0.25) {   // the mound, its tip rising
                Specimen s2 = sp; s2.act = AN_GATHER; s2.t0 = t - actSeconds(m, AN_GATHER) * 0.98;
                buildShape(s2, t, pixel, out);
                out.waves.clear();
                return;
            }
            if (p < 0.55) { C.form = CF_SLUG; C.scale = 0.4 + 0.6 * ease(ramp(p, 0.25, 0.4)); buildCell(B, P, C); return; }
            C.form = CF_STALK; C.grow = ease(ramp(p, 0.55, 1)); C.scale = 0.3 + 0.7 * C.grow;
            buildCell(B, P, C);
            return;
        }
        case AN_FRONT: branchingFront(B, p); return;
        case AN_THREAD: {
            const double R = m.widthUm * 0.5;
            if (p < 0.35) { C.scale = 1 + 0.4 * ease(p / 0.35); buildCell(B, P, C); return; }
            const double grow = ease(ramp(p, 0.35, 0.7)), split = ease(ramp(p, 0.75, 1));
            Look L = cellLook(m, R); L.stain = 0; L.red = 0; L.density = 0.04f; L.phase = 0.4f; L.wall = (float)(R * 0.08); L.wallDensity = 0.5f;
            const double len = R * (1 + 9 * grow), tr = R * (1.0 - 0.6 * grow);
            if (split < 0.3) {
                std::vector<P2> pts; std::vector<float> rr;
                const int n = 40;
                for (int i = 0; i <= n; i++) {
                    const double q = i / (double)n, x = -len * 0.5 + len * q;
                    const double bead = 1 - 0.65 * ramp(p, 0.7, 0.85) * (0.5 + 0.5 * std::cos(q * TAU * 6));
                    pts.push_back({x, R * 0.6 * std::sin(q * 4 + t * 0.2)}); rr.push_back((float)(tr * bead));
                }
                B.tube(pts, rr, L);
            } else {
                for (int k = 0; k < 6; k++) {
                    const double q = (k + 0.5) / 6.0, x = -len * 0.5 + len * q;
                    const uint64_t h = hk(m.seed, 131, k);
                    Body& b = B.radial(x + (h01(h) - 0.5) * R * split, R * 0.6 * std::sin(q * 4 + t * 0.2) + (h01(h >> 9) - 0.5) * R * 2 * split, 0, tr * 1.25, 1, L);
                    fillOutline(b, 0, 2, 2, 2, m.wander, h, t, 0.4);
                    B.finish(b);
                }
            }
            return;
        }
        default: break;
    }
    buildCell(B, P, C);
}

}   // namespace morph

void specimenBox(const Specimen& s, double t, double& x0, double& y0, double& x1, double& y1) {
    morph::Shape sh;
    morph::buildShape(s, t, s.m ? std::max(1e-3, s.m->widthUm * 0.05) : 0.01, sh);
    x0 = sh.x0; y0 = sh.y0; x1 = sh.x1; y1 = sh.y1;
    for (const morph::Wave& w : sh.waves) { x0 = std::min(x0, w.c.x - w.R); y0 = std::min(y0, w.c.y - w.R); x1 = std::max(x1, w.c.x + w.R); y1 = std::max(y1, w.c.y + w.R); }
}

void specimenPlace(const Specimen& s, double t, double& x, double& y, double& angle) {
    x = s.x; y = s.y; angle = s.angle;
    if (!s.m || s.act != AN_LIVE) return;
    const morph::Pose P = morph::poseAt(*s.m, s, t);
    x = P.x; y = P.y; angle = P.angle;
}

double specimenExtentUm(const Specimen& s, double t) {
    if (!s.m) return 0;
    const Morph& m = *s.m;
    const double e = m.extentUm(), body = std::max(m.lengthUm, m.widthUm);
    double r = e;
    switch (s.form) {
        case CF_COLONY: case CF_SLUG: case CF_STALK:
            if (m.plan == MP_GATHERING) r = m.widthUm * 0.5 * (s.form == CF_SLUG ? 60 * 1.2 + 6 : 130);
            else r = e * (smallCellPlan(m.plan) ? 6.5 : 4.5) + e;
            break;
        case CF_SPORE: case CF_CYST: r = std::max(e * 0.8, m.widthUm * 1.2); break;
        default: break;
    }
    switch (s.act) {
        case AN_DIVIDE: r = e * 2.6 + body; break;
        case AN_BUD: r = e * 2 + body * 1.5; break;
        case AN_RELEASE: r = e * 2.8; break;
        case AN_WAKE: r = e * 2.2 + body * 1.5; break;
        case AN_LYSE: r = e * 1.3 + body * 2.6; break;
        case AN_ENGULF: r = e * 1.6 + body; break;
        case AN_BRIDGE: r = e * 1.3 + body * 3 + m.widthUm * 2; break;
        case AN_TO_LIGHT: r = std::max(m.bodyUm(), m.widthUm) * 6 + e * 2 + morph::motionRadius(m) * 2; break;
        case AN_TO_FIELD: r = std::max(m.bodyUm(), m.widthUm) * 3.5 * 3.2 + e * 2 + morph::motionRadius(m) * 2; break;
        case AN_GLOW: r = std::max(m.widthUm * 3, std::sqrt(m.widthUm * std::max(m.widthUm, m.lengthUm))) * 0.9 * std::sqrt(64.5) + e * 2 + morph::motionRadius(m); break;
        case AN_GATHER: r = m.widthUm * 34 * 1.15 + e * 2 + morph::motionRadius(m) * 2; break;
        case AN_FRUIT: r = std::max(m.widthUm * 34 * 1.15 + e * 2 + morph::motionRadius(m) * 2, m.widthUm * 0.5 * 130); break;
        case AN_FRONT: r = morph::frontRadiusUm(m) * 1.05 + clampd(m.widthUm * 500, 400, 1500) * 0.015; break;
        case AN_THREAD: r = m.widthUm * 0.5 * 10 + m.widthUm * 2; break;
        default: break;
    }
    if (s.act == AN_LIVE && s.form == CF_CELL) r += morph::motionRadius(m);
    (void)t;
    return r;
}
