// G-05 (2026-10-08): the galaxy chart, the star map's second page (R-409: "display whole galaxy map and show a point where you are +
// density of the stars"). The whole disc by its stars: face-on from the closed-form column (`galaxyFaceOnColumn`), edge-on from the
// field along the view's depth, toned like the harness's map and coloured by the light of its populations (the bulge's old stars
// warm, the disc between the arms a pale gold, the arms' young stars blue-white and weighted brighter, as starlight is); the
// globulars as fuzzy points, and from level 6 the open clusters and the star-forming complexes read from their cells (the column
// steps over anything 30 ly thick). Nothing on it has a name: the explorer's things are drawn over it (the ship, home, the stars
// visited and the way between them, the remote target and its line, the signals the radar locked, the stars named at the close levels).
// G-06 (2026-10-08): the next jump chosen on it ("the ability to select on that map where to jump next"): a cursor with what lies under
// it (the region, the stars there, its height, the way and the flight from the ship, what the explorer has there), Enter plots the jump
// to its point by R-407's line (`plotJump`) and flies it on a second Enter, Tab looks at the stars round the plotted end first, N steps
// through the explorer's things, L marks a place of the galaxy with a name (`Guide::galaxyMarks`, lent like R-408's marks).
#include "game.h"
#include "ui.h"
#include "core/parallel.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <ctime>
#include <functional>
#include <set>

namespace {

constexpr int FACE_W = UW;
constexpr int OPEN_KNOTS_LEVEL = 6;   // the open clusters and the star-forming complexes from 1,700 ly tall (13 ly a pixel)

struct Knot { Vec3 c; double r; };

// the globulars of the galaxy (about 145), found once: the cells within 60,000 ly of the centre, as the harness's `galaxy` mode counts them
const std::vector<Knot>& globularKnots() {
    static const std::vector<Knot> list = [] {
        std::vector<Knot> v;
        for (int64_t cx = -30; cx < 30; cx++) for (int64_t cy = -30; cy < 30; cy++) for (int64_t cz = -30; cz < 30; cz++) {
            Vec3 c; double R;
            if (globularInCell(cx, cy, cz, c, R)) v.push_back({c, R});
        }
        return v;
    }();
    return list;
}

// The light of a column of stars: the harness's log tone of the stars per square light year, the arms' young stars weighted up to four
// times at the crest (starlight, not a count: an arm holds twice the stars of its gap and outshines it four times), a square root for
// the screen; the hue by the shares of the three populations
struct Light { double r = 0, g = 0, b = 0; };
Light populationLight(double v, double disk, double arm, double bulge) {
    const double a = 1.6 * arm, s = disk + a + bulge;
    if (s <= 0 || v <= 0) return Light();
    return {v * (1.00 * bulge + 0.95 * disk + 0.68 * a) / s, v * (0.78 * bulge + 0.85 * disk + 0.80 * a) / s, v * (0.52 * bulge + 0.68 * disk + 1.00 * a) / s};
}
// the log tone runs to the core's column (3,000 stars a square light year), so the light climbs all the way in: the core 0.85 of white,
// the arms at 5,000 ly 0.7, home's arm 0.45 (the marks stay readable over it), its gap 0.25, the disc at 25,000 ly 0.12
constexpr double CHART_EXPOSURE = 0.85;
Light faceLight(const FaceOnColumn& c) {
    const double tone = std::log1p(c.column * (1 + 3 * c.arm) / 40) / std::log1p(4000.0 / 40);
    return populationLight(CHART_EXPOSURE * std::pow(clampd(tone, 0, 1), 0.7), c.disk, c.arm, c.bulge);
}
// edge-on, the mean density along the view's depth (0.15 in home's plane, 0.97 in the core), linear in its log tone so the disc's
// thickness reads: home's plane 0.7, two scale heights over it 0.4, four 0.15
Light edgeLight(double mean, double disk, double arm, double bulge) {
    const double tone = std::log1p(mean * (1 + 3 * arm) / 0.004) / std::log1p(3.0 / 0.004);
    return populationLight(0.75 * clampd(tone, 0, 1), disk, arm, bulge);
}
inline uint32_t toRGB(const Light& L, double k = 255) {
    return rgb(std::min(255, (int)(L.r * k)), std::min(255, (int)(L.g * k)), std::min(255, (int)(L.b * k)));
}

// a knot's light added round (fx, fy) (the image's pixels, fractional): a Plummer-like profile of `rpx` pixels, at least a fuzzy point
void addKnot(std::vector<uint32_t>& img, int W, int H, double fx, double fy, double rpx, double cr, double cg, double cb, double strength) {
    const double core = std::max(0.9, 0.5 * rpx);
    const int reach = (int)std::ceil(std::max(1.5, 1.6 * rpx)), ix = (int)std::floor(fx), iy = (int)std::floor(fy);
    for (int y = iy - reach; y <= iy + reach; y++) {
        if (y < 0 || y >= H) continue;
        for (int x = ix - reach; x <= ix + reach; x++) {
            if (x < 0 || x >= W) continue;
            const double dx = x + 0.5 - fx, dy = y + 0.5 - fy, q2 = (dx * dx + dy * dy) / (core * core), w = strength / ((1 + q2) * (1 + q2));
            if (w < 0.01) continue;
            uint32_t& p = img[(size_t)y * W + x];
            p = rgb(std::min(255, (int)(p & 255) + (int)(255 * cr * w)), std::min(255, (int)((p >> 8) & 255) + (int)(255 * cg * w)), std::min(255, (int)((p >> 16) & 255) + (int)(255 * cb * w)));
        }
    }
}

// The face: the column at each pixel (2 x 2 samples to level 3, where an arm's crest is a few pixels wide), then the knots
void buildFace(int level, long long I, long long J, std::vector<uint32_t>& img) {
    const double s = Game::chartScale(level);
    const int H = Game::CHART_FACE_H, ss = level <= 3 ? 2 : 1;
    const double x0 = (double)(I - FACE_W / 2) * s, z0 = (double)(J - H / 2) * s;   // the face's top left (sector units)
    img.assign((size_t)FACE_W * H, rgb(0, 0, 0));
    parallelFor(H, 4, [&](int jb, int je) {
        for (int j = jb; j < je; j++)
            for (int i = 0; i < FACE_W; i++) {
                Light sum;
                for (int sj = 0; sj < ss; sj++)
                    for (int si = 0; si < ss; si++) {
                        const Light L = faceLight(galaxyFaceOnColumn(x0 + (i + (si + 0.5) / ss) * s, z0 + (j + (sj + 0.5) / ss) * s));
                        sum.r += L.r; sum.g += L.g; sum.b += L.b;
                    }
                img[(size_t)j * FACE_W + i] = toRGB(sum, 255.0 / (ss * ss));
            }
    });
    const double x1 = x0 + FACE_W * s, z1 = z0 + H * s;
    for (const Knot& k : globularKnots())   // old stars: warm
        if (k.c.x > x0 - 60 && k.c.x < x1 + 60 && k.c.z > z0 - 60 && k.c.z < z1 + 60)
            addKnot(img, FACE_W, H, (k.c.x - x0) / s, (k.c.z - z0) / s, k.r / s, 1.0, 0.86, 0.62, 0.42);
    if (level < OPEN_KNOTS_LEVEL) return;
    {   // the open clusters: cells of 100 ly from the galactic centre, through six scale heights of the view's centre
        const double h = galaxyTerms(0.5 * (x0 + x1), 0, 0.5 * (z0 + z1)).h, ymax = std::min(9000.0, 6 * h);
        const int64_t ax = (int64_t)std::floor((x0 - 15 - GALAXY_CENTRE_SX) / OPEN_CLUSTER_CELL), bx = (int64_t)std::floor((x1 + 15 - GALAXY_CENTRE_SX) / OPEN_CLUSTER_CELL);
        const int64_t az = (int64_t)std::floor((z0 - 15 - GALAXY_CENTRE_SZ) / OPEN_CLUSTER_CELL), bz = (int64_t)std::floor((z1 + 15 - GALAXY_CENTRE_SZ) / OPEN_CLUSTER_CELL);
        const int64_t ay = (int64_t)std::floor(-ymax / OPEN_CLUSTER_CELL), by = (int64_t)std::floor(ymax / OPEN_CLUSTER_CELL);
        for (int64_t cx = ax; cx <= bx; cx++) for (int64_t cz = az; cz <= bz; cz++) for (int64_t cy = ay; cy <= by; cy++) {   // young stars: blue-white knots
            Vec3 c; double R;
            if (openClusterInCell(cx, cy, cz, c, R)) addKnot(img, FACE_W, H, (c.x - x0) / s, (c.z - z0) / s, R / s, 0.75, 0.86, 1.0, 0.36);
        }
    }
    {   // the star-forming complexes: their columns of NEBULA_CELL in the arms, a faint red glow of the complex's reach
        const int64_t ax = (int64_t)std::floor((x0 - NEBULA_RADIUS) / NEBULA_CELL), bx = (int64_t)std::floor((x1 + NEBULA_RADIUS) / NEBULA_CELL);
        const int64_t az = (int64_t)std::floor((z0 - NEBULA_RADIUS) / NEBULA_CELL), bz = (int64_t)std::floor((z1 + NEBULA_RADIUS) / NEBULA_CELL);
        for (int64_t cx = ax; cx <= bx; cx++) for (int64_t cz = az; cz <= bz; cz++) {
            Vec3 c;
            if (nebulaInCell(cx, cz, c)) addKnot(img, FACE_W, H, (c.x - x0) / s, (c.z - z0) / s, NEBULA_RADIUS / s, 1.0, 0.32, 0.28, 0.3);
        }
    }
}

// The edge: the field's mean density along the face's depth at each pixel of the strip (its height `half` over and under the plane)
void buildEdge(int level, long long I, long long J, double half, std::vector<uint32_t>& img) {
    const double s = Game::chartScale(level);
    const int H = Game::CHART_EDGE_H, FH = Game::CHART_FACE_H, N = 40;
    const double x0 = (double)(I - FACE_W / 2) * s, z0 = (double)(J - FH / 2) * s, depth = FH * s, dy = 2 * half / H;
    img.assign((size_t)FACE_W * H, rgb(0, 0, 0));
    parallelFor(H, 1, [&](int jb, int je) {
        for (int j = jb; j < je; j++) {
            const double y = half - (j + 0.5) * dy;
            for (int i = 0; i < FACE_W; i++) {
                const double x = x0 + (i + 0.5) * s;
                double tot = 0, dk = 0, am = 0, bu = 0;
                for (int k = 0; k < N; k++) {
                    const GalaxyTerms g = galaxyTerms(x, y, z0 + (k + 0.5) / N * depth);
                    const double d0 = g.disk * 0.55, a0 = g.disk * 0.6 * g.arm;
                    dk += d0; am += a0; bu += g.bulge; tot += std::min(GALAXY_CAP, d0 + a0 + g.bulge);
                }
                const double sum = dk + am + bu;
                if (sum > 0) img[(size_t)j * FACE_W + i] = toRGB(edgeLight(tot / N, dk / sum, am / sum, bu / sum));
            }
        }
    });
    for (const Knot& k : globularKnots())
        if (k.c.z >= z0 && k.c.z < z0 + depth && std::fabs(k.c.y) < half + 60)
            addKnot(img, FACE_W, H, (k.c.x - x0) / s, (half - k.c.y) / dy, k.r / std::max(s, dy), 1.0, 0.86, 0.62, 0.42);
}

void blitLogical(RGBCanvas& c, const std::vector<uint32_t>& img, int x0, int y0, int w, int h) {
    const int S = c.scale;
    for (int y = 0; y < h * S; y++) {
        const int py = y0 * S + y;
        if (py < 0 || py >= c.h) continue;
        const uint32_t* row = &img[(size_t)(y / S) * w];
        uint32_t* out = c.px + (size_t)py * c.w;
        for (int x = 0; x < w * S; x++) { const int px = x0 * S + x; if (px >= 0 && px < c.w) out[px] = row[x / S]; }
    }
}

// Liang-Barsky against a rectangle (logical pixels): false when nothing of the line is inside
bool clipLine(double& x0, double& y0, double& x1, double& y1, double xmin, double ymin, double xmax, double ymax) {
    double t0 = 0, t1 = 1;
    const double dx = x1 - x0, dy = y1 - y0;
    const double p[4] = {-dx, dx, -dy, dy}, q[4] = {x0 - xmin, xmax - x0, y0 - ymin, ymax - y0};
    for (int i = 0; i < 4; i++) {
        if (p[i] == 0) { if (q[i] < 0) return false; continue; }
        const double t = q[i] / p[i];
        if (p[i] < 0) { if (t > t1) return false; t0 = std::max(t0, t); }
        else { if (t < t0) return false; t1 = std::min(t1, t); }
    }
    const double ax = x0 + t0 * dx, ay = y0 + t0 * dy;
    x1 = x0 + t1 * dx; y1 = y0 + t1 * dy; x0 = ax; y0 = ay;
    return true;
}

Vec3 sectorCentre(const std::string& key) {   // a star key's sector, its middle (the chart's pixels are 3 ly and more)
    int64_t x = 0, y = 0, z = 0;
    if (!Guide::parseStarKey(key, x, y, z)) return Vec3(1e12, 0, 1e12);
    return Vec3(x + 0.5, y + 0.5, z + 0.5);
}

}   // namespace

void Game::chartCentre(int level, double& cx, double& cz) const {
    if (level == 0) { cx = GALAXY_CENTRE_SX; cz = GALAXY_CENTRE_SZ; return; }   // the whole disc
    if (gchart.follow) { cx = ship.pos.x / SECTOR_KM; cz = ship.pos.z / SECTOR_KM; return; }
    cx = gchart.cx; cz = gchart.cz;
}

void Game::chartView(long long& I, long long& J) const {
    const double s = chartScale(gchart.level);
    double cx, cz; chartCentre(gchart.level, cx, cz);
    I = std::llround(cx / s); J = std::llround(cz / s);
}

bool Game::chartProject(double sx, double sz, int& px, int& py) const {
    long long I, J; chartView(I, J);
    const double s = chartScale(gchart.level);
    const double fx = clampd(sx / s - (double)I + UW / 2, -1e6, 1e6), fy = clampd(sz / s - (double)J + CHART_FACE_H / 2, -1e6, 1e6);
    px = (int)std::floor(fx); py = CHART_FACE_Y0 + (int)std::floor(fy);
    return px >= 0 && px < UW && py >= CHART_FACE_Y0 && py < CHART_FACE_Y0 + CHART_FACE_H;
}

// ---- G-06: the next jump chosen on the chart --------------------------------------------------------------------------------------

Vec3 Game::chartCursor() const { return gchart.onShip ? ship.pos / SECTOR_KM : Vec3(gchart.curX, gchart.curY, gchart.curZ); }

bool Game::chartPlotted() const {
    return gchart.plotHere && !gchart.onShip && jumpPlot.seed && jumpPlot.onChart && ship.hasRemote && ship.remote.seed == jumpPlot.seed && ship.mode != ShipState::VIMANA;
}

void Game::chartLetGo(bool height) {
    if (gchart.onShip) {
        const Vec3 s = ship.pos / SECTOR_KM;
        gchart.curX = s.x; gchart.curZ = s.z; gchart.curY = height ? s.y : 0;   // where the stars are, unless the height is what moves
        gchart.onShip = false;
    }
    chartUnplot();
}

void Game::chartUnplot() {
    if (gchart.plotHere && statusMsg.rfind("PLOTTED", 0) == 0) statusUntil = 0;   // its status goes with it
    gchart.plotHere = false;
}

// Enter on the chart: the jump plotted to the cursor's point (R-407's line: the star nearest its end, or back along it to the first
// stars), or, plotted, flown
void Game::chartEnter() {
    if (ship.mode == ShipState::VIMANA) { status("CANNOT JUMP DURING VIMANA FLIGHT", 3); audio.beep = 3; return; }
    if (chartPlotted()) { state = GameState::SPACE; flyJump(); return; }
    const Vec3 to = chartCursor(), from = ship.pos / SECTOR_KM;
    const double ly = length(to - from);
    if (gchart.onShip || ly < 1) { status("MOVE THE CURSOR TO WHERE THE JUMP GOES", 3); audio.beep = 3; return; }
    if (!plotJump(ly, (to - from) / ly, true)) return;
    gchart.plotHere = true;
    status("PLOTTED - ENTER FLIES, ESC KEEPS IT FOR V", 4);
}

std::vector<Game::ChartStop> Game::chartStops() const {
    const Vec3 ss = ship.pos / SECTOR_KM;
    std::vector<ChartStop> out, g;
    std::set<std::string> taken;   // a star once (its first kind), the ship's own left out: it is YOU
    if (sys.valid) taken.insert(starKeyOf(sys.star));
    auto star = [&](const std::string& key, const std::string& label, uint32_t col) {   // the star of a key's sector (a world's key: its star's)
        const std::string k = key.substr(0, key.find('/'));
        int64_t x, y, z;
        if (taken.count(k) || !Guide::parseStarKey(k, x, y, z)) return;
        taken.insert(k);
        Star st;
        g.push_back({starInSector(x, y, z, st, false) ? st.pos / SECTOR_KM : Vec3(x + 0.5, y + 0.5, z + 0.5), label, col});
    };
    auto kind = [&]() {   // a kind's stops, nearest the ship first
        std::stable_sort(g.begin(), g.end(), [&](const ChartStop& a, const ChartStop& b) { return length2(a.p - ss) < length2(b.p - ss); });
        out.insert(out.end(), g.begin(), g.end());
        g.clear();
    };
    for (const auto& kv : guide.signals) star(kv.first, "A SIGNAL", HUD_RED);
    kind();
    if (!guide.home.empty()) star(guide.home, "HOME", HUD_AMBER);
    kind();
    for (const auto& kv : guide.names) if (kv.first.find('/') == std::string::npos) star(kv.first, upper(kv.second), HUD_GREEN);
    for (const auto& kv : guide.inbox) if (kv.first.find('/') == std::string::npos && !guide.names.count(kv.first)) star(kv.first, upper(kv.second), HUD_CYAN);
    kind();
    for (const GalaxyMark& m : guide.galaxyMarks) g.push_back({Vec3(m.x, m.y, m.z), m.name.empty() ? std::string("A MARK") : upper(m.name), m.lent ? HUD_CYAN : HUD_GREEN});
    kind();
    for (const std::string& k : guide.visited) star(k, "A STAR VISITED", HUD_GREEN);
    kind();
    return out;
}

void Game::chartNextStop(int dir) {
    const std::vector<ChartStop> stops = chartStops();
    if (stops.empty()) { status("NOTHING OF YOURS ON THE CHART YET", 3); audio.beep = 3; return; }
    const int n = (int)stops.size();
    gchart.stop = gchart.stop < 0 || gchart.stop >= n ? (dir > 0 ? 0 : n - 1) : (gchart.stop + dir + n) % n;
    const ChartStop& st = stops[gchart.stop];
    chartUnplot(); gchart.onShip = false;
    gchart.curX = st.p.x; gchart.curY = st.p.y; gchart.curZ = st.p.z;
    if (gchart.level > 0) { gchart.follow = false; gchart.cx = st.p.x; gchart.cz = st.p.z; }
    status(fmt("%s - %d OF %d", trunc(st.label, 36).c_str(), gchart.stop + 1, n), 3);
    audio.beep = 4;
}

int Game::galaxyMarkNear(double sx, double sz, double px) const {
    const double r = px * chartScale(gchart.level);
    int best = -1; double bd = r * r;
    for (int i = 0; i < (int)guide.galaxyMarks.size(); i++) {
        const GalaxyMark& m = guide.galaxyMarks[i];
        const double d2 = (m.x - sx) * (m.x - sx) + (m.z - sz) * (m.z - sz);
        if (d2 <= bd) { bd = d2; best = i; }
    }
    return best;
}

Game::ChartThere Game::chartThere(const Vec3& p) const {
    const double r = 4 * chartScale(gchart.level), r2 = r * r;
    auto near = [&](const std::string& key) { const Vec3 v = sectorCentre(key); return (v.x - p.x) * (v.x - p.x) + (v.z - p.z) * (v.z - p.z) <= r2; };
    ChartThere t;
    const int mk = galaxyMarkNear(p.x, p.z, 4);
    if (mk >= 0) {
        const GalaxyMark& m = guide.galaxyMarks[mk];
        t.kind = 1; t.mark = mk; t.col = m.lent ? HUD_CYAN : HUD_GREEN; t.label = m.name.empty() ? std::string("A MARK") : trunc(upper(m.name), 16);
        return t;
    }
    for (const auto& kv : guide.signals) if (near(kv.first)) { t.kind = 2; t.key = kv.first.substr(0, kv.first.find('/')); t.col = HUD_RED; t.label = "A SIGNAL"; return t; }
    if (!guide.home.empty() && near(guide.home)) { t.kind = 3; t.key = guide.home; t.col = HUD_AMBER; t.label = "HOME"; return t; }
    for (const auto& kv : guide.names) if (kv.first.find('/') == std::string::npos && near(kv.first)) { t.kind = 4; t.key = kv.first; t.col = HUD_GREEN; t.label = trunc(upper(kv.second), 16); return t; }
    for (const auto& kv : guide.inbox) if (kv.first.find('/') == std::string::npos && !guide.names.count(kv.first) && near(kv.first)) { t.kind = 4; t.key = kv.first; t.col = HUD_CYAN; t.label = trunc(upper(kv.second), 16); return t; }
    for (const std::string& k : guide.visited) if (near(k)) { t.kind = 5; t.key = k; t.col = HUD_GREEN; t.label = "VISITED"; return t; }
    return t;
}

std::string Game::galaxyPlace(const Vec3& p, bool height) const {
    const GalaxyTerms g = galaxyTerms(p.x, p.y, p.z);
    std::string s = std::string(REGION_NAMES[galaxyRegion((int64_t)std::floor(p.x), (int64_t)std::floor(p.y), (int64_t)std::floor(p.z))]) + ", " +
                    (g.r < 100 ? std::string("AT THE CORE") : thousands(std::round(g.r / 10) * 10) + " LY FROM THE CORE");
    if (height && std::fabs(p.y) >= 50) s += ", " + thousands(std::round(std::fabs(p.y) / 10) * 10) + (p.y > 0 ? " LY NORTH OF THE PLANE" : " LY SOUTH OF THE PLANE");
    return s;
}

void Game::beginGalaxyMark(int mi) {
    if (mi >= (int)guide.galaxyMarks.size()) return;
    textGalaxyMark = mi;
    textGalaxyAt = mi >= 0 ? Vec3(guide.galaxyMarks[mi].x, guide.galaxyMarks[mi].y, guide.galaxyMarks[mi].z) : chartCursor();
    textGalaxyLine = galaxyPlace(textGalaxyAt, false);
    guideReturn = GameState::STAR_MAP;
    beginTextEntry(mi >= 0 ? "RENAME THE PLACE" : "NAME THE PLACE", 10, mi >= 0 ? upper(guide.galaxyMarks[mi].name) : std::string());
}

void Game::finishGalaxyMark(const std::string& name) {
    if (textGalaxyMark >= 0) {   // a rename (a friend's mark renamed is the explorer's own)
        if (textGalaxyMark >= (int)guide.galaxyMarks.size()) { textGalaxyMark = -1; return; }
        GalaxyMark& m = guide.galaxyMarks[textGalaxyMark];
        const bool renamed = m.name != name;
        m.name = name; m.lent = false;
        guide.save(guidePath);
        if (renamed && !name.empty()) logEvent("NOTE", fmt("NAMED A PLACE OF THE GALAXY %s", name.c_str()));
        status(name.empty() ? "THE MARK HAS NO NAME NOW" : fmt("THE PLACE IS NOW CALLED %s", name.c_str()), 4);
        textGalaxyMark = -1;
        return;
    }
    GalaxyMark m; m.x = textGalaxyAt.x; m.y = textGalaxyAt.y; m.z = textGalaxyAt.z; m.name = name; m.when = (long long)std::time(nullptr);
    guide.galaxyMarks.push_back(m);
    logEvent("MARK", fmt("%s IN THE GALAXY: %s", name.empty() ? "A MARK" : name.c_str(), galaxyPlace(textGalaxyAt, true).c_str()));   // saves the guide
    status(name.empty() ? "A MARK PLACED - L ON IT NAMES IT" : fmt("MARKED: %s", name.c_str()), 4);
    audio.beep = 1;
}

bool Game::drawMapStatus() {
    if (realTime >= statusUntil || statusMsg.empty()) return false;
    blendRectRGB(canvas, 0, UH - 13, UW - 1, UH - 3, rgb(0, 0, 0), 160);
    drawTextCentered(canvas, UW / 2, UH - 12, trunc(statusMsg, 52).c_str(), HUD_AMBER);
    return true;
}

void Game::updateGalaxyChart(const Input& in) {
    if (in.wasPressed(KEY_ESCAPE) || (in.wasPressed(KEY_M) && !in.ctrl())) { state = GameState::SPACE; return; }
    const int was = gchart.level;
    if (in.wheel > 0 || in.wasPressed(KEY_EQUAL)) gchart.level = std::min(CHART_LEVELS - 1, gchart.level + 1);
    if (in.wheel < 0 || in.wasPressed(KEY_MINUS)) gchart.level = std::max(0, gchart.level - 1);
    if (gchart.level != was) {
        audio.beep = 4;
        if (!gchart.onShip && gchart.level > 0) {   // G-06: the zoom about the cursor: it keeps its pixel
            double vx, vz; chartCentre(was, vx, vz);
            const double k = chartScale(gchart.level) / chartScale(was);
            gchart.cx = gchart.curX - (gchart.curX - vx) * k; gchart.cz = gchart.curZ - (gchart.curZ - vz) * k;
            gchart.follow = false;
        }
    }
    if (in.wasPressed(KEY_C) || in.wasPressed(KEY_SPACE)) { gchart.follow = true; gchart.onShip = true; chartUnplot(); audio.beep = 4; }
    if (in.wasPressed(KEY_N) && !in.ctrl()) chartNextStop(in.shift() ? -1 : 1);
    const Vec3 cur = chartCursor();
    if (in.wasPressed(KEY_L) && !in.ctrl()) { beginGalaxyMark(galaxyMarkNear(cur.x, cur.z, 4)); return; }
    if (in.wasPressed(KEY_DELETE) || in.wasPressed(KEY_BACKSPACE)) {   // twice, as on the sector map
        const int mk = galaxyMarkNear(cur.x, cur.z, 4);
        if (mk >= 0 && mk == gchart.delMark && realTime < gchart.delUntil) {
            const std::string name = guide.galaxyMarks[mk].name.empty() ? std::string("A MARK") : upper(guide.galaxyMarks[mk].name);
            guide.galaxyMarks.erase(guide.galaxyMarks.begin() + mk); guide.save(guidePath);
            gchart.delMark = -1; status(fmt("MARK REMOVED: %s", trunc(name, 30).c_str()), 3); audio.beep = 4;
        } else if (mk >= 0) {
            gchart.delMark = mk; gchart.delUntil = realTime + 3;
            status(fmt("REMOVE %s? DELETE AGAIN", trunc(guide.galaxyMarks[mk].name.empty() ? std::string("A MARK") : upper(guide.galaxyMarks[mk].name), 24).c_str()), 3); audio.beep = 3;
        } else { status("NO MARK UNDER THE CURSOR", 2); audio.beep = 3; }
    }
    if (enterKey(in) || in.mousePressed[0]) { chartEnter(); if (state != GameState::STAR_MAP) return; }
    // the height over the plane (PgUp, PgDn): two seconds from the plane to the strip's edge, Shift faster
    const double half0 = clampd(CHART_TALL_LY / std::ldexp(1.0, gchart.level) / 16, 600, 3000);
    double dy = 0;
    if (in.isDown(KEY_PAGE_UP)) dy += (in.shift() ? 3.0 : 1.0) * 0.5 * half0 * lastRealDt;
    if (in.isDown(KEY_PAGE_DOWN)) dy -= (in.shift() ? 3.0 : 1.0) * 0.5 * half0 * lastRealDt;
    if (dy != 0) { chartLetGo(true); gchart.curY = clampd(gchart.curY + dy, -CHART_HEIGHT_LY, CHART_HEIGHT_LY); }
    // the cursor over the face: 80 pixels a second (Shift three times), the mouse; a nudge of the mouse does not let the ship go
    const double s = chartScale(gchart.level), sp = (in.shift() ? 3.0 : 1.0) * 80.0 * lastRealDt;
    double dx = 0, dz = 0;
    if (in.isDown(KEY_LEFT)) dx -= sp;
    if (in.isDown(KEY_RIGHT)) dx += sp;
    if (in.isDown(KEY_UP)) dz -= sp;
    if (in.isDown(KEY_DOWN)) dz += sp;
    if (!gchart.onShip || std::fabs(in.mouseDx) + std::fabs(in.mouseDy) >= 2) { dx += in.mouseDx * 0.5 * settings.mouseSensitivity; dz += in.mouseDy * 0.5 * settings.mouseSensitivity; }
    if (dx == 0 && dz == 0) return;
    chartLetGo(false);
    gchart.curX += dx * s; gchart.curZ += dz * s;
    const double rx = gchart.curX - GALAXY_CENTRE_SX, rz = gchart.curZ - GALAXY_CENTRE_SZ, r = std::sqrt(rx * rx + rz * rz);
    if (r > CHART_REACH_LY) { gchart.curX = GALAXY_CENTRE_SX + rx * CHART_REACH_LY / r; gchart.curZ = GALAXY_CENTRE_SZ + rz * CHART_REACH_LY / r; }
    if (gchart.level == 0) return;   // the whole disc holds it
    double vx, vz; chartCentre(gchart.level, vx, vz);   // the view moves when the cursor nears the face's edge
    const double mx = (UW / 2 - 16) * s, mz = (CHART_FACE_H / 2 - 12) * s;
    const double nx = clampd(vx, gchart.curX - mx, gchart.curX + mx), nz = clampd(vz, gchart.curZ - mz, gchart.curZ + mz);
    if (nx != vx || nz != vz) { gchart.follow = false; gchart.cx = nx; gchart.cz = nz; }
}

void Game::renderGalaxyChart() {
    fillRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0));
    const int level = gchart.level, FY0 = CHART_FACE_Y0, FH = CHART_FACE_H, EY0 = CHART_EDGE_Y0, EH = CHART_EDGE_H;
    const double s = chartScale(level);
    const Vec3 ss = ship.pos / SECTOR_KM;   // the ship in sector units
    const Vec3 cur = chartCursor();         // G-06: the cursor's point
    const bool away = !gchart.onShip, plotted = chartPlotted();
    long long I, J; chartView(I, J);
    // the strip's height over and under the plane: the disc's thickness at the level, the ship and the cursor always inside
    double half = clampd(CHART_TALL_LY / std::ldexp(1.0, level) / 16, 600, 3000);
    const double tallest = std::max(std::fabs(ss.y), std::fabs(cur.y));
    if (tallest > 0.85 * half) half = std::ceil(tallest / 0.85 / 100) * 100;
    if (gchart.builtLevel != level || gchart.builtI != I || gchart.builtJ != J || gchart.builtHalf != half) {
        const auto t0 = std::chrono::steady_clock::now();
        buildFace(level, I, J, gchart.face);
        buildEdge(level, I, J, half, gchart.edge);
        gchart.buildMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        gchart.builtLevel = level; gchart.builtI = I; gchart.builtJ = J; gchart.builtHalf = half;
    }
    blitLogical(canvas, gchart.face, 0, FY0, UW, FH);
    blitLogical(canvas, gchart.edge, 0, EY0, UW, EH);
    drawLineRGB(canvas, 0, EY0 - 2, UW - 1, EY0 - 2, rgb(28, 48, 36));   // the strip's own frame under the face
    drawLineRGB(canvas, 0, EY0 + EH + 1, UW - 1, EY0 + EH + 1, rgb(28, 48, 36));

    const double faceX0 = 0, faceY0 = FY0, faceX1 = UW - 1, faceY1 = FY0 + FH - 1;
    auto proj = [&](double x, double z, double& fx, double& fy) { fx = x / s - (double)I + UW / 2; fy = FY0 + z / s - (double)J + FH / 2; };
    auto inFace = [&](int x, int y) { return x >= 0 && x < UW && y >= FY0 && y < FY0 + FH; };
    auto dot = [&](int x, int y, uint32_t c) { if (inFace(x, y)) fillRectRGB(canvas, x, y, x, y, c); };
    auto segment = [&](double ax, double az, double bx, double bz, uint32_t c, bool dotted) {   // a line between two places, clipped to the face
        double x0, y0, x1, y1; proj(ax, az, x0, y0); proj(bx, bz, x1, y1);
        if (!clipLine(x0, y0, x1, y1, faceX0, faceY0, faceX1, faceY1)) return false;
        const int ix0 = (int)std::floor(x0), iy0 = (int)std::floor(y0), ix1 = (int)std::floor(x1), iy1 = (int)std::floor(y1);
        if (!dotted) { drawLineRGB(canvas, ix0, iy0, ix1, iy1, c); return true; }
        const int n = std::max(std::abs(ix1 - ix0), std::abs(iy1 - iy0));
        for (int k = 0; k <= n; k += 2) dot(ix0 + (n ? (ix1 - ix0) * k / n : 0), iy0 + (n ? (iy1 - iy0) * k / n : 0), c);
        return true;
    };
    auto ring = [&](int x, int y, int r, uint32_t c) { for (int k = 0; k < 16; k++) { const double a = k * TAU / 16; dot(x + (int)std::lround(r * std::cos(a)), y + (int)std::lround(r * std::sin(a)), c); } };
    auto diamond = [&](int x, int y, int r, uint32_t c) {
        for (int k = 0; k <= r; k++) { dot(x - r + k, y - k, c); dot(x + r - k, y - k, c); dot(x - r + k, y + k, c); dot(x + r - k, y + k, c); }
    };
    auto shadowed = [&](const std::function<void(int, int, uint32_t)>& draw, int x, int y, uint32_t c) {   // a dark rim first: readable over the core's light
        for (int oy = -1; oy <= 1; oy++) for (int ox = -1; ox <= 1; ox++) if (ox || oy) draw(x + ox, y + oy, HUD_SHADOW);
        draw(x, y, c);
    };
    auto cross = [&](int x, int y, uint32_t c) { for (int d = 2; d <= 5; d++) { dot(x + d, y, c); dot(x - d, y, c); dot(x, y + d, c); dot(x, y - d, c); } };
    auto pennant = [&](int x, int y, uint32_t c) {   // G-06: a mark of the galaxy, its foot at the place
        for (int k = 0; k <= 5; k++) dot(x, y - k, c);
        for (int k = 1; k <= 3; k++) dot(x + k, y - 5, c);
        dot(x + 1, y - 4, c); dot(x + 2, y - 4, c); dot(x + 1, y - 3, c);
    };
    auto corners = [&](int x, int y, uint32_t c) {   // G-06: the cursor, open round what lies under it
        for (int k = 0; k < 3; k++) {
            dot(x - 4 + k, y - 4, c); dot(x - 4, y - 4 + k, c); dot(x + 4 - k, y - 4, c); dot(x + 4, y - 4 + k, c);
            dot(x - 4 + k, y + 4, c); dot(x - 4, y + 4 - k, c); dot(x + 4 - k, y + 4, c); dot(x + 4, y + 4 - k, c);
        }
    };

    // the way the ship came: a line per leg in the order of the arrivals, then the stars visited
    gchart.legs.clear();
    const uint32_t ROUTE = rgb(70, 130, 95);
    for (size_t k = 1; k < guide.route.size(); k++) {
        if (guide.route[k] == guide.route[k - 1]) continue;
        const Vec3 a = sectorCentre(guide.route[k - 1]), b = sectorCentre(guide.route[k]);
        if (segment(a.x, a.z, b.x, b.z, ROUTE, false)) gchart.legs.push_back({guide.route[k - 1], guide.route[k]});
    }
    for (const std::string& key : guide.visited) {
        const Vec3 v = sectorCentre(key); int x, y;
        if (chartProject(v.x, v.z, x, y)) dot(x, y, HUD_GREEN);
    }
    // the labels kept apart: the ship's first, then home's and what the cursor rests on (G-06), then the names of the explorer's marks of
    // the galaxy at every level and of the stars they named at the close ones; a name that finds no room is left out
    std::vector<std::array<int, 4>> boxes;
    auto fits = [&](int lx, int ly, int w) {
        if (ly < FY0 || ly + 7 > FY0 + FH || lx < 1 || lx + w > UW - 1) return false;
        for (const auto& b : boxes) if (lx < b[2] && lx + w > b[0] && ly < b[3] && ly + 8 > b[1]) return false;
        return true;
    };
    auto place = [&](int x, int y, const std::string& text, int dx, int& lx, int& ly) {   // right of the place, under, over, then left
        const int w = textWidth(text.c_str());
        for (int side = 0; side < 2; side++)
            for (int dy : {-3, 6, -12}) {
                lx = side == 0 ? x + dx : x - dx - w; ly = y + dy;
                if (fits(lx, ly, w)) { boxes.push_back({lx, ly, lx + w, ly + 8}); return true; }
            }
        return false;
    };
    auto label = [&](int x, int y, const std::string& text, uint32_t col, int dx) { int lx, ly; if (place(x, y, text, dx, lx, ly)) drawTextShadow(canvas, lx, ly, text.c_str(), col, HUD_SHADOW); };
    int yx, yy; const bool shipIn = chartProject(ss.x, ss.z, yx, yy);
    int youLX = -9999, youLY = 0;
    if (shipIn) { boxes.push_back({yx - 5, yy - 5, yx + 6, yy + 6}); place(yx, yy, "YOU", 8, youLX, youLY); }
    int hx = -9999, hy = -9999; bool homeIn = false;
    if (!guide.home.empty()) { const Vec3 h = sectorCentre(guide.home); homeIn = chartProject(h.x, h.z, hx, hy); }
    const ChartThere there = away ? chartThere(cur) : ChartThere();
    int ccx = -9999, ccy = -9999; const bool curIn = away && chartProject(cur.x, cur.z, ccx, ccy);
    int thereLX = -9999, thereLY = 0;
    if (curIn) { boxes.push_back({ccx - 4, ccy - 4, ccx + 5, ccy + 5}); if (!there.label.empty()) place(ccx, ccy, there.label, 8, thereLX, thereLY); }
    int homeLX = -9999, homeLY = 0;
    if (homeIn && there.kind != 3 && (!shipIn || std::abs(hx - yx) > 14 || std::abs(hy - yy) > 9)) place(hx, hy, "HOME", 7, homeLX, homeLY);
    for (int i = 0; i < (int)guide.galaxyMarks.size(); i++) {
        const GalaxyMark& m = guide.galaxyMarks[i];
        int x, y;
        if (!chartProject(m.x, m.z, x, y)) continue;
        const uint32_t col = m.lent ? HUD_CYAN : HUD_GREEN;
        shadowed(pennant, x, y, col);
        if (!m.name.empty() && !(there.kind == 1 && there.mark == i)) label(x, y - 2, trunc(upper(m.name), 16), col, 6);
    }
    if (level >= 6) {
        auto named = [&](const std::map<std::string, std::string>& m, uint32_t col) {
            for (const auto& kv : m) {
                if (kv.first.find('/') != std::string::npos) continue;   // a world's, a shard's or a storm's name
                if (&m == &guide.inbox && guide.names.count(kv.first)) continue;   // the explorer's own name wins
                const Vec3 v = sectorCentre(kv.first); int x, y;
                if (!chartProject(v.x, v.z, x, y)) continue;
                dot(x, y, col);
                if (!(there.kind == 4 && there.key == kv.first)) label(x, y, trunc(upper(kv.second), 12), col, 4);
            }
        };
        named(guide.names, HUD_GREEN);
        named(guide.inbox, HUD_CYAN);
    }
    // the signals the radar locked: rings in the radar's red (KI-349)
    for (const auto& kv : guide.signals) {
        const Vec3 v = sectorCentre(kv.first); int x, y;
        if (chartProject(v.x, v.z, x, y)) ring(x, y, 3, HUD_RED);
    }
    // home
    if (homeIn) shadowed([&](int x, int y, uint32_t c) { diamond(x, y, 4, c); }, hx, hy, HUD_AMBER);
    if (homeLX > -9999) drawTextShadow(canvas, homeLX, homeLY, "HOME", HUD_AMBER, HUD_SHADOW);
    // the remote target and the ship's line to it; in a flight the line flown, the ship along it; G-06: the cursor's line before a plot
    if (ship.mode == ShipState::VIMANA) {
        const Vec3 a = ship.flightFrom / SECTOR_KM, b = ship.flightTo / SECTOR_KM;
        segment(a.x, a.z, b.x, b.z, HUD_CYAN, false);
        int x, y; if (chartProject(b.x, b.z, x, y)) drawRectRGB(canvas, x - 3, y - 3, x + 3, y + 3, HUD_CYAN);
    } else {
        if (away && !plotted) segment(ss.x, ss.z, cur.x, cur.z, rgb(150, 150, 150), true);
        if (ship.hasRemote) {
            const Vec3 r = ship.remote.pos / SECTOR_KM;
            segment(ss.x, ss.z, r.x, r.z, HUD_CYAN, true);
            int x, y; if (chartProject(r.x, r.z, x, y)) drawRectRGB(canvas, x - 3, y - 3, x + 3, y + 3, HUD_CYAN);
        }
    }
    // the ship: a crosshair open in the middle (the star under it shows), YOU beside it; off the face an arrow at the edge
    if (shipIn) {
        shadowed(cross, yx, yy, HUD_WHITE);
        if (youLX > -9999) drawTextShadow(canvas, youLX, youLY, "YOU", HUD_WHITE, HUD_SHADOW);
        gchart.youX = yx; gchart.youY = yy;
    } else {
        const int ex = clampi(yx, 4, UW - 5), ey = clampi(yy, FY0 + 4, FY0 + FH - 5);
        const bool side = yx < 0 || yx >= UW;
        const char arrow[2] = {side ? (yx < 0 ? CH_LEFT : CH_RIGHT) : (yy < FY0 ? CH_UP : CH_DOWN), 0};
        const int ax = clampi(ex - 2, 1, UW - 7), ay = clampi(ey - 3, FY0 + 1, FY0 + FH - 8);
        drawTextShadow(canvas, ax, ay, arrow, HUD_WHITE, HUD_SHADOW);
        const int lx = ax + 8 + textWidth("YOU") > UW - 2 ? ax - 4 - textWidth("YOU") : ax + 8;
        drawTextShadow(canvas, lx, ay, "YOU", HUD_WHITE, HUD_SHADOW);
        gchart.youX = gchart.youY = -9999;
    }
    // G-06: the cursor, and what the explorer has under it
    gchart.curPX = gchart.curPY = -9999;
    if (curIn) {
        shadowed(corners, ccx, ccy, HUD_WHITE);
        dot(ccx, ccy, HUD_WHITE);
        if (thereLX > -9999) drawTextShadow(canvas, thereLX, thereLY, there.label.c_str(), there.col, HUD_SHADOW);
        gchart.curPX = ccx; gchart.curPY = ccy;
    }
    // the strip: the ship's height and home's, the cursor's
    {
        const double dyPx = 2 * half / EH;
        auto stripAt = [&](double x, double y, int& px, int& py) { px = (int)std::floor(x / s - (double)I + UW / 2); py = EY0 + (int)std::floor((half - y) / dyPx); return px >= 0 && px < UW && py >= EY0 && py < EY0 + EH; };
        auto sdot = [&](int x, int y, uint32_t c) { if (x >= 0 && x < UW && y >= EY0 && y < EY0 + EH) fillRectRGB(canvas, x, y, x, y, c); };
        int x, y;
        if (!guide.home.empty()) { const Vec3 h = sectorCentre(guide.home); if (stripAt(h.x, h.y, x, y)) { sdot(x - 1, y, HUD_AMBER); sdot(x + 1, y, HUD_AMBER); sdot(x, y - 1, HUD_AMBER); sdot(x, y + 1, HUD_AMBER); } }
        auto sline = [&](int ax, int ay, int bx, int by, uint32_t c, bool dotted) {
            const int n = std::max(std::abs(bx - ax), std::abs(by - ay));
            for (int k = 0; k <= n; k += dotted ? 2 : 1) sdot(ax + (n ? (bx - ax) * k / n : 0), ay + (n ? (by - ay) * k / n : 0), c);
        };
        int sx = 0, sy = 0; const bool shipOnStrip = stripAt(ss.x, ss.y, sx, sy);
        if (ship.mode == ShipState::VIMANA) {   // the line flown, its end
            const Vec3 a = ship.flightFrom / SECTOR_KM, b = ship.flightTo / SECTOR_KM; int ax, ay, bx, by;
            stripAt(a.x, a.y, ax, ay); stripAt(b.x, b.y, bx, by); sline(ax, ay, bx, by, HUD_CYAN, false);
            for (int d = -1; d <= 1; d++) { sdot(bx + d, by - 1, HUD_CYAN); sdot(bx + d, by + 1, HUD_CYAN); sdot(bx - 1, by + d, HUD_CYAN); sdot(bx + 1, by + d, HUD_CYAN); }
        } else if (ship.hasRemote) {   // the target and the ship's line to it
            const Vec3 r = ship.remote.pos / SECTOR_KM; int rx, ry;
            stripAt(r.x, r.y, rx, ry); sline(sx, sy, rx, ry, HUD_CYAN, true);
            for (int d = -1; d <= 1; d++) { sdot(rx + d, ry - 1, HUD_CYAN); sdot(rx + d, ry + 1, HUD_CYAN); sdot(rx - 1, ry + d, HUD_CYAN); sdot(rx + 1, ry + d, HUD_CYAN); }
        }
        if (away) {
            int cx, cy; stripAt(cur.x, cur.y, cx, cy);
            if (!plotted && ship.mode != ShipState::VIMANA) sline(sx, sy, cx, cy, rgb(150, 150, 150), true);   // the cursor's line seen edge-on
            for (int d = 1; d <= 2; d++) { sdot(cx + d, cy + d, HUD_WHITE); sdot(cx - d, cy + d, HUD_WHITE); sdot(cx + d, cy - d, HUD_WHITE); sdot(cx - d, cy - d, HUD_WHITE); }
        }
        if (shipOnStrip) for (int d = 2; d <= 3; d++) { sdot(sx + d, sy, HUD_WHITE); sdot(sx - d, sy, HUD_WHITE); sdot(sx, sy + d, HUD_WHITE); sdot(sx, sy - d, HUD_WHITE); }
        drawTextShadow(canvas, 4, EY0 + 1, "EDGE-ON", HUD_DIM, HUD_SHADOW);
        const std::string tall = thousands(half) + " LY";   // the strip's top over the plane
        drawTextShadow(canvas, UW - 4 - textWidth(tall.c_str()) - 6, EY0 + 1, (std::string(1, CH_UP) + tall).c_str(), HUD_DIM, HUD_SHADOW);
    }
    // the scale bar in the face's lower left (the longest of 1, 2 and 5 times a power of ten in 70 pixels), the zoom beside it
    {
        double L = 1;
        for (double p = 1; p < 1e6; p *= 10) for (double m : {1.0, 2.0, 5.0}) if (m * p / s <= 70) L = m * p;
        const int len = std::max(2, (int)std::lround(L / s)), bx = 6, by = FY0 + FH - 6;
        const std::string text = thousands(L) + " LY  " + fmt("X%d", 1 << level);
        blendRectRGB(canvas, bx - 3, by - 10, bx + len + 6 + textWidth(text.c_str()) + 2, by + 3, rgb(0, 0, 0), 150);
        drawLineRGB(canvas, bx, by, bx + len, by, HUD_WHITE);
        drawLineRGB(canvas, bx, by - 2, bx, by, HUD_WHITE); drawLineRGB(canvas, bx + len, by - 2, bx + len, by, HUD_WHITE);
        drawText(canvas, bx + len + 5, by - 6, text.c_str(), HUD_WHITE);
    }
    // the place: the ship's (the region, how far from the core, how far over or under the plane); G-06: the cursor's (the region, the
    // stars there, its height), or the plotted star's
    {
        auto trimPlane = [](std::string t, size_t max) {   // " OF THE PLANE" goes first
            const std::string tail = " OF THE PLANE";
            if (t.size() > max && t.size() >= tail.size() && t.compare(t.size() - tail.size(), tail.size(), tail) == 0) t.erase(t.size() - tail.size());
            return t;
        };
        auto heightOf = [](double y) { return std::fabs(y) < 50 ? std::string("IN THE PLANE") : thousands(std::round(std::fabs(y) / 10) * 10) + (y > 0 ? " LY NORTH OF THE PLANE" : " LY SOUTH OF THE PLANE"); };
        const std::string region = REGION_NAMES[galaxyRegion((int64_t)std::floor(cur.x), (int64_t)std::floor(cur.y), (int64_t)std::floor(cur.z))];
        drawText(canvas, 6, 4, "THE GALAXY", HUD_AMBER);
        drawText(canvas, UW - 6 - textWidth(region.c_str()), 4, region.c_str(), HUD_WHITE);
        std::string line; uint32_t col = HUD_GREEN;
        if (!away) {
            const GalaxyTerms g = galaxyTerms(ss.x, ss.y, ss.z);
            line = (g.r < 100 ? std::string("AT THE CORE") : thousands(std::round(g.r / 10) * 10) + " LY FROM THE CORE") + ", " + heightOf(ss.y);
        } else if (plotted) {
            const Vec3 r = ship.remote.pos / SECTOR_KM;
            const std::string who = starNamed(ship.remote) ? trunc(upper(starNameOf(ship.remote)), 14) : std::string("A STAR");
            const double off = length(r - cur);
            line = jumpPlot.shortLy > 0 ? fmt("PLOTTED: %s %s LY SHORT, THE VOID", who.c_str(), thousands(off).c_str())
                                        : fmt("PLOTTED: %s %s LY FROM THE CURSOR", who.c_str(), off < 9.5 ? fmt("%.0f", std::max(1.0, off)).c_str() : thousands(off).c_str());
            col = HUD_CYAN;
        } else {
            const double d = galaxyDensity(cur.x, cur.y, cur.z), n = d > 0 ? 1.0 / d : 1e300;
            std::string stars;
            if (n > 1e6) stars = "NO STARS";
            else if (n < 1.5) stars = "A STAR IN EVERY SECTOR";
            else {
                const double mag = std::pow(10.0, std::floor(std::log10(n)) - 1), r = n < 100 ? std::round(n) : std::round(n / mag) * mag;   // two figures
                stars = "ONE STAR IN " + thousands(r) + " SECTORS";
            }
            line = trimPlane(stars + ", " + heightOf(cur.y), 52);
            col = HUD_WHITE;
        }
        drawText(canvas, 6, 13, line.c_str(), col);
    }
    // the jump's line (G-06), else the legend
    if (away) {
        const Vec3 to = plotted ? ship.remote.pos / SECTOR_KM : cur;
        const double ly = length(to - ss);
        const std::string way = ly < 0.5 ? std::string("HERE") : galacticHeading(ss, (to - ss) / ly);
        const std::string line = fmt("%s LY %s - %.0f S BY VIMANA", thousands(ly).c_str(), way.c_str(), vimanaSeconds(ly));
        drawTextCentered(canvas, UW / 2, UH - 23, line.c_str(), plotted ? HUD_CYAN : HUD_WHITE);
    } else {
        const char* items[] = {"YOU", "HOME", "VISITED", "SIGNAL", "TARGET", "MARK"};
        const int N = 6;
        int total = 0;
        for (const char* t : items) total += 9 + textWidth(t) + 10;
        int x = (UW - total + 10) / 2;
        const int y = UH - 23;
        for (int k = 0; k < N; k++) {
            const int gx = x + 3, gy = y + 3;
            auto px = [&](int ox, int oy, uint32_t c) { fillRectRGB(canvas, gx + ox, gy + oy, gx + ox, gy + oy, c); };
            if (k == 0) for (int d = 2; d <= 3; d++) { px(d, 0, HUD_WHITE); px(-d, 0, HUD_WHITE); px(0, d, HUD_WHITE); px(0, -d, HUD_WHITE); }
            if (k == 1) for (int d = 0; d <= 3; d++) { px(-3 + d, -d, HUD_AMBER); px(3 - d, -d, HUD_AMBER); px(-3 + d, d, HUD_AMBER); px(3 - d, d, HUD_AMBER); }
            if (k == 2) px(0, 0, HUD_GREEN);
            if (k == 3) for (int a = 0; a < 12; a++) px((int)std::lround(3 * std::cos(a * TAU / 12)), (int)std::lround(3 * std::sin(a * TAU / 12)), HUD_RED);
            if (k == 4) drawRectRGB(canvas, gx - 3, gy - 3, gx + 3, gy + 3, HUD_CYAN);
            if (k == 5) { for (int d = 0; d <= 5; d++) px(-1, 3 - d, HUD_GREEN); for (int d = 0; d < 3; d++) px(d, -2, HUD_GREEN); px(0, -1, HUD_GREEN); px(1, -1, HUD_GREEN); px(0, 0, HUD_GREEN); }
            drawText(canvas, x + 9, y, items[k], HUD_DIM);
            x += 9 + textWidth(items[k]) + 10;
        }
    }
    // the keys of the moment (the status over them while it shows)
    if (!drawMapStatus()) {
        const char* keys = ship.mode == ShipState::VIMANA ? "ARROWS MOVE  WHEEL ZOOM  N STOPS  L MARK" : "ARROWS MOVE  WHEEL ZOOM  N STOPS  TAB NEIGHBOURHOOD";
        if (plotted) keys = "ENTER FLY  TAB LOOK THERE  ESC KEEP THE TARGET";
        else if (away && galaxyMarkNear(cur.x, cur.z, 4) >= 0) keys = "ENTER PLOT  L RENAME  DEL TWICE REMOVES  C YOU";
        else if (away) keys = ship.mode == ShipState::VIMANA ? "N STOPS  L MARK  PGUP/PGDN HEIGHT  C YOU" : "ENTER PLOT  N STOPS  L MARK  PGUP/PGDN HEIGHT  C YOU";
        drawTextCentered(canvas, UW / 2, UH - 12, keys, HUD_DIM);
    }
}
