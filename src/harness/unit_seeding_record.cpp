// J-12 (R-448): the unit's `seeding` group, a seeded world's record. The natives a line drives out (`seedingDiedOut`) over `seed kinds`' six
// lines and three poisons on `seed climate`'s twenty worlds, and in a game's vials; `seed record`'s world and line in a game: the line found
// wild under the microscope and read by the sequencer (the guide's `wild` lines), the catalogue's rows and the LIFE page's counts
#include "harness/harness.h"
#include "life/seeding.h"
#include "test_game.h"
#include <algorithm>
#include <cmath>

namespace {
const std::vector<std::pair<int, int>> RECORD_EDITS[9] = {{{GN_GAS, WG_CO2}}, {{GN_GAS, WG_METHANE}}, {{GN_GAS, WG_OXYGEN}}, {{GN_GAS, WG_H2S}}, {{GN_GAS, WG_DMS}},
                                                          {{GN_STOP, -2}, {GN_BIND, -1}}, {{GN_POISON, PZ_NERVE}}, {{GN_POISON, PZ_BLOOD}}, {{GN_POISON, PZ_GUT}}};
bool wetNiche(int n) { return n == NI_LIQUID || n == NI_SILT || n == NI_MAT || n == NI_SPRING || n == NI_VENT; }
bool heldIn(const Sample& s, int species) { return std::any_of(s.held.begin(), s.held.end(), [&](const Held& h) { return h.species == species; }); }
// the frame at a slide's time, the status cleared first (unit_lab.cpp's)
void frameAt(TestGame& g, Input& in, double t) { g.testScope().t = t; g.testScopeFresh(); g.testClearStatus(); g.frame(in, 0); in.newFrame(); }
}   // namespace

void unitSeedingRecord(const Check& check) {
    {   // natives died out: each a native the line beat, living only in the line's niches and salts; none a step before its last; a vial of the world
        // holds none of them after it, the natives it beat there a hundredth before it
        const std::vector<SeedAirCase> worlds = seedAirCases(20);
        int took = 0, beaten = 0, died = 0, bad = 0, early = 0, how[PA_COUNT] = {};
        std::string firstBad;
        for (const SeedAirCase& c : worlds) {
            const Biota& B = lifeOf(c.sys)->bodies[c.body];
            for (const auto& wants : RECORD_EDITS) {
                const SeedLine line = seedLineFor(c.sys, c.body, c.wild.kind.s.index, seedGeneEdits(c.wild.genes, wants));
                const Seeding s = seedingFor(c.sys, c.body, line, c.lat, c.lon, 0);
                if (!s.verdict.takes()) continue;
                took++; beaten += (int)s.verdict.beaten.size();
                const double last = (MADE_STEPS * MADE_STEP_S + 1) / s.verdict.pace;   // (its last step's first second)
                early += (int)seedingDiedOut(B, s, last - MADE_STEP_S / s.verdict.pace).size();
                const CultureKind& K = s.wild.kind;
                const bool land = K.world.dry > 0 || K.s.niche == NI_SNOW || K.s.niche == NI_ICE || (K.world.rots() && K.world.rot == ROT_STONE);
                for (const DiedOut& d : seedingDiedOut(B, s, last)) {
                    died++; how[clampi(d.how, 0, PA_COUNT - 1)]++;
                    const Microbe& m = B.species[d.species];
                    bool only = std::find(s.verdict.beaten.begin(), s.verdict.beaten.end(), d.species) != s.verdict.beaten.end();
                    for (int n = 0; n < NI_COUNT; n++) if ((m.niches & nicheBit(n)) && !wetNiche(n) && !land) only = false;
                    for (int salt = std::max(0, m.saltLo); salt <= std::min(2, m.saltHi); salt++) if (salt < K.s.saltLo - 1 || salt > K.s.saltHi + 1) only = false;
                    if (!only && ++bad == 1) firstBad = fmt("; native %d of a %s (niches %x) died out where it lives on", d.species, KINDS[c.sys.bodies[c.body].type].name, m.niches);
                }
            }
        }
        // a game's vials: at the place a step before the last and after it, and at another of the world's waters after it
        SeedRecordCase rc;
        const bool found = seedRecordCase(rc);
        int before = 0, after = 0, there = 0, elsewhere = 0, gone = 0, kept = 0;
        if (found) {
            TestGame g; seedRecordGame(g, rc, "unit_seeding_died");
            const std::vector<Seeding>& ss = g.testSeedings(rc.c.body);
            const Biota& B = lifeOf(rc.c.sys)->bodies[rc.c.body];
            if (!ss.empty()) {
                const Seeding& s = ss[0];
                const std::vector<DiedOut> out = seedingDiedOut(B, s, g.guide.clock);
                gone = (int)out.size();
                Vial v; v.sx = rc.c.sys.star.sx; v.sy = rc.c.sys.star.sy; v.sz = rc.c.sys.star.sz; v.body = rc.c.body;
                v.unit = StarSystem::bodyFromLatLon(rc.c.lat, rc.c.lon); v.hab = s.verdict.place.hab;
                v.clock = s.clock + (MADE_STEPS * MADE_STEP_S + 1) / s.verdict.pace - MADE_STEP_S / s.verdict.pace;
                Sample a = sampleOf(B, v.hab); g.testVialLines(v, a);
                v.clock = g.guide.clock;
                Sample z = sampleOf(B, v.hab); g.testVialLines(v, z);
                Vial w = v; w.hab.place ^= 0x5EED5EEDULL; w.unit = StarSystem::bodyFromLatLon(-rc.c.lat, rc.c.lon + PI);   // (another square metre of its waters)
                Sample o = sampleOf(B, w.hab), o0 = o; g.testVialLines(w, o);
                for (const DiedOut& d : out) {
                    before += heldIn(a, d.species); after += heldIn(z, d.species); there += heldIn(o0, d.species); elsewhere += heldIn(o, d.species);
                }
                for (const Held& h : z.held) kept += h.state == SS_LIVING && std::none_of(out.begin(), out.end(), [&](const DiedOut& d) { return d.species == h.species; });
            }
            remove(g.guidePath.c_str());
        }
        check("seeding: natives died out", took > 100 && died > 0 && died < beaten && bad == 0 && early == 0 && found && gone > 0 && before == gone && after == 0 && elsewhere == 0 &&
                      kept > 0,
              fmt("(%d seedings of nine lines on %zu worlds took: %d natives beaten, %d died out by the 64th step (%d outgrown, %d eaten, %d poisoned), %d a step before the "
                  "last%s; a vial at the place a step before: %d of %d held, after: %d, %d living natives kept; another square metre of its waters: %d of those it held "
                  "as generated, %d after)", took, worlds.size(), beaten, died, how[PA_COMPETE], how[PA_CYCLE], how[PA_POISON], early, firstBad.c_str(), before, gone, after, kept,
                  there, elsewhere));
    }
    {   // B-474 (KI-487): a native the line beat dies out only where none of its band of warmth is left it, the world's waters (for a water
        // niche) and land (for any other) at the habitat's 96 points read as the line's band reads them; and a later line, let go at the same
        // place once the first has covered the world, meets none of those the first drove out
        const std::vector<SeedAirCase> worlds = seedAirCases(20);
        int took = 0, died = 0, warmBad = 0, later = 0, laterMet = 0;
        std::string firstBad;
        for (const SeedAirCase& c : worlds) {
            const Body& b = c.sys.bodies[c.body];
            const BodyGen g = BodyGen::make(b);
            const Biota& B = lifeOf(c.sys)->bodies[c.body];
            for (const auto& wants : RECORD_EDITS) {
                const SeedLine line = seedLineFor(c.sys, c.body, c.wild.kind.s.index, seedGeneEdits(c.wild.genes, wants));
                const Seeding s = seedingFor(c.sys, c.body, line, c.lat, c.lon, 0);
                if (!s.verdict.takes()) continue;
                took++;
                const CultureKind& K = s.wild.kind;
                MadeWorld mw; mw.tempK = b.tempK; mw.sunless = c.sys.sunless();
                mw.freezeC = K.chemistry == CH_HYDROCARBON ? -183 : (K.chemistry == CH_AMMONIA ? -97 : 0);
                double wlo = 1e9, whi = -1e9, dlo = 1e9, dhi = -1e9;
                for (int i = 0; i < 96; i++) {
                    const double z = 1 - (2 * i + 1) / 96.0, r = std::sqrt(std::max(0.0, 1 - z * z)), phi = i * 2.399963229728653;
                    const Vec3 u(r * std::cos(phi), r * std::sin(phi), z);
                    const SurfaceSample smp = sampleSurface(g, u, 20000);
                    if (smp.material == MAT_LAVA) continue;
                    const bool wet = madeWetAt(smp);
                    const double cc = madeMeanAt(g, mw, u, smp, wet, wet ? madeSaltAt(g, smp) : 0);
                    (wet ? wlo : dlo) = std::min(wet ? wlo : dlo, cc); (wet ? whi : dhi) = std::max(wet ? whi : dhi, cc);
                }
                const double last = (MADE_STEPS * MADE_STEP_S + 1) / s.verdict.pace;
                const std::vector<DiedOut> out = seedingDiedOut(B, s, last);
                for (const DiedOut& d : out) {
                    died++;
                    const Microbe& m = B.species[d.species];
                    bool left = false;   // (a part of its band the world has and the line's leaves out)
                    for (int n = 0; n < NI_COUNT; n++) {
                        if (!(m.niches & nicheBit(n))) continue;
                        const double lo = std::max(m.tMin, wetNiche(n) ? wlo : dlo), hi = std::min(m.tMax, wetNiche(n) ? whi : dhi);
                        left |= lo < hi - 0.5 && (lo < K.s.tMin - 0.5 || hi > K.s.tMax + 0.5);
                    }
                    if (left && ++warmBad == 1)
                        firstBad = fmt("; native %d of a %s (%.0f to %.0f C) died out under a line of %.0f to %.0f C, the world's waters %.0f to %.0f C, its land %.0f to %.0f C",
                                       d.species, KINDS[b.type].name, m.tMin, m.tMax, K.s.tMin, K.s.tMax, wlo, whi, dlo, dhi);
                }
                if (out.empty()) continue;
                const std::vector<Seeding> before = {s};
                const Seeding s2 = seedingFor(c.sys, c.body, line, c.lat, c.lon, last + 1, &before);
                later++;
                for (const DiedOut& d : out)
                    laterMet += s2.verdict.native == d.species || std::count(s2.verdict.beaten.begin(), s2.verdict.beaten.end(), d.species) > 0;
            }
        }
        check("seeding: a native the line beat dies out only where none of its warmth is left it, and a later line meets none of them (B-474)",
              took > 100 && died > 50 && warmBad == 0 && later > 20 && laterMet == 0,
              fmt("(%d seedings took, %d natives died out, %d of them with a part of their band the world has and the line's leaves out%s; %d later lines "
                  "at the same place met %d of those the first drove out)", took, died, warmBad, firstBad.c_str(), later, laterMet));
    }
    SeedRecordCase rc;
    if (!seedRecordCase(rc)) { check("seeding: a line found wild", false, "(no world where a poison line drives a native out)"); return; }
    TestGame g; Input in;
    seedRecordGame(g, rc, "unit_seeding_record");
    const StarSystem& S = rc.c.sys;
    const int bi = rc.c.body;
    const std::string world = Guide::bodyKey(S.star.sx, S.star.sy, S.star.sz, bi), wildKey = world + "/L0";
    const std::vector<Seeding> ss = g.testSeedings(bi);
    {   // a line found wild: seen in a vial of its world under the microscope (its record, a SEEDED log line), read there by the sequencer (the
        // record read, what the wild changed in the KIN line), the guide's `wild` line read back and the sequencer's
        bool staged = false, seenFirst = false, logged = false, read = false, kin = false, back = false, saved = false;
        int lineAt = -1;
        if (!ss.empty()) {
            Vial v; v.sx = S.star.sx; v.sy = S.star.sy; v.sz = S.star.sz; v.body = bi; v.unit = StarSystem::bodyFromLatLon(rc.c.lat, rc.c.lon);
            v.hab = ss[0].verdict.place.hab; v.clock = g.guide.clock; v.name = "THE SEA";
            g.testColdStore().push_back(v);
            g.openScreen(GameState::MICROSCOPE); g.frame(in, 0); in.newFrame();
            staged = g.testScopeStage((int)g.testColdStore().size() - 1);
            MicroscopeState& M = g.testScope();
            for (int i = 0; i < (int)M.wild.size(); i++) if (M.wild[i] == wildKey) lineAt = i;
            const SlideKind* K = nullptr;
            for (const SlideKind& k : M.mount.kinds) if (k.species == lineAt) K = &k;
            if (K) {
                const Morph& m = M.forms[lineAt];
                const double mag = magnificationFor(m, WS_BRIGHT, 0.3);
                int step = 0;
                for (int i = 0; i < SCOPE_LIGHT_STEPS; i++) if (std::fabs(std::log(SCOPE_LIGHT[i] / mag)) < std::fabs(std::log(SCOPE_LIGHT[step] / mag))) step = i;
                const SlideCell& c = M.mount.cells[K->first];
                g.testScopeView(WS_BRIGHT, step, c.x, c.y, c.z); frameAt(g, in, 0.5);
            }
            seenFirst = g.guide.wildFound.count(wildKey) && !g.guide.wildFound[wildKey];
            for (const LogEntry& e : g.guide.log) logged = logged || (e.kind == "SEEDED" && e.text.find("FOUND IN THE WILD") != std::string::npos);
            g.closeScreen();
            for (const SequencerChoice& c : g.testSequencerChoices())
                if (c.wildKey == wildKey && !read) {
                    read = g.testSeqPutWild(c);
                    std::string lines; g.testSeqSave(lines);
                    saved = lines.find("seqwildkey " + wildKey) != std::string::npos;
                }
            g.testLabSkip(SEQUENCE_S + 1);
            read = read && g.guide.wildFound[wildKey];
            for (const LogEntry& e : g.guide.log) kin = kin || (e.kind == "KIN" && e.text.find("IN THE WILD") != std::string::npos);
            g.guide.save(g.guidePath);
            Guide R; R.load(g.guidePath);
            back = R.wildFound == g.guide.wildFound;
        }
        check("seeding: a line found wild", staged && lineAt >= 0 && seenFirst && logged && read && saved && kin && back,
              fmt("(%s's line in a vial of its sea: %s, the line %s on the slide; its record %s, the log %s; the sequencer %s (its save line %s), the KIN line %s; the guide's "
                  "`wild` lines %s)", KINDS[S.bodies[bi].type].name, staged ? "staged" : "NOT STAGED", lineAt >= 0 ? fmt("species %d", lineAt).c_str() : "NOT", seenFirst ? "made" : "NOT MADE",
                  logged ? "says FOUND IN THE WILD" : "SAYS NOTHING", read ? "read it there" : "DID NOT READ IT", saved ? "with its record" : "WITHOUT IT",
                  kin ? "says what the wild changed" : "SAYS NOTHING OF IT", back ? "read back" : "NOT READ BACK"));
    }
    {   // the record: the catalogue's rows (the world's head its made kind, each native seen that died out DIED OUT, the line SEEDED, the animals
        // seen with their marks; a friend's lent sheet of one that died out marked, never counted), the LIFE page's counts the catalogue's; a game
        // nobody seeded: no DIED OUT, no SEEDED, no seeded lines on the LIFE page
        const Biota& B = lifeOf(S)->bodies[bi];
        const std::vector<DiedOut> out = ss.empty() ? std::vector<DiedOut>() : seedingDiedOut(B, ss[0], g.guide.clock);
        for (const DiedOut& d : out) g.guide.microbes[world + "/M" + std::to_string(d.species)].seen |= SEEN_FORM;
        g.guide.animals[world + "/A0"].marks = AM_GONE;
        g.guide.animals[world + "/A1"].marks = AM_CHANGED;
        g.openScreen(GameState::CATALOGUE);
        const CatalogueState& C = g.testCat();
        int died = 0, wild = 0, animals = 0, goneRows = 0;
        std::string head;
        auto rows = [&](int& d, int& w, int& a, int& gr) {
            d = w = a = gr = 0;
            for (const CatalogueRow& r : C.rows) {
                if (r.head) { if (r.key == world) head = g.testCatLine(r); continue; }
                d += r.kind == CR_SPECIES && g.testCatLine(r).find("DIED OUT") != std::string::npos;
                w += r.kind == CR_WILD; a += r.kind == CR_ANIMAL; gr += r.kind == CR_ANIMAL && g.testCatLine(r).find("GONE") != std::string::npos;
            }
        };
        rows(died, wild, animals, goneRows);
        g.testCatCounts();
        const CatalogueCounts N = C.counts;
        const std::string kind = madeKindName(g.testMadeNow(bi));
        const bool headRight = head.find(kind.empty() ? KINDS[S.bodies[bi].type].name : kind.c_str()) != std::string::npos;
        // the first native died out seen by a friend alone (their sheet lent, the explorer's taken away): its row marked, never counted
        bool lentMarked = true; int lentCount = N.diedOut;
        if (!out.empty()) {
            const std::string lk = world + "/M" + std::to_string(out[0].species);
            const MicrobeSheet own = g.guide.microbes[lk];
            g.guide.microbes.erase(lk); g.guide.lentMicrobes[lk].seen = SEEN_FORM;
            g.closeScreen(); g.openScreen(GameState::CATALOGUE);
            lentMarked = false;
            for (const CatalogueRow& r : C.rows) lentMarked = lentMarked || (r.key == lk && r.lent && g.testCatLine(r).find("DIED OUT") != std::string::npos);
            g.testCatCounts(); lentCount = C.counts.diedOut;
            g.guide.lentMicrobes.erase(lk); g.guide.microbes[lk] = own;
        }
        g.closeScreen();
        // a game nobody seeded, its guide's sheets and animals the same
        TestGame p;
        p.savePrefix = "shots/tests/unit_seeding_plain_save"; p.settingsPath = "shots/tests/test_settings.txt"; p.keysPath = "shots/tests/test_keys.txt";
        p.guidePath = "shots/tests/unit_seeding_plain_guide.txt"; remove(p.guidePath.c_str());
        p.newGame(); p.settings.cabin = false; applyTestScale(p);
        p.guide.microbes = g.guide.microbes; p.guide.animals = g.guide.animals;
        p.openScreen(GameState::CATALOGUE);
        int pd = 0, pw = 0, pa = 0;
        { const CatalogueState& P = p.testCat(); for (const CatalogueRow& r : P.rows) { if (r.head) continue; pd += r.kind == CR_SPECIES && p.testCatLine(r).find("DIED OUT") != std::string::npos; pw += r.kind == CR_WILD; pa += r.kind == CR_ANIMAL; } }
        p.testCatCounts();
        const bool plain = pd == 0 && pw == 0 && pa == animals && p.testCat().counts.worlds == 0;
        remove(p.guidePath.c_str());
        {   // KI-482: the made kind named beyond the data sheet and the catalogue: the target's label, the analyzer's and the telescope's short
            // name, the orbit's and the landing's log words, the tree's page; every kind's short name ten characters at most
            const MadeWorld made = g.testMadeNow(bi);
            const std::string full = madeKindName(made), shortName = madeKindShort(made), once = full + ", ONCE " + KINDS[S.bodies[bi].type].name;
            bool fits = true;
            for (int k = MDK_NONE + 1; k < MDK_COUNT; k++)
                for (int t = 0; t < (k == MDK_OVERGROWN ? OG_COUNT : 1); t++) {
                    MadeWorld m; m.kind = k; m.kindGrowth = t;
                    fits = fits && !madeKindShort(m).empty() && madeKindShort(m).size() <= 10 && !madeKindName(m).empty();
                }
            const std::string label = upper(g.bodyLabelOf(bi)), tele = g.testTelescopeLineOf(bi), page = g.testTreeWorldWords(world, S.bodies[bi].type);
            const bool named = !full.empty() && g.kindNameNow(bi) == full && g.kindShortNow(bi) == shortName && g.kindWordsNow(bi) == once &&
                               label.find(full) != std::string::npos && tele.find(shortName) != std::string::npos && page.find(full) != std::string::npos;
            check("seeding: a made kind named", named && fits,
                  fmt("(%s: the label \"%s\", the telescope \"%s\", the log \"%s\", the tree's page \"%s\"; every short name %s)", full.empty() ? "NO MADE KIND" : full.c_str(),
                      label.c_str(), tele.c_str(), g.kindWordsNow(bi).c_str(), page.c_str(), fits ? "ten characters at most" : "TOO LONG OR EMPTY"));
        }
        check("seeding: the record", !out.empty() && died == (int)out.size() && wild == 1 && animals >= 2 && goneRows == 1 && headRight && N.worlds == 1 && N.sent == 1 && N.took == 1 &&
                      N.diedOut == died && N.gone == 1 && (int)N.kinds.size() == (kind.empty() ? 0 : 1) && lentMarked && lentCount == N.diedOut - 1 && plain,
              fmt("(the catalogue: \"%s\", %d of %zu natives died out marked DIED OUT, %d line SEEDED, %d animals (%d GONE); a friend's lent sheet %s, the count %d then; "
                  "the LIFE page: %d world, %d of %d lines took, %s, %d died out, %d gone; a game nobody seeded: %d DIED OUT, %d SEEDED, %d animals, %d worlds)", head.c_str(), died,
                  out.size(), wild, animals, goneRows, lentMarked ? "marked" : "NOT MARKED", lentCount, N.worlds, N.took, N.sent, kind.empty() ? "no made kind" : kind.c_str(),
                  N.diedOut, N.gone, pd, pw, pa, p.testCat().counts.worlds));
    }
    remove(g.guidePath.c_str());
}
