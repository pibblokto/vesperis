// T-03 (2026-10-09): the works (see works.h)
#include "works.h"
#include "works_internal.h"
#include "../core/clock.h"

const char* const WORK_KIND_NAMES[WO_COUNT] = {"a quarry", "a mine", "a dam", "an aqueduct", "an observatory", "a shrine", "a pilgrims' way", "a necropolis",
    "lines on a plain", "a henge", "a fortified line", "a battlefield", "a launch site", "an antenna field", "a wreck", "a colossus", "a terraformed basin",
    "the last shelter", "something of the far future", "a wind farm", "a lighthouse", "a fort", "a dome over warm ground", "glasshouses"};
const char* const WORK_HEARD[WO_COUNT] = {"A QUARRY", "A MINE", "A DAM", "AN AQUEDUCT", "AN OBSERVATORY", "A SHRINE", "A WAY", "A NECROPOLIS", "LINES ON THE GROUND",
    "STANDING STONES", "A FORTIFIED LINE", "A BATTLEFIELD", "A LAUNCH SITE", "AN ANTENNA FIELD", "A WRECK", "A COLOSSUS", "SHAPED LAND", "A SHELTER",
    "SOMETHING MADE", "A WIND FARM", "A LIGHTHOUSE", "A FORT", "A DOME", "GLASSHOUSES"};

double editLeft(double years, double wet, bool line) {
    const double y = std::max(1.0, years);
    if (line) return clampd(1 - (std::log10(std::max(100.0, y)) - 2) / (wet > 0.4 ? 2.4 : 4.5), 0.15, 1);   // lines on a dry plain keep for millennia
    return clampd(1 - (std::log10(std::max(30.0, y)) - 1.5) / (wet > 0.4 ? 2.6 : 3.6), 0.12, 1);
}

bool damBreached(const Work& w, const Culture& c) {
    if (w.kind != WO_DAM) return false;
    Building b; b.kind = BK_DAM; b.age = w.age; b.before = w.before; b.style = w.style;
    b.seed = mix64(w.seed ^ 0xB01D0000ULL);   // its wall is the first piece `workSpec` lays
    b.luck = (unitFromHash(mix64(b.seed ^ 0x1CCULL)) - 0.5) * 0.5;
    const Culture wc = w.age == c.age && w.before <= 0 ? c : cultureAtAge(c, w.age, w.before);
    return buildingDecay(b, wc, w.style == 0 ? MT_MASONRY : (w.style == 3 ? MT_COMPOSITE : MT_MASS)) > 0.5;
}

// ---- the world's works ---------------------------------------------------------------------------------------------------------------
std::shared_ptr<WorldWorks> buildWorldWorks(const BodyGen& gSeason) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    auto W = std::make_shared<WorldWorks>();
    W->seed = g.seed; W->Rm = g.R * 1000.0;
    if (!worldHadCivilisation(g)) return W;
    const double t0 = nowMs();
    const std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
    W->peoples = P->peoples;
    DrainageOff off;
    WB b(g, *P, W->works);
    for (int p = 0; p < P->peoples; p++) {
        b.p = p; b.pp = &P->plans[p]; b.age = b.pp->age; b.H = historyOfWorld(g, p); b.C = cultureOf(g, p); b.pseed = peopleGen(g, p).seed ^ 0x7030ULL;
        b.ageFrom = b.H ? ageFromOf(b.H->profile) : 1;
        std::fill(b.made, b.made + WO_COUNT, 0);
        if (b.pp->places.empty()) continue;
        // the history's first (its battles, the last shelter, the works it finished where it finished them), then the world's great ones by
        // the terrain (B-418: the everyday works are the land's, `landWorkOf`)
        b.battlefields(); b.shelter(); b.sunlostWorks(); b.dams(); b.aqueducts(); b.observatories(); b.lighthouses(); b.launchSites(); b.shrine();
        b.colossi(); b.mines(); b.fortLine(); b.antennas(); b.terraforms(); b.strange(); b.geoglyphs();
        for (int k = 0; k < WO_COUNT; k++) W->counts[p][k] = b.made[k];
    }
    W->buildMs = nowMs() - t0;
    return W;
}

namespace {
using WorksFuture = std::shared_future<std::shared_ptr<const WorldWorks>>;
struct WorksSlot { WorksFuture fut; uint64_t used = 0; };
std::mutex g_worksMutex;
std::map<uint64_t, WorksSlot> g_works;
uint64_t g_worksClock = 0;
bool worksReady(const WorksFuture& f) { return f.valid() && f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }
}

std::shared_ptr<const WorldWorks> worldWorksOf(const BodyGen& g) {
    std::shared_ptr<std::promise<std::shared_ptr<const WorldWorks>>> pr;
    WorksFuture fut;
    {
        std::lock_guard<std::mutex> lock(g_worksMutex);
        auto it = g_works.find(g.seed);
        if (it != g_works.end()) { it->second.used = ++g_worksClock; fut = it->second.fut; }
        else {
            while (g_works.size() >= 4) {   // the least used of those built
                auto old = g_works.end();
                for (auto j = g_works.begin(); j != g_works.end(); ++j) if (worksReady(j->second.fut) && (old == g_works.end() || j->second.used < old->second.used)) old = j;
                if (old == g_works.end()) break;
                g_works.erase(old);
            }
            pr = std::make_shared<std::promise<std::shared_ptr<const WorldWorks>>>();
            fut = pr->get_future().share();
            g_works[g.seed] = WorksSlot{fut, ++g_worksClock};
        }
    }
    if (pr) pr->set_value(buildWorldWorks(g));
    return fut.get();
}

std::shared_ptr<const WorldWorks> worldWorksIfReady(const BodyGen& g) {
    std::lock_guard<std::mutex> lock(g_worksMutex);
    auto it = g_works.find(g.seed);
    if (it == g_works.end() || !worksReady(it->second.fut)) return nullptr;
    return it->second.fut.get();
}

// ---- B-418: the land's works, kept ---------------------------------------------------------------------------------------------------
int landCellsAround(const BodyGen& g, int gLat) {
    const double dLat = landCellLat(g);
    return std::max(1, (int)std::floor(TAU * std::cos(clampd((gLat + 0.5) * dLat, -PI / 2, PI / 2)) / dLat));
}

void landCellOf(const BodyGen& g, double lat, double lon, int& gLat, int& gLon) {
    gLat = (int)std::floor(lat / landCellLat(g));
    const int n = landCellsAround(g, gLat);
    double l = std::fmod(lon, TAU); if (l < 0) l += TAU;
    gLon = std::min(n - 1, (int)std::floor(l / (TAU / n)));
}

namespace {
// what every cell of a world's land reads: its plan, its works, its peoples' histories and cultures
struct LandCtx {
    BodyGen g; std::shared_ptr<const WorldPlan> P; std::shared_ptr<const WorldWorks> W;
    std::shared_ptr<const History> H[2]; Culture C[2]; uint64_t pseed[2] = {0, 0}; int ageFrom[2] = {1, 1};
};
std::shared_ptr<LandCtx> makeLandCtx(const BodyGen& g, std::shared_ptr<const WorldWorks> W) {
    auto X = std::make_shared<LandCtx>();
    { BodyGen gTmp; X->g = thenWorld(g, gTmp); }   // B-419: the world's, not its season's (P-04: in its people's time)
    X->P = worldPlanOf(g); X->W = std::move(W);
    for (int p = 0; p < std::min(2, X->P->peoples); p++) {
        X->H[p] = historyOfWorld(g, p); X->C[p] = cultureOf(g, p); X->pseed[p] = peopleGen(g, p).seed ^ 0x7030ULL;
        X->ageFrom[p] = X->H[p] ? ageFromOf(X->H[p]->profile) : 1;
    }
    return X;
}
// a cell's work: none where the draw by the countryside's density and the age's appetite says so, none where the cell's ground offers nothing
std::shared_ptr<const Work> landWorkIn(const LandCtx& X, int gLat, int gLon) {
    const BodyGen& g = X.g; const WorldPlan& P = *X.P;
    const double dLat = landCellLat(g);
    if (P.peoples == 0 || std::fabs((gLat + 0.5) * dLat) > PI / 2 - dLat) return nullptr;
    const LandCell L = landCellAt(g, gLat, gLon);
    const int p = clampi(peopleAt(g, L.latMid, L.lonMid), 0, P.peoples - 1);
    const PeoplePlan& pp = P.plans[p];
    if (pp.places.empty() || unitFromHash(L.h) >= LAND_RATE[clampi(pp.age, 0, AGE_COUNT - 1)] * planCountryside(P, L.latMid, L.lonMid, p)) return nullptr;
    std::vector<Work> out;
    DrainageOff off;
    WB b(g, P, out); b.world = &X.W->works;
    b.p = p; b.pp = &pp; b.age = pp.age; b.H = X.H[p]; b.C = X.C[p]; b.pseed = X.pseed[p]; b.ageFrom = X.ageFrom[p];
    if (!b.landWork(L) || out.empty()) return nullptr;
    out.back().land = true;
    return std::make_shared<const Work>(std::move(out.back()));
}
// the last two worlds' cells (a world's slot: its reading and the cells asked for; cleared when it holds 40,000)
struct LandSlot { uint64_t seed = 0, used = 0; std::shared_ptr<LandCtx> ctx; std::unordered_map<uint64_t, std::shared_ptr<const Work>> cells; };
std::mutex g_landMutex;
LandSlot g_land[2];
uint64_t g_landClock = 0;
inline uint64_t landKey(int gLat, int gl) { return ((uint64_t)(uint32_t)(gLat + 0x100000) << 32) | (uint32_t)gl; }
}

std::shared_ptr<const Work> landWorkOf(const BodyGen& g, int gLat, int gLon) {
    if (!worldHadCivilisation(g)) return nullptr;
    const int n = landCellsAround(g, gLat), gl = ((gLon % n) + n) % n;
    const uint64_t key = landKey(gLat, gl);
    std::shared_ptr<LandCtx> ctx;
    {
        std::lock_guard<std::mutex> lock(g_landMutex);
        for (LandSlot& s : g_land) if (s.ctx && s.seed == g.seed) {
            s.used = ++g_landClock;
            auto it = s.cells.find(key);
            if (it != s.cells.end()) return it->second;
            ctx = s.ctx;
        }
    }
    if (!ctx) {
        std::shared_ptr<LandCtx> X = makeLandCtx(g, worldWorksOf(g));   // the world's works waited for: the land's keep clear of them
        std::lock_guard<std::mutex> lock(g_landMutex);
        LandSlot* s = nullptr;
        for (LandSlot& q : g_land) if (q.ctx && q.seed == g.seed) s = &q;
        if (!s) {
            s = &g_land[0];
            for (LandSlot& q : g_land) if (!q.ctx || q.used < s->used) s = &q;
            *s = LandSlot(); s->seed = g.seed; s->ctx = X;
        }
        s->used = ++g_landClock; ctx = s->ctx;
    }
    std::shared_ptr<const Work> w = landWorkIn(*ctx, gLat, gl);
    std::lock_guard<std::mutex> lock(g_landMutex);
    for (LandSlot& s : g_land) if (s.ctx && s.seed == g.seed) {
        if (s.cells.size() >= 40000) s.cells.clear();
        return s.cells.emplace(key, w).first->second;
    }
    return w;   // its world's slot was taken meanwhile
}

std::shared_ptr<const Work> buildLandWork(const BodyGen& g, int gLat, int gLon) {
    if (!worldHadCivilisation(g)) return nullptr;
    const int n = landCellsAround(g, gLat);
    return landWorkIn(*makeLandCtx(g, worldWorksOf(g)), gLat, ((gLon % n) + n) % n);
}

namespace {
// the world's works whose reach comes within `radiusM`, then the land's cells within `landR` and the farthest a land's work reaches
void worksAroundOf(const BodyGen& g, const std::shared_ptr<const WorldWorks>& W, const Vec3& unit, double radiusM, double landR, std::vector<std::shared_ptr<const Work>>& out) {
    out.clear();
    for (const Work& w : W->works) if (arcLength(unit, w.unit, W->Rm) < radiusM + w.radiusM) out.push_back(std::shared_ptr<const Work>(W, &w));
    radiusM = landR;
    const double Rm = g.R * 1000.0, dLat = landCellLat(g), reach = radiusM + LAND_REACH_MAX;
    double lat, lon; StarSystem::latLonFromBody(unit, lat, lon);
    const int r0 = (int)std::floor((lat - reach / Rm) / dLat), r1 = (int)std::floor((lat + reach / Rm) / dLat);
    double lon01 = std::fmod(lon, TAU); if (lon01 < 0) lon01 += TAU;
    for (int row = r0; row <= r1; row++) {
        const double latc = (row + 0.5) * dLat;
        if (std::fabs(latc) > PI / 2 - dLat) continue;
        const int n = landCellsAround(g, row);
        const double w = TAU / n, edge = std::min(PI / 2, std::fabs(latc) + 0.5 * dLat);
        const double lonR = reach / (Rm * std::max(std::cos(edge), 1e-3));
        int c0 = (int)std::floor((lon01 - lonR) / w), c1 = (int)std::floor((lon01 + lonR) / w);
        if (c1 - c0 + 1 >= n) { c0 = 0; c1 = n - 1; }
        for (int c = c0; c <= c1; c++) {
            const int gl = ((c % n) + n) % n;
            const Vec3 mid = StarSystem::bodyFromLatLon(latc, (gl + 0.5) * w);
            if (arcLength(unit, mid, Rm) > reach + 0.75 * (w * Rm * std::cos(latc) + dLat * Rm)) continue;   // beyond the cell's corners
            std::shared_ptr<const Work> lw = landWorkOf(g, row, gl);
            if (lw && arcLength(unit, lw->unit, Rm) < radiusM + lw->radiusM) out.push_back(std::move(lw));
        }
    }
}
}

void worksAround(const BodyGen& g, const Vec3& unit, double radiusM, std::vector<std::shared_ptr<const Work>>& out) {
    out.clear();
    if (!worldHadCivilisation(g)) return;
    worksAroundOf(g, worldWorksOf(g), unit, radiusM, radiusM, out);
}

bool worksAroundIfReady(const BodyGen& g, const Vec3& unit, double radiusM, std::vector<std::shared_ptr<const Work>>& out, double landRadiusM) {
    out.clear();
    if (!worldHadCivilisation(g)) return true;
    const std::shared_ptr<const WorldWorks> W = worldWorksIfReady(g);
    if (!W) return false;
    worksAroundOf(g, W, unit, radiusM, landRadiusM < 0 ? radiusM : landRadiusM, out);
    return true;
}
