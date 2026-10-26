// E-10: the builder's events (history_builder.h): the old time before year 1, a year's random events, its everyday (H-04), the
// families' private years and the collapse's years
#include "history_builder.h"

// the old time before year 1, as the people remembered it (the years round)
void Builder::oldTime() {
    const int first = H.first;
    if (first >= 1) return;
    const int len = 1 - first;
    auto round = [&](double y) { const int step = len > 3000 ? 100 : (len > 600 ? 50 : 10); return std::min(-step, (int)(std::lround(y / step) * step)); };
    // whence they came: over the sea to an island people, down the rivers, from beyond the mountains, out of a quarter of the sky
    int whence = 2 + rng.irange(4);
    if (!H.places.empty()) {
        if (H.places[0].coast && rng.chance(0.4)) whence = 1;
        else if (H.places[0].river && rng.chance(0.4)) whence = 6;
        else if (W.desert && rng.chance(0.4)) whence = 7;
        else if (rng.chance(0.3)) whence = 0;
    }
    int e = ev(HE_COMING, first, -1, 0.6f, -1); E(e).thing = whence;
    const int eco = H.eras.front().economy;
    e = ev(HE_FIRST, round(first + len * rng.range(0.3, 0.5)), -1, 0.4f, -1); E(e).thing = eco == ECO_HERDING ? 1 : 0;
    if (!H.places.empty() && H.places[0].coast) { e = ev(HE_FIRST, round(first + len * rng.range(0.5, 0.7)), -1, 0.3f, -1); E(e).thing = 2; }
    e = ev(HE_FIRST, round(first + len * rng.range(0.55, 0.8)), -1, 0.3f, -1); E(e).thing = rng.chance(0.5) ? 3 : 4;
    // a great flood, a long drought or a long winter remembered
    const int k = W.desert ? HE_DROUGHT : (W.cold + W.ecc > 0.5 ? HE_WINTER : HE_FLOOD);
    e = ev(k, round(first + len * rng.range(0.1, 0.9)), -1, 0.45f, -1); E(e).value = k == HE_FLOOD ? 0 : 3 + rng.irange(30); E(e).thing = 0;
    if (H.age != AGE_PREHISTORIC && !H.places.empty()) {
        // the stones raised at the capital's site, and the first village there
        const int legend = person(round(first + len * 0.7) - 40, rng.chance(0.5), -1, -1, HR_MAKER, 0);
        H.persons[legend].died = round(first + len * 0.7) + 10;
        e = ev(HE_STONES, round(first + len * rng.range(0.65, 0.8)), -1, 0.45f, -1); E(e).a = legend; E(e).thing = gene.of(GV_MONUMENTS);
        if (E(e).year > H.persons[legend].died || E(e).year < H.persons[legend].born) E(e).year = H.persons[legend].born + 30;
        e = ev(HE_FIRST, -10 * (5 + rng.irange(55)), -1, 0.5f, -1); E(e).thing = 5;
    }
}

void Builder::randomEvents(int y) {
    const PeopleProfile& P = H.profile;
    const int a = ageAt(y);
    const int s = seat(y);
    const double* v = P.values;
    const int stage = P.stageAt(y);
    const bool calm = stage < STAGE_ONSET;
    const int standingN = (int)(standBig.size() + standSmall.size()), bigN = (int)standBig.size();   // a few left lately among them
    // war
    {
        static const double RATE[AGE_COUNT] = {1 / 90.0, 1 / 60.0, 1 / 50.0, 1 / 70.0, 1 / 400.0, 1 / 3000.0};
        static const double GOVF[GOV_COUNT] = {1.2, 1.2, 0.8, 0.9, 0.8, 0.7, 0.8, 0.2};
        double rate = RATE[a] * (0.4 + 2.5 * v[VAL_WAR]) * GOVF[govNow];
        if (reign >= 0 && H.reigns[reign].ruler >= 0 && (H.persons[H.reigns[reign].ruler].traits & (HT_CRUEL | HT_AMBITIOUS))) rate *= 1.4;
        if (calm && standingN >= 2 && warsOn.size() < 2 && rng.chance(rate)) {
            int kind = a == AGE_PREHISTORIC ? HW_BANDS : (a == AGE_MEDIEVAL ? (rng.chance(0.55) ? HW_REBELLION : HW_CITIES) : (rng.chance(0.6) ? HW_CIVIL : HW_REBELLION));
            int A = -1, Bp = -1;
            if (kind == HW_BANDS || kind == HW_CITIES) {
                A = pickPlace(y, [&](int p) { return big(p) ? 2.0 : 1.0; });
                Bp = A >= 0 ? pickPlace(y, [&](int p) { return p != A ? 1.0 / (1.0 + distKm(A, p) / 80.0) : 0.0; }) : -1;
                if (Bp < 0) { kind = HW_REBELLION; A = -1; }
            }
            if (kind == HW_REBELLION || kind == HW_CIVIL) {
                A = -1;
                Bp = pickPlace(y, [&](int p) { return p == s ? 0.0 : (H.places[p].plan >= 0 ? 0.3 + PP->places[H.places[p].plan].remote : 0.5) * (big(p) ? 2.0 : 1.0); });
            }
            if (Bp >= 0) startWar(y, kind, A, Bp, -1, -1);
        }
    }
    // sickness
    {
        static const double RATE[AGE_COUNT] = {1 / 200.0, 1 / 85.0, 1 / 70.0, 1 / 150.0, 1 / 700.0, 1 / 6000.0};
        if (calm && rng.chance(RATE[a] * (0.6 + 1.5 * v[VAL_TRADE]) * (1 + 0.01 * std::min(bigN, 40)))) {
            int sk = -1;
            for (int k = (int)H.sicknesses.size() - 1; k >= 0 && sk < 0; k--) if (y - H.sicknesses[k].last < 220 && !H.sicknesses[k].made && rng.chance(0.35)) sk = k;
            if (sk < 0) { HistorySickness S; S.name = mix64(H.seed ^ (0x405CULL + H.sicknesses.size() * 0x9E3779B97F4A7C15ULL)); S.first = y; H.sicknesses.push_back(S); sk = (int)H.sicknesses.size() - 1; }
            const int origin = pickPlace(y, [&](int p) { return (H.places[p].coast ? 2.0 : 1.0) * (big(p) ? 3.0 : 1.0); });
            const int n = 1 + (rng.chance(0.4) ? 1 + rng.irange(3) : 0);
            int wy = y;
            for (int k = 0; k < n; k++) { waves.push_back({wy, sk, origin, k + 1, rng.range(0.004, 0.05), -1}); wy += 1 + rng.irange(12); }
        }
    }
    // the land and the sky
    const double wet = std::max(0.0, W.wet);
    if (rng.chance(1 / 35.0 * wet) && sunAt(y) > 0.5) {   // D-05: no flood once the sun was going (the draw taken all the same)
        const int p = pickPlace(y, [&](int q) { return H.places[q].river ? 2.0 : (H.places[q].coast ? 1.0 : 0.0); });
        if (p >= 0) {
            const int64_t dead = (int64_t)(placePop(p, y) * rng.range(0.001, 0.03));
            const int e = ev(HE_FLOOD, y, p, 0.4f); E(e).value = dead; E(e).thing = H.places[p].river ? 0 : 1;
            harm(p, dead / placePop(p, y), e, 5);
            maybeLeave(p, y, e, H.places[p].sclass == SC_TOWN || H.places[p].sclass == SC_VILLAGE ? 0.04 : 0.01);
        }
    }
    int drought = 0;
    if (a <= AGE_INDUSTRIAL && rng.chance((W.desert ? 1 / 60.0 : 1 / 140.0) * (1.2 - std::min(1.0, wet))) && sunAt(y) > 0.5) {   // D-05: nor a drought
        drought = 1 + rng.irange(4);
        const int e = ev(HE_DROUGHT, y, -1, 0.4f); E(e).value = drought;
    }
    if (rng.chance(1 / 130.0 * (0.25 + W.volc))) {
        const int p = pickPlace(y, [&](int q) { return H.places[q].plan >= 0 ? 0.3 + G->relief[PP->places[H.places[q].plan].cell] : 0.5; });
        if (p >= 0) {
            const int64_t dead = (int64_t)(placePop(p, y) * rng.range(0.002, 0.05));
            const int e = ev(HE_QUAKE, y, p, 0.45f); E(e).value = dead;
            harm(p, dead / placePop(p, y), e, 6);
            maybeLeave(p, y, e, 0.05);
        }
    }
    int eruption = -1;
    if (W.volc > 0 && rng.chance(1 / 700.0 * W.volc)) {
        const int p = pickPlace(y, [&](int q) { return H.places[q].plan >= 0 ? G->relief[PP->places[H.places[q].plan].cell] : 0.3; });
        if (p >= 0) {
            const int64_t dead = (int64_t)(placePop(p, y) * rng.range(0.01, 0.2));
            eruption = ev(HE_ERUPTION, y, p, 0.6f); E(eruption).value = dead;
            harm(p, dead / placePop(p, y), eruption, 12);
            maybeLeave(p, y, eruption, 0.2);
        }
    }
    int impact = -1;
    if (calm && rng.chance(1 / 9000.0 * (0.3 + W.impacts))) {
        const int p = anyPlace(y, 1);
        const int out = rng.chance(0.6) ? 1 : (rng.chance(0.7) ? 0 : 2);
        const int64_t dead = (int64_t)(out == 2 ? pop(y) * rng.range(0.02, 0.08) : placePop(p, y) * (out == 0 ? 0.5 : rng.range(0.01, 0.1)));
        impact = ev(HE_IMPACT, y, p, out == 2 ? 0.85f : 0.65f); E(impact).value = dead; E(impact).out = out;
        harm(out == 2 ? -1 : p, out == 2 ? dead / pop(y) : dead / placePop(p, y), impact, 12);
        deficit += dead / pop(y);
        if (out == 0 && p >= 0) maybeLeave(p, y, impact, 0.6);
    }
    if (W.redDwarf && rng.chance(1 / 250.0)) {
        const int64_t dead = (int64_t)(pop(y) * rng.range(0.0005, 0.01));
        const int e = ev(HE_FLARE, y, -1, 0.55f); E(e).value = dead;
        harm(-1, dead / pop(y), e, 12);
    }
    if ((eruption >= 0 && rng.chance(0.4)) || (impact >= 0 && E(impact).out == 2) || rng.chance(1 / 350.0 * (W.cold + 2 * W.ecc))) {
        const int e = ev(HE_WINTER, y, -1, 0.5f); E(e).value = 1 + rng.irange(eruption >= 0 || impact >= 0 ? 3 : 6); E(e).cause = eruption >= 0 ? eruption : impact;
        drought = std::max(drought, (int)E(e).value);
    }
    // the harvest (the ages that lived by their fields and herds), its failures and the famines after them
    const int yi = y - H.yearsFrom;
    if (yi >= 0 && yi < (int)H.harvest.size()) {
        double q = 2.6 + 1.25 * ((rng.uni() + rng.uni() + rng.uni()) - 1.5);
        if (drought > 0) q -= 2.2;
        if (eruption >= 0) q -= 1.2;
        if (impact >= 0) q -= 2.5;
        if (!warsOn.empty()) q -= 0.3;
        if (stage >= 0) {
            const int hzd = SCRIPTS[H.cause].hazard;
            const bool fields = hzd == HZ_FAMINE || hzd == HZ_COLD || hzd == HZ_DRYING || hzd == HZ_IMPACT || hzd == HZ_FIRE || H.cause == CAUSE_CLIMATE || H.cause == CAUSE_WATER_WAR || H.cause == CAUSE_POISONED;
            if (fields) { q -= 2.5 * clampd((y - H.stageYear[0]) / (double)std::max(1, H.last - H.stageYear[0]), 0, 1) + (stage >= STAGE_ONSET ? 0.6 : 0); q = std::min(q, 2.4); }
            if (hzd == HZ_SUNLOST) { q -= 4.5 * (1 - sunAt(y)) + (stage >= STAGE_ONSET ? 0.6 : 0); q = std::min(q, 2.4); }   // D-05: the fields under a sun going
        }
        const int hv = clampi((int)std::lround(q), 0, 4);
        H.harvest[yi] = (uint8_t)hv;
        const bool fields = a <= AGE_INDUSTRIAL;
        const int prev = yi > 0 ? H.harvest[yi - 1] : 2;
        if (fields && (hv == 0 || (hv == 1 && prev <= 1))) {
            const bool famine = y - lastFamine >= (stage >= 0 ? 3 : 2) && rng.chance(hv == 0 ? 0.75 : 0.5);
            int he = -1;
            if ((hv == 0 && stage < STAGE_ONSET) || famine) { he = ev(HE_HARVEST, y, -1, 0.25f); E(he).out = hv; }
            if (famine) {
                static const double F[AGE_COUNT] = {1, 1, 0.3, 0.05, 0.02, 0.01};
                const int at = rng.chance(0.4) ? anyPlace(y) : -1;
                const int64_t dead = (int64_t)(pop(y) * rng.range(0.002, 0.025) * F[a]);
                const int fe = ev(HE_FAMINE, y, at, 0.5f); E(fe).value = dead; E(fe).cause = he;
                lastFamine = y;
                harm(-1, dead / pop(y), fe, 4);
                if (stage >= STAGE_ONSET) lastHazardEv = fe;
                deficit += dead / pop(y);
                if (bigN > 0 && rng.chance(0.3)) { const int rp = pickPlace(y, [&](int p) { return big(p) ? 1.0 : 0.0; }); const int u = ev(HE_UNREST, y, rp, 0.3f); E(u).thing = 0; E(u).cause = fe; E(u).value = rng.irange(40); }
            }
        } else if (fields && hv == 4 && rng.chance(0.2)) { const int he = ev(HE_HARVEST, y, -1, 0.1f); E(he).out = 4; }
    }
    // the works
    {
        static const double RATE[AGE_COUNT] = {1 / 50.0, 1 / 22.0, 1 / 9.0, 1 / 8.0, 1 / 25.0, 1 / 400.0};
        if (calm && standingN > 0 && rng.chance(RATE[a] * (0.6 + 0.01 * std::min(bigN, 60)))) {
            double w[WK_COUNT];
            const int railway = inventionIndex("the railway"), radio = inventionIndex("radio"), rocket = inventionIndex("the rocket"), habitat = inventionIndex("the first habitat"), elevator = inventionIndex("the elevator to orbit");
            bool anyRiver = false, anyCoast = false;
            for (int p = 0; p < (int)H.places.size(); p++) if (stands(p, y)) { anyRiver |= H.places[p].river; anyCoast |= H.places[p].coast; }
            for (int k = 0; k < WK_COUNT; k++) {
                const WorkInfo& I = WORKS[k];
                w[k] = 0;
                if (!(I.ages & ageBit(a))) continue;
                double x = 0.2;
                for (int vv = 0; vv < VAL_COUNT; vv++) x += I.w[vv] * v[vv];
                switch (I.need) {
                    case ND_RIVER: if (!anyRiver) x = 0; break;
                    case ND_COAST: if (!anyCoast) x = 0; break;
                    case ND_RAILWAY: if (H.inventionYear(railway) == 0 || H.inventionYear(railway) > y) x = 0; break;
                    case ND_RADIO: if (H.inventionYear(radio) == 0 || H.inventionYear(radio) > y) x = 0; break;
                    case ND_ROCKET: if (H.inventionYear(rocket) == 0 || H.inventionYear(rocket) > y) x = 0; break;
                    case ND_HABITAT: if (H.inventionYear(habitat) == 0 || H.inventionYear(habitat) > y) x = 0; break;
                    case ND_ELEVATOR: if (H.inventionYear(elevator) == 0 || H.inventionYear(elevator) > y) x = 0; break;
                    case ND_MONARCH: if (govNow != GOV_MONARCHY) x = 0; break;
                    case ND_DEAD: if (gene.of(GV_DEAD) == DE_NOTHING) x = 0; break;
                    default: break;
                }
                if (k == WK_GREAT) x = 0;   // the far future's works are its plan's (founded, then finished)
                w[k] = std::max(0.0, x);
            }
            double tot = 0; for (int k = 0; k < WK_COUNT; k++) tot += w[k];
            if (tot > 0) {
                const int k = rng.pick(w, WK_COUNT);
                const WorkInfo& I = WORKS[k];
                const int p = pickPlace(y, [&](int q) {
                    if (I.need == ND_RIVER && !H.places[q].river) return 0.0;
                    if (I.need == ND_COAST && !H.places[q].coast) return 0.0;
                    return q == s ? 4.0 : (big(q) ? 2.0 : 0.5);
                });
                if (p >= 0) {
                    const int years = (int)std::lround(std::exp(rng.range(std::log(I.years[0]), std::log(I.years[1]))));
                    int maker = -1;
                    if ((k == WK_PALACE || k == WK_WALLS || k == WK_FOCUS) && reign >= 0 && H.reigns[reign].ruler >= 0 && rng.chance(0.5)) maker = H.reigns[reign].ruler;
                    else maker = figure(y, HR_MAKER, p, 30, 60);
                    const int e = ev(HE_BUILT, y, p, k == WK_FOCUS || k == WK_TEMPLE || k == WK_ARCOLOGY || k == WK_RING ? 0.55f : 0.35f);
                    E(e).thing = k; E(e).a = maker; E(e).value = years;
                }
            }
        }
    }
    // the far future's works finished (begun at their founding)
    if (H.age == AGE_FAR_FUTURE) for (int p = 0; p < (int)H.places.size(); p++) {
        if (H.places[p].plan < 0) continue;
        const int done = H.places[p].founded + 60 + (int)(unitFromHash(mix64(H.seed ^ (0x40F7ULL + p))) * 500);
        if (y == std::min(done, std::max(H.places[p].founded + 1, P.collapseYear - 1)) && stands(p, y)) {
            const int e = ev(HE_BUILT, y, p, 0.8f); E(e).thing = WK_GREAT; E(e).value = y - H.places[p].founded; E(e).a = figure(y, HR_MAKER, p, 40, 200, false);
        }
    }
    // the sky
    if (cometPeriod > 0 && (y - cometPhase) % cometPeriod == 0) { const int e = ev(HE_OMEN, y, s, 0.12f); E(e).thing = 0; }
    if (body.moonCount > 0 && rng.chance(1 / 150.0)) { const int e = ev(HE_OMEN, y, anyPlace(y), 0.12f); E(e).thing = rng.chance(0.6) ? 1 : 5; }
    if (auroral && rng.chance(1 / 120.0)) { const int e = ev(HE_OMEN, y, anyPlace(y), 0.1f); E(e).thing = 2; }
    if (rng.chance(1 / 250.0)) { const int e = ev(HE_OMEN, y, anyPlace(y), 0.1f); E(e).thing = 3; }
    if (rng.chance(1 / 5000.0)) { const int e = ev(HE_OMEN, y, -1, 0.3f); E(e).thing = 4; }
    // deeds
    {
        static const double RATE[AGE_COUNT] = {1 / 80.0, 1 / 35.0, 1 / 25.0, 1 / 25.0, 1 / 50.0, 1 / 500.0};
        if (calm && standingN > 0 && rng.chance(RATE[a] * (0.6 + v[VAL_ART] + v[VAL_LEARNING]))) {
            static const unsigned DA[13] = {AP | AM | AI | AMO | AF, AM | AI, AM | AI, AM | AI, AP, AM | AI, AP | AM | AI, AP | AM, AM | AI | AMO, AI | AMO, AI | AMO | AF, AMO, AM | AI | AMO | AF};
            const double DW[13] = {v[VAL_ART] * 2, v[VAL_LEARNING], 0.4, v[VAL_LEARNING], 0.6, v[VAL_TRADE], 0.4, v[VAL_ART], v[VAL_LEARNING], v[VAL_ART], v[VAL_LEARNING], v[VAL_ART], v[VAL_ART] * 0.5};
            double w[13]; double tot = 0;
            for (int k = 0; k < 13; k++) { w[k] = (DA[k] & ageBit(a)) ? DW[k] + 0.05 : 0; tot += w[k]; }
            if (tot > 0) {
                int k = rng.pick(w, 13);
                if (k == 2 && (reign < 0 || H.reigns[reign].ruler < 0 || lawGivers.count(H.reigns[reign].ruler))) k = 0;
                const int p = anyPlace(y);
                int who;
                if (k == 2) { who = H.reigns[reign].ruler; lawGivers.insert(who); }
                else who = figure(y, k == 3 || k == 5 ? HR_EXPLORER : (k == 6 || k == 8 || k == 10 || k == 1 ? HR_MAKER : HR_ARTIST), p, 22, 60);
                const int e = ev(HE_DEED, y, p, 0.3f); E(e).a = who; E(e).thing = k;
            }
        }
    }
    // unrest: strikes in the mill towns, risings under a cruel ruler
    {
        double rate = a == AGE_INDUSTRIAL ? 1 / 12.0 : (a == AGE_MODERN ? 1 / 20.0 : (a == AGE_MEDIEVAL ? 1 / 90.0 : 0));
        if (reign >= 0 && H.reigns[reign].ruler >= 0 && (H.persons[H.reigns[reign].ruler].traits & HT_CRUEL)) rate += 1 / 60.0;
        if (calm && rng.chance(rate)) {
            const int p = pickPlace(y, [&](int q) { return big(q) ? 2.0 : 0.5; });
            if (p >= 0) {
                const int k = a >= AGE_INDUSTRIAL ? (rng.chance(0.7) ? 1 : 3) : 2;
                const int e = ev(HE_UNREST, y, p, 0.3f); E(e).thing = k; E(e).value = k == 1 ? 0 : rng.irange(60);
            }
        }
    }
    // the countryside to the cities, once in the industrial age
    if (a == AGE_INDUSTRIAL) {
        const HistoryEra& he = H.eras[H.eraAt(y)];
        if (y == he.start + (he.end - he.start) / 3 && bigN > 0) {
            const int to = s >= 0 ? s : anyPlace(y);
            const int e = ev(HE_MIGRATION, y, -1, 0.5f); E(e).place2 = to; E(e).value = (int64_t)(pop(y) * rng.range(0.05, 0.2)); E(e).thing = 0;
        }
    }
    // a schism or a reform of the rites now and then
    {
        static const double RATE[AGE_COUNT] = {1 / 400.0, 1 / 150.0, 1 / 200.0, 1 / 300.0, 1 / 600.0, 0};
        if (calm && rng.chance(RATE[a] * (0.3 + 3 * v[VAL_PIETY]))) {
            const int k = rng.chance(0.55) ? 1 : (rng.chance(0.6) ? 2 : 4);
            const int p = anyPlace(y);
            const int e = ev(HE_FAITH, y, p, 0.35f); E(e).thing = k; E(e).value = H.eras[H.eraAt(y)].faith;
            if (k == 2) E(e).a = figure(y, HR_PROPHET, p, 30, 60);
        }
    }
}

// H-04's second pass: the everyday of a year, from its own stream (the year's and the people's seed): what the news, the annals and the
// letters of a quiet year are made of. Nothing here harms anyone or makes a person, so the history above is H-02's whatever is drawn here
void Builder::everyday(int y) {
    Rng dr(mix64(H.seed ^ 0xDA11E5ULL ^ (uint64_t)(uint32_t)y * 0x9E3779B97F4A7C15ULL));
    const int a = ageAt(y);
    const int stage = H.profile.stageAt(y);
    const double busy = stage >= STAGE_CRISIS ? 0.35 : (stage >= 0 ? 0.7 : 1.0);   // the collapse's years have less of it
    bool coast = false, river = false, rail = false, mineTown = false, contact = false;
    for (int p = 0; p < (int)H.places.size(); p++) if (stands(p, y)) { coast |= H.places[p].coast; river |= H.places[p].river; }
    const int railway = inventionIndex("the railway"), flight = inventionIndex("flight"), reactor = inventionIndex("the reactor"), habitat = inventionIndex("the first habitat");
    auto had = [&](int inv) { const int iy = inv >= 0 ? H.inventionYear(inv) : 0; return iy > 0 && iy <= y; };
    rail = had(railway);
    for (; evScan < H.events.size(); evScan++) { const HistoryEvent& e = H.events[evScan]; if (e.kind == HE_CONTACT) everContact = true; if (e.kind == HE_BUILT && e.thing == WK_MINE) everMine = true; }
    contact = everContact; mineTown = everMine;
    const bool fields = H.profile.economy == ECO_FARMING || H.profile.economy == ECO_HERDING || H.profile.economy2 == ECO_FARMING || H.profile.economy2 == ECO_HERDING || a <= AGE_MEDIEVAL;
    const bool cold = W.cold > 0.3 || H.seasons == 4, desert = W.desert;
    auto fits = [&](const EverydayWord& w) {
        if (!(w.ages & ageBit(a))) return false;
        switch (w.need) {
            case 1: return coast; case 2: return coast || river; case 3: return rail; case 4: return had(flight); case 5: return had(reactor); case 6: return had(habitat);
            case 7: return cold; case 8: return desert; case 9: return fields; case 10: return contact && !H.lore.other.empty(); case 11: return mineTown; case 12: return !desert;
            default: return true;
        }
    };
    auto placeBy = [&](double bigW, bool coastOnly, bool riverOrCoast) {   // a standing place by weight, from the year's own stream
        double tot = 0;
        for (int p = 0; p < (int)H.places.size(); p++) {
            if (!stands(p, y) || (coastOnly && !H.places[p].coast) || (riverOrCoast && !H.places[p].coast && !H.places[p].river)) continue;
            tot += big(p) ? bigW : 1.0;
        }
        if (tot <= 0) return -1;
        double u = dr.uni() * tot;
        for (int p = 0; p < (int)H.places.size(); p++) {
            if (!stands(p, y) || (coastOnly && !H.places[p].coast) || (riverOrCoast && !H.places[p].coast && !H.places[p].river)) continue;
            u -= big(p) ? bigW : 1.0;
            if (u < 0) return p;
        }
        return -1;
    };
    auto someone = [&](int minAge, int maxAge, int home) {   // a person alive in the year, of an age, at a place where one is asked (none: -1)
        int n = 0, pick = -1;
        for (int i = 0; i < (int)H.persons.size(); i++) {
            const HistoryPerson& P = H.persons[i];
            if (P.born > y || P.died < y || y - P.born < minAge || y - P.born > maxAge || P.role == HR_RULER || P.role == HR_REGENT) continue;
            if (home >= 0 && P.home != home) continue;
            if (dr.irange(++n) == 0) pick = i;   // a reservoir of one
        }
        return pick;
    };
    const int ruler = reign >= 0 && !vacant && H.reigns[reign].ruler >= 0 && H.persons[H.reigns[reign].ruler].died >= y && H.persons[H.reigns[reign].ruler].born <= y ? H.reigns[reign].ruler : -1;
    for (int k = 0; k < EVERYDAY_N; k++) {
        const EverydayKind& K = EVERYDAY[k];
        if (!dr.chance(K.rate[a] * busy)) continue;
        double w[24]; int n = std::min(K.n, 24); double tot = 0;
        for (int i = 0; i < n; i++) { w[i] = fits(K.words[i]) ? 1.0 : 0.0; tot += w[i]; }
        if (tot <= 0) continue;
        const int thing = dr.pick(w, n);
        const char* key = K.words[thing].key;
        const int need = K.words[thing].need;
        int place = placeBy(K.kind == HE_FAIR || K.kind == HE_SHOW || K.kind == HE_GAMES || K.kind == HE_TRIAL || K.kind == HE_LAW ? 3.0 : 1.5, need == 1, need == 2);
        if (K.kind == HE_LAW || K.kind == HE_EMBASSY) place = seat(y);
        if (place < 0 && K.kind != HE_STORM && K.kind != HE_PRICE && K.kind != HE_LAW) continue;
        const int season = dr.irange(H.seasons);
        const int e = ev(K.kind, y, place, K.sal, season);
        E(e).thing = thing;
        switch (K.kind) {
            case HE_FAIR: {   // how many came: a share of the place, at most what a fair of the age drew
                static const double CAP[AGE_COUNT] = {600, 30000, 120000, 300000, 200000, 50000};
                E(e).value = (int64_t)std::min(placePop(place, y) * dr.range(0.05, 0.6), CAP[a] * dr.range(0.3, 1.0));
                break;
            }
            case HE_FIRE: {
                const double share = std::strcmp(key, "half") == 0 ? dr.range(0.3, 0.6) : (std::strcmp(key, "street") == 0 ? dr.range(0.01, 0.08) : dr.range(0.002, 0.02));
                E(e).value = std::max<int64_t>(1, (int64_t)(placePop(place, y) / 5.0 * share));
                if (std::strcmp(key, "half") == 0) E(e).salience = 0.45f;
                break;
            }
            case HE_STORM: E(e).value = dr.chance(0.6) ? 0 : 1 + dr.irange(a >= AGE_MODERN ? 40 : 12); E(e).season = std::strcmp(key, "snow") == 0 ? (H.seasons == 4 ? 3 : season) : season; break;
            case HE_PEST: if (std::strcmp(key, "locusts") == 0 || std::strcmp(key, "blight") == 0) E(e).salience = 0.35f; break;
            case HE_GAMES: E(e).a = someone(16, 34, -1); break;
            case HE_LAW: E(e).a = ruler; break;
            case HE_TRIAL: E(e).a = someone(18, 70, -1); E(e).out = dr.irange(4); if (E(e).a < 0) E(e).out = 1; break;
            case HE_EMBASSY: {
                if (thing == 0) { int from = placeBy(2.0, false, false); if (from == place) from = -1; if (from < 0) { H.events.pop_back(); continue; } E(e).place2 = from; }
                E(e).a = someone(30, 70, -1);
                break;
            }
            case HE_ACCIDENT: E(e).value = std::strcmp(key, "habitat") == 0 || std::strcmp(key, "reactor") == 0 ? 20 + dr.irange(4000) : 1 + dr.irange(std::strcmp(key, "ship") == 0 || std::strcmp(key, "train") == 0 ? 160 : 60);
                if (E(e).value > 100) E(e).salience = 0.5f;
                break;
            case HE_PRICE: {
                const int hv = H.harvestAt(y - 1);
                E(e).out = (std::strcmp(key, "bread") == 0 || std::strcmp(key, "meat") == 0) ? (hv <= 1 ? 1 : (hv >= 3 ? 0 : dr.irange(2))) : (dr.chance(0.65) ? 1 : 0);
                E(e).value = 5 + dr.irange(E(e).out ? 60 : 30);
                break;
            }
            case HE_BEASTS: if (std::strcmp(key, "killed") == 0) E(e).value = 1 + dr.irange(3); if (std::strcmp(key, "none") == 0) E(e).salience = 0.4f; break;
            case HE_SHOW: E(e).a = someone(20, 70, place); if (E(e).a < 0) E(e).a = someone(20, 70, -1); break;
            case HE_VISIT: if (ruler < 0 || place == seat(y)) { H.events.pop_back(); continue; } E(e).a = ruler; E(e).thing = 0; break;
            default: break;
        }
    }
}

// H-04's second pass: what a family's letters are made of besides births, weddings, moves and deaths: an illness got over, a hurt, a child
// apprenticed, the house built, sold, burnt or mended, a quarrel with the neighbours, a journey and back, a betrothal before a wedding. Each
// person followed (the families' members) draws from a stream of their own after the years are done, so nothing the years drew moves;
// a person's place is where their birth and their moves put them in that year
void Builder::family() {
    const int np = (int)H.persons.size();
    std::vector<std::vector<std::pair<int, int>>> homes(np);   // (year, place)
    std::vector<int> wedding(np, 0), weddingWith(np, -1);
    for (const HistoryEvent& e : H.events) {
        if (e.a < 0 || e.a >= np) continue;
        if (e.kind == HE_BIRTH && e.place >= 0) homes[e.a].push_back({e.year, e.place});
        else if (e.kind == HE_MOVE && e.place2 >= 0) homes[e.a].push_back({e.year, e.place2});
        else if (e.kind == HE_MARRIAGE) { wedding[e.a] = e.year; weddingWith[e.a] = e.b; }
    }
    for (auto& h : homes) std::sort(h.begin(), h.end());
    auto homeAt = [&](int p, int y) { int h = -1; for (const auto& m : homes[p]) { if (m.first > y) break; h = m.second; } if (h < 0) h = H.persons[p].home; return h >= 0 && stands(h, y) ? h : -1; };
    std::vector<int> adultsAt;   // for a quarrel's other party
    for (int p = 0; p < np; p++) {
        const HistoryPerson& P = H.persons[p];
        if (P.family < 0 || P.family >= (int)H.families.size() || H.families[P.family].house || P.fate == HF_LET_GO || P.role == HR_RULER) continue;   // the families followed
        Rng fr(mix64(H.seed ^ 0xFA111ULL ^ (uint64_t)(p + 1) * 0x9E3779B97F4A7C15ULL));
        const int last = std::min(P.died == ALIVE ? H.last : P.died - 1, H.last);   // the year of a death has its own news
        bool apprenticed = false;
        for (int y = std::max(P.born + 1, H.first); y <= last; y++) {
            if (H.profile.stageAt(y) >= STAGE_CRISIS && fr.chance(0.5)) continue;
            const int age = y - P.born;
            const int home = homeAt(p, y);
            const int season = fr.irange(H.seasons);
            if (home < 0) continue;
            // an illness got over (the young and the old more), a hurt
            if (fr.chance(age < 12 || age > 55 ? 0.03 : 0.012)) { const int e = ev(HE_ILLNESS, y, home, 0.12f, season); E(e).a = p; E(e).thing = fr.irange((int)(sizeof(PRIVATE_ILLNESS) / sizeof(PRIVATE_ILLNESS[0]))); }
            if (age >= 4 && fr.chance(0.01)) { const int e = ev(HE_INJURY, y, home, 0.12f, season); E(e).a = p; E(e).thing = fr.irange((int)(sizeof(PRIVATE_INJURY) / sizeof(PRIVATE_INJURY[0]))); }
            // a child apprenticed once (the ages that had trades to learn)
            if (!apprenticed && age >= 11 && age <= 15 && ageAt(y) >= AGE_MEDIEVAL && fr.chance(0.25)) {
                apprenticed = true;
                const int e = ev(HE_APPRENTICE, y, home, 0.14f, season); E(e).a = p; E(e).thing = ageAt(y) * 10 + fr.irange(10);
            }
            if (age >= 22 && age <= 70) {
                if (P.spouse >= 0 && fr.chance(0.012)) { const int e = ev(HE_HOUSE, y, home, 0.12f, season); E(e).a = p; E(e).thing = fr.irange((int)(sizeof(PRIVATE_HOUSE) / sizeof(PRIVATE_HOUSE[0]))); }
                if (fr.chance(0.01)) {   // a quarrel with someone of another family at the same place
                    int other = -1, n = 0;
                    for (int q = 0; q < np; q++) {
                        const HistoryPerson& Q = H.persons[q];
                        if (q == p || Q.family < 0 || Q.family == P.family || Q.fate == HF_LET_GO || Q.born > y - 20 || (Q.died != ALIVE && Q.died < y) || homeAt(q, y) != home) continue;
                        if (fr.irange(++n) == 0) other = q;
                    }
                    const int e = ev(HE_QUARREL, y, home, 0.12f, season); E(e).a = p; E(e).b = other; E(e).thing = fr.irange((int)(sizeof(PRIVATE_QUARREL) / sizeof(PRIVATE_QUARREL[0])));
                }
            }
            if (age >= 18 && age <= 66 && fr.chance(0.014)) {   // a journey there and back
                int to = -1, n = 0;
                for (int q = 0; q < (int)H.places.size(); q++) if (q != home && stands(q, y) && fr.irange(++n) == 0) to = q;
                if (to >= 0) { const int e = ev(HE_JOURNEY, y, home, 0.12f, season); E(e).a = p; E(e).place2 = to; E(e).thing = fr.irange((int)(sizeof(PRIVATE_JOURNEY) / sizeof(PRIVATE_JOURNEY[0]))); }
            }
        }
        // a betrothal the year before the wedding (once a couple: told for the one the wedding names first)
        if (wedding[p] > 0 && weddingWith[p] >= 0 && wedding[p] - 1 >= std::max(H.first, P.born + 15) && fr.chance(0.7)) {
            const int y = wedding[p] - 1, home = homeAt(p, y);
            if (home >= 0) { const int e = ev(HE_BETROTHAL, y, home, 0.14f, fr.irange(H.seasons)); E(e).a = p; E(e).b = weddingWith[p]; }
        }
    }
}

void Builder::collapseYear(int y) {
    const PeopleProfile& P = H.profile;
    const CauseScript& CS = SCRIPTS[H.cause];
    for (int s = 0; s < STAGE_COUNT; s++) if (y == H.stageYear[s]) {   // a stage that begins in the year of the one before still marks it
        const int e = ev(HE_STAGE, y, -1, 0.8f, -1); E(e).out = s; E(e).thing = H.cause;
        stageEv[s] = e;
        if (s == 0) lastHazardEv = e;
    }
    const int stage = P.stageAt(y);
    if (stage < 0) return;
    for (int oy : sunOmens) if (oy == y) {   // D-05: the omen of the lost sun's way (the wanderer growing, the new star, the bent stars, the dance)
        const int e = ev(HE_OMEN, y, seat(y), 0.6f); E(e).thing = 6 + clampi(P.sunWay, 0, SW_COUNT - 1); E(e).cause = stageEv[STAGE_WARNING];
    }
    // the year's signs: the warning's and the onset's before its harm, the crisis's and the end's after it; each names the one before it
    // (or its stage), so a year's keep their order
    auto signs = [&](int fromStage, int toStage) {
        for (size_t k = plannedAtM; k < planned.size() && planned[k].year <= y; k++) {
            const Planned& q = planned[k];
            if (q.kind != HE_SIGN || q.year != y || q.b < fromStage || q.b > toStage) continue;
            int p = -1;
            for (const auto& kv : endsByYear) { for (int x : kv.second) if (stands(x, y)) { p = x; break; } if (p >= 0) break; }
            if (q.b == STAGE_END) p = H.lastPlace;
            if (p < 0 || !stands(p, y)) p = seat(y);
            const int e = ev(HE_SIGN, y, p, q.b == STAGE_WARNING ? 0.6f : 0.75f); E(e).thing = q.a; E(e).out = q.b;
            E(e).cause = lastSignEv >= 0 && H.events[lastSignEv].year == y ? lastSignEv : stageEv[q.b];
            lastSignEv = e;
        }
    };
    signs(STAGE_WARNING, STAGE_ONSET);
    const int hazardBefore = (int)H.events.size();
    const int s = seat(y);
    const double prog = clampd((y - H.stageYear[0] + 1) / (double)std::max(1, H.last - H.stageYear[0] + 1), 0, 1);
    const int a = ageAt(y);
    auto exposed = [&]() {   // one of the next few places in the cause's order that still stand
        int next[5], n = 0;
        for (const auto& kv : endsByYear) { for (int p : kv.second) if (stands(p, y) && n < 5) next[n++] = p; if (n >= 5) break; }
        return n ? next[rng.irange(n)] : s;
    };
    // the hazard of the cause, from the onset on
    if (stage >= STAGE_ONSET) {
        const bool onset = y == H.stageYear[STAGE_ONSET];
        switch (CS.hazard) {
            case HZ_FAMINE: if (onset) {
                const int he = ev(HE_HARVEST, y, -1, 0.35f); E(he).out = 0;
                const int fe = ev(HE_FAMINE, y, exposed(), 0.65f); E(fe).cause = he; E(fe).value = (int64_t)(pop(y) * rng.range(0.01, 0.06));
                harm(-1, rng.range(0.01, 0.06), fe, 4); lastHazardEv = fe; } break;
            case HZ_COLD: if (onset || rng.chance(0.07)) {
                int cause = -1;
                if (onset && H.cause == CAUSE_VOLCANIC_WINTER) { const int p = exposed(); cause = ev(HE_ERUPTION, y, p, 0.85f); E(cause).value = (int64_t)(placePop(p, y) * rng.range(0.05, 0.3)); harm(p, 0.15, cause, 12); }
                const int e = ev(HE_WINTER, y, -1, 0.6f); E(e).value = onset ? std::max(1, H.last - y + 1) : 1 + rng.irange(4); E(e).cause = cause;
                harm(-1, rng.range(0.005, 0.03), e, 13); lastHazardEv = e; } break;
            case HZ_FLOOD: if (onset || rng.chance(0.3)) {
                const int p = exposed();
                const int e = ev(HE_FLOOD, y, p, 0.65f); E(e).value = (int64_t)(placePop(p, y) * rng.range(0.01, 0.1)); E(e).thing = p >= 0 && H.places[p].coast ? 1 : 0;
                harm(p, 0.05, e, 5); lastHazardEv = e; } break;
            case HZ_SICKNESS: if (onset || rng.chance(0.3)) {
                if (H.sicknesses.empty() || onset) { HistorySickness S; S.name = mix64(H.seed ^ 0x40DEADULL ^ (uint64_t)y); S.first = y; S.made = H.cause == CAUSE_ENGINEERED_PLAGUE; H.sicknesses.push_back(S); }
                const int sk = (int)H.sicknesses.size() - 1;
                const double share = (H.cause == CAUSE_GREAT_SICKNESS || H.cause == CAUSE_ENGINEERED_PLAGUE ? rng.range(0.08, 0.3) : rng.range(0.02, 0.1));
                waves.push_back({y, sk, exposed(), H.sicknesses[sk].waves + 1, share, -1}); } break;
            case HZ_WAR: if (onset) {
                const int kind = H.cause == CAUSE_CONQUEST ? HW_CONQUEST : (H.cause == CAUSE_BAND_WAR ? HW_BANDS : HW_CIVIL);
                bool joint = false;
                for (const HistoryWar& w : H.wars) if (w.kind == HW_CONQUEST && w.sideB == -2) joint = true;
                if (!joint) {
                    const int B = exposed();
                    const int wi = startWar(y, kind, -1, B, -1, -1, lastHazardEv, std::max(1, H.last - y));
                    H.wars[wi].winner = 1;
                    WarRun& run = warsOn.back();
                    run.battles.clear();
                    for (int t = y; t < H.last; t += 1 + rng.irange(3)) run.battles.push_back(t);
                    run.siegeAt = run.battles.empty() ? -1 : run.battles.back();
                }
            } break;
            case HZ_DRYING: if (onset) { const int e = ev(HE_DROUGHT, y, -1, 0.6f); E(e).value = std::max(1, H.last - y + 1); lastHazardEv = e; } break;
            case HZ_IMPACT: if (onset) {
                const int p = anyPlace(y, 1);
                const int e = ev(HE_IMPACT, y, p, 1.0f); E(e).out = 2; E(e).value = (int64_t)(pop(y) * rng.range(0.1, 0.4));
                harm(-1, rng.range(0.1, 0.4), e, 12); deficit += 0.2;
                const int w2 = ev(HE_WINTER, y, -1, 0.7f); E(w2).value = std::max(1, std::min(H.last - y + 1, 3 + rng.irange(8))); E(w2).cause = e;
                lastHazardEv = e; } break;
            case HZ_FLARE: if (onset || rng.chance(0.2)) {
                const int e = ev(HE_FLARE, y, -1, onset ? 1.0f : 0.6f); E(e).value = (int64_t)(pop(y) * (onset ? rng.range(0.1, 0.5) : rng.range(0.01, 0.05)));
                harm(-1, onset ? rng.range(0.1, 0.5) : 0.03, e, 12); lastHazardEv = e; } break;
            case HZ_UNREST: if (onset || rng.chance(0.35)) {
                const int p = pickPlace(y, [&](int q) { return big(q) ? 2.0 : 0.3; });
                const int e = ev(HE_UNREST, y, p, 0.5f); E(e).thing = a >= AGE_INDUSTRIAL ? (rng.chance(0.5) ? 1 : 3) : 2; E(e).value = (int64_t)(placePop(std::max(0, p), y) * rng.range(0.0005, 0.005));
                harm(p, 0.005, e, 7); lastHazardEv = e; } break;
            case HZ_LEAVING: if (onset || rng.chance(0.3)) {
                const int p = exposed();
                const int e = ev(HE_EXODUS, y, p, 0.6f); E(e).value = (int64_t)(placePop(p, y) * rng.range(0.2, 0.6)); E(e).thing = CS.whither;
                deficit += 0.02; lastHazardEv = e; } break;
            case HZ_FIRE: if (onset || rng.chance(0.15)) {
                const int p = exposed();
                const int e = ev(HE_SIGN, y, p, onset ? 1.0f : 0.6f); E(e).thing = STAGE_ONSET * 2 + (onset ? 0 : 1); E(e).out = STAGE_ONSET;
                const double share = onset ? rng.range(0.15, 0.6) : rng.range(0.01, 0.05);
                E(e).value = (int64_t)(pop(y) * share); harm(-1, share, e, 12); deficit += share * 0.5; lastHazardEv = e; } break;
            case HZ_CHILDREN: break;
            case HZ_SUNLOST: {   // D-05: the long cold from the onset; the sun seen going (smaller, the year that did not end, the sea frozen, the air
                // fallen as snow, the last sunrise, the stars at noon), each once and in its order; the cold's dead every year as the light failed
                const double light = sunAt(y), before = y > 1 ? sunAt(y - 1) : 1;
                if (onset) { const int e = ev(HE_WINTER, y, -1, 0.6f); E(e).value = std::max(1, H.last - y + 1); lastHazardEv = e; }
                auto seen = [&](int thing, float sal) {
                    if (sunSeen >> thing & 1) return;
                    sunSeen |= 1 << thing;
                    const int p = thing == 2 ? exposed() : (s >= 0 ? s : -1);
                    const int e = ev(HE_SUN, y, p, sal); E(e).thing = thing; E(e).cause = lastHazardEv;
                    lastHazardEv = e;
                };
                if (light < 0.6 && before >= 0.6) seen(0, 0.7f);
                if (y == H.stageYear[STAGE_CRISIS]) seen(1, 0.75f);
                if (light < 0.25 && before >= 0.25 && !W.desert) seen(2, 0.75f);
                if (light < 0.05 && before >= 0.05 && airFrozenThen()) seen(3, 0.8f);
                if (y == H.profile.lastSunrise) seen(4, 1.0f);
                if (y == H.profile.lastSunrise + 1) seen(5, 0.95f);
                if (light < 0.8) harm(-1, 0.012 * (1 - light) * (stage >= STAGE_END ? 0.5 : 1.0), lastHazardEv, 13);
                break;
            }
            default: break;
        }
        if (onset && lastSignEv >= 0 && H.events[lastSignEv].year == y)
            for (int i = hazardBefore; i < (int)H.events.size(); i++) if (H.events[i].cause < 0) H.events[i].cause = lastSignEv;
        if (lastHazardEv >= hazardBefore) lastSignEv = lastHazardEv;   // the later signs of the year follow the harm
    }
    signs(STAGE_CRISIS, STAGE_END);
    // the places left in the collapse, in the cause's order, the people with them; the great ones' people leave in a body
    auto it = endsByYear.find(y);
    if (it != endsByYear.end()) for (int p : it->second) {
        if (H.places[p].ended != y) continue;   // left early
        if (p == H.lastPlace) continue;          // the end's
        const int e = ev(HE_ABANDONED, y, p, big(p) ? 0.7f : 0.2f); E(e).cause = lastHazardEv; E(e).thing = 8;
        H.places[p].endEvent = e;
        if (big(p) && placePop(p, y) > 200) {
            const int x = ev(HE_EXODUS, y, p, 0.5f); E(x).value = (int64_t)(placePop(p, y) * rng.range(0.1, 0.4)); E(x).thing = CS.whither; E(x).cause = e;
            if (CS.whither == WH_PLACE || CS.whither == WH_UNDER) E(x).place2 = H.lastPlace;   // D-05: below, at the last place's deep halls
        }
        const int to = nearestStanding(p, y + 1, false);
        for (int q : stepped) {
            HistoryPerson& Q = H.persons[q];
            if (Q.died != ALIVE || Q.home != p) continue;
            Q.home = to >= 0 ? to : H.lastPlace;
            if (mode[q] == 2 && y - Q.born >= 16) { const int m = ev(HE_MOVE, y, p, 0.15f); E(m).a = q; E(m).place2 = Q.home; E(m).cause = e; }
        }
    }
    // the rule ends in the crisis or the end (the last ruler named so)
    if (ruleEnds > 0 && y == ruleEnds && reign >= 0 && H.reigns[reign].end == ALIVE) endReign(y, 6, lastHazardEv);
    // the people's harm through the crisis by the cause's speed (a sudden end's at its onset)
    const int speed = causeSpeed(H.cause);
    if (stage >= STAGE_CRISIS) {
        int how = 15;
        switch (CS.hazard) {
            case HZ_FAMINE: case HZ_DRYING: how = 4; break;
            case HZ_COLD: case HZ_SUNLOST: how = 13; break;   // D-05: the cold
            case HZ_SICKNESS: how = 1; break;
            case HZ_FLOOD: how = 5; break;
            case HZ_IMPACT: case HZ_FLARE: case HZ_FIRE: how = 12; break;
            default: break;
        }
        harm(-1, speed == 2 ? 0.01 : (speed == 1 ? 0.04 : 0.06), lastHazardEv, how);
    }
    (void)prog;
}
