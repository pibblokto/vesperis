// C-07 (2026-10-02): the signal radar's sources (see signals.h).
#include "signals.h"
#include "galaxy/drifters.h"
#include "galaxy/planetmap.h"
#include "ruins.h"
#include "peoples.h"
#include "history.h"
#include "shards.h"
#include "core/rng.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

const char* const SIGNAL_KIND_NAMES[SIG_KIND_COUNT] = {"A PEOPLE'S TRANSMISSION", "A PULSAR", "A COMET OUTGASSING", "A MAGNETOSPHERE", "A DRIFTER"};
const char* const ROGUE_VOICE_NAMES[RV_COUNT] = {"a brown dwarf", "a giant", "a young world", "a metal world", "a small world"};

int rogueVoiceClass(int planetType) {
    switch (planetType) {
        case PT_SUBSTELLAR: return RV_BROWN_DWARF;
        case PT_GASGIANT: return RV_GIANT;
        case PT_TECTONIC: case PT_MOLTEN: return RV_YOUNG;
        case PT_METAL: return RV_METAL;
        default: return RV_SMALL;
    }
}

void rogueVoiceOf(const StarSystem& sys, Signal& s) {
    s.turnS = 0; s.moonS = 0;
    if (sys.bodies.empty()) return;
    s.turnS = std::fabs(sys.bodies[0].rotPeriod);
    double nearest = 1e300;
    for (const Body& m : sys.bodies) if (m.parent == 0 && m.orbitRadiusKm < nearest) { nearest = m.orbitRadiusKm; s.moonS = m.orbitPeriod; }
}

namespace {
// the cells' salt is a design choice like the pinned home star (G-02): of forty tried, C-07's 0xC07F gave the home sector three
// peoples' signals within reach and the two test worlds three each; fifteen of the forty gave home none (the mean 1.8 within
// 200 ly on the arm). A-01 re-pinned it under the radio rule (`scratchpad/a01/salt.cpp`, sixty salts at the new rate, the mean
// 1.85 from home, nine gave home none): this one gives home three peoples of three ages (a modern one at 90 ly, a futuristic
// one at 184, an industrial one at 197) and each test world four, a far-future people among them (C-15 pins it again with the
// transmitters' powers)
constexpr uint64_t SALT_CELL = 0xA01C0717ULL, SALT_PULSAR = 0x9F5A1ULL, SALT_AIR = 0x0A1ULL;
// C-15: the powers' salts, pinned together after a scan of 2,000 pairs (`scratchpad/c15/sim.cpp`): of the 33 that gave home and
// the three C-series test worlds one to three peoples' signals more at every step and home its farthest within 4,500-7,000 ly,
// two kept every step within one to three from the six age worlds as well (50 steps of the ten places); this one puts a far
// future people among home's far signals (4,500 ly at x32). R-413 pinned the weak powers' again, the strong ones' kept (a world's
// second people of its first's age: fewer worlds of two on the air, and home's x18 step added none): of 8,000 tried on the game's
// own scan (`scratchpad/radar/saltscan.cpp`), 33 gave home and the C-series worlds one to three at every step, none every step from
// the six age worlds; this one misses one there (the far-future world's first step adds four)
constexpr uint64_t SALT_POWER = 0xC15A0404ULL, SALT_DEEP = 0xC1500165ULL;
constexpr double POWER_WEAK_LO = 0.005, POWER_WEAK_HI = 0.05, POWER_STRONG_LO = 1, POWER_STRONG_HI = 40, DEEP_MEAN = 1.4;
inline int64_t floorDiv(int64_t a, int64_t b) { int64_t q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) q--; return q; }
inline double logUniform(double u, double lo, double hi) { return lo * std::pow(hi / lo, u); }
double weakPower(int64_t sx, int64_t sy, int64_t sz) { return logUniform(unitFromHash(hash3i(sx, sy, sz, SALT_POWER)), POWER_WEAK_LO, POWER_WEAK_HI); }
}

const int SIGNAL_GAINS[SIGNAL_GAIN_STEPS] = {1, 5, 11, 18, 25, 32};

// a cell holds one candidate with probability 0.306, two with 0.034, three with 0.0034 (a mean of 0.384 a cell: 1,023 cells within
// 200 ly give 390 candidate sectors, 58 of them with a star and 7 of those with a people's world, on the arm's density). A-01: only
// the peoples that had radio transmit (about three in ten), so C-07's rate (a mean of 0.113) went up 3.4 times and about two
// peoples are heard within 200 ly on the arm, as before
int transmitterCell(int64_t cx, int64_t cy, int64_t cz, int64_t* sx, int64_t* sy, int64_t* sz) {
    uint64_t h = hash3i(cx, cy, cz, SALT_CELL);
    double u = unitFromHash(h);
    int n = u < 0.0034 ? 3 : (u < 0.0374 ? 2 : (u < 0.3434 ? 1 : 0));
    for (int k = 0; k < n; k++) {
        uint64_t hk = mix64(h ^ (0x1234ULL * (k + 1)));
        sx[k] = cx * SIGNAL_CELL + (int64_t)(hk % SIGNAL_CELL);
        sy[k] = cy * SIGNAL_CELL + (int64_t)((hk >> 20) % SIGNAL_CELL);
        sz[k] = cz * SIGNAL_CELL + (int64_t)((hk >> 40) % SIGNAL_CELL);
    }
    return n;
}

// C-15: a cell of the strong transmitters holds a Poisson count of mean 1.4 (from the cell's hash through the distribution), each
// at a sector of its own with a power of 1 to 40, log-uniform
int deepCell(int64_t cx, int64_t cy, int64_t cz, int64_t* sx, int64_t* sy, int64_t* sz, double* power) {
    uint64_t h = hash3i(cx, cy, cz, SALT_DEEP);
    double u = unitFromHash(h), p = std::exp(-DEEP_MEAN), c = p;
    int n = 0;
    while (u > c && n < SIGNAL_DEEP_MAX) { n++; p *= DEEP_MEAN / n; c += p; }
    for (int k = 0; k < n; k++) {
        uint64_t hk = mix64(h ^ (0x51ULL * (k + 1)));
        sx[k] = cx * SIGNAL_DEEP_CELL + (int64_t)(hk % SIGNAL_DEEP_CELL);
        sy[k] = cy * SIGNAL_DEEP_CELL + (int64_t)((hk >> 20) % SIGNAL_DEEP_CELL);
        sz[k] = cz * SIGNAL_DEEP_CELL + (int64_t)((hk >> 40) % SIGNAL_DEEP_CELL);
        power[k] = logUniform(unitFromHash(mix64(hk ^ 0x9E37ULL)), POWER_STRONG_LO, POWER_STRONG_HI);
    }
    return n;
}

double transmitterPower(int64_t sx, int64_t sy, int64_t sz) {
    double best = 0;
    int64_t cx[SIGNAL_DEEP_MAX], cy[SIGNAL_DEEP_MAX], cz[SIGNAL_DEEP_MAX]; double pw[SIGNAL_DEEP_MAX];
    int n = transmitterCell(floorDiv(sx, SIGNAL_CELL), floorDiv(sy, SIGNAL_CELL), floorDiv(sz, SIGNAL_CELL), cx, cy, cz);
    for (int k = 0; k < n; k++) if (cx[k] == sx && cy[k] == sy && cz[k] == sz) best = weakPower(sx, sy, sz);
    n = deepCell(floorDiv(sx, SIGNAL_DEEP_CELL), floorDiv(sy, SIGNAL_DEEP_CELL), floorDiv(sz, SIGNAL_DEEP_CELL), cx, cy, cz, pw);
    for (int k = 0; k < n; k++) if (cx[k] == sx && cy[k] == sy && cz[k] == sz) best = std::max(best, pw[k]);
    return best;
}

bool sectorTransmits(int64_t sx, int64_t sy, int64_t sz) { return transmitterPower(sx, sy, sz) > 0; }

double signalReachAtGain(double power, double gain, double reachLy) { return reachLy * std::sqrt(1 + std::max(0.0, gain - 1) * std::max(0.0, power)); }

double signalNeedGain(double distLy, double power, double reachLy) {
    double y = (distLy / reachLy) * (distLy / reachLy) - 1;
    if (y <= 0) return 1;
    return power > 0 ? 1 + y / power : 1e9;
}

double rogueReachAtGain(double power, double gain) { return SIGNAL_ROGUE_REACH_LY * std::sqrt(std::max(0.0, gain - 1) * std::max(0.0, power)); }

double rogueNeedGain(double distLy, double power) { return power > 0 ? 1 + (distLy / SIGNAL_ROGUE_REACH_LY) * (distLy / SIGNAL_ROGUE_REACH_LY) / power : 1e9; }

// the margin over the floor of a far one, (reach / distance)^2, read as C-07 reads the distance (1 - 0.82 sqrt(d / reach) is
// 1 - 0.82 / margin^(1/4)): 0.18 at the edge of its reach, as C-07's signals are at 200 ly. D-02: a drifter's by its own reach
double signalStrengthAtGain(const Signal& s, double gain) {
    if (s.local || (s.needGain <= 1 && s.kind != SIG_ROGUE)) return s.strength;
    double m = s.kind == SIG_ROGUE ? std::max(0.0, gain - 1) * s.power / ((s.distLy / SIGNAL_ROGUE_REACH_LY) * (s.distLy / SIGNAL_ROGUE_REACH_LY))
                                   : (1 + std::max(0.0, gain - 1) * s.power) / ((s.distLy / SIGNAL_REACH_LY) * (s.distLy / SIGNAL_REACH_LY));
    return m < 1 ? 0 : clampd(1 - 0.82 / std::pow(m, 0.25), 0, 1);
}

// the meter: a near or a local signal a little louder with the gain (C-07's strength times gain^0.1: x1.4 at x32), so their side
// lobes with the gain's floor leave the meter room for a far one's rise; a far one its strength at that gain
double signalLevel(const Signal& s, double gain) {
    if (s.local || s.needGain <= 1) return s.strength * std::pow(std::max(1.0, gain), 0.1);
    return signalStrengthAtGain(s, gain);
}

// the receiver's front end: C-07's strength times gain^0.4 (x1 as C-07, x32 four times), so at x32 the peoples within about 165 ly
// and most of a system's own overload it when the beam is on them; a far one is faint by its nature
double signalDrive(const Signal& s, double gain) {
    if (s.local || s.needGain <= 1) return s.strength * std::pow(std::max(1.0, gain), 0.4);
    return 0;
}

bool starSignal(const Star& s, Signal& out) {
    if (!s.valid) return false;
    if (s.cls == STAR_PULSAR) {
        out = Signal(); out.kind = SIG_PULSAR; out.star = s; out.pulseHz = s.pulseHz; out.seed = s.seed; out.pos = s.pos;
        return true;
    }
    StarSystem sys; sys.generate(s);
    const int pw = systemPeoplesWorld(sys);   // P-04: the system's people's world (C-01's kinds first, then a dead world's)
    if (pw >= 0) {
        const Body& b = sys.bodies[pw];
        BodyGen g = BodyGen::make(b);
        int air = peoplesOnAir(g);
        if (!air) return false;   // the system's people's world had no radio: nothing of it is on the air
        out = Signal(); out.kind = SIG_PEOPLE; out.star = s; out.body = b.index; out.bodyType = b.type; out.seed = b.seed; out.pos = s.pos;
        for (int k = 0; k < 2; k++) if (air & (1 << k)) {
            PeopleAge pa = peopleAgeOf(g, k); out.airYears = std::max(out.airYears, pa.endedYearsAgo + (pa.lastYear - pa.radioYear));
            if (out.age < 0) out.age = pa.age;   // the first people on the air
        }
        return true;
    }
    return false;
}

double signalStrengthAt(double distLy, double reachLy) {
    double u = clampd(distLy / std::max(reachLy, 1e-9), 0, 1);
    return 1.0 - 0.82 * std::sqrt(u);
}

int signalsNear(const Vec3& obsKm, std::vector<Signal>& out, double reachLy, double gain) {
    out.clear();
    int64_t ox = sectorOf(obsKm.x), oy = sectorOf(obsKm.y), oz = sectorOf(obsKm.z);
    int64_t R = (int64_t)std::ceil(reachLy);
    int cells = 0;
    double reachKm = reachLy * SECTOR_KM;
    auto near = [&](int64_t x, int64_t y, int64_t z) {   // every transmitter within the reach, whatever its power (C-07's list)
        Star s; if (!starInSector(x, y, z, s, false)) return;
        double d = length(s.pos - obsKm);
        if (d > reachKm || d < 1.0) return;
        for (const Signal& o : out) if (o.star.seed == s.seed) return;   // a sector of both populations
        Signal sig; if (!starSignal(s, sig)) return;
        sig.distLy = d / SECTOR_KM; sig.strength = signalStrengthAt(sig.distLy, reachLy);
        sig.power = transmitterPower(x, y, z) * (sig.kind == SIG_PULSAR ? SIGNAL_PULSAR_POWER : 1.0); sig.needGain = 1;
        out.push_back(sig);
    };
    for (int64_t cx = floorDiv(ox - R, SIGNAL_CELL); cx <= floorDiv(ox + R, SIGNAL_CELL); cx++)
    for (int64_t cy = floorDiv(oy - R, SIGNAL_CELL); cy <= floorDiv(oy + R, SIGNAL_CELL); cy++)
    for (int64_t cz = floorDiv(oz - R, SIGNAL_CELL); cz <= floorDiv(oz + R, SIGNAL_CELL); cz++) {
        cells++;
        int64_t sx[SIGNAL_MAX_CANDIDATES], sy[SIGNAL_MAX_CANDIDATES], sz[SIGNAL_MAX_CANDIDATES];
        int n = transmitterCell(cx, cy, cz, sx, sy, sz);
        for (int k = 0; k < n; k++) near(sx[k], sy[k], sz[k]);
    }
    for (int64_t cx = floorDiv(ox - R, SIGNAL_DEEP_CELL); cx <= floorDiv(ox + R, SIGNAL_DEEP_CELL); cx++)   // the strong ones within the reach too
    for (int64_t cy = floorDiv(oy - R, SIGNAL_DEEP_CELL); cy <= floorDiv(oy + R, SIGNAL_DEEP_CELL); cy++)
    for (int64_t cz = floorDiv(oz - R, SIGNAL_DEEP_CELL); cz <= floorDiv(oz + R, SIGNAL_DEEP_CELL); cz++) {
        cells++;
        int64_t sx[SIGNAL_DEEP_MAX], sy[SIGNAL_DEEP_MAX], sz[SIGNAL_DEEP_MAX]; double pw[SIGNAL_DEEP_MAX];
        int n = deepCell(cx, cy, cz, sx, sy, sz, pw);
        for (int k = 0; k < n; k++) near(sx[k], sy[k], sz[k]);
    }
    // the loud pulsars near: a share of those within their own reach, by their own hash
    int64_t P = (int64_t)std::ceil(SIGNAL_PULSAR_REACH_LY);
    double pReachKm = SIGNAL_PULSAR_REACH_LY * SECTOR_KM;
    for (int64_t x = ox - P; x <= ox + P; x++) for (int64_t y = oy - P; y <= oy + P; y++) for (int64_t z = oz - P; z <= oz + P; z++) {
        if (unitFromHash(hash3i(x, y, z, SALT_PULSAR)) >= SIGNAL_PULSAR_SHARE) continue;
        Star s; if (!starInSector(x, y, z, s, false) || s.cls != STAR_PULSAR) continue;
        double d = length(s.pos - obsKm);
        if (d > pReachKm || d < 1.0) continue;
        bool have = false; for (const Signal& o : out) if (o.star.seed == s.seed) have = true;
        if (have) continue;
        Signal sig; if (!starSignal(s, sig)) continue;
        sig.distLy = d / SECTOR_KM; sig.strength = signalStrengthAt(sig.distLy, SIGNAL_PULSAR_REACH_LY) * 0.8;
        out.push_back(sig);
    }
    if (gain > 1) {   // what the gain carries from beyond the reach, the whole scan at once (the radar spreads it over frames)
        SignalScan scan; scan.begin(obsKm, gain, reachLy);
        while (!scan.step(1 << 20)) {}
        cells += scan.cells;
        for (const Signal& s : scan.out) { bool have = false; for (const Signal& o : out) if (o.star.seed == s.star.seed) have = true; if (!have) out.push_back(s); }
    }
    std::sort(out.begin(), out.end(), [](const Signal& a, const Signal& b) { return a.distLy < b.distLy; });
    return cells;
}

// C-15: layer 0 the far parts of C-07's cells (their weak powers carry them to 200 sqrt(1 + 0.05 (G - 1)) ly at most: 319 at
// x32), layer 1 the strong cells (to 200 sqrt(1 + 40 (G - 1)) ly: 7,045 at x32), each cell's candidates read by their own power
// before any star is looked for, so of the 170,000 cells within 7,000 ly a few hundred stars are looked at and fewer generated
void SignalScan::begin(const Vec3& obsKm, double g, double reach) {
    out.clear(); done = false; cells = 0;
    obs = obsKm; gain = g; reachLy = reach;
    o[0] = sectorOf(obsKm.x); o[1] = sectorOf(obsKm.y); o[2] = sectorOf(obsKm.z);
    layer = 0; beginLayer();
}

// D-02: layer 2 the drifters' cells within the reach of the loudest, layer 3 the globular clusters' cells round it (relative to the
// galactic centre, a cluster's own reaching 60 ly past its cell's)
void SignalScan::beginLayer() {
    const int cell = layer == 0 ? SIGNAL_CELL : (layer == 1 ? SIGNAL_DEEP_CELL : DRIFT_CELL);
    layerReach = layer <= 1 ? signalReachAtGain(layer == 0 ? POWER_WEAK_HI : POWER_STRONG_HI, gain, reachLy) : rogueReachAtGain(drifterPowerMax(), gain);
    int64_t R = (int64_t)std::ceil(layerReach);
    total = 1;
    if (layer == 3) {
        const double base[3] = {GALAXY_CENTRE_SX, 0, GALAXY_CENTRE_SZ};
        for (int a = 0; a < 3; a++) {
            lo[a] = (int64_t)std::floor(((double)o[a] - base[a] - R - 60) / GLOBULAR_CELL);
            span[a] = (int64_t)std::floor(((double)o[a] - base[a] + R + 60) / GLOBULAR_CELL) - lo[a] + 1; total *= span[a];
        }
    } else
        for (int a = 0; a < 3; a++) { lo[a] = floorDiv(o[a] - R, cell); span[a] = floorDiv(o[a] + R, cell) - lo[a] + 1; total *= span[a]; }
    if (layer >= 2 && gain <= 1) total = 0;   // nothing of the drifters at x1
    next = 0;
}

void SignalScan::consider(int64_t x, int64_t y, int64_t z, double power) {
    double dx = (double)(x - o[0]), dy = (double)(y - o[1]), dz = (double)(z - o[2]);
    double R = signalReachAtGain(power, gain, reachLy) + 1.8;   // a star lies anywhere in its sector, the observer anywhere in its own
    if (dx * dx + dy * dy + dz * dz > R * R) return;
    Star s; if (!starInSector(x, y, z, s, false)) return;
    double d = length(s.pos - obs) / SECTOR_KM;
    if (d <= reachLy || d > R) return;   // within the reach: the near list's
    Signal sig; if (!starSignal(s, sig)) return;
    if (sig.kind == SIG_PEOPLE && d > sig.airYears) return;   // the light from here left before their first broadcast: nothing to hear
    double pw = transmitterPower(x, y, z) * (sig.kind == SIG_PULSAR ? SIGNAL_PULSAR_POWER : 1.0);
    double need = signalNeedGain(d, pw, reachLy);
    if (need > gain) return;
    for (const Signal& e : out) if (e.star.seed == s.seed) return;   // a sector of both populations, found once
    sig.distLy = d; sig.power = pw; sig.needGain = need; sig.strength = signalStrengthAtGain(sig, gain);
    out.push_back(sig);
}

// D-02: a drifter a cell listed, by its own law: its place, its power, the least gain that hears it
void SignalScan::considerDrifter(int64_t x, int64_t y, int64_t z) {
    double dx = (double)(x - o[0]), dy = (double)(y - o[1]), dz = (double)(z - o[2]);
    const double R = layerReach + 1.8;
    if (dx * dx + dy * dy + dz * dz > R * R) return;
    Star s; drifterAt(x, y, z, s, false);
    const double d = length(s.pos - obs) / SECTOR_KM;
    if (d < 1e-6) return;
    const double pw = drifterPower(s), need = rogueNeedGain(d, pw);
    if (need > gain) return;
    for (const Signal& e : out) if (e.star.seed == s.seed) return;   // a cluster's and a cell's alike, found once
    Signal sig; sig.kind = SIG_ROGUE; sig.star = s; sig.body = 0; sig.bodyType = drifterBody(s).type; sig.seed = s.seed; sig.pos = s.pos;
    sig.distLy = d; sig.power = pw; sig.needGain = need; sig.strength = signalStrengthAtGain(sig, gain);
    { StarSystem sys; sys.generate(s); rogueVoiceOf(sys, sig); }   // its voice's turning and moon
    out.push_back(sig);
}

bool SignalScan::step(int budget) {
    while (!done && budget > 0) {
        if (next >= total) {
            if (layer < 3) { layer++; beginLayer(); continue; }
            done = true;
            std::sort(out.begin(), out.end(), [](const Signal& a, const Signal& b) { return a.distLy < b.distLy; });
            break;
        }
        int64_t i = next++;
        int64_t c[3] = {lo[0] + i % span[0], lo[1] + (i / span[0]) % span[1], lo[2] + i / (span[0] * span[1])};
        budget--; cells++;
        if (layer == 3) {   // D-02: a globular cluster's own drifters, when the cell holds one within the reach
            Vec3 gc; double gr;
            if (!globularInCell(c[0], c[1], c[2], gc, gr) || length(gc - Vec3((double)o[0] + 0.5, (double)o[1] + 0.5, (double)o[2] + 0.5)) > layerReach + gr + 2) continue;
            int64_t sx[DRIFT_GLOBULAR_MAX], sy[DRIFT_GLOBULAR_MAX], sz[DRIFT_GLOBULAR_MAX];
            const int n = drifterGlobular(c[0], c[1], c[2], sx, sy, sz);
            for (int k = 0; k < n; k++) considerDrifter(sx[k], sy[k], sz[k]);
            continue;
        }
        int cell = layer == 0 ? SIGNAL_CELL : (layer == 1 ? SIGNAL_DEEP_CELL : DRIFT_CELL);
        double near2 = 0;   // the cell's nearest point: a cell wholly beyond the layer's reach holds nothing to hear
        for (int a = 0; a < 3; a++) { double lo2 = (double)(c[a] * cell), hi2 = lo2 + cell, q = (double)o[a] + 0.5; double e = q < lo2 ? lo2 - q : (q > hi2 ? q - hi2 : 0); near2 += e * e; }
        if (near2 > (layerReach + 2) * (layerReach + 2)) continue;
        if (layer == 0) {
            int64_t sx[SIGNAL_MAX_CANDIDATES], sy[SIGNAL_MAX_CANDIDATES], sz[SIGNAL_MAX_CANDIDATES];
            int n = transmitterCell(c[0], c[1], c[2], sx, sy, sz);
            for (int k = 0; k < n; k++) consider(sx[k], sy[k], sz[k], weakPower(sx[k], sy[k], sz[k]));
        } else if (layer == 2) {   // the drifters' field
            int64_t sx[DRIFT_CELL_MAX], sy[DRIFT_CELL_MAX], sz[DRIFT_CELL_MAX];
            const int n = drifterCell(c[0], c[1], c[2], sx, sy, sz);
            for (int k = 0; k < n; k++) considerDrifter(sx[k], sy[k], sz[k]);
        } else {
            int64_t sx[SIGNAL_DEEP_MAX], sy[SIGNAL_DEEP_MAX], sz[SIGNAL_DEEP_MAX]; double pw[SIGNAL_DEEP_MAX];
            int n = deepCell(c[0], c[1], c[2], sx, sy, sz, pw);
            for (int k = 0; k < n; k++) consider(sx[k], sy[k], sz[k], pw[k]);
        }
    }
    return done;
}

void localSignals(const StarSystem& sys, const Vec3& obsKm, double t, std::vector<Signal>& out) {
    out.clear();
    if (!sys.valid) return;
    const int pw = sectorTransmits(sys.star.sx, sys.star.sy, sys.star.sz) ? systemPeoplesWorld(sys) : -1;   // P-04: the system's people's world, as in `starSignal`
    for (const Body& b : sys.bodies) {
        Vec3 p = sys.bodyPos(b.index, t);
        double d = length(p - obsKm);
        if (d < 1.0) continue;
        if (b.drifter && b.index == 0) {   // the drifter itself, whatever its kind, louder near (as a magnetosphere is, by its power)
            Signal sig; sig.kind = SIG_ROGUE; sig.star = sys.star; sig.body = 0; sig.bodyType = b.type; sig.pos = p; sig.seed = sys.star.seed; sig.local = true;
            sig.distLy = d / SECTOR_KM; sig.power = drifterPower(sys.star);
            sig.strength = clampd(0.45 + 0.25 * std::log10(1 + 10 * sig.power), 0.35, 0.95) * clampd(1.4 - d / (6 * AU_GAME_KM), 0.3, 1.0);
            rogueVoiceOf(sys, sig);
            out.push_back(sig);
            continue;
        }
        bool onAir = false;
        if (b.index == pw) onAir = peoplesOnAir(BodyGen::make(b)) != 0;   // only a people that had radio; the first people's world is the system's
        if (onAir) {   // the people of this very system
            Signal sig; sig.kind = SIG_PEOPLE; sig.star = sys.star; sig.body = b.index; sig.bodyType = b.type; sig.pos = p; sig.seed = b.seed; sig.local = true;
            sig.distLy = d / SECTOR_KM; sig.strength = 1.0;
            { BodyGen g = BodyGen::make(b); sig.age = peopleAgeOf(g, (peoplesOnAir(g) & 1) ? 0 : 1).age; }
            out.push_back(sig);
            continue;
        }
        if (b.type == PT_COMET) {   // outgassing within 1.5 AU of its sun: the louder the nearer the sun, fading with the ship's distance
            double au = length(p - sys.parentPos(b.index, t)) / AU_GAME_KM;
            if (au > 1.5) continue;
            Signal sig; sig.kind = SIG_COMET; sig.star = sys.star; sig.body = b.index; sig.bodyType = b.type; sig.pos = p; sig.seed = b.seed; sig.local = true;
            sig.distLy = d / SECTOR_KM; sig.detail = au;
            sig.strength = clampd((1.0 - au / 1.5) * 0.9, 0.25, 0.9) * clampd(1.3 - d / (4 * AU_GAME_KM), 0.35, 1.0);
            out.push_back(sig);
        } else {   // a giant's always; a world's only when its field is of the strongest and the world big enough to hold a magnetosphere worth the name
            double mag = magneticField(b);
            bool giant = kindHas(b.type, KT_GIANT);
            if (!giant && (mag < 0.8 || b.radiusKm < 2500)) continue;
            Signal sig; sig.kind = SIG_MAGNETOSPHERE; sig.star = sys.star; sig.body = b.index; sig.bodyType = b.type; sig.pos = p; sig.seed = b.seed; sig.local = true;
            sig.distLy = d / SECTOR_KM; sig.detail = giant ? std::max(mag, 0.6) : mag;
            sig.strength = clampd(0.35 + 0.6 * sig.detail, 0.3, 0.95) * clampd(1.4 - d / (6 * AU_GAME_KM), 0.3, 1.0);
            out.push_back(sig);
        }
    }
}

double beamGain(double a) {
    a = std::fabs(a);
    double w = a / (40 * DEG), m = a / (10 * DEG), n = a / (3 * DEG);
    return 0.3 * std::exp(-w * w) + 0.3 * std::exp(-m * m) + 0.4 * std::exp(-n * n);
}

// the broadcast walks the fifty in a stride of the world's own (coprime with fifty, so every recording comes round before any
// repeats), one recording a slot of SIGNAL_RECORDING_S from the game's start; a lock held hears the next in the same walk
namespace { int strideOf(uint64_t seed) { static const int S[20] = {1, 3, 7, 9, 11, 13, 17, 19, 21, 23, 27, 29, 31, 33, 37, 39, 41, 43, 47, 49}; return S[mix64(seed ^ SALT_AIR ^ 0x57E9ULL) % 20]; } }
int transmittedShard(uint64_t worldSeed, double t) {
    int64_t slot = (int64_t)std::floor(std::max(t, 0.0) / SIGNAL_RECORDING_S);
    int start = (int)(mix64(worldSeed ^ SALT_AIR) % 50);
    return (int)((start + (slot % 50) * strideOf(worldSeed)) % 50);
}
int slotPeople(const BodyGen& g, uint64_t worldSeed, int slot) {
    int air = peoplesOf(g) > 1 ? peoplesOnAir(g) : (peoplesOnAir(g) & 1);
    if (air == 0) return -1;
    if (air != 3) return air == 2 ? 1 : 0;
    return (mix64(worldSeed ^ ((uint64_t)(slot + 1) * 0x9E3779B97F4A7C15ULL) ^ 0xC13A1ULL) & 1) ? 1 : 0;   // C-13's coin
}
int nextTransmittedShard(uint64_t worldSeed, int prev) { return (((prev + strideOf(worldSeed)) % 50) + 50) % 50; }

std::string signalSourceLine(const Signal& s) {
    auto art = [](const std::string& w) { char c = w.empty() ? 'X' : w[0]; return std::string((c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U') ? "AN " : "A ") + w; };
    switch (s.kind) {
        case SIG_PEOPLE: return art(std::string(KINDS[s.bodyType].name) + " WORLD OF " + art(STAR_CLASSES[s.star.cls].name));
        case SIG_PULSAR: { char buf[64]; snprintf(buf, sizeof buf, "A PULSAR, %.1f PULSES A SECOND", s.pulseHz); return buf; }
        case SIG_COMET: { char buf[64]; snprintf(buf, sizeof buf, "A COMET OUTGASSING %.1f AU FROM THE SUN", s.detail); return buf; }
        case SIG_ROGUE: return s.bodyType >= 0 ? art(KINDS[s.bodyType].name) + " WITH NO STAR" : "A WORLD WITH NO STAR";
        default: return s.bodyType >= 0 ? "THE MAGNETOSPHERE OF " + art(KINDS[s.bodyType].name) : "A MAGNETOSPHERE";
    }
}

// ---- the programmes of a people's broadcast (the user's second call: more sounds, in the cosmic-horror direction) ----
const char* const RADIO_PROGRAMME_NAMES[RP_COUNT] = {"A VOICE", "WHISPERS", "A CHANT", "A NUMBERS STATION", "A LOOP", "A BEACON", "DATA BURSTS", "A BELL TOLLING", "A SIREN", "A MURMUR OF VOICES", "A PIECE, SLOWED",
                                                    "MORSE BY SPARK", "A PHONOGRAPH", "A SYNTHETIC VOICE", "MATHEMATICS"};

namespace {
inline double unit(uint64_t h) { return (h >> 11) * (1.0 / 9007199254740992.0); }
// the world's character: two of the five machines and two of the four ways of speaking
void worldCharacter(uint64_t worldSeed, int* m1, int* m2, int* v1, int* v2) {
    uint64_t h = mix64(worldSeed ^ 0x9C07C0DEULL);
    *m1 = (int)(h % 5); *m2 = (*m1 + 1 + (int)((h >> 8) % 4)) % 5;
    *v1 = (int)((h >> 16) % 4); *v2 = (*v1 + 1 + (int)((h >> 24) % 3)) % 4;
}
const int MACHINES[5] = {RP_BEACON, RP_DATA, RP_TOLL, RP_SIREN, RP_MURMUR};
const int SPEAKINGS[4] = {RP_WHISPERS, RP_CHANT, RP_NUMBERS, RP_LOOP};
}

uint64_t programmeSeed(uint64_t worldSeed, int slot) { return mix64(worldSeed ^ ((uint64_t)(slot + 1) * 0x5157A3B1ULL) ^ 0xC07B2ULL); }

// a music slot: the piece three times in ten (the user found the pieces through the static the least of it), else one of the
// world's two machines; a text slot: the voice as it is eleven times in twenty, the world's two ways of speaking three each,
// its machines the rest
// C-16: by the slot's people's age. The late industrial age: the phonograph six text slots in ten and a music slot's piece one in
// three, the rest Morse by spark; the futuristic: a synthetic voice reading over half the text slots, data and beacons the rest
// and a music slot's piece one in five (C-07's rarity); the far future: the beacon of mathematics, a synthetic voice reading one
// text slot in four; the modern age and anything else C-07's
int programmeOf(uint64_t worldSeed, int slot, bool music, int age) {
    int m1, m2, v1, v2; worldCharacter(worldSeed, &m1, &m2, &v1, &v2);
    double u = unit(programmeSeed(worldSeed, slot) ^ 0x7E1ULL);
    switch (age) {
        case AGE_INDUSTRIAL: return music ? (u < 0.35 ? RP_PHONOGRAPH : RP_MORSE) : (u < 0.6 ? RP_PHONOGRAPH : RP_MORSE);
        case AGE_FUTURISTIC:
            if (music) return u < 0.2 ? RP_MUSIC : (u < 0.6 ? RP_DATA : RP_BEACON);
            return u < 0.55 ? RP_SYNTHETIC : (u < 0.8 ? RP_DATA : RP_BEACON);
        case AGE_FAR_FUTURE: return music || u < 0.75 ? RP_MATHS : RP_SYNTHETIC;
        default: break;
    }
    if (music) return u < 0.3 ? RP_MUSIC : (u < 0.65 ? MACHINES[m1] : MACHINES[m2]);
    if (u < 0.55) return RP_VOICE;
    if (u < 0.70) return SPEAKINGS[v1];
    if (u < 0.85) return SPEAKINGS[v2];
    return u < 0.93 ? MACHINES[m1] : MACHINES[m2];
}

namespace {
// a digit group of four or five in the people's words for the digits, one word spoken digit by digit (as `decodeShard` reads a numeral)
DecodedWord digitGroup(const Tongue& T, uint64_t h, int year, int k) {
    static const char* const DIGITS[10] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
    DecodedWord w; w.known = true; w.post = ".";
    std::string digits;
    if (k == 0 && year > 0) { digits = std::to_string(year); while (digits.size() < 4) digits = "0" + digits; }
    else { int n = 4 + (int)((h >> 3) % 2); uint64_t g = mix64(h ^ (uint64_t)(k + 11) * 0x9E37ULL); for (int i = 0; i < n; i++) { digits += (char)('0' + g % 10); g /= 10; } }
    w.ours = digits;
    for (char c : digits) { if (!w.theirs.empty()) w.theirs += '-'; w.theirs += tongueWord(T, DIGITS[c - '0']); }
    return w;
}
}

namespace {
// C-16: a letter in the people's own code (one to four elements, dots and dashes by the world's hash: two worlds key the same word
// differently), appended with the gaps: one unit between elements, three after the letter
void morseLetter(uint64_t worldSeed, char c, std::vector<uint8_t>& out) {
    uint64_t h = mix64(worldSeed ^ 0x5A4B0C16ULL ^ ((uint64_t)(unsigned char)c * 0x9E3779B97F4A7C15ULL));
    int n = 1 + (int)(h % 4);
    for (int i = 0; i < n; i++) { out.push_back(((h >> (8 + i)) & 1) ? 3 : 1); out.push_back(MORSE_OFF | 1); }
    out.back() = MORSE_OFF | 3;
}
void morseWord(uint64_t worldSeed, const std::string& w, std::vector<uint8_t>& out) {
    size_t before = out.size();
    for (char c : w) { char l = (char)std::tolower((unsigned char)c); if (l >= 'a' && l <= 'z') morseLetter(worldSeed, l, out); }
    if (out.size() > before) out.back() = MORSE_OFF | 7;
}
}

double morseSeconds(const std::vector<uint8_t>& code, double unit) {
    double units = 0; for (uint8_t e : code) units += e & 0x7F;
    return units * unit;
}

void programmeRepeatWords(const Programme& p, int k, const Tongue& T, std::vector<DecodedWord>& out) {
    if (p.kind == RP_NUMBERS && k > 0) { out.clear(); out.push_back(digitGroup(T, p.seed, 0, k)); return; }   // a fresh group each time
    out = p.words;
}

void programmeFor(uint64_t worldSeed, int slot, bool music, int year, const Voice& V, const Tongue& T, const std::vector<DecodedWord>& text, Programme& out, int force, int age) {
    out = Programme();
    out.kind = force >= 0 && force < RP_COUNT ? force : programmeOf(worldSeed, slot, music, age);
    out.seed = programmeSeed(worldSeed, slot); out.age = age;
    uint64_t h = mix64(out.seed ^ 0x51ULL);
    out.voice = V;
    switch (out.kind) {
        case RP_MORSE: {    // C-16: the station's call twice, then the text keyed (as much as forty seconds holds), at the world's own speed
            out.machine = true; out.unit = 0.055 + 0.05 * unit(mix64(worldSeed ^ 0x40125EULL));
            std::string call; uint64_t c = mix64(worldSeed ^ 0xCA11ULL); for (int i = 0; i < 3; i++) { call += (char)('a' + c % 26); c /= 26; }
            for (int k = 0; k < 2; k++) morseWord(worldSeed, call, out.code);
            out.code.push_back(MORSE_OFF | 10);
            if (!music) for (const DecodedWord& w : text) {
                if (w.theirs.empty()) continue;
                std::vector<uint8_t> more = out.code; morseWord(worldSeed, w.theirs, more);
                if (morseSeconds(more, out.unit) > 40 && out.speaks) break;
                out.code.swap(more); out.speaks = true;
            } else {   // a music slot: the call and a group of the people's numbers, again
                uint64_t g = mix64(out.seed ^ 0x6E0ULL); std::string grp; for (int i = 0; i < 5; i++) { grp += (char)('a' + g % 26); g /= 26; }
                for (int k = 0; k < 3; k++) { morseWord(worldSeed, grp, out.code); morseWord(worldSeed, call, out.code); }
            }
            out.seconds = morseSeconds(out.code, out.unit) + 2.0;
            break;
        }
        case RP_PHONOGRAPH:   // the recording played into the transmitter from a phonograph: the voice as it is, or the piece near its speed
            out.speaks = true;
            if (music) { out.piece = true; out.pieceSpeed = 0.94 + 0.06 * unit(mix64(h ^ 13)); }
            else out.words = text;
            break;
        case RP_SYNTHETIC:    // a synthetic voice reading it: one pitch, no breath, no tremor, every syllable alike, a little quick
            out.speaks = true; out.words = text;
            out.voice.range = 0.12; out.voice.breath = 0.02; out.voice.rough = 0; out.voice.tremor = 0; out.voice.nasal = 0; out.voice.sub = 0;
            out.voice.contour = 2; out.voice.stress = 3; out.voice.rate = V.rate * 1.08; break;
        case RP_MATHS: out.machine = true; out.seconds = 30 + 20 * unit(mix64(h ^ 17)); break;   // the synth counts it from the seed
        case RP_VOICE: out.speaks = true; out.words = text; break;
        case RP_WHISPERS:   // close to the microphone, slower, the melody flattened
            out.speaks = true; out.words = text;
            out.voice.breath = 0.95; out.voice.rate = V.rate * 0.8; out.voice.range = V.range * 0.6; out.voice.rough = std::max(V.rough, 0.3); break;
        case RP_CHANT:      // deep, at half the rate, level, trembling, a tone an octave under
            out.speaks = true; out.words = text;
            out.voice.pitch = V.pitch * 0.72; out.voice.rate = V.rate * 0.5; out.voice.range = 0.35; out.voice.contour = 2; out.voice.stress = 3;
            out.voice.tremor = std::max(V.tremor, 0.6); out.voice.sub = 0.4; out.voice.subRatio = 0.5; out.voice.breath = std::min(V.breath, 0.5); break;
        case RP_NUMBERS:    // read flat and slow, a group at a time
            out.voice.range = 0.3; out.voice.rate = V.rate * 0.75; out.voice.contour = 2; out.voice.breath = std::max(V.breath, 0.1);
            out.words.push_back(digitGroup(T, out.seed, year, 0));
            out.repeats = 5 + (int)(h % 4); out.gap = 1.0 + 0.8 * unit(mix64(h ^ 3)); break;
        case RP_LOOP: {     // the opening words, three to six, again and again
            out.speaks = true; out.voice.range = V.range * 0.8;
            int n = 3 + (int)(h % 4); for (size_t i = 0; i < text.size() && (int)out.words.size() < n; i++) if (!text[i].ours.empty()) out.words.push_back(text[i]);
            if (!out.words.empty()) { out.words.back().post = "."; }
            out.repeats = 4 + (int)((h >> 8) % 4); out.gap = 3.0 + 5.0 * unit(mix64(h ^ 5)); break;
        }
        case RP_MUSIC: out.speaks = true; out.piece = true; out.pieceSpeed = 0.5 + 0.25 * unit(mix64(h ^ 7)); break;
        default: out.machine = true; out.seconds = 20 + 25 * unit(mix64(h ^ 9)); break;
    }
    if (out.kind != RP_NUMBERS && out.kind != RP_LOOP) { out.repeats = 1; out.gap = 0; }
}

// ---- D-05: a drifter's beacon ------------------------------------------------------------------------------------------------------
int beaconMachineOf(uint64_t drifterSeed, int people, int age) {
    const uint64_t h = mix64(drifterSeed ^ 0xD05BEAC0ULL ^ (uint64_t)people);
    switch (age) {
        case AGE_INDUSTRIAL: return (h & 1) ? RP_TOLL : RP_BEACON;
        case AGE_MODERN: { static const int M[3] = {RP_BEACON, RP_SIREN, RP_DATA}; return M[h % 3]; }
        case AGE_FUTURISTIC: return (h & 1) ? RP_DATA : RP_BEACON;
        default: return RP_MATHS;
    }
}

namespace {
bool beaconOf(const StarSystem& sys, DrifterBeacon& out, bool wait) {
    out = DrifterBeacon();
    if (sys.bodies.empty() || !drifterHadPeople(sys.bodies[0])) return true;
    const Body& b = sys.bodies[0];
    const BodyGen g = BodyGen::make(b);
    if (!worldHadCivilisation(g)) return true;
    const int radio = historyInventionIndex("radio");
    for (int k = 0; k < peoplesOf(g); k++) {
        const PeopleProfile P = peopleProfileOf(sys, b, g, k);
        if (!P.radio) continue;
        if (!wait && !shardTextsIfReady(sys, b, g, loreQuick(sys, b, g, k))) return false;   // its records and its history being written
        std::shared_ptr<const History> H = historyOf(sys, b, g, k);
        if (!H || H->inventionYear(radio) <= 0) continue;   // its radio came after the last sunrise: no broadcast left
        out.people = k; out.age = P.age; out.machine = beaconMachineOf(sys.star.seed, k, P.age);
        return true;
    }
    return true;
}
}
bool drifterBeaconIfReady(const StarSystem& sys, DrifterBeacon& out) { return beaconOf(sys, out, false); }
DrifterBeacon drifterBeaconOf(const StarSystem& sys) { DrifterBeacon B; beaconOf(sys, B, true); return B; }

void beaconProgramme(uint64_t drifterSeed, const DrifterBeacon& B, const Voice& V, const std::vector<DecodedWord>& text, Programme& out) {
    out = Programme();
    out.kind = B.machine; out.seed = mix64(drifterSeed ^ 0xD05BEA5ULL); out.age = B.age;
    out.speaks = true; out.words = text;
    out.voice = V;   // slowed and worn: a third slower, lower a little, the melody flattened, rough and breathy
    out.voice.rate = V.rate * 0.68; out.voice.pitch = V.pitch * 0.94; out.voice.range = V.range * 0.7;
    out.voice.rough = std::max(V.rough, 0.35); out.voice.breath = std::max(V.breath, 0.3);
    out.repeats = 3; out.gap = 6.0 + 6.0 * unit(mix64(out.seed ^ 0x6A9ULL));
}
