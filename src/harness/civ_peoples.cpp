// E-03: the peoples and their histories (A-01, H-02, C-13): `peoples`, `history`, the ages' checks
#include "harness/harness.h"
#include "harness/test_worlds.h"
#include "core/text.h"

static std::string withThousands(int64_t n) {
    std::string s = std::to_string(n < 0 ? -n : n), o;
    for (size_t i = 0; i < s.size(); i++) { if (i > 0 && (s.size() - i) % 3 == 0) o += ','; o += s[i]; }
    return n < 0 ? "-" + o : o;
}

// H-02: the history. On the six age worlds and on the C-13 world of two (Eleinewai I, both its peoples): the same twice and in milliseconds; every
// event within the history's years (none after the end), its persons alive at it (a death in its year), its places the plan's (or the people's
// cleared ones) and standing at it; the collapse's stages in order with their signs in them, every place ended by the end and the last place at
// it; causes before their effects, a battle within its war, the kin coherent (a child born to living parents of age, a reign within its ruler's
// life, the reigns in turn); the two peoples' meetings, treaties and wars on the same years in both histories
void historyChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    struct World { Star st; StarSystem sys; int body = 0; BodyGen g; int which = 0; bool ok = false; std::string name; };
    std::vector<World> W;
    for (int a = 0; a < AGE_COUNT; a++) {
        World w;
        if (!starInSector(AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz, w.st, true)) continue;
        w.sys.generate(w.st);
        if (AGE_WORLDS[a].body >= (int)w.sys.bodies.size()) continue;
        w.body = AGE_WORLDS[a].body; w.g = BodyGen::make(w.sys.bodies[w.body]); w.which = AGE_WORLDS[a].which; w.ok = worldHadCivilisation(w.g); w.name = w.sys.bodies[w.body].name;
        W.push_back(w);
    }
    // the worlds of two: Leanmya V-a (19.4 ly, the nearest whose peoples met: two treaties, nine wars) and Hiameshseine I-a (37.7 ly, the
    // nearest with a conquest). R-413: C-13's Eleinewai I and Meareafionris III, their peoples of one age since, never met
    const struct { int64_t sx, sz; int body; } TWO[2] = {{179, 57, 18}, {202, 9, 5}};
    for (const auto& tw : TWO)
        for (int k = 0; k < 2; k++) {
            World w;
            if (!starInSector(tw.sx, 0, tw.sz, w.st, true)) continue;
            w.sys.generate(w.st);
            if (tw.body >= (int)w.sys.bodies.size()) continue;
            w.body = tw.body; w.g = BodyGen::make(w.sys.bodies[tw.body]); w.which = k; w.ok = worldHadCivilisation(w.g) && peoplesOf(w.g) > 1;
            w.name = w.sys.bodies[tw.body].name + (k ? " (second)" : "");
            W.push_back(w);
        }
    std::vector<std::shared_ptr<History>> HS(W.size());
    {   // the same twice; the time a history takes (its world's plan built before)
        bool same = true; double worst = 0; std::string times; int worlds = 0;
        for (size_t i = 0; i < W.size(); i++) {
            if (!W[i].ok) continue;
            worlds++;
            worldPlanOf(W[i].g);
            std::shared_ptr<History> A = buildHistory(W[i].sys, W[i].sys.bodies[W[i].body], W[i].g, W[i].which);
            std::shared_ptr<History> B = buildHistory(W[i].sys, W[i].sys.bodies[W[i].body], W[i].g, W[i].which);
            HS[i] = A;
            worst = std::max(worst, std::min(A->buildMs, B->buildMs));
            times += fmt("%s%.1f", times.empty() ? "" : "/", std::min(A->buildMs, B->buildMs));
            if (A->events.size() != B->events.size() || A->persons.size() != B->persons.size() || A->places.size() != B->places.size() || A->reigns.size() != B->reigns.size()) { same = false; continue; }
            for (size_t e = 0; e < A->events.size(); e++) {
                const HistoryEvent &x = A->events[e], &y = B->events[e];
                if (x.kind != y.kind || x.year != y.year || x.season != y.season || x.place != y.place || x.a != y.a || x.b != y.b || x.cause != y.cause || x.thing != y.thing || x.value != y.value) same = false;
            }
            for (size_t p = 0; p < A->persons.size(); p++) if (A->persons[p].name != B->persons[p].name || A->persons[p].born != B->persons[p].born || A->persons[p].died != B->persons[p].died) same = false;
        }
        check("H-02: a people's history the same twice, in milliseconds", same && worlds >= 10 && worst < 30,
              fmt("%d histories (the six ages, two worlds of two) %s; built in %s ms (the world's plan before it)", worlds, same ? "the same twice" : "NOT the same twice", times.c_str()));
    }
    {   // the years, the persons alive, the places on the plan and standing
        int events = 0, outside = 0, deadAt = 0, notPlan = 0, notStanding = 0, persons = 0; std::string bad;
        for (size_t i = 0; i < W.size(); i++) {
            if (!HS[i]) continue;
            const History& H = *HS[i];
            std::shared_ptr<const WorldPlan> P = worldPlanOf(W[i].g);
            const PeoplePlan& pp = P->plans[std::min(H.people, P->peoples - 1)];
            int onPlan = 0;
            for (size_t p = 0; p < H.places.size(); p++) {
                const HistoryPlace& hp = H.places[p];
                if (hp.plan >= 0) {
                    onPlan++;
                    if (hp.plan >= (int)pp.places.size() || pp.places[hp.plan].lat != hp.lat || pp.places[hp.plan].lon != hp.lon) { notPlan++; if (bad.size() < 200) bad += fmt("; %s place %zu not its plan's", W[i].name.c_str(), p); }
                } else if (hp.fate != HP_CLEARED || hp.ended >= H.profile.collapseYear || H.age < AGE_FUTURISTIC) { notPlan++; if (bad.size() < 200) bad += fmt("; %s place %zu off the plan and not cleared", W[i].name.c_str(), p); }
            }
            if (onPlan != (int)pp.places.size()) { notPlan++; bad += fmt("; %s holds %d of the plan's %zu places", W[i].name.c_str(), onPlan, pp.places.size()); }
            persons += (int)H.persons.size();
            for (size_t e = 0; e < H.events.size(); e++) {
                const HistoryEvent& ev = H.events[e];
                events++;
                if (ev.year < H.first || ev.year > H.last) { outside++; if (bad.size() < 200) bad += fmt("; %s event %zu (%s) in year %d", W[i].name.c_str(), e, HISTORY_EVENT_NAMES[ev.kind], ev.year); }
                const int slots[3] = {ev.a, ev.b, ev.c};
                for (int k = 0; k < 3; k++) {
                    const int q = slots[k];
                    if (q < 0) continue;
                    if (ev.kind == HE_ACCESSION && k == 1) continue;   // the predecessor, the regent: named after they may have gone
                    if (q >= (int)H.persons.size() || ev.year < H.persons[q].born || ev.year > H.persons[q].died || (ev.kind == HE_DEATH && k == 0 && H.persons[q].died != ev.year)) {
                        deadAt++;
                        if (bad.size() < 300) bad += fmt("; %s %s in %d names %s (%d-%d)", W[i].name.c_str(), HISTORY_EVENT_NAMES[ev.kind], ev.year, historyPersonName(H, q).c_str(), q < (int)H.persons.size() ? H.persons[q].born : 0, q < (int)H.persons.size() ? H.persons[q].died : 0);
                    }
                }
                for (int pl : {ev.place, ev.place2}) {
                    if (pl < 0) continue;
                    if (pl >= (int)H.places.size() || !H.placeStands(pl, ev.year)) {
                        notStanding++;
                        if (bad.size() < 400) bad += fmt("; %s %s in %d at %s (%d-%d)", W[i].name.c_str(), HISTORY_EVENT_NAMES[ev.kind], ev.year, historyPlaceName(H, pl).c_str(), pl < (int)H.places.size() ? H.places[pl].founded : 0, pl < (int)H.places.size() ? H.places[pl].ended : 0);
                    }
                }
            }
        }
        check("H-02: every event in the history's years, its figures alive at it, its places the plan's and standing", outside == 0 && deadAt == 0 && notPlan == 0 && notStanding == 0,
              fmt("%d events, %d persons: %d outside the years, %d naming someone not alive, %d places not the plan's, %d at a place not standing%s", events, persons, outside, deadAt, notPlan, notStanding, bad.c_str()));
    }
    {   // the collapse: its stages in order with their events and signs in them; every place ended by the end, the last at it; everyone gone by then
        int bad = 0; std::string why, sample;
        for (size_t i = 0; i < W.size(); i++) {
            if (!HS[i]) continue;
            const History& H = *HS[i];
            const PeopleProfile& P = H.profile;
            auto fail = [&](const std::string& s) { bad++; if (why.size() < 300) why += "; " + W[i].name + ": " + s; };
            if (H.cause != P.cause) fail("the cause is not the profile's");
            for (int s = 0; s < STAGE_COUNT; s++) {
                if (s > 0 && H.stageYear[s] < H.stageYear[s - 1]) fail(fmt("stage %d before stage %d", s, s - 1));
                if (H.stageYear[s] > H.last) fail(fmt("stage %d after the end", s));
                if (P.stageAt(H.stageYear[s]) < s) fail(fmt("stage %d's year not in it", s));
                bool marked = false;
                for (const HistoryEvent& e : H.events) if (e.kind == HE_STAGE && e.out == s && e.year == H.stageYear[s]) marked = true;
                if (!marked) fail(fmt("stage %d not marked", s));
            }
            for (const HistoryEvent& e : H.events) {
                if (e.kind == HE_SIGN && P.stageAt(e.year) < e.out) fail(fmt("a sign of stage %d in %d", e.out, e.year));
                if (e.kind == HE_FOUNDING && e.year >= P.collapseYear) fail(fmt("a founding in the collapse (%d)", e.year));
            }
            int lastPlaces = 0;
            for (const HistoryPlace& p : H.places) { if (p.ended > H.last || p.ended < p.founded) fail("a place's years wrong"); if (p.fate == HP_LAST) { lastPlaces++; if (p.ended != H.last) fail("the last place ended before the end"); } }
            if (!H.places.empty() && (lastPlaces != 1 || H.lastPlace < 0 || H.places[H.lastPlace].fate != HP_LAST)) fail(fmt("%d last places", lastPlaces));
            for (const HistoryPerson& q : H.persons) if (q.died > H.last) { fail("someone outlived the end"); break; }
            bool ended = false; for (const HistoryEvent& e : H.events) if (e.kind == HE_END && e.year == H.last && e.place == H.lastPlace && e.a == H.lastPerson) ended = true;
            if (!ended) fail("no end at the last place");
            if (i < 6 && sample.size() < 400) sample += fmt("%s%s %s %d-%d (%s, %s, %s, %s), the last %s at %s", sample.empty() ? "" : "; ", W[i].name.c_str(), CAUSE_NAMES[H.cause], H.stageYear[0], H.last,
                                                      withThousands(H.stageYear[0]).c_str(), withThousands(H.stageYear[1]).c_str(), withThousands(H.stageYear[2]).c_str(), withThousands(H.stageYear[3]).c_str(),
                                                      historyPersonName(H, H.lastPerson).c_str(), historyPlaceName(H, H.lastPlace).c_str());
        }
        check("H-02: the collapse's stages in order to the end, every place left by it, the last place at it", bad == 0, (bad ? fmt("%d wrong", bad) + why : sample));
    }
    {   // causes before effects, battles within their wars, the kin and the reigns coherent
        int causes = 0, wars = 0, kin = 0, reigns = 0, checked = 0; std::string why;
        for (size_t i = 0; i < W.size(); i++) {
            if (!HS[i]) continue;
            const History& H = *HS[i];
            for (size_t e = 0; e < H.events.size(); e++) {
                const HistoryEvent& ev = H.events[e];
                if (ev.cause >= 0) {
                    checked++;
                    if (ev.cause >= (int)e || H.events[ev.cause].year > ev.year) { causes++; if (why.size() < 300) why += fmt("; %s %s in %d caused by %s in %d", W[i].name.c_str(), HISTORY_EVENT_NAMES[ev.kind], ev.year, HISTORY_EVENT_NAMES[H.events[ev.cause].kind], H.events[ev.cause].year); }
                }
                if (ev.kind == HE_BATTLE || ev.kind == HE_SIEGE || ev.kind == HE_SACK || ev.kind == HE_PEACE) {
                    const HistoryWar& w = H.wars[ev.thing];
                    if (ev.year < w.start || ev.year > w.end || (w.declared >= 0 && H.events[w.declared].year > ev.year)) { wars++; if (why.size() < 300) why += fmt("; %s %s in %d outside its war %d-%d", W[i].name.c_str(), HISTORY_EVENT_NAMES[ev.kind], ev.year, w.start, w.end); }
                }
                if (ev.kind == HE_FAMINE && ev.cause >= 0 && H.events[ev.cause].kind == HE_HARVEST && H.events[ev.cause].year != ev.year) causes++;
                if (ev.kind == HE_MARRIAGE && (!H.alive(ev.a, ev.year) || !H.alive(ev.b, ev.year))) kin++;
            }
            for (size_t w = 0; w < H.wars.size(); w++) if (H.wars[w].peace >= 0 && H.events[H.wars[w].peace].year != H.wars[w].end) { wars++; if (why.size() < 300) why += fmt("; %s war %zu's peace off its end", W[i].name.c_str(), w); }
            for (size_t p = 0; p < H.persons.size(); p++) {
                const HistoryPerson& q = H.persons[p];
                if (q.died < q.born) kin++;
                for (int par : {q.mother, q.father}) {
                    if (par < 0) continue;
                    const HistoryPerson& r = H.persons[par];
                    if (q.born < r.born + 12 || q.born > r.died + (par == q.father ? 1 : 0)) { kin++; if (why.size() < 300) why += fmt("; %s %s born %d to %s (%d-%d)", W[i].name.c_str(), historyPersonName(H, (int)p).c_str(), q.born, historyPersonName(H, par).c_str(), r.born, r.died); }
                }
            }
            for (size_t k = 0; k < H.reigns.size(); k++) {
                const HistoryReign& R = H.reigns[k];
                if (R.end < R.start || (k > 0 && R.start < H.reigns[k - 1].end)) { reigns++; if (why.size() < 300) why += fmt("; %s reign %zu (%d-%d) after %d", W[i].name.c_str(), k, R.start, R.end, k ? H.reigns[k - 1].end : 0); }
                if (R.ruler >= 0 && (R.start < H.persons[R.ruler].born || R.end > H.persons[R.ruler].died)) { reigns++; if (why.size() < 300) why += fmt("; %s reign %zu outlives its ruler", W[i].name.c_str(), k); }
            }
        }
        check("H-02: causes before their effects, battles within their wars, the kin and the reigns coherent", causes == 0 && wars == 0 && kin == 0 && reigns == 0,
              fmt("%d causes: %d after their effects; %d out of their wars; %d kin wrong; %d reigns wrong%s", checked, causes, wars, kin, reigns, why.c_str()));
    }
    {   // the worlds of two: their meetings, treaties, wars and the conquest on the same years in both histories
        bool ok = true; std::string why; int conquests = 0, pairsN = 0;
        for (size_t i = 0; i + 1 < W.size(); i++) {
        if (!HS[i] || !HS[i + 1] || W[i].which != 0 || W[i + 1].which != 1 || W[i].g.seed != W[i + 1].g.seed) continue;
        const History* A = HS[i].get(); const History* B = HS[i + 1].get();
        pairsN++;
        for (const HistoryWar& w : A->wars) if (w.kind == HW_CONQUEST && w.sideB == -2) conquests++;
        for (const HistoryWar& w : B->wars) if (w.kind == HW_CONQUEST && w.sideB == -2) conquests++;
        int matched = 0, joint = 0;
        {
            auto agoOfH = [](const History& H, int y) { return (int64_t)std::llround(H.profile.endedYearsAgo) + (H.last - y); };
            auto isJoint = [](const History& H, const HistoryEvent& e) {
                if (e.kind == HE_CONTACT || e.kind == HE_TREATY || e.kind == HE_OTHER_END) return true;
                if (e.kind == HE_WAR || e.kind == HE_BATTLE || e.kind == HE_PEACE) return H.wars[e.thing].sideB == -2;
                return false;
            };
            for (const HistoryEvent& e : A->events) {
                if (!isJoint(*A, e) || e.kind == HE_OTHER_END) continue;
                joint++;
                const int64_t ago = agoOfH(*A, e.year);
                bool found = false;
                for (const HistoryEvent& f : B->events) if (f.kind == e.kind && isJoint(*B, f) && agoOfH(*B, f.year) == ago) { found = true; break; }
                if (found) matched++; else if (why.size() < 200) why += fmt("; %s in %d not in the other's", HISTORY_EVENT_NAMES[e.kind], e.year);
            }
            int jointB = 0; for (const HistoryEvent& f : B->events) if (isJoint(*B, f) && f.kind != HE_OTHER_END) jointB++;
            if (!(joint > 0 && matched == joint && jointB == joint)) ok = false;
            why += fmt("%s%s: %d of the first people's %d shared events found in the second's (%d there)", why.empty() ? "" : "; ", W[i].name.c_str(), matched, joint, jointB);
        }
        }
        if (pairsN < 2 || conquests < 2) ok = false;
        check("H-02: two peoples' meetings, treaties, wars and a conquest the same in both histories", ok, why + fmt("; %d conquest wars (one in each history of the conquest)", conquests));
    }
}



// A-03: what each age builds. The decay by material (each material's years, C-08's masonry to the digit, the rain's share); every age's
// settlements of every class on one world built of its own kinds to its heights with nothing resting on air; the plan's examples (a wooden
// village after a thousand years is rings of post holes, a concrete tower after three thousand a stump, a far-future spire after a hundred
// thousand still sharp); every room's doorway open and its spots clear
void agesChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    {   // the decay by material
        bool mono = true, wetFaster = true, masonryC08 = true;
        const double YEARS[] = {10, 30, 100, 300, 1000, 3000, 10000, 30000, 100000, 1e6};
        for (int m = 0; m < MT_COUNT; m++) {
            for (int k = 1; k < 10; k++) if (materialDecay(m, YEARS[k], 0.5) < materialDecay(m, YEARS[k - 1], 0.5) - 1e-12) mono = false;
            if (m != MT_GLASS && m != MT_ETERNAL && !(materialDecay(m, 2000, 0.9) > materialDecay(m, 2000, 0.1) || materialDecay(m, 2000, 0.1) >= 1.0)) wetFaster = false;
        }
        for (double y : {200.0, 2000.0, 20000.0}) for (double w : {0.1, 0.5, 0.9}) { double d = decayOf(y, w); if (d > 0.05 && d < 0.95 && std::fabs(materialDecay(MT_MASONRY, y, w) - d) > 1e-9) masonryC08 = false; }
        auto D = [](int m, double y, double w = 0.5) { return materialDecay(m, y, w); };
        bool timber = D(MT_TIMBER, 30) < 0.3 && D(MT_TIMBER, 1000) > 0.99 && D(MT_TIMBER, 1000, 0.1) > 0.95;
        bool earth = D(MT_EARTH, 1000, 0.1) < 0.5 && D(MT_EARTH, 1000, 0.9) > 0.8;
        bool drystone = D(MT_DRYSTONE, 1000) < 0.15 && D(MT_DRYSTONE, 50000) > 0.5;
        bool iron = D(MT_IRON, 1000) > 0.5 && D(MT_IRON, 50) < 0.3;
        bool concrete = D(MT_CONCRETE, 100) > 0.05 && D(MT_CONCRETE, 100) < 0.3 && D(MT_CONCRETE, 3000) > 0.65 && D(MT_MASS, 3000) < 0.4;
        bool glass = D(MT_GLASS, 30) > 0.95;
        bool composite = D(MT_COMPOSITE, 1000) < 0.1 && D(MT_COMPOSITE, 100000) > 0.85;
        bool eternal = D(MT_ETERNAL, 1e6) == 0 && D(MT_ETERNAL, 1e6, 1.0) == 0;
        std::string table;
        for (int m = 0; m < MT_COUNT; m++) table += fmt("%s%s %.2f/%.2f/%.2f/%.2f", m ? "; " : "", BUILT_MATERIAL_NAMES[m], D(m, 100), D(m, 1000), D(m, 10000), D(m, 100000));
        check("the decay by material (A-03)", mono && wetFaster && masonryC08 && timber && earth && drystone && iron && concrete && glass && composite && eternal,
              fmt("%s%s%s%s%s%s%s%s%s%s%s; at 100/1,000/10,000/100,000 years: %s", mono ? "" : "NOT monotonic; ", wetFaster ? "" : "rain NOT faster; ", masonryC08 ? "masonry is C-08's" : "masonry NOT C-08's",
                  timber ? "" : "; timber wrong", earth ? "" : "; earth wrong", drystone ? "" : "; dry stone wrong", iron ? "" : "; iron wrong", concrete ? "" : "; concrete wrong", glass ? "" : "; glass wrong", composite ? "" : "; composite wrong", eternal ? "" : "; the far future's weathers",
                  table.c_str()));
    }
    // every age's settlements on one world (the medieval test world's desert): six cells of each class at three hundred years
    Star st; StarSystem sys; BodyGen g; bool world = false;
    if (starInSector(AGE_WORLDS[AGE_MEDIEVAL].sx, 0, AGE_WORLDS[AGE_MEDIEVAL].sz, st, true)) { sys.generate(st); if (AGE_WORLDS[AGE_MEDIEVAL].body < (int)sys.bodies.size()) { g = BodyGen::make(sys.bodies[AGE_WORLDS[AGE_MEDIEVAL].body]); world = true; } }
    if (!world) { check("what each age builds (A-03)", false, "no test world"); return; }
    int gLat0, gLon0; ruinCellOf(g, 0.3, 1.1, gLat0, gLon0);
    {
        int settlements = 0, buildingsN = 0, later = 0, floating = 0, rooms = 0, doorsShut = 0, spotsBad = 0; std::string bad, kindsLine, floatBad;
        double tallest[AGE_COUNT] = {0}; std::set<int> own[AGE_COUNT];
        std::vector<FormDisc> discs;
        for (int a = 0; a < AGE_COUNT; a++) {
            Culture c = cultureAs(g, 0, a, 300);
            for (int cls = 0; cls < 4; cls++) {
                if (a == AGE_FAR_FUTURE && cls != SC_MONUMENT) continue;
                for (int k = 0; k < 6; k++) {
                    RuinSpec sp; settlementOf(g, gLat0 + k * 3, gLon0 + cls * 5, c, cls, sp);
                    std::vector<Form> el; ruinElements(sp, c, 0, el);
                    settlements++; buildingsN += (int)sp.buildings.size();
                    for (const Building& b : sp.buildings) {
                        if (buildingAge(b.kind) > a) { later++; if (bad.size() < 200) bad += fmt("a %s in a %s settlement; ", BUILDING_KIND_NAMES[b.kind], AGE_NAMES[a]); }
                        if (buildingAge(b.kind) == a) own[a].insert(b.kind);
                    }
                    for (size_t i = 0; i < el.size(); i++) {
                        tallest[a] = std::max(tallest[a], formTop(el[i]));
                        if (!formRests(el, i)) { floating++; if (floatBad.size() < 600) floatBad += fmt("a %s %s (part %d, %.1fx%.1f) of a %s %s at %.1f m on nothing; ", FORM_KIND_NAMES[el[i].kind], el[i].material >= 0 ? BUILT_MATERIAL_NAMES[el[i].material] : "sand", el[i].part, el[i].hx, el[i].hz, AGE_NAMES[a], BUILDING_KIND_NAMES[sp.buildings[el[i].building].kind], el[i].y0); }
                    }
                    for (size_t bi = 0; bi < sp.buildings.size(); bi++) {   // the rooms: the doorway open (a run of a metre along its wall's line clear of the pieces' colliders and the explorer's
                        const Building& b = sp.buildings[bi]; BuildingRoom R;   // radius), the spots clear of them
                        if (!buildingRoom(b, R)) continue;
                        rooms++;
                        discs.clear(); for (const Form& e : el) if (e.building == (int)bi) formColliders(e, discs);
                        auto blockedAt = [&](double x, double z, double margin) { for (const FormDisc& q : discs) if (q.y0 < 1.0 && q.y1 > 0.6 && std::hypot(q.x - x, q.z - z) < q.r + margin) return true; return false; };
                        if (R.door) {
                            int run = 0, best = 0;
                            for (double t = -R.doorL; t <= R.doorL; t += 0.1) { run = blockedAt(R.doorX + R.doorAx * t, R.doorZ + R.doorAz * t, 0.35) ? 0 : run + 1; best = std::max(best, run); }
                            if (best * 0.1 < 1.0) {
                                doorsShut++;
                                if (bad.size() < 1400) {   // what shuts it: the first form whose collider stands in the doorway's middle
                                    std::string by;
                                    for (const Form& e : el) { if (e.building != (int)bi) continue; std::vector<FormDisc> d1; formColliders(e, d1); for (const FormDisc& q : d1) if (q.y0 < 1.0 && q.y1 > 0.6 && std::hypot(q.x - R.doorX, q.z - R.doorZ) < q.r + 0.35) { by = fmt("%s part %d %.1fx%.1f", FORM_KIND_NAMES[e.kind], e.part, e.hx, e.hz); break; } if (!by.empty()) break; }
                                    bad += fmt("a %s %s (form %d, door %d, found %d, profile %d) shut by %s; ", AGE_NAMES[a], BUILDING_KIND_NAMES[b.kind], b.form, b.door, b.found, c.genome.of(GV_PROFILE), by.c_str());
                                }
                            }
                        }
                        int clearSpots = 0;
                        for (const auto& p : R.spots) { double x = b.x + std::cos(b.heading) * p.first + std::sin(b.heading) * p.second, z = b.z - std::sin(b.heading) * p.first + std::cos(b.heading) * p.second; if (!blockedAt(x, z, 0.42)) clearSpots++; }
                        if (clearSpots == 0 && !R.spots.empty()) { spotsBad++; if (bad.size() < 700) bad += fmt("a %s without a clear spot; ", BUILDING_KIND_NAMES[b.kind]); }
                    }
                }
            }
        }
        const double LO[AGE_COUNT] = {2, 25, 40, 100, 300, 150}, HI[AGE_COUNT] = {25, 120, 130, 800, 2500, 4000};   // a lofty, ambitious people's towers to 650 m, their masts over them
        bool heights = true, kinds = true;
        for (int a = 0; a < AGE_COUNT; a++) {
            if (tallest[a] < LO[a] || tallest[a] > HI[a]) heights = false;
            if ((int)own[a].size() < (a == AGE_FAR_FUTURE ? 1 : 3)) kinds = false;
            kindsLine += fmt("%s%s %zu kinds, tallest %.0f m", a ? "; " : "", AGE_NAMES[a], own[a].size(), tallest[a]);
        }
        check("what each age builds: its own kinds, its heights (A-03)", later == 0 && heights && kinds,
              fmt("%d settlements, %d buildings: %s%s%s", settlements, buildingsN, kindsLine.c_str(), later ? fmt("; %d of a later age: ", later).c_str() : "", later ? bad.substr(0, 200).c_str() : ""));
        check("every age's pieces rest on something, its rooms open (A-03)", floating == 0 && doorsShut == 0 && spotsBad == 0 && rooms > 0,
              fmt("%d rooms; %d pieces on nothing, %d doorways shut, %d rooms without a clear spot%s%s%s", rooms, floating, doorsShut, spotsBad, bad.empty() && floatBad.empty() ? "" : ": ", floatBad.c_str(), bad.substr(0, 1400).c_str()));
    }
    {   // the plan's examples
        // a wooden village after a thousand years: rings of post holes (no timber standing over a step, eight holes or more a hut)
        Culture cv = cultureAs(g, 0, AGE_PREHISTORIC, 1000);   // a people of round huts apart, built neither light nor to last
        cv.genome.set(GV_PLAN, PF_CIRCLE); cv.genome.set(GV_GROUPING, GR_DETACHED); cv.genome.v[GV_LASTING] = 0.5; cv.genome.v[GV_SAMENESS] = 0.1; cultureWays(cv, peopleGen(g, 0).seed); cv.hut = 0;
        int huts = 0, ringed = 0, standing = 0;
        for (int k = 0; k < 6; k++) {
            RuinSpec sp; settlementOf(g, gLat0 + 40 + k, gLon0, cv, SC_VILLAGE, sp);
            std::vector<Form> el; ruinElements(sp, cv, 0, el);
            for (const Form& e : el) if (e.material == MT_TIMBER && formTop(e) - std::min(0.0, e.y0) > 0.3) standing++;
            for (size_t bi = 0; bi < sp.buildings.size(); bi++) {
                if (sp.buildings[bi].kind != BK_HUT) continue;
                huts++; int holes = 0;
                for (const Form& e : el) if (e.building == (int)bi && e.material == MT_TIMBER && e.y1 < 0.06) holes++;
                if (holes >= 8) ringed++;
            }
        }
        // a concrete tower after three thousand years: a stump (what stands of its concrete under a fifth of its height; what fell lies)
        Culture cm = cultureAs(g, 0, AGE_MODERN, 3000);
        cm.genome.v[GV_LASTING] = 0.5; cultureWays(cm, peopleGen(g, 0).seed);   // the material's own years (a people built to last outlasts them)
        int towers = 0, stumps = 0; double worst = 0;
        for (int k = 0; k < 8; k++) {
            RuinSpec sp; settlementOf(g, gLat0 + 50 + k, gLon0, cm, SC_TOWN, sp);
            std::vector<Form> el; ruinElements(sp, cm, 0, el);
            for (size_t bi = 0; bi < sp.buildings.size(); bi++) {
                const Building& b = sp.buildings[bi];
                if (b.kind != BK_HIGHRISE) continue;
                towers++; double top = 0;
                for (const Form& e : el) if (e.building == (int)bi && e.material == MT_CONCRETE && e.tilt < 0.3 && e.part != 3) top = std::max(top, formTop(e));
                worst = std::max(worst, top / b.hWall);
                if (top <= 0.2 * b.hWall) stumps++;
            }
        }
        // a far-future spire after a hundred thousand years: still sharp (its pieces those of its first year, none broken)
        Culture c0 = cultureAs(g, 0, AGE_FAR_FUTURE, 1), c1 = cultureAs(g, 0, AGE_FAR_FUTURE, 100000);
        c0.great = c1.great = 0;   // the spike
        RuinSpec s0, s1; settlementOf(g, gLat0, gLon0 + 40, c0, SC_MONUMENT, s0); settlementOf(g, gLat0, gLon0 + 40, c1, SC_MONUMENT, s1);
        std::vector<Form> e0, e1; ruinElements(s0, c0, 0, e0); ruinElements(s1, c1, 0, e1);
        bool sharp = !e0.empty() && e0.size() == e1.size() && s0.buildings.size() == 1 && s0.buildings[0].kind == BK_SPIKE;
        for (size_t i = 0; sharp && i < e0.size(); i++) if (e0[i].broken != 0 || e1[i].broken != 0 || e0[i].y1 != e1[i].y1 || e0[i].hx != e1[i].hx || e0[i].tilt != e1[i].tilt) sharp = false;
        check("the decay's examples: post holes, a stump, a spire still sharp (A-03)", huts > 0 && ringed == huts && standing == 0 && towers > 0 && stumps == towers && sharp,
              fmt("a wooden village after 1,000 years: %d huts, %d in rings of post holes, %d timber pieces standing; a concrete tower after 3,000: %d towers, %d stumps (the tallest standing %.0f%% of its height); a far-future spike after 100,000: %s",
                  huts, ringed, standing, towers, stumps, 100 * worst, sharp ? "as sharp as in its first year" : "NOT as it stood"));
    }
}

// H-02: a people's history as Dwarf Fortress's legends would print it: the head (the span, the height, the end and what was told), the eras, the
// rulers by house, the places (the capital and the cities each, the towns by the age they were founded in, the ones left early and the ones
// cleared), the wars, the sicknesses, the inventions and the institutions, the figures, the families followed, and the chronicle (`full`: every
// public event by year) and the collapse to the last person
static void printLegends(const StarSystem& sys, const Body& b, const History& H, FILE* f, bool full) {
    auto out = [&](const std::string& s) { fputs(s.c_str(), f); fputc('\n', f); };
    const PeopleProfile& P = H.profile;
    auto yr = [&](int y) { return withThousands(y); };
    auto line = [&](int i) {
        const HistoryEvent& e = H.events[i];
        const std::string se = historySeason(H, e.season);
        return fmt("  %s%s: %s.", historyYear(H, e.year).c_str(), se.empty() ? "" : (", " + se).c_str(), historyEventLine(H, i).c_str());
    };
    int cities = 0, towns = 0, lost = 0, early = 0, privN = 0;
    for (const HistoryPlace& p : H.places) {
        if (p.plan < 0) lost++;
        else if (p.sclass == SC_CITY || (H.age == AGE_FUTURISTIC && p.sclass == SC_TOWN)) cities++;
        else if (p.sclass != SC_CAPITAL) towns++;
        if (p.fate == HP_EARLY) early++;
    }
    for (const HistoryEvent& e : H.events) if (!e.pub) privN++;
    out(fmt("THE %s OF %s", upper(H.lore.people).c_str(), upper(b.name).c_str()));
    const bool pre = H.age == AGE_PREHISTORIC;
    out(fmt("%s %s people of a %s world. Their count runs %s years from %s %s to their end, %s years ago%s.", H.age == AGE_INDUSTRIAL ? "An" : "A", AGE_NAMES[H.age], lowerOf(KINDS[b.type].name).c_str(),
            withThousands(H.last).c_str(), pre ? "the first winter at" : "the founding of", H.lore.city.c_str(),
            withThousands((int64_t)std::llround(P.endedYearsAgo)).c_str(), H.first < 1 ? fmt("; the old time before it, %s years as they remembered it", withThousands(1 - H.first).c_str()).c_str() : ""));
    out(fmt("At their height %s on %.0f%% of the land: %zu places (%d %s, %d %s%s); ended by %s over %s years (the warning in %s, the onset %s, the crisis %s, the end %s).",
            peopleCount(P.peak).c_str(), 100 * P.held, H.places.size() - lost, cities, H.age == AGE_FUTURISTIC ? "great places" : "cities", towns, pre ? "seats" : (H.age == AGE_FUTURISTIC ? "lesser places" : (H.age == AGE_FAR_FUTURE ? "works" : "towns")),
            lost ? fmt(", and %d older cities it cleared", lost).c_str() : "", CAUSE_NAMES[H.cause], withThousands(P.collapseYears).c_str(),
            yr(H.stageYear[0]).c_str(), yr(H.stageYear[1]).c_str(), yr(H.stageYear[2]).c_str(), yr(H.stageYear[3]).c_str()));
    out(fmt("Told: %s events (%s in the chronicle, %s of the families), %s persons, %zu families (followed from year %s), %zu reigns, %zu wars, %zu sicknesses, %zu inventions, %zu institutions; %d places left early. Built in %.1f ms.",
            withThousands(H.events.size()).c_str(), withThousands(H.publicEvents).c_str(), withThousands(privN).c_str(), withThousands(H.persons.size()).c_str(), H.families.size(), yr(H.famFrom).c_str(),
            H.reigns.size(), H.wars.size(), H.sicknesses.size(), H.inventions.size(), H.institutions.size(), early, H.buildMs));
    {   // the chronicle by kind
        int byKind[HE_COUNT] = {0};
        for (const HistoryEvent& e : H.events) if (e.pub) byKind[e.kind]++;
        std::string k;
        for (int i = 0; i < HE_COUNT; i++) if (byKind[i]) k += fmt("%s%s %d", k.empty() ? "" : ", ", HISTORY_EVENT_NAMES[i], byKind[i]);
        out("The chronicle by kind: " + k + ".");
    }
    // the eras
    out("");
    out("THE ERAS");
    for (const HistoryEra& e : H.eras) {
        std::string gov = GOVERNMENT_NAMES[e.govFrom];
        if (e.govChange > 0) gov += fmt(", then %s from %s", GOVERNMENT_NAMES[e.government], yr(e.govChange).c_str());
        int reigns = 0, wars = 0, founded = 0;
        for (const HistoryReign& R : H.reigns) if (R.start >= e.start && R.start <= e.end) reigns++;
        for (const HistoryWar& w : H.wars) if (w.start >= e.start && w.start <= e.end) wars++;
        for (const HistoryPlace& p : H.places) if (p.founded >= e.start && p.founded <= e.end) founded++;
        std::string inv;
        for (const HistoryInvention& I : H.inventions) if (I.year >= e.start && I.year <= e.end) inv += (inv.empty() ? "" : ", ") + historyInventionName(I.id);
        const bool oldTime = e.age == AGE_PREHISTORIC && H.age != AGE_PREHISTORIC;
        const bool lastEra = &e == &H.eras.back();
        out(fmt("  %s (%s), %s to %s: %s; the faith of %s; %s%s.", e.name.c_str(), AGE_NAMES[e.age], oldTime ? fmt("about %s years before the first", withThousands(1 - e.start).c_str()).c_str() : fmt("year %s", yr(e.start).c_str()).c_str(),
                oldTime ? "year 0" : fmt("year %s", yr(e.end).c_str()).c_str(), gov.c_str(), FAITH_NAMES[e.faith], ECONOMY_NAMES[e.economy], lastEra && P.economy2 != P.economy ? (std::string(" and ") + ECONOMY_NAMES[P.economy2]).c_str() : ""));
        if (!oldTime) out(fmt("     %d reign%s, %d war%s, %d place%s founded%s", reigns, reigns == 1 ? "" : "s", wars, wars == 1 ? "" : "s", founded, founded == 1 ? "" : "s", inv.empty() ? "" : ("; made: " + inv).c_str()));
    }
    // the rulers
    out("");
    out("THE RULERS");
    int house = -2;
    for (size_t k = 0; k < H.reigns.size(); k++) {
        const HistoryReign& R = H.reigns[k];
        if (R.house != house && R.house >= 0) {
            int n = 0, to = R.start; for (size_t j = k; j < H.reigns.size() && H.reigns[j].house == R.house; j++) { n++; to = H.reigns[j].end; }
            out(fmt("  the House of %s (%d reign%s, %s to %s)", historyPersonName(H, H.families[R.house].founder, false).c_str(), n, n == 1 ? "" : "s", yr(R.start).c_str(), yr(to).c_str()));
        }
        house = R.house;
        static const char* const HOW[11] = {"founded the rule", "inherited", "chosen", "elected", "seized the rule", "appointed", "won it in a war", "came of age", "given the rule", "raised by the revolution", "elected again"};
        static const char* const END[8] = {"died", "deposed", "stepped down", "term ended", "overthrown", "fled", "the rule ended", "gave it over"};
        std::string who = R.ruler >= 0 ? historyPersonName(H, R.ruler, false) : std::string("the mind");
        std::string t = R.ruler >= 0 ? historyRulerTitle(H, R.government, H.persons[R.ruler].female) : "";
        if (!t.empty() && t.compare(0, 4, "the ") != 0) t[0] = (char)toupper(t[0]);
        std::string life;
        if (R.ruler >= 0) {
            const HistoryPerson& Q = H.persons[R.ruler];
            life = fmt(" (born %s, died %s", yr(Q.born).c_str(), yr(Q.died).c_str());
            if (Q.death >= 0 && H.events[Q.death].cause >= 0) {
                const HistoryEvent& c = H.events[H.events[Q.death].cause];
                if (c.kind == HE_SICKNESS) life += ", of " + historySicknessName(H, c.thing);
                else if (c.kind == HE_BATTLE) life += ", killed at " + historyPlaceName(H, c.place);
                else if (c.kind == HE_END) life += ", in the end";
            } else if (Q.death >= 0 && H.events[Q.death].thing == 8) life += ", murdered";
            life += ")";
        }
        out(fmt("    years %s-%s: %s%s%s; %s, %s%s", yr(R.start).c_str(), yr(R.end).c_str(), t.empty() ? "" : (t + " ").c_str(), who.c_str(), life.c_str(), HOW[clampi(R.how, 0, 10)], END[clampi(R.endHow, 0, 7)],
                R.regent >= 0 ? (", the regent " + historyPersonName(H, R.regent, false)).c_str() : ""));
    }
    // the places
    out("");
    out("THE PLACES");
    auto placeLine = [&](int i) {
        const HistoryPlace& p = H.places[i];
        static const char* const FATE[4] = {"left in the collapse", "left early", "the last place", "cleared"};
        std::string cls = p.sclass == SC_CAPITAL ? "the capital" : (p.sclass == SC_CITY ? "a city" : (p.sclass == SC_TOWN ? (H.age == AGE_FUTURISTIC && p.plan >= 0 ? "a great place" : "a town") : (p.sclass == SC_VILLAGE ? (pre ? "a seat" : "a lesser place") : "a work")));
        std::string why;
        if (p.endEvent >= 0 && p.fate != HP_LAST) why = " (" + historyEventLine(H, p.endEvent) + ")";
        return fmt("  %s, %s, of the %s age: founded %s%s%s; %s in %s%s", historyPlaceName(H, i).c_str(), cls.c_str(), AGE_NAMES[p.age], yr(p.founded).c_str(),
                   p.founder >= 0 ? (" by " + historyPersonName(H, p.founder, false)).c_str() : "", p.from >= 0 ? (" from " + historyPlaceName(H, p.from)).c_str() : "",
                   FATE[clampi(p.fate, 0, 3)], yr(p.ended).c_str(), why.c_str());
    };
    int townsByAge[AGE_COUNT] = {0};
    for (int i = 0; i < (int)H.places.size(); i++) {
        const HistoryPlace& p = H.places[i];
        if (p.sclass == SC_CAPITAL || p.sclass == SC_CITY || p.plan < 0 || p.fate == HP_EARLY || p.fate == HP_LAST || (H.age >= AGE_FUTURISTIC) || full) out(placeLine(i));
        else townsByAge[p.age]++;
    }
    std::string ta;
    for (int a = 0; a < AGE_COUNT; a++) if (townsByAge[a]) ta += fmt("%s%d of the %s age", ta.empty() ? "" : ", ", townsByAge[a], AGE_NAMES[a]);
    if (!ta.empty()) out("  and the towns, left in the collapse: " + ta);
    // the wars, the sicknesses, the inventions, the institutions
    out("");
    out("THE WARS");
    for (size_t k = 0; k < H.wars.size(); k++) {
        const HistoryWar& w = H.wars[k];
        out(fmt("  %s (%s-%s), %s: %d battle%s, %s dead%s", historyWarName(H, (int)k).c_str(), yr(w.start).c_str(), yr(w.end).c_str(), HISTORY_WAR_KINDS[w.kind], w.battles, w.battles == 1 ? "" : "s",
                withThousands(w.dead).c_str(), w.peace >= 0 ? ("; " + historyEventLine(H, w.peace).substr(historyWarName(H, (int)k).size() + 8)).c_str() : ""));
    }
    out("");
    out("THE SICKNESSES");
    for (size_t k = 0; k < H.sicknesses.size(); k++) {
        const HistorySickness& S = H.sicknesses[k];
        out(fmt("  %s, %s-%s: %d wave%s, %s dead", historySicknessName(H, (int)k).c_str(), yr(S.first).c_str(), yr(S.last).c_str(), S.waves, S.waves == 1 ? "" : "s", withThousands(S.dead).c_str()));
    }
    out("");
    out("THE INVENTIONS AND THE INSTITUTIONS");
    for (const HistoryInvention& I : H.inventions) out(fmt("  %s: %s, by %s at %s", yr(I.year).c_str(), historyInventionName(I.id).c_str(), historyPersonName(H, I.person, false).c_str(), historyPlaceName(H, I.place).c_str()));
    for (size_t k = 0; k < H.institutions.size(); k++) {
        const HistoryInstitution& I = H.institutions[k];
        for (size_t i = 0; i < H.events.size(); i++) if (H.events[i].kind == HE_INSTITUTION && H.events[i].thing == (int)k) { out(fmt("  %s: %s", yr(I.founded).c_str(), historyEventLine(H, (int)i).c_str())); break; }
    }
    // the figures
    out("");
    out("THE FIGURES");
    int figs = 0;
    for (int i = 0; i < (int)H.persons.size(); i++) {
        const HistoryPerson& Q = H.persons[i];
        if (Q.role == HR_NONE || Q.role == HR_RULER) continue;
        if (Q.role == HR_FOUNDER && !full) { bool great = false; for (const HistoryPlace& p : H.places) if (p.founder == i && (p.sclass == SC_CAPITAL || p.sclass == SC_CITY)) great = true; if (!great) continue; }
        figs++;
        if (!full && figs > 60) continue;
        std::string deeds;
        for (int ev : H.eventsOf(i)) { const HistoryEvent& e = H.events[ev]; if (e.pub && e.kind != HE_DEATH) { deeds = historyEventLine(H, ev); break; } }
        out(fmt("  %s, %s%s (born %s, died %s): %s", historyPersonName(H, i, false).c_str(), HISTORY_ROLE_NAMES[Q.role], Q.home >= 0 ? (" of " + historyPlaceName(H, Q.home)).c_str() : "", yr(Q.born).c_str(), yr(Q.died).c_str(), deeds.c_str()));
    }
    if (!full && figs > 60) out(fmt("  and %d more", figs - 60));
    // the families
    out("");
    out(fmt("THE FAMILIES (followed from year %s)", yr(H.famFrom).c_str()));
    for (size_t k = 0; k < H.families.size(); k++) {
        const HistoryFamily& F = H.families[k];
        if (F.house && !full) continue;
        int n = 0, plague = 0, battle = 0, famine = 0, young = 0, gen = 0, lastAlive = 0;
        std::vector<int> depth(H.persons.size(), -1);
        for (int i = 0; i < (int)H.persons.size(); i++) {
            const HistoryPerson& Q = H.persons[i];
            if (Q.family != (int)k) continue;
            n++;
            int d = 0; const int par = Q.mother >= 0 && H.persons[Q.mother].family == (int)k ? Q.mother : (Q.father >= 0 && H.persons[Q.father].family == (int)k ? Q.father : -1);
            if (par >= 0 && depth[par] >= 0) d = depth[par] + 1;
            depth[i] = d; gen = std::max(gen, d + 1);
            if (Q.death >= 0) {
                const HistoryEvent& e = H.events[Q.death];
                if (e.thing == 1) plague++; else if (e.thing == 3) battle++; else if (e.thing == 4) famine++; else if (e.thing == 11) young++;
            }
            if (Q.died >= H.last) lastAlive++;
        }
        const std::string surname = tongueName(tongueOfSeed(H.lore.seed), F.name, TN_FAMILY);
        const std::string what = F.house ? fmt("the House of %s, rulers at %s from %s", historyPersonName(H, F.founder, false).c_str(), historyPlaceName(H, F.home).c_str(), yr(F.from).c_str())
                                         : fmt("the family %s, %s of %s from %s", surname.c_str(), historyTradePlural(F.trade).c_str(), historyPlaceName(H, F.home).c_str(), yr(F.from).c_str());
        out(fmt("  %s: %d persons over %d generations; %d died young, %d of sicknesses, %d in battle, %d in famines; %d at the end", what.c_str(), n, gen, young, plague, battle, famine, lastAlive));
    }
    // a family's years, told: the first family's private events (a sample of what the records will read)
    if (!H.families.empty()) {
        int fam = -1; for (size_t k = 0; k < H.families.size(); k++) if (!H.families[k].house) { fam = (int)k; break; }
        if (fam >= 0) {
            out("");
            out(fmt("ONE FAMILY'S YEARS: the family %s (the first sixty of its events)", historyFamilyName(H, fam).c_str()));
            int shown = 0;
            for (int i = 0; i < (int)H.events.size() && shown < 60; i++) {
                const HistoryEvent& e = H.events[i];
                if (e.pub) continue;
                bool ours = false;
                for (int x : {e.a, e.b}) if (x >= 0 && H.persons[x].family == fam) ours = true;
                if (!ours) continue;
                out(line(i)); shown++;
            }
        }
    }
    // the chronicle
    const int collapseFrom = H.stageYear[0];
    if (full) {
        out("");
        out("THE CHRONICLE");
        int era = -1;
        for (int i = 0; i < (int)H.events.size(); i++) {
            const HistoryEvent& e = H.events[i];
            if (!e.pub || e.year >= collapseFrom) continue;
            const int ei = H.eraAt(e.year);
            if (ei != era && e.year >= H.eras[ei].start) { era = ei; out(fmt(" -- %s (%s) --", H.eras[ei].name.c_str(), AGE_NAMES[H.eras[ei].age])); }
            out(line(i));
        }
    }
    out("");
    out(fmt("THE COLLAPSE (%s, from year %s)", CAUSE_NAMES[H.cause], yr(collapseFrom).c_str()));
    for (int i = 0; i < (int)H.events.size(); i++) {
        const HistoryEvent& e = H.events[i];
        if (e.year < collapseFrom) continue;
        if (!e.pub && !(e.kind == HE_DEATH && e.a == H.lastPerson)) continue;
        if (!full && e.salience < 0.3f) continue;
        out(line(i));
    }
    if (H.lastPerson >= 0) {
        const HistoryPerson& Q = H.persons[H.lastPerson];
        std::string kin;
        if (Q.family >= 0 && !H.families[Q.family].house) kin = " of the family " + historyFamilyName(H, Q.family);
        out(fmt("  The last: %s%s, %s, born %s at %s.", historyPersonName(H, H.lastPerson, false).c_str(), kin.c_str(), historyTradeName(Q.trade).c_str(), yr(Q.born).c_str(), historyPlaceName(H, Q.home).c_str()));
    }
    (void)sys;
}

int runHistory(int argc, char** argv) {
    mkdir("shots/tests", 0755);
    const int which = takePeopleArg(argc, argv);
    std::vector<AgeWorld> worlds;
    if (argc > 2 && std::string(argv[2]) == "check") {   // the unit's H-02 checks alone
        int failures = 0;
        historyChecks(modeCheck(failures));
        printf("history check: %d failure%s\n", failures, failures == 1 ? "" : "s");
        return failures ? 1 : 0;
    }
    const bool ages = argc > 2 && std::string(argv[2]) == "ages";
    if (ages) for (int a = 0; a < AGE_COUNT; a++) worlds.push_back(AGE_WORLDS[a]);
    else if (argc > 4) worlds.push_back(AgeWorld{atoll(argv[2]), atoll(argv[3]), atoi(argv[4]), which});
    else { printf("history <sx> <sz> <body> [second] | history ages | history check\n"); return 1; }
    for (const AgeWorld& aw : worlds) {
        Star st; StarSystem sys; if (!systemAt(aw.sx, 0, aw.sz, st, sys, true)) { printf("no star at %lld 0 %lld\n", (long long)aw.sx, (long long)aw.sz); continue; }
        if (aw.body >= (int)sys.bodies.size()) { printf("no body %d\n", aw.body); continue; }
        const Body& b = sys.bodies[aw.body];
        const BodyGen g = BodyGen::make(b);
        if (!worldHadCivilisation(g)) { printf("%s had no people\n", b.name.c_str()); continue; }
        const int people = std::min(aw.which, peoplesOf(g) - 1);
        double t0 = nowSec();
        std::shared_ptr<const WorldPlan> plan = worldPlanOf(g);
        const double planMs = (nowSec() - t0) * 1000;
        std::shared_ptr<History> H = buildHistory(sys, b, g, people);
        std::shared_ptr<History> H2 = buildHistory(sys, b, g, people);   // the time again, warm
        std::string name = b.name; for (char& ch : name) if (ch == ' ') ch = '_';
        const std::string fn = fmt("shots/tests/history_%s%s.txt", name.c_str(), people ? "_2" : "");
        FILE* f = fopen(fn.c_str(), "w");
        if (f) { printLegends(sys, b, *H, f, true); fclose(f); }
        printLegends(sys, b, *H, stdout, false);
        printf("\n  the world's plan %.0f ms first (cached after); the history %.1f ms, again %.1f ms -> %s\n\n", planMs, H->buildMs, H2->buildMs, fn.c_str());
    }
    return 0;
}

// C-13: `peoples [count]`: the worlds of two peoples among the civilisation worlds of the scan near home (sectors 150..300 x
// 20..120 on the plane, the scan stopping once `count` are found, default 6): each with its two peoples by name, their ends, the
// divide's pole and how much land it crosses, and a land point deep on each side (where the finders and the scenes stand).
// `peoples <sx> <sz> <body index> [latDeg lonDeg]`: one world's peoples in full: each one's names, culture, span, calendar and
// last recording, the other's fate, the shards of each that speak of the other (decoded whole), then the settlements of the
// 13 x 13 cells round the point by people and the roads between the two peoples' settlements
static bool pointIsLand(const BodyGen& g, const Vec3& u) { SurfaceSample s = sampleSurface(g, u, 2048.0); return !(s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water) || s.oldSea > 0.5); }
static int divideLandPoints(const BodyGen& g) {   // of 48 along the divide
    Vec3 n = peopleDivide(g); DrainageOff off;
    Vec3 e = normalize(cross(n, std::fabs(n.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), f = cross(n, e);
    int land = 0; for (int k = 0; k < 48; k++) { double t = k * TAU / 48; if (pointIsLand(g, normalize(e * std::cos(t) + f * std::sin(t)))) land++; }
    return land;
}
Vec3 peopleSidePoint(const BodyGen& g, int people, double wantLat) {   // a land point deep on a people's side of the divide, near the wanted latitude
    Vec3 n = peopleDivide(g); DrainageOff off; Vec3 best = people == 0 ? n : n * -1.0; double bestScore = -1e9;
    for (int j = 0; j < 36; j++) for (int i = 0; i < 72; i++) {
        double lat = (-87.5 + 5 * j) * DEG, lon = (-177.5 + 5 * i) * DEG;
        Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
        double side = dot(u, n) * (people == 0 ? 1 : -1);
        if (side < 0.3 || !pointIsLand(g, u)) continue;
        double score = side - std::fabs(lat - wantLat) / (60 * DEG);
        if (score > bestScore) { bestScore = score; best = u; }
    }
    return best;
}
bool frontierPoint(const BodyGen& g, double& lat, double& lon) {   // the first of 72 points along the divide whose 5 x 5 cells hold sited settlements of both peoples
    Vec3 n = peopleDivide(g); DrainageOff off;
    Vec3 e = normalize(cross(n, std::fabs(n.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), f = cross(n, e);
    for (int k = 0; k < 72; k++) {
        double t = k * TAU / 72; Vec3 u = normalize(e * std::cos(t) + f * std::sin(t));
        double la, lo; StarSystem::latLonFromBody(u, la, lo); int gl, gn; ruinCellOf(g, la, lo, gl, gn); int cnt[2] = {0, 0};
        for (int dl = -2; dl <= 2; dl++) for (int dn = -2; dn <= 2; dn++) { RuinSpec sp; if (ruinOfCell(g, gl + dl, gn + dn, sp, false) && ruinSiteOk(g, sp)) cnt[sp.people]++; }
        if (cnt[0] && cnt[1]) { lat = la; lon = lo; return true; }
    }
    return false;
}
// T-01: a point of a people's countryside for the checks that read it (since the plan of the world the cells hold settlements only in the land a
// people held): a place of its plan of the middle rank (a town: its cells round it the countryside's own), or with `coast` the first place by
// rank with water within 700 m of its core (the harbours); false when the people has no place
bool planPlacePoint(const BodyGen& g, int people, double& lat, double& lon, bool coast) {
    std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
    if (P->peoples == 0) return false;
    const PeoplePlan& pp = P->plans[std::min(people, P->peoples - 1)];
    if (pp.places.empty()) return false;
    if (coast) {
        DrainageOff off;
        const double Rm = g.R * 1000.0;
        for (const PlanPlace& p : pp.places) {
            if (!p.coast) continue;
            for (int k = 0; k < 12; k++) {
                const double a = k * TAU / 12, d = p.radiusM + 700, cl = std::max(std::cos(p.lat), 0.05);
                const double la = p.lat + std::cos(a) * d / Rm, lo = p.lon + std::sin(a) * d / (Rm * cl);
                const SurfaceSample q = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 64.0);
                if (q.material == MAT_WATER || (q.water > -1e8 && q.height < q.water) || q.oldSea > 0.5) { lat = p.lat; lon = p.lon; return true; }
            }
        }
    }
    const PlanPlace& p = pp.places[pp.places.size() / 2];
    lat = p.lat; lon = p.lon;
    return true;
}
// C-13: `second` anywhere in a mode's arguments asks for a world's second people; the word is taken out before the rest are read
int takePeopleArg(int& argc, char** argv) {
    int which = 0;
    for (int i = 2; i < argc; i++) if (std::string(argv[i]) == "second") { which = 1; for (int j = i; j + 1 < argc; j++) argv[j] = argv[j + 1]; argc--; i--; }
    return which;
}
static std::string erasLine(const PeopleAge& P) {   // "prehistoric -8411..0 (chiefs), medieval 1..2310 (a monarchy), ..."
    std::string s;
    for (const Era& e : P.eras) s += fmt("%s%s %d..%d (%s)", s.empty() ? "" : ", ", AGE_NAMES[e.age], e.start, e.end, GOVERNMENT_NAMES[e.government]);
    return s;
}
static void printProfile(const PeopleProfile& P, const char* indent) {
    printf("%sthe age: %s; the eras %s; their count ends at year %d%s\n", indent, AGE_NAMES[P.age], erasLine(P).c_str(), P.lastYear, P.radio ? fmt("; radio from year %d", P.radioYear).c_str() : "; no radio");
    printf("%sthe end %.0f years ago by %s (%s%s), over %d years from year %d: %s from %d, %s from %d, %s from %d\n", indent, P.endedYearsAgo, CAUSE_NAMES[P.cause], causeSpeed(P.cause) == 0 ? "sudden" : (causeSpeed(P.cause) == 1 ? "over years" : "over generations"),
           causeWorldwide(P.cause) ? ", it could end a world" : "", P.collapseYears, P.collapseYear, STAGE_NAMES[STAGE_ONSET], P.collapseYear + (int)std::lround(P.stage[STAGE_ONSET] * P.collapseYears),
           STAGE_NAMES[STAGE_CRISIS], P.collapseYear + (int)std::lround(P.stage[STAGE_CRISIS] * P.collapseYears), STAGE_NAMES[STAGE_END], P.collapseYear + (int)std::lround(P.stage[STAGE_END] * P.collapseYears));
    printf("%s%s at their height on %.0f%% of the land (the world %.0f%% land in their time); %s (the %s), %s and %s, %s (%s, at %s); valued war %.2f, piety %.2f, trade %.2f, learning %.2f, art %.2f\n", indent, peopleCount(P.peak).c_str(), 100 * P.held, 100 * P.landShare,
           GOVERNMENT_NAMES[P.government], GOVERNMENT_RULERS[P.government][0], ECONOMY_NAMES[P.economy], ECONOMY_NAMES[P.economy2], FAITH_NAMES[P.faith], RITE_NAMES[P.rite], HOLY_NAMES[P.holy],
           P.values[VAL_WAR], P.values[VAL_PIETY], P.values[VAL_TRADE], P.values[VAL_LEARNING], P.values[VAL_ART]);
}
// A-01: `peoples ages [count]`: the peoples within 25 ly of home by distance (every star of the cube round it): the first `count`
// (100) with their age, end, cause and extent, whether they had radio and whether they are on the air (a transmitter sector); the
// shares of the ages over all of them against the decided ones, radio, the ends and the causes by age, the profile's cost; the
// nearest people of each age in space and on the harness's plane (the test worlds, against `AGE_WORLDS`)
static int runPeoplesAges(int argc, char** argv) {
    int want = argc > 3 ? atoi(argv[3]) : 100;
    const int R = 25;
    struct Near { int64_t x, y, z; double d; };
    std::vector<Near> stars; double t0 = nowSec();
    for (int64_t x = HOME_SX - R; x <= HOME_SX + R; x++) for (int64_t y = HOME_SY - R; y <= HOME_SY + R; y++) for (int64_t z = HOME_SZ - R; z <= HOME_SZ + R; z++) {
        double d = std::sqrt((double)((x - HOME_SX) * (x - HOME_SX) + (y - HOME_SY) * (y - HOME_SY) + (z - HOME_SZ) * (z - HOME_SZ)));
        Star s; if (d > R || !starInSector(x, y, z, s, false)) continue;
        stars.push_back({x, y, z, d});
    }
    std::sort(stars.begin(), stars.end(), [](const Near& a, const Near& b) { return a.d != b.d ? a.d < b.d : (a.x != b.x ? a.x < b.x : (a.y != b.y ? a.y < b.y : a.z < b.z)); });
    int ages[AGE_COUNT] = {0}, radio[AGE_COUNT] = {0}, causes[CAUSE_COUNT][AGE_COUNT] = {{0}}, peoples = 0, worlds = 0, onAir = 0, printed = 0;
    double eLo[AGE_COUNT], eHi[AGE_COUNT], heldSum[AGE_COUNT] = {0}, tProfile = 0; for (int a = 0; a < AGE_COUNT; a++) { eLo[a] = 1e18; eHi[a] = 0; }
    struct Found { Near n; int body = -1, which = 0; std::string name; };
    Found nearest[AGE_COUNT];
    printf("the peoples within %d ly of home (sector %lld %lld %lld), the nearest first:\n", R, (long long)HOME_SX, (long long)HOME_SY, (long long)HOME_SZ);
    for (const Near& n : stars) {
        Star s; starInSector(n.x, n.y, n.z, s, true);
        StarSystem sys; sys.generate(s);
        const int pw = sectorTransmits(n.x, n.y, n.z) ? systemPeoplesWorld(sys) : -1;   // the system's people's world is the one on the air
        for (const Body& b : sys.bodies) {
            if (!kindHas(b.type, KT_PEOPLE)) continue;   // a scorched world's people too
            BodyGen g = BodyGen::make(b);
            int np = peoplesOf(g); if (!np) continue;
            worlds++;
            int air = b.index == pw ? peoplesOnAir(g) : 0;
            for (int k = 0; k < np; k++) {
                double p0 = nowSec(); PeopleProfile P = peopleProfileOf(sys, b, g, k); tProfile += nowSec() - p0;
                peoples++; ages[P.age]++; if (P.radio) radio[P.age]++; causes[P.cause][P.age]++; heldSum[P.age] += P.held;
                eLo[P.age] = std::min(eLo[P.age], P.endedYearsAgo); eHi[P.age] = std::max(eHi[P.age], P.endedYearsAgo);
                bool heard = (air >> k) & 1; if (heard) onAir++;
                std::string name = peopleNameOf(peopleGen(g, k));
                if (nearest[P.age].body < 0) { nearest[P.age].n = n; nearest[P.age].body = b.index; nearest[P.age].which = k; nearest[P.age].name = fmt("%s (%s), the %s", b.name.c_str(), KINDS[b.type].name, name.c_str()); }
                if (printed < want) {
                    printed++;
                    printf("%4d %5.1f ly %4lld %3lld %4lld b%-2d%s %-22s %-12s %-11s ended %7.0f y ago by %s over %d y; %s on %.0f%% of %.0f%% land%s\n", printed, n.d, (long long)n.x, (long long)n.y, (long long)n.z, b.index, k ? "/2" : "  ",
                           fmt("%s (%s)", b.name.c_str(), b.type == PT_DESERT ? "desert" : (b.type == PT_SCORCHED_FELISIAN ? "scorched" : "felisian")).c_str(), name.c_str(), AGE_NAMES[P.age], P.endedYearsAgo, CAUSE_NAMES[P.cause], P.collapseYears, peopleCount(P.peak).c_str(), 100 * P.held, 100 * P.landShare,
                           heard ? "; ON THE AIR" : (P.radio ? "; radio" : ""));
                }
            }
        }
    }
    printf("\n%zu stars within %d ly, %d worlds with peoples, %d peoples; %.1f s (a profile %.2f ms)\n", stars.size(), R, worlds, peoples, nowSec() - t0, 1e3 * tProfile / std::max(1, peoples));
    printf("the ages' shares against the decided ones (with radio; the end's years; the land held at their height, on average):\n");
    int radioAll = 0;
    for (int a = 0; a < AGE_COUNT; a++) {
        radioAll += radio[a];
        printf("  %-11s %4d  %5.1f%% (decided %4.1f%%)  radio %3d  ended %.0f..%.0f years ago  held %.0f%%\n", AGE_NAMES[a], ages[a], 100.0 * ages[a] / std::max(1, peoples), 100 * AGE_SHARES[a], radio[a], eLo[a] < 1e17 ? eLo[a] : 0.0, eHi[a], ages[a] ? 100 * heldSum[a] / ages[a] : 0.0);
    }
    printf("  radio: %d of %d (%.1f%%); on the air (a transmitter sector's people's world): %d\n", radioAll, peoples, 100.0 * radioAll / std::max(1, peoples), onAir);
    printf("the causes by age:\n");
    for (int a = 0; a < AGE_COUNT; a++) {
        printf("  %s:", AGE_NAMES[a]);
        for (int c = 0; c < CAUSE_COUNT; c++) if (causes[c][a]) printf(" %s %d;", CAUSE_NAMES[c], causes[c][a]);
        printf("\n");
    }
    printf("the nearest people of each age:\n");
    for (int a = 0; a < AGE_COUNT; a++) {
        const Found& f = nearest[a];
        if (f.body < 0) { printf("  %-11s none within %d ly\n", AGE_NAMES[a], R); continue; }
        printf("  %-11s %5.1f ly at %lld %lld %lld body %d%s: %s\n", AGE_NAMES[a], f.n.d, (long long)f.n.x, (long long)f.n.y, (long long)f.n.z, f.body, f.which ? " (the second people)" : "", f.name.c_str());
    }
    // the test worlds: the nearest of each age on the plane y 0 (every mode's `<sx> <sz> <body>` reaches them)
    Found plane[AGE_COUNT]; int left = AGE_COUNT;
    std::vector<Near> flat;
    for (int64_t x = HOME_SX - 80; x <= HOME_SX + 80; x++) for (int64_t z = HOME_SZ - 80; z <= HOME_SZ + 80; z++) {
        double d = std::sqrt((double)((x - HOME_SX) * (x - HOME_SX) + (z - HOME_SZ) * (z - HOME_SZ)));
        Star s; if (d > 80 || !starInSector(x, 0, z, s, false)) continue;
        flat.push_back({x, 0, z, d});
    }
    std::sort(flat.begin(), flat.end(), [](const Near& a, const Near& b) { return a.d != b.d ? a.d < b.d : (a.x != b.x ? a.x < b.x : a.z < b.z); });
    for (const Near& n : flat) {
        if (!left) break;
        Star s; starInSector(n.x, 0, n.z, s, true);
        StarSystem sys; sys.generate(s);
        for (const Body& b : sys.bodies) {
            if (b.type != PT_FELISIAN && b.type != PT_DESERT) continue;
            BodyGen g = BodyGen::make(b);
            int np = peoplesOf(g);
            for (int k = 0; k < np; k++) {
                int a = peopleAgeOf(g, k).age;
                if (plane[a].body >= 0) continue;
                plane[a].n = n; plane[a].body = b.index; plane[a].which = k; plane[a].name = fmt("%s (%s), the %s", b.name.c_str(), KINDS[b.type].name, peopleNameOf(peopleGen(g, k)).c_str()); left--;
            }
        }
    }
    printf("the test worlds, the nearest of each age on the plane (sector y 0):\n");
    int moved = 0;
    for (int a = 0; a < AGE_COUNT; a++) {
        const Found& f = plane[a]; const AgeWorld& w = AGE_WORLDS[a];
        bool same = f.body >= 0 && w.sx == f.n.x && w.sz == f.n.z && w.body == f.body && w.which == f.which;
        if (!same) moved++;
        if (f.body < 0) { printf("  %-11s none within 80 ly\n", AGE_NAMES[a]); continue; }
        printf("  %-11s %5.1f ly: %lld %lld body %d%s, %s%s\n", AGE_NAMES[a], f.n.d, (long long)f.n.x, (long long)f.n.z, f.body, f.which ? " second" : "", f.name.c_str(), same ? "" : fmt("  (PINNED %lld %lld body %d%s)", (long long)w.sx, (long long)w.sz, w.body, w.which ? " second" : "").c_str());
    }
    printf("%s\n", moved ? "the pinned test worlds (AGE_WORLDS) differ from these: pin them again" : "the pinned test worlds (AGE_WORLDS) hold");
    // the scorched worlds' test worlds, the nearest scorched people of each age on the plane (they are rare: within 300 ly)
    Found dead[AGE_COUNT]; left = AGE_COUNT;
    std::vector<Near> wide;
    for (int64_t x = HOME_SX - 300; x <= HOME_SX + 300; x++) for (int64_t z = HOME_SZ - 300; z <= HOME_SZ + 300; z++) {
        double d = std::sqrt((double)((x - HOME_SX) * (x - HOME_SX) + (z - HOME_SZ) * (z - HOME_SZ)));
        Star s; if (d > 300 || !starInSector(x, 0, z, s, false)) continue;
        if (s.cls != STAR_RED_GIANT && s.cls != STAR_ORANGE_GIANT && s.cls != STAR_CARBON && s.cls != STAR_WHITE_DWARF) continue;   // the scorched worlds' stars (a companion's too: its primary one of them is rare)
        wide.push_back({x, 0, z, d});
    }
    std::sort(wide.begin(), wide.end(), [](const Near& a, const Near& b) { return a.d != b.d ? a.d < b.d : (a.x != b.x ? a.x < b.x : a.z < b.z); });
    int deadPeoples = 0;
    for (const Near& n : wide) {
        Star s; starInSector(n.x, 0, n.z, s, true);
        StarSystem sys; sys.generate(s);
        for (const Body& b : sys.bodies) {
            if (b.type != PT_SCORCHED_FELISIAN) continue;
            BodyGen g = BodyGen::make(b);
            int np = peoplesOf(g);
            for (int k = 0; k < np; k++) {
                deadPeoples++;
                int a = peopleAgeOf(g, k).age;
                if (dead[a].body >= 0) continue;
                dead[a].n = n; dead[a].body = b.index; dead[a].which = k; dead[a].name = fmt("%s (%s), the %s", b.name.c_str(), STAR_CLASSES[s.cls].code, peopleNameOf(peopleGen(g, k)).c_str()); left--;
            }
        }
    }
    printf("the scorched worlds' peoples on the plane within 300 ly: %d; the nearest of each age (P-04):\n", deadPeoples);
    int deadMoved = 0;
    for (int a = 0; a < AGE_COUNT; a++) {
        const Found& f = dead[a]; const AgeWorld& w = SCORCHED_WORLDS[a];
        bool same = (f.body < 0 && w.body < 0) || (f.body >= 0 && w.sx == f.n.x && w.sz == f.n.z && w.body == f.body && w.which == f.which);
        if (!same) deadMoved++;
        if (f.body < 0) { printf("  %-11s none within 300 ly%s\n", AGE_NAMES[a], same ? "" : "  (PINNED one)"); continue; }
        printf("  %-11s %5.1f ly: %lld %lld body %d%s, %s%s\n", AGE_NAMES[a], f.n.d, (long long)f.n.x, (long long)f.n.z, f.body, f.which ? " second" : "", f.name.c_str(), same ? "" : fmt("  (PINNED %lld %lld body %d%s)", (long long)w.sx, (long long)w.sz, w.body, w.which ? " second" : "").c_str());
    }
    printf("%s\n", deadMoved ? "the pinned scorched test worlds (SCORCHED_WORLDS) differ from these: pin them again" : "the pinned scorched test worlds (SCORCHED_WORLDS) hold");
    return 0;
}
// A-03: `peoples fresh`: on the harness's plane (sector y 0) near home, the peoples of each age that ended most recently (what is
// left of their works stands), the nearest first: the candidates for the ages' scenes (`AGE_SCENE_WORLDS`)
static int runPeoplesFresh(int argc, char** argv) {
    (void)argc; (void)argv;
    static const double LIMIT[AGE_COUNT] = {400, 700, 900, 1200, 1e9, 1e9};
    struct F { double d; int64_t x, z; int body, which; std::string what; double years, wet; };
    std::vector<F> found[AGE_COUNT];
    for (int64_t x = HOME_SX - 40; x <= HOME_SX + 40; x++) for (int64_t z = HOME_SZ - 40; z <= HOME_SZ + 40; z++) {
        Star s; if (!starInSector(x, 0, z, s, true)) continue;
        double d = std::sqrt((double)((x - HOME_SX) * (x - HOME_SX) + HOME_SY * HOME_SY + (z - HOME_SZ) * (z - HOME_SZ)));
        StarSystem sys; sys.generate(s);
        for (const Body& b : sys.bodies) {
            if (b.type != PT_FELISIAN && b.type != PT_DESERT) continue;
            BodyGen g = BodyGen::make(b);
            int np = peoplesOf(g);
            for (int k = 0; k < np; k++) {
                Culture c = cultureOf(g, k);
                if (c.ageYears > LIMIT[c.age]) continue;
                found[c.age].push_back({d, x, z, b.index, k, fmt("%s (%s)", b.name.c_str(), KINDS[b.type].name), c.ageYears, c.wet});
            }
        }
    }
    for (int a = 0; a < AGE_COUNT; a++) {
        std::sort(found[a].begin(), found[a].end(), [](const F& p, const F& q) { return p.d < q.d; });
        printf("%s (ended under %.0f years ago): %zu\n", AGE_NAMES[a], LIMIT[a], found[a].size());
        for (size_t i = 0; i < found[a].size() && i < 6; i++) { const F& f = found[a][i]; printf("  %5.1f ly  %lld 0 %lld body %d%s: %s, ended %.0f years ago, wet %.2f\n", f.d, (long long)f.x, (long long)f.z, f.body, f.which ? " (second)" : "", f.what.c_str(), f.years, f.wet); }
    }
    return 0;
}

// R-415: the landing map's peoples (V): `peoples map [sx sz body]` (default the drone bay's world of two, Yeesheleemiola II-b): the world's map with
// the layer off (as it opens), on with nothing held, with a few records of the first people held (the places they were kept at) and with all
// of them (its whole map: its land, its places, its ways), then the zoom over its capital with the layer on and off; two marks of ruins by
// the capital, hidden with the layer off
static int runPeoplesMap(int argc, char** argv) {
    const int64_t sx = argc > 5 ? atoll(argv[3]) : 208, sz = argc > 5 ? atoll(argv[4]) : 51; const int bi = argc > 5 ? atoi(argv[5]) : 5;
    Star st; if (!starInSector(sx, 0, sz, st, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
    game.newGame(); game.setState(GameState::SPACE);
    game.settings.renderScale = 2; game.applySettings();
    Input in;
    auto run = [&](double secs) { runFramesAtLeastOne(game, in, secs); };
    auto press = [&](int key) { pressKey(game, in, key); };
    auto shot = [&](const char* name) { const std::string fn = fmt("shots/tests/landpeoples_%s.png", name); writePNG(fn.c_str(), game.output(), FBW, FBH); printf("  %-6s %s\n         -> %s\n", name, game.testLandPeoplesInfo().c_str(), fn.c_str()); };
    game.testParkAt(st, bi); run(0.2);
    StarSystem sys; sys.generate(st);
    if (bi >= (int)sys.bodies.size()) { printf("no body %d\n", bi); return 1; }
    const BodyGen g = BodyGen::make(sys.bodies[bi]);
    if (!worldHadCivilisation(g)) { printf("%s: no people lived there\n", sys.bodies[bi].name.c_str()); return 1; }
    std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
    game.testDronesSettle(); run(0.1);
    const PlanPlace cap = P->plans[0].places.empty() ? PlanPlace() : P->plans[0].places[0];
    printf("%s (%d people%s), the first people's capital at %.2f %.2f, %d records\n", sys.bodies[bi].name.c_str(), peoplesOf(g), peoplesOf(g) > 1 ? "s" : "", cap.lat / DEG, cap.lon / DEG, recordCountOf(g, 0));
    const std::string bkey = Guide::bodyKey(sx, 0, sz, bi);
    for (int k = 0; k < 2; k++) { SurfaceMark m; m.body = bkey; m.lat = cap.lat + 0.004 * (k + 1); m.lon = cap.lon + 0.003 * k; m.kind = LM_RUIN; m.name = k ? "SECOND RUIN" : "FIRST RUIN"; game.guide.marks.push_back(m); }
    game.testLandCursor(cap.lat / DEG, cap.lon / DEG); run(0.1);
    shot("off");
    const std::string off = game.testLandPeoplesInfo();
    press(KEY_V); run(0.1); shot("none");
    const int n0 = recordCountOf(g, 0);
    for (int i = 0; i < n0; i += 7) game.guide.shards.insert(bkey + "/S" + std::to_string(shardWorldIndex(0, i)));
    run(0.1); shot("some");
    const std::string some = game.testLandPeoplesInfo();
    for (int i = 0; i < n0; i++) game.guide.shards.insert(bkey + "/S" + std::to_string(shardWorldIndex(0, i)));
    run(0.1); shot("all");
    const std::string all = game.testLandPeoplesInfo();
    press(KEY_Z); run(0.3); shot("zoom");
    const std::string zoomOn = game.testLandPeoplesInfo();
    press(KEY_V); run(0.1); shot("zoom_off");
    printf("the layer: off at first %s; the marks hidden with it %s; a few held: places %s; all held: the whole map %s; the zoom: rings %s\n",
           off.find("hidden") != std::string::npos ? "yes" : "NO", off.find("0 marks") != std::string::npos ? "yes" : "NO", some.find("places known") != std::string::npos ? "yes" : "NO",
           all.find("ALL") != std::string::npos ? "yes" : "NO", zoomOn.find(" 0 rings") == std::string::npos ? "yes" : "NO");
    return 0;
}
// P-04: `peoples scorched [age]`: a scorched world's people, the test world of the age (`SCORCHED_WORLDS`; the medieval one by default, an age by its
// name or its number): its towns as `towns` prints them, then its fifty records as `shards` prints them, to read beside a felisian people's
static int runPeoplesScorched(int argc, char** argv) {
    int a = AGE_MEDIEVAL;
    if (argc > 3) { std::string w = argv[3]; for (char& c : w) if (c == '_') c = ' '; for (int k = 0; k < AGE_COUNT; k++) if (w == AGE_NAMES[k] || w == std::to_string(k)) a = k; }
    const AgeWorld& W = SCORCHED_WORLDS[a];
    if (W.body < 0) { printf("no scorched people of the %s age pinned\n", AGE_NAMES[a]); return 1; }
    std::string sx = std::to_string(W.sx), sz = std::to_string(W.sz), bi = std::to_string(W.body);
    printf("the %s age's scorched test world (%s %s %s):\n", AGE_NAMES[a], sx.c_str(), sz.c_str(), bi.c_str());
    char* tv[] = {argv[0], (char*)"towns", sx.data(), sz.data(), bi.data(), nullptr};
    const int t = runTowns(5, tv);
    char* sv[] = {argv[0], (char*)"shards", sx.data(), sz.data(), bi.data(), nullptr};
    return runShards(5, sv) || t;
}

// D-05: `peoples rogues [ly]`: the drifters near home that kept a people (one frozen felisian or desert drifter in four), nearest first: the drifter,
// the star it left and its former orbit, each people's age, radio, how they lost their sun, their years (the collapse, the last sunrise, the
// end and how long ago), their records and the beacon of a radio age's
static int runPeoplesRogues(int argc, char** argv) {
    const double R = argc > 3 ? std::max(10.0, atof(argv[3])) : 3000.0;
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
    const std::vector<Star> list = drifterPeoplesNearHome(R);
    int ages[AGE_COUNT] = {0}, ways[SW_COUNT] = {0}, peoples = 0, radio = 0, beacons = 0;
    printf("the drifters within %.0f ly of home that kept a people: %zu\n", R, list.size());
    for (size_t i = 0; i < list.size(); i++) {
        const Star& d = list[i];
        StarSystem sys; sys.generate(d);
        const Body& b = sys.bodies[0];
        const BodyGen g = BodyGen::make(b);
        const DrifterPast past = drifterPast(d);
        const PeoplesWorld W = peoplesWorldOf(sys, b);
        printf("%2zu  %s (%s, %.0f km), sector %lld %lld %lld, %.0f ly from home\n", i, d.name.c_str(), KINDS[b.type].name, b.radiusKm, (long long)d.sx, (long long)d.sy, (long long)d.sz, length(d.pos - home.pos) / SECTOR_KM);
        printf("    left %s (%s) %.0f years ago at %.1f km/s, %.0f ly from it now; its year there %.2f AU out, %.0f of its days%s\n", past.star.name.c_str(), STAR_CLASSES[past.star.cls].name,
               past.years, past.speedKms, length(past.star.pos - d.pos) / SECTOR_KM, W.b->orbitRadiusKm / AU_GAME_KM, W.b->orbitPeriod / std::max(std::fabs(W.b->rotPeriod), 1.0), W.hold ? "" : " (NO FORMER SYSTEM)");
        const DrifterBeacon B = drifterBeaconOf(sys);   // the beacon: a radio people's last broadcast (its radio invented before the last sunrise)
        if (B.people >= 0) beacons++;
        for (int k = 0; k < peoplesOf(g); k++) {
            const PeopleProfile P = peopleProfileOf(sys, b, g, k);
            const Lore L = loreQuick(sys, b, g, k);
            peoples++; ages[P.age]++; if (P.sunWay >= 0) ways[P.sunWay]++; if (P.radio) radio++;
            printf("    the %s: %s%s; lost their sun by %s%s; the warning from year %d, the last sunrise in %d, the end in %d (%d years in the dark), %.0f years ago; %d records%s\n",
                   L.people.c_str(), AGE_NAMES[P.age], P.radio ? ", radio" : "", P.sunWay >= 0 ? SUN_WAY_NAMES[P.sunWay] : "?",
                   P.sunGiant >= 0 ? fmt(" (%s, a %s)", W.sys->bodyLabel(P.sunGiant).c_str(), KINDS[W.sys->bodies[P.sunGiant].type].name).c_str() : "",
                   P.collapseYear, P.lastSunrise, P.lastYear, P.lastYear - P.lastSunrise, P.endedYearsAgo, L.records,
                   B.people == k ? fmt("; the beacon (%s under the voice)", RADIO_PROGRAMME_NAMES[std::max(0, B.machine)]).c_str()
                                 : (P.radio ? (B.people >= 0 ? "; the beacon the other people's" : "; no beacon (its radio came after the last sunrise)") : ""));
        }
    }
    printf("%d peoples:", peoples); for (int a = 0; a < AGE_COUNT; a++) printf(" %s %d%s", AGE_NAMES[a], ages[a], a + 1 < AGE_COUNT ? "," : ";");
    printf(" by"); for (int w = 0; w < SW_COUNT; w++) printf(" %s %d%s", SUN_WAY_NAMES[w], ways[w], w + 1 < SW_COUNT ? "," : ";");
    printf(" %d with radio; %d beacons\n", radio, beacons);
    return 0;
}

int runPeoples(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[2]) == "rogues") return runPeoplesRogues(argc, argv);
    if (argc >= 3 && std::string(argv[2]) == "map") return runPeoplesMap(argc, argv);
    if (argc >= 3 && std::string(argv[2]) == "scorched") return runPeoplesScorched(argc, argv);
    if (argc >= 3 && std::string(argv[2]) == "ages") return runPeoplesAges(argc, argv);
    if (argc >= 3 && std::string(argv[2]) == "fresh") return runPeoplesFresh(argc, argv);
    if (argc >= 5) {   // one world in full
        int64_t sx = atoll(argv[2]), sz = atoll(argv[3]); int bi = atoi(argv[4]);
        Star s; StarSystem sys; if (!systemAt(sx, 0, sz, s, sys, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
        if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
        const Body& b = sys.bodies[bi]; BodyGen g = BodyGen::make(b);
        int np = peoplesOf(g);
        if (np == 0) { printf("%s: no civilisation\n", b.name.c_str()); return 0; }
        Vec3 n = peopleDivide(g);
        printf("%s, %s of %s: %s%s\n", b.name.c_str(), KINDS[b.type].name, s.name.c_str(), np > 1 ? "two peoples" : "one people", np > 1 ? fmt(", %s; the divide's pole %.3f %.3f %.3f crosses land at %d of 48 points", peoplesEndedTogether(g) ? "ended together" : "ended apart", n.x, n.y, n.z, divideLandPoints(g)).c_str() : "");
        Lore L[2]; Culture C[2];
        for (int k = 0; k < np; k++) {
            L[k] = loreOf(sys, b, g, k); C[k] = cultureOf(g, k);
            std::vector<Shard> sh; shardsOf(sys, b, g, L[k], SHARDS_PER_WORLD, sh);
            printf("  people %d, the %s: names of style %d (god %s, river %s, mountain %s, sea %s, towns %s and %s, founder %s, moon %s, star %s, festival %s); %d years from the founding; %s; the last S%d\n", k + 1, L[k].people.c_str(), L[k].style,
                   L[k].god.c_str(), L[k].river.c_str(), L[k].mountain.c_str(), L[k].sea.c_str(), L[k].city.c_str(), L[k].city2.c_str(), L[k].founder.c_str(), L[k].moon.c_str(), L[k].star.c_str(), L[k].festival.c_str(), L[k].spanYears + 1, calendarLine(L[k]).c_str(), L[k].last);
            printf("    culture: style %d, family %d, tall %.2f, %s roofs, towns' plan %d, towers %d, decay %.2f (the end %.0f years ago, weathered as %.0f, wet %.2f), buried %.1f m; roads %s, %.1f m wide, wear %.2f\n", C[k].style, C[k].family, C[k].tall, C[k].stoneRoofs ? "stone" : "no", C[k].plan, (int)C[k].towers, C[k].decay, C[k].ageYears, C[k].weatherYears, C[k].wet, C[k].buried, C[k].paved ? "paved" : "beaten", 2 * C[k].roadHalf, C[k].roadWear);
            printProfile(peopleProfileOf(sys, b, g, k), "    ");
            Tongue T = tongueOf(g, L[k]); Voice V = voiceOf(g, L[k], T); Tradition Tr = traditionOf(g, L[k]); Language lang; languageOf(L[k], sh, T, lang);
            printf("    voice: %s; music: %s, %s; %d words in the language\n", voiceLine(V).c_str(), traditionLine(Tr).c_str(), timbresLine(Tr, Tr.hasDrum).c_str(), (int)lang.words.size());
            if (np > 1) {
                printf("    the other: the %s, who %s; the shards that speak of them:\n", L[k].other.c_str(), L[k].otherFate < 0 ? "ended first" : (L[k].otherFate > 0 ? "went on after this people's end" : "ended with this people"));
                int shown = 0; for (int i = 0; i < SHARDS_PER_WORLD; i++) { if (sh[i].music || sh[i].text.find(L[k].other) == std::string::npos) continue; shown++; printf("      S%d year %d %s %s: %s\n", i, sh[i].year, SHARD_TONE_NAMES[sh[i].tone], sh[i].chart ? "star chart" : shardKindName(sh[i].kind), sh[i].text.c_str()); }
                if (!shown) printf("      none\n");
            }
        }
        if (np > 1) {   // the settlements by people round the point, and the roads between the peoples
            double lat = 0.2, lon = 0.7;
            if (argc > 6) { lat = atof(argv[5]) * DEG; lon = atof(argv[6]) * DEG; }
            else if (frontierPoint(g, lat, lon)) printf("the frontier: %.2f %.2f (both peoples' settlements within two cells)\n", lat / DEG, lon / DEG);
            else { lat = 0.2; lon = 0.7; printf("no point of the divide has both peoples' settlements within two cells\n"); }
            int gLat0, gLon0; ruinCellOf(g, lat, lon, gLat0, gLon0);
            int byPeople[2] = {0, 0}, printed = 0; DrainageOff off;
            RoadNodeCache nc; nc.reset(&g); std::vector<RoadPair> pairs;
            for (int dl = -6; dl <= 6; dl++) for (int dn = -6; dn <= 6; dn++) {
                RuinSpec sp;
                if (!ruinOfCell(g, gLat0 + dl, gLon0 + dn, sp, false) || !ruinSiteOk(g, sp)) continue;
                byPeople[sp.people]++;
                if (printed++ < 12) printf("  %-8s %3d %4d at %7.3f %8.3f: people %d (the %s)\n", SETTLEMENT_CLASS_NAMES[sp.sclass], gLat0 + dl, gLon0 + dn, sp.lat / DEG, sp.lon / DEG, sp.people + 1, L[sp.people].people.c_str());
                nc.pairsOf(gLat0 + dl, gLon0 + dn, pairs);
            }
            std::sort(pairs.begin(), pairs.end(), [](const RoadPair& x, const RoadPair& y) { return x.id < y.id; });
            pairs.erase(std::unique(pairs.begin(), pairs.end(), [](const RoadPair& x, const RoadPair& y) { return x.id == y.id; }), pairs.end());
            int within = 0, between = 0, walked = 0;
            for (const RoadPair& p : pairs) {
                if (p.a.people == p.b.people) { within++; continue; }
                between++;
                Road r; if (roadWay(g, C[roadPeopleOf(p.a, p.b)], p.a, p.b, r)) { walked++; if (walked <= 6) printf("  a road between the peoples: %s of people %d %d %d -> %s of people %d %d %d, %.0f m, built as people %d built (%s, %.1f m wide)\n", SETTLEMENT_CLASS_NAMES[r.a.sclass], r.a.people + 1, r.a.gLat, r.a.gLon, SETTLEMENT_CLASS_NAMES[r.b.sclass], r.b.people + 1, r.b.gLat, r.b.gLon, r.lengthM, r.people + 1, C[r.people].paved ? "paved" : "beaten", 2 * r.halfWidth); }
            }
            printf("13 x 13 cells round %.2f %.2f: %d settlements of the %s, %d of the %s; %d joined pairs within a people, %d between the two (%d walked)\n", lat / DEG, lon / DEG, byPeople[0], L[0].people.c_str(), byPeople[1], L[1].people.c_str(), within, between, walked);
        }
        return 0;
    }
    int want = argc > 2 ? atoi(argv[2]) : 6;
    int civ = 0, two = 0; double t0 = nowSec();
    for (int64_t x = 150; x < 300 && two < want; x++) for (int64_t z = 20; z < 120 && two < want; z++) {
        Star s; StarSystem sys; if (!systemAt(x, 0, z, s, sys, false)) continue;
        for (int bi = 0; bi < (int)sys.bodies.size() && two < want; bi++) {
            const Body& b = sys.bodies[bi];
            if (!kindHas(b.type, KT_PEOPLE)) continue;   // P-04: every people's world (a scorched one's too)
            BodyGen g = BodyGen::make(b);
            int np = peoplesOf(g); if (np == 0) continue;
            civ++; if (np < 2) continue;
            two++;
            Star full; starInSector(x, 0, z, full, true); StarSystem fs; fs.generate(full); const Body& fb = fs.bodies[bi];
            Lore A = loreQuick(fs, fb, g, 0), B = loreQuick(fs, fb, g, 1);
            Culture ca = cultureOf(g, 0), cb = cultureOf(g, 1);
            Vec3 n = peopleDivide(g), pa = peopleSidePoint(g, 0, 35 * DEG), pb = peopleSidePoint(g, 1, 35 * DEG);
            double la, lo, lb, lob, fla = 0, flo = 0; StarSystem::latLonFromBody(pa, la, lo); StarSystem::latLonFromBody(pb, lb, lob); bool fr = frontierPoint(g, fla, flo);
            printf("%lld 0 %lld body %d: %s, %s of %s: the %s (style %d, the end %.0f years ago, walls %s/%s) and the %s (style %d, %.0f years ago, walls %s/%s), %s; the divide's pole %.2f %.2f %.2f crosses land at %d of 48 points; the first's side at %.1f %.1f, the second's at %.1f %.1f; %s\n",
                   (long long)x, (long long)z, bi, fb.name.c_str(), KINDS[fb.type].name, full.name.c_str(), A.people.c_str(), A.style, ca.ageYears, ca.style ? "striated" : "smooth", ca.family ? "adobe" : "rock",
                   B.people.c_str(), B.style, cb.ageYears, cb.style ? "striated" : "smooth", cb.family ? "adobe" : "rock", peoplesEndedTogether(g) ? "ended together" : "ended apart", n.x, n.y, n.z, divideLandPoints(g), la / DEG, lo / DEG, lb / DEG, lob / DEG,
                   fr ? fmt("the frontier at %.1f %.1f", fla / DEG, flo / DEG).c_str() : "no frontier on land");
        }
    }
    printf("%d civilisation worlds scanned until %d of two peoples were found (%.0f%%), in %.1f s\n", civ, two, civ ? 100.0 * two / civ : 0.0, nowSec() - t0);
    return 0;
}
