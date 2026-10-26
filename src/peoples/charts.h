#pragma once
// C-10 (2026-10-03): star charts as shards. "Some shards are a map, not a sound: the culture's own sky, drawn from their
// world with their constellations and names. Decoding it on the ship overlays their sky on yours, so you can find the star
// they named, and sometimes it points at another dead world. Cultures that knew each other become a chain to follow"
// (`docs/archive/ideas/IDEAS-ruins-shards-signals.md`, section 3). Three to five of a world's fifty shards are charts
// (`shardIsChart`, `peoples/shards.h`): each is drawn round one star the people marked (`Lore::marks`, `chartMarksOf`: a
// people's world among the bright stars of their sky, the star a transmission they heard came from (C-07's transmitters
// within reach), or, when the charts outnumber the peoples they knew, the brightest star of their nights, the one the lore
// names) and shows the hemisphere of their sky round it: the brightest hundred stars within ten light years of their
// star (the cube the game draws from there, so every star of the chart is in the sky when the ship is at their star). The
// people's figures are one thing over their whole sky (`skyFiguresOf`: ten to fourteen, each a few bright stars joined, with
// names of their own from `chartFigureNames`), and a chart shows the ones that fit whole within its field, which the chart's
// caption lists, so the names are words of the world's language and resolve as it is learnt (C-06). The chart is a function of the galaxy as it
// is: the same stars, seen from the people's star; the decoder draws it (`game/shards_screen.cpp`) and holds it up to the
// sky (`Game::drawChartOverlay`: the figures' lines between the real stars from wherever the ship is, the marked star's
// diamond), and Enter on the chart targets the marked star, so a chain of peoples is flown link by link.
// H-05 (2026-10-10): each chart is drawn in the way of its record's age and medium (`chartStyleOf`: ideas note, section 10): a sky
// painted on a cave's ceiling or carved on a bone with the figures as animals and the moon's days counted beside them; a vault painted
// blue with gold stars, an astrolabe's plate or a codex's page with the figures as creatures and heroes and the wanderers' paths; an
// engraved atlas with the grid of their coordinates and the stars numbered, or a photograph on glass; a survey by magnitudes with the
// peoples heard by radio marked; a navigation chart of the routes to the worlds they reached or meant to (a map of their neighbourhood,
// not their sky); the far future's chart of things the explorer cannot find, or finds much later (a mark thousands of light years off,
// places where nothing is). The figures stay one set over the people's sky (C-10); their names are the people's age's (`skyFiguresOf`).
#include "lore.h"
#include "peoples.h"
#include <array>

constexpr int CHART_RADIUS_SECTORS = 10;   // the sky's stars: the cube the game draws round a position (StarNeighborhood's radius)
constexpr int CHART_BRIGHT = 100;          // the brightest of them a chart may draw
constexpr int CHART_NEAR_CANDIDATES = 60;  // the brightest stars whose systems are looked at for a people's world
constexpr double CHART_FIELD = PI / 2;     // the chart's radius: a hemisphere round the marked star
constexpr double CHART_LINK = 30 * DEG;    // how far a figure reaches for its next star
constexpr double CHART_APART = 12 * DEG;   // the figures' seeds keep this far from every star already taken
constexpr int CHART_FIGURES_MIN = 10, CHART_FIGURES_VAR = 5;   // a people's figures over the whole sky: 10..14

// H-05: the ways a chart was made (by the record's age and medium)
enum ChartStyle {
    CHS_CEILING = 0,   // prehistoric, a painted wall: ochre and soot on a cave's ceiling, the figures as animals, the moon's days counted
    CHS_BONE,          // prehistoric, a carved bone: the brightest stars scratched on a bone, the animals, the moon's days notched on its edge
    CHS_VAULT,         // medieval, a painted wall: a vault painted blue with gold stars, the figures as creatures and heroes
    CHS_ASTROLABE,     // medieval, an engraved plate: an astrolabe's plate: the graduated limb, their equator, tropics and ecliptic, the rete's pointers
    CHS_PAGE,          // medieval, a codex: a page in ink, the figures as creatures and heroes, the wanderers' paths in red along the ecliptic
    CHS_ATLAS,         // industrial, printed pages: an engraved atlas: the grid of their coordinates, the stars numbered by brightness
    CHS_PLATE,         // industrial, a glass plate: a photograph of the sky (a negative), the plate's reseau, the mark circled in ink
    CHS_SURVEY,        // modern (glass, printed pages, a drive): a survey by magnitudes, the peoples heard by radio marked
    CHS_ROUTES,        // futuristic: a navigation chart: a map of their neighbourhood, the routes to the worlds they reached or meant to
    CHS_BEYOND,        // far future: a mark thousands of light years off and places where nothing is
    CHS_SUNMEASURE,    // D-05: a drifter's people's measure of their sun going, its disc year by year in its age's way (suns painted smaller and smaller on a
                       // cave's ceiling, circles drawn on a page or engraved, a printed graph, a light curve on a screen): its mark the star that was their sun
    CHS_COUNT
};
extern const char* const CHART_STYLE_NAMES[CHS_COUNT];   // "a cave's ceiling", "a carved bone", ... (the harness)
extern const char* const CHART_STYLE_KEYS[CHS_COUNT];    // "ceiling", "bone", ... (the caption's `chartstyle`)
int chartStyleOf(int age, int medium);                  // the style of a chart of that age on that medium (RM_*)
std::string chartStyleLine(int style, int medium);      // the decoder's: "ON A CAVE'S CEILING", "A SURVEY ON GLASS", "A NAVIGATION CHART"
constexpr int CHART_GLYPH_W = 7, CHART_GLYPH_H = 6;
constexpr int CHART_GLYPHS = 24;                        // 0..11 the animals, 12..23 the creatures and heroes
extern const char* const CHART_GLYPH_ROWS[CHART_GLYPHS][CHART_GLYPH_H];   // '#' a mark, '.' none
constexpr double CHART_MAP_LY = 12;                     // CHS_ROUTES: the map's radius
enum ChartCurveKind { CC_EQUATOR = 0, CC_TROPIC, CC_ECLIPTIC, CC_DECLINATION, CC_HOUR, CC_RING };
struct ChartCurve {
    int kind = CC_EQUATOR;
    double value = 0;                                   // a declination's or an hour circle's, in the people's own units (`StarChart::circle`, `hours`)
    std::vector<std::array<float, 2>> pts;              // on the chart's x y (radius 1 the field), one run within the field
};
struct ChartPath { int body = -1; double size = 1; std::vector<std::array<float, 2>> pts; };   // a wanderer's way across the field (its size: how bright it was)

struct ChartStar {
    Star star;              // without a name (the view resolves the explorer's name); CHS_BEYOND's places where nothing is: no star (`sign`), the position only
    Vec3 dir;               // unit, from the chart's viewpoint
    double bright = 0;      // luminosity over the squared distance in light years (the star field's own measure)
    double ang = 0;         // off the chart's centre, radians
    double x = 0, y = 0;    // on the chart: azimuthal equidistant, radius 1 at CHART_FIELD, y up (CHS_ROUTES: the map's, radius 1 at CHART_MAP_LY)
    bool mark = false;
    // H-05
    int number = 0;         // CHS_ATLAS: its number in their catalogue (1 the brightest), 0 none
    double mag = 0;         // CHS_SURVEY: its magnitude in their steps (0 the brightest)
    bool heard = false;     // CHS_SURVEY: a people they heard by radio
    bool sign = false;      // CHS_BEYOND: a place where nothing is (no star)
    bool home = false;      // CHS_ROUTES: their own star (the map's centre)
    double rise = 0;        // CHS_ROUTES: its height over their plane on the map (radius units)
    double ly = 0;          // from the viewpoint
};
struct ChartFigure {
    std::string name;       // ours, lowercase ("the hunter"); the decoder shows it in the people's words until learnt
    std::vector<int> stars; // indices into StarChart::stars, the seed (the brightest) first
    std::vector<std::pair<int, int>> lines;
    int label = 0;          // the star the name hangs at
    // H-05
    int glyph = -1;         // the animal or the creature drawn with it (CHART_GLYPH_ROWS), -1 none
    int kind = 0;           // 0 a figure of the sky, 1 a route (CHS_ROUTES), 2 a line to a place where nothing is (CHS_BEYOND)
    bool planned = false;   // a route meant and not flown
};
struct StarChart {
    bool valid = false;
    uint64_t seed = 0;
    int index = -1;         // the shard's index among the fifty
    int which = -1;         // which of the world's charts (its mark's index in `Lore::marks`)
    Vec3 from;              // the viewpoint: the world's star, km
    Vec3 centre, up, right; // the chart's frame: the centre the marked star's direction, up toward the galaxy's north
    ChartMark mark;
    int markStar = -1;      // the marked star among `stars`
    std::vector<ChartStar> stars;
    std::vector<ChartFigure> figures;
    // H-05: the way it was made and what that way shows
    int age = AGE_MEDIEVAL, medium = 0, style = CHS_PAGE;
    int tally = 0, tallyKind = 0;       // the moon's count beside the figures: 1 the days of its month, 2 its moons in the year (0 none: no moon to count)
    std::vector<ChartCurve> curves;     // their equator, tropics and ecliptic (an astrolabe), their grid (an atlas), the far future's rings
    std::vector<ChartPath> wanderers;   // the system's other planets along the ecliptic (an astrolabe, a page)
    int circle = 360, hours = 24;       // their coordinates: a circle's degrees, a day's hours (the atlas's grid and its numbers)
    int plate = 0;                      // the atlas's plate, the photograph's number, the survey's field
    double magStep = 2.5;               // the survey's: the brightness one of their magnitudes is (2 to 4 times)
    int magLimit = 0;                   // the survey's faintest magnitude
    int heardN = 0;                     // the peoples heard by radio on it
    int reached = 0, planned = 0;       // CHS_ROUTES: the routes flown and the ones only meant
    double farLy = 0; int farKind = 0;  // CHS_BEYOND: the mark's distance; 1 toward the galaxy's heart, 2 a cluster, 3 out of the disc, 4 along the disc
    int signs = 0;                      // CHS_BEYOND: the places where nothing is
    // D-05: the sun's measure (CHS_SUNMEASURE): the way of the chart it is drawn as (its age's and medium's ordinary one: the cave's ceiling, the
    // page, the atlas's paper, the survey's screen), the disc year by year from the onset to the chart's year (its year, the disc in the people's
    // own parts) and those parts (the disc at the first measure: 60, 100, 120, 32 or 24 by the people, as their observatory's table counts)
    int baseStyle = -1;
    std::vector<std::array<float, 2>> sunCurve;
    int sunUnits = 0;
};

// the stars the people marked, one per chart of the world (`chartShardsOf` of them), for `loreOf`; `g` is the people's generator.
// A-01: a people heard a transmission only if it had radio itself and the star's people had it too (`peoplesOnAir`)
// H-05: a far future's chart marks a star thousands of light years off (`ChartMark::how` 3: toward the galaxy's heart, a cluster, out of
// the disc or along it), found once a people and kept; the rest as C-10's. D-05: a drifter's people's charts of its years without the sun (its
// dimming and its going: the sun's measure; the dark after the last sunrise: the sky from where the drifter was by then) mark the star that was
// their sun (`how` 4), which Enter flies to
void chartMarksOf(const StarSystem& sys, const Body& b, const BodyGen& g, std::vector<ChartMark>& out, int count = -1);   // T-04: `count` the people's charts (`recordChartCount`), -1 C-10's
int chartFarKind(const BodyGen& g, const Star& home, int k);   // H-05: the k-th far mark's kind (1 the heart, 2 a cluster, 3 out of the disc, 4 along it), 0 none
// the sky of a star as a chart sees it: every star of the cube round it, the brightest first
void chartSkyOf(const Star& home, std::vector<ChartStar>& out);
// the people's figures over their whole sky (indices into the brightest CHART_BRIGHT of `sky`): each grown from a seed star
// (the brightest left that keeps CHART_APART from every star taken) by the nearest star left within CHART_LINK of any of its
// stars, three to six stars, joined to the star it was nearest to; one of one star is dropped; named from the world's draw
void skyFiguresOf(const BodyGen& g, const std::vector<ChartStar>& sky, std::vector<ChartFigure>& out);
void chartFigureNames(uint64_t worldSeed, int n, std::vector<std::string>& out, std::vector<int>* glyphs = nullptr, bool animals = false);   // n distinct names, lowercase (H-05: animals for a people of the oldest age, else creatures and heroes, each with its glyph)
std::string chartFigureList(const std::vector<std::string>& names);              // "the hunter, the boat and the two sisters"
// the chart of a world's shard (false unless the shard is a chart). H-05: drawn in its record's way (`StarChart::style`): the sky's charts
// hold what their way shows (the moon's count, their equator, tropics and ecliptic, the wanderers' paths, their grid and numbers, the
// magnitudes, the peoples heard), a navigation chart the map of their neighbourhood and its routes (the figures, `kind` 1), the far
// future's its far mark and the places where nothing is (`sign` stars joined to the mark, `kind` 2)
bool chartOf(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L, int index, StarChart& out);
// a direction (unit, from the viewpoint) on the chart: x right, y up, radius 1 at CHART_FIELD; false beyond the field
bool chartProject(const StarChart& c, const Vec3& dir, double& x, double& y);
