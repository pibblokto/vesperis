// Y-02: `lab genes` (R-443; docs/archive/ideas/IDEAS-genes.md section 1, life/genes.h): the genes of a world's species for worlds of four, six and eight
// letters and both hands (each genesis's map and code; each species' genes, their working parts and what they give), every gene's edit and cut
// on a species that has it (its values before and after); and the laws the unit's `genes` group holds (`geneLaws`): the genes deterministic,
// every species of forty worlds unedited itself, its parent's letters again nothing, each gene's edit as its code says within its limits, how
// it lives and its plan's family fixed. Y-03: genes moved in from a world's kin (their values, their price), the editor beside a kin
// (`lab_genes_kin.png`), the law of a gene moved in. Y-04: a glow and a rest switched (`lab_genes_switch.png`), the law of the own switches.
// J-01: the world genes (places 19 to 32): forty species' wild values, each one's edit and cut (`worldWords`), their laws (the cell's genes as
// before by a digest of generation 17's, every species carrying all fourteen read from what it is, each edit as its code says, the price)
#include "harness.h"
#include "harness/test_game.h"
#include "life/genes.h"
#include <algorithm>
#include <cmath>
#include <functional>

namespace {
std::string worldWords(const CultureWorld& w);
// a species' values in a line, as the laws and the print read them (J-01: and its world values)
std::string valuesOf(const CultureKind& k, const Morph& m) {
    const Microbe& s = k.s;
    std::string t = fmt("%.0f-%.0f C (best %.0f), salt %d-%d, %.2f um, %s, %s", s.tMin, s.tMax, s.tOpt, s.saltLo, s.saltHi, k.bodyUm, MICROBE_PLAN_NAMES[s.plan],
                        WAY_NAMES[s.way]);
    if (s.way == LW_CHEMICAL) t += std::string(" on ") + SUBSTRATE_NAMES[s.substrate];
    if (s.pigment) t += std::string(", ") + PIGMENT_NAMES[s.pigment];
    if (s.motion) t += fmt(", %s %.1f um/s", MOTION_NAMES[s.motion], m.speed);
    if (s.taxis) { t += ", to"; for (int b = 0; b < TX_BITS; b++) if (s.taxis & (1u << b)) t += std::string(" ") + TAXIS_NAMES[b]; }
    if (fleesLight(s)) t += ", flees the light";
    if (s.cycle & (LC_SPORULATING | LC_DORMANT | LC_GATHERING)) t += ", rests";
    if (s.glows) t += fmt(", glows at %.1f%%", 100 * k.quorum);
    if (k.dead) t += ", DOES NOT LIVE";
    const std::string w = worldWords(k.world);
    if (!w.empty()) t += "; " + w;
    return t;
}
bool liveInNiche(const Microbe& s, int n) { return s.niche == n || (s.niches & nicheBit(n)); }
// J-01: a line's world values in a line (what works only: none, "")
std::string worldWords(const CultureWorld& w) {
    std::string t;
    auto add = [&](const std::string& x) { t += (t.empty() ? "" : ", ") + x; };
    if (w.gives()) add(fmt("gas %s %.2f", WORLD_GAS_NAMES[w.gas], w.gasRate));
    if (w.dyes()) add(fmt("dye %s %.2f", WORLD_DYE_NAMES[w.dye], w.dyeRate));
    if (w.lift > 0) add(fmt("lift %.2f", w.lift));
    if (w.frostC > FROST_NONE + 1) add(fmt("frost %.1f C", w.frostC));
    if (w.slime > 0) add(fmt("slime %.2f", w.slime));
    if (w.shells()) add(fmt("shell %s %.2f", WORLD_SHELL_NAMES[w.shell], w.shellRate));
    if (w.overgrows) add("never stops");
    else if (w.stopShift != 0) add(fmt("stops %+.2f decades", w.stopShift));
    if (w.bind > 0) add(fmt("bind %.2f", w.bind));
    if (w.rots()) add(fmt("rots %s %.2f", WORLD_ROT_NAMES[w.rot], w.rotRate));
    if (w.poison != PZ_NONE) add(fmt("poison %s %.2f", WORLD_POISON_NAMES[w.poison], w.poisonRate));
    if (w.carrier > 0) add(fmt("carrier %.2f", w.carrier));
    if (w.hosted()) add(fmt("host %s %.2f", WORLD_HOST_NAMES[w.host], w.hostRate));
    if (w.dry > 0) add(fmt("dry %.2f", w.dry));
    if (w.shield > 0) add(fmt("shield %.2f", w.shield));
    return t;
}
bool sameWorld(const CultureWorld& a, const CultureWorld& b) {
    return a.gas == b.gas && a.gasRate == b.gasRate && a.dye == b.dye && a.dyeRate == b.dyeRate && a.lift == b.lift && a.frostC == b.frostC && a.slime == b.slime &&
           a.shell == b.shell && a.shellRate == b.shellRate && a.stopShift == b.stopShift && a.overgrows == b.overgrows && a.bind == b.bind && a.rot == b.rot &&
           a.rotRate == b.rotRate && a.poison == b.poison && a.poisonRate == b.poisonRate && a.carrier == b.carrier && a.host == b.host && a.hostRate == b.hostRate &&
           a.dry == b.dry && a.shield == b.shield;
}
bool sameValues(const CultureKind& a, const CultureKind& b, const Morph& ma, const Morph& mb) {
    const Microbe &x = a.s, &y = b.s;
    return x.tMin == y.tMin && x.tOpt == y.tOpt && x.tMax == y.tMax && x.saltLo == y.saltLo && x.saltHi == y.saltHi && x.pigment == y.pigment &&
           x.substrate == y.substrate && x.motion == y.motion && x.taxis == y.taxis && x.flees == y.flees && x.cycle == y.cycle && x.glows == y.glows &&
           x.plan == y.plan && x.way == y.way && a.bodyUm == b.bodyUm && a.dead == b.dead && a.quorum == b.quorum && ma.lengthUm == mb.lengthUm &&
           ma.widthUm == mb.widthUm && ma.aspect == mb.aspect && ma.speed == mb.speed && ma.pigment == mb.pigment && ma.plan == mb.plan && ma.motion == mb.motion &&
           sameWorld(a.world, b.world) && ma.gas == mb.gas && ma.capsule == mb.capsule && ma.arrangement == mb.arrangement;   // (J-01: its world's too)
}
bool shapePlanOf(int p) { return p == MP_SPHERE || p == MP_ROD || p == MP_CURVED || p == MP_SPIRAL; }   // the four a shape gene moves among
std::string mapOf(const GeneCode& c) {
    std::string s;
    for (int p = 0; p < GENE_PLACES; p++) s += fmt("%s%d %s", p ? ", " : "", p + 1, GENE_KIND_NAMES[c.kind[p]]);
    return s;
}
// a gene's working part changed by one: a weight's first letter that can be one heavier, a table's run to the next value it reads
std::string oneStep(const GeneCode& c, const MicrobeGene& G) {
    std::string p = G.part;
    const char* abc = alphabetOf(c.letters);
    const int L = c.letters;
    for (int i = 0; i < (int)p.size(); i++) {
        const int at = (int)(std::strchr(abc, p[i]) - abc);
        for (int d = 1; d < L; d++) {
            std::string q = p;
            q[i] = abc[(at + d) % L];
            if (geneWeight(c, q) == geneWeight(c, p) + 1 || (G.kind == GN_PIGMENT || G.kind == GN_FOOD || G.kind == GN_SENSE)) return q;
        }
    }
    p[0] = abc[(std::strchr(abc, p[0]) - abc + 1) % L];
    return p;
}
}   // namespace

// J-01: the cell's genes as generation 17's genes drew them (places 1 to 18): over forty living worlds, each genesis's map of its first eighteen
// places, its letters' weights and its old tables, each species' genes there (their letters, lengths and switches), and each one's edit by one
// and cut (its values after, every cell value to the last bit); its edits' words written and read again. One hash, the same as before J-01
uint64_t cellGenesDigest(int& species, int& edits) {
    uint64_t h = 0xCE11D16E57ULL;
    auto eat = [&](const std::string& s) { h = mix64(h ^ fnv1a(s)); };
    auto vals = [](const CultureKind& k, const Morph& m) {
        const Microbe& s = k.s;
        return fmt("%.17g %.17g %.17g %d %d %.17g %d %d %d %d %d %.17g %u %d %u %d %.17g %d %.17g %.17g %.17g %d %d %.17g %d", s.tMin, s.tOpt, s.tMax, s.saltLo, s.saltHi,
                   k.bodyUm, s.plan, s.way, s.substrate, s.pigment, s.motion, m.speed, s.taxis, s.flees, s.cycle, s.glows ? 1 : 0, k.quorum, k.dead ? 1 : 0,
                   m.lengthUm, m.widthUm, m.aspect, m.pigment, m.plan, s.sporeYears, (int)k.switches.size());
    };
    species = 0; edits = 0;
    int worlds = 0;
    for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160, [&] { return worlds < 40; })) {
        auto L = lifeOf(sys);
        for (int b = 0; b < (int)sys.bodies.size() && worlds < 40; b++) {
            const Biota& B = L->bodies[b];
            if (!livingStage(B.stage) || B.genesis.id == 0) continue;
            worlds++;
            const GeneCode c = geneCodeOf(B.genesis);
            std::string code = fmt("%d", c.letters);
            for (int p = 0; p < 18; p++) code += fmt(" %d.%d", c.kind[p], c.slot[p]);
            for (int i = 0; i < c.letters; i++) code += fmt(" w%d", c.weight[i]);
            for (const std::vector<int>* t : {&c.pigment, &c.food, &c.motion, &c.sense, &c.shape}) { code += " |"; for (int v : *t) code += fmt(" %d", v); }
            eat(code);
            const std::vector<Morph> forms = morphsOf(sys, b);
            for (const Microbe& m : B.species) {
                if (m.state == SS_FOSSIL || m.state == SS_PROTO) continue;
                species++;
                const MicrobeGenes g0 = microbeGenesOf(m, forms[m.index], B.genesis);
                std::string gs = fmt("%d %u %.17g", g0.valid ? 1 : 0, g0.restCycle, g0.sporeYears);
                for (const MicrobeGene& G : g0.genes) if (G.place < 18) gs += fmt(" %d.%d.%d.%d.%s.%d", G.place, G.kind, G.slot, G.length, G.part.c_str(), G.sw);
                eat(gs);
                const CultureKind k0 = cultureKindOf(B, m.index, 2 * forms[m.index].bodyUm());
                for (const MicrobeGene& G : g0.genes) {
                    if (G.place >= 18) continue;
                    for (int cut = 0; cut < 2; cut++) {
                        GeneEdit e; e.place = G.place; e.cut = cut == 1; e.part = cut ? G.part : oneStep(c, G);
                        const std::string text = geneEditsText({e});
                        CultureKind k = k0; Morph f = forms[m.index]; MicrobeGenes g = g0;
                        applyEdits(k, f, g, geneEditsOf(text));
                        eat(text + " " + vals(k, f));
                        edits++;
                    }
                }
            }
        }
    }
    return h;
}

// J-01: a world gene's edit or cut as its code says: its table's choice its run's, its amount its weight's (none cut; STOP cut never stopping,
// else its crowd moved by twice its weight's share changed, in -1.5 to 1 decades); a lift's vesicles, a slime's coat and a binding's cells in its
// form; every cell value and every other world value as they were
bool worldEditOk(const GeneCode& c, const MicrobeGene& G, const GeneEdit& e, const CultureKind& k0, const CultureKind& k, const Morph& f0, const Morph& f) {
    const std::vector<int>* t = worldTable(c, G.kind);
    const double a = e.cut ? 0 : geneAmount(c, e.part, t ? 2 : 0, 4);
    const int ch = e.cut || !t ? 0 : geneTable(c, *t, e.part, 0, 2);
    CultureWorld want = k0.world;
    switch (G.kind) {
        case GN_GAS: want.gas = ch; want.gasRate = a; break;
        case GN_DYE: want.dye = ch; want.dyeRate = a; break;
        case GN_LIFT: want.lift = a; break;
        case GN_FROST: want.frostC = a > 0 ? -12 + 10 * a : FROST_NONE; break;
        case GN_SLIME: want.slime = a; break;
        case GN_SHELL: want.shell = ch; want.shellRate = a; break;
        case GN_STOP:
            want.overgrows = e.cut;
            if (!e.cut) want.stopShift = clampd(k0.world.stopShift + 2.0 * (geneWeight(c, e.part) - geneWeight(c, G.part)) / (4 * (c.letters - 1)), -1.5, 1.0);
            break;
        case GN_BIND: want.bind = a; break;
        case GN_ROT: want.rot = ch; want.rotRate = a; break;
        case GN_POISON: want.poison = ch; want.poisonRate = a; break;
        case GN_CARRIER: want.carrier = a; break;
        case GN_HOST: want.host = ch; want.hostRate = a; break;
        case GN_DRY: want.dry = a; break;
        case GN_SHIELD: want.shield = a; break;
        default: break;
    }
    bool ok = sameWorld(k.world, want) && a >= 0 && a <= 1 && k.world.stopShift >= -1.5 && k.world.stopShift <= 1.0;
    if (G.kind == GN_LIFT) ok = ok && f.gas == 0.9 * want.lift;
    else ok = ok && f.gas == f0.gas;
    if (G.kind == GN_SLIME) ok = ok && f.capsule == 1.4 * want.slime;
    else ok = ok && f.capsule == f0.capsule;
    if (G.kind != GN_BIND) ok = ok && f.arrangement == f0.arrangement && f.groupN == f0.groupN;
    else if (smallCellPlan(k0.s.plan) && k0.s.plan != MP_FILAMENT) ok = ok && (want.bind > 0) == (f.arrangement != AR_SINGLE);
    else ok = ok && f.arrangement == f0.arrangement;
    CultureKind kc = k; kc.world = k0.world;
    Morph fc = f; fc.gas = f0.gas; fc.capsule = f0.capsule; fc.arrangement = f0.arrangement; fc.groupN = f0.groupN;
    return ok && sameValues(kc, k0, fc, f0);
}

// the first living world found in the scan region of each alphabet and hand (four letters ours: home's genesis's kin)
bool geneWorlds(GeneWorld (&out)[6]) {
    const int letters[6] = {4, 4, 6, 6, 8, 8};
    for (int i = 0; i < 6; i++) { out[i] = GeneWorld(); out[i].letters = letters[i]; out[i].mirrored = (i % 2) == 1; }
    auto all = [&] { for (const GeneWorld& w : out) if (w.body < 0) return false; return true; };
    for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160, [&] { return !all(); })) {
        auto L = lifeOf(sys);
        for (int b = 0; b < (int)sys.bodies.size(); b++) {
            const Biota& B = L->bodies[b];
            if (!livingStage(B.stage) || B.species.size() < 4 || B.genesis.id == 0) continue;
            for (GeneWorld& w : out)
                if (w.body < 0 && B.genesis.letters == w.letters && B.genesis.mirrored == w.mirrored) { w.sys = sys; w.body = b; w.forms = morphsOf(sys, b); }
        }
    }
    return all();
}

// the laws, over forty living worlds of the scan region and the six alphabets' and hands'
GeneLaws geneLaws() {
    GeneLaws R;
    GeneWorld W[6];
    const bool six = geneWorlds(W);
    R.worlds = {six, six ? "4, 6 and 8 letters, ours and mirrored" : "one missing"};
    // forty worlds: every species' genes the same drawn twice; unedited (no edits, its own letters again) itself
    int worlds = 0, species = 0, genes = 0, alike = 0, itself = 0, again = 0, segs = 0, kinds = 0, minG = 99, maxG = 0, wild = 0;
    // J-01: every species carrying the fourteen world genes, each once at places 19 to 32, its wild values read from what it is
    int carried = 0, read = 0, works[GN_COUNT] = {}, gases[WG_COUNT] = {};
    std::string wmiss;
    for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160, [&] { return worlds < 40; })) {
        auto L = lifeOf(sys);
        for (int b = 0; b < (int)sys.bodies.size() && worlds < 40; b++) {
            const Biota& B = L->bodies[b];
            if (!livingStage(B.stage) || B.genesis.id == 0) continue;
            worlds++;
            const std::vector<Morph> forms = morphsOf(sys, b);
            for (const Microbe& m : B.species) {
                if (m.state == SS_FOSSIL || m.state == SS_PROTO) continue;
                species++;
                const MicrobeGenes g1 = microbeGenesOf(m, forms[m.index], B.genesis), g2 = microbeGenesOf(m, forms[m.index], B.genesis);
                bool same = g1.genes.size() == g2.genes.size();
                for (size_t i = 0; same && i < g1.genes.size(); i++) same = g1.genes[i].part == g2.genes[i].part && g1.genes[i].place == g2.genes[i].place;
                alike += same;
                genes += (int)g1.genes.size(); minG = std::min(minG, (int)g1.genes.size()); maxG = std::max(maxG, (int)g1.genes.size());
                {   // J-01
                    int seenKinds[GN_COUNT] = {}, inPlace = 0;
                    for (const MicrobeGene& G : g1.genes) if (worldGene(G.kind)) { seenKinds[G.kind]++; inPlace += G.place >= CELL_GENE_PLACES && G.place < GENE_PLACES; }
                    bool all = inPlace == WORLD_GENES;
                    for (int kd = GN_GAS; kd < GN_COUNT; kd++) all = all && seenKinds[kd] == 1;
                    carried += all;
                    CultureWorld w;
                    worldOf(w, g1);
                    const Morph& f = forms[m.index];
                    const bool cold = liveInNiche(m, NI_SNOW) || liveInNiche(m, NI_ICE) || liveInNiche(m, NI_CLOUD);
                    const bool ok = w.stopShift == 0 && !w.overgrows && !w.poisons() && (w.lift > 0) == (f.gas > 0) && (w.slime > 0) == (f.capsule > 0) &&
                                    (w.frostC <= FROST_NONE + 1 || (cold && B.chemistry == CH_WATER)) && (w.carrier > 0) == ((m.cycle & LC_EXCHANGING) != 0) &&
                                    !w.hosted();
                    read += ok;
                    if (!ok && wmiss.size() < 300) wmiss += fmt(" [%s M%d: %s]", sys.bodies[b].name.c_str(), m.index, worldWords(w).c_str());
                    if (w.gives()) works[GN_GAS]++, gases[w.gas]++;
                    if (w.dyes()) works[GN_DYE]++;
                    if (w.lift > 0) works[GN_LIFT]++;
                    if (w.frostC > FROST_NONE + 1) works[GN_FROST]++;
                    if (w.slime > 0) works[GN_SLIME]++;
                    if (w.shells()) works[GN_SHELL]++;
                    if (w.bind > 0) works[GN_BIND]++;
                    if (w.rots()) works[GN_ROT]++;
                    if (w.poison != PZ_NONE) works[GN_POISON]++;
                    if (w.carrier > 0) works[GN_CARRIER]++;
                    if (w.dry > 0) works[GN_DRY]++;
                    if (w.shield >= UV_SHIELD) works[GN_SHIELD]++;
                }
                CultureKind k0 = cultureKindOf(B, m.index, 2 * forms[m.index].bodyUm());
                worldOf(k0.world, g1);   // J-01: its world genes' values, as a flask reads them
                CultureKind k = k0; Morph f = forms[m.index]; MicrobeGenes g = g1;
                applyEdits(k, f, g, {});
                itself += sameValues(k, k0, f, forms[m.index]);
                std::vector<GeneEdit> own;
                for (const MicrobeGene& G : g1.genes) { GeneEdit e; e.place = G.place; e.part = G.part; own.push_back(e); }
                k = k0; f = forms[m.index]; g = g1;
                applyEdits(k, f, g, own);
                again += sameValues(k, k0, f, forms[m.index]);
                // its culture's segments and its slide's kinds, the line's against its species'
                FlaskCulture ca, cb;
                ca.kind[0] = k0; cb.kind[0] = k;
                CultureEvent e; e.kind = CD_START; e.m.cell = chemistryCell(k0.chemistry); e.m.tempC = std::round(k0.s.tOpt); e.m.salt = k0.s.saltLo;
                e.m.energy = k0.s.way == LW_CHEMICAL ? k0.s.substrate : CE_BROTH; e.m.light = k0.s.way == LW_LIGHT ? CL_WHITE : CL_DARK;
                ca.events.push_back(e); cb.events.push_back(e);
                const CultureRun ra = runCulture(ca), rb = runCulture(cb);
                auto sameRun = [](const CultureRun& x, const CultureRun& y) {
                    bool same = x.seg[0].size() == y.seg[0].size();
                    for (size_t i = 0; same && i < x.seg[0].size(); i++)
                        same = x.seg[0][i].t0 == y.seg[0][i].t0 && x.seg[0][i].mode == y.seg[0][i].mode && x.seg[0][i].N == y.seg[0][i].N && x.seg[0][i].mu == y.seg[0][i].mu &&
                               x.seg[0][i].K == y.seg[0][i].K && x.seg[0][i].stage == y.seg[0][i].stage && x.seg[0][i].state == y.seg[0][i].state &&
                               x.seg[0][i].quorum == y.seg[0][i].quorum && x.seg[0][i].sigma == y.seg[0][i].sigma;
                    return same;
                };
                const bool sameSegs = sameRun(ra, rb);
                segs += sameSegs;
                {   // Y-04: every gene switched to its own again (a rest's ALWAYS too, working as its own): itself, nothing listed for its culture, its
                    // segments the same in its best medium and lit the other way
                    std::vector<GeneEdit> sw;
                    for (const MicrobeGene& G : g1.genes) { GeneEdit x; x.place = G.place; x.sw = G.kind == GN_REST ? GW_ALWAYS : geneOwnSwitch(G.kind); sw.push_back(x); }
                    CultureKind kw = k0; Morph fw = forms[m.index]; MicrobeGenes gw = g1;
                    applyEdits(kw, fw, gw, sw);
                    FlaskCulture cw = ca; cw.kind[0] = kw;
                    FlaskCulture cl = ca, cwl = cw;
                    cl.events[0].m.light = cwl.events[0].m.light = ca.events[0].m.light == CL_DARK ? CL_WHITE : CL_DARK;
                    wild += sameValues(kw, k0, fw, forms[m.index]) && kw.switches.empty() && !kw.glowAny && sameRun(ra, runCulture(cw)) && sameRun(runCulture(cl), runCulture(cwl));
                }
                Biota Bb = B; Bb.species[m.index] = k.s;
                std::vector<Morph> fb = forms; fb[m.index] = f;
                Microhabitat h; h.niche = m.niche; h.meanC = clampd(B.meanC, m.tMin + 1, m.tMax - 1); h.salt = m.saltLo; h.place = mix64(sys.bodies[b].seed ^ (uint64_t)(m.index + 7));
                const Sample S = sampleOf(B, h);
                const SlideMount ma = mountOf(B, forms, S, h), mb = mountOf(Bb, fb, S, h);
                bool sameKinds = ma.kinds.size() == mb.kinds.size() && ma.cells.size() == mb.cells.size();
                for (size_t i = 0; sameKinds && i < ma.kinds.size(); i++)
                    sameKinds = ma.kinds[i].taxis == mb.kinds[i].taxis && ma.kinds[i].speed == mb.kinds[i].speed && ma.kinds[i].flees == mb.kinds[i].flees &&
                                ma.kinds[i].lives == mb.kinds[i].lives && ma.kinds[i].tile == mb.kinds[i].tile && ma.kinds[i].n == mb.kinds[i].n && ma.kinds[i].reach == mb.kinds[i].reach;
                kinds += sameKinds;
            }
        }
    }
    R.deterministic = {alike == species && species > 0, fmt("%d species of %d worlds drawn twice the same; %d genes, %d to %d a species", species, worlds, genes, minG, maxG)};
    R.itself = {itself == species && again == species && segs == species && kinds == species,
                fmt("%d of %d with no edits, %d with its own letters written again; its culture's segments %d, its slide's kinds %d the same", itself, species, again, segs, kinds)};
    R.wild = {wild == species && species > 0, fmt("%d of %d species with every gene switched to its own (a glow's WHEN CROWDED, a rest's WHEN STARVING or ALWAYS, "
                                                  "the rest ALWAYS) themselves: their values, their segments lit and dark", wild, species)};
    {   // J-01: the cell's genes as generation 17's genes drew them, by the digest the code before J-01 gave (c9e98be)
        int n = 0, e = 0;
        const uint64_t d = cellGenesDigest(n, e);
        // (B-504, J-16: a hot world of a dead star or a brown dwarf stays the kind it was drawn as, so one of the first forty living worlds is
        // another: e58ae38013b383cd over 349 species and 7702 edits with the old chemotroph, as before J-01)
        R.cellAsBefore = {d == 0x2f19f43bd44b6acbULL && n == 347 && e == 7626, fmt("digest %016llx over %d species and %d edits (B-504's worlds: 2f19f43bd44b6acb, 347, 7626)",
                                                                                     (unsigned long long)d, n, e)};
    }
    std::string shares = fmt("gas %d (", works[GN_GAS]);
    for (int i = 1; i < WG_COUNT; i++) if (gases[i]) shares += fmt("%s%s %d", shares.back() == '(' ? "" : ", ", WORLD_GAS_NAMES[i], gases[i]);
    shares += ")";
    for (int kd : {GN_DYE, GN_LIFT, GN_FROST, GN_SLIME, GN_SHELL, GN_BIND, GN_ROT, GN_POISON, GN_CARRIER, GN_DRY, GN_SHIELD})
        shares += fmt(", %s %d", GENE_KIND_NAMES[kd], works[kd]);
    R.worldCarried = {carried == species && read == species && species > 0,
                      fmt("%d of %d carry the fourteen at 19 to 32, %d read as they are (none overgrowing, poisoning or hosted; lift, slime, frost and carrying "
                          "their form's and niche's); working: %s%s", carried, species, read, shares.c_str(), wmiss.c_str())};
    // each gene's edit and cut, on the six worlds' species: its value moved as its code says, within its limits; how it lives and its plan's
    // family never move
    int edits = 0, asCode = 0, within = 0, fixedWay = 0, killed = 0, vitals = 0, quiet = 0, quietSame = 0;
    std::string miss;
    for (const GeneWorld& w : W) {
        if (w.body < 0) continue;
        const Biota& B = lifeOf(w.sys)->bodies[w.body];
        const GeneCode c = geneCodeOf(B.genesis);
        double lo, hi;
        cellRange(chemistryCell(B.chemistry), lo, hi);
        for (const Microbe& m : B.species) {
            if (m.state != SS_LIVING) continue;
            const MicrobeGenes g0 = microbeGenesOf(m, w.forms[m.index], B.genesis);
            CultureKind k0 = cultureKindOf(B, m.index, 2 * w.forms[m.index].bodyUm());
            worldOf(k0.world, g0);
            for (const MicrobeGene& G : g0.genes)
                for (int cut = 0; cut < 2; cut++) {
                    GeneEdit e; e.place = G.place; e.cut = cut == 1; e.part = cut ? G.part : oneStep(c, G);
                    CultureKind k = k0; Morph f = w.forms[m.index]; MicrobeGenes g = g0;
                    applyEdits(k, f, g, {e});
                    edits++;
                    fixedWay += k.s.way == k0.s.way && smallCellPlan(k.s.plan) == smallCellPlan(k0.s.plan) && largeCellPlan(k.s.plan) == largeCellPlan(k0.s.plan) &&
                                colonyPlan(k.s.plan) == colonyPlan(k0.s.plan);
                    within += k.s.tMin >= std::min(lo, k0.s.tMin) - 1e-9 && k.s.tMax <= std::max(hi, k0.s.tMax) + 1e-9 && k.s.tMin < k.s.tOpt && k.s.tOpt < k.s.tMax &&
                              k.s.saltLo >= 0 && k.s.saltHi <= 2 && k.s.saltLo <= k.s.saltHi;
                    bool ok = true;
                    const double step = std::max(0.5, std::round(2 * (hi - lo) / (12.0 * (c.letters - 1))) / 2);
                    switch (G.kind) {
                        case GN_VITAL: ok = k.dead; vitals++; killed += k.dead; break;
                        case GN_QUIET: ok = sameValues(k, k0, f, w.forms[m.index]); quiet++; quietSame += ok; break;
                        case GN_WARMTH:
                            if (cut) ok = k.s.tMax <= k0.s.tOpt + 2 + 1e-9 && k.s.tMin >= k0.s.tOpt - 4 - 1e-9;
                            else ok = std::fabs(k.s.tOpt - k0.s.tOpt - step) < 1e-9 || k.s.tMax >= std::max(hi, k0.s.tMax) - 1e-9;   // a step, or held at its cell's top
                            break;
                        case GN_SIZE: {   // by two to the third of its weight's change (half cut), held in its family's widths
                            const double want = cut ? 0.5 : std::pow(2.0, (geneWeight(c, e.part) - geneWeight(c, G.part)) / 3.0);
                            const double w0 = w.forms[m.index].widthUm, lo = smallCellPlan(m.plan) ? 0.1 : 2, hi = smallCellPlan(m.plan) ? 16 : (largeCellPlan(m.plan) ? 2500 : 3000);
                            const double held = clampd(want, std::min(1.0, lo / w0), std::max(1.0, hi / w0));
                            ok = std::fabs(k.bodyUm / k0.bodyUm - held) < 1e-6 && std::fabs(f.widthUm / w0 - held) < 1e-6;
                            break;
                        }
                        case GN_PIGMENT: ok = k.s.pigment == (cut ? PG_NONE : geneTable(c, c.pigment, e.part, 0, 2)) && f.pigment == k.s.pigment; break;
                        case GN_FOOD: ok = k.s.substrate == (cut ? SU_NONE : geneTable(c, c.food, e.part, 0, 3)); break;
                        case GN_MOTION: ok = cut ? k.s.motion == MO_NONE : true; break;
                        case GN_SENSE: ok = !cut || (k.s.taxis & ~k0.s.taxis) == 0; break;
                        case GN_REST: ok = !cut || !(k.s.cycle & (LC_SPORULATING | LC_DORMANT | LC_GATHERING)); break;
                        case GN_GLOW: ok = !cut || !k.s.glows; break;
                        case GN_SHAPE: ok = !cut || k.s.plan == MP_SPHERE; break;
                        default:
                            if (worldGene(G.kind)) ok = worldEditOk(c, G, e, k0, k, w.forms[m.index], f);   // J-01
                            break;
                    }
                    asCode += ok;
                    if (!ok && miss.size() < 300) miss += fmt(" [%s M%d gene %d %s%s: %s]", w.sys.bodies[w.body].name.c_str(), m.index, G.place + 1, GENE_KIND_NAMES[G.kind], cut ? " cut" : "",
                                                                valuesOf(k, f).c_str());
                }
        }
    }
    R.asCode = {asCode == edits && edits > 0, fmt("%d of %d%s", asCode, edits, miss.c_str())};
    R.vitalQuiet = {killed == vitals && quietSame == quiet && vitals > 0 && quiet > 0, fmt("%d of %d vital edits dead, %d of %d quiet the same", killed, vitals, quietSame, quiet)};
    R.fixed = {fixedWay == edits && within == edits, fmt("%d of %d its way and family, %d within its cell's warmth and the salts", fixedWay, edits, within)};
    // Y-03: each gene of every other living species of its world moved into each (its donor read): its letters and its value its donor's, over
    // its own gene or into a gap; a shape only in the four small cells' plans; how it lives and its plan's family never move, within its limits
    int moves = 0, movedOk = 0, movedFixed = 0, gaps = 0;
    std::string mmiss;
    for (const GeneWorld& w : W) {
        if (w.body < 0) continue;
        const Biota& B = lifeOf(w.sys)->bodies[w.body];
        const GeneCode c = geneCodeOf(B.genesis);
        double lo, hi;
        cellRange(chemistryCell(B.chemistry), lo, hi);
        for (const Microbe& a : B.species) {
            if (a.state != SS_LIVING) continue;
            const MicrobeGenes ga = microbeGenesOf(a, w.forms[a.index], B.genesis);
            const CultureKind ka = cultureKindOf(B, a.index, 2 * w.forms[a.index].bodyUm());
            const Morph& fa = w.forms[a.index];
            for (const Microbe& d : B.species) {
                if (d.state != SS_LIVING || d.index == a.index) continue;
                const MicrobeGenes gd = microbeGenesOf(d, w.forms[d.index], B.genesis);
                const CultureKind kd = cultureKindOf(B, d.index, 2 * w.forms[d.index].bodyUm());
                const Morph& fd = w.forms[d.index];
                for (const MicrobeGene& D : gd.genes) {
                    if (D.kind == GN_VITAL || D.kind == GN_QUIET) continue;
                    GeneEdit e; e.place = D.place; e.how = GX_MOVED; e.part = D.part; e.donor = "D"; e.read = true;
                    e.donorKind = kd; e.donorForm = fd; e.donorGene = D; e.far = GFROM_WORLD; e.origin = "D";
                    CultureKind k = ka; Morph f = fa; MicrobeGenes g = ga;
                    gaps += ga.at(D.place) == nullptr;
                    applyEdits(k, f, g, {e});
                    moves++;
                    const MicrobeGene* G = g.at(D.place);
                    bool ok = G && !G->cut && G->part == D.part && G->from == GFROM_WORLD;
                    const Microbe &x = k.s, &y = kd.s;
                    switch (D.kind) {
                        case GN_WARMTH: ok = ok && x.tMin == y.tMin && x.tOpt == y.tOpt && x.tMax == y.tMax; break;
                        case GN_SALT: ok = ok && x.saltLo == y.saltLo && x.saltHi == y.saltHi; break;
                        case GN_SIZE: {   // its donor's width, held in its family's
                            const double flo = smallCellPlan(x.plan) ? 0.1 : 2, fhi = smallCellPlan(x.plan) ? 16 : (largeCellPlan(x.plan) ? 2500 : 3000);
                            const double want = clampd(fd.widthUm, std::min(fa.widthUm, flo), std::max(fa.widthUm, fhi));
                            ok = ok && std::fabs(f.widthUm - want) <= 1e-9 * want;
                            break;
                        }
                        case GN_PIGMENT: ok = ok && x.pigment == y.pigment && f.pigment == y.pigment; break;
                        case GN_FOOD: ok = ok && x.substrate == y.substrate; break;
                        case GN_MOTION: {   // its kind among its own plan's by its donor's run, its donor's share of its kind's pace
                            int kinds[4];
                            const int nk = planMotions(x.plan, kinds), run = geneTable(c, c.motion, D.part, 0, 2);
                            const int want = run == 0 || nk == 0 ? MO_NONE : kinds[(run - 1) % nk];
                            ok = ok && x.motion == want;
                            if (ok && want != MO_NONE) {
                                const double b0 = kindPace(y.motion, fd), share = b0 > 0 && fd.speed > 0 ? fd.speed / b0 : 1, b = kindPace(want, f);
                                ok = std::fabs(f.speed - clampd(b * share, 0.2 * b, 4 * b)) <= 1e-9 * std::max(1.0, f.speed);
                            }
                            break;
                        }
                        case GN_SENSE: {
                            const int t = geneTable(c, c.sense, D.part, 0, 2);
                            const uint32_t bit = t == SE_LIGHT ? TX_LIGHT : (t == SE_FOOD ? TX_FOOD : (t == SE_FIELD ? TX_FIELD : (t == SE_WARMTH ? TX_WARMTH : 0u)));
                            ok = ok && (bit == 0 || (x.taxis & bit)) && (t != SE_DARK || x.flees == 1 || (x.taxis & (TX_LIGHT | TX_FIELD)));
                            break;
                        }
                        case GN_REST: ok = ok && (x.cycle & (LC_DORMANT | LC_SPORULATING | LC_GATHERING)) != 0; break;
                        case GN_GLOW: ok = ok && x.glows && k.quorum == kd.quorum; break;
                        case GN_SHAPE:
                            if (shapePlanOf(ka.s.plan)) ok = ok && x.plan == y.plan && (y.plan == MP_SPHERE || std::fabs(f.aspect - clampd(fd.aspect, 1.2, 30)) <= 1e-9 * fd.aspect);
                            else ok = ok && x.plan == ka.s.plan;   // a shape in another form: nothing
                            break;
                        default: break;
                    }
                    movedOk += ok;
                    movedFixed += x.way == ka.s.way && smallCellPlan(x.plan) == smallCellPlan(ka.s.plan) && largeCellPlan(x.plan) == largeCellPlan(ka.s.plan) &&
                                  colonyPlan(x.plan) == colonyPlan(ka.s.plan) && x.tMin >= std::min({lo, ka.s.tMin, y.tMin}) - 1e-9 &&
                                  x.tMax <= std::max({hi, ka.s.tMax, y.tMax}) + 1e-9 && x.tMin < x.tOpt && x.tOpt < x.tMax && x.saltLo >= 0 && x.saltHi <= 2 && x.saltLo <= x.saltHi;
                    if (!ok && mmiss.size() < 300) mmiss += fmt(" [%s M%d gene %d %s from M%d: %s]", w.sys.bodies[w.body].name.c_str(), a.index, D.place + 1, GENE_KIND_NAMES[D.kind], d.index,
                                                                valuesOf(k, f).c_str());
                }
            }
        }
    }
    R.moved = {movedOk == moves && movedFixed == moves && moves > 0 && gaps > 0,
               fmt("%d of %d moved in as their donors' (%d into a gap), %d of %d its way, family and limits%s", movedOk, moves, gaps, movedFixed, moves, mmiss.c_str())};
    // J-01: a product a species does not make (a gas, a dye, a slime, a shell, a poison), written in at its heaviest, costs its line 2 to 6% of its
    // growth; its own costs nothing
    int own = 0, ownFree = 0, made = 0, priced = 0;
    std::string pmiss;
    for (const GeneWorld& w : W) {
        if (w.body < 0) continue;
        const Biota& B = lifeOf(w.sys)->bodies[w.body];
        const GeneCode c = geneCodeOf(B.genesis);
        const char* abc = alphabetOf(c.letters);
        int heavy = 0;
        for (int i = 0; i < c.letters; i++) if (c.weight[i] == c.letters - 1) heavy = i;
        for (const Microbe& m : B.species) {
            if (m.state != SS_LIVING) continue;
            const MicrobeGenes g0 = microbeGenesOf(m, w.forms[m.index], B.genesis);
            CultureKind k0 = cultureKindOf(B, m.index, 2 * w.forms[m.index].bodyUm());
            worldOf(k0.world, g0);
            const Morph& f0 = w.forms[m.index];
            for (const MicrobeGene& G : g0.genes) {
                if (G.kind != GN_GAS && G.kind != GN_DYE && G.kind != GN_SLIME && G.kind != GN_SHELL && G.kind != GN_POISON) continue;
                own++; ownFree += genePrice(k0, f0, k0, f0, G) == 0;
                const std::vector<int>* t = worldTable(c, G.kind);
                const int wildChoice = t ? geneTable(c, *t, G.part, 0, 2) : 0;
                std::string part;
                if (t) {   // a run reading another value than its own, then its heaviest weight
                    int run = -1;
                    for (int r = 0; r < (int)t->size() && run < 0; r++) if ((*t)[r] != 0 && (*t)[r] != wildChoice) run = r;
                    if (run < 0) continue;
                    part += abc[run % c.letters]; part += abc[(run / c.letters) % c.letters];
                } else if (k0.world.slime > 0) continue;   // (a slime of its own: not a new product)
                part += std::string(4, abc[heavy]);
                GeneEdit e; e.place = G.place; e.part = part;
                CultureKind k = k0; Morph f = f0; MicrobeGenes g = g0;
                applyEdits(k, f, g, {e});
                const double p = genePrice(k0, f0, k, f, *g.at(G.place));
                made++; priced += p >= 0.02 - 1e-9 && p <= 0.06 + 1e-9;
                if (!(p >= 0.02 - 1e-9 && p <= 0.06 + 1e-9) && pmiss.size() < 200) pmiss += fmt(" [M%d gene %d %s: %.3f]", m.index, G.place + 1, GENE_KIND_NAMES[G.kind], p);
            }
        }
    }
    R.worldPrice = {own > 0 && ownFree == own && made > 0 && priced == made,
                    fmt("%d of %d of their own free; %d of %d new products at their heaviest priced 2 to 6%%%s", ownFree, own, priced, made, pmiss.c_str())};
    return R;
}

int labGenes() {
    GeneWorld W[6];
    geneWorlds(W);
    printf("lab genes: a world's species' genes for each alphabet and hand (the first living world of each in the scan region)\n");
    {   // J-01: the cell's genes as before (their digest: places 1 to 18, the old tables, every edit of them)
        int n = 0, e = 0;
        const uint64_t d = cellGenesDigest(n, e);
        printf("the cell's genes (places 1 to 18): digest %016llx over %d species and %d edits\n", (unsigned long long)d, n, e);
    }
    for (const GeneWorld& w : W) {
        if (w.body < 0) { printf("\n%d letters, %s: none in the scan region\n", w.letters, w.mirrored ? "mirrored" : "ours"); continue; }
        const Biota& B = lifeOf(w.sys)->bodies[w.body];
        const GeneCode c = geneCodeOf(B.genesis);
        const char* abc = alphabetOf(c.letters);
        std::string weights;
        for (int i = 0; i < c.letters; i++) weights += fmt("%s%c %d", i ? ", " : "", abc[i], c.weight[i]);
        printf("\n%s (%s), its genesis %016llx: %d letters, %s, %s\n  its map: %s\n  its letters' weights: %s\n", w.sys.bodies[w.body].name.c_str(),
               upper(shortType(w.sys.bodies[w.body].type)).c_str(), (unsigned long long)B.genesis.id, c.letters, w.mirrored ? "mirrored" : "ours in hand",
               CHEMISTRY_NAMES[B.chemistry], mapOf(c).c_str(), weights.c_str());
        for (const Microbe& m : B.species) {
            if (m.state == SS_FOSSIL || m.state == SS_PROTO) continue;
            CultureKind k = cultureKindOf(B, m.index, 2 * w.forms[m.index].bodyUm());
            const MicrobeGenes g = microbeGenesOf(m, w.forms[m.index], B.genesis);
            worldOf(k.world, g);   // J-01
            printf("  M%d: %s\n", m.index, valuesOf(k, w.forms[m.index]).c_str());
            for (const MicrobeGene& G : g.genes)
                printf("    %2d %-12s %-12s %5d letters  %s\n", G.place + 1, GENE_KIND_NAMES[G.kind], G.part.c_str(), G.length, geneValueWords(k, w.forms[m.index], G, c).c_str());
        }
    }
    // J-01: forty species' wild world values (the scan region's first living worlds, a species of each way and form in turn)
    {
        printf("\nforty species' world values as they are (their genes 19 to 32):\n");
        int n = 0;
        for (auto& [x, z, sv, sys] : scanSystems(150, 340, 20, 160, [&] { return n < 40; })) {
            auto L = lifeOf(sys);
            for (int b = 0; b < (int)sys.bodies.size() && n < 40; b++) {
                const Biota& B = L->bodies[b];
                if (!livingStage(B.stage) || B.genesis.id == 0) continue;
                const std::vector<Morph> forms = morphsOf(sys, b);
                for (const Microbe& m : B.species) {
                    if (m.state != SS_LIVING || n >= 40) continue;
                    CultureWorld w;
                    worldOf(w, microbeGenesOf(m, forms[m.index], B.genesis));
                    printf("  %-22s M%-2d %-9s %-8s %-6s %s\n", sys.bodies[b].name.c_str(), m.index, MICROBE_PLAN_NAMES[m.plan], WAY_NAMES[m.way], NICHE_NAMES[m.niche],
                           worldWords(w).c_str());
                    n++;
                }
            }
        }
    }
    // an edit's values before and after: every gene of the four letters' species with the most genes, a letter one heavier (a table's run its
    // next) and the gene cut
    if (W[0].body >= 0) {
        const Biota& B = lifeOf(W[0].sys)->bodies[W[0].body];
        const GeneCode c = geneCodeOf(B.genesis);
        int best = -1; size_t most = 0;
        for (const Microbe& m : B.species) {
            if (m.state != SS_LIVING) continue;
            const size_t n = microbeGenesOf(m, W[0].forms[m.index], B.genesis).genes.size();
            if (n > most) { most = n; best = m.index; }
        }
        if (best >= 0) {
            const Microbe& m = B.species[best];
            const MicrobeGenes g0 = microbeGenesOf(m, W[0].forms[best], B.genesis);
            CultureKind k0 = cultureKindOf(B, best, 2 * W[0].forms[best].bodyUm());
            worldOf(k0.world, g0);   // J-01
            printf("\nedits of %s M%d (%zu genes): %s\n", W[0].sys.bodies[W[0].body].name.c_str(), best, g0.genes.size(), valuesOf(k0, W[0].forms[best]).c_str());
            for (const MicrobeGene& G : g0.genes)
                for (int cut = 0; cut < 2; cut++) {
                    GeneEdit e; e.place = G.place; e.cut = cut == 1; e.part = cut ? G.part : oneStep(c, G);
                    CultureKind k = k0; Morph f = W[0].forms[best]; MicrobeGenes g = g0;
                    applyEdits(k, f, g, {e});
                    printf("  gene %2d %-11s %-12s -> %-12s %s\n", G.place + 1, GENE_KIND_NAMES[G.kind], G.part.c_str(), cut ? "CUT" : e.part.c_str(), valuesOf(k, f).c_str());
                }
        }
    }
    // Y-03: genes moved in: into the four letters' species with the most genes, each of its world's other species' genes its own differs in (its
    // value after, its price), and a gene's price by how far it came (its best doubling at each)
    if (W[0].body >= 0) {
        const Biota& B = lifeOf(W[0].sys)->bodies[W[0].body];
        const GeneCode c = geneCodeOf(B.genesis);
        int best = -1; size_t most = 0;
        for (const Microbe& m : B.species) {
            if (m.state != SS_LIVING) continue;
            const size_t n = microbeGenesOf(m, W[0].forms[m.index], B.genesis).genes.size();
            if (n > most) { most = n; best = m.index; }
        }
        if (best >= 0) {
            const MicrobeGenes ga = microbeGenesOf(B.species[best], W[0].forms[best], B.genesis);
            CultureKind ka = cultureKindOf(B, best, 2 * W[0].forms[best].bodyUm());
            worldOf(ka.world, ga);   // J-01
            const Morph& fa = W[0].forms[best];
            printf("\ngenes moved into %s M%d from its world's kin (its letters -> its kin's; its value after; its price):\n", W[0].sys.bodies[W[0].body].name.c_str(), best);
            for (const Microbe& d : B.species) {
                if (d.state != SS_LIVING || d.index == best) continue;
                const MicrobeGenes gd = microbeGenesOf(d, W[0].forms[d.index], B.genesis);
                CultureKind kd = cultureKindOf(B, d.index, 2 * W[0].forms[d.index].bodyUm());
                worldOf(kd.world, gd);
                for (const MicrobeGene& D : gd.genes) {
                    const MicrobeGene* H = ga.at(D.place);
                    if (D.kind == GN_VITAL || D.kind == GN_QUIET || (H && H->part == D.part)) continue;
                    GeneEdit e; e.place = D.place; e.how = GX_MOVED; e.part = D.part; e.donor = "M"; e.read = true;
                    e.donorKind = kd; e.donorForm = W[0].forms[d.index]; e.donorGene = D; e.far = GFROM_WORLD; e.origin = "M";
                    CultureKind k = ka; Morph f = fa; MicrobeGenes g = ga;
                    applyEdits(k, f, g, {e});
                    k.burden = geneBurden(ka, fa, k, f, g);
                    printf("  gene %2d %-11s from M%d: %-12s -> %-12s %s; %.0f%%\n", D.place + 1, GENE_KIND_NAMES[D.kind], d.index, H ? H->part.c_str() : "(a gap)", D.part.c_str(),
                           geneValueWords(k, f, *g.at(D.place), c).c_str(), 100 * k.burden);
                }
            }
            printf("a gene not its own, by how far it came: its best doubling %.0f s at no price", bestDoublingS(ka));
            const char* far[GFROM_COUNT] = {"", "its world's", "its system's", "another star's"};
            for (int i = GFROM_WORLD; i < GFROM_COUNT; i++) { CultureKind k = ka; k.burden = GENE_PRICE[i]; printf(", %s %.0f%% %.0f s", far[i], 100 * GENE_PRICE[i], bestDoublingS(k)); }
            printf("\n");
        }
    }
    // the editor's screen in a game: its chooser, a rod of the four letters' world on it (its warmth and its shape known), a known gene's letters
    // changed, its line writing, the line in its flask (`lab_genes.png`); Y-03, a kin beside it (`lab_genes_kin.png`): its chooser, the map beside
    // its kin, its kin's warmth moved in, a gene into a gap, the line on its world's tree from its kin's leaf
    if (W[0].body >= 0) {
        TestGame g; Input in;
        g.savePrefix = "shots/tests/lab_genes_save"; g.settingsPath = "shots/tests/lab_genes_settings.txt"; g.guidePath = "shots/tests/lab_genes_guide.txt"; g.keysPath = "shots/tests/test_keys.txt";
        remove(g.guidePath.c_str()); remove(g.settingsPath.c_str());
        g.newGame(); g.setState(GameState::SPACE);
        applyTestScale(g);
        const Biota& B = lifeOf(W[0].sys)->bodies[W[0].body];
        FlaskSpecies X; X.sx = W[0].sys.star.sx; X.sy = W[0].sys.star.sy; X.sz = W[0].sys.star.sz; X.body = W[0].body;
        for (const Microbe& m : B.species) if (X.species < 0 && m.state == SS_LIVING && m.plan == MP_ROD) X.species = m.index;
        if (X.species < 0) for (const Microbe& m : B.species) if (X.species < 0 && m.state == SS_LIVING) X.species = m.index;
        const std::string key = Guide::bodyKey(X.sx, X.sy, X.sz, X.body) + "/M" + std::to_string(X.species);
        g.guide.microbes[key].seen |= SEEN_FORM | SEEN_KIN;
        CultureKind k0; Morph f0; MicrobeGenes g0;
        g.testFlaskKind(X, k0, f0, &g0);
        Medium m; m.cell = chemistryCell(k0.chemistry); m.tempC = std::round(k0.s.tOpt); m.salt = k0.s.saltLo;
        m.light = k0.s.way == LW_LIGHT ? CL_WHITE : CL_DARK; m.energy = k0.s.way == LW_CHEMICAL ? k0.s.substrate : (k0.s.way == LW_LIGHT ? CE_NONE : CE_BROTH);
        Flask& F = g.testLab().flasks[0];
        F = Flask(); F.n = 1; F.who[0] = X; F.medium = F.asWorld = m;
        CultureEvent e; e.kind = CD_START; e.t = 0; e.m = m; F.events.push_back(e);
        g.testLabSkip(200);
        const GeneCode c = geneCodeOf(g0.genesis);
        const MicrobeGene* Wm = nullptr;
        for (const MicrobeGene& G : g0.genes) {
            if (G.kind == GN_WARMTH) Wm = &G;
            if (G.kind == GN_WARMTH || G.kind == GN_SHAPE || G.kind == GN_VITAL || G.kind == GN_QUIET) { GeneKnown k; k.kind = G.kind; g.guide.genes[g.testGeneKey(X, G.place)] = k; }
        }
        std::vector<std::pair<std::vector<uint32_t>, std::string>> shots;
        auto grab = [&](const std::string& label) {
            g.testClearStatus(); g.frame(in, 0); in.newFrame(); g.testClearStatus(); g.frame(in, 0); in.newFrame();
            shots.push_back({std::vector<uint32_t>(g.output(), g.output() + (size_t)FBW * FBH), label});
        };
        g.openScreen(GameState::EDITOR);
        grab("EMPTY");
        pressKey(g, in, KEY_ENTER);
        pressKey(g, in, KEY_ENTER);
        grab("ITS CHOOSER: THE FLASKS");
        pressKey(g, in, KEY_ENTER);
        grab("A ROD ON IT: ITS MAP");
        EditorState& E = g.testEditor();
        {   // J-01: Page Down, the world genes' page: its gas gene known, chosen
            for (const MicrobeGene& G : g0.genes)
                if (G.kind == GN_GAS || G.kind == GN_STOP) { GeneKnown k; k.kind = G.kind; g.guide.genes[g.testGeneKey(X, G.place)] = k; }
            pressKey(g, in, KEY_PAGE_DOWN);
            for (const MicrobeGene& G : g0.genes) if (G.kind == GN_GAS) E.sel = G.place;
            grab("PAGE DOWN: ITS WORLD GENES, ITS GAS");
            pressKey(g, in, KEY_ENTER); pressKey(g, in, KEY_UP);   // R-450: its letters open, the first changed: the guide to its pick
            grab("ITS GAS'S GUIDE, ITS FIRST LETTER");
            pressKey(g, in, KEY_BACKSPACE); pressKey(g, in, KEY_ENTER);   // (its parent's letter again, the map)
            pressKey(g, in, KEY_PAGE_UP);
        }
        if (Wm) {
            E.sel = Wm->place;
            pressKey(g, in, KEY_ENTER); pressKey(g, in, KEY_RIGHT); pressKey(g, in, KEY_RIGHT); pressKey(g, in, KEY_UP); pressKey(g, in, KEY_UP);
            grab("ITS WARMTH'S LETTERS CHANGED");
            pressKey(g, in, KEY_ENTER);
        }
        pressKey(g, in, KEY_W);
        grab("W: WHICH FOLDER (R-445)");
        pressKey(g, in, KEY_ENTER);
        grab("THEN THE NAME, PARENT'S TO START");
        pressKey(g, in, KEY_ENTER);
        g.testLabSkip(EDIT_S / 2);
        grab("WRITING ITS LINE");
        g.testLabSkip(EDIT_S);
        g.closeScreen(); g.openScreen(GameState::INCUBATOR);
        g.testIncub().sel = 1;
        grab("THE LINE IN FLASK 2");
        g.closeScreen();
        const int gap = 4, head = 14;
        Sheet sh((int)shots.size() * (FBW + gap) + gap, FBH + 2 * gap + head);
        sh.label(4, 3, fmt("lab genes: the gene editor, %s M%d of %s on it", B.species[X.species].plan == MP_ROD ? "a rod" : "a cell", X.species, W[0].sys.bodies[W[0].body].name.c_str()));
        for (size_t i = 0; i < shots.size(); i++) { sh.blit(shots[i].first, FBW, FBH, gap + (int)i * (FBW + gap), head + gap); sh.label(gap + (int)i * (FBW + gap) + 2, head + gap + FBH - 11, shots[i].second); }
        sh.save("shots/tests/lab_genes.png");
        printf("\nthe editor: %s; its line %s in flask 2 (%s)\nwrote shots/tests/lab_genes.png\n", E.on ? "a culture on it" : "EMPTY",
               g.testLab().flasks[1].n ? g.testLineTag(g.testLab().flasks[1].who[0]).c_str() : "NONE", g.testLab().flasks[1].n ? g.testLab().flasks[1].label[0].c_str() : "");
        // Y-03: a kin of its world beside it (its warmth other, a gene it lacks), moved in, written, on its world's tree
        FlaskSpecies K; int gapAt = -1;
        const MicrobeGene* wx = nullptr;
        for (const MicrobeGene& G : g0.genes) if (G.kind == GN_WARMTH) wx = &G;
        for (const Microbe& m : B.species) {
            if (K.species >= 0 || m.state != SS_LIVING || m.index == X.species) continue;
            const MicrobeGenes gk = microbeGenesOf(m, W[0].forms[m.index], B.genesis);
            const MicrobeGene* wk = nullptr;
            for (const MicrobeGene& G : gk.genes) if (G.kind == GN_WARMTH) wk = &G;
            int p = -1;
            for (const MicrobeGene& G : gk.genes) if (p < 0 && !g0.at(G.place) && G.kind != GN_QUIET) p = G.place;
            if (wx && wk && wk->part != wx->part && p >= 0) { K = X; K.species = m.index; gapAt = p; }
        }
        shots.clear();
        if (K.species >= 0) {
            const std::string kkey = Guide::bodyKey(K.sx, K.sy, K.sz, K.body) + "/M" + std::to_string(K.species);
            g.guide.microbes[kkey].seen |= SEEN_FORM | SEEN_KIN;
            g.testEditorPut(X, m, true);
            g.openScreen(GameState::EDITOR);
            pressKey(g, in, KEY_K);
            grab("K: A KIN BESIDE IT");
            g.testEditor().choosing = false;
            g.testEditorKin(K);
            g.testEditor().sel = wx->place;
            grab("ITS KIN BESIDE IT, ITS WARMTH");
            pressKey(g, in, KEY_M);
            grab("M: ITS KIN'S WARMTH MOVED IN");
            g.testEditor().sel = gapAt;
            pressKey(g, in, KEY_M);
            grab(fmt("A GENE INTO A GAP (%d)", gapAt + 1));
            pressKey(g, in, KEY_W); pressKey(g, in, KEY_ENTER); pressKey(g, in, KEY_ENTER);   // R-445: its folder, its name
            g.testLabSkip(EDIT_S + 1);
            g.closeScreen(); g.openScreen(GameState::TREE);
            g.testTreeShow(key);
            for (size_t i = 0; i < g.testTree().shown.size(); i++) if (g.testTree().shown[i].key == key + "/S2") g.testTree().sel = (int)i;
            grab("ITS WORLD'S TREE: THE KIN'S GENE IN ITS LINE");
            g.closeScreen();
            Sheet sk((int)shots.size() * (FBW + gap) + gap, FBH + 2 * gap + head);
            sk.label(4, 3, fmt("lab genes: genes from others, M%d of %s beside M%d", K.species, W[0].sys.bodies[W[0].body].name.c_str(), X.species));
            for (size_t i = 0; i < shots.size(); i++) { sk.blit(shots[i].first, FBW, FBH, gap + (int)i * (FBW + gap), head + gap); sk.label(gap + (int)i * (FBW + gap) + 2, head + gap + FBH - 11, shots[i].second); }
            sk.save("shots/tests/lab_genes_kin.png");
            const auto rec = g.guide.strains.find(key + "/S2");
            printf("its kin M%d beside it: its line %s (%s), %zu donor's lines on its world's tree\n", K.species,
                   rec != g.guide.strains.end() ? "E2" : "NONE", rec != g.guide.strains.end() ? rec->second.edits.c_str() : "", g.testTree().donors.size());
        }
        remove(g.guidePath.c_str()); remove(g.settingsPath.c_str());
    }
    // Y-04: the switches (`lab_genes_switch.png`): a luminous species on the editor, its glow switched by S to IN THE DARK (the map's ~, its switch's
    // line); its line written into a dark flask, glowing from its first cells, its species beside it not yet; a spore former's rest switched IN THE
    // LIGHT, at its end resting lit and dying in the dark
    {
        FlaskSpecies Lw, Rw; int lg = -1, rp = -1;
        CultureKind Lk, Rk;
        for (auto& [x, z, sv, sys] : scanSystems(150, 340, 20, 160, [&] { return lg < 0 || rp < 0; })) {
            auto L = lifeOf(sys);
            for (int b = 0; b < (int)sys.bodies.size(); b++) {
                const Biota& B = L->bodies[b];
                if (!livingStage(B.stage) || B.genesis.id == 0) continue;
                const std::vector<Morph> forms = morphsOf(sys, b);
                for (const Microbe& m : B.species) {
                    if (m.state != SS_LIVING) continue;
                    const CultureKind k = cultureKindOf(B, m.index, 2 * forms[m.index].bodyUm());
                    Medium best; best.cell = chemistryCell(k.chemistry); best.tempC = std::round(m.tOpt); best.salt = m.saltLo;
                    best.light = m.way == LW_LIGHT ? CL_WHITE : CL_DARK; best.energy = m.way == LW_CHEMICAL ? m.substrate : (m.way == LW_LIGHT ? CE_NONE : CE_BROTH);
                    if (growthIn(k, best).fit != GF_GROWS) continue;
                    const MicrobeGenes ge = microbeGenesOf(m, forms[m.index], B.genesis);
                    FlaskSpecies w; w.sx = sys.star.sx; w.sy = sys.star.sy; w.sz = sys.star.sz; w.body = b; w.species = m.index;
                    for (const MicrobeGene& G : ge.genes) {
                        if (lg < 0 && G.kind == GN_GLOW && m.way != LW_LIGHT) { Lw = w; Lk = k; lg = G.place; }
                        if (rp < 0 && G.kind == GN_REST && (m.cycle & LC_SPORULATING) && !(m.cycle & LC_GATHERING) && m.way != LW_LIGHT) { Rw = w; Rk = k; rp = G.place; }
                    }
                }
            }
        }
        if (lg >= 0 && rp >= 0) {
            TestGame g; Input in;
            g.savePrefix = "shots/tests/lab_genes_save"; g.settingsPath = "shots/tests/lab_genes_settings.txt"; g.guidePath = "shots/tests/lab_genes_guide.txt"; g.keysPath = "shots/tests/test_keys.txt";
            remove(g.guidePath.c_str()); remove(g.settingsPath.c_str());
            g.newGame(); g.setState(GameState::SPACE);
            applyTestScale(g);
            const std::string lkey = Guide::bodyKey(Lw.sx, Lw.sy, Lw.sz, Lw.body) + "/M" + std::to_string(Lw.species);
            const std::string rkey = Guide::bodyKey(Rw.sx, Rw.sy, Rw.sz, Rw.body) + "/M" + std::to_string(Rw.species);
            g.guide.microbes[lkey].seen |= SEEN_FORM | SEEN_KIN;
            CultureKind k0; Morph f0; MicrobeGenes g0;
            g.testFlaskKind(Lw, k0, f0, &g0);
            { GeneKnown k; k.kind = GN_GLOW; g.guide.genes[g.testGeneKey(Lw, lg)] = k; }
            auto bestIn = [](const CultureKind& k, int light) {
                Medium m; m.cell = chemistryCell(k.chemistry); m.tempC = std::round(k.s.tOpt); m.salt = k.s.saltLo; m.light = light;
                m.energy = k.s.way == LW_CHEMICAL ? k.s.substrate : CE_BROTH;
                return m;
            };
            auto flaskIn = [&](int i, const FlaskSpecies& w, const Medium& m) {
                Flask& F = g.testLab().flasks[i];
                F = Flask(); F.n = 1; F.who[0] = w; F.who[0].strainIn = w.strain; F.medium = F.asWorld = m;
                CultureEvent e; e.kind = CD_START; e.t = g.testLab().clock; e.m = m; F.events.push_back(e);
                g.testLabSkip(0);
            };
            std::vector<std::pair<std::vector<uint32_t>, std::string>> shots;
            auto grab = [&](const std::string& label) {
                g.testClearStatus(); g.frame(in, 0); in.newFrame(); g.testClearStatus(); g.frame(in, 0); in.newFrame();
                shots.push_back({std::vector<uint32_t>(g.output(), g.output() + (size_t)FBW * FBH), label});
            };
            flaskIn(0, Lw, bestIn(Lk, CL_DARK));
            g.testEditorPut(Lw, bestIn(Lk, CL_DARK), true);
            g.openScreen(GameState::EDITOR);
            g.testEditor().sel = lg;
            for (int i = 0; i < 6; i++) pressKey(g, in, KEY_S);   // WHEN CROWDED (its own), WHEN STARVING, WHEN COLD, ALWAYS, NEVER, IN THE LIGHT: IN THE DARK
            grab("S: ITS GLOW IN THE DARK");
            const std::string edits = geneEditsText(g.testEditor().edits);
            pressKey(g, in, KEY_W); pressKey(g, in, KEY_ENTER); pressKey(g, in, KEY_ENTER);   // R-445: its folder, its name
            g.testLabSkip(EDIT_S + 1);
            g.closeScreen();
            flaskIn(2, Lw, bestIn(Lk, CL_DARK));   // its species beside it, in the dark too
            g.testLabSkip(48);   // (R-449: a fifth of the four minutes it was, the flask five times as quick)
            g.openScreen(GameState::INCUBATOR);
            g.testIncub().sel = 1;
            grab("ITS LINE IN THE DARK: GLOWING");
            const bool lineGlows = g.testFlaskNow(1).one[0].glowing, wildGlows = g.testFlaskNow(2).one[0].glowing;
            g.testIncub().sel = 2;
            grab("ITS SPECIES THERE: NOT YET");
            g.closeScreen();
            StrainRecord r; r.what = SM_EDIT; r.edits = fmt("%d~LIGHT", rp + 1);
            g.guide.strains[rkey + "/S1"] = r;
            FlaskSpecies Rl = Rw; Rl.strain = Rl.strainIn = 1;
            Medium starvedLit = bestIn(Rk, CL_WHITE), starvedDark = bestIn(Rk, CL_DARK);   // R-449: a full flask is fed for good, so nothing it eats
            starvedLit.energy = starvedDark.energy = CE_NONE;
            flaskIn(3, Rl, starvedLit);
            flaskIn(4, Rl, starvedDark);
            g.testLabSkip(50 * 60);
            g.openScreen(GameState::INCUBATOR);
            g.testIncub().sel = 3;
            grab("A REST IN THE LIGHT, LIT, STARVED: SPORES");
            g.testIncub().sel = 4;
            grab("THE SAME IN THE DARK: DYING");
            const CultureOne lit = g.testFlaskNow(3).one[0], unlit = g.testFlaskNow(4).one[0];
            g.closeScreen();
            const int gap = 4, head = 14;
            Sheet sw((int)shots.size() * (FBW + gap) + gap, FBH + 2 * gap + head);
            sw.label(4, 3, fmt("lab genes: the switches, M%d's glow IN THE DARK and M%d's rest IN THE LIGHT", Lw.species, Rw.species));
            for (size_t i = 0; i < shots.size(); i++) { sw.blit(shots[i].first, FBW, FBH, gap + (int)i * (FBW + gap), head + gap); sw.label(gap + (int)i * (FBW + gap) + 2, head + gap + FBH - 11, shots[i].second); }
            sw.save("shots/tests/lab_genes_switch.png");
            printf("the switches: M%d's glow (gene %d) by S %s, written: its line in the dark %s at 48 s, its species %s; M%d's rest (gene %d) IN THE LIGHT, starved, at fifty "
                   "minutes: lit %s (%.1e spores), dark %s (%.0f spores)\n", Lw.species, lg + 1, edits.c_str(), lineGlows ? "glowing" : "DARK",
                   wildGlows ? "GLOWING" : "not yet", Rw.species, rp + 1, CULTURE_STAGE_NAMES[lit.stage], lit.spores, CULTURE_STAGE_NAMES[unlit.stage], unlit.spores);
            remove(g.guidePath.c_str()); remove(g.settingsPath.c_str());
        } else printf("the switches: NO LUMINOUS ONE OR SPORE FORMER\n");
    }
    int fails = 0;
    printf("\nthe laws:\n");
    const GeneLaws R = geneLaws();
    const std::pair<const char*, const GeneLaw*> laws[] = {{"a living world of each alphabet and hand", &R.worlds}, {"deterministic", &R.deterministic},
                                                           {"every species unedited is itself", &R.itself}, {"each gene's edit and cut as its code says", &R.asCode},
                                                           {"a vital gene's edit kills, a quiet one shows nothing", &R.vitalQuiet},
                                                           {"how it lives and its plan's family never move; its values within their limits", &R.fixed},
                                                           {"a gene moved in takes its donor's letters and value", &R.moved},
                                                           {"the own switches give every species' curve as before", &R.wild},
                                                           {"the cell's genes (1 to 18) as before J-01", &R.cellAsBefore},
                                                           {"every species carries the fourteen world genes, read from what it is", &R.worldCarried},
                                                           {"a world product not its own costs its line, its own nothing", &R.worldPrice}};
    for (const auto& l : laws) { printf("  %s %s: %s\n", l.second->ok ? "ok  " : "FAIL", l.first, l.second->detail.c_str()); fails += !l.second->ok; }
    return fails ? 1 : 0;
}
