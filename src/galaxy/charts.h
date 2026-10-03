#pragma once
// C-10 (2026-10-03): star charts as shards. "Some shards are a map, not a sound: the culture's own sky, drawn from their
// world with their constellations and names. Decoding it on the ship overlays their sky on yours, so you can find the star
// they named, and sometimes it points at another dead world. Cultures that knew each other become a chain to follow"
// (`docs/ideas/IDEAS-ruins-shards-signals.md`, section 3). Three to five of a world's fifty shards are charts
// (`shardIsChart`, `galaxy/shards.h`): each is drawn round one star the people marked (`Lore::marks`, `chartMarksOf`: a
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
#include "shards.h"

constexpr int CHART_RADIUS_SECTORS = 10;   // the sky's stars: the cube the game draws round a position (StarNeighborhood's radius)
constexpr int CHART_BRIGHT = 100;          // the brightest of them a chart may draw
constexpr int CHART_NEAR_CANDIDATES = 60;  // the brightest stars whose systems are looked at for a people's world
constexpr double CHART_FIELD = PI / 2;     // the chart's radius: a hemisphere round the marked star
constexpr double CHART_LINK = 30 * DEG;    // how far a figure reaches for its next star
constexpr double CHART_APART = 12 * DEG;   // the figures' seeds keep this far from every star already taken
constexpr int CHART_FIGURES_MIN = 10, CHART_FIGURES_VAR = 5;   // a people's figures over the whole sky: 10..14

struct ChartStar {
    Star star;              // without a name (the view resolves the explorer's name)
    Vec3 dir;               // unit, from the chart's viewpoint
    double bright = 0;      // luminosity over the squared distance in light years (the star field's own measure)
    double ang = 0;         // off the chart's centre, radians
    double x = 0, y = 0;    // on the chart: azimuthal equidistant, radius 1 at CHART_FIELD, y up
    bool mark = false;
};
struct ChartFigure {
    std::string name;       // ours, lowercase ("the hunter"); the decoder shows it in the people's words until learnt
    std::vector<int> stars; // indices into StarChart::stars, the seed (the brightest) first
    std::vector<std::pair<int, int>> lines;
    int label = 0;          // the star the name hangs at
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
};

// the stars the people marked, one per chart of the world (`chartShardsOf` of them), for `loreOf`
void chartMarksOf(const StarSystem& sys, const Body& b, const BodyGen& g, std::vector<ChartMark>& out);
// the sky of a star as a chart sees it: every star of the cube round it, the brightest first
void chartSkyOf(const Star& home, std::vector<ChartStar>& out);
// the people's figures over their whole sky (indices into the brightest CHART_BRIGHT of `sky`): each grown from a seed star
// (the brightest left that keeps CHART_APART from every star taken) by the nearest star left within CHART_LINK of any of its
// stars, three to six stars, joined to the star it was nearest to; one of one star is dropped; named from the world's draw
void skyFiguresOf(const BodyGen& g, const std::vector<ChartStar>& sky, std::vector<ChartFigure>& out);
void chartFigureNames(uint64_t worldSeed, int n, std::vector<std::string>& out);   // n distinct of the list, lowercase
std::string chartFigureList(const std::vector<std::string>& names);              // "the hunter, the boat and the two sisters"
// the chart of a world's shard (false unless the shard is a chart)
bool chartOf(const StarSystem& sys, const Body& b, const BodyGen& g, const Lore& L, int index, StarChart& out);
// a direction (unit, from the viewpoint) on the chart: x right, y up, radius 1 at CHART_FIELD; false beyond the field
bool chartProject(const StarChart& c, const Vec3& dir, double& x, double& y);
