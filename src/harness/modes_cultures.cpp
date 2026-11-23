// Z-05: the incubator's cultures (`lab cultures`): twenty species of the scan region, each of a kind of life (a way to live, a stage of its own
// cycle, a chemistry, a salt, a warmth), grown an hour in four media (as its world, the lab's standard broth, its best, its hot edge): each curve
// by five minutes, the times its stages began, its doubling and any strain; then pairs of them in a flask (one wins, they cycle, they feed each
// other, mirrored they ignore each other), the cycles' periods, the winners; Y-04, lines switched (a glow IN THE DARK, a rest IN THE LIGHT) lit and
// dark beside their species. J-01: the world genes' looks, a line each changed once (`labWorldLooks`): the incubator's screen of each
// (`lab_cultures_world.png`)
#include "harness/harness.h"
#include "life/cultures.h"
#include "life/genes.h"
#include "life/morphs.h"
#include <set>

namespace {
enum CultureWant { CW_LIGHT, CW_BROTH, CW_HYDROGEN, CW_SULPHIDE, CW_IRON, CW_METHANE, CW_GRAZER, CW_PREDATOR, CW_GATHERING, CW_LUMINOUS, CW_SPORES,
                   CW_CYSTS, CW_COLONY, CW_BRINE, CW_HOT, CW_COLD, CW_ACID, CW_AMMONIA, CW_HYDROCARBON, CW_ASLEEP, CW_WANTS };
const char* const WANT_NAMES[CW_WANTS] = {"light", "broth", "hydrogen", "sulphide", "iron", "methane", "grazer", "predator", "gathering", "luminous",
                                          "spores", "cysts", "colony", "brine", "hot", "cold", "acid", "ammonia", "hydrocarbon", "asleep"};
struct Found { StarSystem sys; int body = -1, species = -1; CultureKind kind; Microhabitat hab; Medium asWorld; std::string world; };

bool wants(int w, const Biota& B, const Microbe& m) {
    const bool living = m.state == SS_LIVING, water = B.chemistry == CH_WATER;
    switch (w) {
        case CW_LIGHT: return living && water && m.way == LW_LIGHT && smallCellPlan(m.plan);
        case CW_BROTH: return living && water && m.way == LW_DEAD && smallCellPlan(m.plan) && !(m.cycle & LC_SPORULATING) && !m.glows;
        case CW_HYDROGEN: return living && water && m.way == LW_CHEMICAL && m.substrate == SU_HYDROGEN;
        case CW_SULPHIDE: return living && water && m.way == LW_CHEMICAL && m.substrate == SU_SULPHIDE;
        case CW_IRON: return living && water && m.way == LW_CHEMICAL && m.substrate == SU_IRON;
        case CW_METHANE: return living && water && m.way == LW_CHEMICAL && m.substrate == SU_METHANE;
        case CW_GRAZER: return living && water && m.way == LW_GRAZER && largeCellPlan(m.plan);
        case CW_PREDATOR: return living && water && m.way == LW_PREDATOR && largeCellPlan(m.plan);
        case CW_GATHERING: return living && m.plan == MP_GATHERING;
        case CW_LUMINOUS: return living && m.glows && m.way != LW_LIGHT;
        case CW_SPORES: return living && water && smallCellPlan(m.plan) && (m.cycle & LC_SPORULATING) && m.way == LW_DEAD;
        case CW_CYSTS: return living && water && largeCellPlan(m.plan) && (m.cycle & LC_DORMANT);
        case CW_COLONY: return living && water && colonyPlan(m.plan) && m.plan != MP_GATHERING && (m.cycle & LC_COLONY);
        case CW_BRINE: return living && water && m.saltLo == 2;
        case CW_HOT: return living && water && m.tOpt >= 60;
        case CW_COLD: return living && water && m.tOpt <= 5;
        case CW_ACID: return living && B.chemistry == CH_ACID;
        case CW_AMMONIA: return living && B.chemistry == CH_AMMONIA;
        case CW_HYDROCARBON: return living && B.chemistry == CH_HYDROCARBON;
        case CW_ASLEEP: return m.state == SS_DORMANT && smallCellPlan(m.plan);
        default: return false;
    }
}

// the scan region's first species of each want, with its world's facts (its form's size, its place as found: a living one where it grows)
void findCultures(Found (&out)[CW_WANTS], bool (&found)[CW_WANTS], Found* mirrored, bool* haveMirrored) {
    for (bool& f : found) f = false;
    if (haveMirrored) *haveMirrored = false;
    auto all = [&] { for (bool f : found) if (!f) return false; return !haveMirrored || *haveMirrored; };
    for (auto& [x, z, s, scanned] : scanSystems(150, 340, 20, 160, [&] { return !all(); })) {
        const StarSystem& sys = scanned;
        auto L = lifeOf(sys);
        for (int i = 0; i < (int)sys.bodies.size(); i++) {
            const Biota& B = L->bodies[i];
            if (B.species.empty() || B.stage == LS_PREBIOTIC) continue;
            std::vector<Morph> forms;
            auto take = [&](Found& F, const Microbe& m) {
                if (forms.empty()) forms = morphsOf(sys, i);
                F.sys = sys; F.body = i; F.species = m.index;
                F.kind = cultureKindOf(B, m.index, 2 * forms[m.index].bodyUm());
                F.hab.niche = m.niche; F.hab.salt = m.saltLo; F.hab.place = mix64(sys.bodies[i].seed ^ (uint64_t)(m.index + 1));
                F.hab.meanC = m.state == SS_LIVING ? clampd(B.meanC, m.tMin + 1, m.tMax - 1) : B.meanC;
                const int cls = sys.ofCompanion(i) ? sys.companionStar().cls : sys.star.cls;
                F.asWorld = mediumOfPlace(B, sys.bodies[i].type, sys.sunless() ? STAR_ROGUE : cls, F.hab, F.hab.meanC);
                F.world = fmt("%s (%s)", sys.bodies[i].name.c_str(), upper(shortType(sys.bodies[i].type)).c_str());
            };
            for (const Microbe& m : B.species) {
                for (int w = 0; w < CW_WANTS; w++) if (!found[w] && wants(w, B, m)) { take(out[w], m); found[w] = true; }
                if (haveMirrored && !*haveMirrored && B.genesis.mirrored && B.chemistry == CH_WATER && m.state == SS_LIVING && m.way == LW_DEAD && smallCellPlan(m.plan)) {
                    take(*mirrored, m); *haveMirrored = true;
                }
            }
        }
    }
}

std::string mmss(double t) { const int s = (int)std::floor(t + 1e-6); return fmt("%d:%02d", s / 60, s % 60); }
std::string mediumLine(const Medium& m) {
    return fmt("%s %.0f C, %s, %s, %s", CULTURE_CELL_NAMES[m.cell], m.tempC, CULTURE_LIGHT_NAMES[m.light], CULTURE_ENERGY_NAMES[m.energy], CULTURE_SALT_NAMES[clampi(m.salt, 0, 2)]);
}
// a culture's hour: its curve by five minutes (log10 cells a millilitre; spores after a slash where it has them), its stages' times, a strain's
std::string hourOf(const FlaskCulture& c, int who, std::string* stages) {
    const CultureRun R = runCulture(c);
    std::string curve;
    for (int k = 0; k <= 12; k++) {
        const CultureNow n = cultureAt(c, R, k * 300.0);
        const CultureOne& o = n.one[who];
        curve += fmt("%s%4.1f", k ? " " : "", o.cells > 0 ? std::log10(o.cells) : 0.0);
        if (o.spores >= 1) curve += fmt("/%.1f", std::log10(o.spores));
    }
    if (stages) {
        stages->clear();
        int last = -1;
        for (int s = 0; s <= 3600; s++) {
            const int st = cultureAt(c, R, s).one[who].stage;
            if (st != last) { *stages += fmt("%s%s %s", stages->empty() ? "" : ", ", CULTURE_STAGE_NAMES[st], mmss(s).c_str()); last = st; }
        }
        if (R.wokeT[who] >= 0) *stages += fmt("; woke %s", mmss(R.wokeT[who]).c_str());
        if (R.strainT[who] >= 0) *stages += fmt("; a strain at %s (move %d to %.1f)", mmss(R.strainT[who]).c_str(), R.strainWhat[who], R.strainValue[who]);
    }
    return curve;
}
FlaskCulture alone(const CultureKind& k, const Medium& m) {
    FlaskCulture c; c.kind[0] = k;
    CultureEvent e; e.kind = CD_START; e.t = 0; e.m = m;
    c.events.push_back(e);
    return c;
}
// Y-04: a found species' line with its gene of a kind switched (its genes read from it, its switch priced as a flask reads it); false: none
bool switchedOf(const Found& f, int kind, int sw, CultureKind& out) {
    const Biota& B = lifeOf(f.sys)->bodies[f.body];
    Morph form = morphsOf(f.sys, f.body)[f.species];
    const Morph wildForm = form;
    MicrobeGenes g = microbeGenesOf(B.species[f.species], form, B.genesis);
    int place = -1;
    for (const MicrobeGene& G : g.genes) if (G.kind == kind && place < 0) place = G.place;
    if (place < 0) return false;
    out = f.kind;
    GeneEdit e; e.place = place; e.sw = sw;
    applyEdits(out, form, g, {e});
    out.burden = geneBurden(f.kind, wildForm, out, form, g);
    for (CultureSwitch& w : out.switches) if (const MicrobeGene* G = g.at(w.place)) w.price = genePrice(f.kind, wildForm, out, form, *G);
    return true;
}
// when a culture first glows in its hour (-1 never)
double glowFrom(const FlaskCulture& c) {
    const CultureRun R = runCulture(c);
    for (int t = 0; t <= 3600; t += 5) if (cultureAt(c, R, t).one[0].glowing) return t;
    return -1;
}
enum PairWant { PW_GRAZE, PW_PREY, PW_LIGHT_FEED, PW_HYDROGEN_FEED, PW_COMPETE, PW_MIRRORED, PW_COUNT };
const char* const PAIR_WANT_NAMES[PW_COUNT] = {"a grazer and a small cell", "a predator and a large cell", "a light-feeder and a heterotroph",
                                               "a fermenter and a hydrogen-eater", "two heterotrophs", "two of mirrored hands"};
struct PairFound { bool ok = false; CultureKind a, b; int ia = -1, ib = -1; std::string world, worldB; Medium m; };

// a medium both grow in (the fed one on the other): their chemistry's cell, a warmth in both ranges, a salt both bear, the energy and light given
bool bothGrow(const CultureKind& a, const CultureKind& b, int energy, int light, Medium& out) {
    if (a.chemistry != b.chemistry) return false;
    const double lo = std::max(a.s.tMin, b.s.tMin) + 1, hi = std::min(a.s.tMax, b.s.tMax) - 1;
    if (lo >= hi) return false;
    Medium m;
    m.cell = chemistryCell(a.chemistry);
    m.salt = std::max(a.s.saltLo, b.s.saltLo); m.energy = energy; m.light = light;
    double clo, chi; cellRange(m.cell, clo, chi);
    if (m.salt > std::min(a.s.saltHi, b.s.saltHi)) return false;
    int lead = -1;
    const int pr = pairOf(a, b, lead);
    const bool aFed = (pr == PA_CYCLE && lead == 0) || (pr == PA_FEED && lead == 1), bFed = (pr == PA_CYCLE && lead == 1) || (pr == PA_FEED && lead == 0);
    double best = 0;   // the whole degree where the slower of the two grows the fastest
    for (double T = std::ceil(std::max(lo, clo)); T <= std::min(hi, chi); T += 1) {
        m.tempC = T;
        const Growth ga = growthIn(a, m, aFed), gb = growthIn(b, m, bFed);
        if (ga.fit != GF_GROWS || gb.fit != GF_GROWS) continue;
        const double share = std::min(ga.mu * bestDoublingS(a), gb.mu * bestDoublingS(b)) / std::log(2.0);   // each against its best
        if (share > best) { best = share; out = m; }
    }
    return best >= 0.25;   // both at a quarter of their best pace at least
}
void findPairs(PairFound (&P)[PW_COUNT]) {
    auto all = [&] { for (const PairFound& p : P) if (!p.ok) return false; return true; };
    CultureKind ourHand, mirrorHand; bool haveOur = false, haveMirror = false; std::string ourWorld, mirrorWorld; int ourI = -1, mirrorI = -1;
    for (auto& [x, z, s, scanned] : scanSystems(150, 340, 20, 160, [&] { return !all(); })) {
        const StarSystem& sys = scanned;
        auto L = lifeOf(sys);
        for (int i = 0; i < (int)sys.bodies.size(); i++) {
            const Biota& B = L->bodies[i];
            if (!livingStage(B.stage)) continue;
            const std::vector<Morph> forms = morphsOf(sys, i);
            std::vector<CultureKind> K;
            for (const Microbe& m : B.species) K.push_back(cultureKindOf(B, m.index, 2 * forms[m.index].bodyUm()));
            const std::string world = fmt("%s (%s)", sys.bodies[i].name.c_str(), upper(shortType(sys.bodies[i].type)).c_str());
            auto put = [&](int w, int a, int b, const Medium& m) { P[w].ok = true; P[w].a = K[a]; P[w].b = K[b]; P[w].ia = a; P[w].ib = b; P[w].world = P[w].worldB = world; P[w].m = m; };
            for (int a = 0; a < (int)K.size(); a++) {
                if (K[a].s.state != SS_LIVING) continue;
                for (int b = 0; b < (int)K.size(); b++) {
                    if (a == b || K[b].s.state != SS_LIVING) continue;
                    int lead = -1;
                    const int pr = pairOf(K[a], K[b], lead);
                    const int prey = K[b].s.way == LW_LIGHT ? CE_NONE : (K[b].s.way == LW_CHEMICAL ? K[b].s.substrate : CE_BROTH);
                    const int preyLight = K[b].s.way == LW_LIGHT ? CL_WHITE : CL_DARK;
                    Medium m;
                    if (!P[PW_GRAZE].ok && pr == PA_CYCLE && lead == 0 && K[a].s.way == LW_GRAZER && bothGrow(K[a], K[b], prey, preyLight, m)) put(PW_GRAZE, a, b, m);
                    if (!P[PW_PREY].ok && pr == PA_CYCLE && lead == 0 && K[a].s.way == LW_PREDATOR && largeCellPlan(K[b].s.plan) && bothGrow(K[a], K[b], prey, preyLight, m)) put(PW_PREY, a, b, m);
                    if (!P[PW_LIGHT_FEED].ok && pr == PA_FEED && lead == 0 && K[a].s.way == LW_LIGHT && bothGrow(K[a], K[b], CE_NONE, CL_WHITE, m)) put(PW_LIGHT_FEED, a, b, m);
                    if (!P[PW_HYDROGEN_FEED].ok && pr == PA_FEED && lead == 0 && K[a].s.way == LW_DEAD && bothGrow(K[a], K[b], CE_BROTH, CL_DARK, m)) put(PW_HYDROGEN_FEED, a, b, m);
                    if (!P[PW_COMPETE].ok && pr == PA_COMPETE && K[a].s.way == LW_DEAD && K[b].s.way == LW_DEAD && bothGrow(K[a], K[b], CE_BROTH, CL_DARK, m)) put(PW_COMPETE, a, b, m);
                }
                if (B.chemistry == CH_WATER && K[a].s.way == LW_DEAD && smallCellPlan(K[a].s.plan)) {   // a heterotroph of each hand, for the mirrored two
                    if (K[a].mirrored && !haveMirror) { mirrorHand = K[a]; haveMirror = true; mirrorWorld = world; mirrorI = a; }
                    if (!K[a].mirrored && !haveOur) { ourHand = K[a]; haveOur = true; ourWorld = world; ourI = a; }
                }
            }
            Medium m;
            if (!P[PW_MIRRORED].ok && haveOur && haveMirror && bothGrow(ourHand, mirrorHand, CE_BROTH, CL_DARK, m)) {
                P[PW_MIRRORED].ok = true; P[PW_MIRRORED].a = ourHand; P[PW_MIRRORED].b = mirrorHand; P[PW_MIRRORED].ia = ourI; P[PW_MIRRORED].ib = mirrorI;
                P[PW_MIRRORED].world = ourWorld; P[PW_MIRRORED].worldB = mirrorWorld; P[PW_MIRRORED].m = m;
            } else if (!P[PW_MIRRORED].ok && haveOur && haveMirror) haveMirror = false;   // their warmths apart: another of the mirrored hand
        }
    }
}
}   // namespace

int labCultures() {
    const auto t0 = std::chrono::steady_clock::now();
    Found F[CW_WANTS];
    bool found[CW_WANTS];
    findCultures(F, found, nullptr, nullptr);
    int missing = 0, grew = 0, media = 0;
    printf("lab cultures: twenty species of the scan region, an hour in four media each (log10 cells a millilitre by five minutes; /spores)\n");
    for (int w = 0; w < CW_WANTS; w++) {
        if (!found[w]) { printf("\n%s: none in the scan region\n", WANT_NAMES[w]); missing++; continue; }
        const Found& f = F[w];
        const Microbe& s = f.kind.s;
        printf("\n%s: %s M%d, %s %s, %s%s, %.0f to %.0f C (best %.0f), salt %d-%d, %s, doubling at its best %.0f s\n", WANT_NAMES[w], f.world.c_str(), f.species,
               MICROBE_PLAN_NAMES[s.plan], WAY_NAMES[s.way], s.way == LW_CHEMICAL ? SUBSTRATE_NAMES[s.substrate] : "", f.kind.spores ? " (as spores)" : "",
               s.tMin, s.tMax, s.tOpt, s.saltLo, s.saltHi, f.kind.mirrored ? "mirrored" : "our hand", bestDoublingS(f.kind));
        Medium best;
        best.cell = chemistryCell(f.kind.chemistry); best.tempC = std::round(s.tOpt); best.salt = s.saltLo;
        best.light = s.way == LW_LIGHT ? CL_WHITE : CL_DARK;
        best.energy = s.way == LW_CHEMICAL ? s.substrate : (s.way == LW_LIGHT ? CE_NONE : CE_BROTH);
        Medium standard;
        standard.cell = best.cell;
        double lo, hi; cellRange(standard.cell, lo, hi);
        standard.tempC = clampd(30, lo, hi);
        Medium edge = best;
        edge.tempC = std::round(10 * (s.tOpt + 0.85 * (s.tMax - s.tOpt))) / 10;
        const Medium ms[4] = {f.asWorld, standard, best, edge};
        const char* const names[4] = {"as its world", "standard", "its best", "its hot edge"};
        for (int k = 0; k < 4; k++) {
            const FlaskCulture c = alone(f.kind, ms[k]);
            std::string stages;
            const std::string curve = hourOf(c, 0, &stages);
            const Growth g = growthIn(f.kind, ms[k]);
            printf("  %-12s %s: %s; doubling %s\n    %s\n    %s\n", names[k], mediumLine(ms[k]).c_str(), GROWTH_FIT_NAMES[g.fit],
                   g.mu > 0 ? fmt("%.0f s", std::log(2.0) / g.mu).c_str() : "none", curve.c_str(), stages.c_str());
            media++;
            grew += g.fit == GF_GROWS;
        }
    }
    // the pairs, each of one world (one genesis, one hand) but the mirrored two: a grazer and a small cell, a predator and a large one, a
    // light-feeder and a heterotroph, a fermenter and a hydrogen-eater, two heterotrophs; in a medium both grow in, the second added at 2 minutes
    PairFound P[PW_COUNT];
    findPairs(P);
    printf("\nthe pairs (the second added at 2 minutes; log10 cells a millilitre by five minutes):\n");
    int pairsRun = 0, cycles = 0, winners = 0, ignored = 0, fed = 0;
    for (int w = 0; w < PW_COUNT; w++) {
        if (!P[w].ok) { printf("  %s: not found\n", PAIR_WANT_NAMES[w]); continue; }
        const PairFound& f = P[w];
        FlaskCulture c = alone(f.a, f.m);
        c.kind[1] = f.b;
        CultureEvent add; add.kind = CD_ADD; add.t = 120; add.m = f.m;
        c.events.push_back(add);
        const CultureRun R = runCulture(c);
        const CultureNow end = cultureAt(c, R, 3600);
        std::string verdict = CULTURE_PAIR_NAMES[R.pair];
        const char* who[2] = {"the first", "the second"};
        if (R.pair == PA_COMPETE && R.lead >= 0) { verdict += fmt(": %s wins", who[R.lead]); winners++; }
        if (R.pair == PA_CYCLE) { verdict += fmt(": %s eats, a turn of %s from %s", who[R.lead], mmss(R.period).c_str(), mmss(std::max(0.0, R.cycleT)).c_str()); cycles += R.period > 0; }
        if (R.pair == PA_FEED) { verdict += fmt(": %s feeds", who[R.lead]); fed++; }
        if (R.pair == PA_IGNORE) ignored++;
        printf("  %s: %s M%d (%s %s) and %s M%d (%s %s), %s: %s; at an hour %s and %s\n    first  %s\n    second %s\n", PAIR_WANT_NAMES[w], f.world.c_str(), f.ia,
               MICROBE_PLAN_NAMES[f.a.s.plan], WAY_NAMES[f.a.s.way], f.worldB.c_str(), f.ib, MICROBE_PLAN_NAMES[f.b.s.plan], WAY_NAMES[f.b.s.way], mediumLine(f.m).c_str(),
               verdict.c_str(), CULTURE_STAGE_NAMES[end.one[0].stage], CULTURE_STAGE_NAMES[end.one[1].stage], hourOf(c, 0, nullptr).c_str(), hourOf(c, 1, nullptr).c_str());
        pairsRun++;
    }
    // Y-04: lines switched: the luminous one's glow IN THE DARK (lit from its first cells in the dark, never in the light), the spore former's
    // rest IN THE LIGHT (spores at its end lit, none in the dark), each an hour lit and dark beside its species
    printf("\nthe switches (an hour, log10 cells a millilitre by five minutes; /spores):\n");
    int switched = 0;
    struct Sw { int want, kind, sw; const char* what; };
    for (const Sw& x : {Sw{CW_LUMINOUS, GN_GLOW, GW_DARK, "its glow IN THE DARK"}, Sw{CW_SPORES, GN_REST, GW_LIGHT, "its rest IN THE LIGHT"}}) {
        CultureKind k;
        if (!found[x.want] || !switchedOf(F[x.want], x.kind, x.sw, k)) { printf("  %s: none\n", x.what); continue; }
        const Found& f = F[x.want];
        printf("  %s M%d (%s), %s:\n", f.world.c_str(), f.species, WANT_NAMES[x.want], x.what);
        for (int light : {CL_DARK, CL_WHITE}) {
            Medium m;
            m.cell = chemistryCell(k.chemistry); m.tempC = std::round(k.s.tOpt); m.salt = k.s.saltLo; m.light = light;
            m.energy = k.s.way == LW_CHEMICAL ? k.s.substrate : (k.s.way == LW_LIGHT ? CE_NONE : CE_BROTH);
            const FlaskCulture line = alone(k, m), wild = alone(f.kind, m);
            std::string stages, wildStages;
            const std::string curve = hourOf(line, 0, &stages), wildCurve = hourOf(wild, 0, &wildStages);
            const double lg = glowFrom(line), wg = glowFrom(wild);
            printf("    %-5s the line   %s\n          %s%s\n      its species %s\n          %s%s\n", CULTURE_LIGHT_NAMES[light], curve.c_str(), stages.c_str(),
                   lg >= 0 ? fmt("; glowing from %s", mmss(lg).c_str()).c_str() : (k.s.glows ? "; never glowing" : ""), wildCurve.c_str(), wildStages.c_str(),
                   wg >= 0 ? fmt("; glowing from %s", mmss(wg).c_str()).c_str() : "");
        }
        switched++;
    }
    // the incubator's screen in a game: the pairs, the sleeping spores, a culture held at its hot edge, a luminous one, forty minutes on
    {
        TestGame g; Input in;
        g.savePrefix = "shots/tests/lab_cultures_save"; g.settingsPath = "shots/tests/lab_cultures_settings.txt"; g.guidePath = "shots/tests/lab_cultures_guide.txt"; g.keysPath = "shots/tests/test_keys.txt";
        remove(g.guidePath.c_str()); remove(g.settingsPath.c_str());
        g.newGame(); g.setState(GameState::SPACE);
        applyTestScale(g);
        auto vialOf = [&](const StarSystem& sys, int body) {
            Vial v; v.sx = sys.star.sx; v.sy = sys.star.sy; v.sz = sys.star.sz; v.body = body; v.name = "WET SILT"; v.hab.niche = NI_SILT;
            g.testColdStore().push_back(v);
            return (int)g.testColdStore().size() - 1;
        };
        std::vector<std::string> names;
        int f = 0;
        auto best = [](const CultureKind& k) {
            Medium m; m.cell = chemistryCell(k.chemistry); m.tempC = std::round(k.s.tOpt); m.salt = k.s.saltLo;
            m.energy = k.s.way == LW_CHEMICAL ? k.s.substrate : (k.s.way == LW_LIGHT ? CE_NONE : CE_BROTH);
            m.light = k.s.way == LW_LIGHT ? CL_WHITE : CL_DARK;
            return m;
        };
        for (int w = 0; w < PW_COUNT && f < 6; w++) {   // the pairs (their worlds' vials found again by their species)
            if (!P[w].ok) continue;
            bool put = false;
            for (auto& [x, z, s2, scanned] : scanSystems(150, 340, 20, 160, [&] { return !put; })) {
                const StarSystem& sys = scanned;
                for (int b = 0; b < (int)sys.bodies.size() && !put; b++) {
                    const std::string nm = fmt("%s (%s)", sys.bodies[b].name.c_str(), upper(shortType(sys.bodies[b].type)).c_str());
                    if (nm != P[w].world) continue;
                    const int v = vialOf(sys, b);
                    if (!g.testFlaskPut(f, v, P[w].ia)) continue;
                    g.testFlaskSet(f, P[w].m);
                    if (P[w].worldB == P[w].world) g.testFlaskPut(f, v, P[w].ib);
                    put = true;
                }
            }
            if (P[w].worldB != P[w].world && put) {   // the mirrored second, of its own world
                bool put2 = false;
                for (auto& [x, z, s2, scanned] : scanSystems(150, 340, 20, 160, [&] { return !put2; })) {
                    const StarSystem& sys = scanned;
                    for (int b = 0; b < (int)sys.bodies.size() && !put2; b++) {
                        if (fmt("%s (%s)", sys.bodies[b].name.c_str(), upper(shortType(sys.bodies[b].type)).c_str()) != P[w].worldB) continue;
                        put2 = g.testFlaskPut(f, vialOf(sys, b), P[w].ib);
                    }
                }
            }
            if (put) { names.push_back(PAIR_WANT_NAMES[w]); f++; }
        }
        for (int w : {CW_ASLEEP, CW_LUMINOUS, CW_SPORES}) {   // the spores woken, the luminous crowd, a spore former held at its hot edge
            if (!found[w] || f >= INCUBATOR_FLASKS) continue;
            const int v = vialOf(F[w].sys, F[w].body);
            if (!g.testFlaskPut(f, v, F[w].species)) continue;
            Medium m = best(F[w].kind);
            if (w == CW_SPORES) m.tempC = std::round(10 * (F[w].kind.s.tOpt + 0.85 * (F[w].kind.s.tMax - F[w].kind.s.tOpt))) / 10;
            g.testFlaskSet(f, m);
            names.push_back(w == CW_SPORES ? "a spore former at its hot edge" : WANT_NAMES[w]);
            f++;
        }
        g.testLabSkip(40 * 60);
        g.openScreen(GameState::INCUBATOR);
        std::vector<std::pair<std::vector<uint32_t>, std::string>> ps;
        for (int i = 0; i < f; i++) {
            g.testIncub().sel = i;
            g.testClearStatus();
            g.frame(in, 0); in.newFrame();
            g.testClearStatus();
            g.frame(in, 0); in.newFrame();
            ps.push_back({std::vector<uint32_t>(g.output(), g.output() + (size_t)FBW * FBH), fmt("FLASK %d: %s", i + 1, upper(names[i]).c_str())});
        }
        const int gap = 4, head = 14, cols = 3, rows = ((int)ps.size() + cols - 1) / cols;
        Sheet sh(cols * (FBW + gap) + gap, rows * (FBH + gap) + gap + head);
        sh.label(4, 3, "lab cultures: the incubator's screen forty minutes on (the pairs, the spores woken, the luminous, a strain at the hot edge)");
        for (size_t i = 0; i < ps.size(); i++) {
            const int x = gap + (int)(i % cols) * (FBW + gap), y = head + gap + (int)(i / cols) * (FBH + gap);
            sh.blit(ps[i].first, FBW, FBH, x, y);
            sh.label(x + 2, y + FBH - 11, ps[i].second);
        }
        sh.save("shots/tests/lab_cultures.png");
        int strains = 0;
        for (const auto& kv : g.guide.strains) { (void)kv; strains++; }
        printf("the incubator: %d flasks forty minutes on, %d strains, %d WOKEN in the log\n", f, strains,
               (int)std::count_if(g.guide.log.begin(), g.guide.log.end(), [](const LogEntry& e) { return e.kind == "WOKEN"; }));
        // a drop of each flask on the microscope's stage as its culture is (its cells, a quarter of them dividing while it grows, its spores
        // or cysts, its lysed), at its first look five seconds on
        g.closeScreen();
        g.openScreen(GameState::MICROSCOPE);
        std::vector<std::pair<std::vector<uint32_t>, std::string>> drops;
        for (int i = 0; i < f; i++) {
            if (!g.testScopeStageFlask(i)) { printf("  flask %d's drop: NOT STAGED\n", i + 1); continue; }
            const MicroscopeState& M = g.testScope();
            std::string kinds;
            for (const SlideKind& K : M.mount.kinds) kinds += fmt("%s M%d %s%s, %d in a tile of %.0f um", kinds.empty() ? "" : ";", K.species, CYCLE_FORM_NAMES[K.form],
                                                                  K.active ? " active" : "", K.n, K.tile);
            g.testScope().t = 5;
            g.testClearStatus(); g.frame(in, 0); in.newFrame();
            g.testClearStatus(); g.frame(in, 0); in.newFrame();
            printf("  flask %d's drop: %s x%.0f, prep %+d, %zu in view:%s\n", i + 1, SEEING_NAMES[M.seeing], g.testScopeMag(), M.mount.prep, M.shown.size(), kinds.c_str());
            drops.push_back({std::vector<uint32_t>(g.output(), g.output() + (size_t)FBW * FBH), fmt("FLASK %d'S DROP: %s", i + 1, upper(names[i]).c_str())});
        }
        {
            const int dcols = 3, drows = ((int)drops.size() + dcols - 1) / dcols;
            Sheet ds(dcols * (FBW + gap) + gap, drows * (FBH + gap) + gap + head);
            ds.label(4, 3, "lab cultures: a drop of each flask on the microscope's stage, as its culture is forty minutes on");
            for (size_t i = 0; i < drops.size(); i++) {
                const int x = gap + (int)(i % dcols) * (FBW + gap), y = head + gap + (int)(i / dcols) * (FBH + gap);
                ds.blit(drops[i].first, FBW, FBH, x, y);
                ds.label(x + 2, y + FBH - 11, drops[i].second);
            }
            ds.save("shots/tests/lab_cultures_drops.png");
        }
    }
    {   // J-01: the world genes' looks, the incubator's screen of each flask forty minutes on
        TestGame g; Input in;
        g.savePrefix = "shots/tests/lab_cultures_save"; g.settingsPath = "shots/tests/lab_cultures_settings.txt"; g.guidePath = "shots/tests/lab_cultures_world_guide.txt";
        g.keysPath = "shots/tests/test_keys.txt";
        remove(g.guidePath.c_str()); remove(g.settingsPath.c_str());
        g.newGame(); g.setState(GameState::SPACE);
        applyTestScale(g);
        std::vector<std::string> names;
        const int f = labWorldLooks(g, names);
        g.openScreen(GameState::INCUBATOR);
        std::vector<std::pair<std::vector<uint32_t>, std::string>> ps;
        printf("\nthe world genes' looks (a line each, at its height):\n");
        for (int i = 0; i < f; i++) {
            g.testIncub().sel = i;
            g.testClearStatus(); g.frame(in, 0); in.newFrame();
            g.testClearStatus(); g.frame(in, 0); in.newFrame();
            ps.push_back({std::vector<uint32_t>(g.output(), g.output() + (size_t)FBW * FBH), fmt("FLASK %d: %s", i + 1, names[i].c_str())});
            const FlaskShow S = g.testFlaskShow(i);
            const CultureNow now = g.testFlaskNow(i);
            printf("  flask %2d %-24s %s%s; bubbles %.2f, dyed %.2f, scum %.2f, slime %.2f, sediment %.2f, clumps %.2f%s%s%s%s\n", i + 1, names[i].c_str(),
                   CULTURE_STAGE_NAMES[now.one[0].stage], now.n > 1 ? (std::string(" + ") + CULTURE_STAGE_NAMES[now.one[1].stage]).c_str() : "", S.bubbles, S.dyed, S.scum,
                   S.slime, S.sediment, S.clumps, S.overgrown ? (S.tissue ? ", overgrown in tissue" : ", overgrown") : "", S.frozen ? ", frozen" : "",
                   S.poisoned >= 0 ? ", the other poisoned" : "", S.passedTo >= 0 ? ", genes passed" : "");
        }
        const int gap = 4, head = 14, cols = 3, rows = ((int)ps.size() + cols - 1) / cols;
        Sheet sh(cols * (FBW + gap) + gap, rows * (FBH + gap) + gap + head);
        sh.label(4, 3, "lab cultures: the world genes' looks, a line each changed once, at its height (J-01)");
        for (size_t i = 0; i < ps.size(); i++) {
            const int x = gap + (int)(i % cols) * (FBW + gap), y = head + gap + (int)(i / cols) * (FBH + gap);
            sh.blit(ps[i].first, FBW, FBH, x, y);
            sh.label(x + 2, y + FBH - 11, ps[i].second);
        }
        sh.save("shots/tests/lab_cultures_world.png");
    }
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    printf("\nlab cultures: %d species (%d not found), %d media (%d grow), %d pairs: %d cycling, %d won, %d feeding, %d mirrored; %d lines switched (%.1f s)\n",
           CW_WANTS - missing, missing, media, grew, pairsRun, cycles, winners, fed, ignored, switched, secs);
    return 0;
}

// J-01: the twelve flasks stocked with the world genes' looks, each a line of a scan region's species changed once (its gene written in the guide):
// METHANE bubbles, RED water, a scum, slime, a CHALK sediment, clumps, overgrown into tissue (STOP cut, BIND at its heaviest), overgrown packed
// (STOP cut), frozen at -4 C, grown under the ultraviolet light, a NERVE poison killing another, genes passed to another; each begun so long ago
// that its look is at its height now (a gas while it grows, an overgrowth past its crowd). Their names in `names`; returns the flasks filled
int labWorldLooks(TestGame& g, std::vector<std::string>& names) {
    struct Line { FlaskSpecies w; std::string key; CultureKind k; MicrobeGenes genes; };
    auto lines = [&](const StarSystem& sys, int body) {
        std::vector<Line> out;
        for (const Microbe& m : lifeOf(sys)->bodies[body].species) {
            if (m.state != SS_LIVING) continue;
            Line l; l.w.sx = sys.star.sx; l.w.sy = sys.star.sy; l.w.sz = sys.star.sz; l.w.body = body; l.w.species = m.index;
            l.key = Guide::bodyKey(l.w.sx, l.w.sy, l.w.sz, body) + "/M" + std::to_string(m.index);
            Morph f;
            if (g.testFlaskKind(l.w, l.k, f, &l.genes) && l.genes.valid) out.push_back(l);
        }
        return out;
    };
    auto bestIn = [](const CultureKind& k) {
        Medium m; m.cell = chemistryCell(k.chemistry); m.tempC = std::round(k.s.tOpt); m.salt = k.s.saltLo;
        m.light = k.s.way == LW_LIGHT ? CL_WHITE : CL_DARK;
        m.energy = k.s.way == LW_CHEMICAL ? k.s.substrate : (k.s.way == LW_LIGHT ? CE_NONE : CE_BROTH);
        return m;
    };
    auto grows = [&](const Line& l) { return growthIn(l.k, bestIn(l.k)).fit == GF_GROWS; };
    auto geneOf = [](const Line& l, int kind) { for (const MicrobeGene& G : l.genes.genes) if (G.kind == kind) return &G; return (const MicrobeGene*)nullptr; };
    auto heaviest = [](const GeneCode& c, std::string p) {
        const char* abc = alphabetOf(c.letters);
        for (int l = 0; l < c.letters; l++) if (c.weight[l] == c.letters - 1) for (char& ch : p) ch = abc[l];
        return p;
    };
    auto reading = [](const GeneCode& c, const std::vector<int>& t, int v) {
        const char* abc = alphabetOf(c.letters);
        for (int r = 0; r < (int)t.size(); r++) if (t[r] == v) return std::string() + abc[r % c.letters] + abc[(r / c.letters) % c.letters];
        return std::string();
    };
    int f = 0, n = 0;
    names.clear();
    g.testLabSkip(3600);   // (an hour on, so each culture can be begun its own while ago)
    // a line of the first species a look wants (and a second of its world for a pair), its edits, its medium
    auto stock = [&](const char* name, const std::function<bool(const Line&)>& want, const std::function<std::string(const Line&, const GeneCode&)>& edits,
                     const std::function<void(Medium&)>& med, bool pair, double minutes) {
        bool done = false;
        for (auto& [x, z, sv, sys] : scanSystems(150, 340, 20, 160, [&] { return !done; })) {
            auto L = lifeOf(sys);
            for (int b = 0; b < (int)sys.bodies.size() && !done; b++) {
                if (!livingStage(L->bodies[b].stage) || L->bodies[b].genesis.id == 0) continue;
                const std::vector<Line> ls = lines(sys, b);
                for (size_t i = 0; i < ls.size() && !done; i++) {
                    if (!want(ls[i])) continue;
                    const Medium m0 = bestIn(ls[i].k);
                    int other = -1, lead = -1;
                    for (size_t j = 0; pair && j < ls.size() && other < 0; j++)
                        if (j != i && growthIn(ls[j].k, m0).fit == GF_GROWS && !ls[j].k.world.poisons() && ls[j].k.world.carrier <= 0 &&
                            pairOf(ls[i].k, ls[j].k, lead) == PA_COMPETE)
                            other = (int)j;
                    if (pair && other < 0) continue;
                    const GeneCode c = geneCodeOf(ls[i].genes.genesis);
                    StrainRecord r; r.what = SM_EDIT; r.edits = edits(ls[i], c);
                    const int sn = ++n;
                    g.guide.strains[ls[i].key + "/S" + std::to_string(sn)] = r;
                    g.guide.microbes[ls[i].key].seen |= SEEN_FORM | SEEN_KIN;
                    Flask& F = g.testLab().flasks[f];
                    Medium m = m0;
                    med(m);
                    F = Flask(); F.n = 1; F.who[0] = ls[i].w; F.who[0].strain = F.who[0].strainIn = sn; F.medium = F.asWorld = m;
                    CultureEvent e; e.kind = CD_START; e.t = g.testLab().clock - 60 * minutes; e.m = m;
                    F.events.push_back(e);
                    if (other >= 0) {
                        F.n = 2; F.who[1] = ls[other].w;
                        CultureEvent a; a.kind = CD_ADD; a.t = e.t; a.m = m; a.who = 1;
                        F.events.push_back(a);
                    }
                    F.made = false;
                    names.push_back(name);
                    f++; done = true;
                }
            }
        }
    };
    auto one = [&](int kind, std::function<std::string(const Line&, const GeneCode&, const MicrobeGene&)> part) {
        return [kind, part, geneOf](const Line& l, const GeneCode& c) { const MicrobeGene* G = geneOf(l, kind); return fmt("%d=%s", G->place + 1, part(l, c, *G).c_str()); };
    };
    auto table = [&](int value) {
        return [value, reading](const Line&, const GeneCode& c, const MicrobeGene& G) { return reading(c, *worldTable(c, G.kind), value) + G.part.substr(2); };
    };
    auto heavy = [&](const Line&, const GeneCode& c, const MicrobeGene& G) { return heaviest(c, G.part); };
    auto best = [](Medium&) {};
    stock("METHANE BUBBLES (GAS)", [&](const Line& l) { return grows(l) && !l.k.world.gives(); }, one(GN_GAS, table(WG_METHANE)), best, false, 7);
    stock("RED WATER (DYE)", [&](const Line& l) { return grows(l) && !l.k.world.dyes(); }, one(GN_DYE, table(WD_RED)), best, false, 15);
    stock("A SCUM (LIFT)", [&](const Line& l) { return grows(l) && l.k.world.lift <= 0; }, one(GN_LIFT, heavy), best, false, 12);
    stock("SLIME (SLIME)", [&](const Line& l) { return grows(l) && l.k.world.slime <= 0; }, one(GN_SLIME, heavy), best, false, 12);
    stock("A CHALK SEDIMENT (SHELL)", [&](const Line& l) { return grows(l) && !l.k.world.shells(); }, one(GN_SHELL, table(SHL_CHALK)), best, false, 20);
    stock("CLUMPS (BIND)", [&](const Line& l) { return grows(l) && l.k.world.bind <= 0; }, one(GN_BIND, heavy), best, false, 12);
    stock("TISSUE (STOP CUT, BIND)", grows, [&](const Line& l, const GeneCode& c) {
              const MicrobeGene *S = geneOf(l, GN_STOP), *B = geneOf(l, GN_BIND);
              return fmt("%d=-;%d=%s", S->place + 1, B->place + 1, heaviest(c, B->part).c_str());
          }, best, false, 40);
    stock("OVERGROWN (STOP CUT)", [&](const Line& l) { return grows(l) && l.k.world.bind <= 0; }, [&](const Line& l, const GeneCode&) { return fmt("%d=-", geneOf(l, GN_STOP)->place + 1); },
          best, false, 40);
    stock("FROZEN AT -4 C (FROST)", [&](const Line& l) { return l.k.chemistry == CH_WATER && l.k.world.frostC <= FROST_NONE + 1 && l.k.s.saltLo <= 1 && l.k.s.tMax > 0; },
          one(GN_FROST, heavy), [](Medium& m) { m.tempC = -4; m.salt = 0; }, false, 5);
    stock("UNDER UV (SHIELD)", [&](const Line& l) { return grows(l) && l.k.world.shield < UV_SHIELD && l.k.s.way != LW_LIGHT; }, one(GN_SHIELD, heavy),
          [](Medium& m) { m.light = CL_UV; }, false, 10);
    stock("A NERVE POISON (POISON)", grows, one(GN_POISON, [&](const Line&, const GeneCode& c, const MicrobeGene& G) {
              return reading(c, c.poison, PZ_NERVE) + heaviest(c, G.part.substr(2)); }), best, true, 12);
    stock("GENES PASSED (CARRIER)", [&](const Line& l) { return grows(l) && l.k.world.carrier <= 0; }, one(GN_CARRIER, heavy), best, true, 12);
    for (int i = 0; i < f; i++) g.testFlaskNow(i);   // (made)
    g.testLabSkip(1);
    return f;
}

// Z-05: a laboratory stocked for the pictures and the bench: the scan region's living water world with the most species that grow (a luminous one
// among them where there is one), a vial of its ground in the cold store, its species in the twelve flasks in turn, each in its best medium
// under the lights in turn (white, red, blue, dark; a light-feeder never dark), the lab's clock run fifteen minutes on. Returns the flasks
// filled
int labStock(TestGame& g) {
    StarSystem best; int bestBody = -1, bestN = 0; bool bestGlow = false;
    for (auto& [x, z, s, scanned] : scanSystems(150, 340, 20, 160, [&] { return !(bestN >= 8 && bestGlow); })) {
        const StarSystem& sys = scanned;
        auto L = lifeOf(sys);
        for (int i = 0; i < (int)sys.bodies.size(); i++) {
            const Biota& B = L->bodies[i];
            if (!livingStage(B.stage) || B.chemistry != CH_WATER) continue;
            int n = 0; bool glow = false;
            for (const Microbe& m : B.species) if (m.state == SS_LIVING && m.way != LW_NONE && m.way != LW_PREDATOR) { n++; glow = glow || m.glows; }
            if ((glow && !bestGlow && n >= 6) || ((glow || !bestGlow) && n > bestN)) { best = sys; bestBody = i; bestN = n; bestGlow = glow; }
        }
    }
    if (bestBody < 0) return 0;
    auto L = lifeOf(best);
    const Biota& B = L->bodies[bestBody];
    Vial v; v.sx = best.star.sx; v.sy = best.star.sy; v.sz = best.star.sz; v.body = bestBody; v.name = "WET SILT"; v.readingC = B.meanC;
    v.hab.niche = NI_SILT; v.hab.meanC = B.meanC; v.hab.place = mix64(best.bodies[bestBody].seed ^ 0x5EEDULL);
    g.testColdStore().push_back(v);
    const int vial = (int)g.testColdStore().size() - 1;
    std::vector<int> grow;
    for (const Microbe& m : B.species) if (m.state == SS_LIVING && m.way != LW_NONE && m.way != LW_PREDATOR) grow.push_back(m.index);
    std::stable_sort(grow.begin(), grow.end(), [&](int a, int b) { return B.species[a].glows > B.species[b].glows; });   // a luminous one first
    int put = 0;
    for (int i = 0; i < INCUBATOR_FLASKS && !grow.empty(); i++) {
        const Microbe& m = B.species[grow[i % grow.size()]];
        if (!g.testFlaskPut(i, vial, m.index)) continue;
        Medium md;
        md.cell = chemistryCell(B.chemistry); md.tempC = std::round(m.tOpt); md.salt = m.saltLo;
        md.energy = m.way == LW_CHEMICAL ? m.substrate : (m.way == LW_LIGHT ? CE_NONE : CE_BROTH);
        const int lights[4] = {CL_WHITE, CL_RED, CL_BLUE, CL_DARK};
        md.light = lights[i % 4];
        if (m.way == LW_LIGHT && md.light == CL_DARK) md.light = CL_WHITE;
        if (m.glows) md.light = CL_DARK;   // its own light in the dark
        g.testFlaskSet(i, md);
        put++;
    }
    g.testLabSkip(15 * 60);
    return put;
}
