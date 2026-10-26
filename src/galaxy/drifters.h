#pragma once
// D-01 (2026-10-23): the drifters (R-423; `docs/ideas/IDEAS-worlds-and-wanderers.md` section 3, `docs/ideas/ITERATION-1.4.0.md`): worlds
// with no sun. Microlensing counts about twenty for every star (Sumi et al. 2023), most of them small and all of them dark; the game holds
// the few that call (SIMP 0136 was found by its radio aurorae), about one for every seven million stars. They are a population of their own,
// hashed in cells of DRIFT_CELL sectors with a salt of their own and placed only in a sector that holds no star (`starInSector` fails
// there), so no star or system moves and the star field, the neighbourhood, the aim and `jumpTarget` never see one. A drifter is a place as
// a star is: a `Star` of class STAR_ROGUE (no light, no radius, no mass) at its anchor in its sector, and its system is the drifter on the
// anchor with its moons (`StarSystem::generate`). The field: up to DRIFT_CELL_MAX candidates a cell at hashed sectors, each kept as often as
// the stars are dense there (so the drifters follow the stars: they were thrown out of systems), twice as often among the old populations
// (the core, the bulge, the halo and the globulars: they have had longest to lose their worlds), and only where the sector holds no star;
// and the globular clusters' own besides (DRIFT_GLOBULAR_MEAN a cluster: the oldest crowds, where passing stars pull worlds away). They
// stay where they are: a few km/s is nothing on the scale of the game's light years.
#include "starfield.h"
#include <vector>

constexpr int DRIFT_CELL = 256;                   // the field's cells, sectors a side
constexpr int DRIFT_CELL_MAX = 4;                 // candidates a cell may hold
constexpr double DRIFT_RATE = 0.6;                // the field: DRIFT_CELL_MAX DRIFT_RATE / DRIFT_CELL^3 drifters a star (1.4e-7)
constexpr double DRIFT_OLD = 2.0;                 // the old populations' factor
constexpr double DRIFT_GLOBULAR_MEAN = 1.2;       // a globular cluster's own, on average (at most DRIFT_GLOBULAR_MAX)
constexpr int DRIFT_GLOBULAR_MAX = 4;
constexpr uint64_t DRIFT_SALT = 0xD81F7E5500000001ULL;

// the drifters of a field cell (the cell's corner is cx * DRIFT_CELL ...) and of the globular cluster of a cluster cell (`globularInCell`'s
// cells, relative to the galactic centre), their sectors; each returns how many it wrote
int drifterCell(int64_t cx, int64_t cy, int64_t cz, int64_t* sx, int64_t* sy, int64_t* sz);
int drifterGlobular(int64_t gx, int64_t gy, int64_t gz, int64_t* sx, int64_t* sy, int64_t* sz);
bool drifterInSector(int64_t sx, int64_t sy, int64_t sz, Star& out, bool withName = true);   // the sector's drifter as a place (STAR_ROGUE)
void drifterAt(int64_t sx, int64_t sy, int64_t sz, Star& out, bool withName = true);   // D-02: the place of a sector a cell listed (no test: `drifterInSector` tests)
// the one resolver from a sector to its place: its star, else its drifter. A key that names a place (the save's star and target, a mark, the
// previous and the home, a lock, a record's world) turns into it through this, so a drifter is reached as a star is; the star field, the
// neighbourhood and the jumps stay the stars' (`starInSector`)
bool placeInSector(int64_t sx, int64_t sy, int64_t sz, Star& out, bool withName = true);

// what a drifter is, from its seed alone (`StarSystem::generate` reads it): its kind by the dark's table (section 3: every kind that can be
// what it is with no sun: the giants and the brown dwarfs most, as the loudest; cold rock and ice; a young molten or tectonic world still
// hot inside; a desert; a europan world whose ocean a moon's tides keep; a living world frozen, never a living one), its size in the
// kind's range and its own heat (`Body::heatK`: 20-60 K for cold rock and ice, a giant's own, a brown dwarf's glow, a young interior's)
// D-04: a rocky drifter that kept the hydrogen it was born in is a dark ocean (a share by the kind's salt): made from the draw, so its
// size and heat are drawn by the same uniforms in its own ranges, and what the system makes round it and the radar hears of it (its moons,
// its power) stay the rocky drifter's it was drawn as (`drawnType`, `drawnRadiusKm`)
struct DrifterBody { int type = 0; double radiusKm = 0, heatK = 0; int drawnType = 0; double drawnRadiusKm = 0; };
DrifterBody drifterBody(const Star& d);
constexpr double DARK_OCEAN_SHARE = 0.4;          // of the rocky drifters
constexpr double DARK_OCEAN_HEAT_LO = 280, DARK_OCEAN_HEAT_HI = 350;   // its sea's warmth under the hydrogen (Stevenson's 280-350 K)
// where a ship coming to a drifter stops: 40-80 of the drifter's radii from it, by its seed (a star's rule, its first orbit or 25 of its
// radii, puts the ship at a drifter's centre: it has neither)
double drifterStandOffKm(const Star& d);
// D-02: how loud it calls, the power of its radio aurorae in the radar's reach law (`signals.h`: heard to SIGNAL_ROGUE_REACH_LY
// sqrt((G - 1) p) ly at gain G, so never at x1): a brown dwarf's the loudest (SIMP 0136's), a giant's next, a young world's with its
// dynamo and a metal world's iron core, then a rock's, a desert's, a europan or a frozen living world's, a small icy world's the
// faintest; by its size against its kind's middle and a spread of its own (its spin and its field, 0.5-2 log-uniform)
double drifterPower(const Star& d);
double drifterPowerMax();   // the most any drifter has (the scan's reach)
struct DarkKind { int type; double weight, heatLo, heatHi; };
extern const DarkKind DARK_KINDS[];
extern const int DARK_KIND_COUNT;
// where it came from: the star it left (a real star, back along a hashed way near the disc's plane as far as its speed carried it in
// the years since: a game light year is 1.5e10 km, so a few km/s for a millennium is tens of them), how long ago (1,500-20,000 years: a
// people that lived there ended after it, D-05) and how fast (2-8 km/s, a scattering's kick): 6-340 light years. It searches the stars
// round the way's end, so it is asked for, not kept
// D-05: a drifter that kept a people (`drifterHadPeople`, galaxy/system.h) left a star its people's world could have circled (`livingSun`: a yellow
// star, an orange or a red dwarf) and a system its way of losing it needs (`drifterSunWayOf`): of the living suns nearest the way's end (the
// sixteen nearest), the first whose system holds a giant for the wanderer's way (planet-planet scattering), a second sun for the dance's; the
// nearest else (and where none fits)
struct DrifterPast { Star star; bool found = false; double years = 0, speedKms = 0, travelLy = 0; Vec3 dir; };
DrifterPast drifterPast(const Star& d);
double drifterLeftYears(const Star& d);   // D-05: `drifterPast`'s years alone (no search: the people's end reads it)
double drifterSpeedKms(const Star& d);    // D-05: and its speed (a chart drawn in the dark: where the drifter was by then)
inline bool livingSun(int cls) { return cls == STAR_YELLOW || cls == STAR_ORANGE || cls == STAR_RED_DWARF; }
// D-05: how a drifter's people lost its sun as drawn (`SunWay`, peoples/peoples.h: the wanderer five in ten, the intruder two, the dark one, the
// dance two), from its world's seed; the profile takes it where the system it left allows it (a giant, a companion), else the intruder's
int drifterSunWayOf(uint64_t worldSeed);

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do
// the drifters within reachLy of a point (km), nearest first (without names); returns the field cells and clusters it looked at
int driftersNear(const Vec3& obsKm, double reachLy, std::vector<Star>& out);
