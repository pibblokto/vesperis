// The Stardrifter as rows (R-426; docs/ideas/IDEAS-small-and-spacious.md section 2): its rooms on three decks, the ways between them
// (the arches, the spiral stair, the lifter), their windows, what stands in them, its lamps and the places the explorer stands; the
// mesh drawn from them, built once in ship metres (+x starboard, +y up, +z the bow), the walk they keep, the routes through them and
// what the eye sees of them. I-01 made the rows (the 1.4.0 cabin its first plan); I-02 the ship on them: the bridge, the hall, the
// library, the laboratory, the quarters, the hangar, the engine room (R-427: the maintenance room) and the observatory. The devices are
// `Game::cabinDevices`' rows
#pragma once
#include "core/types.h"
#include "core/raster.h"
#include <vector>

// a place the explorer stands in a room and where they look; a view is one by its name (auto-explore's windows, the harness's views)
struct ShipStand { double x, z, yaw, pitch; };
struct ShipView { const char* name; int room; ShipStand at; };

// the rooms, in the plan's order: a room's number is saved (`cabin_place`), so a new one goes before SHIP_ROOMS
enum ShipRoomId { ROOM_BRIDGE, ROOM_HALL, ROOM_LIBRARY, ROOM_LAB, ROOM_QUARTERS, ROOM_HANGAR, ROOM_MAINTENANCE, ROOM_OBSERVATORY, ROOM_STAIR, SHIP_ROOMS };   // R-427: the engine room's place
constexpr ShipView SHIP_START = {"THE START", ROOM_BRIDGE, {0.0, 7.9, 0, 0}};   // where a new expedition stands: the bridge, facing the window

// ---- the rows ---------------------------------------------------------------------------------------------------------------------

// a room's shape: RS_BOX four walls; RS_BOW the bridge's (three walls and the great window's arc across the bow, two steps up from its
// arch); RS_DISC the observatory's (a disc under its glass dome); RS_STAIR the spiral stair (its steps, its column and its rails, drawn
// with the rooms it joins)
enum ShipRoomShape { RS_BOX, RS_BOW, RS_DISC, RS_STAIR };
enum ShipDeck { DECK_LOWER, DECK_MAIN, DECK_TOP };
// a box's sides, the bow's first (a wall's shade is its side's, the hull's factor): the bow, the starboard, the stern, the port
enum ShipSide { SIDE_BOW, SIDE_STARBOARD, SIDE_STERN, SIDE_PORT };
// what of a room is the outer hull (glass when depolarised, B-411): a bit a side, the floor's, the ceiling's
enum ShipOuter { OUT_BOW = 1, OUT_STARBOARD = 2, OUT_STERN = 4, OUT_PORT = 8, OUT_FLOOR = 16, OUT_CEILING = 32 };

// I-03: what a surface is made of: the hull's grey (its bank fixed), warm panels, a darker floor, fabric, brass, plants, glass, the hull
// outside; each but the hull's asks the frame for a bank by its home (a key of the dealing: `ShipBanks`), its colour, its grain's scale
// and weight. I-04: the photographs' one tone (the prints on the quarters' wall), the garden's second green (the home world's two flora
// colours are the plants' and the second's, `Game::finishColour`)
enum ShipFinish { FIN_HULL, FIN_PANEL, FIN_DECK, FIN_FABRIC, FIN_BRASS, FIN_PLANT, FIN_GLASS, FIN_OUTSIDE, FIN_PRINT, FIN_PLANT2, SHIP_FINISHES };
struct ShipFinishRow { const char* name; int home; RGB col; double grainScale, grainWeight; };

// a room: its name, deck and shape; a box's walls (the inner faces: x0 port, x1 starboard, z0 stern, z1 bow), a disc's middle (x0, z0)
// and radius (x1); its floor's and ceiling's heights; its outer hull; its shades (the hull's factors: a side's wall, the floor, the
// ceiling). RS_BOW: where its side walls end and the arc begins (z1), the arc's apex (bowZ), its panes and their lower and upper edges,
// its steps (from its stern wall: their count, rise and depth, the floor below them at `stepFloor`). RS_DISC: the parapet's height and
// the dome's meridians and rings. I-03: its floor's, walls' and ceiling's finishes
struct ShipRoom {
    const char* name;
    int deck, shape;
    double x0, x1, z0, z1, floor, ceiling;
    int outer;
    double wallShade[4], floorShade, ceilingShade;
    double bowZ; int bowPanes; double bowLo, bowHi;
    int steps; double stepRise, stepDepth, stepFloor;
    double parapet; int domeN, domeRings;
    int floorFinish, wallFinish, ceilingFinish;
};

// a way between two rooms. OPEN_ARCH: an open arch in `from`'s wall on `side` (its span along the wall, its height to the head's top, a
// round head), through the slab to `to`'s facing wall; OPEN_WELL: the hole in `from`'s floor down to `to`'s ceiling (the stair's well),
// OPEN_OCULUS the hole in `from`'s ceiling up to `to`'s floor (each a circle at x, z of radius r; seen through, never walked);
// OPEN_STAIR: the stair's landing in `from` at its level (where the stair is walked onto and off); OPEN_LIFTER: the lift up to `to`
// from any other room, down to `from` (Page Up and Page Down, the flight computer's row): the places it leaves the explorer, its lines
enum ShipOpeningKind { OPEN_ARCH, OPEN_WELL, OPEN_OCULUS, OPEN_STAIR, OPEN_LIFTER };
struct ShipOpening {
    int kind, from, to;
    int side; double lo, hi, height; bool round;
    double x, z, r;
    ShipStand up, down;
    const char* upSays;
    const char* downSays;
    const char* upPad;
    const char* downPad;
};

// a window in a box room's wall: its side, its span along the wall and its lower and upper edges, the mullions across it (the bridge's
// arc and the dome are their rooms')
struct ShipWindow { int room, side; double lo, hi, y0, y1; int mullions; };

// the spiral stair (RS_STAIR's room): its middle, its column's and its steps' radii; its landings, a level each from the lowest (the
// hangar's floor, the hall's, the observatory's), the hall's at angle `at` (atan2(x, z): 0 the bow, a quarter turn starboard); a landing
// spans `landing` either side of its angle, the way off it `gap` either side; between the landings the steps, `step` radians each, up
// the angle (clockwise from above). The walk's height by its angle: flat on a landing, a ramp between (the steps' risers drawn)
struct ShipStair {
    double x, z, column, outer;
    int levels; double level[3]; int room[3];
    double at, landing, gap, step;
    int steps[2];   // between the levels: the lowest's and the next's, the next's and the top's
};

// what stands in a room, in the serial order its room draws it: a box (four corners on its floor in order, its floor's height, a
// height, its faces in their order, each with its shade's factor of the hull's), a screen (four corners, the device whose bank and
// shade it takes, a factor of that shade), the capsule's cage (its middle, its bars, the hatch's radius, the capsule's dome), the drone
// rack (drawn by drone_draw.cpp from its box, its screen's edges, where a drone stands on its shelf, its floor, and the drones' way out:
// how far out from the hull and how high they rise), a tube (a square section of half width `radius` from one end to the other: the
// telescope's), a ring standing upright (the drive's: its middle, its axis along z, its outer and inner radii, its depth). A box or a
// tube is solid (the walk kept off its footprint) unless `open`. I-03: its finish, a box's top's own (a bed's plants, a sofa's seat);
// outside the hull (room SHIP_OUTSIDE) the radar's mast (its foot, its height, its dish's radius: the dish turned by the frame). I-04:
// what the frame fills with the explorer's things, each a box's corners where it stands: the shards' shelves (a unit's carcass and its
// boards, `slots` worlds a board), a garden's bed (its plants, `slots` the bed's number), the player's deck (its reels), the kit's locker
// (its doors); the photographs' wall (a box's corners on the wall: `slots` across, `boards` down), the case's display (the instruments
// against its glass: its box's front, `boards` shelves of `slots`), the drive's core under the floor glass (its pit `height` deep). Z-04: the
// laboratory's cold store (its door's lamps, `slots` of them, lit by how full it is). Z-05: the incubator's cabinet (its flasks, `boards`
// shelves of `slots`, behind its glass door) and the jars' shelf (`slots` jars behind its glass). Z-06: the tree of kin's lines over its screen
// (a box's corners on the screen's face, no faces of its own)
enum ShipFixtureKind { FX_BOX, FX_SCREEN, FX_CAGE, FX_RACK, FX_TUBE, FX_RING, FX_MAST, FX_SHELF, FX_BED, FX_DECK, FX_LOCKER, FX_PRINTS, FX_DISPLAY, FX_CORE,
                       FX_STORE, FX_INCUBATOR, FX_JARS, FX_TREE, FX_VEHICLE };   // R-427: a vehicle on its platform (`slots`: 0 the buggy, 1 the drone)
constexpr int INCUBATOR_FLASKS = 12, JAR_PLACES = 8;   // Z-05: the incubator's flasks (three shelves of four), the jars' places
// a box's kinds: its corners and height a box's, its faces drawn as a box's (the boxes' shadows, colliders and tops)
constexpr bool fixtureBoxLike(int kind) {
    return kind == FX_BOX || kind == FX_SHELF || kind == FX_BED || kind == FX_DECK || kind == FX_LOCKER || kind == FX_STORE || kind == FX_INCUBATOR || kind == FX_JARS;
}
// a box's faces by its corners a b c d and their heights (+h): BF_TOP a+h b+h c+h d+h, BF_FRONT b b+h c+h c, BF_SIDE_A a b b+h a+h,
// BF_SIDE_D d c c+h d+h, BF_BOTTOM a b c d, BF_BACK a d d+h a+h
enum ShipBoxFace { BF_NONE, BF_TOP, BF_FRONT, BF_SIDE_A, BF_SIDE_D, BF_BOTTOM, BF_BACK };
struct ShipFixture {
    int room, kind, device;
    double x[4], y[4], z[4];         // a box's corners (y: its floor), a screen's; a tube's ends (0 and 1), a ring's middle (0)
    double height;                   // a box's
    int faces[6]; double shade[6];   // a box's in their order, BF_NONE after the last
    double factor;                   // a screen's
    bool open;                       // the walk passes it (a low panel, a hatch)
    // the cage's: its middle (x[0], z[0]); the bars' radius, count, ends' heights and shade; I-04: the floor's hatch's radius; the
    // capsule's dome's radius, its middle's height and squash, its rings and segments; a tube's half width, a ring's outer radius
    double radius; int bars; double barLo, barHi, barShade;
    double ringR;
    double domeR, domeY, domeSquash; int domeRings, domeSegments;
    // the rack's: its box along x and z, its top, its screen's lower and upper edges, where a drone stands on its shelf, its floor, the
    // drones' way out (how far from the hull, how high); a ring's inner radius and depth; I-04: the display's lowest glass's foot over the
    // case's floor, its height, the step to the next and its inset from the ends (`screenLo`, `screenHi`, `rise`, `depth`)
    double x0, x1, z0, z1, top, screenLo, screenHi, droneX, droneZ, base, outward, rise;
    double inner, depth;
    int finish, topFinish;
    int boards, slots;   // I-04: a shelf's boards and its worlds a board, the prints' rows and columns, the display's shelves and places, a bed's number
};

// what keeps the walk off a place, in its room: a circle (the capsule's cage), a box (a fixture's footprint, the walker's width round
// it: kept to the side it was met from), a ring (a hole's rail: the well, the oculus, the stair's foot) open to the stair's landing
// at `gapAt`, `gapHalf` either side
enum ShipColliderKind { COLLIDE_CIRCLE, COLLIDE_BOX, COLLIDE_RING };
struct ShipCollider {
    int room, kind;
    double x, z, r;
    double x0, x1, z0, z1;
    double gapAt, gapHalf;
};

// a lamp, a point of the light's bank; I-03: its light lies on its room as pools, each vertex of the room's surfaces lit by its lamps
// once with the mesh (the frame scales it by the light's level, U's dimming): a bulb under a ceiling (its pool below it, within `cone` of
// straight down), a glow (a reading lamp, a floor light: all round); its power (at its reach a face toward it takes half of it) and its
// reach (R-427: the engine room's fuel orb went with its ring)
enum ShipLampKind { LAMP_BULB, LAMP_GLOW };
struct ShipLamp { int room, kind; double x, y, z, power, reach, cone; };
// I-04: U's night: the bulbs go out, the glows (the reading lamps, the bedside lamp, the picture lamp, the dome's floor lights) keep this
// share of their light (a vertex's `night`: its glows' light and their share of the fill)
constexpr double SHIP_NIGHT_GLOW = 0.35;

template <class Row> struct ShipRows {
    const Row* row; int count;
    constexpr const Row* begin() const { return row; }
    constexpr const Row* end() const { return row + count; }
    constexpr const Row& operator[](int i) const { return row[i]; }
};
constexpr int SHIP_OUTSIDE = SHIP_ROOMS;   // I-03: the room of what stands outside the hull and of the mesh's outside (never a place)
ShipRows<ShipFinishRow> shipFinishes();
ShipRows<ShipRoom> shipRooms();
ShipRows<ShipOpening> shipOpenings();
ShipRows<ShipWindow> shipWindows();
ShipRows<ShipFixture> shipFixtures();
ShipRows<ShipCollider> shipColliders();   // the fixtures' footprints and the cage's circle, made once, then the holes' rails
ShipRows<ShipLamp> shipLamps();
ShipRows<ShipView> shipViews();
const ShipStair& shipStair();
const ShipView& shipView(const char* name);   // by its name; a name not in the plan: the start
const ShipFixture& shipFixtureOf(int kind);   // the first fixture of a kind (the cage, the rack)
const ShipLamp& shipLampIn(int room, int kind);   // a room's lamp of a kind, else the first of it
const ShipOpening& shipLifter();               // the lift to the observatory
int shipRoomNamed(const char* name);           // -1 when none
double shipRoomNear(int room, const Vec3& p);   // I-03: how far a place is from a room's bounds (0 within)

// ---- the walk ---------------------------------------------------------------------------------------------------------------------

// where the explorer is: a room, a place in it, and on the stair the angle walked round it (unwrapped: its height goes by it)
struct ShipPlace { int room = SHIP_START.room; double x = SHIP_START.at.x, z = SHIP_START.at.z, stairA = 0; };
constexpr double SHIP_EYE = 1.6, SHIP_WALKER = 0.3;   // the eye over the floor; the walker's width each way (kept off walls by it)
// a step of the walk from `p` toward (x, z): kept on the room's floors and through its arches (crossing an arch's middle changes the
// room), off its colliders (in their order), onto the stair at a landing and round it, off it at a landing
void shipWalk(ShipPlace& p, double x, double z);
// the floor's height under the explorer (the bridge's steps and the stair go by place), and the room the eye is in (on the stair the
// hangar's, the hall's or the observatory's by its height)
double shipFloorAt(const ShipPlace& p);
int shipEyeRoom(const ShipPlace& p);
// whether (x, z) at height y is a place the walk keeps (on a floor of the room, off its colliders): the unit's and `ship walk`'s check
bool shipWalkable(const ShipPlace& p);
// a place by a view or a stand, its stair angle found where it stands on the stair
ShipPlace shipPlaceOf(int room, double x, double z);
// the stair's landing in a room: its angle walked round the stair (unwrapped), the stair's `at` for the hall's
double shipLandingAngle(int room);

// ---- the routes (auto-explore) ----------------------------------------------------------------------------------------------------

// the next way to take from room `from` toward room `to` (an index of `shipOpenings`, an arch or a landing): -1 when there or none
int shipNextWay(int from, int to);
// the points a walk through a way passes from `room`: before it (in `room`) and after it (in the room beyond); a landing's after is on
// the stair, the stair's on the landing's room
void shipWayPoints(int way, int room, double& ax, double& az, double& bx, double& bz);

// ---- the sight --------------------------------------------------------------------------------------------------------------------

// what the eye sees of the ship: the rooms through the arches, the well and the oculus from the room it stands in (a few ways deep),
// each with the screen's rows and columns it is seen in (its ways' bounds: a room's pieces filled there only), and whether a window,
// the oculus or the dome is in view (else the space view is not drawn)
struct ShipSight {
    bool seen[SHIP_ROOMS] = {};
    int depth[SHIP_ROOMS] = {};
    double x0[SHIP_ROOMS] = {}, y0[SHIP_ROOMS] = {}, x1[SHIP_ROOMS] = {}, y1[SHIP_ROOMS] = {};
    bool seesOut = false;
    int rooms = 0;
    double ox0 = 0, oy0 = 0, ox1 = 0, oy1 = 0;   // I-03: where the outside is seen (the glass in view within its rooms' boxes; the frame depolarised)
};
constexpr int SHIP_SIGHT_DEPTH = 4;
void shipSight(const ShipPlace& p, const Vec3& eye, const Mat3& look, const Proj& pj, bool depolarised, ShipSight& out);

// ---- the mesh ---------------------------------------------------------------------------------------------------------------------

// a piece of the mesh, its room's (drawn when its room is seen; the outside's, SHIP_OUTSIDE, when the outside is), in its room's serial
// order: a polygon or a line in ship metres, its material, its finish (its bank as the frame dealt it), the device whose screen it is, its
// shade's factors, the way it faces (where it is seen from: a face turned away is not drawn, `cull`); or a fixture drawn by its own code
// (the rack, the mast). MAT_HULL: the outer hull, glass when depolarised (B-411; its shade the light's x a x b, times the glass's sheen);
// MAT_SOLID: the inner walls and what stands in the rooms (never glass; the light's x a x b); MAT_SCREEN: a device's screen (its bank as
// dealt, the screens' grain; the device's shade in the frame x a); MAT_PLAIN: no grain (the cage's bars, the rails; the light's x a
// x b); MAT_PANE: a window's glass, written to
// depth only (the outside behind it, nothing of the ship drawn through it); I-03: MAT_TINT, a glass finish (the room behind it tinted,
// the light caught on it); MAT_OUTSIDE, the hull outside (lit by the sun and the parked world; glass with the hull depolarised, so not
// drawn); MAT_FEATURE, what stands outside it (lit so, drawn whatever the hull); MAT_GLOW, the drive's ring's inner face (the light's bank,
// the drive's pulse). I-04: what the frame fills with the explorer's things where it comes in its room's order, each its fixture's
// (`fixture`): the shards on a shelf, the photographs, a bed's plants, the drive's core, the instruments in the case, the deck's reels,
// the capsule in its cradle and the hatch under it; Z-04: the cold store's door's lamps; Z-05: the incubator's flasks, the jars; Z-06: the tree
enum ShipPieceKind { PIECE_POLYGON, PIECE_LINE, PIECE_RACK, PIECE_MAST, PIECE_SHELF, PIECE_PRINTS, PIECE_GARDEN, PIECE_CORE, PIECE_DISPLAY, PIECE_DECK, PIECE_CAPSULE,
                     PIECE_STORE, PIECE_INCUBATOR, PIECE_JARS, PIECE_TREE, PIECE_VEHICLE };   // R-427: a vehicle on its platform
enum ShipMaterial { MAT_HULL, MAT_SOLID, MAT_SCREEN, MAT_PLAIN, MAT_PANE, MAT_TINT, MAT_OUTSIDE, MAT_FEATURE, MAT_GLOW };
struct ShipPiece {
    int room = 0, kind = PIECE_POLYGON, mat = MAT_HULL, finish = FIN_HULL, bank = 0, device = 0;
    double a = 1, b = 1;
    bool cull = false;
    Vec3 nrm;               // the way it faces (none for a line)
    int first = 0, n = 0;   // its vertices in the mesh's
    int fixture = -1;       // I-04: the fixture a piece the frame fills is (`shipFixtures`' index)
};
// a vertex: its place, its grain's u and v (a screen's v is y x 3 and the screens' clock, made in the frame); I-03: the light its room's
// lamps give it (the bulbs' and the glows' with the room's fill), fixed; I-04: the glows' share of it (what U's night keeps)
struct ShipVert { Vec3 p; double u = 0, v = 0, lamp = 0, night = 0; };
// I-03: where the sun comes in: a window's light between its mullions, a pane of the great window, the oculus (its outline, the way it
// faces out), or the dome (the observatory's floor lit but in the parapet's shadow)
struct ShipGlass { int room = 0, n = 0; Vec3 p[16]; Vec3 out; bool dome = false; };
// I-03: a surface the sun's patch falls on: a room's floor, walls and ceiling, a box's top (its outline, convex, the way it faces in), its
// shade's factor; on a floor the room's boxes cast their shadows
struct ShipReceiver { int room = 0, n = 0; Vec3 p[16]; Vec3 nrm; double a = 1; bool floor = false; };
struct ShipMesh {
    std::vector<ShipPiece> pieces; std::vector<ShipVert> verts;
    int first[SHIP_ROOMS + 2] = {};        // a room's pieces: [first[r], first[r + 1]); the outside's last
    unsigned finishes[SHIP_ROOMS + 1] = {};   // the finishes a room's pieces take (a bit a finish)
    std::vector<ShipGlass> glass;
    std::vector<ShipReceiver> receivers;
    double fill[SHIP_ROOMS] = {}, fillNight[SHIP_ROOMS] = {};   // I-04: a room's fill (its surfaces' light given back), the glows' share of it
};
const ShipMesh& shipMesh();   // built once from the rows
// I-04: what a room's lamps give a place facing `nrm` (none: lit all round), as the mesh's vertices take it: the bulbs' and the glows' with
// the room's fill, the glows' share of it, the orb's (what the frame fills is lit so)
void shipLampLight(int room, const Vec3& p, const Vec3* nrm, double& lamp, double& night);

// ---- the light (I-03) -------------------------------------------------------------------------------------------------------------

// a patch of the sun's light: its outline on a surface (ship metres) and the shade it adds there, in the surface's own bank
struct ShipPatch { int room = 0, n = 0; Vec3 p[16]; double shade = 0; };
// the patches the sun (toward it, ship metres; `light` its strength, 0 none) casts in a room: each of its glass's outline cast along the
// light onto each surface facing it, cut to the surface, a floor's boxes' shadows taken out
void shipPatches(int room, const Vec3& sun, double light, std::vector<ShipPatch>& out);
// a room's sight of the outside through its glass toward a direction (0 none, 1 a window facing it): the parked world's light on its walls
double shipGlassToward(int room, const Vec3& dir);

// ---- the banks --------------------------------------------------------------------------------------------------------------------

// I-02: the banks dealt in a frame. What asks for one (a seen device's screen, the drones' paint and lamps; I-03: a seen room's finishes,
// the hull outside) asks by its home bank, its colour and its nearness; the hull's and the light's are fixed. I-03: the pool's nearest are
// dealt, each its home bank when that is in the pool, else the first free one; past the pool a screen dims and a finish falls to the hull's
// grey (`bankOf` -1). The pool is every bank but those kept: 0 (the drones' dark, the cleared frame), the hull's, the light's and (B-437)
// looking out the outside's one (`cabinDeal`: what shows through the glass is in it, read through the space view's own palette)
struct ShipBankAsk { int home; RGB col; double near; };
// Z-05: the homes are keys: a bank's number (its own when the pool has it) or past them (SHIP_LIFE_HOMES on: the lab's cultures' tones,
// always a free bank)
constexpr int SHIP_HOMES = 48;
struct ShipBanks {
    int of[SHIP_HOMES];      // a home's dealt bank, -1 dimmed or not asked
    RGB col[SHIP_HOMES];     // a home's colour as asked
    int asked = 0, dealt = 0, dimmed = 0, pool = 0;
    void deal(const std::vector<ShipBankAsk>& asks, uint32_t kept);   // kept: a bit a bank left out of the pool
    int bankOf(int home) const { return home >= 0 && home < SHIP_HOMES ? of[home] : -1; }
};
