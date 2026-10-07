// W-01: the almanac's finders. Every event is a zero crossing or an extremum of a function of the system's geometry
// (the same formulas `SurfaceSite::sun`, the sky and the landing map use), scanned at a step small against the shortest
// period it moves with and refined by bisection or a golden section: the time listed is the time the geometry reaches it.
#include "almanac.h"
#include "fronts.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

// ---- the observer ------------------------------------------------------------------------------------------------
struct Ground { Vec3 up, pos; };   // the place's vertical (world, unit) and its position (world km)

Ground groundAt(const AlmanacPlace& p, double t) {
    const Body& b = p.sys->bodies[p.body];
    Mat3 frameT = p.sys->bodyFrame(p.body, t).transposed();
    Vec3 up = frameT * StarSystem::bodyFromLatLon(p.lat, p.lon);
    return {up, p.sys->bodyPos(p.body, t) + up * b.radiusKm};
}

// the ship's lap round the parked body in orbit mode (`Game::updateShipMotion`: one lap in 600 + 200 R / 6000 s)
double shipLapPeriod(const AlmanacPlace& p) { return p.body >= 0 ? 600.0 + 200.0 * p.sys->bodies[p.body].radiusKm / 6000.0 : 1e18; }

Vec3 observerAt(const AlmanacPlace& p, double t) {
    if (p.ground) return groundAt(p, t).pos;
    if (p.body < 0) return p.pos;
    Vec3 dir = p.parkDir;
    if (p.orbiting) dir = normalize(Mat3::axisAngle(p.sys->bodies[p.body].spinAxis, TAU / shipLapPeriod(p) * (t - p.t)) * dir);
    return p.sys->bodyPos(p.body, t) + dir * p.parkDist;
}

// the sun from the world's centre, as the site reads it (the parallax of the place is under a thousandth of a degree)
Vec3 sunDir(const AlmanacPlace& p, double t, double* angR) {
    Vec3 d = p.sys->star.pos - p.sys->bodyPos(p.body, t);
    double L = length(d);
    if (angR) *angR = std::asin(clampd(p.sys->star.radiusKm / L, 0, 1));
    return d / L;
}
double sunSinAlt(const AlmanacPlace& p, double t, double* angR) { return clampd(dot(sunDir(p, t, angR), groundAt(p, t).up), -1, 1); }

double hourAngle(const AlmanacPlace& p, double t) {   // 0 at the transit, as `SurfaceSite::sun` counts the day
    Vec3 sunBody = p.sys->bodyFrame(p.body, t) * sunDir(p, t, nullptr);
    return wrapAngle(p.lon - std::atan2(sunBody.y, sunBody.x));
}

// a body's altitude over the place's horizon and its angular radius
double bodyAlt(const AlmanacPlace& p, double t, int j, double* angR) {
    Ground g = groundAt(p, t);
    Vec3 rel = p.sys->bodyPos(j, t) - g.pos;
    double d = length(rel);
    if (angR) *angR = std::asin(clampd(p.sys->bodies[j].radiusKm / d, 0, 1));
    return std::asin(clampd(dot(rel / d, g.up), -1, 1));
}

// a body's lit share (the sky's and the landing map's phase), read from the world's centre on the ground: the parallax of
// the place's turn under a moon at five radii would give the share a local maximum every turn near the full
double litShare(const AlmanacPlace& p, double t, int j) {
    Vec3 bp = p.sys->bodyPos(j, t);
    Vec3 obs = p.ground ? p.sys->bodyPos(p.body, t) : observerAt(p, t);
    return 0.5 * (1 + dot(normalize(p.sys->star.pos - bp), normalize(obs - bp)));
}

// M1-08's cover of the sun's disc by a body at an angular separation theta (`SurfaceSite::sun`)
double coverOf(double theta, double rs, double rb) {
    if (theta >= rs + rb) return 0;
    if (theta <= std::fabs(rb - rs)) return rb >= rs ? 1.0 : (rb * rb) / (rs * rs);
    return clampd((rs + rb - theta) / (2 * std::min(rs, rb)), 0, 1) * std::min(1.0, (rb * rb) / (rs * rs));
}

// a body's angular separation from the sun as seen from the observer, with both angular radii
double sunSeparation(const AlmanacPlace& p, double t, int j, double& rs, double& rb, bool* nearer = nullptr) {
    Vec3 obs = observerAt(p, t);
    Vec3 dW; double ds;
    if (p.ground) dW = sunDir(p, t, &rs);
    else { Vec3 d = p.sys->star.pos - obs; ds = length(d); dW = d / ds; rs = std::asin(clampd(p.sys->star.radiusKm / ds, 0, 1)); }
    Vec3 rel = p.sys->bodyPos(j, t) - obs;
    double d = length(rel);
    rb = std::asin(clampd(p.sys->bodies[j].radiusKm / d, 0, 1));
    if (nearer) *nearer = d < length(p.sys->star.pos - obs);
    return std::acos(clampd(dot(rel / d, dW), -1, 1));
}

// O0-01's ring between the place and the sun: the density there, 0 clear
double ringShadow(const AlmanacPlace& p, double t) {
    if (p.ringProf.empty()) return 0;
    const Body& b = p.sys->bodies[p.body];
    Vec3 dW = sunDir(p, t, nullptr);
    Vec3 P = groundAt(p, t).pos - p.sys->bodyPos(p.body, t);
    double denom = dot(dW, b.spinAxis);
    if (std::fabs(denom) <= 1e-9) return 0;
    double s = -dot(P, b.spinAxis) / denom;
    if (s <= 0) return 0;
    double rr = length(P + dW * s);
    double r0 = b.ringInner * b.radiusKm, r1 = b.ringOuter * b.radiusKm;
    if (rr < r0 || rr > r1) return 0;
    return 0.85 * p.ringProf[clampi((int)((rr - r0) / (r1 - r0) * 255), 0, 255)];
}

// ---- the scans ---------------------------------------------------------------------------------------------------
// the zero crossings of f over (t0, t1], sampled every h and refined by bisection to a millisecond or better; `fn(t, dir)`
// takes each (dir +1 upward, -1 downward) and returns true to stop
template <class F, class G> void crossings(F f, double t0, double t1, double h, G fn) {
    double ta = t0, fa = f(ta);
    while (ta < t1) {
        double tb = std::min(ta + h, t1), fb = f(tb);
        if ((fa < 0 && fb >= 0) || (fa > 0 && fb <= 0)) {
            int dir = fa < 0 ? 1 : -1;
            double a = ta, b = tb, va = fa;
            for (int i = 0; i < 48 && b - a > 1e-4; i++) { double m = 0.5 * (a + b), vm = f(m); if ((vm < 0) == (va < 0)) { a = m; va = vm; } else b = m; }
            if (fn(0.5 * (a + b), dir)) return;
        }
        if (tb >= t1) break;
        ta = tb; fa = fb;
    }
}
// the first upward and the first downward crossing (-1 none)
template <class F> void firstCrossings(F f, double t0, double t1, double h, double& up, double& down) {
    up = down = -1;
    crossings(f, t0, t1, h, [&](double t, int dir) { if (dir > 0 && up < 0) up = t; if (dir < 0 && down < 0) down = t; return up >= 0 && down >= 0; });
}
// the local extrema of f over (t0, t1] (maxima when `wantMax`), sampled every h from a step before t0 so one just ahead is
// bracketed, each refined by a golden section of its bracket; `fn(t, value)` takes each and returns true to stop
template <class F, class G> void extrema(F f, double t0, double t1, double h, bool wantMax, G fn) {
    double tp = t0 - h, tc = t0, fp = f(tp), fc = f(tc);
    const double g = 0.6180339887498949;
    while (tc < t1) {
        double tn = tc + h, fn_ = f(tn);
        bool ext = wantMax ? (fc >= fp && fc > fn_) : (fc <= fp && fc < fn_);
        if (ext) {
            double a = tp, b = tn, x1 = b - g * (b - a), x2 = a + g * (b - a), f1 = f(x1), f2 = f(x2);
            for (int i = 0; i < 70 && b - a > 1e-4; i++) {
                if (wantMax ? f1 > f2 : f1 < f2) { b = x2; x2 = x1; f2 = f1; x1 = b - g * (b - a); f1 = f(x1); }
                else { a = x1; x1 = x2; f1 = f2; x2 = a + g * (b - a); f2 = f(x2); }
            }
            double tx = 0.5 * (a + b);
            if (tx > t0 && tx <= t1 && fn(tx, f(tx))) return;
        }
        tp = tc; fp = fc; tc = tn; fc = fn_;
    }
}
// the minima of f below zero: a coarse scan at `hCoarse` finds the dips of the slow motion, a fine scan at `hFine` round
// each (an eighth of the coarse period either side) resolves the fast one (the place's turn, the ship's lap); the first
// minimum under zero stops it. `fn(t, value)` returns true to stop
template <class F, class G> void dips(F f, double t0, double t1, double hCoarse, double hFine, G fn) {
    if (hFine >= hCoarse) { extrema(f, t0, t1, hFine, false, [&](double t, double v) { return v < 0 && fn(t, v); }); return; }
    bool stop = false;
    extrema(f, t0, t1, hCoarse, false, [&](double tc, double) {
        double a = std::max(t0, tc - 8 * hCoarse), b = std::min(t1, tc + 8 * hCoarse);
        extrema(f, a, b, hFine, false, [&](double t, double v) { if (v < 0) { stop = fn(t, v); return stop; } return false; });
        return stop;
    });
}

double stepOf(double period, double div = 64) { return std::max(10.0, std::fabs(period) / div); }

// ---- the ground ----------------------------------------------------------------------------------------------------
void findSun(const AlmanacPlace& p, std::vector<AlmanacEvent>& out) {
    if (!p.ground || p.body < 0) return;
    const Body& b = p.sys->bodies[p.body];
    double h = stepOf(std::min(std::fabs(b.rotPeriod), std::fabs(b.orbitPeriod)));   // the sun moves with the turn, and on a locked world with the orbit alone
    double t0 = p.t, t1 = p.t + p.window, up, down;
    firstCrossings([&](double t) { double ar; double s = sunSinAlt(p, t, &ar); return std::asin(s) + ar; }, t0, t1, h, up, down);
    if (up > 0) out.push_back({AL_SUNRISE, up});
    if (down > 0) out.push_back({AL_SUNSET, down});
    crossings([&](double t) { return std::sin(hourAngle(p, t)); }, t0, t1, h, [&](double t, int) {   // the transit: the hour angle through zero, not through the night's wrap
        if (std::cos(hourAngle(p, t)) > 0) { out.push_back({AL_NOON, t}); return true; }
        return false;
    });
    if (p.atmosphere) {
        firstCrossings([&](double t) { return sunSinAlt(p, t, nullptr) + 0.12; }, t0, t1, h, up, down);
        if (up > 0) out.push_back({AL_DAWN, up});
        if (down > 0) out.push_back({AL_DUSK, down});
    }
}

// the bodies with a disc in the world's sky: at least a quarter of a degree across at their nearest within the window
// (the data sheet's points threshold); the companion sun always. `near` is each one's nearest distance (km)
void skyBodies(const AlmanacPlace& p, std::vector<int>& out) {
    out.clear();
    for (int j = 0; j < (int)p.sys->bodies.size(); j++) {
        if (j == p.body) continue;
        const Body& o = p.sys->bodies[j];
        if (o.type == PT_COMET) continue;
        if (o.type == PT_COMPANION) { out.push_back(j); continue; }
        double dMin = 1e300;
        for (int k = 0; k <= 64; k++) { double t = p.t + p.window * k / 64.0; dMin = std::min(dMin, length(p.sys->bodyPos(j, t) - p.sys->bodyPos(p.body, t)) - p.sys->bodies[p.body].radiusKm); }
        if (dMin > 0 && std::asin(clampd(o.radiusKm / dMin, 0, 1)) >= 0.25 * DEG) out.push_back(j);
    }
}

void findSkyBodies(const AlmanacPlace& p, std::vector<AlmanacEvent>& out) {
    if (!p.ground || p.body < 0) return;
    const Body& b = p.sys->bodies[p.body];
    std::vector<int> sky; skyBodies(p, sky);
    double t0 = p.t, t1 = p.t + p.window;
    for (int j : sky) {
        const Body& o = p.sys->bodies[j];
        double h = stepOf(std::min(std::min(std::fabs(b.rotPeriod), std::fabs(b.orbitPeriod)), std::fabs(o.orbitPeriod)));
        double up, down;
        firstCrossings([&](double t) { double ar; double a = bodyAlt(p, t, j, &ar); return a + ar; }, t0, t1, h, up, down);
        if (up > 0) out.push_back({AL_RISE, up, j});
        if (down > 0) out.push_back({AL_SET, down, j});
        if (o.type == PT_COMPANION || o.type == PT_SUBSTELLAR) continue;   // a sun has no phase
        extrema([&](double t) { return litShare(p, t, j); }, t0, t1, h, true, [&](double t, double v) { out.push_back({AL_FULL, t, j, -1, v}); return true; });
        extrema([&](double t) { return litShare(p, t, j); }, t0, t1, h, false, [&](double t, double v) { out.push_back({AL_NEW, t, j, -1, v}); return true; });
    }
}

void findEclipses(const AlmanacPlace& p, std::vector<AlmanacEvent>& out) {   // a body over the sun from the ground
    if (!p.ground || p.body < 0) return;
    const Body& b = p.sys->bodies[p.body];
    std::vector<int> sky; skyBodies(p, sky);
    for (int j : sky) {
        const Body& o = p.sys->bodies[j];
        if (o.type == PT_COMPANION) continue;
        double hCoarse = stepOf(std::min(std::fabs(o.orbitPeriod), std::fabs(b.orbitPeriod))), hFine = stepOf(std::min(std::fabs(b.rotPeriod), std::fabs(o.orbitPeriod)));
        dips([&](double t) { double rs, rb; return sunSeparation(p, t, j, rs, rb) - rs - rb; }, p.t, p.t + p.window, hCoarse, hFine, [&](double t, double) {
            double rs, rb, theta = sunSeparation(p, t, j, rs, rb);
            double cov = coverOf(theta, rs, rb);
            if (cov > 0.005) out.push_back({AL_ECLIPSE, t, j, -1, cov});
            return cov > 0.005;
        });
    }
}

void findRingShadow(const AlmanacPlace& p, std::vector<AlmanacEvent>& out) {
    if (!p.ground || p.body < 0 || p.ringProf.empty()) return;
    const Body& b = p.sys->bodies[p.body];
    double up, down;
    firstCrossings([&](double t) { return ringShadow(p, t) - 1e-4; }, p.t, p.t + p.window, stepOf(std::min(std::fabs(b.rotPeriod), std::fabs(b.orbitPeriod))), up, down);
    if (up > 0) out.push_back({AL_RING_SHADOW, up, -1, -1, ringShadow(p, up + 1)});
    if (down > 0) out.push_back({AL_RING_CLEAR, down, -1, -1, ringShadow(p, down - 1)});
}

// ---- the system ----------------------------------------------------------------------------------------------------
void findComets(const AlmanacPlace& p, std::vector<AlmanacEvent>& out) {   // from the ground and the ship alike
    for (int j = 0; j < (int)p.sys->bodies.size(); j++) {
        const Body& c = p.sys->bodies[j];
        if (c.type != PT_COMET) continue;
        double M = p.sys->meanAnomaly(j, p.t), P = std::fabs(c.orbitPeriod);
        double peri = p.t + (TAU - M) / TAU * P, apo = p.t + (M < PI ? PI - M : 3 * PI - M) / TAU * P;
        if (peri <= p.t + p.window) out.push_back({AL_PERIAPSIS, peri, j});
        if (apo <= p.t + p.window) out.push_back({AL_APOAPSIS, apo, j});
    }
}

// ---- the ship ------------------------------------------------------------------------------------------------------
// the perpendicular distance of a point from the line from the star through `axisBody`, beyond it (sunward of it the
// distance to the body itself, so the function stays continuous), less `reach`: under zero the point is in the shadow
double shadowGap(const AlmanacPlace& p, double t, int axisBody, const Vec3& point, double reach) {
    Vec3 A = p.sys->bodyPos(axisBody, t);
    Vec3 u = normalize(A - p.sys->star.pos), r = point - A;
    double along = dot(r, u);
    double perp = along > 0 ? std::sqrt(std::max(0.0, length2(r) - along * along)) : length(r);
    return perp - reach;
}

void findShipMoons(const AlmanacPlace& p, std::vector<AlmanacEvent>& out) {
    if (p.ground || p.body < 0) return;
    const Body& B = p.sys->bodies[p.body];
    double lap = p.orbiting ? shipLapPeriod(p) : 1e18;
    for (int m = 0; m < (int)p.sys->bodies.size(); m++) {
        const Body& o = p.sys->bodies[m];
        if (o.parent != p.body) continue;
        double t0 = p.t, t1 = p.t + std::min(p.window, std::max(2 * 86400.0, 3 * std::fabs(o.orbitPeriod)));   // a moon's events come round every orbit
        double h = stepOf(std::min(std::fabs(o.orbitPeriod), lap), 32);
        // first contact with the body's disc in the ship's sky, in front of it (a transit) or behind it (an occultation)
        auto contact = [&](double t) {
            Vec3 S = observerAt(p, t), rb = p.sys->bodyPos(p.body, t) - S, rm = p.sys->bodyPos(m, t) - S;
            double db = length(rb), dm = length(rm);
            return std::acos(clampd(dot(rb / db, rm / dm), -1, 1)) - std::asin(clampd(B.radiusKm / db, 0, 1)) - std::asin(clampd(o.radiusKm / dm, 0, 1));
        };
        auto inFront = [&](double t) { Vec3 S = observerAt(p, t); return length(p.sys->bodyPos(m, t) - S) < length(p.sys->bodyPos(p.body, t) - S); };
        bool transit = false, occult = false;
        crossings(contact, t0, t1, h, [&](double t, int dir) {
            if (dir > 0) return false;
            bool front = inFront(t);
            if (front && !transit) { transit = true; out.push_back({AL_TRANSIT, t, m, p.body}); }
            if (!front && !occult) { occult = true; out.push_back({AL_OCCULTATION, t, m, p.body}); }
            return transit && occult;
        });
        double hs = stepOf(o.orbitPeriod);
        crossings([&](double t) { return shadowGap(p, t, m, p.sys->bodyPos(p.body, t), B.radiusKm + o.radiusKm); }, t0, t1, hs, [&](double t, int dir) { if (dir < 0) out.push_back({AL_SHADOW, t, m, p.body}); return dir < 0; });
        crossings([&](double t) { return shadowGap(p, t, p.body, p.sys->bodyPos(m, t), B.radiusKm); }, t0, t1, hs, [&](double t, int dir) { if (dir < 0) out.push_back({AL_ECLIPSED, t, m, p.body}); return dir < 0; });
    }
}

void findConjunctions(const AlmanacPlace& p, std::vector<AlmanacEvent>& out) {   // two planets within two degrees in the ship's sky
    if (p.ground) return;
    // read from the parked body's centre: the ship's lap moves the nearest planet by a degree in fifty minutes, which would
    // put a minimum into every lap of a scan at the planets' step; the meeting is the planets' own and its time hardly moves
    auto observer = [&](double t) { return p.body >= 0 ? p.sys->bodyPos(p.body, t) : p.pos; };
    std::vector<int> planets;
    for (int j = 0; j < (int)p.sys->bodies.size(); j++) { const Body& o = p.sys->bodies[j]; if (j != p.body && o.type != PT_COMET && (o.parent < 0 || o.type == PT_COMPANION)) planets.push_back(j); }
    for (size_t a = 0; a < planets.size(); a++)
        for (size_t c = a + 1; c < planets.size(); c++) {
            int i = planets[a], j = planets[c];
            double h = stepOf(std::min(std::fabs(p.sys->bodies[i].orbitPeriod), std::fabs(p.sys->bodies[j].orbitPeriod)));
            auto sep = [&](double t) { Vec3 S = observer(t); return std::acos(clampd(dot(normalize(p.sys->bodyPos(i, t) - S), normalize(p.sys->bodyPos(j, t) - S)), -1, 1)); };
            extrema(sep, p.t, p.t + p.window, h, false, [&](double t, double v) { if (v < 2 * DEG) out.push_back({AL_CONJUNCTION, t, i, j, v}); return v < 2 * DEG; });
        }
}

void findSunTransits(const AlmanacPlace& p, std::vector<AlmanacEvent>& out) {   // a body touches the sun's disc as seen from the ship
    if (p.ground) return;
    double lap = p.orbiting ? shipLapPeriod(p) : 1e18;
    for (int j = 0; j < (int)p.sys->bodies.size(); j++) {
        const Body& o = p.sys->bodies[j];
        if (j == p.body || o.type == PT_COMET || o.type == PT_COMPANION) continue;
        if (o.parent >= 0 && o.parent != p.body) continue;   // the planets and the parked body's moons
        double hCoarse = stepOf(o.orbitPeriod), hFine = stepOf(std::min(std::fabs(o.orbitPeriod), lap), 32);
        double t1 = p.t + (o.parent >= 0 ? std::min(p.window, std::max(2 * 86400.0, 3 * std::fabs(o.orbitPeriod))) : p.window);   // a moon's crossings come round every orbit
        dips([&](double t) { double rs, rb; return sunSeparation(p, t, j, rs, rb) - rs - rb; }, p.t, t1, hCoarse, hFine, [&](double tMin, double) {
            double rs, rb; bool nearer; sunSeparation(p, tMin, j, rs, rb, &nearer);
            if (!nearer) return false;
            // back from the deepest to the first contact: a crossing lasts under a thirty-second of the orbit (a sun of ten degrees' radius)
            double tFirst = tMin;
            crossings([&](double t) { double r1, r2; return sunSeparation(p, t, j, r1, r2) - r1 - r2; }, std::max(p.t, tMin - 2 * hCoarse), tMin, std::min(hFine, hCoarse) / 4, [&](double t, int dir) { if (dir < 0) tFirst = t; return dir < 0; });
            out.push_back({AL_SUN_TRANSIT, tFirst, j, -1, coverOf(0, rs, rb)});
            return true;
        });
    }
}

std::vector<AlmanacFinder>& registry() {
    static std::vector<AlmanacFinder> r = {findSun, findSkyBodies, findEclipses, findRingShadow, findComets, findShipMoons, findConjunctions, findSunTransits};
    return r;
}

std::string shortUpper(const std::string& s, size_t n) {
    std::string r;
    for (char c : s) r += (char)toupper((unsigned char)c);
    return r.size() <= n ? r : r.substr(0, n - 1) + ".";
}

}   // namespace

void addAlmanacFinder(AlmanacFinder fn) { registry().push_back(fn); }

void almanacOf(const AlmanacPlace& p, std::vector<AlmanacEvent>& out) {
    out.clear();
    if (!p.sys || p.sys->bodies.empty()) return;
    if (p.body >= (int)p.sys->bodies.size()) return;
    if (p.ground && p.body < 0) return;
    for (AlmanacFinder f : registry()) f(p, out);
    out.erase(std::remove_if(out.begin(), out.end(), [&](const AlmanacEvent& e) { return !(e.t > p.t) || e.t > p.t + p.window || !std::isfinite(e.t); }), out.end());
    std::stable_sort(out.begin(), out.end(), [](const AlmanacEvent& a, const AlmanacEvent& b) { return a.t < b.t; });
}

double almanacSunAltitude(const AlmanacPlace& p, double t, double* angR) {
    if (!p.ground || !p.sys || p.body < 0 || p.body >= (int)p.sys->bodies.size()) { if (angR) *angR = 0; return 0; }
    return std::asin(sunSinAlt(p, t, angR));
}

std::string almanacLabel(const AlmanacEvent& e, const StarSystem& sys, const std::function<std::string(int)>& nameOf, const char* degree) {
    auto N = [&](int j) { return shortUpper(nameOf(j), 12); };
    auto isSun = [&](int j) { return j >= 0 && j < (int)sys.bodies.size() && sys.bodies[j].type == PT_COMPANION; };
    char buf[64];
    switch (e.kind) {
        case AL_SUNRISE: return "SUNRISE";
        case AL_SUNSET: return "SUNSET";
        case AL_NOON: return "NOON";
        case AL_DUSK: return "END OF TWILIGHT";
        case AL_DAWN: return "FIRST LIGHT";
        case AL_RISE: return isSun(e.body) ? "SECOND SUN RISES" : N(e.body) + " RISES";
        case AL_SET: return isSun(e.body) ? "SECOND SUN SETS" : N(e.body) + " SETS";
        case AL_FULL: return N(e.body) + " FULL";
        case AL_NEW: return N(e.body) + " NEW";
        case AL_ECLIPSE: snprintf(buf, sizeof buf, " OVER THE SUN %.0f%%", e.value * 100); return N(e.body) + buf;
        case AL_RING_SHADOW: return "INTO THE RING SHADOW";
        case AL_RING_CLEAR: return "OUT OF THE RING SHADOW";
        case AL_PERIAPSIS: return N(e.body) + " AT PERIAPSIS";
        case AL_APOAPSIS: return N(e.body) + " AT APOAPSIS";
        case AL_TRANSIT: return N(e.body) + " ACROSS " + N(e.body2);
        case AL_OCCULTATION: return N(e.body) + " BEHIND " + N(e.body2);
        case AL_SHADOW: return "SHADOW OF " + N(e.body) + " ON " + N(e.body2);
        case AL_ECLIPSED: return N(e.body) + " ECLIPSED BY " + N(e.body2);
        case AL_CONJUNCTION: snprintf(buf, sizeof buf, " %.1f%s APART", e.value / DEG, degree); return N(e.body) + " AND " + N(e.body2) + buf;
        case AL_SUN_TRANSIT: return N(e.body) + " ACROSS THE SUN";
        case AL_FRONT: return std::string(e.body >= 0 && e.body < (int)sys.bodies.size() && (sys.bodies[e.body].type == PT_DESERT || sys.bodies[e.body].type == PT_THINATMO) ? "A DUST FRONT FROM THE " : "A FRONT FROM THE ") + frontCompass(e.value);   // W-03
        case AL_FRONT_CLEAR: return "THE FRONT CLEARS";
        case AL_SHOWER: return N(e.body) + " METEORS PEAK";   // W-04
        case AL_FREE: return "RUN FOR " + countdownString(e.value);
        default: return "?";
    }
}

std::string countdownString(double secs) {
    char buf[48];
    if (secs < 0) secs = 0;
    if (secs < 90) snprintf(buf, sizeof buf, "%.0f S", secs);
    else if (secs < 3600) snprintf(buf, sizeof buf, "%.0f MIN", std::floor(secs / 60));
    else if (secs < 48 * 3600) snprintf(buf, sizeof buf, "%d H %02d MIN", (int)(secs / 3600), (int)std::fmod(secs / 60, 60));
    else snprintf(buf, sizeof buf, "%d D %d H", (int)(secs / 86400), (int)std::fmod(secs / 3600, 24));
    return buf;
}
