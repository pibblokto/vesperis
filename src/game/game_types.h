// The game's types beside `Game` (game.h): its states, the cabin and the ship, and the free functions of the game's
// files that the harness calls too
#pragma once
#include "drone_state.h"
#include "audio.h"
#include "core/font.h"
#include "core/types.h"
#include "peoples/forms.h"
#include "peoples/instruments.h"
#include "peoples/shards.h"
#include "galaxy/starfield.h"
#include "surface/surface_view.h"
#include <cstdint>
#include <string>
#include <vector>

enum class GameState { TITLE, SPACE, LANDING_MAP, DESCENT, SURFACE, ASCENT, HELP, MENU, SYSTEM_LIST, DATA, SETTINGS, SLOTS,
                       GUIDE, TEXT_ENTRY, STAR_MAP, LOG, STATS, GALLERY, SHIPSCREEN, CONSOLE, SECTOR_MAP, KEYS, SHARDS, ALMANAC, PROBE, RECORDING,
                       INSTRUMENTS,    // C-11: the instruments' case
                       DRONES,         // the drone bay's screen
                       AUTO };         // auto-explore's profiles

extern const char* const SCAN_MODE_NAMES[];   // R-408: the scanner's modes by LandmarkKind ("PEAKS", ...; game/marks.cpp)
// U-03: the ways in. A menu is a list of rows in its order, its count read from the list (`rowsOf`), and the harness finds a
// row by its label (U-01): `Game::escMenu` and `settingRows` (menus.cpp), `guideMenu` (guide_screens.cpp), `shipPages`
// (cabin.cpp)
class Game;
struct Settings;
template <class Row> struct RowList {
    const Row* row; int count;
    constexpr const Row* begin() const { return row; }
    constexpr const Row* end() const { return row + count; }
};
template <class Row, int N> constexpr RowList<Row> rowsOf(const Row (&rows)[N]) { return {rows, N}; }
// a menu's row: its label and what Enter does on it; the guide menu's: the status that refuses it on the ground (opened from
// the surface), and a label that names (the star, the world; empty for its label)
struct MenuRow {
    const char* label;
    void (*act)(Game& g);
    const char* ground = nullptr;
    std::string (*name)(const Game& g) = nullptr;
};
// a setting's row: its label, its value's text and its change by Left and Right (Enter: Right), the settings applied after;
// a row that opens a screen has `open` instead (CONTROLS). BACK is drawn after the rows, not one of them
struct SettingRow {
    const char* label;
    std::string (*value)(const Settings& s);
    void (*change)(Game& g, Settings& s, int dir) = nullptr;
    void (*open)(Game& g) = nullptr;
};
// a row of the flight computer's: its label, what Enter does, its status column (the devices' page), and whether its act runs
// on the screen, which stays open (else at the window: SPACE)
struct ShipRow {
    const char* label;
    void (*act)(Game& g);
    std::string (*status)(const Game& g) = nullptr;
    bool stays = false;
};
// the flight computer's pages, each its name and its rows
struct ShipPage { const char* name; RowList<ShipRow> rows; };
// each row whole, asserted where its list is made: a label and what Enter does (a setting's: its value, and its change or the
// screen it opens; a page's: its name and its rows whole)
constexpr bool rowWhole(const MenuRow& r) { return r.label && r.act; }
constexpr bool rowWhole(const SettingRow& r) { return r.label && r.value && (r.change || r.open); }
constexpr bool rowWhole(const ShipRow& r) { return r.label && r.act; }
template <class Row> constexpr bool rowsWhole(const RowList<Row>& l) {
    for (int i = 0; i < l.count; i++)
        if (!rowWhole(l.row[i])) return false;
    return true;
}
constexpr bool rowWhole(const ShipPage& p) { return p.name && p.rows.count > 0 && rowsWhole(p.rows); }

// E-06: what each state is: its row in `Game::STATE_ROWS` (states.cpp, U-02), in GameState's order, holds the traits the frame,
// the radio, the save and the controller read, the controller's context, the name the harness prints and the methods the frame
// runs; `stateHas`, `statePad` and `stateName` read it. Unit `states` holds its first 29 rows to the lists they replaced.
enum StateTrait : unsigned {
    ST_SIMULATES = 1 << 0,     // the clock runs under it: the time warp, an almanac's run
    ST_OVERLAY = 1 << 1,       // drawn over the scene it was opened from (`renderUnder`), unless its row's `covers` says not now
    ST_STOWS = 1 << 2,         // the ship's cameras are put away on it: the radar camera off, the telescope stowed
    ST_TAKES_SYNTH = 1 << 3,   // the dark radio is silent: the synth is the screen's, the probe's, the landing's or the title's
    ST_IN_PLAY = 1 << 4,       // a save places the explorer by it (on any other, by `returnState`)
    ST_MOUSE = 1 << 5,         // the mouse is captured: it looks round, or moves a map's cursor
    ST_PHOTO = 1 << 6,         // the photo tools work (Ctrl+P, Ctrl+Shift+P, Ctrl+R): the scenes, the probe's camera, a recording
    ST_TYPING = 1 << 7,        // keys type: the global keys rest, the controller's keyboard stays up
    ST_GROUND = 1 << 8,        // on the ground, or on the capsule's way down to it or back up
    ST_SHIP_MOVES = 1 << 9,    // U-02: the ship moves on after its update, by the frame's clock (the star map, the cabin's screens)
    ST_BASE = 1 << 10,         // U-04: a base, which the screens opened from it go back to (`returnState`): the title, the scenes
    ST_CLOCK = 1 << 11,        // U-04: the time warp and the almanac are at hand (T, Ctrl+T): the scenes and the landing map
};
// the controller's contexts (pad.cpp): what its sticks and buttons do. A state's row names its own; the ground's and the case's
// are refined as the explorer walks, drives or flies, or plays (`padContext`)
enum PadCtx { PCX_SCREEN, PCX_TITLE, PCX_SHIP, PCX_FOOT, PCX_BUGGY, PCX_DRONE, PCX_PROBE, PCX_RECORDING, PCX_INSTRUMENT };
constexpr int STATE_COUNT = (int)GameState::AUTO + 1;   // AUTO is the last state
// U-05: where a state's Esc goes, its row's `back` (`Game::closeScreen` follows it): a state's number, the scene it is drawn
// over (`returnState`: the title, space or the ground), or the screen it was opened from (`openScreen` records it); a scene, none
enum StateBack { BACK_NONE = -1, BACK_SCENE = -2, BACK_OPENER = -3 };
// a state's trait, its controller's context and its name, read from its row (a value that is no state has no trait, the screen's
// context, "?")
bool stateHas(GameState s, unsigned trait);
int statePad(GameState s);
const char* stateName(GameState s);

// F-05: what the cabin's bands share, taken once a frame (`drawCabin`): the eye, the light, the screens' flickers, the decoder's wait, and
// the drones on the rack (where each stands, its pose and its model)
struct CabinFrame {
    Mat3 look; Vec3 eye;
    double lit = 0, hullShade = 0, flick = 1, breathe = 1, sweep = 1, glow = 1, shine = 0;
    bool waiting = false, ready = false;
    bool here[DRONE_COUNT] = {}; double x[DRONE_COUNT] = {}, lift[DRONE_COUNT] = {}, blur[DRONE_COUNT] = {};
    DronePose pose[DRONE_COUNT];
    std::vector<DronePrim> prims[DRONE_COUNT];
};

// what the explorer faces in the cabin, E using it (`Cabin::facing`; its row in `Game::cabinDevices`): nothing, the GOES console, the flight computer, the
// star map's, the landing map's and the target's screens, the capsule's cage, the shard decoder, the signal radar, the telescope, the
// instruments' case, the drone bay
enum CabinFacing { FACE_NOTHING = 0, FACE_GOES, FACE_FLIGHT, FACE_STAR_MAP, FACE_LANDING, FACE_TARGET, FACE_CAGE, FACE_DECODER, FACE_RADAR, FACE_TELESCOPE, FACE_CASE, FACE_BAY, FACE_COUNT };
// what a text entry names (`Game::textKind`): the star, a world, a note for the log, a star to find, a sector to go to, a mark, a piece
// of music, a star of the sky (the telescope's), a storm, a place of the galaxy
enum TextKind { TEXT_NONE = 0, TEXT_STAR, TEXT_WORLD, TEXT_NOTE, TEXT_FIND_STAR, TEXT_SECTOR, TEXT_MARK, TEXT_PIECE, TEXT_SKY_STAR, TEXT_STORM, TEXT_PLACE };

// M2: the explorer inside the Stardrifter
struct Cabin {
    double x = 0.4, z = -1.6, yaw = 0, pitch = 0;   // ship-local metres; yaw 0 looks out of the front window
    bool light = true, depolarised = false, onRoof = false;
    double lightLevel = 1;
    int facing = FACE_NOTHING;   // what E would use (a CabinFacing)
};

// the cabin's banks: its hull, the flight computer's screen, its light (cabin.cpp; drone_draw.cpp draws the rack in them)
const int BANK_HULL = 8, BANK_SCREEN = 9, BANK_LIGHT = 10;
// U-03: a device's body against a wall, which the explorer is kept off: pushed back to its face, along the wall between `from`
// and `to` (a screen in the wall has none)
enum CabinWall { WALL_NONE, WALL_LEFT, WALL_BACK, WALL_FRONT };
struct CabinBlock { int wall = WALL_NONE; double face = 0, from = 0, to = 0; };
// U-03: a cabin device, a row a CabinFacing in its order (`Game::cabinDevices`, cabin.cpp; its picture is drawCabinBand's):
// where E uses it from (the spot most ahead within its reach is the one faced), its line under the crosshair, the controller's
// command ("USE " and it), its screen's bank and colour, what E does at it, its body, and its line while it works (the radar on,
// a probe falling; null: `hud`). R-404: every computer in a colour of its own, so the one wanted is found at a glance (9 the
// flight computer's green, 21-29 the others, the cage's light no screen); the prompt under the crosshair takes the colour too
struct CabinDevice {
    int facing;
    double x, z, reach;
    const char* hud;
    const char* use;
    int bank; RGB col;
    void (*act)(Game& g);
    CabinBlock block = {};
    const char* (*hudNow)(const Game& g) = nullptr;
};
// the devices whole: a row a facing, in its order, each with its lines and its act
constexpr bool devicesWhole(const RowList<CabinDevice>& l) {
    if (l.count != FACE_COUNT) return false;
    for (int i = 0; i < l.count; i++)
        if (l.row[i].facing != i || !l.row[i].hud || !l.row[i].use || !l.row[i].act) return false;
    return true;
}

// the view's forward from its yaw and pitch (auto-explore's and the telescope's)
inline Vec3 forwardOf(double yaw, double pitch) { return Vec3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)); }

struct ShipState {
    Vec3 pos;
    double yaw = 0, pitch = 0;
    enum Mode { STANDBY, VIMANA, APPROACH, PARKED };
    Mode mode = STANDBY;
    bool hasRemote = false;
    Star remote;
    int localTarget = -1;
    int parkedBody = -1;
    Vec3 parkDir;             // unit vector from body centre to the ship (world)
    double parkDist = 0;      // km
    bool orbiting = true;
    Vec3 flightFrom, flightTo;
    double flightT = 0, flightDur = 1;
    Vec3 approachDir0; double approachD0 = 0;   // R-419: the fine approach's start from its world (unit, km): the distance closed by ratios, the direction swung early
    bool targeting = false;   // remote crosshair mode
    double aimYaw0 = 0, aimPitch0 = 0;   // orientation at approach start
    // a belt as the local target or the parking place. The ship parks beside one rock of the belt, named
    // by its co-rotating cell and index so the same rock is there again after a load, six rock radii off on its sunlit side
    int targetBelt = -1, parkedBelt = -1;
    int64_t rockIr = 0, rockIa = 0, rockIy = 0; int rockM = 0;
    double rockDist = 0;      // km from the rock's centre
};

// what the harness sets that the game's code reads: Game's `harness` (TestGame, src/harness/test_game.h, holds the hooks)
struct HarnessKnobs {
    bool probeQuiet = false;                                 // X-02: the probe's frames without the camera's shake and the link's static
    bool aimOverride = false; double aimLat = 0, aimLon = 0;   // the aim for C, as if the telescope's reticle were there
};

// C-17: an instrument's parts drawn as it lies, seen from above, in a box of the canvas's logical units (the case's picture, the harness's
// sheets: game/instrument_draw.cpp): lit from the upper left, its courses glowing as `glow` says, a ruler of its size when `ruler`
void drawInstrumentParts(RGBCanvas& c, int x0, int y0, int w, int h, const std::vector<InstrumentPart>& parts, const FormPalette& pal, const std::vector<double>& glow, double t, bool ruler);

// H-06: a record's kind (RC_*, peoples/records.h) and its glyph on the decoder's pages (a page, a note, a star; game/history_screen.cpp)
int recordKindOf(const Shard& s);
void drawRecordKindGlyph(RGBCanvas& fb, int x, int y, int kind, uint32_t col);

// Q-03: the sounds of the ground the surface cues (the dunes' hum, the ice's song, a quake's heave and rumble) into the synth's levels, as much
// as the air carries (game/surface_play.cpp; the harness's scenes hear them through it too)
void groundSoundsTo(SurfaceView& surf, AudioState& audio);
