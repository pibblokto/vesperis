// Z-04: a slide made from a vial (life/slides.h): the mount (its preparation, each species' tile and cells, the first look), what its events
// make of it at a time (the stage's warmth eased in and out, the magnet's field, the spot, the food) and where each copy of a cell is then and
// what it does: drawn to the spot or the food it turns to at its speed, along the field with a U-turn at each turn of it, stressed past its
// warmth into a spore, a cyst or its lysis
#include "life/slides.h"
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
constexpr double WARM_C = 30, WARM_TAU = 12;      // the warm stage: 30 C over the sample's, eased in and out with a time constant of 12 s
constexpr double STRESS_C = 2;                    // over its range's top by this, a species' cells are stressed
constexpr double TURN_S = 1.6;                    // a magnetotactic cell's U-turn when the field is turned half round
constexpr double EASE_S = 2;                      // a cell's wander narrowing as it is drawn (and widening again as it is let go)
constexpr double TILE_MAX = 1500;                 // the longest tile (um): a rare species over-represented to a cell in it (found by a look round at x40)
constexpr int TILE_CELLS = 160;                   // the most cells a species keeps in its tile
constexpr int KIND_SPECIMENS = 3000;              // what a view draws of a species at most (the low powers' specks: past it, a share of its copies)

inline double h01(uint64_t h) { return unitFromHash(mix64(h)); }
double bodyAcross(const Morph& m) { return std::max(0.3, 2 * m.bodyUm()); }
double ease(double x) { x = clampd(x, 0, 1); return x * x * (3 - 2 * x); }

// the stage's warmth: the sample's, eased toward 30 C over it while the warm stage is on and back while it is off; `reaches` the first moment
// it is over `c` (1e30 never)
struct Warmth {
    double base, T, from = 0; bool warm = false;
    explicit Warmth(double c0) : base(c0), T(c0) {}
    double target() const { return warm ? base + WARM_C : base; }
    double crossing(double until, double c) const {   // within [from, until): when the eased warmth passes `c` going up, else -1
        const double A = target();
        if (A <= c || T >= c) return -1;
        const double tc = from - WARM_TAU * std::log((c - A) / (T - A));
        return tc < until ? tc : -1;
    }
    void to(double until) { const double A = target(); T = A + (T - A) * std::exp(-(until - from) / WARM_TAU); from = until; }
};
double warmthReaches(const SlideMount& M, const std::vector<SlideEvent>& ev, double c) {
    Warmth W(M.stageC);
    if (W.T > c) return 0;
    for (const SlideEvent& e : ev) {
        if (e.kind != SD_WARM) continue;
        const double tc = W.crossing(e.t, c);
        if (tc >= 0) return tc;
        W.to(e.t); W.warm = e.on;
    }
    const double tc = W.crossing(1e30, c);
    return tc >= 0 ? tc : 1e30;
}

// a copy's seed: the mount's cell's own in the first tile, else its tile's
uint64_t copySeed(const SlideCell& c, int64_t ti, int64_t tj) { return ti == 0 && tj == 0 ? c.seed : mix64(c.seed ^ hash2i(ti, tj, SALT_COPY)); }

// a kind's way along the magnet's field: the turns to it (its side its own, the other way the next time: the U-turns' sideways steps
// cancel), its swim along it at its speed; from the magnet's coming. `phi` its heading then, `x`, `y` its place moved
void fieldRun(const SlideKind& K, const std::vector<SlideEvent>& ev, double t, double phi0, double side, double& x, double& y, double& phi, double& since) {
    phi = phi0; since = -1;
    bool on = false; double th = 0, last = 0; int turns = 0;
    auto run = [&](double t0, double t1) {
        double d = wrapAngle(th - phi);
        if (std::fabs(std::fabs(d) - PI) < 1e-3) d = (turns & 1 ? -side : side) * PI;
        const double tau = TURN_S * std::fabs(d) / PI, w = tau > 1e-9 ? d / tau : 0, a = std::min(t1 - t0, tau);
        if (a > 0 && std::fabs(w) > 1e-9) { x += K.speed * (std::sin(phi + w * a) - std::sin(phi)) / w; y += K.speed * (std::cos(phi) - std::cos(phi + w * a)) / w; }
        phi += w * a;
        const double b = t1 - t0 - a;
        if (b > 0) { phi = th; x += K.speed * std::cos(th) * b; y += K.speed * std::sin(th) * b; }
    };
    for (const SlideEvent& e : ev) {
        if (e.t > t) break;
        if (e.kind != SD_MAGNET) continue;
        if (on) { run(last, e.t); turns++; } else since = e.t;
        on = true; th = e.x; last = e.t;
    }
    if (on) run(last, t);
}

// how far a cell senses a spot or a drop of a radius: four radii, or what it swims in 25 s, to twelve
double reachOf(const SlideKind& K, double r) { return std::max(r * 4, std::min(K.speed * 25, r * 12)); }

// the share of a kind's cells within a spot's reach that answer it: enough to cover a third of the spot (a crowd one cell deep), all where fewer
double answering(const SlideKind& K, double r) {
    const double reach = reachOf(K, r), density = K.n / std::max(1e-9, K.tile * K.tile);
    const double room = 0.35 * (0.8 * r) * (0.8 * r) / std::max(1e-6, 0.25 * K.body * K.body);
    return std::min(1.0, room / std::max(1e-9, density * PI * reach * reach));
}

// a copy drawn to the spots and the drops of food it turns to: each one it senses (within its reach, the crowd's share of its kind) starts a run
// to a point of it at its speed; the light taken away leaves it where it got to. Its drift now, and how far its wander is narrowed (eased in as it
// is drawn, out as it is let go)
void attractedRun(const SlideKind& K, const std::vector<SlideEvent>& ev, double t, uint64_t seed, double& x, double& y, double& vx, double& vy, double& wander) {
    vx = vy = 0; wander = 1;
    const bool light = (K.taxis & TX_LIGHT) != 0, food = (K.taxis & TX_FOOD) != 0;
    if (!K.active || K.speed <= 0 || (!light && !food)) return;
    bool going = false; double tx = 0, ty = 0, from = 0, fx = x, fy = y, drawnAt = -1, letAt = -1;
    auto advance = [&](double until) {
        if (!going) return;
        const double dx = tx - fx, dy = ty - fy, d = std::sqrt(dx * dx + dy * dy), s = std::min(d, K.speed * std::max(0.0, until - from));
        if (d > 1e-9) { x = fx + dx / d * s; y = fy + dy / d * s; }
    };
    int k = 0;
    for (const SlideEvent& e : ev) {
        if (e.t > t) break;
        k++;
        if (!((e.kind == SD_LIGHT && light) || (e.kind == SD_FOOD && food))) continue;
        advance(e.t);
        const bool was = going;
        going = false;
        if (e.on) {
            const uint64_t q = hashCombine(seed ^ SALT_COPY, (uint64_t)k * 0x2545F4914F6CDD1DULL);
            const double rr = e.r * 0.8 * std::sqrt(h01(q)), a = h01(q + 1) * TAU, qx = e.x + rr * std::cos(a), qy = e.y + rr * std::sin(a);
            if (std::hypot(qx - x, qy - y) <= reachOf(K, e.r) && h01(q + 2) < answering(K, e.r)) { going = true; tx = qx; ty = qy; from = e.t; fx = x; fy = y; if (!was) drawnAt = e.t; }
        }
        if (was && !going) letAt = e.t;
    }
    if (going) {
        advance(t);
        const double left = std::hypot(tx - x, ty - y), size = std::max(1.0, K.reach * 0.3);
        if (left > 1e-6) { vx = (tx - x) / left * K.speed; vy = (ty - y) / left * K.speed; }
        const double w = 0.25 + 0.1 * clampd(left / (4 * size), 0, 1);
        wander = 1 + (w - 1) * ease((t - drawnAt) / EASE_S);
    } else if (letAt >= 0) wander = 0.25 + 0.75 * ease((t - letAt) / (EASE_S * 1.5));
}

// what a kind's cells share at a time: when the stage's warmth stresses them, the field's run of a cell starting along it (the copies' range)
struct KindNow { double stressAt = 1e30; double fx = 0, fy = 0, fieldBound = 0, pull = 0; };
KindNow kindNow(const SlideMount& M, const SlideKind& K, const std::vector<SlideEvent>& ev, double t) {
    KindNow N;
    if (K.stress >= 0) N.stressAt = warmthReaches(M, ev, K.tMax + STRESS_C);
    if (K.active && K.speed > 0 && (K.taxis & TX_FIELD)) {
        double th0 = 0;
        for (const SlideEvent& e : ev) if (e.kind == SD_MAGNET && e.t <= t) { th0 = e.x; break; }
        double phi, since;
        fieldRun(K, ev, std::min(t, N.stressAt + 7), th0, 1, N.fx, N.fy, phi, since);
        N.fieldBound = K.speed * (TURN_S * 3 + (t > N.stressAt ? 6 : 0));   // the copies' own turns; their stops, 5 s either side of the shared
    } else if (K.active && K.speed > 0 && (K.taxis & (TX_LIGHT | TX_FOOD)))
        for (const SlideEvent& e : ev) if (e.t <= t && e.on && (e.kind == SD_LIGHT || e.kind == SD_FOOD)) N.pull = std::max(N.pull, reachOf(K, e.r));
    return N;
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
    s.still = !K.active || c.form == CF_COLONY || m.motion == MO_NONE;
    const bool stressable = K.stress >= 0 && N.stressAt < 1e29 && (c.form == CF_CELL || c.form == CF_DIVIDING);
    const double tStress = stressable ? N.stressAt + 2 + 10 * h01(cs ^ 0x57E55) : 1e30;
    const double tm = std::min(t, tStress);   // a stressed cell's moves end where its stress takes it
    if (!s.still && (K.taxis & TX_FIELD)) {
        double phi, since;
        fieldRun(K, ev, tm, s.angle, h01(cs ^ 0x51DE) < 0.5 ? -1 : 1, s.x, s.y, phi, since);
        if (since >= 0) {
            s.wander = 1 - 0.7 * ease((tm - since) / EASE_S);
            if (tm == t) { s.vx = K.speed * std::cos(phi); s.vy = K.speed * std::sin(phi); }
            if (m.motion == MO_GLIDING || m.motion == MO_CORKSCREW || m.motion == MO_TWITCHING) s.angle = phi;
        }
    } else if (!s.still) {
        double vx, vy, w;
        attractedRun(K, ev, tm, cs, s.x, s.y, vx, vy, w);
        s.wander = w;
        if (tm == t) { s.vx = vx; s.vy = vy; }
        if ((vx != 0 || vy != 0) && (m.motion == MO_GLIDING || m.motion == MO_CORKSCREW || m.motion == MO_TWITCHING)) s.angle = std::atan2(vy, vx);
    }
    if (t >= tStress) {   // stressed: its act from where it was, then its last form
        double x, y, a;
        specimenPlace(s, tm, x, y, a);
        s.x = x; s.y = y; s.angle = a; s.vx = s.vy = 0;
        if (t < tStress + actSeconds(m, K.stress)) { s.act = K.stress; s.t0 = tStress; s.form = CF_CELL; }
        else { s.form = K.stressForm; s.still = true; s.wander = 1; }
    }
    // its depth: a swimmer's rising and falling through the drop, the blur out of focus (the electron's and the section's in focus throughout)
    const double z = mountDepth(M, SlideRef{ci, ti, tj}, t);
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
        K.active = H.active && H.state == SS_LIVING && K.form == CF_CELL;
        K.taxis = K.active && m.motion != MO_NONE ? m.taxis : 0;
        K.speed = m.speed;
        K.tMax = sp.tMax;
        if (K.active) {   // the warm stage's stress: a spore former's spores, a large cell's cyst, else its lysis
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
            case SD_LIGHT: S.spot = e.on; S.spotX = e.x; S.spotY = e.y; S.spotR = e.r; break;
            case SD_MAGNET: S.magnet = true; S.field = e.x; break;
            case SD_WARM: W.to(e.t); W.warm = e.on; break;
            case SD_FOOD: S.food = e.on; S.foodX = e.x; S.foodY = e.y; S.foodR = e.r; break;
            default: break;
        }
    }
    W.to(t);
    S.stageC = W.T; S.warming = W.warm;
    return S;
}

double mountDepth(const SlideMount& M, const SlideRef& r, double t) {
    if (r.cell < 0 || r.cell >= (int)M.cells.size()) return 0;
    const SlideCell& c = M.cells[r.cell];
    const SlideKind& K = M.kinds[c.kind];
    const uint64_t cs = copySeed(c, r.ti, r.tj);
    const double z0 = r.ti == 0 && r.tj == 0 ? c.z : K.z0 + (K.z1 - K.z0) * h01(cs ^ 0x2D);
    if (!K.active || c.form != CF_CELL || K.z1 - K.z0 < 1) return z0;
    const double A = std::min(3.0, (K.z1 - K.z0) * 0.5), w = 0.15 + 0.25 * h01(cs ^ 0x3D);
    return clampd(z0 + A * std::sin(w * t + h01(cs ^ 0x4D) * TAU), K.z0, K.z1);
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
