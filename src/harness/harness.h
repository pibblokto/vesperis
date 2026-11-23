#pragma once
// E-03: what the harness's files share: the engine's headers, the harness's own types (Mode, Check, UnitGroup, SceneCode) and the
// shared ones, and each file's functions that another file calls, under the file's name. A function used by one file only is
// static in it. src/harness/CLAUDE.md maps the files; `vesperis_test help` lists the modes with theirs.
#include "core/framebuffer.h"
#include "core/raster.h"
#include "core/font.h"
#include "core/png.h"
#include "core/parallel.h"
#include "core/fs.h"
#include "galaxy/starfield.h"
#include "life/morphs.h"
#include "life/seeding.h"
#include "galaxy/system.h"
#include "galaxy/planetmap.h"
#include "galaxy/planet_features.h"
#include "galaxy/drainage.h"
#include "galaxy/landmarks.h"
#include "galaxy/almanac.h"
#include "galaxy/fronts.h"
#include "galaxy/nights.h"
#include "peoples/ruins.h"
#include "peoples/shards.h"
#include "peoples/music.h"
#include "peoples/voice.h"
#include "peoples/signals.h"
#include "peoples/charts.h"
#include "peoples/peoples.h"
#include "peoples/buildings.h"
#include "peoples/plan.h"
#include "peoples/history.h"
#include "peoples/towns.h"
#include "peoples/works.h"
#include "peoples/records.h"
#include "peoples/genres.h"
#include "harness/fingerprint.h"
#include <set>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <sys/stat.h>
#include <sstream>
#include <dirent.h>
#if defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>   // the process's CPU time (`load`, `soak`)
#include <unistd.h>
#endif
#ifdef __APPLE__
#include <mach/mach.h>       // its threads and resident memory
#endif
#include <cmath>
#include <complex>
#include <map>
#include <functional>
#include <optional>
#include <thread>
#include <limits>
#include "space/space_view.h"
#include "surface/surface_view.h"
#include "core/input.h"
#include "game/game.h"
#include "game/ui.h"
#include "harness/test_game.h"   // TestGame, Game with its hooks

// Two RGB frames compared pixel for pixel (`pngdiff`, `bothways`, the unit's both-ways checks): how many pixels differ, the largest step of
// one channel, the first differing pixel, the rows they span and how many fall in each eighth of the frame's height
struct FrameDiff { long differ = 0; int step = 0, firstX = -1, firstY = -1, rowMin = -1, rowMax = -1; long eighths[8] = {0, 0, 0, 0, 0, 0, 0, 0}; };

// E-03: the scenes' codes (CmpScene::wantMat, setupSceneForType's last argument): what the finder looks for, by family;
// the values are the magic numbers they replaced (a range is a family: SCENE_BIOME - BIO_*, the fronts, the nights, the ages,
// the dark); a code from 0 up is a material to stand on (MAT_SAND)
enum SceneCode : int {
    SCENE_ANY = -1,   // the type's default spot
    SCENE_HERD = -3,   // a herd in view
    SCENE_FOREST = -4,   // a deep forest (three fifths of the ground within 200 m)
    SCENE_HERD_NEAR = -5,   // a herd near
    SCENE_HERD_FAR = -6,   // a herd far
    SCENE_CREATURES = -7,   // one of each land species
    SCENE_BUGGY_SEAT = -8,   // the buggy from its seat
    SCENE_BUGGY_PARKED = -9,   // the buggy parked, from 5 m
    SCENE_BIOME = -10,   // a site of one biome: SCENE_BIOME - BIO_*
    SCENE_PINNED_MOUNTAIN = -20,   // the pinned mountain site
    SCENE_RIVER = -21,   // a river
    SCENE_LAKE = -22,   // a lake
    SCENE_STAINED_CRACK = -30,   // a stained crack (europan)
    SCENE_LAVA_FISSURE = -31,   // a lava fissure (tectonic)
    SCENE_DUNES = -32,   // dunes (desert)
    SCENE_METHANE_SHORE = -33,   // a methane shore
    SCENE_RAYED_PLAIN = -34,   // a rayed plain
    SCENE_ACID_SHORE = -35,   // an acid shore
    SCENE_GEYSERS = -36,   // a geyser basin at full strength
    SCENE_MAGMA_SHORE = -37,   // the magma sea
    SCENE_STEAM_VENT = -38,   // a steam vent's sinter
    SCENE_BRINE_POOL = -39,   // a brine pool
    SCENE_AURORA = -40,   // an aurora
    SCENE_AURORA_ZENITH = -41,   // under the auroral oval
    SCENE_AURORA_CURTAIN = -42,   // the curtain overhead
    SCENE_NEUTRON_GLASS = -43,   // a neutron star's glassed world
    SCENE_PROTOSTAR = -44,   // a protostar's world
    SCENE_WOLF_RAYET = -45,   // a Wolf-Rayet star's world
    SCENE_BLACK_HOLE = -46,   // a black hole's world
    SCENE_TOWN = -47,   // a people's town
    SCENE_DEAD_SEA = -48,   // a dead desert world's old shore
    SCENE_SHARD = -49,   // a shard within reach
    SCENE_HARBOUR = -50,   // a harbour
    SCENE_TERRACES = -51,   // terraces
    SCENE_ROAD = -52,   // a road
    SCENE_GRAVES = -53,   // graves
    SCENE_SECOND_PEOPLE = -54,   // the second people's side of a divide
    SCENE_GLASS_FLOW = -55,   // a glass flow
    SCENE_RUST_VALLEY = -56,   // a rust valley
    SCENE_ARCHEAN_FLATS = -57,   // archean flats
    SCENE_TUNDRA_ICE = -58,   // tundra ice
    SCENE_LICHEN = -59,   // a lichen world's moss
    SCENE_FRONT_FAR = -60,   // a front far off (setupFrontScene)
    SCENE_FRONT_ARRIVING = -61,   // a front arriving
    SCENE_FRONT_THROUGH = -62,   // a front passing
    SCENE_FRONT_BEHIND = -63,   // a front gone by
    SCENE_METEOR_SHOWER = -64,   // a meteor shower (setupNightScene)
    SCENE_ZODIACAL_LIGHT = -65,   // the zodiacal light
    SCENE_EARTHSHINE = -66,   // earthshine
    SCENE_MOONLIT = -67,   // a moonlit night
    SCENE_HOTHOUSE_SWAMP = -68,   // a hothouse swamp
    SCENE_STEPPE = -69,   // a steppe plain
    SCENE_AGE_PREHISTORIC = -70,   // a prehistoric people's ruins (setupAgeScene)
    SCENE_AGE_MEDIEVAL = -71,   // medieval
    SCENE_AGE_INDUSTRIAL = -72,   // industrial
    SCENE_AGE_MODERN = -73,   // modern
    SCENE_AGE_FUTURISTIC = -74,   // futuristic
    SCENE_AGE_FAR_FUTURE = -75,   // far future
    SCENE_EYEBALL_SHORE = -76,   // an eyeball world's shore
    SCENE_TERMINATOR_RING = -77,   // the terminator ring
    SCENE_HYCEAN_SEA = -78,   // a hycean sea
    SCENE_AMMONIA_SHORE = -81,   // an ammonia shore
    SCENE_SNOWBALL_BELT = -82,   // a snowball's belt
    SCENE_CHEMOTROPH_VENTS = -83,   // chemotroph vents
    SCENE_SCORCHED_SEABED = -84,   // a scorched seabed
    SCENE_DEAD_FOREST = -85,   // a frozen or scorched forest
    SCENE_NITROGEN_CELLS = -86,   // nitrogen cells
    SCENE_CRYO_PLUME = -87,   // a cryovolcanic plume
    SCENE_SPIDERS = -88,   // dry ice spiders
    SCENE_WANDERER_SNOW = -89,   // a wanderer's snow
    SCENE_BRINE_FACULAE = -90,   // brine faculae
    SCENE_FROST_DWARF = -91,   // a frost dwarf
    SCENE_BRINE_MOUNTAIN = -92,   // the brine dwarf's mountain
    SCENE_GROOVED_LANES = -93,   // a grooved moon's lanes
    SCENE_TWO_FACED_BOUNDARY = -94,   // a two-faced moon's boundary
    SCENE_TWO_FACED_RIDGE = -95,   // a two-faced moon's ridge
    SCENE_TIGER_STRIPES = -96,   // tiger stripes
    SCENE_PATCHWORK_CLIFF = -97,   // the patchwork's cliff
    SCENE_SPONGE_PITS = -98,   // sponge pits
    SCENE_SINGING_DUNES = -99,   // singing dunes
    SCENE_SINGING_ICE = -100,   // singing ice
    SCENE_QUAKES = -101,   // a quake
    SCENE_ROGUE_LAMP = -102,   // the lamp on a dark drifter (setupDarkScene)
    SCENE_ROGUE_EMBERS = -103,   // a drifter's embers
    SCENE_ROGUE_FROZEN_AIR = -104,   // a frozen air
    SCENE_DARK_OCEAN = -105,   // a dark ocean
    SCENE_SPRING_BANDS = -106,   // Z-03: a terraced hot spring's banded pools
    SCENE_RED_SNOW = -107,   // Z-03: red snow
    SCENE_MATS = -108,   // Z-03: a microbial mat on a wet flat
    SCENE_HUNT = -109,   // J-07: a hunter's chase
    SCENE_FLOCK = -110,   // J-07: a flock overhead
    SCENE_SWIMMERS = -111,   // J-07: a swimmer at the surface off a shore
};

// M7-03 unit tests: frame maths, the sun against analytic cases, noise statistics, name lengths, the Kepler solver
// the receiver's renders (the unit and `signals wav`): a people's world (its voice, tongue, shards, language and tradition),
// a programme of it through the synth at full clarity with a loop's and a numbers station's repeats as the game does them, a
// natural signal by its seed, and a WAV writer
struct PeopleWorld {
    bool valid = false;
    StarSystem sys;
    int body = -1;
    Lore lore;
    Tongue tongue;
    Voice voice;
    std::vector<Shard> shards;
    Language lang;
    Tradition tradition;
    int firstText = -1, firstMusic = -1;
    uint64_t seed = 0;
    std::string name;
};

struct RenderStats { double rms = 0, peak = 0; int nan = 0, clip = 0; };

// C-16: what the receiver is told besides the signal: the dial, the sky, the wear, the age on the air, the signal under it, the
// whistle and the ring, how clear the signal is (1 at full; a far signal locked 0.6) and C-15's overdrive; `music` the piece's slot
struct RxOpts {
    double gainT = 0, sky = 0, wear = 0, clarity = 1, drive = 0; int age = -1; bool music = false; int slot = -1;
    int kind2 = -1, voice2 = -1, age2 = -1; uint64_t seed2 = 0; double pulse2 = 1, sig2 = 0, het = 0, hetHz = 800, intermod = 0;
    RogueVoice rogue, rogue2; int voice = -1, set = RADIO_CAMERA; double level = 1, hum = 0;   // a drifter's (and its beacon), the radio it is heard on, the ship's hum beside it
};

// D-03: a drifter as the voice's synth takes it (its class, turning, moon and power), from its place: its system generated
struct RogueRef { Star star; Signal sig; RogueVoice voice; };

// D-05: a drifter's beacon through the receiver as the radar plays it locked: the drifter's voice, its beacon's machine under it, and its radio
// people's last recording (its opening) spoken slowed and worn, three times and the gap
struct BeaconInfo { bool valid = false; DrifterBeacon B; std::string people, genre, opening, theirs; int words = 0; double speechS = 0, gap = 0; };

// D-03: the distinctness: the drifters' voices rendered over a breath each (its turning: the main lobe, the figure and the answer, the silence;
// 32 s at least) against `ref` signals of each natural kind by seeds of their own over `refSeconds` (the workers share them out); the
// drifters' nearest pair (prints and parameters), and each kind's median pair, the bar
struct Distinctness {
    int n = 0, pairs = 0, under = 0, nearA = -1, nearB = -1, traitA = -1, traitB = -1;
    double nearest = 1e9, median = 0, traitNearest = 1e9;
    double kindMedian[4] = {0, 0, 0, 0}, kindNearest[4] = {1e9, 1e9, 1e9, 1e9};
    double bar = 0;
    std::vector<int> inUnder;
};

// Q-03: the sounds of the ground through the synth as the game sets them (22,050 Hz mono, master 0.8, nothing else playing): each a setter of the
// game's levels by the second, rendered in the synth's blocks. The singing dunes as a slide down a slip face (the hum rising over a second, held
// three, then dying as the sand runs on: the surface's 2.5 s), at the note of an Earth's grains and of three other gravities (Titan's, a Mars'
// under its thin air, a heavy world's); the singing ice at its most (dusk) and at its least (noon); a quake struck at 0.3 s and dying as the
// surface's (4 s), under an air, through the boots of an airless world, and on a tectonic world (its lava's rumble rising with it, R-307's)
struct GroundSound { const char* name; double seconds; std::function<void(AudioState&, double)> set; const char* what; };

// Q-03: a scene's sound as the game hears it: the view run `seconds` at 30 frames a second (walking ahead the first `walkS`, the controls on), each
// frame's levels of the sounds of the ground through the game's own mapping (`groundSoundsTo`) into the synth: the cue's sound alone; the cues'
// top levels and the ice's sounds begun
struct SceneSound { AudioSynth synth; AudioState st; std::vector<int16_t> pcm; double dunes = 0, ice = 0, quake = 0; SceneSound() { st.master = 0.8; } };

// The three test places: a landable moon of a ringed gas giant, a locked world with a moon, and a plain world (tilted,
// unlocked, with an atmosphere and a moon), the first of each in the scan; latitude 12, longitude 0.5 rad, the start's
// clock. `almanac` prints their lists (and a ship's at the giant), `unit` re-reads every event with the site's own sun
// and frame at the time given.
struct AlmanacTestPlace { StarSystem sys; int body = -1; double latDeg = 12, lonDeg = 0.5 / DEG; const char* what = ""; };

// X-03: a descent's layers along its clock: runs of a stage and its deck (the deck the probe is in, or the one over a clear
// band), each named for the frames ("ammonia", "water", "1" for the band under the first deck)
struct LayerRun { int stage = 0, deck = -1; double from = 0, to = 0; std::string tag; };

// A-04: the peoples near home, the nearest first: every star within `R` sectors of home (the cube round it, its sphere), its desert and
// felisian worlds' peoples (`peoplesOf`), each with its seed (`peopleGen`), its age and its years since the end
struct NearPeople { int64_t x, y, z; double d; int body, which; uint64_t seed; std::string world; int age; double years; };

// R-420: the platform's side of auto-explore's tape as the harness plays it: a file begun "plays" (if it is there) at the frames' pace, `len`
// seconds long, its end reported (begun at its very end, it is over at once, as the platform's); the deck's sequence taken as the synth
// takes it, and counted
struct TapePump {
    bool playing = false; double len = 10, from = 0; std::string file; int decks[DECK_KINDS] = {}; uint32_t lastSeed = 0;
    void step(AudioState& st, double dt) {
        if (st.deck > 0) { decks[st.deck]++; lastSeed = st.deckSeed; }
        st.deck = 0;
        if (st.musicGo) {
            st.musicGo = false; playing = false; st.musicT = -1;
            if (st.music.empty()) return;
            FILE* f = fopen(st.music.c_str(), "rb");
            if (!f) { st.musicFailed = true; return; }
            fclose(f);
            file = st.music; from = st.musicFrom; st.musicLen = len;
            if (from > 0 && from >= len - 2) { st.musicEnded = true; return; }
            playing = true; st.musicT = from;
        } else if (playing && (st.musicT += dt) >= len) { playing = false; st.musicT = -1; st.musicEnded = true; }
    }
};

static const char* const CLASS_NAMES[3] = {"FLAT", "HILLS", "MOUNTAINS"};

struct SiteMetrics {
    int cls = 0;                                   // the landing map's class: 0 flat, 1 hills, 2 mountains
    double probe = 0;                              // its slope probe
    double med4 = 0;                               // median slope of the 4 m grid
    std::vector<double> slopes16, slopes512;       // every cell's slope (pooled per class by the table)
    double relief770 = 0, relief2k = 0, relief16k = 0;
    double floors = 0;                             // share of the lowest quarter of the 64 m cells under a slope of 0.05
    double usPerSample = 0;                        // the 16 m grid's cost, when timed
    double hiYaw = 0;                              // toward the highest ground within a kilometre (the gallery looks there)
    double lat = 0, lon = 0;
    int material = MAT_ROCK;
};

struct Sheet {
    int w, h; std::vector<uint32_t> px; RGBCanvas cv;
    Sheet(int W, int H) : w(W), h(H), px((size_t)W * H, rgb(12, 12, 14)), cv{px.data(), W, H, 1} {}
    void blit(const std::vector<uint32_t>& src, int sw, int sh, int ox, int oy) {
        for (int y = 0; y < sh; y++) for (int x = 0; x < sw; x++) { int X = ox + x, Y = oy + y; if (X >= 0 && Y >= 0 && X < w && Y < h) px[(size_t)Y * w + X] = src[(size_t)y * sw + x]; }
    }
    void label(int x, int y, const std::string& s, uint32_t c = rgb(255, 220, 120)) {
        fillRectRGB(cv, x, y, x + textWidth(s.c_str()) + 3, y + 8, rgb(0, 0, 0));
        drawText(cv, x + 2, y + 1, s.c_str(), c);
    }
    void save(const std::string& fn) { writePNG(fn.c_str(), px.data(), w, h); printf("wrote %s\n", fn.c_str()); }
};

// `plan <sx> <sz> <body> [second]`: the world's plan over its map for each age (the people as if it had reached it, over one reading of the
// world), and the plan the game builds (the people's own age), with the build times; `plan ages`: the six age worlds' own plans
static const uint32_t* const PLAN_WAY_COLOURS = NET_KIND_COLOURS;   // peoples/plan.h's (R-415: the landing map's too)


// a people's places, its works and its countryside round its capital, with their records (the harness's and the checks' one walk)
struct RecordPlace { RuinSpec spec; std::vector<ShardSite> sites; int work = -1; bool countryside = false; std::vector<InstrumentSite> instruments; };   // C-11: and the instruments beside its music

// E-03: a check (the unit's, and a `<mode> check` run's): `  <name> ok|FAIL <detail>`, the name padded as the unit prints it or not
// (a mode's run, modeCheck), flushed (A-03: a slow check shows where it is), and counted (the checks when it has a count); with
// `failsTo` (`unit -q`) only a failure is printed, there
struct Check {
    int* checks;
    int* fails;
    FILE* failsTo = nullptr;
    bool padded = true;
    void operator()(const char* name, bool ok, const std::string& detail = "") const {
        if (!failsTo) { printf(padded ? "  %-34s %s %s\n" : "  %s %s %s\n", name, ok ? "ok" : "FAIL", detail.c_str()); fflush(stdout); }
        else if (!ok) { fprintf(failsTo, padded ? "  %-34s %s %s\n" : "  %s %s %s\n", name, "FAIL", detail.c_str()); fflush(failsTo); }
        if (checks) (*checks)++;
        if (!ok) (*fails)++;
    }
};

struct UnitGroup { const char* name; const char* file; int scale, mush; void (*run)(const Check& check); };   // UNIT_GROUPS' rows

struct QuietStdout {   // `unit -q`: stdout to /dev/null while the groups run; `saved` is the stdout it was (the failures go there)
    FILE* saved = nullptr;
    int fd = -1;
    explicit QuietStdout(bool on);
    void restore();
    ~QuietStdout() { restore(); }
};

// E-03: a mode of vesperis_test (main.cpp's MODES): its name, help's group, its arguments, one line, its file, and what
// main() runs for it
struct Mode { const char* name; const char* group; const char* usage; const char* help; const char* file; int (*run)(int argc, char** argv); };

// ---- main.cpp
extern int g_testScale;

// ---- frames.cpp
double nowSec();
void saveFB(Framebuffer& fb, const char* path, double gain = 1.0);
void warmMaps(SpaceRenderer& sr, const StarSystem& sys, int only, double t);   // the season at t set first (B-438)
uint64_t fnv(uint64_t h, const void* p, size_t n);
double effectiveSunAlt(const SurfaceSite& site, double t);
double findTime(SurfaceSite& site, double targetAlt, double t0, double* errOut = nullptr);
FrameDiff diffFrames(const uint32_t* a, const uint32_t* b, int w, int h);
std::string diffLine(const FrameDiff& d, int w, int h);
void writeDiffImage(const char* path, const uint32_t* a, const uint32_t* b, int w, int h);
void applyTestScale(Game& g);
void pressKey(Game& game, Input& in, int key);
void runFrames(Game& game, Input& in, double secs);
void runFramesAtLeastOne(Game& game, Input& in, double secs);
bool selectRow(TestGame& game, Input& in, const std::string& label);

// ---- audio_util.cpp
RenderStats renderReceiver(AudioSynth& synth, AudioState& st, double seconds, std::vector<int16_t>* pcm, const std::function<void(double)>& between = nullptr);
bool writeWav16(const std::string& fn, const std::vector<int16_t>& pcm, int sr);
void fftInPlace(std::vector<std::complex<double>>& a);
double spectralPeak(const std::vector<float>& x, double from, double lo, double hi);
double pcmPitch(const std::vector<int16_t>& pcm, int sr, double t0, double t1, double fmin, double fmax);

// ---- scenes_setup.cpp
bool pinnedPlainFront(StarSystem& sys, int& bi, double t0, FrontForecast& fc, double window = 60 * 86400.0);   // W-03, with the scene finders
bool setupSceneForType(int type, double latDeg, double alt, double yawOff, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut, int wantMat = -1);
std::string surfaceFrontInfo(const SurfaceView& sv);

// ---- scenes_special.cpp
bool stageElsewhere(StarSystem& sys, int bi, int type);              // P-04: a kind made where no system is, staged on a world of the scan
bool plumeStand(const StarSystem& sys, int bi, Vec3& vent, double& lat, double& lon);                 // P-05: a cryovolcanic world's plume and where to see it from
bool springStand(const StarSystem& sys, int bi, double& t0, Vec3& vent, double& lat, double& lon);    // P-05: a dry ice world's spring and a spider's jet
bool curtainStand(const StarSystem& sys, int bi, Vec3& vent, double& lat, double& lon);               // P-06: a tiger stripe's middle and where to see its jets from
bool setupPinnedMountainScene(double alt, double yawOff, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut);
bool setupFrontScene(int stage, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut);   // W-03, with the pinned sites below
bool setupNightScene(int which, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut);   // W-04: the shower, the zodiacal light, the earthshine, the moonlit night
bool setupAgeScene(int age, double alt, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut);   // A-03: a settlement of each age on its test world
bool setupDarkScene(int which, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut);   // D-04: the drifters' ground in the dark

// ---- modes_galaxy.cpp
void testStars();
void runGalaxy();
int runChart();
int runFlightStability();
int runCrossing();
void runHome();
void parkedNeighbours(const StarSystem& sys, int bi, double t, double& skyDeg, double& viewDeg);
void runSurvey(int wanted);

// ---- modes_space.cpp
void renderMaps();
void renderSpace();
double telescopeFrame(SpaceRenderer& sr, StarNeighborhood& nb, StarSystem& sys, double t, const Vec3& shipPos, const Vec3& aim, double zoom, int bankA, int bankB, const char* file,
        double* rpxOut = nullptr, bool plate = true);
void renderMoonSky();
void renderBinarySurface(SpaceRenderer& sr, StarNeighborhood& nb);
void almanacTestPlaces(std::vector<AlmanacTestPlace>& out);
AlmanacPlace almanacGroundPlace(const SurfaceSite& site, double t);
AlmanacPlace almanacShipPlace(const StarSystem& sys, int body, double t);
int runShowers(int argc, char** argv);
int runAlmanac(int argc, char** argv);
void testSeasons();

// ---- modes_probe.cpp
std::vector<LayerRun> probeLayerRuns(const TestGame& game, double endC);
int runProbe(int argc, char** argv);
int runProbeGiants();
int runProbeStability();

// ---- modes_surface.cpp
void renderSurface(int onlyType);
void runConsistency();
int runDrive();
int runBestiary();
int runBestiaryPlates(int worlds);   // modes_bestiary.cpp (J-06)
int runBestiaryWays();               // modes_bestiary.cpp (J-07)
bool setupWaysScene(SurfaceView& sv, int wantMat, double& t);   // J-07: the hunt's, the flock's and the swimmers' scenes staged (modes_bestiary.cpp)
// J-07 (shared with J-08's `seed fates`): a place for a way (0 open dry ground, 1 dry ground off the forest, 2 water 3 m deep, 3 a shallow shore)
// at a time its sun stands 25 to 55 degrees up (or night's 12 under); a frame at 2x mushed to a file; a herd of a species put down round a point
bool waysSite(StarSystem& sys, int bi, int need, bool night, double& lat, double& lon, double& t, int salt = 0);
void waysShot(SurfaceView& sv, StarNeighborhood& nb, SpaceRenderer& sr, double t, const std::string& fn);
void waysHerd(SurfaceView& sv, int spi, int count, double cx, double cz, uint64_t seed);
int seedFates();   // J-08: `seed fates` (modes_seed_fates.cpp)
int seedKinds();   // J-11: `seed kinds` (modes_seed_kinds.cpp): the made kinds of forty worlds and six lines, the shares, each kind drawn
int seedRecord();  // J-12: `seed record` (modes_seed_record.cpp): a seeded world's record in the catalogue and on the LIFE page, `record/`
int seedCold();    // J-13: `seed cold` (modes_seed_cold.cpp): forty worlds' warmth on the ground and down, lines tuned for each on the ground and deep
int seedClouds();  // J-14: `seed clouds` (modes_seed_clouds.cpp): the cloud decks' heights, a cloud line let go in them, its clouds dyed from orbit
// J-14: a cloud line's species: the first venusian worlds' first cloud species that lives on what the clouds give (light, dead matter); and a
// GLOW gene moved in from the first glowing species of the forty worlds (its donor read), as a player moves one
bool seedCloudSource(StarSystem& cs, int& cb, int& csp);
bool seedGlowDonor(GeneEdit& glow);
// J-13: a line's edits as a player would make them for a place (its warmth's best nearest the place's, brine and its frost cut where only a brine
// stays liquid, DRY where the ground is dry, a SHIELD where the dose asks): species `sp` of body `lb`
std::vector<GeneEdit> seedTunedEdits(const StarSystem& ls, int lb, int sp, const SeedVerdict& place);
// a sheet of frames side by side, each halved (the mean of four), `cols` to a row (J-09's plates too)
void writeSheet(const std::vector<std::vector<uint32_t>>& cells, int w, int h, int cols, const std::string& fn);
int overgrowthPlates();   // J-09: `overgrowth plates` (modes_overgrowth.cpp)
// J-09: an overgrowth made by hand at its fullest (`overgrowth plates`, the unit, the bench), and a layer of one line it covers the whole world
// from; the first of `seed climate`'s flora worlds with a place at a forest's edge (or, `shore`, a shore) by day, and a time at night there
MadeGrowth growthOfType(int type, int texture, const RGB& colour, double grain, double wet, int hard, bool glow, bool hair);
MadeWorld growthLayer(double radiusKm, const Vec3& place, const MadeGrowth& G);
struct GrowthGround { StarSystem sys; int body = -1; double lat = 0, lon = 0, day = 0, night = 0; };
bool growthGround(GrowthGround& P, bool shore);
int overgrowthWorld();    // J-09: `overgrowth world`
int overgrowthRuins();    // J-10: `overgrowth ruins`
// J-10: an overgrowth's layer at its line's step (its steps, age and reach as the seeding makes them), let go at `place`; a layer put on a view's
// ground and ruins (its tiles sampled again, its ruins' cover grown again)
MadeWorld growthAtStep(double radiusKm, const Vec3& place, MadeGrowth G, int step);
void growthLayerOn(SurfaceView& S, const MadeWorld& m);
void runEncounters(int wanted);
void renderLandingMaps();
int runStability(const char* want, double walk);
void renderCompare();
void composeSheets();
void renderGallery(int type);
void testReanchor();
int runReanchorComet(int argc, char** argv);
int runScene(int argc, char** argv);
int runCapsule(int argc, char** argv);
int runSpot(int argc, char** argv);

// ---- modes_terrain.cpp
bool firstBodyOfType(int type, StarSystem& sysOut, int& biOut);   // O6-03: the first body of a type in the scan region (defined with forTypeBodies below)
void landSitesOf(const StarSystem& sys, int bi, uint64_t salt, int n, std::vector<std::pair<double, double>>& out);   // random land sites of a body (defined below)
// B-404: the share of points round (lat, lon) whose material differs between the 4 and 16 m samples (out[0]) and the 16
// and 64 m samples (out[1]) of the planet function, over a square of +-halfM at stepM; the mode `matlod` measures all
// five ring scales, the unit test guards the walked rings' agreement with this
void materialAgreement(const StarSystem& sys, int bi, double lat, double lon, double halfM, double stepM, double out[2]);
void gridHeights(const SurfaceSite& site, int N, double cell, std::vector<double>& h);
SiteMetrics measureSite(const SurfaceSite& site, bool timeIt);
int runDescent(double latDeg, double lonDeg);
extern int g_terrainType;
int runTerrainStats();
void hillshade(const SurfaceSite& site, int N, double cell, double zFactor, std::vector<uint32_t>& px);
double renderSiteFrame(const StarSystem& sys, int bi, double lat, double lon, double yaw, double alt, double pitch, SpaceRenderer& sr, StarNeighborhood& nb, std::vector<uint32_t>& px);
void renderLandforms(int type, int wantCls);
void renderLandformSites(bool search);
int runMatlod(int argc, char** argv);
int runLandmarks(int argc, char** argv);
int runDrain(int argc, char** argv);
int runWater(int argc, char** argv);
int runClimate(int argc, char** argv);
int runLandforms(int argc, char** argv);

// ---- modes_kinds.cpp
int runDataSheet(int argc, char** argv);
int runTides(int argc, char** argv);
void renderTraits(const char* want);
int runKinds(int argc, char** argv);
int runSaltScan(int argc, char** argv);

// ---- modes_signals.cpp
bool peopleWorldOf(const Star& s, int bi, PeopleWorld& w);
double gainShareOf(int g);
RenderStats renderProgramme(const PeopleWorld& w, int p, double seconds, std::vector<int16_t>* pcm, Programme* progOut = nullptr, const RxOpts& o = RxOpts());
RenderStats renderKind(int kind, uint64_t seed, double pulseHz, double seconds, std::vector<int16_t>* pcm, const RxOpts& o = RxOpts(), int* events = nullptr, int* eventKinds = nullptr);
RogueRef rogueRefOf(const Star& d);
RenderStats renderRogue(const RogueRef& r, double seconds, std::vector<int16_t>* pcm, RxOpts o = RxOpts());
RenderStats renderBeacon(const Star& d, double seconds, std::vector<int16_t>* pcm, BeaconInfo* info, RxOpts o = RxOpts());
Distinctness rogueDistinctness(const std::vector<RogueRef>& D, int ref, double refSeconds, int firstKind = SIG_PULSAR);
std::vector<RogueRef> roguesNearest(const Vec3& at, int count);
std::vector<GroundSound> groundSounds();
RenderStats renderGroundSound(const GroundSound& gs, std::vector<int16_t>* pcm, int* iceEvents = nullptr, std::vector<float>* raw = nullptr);
void sceneSoundRun(SurfaceView& sv, double& t, double seconds, double walkS, SceneSound& ss);
bool firstPeopleWorld(PeopleWorld& w, Signal* sigOut = nullptr);
int runSignals(int argc, char** argv);
int runVoice(int argc, char** argv);
int runSounds(int argc, char** argv);

// ---- modes_rogues.cpp
int runRogues(int argc, char** argv);

// ---- modes_life.cpp, unit_life.cpp
int runLife(int argc, char** argv);
std::vector<Star> starsNearHome(int r);   // the stars within r sectors of home (a cube), nearest first
bool coastSite(const BodyGen& g, double radiusKm, double& latDeg, double& lonDeg);   // Z-07: land 200 m inland of a sea's coast (`life auto`, the flow)
// ---- modes_life_look.cpp
int runLifeLook(int argc, char** argv);
std::vector<Morph> everyForm();   // every plan's every variant, of home's genesis (the forms' sheets, the unit)
void unitLife(const Check& check);
// ---- modes_seed.cpp (J-02: the seeded worlds, `seed test`; J-03: `seed worlds`, `seed sea`); unit_seeding.cpp: the unit's `seeding` group
int runSeed(int argc, char** argv);
void unitSeeding(const Check& check);
void unitAnimals(const Check& check);
// J-03: a line of a world's species as the game reads one (its form, its genes, its world genes; its edits applied, their price), keyed as the
// guide keys it; and a seeding of it at a place (its record's fields), worked out (`seedingRun`)
SeedLine seedLineFor(const StarSystem& sys, int body, int species, const std::vector<GeneEdit>& edits);
Seeding seedingFor(const StarSystem& sys, int body, const SeedLine& line, double lat, double lon, double clock, const std::vector<Seeding>* earlier = nullptr);
// the six lines `seed worlds` sends (ITERATION-1.5.0.md J-03): a species wild; warm and cold (its warmth gene six weights heavier or
// lighter); a dye (DYE red, its weight the heaviest); a methane-maker (GAS methane); STOP cut. Their edits on a species' genes
enum SeedLineKind { SL_WILD = 0, SL_WARM, SL_COLD, SL_DYE, SL_METHANE, SL_STOP, SL_COUNT };
extern const char* const SEED_LINE_NAMES[SL_COUNT];
std::vector<GeneEdit> seedLineEdits(const MicrobeGenes& g, int kind);
// the lines' species: the first felisian world's first sea light-feeder (else its first that lives on the dead), wild
bool seedSource(StarSystem& ls, int& lb, int& sp, SeedLine& wild);
// a world's place for a seeding: its sea nearest the equator where it has one (the world map's scale), else a lake's (true), else a hashed
// place of its land (false); the forty worlds `seed worlds` and the unit send to (the first of each landable kind) and their places
bool seedPlaceFor(const StarSystem& sys, int body, double& lat, double& lon);
struct SeedCase { StarSystem sys; int body = -1; double lat = 0, lon = 0; bool wet = false; };
bool seedPlaceTaking(const StarSystem& sys, int body, const SeedLine& line, double& lat, double& lon);   // its sea's first place where a line takes
std::vector<SeedCase> seedCases();
// ---- modes_seed_air.cpp (J-04: `seed climate`): edits setting world genes (each {kind, value}: a table gene to the run reading the value, its
// weight the heaviest; a weight gene, value -1, the heaviest); a world's own line (its sea's first light-feeder, else dead-eater, else chemical-
// eater, wild); the worlds of the living and the wet kinds (the first of each, then the second, the third: n at most) with their own line and
// its sea's place where it takes
int seedClimate();
int seedTraits();   // J-04: each made air's trait from orbit and on the ground, before and after (`made_<trait>_<orbit|ground>_<before|after>.png`)
std::vector<GeneEdit> seedGeneEdits(const MicrobeGenes& g, const std::vector<std::pair<int, int>>& wants);   // (J-05: a value of -2 cuts the gene)
// ---- modes_seed_looks.cpp (J-05: `seed looks`, each made waters', ground's and plants' trait from orbit and on the ground, before and after:
// `look_<trait>_<map|orbit|ground>_<before|after>.png`)
int seedLooks();
bool seedNativeLine(const StarSystem& sys, int body, SeedLine& wild);
struct SeedAirCase { StarSystem sys; int body = -1; double lat = 0, lon = 0; SeedLine wild; };
// J-15: a greened desert's line: the first light-feeder of water with a green pigment living on the forty worlds (`seedCases`), tuned for the first
// desert's place of them (`madeGreens`: its warmth, DRY) where it takes, with `wants` set on top (`seedGeneEdits`), let go there at 0 (`world`'s
// wild its line); false where none takes. `seed kinds`' recipe and the unit's
bool seedDesertLine(const std::vector<std::pair<int, int>>& wants, SeedAirCase& world, std::vector<GeneEdit>& edits, Seeding& sd);
std::vector<SeedAirCase> seedAirCases(int n, int rounds = 3);
// J-12: the world and line `seed record` and the unit's record checks send: of `seed climate`'s forty worlds with plants and animals, the first
// where its own sea line given a nerve poison (its STOP cut and its cells bound: it overgrows too) takes and drives a native out by its 64th
// step, else the poison alone; a game with it let go at the place (an edited strain of the explorer's, `<world>/M<i>/S1`), parked at the
// world, the guide's clock at the line's 64th step
struct SeedRecordCase { SeedAirCase c; std::vector<GeneEdit> edits; SeedLine line; double end = 0; };
bool seedRecordCase(SeedRecordCase& out);
void seedRecordGame(TestGame& g, const SeedRecordCase& rc, const std::string& name);
// a game's flask filled with a line in a medium, the lab's clock stepped a minute at a time till it shows FULL (false: not within four hours);
// a kind's best medium (its cell, its best warmth, its salt, its light and its energy)
bool labFlaskFull(TestGame& g, int i, const FlaskSpecies& w, const Medium& m);
Medium seedBestMedium(const CultureKind& k);
// ---- modes_lab.cpp: the laboratory (Z-04: `lab microscope`, `lab time`); unit_lab.cpp: the unit's `lab` group
int runLab(int argc, char** argv);
bool labVial(const StarSystem& sys, int body, Vial& out);   // a vial of a world's ground as the kit takes it (its most abundant living species of a drop)
bool labDenseVial(Vial& out);   // the densest slide the lab knows (`lab time`'s, the bench's microscope row)
void unitLab(const Check& check);
void unitLabAnswers(const Check& check);   // R-435 to R-438, B-441: the group `experiments`
void unitLabIncubator(const Check& check);   // B-442, R-440 to R-442: the group `incubator`
void unitLabGenes(const Check& check);   // the Y series (Y-01: the trees by world and by folder): the group `genes`
// a gene's letters a weight `d` heavier (a letter at a time, the first that can), a table's run that reads `want` (its first `n` letters)
std::string heavier(const GeneCode& c, std::string p, int d);
std::string lighter(const GeneCode& c, std::string p, int d);   // (`heavier`'s other way: modes_seed.cpp)
std::string runReading(const GeneCode& c, const std::vector<int>& t, int want, int n);
void unitLabWorld(const Check& check);   // J-01: the world genes in the lab, the group `world`
// ---- modes_cultures.cpp: the incubator's (Z-05: `lab cultures`)
int labCultures();
int labStock(TestGame& g);   // the lab stocked for the pictures and the bench: twelve flasks of a living world's species
int labWorldLooks(TestGame& g, std::vector<std::string>& names);   // J-01: the twelve flasks with the world genes' looks
// ---- modes_genes.cpp (Y-02: `lab genes`): the first living world of each alphabet and hand; the genes' laws (the unit's `genes` group)
struct GeneWorld { int letters = 4; bool mirrored = false; StarSystem sys; int body = -1; std::vector<Morph> forms; };
bool geneWorlds(GeneWorld (&out)[6]);
struct GeneLaw { bool ok = false; std::string detail; };
struct GeneLaws { GeneLaw worlds, deterministic, itself, asCode, vitalQuiet, fixed, moved, wild, cellAsBefore, worldCarried, worldPrice; };   // Y-04: wild; J-01: the last three
GeneLaws geneLaws();
int labGenes();
// ---- modes_tree.cpp (Z-06; Z-07's `lab catalogue`)
int treeKin(int r);
int lifeMade(int want);
int labTree();
int labCatalogue();

// ---- modes_drones.cpp
void droneChecks(const std::function<void(const char*, bool, const std::string&)>& check);
int runDrones(int argc, char** argv);

// ---- modes_ship.cpp
int runShip(int argc, char** argv);
// I-03: a frame's banks whole: each dealt bank one home's (no bank two hues), none 0, the hull's or the light's, looking out never the
// outside's (B-437: 7, read through the space view's own palette); `used` the banks the ship's palette takes (the dealt, 0, the two
// fixed and the outside's one), at most 32
bool shipBanksWhole(const CabinFrame& f, int& used, std::string& why);

// ---- modes_game.cpp
void runGameFlow();
void testSettings();
void testAudio();
int runZoom(int argc, char** argv);
int runParkings();
int runTape(int argc, char** argv);
int runTail(int argc, char** argv);
int runAuto(int argc, char** argv);

// ---- modes_input.cpp
int runFuzz(int frames);
int testInput();

// ---- modes_tour.cpp
int runTour(int argc, char** argv);
bool tourSetupNamed(GameState s);   // U-06: the tour opens the state as the game does (a scene, a world set up first), not by its row

// ---- modes_bench.cpp
void fillBenchGuide(Guide& G);
int runBenchForest();
int runBenchOvergrowth();   // J-09: `bench overgrowth`
int runBench(bool check, double slack);
int runPool();
int runBenchNoise();
int runLoad(int argc, char** argv);
int runSoak(int argc, char** argv);

// ---- modes_proof.cpp
int runRegress(bool bless);
int runPngDiff(int argc, char** argv);
int runBothWays(int argc, char** argv);
int runSunCone();
int runScreens(int argc, char** argv);

// ---- civ_forms.cpp
FormPalette testPalette();
void placeCity(SurfaceView& sv, double x, double z, uint64_t seed, int* nBuildings = nullptr, int* nForms = nullptr);
void formsCamera(SurfaceView& sv, double eyeH, double tx, double ty, double tz, double t);
void formsChecks(const std::function<void(const char*, bool, const std::string&)>& check);
int runForms(int argc, char** argv);

// ---- civ_genome.cpp
bool formRests(const std::vector<Form>& el, size_t i);
std::vector<NearPeople> peoplesNearHome(int want, int R = 25);
Culture cultureWithGenome(const BodyGen& g, int age, double years, uint64_t peopleSeed);
void genomeChecks(const std::function<void(const char*, bool, const std::string&)>& check);
int runGenome(int argc, char** argv);

// ---- civ_walls.cpp
void wallsChecks(const std::function<void(const char*, bool, const std::string&)>& check);
int runWalls(int argc, char** argv);

// ---- civ_plan.cpp
void planChecks(const std::function<void(const char*, bool, const std::string&)>& check);
void townChecks(const std::function<void(const char*, bool, const std::string&)>& check);
int runPlan(int argc, char** argv);
int runTowns(int argc, char** argv);
int runRoads(int argc, char** argv);

// ---- civ_works.cpp
void landWorksOf(const BodyGen& g, int people, int nPlaces, double radiusM, int nPoints, std::vector<std::shared_ptr<const Work>>& out);
void worksChecks(const std::function<void(const char*, bool, const std::string&)>& check);
int runWorks(int argc, char** argv);

// ---- civ_peoples.cpp
// C-13: the peoples' helpers (defined with the `peoples` mode, below): a land point deep on a people's side of the divide, the `second` argument
Vec3 peopleSidePoint(const BodyGen& g, int people, double wantLat);
bool frontierPoint(const BodyGen& g, double& lat, double& lon);
bool planPlacePoint(const BodyGen& g, int people, double& lat, double& lon, bool coast = false);   // T-01: a town of a people's plan
int takePeopleArg(int& argc, char** argv);
void historyChecks(const std::function<void(const char*, bool, const std::string&)>& check);
void agesChecks(const std::function<void(const char*, bool, const std::string&)>& check);
int runHistory(int argc, char** argv);
int runPeoples(int argc, char** argv);

// ---- civ_lore.cpp
bool checkChartsByAge(std::string& detail);
std::vector<Star> drifterPeoplesNearHome(double reachLy = 3000);
int shardsOfWorld(const Star& s, StarSystem& sys, int bi, int which, int n, bool full, int held, int64_t sx, int64_t sz);
int runCharts(int argc, char** argv);
int runRuins(int argc, char** argv);
int runGraves(int argc, char** argv);
int runShards(int argc, char** argv);

// ---- civ_music.cpp
int runMusic(int argc, char** argv);

// ---- civ_records.cpp
void recordsChecks(const std::function<void(const char*, bool, const std::string&)>& check);
void mediaChecks(const std::function<void(const char*, bool, const std::string&)>& check);
void historyPageChecks(const std::function<void(const char*, bool, const std::string&)>& check);
void recordPlacesOf(const BodyGen& g, int people, std::vector<RecordPlace>& out, int countryside = 9);
std::string fileWord(std::string s);
int runDecoder(int argc, char** argv);
int runRecords(int argc, char** argv);

// ---- civ_instruments.cpp
void instrumentChecks(const std::function<void(const char*, bool, const std::string&)>& check);
void playerChecks(const std::function<void(const char*, bool, const std::string&)>& check);
void makerChecks(const std::function<void(const char*, bool, const std::string&)>& check);
int runInstruments(int argc, char** argv);

// ---- civ_grammar.cpp
void grammarChecks(const std::function<void(const char*, bool, const std::string&)>& check);
double nameClassify(const std::vector<std::vector<std::string>>& names);                           // H-04's second pass
int runTongues(int argc, char** argv);
int runGrammar(int argc, char** argv);

// ---- unit.cpp
int testUnit(int argc, char** argv);

// ---- unit_basics.cpp
void unitNoise(const Check& check);
void unitBasics(const Check& check);

// ---- unit_kinds.cpp
void unitKinds(const Check& check);
void unitDead(const Check& check);
void unitCold(const Check& check);
void unitMoons(const Check& check);
void unitGiants(const Check& check);
void unitTraits(const Check& check);

// ---- unit_drifters.cpp
void unitDrifters(const Check& check);

// ---- unit_signals.cpp
void unitSignals(const Check& check);

// ---- unit_navigation.cpp
void unitNavigation(const Check& check);
void unitSights(const Check& check);
void unitAuto(const Check& check);
void unitStates(const Check& check);

// ---- unit_ship.cpp
void unitShip(const Check& check);

// ---- unit_search.cpp
void unitSearch(const Check& check);

// ---- unit_civ.cpp
void unitCivilisations(const Check& check);
void unitRuins(const Check& check);
void unitCharts(const Check& check);
void unitRoads(const Check& check);
void unitGraves(const Check& check);

// ---- unit_peoples.cpp
void unitCultures(const Check& check);
void unitProfiles(const Check& check);
void unitStory(const Check& check);

// ---- unit_sky.cpp
void unitStars(const Check& check);
void unitSky(const Check& check);

// ---- unit_probe.cpp
void unitProbe(const Check& check);

// ---- unit_proof.cpp
void unitPerf(const Check& check);

// E-12: a star found in its sector (`named`: starInSector's) with its system made; false, the system left as it was, when there is none
inline bool systemAt(int64_t sx, int64_t sy, int64_t sz, Star& st, StarSystem& sys, bool named = false) {
    if (!starInSector(sx, sy, sz, st, named)) return false;
    sys.generate(st);
    return true;
}
// E-12: the sectors x0 <= x < x1 by z0 <= z < z1 of the plane y = 0, x outer, each star with its system made, as the scans' two loops
// `for x (while go) { for z (while go) { Star s; if (!starInSector(x, 0, z, s)) continue; StarSystem sys; sys.generate(s); ... } }`
// went, `go` (none: always) asked where their heads asked it: `for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160))`. A
// `continue` goes on to the next star; a `break` would end the whole scan, where it ended the z loop alone
struct ScanHit { int64_t x, z; Star s; StarSystem sys; };
class scanSystems {
public:
    scanSystems(int64_t x0, int64_t x1, int64_t z0, int64_t z1, std::function<bool()> go = nullptr)
        : x0(x0), x1(x1), z0(z0), z1(z1), go(std::move(go)) {}
    struct End {};
    struct Iter {
        scanSystems* r;
        ScanHit& operator*() const { return *r->hit; }
        Iter& operator++() { r->next(); return *this; }
        bool operator!=(End) const { return !r->done; }
    };
    Iter begin() { next(); return Iter{this}; }
    End end() const { return End{}; }
private:
    int64_t x0, x1, z0, z1, x = 0, z = 0;
    bool started = false, done = false;
    std::function<bool()> go;
    std::optional<ScanHit> hit;
    bool goes() { return !go || go(); }
    void next() {
        hit.reset();   // the last star's system goes before the next is looked for, as the loops' locals did
        if (!started) { started = true; x = x0; if (!(x < x1 && goes())) { done = true; return; } z = z0; }
        else z++;
        for (;;) {
            if (z < z1 && goes()) {
                Star s;
                if (starInSector(x, 0, z, s)) { hit.emplace(); hit->x = x; hit->z = z; hit->s = s; hit->sys.generate(hit->s); return; }
                z++;
                continue;
            }
            x++;
            if (!(x < x1 && goes())) { done = true; return; }
            z = z0;
        }
    }
};

// the first `maxBodies` bodies of a type in the start region, in the scan order every harness mode uses; `fn(sys, bi)` (P-04: a kind made
// only where no system is, on the worlds `stageElsewhere` stages it on; P-05: after the real ones, the wanderers)
template <class F>
static int forTypeBodies(int type, int maxBodies, bool preferMoon, F fn) {
    int found = 0;
    if (type == PT_DARK_OCEAN) {   // the drifters' own kind: the drifters nearest home (no system holds one)
        Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, false);
        std::vector<Star> near; driftersNear(home.pos, 4000, near);
        for (const Star& d0 : near) {
            if (found >= maxBodies) break;
            Star d; drifterAt(d0.sx, d0.sy, d0.sz, d, true);
            StarSystem sys; sys.generate(d);
            for (int bi = 0; bi < (int)sys.bodies.size() && found < maxBodies; bi++) if (sys.bodies[bi].type == type) { found++; fn(sys, bi); }
        }
        return found;
    }
    const int stagePasses = KINDS[type].elsewhere ? 2 : 1;   // the real ones first (the wanderers), then the staged
    for (int stagePass = 0; stagePass < stagePasses && found < maxBodies; stagePass++)
    for (int pass = preferMoon ? 0 : 1; pass < 2 && found < maxBodies; pass++)
        for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160, [&] { return found < maxBodies; })) {
            for (int bi = 0; bi < (int)sys.bodies.size() && found < maxBodies; bi++) {
                const Body& b = sys.bodies[bi];
                if (stagePass == 0 ? b.type != type : !stageElsewhere(sys, bi, type)) continue;
                if (pass == 0 && b.parent < 0) continue;   // the first pass looks for moons only
                found++;
                fn(sys, bi);
            }
        }
    return found;
}

inline Check modeCheck(int& failures) { return Check{nullptr, &failures, nullptr, false}; }   // a `<mode> check` run's
