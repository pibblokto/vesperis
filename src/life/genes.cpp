// Y-02: a species' genes (life/genes.h): a genesis's map and code, a species' genes read from what it is, a line's values from its edits, a
// strain's genes from its move. J-01: the world genes (places 19 to 32), their wild letters read from what a species is, their work in a line
#include "life/genes.h"
#include "life/seeding.h"
#include "core/rng.h"
#include "core/types.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

const char* const GENE_KIND_NAMES[GN_COUNT] = {"VITAL", "QUIET", "WARMTH", "SALT", "SIZE", "PIGMENT", "FOOD", "MOTION", "SENSE", "REST", "GLOW",
                                               "SHAPE", "GAS", "DYE", "LIFT", "FROST", "SLIME", "SHELL", "STOP", "BIND", "ROT", "POISON",
                                               "CARRIER", "HOST", "DRY", "SHIELD"};
const char* const GENE_SENSE_NAMES[SE_COUNT] = {"NOTHING", "LIGHT", "AWAY FROM LIGHT", "FOOD", "THE MAGNET", "WARMTH"};
const char* const GENE_KIND_PURPOSE[GN_COUNT] = {"THE CELL CANNOT LIVE WITHOUT IT", "NOTHING THE LAB CAN SEE", "THE WARMTHS IT GROWS AT",
                                                 "FRESH WATER, SEA OR BRINE", "HOW BIG ITS CELLS ARE", "ITS COLOUR, AND THE LIGHT IT FEEDS ON",
                                                 "WHAT A CHEMICAL EATER LIVES ON", "WHETHER IT MOVES, HOW AND HOW FAST", "WHAT IT SWIMS TOWARD",
                                                 "SPORES OR CYSTS WHEN STARVED, OR DEATH", "WHETHER A CROWD OF IT GLOWS",
                                                 "A SMALL CELL'S SHAPE AND LENGTH", "THE GAS IT GIVES OFF: A WORLD'S AIR",
                                                 "THE DYE IT LEAKS: A WORLD'S WATERS", "GAS BUBBLES: IT FLOATS INTO THE CLOUDS",
                                                 "IT FREEZES WATER: RAIN AND SNOW", "A SLIME COAT: JELLY SEAS, SLIMY ROCKS",
                                                 "A SHELL: MILKY SEAS, STONE, CRUSTS", "WHEN IT STOPS GROWING",
                                                 "ITS CELLS STICK: CRUSTS AND TISSUE", "WHAT IT ROTS", "A POISON: RIVALS AND ANIMALS DIE",
                                                 "IT PASSES ITS GENES TO OTHER LIFE", "IT LIVES ON PLANTS OR ANIMALS",
                                                 "IT LIVES DRIED OUT, AWAY FROM WATER", "IT TAKES ULTRAVIOLET AND RADIATION"};
const double GENE_PRICE[GFROM_COUNT] = {0, 0.03, 0.06, 0.10};

namespace {
// the genes' salts, each decision its own (nothing of generation 17 moves: a species is read, never drawn again)
constexpr uint64_t SALT_GENE_MAP = 0x6E4E500000000001ULL;     // a genesis's map: its places' order
constexpr uint64_t SALT_GENE_WEIGHT = 0x6E4E500000000002ULL;  // its letters' weights
constexpr uint64_t SALT_GENE_TABLE = 0x6E4E500000000003ULL;   // its tables
constexpr uint64_t SALT_GENE_QUIET = 0x6E4E500000000004ULL;   // a species' quiet genes
constexpr uint64_t SALT_GENE_WILD = 0x6E4E500000000005ULL;    // its wild letters
constexpr uint64_t SALT_GENE_LENGTH = 0x6E4E500000000006ULL;  // a gene's letters in all
constexpr uint64_t SALT_GENE_STRAIN = 0x6E4E500000000007ULL;  // a strain's letters stepped
// J-01: the world genes' (places 19 to 32 after the first eighteen's draw; each table on its own salt, so none moves another)
constexpr uint64_t SALT_GENE_WORLD_MAP = 0x6E4E500000000008ULL;   // their places' order
constexpr uint64_t SALT_GENE_GAS = 0x6E4E500000000009ULL;         // the tables: the gas's
constexpr uint64_t SALT_GENE_DYE = 0x6E4E50000000000AULL;
constexpr uint64_t SALT_GENE_SHELL = 0x6E4E50000000000BULL;
constexpr uint64_t SALT_GENE_ROT = 0x6E4E50000000000CULL;
constexpr uint64_t SALT_GENE_POISON = 0x6E4E50000000000DULL;
constexpr uint64_t SALT_GENE_HOST = 0x6E4E50000000000EULL;
constexpr uint64_t SALT_GENE_WORLD_WILD = 0x6E4E50000000000FULL;  // a species' wild world values (its choices among the likely, its amounts)
constexpr uint64_t SALT_GENE_DYE_MORE = 0x6E4E500000000010ULL;    // R-453: the dye table's synonyms that became GREEN and PINK

const char ABC[] = "ACGTZPSB";   // every alphabet is its start (`alphabetOf`)
int letterIndex(char ch) { const char* p = std::strchr(ABC, ch); return p && ch ? (int)(p - ABC) : 0; }

// the map's places: three vital, three quiet, a warmth, a salt, a size, a pigment, a food, a motion, three senses, a rest, a glow, a shape; J-01:
// then the world genes, one each
constexpr int MAP_KINDS[CELL_GENE_PLACES] = {GN_VITAL, GN_VITAL, GN_VITAL, GN_QUIET, GN_QUIET, GN_QUIET, GN_WARMTH, GN_SALT, GN_SIZE, GN_PIGMENT, GN_FOOD,
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
// R-453: a wild dye's run as J-01 picked it from its table; where that run became GREEN or PINK, the next of its runs that still reads it
std::string runForKept(const GeneCode& c, const std::vector<int>& was, const std::vector<int>& now, int value, uint64_t h) {
    std::vector<int> runs;
    for (int i = 0; i < (int)was.size(); i++) if (was[i] == value) runs.push_back(i);
    if (runs.empty()) return runLetters(c, 0, 2);
    const size_t at = mix64(h) % runs.size();
    for (size_t k = 0; k < runs.size(); k++) {
        const int r = runs[(at + k) % runs.size()];
        if (r < (int)now.size() && now[r] == value) return runLetters(c, r, 2);
    }
    return runLetters(c, runs[at], 2);
}
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

// Y-03: a rest put in a line that had none: its donor's way where its family's (spores or cysts, waking), the gatherers' only where it gathers,
// else its family's own (a large cell's cysts, a small one's spores: the dormant bit reads as either)
uint32_t restBitsFor(const CultureKind& k, uint32_t donor) {
    uint32_t b = donor & (LC_DORMANT | LC_SPORULATING | LC_WAKING);
    if (k.s.plan == MP_GATHERING) b |= donor & (LC_GATHERING | LC_FRUITING);
    if (!(b & (LC_DORMANT | LC_SPORULATING | LC_GATHERING))) b |= LC_DORMANT | LC_WAKING;
    return b;
}
// Y-03: a moved gene's value its donor's where it is an amount (its warmth's range, its salt's band, its cells' width in its family's, its pace's
// share of its kind's, its glow and its crowd, its shape's length, a rest's way where it had none); a table's choice is its letters' (after it)
void donorValue(CultureKind& k, Morph& m, MicrobeGenes& g, const GeneEdit& e, double& paceShare, double& aspect) {
    const CultureKind& D = e.donorKind;
    switch (e.donorGene.kind) {
        case GN_WARMTH: k.s.tMin = D.s.tMin; k.s.tOpt = D.s.tOpt; k.s.tMax = D.s.tMax; break;
        case GN_SALT: k.s.saltLo = D.s.saltLo; k.s.saltHi = D.s.saltHi; break;
        case GN_SIZE:
            if (m.widthUm > 0 && e.donorForm.widthUm > 0) {
                double lo, hi;
                familyWidth(k.s.plan, lo, hi);
                const double f = clampd(e.donorForm.widthUm / m.widthUm, std::min(1.0, lo / m.widthUm), std::max(1.0, hi / m.widthUm));
                k.bodyUm *= f; m.lengthUm *= f; m.widthUm *= f;
            }
            break;
        case GN_MOTION: {
            const double b = kindSpeed(D.s.motion, e.donorForm);
            paceShare = b > 0 && e.donorForm.speed > 0 ? e.donorForm.speed / b : 1;
            break;
        }
        case GN_GLOW: if (D.s.glows) { k.s.glows = m.glows = true; k.quorum = D.quorum; } break;
        case GN_SHAPE: if (e.donorForm.plan != MP_SPHERE && shapePlan(e.donorForm.plan)) aspect = e.donorForm.aspect; break;
        case GN_REST: if (!g.restCycle) { g.restCycle = restBitsFor(k, D.s.cycle); g.sporeYears = D.s.sporeYears > 0 ? D.s.sporeYears : 1e3; } break;
        default: break;   // a pigment, a food, a sense: its letters' choice; a vital or a quiet gene nothing more
    }
}
// Y-03: a gene where none worked, by its letters alone (a copy, a donor not read): a pace at its kind's by its power's weight from their middle,
// a shape's length by its weight from its middle, a rest its family's way
void freshValue(const CultureKind& k, MicrobeGenes& g, const GeneCode& c, int kind, const std::string& part, double& paceShare, double& aspect) {
    const int top = c.letters - 1;
    switch (kind) {
        case GN_MOTION: paceShare = std::pow(2.0, (geneWeight(c, part, 2, 4) - 2 * top) / 4.0); break;
        case GN_SHAPE: aspect = 3 * std::pow(2.0, (geneWeight(c, part, 2, 2) - top) / 2.0); break;
        case GN_REST: if (!g.restCycle) { g.restCycle = restBitsFor(k, 0); g.sporeYears = 1e3; } break;
        default: break;
    }
}
// J-01: the world genes in the map's order after the first eighteen, their places shuffled on a salt of their own
constexpr int WORLD_KINDS[WORLD_GENES] = {GN_GAS, GN_DYE, GN_LIFT, GN_FROST, GN_SLIME, GN_SHELL, GN_STOP, GN_BIND, GN_ROT, GN_POISON, GN_CARRIER,
                                          GN_HOST, GN_DRY, GN_SHIELD};
// a weight's quarter (nothing works below it, `keeps`'s) and the weight an amount is written at: none, at or under the quarter (the hash's);
// else past it by its share of the rest
int quarterOf(const GeneCode& c, int n) { return n * (c.letters - 1) / 4; }
int weightFor(const GeneCode& c, int n, double amount, uint64_t h) {
    const int top = n * (c.letters - 1), q = quarterOf(c, n);
    if (amount <= 0) return (int)(mix64(h) % (uint64_t)(q + 1));
    return clampi(q + std::max(1, (int)std::lround(amount * (top - q))), q + 1, top);
}
double h01(uint64_t h) { return unitFromHash(mix64(h)); }
// what the dye a pigment leaks is (a mat's or a spring's band, a pink lake's)
int dyeOfPigment(int pg) {
    switch (pg) {
        case PG_RED: return WD_RED;
        case PG_ORANGE: return WD_ORANGE;
        case PG_PURPLE: return WD_VIOLET;
        case PG_BLUE_GREEN: return WD_BLUE;
        case PG_BROWN: return WD_YELLOW;
        case PG_BLACK: return WD_BLACK;
        default: return WD_NONE;
    }
}
bool liveIn(const Microbe& s, int niche) { return s.niche == niche || (s.niches & nicheBit(niche)); }
// J-01: a wild species' world value (its table's choice, its amount 0 to 1), read from what it is (IDEAS-seeded-worlds.md section 4): its gas by
// how it lives and what it burns, its dye by its pigment where a pigment leaks (red snow, a spring's band, a pink lake, a red tide), its lift its
// gas vesicles', its frost a snow's, an ice's or a cloud's, its slime its capsule's, its shell its wall's mineral or its crystals, its stop at
// its own crowd, its binding its cells' arrangement, its rot a dead-eater's or a rock-borer's, a hunter's weak poison, a gene-exchanger's
// carrying, no host, its life dried a dry ground's, its shield its radiation's dose (and a dark or a red pigment). A table reading NONE keeps a
// working weight, so one change of its choice shows
void wildWorld(const Microbe& s, const Morph& m, int chemistry, int kind, uint64_t h, int& choice, double& amount) {
    choice = 0; amount = 0;
    const double u = h01(h ^ 0x11), v = h01(h ^ 0x22);
    const bool water = chemistry == CH_WATER, salty = s.saltHi >= 1;
    switch (kind) {
        case GN_GAS:
            switch (s.way) {
                case LW_LIGHT:   // the oxygen-makers (a purple rhodopsin's and a dark pigment's make none); a sea's algae their sulphur gas
                    if (water && s.pigment != PG_PURPLE && s.pigment != PG_BLACK) choice = salty && s.pigment != PG_GREEN && u < 0.4 ? WG_DMS : WG_OXYGEN;
                    break;
                case LW_CHEMICAL:
                    switch (s.substrate) {
                        case SU_HYDROGEN: case SU_ACETYLENE: choice = WG_METHANE; break;   // the methanogens; Titan's (McKay and Smith 2005)
                        case SU_METHANE: choice = WG_CO2; break;
                        case SU_AMMONIA: choice = WG_N2O; break;                           // the nitrifiers' leak
                        default: break;
                    }
                    break;
                case LW_DEAD: choice = s.oxygen >= 1 ? WG_CO2 : (u < 0.4 ? WG_H2S : (u < 0.7 ? WG_HYDROGEN : WG_CO2)); break;   // the anaerobes' sulphate, their fermenting
                case LW_GRAZER: case LW_PREDATOR: choice = WG_CO2; break;
                default: break;
            }
            amount = choice ? 0.35 + 0.45 * v : 0.3 + 0.3 * v;
            break;
        case GN_DYE:
            if (liveIn(s, NI_SNOW) && s.pigment == PG_RED) choice = WD_RED;                                   // the red snow's alga
            else if (s.niche == NI_SPRING || s.niche == NI_MAT) choice = dyeOfPigment(s.pigment);             // the springs' bands
            else if (s.niche == NI_SALT && (s.pigment == PG_RED || s.pigment == PG_ORANGE || s.pigment == PG_PURPLE)) choice = WD_CRIMSON;
            else if (s.niche == NI_LIQUID && s.way == LW_LIGHT && s.pigment == PG_RED && salty && u < 0.3) choice = WD_RED;   // a red tide
            amount = choice ? 0.4 + 0.4 * v : 0.3 + 0.3 * v;
            break;
        case GN_LIFT: amount = m.gas > 0 ? clampd(m.gas / 0.9, 0.05, 1) : 0; break;
        case GN_FROST:   // a snow's, an ice's or a cloud's own (a third of those that only visit them: the nucleators are few, Christner et al. 2008), of
            // the melt only: one at its best below freezing keeps its water liquid instead (antifreeze, not ice seeds), never freezing its own flask
            if (!water || s.tOpt < 0) break;
            if (s.niche == NI_SNOW || s.niche == NI_ICE || s.niche == NI_CLOUD) amount = 0.3 + 0.6 * v;
            else if ((liveIn(s, NI_SNOW) || liveIn(s, NI_ICE) || liveIn(s, NI_CLOUD)) && u < 0.33) amount = 0.2 + 0.5 * v;
            break;
        case GN_SLIME: amount = m.capsule > 0 ? clampd(m.capsule / 1.4, 0.05, 1) : 0; break;
        case GN_SHELL: {
            const bool walled = s.plan == MP_SHELLED || s.plan == MP_TEST || s.plan == MP_LATTICE || s.plan == MP_CHAMBERED || s.plan == MP_ARMOURED ||
                                s.plan == MP_PLATE;
            if (walled && m.mineral == MI_CALCITE) choice = SHL_CHALK;
            else if (walled && m.mineral == MI_SILICA) choice = SHL_GLASS;
            else if (m.magnetosomes > 0 || (s.way == LW_CHEMICAL && s.substrate == SU_IRON) || (walled && m.mineral == MI_IRON)) choice = SHL_RUST;
            else if (m.droplets > 0 && s.substrate == SU_SULPHIDE) choice = SHL_SULPHUR;
            else if (walled && m.mineral == MI_SULPHUR) choice = SHL_SULPHUR;
            amount = choice ? 0.4 + 0.4 * v : 0.3 + 0.3 * v;
            break;
        }
        case GN_STOP: amount = 0.5; break;   // (written at its middle: its crowd its own)
        case GN_BIND:
            if (colonyPlan(s.plan)) amount = 0.9;
            else if (s.plan == MP_FILAMENT) amount = 0.7;
            else if (m.arrangement == AR_PAIRS) amount = 0.15;
            else if (m.arrangement != AR_SINGLE) amount = 0.5;
            break;
        case GN_ROT:
            if (s.way == LW_DEAD) {
                const bool ground = s.niche == NI_SOIL || s.niche == NI_MAT || s.niche == NI_CRUST || s.niche == NI_RUINS || s.niche == NI_SILT;
                choice = ground ? (u < 0.6 ? ROT_WOOD : (u < 0.85 ? ROT_FLESH : ROT_SHELL)) : (u < 0.5 ? ROT_FLESH : (u < 0.8 ? ROT_WOOD : ROT_SHELL));
            } else if (s.niche == NI_ROCK && s.way == LW_CHEMICAL) choice = ROT_STONE;   // the endoliths' boring
            amount = choice ? 0.4 + 0.4 * v : 0.3 + 0.3 * v;
            break;
        case GN_POISON:
            if (s.way == LW_PREDATOR) choice = u < 0.4 ? PZ_NERVE : (u < 0.7 ? PZ_BLOOD : PZ_GUT);   // a hunter's toxin, too weak to work alone
            else if (s.way == LW_GRAZER && u < 0.3) choice = PZ_GUT;
            amount = choice ? 0 : 0.3 + 0.3 * v;
            break;
        case GN_CARRIER: if (s.cycle & LC_EXCHANGING) amount = 0.4 + 0.4 * v; break;
        case GN_HOST: amount = 0.3 + 0.3 * v; break;
        case GN_DRY:
            if (s.niche == NI_CRUST || s.niche == NI_DUST || s.niche == NI_ROCK || s.niche == NI_RUINS) amount = 0.6 + 0.4 * v;
            else if (s.niche == NI_SOIL || s.niche == NI_SALT) amount = 0.3 + 0.3 * v;
            break;
        case GN_SHIELD: {
            double a = s.doseGy > 0 ? (std::log10(s.doseGy) - 1.5) / 2.7 : 0;
            if (s.pigment == PG_BROWN || s.pigment == PG_BLACK || s.pigment == PG_ORANGE || s.pigment == PG_RED) a += 0.15;   // melanin's, carotenoids'
            amount = a >= 0.05 ? clampd(a, 0.05, 1) : 0;
            break;
        }
        default: break;
    }
}
// J-01: a bound small cell's arrangement (a sphere's clusters, a rod's chains, a square's fours, a stalk's rosettes, the rest in pairs) and how
// many, by its binding; none for a large cell's or a colony's plan (its own)
bool bindsForm(int plan) { return smallCellPlan(plan) && plan != MP_FILAMENT; }
void boundForm(Morph& m, double bind) {
    if (!bindsForm(m.plan)) return;
    if (bind <= 0) { m.arrangement = AR_SINGLE; m.groupN = 1; return; }
    switch (m.plan) {
        case MP_SPHERE: m.arrangement = AR_CLUSTERS; m.groupN = 6 + (int)std::lround(14 * bind); break;
        case MP_ROD: m.arrangement = AR_CHAINS; m.groupN = 3 + (int)std::lround(9 * bind); break;
        case MP_SQUARE: m.arrangement = AR_FOURS; m.groupN = 4; break;
        case MP_STALKED: m.arrangement = AR_ROSETTES; m.groupN = 3 + (int)std::lround(5 * bind); break;
        default: m.arrangement = AR_PAIRS; m.groupN = 2; break;
    }
}
// J-01: a world gene's value from its working part (its table's choice in its first two letters, its amount in its weight's), into its line's
// `CultureWorld`; cut, it does nothing (STOP cut: it never stops). Its crowd's shift is the change of its weight from `was`'s (its parent's), so
// a species' own STOP is its crowd
void worldValue(CultureWorld& w, const GeneCode& c, int kind, const std::string& part, const std::string& was, bool cut) {
    const bool table = worldTable(c, kind) != nullptr;
    const int from = table ? 2 : 0;
    const double a = cut ? 0 : geneAmount(c, part, from, 4);
    const int ch = cut || !table ? 0 : geneTable(c, *worldTable(c, kind), part, 0, 2);
    switch (kind) {
        case GN_GAS: w.gas = ch; w.gasRate = a; break;
        case GN_DYE: w.dye = ch; w.dyeRate = a; break;
        case GN_LIFT: w.lift = a; break;
        case GN_FROST: w.frostC = a > 0 ? -12 + 10 * a : FROST_NONE; break;   // a strong nucleator's ice at -2 C, a weak one's at -12
        case GN_SLIME: w.slime = a; break;
        case GN_SHELL: w.shell = ch; w.shellRate = a; break;
        case GN_STOP:
            w.overgrows = cut;
            if (cut) break;
            if (!was.empty()) {
                const int top = 4 * (c.letters - 1);
                w.stopShift = clampd(w.stopShift + 2.0 * (geneWeight(c, part) - geneWeight(c, was)) / top, -1.5, 1.0);
            }
            break;
        case GN_BIND: w.bind = a; break;
        case GN_ROT: w.rot = ch; w.rotRate = a; break;
        case GN_POISON: w.poison = ch; w.poisonRate = a; break;
        case GN_CARRIER: w.carrier = a; break;
        case GN_HOST: w.host = ch; w.hostRate = a; break;
        case GN_DRY: w.dry = a; break;
        case GN_SHIELD: w.shield = a; break;
        default: break;
    }
}
}   // namespace

int genePartLetters(int kind) {
    switch (kind) {
        case GN_WARMTH: return 12;
        case GN_SALT: case GN_MOTION: return 6;
        case GN_SIZE: case GN_REST: case GN_GLOW: case GN_SHAPE: return 4;
        case GN_FOOD: return 3;
        case GN_PIGMENT: case GN_SENSE: return 2;
        case GN_GAS: case GN_DYE: case GN_SHELL: case GN_ROT: case GN_POISON: case GN_HOST: return 6;   // J-01: a table's two, a weight's four
        case GN_LIFT: case GN_FROST: case GN_SLIME: case GN_STOP: case GN_BIND: case GN_CARRIER: case GN_DRY: case GN_SHIELD: return 4;
        default: return 8;   // vital, quiet
    }
}

const std::vector<int>* worldTable(const GeneCode& c, int kind) {
    switch (kind) {
        case GN_GAS: return &c.gas;
        case GN_DYE: return &c.dye;
        case GN_SHELL: return &c.shell;
        case GN_ROT: return &c.rot;
        case GN_POISON: return &c.poison;
        case GN_HOST: return &c.host;
        default: return nullptr;
    }
}
double geneAmount(const GeneCode& c, const std::string& part, int from, int n) {
    const int top = n * (c.letters - 1), q = quarterOf(c, n), W = geneWeight(c, part, from, n);
    return W > q ? double(W - q) / (top - q) : 0;
}
void worldOf(CultureWorld& w, const MicrobeGenes& g) {
    w = CultureWorld();
    if (!g.valid) return;
    const GeneCode c = geneCodeOf(g.genesis);
    for (const MicrobeGene& G : g.genes) if (worldGene(G.kind)) worldValue(w, c, G.kind, G.part, std::string(), G.cut);
}

double kindPace(int motion, const Morph& m) { return kindSpeed(motion, m); }

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
    {   // the map: its places' order (J-01: the cell's eighteen as Y-02 drew them, then the world's fourteen on their own salt)
        int k[GENE_PLACES];
        for (int i = 0; i < CELL_GENE_PLACES; i++) k[i] = MAP_KINDS[i];
        Rng r(g.id ^ SALT_GENE_MAP);
        for (int i = CELL_GENE_PLACES - 1; i > 0; i--) std::swap(k[i], k[r.irange(i + 1)]);
        for (int i = 0; i < WORLD_GENES; i++) k[CELL_GENE_PLACES + i] = WORLD_KINDS[i];
        Rng w(g.id ^ SALT_GENE_WORLD_MAP);
        for (int i = WORLD_GENES - 1; i > 0; i--) std::swap(k[CELL_GENE_PLACES + i], k[CELL_GENE_PLACES + w.irange(i + 1)]);
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
    // J-01: the world genes' tables, each its two letters' runs on its own salt
    auto own = [&](std::vector<int>& t, int values, uint64_t salt) { Rng q(g.id ^ salt); table(t, L * L, values, q); };
    own(c.gas, WG_COUNT, SALT_GENE_GAS); own(c.dye, WD_FLUORESCENT + 1, SALT_GENE_DYE); own(c.shell, SHL_COUNT, SALT_GENE_SHELL);
    c.dyeJ01 = c.dye;
    {   // R-453: the dye's table as J-01 drew it, then two of its synonyms (a run whose dye another run also gives) made GREEN and PINK, so every
        // run that gave a dye before gives it still but those two
        Rng q(g.id ^ SALT_GENE_DYE_MORE);
        for (int add = WD_GREEN; add < WD_COUNT; add++) {
            std::vector<int> spare;
            for (int i = 0; i < (int)c.dye.size(); i++) {
                int same = 0;
                for (int v : c.dye) same += v == c.dye[i];
                if (c.dye[i] < WD_GREEN && same > 1) spare.push_back(i);
            }
            if (!spare.empty()) c.dye[spare[q.irange((int)spare.size())]] = add;
        }
    }
    own(c.rot, ROT_COUNT, SALT_GENE_ROT); own(c.poison, PZ_COUNT, SALT_GENE_POISON); own(c.host, HO_COUNT, SALT_GENE_HOST);
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
            default: has = worldGene(kind); break;   // J-01: every species carries the world's fourteen
        }
        if (!has) continue;
        MicrobeGene gene;
        gene.place = p; gene.kind = kind; gene.slot = slot;
        gene.sw = geneOwnSwitch(kind);   // Y-04: its kind's own switch
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
            default: {
                if (worldGene(kind)) {   // J-01: its value as it is (its choice among the table's runs that read it, its amount's weight)
                    int choice = 0; double amount = 0;
                    wildWorld(s, m, g.chemistry, kind, hashCombine(s.seed ^ SALT_GENE_WORLD_WILD, (uint64_t)kind), choice, amount);
                    const std::vector<int>* t = worldTable(c, kind);
                    if (t) gene.part = kind == GN_DYE ? runForKept(c, c.dyeJ01, c.dye, choice, h) : runFor(c, *t, choice, 2, h);   // (R-453)
                    gene.part += lettersOfWeight(c, 4, weightFor(c, 4, amount, h ^ 6), h ^ 7);
                    break;
                }
                Rng r(h);   // vital, quiet: their letters the hash's
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
    for (const GeneEdit& x : e) {
        s += (s.empty() ? "" : ";") + std::to_string(x.place + 1);
        if (x.cut) s += "=-";
        else if (x.how == GX_MOVED) s += "+" + x.part + "@" + x.donor;                       // Y-03: moved in, its donor's key
        else if (x.how == GX_COPY) s += "*" + x.part + "@" + std::to_string(x.from + 1);     // a copy of its gene `from`
        else if (!x.part.empty()) s += "=" + x.part;
        if (!x.cut && x.sw >= 0 && x.sw < GW_COUNT) s += std::string("~") + GENE_SWITCH_KEYS[x.sw];   // Y-04: its switch
    }
    return s;
}
std::vector<GeneEdit> geneEditsOf(const std::string& text) {
    std::vector<GeneEdit> out;
    size_t at = 0;
    while (at < text.size()) {
        size_t end = text.find(';', at);
        if (end == std::string::npos) end = text.size();
        std::string one = text.substr(at, end - at);
        int sw = -1;   // Y-04: a switch after the rest ("~DARK"); an unknown key read as none
        const size_t tilde = one.rfind('~');
        if (tilde != std::string::npos) {
            const std::string key = one.substr(tilde + 1);
            for (int i = 0; i < GW_COUNT; i++) if (key == GENE_SWITCH_KEYS[i]) sw = i;
            one = one.substr(0, tilde);
        }
        const size_t eq = one.find_first_of("=+*");
        if (eq != std::string::npos) {
            GeneEdit e;
            e.place = std::atoi(one.c_str()) - 1;
            const char mark = one[eq];
            std::string v = one.substr(eq + 1), tail;
            if (mark != '=') { const size_t a = v.find('@'); if (a != std::string::npos) { tail = v.substr(a + 1); v = v.substr(0, a); } }
            e.how = mark == '+' ? GX_MOVED : (mark == '*' ? GX_COPY : GX_HAND);
            e.cut = mark == '=' && v == "-";
            if (!e.cut) for (char ch : v) if (std::strchr(ABC, ch) && ch) e.part += ch;
            if (e.how == GX_MOVED) e.donor = tail;
            if (e.how == GX_COPY) e.from = std::atoi(tail.c_str()) - 1;
            if (!e.cut) e.sw = sw;
            const bool ok = e.how == GX_MOVED ? !e.donor.empty() : (e.how == GX_COPY ? e.from >= 0 && e.from < GENE_PLACES && e.from != e.place : true);
            if (e.place >= 0 && e.place < GENE_PLACES && (e.cut || !e.part.empty()) && ok) out.push_back(e);
        } else if (sw >= 0) {   // its switch alone
            GeneEdit e;
            e.place = std::atoi(one.c_str()) - 1; e.sw = sw;
            if (e.place >= 0 && e.place < GENE_PLACES) out.push_back(e);
        }
        at = end + 1;
    }
    return out;
}

namespace {
// Y-04: the work a gene's kind loses off, as its culture reads it
int workOf(int kind) {
    switch (kind) {
        case GN_VITAL: return CX_LIFE;
        case GN_WARMTH: return CX_WARMTH;
        case GN_SALT: return CX_SALT;
        case GN_PIGMENT: return CX_PIGMENT;
        case GN_FOOD: return CX_FOOD;
        case GN_REST: return CX_REST;
        case GN_GLOW: return CX_GLOW;
        case GN_GAS: return CX_GAS;   // J-01
        case GN_DYE: return CX_DYE;
        case GN_LIFT: return CX_LIFT;
        case GN_FROST: return CX_FROST;
        case GN_SLIME: return CX_SLIME;
        case GN_SHELL: return CX_SHELL;
        case GN_STOP: return CX_STOP;
        case GN_BIND: return CX_BIND;
        case GN_ROT: return CX_ROT;
        case GN_POISON: return CX_POISON;
        case GN_CARRIER: return CX_CARRIER;
        case GN_HOST: return CX_HOST;
        case GN_DRY: return CX_DRY;
        case GN_SHIELD: return CX_SHIELD;
        default: return CX_NONE;   // quiet; a motion, a sense, a size, a shape: its drop's alone
    }
}

// a line's values from its edits (`applyEdits` without its switches' list: a line in a state takes its genes off with it, `switchedLine`)
void applyCore(CultureKind& k, Morph& m, MicrobeGenes& g, const std::vector<GeneEdit>& edits) {
    const Genesis& gen = g.genesis;
    const GeneCode c = geneCodeOf(gen);
    double tMinFloor = k.s.tMin;   // (J-13: its lowest warmth before the edits, or a donor's moved in)
    uint32_t senseWas = 0;
    for (const MicrobeGene& G : g.genes) if (G.kind == GN_SENSE && !G.cut) senseWas |= senseBit(geneTable(c, c.sense, G.part, 0, 2));
    bool motion = false, sense = false, reshaped = false;
    std::string motionWas;
    double paceShare = -1, aspect = -1;   // Y-03: a motion's pace put in (a share of its kind's), a shape's length put in
    std::vector<const GeneEdit*> order;   // Y-03: the copies after the rest, so each copies its gene as edited
    for (const GeneEdit& e : edits) if (e.how != GX_COPY) order.push_back(&e);
    for (const GeneEdit& e : edits) if (e.how == GX_COPY) order.push_back(&e);
    for (const GeneEdit* ep : order) {
        const GeneEdit& e = *ep;
        if (e.place < 0 || e.place >= GENE_PLACES) continue;
        MicrobeGene* G = g.at(e.place);
        // Y-03: a gene moved in or copied, its letters its kind's; where none works (a gap, a gene cut out) it is put there
        const bool put = !e.cut && (e.how == GX_MOVED || e.how == GX_COPY) && (int)e.part.size() == genePartLetters(c.kind[e.place]);
        const MicrobeGene* src = e.how == GX_COPY ? g.at(e.from) : (e.read ? &e.donorGene : nullptr);
        if (put && e.how == GX_COPY && (!src || src->cut || src->kind != c.kind[e.place])) continue;   // nothing of its kind to copy there
        bool fresh = false;
        if (put && (!G || G->cut)) {
            if (!G) {
                MicrobeGene n; n.place = e.place; n.kind = c.kind[e.place]; n.slot = c.slot[e.place];
                g.genes.push_back(n);
                std::sort(g.genes.begin(), g.genes.end(), [](const MicrobeGene& x, const MicrobeGene& y) { return x.place < y.place; });
                G = g.at(e.place);
                src = e.how == GX_COPY ? g.at(e.from) : src;   // (the vector moved)
            }
            G->cut = false;
            if (src) G->length = src->length;
            if (G->length <= 0) G->length = (int)std::lround(600 + 3400 * unitFromHash(mix64(hashCombine(gen.id ^ SALT_GENE_LENGTH, (uint64_t)e.place))));
            fresh = true;
        }
        if (!G || G->cut) continue;   // a gap, or cut out in its parent: nothing there to change by hand
        if (e.sw >= 0 && e.sw < GW_COUNT && !e.cut && !put) G->sw = e.sw;   // Y-04: its switch set
        if (!put && !e.cut && ((int)e.part.size() != genePartLetters(G->kind) || e.part == G->part)) continue;
        const std::string own = fresh ? std::string() : G->part;
        std::string was = own;
        if (put) {
            if (e.how == GX_MOVED) {   // whose it was and how far it came, kept down its lines; Y-04: its switch its donor's
                G->from = e.far; G->origin = e.origin; G->sw = e.read ? e.donorGene.sw : geneOwnSwitch(G->kind);
            } else { G->from = src->from; G->origin = src->origin; G->sw = src->sw; }
            if (e.sw >= 0 && e.sw < GW_COUNT) G->sw = e.sw;
            if (e.how == GX_MOVED && e.read) {
                donorValue(k, m, g, e, paceShare, aspect); was = e.donorGene.part;
                if (G->kind == GN_WARMTH) tMinFloor = std::min(tMinFloor, k.s.tMin);   // (J-13: a donor's own cold kept)
            }
            else if (fresh) { freshValue(k, g, c, G->kind, e.part, paceShare, aspect); was = e.part; }
        }
        const bool cut = e.cut;
        switch (G->kind) {
            case GN_VITAL: if (cut || e.part != own) k.dead = true; break;   // (its own letters moved back in: nothing changed)
            case GN_WARMTH:
                if (cut) { const double best = k.s.tOpt; k.s.tMin = std::max(k.s.tMin, best - 4); k.s.tMax = std::min(k.s.tMax, best + 2); }
                else {
                    double lo, hi;
                    cellRange(chemistryCell(k.chemistry), lo, hi, 2);   // (J-13: a brine's coldest; the end's pass lifts a line that is not a brine's antifreeze)
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
                if (!shapePlan(k.s.plan)) break;   // Y-03: a shape shapes only the four small cells' plans (moved into another form: nothing)
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
                    if (aspect > 0) m.aspect = aspect;   // Y-03: its donor's length, or its letters' alone
                    const double f = std::pow(2.0, (geneWeight(c, e.part, 2, 2) - geneWeight(c, was, 2, 2)) / 2.0);
                    m.aspect = clampd(m.aspect * f, 1.2, 30); m.lengthUm = m.widthUm * m.aspect;
                }
                break;
            }
            default:
                if (worldGene(G->kind)) {   // J-01: its value from its letters; a lift's vesicles, a slime's coat and a binding's cells in its form
                    worldValue(k.world, c, G->kind, e.part, was, cut);
                    if (G->kind == GN_LIFT) m.gas = 0.9 * k.world.lift;
                    if (G->kind == GN_SLIME) m.capsule = 1.4 * k.world.slime;
                    if (G->kind == GN_BIND) boundForm(m, k.world.bind);
                }
                break;   // quiet
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
            if (motion && paceShare > 0) m.speed = kindSpeed(kind, m) * paceShare;   // Y-03: put in: its donor's share of its kind's pace
            else if (kind != had || m.speed <= 0) {
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
    // J-13 (R-454): water life grows down to WATER_COLDEST_C; a line that lives in brine and makes no ice of its own down to BRINE_COLDEST_C.
    // Read once every edit is in (its salt, its frost and its warmth in any order); a band edited lower is lifted whole, as far as its own or its
    // donor's was
    if (k.chemistry == CH_WATER) {
        const double floor = std::min(tMinFloor, brineAntifreeze(k) ? BRINE_COLDEST_C : WATER_COLDEST_C);
        if (k.s.tMin < floor) { const double d = floor - k.s.tMin; k.s.tMin += d; k.s.tOpt += d; k.s.tMax += d; }
    }
}
}   // namespace

int geneOwnSwitch(int kind) { return kind == GN_GLOW ? GW_CROWDED : (kind == GN_REST ? GW_STARVING : GW_ALWAYS); }

void applyEdits(CultureKind& k, Morph& m, MicrobeGenes& g, const std::vector<GeneEdit>& edits) {
    if (!g.valid || edits.empty()) return;
    applyCore(k, m, g, edits);
    // Y-04: its switches as its culture reads them, made again from its genes: each switched from its own to a condition or to NEVER (ALWAYS
    // works as a rest's own), its glow lit by its switch where that is not its crowd
    k.switches.clear(); k.glowAny = false;
    for (const MicrobeGene& G : g.genes) {
        if (G.cut) continue;
        if (G.kind == GN_GLOW && G.sw != GW_CROWDED && k.s.glows) k.glowAny = true;
        if (G.sw == GW_ALWAYS || G.sw == geneOwnSwitch(G.kind)) continue;
        CultureSwitch w; w.place = G.place; w.when = G.sw; w.work = workOf(G.kind);
        if (G.kind == GN_REST) w.bits = g.restCycle;
        k.switches.push_back(w);
    }
}

void switchedLine(CultureKind& k, Morph& m, const MicrobeGenes& g, uint32_t state) {
    std::vector<GeneEdit> off;
    double price = 0;
    for (const CultureSwitch& w : k.switches)
        if (!switchOn(w.when, state)) { GeneEdit e; e.place = w.place; e.cut = true; off.push_back(e); price += w.price; }
    k.switches.clear();
    if (off.empty() || !g.valid) return;
    MicrobeGenes copy = g;
    applyCore(k, m, copy, off);
    k.burden = std::max(0.0, k.burden - price);
}

double genePrice(const CultureKind& wild, const Morph& wildForm, const CultureKind& k, const Morph& m, const MicrobeGene& G) {
    if (G.cut) return 0;
    double b = GENE_PRICE[clampi(G.from, 0, GFROM_COUNT - 1)];
    if (G.kind == GN_MOTION && k.s.motion != MO_NONE && m.speed > 0) {
        if (wild.s.motion == MO_NONE || wildForm.speed <= 0) b += 0.05;
        else if (m.speed > wildForm.speed * (1 + 1e-9)) b += 0.04 * std::log2(m.speed / wildForm.speed);
    }
    if (G.kind == GN_GLOW && k.s.glows) {
        const double q = k.glowAny ? 0.005 : k.quorum;   // Y-04: lit by its switch, as at its least crowd
        if (!wild.s.glows) b += 0.04;
        else if (q < wild.quorum * (1 - 1e-9)) b += 0.02 * std::log2(wild.quorum / q);
    }
    // J-01: a product its species does not make (a gas, a dye, a slime, a shell, a poison) 2% and up to 4% more by its amount; more of its own,
    // up to 4% by how much more
    auto product = [](bool works, bool wildWorks, bool same, double a, double a0) {
        if (!works) return 0.0;
        if (!wildWorks || !same) return 0.02 + 0.04 * a;
        return a > a0 + 1e-9 ? 0.04 * (a - a0) : 0.0;
    };
    const CultureWorld &w = k.world, &w0 = wild.world;
    switch (G.kind) {
        case GN_GAS: b += product(w.gives(), w0.gives(), w.gas == w0.gas, w.gasRate, w0.gasRate); break;
        case GN_DYE: b += product(w.dyes(), w0.dyes(), w.dye == w0.dye, w.dyeRate, w0.dyeRate); break;
        case GN_SLIME: b += product(w.slime > 0, w0.slime > 0, true, w.slime, w0.slime); break;
        case GN_SHELL: b += product(w.shells(), w0.shells(), w.shell == w0.shell, w.shellRate, w0.shellRate); break;
        case GN_POISON: b += product(w.poisons(), w0.poisons(), w.poison == w0.poison, w.poisonRate, w0.poisonRate); break;
        default: break;
    }
    return b;
}
double geneBurden(const CultureKind& wild, const Morph& wildForm, const CultureKind& k, const Morph& m, const MicrobeGenes& g) {
    double b = 0;
    for (const MicrobeGene& G : g.genes) b += genePrice(wild, wildForm, k, m, G);
    return std::max(0.0, b);
}

int geneNeeds(const CultureKind& k, const MicrobeGene& gene) {
    if (gene.cut) return NEED_NONE;
    int kinds[4];
    switch (gene.kind) {
        case GN_SENSE: return k.s.motion == MO_NONE ? NEED_MOTION : NEED_NONE;
        case GN_FOOD: return k.s.way != LW_CHEMICAL ? NEED_EATER : NEED_NONE;
        case GN_PIGMENT: return k.s.way != LW_LIGHT ? NEED_LIGHT : NEED_NONE;
        case GN_SHAPE: return !shapePlan(k.s.plan) ? NEED_SHAPE : NEED_NONE;
        case GN_MOTION: return planMotions(k.s.plan, kinds) == 0 ? NEED_MOVER : NEED_NONE;
        default: return NEED_NONE;
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
        if (H && (H->cut != G.cut || (!G.cut && (H->part != G.part || H->sw != G.sw)))) out.push_back(G.place);   // (Y-04: or its switch)
    }
    return out;
}

int geneChoiceLetters(const GeneCode& c, int kind) {
    switch (kind) {
        case GN_FOOD: return 3;
        case GN_PIGMENT: case GN_SENSE: case GN_MOTION: case GN_SHAPE: return 2;
        default: return worldTable(c, kind) ? 2 : 0;
    }
}

std::string geneLettersWords(const GeneCode& c, int kind) {
    switch (kind) {
        case GN_VITAL: return "ANY CHANGE TO ITS LETTERS KILLS THE STRAIN";
        case GN_QUIET: return "ITS LETTERS DO NOTHING: CHANGE THEM FREELY";
        case GN_WARMTH: return "ITS LETTERS' WEIGHTS ADD UP: HEAVIER, WARMER";
        case GN_SALT: return "ITS LETTERS' WEIGHTS ADD UP: HEAVIER, SALTIER";
        case GN_SIZE: return "ITS LETTERS' WEIGHTS ADD UP: HEAVIER, BIGGER";
        case GN_REST: return "WEIGHTS PAST A QUARTER OF THEIR MOST: IT RESTS";
        case GN_GLOW: return "WEIGHTS PAST A QUARTER: IT GLOWS, HEAVIER SOONER";
        case GN_PIGMENT: return "ITS 2 LETTERS PICK THE PIGMENT FROM A LIST";
        case GN_FOOD: return "ITS 3 LETTERS PICK THE FOOD FROM A LIST";
        case GN_SENSE: return "ITS 2 LETTERS PICK WHAT IT TURNS TO";
        case GN_MOTION: return "LETTERS 1-2 PICK HOW IT MOVES, THE REST HOW FAST";
        case GN_SHAPE: return "LETTERS 1-2 PICK THE SHAPE, 3-4 HOW LONG";
        case GN_STOP: return "HEAVIER, A BIGGER CROWD; CUT IT AND IT NEVER STOPS";
        default: break;
    }
    if (worldTable(c, kind)) return "LETTERS 1-2 PICK WHICH, 3-6 HOW MUCH: HEAVIER, MORE";
    return "WEIGHTS PAST A QUARTER OF THEIR MOST: HEAVIER, STRONGER";
}

std::string geneValueShort(const CultureKind& k, const Morph& m, const MicrobeGene& gene, const GeneCode& c) {
    if (gene.cut) return "CUT OUT";
    const Microbe& s = k.s;
    const CultureWorld& w = k.world;
    auto much = [](double a) { return std::string(a < 0.34 ? ", A LITTLE" : (a < 0.67 ? "" : ", MUCH")); };
    auto salt = [](int i) { return std::string(i == 0 ? "FRESH" : (i == 1 ? "SEA" : "BRINE")); };
    switch (gene.kind) {
        case GN_VITAL: return k.dead ? "DIES" : "LIVES";
        case GN_QUIET: return "NO CHANGE";
        case GN_WARMTH: return words("BEST %.0f C", s.tOpt);
        case GN_SALT: return s.saltLo == s.saltHi ? salt(s.saltLo) : salt(s.saltLo) + " TO " + salt(s.saltHi);
        case GN_SIZE: return words(m.widthUm < 10 ? "%.1f UM WIDE" : "%.0f UM WIDE", m.widthUm);
        case GN_PIGMENT: return s.pigment == PG_NONE ? std::string("NO PIGMENT") : std::string(PIGMENT_NAMES[s.pigment]);
        case GN_FOOD: return s.substrate == SU_NONE ? std::string("NOTHING") : std::string(SUBSTRATE_NAMES[s.substrate]);
        case GN_MOTION: return s.motion == MO_NONE ? std::string("STILL") : std::string(MOTION_NAMES[s.motion]) + words(m.speed < 10 ? " %.1f UM/S" : " %.0f UM/S", m.speed);
        case GN_SENSE: {
            const int t = geneTable(c, c.sense, gene.part, 0, 2);
            return t == SE_NONE ? "NOTHING" : (t == SE_DARK ? "FLEES LIGHT" : (t == SE_FIELD ? "MAGNET" : GENE_SENSE_NAMES[t]));
        }
        case GN_REST:
            if (!keeps(c, geneWeight(c, gene.part))) return "DIES";
            return (s.cycle & LC_GATHERING) ? "FRUITS" : (largeCellPlan(s.plan) ? "CYSTS" : "SPORES");
        case GN_GLOW: return !s.glows ? std::string("NO GLOW") : (k.glowAny ? std::string("WHEN SWITCHED") : words("AT %.1f%% FULL", 100 * k.quorum));
        case GN_SHAPE:
            if (s.plan == MP_SPHERE) return "SPHERE";
            return std::string(s.plan == MP_ROD ? "ROD" : (s.plan == MP_CURVED ? "CURVED ROD" : "SPIRAL")) + words(" X%.1f", m.aspect);
        case GN_GAS: return w.gives() ? std::string(WORLD_GAS_NAMES[w.gas]) + much(w.gasRate) : std::string("NO GAS");
        case GN_DYE: return w.dyes() ? std::string(WORLD_DYE_NAMES[w.dye]) + much(w.dyeRate) : std::string("NO DYE");
        case GN_LIFT: return w.lift <= 0 ? "SINKS" : (w.lift < 0.5 ? "FLOATS" : "FLOATS HIGH");
        case GN_FROST: return w.frostC <= FROST_NONE + 1 ? std::string("NO FROST") : words("ICE AT %.0f C", w.frostC);
        case GN_SLIME: return w.slime <= 0 ? "NO SLIME" : (w.slime < 0.5 ? "THIN SLIME" : "THICK SLIME");
        case GN_SHELL: return w.shells() ? std::string(WORLD_SHELL_NAMES[w.shell]) + much(w.shellRate) : std::string("NO SHELL");
        case GN_STOP: return w.overgrows ? std::string("NEVER STOPS") : (std::fabs(w.stopShift) < 0.05 ? std::string("STOPS WHEN FULL") : words("CROWD X%.1f", std::pow(10.0, w.stopShift)));
        case GN_BIND: return w.bind <= 0 ? "CELLS APART" : (w.bind < 0.5 ? "STICK LOOSELY" : "STICK TOGETHER");
        case GN_ROT: return w.rots() ? std::string("ROTS ") + WORLD_ROT_NAMES[w.rot] : std::string("ROTS NOTHING");
        case GN_POISON: return w.poison == PZ_NONE ? std::string("NO POISON") : std::string(WORLD_POISON_NAMES[w.poison]) + (w.poisonRate <= 0 ? ", TOO WEAK" : much(w.poisonRate));
        case GN_CARRIER: return w.carrier <= 0 ? "KEEPS ITS GENES" : (w.carrier < 0.5 ? "NOW AND THEN" : "PASSES GENES");
        case GN_HOST: return w.hosted() ? std::string(WORLD_HOST_NAMES[w.host]) : std::string("LIVES FREE");
        case GN_DRY: return w.dry <= 0 ? "DIES DRY" : (w.dry < 0.34 ? "DRY FOR DAYS" : (w.dry < 0.67 ? "DRY FOR MONTHS" : "DRY FOR YEARS"));
        case GN_SHIELD: return w.shield <= 0 ? "NO SHIELD" : (w.shield < UV_SHIELD ? "A LITTLE UV" : (w.shield < 0.7 ? "TAKES UV" : "STRONG UV"));
        default: return "";
    }
}

std::string geneValueWords(const CultureKind& k, const Morph& m, const MicrobeGene& gene, const GeneCode& c) {
    if (gene.cut) return "CUT OUT";
    const Microbe& s = k.s;
    switch (gene.kind) {
        case GN_VITAL: return k.dead ? "CHANGED: CANNOT LIVE" : "A CHANGE KILLS";
        case GN_QUIET: return "NOTHING SEEN WHEN CHANGED";
        case GN_WARMTH: return words("GROWS %.0f TO %.0f C, BEST %.0f", s.tMin, s.tMax, s.tOpt);
        case GN_SALT:
            return s.saltLo == s.saltHi ? std::string("IN ") + CULTURE_SALT_NAMES[s.saltLo]
                                        : std::string(CULTURE_SALT_NAMES[s.saltLo]) + " TO " + CULTURE_SALT_NAMES[s.saltHi];
        case GN_SIZE:
            if (m.lengthUm <= m.widthUm * 1.2) return words(m.widthUm < 10 ? "CELLS %.1f UM ACROSS" : "CELLS %.0f UM ACROSS", m.widthUm);
            return words(m.widthUm < 10 ? "CELLS %.1f UM BY %.1f UM" : "CELLS %.0f UM BY %.0f UM", m.lengthUm, m.widthUm);
        case GN_PIGMENT: return s.pigment == PG_NONE ? std::string("NO PIGMENT") : std::string(PIGMENT_NAMES[s.pigment]);
        case GN_FOOD: return s.substrate == SU_NONE ? std::string("BURNS NOTHING") : std::string("BURNS ") + SUBSTRATE_NAMES[s.substrate];
        case GN_MOTION: return s.motion == MO_NONE ? std::string("STILL") : std::string(MOTION_NAMES[s.motion]) + words(m.speed < 10 ? ", %.1f UM/S" : ", %.0f UM/S", m.speed);
        case GN_SENSE: {
            const int t = geneTable(c, c.sense, gene.part, 0, 2);
            return t == SE_NONE ? std::string("SENSES NOTHING") : (t == SE_DARK ? std::string("FLEES LIGHT") : (t == SE_FIELD ? std::string("FOLLOWS THE MAGNET")
                                                                                                                        : std::string("TURNS TO ") + GENE_SENSE_NAMES[t]));
        }
        case GN_REST:
            if (!keeps(c, geneWeight(c, gene.part))) return "DIES WHEN STARVED";
            return (s.cycle & LC_GATHERING) ? "GATHERS AND FRUITS WHEN STARVED" : (largeCellPlan(s.plan) ? "CYSTS WHEN STARVED" : "SPORES WHEN STARVED");
        case GN_GLOW:
            if (!s.glows) return "NO GLOW";
            return k.glowAny ? std::string("GLOWS WHEN SWITCHED ON") : words("GLOWS ONCE %.1f%% FULL", 100 * k.quorum);
        case GN_SHAPE:
            if (s.plan == MP_SPHERE) return "A SPHERE";
            return std::string(s.plan == MP_ROD ? "A ROD" : (s.plan == MP_CURVED ? "A CURVED ROD" : "A SPIRAL")) + words(", %.1f TIMES LONGER THAN WIDE", m.aspect);
        default: break;
    }
    // J-01: a world gene's, as its line's world values have it
    const CultureWorld& w = k.world;
    auto much = [](double a, const char* little, const char* some, const char* lot) { return std::string(a < 0.34 ? little : (a < 0.67 ? some : lot)); };
    switch (gene.kind) {
        case GN_GAS: return w.gives() ? "GIVES OFF " + much(w.gasRate, "A LITTLE ", "", "MUCH ") + WORLD_GAS_NAMES[w.gas] : std::string("GIVES OFF NO GAS");
        case GN_DYE: {
            if (!w.dyes()) return "LEAKS NO DYE";
            const std::string a = much(w.dyeRate, "FAINT ", "", "STRONG ");
            return std::string(a.empty() && w.dye == WD_ORANGE ? "LEAKS AN " : "LEAKS A ") + a + WORLD_DYE_NAMES[w.dye] + " DYE";
        }
        case GN_LIFT: return w.lift <= 0 ? "SINKS" : (w.lift < 0.5 ? "FLOATS" : "FLOATS HIGH");
        case GN_FROST: return w.frostC <= FROST_NONE + 1 ? std::string("NO FROST") : words("FREEZES WATER AT %.0f C", w.frostC);
        case GN_SLIME: return w.slime <= 0 ? "NO SLIME" : (w.slime < 0.5 ? "A THIN SLIME COAT" : "A THICK SLIME COAT");
        case GN_SHELL: return w.shells() ? (w.shellRate < 0.34 ? "MAKES A LITTLE " : "MAKES ") + std::string(WORLD_SHELL_NAMES[w.shell]) : std::string("MAKES NO SHELL");
        case GN_STOP:
            if (w.overgrows) {   // J-15: what it overgrows the land as (a carpet or a lichen greens a desert)
                const int t = overgrowthTypeOf(k);
                if (t == OG_NONE) return "NEVER STOPS GROWING";
                return std::string("NEVER STOPS: OVERGROWS AS ") + MADE_GROWTH_NAMES[t] + (t == OG_CARPETS || t == OG_LICHEN ? ", GREENS A DESERT" : "");
            }
            if (std::fabs(w.stopShift) < 0.05) return "STOPS WHEN FULL";
            return w.stopShift > 0 ? words("GROWS TO %.1f TIMES THE USUAL CROWD", std::pow(10.0, w.stopShift))
                                   : words("STOPS AT %.0f%% OF THE USUAL CROWD", 100 * std::pow(10.0, w.stopShift));
        case GN_BIND: return w.bind <= 0 ? "CELLS PART AFTER DIVIDING" : (w.bind < 0.5 ? "CELLS STICK LOOSELY" : "CELLS STICK TOGETHER");
        case GN_ROT: return w.rots() ? std::string("ROTS ") + WORLD_ROT_NAMES[w.rot] : std::string("ROTS NOTHING");
        case GN_POISON:
            if (w.poison == PZ_NONE) return "NO POISON";
            if (w.poisonRate <= 0) return std::string("A ") + WORLD_POISON_NAMES[w.poison] + " POISON, TOO WEAK TO HARM";
            return much(w.poisonRate, "A WEAK ", "A ", "A STRONG ") + WORLD_POISON_NAMES[w.poison] + " POISON";
        case GN_CARRIER: return w.carrier <= 0 ? "KEEPS ITS GENES" : (w.carrier < 0.5 ? "PASSES GENES NOW AND THEN" : "PASSES GENES TO OTHERS");
        case GN_HOST: {
            if (!w.hosted()) return "LIVES FREE";
            const int d = clampi((int)(w.hostRate * 4), 0, 3);
            static const char* const PLANT[4] = {"LIVES ON PLANTS", "LIVES IN PLANTS", "LIVES IN PLANTS' SAP", "LIVES IN PLANTS' CELLS"};
            static const char* const ANIMAL[4] = {"LIVES ON ANIMALS' SKIN", "LIVES IN ANIMALS' GUTS", "LIVES IN ANIMALS' BLOOD", "LIVES IN ANIMALS' CELLS"};
            static const char* const BOTH[4] = {"LIVES ON PLANTS AND ANIMALS", "LIVES IN PLANTS AND ANIMALS", "LIVES IN SAP AND BLOOD",
                                                "LIVES IN PLANT AND ANIMAL CELLS"};
            return w.host == HO_PLANTS ? PLANT[d] : (w.host == HO_ANIMALS ? ANIMAL[d] : BOTH[d]);
        }
        case GN_DRY: return w.dry <= 0 ? "DIES DRIED OUT" : "LIVES DRIED OUT FOR " + much(w.dry, "DAYS", "MONTHS", "YEARS");
        case GN_SHIELD:
            if (w.shield <= 0) return "ULTRAVIOLET KILLS IT";
            return w.shield < UV_SHIELD ? "TAKES A LITTLE ULTRAVIOLET" : (w.shield < 0.7 ? "TAKES ULTRAVIOLET" : "TAKES STRONG ULTRAVIOLET");
        default: return "";
    }
}
