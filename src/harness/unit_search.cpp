// Z-03: the unit's search (group `search`): a sample a pure function of its place, a sterile world's holding nothing alive, the lamp's
// ultraviolet absent to the bit when off, the kit's count (the vials, the refusals, the climb aboard, the save's lines)
#include "harness/harness.h"
#include "harness/test_game.h"
#include "harness/scenes_table.h"
#include "life/samples.h"

namespace {
// a read and what it holds, as one string (the place's microhabitat and every species' count)
std::string readText(SurfaceView& sv, double x, double z) {
    const SampleRead r = sv.readSample(x, z, false);
    if (!r.ok) return std::string("refused ") + (r.refusal ? r.refusal : "");
    const Sample S = sampleOf(sv.biota(), r.hab);
    std::string s = fmt("%s %d %.1f %d %d %d %016llx %d", r.name.c_str(), r.hab.niche, r.hab.meanC, r.hab.salt, (int)r.hab.shade, (int)r.hab.bloom,
                        (unsigned long long)r.hab.place, S.activity);
    for (const Held& h : S.held) s += fmt(" %d:%d:%.6g:%d", h.species, h.state, h.cells, (int)h.active);
    return s;
}
}   // namespace

void unitSearch(const Check& check) {
    {   // the same square metres read from two landings 300 m apart, on a living world's ground and an archean's mats: the same microhabitats and samples
        int places = 0, same = 0, worlds = 0;
        for (int type : {PT_FELISIAN, PT_ARCHEAN}) {
            StarSystem sys; int bi = -1;
            if (!firstBodyOfType(type, sys, bi)) continue;
            worlds++;
            SurfaceView a, b;
            a.init(&sys, bi, 0.3, 1.1, 1000.0);
            double la, lo; StarSystem::latLonFromBody(a.site.unitAt(300, 0), la, lo);
            b.init(&sys, bi, la, lo, 1000.0);
            for (int k = 0; k < 40; k++) {
                const double r = 90.0 * std::sqrt(k / 39.0), ang = k * 2.399963, x = r * std::sin(ang), z = r * std::cos(ang);
                double x2, z2; b.site.localAt(a.site.unitAt(x, z), x2, z2);
                places++;
                same += readText(a, x, z) == readText(b, x2, z2) && readText(a, x, z) == readText(a, x, z);
            }
        }
        check("search: a sample a pure function of its place (Z-03)", worlds == 2 && same == places, fmt("%d of %d places the same from two landings, on %d worlds", same, places, worlds));
    }
    {   // a furnace's and a sterile cold rock's ground: grains, nothing alive
        int places = 0, living = 0, worlds = 0;
        for (int type : {PT_MAGMA_OCEAN, PT_MOLTEN, PT_CRATERED}) {
            StarSystem sys; int bi = -1;
            if (!firstBodyOfType(type, sys, bi) || biotaOf(sys, bi).stage != LS_STERILE) continue;
            worlds++;
            SurfaceView sv; sv.init(&sys, bi, 0.2, 0.9, 1000.0);
            for (int k = 0; k < 30; k++) {
                const SampleRead r = sv.readSample(k * 7.0 - 100, (k % 5) * 11.0, false);
                if (!r.ok) continue;
                places++;
                living += sampleOf(sv.biota(), r.hab).living();
            }
        }
        check("search: a sterile world's samples hold nothing alive (Z-03)", worlds >= 2 && places > 30 && living == 0, fmt("%d worlds, %d samples, %d living held", worlds, places, living));
    }
    {   // an archean's mats at night: the frame with the ultraviolet off, on (the glow in bank 15), off again: the first and the last the same to the bit,
        // the pixels and the colours; a sterile world's the same on and off (nothing where nothing lives)
        auto frameOf = [](SurfaceView& sv, double t, StarNeighborhood& nb, SpaceRenderer& sr, std::vector<uint32_t>& rgb, std::vector<Pix>& idx) {
            Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0);
            idx = fb.idx;
            fb.mush(2); rgb.resize((size_t)FBW * FBH); fb.toRGB(rgb.data(), 1.0);
        };
        int glowing = 0; bool offSame = false, steady = false, sterileSame = false;
        {
            StarSystem sys; SurfaceView sv; StarNeighborhood nb; SpaceRenderer sr; double t = 0;
            if (setupSceneForType(PT_ARCHEAN, 8, -25 * DEG, 0.0, -0.45, sv, sys, nb, t, SCENE_MATS)) {
                std::vector<uint32_t> a, a2, b, c; std::vector<Pix> ia, ia2, ib, ic;
                sv.uvLamp = false; frameOf(sv, t, nb, sr, a, ia); frameOf(sv, t, nb, sr, a2, ia2);
                sv.uvLamp = true; frameOf(sv, t, nb, sr, b, ib);
                sv.uvLamp = false; frameOf(sv, t, nb, sr, c, ic);
                steady = a == a2 && ia == ia2;
                offSame = a == c && ia == ic;
                for (Pix p : ib) glowing += bankOf(p) == 15;
            }
        }
        {
            StarSystem sys; SurfaceView sv; StarNeighborhood nb; SpaceRenderer sr; double t = 0;
            if (setupSceneForType(PT_MAGMA_OCEAN, 15, -25 * DEG, 0.0, -0.3, sv, sys, nb, t)) {
                std::vector<uint32_t> a, b; std::vector<Pix> ia, ib;
                sv.uvLamp = false; frameOf(sv, t, nb, sr, a, ia);
                sv.uvLamp = true; frameOf(sv, t, nb, sr, b, ib);
                sterileSame = ia == ib && a == b;
            }
        }
        check("search: the lamp's ultraviolet absent to the bit when off (Z-03)", steady && offSame && glowing > 500 && sterileSame,
              fmt("the mats at night: %d pixels glowing with it on, %s off again%s; a magma ocean's ground the same with it on: %s", glowing, offSame ? "the same" : "DIFFERENT",
                  steady ? "" : " (the frame not steady)", sterileSame ? "yes" : "NO"));
    }
    {   // the kit on a living world: eight vials, the ninth refused, none from the buggy's seat (a buggy unfolded at the capsule), the cold store's
        // places counted with the vials in hand; the climb aboard: the vials to the cold store, its line; the save's lines loaded back
        TestGame game;
        game.savePrefix = "shots/tests/unit_search_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_search_guide.txt";
        game.keysPath = "shots/tests/test_keys.txt";
        remove(game.guidePath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input in;
        StarSystem fs; int fb = -1;
        int filled = 0, ninth = 0, buggy = 0, full = 0, stored = 0, loaded = 0;
        std::string first, line, store;
        if (firstBodyOfType(PT_FELISIAN, fs, fb)) {
            game.testParkAt(fs.star, fb);
            game.testLandSite();
            pressKey(game, in, KEY_ENTER); runFrames(game, in, 8.5);
            game.testSetPitch(-0.8); runFrames(game, in, 0.3);
            for (int k = 0; k < 8; k++) { pressKey(game, in, KEY_O); if (k == 0) first = game.testStatus(); runFrames(game, in, 0.1); }
            filled = (int)game.testKit().size();
            pressKey(game, in, KEY_O);
            ninth = game.testStatus().find("EIGHT VIALS ARE FULL") != std::string::npos && (int)game.testKit().size() == 8;
            const std::vector<Vial> eight = game.testKit();
            game.testKit().resize(2);
            game.testColdStore().assign(COLD_STORE - 2, eight[0]);
            pressKey(game, in, KEY_O);
            full = game.testStatus().find("COLD STORE ABOARD IS FULL") != std::string::npos && game.testKit().size() == 2;
            game.testColdStore().clear(); game.testKit() = eight; game.testKit().resize(3);
            pressKey(game, in, KEY_B); runFrames(game, in, 2.5); game.testWalkToBuggy(); pressKey(game, in, KEY_E); runFrames(game, in, 0.2);
            pressKey(game, in, KEY_O);
            buggy = game.testStatus().find("NOT FROM THE BUGGY") != std::string::npos && game.testKit().size() == 3;
            pressKey(game, in, KEY_E); runFrames(game, in, 0.3);
            pressKey(game, in, KEY_K); runFrames(game, in, 0.3);
            pressKey(game, in, KEY_Q); runFrames(game, in, 9.5);
            store = game.testStatus();
            stored = (int)game.testColdStore().size() == 3 && game.testKit().empty() && store == "3 SAMPLES TO THE COLD STORE";
            game.save("shots/tests/unit_search_vials.txt");
            const std::vector<Vial> was = game.testColdStore();
            game.load("shots/tests/unit_search_vials.txt");
            loaded = game.testColdStore().size() == was.size();
            for (size_t i = 0; loaded && i < was.size(); i++)
                loaded = was[i].name == game.testColdStore()[i].name && was[i].hab.place == game.testColdStore()[i].hab.place && was[i].hab.meanC == game.testColdStore()[i].hab.meanC &&
                         was[i].unit.x == game.testColdStore()[i].unit.x && was[i].activity == game.testColdStore()[i].activity;
        }
        check("search: the kit's count, its refusals, the cold store and its lines (Z-03)", filled == 8 && ninth && full && buggy && stored && loaded,
              fmt("%d vials filled (the first: %s), the ninth refused %d, a full cold store refused %d, from the buggy refused %d, the climb aboard %d ('%s'), loaded back %d",
                  filled, first.c_str(), ninth, full, buggy, stored, store.c_str(), loaded));
    }
}
