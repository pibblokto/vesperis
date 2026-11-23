// Z-01: the unit's living small (group `life`): the table against the kinds, the biota the same however asked, the rules it keeps
#include "harness/harness.h"
#include "life/biota.h"
#include "life/morphs.h"
#include "life/morph_draw.h"
#include "galaxy/drifters.h"

namespace {
uint64_t microbeHash(uint64_t h, const Microbe& s) {
    auto mixI = [&](int64_t v) { h = mix64(h ^ (uint64_t)v); };
    auto mixD = [&](double v) { int64_t b; std::memcpy(&b, &v, sizeof b); mixI(b); };
    mixI(s.index); mixI(s.state); mixI(s.plan); mixI(s.niche); mixI(s.niches); mixI(s.way); mixI(s.substrate); mixI(s.pigment); mixI(s.oxygen);
    mixD(s.tMin); mixD(s.tOpt); mixD(s.tMax); mixI(s.saltLo); mixI(s.saltHi); mixD(s.phLo); mixD(s.phHi); mixD(s.doseGy); mixD(s.sporeYears);
    mixI(s.cycle); mixI(s.division); mixI(s.motion); mixI(s.taxis); mixI(s.glows); mixD(s.abundance); mixI(s.clade); mixI((int64_t)s.genesis);
    mixI((int64_t)s.seed); mixD(s.ageYears); mixI(s.fromBody); mixI(s.fromIndex);
    return h;
}
uint64_t biotaHash(uint64_t h, const Biota& B) {
    h = hashCombine(h, (uint64_t)B.stage * 31 + (uint64_t)B.second * 7 + (uint64_t)B.chemistry);
    h = hashCombine(h, B.genesis.id ^ ((uint64_t)B.habitats << 40));
    int64_t d; std::memcpy(&d, &B.dormantYears, sizeof d); h = hashCombine(h, (uint64_t)d);
    for (const Microbe& s : B.species) h = microbeHash(h, s);
    return h;
}
uint64_t morphHash(const Morph& m) {
    uint64_t h = 0x3F0A;
    auto mixI = [&](int64_t v) { h = mix64(h ^ (uint64_t)v); };
    auto mixD = [&](double v) { int64_t b; std::memcpy(&b, &v, sizeof b); mixI(b); };
    mixI((int64_t)m.seed); mixI(m.plan); mixI(m.state); mixI(m.chemistry); mixI(m.way); mixI(m.variant); mixI(m.sym);
    for (double v : {m.n1, m.n2, m.n3, m.aspect, m.bend, m.amp, m.turns, m.wander, m.taper, m.lengthUm, m.widthUm, m.wall, m.spineLen, m.capsule, m.sheath, m.flagLen,
                     m.beatHz, m.waveLen, m.cilia, m.ciliaLen, m.ciliaWave, m.stalk, m.nucleus, m.vacPulse, m.granules, m.gas, m.density, m.speed, m.runS, m.sporeSize,
                     m.nucleoid, m.wave, m.waves}) mixD(v);
    for (int v : {m.arrangement, m.groupN, m.mark, m.marks, m.spines, m.spineBranch, m.flagella, m.flagPlace, m.flagKind, m.ciliaRows, m.pili, m.pods, m.podCount,
                  m.vacuoles, m.plastids, m.plastidShape, m.magnetosomes, m.droplets, m.pigment, m.motion, m.coat, m.sporeAt, m.division, m.mineral, m.styleSym,
                  m.cortex, m.polar, m.clay}) mixI(v);
    mixI(m.holdfast); mixI(m.eyespot); mixI(m.glows); mixI(m.mirrored); mixI(m.taxis); mixI(m.cycle); mixI(m.style);
    return h;
}
}   // namespace

// Z-01: where life is. The table held against KINDS; the biota of the scan region's first 300 systems the same twice and drawn by the pool's
// workers; over 1,500 systems and the forty drifters nearest home: the furnaces sterile, no giant with a biota, nothing fed by light in the dark
// or under ice, a world whose life shows on its ground alive, a living world's species 3-16 and a dead world's spores 1-4, every landable kind
// but the furnaces with something; home's genesis pinned and the ship's species of it; every carried spore its source's species, of its world's
// genesis, from a living neighbour; a system's living worlds of one chemistry of one genesis; a drifter's life the star's it left.
// Z-02: what it looks like. The forms the same twice and none of a world alike; a slide's frame the same however reached and at one thread;
// every plan's forms in every act within their bounds at every zoom; a way of seeing's banks its own, its ramps meeting the field's
void unitLife(const Check& check) {
    {
        int bad = 0, furnaces = 0, landable = 0;
        std::string why;
        for (int t = 0; t < PT_COUNT; t++) {
            const LifeRow& r = BIOTA[t];
            const KindDef& K = KINDS[t];
            double life = 0; for (int s = LS_PREBIOTIC; s < LS_FOSSIL; s++) life += r.w[s];
            bool ok = K.landable ? r.habitats != 0 && (r.furnace || life > 0 || r.keepsSpores) : r.habitats == 0 && life == 0 && !r.keepsSpores;
            if (K.tags & KT_FLORA) ok = ok && r.w[LS_COLONIAL] > 0 && life == r.w[LS_COLONIAL];   // a world with flora is colonial
            if (K.tags & KT_MICROBIAL) ok = ok && r.w[LS_SIMPLE] > 0;
            if (deadKind(t)) ok = ok && r.w[LS_DORMANT] > 0 && r.fossils >= 1;
            if (r.furnace) furnaces++;
            if (K.landable) landable++;
            if (!ok) { bad++; why += std::string(why.empty() ? "" : ", ") + K.name; }
        }
        bool furnaceSet = true;
        for (int t : {PT_MOLTEN, PT_VOLCANIC, PT_TECTONIC, PT_MAGMA_OCEAN, PT_CHTHONIAN, PT_GLASS, PT_STEAM}) furnaceSet = furnaceSet && BIOTA[t].furnace;
        check("life: a row a kind, every landable kind's habitats and stages (Z-01)", bad == 0 && furnaceSet && furnaces == 7,
              fmt("%d landable kinds, %d furnaces%s", landable, furnaces, bad ? fmt("; wrong: %s", why.c_str()).c_str() : ""));
    }
    // the scan region's first 300 systems, twice in order and once by the workers (the cache holds 16: each pass draws them again)
    {
        std::vector<StarSystem> systems;
        for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160, [&] { return systems.size() < 300; })) systems.push_back(sys);
        auto pass = [&](std::vector<uint64_t>& out) { out.assign(systems.size(), 0); for (size_t i = 0; i < systems.size(); i++) { auto L = lifeOf(systems[i]);
                                                         uint64_t h = 0xB107A; for (const Biota& B : L->bodies) h = biotaHash(h, B); out[i] = h; } };
        std::vector<uint64_t> a, b, c(systems.size(), 0);
        pass(a); pass(b);
        parallelFor((int)systems.size(), 1, [&](int i0, int i1) {
            for (int i = i0; i < i1; i++) { auto L = lifeOf(systems[i]); uint64_t h = 0xB107A; for (const Biota& B : L->bodies) h = biotaHash(h, B); c[i] = h; }
        });
        int differ = 0;
        for (size_t i = 0; i < systems.size(); i++) differ += a[i] != b[i] || a[i] != c[i];
        check("life: the same twice and by the workers (Z-01)", differ == 0 && systems.size() == 300, fmt("%zu systems, %d differ", systems.size(), differ));
    }
    {
        std::vector<StarSystem> systems;
        for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160, [&] { return systems.size() < 1500; })) systems.push_back(sys);
        Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
        std::vector<Star> near; driftersNear(home.pos, 3000, near);
        int drifters = 0;
        for (const Star& d0 : near) {
            if (drifters >= 40) break;
            Star d; if (!drifterInSector(d0.sx, d0.sy, d0.sz, d, true)) continue;
            systems.emplace_back(); systems.back().generate(d); drifters++;
        }
        int furnaceLife = 0, giantLife = 0, darkLight = 0, iceLight = 0, livingOut = 0, deadOut = 0, living = 0, dead = 0, minL = 99, maxL = 0;
        int carried = 0, carriedWrong = 0, shared = 0, sharedWrong = 0, drifterLife = 0, drifterWrong = 0, shows = 0, showsBare = 0;
        std::vector<int> some(PT_COUNT, 0), seen(PT_COUNT, 0);
        for (const StarSystem& sys : systems) {
            auto L = lifeOf(sys);
            DrifterPast past; if (sys.sunless()) past = drifterPast(sys.star);
            std::map<int, uint64_t> byChem;
            for (int i = 0; i < (int)sys.bodies.size(); i++) {
                const Body& b = sys.bodies[i];
                const Biota& B = L->bodies[i];
                const LifeRow& r = BIOTA[b.type];
                if (!KINDS[b.type].landable) { if (B.habitats || !B.species.empty() || B.stage != LS_STERILE || B.genesis.id) giantLife++; continue; }
                seen[b.type]++;
                if (B.stage != LS_STERILE) some[b.type]++;
                if (r.furnace && (B.stage != LS_STERILE || !B.species.empty())) furnaceLife++;
                if (kindHas(b.type, KT_FLORA | KT_FAUNA | KT_MICROBIAL | KT_VENT_LIFE) && !sys.sunless()) { shows++; if (!livingStage(B.stage)) showsBare++; }
                for (const Microbe& s : B.species) {
                    if (s.way == LW_LIGHT && sys.sunless()) darkLight++;
                    if (s.way == LW_LIGHT && r.underIce && s.fromBody < 0) iceLight++;
                    if (s.fromBody < 0) continue;
                    carried++;
                    const Biota& A = L->bodies[s.fromBody];
                    const bool same = s.fromIndex >= 0 && s.fromIndex < (int)A.species.size() && A.species[s.fromIndex].seed == s.seed && A.species[s.fromIndex].plan == s.plan;
                    if (!livingStage(A.stage) || A.genesis.id != B.genesis.id || A.genesis.id != s.genesis || !same || s.state != SS_DORMANT) carriedWrong++;
                }
                if (livingStage(B.stage)) {
                    living++;
                    const int n = B.count(SS_LIVING);
                    minL = std::min(minL, n); maxL = std::max(maxL, n);
                    if (n < 3 || n > 16) livingOut++;
                    if (!b.wanderer) {
                        auto it = byChem.find(B.chemistry);
                        if (it == byChem.end()) byChem[B.chemistry] = B.genesis.id;
                        else { shared++; if (it->second != B.genesis.id) sharedWrong++; }
                    }
                }
                bool carriedHere = false; for (const Microbe& s : B.species) carriedHere = carriedHere || s.fromBody >= 0;
                if (B.stage == LS_DORMANT && !carriedHere) { dead++; const int n = B.count(SS_DORMANT); if (n < 1 || n > 4) deadOut++; }
                if (sys.sunless() && B.genesis.id && !carriedHere) {
                    drifterLife++;
                    if (past.found && B.genesis.id != systemGenesis(past.star, B.genesis.chemistry).id) drifterWrong++;
                }
            }
        }
        std::string bare;
        for (int t = 0; t < PT_COUNT; t++) if (KINDS[t].landable && !BIOTA[t].furnace && (seen[t] == 0 || some[t] == 0)) bare += std::string(bare.empty() ? "" : ", ") + KINDS[t].name;
        check("life: the furnaces sterile, no giant with a biota (Z-01)", furnaceLife == 0 && giantLife == 0,
              fmt("%zu systems (%d drifters): %d furnace worlds with life, %d giants or stars", systems.size(), drifters, furnaceLife, giantLife));
        check("life: nothing fed by light in the dark or under ice (Z-01)", darkLight == 0 && iceLight == 0, fmt("%d in the dark, %d under ice", darkLight, iceLight));
        check("life: a world whose life shows on its ground lives (Z-01)", shows > 500 && showsBare == 0,
              fmt("%d worlds with flora, animals, mats or vents: %d without life", shows, showsBare));
        check("life: something on every landable kind but the furnaces (Z-01)", bare.empty(), bare.empty() ? "every one" : "none on " + bare);
        check("life: a living world's species 3-16, a dead world's spores 1-4 (Z-01)", livingOut == 0 && deadOut == 0 && living > 1000 && dead > 100,
              fmt("%d living worlds (%d-%d), %d out; %d dead or dormant, %d out", living, minL, maxL, livingOut, dead, deadOut));
        check("life: a carried spore its living neighbour's, of one genesis (Z-01)", carried > 1000 && carriedWrong == 0, fmt("%d carried, %d wrong", carried, carriedWrong));
        check("life: a system's living worlds of one chemistry, one genesis (Z-01)", shared > 100 && sharedWrong == 0, fmt("%d pairs, %d apart", shared, sharedWrong));
        check("life: a drifter's life the star's it left (Z-01)", drifterLife > 0 && drifterWrong == 0, fmt("%d bodies of the dark with life, %d not their old star's", drifterLife, drifterWrong));
    }
    {
        const Genesis g = homeGenesis();
        Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
        StarSystem hs; hs.generate(home);
        int homeWorld = -1;
        for (int i = 0; i < (int)hs.bodies.size(); i++) if (hs.bodies[i].parent < 0 && hs.bodies[i].type == PT_FELISIAN && homeWorld < 0) homeWorld = i;
        const Biota B = homeWorld >= 0 ? biotaOf(hs, homeWorld) : Biota{};
        bool ship = true;
        for (int k = 0; k < SHIP_SPECIES; k++) ship = ship && shipSpecies(k).genesis == g.id && shipSpecies(k).state == SS_LIVING;
        check("life: home's genesis pinned, the ship's species of it (Z-01)",
              g.id == HOME_GENESIS_ID && g.origin == GO_CELL && g.cellIndex == 0 && !g.mirrored && g.letters == 4 && B.genesis.id == g.id && livingStage(B.stage) && ship,
              fmt("%016llx (pinned %016llx), %s #%d, %s, %d letters; %s %s", (unsigned long long)g.id, (unsigned long long)HOME_GENESIS_ID, GENESIS_ORIGIN_NAMES[g.origin],
                  g.cellIndex, g.mirrored ? "mirrored" : "ours", g.letters, homeWorld >= 0 ? hs.bodies[homeWorld].name.c_str() : "no home world",
                  LIFE_STAGE_NAMES[B.stage]));
    }

    // Z-02: what it looks like. The forms the same twice; no two of a world's species alike over the sixty worlds nearest home, nor the
    // ship's three
    {
        int worlds = 0, pairs = 0, alike = 0, differ = 0;
        double nearest = 1e9;
        for (const Star& st : starsNearHome(14)) {
            if (worlds >= 60) break;
            StarSystem sys; sys.generate(st);
            auto L = lifeOf(sys);
            for (int b = 0; b < (int)sys.bodies.size() && worlds < 60; b++) {
                if (L->bodies[b].species.size() < 2) continue;
                const std::vector<Morph> a = morphsOf(sys, b), c = morphsOf(sys, b);
                for (size_t i = 0; i < a.size(); i++) {
                    if (morphHash(a[i]) != morphHash(c[i])) differ++;
                    for (size_t j = i + 1; j < a.size(); j++) { const double d = morphDistance(a[i], a[j]); pairs++; alike += d < MORPH_ALIKE; nearest = std::min(nearest, d); }
                }
                worlds++;
            }
        }
        int shipAlike = 0;
        for (int i = 0; i < SHIP_SPECIES; i++) for (int j = i + 1; j < SHIP_SPECIES; j++) shipAlike += morphDistance(shipMorph(i), shipMorph(j)) < MORPH_ALIKE;
        check("life: forms the same twice, none of a world alike (Z-02)", worlds == 60 && differ == 0 && alike == 0 && shipAlike == 0,
              fmt("%d worlds, %d pairs: %d alike (under %.1f), the nearest %.2f; %d drawn twice apart; the ship's %d alike", worlds, pairs, alike, MORPH_ALIKE, nearest,
                  differ, shipAlike));
    }
    // a slide's frame at a time the same drawn at once, reached through the frames before it, and at one thread: every plan's forms doing
    // their acts in the six ways of seeing
    const std::vector<Morph> forms = everyForm();
    {
        std::vector<Specimen> cells;
        Microbe preyS; preyS.plan = MP_ROD; preyS.seed = 0x9AE70001ULL;
        static const Morph prey = morphOf(preyS, CH_WATER, homeGenesis());
        for (size_t i = 0; i < forms.size(); i += 3) {
            Specimen s; s.m = &forms[i]; s.seed = 11 + i; s.x = ((int)(i % 9) - 4) * 40.0; s.y = ((int)(i / 9) % 5 - 2) * 40.0; s.angle = i * 0.7; s.other = &prey;
            for (int a = 1; a < AN_COUNT; a++) if (actSuits(forms[i], (a + (int)i) % AN_COUNT) && (a + (int)i) % AN_COUNT != AN_FRONT) { s.act = (a + (int)i) % AN_COUNT; break; }
            s.t0 = 2.5 * (i % 7);
            cells.push_back(s);
        }
        int same = 0, ways = 0; std::string off;
        for (int way = 0; way < WS_COUNT; way++) {
            Slide v; v.seeing = way; v.umPerPx = umPerPxAt(100); v.seed = 5;
            Framebuffer a, b, c;
            slidePalette(a.pal, way); slidePalette(b.pal, way); slidePalette(c.pal, way);
            v.t = 37.5; drawSlide(a, v, cells); a.mush(2);
            for (double t = 0; t <= 37.5; t += 7.5) { v.t = t; drawSlide(b, v, cells); }
            b.mush(2);
            parallelThreadsOverride() = 1;
            v.t = 37.5; drawSlide(c, v, cells); c.mush(2);
            parallelThreadsOverride() = 0;
            const bool ok = a.idx == b.idx && a.idx == c.idx;
            same += ok; ways++;
            if (!ok) off += std::string(off.empty() ? "" : ", ") + SEEING_NAMES[way];
        }
        check("life: a slide's frame however reached (Z-02)", same == ways, fmt("%zu specimens in %d ways of seeing: %d the same skipped to and at one thread%s", cells.size(), ways, same,
              off.empty() ? "" : ("; apart: " + off).c_str()));
    }
    // every plan's forms within their bounds at every zoom: each pixel a specimen changes is within its extent of its place, at the
    // magnification that suits it, at half and at twice it, in a dark field and in phase contrast, living and in an act, early and late
    {
        int specimens = 0, frames = 0, outside = 0;
        double worst = 0; std::string worstText;
        std::map<std::string, double> byForm;   // each form's worst, listed when it fails
        std::map<std::pair<long, int>, std::vector<Pix>> empty;
        Microbe preyS; preyS.plan = MP_SPHERE; preyS.seed = 0x9AE70002ULL;
        const Morph prey = morphOf(preyS, CH_WATER, homeGenesis());
        for (const Morph& m : forms) {
            int first = -1;
            for (int act = 0; act < AN_COUNT; act++) {
                if (!actSuits(m, act)) continue;
                if (act > 0 && first < 0) first = act;
                const double T = actSeconds(m, act);
                specimens++;
                for (double f : {0.45, 0.95}) {
                    const double t = act == AN_LIVE ? 1.3 + 2 * f : T * f;
                    Specimen s; s.m = &m; s.act = act; s.form = act == AN_WAKE ? CF_SPORE : CF_CELL; s.seed = m.seed ^ 0x5A; s.angle = 0.3; s.t0 = 0; s.other = &prey;
                    const double e = specimenExtentUm(s, t);
                    for (double zoom : {0.5, 1.0, 2.0}) {
                        if (zoom != 1.0 && act != AN_LIVE && act != first) continue;   // every zoom for its life and its first act; its other acts at one
                        const double upp = std::max(1e-3, e * 2.4 / UH) / zoom;   // the specimen's extent fills the view's height at x1
                        for (int way : {WS_DARK, WS_PHASE}) {
                            Slide v; v.seeing = way; v.umPerPx = upp; v.t = t; v.seed = 9;
                            Framebuffer fb;
                            slidePalette(fb.pal, way);
                            const std::pair<long, int> key{(long)std::llround(upp * 1e6), way};
                            if (!empty.count(key)) { drawSlide(fb, v, {}); empty[key] = fb.idx; }
                            drawSlide(fb, v, {s});
                            frames++;
                            const std::vector<Pix>& e0 = empty[key];
                            const double pz = upp / FB_SCALE, margin = pz * (g_mushKernel + 3);
                            for (int y = 0; y < FBH; y++)
                                for (int x = 0; x < FBW; x++) {
                                    if (fb.idx[(size_t)y * FBW + x] == e0[(size_t)y * FBW + x]) continue;
                                    const double dx = (x + 0.5 - FBW * 0.5) * pz, dy = (y + 0.5 - FBH * 0.5) * pz, d = std::sqrt(dx * dx + dy * dy);
                                    if (d > e + margin) {
                                        outside++;

                                        const std::string k = fmt("%s V%d %s", MICROBE_PLAN_NAMES[m.plan], m.variant, ACT_NAMES[act]);
                                        byForm[k] = std::max(byForm[k], d / (e + margin));
                                        if (d / (e + margin) > worst) { worst = d / (e + margin); worstText = fmt("%s V%d %s at %.0f s, x%.1f %s: %.1f of %.1f um", MICROBE_PLAN_NAMES[m.plan], m.variant,
                                                                                                                      ACT_NAMES[act], t, zoom, SEEING_NAMES[way], d, e); }
                                    }
                                }
                        }
                    }
                }
            }
        }
        if (outside) for (auto& kv : byForm) printf("    out of bounds: %s by %.2f\n", kv.first.c_str(), kv.second);
        check("life: every form within its bounds at every zoom (Z-02)", outside == 0 && specimens > 250,
              fmt("%d specimens (a form in an act) of %zu forms, %d frames: %d pixels outside%s", specimens, forms.size(), frames, outside,
                  worstText.empty() ? "" : ("; the worst " + worstText).c_str()));
    }
    // a way of seeing's banks within its set of 32 at most, and where it has a field, every bank's ramp at the field's shade the field's colour
    {
        int bad = 0, wrongRamps = 0, maxBanks = 0;
        std::string why;
        for (int way = 0; way < WS_COUNT; way++) {
            bool used[BANKS];
            const int n = seeingBanks(way, used);
            maxBanks = std::max(maxBanks, n);
            Framebuffer fb;
            slidePalette(fb.pal, way);
            for (size_t i = 0; i < forms.size(); i += 2) {
                Specimen s; s.m = &forms[i]; s.seed = 3 + i; s.form = way == WS_SECTION ? CF_FOSSIL : CF_CELL;
                Slide v; v.seeing = way; v.umPerPx = umPerPxAt(magnificationFor(forms[i], way)); v.t = 1.7; v.seed = 7;
                double ang; specimenPlace(s, v.t, v.cx, v.cy, ang);
                drawSlide(fb, v, {s});
                fb.mush(2);
                for (Pix p : fb.idx) if (!used[bankOf(p)]) { bad++; if (why.size() < 80) why += fmt(" %s bank %d (%s);", SEEING_NAMES[way], bankOf(p), MICROBE_PLAN_NAMES[forms[i].plan]); break; }
            }
            if (way == WS_BRIGHT || way == WS_PHASE || way == WS_DARK) {
                const int at = way == WS_BRIGHT ? 56 : (way == WS_PHASE ? 34 : 3);
                for (int b = 0; b < BANKS; b++) {
                    if (!used[b]) continue;
                    for (int c = 0; c < 3; c++) if (std::abs((int)fb.pal[(b * 64 + at) * 3 + c] - (int)fb.pal[at * 3 + c]) > 6) { wrongRamps++; break; }
                }
            }
        }
        check("life: a way of seeing's banks its own (Z-02)", bad == 0 && wrongRamps == 0 && maxBanks <= BANKS,
              fmt("six ways, %zu forms each: %d frames with a bank of another's; at most %d banks; %d ramps not meeting the field%s", forms.size(), bad, maxBanks, wrongRamps,
                  why.c_str()));
    }
}
