// Z-05: a culture in a flask (life/cultures.h): a species' rate and crowd in a medium, the stages it passes, two in a flask, the strains; a
// flask's events laid out once as laws over stretches of its clock (`runCulture`), each closed-form, read at any time (`cultureAt`)
#include "life/cultures.h"
#include "core/rng.h"
#include "core/types.h"
#include <algorithm>
#include <cmath>
#include <limits>

const char* const CULTURE_CELL_NAMES[CC_COUNT] = {"WATER", "ACID", "AMMONIA", "CRYOSTAT"};
const char* const CULTURE_LIGHT_NAMES[CL_COUNT] = {"DARK", "WHITE", "RED", "BLUE"};
const char* const CULTURE_ENERGY_NAMES[CE_COUNT] = {"NONE", "HYDROGEN", "SULPHIDE", "IRON", "METHANE", "AMMONIA", "ACETYLENE", "BROTH"};
const char* const CULTURE_SALT_NAMES[3] = {"FRESH", "SEA SALT", "BRINE"};
const char* const GROWTH_FIT_NAMES[GF_COUNT] = {"GROWS", "TOO COLD", "TOO HOT", "THE WRONG SALT", "THE WRONG SOLVENT", "NOTHING IT EATS", "NO LIGHT",
                                               "IT DOES NOT LIVE"};
const char* const CULTURE_STAGE_NAMES[SG_COUNT] = {"EMPTY", "ASLEEP", "WAKING", "IN ITS LAG", "GROWING", "IN COLONIES", "AT ITS CROWD", "STARVING",
                                                   "TOO COLD TO GROW", "GATHERING", "FRUITING", "SPORULATING", "ENCYSTING", "DYING", "DEAD",
                                                   "CYCLING", "OUTGROWN"};
const char* const CULTURE_PAIR_NAMES[PA_COUNT] = {"ALONE", "ONE WINS", "THEY CYCLE", "THEY FEED EACH OTHER", "MIRRORED: THEY IGNORE EACH OTHER"};

namespace {
constexpr uint64_t SALT_CULTURE = 0x11FE57A6E0000020ULL;   // a species' pace in a flask (its time to its crowd, its lag, its waking, its hold)
constexpr uint64_t SALT_STRAIN = 0x11FE57A6E0000021ULL;    // a strain's move
constexpr double INF = std::numeric_limits<double>::infinity();
constexpr double GATHER_S = 120;                           // the gathered amoebae's slug before they rise into their stalks

double h01(uint64_t h) { return unitFromHash(mix64(h)); }

enum Mode { MD_EMPTY = 0, MD_ASLEEP, MD_WAKING, MD_GROW, MD_HOLD, MD_WAIT, MD_DECLINE, MD_DEAD, MD_CYCLE };

// a light's use to a pigment (dark, white, red, blue): chlorophyll's red and blue, rhodopsin's green (poor in red and blue), the carotenoids'
// blue, phycocyanin's red, the black's every light (Kiang et al. 2007)
constexpr double LIGHT_FIT[PG_COUNT][CL_COUNT] = {
    {0, 0.3, 0.2, 0.2}, {0, 1, 0.8, 0.8}, {0, 1, 0.3, 0.4}, {0, 1, 0.25, 0.9}, {0, 1, 0.3, 0.7}, {0, 1, 0.5, 0.8}, {0, 1, 0.9, 0.5}, {0, 1, 1, 1}};
// the crowd an energy feeds, log10 against the broth's (the lithotrophs' small yields: iron's the least, Hedrich et al. 2011)
constexpr double SUBSTRATE_YIELD[SU_COUNT] = {0, -1.0, -1.0, -2.0, -0.5, -1.5, -1.0};

// a species' pace in a flask, from its seed: its time to its crowd at its best (the small cells' 8 to 14 minutes, the large cells' and the
// colonies' longer), its lag, its spores' waking, how long it holds at its crowd, its decline and its stress's, its inoculum and its crowd
struct Traits {
    double bestS = 600, lagS = 60, wakeS = 40, holdS = 600, kd = 0.004, kdStress = 0.03;
    double sigma = 0, sigmaStress = 0;
    int decline = SG_DYING, stressDecline = SG_DYING;
    double inocLog = 3, baseLog = 9, muMax = 0.02, q0 = 0.05;
};
double bestYieldLog(const Microbe& s) {
    switch (s.way) {
        case LW_LIGHT: return -0.5;
        case LW_CHEMICAL: return SUBSTRATE_YIELD[clampi(s.substrate, 0, SU_COUNT - 1)];
        case LW_GRAZER: case LW_PREDATOR: return -1.0;
        default: return 0;
    }
}
Traits traitsOf(const CultureKind& k) {
    const Microbe& s = k.s;
    Traits T;
    auto u = [&](int i) { return h01(hashCombine(s.seed ^ SALT_CULTURE, (uint64_t)i + 1)); };
    const int p = s.plan;
    if (colonyPlan(p)) { T.baseLog = 5.0; T.inocLog = 1.5; T.bestS = 700 + 240 * u(0); }
    else if (largeCellPlan(p)) { T.baseLog = 6.0; T.inocLog = 2.0; T.bestS = 660 + 420 * u(0); }
    else if (p == MP_FILAMENT) { T.baseLog = 8.3; T.inocLog = 2.5; T.bestS = 560 + 360 * u(0); }
    else { T.baseLog = 9.0; T.inocLog = 3.0; T.bestS = 480 + 360 * u(0); }
    if (s.tOpt >= 80) T.bestS *= 0.8;   // the hyperthermophiles' quick lives
    T.lagS = 40 + 60 * u(1);
    T.wakeS = 20 + 40 * u(2);
    T.holdS = 300 + 600 * u(3);
    T.kd = 1.0 / (200 + 200 * u(4));
    T.kdStress = 1.0 / (20 + 15 * u(5));
    if (s.cycle & LC_GATHERING) { T.decline = SG_GATHERING; T.sigma = 0.8; }
    else if ((s.cycle & LC_SPORULATING) && !largeCellPlan(p)) { T.decline = SG_SPORULATING; T.sigma = 0.6; }
    else if (s.cycle & LC_DORMANT) { T.decline = largeCellPlan(p) ? SG_ENCYSTING : SG_SPORULATING; T.sigma = 0.45; }
    T.stressDecline = T.decline == SG_GATHERING ? SG_DYING : T.decline;
    T.sigmaStress = T.decline == SG_GATHERING ? 0 : 0.5 * T.sigma;
    const double best = T.baseLog + bestYieldLog(s);
    T.muMax = std::log(10.0) * std::max(1.0, best - T.inocLog) / std::max(60.0, T.bestS - T.lagS);
    T.q0 = 1.0 / std::expm1(std::max(0.05, T.muMax * T.lagS));   // Baranyi's h0: the lag's work, its time at the best rate
    return T;
}

// a species' rate against its three temperatures: a smooth rise from its lowest to its best, the steep fall above it (the cardinal temperatures:
// Rosso et al. 1993)
double cardinal(double T, double lo, double opt, double hi) {
    if (T <= lo || T >= hi) return 0;
    opt = clampd(opt, lo + 0.25, hi - 0.25);
    if (T <= opt) { const double s = std::sin(0.5 * PI * (T - lo) / (opt - lo)); return s * s; }
    const double x = (T - opt) / (hi - opt);
    return 1 - x * x * x;
}

// Baranyi and Roberts' logistic: the cells after `tau` at `mu` toward the crowd K from N0 with the lag's state q (and q then)
double growN(double N0, double q, double mu, double K, double tau, double* qOut) {
    const double lnQ = std::log(q) + mu * tau;
    if (qOut) *qOut = std::exp(std::min(lnQ, 690.0));
    const double r = lnQ > 30 ? (1 + q) * std::exp(-lnQ) : (1 + q) / (1 + std::exp(lnQ));
    return K / (1 + (K / N0 - 1) * r);
}
// the time that growth takes from N0 to Nt (under K)
double growTau(double N0, double q, double mu, double K, double Nt) {
    if (Nt <= N0) return 0;
    if (Nt >= K || mu <= 0) return INF;
    const double r = (K / Nt - 1) / (K / N0 - 1);
    const double Q = (1 + q) / r - 1;
    return std::log(Q / q) / mu;
}

struct SpState {
    double N = 0, q = 1, Sp = 0, G = 0;
    double hold = 0, wake = 0, gens = 0, declineT0 = -1;
    bool spent = false;   // its crowd's energy used: no growth, no waking, till a fresh broth
};
void startState(SpState& S, const Traits& T, const CultureKind& k, double N) {
    S = SpState();
    if (k.spores && !k.dead) S.Sp = N; else S.N = N;   // Y-02: a line that does not live never wakes: its cells lyse
    S.q = T.q0;
}

// a strain's move at its edge: the highest warmth up three to six degrees, the lowest down as much or the cells a quarter bigger (the cold's
// rule: Atkinson 1994), the salt it was held in taken into its range, a pigment that takes the light it was held under
void strainMove(const CultureKind& k, const Medium& m, int edge, int count, int& what, double& value) {
    const double u = h01(hashCombine(k.s.seed ^ SALT_STRAIN, (uint64_t)count + 1));
    what = SM_NONE; value = 0;
    switch (edge) {
        case EG_HOT: what = SM_HOT; value = k.s.tMax + std::round(3 + 3 * u); break;
        case EG_COLD:
            if (u < 0.5) { what = SM_COLD; value = k.s.tMin - std::round(3 + 6 * u); }
            else { what = SM_SIZE; value = 1.25; }
            break;
        case EG_SALT: what = m.salt < k.s.saltLo ? SM_SALT_LO : SM_SALT_HI; value = m.salt; break;
        case EG_LIGHT: what = SM_PIGMENT; value = m.light == CL_RED ? PG_BLUE_GREEN : (m.light == CL_BLUE ? PG_ORANGE : PG_BLACK); break;
        default: break;
    }
}

// one species' laws over [ta, tb) from its state at ta, alone (its pair's relation in its growth); its state at tb after; the first strain's and
// the spores' waking marked
struct Marks { double* strainT; int* strainWhat; double* strainValue; double* wokeT; int strains; const Medium* m; };
void advance(const Traits& T, const CultureKind& k, const Growth& g, SpState& S, double ta, double tb, std::vector<CultureSeg>& out, const Marks& mk) {
    double t = ta;
    if (S.N > 0 && S.N < CULTURE_EXTINCT) S.N = 0;
    for (int guard = 0; guard < 16 && t < tb; guard++) {
        CultureSeg s;
        s.t0 = t; s.N = S.N; s.q = S.q; s.Sp = S.Sp; s.G = S.G;
        s.K = std::pow(10.0, g.crowdLog); s.fit = g.fit; s.edge = g.edge;
        s.glows = k.s.glows; s.colony = (k.s.cycle & LC_COLONY) != 0; s.quorum = k.quorum;
        double end = tb;
        bool crowd = false;
        if (S.N <= 0) {
            if (S.Sp > 0 && g.fit == GF_GROWS && !S.spent) { s.mode = MD_WAKING; end = std::min(tb, t + std::max(0.0, T.wakeS - S.wake)); }
            else { s.mode = S.Sp > 0 ? MD_ASLEEP : MD_DEAD; S.wake = 0; }
        } else if (g.fit == GF_HOT || g.fit == GF_SALT || g.fit == GF_SOLVENT || g.fit == GF_UNVIABLE) {
            const bool lyses = g.fit == GF_SOLVENT || g.fit == GF_UNVIABLE;   // Y-02: a line that does not live lyses in seconds
            s.mode = MD_DECLINE; s.kd = g.fit == GF_UNVIABLE ? 1.0 : T.kdStress; s.sigma = lyses ? 0 : T.sigmaStress;
            s.stage = lyses ? SG_DYING : T.stressDecline;
            if (S.declineT0 < 0) S.declineT0 = t;
            end = std::min(tb, t + std::log(S.N / CULTURE_EXTINCT) / s.kd);
        } else if (g.fit == GF_COLD) s.mode = MD_WAIT;
        else if (g.fit == GF_GROWS && !S.spent && S.N < 0.9 * s.K) {
            s.mode = MD_GROW; s.mu = g.mu;
            if (g.edge == EG_NONE) S.gens = 0;   // off its edge: nothing pressing it to change
            const double tf = growTau(S.N, S.q, g.mu, s.K, 0.9 * s.K);
            crowd = t + tf <= tb;
            end = std::min(tb, t + tf);
            if (g.edge != EG_NONE && *mk.strainT < 0) {
                const double Ns = S.N * std::pow(2.0, std::max(0.0, STRAIN_GENERATIONS - S.gens));
                const double ts = t + growTau(S.N, S.q, g.mu, s.K, Ns);
                if (ts < end) { *mk.strainT = ts; strainMove(k, *mk.m, g.edge, mk.strains, *mk.strainWhat, *mk.strainValue); }
            }
        } else if (S.hold < T.holdS) {   // at its crowd, or with nothing to eat: its reserves held a while
            s.mode = MD_HOLD; s.stage = g.fit == GF_GROWS ? SG_CROWD : SG_HUNGRY;
            end = std::min(tb, t + (T.holdS - S.hold));
        } else {   // starved
            s.mode = MD_DECLINE; s.kd = T.kd; s.sigma = T.sigma; s.stage = T.decline;
            if (S.declineT0 < 0) S.declineT0 = t;
            end = std::min(tb, t + std::log(S.N / CULTURE_EXTINCT) / s.kd);
        }
        s.declineT0 = S.declineT0;
        out.push_back(s);
        const double tau = end - t;
        switch (s.mode) {
            case MD_WAKING:
                S.wake += tau;
                if (end < tb || S.wake >= T.wakeS) {   // awake: most of them, the rest left asleep
                    S.N = 0.7 * S.Sp; S.Sp *= 0.3; S.q = T.q0; S.wake = 0; S.hold = 0; S.declineT0 = -1;
                    if (*mk.wokeT < 0) *mk.wokeT = end;
                }
                break;
            case MD_GROW: {
                double qn;
                const double Nn = crowd ? 0.9 * s.K : growN(S.N, S.q, g.mu, s.K, tau, &qn);
                if (crowd) growN(S.N, S.q, g.mu, s.K, tau, &qn);
                if (g.edge != EG_NONE) S.gens += std::log2(Nn / S.N);
                S.N = Nn; S.q = qn;
                if (crowd) S.spent = true;
                break;
            }
            case MD_HOLD: S.hold += tau; break;
            case MD_DECLINE: {
                if (end < tb) { S.Sp += s.sigma * S.N; S.G += (1 - s.sigma) * S.N; S.N = 0; break; }   // the last of them
                const double e = std::exp(-s.kd * tau), lost = S.N * (1 - e);
                S.Sp += s.sigma * lost; S.G += (1 - s.sigma) * lost; S.N *= e;
                break;
            }
            default: break;
        }
        t = end;
    }
}

// a segment read at a time: its cells (its spores, its lysed) and its lag's state
void evalSeg(const CultureSeg& s, double t, double& N, double& Sp, double& G, double& q) {
    const double tau = std::max(0.0, t - s.t0);
    N = s.N; Sp = s.Sp; G = s.G; q = s.q;
    switch (s.mode) {
        case MD_GROW: N = growN(s.N, s.q, s.mu, s.K, tau, &q); break;
        case MD_DECLINE: {
            const double e = std::exp(-s.kd * tau), lost = s.N * (1 - e);
            N = s.N * e; Sp = s.Sp + s.sigma * lost; G = s.G + (1 - s.sigma) * lost;
            if (N < CULTURE_EXTINCT) N = 0;
            break;
        }
        case MD_CYCLE: {
            const double w = std::sqrt(s.r * s.m), d = w / (TAU * 4), e = std::exp(-d * tau), c = std::cos(w * tau), sn = std::sin(w * tau);
            if (s.role == 0) N = s.Pstar * std::exp(e * (s.x0 * c - std::sqrt(s.r / s.m) * s.y0 * sn));
            else N = s.Zstar * std::exp(e * (s.y0 * c + std::sqrt(s.m / s.r) * s.x0 * sn));
            break;
        }
        default: break;
    }
}
// the cycle's state (its log deviations) after tau
void cycleAt(const CultureSeg& s, double tau, double& x, double& y) {
    const double w = std::sqrt(s.r * s.m), d = w / (TAU * 4), e = std::exp(-d * tau), c = std::cos(w * tau), sn = std::sin(w * tau);
    x = e * (s.x0 * c - std::sqrt(s.r / s.m) * s.y0 * sn);
    y = e * (s.y0 * c + std::sqrt(s.m / s.r) * s.x0 * sn);
}
const CultureSeg* segAt(const std::vector<CultureSeg>& v, double t) {
    if (v.empty() || t < v[0].t0) return nullptr;
    size_t lo = 0, hi = v.size();
    while (hi - lo > 1) { const size_t mid = (lo + hi) / 2; if (v[mid].t0 <= t) lo = mid; else hi = mid; }
    return &v[lo];
}
}   // namespace

double cardinalRate(double T, double lo, double opt, double hi) { return cardinal(T, lo, opt, hi); }

int cellChemistry(int cell) { return cell == CC_ACID ? CH_ACID : (cell == CC_AMMONIA ? CH_AMMONIA : (cell == CC_CRYOSTAT ? CH_HYDROCARBON : CH_WATER)); }
int chemistryCell(int chemistry) { return chemistry == CH_ACID ? CC_ACID : (chemistry == CH_AMMONIA ? CC_AMMONIA : (chemistry == CH_HYDROCARBON ? CC_CRYOSTAT : CC_WATER)); }
void cellRange(int cell, double& loC, double& hiC) {
    switch (cell) {
        case CC_ACID: loC = -20; hiC = 90; break;
        case CC_AMMONIA: loC = -100; hiC = -30; break;
        case CC_CRYOSTAT: loC = -188; hiC = -168; break;
        default: loC = -20; hiC = 125; break;   // the water's (its brine kept liquid below its freezing; a pressure cell above its boiling for the vents' life)
    }
}
int lightOfStar(int cls) {
    switch (cls) {
        case STAR_RED_DWARF: case STAR_RED_GIANT: case STAR_ORANGE_GIANT: case STAR_CARBON: case STAR_PROTOSTAR: return CL_RED;
        case STAR_BLUE_GIANT: case STAR_BLUE_WHITE: case STAR_WHITE_DWARF: case STAR_NEUTRON: case STAR_PULSAR: case STAR_WOLF_RAYET: case STAR_BLACK_HOLE: return CL_BLUE;
        case STAR_ROGUE: return CL_DARK;
        default: return CL_WHITE;
    }
}

CultureKind cultureKindOf(const Biota& B, int species, double bodyUm) {
    CultureKind k;
    if (species < 0 || species >= (int)B.species.size()) return k;
    k.s = B.species[species];
    k.chemistry = B.chemistry == CH_NONE ? CH_WATER : B.chemistry;
    k.mirrored = B.genesis.mirrored;
    k.bodyUm = std::max(0.2, bodyUm);
    k.spores = k.s.state == SS_DORMANT;
    return k;
}

Medium mediumOfPlace(const Biota& B, int bodyType, int starClass, const Microhabitat& h, double readingC) {
    Medium m;
    m.cell = chemistryCell(B.chemistry);
    double lo, hi;
    cellRange(m.cell, lo, hi);
    m.tempC = clampd(std::round(readingC), lo, hi);
    const int n = h.niche;
    const bool lit = !h.shade && n != NI_VENT && n != NI_ICE && n != NI_ROCK && n != NI_PLUME;
    m.light = lit ? lightOfStar(starClass) : CL_DARK;
    if (n == NI_SPRING || n == NI_VENT) m.energy = CE_SULPHIDE;
    else if (n == NI_PLUME || n == NI_ROCK) m.energy = CE_HYDROGEN;
    else if (bodyType == PT_RUST) m.energy = CE_IRON;
    else if (B.chemistry == CH_AMMONIA) m.energy = CE_METHANE;
    else if (B.chemistry == CH_HYDROCARBON) m.energy = CE_ACETYLENE;
    else m.energy = CE_BROTH;
    m.salt = clampi(h.salt, 0, 2);
    return m;
}

Growth growthIn(const CultureKind& k, const Medium& m, bool fed) {
    Growth g;
    const Microbe& s = k.s;
    if (k.dead) { g.fit = GF_UNVIABLE; return g; }   // Y-02: a vital gene changed
    if (cellChemistry(m.cell) != k.chemistry) { g.fit = GF_SOLVENT; return g; }
    if (m.tempC >= s.tMax) { g.fit = GF_HOT; return g; }
    if (m.tempC <= s.tMin) { g.fit = GF_COLD; return g; }
    if (m.salt < s.saltLo - 1 || m.salt > s.saltHi + 1) { g.fit = GF_SALT; return g; }
    const Traits T = traitsOf(k);
    const double fT = cardinal(m.tempC, s.tMin, s.tOpt, s.tMax);
    const bool saltEdge = m.salt < s.saltLo || m.salt > s.saltHi;
    double fE = 1, yield = 0;
    switch (s.way) {
        case LW_LIGHT:
            if (m.light == CL_DARK) { g.fit = GF_DARK; return g; }
            fE = LIGHT_FIT[clampi(s.pigment, 0, PG_COUNT - 1)][clampi(m.light, 0, CL_COUNT - 1)]; yield = -0.5;
            break;
        case LW_CHEMICAL:
            if (m.energy != s.substrate && !fed) { g.fit = GF_HUNGRY; return g; }
            yield = SUBSTRATE_YIELD[clampi(s.substrate, 0, SU_COUNT - 1)];
            break;
        case LW_DEAD:
            if (m.energy != CE_BROTH && !fed) { g.fit = GF_HUNGRY; return g; }
            break;
        case LW_GRAZER:   // on its prey; alone in broth slowly and few (Tetrahymena's broth)
            if (fed) yield = -1;
            else if (m.energy == CE_BROTH) { fE = 0.3; yield = -2; }
            else { g.fit = GF_HUNGRY; return g; }
            break;
        case LW_PREDATOR:
            if (!fed) { g.fit = GF_HUNGRY; return g; }
            yield = -1;
            break;
        default: g.fit = GF_HUNGRY; return g;   // a protocell: no life of its own to grow
    }
    g.mu = T.muMax * fT * fE * (saltEdge ? 0.3 : 1);
    g.crowdLog = T.baseLog + yield - (saltEdge ? 0.5 : 0);
    if (fT < 0.5) g.edge = m.tempC > s.tOpt ? EG_HOT : EG_COLD;
    else if (saltEdge) g.edge = EG_SALT;
    else if (s.way == LW_LIGHT && fE <= 0.4) g.edge = EG_LIGHT;
    return g;
}

double bestDoublingS(const CultureKind& k) { return std::log(2.0) / traitsOf(k).muMax; }
int cultureDecline(const CultureKind& k) { return traitsOf(k).decline; }

void applyStrain(CultureKind& k, int what, double value) {
    switch (what) {
        case SM_HOT: { const double d = value - k.s.tMax; k.s.tMax = value; k.s.tOpt += 0.5 * d; break; }
        case SM_COLD: { const double d = k.s.tMin - value; k.s.tMin = value; k.s.tOpt -= 0.25 * d; break; }
        case SM_SIZE: k.bodyUm *= value; break;
        case SM_SALT_LO: k.s.saltLo = clampi((int)value, 0, 2); break;
        case SM_SALT_HI: k.s.saltHi = clampi((int)value, 0, 2); break;
        case SM_PIGMENT: k.s.pigment = clampi((int)value, 0, PG_COUNT - 1); break;
        default: break;   // Y-02: an edited line's are its genes' (`applyEdits`)
    }
}

uint32_t stageCycleBit(int stage) {
    switch (stage) {
        case SG_ASLEEP: return LC_DORMANT;
        case SG_WAKING: return LC_WAKING;
        case SG_LAG: case SG_GROWING: case SG_CYCLING: return LC_GROWING | LC_DIVIDING;
        case SG_COLONY: return LC_GROWING | LC_DIVIDING | LC_COLONY;
        case SG_GATHERING: return LC_GATHERING;
        case SG_FRUITING: return LC_FRUITING;
        case SG_SPORULATING: case SG_ENCYSTING: return LC_SPORULATING;
        case SG_DYING: case SG_OUTGROWN: return LC_DYING;
        default: return 0;
    }
}

int pairOf(const CultureKind& a, const CultureKind& b, int& lead) {
    lead = -1;
    if (a.mirrored != b.mirrored) return PA_IGNORE;   // neither can eat what the other is made of
    auto eats = [](const CultureKind& z, const CultureKind& p) {
        if (z.s.way != LW_GRAZER && z.s.way != LW_PREDATOR) return false;
        if (p.bodyUm >= 0.7 * z.bodyUm) return false;
        return z.s.way == LW_PREDATOR || smallCellPlan(p.s.plan) || protocellPlan(p.s.plan);   // a grazer the small cells, a predator any smaller
    };
    if (eats(a, b)) { lead = 0; return PA_CYCLE; }
    if (eats(b, a)) { lead = 1; return PA_CYCLE; }
    auto feeds = [](const CultureKind& f, const CultureKind& d) {   // a light-feeder's leavings a heterotroph's food; a fermenter's hydrogen a hydrogen-eater's
        return (f.s.way == LW_LIGHT && d.s.way == LW_DEAD) || (f.s.way == LW_DEAD && d.s.way == LW_CHEMICAL && d.s.substrate == SU_HYDROGEN);
    };
    if (feeds(a, b)) { lead = 0; return PA_FEED; }
    if (feeds(b, a)) { lead = 1; return PA_FEED; }
    return PA_COMPETE;
}

CultureRun runCulture(const FlaskCulture& c) {
    CultureRun R;
    std::vector<CultureEvent> ev = c.events;
    std::stable_sort(ev.begin(), ev.end(), [](const CultureEvent& a, const CultureEvent& b) { return a.t < b.t; });
    if (ev.empty() || ev[0].kind != CD_START) return R;
    CultureKind K[2] = {c.kind[0], c.kind[1]};
    Traits T[2] = {traitsOf(K[0]), traitsOf(K[1])};
    SpState S[2];
    bool present[2] = {false, false};
    int strains[2] = {0, 0};
    Medium med;
    bool cycling = false, broken = false;
    double cx = 0, cy = 0, Pstar = 0, Zstar = 0;
    auto marks = [&](int k) { return Marks{&R.strainT[k], &R.strainWhat[k], &R.strainValue[k], &R.wokeT[k], strains[k], &med}; };
    for (size_t i = 0; i < ev.size(); i++) {
        const CultureEvent& e = ev[i];
        const double ta = e.t, tb = i + 1 < ev.size() ? ev[i + 1].t : INF;
        switch (e.kind) {
            case CD_START:
                if (i > 0) break;
                med = e.m; present[0] = true; R.n = 1;
                startState(S[0], T[0], K[0], std::pow(10.0, T[0].inocLog));
                break;
            case CD_MEDIUM: {
                const bool fresh = e.m.energy != med.energy || e.m.salt != med.salt || e.m.cell != med.cell;
                med = e.m;
                if (fresh) for (SpState& s : S) { s.spent = false; s.hold = 0; }
                break;
            }
            case CD_FRESH:   // a hundredth carried into new broth: a new lag, its reserves and its crowd's energy new; a cycle begins again
                for (int k = 0; k < 2; k++) {
                    if (!present[k]) continue;
                    SpState& s = S[k];
                    if (cycling) s.N = k == R.lead ? Zstar * std::exp(cy) : Pstar * std::exp(cx);
                    s.N /= 100; s.Sp /= 100; s.G /= 100;
                    if (s.N < CULTURE_EXTINCT) s.N = 0;
                    s.q = T[k].q0; s.spent = false; s.hold = 0; s.declineT0 = -1;
                }
                cycling = false;
                break;
            case CD_ADD:
                if (present[1] || !present[0]) break;
                present[1] = true; R.n = 2; R.tAdd = ta;
                startState(S[1], T[1], K[1], std::pow(10.0, T[1].inocLog));
                R.pair = pairOf(K[0], K[1], R.lead);
                if (R.pair == PA_COMPETE) {   // the one at its crowd keeps it; else the quicker in this medium
                    const Growth g0 = growthIn(K[0], med), g1 = growthIn(K[1], med);
                    R.lead = (S[0].N >= 0.5 * std::pow(10.0, g0.crowdLog) && g0.fit == GF_GROWS) || g0.mu >= g1.mu ? 0 : 1;
                }
                break;
            case CD_STRAIN:
                if (e.who < 0 || e.who > 1 || !present[e.who]) break;
                applyStrain(K[e.who], e.what, e.value);
                T[e.who] = traitsOf(K[e.who]);
                S[e.who].gens = 0; strains[e.who]++;
                R.strainT[e.who] = -1; R.strainWhat[e.who] = 0; R.strainValue[e.who] = 0;
                break;
            default: break;
        }
        if (!present[0]) continue;
        // the stretch: each species' growth as the pair has it
        const bool two = present[1];
        const int lead = R.lead, other = 1 - std::max(0, lead);
        Growth g[2];
        bool capped[2] = {false, false};
        const size_t had[2] = {R.seg[0].size(), R.seg[1].size()};
        for (int k = 0; k < 2; k++) {
            if (!present[k]) continue;
            bool fed = false;
            if (two && R.pair == PA_CYCLE && k == lead) fed = cycling || S[other].N > 0;
            if (two && R.pair == PA_FEED && k != lead) fed = growthIn(K[lead], med).fit == GF_GROWS && S[lead].N > 0;
            g[k] = growthIn(K[k], med, fed);
            if (fed && R.pair == PA_FEED && growthIn(K[k], med).fit == GF_GROWS) { g[k] = growthIn(K[k], med); fed = false; }   // its own food there too
            capped[k] = fed && R.pair == PA_FEED;
        }
        if (two && R.pair == PA_COMPETE) g[1 - lead].crowdLog -= std::log10(2.0);   // the loser's share of the energy
        if (two && R.pair == PA_FEED) g[lead].crowdLog += std::log10(2.0);          // the feeder helped in turn
        if (two && R.pair == PA_CYCLE) {
            if (!cycling) Zstar = (smallCellPlan(K[other].s.plan) ? 1e-3 : 0.03) * std::pow(10.0, g[other].crowdLog);   // a grazer of bacteria a thousandth of them, a protist's predator more
            g[lead].crowdLog = std::log10(Zstar / 0.9);   // the predator's numbers rise to the prey's measure, then they cycle
        }
        double t = ta;
        if (two && R.pair == PA_CYCLE && !cycling && !broken && g[lead].fit == GF_GROWS && g[other].fit == GF_GROWS && S[other].N > 0) {
            // the predator's rise: where it reaches its measure the cycle begins
            SpState probe = S[lead];
            std::vector<CultureSeg> tmp;
            double st = -1, wt = -1; int sw = 0; double sv = 0;
            advance(T[lead], K[lead], g[lead], probe, ta, tb, tmp, Marks{&st, &sw, &sv, &wt, strains[lead], &med});
            double tc = S[lead].N >= Zstar ? ta : -1;   // at its measure already
            for (const CultureSeg& s : tmp) if (tc < 0 && s.mode == MD_GROW) { const double tf = s.t0 + growTau(s.N, s.q, s.mu, s.K, 0.9 * s.K); if (tf < tb) tc = tf; }
            if (tc >= ta && tc < tb) {
                for (int k = 0; k < 2; k++) advance(T[k], K[k], g[k], S[k], ta, tc, R.seg[k], marks(k));
                if (S[other].N > 0 && g[other].fit == GF_GROWS) {   // at its measure the predator brings its prey a decade down, and they turn about that
                    Pstar = 0.1 * S[other].N; cx = std::log(10.0);
                    cy = std::log(std::max(S[lead].N, 1e-6) / Zstar);
                    cycling = true; R.cycleT = tc;
                }
                t = tc;
            }
        }
        if (cycling && two) {
            if (g[lead].fit == GF_GROWS && g[other].fit == GF_GROWS && g[other].mu > 0) {
                CultureSeg s;
                s.t0 = t; s.mode = MD_CYCLE; s.x0 = cx; s.y0 = cy;
                s.r = g[other].mu; s.m = 0.3 * std::max(1e-5, g[lead].mu);
                s.Pstar = Pstar; s.Zstar = Zstar; s.stage = SG_CYCLING; s.fit = GF_GROWS;
                if (R.period <= 0) R.period = TAU / std::sqrt(s.r * s.m);
                for (int k = 0; k < 2; k++) {
                    CultureSeg sk = s;
                    sk.role = k == lead ? 1 : 0;
                    sk.N = k == lead ? Zstar * std::exp(cy) : Pstar * std::exp(cx);
                    sk.Sp = S[k].Sp; sk.G = S[k].G;
                    sk.K = k == lead ? 10 * Zstar : 10 * Pstar; sk.glows = K[k].s.glows; sk.quorum = K[k].quorum; sk.edge = g[k].edge;
                    R.seg[k].push_back(sk);
                }
                if (tb < INF) cycleAt(s, tb - t, cx, cy);
                continue;
            }
            // the cycle broken (the prey or its predator cannot grow here): each from its numbers then, the predator hungry
            S[other].N = Pstar * std::exp(cx); S[lead].N = Zstar * std::exp(cy);
            for (int k = 0; k < 2; k++) { S[k].q = 1e9; S[k].spent = false; S[k].hold = 0; }
            cycling = false; broken = true;
            g[lead] = growthIn(K[lead], med, false);
        }
        for (int k = 0; k < 2; k++) if (present[k]) advance(T[k], K[k], g[k], S[k], t, tb, R.seg[k], marks(k));
        for (int k = 0; k < 2; k++) for (size_t j = had[k]; j < R.seg[k].size(); j++) R.seg[k][j].capped = capped[k];
    }
    // one wins: from when the winner holds half its crowd the loser falls (Gause 1934)
    if (R.pair == PA_COMPETE && R.n == 2) {
        const int w = R.lead, l = 1 - w;
        const std::vector<CultureSeg>& v = R.seg[w];
        for (size_t i = 0; i < v.size(); i++) {
            const CultureSeg& s = v[i];
            const double t1 = i + 1 < v.size() ? v[i + 1].t0 : INF;
            if (t1 <= R.tAdd) continue;
            const double from = std::max(s.t0, R.tAdd);
            double N, Sp, G, q;
            evalSeg(s, from, N, Sp, G, q);
            if (N > 0 && N >= 0.5 * s.K) { R.exT = from; break; }
            if (s.mode == MD_GROW) { const double th = s.t0 + growTau(s.N, s.q, s.mu, s.K, 0.5 * s.K); if (th >= from && th < t1) { R.exT = th; break; } }
        }
        R.exRate = 0.15 * traitsOf(K[l]).muMax;
    }
    return R;
}

CultureNow cultureAt(const FlaskCulture& c, const CultureRun& R, double t) {
    (void)c;
    CultureNow now;
    double raw[2] = {0, 0};
    for (int k = 0; k < 2; k++) {
        const CultureSeg* s = segAt(R.seg[k], t);
        if (!s) continue;
        now.n = k + 1;
        CultureOne& o = now.one[k];
        double q;
        evalSeg(*s, t, o.cells, o.spores, o.ghosts, q);
        raw[k] = o.cells;
        o.fit = s->fit; o.edge = s->edge;
        switch (s->mode) {
            case MD_ASLEEP: o.stage = SG_ASLEEP; break;
            case MD_WAKING: o.stage = SG_WAKING; break;
            case MD_DEAD: o.stage = SG_DEAD; break;
            case MD_WAIT: o.stage = SG_WAITING; break;
            case MD_GROW: o.stage = q < 1 ? SG_LAG : (s->colony && o.cells >= 0.01 * s->K ? SG_COLONY : SG_GROWING); break;
            case MD_HOLD: o.stage = s->stage; break;
            case MD_DECLINE:
                o.stage = s->stage;
                if (o.stage == SG_GATHERING && s->declineT0 >= 0 && t - s->declineT0 > GATHER_S) o.stage = SG_FRUITING;
                if (o.cells <= 0) o.stage = o.spores > 0 ? SG_ASLEEP : SG_DEAD;
                break;
            case MD_CYCLE: o.stage = SG_CYCLING; break;
            default: break;
        }
        o.glowing = s->glows && o.cells > 0 && o.cells >= s->quorum * s->K;
    }
    if (R.n == 2 && R.pair == PA_COMPETE && R.exT >= 0 && t > R.exT) {   // the loser outgrown (all it makes the fewer)
        CultureOne& o = now.one[1 - R.lead];
        const double f = std::exp(-R.exRate * (t - R.exT));
        o.cells *= f; o.spores *= f; o.ghosts *= f;
        if (o.cells < CULTURE_EXTINCT) o.cells = 0;
        if (o.cells > 0 && o.stage != SG_DEAD && o.stage != SG_ASLEEP) o.stage = SG_OUTGROWN;
        else if (o.cells <= 0) o.stage = o.spores > 0 ? SG_ASLEEP : SG_DEAD;
    }
    if (R.n == 2 && R.pair == PA_FEED && R.lead >= 0) {   // the fed one held to a share of its feeder
        const CultureSeg* s = segAt(R.seg[1 - R.lead], t);
        CultureOne& o = now.one[1 - R.lead];
        if (s && s->capped) { o.cells = std::min(o.cells, 0.3 * raw[R.lead]); if (o.cells < CULTURE_EXTINCT) o.cells = 0; }
    }
    return now;
}

uint32_t cultureStagesBy(const CultureRun& R, int who, double t) {
    uint32_t bits = 0;
    if (who < 0 || who > 1) return 0;
    const std::vector<CultureSeg>& v = R.seg[who];
    for (size_t i = 0; i < v.size() && v[i].t0 <= t; i++) {
        const CultureSeg& s = v[i];
        const double t1 = std::min(t, i + 1 < v.size() ? v[i + 1].t0 : t);
        switch (s.mode) {
            case MD_ASLEEP: bits |= LC_DORMANT; break;
            case MD_WAKING: bits |= LC_WAKING; break;
            case MD_GROW: {
                double N, Sp, G, q;
                evalSeg(s, t1, N, Sp, G, q);
                if (q >= 1) bits |= LC_GROWING | LC_DIVIDING;
                if (s.colony && N >= 0.01 * s.K) bits |= LC_COLONY;
                break;
            }
            case MD_DECLINE:
                bits |= stageCycleBit(s.stage);
                if (s.stage == SG_GATHERING && s.declineT0 >= 0 && t1 - s.declineT0 > GATHER_S) bits |= LC_FRUITING;
                break;
            case MD_CYCLE: bits |= LC_GROWING | LC_DIVIDING; break;
            default: break;
        }
    }
    return bits;
}
