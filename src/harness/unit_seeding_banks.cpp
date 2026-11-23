// The unit's `seeding` group, the made colours' known issues closed after J-12 (KI-455, KI-459 to KI-461, KI-464, KI-465, KI-473 to KI-475):
// two lines' colours each where it lies (its waters', its overgrowth's) in every view; the ground's made colours and the hides dealt from
// every bank the world leaves free where a second people's forms stand; a dyed rain's bank; the globe's banks for a stain, the waters and
// the clouds on either disc; a flask past the ship's pool borrowing the nearest colour
#include "harness/harness.h"
#include "life/seeding.h"
#include "test_game.h"
#include <algorithm>
#include <cmath>

namespace {
bool sameRGB(const RGB& a, const RGB& b) { return std::fabs(a.r - b.r) < 1e-6f && std::fabs(a.g - b.g) < 1e-6f && std::fabs(a.b - b.b) < 1e-6f; }
}   // namespace

void unitSeedingBanks(const Check& check) {
    SeedRecordCase rc;
    if (!seedRecordCase(rc)) { check("seeding: a colour a line", false, "(no world where a poison line overgrows)"); return; }
    const StarSystem& S = rc.c.sys;
    const int bi = rc.c.body;
    const Body& b = S.bodies[bi];
    // two lines of the record case's overgrowing line, a red dye and a blue one, let go on opposite sides of the world
    Seeding a = seedingFor(S, bi, rc.line, rc.c.lat, rc.c.lon, 0);
    a.wild.kind.world.dye = WD_RED; a.wild.kind.world.dyeRate = 1;
    Seeding o = a;
    o.line += "/B"; o.lat = -a.lat; o.lon = a.lon + PI; o.clock = 1;
    o.wild.kind.world.dye = WD_BLUE;
    // (at each line's 16th step, each over its own side of the world: where both lie the later's colour is drawn, a tie its own)
    const double end = 1 + (16 * MADE_STEP_S + 1) / std::max(0.05, a.verdict.pace);
    MadeWorld m = madeWorldOf(S, bi, {a, o}, end);
    BodyGen g = BodyGen::make(b); g.made = m;
    {   // KI-460, KI-473: each line's own colour where its look or its overgrowth lies, on the planet function and the maps' ramps
        int water[2] = {0, 0}, grown[2] = {0, 0}, wrong = 0;
        for (int j = 0; j < 36; j++)
            for (int i = 0; i < 72; i++) {
                const SurfaceSample z = sampleSurface(g, StarSystem::bodyFromLatLon((j + 0.5) / 36 * PI - PI / 2, (i + 0.5) / 72 * TAU - PI), 40000);
                if (z.madeLine < 0 || z.madeLine > 1) continue;
                const int ln = z.madeLine;
                if (madeWetAt(z) && z.madeTint > 0.5) { water[ln]++; wrong += !sameRGB(madeColourAt(g, MAT_WATER, 1, ln), m.spread[ln].waterTint); }
                if (madeGrowthMaterial(z.material)) { grown[ln]++; wrong += !sameRGB(madeMatColour(g, z.material, ln), m.spread[ln].growth.colour); }
            }
        const bool two = !sameRGB(m.spread[0].waterTint, m.spread[1].waterTint) && !sameRGB(m.spread[0].growth.colour, m.spread[1].growth.colour);
        check("seeding: a colour a line", two && water[0] > 0 && water[1] > 0 && grown[0] > 0 && grown[1] > 0 && wrong == 0,
              fmt("(a red line and a blue one on %s: its waters %d and %d points in each line's own colour, its overgrowth %d and %d; %d in another's)", KINDS[b.type].name,
                  water[0], water[1], grown[0], grown[1], wrong));
    }
    // on the ground: land by the red line's place, the layer set, then a second people's settlement staged (its forms in the lent banks)
    TestGame G;
    G.savePrefix = "shots/tests/unit_seeding_banks_save"; G.settingsPath = "shots/tests/test_settings.txt"; G.keysPath = "shots/tests/test_keys.txt";
    G.guidePath = "shots/tests/unit_seeding_banks_guide.txt"; remove(G.guidePath.c_str());
    G.newGame(); G.settings.cabin = false; applyTestScale(G);
    Input in;
    m.rainTintK = 0.6; m.rainTint = dyeColour(WD_RED); m.air |= MA_DYE_RAIN; m.key = madeKey(m);   // (a dye lifted into its rain)
    G.testSetTime(30000); G.testParkAt(S.star, bi); G.testLandCursor(rc.c.lat / DEG, rc.c.lon / DEG);
    G.testMadeSet(bi, m);
    in.pressed[KEY_ENTER] = in.down[KEY_ENTER] = true; G.frame(in, 1.0 / 30); in.newFrame(); in.down[KEY_ENTER] = false;
    for (int k = 0; k < 30 * 40 && G.state != GameState::SURFACE; k++) runFramesAtLeastOne(G, in, 1.0 / 30);
    runFramesAtLeastOne(G, in, 0.3);
    const bool landed = G.state == GameState::SURFACE;
    SurfaceView& V = const_cast<SurfaceView&>(G.testSurface());
    const int own0 = V.testGrowthLineBank(0), own1 = V.testGrowthLineBank(1), tint0 = V.testTintLineBank(0), tint1 = V.testTintLineBank(1);
    {   // KI-464, KI-465, KI-474, KI-459: with a second people's forms staged (the lent banks theirs), the overgrowth, the waters' tint, the hides
        // and the dyed rain still dealt banks of their own, none shared
        RuinSpec spec; spec.people = 1; spec.size = 20;
        const std::vector<Form> none[3];
        V.testPlaceSettlement(V.player.x + 400, V.player.z + 400, spec, none);
        runFramesAtLeastOne(G, in, 0.1);
        const int gb = V.testGrowthBank(), tb = V.testTintBank(), h0 = V.testHideBank(0), h1 = V.testHideBank(1), rb = V.testRainBank();
        const int dealt[3] = {gb, tb, h0};
        bool apart = true;   // (the rain may share the waters', the two hides one where the pool runs short)
        for (int i = 0; i < 3; i++) for (int j = i + 1; j < 3; j++) apart = apart && dealt[i] != dealt[j];
        const bool hides = kindHas(b.type, KT_FAUNA) ? h0 != 3 && h0 != 0 && h1 != 0 && h1 != 3 : true;
        check("seeding: the made colours dealt from every free bank", landed && gb >= 0 && tb >= 0 && hides && rb >= 0 && apart && (h1 == h0 || (h1 != gb && h1 != tb)),
              fmt("(a second people's forms staged on %s: the overgrowth's bank %d, the waters' %d, the hides' %d and %d, the dyed rain's %d)", KINDS[b.type].name, gb, tb, h0, h1, rb));
        V.testClearPlaced();
    }
    remove(G.guidePath.c_str()); remove(G.slotPath(0).c_str());
    {   // KI-473 on the ground and KI-461, KI-475 on the globe: each line's banks of its own; either disc's stain, waters and clouds at once
        SpaceRenderer& R = G.testSpaceR();
        Framebuffer fb;
        int other = -1;
        for (int k = 0; k < (int)S.bodies.size() && other < 0; k++) if (k != bi) other = k;
        int discs = 0;
        std::string say;
        for (int which = 0; which < 2 && other >= 0; which++) {
            R.setupPalette(fb, &S, which == 0 ? bi : other, which == 0 ? other : bi, 1.0);
            const SpaceRenderer::DiscMade& dm = R.discMade[which];
            const bool ok = dm.stain[0] >= 0 && dm.stain[1] >= 0 && dm.stain[0] != dm.stain[1] && dm.water[0] >= 0 && dm.water[1] >= 0 && dm.water[0] != dm.water[1];
            discs += ok;
            say += fmt("%s disc %d: stains %d %d, waters %d %d, clouds %d", say.empty() ? "" : "; ", which + 1, dm.stain[0], dm.stain[1], dm.water[0], dm.water[1], dm.cloud);
        }
        check("seeding: each line's banks on the ground and the globe", landed && own0 >= 0 && own1 >= 0 && own0 != own1 && tint0 >= 0 && tint1 >= 0 && tint0 != tint1 && discs == 2,
              fmt("(the ground: the overgrowth's %d and %d, the waters' %d and %d; the globe, %s)", own0, own1, tint0, tint1, say.c_str()));
        // a line whose waters glow and whose plants carry its blight beside a glowing host: the blight's bank dark at night, the host's lit
        MadeWorld w = m;
        for (int i = 0; i < 2; i++) w.spread[i].growth = MadeGrowth();
        w.growths = 0; w.growthLine = -1;
        w.spread[0].glow = 1; w.spread[0].glowTint = RGB(0.3f, 1.0f, 0.6f);
        MadeLook& bl = w.spread[0].look[MC_PLANTS]; bl.on = bl.tinted = true; bl.kind = MP_BLIGHT; bl.tint = RGB(0.45f, 0.35f, 0.2f);
        MadeLook& gl = w.spread[1].look[MC_PLANTS]; gl.on = true; gl.tinted = false; gl.kind = MP_GLOWING;
        w.spread[1].glow = 1; w.spread[1].glowTint = RGB(0.4f, 0.6f, 1.0f);
        for (int i = 0; i < 2; i++) { w.spread[i].makes |= 1 << MC_PLANTS; w.spread[i].plants = true; }
        w.plant = gl; w.plantGlow = 1; w.plantGlowTint = w.spread[1].glowTint; w.flora |= MP_BLIGHT | MP_GLOWING; w.key = madeKey(w);
        G.testMadeSet(bi, w);
        R.setupPalette(fb, &S, bi, other, 1.0);
        const SpaceRenderer::DiscMade& dm = R.discMade[0];
        const int pb0 = dm.look[MC_PLANTS][0], pb1 = dm.look[MC_PLANTS][1];
        check("seeding: a line's glow in its own plants alone", pb0 >= 0 && pb1 >= 0 && pb0 != pb1 && !dm.lookGlow[MC_PLANTS][0] && dm.lookGlow[MC_PLANTS][1]
                  && w.spread[0].plantsGlow() == 0 && w.spread[1].plantsGlow() > 0,
              fmt("(a glowing line's blight in bank %d, %s at night; a glowing host's in bank %d, %s)", pb0, dm.lookGlow[MC_PLANTS][0] ? "LIT" : "dark", pb1,
                  dm.lookGlow[MC_PLANTS][1] ? "lit" : "DARK"));
    }
    {   // KI-455: past the ship's pool a flask borrows the dealt bank nearest its colour (within a fifth of the cube's diagonal), else dims
        std::vector<ShipBankAsk> asks;
        for (int k = 0; k < 16; k++) {   // eight colours, each asked twice a shade apart, the second farther
            const float h = (k % 8) / 8.0f;
            ShipBankAsk q; q.home = 32 + k; q.col = RGB(0.5f + 0.4f * std::cos(h * 6.28f), 0.5f + 0.4f * std::cos(h * 6.28f - 2.09f), 0.5f + 0.4f * std::cos(h * 6.28f + 2.09f)) * (k < 8 ? 1.0f : 0.95f);
            q.near = k; asks.push_back(q);
        }

        ShipBanks B;
        uint32_t kept = 0;
        for (int bk = 8; bk < 32; bk++) kept |= 1u << bk;   // (a pool of 8)
        B.deal(asks, kept);
        int borrowed = 0, right = 0;
        for (int k = 8; k < 16; k++) { const int nb = B.bankNear(32 + k); borrowed += nb >= 0; right += nb >= 0 && nb == B.bankOf(32 + k - 8); }
        check("ship: a flask past the pool borrows the nearest colour", B.dealt == 8 && B.dimmed == 8 && borrowed == 8 && right == 8 && B.borrowed == 8,
              fmt("(16 flasks asked of a pool of %d: %d dealt, %d past it, %d of them in the bank of the flask of their colour)", B.pool, B.dealt, B.dimmed, right));
    }
}
