// X-01: a probe's fall into a giant (probe.h): the giant's air and the probe's clock.
#include "probe.h"
#include "core/rng.h"
#include "core/noise.h"
#include "core/parallel.h"
#include <cmath>
#include <algorithm>

const char* const PROBE_STAGE_NAMES[PS_COUNT] = {"FALLING", "ENTRY", "ABOVE THE CLOUDS", "THE UPPER HAZE", "THE AMMONIA DECK", "BETWEEN THE DECKS", "THE WATER DECK", "THE DEEP", "SIGNAL LOST"};

namespace {
const double ADIABAT = 0.286;   // R / cp of hydrogen and helium: T grows as P^0.286 down the convecting air
const double H_PER_K = 3.6148;  // km of scale height per kelvin at 1 m/s^2: R / (mu g) with mu 2.3 g/mol
const double RF = 1.0 / (4 * PROBE_TAU1);   // the fourth root of the pressure gained a second, falling free
double tropoK(const GiantAtmosphere& a) { return a.t1K * std::pow(a.tropoBar, ADIABAT); }
double qTop() { return std::pow(PROBE_TOP_BAR, 0.25); }
// the fourth root of the pressure at a clock of the descent: even steps, a quarter as fast under the chute
double qAt(const ProbeFlight& f, double c) {
    double c0 = probeDescentStart(), q = qTop();
    if (c <= c0) return q;
    if (f.chuteOpen < 0 || c <= f.chuteOpen) return q + (c - c0) * RF;
    q += (f.chuteOpen - c0) * RF;
    double cc = f.chuteClose >= 0 ? std::min(c, f.chuteClose) : c;
    q += (cc - f.chuteOpen) * RF / PROBE_CHUTE_SLOW;
    if (f.chuteClose >= 0 && c > f.chuteClose) q += (c - f.chuteClose) * RF;
    return q;
}
// how far through a span of pressure (by its logarithm) a pressure is, 0..1
double through(double bar, double from, double to) { return clampd(std::log(bar / from) / std::log(to / from), 0, 1); }
// X-02: an octave of wavelength `lambda` under the sampling's spacing `lod` fades out (it would alias): whole over five
// spacings, gone under two
inline double lodFade(double lambda, double lod) { return lod <= 0 ? 1.0 : smoothstep(1.5 * lod, 3.5 * lod, lambda); }
// a thunderhead by the radius's fraction: steep sides up to the anvil's broad, nearly flat top, an overshooting dome over the core
inline double towerProfile(double d) {
    double p = std::pow(std::max(0.0, 1 - d * d), 0.6);
    if (d < 0.38) { double c = d / 0.38; p += 0.16 * (1 - c * c); }
    return p;
}
inline uint32_t chash(int32_t i, int32_t j, int32_t k, uint32_t seed) {
    uint32_t h = (uint32_t)i * 0x8DA6B343u ^ (uint32_t)j * 0xD8163841u ^ (uint32_t)k * 0xCB1AB31Fu ^ seed * 0x9E3779B9u;
    h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
    return h;
}
inline float cfade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
const float G2[8][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}, {1.4142f, 0}, {-1.4142f, 0}, {0, 1.4142f}, {0, -1.4142f}};
const float G3[16][3] = {{1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0}, {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
                         {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}, {1, 1, 0}, {0, -1, 1}, {-1, 1, 0}, {0, -1, -1}};
inline float cgrad2(uint32_t h, float x, float y) { const float* g = G2[h & 7]; return g[0] * x + g[1] * y; }
inline float cgrad3(uint32_t h, float x, float y, float z) { const float* g = G3[h & 15]; return g[0] * x + g[1] * y + g[2] * z; }
inline int32_t ifloor32(double x) { int32_t i = (int32_t)x; return x < i ? i - 1 : i; }
inline uint32_t seed32(uint64_t s) { return (uint32_t)(s ^ (s >> 32)); }
}

float cloudNoise2(double x, double y, uint32_t seed) {
    const int32_t i = ifloor32(x), j = ifloor32(y);
    const float tx = (float)(x - i), ty = (float)(y - j), u = cfade(tx), v = cfade(ty);
    const float n00 = cgrad2(chash(i, j, 0, seed), tx, ty), n10 = cgrad2(chash(i + 1, j, 0, seed), tx - 1, ty);
    const float n01 = cgrad2(chash(i, j + 1, 0, seed), tx, ty - 1), n11 = cgrad2(chash(i + 1, j + 1, 0, seed), tx - 1, ty - 1);
    const float a = n00 + (n10 - n00) * u, b = n01 + (n11 - n01) * u;
    return 0.745f * (a + (b - a) * v);
}

float cloudNoise3(double x, double y, double z, uint32_t seed) {
    const int32_t i = ifloor32(x), j = ifloor32(y), k = ifloor32(z);
    const float tx = (float)(x - i), ty = (float)(y - j), tz = (float)(z - k), u = cfade(tx), v = cfade(ty), w = cfade(tz);
    float c[2][2][2];
    for (int dk = 0; dk < 2; dk++) for (int dj = 0; dj < 2; dj++) for (int di = 0; di < 2; di++) c[dk][dj][di] = cgrad3(chash(i + di, j + dj, k + dk, seed), tx - di, ty - dj, tz - dk);
    const float x00 = c[0][0][0] + (c[0][0][1] - c[0][0][0]) * u, x10 = c[0][1][0] + (c[0][1][1] - c[0][1][0]) * u;
    const float x01 = c[1][0][0] + (c[1][0][1] - c[1][0][0]) * u, x11 = c[1][1][0] + (c[1][1][1] - c[1][1][0]) * u;
    const float y0 = x00 + (x10 - x00) * v, y1 = x01 + (x11 - x01) * v;
    return 1.05f * (y0 + (y1 - y0) * w);
}

const char* const GIANT_KIND_NAMES[3] = {"GAS GIANT", "ICE GIANT", "BROWN DWARF"};
const CloudSpeciesInfo CLOUD_SPECIES[CS_COUNT] = {
    {"THE METHANE DECK", "methane", 80, 2.2, 1.8, RGB(0.84f, 0.95f, 0.98f), false},
    {"THE SULPHIDE DECK", "hydrogen sulphide", 125, 2.0, 2.2, RGB(0.88f, 0.86f, 0.72f), false},
    {"THE AMMONIA DECK", "ammonia", 150, 2.4, 2.2, RGB(0.97f, 0.94f, 0.87f), false},
    {"THE HYDROSULPHIDE DECK", "ammonium hydrosulphide", 200, 1.7, 1.6, RGB(0.82f, 0.64f, 0.44f), false},
    {"THE WATER DECK", "water", 275, 1.8, 3.5, RGB(0.84f, 0.84f, 0.88f), true},
    {"THE SALT DECK", "chloride and sulphide salts", 700, 1.9, 2.8, RGB(0.66f, 0.58f, 0.52f), true},
};

namespace {
// a star's youth (0 an old remnant .. 1 still forming): a giant of a short-lived star is young and hot inside
double starYouth(int cls) {
    switch (cls) {
        case STAR_PROTOSTAR: return 1.0;
        case STAR_WOLF_RAYET: return 0.95;
        case STAR_BLUE_GIANT: return 0.9;
        case STAR_BLUE_WHITE: return 0.6;
        case STAR_YELLOW: return 0.3;
        case STAR_ORANGE: return 0.25;
        case STAR_RED_DWARF: return 0.2;
        case STAR_ORANGE_GIANT: case STAR_RED_GIANT: case STAR_CARBON: return 0.1;
        default: return 0.03;   // the white dwarf, the pulsar, the neutron star, the black hole: what is left of a star
    }
}
}

GiantCharacter giantCharacterOf(const StarSystem& sys, int bi) {
    GiantCharacter c;
    const Body& b = sys.bodies[bi];
    if (!isProbeGiant(b.type)) return c;
    const BodyGen g = BodyGen::make(b);
    auto hu = [&](uint64_t salt) { return unitFromHash(mix64(b.seed ^ salt)); };
    c.kind = b.type == PT_SUBSTELLAR ? GK_BROWN : (isIceGiant(b) ? GK_ICE : GK_GAS);
    const double youth = starYouth(sys.star.cls);
    if (c.kind == GK_BROWN) c.heat = 1;
    else {
        c.heat = clampd(0.75 * youth + 0.25 * (b.radiusKm - 24000) / 48000 + 0.35 * (hu(0x4EA7) - 0.5), 0, 1);
        if (c.kind == GK_ICE) c.heat *= 0.6;
    }
    c.young = c.kind != GK_BROWN && youth >= 0.5 && c.heat >= 0.62;
    c.abundance = std::exp((c.kind == GK_ICE ? 0.25 : 0.0) + 0.3 * (hu(0xAB0D) - 0.5) * 2);   // an ice giant is richer in what condenses
    c.storms = (0.35 + 0.65 * g.stormShare / 0.15) * (0.8 + 0.4 * c.heat) * (c.kind == GK_ICE ? 0.55 : 1.0);   // the plain giant's 1, a calm one's half
    c.glow = c.kind == GK_BROWN ? 1.4 + 1.6 * clampd((g.selfGlow - 0.55) / 0.45, 0, 1) : (c.kind == GK_ICE ? 0.003 + 0.04 * c.heat * c.heat : 0.004 + 0.3 * std::pow(c.heat, 2.5));
    c.glowCol = c.kind == GK_BROWN ? RGB(1.0f, 0.42f, 0.16f) : (c.kind == GK_ICE ? RGB(0.55f, 0.2f, 0.22f) : RGB(0.78f, 0.24f, 0.08f));
    {   // the aurorae: the field (R-402's hash), the star's wind, a volcanic moon's plasma feeding them steadily, a brown dwarf's own
        bool volcanic = false;
        for (const Body& m : sys.bodies) if (m.parent == bi && m.type == PT_VOLCANIC) volcanic = true;
        c.aurora = clampd(0.3 * starActivity(sys.star.cls) + 0.5 * magneticField(b) + (volcanic ? 0.35 : 0.0) + (c.kind == GK_BROWN ? 0.4 : 0.0) - 0.15, 0, 1);
        c.magColat = (c.kind == GK_ICE ? 30 + 30 * hu(0xA6C0) : (c.kind == GK_BROWN ? 5 + 10 * hu(0xA6C0) : 2 + 10 * hu(0xA6C0))) * DEG;
        c.magLon = (hu(0xA6C1) - 0.5) * TAU;
        c.ovalRad = (c.kind == GK_BROWN ? 10 + 10 * hu(0xA6C2) : 14 + 9 * hu(0xA6C2)) * DEG;
        if (c.kind == GK_ICE) { c.auroraLow = RGB(0.42f, 0.48f, 1.0f); c.auroraHigh = RGB(0.36f, 0.22f, 0.8f); }
        else if (c.kind == GK_BROWN) { c.auroraLow = RGB(1.0f, 0.22f, 0.28f); c.auroraHigh = RGB(0.95f, 0.48f, 0.16f); }
    }
    {   // the clear-air holes: one giant in four (an ice giant's air seldom parts; a brown dwarf's patchy clouds part often)
        double odds = c.kind == GK_BROWN ? 0.6 : (c.kind == GK_ICE ? 0.08 : 0.25);
        if (hu(0x401E) < odds) c.holes = 0.15 + 0.2 * hu(0x401F);
    }
    c.lightning = (c.kind == GK_ICE ? 0.12 : (c.kind == GK_BROWN ? 0.8 : 0.4 + 1.2 * c.heat)) * (0.5 + hu(0x11F7));   // an ice giant's seldom
    c.hail = sys.star.cls == STAR_CARBON || hu(0xD1A3) < 0.06;
    c.sight = c.kind != GK_BROWN && hu(0x5167) < 1.0 / 40;
    c.greatStorm = g.gsOn;
    return c;
}

// X-03: the decks where the adiabat crosses each cloud's temperature (the more there is of it, the warmer it condenses: its base
// deeper), within the probe's reach (32 bar or the heat's end), from the top down, kept apart by a clear band of a quarter at least
GiantAtmosphere giantAtmosphereOf(const StarSystem& sys, int bi) {
    GiantAtmosphere a;
    const Body& b = sys.bodies[bi];
    if (!isProbeGiant(b.type)) return a;
    a.valid = true;
    a.ch = giantCharacterOf(sys, bi);
    const GiantCharacter& c = a.ch;
    const BodyGen g = BodyGen::make(b);
    const double teq = std::max(60.0, 1.5 * b.tempK);
    a.t1K = c.kind == GK_BROWN ? teq + 90 + 120 * clampd((g.selfGlow - 0.55) / 0.45, 0, 1) : teq * (1 + (c.kind == GK_ICE ? 0.12 : 0.3) * c.heat * c.heat);
    a.gravity = std::max(1.0, b.gravity);
    a.windPeak = (c.kind == GK_ICE ? 220 + 380 * unitFromHash(mix64(b.seed ^ 0xA7E05ULL)) : 80 + 370 * unitFromHash(mix64(b.seed ^ 0xA7E05ULL)));   // an ice giant's jets the fastest
    a.bandCount = g.bandCount;
    a.driftSign = b.rotPeriod < 0 ? -1 : 1;
    const double tropoK = a.t1K * std::pow(a.tropoBar, ADIABAT);
    const double heatEnd = std::pow(a.heatK / a.t1K, 1 / ADIABAT), reach = std::min(32.0, heatEnd);
    static const int GAS[] = {CS_METHANE, CS_AMMONIA, CS_NH4SH, CS_WATER, CS_SALT}, ICE[] = {CS_METHANE, CS_H2S, CS_NH4SH, CS_WATER, CS_SALT}, BROWN[] = {CS_AMMONIA, CS_NH4SH, CS_WATER, CS_SALT};
    const int* list = c.kind == GK_ICE ? ICE : (c.kind == GK_BROWN ? BROWN : GAS);
    const int nl = c.kind == GK_BROWN ? 4 : 5;
    a.decks = 0;
    double lastBase = 0;
    // what the giant is poor in: no sulphur to bind (the hydrosulphide), the ammonia dissolved deep, no salts brought up from below
    static const double KEEP[CS_COUNT] = {1.0, 0.8, 0.9, 0.55, 1.0, 0.75};
    for (int i = 0; i < nl && a.decks < GIANT_MAX_DECKS; i++) {
        const CloudSpeciesInfo& sp = CLOUD_SPECIES[list[i]];
        if (sp.condK < tropoK + 6) continue;                                           // the coldest air is too warm for it to condense
        if (unitFromHash(mix64(b.seed ^ (0xC1D0ULL + (uint64_t)list[i]))) >= KEEP[list[i]]) continue;
        double base = std::pow(sp.condK / a.t1K, 1 / ADIABAT) * std::pow(c.abundance, 0.6);
        double top = std::max(base / sp.span, a.tropoBar * 0.7);
        if (lastBase > 0) top = std::max(top, lastBase * 1.25);
        if (base < top * 1.3 || base < a.tropoBar * 1.2 || top >= reach) continue;     // squeezed out, at the tropopause, or out of reach
        a.species[a.decks] = list[i]; a.top[a.decks] = top; a.base[a.decks] = base; a.decks++;
        lastBase = base;
    }
    if (a.decks == 0) {   // nothing condenses within the reach: a haze deck of the salts just over the end (never seen on the giants near home)
        a.species[0] = CS_SALT; a.base[0] = reach * 0.8; a.top[0] = reach * 0.8 / 1.9; a.decks = 1;
    }
    for (int k = a.decks; k < GIANT_MAX_DECKS; k++) { a.species[k] = a.species[a.decks - 1]; a.top[k] = a.top[a.decks - 1]; a.base[k] = a.base[a.decks - 1]; }
    bool convective = false;
    for (int k = 0; k < a.decks; k++) convective = convective || CLOUD_SPECIES[a.species[k]].convective;
    if (!convective) a.ch.lightning *= 0.15;   // no water within the reach: only a far glow now and then from under the decks
    if (c.kind == GK_BROWN) {   // a brown dwarf's tops shine from under them as its globe does from afar (BodyGen::selfGlow), the deep the more
        a.ch.glow = 1;
        a.ch.glow = 0.5 * g.selfGlow / std::max(1e-6, giantGlowAt(a, a.top[0]));
    }
    a.hazeBar = clampd(a.top[0] / (1.6 + 1.4 * unitFromHash(mix64(b.seed ^ 0x4A2EULL))), 0.02, 0.4);
    return a;
}

double giantTemperatureK(const GiantAtmosphere& a, double bar) {
    bar = std::max(bar, 1e-9);
    if (bar >= a.tropoBar) return a.t1K * std::pow(bar, ADIABAT);
    double x = std::log(a.tropoBar / bar);
    return tropoK(a) * (1 + 0.4 * (1 - std::exp(-x / 3)));   // the stratosphere warms by two fifths toward the top
}

// the height over the one-bar level: the scale height (R T / mu g) summed over the e-folds of pressure, in closed form on
// the adiabat (T = t1 P^a) and in the stratosphere (T = Tt (1.4 - 0.4 e^(-x/3)), x the e-folds above the tropopause)
double giantAltitudeKm(const GiantAtmosphere& a, double bar) {
    bar = std::max(bar, 1e-9);
    double hk = H_PER_K / a.gravity;
    double adiabat = -hk * a.t1K / ADIABAT * (std::pow(std::max(bar, a.tropoBar), ADIABAT) - 1);
    if (bar >= a.tropoBar) return adiabat;
    double x = std::log(a.tropoBar / bar);
    return adiabat + hk * tropoK(a) * (1.4 * x - 1.2 * (1 - std::exp(-x / 3)));
}

// the globe's bands drift with `sin(lat x bands)` (space_view's zonal jets: a band whose texture runs to the west is a wind
// toward the west); the magnitude is the giant's own, half of it high in the haze, half again more below the first deck
double giantWindEast(const GiantAtmosphere& a, double latRad, double bar) {
    double lp = std::log(std::max(bar, 1e-9));
    double prof = (0.45 + 0.55 * smoothstep(std::log(0.01), 0.0, lp)) * (1 + 0.5 * smoothstep(0.0, std::log(5.0), lp));
    return -a.driftSign * a.windPeak * std::sin(latRad * a.bandCount) * prof;
}

int giantStageAt(const GiantAtmosphere& a, double bar) {
    if (bar < a.hazeBar) return PS_ABOVE;
    if (bar < a.top[0]) return PS_HAZE;
    for (int k = 0; k < a.decks; k++) {
        if (bar < a.base[k]) return bar >= a.top[k] ? (k == 0 ? PS_DECK1 : PS_DECK2) : PS_BETWEEN;
    }
    return PS_DEEP;
}

const char* giantStageName(const GiantAtmosphere& a, double bar) {
    int k = giantDeckAt(a, bar);
    return k >= 0 ? CLOUD_SPECIES[a.species[k]].deckName : PROBE_STAGE_NAMES[giantStageAt(a, bar)];
}

int giantDeckAt(const GiantAtmosphere& a, double bar) {
    for (int k = 0; k < a.decks; k++) if (bar >= a.top[k] && bar < a.base[k]) return k;
    return -1;
}

int giantLayerAt(const GiantAtmosphere& a, double bar) {
    if (bar < a.hazeBar) return 0;
    if (bar < a.top[0]) return 1;
    for (int k = 0; k < a.decks; k++) if (bar < a.base[k]) return bar >= a.top[k] ? 2 + 2 * k : 1 + 2 * k;
    return 1 + 2 * a.decks;
}

int giantDeckAbove(const GiantAtmosphere& a, double bar) {
    int k = -1;
    for (int i = 0; i < a.decks; i++) if (a.base[i] <= bar) k = i;
    return k;
}

int giantStormDeckFrom(const GiantAtmosphere& a, double bar) {
    for (int k = 0; k < a.decks; k++) if (a.base[k] > bar && CLOUD_SPECIES[a.species[k]].convective) return k;
    return -1;
}

std::string giantCharacterText(const GiantAtmosphere& a) {
    const GiantCharacter& c = a.ch;
    std::string s = std::string(GIANT_KIND_NAMES[c.kind]) + (c.young ? ", young" : "") + ", heat " + std::to_string((int)std::lround(c.heat * 100)) + "%, " +
                    std::to_string((int)std::lround(a.t1K)) + " K at one bar, the haze from " + std::to_string(a.hazeBar).substr(0, 5) + " bar\n";
    for (int k = 0; k < a.decks; k++) {
        char buf[200];
        snprintf(buf, sizeof buf, "    %s: %.2f-%.2f bar, %.0f to %.0f km%s\n", CLOUD_SPECIES[a.species[k]].deckName, a.top[k], a.base[k], giantAltitudeKm(a, a.top[k]), giantAltitudeKm(a, a.base[k]),
                 CLOUD_SPECIES[a.species[k]].convective ? ", convective" : "");
        s += buf;
    }
    char buf[400];
    snprintf(buf, sizeof buf, "    storms x%.2f, lightning x%.2f, glow %.3f, aurora %.2f (pole %.0f deg off the axis, oval %.0f deg), wind %.0f m/s", c.storms, c.lightning, c.glow, c.aurora,
             c.magColat / DEG, c.ovalRad / DEG, a.windPeak);
    s += buf;
    std::string rare;
    if (c.greatStorm) rare += ", a great storm";
    if (c.holes > 0) rare += ", clear-air holes";
    if (c.hail) rare += ", diamond hail";
    if (c.sight) rare += ", the sight";
    if (!rare.empty()) s += "\n    rare:" + rare.substr(1);
    return s;
}

double probeCrushBar(uint64_t seed) { return 18 + 14 * unitFromHash(mix64(seed ^ 0xC2B5ULL)); }
double probeDescentStart() { return PROBE_FALL_S + PROBE_ENTRY_S; }

double probeBarAt(const ProbeFlight& f, double c) {
    if (c < PROBE_FALL_S) return 0;
    if (c < probeDescentStart()) {   // the entry: a millionth of a bar to a thousandth, eased
        double u = (c - PROBE_FALL_S) / PROBE_ENTRY_S, s = u * u * (3 - 2 * u);
        return std::exp(std::log(PROBE_ENTRY_BAR) + (std::log(PROBE_TOP_BAR) - std::log(PROBE_ENTRY_BAR)) * s);
    }
    double q = qAt(f, c);
    return q * q * q * q;
}

double probeChuteCloseAt(const ProbeFlight& f, double openClock) {
    ProbeFlight g = f; g.chuteOpen = -1; g.chuteClose = -1;
    double q0 = qAt(g, openClock);
    return openClock + q0 * (std::pow(PROBE_CHUTE_SPAN, 0.25) - 1) / (RF / PROBE_CHUTE_SLOW);
}

double probeEndBar(const GiantAtmosphere& a, const ProbeFlight& f) {
    double pHeat = std::pow(a.heatK / a.t1K, 1 / ADIABAT);   // below the tropopause the heat grows with the pressure
    if (pHeat < a.tropoBar) pHeat = a.tropoBar;
    return std::max(PROBE_TOP_BAR * 1.01, std::min(f.crushBar, pHeat));
}

double probeEndClock(const GiantAtmosphere& a, const ProbeFlight& f) {
    double qe = std::pow(probeEndBar(a, f), 0.25), c0 = probeDescentStart(), q = qTop();
    if (f.chuteOpen < 0) return c0 + (qe - q) / RF;
    double qo = q + (f.chuteOpen - c0) * RF;
    if (qe <= qo) return c0 + (qe - q) / RF;
    double close = f.chuteClose >= 0 ? f.chuteClose : 1e18;
    double qc = qo + (close - f.chuteOpen) * RF / PROBE_CHUTE_SLOW;
    if (qe <= qc) return f.chuteOpen + (qe - qo) / (RF / PROBE_CHUTE_SLOW);
    return close + (qe - qc) / RF;
}

int probeStageAt(const GiantAtmosphere& a, const ProbeFlight& f, double c) {
    if (c < PROBE_FALL_S) return PS_FALL;
    if (c < probeDescentStart()) return PS_ENTRY;
    if (c >= probeEndClock(a, f)) return PS_END;
    return giantStageAt(a, probeBarAt(f, c));
}

// the optical depth over the probe: the clear air's (giantHazeTau) and the decks' own
double giantSunlightAt(const GiantAtmosphere& a, double bar, unsigned cut) {
    double tau = giantHazeTau(a, bar);
    for (int k = 0; k < a.decks; k++) if (!(cut & (1u << k))) tau += CLOUD_SPECIES[a.species[k]].tau * through(bar, a.top[k], a.base[k]);
    tau += 1.7 * through(bar, a.base[a.decks - 1], a.base[a.decks - 1] * 3);
    return std::exp(-tau);
}

double giantHazeTau(const GiantAtmosphere& a, double bar) {
    bar = std::max(bar, 1e-12);
    double tau = 0.03 * std::min(1.0, bar / 0.02) + 0.06 * through(bar, 0.02, a.hazeBar) + 0.12 * through(bar, a.hazeBar, a.top[0]);
    for (int k = 0; k + 1 < a.decks; k++) tau += 0.15 * through(bar, a.base[k], a.top[k + 1]);
    return tau + 0.3 * through(bar, a.base[a.decks - 1], a.base[a.decks - 1] * 3);
}

// the glow from below: the giant's under the lowest deck (rising over the deep's last span), each deck under the pressure
// letting through what its optical depth does (a little more than the sunlight's: the light from below is diffuse already)
double giantGlowAt(const GiantAtmosphere& a, double bar, unsigned cut) {
    const GiantCharacter& c = a.ch;
    if (c.glow <= 0) return 0;
    const double last = a.base[a.decks - 1];
    double g = c.glow * (0.35 + 0.65 * through(bar, last, last * 3));
    for (int k = 0; k < a.decks; k++) if (!(cut & (1u << k))) g *= std::exp(-0.35 * CLOUD_SPECIES[a.species[k]].tau * (1 - through(bar, a.top[k], a.base[k])));
    return g;
}

double giantJetTurn(const Body& b, double bandCount, double lat, double t) {
    return b.type == PT_GASGIANT ? TAU * t / b.rotPeriod * 0.06 * std::sin(lat * bandCount) : 0.0;
}

double giantJetAt(const Body& b, const BodyGen& g, double lat, double lon, double t) {
    double jet = giantJetTurn(b, g.bandCount, lat, t);
    if (!g.gsOn || jet == 0.0) return jet;
    const double jetS = giantJetTurn(b, g.bandCount, g.gsLat, t);   // the storm's centre where the pattern has carried it
    const double ex = wrapAngle(lon - (g.gsLon - jetS)) * std::cos(g.gsLat) / g.gsA, ey = (lat - g.gsLat) / g.gsB;
    const double d2 = ex * ex + ey * ey;
    if (d2 >= 2.2 * 2.2) return jet;
    return jet + (jetS - jet) * (1 - smoothstep(1.3, 2.2, std::sqrt(d2)));
}

void GiantBands::at(double x, double z, double& a, double& s) const {
    if (!valid()) { a = 0.55; s = 0; return; }
    double step = 2 * halfKm / (n - 1);
    double fx = clampd((x + halfKm) / step, 0, n - 1.0001), fz = clampd((z + halfKm) / step, 0, n - 1.0001);
    int i = (int)fx, j = (int)fz; double u = fx - i, w = fz - j;
    size_t o = (size_t)j * n + i;
    a = (alb[o] * (1 - u) + alb[o + 1] * u) * (1 - w) + (alb[o + n] * (1 - u) + alb[o + n + 1] * u) * w;
    s = (storm[o] * (1 - u) + storm[o + 1] * u) * (1 - w) + (storm[o + n] * (1 - u) + storm[o + n + 1] * u) * w;
}

// the planet function at the points of the tangent plane (the gnomonic projection: the plane's point pushed onto the sphere),
// turned by the jets as the globe turns its lookup, the polar vortex as the map has it
GiantBands giantBandsOf(const Body& b, const BodyGen& g, double lat, double lon, double t, double halfKm, int n) {
    GiantBands B;
    if (n < 2) return B;
    B.halfKm = halfKm; B.n = n;
    B.alb.assign((size_t)n * n, 0.55f); B.storm.assign((size_t)n * n, 0.f);
    Vec3 c0 = StarSystem::bodyFromLatLon(lat, lon);
    Vec3 e0 = cross(Vec3(0, 0, 1), c0);
    e0 = length(e0) < 1e-9 ? Vec3(0, 1, 0) : normalize(e0);   // the body frame's spin axis is its z
    Vec3 n0 = cross(c0, e0);
    const double R = b.radiusKm, step = 2 * halfKm / (n - 1);
    parallelFor(n, 4, [&](int j0, int j1) {
        for (int j = j0; j < j1; j++)
            for (int i = 0; i < n; i++) {
                Vec3 u = normalize(c0 * R + e0 * (-halfKm + i * step) + n0 * (-halfKm + j * step));
                double la, lo; StarSystem::latLonFromBody(u, la, lo);
                lo += giantJetAt(b, g, la, lo, t);
                SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 1000.0);
                double a = s.albedo;
                if (b.type == PT_GASGIANT && std::fabs(la) > 76 * DEG) a *= 1 + 0.14 * std::sin(6 * lo) * smoothstep(76 * DEG, 86 * DEG, std::fabs(la));
                B.alb[(size_t)j * n + i] = (float)clampd(a, 0, 1);
                B.storm[(size_t)j * n + i] = (float)clampd(s.veg, 0, 1);
            }
    });
    if (g.gsOn) {   // X-03: the great storm's oval in the plane, where the pattern has carried it by the time (it turns whole: giantJetAt)
        Vec3 us = StarSystem::bodyFromLatLon(g.gsLat, g.gsLon - giantJetTurn(b, g.bandCount, g.gsLat, t));
        double cz = dot(us, c0);
        if (cz > 0.3) {
            B.gsOn = true;
            B.gsX = R * dot(us, e0) / cz; B.gsZ = R * dot(us, n0) / cz;
            Vec3 es = normalize(cross(Vec3(0, 0, 1), us));
            double ex = dot(es, e0), ez = dot(es, n0), el = std::hypot(ex, ez);
            B.gsEx = el > 1e-9 ? ex / el : 1; B.gsEz = el > 1e-9 ? ez / el : 0;
            B.gsAKm = g.gsA * R; B.gsBKm = g.gsB * R; B.gsTone = g.gsTone;
        }
    }
    return B;
}

double GiantBands::greatStormAt(double x, double z, double* angle) const {
    if (!gsOn) return 9;
    const double dx = x - gsX, dz = z - gsZ, ue = (dx * gsEx + dz * gsEz) / gsAKm, un = (-dx * gsEz + dz * gsEx) / gsBKm;
    if (angle) *angle = std::atan2(un, ue);
    return std::sqrt(ue * ue + un * un);
}

void GiantDecks::init(const GiantAtmosphere& a, const GiantBands* b, uint64_t sd) {
    bands = b; seed = sd;
    sh = clampd(H_PER_K * a.t1K / a.gravity / 25.0, 0.2, 2.5);   // a brown dwarf's decks are a quarter of the plain giant's, so is their relief
    n = std::max(1, a.decks);
    for (int k = 0; k < GIANT_MAX_DECKS; k++) {
        species[k] = a.species[k];
        lvTop[k] = giantAltitudeKm(a, a.top[k]); lvBase[k] = giantAltitudeKm(a, a.base[k]);
        cell[k] = (convective(k) ? 44 : 52) * sh;
    }
    storms = a.ch.storms; calm = a.ch.kind == GK_ICE ? 0.6 : 1.0;
    holes = a.ch.holes; holeCell = 600 * sh;
    for (Hole& h : path) h = Hole();
}

namespace {
// the great storm's edge (km from it, negative inside) and the oval's own coordinates round its centre
inline double stormEdgeKm(const GiantBands& B, double x, double z, double* angle = nullptr, double* d = nullptr) {
    double dd = B.greatStormAt(x, z, angle);
    if (d) *d = dd;
    return (dd - 1) * std::sqrt(B.gsAKm * B.gsBKm);
}
}

// zones stand high and belts low (the globe's albedo), a storm higher still: the first deck only (the decks under it are the
// same under every band). X-03: the great storm's dome, six scale heights over the bands, and its wall: ten more over its
// edge, its inner face steep (25 scale heights of run), its outer slope long (300)
double GiantDecks::lift(int k, double x, double z) const {
    if (k != 0) return 0;
    double a = 0.55, s = 0;
    if (bands) bands->at(x, z, a, s);
    double h = sh * (4.0 * (a - 0.55) + 4.5 * s);
    if (bands && bands->gsOn) {
        const double e = stormEdgeKm(*bands, x, z) / sh;
        if (e < 320) h += sh * (6.0 * (1 - smoothstep(0, 220, e)) + 10.0 * smoothstep(-45, -20, e) * (1 - smoothstep(0, 300, e)));
    }
    return h;
}

// at most one tower in a cell, inside it (its centre in the middle two fifths, its radius under a quarter of the cell): in the
// first deck more in the belts and the storms than in the zones, most of them modest and a few tall, a crowd of tall ones on
// the great storm's wall and few inside it; a convective deck's taller, held under the deck over it; a quiet lower deck's
// few and low. As many as the giant's storms make
GiantDecks::Tower GiantDecks::towerOf(int k, int64_t ci, int64_t cj) const {
    Tower t;
    uint64_t h = mix64(seed ^ (0x70E1ULL + (uint64_t)k) ^ ((uint64_t)ci * 0x9E3779B97F4A7C15ULL) ^ ((uint64_t)cj * 0xC2B2AE3D27D4EB4FULL));
    const double C = cell[k];
    double cx = (ci + 0.3 + 0.4 * unitFromHash(mix64(h ^ 1))) * C, cz = (cj + 0.3 + 0.4 * unitFromHash(mix64(h ^ 2))) * C;
    const bool conv = convective(k);
    double odds = conv ? 0.42 : 0.12, wall = 0;
    if (k == 0) {
        double a = 0.55, s = 0; if (bands) bands->at(cx, cz, a, s);
        odds = 0.06 + 0.32 * clampd((0.75 - a) / 0.5, 0, 1) + 0.3 * s;
        if (bands && bands->gsOn) {
            const double e = stormEdgeKm(*bands, cx, cz) / sh;
            wall = std::exp(-((e + 20) / 45) * ((e + 20) / 45));
            odds = odds * (0.25 + 0.75 * smoothstep(-160, -60, e)) + 0.75 * wall;   // a calm inside, a crowd on the wall
        }
    }
    odds = std::min(0.92, odds * std::min(storms, 1.8) * (1 + wall));
    if (unitFromHash(h) >= odds) return t;
    double u = unitFromHash(mix64(h ^ 4));
    t.on = true; t.cx = cx; t.cz = cz;
    t.r = (0.11 + 0.13 * unitFromHash(mix64(h ^ 3))) * C;
    t.h = (conv ? 5 + 12 * u : (k == 0 ? 2.5 + 7.5 * u * u : 1.5 + 4 * u)) * sh * (0.8 + 0.2 * std::min(storms, 2.0)) * (1 + 0.9 * wall);
    if (k > 0) t.h = std::min(t.h, 0.55 * (lvBase[k - 1] - lvTop[k]));
    return t;
}

// a tower's dome, its edge lumped into cauliflower heads (the radius wanders with a noise round it)
double GiantDecks::towerAt(int k, double x, double z, double* inside) const {
    const double C = cell[k];
    if (inside) *inside = 0;
    Tower t = towerOf(k, (int64_t)std::floor(x / C), (int64_t)std::floor(z / C));
    if (!t.on) return 0;
    double d = std::hypot(x - t.cx, z - t.cz) / t.r;
    if (d >= 1.3) return 0;
    d *= 1 + 0.16 * smoothstep(0.55, 0.9, d) * cloudNoise2(x / (0.3 * t.r), z / (0.3 * t.r), seed32(seed) ^ 0x70E3u);   // the sides lumped into cauliflower heads
    if (d >= 1) return 0;
    if (inside) *inside = 1 - d;
    return t.h * towerProfile(d);
}

double GiantDecks::top(int k, double x, double z, double lod, double* low) const {
    const double S = sh, X = x / S, Z = z / S, L = lod / S;   // the shapes in the plain giant's units
    const uint32_t s0 = seed32(seed) ^ (0x7A01u + (uint32_t)k);
    const bool conv = convective(k);
    double h = lvTop[k] + lift(k, x, z), calmHere = calm;
    h += S * 1.4 * cloudNoise2(X / 150, Z / 80, s0);                                     // the long swell
    double streets = cloudNoise2(X / 48, Z / 11, s0 + 1), inStorm = 0;
    if (k == 0 && bands && bands->gsOn) {   // X-03: inside the great storm the streets run round its centre in long swirls, the billows calmer
        double ang, d; const double e = stormEdgeKm(*bands, x, z, &ang, &d) / S;
        if (e < 60) {
            inStorm = 1 - smoothstep(-120, 0, e);
            // on the circle round the centre (its cosine and sine: no seam), wound further round inside, the streets along the circles
            const double r = d * std::sqrt(bands->gsAKm * bands->gsBKm) / S;
            const double tw = ang + 2.4 * std::pow(std::max(0.0, 1 - d), 1.2), ct = std::cos(tw), st = std::sin(tw);
            streets += (cloudNoise3(ct * r / 48, st * r / 48, r / 11, s0 + 1) - streets) * inStorm;   // 48 km along the circle, 11 across
            calmHere *= 1 - 0.3 * inStorm;
            // the swirl's filaments: ridges of cloud wound round the centre, the spiral's arms a few tens of km apart
            const double ts = tw + r / 900;
            h += S * 2.2 * inStorm * (0.35 - std::fabs(cloudNoise3(std::cos(ts) * r / 160, std::sin(ts) * r / 160, r / 24, s0 + 5))) * lodFade(24, L);
        }
    }
    h += S * (conv ? 1.2 : 0.9) * (0.5 + 0.5 * calmHere + 1.2 * inStorm) * streets * lodFade(11, L);   // the cloud streets, rolls along the bands
    h += S * 2.4 * calmHere * (0.3 - std::fabs(cloudNoise2(X / 95, Z / 70, s0 + 2))) * lodFade(70, L);   // the convection's cells: broad domes parted by narrow troughs
    if (low) *low = h;
    double in = 0;
    h += towerAt(k, x, z, &in);
    double amp = S * (conv ? 2.4 : 2.0) * calmHere * (1 + 1.2 * in), lx = 22, lz = 15;   // the billows (steeper the finer), a tower's cauliflower
    for (int o = 0; o < 5; o++) {
        double w = lodFade(std::min(lx, lz), L);
        if (w <= 0) break;
        h += amp * w * (std::fabs(cloudNoise2(X / lx, Z / lz, s0 + 7 + o)) - 0.28);
        amp *= 0.55; lx *= 0.4; lz *= 0.42;
    }
    return h;
}

// the underside hangs in pouches (the folds of a rounded noise) over a slow swell
double GiantDecks::base(int k, double x, double z, double lod, double* swell) const {
    const double S = sh, X = x / S, Z = z / S, L = lod / S;
    const uint32_t s0 = seed32(seed) ^ (0xBA01u + (uint32_t)k);
    double h = lvBase[k] + S * 1.5 * cloudNoise2(X / 60, Z / 34, s0);
    if (swell) *swell = h;
    double amp = S * 1.4, l = 3.4;
    for (int o = 0; o < 2; o++) {
        double w = lodFade(l, L);
        if (w <= 0) break;
        h -= amp * w * std::sqrt(std::max(0.0, (double)cloudNoise2(X / l, Z / l, s0 + 3 + o) + 0.12));
        amp *= 0.45; l *= 0.42;
    }
    return h;
}

// the towers between a point and the sun (the cells the sun's way crosses, each tower once): the sun's way against the
// tower's dome at the closest approach. A tower behind the point (away from the sun) casts nothing on it, nor a tower on itself
double GiantDecks::towerShadow(int k, double x, double y, double z, const Vec3& sun, double maxKm) const {
    double hl = std::hypot(sun.x, sun.z);
    if (sun.y <= 0.004 || hl < 1e-6) return 0;
    const double sx = sun.x / hl, sz = sun.z / hl, tg = sun.y / hl, C = cell[k];
    double reach = std::min(maxKm, (lvTop[k] + (convective(k) ? 22 : 16) * sh * (k == 0 && bands && bands->gsOn ? 2.2 : 1.0) - y) / tg);
    if (reach <= 0) return 0;
    double best = 0;
    int64_t li = INT64_MIN, lj = INT64_MIN;
    for (double u = 0; u <= reach + C * 0.5; u += C * 0.5) {
        int64_t ci = (int64_t)std::floor((x + sx * u) / C), cj = (int64_t)std::floor((z + sz * u) / C);
        if (ci == li && cj == lj) continue;
        li = ci; lj = cj;
        Tower t = towerOf(k, ci, cj);
        if (!t.on) continue;
        double ux = t.cx - x, uz = t.cz - z, along = ux * sx + uz * sz;
        if (along <= 0 || ux * ux + uz * uz < t.r * t.r) continue;   // a tower shades only what lies off it (its own far side is its shading's: a lumped rim would dot)
        double perp = std::fabs(ux * sz - uz * sx);
        if (perp >= t.r) continue;
        double d = perp / t.r;
        double crest = lvTop[k] + lift(k, t.cx, t.cz) + t.h * towerProfile(d);
        double s = smoothstep(-0.4 * sh, 0.4 * sh, crest - (y + along * tg)) * (1 - smoothstep(0.8, 1.0, d));
        if (s > best) best = s;
    }
    return best;
}

// inside a deck: a soft edge under the top and over the base, then a structure of flattened cells (the deck's layers) with
// pockets of clearer air where it falls low
double GiantDecks::density(int k, double x, double y, double z, double topKm, double baseKm) const {
    if (y >= topKm || y <= baseKm) return 0;
    const double S = sh;
    double e = smoothstep(0, 0.35 * S, topKm - y) * smoothstep(0, 0.9 * S, y - baseKm);
    if (e <= 0) return 0;
    const uint32_t s0 = seed32(seed) ^ (0xF0C1u + (uint32_t)k);
    const double X = x / S, Y = y / S, Z = z / S;
    // caverns a few km across where the big cells fall low, the deck's flattened layers and its fine folds elsewhere
    double cav = cloudNoise3(X / 6.5, Y / 2.6, Z / 6.5, s0 + 2);
    double n = 0.55 + 0.9 * cloudNoise3(X / 2.4, Y / 0.9, Z / 2.4, s0) + 0.4 * cloudNoise3(X / 0.8, Y / 0.4, Z / 0.8, s0 + 1);
    n *= smoothstep(-0.32, -0.12, cav);
    return (convective(k) ? 1.6 : 1.25) / S * e * clampd(n, 0.0, 1.4);
}

// X-03: a clear-air hole (a hot spot: the air sinking, the clouds dried out down to the last deck): the descent's own, else one in
// a hashed cell at the giant's share, a little longer east to west than north to south; its wall the outer two fifths of its radius
bool GiantDecks::holeable(int k) const {
    return k < n - 1 && lvBase[k] - lvTop[k + 1] < 1.5 * (lvTop[k] - lvBase[k]) + 10 * sh;
}

double GiantDecks::holeAt(int k, double x, double z) const {
    if (!holeable(k)) return 0;
    double m = 0;
    const Hole& p = path[k];
    if (p.on) { double d = std::hypot((x - p.cx) / 1.5, z - p.cz) / p.r; if (d < 1) m = 1 - smoothstep(0.62, 1.0, d); }
    if (holes > 0) {
        const int64_t ci = (int64_t)std::floor(x / holeCell), cj = (int64_t)std::floor(z / holeCell);
        const uint64_t h = mix64(seed ^ 0x401EULL ^ ((uint64_t)ci * 0x9E3779B97F4A7C15ULL) ^ ((uint64_t)cj * 0xC2B2AE3D27D4EB4FULL));
        if (unitFromHash(h) < holes) {
            const double cx = (ci + 0.25 + 0.5 * unitFromHash(mix64(h ^ 1))) * holeCell, cz = (cj + 0.25 + 0.5 * unitFromHash(mix64(h ^ 2))) * holeCell;
            const double r = (25 + 60 * unitFromHash(mix64(h ^ 3))) * sh, d = std::hypot((x - cx) / 1.5, z - cz) / r;
            if (d < 1) m = std::max(m, 1 - smoothstep(0.62, 1.0, d));
        }
    }
    return m;
}

GiantDecks::Layers GiantDecks::layersAt(double z, const double* px, const double* pz) const {
    Layers L;
    int above = -1;   // the last deck over the probe
    for (int k = 0; k < n; k++) {
        if (k < n - 1 && holeAt(k, px[k], pz[k]) >= 0.5) {
            int kk = k + 1;
            while (kk < n - 1 && holeAt(kk, px[kk], pz[kk]) >= 0.5) kk++;
            if (z > top(kk, px[kk], pz[kk], 0.3)) { L.floor = k; L.ceil = above; L.skyOpen = above < 0; L.inHole = z < lvTop[k] + lift(k, px[k], pz[k]); return L; }
            k = kk - 1;   // under the shaft's floor: the next whole deck from here
            continue;
        }
        const double t = top(k, px[k], pz[k], 0.3), b = base(k, px[k], pz[k], 0.3);
        if (z > t) { L.floor = k; L.ceil = above; L.skyOpen = above < 0; return L; }
        if (z > b) { L.fog = k; if (z > 0.5 * (t + b)) { L.floor = k; L.floorFar = true; } else L.floor = k + 1 < n ? k + 1 : -1; return L; }
        above = k;
    }
    L.ceil = above;
    return L;
}

int giantFoundLayer(const GiantAtmosphere& a, const GiantDecks::Layers& L, double bar) {
    if (L.inHole) return GIANT_HOLE_LAYER + L.floor;
    if (L.fog >= 0) return 2 + 2 * L.fog;
    if (L.ceil >= 0) return 3 + 2 * L.ceil;
    return bar < a.hazeBar ? 0 : 1;
}

std::string giantLayerName(const GiantAtmosphere& a, int layer, bool brief) {
    if (layer >= GIANT_HOLE_LAYER) return brief ? "CLEAR-AIR HOLE" : "A CLEAR-AIR HOLE";
    if (layer <= 1) return brief ? (layer ? "UPPER HAZE" : "ABOVE CLOUDS") : PROBE_STAGE_NAMES[layer ? PS_HAZE : PS_ABOVE];
    const int k = (layer - 2) / 2;
    if (layer & 1) return k >= a.decks - 1 ? (brief ? "THE DEEP" : PROBE_STAGE_NAMES[PS_DEEP]) : (brief ? "CLEAR BAND" : PROBE_STAGE_NAMES[PS_BETWEEN]);
    std::string d = CLOUD_SPECIES[a.species[std::min(k, GIANT_MAX_DECKS - 1)]].deckName;   // "THE AMMONIA DECK"
    if (!brief) return d;
    d = d.substr(4);
    return d.size() > 14 ? d.substr(0, d.size() - 5) : d;   // "HYDROSULPHIDE"
}

int giantStormAt(const Body& b, const BodyGen& g, double lat, double lon, double t, uint64_t& id) {
    id = 0;
    const double lo = lon + giantJetAt(b, g, lat, lon, t);   // the pattern's place under the aim at the time, as giantBandsOf samples it
    if (g.gsOn) {
        const double ex = wrapAngle(lo - g.gsLon) * std::cos(g.gsLat) / g.gsA, ey = (lat - g.gsLat) / g.gsB;
        if (ex * ex + ey * ey < 1) return 2;
    }
    if (giantStormCell(g, StarSystem::bodyFromLatLon(lat, lo), &id) > 0.3) return 1;
    id = 0;
    return 0;
}
