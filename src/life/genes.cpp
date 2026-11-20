// Y-02: a species' genes (life/genes.h): a genesis's map and code, a species' genes read from what it is, a line's values from its edits, a
// strain's genes from its move
#include "life/genes.h"
#include "core/rng.h"
#include "core/types.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

const char* const GENE_KIND_NAMES[GN_COUNT] = {"VITAL", "QUIET", "ITS WARMTH", "ITS SALT", "ITS SIZE", "ITS PIGMENT", "ITS FOOD", "ITS MOTION",
                                               "A SENSE", "ITS REST", "ITS GLOW", "ITS SHAPE"};
const char* const GENE_SENSE_NAMES[SE_COUNT] = {"NOTHING", "THE LIGHT", "AWAY FROM THE LIGHT", "THE FOOD", "THE FIELD", "THE WARMTH"};

namespace {
// the genes' salts, each decision its own (nothing of generation 17 moves: a species is read, never drawn again)
constexpr uint64_t SALT_GENE_MAP = 0x6E4E500000000001ULL;     // a genesis's map: its places' order
constexpr uint64_t SALT_GENE_WEIGHT = 0x6E4E500000000002ULL;  // its letters' weights
constexpr uint64_t SALT_GENE_TABLE = 0x6E4E500000000003ULL;   // its tables
constexpr uint64_t SALT_GENE_QUIET = 0x6E4E500000000004ULL;   // a species' quiet genes
constexpr uint64_t SALT_GENE_WILD = 0x6E4E500000000005ULL;    // its wild letters
constexpr uint64_t SALT_GENE_LENGTH = 0x6E4E500000000006ULL;  // a gene's letters in all
constexpr uint64_t SALT_GENE_STRAIN = 0x6E4E500000000007ULL;  // a strain's letters stepped

const char ABC[] = "ACGTZPSB";   // every alphabet is its start (`alphabetOf`)
int letterIndex(char ch) { const char* p = std::strchr(ABC, ch); return p && ch ? (int)(p - ABC) : 0; }

// the map's places: three vital, three quiet, a warmth, a salt, a size, a pigment, a food, a motion, three senses, a rest, a glow, a shape
constexpr int MAP_KINDS[GENE_PLACES] = {GN_VITAL, GN_VITAL, GN_VITAL, GN_QUIET, GN_QUIET, GN_QUIET, GN_WARMTH, GN_SALT, GN_SIZE, GN_PIGMENT, GN_FOOD,
                                        GN_MOTION, GN_SENSE, GN_SENSE, GN_SENSE, GN_REST, GN_GLOW, GN_SHAPE};
constexpr int SHAPE_PLANS[4] = {MP_SPHERE, MP_ROD, MP_CURVED, MP_SPIRAL};   // Jones et al. 2001's MreB, Ausmees et al. 2003's crescentin
bool shapePlan(int p) { return p == MP_SPHERE || p == MP_ROD || p == MP_CURVED || p == MP_SPIRAL; }

void table(std::vector<int>& t, int runs, int values, Rng& r) {   // every value at least once, the rest at random (most runs synonyms)
    t.assign(runs, 0);
    std::vector<int> order(runs);
    for (int i = 0; i < runs; i++) order[i] = i;
    for (int i = runs - 1; i > 0; i--) std::swap(order[i], order[r.irange(i + 1)]);
    for (int i = 0; i < runs; i++) t[order[i]] = i < values ? i : r.irange(values);
}
int runOf(const GeneCode& c, const std::string& part, int from, int n) {
    int v = 0;
    for (int i = n - 1; i >= 0; i--) v = v * c.letters + (from + i < (int)part.size() ? letterIndex(part[from + i]) % c.letters : 0);
    return v;
}
std::string runLetters(const GeneCode& c, int run, int n) {
    std::string s;
    for (int i = 0; i < n; i++) { s += ABC[run % c.letters]; run /= c.letters; }
    return s;
}
// a run whose table gives `value`, chosen by the hash among its synonyms
std::string runFor(const GeneCode& c, const std::vector<int>& t, int value, int n, uint64_t h) {
    std::vector<int> runs;
    for (int i = 0; i < (int)t.size(); i++) if (t[i] == value) runs.push_back(i);
    if (runs.empty()) return runLetters(c, 0, n);
    return runLetters(c, runs[mix64(h) % runs.size()], n);
}
// n letters of a weight (spread by the hash: the same sum, different letters)
std::string lettersOfWeight(const GeneCode& c, int n, int W, uint64_t h) {
    const int top = c.letters - 1;
    W = clampi(W, 0, n * top);
    std::vector<int> w(n, W / n);
    for (int i = 0; i < W % n; i++) w[i]++;
    Rng r(h);
    for (int i = n - 1; i > 0; i--) std::swap(w[i], w[r.irange(i + 1)]);
    for (int t = 0; t < 2 * n; t++) {
        const int a = r.irange(n), b = r.irange(n);
        if (a != b && w[a] > 0 && w[b] < top && r.chance(0.5)) { w[a]--; w[b]++; }
    }
    int byWeight[8] = {};
    for (int i = 0; i < c.letters; i++) byWeight[c.weight[i]] = i;
    std::string s;
    for (int v : w) s += ABC[byWeight[v]];
    return s;
}
// a run's weight stepped by `d` (one letter a step, the hash's order of places)
void stepWeight(const GeneCode& c, std::string& part, int from, int n, int d, uint64_t h) {
    int byWeight[8] = {};
    for (int i = 0; i < c.letters; i++) byWeight[c.weight[i]] = i;
    Rng r(h);
    for (int guard = 0; d != 0 && guard < 64; guard++) {
        const int at = from + r.irange(n);
        const int w = c.weight[letterIndex(part[at]) % c.letters];
        if (d > 0 && w < c.letters - 1) { part[at] = ABC[byWeight[w + 1]]; d--; }
        else if (d < 0 && w > 0) { part[at] = ABC[byWeight[w - 1]]; d++; }
    }
}

double warmthStep(int chemistry, int letters) {   // a weight's warmth: its cell's range over the gene's heaviest, to half a degree
    double lo, hi;
    cellRange(chemistryCell(chemistry), lo, hi);
    return std::max(0.5, std::round(2 * (hi - lo) / (12.0 * (letters - 1))) / 2);
}
int saltBand(const GeneCode& c, int W) { return std::min(2, 3 * W / (6 * (c.letters - 1) + 1)); }
bool keeps(const GeneCode& c, int W) { return W > (4 * (c.letters - 1)) / 4; }   // a rest's or a glow's weight past a quarter: it does its work
double familyRef(int plan) { return smallCellPlan(plan) ? 1.0 : (largeCellPlan(plan) ? 40.0 : 50.0); }   // a cell's width at the size gene's middle
void familyWidth(int plan, double& lo, double& hi) {
    if (smallCellPlan(plan)) { lo = 0.1; hi = 16; }
    else if (largeCellPlan(plan)) { lo = 2; hi = 2500; }
    else { lo = 2; hi = 3000; }
}
// a motion's pace at the middle of its kind's (drawMotion's with its draw at half)
double kindSpeed(int motion, const Morph& m) {
    const double body = std::max(m.lengthUm, m.widthUm);
    double v = 0;
    switch (motion) {
        case MO_RUN_TUMBLE: v = body * 14; break;
        case MO_HELICAL: v = largeCellPlan(m.plan) || colonyPlan(m.plan) ? body * 5 : body * 11; break;
        case MO_GLIDING: v = 2.5; break;
        case MO_TWITCHING: v = 0.6; break;
        case MO_CRAWLING: v = 1.55; break;
        case MO_ROLLING: v = body * 0.65; break;
        case MO_CORKSCREW: v = 12.5; break;
        default: break;
    }
    if (m.chemistry == CH_AMMONIA) v *= 0.3;
    if (m.chemistry == CH_HYDROCARBON) v *= 0.1;
    return v;
}
uint32_t senseBit(int s) { return s == SE_LIGHT ? TX_LIGHT : (s == SE_FOOD ? TX_FOOD : (s == SE_FIELD ? TX_FIELD : (s == SE_WARMTH ? TX_WARMTH : 0u))); }
std::string words(const char* f, double a = 0, double b = 0, double c = 0) { char buf[96]; std::snprintf(buf, sizeof buf, f, a, b, c); return buf; }
}   // namespace

int genePartLetters(int kind) {
    switch (kind) {
        case GN_WARMTH: return 12;
        case GN_SALT: case GN_MOTION: return 6;
        case GN_SIZE: case GN_REST: case GN_GLOW: case GN_SHAPE: return 4;
        case GN_FOOD: return 3;
        case GN_PIGMENT: case GN_SENSE: return 2;
        default: return 8;   // vital, quiet
    }
}

int planMotions(int plan, int* out) {
    int n = 0;
    auto add = [&](int m) { out[n++] = m; };
    switch (plan) {   // the biota's kinds a plan moves in (its still one aside)
        case MP_SPHERE: add(MO_TWITCHING); add(MO_RUN_TUMBLE); break;
        case MP_ROD: add(MO_RUN_TUMBLE); add(MO_GLIDING); add(MO_TWITCHING); break;
        case MP_CURVED: add(MO_RUN_TUMBLE); add(MO_HELICAL); break;
        case MP_SPIRAL: add(MO_HELICAL); break;
        case MP_CORKSCREW: add(MO_CORKSCREW); break;
        case MP_FILAMENT: case MP_CHAIN: case MP_SHELLED: add(MO_GLIDING); break;
        case MP_STALKED: case MP_BUDDING: add(MO_RUN_TUMBLE); break;
        case MP_CHANGING: add(MO_TWITCHING); add(MO_GLIDING); break;
        case MP_AMOEBA: case MP_TEST: case MP_CHAMBERED: case MP_GATHERING: add(MO_CRAWLING); break;
        case MP_FLAGELLATE: case MP_CILIATE: case MP_ARMOURED: add(MO_HELICAL); break;
        case MP_ROLLER: add(MO_ROLLING); break;
        case MP_PLATE: add(MO_HELICAL); add(MO_ROLLING); break;
        default: break;
    }
    return n;
}

GeneCode geneCodeOf(const Genesis& g) {
    GeneCode c;
    c.genesis = g.id;
    c.letters = g.letters >= 8 ? 8 : (g.letters >= 6 ? 6 : 4);
    {   // the map: its places' order
        int k[GENE_PLACES];
        for (int i = 0; i < GENE_PLACES; i++) k[i] = MAP_KINDS[i];
        Rng r(g.id ^ SALT_GENE_MAP);
        for (int i = GENE_PLACES - 1; i > 0; i--) std::swap(k[i], k[r.irange(i + 1)]);
        int seen[GN_COUNT] = {};
        for (int i = 0; i < GENE_PLACES; i++) { c.kind[i] = k[i]; c.slot[i] = seen[k[i]]++; }
    }
    if (g.id == HOME_GENESIS_ID) { c.weight[0] = 0; c.weight[1] = 2; c.weight[2] = 3; c.weight[3] = 1; }   // A, T, C, G (ours: a G and C rich stem holds)
    else {
        int w[8];
        for (int i = 0; i < c.letters; i++) w[i] = i;
        Rng r(g.id ^ SALT_GENE_WEIGHT);
        for (int i = c.letters - 1; i > 0; i--) std::swap(w[i], w[r.irange(i + 1)]);
        for (int i = 0; i < c.letters; i++) c.weight[i] = w[i];
    }
    Rng r(g.id ^ SALT_GENE_TABLE);
    const int L = c.letters;
    table(c.pigment, L * L, PG_COUNT, r);
    table(c.food, L * L * L, SU_COUNT, r);
    table(c.motion, L * L, 4, r);
    table(c.sense, L * L, SE_COUNT, r);
    table(c.shape, L * L, 4, r);
    return c;
}

const MicrobeGene* MicrobeGenes::at(int place) const { for (const MicrobeGene& g : genes) if (g.place == place) return &g; return nullptr; }
MicrobeGene* MicrobeGenes::at(int place) { for (MicrobeGene& g : genes) if (g.place == place) return &g; return nullptr; }

int geneWeight(const GeneCode& c, const std::string& part, int from, int n) {
    if (n < 0) n = (int)part.size() - from;
    int w = 0;
    for (int i = from; i < from + n && i < (int)part.size(); i++) w += c.weight[letterIndex(part[i]) % c.letters];
    return w;
}
int geneTable(const GeneCode& c, const std::vector<int>& t, const std::string& part, int from, int n) {
    const int run = runOf(c, part, from, n);
    return run >= 0 && run < (int)t.size() ? t[run] : 0;
}

bool fleesLight(const Microbe& s) {
    if (s.flees >= 0) return s.flees == 1;
    const int n = s.niche;
    return s.motion != MO_NONE && s.way != LW_LIGHT && !(s.taxis & (TX_LIGHT | TX_FIELD)) &&
           (n == NI_SOIL || n == NI_SILT || n == NI_VENT || n == NI_ROCK || n == NI_PLUME);
}

MicrobeGenes microbeGenesOf(const Microbe& s, const Morph& m, const Genesis& g) {
    MicrobeGenes G;
    if (g.id == 0 || s.state == SS_FOSSIL || s.state == SS_PROTO || protocellPlan(s.plan)) return G;
    const GeneCode c = geneCodeOf(g);
    G.valid = true; G.genesis = g; G.letters = c.letters;
    const uint32_t rest = LC_DORMANT | LC_SPORULATING | LC_GATHERING;
    G.restCycle = s.cycle & (rest | LC_WAKING | ((s.cycle & LC_GATHERING) ? LC_FRUITING : 0u));
    G.sporeYears = s.sporeYears;
    // the senses it has, in their places' order: the light (or away from it), the food, the field, the warmth
    int senses[3] = {SE_NONE, SE_NONE, SE_NONE}, nSense = 0;
    auto sense = [&](int t) { if (nSense < 3) senses[nSense++] = t; };
    if (s.taxis & TX_LIGHT) sense(SE_LIGHT); else if (fleesLight(s)) sense(SE_DARK);
    if (s.taxis & TX_FOOD) sense(SE_FOOD);
    if (s.taxis & TX_FIELD) sense(SE_FIELD);
    if (s.taxis & TX_WARMTH) sense(SE_WARMTH);
    bool quiet[3];
    int nQuiet = 0;
    for (int q = 0; q < 3; q++) { quiet[q] = unitFromHash(mix64(hashCombine(s.seed ^ SALT_GENE_QUIET, (uint64_t)q))) < 2.0 / 3; nQuiet += quiet[q]; }
    if (nQuiet == 0) quiet[s.seed % 3] = true;
    const int L = c.letters, top = L - 1;
    for (int p = 0; p < GENE_PLACES; p++) {
        const int kind = c.kind[p], slot = c.slot[p];
        bool has = false;
        switch (kind) {
            case GN_VITAL: case GN_WARMTH: case GN_SALT: case GN_SIZE: has = true; break;
            case GN_QUIET: has = quiet[slot]; break;
            case GN_PIGMENT: has = s.pigment != PG_NONE || s.way == LW_LIGHT; break;
            case GN_FOOD: has = s.way == LW_CHEMICAL; break;
            case GN_MOTION: has = s.motion != MO_NONE; break;
            case GN_SENSE: has = slot < nSense; break;
            case GN_REST: has = (G.restCycle & rest) != 0; break;
            case GN_GLOW: has = s.glows; break;
            case GN_SHAPE: has = shapePlan(s.plan); break;
            default: break;
        }
        if (!has) continue;
        MicrobeGene gene;
        gene.place = p; gene.kind = kind; gene.slot = slot;
        gene.length = (int)std::lround((600 + 3400 * unitFromHash(mix64(hashCombine(g.id ^ SALT_GENE_LENGTH, (uint64_t)p)))) *
                                       (0.9 + 0.2 * unitFromHash(mix64(hashCombine(s.seed ^ SALT_GENE_LENGTH, (uint64_t)p)))));
        const uint64_t h = hashCombine(s.seed ^ SALT_GENE_WILD, (uint64_t)p);
        const int n = genePartLetters(kind);
        switch (kind) {
            case GN_WARMTH: {
                double lo, hi;
                cellRange(chemistryCell(g.chemistry), lo, hi);
                gene.part = lettersOfWeight(c, n, (int)std::lround((s.tOpt - lo) / warmthStep(g.chemistry, L)), h);
                break;
            }
            case GN_SALT: {
                const int band = (s.saltLo + s.saltHi + 1) / 2, span = n * top + 1;
                const int w0 = (band * span + 2) / 3, w1 = ((band + 1) * span + 2) / 3 - 1;
                gene.part = lettersOfWeight(c, n, w0 + (int)(mix64(h ^ 1) % (uint64_t)std::max(1, w1 - w0 + 1)), h);
                break;
            }
            case GN_SIZE:   // its cells' width (a filament's or a chain's a cell's, not the whole)
                gene.part = lettersOfWeight(c, n, (int)std::lround(2 * top + 3 * std::log2(std::max(0.01, m.widthUm) / familyRef(s.plan))), h);
                break;
            case GN_PIGMENT: gene.part = runFor(c, c.pigment, s.pigment, 2, h); break;
            case GN_FOOD: gene.part = runFor(c, c.food, s.substrate, 3, h); break;
            case GN_MOTION: {   // a run its table reads as its own kind among its plan's, then its power's four letters at their middle
                int kinds[4];
                const int nk = planMotions(s.plan, kinds);
                std::vector<int> runs;
                for (int i = 0; i < (int)c.motion.size(); i++) { const int v = c.motion[i]; if (v > 0 && nk > 0 && kinds[(v - 1) % nk] == s.motion) runs.push_back(i); }
                gene.part = (runs.empty() ? runLetters(c, 0, 2) : runLetters(c, runs[mix64(h) % runs.size()], 2)) +
                            lettersOfWeight(c, 4, 2 * top - 1 + (int)(mix64(h ^ 2) % 3), h ^ 3);
                break;
            }
            case GN_SENSE: gene.part = runFor(c, c.sense, senses[slot], 2, h); break;
            case GN_REST: case GN_GLOW: gene.part = lettersOfWeight(c, n, n * top / 4 + 1 + (int)(mix64(h ^ 4) % (uint64_t)(n * top - n * top / 4)), h); break;
            case GN_SHAPE: {
                int plan = 0;
                for (int i = 0; i < 4; i++) if (SHAPE_PLANS[i] == s.plan) plan = i;
                gene.part = runFor(c, c.shape, plan, 2, h) + lettersOfWeight(c, 2, (int)std::lround(top + 2 * std::log2(std::max(1.0, m.aspect) / 3)), h ^ 5);
                break;
            }
            default: {   // vital, quiet: their letters the hash's
                Rng r(h);
                for (int i = 0; i < n; i++) gene.part += ABC[r.irange(L)];
                break;
            }
        }
        G.genes.push_back(gene);
    }
    return G;
}

std::string geneEditsText(const std::vector<GeneEdit>& e) {
    std::string s;
    for (const GeneEdit& x : e) s += (s.empty() ? "" : ";") + std::to_string(x.place + 1) + "=" + (x.cut ? std::string("-") : x.part);
    return s;
}
std::vector<GeneEdit> geneEditsOf(const std::string& text) {
    std::vector<GeneEdit> out;
    size_t at = 0;
    while (at < text.size()) {
        size_t end = text.find(';', at);
        if (end == std::string::npos) end = text.size();
        const std::string one = text.substr(at, end - at);
        const size_t eq = one.find('=');
        if (eq != std::string::npos) {
            GeneEdit e;
            e.place = std::atoi(one.c_str()) - 1;
            const std::string v = one.substr(eq + 1);
            e.cut = v == "-";
            if (!e.cut) for (char ch : v) if (std::strchr(ABC, ch) && ch) e.part += ch;
            if (e.place >= 0 && e.place < GENE_PLACES && (e.cut || !e.part.empty())) out.push_back(e);
        }
        at = end + 1;
    }
    return out;
}

void applyEdits(CultureKind& k, Morph& m, MicrobeGenes& g, const std::vector<GeneEdit>& edits) {
    if (!g.valid || edits.empty()) return;
    const Genesis& gen = g.genesis;
    const GeneCode c = geneCodeOf(gen);
    uint32_t senseWas = 0;
    for (const MicrobeGene& G : g.genes) if (G.kind == GN_SENSE && !G.cut) senseWas |= senseBit(geneTable(c, c.sense, G.part, 0, 2));
    bool motion = false, sense = false, reshaped = false;
    std::string motionWas;
    for (const GeneEdit& e : edits) {
        MicrobeGene* G = g.at(e.place);
        if (!G || G->cut) continue;   // a gap, or cut out in its parent: nothing there to change
        if (!e.cut && ((int)e.part.size() != genePartLetters(G->kind) || e.part == G->part)) continue;
        const std::string was = G->part;
        const bool cut = e.cut;
        switch (G->kind) {
            case GN_VITAL: k.dead = true; break;
            case GN_WARMTH:
                if (cut) { const double best = k.s.tOpt; k.s.tMin = std::max(k.s.tMin, best - 4); k.s.tMax = std::min(k.s.tMax, best + 2); }
                else {
                    double lo, hi;
                    cellRange(chemistryCell(k.chemistry), lo, hi);
                    double d = warmthStep(k.chemistry, c.letters) * (geneWeight(c, e.part) - geneWeight(c, was));
                    d = clampd(d, std::min(lo, k.s.tMin) - k.s.tMin, std::max(hi, k.s.tMax) - k.s.tMax);
                    k.s.tMin += d; k.s.tOpt += d; k.s.tMax += d;
                }
                break;
            case GN_SALT:
                if (cut) k.s.saltLo = k.s.saltHi = 0;
                else {
                    const int d = saltBand(c, geneWeight(c, e.part)) - saltBand(c, geneWeight(c, was));
                    int lo = k.s.saltLo + d, hi = k.s.saltHi + d;
                    if (lo < 0) { hi -= lo; lo = 0; }
                    if (hi > 2) { lo -= hi - 2; hi = 2; }
                    k.s.saltLo = clampi(lo, 0, 2); k.s.saltHi = clampi(hi, k.s.saltLo, 2);
                }
                break;
            case GN_SIZE: {
                double f = cut ? 0.5 : std::pow(2.0, (geneWeight(c, e.part) - geneWeight(c, was)) / 3.0);
                double lo, hi;
                familyWidth(k.s.plan, lo, hi);
                if (m.widthUm > 0) f = clampd(f, std::min(1.0, lo / m.widthUm), std::max(1.0, hi / m.widthUm));
                k.bodyUm *= f; m.lengthUm *= f; m.widthUm *= f;
                break;
            }
            case GN_PIGMENT: {
                const int p = cut ? PG_NONE : geneTable(c, c.pigment, e.part, 0, 2), had = k.s.pigment;
                k.s.pigment = m.pigment = p;
                if (p == PG_NONE && had != PG_NONE) m.density = std::min(m.density, 0.15);
                else if (had == PG_NONE && p != PG_NONE) m.density = std::max(m.density, k.s.way == LW_LIGHT ? 0.5 : 0.3);
                break;
            }
            case GN_FOOD: k.s.substrate = cut ? SU_NONE : geneTable(c, c.food, e.part, 0, 3); break;
            case GN_MOTION: motion = true; motionWas = was; break;
            case GN_SENSE: sense = true; break;
            case GN_REST: {
                if (!cut && keeps(c, geneWeight(c, e.part))) { k.s.cycle |= g.restCycle; if (k.s.sporeYears <= 0) k.s.sporeYears = g.sporeYears; }
                else { k.s.cycle &= ~g.restCycle; k.s.sporeYears = 0; }
                m.cycle = k.s.cycle;
                break;
            }
            case GN_GLOW: {
                const int w = geneWeight(c, e.part), w0 = geneWeight(c, was);
                const bool on = !cut && keeps(c, w);
                if (on && k.s.glows) k.quorum = clampd(k.quorum * std::pow(2.0, -(w - w0) / 2.0), 0.005, 0.3);
                else if (on) k.quorum = 0.03;
                k.s.glows = m.glows = on;
                break;
            }
            case GN_SHAPE: {
                const int plan = cut ? MP_SPHERE : SHAPE_PLANS[geneTable(c, c.shape, e.part, 0, 2) & 3];
                if (plan != k.s.plan) {   // its outline drawn again for the new plan (its seed's), its width kept
                    Microbe t = k.s; t.plan = plan;
                    const Morph f = morphOf(t, k.chemistry, gen, 0);
                    m.plan = plan; m.variant = f.variant; m.sym = f.sym; m.n1 = f.n1; m.n2 = f.n2; m.n3 = f.n3;
                    m.aspect = f.aspect; m.bend = f.bend; m.amp = f.amp; m.turns = f.turns; m.wander = f.wander; m.taper = f.taper;
                    m.lengthUm = m.widthUm * m.aspect;
                    k.s.plan = plan; reshaped = true;
                }
                if (!cut && plan != MP_SPHERE) {
                    const double f = std::pow(2.0, (geneWeight(c, e.part, 2, 2) - geneWeight(c, was, 2, 2)) / 2.0);
                    m.aspect = clampd(m.aspect * f, 1.2, 30); m.lengthUm = m.widthUm * m.aspect;
                }
                break;
            }
            default: break;   // quiet
        }
        if (cut) G->cut = true; else G->part = e.part;
    }
    const MicrobeGene* M = nullptr;
    for (const MicrobeGene& G : g.genes) if (G.kind == GN_MOTION) M = &G;
    if (M && (motion || reshaped)) {   // its kind among its plan's (a new plan's own), its pace by its power's letters
        int kinds[4];
        const int nk = planMotions(k.s.plan, kinds), run = M->cut ? 0 : geneTable(c, c.motion, M->part, 0, 2);
        const int kind = run == 0 || nk == 0 ? MO_NONE : kinds[(run - 1) % nk], had = k.s.motion;
        if (kind == MO_NONE) m.speed = 0;
        else {
            if (kind != had || m.speed <= 0) {
                const double b0 = kindSpeed(had, m), b1 = kindSpeed(kind, m);
                m.speed = b1 * (b0 > 0 && m.speed > 0 ? m.speed / b0 : 1);
            }
            if (motion && !motionWas.empty()) m.speed *= std::pow(2.0, (geneWeight(c, M->part, 2, 4) - geneWeight(c, motionWas, 2, 4)) / 4.0);
            const double b = kindSpeed(kind, m);
            m.speed = clampd(m.speed, 0.2 * b, 4 * b);
            if ((kind == MO_RUN_TUMBLE || kind == MO_HELICAL) && m.flagella == 0 && smallCellPlan(k.s.plan) && k.s.plan != MP_CORKSCREW) {
                m.flagella = 1; m.flagPlace = FV_POLAR; m.flagLen = 1.6; m.beatHz = 5;   // a swimmer's flagellum
            }
            if (kind == MO_TWITCHING && m.pili == 0) m.pili = 5;
        }
        k.s.motion = m.motion = kind;
    }
    if (sense) {   // what it turns to: its senses' targets now, what no sense gave kept; away from the light its own
        uint32_t now = 0;
        bool dark = false;
        for (const MicrobeGene& G : g.genes) {
            if (G.kind != GN_SENSE || G.cut) continue;
            const int t = geneTable(c, c.sense, G.part, 0, 2);
            now |= senseBit(t); dark = dark || t == SE_DARK;
        }
        k.s.taxis = m.taxis = (k.s.taxis & ~senseWas) | now;
        k.s.flees = dark && !(now & (TX_LIGHT | TX_FIELD)) ? 1 : 0;
        if ((now & TX_FIELD) && m.magnetosomes == 0) m.magnetosomes = 8;
        if (!(now & TX_FIELD)) m.magnetosomes = 0;
    }
}

void strainGenes(MicrobeGenes& g, const GeneCode& c, const CultureKind& before, int what, double value, uint64_t seed) {
    if (!g.valid) return;
    auto gene = [&](int kind) -> MicrobeGene* { for (MicrobeGene& G : g.genes) if (G.kind == kind && !G.cut) return &G; return nullptr; };
    const uint64_t h = seed ^ SALT_GENE_STRAIN;
    const double step = warmthStep(before.chemistry, c.letters);
    switch (what) {
        case SM_HOT: if (MicrobeGene* G = gene(GN_WARMTH)) stepWeight(c, G->part, 0, 12, std::max(1, (int)std::lround((value - before.s.tMax) / step)), h); break;
        case SM_COLD: if (MicrobeGene* G = gene(GN_WARMTH)) stepWeight(c, G->part, 0, 12, -std::max(1, (int)std::lround((before.s.tMin - value) / step)), h); break;
        case SM_SIZE: if (MicrobeGene* G = gene(GN_SIZE)) stepWeight(c, G->part, 0, 4, 1, h); break;
        case SM_SALT_LO: case SM_SALT_HI:
            if (MicrobeGene* G = gene(GN_SALT)) {
                const int lo = what == SM_SALT_LO ? (int)value : before.s.saltLo, hi = what == SM_SALT_HI ? (int)value : before.s.saltHi;
                const int band = (lo + hi + 1) / 2, was = saltBand(c, geneWeight(c, G->part));
                if (band != was) {
                    const int span = 6 * (c.letters - 1) + 1, w0 = (band * span + 2) / 3, w1 = ((band + 1) * span + 2) / 3 - 1;
                    G->part = lettersOfWeight(c, 6, band > was ? w0 : w1, h);
                }
            }
            break;
        case SM_PIGMENT: if (MicrobeGene* G = gene(GN_PIGMENT)) G->part = runFor(c, c.pigment, (int)value, 2, h); break;
        default: break;
    }
}

std::vector<int> genesDiffer(const MicrobeGenes& a, const MicrobeGenes& b) {
    std::vector<int> out;
    for (const MicrobeGene& G : a.genes) {
        const MicrobeGene* H = b.at(G.place);
        if (H && (H->cut != G.cut || (!G.cut && H->part != G.part))) out.push_back(G.place);
    }
    return out;
}

std::string geneValueWords(const CultureKind& k, const Morph& m, const MicrobeGene& gene, const GeneCode& c) {
    if (gene.cut) return "CUT OUT";
    const Microbe& s = k.s;
    switch (gene.kind) {
        case GN_VITAL: return k.dead ? "CHANGED: IT DOES NOT LIVE" : "A CHANGE KILLS IT";
        case GN_QUIET: return "NOTHING SEEN WHEN IT CHANGED";
        case GN_WARMTH: return words("IT GROWS %.0f TO %.0f C, BEST %.0f", s.tMin, s.tMax, s.tOpt);
        case GN_SALT:
            return s.saltLo == s.saltHi ? std::string("IN ") + CULTURE_SALT_NAMES[s.saltLo]
                                        : std::string(CULTURE_SALT_NAMES[s.saltLo]) + " TO " + CULTURE_SALT_NAMES[s.saltHi];
        case GN_SIZE:
            if (m.lengthUm <= m.widthUm * 1.2) return words(m.widthUm < 10 ? "ITS CELLS %.1f UM ACROSS" : "ITS CELLS %.0f UM ACROSS", m.widthUm);
            return words(m.widthUm < 10 ? "ITS CELLS %.1f UM BY %.1f UM" : "ITS CELLS %.0f UM BY %.0f UM", m.lengthUm, m.widthUm);
        case GN_PIGMENT: return s.pigment == PG_NONE ? std::string("NO PIGMENT") : std::string(PIGMENT_NAMES[s.pigment]);
        case GN_FOOD: return s.substrate == SU_NONE ? std::string("IT BURNS NOTHING") : std::string("IT BURNS ") + SUBSTRATE_NAMES[s.substrate];
        case GN_MOTION: return s.motion == MO_NONE ? std::string("STILL") : std::string(MOTION_NAMES[s.motion]) + words(m.speed < 10 ? ", %.1f UM/S" : ", %.0f UM/S", m.speed);
        case GN_SENSE: {
            const int t = geneTable(c, c.sense, gene.part, 0, 2);
            return t == SE_DARK ? std::string("IT FLEES THE LIGHT") : std::string("IT TURNS TO ") + GENE_SENSE_NAMES[t];
        }
        case GN_REST:
            if (!keeps(c, geneWeight(c, gene.part))) return "IT DIES AT ITS END";
            return (s.cycle & LC_GATHERING) ? "IT GATHERS AND FRUITS AT ITS END" : (largeCellPlan(s.plan) ? "CYSTS AT ITS END" : "SPORES AT ITS END");
        case GN_GLOW: return s.glows ? words("IT GLOWS AT %.1f%% OF ITS CROWD", 100 * k.quorum) : std::string("NO GLOW");
        case GN_SHAPE:
            if (s.plan == MP_SPHERE) return "A SPHERE";
            return std::string(s.plan == MP_ROD ? "A ROD" : (s.plan == MP_CURVED ? "A CURVED ROD" : "A SPIRAL")) + words(", %.1f TIMES AS LONG AS WIDE", m.aspect);
        default: return "";
    }
}
