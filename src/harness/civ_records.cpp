// E-03: the records and the decoder (T-04, H-05, H-06): `records`, `decoder` and their checks
#include "harness/harness.h"
#include "harness/test_worlds.h"
void recordPlacesOf(const BodyGen& g, int people, std::vector<RecordPlace>& out, int countryside) {
    out.clear();
    std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
    const int which = std::min(people, std::max(0, P->peoples - 1));
    const PeoplePlan& pp = P->plans[which];
    const Culture cu = cultureOf(g, which);
    for (const PlanPlace& p : pp.places) {
        RecordPlace rp; if (!ruinOfCell(g, p.gLat, p.gLon, rp.spec, true) || rp.spec.people != which) continue;
        recordSitesOf(g, rp.spec, cu, rp.sites, &rp.instruments); out.push_back(std::move(rp));
    }
    std::shared_ptr<const WorldWorks> W = worldWorksOf(g);
    for (const Work& w : W->works) {
        if (w.people != which) continue;
        RecordPlace rp; rp.work = w.kind; workSpec(g, w, rp.spec);
        recordSitesOf(g, rp.spec, cu, rp.sites); out.push_back(std::move(rp));
    }
    {   // and the land's round the capital (the countryside's reach)
        std::vector<std::shared_ptr<const Work>> lw; landWorksOf(g, which, 1, 2000.0 * countryside + 2000, 0, lw);
        for (const std::shared_ptr<const Work>& w : lw) { RecordPlace rp; rp.work = w->kind; workSpec(g, *w, rp.spec); recordSitesOf(g, rp.spec, cu, rp.sites); out.push_back(std::move(rp)); }
    }
    if (pp.places.empty()) return;
    DrainageOff off;
    const PlanPlace& c0 = pp.places[0];
    for (int dl = -countryside; dl <= countryside; dl++) for (int dn = -countryside; dn <= countryside; dn++) {
        RecordPlace rp; const int gl = c0.gLat + dl, gn = ruinCellLonIn(g, c0.gLat, c0.gLon, gl) + dn;
        if (!ruinOfCell(g, gl, gn, rp.spec, true) || rp.spec.rank >= 0 || rp.spec.sclass == SC_WORKS || rp.spec.people != which) continue;
        rp.countryside = true; recordSitesOf(g, rp.spec, cu, rp.sites, &rp.instruments); out.push_back(std::move(rp));
    }
}

static std::string recordSiteLine(const RecordCatalogue& C, const ShardSite& x) {
    const RecordEntry& e = C.entries[shardLocalOf(x.index)];
    return fmt("%s%s%s S%d %s (%s, year %d) in %s", x.last ? "THE LAST " : "", x.archive ? "archive " : "", x.wallD > 0 ? "on a wall " : "", shardLocalOf(x.index), RECORD_MEDIUM_NAMES[x.medium], RECORD_CATEGORY_NAMES[e.category], e.year, buildingPlaceName(x.place));
}

static void printRecords(const AgeWorld& aw) {
    Star st; StarSystem sys; if (!systemAt(aw.sx, 0, aw.sz, st, sys, true)) { printf("no star at %lld 0 %lld\n", (long long)aw.sx, (long long)aw.sz); return; }
    if (aw.body >= (int)sys.bodies.size()) { printf("no body %d\n", aw.body); return; }
    const Body& b = sys.bodies[aw.body];
    const BodyGen g = BodyGen::make(b);
    if (!worldHadCivilisation(g)) { printf("%s: no people\n", b.name.c_str()); return; }
    const int which = std::min(aw.which, std::max(0, peoplesOf(g) - 1));
    std::shared_ptr<const RecordCatalogue> Cp = recordsOf(g, which);
    const RecordCatalogue& C = *Cp;
    const PeopleAge pa = peopleAgeOf(g, which); const PeopleExtent ex = peopleExtentOf(g, which);
    const Lore L = loreQuick(sys, b, g, which);
    std::shared_ptr<const History> H = historyOf(sys, b, g, which);
    printf("%s, the %s (%s, %s at their height): %d records (80 to 200 by the age and the numbers), %d music, %d star charts; dated year 1 to %d of their count (built in %.2f ms)\n",
           b.name.c_str(), L.people.c_str(), AGE_NAMES[pa.age], peopleCount(ex.peak).c_str(), C.count, C.music, C.charts, C.lastYear, C.buildMs);
    int byAge[AGE_COUNT] = {}, byMedium[RM_COUNT] = {}, cm[RC_COUNT][RM_COUNT] = {};
    for (const RecordEntry& e : C.entries) { byAge[e.age]++; byMedium[e.medium]++; cm[e.category][e.medium]++; }
    printf("  by their years' age:"); for (int a = 0; a < AGE_COUNT; a++) if (byAge[a]) printf(" %s %d", AGE_NAMES[a], byAge[a]); printf("\n");
    for (int c = 0; c < RC_COUNT; c++) { printf("  %s:", RECORD_CATEGORY_NAMES[c]); for (int m = 0; m < RM_COUNT; m++) if (cm[c][m]) printf(" %s %d", RECORD_MEDIUM_NAMES[m], cm[c][m]); printf("\n"); }
    if (C.last >= 0) {
        const int lp = H ? H->lastPlace : -1;
        printf("  the last recording: S%d, year %d, %s; the last place %s (rank %d)\n", C.last, C.entries[C.last].year, RECORD_MEDIUM_NAMES[C.entries[C.last].medium],
               lp >= 0 ? historyPlaceName(*H, lp).c_str() : "-", lp >= 0 ? H->places[lp].plan : -1);
    }
    std::vector<RecordPlace> places; recordPlacesOf(g, which, places);
    std::vector<int> copies(C.count, 0);
    int nPlaces = 0, nRecords = 0, archives = 0, archived = 0, walls = 0, byClass[7] = {}, recByClass[7] = {}, works = 0, recWorks = 0, country = 0, recCountry = 0;
    std::map<std::string, int> byPlace;
    for (const RecordPlace& rp : places) {
        bool arch = false;
        for (const ShardSite& x : rp.sites) { copies[shardLocalOf(x.index)]++; if (x.archive) { archived++; arch = true; } if (x.wallD > 0) walls++; byPlace[buildingPlaceName(x.place)]++; }
        if (arch) archives++;
        nRecords += (int)rp.sites.size();
        if (rp.work >= 0) { works++; recWorks += (int)rp.sites.size(); }
        else if (rp.countryside) { country++; recCountry += (int)rp.sites.size(); }
        else { nPlaces++; byClass[rp.spec.sclass]++; recByClass[rp.spec.sclass] += (int)rp.sites.size(); }
    }
    printf("  where they lie: %d places of the plan (", nPlaces);
    bool firstClass = true;
    for (int k = 0; k < 7; k++) if (byClass[k]) { std::string cn = SETTLEMENT_CLASS_NAMES[k]; if (byClass[k] > 1) cn = cn == "CITY" ? "CITIES" : cn + "S"; printf("%s%d %s holding %d", firstClass ? "" : ", ", byClass[k], cn.c_str(), recByClass[k]); firstClass = false; }
    printf("), %d works holding %d, %d settlements of the countryside round the capital holding %d (%d in all); %d archives holding %d, %d painted walls\n", works, recWorks, country, recCountry, nRecords, archives, archived, walls);
    printf("  by their places:"); for (const auto& kv : byPlace) printf(" %s %d,", kv.first.c_str(), kv.second); printf("\n");
    int none = 0, most = 0; for (int i = 0; i < C.count; i++) { if (!copies[i]) none++; most = std::max(most, copies[i]); }
    printf("  copies over these places: every record but %d has one, the most a record has %d\n", none, most);
    int shown = 0, shownCountry = 0;
    for (const RecordPlace& rp : places) {
        const bool lastHere = std::any_of(rp.sites.begin(), rp.sites.end(), [](const ShardSite& x) { return x.last; });
        const bool arch = std::any_of(rp.sites.begin(), rp.sites.end(), [](const ShardSite& x) { return x.archive > 0; });
        bool show = lastHere || rp.work >= 0;
        if (!show && !rp.countryside && rp.spec.rank >= 0 && (shown < 4 || (arch && shown < 6) || rp.spec.before > 0)) { show = true; shown++; }
        if (!show && rp.countryside && !rp.sites.empty() && shownCountry < 4) { show = true; shownCountry++; }
        if (!show || rp.sites.empty()) continue;
        const std::string what = rp.work >= 0 ? std::string(WORK_KIND_NAMES[rp.work]) : (rp.countryside ? std::string("a countryside ") + SETTLEMENT_CLASS_NAMES[rp.spec.sclass] : fmt("%s rank %d", SETTLEMENT_CLASS_NAMES[rp.spec.sclass], rp.spec.rank));
        printf("    %s%s (%zu buildings%s): %zu\n", what.c_str(), lastHere ? " - THE LAST PLACE" : "", rp.spec.buildings.size(), rp.spec.before > 0 ? fmt(", left %.0f years before the end", rp.spec.before).c_str() : "", rp.sites.size());
        for (const ShardSite& x : rp.sites) printf("      %s\n", recordSiteLine(C, x).c_str());
    }
}

// T-04: each age's records through the game: landed by the scene world's capital, the explorer before a record of each medium the age's
// records come in (an archive's row first), a frame, E, the guide holding it and the frame after (`shots/tests/records_<age>_<n>.png`, a sheet
// a age `records_<age>_sheet.png`); returns the failures (a medium with no record in reach, a record not taken, nothing drawn)
static int recordsScenes(int onlyAge = -1, std::vector<std::string>* lines = nullptr) {
    mkdir("shots/tests", 0755);
    int failures = 0;
    for (int age = 0; age < AGE_COUNT; age++) {
        if (onlyAge >= 0 && age != onlyAge) continue;
        const AgeWorld& aw = AGE_SCENE_WORLDS[age];
        const char* an = age == AGE_FAR_FUTURE ? "far_future" : AGE_NAMES[age];
        Star st; StarSystem sys; if (!systemAt(aw.sx, 0, aw.sz, st, sys, true)) { failures++; continue; }
        const BodyGen g = BodyGen::make(sys.bodies[aw.body]);
        std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
        const int which = std::min(aw.which, std::max(0, P->peoples - 1));
        if (P->plans[which].places.empty()) { failures++; continue; }
        std::shared_ptr<const RecordCatalogue> C = recordsOf(g, which);
        // the capital's media, the most common first (the archive's row is a shot of its own)
        std::vector<RecordPlace> places; recordPlacesOf(g, which, places, 0);
        const RecordPlace* cap = nullptr; for (const RecordPlace& rp : places) if (rp.work < 0 && rp.spec.rank == 0) cap = &rp;
        if (!cap) { failures++; continue; }
        int mc[RM_COUNT] = {}; bool anyArchive = false;
        for (const ShardSite& x : cap->sites) { if (!x.archive && !x.last) mc[x.medium]++; if (x.archive) anyArchive = true; }
        std::vector<int> media; for (int m = 0; m < RM_COUNT; m++) if (mc[m]) media.push_back(m);
        std::sort(media.begin(), media.end(), [&](int a, int b) { return mc[a] > mc[b]; });
        if (media.size() > 4) media.resize(4);
        remove("shots/tests/test_records_guide.txt");
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_records_guide.txt";
        game.newGame(); game.setState(GameState::SPACE); applyTestScale(game);
        Input gi;
        auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        run(0.2); game.testParkAt(st, aw.body); run(0.3);
        const PlanPlace& c0 = P->plans[which].places[0];
        game.testLandCursor(c0.lat / DEG, c0.lon / DEG); run(0.1); press(KEY_ENTER);
        for (int i = 0; i < 30 * 20 && !(game.state == GameState::SURFACE && game.testPlayerAlt() < 3); i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); }
        if (game.state != GameState::SURFACE) { printf("%s: not landed\n", an); failures++; continue; }
        const double rot = std::max(600.0, std::fabs(sys.bodies[aw.body].rotPeriod));
        for (int k = 0; k < 64 && game.testSunHeight() < 0.5; k++) { game.testAdvanceTime(rot / 64); run(0.05); }   // the day: the sun well up
        const int scale = std::max(1, g_testScale), cols = 3, textH = 12 * scale, tileH = FBH + textH;
        struct Shot { std::vector<uint32_t> px; std::string label; };
        std::vector<Shot> shots;
        auto shoot = [&](const std::string& label, const std::string& file) {
            Shot sh; sh.px.assign(game.output(), game.output() + (size_t)FBW * FBH); sh.label = label; shots.push_back(sh);
            writePNG(fmt("shots/tests/records_%s_%s.png", an, file.c_str()).c_str(), game.output(), FBW, FBH);
        };
        std::vector<std::pair<int, bool>> todo;
        if (anyArchive) todo.push_back({-1, true});
        for (int m : media) todo.push_back({m, false});
        int n = 0;
        for (const auto& td : todo) {
            const int idx = game.testWalkToRecord(td.first, td.second);
            if (idx < 0) { printf("  %s: no %s in reach\n", an, td.second ? "archive" : RECORD_MEDIUM_NAMES[td.first]); failures++; continue; }
            run(0.4);
            const int drawn = game.testRecordFormsDrawn(), near = game.testNearShardIndex(), medium = game.testNearShardMedium();
            const std::string hud = near >= 0 ? std::string(RECORD_MEDIUM_HUD[std::max(0, medium)]) + " - E" : std::string("(none in reach)");
            const std::string file = td.second ? "archive" : fmt("%d_%s", n, RECORD_MEDIUM_NAMES[td.first]);
            for (char* c = &const_cast<std::string&>(file)[0]; *c; c++) if (*c == ' ') *c = '_';
            shoot(td.second ? "AN ARCHIVE" : RECORD_MEDIUM_HUD[td.first], file);
            const size_t before = game.guide.shards.size();
            bool taken = false;
            if (near >= 0) { press(KEY_E); run(0.3); taken = game.guide.shards.size() == before + 1; }
            const int after = game.testRecordFormsDrawn();
            const bool wall = medium == RM_PAINTED && after == drawn;   // a painted wall stays painted once recorded
            if (!td.second) shoot(std::string(RECORD_MEDIUM_HUD[td.first]) + (wall ? " RECORDED" : " TAKEN"), file + "_taken");
            const std::string line = fmt("%s %s: S%d %s, %d forms drawn, the HUD '%s', E %s, %d forms after", an, td.second ? "archive" : RECORD_MEDIUM_NAMES[td.first], shardLocalOf(std::max(0, idx)),
                                         near >= 0 ? RECORD_MEDIUM_NAMES[std::max(0, medium)] : "-", drawn, hud.c_str(), taken ? "took it" : "DID NOT TAKE IT", after);
            printf("  %s\n", line.c_str()); fflush(stdout);
            if (lines) lines->push_back(line);
            if (near < 0 || !taken || drawn <= 0 || (!wall && after >= drawn)) failures++;
            n++;
        }
        const int rows = std::max(1, ((int)shots.size() + cols - 1) / cols), SW = cols * FBW, SH = 16 * scale + rows * tileH;
        std::vector<uint32_t> sheet((size_t)SW * SH, rgb(16, 16, 20));
        RGBCanvas cv{sheet.data(), SW, SH, scale};
        drawText(cv, 4, 4, fmt("THE RECORDS OF A %s PEOPLE ON %s: %d IN ALL", age == AGE_FAR_FUTURE ? "FAR FUTURE" : AGE_NAMES[age], sys.bodies[aw.body].name.c_str(), C->count).c_str(), rgb(235, 225, 170));
        for (size_t i = 0; i < shots.size(); i++) {
            const int cx = (int)(i % cols), cy = (int)(i / cols), dx0 = cx * FBW, dy0 = 16 * scale + cy * tileH;
            for (int y = 0; y < FBH; y++) for (int x = 0; x < FBW; x++) sheet[(size_t)(dy0 + y) * SW + dx0 + x] = shots[i].px[(size_t)y * FBW + x];
            drawText(cv, dx0 / scale + 3, (dy0 + FBH) / scale + 2, shots[i].label.c_str(), rgb(200, 205, 190));
        }
        writePNG(fmt("shots/tests/records_%s_sheet.png", an).c_str(), sheet.data(), SW, SH);
        printf("  -> shots/tests/records_%s_sheet.png\n", an);
        remove("shots/tests/test_records_guide.txt");
    }
    return failures;
}

// T-04: the unit's checks of where the records lie (also `records check`)
void recordsChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    struct Wd { StarSystem sys; BodyGen g; int which = 0; bool ok = false; std::string name; };
    std::vector<Wd> W(AGE_COUNT);
    for (int a = 0; a < AGE_COUNT; a++) {
        Star st; if (!starInSector(AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz, st, true)) continue;
        W[a].sys.generate(st);
        if (AGE_WORLDS[a].body >= (int)W[a].sys.bodies.size()) continue;
        W[a].g = BodyGen::make(W[a].sys.bodies[AGE_WORLDS[a].body]); W[a].which = std::min(AGE_WORLDS[a].which, std::max(0, peoplesOf(W[a].g) - 1));
        W[a].ok = worldHadCivilisation(W[a].g); W[a].name = W[a].sys.bodies[AGE_WORLDS[a].body].name;
    }
    {   // the catalogue: 80 to 200 by the age and the numbers, the years the history's with the last the end's, each record in its year's medium, C-02's
        // fifty kept (music and charts where they were), the rest by the age of each record's year (H-05), the same when built again
        int worlds = 0, outCount = 0, outYears = 0, lastsBad = 0, mediumBad = 0, fiftyBad = 0, sharesBad = 0, notSame = 0, counts[AGE_COUNT] = {};
        for (int a = 0; a < AGE_COUNT; a++) {
            if (!W[a].ok) continue;
            worlds++;
            std::shared_ptr<const RecordCatalogue> C = recordsOf(W[a].g, W[a].which);
            const BodyGen pg = peopleGen(W[a].g, W[a].which);
            const PeopleAge pa = peopleAgeOf(W[a].g, W[a].which);
            counts[a] = C->count;
            if (C->count < RECORDS_MIN || C->count > RECORDS_MAX || C->count != recordCountOf(W[a].g, W[a].which)) outCount++;
            int lasts = 0;
            for (int i = 0; i < C->count; i++) {
                const RecordEntry& e = C->entries[i];
                if (e.year < 1 || e.year > C->lastYear) outYears++;
                if (e.last) { lasts++; if (e.year != C->lastYear || i != lastShardOf(pg)) lastsBad++; }
                if (!recordMediumFits(e.medium, e.age, e.category)) mediumBad++;
                if (i < SHARDS_PER_WORLD && ((e.category == RC_MUSIC) != shardIsMusic(pg, i) || (e.category == RC_CHART) != (chartIndexOf(pg, i) >= 0))) fiftyBad++;
                Rng rng(mix64(pg.seed ^ 0x5A4D5ULL) + (uint64_t)i * 0x9E3779B97F4A7C15ULL);
                const double u = rng.uni();
                if (!e.last && e.year != std::min(recordYear(pa, u), std::max(1, C->lastYear - 1))) notSame++;
            }
            if (lasts != 1) lastsBad++;
            {   // past the fifty, music and charts by the age of each record's year: the age's share leaned by the people's fifty, within one record
                int n[AGE_COUNT] = {}, m[AGE_COUNT] = {}, c[AGE_COUNT] = {};
                for (int i = SHARDS_PER_WORLD; i < C->count; i++) { const RecordEntry& e = C->entries[i]; n[e.age]++; if (e.category == RC_MUSIC) m[e.age]++; else if (e.category == RC_CHART) c[e.age]++; }
                for (int k = 0; k < AGE_COUNT; k++) {
                    if (std::abs(m[k] - n[k] * recordMusicShare(k) * musicShardsOf(pg) / 15.0) > 1.0 || std::abs(c[k] - n[k] * recordChartShare(k) * chartShardsOf(pg) / 4.0) > 1.0) sharesBad++;
                }
            }
            if (C->charts != recordChartCount(pg, C->count)) sharesBad++;
        }
        const bool ordered = counts[AGE_PREHISTORIC] < counts[AGE_INDUSTRIAL] && counts[AGE_MEDIEVAL] < counts[AGE_MODERN] && counts[AGE_INDUSTRIAL] < counts[AGE_FUTURISTIC] && counts[AGE_MODERN] < counts[AGE_FAR_FUTURE];
        check("records: 80 to 200 a people by its age and numbers, dated in its history, each in its year's medium (T-04)",
              worlds == AGE_COUNT && outCount == 0 && outYears == 0 && lastsBad == 0 && mediumBad == 0 && fiftyBad == 0 && sharesBad == 0 && notSame == 0 && ordered,
              fmt("%d age worlds: %d, %d, %d, %d, %d, %d records%s; %d counts out, %d years out, %d last recordings wrong, %d media not of their age, %d of C-02's fifty moved, %d shares off, %d years not the same again",
                  worlds, counts[0], counts[1], counts[2], counts[3], counts[4], counts[5], ordered ? " (the later ages more)" : " (NOT ORDERED BY AGE)", outCount, outYears, lastsBad, mediumBad, fiftyBad, sharesBad, notSame));
    }
    // the places, the works and the countryside round each capital, with their records
    std::vector<std::vector<RecordPlace>> places(AGE_COUNT);
    for (int a = 0; a < AGE_COUNT; a++) if (W[a].ok) recordPlacesOf(W[a].g, W[a].which, places[a], 7);
    {   // no two copies in one place; every record its people's and written before its place was left; an archive in the capital and the cities of every
        // literate age; the painted walls before a wall that stands; the same twice
        int placesN = 0, sitesN = 0, dup = 0, foreign = 0, late = 0, capArch = 0, capN = 0, cityArch = 0, cityN = 0, walls = 0, wallsBad = 0, notSame = 0, works = 0, worksWith = 0;
        for (int a = 0; a < AGE_COUNT; a++) {
            if (!W[a].ok) continue;
            std::shared_ptr<const RecordCatalogue> C = recordsOf(W[a].g, W[a].which);
            const Culture cu = cultureOf(W[a].g, W[a].which);
            for (const RecordPlace& rp : places[a]) {
                placesN++; sitesN += (int)rp.sites.size();
                if (rp.work >= 0) { works++; if (!rp.sites.empty()) worksWith++; }
                std::set<int> seen; bool arch = false;
                const int endYear = C->lastYear - (int)std::lround(std::max(0.0, rp.spec.before));
                for (const ShardSite& x : rp.sites) {
                    if (!seen.insert(x.index).second) dup++;
                    if (shardPeopleOf(x.index) != W[a].which || shardLocalOf(x.index) >= C->count) { foreign++; continue; }
                    const RecordEntry& e = C->entries[shardLocalOf(x.index)];
                    if (!e.last && rp.work != WO_SHELTER && e.year > endYear) late++;
                    if (x.medium != e.medium) foreign++;
                    if (x.archive) arch = true;
                    if (x.wallD > 0) { walls++; if (x.wallTop < 1.4 || x.medium != RM_PAINTED) wallsBad++; }
                }
                if (rp.work < 0 && !rp.countryside && a != AGE_PREHISTORIC && a != AGE_FAR_FUTURE) {
                    if (rp.spec.sclass == SC_CAPITAL) { capN++; if (arch) capArch++; }
                    if (rp.spec.sclass == SC_CITY) { cityN++; if (arch) cityArch++; }
                }
                if (&rp == &places[a].front() || rp.work >= 0) {   // the same twice
                    std::vector<ShardSite> again; recordSitesOf(W[a].g, rp.spec, cu, again);
                    if (again.size() != rp.sites.size()) notSame++;
                    else for (size_t i = 0; i < again.size(); i++) if (again[i].index != rp.sites[i].index || again[i].x != rp.sites[i].x || again[i].z != rp.sites[i].z) { notSame++; break; }
                }
            }
        }
        check("records: no two copies in one place, none written after its place was left, an archive in every capital and city of a literate age (T-04)",
              placesN > 0 && sitesN > 0 && dup == 0 && foreign == 0 && late == 0 && capN > 0 && capArch == capN && cityN > 0 && cityArch * 10 >= cityN * 9 && walls > 0 && wallsBad == 0 && notSame == 0 && worksWith > 0,
              fmt("%d places (the plans', %d works, the countryside round the capitals) holding %d records: %d copies twice in a place, %d not the place's people's or medium, %d written after their place was left; archives in %d of %d capitals and %d of %d cities; %d painted walls (%d not standing); %d works hold records; %s",
                  placesN, works, sitesN, dup, foreign, late, capArch, capN, cityArch, cityN, walls, wallsBad, worksWith, notSame ? "NOT the same twice" : "the same twice"));
    }
    {   // an archive holds its class's count (the capital's 8 to 14, a prehistoric capital's 4 to 6, a city's 5 to 8, a town's 3 to 5), the
        // place's own beyond it lie in its other buildings, and every place of the plan holds all its own (a lone stone's foot held two); the far
        // future's structures keep theirs in rings round their feet (the archive that keeps itself). Then the report: Yaiphuyiabrus IV's
        // prehistoric capital, whose archive took its own thirty-one onto its longhouse's floor
        auto archMax = [](int sclass, int age) { return sclass == SC_CAPITAL ? (age == AGE_PREHISTORIC ? 6 : 14) : (sclass == SC_CITY ? 8 : (sclass == SC_TOWN ? 5 : 0)); };
        int archives = 0, over = 0, worst = 0, ownN = 0, ownLaid = 0, rings = 0; std::string worstAt;
        auto tally = [&](const BodyGen& g, int which, const std::vector<RecordPlace>& ps, int age) {
            std::shared_ptr<const RecordCatalogue> C = recordsOf(g, which);
            std::shared_ptr<const RecordHomes> Hm = recordHomesOf(g, which);
            for (const RecordPlace& rp : ps) {
                if (rp.work >= 0 || rp.countryside || rp.spec.rank < 0) continue;
                int arch = 0; for (const ShardSite& x : rp.sites) arch += x.archive > 0;
                if (rp.spec.sclass == SC_MONUMENT && age == AGE_FAR_FUTURE) rings += arch;
                else if (arch) {
                    archives++;
                    if (arch > archMax(rp.spec.sclass, age)) over++;
                    if (arch > worst) { worst = arch; worstAt = fmt("a %s %s", AGE_NAMES[age], SETTLEMENT_CLASS_NAMES[rp.spec.sclass]); }
                }
                for (int i = 0; i < C->count && i < (int)Hm->rank.size(); i++) {
                    if (Hm->rank[i] != rp.spec.rank) continue;
                    ownN++;
                    for (const ShardSite& x : rp.sites) if (shardLocalOf(x.index) == i) { ownLaid++; break; }
                }
            }
        };
        for (int a = 0; a < AGE_COUNT; a++) if (W[a].ok) tally(W[a].g, W[a].which, places[a], a);
        Star ys; StarSystem ysys; bool yOk = starInSector(-31900, 3, -35668, ys, true);
        if (yOk) { ysys.generate(ys); yOk = (int)ysys.bodies.size() > 3; }
        int capArch = 0, capOwn = 0, capLaid = 0, capHeap = 0, capAge = -1; std::set<int> capBuildings;
        if (yOk) {
            const BodyGen yg = BodyGen::make(ysys.bodies[3]);
            yOk = worldHadCivilisation(yg);
            if (yOk) {
                std::vector<RecordPlace> yp; recordPlacesOf(yg, 0, yp, 0);
                capAge = peopleAgeOf(yg, 0).age;
                tally(yg, 0, yp, capAge);
                std::shared_ptr<const RecordHomes> Hm = recordHomesOf(yg, 0);
                for (const RecordPlace& rp : yp) {
                    if (rp.work >= 0 || rp.spec.rank != 0) continue;
                    for (const ShardSite& x : rp.sites) {
                        capArch += x.archive > 0; capBuildings.insert(x.building);
                        int near = 0; for (const ShardSite& y : rp.sites) if (std::hypot(x.x - y.x, x.z - y.z) < 6) near++;
                        capHeap = std::max(capHeap, near);
                    }
                    for (int i = 0; i < (int)Hm->rank.size(); i++) if (Hm->rank[i] == 0) {
                        capOwn++;
                        for (const ShardSite& x : rp.sites) if (shardLocalOf(x.index) == i) { capLaid++; break; }
                    }
                }
            }
        }
        check("records: an archive its class's count, the place's own beyond it in its other buildings, every place holding its own (B-426)",
              archives > 0 && over == 0 && ownN > 0 && ownLaid == ownN && rings > 0 && yOk && capAge == AGE_PREHISTORIC && capArch >= 4 && capArch <= 6 && capLaid == capOwn && (int)capBuildings.size() >= 20 && capHeap <= 8,
              fmt("%d archives, %d over their count (the most %d, %s); %d of the places' %d own records laid at them; the far future's rings %d; Yaiphuyiabrus IV's %s capital: an archive of %d (31 before), its %d own laid %d in %zu buildings, at most %d within 6 m",
                  archives, over, worst, worstAt.empty() ? "-" : worstAt.c_str(), ownLaid, ownN, rings, capAge >= 0 ? AGE_NAMES[capAge] : "?", capArch, capOwn, capLaid, capBuildings.size(), capHeap));
    }
    {   // the last recording where the history ended: at its last place, in the building its age keeps last words in, and in the last shelter by it;
        // nowhere else
        int worlds = 0, good = 0; std::string line;
        for (int a = 0; a < AGE_COUNT; a++) {
            if (!W[a].ok) continue;
            worlds++;
            std::shared_ptr<const RecordCatalogue> C = recordsOf(W[a].g, W[a].which);
            std::shared_ptr<const History> H = historyOfWorld(W[a].g, W[a].which);
            const int lastRank = H && H->lastPlace >= 0 ? H->places[H->lastPlace].plan : 0;
            int atPlace = 0, atShelter = 0, elsewhere = 0, shelters = 0; std::string where;
            for (const RecordPlace& rp : places[a]) {
                if (rp.work == WO_SHELTER) shelters++;
                for (const ShardSite& x : rp.sites) {
                    if (shardLocalOf(x.index) != C->last) continue;
                    if (rp.work == WO_SHELTER) atShelter++;
                    else if (rp.work < 0 && !rp.countryside && rp.spec.rank == lastRank) { atPlace++; where = buildingPlaceName(x.place); }
                    else elsewhere++;
                }
            }
            const bool ok = atPlace == 1 && atShelter == shelters && shelters <= 1 && elsewhere == 0;
            if (ok) good++;
            line += fmt("%s%s: %s (rank %d), %s%s", line.empty() ? "" : "; ", AGE_NAMES[a], atPlace ? where.c_str() : "NOT AT ITS PLACE", lastRank, shelters ? (atShelter ? "and the shelter" : "NOT THE SHELTER") : "no shelter", elsewhere ? fmt(", %d ELSEWHERE", elsewhere).c_str() : "");
        }
        check("records: the last recording at the history's last place and in its last shelter, nowhere else (T-04)", worlds == AGE_COUNT && good == worlds, line);
    }
    {   // an old guide (C-13's keys: the second people's at 50 + index, no `records` line) loaded: kept by index, the counts and the language kept,
        // said once
        const char* path = "shots/tests/test_records_old_guide.txt";
        {
            std::ofstream f(path);
            f << "vesperis-guide 1\ngen 12\n";
            f << "shard 151,0,25/1/S3\nshard 151,0,25/1/S12\ndecoded 151,0,25/1/S3 2\n";
            f << "shard 152,0,92/0/S2\nshard 152,0,92/0/S57\nshard 152,0,92/0/S63\ndecoded 152,0,92/0/S57 3\nheard 152,0,92/0/S57\nname 152,0,92/0/S57 The river at night\nlent 152,0,92/0/S60\nname 151,0,25/1 Kept\n";
        }
        auto heldOf = [](const Guide& G, const std::string& body, int people) { int n = 0; for (const std::string& k : G.shards) if (k.compare(0, body.size() + 2, body + "/S") == 0 && shardPeopleOf(atoi(k.c_str() + body.size() + 2)) == people) n++; return n; };
        Guide G; const bool loaded = G.load(path);
        const bool keys = G.shards.count("151,0,25/1/S3") && G.shards.count("151,0,25/1/S12") && G.shards.count("152,0,92/0/S2") && G.shards.count("152,0,92/0/S1007") && G.shards.count("152,0,92/0/S1013") && G.shards.size() == 5;
        const bool others = G.decoded.count("152,0,92/0/S1007") && G.decoded["152,0,92/0/S1007"] == 3 && G.decoded["151,0,25/1/S3"] == 2 && G.heard.count("152,0,92/0/S1007") && G.names.count("152,0,92/0/S1007") && G.lent.count("152,0,92/0/S1010") && G.names.count("151,0,25/1");
        const bool counts = heldOf(G, "151,0,25/1", 0) == 2 && heldOf(G, "152,0,92/0", 0) == 1 && heldOf(G, "152,0,92/0", 1) == 2 && G.migratedShards == 5;
        G.save(path);
        Guide G2; G2.load(path);
        const bool once = G2.migratedShards == 0 && G2.shards == G.shards && G2.decoded == G.decoded;
        // the game says it once on the load
        { std::ofstream f(path); f << "vesperis-guide 1\ngen 12\nshard 152,0,92/0/S57\nshard 152,0,92/0/S3\n"; }
        std::string said, said2;
        {
            TestGame game; game.savePrefix = "shots/tests/test_records_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = path;
            game.loadFromDisk(); said = game.testStatusNext() + " / " + game.testStatus();
        }
        {
            TestGame game; game.savePrefix = "shots/tests/test_records_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = path;
            game.loadFromDisk(); said2 = game.testStatusNext() + " / " + game.testStatus();
        }
        const bool told = said.find("2 SHARDS KEPT") != std::string::npos && said2.find("SHARDS KEPT") == std::string::npos;
        remove(path);
        check("records: an old guide's shards kept by their index across the change (the second people's moved), the counts and the language kept, said once (T-04)",
              loaded && keys && others && counts && once && told,
              fmt("%s; the keys %s, the decoded, heard, named and lent %s, the counts %s (languages %d%%, %d%%), saved and loaded again %s; the load said '%s', the next '%s'",
                  loaded ? "loaded" : "NOT LOADED", keys ? "kept (S57 now S1007)" : "WRONG", others ? "moved with them" : "NOT MOVED", counts ? "kept" : "CHANGED",
                  (int)std::lround(100 * languageShare(heldOf(G, "152,0,92/0", 1))), (int)std::lround(100 * languageShare(heldOf(G, "151,0,25/1", 0))), once ? "unchanged" : "MOVED AGAIN", trunc(said, 52).c_str(), trunc(said2, 40).c_str()));
    }
    {   // a guide of `records 2` (its records dated over their people's eras) loaded: its keys as they were, its shards' contents new, said
        // once; saved as `records 3` and loaded again with nothing to say
        const char* path = "shots/tests/test_records_guide2.txt";
        { std::ofstream f(path); f << "vesperis-guide 1\ngen 13\nrecords 2\nshard 152,0,92/0/S1007\nshard 152,0,92/0/S3\nshard 151,0,25/1/S12\ndecoded 152,0,92/0/S1007 3\n"; }
        Guide G; const bool loaded = G.load(path);
        const bool keys = G.shards.count("152,0,92/0/S1007") && G.shards.count("152,0,92/0/S3") && G.shards.count("151,0,25/1/S12") && G.shards.size() == 3 && G.decoded.count("152,0,92/0/S1007");
        const int migrated = G.migratedShards;
        G.save(path);
        Guide G2; G2.load(path);
        std::string written; { std::ifstream f(path); std::string l; while (std::getline(f, l)) if (l.compare(0, 8, "records ") == 0) written = l; }
        remove(path);
        check("records: a guide of the records before one age a world keeps its keys, its contents new, said once (B-414)", loaded && keys && migrated == 3 && G2.migratedShards == 0 && G2.shards == G.shards && written == "records 3",
              fmt("%s; the keys %s; %d said new; saved as '%s', loaded again with %d", loaded ? "loaded" : "NOT LOADED", keys ? "as they were" : "MOVED", migrated, written.c_str(), G2.migratedShards));
    }
    {   // in the game: a modern people's capital, each of its media and its archive's drawn, offered and taken with E
        std::vector<std::string> lines;
        const int f = recordsScenes(AGE_MODERN, &lines);
        std::string line; for (const std::string& l : lines) line += (line.empty() ? "" : "; ") + l.substr(l.find(' ') + 1, 70);
        check("records: each object drawn, offered and taken with E in a modern capital (T-04; `records scenes` the six ages)", f == 0 && lines.size() >= 3, fmt("%zu: %s", lines.size(), line.c_str()));
    }
}

// H-05: the decoder by age. `decoder ages`: the counts of each category on the six age worlds, by the age of each record's year, and over the
// peoples near home by the age each reached; then the decoder's pages of each age as the explorer sees them (the people's tongue learnt): a
// text on up to two of the media its records of that age were made on (a recording beside one the ship reads, where the age made both) and a
// piece likewise -> shots/tests/decoder_<age>_<text|music>_<medium>.png. `decoder wav`: a text and a piece of the industrial and the modern
// world through each medium (the people's own voice bare, the ship's reading, a wax cylinder, a disc, a reel, a drive, a crystal, the hum)
// -> shots/tests/medium_<medium>_<text|music>.wav, with their levels
std::string fileWord(std::string s) { for (char& c : s) if (c == ' ') c = '_'; return s; }
static void decoderCounts() {
    printf("the counts of each category per world (H-05: past C-02's fifty, music and charts by the age of each record's year):\n");
    for (int a = 0; a < AGE_COUNT; a++) {
        const AgeWorld& W = AGE_WORLDS[a];
        Star s; StarSystem sys; if (!systemAt(W.sx, 0, W.sz, s, sys, true)) continue; if (W.body >= (int)sys.bodies.size()) continue;
        const Body& b = sys.bodies[W.body]; const BodyGen g = BodyGen::make(b);
        std::shared_ptr<const RecordCatalogue> C = recordsOf(g, W.which);
        int n[AGE_COUNT][RC_COUNT] = {}, tot[RC_COUNT] = {};
        for (const RecordEntry& e : C->entries) { n[e.age][e.category]++; tot[e.category]++; }
        printf("  %-11s %-20s the %-12s %3d records: %3d texts, %2d music, %2d charts;", AGE_NAMES[a], b.name.c_str(), peopleNameOf(peopleGen(g, W.which)).c_str(), C->count, tot[RC_TEXT], tot[RC_MUSIC], tot[RC_CHART]);
        for (int k = 0; k < AGE_COUNT; k++) if (n[k][0] + n[k][1] + n[k][2]) printf("  %s %d/%d/%d", AGE_NAMES[k], n[k][RC_TEXT], n[k][RC_MUSIC], n[k][RC_CHART]);
        printf("\n");
    }
    // the peoples near home, by the age each reached: their records' shares of the three
    int peoples[AGE_COUNT] = {}; double rec[AGE_COUNT] = {}, cat[AGE_COUNT][RC_COUNT] = {}, byYear[AGE_COUNT][RC_COUNT] = {};
    int scanned = 0;
    for (int64_t sx = 150; sx <= 230 && scanned < 160; sx++) for (int64_t sz = 20; sz <= 80 && scanned < 160; sz++) {
        Star s; StarSystem sys; if (!systemAt(sx, 0, sz, s, sys, false)) continue;
        for (const Body& b : sys.bodies) {
            if (!kindHas(b.type, KT_PEOPLE)) continue;   // P-04: every people's world (a scorched one's too)
            const BodyGen g = BodyGen::make(b);
            const int np = peoplesOf(g);
            for (int p = 0; p < np; p++) {
                const int age = clampi(peopleAgeOf(g, p).age, 0, AGE_COUNT - 1);
                std::shared_ptr<const RecordCatalogue> C = recordsOf(g, p);
                peoples[age]++; rec[age] += C->count; scanned++;
                for (const RecordEntry& e : C->entries) { cat[age][e.category]++; byYear[e.age][e.category]++; }
            }
        }
    }
    printf("over %d peoples near home (sectors from 150 20 on), by the age each reached: records, and the shares of texts / music / charts:\n", scanned);
    for (int a = 0; a < AGE_COUNT; a++) {
        if (!peoples[a]) continue;
        const double t = cat[a][0] + cat[a][1] + cat[a][2];
        printf("  %-11s %3d peoples, %5.1f records each: %4.1f%% texts, %4.1f%% music, %4.1f%% charts\n", AGE_NAMES[a], peoples[a], rec[a] / peoples[a], 100 * cat[a][0] / t, 100 * cat[a][1] / t, 100 * cat[a][2] / t);
    }
    printf("  and by the age of each record's year (every people's records together):\n");
    for (int a = 0; a < AGE_COUNT; a++) {
        const double t = byYear[a][0] + byYear[a][1] + byYear[a][2];
        if (t > 0) printf("  %-11s %5.0f records: %4.1f%% texts, %4.1f%% music, %4.1f%% charts\n", AGE_NAMES[a], t, 100 * byYear[a][0] / t, 100 * byYear[a][1] / t, 100 * byYear[a][2] / t);
    }
}
static int decoderFrames() {
    int failures = 0;
    for (int a = 0; a < AGE_COUNT; a++) {
        const AgeWorld& W = AGE_WORLDS[a];
        Star s; StarSystem sys; if (!systemAt(W.sx, 0, W.sz, s, sys, true)) { failures++; continue; } if (W.body >= (int)sys.bodies.size()) { failures++; continue; }
        const Body& b = sys.bodies[W.body]; const BodyGen g = BodyGen::make(b);
        std::shared_ptr<const RecordCatalogue> C = recordsOf(g, W.which);
        // the records of this age by medium; two media a category at most, a recording first where the age made one, then one the ship reads
        std::map<int, std::vector<int>> by[2];
        int chartPick = -1;   // and a star chart of the age: the most common medium's, its middle one
        { std::map<int, std::vector<int>> charts; for (int i = 0; i < C->count; i++) { const RecordEntry& e = C->entries[i]; if (e.age == a && e.category == RC_CHART) charts[e.medium].push_back(i); }
          size_t most = 0; for (const auto& kv : charts) if (kv.second.size() > most) { most = kv.second.size(); chartPick = kv.second[kv.second.size() / 2]; } }
        for (int i = 0; i < C->count; i++) { const RecordEntry& e = C->entries[i]; if (e.age != a || e.last || e.category == RC_CHART) continue; by[e.category == RC_MUSIC][e.medium].push_back(i); }
        struct Pick { int cat, medium, index; };
        std::vector<Pick> picks;
        for (int c = 0; c < 2; c++) {
            int sounds = -1, silent = -1; size_t ns = 0, nq = 0;
            for (const auto& kv : by[c]) { if (recordMediumSounds(kv.first)) { if (kv.second.size() > ns) { ns = kv.second.size(); sounds = kv.first; } } else if (kv.second.size() > nq) { nq = kv.second.size(); silent = kv.first; } }
            if (sounds >= 0) picks.push_back({c, sounds, by[c][sounds][by[c][sounds].size() / 2]});
            if (silent >= 0) picks.push_back({c, silent, by[c][silent][by[c][silent].size() / 2]});
            if (sounds < 0 || silent < 0) for (const auto& kv : by[c]) if (kv.first != sounds && kv.first != silent && (int)picks.size() < 2 * (c + 1)) picks.push_back({c, kv.first, kv.second[kv.second.size() / 2]});
        }
        if (chartPick >= 0) picks.push_back({2, C->entries[chartPick].medium, chartPick});
        printf("== %s: %s, the %s (%d records)\n", AGE_NAMES[a], b.name.c_str(), peopleNameOf(peopleGen(g, W.which)).c_str(), C->count);
        if (picks.empty()) { printf("  no record of the age on this world\n"); continue; }
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide_decoder.txt";
        remove("shots/tests/test_guide_decoder.txt");
        applyTestScale(game); game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFrames(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        std::set<int> held; for (const Pick& p : picks) held.insert(p.index);
        for (int i = 0; i < C->count && (int)held.size() < languageFull(C->count); i++) if (C->entries[i].category == RC_TEXT) held.insert(i);   // the people's tongue learnt
        const std::string key = Guide::bodyKey(s.sx, s.sy, s.sz, W.body) + "/S";
        for (int i : held) game.guide.shards.insert(key + std::to_string(shardWorldIndex(W.which, i)));
        game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1); press(KEY_ENTER); run(0.2);
        for (const Pick& p : picks) {
            if (!game.testShardsOpenIndex(p.index)) { printf("  S%d NOT opened\n", p.index); failures++; continue; }
            if (p.cat == 0) { run(0.1); press(KEY_ENTER); run(0.1); press(KEY_ENTER); run(1.6); }   // the first reading skipped, then the page spoken
            else if (p.cat == 2) { run(0.1); press(KEY_ENTER); run(0.3); }   // a chart's reading skipped
            else run(2.5);
            static const char* const CAT[3] = {"text", "music", "chart"};
            const std::string name = fmt("decoder_%s_%s_%s", fileWord(AGE_NAMES[a]).c_str(), CAT[p.cat], fileWord(RECORD_MEDIUM_NAMES[p.medium]).c_str());
            writePNG(("shots/tests/" + name + ".png").c_str(), game.output(), FBW, FBH);
            const std::string medium = game.audio.medium >= 0 ? RECORD_MEDIUM_NAMES[game.audio.medium] : "-";
            if (p.cat == 2) { printf("  %-44s S%-3d a star chart: %s\n", (name + ".png").c_str(), p.index, game.testChartInfo().c_str()); press(KEY_ESCAPE); run(0.1); continue; }
            printf("  %-44s S%-3d %s; the synth's medium %s, %s\n", (name + ".png").c_str(), p.index, recordMediumSounds(p.medium) ? "a recording" : "no sound of its own", medium.c_str(),
                   p.cat ? (game.audio.piece ? "a piece playing" : "NO piece") : (game.audio.speech ? (game.audio.voice && game.audio.voice->read ? "read by the ship" : "its own voice") : "NOT speaking"));
            if (!game.audio.speech && !game.audio.piece) failures++;
            press(KEY_ESCAPE); run(0.1);
        }
    }
    return failures;
}
static int decoderWavs() {
    makeDir("shots"); makeDir("shots/tests");
    int failures = 0;
    printf("a text and a piece through each medium (the levels as the decoder plays them, the master at the game's 0.8):\n");
    for (int a : {AGE_INDUSTRIAL, AGE_MODERN}) {
        const AgeWorld& W = AGE_WORLDS[a];
        Star s; StarSystem sys; if (!systemAt(W.sx, 0, W.sz, s, sys, true)) continue; const Body& b = sys.bodies[W.body]; const BodyGen g = BodyGen::make(b);
        Lore L = loreOf(sys, b, g, W.which); Tongue T = tongueOf(g, L);
        std::vector<Shard> sh; shardsOf(sys, b, g, L, L.records, sh);
        Language lang; languageOf(L, sh, T, lang);
        const Voice V = voiceOf(g, L, T), R = voiceRead(V);
        Tradition Tr = traditionOf(g, L);
        int ti = -1, mi = -1;
        for (int i = (int)sh.size() - 1; i >= 0 && (ti < 0 || mi < 0); i--) { if (ti < 0 && !sh[i].music && !sh[i].chart && shardWords(sh[i].text) >= 25) ti = i; if (mi < 0 && sh[i].music) mi = i; }
        if (ti < 0 || mi < 0) { failures++; continue; }
        std::vector<DecodedWord> w; decodeShard(sh[ti], lang, T, languageFull(L.records), w); w.resize(shardOpening(w));
        Piece P; pieceOf(Tr, sh[mi], P);
        printf("  the %s world (%s, the %s): text S%d (%zu words), piece S%d (%s, %.0f s)\n", AGE_NAMES[a], b.name.c_str(), L.people.c_str(), ti, w.size(), mi, PIECE_FORM_NAMES[P.form], P.seconds);
        {   // the ship's reading is flatter: the spread of the vowels' pitch, its own voice against the ship's
            auto spread = [&](const Voice& v) { Speech S; speechOf(v, w, sh[ti].seed, S); double sum = 0, sq = 0; int n = 0; for (const Segment& g : S.segs) if (g.phone >= 0 && PHONES[g.phone].kind == PK_VOWEL) { sum += g.pitch; sq += g.pitch * g.pitch; n++; } const double mu = sum / std::max(1, n); return std::sqrt(std::max(0.0, sq / std::max(1, n) - mu * mu)); };
            const double own = spread(V), read = spread(R);
            printf("    the voice's pitch spread over the vowels: its own %.3f, the ship's reading %.3f (%.0f%%)\n", own, read, 100 * read / std::max(own, 1e-9));
            if (!(read < 0.6 * own)) failures++;
        }
        const int media[8] = {-1, -2, RM_CYLINDER, RM_DISC, RM_REEL, RM_DRIVE, RM_CRYSTAL, RM_HUM};   // -1 the own voice bare, -2 the ship's reading
        std::vector<int16_t> bare[2];   // the bare renders: their quiet windows (the pauses between words, the rests) measure each medium's surface
        for (int k = 0; k < 8; k++) {
            const int m = media[k];
            const std::string mname = m == -1 ? "bare" : (m == -2 ? "read_by_the_ship" : fileWord(RECORD_MEDIUM_NAMES[m]));
            for (int cat = 0; cat < 2; cat++) {
                if (cat == 1 && m == -2) continue;   // a piece the ship plays from notation is the bare piece
                AudioSynth synth; AudioState st; Speech S;
                if (cat == 0) { speechOf(m == -2 ? R : V, w, sh[ti].seed, S); st.speech = &S; st.voice = m == -2 ? &R : &V; st.speechStart = true; }
                else { st.piece = &P; st.tradition = &Tr; st.pieceStart = true; }
                st.medium = m >= 0 ? m : -1;
                const double secs = cat == 0 ? S.seconds + 1.5 : std::min(P.seconds + 2.0, 40.0);
                std::vector<int16_t> pcm; RenderStats r = renderReceiver(synth, st, secs, &pcm);
                const std::string fn = fmt("shots/tests/medium_%s_%s_%s.wav", fileWord(AGE_NAMES[a]).c_str(), mname.c_str(), cat ? "music" : "text");
                writeWav16(fn, pcm, 22050);
                const bool bad = r.nan > 0 || r.peak > 0.95 || r.rms < 0.02 || r.rms > 0.2;
                if (bad) failures++;
                if (m == -1) bare[cat] = pcm;
                double floorSum = 0; long floorN = 0;   // the surface: this render where the bare one is silent (50 ms windows under 1/500 of full scale)
                const int win = 1102;
                for (size_t w0 = 0; w0 + win <= std::min(pcm.size(), bare[cat].size()); w0 += win) {
                    double e = 0; for (int j = 0; j < win; j++) { const double v = bare[cat][w0 + j] / 32767.0; e += v * v; }
                    if (std::sqrt(e / win) > 0.002) continue;
                    for (int j = 0; j < win; j++) { const double v = pcm[w0 + j] / 32767.0; floorSum += v * v; } floorN += win;
                }
                const std::string surf = m == -2 || !floorN ? std::string("  -   ") : fmt("%.4f", std::sqrt(floorSum / floorN));   // the ship's reading pauses elsewhere
                printf("    %-20s %-5s %5.1f s  peak %.3f  rms %.3f  surface %s  %d non-finite  %d at the ceiling%s  -> %s\n", mname.c_str(), cat ? "music" : "text", secs, r.peak, r.rms, surf.c_str(), r.nan, r.clip, bad ? "  OUT" : "", fn.c_str());
            }
        }
    }
    return failures;
}
// H-05: the unit's checks of the media: on the decoder a record with no sound of its own is read by the ship (its flat voice) and a recording
// speaks in its people's voice through what it was made on, a piece likewise, each named with its medium; through the synth every medium keeps
// the content's level and its surface where it should be (a wax cylinder's crackle and hiss heard, a drive silent but for its rot)
void mediaChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    const AgeWorld& W = AGE_WORLDS[AGE_INDUSTRIAL];
    Star s; StarSystem sys; bool world = starInSector(W.sx, 0, W.sz, s, true);
    if (world) { sys.generate(s); world = W.body < (int)sys.bodies.size(); }
    if (!world) { check("media: the decoder's (H-05)", false, "no industrial age world"); return; }
    const Body& b = sys.bodies[W.body]; const BodyGen g = BodyGen::make(b);
    std::shared_ptr<const RecordCatalogue> C = recordsOf(g, W.which);
    int want[4] = {-1, -1, -1, -1};   // a written text, a recorded text, notation, a recorded piece (of the industrial years)
    for (int i = 0; i < C->count; i++) {
        const RecordEntry& e = C->entries[i]; if (e.age != AGE_INDUSTRIAL || e.last || e.category == RC_CHART) continue;
        const int k = (e.category == RC_MUSIC ? 2 : 0) + (recordMediumSounds(e.medium) ? 1 : 0);
        if (want[k] < 0) want[k] = i;
    }
    {   // on the decoder
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide_media.txt";
        remove("shots/tests/test_guide_media.txt");
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFrames(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        const std::string key = Guide::bodyKey(s.sx, s.sy, s.sz, W.body) + "/S";
        for (int k = 0; k < 4; k++) if (want[k] >= 0) game.guide.shards.insert(key + std::to_string(shardWorldIndex(W.which, want[k])));
        game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1); press(KEY_ENTER); run(0.2);
        int ok = 0; std::string detail;
        static const char* const WHAT[4] = {"written", "recorded", "notation", "a recorded piece"};
        for (int k = 0; k < 4; k++) {
            if (want[k] < 0 || !game.testShardsOpenIndex(want[k])) { detail += fmt("%s%s NOT opened", detail.empty() ? "" : "; ", WHAT[k]); continue; }
            run(0.3);
            const int m = C->entries[want[k]].medium;
            const bool text = k < 2, sounds = k % 2 == 1;
            bool good = game.audio.medium == m;
            if (text) good = good && game.audio.speech && game.audio.voice && game.audio.voice->read == !sounds;
            else good = good && game.audio.piece;
            if (good) ok++;
            detail += fmt("%s%s S%d on %s: %s, the synth's medium %s", detail.empty() ? "" : "; ", WHAT[k], want[k], RECORD_MEDIUM_NAMES[m],
                          text ? (game.audio.voice ? (game.audio.voice->read ? "read by the ship" : "its own voice") : "NOT spoken") : (game.audio.piece ? "playing" : "NOT playing"), game.audio.medium >= 0 ? RECORD_MEDIUM_NAMES[game.audio.medium] : "none");
            press(KEY_ESCAPE); run(0.1);
        }
        const bool cleared = game.audio.medium < 0;
        check("media: on the decoder a record with no sound of its own is read or played by the ship, a recording heard through what it was made on (H-05)", ok == 4 && cleared,
              fmt("%s; after: %s", detail.c_str(), cleared ? "the medium cleared" : "the medium LEFT SET"));
    }
    {   // through the synth
        Lore L = loreOf(sys, b, g, W.which); Tongue T = tongueOf(g, L);
        std::vector<Shard> sh; shardsOf(sys, b, g, L, L.records, sh);
        Language lang; languageOf(L, sh, T, lang);
        const Voice V = voiceOf(g, L, T), R = voiceRead(V);
        Tradition Tr = traditionOf(g, L);
        int ti = -1, mi = -1;
        for (int i = (int)sh.size() - 1; i >= 0 && (ti < 0 || mi < 0); i--) { if (ti < 0 && !sh[i].music && !sh[i].chart && shardWords(sh[i].text) >= 25) ti = i; if (mi < 0 && sh[i].music) mi = i; }
        bool ok = ti >= 0 && mi >= 0; std::string detail;
        double surf[RM_COUNT] = {};
        if (ok) {
            std::vector<DecodedWord> w; decodeShard(sh[ti], lang, T, languageFull(L.records), w); w.resize(shardOpening(w));
            Piece P; pieceOf(Tr, sh[mi], P);
            auto spread = [&](const Voice& v) { Speech S; speechOf(v, w, sh[ti].seed, S); double sum = 0, sq = 0; int n = 0; for (const Segment& sg : S.segs) if (sg.phone >= 0 && PHONES[sg.phone].kind == PK_VOWEL) { sum += sg.pitch; sq += sg.pitch * sg.pitch; n++; } const double mu = sum / std::max(1, n); return std::sqrt(std::max(0.0, sq / std::max(1, n) - mu * mu)); };
            const double own = spread(V), read = spread(R);
            if (!(read < 0.6 * own)) ok = false;
            detail = fmt("the ship's reading %.0f%% of the voice's pitch spread", 100 * read / std::max(own, 1e-9));
            std::vector<int16_t> bare;
            for (int m : {-1, (int)RM_CYLINDER, (int)RM_DISC, (int)RM_REEL, (int)RM_DRIVE, (int)RM_CRYSTAL, (int)RM_HUM}) {
                for (int cat = 0; cat < 2; cat++) {
                    AudioSynth synth; AudioState st; Speech S;
                    if (cat == 0) { speechOf(V, w, sh[ti].seed, S); st.speech = &S; st.voice = &V; st.speechStart = true; } else { st.piece = &P; st.tradition = &Tr; st.pieceStart = true; }
                    st.medium = m;
                    std::vector<int16_t> pcm; RenderStats r = renderReceiver(synth, st, cat == 0 ? std::min(S.seconds, 9.0) : 8.0, &pcm);
                    if (r.nan || r.peak > 0.95 || r.rms < 0.02 || r.rms > 0.2) { ok = false; detail += fmt("; %s %s OUT (peak %.2f, rms %.3f)", m < 0 ? "bare" : RECORD_MEDIUM_NAMES[m], cat ? "music" : "text", r.peak, r.rms); }
                    if (cat == 0 && m < 0) bare = pcm;
                    if (cat == 0 && m >= 0) {   // the surface in the bare speech's pauses
                        double e2 = 0; long n = 0; const int win = 1102;
                        for (size_t w0 = 0; w0 + win <= std::min(pcm.size(), bare.size()); w0 += win) {
                            double e = 0; for (int j = 0; j < win; j++) { const double v = bare[w0 + j] / 32767.0; e += v * v; }
                            if (std::sqrt(e / win) > 0.002) continue;
                            for (int j = 0; j < win; j++) { const double v = pcm[w0 + j] / 32767.0; e2 += v * v; } n += win;
                        }
                        surf[m] = n ? std::sqrt(e2 / n) : -1;
                    }
                }
            }
            if (!(surf[RM_CYLINDER] > 0.006 && surf[RM_DISC] < surf[RM_CYLINDER] && surf[RM_REEL] > 0.002 && surf[RM_DRIVE] >= 0 && surf[RM_DRIVE] < 0.002 && surf[RM_HUM] > 0.005)) ok = false;
            detail += fmt("; the surfaces: wax cylinder %.4f, disc %.4f, reel %.4f, drive %.4f, crystal %.4f, the hum %.4f", surf[RM_CYLINDER], surf[RM_DISC], surf[RM_REEL], surf[RM_DRIVE], surf[RM_CRYSTAL], surf[RM_HUM]);
        }
        check("media: through the synth each medium keeps the level, its surface where it should be, the ship's reading flatter (H-05)", ok, detail);
    }
}

// H-06: the decoder's pages for a people of each age (`decoder pages`): every third of its records held and four in five of them read (the
// last recording too from the modern age on, so the history closes), the records page, the timeline with the cursor halfway, the history
// page at its top and at its end; the modern people's records page showing its music alone and its history after three records read; each
// history page's lines in `shots/tests/decoder_history_<age>.txt` for reading
static int decoderPages() {
    int failures = 0;
    for (int a = 0; a < AGE_COUNT; a++) {
        const AgeWorld& W = AGE_WORLDS[a];
        Star s; StarSystem sys; if (!systemAt(W.sx, 0, W.sz, s, sys, true)) { failures++; continue; } if (W.body >= (int)sys.bodies.size()) { failures++; continue; }
        const Body& b = sys.bodies[W.body]; const BodyGen g = BodyGen::make(b);
        std::shared_ptr<const RecordCatalogue> C = recordsOf(g, W.which);
        const bool closes = a >= AGE_MODERN;
        printf("== %s: %s, the %s (%d records)%s\n", AGE_NAMES[a], b.name.c_str(), peopleNameOf(peopleGen(g, W.which)).c_str(), C->count, closes ? ", the last recording read" : "");
        for (int early = 0; early < (a == AGE_MODERN ? 2 : 1); early++) {
            TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide_pages.txt";
            remove("shots/tests/test_guide_pages.txt");
            applyTestScale(game); game.newGame(); game.setState(GameState::SPACE);
            Input gi;
            auto run = [&](double secs) { runFrames(game, gi, secs); };
            auto press = [&](int key) { pressKey(game, gi, key); };
            const std::string wkey = Guide::bodyKey(s.sx, s.sy, s.sz, W.body), key = wkey + "/S";
            std::vector<int> held;
            if (early) { for (int i = C->count - 1; i >= 0 && held.size() < 3; i--) if (C->entries[i].category == RC_TEXT && !C->entries[i].last) held.push_back(i); }   // three of its latest texts
            else { for (int i = 0; i < C->count; i += 3) if (!C->entries[i].last) held.push_back(i); if (closes && C->last >= 0) held.push_back(C->last); }
            for (size_t j = 0; j < held.size(); j++) {
                const std::string k = key + std::to_string(shardWorldIndex(W.which, held[j]));
                game.guide.shards.insert(k);
                if (early || j % 5 != 4 || held[j] == C->last) game.guide.decoded[k] = (int)held.size();
            }
            if (closes && !early) game.guide.ended.insert(wkey + (W.which ? "/P1" : ""));
            game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1); press(KEY_ENTER); run(0.2);
            const std::string base = fmt("shots/tests/decoder_pages_%s", fileWord(AGE_NAMES[a]).c_str()) + (early ? "_early" : "");
            auto shot = [&](const char* what) { const std::string fn = base + "_" + what + ".png"; writePNG(fn.c_str(), game.output(), FBW, FBH); printf("  %s\n", fn.c_str()); };
            if (!early) {
                shot("records");
                if (a == AGE_MODERN) { press(KEY_RIGHT); press(KEY_RIGHT); run(0.1); shot("records_music"); printf("    the music alone: the record under the cursor S%d is %s\n", game.testShardSel(), game.testShardSel() >= 0 && C->entries[game.testShardSel()].category == RC_MUSIC ? "music" : "NOT music"); press(KEY_LEFT); press(KEY_LEFT); run(0.1); }
            }
            press(KEY_TAB); run(0.1);
            for (int k = 0; k < (int)held.size() / 2; k++) press(KEY_RIGHT);
            run(0.1); shot("timeline");
            press(KEY_TAB); run(0.1); shot("history");
            printf("    %s\n", game.testHistoryInfo().c_str());
            if (game.testShardsPage() != 2) { printf("    NOT on the history page\n"); failures++; }
            const ToldHistory& T = game.testTold();
            if (!early) {
                for (size_t k = 0; k + 1 < T.events.size(); k++) press(KEY_DOWN);
                run(0.1); shot("history_end");
            }
            const std::vector<std::string> lines = game.testLegendLines();
            int subs = 0; for (const std::string& l : lines) for (const char* w : {" a man", " a woman", " a child", " a ruler", "A man", "A woman", "A child", "A ruler"}) { size_t p = 0; while ((p = l.find(w, p)) != std::string::npos) { subs++; p++; } }
            const std::string fn = base.substr(0, base.find("decoder_pages_")) + "decoder_history_" + fileWord(AGE_NAMES[a]) + (early ? "_early" : "") + ".txt";
            if (FILE* f = fopen(fn.c_str(), "w")) { fprintf(f, "%s\n\n", game.testHistoryInfo().c_str()); for (const std::string& l : lines) fprintf(f, "%s\n", l.c_str()); fclose(f); }
            printf("    %zu rows (%d persons by what they were), written to %s\n", lines.size(), subs, fn.c_str());
            for (size_t k = 0; k < lines.size() && k < 8; k++) printf("      %s\n", lines[k].c_str());
            if (T.read == 0 || (!early && T.events.empty())) failures++;
            if (!early) {   // Enter on the last event opens the record that told it
                press(KEY_ENTER); run(0.2);
                const int sel = game.testShardSel();
                const bool told = !T.events.empty() && std::find(T.events.back().by.begin(), T.events.back().by.end(), sel) != T.events.back().by.end();
                printf("    Enter on the last event: level %d, the record S%d %s\n", game.testShardsLevel(), sel, told ? "told it" : "DID NOT tell it");
                if (game.testShardsLevel() != 2 || !told) failures++;
            }
        }
    }
    return failures;
}

// H-06: the unit's checks of the decoder's history: the page holds the events the read records told and nothing an unread one told, names a
// person only as a read record wrote them, places an era once a record written in it is read, names the age once three records of it are read,
// grows when a record is read, and Enter on an event opens the record that told it; the records page shows one kind on Left/Right
void historyPageChecks(const std::function<void(const char*, bool, const std::string&)>& check) {
    const AgeWorld& W = AGE_WORLDS[AGE_INDUSTRIAL];
    Star s; StarSystem sys; bool world = starInSector(W.sx, 0, W.sz, s, true);
    if (world) { sys.generate(s); world = W.body < (int)sys.bodies.size(); }
    if (!world) { check("history: the decoder's history page (H-06)", false, "no industrial age world"); return; }
    const Body& b = sys.bodies[W.body]; const BodyGen g = BodyGen::make(b);
    const Lore L = loreOf(sys, b, g, W.which);
    std::shared_ptr<const RecordCatalogue> C = recordsOf(g, W.which);
    std::shared_ptr<const ShardTexts> TX = shardTextsOf(sys, b, g, L);
    std::shared_ptr<const History> HP = historyOf(sys, b, g, W.which);
    if (!C || !TX || !HP) { check("history: the decoder's history page (H-06)", false, "no records, texts or history"); return; }
    const History& H = *HP;
    const int lastAge = H.eras.back().age;
    // held: every fourth record but the last recording; read: those of the earlier ages and two texts of the last age (the age not told yet);
    // left unread: the rest, among them a text of the last age that tells something (read in the second part)
    std::vector<int> held, read, unread; int lastAgeRead = 0, next = -1;
    for (int i = 0; i < C->count; i += 4) if (!C->entries[i].last) held.push_back(i);
    for (int i : held) {
        const RecordEntry& e = C->entries[i];
        if (e.age != lastAge) { read.push_back(i); continue; }
        if (e.category == RC_TEXT && !TX->items[i].told.empty()) { if (lastAgeRead < 2) { read.push_back(i); lastAgeRead++; continue; } if (next < 0) { next = i; continue; } }
        unread.push_back(i);
    }
    TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide_history.txt";
    remove("shots/tests/test_guide_history.txt");
    game.newGame(); game.setState(GameState::SPACE);
    Input gi;
    auto run = [&](double secs) { runFrames(game, gi, secs); };
    auto press = [&](int key) { pressKey(game, gi, key); };
    const std::string key = Guide::bodyKey(s.sx, s.sy, s.sz, W.body) + "/S";
    for (int i : held) game.guide.shards.insert(key + std::to_string(shardWorldIndex(W.which, i)));
    for (int i : read) game.guide.decoded[key + std::to_string(shardWorldIndex(W.which, i))] = (int)held.size();
    game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1); press(KEY_ENTER); run(0.2);
    // what the read records told, worked out here from their texts
    auto toldBy = [&](const std::vector<int>& recs, std::set<int>& events, std::set<std::string>& names) {
        for (int i : recs) { const ShardText& t = TX->items[i]; if (t.text.empty()) continue; for (int ev : t.told) events.insert(ev); for (const std::string& n : t.rec.names) names.insert(n); } };
    auto pageCheck = [&](const char* when, const std::vector<int>& readNow, const std::vector<int>& unreadNow, bool ageWanted) {
        const ToldHistory& T = game.testTold();
        std::set<int> want, other; std::set<std::string> names, otherNames;
        toldBy(readNow, want, names); toldBy(unreadNow, other, otherNames);
        std::set<int> page; for (const ToldEvent& t : T.events) page.insert(t.event);
        int extra = 0, missing = 0, leaked = 0;
        for (int ev : page) if (!want.count(ev)) { extra++; if (other.count(ev)) leaked++; }
        for (int ev : want) if (!page.count(ev)) missing++;
        // a person in the lines by name only when a read record wrote it
        std::string text; for (const std::string& l : game.testLegendLines()) text += " " + l;
        for (char& c : text) c = (char)std::tolower((unsigned char)c);
        std::set<int> persons; for (int ev : page) { const HistoryEvent& e = H.events[ev]; for (int p : {e.a, e.b, e.c}) if (p >= 0) persons.insert(p); }
        int namedOk = 0, unnamedHidden = 0, badName = 0; std::string bad;
        for (int p : persons) {
            const std::string full = historyPersonName(H, p, false);
            std::string given = full.substr(0, full.find(' '));
            { const size_t sp = full.find(' '); if (sp != std::string::npos) { const std::string w2 = full.substr(sp + 1, full.find(' ', sp + 1) - sp - 1); if (!w2.empty() && w2.find_first_not_of("IVXLC") == std::string::npos) given = full.substr(0, full.find(' ', sp + 1)); } }
            const bool written = names.count(full) || names.count(given);
            std::string fl = "{" + full + "}"; for (char& c : fl) c = (char)std::tolower((unsigned char)c);
            std::string fl2 = full; for (char& c : fl2) c = (char)std::tolower((unsigned char)c);
            const bool shown = text.find(fl2) != std::string::npos || text.find(fl) != std::string::npos;
            if (written) namedOk++;
            else if (!shown) unnamedHidden++;
            else { badName++; if (bad.size() < 120) bad += " " + full; }
        }
        // the eras placed: those a read record was written in, and (B-414) the earlier ones a read history told
        std::set<int> erasWant;
        for (int i : readNow) {
            const int k = H.eraAt(TX->items[i].year); if (k >= 0) erasWant.insert(k);
            for (int ev : TX->items[i].told) if (ev >= 0 && ev < (int)H.events.size()) { const int ke = H.eraAt(H.events[ev].year); if (ke >= 0 && ke < k) erasWant.insert(ke); }
        }
        int eraBad = 0; for (int k = 0; k < (int)H.eras.size(); k++) if ((k < (int)T.eraRead.size() && T.eraRead[k] > 0) != (erasWant.count(k) > 0)) eraBad++;
        // the people's ways: those the read records expressed and no other
        std::set<std::string> waysWant; for (int i : readNow) for (const auto& w : TX->items[i].ways) waysWant.insert(w.first);
        std::set<std::string> waysPage; for (const auto& w : T.ways) waysPage.insert(w.first);
        std::string wayList; for (const std::string& w : waysPage) wayList += (wayList.empty() ? "" : " ") + w;
        check(fmt("history: the page holds what the read records told and nothing else, %s (H-06)", when).c_str(), extra == 0 && missing == 0 && !page.empty(),
              fmt("%zu read of %zu held: %zu events told by them, %zu on the page (%d not told by a read record, %d of them an unread one's; %d missing); %zu events only the unread told", readNow.size(), held.size(), want.size(), page.size(), extra, leaked, missing, (size_t)std::count_if(other.begin(), other.end(), [&](int ev) { return !want.count(ev); })));
        check(fmt("history: a person named only as a read record wrote them, the eras placed by the records read, the age %s, %s (H-06)", ageWanted ? "told" : "not told yet", when).c_str(),
              badName == 0 && eraBad == 0 && T.ageTold == ageWanted && waysPage == waysWant,
              fmt("%zu persons in the events: %d named by the records, %d by what they were, %d named WITHOUT a record%s; eras wrong %d; the age %s (%d records of its age read); the ways told %s (%s)", persons.size(), namedOk, unnamedHidden, badName, bad.c_str(), eraBad,
                  T.ageTold ? "told" : "not told", T.ageRead[std::max(0, std::min(AGE_COUNT - 1, T.age))], waysPage == waysWant ? "as read" : "NOT as read", wayList.c_str()));
    };
    press(KEY_TAB); press(KEY_TAB); run(0.1);
    if (game.testShardsPage() != 2) { check("history: Tab turns the decoder's pages to the history (H-06)", false, fmt("page %d", game.testShardsPage())); return; }
    pageCheck("with the records read so far", read, unread, false);
    if (next >= 0) {   // a third record of the last age read on the decoder: the page grows by what it told, the age is told
        std::vector<int> unread2 = unread; std::vector<int> read2 = read; read2.push_back(next);
        const size_t before = game.testTold().events.size();
        press(KEY_TAB); run(0.1);   // to the records page
        press(KEY_ESCAPE); run(0.1); press(KEY_ENTER); run(0.2);   // the world opened again
        const bool opened = game.testShardsOpenIndex(next);
        run(0.2); press(KEY_ENTER); run(0.2);   // the first reading's wait skipped: read
        press(KEY_ESCAPE); run(0.1);
        press(KEY_TAB); press(KEY_TAB); run(0.1);
        const size_t after = game.testTold().events.size();
        check("history: a record read on the decoder adds what it told (H-06)", opened && after > before, fmt("S%d %s; events %zu -> %zu", next, opened ? "read" : "NOT opened", before, after));
        pageCheck("after one more read", read2, unread2, true);
        {   // Enter on the event under the cursor opens a record that told it
            const ToldHistory& T = game.testTold();
            const int k = (int)T.events.size() / 2;
            for (int j = 0; j < k; j++) press(KEY_DOWN);
            press(KEY_ENTER); run(0.2);
            const int sel = game.testShardSel();
            const bool ok = game.testShardsLevel() == 2 && k < (int)T.events.size() && std::find(T.events[k].by.begin(), T.events[k].by.end(), sel) != T.events[k].by.end();
            check("history: Enter on an event opens the record that told it (H-06)", ok, fmt("event %d of %zu: level %d, S%d", k, T.events.size(), game.testShardsLevel(), sel));
            press(KEY_ESCAPE); run(0.1);
        }
    } else check("history: a record read on the decoder adds what it told (H-06)", false, "no unread text of the last age that tells something among the held");
    {   // the records page: Right shows the music alone, Up/Down keep to it
        while (game.testShardsPage() != 0) press(KEY_TAB);
        press(KEY_RIGHT); press(KEY_RIGHT); run(0.1);
        int music = 0, steps = 6;
        for (int j = 0; j < steps; j++) { const int sel = game.testShardSel(); if (sel >= 0 && C->entries[sel].category == RC_MUSIC) music++; press(KEY_DOWN); }
        const bool any = std::any_of(held.begin(), held.end(), [&](int i) { return C->entries[i].category == RC_MUSIC; });
        check("history: the records page shows one kind (H-06)", !any || music == steps, fmt("%d of %d steps on music", music, steps));
    }
    {   // Shift with an arrow held runs the timeline's and the history's cursor (a tap of it one step, a second's hold a score), the run
        // stopping at the ends where the arrow alone goes round (the timeline) or stays (the history)
        auto hold = [&](int key, int frames) {   // Shift and the arrow down together for `frames` frames at 30 a second, then both let go
            gi.down[KEY_LEFT_SHIFT] = true; gi.down[key] = true; gi.pressed[key] = true;
            for (int f = 0; f < frames; f++) { game.frame(gi, 1.0 / 30); gi.newFrame(); }
            gi.down[KEY_LEFT_SHIFT] = false; gi.down[key] = false; game.frame(gi, 1.0 / 30); gi.newFrame();
        };
        while (game.testShardsPage() != 1) press(KEY_TAB);
        const int n = game.testShardsHeld();
        hold(KEY_LEFT, 30 * (3 + n / 100)); const int t0 = game.testShardCursor();
        hold(KEY_RIGHT, 1); const int t1 = game.testShardCursor();
        hold(KEY_RIGHT, 30); const int t2 = game.testShardCursor();
        hold(KEY_RIGHT, 30 * (3 + n / 100)); const int t3 = game.testShardCursor();
        press(KEY_RIGHT); const int t4 = game.testShardCursor();
        check("history: Shift held with an arrow runs the timeline, a tap one record, stopping at the ends; the arrow alone goes round (R-416)",
              n >= 15 && t0 == 0 && t1 == 1 && t2 - t1 >= std::min(12, n - 1 - t1) && t2 - t1 <= 30 && t3 == n - 1 && t4 == 0,
              fmt("%d records held: run back to %d, a tap to %d, a second's run on to %d (+%d), a long one to %d, Right alone to %d", n, t0, t1, t2, t2 - t1, t3, t4));
        for (int i : held) game.guide.decoded[key + std::to_string(shardWorldIndex(W.which, i))] = (int)held.size();   // every record held read: more events told
        press(KEY_ESCAPE); run(0.1); press(KEY_ENTER); run(0.2);   // the world opened again, its pages read again
        while (game.testShardsPage() != 2) press(KEY_TAB);
        run(0.1);
        const int ne = (int)game.testTold().events.size();
        hold(KEY_UP, 30 * (3 + ne / 100)); const int h0 = game.testLegendSel();
        hold(KEY_DOWN, 1); const int h1 = game.testLegendSel();
        hold(KEY_DOWN, 30); const int h2 = game.testLegendSel();
        hold(KEY_DOWN, 30 * (3 + ne / 100)); const int h3 = game.testLegendSel();
        press(KEY_DOWN); const int h4 = game.testLegendSel();
        check("history: Shift held with an arrow runs the history's events, a tap one, stopping at the ends (R-416)",
              game.testShardsPage() == 2 && ne >= 15 && h0 == 0 && h1 == 1 && h2 - h1 >= std::min(12, ne - 1 - h1) && h2 - h1 <= 30 && h3 == ne - 1 && h4 == ne - 1,
              fmt("page %d, %d events: run up to %d, a tap to %d, a second's run on to %d (+%d), a long one to %d, Down alone %d", game.testShardsPage(), ne, h0, h1, h2, h2 - h1, h3, h4));
        // (the user: "in shards decoder also add this fast shift scrolling for records tab"): Shift and Up/Down run the records
        // page's cursor through the kind shown, stopping at the ends where Down alone goes round; the music alone shown, the run keeps to it
        while (game.testShardsPage() != 0) press(KEY_TAB);
        while (game.testShardsFilter() != 0) press(KEY_LEFT);
        run(0.1);
        writePNG("shots/tests/decoder_pages_records_hint.png", game.output(), FBW, FBH);
        hold(KEY_UP, 30 * (3 + n / 100)); const int r0 = game.testShardCursor();
        hold(KEY_DOWN, 1); const int r1 = game.testShardCursor();
        hold(KEY_DOWN, 30); const int r2 = game.testShardCursor();
        hold(KEY_DOWN, 30 * (3 + n / 100)); const int r3 = game.testShardCursor();
        press(KEY_DOWN); const int r4 = game.testShardCursor();
        press(KEY_RIGHT); press(KEY_RIGHT); run(0.1);   // the music alone
        const bool anyMusic = std::any_of(held.begin(), held.end(), [&](int i) { return C->entries[i].category == RC_MUSIC; });
        hold(KEY_DOWN, 30 * (3 + n / 100)); const int mLast = game.testShardSel();
        press(KEY_DOWN); const int mFirst = game.testShardSel();   // Down alone round to the first
        hold(KEY_DOWN, 30 * (3 + n / 100)); hold(KEY_UP, 30 * (3 + n / 100)); const int mUp = game.testShardSel();   // run to the last and back: the first
        const bool musicOk = !anyMusic || (game.testShardsFilter() == 2 && mLast >= 0 && mFirst >= 0 && C->entries[mLast].category == RC_MUSIC && C->entries[mFirst].category == RC_MUSIC && mUp == mFirst);
        check("history: Shift held with Up/Down runs the records page through the kind shown, stopping at the ends; Down alone goes round (R-417)",
              game.testShardsPage() == 0 && r0 == 0 && r1 == 1 && r2 - r1 >= std::min(12, n - 1 - r1) && r2 - r1 <= 30 && r3 == n - 1 && r4 == 0 && musicOk,
              fmt("page %d, every kind, %d records: run up to %d, a tap to %d, a second's run on to %d (+%d), a long one to %d, Down alone %d; the music alone: run to S%d, Down alone round to S%d, run down and up to S%d%s",
                  game.testShardsPage(), n, r0, r1, r2, r2 - r1, r3, r4, mLast, mFirst, mUp, anyMusic ? "" : " (no music held)"));
    }
}

int runDecoder(int argc, char** argv) {
    mkdir("shots/tests", 0755);
    const std::string what = argc > 2 ? argv[2] : "ages";
    if (what == "wav") { const int f = decoderWavs(); printf("decoder wav: %d failure%s\n", f, f == 1 ? "" : "s"); return f ? 1 : 0; }
    if (what == "counts") { decoderCounts(); return 0; }
    if (what == "pages") {
        const int f = decoderPages();
        int failures = 0; historyPageChecks(modeCheck(failures));
        printf("decoder pages: %d failure%s\n", f + failures, f + failures == 1 ? "" : "s"); return f + failures ? 1 : 0;
    }
    if (what == "check") { int failures = 0; mediaChecks(modeCheck(failures)); printf("decoder check: %d failure%s\n", failures, failures == 1 ? "" : "s"); return failures ? 1 : 0; }
    decoderCounts();
    const int f = decoderFrames();
    printf("decoder ages: %d failure%s\n", f, f == 1 ? "" : "s");
    return f ? 1 : 0;
}

int runRecords(int argc, char** argv) {
    mkdir("shots/tests", 0755);
    const int which = takePeopleArg(argc, argv);
    if (argc > 2 && std::string(argv[2]) == "ages") {
        for (int a = 0; a < AGE_COUNT; a++) { printf("== %s (the age test world)\n", AGE_NAMES[a]); printRecords(AGE_WORLDS[a]); }
        return 0;
    }
    if (argc > 2 && std::string(argv[2]) == "scenes") {
        int age = -1; if (argc > 3) for (int a = 0; a < AGE_COUNT; a++) if (std::string(argv[3]) == AGE_NAMES[a] || (a == AGE_FAR_FUTURE && std::string(argv[3]) == "far")) age = a;
        const int f = recordsScenes(age);
        printf("records scenes: %d failure%s\n", f, f == 1 ? "" : "s");
        return f ? 1 : 0;
    }
    if (argc > 2 && std::string(argv[2]) == "check") {   // the unit's T-04 checks alone
        int failures = 0;
        recordsChecks(modeCheck(failures));
        printf("records check: %d failure%s\n", failures, failures == 1 ? "" : "s");
        return failures ? 1 : 0;
    }
    if (argc > 4) { printRecords(AgeWorld{atoll(argv[2]), atoll(argv[3]), atoi(argv[4]), which}); return 0; }
    printf("records <sx> <sz> <body> [second] | records ages | records scenes [age] | records check\n");
    return 1;
}
