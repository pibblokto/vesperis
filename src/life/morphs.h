#pragma once
// Z-02 (2026-11-16): what it looks like (R-425; docs/archive/ideas/IDEAS-small-and-spacious.md sections 1.5 and 1.6, ITERATION-1.5.0.md
// Z-02). A species' form is about forty values in groups (the outline by Gielis's superformula, the size, the arrangement, the wall,
// the appendages, the inside, the pigment, the motion, the forms of its life cycle) drawn from its seed by its plan, its stage, its
// chemistry and the style its genesis gives its kin (a wall's mineral, a symmetry, a flagellum's kind, its handedness): a clade looks like
// a family, a stranger looks strange. Nothing here is saved: a form is a pure function of its species (and, on a world, of the species
// before it: a world's are kept apart, `morphsOf`). `morphDistance` measures two forms as A-04's genome distance does (colours by
// distance, measures by their spread, categories 0 or 1); under MORPH_ALIKE two look alike. The drawing is life/morph_draw.h
#include "life/biota.h"
#include <string>
#include <vector>

// the six ways of seeing (Murphy and Davidson 2012): a bright field with the pigments in their colours; phase contrast (grey, the cells dark
// in a bright halo); dark field (black, the edges and the flagella bright); fluorescence (the DNA stain's blue, the plastids' red, a luminous
// species' own light); the scanning electron's relief of one cell; a polarised thin section (the minerals' interference colours, the
// fossils' carbon)
enum Seeing { WS_BRIGHT = 0, WS_PHASE, WS_DARK, WS_GLOW, WS_ELECTRON, WS_SECTION, WS_COUNT };
extern const char* const SEEING_NAMES[WS_COUNT];

// the forms of its life cycle: the cell; a spore (a thick wall, smooth, spined, ridged or netted); a cyst (a cell rounded in a double wall);
// a cell dividing; a colony of it; the gathered amoebae's slug and their stalk with its head of spores; the ghost after lysis; a fossil
enum CycleForm { CF_CELL = 0, CF_SPORE, CF_CYST, CF_DIVIDING, CF_COLONY, CF_SLUG, CF_STALK, CF_GHOST, CF_FOSSIL, CF_COUNT };
extern const char* const CYCLE_FORM_NAMES[CF_COUNT];

// how its cells keep together: alone, in pairs, chains, clusters, fours, packets of eight, palisades side by side, rosettes by their holdfasts
enum Arrangement { AR_SINGLE = 0, AR_PAIRS, AR_CHAINS, AR_CLUSTERS, AR_FOURS, AR_PACKETS, AR_PALISADES, AR_ROSETTES, AR_COUNT };
extern const char* const ARRANGEMENT_NAMES[AR_COUNT];
// the wall's ornament: smooth; pores in rows; ribs; spines; a lattice of holes; plates; a net of ridges; the world's grains glued on
enum WallMark { MK_SMOOTH = 0, MK_PORES, MK_RIBS, MK_SPINES, MK_LATTICE, MK_PLATES, MK_NET, MK_GRAINS, MK_COUNT };
extern const char* const MARK_NAMES[MK_COUNT];
// where its flagella grow: none; one pole; both; a tuft at a pole; all round; from its side; one in a girdle and one trailing
enum FlagellaPlace { FV_NONE = 0, FV_POLAR, FV_BIPOLAR, FV_TUFT, FV_AROUND, FV_LATERAL, FV_GIRDLE, FV_COUNT };
extern const char* const FLAGELLA_PLACE_NAMES[FV_COUNT];
// a flagellum's kind (its genesis's): a smooth whip, a tinsel of hairs, a ribbon that undulates
enum FlagellumKind { FS_WHIP = 0, FS_TINSEL, FS_RIBBON, FS_COUNT };
extern const char* const FLAGELLUM_KIND_NAMES[FS_COUNT];
enum PlastidShape { PX_NONE = 0, PX_DISCS, PX_CUP, PX_BAND, PX_STAR, PX_NET, PX_COUNT };
extern const char* const PLASTID_NAMES[PX_COUNT];
enum Pods { PD_NONE = 0, PD_LOBES, PD_THREADS, PD_NET, PD_COUNT };
extern const char* const POD_NAMES[PD_COUNT];
enum CoatWall { CW_SMOOTH = 0, CW_SPINED, CW_RIDGED, CW_NETTED, CW_COUNT };
extern const char* const COAT_NAMES[CW_COUNT];
// what a wall is made of (its genesis's, its world's chemistry's): organic, silica, calcite, celestine, iron, sulphur, a nitrile membrane
// (the methane's azotosomes: Stevenson et al. 2015)
enum Mineral { MI_ORGANIC = 0, MI_SILICA, MI_CALCITE, MI_CELESTINE, MI_IRON, MI_SULPHUR, MI_NITRILE, MI_COUNT };
extern const char* const MINERAL_NAMES[MI_COUNT];

struct Morph {
    uint64_t seed = 0;          // its species' (an individual's draws come from it and its own seed)
    int plan = MP_SPHERE, state = SS_LIVING, chemistry = CH_WATER;
    int way = LW_NONE;          // how it lives (a light-fed one's thylakoids glow red)
    int variant = 0;            // a plan's sub-form (a diatom centric or pennate, a plate colony's kind, a chain's)
    // the outline (Gielis 2003): r(phi) = (|cos(m phi / 4)|^n2 + |sin(m phi / 4)|^n3)^(-1 / n1)
    int sym = 0;
    double n1 = 2, n2 = 2, n3 = 2;
    double aspect = 1;          // its length over its width
    double bend = 0;            // a rod's curvature: the radians its axis turns over its length
    double amp = 0, turns = 0;  // a helix's amplitude (over the width) and turns over its length
    double wander = 0;          // how far its outline wanders, 0..1
    double taper = 0;           // its ends: 0 round, 1 pointed, under 0 square
    // the size (Young 2006)
    double lengthUm = 2, widthUm = 1;
    // the arrangement
    int arrangement = AR_SINGLE, groupN = 1;
    // the wall
    double wall = 0.06;         // its thickness over the width
    int mark = MK_SMOOTH, marks = 0;   // its ornament and how many (rows, ribs, plates)
    int spines = 0, spineBranch = 0;
    double spineLen = 0;        // over the width
    double capsule = 0;         // a halo round it, over the width
    double sheath = 0;          // a filament's sheath, over the width
    // the appendages
    int flagella = 0, flagPlace = FV_NONE, flagKind = FS_WHIP;
    double flagLen = 0;         // over the length
    double beatHz = 0, waveLen = 0.5;   // a flagellum's beat and its wave's length over its own
    double cilia = 0;           // the share of its edge with cilia
    int ciliaRows = 0;
    double ciliaLen = 0, ciliaWave = 0;   // over the width; the metachronal wave's length in cilia
    int pili = 0;
    double stalk = 0;           // over the length
    bool holdfast = false;
    int pods = PD_NONE, podCount = 0;
    // the inside
    double nucleus = 0;         // its radius over the width (0: a nucleoid)
    int vacuoles = 0;
    double vacPulse = 0;        // a contractile vacuole's period (s), 0 none
    double granules = 0;
    int plastids = 0, plastidShape = PX_NONE;
    bool eyespot = false;
    double gas = 0;
    int magnetosomes = 0, droplets = 0;
    // the pigment
    int pigment = PG_NONE;
    double density = 0.3;
    bool glows = false;
    // the motion
    int motion = MO_NONE;
    double speed = 0;           // um/s
    double runS = 1;            // a run's time
    uint32_t taxis = 0;
    // the forms of its life cycle
    int coat = CW_SMOOTH;
    double sporeSize = 0.6;     // over the width
    int sporeAt = 0;            // 0 central, 1 under a pole, 2 at a pole (the drumstick)
    uint32_t cycle = 0;
    int division = DV_NONE;
    // the small cells' marks the eye tells them by: refractile granules at the poles, the nucleoid's clear zone, a wavy axis; a protocell's
    // clay grains
    int polar = 0;              // 0 none, 1 one pole, 2 both
    double nucleoid = 0;        // its clear zone in phase contrast, 0..1
    double wave = 0, waves = 0; // its axis's waviness (over the width) and its waves along it
    int clay = 0;
    // the style its genesis gives its kin
    int mineral = MI_ORGANIC, styleSym = 0, cortex = 0;
    bool mirrored = false;      // its helices and spirals turn the other way
    uint32_t style = 0;
    // the radius that holds its body (its cells, their arrangement, a colony's whole), and with every appendage (um)
    double bodyUm() const;
    double extentUm() const;
    // the radius a first look takes in: its body, but a filament's few cells, a spirochete's few waves, a mycelium's few branches
    double viewUm() const;
};

// a species' form: from its plan, its state, its world's chemistry, its genesis's style and its seed; `nudge` over 0 draws it again on a
// stream of its own (a world's are kept apart)
Morph morphOf(const Microbe& s, int chemistry, const Genesis& g, int nudge = 0);
// a world's species' forms, in its list's order: a carried spore is its source's (drawn on its world), the world's own drawn again while one
// is alike an earlier (MORPH_ALIKE)
std::vector<Morph> morphsOf(const StarSystem& sys, int body);
// a pigment's colour at its middle in a bright field (the slide's palette and the distance read it)
RGB pigmentColour(int pigment);
// the ship's own (shipSpecies)
Morph shipMorph(int k);
// how far apart two forms are, 0 the same .. morphMaxDistance(); `shared` lists the values they share
double morphDistance(const Morph& a, const Morph& b, std::string* shared = nullptr);
double morphMaxDistance();
constexpr double MORPH_ALIKE = 2.5;   // two forms nearer than this look alike: their pictures do thirteen times as often as two at random (`life distinct`)
