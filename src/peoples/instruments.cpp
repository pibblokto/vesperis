// C-11: the instruments the peoples left; C-17: their makers (see instruments.h). The tables, each people's way of making (`Organology`),
// the makes and their names, the lists by age, and the parts as forms. The constructor of the parts is peoples/instrument_plans.cpp.
#include "instruments.h"
#include "shards.h"
#include "genome.h"
#include "galaxy/planetmap.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <cstring>

const char* const INSTRUMENT_CLASS_NAMES[IC_COUNT] = {
    "an edge-blown pipe", "a reed", "free reeds", "lips", "a plucked string", "a string plucked in the mouth", "a bowed string", "struck metal or stone",
    "a whirled blade", "a hum", "a comb plucked by pins", "an electric string", "electric keys", "struck glass", "a synthesiser", "a swarm", "a field",
    "a skin", "a log's tongues", "a rattle", "jingles", "cymbals", "a drum machine", "the cylinders' machine"};

// the plans: the class, the harness's name, the ages they were made in, the base weight, the people's leanings they answer to
enum { PL_RADIAL = 1, PL_ROUND = 2, PL_OPEN = 4, PL_LONG = 8, PL_CANE = 16, PL_CARAPACE = 32, PL_NOBASS = 64, PL_BASSONLY = 128 };
const InstrumentPlanInfo INSTRUMENT_PLANS[IP_COUNT] = {
    {IC_EDGE, "a straight pipe", 0, 5, 1.2, PL_LONG}, {IC_EDGE, "a side-blown pipe", 0, 5, 0.8, PL_LONG}, {IC_EDGE, "a vessel", 0, 5, 1.0, PL_ROUND},
    {IC_EDGE, "a raft of pipes", 0, 5, 0.9, 0}, {IC_EDGE, "a ring of pipes", 0, 5, 0.35, PL_RADIAL}, {IC_EDGE, "twin pipes", 0, 5, 0.6, 0},
    {IC_REED, "a reed pipe and its bell", 0, 5, 1.2, PL_LONG}, {IC_REED, "a double reed pipe", 0, 5, 0.7, 0}, {IC_REED, "a bag and its pipes", 1, 5, 0.6, PL_ROUND},
    {IC_REED, "a wind chamber and its pipes", 0, 5, 0.6, PL_ROUND | PL_RADIAL},
    {IC_FREE_REED, "a bellows box", 1, 5, 1.0, 0}, {IC_FREE_REED, "a round bellows", 1, 5, 0.7, PL_ROUND | PL_RADIAL},
    {IC_LIP, "a coiled horn", 1, 5, 1.0, PL_ROUND}, {IC_LIP, "a folded horn", 1, 5, 0.9, 0}, {IC_LIP, "a long horn", 0, 5, 0.6, PL_LONG | PL_NOBASS},
    {IC_LIP, "a spiral horn", 0, 5, 0.6, PL_ROUND | PL_RADIAL}, {IC_LIP, "a forked horn", 1, 5, 0.35, PL_RADIAL}, {IC_LIP, "a great upright horn", 1, 5, 1.0, PL_BASSONLY},
    {IC_PLUCK, "a necked body", 0, 5, 1.3, PL_LONG}, {IC_PLUCK, "a frame (a lyre)", 0, 5, 0.9, PL_OPEN}, {IC_PLUCK, "a harp", 0, 5, 0.8, PL_OPEN},
    {IC_PLUCK, "a board of strings", 0, 5, 0.8, 0}, {IC_PLUCK, "a strung tube", 0, 5, 0.45, PL_CANE | PL_LONG}, {IC_PLUCK, "a ring of strings", 0, 5, 0.35, PL_RADIAL | PL_ROUND},
    {IC_PLUCK, "plucked tongues", 0, 5, 0.6, PL_ROUND},
    {IC_MOUTH_PLUCK, "a strung bow", 0, 5, 1.0, PL_LONG}, {IC_MOUTH_PLUCK, "a tongue in a frame", 0, 5, 0.8, 0}, {IC_MOUTH_PLUCK, "bows on a box", 0, 5, 0.6, PL_RADIAL},
    {IC_BOW, "a bowed necked body", 0, 5, 1.3, 0}, {IC_BOW, "a spiked body", 0, 5, 0.8, PL_ROUND | PL_LONG}, {IC_BOW, "a bowed tube", 0, 5, 0.4, PL_LONG | PL_NOBASS},
    {IC_BOW, "a bowed board", 0, 5, 0.4, 0}, {IC_BOW, "a wheel and its keys", 1, 5, 0.45, PL_NOBASS}, {IC_BOW, "a bowed horn", 2, 5, 0.35, PL_NOBASS},
    {IC_STRUCK, "bars on rails", 0, 5, 1.0, 0}, {IC_STRUCK, "hung tubes in a frame", 0, 5, 0.8, PL_OPEN | PL_LONG}, {IC_STRUCK, "a set of bells", 0, 5, 1.0, PL_ROUND},
    {IC_STRUCK, "a tree of bells", 1, 5, 0.4, PL_LONG}, {IC_STRUCK, "a row of bowls", 0, 5, 0.9, PL_ROUND}, {IC_STRUCK, "gongs", 1, 5, 0.8, PL_ROUND},
    {IC_STRUCK, "ringing slabs", 0, 5, 0.8, 0}, {IC_STRUCK, "a wheel of bars", 0, 5, 0.4, PL_RADIAL | PL_ROUND},
    {IC_WHIRL, "a blade on a cord", 0, 5, 1.4, 0}, {IC_WHIRL, "a disc on a cord", 0, 5, 0.6, PL_ROUND}, {IC_WHIRL, "a ridged tube", 0, 5, 0.5, PL_LONG},
    {IC_HUM, "an orb in its cradle", 0, 5, 1.0, PL_ROUND}, {IC_HUM, "a grooved ring", 0, 5, 1.0, PL_RADIAL},
    {IC_COMB, "a pinned drum and its comb", 0, 5, 1.2, 0}, {IC_COMB, "a pinned disc and its comb", 0, 5, 0.7, PL_ROUND},
    {IC_ESTRING, "a solid body", 0, 5, 1.4, 0}, {IC_ESTRING, "a bare frame", 0, 5, 0.5, PL_OPEN}, {IC_ESTRING, "a lap slab", 0, 5, 0.4, PL_NOBASS},
    {IC_EKEYS, "a keyed slab", 0, 5, 1.4, 0}, {IC_EKEYS, "keys on an arc", 0, 5, 0.5, PL_ROUND}, {IC_EKEYS, "a ring of keys", 0, 5, 0.35, PL_RADIAL | PL_ROUND},
    {IC_GLASS, "glass rods on a base", 0, 5, 1.0, 0}, {IC_GLASS, "glass bars", 0, 5, 0.8, 0}, {IC_GLASS, "bowls on an axle", 0, 5, 0.5, PL_ROUND},
    {IC_SYNTH, "a panel of keys and knobs", 0, 5, 1.3, 0}, {IC_SYNTH, "a ribbon", 0, 5, 0.6, PL_LONG}, {IC_SYNTH, "a grid of pads", 0, 5, 0.7, 0},
    {IC_SYNTH, "an orb of keys", 0, 5, 0.35, PL_ROUND | PL_RADIAL}, {IC_SYNTH, "strips of light", 4, 5, 0.6, 0},
    {IC_SWARM, "a field of rods", 0, 5, 1.0, PL_RADIAL}, {IC_SWARM, "a lattice", 0, 5, 0.8, 0}, {IC_SWARM, "a cloud of beads", 0, 5, 0.8, PL_ROUND},
    {IC_FIELD, "rings", 0, 5, 1.0, PL_ROUND | PL_RADIAL}, {IC_FIELD, "a monolith", 0, 5, 1.0, PL_LONG}, {IC_FIELD, "a star of rods", 0, 5, 0.8, PL_RADIAL},
    {IC_FIELD, "an orb and its moons", 0, 5, 0.7, PL_ROUND}, {IC_FIELD, "nested shells", 0, 5, 0.6, PL_ROUND},
    {IC_SKIN, "a frame drum", 0, 5, 1.0, PL_ROUND}, {IC_SKIN, "a barrel drum", 0, 5, 1.0, 0}, {IC_SKIN, "a goblet drum", 0, 5, 0.8, 0},
    {IC_SKIN, "a pot drum", 0, 5, 0.9, PL_ROUND}, {IC_SKIN, "an hourglass drum", 0, 5, 0.6, 0}, {IC_SKIN, "a kettle drum", 1, 5, 0.6, PL_ROUND},
    {IC_SKIN, "a carapace drum", 0, 5, 0.7, PL_CARAPACE}, {IC_SKIN, "a pair of drums", 0, 5, 0.6, 0},
    {IC_LOG, "a slit log", 0, 5, 1.2, PL_LONG}, {IC_LOG, "a tongue box", 0, 5, 0.7, 0}, {IC_LOG, "slit canes", 0, 5, 0.8, PL_CANE},
    {IC_RATTLE, "a vessel rattle", 0, 5, 1.2, PL_ROUND}, {IC_RATTLE, "a strung rattle", 0, 5, 0.9, PL_RADIAL}, {IC_RATTLE, "a rattle stick", 0, 5, 0.7, PL_LONG},
    {IC_RATTLE, "a rain tube", 0, 5, 0.5, PL_LONG},
    {IC_JINGLE, "a jingle fork", 0, 5, 1.0, PL_OPEN}, {IC_JINGLE, "a jingle hoop", 0, 5, 0.9, PL_ROUND}, {IC_JINGLE, "a jingle stick", 0, 5, 0.6, PL_LONG},
    {IC_CYMBAL, "a pair of cymbals", 0, 5, 1.0, PL_ROUND}, {IC_CYMBAL, "small cymbals on cords", 0, 5, 0.6, 0}, {IC_CYMBAL, "a cymbal and its striker", 0, 5, 0.6, PL_ROUND},
    {IC_PADS, "a box of pads", 0, 5, 1.3, 0}, {IC_PADS, "a ring of pads", 0, 5, 0.4, PL_RADIAL | PL_ROUND},
    {IC_MACHINE, "a horned machine", 0, 5, 1.3, 0}, {IC_MACHINE, "a cabinet", 0, 5, 0.8, 0}, {IC_MACHINE, "a box of listening tubes", 0, 5, 0.6, 0},
};

// ---- the materials --------------------------------------------------------------------------------------------------------------

const char* const INSTRUMENT_MATERIAL_NAMES[IM_COUNT] = {
    "BONE", "HORN", "SHELL", "CHITIN", "HIDE", "GUT", "WOODEN", "DARKWOOD", "GOURD", "CANE", "PITH",
    "STONE", "CLAY", "ICE", "OBSIDIAN", "CORAL",
    "BRONZE", "BRASS", "COPPER", "IRON", "STEEL", "SILVER",
    "GLASS", "CRYSTAL", "LACQUER", "PAINTED", "PLASTIC", "ALLOY", "COMPOSITE", "CERAMIC", "LIGHT", "VOID",
    "FIBRE", "WIRE", "FELT", ""};

namespace {
// what each material can be (a letter a use; lower case: seldom): T a tube, V a vessel, B a board or a box, F a frame (a neck, arms, rails,
// a stick), S struck (a bar, a bell, a gong, a slab), K a skin, R a string, X a fitting (a peg, a band, a key, a valve), M an ornament, C a
// machine's case, G a tongue (a lamella), L a whirled blade
enum MatUse { MU_TUBE = 0, MU_VESSEL, MU_BOARD, MU_FRAME, MU_STRUCK, MU_SKIN, MU_STRING, MU_FIT, MU_TRIM, MU_CASE, MU_TONGUE, MU_BLADE, MU_COUNT };
const char MU_LETTERS[MU_COUNT + 1] = "TVBFSKRXMCGL";
// needs: what the world must give (`Organology::avail`)
enum MatNeed { MN_NONE = 0, MN_FAUNA, MN_CHITIN, MN_SEA, MN_WOOD, MN_GOURD, MN_CANE, MN_PITH, MN_COLD, MN_VOLCANIC };
struct MatInfo { const char* uses; MatNeed need; double age[AGE_COUNT]; float r, g, b; int part; int slot; bool natural, metal; };
// the colours: the records' banks where the ruins draw it in one (pale, dark, ochre, clay, metal), else its people's (stone their structure,
// glass and crystal their glass, paint and lacquer their accent, light their light: `instrumentMaterialColour` reads the palette)
const MatInfo MATS[IM_COUNT] = {
    {"TFsXMLg", MN_FAUNA,   {1.0, 0.6, 0.25, 0.08, 0.03, 0.02}, 0.89f, 0.85f, 0.73f, FORM_PART_PALE, 0, true, false},   // bone
    {"TFXGL",   MN_FAUNA,   {0.8, 0.7, 0.3, 0.08, 0.03, 0.02}, 0.40f, 0.29f, 0.18f, FORM_PART_DARK, 0, true, false},    // horn
    {"tVXMLs",  MN_SEA,     {0.9, 0.7, 0.35, 0.1, 0.05, 0.03}, 0.93f, 0.85f, 0.80f, FORM_PART_PALE, 0, true, false},    // shell
    {"TVbFXGL", MN_CHITIN,  {0.9, 0.7, 0.3, 0.1, 0.05, 0.03}, 0.24f, 0.25f, 0.17f, FORM_PART_DARK, 0, true, false},     // chitin
    {"Kx",      MN_NONE,    {1.0, 1.0, 0.8, 0.4, 0.1, 0.05}, 0.78f, 0.67f, 0.49f, FORM_PART_CLAY, 0, true, false},      // hide
    {"Rk",      MN_NONE,    {1.0, 1.0, 0.7, 0.3, 0.1, 0.05}, 0.90f, 0.86f, 0.70f, FORM_PART_PALE, 0, true, false},      // gut
    {"TVBFXCGL", MN_WOOD,   {1.0, 1.0, 0.9, 0.5, 0.15, 0.05}, 0.62f, 0.40f, 0.22f, FORM_PART_OCHRE, 0, true, false},   // wood
    {"TVBFXCG", MN_WOOD,    {0.0, 0.6, 0.8, 0.4, 0.1, 0.03}, 0.22f, 0.15f, 0.11f, FORM_PART_DARK, 0, true, false},      // darkwood
    {"V",       MN_GOURD,   {1.0, 0.8, 0.4, 0.1, 0.03, 0.02}, 0.80f, 0.62f, 0.31f, FORM_PART_CLAY, 0, true, false},     // gourd
    {"TBFGlx",  MN_CANE,    {1.0, 0.8, 0.4, 0.15, 0.05, 0.02}, 0.83f, 0.77f, 0.49f, FORM_PART_CLAY, 0, true, false},    // cane
    {"VbKl",    MN_PITH,    {0.9, 0.7, 0.3, 0.1, 0.03, 0.02}, 0.86f, 0.81f, 0.70f, FORM_PART_PALE, 0, true, false},     // pith
    {"tVbSLc",  MN_NONE,    {0.5, 0.5, 0.3, 0.15, 0.3, 0.6}, 0.55f, 0.53f, 0.50f, -1, 0, true, false},                  // stone
    {"TVsl",    MN_NONE,    {0.6, 1.0, 0.6, 0.2, 0.05, 0.02}, 0.70f, 0.40f, 0.25f, FORM_PART_OCHRE, 0, true, false},    // clay
    {"TvSl",    MN_COLD,    {0.7, 0.4, 0.1, 0.0, 0.0, 0.0}, 0.82f, 0.90f, 0.95f, FORM_PART_PALE, 0, true, false},       // ice
    {"tSLMm",   MN_VOLCANIC, {0.6, 0.4, 0.2, 0.05, 0.05, 0.1}, 0.11f, 0.10f, 0.13f, FORM_PART_DARK, 0, true, false},   // obsidian
    {"tvfxM",   MN_SEA,     {0.5, 0.5, 0.3, 0.1, 0.03, 0.02}, 0.84f, 0.44f, 0.38f, FORM_PART_OCHRE, 0, true, false},    // coral
    {"TVFSXMG", MN_NONE,    {0.0, 1.0, 0.6, 0.3, 0.1, 0.02}, 0.72f, 0.52f, 0.27f, FORM_PART_CLAY, 0, false, true},      // bronze
    {"TVFSXMCG", MN_NONE,   {0.0, 0.3, 1.2, 0.8, 0.2, 0.02}, 0.82f, 0.67f, 0.32f, FORM_PART_CLAY, 0, false, true},     // brass
    {"TVSXMl",  MN_NONE,    {0.0, 0.7, 0.6, 0.3, 0.1, 0.02}, 0.72f, 0.38f, 0.22f, FORM_PART_OCHRE, 0, false, true},     // copper
    {"tFSXGl",  MN_NONE,    {0.0, 0.7, 0.8, 0.3, 0.05, 0.02}, 0.30f, 0.30f, 0.32f, FORM_PART_DARK, 0, false, true},     // iron
    {"TBFSXCGR", MN_NONE,   {0.0, 0.0, 0.8, 1.2, 0.5, 0.1}, 0.68f, 0.70f, 0.75f, FORM_PART_METAL, 0, false, true},     // steel
    {"TVfSXM",  MN_NONE,    {0.0, 0.3, 0.4, 0.4, 0.2, 0.1}, 0.80f, 0.82f, 0.86f, FORM_PART_METAL, 0, false, true},     // silver
    {"TVbSMcg", MN_NONE,    {0.0, 0.15, 0.4, 0.6, 1.0, 1.0}, 0.70f, 0.85f, 0.88f, -1, 2, false, false},                // glass
    {"TVfSMr",  MN_NONE,    {0.0, 0.0, 0.0, 0.2, 0.9, 1.4}, 0.78f, 0.90f, 0.98f, -1, 2, false, false},                 // crystal
    {"BCXM",    MN_NONE,    {0.0, 0.5, 0.6, 0.4, 0.2, 0.05}, 0.35f, 0.12f, 0.10f, -1, 1, false, false},                // lacquer
    {"M",       MN_NONE,    {0.3, 0.5, 0.5, 0.4, 0.2, 0.1}, 0.70f, 0.25f, 0.20f, -1, 1, false, false},                 // paint
    {"TVBFXCK", MN_NONE,    {0.0, 0.0, 0.0, 1.2, 0.6, 0.1}, 0.85f, 0.30f, 0.22f, -1, 1, false, false},                 // plastic
    {"TVBFSXCG", MN_NONE,   {0.0, 0.0, 0.0, 0.8, 1.2, 0.6}, 0.74f, 0.76f, 0.80f, FORM_PART_METAL, 0, false, true},     // alloy
    {"TVBFsXCk", MN_NONE,   {0.0, 0.0, 0.0, 0.2, 1.2, 1.0}, 0.50f, 0.52f, 0.55f, -1, 0, false, false},                 // composite
    {"TVBSC",   MN_NONE,    {0.0, 0.0, 0.0, 0.1, 0.8, 0.8}, 0.92f, 0.92f, 0.90f, FORM_PART_PALE, 0, false, false},     // ceramic
    {"XMr",     MN_NONE,    {0.0, 0.0, 0.0, 0.0, 0.5, 0.8}, 1.00f, 0.80f, 0.50f, -1, 3, false, false},                 // light
    {"TVBFSC",  MN_NONE,    {0.0, 0.0, 0.0, 0.0, 0.1, 1.2}, 0.08f, 0.08f, 0.11f, FORM_PART_DARK, 0, false, false},     // void
    {"Rx",      MN_NONE,    {1.0, 1.0, 0.6, 0.4, 0.2, 0.1}, 0.80f, 0.72f, 0.52f, FORM_PART_CLAY, 0, true, false},      // fibre
    {"R",       MN_NONE,    {0.0, 0.7, 1.0, 1.0, 0.6, 0.3}, 0.80f, 0.80f, 0.78f, FORM_PART_METAL, 0, false, true},     // wire
    {"x",       MN_NONE,    {0.0, 0.3, 1.0, 0.8, 0.3, 0.1}, 0.62f, 0.58f, 0.50f, FORM_PART_PALE, 0, false, false},     // felt
    {"",        MN_NONE,    {0, 0, 0, 0, 0, 0}, 0.05f, 0.04f, 0.04f, FORM_PART_DARK, 0, false, false},                 // a hole
};
double useWeight(int mat, int use) {   // 1 a use it has, 0.3 a seldom one, 0 none
    const char L = MU_LETTERS[use];
    for (const char* c = MATS[mat].uses; *c; c++) { if (*c == L) return 1.0; if (*c == L + 32) return 0.3; }
    return 0;
}
}   // namespace

RGB instrumentMaterialColour(int mat, const FormPalette& pal) {
    if (mat < 0 || mat >= IM_COUNT) return RGB(0.5f, 0.5f, 0.5f);
    const MatInfo& m = MATS[mat];
    if (m.part >= 0) return RGB(m.r, m.g, m.b);
    switch (mat) {
        case IM_STONE: return lerp(RGB(m.r, m.g, m.b), pal.colour[0], 0.55f);
        case IM_COMPOSITE: return pal.colour[0];
        case IM_GLASS: return lerp(RGB(0.75f, 0.86f, 0.9f), pal.colour[2], 0.45f);
        case IM_CRYSTAL: return lerp(RGB(0.85f, 0.93f, 1.0f), pal.colour[2], 0.3f);
        case IM_LACQUER: return pal.colour[1] * 0.55f;
        case IM_PAINT: return pal.colour[1];
        case IM_PLASTIC: return lerp(pal.colour[1], RGB(1, 1, 1), 0.1f);
        case IM_LIGHT: return pal.glow ? pal.colour[3] : RGB(0.6f, 0.95f, 1.0f);
        default: return RGB(m.r, m.g, m.b);
    }
}
void instrumentMaterialPart(int mat, int& part, int& slot) {
    if (mat < 0 || mat >= IM_COUNT) { part = FORM_PART_DARK; slot = 0; return; }
    part = MATS[mat].part >= 0 ? MATS[mat].part : 3; slot = MATS[mat].part >= 0 ? 0 : MATS[mat].slot;
}
bool instrumentMaterialMetal(int mat) { return mat >= 0 && mat < IM_COUNT && MATS[mat].metal; }

// ---- each people's way of making ------------------------------------------------------------------------------------------------

namespace {
struct Organology {
    uint64_t seed = 0;
    double avail[IM_COUNT] = {};   // what the world gave (0 none .. 1), whatever the age
    double love[IM_COUNT] = {};    // how much the people loved it above the rest (1 none)
    double lean[IP_COUNT] = {};    // its plans' leanings
    // the genome's style
    double taper = 1, waist = 0, square = 2, dome = 0; int sides = 0, skew = 0;
    int hole = 0, top = 0, deco = 0, motif = 0, sym = 0, number = 5, rhythm = 0, light = 0, accent = 0;
    double slender = 0.5, painted = 0; int massing = 0, plan = 0, roof = 0;
    double machine = 0.5;          // its chance of a machine for its wax cylinders
};

double keyUnit(uint64_t seed, uint64_t k) { return unitFromHash(mix64(seed ^ (0xC17A11ULL + k * 0x9E3779B97F4A7C15ULL))); }

Organology organologyOf(const BodyGen& gNow, const Lore& L) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gNow, gTmp);   // P-04: the world in their time (a dead world's living one)
    Organology O;
    const uint64_t ps = loreSeed(g, L);
    O.seed = mix64(ps ^ 0xC17ULL);
    const Genome G = genomeOf(ps);
    // the world in their time: its flora's families give wood, stalks, gourds or pith; its animals bone, horn, hide and gut, chitin, the sea's
    // shells and coral; the cold ice, the volcanoes obsidian
    const bool desert = kindHas(g.type, KT_DRY);
    const int fam[2] = {g.floraFamily, g.floraFamily2};
    double wood = 0, cane = 0, gourd = 0, pith = 0;
    for (int f : fam) {
        switch (f) {
            case 4: cane = std::max(cane, 1.0); break;                                       // fibrous stalks
            case 5: cane = std::max(cane, 0.5); wood = std::max(wood, 0.6); break;           // fern trees
            case 6: pith = std::max(pith, 1.0); break;                                       // mushroom trees
            case 8: gourd = std::max(gourd, 0.8); wood = std::max(wood, 0.4); break;        // candelabra: ribs and pods
            case 0: case 2: gourd = std::max(gourd, 0.7); wood = std::max(wood, 1.0); break; // domes and umbrellas: their fruit
            default: wood = std::max(wood, 1.0); break;
        }
    }
    if (g.hasTrait(TR_GIANT_FLORA)) wood = std::max(wood, 1.0) * 1.3;
    if (g.hasTrait(TR_LAKELAND)) cane = std::max(cane, 0.5);
    if (desert) { wood *= 0.3; cane *= 0.35; gourd = std::max(gourd * 0.6, 0.3); pith *= 0.3; }
    if (keyUnit(O.seed, 1) < 0.25) gourd = std::max(gourd, 0.35);
    const bool sea = !desert || g.oldSeaM > -1e8;
    int fauna = 0;
    if (keyUnit(g.seed, 2) < 0.78) fauna |= 1;
    if (keyUnit(g.seed, 3) < 0.45) fauna |= 2;
    if (sea && keyUnit(g.seed, 4) < 0.8) fauna |= 4;
    if (!fauna) fauna = 1;
    const bool cold = g.tempBias < -8 || g.hasTrait(TR_GLACIAL) || g.hasTrait(TR_SNOWBALL) || g.iceCapLat < 52;
    const bool volc = g.hasTrait(TR_CINDER_FIELD) || g.hasTrait(TR_BLACK_SAND) || g.hasTrait(TR_TRAPS) || g.hasTrait(TR_GEYSERS) || g.hasTrait(TR_GLASSED) || g.volcanoes > 0.8;
    const bool crystal = g.hasTrait(TR_SPIRES) || g.hasTrait(TR_SALT_FLATS) || g.hasTrait(TR_GLASSED) || keyUnit(g.seed, 5) < 0.08;
    for (int m = 0; m < IM_COUNT; m++) {
        double a = 1;
        switch (MATS[m].need) {
            case MN_FAUNA: a = fauna & 1 ? 1 : 0; break;
            case MN_CHITIN: a = fauna & 2 ? 1 : 0; break;
            case MN_SEA: a = fauna & 4 ? 1 : 0; break;
            case MN_WOOD: a = std::min(1.3, wood); break;
            case MN_GOURD: a = gourd; break;
            case MN_CANE: a = cane; break;
            case MN_PITH: a = pith; break;
            case MN_COLD: a = cold ? 1 : 0; break;
            case MN_VOLCANIC: a = volc ? 1 : 0; break;
            default: break;
        }
        O.avail[m] = a;
        O.love[m] = 1;
    }
    if (desert) { O.avail[IM_STONE] = 1.4; O.avail[IM_CLAY] = 1.2; }
    if (fauna & 1 && !(fauna & 6)) O.avail[IM_BONE] = 1.3;
    O.avail[IM_CRYSTAL] = crystal ? 1.0 : 0.0;   // natural crystal; the made kind comes with the futuristic age (`chooseMat`)
    // the people's loves: three of the natural materials its world gave, a metal, a made one
    {
        const int nat[] = {IM_BONE, IM_HORN, IM_SHELL, IM_CHITIN, IM_WOOD, IM_GOURD, IM_CANE, IM_PITH, IM_STONE, IM_CLAY, IM_ICE, IM_OBSIDIAN, IM_CORAL, IM_CRYSTAL};
        const int N = (int)(sizeof(nat) / sizeof(nat[0]));
        const double boost[3] = {5.0, 2.6, 1.7};
        Rng r(O.seed ^ 0x10FEULL);
        for (int k = 0; k < 3; k++) {
            double w[16]; double tot = 0;
            for (int i = 0; i < N; i++) { w[i] = O.love[nat[i]] > 1 ? 0.0 : O.avail[nat[i]] * r.range(0.2, 1.0); tot += w[i]; }
            if (tot <= 0) break;
            O.love[nat[r.pick(w, N)]] = boost[k];
        }
        const int metals[] = {IM_BRONZE, IM_COPPER, IM_IRON, IM_SILVER, IM_BRASS, IM_STEEL};
        O.love[metals[r.irange(6)]] = 3.0;
        const int made[] = {IM_PLASTIC, IM_ALLOY, IM_GLASS, IM_COMPOSITE, IM_CERAMIC, IM_CRYSTAL, IM_LACQUER};
        O.love[made[r.irange(7)]] *= 2.5;
    }
    // the plans it leaned to: a log-normal draw each, the genome's push
    O.sym = G.of(GV_SYMMETRY); O.plan = G.of(GV_PLAN); O.roof = G.of(GV_ROOF); O.massing = G.of(GV_MASSING);
    for (int p = 0; p < IP_COUNT; p++) {
        const double u1 = std::max(1e-9, keyUnit(O.seed, 100 + p)), u2 = keyUnit(O.seed, 300 + p);
        const double gz = std::sqrt(-2 * std::log(u1)) * std::cos(TAU * u2);
        double w = std::exp(1.1 * gz);
        const unsigned f = INSTRUMENT_PLANS[p].flags;
        if (f & PL_RADIAL) w *= O.sym == SY_RADIAL ? 4.0 : (O.sym == SY_ROTATIONAL ? 2.2 : 0.7);
        if (f & PL_ROUND) w *= O.plan == PF_CIRCLE ? 1.9 : (O.plan == PF_POLYGON || O.roof == RF_DOMED ? 1.3 : (O.plan == PF_RECT ? 0.75 : 1.0));
        if (f & PL_OPEN) w *= O.roof == RF_OPEN ? 2.2 : 1.0;
        if (f & PL_LONG) w *= O.massing == MS_VERTICAL ? 1.7 : (O.massing == MS_COMPACT ? 0.7 : 1.0);
        O.lean[p] = w;
    }
    // the style: the bodies' outline from the plan, the profile and the roof
    Rng s(O.seed ^ 0x57A1EULL);
    switch (O.plan) {
        case PF_RECT: O.square = s.range(5.5, 9); break;
        case PF_CIRCLE: O.square = s.range(1.9, 2.3); break;
        case PF_POLYGON: O.sides = clampi(G.number(), 5, 8); O.square = 2; break;
        case PF_ELLIPSE: O.square = s.range(2.0, 2.6); break;
        case PF_FREE: O.square = s.range(2.2, 3.2); O.skew = (s.chance(0.5) ? 1 : -1) * (1 + s.irange(3)); break;
        default: O.square = s.range(3.0, 4.2); break;   // a court: its middle open (a great sound hole)
    }
    switch (G.of(GV_PROFILE)) {
        case WP_BATTERED: O.taper = s.range(0.48, 0.72); break;
        case WP_BULGING: O.waist = -s.range(0.1, 0.24); O.taper = s.range(0.9, 1.1); break;
        case WP_LEANING: O.skew += (O.skew >= 0 ? 1 : -1) * (2 + s.irange(2)); O.taper = s.range(0.8, 1.0); break;
        case WP_STEPPED: O.waist = s.range(0.26, 0.42); O.taper = s.range(0.75, 0.95); break;
        default: O.taper = s.range(0.85, 1.1); O.waist = s.range(0.0, 0.08); break;
    }
    switch (O.roof) {
        case RF_FLAT: case RF_OPEN: O.dome = 0; break;
        case RF_DOMED: case RF_CONICAL: O.dome = s.range(0.7, 1.0); break;
        case RF_STEPPED: O.dome = 0.15; break;
        default: O.dome = s.range(0.3, 0.5); break;
    }
    static const int HOLE[6] = {2, 5, 0, 1, 3, 4};   // the openings (square, arched, round, slit, triangular, keyhole) as marks
    O.hole = HOLE[clampi(G.of(GV_OPENING), 0, 5)];
    O.top = clampi(G.of(GV_TOPS), 0, 4); O.deco = clampi(G.of(GV_ORNAMENT), 0, 3); O.motif = G.of(GV_MOTIF);
    O.number = G.number(); O.rhythm = G.of(GV_RHYTHM); O.light = G.of(GV_LIGHT); O.accent = G.accentPlace();
    O.slender = G.at(GV_PROPORTION); O.painted = G.at(GV_PAINTED);
    O.machine = 0.35 + 0.4 * keyUnit(O.seed, 7);
    return O;
}

// a material for a use in an age: what the world gave, the age's weight, the people's love, the plan's own leaning (`prefer`)
int chooseMat(const Organology& O, int age, int use, Rng& rng, const int* prefer = nullptr, double preferW = 0) {
    double w[IM_COUNT]; double tot = 0;
    for (int m = 0; m < IM_COUNT; m++) {
        double x = useWeight(m, use);
        if (x <= 0) { w[m] = 0; continue; }
        double av = O.avail[m];
        if (m == IM_CRYSTAL) av = std::max(av * (age <= AGE_MEDIEVAL ? 1.0 : 0.3), age >= AGE_FUTURISTIC ? 1.0 : (age == AGE_MODERN ? 0.4 : 0.0));
        if (m == IM_GLASS && O.avail[IM_OBSIDIAN] > 0 && age <= AGE_MEDIEVAL) av = std::max(av, 0.3);
        x *= av * MATS[m].age[clampi(age, 0, AGE_COUNT - 1)] * O.love[m];
        if (prefer) for (const int* p = prefer; *p >= 0; p++) if (*p == m) x *= preferW;
        w[m] = x; tot += x;
    }
    if (tot <= 0) {   // nothing of the use: the commonest of each age
        static const int FALL[AGE_COUNT] = {IM_STONE, IM_CLAY, IM_BRASS, IM_STEEL, IM_ALLOY, IM_CRYSTAL};
        return FALL[clampi(age, 0, AGE_COUNT - 1)];
    }
    return rng.pick(w, IM_COUNT);
}
}   // namespace

// ---- the makes ------------------------------------------------------------------------------------------------------------------

int instrumentClassOf(const Timbre& tb, int role, int age, bool kit) {
    if (kit) {
        switch (tb.family) {
            case TF_DRUM: return IC_SKIN;
            case TF_LOG: return IC_LOG;
            case TF_RATTLE: return IC_RATTLE;
            case TF_JINGLE: return IC_JINGLE;
            case TF_CYMBAL: return age == AGE_INDUSTRIAL ? IC_CYMBAL : -1;
            default: return -1;
        }
    }
    switch (tb.family) {
        case TF_PIPE: return IC_EDGE;
        case TF_REED: return role == MR_DRONE ? -1 : (tb.copies >= 2 ? IC_FREE_REED : IC_REED);
        case TF_BRASS: return IC_LIP;
        case TF_PLUCKED: return tb.kind == 1 ? IC_MOUTH_PLUCK : IC_PLUCK;
        case TF_BOWED: return role == MR_DRONE ? -1 : IC_BOW;
        case TF_METAL: return IC_STRUCK;
        case TF_ROARER: return age == AGE_FAR_FUTURE ? IC_HUM : IC_WHIRL;
        case TF_MUSICBOX: return IC_COMB;
        case TF_ESTRING: return IC_ESTRING;
        case TF_EKEYS: return tb.kind == 1 ? IC_GLASS : IC_EKEYS;
        case TF_ORGAN: return tb.kind == 2 ? IC_EKEYS : -1;
        case TF_SYNTH: return tb.kind == 3 ? -1 : IC_SYNTH;
        case TF_PAD: return IC_SYNTH;
        case TF_SWARM: return IC_SWARM;
        case TF_FIELD: return IC_FIELD;
        default: return -1;   // voices, the synthetic voices, the pianos, a church's or a barrel's organ
    }
}

namespace {
int registerOf(int cls, int role, int age) {
    if (role == MR_BASS) return -1;
    switch (cls) {
        case IC_EDGE: case IC_GLASS: case IC_COMB: return 1;
        case IC_STRUCK: return age == AGE_FAR_FUTURE ? 0 : 1;
        case IC_SKIN: return age == AGE_MEDIEVAL ? 0 : -1;
        default: return 0;
    }
}

std::string withArticle(const std::string& s) {
    if (s.empty()) return s;
    const char c = s[0];
    return (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' ? "AN " : "A ") + s;
}
std::string W(int mat) { return mat >= 0 && mat < IM_COUNT ? INSTRUMENT_MATERIAL_NAMES[mat] : ""; }
// a vessel's noun by what it is made of
const char* vesselNoun(int mat, int variant) {
    switch (mat) {
        case IM_GOURD: return "GOURD";
        case IM_SHELL: case IM_CORAL: return "SHELL";
        case IM_CHITIN: return "CARAPACE";
        case IM_CLAY: case IM_CERAMIC: return "POT";
        case IM_PITH: return "POD";
        case IM_STONE: case IM_OBSIDIAN: return "STONE";
        case IM_WOOD: case IM_DARKWOOD: case IM_LACQUER: return variant % 2 ? "BOX" : "BOWL";
        default: return "BOWL";
    }
}
// a late body's noun by its outline
const char* shapeNoun(const InstrumentMake& M) {
    if (M.sym == SY_RADIAL) return "STAR";
    if (M.sides >= 5) return "PRISM";
    if (M.taper < 0.7) return "WEDGE";
    if (M.waist > 0.22) return "TWIN";   // two lobes about a waist
    if (M.square < 2.5) return M.slender > 0.62 ? "BLADE" : "DISC";
    return "SLAB";
}
std::string nameOf(const InstrumentMake& M, bool plural) {
    const std::string m = W(M.mat), m2 = W(M.mat2);
    auto pl = [&](const std::string& one, const std::string& many) { return plural ? many : withArticle(one); };
    switch (M.plan) {
        case IP_PIPE: case IP_CROSSPIPE: return pl(m + " PIPE", m + " PIPES");
        case IP_VESSEL: return pl(m + " WHISTLE", m + " WHISTLES");   // bare: "A WHISTLE"
        case IP_RAFT: return m.empty() ? pl("RAFT OF PIPES", "RAFTS OF PIPES") : m + " PIPES";
        case IP_PIPE_RING: return pl(M.motif == MO_STARS ? "STAR OF PIPES" : "RING OF PIPES", "RINGS OF PIPES");
        case IP_TWIN_PIPE: return m.empty() ? pl("TWIN PIPE", "TWIN PIPES") : pl("TWIN " + m + " PIPE", "TWIN PIPES");
        case IP_REED_PIPE: return pl(m + " REED PIPE", "REED PIPES");
        case IP_TWIN_REED: return pl("DOUBLE REED", "DOUBLE REEDS");
        case IP_BAG: return pl("BAG OF PIPES", "BAGS OF PIPES");
        case IP_CHAMBER: return pl(std::string(M.mat == IM_GOURD ? "GOURD" : (M.mat == IM_SHELL ? "SHELL" : "BOWL")) + " OF PIPES", "BOWLS OF PIPES");
        case IP_BELLOWS: return pl("BELLOWS BOX", "BELLOWS BOXES");
        case IP_BELLOWS_ROUND: return pl("ROUND BELLOWS", "ROUND BELLOWS");
        case IP_HORN_COIL: return pl("COILED HORN", "COILED HORNS");
        case IP_HORN_FOLDED: return pl("FOLDED HORN", "FOLDED HORNS");
        case IP_HORN_STRAIGHT: return m.empty() || M.mat == IM_HORN ? pl("LONG HORN", "LONG HORNS") : pl(m + " HORN", m + " HORNS");
        case IP_HORN_SPIRAL: return pl("SPIRAL HORN", "SPIRAL HORNS");
        case IP_HORN_FORKED: return pl("FORKED HORN", "FORKED HORNS");
        case IP_HORN_UPRIGHT: return pl("GREAT HORN", "GREAT HORNS");
        case IP_NECKED: return pl(std::string("NECKED ") + vesselNoun(M.mat, M.variant), std::string("NECKED ") + vesselNoun(M.mat, M.variant) + "S");
        case IP_FRAME: return M.mat2 == IM_HORN || M.mat2 == IM_BONE || M.mat2 == IM_CHITIN || M.mat2 == IM_CORAL ? pl(m2 + " FRAME", m2 + " FRAMES") : pl("STRUNG FRAME", "STRUNG FRAMES");
        case IP_HARP: return pl(m2 + " HARP", m2 + " HARPS");   // bare: "A HARP"
        case IP_BOARD: return pl("STRING BOARD", "STRING BOARDS");
        case IP_TUBE_ZITHER: return pl(M.mat == IM_CANE ? "STRUNG CANE" : "STRUNG TUBE", "STRUNG TUBES");
        case IP_RING_HARP: return pl("STRUNG RING", "STRUNG RINGS");
        case IP_TONGUES: return pl(std::string("TONGUE ") + (M.variant % 2 ? vesselNoun(M.mat, 0) : "BOARD"), "TONGUE BOARDS");
        case IP_BOW: return pl(M.variant % 3 == 0 ? "MOUTH BOW" : (M.mat2 == IM_GOURD ? "GOURD BOW" : "STRUNG BOW"), "STRUNG BOWS");
        case IP_JAW: return m.empty() ? pl("MOUTH TONGUE", "MOUTH TONGUES") : pl(m + " TONGUE", m + " TONGUES");
        case IP_BOW_HARP: return plural ? std::string("BOWS ON BOXES") : "BOWS ON A " + std::string(vesselNoun(M.mat, 1));
        case IP_FIDDLE: return pl(std::string("BOWED ") + vesselNoun(M.mat, M.variant), "BOWED BODIES");
        case IP_SPIKE: return pl(std::string("SPIKED ") + vesselNoun(M.mat, 0), "SPIKED BODIES");
        case IP_TUBE_FIDDLE: return pl("BOWED TUBE", "BOWED TUBES");
        case IP_BOWED_BOARD: return pl("BOWED BOARD", "BOWED BOARDS");
        case IP_WHEEL: return pl("WHEEL FIDDLE", "WHEEL FIDDLES");
        case IP_HORN_FIDDLE: return pl("BOWED HORN", "BOWED HORNS");
        case IP_BARS: return m.empty() ? std::string("TUNED BARS") : m + " BARS";
        case IP_TUBES: return m.empty() ? std::string("HUNG TUBES") : m + " TUBES";
        case IP_BELLS: return m.empty() ? pl("SET OF BELLS", "SETS OF BELLS") : m + " BELLS";
        case IP_BELL_TREE: return pl("BELL TREE", "BELL TREES");
        case IP_BOWLS: return m.empty() ? pl("ROW OF BOWLS", "ROWS OF BOWLS") : m + " BOWLS";
        case IP_GONGS: return M.variant % 2 ? pl(m + " GONG", m + " GONGS") : pl("ROW OF GONGS", "ROWS OF GONGS");
        case IP_STONES: return M.mat == IM_STONE ? std::string("RINGING STONES") : (m.empty() ? std::string("RINGING SLABS") : m + " SLABS");
        case IP_SPOKES: return pl("WHEEL OF BARS", "WHEELS OF BARS");
        case IP_BLADE: return pl(M.mat == IM_WOOD || M.mat == IM_DARKWOOD || m.empty() ? std::string("WHIRLED BLADE") : "WHIRLED " + m, "WHIRLED BLADES");
        case IP_BUZZER: return pl("WHIRLED DISC", "WHIRLED DISCS");
        case IP_WHIRL_TUBE: return pl("WHIRLED TUBE", "WHIRLED TUBES");
        case IP_HUM_ORB: return pl("HUMMING ORB", "HUMMING ORBS");
        case IP_HUM_RING: return pl("HUMMING RING", "HUMMING RINGS");
        case IP_TUNE_DRUM: { static const char* C[4] = {"TUNE BOX", "TUNE DRUM", "TUNE DOME", "TUNE CASKET"}; return pl(C[M.variant % 4], "TUNE BOXES"); }
        case IP_TUNE_DISC: return pl("TUNE DISC", "TUNE DISCS");
        case IP_SOLID: return pl(std::string(M.reg < 0 ? "BASS " : "ELECTRIC ") + shapeNoun(M), std::string(M.reg < 0 ? "BASS " : "ELECTRIC ") + shapeNoun(M) + "S");
        case IP_SKELETON: return pl(M.reg < 0 ? "BASS FRAME" : "ELECTRIC FRAME", "ELECTRIC FRAMES");
        case IP_LAP: return pl("LAP SLAB", "LAP SLABS");
        case IP_KEY_SLAB: return pl(std::string("KEYED ") + shapeNoun(M), "KEYED SLABS");
        case IP_KEY_ARC: return pl("KEYED ARC", "KEYED ARCS");
        case IP_KEY_RING: return pl("KEYED RING", "KEYED RINGS");
        case IP_GLASS_RODS: return m.empty() ? std::string("SINGING RODS") : m + " RODS";
        case IP_GLASS_BARS: return m.empty() ? std::string("TUNED BARS") : m + " BARS";
        case IP_GLASS_BOWLS: return m.empty() ? std::string("SPINNING BOWLS") : m + " BOWLS";
        case IP_PANEL: return pl(std::string("TONE ") + shapeNoun(M), "TONE SLABS");
        case IP_RIBBON: return pl("TONE RIBBON", "TONE RIBBONS");
        case IP_PAD_GRID: return pl("TONE GRID", "TONE GRIDS");
        case IP_ORB: return pl("TONE ORB", "TONE ORBS");
        case IP_TONE_STRIPS: return pl("TONE PANEL", "TONE PANELS");
        case IP_ROD_FIELD: return pl("FOREST OF RODS", "FORESTS OF RODS");
        case IP_LATTICE: return pl("SINGING LATTICE", "SINGING LATTICES");
        case IP_CLOUD: return pl("CLOUD OF BEADS", "CLOUDS OF BEADS");
        case IP_RINGS: return M.number <= 4 ? pl("SINGING RING", "SINGING RINGS") : std::string("NESTED RINGS");
        case IP_MONOLITH: return pl(M.reg < 0 ? "DEEP STONE" : "SINGING STONE", "SINGING STONES");
        case IP_STAR: return m.empty() ? pl("STAR OF RODS", "STARS OF RODS") : pl(m + " STAR", m + " STARS");
        case IP_FIELD_ORB: return pl("SINGING ORB", "SINGING ORBS");
        case IP_SHELLS: return std::string("NESTED SHELLS");
        case IP_FRAME_DRUM: return pl("FRAME DRUM", "FRAME DRUMS");
        case IP_BARREL: return pl("BARREL DRUM", "BARREL DRUMS");
        case IP_GOBLET: return pl("GOBLET DRUM", "GOBLET DRUMS");
        case IP_POT_DRUM: return m.empty() ? pl("POT DRUM", "POT DRUMS") : pl(m + " DRUM", m + " DRUMS");
        case IP_HOURGLASS: return plural ? std::string("HOURGLASS DRUMS") : std::string("AN HOURGLASS DRUM");
        case IP_KETTLE: return pl("KETTLE DRUM", "KETTLE DRUMS");
        case IP_CARAPACE: return m.empty() ? pl("SHELL DRUM", "SHELL DRUMS") : pl(m + " DRUM", m + " DRUMS");
        case IP_DRUM_PAIR: return pl("PAIR OF DRUMS", "PAIRED DRUMS");
        case IP_SLIT_LOG:
            return M.mat == IM_WOOD || M.mat == IM_DARKWOOD ? pl("SLIT LOG", "SLIT LOGS") : pl(std::string("SLIT ") + vesselNoun(M.mat, 0), std::string("SLIT ") + vesselNoun(M.mat, 0) + "S");
        case IP_TONGUE_BOX: return pl("TONGUE DRUM", "TONGUE DRUMS");
        case IP_SLIT_CANES: return std::string("SLIT CANES");
        case IP_VESSEL_RATTLE: case IP_STRUNG_RATTLE: return pl(m + " RATTLE", m + " RATTLES");
        case IP_STICK_RATTLE: return pl("RATTLE STICK", "RATTLE STICKS");
        case IP_RAIN_TUBE: return pl("RAIN TUBE", "RAIN TUBES");
        case IP_SISTRUM: return plural ? std::string("JINGLES") : withArticle("JINGLE FORK");
        case IP_JINGLE_HOOP: return plural ? std::string("JINGLES") : withArticle("JINGLE HOOP");
        case IP_JINGLE_STICK: return plural ? std::string("JINGLES") : withArticle("JINGLE STICK");
        case IP_CYMBALS: return m.empty() ? std::string("CYMBALS") : m + " CYMBALS";
        case IP_FINGER_CYMBALS: return plural ? m + " CHIMES" : "SMALL " + m + " CYMBALS";
        case IP_HAND_CYMBAL: return pl(m + " CYMBAL", m + " CYMBALS");
        case IP_PAD_BOX: return withArticle(std::string("BEAT ") + shapeNoun(M));
        case IP_PAD_RING: return withArticle("BEAT RING");
        case IP_HORN_MACHINE: return withArticle("TALKING MACHINE");
        case IP_CABINET: return withArticle("CYLINDER BOX");
        default: return withArticle("LISTENING BOX");
    }
}
// the names: past seventeen letters (the case's list), the material is left out (a word never cut)
std::string shortName(InstrumentMake M, bool plural) {
    std::string s = nameOf(M, plural);
    if (s.size() <= 17) return s;
    M.mat = M.mat2 = IM_HOLE;   // no material: its word empty
    std::string t = nameOf(M, plural), u;
    for (size_t i = 0; i < t.size(); i++) if (!(t[i] == ' ' && (u.empty() || u.back() == ' '))) u += t[i];
    while (!u.empty() && u.back() == ' ') u.pop_back();
    if (u.compare(0, 3, "AN ") == 0 && u.size() > 3 && !std::strchr("AEIOU", u[3])) u = "A " + u.substr(3);
    if (u.compare(0, 2, "A ") == 0 && u.size() > 2 && std::strchr("AEIOU", u[2])) u = "AN " + u.substr(2);
    if (u.size() <= 17) return u;
    const size_t cut = u.rfind(' ', 17);   // the last whole word that fits
    return cut != std::string::npos && cut > 2 ? u.substr(0, cut) : u.substr(0, 17);
}

// the materials a plan is made of, and its own choices
void planMaterials(InstrumentMake& M, const Organology& O, Rng& rng) {
    const int a = M.age;
    auto mat = [&](int use) { return chooseMat(O, a, use, rng); };
    auto matP = [&](int use, std::initializer_list<int> pref, double w) { std::vector<int> v(pref); v.push_back(-1); return chooseMat(O, a, use, rng, v.data(), w); };
    M.fit = mat(MU_FIT); M.trim = mat(MU_TRIM); M.str = mat(MU_STRING); M.skin = mat(MU_SKIN);
    M.stick = matP(MU_FRAME, {IM_WOOD, IM_DARKWOOD, IM_CANE, IM_HORN, IM_BONE}, 3); M.vessel = matP(MU_VESSEL, {IM_GOURD, IM_SHELL, IM_CLAY}, 3); M.tongue = mat(MU_TONGUE);
    if (a >= AGE_MEDIEVAL && M.str == IM_GUT && rng.chance(0.3)) M.str = IM_FIBRE;
    if (a >= AGE_INDUSTRIAL && rng.chance(0.75)) M.str = IM_WIRE;
    switch (M.plan) {
        case IP_PIPE: case IP_CROSSPIPE: case IP_TWIN_PIPE: case IP_REED_PIPE: case IP_TWIN_REED: case IP_RAFT: case IP_PIPE_RING:
            M.mat = mat(MU_TUBE); M.mat2 = mat(MU_FIT); break;
        case IP_VESSEL: M.mat = mat(MU_VESSEL); M.mat2 = M.mat; break;
        case IP_BAG: M.mat = IM_HIDE; M.mat2 = mat(MU_TUBE); break;
        case IP_CHAMBER: M.mat = matP(MU_VESSEL, {IM_GOURD, IM_SHELL, IM_CLAY}, 3); M.mat2 = mat(MU_TUBE); break;
        case IP_BELLOWS: case IP_BELLOWS_ROUND: M.mat = mat(MU_BOARD); M.mat2 = M.skin; break;
        case IP_HORN_COIL: case IP_HORN_FOLDED: case IP_HORN_STRAIGHT: case IP_HORN_SPIRAL: case IP_HORN_FORKED: case IP_HORN_UPRIGHT:
            M.mat = a >= AGE_INDUSTRIAL ? matP(MU_TUBE, {IM_BRASS, IM_COPPER, IM_SILVER, IM_STEEL, IM_BRONZE, IM_ALLOY}, 6) : mat(MU_TUBE); M.mat2 = mat(MU_FIT); break;
        case IP_NECKED: case IP_FIDDLE: case IP_SPIKE: case IP_TONGUES: case IP_WHEEL:
            M.mat = M.variant % 2 && M.plan != IP_SPIKE ? mat(MU_BOARD) : mat(MU_VESSEL); M.mat2 = mat(MU_FRAME); break;
        case IP_FRAME: case IP_HARP: M.mat = rng.chance(0.5) ? mat(MU_VESSEL) : mat(MU_BOARD); M.mat2 = mat(MU_FRAME); break;
        case IP_BOARD: case IP_BOWED_BOARD: M.mat = mat(MU_BOARD); M.mat2 = mat(MU_FRAME); break;
        case IP_TUBE_ZITHER: case IP_TUBE_FIDDLE: M.mat = matP(MU_TUBE, {IM_CANE, IM_WOOD}, 6); M.mat2 = mat(MU_FRAME); break;
        case IP_RING_HARP: M.mat = mat(MU_FRAME); M.mat2 = mat(MU_VESSEL); break;
        case IP_BOW: M.mat = mat(MU_FRAME); M.mat2 = rng.chance(0.6) ? matP(MU_VESSEL, {IM_GOURD, IM_SHELL}, 4) : M.mat; break;
        case IP_JAW: M.mat = mat(MU_TONGUE); M.mat2 = M.mat; break;
        case IP_BOW_HARP: M.mat = rng.chance(0.5) ? mat(MU_VESSEL) : mat(MU_BOARD); M.mat2 = mat(MU_FRAME); break;
        case IP_HORN_FIDDLE: M.mat = mat(MU_FRAME); M.mat2 = matP(MU_TUBE, {IM_BRASS, IM_COPPER, IM_STEEL}, 5); break;
        case IP_BARS: case IP_TUBES: case IP_BELLS: case IP_BELL_TREE: case IP_BOWLS: case IP_GONGS: case IP_STONES: case IP_SPOKES:
            M.mat = mat(MU_STRUCK); M.mat2 = mat(MU_FRAME);
            if (M.plan == IP_STONES && !instrumentMaterialMetal(M.mat) && M.mat != IM_STONE && rng.chance(0.4)) M.mat = IM_STONE;
            if (M.plan == IP_STONES && instrumentMaterialMetal(M.mat)) M.mat = O.avail[IM_OBSIDIAN] > 0 && rng.chance(0.5) ? IM_OBSIDIAN : IM_STONE;
            break;
        case IP_BLADE: case IP_BUZZER: M.mat = mat(MU_BLADE); M.mat2 = M.str == IM_WIRE ? IM_FIBRE : M.str; break;
        case IP_WHIRL_TUBE: M.mat = mat(MU_TUBE); M.mat2 = mat(MU_FIT); break;
        case IP_HUM_ORB: case IP_HUM_RING: case IP_RINGS: case IP_MONOLITH: case IP_STAR: case IP_FIELD_ORB: case IP_SHELLS:
            M.mat = mat(M.plan == IP_STAR ? MU_TUBE : MU_STRUCK); M.mat2 = mat(MU_CASE); M.trim = IM_LIGHT; break;
        case IP_TUNE_DRUM: case IP_TUNE_DISC: M.mat = mat(MU_CASE); M.mat2 = matP(MU_FIT, {IM_BRASS, IM_STEEL, IM_SILVER}, 6); break;
        case IP_SOLID: case IP_SKELETON: case IP_LAP: case IP_KEY_SLAB: case IP_KEY_ARC: case IP_KEY_RING: case IP_PANEL: case IP_RIBBON:
        case IP_PAD_GRID: case IP_ORB: case IP_TONE_STRIPS: case IP_PAD_BOX: case IP_PAD_RING:
            M.mat = mat(MU_CASE); M.mat2 = mat(MU_CASE); if (a >= AGE_FUTURISTIC) M.trim = IM_LIGHT; break;
        case IP_GLASS_RODS: case IP_GLASS_BARS: case IP_GLASS_BOWLS: M.mat = matP(MU_STRUCK, {IM_GLASS, IM_CRYSTAL}, 30); M.mat2 = mat(MU_CASE); break;
        case IP_ROD_FIELD: case IP_LATTICE: case IP_CLOUD: M.mat = mat(MU_STRUCK); M.mat2 = mat(MU_CASE); M.trim = IM_LIGHT; break;
        case IP_FRAME_DRUM: case IP_BARREL: case IP_GOBLET: case IP_HOURGLASS: case IP_DRUM_PAIR:
            M.mat = M.plan == IP_FRAME_DRUM ? mat(MU_FRAME) : mat(MU_VESSEL); M.mat2 = mat(MU_FIT); break;
        case IP_POT_DRUM: M.mat = matP(MU_VESSEL, {IM_CLAY, IM_GOURD, IM_STONE, IM_CERAMIC}, 4); M.mat2 = mat(MU_FIT); break;
        case IP_KETTLE: M.mat = matP(MU_VESSEL, {IM_COPPER, IM_BRASS, IM_BRONZE, IM_CLAY}, 5); M.mat2 = mat(MU_FIT); break;
        case IP_CARAPACE: M.mat = O.avail[IM_CHITIN] > 0 && (O.avail[IM_SHELL] <= 0 || rng.chance(0.5)) ? IM_CHITIN : IM_SHELL; M.mat2 = mat(MU_FIT); break;
        case IP_SLIT_LOG: M.mat = matP(MU_VESSEL, {IM_WOOD, IM_DARKWOOD}, 6); M.mat2 = mat(MU_FRAME); break;
        case IP_TONGUE_BOX: M.mat = mat(MU_BOARD); M.mat2 = mat(MU_FRAME); break;
        case IP_SLIT_CANES: M.mat = IM_CANE; M.mat2 = mat(MU_FIT); break;
        case IP_VESSEL_RATTLE: M.mat = mat(MU_VESSEL); M.mat2 = mat(MU_FRAME); break;
        case IP_STRUNG_RATTLE:
            M.mat = matP(MU_TRIM, {IM_SHELL, IM_BONE, IM_CORAL, IM_BRASS, IM_COPPER}, 2);
            if (M.mat == IM_PAINT || M.mat == IM_LACQUER || M.mat == IM_LIGHT) M.mat = mat(MU_FIT);
            M.mat2 = mat(MU_FRAME);
            break;
        case IP_STICK_RATTLE: case IP_RAIN_TUBE: M.mat = mat(M.plan == IP_RAIN_TUBE ? MU_TUBE : MU_FRAME); M.mat2 = mat(MU_FIT); break;
        case IP_SISTRUM:
        case IP_JINGLE_HOOP:
        case IP_JINGLE_STICK:
            M.mat = mat(MU_FRAME);
            M.mat2 = a >= AGE_MEDIEVAL ? matP(MU_FIT, {IM_BRASS, IM_BRONZE, IM_COPPER, IM_SILVER, IM_IRON}, 8) : mat(MU_FIT);
            break;
        case IP_CYMBALS: case IP_FINGER_CYMBALS: case IP_HAND_CYMBAL: M.mat = matP(MU_STRUCK, {IM_BRASS, IM_BRONZE, IM_COPPER, IM_STEEL, IM_SILVER}, 60); M.mat2 = IM_HIDE; break;
        default: M.mat = mat(MU_CASE); M.mat2 = matP(MU_TUBE, {IM_BRASS, IM_COPPER, IM_STEEL, IM_SILVER}, 6); break;   // the machines
    }
    if (M.mat == IM_HOLE) M.mat = IM_WOOD;
}

bool planPossible(int p, const Organology& O, int age, bool bass) {
    const InstrumentPlanInfo& P = INSTRUMENT_PLANS[p];
    if (age < P.ageMin || age > P.ageMax) return false;
    if ((P.flags & PL_NOBASS) && bass) return false;
    if ((P.flags & PL_BASSONLY) && !bass) return false;
    if ((P.flags & PL_CANE) && O.avail[IM_CANE] <= 0 && (p == IP_SLIT_CANES || O.avail[IM_WOOD] <= 0)) return false;
    if ((P.flags & PL_CARAPACE) && O.avail[IM_CHITIN] <= 0 && O.avail[IM_SHELL] <= 0) return false;
    return true;
}

InstrumentMake makeFor(const Organology& O, int age, int cls, int role, uint64_t key, int forcePlan = -1, const std::vector<int>* avoid = nullptr) {
    InstrumentMake M; M.cls = cls; M.age = age;
    M.seed = mix64(O.seed ^ 0xC17A5EULL ^ ((uint64_t)(age + 1) * 0x9E3779B97F4A7C15ULL) ^ ((uint64_t)(cls + 1) * 0xBF58476D1CE4E5B9ULL) ^ (key * 0xD1B54A32D192ED03ULL));
    Rng rng(M.seed);
    M.reg = registerOf(cls, role, age);
    const bool bass = M.reg < 0 && cls != IC_SKIN;
    double w[IP_COUNT]; double tot = 0;
    for (int p = 0; p < IP_COUNT; p++) {
        w[p] = INSTRUMENT_PLANS[p].cls == cls && planPossible(p, O, age, bass) ? INSTRUMENT_PLANS[p].weight * O.lean[p] : 0;
        if (avoid && std::find(avoid->begin(), avoid->end(), p) != avoid->end()) w[p] *= 0.02;   // an age's second of a class: another plan, nearly always
        tot += w[p];
    }
    if (tot <= 0) { for (int p = 0; p < IP_COUNT; p++) if (INSTRUMENT_PLANS[p].cls == cls) { M.plan = p; break; } }
    else M.plan = rng.pick(w, IP_COUNT);
    if (forcePlan >= 0) M.plan = forcePlan;
    M.variant = rng.irange(12);
    planMaterials(M, O, rng);
    // the people's style, each instrument a little its own
    M.square = O.square * rng.range(0.9, 1.1); M.taper = O.taper + rng.sym(0.05); M.waist = O.waist + rng.sym(0.03); M.dome = O.dome;
    M.sides = O.sides; M.skew = O.skew;
    M.hole = O.hole; M.top = O.top; M.deco = O.deco; M.motif = O.motif; M.sym = O.sym; M.number = O.number; M.rhythm = O.rhythm;
    M.light = O.light; M.accent = O.accent;
    M.slender = clampd(O.slender + rng.sym(0.08), 0, 1);
    M.painted = O.painted;
    M.size = rng.range(0.9, 1.1) * (M.reg < 0 && cls != IC_SKIN ? 1.45 : (M.reg > 0 ? 0.85 : 1.0));   // a bass is bigger; a drum is low whatever its size
    M.name = shortName(M, false); M.plural = shortName(M, true);
    return M;
}
}   // namespace

InstrumentMake instrumentMakeOfPlan(const BodyGen& g, const Lore& L, int plan, int age, uint64_t salt) {
    const Organology O = organologyOf(g, L);
    plan = clampi(plan, 0, IP_COUNT - 1);
    const int cls = INSTRUMENT_PLANS[plan].cls;
    const int role = (INSTRUMENT_PLANS[plan].flags & PL_BASSONLY) ? MR_BASS : MR_LEAD;
    return makeFor(O, clampi(age, 0, AGE_COUNT - 1), cls, role, 0x9A11ULL + salt * 7919ULL + (uint64_t)plan, plan);
}
bool instrumentMaterialGiven(const BodyGen& g, const Lore& L, int mat, int age) {
    if (mat < 0 || mat >= IM_COUNT) return false;
    if (mat == IM_HOLE) return true;
    const Organology O = organologyOf(g, L);
    double av = O.avail[mat];
    if (mat == IM_CRYSTAL) av = std::max(av, age >= AGE_MODERN ? 1.0 : 0.0);
    if (mat == IM_GLASS && O.avail[IM_OBSIDIAN] > 0) av = std::max(av, 0.3);
    if (mat == IM_HIDE || mat == IM_PAINT) return true;   // a drum's skin of any beast; paint
    if (mat == IM_LIGHT) return age >= AGE_FUTURISTIC;
    return av > 0 && MATS[mat].age[clampi(age, 0, AGE_COUNT - 1)] > 0;
}

void traditionMakes(Tradition& T, const BodyGen& g, const Lore& L) {
    T.makes.clear(); T.machine = -1;
    const Organology O = organologyOf(g, L);
    // the lead's first where an age had a kind twice (a modern people's synthesiser is its lead's, not its bass's)
    static const int ORDER[MR_COUNT] = {MR_LEAD, MR_SECOND, MR_ALT, MR_CHORDS, MR_BELL, MR_BOX, MR_KEYS, MR_SWARM, MR_SINGER, MR_CHORUS, MR_ORGAN, MR_DRONE, MR_BASS};
    for (int a = 0; a < AGE_COUNT; a++) {
        Ensemble& E = T.ages[a];
        std::vector<std::pair<uint64_t, int>> keys;   // a kind of voice in this age and its make
        std::vector<int> used[IC_COUNT];               // the plans this age's makes of a class were built on
        auto makeOf = [&](uint64_t key, int cls, int role) {
            for (const auto& k : keys) if (k.first == key) return k.second;
            T.makes.push_back(makeFor(O, a, cls, role, key, -1, &used[clampi(cls, 0, IC_COUNT - 1)]));
            used[clampi(cls, 0, IC_COUNT - 1)].push_back(T.makes.back().plan);
            keys.push_back({key, (int)T.makes.size() - 1});
            return (int)T.makes.size() - 1;
        };
        for (int r : ORDER)
            for (size_t v = 0; v < E.voices.size() && v < E.role.size(); v++) {
                if (E.role[v] != r) continue;
                Timbre& tb = E.voices[v];
                const int cls = instrumentClassOf(tb, r, a, false);
                if (cls < 0) continue;
                // a voice of the same family and kind is the same instrument (the strings' lead and second; free reeds' tune, chords and bass),
                // a bass its own (the bowed bass, the bass horn, the electric bass)
                const bool keyed = tb.family == TF_SYNTH || tb.family == TF_BRASS || tb.family == TF_ESTRING || tb.family == TF_FIELD;
                const bool bass = r == MR_BASS && cls != IC_FREE_REED;
                const uint64_t key = (uint64_t)(cls + 1) * 1000003ULL + (uint64_t)(tb.family + 1) * 1009ULL + (uint64_t)(keyed && !bass ? tb.kind + 1 : 0) * 13ULL + (bass ? 7ULL : 0ULL);
                tb.make = makeOf(key, cls, r);
                tb.name = T.makes[tb.make].name;
            }
        if (a == AGE_MODERN && E.band == 0 && !E.kit.empty()) {   // a drum machine: every piece its own pad
            const int mk = makeOf(0xD2ULL, IC_PADS, MR_BASS);
            for (Timbre& k : E.kit) { k.make = mk; k.name = T.makes[mk].name; }
        } else if (!(a == AGE_INDUSTRIAL && E.band == 3)) {   // a barrel organ's drum and cymbal are the barrel's
            for (size_t k = 0; k < E.kit.size(); k++) {
                Timbre& tb = E.kit[k];
                const int cls = instrumentClassOf(tb, k < E.kitRole.size() ? E.kitRole[k] : 0, a, true);
                if (cls < 0) continue;
                tb.make = makeOf(0x4B000ULL + (uint64_t)(cls + 1) * 31ULL + (uint64_t)tb.family, cls, MR_LEAD);
                tb.name = T.makes[tb.make].plural;
            }
        }
        if (a == AGE_INDUSTRIAL && keyUnit(O.seed, 8) < O.machine) T.machine = makeOf(0x3AC4ULL, IC_MACHINE, MR_LEAD);   // the cylinders' machine, where it was made
    }
}

// ---- the lists --------------------------------------------------------------------------------------------------------------------

void instrumentsOf(const Tradition& T, int people, PeopleInstruments& out) {
    out = PeopleInstruments();
    static const int ORDER[MR_COUNT] = {MR_LEAD, MR_SECOND, MR_ALT, MR_CHORDS, MR_BELL, MR_BOX, MR_KEYS, MR_SWARM, MR_SINGER, MR_CHORUS, MR_ORGAN, MR_DRONE, MR_BASS};
    for (int a = 0; a < AGE_COUNT; a++) {
        const Ensemble& E = T.ages[a];
        std::vector<Instrument>& L = out.byAge[a];
        auto have = [&](int make) { for (const Instrument& i : L) if (i.make == make) return true; return false; };
        auto add = [&](int voice, int kit, int make) {
            Instrument I; I.people = people; I.age = a; I.voice = voice; I.kit = kit; I.make = make; I.slot = (int)L.size();
            I.id = people * 1000 + a * 100 + I.slot;
            if (make >= 0 && make < (int)T.makes.size()) { I.reg = T.makes[make].reg; I.seed = T.makes[make].seed; I.pads = T.makes[make].cls == IC_PADS; I.disc = T.makes[make].cls == IC_MACHINE; }
            L.push_back(I);
        };
        for (int r : ORDER)
            for (size_t v = 0; v < E.voices.size() && v < E.role.size(); v++)
                if (E.role[v] == r && E.voices[v].make >= 0 && !have(E.voices[v].make)) add((int)v, -1, E.voices[v].make);
        for (size_t k = 0; k < E.kit.size(); k++)
            if (E.kit[k].make >= 0 && !have(E.kit[k].make)) add(-1, (int)k, E.kit[k].make);
        if (a == AGE_INDUSTRIAL && T.machine >= 0 && !have(T.machine)) add(-1, -1, T.machine);
    }
}

const Instrument* instrumentById(const PeopleInstruments& P, int id) {
    const int age = (id % 1000) / 100, slot = id % 100;
    if (age < 0 || age >= AGE_COUNT || slot < 0 || slot >= (int)P.byAge[age].size()) return nullptr;
    return &P.byAge[age][slot];
}

const InstrumentMake* instrumentMake(const Tradition& T, const Instrument& I) {
    return I.make >= 0 && I.make < (int)T.makes.size() ? &T.makes[I.make] : nullptr;
}
std::string instrumentName(const Tradition& T, const Instrument& I) {
    const InstrumentMake* M = instrumentMake(T, I);
    return M ? M->name : std::string("AN INSTRUMENT");
}

const Timbre* instrumentTimbre(const Tradition& T, const Instrument& I, int key) {
    if (I.age < 0 || I.age >= AGE_COUNT || I.disc) return nullptr;
    const Ensemble& E = T.ages[I.age];
    if (I.kit >= 0) {
        if (E.kit.empty()) return nullptr;
        const int k = I.pads ? ((key % (int)E.kit.size()) + (int)E.kit.size()) % (int)E.kit.size() : I.kit;
        return k < (int)E.kit.size() ? &E.kit[k] : nullptr;
    }
    if (I.voice < 0) return nullptr;
    return I.voice < (int)E.voices.size() ? &E.voices[I.voice] : nullptr;
}

double instrumentHz(const Tradition& T, const Instrument& I, int degree, int regShift) {
    const int n = std::max(1, (int)T.scale.size());
    const Timbre* tb = instrumentTimbre(T, I);
    if (tb && (tb->family == TF_RATTLE || tb->family == TF_JINGLE || tb->family == TF_CYMBAL))   // unpitched: a higher key brighter
        return std::max(60.0, tb->tone) * std::sqrt(traditionHz(T, degree + n * regShift) / traditionHz(T, 0));
    return traditionHz(T, degree + n * (I.reg + regShift));
}

void instrumentDiscOf(const Tradition& T, const Instrument& I, Piece& out) {
    Shard s; s.music = true; s.recordAge = I.age;
    s.seed = mix64(I.seed ^ 0x6A40F0DEULL);
    const double u = (double)(mix64(s.seed ^ 0x70E) >> 11) / 9007199254740992.0;
    s.tone = u < 0.7 ? ST_ORDINARY : (u < 0.9 ? ST_ELEGY : ST_WARNING);
    pieceOf(T, s, out);
}

bool instrumentWithRecord(uint64_t worldSeed, int people, int local, double& pick) {
    const uint64_t h = mix64(worldSeed ^ 0xC11A5ULL ^ ((uint64_t)(people + 1) * 0x9E3779B97F4A7C15ULL) ^ ((uint64_t)(local + 1) * 0xBF58476D1CE4E5B9ULL));
    pick = (double)((h >> 20) & 0xFFFFFF) / 16777216.0;
    return (double)(h & 0xFFFFF) / 1048576.0 < INSTRUMENT_SHARE;
}

int instrumentIdOf(const PeopleInstruments& P, int age, double pick) {
    if (age < 0 || age >= AGE_COUNT || P.byAge[age].empty()) return -1;
    const std::vector<Instrument>& L = P.byAge[age];
    return L[std::min((int)L.size() - 1, std::max(0, (int)(pick * L.size())))].id;
}

void instrumentParts(const Tradition& T, const Instrument& I, std::vector<InstrumentPart>& out) {
    out.clear();
    const InstrumentMake* M = instrumentMake(T, I);
    if (M) instrumentPartsOf(T, *M, out);
}

int instrumentCoursesOf(const std::vector<InstrumentPart>& parts) {
    int n = 0; for (const InstrumentPart& p : parts) n = std::max(n, p.course + 1);
    return std::max(1, n);
}
int instrumentCourses(const Tradition& T, const Instrument& I) {
    std::vector<InstrumentPart> parts; instrumentParts(T, I, parts);
    return instrumentCoursesOf(parts);
}

void instrumentBounds(const std::vector<InstrumentPart>& parts, double& u0, double& u1, double& v0, double& v1, double& top) {
    u0 = 1e9; u1 = -1e9; v0 = 1e9; v1 = -1e9; top = 0;
    auto grow = [&](double u, double v, double r) { u0 = std::min(u0, u - r); u1 = std::max(u1, u + r); v0 = std::min(v0, v - r); v1 = std::max(v1, v + r); };
    for (const InstrumentPart& p : parts) {
        const double c = std::cos(p.ang), s = std::sin(p.ang);
        switch (p.kind) {
            case IPK_TUBE:
            case IPK_STRING: {
                const double r = p.kind == IPK_TUBE ? std::max(p.r0, p.r1) : p.r0;
                grow(p.u, p.v, r);
                grow(p.u + c * p.len, p.v + s * p.len, r);
                top = std::max(top, p.kind == IPK_TUBE ? p.w0 + r : std::max(p.w0, p.w1));
                break;
            }
            case IPK_RING: {
                const double R = p.r0 + p.r1;
                if (p.standing) {
                    grow(p.u, p.v, p.r1);
                    grow(p.u - s * R, p.v + c * R, p.r1);
                    grow(p.u + s * R, p.v - c * R, p.r1);
                    top = std::max(top, p.w0 + R);
                } else {
                    grow(p.u, p.v, R);
                    top = std::max(top, p.w0 + 2 * p.r1);
                }
                break;
            }
            default: {
                const bool lengthwise = p.kind == IPK_PLATE || p.kind == IPK_BOX || p.kind == IPK_MARK;
                const double hl = lengthwise ? p.len * 0.5 : p.r1,
                             hw = p.r0 * (p.kind == IPK_PLATE ? 1 + 0.1 * std::abs(p.skew) + std::max(0.0, -p.waist) : 1.0) * (p.kind == IPK_DISC && p.open ? std::max(1.0, p.taper) : 1.0);
                const double hl2 = p.kind == IPK_DISC && p.open ? hl * std::max(1.0, p.taper) : hl;
                for (int k = 0; k < 4; k++) { const double a = (k & 1 ? 1 : -1) * hl2, b = (k & 2 ? 1 : -1) * hw; grow(p.u + c * a - s * b, p.v + s * a + c * b, 0); }
                top = std::max(top, p.w1 + (p.kind == IPK_PLATE ? std::max(0.0, p.dome) : 0.0));
                break;
            }
        }
    }
    if (parts.empty()) { u0 = u1 = v0 = v1 = 0; }
}
void instrumentExtent(const std::vector<InstrumentPart>& parts, double& lenU, double& lenV, double& top) {
    double u0, u1, v0, v1; instrumentBounds(parts, u0, u1, v0, v1, top);
    lenU = std::max(0.0, u1 - u0); lenV = std::max(0.0, v1 - v0);
}

// ---- its object in the ruins -------------------------------------------------------------------------------------------------------

Form instrumentPlateForm(const InstrumentPart& p) {
    Form f; f.kind = FK_PLATE; f.x = p.u; f.z = p.v; f.heading = p.ang;
    f.hx = std::max(1e-4, p.r0); f.hz = std::max(1e-4, p.len * 0.5); f.y0 = p.w0; f.y1 = std::max(p.w0 + 1e-4, p.w1);
    f.p[0] = p.taper; f.p[1] = p.waist; f.p[2] = p.square; f.p[3] = std::max(0.0, p.dome); f.sides = p.sides; f.sub = p.skew;
    return f;
}

void instrumentPartsToForms(const std::vector<InstrumentPart>& parts, uint64_t seed, double x, double z, double h, std::vector<Form>& out) {
    out.clear();
    const double ch = std::cos(h), sh = std::sin(h);
    // the instrument's (u along, v across) to the settlement's metres: along is (sin h, cos h), across (cos h, -sin h) (as C-11's)
    auto wx = [&](double u, double v) { return x + sh * u + ch * v; };
    auto wz = [&](double u, double v) { return z + ch * u - sh * v; };
    const uint32_t sd = (uint32_t)(seed ^ (seed >> 32));
    for (const InstrumentPart& p : parts) {
        Form f; f.seed = sd;
        instrumentMaterialPart(p.mat, f.part, f.slot);
        const double c = std::cos(p.ang), s = std::sin(p.ang);
        switch (p.kind) {
            case IPK_TUBE: {
                const double R = std::max(p.r0, p.r1), y0 = std::max(0.0, p.w0 - R);
                const double su = p.u - c * y0, sv = p.v - s * y0;   // the lathe's profile starts y0 along its axis from its foot
                f.kind = FK_LATHE; f.sub = std::fabs(p.r1 - p.r0) < 1e-5 ? 0 : 1; f.x = wx(su, sv); f.z = wz(su, sv); f.heading = h + p.ang; f.tilt = PI / 2;
                f.hx = f.hz = std::max(1e-4, p.r0); f.y0 = y0; f.y1 = y0 + std::max(1e-4, p.len); f.p[0] = p.r1 / std::max(1e-5, p.r0);
                if (p.open) f.p[3] = 1;
                break;
            }
            case IPK_PLATE: { Form q = instrumentPlateForm(p); q.seed = sd; q.part = f.part; q.slot = f.slot; q.x = wx(p.u, p.v); q.z = wz(p.u, p.v); q.heading = h + p.ang; f = q; break; }
            case IPK_DISC:
                f.kind = FK_LATHE; f.sub = p.open ? 1 : 0; f.x = wx(p.u, p.v); f.z = wz(p.u, p.v); f.heading = h + p.ang;
                f.hx = std::max(1e-4, p.r0); f.hz = std::max(1e-4, p.r1); f.y0 = p.w0; f.y1 = std::max(p.w0 + 1e-4, p.w1); f.sides = p.sides;
                if (p.open) { f.p[0] = p.taper > 0 ? p.taper : 1.2; f.p[3] = 1; }
                break;
            case IPK_BALL:
                f.kind = FK_LATHE; f.sub = 4; f.x = wx(p.u, p.v); f.z = wz(p.u, p.v); f.heading = h + p.ang;
                f.hx = std::max(1e-4, p.r0); f.hz = std::max(1e-4, p.r1); f.y0 = p.w0; f.y1 = std::max(p.w0 + 1e-4, p.w1); f.p[0] = 2;
                break;
            case IPK_BOX: case IPK_MARK: {
                const bool round = p.kind == IPK_MARK && (p.shape == 0 || p.shape == 3 || p.shape == 5 || p.shape == 6);
                const double y0 = p.w0, y1 = p.kind == IPK_MARK ? p.w0 + 0.0015 : std::max(p.w0 + 1e-4, p.w1);
                // a small round mark a hexagon (it is a few pixels)
                if (round) { f.kind = FK_LATHE; f.sub = 0; f.hx = f.hz = std::max(1e-4, p.r0); f.sides = p.shape == 3 ? 3 : (p.shape == 6 ? 5 : (p.r0 < 0.02 ? 6 : 0)); }
                else { f.kind = FK_BOX; f.hx = std::max(1e-4, p.r0); f.hz = std::max(1e-4, p.kind == IPK_MARK ? (p.shape == 1 ? p.len * 0.5 : (p.shape == 4 ? p.r0 * 1.6 : p.r0)) : p.len * 0.5); }
                f.x = wx(p.u, p.v); f.z = wz(p.u, p.v); f.heading = h + p.ang; f.y0 = y0; f.y1 = y1;
                break;
            }
            case IPK_RING:
                f.kind = FK_TORUS; f.x = wx(p.u, p.v); f.z = wz(p.u, p.v); f.hx = f.hz = std::max(1e-4, p.r0); f.p[0] = std::max(1e-4, p.r1);
                if (p.standing) { f.tilt = PI / 2; f.heading = h + p.ang; f.y0 = std::max(0.0, p.w0 - p.r0 - p.r1); } else f.y0 = p.w0;
                f.y1 = f.y0 + 2 * p.r1;
                break;
            case IPK_STRING: {
                const double eu = p.u + c * p.len, ev = p.v + s * p.len;
                f.kind = FK_CABLE; f.x = wx(p.u, p.v); f.z = wz(p.u, p.v); f.y0 = 0; f.y1 = p.w0; f.hx = f.hz = std::max(0.0005, p.r0);
                f.p[0] = wx(eu, ev); f.p[1] = wz(eu, ev); f.p[2] = p.w1; f.p[3] = 0;
                break;
            }
            default: continue;
        }
        out.push_back(f);
    }
}

void instrumentForms(const Instrument& I, const Tradition& T, double x, double z, double heading, std::vector<Form>& out) {
    std::vector<InstrumentPart> parts; instrumentParts(T, I, parts);
    instrumentPartsToForms(parts, I.seed, x, z, heading, out);
}
