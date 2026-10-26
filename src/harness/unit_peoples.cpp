// E-03: the unit's peoples by their cultures, ages and stories (groups `cultures`, `profiles`, `story`)
#include "harness/harness.h"
#include "harness/test_worlds.h"

// C-13, C-14: two peoples to a world, lending
void unitCultures(const Check& check) {
    {   // C-13: cultures per world. Of the civilisation worlds of the scan near home about one in five had two peoples (the share in
        // bounds, a few found); on the test world of two (Eleinewai I, 152 0 92 body 0: the Ghotokhor and the Khokurt) the peoples' seeds,
        // names and tongues differ, each has a culture, a voice, a tradition and a fifty of its own (the same twice, clean, distinct),
        // each names the other in a few shards, each has one last recording, and they ended together or apart as the world's coin says;
        // the one-people test worlds are read as one people with no other and the first people's seed (nothing of C-02..C-12 moved there).
        // On the ground: the divide is a unit pole found the same twice, the settlements of a region deep on each side are that people's
        // (the cell and `peopleAt` agree) with shards from its fifty, and at the frontier the roads between the two peoples' settlements
        // are built as the bigger settlement's people built
        int civ = 0, two = 0; int64_t fx = -1, fz = -1; int fbi = -1;
        for (int64_t x = 150; x < 300 && two < 12; x++) for (int64_t z = 20; z < 120 && two < 12; z++) {
            Star s; StarSystem sys; if (!systemAt(x, 0, z, s, sys, false)) continue;
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (!kindHas(b.type, KT_PEOPLE)) continue;   // P-04: every people's world (a scorched one's too)
                int np = peoplesOf(BodyGen::make(b)); if (!np) continue;
                civ++; if (np > 1) { two++; if (fx < 0) { fx = x; fz = z; fbi = bi; } }
            }
        }
        check("cultures per world: about one civilisation world in five had two peoples (C-13)", civ >= 20 && two >= 3 && two * 100 >= civ * 8 && two * 100 <= civ * 35,
              fmt("%d of %d civilisation worlds scanned (%.0f%%), the first at %lld 0 %lld body %d", two, civ, civ ? 100.0 * two / civ : 0.0, (long long)fx, (long long)fz, fbi));
        Star s; StarSystem sys; bool have = starInSector(152, 0, 92, s, true);
        bool isTwo = false, seedsDiffer = false, namesDiffer = false, tonguesDiffer = false, own = false, determ = true, clean = true, otherNamed[2] = {false, false}, lastOne[2] = {false, false}, endsAgree = false, divideUnit = false, divideSame = false, oneOk = true;
        int dups = 0;   // a template without a slot that varies can give a people the same text twice (the Khokurt's work song): two such at most a fifty
        int sidesOk = 0, sidesBad = 0, shardsIn = 0, shardsOut = 0, between = 0, walked = 0, builtRight = 0, settle[2] = {0, 0}; std::string otherLine, names, fr = "no frontier";
        if (have) {
            sys.generate(s); const Body& b = sys.bodies[0]; BodyGen g = BodyGen::make(b);
            isTwo = peoplesOf(g) == 2;
            if (isTwo) {
                Lore L[2] = {loreOf(sys, b, g, 0), loreOf(sys, b, g, 1)}; Culture C[2] = {cultureOf(g, 0), cultureOf(g, 1)};
                names = fmt("the %s and the %s, %s", L[0].people.c_str(), L[1].people.c_str(), peoplesEndedTogether(g) ? "ended together" : "ended apart");
                seedsDiffer = L[0].seed != L[1].seed && L[0].seed == g.seed && L[1].seed == peopleGen(g, 1).seed && L[0].which == 0 && L[1].which == 1 && L[0].peoples == 2 && L[1].peoples == 2;
                namesDiffer = L[0].people != L[1].people && L[0].other == L[1].people && L[1].other == L[0].people;
                Tongue T[2] = {tongueOf(g, L[0]), tongueOf(g, L[1])}; tonguesDiffer = T[0].seed != T[1].seed;
                Voice V[2] = {voiceOf(g, L[0], T[0]), voiceOf(g, L[1], T[1])}; Tradition Tr[2] = {traditionOf(g, L[0]), traditionOf(g, L[1])};
                own = V[0].seed != V[1].seed && Tr[0].seed != Tr[1].seed && C[0].people == 0 && C[1].people == 1;
                endsAgree = peoplesEndedTogether(g) ? (C[0].ageYears == C[1].ageYears && L[0].otherFate == 0 && L[1].otherFate == 0) : (C[0].ageYears != C[1].ageYears && L[0].otherFate != 0 && L[0].otherFate == -L[1].otherFate);
                for (int k = 0; k < 2; k++) {
                    std::vector<Shard> sh, sh2; shardsOf(sys, b, g, L[k], SHARDS_PER_WORLD, sh); shardsOf(sys, b, g, L[k], SHARDS_PER_WORLD, sh2);
                    std::set<std::string> texts; int ends = 0, mentions = 0;
                    for (int i = 0; i < SHARDS_PER_WORLD; i++) {
                        if (sh[i].text != sh2[i].text || sh[i].seed != sh2[i].seed) determ = false;
                        if (sh[i].tone == ST_END) ends++;
                        if (sh[i].music) continue;
                        if (sh[i].text.find_first_of("{}[]|") != std::string::npos) clean = false;
                        if (!texts.insert(sh[i].text).second) dups++;
                        if (sh[i].text.find(L[k].other) != std::string::npos) mentions++;
                    }
                    otherNamed[k] = mentions >= 1; lastOne[k] = ends == 1 && L[k].last >= 0 && sh[L[k].last].last && sh[L[k].last].year == L[k].spanYears + 1;
                    otherLine += fmt("%sthe %s name the %s in %d shards", k ? "; " : "", L[k].people.c_str(), L[k].other.c_str(), mentions);
                }
                Vec3 n = peopleDivide(g), n2 = peopleDivide(g); divideUnit = std::fabs(length(n) - 1) < 1e-9; divideSame = n.x == n2.x && n.y == n2.y && n.z == n2.z;
                for (int k = 0; k < 2; k++) {   // each side's region (T-01: round a town of its people's plan)
                    Vec3 u = peopleSidePoint(g, k, 20 * DEG); double la, lo; StarSystem::latLonFromBody(u, la, lo); planPlacePoint(g, k, la, lo);
                    int gl, gn; ruinCellOf(g, la, lo, gl, gn); DrainageOff off;
                    for (int dl = -3; dl <= 3; dl++) for (int dn = -3; dn <= 3; dn++) {
                        RuinSpec sp; if (!ruinOfCell(g, gl + dl, gn + dn, sp, true)) continue;
                        settle[k]++;
                        if (sp.people == k && peopleAt(g, sp.lat, sp.lon) == k) sidesOk++; else sidesBad++;
                        std::vector<ShardSite> ss; recordSitesOf(g, sp, C[sp.people], ss);
                        for (const ShardSite& x : ss) { if (shardPeopleOf(x.index) == sp.people) shardsIn++; else shardsOut++; }
                    }
                }
                double fla, flo;
                if (frontierPoint(g, fla, flo)) {   // the roads between the peoples
                    fr = fmt("the frontier at %.2f %.2f", fla / DEG, flo / DEG);
                    int gl, gn; ruinCellOf(g, fla, flo, gl, gn); RoadNodeCache nc; nc.reset(&g); std::vector<RoadPair> pairs;
                    for (int dl = -4; dl <= 4; dl++) for (int dn = -4; dn <= 4; dn++) nc.pairsOf(gl + dl, gn + dn, pairs);
                    std::sort(pairs.begin(), pairs.end(), [](const RoadPair& x, const RoadPair& y) { return x.id < y.id; });
                    pairs.erase(std::unique(pairs.begin(), pairs.end(), [](const RoadPair& x, const RoadPair& y) { return x.id == y.id; }), pairs.end());
                    for (const RoadPair& p : pairs) {
                        if (p.a.people == p.b.people) continue;
                        between++;
                        Road r; if (!roadWay(g, C[roadPeopleOf(p.a, p.b)], p.a, p.b, r)) continue;
                        walked++;
                        int big = settlementRank(p.b.sclass) > settlementRank(p.a.sclass) ? p.b.people : (settlementRank(p.a.sclass) > settlementRank(p.b.sclass) ? p.a.people : (r.a.people));
                        if (r.people == big && r.halfWidth >= C[big].roadHalf - 1e-9) builtRight++;
                    }
                }
            }
        }
        // R-413: a people names the other in its records where their histories met (H-02's contact): Eleinewai I's two prehistoric peoples
        // never did (their lands apart, neither sailed), Leanmya V-a's medieval ones did (two treaties, nine wars)
        bool metHere = false, metThere = false, namedThere[2] = {false, false}; std::string thereLine = "Leanmya V-a NOT two peoples";
        if (have && isTwo) { std::shared_ptr<const History> H = historyOfWorld(BodyGen::make(sys.bodies[0]), 0); if (H) for (const HistoryEvent& e : H->events) if (e.kind == HE_CONTACT) metHere = true; }
        {
            Star ms; StarSystem msys;
            if (starInSector(179, 0, 57, ms, true)) {
                msys.generate(ms);
                const BodyGen mg = 18 < (int)msys.bodies.size() ? BodyGen::make(msys.bodies[18]) : BodyGen();
                if (18 < (int)msys.bodies.size() && peoplesOf(mg) == 2) {
                    const Body& mb = msys.bodies[18];
                    std::shared_ptr<const History> H = historyOfWorld(mg, 0); if (H) for (const HistoryEvent& e : H->events) if (e.kind == HE_CONTACT) metThere = true;
                    thereLine = fmt("Leanmya V-a's %s:", metThere ? "met" : "NEVER MET");
                    for (int k = 0; k < 2; k++) {
                        const Lore ML = loreOf(msys, mb, mg, k);
                        std::vector<Shard> sh; shardsOf(msys, mb, mg, ML, SHARDS_PER_WORLD, sh);
                        int m = 0; for (const Shard& x : sh) if (!x.music && x.text.find(ML.other) != std::string::npos) m++;
                        namedThere[k] = m >= 1;
                        thereLine += fmt("%s the %s name the %s in %d shards", k ? "," : "", ML.people.c_str(), ML.other.c_str(), m);
                    }
                }
            }
        }
        {   // the one-people test worlds: one people, no other, the world's own seed, the "other" kinds never drawn
            struct W { int64_t sx, sz; int bi; } ws[2] = {{151, 25, 1}, {153, 70, 2}};
            for (const W& w : ws) {
                Star ts; StarSystem tsys; if (!systemAt(w.sx, 0, w.sz, ts, tsys, true)) { oneOk = false; continue; } const Body& tb = tsys.bodies[w.bi]; BodyGen tg = BodyGen::make(tb);
                Lore L = loreOf(tsys, tb, tg), L2 = loreOf(tsys, tb, tg, 1);
                if (peoplesOf(tg) != 1 || L.peoples != 1 || L.which != 0 || !L.other.empty() || L.seed != tg.seed || L2.seed != L.seed || L2.which != 0 || peopleAt(tg, 0.3, 0.7) != 0) oneOk = false;
                std::vector<Shard> sh; shardsOf(tsys, tb, tg, L, SHARDS_PER_WORLD, sh);
                for (const Shard& x : sh) if (x.text.find("the other people") != std::string::npos || (x.kind >= 0 && std::string(shardKindName(x.kind)) == "the other people")) oneOk = false;
            }
        }
        check("a world of two peoples: two lores, tongues, voices, traditions and fifties, each naming the other where they met, one last each, the ends by the coin; one-people worlds as they were (C-13)",
              have && isTwo && seedsDiffer && namesDiffer && tonguesDiffer && own && determ && clean && dups <= 4 && otherNamed[0] == metHere && otherNamed[1] == metHere && metThere && namedThere[0] && namedThere[1] && lastOne[0] && lastOne[1] && endsAgree && oneOk,
              fmt("Eleinewai I: %s; seeds %s, names %s, tongues %s, voices and traditions %s; the fifties %s, %s, %d texts repeated in the hundred; they %s: %s; %s; one last each %s; the ends %s; the one-people worlds %s", isTwo ? names.c_str() : "NOT two peoples", seedsDiffer ? "differ" : "NOT as drawn", namesDiffer ? "cross-named" : "NOT cross-named",
                  tonguesDiffer ? "differ" : "the SAME", own ? "their own" : "NOT their own", determ ? "the same twice" : "NOT the same twice", clean ? "clean" : "NOT clean", dups, metHere ? "met" : "never met", otherLine.c_str(), thereLine.c_str(), lastOne[0] && lastOne[1] ? "yes" : "NO", endsAgree ? "as the coin says" : "NOT as the coin says", oneOk ? "as they were" : "CHANGED"));
        // T-01: the two peoples' lands meet only where their plans' territories touch; where they do not (their capitals far apart, each holding
        // a share of the world's land) there is no frontier and its roads are not asked for: the rule is checked on two settlements made up
        bool meet = false, ruleOk = false;
        if (have && isTwo) {
            const BodyGen g = BodyGen::make(sys.bodies[0]);
            std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
            for (int c = 0; c < P->grid.cells && !meet; c++) {
                if (!P->plans[0].territory[c]) continue;
                int nb[8]; const int k = P->grid.neighbours(c, nb);
                for (int i = 0; i < k; i++) if (P->plans[1].territory[nb[i]]) meet = true;
            }
            RoadNode a, b; a.people = 0; a.sclass = SC_VILLAGE; b.people = 1; b.sclass = SC_TOWN;
            RoadNode a2 = a, b2 = b; a2.sclass = SC_TOWN; b2.sclass = SC_HAMLET;
            ruleOk = roadPeopleOf(a, b) == 1 && roadPeopleOf(b, a) == 1 && roadPeopleOf(a2, b2) == 0 && roadPeopleOf(b2, a2) == 0;
            if (!meet) fr = "the territories do not meet: no frontier";
        }
        check("a world of two peoples on the ground: the divide, each side's settlements its people's with shards of its fifty, the frontier's roads the bigger settlement's build (C-13)",
              have && isTwo && divideUnit && divideSame && sidesOk >= 10 && sidesBad == 0 && settle[0] >= 3 && settle[1] >= 3 && shardsIn >= 5 && shardsOut == 0 && ruleOk && (!meet || (between >= 1 && walked >= 1 && builtRight == walked)),
              fmt("the divide %s; %d and %d settlements deep on the sides, %d their people's, %d NOT; %d shard sites of their people's fifty, %d NOT; %s: %d pairs between the peoples, %d walked, %d built as the bigger settlement's people built", divideUnit && divideSame ? "a unit pole, the same twice" : "NOT right",
                  settle[0], settle[1], sidesOk, sidesBad, shardsIn, shardsOut, (fr + (ruleOk ? "; the rule on two made up: the bigger's people" : "; the rule NOT the bigger's")).c_str(), between, walked, builtRight));
    }
    {   // C-13: the decoder and the ground on a world of two peoples. With three shards of each people of Eleinewai I and the second
        // people's last recording in the guide the decoder lists the world once a people, the second's list holds its four alone, its
        // last recording read at that count ends the second's record under its own key and leaves the first's open; landed at the
        // frontier, a shard taken from a settlement is counted against the world's hundred
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFrames(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        Star ws; StarSystem wsys; bool haveWorld = starInSector(152, 0, 92, ws, true); int li = -1;
        if (haveWorld) { wsys.generate(ws); if (!wsys.bodies.empty()) li = lastShardOf(peopleGen(BodyGen::make(wsys.bodies[0]), 1)); }
        for (int i = 0; i < 3; i++) { game.guide.shards.insert("152,0,92/0/S" + std::to_string(i)); game.guide.shards.insert("152,0,92/0/S" + std::to_string(shardWorldIndex(1, i))); }   // T-04: the second people's at 1000 up
        if (li >= 0) game.guide.shards.insert("152,0,92/0/S" + std::to_string(shardWorldIndex(1, li)));
        game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1);
        bool atDecoder = game.state == GameState::SHARDS && game.testShardsLevel() == 0;
        std::string list = game.testShardsInfo(); bool twoEntries = list.find("2 worlds") != std::string::npos;
        press(KEY_DOWN); run(0.05); press(KEY_ENTER); run(0.1);
        std::string second = game.testShardsInfo(); bool secondOpen = second.find("4 held of 152,0,92/0 (people 2 of 2, 0 lent)") != std::string::npos;
        writePNG("shots/tests/unit_peoples_decoder.png", game.output(), FBW, FBH);   // the second people's list, the people named in the header
        bool opened = li >= 0 && game.testShardsOpenIndex(li); run(0.3); press(KEY_ENTER); run(0.2);   // Enter skips the wait: read
        std::string key = "152,0,92/0/S" + std::to_string(shardWorldIndex(1, li));
        bool read = game.guide.decoded.count(key) && game.guide.decoded[key] == 4;
        bool ended = game.guide.ended.count("152,0,92/0/P1") && !game.guide.ended.count("152,0,92/0");
        bool logged = false; for (const LogEntry& e : game.guide.log) if (e.kind == "SHARD" && e.text.find("THE LAST RECORDING OF THE") != std::string::npos) logged = true;
        press(KEY_ESCAPE); press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
        // the ground: landed at the frontier, the nearest shard taken
        bool landed = false, walked = false, taken = false, hundred = false; int peoplesSeen = 0, takenIndex = -1; std::string takeStatus;
        if (haveWorld) {
            game.testParkAt(ws, 0); run(0.2);
            double la = 0.0, lo = 47.33 * DEG; planPlacePoint(BodyGen::make(wsys.bodies[0]), 1, la, lo);   // T-01: a town of the second people's plan
            game.testLandCursor(la / DEG, lo / DEG); run(0.1);
            press(KEY_ENTER);
            for (int i = 0; i < 30 * 20 && !(game.state == GameState::SURFACE && game.testPlayerAlt() < 3); i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); }
            landed = game.state == GameState::SURFACE;
            if (landed) {
                peoplesSeen = game.testPeoples();
                if ((walked = game.testWalkToShard())) {
                    run(0.3); takenIndex = game.testNearShardIndex();
                    size_t before = game.guide.shards.size();
                    press(KEY_E); run(0.2);
                    taken = game.guide.shards.size() == before + 1 && takenIndex >= 0 && shardPeopleOf(takenIndex) <= 1;
                    takeStatus = game.testStatus(); hundred = haveWorld && takeStatus.find(fmt("OF %d", shardsOfWorld(BodyGen::make(wsys.bodies[0])))) != std::string::npos;   // T-04: both peoples' records
                }
            }
        }
        check("a world of two peoples on the decoder and the ground: an entry a people, the second's list its own, its last ends its own record; a shard taken counts against both peoples' records (C-13)",
              haveWorld && atDecoder && twoEntries && secondOpen && opened && read && ended && logged && landed && peoplesSeen == 2 && walked && taken && hundred,
              fmt("the list %s; the second entry %s; its last S%d %s, read %s, ended %s (the first's open), logged %s; landed %s with %d peoples, at a shard %s, taken %s (S%d), '%s'", twoEntries ? "has two entries" : trunc(list, 60).c_str(), secondOpen ? "is the second people's four" : trunc(second, 70).c_str(),
                  li, opened ? "opened" : "NOT opened", read ? "yes" : "NO", ended ? "yes" : "NO", logged ? "yes" : "NO", landed ? "yes" : "NO", peoplesSeen, walked ? "yes" : "NO", taken ? "yes" : "NO", takenIndex, trunc(takeStatus, 60).c_str()));
        remove("shots/tests/test_guide.txt");
    }
    {   // C-14: lending. A friend's guide (four shards of Aieliaalas II taken, two of them and the first piece read, the piece and the star
        // named; a shard lent to the friend by a third explorer and read) saved beside the test guide as its inbox file; a fresh guide that
        // took one shard of its own (S2, which the friend also took and left unread) imports it: the friend's names go into the inbox, the
        // shards they found and read are lent (S0 and the piece; not the unread S3, not the explorer's own S2, not the one lent to the
        // friend), the import is logged; the decoder lists the world with
        // the lent and the own together, counts the lent in the language, plays the friend's piece under the friend's name, reads a lent
        // text at that count; a lent shard taken afterwards is the explorer's own and the statistics count one lent
        Star ws; StarSystem wsys; bool haveWorld = starInSector(151, 0, 25, ws, true); int mi = -1;
        if (haveWorld) { wsys.generate(ws); if (wsys.bodies.size() > 1) { BodyGen g = BodyGen::make(wsys.bodies[1]); for (int i = 0; i < SHARDS_PER_WORLD && mi < 0; i++) if (shardIsMusic(g, i)) mi = i; } }
        const std::string wk = "151,0,25/1", mk = wk + "/S" + std::to_string(mi);
        Guide friendG;
        for (int i = 0; i < 4; i++) friendG.shards.insert(wk + "/S" + std::to_string(i));
        friendG.decoded[wk + "/S0"] = 4; friendG.decoded[wk + "/S1"] = 4;   // S2 and S3 taken, unread
        if (mi >= 0) { friendG.shards.insert(mk); friendG.decoded[mk] = SHARDS_PER_WORLD; friendG.names[mk] = "The river at night"; }
        friendG.lent.insert(wk + "/S9"); friendG.decoded[wk + "/S9"] = 5;   // lent to the friend by a third explorer and read: does not travel on
        friendG.names["151,0,25"] = "Friendstar";
        remove("shots/tests/test_guide.txt");
        bool wrote = friendG.save("shots/tests/test_guide_inbox.txt");
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        for (const Body& ob : game.testSystem().bodies) if (ob.type != PT_COMPANION) game.testWaitMapOf(ob.index);   // E-02: every map first (F-07)
        game.guide.shards.insert(wk + "/S2");   // the explorer's own already: not lent
        game.importInboxFile(); std::string st = game.testStatus();
        bool namesIn = game.guide.inbox.count("151,0,25") && game.guide.inbox["151,0,25"] == "Friendstar" && mi >= 0 && game.guide.inbox.count(mk) && game.guide.names.empty();
        std::set<std::string> want = {wk + "/S0", mk};
        bool lentRight = game.guide.lent == want;
        bool logged = false; for (const LogEntry& e : game.guide.log) if (e.kind == "INBOX") logged = true;
        int lentCount = game.guide.lentCount();
        Input gi;
        auto run = [&](double secs) { runFrames(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1);
        bool atDecoder = game.state == GameState::SHARDS && game.testShardsLevel() == 0;
        std::string list = game.testShardsInfo(); bool oneWorld = list.find("1 worlds") != std::string::npos;
        press(KEY_ENTER); run(0.1);
        std::string info = game.testShardsInfo(); bool heldRight = info.find("3 held of 151,0,25/1 (people 1 of 1, 2 lent)") != std::string::npos;
        writePNG("shots/tests/unit_lent.png", game.output(), FBW, FBH);   // the list: the lent rows in cyan, the friend's piece under its name
        bool pieceOpened = mi >= 0 && game.testShardsOpenIndex(mi); run(0.3);
        bool pieceRead = haveWorld && game.guide.decoded.count(mk) && game.guide.decoded[mk] == recordCountOf(BodyGen::make(wsys.bodies[1]), 0);   // T-04: read at the people's whole count
        writePNG("shots/tests/unit_lent_piece.png", game.output(), FBW, FBH);   // the piece's screen: LENT after the header, the friend's name in cyan
        press(KEY_ESCAPE); run(0.1);
        bool textOpened = game.testShardsOpenIndex(0); run(0.3); press(KEY_ENTER); run(0.2);   // the lent text read, the wait skipped
        bool textRead = game.guide.decoded.count(wk + "/S0") && game.guide.decoded[wk + "/S0"] == 3;
        press(KEY_ESCAPE); press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
        game.guide.shards.insert(wk + "/S0");   // taken afterwards
        bool ownNow = !game.guide.isLent(wk + "/S0") && game.guide.lentCount() == 1;
        check("lending: a friend's read shards travel with the inbox, listed and read in cyan, counted in the language, the explorer's own once taken (C-14)",
              haveWorld && mi >= 0 && wrote && namesIn && lentRight && logged && lentCount == 2 && atDecoder && oneWorld && heldRight && pieceOpened && pieceRead && textOpened && textRead && ownNow,
              fmt("'%s'; the inbox %s; lent %s (%d); logged %s; the decoder %s, %s; the piece S%d %s and %s; the text S0 %s and %s at 3; taken afterwards %s", trunc(st, 50).c_str(), namesIn ? "has the star's and the piece's names" : "LACKS the names",
                  lentRight ? "S0 and the piece alone" : "NOT as expected", lentCount, logged ? "yes" : "NO", atDecoder ? (oneWorld ? "lists the one world" : trunc(list, 40).c_str()) : "NOT reached", heldRight ? "three held with two lent" : trunc(info, 60).c_str(),
                  mi, pieceOpened ? "opened" : "NOT opened", pieceRead ? "read" : "NOT read", textOpened ? "opened" : "NOT opened", textRead ? "read" : "NOT read", ownNow ? "own, one lent left" : "NOT own"));
        remove("shots/tests/test_guide.txt"); remove("shots/tests/test_guide_inbox.txt");
    }
}

// A-01: the ages, a people's profile, only the radio ages on the air
void unitProfiles(const Check& check) {
    {   // A-01: the ages. A people's profile is one function: over the peoples within 12 ly of home, the same twice, the generator's part
        // (`peopleAgeOf`) the same as the whole's and the culture's end its end; the eras one per age from the prehistoric to the age
        // reached, joined year to year (the reckoning's year 1 the first towns', or a prehistoric people's first village), the last ending
        // with the people; radio exactly where the age had it, its year within the industrial age; the collapse within the last era and
        // ending with it, its stages in order; the end within its age's span (a second people that ended with the first takes the first's
        // years and its cause, one that can end a world); the extent and the values sane; the government the last era's
        int peoplesN = 0, bad = 0, together = 0; std::string badWhat; double tSum = 0;
        static const double E_LO[AGE_COUNT] = {200, 200, 200, 250, 400, 1000}, E_HI[AGE_COUNT] = {40000, 40000, 60000, 100000, 250000, 2000000};
        auto fail = [&](const std::string& what) { if (bad++ < 4) badWhat += what + "; "; };
        for (int64_t x = HOME_SX - 12; x <= HOME_SX + 12; x++) for (int64_t y = HOME_SY - 12; y <= HOME_SY + 12; y++) for (int64_t z = HOME_SZ - 12; z <= HOME_SZ + 12; z++) {
            if ((x - HOME_SX) * (x - HOME_SX) + (y - HOME_SY) * (y - HOME_SY) + (z - HOME_SZ) * (z - HOME_SZ) > 144) continue;
            Star s; StarSystem sys; if (!systemAt(x, y, z, s, sys, false)) continue;
            for (const Body& b : sys.bodies) {
                if (!kindHas(b.type, KT_PEOPLE)) continue;   // P-04: every people's world (a scorched one's too)
                BodyGen g = BodyGen::make(b); int np = peoplesOf(g); if (!np) continue;
                PeopleProfile both[2];
                for (int k = 0; k < np; k++) {
                    double t0 = nowSec(); PeopleProfile P = peopleProfileOf(sys, b, g, k); tSum += nowSec() - t0;
                    PeopleProfile Q = peopleProfileOf(sys, b, g, k); PeopleAge A = peopleAgeOf(g, k); Culture C = cultureOf(g, k);
                    both[k] = P; peoplesN++;
                    std::string at = fmt("%lld %lld %lld b%d p%d", (long long)x, (long long)y, (long long)z, b.index, k);
                    bool same = P.age == Q.age && P.lastYear == Q.lastYear && P.endedYearsAgo == Q.endedYearsAgo && P.radio == Q.radio && P.radioYear == Q.radioYear && P.cause == Q.cause && P.collapseYear == Q.collapseYear && P.collapseYears == Q.collapseYears
                                && P.landShare == Q.landShare && P.held == Q.held && P.peak == Q.peak && P.government == Q.government && P.economy == Q.economy && P.economy2 == Q.economy2 && P.faith == Q.faith && P.rite == Q.rite && P.holy == Q.holy && P.seed == Q.seed && P.eras.size() == Q.eras.size();
                    for (int v = 0; v < VAL_COUNT && same; v++) same = P.values[v] == Q.values[v];
                    for (int st = 0; st < STAGE_COUNT && same; st++) same = P.stage[st] == Q.stage[st];
                    for (size_t e = 0; e < P.eras.size() && same; e++) same = P.eras[e].age == Q.eras[e].age && P.eras[e].start == Q.eras[e].start && P.eras[e].end == Q.eras[e].end && P.eras[e].government == Q.eras[e].government;
                    if (!same) fail(at + " not the same twice");
                    if (A.age != P.age || A.endedYearsAgo != P.endedYearsAgo || A.lastYear != P.lastYear || A.eras.size() != P.eras.size() || A.radio != P.radio) fail(at + " the generator's part differs");
                    if (C.ageYears != P.endedYearsAgo || C.age != P.age || C.weatherYears != P.endedYearsAgo / ageDurability(P.age)) fail(at + " the culture's end differs");
                    if ((int)P.eras.size() != P.age + 1) { fail(at + fmt(" %zu eras for the %s age", P.eras.size(), AGE_NAMES[P.age])); continue; }
                    for (int e = 0; e < (int)P.eras.size(); e++) {
                        const Era& E = P.eras[e];
                        if (E.age != e || E.start > E.end || (e > 0 && E.start != P.eras[e - 1].end + 1) || E.government < 0 || E.government >= GOV_COUNT) fail(at + fmt(" era %d %s %d..%d", e, AGE_NAMES[E.age], E.start, E.end));
                    }
                    if (P.eras.back().end != P.lastYear || (P.age == AGE_PREHISTORIC ? P.eras[0].start != 1 : (P.eras[0].end != 0 || P.eras[1].start != 1))) fail(at + " the reckoning");
                    bool radioWant = P.age > AGE_INDUSTRIAL || (P.age == AGE_INDUSTRIAL && P.radio);
                    if (P.radio != radioWant || (P.radio && (P.radioYear < P.eras[AGE_INDUSTRIAL].start || P.radioYear > P.eras[AGE_INDUSTRIAL].end))) fail(at + " radio");
                    if (P.collapseYears < 1 || P.collapseYear < P.eras.back().start || P.collapseYear + P.collapseYears - 1 != P.lastYear) fail(at + fmt(" the collapse %d over %d", P.collapseYear, P.collapseYears));
                    if (!(P.stage[STAGE_WARNING] == 0 && P.stage[STAGE_ONSET] > 0 && P.stage[STAGE_CRISIS] > P.stage[STAGE_ONSET] && P.stage[STAGE_END] > P.stage[STAGE_CRISIS] && P.stage[STAGE_END] < 1)) fail(at + " the stages");
                    bool stagesUp = true; for (int yr = P.collapseYear + 1; yr <= P.lastYear; yr++) if (P.stageAt(yr) < P.stageAt(yr - 1)) stagesUp = false;
                    if (P.stageAt(P.collapseYear - 1) != -1 || P.stageAt(P.collapseYear) < STAGE_WARNING || !stagesUp || P.stageAt(P.lastYear) != STAGE_END || (P.collapseYears >= 60 && P.stageAt(P.collapseYear) != STAGE_WARNING) || P.eraAt(P.lastYear) != P.age || (P.age > 0 && P.eraAt(P.eras.back().start - 1) != P.age - 1))
                        fail(at + fmt(" stageAt/eraAt (the collapse %d over %d: %d %d)", P.collapseYear, P.collapseYears, P.stageAt(P.collapseYear), P.stageAt(P.lastYear)));
                    bool sharedEnd = k > 0 && peoplesEndedTogether(g);
                    if (!sharedEnd && (P.endedYearsAgo < E_LO[P.age] * 0.999 || P.endedYearsAgo > E_HI[P.age] * 1.001)) fail(at + fmt(" ended %.0f years ago", P.endedYearsAgo));
                    double vs = 0; for (int v = 0; v < VAL_COUNT; v++) vs += P.values[v];
                    if (!(P.landShare > 0 && P.landShare <= 1 && P.held > 0 && P.held <= 1 && P.peak >= 1 && std::fabs(vs - 1) < 1e-9 && P.government == P.eras.back().government && P.economy != P.economy2)) fail(at + " the extent or the rest");
                    if (P.cause < 0 || P.cause >= CAUSE_COUNT) fail(at + " no cause");
                }
                if (np > 1 && peoplesEndedTogether(g)) {
                    together++;
                    if (both[0].endedYearsAgo != both[1].endedYearsAgo || both[0].cause != both[1].cause || !causeWorldwide(both[0].cause) || both[0].collapseYears != both[1].collapseYears) fail(fmt("%lld %lld %lld b%d ended together apart", (long long)x, (long long)y, (long long)z, b.index));
                }
            }
        }
        check("ages: a people's profile is one function, the same twice, its eras, radio, collapse and end in order (A-01)", peoplesN >= 100 && together >= 3 && bad == 0,
              fmt("%d peoples within 12 ly (%d worlds of two that ended together), a profile %.2f ms; %d wrong%s%s", peoplesN, together, peoplesN ? 1e3 * tSum / peoplesN : 0.0, bad, bad ? ": " : "", badWhat.c_str()));
    }
    {   // A-01: the shares as decided over the peoples within 25 ly of home (the generator's part alone), the early ages commoner and the far
        // future rarest, about three peoples in ten with radio; the test worlds of each age where they were pinned
        int ages[AGE_COUNT] = {0}, n = 0, radio = 0; double t0 = nowSec();
        for (int64_t x = HOME_SX - 25; x <= HOME_SX + 25; x++) for (int64_t y = HOME_SY - 25; y <= HOME_SY + 25; y++) for (int64_t z = HOME_SZ - 25; z <= HOME_SZ + 25; z++) {
            if ((x - HOME_SX) * (x - HOME_SX) + (y - HOME_SY) * (y - HOME_SY) + (z - HOME_SZ) * (z - HOME_SZ) > 625) continue;
            Star s; StarSystem sys; if (!systemAt(x, y, z, s, sys, false)) continue;
            for (const Body& b : sys.bodies) {
                if (!kindHas(b.type, KT_PEOPLE)) continue;   // P-04: every people's world (a scorched one's too)
                BodyGen g = BodyGen::make(b); int np = peoplesOf(g);
                for (int k = 0; k < np; k++) { PeopleAge A = peopleAgeOf(g, k); ages[A.age]++; n++; if (A.radio) radio++; }
            }
        }
        double secs = nowSec() - t0;
        bool shares = n >= 1000, order = ages[AGE_MEDIEVAL] > ages[AGE_INDUSTRIAL] && ages[AGE_PREHISTORIC] > ages[AGE_INDUSTRIAL] && ages[AGE_INDUSTRIAL] > ages[AGE_MODERN] && ages[AGE_MODERN] > ages[AGE_FUTURISTIC] && ages[AGE_FUTURISTIC] > ages[AGE_FAR_FUTURE] && ages[AGE_FAR_FUTURE] > 0;
        std::string line;
        for (int a = 0; a < AGE_COUNT; a++) { double sh = (double)ages[a] / std::max(1, n); if (std::fabs(sh - AGE_SHARES[a]) > 0.03) shares = false; line += fmt("%s%s %.1f%%", a ? ", " : "", AGE_NAMES[a], 100 * sh); }
        int pinned = 0; std::string pins;
        for (int a = 0; a < AGE_COUNT; a++) {
            const AgeWorld& w = AGE_WORLDS[a]; Star s;
            if (w.body < 0 || !starInSector(w.sx, 0, w.sz, s, false)) { pins += fmt(" %s: no star;", AGE_NAMES[a]); continue; }
            StarSystem sys; sys.generate(s);
            if (w.body >= (int)sys.bodies.size()) { pins += fmt(" %s: no body;", AGE_NAMES[a]); continue; }
            BodyGen g = BodyGen::make(sys.bodies[w.body]);
            if (peoplesOf(g) > w.which && peopleAgeOf(g, w.which).age == a) pinned++; else pins += fmt(" %s: %lld %lld body %d is not;", AGE_NAMES[a], (long long)w.sx, (long long)w.sz, w.body);
        }
        double radioShare = (double)radio / std::max(1, n);
        check("ages: the shares as decided, the far future rarest, about three in ten with radio; a test world of each age where pinned (A-01)", shares && order && radioShare > 0.22 && radioShare < 0.38 && pinned == AGE_COUNT,
              fmt("%d peoples within 25 ly in %.2f s: %s; radio %.1f%%; %d of %d test worlds hold their age%s", n, secs, line.c_str(), 100 * radioShare, pinned, AGE_COUNT, pins.c_str()));
    }
    {   // A-01: only the radio ages on the air. Every candidate sector within 200 ly of home whose star has a people's world: its signal is
        // there exactly when the world's people (either of two) had radio, and every people's signal heard from home is such a world's;
        // home hears two to six. A world of two peoples broadcasts only those that had radio (every slot's people one of them, both where
        // both had it); a chart marks a transmission heard only where the people charting had radio and the star's people are on the air
        Vec3 home((HOME_SX + 0.5) * SECTOR_KM, (HOME_SY + 0.5) * SECTOR_KM, (HOME_SZ + 0.5) * SECTOR_KM);
        std::vector<Signal> sigs; signalsNear(home, sigs);
        int heard = 0, wrongHeard = 0, candidates = 0, onAir = 0, silent = 0, wrongRule = 0; std::string ages;
        for (const Signal& sg : sigs) {
            if (sg.kind != SIG_PEOPLE) continue;
            heard++;
            Star full; starInSector(sg.star.sx, sg.star.sy, sg.star.sz, full, false); StarSystem sys; sys.generate(full);
            BodyGen g = BodyGen::make(sys.bodies[sg.body]); int mask = peoplesOnAir(g);
            if (!mask) wrongHeard++;
            else { int k = (mask & 1) ? 0 : 1; ages += fmt("%s%.0f ly %s", ages.empty() ? "" : ", ", sg.distLy, AGE_NAMES[peopleAgeOf(g, k).age]); if (peopleAgeOf(g, k).age < AGE_INDUSTRIAL) wrongHeard++; }
        }
        const int64_t R = 200;
        auto fd = [](int64_t a, int64_t b) { int64_t q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) q--; return q; };
        for (int64_t cx = fd(HOME_SX - R, SIGNAL_CELL); cx <= fd(HOME_SX + R, SIGNAL_CELL); cx++) for (int64_t cy = fd(HOME_SY - R, SIGNAL_CELL); cy <= fd(HOME_SY + R, SIGNAL_CELL); cy++) for (int64_t cz = fd(HOME_SZ - R, SIGNAL_CELL); cz <= fd(HOME_SZ + R, SIGNAL_CELL); cz++) {
            int64_t sx[SIGNAL_MAX_CANDIDATES], sy[SIGNAL_MAX_CANDIDATES], sz[SIGNAL_MAX_CANDIDATES]; int nc = transmitterCell(cx, cy, cz, sx, sy, sz);
            for (int k = 0; k < nc; k++) {
                Star s; if (!starInSector(sx[k], sy[k], sz[k], s, false) || s.cls == STAR_PULSAR) continue;
                if (length(s.pos - home) > R * SECTOR_KM) continue;
                StarSystem sys; sys.generate(s);
                const int pw = systemPeoplesWorld(sys);   // P-04: the system's people's world (C-01's first, else a dead world's)
                if (pw >= 0) {
                    const Body& b = sys.bodies[pw];
                    BodyGen g = BodyGen::make(b);
                    candidates++;
                    Signal sg; bool has = starSignal(s, sg); bool want = peoplesOnAir(g) != 0;
                    if (has != want || (has && sg.body != b.index)) wrongRule++;
                    if (want) onAir++; else silent++;
                }
            }
        }
        // the slots of the worlds of two peoples within 25 ly: every slot's people on the air
        int twoWorlds = 0, oneOnAir = 0, bothOnAir = 0, slotsWrong = 0; bool bothSeen = true;
        for (int64_t x = HOME_SX - 25; x <= HOME_SX + 25; x++) for (int64_t y = HOME_SY - 25; y <= HOME_SY + 25; y++) for (int64_t z = HOME_SZ - 25; z <= HOME_SZ + 25; z++) {
            if ((x - HOME_SX) * (x - HOME_SX) + (y - HOME_SY) * (y - HOME_SY) + (z - HOME_SZ) * (z - HOME_SZ) > 625) continue;
            Star s; StarSystem sys; if (!systemAt(x, y, z, s, sys, false)) continue;
            for (const Body& b : sys.bodies) {
                if (!kindHas(b.type, KT_PEOPLE)) continue;   // P-04: every people's world (a scorched one's too)
                BodyGen g = BodyGen::make(b); if (peoplesOf(g) < 2) continue;
                twoWorlds++;
                int mask = peoplesOnAir(g); if (mask == 1 || mask == 2) oneOnAir++; if (mask == 3) bothOnAir++;
                int seen = 0;
                for (int slot = 0; slot < SHARDS_PER_WORLD; slot++) {
                    int w = slotPeople(g, b.seed, slot);
                    if (mask == 0 ? w != -1 : (w < 0 || !((mask >> w) & 1))) slotsWrong++;
                    if (w >= 0) seen |= 1 << w;
                }
                if (mask == 3 && seen != 3) bothSeen = false;
            }
        }
        // the charts of the two test worlds and the pinned ones: a heard mark only from a people with radio, to a world on the air
        int heardMarks = 0, marksWrong = 0, chartWorlds = 0;
        {
            struct TW { int64_t sx, sz; int bi, which; };
            std::vector<TW> tws = {{151, 25, 1, 0}, {153, 70, 2, 0}};
            for (int a = 0; a < AGE_COUNT; a++) if (AGE_WORLDS[a].body >= 0) tws.push_back({AGE_WORLDS[a].sx, AGE_WORLDS[a].sz, AGE_WORLDS[a].body, AGE_WORLDS[a].which});
            for (const TW& t : tws) {
                Star s; StarSystem sys; if (!systemAt(t.sx, 0, t.sz, s, sys, true)) continue; if (t.bi >= (int)sys.bodies.size()) continue;
                const Body& b = sys.bodies[t.bi]; BodyGen g = BodyGen::make(b); if (peoplesOf(g) <= t.which) continue;
                chartWorlds++;
                Lore L = loreOf(sys, b, g, t.which); bool listened = peopleAgeOf(g, t.which).radio;
                for (const ChartMark& m : L.marks) {
                    if (m.how != 2) continue;
                    heardMarks++;
                    Star ms; starInSector(m.star.sx, m.star.sy, m.star.sz, ms, false); StarSystem msys; msys.generate(ms);
                    bool onAirThere = m.body >= 0 && m.body < (int)msys.bodies.size() && peoplesOnAir(BodyGen::make(msys.bodies[m.body])) != 0;
                    if (!listened || !onAirThere || !sectorTransmits(m.star.sx, m.star.sy, m.star.sz)) marksWrong++;
                }
            }
        }
        check("ages: only the radio ages on the air: a transmitter's people had radio, a candidate without is silent; two peoples' slots, the charts' heard marks (A-01)",
              heard >= 2 && heard <= 6 && wrongHeard == 0 && wrongRule == 0 && onAir >= heard && silent >= 1 && twoWorlds >= 50 && oneOnAir >= 5 && slotsWrong == 0 && bothSeen && chartWorlds >= 6 && marksWrong == 0,
              fmt("from home %d peoples' signals (%s), %d wrong; within 200 ly %d candidate worlds: %d on the air, %d silent, %d against the rule; %d worlds of two within 25 ly (%d with one on the air, %d with both): %d slots wrong%s; %d charted worlds, %d heard marks, %d wrong",
                  heard, ages.c_str(), wrongHeard, candidates, onAir, silent, wrongRule, twoWorlds, oneOnAir, bothOnAir, slotsWrong, bothSeen ? "" : ", a world of two on the air with one people's slots only", chartWorlds, heardMarks, marksWrong));
    }
}

// C-02: the story grammar
void unitStory(const Check& check) {
    {   // C-02: the story grammar on the two civilisation worlds of C-01 (Aieliaalas II, Leileashphail III), then across the worlds near home;
        // H-04: the records' genres written from the history (every record, each within its genre's length; the register: the collapse late)
        struct W { int64_t sx, sz; int bi; };
        const W ws[2] = {{151, 25, 1}, {153, 70, 2}};
        int okWorlds = 0, lorePct[2] = {0, 0}, minW = 999, maxW = 0, musicN[2] = {0, 0};
        int ordinaryPct[2] = {0, 0}, collapseN[2] = {0, 0}, collapseEarly = 0, fullAt[2] = {10, 10};   // H-04
        bool determ = true, clean = true, lengths = true, distinct = true;
        bool tradSame = true, tradOk = true, piecesOk = true, pieceSame = true, formsVary = true, tradDiffer = true; int pieceMin = 999, pieceMax = 0, notesMin = 99999, noteTot = 0, badOrder = 0, badTime = 0, badVel = 0, badDeg = 0, badVoice = 0, badLen = 0, ageFamilyBad = 0; Tradition trads[2];   // C-04; H-05
        double readPct[2][11] = {}; bool mono = true, namesKnown = true, tongueSame = true, tongueDistinct = true; int langWords[2] = {0, 0};   // C-06
        bool voiceSame = true, voiceOk = true, speechSame = true, speechOk = true, aligned = true, inventoryOk = true; int badSeg = 0, badWord = 0, speechN = 0; double speechMin = 999, speechMax = 0, speechTot = 0; Voice voices[2];   // C-05
        for (int wi = 0; wi < 2; wi++) {
            Star s; StarSystem sys; if (!systemAt(ws[wi].sx, 0, ws[wi].sz, s, sys, true)) continue;
            if (ws[wi].bi >= (int)sys.bodies.size()) continue;
            const Body& b = sys.bodies[ws[wi].bi];
            BodyGen g = BodyGen::make(b);
            if (!g.hasTrait(TR_CIVILISATION)) continue;
            okWorlds++;
            Lore L = loreOf(sys, b, g);
            std::vector<Shard> sh; shardsOf(sys, b, g, L, L.records, sh);   // T-04: every record (C-02's fifty the first)
            std::shared_ptr<const ShardTexts> TT = shardTextsOf(sys, b, g, L);
            std::shared_ptr<const History> HH = historyOf(sys, b, g, 0);
            fullAt[wi] = languageFull(L.records);
            std::set<std::string> texts;
            int lore = 0, textN = 0, ordinary = 0;
            const std::string names[] = {L.people, L.god, L.river, L.mountain, L.sea, L.city, L.city2, L.founder, L.moon, L.star, L.festival};
            static const std::set<std::string> COLLAPSE = {"warning", "denial", "orders", "inventory", "whoever", "emergency", "last"};
            for (int i = 0; i < (int)sh.size(); i++) {
                const Shard& x = sh[i];
                Shard y = shardOf(sys, b, g, L, i);
                if (y.text != x.text || y.child != x.child || y.music != x.music) determ = false;
                if (x.music) { if (i < 50) musicN[wi]++; continue; }   // C-04: a piece, no text
                if (x.chart) continue;   // C-10: a caption
                textN++;
                if (x.text.find_first_of("{}[]|") != std::string::npos) clean = false;
                int nw = shardWords(x.text); minW = std::min(minW, nw); maxW = std::max(maxW, nw);
                const VgRecord& rec = TT->items[i].rec;   // H-04: within its genre's length
                if (rec.hi > 0 && (nw < rec.lo || nw > rec.hi)) lengths = false;
                texts.insert(x.text);
                bool hasName = !x.names.empty(); for (const std::string& nm : names) if (x.text.find(nm) != std::string::npos) hasName = true;
                if (hasName) lore++;
                const bool collapse = COLLAPSE.count(shardKindName(x.kind)) > 0;   // H-04: the register: the collapse's witnesses come with its years
                if (collapse) { collapseN[wi]++; if (HH && HH->profile.stageAt(x.year) < 0 && !x.last) collapseEarly++; }
                else ordinary++;
            }
            if ((int)texts.size() < textN) distinct = false;
            lorePct[wi] = 100 * lore / std::max(1, textN); ordinaryPct[wi] = 100 * ordinary / std::max(1, textN);
            {   // C-04: the tradition the same twice, its scale five to nine notes ascending with 70 cents between neighbours, its cycle's groups summing, two or three voices; the
                // pieces the same twice, 15-90 s, notes in order and within the range, their forms varied; the two worlds' traditions differ
                // H-05: an ensemble for each age of the age's own families (the hand-played ages' instruments, the machines' after them), every music
                // record played at its own age and at every other, each within the age's length, its notes on the ensemble's voices and kit
                static const int ALLOWED[AGE_COUNT][12] = {
                    {TF_VOICE, TF_PIPE, TF_PLUCKED, TF_ROARER, TF_DRUM, TF_LOG, TF_RATTLE, TF_CLAP, -1},
                    {TF_PLUCKED, TF_PIPE, TF_BOWED, TF_METAL, TF_REED, TF_VOICE, TF_DRUM, TF_JINGLE, -1},
                    {TF_BRASS, TF_BOWED, TF_KEYS, TF_PIPE, TF_REED, TF_ORGAN, TF_MUSICBOX, TF_VOICE, TF_BASSDRUM, TF_SNARE, TF_CYMBAL, TF_DRUM},
                    {TF_SYNTH, TF_ESTRING, TF_EKEYS, TF_PAD, TF_ORGAN, TF_VOICE, TF_KICK, TF_SNARE, TF_CLAP, TF_HAT, TF_CYMBAL, TF_BASSDRUM},
                    {TF_SYNVOICE, TF_EKEYS, TF_SYNTH, TF_SWARM, TF_PAD, TF_METAL, TF_CLICK, TF_SUB, -1},
                    {TF_FIELD, TF_METAL, TF_ROARER, TF_SWARM, -1}};
                static const double SEC_LO[AGE_COUNT] = {20, 15, 15, 12, 25, 45}, SEC_HI[AGE_COUNT] = {95, 95, 100, 130, 120, 140};
                Tradition T = traditionOf(g, L), T2 = traditionOf(g, L); trads[wi] = T;
                if (T.scale != T2.scale || T.groups != T2.groups || T.bpm != T2.bpm || T.voices.size() != T2.voices.size()) tradSame = false;
                for (int a = 0; a < AGE_COUNT; a++) if (T.ages[a].voices.size() != T2.ages[a].voices.size() || T.ages[a].kit.size() != T2.ages[a].kit.size()) tradSame = false;
                int n = (int)T.scale.size(); int gsum = 0; for (int gb : T.groups) gsum += gb;
                if (n < 5 || n > 9 || gsum != T.beats || T.voices.size() < 2 || T.voices.size() > 3 || T.scale[0] != 1.0 || T.rest <= 0 || T.rest >= n) tradOk = false;
                for (int d = 1; d < n && tradOk; d++) if (traditionCents(T, d) - traditionCents(T, d - 1) < 70) tradOk = false;
                if (1200 * std::log2(T.period) - traditionCents(T, n - 1) < 70) tradOk = false;
                for (int a = 0; a < AGE_COUNT; a++) {
                    const Ensemble& E = T.ages[a];
                    if (E.voices.empty() || E.voices.size() != E.role.size() || E.kit.size() != E.kitRole.size()) tradOk = false;
                    auto allowed = [&](int f) { for (int k = 0; k < 12 && ALLOWED[a][k] >= 0; k++) if (ALLOWED[a][k] == f) return true; return false; };
                    for (const Timbre& t : E.voices) if (!allowed(t.family) || timbrePercussive(t.family) || t.name.empty()) { tradOk = false; ageFamilyBad++; }
                    for (const Timbre& t : E.kit) if (!allowed(t.family) || !timbrePercussive(t.family) || t.name.empty()) { tradOk = false; ageFamilyBad++; }
                }
                std::set<int> forms;
                for (int i = 0; i < 50; i++) {
                    if (!sh[i].music) continue;
                    for (int a = -1; a < AGE_COUNT; a++) {
                        Shard d = sh[i]; if (a >= 0) d.recordAge = a;
                        Piece P, P2; pieceOf(T, d, P); pieceOf(T, d, P2);
                        if (P.notes.size() != P2.notes.size() || P.form != P2.form || P.bpm != P2.bpm) pieceSame = false;
                        forms.insert(P.form);
                        const Ensemble& E = pieceEnsemble(T, P);
                        if (a < 0 && P.age != std::max(0, sh[i].recordAge)) piecesOk = false;
                        pieceMin = std::min(pieceMin, (int)P.seconds); pieceMax = std::max(pieceMax, (int)P.seconds); notesMin = std::min(notesMin, (int)P.notes.size()); noteTot += (int)P.notes.size();
                        if (P.seconds < SEC_LO[P.age] || P.seconds > SEC_HI[P.age] || P.notes.size() < 12 || P.form < 0 || P.form >= PF_COUNT) { piecesOk = false; badLen++; }
                        for (size_t k = 0; k < P.notes.size(); k++) {
                            const Note& nt = P.notes[k];
                            if (k > 0 && nt.t < P.notes[k - 1].t) badOrder++;
                            if (nt.t < 0 || nt.t + nt.dur > P.beats + 1e-6 || nt.dur <= 0) badTime++;
                            if (nt.vel <= 0 || nt.vel > 1.0001) badVel++;
                            if (nt.voice >= 0 && (nt.degree < -3 * n || nt.degree > 3 * n)) badDeg++;
                            if (nt.voice >= (int)E.voices.size() || (nt.voice < 0 && (nt.degree < 0 || nt.degree >= (int)E.kit.size()))) badVoice++;
                        }
                        if (badOrder || badTime || badVel || badDeg || badVoice) piecesOk = false;
                    }
                }
                if (forms.size() < 12) formsVary = false;
            }
            {   // C-06: the decoding: the share read with one to ten shards held, never fewer per shard with more held, the names from the start, the tongue the same twice and its forty most used words distinct
                // H-04: by roots, over every record, the whole tongue at the people's own count (ten of eighty records, more of more)
                Tongue T = tongueOf(g, L), T2 = tongueOf(g, L);
                Language lang; languageOf(L, sh, T, lang); langWords[wi] = (int)lang.words.size();
                std::vector<int> prev(sh.size(), 0);
                for (int held = 1; held <= fullAt[wi]; held++) {
                    int read = 0, tot = 0;
                    for (int i = 0; i < (int)sh.size(); i++) {
                        std::vector<DecodedWord> w; int r = decodeShard(sh[i], lang, T, held, w);
                        read += r; tot += (int)w.size();
                        if (r < prev[i]) mono = false; prev[i] = r;
                        if (held == 1) for (const DecodedWord& d : w) { std::string lw = d.ours; for (char& c : lw) c = (char)tolower((unsigned char)c); if (lang.names.count(lw) && !(d.name && d.known)) namesKnown = false; }
                    }
                    if (held <= 10) readPct[wi][held] = tot > 0 ? 100.0 * read / tot : 0;
                    if (held == fullAt[wi]) readPct[wi][0] = tot > 0 ? 100.0 * read / tot : 0;   // at the people's full count
                }
                for (int k = 0; k < (int)lang.words.size() && k < 40; k++) {
                    if (tongueWord(T, lang.words[k]) != tongueWord(T2, lang.words[k])) tongueSame = false;
                    for (int j = 0; j < k; j++) if (lang.theirs[k] == lang.theirs[j]) tongueDistinct = false;
                }
                {   // C-05: the voice the same twice and in bounds, its inventory the tongue's sounds; every text's speech the same twice, 4-45 s, its segments in order
                    // without a gap or an overlap, finite and in pitch, every word with sounds spoken within its span in order, the spans the same at one shard held and at ten
                    Voice V = voiceOf(g, L, T), V2 = voiceOf(g, L, T); voices[wi] = V;
                    if (V.pitch != V2.pitch || V.rate != V2.rate || V.stress != V2.stress || V.inventory != V2.inventory) voiceSame = false;
                    if (V.pitch < 80 || V.pitch > 320 || V.rate < 2.5 || V.rate > 5.6 || V.tract < 0.7 || V.tract > 1.4 || V.range < 0.4 || V.range > 1.7 || V.inventory.size() < 8 || voiceLine(V).size() > 52) voiceOk = false;
                    for (int p : V.inventory) if (p < 0 || p >= phoneCount()) inventoryOk = false;
                    bool vowel = false; for (int p : V.inventory) if (PHONES[p].kind == PK_VOWEL) vowel = true;
                    if (!vowel) inventoryOk = false;
                    for (int i = 0; i < 50; i++) {
                        if (sh[i].music || sh[i].chart) continue;
                        std::vector<DecodedWord> w1, w10; decodeShard(sh[i], lang, T, 1, w1); decodeShard(sh[i], lang, T, fullAt[wi], w10);
                        w1.resize(shardOpening(w1)); w10.resize(shardOpening(w10));   // H-04: the opening, as the radar carries it
                        Speech S, S2, S10; speechOf(V, w10, sh[i].seed, S); speechOf(V, w10, sh[i].seed, S2); speechOf(V, w1, sh[i].seed, S10);
                        speechN++; speechMin = std::min(speechMin, S.seconds); speechMax = std::max(speechMax, S.seconds); speechTot += S.seconds;
                        if (S.segs.size() != S2.segs.size() || S.seconds != S2.seconds || S.syllables != S2.syllables) speechSame = false;
                        if (S.wordStart != S10.wordStart || S.wordEnd != S10.wordEnd) aligned = false;   // the speech does not depend on the share read
                        if (S.seconds < 4 || S.seconds > 75 || S.syllables < (int)w10.size() || S.segs.empty()) speechOk = false;   // H-04: a long record's opening, up to some sixty words
                        double tEnd = 0;
                        for (size_t k = 0; k < S.segs.size(); k++) {
                            const Segment& sg = S.segs[k];
                            if (!std::isfinite(sg.t) || !std::isfinite(sg.dur) || sg.dur <= 0 || std::fabs(sg.t - tEnd) > 1e-6 || sg.pitch < 0.4 || sg.pitch > 2.5 || sg.pitchEnd < 0.4 || sg.pitchEnd > 2.5 || sg.amp < 0 || sg.amp > 2 || sg.phone >= phoneCount()) badSeg++;
                            if (sg.phone >= 0 && (sg.word < 0 || sg.word >= (int)w10.size() || sg.t < S.wordStart[sg.word] - 1e-6 || sg.t + sg.dur > S.wordEnd[sg.word] + 1e-6)) badWord++;
                            tEnd = sg.t + sg.dur;
                        }
                        if (std::fabs(tEnd - S.seconds) > 1e-6) badSeg++;
                        for (size_t k = 0; k < w10.size(); k++) {
                            if (S.wordEnd[k] < S.wordStart[k] || (k > 0 && S.wordStart[k] < S.wordEnd[k - 1] - 1e-6)) badWord++;
                            std::string core = w10[k].ours; bool letters = false; for (char c : core) if (std::isalpha((unsigned char)c)) letters = true;
                            if (letters && S.wordEnd[k] <= S.wordStart[k]) badWord++;   // a word with letters has sounds
                        }
                    }
                    if (badSeg || badWord) speechOk = false;
                }
            }
        }
        check("records: deterministic, complete, each within its genre's length, the texts distinct (C-02, H-04)", okWorlds == 2 && determ && clean && lengths && distinct && musicN[0] >= 8 && musicN[0] <= 22 && musicN[1] >= 8 && musicN[1] <= 22,
              fmt("%d worlds of 2 with the trait; %s; %s; %d-%d words, %s; %s; %d and %d of their first fifty are music", okWorlds, determ ? "the same twice" : "NOT the same twice", clean ? "no mark left" : "a mark left", minW, maxW,
                  lengths ? "each within its genre's" : "one OUT of its genre's length", distinct ? "all distinct" : "repeats", musicN[0], musicN[1]));
        check("records: the world's and the history's names recur", okWorlds == 2 && lorePct[0] >= 70 && lorePct[1] >= 70, fmt("felisian %d%%, desert %d%% of the texts name a place, a person or a thing of the world", lorePct[0], lorePct[1]));
        check("records: the register: mostly the ordinary life, the collapse's witnesses only in its years (H-04)", okWorlds == 2 && ordinaryPct[0] >= 75 && ordinaryPct[1] >= 75 && collapseEarly == 0,
              fmt("the warnings, denials, orders, inventories, messages to whoever comes and the last: felisian %d (%d%% of the texts the ordinary genres), desert %d (%d%%); %d before the collapse's first year", collapseN[0], ordinaryPct[0], collapseN[1], ordinaryPct[1], collapseEarly));
        check("decoding: a third of the words read with one shard, all with the people's full count, never fewer with more, the names from the start, the people's words the same wherever they recur",   // C-06; H-04: by roots, the count by the records
              okWorlds == 2 && readPct[0][1] >= 25 && readPct[0][1] <= 50 && readPct[1][1] >= 25 && readPct[1][1] <= 50 && readPct[0][5] >= 45 && readPct[1][5] >= 45 && readPct[0][0] >= 99.9 && readPct[1][0] >= 99.9 && mono && namesKnown && tongueSame && tongueDistinct,
              fmt("read with 1/3/5/10/all (%d, %d) shards: felisian %.0f/%.0f/%.0f/%.0f/%.0f%% of the words (%d roots in the language), desert %.0f/%.0f/%.0f/%.0f/%.0f%% (%d); %s; %s; %s; %s", fullAt[0], fullAt[1], readPct[0][1], readPct[0][3], readPct[0][5], readPct[0][10], readPct[0][0], langWords[0],
                  readPct[1][1], readPct[1][3], readPct[1][5], readPct[1][10], readPct[1][0], langWords[1],
                  mono ? "never fewer with more held" : "FEWER read with more held", namesKnown ? "the names from the start" : "a name NOT read at one", tongueSame ? "the tongue the same twice" : "the tongue NOT the same twice", tongueDistinct ? "its forty most used words distinct" : "two of its most used words the SAME"));
        if (okWorlds == 2 && trads[0].scale == trads[1].scale && trads[0].groups == trads[1].groups) tradDiffer = false;
        check("music: a tradition a world, an ensemble an age (C-04, H-05)",
              okWorlds == 2 && tradSame && tradOk && tradDiffer,
              fmt("%s; %s; %d and %d notes to the scale, cycles of %d and %d beats, %zu and %zu voices of their own; ensembles of %zu/%zu/%zu/%zu/%zu/%zu voices",
                  tradSame ? "the tradition the same twice" : "the tradition NOT the same twice", tradOk ? "scale, cycle and the ages' ensembles in bounds" : fmt("scale, cycle or ensembles OUT of bounds (%d instruments not of their age)", ageFamilyBad).c_str(),
                  (int)trads[0].scale.size(), (int)trads[1].scale.size(), trads[0].beats, trads[1].beats, trads[0].voices.size(), trads[1].voices.size(),
                  trads[0].ages[0].voices.size(), trads[0].ages[1].voices.size(), trads[0].ages[2].voices.size(), trads[0].ages[3].voices.size(), trads[0].ages[4].voices.size(), trads[0].ages[5].voices.size()));
        check("music: every piece at every age in its length, its notes on its ensemble (H-05)",
              okWorlds == 2 && piecesOk && pieceSame && formsVary && tradDiffer,
              fmt("pieces %s, %s, %d-%d s, %d notes at least, %d in all; %s forms; the two traditions %s",
                  pieceSame ? "the same twice" : "NOT the same twice", piecesOk ? "in order and in range" : fmt("OUT of order or range (%d out of their age's length, %d out of order, %d out of time, %d velocity, %d degree, %d voice)", badLen, badOrder, badTime, badVel, badDeg, badVoice).c_str(), pieceMin, pieceMax, notesMin, noteTot, formsVary ? "twelve or more" : "FEWER than twelve", tradDiffer ? "differ" : "are the SAME"));
        {   // C-05: the two worlds' voices differ
            bool voicesDiffer = okWorlds == 2 && (voices[0].pitch != voices[1].pitch || voices[0].rate != voices[1].rate);
            check("voice: a voice a world, every text's opening spoken in time with its words",
                  okWorlds == 2 && voiceSame && voiceOk && inventoryOk && speechSame && speechOk && aligned && voicesDiffer,
                  fmt("%s; %s (felisian %.0f Hz %.1f syl/s, %zu sounds, '%s'; desert %.0f Hz %.1f syl/s, %zu sounds, '%s'); %d texts spoken %s, %.0f-%.0f s (%.0f s in all), %s, %s; the two voices %s",
                      voiceSame ? "the voice the same twice" : "the voice NOT the same twice", voiceOk && inventoryOk ? "in bounds" : "OUT of bounds", voices[0].pitch, voices[0].rate, voices[0].inventory.size(), voiceLine(voices[0]).c_str(),
                      voices[1].pitch, voices[1].rate, voices[1].inventory.size(), voiceLine(voices[1]).c_str(), speechN, speechSame ? "the same twice" : "NOT the same twice", speechMin, speechMax, speechTot,
                      speechOk ? "segments and words in order" : fmt("%d segments and %d words OUT of order", badSeg, badWord).c_str(), aligned ? "aligned at any share" : "NOT aligned across shares", voicesDiffer ? "differ" : "are the SAME"));
        }
        {   // C-05: a shard through the synth: finite, under the ceiling, loud enough, over when the speech is, silent once stopped
            Star s; StarSystem sys; bool rendered = false; int nan = 0, clip = 0; double rms = 0, tailRms = 0, stopRms = 0, secs = 0, peak = 0; bool done = false;
            if (starInSector(151, 0, 25, s, true)) {
                sys.generate(s); const Body& b = sys.bodies[1]; BodyGen g = BodyGen::make(b); Lore L = loreOf(sys, b, g);
                Tongue T = tongueOf(g, L); std::vector<Shard> sh; shardsOf(sys, b, g, L, 50, sh); Language lang; languageOf(L, sh, T, lang); Voice V = voiceOf(g, L, T);
                for (int i = 0; i < 50 && !rendered; i++) {
                    if (sh[i].music) continue;
                    std::vector<DecodedWord> w; decodeShard(sh[i], lang, T, 10, w);
                    Speech S; speechOf(V, w, sh[i].seed, S); secs = S.seconds;
                    AudioSynth synth; AudioState st; st.speech = &S; st.voice = &V; st.speechStart = true;
                    const int sr = 22050; std::vector<float> buf(2048); double sum = 0; long cnt = 0, tailCnt = 0; double tailSum = 0;
                    int frames = 0, total = (int)(sr * (S.seconds + 3));
                    while (frames < total) {
                        synth.render(buf.data(), (int)buf.size(), sr, st);
                        bool tail = frames > sr * (S.seconds + 1.5);
                        for (float v : buf) { if (!std::isfinite(v)) nan++; if (std::fabs(v) >= 0.999f) clip++; peak = std::max(peak, (double)std::fabs(v)); sum += (double)v * v; cnt++; if (tail) { tailSum += (double)v * v; tailCnt++; } }
                        frames += (int)buf.size();
                    }
                    done = st.speechDone; rms = std::sqrt(sum / std::max(1L, cnt)); tailRms = std::sqrt(tailSum / std::max(1L, tailCnt));
                    st.speech = nullptr; st.voice = nullptr;
                    double ssum = 0; long scnt = 0;
                    for (int k = 0; k < 10; k++) { synth.render(buf.data(), (int)buf.size(), sr, st); for (float v : buf) { ssum += (double)v * v; scnt++; } }
                    stopRms = std::sqrt(ssum / std::max(1L, scnt));
                    rendered = true;
                }
            }
            check("voice: a shard through the synth", rendered && nan == 0 && clip < 50 && rms > 0.02 && rms < 0.35 && peak > 0.2 && done && tailRms < 0.005 && stopRms < 1e-6,
                  fmt("%.1f s rendered: %d non-finite, %d at the ceiling, peak %.2f, rms %.3f, %s, the tail %.4f, after the stop %.6f", secs, nan, clip, peak, rms, done ? "ended on time" : "NOT ended", tailRms, stopRms));
        }
        {   // C-04: a piece through the synth: finite, under the ceiling, loud enough, over when the piece is, silent once stopped; H-05: one of each age (Aieliaalas
            // II's first piece played at each), each in level, the dearest within a twentieth of real time
            Star s; StarSystem sys; int rendered = 0, nan = 0, clip = 0, quiet = 0, loud = 0, notDone = 0, tails = 0, stops = 0; double rmsLo = 1, rmsHi = 0, peak = 0, dearest = 0; std::string levels;
            if (starInSector(151, 0, 25, s, true)) {
                sys.generate(s); const Body& b = sys.bodies[1]; BodyGen g = BodyGen::make(b); Lore L = loreOf(sys, b, g);
                Tradition T = traditionOf(g, L); std::vector<Shard> sh; shardsOf(sys, b, g, L, 50, sh);
                int first = -1; for (int i = 0; i < 50 && first < 0; i++) if (sh[i].music) first = i;
                for (int a = 0; a < AGE_COUNT && first >= 0; a++) {
                    Shard d = sh[first]; d.recordAge = a;
                    Piece P; pieceOf(T, d, P);
                    AudioSynth synth; AudioState st; st.piece = &P; st.tradition = &T; st.pieceStart = true;
                    const int sr = 22050; std::vector<float> buf(2048); double sum = 0; long cnt = 0, tailCnt = 0; double tailSum = 0;
                    int frames = 0, total = (int)(sr * (P.seconds + 4));
                    const double t0 = nowSec();
                    while (frames < total) {
                        synth.render(buf.data(), (int)buf.size(), sr, st);
                        bool tail = frames > sr * (P.seconds + 2.5);
                        for (float v : buf) { if (!std::isfinite(v)) nan++; if (std::fabs(v) >= 0.999f) clip++; peak = std::max(peak, (double)std::fabs(v)); sum += (double)v * v; cnt++; if (tail) { tailSum += (double)v * v; tailCnt++; } }
                        frames += (int)buf.size();
                    }
                    dearest = std::max(dearest, (nowSec() - t0) / std::max(1.0, (double)frames / sr));
                    const double rms = std::sqrt(sum / std::max(1L, cnt)), tailRms = std::sqrt(tailSum / std::max(1L, tailCnt));
                    if (!st.pieceDone) notDone++;
                    if (tailRms >= 0.01) tails++;
                    if (rms < 0.04) quiet++;
                    if (rms > 0.25) loud++;
                    rmsLo = std::min(rmsLo, rms); rmsHi = std::max(rmsHi, rms);
                    levels += fmt("%s%s %.3f", levels.empty() ? "" : ", ", AGE_NAMES[a], rms);
                    st.piece = nullptr; st.tradition = nullptr;
                    double ssum = 0; long scnt = 0;
                    for (int k = 0; k < 10; k++) { synth.render(buf.data(), (int)buf.size(), sr, st); for (float v : buf) { ssum += (double)v * v; scnt++; } }
                    if (std::sqrt(ssum / std::max(1L, scnt)) >= 1e-6) stops++;
                    rendered++;
                }
            }
            check("music: a piece of each age through the synth (C-04, H-05)", rendered == AGE_COUNT && nan == 0 && clip == 0 && quiet == 0 && loud == 0 && notDone == 0 && tails == 0 && stops == 0 && peak < 0.95 && dearest < 0.05,
                  fmt("%d of %d ages rendered: rms %s; peak %.2f; %d non-finite, %d at the ceiling, %d not ended, %d with a tail, %d not silent once stopped; the dearest %.1f%% of real time", rendered, AGE_COUNT, levels.c_str(), peak, nan, clip, notDone, tails, stops, 100 * dearest));
        }
        int worlds = 0, bad = 0;   // every people's world of the sectors near home: twelve shards each, clean and in length
        for (int64_t sx = 150; sx <= 160 && worlds < 60; sx++) for (int64_t sz = 20; sz <= 80 && worlds < 60; sz++) {
            Star s; StarSystem sys; if (!systemAt(sx, 0, sz, s, sys, true)) continue;
            for (const Body& b : sys.bodies) {
                if (!kindHas(b.type, KT_PEOPLE)) continue;   // P-04: every people's world (a scorched one's too)
                BodyGen g = BodyGen::make(b);
                if (!g.hasTrait(TR_CIVILISATION)) continue;
                worlds++;
                Lore L = loreOf(sys, b, g);
                for (int i = 0; i < 12; i++) { Shard x = shardOf(sys, b, g, L, i); if (x.music) continue; int nw = shardWords(x.text); if (x.text.find_first_of("{}[]|") != std::string::npos || nw < 6 || nw > 440) bad++; }
            }
        }
        check("shards: every people's world near home reads clean", worlds >= 10 && bad == 0, fmt("%d worlds of sectors 150-160 x 20-80, %d shards of %d with a mark left or out of length", worlds, bad, worlds * 12));
    }
}
