// I-02: the Stardrifter's rows (ship_plan.h): three decks and eight spaces (R-426, IDEAS-small-and-spacious.md section 2.2), the ways
// between them, their windows, what stands in them, the lamps and the places; the mesh built once from them; the walk they keep, the
// routes through them, what the eye sees of them, the banks a frame deals
#include "ship_plan.h"
#include "game_types.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace {
constexpr double WR = SHIP_WALKER;
constexpr double STAIR_OFF = 1.7;   // the stair's radius where a landing's way off it begins (between the steps' 1.6 and the well's 1.8)
constexpr int ROUND = 16;           // a circle's segments: the well, the oculus, the column, the dome's meridians
constexpr int HEAD = 8;             // an arch's round head's segments

// ---- the finishes (I-03) ------------------------------------------------------------------------------------------------------------

// the hull's grey (bank 8, fixed: the old cabin's), warm panels, a darker floor, fabric, brass, plants, glass, the hull outside (its ramp
// toward the star's colour); each but the hull's dealt a bank by its home (`ShipBanks`: a key, 11-19 the space view's numbers). I-04: the
// photographs' one tone (a print's sepia), the garden's second green (the plants' two colours the home world's flora's, given by the game)
constexpr ShipFinishRow FINISHES[SHIP_FINISHES] = {
    {"HULL", BANK_HULL, RGB(0.42f, 0.44f, 0.5f), 12, 0.6},     {"PANEL", 11, RGB(0.68f, 0.5f, 0.34f), 12, 0.45},
    {"DECK", 12, RGB(0.3f, 0.27f, 0.26f), 8, 0.7},             {"FABRIC", 13, RGB(0.54f, 0.22f, 0.25f), 30, 0.5},
    {"BRASS", 14, RGB(0.88f, 0.67f, 0.32f), 0, 0},             {"PLANT", 15, RGB(0.3f, 0.56f, 0.24f), 5, 0.9},
    {"GLASS", 16, RGB(0.56f, 0.74f, 0.8f), 0, 0},              {"OUTSIDE", 17, RGB(0.66f, 0.66f, 0.68f), 12, 0.5},
    {"PRINT", 18, RGB(0.7f, 0.58f, 0.43f), 0, 0},              {"PLANT2", 19, RGB(0.45f, 0.58f, 0.22f), 5, 0.9},
};

// ---- the rooms ----------------------------------------------------------------------------------------------------------------------

constexpr ShipRoom roomOf(const char* name, int deck, int shape, double x0, double x1, double z0, double z1, double floor, double ceiling, int outer,
                          int floorFinish = FIN_DECK, int wallFinish = FIN_PANEL, int ceilingFinish = FIN_HULL) {
    ShipRoom r{};
    r.name = name; r.deck = deck; r.shape = shape;
    r.x0 = x0; r.x1 = x1; r.z0 = z0; r.z1 = z1; r.floor = floor; r.ceiling = ceiling; r.outer = outer;
    r.wallShade[SIDE_BOW] = 1.0; r.wallShade[SIDE_STARBOARD] = 0.9; r.wallShade[SIDE_STERN] = 0.8; r.wallShade[SIDE_PORT] = 0.9;
    r.floorShade = 0.7; r.ceilingShade = 0.85;
    r.floorFinish = floorFinish; r.wallFinish = wallFinish; r.ceilingFinish = ceilingFinish;
    return r;
}
// the bridge: 10 m across and 7 deep to the great window's arc (seven panes from 0.5 to 3.8 m over its floor), two steps up from the
// hall's arch
constexpr ShipRoom bridgeRoom() {
    ShipRoom r = roomOf("THE BRIDGE", DECK_MAIN, RS_BOW, -5, 5, 6.3, 11.0, 0.36, 4.56, OUT_BOW | OUT_STARBOARD | OUT_PORT | OUT_FLOOR | OUT_CEILING, FIN_DECK, FIN_HULL);
    r.bowZ = 13.3; r.bowPanes = 7; r.bowLo = 0.86; r.bowHi = 4.16;
    r.steps = 2; r.stepRise = 0.18; r.stepDepth = 0.3; r.stepFloor = 0;
    return r;
}
// the observatory: a disc of 6 m over the hall under its glass dome, a parapet of 0.4 m round its floor
constexpr ShipRoom observatoryRoom() {
    ShipRoom r = roomOf("THE OBSERVATORY", DECK_TOP, RS_DISC, 0, 6, 0, 0, 8.6, 14.6, OUT_BOW | OUT_STARBOARD | OUT_STERN | OUT_PORT, FIN_DECK, FIN_HULL);
    r.parapet = 0.4; r.domeN = ROUND; r.domeRings = 4;
    return r;
}
// the plan: the main deck's five round the hall (two decks high), the hangar and the maintenance room under them, the observatory over the
// hall; 30 m from the bow to the engines, 28.6 across the library and the laboratory, 19.5 from the hangar's floor to the dome's top
constexpr ShipRoom ROOMS[SHIP_ROOMS] = {
    bridgeRoom(),
    roomOf("THE HALL", DECK_MAIN, RS_BOX, -6, 6, -6, 6, 0, 8.4, 0),
    roomOf("THE LIBRARY", DECK_MAIN, RS_BOX, -14.3, -6.3, -4, 4, 0, 4.2, OUT_BOW | OUT_STERN | OUT_PORT | OUT_FLOOR | OUT_CEILING, FIN_DECK, FIN_PANEL, FIN_PANEL),
    roomOf("THE LABORATORY", DECK_MAIN, RS_BOX, 6.3, 14.3, -4, 4, 0, 4.2, OUT_BOW | OUT_STERN | OUT_STARBOARD | OUT_FLOOR | OUT_CEILING, FIN_HULL, FIN_HULL),
    roomOf("THE QUARTERS", DECK_MAIN, RS_BOX, -5, 5, -12.3, -6.3, 0, 3.4, OUT_STARBOARD | OUT_STERN | OUT_PORT | OUT_CEILING, FIN_DECK, FIN_PANEL, FIN_PANEL),
    roomOf("THE HANGAR", DECK_LOWER, RS_BOX, -8, 8, -8, 6, -4.9, -0.3, OUT_BOW | OUT_STARBOARD | OUT_PORT | OUT_FLOOR, FIN_HULL, FIN_HULL),
    roomOf("THE MAINTENANCE ROOM", DECK_LOWER, RS_BOX, -5, 5, -16.3, -8.3, -4.9, -0.3, OUT_STARBOARD | OUT_STERN | OUT_PORT | OUT_FLOOR, FIN_HULL, FIN_HULL),
    observatoryRoom(),
    roomOf("THE STAIR", DECK_MAIN, RS_STAIR, 0, 1.6, 0, 0, -4.9, 8.6, 0, FIN_DECK, FIN_HULL),
};

// the spiral stair in the hall's middle: from the hangar's floor through the hall's well and the oculus to the observatory's floor; a step a
// sixteenth of a turn, 0.196 m down to the hangar and 0.2 up to the dome (3.2 m a turn); the hall's landing toward the bridge
constexpr ShipStair STAIR = {0, 0, 0.3, 1.6, 3, {-4.9, 0, 8.6}, {ROOM_HANGAR, ROOM_HALL, ROOM_OBSERVATORY}, 0, PI / 8, 0.23, PI / 8, {25, 43}};
// a landing's angle, unwrapped (the hall's the stair's `at`; the hangar's and the observatory's by the steps between)
constexpr double landingA(int k) {
    return k == 1 ? STAIR.at : (k == 0 ? STAIR.at - 2 * STAIR.landing - STAIR.steps[0] * STAIR.step : STAIR.at + 2 * STAIR.landing + STAIR.steps[1] * STAIR.step);
}

// ---- the ways, the windows, the lamps, the places -------------------------------------------------------------------------------------

constexpr ShipOpening archOf(int from, int to, int side, double lo, double hi, double height) {
    ShipOpening o{};
    o.kind = OPEN_ARCH; o.from = from; o.to = to; o.side = side; o.lo = lo; o.hi = hi; o.height = height; o.round = true;
    return o;
}
constexpr ShipOpening holeOf(int kind, int from, int to, double x, double z, double r) {
    ShipOpening o{};
    o.kind = kind; o.from = from; o.to = to; o.x = x; o.z = z; o.r = r;
    return o;
}
constexpr ShipOpening landingOf(int room) {
    ShipOpening o{};
    o.kind = OPEN_STAIR; o.from = room; o.to = ROOM_STAIR;
    return o;
}
// the lift in the stair's column, from any room up to the observatory and down to the bridge (I-01's lifter to the roof deck)
constexpr ShipOpening lifterOf() {
    ShipOpening o{};
    o.kind = OPEN_LIFTER; o.from = ROOM_BRIDGE; o.to = ROOM_OBSERVATORY;
    o.up = {0.0, -3.3, 0, 0}; o.down = SHIP_START.at;
    o.upSays = "IN THE OBSERVATORY - PAGE DOWN TO THE BRIDGE"; o.downSays = "BACK ON THE BRIDGE";
    o.upPad = "UP TO THE OBSERVATORY"; o.downPad = "DOWN TO THE BRIDGE";
    return o;
}
// every room of the main deck opens onto the hall by an arch, the maintenance room onto the hangar; the well and the oculus seen through
constexpr ShipOpening OPENINGS[] = {
    archOf(ROOM_HALL, ROOM_BRIDGE, SIDE_BOW, -1.6, 1.6, 3.6),
    archOf(ROOM_HALL, ROOM_LIBRARY, SIDE_PORT, -1.6, 1.6, 3.6),
    archOf(ROOM_HALL, ROOM_LAB, SIDE_STARBOARD, -1.6, 1.6, 3.6),
    archOf(ROOM_HALL, ROOM_QUARTERS, SIDE_STERN, -1.2, 1.2, 3.0),
    archOf(ROOM_HANGAR, ROOM_MAINTENANCE, SIDE_STERN, -1.6, 1.6, 3.6),
    holeOf(OPEN_WELL, ROOM_HALL, ROOM_HANGAR, 0, 0, 1.8),
    holeOf(OPEN_OCULUS, ROOM_HALL, ROOM_OBSERVATORY, 0, 0, 2.0),
    landingOf(ROOM_HANGAR), landingOf(ROOM_HALL), landingOf(ROOM_OBSERVATORY),
    lifterOf(),
};
constexpr int N_OPENINGS = sizeof(OPENINGS) / sizeof(OPENINGS[0]);

// the long windows on the flanks (the library's to port, the laboratory's to starboard) and the quarters' astern
constexpr ShipWindow WINDOWS[] = {
    {ROOM_LIBRARY, SIDE_PORT, -3.2, 3.2, 0.8, 3.4, 3},
    {ROOM_LAB, SIDE_STARBOARD, -3.2, 3.2, 0.8, 3.4, 3},
    {ROOM_QUARTERS, SIDE_STERN, -3.6, 3.6, 0.8, 2.7, 2},
};

// I-03: each room's lamps (IDEAS 2.4): the bridge dim for the view, the hall warm over its beds, the library warm (its reading lamps), the
// lab bright, the quarters low, the hangar's work lights (the rack's first: it lights the drones on it), the maintenance room's work light
// (R-427: and a lamp over each platform, the engine room's fuel orb gone), the dome dark for the eyes: a room, a kind, a place, a power,
// a reach, a bulb's cone. I-04: the
// quarters' picture lamp over the photographs and the desk's lamp (glows: U's night keeps them), the hangar's over the cradle's new place.
// Z-05: the incubator's light inside its cabinet and the jars' behind them (glows: the lab lit by them in the dark ship)
constexpr ShipLamp LAMPS[] = {
    {ROOM_BRIDGE, LAMP_BULB, 2.6, 4.4, 8.9, 1.5, 3.0, 0.9},        {ROOM_BRIDGE, LAMP_BULB, -3.4, 4.4, 8.5, 1.25, 3.0, 0.9},
    {ROOM_BRIDGE, LAMP_BULB, 0, 4.4, 7.0, 0.9, 3.0, 0.9},
    {ROOM_HALL, LAMP_BULB, 4.0, 8.25, 4.0, 2.1, 4.5, 0.8},         {ROOM_HALL, LAMP_BULB, -4.0, 8.25, 4.0, 2.1, 4.5, 0.8},
    {ROOM_HALL, LAMP_BULB, 4.0, 8.25, -4.0, 2.1, 4.5, 0.8},        {ROOM_HALL, LAMP_BULB, -4.0, 8.25, -4.0, 2.1, 4.5, 0.8},
    {ROOM_LIBRARY, LAMP_BULB, -10.3, 4.05, 0, 2.6, 3.5, 1.0},      {ROOM_LIBRARY, LAMP_GLOW, -13.4, 1.5, -0.6, 1.1, 1.6, 0},
    {ROOM_LIBRARY, LAMP_GLOW, -11.4, 1.35, 3.45, 0.8, 1.4, 0},
    {ROOM_LAB, LAMP_BULB, 8.3, 4.05, 0, 2.4, 3.5, 1.1},            {ROOM_LAB, LAMP_BULB, 12.3, 4.05, 0, 2.4, 3.5, 1.1},
    {ROOM_LAB, LAMP_GLOW, 11.25, 1.55, -3.5, 0.7, 1.6, 0},         {ROOM_LAB, LAMP_GLOW, 11.4, 1.85, 3.7, 0.6, 1.6, 0},   // Z-05: the incubator's, the jars'
    {ROOM_QUARTERS, LAMP_GLOW, -3.5, 0.95, -11.2, 1.1, 1.8, 0},    {ROOM_QUARTERS, LAMP_BULB, 0, 3.25, -9.3, 1.2, 3.0, 0.9},
    {ROOM_QUARTERS, LAMP_GLOW, -3.15, 2.76, -6.85, 1.4, 1.5, 0},   {ROOM_QUARTERS, LAMP_GLOW, 4.75, 1.12, -10.0, 0.6, 1.2, 0},
    {ROOM_HANGAR, LAMP_BULB, -6.4, -0.45, 0.0, 2.0, 3.5, 0.8},     {ROOM_HANGAR, LAMP_BULB, 4.4, -0.45, 2.4, 3.0, 4.0, 0.75},
    {ROOM_HANGAR, LAMP_BULB, 5.6, -0.45, -3.0, 1.8, 3.5, 0.8},     {ROOM_HANGAR, LAMP_BULB, -4.0, -0.45, -5.5, 1.5, 3.5, 0.8},
    {ROOM_MAINTENANCE, LAMP_BULB, 0, -0.45, -9.6, 2.4, 3.5, 0.9},  {ROOM_MAINTENANCE, LAMP_BULB, -2.6, -0.45, -13.2, 2.6, 4.2, 0.8},
    {ROOM_MAINTENANCE, LAMP_BULB, 2.7, -0.45, -13.0, 2.6, 4.2, 0.8},
    {ROOM_OBSERVATORY, LAMP_GLOW, 0, 8.85, -5.2, 0.6, 1.8, 0},     {ROOM_OBSERVATORY, LAMP_GLOW, 0, 8.85, 5.2, 0.5, 1.8, 0},
};

constexpr ShipView VIEWS[] = {
    SHIP_START,
    // auto-explore's places: the great window beside the flight desk, the flanks' windows (a long flight's)
    {"THE FRONT WINDOW", ROOM_BRIDGE, {-2.4, 11.4, 0.0, 0.0}},
    {"THE RIGHT WINDOW", ROOM_LAB, {12.9, 0.0, PI / 2, 0}},
    {"THE LEFT WINDOW", ROOM_LIBRARY, {-12.9, 0.6, -PI / 2, 0}},
    // the harness's views (the devices' own are their rows' stands)
    {"TOWARD THE BOW", ROOM_BRIDGE, {3.4, 7.1, -0.45, -0.18}},
    {"THE CONSOLE ARC", ROOM_BRIDGE, {0.9, 8.4, 1.2, -0.3}},
    {"OVER THE TELESCOPE'S SET", ROOM_OBSERVATORY, {1.42, 3.05, 0.4363, -0.6}},
    {"OVER THE RADAR'S SET", ROOM_OBSERVATORY, {-2.62, 1.84, -0.9599, -0.6}},
    {"THE BRIDGE'S MIDDLE", ROOM_BRIDGE, {0.0, 8.4, 0.0, 0.0}},
    {"OUT TO THE DRONES' WAY", ROOM_BRIDGE, {-2.0, 9.0, -0.7, -0.05}},
    {"CLOSE AT THE CASE", ROOM_LIBRARY, {-10.3, -2.95, PI, -0.25}},
    {"NEAR THE GOES CONSOLE", ROOM_BRIDGE, {-3.4, 8.5, -PI / 2, 0.0}},
    {"AT THE TELESCOPE'S SET", ROOM_OBSERVATORY, {1.2678, 2.7190, 0.4363, 0.0}},
    {"ACROSS THE BRIDGE", ROOM_BRIDGE, {3.0, 7.3, -1.25, -0.2}},
    {"BY THE STAIR", ROOM_HALL, {2.6, 3.6, -0.7, 0.2}},
    // each space from its middle (`ship rooms`; the bridge's is THE BRIDGE'S MIDDLE)
    {"THE HALL'S MIDDLE", ROOM_HALL, {3.4, 3.0, -2.29, 0.2}},
    {"THE LIBRARY'S MIDDLE", ROOM_LIBRARY, {-10.3, 0.0, -PI / 2, 0.0}},
    {"THE LABORATORY'S MIDDLE", ROOM_LAB, {10.3, 0.0, PI / 2, 0.0}},
    {"THE QUARTERS' MIDDLE", ROOM_QUARTERS, {0.0, -9.3, PI, 0.0}},
    {"THE HANGAR'S MIDDLE", ROOM_HANGAR, {-2.5, -4.0, 0.55, 0.05}},
    {"THE MAINTENANCE ROOM'S MIDDLE", ROOM_MAINTENANCE, {0.0, -8.45, PI, -0.15}},
    {"THE PLATFORMS", ROOM_MAINTENANCE, {0.5, -9.9, PI + 0.78, -0.25}},   // R-427: the buggy on its platform, from the walk
    {"THE OBSERVATORY'S MIDDLE", ROOM_OBSERVATORY, {0.0, -3.3, 0.0, 0.35}},
    // I-03: where `ship light` sees the sun's patches fall (the bridge's floor by the great window, the library's under its window) and the
    // hull outside (from the dome the roofs, the ring astern and the mast; from the great window the prow)
    {"THE BRIDGE'S FLOOR", ROOM_BRIDGE, {-3.0, 7.4, 0.42, -0.5}},
    {"THE LIBRARY'S FLOOR", ROOM_LIBRARY, {-8.0, 1.5, -1.9, -0.5}},
    {"THE LABORATORY'S ARCH", ROOM_HALL, {3.4, -0.4, PI / 2, 0.05}},   // B-434: where the user saw the laboratory's patch fill the arch
    {"OVER THE ROOFS", ROOM_OBSERVATORY, {-1.2, -5.0, 2.65, -0.5}},
    {"OVER THE PROW", ROOM_BRIDGE, {-1.0, 11.4, 0.15, -0.55}},
    // I-04: life aboard (`ship life`): the photographs on the quarters' wall, a bed of the garden, the shards' shelves either side of the
    // decoder, the drive's core under the hall's floor glass, the case's instruments; the hatch's drop (the camera a descent opens on: the
    // capsule dropping through the hangar's floor)
    {"THE QUARTERS' WALL", ROOM_QUARTERS, {-1.7, -8.9, -0.55, 0.02}},
    {"THE GARDEN", ROOM_HALL, {2.2, 1.8, 0.71, -0.12}},
    {"THE SHELVES", ROOM_LIBRARY, {-10.3, -0.9, 0.0, 0.0}},
    {"AT THE SHELVES", ROOM_LIBRARY, {-7.55, 0.4, 0.0, -0.12}},
    {"THE CORE", ROOM_HALL, {0.0, 5.4, PI, -0.75}},
    {"THE CASE'S GLASS", ROOM_LIBRARY, {-10.3, -2.2, PI, -0.15}},
    {"THE HATCH'S DROP", ROOM_HANGAR, {1.0, -3.4, 0.53, -0.2}},
    // Z-04: the cold store's door (its lamps lit by how full it is), from the laboratory's middle toward the arch
    {"THE COLD STORE", ROOM_LAB, {9.6, -1.35, -2.05, -0.1}},
    // Z-05: the incubator's flasks behind its glass, close; the jars' shelf over the bow bench; the laboratory's stern bench whole (the bench's)
    {"THE INCUBATOR", ROOM_LAB, {11.25, -2.15, PI, -0.32}},
    {"THE JARS", ROOM_LAB, {11.6, 1.6, 0.0, 0.12}},
    {"THE SEQUENCER", ROOM_LAB, {9.05, 2.3, 0.0, -0.36}},   // Z-06: the sequencer on the bow bench; the tree of kin's screen on the port wall
    {"THE TREE OF KIN", ROOM_LAB, {8.3, 2.85, -PI / 2, -0.02}},
    {"THE STERN BENCH", ROOM_LAB, {10.6, 0.6, 2.9, -0.2}},
};

template <class Row, int N> constexpr ShipRows<Row> planRows(const Row (&rows)[N]) { return {rows, N}; }

// the plan whole: the rooms in their order's shapes, the stair's levels rising, every way between two rooms of the plan (an arch on one
// of its room's sides, within the wall), one lifter, every window, lamp and view in a room
constexpr bool planWhole() {
    if (ROOMS[ROOM_BRIDGE].shape != RS_BOW || ROOMS[ROOM_OBSERVATORY].shape != RS_DISC || ROOMS[ROOM_STAIR].shape != RS_STAIR) return false;
    for (int k = 0; k + 1 < STAIR.levels; k++) if (STAIR.level[k] >= STAIR.level[k + 1]) return false;
    int lifters = 0;
    for (const ShipOpening& o : OPENINGS) {
        if (o.from < 0 || o.from >= SHIP_ROOMS || o.to < 0 || o.to >= SHIP_ROOMS || o.from == o.to) return false;
        if (o.kind == OPEN_ARCH) {
            const ShipRoom& R = ROOMS[o.from];
            const double a0 = o.side == SIDE_BOW || o.side == SIDE_STERN ? R.x0 : R.z0, a1 = o.side == SIDE_BOW || o.side == SIDE_STERN ? R.x1 : R.z1;
            if (o.side < 0 || o.side > 3 || o.lo < a0 + 0.5 || o.hi > a1 - 0.5 || o.lo >= o.hi) return false;
        }
        if (o.kind == OPEN_LIFTER) lifters++;
    }
    if (lifters != 1) return false;
    for (const ShipWindow& w : WINDOWS) if (w.room < 0 || w.room >= SHIP_ROOMS || ROOMS[w.room].shape != RS_BOX) return false;
    for (const ShipLamp& l : LAMPS) if (l.room < 0 || l.room >= SHIP_ROOMS) return false;
    for (const ShipView& v : VIEWS) if (v.room < 0 || v.room >= SHIP_ROOMS) return false;
    return true;
}
static_assert(planWhole(), "every row in a room of the plan, every way between two");

// ---- what stands in the rooms (made once: their corners turn by their facings) -------------------------------------------------------

// a box by its middle, the way its front faces (yaw: 0 the bow, a quarter turn starboard), half its width and depth, its floor and
// height, its faces and their shades; its corners a b c d: the back and the front at one end, the front and the back at the other
ShipFixture boxAt(int room, int device, double cx, double cz, double yaw, double hw, double hd, double base, double h, std::initializer_list<int> faces,
                  std::initializer_list<double> shades, bool open = false) {
    ShipFixture f{};
    f.room = room; f.kind = FX_BOX; f.device = device; f.height = h; f.open = open; f.topFinish = -1;
    const double fx = std::sin(yaw), fz = std::cos(yaw), rx = std::cos(yaw), rz = -std::sin(yaw);
    const double sw[4] = {-1, -1, 1, 1}, sd[4] = {-1, 1, 1, -1};
    for (int i = 0; i < 4; i++) { f.x[i] = cx + rx * hw * sw[i] + fx * hd * sd[i]; f.y[i] = base; f.z[i] = cz + rz * hw * sw[i] + fz * hd * sd[i]; }
    int i = 0;
    for (int fc : faces) if (i < 6) f.faces[i++] = fc;
    i = 0;
    for (double s : shades) if (i < 6) f.shade[i++] = s;
    return f;
}
// a screen on a box's front face (b to c), its edges over the box's floor, inset from its ends; a factor of its device's shade
ShipFixture screenOn(const ShipFixture& b, double lo, double hi, double inset, double factor) {
    ShipFixture f{};
    f.room = b.room; f.kind = FX_SCREEN; f.device = b.device; f.factor = factor;
    const double wx = b.x[2] - b.x[1], wz = b.z[2] - b.z[1], w = std::hypot(wx, wz);
    const double nx = b.x[1] - b.x[0], nz = b.z[1] - b.z[0], n = std::hypot(nx, nz);
    const double t0 = inset / w, t1 = 1 - inset / w, ox = nx / n * 0.012, oz = nz / n * 0.012;
    const double ts[4] = {t0, t1, t1, t0}, ys[4] = {lo, lo, hi, hi};
    for (int i = 0; i < 4; i++) { f.x[i] = b.x[1] + wx * ts[i] + ox; f.y[i] = b.y[1] + ys[i]; f.z[i] = b.z[1] + wz * ts[i] + oz; }
    return f;
}
// a screen lying on a desk's top, raised toward its back (the flight desk's)
ShipFixture screenTop(const ShipFixture& b, double inset, double factor) {
    ShipFixture f{};
    f.room = b.room; f.kind = FX_SCREEN; f.device = b.device; f.factor = factor;
    const double wx = b.x[2] - b.x[1], wz = b.z[2] - b.z[1], w = std::hypot(wx, wz);
    const double bx = b.x[0] - b.x[1], bz = b.z[0] - b.z[1];   // from the front toward the back
    const double t0 = inset / w, t1 = 1 - inset / w, top = b.y[1] + b.height;
    const double ts[4] = {t0, t1, t1, t0}, ds[4] = {0.18, 0.18, 0.78, 0.78}, ys[4] = {0.012, 0.012, 0.11, 0.11};
    for (int i = 0; i < 4; i++) { f.x[i] = b.x[1] + wx * ts[i] + bx * ds[i]; f.y[i] = top + ys[i]; f.z[i] = b.z[1] + wz * ts[i] + bz * ds[i]; }
    return f;
}
// the capsule's cradle: twelve bars round its middle up to `high` over its floor and a ring along their tops; I-04: round the floor's hatch
// (its radius `ringR`: a round hole in the floor, its two leaves sliding apart as the capsule drops), the capsule in it as the surface
// draws it (a dome of 1.6 m squashed to 0.8 on its ring of 1.3 times that, ten segments and four rings: `SurfaceView::drawCapsule`'s)
ShipFixture cageAt(int room, double x, double z, double base, double high) {
    ShipFixture f{};
    f.room = room; f.kind = FX_CAGE; f.device = FACE_CAGE; f.x[0] = x; f.z[0] = z; f.y[0] = base;
    f.radius = 2.45; f.bars = 12; f.barLo = base + 0.05; f.barHi = base + high; f.barShade = 1.2;
    f.ringR = 2.2;
    f.domeR = 1.6; f.domeY = base + 0.2; f.domeSquash = 0.8; f.domeRings = 4; f.domeSegments = 10;
    return f;
}
// the drone rack against a wall at x0 (+x into the room), its middle along z, three shelves and the bay's screen across its head (C-18);
// the drones leave through the ports in its back, out from the hull and up past the flank before they turn for the world
ShipFixture rackAt(int room, double x0, double zMid, double base) {
    ShipFixture f{};
    f.room = room; f.kind = FX_RACK; f.device = FACE_BAY;
    f.x0 = x0; f.x1 = x0 + 0.62; f.z0 = zMid - 0.515; f.z1 = zMid + 0.515; f.top = base + 1.66; f.screenLo = base + 1.38; f.screenHi = base + 1.60;
    f.droneX = x0 + 0.31; f.droneZ = zMid; f.base = base; f.outward = 8.0; f.rise = 6.2;
    return f;
}
// a tube of a square section from one end to the other (the telescope's, the launcher's): no footprint (it stands on its own pier)
ShipFixture tubeOf(int room, double x0, double y0, double z0, double x1, double y1, double z1, double half) {
    ShipFixture f{};
    f.room = room; f.kind = FX_TUBE; f.device = FACE_NOTHING; f.open = true; f.radius = half;
    f.x[0] = x0; f.y[0] = y0; f.z[0] = z0; f.x[1] = x1; f.y[1] = y1; f.z[1] = z1;
    return f;
}
// R-427: a vehicle standing on its platform: the buggy (0) or the drone (1), its feet at (x, base, z), its nose toward `yaw` (0 the bow)
ShipFixture vehicleAt(int room, int which, double x, double z, double base, double yaw) {
    ShipFixture f{};
    f.room = room; f.kind = FX_VEHICLE; f.device = FACE_NOTHING; f.slots = which; f.open = true;   // the platform under it keeps the walk off
    f.x[0] = x; f.y[0] = base; f.z[0] = z; f.x[1] = x + std::sin(yaw); f.y[1] = base; f.z[1] = z + std::cos(yaw);
    return f;
}
// a ring standing upright across the ship (its axis along z): the drive's (outside the hull)
ShipFixture ringOf(int room, double x, double y, double z, double outer, double inner, double depth) {
    ShipFixture f{};
    f.room = room; f.kind = FX_RING; f.device = FACE_NOTHING; f.x[0] = x; f.y[0] = y; f.z[0] = z; f.radius = outer; f.inner = inner; f.depth = depth;
    return f;
}
// I-03: the radar's mast outside the hull: its foot, its height, its dish's radius (the dish turned by the frame)
ShipFixture mastAt(double x, double y, double z, double height, double dish) {
    ShipFixture f{};
    f.room = SHIP_OUTSIDE; f.kind = FX_MAST; f.device = FACE_NOTHING; f.open = true; f.x[0] = x; f.y[0] = y; f.z[0] = z; f.height = height; f.radius = dish;
    f.finish = FIN_OUTSIDE;
    return f;
}
// I-03: a fixture's finish, and a box's top's when it is another (a bed's plants, a bunk's mattress)
ShipFixture fin(ShipFixture f, int finish, int top = -1) { f.finish = finish; f.topFinish = top; return f; }
// I-04: a box made another kind the frame fills (a shelf's boards and worlds, a bed's number, the prints' rows and columns, the display's
// shelves and places)
ShipFixture kindOf(ShipFixture f, int kind, int boards = 0, int slots = 0) { f.kind = kind; f.boards = boards; f.slots = slots; return f; }

// the rooms' furniture in the order each room draws it (its shell before it)
std::vector<ShipFixture> makeFixtures() {
    std::vector<ShipFixture> v;
    const std::initializer_list<int> CONSOLE = {BF_TOP, BF_FRONT, BF_SIDE_A, BF_SIDE_D, BF_BACK};
    const std::initializer_list<double> CONSOLE_SHADE = {0.9, 0.8, 0.7, 0.7, 0.75};
    const std::initializer_list<int> AGAINST = {BF_TOP, BF_FRONT, BF_SIDE_A, BF_SIDE_D};   // against a wall: no back
    const std::initializer_list<double> AGAINST_SHADE = {0.9, 0.8, 0.7, 0.7};
    // the bridge: the flight desk facing the great window, the console arc of the star map's, the landing's and the target's screens to
    // starboard (from aft to fore), the GOES console in its alcove to port (R-404: each screen in its computer's own bank)
    const double bf = ROOMS[ROOM_BRIDGE].floor;
    const ShipFixture desk = boxAt(ROOM_BRIDGE, FACE_FLIGHT, 0, 10.55, PI, 1.3, 0.35, bf, 0.85, CONSOLE, CONSOLE_SHADE);
    v.push_back(desk); v.push_back(screenTop(desk, 0.25, 1));
    for (int k = 0; k < 3; k++) {
        const double a = (90 - 14 * k) * DEG;
        const ShipFixture c = boxAt(ROOM_BRIDGE, FACE_STAR_MAP + k, 4.1 * std::sin(a), 8.9 + 4.1 * std::cos(a), a + PI, 0.45, 0.3, bf, 0.95, CONSOLE, CONSOLE_SHADE);
        v.push_back(c); v.push_back(screenOn(c, 0.35, 0.9, 0.07, 1));
    }
    const ShipFixture goes = boxAt(ROOM_BRIDGE, FACE_GOES, -4.65, 8.5, PI / 2, 0.7, 0.35, bf, 0.95, AGAINST, AGAINST_SHADE);
    v.push_back(goes); v.push_back(screenOn(goes, 0.35, 0.9, 0.1, 1));
    for (int s : {-1, 1}) v.push_back(fin(boxAt(ROOM_BRIDGE, FACE_NOTHING, -4.6, 8.5 + s * 1.05, PI / 2, 0.15, 0.4, bf, 2.6, {BF_FRONT, BF_SIDE_A, BF_SIDE_D, BF_TOP}, {0.75, 0.65, 0.65, 0.85}), FIN_PANEL));
    v.push_back(fin(boxAt(ROOM_BRIDGE, FACE_NOTHING, -4.6, 8.5, PI / 2, 1.2, 0.4, bf + 2.6, 0.18, {BF_BOTTOM, BF_FRONT, BF_TOP}, {0.6, 0.75, 0.85}, true), FIN_PANEL));
    // the hall: the garden's four beds in its corners (I-04: the home world's plants in their soil); the drive's core under the floor glass
    // between the bridge's arch and the stair
    int bed = 0;
    for (int sx : {-1, 1})
        for (int sz : {-1, 1}) v.push_back(kindOf(fin(boxAt(ROOM_HALL, FACE_NOTHING, sx * 4.75, sz * 4.75, 0, 1.15, 1.15, 0, 0.55, CONSOLE, {0.55, 0.8, 0.7, 0.7, 0.75}), FIN_PANEL, FIN_DECK), FX_BED, 0, bed++));
    v.push_back(kindOf(boxAt(ROOM_HALL, FACE_NOTHING, 0, 3.6, 0, 1.0, 1.0, -0.26, 0.26, {}, {}, true), FX_CORE));
    // the library: the shard decoder's desk on the bow wall, the instruments' case on the stern wall (its glass's three shelves), the
    // shards' shelves (I-04 fills them), the ship's player's deck by the arch, a sofa facing the window
    const double lf = ROOMS[ROOM_LIBRARY].floor;
    const ShipFixture dec = fin(boxAt(ROOM_LIBRARY, FACE_DECODER, -10.3, 3.65, PI, 0.8, 0.35, lf, 0.95, AGAINST, AGAINST_SHADE), FIN_PANEL);
    v.push_back(dec); v.push_back(screenOn(dec, 0.35, 0.9, 0.2, 1));
    const ShipFixture cs = fin(boxAt(ROOM_LIBRARY, FACE_CASE, -10.3, -3.72, 0, 0.55, 0.28, lf, 1.45, AGAINST, {0.9, 0.75, 0.6, 0.6}), FIN_PANEL);
    v.push_back(cs);
    const double glassLo = 0.18, glassH = 0.34, glassStep = 0.42, glassInset = 0.07;   // its three shelves' glass, lit from behind
    for (int s = 0; s < 3; s++) v.push_back(screenOn(cs, glassLo + s * glassStep, glassLo + glassH + s * glassStep, glassInset, 0.8 + 0.1 * s));
    {   // I-04: the instruments held against the case's lit glass, four a shelf (no walk's footprint: the case's)
        ShipFixture d = kindOf(cs, FX_DISPLAY, 3, 4);
        d.device = FACE_NOTHING; d.open = true; d.finish = FIN_BRASS;
        d.screenLo = glassLo; d.screenHi = glassH; d.rise = glassStep; d.depth = glassInset;
        v.push_back(d);
    }
    // I-04: the shards' shelves (four boards of four worlds each), the player's deck (its reels on its top)
    for (double x : {-7.55, -13.05})
        for (int s : {-1, 1}) v.push_back(kindOf(fin(boxAt(ROOM_LIBRARY, FACE_NOTHING, x, s * 3.75, s > 0 ? PI : 0, 0.75, 0.25, lf, 2.3, AGAINST, {0.9, 0.7, 0.6, 0.6}), FIN_PANEL), FX_SHELF, 4, 4));
    v.push_back(kindOf(fin(boxAt(ROOM_LIBRARY, FACE_DECK, -6.85, 2.7, -PI / 2, 0.45, 0.25, lf, 1.05, AGAINST, {0.9, 0.75, 0.65, 0.65}), FIN_PANEL, FIN_BRASS), FX_DECK));
    v.push_back(fin(boxAt(ROOM_LIBRARY, FACE_NOTHING, -12.7, -2.0, -PI / 2, 1.0, 0.4, lf, 0.45, AGAINST, {0.85, 0.7, 0.6, 0.6}), FIN_FABRIC));
    v.push_back(fin(boxAt(ROOM_LIBRARY, FACE_NOTHING, -12.2, -2.0, -PI / 2, 1.0, 0.12, lf, 0.9, CONSOLE, {0.85, 0.75, 0.6, 0.6, 0.7}), FIN_FABRIC));
    // the laboratory: its benches along the bow and stern walls, the jars' shelf over the bow bench, the cold store's door by the arch (Z-06
    // brings the rest of its devices). Z-04: the microscope on the stern bench by the arch (its stand, its arm and its head, the eyepieces' tube;
    // its camera's screen beside it), the cold store's door lit by how full it is (eight lamps, R-432's by doublings). Z-05: the jars on
    // their shelf behind its glass (eight), the incubator on the stern bench past the microscope (its cabinet's three shelves of four flasks
    // behind its glass door, its screen beside it)
    const double af = ROOMS[ROOM_LAB].floor;
    for (int s : {-1, 1}) v.push_back(boxAt(ROOM_LAB, FACE_NOTHING, 10.3, s * 3.65, s > 0 ? PI : 0, 2.6, 0.35, af, 0.92, AGAINST, {0.9, 0.78, 0.65, 0.65}));
    v.push_back(kindOf(fin(boxAt(ROOM_LAB, FACE_NOTHING, 10.3, 3.82, PI, 2.4, 0.18, af + 1.55, 0.7, {BF_BOTTOM, BF_TOP, BF_SIDE_A, BF_SIDE_D}, {0.6, 0.9, 0.65, 0.65}, true),
                              FIN_GLASS, FIN_HULL), FX_JARS, 1, JAR_PLACES));
    {
        const double top = af + 0.92;
        v.push_back(kindOf(fin(boxAt(ROOM_LAB, FACE_INCUBATOR, 11.25, -3.69, 0, 0.62, 0.28, top, 0.78, {BF_TOP, BF_SIDE_A, BF_SIDE_D}, {0.9, 0.7, 0.7}, true), FIN_PANEL),
                           FX_INCUBATOR, 3, 4));
        const ShipFixture scr = fin(boxAt(ROOM_LAB, FACE_INCUBATOR, 12.42, -3.76, 0, 0.28, 0.05, top + 0.04, 0.4, CONSOLE, CONSOLE_SHADE, true), FIN_PANEL);
        v.push_back(scr); v.push_back(screenOn(scr, 0.06, 0.36, 0.03, 1));   // its screen: the flasks' curves
    }
    {   // Z-06: the sequencer on the bow bench under the jars' shelf (its case, its lid, its screen beside it); the tree of kin's screen on the
        // port wall to the bow of the arch
        const double top = af + 0.92;
        v.push_back(fin(boxAt(ROOM_LAB, FACE_SEQUENCER, 8.75, 3.6, PI, 0.3, 0.2, top, 0.24, CONSOLE, CONSOLE_SHADE, true), FIN_PANEL));
        v.push_back(fin(boxAt(ROOM_LAB, FACE_SEQUENCER, 8.75, 3.63, PI, 0.22, 0.13, top + 0.24, 0.04, CONSOLE, {0.95, 0.85, 0.75, 0.75, 0.8}, true), FIN_BRASS));   // its lid
        const ShipFixture scr = fin(boxAt(ROOM_LAB, FACE_SEQUENCER, 9.4, 3.76, PI, 0.24, 0.05, top + 0.04, 0.4, CONSOLE, CONSOLE_SHADE, true), FIN_PANEL);
        v.push_back(scr); v.push_back(screenOn(scr, 0.06, 0.36, 0.03, 1));   // its reading
        const ShipFixture wall = fin(boxAt(ROOM_LAB, FACE_TREE, 6.34, 2.85, PI / 2, 0.48, 0.04, af + 1.15, 0.9, AGAINST, {0.9, 0.8, 0.7, 0.7}, true), FIN_PANEL);
        v.push_back(wall); v.push_back(screenOn(wall, 0.06, 0.84, 0.05, 1));   // its screen
        v.push_back(kindOf(boxAt(ROOM_LAB, FACE_NOTHING, 6.397, 2.85, PI / 2, 0.43, 0.005, af + 1.21, 0.78, {}, {}, true), FX_TREE));   // the tree's lines over it
    }
    v.push_back(kindOf(boxAt(ROOM_LAB, FACE_NOTHING, 6.68, -2.9, PI / 2, 0.55, 0.38, af, 2.3, AGAINST, {0.9, 0.85, 0.65, 0.65}), FX_STORE, 0, 8));
    {
        const double top = af + 0.92;
        v.push_back(boxAt(ROOM_LAB, FACE_MICROSCOPE, 8.6, -3.62, 0, 0.2, 0.17, top, 0.06, CONSOLE, {0.9, 0.8, 0.7, 0.7, 0.75}, true));       // its foot
        v.push_back(boxAt(ROOM_LAB, FACE_MICROSCOPE, 8.6, -3.74, 0, 0.07, 0.06, top + 0.06, 0.38, CONSOLE, {0.9, 0.8, 0.7, 0.7, 0.75}, true));   // its arm
        v.push_back(boxAt(ROOM_LAB, FACE_MICROSCOPE, 8.6, -3.58, 0, 0.11, 0.1, top + 0.3, 0.11, CONSOLE, {0.95, 0.85, 0.75, 0.75, 0.8}, true));   // its head
        v.push_back(boxAt(ROOM_LAB, FACE_MICROSCOPE, 8.6, -3.5, 0, 0.13, 0.11, top + 0.16, 0.02, CONSOLE, {0.95, 0.8, 0.7, 0.7, 0.75}, true));   // its stage
        v.push_back(fin(tubeOf(ROOM_LAB, 8.6, top + 0.4, -3.6, 8.6, top + 0.5, -3.43, 0.025), FIN_BRASS));   // the eyepieces' tube
        const ShipFixture cam = fin(boxAt(ROOM_LAB, FACE_MICROSCOPE, 9.35, -3.76, 0, 0.3, 0.05, top + 0.04, 0.42, CONSOLE, CONSOLE_SHADE, true), FIN_PANEL);
        v.push_back(cam); v.push_back(screenOn(cam, 0.06, 0.38, 0.03, 1));   // its camera's screen
    }
    // the quarters: the bunk along the port wall with its head and the bedside table, the desk to starboard with its lamp; I-04: the
    // photographs on the bow wall to port of the arch (four across, three down), the picture lamp's arm and bar over them
    const double qf = ROOMS[ROOM_QUARTERS].floor;
    v.push_back(fin(boxAt(ROOM_QUARTERS, FACE_BUNK, -4.33, -9.6, PI / 2, 1.05, 0.65, qf, 0.55, AGAINST, {0.85, 0.7, 0.62, 0.62}), FIN_PANEL, FIN_FABRIC));
    v.push_back(fin(boxAt(ROOM_QUARTERS, FACE_NOTHING, -4.33, -10.75, PI / 2, 0.1, 0.65, qf, 1.1, AGAINST, {0.85, 0.7, 0.62, 0.62}), FIN_PANEL));
    v.push_back(fin(boxAt(ROOM_QUARTERS, FACE_NOTHING, -3.5, -11.4, PI / 2, 0.22, 0.25, qf, 0.6, CONSOLE, {0.85, 0.75, 0.62, 0.62, 0.7}), FIN_PANEL));
    v.push_back(fin(boxAt(ROOM_QUARTERS, FACE_DESK, 4.62, -9.4, -PI / 2, 0.8, 0.38, qf, 0.76, AGAINST, {0.9, 0.78, 0.65, 0.65}), FIN_PANEL));
    v.push_back(fin(tubeOf(ROOM_QUARTERS, 4.75, qf + 0.76, -10.0, 4.75, qf + 1.08, -10.0, 0.018), FIN_BRASS));
    {
        const ShipRoom& Q = ROOMS[ROOM_QUARTERS];
        ShipFixture w = boxAt(ROOM_QUARTERS, FACE_NOTHING, -3.15, Q.z1 - 0.02, PI, 1.3, 0.02, qf + 1.1, 1.29, {}, {}, true);
        v.push_back(kindOf(fin(w, FIN_PRINT), FX_PRINTS, 3, 4));
        v.push_back(fin(tubeOf(ROOM_QUARTERS, -3.15, qf + 2.86, Q.z1, -3.15, qf + 2.8, Q.z1 - 0.55, 0.015), FIN_BRASS));
        v.push_back(fin(tubeOf(ROOM_QUARTERS, -3.6, qf + 2.8, Q.z1 - 0.55, -2.7, qf + 2.8, Q.z1 - 0.55, 0.03), FIN_BRASS));
    }
    // the hangar: the capsule's cradle over the floor's hatch (I-04: the capsule in it, the hatch's leaves under it), the drone rack against
    // the port wall in its door (the ports in its back), the probe's launcher to starboard, the kit's locker aft (I-04: its doors)
    const double hf = ROOMS[ROOM_HANGAR].floor;
    v.push_back(fin(cageAt(ROOM_HANGAR, 4.4, 2.4, hf, 2.2), FIN_BRASS));
    v.push_back(rackAt(ROOM_HANGAR, -8, 0, hf));
    for (int s : {-1, 1}) v.push_back(boxAt(ROOM_HANGAR, FACE_NOTHING, -7.94, s * 0.69, PI / 2, 0.075, 0.06, hf, 2.3, AGAINST, {0.85, 0.7, 0.6, 0.6}));   // the drones' door round it
    v.push_back(boxAt(ROOM_HANGAR, FACE_NOTHING, -7.94, 0, PI / 2, 0.765, 0.06, hf + 2.1, 0.2, {BF_BOTTOM, BF_FRONT, BF_TOP, BF_SIDE_A, BF_SIDE_D}, {0.55, 0.7, 0.85, 0.6, 0.6}, true));
    v.push_back(boxAt(ROOM_HANGAR, FACE_NOTHING, -7.995, 0, PI / 2, 0.615, 0.003, hf + 1.66, 0.44, {BF_FRONT}, {0.4}, true));
    v.push_back(boxAt(ROOM_HANGAR, FACE_NOTHING, 6.6, -3.0, -PI / 2, 1.2, 0.5, hf, 1.1, AGAINST, {0.9, 0.78, 0.65, 0.65}));
    v.push_back(tubeOf(ROOM_HANGAR, 6.6, hf + 1.36, -4.1, 6.6, hf + 1.36, -1.7, 0.24));
    v.push_back(kindOf(boxAt(ROOM_HANGAR, FACE_NOTHING, -6.0, -7.62, 0, 0.9, 0.38, hf, 2.1, AGAINST, {0.9, 0.8, 0.65, 0.65}), FX_LOCKER));
    // R-427: the maintenance room (the engine room's, its drive's ring gone): the buggy and the drone the capsule carries, each on its
    // platform under its work lamp, the buggy turned to the walk between them, the drone's nose to the arch (its fans over its platform's
    // edges); a workbench against the stern wall
    const double mf = ROOMS[ROOM_MAINTENANCE].floor, deck = 0.32;
    v.push_back(fin(boxAt(ROOM_MAINTENANCE, FACE_NOTHING, -2.6, -13.2, 0.4, 1.35, 2.0, mf, deck, CONSOLE, {0.8, 0.85, 0.7, 0.7, 0.75}), FIN_HULL, FIN_DECK));
    v.push_back(vehicleAt(ROOM_MAINTENANCE, 0, -2.6, -13.2, mf + deck, 0.4));
    v.push_back(fin(boxAt(ROOM_MAINTENANCE, FACE_NOTHING, 2.7, -13.0, 0, 1.8, 1.8, mf, deck, CONSOLE, {0.8, 0.85, 0.7, 0.7, 0.75}), FIN_HULL, FIN_DECK));
    v.push_back(vehicleAt(ROOM_MAINTENANCE, 1, 2.7, -13.0, mf + deck, 0));
    v.push_back(fin(boxAt(ROOM_MAINTENANCE, FACE_NOTHING, 0, -15.95, 0, 1.3, 0.33, mf, 0.9, AGAINST, {0.9, 0.78, 0.65, 0.65}), FIN_HULL, FIN_DECK));
    // the observatory: the telescope's set and its mount (the pier, the tube raised to the bow), the radar's set, a couch
    const double of = ROOMS[ROOM_OBSERVATORY].floor;
    auto onRing = [](double degs, double r, double& x, double& z) { x = r * std::sin(degs * DEG); z = r * std::cos(degs * DEG); };
    double x, z;
    onRing(25, 4.4, x, z);
    const ShipFixture tset = boxAt(ROOM_OBSERVATORY, FACE_TELESCOPE, x, z, 25 * DEG + PI, 0.5, 0.3, of, 0.95, CONSOLE, CONSOLE_SHADE);
    v.push_back(tset); v.push_back(screenOn(tset, 0.35, 0.9, 0.1, 1));
    onRing(-15, 4.5, x, z);
    v.push_back(boxAt(ROOM_OBSERVATORY, FACE_NOTHING, x, z, -15 * DEG + PI, 0.28, 0.28, of, 1.15, CONSOLE, {0.85, 0.75, 0.65, 0.65, 0.7}));
    {
        const double e = 50 * DEG, dx = std::sin(-15 * DEG) * std::cos(e), dy = std::sin(e), dz = std::cos(-15 * DEG) * std::cos(e), cy = of + 1.4;
        v.push_back(fin(tubeOf(ROOM_OBSERVATORY, x - dx * 0.8, cy - dy * 0.8, z - dz * 0.8, x + dx * 1.1, cy + dy * 1.1, z + dz * 1.1, 0.19), FIN_BRASS));
    }
    onRing(-55, 4.4, x, z);
    const ShipFixture rset = boxAt(ROOM_OBSERVATORY, FACE_RADAR, x, z, -55 * DEG + PI, 0.55, 0.3, of, 0.95, CONSOLE, CONSOLE_SHADE);
    v.push_back(rset); v.push_back(screenOn(rset, 0.35, 0.9, 0.1, 1));
    onRing(160, 4.6, x, z);
    v.push_back(fin(boxAt(ROOM_OBSERVATORY, FACE_NOTHING, x, z, 160 * DEG + PI, 1.0, 0.4, of, 0.42, CONSOLE, {0.85, 0.7, 0.6, 0.6, 0.7}), FIN_FABRIC));
    onRing(160, 5.1, x, z);
    v.push_back(fin(boxAt(ROOM_OBSERVATORY, FACE_NOTHING, x, z, 160 * DEG + PI, 1.0, 0.12, of, 0.85, CONSOLE, {0.85, 0.75, 0.6, 0.6, 0.7}), FIN_FABRIC));
    // I-03: outside the hull (seen through the windows and the dome): the drive's ring astern on its four struts (its inner face aglow),
    // the radar's mast on the quarters' roof, the drones' door's frame in the hangar's port flank round the rack's ports
    const ShipRoom& E = ROOMS[ROOM_MAINTENANCE];
    const double ry = (E.floor + E.ceiling) / 2, rz = E.z0 - 0.3 - 0.75;
    v.push_back(fin(ringOf(SHIP_OUTSIDE, 0, ry, rz, 5.2, 4.3, 0.7), FIN_OUTSIDE));
    for (int k = 0; k < 4; k++) {
        const double a = (45 + 90 * k) * DEG, sa = std::sin(a), ca = std::cos(a);
        v.push_back(fin(tubeOf(SHIP_OUTSIDE, 3.3 * sa, ry + 2.0 * ca, E.z0 - 0.3, 4.4 * sa, ry + 4.4 * ca, rz + 0.3, 0.14), FIN_OUTSIDE));
    }
    const ShipRoom& QR = ROOMS[ROOM_QUARTERS];
    v.push_back(mastAt(3.5, QR.ceiling + 0.3, -8.5, 3.8, 0.95));
    const ShipRoom& HG = ROOMS[ROOM_HANGAR];
    const double dx = HG.x0 - 0.3;
    for (int s : {-1, 1}) v.push_back(fin(boxAt(SHIP_OUTSIDE, FACE_NOTHING, dx - 0.05, s * 0.8, -PI / 2, 0.1, 0.05, hf, 2.45, {BF_FRONT, BF_SIDE_A, BF_SIDE_D, BF_TOP}, {0.85, 0.7, 0.7, 0.9}, true), FIN_OUTSIDE));
    v.push_back(fin(boxAt(SHIP_OUTSIDE, FACE_NOTHING, dx - 0.05, 0, -PI / 2, 0.9, 0.05, hf + 2.3, 0.15, {BF_FRONT, BF_TOP, BF_BOTTOM, BF_SIDE_A, BF_SIDE_D}, {0.85, 0.95, 0.6, 0.7, 0.7}, true), FIN_OUTSIDE));
    return v;
}
const std::vector<ShipFixture>& fixtures() {
    static const std::vector<ShipFixture> v = makeFixtures();
    return v;
}

// ---- the walk's floors and colliders (made once from the rooms, the arches and the fixtures) -----------------------------------------

// where the walk may stand: a room's floor inset by the walker's width (a box, or the observatory's disc), or an arch's tunnel (its
// width inset, out into both rooms), which belongs to both
struct Floor { int room, arch; bool disc; double x0, x1, z0, z1; };

// an arch's faces across its wall (`from`'s and `to`'s) and the axis across it: true when the axis is x
bool archAcross(const ShipOpening& o, double& f, double& t) {
    const ShipRoom& F = ROOMS[o.from]; const ShipRoom& T = ROOMS[o.to];
    switch (o.side) {
        case SIDE_BOW: f = F.z1; t = T.z0; return false;
        case SIDE_STERN: f = F.z0; t = T.z1; return false;
        case SIDE_PORT: f = F.x0; t = T.x1; return true;
        default: f = F.x1; t = T.x0; return true;
    }
}

std::vector<Floor> makeFloors() {
    std::vector<Floor> v;
    for (int r = 0; r < SHIP_ROOMS; r++) {
        const ShipRoom& R = ROOMS[r];
        if (R.shape == RS_STAIR) continue;
        if (R.shape == RS_DISC) { v.push_back({r, -1, true, R.x0, R.x1 - 0.7, R.z0, 0}); continue; }
        if (R.shape == RS_BOW) {   // to the arc's start, and the bow's middle out to the window
            v.push_back({r, -1, false, R.x0 + WR, R.x1 - WR, R.z0 + WR, 10.7});
            v.push_back({r, -1, false, -3.2, 3.2, 10.7, 12.0});
            continue;
        }
        v.push_back({r, -1, false, R.x0 + WR, R.x1 - WR, R.z0 + WR, R.z1 - WR});
    }
    for (int i = 0; i < N_OPENINGS; i++) {
        const ShipOpening& o = OPENINGS[i];
        if (o.kind != OPEN_ARCH) continue;
        double f, t;
        const bool alongZ = archAcross(o, f, t);
        const double c0 = std::min(f, t) - WR - 0.1, c1 = std::max(f, t) + WR + 0.1;
        if (alongZ) v.push_back({o.from, i, false, c0, c1, o.lo + WR, o.hi - WR});
        else v.push_back({o.from, i, false, o.lo + WR, o.hi - WR, c0, c1});
    }
    return v;
}
const std::vector<Floor>& floors() {
    static const std::vector<Floor> v = makeFloors();
    return v;
}
bool floorOf(const Floor& f, int room) { return f.room == room || (f.arch >= 0 && (OPENINGS[f.arch].from == room || OPENINGS[f.arch].to == room)); }
bool inFloor(const Floor& f, double x, double z, double eps) {
    if (f.disc) return std::hypot(x - f.x0, z - f.z0) <= f.x1 + eps;
    return x >= f.x0 - eps && x <= f.x1 + eps && z >= f.z0 - eps && z <= f.z1 + eps;
}
void clampTo(const Floor& f, double& x, double& z) {
    if (f.disc) {
        const double dx = x - f.x0, dz = z - f.z0, d = std::hypot(dx, dz);
        if (d > f.x1) { x = f.x0 + dx / d * f.x1; z = f.z0 + dz / d * f.x1; }
        return;
    }
    x = clampd(x, f.x0, f.x1); z = clampd(z, f.z0, f.z1);
}

// a landing's angle on the circle (its unwrapped angle's place), the room it is in, the radius of that room's rail round the stair
double landingAt(int k) { return wrapAngle(landingA(k)); }
double railOf(int k) { return k == 0 ? STAIR.outer + WR : (k == 1 ? 1.85 + WR : 2.05 + WR); }

std::vector<ShipCollider> makeColliders() {
    std::vector<ShipCollider> v;
    for (int r = 0; r < SHIP_ROOMS; r++) {
        for (const ShipFixture& f : fixtures()) {
            if (f.room != r || f.open) continue;
            ShipCollider c{};
            c.room = r;
            if (f.kind == FX_CAGE) { c.kind = COLLIDE_CIRCLE; c.x = f.x[0]; c.z = f.z[0]; c.r = f.radius + WR; v.push_back(c); continue; }
            c.kind = COLLIDE_BOX;
            if (fixtureBoxLike(f.kind)) {
                c.x0 = std::min({f.x[0], f.x[1], f.x[2], f.x[3]}); c.x1 = std::max({f.x[0], f.x[1], f.x[2], f.x[3]});
                c.z0 = std::min({f.z[0], f.z[1], f.z[2], f.z[3]}); c.z1 = std::max({f.z[0], f.z[1], f.z[2], f.z[3]});
            } else if (f.kind == FX_RACK) { c.x0 = f.x0; c.x1 = f.x1; c.z0 = f.z0; c.z1 = f.z1; }
            else if (f.kind == FX_RING) { c.x0 = f.x[0] - f.radius; c.x1 = f.x[0] + f.radius; c.z0 = f.z[0] - f.depth / 2; c.z1 = f.z[0] + f.depth / 2; }
            else continue;
            c.x0 -= WR; c.x1 += WR; c.z0 -= WR; c.z1 += WR;
            v.push_back(c);
        }
        for (int k = 0; k < STAIR.levels; k++) {   // the rail round the stair's well, its foot or the oculus, open at its landing
            if (STAIR.room[k] != r) continue;
            ShipCollider c{};
            c.room = r; c.kind = COLLIDE_RING; c.x = STAIR.x; c.z = STAIR.z; c.r = railOf(k); c.gapAt = landingAt(k); c.gapHalf = STAIR.gap;
            v.push_back(c);
        }
    }
    return v;
}

// ---- the mesh --------------------------------------------------------------------------------------------------------------------------

// a point on a circle round (cx, cz) at the angle a (atan2(x, z)) and the height y
Vec3 onCircle(double cx, double cz, double r, double a, double y) { return Vec3(cx + r * std::sin(a), y, cz + r * std::cos(a)); }

struct Hole { double lo, hi, y0, y1; bool round; };

constexpr double LIGHT_CELL = 1.0;   // I-03: the light's grid across a room's surfaces and boxes (m): its lamps' light at a vertex a metre
constexpr double LAMP_FILL = 0.25;   // I-03: the share of a room's mean lamplight its surfaces give back, on every vertex of it

double comp(const Vec3& v, int k) { return k == 0 ? v.x : (k == 1 ? v.y : v.z); }
// a convex polygon's part on one side of a plane through `o` (the side `nrm` points to): Sutherland-Hodgman, its vertex count (31 at most)
int clipPlane(const Vec3* in, int n, const Vec3& o, const Vec3& nrm, Vec3* out) {
    int m = 0;
    for (int i = 0; i < n && m < 31; i++) {
        const Vec3& p = in[i]; const Vec3& q = in[(i + 1) % n];
        const double dp = dot(p - o, nrm), dq = dot(q - o, nrm);
        if (dp >= 0) out[m++] = p;
        if ((dp >= 0) != (dq >= 0) && m < 31) out[m++] = p + (q - p) * (dp / (dp - dq));
    }
    return m;
}
// the same against an axis's value: the part over it (`above`) or under it
int clipAxis(const Vec3* in, int n, int k, double t, bool above, Vec3* out) {
    Vec3 o(0, 0, 0), nrm(0, 0, 0);
    (k == 0 ? o.x : (k == 1 ? o.y : o.z)) = t;
    (k == 0 ? nrm.x : (k == 1 ? nrm.y : nrm.z)) = above ? 1 : -1;
    return clipPlane(in, n, o, nrm, out);
}
Vec3 newellOf(const Vec3* pts, int n) {
    Vec3 nrm(0, 0, 0);
    for (int i = 0; i < n; i++) {
        const Vec3& p = pts[i]; const Vec3& q = pts[(i + 1) % n];
        nrm.x += (p.y - q.y) * (p.z + q.z); nrm.y += (p.z - q.z) * (p.x + q.x); nrm.z += (p.x - q.x) * (p.y + q.y);
    }
    return nrm;
}
Vec3 middleOf(const Vec3* pts, int n) {
    Vec3 c(0, 0, 0);
    for (int i = 0; i < n; i++) c = c + pts[i];
    return c * (1.0 / n);
}
// a room's middle: what its surfaces are seen from (the bridge's to its bow, the disc's over its floor)
Vec3 roomMiddle(const ShipRoom& R) {
    if (R.shape == RS_DISC) return Vec3(R.x0, R.floor + 3, R.z0);
    return Vec3((R.x0 + R.x1) / 2, (R.floor + R.ceiling) / 2, (R.z0 + (R.shape == RS_BOW ? R.bowZ : R.z1)) / 2);
}
// a lamp's light on a place facing `nrm` (none: a line's, lit all round): its power by the face's turn toward it (wrapped a little), by
// its distance over its reach, a bulb's within its cone of straight down (and a little all round)
double lampOn(const ShipLamp& L, const Vec3& p, const Vec3* nrm) {
    const Vec3 d = Vec3(L.x, L.y, L.z) - p;
    const double dist = std::max(0.05, length(d));
    const Vec3 l = d * (1.0 / dist);
    const double face = nrm ? std::max(0.0, 0.15 + 0.85 * dot(*nrm, l)) : 1.0;
    const double fall = 1.0 / (1.0 + (dist / L.reach) * (dist / L.reach));
    const double spot = L.kind == LAMP_BULB ? std::max(0.12, smoothstep(std::cos(L.cone + 0.5), std::cos(L.cone), l.y)) : 1.0;
    return L.power * face * fall * spot;
}

struct Builder {
    ShipMesh m;
    int room = 0;
    bool flat = false;
    int finish = FIN_HULL;   // I-03: what is built's finish
    bool grid = false;       // I-03: its polygons cut to the light's grid (a room's surfaces and boxes, lit at their vertices by its lamps)
    bool cull = true;        // I-03: its faces turned from the eye left out
    Vec3 seen;               // I-03: a point on the side it is seen from (a face's normal turned toward it)
    // B-434: while set, each polygon built is also a surface the sun's patch falls on (a room with glass: its floor's, steps', ceiling's
    // and walls' pieces round their holes, so no patch lies over an arch, a window or the well), its normal facing in
    struct Receives { Vec3 nrm; double a; bool floor; };
    const Receives* receives = nullptr;
    void begin(int kind, int mat, int bank, int device, double a, double b) {
        ShipPiece p;
        p.room = room; p.kind = kind; p.mat = mat; p.finish = finish; p.bank = bank; p.device = device; p.a = a; p.b = b;
        p.first = (int)m.verts.size();
        m.pieces.push_back(p);
        if (mat != MAT_PANE && mat != MAT_SCREEN) m.finishes[room] |= 1u << finish;
    }
    // a vertex: the hull's and the solid's grain mapped along a wall (x + 0.31 z, y + 0.17 z, M2's) or across a floor (x, z)
    void vert(const Vec3& p) {
        ShipVert v; v.p = p;
        const int mat = m.pieces.back().mat;
        if (mat == MAT_HULL || mat == MAT_SOLID || mat == MAT_OUTSIDE || mat == MAT_FEATURE) {
            if (flat) { v.u = p.x; v.v = p.z; }
            else { v.u = p.x + p.z * 0.31; v.v = p.y + p.z * 0.17; }
        } else if (mat == MAT_SCREEN) v.u = p.x + p.z;
        m.verts.push_back(v);
        m.pieces.back().n++;
    }
    void emit(int mat, double a, double b, const Vec3* pts, int n, int bank, int device, const Vec3& nrm) {
        begin(PIECE_POLYGON, mat, bank, device, a, b);
        m.pieces.back().nrm = nrm; m.pieces.back().cull = cull;
        for (int i = 0; i < n; i++) vert(pts[i]);
    }
    // a polygon: its normal (Newell's) turned toward where it is seen from; I-03: a room's surface or box cut to the light's grid
    // (LIGHT_CELL squares at the ship's whole metres across its plane, none thinner than 5 cm), so its lamps' light lies in pools
    void poly(int mat, double a, double b, const Vec3* pts, int n, int bank = BANK_HULL, int device = FACE_NOTHING) {
        Vec3 nrm = newellOf(pts, n);
        const double len = length(nrm);
        if (len < 1e-12) return;
        if (receives) receiver(pts, n, receives->nrm, receives->a, receives->floor);
        nrm = nrm * (1.0 / len);
        if (dot(nrm, seen - middleOf(pts, n)) < 0) nrm = nrm * -1.0;
        flat = std::fabs(nrm.y) > 0.7;
        if (!grid || (mat != MAT_SOLID && mat != MAT_HULL && mat != MAT_TINT)) { emit(mat, a, b, pts, n, bank, device, nrm); return; }
        const int drop = std::fabs(nrm.x) >= std::fabs(nrm.y) && std::fabs(nrm.x) >= std::fabs(nrm.z) ? 0 : (std::fabs(nrm.y) >= std::fabs(nrm.z) ? 1 : 2);
        const int axis[2] = {drop == 0 ? 1 : 0, drop == 2 ? 1 : 2};
        double cuts[2][40]; int nc[2];
        for (int ax = 0; ax < 2; ax++) {
            double lo = 1e30, hi = -1e30;
            for (int i = 0; i < n; i++) { lo = std::min(lo, comp(pts[i], axis[ax])); hi = std::max(hi, comp(pts[i], axis[ax])); }
            int c = 0;
            cuts[ax][c++] = lo;
            for (int k = (int)std::ceil(lo / LIGHT_CELL); k * LIGHT_CELL < hi - 0.05 && c < 39; k++) if (k * LIGHT_CELL > lo + 0.05) cuts[ax][c++] = k * LIGHT_CELL;
            cuts[ax][c++] = hi;
            nc[ax] = c;
        }
        if (nc[0] == 2 && nc[1] == 2) { emit(mat, a, b, pts, n, bank, device, nrm); return; }
        for (int i = 0; i + 1 < nc[0]; i++)
            for (int j = 0; j + 1 < nc[1]; j++) {
                Vec3 w[32], t[32];
                int c = n;
                for (int q = 0; q < n; q++) w[q] = pts[q];
                auto cut = [&](int k, double v, bool above) { c = clipAxis(w, c, k, v, above, t); for (int q = 0; q < c; q++) w[q] = t[q]; };
                if (i > 0) cut(axis[0], cuts[0][i], true);
                if (i + 2 < nc[0] && c >= 3) cut(axis[0], cuts[0][i + 1], false);
                if (j > 0 && c >= 3) cut(axis[1], cuts[1][j], true);
                if (j + 2 < nc[1] && c >= 3) cut(axis[1], cuts[1][j + 1], false);
                if (c >= 3 && c <= 16 && length(newellOf(w, c)) > 2e-5) emit(mat, a, b, w, c, bank, device, nrm);
            }
    }
    void quad(int mat, double a, double b, const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, int bank = BANK_HULL, int device = FACE_NOTHING) {
        const Vec3 pts[4] = {p0, p1, p2, p3};
        poly(mat, a, b, pts, 4, bank, device);
    }
    void line(double a, const Vec3& p, const Vec3& q) {
        flat = false;
        begin(PIECE_LINE, MAT_PLAIN, BANK_HULL, FACE_NOTHING, a, 1);
        vert(p); vert(q);
    }
    // I-03: where the sun comes in (a window's light, a pane, the oculus, the dome) and a surface its patch falls on
    void glassOf(const Vec3* pts, int n, const Vec3& out, bool dome = false) {
        ShipGlass g; g.room = room; g.n = std::min(n, 16); g.out = out; g.dome = dome;
        for (int i = 0; i < g.n; i++) g.p[i] = pts[i];
        m.glass.push_back(g);
    }
    void receiver(const Vec3* pts, int n, const Vec3& nrm, double a, bool floor) {
        ShipReceiver r; r.room = room; r.n = std::min(n, 16); r.nrm = nrm; r.a = a; r.floor = floor;
        for (int i = 0; i < r.n; i++) r.p[i] = pts[i];
        m.receivers.push_back(r);
    }

    // a rectangle of floor or ceiling at y (nothing when empty)
    void rect(int mat, double shade, double x0, double x1, double z0, double z1, double y) {
        if (x1 - x0 < 1e-6 || z1 - z0 < 1e-6) return;
        quad(mat, shade, 1, Vec3(x0, y, z0), Vec3(x1, y, z0), Vec3(x1, y, z1), Vec3(x0, y, z1));
    }
    // I-04: a rectangle of floor less a fixture's footprint where they meet (the core's pit in the hall's floor)
    void rectCut(int mat, double shade, double x0, double x1, double z0, double z1, double y, const ShipFixture* cut) {
        if (!cut) { rect(mat, shade, x0, x1, z0, z1, y); return; }
        const double cx0 = std::min({cut->x[0], cut->x[1], cut->x[2], cut->x[3]}), cx1 = std::max({cut->x[0], cut->x[1], cut->x[2], cut->x[3]});
        const double cz0 = std::min({cut->z[0], cut->z[1], cut->z[2], cut->z[3]}), cz1 = std::max({cut->z[0], cut->z[1], cut->z[2], cut->z[3]});
        if (cx1 <= x0 || cx0 >= x1 || cz1 <= z0 || cz0 >= z1) { rect(mat, shade, x0, x1, z0, z1, y); return; }
        rect(mat, shade, x0, x1, std::max(z0, cz1), z1, y); rect(mat, shade, x0, x1, z0, std::min(z1, cz0), y);
        rect(mat, shade, x0, std::max(x0, cx0), std::max(z0, cz0), std::min(z1, cz1), y); rect(mat, shade, std::min(x1, cx1), x1, std::max(z0, cz0), std::min(z1, cz1), y);
    }
    // a floor or ceiling round a round hole: the rectangle outside the hole's square, and the square round its circle (the two share the
    // square's sides, the circle's sixteen points are the rim's and the landings'); I-04: less a pit's footprint
    void slab(int mat, double shade, double x0, double x1, double z0, double z1, double y, const ShipOpening* hole, const ShipFixture* cut = nullptr) {
        if (!hole) { rectCut(mat, shade, x0, x1, z0, z1, y, cut); return; }
        const double r = hole->r, hx0 = hole->x - r, hx1 = hole->x + r, hz0 = hole->z - r, hz1 = hole->z + r;
        rectCut(mat, shade, x0, x1, hz1, z1, y, cut); rectCut(mat, shade, x0, x1, z0, hz0, y, cut);
        rectCut(mat, shade, x0, hx0, hz0, hz1, y, cut); rectCut(mat, shade, hx1, x1, hz0, hz1, y, cut);
        for (int k = 0; k < ROUND; k++) {
            const double a0 = k * TAU / ROUND, a1 = (k + 1) * TAU / ROUND;
            const double m0 = std::max(std::fabs(std::sin(a0)), std::fabs(std::cos(a0))), m1 = std::max(std::fabs(std::sin(a1)), std::fabs(std::cos(a1)));
            quad(mat, shade, 1, onCircle(hole->x, hole->z, r, a0, y), onCircle(hole->x, hole->z, r / m0, a0, y), onCircle(hole->x, hole->z, r / m1, a1, y), onCircle(hole->x, hole->z, r, a1, y));
        }
    }
    // a hole's rim between two heights (the slab's edge, facing the hole's middle)
    void rim(double cx, double cz, double r, double y0, double y1, double shade) {
        const Vec3 keep = seen;
        seen = Vec3(cx, (y0 + y1) / 2, cz);
        for (int k = 0; k < ROUND; k++) {
            const double a0 = k * TAU / ROUND, a1 = (k + 1) * TAU / ROUND;
            quad(MAT_SOLID, shade, 1, onCircle(cx, cz, r, a0, y1), onCircle(cx, cz, r, a1, y1), onCircle(cx, cz, r, a1, y0), onCircle(cx, cz, r, a0, y0));
        }
        seen = keep;
    }
    // a rail round a hole at radius r over the floor at y: its posts at the sixteen points (none within the landing's way off), its rails at
    // waist and hand height between the posts there are; I-03: in brass
    void rail(double cx, double cz, double r, double y, double gapAt, double gapHalf) {
        const int keep = finish;
        finish = FIN_BRASS;
        bool post[ROUND];
        for (int k = 0; k < ROUND; k++) {
            post[k] = std::fabs(wrapAngle(k * TAU / ROUND - gapAt)) > gapHalf - 0.05;
            if (post[k]) line(0.95, onCircle(cx, cz, r, k * TAU / ROUND, y), onCircle(cx, cz, r, k * TAU / ROUND, y + 0.95));
        }
        for (int k = 0; k < ROUND; k++) {
            const int j = (k + 1) % ROUND;
            if (!post[k] || !post[j]) continue;
            for (double h : {0.5, 0.95}) line(h > 0.6 ? 1.0 : 0.85, onCircle(cx, cz, r, k * TAU / ROUND, y + h), onCircle(cx, cz, r, j * TAU / ROUND, y + h));
        }
        finish = keep;
    }

    // a box room's wall on a side: from a0 to a1 along it and y0 to y1 up it, round its holes (arches, windows; a round head's strips)
    static Vec3 wallPoint(const ShipRoom& R, int side, double along, double y) {
        switch (side) {
            case SIDE_BOW: return Vec3(along, y, R.z1);
            case SIDE_STERN: return Vec3(along, y, R.z0);
            case SIDE_PORT: return Vec3(R.x0, y, along);
            default: return Vec3(R.x1, y, along);
        }
    }
    // a side's way into its room
    static Vec3 inward(int side) {
        switch (side) {
            case SIDE_BOW: return Vec3(0, 0, -1);
            case SIDE_STERN: return Vec3(0, 0, 1);
            case SIDE_PORT: return Vec3(1, 0, 0);
            default: return Vec3(-1, 0, 0);
        }
    }
    void wall(const ShipRoom& R, int side, double a0, double a1, double y0, double y1, std::vector<Hole> holes, int mat, double shade) {
        std::sort(holes.begin(), holes.end(), [](const Hole& a, const Hole& b) { return a.lo < b.lo; });
        auto P = [&](double along, double y) { return wallPoint(R, side, along, y); };
        auto full = [&](double u0, double u1) { if (u1 - u0 > 1e-6) quad(mat, shade, 1, P(u0, y0), P(u1, y0), P(u1, y1), P(u0, y1)); };
        double cur = a0;
        for (const Hole& h : holes) {
            full(cur, h.lo);
            if (h.y0 > y0 + 1e-6) quad(mat, shade, 1, P(h.lo, y0), P(h.hi, y0), P(h.hi, h.y0), P(h.lo, h.y0));
            if (h.round) {
                const double rr = (h.hi - h.lo) / 2, mid = (h.lo + h.hi) / 2, spring = h.y1 - rr;
                for (int k = 0; k < HEAD; k++) {
                    const double t0 = PI - PI * k / HEAD, t1 = PI - PI * (k + 1) / HEAD;
                    const double u0 = mid + rr * std::cos(t0), v0 = spring + rr * std::sin(t0), u1 = mid + rr * std::cos(t1), v1 = spring + rr * std::sin(t1);
                    if (y1 - std::min(v0, v1) > 1e-6) quad(mat, shade, 1, P(u0, v0), P(u1, v1), P(u1, y1), P(u0, y1));
                }
            } else if (h.y1 < y1 - 1e-6) quad(mat, shade, 1, P(h.lo, h.y1), P(h.hi, h.y1), P(h.hi, y1), P(h.lo, y1));
            cur = h.hi;
        }
        full(cur, a1);
    }
    // an arch's tunnel through its wall's slab: its jambs, its head, its floor (`from`'s)
    void archReveal(const ShipOpening& o) {
        const ShipRoom& F = ROOMS[o.from];
        double f, t;
        const bool alongZ = archAcross(o, f, t);
        auto Q = [&](double along, double y, double c) { return alongZ ? Vec3(c, y, along) : Vec3(along, y, c); };
        const double y0 = F.floor, rr = (o.hi - o.lo) / 2, mid = (o.lo + o.hi) / 2, spring = y0 + o.height - rr;
        const Vec3 keep = seen;
        seen = Q(mid, y0 + o.height / 2, (f + t) / 2);
        for (double u : {o.lo, o.hi}) quad(MAT_SOLID, 0.75, 1, Q(u, y0, f), Q(u, y0, t), Q(u, spring, t), Q(u, spring, f));
        for (int k = 0; k < HEAD; k++) {
            const double t0 = PI - PI * k / HEAD, t1 = PI - PI * (k + 1) / HEAD;
            const double u0 = mid + rr * std::cos(t0), v0 = spring + rr * std::sin(t0), u1 = mid + rr * std::cos(t1), v1 = spring + rr * std::sin(t1);
            quad(MAT_SOLID, 0.68, 1, Q(u0, v0, f), Q(u1, v1, f), Q(u1, v1, t), Q(u0, v0, t));
        }
        const int keepFinish = finish;
        finish = F.floorFinish;
        quad(MAT_SOLID, F.floorShade, 1, Q(o.lo, y0, f), Q(o.hi, y0, f), Q(o.hi, y0, t), Q(o.lo, y0, t));
        finish = keepFinish;
        seen = keep;
    }

    const ShipOpening* holeIn(int kind, int roomFrom, int roomTo) {
        for (const ShipOpening& o : OPENINGS) if (o.kind == kind && (roomFrom < 0 || o.from == roomFrom) && (roomTo < 0 || o.to == roomTo)) return &o;
        return nullptr;
    }
    std::vector<Hole> holesIn(int r, int side) {
        std::vector<Hole> h;
        for (const ShipOpening& o : OPENINGS) {
            if (o.kind != OPEN_ARCH) continue;
            if ((o.from == r && o.side == side) || (o.to == r && (o.side + 2) % 4 == side)) h.push_back({o.lo, o.hi, ROOMS[o.from].floor, ROOMS[o.from].floor + o.height, o.round});
        }
        for (const ShipWindow& w : WINDOWS) if (w.room == r && w.side == side) h.push_back({w.lo, w.hi, w.y0, w.y1, false});
        return h;
    }
    int wallMat(const ShipRoom& R, int side) const { return (R.outer & (1 << side)) ? MAT_HULL : MAT_SOLID; }
    static const ShipFixture* fixtureIn(int r, int kind) {
        for (const ShipFixture& f : fixtures()) if (f.room == r && f.kind == kind) return &f;
        return nullptr;
    }
    bool hasGlass(int r) const {
        for (const ShipWindow& w : WINDOWS) if (w.room == r) return true;
        for (const ShipOpening& o : OPENINGS) if (o.kind == OPEN_OCULUS && o.from == r) return true;
        return ROOMS[r].shape == RS_BOW || ROOMS[r].shape == RS_DISC;
    }

    // a box room: its floor and ceiling (round the well or the oculus), its four walls round their holes, its arches' tunnels and holes'
    // rims and rails, its windows' mullions and panes; I-03: the surfaces the sun's patches fall on when it has glass (B-434: their pieces
    // round the holes). I-04: its floor round the cradle's hatch (the hangar's) and less the core's pit (the hall's)
    void boxRoom(int r) {
        const ShipRoom& R = ROOMS[r];
        const bool glass = hasGlass(r);
        const Receives onFloor{Vec3(0, 1, 0), R.floorShade, true}, onCeiling{Vec3(0, -1, 0), R.ceilingShade, false};
        finish = R.floorFinish;
        const ShipFixture* cage = fixtureIn(r, FX_CAGE);
        ShipOpening hatch{};
        if (cage) { hatch.x = cage->x[0]; hatch.z = cage->z[0]; hatch.r = cage->ringR; }
        const ShipOpening* well = holeIn(OPEN_WELL, r, -1);
        receives = glass ? &onFloor : nullptr;
        slab((R.outer & OUT_FLOOR) ? MAT_HULL : MAT_SOLID, R.floorShade, R.x0, R.x1, R.z0, R.z1, R.floor, well ? well : (cage ? &hatch : nullptr), fixtureIn(r, FX_CORE));
        receives = nullptr;
        if (cage) rim(hatch.x, hatch.z, hatch.r, R.floor - 0.4, R.floor, 0.5);   // the hatch's shaft through the hull, under its leaves
        const ShipOpening* above = holeIn(OPEN_OCULUS, r, -1);
        finish = R.ceilingFinish;
        receives = glass ? &onCeiling : nullptr;
        slab((R.outer & OUT_CEILING) ? MAT_HULL : MAT_SOLID, R.ceilingShade, R.x0, R.x1, R.z0, R.z1, R.ceiling, above ? above : holeIn(OPEN_WELL, -1, r));
        finish = R.wallFinish;
        for (int s = 0; s < 4; s++) {
            const bool alongX = s == SIDE_BOW || s == SIDE_STERN;
            const Receives onWall{inward(s), R.wallShade[s], false};
            receives = glass ? &onWall : nullptr;
            wall(R, s, alongX ? R.x0 : R.z0, alongX ? R.x1 : R.z1, R.floor, R.ceiling, holesIn(r, s), wallMat(R, s), R.wallShade[s]);
        }
        receives = nullptr;
        roomWays(r);
        windows(r);
    }
    void roomWays(int r) {
        for (const ShipOpening& o : OPENINGS) {
            if (o.from != r) continue;
            if (o.kind == OPEN_ARCH) archReveal(o);
            else if (o.kind == OPEN_WELL) { rim(o.x, o.z, o.r, ROOMS[o.to].ceiling, ROOMS[r].floor, 0.6); rail(o.x, o.z, 1.85, ROOMS[r].floor, landingAt(1), STAIR.landing); }
            else if (o.kind == OPEN_OCULUS) {
                rim(o.x, o.z, o.r, ROOMS[r].ceiling, ROOMS[o.to].floor, 0.6);
                Vec3 pts[ROUND];
                for (int k = 0; k < ROUND; k++) pts[k] = onCircle(o.x, o.z, o.r, k * TAU / ROUND, ROOMS[r].ceiling);
                glassOf(pts, ROUND, Vec3(0, 1, 0));   // I-03: the sun down through the dome and the oculus
            }
        }
    }
    // a box room's windows: the mullions, then the glass (to depth only); I-03: each light between the mullions where the sun comes in
    void windows(int r) {
        const ShipRoom& R = ROOMS[r];
        for (const ShipWindow& w : WINDOWS) {
            if (w.room != r) continue;
            auto P = [&](double along, double y) { return wallPoint(R, w.side, along, y); };
            for (int k = 1; k <= w.mullions; k++) {   // the mullions before the glass (it is written to depth only)
                const double u = w.lo + (w.hi - w.lo) * k / (w.mullions + 1);
                quad(wallMat(R, w.side), R.wallShade[w.side], 0.7, P(u - 0.04, w.y0), P(u + 0.04, w.y0), P(u + 0.04, w.y1), P(u - 0.04, w.y1));
            }
            quad(MAT_PANE, 1, 1, P(w.lo, w.y0), P(w.hi, w.y0), P(w.hi, w.y1), P(w.lo, w.y1));
            for (int k = 0; k <= w.mullions; k++) {
                const double u0 = w.lo + (w.hi - w.lo) * k / (w.mullions + 1) + (k > 0 ? 0.04 : 0), u1 = w.lo + (w.hi - w.lo) * (k + 1) / (w.mullions + 1) - (k < w.mullions ? 0.04 : 0);
                const Vec3 pts[4] = {P(u0, w.y0), P(u1, w.y0), P(u1, w.y1), P(u0, w.y1)};
                glassOf(pts, 4, inward(w.side) * -1.0);
            }
        }
    }

    // the bridge: the floor under its steps, the steps, the platform out to the great window's arc, the ceiling, its three walls (the
    // hall's arch in its stern wall), the arc's parapets and heads, its mullions and its seven panes; I-03: each pane (inside its mullions)
    // where the sun comes in, its floors, steps, ceiling and walls where its patches fall (B-434: each step's own, the walls' pieces round
    // the arch)
    void bowRoom(int r) {
        const ShipRoom& R = ROOMS[r];
        const double h = R.bowZ - R.z1, rad = (h * h + R.x1 * R.x1) / (2 * h), cz = R.bowZ - rad, bmax = std::asin(R.x1 / rad);
        Vec3 q[16];
        const int np = std::min(R.bowPanes, 14);
        for (int k = 0; k <= np; k++) { const double b = -bmax + 2 * bmax * k / np; q[k] = Vec3(rad * std::sin(b), 0, cz + rad * std::cos(b)); }
        const int fm = (R.outer & OUT_FLOOR) ? MAT_HULL : MAT_SOLID, cm = (R.outer & OUT_CEILING) ? MAT_HULL : MAT_SOLID;
        const double zp = R.z0 + R.steps * R.stepDepth;
        finish = R.floorFinish;
        const Receives onFloor{Vec3(0, 1, 0), R.floorShade, true}, onStep{Vec3(0, 1, 0), 0.8, true};
        receives = &onFloor;
        rect(fm, R.floorShade, R.x0, R.x1, R.z0, R.z0 + R.stepDepth, R.stepFloor);
        receives = &onStep;
        for (int i = 1; i < R.steps; i++) rect(MAT_SOLID, 0.8, R.x0, R.x1, R.z0 + i * R.stepDepth, R.z0 + (i + 1) * R.stepDepth, R.stepFloor + i * R.stepRise);
        receives = nullptr;
        const Vec3 keep = seen;
        seen = Vec3(0, R.stepFloor + 0.1, R.z0 - 1.0);   // the risers face the arch
        for (int i = 1; i <= R.steps; i++) {
            const double z = R.z0 + i * R.stepDepth, ya = R.stepFloor + (i - 1) * R.stepRise, yb = i == R.steps ? R.floor : R.stepFloor + i * R.stepRise;
            quad(MAT_SOLID, 0.72, 1, Vec3(R.x0, ya, z), Vec3(R.x1, ya, z), Vec3(R.x1, yb, z), Vec3(R.x0, yb, z));
        }
        seen = keep;
        Vec3 plan[18]; int npl = 0;   // the room's plan: its stern corners, then the arc from starboard to port
        auto outline = [&](double z0, double y, int mat, double shade) {
            Vec3 pts[18]; int n = 0;
            pts[n++] = Vec3(R.x0, y, z0); pts[n++] = Vec3(R.x1, y, z0);
            for (int k = np; k >= 0; k--) pts[n++] = Vec3(q[k].x, y, q[k].z);
            poly(mat, shade, 1, pts, n);
            for (int i = 0; i < n; i++) plan[i] = pts[i];
            npl = n;
        };
        outline(zp, R.floor, fm, R.floorShade);
        receiver(plan, npl, Vec3(0, 1, 0), R.floorShade, true);
        finish = R.ceilingFinish;
        outline(R.z0, R.ceiling, cm, R.ceilingShade);
        receiver(plan, npl, Vec3(0, -1, 0), R.ceilingShade, false);
        finish = R.wallFinish;
        for (int s : {SIDE_STERN, SIDE_STARBOARD, SIDE_PORT}) {
            const Receives onWall{inward(s), R.wallShade[s], false};
            receives = &onWall;
            if (s == SIDE_STERN) wall(R, s, R.x0, R.x1, R.stepFloor, R.ceiling, holesIn(r, s), wallMat(R, s), R.wallShade[s]);
            else wall(R, s, R.z0, R.z1, R.stepFloor, R.ceiling, holesIn(r, s), wallMat(R, s), R.wallShade[s]);
        }
        receives = nullptr;
        const int am = wallMat(R, SIDE_BOW);
        const double sh = R.wallShade[SIDE_BOW];
        auto A = [&](int k, double y) { return Vec3(q[k].x, y, q[k].z); };
        for (int k = 0; k < np; k++) {
            quad(am, sh, 1, A(k, R.floor), A(k + 1, R.floor), A(k + 1, R.bowLo), A(k, R.bowLo));
            quad(am, sh, 0.92, A(k, R.bowHi), A(k + 1, R.bowHi), A(k + 1, R.ceiling), A(k, R.ceiling));
        }
        for (int k = 0; k <= np; k++) {   // the mullions, a little inside the glass
            const double b = -bmax + 2 * bmax * k / np, tx = std::cos(b) * 0.07, tz = -std::sin(b) * 0.07, ix = -std::sin(b) * 0.03, iz = -std::cos(b) * 0.03;
            const Vec3 c(q[k].x + ix, 0, q[k].z + iz);
            quad(am, sh, 0.7, Vec3(c.x - tx, R.bowLo, c.z - tz), Vec3(c.x + tx, R.bowLo, c.z + tz), Vec3(c.x + tx, R.bowHi, c.z + tz), Vec3(c.x - tx, R.bowHi, c.z - tz));
        }
        for (int k = 0; k < np; k++) {
            quad(MAT_PANE, 1, 1, A(k, R.bowLo), A(k + 1, R.bowLo), A(k + 1, R.bowHi), A(k, R.bowHi));
            const Vec3 e = A(k + 1, 0) - A(k, 0), d = e * (0.07 / length(e)), mid = (A(k, 0) + A(k + 1, 0)) * 0.5;
            const Vec3 pts[4] = {A(k, R.bowLo) + d, A(k + 1, R.bowLo) - d, A(k + 1, R.bowHi) - d, A(k, R.bowHi) + d};
            glassOf(pts, 4, normalize(Vec3(mid.x, 0, mid.z - cz)));
        }
        roomWays(r);
    }

    // the observatory: its floor round the oculus, its parapet, the dome's ribs and then its glass (to depth only), the oculus's rail; I-03:
    // the dome where the sun comes in, the floor's sixteen sectors where its light falls
    void discRoom(int r) {
        const ShipRoom& R = ROOMS[r];
        const ShipOpening* oc = holeIn(OPEN_OCULUS, -1, r);
        const double rin = oc ? oc->r : 0, rad = R.x1, cx = R.x0, cz = R.z0, y = R.floor;
        const int n = R.domeN;
        const double e0 = std::asin(R.parapet / rad), rb = rad * std::cos(e0);
        for (int k = 0; k < n; k++) {
            const double a0 = k * TAU / n, a1 = (k + 1) * TAU / n;
            finish = R.floorFinish;
            quad(MAT_SOLID, R.floorShade, 1, onCircle(cx, cz, rin, a0, y), onCircle(cx, cz, rad, a0, y), onCircle(cx, cz, rad, a1, y), onCircle(cx, cz, rin, a1, y));
            const Vec3 sector[4] = {onCircle(cx, cz, rin, a0, y), onCircle(cx, cz, rad, a0, y), onCircle(cx, cz, rad, a1, y), onCircle(cx, cz, rin, a1, y)};
            receiver(sector, 4, Vec3(0, 1, 0), R.floorShade, true);
            finish = R.wallFinish;
            quad((R.outer & OUT_BOW) ? MAT_HULL : MAT_SOLID, R.wallShade[SIDE_BOW], 0.85, onCircle(cx, cz, rad, a0, y), onCircle(cx, cz, rad, a1, y), onCircle(cx, cz, rb, a1, y + R.parapet),
                 onCircle(cx, cz, rb, a0, y + R.parapet));
        }
        auto D = [&](double e, double a, double s) { return onCircle(cx, cz, rad * s * std::cos(e), a, y + rad * s * std::sin(e)); };
        const int rings = R.domeRings;
        auto E = [&](int j) { return e0 + (PI / 2 - e0) * j / rings; };
        for (int k = 0; k < n; k++)
            for (int j = 0; j < rings; j++) {
                const double a = k * TAU / n, a1 = (k + 1) * TAU / n;
                line(0.9, D(E(j), a, 0.99), D(E(j + 1), a, 0.99));
                line(0.9, D(E(j), a, 0.99), D(E(j), a1, 0.99));
            }
        for (int k = 0; k < n; k++)
            for (int j = 0; j < rings; j++) {
                const double a0 = k * TAU / n, a1 = (k + 1) * TAU / n;
                if (j == rings - 1) { const Vec3 tri[3] = {D(E(j), a0, 1), D(E(j), a1, 1), D(PI / 2, 0, 1)}; poly(MAT_PANE, 1, 1, tri, 3); }
                else quad(MAT_PANE, 1, 1, D(E(j), a0, 1), D(E(j), a1, 1), D(E(j + 1), a1, 1), D(E(j + 1), a0, 1));
            }
        const Vec3 none[3] = {Vec3(cx, y, cz), Vec3(cx, y, cz), Vec3(cx, y, cz)};
        glassOf(none, 3, Vec3(0, 1, 0), true);
        if (oc) rail(cx, cz, 2.05, y, landingAt(2), STAIR.landing);
        roomWays(r);
    }

    // the spiral stair: its column, the hall's and the observatory's landings (each out to its hole's edge in the landing's two
    // sixteenths), and between the levels its steps (a tread, its underside, its outer edge, the riser up to it) and its handrail (brass)
    void stair() {
        const ShipStair& S = STAIR;
        const double c = S.column, o = S.outer, th = 0.12;
        auto C = [&](double r, double a, double y) { return onCircle(S.x, S.z, r, a, y); };
        finish = ROOMS[ROOM_STAIR].wallFinish;
        for (int k = 0; k < ROUND; k++) {
            const double a0 = k * TAU / ROUND, a1 = (k + 1) * TAU / ROUND;
            seen = C(2 * c + 1, (a0 + a1) / 2, (S.level[0] + S.level[2]) / 2);
            quad(MAT_SOLID, 0.8, 1, C(c, a0, S.level[0]), C(c, a1, S.level[0]), C(c, a1, S.level[2] + 0.95), C(c, a0, S.level[2] + 0.95));
        }
        for (int k = 1; k < S.levels; k++) {
            const double A = landingA(k), y = S.level[k], edge = k == 1 ? 1.8 : 2.0;
            for (double yy : {y, y - th}) {
                finish = yy == y ? ROOMS[ROOM_STAIR].floorFinish : ROOMS[ROOM_STAIR].wallFinish;
                seen = C(1, A, yy == y ? y + 2 : y - 2);
                const Vec3 pts[5] = {C(c, A - S.landing, yy), C(edge, A - S.landing, yy), C(edge, A, yy), C(edge, A + S.landing, yy), C(c, A + S.landing, yy)};
                poly(MAT_SOLID, yy == y ? 0.95 : 0.55, 1, pts, 5);
            }
        }
        for (int k = 0; k + 1 < S.levels; k++) {
            const int n = S.steps[k];
            const double rise = (S.level[k + 1] - S.level[k]) / n, a0 = landingA(k) + S.landing;
            double yPrev = S.level[k] - (k == 0 ? 0 : th);
            Vec3 railPrev;
            for (int i = 0; i < n; i++) {
                const double s0 = a0 + i * S.step, s1 = s0 + S.step, y = S.level[k] + (i + 0.5) * rise, mid = (s0 + s1) / 2;
                finish = ROOMS[ROOM_STAIR].floorFinish;
                seen = C(1, mid, y + 2);
                quad(MAT_SOLID, 0.95, 1, C(c, s0, y), C(o, s0, y), C(o, s1, y), C(c, s1, y));
                finish = ROOMS[ROOM_STAIR].wallFinish;
                seen = C(1, mid, y - 2);
                quad(MAT_SOLID, 0.55, 1, C(c, s0, y - th), C(o, s0, y - th), C(o, s1, y - th), C(c, s1, y - th));
                seen = C(o + 1, mid, y);
                quad(MAT_SOLID, 0.7, 1, C(o, s0, y - th), C(o, s1, y - th), C(o, s1, y), C(o, s0, y));
                seen = C(1, s0 - 0.3, y);
                quad(MAT_SOLID, 0.75, 1, C(c, s0, i == 0 ? yPrev : yPrev - th), C(o, s0, i == 0 ? yPrev : yPrev - th), C(o, s0, y), C(c, s0, y));
                finish = FIN_BRASS;
                const Vec3 rp = C(o - 0.06, mid, y + 0.95);
                if (i % 2 == 0) line(0.9, C(o - 0.06, mid, y), rp);
                if (i > 0) line(1.0, railPrev, rp);
                railPrev = rp; yPrev = y;
            }
            const double s = a0 + n * S.step;   // up to the next landing
            finish = ROOMS[ROOM_STAIR].wallFinish;
            seen = C(1, s - 0.3, yPrev);
            quad(MAT_SOLID, 0.75, 1, C(c, s, yPrev - th), C(o, s, yPrev - th), C(o, s, S.level[k + 1]), C(c, s, S.level[k + 1]));
        }
    }

    // I-04: a piece the frame fills where it comes in its room's order (its fixture's), in a finish (what it asks a bank for)
    void marker(int kind, int index, int fin) {
        const int keep = finish;
        finish = fin; flat = false;
        begin(kind, MAT_SOLID, BANK_HULL, FACE_NOTHING, 1, 1);
        m.pieces.back().fixture = index;
        finish = keep;
    }
    // I-04: the shards' shelves' carcass: its back inside, its sides (both their faces), its top (its underside too), its boards (each a top,
    // an underside and a front edge) and the plinth's front; then the shards the frame stands on its boards
    void shelf(const ShipFixture& F, int index) {
        const Vec3 A[4] = {Vec3(F.x[0], F.y[0], F.z[0]), Vec3(F.x[1], F.y[1], F.z[1]), Vec3(F.x[2], F.y[2], F.z[2]), Vec3(F.x[3], F.y[3], F.z[3])};
        auto P = [&](double t, double back, double y) { return A[1] + (A[2] - A[1]) * t + ((A[0] + (A[3] - A[0]) * t) - (A[1] + (A[2] - A[1]) * t)) * back + Vec3(0, y, 0); };
        const Vec3 front = normalize(A[1] - A[0]), mid = P(0.5, 0.5, F.height / 2);
        const double H = F.height;
        auto quadSeen = [&](const Vec3& from, double shade, const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3) { seen = from; quad(MAT_SOLID, shade, 1, p0, p1, p2, p3); };
        quadSeen(mid + front * 3.0, 0.55, P(0, 0.96, 0), P(1, 0.96, 0), P(1, 0.96, H), P(0, 0.96, H));   // the back, inside (off the wall's plane)
        for (int e = 0; e < 2; e++) {   // the sides, out and in
            const double t = e;
            const Vec3 side = normalize(P(t, 0.5, H / 2) - P(1 - t, 0.5, H / 2));
            quadSeen(P(t, 0.5, H / 2) + side * 3.0, 0.7, P(t, 0, 0), P(t, 1, 0), P(t, 1, H), P(t, 0, H));
            quadSeen(P(t, 0.5, H / 2) - side * 0.5, 0.6, P(t, 0, 0), P(t, 1, 0), P(t, 1, H), P(t, 0, H));
        }
        quadSeen(P(0.5, 0.5, H + 3.0), 0.9, P(0, 0, H), P(1, 0, H), P(1, 1, H), P(0, 1, H));   // the top, and under it
        quadSeen(P(0.5, 0.5, H - 1.0), 0.5, P(0, 0, H - 0.03), P(1, 0, H - 0.03), P(1, 1, H - 0.03), P(0, 1, H - 0.03));
        quadSeen(P(0.5, -3.0, 0.05), 0.7, P(0, 0, 0), P(1, 0, 0), P(1, 0, 0.1), P(0, 0, 0.1));   // the plinth
        for (int k = 0; k < F.boards; k++) {
            const double y = 0.1 + k * (H - 0.1) / F.boards;
            quadSeen(P(0.5, 0.5, y + 3.0), 0.85, P(0, 0, y), P(1, 0, y), P(1, 1, y), P(0, 1, y));
            if (k > 0) {
                quadSeen(P(0.5, 0.5, y - 2.0), 0.5, P(0, 0, y - 0.03), P(1, 0, y - 0.03), P(1, 1, y - 0.03), P(0, 1, y - 0.03));
                quadSeen(P(0.5, -3.0, y), 0.8, P(0, 0, y - 0.03), P(1, 0, y - 0.03), P(1, 0, y), P(0, 0, y));
            }
        }
        if (hasGlass(F.room)) { const Vec3 top[4] = {P(0, 0, H), P(1, 0, H), P(1, 1, H), P(0, 1, H)}; receiver(top, 4, Vec3(0, 1, 0), 0.9, false); }
        marker(PIECE_SHELF, index, F.finish);
    }
    // I-04: the drive's core's pit under the hall's floor glass: its walls and floor (the hull's grey), the core and the glass the frame's
    void core(const ShipFixture& F, int index) {
        const double x0 = std::min({F.x[0], F.x[1], F.x[2], F.x[3]}), x1 = std::max({F.x[0], F.x[1], F.x[2], F.x[3]});
        const double z0 = std::min({F.z[0], F.z[1], F.z[2], F.z[3]}), z1 = std::max({F.z[0], F.z[1], F.z[2], F.z[3]});
        const double y0 = F.y[0], y1 = F.y[0] + F.height;
        const int keep = finish;
        finish = FIN_HULL;
        seen = Vec3((x0 + x1) / 2, y0 + 0.5 * (y1 - y0), (z0 + z1) / 2);
        quad(MAT_SOLID, 0.5, 1, Vec3(x0, y0, z0), Vec3(x1, y0, z0), Vec3(x1, y1, z0), Vec3(x0, y1, z0));
        quad(MAT_SOLID, 0.5, 1, Vec3(x0, y0, z1), Vec3(x1, y0, z1), Vec3(x1, y1, z1), Vec3(x0, y1, z1));
        quad(MAT_SOLID, 0.45, 1, Vec3(x0, y0, z0), Vec3(x0, y0, z1), Vec3(x0, y1, z1), Vec3(x0, y1, z0));
        quad(MAT_SOLID, 0.45, 1, Vec3(x1, y0, z0), Vec3(x1, y0, z1), Vec3(x1, y1, z1), Vec3(x1, y1, z0));
        seen = Vec3((x0 + x1) / 2, y1 + 2, (z0 + z1) / 2);
        quad(MAT_SOLID, 0.35, 1, Vec3(x0, y0, z0), Vec3(x1, y0, z0), Vec3(x1, y0, z1), Vec3(x0, y0, z1));
        marker(PIECE_CORE, index, FIN_GLASS);   // the core and the floor glass over it (clear: the frame's)
        finish = keep;
    }

    // what stands in a room: a box's faces, a screen, the cage, a tube, a ring; the rack where it comes; I-03: in its finish (a box's top in
    // its own), outside the hull lit by the sun (the ring's inner face aglow), the mast where it comes; a box's top in a room with glass a
    // surface the sun's patches fall on. I-04: the shards' shelves, a bed's plants, the deck's reels, the locker's doors, the photographs,
    // the case's instruments, the drive's core, the capsule and the hatch (`marker`: the frame fills them); the drive's ring aglow aboard too
    void fixture(const ShipFixture& F, int index) {
        const bool out = F.room == SHIP_OUTSIDE;
        const int solidMat = out ? MAT_FEATURE : (F.finish == FIN_GLASS ? MAT_TINT : MAT_SOLID);
        finish = F.finish;
        if (F.kind == FX_SHELF) { shelf(F, index); return; }
        if (F.kind == FX_CORE) { core(F, index); return; }
        if (F.kind == FX_PRINTS) { marker(PIECE_PRINTS, index, FIN_PRINT); m.finishes[F.room] |= 1u << FIN_DECK; return; }
        if (F.kind == FX_DISPLAY) { marker(PIECE_DISPLAY, index, FIN_BRASS); return; }
        if (F.kind == FX_TREE) { seen = Vec3((F.x[1] + F.x[2]) / 2, F.y[1] + F.height / 2, (F.z[1] + F.z[2]) / 2); marker(PIECE_TREE, index, FIN_PANEL); return; }   // Z-06
        if (F.kind == FX_VEHICLE) { seen = Vec3(F.x[0], F.y[0] + 1, F.z[0]); marker(PIECE_VEHICLE, index, FIN_DECK); return; }   // R-427: its model the frame's
        if (F.kind == FX_INCUBATOR || F.kind == FX_JARS) {   // Z-05: the cabinet's faces and its inside (its back, its shelves), the shelf's; the flasks or the jars the frame's
            const Vec3 A[4] = {Vec3(F.x[0], F.y[0], F.z[0]), Vec3(F.x[1], F.y[1], F.z[1]), Vec3(F.x[2], F.y[2], F.z[2]), Vec3(F.x[3], F.y[3], F.z[3])};
            auto P = [&](double t, double back, double y) { return A[1] + (A[2] - A[1]) * t + ((A[0] + (A[3] - A[0]) * t) - (A[1] + (A[2] - A[1]) * t)) * back + Vec3(0, y, 0); };
            const Vec3 front = normalize(A[1] - A[0]), mid = P(0.5, 0.5, F.height / 2);
            ShipFixture box = F; box.kind = FX_BOX;
            fixture(box, index);
            finish = F.kind == FX_INCUBATOR ? F.finish : FIN_HULL;
            const double H = F.height;
            seen = mid + front * 3.0;
            quad(MAT_SOLID, 0.5, 1, P(0, 0.97, 0), P(1, 0.97, 0), P(1, 0.97, H), P(0, 0.97, H));   // the back, inside
            if (F.kind == FX_INCUBATOR) {
                seen = P(0.5, 0.5, H + 2.0);
                quad(MAT_SOLID, 0.65, 1, P(0, 0, 0.01), P(1, 0, 0.01), P(1, 0.97, 0.01), P(0, 0.97, 0.01));   // its floor
                for (int k = 1; k < F.boards; k++) {   // the shelves between, their edges toward the room
                    const double y = k * H / F.boards;
                    seen = P(0.5, 0.5, y + 2.0);
                    quad(MAT_SOLID, 0.7, 1, P(0.01, 0, y), P(0.99, 0, y), P(0.99, 0.95, y), P(0.01, 0.95, y));
                    seen = P(0.5, -3.0, y);
                    quad(MAT_SOLID, 0.8, 1, P(0.01, 0, y - 0.02), P(0.99, 0, y - 0.02), P(0.99, 0, y), P(0.01, 0, y));
                }
            }
            marker(F.kind == FX_INCUBATOR ? PIECE_INCUBATOR : PIECE_JARS, index, FIN_GLASS);
            m.finishes[F.room] |= 1u << FIN_DECK;   // the jars' silt
            finish = F.finish;
            return;
        }
        if (F.kind == FX_STORE) {   // Z-04: the cold store: its door's seam and handle, then the lamps the frame lights
            const Vec3 A[4] = {Vec3(F.x[0], F.y[0], F.z[0]), Vec3(F.x[1], F.y[1], F.z[1]), Vec3(F.x[2], F.y[2], F.z[2]), Vec3(F.x[3], F.y[3], F.z[3])};
            auto at = [&](double tt, double y) { return A[1] + (A[2] - A[1]) * tt + (A[1] - A[0]) * 0.03 + Vec3(0, y, 0); };
            ShipFixture box = F; box.kind = FX_BOX;
            fixture(box, index);
            finish = F.finish;
            line(0.45, at(0.08, 0.06), at(0.92, 0.06)); line(0.45, at(0.08, F.height - 0.06), at(0.92, F.height - 0.06));
            finish = FIN_BRASS;
            line(1.0, at(0.88, F.height * 0.45), at(0.88, F.height * 0.6));
            finish = F.finish;
            marker(PIECE_STORE, index, FIN_HULL);
            return;
        }
        if (fixtureBoxLike(F.kind)) {
            const Vec3 A[4] = {Vec3(F.x[0], F.y[0], F.z[0]), Vec3(F.x[1], F.y[1], F.z[1]), Vec3(F.x[2], F.y[2], F.z[2]), Vec3(F.x[3], F.y[3], F.z[3])};
            const Vec3 h(0, F.height, 0), mid = (A[0] + A[1] + A[2] + A[3]) * 0.25 + h * 0.5;
            auto face = [&](double s, const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, int fin) {
                const Vec3 c = (p0 + p1 + p2 + p3) * 0.25;
                seen = c + (c - mid) * 2.0 + Vec3(0, 1e-3, 0);
                finish = fin;
                quad(out ? MAT_FEATURE : (fin == FIN_GLASS ? MAT_TINT : MAT_SOLID), s, 1, p0, p1, p2, p3);
                finish = F.finish;
            };
            for (int i = 0; i < 6 && F.faces[i] != BF_NONE; i++) {
                const double s = F.shade[i];
                switch (F.faces[i]) {
                    case BF_TOP: {
                        face(s, A[0] + h, A[1] + h, A[2] + h, A[3] + h, F.topFinish >= 0 ? F.topFinish : F.finish);
                        if (!out && hasGlass(F.room) && F.height > 0.2) { const Vec3 top[4] = {A[0] + h, A[1] + h, A[2] + h, A[3] + h}; receiver(top, 4, Vec3(0, 1, 0), s, false); }
                        break;
                    }
                    case BF_FRONT: face(s, A[1], A[1] + h, A[2] + h, A[2], F.finish); break;
                    case BF_SIDE_A: face(s, A[0], A[1], A[1] + h, A[0] + h, F.finish); break;
                    case BF_SIDE_D: face(s, A[3], A[2], A[2] + h, A[3] + h, F.finish); break;
                    case BF_BACK: face(s, A[0], A[3], A[3] + h, A[0] + h, F.finish); break;
                    default: face(s, A[0], A[1], A[2], A[3], F.finish); break;
                }
            }
            if (F.kind == FX_BED) { marker(PIECE_GARDEN, index, FIN_PLANT); m.finishes[F.room] |= 1u << FIN_PLANT2; }
            if (F.kind == FX_DECK) marker(PIECE_DECK, index, FIN_BRASS);
            if (F.kind == FX_LOCKER) {   // two doors on its front: the seam between them, their handles (brass), a vent along each foot
                auto at = [&](double t, double y) { return A[1] + (A[2] - A[1]) * t + (A[1] - A[0]) * 0.03 + Vec3(0, y, 0); };
                line(0.45, at(0.5, 0.05), at(0.5, F.height - 0.05));
                finish = FIN_BRASS;
                for (double t : {0.46, 0.54}) line(1.0, at(t, F.height * 0.48), at(t, F.height * 0.58));
                finish = F.finish;
                for (double t0 : {0.08, 0.58}) line(0.5, at(t0, 0.15), at(t0 + 0.34, 0.15));
            }
        } else if (F.kind == FX_SCREEN) {
            flat = false;
            begin(PIECE_POLYGON, MAT_SCREEN, 0, F.device, F.factor, 1);
            for (int i = 0; i < 4; i++) vert(Vec3(F.x[i], F.y[i], F.z[i]));
        } else if (F.kind == FX_CAGE) {
            const double cx = F.x[0], cz = F.z[0];
            for (int k = 0; k < F.bars; k++) {   // its bars and the ring along their tops
                const double a = k * TAU / F.bars, a1 = (k + 1) * TAU / F.bars;
                begin(PIECE_LINE, MAT_PLAIN, BANK_HULL, F.device, F.barShade, 1);
                vert(onCircle(cx, cz, F.radius, a, F.barLo)); vert(onCircle(cx, cz, F.radius, a, F.barHi));
                begin(PIECE_LINE, MAT_PLAIN, BANK_HULL, F.device, F.barShade, 1);
                vert(onCircle(cx, cz, F.radius, a, F.barHi)); vert(onCircle(cx, cz, F.radius, a1, F.barHi));
            }
            marker(PIECE_CAPSULE, index, FIN_HULL);   // I-04: the capsule in it and the hatch's leaves under it (and the drop through it)
        } else if (F.kind == FX_TUBE) {
            const Vec3 p0(F.x[0], F.y[0], F.z[0]), p1(F.x[1], F.y[1], F.z[1]), d = normalize(p1 - p0);
            const Vec3 u = normalize(std::fabs(d.y) < 0.9 ? cross(d, Vec3(0, 1, 0)) : cross(d, Vec3(1, 0, 0))) * F.radius, w = cross(d, u);
            const Vec3 off[4] = {u + w, u - w, -u - w, -u + w}, axisMid = (p0 + p1) * 0.5;
            for (int k = 0; k < 4; k++) {
                seen = axisMid + (off[k] + off[(k + 1) % 4]) * 4.0;
                quad(solidMat, 0.6 + 0.1 * k, 1, p0 + off[k], p0 + off[(k + 1) % 4], p1 + off[(k + 1) % 4], p1 + off[k]);
            }
            seen = p0 - d;
            quad(solidMat, 0.5, 1, p0 + off[0], p0 + off[1], p0 + off[2], p0 + off[3]);
            seen = p1 + d;
            quad(solidMat, 0.9, 1, p1 + off[0], p1 + off[1], p1 + off[2], p1 + off[3]);
        } else if (F.kind == FX_RING) {
            const double cx = F.x[0], cy = F.y[0], cz = F.z[0], zf = cz + F.depth / 2, zb = cz - F.depth / 2;
            auto R = [&](double r, double a, double z) { return Vec3(cx + r * std::sin(a), cy + r * std::cos(a), z); };
            for (int k = 0; k < ROUND; k++) {
                const double a0 = k * TAU / ROUND, a1 = (k + 1) * TAU / ROUND, lit = 0.75 + 0.2 * std::cos(a0 * 0.5 + a1 * 0.5);
                seen = Vec3(cx, cy, zf + 1);
                quad(solidMat, 0.8 * lit, 1, R(F.inner, a0, zf), R(F.radius, a0, zf), R(F.radius, a1, zf), R(F.inner, a1, zf));
                seen = Vec3(cx, cy, zb - 1);
                quad(solidMat, 0.7 * lit, 1, R(F.inner, a0, zb), R(F.radius, a0, zb), R(F.radius, a1, zb), R(F.inner, a1, zb));
                seen = R(2 * F.radius, (a0 + a1) / 2, cz);
                quad(solidMat, 0.85 * lit, 1, R(F.radius, a0, zb), R(F.radius, a1, zb), R(F.radius, a1, zf), R(F.radius, a0, zf));
                seen = Vec3(cx, cy, cz);
                quad(MAT_GLOW, 0.6 * lit, 1, R(F.inner, a0, zb), R(F.inner, a1, zb), R(F.inner, a1, zf), R(F.inner, a0, zf));
            }
        } else if (F.kind == FX_MAST) {
            flat = false;
            begin(PIECE_MAST, MAT_FEATURE, BANK_HULL, F.device, 1, 1);   // drawn by the frame (its dish turning) in its place in the order
        } else {
            flat = false;
            begin(PIECE_RACK, MAT_SOLID, BANK_HULL, F.device, 1, 1);   // drawn by drone_draw.cpp in its place in the order
        }
    }

    // I-03: the hull outside (MAT_OUTSIDE, lit by the sun): the main deck's roofs (the bridge's out to its arc, the library's, the
    // laboratory's, the quarters'), the hall's roof round the dome, the maintenance room's astern of the quarters, and the prow below the great
    // window (a fan from the window's foot down to its nose); then what stands outside (the plan's fixtures of SHIP_OUTSIDE)
    void outside() {
        room = SHIP_OUTSIDE; finish = FIN_OUTSIDE; grid = false; cull = true;
        const double T = 0.3;   // the hull's thickness
        auto roof = [&](double x0, double x1, double z0, double z1, double y) {
            seen = Vec3((x0 + x1) / 2, y + 10, (z0 + z1) / 2);
            quad(MAT_OUTSIDE, 1, 1, Vec3(x0, y, z0), Vec3(x1, y, z0), Vec3(x1, y, z1), Vec3(x0, y, z1));
        };
        const ShipRoom& HL = ROOMS[ROOM_HALL];
        const double hx0 = HL.x0 - T, hx1 = HL.x1 + T, hz0 = HL.z0 - T, hz1 = HL.z1 + T;
        for (int r : {ROOM_LIBRARY, ROOM_LAB}) {
            const ShipRoom& R = ROOMS[r];
            roof(r == ROOM_LIBRARY ? R.x0 - T : hx1, r == ROOM_LIBRARY ? hx0 : R.x1 + T, R.z0 - T, R.z1 + T, R.ceiling + T);
        }
        const ShipRoom& Q = ROOMS[ROOM_QUARTERS];
        roof(Q.x0 - T, Q.x1 + T, Q.z0 - T, hz0, Q.ceiling + T);
        const ShipRoom& EN = ROOMS[ROOM_MAINTENANCE];
        roof(EN.x0 - T, EN.x1 + T, EN.z0 - T, Q.z0 - T, EN.ceiling + T);
        const ShipRoom& B = ROOMS[ROOM_BRIDGE];
        const double h = B.bowZ - B.z1, rad = (h * h + B.x1 * B.x1) / (2 * h), cz = B.bowZ - rad, bmax = std::asin(B.x1 / rad);
        const int np = std::min(B.bowPanes, 14);
        {   // the bridge's roof: from the hall to the arc, the hull's thickness out
            Vec3 pts[18]; int n = 0;
            const double y = B.ceiling + T;
            pts[n++] = Vec3(B.x0 - T, y, hz1); pts[n++] = Vec3(B.x1 + T, y, hz1); pts[n++] = Vec3(B.x1 + T, y, B.z1);
            for (int k = np; k >= 0; k--) { const double b = -bmax + 2 * bmax * k / np; pts[n++] = Vec3((rad + T) * std::sin(b), y, cz + (rad + T) * std::cos(b)); }
            pts[n++] = Vec3(B.x0 - T, y, B.z1);
            seen = Vec3(0, y + 10, B.z1);
            poly(MAT_OUTSIDE, 1, 1, pts, n);
        }
        {   // the prow: from the great window's foot down to its nose, two and a half metres out
            const Vec3 nose(0, B.bowLo - 1.3, B.bowZ + 2.6);
            for (int k = 0; k < np; k++) {
                const double b0 = -bmax + 2 * bmax * k / np, b1 = -bmax + 2 * bmax * (k + 1) / np;
                const Vec3 p0(rad * std::sin(b0), B.bowLo, cz + rad * std::cos(b0)), p1(rad * std::sin(b1), B.bowLo, cz + rad * std::cos(b1));
                seen = (p0 + p1) * 0.5 + Vec3(0, 6, 4);
                const Vec3 tri[3] = {p0, p1, nose};
                poly(MAT_OUTSIDE, 0.9, 1, tri, 3);
            }
        }
        {   // the hall's roof round the observatory's floor: from the dome's circle out to the hall's square, a sixteenth at a time
            const ShipRoom& O = ROOMS[ROOM_OBSERVATORY];
            const double y = O.floor, r = O.x1;
            seen = Vec3(0, y + 10, 0);
            for (int k = 0; k < ROUND; k++) {
                const double a0 = k * TAU / ROUND, a1 = (k + 1) * TAU / ROUND;
                auto edge = [&](double a) {   // where a ray from the middle meets the square
                    const double sx = std::sin(a), sz = std::cos(a), t = std::min(std::fabs(sx) > 1e-9 ? hx1 / std::fabs(sx) : 1e30, std::fabs(sz) > 1e-9 ? hz1 / std::fabs(sz) : 1e30);
                    return Vec3(sx * t, y, sz * t);
                };
                quad(MAT_OUTSIDE, 0.95, 1, onCircle(O.x0, O.z0, r, a0, y), edge(a0), edge(a1), onCircle(O.x0, O.z0, r, a1, y));
            }
        }
        for (int i = 0; i < (int)fixtures().size(); i++) if (fixtures()[i].room == SHIP_OUTSIDE) fixture(fixtures()[i], i);
    }

    // I-03: each vertex of the rooms lit by its room's lamps (the stair's by the room its height is in): the bulbs' and the glows' light
    // with a share of the room's mean given back by its surfaces; I-04: the glows' share of it and of the fill apart too (U's night keeps
    // it), the rooms' fills kept for what the frame lights
    void light() {
        double sum[SHIP_ROOMS] = {}, sumNight[SHIP_ROOMS] = {}; int count[SHIP_ROOMS] = {};
        auto roomAt = [](int room, double y) -> int {
            if (room != ROOM_STAIR) return room;
            return y < ROOMS[ROOM_HALL].floor - 0.15 ? ROOM_HANGAR : (y < ROOMS[ROOM_HALL].ceiling ? ROOM_HALL : ROOM_OBSERVATORY);
        };
        for (const ShipPiece& pc : m.pieces) {
            if (pc.room >= SHIP_ROOMS || pc.mat == MAT_SCREEN || pc.mat == MAT_PANE || pc.kind == PIECE_RACK) continue;
            const bool line = pc.kind == PIECE_LINE;
            for (int i = 0; i < pc.n; i++) {
                ShipVert& v = m.verts[pc.first + i];
                const int lr = roomAt(pc.room, v.p.y);
                for (const ShipLamp& L : LAMPS) {
                    if (L.room != lr) continue;
                    const double e = lampOn(L, v.p, line ? nullptr : &pc.nrm);
                    v.lamp += e;
                    if (L.kind == LAMP_GLOW) v.night += e;
                }
                sum[lr] += v.lamp; sumNight[lr] += v.night; count[lr]++;
            }
        }
        for (int r = 0; r < SHIP_ROOMS; r++)
            if (count[r]) { m.fill[r] = LAMP_FILL * sum[r] / count[r]; m.fillNight[r] = LAMP_FILL * sumNight[r] / count[r]; }
        for (const ShipPiece& pc : m.pieces) {
            if (pc.room >= SHIP_ROOMS || pc.mat == MAT_SCREEN || pc.mat == MAT_PANE || pc.kind == PIECE_RACK) continue;
            for (int i = 0; i < pc.n; i++) {
                ShipVert& v = m.verts[pc.first + i];
                const int lr = roomAt(pc.room, v.p.y);
                if (count[lr]) { v.lamp += m.fill[lr]; v.night += m.fillNight[lr]; }
            }
        }
    }
};

ShipMesh buildMesh() {
    Builder b;
    for (int r = 0; r < SHIP_ROOMS; r++) {
        b.room = r;
        b.m.first[r] = (int)b.m.pieces.size();
        const ShipRoom& R = ROOMS[r];
        b.grid = true; b.cull = true; b.seen = roomMiddle(R);
        if (R.shape == RS_BOX) b.boxRoom(r);
        else if (R.shape == RS_BOW) b.bowRoom(r);
        else if (R.shape == RS_DISC) b.discRoom(r);
        else b.stair();
        for (int i = 0; i < (int)fixtures().size(); i++) if (fixtures()[i].room == r) b.fixture(fixtures()[i], i);
    }
    b.m.first[SHIP_OUTSIDE] = (int)b.m.pieces.size();
    b.outside();
    b.m.first[SHIP_OUTSIDE + 1] = (int)b.m.pieces.size();
    b.light();
    return b.m;
}

// ---- the routes ------------------------------------------------------------------------------------------------------------------------

// a way the walk takes between two rooms: an arch, or a landing (its room and the stair)
bool walkedWay(const ShipOpening& o) { return o.kind == OPEN_ARCH || o.kind == OPEN_STAIR; }

struct Routes { int next[SHIP_ROOMS][SHIP_ROOMS]; };
Routes makeRoutes() {
    Routes R;
    for (int a = 0; a < SHIP_ROOMS; a++) {
        for (int b = 0; b < SHIP_ROOMS; b++) R.next[a][b] = -1;
        int first[SHIP_ROOMS]; bool seen[SHIP_ROOMS] = {};
        for (int& f : first) f = -1;
        int queue[SHIP_ROOMS], qn = 0;
        queue[qn++] = a; seen[a] = true;
        for (int qi = 0; qi < qn; qi++) {
            const int r = queue[qi];
            for (int i = 0; i < N_OPENINGS; i++) {
                const ShipOpening& o = OPENINGS[i];
                if (!walkedWay(o) || (o.from != r && o.to != r)) continue;
                const int n = o.from == r ? o.to : o.from;
                if (seen[n]) continue;
                seen[n] = true; first[n] = r == a ? i : first[r]; queue[qn++] = n;
            }
        }
        for (int b = 0; b < SHIP_ROOMS; b++) R.next[a][b] = b == a ? -1 : first[b];
    }
    return R;
}

// ---- the sight -------------------------------------------------------------------------------------------------------------------------

// a way's eight corners (an arch's opening on both its faces; a hole's square at both its heights)
void wayCorners(const ShipOpening& o, Vec3* p) {
    if (o.kind == OPEN_ARCH) {
        double f, t;
        const bool alongZ = archAcross(o, f, t);
        const double y0 = ROOMS[o.from].floor, y1 = y0 + o.height;
        int n = 0;
        for (double c : {f, t}) for (double u : {o.lo, o.hi}) for (double y : {y0, y1}) p[n++] = alongZ ? Vec3(c, y, u) : Vec3(u, y, c);
        return;
    }
    const double ya = o.kind == OPEN_WELL ? ROOMS[o.from].floor : ROOMS[o.from].ceiling, yb = o.kind == OPEN_WELL ? ROOMS[o.to].ceiling : ROOMS[o.to].floor;
    int n = 0;
    for (double y : {ya, yb}) for (double dx : {-o.r, o.r}) for (double dz : {-o.r, o.r}) p[n++] = Vec3(o.x + dx, y, o.z + dz);
}
// whether the eye is on room r's side of a way's middle (only from there is it looked through out of r)
bool eyeOnSide(const ShipOpening& o, int r, const Vec3& eye) {
    if (o.kind == OPEN_ARCH) {
        double f, t;
        const bool alongZ = archAcross(o, f, t);
        const double m = (f + t) / 2, e = alongZ ? eye.x : eye.z;
        const bool fromBelow = t > f;   // `from` lies on the side of lower coordinates
        const bool onFrom = fromBelow ? e < m : e > m;
        return o.from == r ? onFrom : !onFrom;
    }
    const double ya = o.kind == OPEN_WELL ? ROOMS[o.from].floor : ROOMS[o.from].ceiling, yb = o.kind == OPEN_WELL ? ROOMS[o.to].ceiling : ROOMS[o.to].floor, m = (ya + yb) / 2;
    const bool fromAbove = ya > yb;
    const bool onFrom = fromAbove ? eye.y > m : eye.y < m;
    return o.from == r ? onFrom : !onFrom;
}
// the corners' bounds on the screen: 0 when every corner is behind the eye, 1 with the bounds, 2 when some are (as wide as it can be)
int screenBounds(const Vec3* p, int n, const Vec3& eye, const Mat3& look, const Proj& pj, double& x0, double& y0, double& x1, double& y1) {
    int behind = 0;
    x0 = y0 = 1e30; x1 = y1 = -1e30;
    for (int i = 0; i < n; i++) {
        const Vec3 v = look * (p[i] - eye);
        if (v.z < NEAR_Z) { behind++; continue; }
        const double sx = pj.cx + pj.f * v.x / v.z, sy = pj.cy - pj.f * v.y / v.z;
        x0 = std::min(x0, sx); x1 = std::max(x1, sx); y0 = std::min(y0, sy); y1 = std::max(y1, sy);
    }
    return behind == n ? 0 : (behind ? 2 : 1);
}
}

// ---- the rows' readers -----------------------------------------------------------------------------------------------------------------

ShipRows<ShipFinishRow> shipFinishes() { return planRows(FINISHES); }
ShipRows<ShipRoom> shipRooms() { return planRows(ROOMS); }
ShipRows<ShipOpening> shipOpenings() { return planRows(OPENINGS); }
ShipRows<ShipWindow> shipWindows() { return planRows(WINDOWS); }
ShipRows<ShipFixture> shipFixtures() { return {fixtures().data(), (int)fixtures().size()}; }
ShipRows<ShipCollider> shipColliders() {
    static const std::vector<ShipCollider> v = makeColliders();
    return {v.data(), (int)v.size()};
}
ShipRows<ShipLamp> shipLamps() { return planRows(LAMPS); }
ShipRows<ShipView> shipViews() { return planRows(VIEWS); }
const ShipStair& shipStair() { return STAIR; }

const ShipView& shipView(const char* name) {
    for (const ShipView& v : VIEWS) if (std::strcmp(v.name, name) == 0) return v;
    return VIEWS[0];
}
const ShipFixture& shipFixtureOf(int kind) {
    for (const ShipFixture& f : fixtures()) if (f.kind == kind) return f;
    return fixtures()[0];
}
const ShipLamp& shipLampIn(int room, int kind) {
    for (const ShipLamp& l : LAMPS) if (l.room == room && l.kind == kind) return l;
    for (const ShipLamp& l : LAMPS) if (l.kind == kind) return l;
    return LAMPS[0];
}
const ShipOpening& shipLifter() {
    for (const ShipOpening& o : OPENINGS) if (o.kind == OPEN_LIFTER) return o;
    return OPENINGS[0];
}
int shipRoomNamed(const char* name) {
    for (int r = 0; r < SHIP_ROOMS; r++) if (std::strcmp(ROOMS[r].name, name) == 0) return r;
    return -1;
}

// ---- the walk --------------------------------------------------------------------------------------------------------------------------

namespace {
// the stair's height at an angle walked round it: flat on a landing, a ramp between
double stairHeight(double a) {
    const ShipStair& S = STAIR;
    if (a <= landingA(0) + S.landing) return S.level[0];
    for (int k = 0; k + 1 < S.levels; k++) {
        const double lo = landingA(k) + S.landing, hi = landingA(k + 1) - S.landing;
        if (a < lo) return S.level[k];
        if (a <= hi) return S.level[k] + (S.level[k + 1] - S.level[k]) * (a - lo) / (hi - lo);
    }
    return S.level[S.levels - 1];
}
// the colliders of a room on a step from (ox, oz) to (x, z): a circle pushes out round its middle; a box back to the side the step met it
// from (else its nearest); a ring out round its middle unless within its way off, and back to the way's edge when the step slid out of it
void collide(int room, double ox, double oz, double& x, double& z) {
    for (const ShipCollider& c : shipColliders()) {
        if (c.room != room) continue;
        if (c.kind == COLLIDE_BOX) {
            if (x <= c.x0 || x >= c.x1 || z <= c.z0 || z >= c.z1) continue;
            if (ox <= c.x0) x = c.x0;
            else if (ox >= c.x1) x = c.x1;
            else if (oz <= c.z0) z = c.z0;
            else if (oz >= c.z1) z = c.z1;
            else {
                const double d[4] = {x - c.x0, c.x1 - x, z - c.z0, c.z1 - z};
                const int k = (int)(std::min_element(d, d + 4) - d);
                if (k == 0) x = c.x0; else if (k == 1) x = c.x1; else if (k == 2) z = c.z0; else z = c.z1;
            }
            continue;
        }
        const double dx = x - c.x, dz = z - c.z, d = std::sqrt(dx * dx + dz * dz);
        if (d >= c.r || d < 1e-6) continue;
        if (c.kind == COLLIDE_RING) {
            const double off = wrapAngle(std::atan2(dx, dz) - c.gapAt);
            if (std::fabs(off) <= c.gapHalf) continue;
            const double odx = ox - c.x, odz = oz - c.z;
            if (std::hypot(odx, odz) < c.r) {   // inside the way off before the step: kept to its edge
                const double a = c.gapAt + (off > 0 ? c.gapHalf : -c.gapHalf);
                x = c.x + d * std::sin(a); z = c.z + d * std::cos(a);
                continue;
            }
        }
        x = c.x + dx / d * c.r; z = c.z + dz / d * c.r;
    }
}
// round the stair: its angle walked (its height by it), kept between its column and its rail; off at a landing's way into its room
void stairWalk(ShipPlace& p, double nx, double nz) {
    const ShipStair& S = STAIR;
    const double dx = nx - S.x, dz = nz - S.z;
    double r = std::hypot(dx, dz);
    double a = p.stairA + wrapAngle(std::atan2(dx, dz) - p.stairA);
    a = clampd(a, landingA(0) - S.landing, landingA(S.levels - 1) + S.landing);
    int way = -1;
    for (int k = 0; k < S.levels; k++) if (std::fabs(a - landingA(k)) <= S.gap) way = k;
    if (way >= 0 && r > STAIR_OFF) { p.room = S.room[way]; p.x = nx; p.z = nz; return; }
    const double rIn = S.column + WR, rOut = S.outer - WR;
    if (r > rOut && way < 0) {
        const double or_ = std::hypot(p.x - S.x, p.z - S.z);
        int was = -1;
        for (int k = 0; k < S.levels; k++) if (std::fabs(p.stairA - landingA(k)) <= S.gap) was = k;
        if (or_ > rOut && was >= 0) a = clampd(a, landingA(was) - S.gap, landingA(was) + S.gap);   // slid out of a way off: kept to its edge
        else r = rOut;
    }
    r = clampd(r, rIn, STAIR_OFF);
    p.stairA = a; p.x = S.x + r * std::sin(a); p.z = S.z + r * std::cos(a);
}
}

void shipWalk(ShipPlace& p, double nx, double nz) {
    if (p.room == ROOM_STAIR) { stairWalk(p, nx, nz); return; }
    const double ox = p.x, oz = p.z;
    bool on = false;
    for (const Floor& f : floors()) if (floorOf(f, p.room) && inFloor(f, nx, nz, 0)) { on = true; break; }
    if (!on) {   // kept on the floor the step began on (of those it was on, the one nearest where it was going)
        double bx = ox, bz = oz, bd = 1e300;
        for (const Floor& f : floors()) {
            if (!floorOf(f, p.room) || !inFloor(f, ox, oz, 1e-6)) continue;
            double cx = nx, cz = nz;
            clampTo(f, cx, cz);
            const double d = (cx - nx) * (cx - nx) + (cz - nz) * (cz - nz);
            if (d < bd) { bd = d; bx = cx; bz = cz; }
        }
        if (bd > 1e299) for (const Floor& f : floors()) if (f.room == p.room) { bx = nx; bz = nz; clampTo(f, bx, bz); break; }
        nx = bx; nz = bz;
    }
    for (const Floor& f : floors()) {   // an arch's middle crossed: the room beyond
        if (f.arch < 0 || !floorOf(f, p.room) || !inFloor(f, nx, nz, 0)) continue;
        const ShipOpening& o = OPENINGS[f.arch];
        if (!eyeOnSide(o, p.room, Vec3(nx, 0, nz))) { p.room = o.from == p.room ? o.to : o.from; break; }
    }
    collide(p.room, ox, oz, nx, nz);
    const ShipStair& S = STAIR;
    for (int k = 0; k < S.levels; k++) {   // onto the stair at its landing's way
        if (S.room[k] != p.room) continue;
        const double dx = nx - S.x, dz = nz - S.z, off = wrapAngle(std::atan2(dx, dz) - landingAt(k));
        if (std::fabs(off) <= S.gap && std::hypot(dx, dz) < STAIR_OFF) { p.room = ROOM_STAIR; p.stairA = landingA(k) + off; break; }
    }
    p.x = nx; p.z = nz;
}

double shipFloorAt(const ShipPlace& p) {
    if (p.room == ROOM_STAIR) return stairHeight(p.stairA);
    const ShipRoom& R = ROOMS[p.room];
    if (R.steps > 0) {   // the bridge's steps: a ramp under them
        const double z0 = R.z0 + 0.5 * R.stepDepth, u = clampd((p.z - z0) / (R.steps * R.stepDepth), 0, 1);
        return R.stepFloor + (R.floor - R.stepFloor) * u;
    }
    return R.floor;
}

int shipEyeRoom(const ShipPlace& p) {
    if (p.room != ROOM_STAIR) return p.room;
    const double y = stairHeight(p.stairA) + SHIP_EYE;
    if (y < ROOMS[ROOM_HANGAR].ceiling) return ROOM_HANGAR;
    if (y > ROOMS[ROOM_OBSERVATORY].floor) return ROOM_OBSERVATORY;
    return ROOM_HALL;
}

bool shipWalkable(const ShipPlace& p) {
    if (p.room == ROOM_STAIR) {
        const double r = std::hypot(p.x - STAIR.x, p.z - STAIR.z);
        return r >= STAIR.column + WR - 1e-6 && r <= STAIR_OFF + 1e-6;
    }
    bool on = false;
    for (const Floor& f : floors()) if (floorOf(f, p.room) && inFloor(f, p.x, p.z, 1e-6)) on = true;
    if (!on) return false;
    for (const ShipCollider& c : shipColliders()) {
        if (c.room != p.room) continue;
        if (c.kind == COLLIDE_BOX) { if (p.x > c.x0 + 1e-6 && p.x < c.x1 - 1e-6 && p.z > c.z0 + 1e-6 && p.z < c.z1 - 1e-6) return false; continue; }
        const double dx = p.x - c.x, dz = p.z - c.z, d = std::hypot(dx, dz);
        if (d >= c.r - 1e-6) continue;
        if (c.kind == COLLIDE_RING && std::fabs(wrapAngle(std::atan2(dx, dz) - c.gapAt)) <= c.gapHalf + 1e-6) continue;
        return false;
    }
    return true;
}

ShipPlace shipPlaceOf(int room, double x, double z) {
    ShipPlace p;
    p.room = room; p.x = x; p.z = z;
    if (room == ROOM_STAIR) p.stairA = landingA(1) + wrapAngle(std::atan2(x - STAIR.x, z - STAIR.z) - landingAt(1));
    return p;
}

double shipLandingAngle(int room) {
    for (int k = 0; k < STAIR.levels; k++) if (STAIR.room[k] == room) return landingA(k);
    return STAIR.at;
}

// ---- the routes ------------------------------------------------------------------------------------------------------------------------

int shipNextWay(int from, int to) {
    static const Routes R = makeRoutes();
    if (from < 0 || from >= SHIP_ROOMS || to < 0 || to >= SHIP_ROOMS) return -1;
    return R.next[from][to];
}

void shipWayPoints(int way, int room, double& ax, double& az, double& bx, double& bz) {
    const ShipOpening& o = OPENINGS[way];
    if (o.kind == OPEN_ARCH) {
        double f, t;
        const bool alongZ = archAcross(o, f, t);
        const double mid = (o.lo + o.hi) / 2, dir = t > f ? 1 : -1;   // from `from` toward `to`
        const double inFrom = f - dir * 0.9, inTo = t + dir * 0.9;
        const double a = o.from == room ? inFrom : inTo, b = o.from == room ? inTo : inFrom;
        if (alongZ) { ax = a; az = mid; bx = b; bz = mid; } else { ax = mid; az = a; bx = mid; bz = b; }
        return;
    }
    int k = 0;   // a landing: its level, its way off the stair at its angle
    for (int i = 0; i < STAIR.levels; i++) if (STAIR.room[i] == o.from) k = i;
    const double at = landingAt(k), out = railOf(k) + 0.45, on = 0.95;
    const double ra = room == ROOM_STAIR ? on : out, rb = room == ROOM_STAIR ? out : on;
    ax = STAIR.x + ra * std::sin(at); az = STAIR.z + ra * std::cos(at);
    bx = STAIR.x + rb * std::sin(at); bz = STAIR.z + rb * std::cos(at);
}

// ---- the sight -------------------------------------------------------------------------------------------------------------------------

void shipSight(const ShipPlace& p, const Vec3& eye, const Mat3& look, const Proj& pj, bool depolarised, ShipSight& out) {
    out = ShipSight();
    const double W = FBW, H = FBH;
    std::vector<int> queue;
    auto see = [&](int r, int depth, double x0, double y0, double x1, double y1) {
        x0 = std::max(0.0, x0); y0 = std::max(0.0, y0); x1 = std::min(W, x1); y1 = std::min(H, y1);
        if (x1 <= x0 || y1 <= y0) return;
        if (!out.seen[r]) { out.seen[r] = true; out.depth[r] = depth; out.x0[r] = x0; out.y0[r] = y0; out.x1[r] = x1; out.y1[r] = y1; queue.push_back(r); return; }
        if (x0 >= out.x0[r] - 0.5 && y0 >= out.y0[r] - 0.5 && x1 <= out.x1[r] + 0.5 && y1 <= out.y1[r] + 0.5) return;
        out.x0[r] = std::min(out.x0[r], x0); out.y0[r] = std::min(out.y0[r], y0); out.x1[r] = std::max(out.x1[r], x1); out.y1[r] = std::max(out.y1[r], y1);
        if (depth < out.depth[r]) out.depth[r] = depth;
        queue.push_back(r);
    };
    const int start = shipEyeRoom(p);
    see(start, 0, 0, 0, W, H);
    if (p.room == ROOM_STAIR) {   // on the stair: the rooms round its well seen whole when the eye is near their slab
        see(ROOM_STAIR, 0, 0, 0, W, H);
        const double well = (ROOMS[ROOM_HALL].floor + ROOMS[ROOM_HANGAR].ceiling) / 2, oculus = (ROOMS[ROOM_HALL].ceiling + ROOMS[ROOM_OBSERVATORY].floor) / 2;
        if (std::fabs(eye.y - well) < 0.8) { see(ROOM_HALL, 0, 0, 0, W, H); see(ROOM_HANGAR, 0, 0, 0, W, H); }
        if (std::fabs(eye.y - oculus) < 0.8) { see(ROOM_HALL, 0, 0, 0, W, H); see(ROOM_OBSERVATORY, 0, 0, 0, W, H); }
    }
    for (size_t qi = 0; qi < queue.size(); qi++) {
        const int r = queue[qi];
        if (r == ROOM_STAIR || out.depth[r] >= SHIP_SIGHT_DEPTH) continue;
        for (const ShipOpening& o : OPENINGS) {
            if ((o.kind != OPEN_ARCH && o.kind != OPEN_WELL && o.kind != OPEN_OCULUS) || (o.from != r && o.to != r)) continue;
            if (!eyeOnSide(o, r, eye)) continue;
            Vec3 c[8];
            wayCorners(o, c);
            double x0, y0, x1, y1;
            const int b = screenBounds(c, 8, eye, look, pj, x0, y0, x1, y1);
            if (b == 0) continue;
            if (b == 2) { x0 = out.x0[r]; y0 = out.y0[r]; x1 = out.x1[r]; y1 = out.y1[r]; }
            see(o.from == r ? o.to : o.from, out.depth[r] + 1, std::max(x0, out.x0[r]), std::max(y0, out.y0[r]), std::min(x1, out.x1[r]), std::min(y1, out.y1[r]));
        }
    }
    for (int k = 0; k < STAIR.levels; k++) {   // the stair with any of its rooms, as wide as they are
        const int r = STAIR.room[k];
        if (out.seen[r]) see(ROOM_STAIR, out.depth[r], out.x0[r], out.y0[r], out.x1[r], out.y1[r]);
    }
    for (int r = 0; r < SHIP_ROOMS; r++) out.rooms += out.seen[r] ? 1 : 0;
    if (depolarised) { out.seesOut = true; out.ox0 = 0; out.oy0 = 0; out.ox1 = W; out.oy1 = H; return; }
    // I-03: every pane in view within its room's box, their bounds the outside's (what is drawn of the hull outside)
    const ShipMesh& m = shipMesh();
    out.ox0 = out.oy0 = 1e30; out.ox1 = out.oy1 = -1e30;
    for (int r = 0; r < SHIP_ROOMS; r++) {
        if (!out.seen[r]) continue;
        for (int i = m.first[r]; i < m.first[r + 1]; i++) {
            const ShipPiece& pc = m.pieces[i];
            if (pc.mat != MAT_PANE) continue;
            RVert q[16];
            for (int k = 0; k < pc.n && k < 16; k++) {
                const Vec3 v = look * (m.verts[pc.first + k].p - eye);
                q[k].x = v.x; q[k].y = v.y; q[k].z = v.z;
            }
            ScreenVert sv[16];
            double miny, maxy;
            const int n = projectPolygon(q, std::min(pc.n, 16), pj, sv, miny, maxy);
            if (!n) continue;
            double minx = 1e30, maxx = -1e30;
            for (int k = 0; k < n; k++) { minx = std::min(minx, sv[k].sx); maxx = std::max(maxx, sv[k].sx); }
            if (maxx >= out.x0[r] && minx <= out.x1[r] && maxy >= out.y0[r] && miny <= out.y1[r]) {
                out.seesOut = true;
                out.ox0 = std::min(out.ox0, std::max(minx, out.x0[r])); out.ox1 = std::max(out.ox1, std::min(maxx, out.x1[r]));
                out.oy0 = std::min(out.oy0, std::max(miny, out.y0[r])); out.oy1 = std::max(out.oy1, std::min(maxy, out.y1[r]));
            }
        }
    }
    if (!out.seesOut) out.ox0 = out.oy0 = out.ox1 = out.oy1 = 0;
}


double shipRoomNear(int room, const Vec3& p) {
    if (room < 0 || room >= SHIP_ROOMS) return 1e30;
    const ShipRoom& R = ROOMS[room];
    const double dy = p.y < R.floor ? R.floor - p.y : (p.y > R.ceiling ? p.y - R.ceiling : 0);
    if (R.shape == RS_DISC || R.shape == RS_STAIR) {
        const double r = R.shape == RS_DISC ? R.x1 : STAIR.outer, d = std::max(0.0, std::hypot(p.x - R.x0, p.z - R.z0) - r);
        return std::hypot(d, dy);
    }
    const double z1 = R.shape == RS_BOW ? R.bowZ : R.z1;
    const double dx = p.x < R.x0 ? R.x0 - p.x : (p.x > R.x1 ? p.x - R.x1 : 0), dz = p.z < R.z0 ? R.z0 - p.z : (p.z > z1 ? p.z - z1 : 0);
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// ---- the mesh and the banks ------------------------------------------------------------------------------------------------------------

const ShipMesh& shipMesh() {
    static const ShipMesh mesh = buildMesh();
    return mesh;
}

// I-04: a place lit as the mesh's vertices are: its room's lamps (the bulbs and the glows, the glows apart), the room's fills
void shipLampLight(int room, const Vec3& p, const Vec3* nrm, double& lamp, double& night) {
    lamp = night = 0;
    if (room < 0 || room >= SHIP_ROOMS) return;
    for (const ShipLamp& L : LAMPS) {
        if (L.room != room) continue;
        const double e = lampOn(L, p, nrm);
        lamp += e;
        if (L.kind == LAMP_GLOW) night += e;
    }
    const ShipMesh& m = shipMesh();
    lamp += m.fill[room]; night += m.fillNight[room];
}

// ---- the light (I-03) ------------------------------------------------------------------------------------------------------------------

namespace {
constexpr double PATCH_SHADES = 48;   // the sun's patch at its full: the shades it adds on a surface square to it (its factor 1)

// a convex polygon (on a plane of normal `nrm`) cut to a convex outline on the same plane
int clipTo(const Vec3* in, int n, const Vec3* poly, int pn, const Vec3& nrm, Vec3* out) {
    Vec3 a[32], b[32];
    int m = std::min(n, 31);
    for (int i = 0; i < m; i++) a[i] = in[i];
    const Vec3 mid = middleOf(poly, pn);
    for (int e = 0; e < pn && m >= 3; e++) {
        const Vec3& p = poly[e]; const Vec3& q = poly[(e + 1) % pn];
        Vec3 en = cross(nrm, q - p);
        if (dot(en, mid - p) < 0) en = en * -1.0;
        m = clipPlane(a, m, p, en, b);
        for (int i = 0; i < m; i++) a[i] = b[i];
    }
    for (int i = 0; i < m; i++) out[i] = a[i];
    return m;
}
// a box's shadow on a floor at y along the light (toward the sun `s`): the hull of its eight corners cast there, counter-clockwise seen
// from above (x, z), at most eight points
int shadowOf(const ShipFixture& f, double y, const Vec3& s, Vec3* out) {
    Vec3 p[8];
    for (int i = 0; i < 4; i++)
        for (int k = 0; k < 2; k++) {
            const Vec3 c(f.x[i], f.y[i] + k * f.height, f.z[i]);
            p[i * 2 + k] = c - s * ((c.y - y) / s.y);
        }
    std::sort(p, p + 8, [](const Vec3& a, const Vec3& b) { return a.x < b.x || (a.x == b.x && a.z < b.z); });
    auto turn = [](const Vec3& o, const Vec3& a, const Vec3& b) { return (a.x - o.x) * (b.z - o.z) - (a.z - o.z) * (b.x - o.x); };
    Vec3 h[16]; int k = 0;
    for (int i = 0; i < 8; i++) { while (k >= 2 && turn(h[k - 2], h[k - 1], p[i]) <= 1e-12) k--; h[k++] = p[i]; }
    for (int i = 6, t = k + 1; i >= 0; i--) { while (k >= t && turn(h[k - 2], h[k - 1], p[i]) <= 1e-12) k--; h[k++] = p[i]; }
    k--;
    for (int i = 0; i < k; i++) { out[i] = h[i]; out[i].y = y; }
    return k;
}
// a floor's piece less a box's shadow: the parts outside each of the shadow's edges in turn (convex, disjoint), what is inside left out
void lessShadow(const Vec3* piece, int n, const Vec3* sh, int sn, std::vector<std::vector<Vec3>>& out) {
    Vec3 rest[32]; int m = n;
    for (int i = 0; i < n; i++) rest[i] = piece[i];
    for (int e = 0; e < sn && m >= 3; e++) {
        const Vec3& p = sh[e]; const Vec3& q = sh[(e + 1) % sn];
        const Vec3 outward(q.z - p.z, 0, -(q.x - p.x));   // the right of a counter-clockwise edge seen from above (x right, z up)
        Vec3 part[32];
        const int k = clipPlane(rest, m, p, outward, part);
        if (k >= 3) out.emplace_back(part, part + k);
        Vec3 in[32];
        m = clipPlane(rest, m, p, outward * -1.0, in);
        for (int i = 0; i < m; i++) rest[i] = in[i];
    }
}
}

// the sun's patches in a room: each of its glass's outlines cast along the light (from the sun, `s` toward it) onto each surface facing the
// sun, cut to the surface; on a floor, the room's boxes' shadows taken out. The dome lights the observatory's floor but where the parapet's
// shadow falls (a disc of its radius shifted from the sun by the parapet's height over the sun's slope)
void shipPatches(int room, const Vec3& s, double light, std::vector<ShipPatch>& out) {
    if (light <= 0 || room < 0 || room >= SHIP_ROOMS) return;
    const ShipMesh& m = shipMesh();
    auto put = [&](const Vec3* p, int n, double shade) {
        if (n < 3 || shade <= 0.05) return;
        ShipPatch q; q.room = room; q.n = std::min(n, 16); q.shade = shade;
        for (int i = 0; i < q.n; i++) q.p[i] = p[i];
        out.push_back(q);
    };
    for (const ShipGlass& g : m.glass) {
        if (g.room != room) continue;
        if (g.dome) {
            const ShipRoom& R = ROOMS[room];
            const double sy = s.y;
            if (sy < 0.07) continue;
            const double hl = std::sqrt(std::max(1e-9, s.x * s.x + s.z * s.z)), w = R.parapet * hl / sy;
            const Vec3 c(R.x0 - s.x / hl * w, R.floor, R.z0 - s.z / hl * w);
            Vec3 lit[ROUND];
            for (int k = 0; k < ROUND; k++) lit[k] = onCircle(c.x, c.z, R.x1 - 0.02, (k + 0.5) * TAU / ROUND, R.floor);
            for (const ShipReceiver& r : m.receivers) {
                if (r.room != room || !r.floor) continue;
                Vec3 part[32];
                const int n = clipTo(r.p, r.n, lit, ROUND, Vec3(0, 1, 0), part);
                put(part, n, PATCH_SHADES * light * sy * r.a);
            }
            continue;
        }
        if (dot(s, g.out) < 0.02) continue;
        for (const ShipReceiver& r : m.receivers) {
            if (r.room != room) continue;
            const double c = dot(s, r.nrm);
            if (c < 0.02) continue;
            Vec3 cast[16];
            bool front = true;
            for (int i = 0; i < g.n && front; i++) {
                const double t = dot(g.p[i] - r.p[0], r.nrm) / c;
                front = t > 1e-6;
                cast[i] = g.p[i] - s * t;
            }
            if (!front) continue;
            Vec3 part[32];
            const int n = clipTo(cast, g.n, r.p, r.n, r.nrm, part);
            if (n < 3) continue;
            const double shade = PATCH_SHADES * light * c * r.a;
            if (!r.floor) { put(part, n, shade); continue; }
            std::vector<std::vector<Vec3>> pieces = {std::vector<Vec3>(part, part + n)};
            for (const ShipFixture& f : fixtures()) {
                if (f.room != room || !fixtureBoxLike(f.kind) || f.open || f.height < 0.3 || f.y[0] < r.p[0].y - 0.05) continue;
                Vec3 sh[16];
                const int sn = shadowOf(f, r.p[0].y, s, sh);
                if (sn < 3) continue;
                std::vector<std::vector<Vec3>> next;
                for (const std::vector<Vec3>& pc : pieces) {
                    double x0 = 1e30, x1 = -1e30, z0 = 1e30, z1 = -1e30, sx0 = 1e30, sx1 = -1e30, sz0 = 1e30, sz1 = -1e30;
                    for (const Vec3& v : pc) { x0 = std::min(x0, v.x); x1 = std::max(x1, v.x); z0 = std::min(z0, v.z); z1 = std::max(z1, v.z); }
                    for (int i = 0; i < sn; i++) { sx0 = std::min(sx0, sh[i].x); sx1 = std::max(sx1, sh[i].x); sz0 = std::min(sz0, sh[i].z); sz1 = std::max(sz1, sh[i].z); }
                    if (x1 <= sx0 || x0 >= sx1 || z1 <= sz0 || z0 >= sz1) { next.push_back(pc); continue; }
                    lessShadow(pc.data(), (int)pc.size(), sh, sn, next);
                }
                pieces.swap(next);
            }
            for (const std::vector<Vec3>& pc : pieces) put(pc.data(), (int)pc.size(), shade);
        }
    }
}

// a room's sight of the outside toward a direction: the best of its glass's faces toward it (the dome's above its parapet)
double shipGlassToward(int room, const Vec3& dir) {
    double best = 0;
    for (const ShipGlass& g : shipMesh().glass) {
        if (g.room != room) continue;
        best = std::max(best, g.dome ? smoothstep(-0.05, 0.25, dir.y) : smoothstep(0.0, 0.35, dot(g.out, dir)));
    }
    return best;
}

void ShipBanks::deal(const std::vector<ShipBankAsk>& asks, uint32_t kept) {
    for (int& b : of) b = -1;
    bool used[32] = {}, pooled[32] = {};
    auto own = [&](int home) { return home < 32 && pooled[home]; };   // Z-05: a home past the banks has no bank of its own
    for (int b = 0; b < 32; b++) pooled[b] = !((kept >> b) & 1u);
    pool = 0;
    for (bool b : pooled) pool += b ? 1 : 0;
    std::vector<ShipBankAsk> a;   // each home once, at its nearest
    for (const ShipBankAsk& k : asks) {
        if (k.home < 0 || k.home >= SHIP_HOMES) continue;
        bool merged = false;
        for (ShipBankAsk& e : a) if (e.home == k.home) { e.near = std::min(e.near, k.near); merged = true; }
        if (!merged) a.push_back(k);
        col[k.home] = k.col;
    }
    std::stable_sort(a.begin(), a.end(), [](const ShipBankAsk& x, const ShipBankAsk& y) { return x.near < y.near; });
    asked = (int)a.size(); dealt = dimmed = 0;
    const int take = std::min(asked, pool);   // I-03: the pool's nearest are dealt (their homes first, then the free ones), the rest dim
    for (int i = 0; i < take; i++) if (own(a[i].home) && !used[a[i].home]) { of[a[i].home] = a[i].home; used[a[i].home] = true; dealt++; }
    for (int i = 0; i < asked; i++) {
        if (of[a[i].home] >= 0) continue;
        int free = -1;
        if (i < take) for (int b = 0; b < 32 && free < 0; b++) if (pooled[b] && !used[b]) free = b;
        if (free >= 0) { of[a[i].home] = free; used[free] = true; dealt++; }
        else dimmed++;
    }
}
