// Z-06: the genomes and the tree (life/sequences.h, game/sequencer.cpp, game/tree_screen.cpp, peoples/made.h): `lab tree`, the flow's species
// sequenced and the tree's screen, the kin near home across stars with their years; `life made`, the made species near home and their
// watermarks as the decoder reads them, their repeats measured as the records' are
#include "harness.h"
#include "life/sequences.h"
#include "peoples/made.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>

namespace {
// the ranking within a deep branch over the systems near home: each species against the others of its genesis by where they live (one world,
// one system, two stars), the common ancestors' ranges and the markers' agreement; and across geneses none
struct KinStats {
    double lo[3] = {1e9, 1e9, 1e9}, hi[3] = {0, 0, 0}, alike[3] = {0, 0, 0};
    long n[3] = {0, 0, 0}, across = 0, acrossNone = 0, wrong = 0;
};
int where(const Lineage& a, const Lineage& b) {
    if (a.lx != b.lx || a.ly != b.ly || a.lz != b.lz) return 2;
    if (a.wx != b.wx || a.wy != b.wy || a.wz != b.wz || a.world != b.world) return 1;
    return 0;
}
}   // namespace

// the kin of home's genesis within r sectors of home: each system's water life of it, its line's parting and the common ancestor with the
// ship's own; the ranking's measure over every living system there
int treeKin(int r) {
    const double t0 = nowSec();
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
    const Genesis hg = homeGenesis();
    const Lineage ship = shipLineage(0);
    printf("home's genesis %016llx: %s, %d letters (%s), began %.2f billion years ago; its deep branches parted %.2f, its cell's lines %.2f, home's box of "
           "sixteen sectors %.2f, of four %.2f; home's worlds' last exchange %.2f\n", (unsigned long long)hg.id, hg.mirrored ? "mirrored" : "ours in hand", hg.letters,
           alphabetOf(hg.letters), hg.ageGyr, ship.cladeGyr, ship.cellGyr, ship.wideGyr, ship.nearGyr, ship.exchangeGyr);
    for (int k = 0; k < SHIP_SPECIES; k++) {
        const Lineage L = shipLineage(k);
        printf("  the ship's %d (%s, branch %d): leaf %d of %d on home's world; marker %s...\n", k, MICROBE_PLAN_NAMES[L.plan], L.clade, L.leaf, L.leaves,
               markerOf(L, 0, 48).c_str());
    }
    std::vector<std::pair<Lineage, std::string>> all;   // every sequenceable species of the systems near home, with its label
    struct Kin { std::string star; double ly, years, part; int worlds; };
    std::vector<Kin> kin;
    for (const Star& s : starsNearHome(r)) {
        StarSystem sys; sys.generate(s);
        auto L = lifeOf(sys);
        bool kinHere = false; int worlds = 0; double years = -1, part = 0;
        for (int i = 0; i < (int)sys.bodies.size(); i++) {
            const Biota& B = L->bodies[i];
            if (!B.genesis.id) continue;
            for (const Microbe& m : B.species) {
                const Lineage ln = lineageOf(sys, i, m.index);
                if (!ln.valid) continue;
                if ((int)all.size() < 6000) all.push_back({ln, fmt("%s M%d", sys.bodyLabel(i).c_str(), m.index)});
                if (B.genesis.id == hg.id && ln.clade == 0) {
                    const double ca = commonAncestorGyr(ln, ship);
                    if (!kinHere) worlds = 0;
                    kinHere = true; years = std::max(years, ca); part = ln.nearGyr;
                }
            }
            if (B.genesis.id == hg.id && livingStage(B.stage)) worlds++;
        }
        if (kinHere && !(s.sx == HOME_SX && s.sy == HOME_SY && s.sz == HOME_SZ)) kin.push_back({s.name, length(s.pos - home.pos) / SECTOR_KM, years, part, worlds});
    }
    printf("the kin of home's life within %d sectors (%zu systems; their life's line left the cell's, the common ancestor with the ship's soil):\n", r, kin.size());
    for (const Kin& k : kin) printf("  %-16s %5.1f ly  its star's line %.2f Gyr ago, %d living world%s; with the ship's own: %s ago\n", k.star.c_str(), k.ly, k.part, k.worlds,
                                    k.worlds == 1 ? "" : "s", yearsWords(k.years * 1e9).c_str());
    KinStats S;
    for (size_t i = 0; i < all.size(); i++) {
        double nearest[3] = {1e9, 1e9, 1e9}, farthest[3] = {-1, -1, -1};
        for (size_t j = 0; j < all.size(); j++) {
            if (i == j) continue;
            const Lineage& a = all[i].first; const Lineage& b = all[j].first;
            const double ca = commonAncestorGyr(a, b);
            if (a.genesis.id != b.genesis.id) { if (j % 97 == i % 97) { S.across++; S.acrossNone += ca < 0; } continue; }
            if (a.clade != b.clade || ca <= 0) continue;
            const int w = where(a, b);
            nearest[w] = std::min(nearest[w], ca); farthest[w] = std::max(farthest[w], ca);
            S.lo[w] = std::min(S.lo[w], ca); S.hi[w] = std::max(S.hi[w], ca); S.n[w]++;
            if (S.n[w] % 7 == 1) S.alike[w] += markerAlike(a, b);
        }
        if (farthest[0] >= 0 && nearest[1] < 1e9 && farthest[0] >= nearest[1]) S.wrong++;
        if (farthest[1] >= 0 && nearest[2] < 1e9 && farthest[1] >= nearest[2]) S.wrong++;
        if (farthest[0] >= 0 && nearest[2] < 1e9 && farthest[0] >= nearest[2]) S.wrong++;
    }
    const char* W[3] = {"one world", "one system", "two stars"};
    printf("the ranking over %zu species within a deep branch (%.1f s):\n", all.size(), nowSec() - t0);
    for (int w = 0; w < 3; w++)
        printf("  %-10s %7ld pairs, their common ancestor %.3f-%.2f Gyr ago, their markers %.0f%% alike\n", W[w], S.n[w], S.n[w] ? S.lo[w] : 0.0, S.hi[w],
               S.n[w] ? 100.0 * S.alike[w] / std::max(1L, (S.n[w] + 6) / 7) : 0.0);
    printf("  a species with a nearer kin farther than a farther one: %ld; across geneses %ld pairs, %ld with no common ancestor\n", S.wrong, S.across, S.acrossNone);
    return 0;
}

// `life made [n]`: the made species of the n worlds of late peoples nearest home (the futuristic and the far future's): each world, its people,
// the species and the year they made it, its watermark's letters and its text as the decoder reads it whole (the first five as one shard
// and none read it, in their tongue); the grammar's checks on every one, the names the history's at the year, and the repeated frames at a
// hundred (the records' measure: sentences of six words or more, names and numbers masked, the formulas left out)
int lifeMade(int want) {
    const double t0 = nowSec();
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
    std::shared_ptr<const VgGrammar> G = shardGrammar();
    std::vector<std::string> texts; std::vector<std::vector<std::string>> forms;
    int counts[VC_COUNT] = {}, failed = 0, namesTold = 0, namesBad = 0, minW = 1 << 30, maxW = 0, worlds = 0, none = 0, asleep = 0;
    long words = 0;
    int byWay[LW_COUNT] = {};
    std::vector<std::string> failures;
    const int R = 40;
    struct St { int64_t x, y, z; double d; };
    std::vector<St> stars;
    for (int64_t x = HOME_SX - R; x <= HOME_SX + R; x++) for (int64_t y = HOME_SY - R; y <= HOME_SY + R; y++) for (int64_t z = HOME_SZ - R; z <= HOME_SZ + R; z++) {
        const double d = std::sqrt((double)((x - HOME_SX) * (x - HOME_SX) + (y - HOME_SY) * (y - HOME_SY) + (z - HOME_SZ) * (z - HOME_SZ)));
        Star s; if (d > R || !starInSector(x, y, z, s, false)) continue;
        stars.push_back({x, y, z, d});
    }
    std::sort(stars.begin(), stars.end(), [](const St& a, const St& b) { return a.d != b.d ? a.d < b.d : (a.x != b.x ? a.x < b.x : (a.y != b.y ? a.y < b.y : a.z < b.z)); });
    printf("life made: the made species of the %d worlds of late peoples nearest home, their watermarks as the decoder reads them\n", want);
    for (const St& n : stars) {
        if ((int)texts.size() >= want) break;
        Star s; starInSector(n.x, n.y, n.z, s, true);
        StarSystem sys; sys.generate(s);
        for (const Body& b : sys.bodies) {
            if ((int)texts.size() >= want) break;
            if (!kindHas(b.type, KT_PEOPLE)) continue;
            const BodyGen g = BodyGen::make(b);
            if (!worldHadCivilisation(g) || peopleAgeOf(g, 0).age < AGE_FUTURISTIC) continue;
            worlds++;
            const MadeSpecies m = madeSpeciesOf(sys, b.index);
            if (m.species < 0) { none++; continue; }
            const Lore L = loreQuick(sys, b, g, m.people);
            Shard w; VgRecord rec;
            if (!madeWatermarkOf(sys, b.index, L, m, w, &rec)) { none++; continue; }
            const Microbe& sp = lifeOf(sys)->bodies[b.index].species[m.species];
            Lineage ln = lineageOf(sys, b.index, m.species); ln.made = true;
            asleep += sp.state == SS_DORMANT; byWay[sp.way]++;
            texts.push_back(w.text); forms.push_back(rec.formulas);
            const int nw = shardWords(w.text); words += nw; minW = std::min(minW, nw); maxW = std::max(maxW, nw);
            const int age = peopleAgeOf(g, m.people).age;
            std::vector<std::string> why;
            if (vgCheck(*G, rec, w.recordAge, counts, &why)) { failed++; if (failures.size() < 20) failures.push_back(fmt("%s: %s\n      %s", b.name.c_str(), why[0].c_str(), w.text.c_str())); }
            if (std::shared_ptr<const History> H = historyOf(*peoplesWorldOf(sys, b).sys, *peoplesWorldOf(sys, b).b, g, m.people))
                for (const std::string& nm : rec.names) { namesTold++; if (!recordNameKnown(*H, L, m.year, nm)) { namesBad++; if (failures.size() < 20) failures.push_back(fmt("%s: the name '%s' not the history's in year %d", b.name.c_str(), nm.c_str(), m.year)); } }
            if ((int)texts.size() > 50) continue;
            printf("\n%2zu %s (%s, %.1f ly), the %s (%s): made in their year %d, M%d a %s %s of the %s%s; %d letters%s\n", texts.size(), b.name.c_str(), s.name.c_str(), n.d,
                   L.people.c_str(), AGE_NAMES[age], m.year, m.species, MICROBE_PLAN_NAMES[sp.plan], WAY_NAMES[sp.way], NICHE_NAMES[sp.niche], sp.state == SS_DORMANT ? ", asleep" : "",
                   ln.genesis.letters, ln.genesis.mirrored ? ", mirrored" : "");
            printf("   its letters: %s...\n", watermarkLetters(w.text, ln.genesis.letters, 0, 60).c_str());
            printf("   %s\n", w.text.c_str());
            if (texts.size() <= 5) {   // as the decoder reads it with none of their records held, and with three
                std::vector<Shard> recs; shardsOf(sys, b, g, L, L.records, recs);
                const Tongue T = tongueOf(g, L);
                Language lang; languageOf(L, recs, T, lang);
                for (int held : {0, 3}) {
                    std::vector<DecodedWord> dw;
                    const int known = decodeShard(w, lang, T, held, dw);
                    printf("   with %d of their records read (%d of %zu words): %s\n", held, known, dw.size(), decodedText(dw).c_str());
                }
            }
        }
    }
    std::vector<const std::vector<std::string>*> f; for (const auto& x : forms) f.push_back(&x);
    std::vector<std::string> t100(texts.begin(), texts.begin() + std::min<size_t>(100, texts.size()));
    const VgMeasure M = vgMeasure(t100, &f);
    printf("\nthe made: %d worlds of late peoples, %zu made species (%d asleep; %d light, %d chemical, %d dead, %d grazers, %d predators), %d with none (%.1f s)\n",
           worlds, texts.size(), asleep, byWay[LW_LIGHT], byWay[LW_CHEMICAL], byWay[LW_DEAD], byWay[LW_GRAZER], byWay[LW_PREDATOR], none, nowSec() - t0);
    printf("  their watermarks: %ld words a text (%d-%d); the checks failed on %d", texts.empty() ? 0L : words / (long)texts.size(), texts.empty() ? 0 : minW, maxW, failed);
    for (int k = 0; k < VC_COUNT; k++) if (counts[k]) printf(", %s %d", VG_CHECK_NAMES[k], counts[k]);
    printf("; %d names, %d not the history's at the year\n", namesTold, namesBad);
    printf("  at %d: %d sentences of six words or more, %d on a repeated frame (%.1f%%; the records' aim 5%% at a hundred)\n", M.texts, M.longS, M.longRep,
           M.longS ? 100.0 * M.longRep / M.longS : 0.0);
    std::vector<std::pair<int, std::string>> top; vgTopFrames(t100, 8, top, &f);
    for (const auto& t : top) printf("    x%d  %s\n", t.first, t.second.c_str());
    for (const std::string& x : failures) printf("  FAIL %s\n", x.c_str());
    return failed || namesBad ? 1 : 0;
}

namespace {
// a species of a world to sequence: its world, its index, what it is (the tree's printout)
struct Pick { StarSystem sys; int body = -1, species = -1; std::string why; };
// a world's sequenceable species (alive or asleep, its own; of one deep branch where `clade` >= 0), at most n
void picksOf(const StarSystem& sys, int body, int n, const std::string& why, std::vector<Pick>& out, int clade = -1) {
    if (body < 0) return;
    const Biota& B = lifeOf(sys)->bodies[body];
    int k = 0;
    for (const Microbe& m : B.species) {
        if (k >= n) break;
        if (!lineageOf(sys, body, m.index).valid || (clade >= 0 && m.clade != clade)) continue;
        Pick p; p.sys = sys; p.body = body; p.species = m.index; p.why = why;
        out.push_back(p); k++;
    }
}
// the first world of a system whose water life is of a genesis that passes `want`
int worldOfGenesis(const StarSystem& sys, const std::function<bool(const Genesis&)>& want) {
    auto L = lifeOf(sys);
    for (int i = 0; i < (int)sys.bodies.size(); i++) {
        const Biota& B = L->bodies[i];
        if (!livingStage(B.stage) || !B.genesis.id || sys.bodies[i].wanderer || !want(B.genesis)) continue;
        return i;
    }
    return -1;
}
}   // namespace

// `lab tree`: the tree of a sequenced set: the flow's world's species (the scan's first felisian world, where the flow lands and samples),
// home's world's, a kin star's (home's genesis across stars), a world of a genesis of its own, a mirrored life's, a drifter's and the star it
// left, a made species read on the sequencer (its screen as it reads and once read: `lab_sequencer.png`); the tree printed (each leaf's join
// with the one before, the geneses' heads) and drawn (`lab_tree.png`); the decoder's records of the made species' people with its watermark
// (`lab_watermark.png`); then the kin near home across stars with their years (`treeKin`)
int labTree() {
    const double t0 = nowSec();
    TestGame g; Input in;
    g.savePrefix = "shots/tests/lab_tree_save"; g.settingsPath = "shots/tests/lab_tree_settings.txt"; g.guidePath = "shots/tests/lab_tree_guide.txt"; g.keysPath = "shots/tests/test_keys.txt";
    remove(g.guidePath.c_str()); remove(g.settingsPath.c_str());
    g.newGame(); g.setState(GameState::SPACE);
    applyTestScale(g);
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
    std::vector<Pick> picks;
    {   // the flow's world
        StarSystem fs; int fb = -1;
        if (firstBodyOfType(PT_FELISIAN, fs, fb)) picksOf(fs, fb, 4, "the flow's world", picks);
    }
    {   // home's world
        StarSystem hs; hs.generate(home);
        picksOf(hs, worldOfGenesis(hs, [](const Genesis& G) { return G.id == HOME_GENESIS_ID; }), 2, "home's world", picks);
    }
    bool kin = false, own = false, mirrored = false;
    for (const Star& s : starsNearHome(8)) {   // a kin star, a genesis of its own, a mirrored life
        if (kin && own && mirrored) break;
        if (s.sx == HOME_SX && s.sy == HOME_SY && s.sz == HOME_SZ) continue;
        StarSystem sys; sys.generate(s);
        if (!kin) { const int w = worldOfGenesis(sys, [](const Genesis& G) { return G.id == HOME_GENESIS_ID; }); if (w >= 0) { picksOf(sys, w, 2, "a kin star's", picks); kin = true; continue; } }
        if (!own) { const int w = worldOfGenesis(sys, [](const Genesis& G) { return G.origin == GO_OWN && !G.mirrored; }); if (w >= 0) { picksOf(sys, w, 2, "a genesis of its own", picks); own = true; continue; } }
        if (!mirrored) { const int w = worldOfGenesis(sys, [](const Genesis& G) { return G.mirrored; }); if (w >= 0) { picksOf(sys, w, 1, "a mirrored life", picks); mirrored = true; } }
    }
    {   // a drifter with life of the star it left, and that star's life
        std::vector<Star> near; driftersNear(home.pos, 1500, near);
        bool done = false;
        for (const Star& d0 : near) {
            if (done) break;
            Star d; if (!drifterInSector(d0.sx, d0.sy, d0.sz, d, true)) continue;
            StarSystem ds; ds.generate(d);
            const DrifterPast past = drifterPast(d);
            if (!past.found) continue;
            auto L = lifeOf(ds);
            for (int i = 0; i < (int)ds.bodies.size() && !done; i++) {
                const Biota& B = L->bodies[i];
                if (!B.genesis.id || ds.bodies[i].wanderer) continue;
                StarSystem os; os.generate(past.star);
                const uint64_t gid = B.genesis.id;
                const int w = worldOfGenesis(os, [gid](const Genesis& G) { return G.id == gid; });
                if (w < 0) continue;
                const size_t before = picks.size();
                picksOf(ds, i, 1, "a drifter's", picks);
                if (picks.size() == before) continue;
                picksOf(os, w, 1, "the star the drifter left", picks, lifeOf(ds)->bodies[i].species[picks.back().species].clade);   // its deep branch's
                done = true;
            }
        }
    }
    // the made species: the nearest late people's world's, read on the sequencer
    StarSystem madeSys; int madeBody = -1; MadeSpecies made;
    for (const Star& s : starsNearHome(16)) {
        StarSystem sys; sys.generate(s);
        for (const Body& b : sys.bodies) {
            if (!kindHas(b.type, KT_PEOPLE)) continue;
            const MadeSpecies m = madeSpeciesOf(sys, b.index);
            if (m.species >= 0) { madeSys = sys; madeBody = b.index; made = m; break; }
        }
        if (madeBody >= 0) break;
    }
    // every pick seen and read; the made one through the sequencer
    for (const Pick& p : picks) g.guide.microbes[Guide::bodyKey(p.sys.star.sx, p.sys.star.sy, p.sys.star.sz, p.body) + "/M" + std::to_string(p.species)].seen |= SEEN_FORM | SEEN_KIN;
    std::vector<std::pair<std::vector<uint32_t>, std::string>> seqShots;
    auto grab = [&](std::vector<std::pair<std::vector<uint32_t>, std::string>>& into, const std::string& label) {
        g.testClearStatus(); g.frame(in, 0); in.newFrame(); g.testClearStatus(); g.frame(in, 0); in.newFrame();
        into.push_back({std::vector<uint32_t>(g.output(), g.output() + (size_t)FBW * FBH), label});
    };
    std::string madeKey;
    if (madeBody >= 0) {
        madeKey = Guide::bodyKey(madeSys.star.sx, madeSys.star.sy, madeSys.star.sz, madeBody) + "/M" + std::to_string(made.species);
        g.guide.microbes[madeKey].seen |= SEEN_FORM;
        g.openScreen(GameState::SEQUENCER);
        const bool put = g.testSeqPut(madeSys.star.sx, madeSys.star.sy, madeSys.star.sz, madeBody, made.species);
        g.testLabSkip(SEQUENCE_S * 0.45); grab(seqShots, "READING A MADE SPECIES' GENOME, ITS WATERMARK BETWEEN THE ROWS");
        g.testLabSkip(SEQUENCE_S * 0.6); grab(seqShots, "READ: ITS PLACE ON THE TREE");
        const SequencerState& Q = g.testLab().seq;
        printf("the sequencer: %s %s of %s (%s); its result:\n", put ? "read" : "COULD NOT READ", Q.name.c_str(), Q.world.c_str(), madeSys.bodies[madeBody].name.c_str());
        for (const std::string& l : Q.result) printf("    %s\n", l.c_str());
        printf("  the guide: %s sequenced, its watermark %s\n", (g.guide.microbes[madeKey].seen & SEEN_KIN) ? "is" : "is NOT", g.guide.watermarks.count(madeKey) ? "the decoder's" : "NOT held");
        g.closeScreen();
    }
    if (!seqShots.empty()) {
        const int gap = 4, head = 14;
        Sheet sh((int)seqShots.size() * (FBW + gap) + gap, FBH + 2 * gap + head);
        sh.label(4, 3, "lab tree: the sequencer reading a made species and read");
        for (size_t i = 0; i < seqShots.size(); i++) { sh.blit(seqShots[i].first, FBW, FBH, gap + (int)i * (FBW + gap), head + gap); sh.label(gap + (int)i * (FBW + gap) + 2, head + gap + FBH - 11, seqShots[i].second); }
        sh.save("shots/tests/lab_sequencer.png");
    }
    // the tree: printed and drawn at three leaves (the first, a drifter's, the made one)
    g.openScreen(GameState::TREE);
    TreeState& T = g.testTree();
    printf("\nthe tree of %zu leaves (%zu picked, the ship's own three, the made one): life began at least %d times\n", T.leaves.size(), picks.size(), T.geneses);
    for (size_t k = 0; k < T.leaves.size(); k++) {
        const TreeLeaf& L = T.leaves[k];
        if (T.join[k] < 0)
            printf("  -- a genesis: %016llx, %s, %d letters (%s), %s, began %.2f billion years ago\n", (unsigned long long)L.line.genesis.id, GENESIS_ORIGIN_NAMES[L.line.genesis.origin],
                   L.line.genesis.letters, alphabetOf(L.line.genesis.letters), L.line.genesis.mirrored ? "mirrored" : "ours in hand", L.line.genesis.ageGyr);
        std::string why;
        for (const Pick& p : picks) if (Guide::bodyKey(p.sys.star.sx, p.sys.star.sy, p.sys.star.sz, p.body) + "/M" + std::to_string(p.species) == L.key) why = p.why;
        if (L.key == madeKey) why = "the made one";
        double ny = -1; const int nk = g.testTreeNearest((int)k, ny);
        printf("  %-26s %-28s %-16s branch %d, leaf %d of %d%s%s; nearest %s\n", T.join[k] < 0 ? "" : fmt("joins above %s ago", yearsWords(std::max(1.0, T.join[k] * 1e9)).c_str()).c_str(),
               (L.name + ", " + L.world).c_str(), why.c_str(), L.line.clade, L.line.leaf, L.line.leaves, L.line.made ? ", MADE" : "", L.drifter ? ", A DRIFTER'S" : "",
               nk < 0 ? "none" : fmt("%s (%s ago)", T.leaves[nk].key.c_str(), yearsWords(std::max(1.0, ny * 1e9)).c_str()).c_str());
    }
    // Y-01: the lab's lines and a folder: the flow's world's first pick given two strains (the second of the first), a folder holding a kin
    // star's pick, a mirrored life's and that second strain kept alone; every page printed, five drawn
    std::string lined, drifterKey;
    if (!picks.empty()) {
        const Pick& p = picks[0];
        lined = Guide::bodyKey(p.sys.star.sx, p.sys.star.sy, p.sys.star.sz, p.body) + "/M" + std::to_string(p.species);
        StrainRecord s1, s2;
        s1.what = SM_HOT; s1.value = std::round(lifeOf(p.sys)->bodies[p.body].species[p.species].tMax + 3);
        s2.parent = 1; s2.what = SM_SIZE; s2.value = 1.25;
        g.guide.strains[lined + "/S1"] = s1; g.guide.strains[lined + "/S2"] = s2;
        CultureFolder f; f.id = 1; f.name = "THE PICKS";
        g.testLab().folders.push_back(f);
        auto keep = [&](const Pick& q, int strain) {
            KeptCulture K; K.folder = 1;
            K.who.sx = q.sys.star.sx; K.who.sy = q.sys.star.sy; K.who.sz = q.sys.star.sz; K.who.body = q.body; K.who.species = q.species;
            K.who.strain = K.who.strainIn = strain;
            g.testLab().kept.push_back(K);
        };
        for (const Pick& q : picks) if (q.why == "a kin star's" || q.why == "a mirrored life") keep(q, 0);
        keep(p, 2);
    }
    for (const TreeLeaf& L : g.testTree().leaves) if (L.drifter && drifterKey.empty()) drifterKey = L.key;
    g.closeScreen(); g.openScreen(GameState::TREE);
    printf("\nits pages (Y-01): %zu\n", T.pages.size());
    for (size_t p = 0; p < T.pages.size(); p++) {
        g.testTreePage((int)p);
        printf("  %zu. %s: %zu rows, %d not read, its years %s\n", p + 1, T.pages[p].name.c_str(), T.shown.size(), T.unread,
               yearsWords(std::max(1.0, g.testTreeYears() * 1e9)).c_str());
        for (const TreeLeaf& L : T.shown)
            printf("     %*s%s%s\n", 3 * L.depth, "", TestGame::testTreeLine(L, T.pages[p].kind != TP_WORLD).c_str(), L.lab ? "  (a lab's line)" : "");
    }
    std::vector<std::pair<std::vector<uint32_t>, std::string>> treeShots;
    g.testTreePage(0); grab(treeShots, "THE SHIP'S OWN");
    if (!lined.empty()) {
        g.testTreeShow(lined);
        for (size_t k = 0; k < T.shown.size(); k++) if (T.shown[k].key == lined + "/S1") T.sel = (int)k;
        grab(treeShots, "THE FLOW'S WORLD: A STRAIN AND ITS STRAIN");
    }
    if (!drifterKey.empty()) { g.testTreeShow(drifterKey); grab(treeShots, "A DRIFTER'S, ITS KIN THE STAR IT LEFT"); }
    if (!madeKey.empty()) { g.testTreeShow(madeKey); grab(treeShots, "THE MADE ONE'S WORLD"); }
    for (size_t p = 0; p < T.pages.size(); p++) if (T.pages[p].kind == TP_FOLDER) { g.testTreePage((int)p); grab(treeShots, "A FOLDER'S TREE"); break; }
    g.closeScreen();
    {
        const int gap = 4, head = 14;
        Sheet sh((int)treeShots.size() * (FBW + gap) + gap, FBH + 2 * gap + head);
        sh.label(4, 3, fmt("lab tree: the tree of kin by world and by folder, %zu leaves read, life began at least %d times", T.leaves.size(), T.geneses));
        for (size_t i = 0; i < treeShots.size(); i++) { sh.blit(treeShots[i].first, FBW, FBH, gap + (int)i * (FBW + gap), head + gap); sh.label(gap + (int)i * (FBW + gap) + 2, head + gap + FBH - 11, treeShots[i].second); }
        sh.save("shots/tests/lab_tree.png");
    }
    // the decoder: the made species' people's records, its watermark among them, read with none of their records held
    if (madeBody >= 0) {
        const std::string wk = Guide::bodyKey(madeSys.star.sx, madeSys.star.sy, madeSys.star.sz, madeBody);
        g.testOpenShards();
        std::vector<std::string>& keys = g.testShardWorldKeys();
        int row = -1; for (size_t i = 0; i < keys.size(); i++) if (keys[i] == wk) row = (int)i;
        for (int i = 0; i < row; i++) { pressKey(g, in, KEY_DOWN); }
        pressKey(g, in, KEY_ENTER);
        const ShardWorld& W = g.testShardWorldNow();
        const bool opened = W.valid && W.watermark >= 0 && g.testShardsOpenIndex(W.watermark);
        for (int k = 0; k < 4; k++) { g.frame(in, 0.05); in.newFrame(); }
        g.testClearStatus(); g.frame(in, 0); in.newFrame();
        writePNG("shots/tests/lab_watermark.png", g.output(), FBW, FBH);
        Shard quick;   // the text as `life made` writes it (the quick lore): the decoder's the same
        const Body& mb = madeSys.bodies[madeBody];
        madeWatermarkOf(madeSys, madeBody, loreQuick(madeSys, mb, BodyGen::make(mb), made.people), made, quick);
        const bool same = W.watermark >= 0 && W.watermark < (int)W.shards.size() && W.shards[W.watermark].text == quick.text;
        printf("\nthe decoder: %s's people's records listed %s (row %d of %zu), the watermark %s (record %d of %zu; %s life made's): %s\n", mb.name.c_str(),
               row >= 0 ? "with the watermark's world" : "WITHOUT the watermark's world", row, keys.size(), opened ? "opened" : "NOT OPENED", W.watermark, W.shards.size(),
               same ? "its text" : "its text NOT", g.testShardsInfo().c_str());
        printf("wrote shots/tests/lab_watermark.png\n");
    }
    printf("wrote shots/tests/lab_sequencer.png, shots/tests/lab_tree.png (%.1f s)\n\n", nowSec() - t0);
    return treeKin(12);
}

namespace {
// frames in a grid of `cols`, each its label, under a title
void gridSheet(const std::vector<std::pair<std::vector<uint32_t>, std::string>>& shots, int cols, const std::string& title, const std::string& file) {
    const int gap = 4, head = 14, rows = ((int)shots.size() + cols - 1) / cols;
    Sheet sh(std::min(cols, (int)shots.size()) * (FBW + gap) + gap, rows * (FBH + gap) + gap + head);
    sh.label(4, 3, title);
    for (size_t i = 0; i < shots.size(); i++) {
        const int x = gap + (int)(i % cols) * (FBW + gap), y = head + gap + (int)(i / cols) * (FBH + gap);
        sh.blit(shots[i].first, FBW, FBH, x, y);
        sh.label(x + 2, y + FBH - 11, shots[i].second);
    }
    sh.save(file);
}
}   // namespace

// Z-07: `lab catalogue`: the record. A guide as the lab fills it: the flow's world's four species (one seen every way and named, one moving, one a
// spore woken after its years, one a fossil), home's two and a kin star's two sequenced, a mirrored life read, the made one with its watermark, a
// strain named, the samples counted; a friend's inbox file of their own sheets (a world the explorer never saw, one sequenced and named by them;
// one the explorer saw too; one lent to them) imported. The catalogue by world (its lent rows in cyan, their pixels counted), by kin, a species'
// sheet with its form live, in the electron's way, a lent one's sheet, a species named from it; the statistics' first page, the LIFE page, the
// data sheet's microbe line, a micrograph in the gallery (`lab_catalogue.png`, `lab_life.png`)
int labCatalogue() {
    const double t0 = nowSec();
    TestGame g; Input in;
    g.savePrefix = "shots/tests/lab_catalogue_save"; g.settingsPath = "shots/tests/lab_catalogue_settings.txt"; g.guidePath = "shots/tests/lab_catalogue_guide.txt";
    g.keysPath = "shots/tests/test_keys.txt";
    const std::string inbox = "shots/tests/lab_catalogue_guide_inbox.txt";
    remove(g.guidePath.c_str()); remove(g.settingsPath.c_str()); remove(inbox.c_str());
    g.newGame(); g.setState(GameState::SPACE);
    applyTestScale(g);
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
    auto keyOf = [](const Pick& p) { return Guide::bodyKey(p.sys.star.sx, p.sys.star.sy, p.sys.star.sz, p.body) + "/M" + std::to_string(p.species); };
    // the explorer's: the flow's world's four
    StarSystem fs; int fb = -1;
    std::vector<Pick> flowPicks, readPicks, lentPicks;
    if (firstBodyOfType(PT_FELISIAN, fs, fb)) picksOf(fs, fb, 4, "the flow's world", flowPicks);
    {
        StarSystem hs; hs.generate(home);
        picksOf(hs, worldOfGenesis(hs, [](const Genesis& G) { return G.id == HOME_GENESIS_ID; }), 2, "home's world", readPicks);
    }
    bool kin = false, mirrored = false, own = false;
    for (const Star& s : starsNearHome(8)) {
        if (kin && mirrored && own) break;
        if (s.sx == HOME_SX && s.sy == HOME_SY && s.sz == HOME_SZ) continue;
        StarSystem sys; sys.generate(s);
        if (!kin) { const int w = worldOfGenesis(sys, [](const Genesis& G) { return G.id == HOME_GENESIS_ID; }); if (w >= 0) { picksOf(sys, w, 2, "a kin star's", readPicks); kin = true; continue; } }
        if (!mirrored) { const int w = worldOfGenesis(sys, [](const Genesis& G) { return G.mirrored; }); if (w >= 0) { picksOf(sys, w, 1, "a mirrored life", readPicks); mirrored = true; continue; } }
        if (!own) { const int w = worldOfGenesis(sys, [](const Genesis& G) { return G.origin == GO_OWN && !G.mirrored; }); if (w >= 0) { picksOf(sys, w, 2, "the friend's world", lentPicks); own = true; } }
    }
    StarSystem madeSys; int madeBody = -1; MadeSpecies made;
    for (const Star& s : starsNearHome(16)) {
        StarSystem sys; sys.generate(s);
        for (const Body& b : sys.bodies) {
            if (!kindHas(b.type, KT_PEOPLE)) continue;
            const MadeSpecies m = madeSpeciesOf(sys, b.index);
            if (m.species >= 0) { madeSys = sys; madeBody = b.index; made = m; break; }
        }
        if (madeBody >= 0) break;
    }
    Guide& G = g.guide;
    const uint32_t cellForms = (1u << CF_CELL) | (1u << CF_DIVIDING);
    for (size_t i = 0; i < flowPicks.size(); i++) {
        MicrobeSheet& m = G.microbes[keyOf(flowPicks[i])];
        m.seen = SEEN_FORM; m.forms = cellForms;
        if (i == 0) {   // every part seen, a culture's growth, named
            m.seen |= SEEN_MOTION | SEEN_FLAGELLA | SEEN_DNA | SEEN_WALL; m.forms |= (1u << CF_SPORE) | (1u << CF_COLONY);
            m.stages = LC_GROWING | LC_COLONY | LC_SPORULATING; m.doublingS = 840; m.cell = 1; m.tempC = 31; m.light = 1; m.energy = 1; m.salt = 0;
            G.names[keyOf(flowPicks[i])] = "CORAL ROD";
            G.strains[keyOf(flowPicks[i]) + "/S1"] = StrainRecord{0, 0, 41, ""};
            G.names[keyOf(flowPicks[i]) + "/S1"] = "FAST BLOOM";
        }
        if (i == 1) m.seen |= SEEN_MOTION;
        if (i == 2) { m.forms = (1u << CF_SPORE) | (1u << CF_CELL); m.stages = LC_DORMANT | LC_WAKING | LC_GROWING; m.sleptYears = 2.3e8; }
        if (i == 3) m.forms = 1u << CF_FOSSIL;
    }
    for (const Pick& p : readPicks) G.microbes[keyOf(p)].seen |= SEEN_FORM | SEEN_KIN, G.microbes[keyOf(p)].forms |= cellForms;
    std::string madeKey;
    if (madeBody >= 0) {
        madeKey = Guide::bodyKey(madeSys.star.sx, madeSys.star.sy, madeSys.star.sz, madeBody) + "/M" + std::to_string(made.species);
        G.microbes[madeKey].seen |= SEEN_FORM | SEEN_KIN; G.microbes[madeKey].forms |= cellForms;
        G.watermarks.insert(madeKey);
    }
    G.samplesTaken = 23;
    for (const auto& kv : G.microbes) G.sampled.insert(kv.first.substr(0, kv.first.rfind("/M")));
    G.save(g.guidePath);
    // the friend's file: their own sheets (a world the explorer never saw, the first sequenced and named; one of the explorer's), one lent to them
    int lentWant = 0;
    {
        Guide F;
        for (size_t i = 0; i < lentPicks.size(); i++) {
            MicrobeSheet& m = F.microbes[keyOf(lentPicks[i])];
            m.seen = SEEN_FORM | SEEN_MOTION | (i == 0 ? SEEN_KIN | SEEN_DNA : 0); m.forms = cellForms;
            if (i == 0) F.names[keyOf(lentPicks[i])] = "GLASSWORT";
            lentWant++;
        }
        if (!flowPicks.empty()) F.microbes[keyOf(flowPicks[1 % flowPicks.size()])].seen |= SEEN_FORM | SEEN_DNA;   // the explorer saw it too: theirs wins
        if (!readPicks.empty()) F.lentMicrobes[keyOf(readPicks.back()) + "0"] = MicrobeSheet();   // lent to them: never travels on
        F.save(inbox);
    }
    g.importInboxFile();
    const std::string importLine = g.testStatus();
    const int lentHeld = (int)G.lentMicrobes.size();
    {   // the guide's file: its new lines read back alike, saved again byte for byte
        Guide R; R.load(g.guidePath);
        const std::string woken = flowPicks.size() > 2 ? keyOf(flowPicks[2]) : std::string();
        const bool back = R.lentMicrobes.size() == G.lentMicrobes.size() && R.samplesTaken == G.samplesTaken && R.sampled == G.sampled &&
                          R.microbes.size() == G.microbes.size() && (woken.empty() || R.microbes[woken].sleptYears == 2.3e8);
        R.save("shots/tests/lab_catalogue_guide_again.txt");
        std::ifstream a(g.guidePath), b("shots/tests/lab_catalogue_guide_again.txt");
        std::stringstream sa, sb; sa << a.rdbuf(); sb << b.rdbuf();
        printf("lab catalogue: %zu species of the explorer's (%zu worlds' picks), %d lent by the friend's file of %d of their own (%s); the guide's lines %s, saved again %s\n",
               G.microbes.size(), flowPicks.size() + readPicks.size() + (madeBody >= 0 ? 1 : 0), lentHeld, lentWant + 1, importLine.c_str(), back ? "read back" : "NOT READ BACK",
               sa.str() == sb.str() ? "byte for byte" : "CHANGED");
    }
    std::vector<std::pair<std::vector<uint32_t>, std::string>> shots;
    auto grab = [&](std::vector<std::pair<std::vector<uint32_t>, std::string>>& into, const std::string& label) {
        g.testClearStatus(); g.frame(in, 1.0 / 30); in.newFrame(); g.testClearStatus(); g.frame(in, 1.0 / 30); in.newFrame();
        into.push_back({std::vector<uint32_t>(g.output(), g.output() + (size_t)FBW * FBH), label});
    };
    auto cyanPixels = [&]() { int n = 0; for (size_t i = 0; i < (size_t)FBW * FBH; i++) n += g.output()[i] == HUD_CYAN; return n; };
    // the catalogue by world, a lent row chosen, by kin
    g.openScreen(GameState::GUIDE); g.frame(in, 1.0 / 30); in.newFrame();
    const bool row = selectRow(g, in, "THE CATALOGUE");
    pressKey(g, in, KEY_ENTER);
    CatalogueState& C = g.testCat();
    int lentRows = 0, species = 0, heads = 0;
    for (const CatalogueRow& r : C.rows) { lentRows += !r.head && r.lent; species += !r.head; heads += r.head; }
    grab(shots, "BY WORLD");
    const int cyan = cyanPixels();
    for (int i = 0; i < (int)C.rows.size(); i++) if (!C.rows[i].head && C.rows[i].lent) { while (C.sel < i && C.sel >= 0) { const int was = C.sel; pressKey(g, in, KEY_DOWN); if (C.sel == was) break; } break; }
    grab(shots, "A FRIEND'S, LENT: IN CYAN");
    pressKey(g, in, KEY_TAB);
    int kinHeads = 0; for (const CatalogueRow& r : C.rows) kinHeads += r.head && r.genesis;
    grab(shots, "BY KIN");
    pressKey(g, in, KEY_TAB);
    // a species' sheet: the one seen every way, its form live in the bright field and under the electron
    const std::string full = flowPicks.empty() ? std::string() : keyOf(flowPicks[0]);
    for (int i = 0; i < (int)C.rows.size(); i++) if (C.rows[i].key == full) C.sel = i;
    pressKey(g, in, KEY_ENTER);
    runFrames(g, in, 1.5);
    grab(shots, "ITS SHEET, ITS FORM LIVE");
    for (int k = 0; k < 6 && C.seeing != WS_ELECTRON; k++) pressKey(g, in, KEY_V);
    runFrames(g, in, 1.0);
    grab(shots, "UNDER THE ELECTRON");
    pressKey(g, in, KEY_ESCAPE);
    for (int i = 0; i < (int)C.rows.size(); i++) if (C.rows[i].lent && !C.rows[i].head) { C.sel = i; break; }
    pressKey(g, in, KEY_ENTER);
    runFrames(g, in, 1.0);
    grab(shots, "A LENT ONE'S SHEET");
    pressKey(g, in, KEY_ESCAPE);
    {   // named from the catalogue: the second of the flow's world
        const std::string k = flowPicks.size() > 1 ? keyOf(flowPicks[1]) : std::string();
        for (int i = 0; i < (int)C.rows.size(); i++) if (C.rows[i].key == k) C.sel = i;
        pressKey(g, in, KEY_N); g.testTypeText("SALT DANCER"); pressKey(g, in, KEY_ENTER);
    }
    grab(shots, "NAMED FROM IT");
    const bool named = flowPicks.size() > 1 && G.names.count(keyOf(flowPicks[1])) && G.names[keyOf(flowPicks[1])] == "SALT DANCER" && g.state == GameState::CATALOGUE;
    pressKey(g, in, KEY_ESCAPE);
    gridSheet(shots, 4, fmt("lab catalogue: %d species (%d lent, in cyan), %d worlds, by world and by kin", species, lentRows, heads), "shots/tests/lab_catalogue.png");
    printf("the catalogue (row %s): %d species under %d worlds' heads, %d lent rows (%d cyan pixels on the list), by kin %d geneses' heads; a sheet in %s; named from it %s\n",
           row ? "found" : "NOT FOUND", species, heads, lentRows, cyan, kinHeads, SEEING_NAMES[C.seeing], named ? "yes" : "NO");
    // the statistics' first page and its LIFE page; the data sheet's microbe line; a micrograph in the gallery
    std::vector<std::pair<std::vector<uint32_t>, std::string>> life;
    g.openScreen(GameState::STATS); grab(life, "THE STATISTICS: L LIFE");
    pressKey(g, in, KEY_L); grab(life, "THE LIFE PAGE");
    const bool lifePage = g.testStatsLife();
    pressKey(g, in, KEY_ESCAPE);
    if (fb >= 0) {
        g.testParkAt(fs.star, fb); pressKey(g, in, KEY_ESCAPE); runFrames(g, in, 0.3);
        pressKey(g, in, KEY_I); grab(life, "THE DATA SHEET'S MICROBES");
        pressKey(g, in, KEY_SPACE);
        const std::string gallery = "shots/tests/lab_catalogue_gallery";
        makeDir(gallery);
        for (int i = 0; i < 8; i++) { remove(fmt("%s/screenshot_%d.png", gallery.c_str(), i).c_str()); remove(fmt("%s/screenshot_%d.txt", gallery.c_str(), i).c_str()); }
        g.shotsDir = gallery;
        Vial v;
        if (labVial(fs, fb, v)) {
            g.testColdStore().push_back(v);
            // R-432: Enter the world's vials, Enter its first on the stage (the frame of the one Enter more taken from the wait before E, the
            // one Esc more from the wait after: the frames after them keep their times)
            g.testCabinAtDevice(FACE_MICROSCOPE); runFrames(g, in, 0.167); pressKey(g, in, KEY_E); runFrames(g, in, 0.1);
            pressKey(g, in, KEY_ENTER); pressKey(g, in, KEY_ENTER); runFrames(g, in, 2.0);
            g.testClearStatus(); g.frame(in, 1.0 / 30); in.newFrame();   // a micrograph as the window writes P's: the frame, its caption beside it
            writePNG(fmt("%s/screenshot_0.png", gallery.c_str()).c_str(), g.output(), FBW, FBH);
            { FILE* sc = fopen(fmt("%s/screenshot_0.txt", gallery.c_str()).c_str(), "w"); if (sc) { fprintf(sc, "%s\n", g.screenshotCaption().c_str()); fclose(sc); } }
            g.noteScreenshot();
            pngFlush();
            pressKey(g, in, KEY_ESCAPE); pressKey(g, in, KEY_ESCAPE); pressKey(g, in, KEY_ESCAPE); runFrames(g, in, 0.067);
            g.openScreen(GameState::GALLERY); grab(life, "A MICROGRAPH IN THE GALLERY");
            std::ifstream cap(fmt("%s/screenshot_0.txt", gallery.c_str()));
            std::string caption; std::getline(cap, caption);
            printf("the gallery: a micrograph, its caption '%s'\n", caption.c_str());
        }
    }
    gridSheet(life, 4, "lab catalogue: the statistics, the LIFE page, the data sheet's microbes, a micrograph in the gallery", "shots/tests/lab_life.png");
    printf("the LIFE page %s; the data sheet's line drawn; %zu frames (%.1f s)\n", lifePage ? "opened with L" : "NOT OPENED", life.size(), nowSec() - t0);
    const bool ok = row && lentRows == lentWant && lentHeld == lentWant && cyan > 0 && kinHeads >= 2 && named && lifePage && life.size() == 4;
    return ok ? 0 : 1;
}
