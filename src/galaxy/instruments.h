// C-11 (2026-10-10): the instruments the peoples left (`docs/ideas/ITERATION-1.3.0.md`, C-11). A people played its music on the instruments
// of each age it lived through (H-05's `Ensemble`, `Tradition::ages`): what a hand could carry of them lies in its ruins, beside the music
// records of their age (a third of the music records, by the record: every copy of that record has it beside it, and taking it once takes
// every copy, as with the records), and is played from the cabin: its people's own timbre (the ensemble's voice it is) on its people's own
// scale (the tradition's degrees and its period), a key a degree.
//
// C-17 (2026-10-10): the instruments' maker. C-11's were 49 fixed kinds chosen by the age (every prehistoric people's bone flute, every
// industrial people's gramophone); now each is built from parts by its people's own way of making:
//
// * what sounds in it is its voice's (the timbre's family: an edge-blown pipe, a reed, lips, a plucked, a bowed string, struck metal or
//   stone, a skin, a whirled blade, a machine: `InstrumentClass`); the plan it is built on is drawn from the people's leanings among its
//   class's (a straight pipe, a vessel, a raft, a ring of pipes; a necked body, a frame, a harp, a board, a ring of strings; bars, bowls,
//   bells, a gong, ringing stones ...: `InstrumentPlan`), each people loving a few above the rest and keeping them through its ages;
// * its materials are what its world gave it in its age (`InstrumentMaterial`): wood, stalks, gourds and pith where plants grew of those
//   families, bone, horn, hide and gut, chitin, shell and coral from its animals and its seas, ice in the cold, obsidian by the volcanoes,
//   stone and clay; the metals from the medieval age, brass and steel from the industrial, the made materials after; the people loving a
//   few of them above the rest;
// * its style is the people's architecture genome's (galaxy/genome.h): its bodies' outline from the plan, the profile and the roof (a
//   rectangle, a circle, a polygon of the people's number, a waist, a taper, a dome), its holes and sound holes the shape of its windows,
//   its heads its buildings' tops, its ornament and motif, its symmetry (a radial people's pipes in a ring, its strings a wheel), the
//   rhythm of its holes and keys, its proportions, its paint, its light in the late ages;
// * its numbers are its people's scale's: a pipe's holes, a raft's tubes and a harp's strings by the degrees, the frets by the tuning;
// * one list of parts (`InstrumentPart`: tubes, outlined bodies, discs, balls, boxes, rings, strings, marks) is both the object lying in
//   the ruins (`instrumentForms`: A-02's forms, an outlined body a `FK_PLATE`) and the picture in the cabin's case (game/instrument_draw.cpp,
//   seen from above as it lies); its name is what it is made of and how ("A SHELL PIPE", "A STRUNG GOURD", "BRONZE BOWLS"), the timbre's
//   name the decoder gives the piece's instruments the same (`Timbre::name`).
//
// The industrial age's machine that played the wax cylinders (C-11's gramophone) is made only where the people made one, in a form of its
// own. The sound is the voice's as it was (the music is unchanged); what the make adds is the thing that made it.
#pragma once
#include "music.h"
#include "forms.h"
#include <memory>
#include <string>
#include <vector>

struct BodyGen;
struct Lore;

// what sounds in it: the classes its plans are drawn from
enum InstrumentClass {
    IC_EDGE = 0, IC_REED, IC_FREE_REED, IC_LIP, IC_PLUCK, IC_MOUTH_PLUCK, IC_BOW, IC_STRUCK, IC_WHIRL, IC_HUM, IC_COMB,
    IC_ESTRING, IC_EKEYS, IC_GLASS, IC_SYNTH, IC_SWARM, IC_FIELD,
    IC_SKIN, IC_LOG, IC_RATTLE, IC_JINGLE, IC_CYMBAL, IC_PADS, IC_MACHINE,
    IC_COUNT
};
extern const char* const INSTRUMENT_CLASS_NAMES[IC_COUNT];   // "an edge-blown pipe", ... (the harness)

// the plans it is built on
enum InstrumentPlan {
    IP_PIPE = 0, IP_CROSSPIPE, IP_VESSEL, IP_RAFT, IP_PIPE_RING, IP_TWIN_PIPE,                                        // edge-blown
    IP_REED_PIPE, IP_TWIN_REED, IP_BAG, IP_CHAMBER,                                                                   // reeds
    IP_BELLOWS, IP_BELLOWS_ROUND,                                                                                    // free reeds
    IP_HORN_COIL, IP_HORN_FOLDED, IP_HORN_STRAIGHT, IP_HORN_SPIRAL, IP_HORN_FORKED, IP_HORN_UPRIGHT,                   // lips
    IP_NECKED, IP_FRAME, IP_HARP, IP_BOARD, IP_TUBE_ZITHER, IP_RING_HARP, IP_TONGUES,                                  // plucked
    IP_BOW, IP_JAW, IP_BOW_HARP,                                                                                     // the bow's sound (a bow, a tongue, bows on a box)
    IP_FIDDLE, IP_SPIKE, IP_TUBE_FIDDLE, IP_BOWED_BOARD, IP_WHEEL, IP_HORN_FIDDLE,                                     // bowed
    IP_BARS, IP_TUBES, IP_BELLS, IP_BELL_TREE, IP_BOWLS, IP_GONGS, IP_STONES, IP_SPOKES,                               // struck
    IP_BLADE, IP_BUZZER, IP_WHIRL_TUBE,                                                                              // whirled
    IP_HUM_ORB, IP_HUM_RING,                                                                                         // the far future's hum
    IP_TUNE_DRUM, IP_TUNE_DISC,                                                                                      // a comb plucked by pins
    IP_SOLID, IP_SKELETON, IP_LAP,                                                                                   // electric strings
    IP_KEY_SLAB, IP_KEY_ARC, IP_KEY_RING,                                                                            // electric keys
    IP_GLASS_RODS, IP_GLASS_BARS, IP_GLASS_BOWLS,                                                                    // struck glass
    IP_PANEL, IP_RIBBON, IP_PAD_GRID, IP_ORB, IP_TONE_STRIPS,                                                        // synthesisers
    IP_ROD_FIELD, IP_LATTICE, IP_CLOUD,                                                                              // swarms
    IP_RINGS, IP_MONOLITH, IP_STAR, IP_FIELD_ORB, IP_SHELLS,                                                         // fields
    IP_FRAME_DRUM, IP_BARREL, IP_GOBLET, IP_POT_DRUM, IP_HOURGLASS, IP_KETTLE, IP_CARAPACE, IP_DRUM_PAIR,             // skins
    IP_SLIT_LOG, IP_TONGUE_BOX, IP_SLIT_CANES,                                                                       // logs
    IP_VESSEL_RATTLE, IP_STRUNG_RATTLE, IP_STICK_RATTLE, IP_RAIN_TUBE,                                               // rattles
    IP_SISTRUM, IP_JINGLE_HOOP, IP_JINGLE_STICK,                                                                     // jingles
    IP_CYMBALS, IP_FINGER_CYMBALS, IP_HAND_CYMBAL,
    IP_PAD_BOX, IP_PAD_RING,                                                                                         // drum machines
    IP_HORN_MACHINE, IP_CABINET, IP_EAR_TUBES,                                                                       // the cylinders' machine
    IP_COUNT
};
struct InstrumentPlanInfo { int cls; const char* name; int ageMin, ageMax; double weight; unsigned flags; };
extern const InstrumentPlanInfo INSTRUMENT_PLANS[IP_COUNT];

// what it is made of
enum InstrumentMaterial {
    IM_BONE = 0, IM_HORN, IM_SHELL, IM_CHITIN, IM_HIDE, IM_GUT, IM_WOOD, IM_DARKWOOD, IM_GOURD, IM_CANE, IM_PITH,
    IM_STONE, IM_CLAY, IM_ICE, IM_OBSIDIAN, IM_CORAL,
    IM_BRONZE, IM_BRASS, IM_COPPER, IM_IRON, IM_STEEL, IM_SILVER,
    IM_GLASS, IM_CRYSTAL, IM_LACQUER, IM_PAINT, IM_PLASTIC, IM_ALLOY, IM_COMPOSITE, IM_CERAMIC, IM_LIGHT, IM_VOID,
    IM_FIBRE, IM_WIRE, IM_FELT, IM_HOLE,
    IM_COUNT
};
extern const char* const INSTRUMENT_MATERIAL_NAMES[IM_COUNT];   // "BONE", "HORN", ... ("" for a hole)
// its colour on the case's picture (its people's own where it takes their banks in the ruins: stone their structure's, glass their glass,
// paint and lacquer their accent, light their light) and its part in the ruins (`Form::part`, `Form::slot`)
RGB instrumentMaterialColour(int mat, const FormPalette& pal);
void instrumentMaterialPart(int mat, int& part, int& slot);
bool instrumentMaterialMetal(int mat);

// a part of an instrument in its own metres as it lies: u along it, v across it, w up from the floor
enum InstrumentPartKind {
    IPK_TUBE = 0,   // a tube lying from (u, v) along `ang` for `len`, its radius r0 at its start and r1 at its end (a cone, a bell), its axis at w0
    IPK_PLATE,      // an outlined body round (u, v), its length `len` along `ang` and its half width r0, from w0 to w1 (`FK_PLATE`'s outline)
    IPK_DISC,       // a round body standing on its foot at (u, v): radii r0 across `ang` and r1 along it, from w0 to w1; `open` a bowl
    IPK_BALL,       // a rounded body (a gourd's back, a bead, a knob): a dome of radius r0 (r1 along) on w0, its top at w1
    IPK_BOX,        // a box round (u, v), `len` along `ang`, half width r0, from w0 to w1
    IPK_RING,       // a ring of radius r0, its tube r1: lying flat round (u, v) at w0, or standing round a tube along `ang` (`standing`) its axis at w0
    IPK_STRING,     // a string from (u, v) at w0 along `ang` for `len` to w1, half thick r0
    IPK_MARK,       // a mark on a surface at w0 (a hole, a sound hole, a fret, an inlay): `shape` (0 round, 1 slit, 2 square, 3 a triangle,
                   // 4 a keyhole, 5 an arch, 6 a star), r0 its radius (a slit's half width), `len` a slit's length along `ang`
    IPK_COUNT
};
struct InstrumentPart {
    int kind = IPK_BOX, mat = IM_WOOD;
    int course = -1;            // the key that sounds it (a hole, a string, a bar, a key, a pad), -1 none: it glows as it is played
    double u = 0, v = 0, ang = 0, len = 0.1, r0 = 0.01, r1 = 0.01, w0 = 0, w1 = 0.02;
    double taper = 1, waist = 0, square = 2, dome = 0; int sides = 0, skew = 0;   // a plate's outline; `sides` a disc's polygon too
    int shape = 0;
    bool open = false, standing = false;
};

// the makes of a people's instruments (`Tradition::makes`): each voice and kit piece of each age that is a thing a hand made (not its
// singers, a church's organ, a piano, a drum kit) is given one and its timbre its name; the industrial age's machine where the people made
// one (`Tradition::machine`). `traditionOf` calls it last, on streams of its own
void traditionMakes(Tradition& T, const BodyGen& g, const Lore& L);
// the class a timbre's instrument is of (-1: no thing a hand made); `bass` its role's
int instrumentClassOf(const Timbre& tb, int role, int age, bool kit);
// the harness's: a make of a given plan in a people's way (its world's materials, its genome's style) at an age; whether a people's world
// and age gave a material (what the unit holds every make to)
InstrumentMake instrumentMakeOfPlan(const BodyGen& g, const Lore& L, int plan, int age, uint64_t salt);
bool instrumentMaterialGiven(const BodyGen& g, const Lore& L, int mat, int age);

struct Instrument {
    int people = 0;      // which of the world's peoples
    int age = 0;         // the age it was played in (its ensemble's)
    int voice = -1;      // the ensemble's voice it is (`Ensemble::voices`); -1 the machine, which plays a cylinder, or a kit's piece
    int kit = -1;        // the ensemble's kit piece it is (`Ensemble::kit`: a drum, a log drum, a rattle); with `pads` the whole kit, a key a piece
    bool pads = false;   // a drum machine: each key strikes a piece of the kit at its own tone
    bool disc = false;   // C-17: the machine that played the wax cylinders (C-11's gramophone): Enter winds it
    int slot = 0;        // its place in the age's list
    int id = 0;          // the guide's: people x 1000 + age x 100 + slot (`<body>/I<id>`)
    int make = -1;       // C-17: its make (`Tradition::makes`)
    int reg = 0;         // its register: the period its first key plays in (-1 a bass, +1 a flute's)
    uint64_t seed = 0;   // the make's
};
struct PeopleInstruments { std::vector<Instrument> byAge[AGE_COUNT]; };

// a people's instruments of each age from its tradition's makes (`traditionOf`, the decoder's: the same voices its pieces are played on)
void instrumentsOf(const Tradition& T, int people, PeopleInstruments& out);
const Instrument* instrumentById(const PeopleInstruments& P, int id);
const InstrumentMake* instrumentMake(const Tradition& T, const Instrument& I);
std::string instrumentName(const Tradition& T, const Instrument& I);   // "A SHELL PIPE" (the HUD, the case, the log)
// the timbre it sounds with (the machine: none, it plays its cylinder); a drum machine's: the kit piece a key strikes (`key` 0..)
const Timbre* instrumentTimbre(const Tradition& T, const Instrument& I, int key = 0);
inline bool instrumentPlaysDisc(const Instrument& I) { return I.disc; }   // the machine: Enter winds it, its cylinder plays
// a degree's pitch on it (its register's period, the scale's degrees from the tonic; 0 the register's tonic, past the scale the next period);
// an unpitched piece (a rattle, jingles) is brighter for a higher key
double instrumentHz(const Tradition& T, const Instrument& I, int degree, int regShift = 0);
// the machine's cylinder: a piece of its people's of the age it was made in (a form of the age's, its own seed)
void instrumentDiscOf(const Tradition& T, const Instrument& I, Piece& out);
// a music record (a people's record `local`) has an instrument lying beside it (a third of them; every copy of the record alike), and the pick
// among its age's instruments (`instrumentIdOf`)
constexpr double INSTRUMENT_SHARE = 0.34;
bool instrumentWithRecord(uint64_t worldSeed, int people, int local, double& pick);
int instrumentIdOf(const PeopleInstruments& P, int age, double pick);   // -1 none
// C-17: its parts as it lies, in its own metres round its middle (the constructor: galaxy/instrument_plans.cpp)
void instrumentParts(const Tradition& T, const Instrument& I, std::vector<InstrumentPart>& out);
void instrumentPartsOf(const Tradition& T, const InstrumentMake& M, std::vector<InstrumentPart>& out);
// its object lying in the ruins: its parts as forms, centred at (x, z) of the settlement's metres along `heading`
void instrumentForms(const Instrument& I, const Tradition& T, double x, double z, double heading, std::vector<Form>& out);
void instrumentPartsToForms(const std::vector<InstrumentPart>& parts, uint64_t seed, double x, double z, double heading, std::vector<Form>& out);
// a plate's form in the instrument's own frame (its outline: `formPlateOutline`; the picture draws the same)
Form instrumentPlateForm(const InstrumentPart& p);
// the courses it shows that a key sounds (its holes, strings, bars, keys, pads), at least one
int instrumentCourses(const Tradition& T, const Instrument& I);
int instrumentCoursesOf(const std::vector<InstrumentPart>& parts);
// its extent as it lies: its box in its own metres, and its length along and across, its top
void instrumentBounds(const std::vector<InstrumentPart>& parts, double& u0, double& u1, double& v0, double& v1, double& top);
void instrumentExtent(const std::vector<InstrumentPart>& parts, double& lenU, double& lenV, double& top);
