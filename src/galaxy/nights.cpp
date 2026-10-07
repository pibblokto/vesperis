#include "nights.h"
#include "almanac.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>

namespace {
constexpr uint64_t SHOWER_SALT = 0x5A0E7E0B5ULL;
constexpr double STREAM_HALF_WIDTH = 0.03;   // the stream's half-width, of the world's orbit radius
constexpr int COARSE = 4096;                 // the comet's orbit sampled by true anomaly before the refinement

void orbitBasis(const Body& b, Vec3& u, Vec3& v) {
    u = Vec3(std::cos(b.orbitNode), 0, std::sin(b.orbitNode));
    Vec3 v0 = cross(Vec3(0, 1, 0), u);
    v = v0 * std::cos(b.orbitIncl) + Vec3(0, 1, 0) * std::sin(b.orbitIncl);
}

// the distance from a point (relative to the star) to an orbit, measured in the orbit's plane radially and out of it:
// the point's angle in the plane gives the orbit's radius there (planets' orbits are near circles, so this is the least
// distance within a fraction of a percent); `phOut` the angle in the plane (from the node, as `bodyPos` measures it)
double radialDistance(const Body& w, const Vec3& u, const Vec3& v, const Vec3& n, const Vec3& P, double& phOut) {
    double x = dot(P, u), y = dot(P, v), z = dot(P, n);
    double ph = std::atan2(y, x);
    double nu = ph - w.argPeri;
    double r = w.orbitRadiusKm * (1 - w.ecc * w.ecc) / (1 + w.ecc * std::cos(nu));
    double rho = std::sqrt(x * x + y * y);
    phOut = ph;
    return std::sqrt((rho - r) * (rho - r) + z * z);
}

// the speed on an orbit at radius r (vis-viva, the parent's mass from the period) along the orbit's direction of travel
Vec3 orbitVelocityAt(const Body& b, double nu) {
    const double h = 1e-4;
    Vec3 tangent = normalize(orbitPointAt(b, nu + h) - orbitPointAt(b, nu - h));
    double a = b.orbitRadiusKm, r = length(orbitPointAt(b, nu));
    double n = TAU / std::fabs(b.orbitPeriod);
    double gm = n * n * a * a * a;
    double speed = std::sqrt(std::max(0.0, gm * (2 / r - 1 / a)));
    return tangent * (speed * (b.orbitPeriod < 0 ? -1.0 : 1.0));
}

// the time (in [0, period)) at which a body is at the true anomaly nu
double timeAtTrueAnomaly(const Body& b, double nu) {
    double M = nu;
    if (b.ecc > 1e-9) {
        double E = 2 * std::atan(std::sqrt((1 - b.ecc) / (1 + b.ecc)) * std::tan(nu / 2));
        M = E - b.ecc * std::sin(E);
    }
    double T = b.orbitPeriod;
    double t = (M - b.orbitPhase0) / TAU * T;
    double P = std::fabs(T);
    t = std::fmod(t, P);
    if (t < 0) t += P;
    return t;
}

// W-04's almanac finder: the next peak of every shower of the place's world within the window
void findShowers(const AlmanacPlace& p, std::vector<AlmanacEvent>& out) {
    if (!p.ground || !p.sys || p.body < 0 || p.body >= (int)p.sys->bodies.size()) return;
    std::vector<MeteorShower> sh;
    meteorShowersOf(*p.sys, p.body, sh);
    for (const MeteorShower& s : sh) {
        double tp = showerPeakAfter(s, p.t);
        if (tp > p.t + p.window) continue;
        AlmanacEvent e; e.kind = AL_SHOWER; e.t = tp; e.body = s.comet; e.body2 = p.body; e.value = s.strength * SHOWER_PEAK_PER_MIN;
        out.push_back(e);
    }
}
struct ShowerFinderRegistrar { ShowerFinderRegistrar() { addAlmanacFinder(findShowers); } } showerFinderRegistrar;
}   // namespace

Vec3 orbitPointAt(const Body& b, double nu) {
    double r = b.orbitRadiusKm * (1 - b.ecc * b.ecc) / (1 + b.ecc * std::cos(nu));
    double ph = nu + b.argPeri;
    Vec3 u, v; orbitBasis(b, u, v);
    return (u * std::cos(ph) + v * std::sin(ph)) * r;
}

double orbitDistanceKm(const Body& b, const Vec3& relKm) {
    double best = 1e300, bestNu = 0;
    for (int i = 0; i < COARSE; i++) {
        double nu = TAU * i / COARSE;
        double d = length(orbitPointAt(b, nu) - relKm);
        if (d < best) { best = d; bestNu = nu; }
    }
    double lo = bestNu - TAU / COARSE, hi = bestNu + TAU / COARSE;   // golden section round the best sample
    const double g = 0.6180339887498949;
    double a = hi - g * (hi - lo), c = lo + g * (hi - lo);
    double fa = length(orbitPointAt(b, a) - relKm), fc = length(orbitPointAt(b, c) - relKm);
    for (int k = 0; k < 60; k++) {
        if (fa < fc) { hi = c; c = a; fc = fa; a = hi - g * (hi - lo); fa = length(orbitPointAt(b, a) - relKm); }
        else { lo = a; a = c; fa = fc; c = lo + g * (hi - lo); fc = length(orbitPointAt(b, c) - relKm); }
    }
    return std::min(best, std::min(fa, fc));
}

void meteorShowersOf(const StarSystem& sys, int body, std::vector<MeteorShower>& out) {
    out.clear();
    if (body < 0 || body >= (int)sys.bodies.size()) return;
    int wi = sys.bodies[body].parent >= 0 ? sys.bodies[body].parent : body;
    const Body& w = sys.bodies[wi];
    if (w.type == PT_COMET || w.type == PT_COMPANION || w.parent >= 0) return;
    Vec3 u, v; orbitBasis(w, u, v);
    Vec3 n = normalize(cross(u, v));
    const double halfW = STREAM_HALF_WIDTH * w.orbitRadiusKm;
    for (int ci = 0; ci < (int)sys.bodies.size(); ci++) {
        const Body& c = sys.bodies[ci];
        if (c.type != PT_COMET || c.parent >= 0) continue;
        // the comet's orbit by true anomaly against the world's, then the least distance refined round the best sample
        double best = 1e300, bestNu = 0, ph = 0;
        for (int i = 0; i < COARSE; i++) {
            double nu = TAU * i / COARSE;
            double d = radialDistance(w, u, v, n, orbitPointAt(c, nu), ph);
            if (d < best) { best = d; bestNu = nu; }
        }
        if (best > 4 * halfW) continue;
        double lo = bestNu - TAU / COARSE, hi = bestNu + TAU / COARSE;
        const double g = 0.6180339887498949;
        double a = hi - g * (hi - lo), cc = lo + g * (hi - lo);
        double fa = radialDistance(w, u, v, n, orbitPointAt(c, a), ph), fc = radialDistance(w, u, v, n, orbitPointAt(c, cc), ph);
        for (int k = 0; k < 60; k++) {
            if (fa < fc) { hi = cc; cc = a; fc = fa; a = hi - g * (hi - lo); fa = radialDistance(w, u, v, n, orbitPointAt(c, a), ph); }
            else { lo = a; a = cc; fa = fc; cc = lo + g * (hi - lo); fc = radialDistance(w, u, v, n, orbitPointAt(c, cc), ph); }
        }
        double nuC = fa < fc ? a : cc;
        Vec3 P = orbitPointAt(c, nuC);
        double moid = radialDistance(w, u, v, n, P, ph);
        double nuW = ph - w.argPeri;
        Vec3 Q = orbitPointAt(w, nuW);
        // the stream: as old as a hash makes it (0.5..1 of its density), as dense as the comet is active at its periapsis
        double q = c.orbitRadiusKm * (1 - c.ecc);
        double age = 0.5 + 0.5 * unitFromHash(mix64(c.seed ^ SHOWER_SALT));
        double shed = 0.3 + 0.7 * cometActivityAt(sys, q);
        double strength = age * shed * std::exp(-0.5 * (moid / halfW) * (moid / halfW));
        if (strength < SHOWER_MIN_STRENGTH) continue;
        MeteorShower s;
        s.comet = ci; s.world = wi; s.moidKm = moid; s.strength = strength;
        s.period = std::fabs(w.orbitPeriod);
        s.tPeak0 = timeAtTrueAnomaly(w, nuW);
        Vec3 vw = orbitVelocityAt(w, nuW), vc = orbitVelocityAt(c, nuC);
        Vec3 rel = vc - vw;
        s.speedKms = length(rel);
        s.radiant = s.speedKms > 1e-9 ? rel / -s.speedKms : Vec3(0, 1, 0);
        s.sigma = clampd(halfW / std::max(1e-6, length(vw)), 300.0, 86400.0);
        s.streamKm = P; s.worldKm = Q;
        out.push_back(s);
    }
    std::sort(out.begin(), out.end(), [](const MeteorShower& x, const MeteorShower& y) { return x.strength > y.strength; });
}

double showerActivity(const MeteorShower& s, double t) {
    if (s.period <= 0) return 0;
    double dt = std::remainder(t - s.tPeak0, s.period);
    return s.strength * std::exp(-0.5 * (dt / s.sigma) * (dt / s.sigma));
}

double showerPeakAfter(const MeteorShower& s, double t) {
    if (s.period <= 0) return 1e300;
    double k = std::ceil((t - s.tPeak0) / s.period);
    return s.tPeak0 + k * s.period;
}

double zodiacalDust(const StarSystem& sys, int body) {
    if (body < 0 || body >= (int)sys.bodies.size()) return 0;
    int wi = sys.bodies[body].parent >= 0 ? sys.bodies[body].parent : body;
    double r = sys.bodies[wi].orbitRadiusKm;
    double young = 0.1;
    switch (sys.star.cls) {
        case STAR_PROTOSTAR: young = 1.0; break;
        case STAR_WOLF_RAYET: young = 0.5; break;
        case STAR_BLUE_GIANT: young = 0.4; break;
        case STAR_BLUE_WHITE: young = 0.3; break;
        default: break;
    }
    double belt = 0;
    for (const Belt& bt : sys.belts) {
        double mid = 0.5 * (bt.innerKm + bt.outerKm);
        if (mid <= 0 || r <= 0) continue;
        double l = std::log(r / mid) / 0.45;
        belt = std::max(belt, 0.8 * std::exp(-l * l));
    }
    return clampd(young + belt, 0, 1) * (0.6 + 0.4 * unitFromHash(mix64(sys.star.seed ^ 0x2D0D1ACULL)));
}

double zodiacalLight(const Vec3& dirW, const Vec3& sunW) {
    double eps = std::acos(clampd(dot(dirW, sunW), -1, 1));
    double beta = std::asin(clampd(dirW.y, -1, 1));
    double e = std::max(eps, 0.12);
    // the dust's light by latitude: a near-constant width (16 degrees at 30 from the sun, 14 at right angles) under a brightness
    // falling away from the sun, so what shows is a cone broad over the sun's place and narrowing up the plane
    double w = 0.30 - 0.10 * std::min(e, PI) / PI;
    double cone = 0.25 * std::pow(0.6 / e, 1.3) * std::exp(-std::pow(std::fabs(beta) / w, 1.2));   // the latitude's fall nearer an exponential's than a bell's
    double anti = (PI - eps) / 0.15, gegen = 0.07 * std::exp(-anti * anti) * std::exp(-(beta / 0.12) * (beta / 0.12));
    return std::min(1.5, cone + gegen);
}

double bodyShineAt(const StarSystem& sys, int bi, const Vec3& placeKm, double t, double lightFactor) {
    const Body& ob = sys.bodies[bi];
    Vec3 bp = sys.bodyPos(bi, t);
    double dist = length(bp - placeKm);
    double angR = std::asin(clampd(ob.radiusKm / dist, 0, 1));
    double phase = 0.5 * (1 + dot(normalize(sys.star.pos - bp), normalize(placeKm - bp)));
    double illum = phase * PLANET_TYPES[ob.type].albedo * (angR / (5 * DEG)) * (angR / (5 * DEG)) * lightFactor;
    if (ob.type == PT_SUBSTELLAR) illum = std::max(illum, 0.3 * clampd(ob.luminosity / 0.003, 0.55, 1.0) * (angR / (4 * DEG)) * (angR / (4 * DEG)));   // M5-02: warm light of its own
    return std::min(0.5, illum);
}
