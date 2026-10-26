// E-10: the builder's setup (history_builder.h): the eras, the places and their founding (the founding schedule, which the other
// people's meeting reads too), the collapse's script, the planned events, and the other people's meeting, treaties and wars
#include "history_builder.h"
#include "tongue.h"

// ---- the founding schedule (shared with the other people's meeting: both histories read the same years) -------------------------
namespace {
// the eras' indices of the historic ages (the medieval and later; a prehistoric people's one era)
// `scores`: the older ages' scores over the people's territory, filled where asked for and kept (the history's cleared places read the medieval)
Schedule foundingSchedule(const BodyGen& g, const PlanGrid& G, const PeoplePlan& pp, const PeopleProfile& prof, int people, std::vector<std::vector<float>>* scores = nullptr) {
    Schedule S;
    const int n = (int)pp.places.size();
    S.founded.assign(n, 1); S.age.assign(n, prof.age);
    if (n == 0) return S;
    Rng r(prof.seed ^ 0x40FD5ULL);
    std::vector<int> eraOf(n, -1);
    int firstHist = 0;
    while (firstHist + 1 < (int)prof.eras.size() && prof.eras[firstHist].age == AGE_PREHISTORIC && prof.age != AGE_PREHISTORIC) firstHist++;
    const int lastEra = (int)prof.eras.size() - 1;
    if (prof.age <= AGE_MEDIEVAL) for (int i = 0; i < n; i++) eraOf[i] = firstHist;
    else {
        // which places were old: their ground's score for each older age, the best of the age's share founded in it
        std::vector<int> left;
        for (int i = 1; i < n; i++) left.push_back(i);
        // the share of the places each era founded: the old cities, the mill towns, the new
        double share[AGE_COUNT] = {0, 0, 0, 0, 0, 0};
        switch (prof.age) {
            case AGE_INDUSTRIAL: share[AGE_MEDIEVAL] = 0.72; break;
            case AGE_MODERN: share[AGE_MEDIEVAL] = 0.55; share[AGE_INDUSTRIAL] = 0.25 / 0.45; break;
            case AGE_FUTURISTIC: share[AGE_MEDIEVAL] = 0.45; share[AGE_INDUSTRIAL] = 0.2 / 0.55; share[AGE_MODERN] = 0.15 / 0.35; break;
            default: break;   // the far future: its works are its own
        }
        std::vector<std::vector<float>> own(AGE_COUNT);
        std::vector<std::vector<float>>& sc = scores ? *scores : own;
        sc.resize(AGE_COUNT);
        for (int e = firstHist; e <= lastEra; e++) { const int a = prof.eras[e].age; if (a < prof.age && share[a] > 0 && sc[a].empty()) planScores(g, G, people, a, sc[a], &pp.territory); }
        for (int e = firstHist; e < lastEra; e++) {
            const int a = prof.eras[e].age;
            if (share[a] <= 0 || sc[a].empty()) continue;
            std::vector<std::pair<double, int>> k;
            for (int i : left) {
                const PlanPlace& p = pp.places[i];
                if (prof.age == AGE_FUTURISTIC && p.sclass == SC_VILLAGE) continue;   // a futuristic people's lesser places are its own age's
                const double big = (p.sclass == SC_CITY || (prof.age == AGE_FUTURISTIC && p.sclass == SC_TOWN)) ? 0.25 : 0.0;
                k.push_back({sc[a][p.cell] + big + 0.15 * unitFromHash(mix64(p.seed ^ (0x40FDAULL + a))), i});
            }
            std::sort(k.begin(), k.end(), [](const std::pair<double, int>& x, const std::pair<double, int>& y) { return x.first != y.first ? x.first > y.first : x.second < y.second; });
            const int take = (int)std::lround(share[a] * left.size());
            std::vector<int> rest;
            std::vector<char> took(n, 0);
            for (int t = 0; t < take && t < (int)k.size(); t++) { eraOf[k[t].second] = e; took[k[t].second] = 1; }
            for (int i : left) if (!took[i]) rest.push_back(i);
            left.swap(rest);
        }
        int newest = lastEra;
        if (prof.age != AGE_FAR_FUTURE && lastEra > firstHist && prof.collapseYear - prof.eras[lastEra].start < 40) newest = lastEra - 1;
        for (int i : left) eraOf[i] = newest;
        eraOf[0] = prof.age == AGE_FAR_FUTURE ? lastEra : firstHist;
    }
    if (prof.age != AGE_FAR_FUTURE) eraOf[0] = firstHist;
    // the years: in the plan's order within each era, most in its first half (the land filled quickly), none in the collapse
    for (int e = 0; e <= lastEra; e++) {
        std::vector<int> list;
        for (int i = 0; i < n; i++) if (eraOf[i] == e) list.push_back(i);
        if (list.empty()) continue;
        const Era& E = prof.eras[e];
        int start = E.start;
        const bool capHere = eraOf[0] == e && prof.age != AGE_FAR_FUTURE;
        if (capHere) { S.founded[0] = 1; start = std::max(start, 2); list.erase(std::remove(list.begin(), list.end(), 0), list.end()); }
        int stop = E.end;
        if (e == lastEra) stop = std::min(stop, prof.collapseYear - 1);
        const int len = std::max(1, stop - start + 1);
        const double span = (prof.age == AGE_FAR_FUTURE && e == lastEra) ? 0.6 * len : 0.7 * len;
        const int m = (int)list.size();
        std::vector<int> ys(m);
        // slow, then the land filling, then slowing (the inverse of a smoothstep), the first daughter towns a few decades on
        auto invSmooth = [](double u) { double lo = 0, hi = 1; for (int it = 0; it < 30; it++) { const double mid = 0.5 * (lo + hi); if (mid * mid * (3 - 2 * mid) < u) lo = mid; else hi = mid; } return 0.5 * (lo + hi); };
        for (int k = 0; k < m; k++) {
            const double t = 0.04 + 0.96 * invSmooth((k + 0.5) / m) + r.sym(0.015);
            ys[k] = clampi(start + (int)std::floor(span * clampd(t, 0, 1)), start, std::max(start, stop));
        }
        std::sort(ys.begin(), ys.end());
        for (int k = 0; k < m; k++) S.founded[list[k]] = ys[k];
    }
    for (int i = 0; i < n; i++) S.age[i] = prof.eras[std::max(0, eraOf[i])].age;
    if (prof.age == AGE_PREHISTORIC) S.founded[0] = 1;
    return S;
}
int64_t agoOf(const PeopleProfile& P, int year) { return (int64_t)std::llround(P.endedYearsAgo) + (P.lastYear - year); }
// `own` and `ownSchedule` are the asking people's (each history builds its own; the other's are built here), so both histories reach the
// same list
Joint jointOf(const StarSystem& sys, const Body& b, const BodyGen& g, const WorldPlan& WP, int ownPeople, const PeopleProfile& own, const Schedule& ownSchedule) {
    Joint J;
    if (WP.peoples < 2) return J;
    const int other = 1 - ownPeople;
    PeopleProfile P[2];
    P[ownPeople] = own; P[other] = peopleProfileOf(sys, b, g, other);
    const PeoplePlan* pp[2] = {&WP.plans[0], &WP.plans[1]};
    if (pp[0]->places.empty() || pp[1]->places.empty()) return J;
    // the years both lived in: from the later year 1 to the earlier end
    const int64_t from = std::min(agoOf(P[0], 1), agoOf(P[1], 1)), to = std::max(agoOf(P[0], P[0].lastYear), agoOf(P[1], P[1].lastYear));
    if (from <= to) return J;
    // a people the other conquered (its end a conquest or a war, the other going on and alive when its collapse began)
    int conquered = -1;
    for (int k = 0; k < 2; k++) {
        const int c2 = P[k].cause;
        const bool otherOn = agoOf(P[1 - k], P[1 - k].lastYear) < agoOf(P[k], P[k].lastYear) && agoOf(P[1 - k], 1) > agoOf(P[k], P[k].collapseYear);
        if (otherOn && (c2 == CAUSE_CONQUEST || c2 == CAUSE_BAND_WAR || c2 == CAUSE_INDUSTRIAL_WAR)) conquered = k;
    }
    const int64_t conquestFrom = conquered >= 0 ? agoOf(P[conquered], P[conquered].collapseYear) : INT64_MIN;
    // could they meet: lands that touch, or a people that sailed (the medieval age and later) by the time of the meeting
    bool touch = false;
    for (int c = 0; c < WP.grid.cells && !touch; c++) {
        if (!pp[0]->territory[c]) continue;
        int nb[8]; const int k = WP.grid.neighbours(c, nb);
        for (int t = 0; t < k; t++) if (pp[1]->territory[nb[t]]) { touch = true; break; }
    }
    Schedule S[2];
    S[ownPeople] = ownSchedule; S[other] = foundingSchedule(g, WP.grid, *pp[other], P[other], other);
    Rng r(g.seed ^ 0x40C13ULL);
    int64_t meet = from - (int64_t)(touch ? r.range(0, 40) : r.range(30, 300));
    if (!touch) {   // by sea: once one of them sailed (its age at that time the medieval or later)
        int64_t sail = INT64_MIN;
        for (int k = 0; k < 2; k++) for (const Era& e : P[k].eras) if (e.age >= AGE_MEDIEVAL) { sail = std::max(sail, agoOf(P[k], e.start)); break; }
        meet = sail == INT64_MIN ? INT64_MIN : std::min(meet, sail - (int64_t)r.range(50, 400));
    }
    // a conquest needs the conqueror to have come: a meeting a few decades before it at the latest
    if (conquered >= 0 && meet <= conquestFrom) meet = std::min(from, conquestFrom + (int64_t)r.range(10, 80));
    if (meet <= to) return J;
    // the frontier: the pair of places nearest each other, and the next few pairs
    struct Pair { double d; int a, b; };   // d: the negated dot (the nearer the smaller)
    auto nearer = [](const Pair& x, const Pair& y) { return x.d != y.d ? x.d < y.d : (x.a != y.a ? x.a < y.a : x.b < y.b); };
    std::vector<Pair> pairs;   // a heap of the 24 nearest so far, the farthest of them on top
    for (int a = 0; a < (int)pp[0]->places.size(); a++) for (int c = 0; c < (int)pp[1]->places.size(); c++) {
        const Pair q{-dot(pp[0]->places[a].unit, pp[1]->places[c].unit), a, c};
        if (pairs.size() < 24) { pairs.push_back(q); std::push_heap(pairs.begin(), pairs.end(), nearer); }
        else if (nearer(q, pairs.front())) { std::pop_heap(pairs.begin(), pairs.end(), nearer); pairs.back() = q; std::push_heap(pairs.begin(), pairs.end(), nearer); }
    }
    std::sort(pairs.begin(), pairs.end(), nearer);
    auto standing = [&](int k, int place, int64_t ago) { const int y = yearOf(P[k], ago); return y >= S[k].founded[place] && y <= P[k].lastYear; };
    const int nearN = (int)pairs.size();
    auto pairAt = [&](int64_t ago, int salt) {   // one of the nearest pairs that stood then; else the nearest pair that did, kept with them
        for (int t = 0; t < nearN; t++) {
            const int i = (t + salt) % nearN;
            if (standing(0, pairs[i].a, ago) && standing(1, pairs[i].b, ago)) return i;
        }
        Pair best{1e9, -1, -1};
        for (int a = 0; a < (int)pp[0]->places.size(); a++) {
            if (!standing(0, a, ago)) continue;
            for (int c = 0; c < (int)pp[1]->places.size(); c++) {
                if (!standing(1, c, ago)) continue;
                const Pair q{-dot(pp[0]->places[a].unit, pp[1]->places[c].unit), a, c};
                if (nearer(q, best)) best = q;
            }
        }
        if (best.a < 0) return -1;
        for (int i = nearN; i < (int)pairs.size(); i++) if (pairs[i].a == best.a && pairs[i].b == best.b) return i;
        pairs.push_back(best);
        return (int)pairs.size() - 1;
    };
    int pi = pairAt(meet, 0);
    if (pi < 0) { pi = 0; meet = std::min(meet, std::min(agoOf(P[0], S[0].founded[pairs[0].a]), agoOf(P[1], S[1].founded[pairs[0].b])) - 1); if (meet <= to) return J; }
    J.met = true;
    JointEvent c; c.kind = HE_CONTACT; c.ago = meet; c.place[0] = pairs[pi].a; c.place[1] = pairs[pi].b; c.salience = 0.8f;
    J.events.push_back(c);
    // the years after: treaties and wars by what both valued; the one that went on conquered the other where its cause says so
    const double warW = 0.2 + P[0].values[VAL_WAR] + P[1].values[VAL_WAR], tradeW = 0.2 + P[0].values[VAL_TRADE] + P[1].values[VAL_TRADE];
    for (int64_t ago = meet - 1; ago > to; ago--) {
        if (ago <= conquestFrom) break;
        if (r.chance(tradeW / 260.0)) {
            const int q = pairAt(ago, r.irange(4)); if (q < 0) continue;
            JointEvent t; t.kind = HE_TREATY; t.ago = ago; t.place[0] = pairs[q].a; t.place[1] = pairs[q].b; t.out = r.irange(3); t.salience = 0.45f;
            J.events.push_back(t);
        }
        if (r.chance(warW / 160.0)) {
            JointWar w; w.kind = HW_OTHER; w.from = ago; w.to = std::max(to + 1, ago - (int64_t)std::lround(std::exp(r.range(0, std::log(8.0)))) + 1);
            w.aggressor = r.irange(2); w.name = mix64(g.seed ^ (0x40C1AULL + J.wars.size()));
            const double strength = std::log(std::max(1.0, P[0].peak) / std::max(1.0, P[1].peak));
            w.winner = r.chance(0.2) ? -1 : (r.chance(1.0 / (1.0 + std::exp(-0.6 * strength))) ? 0 : 1);
            const int wi = (int)J.wars.size(); J.wars.push_back(w);
            const int q0 = pairAt(ago, r.irange(3)); if (q0 < 0) { J.wars.pop_back(); continue; }
            JointEvent d; d.kind = HE_WAR; d.ago = ago; d.war = wi; d.place[0] = pairs[q0].a; d.place[1] = pairs[q0].b; d.salience = 0.8f; J.events.push_back(d);
            const int nb = 1 + r.irange(3);
            for (int k = 0; k < nb; k++) {
                const int64_t ba = w.from - (int64_t)((w.from - w.to) * (k + 0.5) / nb);
                const int q = pairAt(ba, r.irange(6)); if (q < 0) continue;
                JointEvent bt; bt.kind = HE_BATTLE; bt.ago = ba; bt.war = wi; bt.place[0] = pairs[q].a; bt.place[1] = pairs[q].b; bt.salience = 0.6f;
                bt.out = w.winner >= 0 && r.chance(0.7) ? w.winner : r.irange(2);
                const int fought = std::max(P[0].eras[P[0].eraAt(yearOf(P[0], ba))].age, P[1].eras[P[1].eraAt(yearOf(P[1], ba))].age);
                bt.value = (int64_t)std::min(std::min(P[0].peak, P[1].peak) * r.range(2e-4, 3e-3), BATTLE_CAP[fought] * r.range(0.15, 1.0));
                J.wars[wi].dead += bt.value;
                bt.season = 1 + r.irange(2);
                J.events.push_back(bt);
            }
            JointEvent pe; pe.kind = HE_PEACE; pe.ago = w.to; pe.war = wi; pe.out = w.winner < 0 ? 2 : w.winner; pe.salience = 0.6f;
            const int qp = pairAt(w.to, 0); if (qp >= 0) { pe.place[0] = pairs[qp].a; pe.place[1] = pairs[qp].b; }
            J.events.push_back(pe);
            ago = w.to;
        }
    }
    // the conquest: through the conquered people's collapse, the conqueror taking its places from the frontier in to its capital
    if (conquered >= 0 && conquestFrom < meet) {
        const int k = conquered;
        JointWar w; w.kind = HW_CONQUEST; w.from = conquestFrom; w.to = agoOf(P[k], P[k].lastYear); w.winner = 1 - k; w.aggressor = 1 - k; w.name = mix64(g.seed ^ 0x40C0AULL);
        const int wi = (int)J.wars.size(); J.wars.push_back(w);
        const int q0 = std::max(0, pairAt(w.from, 0));
        JointEvent d; d.kind = HE_WAR; d.ago = w.from; d.war = wi; d.place[0] = pairs[q0].a; d.place[1] = pairs[q0].b; d.salience = 0.9f; J.events.push_back(d);
        const int nb = std::max(2, std::min(8, (int)((w.from - w.to) / 3) + 1));
        for (int t = 0; t < nb; t++) {
            const int64_t ba = w.from - (int64_t)((w.from - w.to) * (t + 0.5) / nb);
            const int q = pairAt(ba, t); if (q < 0) continue;
            JointEvent bt; bt.kind = HE_BATTLE; bt.ago = ba; bt.war = wi; bt.place[0] = pairs[q].a; bt.place[1] = pairs[q].b; bt.out = r.chance(0.8) ? 1 - k : k; bt.salience = 0.7f;
            const int fought = std::max(P[0].eras[P[0].eraAt(yearOf(P[0], ba))].age, P[1].eras[P[1].eraAt(yearOf(P[1], ba))].age);
            bt.value = (int64_t)std::min(P[k].peak * r.range(5e-4, 4e-3), BATTLE_CAP[fought] * r.range(0.15, 1.0)); J.wars[wi].dead += bt.value; bt.season = 1 + r.irange(2);
            J.events.push_back(bt);
        }
    }
    // word of the other's end, where one went on (the meeting before it)
    const int64_t end0 = agoOf(P[0], P[0].lastYear), end1 = agoOf(P[1], P[1].lastYear);
    if (end0 != end1) {
        const int gone = end0 > end1 ? 0 : 1;
        const int64_t when = std::max(std::min(end0, end1) + 1, std::max(end0, end1) - (int64_t)r.range(1, 30));
        JointEvent o; o.kind = HE_OTHER_END; o.ago = when; o.place[gone] = -1; o.out = gone; o.salience = 0.7f;
        if (when < meet) J.events.push_back(o);
    }
    return J;
}
}   // namespace

void Builder::setup() {
    const PeopleProfile& P = H.profile;
    H.seed = P.seed; H.age = P.age; H.cause = P.cause;
    H.last = P.lastYear;
    gene = genomeOf(P.seed);
    W = readWorld(sys, body, g);
    rng = Rng(P.seed ^ 0x40A11ULL);
    H.seasons = std::fabs(body.axialTilt) / DEG > 8 ? 4 : 2;
    primo = rng.pick(std::vector<double>{0.45, 0.35, 0.2}.data(), 3);
    residence = rng.chance(0.7) ? 0 : 1;
    static const int TERMS[6] = {3, 4, 4, 5, 6, 7};
    termRepublic = TERMS[rng.irange(6)]; termAssembly = 3 + rng.irange(4);
    // the eras: A-01's, named, with the faith and what they lived by (the last era's the profile's, the earlier drawn by their age, often kept)
    H.eras.clear();
    for (size_t k = 0; k < P.eras.size(); k++) {
        const Era& e = P.eras[k];
        HistoryEra he; he.age = e.age; he.start = e.start; he.end = e.end; he.government = e.government;
        he.govFrom = k == 0 ? (e.age == AGE_PREHISTORIC ? GOV_CHIEFS : e.government) : P.eras[k - 1].government;
        H.eras.push_back(he);
    }
    for (int k = (int)H.eras.size() - 1; k >= 0; k--) {
        HistoryEra& he = H.eras[k];
        if (k == (int)H.eras.size() - 1) { he.faith = P.faith; he.economy = P.economy; continue; }
        const HistoryEra& nx = H.eras[k + 1];
        static const double FW[AGE_COUNT][FAITH_COUNT] = {{4, 4, 3, 1, 0.2, 0, 0}, {2, 1, 1, 4, 4, 0.5, 0}, {1, 0.3, 0.3, 2, 4, 2, 0}, {1, 0.2, 0.2, 1, 3, 4, 0.2}, {1, 0.2, 0.2, 0.3, 1, 5, 2}, {0.2, 0.2, 1, 0.2, 0.2, 3, 3}};
        static const double EW[AGE_COUNT][ECO_COUNT] = {{4, 3, 3, 0.3, 0, 0}, {0.3, 2, 6, 2, 0.2, 0}, {0, 0.3, 2, 2, 6, 0}, {0, 0, 0.5, 3, 4, 3}, {0, 0, 0.2, 1, 3, 6}, {0, 0, 0.1, 0.5, 0.5, 8}};
        he.faith = rng.chance(0.45) && FW[he.age][nx.faith] > 0 ? nx.faith : rng.pick(FW[he.age], FAITH_COUNT);
        he.economy = rng.pick(EW[he.age], ECO_COUNT);
    }
    // the first government of a medieval people's first era is often the chiefs' (the kingdom founded later in it)
    for (size_t k = 0; k < H.eras.size(); k++) {
        HistoryEra& he = H.eras[k];
        if (k > 0 && H.eras[k - 1].age == AGE_PREHISTORIC && he.age >= AGE_MEDIEVAL && rng.chance(0.5)) he.govFrom = GOV_CHIEFS;
        if (he.age == AGE_PREHISTORIC && H.age == AGE_PREHISTORIC) he.govFrom = GOV_CHIEFS;
        const bool oldTimeOnly = he.age == AGE_PREHISTORIC && H.age != AGE_PREHISTORIC;
        if (he.govFrom != he.government && !oldTimeOnly) {
            const int len = he.end - he.start + 1;
            int stop = he.end;
            if (k == H.eras.size() - 1) stop = std::min(stop, P.collapseYear - 1);
            he.govChange = clampi(he.start + (int)std::lround(len * rng.range(0.12, 0.65)), he.start + 1, std::max(he.start + 1, stop));
        }
    }
    // the names: the government's and the values' first where they speak, else the age's own
    for (size_t k = 0; k < H.eras.size(); k++) {
        HistoryEra& he = H.eras[k];
        const int a = he.age;
        int pickIdx = (int)(unitFromHash(mix64(P.seed ^ (0x40E4AULL + k))) * 6);
        if (a == AGE_MEDIEVAL) {
            if (he.government == GOV_MONARCHY) pickIdx = rng.chance(0.5) ? 0 : 1;
            else if (he.government == GOV_THEOCRACY) pickIdx = 2;
            else if (he.government == GOV_COUNCIL) pickIdx = 3;
            else if (P.values[VAL_LEARNING] > 0.3) pickIdx = 4;
            else if (P.values[VAL_TRADE] > 0.3) pickIdx = 5;
        }
        if (a == AGE_INDUSTRIAL && he.government == GOV_CORPORATE) pickIdx = 5;
        he.name = ERA_NAMES[a][clampi(pickIdx, 0, 5)];
        if (a == AGE_MEDIEVAL && he.government == GOV_MONARCHY && pickIdx == 0) he.name = "the Kingdom of " + H.lore.city;
        if (a == AGE_PREHISTORIC && H.age == AGE_PREHISTORIC) { static const char* const OWN[4] = {"the Time of the Fires", "the Gathering", "the Years of the Bands", "the Herds' Years"}; he.name = OWN[pickIdx % 4]; }
        {   // H-04's second pass: half the peoples' eras take one of the age's further names (their own way of naming them; the draws above stand)
            const uint64_t eh = mix64(P.seed ^ 0xE4A5ULL ^ (uint64_t)(k + 1) * 0x9E3779B97F4A7C15ULL);
            if (unitFromHash(eh) < 0.55) {
                std::string nm = ERA_MORE[clampi(a, 0, AGE_COUNT - 1)][(eh >> 20) % 10];
                auto fill = [&](const char* key, const std::string& v) { size_t at = nm.find(key); if (at != std::string::npos) nm.replace(at, std::strlen(key), v); };
                fill("{city}", H.lore.city); fill("{god}", H.lore.god); fill("{river}", H.lore.river); fill("{mountain}", H.lore.mountain); fill("{founder}", H.lore.founder);
                fill("{moon}", H.lore.moons > 0 ? H.lore.moon : H.lore.star);
                he.name = nm;
            }
        }
    }
    // the years told: the old time's start (a prehistoric people's coming, a few hundred years before its first village)
    H.first = P.eras.front().start;
    if (H.age == AGE_PREHISTORIC) H.first = 1 - 100 * (3 + rng.irange(28));
    H.famFrom = std::max(1, H.last - FAMILY_YEARS);
    // the sky: the comet that returns in a span the people could count, the lights
    const double yearSec = std::max(1.0, body.orbitPeriod);
    double bestP = 0;
    for (const Body& o : sys.bodies) {
        if (o.type != PT_COMET) continue;
        const double per = o.orbitPeriod / yearSec;
        if (per >= 30 && per <= 2500 && (bestP == 0 || per < bestP)) bestP = per;
    }
    if (bestP > 0) { cometPeriod = (int)std::lround(bestP); cometPhase = (int)(unitFromHash(mix64(P.seed ^ 0x40C0EULL)) * cometPeriod); }
    auroral = auroraPotentialAt(sys, body, 0.7) > 0.25;
}

void Builder::setupPlaces() {
    const PeopleProfile& P = H.profile;
    const int n = PP ? (int)PP->places.size() : 0;
    sched = n ? foundingSchedule(g, *G, *PP, P, people, &ageScores) : Schedule();
    std::vector<std::string> used = {H.lore.people, H.lore.god, H.lore.river, H.lore.mountain, H.lore.sea, H.lore.city, H.lore.city2, H.lore.founder, H.lore.moon, H.lore.star, H.lore.festival};
    const Tongue& TG = tongueOfSeed(H.lore.seed);   // H-04's second pass: the places' names in the people's tongue, by their rank
    auto uniqueName = [&](uint64_t seed, int sclass) {
        for (int t = 0; t < 24; t++) {
            const uint64_t s = mix64(seed + t * 0x9E3779B97F4A7C15ULL);
            const std::string nm = tongueName(TG, s, tongueNameKindOfRank(sclass));
            if (std::find(used.begin(), used.end(), nm) == used.end()) { used.push_back(nm); return s; }
        }
        return mix64(seed ^ 0x55);
    };
    const bool farFuture = H.age == AGE_FAR_FUTURE;
    for (int i = 0; i < n; i++) {
        const PlanPlace& p = PP->places[i];
        HistoryPlace hp;
        hp.plan = i; hp.sclass = p.sclass; hp.lat = p.lat; hp.lon = p.lon; hp.peak = p.population; hp.coast = p.coast; hp.river = p.river;
        hp.founded = sched.founded[i]; hp.age = sched.age[i]; hp.ended = H.last;
        if (!farFuture && i == 0) hp.name = NAME_CITY;
        else if (!farFuture && i == 1) hp.name = NAME_CITY2;
        else hp.name = uniqueName(p.seed ^ 0x40A3EULL, p.sclass);
        H.places.push_back(hp);
    }
    // a futuristic or far-future people's older cities, which its plan no longer draws: the land it cleared, kept as places
    if (H.age >= AGE_FUTURISTIC && PP && !PP->territory.empty()) {
        if (ageScores.size() < AGE_COUNT) ageScores.resize(AGE_COUNT);
        if (ageScores[AGE_MEDIEVAL].empty()) planScores(g, *G, people, AGE_MEDIEVAL, ageScores[AGE_MEDIEVAL], &PP->territory);
        const std::vector<float>& sm = ageScores[AGE_MEDIEVAL];
        std::vector<std::pair<double, int>> cand;
        for (int c = 0; c < G->cells; c++) if (PP->territory[c] && sm[c] > 0.25f) cand.push_back({sm[c] + 0.2 * unitFromHash(hash2i(c, 0x40C1, P.seed)), c});
        std::sort(cand.begin(), cand.end(), [](const std::pair<double, int>& a, const std::pair<double, int>& b) { return a.first != b.first ? a.first > b.first : a.second < b.second; });
        const int want = farFuture ? 16 + rng.irange(10) : 12 + rng.irange(10);
        std::vector<Vec3> taken;
        for (const PlanPlace& p : PP->places) taken.push_back(p.unit);
        const size_t planN = taken.size();
        const double minLost = 220.0 / g.R, minPlan = 110.0 / g.R;
        int made = 0;
        for (const auto& cv : cand) {
            if (made >= want) break;
            const Vec3 u = G->unitOf(cv.second);
            bool near = false;
            for (size_t t = 0; t < taken.size() && !near; t++) near = std::acos(clampd(dot(u, taken[t]), -1, 1)) < (t < planN ? minPlan : minLost);
            if (near) continue;
            taken.push_back(u);
            HistoryPlace hp;
            hp.plan = -1; hp.fate = HP_CLEARED;
            hp.sclass = made == 0 && farFuture ? SC_CAPITAL : (made < want / 3 ? SC_CITY : SC_TOWN);
            G->latLonOf(cv.second, hp.lat, hp.lon);
            hp.coast = G->coastKm[cv.second] < 1.5 * G->sizeKm(); hp.river = G->river[cv.second] > 15;
            hp.peak = H.profile.peak * (hp.sclass == SC_CAPITAL ? 0.02 : (hp.sclass == SC_CITY ? 0.006 : 0.0015));
            hp.name = made == 0 && farFuture ? NAME_CITY : (made == 1 && farFuture ? NAME_CITY2 : uniqueName(mix64(P.seed ^ (0x40C2ULL + cv.second)), hp.sclass));
            H.places.push_back(hp);
            made++;
        }
        // founded through the older ages (the medieval most), cleared in the people's own (the far future's old capital the last)
        int firstHist = 0; while (firstHist + 1 < (int)H.eras.size() && H.eras[firstHist].age == AGE_PREHISTORIC) firstHist++;
        int clearEra = (int)H.eras.size() - 1; while (clearEra > 0 && H.eras[clearEra].age > AGE_FUTURISTIC) clearEra--;
        const HistoryEra& CE = H.eras[clearEra];
        const HistoryEra& LE = H.eras.back();
        for (int i = n; i < (int)H.places.size(); i++) {
            HistoryPlace& hp = H.places[i];
            const bool oldCap = farFuture && i == n;
            int e = firstHist;
            if (!oldCap && rng.chance(0.25)) e = std::min((int)H.eras.size() - 1, firstHist + 1 + rng.irange(2));
            if (H.eras[e].age >= H.age) e = firstHist;
            const HistoryEra& FE = H.eras[e];
            hp.founded = oldCap ? 1 : clampi(FE.start + (int)(rng.range(0.02, 0.6) * (FE.end - FE.start + 1)), std::max(2, FE.start), std::max(2, FE.end));
            hp.age = FE.age;
            const HistoryEra& when = oldCap ? LE : (farFuture && rng.chance(0.5) ? LE : CE);
            const int stop = std::min(when.end, P.collapseYear - 1);
            hp.ended = clampi(when.start + (int)(rng.range(0.1, oldCap ? 0.5 : 0.8) * (stop - when.start + 1)), std::max(hp.founded + 20, when.start), std::max(hp.founded + 20, stop));
            hp.ended = std::min(hp.ended, P.collapseYear - 1);
            if (hp.ended <= hp.founded) hp.ended = std::min(P.collapseYear - 1, hp.founded + 50);
        }
        if (farFuture && (int)H.places.size() > n) seatOld = n;
    }
    unit.clear();
    for (const HistoryPlace& hp : H.places) unit.push_back(StarSystem::bodyFromLatLon(hp.lat, hp.lon));
    // the settlers' places: the nearest standing before each founding
    for (int i = 0; i < (int)H.places.size(); i++) {
        HistoryPlace& hp = H.places[i];
        int best = -1; double bd = 1e300;
        for (int j = 0; j < (int)H.places.size(); j++) {
            if (j == i || H.places[j].founded >= hp.founded || H.places[j].ended < hp.founded) continue;
            const double d = -dot(unit[i], unit[j]);
            if (d < bd) { bd = d; best = j; }
        }
        hp.from = best;
    }
    // the ways: the year each of the plan's opened (both its places standing, the kind's invention made)
    H.wayOpened.assign(PP ? PP->edges.size() : 0, 0);
}

void Builder::setupCollapse() {
    const PeopleProfile& P = H.profile;
    // the stages' first years
    H.stageYear[STAGE_WARNING] = P.collapseYear;
    for (int s = 1; s < STAGE_COUNT; s++) {
        int y = H.stageYear[s - 1];
        while (y < H.last && P.stageAt(y) < s) y++;
        H.stageYear[s] = y;
    }
    // the order the places were left in by the cause, the last place the least exposed
    const CauseScript& CS = SCRIPTS[H.cause];
    std::vector<std::pair<double, int>> exp;
    double latMax = 1e-9;
    for (const HistoryPlace& p : H.places) latMax = std::max(latMax, std::fabs(p.lat));
    for (int i = 0; i < (int)H.places.size(); i++) {
        const HistoryPlace& p = H.places[i];
        if (p.ended < P.collapseYear) continue;   // cleared before
        double x = 0;
        const bool capital = i == 0 || p.sclass == SC_CAPITAL;
        const double remote = p.plan >= 0 ? PP->places[p.plan].remote : 0.5;
        const double sz = p.sclass == SC_CAPITAL ? 1 : (p.sclass == SC_CITY ? 0.7 : (p.sclass == SC_TOWN ? 0.4 : 0.2));
        double h = 0; if (p.plan >= 0) h = G->height[PP->places[p.plan].cell];
        switch (CS.order) {
            case OR_REMOTE: x = remote - (capital ? 1 : 0); break;
            case OR_COAST: x = (p.coast ? 0.6 : 0) + (p.river ? 0.3 : 0) - h / 3000.0; break;
            case OR_COLD: x = std::fabs(p.lat) / latMax + h / 4000.0; break;
            case OR_DRY: x = (p.river ? -0.5 : 0) + (p.coast ? -0.2 : 0) + 0.5 * remote; break;
            case OR_CROWDED: x = sz + (p.coast ? 0.15 : 0); break;
            case OR_SMALL: x = 1 - sz - (capital ? 0.5 : 0); break;
            case OR_WARM: {   // D-05: the high and the cold first, the warmest ground last (a field of embers, the deep valleys)
                x = h / 2000.0 + 0.3 * std::fabs(p.lat) / latMax;
                if (g.hasTrait(TR_EMBERS)) x -= 2.0 * emberFieldAt(g, StarSystem::bodyFromLatLon(p.lat, p.lon) * g.R);
                break;
            }
            default: x = 0; break;
        }
        x += 0.25 * unitFromHash(mix64(P.seed ^ (0x40C7ULL + i)));
        if (H.age == AGE_FAR_FUTURE && p.plan >= 0) x -= 5;   // a far-future people's works outlast everything else
        exp.push_back({x, i});
    }
    std::sort(exp.begin(), exp.end(), [](const std::pair<double, int>& a, const std::pair<double, int>& b) { return a.first != b.first ? a.first > b.first : a.second < b.second; });
    if (H.cause == CAUSE_SUN_LOST) {   // the omens of its way through the warning, two to four (a stream of its own)
        Rng ro(P.seed ^ 0xD05011ULL);
        const int from = H.stageYear[STAGE_WARNING], to = std::max(from, H.stageYear[STAGE_ONSET] - 1), n = 2 + ro.irange(3);
        for (int k = 0; k < n; k++) sunOmens.push_back(from + (int)std::lround((to - from) * (k + ro.range(0.1, 0.9)) / n));
    }
    if (exp.empty()) return;
    H.lastPlace = exp.back().second;
    const int C = H.stageYear[STAGE_CRISIS], L = H.last;
    const int m = (int)exp.size();
    for (int k = 0; k < m; k++) {
        const int i = exp[k].second;
        int y = L;
        if (k < m - 1) y = C + (int)std::floor((L - C) * std::pow((k + 0.5) / m, 0.7));
        y = clampi(y, std::max(C, H.places[i].founded), L);
        if (k < m - 1 && y >= L && L > C) y = L - 1;
        y = std::max(y, H.places[i].founded);
        H.places[i].ended = y;
        H.places[i].fate = k == m - 1 ? HP_LAST : HP_COLLAPSE;
        endsByYear[y].push_back(i);
    }
}

void Builder::setupPlanned() {
    const PeopleProfile& P = H.profile;
    // the inventions: each age's, at its share of the whole age, as far as the people got (radio at its year)
    for (size_t k = 0; k < H.eras.size(); k++) {
        const HistoryEra& he = H.eras[k];
        if (he.age == AGE_PREHISTORIC && H.age != AGE_PREHISTORIC) continue;
        const double full = he.age == AGE_PREHISTORIC ? std::max(400.0, (double)H.last) : std::max(1.0, P.fullYears[he.age]);
        // D-05: a people that lost its sun went on making things while it dimmed (its radio among them), to the last sunrise
        const int stop = std::min(he.end, P.cause == CAUSE_SUN_LOST && P.lastSunrise > 0 ? P.lastSunrise : P.collapseYear - 1);
        for (int i = 0; i < INVENTION_COUNT; i++) {
            const InventionInfo& I = INVENTIONS[i];
            if (I.age != he.age) continue;
            int y;
            if (I.at < -5) { if (!P.radio || P.radioYear <= 0) continue; y = P.radioYear; }
            else y = he.start + (int)std::lround(I.at * full);
            if (y < 1) y = 1 + rng.irange(20);   // writing: with the first towns
            if (y > stop) continue;
            planned.push_back({y, HE_INVENTION, i, 0});
        }
    }
    // the institutions, at the eras' and the governments' turns
    auto inst = [&](int kind, int y) { if (y >= std::max(1, H.first) && y < P.collapseYear) planned.push_back({y, HE_INSTITUTION, kind, 0}); };
    for (size_t k = 0; k < H.eras.size(); k++) {
        const HistoryEra& he = H.eras[k];
        const int len = he.end - he.start + 1;
        auto at = [&](double lo, double hi) { return he.start + (int)std::lround(len * rng.range(lo, hi)); };
        if (he.age == AGE_MEDIEVAL) {
            if (P.values[VAL_PIETY] > 0.15) inst(HI_PRIESTHOOD, at(0.03, 0.25));
            if (P.values[VAL_TRADE] > 0.12) inst(HI_GUILDS, at(0.25, 0.6));
            if (P.values[VAL_LEARNING] > 0.12) inst(HI_ACADEMY, at(0.35, 0.8));
            if (P.values[VAL_WAR] > 0.22 && he.government == GOV_MONARCHY) inst(HI_ARMY, at(0.2, 0.7));
        }
        if (he.age == AGE_INDUSTRIAL) {
            inst(HI_COMPANY, at(0.04, 0.3));
            inst(HI_GAZETTE, at(0.08, 0.3));
            if (P.values[VAL_LEARNING] > 0.1) inst(HI_UNIVERSITY, at(0.15, 0.5));
        }
        if (he.age >= AGE_INDUSTRIAL && P.radio && P.radioYear > 0 && P.radioYear >= he.start && P.radioYear <= he.end) inst(HI_BROADCASTER, P.radioYear + 3 + rng.irange(15));
        if (he.age == AGE_MODERN) inst(HI_NETWORK, at(0.5, 0.75));
        if (he.age == AGE_FUTURISTIC) inst(HI_MIND, at(0.0, 0.15));
        if (he.govChange > 0) {
            if (he.government == GOV_COUNCIL) inst(HI_COUNCIL, he.govChange);
            if (he.government == GOV_ASSEMBLY || he.government == GOV_REPUBLIC) inst(HI_ASSEMBLY, he.govChange);
        } else if (k == 0 || H.eras[k - 1].age == AGE_PREHISTORIC) {
            if (he.government == GOV_COUNCIL) inst(HI_COUNCIL, std::max(1, he.start + 2));
            if (he.government == GOV_ASSEMBLY || he.government == GOV_REPUBLIC) inst(HI_ASSEMBLY, std::max(1, he.start + 2));
        }
    }
    // the faiths: a new one preached where an era's faith was not the one before
    for (size_t k = 1; k < H.eras.size(); k++) {
        const HistoryEra& he = H.eras[k];
        if (he.faith == H.eras[k - 1].faith) continue;
        const int len = he.end - he.start + 1;
        const int y = he.start + (int)std::lround(len * rng.range(-0.04, 0.18));
        if (y >= 1 && y < P.collapseYear) planned.push_back({y, HE_FAITH, 0, he.faith});
    }
    // the discoveries
    int firstHist = 0; while (firstHist + 1 < (int)H.eras.size() && H.eras[firstHist].age == AGE_PREHISTORIC && H.age != AGE_PREHISTORIC) firstHist++;
    for (size_t k = 0; k < H.eras.size(); k++) {
        const HistoryEra& he = H.eras[k];
        const int len = he.end - he.start + 1;
        const int stop = std::min(he.end, P.collapseYear - 1);
        auto at = [&](double lo, double hi) { return he.start + (int)std::lround(len * rng.range(lo, hi)); };
        auto disc = [&](int kind, int y) { if (y >= 1 && y <= stop) planned.push_back({y, HE_DISCOVERY, kind, 0}); };
        if (he.age == AGE_MEDIEVAL) {
            if (!H.places.empty() && H.places[0].river && rng.chance(0.6)) disc(1, at(0.15, 0.6));
            if (rng.chance(0.45)) disc(2, at(0.1, 0.7));
            if (P.values[VAL_LEARNING] > 0.15) { const double full = std::max(1.0, P.fullYears[AGE_MEDIEVAL]); disc(3, he.start + (int)std::lround(full * rng.range(0.7, 0.95))); }
        }
        if (he.age == AGE_INDUSTRIAL) { if (P.landShare < 0.85) disc(4, at(0.3, 0.6)); disc(5, at(0.55, 0.95)); }
        if (he.age == AGE_FUTURISTIC) disc(8, at(0.25, 0.6));
    }
    // the far shore: the first place founded over the water from an older one
    if (PP) {
        int best = -1; int by = INT_MAX;
        for (size_t e = 0; e < PP->edges.size(); e++) {
            const PlanEdge& pe = PP->edges[e];
            if (pe.kind != NK_SEA) continue;
            const int a = pe.a, b = pe.b;
            const int later = H.places[a].founded > H.places[b].founded ? a : b;
            if (H.places[later].founded < by && H.places[later].founded > 1) { by = H.places[later].founded; best = later; }
        }
        if (best >= 0) planned.push_back({std::max(1, by - 1 - rng.irange(4)), HE_DISCOVERY, 0, best});
    }
    // the ways: the year each opened
    if (PP) {
        const int railway = inventionIndex("the railway");
        int railYear = 0;
        for (const Planned& q : planned) if (q.kind == HE_INVENTION && q.a == railway) railYear = q.year;
        int indStart = 0, modStart = 0, futStart = 0;
        for (const HistoryEra& he : H.eras) { if (he.age == AGE_INDUSTRIAL) indStart = he.start; if (he.age == AGE_MODERN) modStart = he.start; if (he.age == AGE_FUTURISTIC) futStart = he.start; }
        // a people that lost its sun built on while it dimmed (its ways to the last sunrise, as its inventions); its plan's railways came
        // after the railway (where its age reached it too late to be invented, it was at the first line's year)
        const bool sunlost = P.cause == CAUSE_SUN_LOST && P.lastSunrise > 0;
        const int wayStop = sunlost ? P.lastSunrise : P.collapseYear - 1;
        if (sunlost && railYear == 0 && indStart > 0) {
            bool rails = false; for (const PlanEdge& pe : PP->edges) if (pe.kind == NK_RAIL) rails = true;
            if (rails) { railYear = std::max(1, std::min(indStart + 10, wayStop)); planned.push_back({railYear, HE_INVENTION, railway, 0}); }
        }
        for (size_t e = 0; e < PP->edges.size(); e++) {
            const PlanEdge& pe = PP->edges[e];
            const HistoryPlace& A = H.places[pe.a]; const HistoryPlace& B = H.places[pe.b];
            int y = std::max(A.founded, B.founded) + 1;
            int lo = 1, hi = 30;
            switch (pe.kind) {
                case NK_RAIL: y = std::max(y, railYear > 0 ? railYear : indStart + 10); hi = 40; break;
                case NK_CANAL: y = std::max(y, indStart > 0 ? indStart : y); hi = 60; break;
                case NK_HIGHWAY: y = std::max(y, modStart > 0 ? modStart + 5 : y); hi = 30; break;
                case NK_MAGLEV: y = std::max(y, futStart > 0 ? futStart + 15 : y); hi = 120; break;
                default: break;
            }
            y += lo + rng.irange(hi);
            const int stop = std::min(std::min(A.ended, B.ended), wayStop);
            if (y > stop) y = std::max(std::max(A.founded, B.founded) + 1, stop);
            if (y > std::min(A.ended, B.ended)) y = std::max(A.founded, B.founded);
            H.wayOpened[e] = y;
            const bool great = big(pe.a) && big(pe.b);
            const bool untold = sunlost && pe.kind == NK_RAIL && y < railYear;   // a line between places left before the railway: its opening not told
            if (!untold && ((pe.trunk && great) || pe.kind == NK_CANAL || pe.kind == NK_RAIL || pe.kind == NK_MAGLEV || (pe.kind == NK_HIGHWAY && great))) waysByYear[y].push_back((int)e);
        }
    }
    // the collapse's signs: one or two in each stage, at a place by the cause's order
    for (int s = 0; s < STAGE_COUNT; s++) {
        if (s == STAGE_ONSET && SCRIPTS[H.cause].hazard == HZ_FIRE) continue;   // its harm is its sign
        const int from = H.stageYear[s], to = s + 1 < STAGE_COUNT ? std::max(from, H.stageYear[s + 1] - 1) : H.last;
        if (s == STAGE_END) { planned.push_back({H.last, HE_SIGN, s * 2 + rng.irange(2), s}); continue; }   // the end's sign closes the last year
        const int k = 1 + (to - from >= 6 ? 1 : 0);
        for (int t = 0; t < k; t++) planned.push_back({from + (to - from) * t / std::max(1, k) + (t > 0 ? rng.irange(std::max(1, (to - from) / 3)) : 0), HE_SIGN, s * 2 + (t + rng.irange(2)) % 2, s});
    }
    std::stable_sort(planned.begin(), planned.end(), [](const Planned& a, const Planned& b) { return a.year < b.year; });
    // the foundings, the clearings and the festivals by year; the year a place would have been left with no event to blame (an
    // exponential wait from its fortieth year at its rate: a band's seat, a mine town, a town on poor ground, a river's town)
    for (int p = 0; p < (int)H.places.size(); p++) {
        const HistoryPlace& hp = H.places[p];
        foundByYear[hp.founded].push_back(p);
        if (hp.fate == HP_CLEARED) clearByYear[hp.ended].push_back(p);
        const bool capital = p == 0 || p == seatOld;
        if (capital) for (int y = hp.founded + 100; y <= std::min(hp.ended, H.last); y += 100) festByYear[y].push_back(p);
        if (p == 0 || hp.plan < 0) continue;
        double rate = 0; int why = 0;
        const PlanPlace& pl = PP->places[hp.plan];
        if (H.age == AGE_PREHISTORIC) { rate = 1 / 2500.0; why = 6; }
        else if (hp.age == AGE_INDUSTRIAL && G->res[pl.cell] > 0.5) { rate = 1 / 1500.0; why = 3; }
        else if (hp.age == AGE_MEDIEVAL && pl.score < 0.3) { rate = 1 / 6000.0; why = 4; }
        else if (hp.river) { rate = 1 / 12000.0; why = 1; }
        if (hp.sclass == SC_CITY) rate *= 0.3;
        if (rate <= 0) continue;
        const double u = unitFromHash(mix64(H.seed ^ (0x40E1ULL + p)));
        const double wait = -std::log(1 - u * 0.999999) / rate;
        if (wait > 1e6) continue;
        const int y = hp.founded + 40 + (int)wait;
        if (y < P.collapseYear - 2) naturalEnds[y].push_back({p, why});
    }
    // the people's number: from the old time through each era's close to the peak at the warning, then to nothing at the end
    static const double DENS[AGE_COUNT] = {0.045, 4.9, 17.9, 42, 31.6, 7.1};
    const double peak = std::max(100.0, P.peak);
    anchors.clear();
    if (H.age == AGE_PREHISTORIC) { anchors.push_back({H.first, peak * 0.05}); anchors.push_back({1, peak * 0.15}); }
    else {
        anchors.push_back({H.first, peak * DENS[AGE_PREHISTORIC] / DENS[H.age] * 0.08});
        for (size_t k = 0; k + 1 < H.eras.size(); k++) anchors.push_back({H.eras[k].end, peak * DENS[H.eras[k].age] / DENS[H.age]});
    }
    if (H.stageYear[STAGE_WARNING] > anchors.back().first) anchors.push_back({H.stageYear[STAGE_WARNING], peak});
    anchors.push_back({H.last + 1, peak * 0.001});
}

void Builder::importJoint() {
    if (!WP || WP->peoples < 2) return;
    joint = jointOf(sys, body, g, *WP, people, H.profile, sched);
    if (!joint.met) return;
    jointWar.assign(joint.wars.size(), -1);
    jointLastBattle.assign(joint.wars.size(), -1);
    for (size_t i = 0; i < joint.events.size(); i++) {
        const int y = yearOf(H.profile, joint.events[i].ago);
        if (y < 1 || y > H.last) continue;
        jointByYear[y].push_back((int)i);
    }
}
