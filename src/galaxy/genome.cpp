// A-04: the architecture genome (see genome.h).
#include "genome.h"
#include "peoples.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

const char* const TECHNIQUE_NAMES[TQ_COUNT] = {"POST AND LINTEL", "ARCH AND VAULT", "FRAME", "CANTILEVER", "TENSION", "SHELL", "PRINTED", "GROWN"};

namespace {
const char* const V_MASSING[] = {"COMPACT", "SPRAWLING", "VERTICAL", "BURIED", "STEPPED"};
const char* const V_PLAN[] = {"RECTANGLE", "CIRCLE", "POLYGON", "ELLIPSE", "FREE"};
const char* const V_ROOF[] = {"FLAT", "PITCHED", "HIPPED", "DOMED", "CONICAL", "VAULTED", "SADDLE", "STEPPED", "OPEN"};
const char* const V_PROFILE[] = {"UPRIGHT", "BATTERED", "BULGING", "LEANING OUT", "STEPPED BACK"};
const char* const V_SYMMETRY[] = {"BILATERAL", "RADIAL", "ROTATIONAL", "NONE"};
const char* const V_NUMBER[] = {"THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT", "NINE"};
const char* const V_MODULAR[] = {"ONE-OFFS", "CELLS"};
const char* const V_OPENING[] = {"SQUARE", "ARCHED", "ROUND", "SLIT", "TRIANGULAR", "KEYHOLE"};
const char* const V_RHYTHM[] = {"REGULAR", "PAIRED", "GROUPED", "SCATTERED"};
const char* const V_ORNAMENT[] = {"BARE", "BANDED", "CARVED", "ENCRUSTED"};
const char* const V_MOTIF[] = {"LINES", "CIRCLES", "STEPS", "SPIRALS", "GLYPH BANDS", "FIGURES", "STARS"};
const char* const V_EXPRESSION[] = {"HEAVY MASONRY", "EXPRESSED FRAME", "SMOOTH MONOLITH"};
const char* const V_TOPS[] = {"FINIALS", "CRENELS", "MASTS", "STATUES", "NOTHING"};
const char* const V_BASE[GENOME_BASES] = {"WHITE", "CREAM", "BUFF", "OCHRE", "YELLOW", "TERRACOTTA", "RED", "ROSE", "UMBER", "GREY", "SLATE", "CHARCOAL", "BLUE GREY", "SAGE", "PALE BLUE", "VIOLET"};
const char* const V_ACCENT[GENOME_ACCENTS * 4] = {
    "RED BANDS", "RED ROOFS", "RED DOORS", "RED TOPS", "GOLD BANDS", "GOLD ROOFS", "GOLD DOORS", "GOLD TOPS",
    "BLUE BANDS", "BLUE ROOFS", "BLUE DOORS", "BLUE TOPS", "GREEN BANDS", "GREEN ROOFS", "GREEN DOORS", "GREEN TOPS",
    "TURQUOISE BANDS", "TURQUOISE ROOFS", "TURQUOISE DOORS", "TURQUOISE TOPS", "WHITE BANDS", "WHITE ROOFS", "WHITE DOORS", "WHITE TOPS",
    "BLACK BANDS", "BLACK ROOFS", "BLACK DOORS", "BLACK TOPS"};
const char* const V_PAINTED[] = {"ITS OWN", "PAINTED"};
const char* const V_TINT[GENOME_TINTS] = {"CLEAR", "BRONZE", "GREEN", "BLUE", "GOLD", "SMOKE"};
const char* const V_LIGHT[] = {"NONE", "LAMPS", "LINES"};
const char* const V_LAND[] = {"RIDGES", "VALLEYS", "TERRACED", "CLIFFS", "STILTS", "MOUNDS", "PITS"};
const char* const V_ORIENT[] = {"SUNRISE", "A STAR", "THE MOON", "THE RIVER", "THE WIND", "NONE"};
const char* const V_STREETS[] = {"GRID", "RADIAL", "CONCENTRIC", "ORGANIC", "A LINE", "CONTOUR", "BRANCHING"};
const char* const V_PUBLIC[] = {"PLAZAS", "COURTS", "GARDENS", "NONE"};
const char* const V_GROUPING[] = {"COMPOUNDS", "ROWS", "DETACHED"};
const char* const V_EDGE[] = {"WALLS", "DITCHES", "NOTHING"};
const char* const V_LINKS[] = {"ARCADES", "BRIDGES", "TUNNELS", "COLONNADES", "NONE"};
const char* const V_FOCUS[] = {"SPIRE", "DOME", "GATE", "TOWER", "COLOSSUS", "MAST", "RING", "OBELISK", "POOL", "GREAT TREE"};
const char* const V_CENTRE[] = {"TEMPLE", "PALACE", "ASSEMBLY", "MARKET", "TOMB", "MACHINE"};
const char* const V_MONUMENTS[] = {"STELAE", "OBELISKS", "COLOSSI", "ARCHES", "PILLARS"};
const char* const V_DEAD[] = {"BARROWS", "TOMBS", "CEMETERIES", "TOWERS OF SILENCE", "NOTHING"};
// R-412
const char* const V_EAVES[] = {"FLUSH", "OVERHANGING", "DEEP", "FLARED"};
const char* const V_ROOFLINE[] = {"PLAIN", "DORMERS", "GABLETS", "CRESTED", "LANTERNS"};
const char* const V_GABLES[] = {"PLAIN", "STEPPED", "CURVED", "PEDIMENTED", "HALF-HIPPED"};
const char* const V_ROOFING[GENOME_ROOFINGS] = {"STRAW", "TERRACOTTA", "OCHRE", "BROWN", "SLATE", "CHARCOAL", "MOSS", "VERDIGRIS"};
const char* const V_POLYCHROMY[] = {"UNIFORM", "TWO-TONE", "VARIED", "STRIPED"};
const char* const V_TRIM[] = {"NONE", "LIGHT", "DARK", "SHUTTERS"};
const char* const V_PROJECTIONS[] = {"NONE", "BALCONIES", "GALLERIES", "BAYS", "ORIELS"};
const char* const V_ENTRANCE[] = {"PLAIN", "PORCHES", "STEPS", "PORTICOS", "CANOPIES"};
const char* const V_CORNERS[] = {"PLAIN", "QUOINS", "PILASTERS", "TURRETS"};
const char* const V_FRONTAGE[] = {"EAVES TO THE STREET", "GABLES TO THE STREET"};
const char* const V_BOUNDARY[] = {"OPEN", "WALLS", "FENCES", "HEDGES", "RAILINGS"};
const char* const V_FURNITURE[] = {"NONE", "WELLS", "LAMPS", "FOUNTAINS", "STATUES"};
const char* const V_POLES[] = {"NONE", "TOTEMS", "BANNERS", "SKULLS", "CAIRNS"};

constexpr unsigned P = 1, M = 2, I = 4, MO = 8, F = 16, FF = 32, ALL = 63;

const RGB BASES[GENOME_BASES] = {
    RGB(0.88f, 0.87f, 0.84f), RGB(0.86f, 0.80f, 0.66f), RGB(0.76f, 0.66f, 0.50f), RGB(0.78f, 0.58f, 0.30f),
    RGB(0.84f, 0.74f, 0.38f), RGB(0.72f, 0.42f, 0.28f), RGB(0.60f, 0.24f, 0.20f), RGB(0.78f, 0.58f, 0.54f),
    RGB(0.46f, 0.35f, 0.26f), RGB(0.60f, 0.60f, 0.58f), RGB(0.40f, 0.43f, 0.48f), RGB(0.22f, 0.22f, 0.24f),
    RGB(0.52f, 0.60f, 0.68f), RGB(0.56f, 0.63f, 0.50f), RGB(0.68f, 0.77f, 0.84f), RGB(0.52f, 0.46f, 0.62f)};
const RGB ACCENTS[GENOME_ACCENTS] = {RGB(0.72f, 0.20f, 0.16f), RGB(0.86f, 0.66f, 0.24f), RGB(0.20f, 0.34f, 0.70f), RGB(0.22f, 0.52f, 0.34f),
                                     RGB(0.18f, 0.62f, 0.62f), RGB(0.93f, 0.93f, 0.91f), RGB(0.10f, 0.10f, 0.11f)};
const RGB TINTS[GENOME_TINTS] = {RGB(0.62f, 0.70f, 0.74f), RGB(0.55f, 0.42f, 0.28f), RGB(0.40f, 0.60f, 0.50f), RGB(0.34f, 0.48f, 0.70f),
                                 RGB(0.80f, 0.66f, 0.34f), RGB(0.30f, 0.32f, 0.35f)};
}   // namespace

// name, group, categories (0 a measure), their names or the measure's ends, the ages it acts in, its weight among the strongest
const GenomeVarInfo GENOME_VARS[GV_COUNT] = {
    {"MASSING", "MASS", 5, V_MASSING, nullptr, nullptr, P | M | I | MO | F, 1.2},
    {"PROPORTION", "MASS", 0, nullptr, "SQUAT", "SLENDER", ALL, 1.0},
    {"AMBITION", "MASS", 0, nullptr, "MODEST", "COLOSSAL", ALL, 0},
    {"HEIGHT", "MASS", 0, nullptr, "LOW", "LOFTY", P | M | I | MO | F, 0.6},
    {"PLAN", "FORM", 5, V_PLAN, nullptr, nullptr, ALL, 1.2},
    {"ROOF", "FORM", 9, V_ROOF, nullptr, nullptr, P | M | I | MO | F, 1.5},
    {"PROFILE", "FORM", 5, V_PROFILE, nullptr, nullptr, P | M | I | MO | F, 0.8},
    {"SYMMETRY", "FORM", 4, V_SYMMETRY, nullptr, nullptr, M | I | MO | F, 0},
    {"NUMBER", "FORM", 7, V_NUMBER, nullptr, nullptr, ALL, 0},
    {"MODULES", "FORM", 2, V_MODULAR, nullptr, nullptr, M | I | MO, 0},
    {"OPENINGS", "SURFACE", 6, V_OPENING, nullptr, nullptr, M | I | MO | F, 0.6},
    {"RHYTHM", "SURFACE", 4, V_RHYTHM, nullptr, nullptr, M | I | MO | F, 0},
    {"GLAZING", "SURFACE", 0, nullptr, "FEW", "WIDE", M | I | MO | F, 0},
    {"ORNAMENT", "SURFACE", 4, V_ORNAMENT, nullptr, nullptr, M | I | MO | F, 0.4},
    {"MOTIF", "SURFACE", 7, V_MOTIF, nullptr, nullptr, M | I | MO | F, 0},
    {"STRUCTURE", "SURFACE", 3, V_EXPRESSION, nullptr, nullptr, M | I | MO | F, 0.6},
    {"TOPS", "SURFACE", 5, V_TOPS, nullptr, nullptr, M | I | MO | F, 0.5},
    {"BASE", "COLOUR", GENOME_BASES, V_BASE, nullptr, nullptr, ALL, 2.0},
    {"ACCENT", "COLOUR", GENOME_ACCENTS * 4, V_ACCENT, nullptr, nullptr, ALL, 1.0},
    {"FINISH", "COLOUR", 2, V_PAINTED, nullptr, nullptr, P | M | MO, 0},   // B-415: an industrial people built in brick, painted or not
    {"TINT", "COLOUR", GENOME_TINTS, V_TINT, nullptr, nullptr, F, 0},   // B-415: a modern ruin's glass gone within years, its metal roofs with B-415's flat ones
    {"LIGHT", "COLOUR", 3, V_LIGHT, nullptr, nullptr, FF, 0},
    {"LAND", "PLACE", 7, V_LAND, nullptr, nullptr, P | M | I | MO, 0},
    {"FACING", "PLACE", 6, V_ORIENT, nullptr, nullptr, ALL, 0},
    {"STREETS", "PLACE", 7, V_STREETS, nullptr, nullptr, M | I | MO, 0.5},
    {"PUBLIC", "PLACE", 4, V_PUBLIC, nullptr, nullptr, M | I | MO, 0},
    {"GROUPING", "PLACE", 3, V_GROUPING, nullptr, nullptr, P | M | I | MO, 0},
    {"EDGE", "PLACE", 3, V_EDGE, nullptr, nullptr, P | M, 0},   // T-02: an industrial town is not walled (its medieval core keeps its wall)
    {"LINKS", "PLACE", 5, V_LINKS, nullptr, nullptr, M | I | MO | F, 0},
    {"FOCUS", "MEANING", 10, V_FOCUS, nullptr, nullptr, ALL, 1.0},
    {"CENTRE", "MEANING", 6, V_CENTRE, nullptr, nullptr, P | M | I | MO | F, 0.5},
    {"MONUMENTS", "MEANING", 5, V_MONUMENTS, nullptr, nullptr, P | M | I | MO, 0},
    {"THE DEAD", "MEANING", 5, V_DEAD, nullptr, nullptr, P | M | I | MO | F, 0},
    {"HOUSES", "MEANING", 0, nullptr, "ALIKE", "VARIED", P | M | I | MO | F, 0},
    {"BUILT", "MEANING", 0, nullptr, "LIGHT", "TO LAST", P | M | I | MO, 0},
    // R-412: the people's own within its age's ways
    {"PITCH", "ROOFS", 0, nullptr, "SHALLOW", "STEEP", P | M | I, 0.6},
    {"EAVES", "ROOFS", 4, V_EAVES, nullptr, nullptr, P | M | I | MO, 0.5},
    {"ROOFLINE", "ROOFS", 5, V_ROOFLINE, nullptr, nullptr, M | I | MO, 0.4},
    {"GABLES", "ROOFS", 5, V_GABLES, nullptr, nullptr, M | I, 0.3},
    {"ROOFING", "ROOFS", GENOME_ROOFINGS, V_ROOFING, nullptr, nullptr, P | M | I, 1.0},
    {"POLYCHROMY", "WALLS", 4, V_POLYCHROMY, nullptr, nullptr, M | I | MO | F, 0.6},
    {"TRIM", "WALLS", 4, V_TRIM, nullptr, nullptr, M | I | MO, 0.3},
    {"PROJECTIONS", "WALLS", 5, V_PROJECTIONS, nullptr, nullptr, M | I | MO | F, 0.6},
    {"ENTRANCES", "WALLS", 5, V_ENTRANCE, nullptr, nullptr, P | M | I | MO | F, 0.3},
    {"CORNERS", "WALLS", 4, V_CORNERS, nullptr, nullptr, M | I | MO, 0.4},
    {"STOREYS", "TOWN", 0, nullptr, "LOW", "TALL", M | I | MO | F, 0.6},
    {"FRONTAGE", "TOWN", 2, V_FRONTAGE, nullptr, nullptr, M | I, 0.4},
    {"SETBACK", "TOWN", 0, nullptr, "AT THE STREET", "IN GARDENS", M | I | MO, 0},
    {"BOUNDARIES", "TOWN", 5, V_BOUNDARY, nullptr, nullptr, P | M | I | MO, 0.4},
    {"FURNITURE", "TOWN", 5, V_FURNITURE, nullptr, nullptr, M | I | MO | F, 0},
    {"POLES", "TOWN", 5, V_POLES, nullptr, nullptr, P, 0},
};

int Genome::of(int var) const {
    const int n = GENOME_VARS[var].n;
    return n <= 0 ? 0 : std::min(n - 1, (int)(v[var] * n));
}

void Genome::set(int var, int category) {
    const int n = GENOME_VARS[var].n;
    v[var] = n <= 0 ? clampd(category / 2.0, 0, 0.999) : (std::min(std::max(category, 0), n - 1) + 0.5) / n;   // a measure: 0 low, 1 middle, 2 high
}

Genome genomeOf(uint64_t peopleSeed) {
    Genome G;
    for (int i = 0; i < GV_COUNT; i++) G.v[i] = unitFromHash(mix64(peopleSeed ^ (0xA04A0000ULL + (uint64_t)(i + 1) * 0x9E3779B97F4A7C15ULL)));
    return G;
}

std::string genomeValue(const Genome& G, int var) {
    const GenomeVarInfo& V = GENOME_VARS[var];
    if (V.n > 0) return V.values[G.of(var)];
    const double x = G.at(var);
    char buf[48]; snprintf(buf, sizeof buf, "%s %.2f", x < 0.5 ? V.lo : V.hi, x);
    return buf;
}

RGB genomeBase(const Genome& G) { return BASES[G.of(GV_BASE)]; }
RGB genomeAccent(const Genome& G) { return ACCENTS[std::min(GENOME_ACCENTS - 1, G.accentColour())]; }
RGB genomeTint(const Genome& G) { return TINTS[G.of(GV_TINT)]; }
// R-412: a roofing's colour as the age made it: a prehistoric people's thatch and turf (straw, a reddish reed, a dark sedge, a grey, the
// smoke's black, a mossed one); a medieval people's tiles, slates, shingles and thatch; an industrial people's slates and tiles and their
// coppered and tarred sheets
RGB genomeRoofing(const Genome& G, int age) {
    static const RGB PRE[GENOME_ROOFINGS] = {RGB(0.62f, 0.53f, 0.33f), RGB(0.55f, 0.40f, 0.27f), RGB(0.60f, 0.50f, 0.30f), RGB(0.45f, 0.37f, 0.26f),
                                             RGB(0.48f, 0.47f, 0.42f), RGB(0.25f, 0.23f, 0.20f), RGB(0.40f, 0.44f, 0.28f), RGB(0.52f, 0.50f, 0.38f)};
    static const RGB MED[GENOME_ROOFINGS] = {RGB(0.66f, 0.56f, 0.34f), RGB(0.66f, 0.33f, 0.22f), RGB(0.70f, 0.50f, 0.26f), RGB(0.44f, 0.33f, 0.24f),
                                             RGB(0.34f, 0.36f, 0.41f), RGB(0.21f, 0.21f, 0.23f), RGB(0.38f, 0.43f, 0.30f), RGB(0.34f, 0.55f, 0.48f)};
    static const RGB IND[GENOME_ROOFINGS] = {RGB(0.56f, 0.46f, 0.36f), RGB(0.62f, 0.30f, 0.21f), RGB(0.64f, 0.46f, 0.28f), RGB(0.38f, 0.30f, 0.25f),
                                             RGB(0.30f, 0.33f, 0.39f), RGB(0.18f, 0.18f, 0.20f), RGB(0.33f, 0.38f, 0.30f), RGB(0.30f, 0.52f, 0.46f)};
    const int k = G.of(GV_ROOFING);
    return age <= AGE_PREHISTORIC ? PRE[k] : (age == AGE_MEDIEVAL ? MED[k] : IND[k]);
}
RGB genomeLight(const Genome& G) {
    if (G.of(GV_LIGHT) == LT_LAMPS) return RGB(1.0f, 0.78f, 0.45f);
    RGB a = genomeAccent(G);   // lines: the accent's hue at a glow's brightness (white and black ones a cold white)
    const float mx = std::max(a.r, std::max(a.g, a.b)), mn = std::min(a.r, std::min(a.g, a.b));
    if (mx - mn < 0.1f) return RGB(0.72f, 0.86f, 1.0f);
    return lerp(a * (1.0f / mx), RGB(1, 1, 1), 0.25f);
}

namespace {
// a crude distance between two colours as the eye weighs them (green most, blue least), 0 the same .. 1 far apart
double colourDistance(const RGB& a, const RGB& b) {
    const double dr = a.r - b.r, dg = a.g - b.g, db = a.b - b.b;
    return std::min(1.0, std::sqrt(2 * dr * dr + 4 * dg * dg + 3 * db * db) / 0.55);
}
}   // namespace

double genomeDistance(const Genome& a, const Genome& b, std::string* shared) {
    double d = 0;
    if (shared) shared->clear();
    for (int i = 0; i < GV_COUNT; i++) {
        const GenomeVarInfo& V = GENOME_VARS[i];
        if (V.weight <= 0) continue;
        double x;
        if (i == GV_BASE) x = colourDistance(genomeBase(a), genomeBase(b));
        else if (i == GV_ACCENT) x = 0.6 * colourDistance(genomeAccent(a), genomeAccent(b)) + 0.4 * (a.accentPlace() != b.accentPlace());
        else if (V.n <= 0) x = std::min(1.0, std::fabs(a.at(i) - b.at(i)) / 0.35);   // a third of the range apart reads as another people's
        else x = a.of(i) != b.of(i) ? 1.0 : 0.0;
        d += V.weight * x;
        if (shared && x < 0.35) { if (!shared->empty()) *shared += ", "; *shared += V.name; *shared += " "; *shared += genomeValue(a, i); }
    }
    return d;
}

double genomeMaxDistance() { double s = 0; for (int i = 0; i < GV_COUNT; i++) s += GENOME_VARS[i].weight; return s; }

int techniqueOf(const Genome& G, int age, uint64_t peopleSeed) {
    double w[TQ_COUNT] = {0};
    const int roof = G.of(GV_ROOF), ex = G.of(GV_EXPRESSION), op = G.of(GV_OPENING), plan = G.of(GV_PLAN), mass = G.of(GV_MASSING), focus = G.of(GV_FOCUS);
    const bool curved = roof == RF_DOMED || roof == RF_VAULTED || roof == RF_SADDLE;
    switch (age) {
        case AGE_PREHISTORIC: return TQ_POST_LINTEL;
        case AGE_MEDIEVAL:
            w[TQ_POST_LINTEL] = 1.0 + (roof == RF_FLAT || roof == RF_PITCHED || roof == RF_OPEN ? 0.8 : 0) + (op == OP_SQUARE || op == OP_SLIT ? 0.6 : 0);
            w[TQ_ARCH_VAULT] = 1.0 + (curved || roof == RF_CONICAL ? 1.2 : 0) + (op == OP_ARCHED || op == OP_ROUND || op == OP_KEYHOLE ? 0.8 : 0);
            break;
        case AGE_INDUSTRIAL:
            w[TQ_ARCH_VAULT] = 1.0 + (curved ? 1.0 : 0) + (op == OP_ARCHED || op == OP_ROUND ? 0.6 : 0) + (ex == EX_MASSIVE ? 0.6 : 0);
            w[TQ_FRAME] = 1.0 + (ex == EX_FRAME ? 1.4 : 0) + (G.at(GV_SHARE) > 0.6 ? 0.6 : 0);
            break;
        case AGE_MODERN:
            w[TQ_FRAME] = 1.0 + (ex == EX_FRAME ? 1.0 : 0) + (plan == PF_RECT ? 0.4 : 0);
            w[TQ_CANTILEVER] = 0.8 + (mass == MS_STEPPED || mass == MS_VERTICAL ? 1.0 : 0) + (plan == PF_FREE ? 0.5 : 0);
            w[TQ_TENSION] = 0.7 + (focus == FO_MAST || focus == FO_RING || focus == FO_GATE ? 1.2 : 0) + (G.of(GV_TOPS) == TP_MASTS ? 0.6 : 0);
            w[TQ_SHELL] = 0.7 + (curved ? 1.4 : 0) + (ex == EX_MONOLITH ? 0.6 : 0);
            break;
        case AGE_FUTURISTIC:
            w[TQ_SHELL] = 1.0 + (curved || focus == FO_DOME ? 1.2 : 0);
            w[TQ_TENSION] = 0.8 + (focus == FO_MAST || focus == FO_RING || focus == FO_GATE ? 1.2 : 0);
            w[TQ_PRINTED] = 0.9 + (mass == MS_STEPPED || ex == EX_MASSIVE ? 1.0 : 0) + (roof == RF_STEPPED ? 0.6 : 0);
            w[TQ_GROWN] = 0.8 + (plan == PF_FREE ? 1.2 : 0) + (G.of(GV_MOTIF) == MO_SPIRALS || G.of(GV_MOTIF) == MO_FIGURES || focus == FO_TREE ? 0.8 : 0);
            break;
        default: return TQ_PRINTED;
    }
    double sum = 0; for (double x : w) sum += x;
    double u = unitFromHash(mix64(peopleSeed ^ (0xA04B0000ULL + (uint64_t)age * 0x9E3779B97F4A7C15ULL))) * sum;
    for (int t = 0; t < TQ_COUNT; t++) { if (w[t] <= 0) continue; if (u < w[t]) return t; u -= w[t]; }
    for (int t = TQ_COUNT - 1; t >= 0; t--) if (w[t] > 0) return t;
    return TQ_POST_LINTEL;
}
