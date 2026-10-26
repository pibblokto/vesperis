#pragma once
// E-10: history.cpp's builder, for its parts alone (history, history_setup, history_events, history_years): `Builder`, the state one
// history's build keeps, with its small helpers (in the class, so inline), and the types it holds: the founding schedule, the
// other people's events and wars
#include "history_tables.h"

struct Schedule {
    std::vector<int> founded, age;   // per place of the plan
};

// a battle's dead at most, by the age that fought it (an age's armies and weapons: a band's feud to a modern people's war)
constexpr double BATTLE_CAP[AGE_COUNT] = {300, 30000, 200000, 600000, 100000, 10000};
// the other people (C-13): the meeting, the treaties and the wars both histories hold, in years before the world's present
struct JointEvent { int kind = HE_CONTACT; int64_t ago = 0; int place[2] = {-1, -1}; int war = -1; int out = 0; int64_t value = 0; int season = -1; float salience = 0.5f; };
struct JointWar { int kind = HW_OTHER; int64_t from = 0, to = 0; int winner = -1; int64_t dead = 0; uint64_t name = 0; int aggressor = 0; };
struct Joint { std::vector<JointEvent> events; std::vector<JointWar> wars; bool met = false; };
inline int yearOf(const PeopleProfile& P, int64_t ago) { return P.lastYear - (int)(ago - (int64_t)std::llround(P.endedYearsAgo)); }

// ---- the builder ---------------------------------------------------------------------------------------------------------------
struct Builder {
    const StarSystem& sys; const Body& body; const BodyGen& g; const int people;
    History& H;
    std::shared_ptr<const WorldPlan> WP;
    const PeoplePlan* PP = nullptr;
    const PlanGrid* G = nullptr;
    Genome gene;
    WorldRead W;
    Rng rng;
    Schedule sched;
    std::vector<std::vector<float>> ageScores;   // the older ages' scores over the territory (the founding's eras, the cleared places)
    std::vector<uint8_t> mode;           // per person: 0 not stepped, 1 its death alone, 2 its whole life (marriage, children)
    std::vector<int> stepped;
    std::vector<double> hz;              // the year's harm per place (index + 1; 0 the whole land): the share it kills
    std::vector<int> hzEv, hzHow;
    double hzBattle = 0; int hzBattleEv = -1;   // the year's battles: the share of the fighting age they killed
    int reign = -1;
    bool vacant = false;
    int govNow = GOV_CHIEFS;
    int primo = 0, residence = 0, termRepublic = 4, termAssembly = 3;
    int seatOld = -1;                    // the far future's old capital (a cleared place): the seat until it was cleared
    double deficit = 0;
    std::vector<std::pair<int, double>> anchors;   // the people's number: (year, how many) between which it grew
    int earlyLeft = 0;
    int lastHazardEv = -1;               // the collapse's latest harm (a place left in it names it)
    int ruleEnds = 0;                    // the year the rule ended in the collapse
    int vacancyEv = -1;                  // what opened the rule (a ruler's death, a rule's end): the next accession names it
    int lastFamine = -100;
    int lastAccessionEv = -1;
    int leftThisYear = 0;                 // the year a place was last left early (the year's end rehouses its people)
    int stageEv[STAGE_COUNT] = {-1, -1, -1, -1};
    size_t plannedAtM = 0;                // the planned list's first of the year (the collapse's step reads its signs)
    int lastSignEv = -1;
    std::set<int> lawGivers;
    int lastHouse = -1;                  // the house that ruled last
    struct WarRun { int war; std::vector<int> battles; int siegeAt = -1; int target = -1; int end = 0; int lastBattle = -1; };
    std::vector<WarRun> warsOn;
    struct Wave { int year; int sickness; int place; int n; double share; int first; };
    std::vector<Wave> waves;
    struct Planned { int year; int kind; int a; int b; };   // inventions, institutions, faiths, discoveries, signs
    std::vector<Planned> planned;
    std::vector<std::vector<int>> wayAt;  // per year offset: the plan's edges opened then (sparse map instead)
    std::map<int, std::vector<int>> waysByYear;
    std::map<int, std::vector<int>> endsByYear;   // the collapse's: the places left in a year
    Joint joint; std::map<int, std::vector<int>> jointByYear;   // joint events by this people's year
    std::vector<int> jointWar;            // joint war -> this history's war
    std::vector<int> jointLastBattle;     // joint war -> its last battle here
    int cometPeriod = 0, cometPhase = 0;
    bool auroral = false;
    std::vector<int> famOf;               // per family: its living count (rebuilt each year)
    std::vector<std::vector<int>> kids;   // per person: the children, the eldest first
    std::map<int, std::vector<int>> foundByYear, festByYear, clearByYear;
    std::map<int, std::vector<std::pair<int, int>>> naturalEnds;   // a place left early with no event to blame: (place, why)
    std::vector<int> standBig, standSmall;   // the places founded (some since left: dropped when met)

    std::vector<int> sunOmens;           // D-05: the years the omens of the lost sun's way were seen (the warning's)
    int sunSeen = 0;                     // the HE_SUN things seen so far (bits)

    Builder(const StarSystem& s, const Body& b, const BodyGen& gg, int p, History& h) : sys(s), body(b), g(gg), people(p), H(h) {}

    bool airFrozenThen() const { return g.hasTrait(TR_FROZEN_AIR); }   // its air fell as snow in the dark (the world's own trait now)
    // the light of a drifter's people's sun by year over its old light: whole to the onset, then falling as the orbit widened (ten times
    // the distance by the last sunrise, its light the inverse square of it), the stars' alone after the last sunrise
    double sunAt(int y) const {
        if (H.cause != CAUSE_SUN_LOST) return 1;
        return sunLightOf(H.stageYear[STAGE_ONSET], H.profile.lastSunrise, y);
    }

    // ---- small helpers
    int ageAt(int y) const { return H.eras[H.eraAt(y)].age; }
    double base(int y) const {
        if (anchors.empty()) return 1;
        if (y <= anchors.front().first) return anchors.front().second;
        for (size_t k = 1; k < anchors.size(); k++) {
            if (y > anchors[k].first) continue;
            const double t = smoothstep(0, 1, (double)(y - anchors[k - 1].first) / std::max(1, anchors[k].first - anchors[k - 1].first));
            return std::exp(std::log(anchors[k - 1].second) + t * (std::log(anchors[k].second) - std::log(anchors[k - 1].second)));
        }
        return anchors.back().second;
    }
    double pop(int y) const { return std::max(1.0, base(y) * (1 - deficit)); }
    double placePop(int p, int y) const {
        if (p < 0 || p >= (int)H.places.size()) return pop(y);
        const double share = H.places[p].peak / std::max(1.0, H.profile.peak);
        return std::max(40.0, share * pop(y) * (H.places[p].sclass == SC_CAPITAL ? 1.0 : 1.2));
    }
    int seasonFor(int kind) {
        if (H.seasons == 4) {
            switch (kind) {
                case HE_FOUNDING: case HE_FLOOD: if (rng.chance(0.6)) return 0; break;
                case HE_HARVEST: case HE_FAMINE: if (rng.chance(0.7)) return kind == HE_HARVEST ? 2 : 3; break;
                case HE_BATTLE: case HE_SIEGE: case HE_SACK: if (rng.chance(0.7)) return 1 + rng.irange(2); break;
                case HE_SICKNESS: if (rng.chance(0.5)) return 1; break;
                case HE_WINTER: return 3;
                default: break;
            }
            return rng.irange(4);
        }
        if (kind == HE_FLOOD && rng.chance(0.7)) return 0;
        if (kind == HE_HARVEST && rng.chance(0.7)) return 1;
        return rng.irange(2);
    }
    int ev(int kind, int year, int place = -1, float sal = 0.5f, int season = -2) {
        HistoryEvent e; e.kind = kind; e.year = year; e.place = place; e.salience = sal;
        e.pub = kind < HE_BIRTH;
        e.season = season == -2 ? seasonFor(kind) : season;
        H.events.push_back(e);
        return (int)H.events.size() - 1;
    }
    HistoryEvent& E(int i) { return H.events[i]; }
    bool stands(int p, int y) const { return H.placeStands(p, y); }
    int seat(int y) const {   // the seat of the rule
        if (seatOld >= 0 && stands(seatOld, y)) return seatOld;
        if (!H.places.empty() && stands(0, y)) return 0;
        for (int p = 0; p < (int)H.places.size(); p++) if (stands(p, y)) return p;
        return -1;
    }
    bool big(int p) const { return p >= 0 && (H.places[p].sclass == SC_CAPITAL || H.places[p].sclass == SC_CITY || (H.age == AGE_FUTURISTIC && H.places[p].sclass == SC_TOWN)); }
    // a standing place by weight (`w` may return 0 to skip one); -1 none
    template <class F> int pickPlace(int y, F w) {
        double tot = 0;
        for (int p = 0; p < (int)H.places.size(); p++) if (stands(p, y)) tot += w(p);
        if (tot <= 0) return -1;
        double u = rng.uni() * tot;
        for (int p = 0; p < (int)H.places.size(); p++) {
            if (!stands(p, y)) continue;
            const double x = w(p);
            if (x <= 0) continue;
            u -= x;
            if (u < 0) return p;
        }
        for (int p = (int)H.places.size() - 1; p >= 0; p--) if (stands(p, y) && w(p) > 0) return p;
        return -1;
    }
    // a standing place, the seat a quarter of the time, a great place `bigW` times as likely as a small one (from the kept lists)
    int anyPlace(int y, double bigW = 3, int notThis = -1) {
        const int s = seat(y);
        if (s >= 0 && s != notThis && rng.chance(0.25)) return s;
        for (int t = 0; t < 12; t++) {
            const double wb = bigW * standBig.size(), ws = (double)standSmall.size();
            if (wb + ws <= 0) break;
            std::vector<int>& L = rng.uni() * (wb + ws) < wb ? standBig : standSmall;
            if (L.empty()) continue;
            const int k = rng.irange((int)L.size());
            const int p = L[k];
            if (!stands(p, y)) { L[k] = L.back(); L.pop_back(); continue; }
            if (p == notThis) continue;
            return p;
        }
        return pickPlace(y, [&](int p) { return p == notThis ? 0.0 : (big(p) ? bigW : 1.0); });
    }
    void onFound(int p) { (big(p) ? standBig : standSmall).push_back(p); }
    void cleanStanding(int y) {
        for (std::vector<int>* L : {&standBig, &standSmall}) {
            std::vector<int> keep;
            for (int p : *L) if (H.places[p].ended >= y) keep.push_back(p);
            L->swap(keep);
        }
    }
    std::vector<Vec3> unit;               // per place: where it stands
    double distKm(int a, int b) const { return std::acos(clampd(dot(unit[a], unit[b]), -1, 1)) * g.R; }
    int nearestStanding(int from, int y, bool wantBig = false) {
        int best = -1; double bd = 1e300;
        for (int p = 0; p < (int)H.places.size(); p++) {
            if (p == from || !stands(p, y) || (wantBig && !big(p))) continue;
            const double d = from >= 0 ? -dot(unit[from], unit[p]) : p;   // the nearer the greater the dot
            if (d < bd) { bd = d; best = p; }
        }
        return best;
    }

    // ---- persons
    int person(int born, bool female, int family, int home, int role, int md, uint64_t name = 0) {
        HistoryPerson p;
        p.name = name ? name : mix64(H.seed ^ (0x4E50ULL + H.persons.size() * 0x9E3779B97F4A7C15ULL));
        p.female = female; p.born = born; p.died = ALIVE; p.family = family; p.home = home; p.role = role;
        const int a = ageAt(std::max(H.first, born + 20));
        p.trade = family >= 0 && rng.chance(0.6) ? H.families[family].trade : a * 10 + rng.irange(10);
        uint32_t t = 0;
        const double* v = H.profile.values;
        if (rng.chance(0.2 + v[VAL_WAR])) t |= HT_BRAVE;
        if (rng.chance(0.08 + 0.3 * v[VAL_WAR])) t |= HT_CRUEL;
        if (rng.chance(0.1 + v[VAL_PIETY])) t |= HT_PIOUS;
        if (rng.chance(0.15)) t |= HT_WISE;
        if (rng.chance(0.08 + v[VAL_LEARNING])) t |= HT_LEARNED;
        if (rng.chance(0.15)) t |= HT_AMBITIOUS;
        if (rng.chance(0.2)) t |= HT_KIND;
        if (rng.chance(0.12)) t |= HT_PROUD;
        p.traits = t;
        H.persons.push_back(p);
        mode.push_back((uint8_t)md);
        kids.emplace_back();
        if (md > 0) stepped.push_back((int)H.persons.size() - 1);
        if (family >= 0) H.families[family].persons++;
        return (int)H.persons.size() - 1;
    }
    void link(int child, int mother, int father) {
        H.persons[child].mother = mother; H.persons[child].father = father;
        if (mother >= 0) kids[mother].push_back(child);
        if (father >= 0) kids[father].push_back(child);
    }
    int figure(int y, int role, int home, int minAge, int maxAge, bool fromFamilies = true) {
        if (fromFamilies && y >= H.famFrom && rng.chance(0.2)) {   // one of the families followed, now and then
            std::vector<int> c;
            for (int q : stepped) { const HistoryPerson& P = H.persons[q]; if (mode[q] == 2 && P.died == ALIVE && P.role == HR_NONE && y - P.born >= minAge && y - P.born <= maxAge && (home < 0 || P.home == home)) c.push_back(q); }
            if (!c.empty()) { const int q = c[rng.irange((int)c.size())]; H.persons[q].role = role; return q; }
        }
        return person(y - minAge - rng.irange(std::max(1, maxAge - minAge)), rng.chance(0.5), -1, home, role, 1);
    }
    double hazard(int age, int years) const {
        const Mortality& M = MORTALITY[clampi(age, 0, AGE_COUNT - 1)];
        if (years < 1) return M.infant;
        if (years < 15) return M.child;
        if (years >= M.maxAge) return 1;
        return std::min(1.0, M.A + M.B * std::exp(M.c * years));
    }
    void die(int p, int y, int how, int cause, int place = -2) {
        HistoryPerson& P = H.persons[p];
        if (P.died != ALIVE) return;
        P.died = y; P.fate = HF_DIED;
        const int at = place == -2 ? P.home : place;
        const int role = P.role;
        bool pubDeath = role == HR_RULER || role == HR_REGENT || role == HR_GENERAL || role == HR_PROPHET || role == HR_INVENTOR || role == HR_EXPLORER || role == HR_REBEL || role == HR_LAST;
        if (role == HR_RULER && !(reign >= 0 && H.reigns[reign].ruler == p && H.reigns[reign].end == ALIVE)) {
            for (int k = (int)H.reigns.size() - 1; k >= 0; k--) if (H.reigns[k].ruler == p) {
                const int gv = H.reigns[k].government;
                if (gv == GOV_REPUBLIC || gv == GOV_ASSEMBLY || gv == GOV_CORPORATE) pubDeath = false;
                break;
            }
        }
        if (role == HR_FOUNDER) for (int q = 0; q < (int)H.places.size() && !pubDeath; q++) if (H.places[q].founder == p && big(q)) pubDeath = true;
        const int e = ev(HE_DEATH, y, at >= 0 && stands(at, y) ? at : -1, pubDeath ? (role == HR_RULER ? 0.6f : 0.35f) : 0.2f, cause >= 0 ? H.events[cause].season : -2);
        E(e).a = p; E(e).thing = how; E(e).cause = cause; E(e).pub = pubDeath;
        H.persons[p].death = e;
        mode[p] = 0;
        if (reign >= 0 && H.reigns[reign].ruler == p && H.reigns[reign].end == ALIVE) { H.reigns[reign].end = y; H.reigns[reign].endHow = 0; vacant = true; vacancyEv = e; }
    }
    void letGo(int p, int y) {   // followed no further: its death drawn alone (natural), told nowhere (never a person of the rule or a figure)
        HistoryPerson& P = H.persons[p];
        if (P.died != ALIVE || P.role != HR_NONE) return;
        const int a = ageAt(y);
        int d = y;
        while (d < H.last && !rng.chance(hazard(a, d - P.born))) d++;
        P.died = std::min(d, H.last); P.fate = HF_LET_GO; mode[p] = 0;
    }
    std::vector<int> childrenOf(int p) const { return p >= 0 && p < (int)kids.size() ? kids[p] : std::vector<int>(); }

    // ---- the rule
    int newReign(int ruler, int y, int how, int house = -1, int predecessor = -1) {
        HistoryReign R; R.ruler = ruler; R.start = y; R.end = ALIVE; R.government = govNow; R.house = house; R.how = how;
        H.reigns.push_back(R);
        reign = (int)H.reigns.size() - 1;
        vacant = false;
        if (ruler >= 0) { H.persons[ruler].role = HR_RULER; if (mode[ruler] == 0 && H.persons[ruler].died == ALIVE) { mode[ruler] = 1; stepped.push_back(ruler); } }
        const int e = ev(HE_ACCESSION, y, seat(y), how == 10 ? 0.25f : 0.55f);
        E(e).a = ruler; E(e).b = predecessor; E(e).thing = how; E(e).value = reign; E(e).cause = vacancyEv;
        vacancyEv = -1;
        lastAccessionEv = e;
        // a house that lost the rule: its members are followed no further (its branches would grow without end)
        if (lastHouse >= 0 && lastHouse != house) for (int q : stepped) if (mode[q] == 2 && H.persons[q].family == lastHouse && H.persons[q].died == ALIVE) letGo(q, y);
        if (house >= 0) { lastHouse = house; if (ruler >= 0) H.families[house].head = ruler; }
        return reign;
    }
    void endReign(int y, int how, int cause = -1) {
        if (reign < 0 || H.reigns[reign].end != ALIVE) return;
        H.reigns[reign].end = y; H.reigns[reign].endHow = how;
        const int r = H.reigns[reign].ruler;
        if (how != 0) { const int e = ev(HE_RULE_END, y, seat(y), how == 6 ? 0.7f : (how == 3 ? 0.15f : 0.4f)); E(e).a = r; E(e).thing = how; E(e).cause = cause; vacancyEv = e; if (how == 3) E(e).pub = false; }
        else vacancyEv = cause;
        vacant = true;
    }
    // a monarchy's heir: the late ruler's children by the people's rule, then the siblings, then the house's nearest
    int heirOf(int ruler, int y) {
        if (ruler < 0) return -1;
        auto order = [&](std::vector<int>& v) {
            std::stable_sort(v.begin(), v.end(), [&](int a, int b) {
                const HistoryPerson& A = H.persons[a]; const HistoryPerson& B = H.persons[b];
                if (primo != 1 && A.female != B.female) return primo == 0 ? !A.female : A.female;
                return A.born < B.born;
            });
        };
        auto livingOf = [&](const std::vector<int>& v) { std::vector<int> o; for (int q : v) if (H.persons[q].died == ALIVE) o.push_back(q); return o; };
        std::vector<int> kids = livingOf(childrenOf(ruler));
        order(kids);
        if (!kids.empty()) return kids[0];
        const HistoryPerson& R = H.persons[ruler];
        // the grandchildren through a child who died
        std::vector<int> grand;
        for (int c : childrenOf(ruler)) for (int gc : livingOf(childrenOf(c))) grand.push_back(gc);
        order(grand);
        if (!grand.empty()) return grand[0];
        const int par = R.father >= 0 ? R.father : R.mother;
        if (par >= 0) {
            std::vector<int> sib;
            for (int s : childrenOf(par)) if (s != ruler && H.persons[s].died == ALIVE) sib.push_back(s);
            order(sib);
            if (!sib.empty()) return sib[0];
            std::vector<int> neph;
            for (int s : childrenOf(par)) if (s != ruler) for (int n : livingOf(childrenOf(s))) neph.push_back(n);
            order(neph);
            if (!neph.empty()) return neph[0];
        }
        return -1;
    }
    int newHouse(int y, int founder) {
        HistoryFamily F; F.name = H.persons[founder].name; F.head = founder; F.founder = founder; F.home = seat(y); F.trade = 0; F.from = y; F.to = H.last; F.house = true;
        H.families.push_back(F);
        const int f = (int)H.families.size() - 1;
        H.persons[founder].family = f;
        H.families[f].persons++;
        if (mode[founder] < 2) { if (mode[founder] == 0) stepped.push_back(founder); mode[founder] = 2; }
        household(founder, y, f);
        return f;
    }
    // a person's household as it stood when the history took them up: a spouse and the children living (those who died young are not told)
    void household(int who, int y, int fam) {
        const int a = ageAt(y);
        const Mortality& M = MORTALITY[a];
        const int born = H.persons[who].born, years = y - born;
        if (H.persons[who].spouse >= 0 || years < M.marry + 3) return;
        const bool female = H.persons[who].female;
        const int home = H.persons[who].home;
        const int married = born + M.marry + rng.irange(std::max(1, std::min(10, years - M.marry - 2)));
        const int sp = person(std::min(born + rng.irange(9) - 4 + (female ? 3 : -3), married - M.marry), !female, fam, home, HR_NONE, 2);
        H.persons[who].spouse = sp; H.persons[sp].spouse = who;
        const int me = ev(HE_MARRIAGE, married, home >= 0 && stands(home, married) ? home : -1, 0.15f); E(me).a = who; E(me).b = sp;
        const int mother = female ? who : sp, father = female ? sp : who;
        const int mBorn = H.persons[mother].born;
        for (int c = married + 1; c < y && c - mBorn <= M.fertileTo; c++) {
            if (!rng.chance(M.births)) continue;
            double surv = 1;
            for (int t = 0; t < y - c; t++) surv *= 1 - hazard(a, t);
            if (!rng.chance(surv)) continue;
            const int k = person(c, rng.chance(0.5), fam, home, HR_NONE, 2);
            link(k, mother, father);
            const int be = ev(HE_BIRTH, c, home >= 0 && stands(home, c) ? home : -1, 0.1f); E(be).a = k; E(be).b = mother; E(be).c = father;
        }
    }
    // the family's head now: the first followed, then the eldest living child of the head before (a grandchild through a dead child, a
    // living member at last)
    int familyHead(int fam) {
        int h = H.families[fam].head;
        for (int guard = 0; guard < 64 && h >= 0 && H.persons[h].died != ALIVE; guard++) {
            int next = -1;
            std::vector<int> frontier = kids[h];
            for (int depth = 0; depth < 4 && next < 0 && !frontier.empty(); depth++) {
                for (int c : frontier) if (H.persons[c].died == ALIVE && H.persons[c].family == fam && mode[c] == 2) { next = c; break; }
                std::vector<int> nx; for (int c : frontier) for (int gc : kids[c]) nx.push_back(gc);
                frontier.swap(nx);
            }
            if (next < 0) for (int q : stepped) if (mode[q] == 2 && H.persons[q].family == fam && H.persons[q].died == ALIVE && (next < 0 || H.persons[q].born < H.persons[next].born)) next = q;
            h = next;
        }
        H.families[fam].head = h;
        return h;
    }
    void succeed(int y, int how) {
        const int prev = reign >= 0 ? H.reigns[reign].ruler : -1;
        const int s = seat(y);
        if (govNow == GOV_MACHINE) { newReign(-1, y, 8, -1, prev); return; }
        if (govNow == GOV_MONARCHY) {
            const int house = reign >= 0 ? H.reigns[reign].house : -1;
            int heir = house >= 0 && H.reigns[reign].government == GOV_MONARCHY ? heirOf(prev, y) : -1;
            if (heir >= 0) {
                const int r = newReign(heir, y, 1, house, prev);
                H.persons[heir].home = s;
                if (y - H.persons[heir].born < 16) {   // a child: the other parent, an uncle or aunt, or a lord of the realm rules for it
                    int regent = -1;
                    const int km = H.persons[heir].mother, kf = H.persons[heir].father;
                    for (int par : {km, kf}) if (par >= 0 && par != prev && H.persons[par].died == ALIVE) { regent = par; break; }
                    if (regent < 0) regent = figure(y, HR_REGENT, s, 30, 55, false);
                    if (H.persons[regent].role == HR_NONE) H.persons[regent].role = HR_REGENT;
                    H.reigns[r].regent = regent;
                    const int e = ev(HE_REGENCY, y, s, 0.4f); E(e).a = regent; E(e).b = heir; E(e).cause = lastAccessionEv;
                }
                return;
            }
            // no heir: a new house, chosen by the lords or by a war between two claimants
            if (prev >= 0 && rng.chance(0.4) && H.places.size() > 1) {
                const int a = figure(y, HR_RULER, s, 25, 50, false);
                const int other = pickPlace(y, [&](int p) { return p != s && big(p) ? 1.0 : 0.0; });
                const int b = figure(y, HR_REBEL, other >= 0 ? other : s, 25, 50, false);
                newHouse(y, a);
                newReign(a, y, 4, (int)H.families.size() - 1, prev);
                startWar(y, HW_SUCCESSION, -1, other >= 0 ? other : s, a, b);
                return;
            }
            const int a = figure(y, HR_RULER, s, 25, 50, false);
            const int f = newHouse(y, a);
            newReign(a, y, rng.chance(0.6) ? 2 : 4, f, prev);
            return;
        }
        int lo = 30, hi = 50, h = 2;
        switch (govNow) {
            case GOV_CHIEFS: lo = 28; hi = 50; h = 2; break;
            case GOV_COUNCIL: lo = 50; hi = 72; h = 2; break;
            case GOV_THEOCRACY: lo = 38; hi = 60; h = 2; break;
            case GOV_REPUBLIC: lo = 40; hi = 65; h = 3; break;
            case GOV_ASSEMBLY: lo = 35; hi = 65; h = 3; break;
            case GOV_CORPORATE: lo = 40; hi = 60; h = 5; break;
            default: break;
        }
        if (how == 9) h = 9;
        const int a = figure(y, HR_RULER, s, lo, hi, true);
        newReign(a, y, h, -1, prev);
    }
    // a ruler's term (the elected and appointed forms), 0 for life
    int termOf(int y) {
        const int a = ageAt(y);
        const int k = a == AGE_FAR_FUTURE ? 12 : (a == AGE_FUTURISTIC ? 2 : (a == AGE_PREHISTORIC ? 3 : 1));
        switch (govNow) {
            case GOV_REPUBLIC: return termRepublic * k;
            case GOV_ASSEMBLY: return termAssembly * k;
            case GOV_CORPORATE: return (5 + rng.irange(11)) * k;
            default: return 0;
        }
    }

    // ---- war
    int startWar(int y, int kind, int sideA, int sideB, int leaderA, int leaderB, int cause = -1, int length = 0) {
        HistoryWar w; w.kind = kind; w.start = y; w.sideA = sideA; w.sideB = sideB; w.leaderA = leaderA; w.leaderB = leaderB; w.dead = 0;
        w.name = mix64(H.seed ^ (0x40A5ULL + H.wars.size() * 0x9E3779B97F4A7C15ULL));
        const int a = ageAt(y);
        if (length <= 0) {
            static const double LEN[HW_COUNT] = {2, 6, 4, 4, 3, 5, 4, 10};
            length = std::max(1, (int)std::lround(std::exp(rng.range(0, std::log(LEN[kind] * (a >= AGE_MODERN ? 0.6 : 1.0) * 2)))));
        }
        w.end = std::min(H.last, y + length - 1);
        if (kind == HW_REBELLION) w.winner = rng.chance(0.68) ? 0 : 1;
        else if (kind == HW_REVOLUTION) w.winner = 1;
        else w.winner = rng.chance(0.15) ? -1 : rng.irange(2);
        H.wars.push_back(w);
        const int wi = (int)H.wars.size() - 1;
        const int target = sideB >= 0 ? sideB : (sideA >= 0 ? sideA : seat(y));
        const int e = ev(HE_WAR, y, target >= 0 && stands(target, y) ? target : -1, kind == HW_BANDS ? 0.45f : 0.7f);
        E(e).thing = wi; E(e).a = leaderA; E(e).b = leaderB; E(e).cause = cause;
        H.wars[wi].declared = e;
        WarRun run; run.war = wi; run.end = w.end; run.target = target;
        const int nb = std::max(1, std::min(8, 1 + (int)std::lround(rng.range(0, 0.6 * length))));
        for (int k = 0; k < nb; k++) run.battles.push_back(y + (int)std::floor((w.end - y + 1) * (k + rng.uni() * 0.9) / nb));
        std::sort(run.battles.begin(), run.battles.end());
        if (kind != HW_BANDS && rng.chance(0.45)) run.siegeAt = run.battles.back();
        warsOn.push_back(run);
        return wi;
    }
    int warLeader(int w, int side, int y) {
        HistoryWar& W2 = H.wars[w];
        int& L = side == 0 ? W2.leaderA : W2.leaderB;
        if (L >= 0 && H.persons[L].died == ALIVE && y - H.persons[L].born < 75) return L;
        if (side == 0 && W2.sideA == -1 && reign >= 0 && H.reigns[reign].ruler >= 0 && rng.chance(0.3)) return H.reigns[reign].ruler;
        const int home = side == 0 ? (W2.sideA >= 0 ? W2.sideA : seat(y)) : W2.sideB;
        L = figure(y, side == 0 ? HR_GENERAL : (W2.kind == HW_REBELLION || W2.kind == HW_REVOLUTION ? HR_REBEL : HR_GENERAL), home >= 0 && stands(home, y) ? home : seat(y), 25, 50);
        return L;
    }
    void stepWars(int y) {
        for (size_t k = 0; k < warsOn.size();) {
            WarRun& run = warsOn[k];
            HistoryWar& w = H.wars[run.war];
            const int a = ageAt(y);
            for (int by : run.battles) {
                if (by != y) continue;
                int at = run.target;
                if (at < 0 || !stands(at, y) || rng.chance(0.4)) {
                    const int near = w.sideB >= 0 && stands(w.sideB, y) ? w.sideB : seat(y);
                    at = pickPlace(y, [&](int p) { return near >= 0 && p != near ? 1.0 / (1.0 + distKm(near, p) / 60.0) : (p == near ? 0.6 : 0.0); });
                }
                const int side = w.winner >= 0 && rng.chance(0.72) ? w.winner : rng.irange(2);
                const int la = warLeader(run.war, side, y), lb = warLeader(run.war, 1 - side, y);
                static const double DEAD[AGE_COUNT][2] = {{1e-3, 1e-2}, {1e-4, 1.5e-3}, {5e-4, 4e-3}, {1e-3, 6e-3}, {1e-4, 1e-3}, {1e-5, 1e-4}};
                const double drawn = pop(y) * rng.range(DEAD[a][0], DEAD[a][1]) * (w.kind == HW_BANDS ? 0.3 : 1.0);
                const int64_t dead = std::max<int64_t>(3, (int64_t)std::min(drawn, BATTLE_CAP[a] * rng.range(0.15, 1.0)));
                const int e = ev(HE_BATTLE, y, at, 0.55f);
                E(e).thing = run.war; E(e).a = la; E(e).b = lb; E(e).out = side; E(e).value = dead; E(e).cause = w.declared;
                run.lastBattle = e;
                w.dead += dead; w.battles++;
                deficit += dead / pop(y);
                const double share = dead / std::max(1.0, pop(y) * 0.12);
                if (share > hzBattle) { hzBattle = share; hzBattleEv = e; }
                if (la >= 0 && H.persons[la].died == ALIVE && rng.chance(0.04)) die(la, y, 3, e, at);
                if (lb >= 0 && H.persons[lb].died == ALIVE && rng.chance(0.12)) die(lb, y, 3, e, at);
                if (run.siegeAt == y && at >= 0) {
                    const int s2 = ev(HE_SIEGE, y, at, 0.5f); E(s2).thing = run.war; E(s2).cause = e;
                    const bool fell = rng.chance(0.55);
                    E(s2).out = fell ? 1 : 0;
                    if (fell && at != seat(y) && rng.chance(0.55)) {
                        const int64_t sd = (int64_t)(placePop(at, y) * rng.range(0.03, 0.25));
                        const int sk = ev(HE_SACK, y, at, 0.55f); E(sk).thing = run.war; E(sk).value = sd; E(sk).cause = s2;
                        harm(at, sd / std::max(1.0, placePop(at, y)), sk, 3);
                        maybeLeave(at, y, sk, H.places[at].sclass == SC_CITY ? 0.06 : 0.16);
                    }
                }
            }
            if (y >= run.end) {
                const int e = ev(HE_PEACE, y, run.target >= 0 && stands(run.target, y) ? run.target : -1, 0.5f);
                E(e).thing = run.war; E(e).out = w.winner < 0 ? 2 : w.winner; E(e).cause = run.lastBattle >= 0 ? run.lastBattle : w.declared;
                const int la = w.leaderA >= 0 && H.persons[w.leaderA].died == ALIVE ? w.leaderA : -1, lb = w.leaderB >= 0 && H.persons[w.leaderB].died == ALIVE ? w.leaderB : -1;
                E(e).a = la; E(e).b = lb;
                w.peace = e; w.end = y;
                // a war of succession: the claimant who won rules
                if (w.kind == HW_SUCCESSION && w.winner == 1 && lb >= 0 && govNow == GOV_MONARCHY) {
                    endReign(y, 1, e);
                    const int prev = reign >= 0 ? H.reigns[reign].ruler : -1;
                    const int f = newHouse(y, lb);
                    newReign(lb, y, 6, f, prev);
                }
                // a rising that won deposes the ruler now and then
                if ((w.kind == HW_REBELLION || w.kind == HW_CIVIL) && w.winner == 1 && rng.chance(0.35) && reign >= 0 && H.reigns[reign].end == ALIVE && govNow != GOV_MACHINE) {
                    endReign(y, 1, e);
                    if (lb >= 0 && govNow == GOV_MONARCHY) { const int prev = H.reigns[reign].ruler; const int f = newHouse(y, lb); newReign(lb, y, 6, f, prev); }
                }
                warsOn.erase(warsOn.begin() + k);
                continue;
            }
            k++;
        }
    }

    // ---- harm: the share of a place's people (or the whole land's) an event kills this year, for the persons' deaths
    std::vector<int> hzTouched;
    void harm(int place, double share, int ev2, int how) {
        const int k = place + 1;
        if (k < 0 || k >= (int)hz.size()) return;
        if (hz[k] == 0 && share > 0) hzTouched.push_back(k);
        if (share > hz[k]) { hz[k] = share; hzEv[k] = ev2; hzHow[k] = how; }
    }
    void maybeLeave(int p, int y, int cause, double chance) {
        if (p < 0 || p == seat(y) || p == 0 || H.places[p].plan < 0) return;
        if (y >= H.profile.collapseYear - 2 || y - H.places[p].founded < 30) return;
        if (earlyLeft * 7 > (int)H.places.size()) return;
        if (!rng.chance(chance)) return;
        leave(p, y, HP_EARLY, cause, 0);
        earlyLeft++;
    }
    void leave(int p, int y, int fate, int cause, int why) {
        HistoryPlace& P = H.places[p];
        if (P.ended < y) return;
        P.ended = y; P.fate = fate;
        leftThisYear = y;
        const int e = ev(HE_ABANDONED, y, p, big(p) ? 0.6f : 0.3f);
        E(e).cause = cause; E(e).thing = why;
        P.endEvent = e;
        // its people move to the nearest place that stands on
        const int to = nearestStanding(p, y + 1, false);
        for (int q : stepped) {
            HistoryPerson& Q = H.persons[q];
            if (Q.died != ALIVE || Q.home != p) continue;
            Q.home = to;
            if (mode[q] == 2 && to >= 0 && y - Q.born >= 16) { const int m = ev(HE_MOVE, y, p, 0.15f); E(m).a = q; E(m).place2 = to; E(m).cause = e; }
        }
    }

    // ---- the build
    void setup();
    void setupPlaces();
    void setupCollapse();
    void setupPlanned();
    void oldTime();
    void yearLoop();
    void stepPersons(int y);
    void everyday(int y);                // H-04's second pass: the everyday's own stream (no harm, no person made)
    void family();                       // H-04's second pass: the families' own everyday, after the years (each person's stream)
    size_t evScan = 0; bool everContact = false, everMine = false;   // what the everyday has seen of the events so far
    void randomEvents(int y);
    void collapseYear(int y);
    void importJoint();
    void finish();
};
