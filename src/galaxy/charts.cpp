// C-10 (2026-10-03): star charts as shards (see charts.h).
#include "charts.h"
#include "signals.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <set>

// the sky of a star: every star of the cube the game draws round it, the brightest first (the star field's own measure:
// the luminosity over the squared distance in light years; ties by the seed, so no sector order shows through)
void chartSkyOf(const Star& home, std::vector<ChartStar>& out) {
    out.clear();
    for (int64_t x = home.sx - CHART_RADIUS_SECTORS; x <= home.sx + CHART_RADIUS_SECTORS; x++)
        for (int64_t y = home.sy - CHART_RADIUS_SECTORS; y <= home.sy + CHART_RADIUS_SECTORS; y++)
            for (int64_t z = home.sz - CHART_RADIUS_SECTORS; z <= home.sz + CHART_RADIUS_SECTORS; z++) {
                Star q; if (!starInSector(x, y, z, q, false) || q.seed == home.seed) continue;
                Vec3 d = q.pos - home.pos;
                double d2 = length2(d) / (SECTOR_KM * SECTOR_KM);
                if (d2 < 1e-6) continue;
                ChartStar cs; cs.star = q; cs.dir = normalize(d); cs.bright = q.luminosity / d2;
                out.push_back(cs);
            }
    std::sort(out.begin(), out.end(), [](const ChartStar& a, const ChartStar& b) { return a.bright != b.bright ? a.bright > b.bright : a.star.seed < b.star.seed; });
}

namespace {
// the first people's world of a star's system, with its people's name
bool peoplesWorldOf(const Star& s, int& body, std::string& people) {
    StarSystem sys; sys.generate(s);
    for (const Body& b : sys.bodies) {
        if (b.type != PT_FELISIAN && b.type != PT_DESERT) continue;
        BodyGen g = BodyGen::make(b);
        if (!worldHadCivilisation(g)) continue;
        body = b.index; people = peopleNameOf(g);
        return true;
    }
    return false;
}
const char* const FIGURES[24] = {"the hunter", "the boat", "the river", "the two sisters", "the plough", "the serpent", "the lamp", "the gate", "the fish", "the bird", "the tree", "the crown",
                                 "the wanderer", "the hearth", "the cup", "the ladder", "the eye", "the net", "the hand", "the mother", "the fire", "the well", "the bridge", "the dancer"};
}

// the stars a world's charts mark. A people's world is at one star in eight, so a chart that marks one is a choice, not a
// chance ("sometimes it points at another dead world"): the first chart marks the peoples it knew (the nearest it heard over
// the radio's reach, else the brightest near one among the stars of its sky), the second the brightest star of its nights
// (the one the lore names), the rest a people (heard and near in turns) or a bright star by the chart's own coin; no star twice
void chartMarksOf(const StarSystem& sys, const Body& b, const BodyGen& g, std::vector<ChartMark>& out) {
    out.clear();
    const Star& home = sys.star;
    std::vector<ChartStar> sky; chartSkyOf(home, sky);
    std::vector<ChartMark> near, heard;
    for (int k = 0; k < (int)sky.size() && k < CHART_NEAR_CANDIDATES; k++) {
        int body = -1; std::string people;
        if (!peoplesWorldOf(sky[k].star, body, people)) continue;
        ChartMark m; m.star = sky[k].star; m.body = body; m.people = people; m.ly = length(sky[k].star.pos - home.pos) / SECTOR_KM;
        m.how = sectorTransmits(m.star.sx, m.star.sy, m.star.sz) ? 2 : 1;   // a near people that transmits is known by its voice
        (m.how == 2 ? heard : near).push_back(m);
    }
    std::vector<Signal> far; signalsNear(home.pos, far);
    for (const Signal& s : far) {
        if (s.kind != SIG_PEOPLE || s.star.seed == home.seed) continue;
        bool have = false; for (const ChartMark& m : heard) if (m.star.seed == s.star.seed) have = true;
        if (have) continue;
        int body = -1; std::string people;
        if (!peoplesWorldOf(s.star, body, people)) continue;
        ChartMark m; m.star = s.star; m.body = body; m.people = people; m.how = 2; m.ly = s.distLy;
        heard.push_back(m);
    }
    std::sort(heard.begin(), heard.end(), [](const ChartMark& a, const ChartMark& c) { return a.ly < c.ly; });
    int count = chartShardsOf(g);
    std::set<uint64_t> used;
    auto take = [&](const ChartMark& m) { if (used.count(m.star.seed)) return false; used.insert(m.star.seed); out.push_back(m); return true; };
    size_t hi = 0, ni = 0, si = 0;
    auto takePeople = [&](int turn) {   // the heard and the near in turns, whichever is left
        bool fromHeard = (turn % 2 == 0) ? hi < heard.size() : ni >= near.size();
        while (hi < heard.size() || ni < near.size()) {
            if (fromHeard && hi < heard.size()) { if (take(heard[hi++])) return true; }
            else if (ni < near.size()) { if (take(near[ni++])) return true; }
            else if (hi < heard.size()) { if (take(heard[hi++])) return true; }
        }
        return false;
    };
    auto takeBright = [&]() { for (; si < sky.size(); si++) { ChartMark m; m.star = sky[si].star; m.how = 0; m.ly = length(sky[si].star.pos - home.pos) / SECTOR_KM; if (take(m)) { si++; return true; } } return false; };
    uint64_t coin = mix64(g.seed ^ 0xC4A29ULL);
    for (int j = 0; j < count; j++) {
        bool people = j == 0 || (j >= 2 && ((coin >> j) & 1));
        if (people) { if (!takePeople(j) && !takeBright()) break; }
        else if (!takeBright() && !takePeople(j)) break;
    }
    (void)b;
}

void chartFigureNames(uint64_t worldSeed, int n, std::vector<std::string>& out) {
    out.clear();
    int order[24]; for (int i = 0; i < 24; i++) order[i] = i;
    Rng rng(mix64(worldSeed ^ 0x9A3E5ULL));
    for (int i = 23; i > 0; i--) std::swap(order[i], order[rng.irange(i + 1)]);
    for (int i = 0; i < n && i < 24; i++) out.push_back(FIGURES[order[i]]);
}

std::string chartFigureList(const std::vector<std::string>& names) {
    std::string s;
    for (size_t i = 0; i < names.size(); i++) {
        if (i > 0) s += i + 1 == names.size() ? " and " : ", ";
        s += names[i];
    }
    return s;
}

bool chartProject(const StarChart& c, const Vec3& dir, double& x, double& y) {
    double ang = std::acos(clampd(dot(dir, c.centre), -1, 1));
    double px = dot(dir, c.right), py = dot(dir, c.up), l = std::sqrt(px * px + py * py);
    if (l < 1e-9) { x = 0; y = 0; return ang <= CHART_FIELD; }
    double r = ang / CHART_FIELD;
    x = px / l * r; y = py / l * r;
    return ang <= CHART_FIELD;
}

void skyFiguresOf(const BodyGen& g, const std::vector<ChartStar>& sky, std::vector<ChartFigure>& out) {
    out.clear();
    int N = std::min((int)sky.size(), CHART_BRIGHT);
    if (N < 2) return;
    uint64_t seed = mix64(g.seed ^ 0xF16A5ULL);
    int n = CHART_FIGURES_MIN + (int)(seed % CHART_FIGURES_VAR);
    std::vector<std::string> names; chartFigureNames(g.seed, n, names);
    Rng rng(mix64(seed ^ 0x51ULL));
    std::vector<char> used(N, 0);
    auto angBetween = [&](int a, int c) { return std::acos(clampd(dot(sky[a].dir, sky[c].dir), -1, 1)); };
    for (int f = 0; f < n; f++) {
        ChartFigure fig; fig.name = names[f];
        int seedStar = -1;
        for (int tries = 0; tries < 2 && seedStar < 0; tries++)   // with the spacing first, then without
            for (int k = 0; k < N && seedStar < 0; k++) {
                if (used[k]) continue;
                bool apart = true;
                if (tries == 0) for (int j = 0; j < N && apart; j++) if (used[j] && angBetween(k, j) < CHART_APART) apart = false;
                if (apart) seedStar = k;
            }
        int size = 3 + rng.irange(4);
        if (seedStar < 0) break;
        fig.stars.push_back(seedStar); used[seedStar] = 1;
        while ((int)fig.stars.size() < size) {
            int best = -1, link = -1; double bd = CHART_LINK;
            for (int k = 0; k < N; k++) {
                if (used[k]) continue;
                for (int s : fig.stars) { double a = angBetween(k, s); if (a < bd) { bd = a; best = k; link = s; } }
            }
            if (best < 0) break;
            used[best] = 1; fig.stars.push_back(best); fig.lines.push_back({link, best});
        }
        if (fig.stars.size() < 2) continue;
        fig.label = seedStar;
        out.push_back(fig);
    }
}

// the chart: the hemisphere of the people's sky round the star they marked (the brightest hundred of the cube, those
// within ninety degrees of it; the mark itself added dim when it is not among them: a far transmitter is a star of no
// account in their sky), with the people's figures that fit whole within the field
bool chartOf(const StarSystem& sys, const Body& b, const BodyGen& g0, const Lore& L, int index, StarChart& out) {
    out = StarChart();
    BodyGen g = g0; g.seed = loreSeed(g0, L);   // C-13: the people's draws
    int which = chartIndexOf(g, index);
    if (which < 0 || which >= (int)L.marks.size()) return false;
    out.index = index; out.which = which; out.seed = chartSeedOf(g, index); out.from = sys.star.pos; out.mark = L.marks[which];
    out.centre = normalize(out.mark.star.pos - out.from);
    Vec3 pole = std::fabs(out.centre.y) < 0.98 ? Vec3(0, 1, 0) : Vec3(0, 0, 1);
    out.up = normalize(pole - out.centre * dot(pole, out.centre));
    out.right = cross(out.up, out.centre);
    std::vector<ChartStar> sky; chartSkyOf(sys.star, sky);
    std::vector<ChartFigure> figures; skyFiguresOf(g, sky, figures);
    std::vector<int> onChart(std::min((int)sky.size(), CHART_BRIGHT), -1);   // a sky star's index on the chart
    for (size_t k = 0; k < sky.size() && (int)k < CHART_BRIGHT; k++) {
        ChartStar cs = sky[k];
        cs.ang = std::acos(clampd(dot(cs.dir, out.centre), -1, 1));
        if (cs.ang > CHART_FIELD) continue;
        chartProject(out, cs.dir, cs.x, cs.y);
        if (cs.star.seed == out.mark.star.seed) { cs.mark = true; out.markStar = (int)out.stars.size(); }
        onChart[k] = (int)out.stars.size();
        out.stars.push_back(cs);
    }
    if (out.markStar < 0) {
        ChartStar cs; cs.star = out.mark.star; cs.dir = out.centre; cs.ang = 0; cs.x = 0; cs.y = 0; cs.mark = true;
        double d2 = length2(cs.star.pos - out.from) / (SECTOR_KM * SECTOR_KM); cs.bright = cs.star.luminosity / std::max(d2, 1e-6);
        out.markStar = (int)out.stars.size(); out.stars.push_back(cs);
    }
    for (const ChartFigure& f : figures) {
        bool whole = true;
        for (int k : f.stars) if (onChart[k] < 0) whole = false;
        if (!whole) continue;
        ChartFigure cf; cf.name = f.name; cf.label = onChart[f.label];
        for (int k : f.stars) cf.stars.push_back(onChart[k]);
        for (const auto& ln : f.lines) cf.lines.push_back({onChart[ln.first], onChart[ln.second]});
        out.figures.push_back(cf);
    }
    out.valid = true;
    (void)b;
    return true;
}
