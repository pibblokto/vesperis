// A-03: what each age builds (see buildings.h). C-01's ten kinds and C-08's features are the medieval age's (and the features
// every age's that built them), drawn as they were but for their materials' decay and their roofs; every other kind is new. Each
// generator reads its building's seed (`U`), its materials' decay (`D`, `Dk` for a piece of it), and its people's ways of building,
// and writes A-02's forms in the building's frame: x' across its heading, z' along it (the front, side 0, faces +z').
// the people's ways are its genome's (peoples/genome.h) read through its age (`cultureWays`): the plan's form and the roof, the
// walls' profile and their dress (bands, a cornice, an expressed frame, the door's surround, the glyphs), the tops, the towers'
// sections, the motif on the great buildings, the focal accent, the colours (`culturePalette`); the slots by what a piece is
// (`Gen::roofSlot` and the rest: the accent where the people put it).
#include "buildings.h"
#include "buildings_internal.h"

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
    const double d = materialDecay(m, c.ageYears / std::max(0.1, c.life), c.wet);   // the years over its people's lifetimes (built light, built to last)
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
        case BK_CRANE: case BK_RESERVOIR: return -1;   // an industrial people's and a modern people's in their ways
        case BK_CONTAINERS: return AGE_MODERN;
        // the works' kinds: earth, a dam, a tomb, a fort, a ship, a colossus, a lighthouse, a derrick, cut blocks and a conveyor in each
        // age's way; the machines' the modern age's, the tether the futuristic's, the lattice and the sphere the far future's, a trilithon the
        // old time's
        case BK_MOUND: case BK_DAM: case BK_TOMB: case BK_FORT: case BK_SHIP: case BK_COLOSSUS: case BK_LIGHTHOUSE: case BK_DERRICK: case BK_BLOCKS:
        case BK_CONVEYOR: case BK_HANGAR: case BK_FURNITURE: return -1;
        case BK_DOME: case BK_GREENHOUSE: return -1;   // in the way of the age that lost its sun
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
        case BK_TOMB: return "A TOMB";
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
        case BK_DOME: return "A DOME";
        case BK_GREENHOUSE: return "A GLASSHOUSE";
        default: return "A RUIN";
    }
}

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
        case BK_CRANE: genCrane(g); break;
        case BK_CONTAINERS: genContainers(g); break;
        case BK_RESERVOIR: genReservoir(g); break;
        case BK_MOUND: genMound(g); break;
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
        case BK_FURNITURE: genFurniture(g); break;
        case BK_DOME: genDome(g); break;
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
        case BK_MILL: {   // its doorway in its front, its floor to the chimney's heap in its back end
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
        case BK_DOME: {   // the floor inside its ring (the far future's: at its foot)
            if (b.style == 2) { R.outside = true; R.spots = {{0, b.hw + 2.5}, {b.hw + 2.5, 0}}; return true; }
            const int segs = domeRingSegs(b.hw), ds = ringDoorSegs(b.hw, segs);
            roundRoom(b, R, b.hw, b.hw, segs, ringDoorFirst(b, segs, ds), ds); return true;
        }
        case BK_GREENHOUSE: rectRoom(b, R, 0, 0, b.hw, b.hd, 0, 0); R.spots = {{0, -b.hd * 0.6}, {0, 0}, {0, b.hd * 0.5}}; return true;   // along its middle walk
        // where the works keep a record: at the foot of a mast (a station's hut), a dish, a tomb, a barrow, a blockhouse, a hangar, a
        // lighthouse, a trilithon, a fort, outside each side
        case BK_MAST: case BK_DISH: case BK_TOMB: case BK_MOUND: case BK_BUNKER: case BK_HANGAR: case BK_LIGHTHOUSE: case BK_TRILITHON: case BK_FORT: {
            R.outside = true; R.hw = b.hw; R.hd = b.hd;
            const double m = b.kind == BK_MAST || b.kind == BK_LIGHTHOUSE ? 1.8 : 1.4;
            // off an earthwork's own banks: a heap, a long barrow or a bank beyond its length too (`genMound`'s hw by hd), a ring bank
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
    // a window of the age: its bay and storey (m), its width and height as shares of them (a group's lights together), its sill, its
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
            P.colour[2] = lerp(genomeRoofing(G, AGE_PREHISTORIC), base, 0.15f);   // its thatch, its reed, its heather, its turf, its hides
            P.colour[3] = RGB(0.22f, 0.17f, 0.13f);
            break;
        case AGE_MEDIEVAL: {   // C-01's walls of the rock (or adobe) tinted toward the base, or painted in it; the roofing; timber
            P.own[0] = !painted; P.tint[0] = 0.3f; P.colour[0] = base;
            P.colour[2] = genomeRoofing(G, AGE_MEDIEVAL);   // its thatch, tiles, shingles, slate or copper (A-04 drew it among three by the base)
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
            P.colour[2] = genomeRoofing(G, AGE_INDUSTRIAL);   // its slate, tiles or copper (A-04: slate or a brown sheet)
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
            P.windows[1] = win(5.0, 5.0, 1.0, 1.0, 0.3, shape == 1 || shape == 6 ? 3 : shape, 14); P.nWindows = 2;   // an arched or a keyholed people's portholes
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
    // the people's dress on its walls, read through the age: the windows' surrounds lighter or darker than the wall, or shutters beside
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
