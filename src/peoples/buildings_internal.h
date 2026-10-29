#pragma once
// E-09: what buildings.cpp's parts share, for them alone (buildings, buildings_frame, buildings_dress, buildings_medieval,
// buildings_ages, buildings_works): the generators' frame (`Gen`), the walls', rings' and towers' specs, the small helpers every
// age calls (inline, as they were in their one file), and each part's functions the others call, declared at the end
#include "buildings.h"
#include "peoples.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>
#include <initializer_list>

inline double u01(uint64_t seed, int k) { return unitFromHash(mix64(seed ^ (0x9E3779B97F4A7C15ULL * (uint64_t)(k + 1)))); }   // C-01's building draws
inline uint32_t pieceSeed(uint64_t seed, int k) { return (uint32_t)mix64(seed ^ (0xA03B0000ULL + (uint64_t)(k + 1) * 0xD1B54A32D192ED03ULL)); }

// the share of a low wall still standing (C-01's: some stand nearly whole, others to their foot) and of a tall thing (a tower, a
// chimney, a mast, a wall over 8 m): whole through its first decay, then falling, to a stump
inline double wallShare(double D, double u) { return 1 - D * (0.15 + 0.85 * u); }
inline double tallShare(double D, double u) { double d = clampd((D - 0.1 - 0.1 * u) / 0.55, 0, 1); return clampd(std::pow(1 - d, 1.6), 0.05, 1); }

// the doorway's place along its wall (C-01's draw: a house's, a hall's, a tower's) and its midpoint in the building's frame (the walls'
// axes run +x' on the front and the back and -z' on the right and the left, so the gap at `pos` along the axis sits at x' = pos on
// sides 0 and 2 and at z' = -pos on sides 1 and 3: C-08's correction)
inline void doorMid(int side, double pos, double hw, double hd, double& lx, double& lz) {
    switch (side) { case 0: lx = pos; lz = hd; break; case 2: lx = pos; lz = -hd; break; case 1: lx = hw; lz = -pos; break; default: lx = -hw; lz = -pos; break; }
}
// a house's doorway along its wall: C-01's draw, or the middle (A-04: `Building::centred`)
inline double houseDoorPos(const Building& b, double doorL) { return b.centred ? 0.0 : (doorL - 1.6) * (u01(b.seed, 20) - 0.5) * 1.2; }
// B-414: a mill's chimney's radius (its generator's draw), and where its floor ends before the chimney in its back end and the heap at its foot
inline double millChimneyR(const Building& b) { return std::min(2.4 + 0.8 * u01(b.seed, 32), 0.3 * b.hw); }
inline double millFloorBack(const Building& b) { return std::min(b.hd - 2.0, -b.hd + 3 * millChimneyR(b) + 1.4); }

// what a piece is built of that holds its shape (the structure's slot) and what is sheet, timber or metal (the roofing's)
inline bool structural(int m) { return m >= 0 && m != MT_TIMBER && m != MT_IRON && m != MT_GLASS; }

// a ring's segments and the doorway's: as many segments left out as make a gap of two metres
inline int ringDoorSegs(double r, int segs) { int n = 1; while (n < segs / 3 && 2 * r * std::sin(PI * n / segs) < 2.0) n++; return n; }
// a ring's segments by the plan's form: a circle twelve (fewer when small), a polygon its sides, an ellipse fourteen
inline int ringSegs(int form, int sides, double r) { return form == PF_POLYGON ? clampi(sides, 3, 9) : (form == PF_ELLIPSE ? 14 : (r < 2.6 ? 10 : 12)); }

// B-415: the people's own great buildings (its temples, churches, town halls, palaces, keeps and towers, its focal accent, monuments,
// gates, platforms, rotundas, joined towers, aqueducts and tunnels), where its genome speaks fully in every age; the rest are everyday
// buildings (its houses, rows, tenements, villas, blocks, halls, mills and works), where the age speaks first
// the people's dwellings (its houses, rows, tenements, villas, blocks, suburban houses, rotundas and pods), where its own colours and
// dress show from house to house
inline bool dwellingKind(int kind) {
    return kind == BK_HOUSE || kind == BK_ROW || kind == BK_TENEMENT || kind == BK_VILLA || kind == BK_BLOCK || kind == BK_SUBURB || kind == BK_ROTUNDA || kind == BK_POD;
}
inline bool greatKind(const Building& b) {
    switch (b.kind) {
        case BK_TEMPLE: case BK_CHURCH: case BK_TOWN_HALL: case BK_FOCUS: case BK_STELA: case BK_TOWER: case BK_WALL_TOWER: case BK_GATE:
        case BK_PLATFORM: case BK_ROTUNDA: case BK_TWIN: case BK_AQUEDUCT: case BK_PORTAL: case BK_OBSERVATORY: case BK_WINDMILL: return true;
        case BK_HALL: return b.style == 2;   // the palace
        default: return false;
    }
}
struct Gen {
    const Building& b; const Culture& c; const SiteRead& st; int lod; std::vector<Form>& out; int bi;
    double sx, sz, fx, fz, sink = 0;
    int pieces = 0;
    bool great;   // B-415: one of the people's great buildings (`greatKind`)
    // a dwelling of a people that painted each house its own (`GV_POLYCHROMY`): its walls' shades over the slot's, and its walls in the
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
    // the genome; B-415: as the age built it (`ageExpressed`)
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
    // the people's pitch over its roofs' (the hand-made and the industrial ages'), its eaves' overhang on a pitched, hipped or conical roof
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
    Form& lathe(int sub, double lx, double lz, double r, double y0, double y1, int slot, int mat, int sides = 0) {
        Form& e = add(FK_LATHE, sub, lx, lz, 0, r, r, y0, y1, slot, mat);
        e.sides = sides;
        return e;
    }
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
inline int archSub(const Gen& g) {
    const int op = g.gv(GV_OPENING);
    if (g.c.age >= AGE_MODERN) return op == OP_ARCHED || op == OP_ROUND || op == OP_KEYHOLE ? 0 : 2;
    if (g.c.age == AGE_INDUSTRIAL) return op == OP_SQUARE || op == OP_SLIT ? 2 : 0;
    return op == OP_TRIANGULAR ? 1 : (op == OP_SQUARE || op == OP_SLIT ? 2 : 0);
}
// B-415: a dome's profile (`FK_DOME`'s sub: 0 a hemisphere, 1 a saucer, 2 pointed, 3 an onion) as its age built it: an industrial people's a
// hemisphere where its people's was pointed or an onion, a modern people's a saucer of concrete (the later ages' their own)
inline int ageDome(const Gen& g, int prof) {
    if (g.c.age == AGE_MODERN) return 1;
    if (g.c.age == AGE_INDUSTRIAL && prof >= 2) return 0;
    return prof;
}

// four walls round a room: side 0 the front (+z'), 1 the right (+x'), 2 the back (-z'), 3 the left (-x'), each standing to its share of
// `h` by the material's decay (a wall over 8 m by the tall share, its top ragged); the doorway (1.8 m: a lintel over it and, where the
// wall is tall, the wall over the lintel) in the side `door` at `doorPos` along its axis; C-01's window in the wall opposite; the
// windows' pattern on them; at level 1 the walls whole, at level 2 one box. The heights each wall stands to are in `hs`.
// the people's profile (battered walls leaning in on a thicker foot, bulging ones with their rounded buttresses, an upper storey
// jettied out or set back over 4.5 m, a low wall's plinth) and its dress (`dress`: the bands, the cornice, the expressed frame, the
// door's surround, the glyphs round every side of a carved people's, the crenels along the tops)
struct Walls {
    double cx = 0, cz = 0, hw = 3, hd = 3, h = 3, thick = 0.5, y0 = 0;
    int door = 0; double doorPos = 0; bool window = false, glyphs = false;
    int mat = MT_MASONRY, slot = 0, pattern = 0, part = 0, k0 = 10;
    bool whole = false;   // standing whole whatever the decay (a storey that holds up what stands on it)
    int profile = -1;     // `WallProfile`, -1 the people's (`dress` false: upright)
    bool dress = true;    // the people's dress on them (a work's walls go bare)
    bool crenels = false; // merlons along the tops of the walls that stand whole (a flat roof's or a keep's)
    double parapet = 0;   // a parapet this high along the tops of the walls that stand whole (an industrial people's low roof behind it)
};
struct WallsOut { double hmin = 0, thick = 0.5, top = 0; };   // the lowest wall, the walls' thickness as built

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
inline bool ringInDoor(const Ring& R, int k) { if (R.open) return false; int d = ((k - R.door) % R.segs + R.segs) % R.segs; return d < R.doorSegs; }
// the stone or the roofing a people roofs with in an age: stone where the form holds itself up (a slab, a vault, a dome, a stepped
// crown) and the people builds to last or the age has no timber left to roof with; the roofing's sheet otherwise
inline int roofMaterial(const Gen& g, int form, int stone) {
    const bool selfHolding = form == RF_FLAT || form == RF_VAULTED || form == RF_DOMED || form == RF_STEPPED || form == RF_OPEN;
    switch (g.c.age) {
        case AGE_PREHISTORIC: return form == RF_DOMED ? MT_DRYSTONE : MT_TIMBER;
        case AGE_MEDIEVAL: return selfHolding ? stone : MT_TIMBER;
        case AGE_INDUSTRIAL: return selfHolding ? stone : MT_TIMBER;
        case AGE_MODERN: return selfHolding || form == RF_SADDLE ? MT_CONCRETE : MT_IRON;
        default: return MT_COMPOSITE;
    }
}

// A-04: a tower's shaft of radius `r` to `H` at (tx, tz) in the people's section (round, its polygon, square, oval) and profile
// (battered: a frustum; bulging: a barrel; leaning out: widening to a corbelled top; stepped back: tiers), snapped by the material's
// tall share, its top lying where it fell; its windows `pattern`. Returns the height it stands to (`whole` with it standing whole)
struct TowerOut { double h = 0; bool whole = false; double rTop = 0; };

// the walls' material of a people that built in stone (the rock, or the adobe of a sand family); A-04: a people that built light builds
// its houses of timber, one of expressed frames of daub between its posts
inline int stoneOf(const Culture& c) { return c.family == 1 ? MT_EARTH : MT_MASONRY; }
inline int houseMaterial(const Culture& c) {
    if (c.genome.at(GV_LASTING) < 0.2) return MT_TIMBER;
    if (c.genome.of(GV_EXPRESSION) == EX_FRAME) return MT_EARTH;
    return stoneOf(c);
}
// the slot of a material's walls: timber's own (dark) where the walls are of it, the structure's otherwise
inline int wallSlot(const Gen& g, int mat) { return mat == MT_TIMBER ? g.timberSlot() : 0; }
// the material of the works every age built the same way (C-08's features, the platform, a stela, the town wall, A-04's focal accent):
// its stone, then brick, mass concrete, composite
inline int worksOf(const Building& b, const Culture& c) {
    switch (b.age) {
        case AGE_PREHISTORIC: return MT_DRYSTONE;
        case AGE_MODERN: return MT_MASS;
        case AGE_FUTURISTIC: return MT_COMPOSITE;
        case AGE_FAR_FUTURE: return MT_ETERNAL;
        default: return stoneOf(c);
    }
}
// the structure of a great building of its age: stone, brick, concrete, composite
inline int greatOf(const Building& b, const Culture& c) {
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
inline int houseRoof(const Gen& g) {
    const double var = g.gf(GV_SAMENESS);
    if (var > 0.55 && g.U(41) < (var - 0.55) * 1.6) { static const int ALT[4] = {RF_PITCHED, RF_HIPPED, RF_FLAT, RF_CONICAL}; return ALT[std::min(3, (int)(g.U(42) * 4))]; }
    return g.c.roof;
}
// B-414: the roof an industrial or a modern people put on its everyday buildings (its rows, tenements, villas, mills, halls and blocks);
// the people's roof stays on its great buildings. B-415: the age's alone: an industrial people's slate on timber, pitched (a people of
// gables, steps, vaults or saddles) or hipped (the rest; a people of flat roofs hid it behind a parapet: `parapetRoof`), a modern people's
// flat (B-414 kept a modern people's pitched, hipped and saddle roofs and an industrial people's flat ones: BUBAS's flat-roofed rows read as
// modern, a modern suburb of gables as industrial)
inline int ageRoof(const Culture& c, int rf) {
    if (c.age == AGE_INDUSTRIAL) return rf == RF_PITCHED || rf == RF_STEPPED || rf == RF_VAULTED || rf == RF_SADDLE ? RF_PITCHED : RF_HIPPED;
    if (c.age == AGE_MODERN) return RF_FLAT;
    return rf;
}
// B-415: an industrial people of flat roofs or open tops: its everyday buildings' low hipped roofs behind a parapet
inline bool parapetRoof(const Culture& c) { return c.age == AGE_INDUSTRIAL && (c.roof == RF_FLAT || c.roof == RF_OPEN); }
// a round temple's ring: its people's polygon (doubled where it has few sides) or sixteen
inline int templeRingSegs(const Building& b) { return b.form == PF_POLYGON ? clampi(b.sides, 3, 9) * (b.sides <= 4 ? 2 : 1) : 16; }
// the first segment of a ring's doorway: the building's draw (C-01's rotunda), or round the front for a centred one
inline int ringDoorFirst(const Building& b, int segs, int doorSegs) { return b.centred ? ((segs - doorSegs / 2) % segs + segs) % segs : (int)(u01(b.seed, 5) * segs); }
// the slot the walls are drawn in (`walls4x`: a house painted in the accent)
inline int wallsSlot(const Gen& g, const Walls& W) { return W.slot == 0 && g.painted ? 1 : W.slot; }
// a hut's ring: its people's polygon or oval, a circle of ten otherwise; its doorway two segments (C-01's draw, or the front's middle)
inline int hutSegs(const Building& b) { return b.form == PF_POLYGON ? clampi(b.sides, 5, 9) : (b.form == PF_ELLIPSE ? 12 : 10); }
inline int hutDoorSegs(const Building& b) { return std::max(2, ringDoorSegs(std::min(b.hw, b.form == PF_ELLIPSE ? b.hd : b.hw), hutSegs(b))); }
inline int hutDoorSeg(const Building& b) { return ringDoorFirst(b, hutSegs(b), hutDoorSegs(b)); }   // the first of the segments left out for the doorway
// the walls of the industrial age: brick or the rock (the palette says which colour: A-04 the people's brick, or its stucco), the
// people's windows, a roof of slate on timber
inline int brickOf(const Culture& c) { return c.family == 1 && c.age <= AGE_MEDIEVAL ? MT_EARTH : MT_MASONRY; }
// the doorway's place on a front of `L` half length (a row's, a block's): off the middle by the building's draw, or the middle
inline double frontDoorPos(const Building& b, double L) { return b.centred ? 0.0 : (L - 1.6) * (u01(b.seed, 20) - 0.5) * 1.0; }
// a gate hall at the foot of a great structure: a parabolic vault open at both ends, its axis along the building's heading, set
// before the structure's front at `z0` (the records' room of an age that built without rooms of its own on the ground)
constexpr double PORTAL_HW = 7, PORTAL_HD = 13, PORTAL_H = 12;
// the front of a great structure of radius `r` and the portal before it
inline double portalZ(double r) { return r + 2; }

// ---- each part's functions the others call, defined once in it

// the frame (buildings_frame.cpp)
void figure(Gen& g, double lx, double lz, double y0, double h, int slot, int mat, double D);
void topsAt(Gen& g, const std::vector<std::pair<double, double>>& pts, double y, double scale, int mat);
double walls4(Gen& g, const Walls& W, double hs[4]);
double ringWalls(Gen& g, const Ring& R0);
double roof(Gen& g, int form, double cx, double cz, double hw, double hd, double hmin, double h, double pitch, int mat);
void gables(Gen& g, const Walls& W, const double hs[4], double pitch, bool across = false);
double roundRoof(Gen& g, int form, double cx, double cz, double rx, double rz, int segs, double hmin, double h, double pitch, int mat);
TowerOut towerShaft(Gen& g, double tx, double tz, double r, double H, int mat, int k, int pattern, bool fallTop = true);
void towerTop(Gen& g, double tx, double tz, double r, double h, double H, int mat, bool spire, double spireH);
void foundation(Gen& g, double rx, double rz, int doorSide, double doorPos, int mat);
bool stilts(Gen& g, double rx, double rz, double fy);
void drifts(Gen& g, double hw, double hd, double D);
void rubble(Gen& g, double hw, double hd, const double hs[4], double h, double doorX, double doorZ, int slot, int mat);
// the dress (buildings_dress.cpp)
double wallsHalf(const Gen& g, const Walls& W);
void entranceAt(Gen& g, const Walls& W, double ht, double H);
void cornersOf(Gen& g, const Walls& W, const double hs[4], double ht, int roofMat);
void rooflineOf(Gen& g, const Walls& W, double hmin, int rf, double rise, int roofMat);
void ringDress(Gen& g, const Ring& R, double hmin);
void houseDress(Gen& g, const Walls& W, const double hs[4], double hmin, int rf, double rise, int roofMat, bool roofline);
void yardOf(Gen& g, double hw, double hd, int doorSide, double doorPos, int mat);
void hutPole(Gen& g, double px, double pz);
void hutRing(Gen& g, double rx, double rz, double doorA);
void hutEntrance(Gen& g, double rx, double rz, double doorA, double h, bool stone);
// the medieval age (buildings_medieval.cpp)
void houseTops(Gen& g, int rf, double cx, double cz, double hw, double hd, double y, double ridge, int roofMat, int wallMat);
void roundTops(Gen& g, int rf, double y, double rise, double scale, int roofMat);
double rectBody(Gen& g, Walls W, int rf, int roofMat, double hs[4], bool cells);
void motifOn(Gen& g, double cx, double cz, double hw, double hd, double h, double H, int mat);
// the other ages (buildings_ages.cpp)
void standingStone(Gen& g, double lx, double lz, double dh, double hx, double hz, double h, int k, bool tapered);
void stack(Gen& g, double x, double z, double hx, double hz, double y0, double y1, int mat);
// the works (buildings_works.cpp)
int domeRingSegs(double r);

// the generators, each a building kind's, that `buildingForms` (buildings.cpp) calls, by part
void genFurniture(Gen& g);
void genHouse(Gen& g); void genRotunda(Gen& g); void genGate(Gen& g); void genWallTower(Gen& g); void genPlatform(Gen& g);
void genColonnade(Gen& g); void genStela(Gen& g); void genRubble(Gen& g); void genWall(Gen& g); void genFeature(Gen& g);
void genTemple(Gen& g); void genWindmill(Gen& g);
void genHut(Gen& g); void genLonghouse(Gen& g); void genTentRing(Gen& g); void genStoneCircle(Gen& g); void genMenhir(Gen& g);
void genDolmen(Gen& g); void genPalisade(Gen& g); void genRow(Gen& g); void genTenement(Gen& g); void genVilla(Gen& g);
void genChurch(Gen& g); void genTownHall(Gen& g); void genStation(Gen& g); void genMill(Gen& g); void genGasometer(Gen& g);
void genWaterTower(Gen& g); void genHeadframe(Gen& g); void genObservatory(Gen& g); void genBlock(Gen& g); void genSuburb(Gen& g);
void genHighrise(Gen& g); void genStadium(Gen& g); void genMast(Gen& g); void genPower(Gen& g); void genTanks(Gen& g);
void genArcology(Gen& g); void genHabitat(Gen& g); void genSpire(Gen& g); void genRing(Gen& g); void genMaglev(Gen& g);
void genWreck(Gen& g); void genFar(Gen& g);
void genFocus(Gen& g); void genAqueduct(Gen& g); void genTwin(Gen& g); void genPortal(Gen& g); void genCrane(Gen& g);
void genContainers(Gen& g); void genReservoir(Gen& g); void genMound(Gen& g); void genDam(Gen& g); void genDish(Gen& g);
void genTomb(Gen& g); void genFort(Gen& g); void genBunker(Gen& g); void genTeeth(Gen& g); void genVehicle(Gen& g);
void genAircraft(Gen& g); void genShip(Gen& g); void genLaunchpad(Gen& g); void genHangar(Gen& g); void genColossus(Gen& g);
void genTurbine(Gen& g); void genLighthouse(Gen& g); void genLattice(Gen& g); void genDerrick(Gen& g); void genBlocks(Gen& g);
void genSphere(Gen& g); void genTether(Gen& g); void genTrilithon(Gen& g); void genConveyor(Gen& g); void genDome(Gen& g);
void genGreenhouse(Gen& g);
