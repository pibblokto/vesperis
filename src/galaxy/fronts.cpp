// W-03: the fronts (see fronts.h). Every number here is a hash of the body's seed, the slot and the epoch under the fronts'
// own salt, so the fronts are a new stream beside the planet function: no world, map or site of an older save moves.
#include "fronts.h"
#include "almanac.h"
#include "core/rng.h"
#include "core/parallel.h"
#include "core/noise.h"
#include <cmath>
#include <algorithm>

namespace {
constexpr uint64_t FRONT_SALT = 0x5745415448455231ULL;

// a slot's rhythm: a front every two to four days, at its own phase
void slotRhythm(const Body& b, int k, double& P, double& phase) {
    Rng r(hash3i(k, 0, 0, b.seed ^ FRONT_SALT));
    P = (2 + 2 * r.uni()) * 86400.0;
    phase = r.uni() * P;
}

Front makeFront(const Body& b, int k, int64_t e, double P, double phase) {
    Rng r(hash3i(k, e, 1, b.seed ^ FRONT_SALT));
    Front f; f.slot = k; f.epoch = e;
    f.R = b.radiusKm * 1000.0;
    f.born = e * P - phase + r.uni() * 0.3 * P;
    f.dies = f.born + (0.4 + 0.35 * r.uni()) * P;
    double sign = r.uni() < 0.5 ? -1 : 1;
    double lat = sign * (20 + 50 * r.uni()) * DEG;
    double lon = r.uni() * TAU;
    f.c0 = StarSystem::bodyFromLatLon(lat, lon);
    Vec3 z(0, 0, 1);
    Vec3 east = normalize(cross(z, f.c0)), north = cross(f.c0, east);
    // the heading: east with a poleward lean in the mid-latitudes, west in the tropics
    double bearing = std::fabs(lat) > 25 * DEG ? 90 * DEG - sign * (10 + 30 * r.uni()) * DEG : 270 * DEG + (r.uni() - 0.5) * 40 * DEG;
    f.n0 = normalize(east * std::sin(bearing) + north * std::cos(bearing));
    f.a = normalize(cross(f.c0, f.n0));
    f.speed = 8 + 17 * r.uni();
    f.halfLen = std::min((1200 + 1800 * r.uni()) * 1e3, 0.35 * PI * f.R);   // a small world's fronts are shorter than its girth
    f.width = 0.75 + 0.5 * r.uni();
    f.strength = 0.45 + 0.55 * r.uni();
    f.tempDrop = 3 + 8 * r.uni();
    f.bow = (r.uni() < 0.5 ? -1 : 1) * (200 + 400 * r.uni()) * 1e3;
    f.seed = b.seed ^ FRONT_SALT;
    f.dust = b.type == PT_DESERT || b.type == PT_THINATMO;
    return f;
}

// the bearing (radians from north, clockwise) of a body-frame tangent direction at p
double bearingOf(const Vec3& dir, const Vec3& p) {
    Vec3 east = cross(Vec3(0, 0, 1), p);
    double L = length(east);
    if (L < 1e-9) return 0;
    east = east / L;
    Vec3 north = cross(p, east);
    return wrap2pi(std::atan2(dot(dir, east), dot(dir, north)));
}

// W-03's almanac finder: the arrival and the clearing of every front that reaches the place within the window
void findFronts(const AlmanacPlace& p, std::vector<AlmanacEvent>& out) {
    if (!p.ground || !p.sys || p.body < 0 || p.body >= (int)p.sys->bodies.size()) return;
    const Body& b = p.sys->bodies[p.body];
    if (!worldHasFronts(b)) return;
    std::vector<FrontForecast> fc;
    frontsAhead(b, StarSystem::bodyFromLatLon(p.lat, p.lon), p.t, p.window, fc);
    for (const FrontForecast& f : fc) {
        AlmanacEvent e; e.kind = AL_FRONT; e.t = f.tArrive; e.body = p.body; e.value = f.fromBearing; out.push_back(e);
        AlmanacEvent e2; e2.kind = AL_FRONT_CLEAR; e2.t = f.tClear; e2.body = p.body; e2.value = f.fromBearing; out.push_back(e2);
    }
}
struct FrontFinderRegistrar { FrontFinderRegistrar() { addAlmanacFinder(findFronts); } } frontFinderRegistrar;
}   // namespace

double frontCloudProfile(double x) { return smoothstep(280e3, 180e3, x) * smoothstep(-330e3, -200e3, x); }
double frontCloudCore(double x) { return smoothstep(100e3, 90e3, x) * smoothstep(-260e3, -130e3, x); }
double frontVeilProfile(double x) { return smoothstep(520e3, 250e3, x) * smoothstep(-330e3, -200e3, x); }
double frontRainProfile(double x) { if (x > 0) return 0; return x > -25e3 ? smoothstep(0, -25e3, x) : smoothstep(FRONT_RAIN_END, -25e3, x); }
double frontWindProfile(double x) { return x >= 0 ? smoothstep(350e3, 30e3, x) : smoothstep(-250e3, -80e3, x); }
double frontColdProfile(double x) { return smoothstep(20e3, -80e3, x) * smoothstep(-900e3, -500e3, x); }

bool worldHasFronts(const Body& b) {
    if (!PLANET_TYPES[b.type].atmosphere || hasOpaqueDeck(b.type)) return false;
    return b.type == PT_FELISIAN || b.type == PT_OCEAN || b.type == PT_DESERT || b.type == PT_THINATMO || b.type == PT_ACIDIC || b.type == PT_QUARTZ;
}

int frontSlots(const Body& b) {
    double r = b.radiusKm / 6000.0;
    return clampi((int)std::lround(6 + 14 * r * r), 3, 24);
}

void frontsOf(const Body& b, double t, std::vector<Front>& out, double ahead) {
    out.clear();
    if (!worldHasFronts(b)) return;
    int ns = frontSlots(b);
    for (int k = 0; k < ns; k++) {
        double P, phase; slotRhythm(b, k, P, phase);
        int64_t e0 = (int64_t)std::floor((t + phase) / P) - 1, e1 = (int64_t)std::floor((t + ahead + phase) / P);
        for (int64_t e = e0; e <= e1; e++) {
            Front f = makeFront(b, k, e, P, phase);
            if (f.dies <= t || f.born > t + ahead) continue;
            out.push_back(f);
        }
    }
}

void frontAt(const Front& f, double t, Vec3& c, Vec3& n) {
    Mat3 rot = Mat3::axisAngle(f.a, f.speed * (t - f.born) / f.R);
    c = rot * f.c0; n = rot * f.n0;
}

double frontLife(const Front& f, double t) {
    if (t <= f.born || t >= f.dies) return 0;
    double u = (t - f.born) / (f.dies - f.born);
    return smoothstep(0.0, 0.15, u) * smoothstep(1.0, 0.8, u);
}

void frontDistance(const Front& f, double t, const Vec3& p, double R, double& ahead, double& along) {
    Vec3 c, n; frontAt(f, t, c, n);
    if (dot(p, c) < 0.05) { ahead = 1e18; along = 1e18; return; }
    along = R * std::asin(clampd(dot(p, f.a), -1, 1));
    double u = along / f.halfLen;
    ahead = R * std::asin(clampd(dot(p, n), -1, 1)) - f.bow * u * u;   // the bowed line: its ends trail (or lead) its centre
}

double frontTexture(const Front& f, const Vec3& p) {
    double n = 0.5 + 0.5 * fbm3(p * 14.0, f.seed, 3, 2.1, 0.55);
    return clampd((n - 0.3) / 0.35, 0, 1);
}

double frontLateral(const Front& f, double along) { return smoothstep(f.halfLen, 0.8 * f.halfLen, std::fabs(along)); }

void frontWeatherAt(const std::vector<Front>& fronts, const Vec3& p, double t, double R, FrontWeather& out) {
    out = FrontWeather();
    double bestW = 0, bestAbs = 1e18;
    for (size_t i = 0; i < fronts.size(); i++) {
        const Front& f = fronts[i];
        double life = frontLife(f, t);
        if (life <= 0) continue;
        double ahead, along; frontDistance(f, t, p, R, ahead, along);
        if (ahead > 1e17 || ahead > FRONT_REACH_AHEAD * f.width || ahead < FRONT_REACH_BEHIND * f.width) continue;
        double lat = frontLateral(f, along);
        if (lat <= 0) continue;
        double g = life * lat, x = ahead / f.width, tex = frontTexture(f, p);
        out.cloud = std::max(out.cloud, frontCloudAt(x, tex) * g);
        out.precip = std::max(out.precip, frontRainProfile(x) * f.strength * g);
        out.windRise = std::max(out.windRise, frontWindProfile(x) * f.strength * g);
        out.cold = std::max(out.cold, frontColdProfile(x) * f.tempDrop * g);
        double w = std::max(frontWindProfile(x), frontColdProfile(x)) * g;
        if (w > bestW) {   // the wind: along the line toward the pole ahead of it, along the motion behind it, veering across the line
            bestW = w; out.windWeight = w;
            Vec3 c, n; frontAt(f, t, c, n);
            Vec3 aP = (f.a.z * (p.z >= 0 ? 1 : -1) < 0) ? f.a * -1.0 : f.a;
            double veer = smoothstep(-40e3, 40e3, x);
            Vec3 wt = aP * veer + n * (1 - veer);
            wt = wt - p * dot(wt, p);
            double L = length(wt);
            out.windTo = L > 1e-9 ? wt / L : n;
        }
        if (std::fabs(ahead) < bestAbs) { bestAbs = std::fabs(ahead); out.nearest = (int)i; out.nearestAhead = ahead; out.nearestAlong = along; out.nearestGain = g; out.nearestTex = tex; }
    }
}

void frontsAhead(const Body& b, const Vec3& p, double t, double window, std::vector<FrontForecast>& out) {
    out.clear();
    if (!worldHasFronts(b)) return;
    const double BACK = 8 * 3600.0, h = 600;
    std::vector<Front> fs; frontsOf(b, t - BACK, fs, window + BACK);
    double R = b.radiusKm * 1000.0;
    for (const Front& f : fs) {
        double t0 = std::max(f.born, t - BACK), t1 = std::min(f.dies, t + window);
        if (t1 <= t0) continue;
        auto aheadAt = [&](double T) { double ah, al; frontDistance(f, T, p, R, ah, al); return ah; };
        double Tp = t0, vp = aheadAt(t0), Ta = -1;
        for (double T2 = t0 + h; ; T2 += h) {   // the line reaches the place: the distance ahead through zero (it falls at the speed at most, so no step skips it)
            double Tc = std::min(T2, t1), v = aheadAt(Tc);
            if (vp > 0 && v <= 0 && vp < 1e17) {
                double lo = Tp, hi = Tc;
                for (int i = 0; i < 48; i++) { double mid = 0.5 * (lo + hi); if (aheadAt(mid) > 0) lo = mid; else hi = mid; }
                Ta = hi; break;
            }
            if (Tc >= t1) break;
            Tp = Tc; vp = v;
        }
        if (Ta < 0) continue;
        double ah, al; frontDistance(f, Ta, p, R, ah, al);
        double life = frontLife(f, Ta), lat = frontLateral(f, al);
        if (life < 0.25 || lat < 0.5) continue;   // a front dying or ending here brings no weather worth a line
        FrontForecast fc;
        fc.tArrive = Ta; fc.strength = f.strength; fc.gain = life * lat; fc.dust = f.dust; fc.slot = f.slot; fc.epoch = f.epoch;
        Vec3 c, n; frontAt(f, Ta, c, n);
        Vec3 from = (n - p * dot(n, p)) * -1.0;
        fc.fromBearing = bearingOf(from, p);
        // the rain ends where the line is 150 km past the place (times the width), or when the front dies
        double endAt = FRONT_RAIN_END * f.width, Tc = f.dies;
        for (double T2 = Ta + h; T2 < f.dies; T2 += h) {
            if (aheadAt(T2) <= endAt) {
                double lo = T2 - h, hi = T2;
                for (int i = 0; i < 48; i++) { double mid = 0.5 * (lo + hi); if (aheadAt(mid) > endAt) lo = mid; else hi = mid; }
                Tc = hi; break;
            }
        }
        fc.tClear = Tc;
        out.push_back(fc);
    }
    std::sort(out.begin(), out.end(), [](const FrontForecast& a, const FrontForecast& b2) { return a.tArrive < b2.tArrive; });
}

void buildFrontMap(const Body& b, double t, int W, int H, std::vector<uint8_t>& out) {
    out.assign((size_t)W * H, 0);
    std::vector<Front> fs; frontsOf(b, t, fs, 0);
    struct Live { Vec3 c, n, a; double life, width, halfLen, capCos; const Front* f; };
    std::vector<Live> live;
    for (const Front& f : fs) {
        double life = frontLife(f, t);
        if (life <= 0) continue;
        Live L; frontAt(f, t, L.c, L.n); L.a = f.a; L.life = life; L.width = f.width; L.halfLen = f.halfLen; L.f = &f;
        double cap = std::hypot(f.halfLen, FRONT_REACH_AHEAD * f.width + std::fabs(f.bow)) / f.R;   // the band's reach from its centre
        L.capCos = std::cos(std::min(cap, PI * 0.5 - 0.01));
        live.push_back(L);
    }
    if (live.empty()) return;
    double R = b.radiusKm * 1000.0;
    std::vector<double> cl(W), sl(W);
    for (int x = 0; x < W; x++) { double lon = ((x + 0.5) / W) * TAU - PI; cl[x] = std::cos(lon); sl[x] = std::sin(lon); }
    parallelFor(H, 8, [&](int y0, int y1) {
        for (int y = y0; y < y1; y++) {
            double lat = (0.5 - (y + 0.5) / H) * PI, cla = std::cos(lat), sla = std::sin(lat);
            for (int x = 0; x < W; x++) {
                Vec3 p(cla * cl[x], cla * sl[x], sla);
                double best = 0;
                for (const Live& L : live) {
                    if (dot(p, L.c) < L.capCos) continue;
                    double along = R * std::asin(clampd(dot(p, L.a), -1, 1));
                    double lat2 = frontLateral(*L.f, along);
                    if (lat2 <= 0) continue;
                    double u = along / L.halfLen;
                    double ahead = R * std::asin(clampd(dot(p, L.n), -1, 1)) - L.f->bow * u * u;   // as `frontDistance` reads it
                    double v = frontCloudProfile(ahead / L.width) * L.life * lat2;
                    if (v > best) { v = frontCloudAt(ahead / L.width, frontTexture(*L.f, p)) * L.life * lat2; if (v > best) best = v; }
                }
                out[(size_t)y * W + x] = (uint8_t)(best * 255 + 0.5);
            }
        }
    });
}

double frontMapAt(const std::vector<uint8_t>& m, int W, int H, double lon, double lat) {
    if ((int)m.size() != W * H || W < 2 || H < 2) return 0;
    double u = (lon + PI) * (1.0 / TAU);   // the longitude from atan2 lies in [-pi, pi]: one fold at each end, no modulo
    if (u < 0) u += 1; else if (u >= 1) u -= 1;
    double fx = u * W - 0.5, fy = (0.5 - lat / PI) * H - 0.5;
    int x0 = (int)std::floor(fx), y0 = (int)std::floor(fy);
    double tx = fx - x0, ty = fy - y0;
    int x1 = x0 + 1;
    if (x0 < 0) x0 += W;
    if (x1 >= W) x1 -= W;
    int y1 = y0 + 1;
    if (y0 < 0) y0 = 0; else if (y0 >= H) y0 = H - 1;
    if (y1 < 0) y1 = 0; else if (y1 >= H) y1 = H - 1;
    const uint8_t* r0 = &m[(size_t)y0 * W]; const uint8_t* r1 = &m[(size_t)y1 * W];
    unsigned a = r0[x0], bb = r0[x1], c = r1[x0], d = r1[x1];
    if ((a | bb | c | d) == 0) return 0;
    double top = a + (double)((int)bb - (int)a) * tx, bottom = c + (double)((int)d - (int)c) * tx;
    return (top + (bottom - top) * ty) * (1.0 / 255.0);
}

const char* frontCompass(double bearing) {
    static const char* names[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    return names[(int)std::floor(wrap2pi(bearing) / TAU * 8 + 0.5) & 7];
}
