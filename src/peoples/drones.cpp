// C-18 (2026-10-11): what the drone bay's drones can bring from a people's world, and a trip's haul (peoples/drones.h).
#include "drones.h"
#include "records.h"
#include "instruments.h"
#include "music.h"
#include "shards.h"
#include "plan.h"
#include "ruins.h"
#include "core/rng.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <set>

const char* const DRONE_FIND_NAMES[DF_COUNT] = {"text", "music", "star chart", "instrument"};

void droneWorldOf(const StarSystem& sys, int body, DroneWorld& out) {
    out = DroneWorld();
    if (body < 0 || body >= (int)sys.bodies.size()) return;
    const auto t0 = std::chrono::steady_clock::now();
    const Body& b = sys.bodies[body];
    const BodyGen g = BodyGen::make(b);
    out.seed = g.seed;
    if (!worldHadCivilisation(g)) return;
    out.peoples = std::max(1, peoplesOf(g));
    std::shared_ptr<const WorldPlan> P = worldPlanOf(g);   // the plan the homes are ranks of (built on the approach's thread: waited for)
    std::map<std::pair<int, int>, int> placeIndex;
    auto placeOf = [&](int people, int rank) {
        if (!P || rank < 0 || people < 0 || people > 1 || rank >= (int)P->plans[people].places.size()) return -1;
        const auto key = std::make_pair(people, rank);
        auto it = placeIndex.find(key);
        if (it != placeIndex.end()) return it->second;
        const PlanPlace& pp = P->plans[people].places[rank];
        DronePlace dp; dp.people = people; dp.rank = rank; dp.sclass = pp.sclass; dp.lat = pp.lat; dp.lon = pp.lon;
        out.places.push_back(dp);
        return placeIndex[key] = (int)out.places.size() - 1;
    };
    for (int k = 0; k < out.peoples; k++) {
        std::shared_ptr<const RecordCatalogue> C = recordsOf(g, k);
        std::shared_ptr<const RecordHomes> H = recordHomesOf(g, k);
        if (H) for (int r : H->rank) if (r >= 0) { out.homes = true; break; }
        const Lore L = loreQuick(sys, b, g, k);
        out.peopleNames.push_back(L.people);
        const Tradition T = traditionOf(g, L);
        PeopleInstruments PI; instrumentsOf(T, L.which, PI);
        std::set<int> seen;   // an instrument lies beside a third of the music records (each copy alike): one of each on the world
        for (int i = 0; i < C->count && i < (int)C->entries.size(); i++) {
            const RecordEntry& e = C->entries[i];
            const int place = placeOf(k, H && i < (int)H->rank.size() ? H->rank[i] : -1);
            DroneItem it;
            it.kind = e.category == RC_MUSIC ? DF_MUSIC : (e.category == RC_CHART ? DF_CHART : DF_TEXT);
            it.index = shardWorldIndex(k, i); it.people = k; it.medium = e.medium; it.age = e.age; it.place = place;
            out.items.push_back(it);
            double pick = 0;
            if (e.category != RC_MUSIC || !instrumentWithRecord(g.seed, k, i, pick)) continue;
            const int id = instrumentIdOf(PI, e.age, pick);
            const Instrument* I = id >= 0 && !seen.count(id) ? instrumentById(PI, id) : nullptr;
            if (!I) continue;
            seen.insert(id);
            DroneItem in; in.kind = DF_INSTRUMENT; in.index = id; in.people = k; in.age = I->age; in.place = place; in.name = instrumentName(T, *I);
            out.items.push_back(in);
        }
    }
    out.buildMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

void droneHaulOf(const DroneWorld& W, const std::vector<bool>& held, uint64_t seed, DroneHaul& out) {
    out = DroneHaul();
    Rng rng(seed ^ 0xC18D0E5ULL);
    const int np = (int)W.places.size(), n = (int)W.items.size();
    std::vector<bool> gone(n, false);   // held, or in a drone's hands already
    for (int i = 0; i < n; i++) gone[i] = i < (int)held.size() && held[i];
    std::vector<std::vector<int>> own(np + 1);   // each place's own (the records it is the home of, the instruments beside them); the last list: no place's
    for (int i = 0; i < n; i++) if (!gone[i]) { const int p = W.items[i].place; own[p >= 0 && p < np ? p : np].push_back(i); }
    static const double KIND_W[DF_COUNT] = {1.0, 1.4, 2.2, 4.0};   // the rarer kinds leaned toward (a world holds a few instruments to its hundreds of records)
    std::vector<int> cand; std::vector<double> kw;
    auto draw = [&](const std::vector<int>& from, int count, std::vector<int>& into) {   // by kind, without putting back
        cand.clear();
        for (int i : from) if (!gone[i]) cand.push_back(i);
        while (count > 0 && !cand.empty()) {
            kw.resize(cand.size());
            for (size_t j = 0; j < cand.size(); j++) kw[j] = KIND_W[std::max(0, std::min(DF_COUNT - 1, W.items[cand[j]].kind))];
            const int j = rng.pick(kw.data(), (int)kw.size());
            into.push_back(cand[j]); gone[cand[j]] = true; cand.erase(cand.begin() + j); count--;
        }
    };
    auto ownLeft = [&](int p) { int c = 0; for (int i : own[p]) if (!gone[i]) c++; return c; };
    for (int k = 0; k < DRONE_COUNT; k++) out.wanted[k] = DRONE_MIN_FINDS + rng.irange(DRONE_MAX_FINDS - DRONE_MIN_FINDS + 1);
    std::vector<int> used(np + 1, 0), copies;
    std::vector<double> w(np + 1);
    for (int k = 0; k < DRONE_COUNT; k++) {
        int left = out.wanted[k];
        for (int s = 0; s < DRONE_MAX_STOPS && left > 0; s++) {
            double total = 0;   // the place: the more of its own it holds the likelier (sublinearly), one another drone searched seldom
            for (int p = 0; p <= np; p++) { const int c = ownLeft(p); w[p] = c ? std::pow((double)c, 0.75) * (used[p] ? 0.15 : 1.0) : 0.0; total += w[p]; }
            int p = np;
            if (total > 0) p = rng.pick(w.data(), np + 1);
            else {   // no place holds its own any more: copies alone, at any place
                bool any = false;
                for (int i = 0; i < n && !any; i++) any = !gone[i];
                if (!any) break;
                if (np > 0) p = rng.irange(np);
            }
            used[p]++;
            DroneLeg leg; leg.place = p < np ? p : -1;
            const int here = s + 1 < DRONE_MAX_STOPS ? std::min(left, 3 + rng.irange(3)) : left;   // three to five at a stop, the rest further on
            if (rng.chance(0.35)) {   // an instrument of the place's people, beside a copy of its record there (a world holds few: looked for first)
                const int people = p < np ? W.places[p].people : -1;
                copies.clear();
                for (int i = 0; i < n; i++) if (!gone[i] && W.items[i].kind == DF_INSTRUMENT && (people < 0 || W.items[i].people == people)) copies.push_back(i);
                if (!copies.empty()) { const int i = copies[rng.irange((int)copies.size())]; leg.items.push_back(i); gone[i] = true; }
            }
            draw(own[p], here - (int)leg.items.size(), leg.items);
            if ((int)leg.items.size() < here) {   // copies kept there of the place's people's records (an instrument lies beside every copy of its record)
                const int people = p < np ? W.places[p].people : -1;
                copies.clear();
                for (int i = 0; i < n; i++) if (!gone[i] && (people < 0 || W.items[i].people == people)) copies.push_back(i);
                draw(copies, here - (int)leg.items.size(), leg.items);
            }
            if (leg.items.empty()) continue;
            left -= (int)leg.items.size(); out.total += (int)leg.items.size();
            out.legs[k].push_back(leg);
        }
    }
}
