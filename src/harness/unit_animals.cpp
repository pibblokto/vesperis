// J-06 (R-448): the unit's `animals` group: the bestiary a pure function (serially, reversed and on the pool), under 1.5% of 1,000 species
// alike another of their world, every plan's poses within bounds (and its shapes made at each detail), the herds' habitat cells as generation
// 17 placed them (which 512 m cells round a landing hold a herd, and where its spot is), and an older save's notice. J-07: every plan's gaits
// within bounds and its feet put down; the record of what was seen, a name given with N, the guide's bytes, a loan's names. J-08: the fates'
// shares fixed by world and line, remains a pure function of their cell, the record's marks
#include "harness/harness.h"
#include "harness/test_game.h"
#include "surface/surface_view.h"
#include "galaxy/drainage.h"
#include "core/parallel.h"
#include "core/rng.h"
#include <fstream>
#include <sstream>
#include <cstring>

namespace {
// the cells with a herd round two landings on each of twelve worlds with plants and animals, and their spots, hashed (not the species or
// the herd's size: those are the bestiary's)
uint64_t herdCellsDigest(int& worlds, int& sites, int& cells) {
    DrainageOff off;   // the cells read the planet function at 16 m; the rivers would take a tile a landing
    worlds = sites = cells = 0;
    uint64_t h = 0x4E4DC311ULL;
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return worlds < 12; })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && worlds < 12; bi++) {
            const Body& b = sys.bodies[bi];
            if (!kindHas(b.type, KT_FAUNA) || !kindHas(b.type, KT_FLORA)) continue;
            worlds++;
            Rng r(mix64(b.seed ^ 0xCE11ULL));
            for (int k = 0; k < 2; k++) {
                const double lat = r.range(-60 * DEG, 60 * DEG), lon = r.range(-PI, PI);
                SurfaceSite site; site.init(&sys, bi, lat, lon, 5000.0);
                const Bestiary B = Bestiary::make(b, site.gen);
                sites++;
                int gLat0, gLon0;
                SurfaceView::lifeCellAt(site, 0, 0, gLat0, gLon0);
                for (int dl = -2; dl <= 2; dl++)
                    for (int dn = -3; dn <= 3; dn++) {
                        SurfaceView::Herd hd; int spi = -1, n = 0;
                        if (!SurfaceView::planCellHerd(site, B, gLat0 + dl, gLon0 + dn, hd, spi, n)) continue;
                        cells++;
                        h = hashCombine(h, b.seed);
                        h = hashCombine(h, (uint64_t)(int64_t)(gLat0 + dl) * 0x9E3779B97F4A7C15ULL ^ (uint64_t)(int64_t)(gLon0 + dn));
                        h = hashCombine(h, (uint64_t)std::llround(hd.cx * 100) * 31 ^ (uint64_t)std::llround(hd.cz * 100));
                    }
            }
        }
    }
    return h;
}

// a bestiary's every drawn value hashed (the words, the ways, the plans and sizes, the ecology, every part)
uint64_t bestiaryDigest(const Bestiary& B) {
    uint64_t h = 0xBE57ULL;
    auto d = [&](double v) { h = hashCombine(h, (uint64_t)std::llround(v * 1e6)); };
    auto i = [&](int64_t v) { h = hashCombine(h, (uint64_t)v); };
    for (int k = 0; k < (int)B.species.size(); k++) {
        const Species& s = B.species[k];
        for (char c : B.words(k)) i(c);
        i(s.way); i(s.social); i(s.prey);   // J-07
        i(s.plan); d(s.size); d(s.walkSpeed); d(s.runSpeed); i(s.gait); i(s.runGait); i(s.habitat); i(s.diet); i(s.activity); i(s.temperament); i(s.call);
        d(s.callPitch); i(s.herdMin); i(s.herdMax); i((int64_t)s.seed);
        const AnimalBody& a = s.body;
        i(a.variant); i(a.segs); i(a.girth); d(a.length); d(a.width); d(a.depth); d(a.pitch); i(a.pairs); i(a.joints); i(a.kneeBack); i(a.stance); i(a.foot); d(a.leg);
        d(a.legThick); d(a.arms); d(a.neck); d(a.neckThick); d(a.neckUp); i(a.arched); i(a.head); d(a.headLen); d(a.headW); d(a.headH); i(a.eyes); i(a.eyeLayout);
        i(a.ornament); d(a.ornamentLen); i(a.tail); d(a.tailLen); d(a.tailThick); i(a.dorsal); d(a.dorsalH); i(a.wings); d(a.span); i(a.hide); i(a.coat); i(a.bank);
        i(a.coatBank); d(a.tone); d(a.coatTone); i((int64_t)a.seed);
    }
    for (int k = 0; k < 2; k++) { d(B.style.colour[k].r); d(B.style.colour[k].g); d(B.style.colour[k].b); }
    return h;
}

// a sink that counts the shapes made and wants each vertex finite
struct CountSink final : ShapeSink {
    int polys = 0, lines = 0, points = 0; bool finite = true;
    void see(const RVert& v) { if (!(std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && std::isfinite(v.shade))) finite = false; }
    void polygon(const RVert* q, int n, const RasterParams&) override { polys++; for (int k = 0; k < n; k++) see(q[k]); }
    void line(const RVert& a, const RVert& b, int, bool, int) override { lines++; see(a); see(b); }
    void point(const RVert& a, int, bool, int) override { points++; see(a); }
};
// a view looking north from 6 m south of the animal, lit from above-east
struct TestView final : AnimalView {
    Vec3 view(const Vec3& p) const override { return Vec3(p.x - eye.x, p.y - eye.y, p.z - eye.z); }
    double light(const Vec3&, const Vec3& n) const override { return 52 * std::pow(0.15 + 0.85 * std::max(0.0, dot(n, normalize(Vec3(0.4, 0.8, 0.2)))), 0.65); }
};
std::string fileText(const std::string& path) { std::ifstream f(path, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
}

// J-08: a layer's animal fates hashed (the air's, each line's, the traits, the age), and each species' fate under it at a place
uint64_t fateDigest(const MadeWorld& m, const Bestiary& B, const Vec3& at, uint64_t seed) {
    uint64_t h = 0xFA7E5ULL;
    auto mix = [&](double v) { h = mix64(h ^ (uint64_t)std::llround(v * 1e6) ^ 0x9E3779B97F4A7C15ULL); };
    auto fate = [&](const MadeFate& f) { mix(f.fauna); for (double v : f.harm) mix(v); mix(f.shelled); mix(f.starve); mix(f.death); mix(f.sickness); mix(f.sick); mix(f.changed); mix(f.steer); mix(f.dye); mix(f.thrive); mix(f.age); };
    fate(m.fate);
    for (int i = 0; i < m.lines; i++) fate(m.spread[i].fate);
    mix(m.fauna); mix(m.fateAge); mix(m.rotFast);
    const MadeFate here = madeFateAt(m, at, seed);
    fate(here);
    int steer = m.fate.steer;
    for (int i = 0; i < m.lines; i++) steer |= m.spread[i].fate.steer;
    for (const Species& sp : B.species) {
        int ch[3] = {-1, -1, -1};
        const int nc = animalChangesOf(sp, steer, ch);
        const AnimalFate af = animalFateOf(sp, here, ch, nc);
        mix(af.dead); mix(af.sickness); mix(af.sick); mix(af.changed); mix(af.thrive); mix(af.age);
        for (int k = 0; k < af.nChanges; k++) mix(af.changes[k]);
    }
    return h;
}

void unitAnimals(const Check& check) {
    // the fauna worlds of the scan region in its order, their bodies made (the checks below share them)
    std::vector<std::pair<Body, BodyGen>> worlds;
    int species = 0;
    for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 200, [&] { return species < 1000; }))
        for (const Body& b : sys.bodies) {
            if (!kindHas(b.type, KT_FAUNA) || species >= 1000) continue;
            worlds.push_back({b, BodyGen::make(b)});
            species += (int)Bestiary::make(b, worlds.back().second).species.size();
        }
    {   // the bestiary a pure function: each world's the same serially, made again in reverse and on the pool
        const int n = (int)worlds.size();
        std::vector<uint64_t> serial(n), reversed(n), pooled(n);
        for (int k = 0; k < n; k++) serial[k] = bestiaryDigest(Bestiary::make(worlds[k].first, worlds[k].second));
        for (int k = n - 1; k >= 0; k--) reversed[k] = bestiaryDigest(Bestiary::make(worlds[k].first, worlds[k].second));
        parallelFor(n, 1, [&](int a, int b) { for (int k = a; k < b; k++) pooled[k] = bestiaryDigest(Bestiary::make(worlds[k].first, worlds[k].second)); });
        int same = 0;
        for (int k = 0; k < n; k++) same += serial[k] == reversed[k] && serial[k] == pooled[k];
        check("animals: the bestiary a pure function", same == n && n > 0, fmt("%d of %d worlds alike serially, reversed and on the pool (%d species)", same, n, species));
    }
    {   // no two of a world alike: the species with another of their world under ANIMAL_ALIKE, over the first thousand species; the plans all drawn;
        // a world's kin nearer each other than strangers
        int total = 0, alike = 0, plans[PLAN_COUNT] = {0};
        double within = 0, across = 0; long nw = 0, na = 0;
        std::vector<Bestiary> bs;
        for (const auto& w : worlds) bs.push_back(Bestiary::make(w.first, w.second));
        for (size_t k = 0; k < bs.size(); k++) {
            const Bestiary& B = bs[k];
            for (size_t i = 0; i < B.species.size() && total < 1000; i++) {
                total++; plans[B.species[i].plan]++;
                bool any = false;
                for (size_t j = 0; j < B.species.size(); j++) {
                    if (j == i) continue;
                    const double dd = animalDistance(B.species[i].body, B.species[j].body);
                    if (dd < ANIMAL_ALIKE) any = true;
                    if (j > i && B.species[i].plan == B.species[j].plan) { within += dd; nw++; }
                }
                alike += any;
                if (k + 1 < bs.size())   // against the next world's of the same plan
                    for (const Species& o : bs[k + 1].species) if (o.plan == B.species[i].plan) { across += animalDistance(B.species[i].body, o.body); na++; }
            }
        }
        int missing = 0; std::string none;
        for (int p = 0; p < PLAN_COUNT; p++) if (!plans[p]) { missing++; none += std::string(none.empty() ? "" : ", ") + PLAN_NAMES[p]; }
        const double share = 100.0 * alike / std::max(1, total);
        check("animals: no two of a world alike", total == 1000 && share < 1.5 && missing == 0,
              fmt("%d of %d species alike another of their world (%.2f%%, under 1.5%%); %d plans of %d drawn%s; a plan's two of a world %.2f apart, of two worlds %.2f", alike, total,
                  share, PLAN_COUNT - missing, PLAN_COUNT, missing ? (" (none: " + none + ")").c_str() : "", within / std::max(1L, nw), across / std::max(1L, na)));
    }
    {   // every plan's poses within bounds: each species posed idle, walking and running through its cycle, grazing, lying, on the level and on a
        // slope (aloft flapping and folded, afloat), every point near it and none under its ground; its shapes made at each detail, all finite
        int poses = 0, outside = 0, unfinite = 0, empty = 0, seen[PLAN_COUNT] = {0};
        std::string bad;
        for (const auto& w : worlds) {
            const Bestiary B = Bestiary::make(w.first, w.second);
            for (const Species& sp : B.species) {
                if (seen[sp.plan] >= 6) continue;   // six species a plan
                seen[sp.plan]++;
                const AnimalRig R = rigOf(sp.body);
                const AnimalBody& a = sp.body;
                const double reach = a.size + a.length + a.neck + a.tailLen + a.span + a.leg + a.headLen + a.ornamentLen;
                for (int slope = 0; slope < 2; slope++)
                    for (int q = 0; q < 24; q++) {
                        AnimalPoseIn in;
                        in.at = Vec3(0, 0, 0); in.heading = 0.3 * q; in.scale = 0.85 + 0.3 * (q % 3) / 2.0; in.t = 0.37 * q; in.seed = sp.seed;
                        in.gx = 0.5 * a.length + a.leg + a.neck; in.gz = a.width + a.leg;
                        for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) in.ground[i][j] = slope ? 0.2 * in.gx * (i - 1) + 0.1 * in.gz * (j - 1) : 0;
                        in.phase = q / 8.0; in.moving = q % 8 != 0; in.running = q >= 16; in.gait = in.running ? sp.runGait : sp.gait;
                        in.headPitch = q % 6 == 5 ? 1 : 0; in.lift = q % 8 == 7 ? 1 : 0;
                        in.aloft = planFlies(sp.plan) && (q % 4 != 3 || sp.plan == PLAN_FLOATER);   // a floater never comes down
                        in.folded = planFlies(sp.plan) && !in.aloft; in.flap = std::sin(q * 0.9);
                        in.afloat = planSwims(sp.plan);
                        AnimalPose P;
                        poseAnimal(R, in, P);
                        poses++;
                        const bool onGround = !in.aloft && !in.afloat;
                        double low = 0;
                        for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) low = std::min(low, in.ground[i][j]);
                        const double floorY = onGround ? low - 0.03 * a.size * in.scale - 0.02 : -1e9;   // under the ground's lowest corner
                        if (!poseWithin(P, in.at, 1.5 * reach * in.scale, floorY)) {
                            outside++;
                            if (bad.size() < 200) bad += fmt(" %s(q%d%s)", PLAN_NAMES[sp.plan], q, slope ? " slope" : "");
                        }
                        for (int det = 0; det < 3; det++) {
                            TestView V; V.eye = Vec3(0.5, 0.8 * a.size + 1, -6 - reach); V.pxPerM = 40.0 * (1 + det * 3); V.fbScale = 1;
                            CountSink sink;
                            emitAnimal(R, P, V, det, sink);
                            if (!sink.finite) unfinite++;
                            if (sink.polys + sink.lines + sink.points == 0) empty++;
                        }
                    }
            }
        }
        int plansPosed = 0; for (int p = 0; p < PLAN_COUNT; p++) plansPosed += seen[p] > 0;
        check("animals: every plan's poses within bounds", outside == 0 && unfinite == 0 && empty == 0 && plansPosed == PLAN_COUNT,
              fmt("%d poses of %d plans: %d outside%s; their shapes at three details: %d not finite, %d empty", poses, plansPosed, outside, bad.c_str(), unfinite, empty));
    }

    {   // J-07: every plan's gaits within bounds, its feet where they were put: each land species walking and running at its speeds through
        // its cycle (its gait and stride by `speciesGait` and `speciesStride`), stalking low, browsing, half under the ground and on a trunk,
        // every point near it and none under its ground (but a burrower's going under and a climber's on its trunk); a foot on the ground moves
        // under 3% of a stride while the body goes a fiftieth of a cycle
        int poses = 0, outside = 0, stance = 0, slips = 0, plans = 0, seen[PLAN_COUNT] = {0}, gaitsUsed = 0;
        bool gaitSeen[GAIT_COUNT] = {};
        double worst = 0;
        std::string bad;
        for (const auto& w : worlds) {
            const Bestiary B = Bestiary::make(w.first, w.second);
            for (const Species& sp : B.species) {
                if (seen[sp.plan] >= 4 || planFlies(sp.plan) || planSwims(sp.plan)) continue;
                if (!seen[sp.plan]++) plans++;
                const AnimalRig R = rigOf(sp.body);
                const AnimalBody& a = sp.body;
                const double reach = a.size + a.length + a.neck + a.tailLen + a.span + a.leg + a.headLen + a.ornamentLen;
                for (int run = 0; run < 2; run++) {
                    const double speed = run ? sp.runSpeed : sp.walkSpeed;
                    const int gait = speciesGait(sp, 1, speed, 9.81, run);
                    const double stride = speciesStride(sp, 1, speed, 9.81);
                    gaitSeen[gait] = true;
                    for (int q = 0; q < 24; q++) {
                        AnimalPoseIn in;
                        in.heading = 0; in.scale = 1; in.t = 0.37 * q; in.seed = sp.seed;
                        in.gx = 0.5 * a.length + a.leg + a.neck; in.gz = a.width + a.leg;
                        in.moving = true; in.running = run; in.gait = gait; in.stride = stride;
                        const int what = q % 6;   // stepping, stalking, browsing, half under, on a trunk, stepping again
                        in.crouch = what == 1 ? 1 : 0; in.reach = what == 2 ? 1 : 0; in.sink = what == 3 ? 0.5 : 0;
                        in.climbing = what == 4; in.wall = Vec3(1, 0, 0);
                        AnimalPose P[2];
                        for (int k = 0; k < 2; k++) {
                            in.phase = q / 24.0 + k * 0.02;
                            in.at = Vec3(0, 0, in.phase * stride);   // the body gone its share of its strides (heading 0 is +z)
                            poseAnimal(R, in, P[k]);
                            poses++;
                            const double floorY = in.sink > 0 || in.climbing ? -1e9 : -0.03 * a.size - 0.02;
                            if (!poseWithin(P[k], in.at, 1.5 * reach, floorY)) {
                                outside++;
                                if (bad.size() < 200) bad += fmt(" %s(%s q%d)", PLAN_NAMES[sp.plan], GAIT_NAMES[gait], q);
                            }
                        }
                        if (what != 0 && what != 5) continue;
                        for (int l = 0; l < P[0].legs; l++) {   // a foot on the ground in both stays where it was put
                            const Vec3 f0 = P[0].leg[l][P[0].joints - 1], f1 = P[1].leg[l][P[1].joints - 1];
                            const double onGround = 0.2 * P[0].legR + 1e-3;
                            if (f0.y > onGround || f1.y > onGround) continue;
                            stance++;
                            const double moved = std::hypot(f1.x - f0.x, f1.z - f0.z);
                            worst = std::max(worst, moved / stride);
                            if (moved > 0.03 * stride + 1e-3) slips++;
                        }
                    }
                }
            }
        }
        for (bool g : gaitSeen) gaitsUsed += g;
        check("animals: the gaits within bounds, the feet put down", outside == 0 && slips == 0 && plans == 12 && stance > 1000,
              fmt("%d poses of %d land plans in %d gaits: %d outside%s; %d feet on the ground over a fiftieth of a cycle, %d slipped (the most %.1f%% of a stride, "
                  "under 3%%)", poses, plans, gaitsUsed, outside, bad.c_str(), stance, slips, 100 * worst));
    }
    {   // J-07: the record: a landing walked up to a herd sees its species (the guide's `animal` lines), N with the crosshair on one names it; the
        // guide saved, loaded and saved again is the same bytes; a friend's export lent (a loan): its names in the inbox, the explorer's own
        // name winning
        const std::string pre = "shots/tests/unit_animals";
        TestGame game;
        game.savePrefix = pre + "_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = pre + "_guide.txt"; game.keysPath = "shots/tests/test_keys.txt";
        remove(game.guidePath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input in;
        StarSystem fs; int fb = -1;
        int records = 0, named = 0, same = 0, lent = 0, own = 0;
        std::string said, key;
        if (firstBodyOfType(PT_FELISIAN, fs, fb)) {
            game.testParkAt(fs.star, fb);
            game.testLandSite();
            pressKey(game, in, KEY_ENTER); runFrames(game, in, 8.5);
            if (game.testGotoHerd(16)) {
                runFrames(game, in, 1.0);
                records = (int)game.guide.animals.size();
                // the crosshair on the nearest: the explorer turned to it until the view's middle rests on it
                const SurfaceView& sv = game.testSurface();
                for (int k = 0; k < 40 && sv.lifeTarget.species < 0; k++) {
                    double best = 1e9, yaw = 0, pitch = 0;
                    for (const Critter& c : sv.critters) {
                        const double dx = c.x - sv.player.x, dz = c.z - sv.player.z, d = std::hypot(dx, dz);
                        if (d < best) { best = d; yaw = std::atan2(dx, dz); pitch = std::atan2(0.5 * c.size - sv.player.eyeHeight, d); }   // (the ground taken as level)
                    }
                    game.testSurfaceFace(yaw, pitch);
                    runFrames(game, in, 0.05);
                }
                if (sv.lifeTarget.species >= 0) {
                    key = Guide::bodyKey(fs.star.sx, fs.star.sy, fs.star.sz, fb) + "/A" + std::to_string(sv.lifeTarget.species);
                    pressKey(game, in, KEY_N); runFrames(game, in, 0.1);
                    game.testTypeText("TESTBEAST"); pressKey(game, in, KEY_ENTER); runFrames(game, in, 0.1);
                    said = game.testStatus();
                    named = game.guide.names.count(key) && game.guide.names[key] == "TESTBEAST";
                }
            }
            game.guide.save(pre + "_a.txt");
            Guide g2;
            g2.load(pre + "_a.txt");
            g2.save(pre + "_b.txt");
            same = fileText(pre + "_a.txt") == fileText(pre + "_b.txt") && g2.animals.size() == game.guide.animals.size() && !g2.animals.empty();
            Guide friendOf;   // the borrower: the record's names lent; then a name of its own, which wins
            lent = friendOf.importInbox(pre + "_a.txt") >= 0 && !key.empty() && friendOf.inbox.count(key) && friendOf.inbox[key] == "TESTBEAST";
            friendOf.names[key] = "MINE";
            friendOf.save(pre + "_c.txt");
            Guide again; again.load(pre + "_c.txt");
            own = again.names[key] == "MINE" && again.inbox[key] == "TESTBEAST";
            for (const char* f : {"_a.txt", "_b.txt", "_c.txt", "_guide.txt", "_save_auto.txt"}) remove((pre + f).c_str());
        }
        check("animals: the record and its names", records > 0 && named && same && lent && own,
              fmt("%d species seen walking up to a herd; N named one TESTBEAST (%s, '%s'); the guide saved again %s; a friend's lent the name %s, its own %s", records,
                  named ? "kept" : "NOT KEPT", said.c_str(), same ? "the same bytes" : "DIFFERENT", lent ? "in the inbox" : "NOT", own ? "wins" : "DOES NOT WIN"));
    }

    {   // the herds' cells as generation 17 had them: the bestiary drawn again moves no herd's cell or spot
        int worlds, sites, cells;
        const uint64_t d = herdCellsDigest(worlds, sites, cells);
        // (generation 17's bestiary: 108 cells round 24 landings on 12 worlds)
        check("animals: the herds' cells", d == 0x8a84caf071cf0447ULL && cells == 108,
              fmt("%d cells with a herd round %d landings on %d worlds, digest %016llx (17: 108, 8a84caf071cf0447)", cells, sites, worlds, (unsigned long long)d));
    }
    {   // a save and a guide of generation 17 load with the regeneration's notice (VESPERIS_PLAYER_DIR: copies of the player's own too, read only)
        const std::string pre = "shots/tests/animals_gen";
        std::string said;
        {
            TestGame g; g.savePrefix = pre + "_save"; g.guidePath = pre + "_guide.txt"; g.settingsPath = pre + "_settings.txt";
            g.newGame();
            g.save(g.slotPath(0));
            g.guide.save(g.guidePath);
        }
        for (const std::string& f : {pre + "_save_auto.txt", pre + "_guide.txt"}) {   // their generation set back to 17
            std::string t = fileText(f);
            const size_t at = t.find(fmt("\ngen %d\n", GEN_VERSION));
            if (at != std::string::npos) t.replace(at, 8, "\ngen 17\n");
            std::ofstream(f, std::ios::binary) << t;
        }
        {
            TestGame g; g.savePrefix = pre + "_save"; g.guidePath = pre + "_guide.txt"; g.settingsPath = pre + "_settings.txt";
            g.loadFromDisk();
            said = g.testStatusNext() + " / " + g.testStatus();
        }
        std::string player;
        if (const char* dir = std::getenv("VESPERIS_PLAYER_DIR")) {   // the player's, copied first
            const std::string cp = "shots/tests/animals_player";
            std::ofstream(cp + "_save_auto.txt", std::ios::binary) << fileText(std::string(dir) + "/vesperis_save_auto.txt");
            std::ofstream(cp + "_guide.txt", std::ios::binary) << fileText(std::string(dir) + "/guide.txt");
            TestGame g; g.savePrefix = cp + "_save"; g.guidePath = cp + "_guide.txt"; g.settingsPath = cp + "_settings.txt";
            g.loadFromDisk();
            player = fmt("; the player's copies said '%s'", (g.testStatusNext() + " / " + g.testStatus()).c_str());
        }
        for (const char* f : {"_save_auto.txt", "_guide.txt", "_settings.txt"}) remove((pre + f).c_str());
        check("animals: an older save's notice", said.find(fmt("WORLDS REGENERATED (GEN 17, NOW %d)", GEN_VERSION)) != std::string::npos,
              fmt("a save of generation 17 said '%s'%s", said.c_str(), player.c_str()));
    }
    // ---- J-08: the fates. Worlds with animals and plants and their own sea lines, four lines a world (a nerve poison, a blood poison, a sulphide
    // gas, a host of the animals' cells that passes its genes on), each let go where it takes and read at three clocks
    struct FateCase { const StarSystem* sys; int body; Seeding sd; };
    std::vector<SeedAirCase> fateWorlds;
    for (const SeedAirCase& c : seedAirCases(36))
        if (kindHas(c.sys.bodies[c.body].type, KT_FAUNA) && kindHas(c.sys.bodies[c.body].type, KT_FLORA)) fateWorlds.push_back(c);
    std::vector<FateCase> fcases;
    const std::vector<std::vector<std::pair<int, int>>> fateLines = {{{GN_POISON, PZ_NERVE}}, {{GN_POISON, PZ_BLOOD}}, {{GN_GAS, WG_H2S}}, {{GN_HOST, HO_ANIMALS}, {GN_CARRIER, -1}}};
    for (const SeedAirCase& c : fateWorlds)
        for (const auto& genes : fateLines) {
            SeedLine line = seedLineFor(c.sys, c.body, c.wild.kind.s.index, seedGeneEdits(c.wild.genes, genes));
            line.key += "/S1";
            double lat = c.lat, lon = c.lon;
            if (!line.valid || !seedPlaceTaking(c.sys, c.body, line, lat, lon)) continue;
            FateCase f{&c.sys, c.body, seedingFor(c.sys, c.body, line, lat, lon, 0)};
            if (f.sd.verdict.takes()) fcases.push_back(f);
        }
    {   // the shares fixed by world and line: every case's fates (the layer's and each species' at its place) the same serially, reversed and
        // on the pool, at steps 4, 16 and 40; and named where they are drawn
        const int n = (int)fcases.size();
        auto digestOf = [&](int k, int step) {
            const FateCase& f = fcases[k];
            const Body& b = f.sys->bodies[f.body];
            const MadeWorld m = madeWorldOf(*f.sys, f.body, {f.sd}, step * MADE_STEP_S / f.sd.verdict.pace + 1);
            return fateDigest(m, Bestiary::make(b, BodyGen::make(b)), StarSystem::bodyFromLatLon(f.sd.lat, f.sd.lon), b.seed);
        };
        int same = 0, fated = 0, dieOffs = 0, sick = 0, changed = 0;
        for (int step : {4, 16, 40}) {
            std::vector<uint64_t> serial(n), reversed(n), pooled(n);
            for (int k = 0; k < n; k++) serial[k] = digestOf(k, step);
            for (int k = n - 1; k >= 0; k--) reversed[k] = digestOf(k, step);
            parallelFor(n, 1, [&](int a, int b) { for (int k = a; k < b; k++) pooled[k] = digestOf(k, step); });
            for (int k = 0; k < n; k++) same += serial[k] == reversed[k] && serial[k] == pooled[k];
        }
        for (const FateCase& f : fcases) {
            const MadeWorld m = madeWorldOf(*f.sys, f.body, {f.sd}, 40 * MADE_STEP_S / f.sd.verdict.pace + 1);
            fated += m.fated(); dieOffs += (m.fauna & MF_DIE_OFF) != 0; sick += (m.fauna & MF_SICKNESSES) != 0; changed += (m.fauna & MF_CHANGED_YOUNG) != 0;
        }
        const MadeWorld none;   // (a world nobody seeded: no fate for any species)
        const Body& b0 = fateWorlds.empty() ? Body() : fateWorlds[0].sys.bodies[fateWorlds[0].body];
        bool unseeded = true;
        if (!fateWorlds.empty()) for (const Species& sp : Bestiary::make(b0, BodyGen::make(b0)).species) unseeded = unseeded && !animalFateOf(sp, madeFateAt(none, Vec3(1, 0, 0), b0.seed), nullptr, 0).on();
        check("animals: the fates' shares fixed by world and line", n > 0 && same == 3 * n && fated > n / 2 && dieOffs > 0 && sick > 0 && changed > 0 && unseeded,
              fmt("%d seedings on %zu worlds: %d of %d fates alike serially, reversed and on the pool (steps 4, 16, 40); at step 40 %d with a fate, %d a die-off, %d a "
                  "sickness, %d changed young; a world nobody seeded %s", n, fateWorlds.size(), same, 3 * n, fated, dieOffs, sick, changed, unseeded ? "untouched" : "TOUCHED"));
    }
    {   // remains a pure function of their cell: a blood poison's die-off at step 16 round its place, the cells' remains the same from two landings
        // 3 km apart (each in its own local metres, compared on the body) and in any order; a world nobody seeded has old bones alone
        int cells = 0, remainsN = 0, alike = 0, old = 0, oldOnly = 1;
        bool done = false;
        for (const FateCase& f : fcases) {
            if (done || f.sd.wild.kind.world.poison != PZ_BLOOD) continue;
            StarSystem sys = *f.sys;
            const MadeWorld m = madeWorldOf(sys, f.body, {f.sd}, 16 * MADE_STEP_S / f.sd.verdict.pace + 1);
            const Body& b = sys.bodies[f.body];
            const BodyGen g = BodyGen::make(b);
            double la = 0, lo = 0;   // the land nearest the place
            bool land = false;
            for (double km = 1; km < 400 && !land; km *= 1.3)
                for (int k = 0; k < 16 && !land; k++) {
                    la = f.sd.lat + km / b.radiusKm * std::cos(k * TAU / 16); lo = f.sd.lon + km / b.radiusKm * std::sin(k * TAU / 16) / std::max(0.05, std::cos(f.sd.lat));
                    const SurfaceSample smp = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 16);
                    land = !madeWetAt(smp) && smp.material != MAT_SNOW && smp.material != MAT_ICE;
                }
            if (!land) continue;
            auto remainsOf = [&](double lat, double lon, bool reversed, const MadeWorld& made, std::vector<std::pair<Vec3, Remains>>& out) {
                SurfaceView sv;
                sv.site.made = made;
                sv.init(&sys, f.body, lat, lon, 1000);
                double cla, clo;   // (the cells round the first landing, the same cells from the second)
                int gLat0, gLon0;
                SurfaceSite first; first.made = made; first.init(&sys, f.body, la, lo, 1000);
                SurfaceView::lifeCellAt(first, 0, 0, gLat0, gLon0);
                (void)cla; (void)clo;
                std::vector<std::pair<int, int>> order;
                for (int dl = -2; dl <= 2; dl++) for (int dn = -3; dn <= 3; dn++) order.push_back({gLat0 + dl, gLon0 + dn});
                if (reversed) std::reverse(order.begin(), order.end());
                for (const auto& [gl, gn] : order) {
                    SurfaceView::Herd hd; int spi = -1, nn = 0;
                    const bool herd = SurfaceView::planCellHerd(sv.site, sv.bestiary, gl, gn, hd, spi, nn);
                    std::vector<Remains> rs;
                    sv.cellRemains(gl, gn, herd ? &hd : nullptr, spi, nn, rs);
                    for (const Remains& r : rs) out.push_back({sv.site.unitAt(r.x, r.z), r});
                }
            };
            std::vector<std::pair<Vec3, Remains>> a, a2, c, w;
            remainsOf(la, lo, false, m, a);
            remainsOf(la, lo, true, m, a2);
            remainsOf(la + 3.0 / b.radiusKm, lo, false, m, c);
            remainsOf(la, lo, false, MadeWorld(), w);
            std::sort(a2.begin(), a2.end(), [](const auto& p, const auto& q) { return p.second.seed < q.second.seed; });
            std::sort(c.begin(), c.end(), [](const auto& p, const auto& q) { return p.second.seed < q.second.seed; });
            std::vector<std::pair<Vec3, Remains>> as = a;
            std::sort(as.begin(), as.end(), [](const auto& p, const auto& q) { return p.second.seed < q.second.seed; });
            cells = 35; remainsN = (int)a.size();
            if (as.size() == a2.size() && as.size() == c.size())
                for (size_t k = 0; k < as.size(); k++) {
                    const Remains &p = as[k].second, &q = a2[k].second, &r = c[k].second;
                    const bool sameCell = p.species == q.species && p.stage == q.stage && p.seed == q.seed && std::fabs(p.x - q.x) < 1e-9 && std::fabs(p.z - q.z) < 1e-9;
                    const bool sameBody = p.species == r.species && p.stage == r.stage && p.seed == r.seed && length(as[k].first - c[k].first) * b.radiusKm * 1000 < 0.01;
                    alike += sameCell && sameBody;
                }
            for (const auto& [u, r] : w) { old++; if (!r.old || (r.stage != RS_BONES && r.stage != RS_SCATTERED)) oldOnly = 0; }
            done = true;
        }
        check("animals: remains a pure function of their cell", done && remainsN > 0 && alike == remainsN && oldOnly,
              fmt("%d cells of a blood poison's die-off at step 16: %d remains, %d alike in another order and from a landing 3 km off (on the body within a "
                  "centimetre); a world nobody seeded %d old bones%s", cells, remainsN, alike, old, oldOnly ? ", all of them old" : ", NOT ALL OLD"));
    }
    {   // the record's marks: remains in view mark their species DYING (GONE where none of it lives there), a changed young CHANGED; the guide's
        // `animal` lines keep them and read back the same bytes, a line with no marks as J-07 wrote it; a loan carries no animal lines
        std::string marksSeen;
        bool dying = false, changedSeen = false, bytes = false, plain = false, loan = false;
        for (const FateCase& f : fcases) {
            if (f.sd.wild.kind.world.poison != PZ_BLOOD) continue;
            StarSystem sys = *f.sys;
            const MadeWorld m = madeWorldOf(sys, f.body, {f.sd}, 40 * MADE_STEP_S / f.sd.verdict.pace + 1);
            SurfaceView sv; SpaceRenderer sr; StarNeighborhood nb; Input in;
            sv.site.made = m;
            sv.init(&sys, f.body, f.sd.lat + 0.02, f.sd.lon, 1000);
            if (!sv.bestiary.landCount) continue;
            nb.update(sv.site.worldPos(1000, 0, 0, 0));
            sv.critters.clear(); sv.flocks.clear(); sv.herds.clear(); sv.remains.clear(); sv.habitatT = 1e18;
            const int spi = 0;
            sv.freeCam = true; sv.freePos = Vec3(0, sv.site.groundHeight(0, 0) + 1.6, 0); sv.freeYaw = 0; sv.freePitch = -0.25;
            Remains r; r.species = spi; r.x = 0.4; r.z = 5; r.heading = 1.2; r.size = sv.bestiary.species[spi].size; r.stage = RS_FRESH; r.seed = 99; r.cell = false;
            sv.testRemainsAdd(r);
            // a changed young beside it (its species' changes, or one given)
            SurfaceView::Herd hd; hd.cx = -1; hd.cz = 6; hd.origin = true; sv.herds.push_back(hd);
            Critter c; c.herd = 0; c.species = spi; c.x = -1.2; c.z = 6; c.heading = 1.0; c.timer = 30; c.size = sv.bestiary.species[spi].size; c.seed = 7; c.changed = true;
            c.homeX = c.x; c.homeZ = c.z;
            if (sv.speciesChanges.size() > (size_t)spi && sv.speciesChanges[spi].n == 0) { sv.speciesChanges[spi].n = 1; sv.speciesChanges[spi].changes[0] = CHG_PALE; }
            sv.critters.push_back(c);
            sv.player.x = 0; sv.player.z = -60; sv.player.y = sv.site.surfaceHeight(0, -60);
            for (int k = 0; k < 3; k++) { sv.update(1.0 / 30, in, 1000 + k / 30.0, false); Framebuffer fb; sv.render(fb, 1000 + k / 30.0, nb.stars, sr, 1.0); }
            const uint32_t mk = sv.lifeSeen.size() > (size_t)spi ? sv.lifeSeen[spi].marks : 0;
            dying = (mk & (AM_DYING | AM_GONE)) != 0 && (sv.lifeSeen[spi].doing >> DO_DEAD & 1);
            changedSeen = (mk & AM_CHANGED) != 0;
            marksSeen = fmt("%s%s%s", (mk & AM_DYING) ? "DYING " : "", (mk & AM_GONE) ? "GONE " : "", (mk & AM_CHANGED) ? "CHANGED" : "");
            break;
        }
        {   // the guide's bytes with marks, and a record without
            const std::string pre = "shots/tests/unit_fates";
            Guide g;
            const std::string k0 = Guide::bodyKey(150, 0, 36, 1) + "/A0", k1 = Guide::bodyKey(150, 0, 36, 1) + "/A1";
            g.animals[k0] = AnimalRecord{(1u << DO_DEAD) | 1u, 12, 3, AM_DYING | AM_CHANGED};
            g.animals[k1] = AnimalRecord{2u, 30, 5, 0};
            g.save(pre + "_a.txt");
            Guide g2; g2.load(pre + "_a.txt"); g2.save(pre + "_b.txt");
            const std::string ta = fileText(pre + "_a.txt");
            bytes = ta == fileText(pre + "_b.txt") && g2.animals[k0].marks == (AM_DYING | AM_CHANGED);
            plain = ta.find("animal " + k1 + " 2 30 5\n") != std::string::npos && ta.find(fmt("animal %s %u 12 3 %d\n", k0.c_str(), (1u << DO_DEAD) | 1u, AM_DYING | AM_CHANGED)) != std::string::npos;
            Guide lent;
            loan = lent.importInbox(pre + "_a.txt") >= 0 && lent.animals.empty();
            for (const char* fn : {"_a.txt", "_b.txt"}) remove((pre + fn).c_str());
        }
        check("animals: the record's marks", dying && changedSeen && bytes && plain && loan,
              fmt("remains and a changed young in view marked their species %s; the guide's marks read back %s, a record without them as J-07 wrote it %s; a "
                  "loan's animal lines %s", marksSeen.empty() ? "NOTHING" : marksSeen.c_str(), bytes ? "the same bytes" : "DIFFERENT", plain ? "yes" : "NO",
                  loan ? "none" : "SOME"));
    }
}
