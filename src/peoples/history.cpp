// H-02 (2026-10-09): the history (see history.h).
#include "history.h"
#include "history_builder.h"
#include "shards.h"
#include "../core/clock.h"

// ---- the history's queries ----------------------------------------------------------------------------------------------------
int History::eventsIn(int year, int& from) const {
    auto lo = std::lower_bound(events.begin(), events.end(), year, [](const HistoryEvent& e, int y) { return e.year < y; });
    auto hi = std::upper_bound(lo, events.end(), year, [](int y, const HistoryEvent& e) { return y < e.year; });
    from = (int)(lo - events.begin());
    return (int)(hi - lo);
}
int History::reignAt(int year) const {
    for (int k = (int)reigns.size() - 1; k >= 0; k--) if (reigns[k].start <= year && reigns[k].end >= year) return k;
    return -1;
}
int History::eraAt(int year) const {
    for (int k = 0; k < (int)eras.size(); k++) if (year <= eras[k].end) return k;
    return eras.empty() ? 0 : (int)eras.size() - 1;
}
int History::harvestAt(int year) const { const int i = year - yearsFrom; return i >= 0 && i < (int)harvest.size() ? harvest[i] : 2; }
double History::sunlightAt(int year) const {
    if (sunlight.empty()) return 1;
    const int i = clampi(year - yearsFrom, 0, (int)sunlight.size() - 1);
    return year < yearsFrom ? 1.0 : sunlight[i];
}
int History::inventionYear(int id) const { for (const HistoryInvention& I : inventions) if (I.id == id) return I.year; return 0; }
std::vector<int> History::eventsOf(int person) const {
    std::vector<int> o;
    if (person < 0 || person + 1 >= (int)personFirst.size()) return o;
    for (int k = personFirst[person]; k < personFirst[person + 1]; k++) if (personEvents[k] >= 0) o.push_back(personEvents[k]);
    return o;
}

std::shared_ptr<History> buildHistory(const StarSystem& sys0, const Body& b0, const BodyGen& gSeason, int people) {
    const PeoplesWorld PW = peoplesWorldOf(sys0, b0);   // D-05: a drifter's people's world at its former orbit
    const StarSystem& sys = *PW.sys; const Body& b = *PW.b;
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // the world's, not its season's (P-04: in its people's time)
    const double t0 = nowMs();
    auto H = std::make_shared<History>();
    const int n = std::max(1, peoplesOf(g));
    people = people > 0 && n > 1 ? 1 : 0;
    H->people = people; H->peoples = n;
    H->profile = peopleProfileOf(sys, b, g, people);
    H->lore = loreQuick(sys, b, g, people);
    Builder B(sys, b, g, people, *H);
    B.WP = worldPlanOf(g);
    B.PP = &B.WP->plans[std::min(people, B.WP->peoples - 1)];
    B.G = &B.WP->grid;
    const double tPlan = nowMs();
    B.setup();
    B.setupPlaces();
    B.setupCollapse();
    B.setupPlanned();
    B.importJoint();
    B.oldTime();
    B.yearLoop();
    B.family();
    B.finish();
    H->buildMs = nowMs() - tPlan;
    (void)t0;
    return H;
}

namespace {
struct HistorySlot { std::shared_ptr<const History> h; uint64_t used = 0; };
std::mutex g_histMutex;
std::map<std::pair<uint64_t, int>, HistorySlot> g_hist;
uint64_t g_histTick = 0;
}
std::shared_ptr<const History> historyOf(const StarSystem& sys, const Body& b, const BodyGen& gSeason, int people) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    const std::pair<uint64_t, int> key(g.seed, people > 0 ? 1 : 0);
    {
        std::lock_guard<std::mutex> lk(g_histMutex);
        auto it = g_hist.find(key);
        if (it != g_hist.end()) { it->second.used = ++g_histTick; return it->second.h; }
    }
    std::shared_ptr<const History> h = buildHistory(sys, b, g, people);
    std::lock_guard<std::mutex> lk(g_histMutex);
    if (g_hist.size() >= 8) {
        auto old = g_hist.begin();
        for (auto it = g_hist.begin(); it != g_hist.end(); ++it) if (it->second.used < old->second.used) old = it;
        g_hist.erase(old);
    }
    g_hist[key] = HistorySlot{h, ++g_histTick};
    return h;
}

std::shared_ptr<const History> historyOfWorld(const BodyGen& gSeason, int people) {
    BodyGen gTmp; const BodyGen& g = thenWorld(gSeason, gTmp);   // B-419: the world's, not its season's (P-04: in its people's time)
    const std::pair<uint64_t, int> key(g.seed, people > 0 ? 1 : 0);
    {
        std::lock_guard<std::mutex> lk(g_histMutex);
        auto it = g_hist.find(key);
        if (it != g_hist.end()) { it->second.used = ++g_histTick; return it->second.h; }
    }
    if (!g.starKnown || g.bodyIndex < 0) return nullptr;
    Star st;
    if (!placeInSector(g.starSx, g.starSy, g.starSz, st, true)) return nullptr;
    StarSystem sys; sys.generate(st);
    if (g.bodyIndex >= (int)sys.bodies.size() || sys.bodies[g.bodyIndex].seed != g.seed) return nullptr;
    return historyOf(sys, sys.bodies[g.bodyIndex], g, people);
}
