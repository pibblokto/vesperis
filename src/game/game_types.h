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
#include <vector>

enum class GameState { TITLE, SPACE, LANDING_MAP, DESCENT, SURFACE, ASCENT, HELP, MENU, SYSTEM_LIST, DATA, SETTINGS, SLOTS,
                       GUIDE, TEXT_ENTRY, STAR_MAP, LOG, STATS, GALLERY, SHIPSCREEN, CONSOLE, SECTOR_MAP, KEYS, SHARDS, ALMANAC, PROBE, RECORDING,
                       INSTRUMENTS,    // C-11: the instruments' case
                       DRONES,         // the drone bay's screen
                       AUTO };         // auto-explore's profiles

extern const char* const SCAN_MODE_NAMES[];   // R-408: the scanner's modes by LandmarkKind ("PEAKS", ...; game/marks.cpp)
// U-01: a menu's labels in its order, by which the harness finds a row (U-03 makes each menu rows): the Esc menu's and the
// settings' (game/menus.cpp; the settings' BACK is drawn after theirs), the guide menu's (game/guide_screens.cpp) and the
// flight computer's three pages (game/cabin.cpp)
struct MenuLabels { const char* const* label; int count; };
extern const MenuLabels MENU_LABELS, SETTINGS_LABELS, GUIDE_LABELS, SHIP_PAGE_LABELS[3];

// E-06: what each state is, a row a state in GameState's order: the traits the frame, the radio, the save and the controller
// read, the controller's context, the name the harness prints. A new state appends its row (the count and the order are
// checked below); unit `states` holds its first 29 rows to the lists they replaced.
enum StateTrait : unsigned {
    ST_SIMULATES = 1 << 0,     // the clock runs under it: the time warp, an almanac's run
    ST_OVERLAY = 1 << 1,       // drawn over the scene it was opened from (`returnState`): the render switch's `default`
    ST_STOWS = 1 << 2,         // the ship's cameras are put away on it: the radar camera off, the telescope stowed
    ST_TAKES_SYNTH = 1 << 3,   // the dark radio is silent: the synth is the screen's, the probe's, the landing's or the title's
    ST_IN_PLAY = 1 << 4,       // a save places the explorer by it (on any other, by `returnState`)
    ST_MOUSE = 1 << 5,         // the mouse is captured: it looks round, or moves a map's cursor
    ST_PHOTO = 1 << 6,         // the photo tools work (Ctrl+P, Ctrl+Shift+P, Ctrl+R): the scenes, the probe's camera, a recording
    ST_TYPING = 1 << 7,        // keys type: the global keys rest, the controller's keyboard stays up
    ST_GROUND = 1 << 8,        // on the ground, or on the capsule's way down to it or back up
};
// the controller's contexts (pad.cpp): what its sticks and buttons do. A state's row names its own; the ground's and the case's
// are refined as the explorer walks, drives or flies, or plays (`padContext`)
enum PadCtx { PCX_SCREEN, PCX_TITLE, PCX_SHIP, PCX_FOOT, PCX_BUGGY, PCX_DRONE, PCX_PROBE, PCX_RECORDING, PCX_INSTRUMENT };
struct StateRow { GameState state; const char* name; unsigned traits; int pad; };
inline constexpr StateRow STATE_ROWS[] = {
    {GameState::TITLE, "TITLE",             ST_STOWS | ST_TAKES_SYNTH, PCX_TITLE},
    {GameState::SPACE, "SPACE",             ST_SIMULATES | ST_IN_PLAY | ST_MOUSE | ST_PHOTO, PCX_SHIP},
    {GameState::LANDING_MAP, "LANDING_MAP", ST_SIMULATES | ST_STOWS | ST_TAKES_SYNTH | ST_IN_PLAY, PCX_SCREEN},
    {GameState::DESCENT, "DESCENT",         ST_SIMULATES | ST_STOWS | ST_TAKES_SYNTH | ST_IN_PLAY | ST_MOUSE | ST_GROUND, PCX_SCREEN},
    {GameState::SURFACE, "SURFACE",         ST_SIMULATES | ST_STOWS | ST_IN_PLAY | ST_MOUSE | ST_PHOTO | ST_GROUND, PCX_FOOT},
    {GameState::ASCENT, "ASCENT",           ST_SIMULATES | ST_STOWS | ST_TAKES_SYNTH | ST_IN_PLAY | ST_MOUSE | ST_GROUND, PCX_SCREEN},
    {GameState::HELP, "HELP",               ST_OVERLAY, PCX_SCREEN},
    {GameState::MENU, "MENU",               ST_OVERLAY, PCX_SCREEN},
    {GameState::SYSTEM_LIST, "SYSTEM_LIST", ST_OVERLAY, PCX_SCREEN},
    {GameState::DATA, "DATA",               ST_OVERLAY, PCX_SCREEN},
    {GameState::SETTINGS, "SETTINGS",       ST_OVERLAY, PCX_SCREEN},
    {GameState::SLOTS, "SLOTS",             ST_OVERLAY, PCX_SCREEN},
    {GameState::GUIDE, "GUIDE",             ST_OVERLAY, PCX_SCREEN},
    {GameState::TEXT_ENTRY, "TEXT_ENTRY",   ST_OVERLAY | ST_TYPING, PCX_SCREEN},
    {GameState::STAR_MAP, "STAR_MAP",       ST_OVERLAY | ST_MOUSE, PCX_SCREEN},
    {GameState::LOG, "LOG",                 ST_OVERLAY, PCX_SCREEN},
    {GameState::STATS, "STATS",             ST_OVERLAY, PCX_SCREEN},
    {GameState::GALLERY, "GALLERY",         ST_OVERLAY, PCX_SCREEN},
    {GameState::SHIPSCREEN, "SHIPSCREEN",   ST_SIMULATES | ST_OVERLAY, PCX_SCREEN},
    {GameState::CONSOLE, "CONSOLE",         ST_SIMULATES | ST_OVERLAY | ST_TYPING, PCX_SCREEN},
    {GameState::SECTOR_MAP, "SECTOR_MAP",   ST_OVERLAY | ST_STOWS | ST_MOUSE, PCX_SCREEN},
    {GameState::KEYS, "KEYS",               ST_OVERLAY, PCX_SCREEN},
    {GameState::SHARDS, "SHARDS",           ST_SIMULATES | ST_OVERLAY | ST_STOWS | ST_TAKES_SYNTH, PCX_SCREEN},
    {GameState::ALMANAC, "ALMANAC",         0, PCX_SCREEN},
    {GameState::PROBE, "PROBE",             ST_SIMULATES | ST_STOWS | ST_TAKES_SYNTH | ST_IN_PLAY | ST_PHOTO, PCX_PROBE},
    {GameState::RECORDING, "RECORDING",     ST_TAKES_SYNTH | ST_PHOTO, PCX_RECORDING},
    {GameState::INSTRUMENTS, "INSTRUMENTS", ST_SIMULATES | ST_OVERLAY | ST_STOWS | ST_TAKES_SYNTH, PCX_INSTRUMENT},
    {GameState::DRONES, "DRONES",           ST_SIMULATES | ST_OVERLAY | ST_STOWS | ST_TAKES_SYNTH, PCX_SCREEN},
    {GameState::AUTO, "AUTO",               ST_OVERLAY, PCX_SCREEN},
};
constexpr int STATE_COUNT = (int)GameState::AUTO + 1;   // AUTO is the last state
static_assert(sizeof(STATE_ROWS) / sizeof(STATE_ROWS[0]) == STATE_COUNT, "a row a state");
constexpr bool stateRowsInOrder() {
    for (int i = 0; i < STATE_COUNT; i++)
        if ((int)STATE_ROWS[i].state != i) return false;
    return true;
}
static_assert(stateRowsInOrder(), "each row in its state's place");
// a state's trait, its controller's context and its name (a value that is no state has no trait, the screen's context, "?")
inline bool stateHas(GameState s, unsigned trait) { const unsigned i = (unsigned)s; return i < (unsigned)STATE_COUNT && (STATE_ROWS[i].traits & trait); }
inline int statePad(GameState s) { const unsigned i = (unsigned)s; return i < (unsigned)STATE_COUNT ? STATE_ROWS[i].pad : (int)PCX_SCREEN; }
inline const char* stateName(GameState s) { const unsigned i = (unsigned)s; return i < (unsigned)STATE_COUNT ? STATE_ROWS[i].name : "?"; }

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

// what the explorer faces in the cabin, E using it (`Cabin::facing`; SCREENS' rows): nothing, the GOES console, the flight computer, the
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
// R-404 (2026-10-06): every computer in a colour of its own, so the one wanted is found at a glance: by `facing` (6 is
// the cage, 0 nothing), the bank its screen is drawn in (9 the flight computer's green, 21-27 the others) and its colour;
// the prompt under the crosshair takes the colour too
struct ScreenLook { int bank; RGB col; };
const ScreenLook SCREENS[FACE_COUNT] = {
    {BANK_SCREEN, RGB(0.25f, 0.9f, 0.55f)},   // 0: nothing faced
    {21, RGB(1.0f, 0.72f, 0.25f)},            // 1 the GOES console: amber
    {BANK_SCREEN, RGB(0.25f, 0.9f, 0.55f)},   // 2 the flight computer: the green phosphor
    {22, RGB(0.4f, 0.55f, 1.0f)},             // 3 the star map screen: blue
    {23, RGB(1.0f, 0.45f, 0.8f)},             // 4 the landing map screen: pink (an orange sat too near the radar's red)
    {24, RGB(0.3f, 0.95f, 0.9f)},             // 5 the target data screen: cyan
    {BANK_LIGHT, RGB(1.0f, 0.92f, 0.75f)},    // 6 the capsule cage: no screen
    {25, RGB(0.75f, 0.45f, 1.0f)},            // 7 the shard decoder: violet
    {26, RGB(1.0f, 0.35f, 0.3f)},             // 8 the signal radar: red
    {27, RGB(0.8f, 0.9f, 1.0f)},              // 9 the telescope: ice
    {28, RGB(0.72f, 1.0f, 0.3f)},             // 10 the instruments' case (C-11): lime
    {29, RGB(1.0f, 1.0f, 0.45f)},             // 11 the drone bay (C-18): a pale yellow
};

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
