// Y-02: `lab genes` (R-443; docs/ideas/IDEAS-genes.md section 1, life/genes.h): the genes of a world's species for worlds of four, six and eight
// letters and both hands (each genesis's map and code; each species' genes, their working parts and what they give), every gene's edit and cut
// on a species that has it (its values before and after); and the laws the unit's `genes` group holds (`geneLaws`): the genes deterministic,
// every species of forty worlds unedited itself, its parent's letters again nothing, each gene's edit as its code says within its limits, how
// it lives and its plan's family fixed
#include "harness.h"
#include "harness/test_game.h"
#include "life/genes.h"
#include <algorithm>
#include <cmath>
#include <functional>

namespace {
// a species' values in a line, as the laws and the print read them
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
    return t;
}
bool sameValues(const CultureKind& a, const CultureKind& b, const Morph& ma, const Morph& mb) {
    const Microbe &x = a.s, &y = b.s;
    return x.tMin == y.tMin && x.tOpt == y.tOpt && x.tMax == y.tMax && x.saltLo == y.saltLo && x.saltHi == y.saltHi && x.pigment == y.pigment &&
           x.substrate == y.substrate && x.motion == y.motion && x.taxis == y.taxis && x.flees == y.flees && x.cycle == y.cycle && x.glows == y.glows &&
           x.plan == y.plan && x.way == y.way && a.bodyUm == b.bodyUm && a.dead == b.dead && a.quorum == b.quorum && ma.lengthUm == mb.lengthUm &&
           ma.widthUm == mb.widthUm && ma.aspect == mb.aspect && ma.speed == mb.speed && ma.pigment == mb.pigment && ma.plan == mb.plan && ma.motion == mb.motion;
}
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
    int worlds = 0, species = 0, genes = 0, alike = 0, itself = 0, again = 0, segs = 0, kinds = 0, minG = 99, maxG = 0;
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
                const CultureKind k0 = cultureKindOf(B, m.index, 2 * forms[m.index].bodyUm());
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
                bool sameSegs = ra.seg[0].size() == rb.seg[0].size();
                for (size_t i = 0; sameSegs && i < ra.seg[0].size(); i++)
                    sameSegs = ra.seg[0][i].t0 == rb.seg[0][i].t0 && ra.seg[0][i].mode == rb.seg[0][i].mode && ra.seg[0][i].N == rb.seg[0][i].N && ra.seg[0][i].mu == rb.seg[0][i].mu &&
                               ra.seg[0][i].K == rb.seg[0][i].K && ra.seg[0][i].stage == rb.seg[0][i].stage;
                segs += sameSegs;
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
            const CultureKind k0 = cultureKindOf(B, m.index, 2 * w.forms[m.index].bodyUm());
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
                        default: break;
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
    return R;
}

int labGenes() {
    GeneWorld W[6];
    geneWorlds(W);
    printf("lab genes: a world's species' genes for each alphabet and hand (the first living world of each in the scan region)\n");
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
            const CultureKind k = cultureKindOf(B, m.index, 2 * w.forms[m.index].bodyUm());
            const MicrobeGenes g = microbeGenesOf(m, w.forms[m.index], B.genesis);
            printf("  M%d: %s\n", m.index, valuesOf(k, w.forms[m.index]).c_str());
            for (const MicrobeGene& G : g.genes)
                printf("    %2d %-12s %-12s %5d letters  %s\n", G.place + 1, GENE_KIND_NAMES[G.kind], G.part.c_str(), G.length, geneValueWords(k, w.forms[m.index], G, c).c_str());
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
            const CultureKind k0 = cultureKindOf(B, best, 2 * W[0].forms[best].bodyUm());
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
    // the editor's screen in a game: its chooser, a rod of the four letters' world on it (its warmth and its shape known), a known gene's letters
    // changed, its line writing, the line in its flask (`lab_genes.png`)
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
            if (G.kind == GN_WARMTH || G.kind == GN_SHAPE || G.kind == GN_VITAL) { GeneKnown k; k.kind = G.kind; k.how = GENE_SEEN_EDIT; g.guide.genes[g.testGeneKey(g0.genesis.id, G.place)] = k; }
            if (G.kind == GN_QUIET) { GeneKnown k; k.how = GENE_SEEN_NOTHING; g.guide.genes[g.testGeneKey(g0.genesis.id, G.place)] = k; }
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
        if (Wm) {
            E.sel = Wm->place;
            pressKey(g, in, KEY_ENTER); pressKey(g, in, KEY_RIGHT); pressKey(g, in, KEY_RIGHT); pressKey(g, in, KEY_UP); pressKey(g, in, KEY_UP);
            grab("ITS WARMTH'S LETTERS CHANGED");
            pressKey(g, in, KEY_ENTER);
        }
        pressKey(g, in, KEY_W);
        g.testLabSkip(45);
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
        remove(g.guidePath.c_str()); remove(g.settingsPath.c_str());
    }
    int fails = 0;
    printf("\nthe laws:\n");
    const GeneLaws R = geneLaws();
    const std::pair<const char*, const GeneLaw*> laws[] = {{"a living world of each alphabet and hand", &R.worlds}, {"deterministic", &R.deterministic},
                                                           {"every species unedited is itself", &R.itself}, {"each gene's edit and cut as its code says", &R.asCode},
                                                           {"a vital gene's edit kills, a quiet one shows nothing", &R.vitalQuiet},
                                                           {"how it lives and its plan's family never move; its values within their limits", &R.fixed}};
    for (const auto& l : laws) { printf("  %s %s: %s\n", l.second->ok ? "ok  " : "FAIL", l.first, l.second->detail.c_str()); fails += !l.second->ok; }
    return fails ? 1 : 0;
}
