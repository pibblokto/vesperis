// A-04 (2026-10-09): the architecture genome (`docs/archive/ideas/IDEAS-civilisation-expansion.md`, section 3). Every people draws
// thirty-five deep variables (R-412: fifty-one) from its seed and keeps them through its ages, so a people that loved domes builds them in dry
// stone, in masonry, in iron and glass, in concrete and in diamond glass, and its keep and its towers are one people's; the
// age's own variables (`techniqueOf`, the materials and heights of `peoples/buildings.*`) are what its age could do. Each
// variable is a number in [0, 1): a category is its share of the range (`Genome::of`), a measure the number itself. The
// generators read the genome through the culture (`Culture::genome` and the ways drawn from it, peoples/ruins.h); every
// variable acts at a scale the picture shows (a silhouette, a proportion, a rhythm, a colour), and the uniqueness is measured,
// not hoped for (`genomeDistance`: `vesperis_test genome distinct`).
#pragma once
#include "core/types.h"
#include <cstdint>
#include <string>

enum GenomeVar {
    // mass and proportion
    GV_MASSING = 0, GV_PROPORTION, GV_AMBITION, GV_HEIGHT,
    // form
    GV_PLAN, GV_ROOF, GV_PROFILE, GV_SYMMETRY, GV_NUMBER, GV_MODULAR,
    // surface
    GV_OPENING, GV_RHYTHM, GV_SHARE, GV_ORNAMENT, GV_MOTIF, GV_EXPRESSION, GV_TOPS,
    // colour and light
    GV_BASE, GV_ACCENT, GV_PAINTED, GV_TINT, GV_LIGHT,
    // place
    GV_LAND, GV_ORIENT, GV_STREETS, GV_PUBLIC, GV_GROUPING, GV_EDGE, GV_LINKS,
    // meaning
    GV_FOCUS, GV_CENTRE, GV_MONUMENTS, GV_DEAD, GV_SAMENESS, GV_LASTING,
    // the people's own within its age's ways, every one read through the age (the age speaks first: B-415). Its roofs: their pitch,
    // their eaves, what rises from them, their gables' ends, their colour
    GV_PITCH, GV_EAVES, GV_ROOFLINE, GV_GABLES, GV_ROOFING,
    // its walls: their colours from house to house, the windows' surrounds, what stands out from the fronts, the doorways, the corners
    GV_POLYCHROMY, GV_TRIM, GV_PROJECTIONS, GV_ENTRANCE, GV_CORNERS,
    // its towns: the dwellings' storeys, which end of a house the street sees, how far back they stand, what bounds a yard, what stands in its
    // streets and squares, a prehistoric people's poles
    GV_STOREYS, GV_FRONTAGE, GV_SETBACK, GV_BOUNDARY, GV_FURNITURE, GV_POLES,
    GV_COUNT
};
static_assert(GV_COUNT == 51, "fifty-one deep variables (A-04's thirty-five, R-412's sixteen)");

enum Massing { MS_COMPACT = 0, MS_SPRAWLING, MS_VERTICAL, MS_BURIED, MS_STEPPED };
enum PlanForm { PF_RECT = 0, PF_CIRCLE, PF_POLYGON, PF_ELLIPSE, PF_FREE, PF_COURT };   // PF_COURT: a house round an inward court (`GV_PUBLIC`)
enum RoofForm { RF_FLAT = 0, RF_PITCHED, RF_HIPPED, RF_DOMED, RF_CONICAL, RF_VAULTED, RF_SADDLE, RF_STEPPED, RF_OPEN };
enum WallProfile { WP_UPRIGHT = 0, WP_BATTERED, WP_BULGING, WP_LEANING, WP_STEPPED };
enum Symmetry { SY_BILATERAL = 0, SY_RADIAL, SY_ROTATIONAL, SY_NONE };
enum Opening { OP_SQUARE = 0, OP_ARCHED, OP_ROUND, OP_SLIT, OP_TRIANGULAR, OP_KEYHOLE };
enum Rhythm { RH_REGULAR = 0, RH_PAIRED, RH_GROUPED, RH_SCATTERED };
enum Ornament { ON_BARE = 0, ON_BANDED, ON_CARVED, ON_ENCRUSTED };
enum Motif { MO_LINES = 0, MO_CIRCLES, MO_STEPS, MO_SPIRALS, MO_GLYPHS, MO_FIGURES, MO_STARS };
enum Expression { EX_MASSIVE = 0, EX_FRAME, EX_MONOLITH };
enum Tops { TP_FINIALS = 0, TP_CRENELS, TP_MASTS, TP_STATUES, TP_NOTHING };
enum AccentPlace { AP_BANDS = 0, AP_ROOFS, AP_DOORS, AP_TOPS };
enum LightKind { LT_NONE = 0, LT_LAMPS, LT_LINES };
enum Land { LD_RIDGES = 0, LD_VALLEYS, LD_TERRACED, LD_CLIFFS, LD_STILTS, LD_MOUNDS, LD_PITS };
enum Orient { OT_SUNRISE = 0, OT_STAR, OT_MOON, OT_RIVER, OT_WIND, OT_NONE };
enum Streets { ST_GRID = 0, ST_RADIAL, ST_CONCENTRIC, ST_ORGANIC, ST_LINE, ST_CONTOUR, ST_BRANCHING };
enum Public { PU_PLAZAS = 0, PU_COURTS, PU_GARDENS, PU_NONE };
enum Grouping { GR_COMPOUNDS = 0, GR_ROWS, GR_DETACHED };
enum Edge { ED_WALLS = 0, ED_DITCHES, ED_NOTHING };
enum Links { LK_ARCADES = 0, LK_BRIDGES, LK_TUNNELS, LK_COLONNADES, LK_NONE };
enum Focus { FO_SPIRE = 0, FO_DOME, FO_GATE, FO_TOWER, FO_COLOSSUS, FO_MAST, FO_RING, FO_OBELISK, FO_POOL, FO_TREE };
enum Centre { CE_TEMPLE = 0, CE_PALACE, CE_ASSEMBLY, CE_MARKET, CE_TOMB, CE_MACHINE };
enum Monuments { MN_STELAE = 0, MN_OBELISKS, MN_COLOSSI, MN_ARCHES, MN_PILLARS };
enum Dead { DE_BARROWS = 0, DE_TOMBS, DE_CEMETERIES, DE_TOWERS, DE_NOTHING };
enum Eaves { EV_FLUSH = 0, EV_OVERHANGING, EV_DEEP, EV_FLARED };                          // a flat roof's slab: flush, a lip, a deep slab, a cornice
enum Roofline { RL_PLAIN = 0, RL_DORMERS, RL_GABLETS, RL_CRESTED, RL_LANTERN };          // a flat roof's: bare, a pergola, tanks, rails, a lantern
enum Gables { GB_PLAIN = 0, GB_STEPPED, GB_CURVED, GB_PEDIMENT, GB_HALF_HIPPED };
constexpr int GENOME_ROOFINGS = 8;                                                        // a roofing's colour (the age's materials: thatch, tiles, slate, sheet)
enum Polychromy { PC_UNIFORM = 0, PC_TWO_TONE, PC_VARIED, PC_STRIPED };
enum Trim { TM_NONE = 0, TM_LIGHT, TM_DARK, TM_SHUTTERS };
enum Projections { PJ_NONE = 0, PJ_BALCONIES, PJ_GALLERIES, PJ_BAYS, PJ_ORIELS };
enum Entrance { EN_PLAIN = 0, EN_PORCH, EN_STEPS, EN_PORTICO, EN_CANOPY };
enum Corners { CO_PLAIN = 0, CO_QUOINS, CO_PILASTERS, CO_TURRETS };
enum Frontage { FR_EAVES = 0, FR_GABLES };
enum Boundary { BD_OPEN = 0, BD_WALLS, BD_FENCES, BD_HEDGES, BD_RAILINGS };
enum Furniture { FU_NONE = 0, FU_WELLS, FU_LAMPS, FU_FOUNTAINS, FU_STATUES };
enum Poles { PO_NONE = 0, PO_TOTEMS, PO_BANNERS, PO_SKULLS, PO_CAIRNS };
// the age's techniques: post and lintel, arch and vault, frame, cantilever, tension, shell, printed or grown (`techniqueOf`)
enum Technique { TQ_POST_LINTEL = 0, TQ_ARCH_VAULT, TQ_FRAME, TQ_CANTILEVER, TQ_TENSION, TQ_SHELL, TQ_PRINTED, TQ_GROWN, TQ_COUNT };
extern const char* const TECHNIQUE_NAMES[TQ_COUNT];

struct Genome {
    double v[GV_COUNT] = {0};
    int of(int var) const;                        // a category's index (0 for a measure)
    double at(int var) const { return v[var]; }   // the number (a measure: 0 the low end .. 1 the high)
    void set(int var, int category);              // the middle of a category's share (the harness and the unit)
    int number() const { return 3 + of(GV_NUMBER); }   // the sacred number, 3..9
    int accentColour() const { return of(GV_ACCENT) / 4; }
    int accentPlace() const { return of(GV_ACCENT) % 4; }
    bool operator==(const Genome& o) const { for (int i = 0; i < GV_COUNT; i++) if (v[i] != o.v[i]) return false; return true; }
};

// a variable's name and group, its categories' names (n 0: a measure, named by its two ends), the ages it acts in (a bit an age,
// the unit's reach check reads it), and its weight among the strongest (0: not one of them) in the distinctness measure
struct GenomeVarInfo { const char* name; const char* group; int n; const char* const* values; const char* lo; const char* hi; unsigned ages; double weight; };
extern const GenomeVarInfo GENOME_VARS[GV_COUNT];

// a people's genome from its seed (`peopleGen(g, people).seed`): one hash a variable, so a variable added later moves none of the others
Genome genomeOf(uint64_t peopleSeed);
std::string genomeValue(const Genome& G, int var);   // "STEPPED", "SLENDER 0.82", "GOLD ROOFS"

// the colours (A-04's tables): the structure's base, the accent, the glass's tint, the light
constexpr int GENOME_BASES = 16, GENOME_ACCENTS = 7, GENOME_TINTS = 6;
RGB genomeBase(const Genome& G);
RGB genomeAccent(const Genome& G);
RGB genomeTint(const Genome& G);
RGB genomeLight(const Genome& G);   // warm lamps, or lines in the accent's light
RGB genomeRoofing(const Genome& G, int age);   // R-412: its roofing's colour in the age's material (thatch, tiles, slate, sheet)

// the technique a people used in an age, drawn among the age's by the deep variables' leanings (a people of domes and vaults the
// arch and the shell, of frames the frame, of free plans the grown)
int techniqueOf(const Genome& G, int age, uint64_t peopleSeed);

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do

// how far two peoples' architecture is apart on the strongest variables (0 alike .. the weights' sum), and the variables they
// share as a line; two are alike under `GENOME_ALIKE`
double genomeDistance(const Genome& a, const Genome& b, std::string* shared = nullptr);
double genomeMaxDistance();
constexpr double GENOME_ALIKE = 4.9;   // a quarter of the strongest variables' weight (R-412: 19.8 with its own): two peoples sharing all of them but the weaker few
