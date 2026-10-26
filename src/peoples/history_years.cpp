// E-10: the builder's years (history_builder.h): the persons' lives a year, the year loop that runs the events, and the finish
// (the events in their order, the indices remapped)
#include "history_builder.h"

void Builder::stepPersons(int y) {
    const PeopleProfile& P = H.profile;
    const int a = ageAt(y);
    const Mortality& M = MORTALITY[a];
    const int stage = P.stageAt(y);
    double fert = M.births;
    if (stage >= 0) {
        const double prog = clampd((y - H.stageYear[0] + 1) / (double)std::max(1, H.last - H.stageYear[0] + 1), 0, 1);
        fert *= SCRIPTS[H.cause].hazard == HZ_CHILDREN ? std::max(0.0, 1 - 1.3 * prog) : (stage >= STAGE_CRISIS ? 0.5 : 0.85);
    }
    if (hz[0] > 0.01) fert *= 0.7;
    // the families begin: ten households in the places standing, a couple each with their first children
    if (y == H.famFrom) {
        for (int f = 0; f < FAMILIES; f++) {
            int home = anyPlace(y, 1.5);
            for (int t = 0; t < 8 && home >= 0 && H.places[home].ended <= y; t++) home = anyPlace(y, 1.5);
            HistoryFamily F; F.name = mix64(H.seed ^ (0x40FAULL + f * 0x9E3779B97F4A7C15ULL)); F.home = home; F.trade = a * 10 + rng.irange(10); F.from = y; F.to = H.last;
            H.families.push_back(F);
            const int fi = (int)H.families.size() - 1;
            const int married = y - 2 - rng.irange(10);
            const int hb = person(married - M.marry - rng.irange(8), false, fi, home, HR_NONE, 2);
            const int wf = person(married - M.marry - rng.irange(6), true, fi, home, HR_NONE, 2);
            H.persons[hb].spouse = wf; H.persons[wf].spouse = hb;
            H.families[fi].head = hb; H.families[fi].founder = hb;
            const int me = ev(HE_MARRIAGE, std::max(H.first, married), home >= 0 && stands(home, married) ? home : -1, 0.1f); E(me).a = hb; E(me).b = wf;
            for (int c = married + 1; c < y; c++) if (rng.chance(M.births * 1.2)) {
                const int k = person(c, rng.chance(0.5), fi, home, HR_NONE, 2);
                link(k, wf, hb);
                const int be = ev(HE_BIRTH, c, home >= 0 && stands(home, c) ? home : -1, 0.1f); E(be).a = k; E(be).b = wf; E(be).c = hb;
            }
        }
    }
    // a family that died out: another household of the same place taken up in its place (the records need writers to the end)
    if (y > H.famFrom && y < H.stageYear[STAGE_CRISIS] && (y - H.famFrom) % 10 == 0) {
        std::vector<int> livingIn(H.families.size(), 0);
        for (int q : stepped) if (mode[q] == 2 && H.persons[q].died == ALIVE && H.persons[q].family >= 0) livingIn[H.persons[q].family]++;
        int followed = 0;
        for (size_t f = 0; f < H.families.size(); f++) if (!H.families[f].house && livingIn[f] > 0) followed++;
        for (int k = followed; k < FAMILIES; k++) {
            int home = anyPlace(y, 1.5);
            for (int t = 0; t < 8 && home >= 0 && H.places[home].ended <= y; t++) home = anyPlace(y, 1.5);
            HistoryFamily F; F.name = mix64(H.seed ^ (0x40FBULL + H.families.size() * 0x9E3779B97F4A7C15ULL)); F.home = home; F.trade = a * 10 + rng.irange(10); F.from = y; F.to = H.last;
            H.families.push_back(F);
            const int fi = (int)H.families.size() - 1;
            const int hb = person(y - M.marry - 2 - rng.irange(12), false, fi, home, HR_NONE, 2);
            H.families[fi].head = hb; H.families[fi].founder = hb;
            household(hb, y, fi);
        }
    }
    // the persons stepped: the families followed, a ruling house (its heirs, outside the families' years too), the figures (their deaths)
    std::vector<int> now;
    now.reserve(stepped.size());
    for (int q : stepped) if (mode[q] > 0 && H.persons[q].died == ALIVE) now.push_back(q);
    stepped.swap(now);
    famOf.assign(H.families.size(), 0);
    for (int q : stepped) if (mode[q] == 2 && H.persons[q].family >= 0) famOf[H.persons[q].family]++;
    auto alive = [&](int p) { return p >= 0 && H.persons[p].died == ALIVE; };
    const size_t n0 = stepped.size();
    for (size_t k = 0; k < n0; k++) {
        const int q = stepped[k];
        if (!alive(q) || mode[q] == 0) continue;
        const int years = y - H.persons[q].born;
        const bool female = H.persons[q].female;
        const int home = H.persons[q].home;
        // death: the age's, and the year's harm where they live (the battles for those of fighting age)
        const double nat = hazard(a, years);
        const int hk = home + 1;
        double ext = hz[0] + (hk > 0 && hk < (int)hz.size() ? hz[hk] : 0);
        const bool fighting = years >= 17 && years <= 45 && (!female || a >= AGE_MODERN);
        if (fighting) ext += hzBattle * 0.5;
        if (rng.chance(std::min(1.0, nat + ext))) {
            int cause = -1, how = years < 15 ? 11 : 0;
            if (ext > 0 && rng.chance(ext / (nat + ext))) {
                double best = hz[0]; int ce = hzEv[0], ch = hzHow[0];
                if (hk > 0 && hk < (int)hz.size() && hz[hk] > best) { best = hz[hk]; ce = hzEv[hk]; ch = hzHow[hk]; }
                if (fighting && hzBattle * 0.5 > best) { ce = hzBattleEv; ch = 3; }
                cause = ce; how = ch;
                if (cause < 0) how = years < 15 ? 11 : 0;
            } else if (years >= 15 && years < 50 && rng.chance(0.08)) how = 7;   // an accident
            die(q, y, how, cause);
            continue;
        }
        if (mode[q] != 2) continue;
        // marriage: within the household's place, now and then into another family followed
        const int sp0 = H.persons[q].spouse;
        if (sp0 < 0 || !alive(sp0)) {
            const bool widowed = sp0 >= 0;
            if (years >= M.marry && years <= 45 && rng.chance(widowed ? 0.06 : 0.16)) {
                int sp = -1;
                const int fam = H.persons[q].family;
                if (rng.chance(0.3)) {
                    for (int r2 : stepped) {
                        const HistoryPerson& R = H.persons[r2];
                        if (r2 == q || mode[r2] != 2 || R.died != ALIVE || R.female == female || R.family == fam || R.home != home) continue;
                        if (R.spouse >= 0 && H.persons[R.spouse].died == ALIVE) continue;
                        if (std::abs((y - R.born) - years) > 10 || y - R.born < M.marry) continue;
                        if (R.mother >= 0 && (R.mother == H.persons[q].mother || R.father == H.persons[q].father)) continue;
                        sp = r2; break;
                    }
                }
                if (sp < 0) {   // a husband a few years older as often as not, every spouse of the age to marry
                    const int spAge = clampi(years + (female ? rng.irange(9) - 2 : 2 - rng.irange(9)), M.marry, 60);
                    sp = person(y - spAge, !female, fam, home, HR_NONE, 2);
                }
                H.persons[q].spouse = sp; H.persons[sp].spouse = q;
                // the couple's home: the husband's or the wife's household by the people's custom
                const int husband = female ? sp : q, wife = female ? q : sp;
                const int to = residence == 0 ? H.persons[husband].home : H.persons[wife].home;
                for (int mv : {q, sp}) if (H.persons[mv].home != to && to >= 0) {
                    const int m = ev(HE_MOVE, y, H.persons[mv].home, 0.12f); E(m).a = mv; E(m).place2 = to; H.persons[mv].home = to;
                }
                const int me = ev(HE_MARRIAGE, y, to >= 0 && stands(to, y) ? to : -1, 0.15f); E(me).a = q; E(me).b = sp;
                // too many in the household: a branch off the family's main line (its head and the head's eldest living child; a ruling
                // house's head is its ruler) is let go, its children not followed
                if (fam >= 0 && fam < (int)famOf.size() && famOf[fam] > FAMILY_CAP) {
                    const int head = familyHead(fam);
                    bool mainLine = q == head || head < 0;
                    if (!mainLine) for (int c : kids[head]) if (H.persons[c].died == ALIVE) { mainLine = c == q; break; }
                    if (!mainLine) { letGo(q, y); letGo(sp, y); continue; }
                }
            }
        }
        // a child: a wife in her years with her husband alive
        const int hb = H.persons[q].spouse;
        if (female && alive(hb) && years >= M.marry && years <= M.fertileTo && rng.chance(fert)) {
            const int fam = residence == 0 ? H.persons[hb].family : H.persons[q].family;
            bool girl = rng.chance(0.5);
            uint64_t nm = 0;
            if (rng.chance(0.18)) {   // named after a grandparent
                const int gps[4] = {H.persons[q].mother, H.persons[q].father, H.persons[hb].mother, H.persons[hb].father};
                const int gp = gps[rng.irange(4)];
                if (gp >= 0) { nm = H.persons[gp].name; girl = H.persons[gp].female; }
            }
            const int c = person(y, girl, fam, home, HR_NONE, 2, nm);
            link(c, q, hb);
            const int be = ev(HE_BIRTH, y, home >= 0 && stands(home, y) ? home : -1, 0.1f); E(be).a = c; E(be).b = q; E(be).c = hb;
            if (rng.chance(a <= AGE_MEDIEVAL ? 0.012 : (a == AGE_INDUSTRIAL ? 0.006 : 0.0005))) die(q, y, 2, be);
            continue;
        }
        // a move to another place: the young to the cities, more in the industrial age
        static const double MOVE[AGE_COUNT] = {0.004, 0.006, 0.02, 0.03, 0.02, 0.01};
        if (years >= 18 && years <= 40 && home >= 0 && rng.chance(MOVE[a])) {
            const int to = anyPlace(y, 4, home);
            if (to >= 0 && to != home && H.places[to].ended > y) {
                const int m = ev(HE_MOVE, y, home, 0.12f); E(m).a = q; E(m).place2 = to;
                H.persons[q].home = to;
                const int sp = H.persons[q].spouse;
                if (alive(sp) && H.persons[sp].home == home) { const int m2 = ev(HE_MOVE, y, home, 0.1f); E(m2).a = sp; E(m2).place2 = to; H.persons[sp].home = to; }
                for (int c : kids[q]) if (alive(c) && y - H.persons[c].born < 16) H.persons[c].home = to;
            }
        }
    }
}

void Builder::yearLoop() {
    const PeopleProfile& P = H.profile;
    const int n = (int)H.places.size();
    hz.assign(n + 1, 0); hzEv.assign(n + 1, -1); hzHow.assign(n + 1, 0);
    H.yearsFrom = std::max(1, H.last - 25000);
    H.harvest.assign(H.last - H.yearsFrom + 1, 2);
    H.population.assign(H.last - H.yearsFrom + 1, 0);
    if (H.cause == CAUSE_SUN_LOST) { H.sunlight.resize(H.last - H.yearsFrom + 1); for (int y = H.yearsFrom; y <= H.last; y++) H.sunlight[y - H.yearsFrom] = (float)sunAt(y); }   // D-05
    size_t plannedAt = 0;
    int termEnd = 0;
    ruleEnds = H.stageYear[STAGE_CRISIS] + (int)std::lround(rng.range(0.3, 1.0) * (H.last - H.stageYear[STAGE_CRISIS]));
    if (ruleEnds >= H.last) ruleEnds = H.last;
    int lastEra = -1;
    for (int y = 1; y <= H.last; y++) {
        for (int k : hzTouched) { hz[k] = 0; hzEv[k] = -1; }
        hzTouched.clear();
        hzBattle = 0; hzBattleEv = -1;
        const int ei = H.eraAt(y);
        const HistoryEra& era = H.eras[ei];
        if (ei != lastEra) {
            if (lastEra < 0 || era.govFrom != govNow) govNow = era.govFrom;
            lastEra = ei;
        }
        if (y % 16 == 0) cleanStanding(y);
        // the foundings
        auto fy = foundByYear.find(y);
        if (fy != foundByYear.end()) for (int p : fy->second) {
            int founder;
            if (p == 0 && y == 1 && H.age != AGE_FAR_FUTURE) founder = person(1 - 30 - rng.irange(15), rng.chance(0.5), -1, 0, HR_FOUNDER, 1, NAME_FOUNDER);
            else if (H.age == AGE_FAR_FUTURE && p == seatOld) founder = person(1 - 30 - rng.irange(15), rng.chance(0.5), -1, p, HR_FOUNDER, 1, NAME_FOUNDER);
            else founder = figure(y, HR_FOUNDER, H.places[p].from >= 0 && stands(H.places[p].from, y) ? H.places[p].from : -1, 22, 50);
            H.persons[founder].home = p;
            if (H.persons[founder].role == HR_NONE) H.persons[founder].role = HR_FOUNDER;
            H.places[p].founder = founder;
            const int e = ev(HE_FOUNDING, y, p, p == 0 ? 0.9f : (big(p) ? 0.55f : 0.25f));
            if (y == 1 && (p == 0 || p == seatOld)) vacancyEv = e;   // the first ruler's rule begins with the capital
            E(e).a = founder; E(e).place2 = H.places[p].from >= 0 && stands(H.places[p].from, y) ? H.places[p].from : -1;
            int why = 0;
            const int sc = H.places[p].sclass;
            if (H.age == AGE_FAR_FUTURE && H.places[p].plan >= 0) why = 11;
            else if (p == 0 || p == seatOld) why = 12;
            else if (H.age == AGE_PREHISTORIC) why = 9;
            else if (H.age == AGE_FUTURISTIC && sc == SC_TOWN && H.places[p].age == AGE_FUTURISTIC) why = 10;
            else if (H.age == AGE_FAR_FUTURE && H.places[p].plan >= 0) why = 11;
            else if (H.places[p].age == AGE_INDUSTRIAL && H.places[p].plan >= 0 && G->res[PP->places[H.places[p].plan].cell] > 0.5) why = 4;
            else if (H.places[p].age == AGE_INDUSTRIAL && rng.chance(0.4)) why = 7;
            else if (H.places[p].coast && rng.chance(0.5)) why = 2;
            else if (H.places[p].river && rng.chance(0.4)) why = 3;
            else if (P.values[VAL_WAR] > 0.25 && H.places[p].age <= AGE_MEDIEVAL && rng.chance(0.35)) why = 1;
            else if (P.values[VAL_TRADE] > 0.25 && rng.chance(0.35)) why = 5;
            else if (P.values[VAL_PIETY] > 0.25 && rng.chance(0.35)) why = 6;
            E(e).thing = why;
            if (H.age == AGE_PREHISTORIC && p == 0) E(e).thing = 8;
            onFound(p);
        }
        // the clearings of the places a later people's plan does not draw
        auto cy = clearByYear.find(y);
        if (cy != clearByYear.end()) for (int p : cy->second) {
            if (H.places[p].ended != y) continue;
            const int e = ev(HE_ABANDONED, y, p, p == seatOld ? 0.7f : 0.4f); E(e).thing = 7;
            H.places[p].endEvent = e;
            const int to = nearestStanding(p, y + 1, false);
            for (int q : stepped) if (H.persons[q].died == ALIVE && H.persons[q].home == p) H.persons[q].home = to;
        }
        // the places left early with no event to blame (a band moved on, a mine worked out, the soil spent, a river's new course)
        auto ny = naturalEnds.find(y);
        if (ny != naturalEnds.end()) for (const auto& pw : ny->second) {
            const int p = pw.first;
            if (!stands(p, y) || p == seat(y) || earlyLeft * 7 > (int)H.places.size()) continue;
            leave(p, y, HP_EARLY, -1, pw.second); earlyLeft++;
        }
        // the festivals: the capital's hundreds, a city's hundredth and its five hundreds
        auto fe = festByYear.find(y);
        if (fe != festByYear.end()) for (int p : fe->second) {
            if (!stands(p, y)) continue;
            const int age2 = y - H.places[p].founded;
            const bool cap = p == seat(y);
            if (!cap && age2 % 1000 != 0) continue;
            const int e = ev(HE_FESTIVAL, y, p, cap ? (age2 % 500 == 0 ? 0.4f : 0.15f) : 0.1f); E(e).value = age2;
        }
        // the rule: the first ruler at year 1, the government's change, a term's end, a death, the odd deposition
        if (y == 1) {
            int founder = H.places.empty() ? -1 : H.places[seatOld >= 0 ? seatOld : 0].founder;
            if (founder < 0) founder = person(-30 - rng.irange(15), rng.chance(0.5), -1, seat(1), HR_FOUNDER, 1, NAME_FOUNDER);
            if (govNow == GOV_MONARCHY) { const int f = newHouse(y, founder); newReign(founder, y, 0, f); }
            else if (govNow == GOV_MACHINE) newReign(-1, y, 8);
            else newReign(founder, y, 0);
            termEnd = termOf(y) > 0 ? y + termOf(y) : 0;
        }
        if (era.govChange == y && govNow != era.government) {
            const int from = govNow, to = era.government;
            int how = 0;
            if (to == GOV_MONARCHY && from == GOV_CHIEFS) how = 2;
            else if (to == GOV_THEOCRACY) how = 3;
            else if (to == GOV_CORPORATE) how = 4;
            else if (to == GOV_MACHINE) how = 5;
            else if (to == GOV_COUNCIL) how = 7;
            else if (to == GOV_ASSEMBLY) how = 8;
            else if (to == GOV_MONARCHY) how = 6;
            else if (from == GOV_MONARCHY && rng.chance(0.6)) how = 1;
            const int ruler = reign >= 0 ? H.reigns[reign].ruler : -1;
            int leader = how == 2 && ruler >= 0 && H.persons[ruler].died == ALIVE ? ruler : -1;
            if (how == 1) {   // a revolution: a short war first
                const int rebel = figure(y, HR_REBEL, seat(y), 25, 45);
                leader = rebel;
                const int wi = startWar(y, HW_REVOLUTION, -1, seat(y), ruler >= 0 && H.persons[ruler].died == ALIVE ? ruler : -1, rebel, -1, 1 + rng.irange(3));
                (void)wi;
            } else if (leader < 0 && how != 5) leader = figure(y, how == 3 ? HR_PROPHET : HR_RULER, seat(y), 35, 60);
            const int e = ev(HE_GOVERNMENT, y, seat(y), 0.85f); E(e).thing = to; E(e).value = from; E(e).out = how; E(e).a = leader;
            govNow = to;
            if (how == 2 && leader >= 0) {   // the chief made a kingdom: the same person, a new reign and a house
                endReign(y, 7, e);
                const int f = newHouse(y, leader);
                newReign(leader, y, 0, f, leader);
            } else {
                endReign(y, how == 1 ? 4 : 7, e);
                if (how == 1 && ruler >= 0 && H.persons[ruler].died == ALIVE && rng.chance(0.3)) die(ruler, y, 9, e, seat(y));
                if (to == GOV_MACHINE) newReign(-1, y, 8, -1, ruler);
                else if (leader >= 0 && (how == 1 || how == 6 || how == 3)) {
                    if (to == GOV_MONARCHY) { const int f = newHouse(y, leader); newReign(leader, y, 4, f, ruler); }
                    else newReign(leader, y, how == 1 ? 9 : 2, -1, ruler);
                } else succeed(y, 2);
            }
            termEnd = termOf(y) > 0 ? y + termOf(y) : 0;
        }
        const bool ruleOver = ruleEnds > 0 && y >= ruleEnds;
        if (!ruleOver && reign >= 0 && H.reigns[reign].end == ALIVE) {
            if (termEnd > 0 && y >= termEnd) {
                const int prev = H.reigns[reign].ruler;
                const bool again = (govNow == GOV_REPUBLIC || govNow == GOV_ASSEMBLY) && prev >= 0 && H.persons[prev].died == ALIVE && rng.chance(govNow == GOV_REPUBLIC ? 0.3 : 0.4)
                                   && (reign < 1 || H.reigns[reign - 1].ruler != prev);
                if (again) { H.reigns[reign].end = y; H.reigns[reign].endHow = 3; newReign(prev, y, 10, -1, prev); }
                else { endReign(y, 3); succeed(y, 3); }
                termEnd = termOf(y) > 0 ? y + termOf(y) : 0;
            }
            else if (govNow != GOV_MACHINE && H.reigns[reign].ruler >= 0) {
                static const double DEPOSE[GOV_COUNT] = {0.008, 0.003, 0.002, 0.002, 0.002, 0.002, 0.005, 0};
                static const double MURDER[GOV_COUNT] = {0.004, 0.002, 0.0005, 0.001, 0.0005, 0.0005, 0.0005, 0};
                const int r = H.reigns[reign].ruler;
                if (rng.chance(DEPOSE[govNow])) {
                    endReign(y, 1);
                    if (govNow == GOV_MONARCHY) { const int a2 = figure(y, HR_RULER, seat(y), 25, 50, false); const int f = newHouse(y, a2); newReign(a2, y, 4, f, r); }
                    else succeed(y, 4);
                    termEnd = termOf(y) > 0 ? y + termOf(y) : 0;
                } else if (rng.chance(MURDER[govNow])) die(r, y, 8, -1, seat(y));
            }
            // a regency ends when the heir comes of age
            if (reign >= 0 && H.reigns[reign].regent >= 0 && H.reigns[reign].ruler >= 0 && y - H.persons[H.reigns[reign].ruler].born == 16 && H.reigns[reign].end == ALIVE) {
                const int e = ev(HE_ACCESSION, y, seat(y), 0.3f); E(e).a = H.reigns[reign].ruler; E(e).b = H.reigns[reign].regent; E(e).thing = 7; E(e).value = reign;
            }
        }
        // the inventions, the institutions, the faiths, the discoveries, the signs of the collapse
        while (plannedAt < planned.size() && planned[plannedAt].year < y) plannedAt++;
        plannedAtM = plannedAt;
        for (size_t k = plannedAt; k < planned.size() && planned[k].year == y; k++) {
            const Planned& q = planned[k];
            switch (q.kind) {
                case HE_INVENTION: {
                    int id = q.a;
                    const int p = pickPlace(y, [&](int x) { return x == seat(y) ? 4.0 : (big(x) ? 2.0 : 0.3); });
                    const int who = figure(y, HR_INVENTOR, p, 25, 60);
                    const int e = ev(HE_INVENTION, y, p, 0.6f); E(e).thing = id; E(e).a = who;
                    HistoryInvention I; I.id = id; I.year = y; I.person = who; I.place = p;
                    H.inventions.push_back(I);
                    break;
                }
                case HE_INSTITUTION: {
                    const int p = q.a == HI_COMPANY || q.a == HI_GUILDS ? pickPlace(y, [&](int x) { return (big(x) ? 2.0 : 0.3) * (H.places[x].coast ? 1.5 : 1.0); }) : seat(y);
                    HistoryInstitution I; I.kind = q.a; I.founded = y; I.ended = H.last; I.place = p; I.name = mix64(H.seed ^ (0x4015ULL + H.institutions.size()));
                    I.founder = q.a == HI_COUNCIL || q.a == HI_ASSEMBLY ? -1 : figure(y, q.a == HI_PRIESTHOOD ? HR_PROPHET : HR_MAKER, p, 30, 60);
                    H.institutions.push_back(I);
                    const int e = ev(HE_INSTITUTION, y, p, 0.45f); E(e).thing = (int)H.institutions.size() - 1; E(e).a = I.founder;
                    break;
                }
                case HE_FAITH: {
                    const int p = anyPlace(y);
                    const int who = figure(y, HR_PROPHET, p, 25, 55);
                    const int e = ev(HE_FAITH, y, p, 0.6f); E(e).thing = 0; E(e).a = who; E(e).value = q.b;
                    break;
                }
                case HE_DISCOVERY: {
                    const int p = q.a == 0 && q.b >= 0 ? (H.places[q.b].from >= 0 && stands(H.places[q.b].from, y) ? H.places[q.b].from : seat(y)) : anyPlace(y);
                    const int who = figure(y, HR_EXPLORER, p, 20, 50);
                    const int e = ev(HE_DISCOVERY, y, p, 0.5f); E(e).thing = q.a; E(e).a = who; E(e).value = q.a == 0 ? q.b : -1;
                    break;
                }
                case HE_SIGN: break;   // after the year's stages (below)
                default: break;
            }
        }
        // the ways opened
        auto wy = waysByYear.find(y);
        if (wy != waysByYear.end()) for (int e2 : wy->second) {
            const PlanEdge& pe = PP->edges[e2];
            if (!stands(pe.a, y) || !stands(pe.b, y)) continue;
            const int e = ev(HE_WAY, y, pe.a, pe.trunk ? 0.4f : 0.25f); E(e).place2 = pe.b; E(e).thing = e2; E(e).out = pe.kind;
        }
        // the other people's meeting, treaties and wars (C-13: both histories hold them)
        auto jy = jointByYear.find(y);
        if (jy != jointByYear.end()) for (int ji : jy->second) {
            const JointEvent& J = joint.events[ji];
            int mine = J.place[people];
            if (mine >= 0 && !stands(mine, y)) mine = nearestStanding(mine, y);
            if (mine < 0 && J.kind != HE_OTHER_END) mine = seat(y);
            const int theirs = J.place[1 - people];
            switch (J.kind) {
                case HE_CONTACT: { const int e = ev(HE_CONTACT, y, mine, J.salience); E(e).other = theirs; E(e).a = figure(y, HR_ENVOY, mine, 25, 55); break; }
                case HE_TREATY: { const int e = ev(HE_TREATY, y, mine, J.salience); E(e).other = theirs; E(e).thing = J.out; break; }
                case HE_WAR: {
                    const JointWar& JW = joint.wars[J.war];
                    HistoryWar w; w.kind = JW.kind; w.start = y; w.end = clampi(yearOf(P, JW.to), y, H.last); w.sideA = -1; w.sideB = -2;
                    w.winner = JW.winner < 0 ? -1 : (JW.winner == people ? 0 : 1); w.name = JW.name;
                    w.leaderA = reign >= 0 && H.reigns[reign].ruler >= 0 && H.persons[H.reigns[reign].ruler].died == ALIVE && rng.chance(0.4) ? H.reigns[reign].ruler : figure(y, HR_GENERAL, seat(y), 28, 55);
                    H.wars.push_back(w);
                    jointWar[J.war] = (int)H.wars.size() - 1;
                    const int e = ev(HE_WAR, y, mine, J.salience); E(e).thing = jointWar[J.war]; E(e).a = H.wars.back().leaderA; E(e).other = theirs; E(e).out = JW.aggressor == people ? 0 : 1;
                    H.wars.back().declared = e;
                    break;
                }
                case HE_BATTLE: {
                    const int wi = J.war >= 0 ? jointWar[J.war] : -1;
                    if (wi < 0) break;
                    HistoryWar& w = H.wars[wi];
                    const int side = J.out == people ? 0 : 1;
                    int L = w.leaderA;
                    if (L < 0 || H.persons[L].died != ALIVE) { L = figure(y, HR_GENERAL, seat(y), 28, 55); w.leaderA = L; }
                    const int e = ev(HE_BATTLE, y, mine, J.salience, J.season); E(e).thing = wi; E(e).out = side; E(e).value = J.value; E(e).other = theirs; E(e).cause = w.declared;
                    if (side == 0) E(e).a = L; else E(e).b = L;
                    w.dead += J.value; w.battles++;
                    jointLastBattle[J.war] = e;
                    const double share = J.value / std::max(1.0, pop(y) * 0.12);
                    if (share > hzBattle) { hzBattle = share; hzBattleEv = e; }
                    if (rng.chance(side == 0 ? 0.04 : 0.12) && H.persons[L].died == ALIVE) die(L, y, 3, e, mine);
                    if (w.kind == HW_CONQUEST && side == 1 && mine >= 0 && mine != H.lastPlace && rng.chance(0.4)) { const int sk = ev(HE_SACK, y, mine, 0.6f); E(sk).thing = wi; E(sk).cause = e; E(sk).value = (int64_t)(placePop(mine, y) * rng.range(0.05, 0.3)); lastHazardEv = sk; harm(mine, 0.1, sk, 3); }
                    break;
                }
                case HE_PEACE: {
                    const int wi = J.war >= 0 ? jointWar[J.war] : -1;
                    if (wi < 0) break;
                    HistoryWar& w = H.wars[wi];
                    const int e = ev(HE_PEACE, y, mine, J.salience); E(e).thing = wi; E(e).out = J.out == 2 ? 2 : (J.out == people ? 0 : 1); E(e).other = theirs;
                    E(e).cause = jointLastBattle[J.war] >= 0 ? jointLastBattle[J.war] : w.declared;
                    E(e).a = w.leaderA >= 0 && H.persons[w.leaderA].died == ALIVE ? w.leaderA : -1;
                    w.peace = e; w.end = y;
                    break;
                }
                case HE_OTHER_END: if (J.out != people) { const int e = ev(HE_OTHER_END, y, -1, J.salience); (void)e; } break;
                default: break;
            }
        }
        // a conquest by the other people closes when the conquered people ends
        for (HistoryWar& w : H.wars) if (w.kind == HW_CONQUEST && w.sideB == -2 && w.peace < 0 && y == H.last && w.end >= H.last) w.end = H.last;
        randomEvents(y);
        everyday(y);
        if (y >= P.collapseYear) collapseYear(y);
        stepWars(y);
        // a sickness's waves: where it strikes this year, and the places near it
        for (size_t k = 0; k < waves.size();) {
            Wave& wv = waves[k];
            if (wv.year != y) { k++; continue; }
            HistorySickness& S = H.sicknesses[wv.sickness];
            int at = wv.place >= 0 && stands(wv.place, y) ? wv.place : anyPlace(y);
            const int64_t dead = (int64_t)(pop(y) * wv.share);
            int first = -1;
            for (int t = 0; t < (wv.share > 0.06 ? 2 : 1) && at >= 0; t++) {
                const int e = ev(HE_SICKNESS, y, at, wv.n == 1 ? 0.6f : 0.45f); E(e).thing = wv.sickness; E(e).out = wv.n; E(e).value = t == 0 ? dead : dead / 3;
                if (first < 0) first = e; else E(e).cause = first;
                harm(at, wv.share * 3, e, 1);
                if (t == 0) maybeLeave(at, y, e, H.places[at].sclass == SC_TOWN ? 0.03 : 0.01);
                at = nearestStanding(at, y, false);
            }
            harm(-1, wv.share * 0.5, first, 1);
            if (P.stageAt(y) >= STAGE_ONSET) lastHazardEv = first;
            S.last = y; S.waves = std::max(S.waves, wv.n); S.dead += dead;
            deficit += wv.share;
            waves.erase(waves.begin() + k);
        }
        // the rule after a death or a term: the next ruler (the collapse's end leaves none)
        if (vacant && !(ruleEnds > 0 && y >= ruleEnds)) { succeed(y, govNow == GOV_REPUBLIC || govNow == GOV_ASSEMBLY ? 3 : 2); termEnd = termOf(y) > 0 ? y + termOf(y) : 0; }
        stepPersons(y);
        if (vacant && !(ruleEnds > 0 && y >= ruleEnds)) { succeed(y, 2); termEnd = termOf(y) > 0 ? y + termOf(y) : 0; }
        // anyone whose place ended this year (moved in late, born there, a household taken up there) goes on to the nearest place that
        // stands the next year
        bool ended = false;
        if (endsByYear.count(y) || clearByYear.count(y) || leftThisYear == y) ended = true;
        if (y < H.last && ended) for (int q : stepped) {
            HistoryPerson& Q = H.persons[q];
            if (Q.died != ALIVE || Q.home < 0 || H.places[Q.home].ended > y) continue;
            const int to = nearestStanding(Q.home, y + 1, false);
            Q.home = to >= 0 ? to : H.lastPlace;
        }
        // the people's number
        deficit = std::min(0.6, deficit) * 0.96;
        const int yi = y - H.yearsFrom;
        if (yi >= 0 && yi < (int)H.population.size()) H.population[yi] = (float)pop(y);
    }
    // the end: the last place, the last person, everyone left
    const int lp = H.lastPlace >= 0 ? H.lastPlace : seat(H.last);
    int lastP = -1;
    for (int q : stepped) if (H.persons[q].died == ALIVE && mode[q] == 2 && H.persons[q].home == lp && H.last - H.persons[q].born >= 14) { if (lastP < 0 || H.persons[q].born < H.persons[lastP].born) lastP = q; }
    if (lastP < 0) for (int q : stepped) if (H.persons[q].died == ALIVE && H.last - H.persons[q].born >= 14) { lastP = q; break; }
    if (lastP < 0) lastP = person(H.last - 25 - rng.irange(40), rng.chance(0.5), -1, lp, HR_LAST, 1);
    if (H.persons[lastP].role == HR_NONE) H.persons[lastP].role = HR_LAST;
    if (H.persons[lastP].home != lp && lp >= 0) { const int m = ev(HE_MOVE, H.last, H.persons[lastP].home, 0.3f); E(m).a = lastP; E(m).place2 = lp; H.persons[lastP].home = lp; }
    if (reign >= 0 && H.reigns[reign].end == ALIVE) endReign(H.last, 6);
    const int endE = ev(HE_END, H.last, lp, 1.0f, H.seasons - 1); E(endE).a = lastP; E(endE).cause = lastHazardEv;
    if (lp >= 0) { H.places[lp].ended = H.last; H.places[lp].fate = HP_LAST; H.places[lp].endEvent = endE; }
    H.lastPerson = lastP;
    const int whither = SCRIPTS[H.cause].whither;
    const bool leaving = SCRIPTS[H.cause].hazard == HZ_LEAVING;
    for (int q = 0; q < (int)H.persons.size(); q++) {
        HistoryPerson& Q = H.persons[q];
        if (Q.died != ALIVE) continue;
        if (leaving && q != lastP) { Q.died = H.last; Q.fate = HF_LEFT; mode[q] = 0; continue; }
        die(q, H.last, 10, endE, lp);
    }
    (void)whither;
    for (HistoryReign& R : H.reigns) if (R.end == ALIVE) R.end = H.last;
    for (HistoryInstitution& I : H.institutions) I.ended = std::min(I.ended, H.last);
    for (HistoryWar& w : H.wars) if (w.end > H.last) w.end = H.last;
}

void Builder::finish() {
    // an effect never in a season before its cause's (a death in its sickness's season, an accession after the death that opened the rule)
    for (HistoryEvent& e : H.events) if (e.cause >= 0 && H.events[e.cause].year == e.year && H.events[e.cause].season > e.season) e.season = H.events[e.cause].season;
    // the events by their year and season, then in the order they happened; every index into them remapped
    const int n = (int)H.events.size();
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        const HistoryEvent& A = H.events[a]; const HistoryEvent& B = H.events[b];
        if (A.year != B.year) return A.year < B.year;
        return A.season < B.season;
    });
    std::vector<int> at(n);
    for (int i = 0; i < n; i++) at[order[i]] = i;
    std::vector<HistoryEvent> sorted(n);
    for (int i = 0; i < n; i++) sorted[i] = H.events[order[i]];
    for (HistoryEvent& e : sorted) if (e.cause >= 0) e.cause = at[e.cause];
    H.events.swap(sorted);
    for (HistoryPerson& p : H.persons) if (p.death >= 0) p.death = at[p.death];
    for (HistoryPlace& p : H.places) if (p.endEvent >= 0) p.endEvent = at[p.endEvent];
    for (HistoryWar& w : H.wars) { if (w.declared >= 0) w.declared = at[w.declared]; if (w.peace >= 0) w.peace = at[w.peace]; }
    // the rulers' numbers: a name the rulers shared counted in order (the first a I only where a second came)
    std::map<uint64_t, int> seen, total;
    for (const HistoryReign& R : H.reigns) if (R.ruler >= 0) {
        bool repeat = false; for (const HistoryReign& Q : H.reigns) { if (&Q == &R) break; if (Q.ruler == R.ruler) repeat = true; }
        if (!repeat) total[H.persons[R.ruler].name]++;
    }
    for (const HistoryReign& R : H.reigns) if (R.ruler >= 0) {
        HistoryPerson& p = H.persons[R.ruler];
        if (p.ordinal) continue;
        const int k = ++seen[p.name];
        if (total[p.name] > 1) p.ordinal = k;
    }
    // the indices
    H.publicEvents = 0;
    for (const HistoryEvent& e : H.events) if (e.pub) H.publicEvents++;
    const int np = (int)H.persons.size(), nl = (int)H.places.size();
    std::vector<int> cp(np + 1, 0), cl(nl + 1, 0);
    for (const HistoryEvent& e : H.events) {
        for (int x : {e.a, e.b, e.c}) if (x >= 0 && x < np) cp[x + 1]++;
        for (int x : {e.place, e.place2}) if (x >= 0 && x < nl) cl[x + 1]++;
    }
    for (int i = 0; i < np; i++) cp[i + 1] += cp[i];
    for (int i = 0; i < nl; i++) cl[i + 1] += cl[i];
    H.personFirst = cp; H.placeFirst = cl;
    H.personEvents.assign(cp[np], 0); H.placeEvents.assign(cl[nl], 0);
    std::vector<int> fp(cp.begin(), cp.end() - 1), fl(cl.begin(), cl.end() - 1);
    for (int i = 0; i < (int)H.events.size(); i++) {
        const HistoryEvent& e = H.events[i];
        int prevP = -2;
        for (int x : {e.a, e.b, e.c}) if (x >= 0 && x < np && x != prevP) { H.personEvents[fp[x]++] = i; prevP = x; }
        if (e.place >= 0 && e.place < nl) H.placeEvents[fl[e.place]++] = i;
        if (e.place2 >= 0 && e.place2 < nl && e.place2 != e.place) H.placeEvents[fl[e.place2]++] = i;
    }
    // trim the unfilled tails (a person named twice in one event counted once)
    for (int i = 0; i < np; i++) for (int k = fp[i]; k < cp[i + 1]; k++) H.personEvents[k] = -1;
    for (int i = 0; i < nl; i++) for (int k = fl[i]; k < cl[i + 1]; k++) H.placeEvents[k] = -1;
}
