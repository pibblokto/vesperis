// Z-02: what it looks like (life/morphs.h): a species' form genome from its plan, its stage, its chemistry, its genesis's style and its seed;
// a world's kept apart; the distance between two
#include "life/morphs.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

const char* const SEEING_NAMES[WS_COUNT] = {"BRIGHT", "PHASE", "DARK", "GLOW", "ELECTRON", "SECTION"};
const char* const CYCLE_FORM_NAMES[CF_COUNT] = {"CELL", "SPORE", "CYST", "DIVIDING", "COLONY", "SLUG", "STALK", "GHOST", "FOSSIL"};
const char* const ARRANGEMENT_NAMES[AR_COUNT] = {"SINGLE", "PAIRS", "CHAINS", "CLUSTERS", "FOURS", "PACKETS", "PALISADES", "ROSETTES"};
const char* const MARK_NAMES[MK_COUNT] = {"SMOOTH", "PORES", "RIBS", "SPINES", "LATTICE", "PLATES", "NET", "GRAINS"};
const char* const FLAGELLA_PLACE_NAMES[FV_COUNT] = {"NONE", "POLAR", "BIPOLAR", "TUFT", "AROUND", "LATERAL", "GIRDLE"};
const char* const FLAGELLUM_KIND_NAMES[FS_COUNT] = {"WHIP", "TINSEL", "RIBBON"};
const char* const PLASTID_NAMES[PX_COUNT] = {"NONE", "DISCS", "CUP", "BAND", "STAR", "NET"};
const char* const POD_NAMES[PD_COUNT] = {"NONE", "LOBES", "THREADS", "NET"};
const char* const COAT_NAMES[CW_COUNT] = {"SMOOTH", "SPINED", "RIDGED", "NETTED"};
const char* const MINERAL_NAMES[MI_COUNT] = {"ORGANIC", "SILICA", "CALCITE", "CELESTINE", "IRON", "SULPHUR", "NITRILE"};

namespace {
constexpr uint64_t SALT_MORPH = 0x11FE57A6E1000001ULL;        // a form's groups, each its own stream (the group's number combined)
constexpr uint64_t SALT_PROTO_STYLE = 0x11FE57A6E1000002ULL;  // a protocell's style: its chemistry's (no genesis)

enum Group { G_VARIANT = 1, G_OUTLINE, G_SIZE, G_ARRANGE, G_WALL, G_APPEND, G_INSIDE, G_PIGMENT, G_MOTION, G_CYCLE, G_MARKS };

double logU(Rng& r, double lo, double hi) { return lo * std::pow(hi / lo, r.uni()); }
int pickOf(Rng& r, std::initializer_list<double> w) {   // a weighted pick, one draw
    double t = 0; for (double x : w) t += x;
    double u = r.uni() * t; int i = 0;
    for (double x : w) { if (u < x) return i; u -= x; i++; }
    return (int)w.size() - 1;
}

// the style a genesis gives its kin, from its 32 bits: a symmetry it favours (3-8), its walls' mineral (by its chemistry), its flagella's kind,
// its cortex, how angular, ornate and spiny its forms run; its handedness
struct Style { int sym = 5, mineral = MI_SILICA, flagKind = FS_WHIP, cortex = 0; double angular = 0.5, ornate = 0.5, spiny = 0.5; bool mirrored = false; };
Style styleOf(uint32_t st, int chemistry, bool mirrored) {
    Style S;
    S.sym = 3 + (int)(st % 6);
    const int m = (st >> 3) & 7;
    if (chemistry == CH_ACID) S.mineral = m < 5 ? MI_SULPHUR : MI_SILICA;
    else if (chemistry == CH_AMMONIA) S.mineral = m < 4 ? MI_ORGANIC : MI_SILICA;
    else if (chemistry == CH_HYDROCARBON) S.mineral = MI_NITRILE;
    else { static const int W[8] = {MI_SILICA, MI_SILICA, MI_SILICA, MI_CALCITE, MI_CALCITE, MI_CELESTINE, MI_IRON, MI_ORGANIC}; S.mineral = W[m]; }
    S.flagKind = (int)((st >> 6) % 3);
    S.cortex = (int)((st >> 8) & 3);
    S.angular = ((st >> 10) & 255) / 255.0;
    S.ornate = ((st >> 18) & 63) / 63.0;
    S.spiny = ((st >> 24) & 63) / 63.0;
    S.mirrored = mirrored;
    return S;
}

// the variants a plan has (its sub-forms: life/morph_shape.cpp draws each)
int variantsOf(int plan) {
    switch (plan) {
        case MP_VESICLE: case MP_DROPLET: case MP_CURVED: case MP_SQUARE: case MP_STAR: case MP_BUDDING: case MP_CHANGING: case MP_SPIRAL:
        case MP_CORKSCREW: case MP_ROLLER: return 2;
        case MP_SPHERE: case MP_TEST: case MP_LATTICE: case MP_CHAMBERED: case MP_ARMOURED: case MP_PLATE: case MP_BRANCHING: case MP_GATHERING:
        case MP_STALKED: return 3;
        case MP_ROD: case MP_FILAMENT: case MP_FLAGELLATE: case MP_CILIATE: case MP_SHELLED: case MP_AMOEBA: case MP_CHAIN: return 4;
        default: return 1;
    }
}

void drawVariant(Morph& M, const Microbe& s, const Style& S, Rng& r) {
    const int p = M.plan;
    const double u = r.uni(), v = r.uni();
    M.variant = std::min(variantsOf(p) - 1, (int)(u * variantsOf(p)));
    switch (p) {   // the leanings: a light-fed flagellate two-flagellate and green more often, an iron oxidiser's stalk a twisted ribbon
        case MP_FLAGELLATE: if (s.way == LW_LIGHT) M.variant = v < 0.45 ? 1 : (v < 0.8 ? 0 : 2); else if (v < 0.3) M.variant = 3; break;
        case MP_STALKED: if (s.substrate == SU_IRON) M.variant = 1; else if (M.variant == 1) M.variant = v < 0.5 ? 0 : 2; break;
        case MP_FILAMENT: if (s.way != LW_LIGHT && M.variant == 1) M.variant = 2; break;
        case MP_SPIRAL: if (s.substrate == SU_SULPHIDE) M.variant = 1; break;
        case MP_SHELLED: if (S.angular > 0.75 && M.variant == 0) M.variant = 3; break;
        default: break;
    }
}

void drawOutline(Morph& M, const Style& S, Rng& r) {
    const int p = M.plan, v = M.variant;
    const double a = r.uni(), b = r.uni(), c = r.uni(), d = r.uni(), e = r.uni(), f = r.uni();
    M.sym = 0; M.n1 = M.n2 = M.n3 = 2; M.aspect = 1; M.bend = 0; M.amp = 0; M.turns = 0; M.wander = 0.02 + 0.05 * a; M.taper = 0;
    auto superShape = [&](int m, double n1lo, double n1hi) { M.sym = m; M.n1 = n1lo + (n1hi - n1lo) * b; M.n2 = M.n3 = 1.0 + 4.0 * c; if (d < 0.3) M.n3 = M.n2 * (0.6 + 0.8 * e); };
    switch (p) {
        case MP_VESICLE: M.wander = 0.04 + 0.12 * a; break;
        case MP_LAMELLAR: M.wander = 0.03 + 0.1 * a; break;
        case MP_DROPLET: M.wander = 0.01 + 0.04 * a; break;
        case MP_SPHERE: M.aspect = 1 + 0.15 * b; M.wander = 0.01 + 0.03 * a; break;
        case MP_ROD:
            M.aspect = 1.8 + 4.2 * b * b;
            M.taper = v == 1 ? -0.4 - 0.5 * c : (v == 2 ? 0.5 + 0.5 * c : (v == 3 ? 0.3 : 0.1 * c));
            M.bend = d < 0.25 ? 0.15 * e : 0; break;
        case MP_CURVED: M.aspect = 2.5 + 2.5 * b; M.bend = (v == 1 ? 1.6 : 0.8) + 1.2 * c; M.taper = 0.2 * d; break;
        case MP_SPIRAL: M.aspect = 6 + 22 * b; M.turns = 1.5 + 3.5 * c; M.amp = (v == 1 ? 0.8 : 1.2) + 1.6 * d; break;
        case MP_CORKSCREW: M.aspect = 25 + 120 * b * b; M.turns = M.aspect / (v == 1 ? 1.8 + 1.4 * c : 3 + 4 * c); M.amp = 0.8 + 1.2 * d; break;
        case MP_FILAMENT:
            M.aspect = 0.4 + 1.6 * b;   // a cell's length over its width
            M.bend = 0.2 + 1.5 * c;     // the filament's sweep
            if (v == 3) { M.turns = 3 + 6 * d; M.amp = 1.2 + 1.5 * e; } break;
        case MP_SQUARE: superShape(4, 18, 60); M.n2 = M.n3 = 18 + 40 * c; M.aspect = 1 + 0.25 * d; M.wander = 0.03 + 0.05 * a; break;
        case MP_STAR: superShape(S.sym >= 5 ? S.sym : 6, 0.2 + 0.15 * S.angular, 0.55); M.n2 = M.n3 = 0.8 + 1.0 * c; break;
        case MP_STALKED: M.aspect = 1.6 + 2.2 * b; M.bend = 0.3 + 0.9 * c; break;
        case MP_BUDDING: M.aspect = 1.2 + 0.5 * b; break;
        case MP_CHANGING: M.wander = 0.18 + 0.25 * a; M.aspect = 1 + 0.4 * b; break;
        case MP_AMOEBA: M.wander = 0.1 + 0.3 * a; M.aspect = v == 3 ? 2.2 + 1.8 * b : 1 + 0.8 * b; break;
        case MP_FLAGELLATE:
            M.aspect = v == 0 ? 2.2 + 2.5 * b : (v == 1 ? 1.1 + 0.5 * b : (v == 2 ? 1.4 + 0.9 * b : 1.2 + 0.6 * b));
            M.taper = v == 0 ? 0.55 + 0.4 * c : (v == 2 ? 0.4 + 0.4 * c : 0.1 * c);
            M.wander = 0.02 + 0.04 * a; break;
        case MP_CILIATE:
            M.aspect = v == 0 ? 2 + 1.6 * b : (v == 1 ? 2.2 + 1.5 * b : (v == 2 ? 1.2 + 0.5 * b : 1.3 + 0.4 * b));
            M.taper = v == 1 ? 0.8 : (v == 2 ? 0.45 : 0.15 + 0.3 * c); break;
        case MP_SHELLED:
            if (v == 0) { M.aspect = 1; M.sym = 0; }
            else if (v == 1) { M.aspect = 3 + 8 * b * b; M.taper = 0.3 + 0.6 * c; }
            else if (v == 2) { M.aspect = 3 + 4 * b; M.bend = 0.6 + 0.9 * c; M.taper = 0.5 + 0.4 * d; }
            else { superShape(S.sym >= 3 && S.sym <= 6 ? S.sym : 3, 3 + 4 * S.angular, 9); M.n2 = M.n3 = 4 + 6 * c; }
            break;
        case MP_TEST: M.aspect = v == 0 ? 1.3 + 0.7 * b : (v == 1 ? 1 : 1.3 + 0.5 * b); M.taper = v == 0 ? 0.35 + 0.4 * c : 0.1; M.wander = 0.02 + 0.04 * a; break;
        case MP_LATTICE: M.aspect = v == 2 ? 1.3 + 0.6 * b : 1; break;
        case MP_CHAMBERED: M.aspect = 1; M.turns = 1.4 + 1.6 * b; M.amp = 1.12 + 0.2 * c; break;   // whorls and the chambers' growth
        case MP_ARMOURED:
            M.aspect = v == 1 ? 1 + 0.3 * b : 1 + 0.35 * b;
            superShape(v == 0 ? 4 : 3, 2.5 + 2 * S.angular, 6); M.wander = 0.02; break;
        case MP_ROLLER: M.aspect = 1 + 0.08 * b; break;
        case MP_PLATE: M.aspect = 1; break;
        case MP_CHAIN: M.aspect = v == 0 ? 2.2 + 1.5 * b : (v == 2 ? 2.5 + 3 * b : 1 + 0.6 * b); M.taper = v == 0 ? 0.5 * c : 0.1; break;
        case MP_BRANCHING: M.aspect = 1; M.bend = 0.3 + 0.6 * b; break;
        case MP_GATHERING: M.wander = 0.12 + 0.22 * a; M.aspect = 1.1 + 1.3 * b; break;
        default: break;
    }
    (void)f;
}

void drawSize(Morph& M, Rng& r) {
    const int p = M.plan, v = M.variant;
    double w = 1;
    switch (p) {
        case MP_VESICLE: w = logU(r, 1.5, 16); break;
        case MP_LAMELLAR: w = logU(r, 2.5, 25); break;
        case MP_DROPLET: w = logU(r, 1.5, 30); break;
        case MP_SPHERE: w = logU(r, 0.5, 2.4); break;
        case MP_ROD: w = logU(r, 0.4, 1.5); break;
        case MP_CURVED: w = logU(r, 0.35, 1.0); break;
        case MP_SPIRAL: w = v == 1 ? logU(r, 1.5, 3.5) : logU(r, 0.4, 1.4); break;
        case MP_CORKSCREW: w = logU(r, 0.12, 0.5); break;
        case MP_FILAMENT: w = logU(r, 1.0, 12); break;
        case MP_SQUARE: w = logU(r, 1.5, 6); break;
        case MP_STAR: w = logU(r, 0.6, 2.2); break;
        case MP_STALKED: w = logU(r, 0.4, 0.9); break;
        case MP_BUDDING: w = logU(r, 0.6, 1.8); break;
        case MP_CHANGING: w = logU(r, 0.5, 5); break;
        case MP_AMOEBA: w = logU(r, 10, 400); break;
        case MP_FLAGELLATE: w = logU(r, 4, 25); break;
        case MP_CILIATE: w = logU(r, 15, 120); break;
        case MP_SHELLED: w = v == 0 || v == 3 ? logU(r, 10, 180) : logU(r, 5, 30); break;
        case MP_TEST: w = logU(r, 25, 220); break;
        case MP_LATTICE: w = logU(r, 40, 280); break;
        case MP_CHAMBERED: w = logU(r, 80, 1600); break;
        case MP_ARMOURED: w = logU(r, 18, 120); break;
        case MP_ROLLER: w = v == 0 ? logU(r, 150, 1500) : logU(r, 30, 150); break;
        case MP_PLATE: w = logU(r, 20, 160); break;
        case MP_CHAIN: w = logU(r, 4, 30); break;            // a cell's width
        case MP_BRANCHING: w = v == 1 ? logU(r, 8, 40) : logU(r, 0.5, 1.6); break;   // a thread's width
        case MP_GATHERING: w = logU(r, 6, 24); break;         // an amoeba's
        default: break;
    }
    M.widthUm = w;
    M.lengthUm = w * M.aspect;
}

void drawArrangement(Morph& M, Rng& r) {
    const int p = M.plan;
    const double u = r.uni(), n = r.uni();
    M.arrangement = AR_SINGLE; M.groupN = 1;
    switch (p) {
        case MP_SPHERE: M.arrangement = pickOf(r, {3, 2, 1.5, 1.5, 0.8, 0.6, 0, 0}); break;
        case MP_ROD: M.arrangement = pickOf(r, {4, 1.5, 1.2, 0, 0, 0, 0.8, 0}); break;
        case MP_CURVED: case MP_SPIRAL: M.arrangement = u < 0.85 ? AR_SINGLE : AR_PAIRS; break;
        case MP_STALKED: M.arrangement = u < 0.6 ? AR_SINGLE : AR_ROSETTES; break;
        case MP_SQUARE: M.arrangement = u < 0.6 ? AR_SINGLE : AR_FOURS; break;
        case MP_STAR: case MP_CHANGING: case MP_BUDDING: M.arrangement = u < 0.8 ? AR_SINGLE : AR_PAIRS; break;
        case MP_SHELLED: M.arrangement = u < 0.7 ? AR_SINGLE : AR_CHAINS; break;
        case MP_VESICLE: case MP_LAMELLAR: case MP_DROPLET: M.arrangement = u < 0.4 ? AR_SINGLE : (u < 0.65 ? AR_PAIRS : AR_CLUSTERS); break;   // fused, budding
        default: break;
    }
    switch (M.arrangement) {
        case AR_PAIRS: M.groupN = 2; break;
        case AR_CHAINS: M.groupN = 3 + (int)(n * 9); break;
        case AR_CLUSTERS: M.groupN = 6 + (int)(n * 14); break;
        case AR_FOURS: M.groupN = 4; break;
        case AR_PACKETS: M.groupN = 8; break;
        case AR_PALISADES: M.groupN = 3 + (int)(n * 5); break;
        case AR_ROSETTES: M.groupN = 3 + (int)(n * 5); break;
        default: break;
    }
    // the colonies' and the filaments' own counts
    if (p == MP_FILAMENT) M.groupN = 12 + (int)(n * 40);
    if (p == MP_ROLLER) M.groupN = M.variant == 0 ? 300 + (int)(n * 900) : 16 << (int)(n * 2.99);
    if (p == MP_PLATE) M.groupN = M.variant == 1 ? 8 + (int)(n * 4) * 8 : (M.variant == 0 ? 4 << (int)(n * 2.99) : 16 << (int)(n * 2.99));
    if (p == MP_CHAIN) M.groupN = M.variant == 0 ? (n < 0.5 ? 4 : 8) : 4 + (int)(n * 10);
    if (p == MP_BRANCHING) M.groupN = 3 + (int)(n * 5);   // its first branches
    if (p == MP_GATHERING) M.groupN = 1;
    if (protocellPlan(p) && M.arrangement == AR_CLUSTERS) M.groupN = 3 + (int)(n * 5);
}

void drawWall(Morph& M, const Microbe& s, const Style& S, Rng& r) {
    const int p = M.plan, v = M.variant;
    const double a = r.uni(), b = r.uni(), c = r.uni(), d = r.uni(), e = r.uni(), f = r.uni();
    M.wall = 0.05 + 0.08 * a; M.mark = MK_SMOOTH; M.marks = 0; M.spines = 0; M.spineLen = 0; M.spineBranch = 0; M.capsule = 0; M.sheath = 0;
    const double orn = S.ornate, spiny = S.spiny;
    if (smallCellPlan(p)) {
        if (b < 0.3) M.capsule = 0.25 + 0.9 * c;   // a capsule's halo
        if (s.clade == 1 && d < 0.4) { M.mark = MK_NET; M.marks = 10 + (int)(e * 20); }   // an S-layer's lattice
        if (p == MP_SPHERE && v == 1) { M.spines = 8 + (int)(e * 16); M.spineLen = 0.3 + 0.5 * f; M.spineBranch = 1; }   // the hami's hooks
        if (p == MP_FILAMENT) { M.sheath = v == 2 ? 0.25 + 0.3 * c : (b < 0.4 ? 0.06 + 0.12 * c : 0); M.capsule = 0; }
        if (p == MP_STAR) M.capsule = 0;
    }
    switch (p) {
        case MP_VESICLE: M.wall = 0.02 + 0.03 * a; break;
        case MP_LAMELLAR: M.wall = 0.03 + 0.03 * a; M.marks = 3 + (int)(b * 6); break;   // its layers
        case MP_DROPLET: M.wall = 0; break;
        case MP_AMOEBA: M.wall = 0.01; if (v == 2) { M.spines = 0; } break;
        case MP_FLAGELLATE:
            M.wall = 0.03 + 0.03 * a;
            if (v == 0 && b < 0.7) { M.mark = MK_RIBS; M.marks = 12 + (int)(c * 30); }   // the pellicle's strips
            else if (orn > 0.6 && b < 0.4) { M.mark = MK_PLATES; M.marks = 20 + (int)(c * 40); }   // scales
            break;
        case MP_CILIATE: M.wall = 0.02 + 0.02 * a; M.mark = MK_NET; M.marks = 20 + (int)(c * 30); break;
        case MP_SHELLED:
            M.wall = 0.04 + 0.04 * a;
            M.mark = b < 0.55 + 0.3 * orn ? MK_PORES : MK_RIBS; M.marks = v == 0 || v == 3 ? 16 + (int)(c * 40) : 10 + (int)(c * 28);
            if (spiny > 0.5 && d < 0.5 && (v == 0 || v == 3)) { M.spines = 8 + (int)(e * 24); M.spineLen = 0.04 + 0.12 * f; }
            break;
        case MP_TEST:
            M.wall = 0.08 + 0.08 * a;
            M.mark = v == 0 ? MK_GRAINS : (v == 1 ? MK_NET : MK_PLATES); M.marks = v == 1 ? 24 + (int)(c * 30) : 20 + (int)(c * 40);
            if (v == 2 && d < spiny) { M.spines = 2 + (int)(e * 6); M.spineLen = 0.15 + 0.35 * f; }
            break;
        case MP_LATTICE:
            M.wall = 0.04 + 0.04 * a; M.mark = MK_LATTICE; M.marks = 8 + (int)(b * (6 + 10 * orn));
            M.spines = v == 1 ? 20 : (int)(2 + c * (S.sym * 2 + 4)); M.spineLen = v == 1 ? 0.8 + 0.8 * d : 0.25 + 1.0 * d; M.spineBranch = e < spiny * 0.6 ? 1 + (int)(f * 2) : 0;
            break;
        case MP_CHAMBERED: M.wall = 0.05 + 0.05 * a; if (b < 0.7) { M.mark = MK_PORES; M.marks = 20 + (int)(c * 30); } if (d < spiny * 0.5) { M.spines = 6 + (int)(e * 12); M.spineLen = 0.1 + 0.25 * f; } break;
        case MP_ARMOURED:
            M.wall = 0.04 + 0.03 * a; M.mark = MK_PLATES; M.marks = 14 + (int)(b * 20);
            M.spines = v == 1 ? 3 + (c < 0.5 ? 1 : 0) : (v == 0 ? (c < 0.5 ? 2 : 0) : 0); M.spineLen = v == 1 ? 0.9 + 1.4 * d : 0.15 + 0.2 * d;
            break;
        case MP_ROLLER: M.wall = 0.01; M.capsule = 0; break;
        case MP_PLATE: M.wall = 0.05; M.capsule = v == 0 ? 0.08 + 0.1 * c : 0; if (v == 1) { M.spines = 2; M.spineLen = 0.2 + 0.5 * d; } break;
        case MP_CHAIN:
            M.wall = 0.06; if (v == 0) { M.spines = 2 + (c < 0.5 ? 2 : 0); M.spineLen = 0.6 + 1.2 * d; }
            if (v == 1) { M.spines = 2; M.spineLen = 4 + 8 * d; }   // the setae
            if (v == 3) M.capsule = 0.6 + 0.8 * c;
            break;
        case MP_BRANCHING: M.wall = 0.08; if (v == 1) M.sheath = 0.04; break;
        case MP_GATHERING: M.wall = 0.01; break;
        default: break;
    }
    if (M.chemistry == CH_ACID) M.wall *= 1.6;
    if (M.chemistry == CH_AMMONIA) M.wall *= 1.3;
    if (M.chemistry == CH_HYDROCARBON) M.wall = std::max(M.wall, 0.08);
}

void drawAppendages(Morph& M, const Microbe& s, Rng& r) {
    const int p = M.plan, v = M.variant, mo = s.motion;
    const double a = r.uni(), b = r.uni(), c = r.uni(), d = r.uni(), e = r.uni(), f = r.uni(), g = r.uni();
    M.flagella = 0; M.flagPlace = FV_NONE; M.flagLen = 0; M.beatHz = 0; M.waveLen = 0.4 + 0.4 * f;
    M.cilia = 0; M.ciliaRows = 0; M.ciliaLen = 0; M.ciliaWave = 0; M.pili = 0; M.stalk = 0; M.holdfast = false; M.pods = PD_NONE; M.podCount = 0;
    const bool swims = mo == MO_RUN_TUMBLE || mo == MO_HELICAL;
    if (smallCellPlan(p)) {
        if (swims && p != MP_CORKSCREW) {
            if (p == MP_SPIRAL) { M.flagPlace = FV_BIPOLAR; M.flagella = 2 + (int)(a * 6); }
            else if (p == MP_ROD && b < 0.5) { M.flagPlace = FV_AROUND; M.flagella = 4 + (int)(a * 6); }
            else if (b < 0.2) { M.flagPlace = FV_TUFT; M.flagella = 2 + (int)(a * 4); }
            else { M.flagPlace = FV_POLAR; M.flagella = 1; }
            M.flagLen = (p == MP_SPIRAL ? 0.3 : 1.2) + (p == MP_SPIRAL ? 0.4 : 1.8) * c; M.beatHz = 3 + 5 * d;
        }
        if (mo == MO_TWITCHING || e < 0.3) M.pili = mo == MO_TWITCHING ? 3 + (int)(g * 6) : 6 + (int)(g * 20);
        if (p == MP_STALKED) { M.stalk = 1.0 + 3.0 * c; M.holdfast = true; if (mo == MO_RUN_TUMBLE) { M.flagPlace = FV_POLAR; M.flagella = 1; M.flagLen = 1.2 + c; M.beatHz = 4; } }
        if (p == MP_BUDDING && v == 0) M.stalk = 1.2 + 2.5 * c;   // the hypha the bud grows at
        if (p == MP_STAR && v == 1) M.stalk = 0.6 + 1.2 * c;      // the prosthecae
        return;
    }
    switch (p) {
        case MP_AMOEBA:
            M.pods = v == 0 || v == 3 ? PD_LOBES : (v == 1 ? PD_THREADS : PD_THREADS); M.podCount = v == 3 ? 1 : (v == 2 ? 24 + (int)(a * 40) : 2 + (int)(a * 8));
            if (v == 2) M.flagLen = 0.6 + 0.8 * c;   // the heliozoan's axopods, over its width
            break;
        case MP_FLAGELLATE:
            M.flagPlace = v == 3 ? FV_POLAR : (v == 2 ? FV_TUFT : FV_POLAR);
            M.flagella = v == 0 ? (a < 0.7 ? 1 : 2) : (v == 1 ? (a < 0.8 ? 2 : 4) : (v == 2 ? 4 + (int)(a * 5) : 1));
            M.flagLen = v == 0 ? 0.8 + 0.8 * c : (v == 1 ? 1.0 + 0.8 * c : 0.8 + 0.8 * c); M.beatHz = 2 + 4 * d;
            if (v == 3) { M.cilia = 1; M.ciliaRows = 1; M.ciliaLen = 0.5 + 0.5 * e; M.ciliaWave = 0; }   // the collar's microvilli
            break;
        case MP_CILIATE:
            M.cilia = v == 2 ? 0.3 : (v == 1 ? 0.35 + 0.2 * a : 0.85 + 0.15 * a); M.ciliaRows = 10 + (int)(b * 30);
            M.ciliaLen = 0.08 + 0.1 * c; M.ciliaWave = 6 + 10 * d; M.beatHz = 3 + 3 * e;
            if (v == 2) { M.stalk = 1.2 + 2.5 * f; M.holdfast = true; }
            if (v == 3) { M.cilia = 0.5; M.ciliaLen = 0.18 + 0.12 * c; }   // the cirri's tufts
            break;
        case MP_SHELLED: if (g < 0.15) { M.stalk = 0.6 + 1.5 * f; M.holdfast = true; } break;
        case MP_TEST: M.pods = v == 1 ? PD_LOBES : (a < 0.6 ? PD_LOBES : PD_THREADS); M.podCount = 1 + (int)(b * 4); break;
        case MP_LATTICE: M.pods = PD_THREADS; M.podCount = 30 + (int)(a * 60); M.flagLen = 0.5 + 0.8 * c; break;   // the axopods
        case MP_CHAMBERED: M.pods = PD_NET; M.podCount = 8 + (int)(a * 20); M.flagLen = 0.6 + 1.2 * c; break;     // the reticulopodia
        case MP_ARMOURED: M.flagPlace = FV_GIRDLE; M.flagella = 2; M.flagLen = 1.0 + 0.8 * c; M.beatHz = 2 + 3 * d; break;
        case MP_ROLLER: M.flagella = 2; M.flagPlace = FV_POLAR; M.flagLen = 0.6; M.beatHz = 3 + 2 * d; break;   // each cell's two
        case MP_PLATE: if (v == 0) { M.flagella = 2; M.flagPlace = FV_POLAR; M.flagLen = 0.8; M.beatHz = 3 + 2 * d; } break;
        case MP_GATHERING: M.pods = PD_LOBES; M.podCount = 2 + (int)(a * 3); break;
        default: break;
    }
}

void drawInside(Morph& M, const Microbe& s, const Style& S, Rng& r) {
    const int p = M.plan, v = M.variant;
    const double a = r.uni(), b = r.uni(), c = r.uni(), d = r.uni(), e = r.uni(), f = r.uni(), g = r.uni(), h = r.uni();
    M.nucleus = 0; M.vacuoles = 0; M.vacPulse = 0; M.granules = 0.1 + 0.3 * a; M.plastids = 0; M.plastidShape = PX_NONE; M.eyespot = false;
    M.gas = 0; M.magnetosomes = 0; M.droplets = 0;
    const bool light = s.way == LW_LIGHT, fresh = s.saltHi == 0;
    if (s.taxis & TX_FIELD) M.magnetosomes = 6 + (int)(b * 18);
    if (s.way == LW_CHEMICAL && s.substrate == SU_SULPHIDE && c < 0.7) M.droplets = 3 + (int)(d * 9);   // the sulphur globules (Beggiatoa's)
    if (smallCellPlan(p)) {
        M.granules = 0.05 + 0.25 * a;
        if (p == MP_SQUARE) M.gas = 0.4 + 0.5 * e;
        else if (light && (p == MP_FILAMENT || p == MP_SPHERE) && e < 0.35) M.gas = 0.3 + 0.5 * f;
        if (p == MP_FILAMENT && light && v == 1) M.vacuoles = 1;   // its heterocysts (a flag: life/morph_shape.cpp spaces them)
        return;
    }
    if (protocellPlan(p)) {
        M.granules = p == MP_DROPLET ? 0.2 + 0.5 * a : 0.05 + 0.15 * a;
        if (p == MP_VESICLE && v == 1) M.vacuoles = 1 + (int)(b * 4);   // vesicles in it
        if (p == MP_DROPLET && v == 1) M.vacuoles = 2 + (int)(b * 8);
        return;
    }
    // the large cells and the colonies' cells
    M.nucleus = p == MP_CILIATE ? 0.22 + 0.12 * b : 0.12 + 0.12 * b;
    M.granules = 0.3 + 0.6 * a;
    if ((fresh && (p == MP_AMOEBA || p == MP_FLAGELLATE || p == MP_CILIATE || p == MP_TEST || p == MP_GATHERING)) || (p == MP_CILIATE && c < 0.5)) {
        M.vacuoles = p == MP_CILIATE ? 2 : 1; M.vacPulse = 6 + 14 * d;   // the contractile vacuoles
    }
    if (s.way == LW_GRAZER || s.way == LW_PREDATOR) M.vacuoles += 2 + (int)(e * 5);   // the food vacuoles
    if (light) {
        if (p == MP_FLAGELLATE) { M.plastidShape = v == 1 ? PX_CUP : (f < 0.6 ? PX_DISCS : PX_BAND); }
        else if (p == MP_SHELLED || p == MP_ARMOURED) M.plastidShape = f < 0.7 ? PX_DISCS : PX_BAND;
        else if (p == MP_CHAIN || p == MP_PLATE || p == MP_ROLLER) M.plastidShape = PX_CUP;
        else if (p == MP_BRANCHING && v == 1) M.plastidShape = f < 0.5 ? PX_NET : PX_BAND;
        else M.plastidShape = f < 0.5 ? PX_DISCS : (f < 0.75 ? PX_STAR : PX_BAND);
        M.plastids = M.plastidShape == PX_DISCS ? 6 + (int)(g * 30) : (M.plastidShape == PX_BAND ? 1 + (int)(g * 2.5) : 1 + (int)(g * 1.5));
        if ((s.taxis & TX_LIGHT) && (p == MP_FLAGELLATE || p == MP_ARMOURED || p == MP_ROLLER || p == MP_PLATE)) M.eyespot = true;
    } else if (p == MP_LATTICE || p == MP_ARMOURED || p == MP_CHAMBERED) {
        if (h < 0.35) { M.plastidShape = PX_DISCS; M.plastids = 10 + (int)(g * 30); }   // the symbionts they keep (zooxanthellae)
    }
    M.droplets = std::max(M.droplets, h < 0.4 ? 1 + (int)(g * 6) : 0);
    (void)S;
}

}   // namespace

RGB pigmentColour(int pg) {
    switch (pg) {
        case PG_GREEN: return RGB(0.35f, 0.62f, 0.22f);
        case PG_PURPLE: return RGB(0.55f, 0.25f, 0.55f);
        case PG_ORANGE: return RGB(0.85f, 0.5f, 0.15f);
        case PG_RED: return RGB(0.75f, 0.18f, 0.15f);
        case PG_BROWN: return RGB(0.5f, 0.33f, 0.18f);
        case PG_BLUE_GREEN: return RGB(0.2f, 0.55f, 0.5f);
        case PG_BLACK: return RGB(0.15f, 0.15f, 0.12f);
        default: return RGB(0.82f, 0.8f, 0.74f);
    }
}

namespace {
void drawPigment(Morph& M, const Microbe& s, Rng& r) {
    const double a = r.uni();
    M.pigment = s.pigment;
    M.density = s.way == LW_LIGHT ? 0.45 + 0.45 * a : (s.pigment != PG_NONE ? 0.25 + 0.4 * a : 0.08 + 0.12 * a);
    M.glows = s.glows;
}

void drawMotion(Morph& M, const Microbe& s, Rng& r) {
    const double a = r.uni(), b = r.uni();
    const double body = std::max(M.lengthUm, M.widthUm);
    M.motion = s.motion; M.taxis = s.taxis; M.speed = 0; M.runS = 0.6 + 1.4 * b;
    switch (s.motion) {
        case MO_RUN_TUMBLE: M.speed = body * (8 + 12 * a); break;
        case MO_HELICAL: M.speed = largeCellPlan(M.plan) || colonyPlan(M.plan) ? body * (2 + 6 * a) : body * (6 + 10 * a); break;
        case MO_GLIDING: M.speed = 0.5 + 4 * a; break;
        case MO_TWITCHING: M.speed = 0.2 + 0.8 * a; break;
        case MO_CRAWLING: M.speed = 0.3 + 2.5 * a; break;
        case MO_ROLLING: M.speed = body * (0.3 + 0.7 * a); break;
        case MO_CORKSCREW: M.speed = 5 + 15 * a; break;
        default: break;
    }
    if (M.chemistry == CH_AMMONIA) M.speed *= 0.3;
    if (M.chemistry == CH_HYDROCARBON) M.speed *= 0.1;
}

// the marks the eye tells small cells and protocells apart by: refractile granules at a pole or both (the storage polymers' bright dots in phase
// contrast), the nucleoid's clear zone, a wavy axis; a protocell's organic tint (tholins: Sagan and Khare 1979) and its clay grains
void drawMarks(Morph& M, const Microbe& s, Rng& r) {
    const double a = r.uni(), b = r.uni(), c = r.uni(), d = r.uni(), e = r.uni(), f = r.uni(), g = r.uni();
    const int p = M.plan;
    if (smallCellPlan(p) && p != MP_FILAMENT) {
        M.polar = a < 0.2 ? 1 : (a < 0.38 ? 2 : 0);
        M.nucleoid = b < 0.5 ? 0.35 + 0.55 * c : 0;
        if ((p == MP_ROD || p == MP_CURVED || p == MP_STALKED) && d < 0.22) { M.wave = 0.15 + 0.35 * e; M.waves = 0.6 + 1.4 * f; }
    }
    if (protocellPlan(p)) {
        const bool tholin = M.chemistry == CH_HYDROCARBON || (M.chemistry == CH_AMMONIA && a < 0.6);
        M.pigment = tholin ? (b < 0.6 ? PG_ORANGE : PG_BROWN) : (b < 0.45 ? PG_NONE : (b < 0.8 ? PG_BROWN : PG_ORANGE));
        M.density = M.pigment == PG_NONE ? M.density : 0.18 + 0.3 * c;
        M.clay = d < 0.45 ? 1 + (int)(e * 4) : 0;
        M.wander += 0.1 * g;
    }
    (void)s;
}

void drawCycleForms(Morph& M, const Style& S, Rng& r) {
    const double a = r.uni(), b = r.uni(), c = r.uni();
    M.coat = a < 0.4 ? CW_SMOOTH : (a < 0.4 + 0.25 * (0.5 + S.spiny) ? CW_SPINED : (a < 0.85 ? CW_RIDGED : CW_NETTED));
    M.sporeSize = smallCellPlan(M.plan) ? 0.6 + 0.6 * b : 0.7 + 0.25 * b;
    M.sporeAt = smallCellPlan(M.plan) ? (c < 0.45 ? 0 : (c < 0.8 ? 1 : 2)) : 0;
}
}   // namespace

double Morph::bodyUm() const {
    const double body = std::max(lengthUm, widthUm) * 0.5 * (1 + wander), R = widthUm * 0.5;
    double e = body;
    switch (arrangement) {
        case AR_PAIRS: e = body * 2.1; break;
        case AR_CHAINS: e = std::max(lengthUm, widthUm) * 1.04 * (groupN * 0.5) + body; break;
        case AR_CLUSTERS: e = std::sqrt(groupN + 0.5) * widthUm * 0.55 + body; break;
        case AR_FOURS: e = body + widthUm * 0.75; break;
        case AR_PACKETS: e = body + widthUm * 1.0; break;
        case AR_PALISADES: e = std::max(body + lengthUm * 0.15, widthUm * 1.08 * groupN * 0.5 + body); break;
        case AR_ROSETTES: e = lengthUm * (1.0 + stalk) + body; break;
        default: break;
    }
    switch (plan) {
        case MP_FILAMENT: e = std::max(widthUm * 0.4, widthUm * aspect) * groupN * 0.5 + widthUm * (1 + sheath * 2); break;
        case MP_CHAIN:
            e = variant == 0 ? std::max(groupN * R + R, R * aspect) : (variant == 1 ? (groupN - 1) * 0.5 * 1.84 * R + R * 1.2
                : (variant == 2 ? groupN * 0.75 * R * aspect + R * aspect : std::max(R * (2.2 + groupN * 0.25) * 1.2, 6.5 * R)));
            break;
        case MP_BRANCHING: e = variant == 1 ? widthUm * 30 : widthUm * 60; break;
        case MP_STALKED: case MP_BUDDING: e = std::max(e, lengthUm * 0.6 + (plan == MP_BUDDING ? lengthUm * (0.6 + stalk) : 0)); break;
        case MP_CORKSCREW: e = lengthUm * 0.55; break;
        case MP_SHELLED:
            if (arrangement == AR_CHAINS) { const double D = widthUm, h = D * 0.8; e = std::max(variant == 0 || variant == 3 ? D * 0.5 : lengthUm * 0.5, groupN * h * 0.52 + h); }
            break;
        case MP_PLATE: e = R * 1.08; break;
        case MP_CHAMBERED: e = R * 1.05; break;
        default: break;
    }
    return e;
}

double Morph::viewUm() const {
    switch (plan) {
        case MP_FILAMENT: return std::min(bodyUm(), widthUm * std::max(3.0, aspect * 4.5));
        case MP_CORKSCREW: return std::min(bodyUm(), std::max(widthUm * 8, lengthUm / std::max(1.0, turns) * 3.0));
        case MP_BRANCHING: return std::min(bodyUm(), widthUm * (variant == 1 ? 8 : 14));
        default: return bodyUm();
    }
}

double Morph::extentUm() const {
    double reach = widthUm * (capsule + sheath) + widthUm * 0.2;   // a capsule's, a sheath's, phase contrast's halo
    if (spines) reach = std::max(reach, widthUm * spineLen * (1 + 0.3 * spineBranch) + widthUm * 0.1);
    if (flagella) reach = std::max(reach, std::max(lengthUm, widthUm) * flagLen * 1.1);
    if (pods == PD_THREADS || pods == PD_NET) reach = std::max(reach, widthUm * flagLen * 1.1 + widthUm * 0.5);
    if (pods == PD_LOBES) reach = std::max(reach, widthUm * 0.9);
    if (plan == MP_AMOEBA && variant == 1) reach = std::max(reach, widthUm * 1.35);   // the filose threads
    if (plan == MP_TEST && pods != PD_NONE) reach = std::max(reach, widthUm * 1.35);  // the pods out of its aperture
    if (stalk > 0) reach = std::max(reach, lengthUm * stalk * 2.2 + widthUm);
    if (cilia > 0) reach = std::max(reach, widthUm * ciliaLen * 2.4);
    if (pili > 0) reach = std::max(reach, widthUm * 3.2);
    if (plan == MP_CHAIN && variant == 1) reach = std::max(reach, widthUm * (spineLen * 1.15 + 0.8));   // the setae from the corners
    if (plan == MP_ROLLER || plan == MP_PLATE) reach = std::max(reach, widthUm * 0.25);
    return bodyUm() + reach;
}

Morph morphOf(const Microbe& s, int chemistry, const Genesis& g, int nudge) {
    Morph M;
    M.seed = s.seed; M.plan = s.plan; M.state = s.state; M.chemistry = chemistry > CH_NONE ? chemistry : CH_WATER;
    const uint64_t base = hashCombine(s.seed ^ SALT_MORPH, 0x51D0ULL + (uint64_t)nudge);
    auto stream = [&](int grp) { return Rng(hashCombine(base, (uint64_t)grp)); };
    const uint32_t st = g.id ? g.style : (uint32_t)mix64(SALT_PROTO_STYLE ^ (uint64_t)M.chemistry);
    const Style S = styleOf(st, M.chemistry, g.id && g.mirrored);
    M.style = st; M.mineral = S.mineral; M.styleSym = S.sym; M.cortex = S.cortex; M.mirrored = S.mirrored; M.flagKind = S.flagKind;
    M.cycle = s.cycle; M.division = s.division; M.way = s.way;
    { Rng r = stream(G_VARIANT); drawVariant(M, s, S, r); }
    { Rng r = stream(G_OUTLINE); drawOutline(M, S, r); }
    { Rng r = stream(G_SIZE); drawSize(M, r); }
    { Rng r = stream(G_ARRANGE); drawArrangement(M, r); }
    { Rng r = stream(G_WALL); drawWall(M, s, S, r); }
    { Rng r = stream(G_APPEND); drawAppendages(M, s, r); }
    M.flagKind = S.flagKind;
    { Rng r = stream(G_INSIDE); drawInside(M, s, S, r); }
    { Rng r = stream(G_PIGMENT); drawPigment(M, s, r); }
    { Rng r = stream(G_MOTION); drawMotion(M, s, r); }
    { Rng r = stream(G_CYCLE); drawCycleForms(M, S, r); }
    { Rng r = stream(G_MARKS); drawMarks(M, s, r); }
    return M;
}

std::vector<Morph> morphsOf(const StarSystem& sys, int body) {
    std::vector<Morph> out;
    if (body < 0 || body >= (int)sys.bodies.size()) return out;
    auto L = lifeOf(sys);
    const Biota& B = L->bodies[body];
    out.resize(B.species.size());
    std::vector<bool> fixed(B.species.size(), false);
    // the carried spores first: their source's forms, as drawn on its world
    for (size_t i = 0; i < B.species.size(); i++) {
        const Microbe& s = B.species[i];
        if (s.fromBody < 0 || s.fromBody == body) continue;
        const std::vector<Morph> src = morphsOf(sys, s.fromBody);
        if (s.fromIndex >= 0 && s.fromIndex < (int)src.size()) { out[i] = src[s.fromIndex]; out[i].state = s.state; fixed[i] = true; }
    }
    // then the world's own, each drawn again while it looks like one already kept
    for (size_t i = 0; i < B.species.size(); i++) {
        if (fixed[i]) continue;
        const Microbe& s = B.species[i];
        Morph best; double bestD = -1;
        for (int nudge = 0; nudge < 24; nudge++) {   // drawn again until none kept is alike it; else the draw farthest from them
            const Morph m = morphOf(s, B.chemistry, B.genesis, nudge);
            double nearest = 1e9;
            for (size_t j = 0; j < B.species.size(); j++) if (fixed[j]) nearest = std::min(nearest, morphDistance(m, out[j]));
            if (nearest > bestD) { bestD = nearest; best = m; }
            if (nearest >= MORPH_ALIKE) break;
        }
        out[i] = best; fixed[i] = true;
    }
    return out;
}

Morph shipMorph(int k) {
    std::vector<Morph> kept;
    Morph m;
    for (int i = 0; i <= k && i < SHIP_SPECIES; i++) {
        const Microbe s = shipSpecies(i);
        double bestD = -1; Morph best;
        for (int nudge = 0; nudge < 24; nudge++) {
            const Morph c = morphOf(s, CH_WATER, homeGenesis(), nudge);
            double nearest = 1e9;
            for (const Morph& o : kept) nearest = std::min(nearest, morphDistance(c, o));
            if (nearest > bestD) { bestD = nearest; best = c; }
            if (nearest >= MORPH_ALIKE) break;
        }
        m = best;
        kept.push_back(m);
    }
    return m;
}

namespace {
struct Weighed { const char* name; double w; };
// the strongest values and their weights (A-04's way: colours by distance, measures by their spread, categories 0 or 1)
constexpr Weighed MORPH_WEIGHTS[] = {
    {"plan", 3.0}, {"variant", 1.0}, {"symmetry", 0.6}, {"outline", 0.6}, {"aspect", 1.0}, {"size", 1.2}, {"bend", 0.5}, {"helix", 0.6},
    {"taper", 0.3}, {"arrangement", 1.0}, {"wall", 0.3}, {"ornament", 1.0}, {"spines", 0.8}, {"capsule", 0.5}, {"sheath", 0.4},
    {"flagella", 0.7}, {"cilia", 0.8}, {"stalk", 0.6}, {"pods", 0.6}, {"nucleus", 0.3}, {"plastids", 0.6}, {"granules", 0.3},
    {"pigment", 1.2}, {"density", 0.3}, {"mineral", 0.5}, {"gas", 0.3}, {"marks", 0.8}, {"insides", 0.6}};
double colourDist(const RGB& a, const RGB& b) {
    const double dr = a.r - b.r, dg = a.g - b.g, db = a.b - b.b;
    return std::min(1.0, std::sqrt(2 * dr * dr + 4 * dg * dg + 3 * db * db) / 0.55);
}
double logApart(double a, double b, double decade) { return std::min(1.0, std::fabs(std::log10(std::max(1e-6, a) / std::max(1e-6, b))) / decade); }
}   // namespace

double morphDistance(const Morph& a, const Morph& b, std::string* shared) {
    double x[sizeof(MORPH_WEIGHTS) / sizeof(MORPH_WEIGHTS[0])];
    int k = 0;
    const bool radialA = a.sym > 0, radialB = b.sym > 0;
    x[k++] = a.plan != b.plan;
    x[k++] = a.plan == b.plan ? (a.variant != b.variant) : 1.0;
    x[k++] = (radialA || radialB) ? (a.sym != b.sym) : 0.0;
    x[k++] = std::min(1.0, std::fabs(std::log(a.n1 / b.n1)) / 1.2 + std::fabs(std::log(a.n2 / b.n2)) / 2.5 + std::fabs(a.wander - b.wander) / 0.2);
    x[k++] = logApart(a.aspect, b.aspect, 0.35);
    x[k++] = logApart(a.lengthUm, b.lengthUm, 0.6);
    x[k++] = std::min(1.0, std::fabs(a.bend - b.bend) / 0.8);
    x[k++] = std::min(1.0, std::fabs(a.turns - b.turns) / 2.0 + std::fabs(a.amp - b.amp) / 1.5);
    x[k++] = std::min(1.0, std::fabs(a.taper - b.taper) / 0.6);
    x[k++] = a.arrangement != b.arrangement;
    x[k++] = std::min(1.0, std::fabs(a.wall - b.wall) / 0.08);
    x[k++] = a.mark != b.mark ? 1.0 : std::min(1.0, std::fabs((double)a.marks - b.marks) / 20.0);
    x[k++] = std::min(1.0, std::fabs(std::log2(1.0 + a.spines) - std::log2(1.0 + b.spines)) / 2.0 + std::fabs(a.spineLen - b.spineLen) / 1.0);
    x[k++] = std::min(1.0, std::fabs(a.capsule - b.capsule) / 0.4);
    x[k++] = std::min(1.0, std::fabs(a.sheath - b.sheath) / 0.2);
    x[k++] = std::min(1.0, std::fabs(std::log2(1.0 + a.flagella) - std::log2(1.0 + b.flagella)) / 1.5 + (a.flagPlace != b.flagPlace) * 0.5);
    x[k++] = std::min(1.0, std::fabs(a.cilia - b.cilia) / 0.5);
    x[k++] = std::min(1.0, std::fabs(a.stalk - b.stalk) / 1.5);
    x[k++] = a.pods != b.pods;
    x[k++] = std::min(1.0, std::fabs(a.nucleus - b.nucleus) / 0.15);
    x[k++] = a.plastidShape != b.plastidShape ? 1.0 : std::min(1.0, std::fabs((double)a.plastids - b.plastids) / 15.0);
    x[k++] = std::min(1.0, std::fabs(a.granules - b.granules) / 0.4);
    x[k++] = colourDist(pigmentColour(a.pigment), pigmentColour(b.pigment));
    x[k++] = std::min(1.0, std::fabs(a.density - b.density) / 0.4);
    x[k++] = a.mineral != b.mineral && (a.mark != MK_SMOOTH || b.mark != MK_SMOOTH);
    x[k++] = std::min(1.0, std::fabs(a.gas - b.gas) / 0.5);
    x[k++] = std::min(1.0, std::abs(a.polar - b.polar) * 0.5 + std::fabs(a.nucleoid - b.nucleoid) / 0.6 + std::fabs(a.wave - b.wave) / 0.3 + std::abs(a.clay - b.clay) / 3.0);
    x[k++] = std::min(1.0, std::abs(a.vacuoles - b.vacuoles) / 3.0 + std::abs(a.droplets - b.droplets) / 3.0 + (a.pods == b.pods ? std::abs(a.podCount - b.podCount) / 5.0 : 0));
    double d = 0;
    if (shared) shared->clear();
    for (int i = 0; i < k; i++) {
        d += MORPH_WEIGHTS[i].w * x[i];
        if (shared && x[i] < 0.35 && MORPH_WEIGHTS[i].w >= 0.6) { if (!shared->empty()) *shared += ", "; *shared += MORPH_WEIGHTS[i].name; }
    }
    return d;
}

double morphMaxDistance() { double s = 0; for (const Weighed& w : MORPH_WEIGHTS) s += w.w; return s; }
