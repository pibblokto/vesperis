// Z-04: a slide made from a vial (life/slides.h): the mount (its preparation, each species' tile and cells, the first look), what its events
// make of it at a time (the stage's warmth eased to the warmth it is set to, the magnet's field, the spot, the food) and where each copy of a cell
// is then and what it does: drawn to the spot or the food it turns to, out of the spot (a dark-dweller: R-438), round lit light-feeders, along
// the field with a U-turn at each turn of it, at the pace its warmth gives it (still out of its range), stressed past its top into a spore, a
// cyst or its lysis
#include "life/slides.h"
#include "life/cultures.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

const char* const SLIDE_DO_NAMES[SD_COUNT] = {"LIGHT", "MAGNET", "WARM", "FOOD"};

namespace {
constexpr uint64_t SALT_MOUNT = 0x5D1DE00000000001ULL;   // a mount's cells: their places, depths, headings and seeds
constexpr uint64_t SALT_COPY = 0x5D1DE00000000002ULL;    // a copy's own: its seed, its depth, its target in a spot, its delay
constexpr double DROP_UM = 20, SECTION_UM = 30;   // a wet mount's depth under its cover glass; a thin section's thickness
constexpr double COVER = 1.0 / 16;                // the most crowded species' cover of the glass after the preparation (4 bodies apart)
constexpr double COVER_MAX = 1.0 / 8;             // no species covers more: the big ones (filaments, colonies) a few in a field
constexpr double PREP_BODY = 20;                  // a body over this counts as this for the preparation (diluted for the cells, not the filaments)
constexpr double WARM_TAU = 12;                   // the stage eased to the warmth it is set to with a time constant of 12 s
constexpr double STRESS_C = 2;                    // over its range's top by this, a species' cells are stressed
constexpr double TURN_S = 1.6;                    // a magnetotactic cell's U-turn when the field is turned half round
constexpr double EASE_S = 2;                      // a cell's wander narrowing as it is drawn (and widening again as it is let go)
constexpr double TILE_MAX = 1500;                 // the longest tile (um): a rare species over-represented to a cell in it (found by a look round at x40)
constexpr int TILE_CELLS = 160;                   // the most cells a species keeps in its tile
constexpr int KIND_SPECIMENS = 3000;              // what a view draws of a species at most (the low powers' specks: past it, a share of its copies)
constexpr double PACE_MIN = 0.2, PACE_MAX = 2;    // R-438: a kind's pace in its range, against its pace at the vial's warmth (its species' speed there)
constexpr double PACE_STEP = 0.25;                // R-438: the paced clock's table through an easing (s)
constexpr double SCENT_D = 1000;                  // R-437: a small molecule's diffusion in water (um^2/s, an amino acid's, the drop's stirring with it): its front at sqrt(4Dt)
constexpr double LIT_S = 10;                      // R-438: lit light-feeders' gift drawing the eaters from this long after the light (Engelmann 1882)
constexpr double CROWD_FILL = 0.55;               // R-438: a crowd's packing into the disc it gathers in (through the depths it keeps)

inline double h01(uint64_t h) { return unitFromHash(mix64(h)); }
double bodyAcross(const Morph& m) { return std::max(0.3, 2 * m.bodyUm()); }
double ease(double x) { x = clampd(x, 0, 1); return x * x * (3 - 2 * x); }

// the stage's warmth: the sample's, eased toward the warmth it is set to (R-438: any its drop stays liquid at, the vial's again by Shift+H);
// `crossing` the moment within a stretch it passes `c` going up (-1 none)
struct Warmth {
    double T, A, from = 0;
    explicit Warmth(double c0) : T(c0), A(c0) {}
    double crossing(double until, double c) const {   // within [from, until): when the eased warmth passes `c` going up, else -1
        if (A <= c || T >= c) return -1;
        const double tc = from - WARM_TAU * std::log((c - A) / (T - A));
        return tc < until ? tc : -1;
    }
    void to(double until) { T = A + (T - A) * std::exp(-(until - from) / WARM_TAU); from = until; }
    void set(double at, double c) { to(at); A = c; }
};
// the first moment the stage's warmth passes `c` going up (1e30 never); over it already (a sample too warm for a kind, waiting), the moment it is
// set warmer still
double warmthReaches(const SlideMount& M, const std::vector<SlideEvent>& ev, double c) {
    Warmth W(M.stageC);
    for (const SlideEvent& e : ev) {
        if (e.kind != SD_WARM) continue;
        const double tc = W.crossing(e.t, c);
        if (tc >= 0) return tc;
        W.to(e.t);
        if (W.T >= c && e.x > W.T) return e.t;
        W.set(e.t, e.x);
    }
    const double tc = W.crossing(1e30, c);
    return tc >= 0 ? tc : 1e30;
}
double warmthAt(const SlideMount& M, const std::vector<SlideEvent>& ev, double t) {
    Warmth W(M.stageC);
    for (const SlideEvent& e : ev) { if (e.t > t) break; if (e.kind == SD_WARM) W.set(e.t, e.x); }
    W.to(t);
    return W.T;
}

// R-438: a kind's pace at a warmth: none out of its range (still: torpid in the cold, stopped in the heat), else its cardinal curve's against its
// pace's measure (the vial's warmth: the species' speed is its speed there), PACE_MIN to PACE_MAX
double paceAt(const SlideKind& K, double T) {
    if (!K.lives || T < K.tMin || T > K.tMax) return 0;
    return clampd(cardinalRate(T, K.tMin, K.tOpt, K.tMax) / K.paceRef, PACE_MIN, PACE_MAX);
}

// R-438: a kind's paced clock: the seconds its moves have run at its pace (its runs, its turns, its wander, its rise and fall through the drop go
// by it): the slide's own while the stage keeps the vial's warmth (none for a kind still there); from the stage's first setting a table at
// PACE_STEP through each easing, its pace steady once the warmth is within a hundredth of a degree of the setting
struct PaceClock {
    bool set = false, moves = false;
    double t0 = 0;
    struct Seg { double a = 0, Ta = 0, A = 0, conv = 0, Pconv = 0, f = 0; int i0 = 0, n = 0; };
    std::vector<Seg> segs;
    std::vector<double> tab;
    double at(double t) const {
        if (!set || t <= t0) return moves ? t : 0;
        size_t i = segs.size() - 1;
        while (i > 0 && segs[i].a > t) i--;
        const Seg& s = segs[i];
        if (t >= s.conv || s.n < 2) return s.Pconv + s.f * std::max(0.0, t - s.conv);
        const double u = (t - s.a) / PACE_STEP;
        const int k = std::min((int)u, s.n - 2);
        return tab[s.i0 + k] + (tab[s.i0 + k + 1] - tab[s.i0 + k]) * (u - k);
    }
};
PaceClock paceClock(const SlideMount& M, const SlideKind& K, const std::vector<SlideEvent>& ev, double until) {
    PaceClock C;
    C.moves = K.lives && K.active;
    if (!K.lives) return C;
    for (size_t j = 0; j < ev.size(); j++) {
        const SlideEvent& e = ev[j];
        if (e.t > until) break;
        if (e.kind != SD_WARM) continue;
        double T = M.stageC, P = 0;
        if (!C.set) { C.set = true; C.t0 = e.t; P = C.moves ? e.t : 0; }
        else { const PaceClock::Seg& p = C.segs.back(); T = p.A + (p.Ta - p.A) * std::exp(-(e.t - p.a) / WARM_TAU); P = C.at(e.t); }
        double end = until;   // the next setting, or the frame's time
        for (size_t q = j + 1; q < ev.size(); q++) if (ev[q].kind == SD_WARM) { end = std::min(end, ev[q].t); break; }
        PaceClock::Seg s; s.a = e.t; s.Ta = T; s.A = e.x; s.i0 = (int)C.tab.size();
        double tt = s.a, p = P, Tt = T, f = paceAt(K, T);
        C.tab.push_back(p);
        while (tt < end && std::fabs(Tt - s.A) > 0.01) {
            const double t2 = tt + PACE_STEP, T2 = s.A + (s.Ta - s.A) * std::exp(-(t2 - s.a) / WARM_TAU), f2 = paceAt(K, T2);
            p += 0.5 * (f + f2) * PACE_STEP;
            C.tab.push_back(p);
            tt = t2; Tt = T2; f = f2;
        }
        s.n = (int)C.tab.size() - s.i0; s.conv = tt; s.Pconv = p; s.f = f;
        C.segs.push_back(s);
    }
    return C;
}

// a copy's seed: the mount's cell's own in the first tile, else its tile's
uint64_t copySeed(const SlideCell& c, int64_t ti, int64_t tj) { return ti == 0 && tj == 0 ? c.seed : mix64(c.seed ^ hash2i(ti, tj, SALT_COPY)); }

// a kind's way along the magnet's field by its paced clock: the turns to it (its side its own, the other way the next time: the U-turns'
// sideways steps cancel), its swim along it at its speed; from the magnet's coming till it is taken away (R-436). `phi` its heading then, `x`,
// `y` its place moved, `since` the magnet's last coming (-1 never), `off` its going since (-1 none)
void fieldRun(const SlideKind& K, const PaceClock& C, const std::vector<SlideEvent>& ev, double t, double phi0, double side, double& x, double& y,
              double& phi, double& since, double& off) {
    phi = phi0; since = -1; off = -1;
    bool on = false; double th = 0, last = 0; int turns = 0;
    auto run = [&](double t0, double t1) {
        double d = wrapAngle(th - phi);
        if (std::fabs(std::fabs(d) - PI) < 1e-3) d = (turns & 1 ? -side : side) * PI;
        const double dur = C.at(t1) - C.at(t0);
        const double tau = TURN_S * std::fabs(d) / PI, w = tau > 1e-9 ? d / tau : 0, a = std::min(dur, tau);
        if (a > 0 && std::fabs(w) > 1e-9) { x += K.speed * (std::sin(phi + w * a) - std::sin(phi)) / w; y += K.speed * (std::cos(phi) - std::cos(phi + w * a)) / w; }
        phi += w * a;
        const double b = dur - a;
        if (b > 0) { phi = th; x += K.speed * std::cos(th) * b; y += K.speed * std::sin(th) * b; }
    };
    for (const SlideEvent& e : ev) {
        if (e.t > t) break;
        if (e.kind != SD_MAGNET) continue;
        if (on) { run(last, e.t); turns++; }
        if (!e.on) { on = false; off = e.t; continue; }
        if (!on) { since = e.t; off = -1; }
        on = true; th = e.x; last = e.t;
    }
    if (on) run(last, t);
}

// how far a cell senses a spot or a drop of a radius: four radii, or what it swims in 25 s, to twelve
double reachOf(const SlideKind& K, double r) { return std::max(r * 4, std::min(K.speed * 25, r * 12)); }

// R-438: the disc a kind's crowd packs into round a spot or a drop of a radius: eight tenths of it, or (a bigger crowd: every cell within its
// reach answers) what holds them at CROWD_FILL through the depths it keeps, to twice its radius (past it a crowd piles up: it never spreads
// outward from the stimulus)
double crowdRadius(const SlideKind& K, double r) {
    const double reach = reachOf(K, r), n = K.n / std::max(1e-9, K.tile * K.tile) * PI * reach * reach;
    const double layers = clampd((K.z1 - K.z0) / std::max(0.3, K.body), 1, 6);
    return std::min(2 * r, std::max(0.8 * r, K.body * std::sqrt(n / (PI * CROWD_FILL * layers))));
}

// R-438: a kind's copies whose places lie within a radius of a point (a spot's light-feeders: what it lights)
int copiesWithin(const SlideKind& K, const SlideMount& M, double x, double y, double r) {
    int n = 0;
    const int64_t i0 = (int64_t)std::floor((x - r) / K.tile) - 1, i1 = (int64_t)std::floor((x + r) / K.tile), j0 = (int64_t)std::floor((y - r) / K.tile) - 1,
                  j1 = (int64_t)std::floor((y + r) / K.tile);
    for (int64_t tj = j0; tj <= j1; tj++)
        for (int64_t ti = i0; ti <= i1; ti++)
            for (int ci = K.first; ci < K.first + K.n; ci++) n += std::hypot(M.cells[ci].x + ti * K.tile - x, M.cells[ci].y + tj * K.tile - y) <= r;
    return n;
}

// what a kind's cells share at a time: its paced clock and its pace now (0: still at this warmth), when the stage's warmth stresses them, the
// field's run of a cell starting along it (the copies' range), the events it answers (an eater's: the lit light-feeders' gift among them, `lit`)
struct KindNow {
    PaceClock clock;
    double paceNow = 0;
    double stressAt = 1e30; double fx = 0, fy = 0, fieldBound = 0, pull = 0;
    std::vector<SlideEvent> ev;
    std::vector<char> lit;
};
KindNow kindNow(const SlideMount& M, const SlideKind& K, const std::vector<SlideEvent>& ev, double t) {
    KindNow N;
    N.clock = paceClock(M, K, ev, t);
    N.paceNow = N.clock.set ? paceAt(K, warmthAt(M, ev, t)) : (K.active ? 1 : 0);
    N.ev = ev; N.lit.assign(ev.size(), 0);
    if (K.lives && K.speed > 0 && (K.taxis & TX_FOOD) && !(K.taxis & TX_FIELD)) {   // R-438: a spot on live light-feeders: their gift draws the eaters
        std::vector<std::pair<SlideEvent, char>> all;
        for (const SlideEvent& e : ev) all.push_back({e, 0});
        for (size_t i = 0; i < ev.size(); i++) {
            const SlideEvent& e = ev[i];
            if (e.kind != SD_LIGHT || !e.on || e.t > t) continue;
            double ends = 1e30;
            for (size_t q = i + 1; q < ev.size(); q++) if (ev[q].kind == SD_LIGHT) { ends = ev[q].t; break; }
            if (ends <= e.t + LIT_S) continue;
            const double T = warmthAt(M, ev, e.t + LIT_S);
            int lit = 0;   // the light-feeders alive at its warmth the spot lights
            for (const SlideKind& L : M.kinds)
                if (L.lightFeeder && L.lives && (N.clock.set ? paceAt(L, T) > 0 : L.active) && lit == 0) lit += copiesWithin(L, M, e.x, e.y, e.r);
            if (lit == 0) continue;
            SlideEvent g = e; g.kind = SD_FOOD; g.t = e.t + LIT_S; all.push_back({g, 1});
            if (ends < 1e29) { g.t = ends; g.on = false; all.push_back({g, 1}); }
        }
        std::stable_sort(all.begin(), all.end(), [](const std::pair<SlideEvent, char>& a, const std::pair<SlideEvent, char>& b) { return a.first.t < b.first.t; });
        N.ev.clear(); N.lit.clear();
        for (const auto& p : all) { N.ev.push_back(p.first); N.lit.push_back(p.second); }
    }
    if (K.stress >= 0) N.stressAt = warmthReaches(M, ev, K.tMax + STRESS_C);
    if (K.lives && K.speed > 0 && (K.taxis & TX_FIELD)) {
        double th0 = 0;
        for (const SlideEvent& e : ev) if (e.kind == SD_MAGNET && e.on && e.t <= t) { th0 = e.x; break; }
        double phi, since, off;
        fieldRun(K, N.clock, ev, std::min(t, N.stressAt + 7), th0, 1, N.fx, N.fy, phi, since, off);
        N.fieldBound = K.speed * (N.clock.set ? PACE_MAX : 1) * (TURN_S * 3 + (t > N.stressAt ? 6 : 0));   // the copies' own turns; their stops
    } else if (K.lives && K.speed > 0 && ((K.taxis & (TX_LIGHT | TX_FOOD)) || K.flees))
        for (const SlideEvent& e : N.ev) if (e.t <= t && e.on && (e.kind == SD_LIGHT || e.kind == SD_FOOD)) N.pull = std::max(N.pull, reachOf(K, e.r));
    return N;
}

// R-438: the runs a copy makes to what it answers, by its paced clock: into a spot of light (a light-feeder), out of it (a dark-dweller: from
// within it to past its edge), to a drop of food or to lit light-feeders' gift (an eater), each from when it senses it (the light at once, a
// scent as its front reaches the copy) to a point of the crowd it joins (every cell within its reach answers); the stimulus taken away, moved
// past its reach or rinsed lets it go where it got to. Its drift now (at its pace), how far its wander is narrowed (eased in as it is drawn, out
// as it is let go), what it answers (RS_*)
enum RunSource { RS_NONE = 0, RS_LIGHT, RS_FLEE, RS_FOOD, RS_LIT };
void attractedRun(const SlideKind& K, const KindNow& N, double t, uint64_t seed, double& x, double& y, double& vx, double& vy, double& wander, int& answer) {
    vx = vy = 0; wander = 1; answer = RS_NONE;
    const bool light = (K.taxis & TX_LIGHT) != 0, food = (K.taxis & TX_FOOD) != 0;
    if (!K.lives || K.speed <= 0 || (!light && !food && !K.flees)) return;
    int src = RS_NONE, pend = RS_NONE;
    double tx = 0, ty = 0, from = 0, fx = x, fy = y, drawnAt = -1, letAt = -1, pendT = 0, pendX = 0, pendY = 0;
    auto place = [&](double at) {   // where its run has taken it by `at`
        if (src == RS_NONE) return;
        const double dx = tx - fx, dy = ty - fy, d = std::sqrt(dx * dx + dy * dy), s = std::min(d, K.speed * std::max(0.0, N.clock.at(at) - N.clock.at(from)));
        if (d > 1e-9) { x = fx + dx / d * s; y = fy + dy / d * s; }
    };
    auto start = [&](int what, double at, double px, double py) {
        place(at);
        if (src == RS_NONE && letAt != at) drawnAt = at;   // (a stimulus moved: its narrowing goes on)
        src = what; tx = px; ty = py; from = at; fx = x; fy = y;
    };
    auto letGo = [&](double at) { place(at); fx = x; fy = y; src = RS_NONE; letAt = at; };
    auto due = [&](double until) { if (pend != RS_NONE && pendT <= until) { start(pend, pendT, pendX, pendY); pend = RS_NONE; } };
    auto family = [](int s) { return s == RS_FLEE ? RS_LIGHT : s; };
    for (size_t i = 0; i < N.ev.size(); i++) {
        const SlideEvent& e = N.ev[i];
        if (e.t > t) break;
        const int what = e.kind == SD_LIGHT ? (light ? RS_LIGHT : (K.flees ? RS_FLEE : RS_NONE)) : (e.kind == SD_FOOD && food ? (N.lit[i] ? RS_LIT : RS_FOOD) : RS_NONE);
        if (what == RS_NONE) continue;
        due(e.t);
        if (pend != RS_NONE && family(pend) == family(what)) pend = RS_NONE;   // a scent on its way, of a drop rinsed or put again
        if (src != RS_NONE && family(src) == family(what)) letGo(e.t);       // its run to this stimulus ends (again if it senses the new one)
        if (!e.on) continue;
        place(e.t);
        const uint64_t q = hashCombine(seed ^ SALT_COPY, (uint64_t)(i + 1) * 0x2545F4914F6CDD1DULL);
        const double d = std::hypot(e.x - x, e.y - y);
        if (what == RS_FLEE) {   // within the spot (its wander's reach on it): out past its edge, away from its middle
            if (d > e.r + K.reach) continue;
            const double a = d > 1e-6 ? std::atan2(y - e.y, x - e.x) : h01(q) * TAU, rr = e.r * (1.2 + 0.5 * h01(q + 1)) + K.reach;
            start(RS_FLEE, e.t, e.x + rr * std::cos(a), e.y + rr * std::sin(a));
            continue;
        }
        if (d > reachOf(K, e.r)) continue;
        const double rr = crowdRadius(K, e.r) * std::sqrt(h01(q)), a = h01(q + 1) * TAU, px = e.x + rr * std::cos(a), py = e.y + rr * std::sin(a);
        if (what == RS_LIGHT) { start(RS_LIGHT, e.t, px, py); continue; }
        const double dd = std::max(0.0, d - e.r), at = e.t + dd * dd / (4 * SCENT_D);   // the scent's front reaching it
        if (at <= e.t) start(what, e.t, px, py);
        else { pend = what; pendT = at; pendX = px; pendY = py; }
    }
    due(t);
    if (src != RS_NONE) {
        place(t);
        const double left = std::hypot(tx - x, ty - y), size = std::max(1.0, K.reach * 0.3);
        if (left > 1e-6 && N.paceNow > 0) { vx = (tx - x) / left * K.speed * N.paceNow; vy = (ty - y) / left * K.speed * N.paceNow; }
        const double w = src == RS_FLEE ? 0.5 : 0.25 + 0.1 * clampd(left / (4 * size), 0, 1);
        wander = 1 + (w - 1) * ease((t - drawnAt) / EASE_S);
        answer = src;
    } else if (letAt >= 0) wander = 0.25 + 0.75 * ease((t - letAt) / (EASE_S * 1.5));
}

// a copy's depth: where it lies; a living cell's rising and falling through the drop by its paced clock (a kind still at the vial's warmth from
// where it lies once it moves)
double depthOf(const SlideKind& K, const SlideCell& c, uint64_t cs, bool firstTile, double tp) {
    const double z0 = firstTile ? c.z : K.z0 + (K.z1 - K.z0) * h01(cs ^ 0x2D);
    if (!K.lives || c.form != CF_CELL || K.z1 - K.z0 < 1) return z0;
    const double A = std::min(3.0, (K.z1 - K.z0) * 0.5), w = 0.15 + 0.25 * h01(cs ^ 0x3D), ph = h01(cs ^ 0x4D) * TAU;
    return clampd(z0 + A * (std::sin(w * tp + ph) - (K.active ? 0 : std::sin(ph))), K.z0, K.z1);
}

// a copy as a specimen at a time
void copyAt(const SlideMount& M, const std::vector<Morph>& forms, const std::vector<SlideEvent>& ev, double t, const SlideLook& look, const KindNow& N,
            int ci, int64_t ti, int64_t tj, Specimen& s) {
    const SlideCell& c = M.cells[ci];
    const SlideKind& K = M.kinds[c.kind];
    const Morph& m = forms[K.species];
    const uint64_t cs = copySeed(c, ti, tj);
    s = Specimen();
    s.m = &m; s.form = c.form; s.act = AN_LIVE; s.seed = cs;
    s.x = c.x + ti * K.tile; s.y = c.y + tj * K.tile;
    s.angle = ti == 0 && tj == 0 ? c.angle : h01(cs ^ 0xA9) * TAU;
    const bool moves = K.lives && c.form != CF_COLONY && m.motion != MO_NONE;   // its runs keep where they took it, moving now or still
    s.still = !moves || N.paceNow <= 0;
    const bool stressable = K.stress >= 0 && N.stressAt < 1e29 && (c.form == CF_CELL || c.form == CF_DIVIDING);
    const double tStress = stressable ? N.stressAt + 2 + 10 * h01(cs ^ 0x57E55) : 1e30;
    const double tm = std::min(t, tStress);   // a stressed cell's moves end where its stress takes it
    if (N.clock.set) s.clock = N.clock.at(tm);
    if (moves && (K.taxis & TX_FIELD)) {
        double phi, since, off;
        fieldRun(K, N.clock, ev, tm, s.angle, h01(cs ^ 0x51DE) < 0.5 ? -1 : 1, s.x, s.y, phi, since, off);
        if (since >= 0) {
            if (off >= since) s.wander = 0.3 + 0.7 * ease((tm - off) / (EASE_S * 1.5));   // R-436: the magnet taken away, its stream let go
            else {
                s.wander = 1 - 0.7 * ease((tm - since) / EASE_S);
                if (tm == t && N.paceNow > 0) { s.vx = K.speed * N.paceNow * std::cos(phi); s.vy = K.speed * N.paceNow * std::sin(phi); }
            }
            if (m.motion == MO_GLIDING || m.motion == MO_CORKSCREW || m.motion == MO_TWITCHING) s.angle = phi;
        }
    } else if (moves) {
        double vx, vy, w; int answer;
        attractedRun(K, N, tm, cs, s.x, s.y, vx, vy, w, answer);
        s.wander = w;
        if (tm == t) { s.vx = vx; s.vy = vy; }
        if ((vx != 0 || vy != 0) && (m.motion == MO_GLIDING || m.motion == MO_CORKSCREW || m.motion == MO_TWITCHING)) s.angle = std::atan2(vy, vx);
    }
    if (t >= tStress) {   // stressed: its act from where it was, then its last form
        double x, y, a;
        specimenPlace(s, tm, x, y, a);
        s.x = x; s.y = y; s.angle = a; s.vx = s.vy = 0; s.clock = -1;
        if (t < tStress + actSeconds(m, K.stress)) { s.act = K.stress; s.t0 = tStress; s.form = CF_CELL; }
        else { s.form = K.stressForm; s.still = true; s.wander = 1; }
    }
    // its depth: a swimmer's rising and falling through the drop, the blur out of focus (the electron's and the section's in focus throughout)
    const double z = depthOf(K, c, cs, ti == 0 && tj == 0, N.clock.at(t));
    if (look.seeing != WS_ELECTRON && look.seeing != WS_SECTION) {
        const double off = std::fabs(z - look.focus) - depthOfField(look.mag) * 0.5;
        if (off > 0) s.blurUm = std::min(off * 0.45, 10 * umPerPxAt(look.mag));
    }
}
}   // namespace

double depthOfField(double mag) { return clampd(0.6 + 800.0 / std::max(1.0, mag), 0.7, 25); }

SlideMount mountOf(const Biota& B, const std::vector<Morph>& forms, const Sample& S, const Microhabitat& h) {
    SlideMount M;
    M.seed = mix64(h.place ^ SALT_MOUNT);
    M.stageC = h.meanC;
    M.section = h.niche == NI_ROCK;
    M.depthUm = M.section ? SECTION_UM : DROP_UM;
    auto usable = [&](const Held& H) { return H.species >= 0 && H.species < (int)forms.size() && H.species < (int)B.species.size(); };
    // the preparation: the most crowded species' cover of the glass about a sixty-fourth (a section's taken where they live; its fossils as
    // they lie)
    double cover = 0;
    for (const Held& H : S.held) {
        if (!usable(H) || H.state == SS_FOSSIL) continue;
        const double b = std::min(PREP_BODY, bodyAcross(forms[H.species]));
        cover = std::max(cover, H.cells * M.depthUm * 1e-12 * b * b);
    }
    M.prep = cover <= 0 ? 0 : (int)clampd(std::round(std::log10(COVER / cover)), -6, 6);
    const double k = std::pow(10.0, M.prep);
    int first = -1; double firstCover = -1;
    for (size_t hi = 0; hi < S.held.size(); hi++) {
        const Held& H = S.held[hi];
        if (!usable(H)) continue;
        const Microbe& sp = B.species[H.species];
        const Morph& m = forms[H.species];
        SlideKind K;
        K.species = H.species; K.held = (int)hi;
        K.form = H.state == SS_FOSSIL ? CF_FOSSIL : (H.state == SS_DORMANT ? (actSuits(m, AN_ENCYST) && !smallCellPlan(m.plan) ? CF_CYST : CF_SPORE) : CF_CELL);
        if (H.form >= 0 && H.form < CF_COUNT) K.form = H.form;   // Z-05: a culture's lysed, its slug, its stalks
        // R-438: it lives on the slide where its drop suits it (a culture's verdict, else the vial's salt) and moves where the stage's warmth is in
        // its range (the vial's to begin: a sample's activity); its pace measured at the vial's warmth; the dark's swimmers, the light-feeders
        const bool saltSuits = !(h.salt < sp.saltLo || h.salt > sp.saltHi);
        K.lives = H.state == SS_LIVING && K.form == CF_CELL && (H.alive >= 0 ? H.alive == 1 : saltSuits);
        K.tMin = sp.tMin; K.tOpt = sp.tOpt; K.tMax = sp.tMax;
        K.active = K.lives && h.meanC >= sp.tMin && h.meanC <= sp.tMax;
        K.paceRef = K.active ? std::max(0.02, cardinalRate(h.meanC, sp.tMin, sp.tOpt, sp.tMax)) : 1;
        K.taxis = K.lives && m.motion != MO_NONE ? m.taxis : 0;
        K.speed = m.speed;
        K.lightFeeder = sp.way == LW_LIGHT;
        K.flees = K.lives && m.motion != MO_NONE && !K.lightFeeder &&
                  (sp.flees >= 0 ? sp.flees == 1
                                 : !(m.taxis & (TX_LIGHT | TX_FIELD)) &&
                                       (sp.niche == NI_SOIL || sp.niche == NI_SILT || sp.niche == NI_VENT || sp.niche == NI_ROCK || sp.niche == NI_PLUME));
        if (K.lives) {   // the stage's stress past its top: a spore former's spores, a large cell's cyst, else its lysis (still or moving)
            if (actSuits(m, AN_SPORULATE)) { K.stress = AN_SPORULATE; K.stressForm = CF_SPORE; }
            else if (actSuits(m, AN_ENCYST)) { K.stress = AN_ENCYST; K.stressForm = CF_CYST; }
            else if (actSuits(m, AN_LYSE)) { K.stress = AN_LYSE; K.stressForm = CF_GHOST; }
        }
        // its depths: the swimmers throughout the drop, the gas-filled under the cover glass, the rest settled on the glass; a cell lies flat (its
        // thickness its width's), one thicker than the drop pressed under the cover
        const double b = bodyAcross(m), dz = M.depthUm, th = std::min(b, std::max(0.3, m.widthUm * (colonyPlan(m.plan) ? 3.0 : 1.0)));
        if (M.section) { K.z0 = 0; K.z1 = dz; }
        else if (th >= dz * 0.8) { K.z0 = K.z1 = dz * 0.5; }
        else if (K.active && m.motion != MO_NONE) { K.z0 = th * 0.5; K.z1 = dz - th * 0.5; }
        else if (m.gas > 0.2 && K.form == CF_CELL) { K.z0 = dz - th * 0.5 - 2; K.z1 = dz - th * 0.5; }
        else { K.z0 = th * 0.5; K.z1 = th * 0.5 + std::min(2.0, th); }
        K.z0 = clampd(K.z0, 0, dz); K.z1 = clampd(K.z1, K.z0, dz);
        const double rho = std::min(H.cells * k * M.depthUm * 1e-12, COVER_MAX / (b * b));   // its cells a square micrometre of the glass
        if (H.state == SS_FOSSIL) { K.n = std::max(1, (int)H.cells); K.tile = 1500; }   // a section's few
        else {   // a hundred bodies' tile, its cells at its density (a few hundred at most); a sparse species' tile at the longest, a cell in it at least
            K.n = (int)clampd(std::round(rho * 1e4 * b * b), 1, TILE_CELLS);
            K.tile = std::sqrt(K.n / std::max(rho, 1e-30));
            if (K.tile > TILE_MAX) { K.tile = TILE_MAX; K.n = (int)clampd(std::round(rho * TILE_MAX * TILE_MAX), 1, TILE_CELLS); }
            K.tile = std::max(K.tile, 2 * b);
        }
        K.body = b;
        {   // the radius that holds one: its cell living, a colony of it, its stress's act
            Specimen one; one.m = &m; one.form = K.form; one.seed = m.seed;
            K.reach = specimenExtentUm(one, 0);
            if ((sp.cycle & LC_COLONY) && smallCellPlan(m.plan)) { one.form = CF_COLONY; K.reach = std::max(K.reach, specimenExtentUm(one, 0)); one.form = K.form; }
            if (K.stress >= 0) { one.act = K.stress; K.reach = std::max(K.reach, specimenExtentUm(one, 0)); }
        }
        K.first = (int)M.cells.size();
        for (int i = 0; i < K.n; i++) {
            const uint64_t q = hashCombine(M.seed, (uint64_t)hi * 0x9E3779B97F4A7C15ULL + (uint64_t)i * 0xBF58476D1CE4E5B9ULL + 1);
            SlideCell c;
            c.kind = (int)M.kinds.size();
            c.x = h01(q) * K.tile; c.y = h01(q + 1) * K.tile; c.angle = h01(q + 2) * TAU; c.seed = mix64(q + 3);
            c.z = K.z0 + (K.z1 - K.z0) * h01(q + 4);
            c.form = K.form;
            if (K.form == CF_CELL && K.active) {   // some found in a microcolony (a colony former's), a few dividing
                const double u = h01(q + 5);
                if ((sp.cycle & LC_COLONY) && smallCellPlan(m.plan) && u < 0.2) c.form = CF_COLONY;
                else if (actSuits(m, AN_DIVIDE) && u > (H.dividing >= 0 ? 1 - H.dividing : 0.93)) c.form = CF_DIVIDING;
            }
            M.cells.push_back(c);
        }
        // the first look: a section's first fossil, else the most crowded species
        const double cov = H.state == SS_FOSSIL ? (M.section ? 1e9 : -1) : rho * b * b;
        if (cov > firstCover) { firstCover = cov; first = (int)M.kinds.size(); }
        M.kinds.push_back(K);
    }
    if (first >= 0) {
        const SlideKind& K = M.kinds[first];
        const SlideCell& c = M.cells[K.first];
        const Morph& m = forms[K.species];
        M.cx = c.x; M.cy = c.y; M.focus = c.z;
        M.seeing = WS_PHASE;
        if (K.form == CF_FOSSIL) M.seeing = M.section ? WS_SECTION : WS_BRIGHT;
        else for (int w : seeingsFor(m)) if (w != WS_ELECTRON && w != WS_SECTION) { M.seeing = w; break; }
        M.mag = magnificationFor(m, M.seeing == WS_SECTION ? WS_BRIGHT : M.seeing, 0.15);
    } else { M.seeing = M.section ? WS_SECTION : WS_BRIGHT; M.mag = 100; M.focus = 1; }
    return M;
}

SlideState slideStateAt(const SlideMount& M, const std::vector<SlideEvent>& ev, double t) {
    SlideState S;
    Warmth W(M.stageC);
    for (const SlideEvent& e : ev) {
        if (e.t > t) break;
        switch (e.kind) {
            case SD_LIGHT: S.spot = e.on; S.spotX = e.x; S.spotY = e.y; S.spotR = e.r; S.spotT = e.t; break;
            case SD_MAGNET: S.magnet = e.on; S.field = e.x; break;
            case SD_WARM: W.set(e.t, e.x); break;
            case SD_FOOD: S.food = e.on; S.foodX = e.x; S.foodY = e.y; S.foodR = e.r; S.foodT = e.t; break;
            default: break;
        }
    }
    W.to(t);
    S.stageC = W.T; S.setC = W.A;
    return S;
}

double mountDepth(const SlideMount& M, const SlideRef& r, double t, const std::vector<SlideEvent>* ev) {
    if (r.cell < 0 || r.cell >= (int)M.cells.size()) return 0;
    const SlideCell& c = M.cells[r.cell];
    const SlideKind& K = M.kinds[c.kind];
    const double tp = ev ? paceClock(M, K, *ev, t).at(t) : (K.active ? t : 0);
    return depthOf(K, c, copySeed(c, r.ti, r.tj), r.ti == 0 && r.tj == 0, tp);
}

void mountSpecimens(const SlideMount& M, const std::vector<Morph>& forms, const std::vector<SlideEvent>& ev, double t, const SlideLook& look,
                    std::vector<Specimen>& out, std::vector<SlideRef>* refs) {
    out.clear();
    if (refs) refs->clear();
    for (const SlideKind& K : M.kinds) {
        if (K.species < 0 || K.species >= (int)forms.size() || K.n <= 0) continue;
        const KindNow N = kindNow(M, K, ev, t);
        int drawn = 0;
        // the tiles whose copies can be in view: the view moved back by the field's run, widened by a copy's reach and its pull
        const double wx = look.halfW + K.reach + N.fieldBound + N.pull, wy = look.halfH + K.reach + N.fieldBound + N.pull;
        const double cx = look.cx - N.fx, cy = look.cy - N.fy;
        const int64_t i0 = (int64_t)std::floor((cx - wx) / K.tile) - 1, i1 = (int64_t)std::floor((cx + wx) / K.tile);
        const int64_t j0 = (int64_t)std::floor((cy - wy) / K.tile) - 1, j1 = (int64_t)std::floor((cy + wy) / K.tile);
        // more than a view draws (a crowd of small cells at a low power): an even share of its copies, by their own hashes
        const double expect = K.n * (2 * wx) * (2 * wy) / (K.tile * K.tile), share = std::min(1.0, KIND_SPECIMENS / std::max(1.0, expect));
        for (int64_t tj = j0; tj <= j1 && drawn < KIND_SPECIMENS; tj++)
            for (int64_t ti = i0; ti <= i1 && drawn < KIND_SPECIMENS; ti++)
                for (int ci = K.first; ci < K.first + K.n && drawn < KIND_SPECIMENS; ci++) {
                    const SlideCell& c = M.cells[ci];
                    const double bx = c.x + ti * K.tile + N.fx, by = c.y + tj * K.tile + N.fy;
                    if (std::fabs(bx - look.cx) > wx || std::fabs(by - look.cy) > wy) continue;
                    if (share < 1 && h01(copySeed(c, ti, tj) ^ 0x5A4E) >= share) continue;
                    Specimen s;
                    copyAt(M, forms, ev, t, look, N, ci, ti, tj, s);
                    double px, py, pa;
                    specimenPlace(s, t, px, py, pa);
                    const double ext = s.act == AN_LIVE && s.form == CF_CELL ? s.m->extentUm() : specimenExtentUm(s, t);   // its own, its wander placed
                    if (std::fabs(px - look.cx) > look.halfW + ext || std::fabs(py - look.cy) > look.halfH + ext) continue;
                    out.push_back(s);
                    if (refs) refs->push_back(SlideRef{ci, ti, tj, px, py});
                    drawn++;
                }
    }
}

bool mountSpecimen(const SlideMount& M, const std::vector<Morph>& forms, const std::vector<SlideEvent>& ev, double t, const SlideLook& look, const SlideRef& r,
                   Specimen& out) {
    if (r.cell < 0 || r.cell >= (int)M.cells.size()) return false;
    const SlideKind& K = M.kinds[M.cells[r.cell].kind];
    if (K.species < 0 || K.species >= (int)forms.size()) return false;
    copyAt(M, forms, ev, t, look, kindNow(M, K, ev, t), r.cell, r.ti, r.tj, out);
    return true;
}

double slideScentUm(double seconds) { return seconds > 0 ? std::sqrt(4 * SCENT_D * seconds) : 0; }

uint32_t slideAnswer(const SlideMount& M, const std::vector<SlideEvent>& ev, int kind, double t) {
    if (kind < 0 || kind >= (int)M.kinds.size()) return 0;
    const SlideKind& K = M.kinds[kind];
    if (!K.lives) return 0;
    const KindNow N = kindNow(M, K, ev, t);
    const SlideState S = slideStateAt(M, ev, t);
    uint32_t a = 0;
    const bool stressed = K.stress >= 0 && t >= N.stressAt + 2;
    if (stressed) a |= SA_STRESSED;
    if (K.speed <= 0) return a;   // what does not swim shows no moves to tell
    if (N.paceNow <= 0) { if (!stressed) a |= S.stageC < K.tMin ? SA_COLD : (S.stageC > K.tMax ? SA_HOT : 0); return a; }
    a |= SA_MOVING;
    if (stressed) return a;
    if (K.taxis & TX_FIELD) return S.magnet ? (a | SA_FIELD) : a;   // a magnetotactic cell answers the field alone (its light and food unturned)
    if (S.spot && (K.taxis & TX_LIGHT)) a |= SA_LIGHT;
    if (S.spot && K.flees) a |= SA_FLEES;
    if (S.food && (K.taxis & TX_FOOD) && slideScentUm(t - S.foodT) >= 2 * S.foodR) a |= SA_FOOD;
    for (size_t i = 0; i < N.ev.size() && N.ev[i].t <= t; i++) if (N.lit[i]) a = N.ev[i].on ? (a | SA_LIT) : (a & ~SA_LIT);
    return a;
}
