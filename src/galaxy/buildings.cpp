// A-03: what each age builds (see buildings.h). C-01's ten kinds and C-08's features are the medieval age's (and the features
// every age's that built them), drawn as they were but for their materials' decay and their roofs; every other kind is new. Each
// generator reads its building's seed (`U`), its materials' decay (`D`, `Dk` for a piece of it), and its people's ways of building,
// and writes A-02's forms in the building's frame: x' across its heading, z' along it (the front, side 0, faces +z').
// A-04: the people's ways are its genome's (galaxy/genome.h) read through its age (`cultureWays`): the plan's form and the roof, the
// walls' profile and their dress (bands, a cornice, an expressed frame, the door's surround, the glyphs), the tops, the towers'
// sections, the motif on the great buildings, the focal accent, the colours (`culturePalette`); the slots by what a piece is
// (`Gen::roofSlot` and the rest: the accent where the people put it).
#include "buildings.h"
#include "peoples.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>
#include <initializer_list>

const char* const BUILT_MATERIAL_NAMES[MT_COUNT] = {"timber", "earth", "dry stone", "masonry", "iron", "concrete", "mass concrete", "glass", "composite", "the far future's"};

namespace {
// the years to half gone at a middling rain (wet 0.5), the decades from a tenth gone to nine tenths, and how many decades sooner a
// wet world (1) takes it than a desert (0)
struct Life { double y50, spread, wet; };
const Life LIFE[MT_COUNT] = {
    {80, 1.2, 0.8},         // timber, thatch, hide: a roof falls in within a century, the posts rot to their holes in a few
    {400, 2.0, 1.6},        // earth: mud brick and turf; a desert keeps it for millennia, rain melts it in a century or two
    {20000, 2.5, 0.4},      // dry stone and megaliths: standing tens of millennia, leaning, fallen
    {1165.9144011798317, 8.0 / 3.0, 2.0 / 3.0},   // mortared stone and brick: C-08's decayOf (0.08 + 0.3 log10(years / 100) + 0.2 wet; half at 10^(0.92 / 0.3))
    {400, 2.0, 0.8},        // iron and steel: rusted to a skeleton in centuries, a stain in millennia
    {500, 2.0, 0.6},        // reinforced concrete: spalling in its first century, towers falling over centuries to a few millennia
    {8000, 2.0, 0.5},       // mass concrete: dams, bunkers, harbour works
    {5, 1.0, 0.0},          // glass: gone at once (the frames empty)
    {15000, 1.5, 0.4},      // composites, ceramics, grown stone, diamond glass: millennia, then crumbling
    {1, 1, 0}               // the far future's: nothing weathers
};
}   // namespace

double materialDecay(int m, double years, double wet) {
    if (m < 0 || m >= MT_COUNT || m == MT_ETERNAL) return 0;
    const Life& L = LIFE[m];
    const double yw = std::max(1.0, years) * std::pow(10.0, L.wet * (wet - 0.5));
    return clampd(0.5 + 0.8 * std::log10(yw / L.y50) / L.spread, 0, 1);
}

double buildingDecay(const Building& b, const Culture& c, int m) {   // the luck's spread C-01's (a quarter) where the material is half gone;
    const double d = materialDecay(m, c.ageYears / std::max(0.1, c.life), c.wet);   // A-04: the years over its people's lifetimes (built light, built to last)
    return clampd(d + b.luck * 2.5 * d * (1 - d), 0, 1);
}

// ---- the kinds' ages and places --------------------------------------------------------------------------------------------

int buildingAge(int k) {
    if (k >= BK_HUT && k <= BK_PALISADE) return AGE_PREHISTORIC;
    if (k >= BK_ROW && k <= BK_OBSERVATORY) return AGE_INDUSTRIAL;
    if (k >= BK_BLOCK && k <= BK_TANKS) return AGE_MODERN;
    if (k >= BK_ARCOLOGY && k <= BK_POD) return AGE_FUTURISTIC;
    if (k >= BK_SPIKE && k <= BK_MONOLITH) return AGE_FAR_FUTURE;
    switch (k) {   // the works every age built its own way (a gate, a stretch of wall, a stela, a heap; C-08's features; A-04's focal accent,
                   // aqueduct, joined towers and tunnels)
        case BK_GATE: case BK_PLATFORM: case BK_COLONNADE: case BK_STELA: case BK_RUBBLE: case BK_WALL:
        case BK_QUAY: case BK_MOLE: case BK_BOLLARD: case BK_TERRACE: case BK_CISTERN: case BK_WINDWALL: case BK_CAIRN:
        case BK_FOCUS: case BK_AQUEDUCT: case BK_TWIN: case BK_PORTAL: return -1;
        case BK_CRANE: case BK_RESERVOIR: return -1;   // T-02: an industrial people's and a modern people's in their ways
        case BK_CONTAINERS: return AGE_MODERN;
        // T-03: the works' kinds: earth, a dam, a tomb, a fort, a ship, a colossus, a lighthouse, a derrick, cut blocks and a conveyor in each
        // age's way; the machines' the modern age's, the tether the futuristic's, the lattice and the sphere the far future's, a trilithon the
        // old time's
        case BK_MOUND: case BK_DAM: case BK_TOMB: case BK_FORT: case BK_SHIP: case BK_COLOSSUS: case BK_LIGHTHOUSE: case BK_DERRICK: case BK_BLOCKS:
        case BK_CONVEYOR: case BK_HANGAR: case BK_FURNITURE: return -1;
        case BK_DOME: case BK_GREENHOUSE: return -1;   // D-05: in the way of the age that lost its sun
        case BK_DISH: case BK_BUNKER: case BK_TEETH: case BK_VEHICLE: case BK_AIRCRAFT: case BK_LAUNCHPAD: case BK_TURBINE: return AGE_MODERN;
        case BK_TETHER: return AGE_FUTURISTIC;
        case BK_LATTICE: case BK_SPHERE: return AGE_FAR_FUTURE;
        case BK_TRILITHON: return AGE_PREHISTORIC;
        default: return AGE_MEDIEVAL;   // C-01's dwellings and towers, the temple, the wall's towers, the windmill
    }
}

const char* buildingPlaceName(int k) {
    switch (k) {
        case BK_HOUSE: return "A HOUSE";
        case BK_HALL: return "A HALL";
        case BK_TOWER: return "A TOWER";
        case BK_ROTUNDA: return "A ROTUNDA";
        case BK_STELA: return "THE FOOT OF A STELA";
        case BK_HUT: return "A HUT";
        case BK_LONGHOUSE: return "A LONGHOUSE";
        case BK_TENT_RING: return "A RING OF STONES";
        case BK_STONE_CIRCLE: return "A STONE CIRCLE";
        case BK_MENHIR: return "THE FOOT OF A STANDING STONE";
        case BK_DOLMEN: return "A STONE CHAMBER";
        case BK_TEMPLE: return "A TEMPLE";
        case BK_ROW: return "A ROW OF HOUSES";
        case BK_TENEMENT: return "A TENEMENT";
        case BK_VILLA: return "A VILLA";
        case BK_CHURCH: return "A CHURCH";
        case BK_TOWN_HALL: return "A TOWN HALL";
        case BK_STATION: return "A STATION";
        case BK_MILL: return "A MILL";
        case BK_HEADFRAME: return "AN ENGINE HOUSE";
        case BK_OBSERVATORY: return "AN OBSERVATORY";
        case BK_BLOCK: return "A BLOCK";
        case BK_SUBURB: return "A HOUSE";
        case BK_HIGHRISE: return "THE FOOT OF A TOWER";
        case BK_STADIUM: return "A STADIUM";
        case BK_ARCOLOGY: return "THE GATE OF A GREAT BUILDING";
        case BK_HABITAT: return "THE GATE OF A DOME";
        case BK_SPIRE: return "THE GATE OF A SPIRE";
        case BK_POD: return "A DOME";
        case BK_RING: return "THE FOOT OF A RING";
        case BK_WRECK: return "A WRECK";
        case BK_SPIKE: case BK_HALO: case BK_PILLARS: case BK_SHAFT: case BK_MONOLITH: return "THE FOOT OF A STRUCTURE";
        case BK_FOCUS: return "THE FOOT OF A MONUMENT";
        case BK_WINDMILL: return "A WINDMILL";
        case BK_AQUEDUCT: return "THE FOOT OF AN AQUEDUCT";
        case BK_TWIN: return "THE FOOT OF A TOWER";
        case BK_PORTAL: return "THE MOUTH OF A TUNNEL";
        case BK_TOMB: return "A TOMB";                       // T-03
        case BK_BUNKER: return "A BUNKER";
        case BK_FORT: return "A FORT";
        case BK_LIGHTHOUSE: return "THE FOOT OF A LIGHTHOUSE";
        case BK_HANGAR: return "A GREAT HALL";
        case BK_SHIP: return "A WRECK";
        case BK_DAM: return "THE FOOT OF A DAM";
        case BK_COLOSSUS: return "THE FOOT OF A COLOSSUS";
        case BK_MOUND: return "A MOUND";
        case BK_MAST: return "THE FOOT OF A MAST";            // T-04: where the works keep a record
        case BK_DISH: return "THE FOOT OF A DISH";
        case BK_TRILITHON: return "A TRILITHON";
        case BK_GATE: return "A GATE";
        case BK_WALL_TOWER: return "A WALL'S TOWER";
        case BK_DOME: return "A DOME";                       // D-05
        case BK_GREENHOUSE: return "A GLASSHOUSE";
        default: return "A RUIN";
    }
}

// ---- the generators' frame and helpers --------------------------------------------------------------------------------------

namespace {
inline double u01(uint64_t seed, int k) { return unitFromHash(mix64(seed ^ (0x9E3779B97F4A7C15ULL * (uint64_t)(k + 1)))); }   // C-01's building draws
inline uint32_t pieceSeed(uint64_t seed, int k) { return (uint32_t)mix64(seed ^ (0xA03B0000ULL + (uint64_t)(k + 1) * 0xD1B54A32D192ED03ULL)); }

// the share of a low wall still standing (C-01's: some stand nearly whole, others to their foot) and of a tall thing (a tower, a
// chimney, a mast, a wall over 8 m): whole through its first decay, then falling, to a stump
double wallShare(double D, double u) { return 1 - D * (0.15 + 0.85 * u); }
double tallShare(double D, double u) { double d = clampd((D - 0.1 - 0.1 * u) / 0.55, 0, 1); return clampd(std::pow(1 - d, 1.6), 0.05, 1); }

// the doorway's place along its wall (C-01's draw: a house's, a hall's, a tower's) and its midpoint in the building's frame (the walls'
// axes run +x' on the front and the back and -z' on the right and the left, so the gap at `pos` along the axis sits at x' = pos on
// sides 0 and 2 and at z' = -pos on sides 1 and 3: C-08's correction)
void doorMid(int side, double pos, double hw, double hd, double& lx, double& lz) {
    switch (side) { case 0: lx = pos; lz = hd; break; case 2: lx = pos; lz = -hd; break; case 1: lx = hw; lz = -pos; break; default: lx = -hw; lz = -pos; break; }
}
// a house's doorway along its wall: C-01's draw, or the middle (A-04: `Building::centred`)
double houseDoorPos(const Building& b, double doorL) { return b.centred ? 0.0 : (doorL - 1.6) * (u01(b.seed, 20) - 0.5) * 1.2; }
// B-414: a mill's chimney's radius (its generator's draw), and where its floor ends before the chimney in its back end and the heap at its foot
double millChimneyR(const Building& b) { return std::min(2.4 + 0.8 * u01(b.seed, 32), 0.3 * b.hw); }
double millFloorBack(const Building& b) { return std::min(b.hd - 2.0, -b.hd + 3 * millChimneyR(b) + 1.4); }

// what a piece is built of that holds its shape (the structure's slot) and what is sheet, timber or metal (the roofing's)
bool structural(int m) { return m >= 0 && m != MT_TIMBER && m != MT_IRON && m != MT_GLASS; }

// a ring's segments and the doorway's: as many segments left out as make a gap of two metres
int ringDoorSegs(double r, int segs) { int n = 1; while (n < segs / 3 && 2 * r * std::sin(PI * n / segs) < 2.0) n++; return n; }
// a ring's segments by the plan's form: a circle twelve (fewer when small), a polygon its sides, an ellipse fourteen
int ringSegs(int form, int sides, double r) { return form == PF_POLYGON ? clampi(sides, 3, 9) : (form == PF_ELLIPSE ? 14 : (r < 2.6 ? 10 : 12)); }

// B-415: the people's own great buildings (its temples, churches, town halls, palaces, keeps and towers, its focal accent, monuments,
// gates, platforms, rotundas, joined towers, aqueducts and tunnels), where its genome speaks fully in every age; the rest are everyday
// buildings (its houses, rows, tenements, villas, blocks, halls, mills and works), where the age speaks first
// R-412: the people's dwellings (its houses, rows, tenements, villas, blocks, suburban houses, rotundas and pods), where its own colours and
// dress show from house to house
bool dwellingKind(int kind) {
    return kind == BK_HOUSE || kind == BK_ROW || kind == BK_TENEMENT || kind == BK_VILLA || kind == BK_BLOCK || kind == BK_SUBURB || kind == BK_ROTUNDA || kind == BK_POD;
}
bool greatKind(const Building& b) {
    switch (b.kind) {
        case BK_TEMPLE: case BK_CHURCH: case BK_TOWN_HALL: case BK_FOCUS: case BK_STELA: case BK_TOWER: case BK_WALL_TOWER: case BK_GATE:
        case BK_PLATFORM: case BK_ROTUNDA: case BK_TWIN: case BK_AQUEDUCT: case BK_PORTAL: case BK_OBSERVATORY: case BK_WINDMILL: return true;
        case BK_HALL: return b.style == 2;   // the palace
        default: return false;
    }
}
}   // namespace

// B-415: a genome variable as its people's age built it (`Gen::gv`). Crenels are the hand-made ages' alone; an industrial people's everyday
// buildings stand upright to their eaves (no battered, bulging or jettied walls, no storey set back on a flat roof) with chimneys for their
// tops (no statues or masts on a row), its great ones upright or stepped; a modern people's everyday buildings stand upright and bare (no
// carved cornice), a mast or nothing on their roofs, its great ones keep every profile but the bulge of rounded buttresses; a futuristic or a
// far-future people sets no statues or crenels on its tops
int ageExpressed(const Culture& c, int var, bool great) {
    const int v = c.genome.of(var);
    switch (c.age) {
        case AGE_INDUSTRIAL:
            if (var == GV_TOPS) return v == TP_CRENELS || (!great && (v == TP_STATUES || v == TP_MASTS)) ? TP_NOTHING : v;
            if (var == GV_PROFILE) return great && v == WP_STEPPED ? v : WP_UPRIGHT;
            if (var == GV_MASSING && !great && v == MS_STEPPED) return MS_COMPACT;
            return v;
        case AGE_MODERN:
            if (var == GV_TOPS) return v == TP_CRENELS || (!great && v != TP_MASTS) ? TP_NOTHING : v;
            if (var == GV_PROFILE) return !great || v == WP_BULGING ? WP_UPRIGHT : v;
            if (var == GV_ORNAMENT && !great && (v == ON_CARVED || v == ON_ENCRUSTED)) return ON_BANDED;
            return v;
        case AGE_FUTURISTIC: case AGE_FAR_FUTURE:
            if (var == GV_TOPS && (v == TP_CRENELS || v == TP_STATUES)) return TP_NOTHING;
            return v;
        default: return v;
    }
}

namespace {
struct Gen {
    const Building& b; const Culture& c; const SiteRead& st; int lod; std::vector<Form>& out; int bi;
    double sx, sz, fx, fz, sink = 0;
    int pieces = 0;
    bool great;   // B-415: one of the people's great buildings (`greatKind`)
    // R-412: a dwelling of a people that painted each house its own (`GV_POLYCHROMY`): its walls' shades over the slot's, and its walls in the
    // accent now and then
    float tone = 0; bool painted = false;
    Gen(const Building& bb, const Culture& cc, const SiteRead& ss, int l, std::vector<Form>& o, int i)
        : b(bb), c(cc), st(ss), lod(l), out(o), bi(i), sx(std::cos(bb.heading)), sz(-std::sin(bb.heading)), fx(std::sin(bb.heading)), fz(std::cos(bb.heading)), great(greatKind(bb)) {
        if (dwellingKind(bb.kind) && cc.age >= AGE_MEDIEVAL && cc.age <= AGE_FUTURISTIC && cc.genome.of(GV_POLYCHROMY) == PC_VARIED) {
            tone = (float)((U(950) - 0.5) * 13); painted = U(951) < 0.3;
        }
    }
    double U(int k) const { return u01(b.seed, k); }
    double D(int m) const { return buildingDecay(b, c, m); }
    // a piece's own decay: the building's, spread by the piece's draw where the material is half gone (a stone of a circle, a post, a span)
    double Dk(int m, int k) const { double d = D(m); return clampd(d + (U(k) - 0.5) * 1.2 * 4 * d * (1 - d), 0, 1); }
    // A-04: the genome; B-415: as the age built it (`ageExpressed`)
    int gv(int v) const { return ageExpressed(c, v, great); }
    double gf(int v) const { return c.genome.at(v); }
    bool accentOn(int place) const { return c.genome.accentPlace() == place; }
    // the slots by what a piece is: the palette's 0 the structure, 1 the accent, 2 the roofing (thatch, tiles, slate, metal sheet,
    // glass), 3 timber and iron in the ages without light (the light's in the futuristic and the far future's)
    int roofSlot(int m) const { return accentOn(AP_ROOFS) ? 1 : (structural(m) ? 0 : 2); }
    int topSlot(int m) const { return accentOn(AP_TOPS) ? 1 : (structural(m) ? 0 : 2); }
    int doorSlot() const { return accentOn(AP_DOORS) ? 1 : 0; }
    int timberSlot() const { return c.age >= AGE_FUTURISTIC ? 2 : 3; }
    int ironSlot() const { return c.age >= AGE_FUTURISTIC ? 2 : 3; }
    // R-412: the people's pitch over its roofs' (the hand-made and the industrial ages'), its eaves' overhang on a pitched, hipped or conical roof
    // (a flat roof's slab: `slabEave`)
    double pitchK() const { return c.age <= AGE_INDUSTRIAL ? 0.62 + 0.76 * gf(GV_PITCH) : 1.0; }
    double eave() const { static const double E[4] = {0.12, 0.45, 1.0, 0.7}; return c.age <= AGE_MODERN ? E[clampi(c.genome.of(GV_EAVES), 0, 3)] : 0.45; }
    double slabEave() const { static const double E[4] = {0.05, 0.45, 1.25, 0.3}; return c.age == AGE_MODERN ? E[clampi(c.genome.of(GV_EAVES), 0, 3)] : 0.2; }
    bool flared() const { return c.age <= AGE_INDUSTRIAL && c.genome.of(GV_EAVES) == EV_FLARED; }
    bool carved() const { int o = gv(GV_ORNAMENT); return o == ON_CARVED || o == ON_ENCRUSTED; }
    bool banded() const { int o = gv(GV_ORNAMENT); return o == ON_BANDED || o == ON_ENCRUSTED || accentOn(AP_BANDS); }
    void to(double lx, double lz, double& x, double& z) const { x = b.x + sx * lx + fx * lz; z = b.z + sz * lx + fz * lz; }
    // a form in the building's frame: its foot at (lx, lz), its heading the building's turned by `dh`
    Form& add(int kind, int sub, double lx, double lz, double dh, double hx, double hz, double y0, double y1, int slot, int mat, int part = 0) {
        Form e; e.kind = kind; e.sub = sub; to(lx, lz, e.x, e.z); e.heading = b.heading + dh; e.hx = hx; e.hz = hz; e.y0 = y0; e.y1 = y1;
        e.slot = slot; e.material = mat; e.part = part; e.building = bi; e.seed = pieceSeed(b.seed, pieces++);
        if (tone != 0 && slot <= 1 && part != 7 && part != 8) e.tone = tone;
        out.push_back(e);
        return out.back();
    }
    Form& box(double lx, double lz, double dh, double hx, double hz, double y0, double y1, int slot, int mat, int part = 0) { return add(FK_BOX, 0, lx, lz, dh, hx, hz, y0, y1, slot, mat, part); }
    Form& lathe(int sub, double lx, double lz, double r, double y0, double y1, int slot, int mat, int sides = 0) { Form& e = add(FK_LATHE, sub, lx, lz, 0, r, r, y0, y1, slot, mat); e.sides = sides; return e; }
    // a post that rots: whole, a stump, then its hole (a dark ring of soil, at the nearest level only)
    void post(double lx, double lz, double h, double r, int k) {
        double d = Dk(MT_TIMBER, k);
        if (d < 0.82) { double hh = d < 0.3 ? h : h * (1 - d) * (0.45 + 0.55 * U(k + 900)); lathe(0, lx, lz, r, 0, std::max(0.2, hh), timberSlot(), MT_TIMBER, 6); }
        else if (lod == 0) lathe(0, lx, lz, r * 1.7, 0, 0.035, timberSlot(), MT_TIMBER, 6);
    }
    // a low mound of what fell (a tower's top, a block's floors) at a point of the building's frame, its own material's colour
    void heap(double lx, double lz, double rx, double rz, double h, int slot, int mat) {
        if (h < 0.3) return;
        { Form& e = add(FK_PYRAMID, 0, lx, lz, U(77) * TAU, rx, rz, 0, h * 0.55, slot, mat, 3); e.sides = 9; e.p[1] = 0.4; e.broken = 0.6; }   // a low mound, its top broad and its corners worn
        if (lod != 0) return;
        for (int k = 0; k < 4; k++) {   // blocks tumbled on its flanks
            double a = U(700 + k) * TAU, d = 0.35 + 0.45 * U(710 + k), s = std::max(0.4, std::min(rx, rz) * (0.08 + 0.1 * U(720 + k)));
            Form& bb = box(lx + std::sin(a) * rx * d, lz + std::cos(a) * rz * d, U(730 + k) * TAU, s * 1.4, s, 0, s * 1.1, slot, mat, 3); bb.tilt = 0.12 + 0.15 * U(740 + k);
        }
    }
    // the top of a tall thing lying where it fell: `len` of it along `dir` (radians off the building's heading) from (lx, lz), broken; it
    // breaks up with its material (gone past three quarters: the heap alone is left)
    void fallen(int kind, int sub, double lx, double lz, double dir, double r, double len, int slot, int mat, int sides = 0, int pattern = 0) {
        if (len < 1 || D(mat) > 0.75) return;
        Form& e = add(kind, sub, lx + std::sin(dir) * r, lz + std::cos(dir) * r, dir, r, r, 0, len, slot, mat, 3);
        e.tilt = PI / 2; e.broken = 0.55; e.sides = sides; e.pattern = pattern;
    }
};

// B-415: an arch's head by the people's openings (`FK_ARCH`'s sub: 0 round, 1 pointed, 2 flat) as its age built it: a pointed arch the hand-made
// ages' (an industrial people's round), a modern people's round or flat
int archSub(const Gen& g) {
    const int op = g.gv(GV_OPENING);
    if (g.c.age >= AGE_MODERN) return op == OP_ARCHED || op == OP_ROUND || op == OP_KEYHOLE ? 0 : 2;
    if (g.c.age == AGE_INDUSTRIAL) return op == OP_SQUARE || op == OP_SLIT ? 2 : 0;
    return op == OP_TRIANGULAR ? 1 : (op == OP_SQUARE || op == OP_SLIT ? 2 : 0);
}
// B-415: a dome's profile (`FK_DOME`'s sub: 0 a hemisphere, 1 a saucer, 2 pointed, 3 an onion) as its age built it: an industrial people's a
// hemisphere where its people's was pointed or an onion, a modern people's a saucer of concrete (the later ages' their own)
int ageDome(const Gen& g, int prof) {
    if (g.c.age == AGE_MODERN) return 1;
    if (g.c.age == AGE_INDUSTRIAL && prof >= 2) return 0;
    return prof;
}

// A-04: a figure: a statue on the tops, a colossus at a town's heart: a robed body (a frustum), its shoulders, a head; `h` its height
void figure(Gen& g, double lx, double lz, double y0, double h, int slot, int mat, double D) {
    if (D > 0.8 || h < 0.5) return;
    const double r = h * 0.17;
    { Form& e = g.lathe(1, lx, lz, r, y0, y0 + h * 0.72, slot, mat, 8); e.p[0] = 0.62; e.broken = D > 0.45 ? 0.3 : 0; }
    if (D > 0.45) return;   // the head and the arms gone first
    { Form& e = g.add(FK_BLOB, 0, lx, lz, 0, h * 0.1, h * 0.1, y0 + h * 0.7, y0 + h * 0.92, slot, mat); e.p[0] = 0; e.p[1] = 0.1; }
    if (g.lod == 0) for (int s = -1; s <= 1; s += 2) { Form& a = g.box(lx + s * r * 0.75, lz, 0, r * 0.18, r * 0.2, y0 + h * 0.36, y0 + h * 0.68, slot, mat, 4); a.tilt = 0.05 * s; }
}

// A-04: the tops a people set on its roofs and towers at the given points (the building's frame): finials, a mast, a statue; crenels
// are the walls' (`walls4`), nothing is nothing
void topsAt(Gen& g, const std::vector<std::pair<double, double>>& pts, double y, double scale, int mat) {
    if (g.lod == 2) return;
    const int tp = g.gv(GV_TOPS); const double D = g.D(mat);
    if (D > 0.55 || tp == TP_CRENELS || tp == TP_NOTHING) return;
    const int slot = g.topSlot(mat), motif = g.gv(GV_MOTIF);
    for (const auto& p : pts) {
        switch (tp) {
            case TP_FINIALS:
                if (motif == MO_CIRCLES) { Form& e = g.add(FK_DOME, 0, p.first, p.second, 0, 0.32 * scale, 0.32 * scale, y, y + 0.32 * scale, slot, mat, 4); (void)e; }
                else { Form& e = g.lathe(2, p.first, p.second, 0.26 * scale, y, y + 1.6 * scale, slot, mat, motif == MO_STARS ? 4 : 0); e.p[0] = 1.2; }
                break;
            case TP_MASTS: { Form& e = g.lathe(0, p.first, p.second, 0.07 * scale, y, y + 4.5 * scale, g.timberSlot(), g.c.age >= AGE_INDUSTRIAL ? MT_IRON : MT_TIMBER, 4); (void)e; break; }
            default: figure(g, p.first, p.second, y, 1.9 * scale, slot, mat, D); break;
        }
    }
}

// four walls round a room: side 0 the front (+z'), 1 the right (+x'), 2 the back (-z'), 3 the left (-x'), each standing to its share of
// `h` by the material's decay (a wall over 8 m by the tall share, its top ragged); the doorway (1.8 m: a lintel over it and, where the
// wall is tall, the wall over the lintel) in the side `door` at `doorPos` along its axis; C-01's window in the wall opposite; the
// windows' pattern on them; at level 1 the walls whole, at level 2 one box. The heights each wall stands to are in `hs`.
// A-04: the people's profile (battered walls leaning in on a thicker foot, bulging ones with their rounded buttresses, an upper storey
// jettied out or set back over 4.5 m, a low wall's plinth) and its dress (`dress`: the bands, the cornice, the expressed frame, the
// door's surround, the glyphs round every side of a carved people's, the crenels along the tops)
struct Walls {
    double cx = 0, cz = 0, hw = 3, hd = 3, h = 3, thick = 0.5, y0 = 0;
    int door = 0; double doorPos = 0; bool window = false, glyphs = false;
    int mat = MT_MASONRY, slot = 0, pattern = 0, part = 0, k0 = 10;
    bool whole = false;   // standing whole whatever the decay (a storey that holds up what stands on it)
    int profile = -1;     // A-04: `WallProfile`, -1 the people's (`dress` false: upright)
    bool dress = true;    // A-04: the people's dress on them (a work's walls go bare)
    bool crenels = false; // A-04: merlons along the tops of the walls that stand whole (a flat roof's or a keep's)
    double parapet = 0;   // B-415: a parapet this high along the tops of the walls that stand whole (an industrial people's low roof behind it)
};
struct WallsOut { double hmin = 0, thick = 0.5, top = 0; };   // the lowest wall, the walls' thickness as built
WallsOut walls4x(Gen& g, const Walls& W0, double hs[4]) {
    Walls W = W0;
    if (W.slot == 0 && g.painted) W.slot = 1;   // R-412: a house painted in the accent
    const double Dm = g.D(W.mat);
    const bool tall = W.h > 8;
    const int prof = W.profile >= 0 ? W.profile : (W.dress ? g.gv(GV_PROFILE) : WP_UPRIGHT);
    double thick = W.thick;
    if (W.dress && g.gv(GV_EXPRESSION) == EX_MASSIVE) thick *= 1.35;
    if (prof == WP_BATTERED) thick = std::max(thick * 1.3, std::min(2.4, 0.12 * W.h));
    else if (prof == WP_BULGING) thick *= 1.6;
    WallsOut R; R.thick = thick;
    double hmin = 1e9, hsum = 0;
    for (int s = 0; s < 4; s++) { hs[s] = W.whole ? W.h : std::max(0.35, W.h * (tall ? tallShare(Dm, g.U(W.k0 + s)) : wallShare(Dm, g.U(W.k0 + s)))); hmin = std::min(hmin, hs[s]); hsum += hs[s]; }
    R.hmin = hmin;
    const double base = g.sink + W.y0;
    auto rag = [&](Form& e, double h) { if (tall && h < W.h * 0.98) e.broken = clampd(3.0 / std::max(h, 1.0), 0.08, 0.35); };
    if (g.lod == 2) { Form& e = g.box(W.cx, W.cz, 0, W.hw, W.hd, base, base + std::max(0.4, hsum * 0.25), W.slot, W.mat, W.part); e.pattern = W.pattern; return R; }
    const double ht = thick * 0.5;
    const bool tiered = (prof == WP_LEANING || prof == WP_STEPPED) && W.h > 4.5;
    const double tierY = tiered ? clampd(W.h * 0.5, 2.9, 4.2) : 1e9, jetty = prof == WP_LEANING ? 0.5 : -0.45;
    // the lean of each wall about its foot, inward positive: battered (on its thickened foot), a bulging wall's slight one, a low wall
    // leaning out
    const double lean = prof == WP_BATTERED ? std::atan(std::min(0.1 * W.h, 1.25 * ht) / std::max(1.0, W.h)) : (prof == WP_BULGING ? 0.035 : (prof == WP_LEANING && !tiered ? -0.06 : 0.0));
    const bool plinth = W.dress && prof == WP_STEPPED && !tiered;
    const int glyphAll = W.dress && (g.carved() || g.gv(GV_MOTIF) == MO_GLYPHS);
    for (int side = 0; side < 4; side++) {
        const bool along = side == 0 || side == 2;
        const double L = along ? W.hw : W.hd, h = hs[side];
        const double nx = side == 1 ? 1 : (side == 3 ? -1 : 0), nz = side == 0 ? 1 : (side == 2 ? -1 : 0);   // the outward normal
        const double lx = W.cx + nx * W.hw, lz = W.cz + nz * W.hd;
        const double dh = along ? 0 : PI / 2, ax = along ? 1 : 0, az = along ? 0 : -1;   // the wall's own axis in the building's frame
        const double leanS = side == W.door ? 0.0 : lean;   // T-02: the doorway's side upright (a leaning jamb's colliders closed the doorway)
        const double tiltOut = (side == 0 || side == 1) ? -leanS : leanS;   // a box's own +along points out on sides 0 and 1, in on 2 and 3
        const bool window = W.window && side == (W.door + 2) % 4 && h > 2.4 && Dm < 0.55;
        const bool glyphs = W.glyphs && g.lod == 0 && (side == 0 || glyphAll) && h > 1.5;
        const double pos = W.doorPos;
        // a run of this side's wall from ya to yb over the base, set `off` outward (its length grown or cut by as much, to meet its
        // neighbours set as far), the doorway and the window left in it where `openings`
        auto run = [&](double ya, double yb, double off, bool openings) {
            if (yb - ya < 0.05) return;
            const double Ls = L + off, ox = lx + nx * off, oz = lz + nz * off;
            auto B = [&](double t0, double t1, double y0r, double y1r, int part) -> Form& {
                const double sIn = std::tan(leanS) * y0r;   // a piece over the foot of a leaning wall sits where the wall is at its height
                Form& e = g.box(ox + ax * (t0 + t1) * 0.5 - nx * sIn, oz + az * (t0 + t1) * 0.5 - nz * sIn, dh, (t1 - t0) * 0.5, ht, base + y0r, base + y1r, W.slot, W.mat, part);
                e.tilt = tiltOut; return e;
            };
            if (!openings || g.lod == 1 || (side != W.door && !window)) { Form& e = B(-Ls, Ls, ya, yb, W.part); e.glyphs = glyphs; e.pattern = W.pattern; rag(e, h); return; }
            if (side == W.door) {   // the doorway: a gap of 1.8 m, a lintel over it where the jambs stand over 2.4 m; A-04: its head 2.1 m over the
                const double gap = 0.9, dt = 2.1 + std::max(0.0, -base);   // ground outside however deep the walls are sunk (a buried people's, a desert's sand)
                { Form& e = B(-Ls, pos - gap, ya, yb, W.part); e.glyphs = glyphs; e.pattern = W.pattern; rag(e, h); }
                { Form& e = B(pos + gap, Ls, ya, yb, W.part); e.pattern = W.pattern; rag(e, h); }
                if (yb > dt + 0.3) B(pos - gap - 0.3, pos + gap + 0.3, dt, std::min(yb, dt + 0.5), 4);
                if (yb > dt + 1.1) { Form& e = B(pos - gap - 0.3, pos + gap + 0.3, dt + 0.5, yb, W.part); e.pattern = W.pattern; rag(e, h); }
            } else {   // C-01's window: a sill under it and the wall over it
                const double gap = 0.55, wp = (L - 1.2) * (g.U(21) - 0.5);
                B(-Ls, wp - gap, ya, yb, W.part); B(wp + gap, Ls, ya, yb, W.part);
                B(wp - gap, wp + gap, ya, std::min(yb, 1.0), W.part);
                if (yb > 1.9) B(wp - gap, wp + gap, 1.9, yb, W.part);
            }
        };
        if (tiered) { run(0, std::min(h, tierY), 0, true); if (h > tierY + 0.3) run(tierY, h, jetty, false); }
        else run(0, h, 0, true);
        if (plinth && g.lod == 0) {   // a stepped people's low wall on a plinth a little wider (no higher than a step: no collider)
            const double ph = 0.85;
            auto P = [&](double t0, double t1) { if (t1 - t0 > 0.05) g.box(lx + ax * (t0 + t1) * 0.5, lz + az * (t0 + t1) * 0.5, dh, (t1 - t0) * 0.5, ht + 0.22, base, base + ph, W.slot, W.mat, 4); };
            if (side == W.door) { P(-L - 0.22, pos - 1.0); P(pos + 1.0, L + 0.22); } else P(-L - 0.22, L + 0.22);
        }
        // the dress: bands of the accent, a cornice, an expressed frame, crenels (each set off the wall by its own part: no collider)
        if (!W.dress) continue;
        const bool standing = h >= W.h * 0.97;
        const double topOff = tiered && h > tierY + 0.3 ? jetty : 0.0, Lt = L + topOff, tx = lx + nx * topOff, tz = lz + nz * topOff;
        const double topIn = std::tan(lean) * h;
        auto band = [&](double y0b, double y1b, double off, double proud, int slot, int mat) {   // a course along the side at a height
            const double sIn = std::tan(lean) * y0b;
            Form& e = g.box(lx + nx * (off - sIn), lz + nz * (off - sIn), dh, L + off + proud, ht + proud, base + y0b, base + y1b, slot, mat, 4); e.tilt = tiltOut;
        };
        if (g.lod <= 1 && g.banded()) {
            const double bw = g.accentOn(AP_BANDS) ? 0.45 : 0.28;
            if (standing) { const double y = h - 0.1 - bw; Form& e = g.box(tx - nx * topIn, tz - nz * topIn, dh, Lt + 0.07, ht + 0.07, base + y, base + y + bw, 1, W.mat, 4); e.tilt = tiltOut; }
            if (tiered && h > tierY + 0.3) band(tierY - bw, tierY, 0, 0.07, 1, W.mat);
            else if (g.accentOn(AP_BANDS) && h > 2.8) band(std::min(h, W.h) * 0.48, std::min(h, W.h) * 0.48 + bw, 0, 0.07, 1, W.mat);
        }
        if (g.lod <= 1 && g.carved() && standing) { Form& e = g.box(tx - nx * topIn, tz - nz * topIn, dh, Lt + 0.24, ht + 0.24, base + h - 0.05, base + h + 0.35, g.accentOn(AP_TOPS) ? 1 : W.slot, W.mat, 4); e.tilt = tiltOut; }
        if (g.gv(GV_EXPRESSION) == EX_FRAME && g.lod == 0 && (g.c.age == AGE_INDUSTRIAL || g.c.age == AGE_MODERN)) {
            // B-415: an industrial people's frame its brick piers between the windows (its iron posts stood off its walls like a ruined
            // pergola), a modern people's its concrete grid: the columns between the windows and a floor's slab at every storey. Each
            // piece of the wall lays its windows from its own end, so the piers stand at its bays' edges piece by piece
            const bool modern = g.c.age == AGE_MODERN;
            const int fm = modern ? MT_CONCRETE : W.mat, fs = W.slot;
            const double ragged = tall && h < W.h * 0.98 ? clampd(3.0 / std::max(h, 1.0), 0.08, 0.35) : 0.0;   // under a ruined wall's ragged top (they stood over it like merlons)
            const double Df = g.D(fm), fh = std::min(h * (1 - ragged), tiered ? tierY : h);
            const WindowSpec* ws = W.pattern > 0 && W.pattern <= g.c.palette.nWindows ? &g.c.palette.windows[W.pattern - 1] : nullptr;
            const double bay = ws ? std::max(2.0, ws->bay) : 3.0, storey = ws ? std::max(2.6, ws->storey) : 3.2;
            if (Df < 0.6 && fh > 1.0) {
                auto piers = [&](double t0, double t1) {   // at the piece's bays' edges and its far end
                    for (int k = 0;; k++) {
                        const bool last = t0 + k * bay > t1 - 0.4;
                        const double t = last ? t1 : t0 + k * bay;
                        if (!(side == W.door && std::fabs(t - pos) < 1.1)) { Form& e = g.box(lx + ax * t + nx * 0.07, lz + az * t + nz * 0.07, dh, modern ? 0.2 : 0.18, ht + (modern ? 0.08 : 0.12), base, base + fh, fs, fm, 4); e.tilt = tiltOut; }
                        if (last) break;
                    }
                };
                if (side == W.door) { piers(-L, pos - 0.9); piers(pos + 0.9, L); } else piers(-L, L);
                if (modern) for (double y = storey; y < fh - 0.6; y += storey) band(y - 0.35, y, 0.04, 0.06, fs, fm);   // the floors' slabs
            }
        } else if (g.gv(GV_EXPRESSION) == EX_FRAME && g.lod == 0) {   // posts at the corners and every two and a half metres, a beam along the top
            const int fm = g.c.age <= AGE_MEDIEVAL ? MT_TIMBER : MT_COMPOSITE;
            const double Df = g.D(fm);
            if (Df < 0.5) {
                const int fs = fm == MT_TIMBER || fm == MT_IRON ? g.timberSlot() : (fm == MT_COMPOSITE ? 1 : W.slot);
                const double fh = std::min(h, tiered ? tierY : h);
                const int n = std::max(1, (int)std::lround(2 * L / 2.5));
                for (int k = 0; k <= n; k++) {
                    double t = -L + 2 * L * k / n;
                    if (side == W.door && std::fabs(t - pos) < 1.25) continue;
                    if (window && std::fabs(t - (L - 1.2) * (g.U(21) - 0.5)) < 0.8) continue;
                    Form& e = g.box(lx + ax * t + nx * 0.06, lz + az * t + nz * 0.06, dh, 0.13, ht + 0.06, base, base + fh, fs, fm, 4); e.tilt = tiltOut;
                }
                if (fh > 2.9) band(std::min(fh, 3.1) - 0.25, std::min(fh, 3.1), 0.06, 0.0, fs, fm);
            }
        }
        if (W.parapet > 0 && standing && g.lod <= 1) { Form& e = g.box(tx - nx * topIn, tz - nz * topIn, dh, Lt, ht, base + h - 0.02, base + h + W.parapet, W.slot, W.mat, 4); e.tilt = tiltOut; }   // B-415
        if (W.crenels && standing && g.lod == 0) {   // merlons along the top
            const int n = std::max(2, (int)(2 * Lt / 1.6));
            for (int k = 0; k < n; k++) {
                double t = -Lt + (k + 0.5) * 2 * Lt / n;
                Form& e = g.box(tx + ax * t - nx * topIn, tz + az * t - nz * topIn, dh, Lt / n * 0.5, ht, base + h, base + h + 0.8 + (g.gv(GV_MOTIF) == MO_STEPS && k % 2 ? 0.4 : 0), g.topSlot(W.mat), W.mat);
                e.tilt = tiltOut;
            }
        }
    }
    if (W.dress && W.door >= 0 && g.lod == 0 && (g.accentOn(AP_DOORS) || g.carved())) {   // the door's surround: its jambs and its head (no collider: a trim)
        const bool along = W.door == 0 || W.door == 2;
        const double nx = W.door == 1 ? 1 : (W.door == 3 ? -1 : 0), nz = W.door == 0 ? 1 : (W.door == 2 ? -1 : 0);
        const double lx = W.cx + nx * W.hw, lz = W.cz + nz * W.hd, dh = along ? 0 : PI / 2, ax = along ? 1 : 0, az = along ? 0 : -1;
        if (hs[W.door] > 2.7) {
            const int ds = g.doorSlot();
            for (int s = -1; s <= 1; s += 2) g.box(lx + ax * (W.doorPos + s * 1.04) + nx * 0.05, lz + az * (W.doorPos + s * 1.04) + nz * 0.05, dh, 0.14, ht + 0.08, base, base + 2.3, ds, W.mat, 4);
            g.box(lx + ax * W.doorPos + nx * 0.08, lz + az * W.doorPos + nz * 0.08, dh, 1.25, ht + 0.12, base + 2.22, base + 2.62, ds, W.mat, 4);
        }
    }
    if (W.dress && prof == WP_BULGING) {   // rounded buttresses at the corners, over the walls' tops where they stand
        for (int q = 0; q < 4; q++) {
            const int sa = q < 2 ? 0 : 2, sb = q % 2 ? 1 : 3;
            const double hb = std::min(hs[sa], hs[sb]);
            const double x = W.cx + (q % 2 ? 1 : -1) * W.hw, z = W.cz + (q < 2 ? 1 : -1) * W.hd;
            Form& e = g.lathe(1, x, z, ht + 0.32, base, base + hb + (hb >= W.h * 0.97 ? 0.7 : 0), W.slot, W.mat, g.lod == 0 ? 0 : 6); e.p[0] = 0.55;
        }
    }
    R.top = base;
    return R;
}
double walls4(Gen& g, const Walls& W, double hs[4]) { return walls4x(g, W, hs).hmin; }

// A-04: a ring of wall round an ellipse of radii `rx` (across) and `rz` (along) in `segs` straight segments (a circle, the people's
// polygon, an ellipse), the doorway's segments left out from `door`; each segment a box along its chord, its own +along outward (C-01's
// rotunda and A-03's huts stood their segments across the ring as fins: A-04 lays them along it). The profile as `walls4`'s; the bands
// and the cornice segment by segment; the heights each segment stands to in `hs` (empty where left out)
struct Ring {
    double cx = 0, cz = 0, rx = 3, rz = 3, h = 3, thick = 0.5, y0 = 0;
    int segs = 12, door = 0, doorSegs = 1;
    int mat = MT_MASONRY, slot = 0, pattern = 0, part = 0, k0 = 10;
    bool whole = false, dress = true, crenels = false, open = false;   // `open`: no doorway (a ring of a tower, a drum)
    int profile = -1;
};
bool ringInDoor(const Ring& R, int k) { if (R.open) return false; int d = ((k - R.door) % R.segs + R.segs) % R.segs; return d < R.doorSegs; }
double ringWalls(Gen& g, const Ring& R0) {
    Ring R = R0;
    if (R.slot == 0 && g.painted) R.slot = 1;   // R-412: a house painted in the accent
    const double Dm = g.D(R.mat);
    const int prof = R.profile >= 0 ? R.profile : (R.dress ? g.gv(GV_PROFILE) : WP_UPRIGHT);
    double thick = R.thick;
    if (R.dress && g.gv(GV_EXPRESSION) == EX_MASSIVE) thick *= 1.35;
    if (prof == WP_BATTERED) thick = std::max(thick * 1.3, std::min(2.0, 0.12 * R.h));
    else if (prof == WP_BULGING) thick *= 1.6;
    const double ht = thick * 0.5, base = g.sink + R.y0;
    if (g.lod == 2) {
        Form& e = g.add(FK_LATHE, 0, R.cx, R.cz, 0, R.rx, R.rz, base, base + R.h * (R.whole ? 1 : wallShare(Dm, 0.5)), R.slot, R.mat, R.part);
        e.sides = R.segs <= 8 ? R.segs : 0; e.pattern = R.pattern;
        return R.whole ? R.h : R.h * wallShare(Dm, 0.5);
    }
    const bool tiered = (prof == WP_LEANING || prof == WP_STEPPED) && R.h > 4.5;
    const double tierY = tiered ? clampd(R.h * 0.5, 2.9, 4.2) : 1e9, jetty = prof == WP_LEANING ? 0.5 : -0.45;
    const double lean = prof == WP_BATTERED ? std::atan(std::min(0.1 * R.h, 1.25 * ht) / std::max(1.0, R.h)) : (prof == WP_BULGING ? 0.05 : (prof == WP_LEANING && !tiered ? -0.06 : 0.0));
    const int glyphAll = R.dress && (g.carved() || g.gv(GV_MOTIF) == MO_GLYPHS);
    double hmin = 1e9;
    const int N = R.segs;
    for (int k = 0; k < N; k++) {
        if (ringInDoor(R, k) && g.lod == 0) continue;
        const double a0 = k * TAU / N, a1 = (k + 1) * TAU / N;
        const double x0 = std::sin(a0) * R.rx, z0 = std::cos(a0) * R.rz, x1 = std::sin(a1) * R.rx, z1 = std::cos(a1) * R.rz;
        const double dx = x1 - x0, dz = z1 - z0, chord = std::sqrt(dx * dx + dz * dz), ux = dx / chord, uz = dz / chord;
        const double nx = -uz, nz = ux;   // the chord's normal; outward for a ring laid clockwise
        const double dh = std::atan2(-dz, dx);   // the box's across axis along the chord, its +along (nx, nz) outward
        const double mx = R.cx + 0.5 * (x0 + x1), mz = R.cz + 0.5 * (z0 + z1);
        const double h = R.whole ? R.h : std::max(0.35, R.h * wallShare(Dm, g.U(R.k0 + k)));
        hmin = std::min(hmin, h);
        const double rMid = std::sqrt(mx * mx + mz * mz) + 1e-6;
        // each segment overlaps its neighbours at the joints, but not into the doorway's gap (its width is the chord's)
        const double ovA = g.lod == 0 && ringInDoor(R, (k + N - 1) % N) ? 0 : ht * 0.6, ovB = g.lod == 0 && ringInDoor(R, (k + 1) % N) ? 0 : ht * 0.6, shift = 0.5 * (ovB - ovA);
        auto seg = [&](double ya, double yb, double off, int part) -> Form& {
            const double sIn = std::tan(lean) * ya, grow = 1 + off / rMid;
            Form& e = g.box(mx + ux * shift + nx * (off - sIn), mz + uz * shift + nz * (off - sIn), dh, (chord * 0.5 + 0.5 * (ovA + ovB)) * grow, ht, base + ya, base + yb, R.slot, R.mat, part);
            e.tilt = -lean; e.pattern = R.pattern; return e;
        };
        { Form& e = seg(0, std::min(h, tierY), 0, R.part); e.glyphs = g.lod == 0 && glyphAll && h > 1.5 && k % 3 == 0; }
        if (tiered && h > tierY + 0.3) seg(tierY, h, jetty, R.part);
        if (!R.dress || g.lod > 1) continue;
        const bool standing = h >= R.h * 0.97;
        const double topOff = tiered && h > tierY + 0.3 ? jetty : 0.0;
        if (standing && g.banded()) { const double bw = g.accentOn(AP_BANDS) ? 0.45 : 0.28; Form& e = seg(h - 0.1 - bw, h - 0.1, topOff + 0.07, 4); e.hz = ht + 0.07; e.slot = 1; e.pattern = 0; }
        if (standing && g.carved()) { Form& e = seg(h - 0.05, h + 0.35, topOff + 0.2, 4); e.hz = ht + 0.2; e.slot = g.accentOn(AP_TOPS) ? 1 : R.slot; e.pattern = 0; }
        if (standing && R.crenels && g.lod == 0 && k % 2 == 0) { Form& e = seg(h, h + 0.8, topOff, 0); e.hx *= 0.55; e.pattern = 0; e.slot = g.topSlot(R.mat); }
        if (g.gv(GV_EXPRESSION) == EX_FRAME && g.lod == 0 && !ringInDoor(R, k) && !ringInDoor(R, (k + N - 1) % N)) {   // a post at the segment's joint
            const int fm = g.c.age <= AGE_MEDIEVAL ? MT_TIMBER : (g.c.age == AGE_INDUSTRIAL ? R.mat : (g.c.age == AGE_MODERN ? MT_CONCRETE : MT_COMPOSITE));   // B-415: a brick pier
            if (g.D(fm) < 0.5) { Form& e = g.lathe(0, R.cx + x0 + std::sin(a0) * 0.08, R.cz + z0 + std::cos(a0) * 0.08, ht + 0.1, base, base + std::min(h, tierY), fm == MT_TIMBER || fm == MT_IRON ? g.timberSlot() : (fm == MT_COMPOSITE ? 1 : R.slot), fm, 6); e.part = 4; }
        }
    }
    return hmin;
}

// a roof over walls that all stand to at least `hmin` of their `h` (a roof never floats: none where a wall fell under 92%), the people's
// `RoofForm`: a flat slab (C-01's stone roof: whole on a sound building, half on a worn one), a gable along the longer side, hipped, a
// dome over the room, a tall pyramid (conical), a stone barrel vault, a saddle's shell, a stepped crown, open (a parapet round the
// sky); of `mat` (stone or the roofing's timber, tiles, slate, sheet), whole, then holed, then gone
double roof(Gen& g, int form, double cx, double cz, double hw, double hd, double hmin, double h, double pitch, int mat) {   // returns the crown's rise over the walls (0: none)
    if (hmin < 0.92 * h) return 0;
    const int slot = g.roofSlot(mat);
    const double Dr = g.D(mat), y0 = g.sink + hmin - 0.05;
    const double eave = g.eave(), ps = pitch * (0.8 + 0.4 * g.gf(GV_PROPORTION)) * g.pitchK();   // a slender people's roofs steeper; R-412: its pitch, its eaves
    if (form == RF_FLAT || form == RF_OPEN) {
        if (form == RF_OPEN) {   // a parapet round an open top
            if (g.lod == 0 && Dr < 0.5) for (int s = 0; s < 4; s++) { bool al = s % 2 == 0; g.box(cx + (s == 1 ? hw : (s == 3 ? -hw : 0)), cz + (s == 0 ? hd : (s == 2 ? -hd : 0)), al ? 0 : PI / 2, al ? hw : hd, 0.18, y0, y0 + 0.7, structural(mat) ? 0 : slot, mat, 4); }
            return 0;
        }
        if (structural(mat)) {   // R-412: a modern people's slab flush, a lip, deep, or a cornice
            const double se = g.slabEave(), th = g.c.age == AGE_MODERN && g.c.genome.of(GV_EAVES) == EV_FLARED ? 0.8 : 0.35;
            if (Dr < 0.35) g.box(cx, cz, 0, hw + se, hd + se, y0, y0 + th, slot, mat, 2);
            else if (Dr < 0.6) g.box(cx, cz - hd * 0.5, 0, hw + se, hd * 0.5 + se, y0, y0 + th, slot, mat, 2);
        } else if (Dr < 0.4) { Form& e = g.box(cx, cz, 0, hw + 0.3, hd + 0.3, y0, y0 + 0.3, slot, mat, 2); e.broken = Dr < 0.2 ? 0 : 0.4; }
        return 0;
    }
    if (Dr >= 0.55) return 0;
    const double broken = Dr < 0.25 ? 0 : clampd((Dr - 0.25) / 0.3, 0.2, 0.9);
    double top = y0;
    switch (form) {
        case RF_PITCHED: {   // a gable: a section swept along the longer side, the eaves over the walls
            const bool alongZ = hd >= hw; const double span = (alongZ ? hw : hd) + eave, len = (alongZ ? hd : hw) + eave * 0.6;
            Form& e = g.add(FK_SWEEP, 1, cx, cz, alongZ ? 0 : PI / 2, 0, len, y0, y0 + span * ps, slot, mat, 2);
            e.p[1] = span; e.p[2] = 0.12; e.broken = broken; top = e.y1;
            if (g.flared() && g.lod < 2) { Form& k = g.add(FK_SWEEP, 1, cx, cz, alongZ ? 0 : PI / 2, 0, len, y0 - 0.3, y0 + 0.3, slot, mat, 2); k.p[1] = span + 0.5; k.p[2] = std::max(0.2, span - 0.3); k.broken = broken; }   // R-412: its bellcast eaves
            break;
        }
        case RF_HIPPED: case RF_CONICAL: {
            const bool hip = form == RF_HIPPED;
            Form& e = g.add(FK_PYRAMID, 0, cx, cz, 0, hw + eave, hd + eave, y0, y0 + (hip ? std::min(hw, hd) : std::max(hw, hd) * 1.9) * ps, slot, mat, 2); e.p[1] = hip ? 0.28 : 0; e.broken = broken; top = e.y1;
            if (g.flared() && g.lod < 2) { Form& k = g.add(FK_PYRAMID, 0, cx, cz, 0, hw + eave + 0.5, hd + eave + 0.5, y0 - 0.3, y0 + 0.3, slot, mat, 2); k.p[1] = 0.72; k.broken = broken; }   // R-412: its bellcast eaves
            break;
        }
        case RF_VAULTED: {   // a barrel vault along the longer side, springing from the walls' top
            const bool alongZ = hd >= hw; const double span = (alongZ ? hw : hd) + 0.25, len = alongZ ? hd : hw;
            Form& e = g.add(FK_VAULT, g.gv(GV_OPENING) == OP_ARCHED || g.gv(GV_OPENING) == OP_TRIANGULAR ? 1 : 0, cx, cz, alongZ ? 0 : PI / 2, span, len + 0.25, y0, y0 + span * 0.85, slot, mat, 2);
            e.p[0] = 0.5; e.p[1] = 1; e.broken = Dr < 0.3 ? 0 : clampd(Dr, 0.3, 0.9); top = e.y1;
            break;
        }
        case RF_DOMED: {   // a dome on the room, a flat roof round it where the room is not square
            const double r = std::min(hw, hd) + 0.2;
            if (std::fabs(hw - hd) > 0.6 && structural(mat)) g.box(cx, cz, 0, hw + 0.2, hd + 0.2, y0, y0 + 0.3, structural(mat) ? 0 : slot, mat, 2);
            const int prof = ageDome(g, g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : (g.gv(GV_OPENING) == OP_ARCHED || g.gv(GV_OPENING) == OP_TRIANGULAR ? 2 : (g.c.age >= AGE_INDUSTRIAL ? 1 : 0)));
            Form& e = g.add(FK_DOME, prof, cx, cz, 0, r, r, y0 + 0.25, y0 + 0.25 + r * (prof == 1 ? 0.55 : (prof == 0 ? 1.0 : 1.35)) * (0.85 + 0.3 * g.gf(GV_PROPORTION)), slot, mat, 2);
            e.broken = Dr < 0.3 ? 0 : clampd(Dr, 0.3, 0.9); top = e.y1;
            break;
        }
        case RF_SADDLE: {   // a saddle's shell over the room, its high corners on the front and the back
            Form& e = g.add(FK_SHELL, 0, cx, cz, 0, hw + eave, hd + eave, y0, y0 + std::max(hw, hd) * ps * 0.9, slot, mat, 2);
            e.broken = broken; top = e.y1;
            break;
        }
        case RF_STEPPED: { Form& e = g.add(FK_PYRAMID, 0, cx, cz, 0, hw + 0.25, hd + 0.25, y0, y0 + std::min(hw, hd) * ps * 1.1, slot, mat, 2); e.p[0] = 3; e.p[1] = 0.3; e.p[2] = 1; e.broken = broken; top = e.y1; break; }
        default: { Form& e = g.lathe(1, cx, cz, hw + eave, y0, y0 + hw * ps * 1.3, slot, mat); e.p[0] = 0.04; e.broken = broken; top = e.y1; break; }   // a cone (the round roofs' and A-03's form 4)
    }
    return top - (g.sink + hmin);
}
// the stone or the roofing a people roofs with in an age: stone where the form holds itself up (a slab, a vault, a dome, a stepped
// crown) and the people builds to last or the age has no timber left to roof with; the roofing's sheet otherwise
int roofMaterial(const Gen& g, int form, int stone) {
    const bool selfHolding = form == RF_FLAT || form == RF_VAULTED || form == RF_DOMED || form == RF_STEPPED || form == RF_OPEN;
    switch (g.c.age) {
        case AGE_PREHISTORIC: return form == RF_DOMED ? MT_DRYSTONE : MT_TIMBER;
        case AGE_MEDIEVAL: return selfHolding ? stone : MT_TIMBER;
        case AGE_INDUSTRIAL: return selfHolding ? stone : MT_TIMBER;
        case AGE_MODERN: return selfHolding || form == RF_SADDLE ? MT_CONCRETE : MT_IRON;
        default: return MT_COMPOSITE;
    }
}
// B-415: a pitched roof's gable ends in its walls' own material (stone, brick, adobe): each stands on its wall while the wall stands whole,
// whatever became of the roof, under the roof's slope (a roofless row keeps its gables, a stone house its ends: the age's roofs on a ruin);
// `W` the walls under a roof of `pitch` whose ridge runs along their longer side (across it where `across`: a row's gables to the street)
// R-412: the gable's end as the people shaped it (a medieval or an industrial people's): plain, its crow steps climbing over the roof's
// slope, a curved neck rising over the ridge under its cap, a pediment's cornice across its foot and its raking frame, or its top clipped
// by a little hip of the roofing
void gables(Gen& g, const Walls& W, const double hs[4], double pitch, bool across = false) {
    if (g.lod > 1 || g.c.age < AGE_MEDIEVAL || !structural(W.mat)) return;
    const bool alongZ = (W.hd >= W.hw) != across;   // the ridge's run
    const double ps = pitch * (0.8 + 0.4 * g.gf(GV_PROPORTION)) * g.pitchK(), half = alongZ ? W.hw : W.hd, ht = W.thick * 0.5;
    const int gb = g.c.age <= AGE_INDUSTRIAL ? g.gv(GV_GABLES) : GB_PLAIN, slot = W.slot == 0 && g.painted ? 1 : W.slot;
    const double dh = alongZ ? 0 : PI / 2, rise = half * ps;
    for (int s = 0; s < 4; s++) {
        if ((s == 0 || s == 2) != alongZ || hs[s] < 0.97 * W.h) continue;
        const double nx = s == 1 ? 1 : (s == 3 ? -1 : 0), nz = s == 0 ? 1 : (s == 2 ? -1 : 0), y0 = g.sink + W.y0 + hs[s] - 0.03;
        const double gx = W.cx + nx * W.hw, gz = W.cz + nz * W.hd;
        if (gb == GB_STEPPED && g.lod == 0 && rise > 1.0) {   // its crow steps: each a course narrower, over the slope
            const int n = clampi((int)std::lround(rise / 0.75), 3, 6);
            for (int q = 0; q < n; q++) g.add(FK_BOX, 0, gx, gz, dh, (half + ht) * (1 - (double)q / n) - 0.04, ht, y0 + rise * q / n, y0 + rise * (q + 1) / n + 0.32, slot, W.mat);
            continue;
        }
        const bool clipped = gb == GB_HALF_HIPPED && rise > 1.2;
        Form& e = g.add(FK_SWEEP, 1, gx, gz, dh, 0, ht, y0, y0 + rise * (clipped ? 0.66 : 1.0), slot, W.mat);
        e.p[1] = half + ht; e.p[2] = clipped ? std::max(0.03, (half + ht) * 0.34) : 0.03;
        if (g.lod != 0) continue;
        if (gb == GB_CURVED && rise > 1.2) {   // its neck over the ridge, its cap
            const double nw = std::max(0.5, half * 0.3);
            g.add(FK_BOX, 0, gx, gz, dh, nw, ht, y0 + rise * 0.5, y0 + rise + 0.6, slot, W.mat);
            g.add(FK_DOME, 1, gx, gz, dh, nw, ht + 0.02, y0 + rise + 0.6, y0 + rise + 0.6 + nw * 0.7, slot, W.mat, 4);
            for (int k = -1; k <= 1; k += 2) { Form& sc = g.add(FK_DOME, 0, gx + (alongZ ? k * (nw + 0.2) : 0), gz + (alongZ ? 0 : k * (nw + 0.2)), dh, 0.35, ht + 0.02, y0 + rise * 0.5, y0 + rise * 0.5 + 0.35, slot, W.mat, 4); (void)sc; }   // its scrolls
        } else if (gb == GB_PEDIMENT) {   // its cornice across its foot and its raking frame
            { Form& c = g.add(FK_BOX, 0, gx + nx * 0.06, gz + nz * 0.06, dh, half + ht + 0.22, ht + 0.12, y0 - 0.22, y0 + 0.08, slot, W.mat, 4); c.tone = g.tone + 6; }
            { Form& f = g.add(FK_SWEEP, 1, gx + nx * 0.05, gz + nz * 0.05, dh, 0, ht + 0.06, y0 + 0.08, y0 + 0.08 + (half + 0.22) * ps + 0.22, slot, W.mat, 4); f.p[1] = half + ht + 0.22; f.p[2] = 0.05; f.tone = g.tone + 6; }
        } else if (clipped && g.D(MT_TIMBER) < 0.4) {   // the little hip over its clipped top
            Form& h = g.add(FK_PYRAMID, 0, gx + nx * 0.3, gz + nz * 0.3, dh, (half + ht) * 0.36, 0.75, y0 + rise * 0.64, y0 + rise * 1.0, g.roofSlot(MT_TIMBER), MT_TIMBER, 2);
            h.p[1] = 0.05;
        }
    }
}
// a round roof over a ring (`rx` across, `rz` along): a cone (pitched, hipped, conical), a dome (domed, vaulted), a disc (flat), a
// saddle's shell, a stepped cone of `segs` sides, nothing (open)
double roundRoof(Gen& g, int form, double cx, double cz, double rx, double rz, int segs, double hmin, double h, double pitch, int mat) {   // returns the crown's rise (0: none)
    if (hmin < 0.92 * h) return 0;
    const int slot = g.roofSlot(mat);
    const double Dr = g.D(mat), y0 = g.sink + hmin - 0.1, ps = pitch * (0.8 + 0.4 * g.gf(GV_PROPORTION)) * g.pitchK(), r = std::max(rx, rz), ev = g.eave() + 0.05;   // R-412
    if (form == RF_OPEN) return 0;
    if (form == RF_FLAT) { if (Dr < 0.45) { Form& e = g.add(FK_LATHE, 0, cx, cz, 0, rx + g.slabEave(), rz + g.slabEave(), y0, y0 + 0.35, slot, mat, 2); e.sides = segs <= 8 ? segs : 0; e.broken = Dr < 0.25 ? 0 : 0.5; return 0.25; } return 0; }
    if (Dr >= 0.6) return 0;
    const double broken = Dr < 0.25 ? 0 : clampd((Dr - 0.25) / 0.35, 0.2, 0.9);
    if (form == RF_DOMED || form == RF_VAULTED) {
        const int prof = ageDome(g, g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : (g.gv(GV_OPENING) == OP_ARCHED || g.gv(GV_OPENING) == OP_TRIANGULAR ? 2 : (g.c.age >= AGE_MODERN ? 1 : 0)));
        Form& e = g.add(FK_DOME, prof, cx, cz, 0, rx + 0.1, rz + 0.1, y0, y0 + r * (prof == 1 ? 0.55 : (prof == 0 ? 1.0 : 1.35)) * (0.85 + 0.3 * g.gf(GV_PROPORTION)), slot, mat, 2);
        e.broken = Dr < 0.3 ? 0 : clampd(Dr, 0.3, 0.9);
        return e.y1 - (g.sink + hmin);
    }
    if (form == RF_SADDLE) { Form& e = g.add(FK_SHELL, 0, cx, cz, 0, rx + ev - 0.1, rz + ev - 0.1, y0, y0 + r * ps * 0.9, slot, mat, 2); e.broken = broken; return e.y1 - (g.sink + hmin); }
    if (form == RF_STEPPED) { Form& e = g.add(FK_PYRAMID, 0, cx, cz, 0, rx + 0.3, rz + 0.3, y0, y0 + r * ps * 1.2, slot, mat, 2); e.sides = clampi(segs, 3, 8); e.p[0] = 3; e.p[1] = 0.25; e.p[2] = 1; e.broken = broken; return e.y1 - (g.sink + hmin); }
    Form& e = g.add(FK_LATHE, 1, cx, cz, 0, rx + ev, rz + ev, y0, y0 + r * ps * (form == RF_CONICAL ? 1.9 : 1.3), slot, mat, 2);
    e.sides = segs <= 8 && form != RF_PITCHED ? segs : 0; e.p[0] = 0.04; e.broken = broken;
    if (g.flared() && g.lod < 2) { Form& k = g.add(FK_LATHE, 1, cx, cz, 0, rx + ev + 0.5, rz + ev + 0.5, y0 - 0.3, y0 + 0.3, slot, mat, 2); k.sides = e.sides; k.p[0] = 0.72; k.broken = broken; }   // R-412: its bellcast eaves
    return e.y1 - (g.sink + hmin);
}

// A-04: a tower's shaft of radius `r` to `H` at (tx, tz) in the people's section (round, its polygon, square, oval) and profile
// (battered: a frustum; bulging: a barrel; leaning out: widening to a corbelled top; stepped back: tiers), snapped by the material's
// tall share, its top lying where it fell; its windows `pattern`. Returns the height it stands to (`whole` with it standing whole)
struct TowerOut { double h = 0; bool whole = false; double rTop = 0; };
TowerOut towerShaft(Gen& g, double tx, double tz, double r, double H, int mat, int k, int pattern, bool fallTop = true) {
    TowerOut T;
    const double Dm = g.D(mat), h = H * tallShare(Dm, g.U(k));
    T.h = h; T.whole = h >= H * 0.97;
    const int plan = g.gv(GV_PLAN), prof = g.gv(GV_PROFILE);
    const int sides = plan == PF_POLYGON ? clampi(g.c.genome.number(), 3, 8) : (plan == PF_CIRCLE || plan == PF_ELLIPSE ? 0 : 4);
    const double rz = plan == PF_ELLIPSE ? r * 0.75 : r;
    auto shaft = [&](int sub, double ra, double y0, double y1, double p0, double p1) -> Form& {
        Form& e = g.add(FK_LATHE, sub, tx, tz, 0, ra, ra * rz / r, g.sink + y0, g.sink + y1, 0, mat); e.sides = sides; e.p[0] = p0; e.p[1] = p1; e.pattern = pattern; return e;
    };
    const double brk = T.whole ? 0 : 0.3;
    switch (prof) {
        case WP_BATTERED: { Form& e = shaft(1, r * 1.12, 0, h, 1 - 0.3 * h / H, 0); e.broken = brk; T.rTop = r * 1.12 * (1 - 0.3 * h / H); break; }
        case WP_BULGING: { Form& e = shaft(3, r, 0, h, 1 + 0.14 * std::min(1.0, h / H), 0.5 * H / std::max(h, 1.0)); e.broken = brk; T.rTop = r; break; }
        case WP_LEANING: { Form& e = shaft(1, r * 0.9, 0, h, 1 + 0.28 * h / H, 0); e.broken = brk; T.rTop = r * 0.9 * (1 + 0.28 * h / H); break; }
        case WP_STEPPED: {   // tiers, each set back
            const int tiers = 3; double ra = r;
            for (int i = 0; i < tiers; i++) {
                const double ya = H * i / tiers, yb = H * (i + 1) / tiers;
                if (h <= ya + 0.2) break;
                Form& e = shaft(0, ra, ya, std::min(h, yb), 0, 0); e.broken = h < yb ? brk : 0; T.rTop = ra;
                ra *= 0.8;
            }
            break;
        }
        default: { Form& e = shaft(0, r, 0, h, 0, 0); e.broken = brk; T.rTop = r; break; }
    }
    if (!T.whole && fallTop && g.lod == 0) {
        const double dir = std::atan2(tx, tz + 0.01) + g.U(k + 1) - 0.5;
        g.fallen(FK_LATHE, 0, tx, tz, dir, r * 0.8, (H - h) * 0.45, 0, mat, sides);
        g.heap(tx, tz, r * 1.7, r * 1.7, std::min(4.0, (H - h) * 0.07), 0, mat);
    }
    return T;
}
// a tower's top where its shaft stands whole: a spire (`spire`, or the people's pitched and conical roofs), a dome, crenels, a flat top,
// and the people's tops on it
// B-415: by the age: a modern or a later people's concrete or composite shaft topped with a needle, a saucer or a mast on its flat top; an
// industrial people's tower without crenels, a slate pyramid on it where it raised no spire or dome
void towerTop(Gen& g, double tx, double tz, double r, double h, double H, int mat, bool spire, double spireH) {
    const int roofF = g.c.roof, tp = g.gv(GV_TOPS);
    const int plan = g.gv(GV_PLAN), sides = plan == PF_POLYGON ? clampi(g.c.genome.number(), 3, 8) : (plan == PF_CIRCLE || plan == PF_ELLIPSE ? 0 : 4);
    double yTop = g.sink + h;   // where the tops stand: the spire's tip, the dome's crown, the shaft's top
    int topMat = mat;
    (void)H;
    if (g.c.age >= AGE_MODERN) {
        const double Dm = g.D(mat);
        if (Dm < 0.5) {
            if (spire || roofF == RF_PITCHED || roofF == RF_CONICAL || roofF == RF_HIPPED) { Form& s = g.lathe(2, tx, tz, r * 0.32, yTop, yTop + std::max(spireH * 0.8, r * 5), g.topSlot(mat), mat, sides >= 3 ? sides : 0); s.p[0] = 1.6; yTop += std::max(spireH * 0.8, r * 5); }
            else if (roofF == RF_DOMED || roofF == RF_VAULTED) { g.add(FK_DOME, 1, tx, tz, 0, r * 1.15, r * 1.15, yTop, yTop + r * 0.45, g.topSlot(mat), mat, 2); yTop += r * 0.45; }
            else if (g.lod < 2) { Form& m = g.add(FK_TRUSS, 0, tx, tz, 0, std::max(0.6, r * 0.18), std::max(0.6, r * 0.18), yTop, yTop + std::max(6.0, r * 3), g.ironSlot(), MT_IRON); m.sides = 3; m.p[0] = 0.3; m.broken = g.D(MT_IRON) < 0.3 ? 0 : 0.6; }
        }
        topsAt(g, {{tx, tz}}, yTop, std::max(1.0, r / 2.5), topMat);
        return;
    }
    if (spire || roofF == RF_PITCHED || roofF == RF_CONICAL || roofF == RF_HIPPED) {
        const int sm = g.c.age <= AGE_INDUSTRIAL && !structural(roofMaterial(g, RF_PITCHED, mat)) ? MT_TIMBER : mat;
        const double Ds = g.D(sm), sH = spire ? spireH : r * 2.6;
        if (Ds < 0.5) { Form& s = g.lathe(2, tx, tz, r * 1.04, g.sink + h, g.sink + h + sH, g.topSlot(sm), sm, sides >= 3 ? std::max(sides, 4) : 8); s.p[0] = spire ? 1.0 : 0.8; s.broken = Ds < 0.25 ? 0 : 0.5; if (Ds < 0.25) { yTop += sH; topMat = sm; } }
        if (g.gv(GV_MOTIF) == MO_SPIRALS && g.lod < 2) { Form& c = g.add(FK_TORUS, 0, tx, tz, 0, r * 1.05, r * 1.05, g.sink + h - 1.0, g.sink + h - 0.4, 1, mat); c.p[0] = 0.3; }
    } else if (roofF == RF_DOMED || roofF == RF_VAULTED) {
        const double Dd = g.D(mat);
        if (Dd < 0.55) { Form& d = g.add(FK_DOME, ageDome(g, g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : 0), tx, tz, 0, r, r, g.sink + h, g.sink + h + r * 1.1, g.topSlot(mat), mat, 2); d.broken = Dd < 0.3 ? 0 : 0.5; if (Dd < 0.3) yTop += r * 1.1; }
    } else if (g.c.age == AGE_INDUSTRIAL) {   // a slate pyramid
        const double Dt = g.D(MT_TIMBER);
        if (Dt < 0.5) { Form& p = g.lathe(1, tx, tz, r * 1.08, yTop, yTop + r * 1.5, g.roofSlot(MT_TIMBER), MT_TIMBER, sides >= 3 ? sides : 0); p.p[0] = 0.04; p.broken = Dt < 0.25 ? 0 : 0.5; if (Dt < 0.25) { yTop += r * 1.5; topMat = MT_TIMBER; } }
    } else if (g.lod == 0 && (tp == TP_CRENELS || roofF == RF_STEPPED)) {   // merlons round the top
        const int n = std::max(6, (int)(TAU * r / 1.4));
        for (int k = 0; k < n; k += 2) { double a = k * TAU / n; g.box(tx + std::sin(a) * r * 0.92, tz + std::cos(a) * r * 0.92, a, r * PI / n, 0.3, g.sink + h, g.sink + h + 0.9, g.topSlot(mat), mat); }
    }
    topsAt(g, {{tx, tz}}, yTop, std::max(1.0, r / 2.5), topMat);
}

// A-04: what a building stands on (`Building::found`): stilts (posts under a floor, the walls on it: drawn by the generator, which
// lifts them), a mound, a pit it is sunk in (the generator sinks it), a cut in the rock round it (a rim of the walls' stone, its gap at the
// doorway), a terrace's kerb. `rx`, `rz` the footprint's half extents. B-415: a mound's earth bank and a pit's rim
// are gone: each building stood in a ring of the ground's own grass or sand as wide as its longest side, over its neighbours and the street
// (the user: "round green things around each building [...] they overlap with each other and other buildings a lot"); a people of mounds
// shows it in where it built its towns (`plan.cpp`), a people of pits in its sunk floors
void foundation(Gen& g, double rx, double rz, int doorSide, double doorPos, int mat) {
    if (g.lod != 0) return;
    switch (g.b.found) {
        case FD_CUT: {   // the rock's rim, 1.6 m out, a gap of 2.6 m at the doorway
            const double o = 1.6, H = 1.4 * wallShare(g.D(mat), g.U(600));
            for (int s = 0; s < 4; s++) {
                const bool al = s == 0 || s == 2; const double L = (al ? rx : rz) + o, nx = s == 1 ? 1 : (s == 3 ? -1 : 0), nz = s == 0 ? 1 : (s == 2 ? -1 : 0);
                const double lx = nx * (rx + o), lz = nz * (rz + o), ax = al ? 1 : 0, az = al ? 0 : -1;
                auto P = [&](double t0, double t1) { if (t1 - t0 > 0.2) g.box(lx + ax * (t0 + t1) * 0.5, lz + az * (t0 + t1) * 0.5, al ? 0 : PI / 2, (t1 - t0) * 0.5, 0.45, 0, std::max(0.4, H), 0, mat); };
                if (s == doorSide) { P(-L, doorPos - 1.3); P(doorPos + 1.3, L); } else P(-L, L);
            }
            break;
        }
        case FD_TERRACE: {   // a kerb of stone a step high round it
            const double o = 1.1;
            for (int s = 0; s < 4; s++) {
                const bool al = s == 0 || s == 2; const double L = (al ? rx : rz) + o;
                g.box((s == 1 ? 1 : (s == 3 ? -1 : 0)) * (rx + o), (s == 0 ? 1 : (s == 2 ? -1 : 0)) * (rz + o), al ? 0 : PI / 2, L, 0.3, 0, 0.7, 0, mat, 4);
            }
            break;
        }
        default: break;
    }
}
// stilts under a floor at `fy`: posts at the corners and along the sides (timber, or the age's own), the floor on them while every post
// stands; false when the posts are gone (the house lies on the ground)
bool stilts(Gen& g, double rx, double rz, double fy) {
    const int m = g.c.age >= AGE_INDUSTRIAL ? (g.c.age == AGE_INDUSTRIAL ? MT_IRON : (g.c.age == AGE_MODERN ? MT_CONCRETE : MT_COMPOSITE)) : (g.c.age == AGE_MEDIEVAL && g.c.hut == 1 ? MT_MASONRY : MT_TIMBER);
    const int slot = m == MT_TIMBER || m == MT_IRON ? g.timberSlot() : 0;
    const double D = g.D(m), r = m == MT_TIMBER ? 0.16 : 0.3;
    const bool standing = D < 0.45;
    const int nx = std::max(2, (int)std::lround(rx / 2.2) + 1), nz = std::max(2, (int)std::lround(rz / 2.2) + 1);
    for (int i = 0; i < nx; i++) for (int j = 0; j < nz; j++) {
        if (i != 0 && i != nx - 1 && j != 0 && j != nz - 1) continue;
        const double x = -rx * 0.85 + 1.7 * rx * i / (nx - 1), z = -rz * 0.85 + 1.7 * rz * j / (nz - 1);
        if (m == MT_TIMBER) g.post(x, z, fy, r, 500 + i * 7 + j);
        else { const double hh = standing ? fy : fy * wallShare(D, g.U(500 + i * 7 + j)); g.lathe(0, x, z, r, 0, std::max(0.3, hh), slot, m, 6); }
    }
    if (standing && g.lod < 2) g.box(0, 0, 0, rx + 0.2, rz + 0.2, fy - 0.25, fy, slot, m, 2);
    return standing;
}

// C-08: on a desert the sand drifts against the walls the wind meets (the sides whose outward normal faces within 60 degrees of where
// the wind comes from), a low wedge the age has built up; part 7, drawn in the sand's bank (P-04: a frozen world's snow, in the snow's)
void drifts(Gen& g, double hw, double hd, double D) {
    if (!g.st.drift || g.lod != 0 || g.c.buried < 0.15) return;
    const double hDrift = 0.25 + 0.6 * std::min(1.0, g.c.buried / 1.2) * (0.5 + 0.5 * D);
    for (int side = 0; side < 4; side++) {
        double nAng = g.b.heading + side * PI / 2;   // the side's outward normal: 0 front (+z'), 1 right (+x'), 2 back, 3 left
        if (std::fabs(std::remainder(nAng - g.st.wind, TAU)) > PI / 3) continue;
        bool along = side == 0 || side == 2; double L = along ? hw : hd;
        double o = g.b.thick * 0.5 + 0.95;   // from the wall's outer face outward (clear of the wall's centre line, where the doorway is read)
        double lx = side == 1 ? hw + o : (side == 3 ? -hw - o : 0), lz = side == 0 ? hd + o : (side == 2 ? -hd - o : 0);
        g.box(lx, lz, along ? 0 : PI / 2, L, 0.9, 0, hDrift, 0, -1, 7);
    }
}

// rubble at the foot of a wall that fell (C-01's: three blocks a side, none in the doorway)
void rubble(Gen& g, double hw, double hd, const double hs[4], double h, double doorX, double doorZ, int slot, int mat) {
    if (g.lod != 0) return;
    for (int side = 0; side < 4; side++) {
        if (hs[side] > 0.5 * h) continue;
        bool along = side == 0 || side == 2; double L = along ? hw : hd;
        double lx = side == 1 ? hw : (side == 3 ? -hw : 0), lz = side == 0 ? hd : (side == 2 ? -hd : 0);
        for (int k = 0; k < 3; k++) {
            double u1 = g.U(60 + side * 3 + k), u2 = g.U(80 + side * 3 + k), sz = 0.3 + 0.5 * g.U(100 + side * 3 + k);
            double tt = (u1 - 0.5) * 2 * L, off = 0.8 + 1.5 * u2;
            double px = along ? tt : lx + (side == 1 ? off : -off), pz = along ? lz + (side == 0 ? off : -off) : tt;
            if ((px - doorX) * (px - doorX) + (pz - doorZ) * (pz - doorZ) < (1.9 + sz) * (1.9 + sz)) continue;   // not in the doorway
            g.box(px, pz, u1 * 1.2, sz, sz * 0.8, 0, sz * 1.1, slot, mat, 3);
        }
    }
}

// the walls' material of a people that built in stone (the rock, or the adobe of a sand family); A-04: a people that built light builds
// its houses of timber, one of expressed frames of daub between its posts
int stoneOf(const Culture& c) { return c.family == 1 ? MT_EARTH : MT_MASONRY; }
int houseMaterial(const Culture& c) {
    if (c.genome.at(GV_LASTING) < 0.2) return MT_TIMBER;
    if (c.genome.of(GV_EXPRESSION) == EX_FRAME) return MT_EARTH;
    return stoneOf(c);
}
// the slot of a material's walls: timber's own (dark) where the walls are of it, the structure's otherwise
int wallSlot(const Gen& g, int mat) { return mat == MT_TIMBER ? g.timberSlot() : 0; }
}   // namespace

// ---- the medieval age: C-01's kinds, the temple, the wall's towers, the windmill; C-08's features --------------------------------

namespace {
// the material of the works every age built the same way (C-08's features, the platform, a stela, the town wall, A-04's focal accent):
// its stone, then brick, mass concrete, composite
int worksOf(const Building& b, const Culture& c) {
    switch (b.age) {
        case AGE_PREHISTORIC: return MT_DRYSTONE;
        case AGE_MODERN: return MT_MASS;
        case AGE_FUTURISTIC: return MT_COMPOSITE;
        case AGE_FAR_FUTURE: return MT_ETERNAL;
        default: return stoneOf(c);
    }
}
// the structure of a great building of its age: stone, brick, concrete, composite
int greatOf(const Building& b, const Culture& c) {
    switch (b.age) {
        case AGE_INDUSTRIAL: return c.family == 1 && false ? MT_EARTH : MT_MASONRY;
        case AGE_MODERN: return MT_CONCRETE;
        case AGE_FUTURISTIC: return MT_COMPOSITE;
        case AGE_FAR_FUTURE: return MT_ETERNAL;
        case AGE_PREHISTORIC: return MT_DRYSTONE;
        default: return stoneOf(c);
    }
}
// a house's roof: its people's, or (a people whose houses vary) another of the common ones
int houseRoof(const Gen& g) {
    const double var = g.gf(GV_SAMENESS);
    if (var > 0.55 && g.U(41) < (var - 0.55) * 1.6) { static const int ALT[4] = {RF_PITCHED, RF_HIPPED, RF_FLAT, RF_CONICAL}; return ALT[std::min(3, (int)(g.U(42) * 4))]; }
    return g.c.roof;
}
// B-414: the roof an industrial or a modern people put on its everyday buildings (its rows, tenements, villas, mills, halls and blocks);
// the people's roof stays on its great buildings. B-415: the age's alone: an industrial people's slate on timber, pitched (a people of
// gables, steps, vaults or saddles) or hipped (the rest; a people of flat roofs hid it behind a parapet: `parapetRoof`), a modern people's
// flat (B-414 kept a modern people's pitched, hipped and saddle roofs and an industrial people's flat ones: BUBAS's flat-roofed rows read as
// modern, a modern suburb of gables as industrial)
int ageRoof(const Culture& c, int rf) {
    if (c.age == AGE_INDUSTRIAL) return rf == RF_PITCHED || rf == RF_STEPPED || rf == RF_VAULTED || rf == RF_SADDLE ? RF_PITCHED : RF_HIPPED;
    if (c.age == AGE_MODERN) return RF_FLAT;
    return rf;
}
// B-415: an industrial people of flat roofs or open tops: its everyday buildings' low hipped roofs behind a parapet
bool parapetRoof(const Culture& c) { return c.age == AGE_INDUSTRIAL && (c.roof == RF_FLAT || c.roof == RF_OPEN); }
// a round temple's ring: its people's polygon (doubled where it has few sides) or sixteen
int templeRingSegs(const Building& b) { return b.form == PF_POLYGON ? clampi(b.sides, 3, 9) * (b.sides <= 4 ? 2 : 1) : 16; }
// the first segment of a ring's doorway: the building's draw (C-01's rotunda), or round the front for a centred one
int ringDoorFirst(const Building& b, int segs, int doorSegs) { return b.centred ? ((segs - doorSegs / 2) % segs + segs) % segs : (int)(u01(b.seed, 5) * segs); }
// the tops a people puts on a rectangular building: finials at its gable's ends or its corners, a mast at one corner, statues at the
// front's corners; those on a roof (a gable's ends, a hipped, conical, domed or stepped roof's crown) of the roof's material and only
// while the roof stands whole (a finial outlives no roof), those at the walls' corners of the walls'
void houseTops(Gen& g, int rf, double cx, double cz, double hw, double hd, double y, double ridge, int roofMat, int wallMat) {
    if (g.lod != 0) return;
    const int tp = g.gv(GV_TOPS);
    if (tp == TP_NOTHING || tp == TP_CRENELS) return;
    std::vector<std::pair<double, double>> pts;
    bool onRoof = true;
    if (rf == RF_PITCHED) { const bool alongZ = hd >= hw; if (alongZ) { pts = {{cx, cz + hd}, {cx, cz - hd}}; } else { pts = {{cx + hw, cz}, {cx - hw, cz}}; } y += ridge; }
    else if (rf == RF_HIPPED || rf == RF_CONICAL || rf == RF_DOMED || rf == RF_STEPPED) { pts = {{cx, cz}}; y += ridge; }
    else {
        onRoof = false;
        if (tp == TP_MASTS) pts = {{cx + hw * 0.7, cz - hd * 0.7}};
        else if (tp == TP_STATUES) pts = {{cx - hw, cz + hd}, {cx + hw, cz + hd}};
        else pts = {{cx - hw, cz + hd}, {cx + hw, cz + hd}, {cx - hw, cz - hd}, {cx + hw, cz - hd}};
    }
    if (onRoof && g.D(roofMat) >= 0.25) return;
    topsAt(g, pts, y, 1.0, onRoof ? roofMat : wallMat);
}
// the tops on a round roof's crown (a cone, a dome, a stepped crown; a flat roof's middle), while the roof stands whole
void roundTops(Gen& g, int rf, double y, double rise, double scale, int roofMat) {
    if (rf == RF_OPEN || g.D(roofMat) >= (rf == RF_FLAT ? 0.35 : 0.25)) return;
    topsAt(g, {{0, 0}}, y + (rf == RF_FLAT ? 0.35 : rise), scale, roofMat);
}
// ---- R-412: the people's dress on its dwellings -----------------------------------------------------------------------------------
// what its genome adds to a house's body, read through its age (B-415: the age speaks first): the doorway's porch, steps, portico or
// canopy; its corners' quoins, pilasters or turrets (a modern people's fins and rounded corners); its fronts' balconies, galleries, bays and
// oriels; its roof's dormers, cross gables, cresting and lantern (a modern people's flat roof's pergola, tank, rails and penthouse); its yard's
// wall, fence, hedge or railings; a prehistoric people's pole by its hut's door

// a side of four walls: its outer face's middle, its outward normal, its axis (the walls' own: +x' on the front and the back, -z' on the
// right and the left), its half length, the heading a box laid along it turns by
struct Face { double fx = 0, fz = 0, nx = 0, nz = 0, ax = 1, az = 0, L = 1, dh = 0; };
Face faceOf(const Walls& W, int side, double ht) {
    Face f;
    f.nx = side == 1 ? 1 : (side == 3 ? -1 : 0); f.nz = side == 0 ? 1 : (side == 2 ? -1 : 0);
    const bool along = side == 0 || side == 2;
    f.ax = along ? 1 : 0; f.az = along ? 0 : -1; f.L = along ? W.hw : W.hd; f.dh = along ? 0 : PI / 2;
    f.fx = W.cx + f.nx * (W.hw + ht); f.fz = W.cz + f.nz * (W.hd + ht);
    return f;
}
void faceAt(const Face& f, double t, double o, double& x, double& z) { x = f.fx + f.ax * t + f.nx * o; z = f.fz + f.az * t + f.nz * o; }
// a box against a face: `t` along its axis, its middle `o` out from it, `a` its half length along, `d` its half depth out
Form& faceBox(Gen& g, const Face& f, double t, double o, double a, double d, double y0, double y1, int slot, int mat, int part = 0) {
    double x, z; faceAt(f, t, o, x, z);
    return g.box(x, z, f.dh, a, d, y0, y1, slot, mat, part);
}
// a fence along a face (`across` false) or out from it (true): its middle `t` along and `o` out, `a` its half length
Form& faceFence(Gen& g, const Face& f, bool across, double t, double o, double a, double y0, double y1, int sub, int slot, int mat, int part = 4) {
    double x, z; faceAt(f, t, o, x, z);
    Form& e = g.add(FK_FENCE, sub, x, z, across ? f.dh : f.dh + PI / 2, 0.025, a, y0, y1, slot, mat, part);
    return e;
}
// the slot the walls are drawn in (`walls4x`: a house painted in the accent)
int wallsSlot(const Gen& g, const Walls& W) { return W.slot == 0 && g.painted ? 1 : W.slot; }
// the walls' half thickness as `walls4x` builds them (a massive people's thicker, a battered or a bulging wall's more)
double wallsHalf(const Gen& g, const Walls& W) {
    const int prof = W.profile >= 0 ? W.profile : (W.dress ? g.gv(GV_PROFILE) : WP_UPRIGHT);
    double t = W.thick;
    if (W.dress && g.gv(GV_EXPRESSION) == EX_MASSIVE) t *= 1.35;
    if (prof == WP_BATTERED) t = std::max(t * 1.3, std::min(2.4, 0.12 * W.h)); else if (prof == WP_BULGING) t *= 1.6;
    return 0.5 * t;
}
// a railing's sub and stuff by the age: a medieval people's timber pales (a balustrade of its stone where `stone`), an industrial people's
// iron bars, a modern people's steel ones, a futuristic people's composite
void railOf(const Gen& g, bool stone, int& sub, int& slot, int& mat) {
    switch (g.c.age) {
        case AGE_MEDIEVAL: if (stone) { sub = 3; slot = 0; mat = stoneOf(g.c); } else { sub = 0; slot = g.timberSlot(); mat = MT_TIMBER; } return;
        case AGE_INDUSTRIAL: sub = 1; slot = g.ironSlot(); mat = MT_IRON; return;
        case AGE_MODERN: sub = 1; slot = g.ironSlot(); mat = MT_IRON; return;
        default: sub = 1; slot = 2; mat = MT_COMPOSITE; return;
    }
}

// the doorway's: a porch on two posts under a little gable, a stoop of three steps, a portico of columns under its pediment, a canopy over
// it; a modern people's porch a slab on two square posts, its portico a deep slab on round columns
void entranceAt(Gen& g, const Walls& W, double ht, double H) {
    const Culture& c = g.c;
    if (W.door < 0 || W.y0 > 0.5 || c.age < AGE_MEDIEVAL || c.age > AGE_MODERN) return;
    const Face f = faceOf(W, W.door, ht);
    const double t = W.doorPos, gnd = 0.0;
    const int en = g.gv(GV_ENTRANCE), modern = c.age == AGE_MODERN;
    const double Dt = g.D(MT_TIMBER), Dm = g.D(W.mat);
    switch (en) {
        case EN_PORCH: {
            if (std::fabs(t) + 1.45 > f.L + 0.3) return;
            if (modern) {
                if (g.D(MT_CONCRETE) > 0.6) return;
                for (int s = -1; s <= 1; s += 2) faceBox(g, f, t + s * 1.4, 1.75, 0.12, 0.12, gnd, gnd + 2.6, 0, MT_CONCRETE);
                faceBox(g, f, t, 1.0, 1.65, 1.05, gnd + 2.6, gnd + 2.85, 0, MT_CONCRETE, 2);
                return;
            }
            for (int s = -1; s <= 1; s += 2) { double x, z; faceAt(f, t + s * 1.35, 1.65, x, z); g.post(x, z, 2.55, 0.1, 520 + (s > 0)); }
            if (Dt < 0.35 && g.lod == 0) {
                double x, z; faceAt(f, t, 0.8, x, z);
                const double y0 = gnd + 2.5;
                Form& r = g.add(FK_SWEEP, 1, x, z, f.dh, 0, 1.05, y0, y0 + 1.6 * 0.75 * g.pitchK(), g.roofSlot(MT_TIMBER), MT_TIMBER, 2);
                r.p[1] = 1.6; r.p[2] = 0.08;
            }
            return;
        }
        case EN_STEPS: {
            if (g.lod != 0) return;
            for (int k = 0; k < 3; k++) faceBox(g, f, t, (1.15 - 0.33 * k) * 0.5, 1.25 - 0.1 * k, (1.15 - 0.33 * k) * 0.5, gnd, gnd + 0.17 * (k + 1), W.slot, W.mat, 4);
            if (c.age == AGE_INDUSTRIAL && g.D(MT_IRON) < 0.5) for (int s = -1; s <= 1; s += 2) { Form& e = faceFence(g, f, true, t + s * 1.28, 0.55, 0.55, gnd + 0.51, gnd + 1.35, 1, g.ironSlot(), MT_IRON); e.p[3] = 0.08; }
            if (modern) for (int s = -1; s <= 1; s += 2) faceBox(g, f, t + s * 1.38, 0.6, 0.12, 0.6, gnd, gnd + 0.75, 0, MT_CONCRETE, 4);
            return;
        }
        case EN_PORTICO: {
            if (H < 3.0 || Dm > 0.5) return;
            const bool four = std::fabs(t) + 2.9 <= f.L;
            const double colH = std::min(H - 0.3, modern ? 3.6 : 4.4), out = 2.2, span = four ? 2.75 : 1.5;
            const int cols = four ? 4 : 2;
            for (int k = 0; k < cols; k++) {
                const double tc = t + (cols == 4 ? (k < 2 ? -1 : 1) * (k % 2 ? 2.55 : 1.3) : (k ? 1.3 : -1.3));
                double x, z; faceAt(f, tc, out - 0.25, x, z);
                g.lathe(0, x, z, modern ? 0.16 : 0.24, gnd, gnd + colH, W.slot, W.mat, modern ? 0 : (g.gv(GV_PLAN) == PF_RECT ? 4 : 0));
            }
            if (Dm > 0.35 || g.lod > 1) return;
            faceBox(g, f, t, out * 0.5, span + 0.3, out * 0.5, gnd + colH, gnd + colH + (modern ? 0.35 : 0.45), W.slot, W.mat, 2);
            if (!modern) {   // its pediment, its gable to the street
                double x, z; faceAt(f, t, out * 0.5, x, z);
                const double y0 = gnd + colH + 0.45;
                Form& p = g.add(FK_SWEEP, 1, x, z, f.dh, 0, out * 0.5, y0, y0 + (span + 0.3) * 0.32, W.slot, W.mat, 4); p.p[1] = span + 0.3; p.p[2] = 0.05;
            }
            return;
        }
        case EN_CANOPY: {
            if (g.lod != 0) return;
            if (modern) { if (g.D(MT_CONCRETE) < 0.6) faceBox(g, f, t, 1.0, 1.9, 1.0, gnd + 2.55, gnd + 2.8, 0, MT_CONCRETE, 2); return; }
            if (c.age == AGE_INDUSTRIAL) {   // an iron canopy on its brackets
                if (g.D(MT_IRON) > 0.5) return;
                faceBox(g, f, t, 0.75, 1.45, 0.75, gnd + 2.6, gnd + 2.7, g.ironSlot(), MT_IRON, 2);
                for (int s = -1; s <= 1; s += 2) { Form& b = faceBox(g, f, t + s * 1.3, 0.35, 0.04, 0.35, gnd + 2.1, gnd + 2.6, g.ironSlot(), MT_IRON, 4); (void)b; }
                return;
            }
            if (Dt < 0.4) { Form& h = faceBox(g, f, t, 0.55, 1.3, 0.55, gnd + 2.45, gnd + 2.6, g.roofSlot(MT_TIMBER), MT_TIMBER, 2); h.tilt = W.door <= 1 ? 0.28 : -0.28; }   // a hood over the door, its outer edge lower (a box's +along is out on the front and the right)
            return;
        }
        default: return;
    }
}

// the corners': quoins (alternate long and short blocks up the front's two corners, lighter than the wall), pilasters with their capitals, a
// turret corbelled out over each front corner under its cap; a modern people's fins at its front's corners or its rounded corners
void cornersOf(Gen& g, const Walls& W, const double hs[4], double ht, int roofMat) {
    const Culture& c = g.c;
    if (c.age < AGE_MEDIEVAL || c.age > AGE_MODERN) return;
    const int co = g.gv(GV_CORNERS);
    if (co == CO_PLAIN) return;
    const bool modern = c.age == AGE_MODERN;
    const double base = g.sink + W.y0, H = W.h;
    if (g.b.joined && (co == CO_TURRETS || modern)) return;   // a house in a row: its neighbours at its corners
    for (int q = 0; q < 4; q++) {
        const int sx = q % 2 ? 1 : -1, szs = q < 2 ? 1 : -1;   // q 0, 1 the front's corners
        if (szs < 0 && (co != CO_PILASTERS || g.lod != 0)) continue;
        const int sa = szs > 0 ? 0 : 2, sb = sx > 0 ? 1 : 3;
        const double hc = std::min(hs[sa], hs[sb]);
        const double Cx = W.cx + sx * (W.hw + ht), Cz = W.cz + szs * (W.hd + ht);   // the outer corner
        if (modern) {
            if (hc < 0.97 * H) continue;
            if (co == CO_QUOINS && szs > 0 && H > 5) g.box(Cx - sx * 0.58, Cz - 0.58, 0, 0.6, 0.6, base + 2.8, base + H - 0.4, g.ironSlot(), MT_CONCRETE, 4);   // its corners glazed (dark)
            if (co == CO_PILASTERS && szs > 0) g.box(Cx - sx * 0.12, Cz + 0.45, 0, 0.12, 0.45, base, base + H, W.slot, W.mat, 4);   // a fin
            else if (co == CO_TURRETS && szs > 0 && H > 5) { Form& e = g.lathe(0, Cx - sx * 0.9, Cz - 0.9, 1.6, base, base + H + 0.3, wallsSlot(g, W), W.mat); e.pattern = W.pattern; }   // a rounded corner
            continue;
        }
        if (co == CO_QUOINS) {
            if (g.lod != 0 || hc < 1.0) continue;
            const int n = std::min(9, (int)(hc / 0.62));
            for (int k = 0; k < n; k++) {
                const bool longX = k % 2 == 0;
                const double ex = longX ? 0.55 : 0.3, ez = longX ? 0.3 : 0.55;
                Form& e = g.box(Cx - sx * (ex - 0.05), Cz - szs * (ez - 0.05), 0, ex, ez, base + k * 0.62, base + k * 0.62 + 0.54, W.slot, W.mat, 4);
                e.tone = g.tone + 9;
            }
        } else if (co == CO_PILASTERS) {
            if (hc < 0.97 * H || g.lod > 1) continue;
            g.box(Cx - sx * 0.25, Cz - szs * 0.25, 0, 0.37, 0.37, base, base + H - 0.25, W.slot, W.mat, 4);
            g.box(Cx - sx * 0.25, Cz - szs * 0.25, 0, 0.47, 0.47, base + H - 0.55, base + H - 0.05, W.slot, W.mat, 4);
        } else if (co == CO_TURRETS) {
            if (hc < 0.97 * H) continue;
            const bool low = H < 4.8;   // a low house's: a round tower at its front corner away from its door, from the ground
            if (low && (sx > 0) == (W.door == 0 && W.doorPos > 0)) continue;
            const double rt = clampd(0.2 * std::min(W.hw, W.hd), 0.75, 1.35), tx = Cx + sx * rt * 0.25, tz = Cz + szs * rt * 0.25, y0 = low ? base : base + std::max(2.6, 0.45 * H);
            if (!low) { Form& k = g.lathe(1, tx, tz, 0.3, y0 - 1.1, y0, wallsSlot(g, W), W.mat); k.p[0] = rt / 0.3; }   // its corbel
            { Form& e = g.lathe(0, tx, tz, rt, y0, base + H + (low ? 1.3 : 0.6), wallsSlot(g, W), W.mat, g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 5, 8) : 0); e.pattern = g.lod == 0 ? W.pattern : 0; }
            const double Dr = g.D(roofMat);
            if (Dr < 0.4) {
                const double y1 = base + H + (low ? 1.3 : 0.6);
                if (c.roof == RF_DOMED) { Form& d = g.add(FK_DOME, ageDome(g, 2), tx, tz, 0, rt + 0.1, rt + 0.1, y1, y1 + rt * 1.2, g.roofSlot(roofMat), roofMat, 2); (void)d; }
                else { Form& s = g.lathe(1, tx, tz, rt + 0.15, y1, y1 + rt * 2.4 * g.pitchK(), g.roofSlot(roofMat), roofMat); s.p[0] = 0.04; }
            }
        }
    }
}

// what stands out from its front (side 0) over its doorway's storey: a balcony to a bay of each upper storey, a gallery along each, two
// bay windows from the ground (a canted one an industrial people's), an oriel on the first floor
void projectionsOf(Gen& g, const Walls& W, const double hs[4], double ht, int prof) {
    const Culture& c = g.c;
    if (c.age < AGE_MEDIEVAL || c.age > AGE_MODERN || hs[0] < 0.97 * W.h || W.y0 > 0.5 || g.sink < -0.6) return;   // none on a sunk house (its first floor at the ground)
    const int pj = g.gv(GV_PROJECTIONS);
    if (pj == PJ_NONE) return;
    Face f = faceOf(W, 0, ht);
    const double base = g.sink + W.y0, H = W.h, L = f.L;
    const WindowSpec& ws = c.palette.windows[clampi(W.pattern > 0 ? W.pattern - 1 : 0, 0, 3)];
    const double storey = c.age == AGE_MEDIEVAL ? 2.7 : (c.age == AGE_INDUSTRIAL ? 3.3 : 3.0), bay = std::max(2.4, ws.bay);
    const int storeys = std::min(c.age == AGE_MODERN ? 8 : 5, (int)std::lround(H / storey));
    // the front's face at a height over the foot as `walls4x` leans or jetties it: a battered wall leaning in, a bulging one a little, an
    // upper storey jettied out or set back over its tier
    const bool tiered = (prof == WP_LEANING || prof == WP_STEPPED) && H > 4.5;
    const double tierY = tiered ? clampd(H * 0.5, 2.9, 4.2) : 1e9, jetty = prof == WP_LEANING ? 0.5 : -0.45;
    const double lean = prof == WP_BATTERED ? std::atan(std::min(0.1 * H, 1.25 * ht) / std::max(1.0, H)) : (prof == WP_BULGING ? 0.035 : (prof == WP_LEANING && !tiered ? -0.06 : 0.0));
    auto faceOff = [&](double y) { return (y - base >= tierY - 0.01 ? jetty : 0.0) - std::tan(lean) * (y - base); };
    auto upAt = [&](double y) { Face q = f; const double o = faceOff(y); q.fx += q.nx * o; q.fz += q.nz * o; return q; };   // the face as it stands at y
    const bool doorFront = W.door == 0;
    auto clearOfDoor = [&](double t, double a) { return !doorFront || std::fabs(t - W.doorPos) > a + 1.25; };
    int sub, rs, rm; railOf(g, false, sub, rs, rm);
    const bool railUp = g.D(rm) < 0.55;
    const int slabMat = c.age == AGE_MEDIEVAL ? MT_TIMBER : (c.age == AGE_MODERN ? MT_CONCRETE : W.mat), slabSlot = slabMat == MT_TIMBER ? g.timberSlot() : W.slot;
    const bool slabUp = g.D(slabMat) < 0.5;
    switch (pj) {
        case PJ_BALCONIES: {
            if (storeys < 2 || !slabUp || g.lod != 0) return;
            const int nb = clampi((int)std::lround(L / bay), 1, 3);
            for (int s = 1; s < storeys; s++)
                for (int j = 0; j < nb; j++) {
                    const double tb = -L + (j + 0.5) * 2 * L / nb, a = std::min(1.1, L / nb - 0.25), y = base + s * storey;
                    if (a < 0.5) continue;
                    const Face q = upAt(y);
                    faceBox(g, q, tb, 0.5, a, 0.5, y - 0.18, y, slabSlot, slabMat, 2);
                    if (railUp) { Form& r = faceFence(g, q, false, tb, 0.95, a, y, y + 1.0, sub, rs, rm); r.p[3] = 0; }
                }
            return;
        }
        case PJ_GALLERIES: {
            if (!slabUp || g.lod != 0) return;
            if (storeys < 2) {   // one storey: a verandah along its front, its posts and its lean-to roof under the eaves
                if (H < 2.6 || c.age == AGE_MODERN) return;
                const double y = base + std::min(H - 0.15, 2.7);
                const int np = std::max(1, (int)std::lround(2 * L / 2.6));
                for (int k = 0; k <= np; k++) {
                    const double tp = -L + 2 * L * k / np;
                    if (!clearOfDoor(tp, 0.2)) continue;
                    double x, z; faceAt(f, tp, 1.55, x, z);
                    if (c.age == AGE_INDUSTRIAL) { if (g.D(MT_IRON) < 0.6) g.lathe(0, x, z, 0.08, 0, y - 0.1, g.ironSlot(), MT_IRON, 6); }
                    else g.post(x, z, y - 0.1, 0.11, 560 + k);
                }
                if (g.D(MT_TIMBER) < 0.35) { Form& r = faceBox(g, f, 0, 0.85, L + 0.25, 0.85, y - 0.1, y + 0.06, g.roofSlot(MT_TIMBER), MT_TIMBER, 2); r.tilt = 0.2; }   // its outer edge lower
                return;
            }
            for (int s = 1; s < storeys; s++) {
                const double y = base + s * storey;
                const Face q = upAt(y);
                faceBox(g, q, 0, 0.75, L + 0.2, 0.75, y - 0.2, y, slabSlot, slabMat, 2);
                if (railUp) faceFence(g, q, false, 0, 1.45, L + 0.15, y, y + 1.0, sub, rs, rm);
                if (s == 1 && c.age != AGE_MODERN)   // its posts down to the ground: a verandah under the first
                    for (int k = 0; k <= std::max(1, (int)std::lround(2 * L / 2.6)); k++) {
                        const double tp = -L + 2 * L * k / std::max(1, (int)std::lround(2 * L / 2.6));
                        if (!clearOfDoor(tp, 0.2)) continue;
                        double x, z; faceAt(f, tp, 1.35, x, z);
                        if (c.age == AGE_INDUSTRIAL) { if (g.D(MT_IRON) < 0.6) g.lathe(0, x, z, 0.09, 0, y - 0.2, g.ironSlot(), MT_IRON, 6); }
                        else g.post(x, z, y - 0.2 - base, 0.12, 540 + k);
                    }
            }
            return;
        }
        case PJ_BAYS: {
            if (prof == WP_BATTERED) return;   // nothing stands against a wall leaning in
            const double a = std::min(1.2, L * 0.3), top = std::min(tiered ? tierY - 0.05 : 1e9, H > 8 ? std::min(H - 0.3, 2 * storey) : H - 0.3);
            if (a < 0.7 || top < 2.4) return;
            for (int s = -1; s <= 1; s += 2) {
                const double tb = s * L * 0.55;
                if (!clearOfDoor(tb, a) || std::fabs(tb) + a > L - 0.2) continue;
                { Form& e = faceBox(g, f, tb, 0.45, a, 0.45, base, base + top, wallsSlot(g, W), W.mat); e.pattern = W.pattern; }
                if (g.lod == 0) {   // its roof
                    if (c.age == AGE_MODERN) faceBox(g, f, tb, 0.5, a + 0.12, 0.55, base + top, base + top + 0.25, W.slot, W.mat, 2);
                    else if (g.D(MT_TIMBER) < 0.4) { double x, z; faceAt(f, tb, 0.35, x, z); Form& r = g.add(FK_PYRAMID, 0, x, z, f.dh, a + 0.15, 0.7, base + top, base + top + 0.55, g.roofSlot(MT_TIMBER), MT_TIMBER, 2); r.p[1] = 0.25; }
                }
            }
            return;
        }
        case PJ_ORIELS: {
            if (H < 3.6) return;
            const double a = std::min(0.95, L * 0.28), y0 = base + std::max(tiered ? tierY : 2.0, std::min(storey, H * 0.45)), y1 = base + std::min(H - 0.3, y0 - base + storey * (storeys > 2 ? 2 : 1) - 0.2);
            if (a < 0.6 || y1 - y0 < 1.2) return;
            const int n = L > 4.2 ? 2 : 1;
            for (int j = 0; j < n; j++) {
                const double tb = n == 1 ? (doorFront ? (W.doorPos > 0 ? -L * 0.45 : L * 0.45) : 0) : (j ? 1 : -1) * L * 0.5;
                const Face q = upAt(y0);
                Form& e = faceBox(g, q, tb, 0.42, a, 0.42, y0, y1, wallsSlot(g, W), W.mat); e.pattern = W.pattern;
                faceBox(g, q, tb, 0.25, a * 0.7, 0.25, y0 - 0.7, y0, wallsSlot(g, W), W.mat, 4);   // its corbel
                if (g.lod == 0) {
                    if (c.age == AGE_MODERN) faceBox(g, q, tb, 0.45, a + 0.08, 0.47, y1, y1 + 0.2, W.slot, W.mat, 2);
                    else if (g.D(MT_TIMBER) < 0.4) { double x, z; faceAt(q, tb, 0.42, x, z); Form& r = g.add(FK_PYRAMID, 0, x, z, f.dh, a + 0.1, 0.52, y1, y1 + 0.9 * g.pitchK(), g.roofSlot(MT_TIMBER), MT_TIMBER, 2); r.p[1] = 0; }
                }
            }
            return;
        }
        default: return;
    }
}

// what rises from its roof: on a pitched or a hipped roof dormers along its front slope, a cross gable in the middle of it, cresting along its
// ridge (a medieval people's pales of its roofing, an industrial people's iron), a lantern on it (a medieval people's bell-cote, an
// industrial people's ventilator); on a modern people's flat roof a pergola, a water tank on its legs, rails round its edge, a penthouse
void rooflineOf(Gen& g, const Walls& W, double hmin, int rf, double rise, int roofMat) {
    const Culture& c = g.c;
    if (c.age < AGE_MEDIEVAL || c.age > AGE_MODERN || g.lod != 0 || hmin < 0.92 * W.h || W.hw < 1.6 || W.hd < 1.6) return;
    const int rl = g.gv(GV_ROOFLINE);
    if (rl == RL_PLAIN) return;
    const double y0 = g.sink + W.y0 + hmin - 0.05;
    if (rf == RF_FLAT && c.age != AGE_MODERN) {   // a medieval or an industrial people's flat roof: a pavilion on it, a stair house, a balustrade
        const int stone = W.mat;                     // round its edge, a cupola on its drum
        if (g.D(stone) > 0.45 || !structural(stone)) return;
        const double top = g.sink + W.y0 + hmin + 0.3, hw = W.hw, hd = W.hd;
        switch (rl) {
            case RL_DORMERS: {
                if (hw < 2.0 || hd < 2.0) return;
                const double px = W.cx - hw * 0.35, pz = W.cz - hd * 0.3;
                Form& p = g.box(px, pz, 0, hw * 0.42, hd * 0.42, top, top + 2.2, wallsSlot(g, W), stone); p.pattern = W.pattern;
                g.box(px, pz, 0, hw * 0.42 + 0.2, hd * 0.42 + 0.2, top + 2.2, top + 2.45, W.slot, stone, 2);
                return;
            }
            case RL_GABLETS:
                g.box(W.cx + hw - 0.9, W.cz - hd + 0.9, 0, 0.75, 0.75, top, top + 2.3, wallsSlot(g, W), stone);
                g.box(W.cx + hw - 0.9, W.cz - hd + 0.9, 0, 0.9, 0.9, top + 2.3, top + 2.5, W.slot, stone, 2);
                return;
            case RL_CRESTED:
                for (int s = 0; s < 4; s++) {
                    const bool al = s % 2 == 0;
                    const double x = W.cx + (s == 1 ? hw : (s == 3 ? -hw : 0)), z = W.cz + (s == 0 ? hd : (s == 2 ? -hd : 0));
                    g.add(FK_FENCE, 3, x, z, al ? PI / 2 : 0, 0.1, (al ? hw : hd) - 0.05, top, top + 0.8, W.slot, stone);
                }
                return;
            default: {
                const double r = clampd(0.22 * std::min(hw, hd), 0.6, 1.2);
                g.lathe(0, W.cx, W.cz, r, top, top + 0.9, W.slot, stone, 8);
                g.add(FK_DOME, ageDome(g, g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : (g.gv(GV_OPENING) == OP_TRIANGULAR ? 2 : 0)), W.cx, W.cz, 0, r + 0.1, r + 0.1, top + 0.9, top + 0.9 + r * 1.1, g.roofSlot(stone), stone, 2);
                return;
            }
        }
    }
    if (rf == RF_FLAT) {
        if (c.age != AGE_MODERN || g.D(MT_CONCRETE) > 0.45) return;
        const double top = g.sink + W.y0 + hmin + 0.3, hw = W.hw, hd = W.hd;
        switch (rl) {
            case RL_DORMERS: {   // a pergola over a corner of it
                if (g.D(MT_IRON) > 0.5 || hw < 2.5 || hd < 2.5) return;
                const double px = W.cx + hw * 0.45, pz = W.cz + hd * 0.4, ex = std::min(2.0, hw * 0.4), ez = std::min(2.0, hd * 0.45);
                for (int k = 0; k < 4; k++) g.lathe(0, px + (k % 2 ? ex : -ex), pz + (k < 2 ? ez : -ez), 0.07, top, top + 2.3, g.ironSlot(), MT_IRON, 4);
                for (int k = -1; k <= 1; k += 2) g.box(px, pz + k * ez, 0, ex + 0.1, 0.05, top + 2.15, top + 2.3, g.ironSlot(), MT_IRON, 4);   // its girts on the posts
                for (int k = -2; k <= 2; k++) g.box(px + k * ex * 0.45, pz, 0, 0.05, ez + 0.2, top + 2.3, top + 2.45, g.ironSlot(), MT_IRON, 4);   // its beams on them
                return;
            }
            case RL_GABLETS: {   // a water tank on its legs
                const double tx = W.cx - hw * 0.4, tz = W.cz - hd * 0.3, r = clampd(0.22 * std::min(hw, hd), 0.9, 1.8);
                for (int k = 0; k < 4; k++) { const double a = PI / 4 + k * PI / 2; g.lathe(0, tx + std::sin(a) * r * 0.8, tz + std::cos(a) * r * 0.8, 0.08, top, top + 1.4, g.ironSlot(), MT_IRON, 4); }
                Form& t = g.lathe(4, tx, tz, r, top + 1.4, top + 1.4 + 2.2 * r, 0, MT_CONCRETE); t.p[0] = 0.35;
                return;
            }
            case RL_CRESTED: {   // rails round its edge
                if (g.D(MT_IRON) > 0.5) return;
                for (int s = 0; s < 4; s++) {
                    const bool al = s % 2 == 0;
                    const double x = W.cx + (s == 1 ? hw : (s == 3 ? -hw : 0)), z = W.cz + (s == 0 ? hd : (s == 2 ? -hd : 0));
                    g.add(FK_FENCE, 1, x, z, al ? PI / 2 : 0, 0.025, (al ? hw : hd) - 0.05, top, top + 1.0, g.ironSlot(), MT_IRON);
                }
                return;
            }
            default: {   // a penthouse
                if (hw < 3 || hd < 3) return;
                Form& p = g.box(W.cx - hw * 0.2, W.cz - hd * 0.15, 0, hw * 0.55, hd * 0.55, top, top + 2.9, wallsSlot(g, W), W.mat); p.pattern = W.pattern;
                g.box(W.cx - hw * 0.2, W.cz - hd * 0.15, 0, hw * 0.55 + 0.3, hd * 0.55 + 0.3, top + 2.9, top + 3.15, W.slot, W.mat, 2);
                return;
            }
        }
    }
    if (rise < 0.8 || g.D(roofMat) > 0.25) return;
    if (rf == RF_DOMED || rf == RF_CONICAL || rf == RF_VAULTED) {   // a lantern on a dome's or a cone's crown, a vault's ridge crested or its lantern
        const double yTop = y0 + rise, along = std::max(W.hw, W.hd) - 0.3;
        if (rl == RL_LANTERN) {
            const double r = clampd(0.15 * std::min(W.hw, W.hd), 0.45, 0.9);
            g.lathe(0, W.cx, W.cz, r, yTop - 0.3, yTop + 1.1, wallsSlot(g, W), W.mat, 6);
            Form& s = g.lathe(1, W.cx, W.cz, r + 0.12, yTop + 1.1, yTop + 1.1 + r * 2.2 * g.pitchK(), g.roofSlot(roofMat), roofMat, 6); s.p[0] = 0.04;
        } else if (rl == RL_CRESTED && rf == RF_VAULTED) {
            Form& e = g.add(FK_FENCE, c.age == AGE_INDUSTRIAL ? 1 : 0, W.cx, W.cz, W.hd >= W.hw ? 0 : PI / 2, 0.03, along, yTop - 0.08, yTop + 0.42, g.roofSlot(roofMat), roofMat, 4);
            e.p[0] = 0.3; e.p[1] = 0.5; e.p[3] = 0.13;
        }
        return;
    }
    if (!(rf == RF_PITCHED || rf == RF_HIPPED)) return;
    const bool alongZ = W.hd >= W.hw, hip = rf == RF_HIPPED;   // the ridge's run (`roof`)
    const double eave = g.eave(), span = (alongZ ? W.hw : W.hd) + eave, len = (alongZ ? W.hd : W.hw) + (hip ? eave : eave * 0.6);
    const double topHalf = hip ? 0.28 * span : 0.12, ridgeLen = hip ? std::max(0.3, len - span + 0.28 * span) : len, yTop = y0 + rise;
    auto roofAt = [&](double w) { return y0 + rise * clampd((span - w) / std::max(0.1, span - topHalf), 0, 1); };
    // the frame along the ridge: u along it, w across it toward the front slope (+z' where the ridge runs along x', +x' where along z')
    auto at = [&](double u, double w, double& x, double& z) { if (alongZ) { x = W.cx + w; z = W.cz + u; } else { x = W.cx + u; z = W.cz + w; } };
    const double dhRun = alongZ ? 0 : PI / 2, dhCross = alongZ ? PI / 2 : 0;   // a sweep or a fence along the ridge, or across it (a box across it lies along it)
    const int rslot = g.roofSlot(roofMat);
    const double wall = (alongZ ? W.hw : W.hd);
    switch (rl) {
        case RL_DORMERS: {
            const int n = hip ? 1 : clampi((int)std::lround(2 * (len - eave) / 4.2), 1, 3);
            const double wf = span * 0.5, yr = roofAt(wf + 0.12);
            for (int k = 0; k < n; k++) {
                const double u = n == 1 ? 0 : -(len - eave) + (k + 0.5) * 2 * (len - eave) / n;
                double x, z;
                at(u, wf - 0.5, x, z); g.box(x, z, dhCross, 0.62, 0.62, yr - 0.45, yr + 1.1, wallsSlot(g, W), W.mat);
                at(u, wf + 0.13, x, z); g.box(x, z, dhCross, 0.32, 0.03, yr + 0.1, yr + 0.85, g.timberSlot(), MT_TIMBER, 4);   // its window
                at(u, wf - 0.45, x, z); Form& r = g.add(FK_SWEEP, 1, x, z, dhCross, 0, 0.78, yr + 1.05, yr + 1.05 + 0.82 * 0.8 * g.pitchK(), rslot, roofMat, 2); r.p[1] = 0.82; r.p[2] = 0.06;
            }
            return;
        }
        case RL_GABLETS: {   // a cross gable in the middle of the front slope, its own gable end in the walls' material over the front wall
            const double cw = std::min({len * 0.42, 2.4, span * 0.85}), ps = rise / std::max(0.1, span - topHalf);
            if (cw < 1.0) return;
            double x, z;
            at(0, span * 0.5, x, z); Form& r = g.add(FK_SWEEP, 1, x, z, dhCross, 0, span * 0.5 + 0.1, y0, y0 + cw * ps, rslot, roofMat, 2); r.p[1] = cw; r.p[2] = 0.1;
            at(0, wall, x, z); Form& e = g.add(FK_SWEEP, 1, x, z, dhCross, 0, 0.2, y0 + 0.02, y0 + (cw - eave) * ps, wallsSlot(g, W), W.mat); e.p[1] = cw - eave + 0.2; e.p[2] = 0.03;
            return;
        }
        case RL_CRESTED: {
            int sub = 0, slot = rslot, mat = roofMat;
            if (c.age == AGE_INDUSTRIAL) { sub = 1; slot = g.ironSlot(); mat = MT_IRON; if (g.D(MT_IRON) > 0.5) return; }
            Form& e = g.add(FK_FENCE, sub, W.cx, W.cz, dhRun, 0.03, std::max(0.4, (hip ? ridgeLen : len) - 0.2), yTop - 0.08, yTop + (sub ? 0.5 : 0.38), slot, mat, 4);
            e.p[0] = sub ? 0.16 : 0.32; e.p[1] = sub ? 0.25 : 0.55; e.p[3] = sub ? 0.1 : 0.14;
            return;
        }
        default: {   // a lantern on the ridge's middle
            if (c.age == AGE_INDUSTRIAL) {   // a ventilator: a louvred box under its own little roof
                g.box(W.cx, W.cz, dhRun, 0.45, std::min(1.0, ridgeLen * 0.4), yTop - 0.4, yTop + 0.75, g.timberSlot(), MT_TIMBER);
                Form& r = g.add(FK_SWEEP, 1, W.cx, W.cz, dhRun, 0, std::min(1.0, ridgeLen * 0.4) + 0.15, yTop + 0.7, yTop + 1.15, rslot, roofMat, 2); r.p[1] = 0.7; r.p[2] = 0.05;
                return;
            }
            Form& l = g.lathe(0, W.cx, W.cz, 0.55, yTop - 0.4, yTop + 1.05, wallsSlot(g, W), W.mat, 6); l.pattern = 0;
            if (c.roof == RF_DOMED) g.add(FK_DOME, ageDome(g, 3), W.cx, W.cz, 0, 0.62, 0.62, yTop + 1.05, yTop + 1.9, rslot, roofMat, 2);
            else { Form& s = g.lathe(1, W.cx, W.cz, 0.68, yTop + 1.05, yTop + 1.05 + 1.3 * g.pitchK(), rslot, roofMat, 6); s.p[0] = 0.04; }
            return;
        }
    }
}

// R-412: what stands out from a round house's wall (a medieval, an industrial or a modern people's): a verandah round it on its posts under
// a ring of roof, round bays from the ground under their cones, oriels on their corbels, balconies on an upper storey; at its sides'
// middles for a polygon, round it for a circle or an oval, none in its doorway
void ringDress(Gen& g, const Ring& R, double hmin) {
    const Culture& c = g.c;
    if (g.lod > 1 || c.age < AGE_MEDIEVAL || c.age > AGE_MODERN || hmin < 0.92 * R.h || !dwellingKind(g.b.kind) || g.sink < -0.6 || R.y0 > 0.5) return;
    const int pj = g.gv(GV_PROJECTIONS);
    if (pj == PJ_NONE) return;
    const int segs = R.segs;
    const bool round = segs >= 10;
    std::vector<std::pair<double, double>> wallAt;   // the angle, the wall's distance from the middle there
    for (int k = 0; k < segs; k++) {
        if (ringInDoor(R, k) || ringInDoor(R, (k + segs - 1) % segs) || ringInDoor(R, (k + 1) % segs)) continue;   // clear of the doorway
        const double a = (k + 0.5) * TAU / segs;
        wallAt.push_back({a, round ? std::hypot(std::sin(a) * R.rx, std::cos(a) * R.rz) : R.rx * std::cos(PI / segs)});
    }
    if (wallAt.empty()) return;
    const int want = std::min<int>(3, (int)wallAt.size());
    const double base = g.sink + R.y0, H = R.h, ht = 0.5 * R.thick;
    const int wslot = R.slot == 0 && g.painted ? 1 : R.slot;
    auto pick = [&](int j) { return wallAt[(size_t)((j * wallAt.size()) / want)]; };
    auto spot = [&](const std::pair<double, double>& w, double out, double& x, double& z) { x = R.cx + std::sin(w.first) * (w.second + out); z = R.cz + std::cos(w.first) * (w.second + out); };
    switch (pj) {
        case PJ_GALLERIES: {   // a verandah round it
            if (H < 2.6 || g.lod != 0 || c.age == AGE_MODERN) return;
            const double y = base + std::min(H - 0.15, 2.6), rr = std::max(R.rx, R.rz) + 1.6;
            const int np = clampi((int)std::lround(TAU * rr / 2.8), 6, 24);
            const double doorA = (R.door + 0.5 * R.doorSegs) * TAU / segs;
            for (int k = 0; k < np; k++) {
                const double a = (k + 0.5) * TAU / np;
                if (std::fabs(std::remainder(a - doorA, TAU)) * rr < 1.4) continue;   // its doorway's path
                const double x = R.cx + std::sin(a) * (R.rx + 1.5), z = R.cz + std::cos(a) * (R.rz + 1.5);
                if (c.age == AGE_INDUSTRIAL) { if (g.D(MT_IRON) < 0.6) g.lathe(0, x, z, 0.08, 0, y - 0.1, g.ironSlot(), MT_IRON, 6); }
                else g.post(x, z, y - 0.1, 0.11, 580 + k);
            }
            if (g.D(MT_TIMBER) < 0.35) {   // its lean-to roof: a ring of boards sloping out from the wall (a hollow cone's collider filled the room)
                const int nb = clampi((int)std::lround(TAU * rr / 2.4), 8, 20);
                for (int k = 0; k < nb; k++) {
                    const double a = (k + 0.5) * TAU / nb, x = R.cx + std::sin(a) * (R.rx + ht + 0.85), z = R.cz + std::cos(a) * (R.rz + ht + 0.85);
                    Form& b = g.add(FK_BOX, 0, x, z, a, rr * std::sin(PI / nb) + 0.12, 0.9, y - 0.05, y + 0.07, g.roofSlot(MT_TIMBER), MT_TIMBER, 2); b.tilt = 0.28;   // its outer edge lower
                }
            }
            return;
        }
        case PJ_BAYS: {   // round bays from the ground under their cones
            if (H < 2.4) return;
            for (int j = 0; j < want; j++) {
                double x, z; spot(pick(j), 0.25, x, z);
                Form& e = g.lathe(0, x, z, 0.85, base, base + H - 0.3, wslot, R.mat, 6); e.pattern = R.pattern;
                if (g.lod == 0 && g.D(MT_TIMBER) < 0.4) { Form& k = g.lathe(1, x, z, 0.95, base + H - 0.3, base + H + 0.5 * g.pitchK(), g.roofSlot(MT_TIMBER), MT_TIMBER, 6); k.p[0] = 0.05; }
            }
            return;
        }
        case PJ_ORIELS: {   // on their corbels, up its wall
            if (H < 3.4) return;
            for (int j = 0; j < want; j++) {
                double x, z; spot(pick(j), 0.3, x, z);
                const double y0 = base + std::max(1.9, 0.45 * H);
                { Form& k = g.lathe(1, x, z, 0.25, y0 - 0.8, y0, wslot, R.mat, 6); k.p[0] = 2.6; }
                Form& e = g.lathe(0, x, z, 0.65, y0, base + H - 0.2, wslot, R.mat, 6); e.pattern = R.pattern;
                if (g.lod == 0 && g.D(MT_TIMBER) < 0.4) { Form& cap = g.lathe(1, x, z, 0.75, base + H - 0.2, base + H + 0.7 * g.pitchK(), g.roofSlot(MT_TIMBER), MT_TIMBER, 6); cap.p[0] = 0.05; }
            }
            return;
        }
        default: {   // balconies on an upper storey
            if (H < 4.6 || g.lod != 0) return;
            const int slabMat = c.age == AGE_MEDIEVAL ? MT_TIMBER : (c.age == AGE_MODERN ? MT_CONCRETE : R.mat);
            if (g.D(slabMat) > 0.5) return;
            for (int j = 0; j < want; j++) {
                double x, z; spot(pick(j), 0.3, x, z);
                const double y = base + std::min(H - 1.2, c.age == AGE_MEDIEVAL ? 2.7 : 3.2);
                Form& e = g.lathe(0, x, z, 1.0, y - 0.18, y, slabMat == MT_TIMBER ? g.timberSlot() : wslot, slabMat); e.part = 2;
            }
            return;
        }
    }
}

// R-412: the dress a dwelling's body takes from its people (its doorway, its corners, its front, its roof), each where its age had it
void houseDress(Gen& g, const Walls& W, const double hs[4], double hmin, int rf, double rise, int roofMat, bool roofline) {
    if (g.lod > 1 || !W.dress) return;
    const int kind = g.b.kind;
    const bool dwelling = dwellingKind(kind) && !(kind == BK_HOUSE && g.c.age >= AGE_INDUSTRIAL);   // an industrial people's warehouse goes bare
    if (!dwelling && kind != BK_HALL) return;
    const int prof = W.profile >= 0 ? W.profile : g.gv(GV_PROFILE);
    const double ht = wallsHalf(g, W);
    entranceAt(g, W, ht, W.h);
    if (dwelling && roofline) rooflineOf(g, W, hmin, rf, rise, roofMat);
    if (prof == WP_UPRIGHT || prof == WP_BULGING) cornersOf(g, W, hs, ht, roofMat);   // a battered or a jettied front's corners its profile's
    if (dwelling) projectionsOf(g, W, hs, ht, prof);
}

// R-412: a yard's boundary: along the front to the street where a town's house stands back from it (`Building::yard`), its gate before the
// doorway, its returns back to the house's front corners; else round a garden behind a house that stands alone. A low wall of the walls'
// stone, brick or concrete; a fence of pales (a medieval people's timber, a later people's painted); a hedge (a wall on a desert); railings
// (an industrial people's iron on a low wall, a medieval people's balustrade, a modern people's steel on a kerb). Each under C-01's 0.9 m,
// stepped over as a low wall is
void yardRun(Gen& g, double x0, double z0, double x1, double z1, int mat) {
    const double len = std::hypot(x1 - x0, z1 - z0);
    if (len < 0.4) return;
    const double cx = 0.5 * (x0 + x1), cz = 0.5 * (z0 + z1), dh = std::atan2(x1 - x0, z1 - z0);   // its length along its heading
    const Culture& c = g.c;
    switch (g.gv(GV_BOUNDARY)) {
        case BD_WALLS: { const double D = g.D(mat); if (D < 0.85) g.box(cx, cz, dh, 0.18, len * 0.5, 0, 0.8 * wallShare(D, g.U(611)), 0, mat); return; }
        case BD_FENCES: {
            const double D = g.D(MT_TIMBER);
            if (D > 0.85) return;
            Form& e = g.add(FK_FENCE, 0, cx, cz, dh, 0.025, len * 0.5, 0, 0.85, c.age == AGE_MEDIEVAL || c.age == AGE_PREHISTORIC ? g.timberSlot() : 0, MT_TIMBER);
            e.p[3] = 0.1; e.broken = D < 0.25 ? 0 : clampd(D, 0.3, 0.9);
            return;
        }
        case BD_HEDGES:
            if (g.st.desert) { g.box(cx, cz, dh, 0.18, len * 0.5, 0, 0.7, 0, mat); return; }
            g.box(cx, cz, dh, 0.32, len * 0.5, 0, 0.85, 0, -1, 11);
            return;
        default: {   // railings
            if (c.age == AGE_MEDIEVAL) { Form& e = g.add(FK_FENCE, 3, cx, cz, dh, 0.1, len * 0.5, 0, 0.8, 0, mat); e.broken = clampd(g.D(mat) - 0.3, 0, 0.9); return; }
            const double kerb = c.age == AGE_INDUSTRIAL ? 0.4 : 0.15;
            g.box(cx, cz, dh, 0.16, len * 0.5, 0, kerb, 0, c.age == AGE_INDUSTRIAL ? mat : MT_CONCRETE);
            const double Di = g.D(MT_IRON);
            if (Di < 0.6) { Form& e = g.add(FK_FENCE, 1, cx, cz, dh, 0.02, len * 0.5, kerb, 0.85, g.ironSlot(), MT_IRON); e.p[3] = c.age == AGE_INDUSTRIAL ? 0.09 : 0; e.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.9); }
            return;
        }
    }
}
void yardOf(Gen& g, double hw, double hd, int doorSide, double doorPos, int mat) {
    const Building& b = g.b; const Culture& c = g.c;
    if (g.lod != 0 || c.age < AGE_MEDIEVAL || c.age > AGE_MODERN || g.gv(GV_BOUNDARY) == BD_OPEN) return;
    if (!dwellingKind(b.kind) || b.kind == BK_POD || (b.kind == BK_HOUSE && c.age >= AGE_INDUSTRIAL)) return;
    if (b.yard >= 2.2) {   // its front yard
        const double zf = hd + b.yard - 1.4, xa = b.joined ? -hw : -hw - 0.5, xb = b.joined ? hw : hw + 0.5;   // its kerb before it (the lamps')
        if (doorSide == 0) { yardRun(g, xa, zf, doorPos - 1.1, zf, mat); yardRun(g, doorPos + 1.1, zf, xb, zf, mat); }
        else yardRun(g, xa, zf, xb, zf, mat);
        if (!b.joined) { yardRun(g, xa, hd + 0.35, xa, zf, mat); yardRun(g, xb, hd + 0.35, xb, zf, mat); }
        return;
    }
    if (b.joined || b.kind == BK_BLOCK) return;
    const double zb = -hd - 3.2, xa = -hw - 0.4, xb = hw + 0.4;   // a garden behind it, its gate in its back
    yardRun(g, xa, -hd - 0.35, xa, zb, mat); yardRun(g, xb, -hd - 0.35, xb, zb, mat);
    yardRun(g, xa, zb, -1.0, zb, mat); yardRun(g, 1.0, zb, xb, zb, mat);
    (void)doorPos;
}

// R-412: a prehistoric people's pole by a hut's door (`GV_POLES`): a carved post (a totem), a pole flying a cloth of its accent, a pole with
// a skull on it, a cairn of three stones; and its yard (`GV_BOUNDARY`): a ring of wattle, of dry stone, of thorn, of pales round the hut
void hutPole(Gen& g, double px, double pz) {
    if (g.lod != 0) return;
    const double Dt = g.D(MT_TIMBER);
    switch (g.gv(GV_POLES)) {
        case PO_TOTEMS: {
            if (Dt > 0.6) return;
            const double h = 3.2 + 1.4 * g.U(620), r = 0.2, hh = Dt < 0.3 ? h : h * (1 - Dt);
            g.lathe(0, px, pz, r, 0, hh, g.timberSlot(), MT_TIMBER, 4);
            for (int k = 1; k <= 2; k++) if (hh > h * k / 3 + 0.3) { Form& e = g.box(px, pz, 0, r * 1.5, r * 1.5, h * k / 3 - 0.25, h * k / 3 + 0.25, k == 2 ? 1 : g.timberSlot(), MT_TIMBER, 4); e.heading += PI / 4; }
            if (Dt < 0.3) g.box(px, pz, 0, 0.75, 0.12, h - 0.55, h - 0.35, 1, MT_TIMBER, 4);   // its wings
            return;
        }
        case PO_BANNERS: {
            if (Dt > 0.5) return;
            g.lathe(0, px, pz, 0.06, 0, 4.6, g.timberSlot(), MT_TIMBER, 4);
            if (Dt < 0.25) { g.box(px, pz, 0, 0.04, 0.55, 4.45, 4.55, g.timberSlot(), MT_TIMBER, 4); g.box(px, pz + 0.05, 0, 0.02, 0.5, 3.0, 4.45, 1, MT_TIMBER, 4); }   // its cloth
            return;
        }
        case PO_SKULLS: {
            if (Dt > 0.6) return;
            g.lathe(0, px, pz, 0.07, 0, 2.3, g.timberSlot(), MT_TIMBER, 4);
            Form& s = g.add(FK_BLOB, 0, px, pz, 0, 0.16, 0.19, 2.25, 2.6, 0, -1, FORM_PART_PALE); s.p[0] = 0; s.p[1] = 0.1;
            for (int k = -1; k <= 1; k += 2) { Form& h = g.box(px + k * 0.22, pz, 0, 0.16, 0.03, 2.5, 2.56, 0, -1, FORM_PART_PALE); h.tilt = 0.5 * k; }   // its horns
            return;
        }
        case PO_CAIRNS: {
            const double Ds = g.D(MT_DRYSTONE);
            const int tiers = Ds < 0.35 ? 3 : (Ds < 0.7 ? 2 : 1);
            for (int i = 0; i < tiers; i++) g.box(px, pz, i * 0.4, 0.45 * (1 - 0.28 * i), 0.4 * (1 - 0.28 * i), 0.28 * i, 0.28 * (i + 1), 0, MT_DRYSTONE, 3);
            return;
        }
        default: return;
    }
}
void hutRing(Gen& g, double rx, double rz, double doorA) {
    if (g.lod != 0) return;
    const int bd = g.gv(GV_BOUNDARY);
    if (bd == BD_OPEN) return;
    const double Rx = rx + 1.0, Rz = rz + 1.0;
    const int N = 10;
    for (int k = 0; k < N; k++) {
        const double a0 = (k + 0.08) * TAU / N, a1 = (k + 0.92) * TAU / N, am = 0.5 * (a0 + a1);
        if (std::fabs(std::remainder(am - doorA, TAU)) < 0.45) continue;   // its gap at the doorway
        const double x0 = std::sin(a0) * Rx, z0 = std::cos(a0) * Rz, x1 = std::sin(a1) * Rx, z1 = std::cos(a1) * Rz;
        const double cx = 0.5 * (x0 + x1), cz = 0.5 * (z0 + z1), len = std::hypot(x1 - x0, z1 - z0), dh = std::atan2(x1 - x0, z1 - z0);
        switch (bd) {
            case BD_WALLS: { const double D = g.D(MT_DRYSTONE); g.box(cx, cz, dh, 0.25, len * 0.5, 0, std::max(0.25, 0.75 * wallShare(D, g.U(640 + k))), 0, MT_DRYSTONE, 3); break; }
            case BD_FENCES: { const double D = g.D(MT_TIMBER); if (D < 0.8) { Form& e = g.add(FK_FENCE, 2, cx, cz, dh, 0.05, len * 0.5, 0, 0.85, g.timberSlot(), MT_TIMBER); e.broken = D < 0.25 ? 0 : clampd(D, 0.3, 0.9); } break; }
            case BD_HEDGES: if (!g.st.desert) g.box(cx, cz, dh, 0.35, len * 0.5, 0, 0.8, 0, -1, 11); break;
            default: { const double D = g.D(MT_TIMBER); if (D < 0.7) { Form& e = g.add(FK_FENCE, 0, cx, cz, dh, 0.06, len * 0.5, 0, 0.85, g.timberSlot(), MT_TIMBER); e.p[0] = 0.24; e.p[1] = 0.8; e.p[3] = 0.12; e.broken = D < 0.25 ? 0 : clampd(D, 0.3, 0.9); } break; }   // a ring of stakes
        }
    }
}
// R-412: a hut's entrance (`GV_ENTRANCE`): a porch passage of two short walls or rows of posts and a lintel, a threshold of stones, two
// portal posts, a hood of thatch
void hutEntrance(Gen& g, double rx, double rz, double doorA, double h, bool stone) {
    if (g.lod != 0) return;
    const double dx = std::sin(doorA), dz = std::cos(doorA), r = std::sqrt(rx * rx * dx * dx + rz * rz * dz * dz);
    const double heading = doorA, Dt = g.D(MT_TIMBER);
    auto at = [&](double across, double out, double& x, double& z) { x = dx * (r + out) + dz * across; z = dz * (r + out) - dx * across; };
    switch (g.gv(GV_ENTRANCE)) {
        case EN_PORCH: {   // a passage out from the doorway: its two cheeks and the lintel over it
            double x, z;
            for (int s = -1; s <= 1; s += 2) {   // as wide apart as the doorway
                at(s * 1.17, 0.7, x, z);
                if (stone) g.box(x, z, heading, 0.22, 0.75, 0, std::max(0.4, h * 0.85 * wallShare(g.D(MT_DRYSTONE), g.U(650 + (s > 0)))), 0, MT_DRYSTONE);
                else { at(s * 1.1, 1.3, x, z); g.post(x, z, h * 0.95, 0.11, 652 + (s > 0)); }
            }
            if (Dt < 0.35) { at(0, 0.75, x, z); Form& t = g.add(FK_SWEEP, 1, x, z, heading, 0, 0.8, h * 0.9, h * 0.9 + 0.75 * g.pitchK(), g.roofSlot(MT_TIMBER), MT_TIMBER, 2); t.p[1] = 1.5; t.p[2] = 0.05; }
            return;
        }
        case EN_STEPS: { double x, z; at(0, 0.45, x, z); g.box(x, z, heading, 0.9, 0.42, 0, 0.16, 0, MT_DRYSTONE, 4); return; }   // a threshold slab
        case EN_PORTICO: { double x, z; for (int s = -1; s <= 1; s += 2) { at(s * 1.1, 0.6, x, z); g.post(x, z, h * 1.6, 0.15, 656 + (s > 0)); } return; }   // two tall portal posts
        case EN_CANOPY: { if (Dt < 0.4) { double x, z; at(0, 0.5, x, z); Form& c = g.box(x, z, heading, 1.1, 0.55, h * 0.95, h * 0.95 + 0.14, g.roofSlot(MT_TIMBER), MT_TIMBER, 2); c.tilt = 0.28; } return; }
        default: return;
    }
}

// R-412: a futuristic people's pod's dress: what stands out from its ring (a ring gallery round it over its doorway's head, balconies,
// blisters of glass at its foot, oriels up its wall) and its doorway's (a tunnel out from it, a low step, two pylons, a shell over it)
void podDress(Gen& g, double rx, double rz, double h, double hmin, int segs, int door, int doorSegs) {
    if (g.lod > 1 || g.c.age != AGE_FUTURISTIC) return;
    const double Dc = g.D(MT_COMPOSITE);
    if (Dc > 0.6) return;
    const double doorA = (door + 0.5 * doorSegs) * TAU / segs;
    const bool whole = hmin >= 0.92 * h;   // what stands out from its wall only while its wall stands
    // its wall's spots: a round ring's anywhere round it, a polygon's at its sides' middles (out from them), none in its doorway
    std::vector<std::pair<double, double>> wallAt;   // the angle, the wall's distance from the middle there
    const bool round = segs >= 10;
    if (round) { for (int k = 0; k < 4; k++) { const double a = doorA + PI / 4 + k * PI / 2; wallAt.push_back({a, std::hypot(std::sin(a) * rx, std::cos(a) * rz)}); } }
    else for (int k = 0; k < segs; k++) {
        if (((k - door) % segs + segs) % segs < doorSegs) continue;
        wallAt.push_back({(k + 0.5) * TAU / segs, rx * std::cos(PI / segs)});
    }
    auto spot = [&](size_t i, double out, double& x, double& z) { x = std::sin(wallAt[i].first) * (wallAt[i].second + out); z = std::cos(wallAt[i].first) * (wallAt[i].second + out); };
    switch (whole ? g.gv(GV_PROJECTIONS) : PJ_NONE) {
        case PJ_BALCONIES: if (g.lod == 0) for (size_t i = 0; i < wallAt.size(); i++) { double x, z; spot(i, 0.4, x, z); Form& e = g.lathe(0, x, z, 1.3, h * 0.62, h * 0.62 + 0.22, 0, MT_COMPOSITE); e.part = 2; } break;
        case PJ_GALLERIES:
            if (round) { Form& t = g.add(FK_TORUS, 0, 0, 0, 0, std::max(rx, rz) + 0.8, std::max(rx, rz) + 0.8, h - 0.75, h - 0.15, 2, MT_COMPOSITE); t.p[0] = 0.3; }
            else for (size_t i = 0; i < wallAt.size(); i++) { double x, z; spot(i, 0.7, x, z); const double len = rx * std::sin(PI / segs); g.box(x, z, wallAt[i].first + PI / 2, len * 0.9, 0.7, h * 0.62 - 0.2, h * 0.62, 2, MT_COMPOSITE, 2); }
            break;
        case PJ_BAYS: for (size_t i = 0; i < wallAt.size() && i < 3; i++) { double x, z; spot(i, -0.6, x, z); g.add(FK_DOME, 1, x, z, 0, 2.1, 2.1, 0, 1.6, 2, MT_COMPOSITE, 2); } break;
        case PJ_ORIELS: for (size_t i = 0; i < wallAt.size() && i < 3; i++) { double x, z; spot(i, 0.1, x, z); Form& e = g.lathe(0, x, z, 0.95, h * 0.45, h + 0.5, 0, MT_COMPOSITE); e.pattern = 2; } break;
        default: break;
    }
    if (g.lod != 0) return;
    const double dx = std::sin(doorA), dz = std::cos(doorA), r = round ? std::sqrt(rx * rx * dx * dx + rz * rz * dz * dz) : rx * std::cos(PI / segs);
    auto at = [&](double across, double out, double& x, double& z) { x = dx * (r + out) + dz * across; z = dz * (r + out) - dx * across; };
    double x, z;
    switch (g.gv(GV_ENTRANCE)) {
        case EN_PORCH: { at(0, 1.0, x, z); Form& v = g.add(FK_ARCH, 0, x, z, doorA, 1.7, 1.0, 0, 3.1, 2, MT_COMPOSITE); v.p[0] = 1; v.p[1] = 0.32; v.p[2] = 2.1; break; }   // a deep arch out from its doorway
        case EN_STEPS: at(0, 1.0, x, z); g.box(x, z, doorA, 1.4, 1.0, 0, 0.15, 0, MT_COMPOSITE, 4); break;
        case EN_PORTICO: for (int s = -1; s <= 1; s += 2) { at(s * 1.6, 0.8, x, z); g.lathe(1, x, z, 0.3, 0, h + 1.8, 0, MT_COMPOSITE); } break;
        case EN_CANOPY: { at(0, 0.8, x, z); Form& s = g.add(FK_SHELL, 0, x, z, doorA, 1.7, 1.3, 2.7, 3.4, 2, MT_COMPOSITE, 2); (void)s; break; }
        default: break;
    }
}

// R-412: the people's street furniture (`BK_FURNITURE`, its style the `Furniture`): a well (a medieval people's curb under its little
// roof, an industrial people's iron pump, a modern people's drinking fountain, a futuristic people's column of water), a lamp (a lantern on
// its post, an iron lamp post, a street light on its arm, a pylon of light), a fountain's basin and its bowls, a figure on its plinth (a
// modern people's abstract one, a futuristic people's ring)
void genFurniture(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    if (g.lod == 2) return;
    const int age = c.age;
    const int stone = age == AGE_MODERN ? MT_CONCRETE : (age >= AGE_FUTURISTIC ? MT_COMPOSITE : (age == AGE_INDUSTRIAL ? MT_MASONRY : stoneOf(c)));
    const double Ds = g.D(stone), Di = g.D(MT_IRON), Dt = g.D(MT_TIMBER);
    switch (b.style) {
        case FU_WELLS:
            if (age == AGE_MEDIEVAL) {
                { Form& e = g.lathe(0, 0, 0, 0.85, 0, 0.8 * wallShare(Ds, g.U(10)), 0, stone, 10); e.p[3] = 1; }
                for (int s = -1; s <= 1; s += 2) g.post(s * 0.75, 0, 2.3, 0.08, 20 + (s > 0));
                if (Dt < 0.3 && g.lod == 0) { g.box(0, 0, 0, 0.85, 0.06, 2.05, 2.17, g.timberSlot(), MT_TIMBER, 4); Form& r = g.add(FK_SWEEP, 1, 0, 0, PI / 2, 0, 0.95, 2.2, 2.85, g.roofSlot(MT_TIMBER), MT_TIMBER, 2); r.p[1] = 0.85; r.p[2] = 0.05; }
            } else if (age == AGE_INDUSTRIAL) {
                if (Di > 0.7) return;
                g.lathe(0, 0, 0, 0.13, 0, 1.45, g.ironSlot(), MT_IRON, 8);
                g.add(FK_DOME, 0, 0, 0, 0, 0.17, 0.17, 1.45, 1.62, g.ironSlot(), MT_IRON);
                if (g.lod == 0) { g.box(0, 0.2, 0, 0.04, 0.2, 1.0, 1.08, g.ironSlot(), MT_IRON, 4); Form& h = g.box(0, -0.25, 0, 0.03, 0.3, 1.3, 1.36, g.ironSlot(), MT_IRON, 4); h.tilt = 0.35; g.box(0, 0.55, 0, 0.35, 0.25, 0, 0.35, 0, MT_MASONRY, 4); }
            } else if (age == AGE_MODERN) {
                if (Ds > 0.7) return;
                g.box(0, 0, 0, 0.18, 0.18, 0, 0.92, 0, MT_CONCRETE, 4); g.box(0, 0.1, 0, 0.3, 0.25, 0.85, 1.0, 0, MT_CONCRETE, 4);
            } else { Form& e = g.lathe(0, 0, 0, 0.4, 0, 2.3, 2, MT_COMPOSITE); (void)e; }
            return;
        case FU_LAMPS:
            switch (age) {
                case AGE_MEDIEVAL:
                    g.post(0, 0, 2.8, 0.09, 30);
                    if (Dt < 0.35 && g.lod == 0) { g.box(0, 0.22, 0, 0.05, 0.24, 2.6, 2.68, g.timberSlot(), MT_TIMBER, 4); g.box(0, 0.34, 0, 0.13, 0.13, 2.2, 2.55, g.ironSlot(), MT_IRON, 4); }   // its bracket, its lantern under it
                    return;
                case AGE_INDUSTRIAL: {
                    if (Di > 0.7) return;
                    const double h = 3.9 * (Di < 0.35 ? 1.0 : 1 - Di);
                    g.lathe(0, 0, 0, 0.17, 0, 0.6, g.ironSlot(), MT_IRON, 8);
                    g.lathe(0, 0, 0, 0.07, 0.6, h, g.ironSlot(), MT_IRON, 6);
                    if (Di < 0.35 && g.lod == 0) { Form& l = g.lathe(1, 0, 0, 0.13, h, h + 0.48, g.ironSlot(), MT_IRON, 4); l.p[0] = 1.7; Form& t = g.lathe(1, 0, 0, 0.26, h + 0.48, h + 0.72, g.ironSlot(), MT_IRON, 4); t.p[0] = 0.1; g.box(0, 0, 0, 0.36, 0.03, h - 0.5, h - 0.44, g.ironSlot(), MT_IRON, 4); }
                    return;
                }
                case AGE_MODERN: {
                    if (Di > 0.75) return;
                    const double h = 7.5 * (Di < 0.4 ? 1.0 : 1.2 - Di);
                    g.lathe(1, 0, 0, 0.12, 0, h, g.ironSlot(), MT_IRON, 8).p[0] = 0.6;
                    if (Di < 0.4 && g.lod == 0) { g.box(0, 0.7, 0, 0.05, 0.75, h - 0.3, h, g.ironSlot(), MT_IRON, 4); g.box(0, 1.35, 0, 0.16, 0.3, h - 0.24, h - 0.04, g.ironSlot(), MT_IRON, 4); }   // its arm, its lamp under the arm's end
                    return;
                }
                default: {
                    g.lathe(1, 0, 0, 0.16, 0, 5.6, 2, MT_COMPOSITE, 6).p[0] = 0.5;
                    Form& t = g.add(FK_TORUS, 0, 0, 0, 0, 0.45, 0.45, 5.5, 5.62, 3, MT_COMPOSITE); t.p[0] = 0.06;
                    return;
                }
            }
        case FU_FOUNTAINS: {
            const double R = age == AGE_MODERN ? 3.0 : (age >= AGE_FUTURISTIC ? 2.6 : 2.2);
            if (age >= AGE_FUTURISTIC) { Form& t = g.add(FK_TORUS, 0, 0, 0, 0, R, R, 0, 0.5, 2, MT_COMPOSITE); t.p[0] = 0.25; g.lathe(0, 0, 0, 0.35, 0, 3.0, 2, MT_COMPOSITE); return; }
            { Form& e = g.lathe(0, 0, 0, R, 0, (age == AGE_MODERN ? 0.45 : 0.6) * wallShare(Ds, g.U(40)), 0, stone, 16); e.p[3] = 1; }
            if (age == AGE_MODERN) { if (Ds < 0.5) g.box(0, 0, 0.6, 0.3, 0.9, 0, 1.6, 0, MT_CONCRETE); return; }
            if (Ds > 0.55) return;
            g.lathe(0, 0, 0, 0.24, 0, 1.6, 0, stone, 8);
            { Form& w = g.lathe(1, 0, 0, 0.3, 1.6, 1.9, 0, stone, 12); w.p[0] = 2.7; }
            if (age == AGE_INDUSTRIAL && g.lod == 0) { g.lathe(0, 0, 0, 0.14, 1.9, 2.5, g.ironSlot(), MT_IRON, 8); Form& w2 = g.lathe(1, 0, 0, 0.16, 2.5, 2.7, g.ironSlot(), MT_IRON, 8); w2.p[0] = 2.8; }
            return;
        }
        default: {   // a figure on its plinth
            if (age >= AGE_FUTURISTIC) { g.lathe(0, 0, 0, 0.6, 0, 0.6, 0, MT_COMPOSITE); Form& t = g.add(FK_TORUS, 0, 0, 0, 0, 1.4, 1.4, 1.2, 4.24, 3, MT_COMPOSITE); t.p[0] = 0.12; t.tilt = PI / 2; return; }
            if (age == AGE_MODERN) {
                g.box(0, 0, 0, 0.9, 0.6, 0, 0.5, 0, MT_CONCRETE);
                if (Ds < 0.5) { Form& e = g.box(0, 0, 0.4, 0.22, 0.5, 0.5, 3.4, g.ironSlot(), MT_IRON); e.tilt = 0.22; }
                return;
            }
            g.box(0, 0, 0, 0.55, 0.55, 0, 1.5 * wallShare(Ds, g.U(50)), 0, stone);
            if (Ds < 0.45) figure(g, 0, 0, 1.5, 2.3, age == AGE_INDUSTRIAL ? g.ironSlot() : 0, age == AGE_INDUSTRIAL ? MT_IRON : stone, Ds);
            return;
        }
    }
}

// a rectangular body: walls, its roof (split into cells where the people built in cells), its upper storey (a stepped people's), its
// tops; the room's walls `W` already set. Returns the walls' lowest height
double rectBody(Gen& g, Walls W, int rf, int roofMat, double hs[4], bool cells) {
    W.crenels = W.dress && g.gv(GV_TOPS) == TP_CRENELS && (rf == RF_FLAT || rf == RF_OPEN);
    const double hmin = walls4(g, W, hs);
    const bool stepped = W.dress && g.gv(GV_MASSING) == MS_STEPPED && hmin >= 0.97 * W.h && g.lod < 2 && W.hw > 2.2 && W.hd > 2.2;
    const int mainRoof = stepped ? RF_FLAT : rf;
    const double longL = std::max(W.hw, W.hd);
    const int n = cells && W.dress && g.gv(GV_MODULAR) == 1 && longL > 3.4 && mainRoof != RF_FLAT && mainRoof != RF_OPEN ? clampi((int)std::lround(longL / 2.8), 2, 4) : 1;
    double rise = 0;
    if (n == 1) {
        rise = roof(g, mainRoof, W.cx, W.cz, W.hw, W.hd, hmin, W.h, 0.8, mainRoof == RF_FLAT && !stepped ? roofMat : (stepped ? W.mat : roofMat));
        if (mainRoof == RF_PITCHED) gables(g, W, hs, 0.8);   // B-415
    } else {   // a roof a cell: gables across the long side, a row of them
        const bool alongZ = W.hd >= W.hw;
        const int rl = g.c.age >= AGE_MEDIEVAL && g.c.age <= AGE_INDUSTRIAL && g.lod == 0 && dwellingKind(g.b.kind) ? g.gv(GV_ROOFLINE) : RL_PLAIN;
        for (int k = 0; k < n; k++) {
            const double t = -longL + (k + 0.5) * 2 * longL / n, half = longL / n;
            const double cxk = W.cx + (alongZ ? 0 : t), czk = W.cz + (alongZ ? t : 0);
            const double rk = roof(g, mainRoof, cxk, czk, alongZ ? W.hw : half, alongZ ? half : W.hd, hmin, W.h, 0.8, roofMat);
            // R-412: the cells' roofline: cresting along each cell's ridge, a lantern on the middle one's
            if (rk > 0.8 && g.D(roofMat) < 0.25 && (rl == RL_CRESTED || (rl == RL_LANTERN && k == n / 2))) {
                const double yTop = g.sink + W.y0 + hmin - 0.05 + rk;
                const bool cellZ = (alongZ ? half : W.hd) >= (alongZ ? W.hw : half);   // the cell's own ridge's run
                if (rl == RL_CRESTED) { Form& e = g.add(FK_FENCE, g.c.age == AGE_INDUSTRIAL ? 1 : 0, cxk, czk, cellZ ? 0 : PI / 2, 0.03, std::max(0.4, (cellZ ? (alongZ ? half : W.hd) : (alongZ ? W.hw : half)) - 0.2), yTop - 0.08, yTop + 0.4, g.roofSlot(roofMat), roofMat, 4); e.p[0] = 0.3; e.p[1] = 0.5; e.p[3] = 0.13; }
                else { g.lathe(0, cxk, czk, 0.45, yTop - 0.4, yTop + 0.9, wallsSlot(g, W), W.mat, 6); Form& s2 = g.lathe(1, cxk, czk, 0.58, yTop + 0.9, yTop + 0.9 + 1.2 * g.pitchK(), g.roofSlot(roofMat), roofMat, 6); s2.p[0] = 0.04; }
            }
        }
    }
    if (stepped) {   // the upper storey set back on the walls, its own roof
        Walls U = W; U.hw = W.hw * 0.62; U.hd = W.hd * 0.62; U.h = W.h * 0.75; U.y0 = W.y0 + hmin; U.door = -1; U.window = false; U.glyphs = false; U.whole = false; U.k0 = W.k0 + 40; U.crenels = false;
        double hu[4]; const double hmu = walls4(g, U, hu);
        const double riseU = roof(g, rf, U.cx, U.cz, U.hw, U.hd, hmu, U.h, 0.8, roofMat);
        if (rf == RF_PITCHED) gables(g, U, hu, 0.8);
        houseTops(g, rf, U.cx, U.cz, U.hw, U.hd, g.sink + U.y0 + hmu, riseU, roofMat, W.mat);
    } else if (hmin >= 0.92 * W.h) houseTops(g, n > 1 ? RF_FLAT : rf, W.cx, W.cz, W.hw, W.hd, g.sink + W.y0 + hmin, rise, roofMat, W.mat);
    houseDress(g, W, hs, hmin, mainRoof, rise, mainRoof == RF_FLAT && !stepped ? roofMat : roofMat, !stepped && n == 1);   // R-412
    return hmin;
}

// C-01's house, hall and tower; A-04: in the people's plan (a rectangle, a ring of its polygon, an oval, an L, a court), roof, profile,
// dress and tops, on its foundation
void genKeep(Gen& g);
void genMarketHall(Gen& g);
void genPalace(Gen& g);
void genHouse(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    if (b.kind == BK_TOWER) { genKeep(g); return; }
    if (b.kind == BK_HALL && b.style == 1) { genMarketHall(g); return; }
    if (b.kind == BK_HALL && b.style == 2) { genPalace(g); return; }
    const int mat = b.kind == BK_HOUSE ? houseMaterial(c) : stoneOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    if (b.found == FD_PIT) g.sink -= std::min(1.2, 0.35 * b.hWall);
    if (g.gv(GV_MASSING) == MS_BURIED) g.sink -= 0.4 * b.hWall;
    double y0 = 0;
    if (b.found == FD_STILTS) y0 = stilts(g, b.hw, b.hd, 2.2) ? 2.2 : 0;
    const int rf = ageRoof(c, houseRoof(g)), roofMat = roofMaterial(g, rf, mat);   // B-415: an industrial people's warehouse under the age's roof
    const int doorSide = b.form == PF_FREE || b.form == PF_COURT ? 0 : b.door;
    double doorPos = 0;
    double hs[4] = {b.hWall, b.hWall, b.hWall, b.hWall};
    switch (b.form) {
        case PF_CIRCLE: case PF_POLYGON: case PF_ELLIPSE: {
            Ring R; R.rx = b.hw; R.rz = b.form == PF_ELLIPSE ? b.hd : b.hw; R.h = b.hWall; R.thick = b.thick; R.mat = mat; R.slot = wallSlot(g, mat); R.y0 = y0;
            R.segs = ringSegs(b.form, b.sides, std::min(R.rx, R.rz)); R.doorSegs = ringDoorSegs(std::min(R.rx, R.rz), R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs); R.pattern = 1;
            R.crenels = g.gv(GV_TOPS) == TP_CRENELS && (rf == RF_FLAT || rf == RF_OPEN);
            const double hmin = ringWalls(g, R);
            const double rise = roundRoof(g, rf, 0, 0, R.rx + b.thick * 0.5, R.rz + b.thick * 0.5, R.segs, hmin, b.hWall, 0.8, roofMat);
            if (hmin >= 0.92 * b.hWall) roundTops(g, rf, g.sink + y0 + hmin, rise, 1.0, roofMat);
            foundation(g, R.rx, R.rz, 0, 0, mat);
            ringDress(g, R, hmin);   // R-412
            yardOf(g, R.rx, R.rz, -1, 0, mat);
            drifts(g, R.rx, R.rz, Dm);
            return;
        }
        case PF_FREE: {   // an L: the front wing with the doorway, a back wing on one side
            Walls W; W.cz = b.hd * 0.44; W.hw = b.hw; W.hd = b.hd * 0.56; W.h = b.hWall; W.thick = b.thick; W.door = 0; W.mat = mat; W.slot = wallSlot(g, mat); W.y0 = y0;
            W.doorPos = doorPos = houseDoorPos(b, b.hw); W.glyphs = true; W.pattern = 1;
            const double hmin = rectBody(g, W, rf, roofMat, hs, true);
            Walls V = W; V.cx = (g.U(43) < 0.5 ? -1 : 1) * b.hw * 0.5; V.cz = -b.hd * 0.57; V.hw = b.hw * 0.5; V.hd = b.hd * 0.43; V.h = b.hWall * (0.8 + 0.2 * g.U(44)); V.door = -1; V.glyphs = false; V.k0 = 30;
            double hv[4]; rectBody(g, V, rf, roofMat, hv, false);
            (void)hmin;
            break;
        }
        case PF_COURT: {   // four ranges round an open court, the doorway through the front range and on into the court
            Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = b.thick; W.door = 0; W.mat = mat; W.slot = wallSlot(g, mat); W.y0 = y0;
            W.doorPos = doorPos = houseDoorPos(b, b.hw - 3.6); W.glyphs = true; W.pattern = 1;
            W.crenels = g.gv(GV_TOPS) == TP_CRENELS;
            const double hmin = walls4(g, W, hs);
            Walls I = W; I.hw = b.hw - 3.4; I.hd = b.hd - 3.4; I.glyphs = false; I.k0 = 30; I.crenels = false; I.pattern = 1;
            double hi[4]; const double him = walls4(g, I, hi);
            const double hr = std::min(hmin, him);
            const int rr = rf == RF_PITCHED || rf == RF_HIPPED || rf == RF_CONICAL || rf == RF_SADDLE ? RF_PITCHED : (rf == RF_VAULTED ? RF_VAULTED : RF_FLAT);
            const int rm = roofMaterial(g, rr, mat);
            roof(g, rr, 0, b.hd - 1.7, b.hw, 1.7, hr, b.hWall, 0.7, rm);
            roof(g, rr, 0, -b.hd + 1.7, b.hw, 1.7, hr, b.hWall, 0.7, rm);
            roof(g, rr, b.hw - 1.7, 0, 1.7, b.hd - 3.4, hr, b.hWall, 0.7, rm);
            roof(g, rr, -b.hw + 1.7, 0, 1.7, b.hd - 3.4, hr, b.hWall, 0.7, rm);
            if (hr >= 0.92 * b.hWall) houseTops(g, RF_FLAT, 0, 0, b.hw, b.hd, g.sink + y0 + hr, 0, rm, mat);
            break;
        }
        default: {   // C-01's rectangle: the doorway on its side, the window opposite, an inner wall now and then, the columns of a hall
            Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = b.thick; W.door = b.door; W.mat = mat; W.slot = wallSlot(g, mat); W.y0 = y0;
            const double doorL = (b.door == 0 || b.door == 2) ? b.hw : b.hd; W.doorPos = doorPos = houseDoorPos(b, doorL);
            const bool works = c.age >= AGE_INDUSTRIAL;   // B-415: an industrial people's warehouse: its tall windows, its parapet, its iron columns
            W.window = !works; W.glyphs = !works; W.pattern = works ? 2 : 1; W.parapet = parapetRoof(c) ? 0.8 : 0;
            const double hmin = rectBody(g, W, b.kind == BK_HALL && rf == RF_FLAT ? RF_FLAT : rf, roofMat, hs, b.kind == BK_HOUSE);
            if (b.kind == BK_HOUSE && g.lod == 0 && g.U(30) < 0.45) {   // an inner wall from the wall opposite the door, three fifths of the way in, beside the
                double cx, cz, half, dh;                                    // centre line (the room's middle stays clear, the doorway too)
                if (b.door == 0 || b.door == 2) { double zs = b.door == 0 ? -1 : 1; cx = 0.5 * b.hw; cz = zs * 0.7 * b.hd; half = 0.3 * b.hd; dh = PI / 2; }
                else { double xs = b.door == 1 ? -1 : 1; cx = xs * 0.7 * b.hw; cz = 0.5 * b.hd; half = 0.3 * b.hw; dh = 0; }
                g.box(cx, cz, dh, half, b.thick * 0.4, g.sink + y0, g.sink + y0 + std::max(0.35, hmin * 0.9), wallSlot(g, mat), mat);
            }
            if (b.kind == BK_HALL && g.lod == 0) {   // two rows of columns inside, some fallen
                int k = 0;
                const bool round = g.gv(GV_PLAN) == PF_CIRCLE || g.gv(GV_PLAN) == PF_ELLIPSE;
                for (int rowS = -1; rowS <= 1; rowS += 2)
                    for (double tt = -b.hd + 2.0; tt < b.hd - 1.5; tt += 3.0, k++) {
                        if (works) { const double d = g.Dk(MT_IRON, 40 + k); if (d < 0.7) g.lathe(0, rowS * b.hw * 0.5, tt, 0.16, g.sink, g.sink + b.hWall * 0.9 * (d < 0.35 ? 1 : 1 - d), g.ironSlot(), MT_IRON, 6); continue; }
                        double f = 1 - Dm * (0.2 + 0.8 * g.U(40 + k));
                        if (f < 0.2) continue;
                        if (round) g.lathe(0, rowS * b.hw * 0.5, tt, 0.36, g.sink, g.sink + b.hWall * 0.9 * f, 0, mat, 8);
                        else g.box(rowS * b.hw * 0.5, tt, 0, 0.35, 0.35, g.sink, g.sink + b.hWall * 0.9 * f, 0, mat, 1);
                    }
            }
            break;
        }
    }
    double doorX, doorZ; doorMid(doorSide, doorPos, b.hw, b.form == PF_FREE ? b.hd : b.hd, doorX, doorZ);
    rubble(g, b.hw, b.hd, hs, b.hWall, doorX, doorZ, wallSlot(g, mat), mat);
    foundation(g, b.hw, b.hd, doorSide, doorPos, mat);
    yardOf(g, b.hw, b.hd, doorSide, doorPos, mat);   // R-412
    drifts(g, b.hw, b.hd, Dm);
}

// a keep: C-01's tower, its walls round a room in the people's plan (four walls, or a ring of its polygon), slits up them, its top
// crenellated, roofed in the people's way, or crowned with its tops
void genKeep(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = stoneOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    const int rf = c.roof, tp = g.gv(GV_TOPS);
    const bool flatTop = rf == RF_FLAT || rf == RF_OPEN || rf == RF_STEPPED || tp == TP_CRENELS;
    double hmin, hs[4] = {b.hWall, b.hWall, b.hWall, b.hWall}, keepRise = 0;
    if (b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE) {
        Ring R; R.rx = b.hw; R.rz = b.form == PF_ELLIPSE ? b.hd : b.hw; R.h = b.hWall; R.thick = std::max(b.thick, 0.7); R.mat = mat; R.pattern = 2;
        R.segs = ringSegs(b.form, b.sides, std::min(R.rx, R.rz)); R.doorSegs = ringDoorSegs(std::min(R.rx, R.rz), R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs);
        R.crenels = flatTop;
        hmin = ringWalls(g, R);
        if (!flatTop) keepRise = roundRoof(g, rf == RF_PITCHED || rf == RF_HIPPED ? RF_CONICAL : rf, 0, 0, R.rx + R.thick * 0.5, R.rz + R.thick * 0.5, R.segs, hmin, b.hWall, 1.0, roofMaterial(g, rf, mat));
    } else {
        Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = b.thick; W.door = b.door; W.mat = mat; W.pattern = 2;
        const double doorL = (b.door == 0 || b.door == 2) ? b.hw : b.hd; W.doorPos = houseDoorPos(b, doorL); W.glyphs = true; W.crenels = flatTop;
        hmin = walls4(g, W, hs);
        if (!flatTop) keepRise = roof(g, rf == RF_PITCHED ? RF_CONICAL : rf, 0, 0, b.hw, b.hd, hmin, b.hWall, 1.0, roofMaterial(g, rf, mat));
        double doorX, doorZ; doorMid(b.door, W.doorPos, b.hw, b.hd, doorX, doorZ);
        rubble(g, b.hw, b.hd, hs, b.hWall, doorX, doorZ, 0, mat);
    }
    if (hmin >= 0.97 * b.hWall && (tp == TP_FINIALS || tp == TP_STATUES || tp == TP_MASTS)) {
        const bool ring = b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE;
        if (flatTop) { const double q = ring ? 0.7 : 0.92; topsAt(g, {{-b.hw * q, b.hd * q}, {b.hw * q, -b.hd * q}}, g.sink + hmin + 0.8, 1.0, mat); }   // on the merlons at two corners (a ring's at its diagonals)
        else {   // on the roof's crown, the roof's own, while it stands
            const int rd = ring ? (rf == RF_PITCHED || rf == RF_HIPPED ? RF_CONICAL : rf) : (rf == RF_PITCHED ? RF_CONICAL : rf);
            if (keepRise > 0) roundTops(g, rd, g.sink + hmin, keepRise, 1.0, roofMaterial(g, rf, mat));
        }
    }
    drifts(g, b.hw, b.hd, Dm);
}

// A-04: a market hall: columns in rows under one roof, open on every side; the roof only while every column stands
void genMarketHall(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    const int n = clampi(c.genome.number() + 1, 4, 8), rows = 2;
    const bool round = g.gv(GV_PLAN) == PF_CIRCLE || g.gv(GV_PLAN) == PF_ELLIPSE;
    const double H = b.hWall;
    bool all = true;
    for (int r = 0; r < rows; r++)
        for (int k = 0; k < n; k++) {
            const double x = (r ? 1 : -1) * (b.hw - 0.5), z = -b.hd + 0.5 + (2 * b.hd - 1) * k / (n - 1);
            const double f = std::max(0.3, wallShare(Dm, g.U(40 + r * 16 + k)));
            if (f < 0.97) all = false;
            if (g.lod == 2 && k % 2) continue;
            if (round) g.lathe(0, x, z, 0.38, g.sink, g.sink + H * f, 0, mat, 8);
            else g.box(x, z, 0, 0.36, 0.36, g.sink, g.sink + H * f, 0, mat, 1);
        }
    if (all) {
        const int rf = c.roof == RF_OPEN ? RF_FLAT : c.roof;
        const double rise = roof(g, rf, 0, 0, b.hw, b.hd, H, H, 0.6, roofMaterial(g, rf, mat));
        houseTops(g, rf, 0, 0, b.hw, b.hd, g.sink + H, rise, roofMaterial(g, rf, mat), mat);
    }
}

// A-04: a palace: ranges round a court, a tower at each corner in the people's section and profile, its doorway in the front range
void genPalace(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = std::max(b.thick, 0.7); W.door = 0; W.doorPos = 0; W.mat = mat; W.glyphs = true; W.pattern = 3;
    W.crenels = g.gv(GV_TOPS) == TP_CRENELS;
    double hs[4]; const double hmin = walls4(g, W, hs);
    Walls I = W; I.hw = b.hw - 4.0; I.hd = b.hd - 4.0; I.glyphs = false; I.k0 = 30; I.crenels = false;
    double hi[4]; const double him = walls4(g, I, hi);
    const double hr = std::min(hmin, him);
    const int rf = c.roof == RF_PITCHED || c.roof == RF_HIPPED || c.roof == RF_CONICAL || c.roof == RF_SADDLE ? RF_PITCHED : RF_FLAT, rm = roofMaterial(g, rf, mat);
    roof(g, rf, 0, b.hd - 2.0, b.hw, 2.0, hr, b.hWall, 0.6, rm); roof(g, rf, 0, -b.hd + 2.0, b.hw, 2.0, hr, b.hWall, 0.6, rm);
    roof(g, rf, b.hw - 2.0, 0, 2.0, b.hd - 4.0, hr, b.hWall, 0.6, rm); roof(g, rf, -b.hw + 2.0, 0, 2.0, b.hd - 4.0, hr, b.hWall, 0.6, rm);
    const double tr = 2.6 + 0.04 * b.hw, tH = b.hWall * (1.5 + 0.8 * g.gf(GV_HEIGHT));
    for (int q = 0; q < 4; q++) {
        const double x = (q % 2 ? 1 : -1) * (b.hw - tr * 0.6), z = (q < 2 ? 1 : -1) * (b.hd - tr * 0.6);
        TowerOut T = towerShaft(g, x, z, tr, tH, mat, 50 + q * 3, 2);
        if (T.whole) towerTop(g, x, z, T.rTop, T.h, tH, mat, g.gv(GV_FOCUS) == FO_SPIRE, tH * 0.6);
    }
    double dx, dz; doorMid(0, 0, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
    drifts(g, b.hw, b.hd, Dm);
}

void genRotunda(Gen& g) {   // C-01's: a ring of wall, its doorway a segment left out, its roof; A-04: the people's ring and roof; a pod the futuristic age's
    const Building& b = g.b; const Culture& c = g.c;
    const bool pod = b.kind == BK_POD;
    const int mat = pod ? MT_COMPOSITE : stoneOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    Ring R; R.rx = b.hw; R.rz = b.form == PF_ELLIPSE ? b.hd : b.hw; R.h = b.hWall; R.thick = b.thick; R.mat = mat; R.pattern = pod ? 2 : 1;
    R.segs = ringSegs(b.form == PF_RECT || b.form == PF_FREE || b.form == PF_COURT ? PF_CIRCLE : b.form, b.sides, std::min(R.rx, R.rz));
    R.doorSegs = ringDoorSegs(std::min(R.rx, R.rz), R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs);
    const int rf = pod && (c.roof == RF_PITCHED || c.roof == RF_HIPPED) ? RF_DOMED : c.roof;
    R.crenels = !pod && g.gv(GV_TOPS) == TP_CRENELS && (rf == RF_FLAT || rf == RF_OPEN);
    const double hmin = ringWalls(g, R);
    if (hmin < b.hWall * 0.7 && !(g.lod == 2)) { drifts(g, R.rx, R.rz, Dm); return; }
    const int rm = pod ? MT_COMPOSITE : roofMaterial(g, rf, mat);
    double rise = roundRoof(g, rf, 0, 0, R.rx + b.thick * 0.5, R.rz + b.thick * 0.5, R.segs, hmin, b.hWall, 0.8, rm);
    if (pod && rf == RF_DOMED) { for (Form& e : g.out) if (e.building == g.bi && e.kind == FK_DOME) { e.slot = 2; e.sub = 1; e.y1 = e.y0 + (e.hx) * 0.55; rise = e.y1 - (g.sink + hmin); } }   // a pod's saucer of glass
    if (hmin >= 0.92 * b.hWall) roundTops(g, rf, g.sink + hmin, rise, 1.0, rm);
    if (pod) podDress(g, R.rx, R.rz, b.hWall, hmin, R.segs, R.door, R.doorSegs);   // R-412
    drifts(g, R.rx, R.rz, Dm);
}

void genGate(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    if (b.age == AGE_PREHISTORIC) {   // two great posts in the palisade's gap
        g.post(-b.hw, 0, b.hWall + 1.0, 0.3, 10); g.post(b.hw, 0, b.hWall + 1.0, 0.3, 11);
        return;
    }
    const int mat = worksOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    if (b.hWall < 3.5) {   // C-01's gate: two piers and the lintel across them
        double h = b.hWall * (1 - 0.5 * Dm), f0 = 1 - Dm * g.U(10), f1 = 1 - Dm * g.U(11);
        g.box(-b.hw, 0, 0, 0.5, 0.5, g.sink, g.sink + std::max(0.5, h * f0), 0, mat, 1);
        g.box(b.hw, 0, 0, 0.5, 0.5, g.sink, g.sink + std::max(0.5, h * f1), 0, mat, 1);
        if (f0 > 0.8 && f1 > 0.8 && g.lod < 2) g.box(0, 0, 0, b.hw + 0.5, 0.5, g.sink + h - 0.7, g.sink + h, 0, mat, 4);
        return;
    }
    // a gatehouse: two towers over the wall (the people's section) and the passage's arch between them (its opening's shape)
    const double h = b.hWall, tw = 1.25;
    const bool round = g.gv(GV_PLAN) == PF_CIRCLE || g.gv(GV_PLAN) == PF_POLYGON || g.gv(GV_PLAN) == PF_ELLIPSE;
    double t0 = h * wallShare(Dm, g.U(10)), t1 = h * wallShare(Dm, g.U(11));
    for (int s = -1; s <= 1; s += 2) {
        const double x = s * (b.hw + tw - 0.1), th = std::max(0.6, s < 0 ? t0 : t1);
        if (round) g.lathe(0, x, 0, 1.5, g.sink, g.sink + th, 0, mat, g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : 0);
        else g.box(x, 0, 0, tw, 1.4, g.sink, g.sink + th, 0, mat);
        if (th >= h * 0.97 && g.lod == 0 && g.gv(GV_TOPS) == TP_CRENELS) for (int k = -1; k <= 1; k += 2) g.box(x + k * 0.6, 0, 0, 0.35, 1.2, g.sink + th, g.sink + th + 0.8, g.topSlot(mat), mat);
    }
    const double archTop = std::min(h - 1.0, b.hw * 1.1 + 3.2);
    const int sub = archSub(g);
    if (std::min(t0, t1) > archTop && g.lod < 2) { Form& e = g.add(FK_ARCH, sub, 0, 0, 0, b.hw + 0.05, 1.0, g.sink, g.sink + archTop + 0.8, g.doorSlot(), mat); e.p[0] = 1; e.p[1] = 0.46; e.p[2] = 3.0; e.broken = Dm < 0.4 ? 0 : clampd(Dm, 0.4, 0.8); }
}

void genWallTower(Gen& g) {   // a tower astride the town wall in the people's section and profile, its top the people's
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = worksOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    TowerOut T = towerShaft(g, 0, 0, b.hw, b.hWall, mat, 10, 2, false);
    if (T.whole) towerTop(g, 0, 0, T.rTop, T.h, b.hWall, mat, false, 0);
}

// C-01's: steps (A-04: the people's number of them), a stair up the front, a shrine on top now and then. B-415: an industrial people's a
// bandstand (its base of brick, iron columns under an iron cone), a modern or a later people's a plinth of two tiers and its mast (A-04's
// stepped platform and shrine in a mill town read as an ancient one's)
void genPlatform(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = worksOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    if (c.age == AGE_INDUSTRIAL) {
        const double rb = std::min(b.hw, b.hd) * 0.75, Di = g.D(MT_IRON);
        { Form& e = g.lathe(0, 0, 0, rb, g.sink, g.sink + 1.0, 0, MT_MASONRY, 8); e.part = 2; }
        if (g.lod == 2) return;
        if (g.lod == 0) g.box(0, rb + 0.6, 0, 1.0, 0.6, 0, 0.5, 0, MT_MASONRY, 2);
        bool all = true;
        for (int k = 0; k < 8; k++) { const double a = (k + 0.5) * TAU / 8, d = g.Dk(MT_IRON, 20 + k); if (d > 0.7) { all = false; continue; } if (d > 0.35) all = false; g.lathe(0, std::sin(a) * rb * 0.85, std::cos(a) * rb * 0.85, 0.1, g.sink + 1.0, g.sink + 1.0 + 3.2 * (d < 0.35 ? 1 : 1 - d), g.ironSlot(), MT_IRON, 6); }
        if (all && Di < 0.4) { Form& r = g.lathe(1, 0, 0, rb * 1.05, g.sink + 4.2, g.sink + 4.2 + rb * 0.6, g.roofSlot(MT_IRON), MT_IRON, 8); r.p[0] = 0.04; }
        return;
    }
    if (c.age >= AGE_MODERN) {
        g.box(0, 0, 0, b.hw, b.hd, g.sink, g.sink + 0.9, 0, mat, 2);
        if (g.lod == 2) return;
        g.box(0, -b.hd * 0.2, 0, b.hw * 0.6, b.hd * 0.5, g.sink + 0.9, g.sink + 1.8, 0, mat, 2);
        if (g.D(MT_IRON) < 0.5) g.lathe(0, 0, -b.hd * 0.2, 0.09, g.sink + 1.8, g.sink + 13, g.ironSlot(), MT_IRON, 6);   // its mast
        return;
    }
    if (g.lod == 2) { g.box(0, 0, 0, b.hw, b.hd, g.sink, g.sink + 3.3, 0, mat, 2); return; }
    const int T = clampi(c.genome.number() - 1, 2, 6); const double th = 3.3 / T;
    for (int i = 0; i < T; i++) g.box(0, 0, 0, b.hw * (1 - 0.84 * i / T), b.hd * (1 - 0.84 * i / T), g.sink + th * i, g.sink + th * (i + 1), 0, mat, 2);
    if (g.lod == 0) for (int k = 0; k < 4; k++) g.box(0, b.hd + 0.35 + 0.7 * (3 - k), 0, 1.6, 0.35, 0, 0.28 * (k + 1), 0, mat, 2);
    if (g.U(7) < 0.5 && Dm < 0.6) { Form& e = g.box(0, 0, 0, 1.3, 1.3, g.sink + 3.3, g.sink + 3.3 + 2.4 * (1 - 0.5 * Dm), 0, mat); e.glyphs = g.lod == 0; if (Dm < 0.3) topsAt(g, {{0, 0}}, g.sink + 3.3 + 2.4, 0.8, mat); }
}

// C-01's columns in a row, some fallen (A-04: the people's number of them, round or square); an arcade where the people built arches. B-415:
// an industrial people's a covered walk, its cast-iron columns under a canopy of iron and glass; a modern people's its pilotis under a
// slab of concrete; a later people's of composite (BUBAS's columns of stone along its main streets read as medieval)
void genColonnade(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    if (g.lod == 2) return;
    if (c.age >= AGE_INDUSTRIAL) {
        const bool iron = c.age == AGE_INDUSTRIAL;
        const int cm = iron ? MT_IRON : (c.age == AGE_MODERN ? MT_CONCRETE : MT_COMPOSITE), cs = iron ? g.ironSlot() : 0;
        const double H = b.hWall;
        const int n = std::max(3, (int)std::lround(2 * b.hd / 3.2) + 1);
        bool all = true;
        for (int k = 0; k < n; k++) {
            const double tt = -b.hd + 2 * b.hd * k / (n - 1), d = g.Dk(cm, 10 + k);
            if (d > 0.75) { all = false; continue; }
            const double f = d < 0.35 ? 1 : 1 - (d - 0.35) * 1.5;
            if (f < 0.97) all = false;
            if (iron) g.lathe(0, 0, tt, 0.12, 0, H * f, cs, cm, 6);
            else g.box(0, tt, 0, 0.2, 0.2, 0, H * f, cs, cm, 1);
        }
        if (all && g.D(cm) < (iron ? 0.4 : 0.55)) g.box(0, 0, 0, 1.4, b.hd + 0.4, H, H + (iron ? 0.15 : 0.3), iron ? 2 : 0, cm, 2);   // the canopy
        return;
    }
    const int mat = worksOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    if (c.technique == TQ_ARCH_VAULT && g.U(3) < 0.75) {
        Form& e = g.add(FK_ARCH, archSub(g), 0, 0, PI / 2, b.hd, 0.45, g.sink, g.sink + b.hWall * 1.25, 0, mat);
        e.p[0] = std::max(2.0, std::round(b.hd * 2 / 3.2)); e.p[1] = 0.34; e.p[2] = b.hWall * 0.65; e.broken = Dm < 0.3 ? 0 : clampd(Dm, 0.3, 0.9);
        return;
    }
    const int n = clampi(c.genome.number() + 2, 4, 11);
    const bool round = g.gv(GV_PLAN) != PF_RECT && g.gv(GV_PLAN) != PF_COURT;
    for (int k = 0; k < n; k++) {
        const double tt = -b.hd + 1.0 + (2 * b.hd - 2.0) * k / (n - 1), f = 1 - Dm * (0.2 + 0.8 * g.U(10 + k));
        if (f < 0.25) continue;
        if (round) g.lathe(0, 0, tt, 0.42, g.sink, g.sink + b.hWall * f, 0, mat, g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : 8);
        else g.box(0, tt, 0, 0.4, 0.4, g.sink, g.sink + b.hWall * f, 0, mat, 1);
        if (f > 0.97 && g.lod == 0 && g.carved()) g.box(0, tt, 0, 0.6, 0.6, g.sink + b.hWall - 0.3, g.sink + b.hWall, g.accentOn(AP_TOPS) ? 1 : 0, mat, 4);   // a capital
    }
}

// C-01's stela; A-04: the people's monument (`Building::style`): a stela, an obelisk, a statue on its pedestal, a free arch, a pillar
// with its capital (the prehistoric age's are standing stones: `genMenhir`)
void genStela(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = worksOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    switch (b.style) {
        case MN_OBELISKS: {   // an obelisk with its point
            double h = b.hWall * 1.6 * (1 - 0.5 * Dm * g.U(10));
            Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, 0.6, 0.6, g.sink, g.sink + h, 0, mat, 5); e.p[1] = 0.62;
            if (h > b.hWall * 1.5) { Form& t = g.add(FK_PYRAMID, 0, 0, 0, 0, 0.6 * 0.62, 0.6 * 0.62, g.sink + h, g.sink + h + 0.7, g.topSlot(mat), mat, 5); t.p[1] = 0; }
            return;
        }
        case MN_COLOSSI: {   // a statue on its pedestal
            const double ph = b.hWall * 0.3;
            g.box(0, 0, 0, 0.9, 0.9, g.sink, g.sink + ph, 0, mat, 5);
            figure(g, 0, 0, g.sink + ph, b.hWall * 0.9, g.accentOn(AP_TOPS) ? 1 : 0, mat, Dm);
            return;
        }
        case MN_ARCHES: {   // a free arch
            Form& e = g.add(FK_ARCH, archSub(g), 0, 0, 0, 2.4, 0.7, g.sink, g.sink + b.hWall * 1.4, 0, mat, 5);
            e.p[0] = 1; e.p[1] = 0.36; e.p[2] = b.hWall * 0.7; e.broken = Dm < 0.4 ? 0 : clampd(Dm, 0.4, 0.85);
            return;
        }
        case MN_PILLARS: {   // a column alone, its capital
            const double h = b.hWall * 1.8 * (Dm < 0.4 ? 1 : 1 - 0.6 * Dm * g.U(10));
            Form& e = g.lathe(0, 0, 0, 0.45, g.sink, g.sink + h, 0, mat, g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : 0); e.part = 5;
            if (h >= b.hWall * 1.75 && g.lod < 2) { g.box(0, 0, 0, 0.7, 0.7, g.sink + h, g.sink + h + 0.4, g.topSlot(mat), mat, 5); topsAt(g, {{0, 0}}, g.sink + h + 0.4, 0.7, mat); }
            return;
        }
        default: { Form& e = g.box(0, 0, 0, 0.45, 0.25, g.sink, g.sink + b.hWall * (1 - Dm * 0.6 * g.U(10)), 0, mat, 5); e.glyphs = g.lod == 0; return; }
    }
}

void genRubble(Gen& g) {   // C-01's heap where a house fell
    const Building& b = g.b; const int mat = worksOf(b, g.c);
    if (g.lod == 2) { g.box(0, 0, 0, b.hw * 0.8, b.hd * 0.8, 0, 1.0, 0, mat, 3); return; }
    int nb = 5 + (int)(g.U(1) * 4);
    for (int k = 0; k < nb; k++) {
        double u1 = g.U(10 + k), u2 = g.U(30 + k), sz = 0.4 + 0.7 * g.U(50 + k);
        g.box((u1 - 0.5) * 1.6 * b.hw, (u2 - 0.5) * 1.6 * b.hd, u1 * 2.0, sz, sz * 0.7, 0, sz * 1.2, 0, mat, 3);
    }
}

void genWall(Gen& g) {   // C-01's stretch of the town wall; A-04: battered where the people battered its walls, crenellated where it crenellated
    const Building& b = g.b; const int mat = worksOf(b, g.c); const double Dm = g.D(mat);
    g.sink = -g.c.buried * Dm;
    const double h = std::max(0.4, b.hWall * wallShare(Dm, g.U(10)));
    const int prof = g.gv(GV_PROFILE);
    Form& e = g.box(0, 0, 0, b.hw, b.hd * (prof == WP_BATTERED ? 1.5 : 1.0), g.sink, g.sink + h, 0, mat);
    if (prof == WP_BATTERED) e.tilt = g.U(11) < 0.5 ? 0.06 : -0.06;   // its outer face leaning back
    if (h >= b.hWall * 0.97 && g.lod == 0 && g.gv(GV_TOPS) == TP_CRENELS) {
        const int n = std::max(2, (int)(2 * b.hw / 1.6));
        for (int k = 0; k < n; k += 2) { double t = -b.hw + (k + 0.5) * 2 * b.hw / n; g.box(t, 0, 0, b.hw / n * 0.5, b.hd, g.sink + h, g.sink + h + 0.8, g.topSlot(mat), mat); }
    }
}

// C-08: the terrain's features, of the age's works' material
void genFeature(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = worksOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    const double sink = g.sink;
    switch (b.kind) {
        case BK_QUAY: {   // two tiers along the shore, the top one worn away toward one end, bollards on the seaward edge
            double keep = 1 - 0.5 * Dm;
            g.box(0, 0, 0, b.hw, b.hd, sink - 0.6, sink + 0.6, 0, mat, 6);
            g.box(-b.hw * (1 - keep), 0, 0, b.hw * keep, b.hd - 0.2, sink + 0.6, sink + 1.2, 0, mat, 6);
            if (g.lod == 0)
                for (int k = 0; k < 3; k++) {
                    if (g.U(20 + k) < Dm * 0.6) continue;
                    g.box(-b.hw * 0.7 + b.hw * 0.7 * k, b.hd - 0.5, 0, 0.22, 0.22, sink + 1.2, sink + 1.9, 0, mat, 1);
                }
            break;
        }
        case BK_MOLE: {   // a wall out into the water, its far end fallen to the waterline
            if (g.lod == 2) { g.box(0, 0, 0, b.hw, b.hd, sink - 0.6, sink + b.hWall * (1 - 0.5 * Dm), 0, mat, 6); break; }
            int segs = std::max(2, (int)(b.hd / 3.0));
            for (int k = 0; k < segs; k++) {
                double u = (k + 0.5) / segs, f = 1 - Dm * (0.1 + 0.9 * u) * (0.6 + 0.4 * g.U(10 + k));
                if (f < 0.12) continue;
                g.box(0, -b.hd + b.hd * 2 * u, 0, b.hw, b.hd / segs, sink - 0.6, sink + std::max(0.1, b.hWall * f), 0, mat, 6);
            }
            break;
        }
        case BK_BOLLARD: g.box(0, 0, 0, 0.22, 0.22, sink, sink + 0.8, 0, mat, 1); break;
        case BK_TERRACE: { double f = 1 - Dm * (0.2 + 0.8 * g.U(10)); g.box(0, 0, 0, b.hw, b.hd, sink, sink + std::max(0.3, b.hWall * f), 0, mat); break; }
        case BK_CISTERN: {   // a round tank of thick wall, its lid fallen in, the channel that fed it from upslope
            double rr = b.hw;
            if (g.lod == 2) { g.box(0, 0, 0, rr, rr, sink, sink + b.hWall * (1 - 0.4 * Dm), 0, mat); break; }
            const int N = 10; double ht = b.thick * 0.5, half = rr * std::sin(PI / N) + ht * 0.6, hmin = 1e9;
            for (int k = 0; k < N; k++) {
                double a = (k + 0.5) * TAU / N, h = std::max(0.4, b.hWall * (1 - Dm * (0.1 + 0.6 * g.U(10 + k))));
                hmin = std::min(hmin, h);
                g.box(std::sin(a) * rr, std::cos(a) * rr, a, half, ht, sink, sink + h, 0, mat);   // A-04: along the ring (C-08's stood across it)
            }
            if (Dm < 0.5) g.box(0, 0, 0, rr * 0.5, rr * 0.5, sink + hmin - 0.1, sink + hmin + 0.25, 0, mat, 2);   // what is left of the lid
            if (g.lod == 0) {   // the channel: two low walls a metre apart, 6-14 m out along the heading (upslope)
                double len = 6 + 8 * g.U(30);
                for (int side = -1; side <= 1; side += 2) g.box(side * 0.6, rr + 0.7 + len * 0.5, 0, 0.15, len * 0.5, sink, sink + 0.45, 0, mat);
            }
            break;
        }
        case BK_WINDWALL: {   // a segment of the crescent, the sand against its windward face
            double f = 1 - Dm * (0.15 + 0.85 * g.U(10));
            g.box(0, 0, 0, b.hw, b.hd, sink, sink + std::max(0.4, b.hWall * f), 0, mat);
            if (g.lod == 0 && c.buried >= 0.15) g.box(0, b.hd + 0.9, 0, b.hw, 0.9, 0, 0.25 + 0.6 * std::min(1.0, c.buried / 1.2), 0, -1, 7);
            break;
        }
        case BK_CAIRN: {   // three tiers of stones, the top ones tumbled by the age
            const double Dc = g.D(MT_DRYSTONE);
            int tiers = Dc < 0.35 ? 3 : (Dc < 0.7 ? 2 : 1);
            for (int i = 0; i < tiers; i++) g.box(0, 0, i * 0.3, b.hw * (1 - 0.3 * i), b.hd * (1 - 0.3 * i), 0.7 * i, 0.7 * (i + 1), 0, MT_DRYSTONE, 3);
            if (g.lod == 0 && tiers < 3) g.box(1.4, 0.6, 0.8, 0.4, 0.3, 0, 0.4, 0, MT_DRYSTONE, 3);
            break;
        }
        default: break;
    }
}

// the people's motif on a great building of half extents hw x hd, its walls standing to `h` of their `H`: ribs up its walls (lines),
// spheres on its corners (circles), steps along its top (steps), a ramp climbing round it (spirals), bands of glyphs (the walls'),
// statues at its front's corners (figures), the points of a star round its foot (stars); what stands on its walls only while they do
void motifOn(Gen& g, double cx, double cz, double hw, double hd, double h, double H, int mat) {
    if (g.lod == 2) return;
    const double base = g.sink;
    const bool whole = h >= 0.92 * H;
    switch (g.gv(GV_MOTIF)) {
        case MO_LINES:
            if (g.lod == 0 && h > 1.5) for (int s = 0; s < 4; s++) {
                const bool al = s % 2 == 0; const double L = al ? hw : hd, nx = s == 1 ? 1 : (s == 3 ? -1 : 0), nz = s == 0 ? 1 : (s == 2 ? -1 : 0);
                const int n = std::max(2, (int)(2 * L / 2.2));
                for (int k = 1; k < n; k++) { const double t = -L + 2 * L * k / n; if (s == 0 && std::fabs(t) < 1.4) continue; g.box(cx + (al ? t : nx * (hw + 0.3)), cz + (al ? nz * (hd + 0.3) : -t), al ? 0 : PI / 2, 0.16, 0.2, base, base + h, 0, mat, 4); }
            }
            break;
        case MO_CIRCLES: if (whole) { const double r = std::min(hw, hd) * 0.18 + 0.3; for (int q = 0; q < 4; q++) g.add(FK_DOME, 0, cx + (q % 2 ? hw : -hw), cz + (q < 2 ? hd : -hd), 0, r, r, base + h, base + h + r, g.topSlot(mat), mat, 4); } break;
        case MO_STEPS:
            if (whole && g.lod == 0) for (int s = 0; s < 4; s += 2) for (int k = 0; k < 3; k++) { const double w = hw * (0.8 - 0.25 * k); g.box(cx, cz + (s == 0 ? hd : -hd), 0, w, 0.35, base + h + 0.6 * k, base + h + 0.6 * (k + 1), g.topSlot(mat), mat, 4); }
            break;
        case MO_SPIRALS:   // a ramp round the walls, a side a quarter of its rise, the front's doorway's stretch left out
            if (whole && g.lod == 0) for (int k = 0; k < 8; k++) {
                const int s = (k / 2 + 1) % 4; const bool al = s % 2 == 0; const double L = al ? hw : hd, nx = s == 1 ? 1 : (s == 3 ? -1 : 0), nz = s == 0 ? 1 : (s == 2 ? -1 : 0);
                const double t = (k % 2 ? 0.5 : -0.5) * L, y = base + h * (k + 1) / 9.0;
                if (s == 0 && std::fabs(t) < 1.6) continue;
                g.box(cx + (al ? t : nx * (hw + 0.45)), cz + (al ? nz * (hd + 0.45) : -t), al ? 0 : PI / 2, L * 0.5, 0.45, y - 0.35, y, 1, mat, 4);
            }
            break;
        case MO_FIGURES: for (int s = -1; s <= 1; s += 2) figure(g, cx + s * (hw + 1.2), cz + hd + 1.2, base, std::max(2.4, H * 0.45), g.accentOn(AP_TOPS) ? 1 : 0, mat, g.D(mat)); break;
        case MO_STARS:
            if (g.lod == 0) for (int k = 0; k < 8; k++) {
                const double a = k * PI / 4 + PI / 8, R = std::max(hw, hd) * 1.15 + 2.0;
                if (std::fabs(std::remainder(a, TAU)) < 0.5) continue;   // the front's point left out (the doorway)
                Form& e = g.add(FK_PYRAMID, 0, cx + std::sin(a) * R, cz + std::cos(a) * R, a, 1.4, 2.6, base, base + 0.6, g.accentOn(AP_TOPS) ? 1 : 0, mat, 4); e.sides = 3; e.p[1] = 0;
            }
            break;
        default: break;   // glyph bands: the walls' own
    }
}

// the temple at a medieval town's centre: a basilica (a nave under a gable or a vault, its towers as its people placed them: a pair
// at the front, one over the crossing, two at opposite corners, one at a corner), a square hall under a dome on its drum with its
// minarets, a stepped mass with a shrine on top and a small house at the stair's foot, or (A-04) a round temple under a dome or a cone
// in a ring of columns. A-04: in its age's material (a modern people's temple of concrete), its towers in the people's section,
// profile and tops, its motif. B-415: an industrial people's round temple a brick rotunda under a saucer of iron (no peristyle); a modern
// people's of its age: its stepped mass a low mausoleum of three broad setbacks, its round temple a drum under a saucer or a cone, its
// domed hall a saucer and needles for minarets, its basilica's roof a shell of concrete (a steep gable, a vault, a saddle, a saucer, a
// tent; steps a flat roof) and its towers' tops needles, saucers or masts (A-04's concrete ziggurats, onion domes and peristyles read as
// a medieval or an ancient town's heart)
void genTemple(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    const double big = b.hWall;   // the walls' height (the nave's, the hall's); the rest in proportion
    const double hgt = 0.7 + 0.8 * g.gf(GV_HEIGHT);   // the people's appetite for height: its towers and minarets
    const bool modern = c.age >= AGE_MODERN, machine = c.age >= AGE_INDUSTRIAL;
    if (b.style == 2) {   // the stepped mass: tiers, the shrine on top, the stair up its front, a house at the stair's foot
        const double H = big * (modern ? 0.9 : 2.2), hx = b.hw * (modern ? 0.9 : 0.8), hz = b.hd * 0.62, cz = -b.hd * 0.3;
        const double top = H * tallShare(Dm * 0.8, g.U(10));
        Form& z = g.add(FK_PYRAMID, 0, 0, cz, 0, hx, hz, g.sink, g.sink + H, 0, mat); z.p[0] = modern ? 3 : clampi(c.genome.number(), 3, 7); z.p[1] = modern ? 0.62 : 0.38; z.p[2] = modern ? 1 : 0.55;
        if (modern) z.pattern = 3;
        if (g.gv(GV_PLAN) == PF_POLYGON) z.sides = clampi(c.genome.number(), 3, 8);
        z.broken = top < H * 0.97 ? clampd(1 - top / H, 0.1, 0.95) : 0;
        if (z.broken == 0 && g.lod < 2 && !modern) { Form& s = g.box(0, cz, 0, hx * 0.3, hz * 0.3, g.sink + H, g.sink + H + big * 0.5, 0, mat); s.glyphs = g.lod == 0; topsAt(g, {{0, cz}}, g.sink + H + big * 0.5, 1.2, mat); }
        if (g.lod == 0) for (int k = 0; k < 6; k++) { double t = (k + 0.5) / 6; g.box(0, cz + hz + 1.2 - t * hz * 0.85, 0, 1.6, 1.2, g.sink, g.sink + H * t * 0.8 * (1 - 0.5 * Dm), 0, mat, 2); }
        Walls W; W.cz = b.hd - 3.5; W.hw = 3.2; W.hd = 3.0; W.h = 4.0; W.thick = 0.6; W.door = 0; W.mat = mat; W.glyphs = true; W.k0 = 30;
        double hs[4]; double hmin = walls4(g, W, hs);
        roof(g, RF_FLAT, 0, W.cz, W.hw, W.hd, hmin, W.h, 0.8, mat);
        drifts(g, b.hw, b.hd, Dm);
        return;
    }
    if (b.style == 3) {   // the round temple: a ring of wall in the people's section, its dome or cone, a ring of columns round it
        const double r = b.hw;
        Ring R; R.rx = r; R.h = big; R.thick = 1.0; R.mat = mat; R.pattern = 3;
        R.segs = templeRingSegs(b);   // the room reads the same (`buildingRoom`)
        R.rz = b.form == PF_ELLIPSE ? r * 1.25 : r;
        R.doorSegs = ringDoorSegs(r, R.segs); R.door = ((R.segs - R.doorSegs / 2) % R.segs + R.segs) % R.segs;
        const double hmin = ringWalls(g, R);
        const int rf = c.roof == RF_DOMED || c.roof == RF_VAULTED || c.roof == RF_FLAT || c.roof == RF_OPEN || c.roof == RF_STEPPED ? (c.roof == RF_STEPPED && !machine ? RF_STEPPED : RF_DOMED) : RF_CONICAL;
        const int rm = machine && rf == RF_DOMED ? (modern ? MT_CONCRETE : MT_IRON) : roofMaterial(g, rf, mat);
        double rise = 0;
        if (machine && rf == RF_DOMED) {   // a saucer: of iron and glass, or a concrete shell
            const double Dr = g.D(rm);
            if (hmin >= big * 0.92 && Dr < 0.6) { Form& e = g.add(FK_DOME, 1, 0, 0, 0, R.rx + 0.5, R.rz + 0.5, g.sink + hmin - 0.1, g.sink + hmin - 0.1 + r * 0.5, g.roofSlot(rm), rm, 2); e.broken = Dr < 0.3 ? 0 : clampd(Dr, 0.3, 0.9); rise = r * 0.4; }
        } else rise = roundRoof(g, rf, 0, 0, R.rx + 0.5, R.rz + 0.5, R.segs, hmin, big, 0.8, rm);
        if (hmin >= big * 0.92) roundTops(g, rf, g.sink + hmin, rise, 1.4, rm);
        if (g.lod < 2 && !machine) {   // the peristyle
            const int n = std::max(8, 2 * c.genome.number());
            for (int k = 0; k < n; k++) {
                const double a = (k + 0.5) * TAU / n; if (std::fabs(std::remainder(a, TAU)) < 0.4) continue;   // the front's left open
                const double f = 1 - Dm * (0.2 + 0.8 * g.U(60 + k)); if (f < 0.25) continue;
                g.lathe(0, std::sin(a) * (R.rx + 2.6), std::cos(a) * (R.rz + 2.6), 0.45, g.sink, g.sink + big * 0.85 * f, 0, mat, 8);
            }
        }
        motifOn(g, 0, 0, r * 0.75, r * 0.75, std::min(hmin, big), big, mat);
        drifts(g, R.rx, R.rz, Dm);
        return;
    }
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = big; W.thick = 1.1; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 3; W.glyphs = true;
    if (b.style == 1) { W.hd = b.hw; W.cz = 0; }   // the square hall
    W.crenels = b.style == 1 && g.gv(GV_TOPS) == TP_CRENELS;
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int sym = g.gv(GV_SYMMETRY);
    if (b.style == 1) {   // the drum, the dome, the minarets (two at the front, four at the corners, or round it by the people's symmetry and number)
        const double rd = b.hw * 0.72, drumTop = big + 3.5 + b.hw * 0.15;
        if (hmin > big * 0.92) {
            Form& d = g.lathe(0, 0, 0, rd, g.sink + hmin, g.sink + drumTop, 0, mat, g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : 8); d.pattern = 3;
            if (Dm < 0.65) {
                const int prof = modern ? 1 : (g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : (g.gv(GV_OPENING) == OP_ARCHED || g.gv(GV_OPENING) == OP_TRIANGULAR ? 2 : (g.gv(GV_MOTIF) == MO_CIRCLES ? 0 : (b.age >= AGE_MODERN ? 1 : 0))));
                const double dH = rd * (prof == 3 ? 1.5 : (prof == 2 ? 1.35 : (prof == 1 ? 0.55 : 1.0)));
                Form& e = g.add(FK_DOME, prof, 0, 0, 0, rd, rd, g.sink + drumTop, g.sink + drumTop + dH, g.roofSlot(mat), mat, 2); e.broken = Dm < 0.3 ? 0 : clampd(Dm, 0.3, 0.9);
                if (Dm < 0.3) topsAt(g, {{0, 0}}, g.sink + drumTop + dH, 1.5, mat);
            }
        }
        const double mh = big * 3.4 * hgt;
        const int nm = sym == SY_NONE ? 1 : (sym == SY_BILATERAL ? 2 : (sym == SY_RADIAL ? clampi(c.genome.number(), 4, 8) : 4));
        for (int k = 0; k < nm; k++) {
            double lx, lz;
            if (sym == SY_RADIAL) { const double a = (k + 0.5) * TAU / nm, rr = b.hw * 1.42 + 3.0; lx = std::sin(a) * rr; lz = std::cos(a) * rr; }
            else if (sym == SY_ROTATIONAL) { const double a = k * PI / 2; lx = std::cos(a) * (b.hw + 3.0) - std::sin(a) * (b.hw * 0.5); lz = std::sin(a) * (b.hw + 3.0) + std::cos(a) * (b.hw * 0.5); }
            else { lx = (k % 2 ? 1 : -1) * (b.hw + 3.0); lz = (k < 2 ? 1 : -1) * (b.hw + 3.0); }
            TowerOut T = towerShaft(g, lx, lz, modern ? 1.0 : 1.5, mh, mat, 13 + k * 3, 2);
            if (T.whole && modern) { Form& t = g.lathe(2, lx, lz, 0.9, g.sink + T.h, g.sink + T.h + mh * 0.18, g.topSlot(mat), mat); t.p[0] = 1.4; }   // a needle
            else if (T.whole) { if (g.c.roof == RF_DOMED) { Form& t = g.add(FK_DOME, 3, lx, lz, 0, 1.6, 1.6, g.sink + T.h, g.sink + T.h + 3.0, g.topSlot(mat), mat); (void)t; } else { Form& t = g.lathe(1, lx, lz, 1.8, g.sink + T.h, g.sink + T.h + 3.5, g.topSlot(mat), mat); t.p[0] = 0; } }
        }
    } else {   // the basilica: its roof, its towers with their spires (the people's focus a spire: tall)
        const int rf = modern ? (c.roof == RF_STEPPED || c.roof == RF_OPEN ? RF_FLAT : c.roof) : (c.roof == RF_FLAT ? RF_VAULTED : (c.roof == RF_OPEN ? RF_PITCHED : c.roof));
        roof(g, rf, 0, 0, b.hw, b.hd, hmin, big, modern && rf == RF_PITCHED ? 1.6 : 0.9, modern ? MT_CONCRETE : roofMaterial(g, rf, mat));   // a modern one's shell of concrete
        if (rf == RF_PITCHED) gables(g, W, hs, modern ? 1.6 : 0.9);
        const double tr = std::max(3.0, b.hw * 0.42), tH = big * 2.6 * hgt, sH = big * (g.gv(GV_FOCUS) == FO_SPIRE ? 3.4 : 2.2);
        std::vector<std::pair<double, double>> at;
        const double side = (g.U(14) < 0.5 ? -1 : 1);
        switch (sym) {
            case SY_BILATERAL: at = {{-(b.hw + tr - 0.6), b.hd - tr}, {b.hw + tr - 0.6, b.hd - tr}}; break;
            case SY_RADIAL: at = {{0, 0}}; break;   // over the crossing
            case SY_ROTATIONAL: at = {{side * (b.hw + tr - 0.6), b.hd - tr}, {-side * (b.hw + tr - 0.6), -b.hd + tr}}; break;
            default: at = {{side * (b.hw + tr - 0.6), b.hd - tr}}; break;
        }
        for (size_t k = 0; k < at.size(); k++) {
            double tx = at[k].first, tz = at[k].second, H0 = 0;
            if (sym == SY_RADIAL) { if (hmin < big * 0.92) continue; H0 = hmin + 0.3; }   // the crossing's tower stands on the walls
            const double sinkSave = g.sink; g.sink += H0;
            TowerOut T = towerShaft(g, tx, tz, sym == SY_RADIAL ? std::min(tr, b.hw * 0.8) : tr, sym == SY_RADIAL ? tH * 0.7 : tH, mat, 15 + (int)k * 3, 2, sym != SY_RADIAL);
            if (T.whole) towerTop(g, tx, tz, T.rTop, T.h, tH, mat, true, sH);
            g.sink = sinkSave;
        }
    }
    motifOn(g, 0, W.cz, W.hw, W.hd, std::min(hmin, big), big, mat);
    double doorX, doorZ; doorMid(0, 0, W.hw, W.hd, doorX, doorZ);
    rubble(g, W.hw, W.hd, hs, big, doorX, doorZ + W.cz, 0, mat);
    drifts(g, W.hw, W.hd, Dm);
}

// A-04: a windmill (a medieval people's machine at its town's heart): a tower in the people's section, tapering, its cap and its sails
void genWindmill(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = stoneOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm;
    const double r = b.hw, H = b.hWall;
    const double h = H * tallShare(Dm, g.U(10));
    const int sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : (g.gv(GV_PLAN) == PF_RECT ? 8 : 0);
    Form& t = g.lathe(1, 0, 0, r, g.sink, g.sink + h, 0, mat, sides); t.p[0] = 1 - 0.32 * h / H; t.pattern = 2; t.broken = h < H * 0.97 ? 0.3 : 0;
    if (h < H * 0.97) return;
    const double rt = r * 0.68, Dt = g.D(MT_TIMBER);
    if (Dt < 0.5) {
        const int rf = c.roof == RF_DOMED || c.roof == RF_VAULTED ? RF_DOMED : RF_CONICAL;
        if (rf == RF_DOMED) g.add(FK_DOME, 0, 0, 0, 0, rt + 0.3, rt + 0.3, g.sink + h, g.sink + h + rt, g.roofSlot(MT_TIMBER), MT_TIMBER, 2);
        else { Form& e = g.lathe(1, 0, 0, rt + 0.4, g.sink + h, g.sink + h + rt * 1.3, g.roofSlot(MT_TIMBER), MT_TIMBER); e.p[0] = 0.05; }
    }
    if (Dt < 0.3 && g.lod < 2) {   // the sails: four arms on the front, crossed
        const double hub = g.sink + h - 0.4, L = H * 0.55;
        for (int k = 0; k < 4; k++) { Form& s = g.box(0, rt + 0.6, PI / 2, 0.9, 0.12, hub, hub + L, g.timberSlot(), MT_TIMBER, 4); s.tilt = 0.4 + k * PI / 2; }   // tipped in the front's plane
    }
}
}   // namespace

// ---- the prehistoric age ------------------------------------------------------------------------------------------------------

namespace {
// a hut's ring: its people's polygon or oval, a circle of ten otherwise; its doorway two segments (C-01's draw, or the front's middle)
int hutSegs(const Building& b) { return b.form == PF_POLYGON ? clampi(b.sides, 5, 9) : (b.form == PF_ELLIPSE ? 12 : 10); }
int hutDoorSegs(const Building& b) { return std::max(2, ringDoorSegs(std::min(b.hw, b.form == PF_ELLIPSE ? b.hd : b.hw), hutSegs(b))); }
int hutDoorSeg(const Building& b) { return ringDoorFirst(b, hutSegs(b), hutDoorSegs(b)); }   // the first of the segments left out for the doorway

// the hut's roof (the people's): a cone of thatch (pitched, hipped, conical), a corbelled dome of dry stone (domed: a beehive), a
// turf roof on it (flat, vaulted), a saddle of hide, a stepped cone, nothing (open: a windbreak)
void hutRoof(Gen& g, double rx, double rz, double hmin, double h, bool stone) {
    const int rf = g.c.roof;
    if (hmin < 0.92 * h || rf == RF_OPEN) return;
    const double Dt = g.D(MT_TIMBER), r = std::max(rx, rz);
    hmin += g.sink;   // over a pit house's sunk floor
    if (rf == RF_DOMED && stone) {   // a beehive: the dry stone corbelled in to a point
        const double Ds = g.D(MT_DRYSTONE);
        if (Ds < 0.6) { Form& e = g.add(FK_DOME, 2, 0, 0, 0, rx + 0.2, rz + 0.2, hmin - 0.1, hmin - 0.1 + r * 1.15, g.roofSlot(MT_DRYSTONE), MT_DRYSTONE, 2); e.broken = Ds < 0.25 ? 0 : clampd(Ds, 0.3, 0.9); }
        return;
    }
    if (rf == RF_FLAT || rf == RF_VAULTED || rf == RF_DOMED) {   // turf on its timbers: a mound of the ground's own grass
        const double De = materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet);
        if (Dt < 0.5 && De < 0.8) { Form& e = g.add(FK_DOME, 1, 0, 0, 0, rx + 0.4, rz + 0.4, hmin - 0.1, hmin - 0.1 + r * (rf == RF_FLAT ? 0.25 : 0.55), 0, MT_EARTH, 8); e.broken = Dt < 0.3 ? 0 : 0.5; }
        return;
    }
    if (Dt >= 0.5) return;
    const int slot = g.roofSlot(MT_TIMBER);
    const double ev = g.eave() + 0.05, pk = g.pitchK();   // R-412: the people's eaves and pitch
    if (rf == RF_SADDLE) { Form& e = g.add(FK_SHELL, 0, 0, 0, 0, rx + ev, rz + ev, hmin - 0.1, hmin - 0.1 + r * 1.0 * pk, slot, MT_TIMBER, 2); e.broken = Dt < 0.22 ? 0 : 0.5; return; }
    if (rf == RF_STEPPED) {   // two cones, the upper set on the lower
        Form& a = g.add(FK_LATHE, 1, 0, 0, 0, rx + ev, rz + ev, hmin - 0.1, hmin - 0.1 + r * 0.6 * pk, slot, MT_TIMBER); a.p[0] = 0.45;
        Form& t = g.add(FK_LATHE, 1, 0, 0, 0, (rx + ev) * 0.5, (rz + ev) * 0.5, hmin + r * 0.45 * pk, hmin + r * 1.25 * pk, slot, MT_TIMBER); t.p[0] = 0.04;
        return;
    }
    Form& e = g.add(FK_LATHE, 1, 0, 0, 0, rx + ev, rz + ev, hmin - 0.1, hmin - 0.1 + r * (rf == RF_CONICAL ? 1.6 : 1.25) * (0.8 + 0.4 * g.gf(GV_PROPORTION)) * pk, slot, MT_TIMBER);
    e.p[0] = 0.04; e.broken = Dt < 0.22 ? 0 : clampd((Dt - 0.22) * 2.4, 0.2, 0.8);
    if (g.flared() && g.lod < 2 && Dt < 0.3) { Form& k = g.add(FK_LATHE, 1, 0, 0, 0, rx + ev + 0.45, rz + ev + 0.45, hmin - 0.35, hmin + 0.2, slot, MT_TIMBER); k.p[0] = 0.7; }   // its skirt of thatch
}

void genHut(Gen& g) {   // a hut on the people's plan: a ring of dry stone, or wattle on posts; its roof; a hearth of four stones; A-04: a rectangle of
    const Building& b = g.b; const Culture& c = g.c;   // posts under a ridge, two rings joined (free), a pit house (buried, or sunk where the people dug in)
    const double h = b.hWall, Dt = g.D(MT_TIMBER), Ds = g.D(MT_DRYSTONE);
    const bool stone = c.hut == 1;
    if (g.gv(GV_MASSING) == MS_BURIED || b.found == FD_PIT) g.sink = -std::min(0.9, 0.45 * h);
    if (b.form == PF_RECT || b.form == PF_COURT) {   // a rectangular hut: walls of stone or wattle on posts, a ridge over it
        Walls W; W.hw = b.hw; W.hd = b.hd; W.h = h; W.thick = stone ? 0.6 : 0.2; W.door = 0; W.doorPos = 0; W.mat = stone ? MT_DRYSTONE : MT_TIMBER; W.slot = stone ? 0 : g.timberSlot(); W.dress = false;
        double hs[4] = {0, 0, 0, 0}, hmin = 0;
        if (stone || Dt < 0.45) hmin = walls4(g, W, hs);
        else if (g.lod == 0) { int k = 0; for (int s = -1; s <= 1; s += 2) for (double t = -b.hd; t <= b.hd + 0.01; t += 1.5, k++) g.post(s * b.hw, t, h, 0.13, 200 + k); }
        if (hmin > 0.92 * h) {
            const int rf = c.roof == RF_DOMED || c.roof == RF_VAULTED || c.roof == RF_FLAT ? RF_FLAT : (c.roof == RF_HIPPED || c.roof == RF_CONICAL ? RF_HIPPED : RF_PITCHED);
            if (rf == RF_FLAT) { const double De = materialDecay(MT_EARTH, c.ageYears * 0.1, c.wet); if (Dt < 0.5 && De < 0.8) { Form& e = g.box(0, 0, 0, b.hw + 0.4, b.hd + 0.4, g.sink + hmin - 0.1, g.sink + hmin + 0.35, 0, MT_EARTH, 8); (void)e; } }
            else roof(g, rf, 0, 0, b.hw, b.hd, hmin, h, 1.05, MT_TIMBER);
        }
        if (g.lod == 0) for (int k = 0; k < 4; k++) { double a = k * PI / 2 + 0.4; g.box(std::sin(a) * 0.45, std::cos(a) * 0.45, a, 0.17, 0.13, 0, 0.16, 0, MT_DRYSTONE, 3); }
        if (hmin > 0.92 * h) hutEntrance(g, b.hw, b.hd, 0, h, stone);   // R-412: its entrance, its pole, its yard
        hutPole(g, b.hw * 0.6 + 1.0, b.hd + 1.5);
        if (!b.joined) hutRing(g, b.hw + 0.4, b.hd + 0.4, 0);
        return;
    }
    const int N = hutSegs(b), dseg = hutDoorSegs(b), door = hutDoorSeg(b);
    const double rx = b.hw, rz = b.form == PF_ELLIPSE ? b.hd : b.hw;
    auto inDoor = [&](int k) { int d = ((k - door) % N + N) % N; return d < dseg; };
    double hmin = h;
    if (g.lod == 2) {
        if (stone) g.add(FK_LATHE, 0, 0, 0, 0, rx, rz, g.sink, g.sink + std::max(0.4, h * wallShare(Ds, 0.5)), 0, MT_DRYSTONE);
        if (Dt < 0.35) { Form& e = g.add(FK_LATHE, 1, 0, 0, 0, rx + 0.5, rz + 0.5, g.sink + (stone ? h : 0), g.sink + h + rx * 1.2, g.roofSlot(MT_TIMBER), MT_TIMBER); e.p[0] = 0.04; }
        return;
    }
    if (stone) {
        Ring R; R.rx = rx; R.rz = rz; R.h = h; R.thick = 0.64; R.mat = MT_DRYSTONE; R.segs = N; R.door = door; R.doorSegs = dseg; R.dress = false; R.profile = g.gv(GV_PROFILE) == WP_BATTERED || g.gv(GV_PROFILE) == WP_BULGING ? WP_BATTERED : WP_UPRIGHT;
        hmin = ringWalls(g, R);
    } else {
        for (int k = 0; k < N; k++) {   // the posts at the joints (none inside the doorway), the wattle between them
            const bool inGap = k != door && inDoor(k) && inDoor((k + N - 1) % N);
            if (inGap) continue;
            const double a = k * TAU / N; g.post(std::sin(a) * rx, std::cos(a) * rz, h, 0.13, 10 + k);
        }
        if (Dt < 0.35)
            for (int k = 0; k < N; k++) {
                if (g.lod == 0 && inDoor(k)) continue;
                const double a0 = k * TAU / N, a1 = (k + 1) * TAU / N, x0 = std::sin(a0) * rx, z0 = std::cos(a0) * rz, x1 = std::sin(a1) * rx, z1 = std::cos(a1) * rz;
                const double dx = x1 - x0, dz = z1 - z0, ch = std::sqrt(dx * dx + dz * dz);
                g.box(0.5 * (x0 + x1), 0.5 * (z0 + z1), std::atan2(-dz, dx), ch * 0.5 + 0.05, 0.08, g.sink, g.sink + h * wallShare(Dt, g.U(30 + k)), g.timberSlot(), MT_TIMBER);
            }
        else hmin = 0;
    }
    if (b.form == PF_FREE && g.lod < 2) {   // a second, smaller ring joined at the back
        Ring R2; R2.cz = -(rz + rz * 0.45); R2.rx = rx * 0.6; R2.rz = rz * 0.6; R2.h = h * 0.85; R2.thick = stone ? 0.6 : 0.2; R2.mat = stone ? MT_DRYSTONE : MT_TIMBER; R2.slot = stone ? 0 : g.timberSlot(); R2.segs = 8; R2.open = true; R2.dress = false; R2.k0 = 40;
        if (stone || Dt < 0.35) { const double h2 = ringWalls(g, R2); if (h2 > 0.92 * R2.h && Dt < 0.5) { Form& e = g.add(FK_LATHE, 1, 0, R2.cz, 0, R2.rx + 0.4, R2.rz + 0.4, g.sink + h2 - 0.1, g.sink + h2 + R2.rx * 1.1, g.roofSlot(MT_TIMBER), MT_TIMBER); e.p[0] = 0.04; } }
    }
    const double sinkSave = g.sink; hutRoof(g, rx, rz, hmin, h, stone); g.sink = sinkSave;
    if (g.lod == 0 && hmin > 0.92 * h) {   // A-04: the accent where the people put it
        const double am = (door + 0.5 * dseg) * TAU / N;
        if (g.accentOn(AP_BANDS) && (stone || Dt < 0.35)) for (int k = 0; k < N; k++) { if (inDoor(k)) continue; const double a0 = k * TAU / N, a1 = (k + 1) * TAU / N, x0 = std::sin(a0) * rx, z0 = std::cos(a0) * rz, x1 = std::sin(a1) * rx, z1 = std::cos(a1) * rz, dx = x1 - x0, dz = z1 - z0, ch = std::sqrt(dx * dx + dz * dz);
                                                                     g.box(0.5 * (x0 + x1) - dz / ch * 0.08, 0.5 * (z0 + z1) + dx / ch * 0.08, std::atan2(-dz, dx), ch * 0.5 + 0.1, stone ? 0.38 : 0.12, g.sink + h * 0.45, g.sink + h * 0.45 + 0.3, 1, stone ? MT_DRYSTONE : MT_TIMBER, 4); }
        if (g.accentOn(AP_DOORS) && g.D(MT_TIMBER) < 0.4) for (int s = -1; s <= 1; s += 2) { const double a = am + s * (PI * dseg / N + 0.12); g.lathe(0, std::sin(a) * (rx + 0.25), std::cos(a) * (rz + 0.25), 0.14, 0, h * 1.25, 1, MT_TIMBER, 6); }
        if (g.accentOn(AP_TOPS) && g.D(MT_TIMBER) < 0.4 && c.roof != RF_OPEN) g.lathe(0, 0, 0, 0.09, g.sink + h, g.sink + h + std::max(rx, rz) * 1.7, 1, MT_TIMBER, 4);
    }
    if (g.lod == 0) for (int k = 0; k < 4; k++) { double a = k * PI / 2 + 0.4; g.box(std::sin(a) * 0.45, std::cos(a) * 0.45, a, 0.17, 0.13, 0, 0.16, 0, MT_DRYSTONE, 3); }
    {   // R-412: its entrance, its pole by the door, its yard
        const double am = (door + 0.5 * dseg) * TAU / N;
        if (hmin > 0.92 * h || !stone) hutEntrance(g, rx, rz, am, h, stone);
        const double pa = am + 0.62, pr = std::max(rx, rz) + 1.5;
        hutPole(g, std::sin(pa) * pr, std::cos(pa) * pr);
        if (!b.joined && b.form != PF_FREE) hutRing(g, rx, rz, am);
    }
}

void genLonghouse(Gen& g) {   // walls of turf or planks, two rows of posts under its roof (the people's: a ridge of thatch, hipped, a turf vault), a hearth or two
    const Building& b = g.b; const Culture& c = g.c;
    const double Dt = g.D(MT_TIMBER), h = b.hWall;
    const bool turf = c.hut == 1 || g.U(3) < 0.4;
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = h; W.thick = turf ? 1.0 : 0.25; W.door = 0; W.doorPos = 0;
    W.mat = turf ? MT_EARTH : MT_TIMBER; W.slot = turf ? 0 : g.timberSlot(); W.part = turf ? 8 : 0; W.dress = false;
    W.profile = g.gv(GV_PROFILE) == WP_BULGING || g.gv(GV_PROFILE) == WP_BATTERED ? WP_BATTERED : WP_UPRIGHT;
    double hs[4] = {0, 0, 0, 0}, hmin = 0;
    if (turf || Dt < 0.45) hmin = walls4(g, W, hs);
    else if (g.lod == 0)   // the planks gone: the posts of the walls in their rows
        for (int side = -1; side <= 1; side += 2) { int k = 0; for (double t = -b.hd; t <= b.hd + 0.01; t += 1.6, k++) g.post(side * b.hw, t, h, 0.12, 200 + k * 2 + (side > 0)); }
    if (g.lod == 2) return;
    if (g.lod == 0) { int k = 0; for (double t = -b.hd + 1.5; t < b.hd - 1.0; t += 2.6, k++) for (int side = -1; side <= 1; side += 2) g.post(side * b.hw * 0.5, t, h + b.hw * 0.7, 0.14, 100 + k * 2 + (side > 0)); }
    if (hmin > 0.92 * h) {
        const int rf = c.roof;
        if (rf == RF_VAULTED || rf == RF_DOMED || rf == RF_FLAT) {   // a turf roof over its timbers, bowed
            const double De = materialDecay(MT_EARTH, c.ageYears * 0.1, c.wet);
            if (Dt < 0.5 && De < 0.8) { Form& e = g.add(FK_VAULT, 0, 0, 0, 0, b.hw + 0.3, b.hd + 0.3, hmin - 0.05, hmin - 0.05 + b.hw * (rf == RF_FLAT ? 0.35 : 0.8), 0, MT_EARTH, 8); e.p[0] = 0.6; e.p[1] = 1; e.broken = Dt < 0.3 ? 0 : 0.5; }
        } else roof(g, rf == RF_HIPPED || rf == RF_CONICAL || rf == RF_STEPPED ? RF_HIPPED : (rf == RF_SADDLE ? RF_SADDLE : RF_PITCHED), 0, 0, b.hw, b.hd, hmin, h, 1.05, MT_TIMBER);
    }
    if (g.lod == 0) for (int q = 0; q < 2; q++) { double z = (q ? -0.3 : 0.3) * b.hd; if (q && b.hd < 8) break; for (int k = 0; k < 4; k++) { double a = k * PI / 2 + 0.4; g.box(std::sin(a) * 0.45, z + std::cos(a) * 0.45, a, 0.17, 0.13, 0, 0.16, 0, MT_DRYSTONE, 3); } }
    if (hmin > 0.92 * h) hutEntrance(g, b.hw, b.hd, 0, h, turf);   // R-412: its entrance, its pole
    hutPole(g, b.hw + 1.3, b.hd + 1.2);
}

void genTentRing(Gen& g) {   // the stones that held a tent's hide down, in a ring; the hide itself gone within a lifetime
    const Building& b = g.b;
    if (g.lod > 0) return;
    const int n = 8 + (int)(g.U(1) * 5);
    for (int k = 0; k < n; k++) {
        double a = (k + 0.3 * g.U(10 + k)) * TAU / n, rr = b.hw * (0.92 + 0.16 * g.U(30 + k)), s = 0.18 + 0.14 * g.U(50 + k);
        g.box(std::sin(a) * rr, std::cos(a) * rr, a, s, s * 0.8, 0, s * 1.1, 0, MT_DRYSTONE, 3);
    }
    const double Dt = g.D(MT_TIMBER);
    if (Dt < 0.4) { Form& e = g.lathe(1, 0, 0, b.hw, 0, b.hWall, g.roofSlot(MT_TIMBER), MT_TIMBER); e.p[0] = 0.06; e.broken = Dt < 0.2 ? 0 : 0.5; }
}

// a standing stone: upright while the dry stone holds, then leaning (the tilt about its foot), then lying where it fell, broken up
// at the last; `k` its draws
void standingStone(Gen& g, double lx, double lz, double dh, double hx, double hz, double h, int k, bool tapered) {
    const double d = g.Dk(MT_DRYSTONE, k);
    if (d > 0.93) return;
    const int kind = tapered ? FK_PYRAMID : FK_BOX;
    if (d < 0.3) { Form& e = g.add(kind, 0, lx, lz, dh, hx, hz, 0, h, 0, MT_DRYSTONE, 5); e.p[1] = tapered ? 0.6 : 0; return; }
    const double fallDir = g.U(k + 300) * TAU;
    if (d < 0.55) { Form& e = g.add(kind, 0, lx, lz, fallDir, hx, hz, 0, h, 0, MT_DRYSTONE, 5); e.p[1] = tapered ? 0.6 : 0; e.tilt = 0.1 + 0.18 * (d - 0.3) / 0.25; return; }
    Form& e = g.add(kind, 0, lx, lz, fallDir, hx, hz, 0, h, 0, MT_DRYSTONE, 5); e.p[1] = tapered ? 0.6 : 0; e.tilt = PI / 2; e.broken = d > 0.78 ? 0.4 : 0;
}

// standing stones in a ring (A-04: their count a multiple of the people's number, their faces to the middle), a lintel over a pair here
// and there, an altar in the middle, a bank round it all; a ring of great posts instead where the people's centre was its assembly
// (`Building::style` 1)
void genStoneCircle(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double R = b.hw, h = b.hWall * (0.8 + 0.4 * g.gf(GV_PROPORTION));
    const int num = c.genome.number();
    const int n0 = std::max(9, (int)std::lround(TAU * R / (2.6 + 1.9 * g.U(1)))), n = std::max(num * 2, (n0 + num - 1) / num * num);
    if (b.style == 1) {   // posts, the wood henge
        for (int k = 0; k < n; k++) { double a = k * TAU / n; g.post(std::sin(a) * R, std::cos(a) * R, h * 1.3, 0.3, 100 + k); }
    } else {
        const bool lintels = g.U(2) < 0.35 && n >= 12;
        std::vector<int> up(n, 0);
        for (int k = 0; k < n; k++) {
            double a = k * TAU / n, w = 0.45 + 0.4 * g.U(10 + k), hh = h * (0.75 + 0.35 * g.U(40 + k));
            if (lintels) hh = h;
            standingStone(g, std::sin(a) * R, std::cos(a) * R, a, w, 0.3 + 0.2 * g.U(70 + k), hh, 100 + k, false);   // A-04: its broad face to the middle
            up[k] = g.Dk(MT_DRYSTONE, 100 + k) < 0.3;
        }
        if (lintels && g.lod < 2)   // a lintel on each upright pair of the even ones; fallen beside them otherwise
            for (int k = 0; k + 1 < n; k += 2) {
                double a = (k + 0.5) * TAU / n, len = R * std::sin(PI / n) + 0.6;
                if (up[k] && up[k + 1]) g.box(std::sin(a) * R * std::cos(PI / n), std::cos(a) * R * std::cos(PI / n), a, len, 0.35, h, h + 0.7, 0, MT_DRYSTONE, 4);
                else if (g.lod == 0 && g.Dk(MT_DRYSTONE, 400 + k) < 0.9) g.box(std::sin(a) * (R + 1.6), std::cos(a) * (R + 1.6), a + 0.3, len, 0.35, 0, 0.7, 0, MT_DRYSTONE, 3);
            }
    }
    if (g.lod == 0) g.box(0, 0, g.U(3) * PI, 1.1, 0.55, 0, 0.55, 0, MT_DRYSTONE, 3);   // the altar stone, lying
    if (R > 9 || g.gv(GV_EDGE) == ED_DITCHES) {   // the bank, its gap toward the front (the earth of a mound lasts ten times a wall's)
        const double De = materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet);
        Form& e = g.add(FK_SWEEP, 0, 0, 0, 0.25, R + 6, R + 6, 0, std::max(0.25, (g.gv(GV_EDGE) == ED_DITCHES ? 2.2 : 1.5) * (1 - 0.7 * De)), 0, MT_EARTH, 8); e.p[0] = TAU - 0.5; e.p[1] = 2.4; e.p[2] = 0.5;
    }
}

void genMenhir(Gen& g) {   // a standing stone, or a row of them along the heading, the tallest at its head (A-04: the people's number of them; an avenue
    const Building& b = g.b;   // of two rows where the people's centre was its market, `style` 1)
    const int num = g.c.genome.number();
    const int n = b.hd > 1.5 ? std::max(2, std::max(num, (int)std::lround(2 * b.hd / (3.0 + 1.5 * g.U(1))))) : 1;
    const double hp = 0.8 + 0.45 * g.gf(GV_PROPORTION);
    for (int row = 0; row < (b.style == 1 ? 2 : 1); row++)
        for (int k = 0; k < n; k++) {
            double t = n > 1 ? -b.hd + 2 * b.hd * k / (n - 1) : 0, f = n > 1 ? 1.0 - 0.45 * k / (n - 1) : 1.0;
            const double x = b.style == 1 ? (row ? 1 : -1) * 3.5 : 0;
            standingStone(g, x, t, g.U(20 + k) * 0.4 - 0.2, 0.55 + 0.45 * g.U(10 + k), 0.4 + 0.3 * g.U(30 + k), b.hWall * hp * f * (0.85 + 0.3 * g.U(40 + k)), 100 + k + row * 50, true);
        }
}

void genDolmen(Gen& g) {   // a chamber of slabs under its capstone, open at the front; the ring of what was its mound round it (A-04: the mound whole over it
    const Building& b = g.b;   // where the people's centre was a tomb, `style` 1, its passage open)
    const double hw = b.hw, hd = b.hd, h = b.hWall;
    const double d0 = g.Dk(MT_DRYSTONE, 10), d1 = g.Dk(MT_DRYSTONE, 11), d2 = g.Dk(MT_DRYSTONE, 12);
    // a slab fallen lies outward (T-02: it used to tip along the chamber, into it half the time: a later people's dolmens in its fields showed
    // it), its foot at its wall's line
    auto slab = [&](double lx, double lz, double dh, double out, double hx, double d) {
        if (d > 0.9) return;
        Form& e = g.box(lx, lz, dh, hx, 0.22, 0, h, 0, MT_DRYSTONE, 0);
        if (d > 0.55) { e.heading = g.b.heading + out; e.tilt = PI / 2; }
        else if (d > 0.3) e.tilt = 0.12;
    };
    slab(-hw, 0, PI / 2, -PI / 2, hd, d0); slab(hw, 0, PI / 2, PI / 2, hd, d1); slab(0, -hd, 0, PI, hw, d2);
    if (std::max(d0, std::max(d1, d2)) < 0.3) { Form& e = g.box(0, -0.2, 0, hw + 0.6, hd + 0.5, h, h + 0.7, 0, MT_DRYSTONE, 2); e.tilt = 0.04; }
    else if (g.lod == 0 && g.Dk(MT_DRYSTONE, 13) < 0.9) { Form& e = g.box(2 * hw + 1.5, 0.4, 0.4, hw + 0.6, hd + 0.5, 0, 0.7, 0, MT_DRYSTONE, 3); e.tilt = 0.18; }   // the capstone slid off beside it
    const double De = materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet);
    if (b.style == 1) {   // the mound whole round the chamber, its passage open at the front (the earth's dome, the chamber's sides inside it)
        const double R = hd * 2.6 + 2;
        Form& m = g.add(FK_SWEEP, 0, 0, 0, 0.35, R * 0.62, R * 0.62, 0, std::max(0.4, h * 1.6 * (1 - 0.5 * De)), 0, MT_EARTH, 8); m.p[0] = TAU - 0.7; m.p[1] = R * 0.45; m.p[2] = 0.8;
        return;
    }
    Form& m = g.add(FK_SWEEP, 0, 0, 0, 0.35, hd * 2.6 + 2, hd * 2.6 + 2, 0, std::max(0.25, 1.6 * (1 - 0.7 * De)), 0, MT_EARTH, 8); m.p[0] = TAU - 0.7; m.p[1] = 2.2; m.p[2] = 0.6;
}

void genPalisade(Gen& g) {   // a stretch of posts on an earth bank: the posts rot to their holes, the bank slumps (A-04: a ditched people's bank alone, higher)
    const Building& b = g.b;
    const double Dt = g.D(MT_TIMBER), h = b.hWall;
    const double De = materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet);
    const bool ditch = g.gv(GV_EDGE) == ED_DITCHES;
    Form& bank = g.add(FK_SWEEP, 1, 0, 0, PI / 2, 0, b.hw, 0, std::max(0.25, (ditch ? 2.2 : 1.0) * (1 - 0.6 * De)), 0, MT_EARTH, 8); bank.p[1] = ditch ? 3.2 : 1.8; bank.p[2] = 0.5;
    if (ditch) return;
    if (Dt < 0.35) { Form& e = g.box(0, 0, 0, b.hw, 0.2, 0.8, 0.8 + h, g.timberSlot(), MT_TIMBER); e.broken = 0.15; return; }
    if (g.lod > 0) return;
    int k = 0;
    for (double t = -b.hw + 0.4; t < b.hw; t += 1.5, k++) g.post(t, 0, 0.8 + h * 0.6, 0.15, 10 + k);
}
}   // namespace

// ---- the industrial age -------------------------------------------------------------------------------------------------------

namespace {
// the walls of the industrial age: brick or the rock (the palette says which colour: A-04 the people's brick, or its stucco), the
// people's windows, a roof of slate on timber
int brickOf(const Culture& c) { return c.family == 1 && c.age <= AGE_MEDIEVAL ? MT_EARTH : MT_MASONRY; }
// the doorway's place on a front of `L` half length (a row's, a block's): off the middle by the building's draw, or the middle
double frontDoorPos(const Building& b, double L) { return b.centred ? 0.0 : (L - 1.6) * (u01(b.seed, 20) - 0.5) * 1.0; }

// B-415: a chimney stack of brick through an industrial people's roof, from `y0` to `y1` at (x, z) (its half extents across and along), and
// its two pots: the age's own tops
void stack(Gen& g, double x, double z, double hx, double hz, double y0, double y1, int mat) {
    g.box(x, z, 0, hx, hz, y0, y1, 0, mat);
    if (g.lod == 0) for (int k = -1; k <= 1; k += 2) g.lathe(0, x + (hx > hz ? k * hx * 0.5 : 0), z + (hx > hz ? 0 : k * hz * 0.5), 0.13, y1, y1 + 0.55, 3, mat, 6);
}

// a terraced row: brick walls of two or three storeys, windows, its roof along the street (A-04: a row of gables to the street, a house to
// each, where the people built in cells; crow steps on a people of steps), stacks on the party walls. B-415: the age's slate roof (a people
// of flat roofs behind a parapet), its brick gables standing on its walls after the roof, the stacks' pots
void genRow(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm * 0.5;
    const bool parapet = parapetRoof(c);
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.45; W.door = 0; W.doorPos = frontDoorPos(b, b.hw); W.mat = mat; W.pattern = 1; W.parapet = parapet ? 0.8 : 0;
    const int rf = ageRoof(c, c.roof);
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int houses = std::max(2, (int)std::lround(b.hw * 2 / 5.5));
    const bool cells = !parapet && (g.gv(GV_MODULAR) == 1 || c.roof == RF_STEPPED || g.gv(GV_FRONTAGE) == FR_GABLES);   // R-412: a gable a house to the street
    const int rm = roofMaterial(g, rf, mat);
    double rise = 0;
    if (cells) {   // a roof a house, gabled to the street
        for (int k = 0; k < houses; k++) {
            const double t = -b.hw + (k + 0.5) * 2 * b.hw / houses, half = b.hw / houses;
            roof(g, RF_PITCHED, t, 0, half, b.hd, hmin, b.hWall, 0.75, rm);
            Walls Wk = W; Wk.cx = t; Wk.hw = half; gables(g, Wk, hs, 0.75);
            if (hmin > 0.92 * b.hWall && g.lod == 0 && (c.roof == RF_STEPPED || g.gv(GV_MOTIF) == MO_STEPS) && g.gv(GV_GABLES) != GB_STEPPED)   // crow steps up the gable's front
                for (int q = 0; q < 3; q++) { const double w = half * (0.85 - 0.28 * q); g.box(t, b.hd, 0, w, 0.3, g.sink + hmin + q * half * 0.28, g.sink + hmin + (q + 1) * half * 0.28, 0, mat, 4); }
        }
    } else {
        rise = roof(g, rf, 0, 0, b.hw, b.hd, hmin, b.hWall, parapet ? 0.35 : 0.7, rm);
        if (rf == RF_PITCHED) gables(g, W, hs, 0.7);   // the row's two ends
    }
    houseDress(g, W, hs, hmin, rf, rise, rm, !cells && !parapet);   // R-412
    if (g.lod < 2 && hmin > 0.92 * b.hWall) {   // the chimney stacks on the party walls, through the ridge
        for (int k = 0; k <= houses; k++) stack(g, (-b.hw + 2 * b.hw * k / houses) * 0.97, 0, 0.35, 0.7, g.sink + hmin, g.sink + hmin + b.hd * 0.7 + 1.1, mat);
        if (g.gv(GV_TOPS) == TP_FINIALS) houseTops(g, cells ? RF_FLAT : rf, 0, 0, b.hw, b.hd, g.sink + hmin, rise, rm, mat);
    }
    double dx, dz; doorMid(0, W.doorPos, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
    foundation(g, b.hw, b.hd, 0, W.doorPos, mat);
    yardOf(g, b.hw, b.hd, 0, W.doorPos, mat);   // R-412
    drifts(g, b.hw, b.hd, Dm);
}

// four to six storeys round a stair, a cornice, its roof. B-415: the age's hipped slate (behind a parapet for a people of flat roofs), its
// stacks
void genTenement(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm * 0.5;
    const bool parapet = parapetRoof(c);
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.55; W.door = 0; W.doorPos = frontDoorPos(b, b.hw); W.mat = mat; W.pattern = 1; W.parapet = parapet ? 0.9 : 0;
    double hs[4]; const double hmin = walls4(g, W, hs);
    if (hmin > 0.92 * b.hWall && g.lod < 2) {
        g.box(0, 0, 0, b.hw + 0.4, b.hd + 0.4, g.sink + hmin - 0.6, g.sink + hmin, g.accentOn(AP_TOPS) ? 1 : 0, mat, 2);   // the cornice
        const int rf = RF_HIPPED, rm = roofMaterial(g, rf, mat);
        const double rise = roof(g, rf, 0, 0, b.hw * 0.95, b.hd * 0.95, hmin, b.hWall, parapet ? 0.3 : 0.5, rm);
        for (int q = 0; q < 4; q++) stack(g, (q % 2 ? 1 : -1) * b.hw * 0.62, (q < 2 ? 1 : -1) * b.hd * 0.3, 0.6, 0.35, g.sink + hmin, g.sink + hmin + rise * 0.75 + 1.4, mat);
        if (g.gv(GV_TOPS) == TP_FINIALS) houseTops(g, rf, 0, 0, b.hw, b.hd, g.sink + hmin, rise, rm, mat);
        houseDress(g, W, hs, hmin, rf, rise, rm, !parapet);   // R-412
    } else houseDress(g, W, hs, hmin, RF_HIPPED, 0, MT_TIMBER, false);
    double dx, dz; doorMid(0, W.doorPos, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
    foundation(g, b.hw, b.hd, 0, W.doorPos, mat);
    yardOf(g, b.hw, b.hd, 0, W.doorPos, mat);   // R-412
}

// two storeys under its roof, its chimney (A-04: on the people's plan: a round villa, an L, a court). B-415: the age's slate roof (behind a
// parapet for a people of flat roofs), its gables, a chimney at each end of a big one
void genVilla(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm * 0.5;
    const int rf = ageRoof(c, c.roof), rm = roofMaterial(g, rf, mat);
    if (b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE) {
        Ring R; R.rx = b.hw; R.rz = b.form == PF_ELLIPSE ? b.hd : b.hw; R.h = b.hWall; R.thick = 0.45; R.mat = mat; R.pattern = 1;
        R.segs = ringSegs(b.form, b.sides, std::min(R.rx, R.rz)); R.doorSegs = ringDoorSegs(std::min(R.rx, R.rz), R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs);
        const double hmin = ringWalls(g, R);
        roundRoof(g, rf, 0, 0, R.rx + 0.3, R.rz + 0.3, R.segs, hmin, b.hWall, 0.62, rm);
        foundation(g, R.rx, R.rz, 0, 0, mat);
        ringDress(g, R, hmin);   // R-412
        yardOf(g, R.rx, R.rz, -1, 0, mat);
        return;
    }
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.45; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 1; W.parapet = parapetRoof(c) ? 0.8 : 0;
    double hs[4]; const double hmin = rectBody(g, W, rf, rm, hs, false);
    if (hmin > 0.92 * b.hWall && g.lod < 2) {
        const double top = g.sink + hmin + std::min(b.hw, b.hd) * 0.62 + 1.2;
        stack(g, b.hw * 0.55, -b.hd * 0.3, 0.45, 0.45, g.sink + hmin, top, mat);
        if (b.hw > 5) stack(g, -b.hw * 0.55, -b.hd * 0.3, 0.45, 0.45, g.sink + hmin, top, mat);
    }
    double dx, dz; doorMid(0, 0, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
    foundation(g, b.hw, b.hd, 0, 0, mat);
    yardOf(g, b.hw, b.hd, 0, 0, mat);   // R-412
}

// a church: a nave under its roof (the people's), its towers where the people's symmetry sets them, their spires (a people of spires:
// tall), or a dome over the crossing where its focus is a dome
void genChurch(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm * 0.5;
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.8; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 3; W.glyphs = true;   // B-415: its own windows, tall
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int rf = c.roof == RF_FLAT || c.roof == RF_OPEN || c.roof == RF_SADDLE || c.roof == RF_STEPPED ? RF_PITCHED : c.roof;   // B-415: a saddle's shell and a stepped crown gabled
    roof(g, rf, 0, 0, b.hw, b.hd, hmin, b.hWall, 1.0, roofMaterial(g, rf, mat));
    if (rf == RF_PITCHED) gables(g, W, hs, 1.0);
    entranceAt(g, W, wallsHalf(g, W), W.h);   // R-412: its doorway's and its corners' (its towers stand at its front's corners: no turrets)
    if (g.gv(GV_CORNERS) != CO_TURRETS) cornersOf(g, W, hs, wallsHalf(g, W), roofMaterial(g, rf, mat));
    const double hgt = 0.7 + 0.8 * g.gf(GV_HEIGHT), tr = std::max(3.0, b.hw * 0.5);
    if (g.gv(GV_FOCUS) == FO_DOME && hmin > 0.92 * b.hWall) {   // a dome on a drum over the crossing
        const double rd = b.hw * 0.85, Dd = g.D(mat);
        Form& d = g.lathe(0, 0, -b.hd * 0.2, rd, g.sink + hmin, g.sink + hmin + 3.5, 0, mat, 8); d.pattern = 2;
        if (Dd < 0.6) { Form& e = g.add(FK_DOME, ageDome(g, g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : 0), 0, -b.hd * 0.2, 0, rd, rd, g.sink + hmin + 3.5, g.sink + hmin + 3.5 + rd, g.roofSlot(mat), mat, 2); e.broken = Dd < 0.3 ? 0 : 0.5; }
    }
    const int sym = g.gv(GV_SYMMETRY);
    const double side = g.U(17) < 0.5 ? -1 : 1;
    std::vector<std::pair<double, double>> at;
    if (sym == SY_BILATERAL) at = {{-(b.hw + tr - 0.5), b.hd - tr}, {b.hw + tr - 0.5, b.hd - tr}};
    else if (sym == SY_ROTATIONAL) at = {{side * (b.hw + tr - 0.5), b.hd - tr}, {-side * (b.hw + tr - 0.5), -b.hd + tr}};
    else at = {{side * (b.hw + tr - 0.5), b.hd - tr}};   // the tower at a front corner, beside the nave's doorway
    const bool spire = g.gv(GV_FOCUS) == FO_SPIRE || g.U(16) < 0.6;
    for (size_t k = 0; k < at.size(); k++) {
        const double tH = b.hWall * 2.6 * hgt;
        TowerOut T = towerShaft(g, at[k].first, at[k].second, tr, tH, mat, 15 + (int)k * 3, 2);
        if (T.whole) towerTop(g, at[k].first, at[k].second, T.rTop, T.h, tH, mat, spire, b.hWall * (g.gv(GV_FOCUS) == FO_SPIRE ? 4.0 : 3.2) * hgt);
    }
    motifOn(g, 0, 0, b.hw, b.hd, std::min(hmin, b.hWall), b.hWall, mat);
    double dx, dz; doorMid(0, 0, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
}

// a town hall: a long front of tall windows, its roof, over its middle a clock tower with a dome on it (`style` 0), a great dome on a
// drum (1: the people's assembly), or a spire (2)
void genTownHall(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm * 0.5;
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.8; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 2; W.glyphs = true;
    W.parapet = parapetRoof(c) ? 1.1 : 0;   // B-415: a flat roof's people's hipped behind its parapet; a saddle and steps hipped
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int rf = c.roof == RF_PITCHED || c.roof == RF_FLAT || c.roof == RF_OPEN || c.roof == RF_SADDLE || c.roof == RF_STEPPED ? RF_HIPPED : c.roof;
    roof(g, rf, 0, 0, b.hw, b.hd, hmin, b.hWall, 0.45, roofMaterial(g, rf, mat));
    entranceAt(g, W, wallsHalf(g, W), W.h);   // R-412: its doorway's and its corners'
    cornersOf(g, W, hs, wallsHalf(g, W), roofMaterial(g, rf, mat));
    if (hmin > 0.92 * b.hWall) {
        const double hgt = 0.7 + 0.8 * g.gf(GV_HEIGHT);
        if (b.style == 1) {   // the assembly's dome on its drum
            const double rd = std::min(b.hw, b.hd) * 0.8, Dd = g.D(mat);
            Form& d = g.lathe(0, 0, 0, rd, g.sink + hmin, g.sink + hmin + 4.0, 0, mat, 0); d.pattern = 2;
            if (Dd < 0.6) { Form& e = g.add(FK_DOME, ageDome(g, g.gv(GV_OPENING) == OP_ARCHED ? 2 : 0), 0, 0, 0, rd, rd, g.sink + hmin + 4.0, g.sink + hmin + 4.0 + rd * 1.05, g.roofSlot(mat), mat, 2); e.broken = Dd < 0.3 ? 0 : 0.5; if (Dd < 0.3) topsAt(g, {{0, 0}}, g.sink + hmin + 4.0 + rd * 1.05, 1.5, mat); }
        } else {
            const double tr = 3.2, tH = b.hWall * (b.style == 2 ? 2.2 : 1.35) * hgt;
            const double sinkSave = g.sink; g.sink += hmin;
            TowerOut T = towerShaft(g, 0, b.hd - tr, tr, tH, mat, 15, 2, false);
            if (T.whole) {
                if (b.style == 2) towerTop(g, 0, b.hd - tr, T.rTop, T.h, tH, mat, true, tH * 1.2);
                else if (g.D(MT_IRON) < 0.6) { Form& d = g.add(FK_DOME, ageDome(g, 3), 0, b.hd - tr, 0, tr, tr, g.sink + T.h, g.sink + T.h + tr * 1.6, g.roofSlot(MT_IRON), MT_IRON, 2); d.broken = g.D(MT_IRON) < 0.3 ? 0 : 0.5; }
            }
            g.sink = sinkSave;
        }
    }
    motifOn(g, 0, 0, b.hw, b.hd, std::min(hmin, b.hWall), b.hWall, mat);
    double dx, dz; doorMid(0, 0, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
}

// a station: a front block over the street and the train shed behind it: an iron vault, its glass gone at once, its ribs left (A-04: a
// people of arches fronts it with an arcade; `style` 1 a market hall, the vault alone with brick ends)
void genStation(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat), Di = g.D(MT_IRON);
    g.sink = -c.buried * Dm * 0.5;
    const double fd = b.style == 1 ? 0 : 5.5, shedHz = b.hd - fd;   // the front block's half depth; the shed fills the rest
    double hs[4] = {b.hWall, b.hWall, b.hWall, b.hWall};
    Walls W; W.cz = b.hd - fd; W.hw = b.hw; W.hd = fd; W.h = b.hWall; W.thick = 0.8; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 2;
    if (b.style == 0) {
        W.parapet = parapetRoof(c) ? 1.0 : 0;
        const double hmin = walls4(g, W, hs);
        const int rf = ageRoof(c, c.roof);   // B-415: the age's
        roof(g, rf, 0, W.cz, b.hw, fd, hmin, b.hWall, 0.45, roofMaterial(g, rf, mat));
        if (rf == RF_PITCHED) gables(g, W, hs, 0.45);
        entranceAt(g, W, wallsHalf(g, W), W.h);   // R-412
        cornersOf(g, W, hs, wallsHalf(g, W), roofMaterial(g, rf, mat));
        if (c.technique == TQ_ARCH_VAULT && g.lod < 2 && hmin > 0.92 * b.hWall) {   // an arcade along its front
            Form& e = g.add(FK_ARCH, archSub(g), 0, b.hd + 2.2, 0, b.hw, 0.4, g.sink, g.sink + std::min(b.hWall * 0.55, 6.0), g.doorSlot(), mat, 4);
            e.p[0] = std::max(3.0, std::round(b.hw / 2.4)); e.p[1] = 0.36; e.p[2] = 2.6;
        }
    } else {   // the market hall's two brick ends
        for (int s = -1; s <= 1; s += 2) { const double h = std::max(0.5, b.hWall * wallShare(Dm, g.U(50 + s))); if (s > 0) { g.box(-b.hw * 0.6, b.hd, 0, b.hw * 0.4, 0.4, g.sink, g.sink + h, 0, mat); g.box(b.hw * 0.6, b.hd, 0, b.hw * 0.4, 0.4, g.sink, g.sink + h, 0, mat); } else g.box(0, -b.hd, 0, b.hw, 0.4, g.sink, g.sink + h, 0, mat); }
    }
    const double span = std::min(b.hw * 0.9, 18.0), sz = -fd, H = span * (0.7 + 0.4 * g.gf(GV_PROPORTION)) + 4;
    const int vs = c.technique == TQ_ARCH_VAULT ? 0 : 2;   // round, or a frame's parabola
    if (Di < 0.25) { Form& v = g.add(FK_VAULT, vs, 0, sz, 0, span, shedHz, 0, H, g.ironSlot(), MT_IRON); v.p[0] = 0.3; v.p[1] = 0; }
    else if (Di < 0.85 && g.lod < 2) {   // the ribs, one by one
        const int n = std::max(3, (int)(shedHz * 2 / 7));
        for (int k = 0; k < n; k++) {
            if (g.Dk(MT_IRON, 40 + k) > 0.7) continue;
            double z = sz - shedHz + (k + 0.5) * 2 * shedHz / n;
            Form& v = g.add(FK_VAULT, vs, 0, z, 0, span, 0.22, 0, H, g.ironSlot(), MT_IRON); v.p[0] = 0.35; v.p[1] = 0;
        }
    }
    if (g.lod == 0 && b.style == 0) for (int k = -1; k <= 1; k += 2) g.box(k * span * 0.35, sz, 0, 2.2, shedHz * 0.95, 0, 1.0, 0, mat, 2);   // the platforms
    if (b.style == 0) { double dx, dz; doorMid(0, 0, b.hw, fd, dx, dz); rubble(g, b.hw, fd, hs, b.hWall, dx, dz + W.cz, 0, mat); }
}

// a long mill of five storeys, its windows in a grid, its roof, its chimney at the end, snapped and fallen when worn. B-415: the age's slate
// roof (a row of gables across it where the people built in cells, a parapet round a people of flat roofs' low one), its brick gables, its
// chimney's lip black with soot (a painted people's white chimneys stood in rows like columns: BUBAS read as medieval)
void genMill(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat);
    g.sink = -c.buried * Dm * 0.5;
    // B-414: its doorway in its front to the street (on its side a neighbour of the works stood against it) and its chimney within its own
    // ground at its back end (beside it, it stood at a neighbour's doorway): an industrial people's towns are all mills and rows now
    const bool parapet = parapetRoof(c);
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.7; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 2; W.parapet = parapet ? 1.0 : 0;
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int rf = ageRoof(c, c.roof), rm = roofMaterial(g, rf, mat);
    if (hmin > 0.92 * b.hWall) {
        if (g.gv(GV_MODULAR) == 1 && !parapet) {   // gables across it in a row, a roof a bay
            const int n = clampi((int)std::lround(b.hd / 3.5), 3, 8);
            for (int k = 0; k < n; k++) roof(g, RF_PITCHED, 0, -b.hd + (k + 0.5) * 2 * b.hd / n, b.hw, b.hd / n, hmin, b.hWall, 0.6, rm);
        } else {
            const double rise = roof(g, rf, 0, 0, b.hw, b.hd, hmin, b.hWall, parapet ? 0.3 : 0.5, rm);
            if (rf == RF_PITCHED) gables(g, W, hs, 0.5);
            if (!parapet) rooflineOf(g, W, hmin, rf, rise, rm);   // R-412: its ventilators, its cresting
        }
    }
    cornersOf(g, W, hs, wallsHalf(g, W), rm);
    const double cr = millChimneyR(b), cH = b.hWall * 3.4 * (0.7 + 0.6 * g.gf(GV_HEIGHT)), cz = -(b.hd - cr - 0.8), h = cH * tallShare(Dm * 1.1, g.U(33));
    Form& ch = g.lathe(5, 0, cz, cr, g.sink, g.sink + h, 0, mat, g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : 0); ch.p[0] = 0.6; ch.broken = h < cH * 0.97 ? 0.25 : 0;
    if (h < cH * 0.97 && g.lod == 0) { g.fallen(FK_LATHE, 0, 0, cz, PI / 2 + (g.U(34) - 0.5), cr * 0.75, (cH - h) * 0.5, 0, mat); g.heap(0, cz, cr * 2, cr * 2, std::min(3.5, (cH - h) * 0.05), 0, mat); }
    if (h >= cH * 0.97 && g.lod < 2) g.lathe(0, 0, cz, cr * 0.72, g.sink + cH - std::min(1.5, 0.04 * cH) - 1.6, g.sink + cH + 0.05, 3, mat);   // the soot of its lip
    if (g.banded() && g.lod < 2 && h >= cH * 0.97) for (int k = 1; k <= 2; k++) { const double t = 0.35 + 0.25 * k, rr = cr * (1 - 0.4 * t) + 0.15; Form& e = g.add(FK_TORUS, 0, 0, cz, 0, rr, rr, g.sink + cH * t, g.sink + cH * t + 0.8, 1, mat, 4); e.p[0] = 0.35; }   // bands round the chimney, under its lip
    double dx, dz; doorMid(0, 0, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, mat);
}

void genGasometer(Gen& g) {   // a ring of iron columns with their girders (A-04: twice the people's number), the bell inside: the sheet goes first, the frame after
    const Building& b = g.b;
    const double R = b.hw, H = b.hWall, Di = g.D(MT_IRON);
    const int n = clampi(2 * g.c.genome.number(), 6, 16), is = g.ironSlot();
    for (int k = 0; k < n; k++) {
        double a = k * TAU / n, d = g.Dk(MT_IRON, 10 + k);
        if (d > 0.85) continue;
        double h = H * (d < 0.35 ? 1 : 1 - (d - 0.35) * 1.4 * g.U(30 + k));
        Form& e = g.lathe(0, std::sin(a) * (R + 1), std::cos(a) * (R + 1), 0.45, 0, std::max(1.0, h), is, MT_IRON, 6); e.broken = h < H ? 0.3 : 0;
    }
    if (Di < 0.25 && g.lod < 2) for (int q = 1; q <= 2; q++) { Form& t = g.add(FK_TORUS, 0, 0, 0, 0, R + 1, R + 1, H * q / 2 - 0.6, H * q / 2, is, MT_IRON); t.p[0] = 0.3; t.broken = Di < 0.3 ? 0 : 0.6; }
    const double Db = g.D(MT_IRON) * 1.4;
    if (Db < 0.8) { Form& e = g.lathe(0, 0, 0, R, 0, H * (0.35 + 0.5 * g.U(2)), g.roofSlot(MT_IRON), MT_IRON); e.broken = Db < 0.3 ? 0 : clampd(Db, 0.3, 0.9); }
}

void genWaterTower(Gen& g) {   // a shaft of brick in the people's section and the iron tank on it
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Dm = g.D(mat), Di = g.D(MT_IRON);
    const double r = b.hw * 0.55, H = b.hWall, h = H * tallShare(Dm, g.U(10));
    const int sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : (g.gv(GV_PLAN) == PF_CIRCLE ? 0 : 8);
    Form& t = g.lathe(g.gv(GV_PROFILE) == WP_BATTERED ? 1 : 0, 0, 0, r, 0, h, 0, mat, sides); t.p[0] = 0.8; t.pattern = 2; t.broken = h < H * 0.97 ? 0.3 : 0;
    if (h >= H * 0.97 && Di < 0.6) { Form& k = g.lathe(4, 0, 0, b.hw, H, H + b.hw * 1.1, g.roofSlot(MT_IRON), MT_IRON); k.p[0] = 0.4; k.broken = Di < 0.3 ? 0 : 0.5; if (Di < 0.3) topsAt(g, {{0, 0}}, H + b.hw * 1.1, 1.0, MT_IRON); }
    else if (g.lod == 0 && Di < 0.8) g.fallen(FK_LATHE, 0, 0, 0, g.U(11) * TAU, b.hw * 0.9, b.hw * 1.6, g.roofSlot(MT_IRON), MT_IRON);
}

void genHeadframe(Gen& g) {   // the iron frame over a shaft with its wheel, the engine house with its stack, the spoil heap
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Di = g.D(MT_IRON), Dm = g.D(mat);
    const double H = b.hWall;
    const int ex = g.gv(GV_EXPRESSION);   // R-412: an iron lattice (a people of frames), a brick tower over the shaft (a massive people's), two legs of an A-frame
    if (ex == EX_MASSIVE) {
        const double Dh = g.D(mat), share = tallShare(Dh, g.U(41));
        Form& t = g.box(0, b.hd * 0.45, 0, 3.6, 3.6, 0, H * share, 0, mat); t.pattern = 2; t.broken = share < 0.97 ? 0.3 : 0;
        if (share >= 0.97 && g.lod < 2) { Walls P; (void)P; g.box(0, b.hd * 0.45, 0, 3.85, 3.85, H, H + 0.6, g.topSlot(mat), mat, 4); }
    } else if (ex == EX_MONOLITH) {
        for (int s = -1; s <= 1; s += 2) { Form& l = g.add(FK_TRUSS, 0, 0, b.hd * 0.45 + s * 2.6, 0, 0.9, 0.9, 0, H * 1.02, g.ironSlot(), MT_IRON); l.sides = 4; l.p[0] = 0.6; l.p[2] = 0.12; l.tilt = -0.17 * s; l.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.95); }
    } else {
        Form& t = g.add(FK_TRUSS, 0, 0, b.hd * 0.45, 0, 3.5, 3.5, 0, H, g.ironSlot(), MT_IRON); t.sides = 4; t.p[0] = 0.35; t.p[2] = 0.18;
        t.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.95);
    }
    if (Di < 0.45) { Form& w = g.add(FK_TORUS, 0, 0, b.hd * 0.45, 0, 2.2, 2.2, H - 4.6, H, g.ironSlot(), MT_IRON); w.p[0] = 0.22; w.tilt = PI / 2; }
    Walls W; W.cz = -b.hd * 0.3; W.hw = 5; W.hd = 6.5; W.h = 8; W.thick = 0.6; W.door = 0; W.doorPos = 0; W.mat = mat; W.pattern = 2; W.k0 = 30;
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int rf = ageRoof(c, c.roof);   // B-415: the age's
    const double rise = roof(g, rf, 0, W.cz, W.hw, W.hd, hmin, W.h, 0.6, roofMaterial(g, rf, mat));
    if (rf == RF_PITCHED) gables(g, W, hs, 0.6);
    rooflineOf(g, W, hmin, rf, rise, roofMaterial(g, rf, mat));   // R-412: its ventilator, its cresting
    cornersOf(g, W, hs, wallsHalf(g, W), roofMaterial(g, rf, mat));
    const double sH = 26, h = sH * tallShare(Dm * 1.1, g.U(40));
    Form& s = g.lathe(5, 5.5, W.cz - 4, 1.2, 0, h, 0, mat); s.p[0] = 0.7; s.broken = h < sH * 0.97 ? 0.25 : 0;
    const double De = materialDecay(MT_EARTH, c.ageYears * 0.1, c.wet);
    Form& heap = g.lathe(1, -b.hw * 0.75, b.hd * 0.5, b.hw * 0.5, 0, b.hw * 0.5 * (0.75 - 0.25 * De), 0, MT_EARTH); heap.part = 8; heap.p[0] = 0.12;
}

void genObservatory(Gen& g) {   // a drum of masonry and its turning dome of iron sheet, which goes first
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = brickOf(c); const double Di = g.D(MT_IRON), rr = b.hw;
    const int segs = g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 5, 9) : 12;   // R-412: its drum the people's polygon, its dome the people's
    Ring R; R.rx = rr; R.rz = rr; R.h = b.hWall; R.thick = 0.8; R.mat = mat; R.segs = segs; R.doorSegs = ringDoorSegs(rr, segs); R.door = ringDoorFirst(b, segs, R.doorSegs); R.pattern = 2;
    const double hmin = ringWalls(g, R);
    const int prof = g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : (g.gv(GV_OPENING) == OP_ARCHED || g.gv(GV_OPENING) == OP_TRIANGULAR ? 2 : 0);
    if (hmin > b.hWall * 0.92 && Di < 0.6) { Form& d = g.add(FK_DOME, ageDome(g, prof), 0, 0, 0, rr + 0.4, rr + 0.4, hmin - 0.1, hmin - 0.1 + rr * (prof == 0 ? 0.95 : 1.25), g.roofSlot(MT_IRON), MT_IRON, 2); d.broken = Di < 0.3 ? 0 : 0.6; }
}
}   // namespace

// ---- the modern age -----------------------------------------------------------------------------------------------------------

namespace {
// a slab of flats: concrete walls in bands of windows, the glass long gone, its roof (the people's: a flat roof and its lift house, a
// gable, a vault, a saddle, ...) on the people's plan (a round block, a bent one, a block round its court); on pilotis where the people
// raised its houses; a mall (`style` 1): long and low under a shell or a vault
void genBlock(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double Dc = g.D(MT_CONCRETE);
    g.sink = -c.buried * Dc * 0.4;
    if (g.gv(GV_MASSING) == MS_BURIED) g.sink -= std::min(6.0, 0.25 * b.hWall);
    double y0 = 0;
    if (b.found == FD_STILTS && b.style == 0) y0 = stilts(g, b.hw, b.hd, 4.5) ? 4.5 : 0;
    // a mall's shell or vault (a people of domes or saddles its own), a slab's roof the age's (B-414)
    const int rf = b.style == 1 ? (c.roof == RF_DOMED || c.roof == RF_SADDLE || c.roof == RF_VAULTED ? c.roof : (c.technique == TQ_SHELL ? RF_SADDLE : RF_VAULTED)) : ageRoof(c, c.roof);
    const int rm = roofMaterial(g, rf, MT_CONCRETE);
    if (b.style == 0 && (b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE)) {
        Ring R; R.rx = b.hw; R.rz = b.form == PF_ELLIPSE ? b.hd : b.hw; R.h = b.hWall; R.thick = 0.45; R.mat = MT_CONCRETE; R.pattern = 1; R.y0 = y0;
        R.segs = ringSegs(b.form, b.sides, std::min(R.rx, R.rz)); R.doorSegs = ringDoorSegs(std::min(R.rx, R.rz), R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs);
        R.crenels = g.gv(GV_TOPS) == TP_CRENELS;
        const double hmin = ringWalls(g, R);
        const double rise = roundRoof(g, rf, 0, 0, R.rx + 0.2, R.rz + 0.2, R.segs, hmin, b.hWall, 0.5, rm);
        if (hmin >= 0.92 * b.hWall) roundTops(g, rf, g.sink + y0 + hmin, rise, 1.4, rm);
        if (g.lod == 0 && hmin < 0.7 * b.hWall) g.heap(0, -R.rz - 3, R.rx * 0.9, 4, std::min(5.0, (b.hWall - hmin) * 0.12), 0, MT_CONCRETE);
        return;
    }
    if (b.style == 0 && b.form == PF_COURT) {   // a block round its court
        Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.45; W.door = 0; W.doorPos = frontDoorPos(b, b.hw - 4); W.mat = MT_CONCRETE; W.pattern = 1; W.y0 = y0;
        double hs[4]; const double hmin = walls4(g, W, hs);
        Walls I = W; I.hw = b.hw - 6; I.hd = b.hd - 6; I.k0 = 30; I.glyphs = false;
        double hi[4]; const double him = walls4(g, I, hi);
        const double hr = std::min(hmin, him);
        if (hr > 0.95 * b.hWall) for (int s = 0; s < 4; s++) { const bool al = s % 2 == 0; g.box(al ? 0 : (s == 1 ? 1 : -1) * (b.hw - 3), al ? (s == 0 ? 1 : -1) * (b.hd - 3) : 0, 0, al ? b.hw + 0.15 : 3.15, al ? 3.15 : b.hd - 6, g.sink + y0 + hr - 0.3, g.sink + y0 + hr + 0.3, 0, MT_CONCRETE, 2); }
        if (g.lod == 0 && hmin < 0.7 * b.hWall) g.heap(0, -b.hd - 3, b.hw * 0.9, 4, std::min(5.0, (b.hWall - hmin) * 0.12), 0, MT_CONCRETE);
        return;
    }
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.4; W.door = 0; W.doorPos = frontDoorPos(b, b.hw); W.mat = MT_CONCRETE; W.pattern = b.style == 1 ? 2 : 1; W.y0 = y0;
    const bool bent = b.form == PF_FREE && b.style == 0;
    if (bent) { W.cz = b.hd * 0.55; W.hd = b.hd * 0.45; }   // a bar along the front and a wing back from one end (`buildingRoom` reads the bar)
    double hs[4];
    double hmin;
    if (rf == RF_FLAT && b.style == 0) {
        W.crenels = g.gv(GV_TOPS) == TP_CRENELS;
        hmin = walls4(g, W, hs);
        if (hmin > 0.95 * b.hWall) {   // the flat roof, its lift house (B-415: a lift house a module's where the people built in cells)
            const double se = g.slabEave(), th = g.c.genome.of(GV_EAVES) == EV_FLARED ? 0.9 : 0.3;   // R-412: its slab flush, a lip, deep, a cornice
            g.box(W.cx, W.cz, 0, W.hw + se, W.hd + se, g.sink + y0 + hmin - 0.3, g.sink + y0 + hmin + th, g.roofSlot(MT_CONCRETE), MT_CONCRETE, 2);
            const int nl = g.gv(GV_MODULAR) == 1 ? std::max(2, (int)std::lround(W.hw * 2 / 11)) : 1;
            if (g.lod < 2) for (int k = 0; k < nl; k++) g.box(W.cx + (nl == 1 ? W.hw * 0.3 : -W.hw + (k + 0.5) * 2 * W.hw / nl), W.cz, 0, nl == 1 ? 2.5 : std::min(2.5, W.hw / nl * 0.55), std::min(2.2, W.hd - 0.3), g.sink + y0 + hmin + 0.3, g.sink + y0 + hmin + 3.2, 0, MT_CONCRETE);
            houseTops(g, RF_FLAT, W.cx, W.cz, W.hw, W.hd, g.sink + y0 + hmin + 0.3, 0, MT_CONCRETE, MT_CONCRETE);
        }
        if (b.style == 0 && !bent) houseDress(g, W, hs, hmin, RF_FLAT, 0.3, MT_CONCRETE, true);   // R-412
    } else hmin = rectBody(g, W, rf, rm, hs, b.style == 0);
    if (bent && g.lod < 2) {   // the wing bent back from one end
        Walls V = W; V.cx = (g.U(43) < 0.5 ? -1 : 1) * b.hw * 0.7; V.cz = -b.hd * 0.45; V.hw = b.hw * 0.3; V.hd = b.hd * 0.55; V.door = -1; V.k0 = 30; V.glyphs = false; V.crenels = false;
        double hv[4]; const double hmv = walls4(g, V, hv);
        roof(g, rf == RF_OPEN ? RF_FLAT : rf, V.cx, V.cz, V.hw, V.hd, hmv, V.h, 0.5, rm);
    }
    if (g.lod == 0 && hmin < 0.7 * b.hWall) g.heap(0, -b.hd - 3, b.hw * 0.9, 4, std::min(5.0, (b.hWall - hmin) * 0.12), 0, MT_CONCRETE);
    foundation(g, b.hw, b.hd, 0, W.doorPos, MT_CONCRETE);
    if (b.style == 0) yardOf(g, b.hw, b.hd, 0, W.doorPos, MT_CONCRETE);   // R-412
}

void genSuburb(Gen& g) {   // a house of one or two storeys, its roof (the people's, or another where its houses vary), on the people's plan
    const Building& b = g.b; const Culture& c = g.c;
    const double Dm = g.D(MT_MASONRY);
    g.sink = -c.buried * Dm * 0.4;
    const int rf = ageRoof(c, houseRoof(g)), rm = roofMaterial(g, rf, MT_MASONRY);   // B-414: the age's
    if (b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE) {
        Ring R; R.rx = b.hw; R.rz = b.form == PF_ELLIPSE ? b.hd : b.hw; R.h = b.hWall; R.thick = 0.35; R.mat = MT_MASONRY; R.pattern = 1;
        R.segs = ringSegs(b.form, b.sides, std::min(R.rx, R.rz)); R.doorSegs = ringDoorSegs(std::min(R.rx, R.rz), R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs);
        const double hmin = ringWalls(g, R);
        roundRoof(g, rf, 0, 0, R.rx + 0.3, R.rz + 0.3, R.segs, hmin, b.hWall, 0.5, rm);
        foundation(g, R.rx, R.rz, 0, 0, MT_MASONRY);
        ringDress(g, R, hmin);   // R-412
        yardOf(g, R.rx, R.rz, -1, 0, MT_MASONRY);
        return;
    }
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.35; W.door = 0; W.doorPos = frontDoorPos(b, b.hw); W.mat = MT_MASONRY; W.slot = 0; W.pattern = 1;
    double hs[4]; rectBody(g, W, rf, rm, hs, false);
    double dx, dz; doorMid(0, W.doorPos, b.hw, b.hd, dx, dz);
    rubble(g, b.hw, b.hd, hs, b.hWall, dx, dz, 0, MT_MASONRY);
    foundation(g, b.hw, b.hd, 0, W.doorPos, MT_MASONRY);
    yardOf(g, b.hw, b.hd, 0, W.doorPos, MT_MASONRY);   // R-412
}

// a tower: its lobby a storey of walls round a room with its doorway, the shaft over it to `H` in the people's form (`Building::style`:
// 0 a slab, 1 setbacks, 2 a cylinder, 3 a prism of the people's number, 4 boxes cantilevered off a core, 5 a shell's waist), its tops
// (a mast, a spire, a crown of merlons, a statue); the glass of its curtain gone at once (the bands of its frame empty), the concrete
// falling over centuries: the shaft snapped lower and lower, its top lying across the ground beside it, to a stump
void genHighrise(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double Dc = g.D(MT_CONCRETE), H = b.hWall, lob = 6.0;
    g.sink = -c.buried * Dc * 0.3;
    const double share = tallShare(Dc, g.U(10)), top = std::max(lob + 2, H * share);
    const int form = b.style;
    // the lobby stands whole while the shaft stands on it, and falls with the rest once the shaft is gone
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = lob; W.thick = 0.5; W.door = 0; W.doorPos = 0; W.mat = MT_CONCRETE; W.pattern = 2; W.k0 = 50; W.whole = Dc < 0.9; W.dress = false;
    double hs[4];
    if (g.lod == 0 || Dc >= 0.9) walls4(g, W, hs);
    const double y0 = g.sink + (g.lod == 0 ? lob : 0);
    const int sides = clampi(c.genome.number(), 3, 8);
    if (top > lob + 1 && Dc < 0.9) {
        const double brk = share < 0.97 ? clampd(4.0 / std::max(8.0, top - lob), 0.05, 0.4) : 0;
        const double cut = (g.sink + top - y0) / std::max(1.0, g.sink + H - y0);   // the share of the shaft standing
        if (form == 1 && cut >= 0.36) {   // setbacks, their top tiers gone (a pyramid's break cuts at most two thirds of it)
            Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw, b.hd, y0, g.sink + H, 0, MT_CONCRETE); e.p[0] = clampi(c.genome.number(), 3, 6); e.p[1] = 0.45; e.p[2] = 1; e.pattern = 2;
            e.broken = share < 0.97 ? clampd((1 - cut) / 0.65, 0.02, 1) : 0;
        } else if (form == 2 || form == 3) { Form& e = g.add(FK_LATHE, 0, 0, 0, 0, std::min(b.hw, b.hd), b.form == PF_ELLIPSE ? std::max(b.hw, b.hd) : std::min(b.hw, b.hd), y0, g.sink + top, 0, MT_CONCRETE); e.sides = form == 3 ? sides : 0; e.pattern = 2; e.broken = brk; }
        else if (form == 4) {   // boxes stacked off a core, each turned or slid off the one under it
            const int n = clampi((int)((H - lob) / 40) + 2, 3, 7);
            for (int k = 0; k < n; k++) {
                const double ya = y0 + (g.sink + H - y0) * k / n, yb = y0 + (g.sink + H - y0) * (k + 1) / n;
                if (ya >= g.sink + top) break;
                const double off = (k % 2 ? 1 : -1) * b.hw * 0.22;
                Form& e = g.box(off, -off * 0.5, k % 2 ? 0.0 : 0.12, b.hw * 0.8, b.hd * 0.85, ya, std::min(yb, g.sink + top), 0, MT_CONCRETE); e.pattern = 2; e.broken = yb > g.sink + top ? brk : 0;
            }
        } else if (form == 5) { Form& e = g.lathe(3, 0, 0, std::min(b.hw, b.hd), y0, g.sink + H, 0, MT_CONCRETE); e.p[0] = 0.66; e.p[1] = 0.62; e.pattern = 2; e.broken = share < 0.97 ? clampd(1 - cut, 0.05, 0.95) : 0; }
        else { Form& e = g.box(0, 0, 0, b.hw, b.hd, y0, g.sink + top, 0, MT_CONCRETE); e.pattern = 2; e.broken = brk; }
        if (share >= 0.97 && g.lod < 2) {   // its tops: the people's
            const int tp = g.gv(GV_TOPS);
            const double rr = std::min(b.hw, b.hd) * (form == 5 ? 0.66 : 1.0);
            const double yT = form == 1 || form == 5 ? g.sink + H : g.sink + top;   // B-415: on the shaft's top as it stands (a mast floated over a slab short of its height)
            if (tp == TP_MASTS || (tp == TP_NOTHING && g.U(12) < 0.3)) { Form& m = g.add(FK_TRUSS, 0, 0, 0, 0, 1.2, 1.2, yT, yT + H * 0.15, g.ironSlot(), MT_IRON); m.sides = 3; m.p[0] = 0.3; m.broken = g.D(MT_IRON) < 0.3 ? 0 : 0.6; }
            else if (tp == TP_FINIALS || g.gv(GV_FOCUS) == FO_SPIRE) { Form& s = g.lathe(2, 0, 0, rr * 0.5, yT, yT + H * (g.gv(GV_FOCUS) == FO_SPIRE ? 0.3 : 0.12), g.topSlot(MT_CONCRETE), MT_CONCRETE, form == 3 ? sides : 0); s.p[0] = 1.4; }
            else if (tp == TP_CRENELS && g.lod == 0) { const int n = 12; for (int k = 0; k < n; k += 2) { double a = k * TAU / n; g.box(std::sin(a) * rr * 0.9, std::cos(a) * rr * 0.9, a, rr * PI / n, 0.5, yT, yT + 4.0, g.topSlot(MT_CONCRETE), MT_CONCRETE); } }
            else if (tp == TP_STATUES) figure(g, 0, 0, yT, std::min(40.0, H * 0.1), g.topSlot(MT_CONCRETE), MT_CONCRETE, Dc);
        }
        if (share < 0.97 && g.lod == 0) {   // the top where it fell: across the ground away from the town's middle, and the heap at the foot on that side (the lobby clear)
            const double dir = std::atan2(b.x, b.z) - b.heading + (g.U(13) - 0.5) * 1.2, len = std::min((H - top) * 0.5, 120.0);
            const double rr = std::min(b.hw, b.hd) * 0.85, out = std::max(b.hw, b.hd) * 1.9, fx = std::sin(dir) * (std::max(b.hw, b.hd) + 1), fz = std::cos(dir) * (std::max(b.hw, b.hd) + 1);
            if (form == 2 || form == 3 || form == 5) g.fallen(FK_LATHE, 0, fx, fz, dir, rr, len, 0, MT_CONCRETE, form == 3 ? sides : 0, 2);
            else g.fallen(FK_BOX, 0, fx, fz, dir, rr, len, 0, MT_CONCRETE, 0, 2);
            g.heap(std::sin(dir) * out, std::cos(dir) * out, b.hw * 0.9, b.hd * 0.9, std::min(9.0, (H - top) * 0.04 + 1), 0, MT_CONCRETE);
        }
    } else if (g.lod == 0) { const double dir = std::atan2(b.x, b.z) - b.heading, out = std::max(b.hw, b.hd) * 1.9; g.heap(std::sin(dir) * out, std::cos(dir) * out, b.hw * 0.9, b.hd * 0.9, std::min(10.0, H * 0.05 + 1), 0, MT_CONCRETE); }
}

// the bowl of tiers round its field, its gate a gap (A-04: of its age's material: an amphitheatre of stone or brick, a stadium of
// concrete with its light masts, guyed where the people built in tension)
void genStadium(Gen& g) {
    const Building& b = g.b;
    const int mat = b.age >= AGE_MODERN ? MT_CONCRETE : greatOf(b, g.c);
    const double R = b.hw, D = g.D(mat);
    const double foot = std::max(9.0, R * 0.22);
    { Form& e = g.add(FK_SWEEP, 0, 0, 0, 0.18, R, R, 0, b.hWall, 0, mat); e.p[0] = TAU - 0.36; e.p[1] = foot; e.p[2] = 2.0; e.p[3] = 8; e.broken = D < 0.3 ? 0 : clampd(D, 0.3, 0.95); e.pattern = b.age >= AGE_MODERN ? 2 : 3; }
    if (b.age < AGE_MODERN || g.lod == 2) return;
    const bool tension = g.c.technique == TQ_TENSION;
    for (int k = 0; k < 4; k++) {
        double a = (k + 0.5) * PI / 2, rr = R + foot + 3, d = g.Dk(MT_IRON, 20 + k);
        if (d > 0.8) continue;
        const double mh = b.hWall * (tension ? 2.6 : 1.9);
        Form& m = g.add(FK_TRUSS, 0, std::sin(a) * rr, std::cos(a) * rr, 0, 1.4, 1.4, 0, mh, g.ironSlot(), MT_IRON); m.sides = 3; m.p[0] = 0.4; m.broken = d < 0.3 ? 0 : clampd(d, 0.3, 0.95);
        if (tension && d < 0.45) { double fx, fz; g.to(std::sin(a) * (R - foot * 0.3), std::cos(a) * (R - foot * 0.3), fx, fz); Form& cb = g.add(FK_CABLE, 0, std::sin(a) * rr, std::cos(a) * rr, 0, 0.06, 0.06, 0, mh, g.ironSlot(), MT_IRON); cb.p[0] = fx; cb.p[1] = fz; cb.p[2] = b.hWall; cb.p[3] = mh * 0.03; }
    }
}

void genMast(Gen& g) {   // a lattice mast held by its guys: they snap first, then the mast bends over and falls
    const Building& b = g.b;
    const double H = b.hWall, Di = g.D(MT_IRON);
    if (Di > 0.85) { if (g.lod == 0) g.fallen(FK_BOX, 0, 0, 0, g.U(10) * TAU, 1.0, H * 0.5, g.ironSlot(), MT_IRON); return; }
    Form& m = g.add(FK_TRUSS, 0, 0, 0, 0, 1.6, 1.6, 0, H, g.ironSlot(), MT_IRON); m.sides = 3; m.p[0] = 0.8; m.p[2] = 0.12; m.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.95);
    if (Di < 0.45 && g.lod < 2)
        for (int k = 0; k < 3; k++) {
            double a = g.U(11) * TAU + k * TAU / 3, d = H * 0.55, fx, fz; g.to(std::sin(a) * d, std::cos(a) * d, fx, fz);
            for (int q = 0; q < 2; q++) { Form& cb = g.add(FK_CABLE, 0, 0, 0, 0, 0.05, 0.05, 0, H * (q ? 0.9 : 0.5), g.ironSlot(), MT_IRON); cb.p[0] = fx; cb.p[1] = fz; cb.p[2] = 0; cb.p[3] = H * 0.01; }
            if (g.lod == 0) g.box(std::sin(a) * d, std::cos(a) * d, a, 1.2, 1.2, 0, 1.0, 0, MT_MASS);
        }
}

void genPower(Gen& g) {   // the cooling towers in a row (A-04: as many as the people's number allows), the stack, the turbine hall
    const Building& b = g.b;
    const double Dc = g.D(MT_CONCRETE);
    const int n = clampi(g.c.genome.number() - 2, 1, 5);
    const double R = b.hd * 0.42, H = R * 2.9;
    for (int k = 0; k < n; k++) {
        double x = -b.hw + R + (2 * b.hw - 2 * R) * (n > 1 ? (double)k / (n - 1) : 0.5), d = g.Dk(MT_CONCRETE, 10 + k);
        double h = H * tallShare(d, g.U(20 + k));
        Form& t = g.lathe(3, x, 0, R, 0, h < H * 0.97 ? h : H, 0, MT_CONCRETE); t.broken = h < H * 0.97 ? clampd(1 - h / H, 0.1, 0.9) : 0;
    }
    const double sH = H * 1.8, sh = sH * tallShare(Dc * 1.05, g.U(30));
    Form& s = g.lathe(5, b.hw * 0.2, b.hd + 8, 5.5, 0, sh, 0, MT_CONCRETE); s.p[0] = 0.55; s.broken = sh < sH * 0.97 ? 0.25 : 0;
    if (sh < sH * 0.97 && g.lod == 0) g.fallen(FK_LATHE, 0, b.hw * 0.2, b.hd + 8, PI / 2 + g.U(31) - 0.5, 4.5, (sH - sh) * 0.5, 0, MT_CONCRETE);
    Form& hall = g.box(-b.hw * 0.3, -b.hd - 14, 0, b.hw * 0.5, 12, 0, H * 0.3 * wallShare(Dc, g.U(32)), 0, MT_CONCRETE); hall.pattern = 1;
}

void genTanks(Gen& g) {   // a row of tanks or silos: iron rusting through, or concrete with their domed caps
    const Building& b = g.b;
    const int n = clampi(g.c.genome.number() - 1, 2, 6);
    const bool silos = g.U(2) < 0.5;
    const double r = std::min(b.hd, b.hw * 2 / n) * 0.8;
    for (int k = 0; k < n; k++) {
        double x = -b.hw + b.hw * 2 * (k + 0.5) / n;
        if (silos) {
            double d = g.Dk(MT_CONCRETE, 10 + k), h = b.hWall * tallShare(d, g.U(20 + k));
            Form& s = g.lathe(0, x, 0, r * 0.7, 0, h, 0, MT_CONCRETE); s.broken = h < b.hWall * 0.97 ? 0.3 : 0;
            if (h >= b.hWall * 0.97) g.add(FK_DOME, 1, x, 0, 0, r * 0.7, r * 0.7, h, h + r * 0.35, g.roofSlot(MT_CONCRETE), MT_CONCRETE, 2);
        } else {
            double d = g.Dk(MT_IRON, 10 + k);
            if (d > 0.9) continue;
            Form& t = g.lathe(4, x, 0, r, 0, b.hWall * 0.6, g.roofSlot(MT_IRON), MT_IRON); t.p[0] = 0.25; t.broken = d < 0.3 ? 0 : clampd(d, 0.3, 0.9);
        }
    }
}
}   // namespace

// ---- the futuristic age -------------------------------------------------------------------------------------------------------

namespace {
// a gate hall at the foot of a great structure: a parabolic vault open at both ends, its axis along the building's heading, set
// before the structure's front at `z0` (the records' room of an age that built without rooms of its own on the ground)
constexpr double PORTAL_HW = 7, PORTAL_HD = 13, PORTAL_H = 12;
void portal(Gen& g, double z0, int mat) {
    if (g.lod == 2) return;
    const double d = g.D(mat);
    Form& v = g.add(FK_VAULT, 2, 0, z0 + PORTAL_HD, 0, PORTAL_HW, PORTAL_HD, 0, PORTAL_H, g.accentOn(AP_DOORS) ? 1 : 0, mat); v.p[0] = 0.8; v.p[1] = 0;
    v.broken = d < 0.35 ? 0 : clampd(d, 0.35, 0.9);
}
// the front of a great structure of radius `r` and the portal before it
double portalZ(double r) { return r + 2; }
// the people's bands round a great turned form (an ornamented people's) at shares of its height
void bandsUp(Gen& g, double r, double H, double pw, int n) {
    if (g.lod == 2 || !g.banded()) return;
    for (int k = 1; k <= n; k++) { const double t = (double)k / (n + 1), rr = r * std::pow(std::max(0.0, 1 - t), pw) + 0.6; Form& e = g.add(FK_TORUS, 0, 0, 0, 0, rr, rr, H * t, H * t + std::max(1.5, H * 0.006), 1, MT_COMPOSITE, 4); e.p[0] = std::max(0.6, H * 0.004); }
}

// the great building of a futuristic town (`Building::style`, the people's technique): a stepped pyramid of the people's sides
// (printed), a cone (a shell), grown (a cluster of the people's number of tapering towers); its gate hall before it
void genArcology(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double Dc = g.D(MT_COMPOSITE), H = b.hWall, r = b.hw;
    const double share = tallShare(Dc, g.U(10));
    const int num = c.genome.number();
    switch (b.style % 3) {
        case 0: { Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, r, b.form == PF_ELLIPSE ? r * 0.75 : r, 0, H, 0, MT_COMPOSITE); e.sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(num, 3, 8) : (g.gv(GV_PLAN) == PF_CIRCLE ? 8 : 4); e.p[0] = 6 + num; e.p[1] = 0.18; e.p[2] = 0.7; e.pattern = 1;
                  e.broken = share < 0.97 ? clampd((1 - share) / 0.65, 0.05, 1) : 0; break; }
        case 1: { Form& e = g.add(FK_LATHE, 1, 0, 0, 0, r, b.form == PF_ELLIPSE ? r * 0.75 : r, 0, H, 0, MT_COMPOSITE); e.p[0] = 0.22; e.pattern = 1; e.sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(num, 3, 8) : 0; e.broken = share < 0.97 ? clampd((1 - share) / 0.85, 0.05, 1) : 0;
                  if (share >= 0.97) bandsUp(g, r, H, 1.0, 3); break; }   // snapped where it stands to, its flank's slope kept
        default: {   // grown: a cluster of tapering towers fused at their feet, the tallest in the middle
            const int n = clampi(num, 4, 8);
            for (int k = 0; k < n; k++) {
                double a = k * TAU / n + g.U(20 + k), d = k == 0 ? 0 : r * (0.45 + 0.25 * g.U(30 + k)), rr = r * (k == 0 ? 0.55 : 0.32 + 0.14 * g.U(40 + k));
                double h = H * (k == 0 ? 1.0 : 0.35 + 0.4 * g.U(50 + k)), sk = tallShare(g.Dk(MT_COMPOSITE, 60 + k), g.U(70 + k));
                Form& e = g.lathe(2, std::sin(a) * d, std::cos(a) * d, rr, 0, h, 0, MT_COMPOSITE); e.p[0] = 1.25; e.pattern = 2;
                e.broken = sk < 0.97 ? clampd((1 - sk) / 0.85, 0.05, 1) : 0;
            }
            break;
        }
    }
    if (share >= 0.97 && g.gv(GV_TOPS) == TP_MASTS && g.lod < 2 && b.style % 3 != 2) { Form& m = g.add(FK_TRUSS, 0, 0, 0, 0, 3.0, 3.0, H, H + H * 0.18, 1, MT_COMPOSITE); m.sides = 3; m.p[0] = 0.2; }
    if (g.lod == 0 && share < 0.97) g.heap(r * 0.6, r * 0.6, r * 0.35, r * 0.3, std::min(30.0, H * 0.03 + 2), 0, MT_COMPOSITE);
    portal(g, portalZ(r), MT_COMPOSITE);
}

void genHabitat(Gen& g) {   // a dome of diamond glass on its ring (A-04: the people's dome: a saucer, a hemisphere, pointed, an onion; a cone; a vault; on its oval), its gate hall at its edge
    const Building& b = g.b; const Culture& c = g.c;
    const double r = b.hw, rz = b.form == PF_ELLIPSE ? b.hw * 0.75 : b.hw, Dc = g.D(MT_COMPOSITE);
    const double tube = std::max(2.5, r * 0.06);
    { Form& ring = g.add(FK_TORUS, 0, 0, 0, 0, r, r, 0, 8, 0, MT_COMPOSITE); ring.p[0] = tube; }
    if (Dc < 0.8) {
        const double brk = Dc < 0.3 ? 0 : clampd(Dc, 0.3, 0.95), y0 = tube * 1.2;
        switch (c.roof) {
            case RF_CONICAL: case RF_PITCHED: { Form& e = g.add(FK_LATHE, 1, 0, 0, 0, r, rz, y0, y0 + r * 0.9, 2, MT_COMPOSITE); e.p[0] = 0.05; e.broken = brk; break; }
            case RF_VAULTED: { Form& e = g.add(FK_VAULT, 2, 0, 0, 0, rz, r, y0, y0 + rz * 0.8, 2, MT_COMPOSITE); e.p[0] = 0.5; e.p[1] = 1; e.broken = brk; break; }
            case RF_STEPPED: { Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, r, rz, y0, y0 + r * 0.55, 2, MT_COMPOSITE); e.sides = 8; e.p[0] = 4; e.p[1] = 0.2; e.p[2] = 0.6; e.broken = brk; break; }
            case RF_SADDLE: { Form& e = g.add(FK_SHELL, 0, 0, 0, 0, r, rz, y0, y0 + r * 0.5, 2, MT_COMPOSITE); e.broken = brk; break; }
            default: {
                const int prof = c.roof == RF_DOMED ? (g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES ? 3 : (g.gv(GV_OPENING) == OP_ARCHED ? 2 : 0)) : 1;
                Form& d = g.add(FK_DOME, prof, 0, 0, 0, r, rz, y0, y0 + r * (prof == 1 ? 0.55 : (prof == 0 ? 0.9 : 1.2)), 2, MT_COMPOSITE); d.broken = brk;
                break;
            }
        }
    }
    portal(g, portalZ(r), MT_COMPOSITE);
}

void genSpire(Gen& g) {   // a needle of a vertical town (A-04: its flank by the people's profile, its collars by its motif, a mast on its tip), the gate hall at its foot
    const Building& b = g.b;
    const double r = b.hw, H = b.hWall, Dc = g.D(MT_COMPOSITE), share = tallShare(Dc, g.U(10));
    const int prof = g.gv(GV_PROFILE);
    const double pw = (prof == WP_BATTERED ? 0.9 : (prof == WP_LEANING ? 2.4 : (prof == WP_BULGING ? 0.7 : 1.5))) + 0.6 * g.U(11), broken = share < 0.97 ? clampd((1 - share) / 0.85, 0.05, 1) : 0;
    const int sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(g.c.genome.number(), 3, 8) : 0;
    { Form& s = g.lathe(2, 0, 0, r, 0, H, 0, MT_COMPOSITE, sides); s.p[0] = pw; s.pattern = 2; s.broken = broken; }   // (a reference to a piece dies with the next piece added)
    if (g.lod < 2) {
        const int collars = g.gv(GV_MOTIF) == MO_SPIRALS ? 5 : (g.gv(GV_MOTIF) == MO_CIRCLES ? 3 : 2);
        for (int k = 1; k <= collars; k++) {
            double t = (0.66 / collars) * k + 0.1 * g.U(12 + k); if (t > 1 - 0.85 * broken) break;
            double rr = r * std::pow(1 - t, pw) + 4;
            Form& c = g.add(FK_TORUS, 0, 0, 0, 0, rr, rr, H * t, H * t + 4, 1, MT_COMPOSITE); c.p[0] = 2.2;
        }
    }
    if (share >= 0.97 && g.lod < 2 && g.gv(GV_TOPS) == TP_MASTS) { Form& m = g.add(FK_TRUSS, 0, 0, 0, 0, 1.6, 1.6, H, H + H * 0.12, 1, MT_COMPOSITE); m.sides = 3; m.p[0] = 0.25; }
    if (share < 0.97 && g.lod == 0) g.fallen(FK_LATHE, 1, 0, 0, std::atan2(b.x, b.z) - b.heading + (g.U(13) - 0.5), r * std::pow(1 - share, pw) * 0.9 + 1, std::min(H * (1 - share) * 0.4, 400.0), 0, MT_COMPOSITE);
    portal(g, portalZ(r), MT_COMPOSITE);
}

void genRing(Gen& g) {   // a ring standing on its rim on a plinth, facing along its heading
    const Building& b = g.b;
    const double Dc = g.D(MT_COMPOSITE);
    const double tube = std::max(3.0, b.hw * 0.06);
    const bool far = b.age == AGE_FAR_FUTURE;
    { Form& e = g.add(FK_TORUS, 0, 0, 0, 0, b.hw, b.hw, 0, 2 * b.hw, far ? 0 : 1, far ? MT_ETERNAL : MT_COMPOSITE); e.p[0] = tube; e.tilt = PI / 2;
      e.broken = far || Dc < 0.35 ? 0 : clampd(Dc, 0.35, 0.95); e.pattern = far && g.gv(GV_LIGHT) != LT_NONE ? 1 : 0; }
    if (!far && g.lod < 2) g.box(0, 0, 0, tube * 3, tube * 2.2, 0, tube * 0.8, 0, MT_COMPOSITE, 2);
}

void genMaglev(Gen& g) {   // pylons along the heading, the guideway's spans on them; a span falls where its own decay says, the pylons later
    const Building& b = g.b;
    const double H = b.hWall, step = 45;
    const int n = std::max(2, (int)std::lround(2 * b.hd / step));
    std::vector<double> top(n + 1, 0);
    for (int k = 0; k <= n; k++) {
        double z = -b.hd + 2 * b.hd * k / n, d = g.Dk(MT_COMPOSITE, 10 + k);
        if (d > 0.9) continue;
        double h = H * (d < 0.5 ? 1 : tallShare(d, g.U(30 + k)));
        Form& p = g.lathe(1, 0, z, 2.2, 0, h, 0, MT_COMPOSITE, 6); p.p[0] = 0.7; p.broken = h < H * 0.97 ? 0.3 : 0;
        top[k] = h;
    }
    for (int k = 0; k < n; k++) {
        double z0 = -b.hd + 2 * b.hd * k / n, z1 = -b.hd + 2 * b.hd * (k + 1) / n, d = g.Dk(MT_COMPOSITE, 60 + k);
        if (top[k] >= H * 0.99 && top[k + 1] >= H * 0.99 && d < 0.55) g.box(0, 0.5 * (z0 + z1), 0, 1.8, 0.5 * (z1 - z0), H, H + 2.4, 1, MT_COMPOSITE);
        else if (g.lod == 0 && d < 0.9) g.box(1.5 * (g.U(80 + k) - 0.5), 0.5 * (z0 + z1), 0.06, 1.8, 0.5 * (z1 - z0) * 0.9, 0, 2.4, 1, MT_COMPOSITE, 3);
    }
}

void genWreck(Gen& g) {   // a hull fallen from orbit: lying broken, half in the ground, its pieces strewn behind it along its furrow
    const Building& b = g.b;
    const double r = b.hw, L = b.hd * 2;
    Form& e = g.lathe(0, 0, -b.hd, r, -r * 0.45, -r * 0.45 + L, 2, MT_COMPOSITE, g.U(1) < 0.5 ? 0 : 6); e.tilt = PI / 2; e.broken = 0.5 + 0.4 * g.U(2);
    if (g.lod == 0) for (int k = 0; k < 4; k++) { double t = g.U(10 + k); Form& f = g.add(FK_BLOB, 0, (g.U(20 + k) - 0.5) * r * 3, -b.hd - 20 - t * L, g.U(30 + k) * TAU, r * 0.35, r * 0.3, 0, r * 0.5, 2, MT_COMPOSITE, 3); f.p[0] = 1; f.p[1] = 0.8; }
    const double De = materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet);
    Form& furrow = g.add(FK_SWEEP, 1, 0, -b.hd - L * 0.6, 0, 0, L * 0.6, 0, std::max(0.3, r * 0.25 * (1 - 0.7 * De)), 0, MT_EARTH, 8); furrow.p[1] = r * 1.3; furrow.p[2] = r * 0.4;
}

// ---- the far future: one structure, nothing weathering ---------------------------------------------------------------------

// the people's one structure (its focus: a spike, a ring, pillars, a shaft, a solid), its proportions, its number (a spike's facets, the
// pillars), its plan (the solid: a cube, a sphere half sunk, a prism of its number, an ellipsoid, a tetrahedron), its light (glowing
// lines up it, lamps)
void genFar(Gen& g) {
    const Building& b = g.b;
    const int num = g.c.genome.number(), light = g.gv(GV_LIGHT), pat = light != LT_NONE ? 1 : 0;
    switch (b.kind) {
        case BK_SPIKE: { Form& e = g.lathe(2, 0, 0, b.hw, 0, b.hWall, 0, MT_ETERNAL, g.U(1) < 0.4 || g.gv(GV_PLAN) == PF_POLYGON ? clampi(num, 3, 8) : 0); e.p[0] = 2.2 + 0.8 * g.U(2); e.pattern = pat; break; }
        case BK_HALO: genRing(g); break;
        case BK_PILLARS: {
            const int n = std::max(3, std::min(num, (int)std::lround(2 * b.hd / (b.hw * 2.5)))), sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(num, 3, 8) : (g.gv(GV_PLAN) == PF_RECT ? 4 : 0);
            for (int k = 0; k < n; k++) { Form& e = g.lathe(0, 0, -b.hd + 2 * b.hd * k / std::max(1, n - 1), b.hw, 0, b.hWall, 0, MT_ETERNAL, sides); e.pattern = pat; }
            break;
        }
        case BK_SHAFT: {   // a ring flat on the ground round a disc of black: the mouth of a shaft
            const double tube = std::max(3.0, b.hw * 0.07);
            { Form& e = g.add(FK_TORUS, 0, 0, 0, 0, b.hw, b.hw, 0, 2, 1, MT_ETERNAL); e.p[0] = tube; e.pattern = pat; }
            g.lathe(0, 0, 0, b.hw - tube * 0.5, 0, 0.25, 2, MT_ETERNAL);
            break;
        }
        default: {   // a solid: by the people's plan
            switch (g.gv(GV_PLAN)) {
                case PF_RECT: case PF_COURT: { Form& e = g.box(0, 0, 0, b.hw, b.hw, 0, b.hWall, 0, MT_ETERNAL); e.pattern = pat; break; }
                case PF_POLYGON: { Form& e = g.lathe(0, 0, 0, b.hw, 0, b.hWall, 0, MT_ETERNAL, clampi(num, 3, 8)); e.pattern = pat; break; }
                case PF_ELLIPSE: { Form& e = g.add(FK_LATHE, 4, 0, 0, 0, b.hw, b.hw * 0.6, -b.hw * 0.2, b.hw * 1.4, 0, MT_ETERNAL); e.p[0] = 1.0; e.pattern = pat; break; }
                case PF_FREE: { Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw, b.hw, 0, b.hWall, 0, MT_ETERNAL); e.sides = 3; e.p[1] = 0; e.pattern = pat; break; }
                default: { Form& e = g.lathe(4, 0, 0, b.hw, -b.hw * 0.2, b.hw * 1.8, 0, MT_ETERNAL); e.p[0] = 1.0; e.pattern = pat; break; }
            }
            break;
        }
    }
}

// ---- A-04: the genome's own works -----------------------------------------------------------------------------------------------

// the focal accent (`Building::style`, the people's `Focus`) in its age's way: a spire on its base, a great dome on its drum, a free
// arch (a trilithon before the medieval age), a great tower (a broch), a colossus on its pedestal (a carved figure stone), a mast (a
// pole, a lattice tower, a guyed mast), a standing ring (a holed stone, a wheel), an obelisk (a menhir), a pool in its rim, a great tree
void genFocus(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat), H = b.hWall;
    const bool pre = b.age == AGE_PREHISTORIC;
    g.sink = -c.buried * Dm;
    switch (b.style) {
        case FO_SPIRE: {
            if (pre) { standingStone(g, 0, 0, 0, b.hw * 0.35, b.hw * 0.25, H, 100, true); break; }
            const double bh = H * 0.28;
            TowerOut T = towerShaft(g, 0, 0, b.hw, bh, mat, 10, 2);
            if (T.whole) towerTop(g, 0, 0, T.rTop, T.h, bh, mat, true, H - bh);
            break;
        }
        case FO_DOME: {
            if (pre) { const double De = materialDecay(MT_EARTH, c.ageYears * 0.1, c.wet); g.add(FK_DOME, 1, 0, 0, 0, b.hw, b.hw, 0, std::max(0.6, H * (1 - 0.4 * De)), 0, MT_EARTH, 8); break; }   // a great mound
            Ring R; R.rx = b.hw; R.rz = b.hw; R.h = H * 0.4; R.thick = 1.2; R.mat = mat; R.segs = 16; R.doorSegs = ringDoorSegs(b.hw, 16); R.door = (16 - R.doorSegs / 2) % 16; R.pattern = 3;
            const double hmin = ringWalls(g, R);
            roundRoof(g, RF_DOMED, 0, 0, b.hw + 0.6, b.hw + 0.6, 16, hmin, R.h, 0.8, mat);
            if (hmin >= R.h * 0.92 && Dm < 0.3) topsAt(g, {{0, 0}}, g.sink + hmin + b.hw * (0.85 + 0.3 * g.gf(GV_PROPORTION)), 1.6, mat);
            break;
        }
        case FO_GATE: {
            if (pre) {   // a trilithon
                const double d0 = g.Dk(MT_DRYSTONE, 10), d1 = g.Dk(MT_DRYSTONE, 11);
                standingStone(g, -b.hw * 0.5, 0, 0, 0.9, 0.6, H, 10, false); standingStone(g, b.hw * 0.5, 0, 0, 0.9, 0.6, H, 11, false);
                if (d0 < 0.3 && d1 < 0.3) g.box(0, 0, 0, b.hw * 0.5 + 1.0, 0.6, H, H + 1.0, 0, MT_DRYSTONE, 4);
                break;
            }
            Form& e = g.add(FK_ARCH, archSub(g), 0, 0, 0, b.hw, std::max(1.2, b.hw * 0.18), g.sink, g.sink + H, g.accentOn(AP_DOORS) ? 1 : 0, mat);
            e.p[0] = 1; e.p[1] = 0.32; e.p[2] = H * 0.55; e.broken = Dm < 0.35 ? 0 : clampd(Dm, 0.35, 0.85);
            if (Dm < 0.3) topsAt(g, {{-b.hw * 0.75, 0}, {b.hw * 0.75, 0}}, g.sink + H, 1.5, mat);
            break;
        }
        case FO_TOWER: {
            TowerOut T = towerShaft(g, 0, 0, b.hw, H, pre ? MT_DRYSTONE : mat, 10, 2);
            if (T.whole && !pre && c.age >= AGE_MODERN && g.lod < 2) { Form& p = g.add(FK_LATHE, 4, 0, 0, 0, b.hw * 2.2, b.hw * 2.2, g.sink + H * 0.8, g.sink + H * 0.88, 0, mat); p.p[0] = 0.3; }   // B-415: a modern people's tower of the air, its pod
            if (T.whole && !pre) towerTop(g, 0, 0, T.rTop, T.h, H, mat, false, 0);
            break;
        }
        case FO_COLOSSUS: {
            if (pre) { standingStone(g, 0, 0, 0, b.hw * 0.4, b.hw * 0.3, H * 0.8, 100, true); if (g.Dk(MT_DRYSTONE, 100) < 0.3) g.add(FK_BLOB, 0, 0, 0, 0, b.hw * 0.3, b.hw * 0.3, H * 0.8, H * 0.8 + b.hw * 0.55, 0, MT_DRYSTONE); break; }
            const double ph = H * 0.18;
            g.box(0, 0, 0, b.hw, b.hw, g.sink, g.sink + ph, 0, mat, 5);
            figure(g, 0, 0, g.sink + ph, H - ph, g.accentOn(AP_TOPS) ? 1 : 0, mat, Dm);
            break;
        }
        case FO_MAST: {
            if (b.age <= AGE_MEDIEVAL) {   // a great pole with its cross arms (a timber's: a stump, its hole)
                g.post(0, 0, H, std::max(0.35, b.hw * 0.12), 10);
                if (g.D(MT_TIMBER) < 0.3 && g.lod < 2) for (int k = 0; k < 2; k++) g.box(0, 0, k * PI / 2, b.hw * 0.5, 0.15, H * 0.82, H * 0.82 + 0.4, g.timberSlot(), MT_TIMBER, 4);
                break;
            }
            const double Di = g.D(b.age >= AGE_FUTURISTIC ? MT_COMPOSITE : MT_IRON);
            Form& m = g.add(FK_TRUSS, 0, 0, 0, 0, b.hw, b.hw, 0, H, b.age >= AGE_FUTURISTIC ? 1 : g.ironSlot(), b.age >= AGE_FUTURISTIC ? MT_COMPOSITE : MT_IRON); m.sides = 4; m.p[0] = b.age == AGE_INDUSTRIAL ? 0.12 : 0.5; m.p[2] = 0.2; m.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.95);
            if (b.age >= AGE_MODERN && Di < 0.45 && g.lod < 2)
                for (int k = 0; k < 3; k++) { double a = k * TAU / 3, fx, fz; g.to(std::sin(a) * H * 0.5, std::cos(a) * H * 0.5, fx, fz); Form& cb = g.add(FK_CABLE, 0, 0, 0, 0, 0.06, 0.06, 0, H * 0.8, g.ironSlot(), MT_IRON); cb.p[0] = fx; cb.p[1] = fz; cb.p[2] = 0; cb.p[3] = H * 0.01; }
            break;
        }
        case FO_RING: {
            const double tube = std::max(0.6, b.hw * (pre ? 0.25 : 0.1));
            Form& e = g.add(FK_TORUS, 0, 0, 0, 0, b.hw, b.hw, pre ? 0 : 1.0, (pre ? 0 : 1.0) + 2 * b.hw, b.age == AGE_INDUSTRIAL ? g.ironSlot() : (g.accentOn(AP_TOPS) ? 1 : 0), b.age == AGE_INDUSTRIAL ? MT_IRON : (pre ? MT_DRYSTONE : mat));
            e.p[0] = tube; e.tilt = PI / 2; e.broken = Dm < 0.35 ? 0 : clampd(Dm, 0.35, 0.9);
            if (!pre) g.box(0, 0, 0, tube * 2.5, tube * 2.0, 0, 1.2, 0, mat, 2);
            break;
        }
        case FO_OBELISK: {
            if (pre) { standingStone(g, 0, 0, 0, b.hw * 0.4, b.hw * 0.3, H, 100, true); break; }
            const double h = H * (Dm < 0.4 ? 1 : 1 - 0.5 * Dm * g.U(10));
            Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw * 0.5, b.hw * 0.5, g.sink, g.sink + h, 0, mat, 5); e.p[1] = 0.62; e.sides = g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 8) : 4;
            if (h >= H * 0.97) { Form& t = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw * 0.31, b.hw * 0.31, g.sink + h, g.sink + h + b.hw * 0.6, g.topSlot(mat), mat, 5); t.p[1] = 0; t.sides = e.sides; }
            g.box(0, 0, 0, b.hw * 0.9, b.hw * 0.9, g.sink, g.sink + 0.8, 0, mat, 4);
            break;
        }
        case FO_POOL: {   // a basin in its rim (no higher than a step), the water in it on a wet world, its floor dry on a desert
            Ring R; R.rx = b.hw; R.rz = b.hd; R.h = 0.8; R.thick = 0.8; R.mat = pre ? MT_DRYSTONE : mat; R.segs = g.gv(GV_PLAN) == PF_POLYGON ? clampi(c.genome.number(), 3, 9) : 20; R.open = true; R.dress = false; R.whole = true;
            ringWalls(g, R);
            if (c.wet > 0.35 && !g.st.desert) { Form& w = g.add(FK_LATHE, 0, 0, 0, 0, b.hw - 0.4, b.hd - 0.4, -0.4, 0.45, 0, -1, 9); w.sides = R.segs <= 8 ? R.segs : 0; }
            if (g.lod == 0 && b.age >= AGE_MEDIEVAL) { Form& f = g.lathe(0, 0, 0, 0.8, g.sink, g.sink + 2.4, g.accentOn(AP_TOPS) ? 1 : 0, mat, 8); (void)f; topsAt(g, {{0, 0}}, g.sink + 2.4, 0.8, mat); }
            break;
        }
        default: {   // a great tree: its trunk of bark, its crown of the ground's leaves while the world grows them (a dead one's trunk rotting)
            const bool alive = !g.st.desert && c.wet > 0.3;
            const double Dt = alive ? 0 : g.D(MT_TIMBER), r = b.hw * 0.16;
            if (Dt > 0.85) break;
            const double th = H * (Dt < 0.3 ? 0.55 : 0.55 * (1 - Dt));
            { Form& t = g.add(FK_LATHE, 1, 0, 0, 0, r, r, 0, th, 0, -1, 10); t.p[0] = 0.55; }
            if (!alive) break;
            for (int k = 0; k < 5; k++) {
                const double a = k * TAU / 5 + g.U(20 + k), d = k == 0 ? 0 : b.hw * 0.45, rr = b.hw * (k == 0 ? 0.55 : 0.4);
                Form& cr = g.add(FK_BLOB, 0, std::sin(a) * d, std::cos(a) * d, a, rr, rr * 0.9, th * 0.8, H * (k == 0 ? 1.0 : 0.85), 0, -1, 11); cr.p[0] = 2; cr.p[1] = 0.5;
            }
            break;
        }
    }
    if (b.style == FO_SPIRE || b.style == FO_TOWER || b.style == FO_OBELISK || b.style == FO_COLOSSUS) motifOn(g, 0, 0, b.hw * 0.6, b.hw * 0.6, 0, 1, mat);   // on the ground round it (statues, a star)
}

// A-04: an aqueduct (a people of the valleys): its arcade along its heading, two tiers where it runs high, its channel on top; the
// prehistoric age's a leat (an earth channel)
void genAqueduct(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    if (b.age == AGE_PREHISTORIC) { const double De = materialDecay(MT_EARTH, c.ageYears * 0.1, c.wet); Form& e = g.add(FK_SWEEP, 1, 0, 0, 0, 0, b.hd, 0, std::max(0.3, 0.8 * (1 - 0.6 * De)), 0, MT_EARTH, 8); e.p[1] = 1.6; e.p[2] = 0.6; return; }
    const int mat = greatOf(b, c); const double Dm = g.D(mat), H = b.hWall;
    const int sub = archSub(g);
    const int tiers = H > 28 ? 3 : (H > 14 ? 2 : 1), n = std::max(3, (int)std::lround(2 * b.hd / 7.0));
    const bool abs = b.base > -1e8;   // T-03: a work's arcade from its line's low ground to its channel, over the reference level
    const double y00 = abs ? b.base : g.sink;
    for (int t = 0; t < tiers; t++) {
        const double y0 = H * t / tiers, y1 = H * (t + 1) / tiers;
        if (t > 0 && Dm > 0.5) break;   // the upper tier gone first
        Form& e = g.add(FK_ARCH, sub, 0, 0, PI / 2, b.hd, 1.2, y00 + y0, y00 + y1, 0, mat, t ? 4 : 0);
        e.p[0] = n * (t ? 2 : 1); e.p[1] = 0.34; e.p[2] = (y1 - y0) * 0.55; e.broken = Dm < 0.3 ? 0 : clampd(Dm, 0.3, 0.9); e.absolute = abs;
    }
}

// A-04: two towers joined by a bridge (a people of bridges): towers in the people's section (keeps, chimneys, tower blocks, spires by the
// age), the span between them where both stand whole; the records at their foot between them
void genTwin(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = b.age >= AGE_MODERN ? (b.age == AGE_MODERN ? MT_CONCRETE : MT_COMPOSITE) : greatOf(b, c);
    const double r = std::min(b.hd, b.hw * 0.3), H = b.hWall;
    bool both = true; double hb = H;
    for (int s = -1; s <= 1; s += 2) {
        TowerOut T = towerShaft(g, s * (b.hw - r), 0, r, H, mat, 10 + (s > 0) * 3, 2);
        if (!T.whole) both = false; else hb = std::min(hb, T.h);
        if (T.whole && b.age <= AGE_INDUSTRIAL) towerTop(g, s * (b.hw - r), 0, T.rTop, T.h, H, mat, false, 0);
    }
    const double D = g.D(mat);
    if (both && D < 0.4 && g.lod < 2) {
        const double y = g.sink + H * 0.68, span = b.hw - r;
        Form& e = g.box(0, 0, 0, span, std::min(r * 0.45, 4.0), y, y + std::max(2.6, H * 0.03), g.accentOn(AP_BANDS) ? 1 : 0, mat); e.pattern = 2;
        if (b.age == AGE_FUTURISTIC) { Form& f = g.add(FK_TORUS, 0, 0, 0, 0, span * 0.3, span * 0.3, y - span * 0.3, y + span * 0.3, 1, mat, 4); f.tilt = PI / 2; f.p[0] = 0.8; }
    }
    (void)hb;
}

// A-04: a tunnel's mouth (a people of tunnels): an arched portal half sunk, its passage going down dark into the ground
void genPortal(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat);
    const int sub = archSub(g);
    Form& e = g.add(FK_ARCH, sub, 0, 0, 0, b.hw, 0.8, -1.4, b.hWall - 1.4, g.doorSlot(), mat); e.p[0] = 1; e.p[1] = 0.4; e.p[2] = b.hWall * 0.45; e.broken = Dm < 0.4 ? 0 : clampd(Dm, 0.4, 0.8);
    g.box(0, -1.2, 0, b.hw * 0.82, 0.4, -1.4, b.hWall * 0.85 - 1.4, g.timberSlot(), mat, 4);   // the dark of the passage
    if (g.lod == 0) for (int s = -1; s <= 1; s += 2) g.box(s * (b.hw + 0.3), 2.5, 0, 0.35, 2.5, 0, 0.8, 0, mat, 4);   // the cutting's low walls
}

// ---- T-02: the harbour's and the water's works -------------------------------------------------------------------------------

// a crane on a quay, its jib or boom out along its heading over the water: an industrial people's jib crane (a lattice tower, its cab, the
// jib and its counterweight), a modern people's gantry (two portal frames astride the quay, the boom out over the berth and back over the
// yard); iron rusting: the jib or the boom down first, lying on the quay, then the tower bending over, then a heap
void genCrane(Gen& g) {
    const size_t first = g.out.size();
    struct Harbour { Gen& g; size_t first; ~Harbour() { for (size_t i = first; i < g.out.size(); i++) if (g.out[i].part == 0 || g.out[i].part == 3) g.out[i].part = 6; } } harbour{g, first};   // a harbour work: drawn from the water's level where its foot is wet
    const Building& b = g.b;
    const double Di = g.D(MT_IRON), H = b.hWall;
    const int slot = g.accentOn(AP_TOPS) ? 1 : g.ironSlot();
    if (Di > 0.9) { if (g.lod == 0) g.heap(0, 0, b.hw * 0.9, b.hd * 0.9, 1.4, g.ironSlot(), MT_IRON); return; }
    const double bent = Di < 0.4 ? 0 : clampd(Di, 0.4, 0.9);
    if (b.style == 0) {
        const double jib = H * (0.7 + 0.3 * g.U(5));
        Form& t = g.add(FK_TRUSS, 0, 0, 0, 0, std::min(b.hw, 1.6), std::min(b.hw, 1.6), 0, H, slot, MT_IRON); t.sides = 4; t.p[0] = 0.75; t.p[2] = 0.12; t.broken = bent;
        if (Di < 0.55) {
            g.box(0, 0, 0, 1.7, 2.0, H, H + 2.4, slot, MT_IRON);
            { Form& j = g.add(FK_TRUSS, 1, 0, jib * 0.5, 0.0, 0.55, jib * 0.5, H + 0.9, H + 2.0, slot, MT_IRON); j.p[2] = 0.07; j.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.7); }
            { Form& k = g.add(FK_TRUSS, 1, 0, -jib * 0.18, 0.0, 0.55, jib * 0.18, H + 0.9, H + 2.0, slot, MT_IRON); k.p[2] = 0.07; }
            g.box(0, -jib * 0.3, 0, 1.0, 1.0, H + 2.0, H + 3.2, 0, MT_MASS);   // the counterweight on its counter-jib
            if (g.lod < 2 && Di < 0.3) { Form& c = g.add(FK_CABLE, 0, 0, jib * 0.96, 0, 0.04, 0.04, 0, H + 0.9, slot, MT_IRON); double fx, fz; g.to(0, jib * 0.96, fx, fz); c.p[0] = fx; c.p[1] = fz; c.p[2] = H * 0.35; c.p[3] = 0.1; }
        } else if (g.lod == 0) g.fallen(FK_BOX, 0, 0, 0, 0.3 + 0.4 * g.U(6), 0.5, jib * 0.8, slot, MT_IRON);
        return;
    }
    // the gantry: its two portal frames b.hw apart across the heading, each two legs along the quay
    const double boomOut = 30 + 14 * g.U(5), boomBack = 14 + 6 * g.U(7), legs = 7.5;
    for (int s = -1; s <= 1; s += 2) {
        for (int q = -1; q <= 1; q += 2) { Form& l = g.box(s * b.hw, q * legs, 0, 0.7, 0.7, 0, H, slot, MT_IRON); l.broken = bent; }
        if (Di < 0.6) g.box(s * b.hw, 0, 0, 0.6, legs + 0.7, H - 1.6, H, slot, MT_IRON);
    }
    if (Di < 0.6) g.box(0, 0, 0, b.hw + 0.7, 1.0, H, H + 1.2, slot, MT_IRON);   // the cross girder over the two frames
    if (Di < 0.45) {
        Form& bm = g.add(FK_TRUSS, 1, 0, (boomOut - boomBack) * 0.5, 0, 1.4, (boomOut + boomBack) * 0.5, H + 1.2, H + 4.2, slot, MT_IRON); bm.p[2] = 0.18; bm.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.6);
        g.box(0, 0, 0, 2.6, 3.0, H + 4.2, H + 6.8, slot, MT_IRON);   // the machinery house over the portal
    } else if (g.lod == 0) g.fallen(FK_BOX, 0, 0, legs + 2, 0.2 * g.U(6) - 0.1, 1.2, boomOut * 0.7, slot, MT_IRON);
}

// a container yard's stacks: rows of boxes 12 m long, stacked one to four, each stack its colour (the accent, the structure, the iron);
// rusting through, the stacks sink to their lowest boxes, then to stains
void genContainers(Gen& g) {
    const Building& b = g.b;
    const double Di = g.D(MT_IRON);
    if (Di > 0.92) return;
    if (g.lod == 2) { g.box(0, 0, 0, b.hw, b.hd, 0, std::max(0.5, 5.2 * (1 - 0.8 * Di)), 3, MT_IRON); return; }
    const int cols = std::max(1, (int)(b.hw * 2 / 12.8)), rows = std::max(1, (int)(b.hd * 2 / 2.75));
    for (int i = 0; i < cols; i++)
        for (int j = 0; j < rows; j++) {
            const double u = g.U(100 + i * 64 + j);
            if (u < 0.08) continue;   // an empty slot
            const int stack = 1 + (int)(g.U(400 + i * 64 + j) * 4);
            const double lost = Di * (0.6 + 0.6 * g.U(700 + i * 64 + j));
            const double h = std::max(0.6, 2.6 * stack * (1 - std::min(0.85, lost)));
            const int slot = u < 0.4 ? 1 : (u < 0.7 ? 3 : 0);
            Form& e = g.box(-b.hw + 6.4 + 12.8 * i, -b.hd + 1.375 + 2.75 * j, 0, 6.0, 1.2, 0, h, slot, MT_IRON);
            e.broken = lost > 0.4 ? clampd(lost, 0.3, 0.8) : 0;
        }
}

// a town's reservoir: an industrial people's basin of masonry in its earth bank, a pump house at a corner; a modern people's round tank of
// concrete open to the sky; the walls falling with their material, the bank lasting
void genReservoir(Gen& g) {
    const Building& b = g.b;
    if (b.style == 0) {
        const double Dm = g.D(MT_MASONRY), De = materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet);
        const double h = b.hWall, t = 0.9;
        for (int k = 0; k < 4; k++) {
            const bool along = k % 2 == 0;
            const double len = along ? b.hw : b.hd, off = along ? b.hd : b.hw;
            const double share = wallShare(Dm, g.U(10 + k));
            if (share < 0.06) continue;
            Form& w = g.box(along ? 0 : (k == 1 ? off : -off), along ? (k == 0 ? off : -off) : 0, along ? 0 : PI / 2, len + t, t * 0.5, 0, h * share, 0, MT_MASONRY);
            w.broken = share < 0.9 ? clampd(1 - share, 0.1, 0.9) : 0;
            if (g.lod < 2) { Form& bk = g.box(along ? 0 : (k == 1 ? off + 2.2 : -off - 2.2), along ? (k == 0 ? off + 2.2 : -off - 2.2) : 0, along ? 0 : PI / 2, len + 3.5, 1.8, 0, std::max(0.4, h * 0.55 * (1 - 0.4 * De)), 0, MT_EARTH, 8); (void)bk; }
        }
        if (g.lod < 2) {   // the pump house
            const double share = wallShare(Dm, g.U(20));
            if (share > 0.1) { Form& p = g.box(b.hw + 5.5, b.hd + 5.5, 0, 3.0, 3.0, 0, 4.5 * share, 0, MT_MASONRY); p.pattern = share > 0.7 ? 1 : 0; }
        }
        return;
    }
    const double Dc = g.D(MT_CONCRETE);
    const double share = 1 - Dc * (0.2 + 0.6 * g.U(10));
    Form& t = g.lathe(0, 0, 0, b.hw, 0, std::max(0.6, b.hWall * share), 0, MT_CONCRETE); t.p[3] = 1; t.broken = Dc < 0.3 ? 0 : clampd(Dc, 0.3, 0.9);
    if (g.lod == 0 && Dc < 0.5) for (int k = 0; k < 4; k++) { const double a = g.U(30) * TAU + k * TAU / 4; g.box(std::sin(a) * (b.hw + 1.2), std::cos(a) * (b.hw + 1.2), a, 0.6, 1.2, 0, b.hWall * share * 0.8, 0, MT_CONCRETE); }   // its buttresses
}
}   // namespace

// ---- T-03: the works' kinds (galaxy/works.h) --------------------------------------------------------------------------------

namespace {
// the earth's slump: what the rain and the years left of a mound, a bank, a spoil heap (A-03's earth: a desert keeps them)
double earthLeft(const Gen& g) { return 1 - 0.55 * materialDecay(MT_EARTH, g.c.ageYears * 0.1, g.c.wet); }
int frameSlot(const Gen& g, int mat) { return mat == MT_TIMBER ? g.timberSlot() : (mat == MT_COMPOSITE ? 0 : g.ironSlot()); }

// earth: a round barrow (`style` 0), a long barrow (1), a conical spoil heap (2), a bank along its length (3: a rampart, a dyke), a ring bank
// round its middle (4: a henge's, a redoubt's), a flat heap of tailings (5), spoil tipped down a slope below an adit (6); grassed over or bare
// by the world
void genMound(Gen& g) {
    const Building& b = g.b;
    const double k = earthLeft(g), h = std::max(0.4, b.hWall * k);
    switch (b.style) {
        case 1: { Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw, b.hd, 0, h, 0, MT_EARTH, 8); e.sides = 8; e.p[1] = 0.45; e.broken = 0.6; break; }
        case 2: { Form& e = g.lathe(1, 0, 0, b.hw, 0, h, 0, MT_EARTH); e.part = 8; e.p[0] = 0.08; break; }   // a tip's cone
        case 6: { Form& e = g.add(FK_BLOB, 0, 0, 0, 0, b.hw, b.hd, 0, h, 0, MT_EARTH, 8); e.p[0] = 2; e.p[1] = 0.55; break; }   // spoil tipped down a slope: lumps
        case 3: { Form& e = g.add(FK_SWEEP, 1, 0, 0, 0, 0, b.hd, 0, h, 0, MT_EARTH, 8); e.p[1] = b.hw; e.p[2] = std::max(0.4, b.hw * 0.3); break; }
        case 4: { Form& e = g.add(FK_SWEEP, 0, 0, 0, 0, b.hw, b.hw, 0, h, 0, MT_EARTH, 8); e.p[0] = TAU; e.p[1] = b.hd; e.p[2] = std::max(0.4, b.hd * 0.3); break; }
        case 5: { Form& e = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw, b.hd, 0, h, 0, MT_EARTH, 8); e.sides = 7; e.p[1] = 0.8; e.broken = 0.5; break; }
        default: { Form& e = g.add(FK_DOME, 1, 0, 0, 0, b.hw, b.hw, 0, h, 0, MT_EARTH, 8); (void)e; break; }
    }
}

// a dam's wall across its valley, its forms from `base` (under the floor) to its crest at base + hWall, over the reference level (they stand
// where the valley is whatever the ground under each does): the heading looks up the reservoir, the crest runs across it (hw its half
// length), its foot hd thick. `style` 0 a masonry gravity dam (its buttresses down the face, a parapet); 1 a concrete gravity dam (its
// spillway down one side, its intake tower in the water); 2 a concrete arch bowed into the reservoir; 3 the futuristic age's wall of
// composite, a line of light along its crest. Breached when its material is half gone: a gap down its middle (the reservoir drains)
void genDam(Gen& g) {
    const Building& b = g.b;
    const int mat = b.style == 0 ? MT_MASONRY : (b.style == 3 ? MT_COMPOSITE : MT_MASS);
    const double D = g.D(mat), H = b.hWall, y0 = b.base, y1 = b.base + H, L = b.hw;
    const double top = b.style == 2 ? 2.2 : (b.style == 0 ? 2.6 : 4.5), foot = std::max(top + 1, b.hd);
    const bool breach = D > 0.5;
    const double gapHalf = breach ? L * (0.08 + 0.3 * (D - 0.5)) : 0;
    auto mark = [&](Form& e) { e.absolute = true; };
    if (b.style == 2) {   // the arch: an arc of radius Ra round a centre downstream, its chord the crest's length
        const double Ra = L * 1.45, half = std::asin(clampd(L / Ra, 0, 1)), cz = -Ra * std::cos(half);
        const double gapA = breach ? std::asin(clampd(gapHalf / Ra, 0, 1)) : 0;
        for (int side = breach ? 0 : -1; side <= (breach ? 1 : -1); side++) {
            double a0 = -half, a1 = half;
            if (breach) { if (side == 0) a1 = -gapA; else a0 = gapA; }
            Form& e = g.add(FK_SWEEP, 0, 0, cz, a0, Ra, Ra, y0, y1, 0, mat); e.p[0] = a1 - a0; e.p[1] = foot * 0.6; e.p[2] = top; e.broken = breach ? 0.15 : 0; mark(e);
        }
    } else {
        for (int side = breach ? 0 : -1; side <= (breach ? 1 : -1); side++) {
            double xa = -L, xb = L;
            if (breach) { if (side == 0) xb = -gapHalf; else xa = gapHalf; }
            Form& e = g.add(FK_SWEEP, 1, 0.5 * (xa + xb), 0, PI / 2, 0, 0.5 * (xb - xa), y0, y1, 0, mat); e.p[1] = foot; e.p[2] = top; e.broken = breach ? 0.2 : 0; mark(e);
        }
        if (g.lod < 2 && b.style == 0) {   // buttresses down the downstream face
            const int n = std::max(3, (int)(2 * L / 28));
            for (int k = 1; k < n; k++) {
                const double x = -L + 2 * L * k / n;
                if (std::fabs(x) < gapHalf + 4) continue;
                Form& e = g.box(x, -foot * 0.55, 0, 1.6, foot * 0.45, y0, y1 - 1.5, 0, mat); e.absolute = true;
            }
        }
    }
    if (g.lod == 2 || D > 0.85) return;
    // the parapet along the crest, a line of light on a futuristic wall, the intake tower, a modern spillway's chute
    if (b.style != 2 && g.lod == 0) {
        for (int s = -1; s <= 1; s += 2) {
            const double half = L - gapHalf;
            if (half < 4) continue;
            if (breach) { for (int q = -1; q <= 1; q += 2) { Form& p = g.box(q * (gapHalf + half * 0.5), s * (top - 0.3), 0, half * 0.5, 0.3, y1, y1 + 1.1, b.style == 3 ? 3 : 0, mat, 4); p.absolute = true; } }
            else { Form& p = g.box(0, s * (top - 0.3), 0, L, 0.3, y1, y1 + 1.1, b.style == 3 ? 3 : 0, mat, 4); p.absolute = true; }
        }
    }
    if (b.style == 1 || b.style == 3) {
        { Form& t = g.lathe(0, L * 0.3, foot + 22, 5.5, y0, y1 + 6, 0, mat); t.absolute = true; t.broken = D > 0.4 ? 0.4 : 0; }
        if (D < 0.4) { Form& c = g.add(FK_DOME, 1, L * 0.3, foot + 22, 0, 6.2, 6.2, y1 + 6, y1 + 8.5, g.roofSlot(mat), mat, 2); c.absolute = true; }
    }
    if (b.style == 1 && g.lod < 2) {   // the spillway: a stepped chute down the face at one end
        const int n = std::max(3, (int)(H / 8));
        for (int k = 0; k < n; k++) {
            const double t = (double)k / n;
            Form& e = g.box(L * 0.82, -top - (foot - top) * t - 3, 0, 9, 4, y0, y1 - H * t - 2, 0, mat); e.absolute = true;
        }
    }
}

// a radio dish on its pedestal: the dish of radius hw aimed up by the style's seed, rusting through (holed), then down on the ground beside
// its pedestal
void genDish(Gen& g) {
    const Building& b = g.b;
    const int mat = b.age >= AGE_FUTURISTIC ? MT_COMPOSITE : MT_IRON;
    const double D = g.D(mat), r = b.hw, mast = std::max(4.0, r * 0.75);
    { Form& p = g.lathe(1, 0, 0, std::max(1.4, r * 0.16), 0, mast * 0.45, 0, MT_CONCRETE, 6); p.p[0] = 0.6; }
    if (D > 0.78) {   // fallen: the dish lies on its rim beside its mount
        if (g.lod == 0) { Form& e = g.add(FK_SHELL, 1, r * 0.7, 0, g.U(4) * TAU, r, r, 0, 0.1, frameSlot(g, mat), mat); e.p[0] = r * 0.3; e.p[1] = 0; e.tilt = 1.3; e.broken = 0.7; }
        return;
    }
    Form& e = g.add(FK_SHELL, 1, 0, 0, 0, r, r, 0, mast, b.style == 1 ? 0 : frameSlot(g, mat), mat);
    e.p[0] = r * 0.32; e.p[1] = mast; e.tilt = 0.25 + 0.8 * g.U(5) + D * 0.4; e.broken = D < 0.35 ? 0 : clampd(D, 0.35, 0.8);
}

// a tomb: a gabled house of the dead (`style` 0), a domed one (1), a façade cut into the rock (2: its door the dark of the chamber), a
// columbarium's long block of niches (3), a tower of silence (4: a high blind ring open to the sky, its one gap)
void genTomb(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat);
    if (b.style == 4) {
        Ring R; R.rx = b.hw; R.rz = b.hw; R.h = b.hWall; R.thick = 1.2; R.mat = mat; R.segs = 16; R.doorSegs = 1; R.door = (int)(g.U(3) * 16); R.pattern = 0; R.dress = false;
        ringWalls(g, R);
        return;
    }
    if (b.style == 2) {
        Form& e = g.add(FK_ARCH, 0, 0, 0, 0, b.hw, 0.9, -1.0, b.hWall, g.doorSlot(), mat); e.p[0] = 1; e.p[1] = 0.32; e.p[2] = b.hWall * 0.4; e.broken = Dm < 0.4 ? 0 : clampd(Dm, 0.4, 0.8);
        g.box(0, -1.0, 0, b.hw * 0.62, 0.3, -1.0, b.hWall * 0.62, g.timberSlot(), mat, 4);
        if (Dm < 0.5 && g.lod == 0) { Form& p = g.add(FK_PYRAMID, 0, 0, 0.2, 0, b.hw * 1.05, 0.7, b.hWall, b.hWall + b.hw * 0.4, 0, mat, 4); p.sides = 3; }
        return;
    }
    if (b.style == 3) {
        const double share = wallShare(Dm, g.U(7));
        Form& e = g.box(0, 0, 0, b.hw, b.hd, 0, std::max(0.6, b.hWall * share), 0, mat); e.pattern = 2; e.broken = share < 0.95 ? 0.3 : 0;
        return;
    }
    Walls W; W.hw = b.hw; W.hd = b.hd; W.h = b.hWall; W.thick = 0.45; W.door = 0; W.doorPos = 0; W.mat = mat; W.k0 = 20; W.dress = true;
    double hs[4]; const double hmin = walls4(g, W, hs);
    roof(g, b.style == 1 ? RF_DOMED : RF_PITCHED, 0, 0, b.hw, b.hd, hmin, b.hWall, 0.5, mat);
    if (b.style != 1) gables(g, W, hs, 0.5);   // R-412: its gables and its doorway the people's
    entranceAt(g, W, wallsHalf(g, W), W.h);
    cornersOf(g, W, hs, wallsHalf(g, W), mat);
}

// a redoubt (`style` 0: a ring bank of earth with its ditch outside and a casemate of masonry in it) or a casemate fort of concrete (1: a low
// polygon, its gun ports, a cupola)
void genFort(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    if (b.style == 0) {
        const double k = earthLeft(g);
        { Form& e = g.add(FK_SWEEP, 0, 0, 0, 0, b.hw, b.hw, 0, std::max(0.5, 3.2 * k), 0, MT_EARTH, 8); e.p[0] = TAU; e.p[1] = 4.5; e.p[2] = 1.4; }
        const int mat = greatOf(b, c); const double Dm = g.D(mat), share = wallShare(Dm, g.U(5));
        Form& h = g.box(0, 0, 0, b.hw * 0.3, b.hw * 0.22, 0, std::max(0.5, 3.0 * share), 0, mat); h.pattern = share > 0.8 ? 2 : 0;
        return;
    }
    const double D = g.D(MT_MASS);
    const int pl = g.gv(GV_PLAN);   // R-412: the people's section
    Form& e = g.lathe(1, 0, 0, b.hw, 0, b.hWall, 0, MT_MASS, pl == PF_POLYGON ? clampi(c.genome.number(), 5, 8) : (pl == PF_CIRCLE || pl == PF_ELLIPSE ? 0 : 6)); e.p[0] = 0.86; e.pattern = 2; e.broken = D < 0.4 ? 0 : clampd(D, 0.4, 0.7);
    if (D < 0.6) { Form& d = g.add(FK_DOME, 1, b.hw * 0.25, 0, 0, b.hw * 0.25, b.hw * 0.25, b.hWall, b.hWall + b.hw * 0.12, frameSlot(g, MT_IRON), MT_IRON, 2); (void)d; }
}

// a pillbox (`style` 0: round or of six sides, its slits, its slab), a blockhouse (1), a shelter's vents (2: short shafts with their caps)
void genBunker(Gen& g) {
    const Building& b = g.b;
    const int mat = b.age >= AGE_FUTURISTIC ? MT_COMPOSITE : MT_MASS;
    const double D = g.D(mat);
    if (b.style == 2) {
        const int n = 2 + (int)(g.U(3) * 3);
        for (int k = 0; k < n; k++) {
            const double x = (k - (n - 1) * 0.5) * 3.2;
            g.lathe(0, x, 0, 0.7, 0, 1.6, 0, mat);
            if (D < 0.6) g.box(x, 0, 0, 1.0, 1.0, 1.6, 1.9, 0, mat, 4);
        }
        return;
    }
    if (b.style == 1) {
        Form& e = g.box(0, 0, 0, b.hw, b.hd, 0, b.hWall, 0, mat); e.pattern = 2; e.broken = D < 0.45 ? 0 : 0.3;
        return;
    }
    const int pl = g.gv(GV_PLAN);   // R-412: the people's section (round, its polygon, square)
    Form& e = g.lathe(0, 0, 0, b.hw, 0, b.hWall, 0, mat, pl == PF_POLYGON ? clampi(g.c.genome.number(), 5, 8) : (pl == PF_RECT || pl == PF_COURT ? 4 : (pl == PF_FREE ? 6 : 0))); e.pattern = 2;
    if (g.lod < 2) g.lathe(0, 0, 0, b.hw + g.slabEave() + 0.3, b.hWall, b.hWall + 0.45, 0, mat, e.sides);
}

// dragon's teeth: rows of concrete pyramids along the line, staggered, some tipped
void genTeeth(Gen& g) {
    const Building& b = g.b;
    const int rows = 2 + (int)(g.U(2) * 3), n = std::max(2, (int)(2 * b.hd / 2.2));
    if (g.lod == 2) { g.box(0, 0, 0, rows * 1.1, b.hd, 0, 1.0, 0, MT_MASS); return; }
    for (int r = 0; r < rows; r++)
        for (int k = 0; k < n; k++) {
            const double z = -b.hd + (k + 0.5 + (r % 2) * 0.5) * 2 * b.hd / n, x = (r - (rows - 1) * 0.5) * 2.2;
            if (z > b.hd) continue;
            const double d = g.Dk(MT_MASS, 100 + r * 64 + k);
            if (d > 0.9) continue;
            Form& e = g.add(FK_PYRAMID, 0, x, z, g.U(300 + k) * 0.6, 0.65, 0.65, 0, 1.0 + 0.6 * g.U(200 + r), 0, MT_MASS); e.p[1] = 0.12;
            if (d > 0.4) e.tilt = 0.2 + 0.6 * (d - 0.4);
        }
}

// a machine's wreck: a tank (`style` 0: its hull, tracks, turret and gun; the turret blown off beside it), a haul truck (1: its bed, its cab,
// its wheels), a truck (2); rusting to a shell, then a stain of iron
void genVehicle(Gen& g) {
    const Building& b = g.b;
    const double D = g.D(MT_IRON);
    if (D > 0.88) { if (g.lod == 0) g.heap(0, 0, b.hw * 0.8, b.hd * 0.8, 0.8, g.ironSlot(), MT_IRON); return; }
    const int sl = g.ironSlot();
    if (b.style == 0) {
        g.box(0, 0, 0, b.hw * 0.72, b.hd, 0.5, 1.6, sl, MT_IRON);
        for (int s = -1; s <= 1; s += 2) g.box(s * b.hw * 0.86, 0, 0, b.hw * 0.16, b.hd * 0.96, 0, 1.1, sl, MT_IRON);
        if (g.lod == 2) return;
        const bool off = g.U(5) < 0.5 || D > 0.5;
        const double tx = off ? b.hw * 2.2 : 0, tz = off ? b.hd * 0.3 : -b.hd * 0.1, ty = off ? 0 : 1.6;
        Form& t = g.lathe(1, tx, tz, b.hw * 0.55, ty, ty + 0.9, sl, MT_IRON, 6); t.p[0] = 0.75; t.tilt = off ? 0.5 : 0;
        if (!off && g.lod == 0) { Form& gun = g.lathe(0, 0, tz + 0.3, 0.12, ty + 0.45, ty + 0.45 + b.hd * 1.1, sl, MT_IRON); gun.tilt = PI / 2 - 0.08; }
        return;
    }
    const double bedH = b.style == 1 ? b.hWall * 0.55 : 1.4;
    g.box(0, -b.hd * 0.15, 0, b.hw, b.hd * 0.82, bedH * 0.45, bedH, sl, MT_IRON);
    g.box(0, b.hd * 0.78, 0, b.hw * 0.7, b.hd * 0.2, bedH * 0.45, bedH + (b.style == 1 ? 1.6 : 1.0), sl, MT_IRON);
    if (g.lod == 0 && D < 0.6)
        for (int s = -1; s <= 1; s += 2) for (int q = -1; q <= 1; q += 2) {
            Form& w = g.add(FK_TORUS, 0, s * b.hw * 0.95, q * b.hd * 0.6, PI / 2, bedH * 0.42, bedH * 0.42, 0, 0.1, sl, MT_IRON); w.p[0] = bedH * 0.14; w.tilt = PI / 2;
        }
}

// an aircraft's wreck: its fuselage broken behind the wings, a wing torn off, the tail fin; in the people's age's materials
void genAircraft(Gen& g) {
    const Building& b = g.b;
    const int mat = b.age >= AGE_FUTURISTIC ? MT_COMPOSITE : MT_IRON;
    const double D = g.D(mat), r = b.hw * 0.14, L = b.hd * 2;
    if (D > 0.9) { if (g.lod == 0) g.heap(0, 0, b.hw * 0.4, b.hd * 0.5, 0.7, g.ironSlot(), mat); return; }
    const int sl = mat == MT_COMPOSITE ? 2 : g.ironSlot();
    { Form& f = g.lathe(0, 0, -b.hd, r, 0, L * 0.6, sl, mat); f.tilt = PI / 2; f.broken = 0.3; }
    { Form& f = g.lathe(1, 0.6, -b.hd + L * 0.64, r, 0, L * 0.36, sl, mat); f.tilt = PI / 2; f.heading += 0.25; f.p[0] = 0.45; f.broken = 0.4; }
    if (g.lod == 2) return;
    g.box(b.hw * 0.5, -b.hd + L * 0.4, 0.05, b.hw * 0.5, b.hd * 0.08, 0.2, 0.55, sl, mat);
    if (g.U(3) < 0.6) { Form& w = g.box(-b.hw * 0.9, -b.hd + L * 0.3, 0.6, b.hw * 0.45, b.hd * 0.07, 0, 0.4, sl, mat); w.tilt = 0.15; }
    if (D < 0.6) g.box(0.6, -b.hd + L * 0.94, 0.25, 0.15, b.hd * 0.1, 0.4, 0.4 + r * 3.5, sl, mat);
}

// a ship's wreck on the shore: a wooden hull's ribs and keel (`style` 0), an iron steamer listing with its funnel (1), a freighter broken
// in two, its superstructure aft (2); the hull in the water drawn from the water's level (a harbour work's foot)
void genShip(Gen& g) {
    const size_t first = g.out.size();
    struct Wet { Gen& g; size_t first; ~Wet() { for (size_t i = first; i < g.out.size(); i++) if (g.out[i].part == 0 || g.out[i].part == 3) g.out[i].part = 6; } } wet{g, first};
    const Building& b = g.b;
    const double L = b.hd, B = b.hw, H = b.hWall;
    if (b.style == 0) {
        const double Dt = g.D(MT_TIMBER);
        if (Dt > 0.9) return;
        g.box(0, 0, 0, 0.35, L, 0, 0.6, g.timberSlot(), MT_TIMBER);
        if (g.lod == 2) { g.box(0, 0, 0, B * 0.8, L * 0.8, 0, H * 0.4, g.timberSlot(), MT_TIMBER); return; }
        const int n = std::max(5, (int)(2 * L / 2.2));
        for (int k = 0; k < n; k++) {
            const double t = -1 + 2.0 * (k + 0.5) / n, w = B * std::sqrt(std::max(0.05, 1 - t * t * 0.85));
            if (g.Dk(MT_TIMBER, 40 + k) > 0.7) continue;
            for (int s = -1; s <= 1; s += 2) { Form& rb = g.box(s * w * 0.55, t * L, 0, 0.12, 0.12, 0, H * (0.6 + 0.4 * g.U(60 + k)), g.timberSlot(), MT_TIMBER); rb.tilt = s * 0.35; rb.heading += PI / 2; }
        }
        return;
    }
    const double Di = g.D(MT_IRON);
    if (Di > 0.92) { if (g.lod == 0) g.heap(0, 0, B, L * 0.6, 1.2, g.ironSlot(), MT_IRON); return; }
    const int sl = g.ironSlot();
    const double list = 0.08 + 0.12 * g.U(4);
    auto hull = [&](double z0, double z1, bool bow, double tilt, double dh) {
        const double len = 0.5 * (z1 - z0), mid = 0.5 * (z0 + z1);
        Form& h = g.box(0, mid, dh + PI / 2, len, B, 0, H, sl, MT_IRON); h.broken = Di < 0.4 ? 0.1 : clampd(Di, 0.2, 0.7); h.tilt = tilt; h.pattern = 0;
        if (bow) { const double hd = h.heading - PI / 2; Form& p = g.lathe(1, 0, z1 + B * 0.2, B, 0, H, sl, MT_IRON, 3); p.p[0] = 0.9; p.heading = hd; }
    };
    if (b.style == 2 && Di > 0.25) {   // broken in two: the bow's half turned off the stern's
        hull(-L, -L * 0.15, false, list, 0);
        hull(L * 0.05, L, true, list * 1.6, 0.18);
    } else hull(-L, L * 0.85, true, list, 0);
    if (g.lod == 2 || Di > 0.6) return;
    if (b.style == 1) { Form& f = g.lathe(0, 0, -L * 0.1, B * 0.22, H, H + B * 1.4, sl, MT_IRON); f.tilt = 0.2; g.box(0, -L * 0.5, 0, B * 0.55, L * 0.12, H, H + 3, sl, MT_IRON); }
    else { Form& s = g.box(0, -L * 0.8, 0, B * 0.85, L * 0.1, H, H + B * 1.1, sl, MT_IRON); s.pattern = 2; if (g.lod == 0) { Form& m = g.add(FK_TRUSS, 0, 0, L * 0.4, 0, 0.6, 0.6, H, H + B * 1.4, sl, MT_IRON); m.sides = 4; m.p[0] = 0.6; m.p[2] = 0.08; } }
}

// a launch pad: its slab, the service tower beside it with its arm out over the pad, the lightning masts round it, its water tower; the iron
// rusting: the arm down first, the tower bending
void genLaunchpad(Gen& g) {
    const Building& b = g.b;
    const double Di = g.D(MT_IRON), H = b.hWall;
    g.box(0, 0, 0, b.hw * 0.42, b.hw * 0.42, 0, 7, 0, MT_MASS);
    if (g.lod < 2) g.box(0, b.hw * 0.42 + 9, 0, 6, 9, 0, 1.5, 0, MT_MASS);   // the flame deflector's wedge
    const int sl = g.ironSlot();
    if (Di < 0.9) { Form& t = g.add(FK_TRUSS, 0, -b.hw * 0.42 - 8, 0, 0, 5, 5, 0, H, sl, MT_IRON); t.sides = 4; t.p[0] = 0.9; t.p[2] = 0.3; t.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.95); }
    if (Di < 0.4 && g.lod < 2) { Form& a = g.add(FK_TRUSS, 1, -b.hw * 0.21, 0, PI / 2, 0.9, b.hw * 0.21 + 4, H * 0.7, H * 0.7 + 2, sl, MT_IRON); a.p[2] = 0.1; }
    if (g.lod == 0)
        for (int k = 0; k < 3; k++) {
            const double a = k * TAU / 3 + 0.4, d = b.hw * 0.9;
            const double dk = g.Dk(MT_IRON, 50 + k);
            if (dk > 0.85) continue;
            Form& m = g.add(FK_TRUSS, 0, std::sin(a) * d, std::cos(a) * d, 0, 1.2, 1.2, 0, H * 1.15, sl, MT_IRON); m.sides = 3; m.p[0] = 0.4; m.p[2] = 0.1; m.broken = dk < 0.3 ? 0 : clampd(dk, 0.3, 0.9);
        }
    if (g.lod < 2) { g.lathe(0, b.hw * 0.75, -b.hw * 0.75, 1.3, 0, 26, 0, MT_MASS); Form& tk = g.lathe(4, b.hw * 0.75, -b.hw * 0.75, 5, 26, 34, sl, MT_IRON); tk.p[0] = 0.3; tk.broken = Di > 0.5 ? 0.6 : 0; }
}

// a great hall: an assembly building's tall box with its door up a whole side (`style` 0), a mill's stepped sheds (1); the cladding goes
// first and the frame stands
void genHangar(Gen& g) {
    const Building& b = g.b;
    const int mat = b.age >= AGE_FUTURISTIC ? MT_COMPOSITE : (b.age >= AGE_MODERN ? MT_CONCRETE : MT_MASONRY);
    const double D = g.D(mat), Di = g.D(MT_IRON);
    if (b.style == 1) {
        for (int k = 0; k < 3; k++) {
            const double z = -b.hd + b.hd * (0.33 + 0.67 * k), h = b.hWall * (1 - 0.25 * k) * wallShare(D, g.U(10 + k));
            Form& e = g.box(0, z, 0, b.hw * (1 - 0.1 * k), b.hd * 0.34, 0, std::max(0.8, h), 0, mat); e.pattern = 2;
            if (h > b.hWall * 0.6 && Di < 0.6) { Form& r = g.add(FK_PYRAMID, 0, 0, z, 0, b.hw * (1 - 0.1 * k), b.hd * 0.34, h, h + 3, g.roofSlot(MT_IRON), MT_IRON, 2); r.sides = 4; r.p[1] = 0.1; }
        }
        return;
    }
    const double share = tallShare(D, g.U(3));
    if (Di < 0.75 || D < 0.4) {
        for (int s = 0; s < 4; s++) {
            const bool along = s % 2 == 0;
            const double off = along ? b.hd : b.hw, len = along ? b.hw : b.hd;
            if (s == 0) {   // the door's side: two jambs and the lintel high up
                for (int q = -1; q <= 1; q += 2) { Form& j = g.box(q * b.hw * 0.78, off, 0, b.hw * 0.22, 1.2, 0, b.hWall * share, 0, mat); j.pattern = 0; }
                Form& l = g.box(0, off, 0, b.hw * 0.56, 1.2, b.hWall * 0.82 * share, b.hWall * share, 0, mat); l.pattern = 0;
                continue;
            }
            Form& w = g.box(along ? 0 : (s == 1 ? off : -off), along ? -off : 0, along ? 0 : PI / 2, len, 1.2, 0, b.hWall * share, 0, mat); w.broken = share < 0.97 ? 0.3 : 0; w.pattern = 1;
        }
        if (share > 0.97 && D < 0.5) {   // R-412: its roof the people's: a barrel vault, a gable, a slab
            const int rf = g.c.roof;
            if (rf == RF_VAULTED || rf == RF_DOMED || rf == RF_SADDLE) { Form& v = g.add(FK_VAULT, rf == RF_SADDLE ? 2 : 0, 0, 0, 0, b.hw + 0.3, b.hd + 0.3, b.hWall, b.hWall + b.hw * 0.45, g.roofSlot(mat), mat, 2); v.p[0] = 0.5; v.p[1] = 1; }
            else if (rf == RF_PITCHED || rf == RF_HIPPED || rf == RF_CONICAL) { Form& r = g.add(FK_SWEEP, 1, 0, 0, 0, 0, b.hd + 0.5, b.hWall, b.hWall + b.hw * 0.35 * g.pitchK(), g.roofSlot(mat), mat, 2); r.p[1] = b.hw + g.eave(); r.p[2] = 0.15; }
            else g.box(0, 0, 0, b.hw + g.slabEave() * 0.5, b.hd + g.slabEave() * 0.5, b.hWall, b.hWall + 2, g.roofSlot(mat), mat, 2);
        }
    } else g.heap(0, 0, b.hw * 0.8, b.hd * 0.8, b.hWall * 0.08, 0, mat);
}

// a colossus on its plinth: the people's figure (its robed body, its shoulders and head, an arm raised with what it held where `style` 1);
// worn, its head and its arms gone; fallen, it lies along the ground beside the plinth, its head a way off
void genColossus(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double D = g.D(mat), H = b.hWall, ph = std::max(3.0, H * 0.18);
    g.sink = -c.buried * D;
    { Form& p = g.add(FK_PYRAMID, 0, 0, 0, 0, b.hw, b.hw, g.sink, g.sink + ph, 0, mat); p.p[0] = 2; p.p[1] = 0.8; p.p[2] = 0.7; }
    const double h = H - ph, r = h * 0.17;
    if (D < 0.6) {
        figure(g, 0, 0, g.sink + ph, h, g.accentOn(AP_TOPS) ? 1 : 0, mat, D * 0.7);
        if (b.style == 1 && D < 0.35 && g.lod < 2) { Form& a = g.lathe(0, r * 0.8, 0, r * 0.2, g.sink + ph + h * 0.62, g.sink + ph + h * 1.12, g.accentOn(AP_TOPS) ? 1 : 0, mat); a.tilt = 0.12; a.heading += PI / 2; }
        return;
    }
    const double dir = g.U(9) * TAU;   // fallen: the body along the ground, the head beyond it
    if (D < 0.9) {
        Form& e = g.lathe(1, std::sin(dir) * (b.hw + 1), std::cos(dir) * (b.hw + 1), r, 0, h * 0.72, 0, mat, 8); e.tilt = PI / 2; e.heading = b.heading + dir; e.p[0] = 0.62; e.broken = 0.4;
        if (g.lod == 0) { Form& hd = g.add(FK_BLOB, 0, std::sin(dir) * (b.hw + h * 0.95), std::cos(dir) * (b.hw + h * 0.95), 0, h * 0.1, h * 0.1, 0, h * 0.2, 0, mat); hd.p[0] = 0; hd.p[1] = 0.25; }
    }
}

// a wind turbine: its tower, the nacelle and hub, three blades facing the wind; the blades go first, then the nacelle, then the tower bends
void genTurbine(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double Di = g.D(MT_IRON), Dc = g.D(MT_COMPOSITE), H = b.hWall, R = b.hw;
    if (Di > 0.9) { if (g.lod == 0) g.fallen(FK_LATHE, 0, 0, 0, g.U(3) * TAU, 2.0, H * 0.6, 0, MT_IRON); return; }
    const double share = Di < 0.5 ? 1 : tallShare(Di, g.U(4));
    // R-412: a lattice tower where the people expressed its frames, its section the people's; its nacelle rounded where its roofs were domes or
    // vaults; two blades or three or four by its number
    if (g.gv(GV_EXPRESSION) == EX_FRAME) { Form& t = g.add(FK_TRUSS, 0, 0, 0, 0, 3.2, 3.2, 0, H * share, 0, MT_IRON); t.sides = 4; t.p[0] = 0.3; t.p[2] = 0.14; t.broken = share < 0.97 ? 0.5 : 0; }
    else { const int pl = g.gv(GV_PLAN); Form& t = g.lathe(1, 0, 0, 2.4, 0, H * share, 0, MT_IRON, pl == PF_POLYGON ? clampi(c.genome.number(), 5, 8) : 0); t.p[0] = 0.62; t.broken = share < 0.97 ? 0.3 : 0; }
    if (share < 0.97 || g.lod == 2) return;
    const bool round = c.roof == RF_DOMED || c.roof == RF_VAULTED || c.roof == RF_SADDLE;
    if (round) { g.box(0, -1.2, 0, 1.6, 3.9, H - 1.4, H + 0.1, 0, MT_IRON); Form& v = g.add(FK_VAULT, 0, 0, -1.2, 0, 1.6, 3.9, H + 0.1, H + 1.7, 0, MT_IRON); v.p[0] = 1.5; v.p[1] = 1; }   // a barrel over its body
    else g.box(0, -1.2, 0, 1.6, 4.5, H - 1.4, H + 1.8, 0, MT_IRON);
    g.lathe(0, 0, 3.6, 1.3, H - 0.9, H + 1.3, 0, MT_IRON);
    const int nb = clampi(c.genome.number() % 3 + 2, 2, 4);
    const int left = Dc < 0.3 ? nb : (Dc < 0.55 ? nb - 1 : (Dc < 0.75 ? 1 : 0));
    const double ph = g.U(6) * TAU;
    for (int k = 0; k < left; k++) {   // each blade a straight line from the hub in the rotor's plane (across the nacelle)
        const double a = ph + k * TAU / nb;
        double fx, fz; g.to(std::sin(a) * R, 3.6, fx, fz);
        Form& e = g.add(FK_CABLE, 0, 0, 3.6, 0, 0.8, 0.8, H, H + 0.2, 0, MT_COMPOSITE); e.p[0] = fx; e.p[1] = fz; e.p[2] = H + 0.2 + std::cos(a) * R; e.p[3] = 0;
    }
}

// a lighthouse: its tower tapering to the gallery, the lantern of glass under its cap, the keeper's house at its foot
// R-412: in the people's section (round, its polygon, square) and taper (a battered people's broad-footed, a slender people's a near cylinder),
// painted in bands of its accent where its people painted in stripes or two tones, its lantern under its people's cap (a dome, a cone, a
// flat lid), its gallery's rail; the keeper's house in the people's dress
void genLighthouse(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const int mat = greatOf(b, c); const double Dm = g.D(mat), H = b.hWall;
    const double share = tallShare(Dm, g.U(3));
    const int plan = g.gv(GV_PLAN), prof = g.gv(GV_PROFILE), sides = plan == PF_POLYGON ? clampi(c.genome.number(), 5, 8) : (plan == PF_RECT || plan == PF_COURT ? 4 : 0);
    const double taper = prof == WP_BATTERED ? 0.5 : (prof == WP_UPRIGHT ? 0.72 + 0.2 * g.gf(GV_PROPORTION) : 0.62);
    { Form& t = g.lathe(1, 0, 0, b.hw, 0, H * share, 0, mat, sides); t.p[0] = taper; t.broken = share < 0.97 ? 0.25 : 0; t.pattern = 2; }
    const int pc = g.gv(GV_POLYCHROMY);
    if ((pc == PC_STRIPED || pc == PC_TWO_TONE) && g.lod < 2 && Dm < 0.5) {   // its painted bands
        const int bands = pc == PC_STRIPED ? 3 : 1;
        for (int k = 0; k < bands; k++) {
            const double ya = H * (pc == PC_STRIPED ? (0.12 + 0.28 * k) : 0.5), yb = ya + H * (pc == PC_STRIPED ? 0.13 : 0.5);
            if (yb > H * share) break;
            const double ra = b.hw * (1 - (1 - taper) * ya / H) + 0.04, rb = b.hw * (1 - (1 - taper) * yb / H) + 0.04;
            Form& e = g.lathe(1, 0, 0, ra, ya, std::min(yb, H * share), 1, mat, sides); e.p[0] = rb / ra; e.part = 4;
        }
    }
    if (share >= 0.97 && g.lod < 2) {
        const double rt = b.hw * taper;
        g.lathe(0, 0, 0, rt * 1.25, H, H + 0.5, 0, mat, sides);
        if (g.lod == 0 && g.D(MT_IRON) < 0.5) { Form& r = g.add(FK_TORUS, 0, 0, 0, 0, rt * 1.18, rt * 1.18, H + 0.5, H + 1.6, g.ironSlot(), MT_IRON); r.p[0] = 0.05; }   // the gallery's rail
        if (g.D(MT_GLASS) < 0.9) g.lathe(0, 0, 0, rt * 0.8, H + 0.5, H + 3.2, 2, MT_GLASS, 8);
        if (g.D(MT_IRON) < 0.6) {
            const int rf = c.roof;
            if (rf == RF_FLAT || rf == RF_OPEN || rf == RF_STEPPED) g.lathe(0, 0, 0, rt * 0.95, H + 3.2, H + 3.55, g.roofSlot(MT_IRON), MT_IRON, 8);
            else if (rf == RF_PITCHED || rf == RF_HIPPED || rf == RF_CONICAL) { Form& k = g.lathe(1, 0, 0, rt * 0.92, H + 3.2, H + 3.2 + rt * 1.6 * g.pitchK(), g.roofSlot(MT_IRON), MT_IRON, 8); k.p[0] = 0.05; }
            else g.add(FK_DOME, ageDome(g, rf == RF_DOMED && (g.gv(GV_MOTIF) == MO_SPIRALS || g.gv(GV_MOTIF) == MO_FIGURES) ? 3 : 2), 0, 0, 0, rt * 0.88, rt * 0.88, H + 3.2, H + 4.6, g.roofSlot(MT_IRON), MT_IRON, 2);
        }
    }
    if (g.lod == 2) return;
    Walls W; W.cx = b.hw + 6; W.hw = 3.5; W.hd = 4.5; W.h = 3.2; W.thick = 0.5; W.door = 3; W.doorPos = 0; W.mat = stoneOf(c); W.k0 = 30; W.pattern = 1;
    double hs[4]; const double hmin = walls4(g, W, hs);
    const int rf = c.age >= AGE_MODERN ? RF_FLAT : (c.roof == RF_OPEN ? RF_PITCHED : c.roof), rm = roofMaterial(g, rf, W.mat);
    const double rise = roof(g, rf, W.cx, 0, W.hw, W.hd, hmin, W.h, 0.6, rm);
    if (rf == RF_PITCHED) gables(g, W, hs, 0.6);
    entranceAt(g, W, wallsHalf(g, W), W.h);
    cornersOf(g, W, hs, wallsHalf(g, W), rm);
    rooflineOf(g, W, hmin, rf, rise, rm);
}

// the far future's lattice over a valley: girders across and along at one height over the reference level (`base` + hWall), a pillar down
// to the ground at its corners; nothing of it weathers
void genLattice(Gen& g) {
    const Building& b = g.b;
    const double y = b.base + b.hWall, cell = std::max(40.0, std::min(b.hw, b.hd) / 3);
    const int nx = std::max(1, (int)std::lround(b.hw / cell)), nz = std::max(1, (int)std::lround(b.hd / cell));
    for (int i = -nx; i <= nx; i++) { Form& e = g.add(FK_TRUSS, 1, i * b.hw / nx, 0, 0, 1.6, b.hd, y - 3, y, 0, MT_ETERNAL); e.p[2] = 0.35; e.absolute = true; }
    for (int j = -nz; j <= nz; j++) { Form& e = g.add(FK_TRUSS, 1, 0, j * b.hd / nz, PI / 2, 1.6, b.hw, y - 3, y, 0, MT_ETERNAL); e.p[2] = 0.35; e.absolute = true; }
    for (int s = -1; s <= 1; s += 2) for (int q = -1; q <= 1; q += 2) { Form& p = g.lathe(0, s * b.hw, q * b.hd, 3.5, b.base - 40, y, 0, MT_ETERNAL, 4); p.absolute = true; }
    if (g.lod == 0) { Form& l = g.box(0, 0, 0, b.hw, 0.4, y + 0.05, y + 0.35, 3, MT_ETERNAL, 4); l.absolute = true; }
}

// a quarry's derrick: a timber mast and its boom on guys (`style` 0), or an iron one of lattice (1)
void genDerrick(Gen& g) {
    const Building& b = g.b;
    const int mat = b.style == 0 ? MT_TIMBER : MT_IRON;
    const double D = g.D(mat), H = b.hWall;
    if (D > 0.8) { if (g.lod == 0) g.fallen(b.style == 0 ? FK_LATHE : FK_TRUSS, b.style == 0 ? 0 : 0, 0, 0, g.U(3) * TAU, 0.3, H * 0.7, frameSlot(g, mat), mat, b.style == 0 ? 6 : 4); return; }
    const int sl = frameSlot(g, mat);
    if (b.style == 0) g.lathe(0, 0, 0, 0.28, 0, H, sl, mat, 6);
    else { Form& m = g.add(FK_TRUSS, 0, 0, 0, 0, 0.9, 0.9, 0, H, sl, mat); m.sides = 4; m.p[0] = 0.5; m.p[2] = 0.08; m.broken = D < 0.3 ? 0 : D; }
    if (g.lod == 2 || D > 0.45) return;
    Form& boom = b.style == 0 ? g.lathe(0, 0, 0.4, 0.2, 1.2, 1.2 + H * 0.85, sl, mat, 6) : g.add(FK_TRUSS, 0, 0, 0.4, 0, 0.5, 0.5, 1.2, 1.2 + H * 0.85, sl, mat);
    boom.tilt = 0.75; if (b.style == 1) { boom.sides = 3; boom.p[2] = 0.06; }
    if (g.lod == 0)
        for (int k = 0; k < 3; k++) {
            double a = g.U(11) * TAU + k * TAU / 3, d = H * 0.8, fx, fz; g.to(std::sin(a) * d, std::cos(a) * d, fx, fz);
            Form& cb = g.add(FK_CABLE, 0, 0, 0, 0, 0.03, 0.03, 0, H, sl, mat); cb.p[0] = fx; cb.p[1] = fz; cb.p[2] = 0; cb.p[3] = H * 0.01;
        }
}

// blocks cut and left: in rows on the floor, a few stacked; the rock's own
void genBlocks(Gen& g) {
    const Building& b = g.b;
    const int n = 4 + (int)(g.U(2) * 8);
    if (g.lod == 2) { g.box(0, 0, 0, b.hw, b.hd, 0, 1.2, 0, MT_DRYSTONE); return; }
    for (int k = 0; k < n; k++) {
        const double x = -b.hw + 2 * b.hw * g.U(10 + k), z = -b.hd + 2 * b.hd * g.U(30 + k);
        const double sx = 0.7 + 0.5 * g.U(50 + k), sz = 0.5 + 0.35 * g.U(70 + k), h = 0.8 + 0.6 * g.U(90 + k);
        g.box(x, z, (g.U(110 + k) - 0.5) * 0.3, sx, sz, 0, h, 0, MT_DRYSTONE, 3);
        if (k % 3 == 0) g.box(x, z, (g.U(130 + k) - 0.5) * 0.4, sx * 0.85, sz * 0.9, h, h + 0.7 + 0.4 * g.U(150 + k), 0, MT_DRYSTONE, 3);
    }
}

// the far future's sphere resting on its point (`style` 0): it never weathers, and nothing holds it up but a needle; or a pillar (1)
void genSphere(Gen& g) {
    const Building& b = g.b;
    if (b.style == 1) {   // a pillar of a line to the horizon: a square prism, a line of light up one face
        g.lathe(0, 0, 0, b.hw, 0, b.hWall, 0, MT_ETERNAL, 4);
        if (g.lod < 2) g.box(0, b.hw * 0.72, 0, 0.25, 0.1, 0, b.hWall, 3, MT_ETERNAL, 4);
        return;
    }
    const double r = b.hw, y0 = std::max(4.0, r * 0.08);
    { Form& s = g.lathe(4, 0, 0, r, y0, y0 + 2 * r, 0, MT_ETERNAL); s.p[0] = 1.0; }
    { Form& n = g.lathe(2, 0, 0, std::max(0.6, r * 0.02), 0, y0 + r * 0.2, 3, MT_ETERNAL); n.p[0] = 1.0; }
}

// a space elevator's anchor: its ring, the tower, and the ribbon up out of sight; the ribbon gone once the composite is a third worn
void genTether(Gen& g) {
    const Building& b = g.b;
    const double D = g.D(MT_COMPOSITE), H = b.hWall;
    { Form& r = g.add(FK_TORUS, 0, 0, 0, 0, b.hw, b.hw, 0, 8, 0, MT_COMPOSITE); r.p[0] = 6; }
    const double share = D < 0.3 ? 1 : tallShare(D, g.U(3));
    { Form& t = g.lathe(1, 0, 0, b.hw * 0.25, 0, H * share, 0, MT_COMPOSITE, 6); t.p[0] = 0.35; t.broken = share < 0.97 ? 0.3 : 0; t.pattern = 1; }
    if (share >= 0.97 && D < 0.3 && g.lod < 2) {
        double fx, fz; g.to(0, 0, fx, fz);
        Form& c = g.add(FK_CABLE, 0, 0, 0, 0, 0.8, 0.8, H, H + 1, 3, MT_COMPOSITE); c.p[0] = fx; c.p[1] = fz; c.p[2] = 60000; c.p[3] = 0;
    }
}

// a trilithon: two uprights and the lintel across them; the lintel down beside them first
void genTrilithon(Gen& g) {
    const Building& b = g.b;
    const double H = b.hWall, w = b.hw * 0.32;
    for (int s = -1; s <= 1; s += 2) standingStone(g, s * (b.hw - w), 0, 0, w, b.hd, H, 100 + s, false);
    const double d = std::max(g.Dk(MT_DRYSTONE, 99), std::max(g.Dk(MT_DRYSTONE, 101), g.Dk(MT_DRYSTONE, 102)));
    if (d < 0.3) g.box(0, 0, 0, b.hw, b.hd * 0.9, H, H + b.hd * 1.4, 0, MT_DRYSTONE, 5);
    else if (d < 0.9 && g.lod == 0) { Form& l = g.box(0, b.hd + 1.4, 0.3 * (g.U(5) - 0.5), b.hw, b.hd * 0.9, 0, b.hd * 1.4, 0, MT_DRYSTONE, 5); l.tilt = 0.05; }
}

// a conveyor: its girder along the heading at hWall over the ground, on legs every twenty metres
void genConveyor(Gen& g) {
    const Building& b = g.b;
    const double Di = g.D(MT_IRON);
    if (Di > 0.85) return;
    const int sl = g.ironSlot();
    { Form& e = g.add(FK_TRUSS, 1, 0, 0, 0, 1.2, b.hd, b.hWall, b.hWall + 2, sl, MT_IRON); e.p[2] = 0.1; e.broken = Di < 0.3 ? 0 : clampd(Di, 0.3, 0.9); }
    if (g.lod == 2) return;
    const int n = std::max(1, (int)(2 * b.hd / 20));
    for (int k = 0; k <= n; k++) { Form& l = g.lathe(0, 0, -b.hd + 2 * b.hd * k / n, 0.3, 0, b.hWall, sl, MT_IRON, 4); (void)l; }
}

// D-05: a dome over the warm ground, a people that lost its sun's (`hw` its radius, `hWall` its ring's height): an industrial people's iron ribs
// and their glass on a ring of brick (`style` 0), a modern people's steel and glass on a ring of concrete (1), the far future's whole (2). The glass
// went at once; the ribs (half rings standing across the ring, crossing at its crown, and hoops round them) rust to stumps over centuries; the
// ring stays. A futuristic people's dome is a habitat's (`BK_HABITAT`)
int domeRingSegs(double r) { return clampi((int)std::lround(r * 0.6), 16, 28); }
void genDome(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double r = b.hw;
    if (b.style == 2) {   // the far future's: nothing weathers
        { Form& t = g.add(FK_TORUS, 0, 0, 0, 0, r, r, 0, 4, 1, MT_ETERNAL); t.p[0] = 2.0; }
        g.add(FK_DOME, 1, 0, 0, 0, r, r, 2.0, 2.0 + r * 0.55, 2, MT_ETERNAL);
        return;
    }
    const int mat = b.style == 0 ? brickOf(c) : MT_CONCRETE;
    const double Dm = g.D(mat), Dg = g.D(MT_GLASS);
    g.sink = -c.buried * Dm;
    Ring R; R.rx = R.rz = r; R.h = b.hWall; R.thick = 1.0; R.mat = mat; R.dress = false; R.k0 = 10;
    R.segs = domeRingSegs(r); R.doorSegs = ringDoorSegs(r, R.segs); R.door = ringDoorFirst(b, R.segs, R.doorSegs);
    ringWalls(g, R);
    if (g.lod == 2) return;
    const double yTop = g.sink + b.hWall, tube = clampd(r * 0.018, 0.45, 0.8);
    const int n = b.style == 0 ? 6 : 8, is = g.ironSlot();
    for (int k = 0; k < n; k++) {   // the ribs: a ring standing on its rim, its middle at the ring's top (its lower half in the ground)
        const double d = g.Dk(MT_IRON, 20 + k);
        if (d > 0.9) continue;
        Form& t = g.add(FK_TORUS, 0, 0, 0, k * PI / n, r, r, yTop - r - tube, yTop + r + tube, is, MT_IRON);
        t.p[0] = tube; t.tilt = PI / 2; t.broken = d < 0.3 ? 0 : clampd(d, 0.3, 0.95);
    }
    for (int q = 1; q <= (b.style == 0 ? 2 : 3); q++) {   // the hoops round them
        const double ph = q * (PI / 2) / (b.style == 0 ? 3 : 4), d = g.Dk(MT_IRON, 40 + q);
        if (d > 0.8) continue;
        const double rr = r * std::cos(ph), y = yTop + r * std::sin(ph);
        Form& t = g.add(FK_TORUS, 0, 0, 0, 0, rr, rr, y - tube, y + tube, is, MT_IRON); t.p[0] = tube * 0.8; t.broken = d < 0.3 ? 0 : clampd(d, 0.3, 0.9);
    }
    if (Dg < 0.8) { Form& e = g.add(FK_DOME, 0, 0, 0, 0, r, r, yTop, yTop + r, 2, MT_GLASS); e.broken = Dg < 0.3 ? 0 : clampd(Dg, 0.3, 0.95); }
}

// D-05: a glasshouse, a people that lost its sun's (`hw` across, `hd` along its heading): an industrial people's iron hoops and their glass on low
// walls of brick (`style` 0, a conservatory's curve), a modern people's tunnel of steel hoops from the ground under its sheet (1), a futuristic or
// a later people's vault of composite (2). The glass and the sheet went at once; the hoops stand over their walls
void genGreenhouse(Gen& g) {
    const Building& b = g.b; const Culture& c = g.c;
    const double hw = b.hw, hd = b.hd;
    if (b.style >= 2) {
        const int mat = c.age >= AGE_FAR_FUTURE ? MT_ETERNAL : MT_COMPOSITE; const double D = g.D(mat);
        Form& v = g.add(FK_VAULT, 2, 0, 0, 0, hw, hd, 0, b.hWall, 2, mat); v.p[0] = 0.3; v.p[1] = 1; v.broken = D < 0.3 ? 0 : clampd(D, 0.3, 0.95);
        return;
    }
    const int mat = b.style == 0 ? brickOf(c) : MT_CONCRETE;
    const double Dm = g.D(mat), Dg = g.D(MT_GLASS);
    g.sink = -c.buried * Dm;
    const double wall = b.style == 0 ? 0.9 : 0.3, th = b.style == 0 ? 0.22 : 0.15;
    for (int s = -1; s <= 1; s += 2) g.box(s * hw, 0, 0, th, hd, g.sink, g.sink + std::max(0.2, wall * wallShare(Dm, g.U(10 + s))), 0, mat);
    if (b.style == 0 && g.lod < 2) {   // its ends: the back whole, the front either side of its door
        g.box(0, -hd, 0, hw, th, g.sink, g.sink + wall * wallShare(Dm, g.U(12)), 0, mat);
        for (int s = -1; s <= 1; s += 2) g.box(s * (hw * 0.5 + 0.6), hd, 0, hw * 0.5 - 0.6, th, g.sink, g.sink + wall * wallShare(Dm, g.U(13 + s)), 0, mat);
    }
    if (g.lod == 2) return;
    const double yTop = g.sink + wall, tube = b.style == 0 ? 0.12 : 0.08, step = b.style == 0 ? 2.5 : 2.0;
    const int n = std::max(2, (int)std::lround(2 * hd / step)), is = g.ironSlot();
    for (int k = 0; k <= n; k++) {   // a hoop across it every few metres (a ring standing on its rim, facing along the house)
        const double d = g.Dk(MT_IRON, 30 + k);
        if (d > 0.9) continue;
        Form& t = g.add(FK_TORUS, 0, 0, -hd + 2 * hd * k / n, 0, hw, hw, yTop - hw - tube, yTop + hw + tube, is, MT_IRON);
        t.p[0] = tube; t.tilt = PI / 2; t.broken = d < 0.3 ? 0 : clampd(d, 0.3, 0.95);
    }
    if (Dg < 0.8) { Form& v = g.add(FK_VAULT, 0, 0, 0, 0, hw, hd, yTop, yTop + hw, 2, MT_GLASS); v.p[0] = 0.05; v.broken = Dg < 0.3 ? 0 : clampd(Dg, 0.3, 0.95); }
}
}   // namespace

// ---- the dispatch ------------------------------------------------------------------------------------------------------------

void buildingForms(const Building& b, const Culture& c, const SiteRead& st, int lod, std::vector<Form>& out, int bi) {
    Gen g(b, c, st, lod, out, bi);
    switch (b.kind) {
        case BK_HOUSE: case BK_HALL: case BK_TOWER: genHouse(g); break;
        case BK_ROTUNDA: case BK_POD: genRotunda(g); break;
        case BK_GATE: genGate(g); break;
        case BK_PLATFORM: genPlatform(g); break;
        case BK_COLONNADE: genColonnade(g); break;
        case BK_STELA: genStela(g); break;
        case BK_RUBBLE: genRubble(g); break;
        case BK_WALL: genWall(g); break;
        case BK_QUAY: case BK_MOLE: case BK_BOLLARD: case BK_TERRACE: case BK_CISTERN: case BK_WINDWALL: case BK_CAIRN: genFeature(g); break;
        case BK_HUT: genHut(g); break;
        case BK_LONGHOUSE: genLonghouse(g); break;
        case BK_TENT_RING: genTentRing(g); break;
        case BK_STONE_CIRCLE: genStoneCircle(g); break;
        case BK_MENHIR: genMenhir(g); break;
        case BK_DOLMEN: genDolmen(g); break;
        case BK_PALISADE: genPalisade(g); break;
        case BK_TEMPLE: genTemple(g); break;
        case BK_WALL_TOWER: genWallTower(g); break;
        case BK_ROW: genRow(g); break;
        case BK_TENEMENT: genTenement(g); break;
        case BK_VILLA: genVilla(g); break;
        case BK_CHURCH: genChurch(g); break;
        case BK_TOWN_HALL: genTownHall(g); break;
        case BK_STATION: genStation(g); break;
        case BK_MILL: genMill(g); break;
        case BK_GASOMETER: genGasometer(g); break;
        case BK_WATER_TOWER: genWaterTower(g); break;
        case BK_HEADFRAME: genHeadframe(g); break;
        case BK_OBSERVATORY: genObservatory(g); break;
        case BK_BLOCK: genBlock(g); break;
        case BK_SUBURB: genSuburb(g); break;
        case BK_HIGHRISE: genHighrise(g); break;
        case BK_STADIUM: genStadium(g); break;
        case BK_MAST: genMast(g); break;
        case BK_POWER: genPower(g); break;
        case BK_TANKS: genTanks(g); break;
        case BK_ARCOLOGY: genArcology(g); break;
        case BK_HABITAT: genHabitat(g); break;
        case BK_SPIRE: genSpire(g); break;
        case BK_RING: genRing(g); break;
        case BK_MAGLEV: genMaglev(g); break;
        case BK_WRECK: genWreck(g); break;
        case BK_SPIKE: case BK_HALO: case BK_PILLARS: case BK_SHAFT: case BK_MONOLITH: genFar(g); break;
        case BK_FOCUS: genFocus(g); break;
        case BK_WINDMILL: genWindmill(g); break;
        case BK_AQUEDUCT: genAqueduct(g); break;
        case BK_TWIN: genTwin(g); break;
        case BK_PORTAL: genPortal(g); break;
        case BK_CRANE: genCrane(g); break;           // T-02
        case BK_CONTAINERS: genContainers(g); break;
        case BK_RESERVOIR: genReservoir(g); break;
        case BK_MOUND: genMound(g); break;           // T-03
        case BK_DAM: genDam(g); break;
        case BK_DISH: genDish(g); break;
        case BK_TOMB: genTomb(g); break;
        case BK_FORT: genFort(g); break;
        case BK_BUNKER: genBunker(g); break;
        case BK_TEETH: genTeeth(g); break;
        case BK_VEHICLE: genVehicle(g); break;
        case BK_AIRCRAFT: genAircraft(g); break;
        case BK_SHIP: genShip(g); break;
        case BK_LAUNCHPAD: genLaunchpad(g); break;
        case BK_HANGAR: genHangar(g); break;
        case BK_COLOSSUS: genColossus(g); break;
        case BK_TURBINE: genTurbine(g); break;
        case BK_LIGHTHOUSE: genLighthouse(g); break;
        case BK_LATTICE: genLattice(g); break;
        case BK_DERRICK: genDerrick(g); break;
        case BK_BLOCKS: genBlocks(g); break;
        case BK_SPHERE: genSphere(g); break;
        case BK_TETHER: genTether(g); break;
        case BK_TRILITHON: genTrilithon(g); break;
        case BK_CONVEYOR: genConveyor(g); break;
        case BK_FURNITURE: genFurniture(g); break;   // R-412
        case BK_DOME: genDome(g); break;             // D-05
        case BK_GREENHOUSE: genGreenhouse(g); break;
        default: break;
    }
}

// ---- the rooms ---------------------------------------------------------------------------------------------------------------

namespace {
// a rectangular room's spots by its door (C-03's: the far corners from the door, then the corners beside the door's wall, then the
// back's middle), in the building's frame round (cx, cz)
void rectSpots(BuildingRoom& R, int door, double cx, double cz, double hw, double hd) {
    static const double uv[5][2] = {{-0.55, -0.55}, {0.55, -0.55}, {-0.55, 0.35}, {0.55, 0.35}, {0, -0.6}};
    for (int k = 0; k < 5; k++) {
        double uu = uv[k][0], vv = uv[k][1], lx, lz;
        switch (door) {
            case 0: lx = uu * hw; lz = vv * hd; break;
            case 2: lx = -uu * hw; lz = -vv * hd; break;
            case 1: lx = vv * hw; lz = uu * hd; break;
            default: lx = -vv * hw; lz = uu * hd; break;
        }
        R.spots.push_back({cx + lx, cz + lz});
    }
}
void toSettlement(const Building& b, double lx, double lz, double& x, double& z) {
    x = b.x + std::cos(b.heading) * lx + std::sin(b.heading) * lz;
    z = b.z - std::sin(b.heading) * lx + std::cos(b.heading) * lz;
}
// a rectangular room with its doorway on `side` at `pos`
void rectRoom(const Building& b, BuildingRoom& R, double cx, double cz, double hw, double hd, int side, double pos) {
    R.round = false; R.cx = cx; R.cz = cz; R.hw = hw; R.hd = hd; R.door = true;
    double lx, lz; doorMid(side, pos, hw, hd, lx, lz);
    toSettlement(b, cx + lx, cz + lz, R.doorX, R.doorZ);
    const bool along = side == 0 || side == 2;
    double ax = along ? 1 : 0, az = along ? 0 : -1;   // the doorway wall's axis in the building's frame, to the settlement's
    R.doorAx = std::cos(b.heading) * ax + std::sin(b.heading) * az; R.doorAz = -std::sin(b.heading) * ax + std::cos(b.heading) * az;
    R.doorL = along ? hw : hd;
}
// a ring's room (an ellipse of radii rx across, rz along, in `segs` straight segments) with the doorway's segment(s) left out from
// `first`: the doorway's midpoint on the chords' line, its axis along their run
void roundRoom(const Building& b, BuildingRoom& R, double rx, double rz, int segs, int first, int count, double cz = 0) {
    R.round = true; R.r = std::min(rx, rz); R.door = true; R.cz = cz;
    const double a0 = first * TAU / segs, a1 = (first + count) * TAU / segs;
    const double x0 = std::sin(a0) * rx, z0 = std::cos(a0) * rz + cz, x1 = std::sin(a1) * rx, z1 = std::cos(a1) * rz + cz;
    toSettlement(b, 0.5 * (x0 + x1), 0.5 * (z0 + z1), R.doorX, R.doorZ);
    const double dx = x1 - x0, dz = z1 - z0, L = std::sqrt(dx * dx + dz * dz), ux = dx / std::max(1e-9, L), uz = dz / std::max(1e-9, L);
    R.doorAx = std::cos(b.heading) * ux + std::sin(b.heading) * uz; R.doorAz = -std::sin(b.heading) * ux + std::cos(b.heading) * uz; R.doorL = L * 0.5;
    const double am = 0.5 * (a0 + a1);
    R.spots.push_back({-std::sin(am) * rx * 0.55, cz - std::cos(am) * rz * 0.55});
    R.spots.push_back({-std::sin(am) * rx * 0.3, cz - std::cos(am) * rz * 0.3});
}
// a house's room on its plan (A-04): a ring's, the L's front wing, a court (the spots in it), C-01's rectangle
void houseRoom(const Building& b, BuildingRoom& R, bool roundable) {
    if (roundable && (b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE)) {
        const double rx = b.hw, rz = b.form == PF_ELLIPSE ? b.hd : b.hw;
        const int segs = ringSegs(b.form, b.sides, std::min(rx, rz)), ds = ringDoorSegs(std::min(rx, rz), segs);
        roundRoom(b, R, rx, rz, segs, ringDoorFirst(b, segs, ds), ds);
        return;
    }
    if (b.form == PF_FREE) {   // the front wing (the hall's and the tower's plans stay their own)
        const double pos = b.kind == BK_BLOCK ? frontDoorPos(b, b.hw) : houseDoorPos(b, b.hw), cz = b.kind == BK_BLOCK ? b.hd * 0.55 : b.hd * 0.44, hd = b.kind == BK_BLOCK ? b.hd * 0.45 : b.hd * 0.56;
        rectRoom(b, R, 0, cz, b.hw, hd, 0, pos); rectSpots(R, 0, 0, cz, b.hw, hd);
        return;
    }
    if (b.form == PF_COURT) {   // the court: the doorway through the front range, the spots in the court
        const double ring = b.kind == BK_BLOCK ? 6.0 : 3.4, pos = b.kind == BK_BLOCK ? frontDoorPos(b, b.hw - 4) : houseDoorPos(b, b.hw - 3.6);
        rectRoom(b, R, 0, 0, b.hw, b.hd, 0, pos);
        const double cw = b.hw - ring, cd = b.hd - ring;
        R.spots = {{-cw * 0.5, -cd * 0.5}, {cw * 0.5, -cd * 0.5}, {0, -cd * 0.4}, {-cw * 0.4, cd * 0.2}};
        return;
    }
    const double doorL = (b.door == 0 || b.door == 2) ? b.hw : b.hd;
    rectRoom(b, R, 0, 0, b.hw, b.hd, b.door, houseDoorPos(b, doorL));
    rectSpots(R, b.door, 0, 0, b.hw, b.hd);
}
}   // namespace

bool buildingRoom(const Building& b, BuildingRoom& R) {
    R = BuildingRoom();
    if (b.found == FD_STILTS && (b.kind == BK_HOUSE || b.kind == BK_BLOCK)) {   // A-04: raised on stilts: the records lie on the ground under its floor
        R.outside = true; R.hw = b.hw; R.hd = b.hd;
        R.spots = {{0, 0}, {b.hw * 0.4, -b.hd * 0.3}, {-b.hw * 0.4, b.hd * 0.3}};
        return true;
    }
    switch (b.kind) {
        case BK_HOUSE: houseRoom(b, R, true); return true;
        case BK_HALL:
            if (b.style == 1) { R.hw = b.hw; R.hd = b.hd; R.spots = {{0, 0}, {0, -b.hd * 0.4}, {0, b.hd * 0.4}}; return true; }   // the market hall: open on every side
            if (b.style == 2) { rectRoom(b, R, 0, 0, b.hw, b.hd, 0, 0); const double cw = b.hw - 4.0, cd = b.hd - 4.0; R.spots = {{-cw * 0.5, -cd * 0.5}, {cw * 0.5, -cd * 0.5}, {0, -cd * 0.4}}; return true; }   // the palace's court
            rectRoom(b, R, 0, 0, b.hw, b.hd, b.door, houseDoorPos(b, (b.door == 0 || b.door == 2) ? b.hw : b.hd)); R.spots = {{0, -b.hd + 1.3}, {0, -b.hd * 0.3}, {0, 0.2}};
            return true;
        case BK_TOWER: houseRoom(b, R, true); if (!R.round) { R.spots.clear(); rectSpots(R, b.door, 0, 0, b.hw, b.hd); } return true;
        case BK_ROTUNDA: case BK_POD: {
            const int form = b.form == PF_RECT || b.form == PF_FREE || b.form == PF_COURT ? PF_CIRCLE : b.form;
            const double rx = b.hw, rz = form == PF_ELLIPSE ? b.hd : b.hw;
            const int segs = ringSegs(form, b.sides, std::min(rx, rz)), ds = ringDoorSegs(std::min(rx, rz), segs);
            roundRoom(b, R, rx, rz, segs, ringDoorFirst(b, segs, ds), ds); return true;
        }
        case BK_OBSERVATORY: { const int ds = ringDoorSegs(b.hw, 12); roundRoom(b, R, b.hw, b.hw, 12, ringDoorFirst(b, 12, ds), ds); return true; }
        case BK_HUT:
            if (b.form == PF_RECT || b.form == PF_COURT) { rectRoom(b, R, 0, 0, b.hw, b.hd, 0, 0); R.spots = {{0, -b.hd * 0.5}, {-b.hw * 0.4, -b.hd * 0.3}}; return true; }
            roundRoom(b, R, b.hw, b.form == PF_ELLIPSE ? b.hd : b.hw, hutSegs(b), hutDoorSeg(b), hutDoorSegs(b)); return true;
        case BK_LONGHOUSE: rectRoom(b, R, 0, 0, b.hw, b.hd, 0, 0); R.spots = {{0, -b.hd + 1.2}, {0, -b.hd * 0.55}, {0, b.hd * 0.55}}; return true;
        case BK_TENT_RING: R.round = true; R.r = b.hw; R.spots = {{0, 0}, {b.hw * 0.35, 0}}; return true;
        case BK_STONE_CIRCLE: R.round = true; R.outside = true; R.r = b.hw; R.spots = {{0, 1.5}, {0, -1.5}, {1.8, 0}}; return true;
        case BK_MENHIR: R.outside = true; R.hw = 1; R.hd = 1; R.spots = b.style == 1 ? std::vector<std::pair<double, double>>{{0, 0}, {0, b.hd * 0.5}} : std::vector<std::pair<double, double>>{{0, 1.6}, {0, -1.6}, {1.6, 0}}; return true;
        case BK_STELA:
            R.outside = true; R.hw = 1; R.hd = 1;
            if (b.style == MN_ARCHES) R.spots = {{0, 1.8}, {0, -1.8}};
            else if (b.style == MN_COLOSSI || b.style == MN_PILLARS) R.spots = {{0, 1.6}, {0, -1.6}};
            else R.spots = {{0, 0.8}, {0, -0.8}};
            return true;
        case BK_DOLMEN: rectRoom(b, R, 0, 0, b.hw, b.hd, 0, 0); R.spots = {{0, -b.hd * 0.35}, {0, 0}}; return true;
        case BK_TEMPLE:   // the nave, the domed hall, the house at the stepped mass's foot, the round temple (`genTemple`)
            if (b.style == 2) { rectRoom(b, R, 0, b.hd - 3.5, 3.2, 3.0, 0, 0); rectSpots(R, 0, 0, b.hd - 3.5, 3.2, 3.0); }
            else if (b.style == 1) { rectRoom(b, R, 0, 0, b.hw, b.hw, 0, 0); R.spots = {{0, -b.hw + 2.0}, {-b.hw * 0.5, -b.hw * 0.5}, {b.hw * 0.5, -b.hw * 0.5}}; }
            else if (b.style == 3) { const int segs = templeRingSegs(b), ds = ringDoorSegs(b.hw, segs); roundRoom(b, R, b.hw, b.form == PF_ELLIPSE ? b.hw * 1.25 : b.hw, segs, ((segs - ds / 2) % segs + segs) % segs, ds); }
            else { rectRoom(b, R, 0, 0, b.hw, b.hd, 0, 0); R.spots = {{0, -b.hd + 2.0}, {0, -b.hd * 0.4}, {0, 0}}; }
            return true;
        case BK_ROW: case BK_TENEMENT: case BK_SUBURB:
            if (b.kind == BK_SUBURB && (b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE)) { houseRoom(b, R, true); return true; }
            rectRoom(b, R, 0, 0, b.hw, b.hd, 0, frontDoorPos(b, b.hw)); rectSpots(R, 0, 0, 0, b.hw, b.hd); return true;
        case BK_BLOCK:
            if (b.style == 0 && b.form != PF_RECT) { if (b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE) houseRoom(b, R, true); else houseRoom(b, R, false); return true; }
            rectRoom(b, R, 0, 0, b.hw, b.hd, 0, frontDoorPos(b, b.hw)); rectSpots(R, 0, 0, 0, b.hw, b.hd); return true;
        case BK_VILLA:
            if (b.form == PF_CIRCLE || b.form == PF_POLYGON || b.form == PF_ELLIPSE) { houseRoom(b, R, true); return true; }
            rectRoom(b, R, 0, 0, b.hw, b.hd, 0, 0); rectSpots(R, 0, 0, 0, b.hw, b.hd); return true;
        case BK_CHURCH: case BK_TOWN_HALL: case BK_HIGHRISE:
            rectRoom(b, R, 0, 0, b.hw, b.hd, 0, 0);
            if (b.kind == BK_CHURCH) R.spots = {{0, -b.hd + 1.5}, {0, -b.hd * 0.4}, {0, 0}}; else rectSpots(R, 0, 0, 0, b.hw, b.hd);
            return true;
        case BK_STATION:
            if (b.style == 1) { R.hw = std::min(b.hw * 0.9, 18.0); R.hd = b.hd; R.spots = {{0, 0}, {0, -b.hd * 0.5}, {0, b.hd * 0.5}}; return true; }   // the market hall's floor under its vault
            rectRoom(b, R, 0, b.hd - 5.5, b.hw, 5.5, 0, 0); rectSpots(R, 0, 0, b.hd - 5.5, b.hw, 5.5); return true;
        case BK_MILL: {   // B-414: its doorway in its front, its floor to the chimney's heap in its back end
            const double back = millFloorBack(b), cz = 0.5 * (back + b.hd), hd = 0.5 * (b.hd - back);
            rectRoom(b, R, 0, cz, b.hw, hd, 0, 0); rectSpots(R, 0, 0, cz, b.hw, hd); return true;
        }
        case BK_HEADFRAME: rectRoom(b, R, 0, -b.hd * 0.3, 5, 6.5, 0, 0); rectSpots(R, 0, 0, -b.hd * 0.3, 5, 6.5); return true;
        case BK_STADIUM: R.round = true; R.outside = true; R.r = b.hw; R.spots = {{0, 0}, {b.hw * 0.3, 0}}; return true;
        case BK_ARCOLOGY: case BK_HABITAT: case BK_SPIRE: {   // the gate hall before it
            const double z0 = portalZ(b.hw) + PORTAL_HD;
            rectRoom(b, R, 0, z0, PORTAL_HW - 1.2, PORTAL_HD, 0, 0);
            R.spots = {{0, z0 - PORTAL_HD * 0.6}, {0, z0}, {0, z0 + PORTAL_HD * 0.5}};
            return true;
        }
        case BK_RING: case BK_WRECK: case BK_SPIKE: case BK_HALO: case BK_PILLARS: case BK_SHAFT: case BK_MONOLITH: {   // at its foot
            R.outside = true;
            double r = b.kind == BK_PILLARS ? b.hw : (b.kind == BK_RING || b.kind == BK_HALO ? std::max(3.0, b.hw * 0.06) * 3 : b.hw);
            if (b.kind == BK_WRECK) r = b.hw * 1.1;
            if (b.kind == BK_HALO || b.kind == BK_RING) R.spots = {{0, r + 2.5}, {0, -r - 2.5}};
            else if (b.kind == BK_PILLARS) R.spots = {{r + 2.5, -b.hd}, {-r - 2.5, -b.hd}};
            else R.spots = {{0, r + 2.5}, {r + 2.5, 0}, {0, -r - 2.5}};
            return true;
        }
        case BK_FOCUS: {   // at its foot (outside its pedestal, its tower, its pool's rim, its dome's ring)
            R.outside = true;
            const double r = b.style == FO_POOL ? std::max(b.hw, b.hd) + 1.4 : (b.style == FO_GATE ? 2.4 : (b.style == FO_DOME ? b.hw + 2.0 : b.hw * 1.15 + 1.6));
            if (b.style == FO_GATE) R.spots = {{0, r}, {0, -r}};
            else R.spots = {{0, r}, {r, 0}, {0, -r}};
            return true;
        }
        case BK_WINDMILL: R.outside = true; R.spots = {{0, b.hw + 1.6}, {b.hw + 1.6, 0}}; return true;
        case BK_AQUEDUCT: { R.outside = true; const double bay = 2 * b.hd / std::max(3, (int)std::lround(2 * b.hd / 7.0)); R.spots = {{0, -b.hd + bay * 0.5}, {0, -b.hd + bay * 1.5}}; return true; }   // under its arches
        case BK_TWIN: R.outside = true; R.spots = {{0, 0}, {0, b.hd + 2.0}}; return true;   // between its towers
        case BK_PORTAL: R.outside = true; R.spots = {{0, 2.6}, {1.2, 3.4}}; return true;   // in its mouth's cutting
        case BK_DOME: {   // D-05: the floor inside its ring (the far future's: at its foot)
            if (b.style == 2) { R.outside = true; R.spots = {{0, b.hw + 2.5}, {b.hw + 2.5, 0}}; return true; }
            const int segs = domeRingSegs(b.hw), ds = ringDoorSegs(b.hw, segs);
            roundRoom(b, R, b.hw, b.hw, segs, ringDoorFirst(b, segs, ds), ds); return true;
        }
        case BK_GREENHOUSE: rectRoom(b, R, 0, 0, b.hw, b.hd, 0, 0); R.spots = {{0, -b.hd * 0.6}, {0, 0}, {0, b.hd * 0.5}}; return true;   // along its middle walk
        // T-04: where the works keep a record: at the foot of a mast (a station's hut), a dish, a tomb, a barrow, a blockhouse, a hangar, a
        // lighthouse, a trilithon, a fort, outside each side
        case BK_MAST: case BK_DISH: case BK_TOMB: case BK_MOUND: case BK_BUNKER: case BK_HANGAR: case BK_LIGHTHOUSE: case BK_TRILITHON: case BK_FORT: {
            R.outside = true; R.hw = b.hw; R.hd = b.hd;
            const double m = b.kind == BK_MAST || b.kind == BK_LIGHTHOUSE ? 1.8 : 1.4;
            // B-421: off an earthwork's own banks: a heap, a long barrow or a bank beyond its length too (`genMound`'s hw by hd), a ring bank
            // inside its ring first and beyond its foot; a redoubt by its casemate inside its ring bank (the spots stood on the banks, under them)
            double hx = b.hw + m, hz = (b.kind == BK_DISH || (b.kind == BK_MOUND && (b.style == 0 || b.style == 2)) ? b.hw : b.hd) + m;
            if (b.kind == BK_FORT && b.style == 0) { hx = b.hw * 0.3 + m; hz = b.hw * 0.22 + m; }
            if (b.kind == BK_MOUND && b.style == 4) { const double o = b.hw + b.hd + m; R.spots = {{0, 0}, {0, b.hw * 0.4}, {0, o}, {o, 0}, {0, -o}, {-o, 0}}; return true; }
            R.spots = {{0, hz}, {hx, 0}, {0, -hz}, {-hx, 0}};
            return true;
        }
        default: return false;
    }
}

// ---- the people's palette and ways ---------------------------------------------------------------------------------------------

namespace {
float luma(const RGB& c) { return 0.3f * c.r + 0.55f * c.g + 0.15f * c.b; }
// B-415: the brick an industrial people fired, by its base colour (`GV_BASE`): its whites and creams a gault and a cream stock, its buffs,
// ochres and yellows the yellow stocks, its reds and rose the reds, its umber a brown, its greys a grey, a blue (the slate's), a dark
// engineering brick and a blue-grey, its sage an olive, its pale blue a pale grey, its violet a plum. Every industrial people built in brick
// (A-04's painted ones stood in their stucco, a white people's white: BUBAS read as modern)
const RGB BRICK[GENOME_BASES] = {
    RGB(0.74f, 0.66f, 0.52f), RGB(0.76f, 0.63f, 0.44f), RGB(0.71f, 0.55f, 0.37f), RGB(0.71f, 0.49f, 0.29f),
    RGB(0.75f, 0.61f, 0.35f), RGB(0.65f, 0.35f, 0.23f), RGB(0.56f, 0.24f, 0.19f), RGB(0.63f, 0.37f, 0.31f),
    RGB(0.46f, 0.31f, 0.22f), RGB(0.53f, 0.46f, 0.41f), RGB(0.38f, 0.37f, 0.42f), RGB(0.30f, 0.25f, 0.24f),
    RGB(0.44f, 0.43f, 0.48f), RGB(0.50f, 0.45f, 0.34f), RGB(0.60f, 0.57f, 0.55f), RGB(0.46f, 0.32f, 0.34f)};
}   // namespace

FormPalette culturePalette(const Culture& c, uint64_t seed) {
    FormPalette P;
    const Genome& G = c.genome;
    const RGB base = genomeBase(G), acc = genomeAccent(G), tint = genomeTint(G), white(1, 1, 1);
    const bool painted = G.of(GV_PAINTED) == 1;
    for (int k = 0; k < FORM_SLOTS; k++) { P.own[k] = false; P.tint[k] = 1; }
    P.glow = c.age >= AGE_FUTURISTIC;
    P.colour[1] = acc;
    P.colour[3] = RGB(0.30f, 0.23f, 0.17f);   // timber (and iron's rust) where there is no light
    // the windows: the people's openings (shape, rhythm, share of the wall) laid out by the age's bays and storeys. B-414: the slots are the
    // dwellings' (0), the public and the tall buildings' (1) and the people's own great buildings' (2: its temples, palaces and focal
    // accent). B-415: the age sets how much of a wall is window and how the windows go, the people's glazing and rhythm choose within it: a
    // medieval people's windows small (a third of a bay at the most, a pair or a group of lights in half of it); an industrial people's tall
    // sash windows one to a bay (a pair at the most, never scattered), round-headed where its openings were arched, round, pointed or
    // keyholed, its own shape on its churches as tall as theirs; a modern people's wide panes or bands, its own shape on its temples as
    // wide (an arched or a keyholed one a band, a triangle a slit). A-04's one rule for every age gave an industrial people of grouped wide
    // windows ribbons of glass (BUBAS read as modern) and a medieval one walls of glass
    static const int SHAPE[6] = {0, 1, 3, 2, 5, 6};
    const int shape = SHAPE[G.of(GV_OPENING)], rhythm = G.of(GV_RHYTHM), ex = G.of(GV_EXPRESSION);
    const int sash = shape == 1 || shape == 3 || shape == 5 || shape == 6 ? 1 : 0;
    const double share = G.at(GV_SHARE) * (ex == EX_MASSIVE ? 0.75 : (ex == EX_MONOLITH ? 0.7 : 1.0));
    const int group = rhythm == RH_PAIRED ? 2 : (rhythm == RH_GROUPED ? clampi(G.number(), 3, 4) : 1);
    const double scatter = rhythm == RH_SCATTERED ? 0.45 : 0;
    auto win = [&](double bay, double storey, double w, double h, double sill, int shp, double dark) {   // A-04's (the futuristic age's)
        WindowSpec W; W.bay = bay; W.storey = storey; W.sill = sill; W.shape = shp; W.dark = dark; W.group = shp == 4 ? 1 : group; W.scatter = shp == 4 ? 0 : scatter;
        if (shp == 2) { W.w = 0.08 + 0.08 * share; W.h = 0.45 + 0.3 * share; }   // slits
        else if (shp == 3) { const double d = (0.16 + 0.42 * share) * w; W.w = d; W.h = std::min(0.9, d * bay / storey); }   // round: as wide as high
        else { W.w = (0.14 + 0.5 * share) * w; W.h = (0.28 + 0.45 * share) * h; }
        if (W.group > 1) W.w = std::min(0.9, W.w * (1 + 0.45 * (W.group - 1)));   // a pair or a group spans more of its bay
        return W;
    };
    // B-415: a window of the age: its bay and storey (m), its width and height as shares of them (a group's lights together), its sill, its
    // shape, its lights, the share of the cells left blank, how dark
    auto spec = [](double bay, double storey, double w, double h, double sill, int shp, int lights, double blank, double dark) {
        WindowSpec W; W.bay = bay; W.storey = storey; W.w = w; W.h = h; W.sill = sill; W.shape = shp; W.group = shp == 4 ? 1 : lights; W.scatter = shp == 4 ? 0 : blank; W.dark = dark;
        if (shp == 3) W.h = std::min(0.9, W.w * bay / storey);   // round: as wide as high
        return W;
    };
    const double s = share;
    switch (c.age) {
        case AGE_PREHISTORIC:   // dry stone of the ground's rock (or daubed in the base), thatch and hide, the dark of timber and a post's hole
            P.own[0] = !painted; P.tint[0] = 0.25f; P.colour[0] = base;
            P.colour[2] = lerp(genomeRoofing(G, AGE_PREHISTORIC), base, 0.15f);   // R-412: its thatch, its reed, its heather, its turf, its hides
            P.colour[3] = RGB(0.22f, 0.17f, 0.13f);
            break;
        case AGE_MEDIEVAL: {   // C-01's walls of the rock (or adobe) tinted toward the base, or painted in it; the roofing; timber
            P.own[0] = !painted; P.tint[0] = 0.3f; P.colour[0] = base;
            P.colour[2] = genomeRoofing(G, AGE_MEDIEVAL);   // R-412: its thatch, tiles, shingles, slate or copper (A-04 drew it among three by the base)
            P.colour[3] = RGB(0.36f, 0.27f, 0.19f);
            auto small = [&](double bay, double storey, double w, double h, double sill, int shp) {
                const double gw = std::min(0.5, w * (1 + 0.35 * (group - 1)));
                if (shp == 2) return spec(bay, storey, 0.08 + 0.06 * s, 0.42 + 0.25 * s, sill, 2, 1, scatter, 20);
                if (shp == 3) return spec(bay, storey, 0.14 + 0.14 * s, 0, sill, 3, 1, scatter, 18);
                return spec(bay, storey, gw, h, sill, shp, group, scatter, 18);
            };
            P.windows[0] = small(3.4, 4.2, 0.12 + 0.16 * s, 0.24 + 0.2 * s, 0.3, shape);
            P.windows[1] = spec(2.6, 4.0, 0.08 + 0.08 * s, 0.45 + 0.3 * s, 0.25, 2, 1, scatter, 20);   // a keep's slits
            P.windows[2] = small(4.0, 6.0, 0.16 + 0.18 * s, 0.34 + 0.26 * s, 0.25, shape);
            P.nWindows = 3;
            break;
        }
        case AGE_INDUSTRIAL: {   // its brick (B-415: every industrial people's), slate, iron gone to rust
            P.colour[0] = BRICK[G.of(GV_BASE)];
            P.colour[2] = genomeRoofing(G, AGE_INDUSTRIAL);   // R-412: its slate, tiles or copper (A-04: slate or a brown sheet)
            P.colour[3] = RGB(0.34f, 0.25f, 0.20f);
            const int pair = group > 1 ? 2 : 1, lancets = std::min(group, 3);
            P.windows[0] = spec(2.8, 3.4, pair > 1 ? 0.46 + 0.1 * s : 0.3 + 0.12 * s, 0.5 + 0.16 * s, 0.27, sash, pair, 0, 18);
            P.windows[1] = spec(3.2, 4.4, 0.34 + 0.12 * s, 0.52 + 0.16 * s, 0.22, sash, 1, 0, 18);
            const double ow = shape == 3 ? 0.3 + 0.2 * s : (shape == 2 ? 0.1 + 0.06 * s : std::min(0.6, (0.22 + 0.12 * s) * (1 + 0.3 * (lancets - 1))));
            P.windows[2] = spec(3.4, 6.0, ow, 0.5 + 0.2 * s, 0.2, shape, shape == 2 || shape == 3 ? 1 : lancets, 0, 20);
            P.nWindows = 3;
            break;
        }
        case AGE_MODERN: {   // concrete toward the base (or rendered in it), the accent, steel and glass in the tint, rust
            P.colour[0] = painted ? base : lerp(RGB(0.64f, 0.63f, 0.60f), base, 0.3f);
            P.colour[2] = tint;
            P.colour[3] = RGB(0.36f, 0.28f, 0.22f);
            const bool bands = c.technique == TQ_FRAME || s > 0.45;
            const int lights = group > 1 ? 2 : 1;
            P.windows[0] = bands ? spec(4.0, 3.1, 1.0, 0.3 + 0.32 * s, 0.35, 4, 1, 0, 20) : spec(3.0, 3.1, std::min(0.85, (0.52 + 0.24 * s) * (lights > 1 ? 1.25 : 1.0)), 0.46 + 0.24 * s, 0.28, 0, lights, 0, 18);
            P.windows[1] = s > 0.3 ? spec(3.2, 3.6, 1.0, 0.52 + 0.38 * s, 0.1, 4, 1, 0, 16) : spec(2.8, 3.6, 0.6 + 0.2 * s, 0.52 + 0.2 * s, 0.2, 0, 1, 0, 16);
            static const int MSHAPE[6] = {0, 4, 3, 2, 2, 4};   // its own on its temples: a square pane, a band, a porthole, a slit
            const int ms = MSHAPE[G.of(GV_OPENING)];
            P.windows[2] = ms == 4 ? spec(3.6, 4.0, 1.0, 0.4 + 0.35 * s, 0.2, 4, 1, 0, 18)
                         : (ms == 3 ? spec(3.6, 4.0, 0.32 + 0.2 * s, 0, 0.2, 3, 1, 0, 18)
                         : (ms == 2 ? spec(3.6, 4.0, 0.12 + 0.1 * s, 0.7 + 0.2 * s, 0.15, 2, 1, 0, 18) : spec(3.6, 4.0, 0.5 + 0.3 * s, 0.55 + 0.3 * s, 0.2, 0, 1, 0, 18)));
            P.nWindows = 3;
            break;
        }
        case AGE_FUTURISTIC:   // composite in a light of the base, the accent, diamond glass in the tint, the light
            P.colour[0] = lerp(base, white, 0.4f);
            P.colour[2] = lerp(tint, white, 0.25f);
            P.colour[3] = genomeLight(G);
            P.windows[0] = win(6.0, 5.0, 1.0, 1.0, 0.4, 4, 14); P.windows[0].w = 1.0; P.windows[0].h = 0.15 + 0.25 * G.at(GV_SHARE);
            P.windows[1] = win(5.0, 5.0, 1.0, 1.0, 0.3, shape == 1 || shape == 6 ? 3 : shape, 14); P.nWindows = 2;   // B-415: an arched or a keyholed people's portholes
            break;
        default: {   // the far future: its one material at the base's extreme, its accent, the black of a shaft's mouth, its light up it
            const float l = luma(base);
            P.colour[0] = l < 0.45f ? base * 0.3f : lerp(base, white, 0.55f);
            P.colour[2] = RGB(0.01f, 0.01f, 0.012f);
            P.colour[3] = genomeLight(G);
            const int lt = G.of(GV_LIGHT);
            if (lt != LT_NONE) {
                WindowSpec W; W.bay = lt == LT_LINES ? 12 : 9; W.storey = lt == LT_LINES ? 30 : 14; W.shape = lt == LT_LINES ? 4 : 3; W.w = lt == LT_LINES ? 1.0 : 0.22; W.h = lt == LT_LINES ? 0.07 : 0.22 * 9 / 14.0; W.sill = 0.5;
                W.lit = true; W.dark = 0; W.scatter = lt == LT_LAMPS ? 0.55 : 0; P.windows[0] = W; P.nWindows = 1;
            }
            break;
        }
    }
    // R-412: the people's dress on its walls, read through the age: the windows' surrounds lighter or darker than the wall, or shutters beside
    // them (the hand-made and the industrial ages' and a modern people's: `GV_TRIM`); its walls of one colour, a dado of another under the
    // first storey's windows (a medieval people's plinth, an industrial people's stucco ground floor, a modern people's base), courses of two
    // colours (a medieval people's striped stone, an industrial people's polychrome brick, a modern or a futuristic people's bands), or each
    // house its own (`GV_POLYCHROMY`: `Gen::tone`)
    if (c.age >= AGE_MEDIEVAL && c.age <= AGE_MODERN) {
        const int tm = G.of(GV_TRIM);
        const double fr = c.age == AGE_MEDIEVAL ? 0.05 : (c.age == AGE_INDUSTRIAL ? 0.06 : 0.035);
        for (int k = 0; k < P.nWindows; k++) {
            WindowSpec& W = P.windows[k];
            if (W.shape == 4 && tm == TM_SHUTTERS) { W.frame = fr * 0.6; W.frameShade = -8; continue; }   // a band of glass: its mullions' frame
            if (tm == TM_LIGHT || tm == TM_DARK) { W.frame = fr; W.frameShade = tm == TM_LIGHT ? 9 : -9; }
            else if (tm == TM_SHUTTERS && W.shape != 2) { W.shutter = std::min(0.5 * W.w, 0.5 * (1 - W.w) - 0.02); W.shutterShade = c.age == AGE_MODERN ? 6 : -8; }
        }
    }
    if (c.age >= AGE_MEDIEVAL && c.age <= AGE_FUTURISTIC) {
        const int pc = G.of(GV_POLYCHROMY);
        for (int k = 0; k < P.nWindows && k < 2; k++) {
            WindowSpec& W = P.windows[k];
            if (pc == PC_TWO_TONE) {
                switch (c.age) {   // darker under (a lighter band blew out on a sunlit face)
                    case AGE_MEDIEVAL: W.dado = 0.9; W.dadoShade = -7; break;
                    case AGE_INDUSTRIAL: W.dado = W.storey * 0.92; W.dadoShade = -8; break;
                    case AGE_MODERN: W.dado = W.storey; W.dadoShade = -6; break;
                    default: W.dado = W.storey * 0.5; W.dadoShade = -6; break;
                }
            } else if (pc == PC_STRIPED) {
                W.stripe = c.age == AGE_MEDIEVAL ? 0.5 : (c.age == AGE_INDUSTRIAL ? 0.45 : W.storey * 0.5);
                W.stripeShade = -7;
            }
        }
    }
    return P;
}

void cultureWays(Culture& c, uint64_t peopleSeed) {
    const Genome& G = c.genome;
    c.technique = techniqueOf(G, c.age, peopleSeed);
    c.tall = 0.75 + 0.55 * G.at(GV_PROPORTION);
    c.towers = G.at(GV_HEIGHT) > 0.2;
    c.style = G.of(GV_EXPRESSION) == EX_MASSIVE ? 1 : 0;
    c.life = std::pow(10.0, 0.6 * (G.at(GV_LASTING) - 0.5));
    c.roof = G.of(GV_ROOF);
    c.stoneRoofs = c.roof == RF_FLAT || c.roof == RF_VAULTED || c.roof == RF_DOMED || c.roof == RF_STEPPED || c.roof == RF_OPEN;
    switch (G.of(GV_STREETS)) {
        case ST_GRID: c.plan = SP_GRID; break;
        case ST_RADIAL: case ST_CONCENTRIC: c.plan = SP_RADIAL; break;
        case ST_ORGANIC: c.plan = SP_CLUSTER; break;
        default: c.plan = SP_STREET; break;   // along a line, on the contour, branching
    }
    const int focus = G.of(GV_FOCUS), centre = G.of(GV_CENTRE), plan = G.of(GV_PLAN);
    if (centre == CE_TOMB) c.temple = 2;
    else if (focus == FO_DOME || c.roof == RF_DOMED) c.temple = 1;
    else if (focus == FO_SPIRE || focus == FO_TOWER) c.temple = 0;
    else if (plan == PF_CIRCLE || plan == PF_POLYGON || plan == PF_ELLIPSE) c.temple = 3;
    else c.temple = c.roof == RF_STEPPED ? 2 : (c.roof == RF_CONICAL ? 3 : 0);
    c.hut = G.at(GV_LASTING) > 0.55 || (c.family == 0 && G.of(GV_EXPRESSION) == EX_MASSIVE) ? 1 : 0;
    switch (c.technique) {
        case TQ_CANTILEVER: c.towerForm = 4; break;
        case TQ_SHELL: c.towerForm = 5; break;
        default:
            c.towerForm = G.of(GV_MASSING) == MS_STEPPED ? 1 : (plan == PF_CIRCLE || plan == PF_ELLIPSE ? 2 : (plan == PF_POLYGON ? 3 : 0));
            break;
    }
    if (c.age == AGE_FAR_FUTURE) {
        switch (focus) {
            case FO_SPIRE: case FO_MAST: case FO_OBELISK: c.great = 0; break;
            case FO_RING: case FO_GATE: c.great = 1; break;
            case FO_TOWER: c.great = 2; break;
            case FO_POOL: c.great = 3; break;
            default: c.great = 4; break;
        }
    } else c.great = c.technique == TQ_PRINTED ? 0 : (c.technique == TQ_GROWN ? 2 : 1);   // the arcology
    c.palette = culturePalette(c, peopleSeed);
}
