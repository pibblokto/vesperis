// Z-02: a specimen's shape at a time (life/morph_shape.h): the outline, the tube and the mosaic of a wall; where a specimen is (its motion,
// closed-form in the clock); the builders of every plan's cell (life/morph_plans.cpp) and the acts that change them
#include "life/morph_shape_internal.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

namespace morph {

double outlineAt(const Body& b, double phi) {
    double f = phi * (RTAB / TAU);
    f -= std::floor(f / RTAB) * RTAB;
    const int i = (int)f % RTAB, j = (i + 1) % RTAB;
    const double t = f - std::floor(f);
    return b.r[i] + (b.r[j] - b.r[i]) * t;
}

double radialDistance(const Body& b, double px, double py, double& u, double& v, double& phi) {
    const double dx = px - b.c.x, dy = py - b.c.y;
    u = dx * b.ca + dy * b.sa;
    v = -dx * b.sa + dy * b.ca;
    const double L = b.R * b.aspect;
    if (b.bend != 0) v -= b.bend * u * u / L;
    double sc = 1;
    if (b.tail > 0 && u < 0) sc = 1 - b.tail * 0.82 * std::pow(std::min(1.0, -u / L), 1.4);
    else if (b.head > 0 && u > 0) sc = 1 - b.head * 0.82 * std::pow(std::min(1.0, u / L), 1.4);
    sc = std::max(sc, 0.1);
    const double qx = u / b.aspect, qy = v / sc;
    const double rho = std::sqrt(qx * qx + qy * qy);
    phi = std::atan2(qy, qx);
    const double rr = b.R * outlineAt(b, phi);
    const double g = rho > 1e-9 ? std::sqrt((qx / b.aspect) * (qx / b.aspect) + (qy / sc) * (qy / sc)) / rho : 1.0;
    return (rho - rr) / std::max(0.1, g);
}

double tubeDistance(const Body& b, double px, double py, int& seg, double& t, double& side) {
    double best = 1e30;
    seg = 0; t = 0; side = 0;
    const int n = (int)b.line.size();
    if (n == 1) { seg = 0; t = 0; const double dx = px - b.line[0].x, dy = py - b.line[0].y; return std::sqrt(dx * dx + dy * dy) - b.rad[0]; }
    for (int i = 0; i + 1 < n; i++) {
        const P2 a = b.line[i], c = b.line[i + 1];
        const double ex = c.x - a.x, ey = c.y - a.y, l2 = ex * ex + ey * ey;
        double q = l2 > 0 ? ((px - a.x) * ex + (py - a.y) * ey) / l2 : 0;
        q = std::max(0.0, std::min(1.0, q));
        const double cx = a.x + ex * q - px, cy = a.y + ey * q - py;
        const double d = std::sqrt(cx * cx + cy * cy) - (b.rad[i] + (b.rad[i + 1] - b.rad[i]) * q);
        if (d < best) { best = d; seg = i; t = q; side = ex * (py - a.y) - ey * (px - a.x) > 0 ? 1 : -1; }
    }
    return best;
}

namespace {
inline double h01(uint64_t h) { return unitFromHash(mix64(h)); }   // re-mixed: a shifted hash would span [0, 2^-n) alone
// a jittered lattice's nearest two points (a mosaic's cells) in 3D: the distance to the nearest, to the second, the nearest's id
void cellular(double x, double y, double z, uint64_t seed, double jitter, double& d1, double& d2, uint64_t& id) {
    const int64_t ix = (int64_t)std::floor(x), iy = (int64_t)std::floor(y), iz = (int64_t)std::floor(z);
    d1 = d2 = 1e9; id = 0;
    for (int64_t a = ix - 1; a <= ix + 1; a++)
        for (int64_t b = iy - 1; b <= iy + 1; b++)
            for (int64_t c = iz - 1; c <= iz + 1; c++) {
                const uint64_t h = hash3i(a, b, c, seed);
                const double px = a + 0.5 + jitter * (h01(h) - 0.5), py = b + 0.5 + jitter * (h01(h >> 5 | h << 59) - 0.5),
                             pz = c + 0.5 + jitter * (h01(mix64(h)) - 0.5);
                const double d = (px - x) * (px - x) + (py - y) * (py - y) + (pz - z) * (pz - z);
                if (d < d1) { d2 = d1; d1 = d; id = h; } else if (d < d2) d2 = d;
            }
    d1 = std::sqrt(d1); d2 = std::sqrt(d2);
}
}   // namespace

double markAt(int mark, int marks, uint64_t seed, double a, double b, double c, int* cell) {
    // (a, b, c): a radial body's unit direction (its surface's point), or a tube's (along in widths, cos round, sin round)
    const double n = std::max(2, marks);
    switch (mark) {
        case MK_PORES: case MK_LATTICE: {
            double d1, d2; uint64_t id;
            const double k = n * 0.32;
            cellular(a * k, b * k, c * k, seed, mark == MK_LATTICE ? 0.35 : 0.6, d1, d2, id);
            const double hole = mark == MK_LATTICE ? 0.36 : 0.22;
            if (cell) *cell = (int)(id & 0xFFFF);
            return d1 < hole ? -1.0 : (d1 < hole * 1.35 ? 0.6 : 0.0);
        }
        case MK_RIBS: {
            const double ang = std::atan2(b, a);
            const double w = std::sin(ang * n * 0.5 + c * 1.5);
            return w > 0.75 ? 1.0 : (w > 0.5 ? 0.4 : 0.0);
        }
        case MK_PLATES: case MK_NET: case MK_GRAINS: {
            double d1, d2; uint64_t id;
            const double k = n * (mark == MK_NET ? 0.45 : (mark == MK_PLATES ? 0.11 : 0.28));
            cellular(a * k, b * k, c * k, seed, 0.9, d1, d2, id);
            if (cell) *cell = (int)(id & 0xFFFF);
            const double edge = d2 - d1;
            if (mark == MK_GRAINS) return edge < 0.07 ? -0.5 : 0.25 + 0.5 * h01(id);   // a grain's own relief
            return edge < (mark == MK_NET ? 0.09 : (mark == MK_PLATES ? 0.035 : 0.06)) ? 1.0 : 0.0;
        }
        default: return 0;
    }
}

// the flat layouts: a centric box's radial rows of pores from a clear centre to its rim; a pennate one's rows across a clear raphe; stripes
// along it; a euglenoid's helical strips
double flatOrnament(const Body& b, double u, double v) {
    const Look& L = b.look;
    const double n = std::max(2, L.marks), R = b.R, Lh = R * b.aspect;
    if (L.layout == 1) {
        const double rho = std::sqrt((u / b.aspect) * (u / b.aspect) + v * v) / R, ang = std::atan2(v, u / b.aspect);
        if (rho < 0.14) return 0;
        if (rho > 0.93) return 0.8;
        if (L.mark == MK_RIBS) return std::cos(ang * n) > 0.8 ? 1.0 : 0.0;
        const double row = std::round(ang * n / TAU), ra = row * TAU / n, dAng = wrapAngle(ang - ra) * rho * R, sp = TAU * R / n * 0.9;
        const double along = rho * R / sp + ((int64_t)row & 1) * 0.5, dAl = (along - std::round(along)) * sp;
        return dAng * dAng + dAl * dAl < sp * sp * 0.09 ? -1.0 : 0.0;
    }
    if (L.layout == 2) {
        const double sp = 2 * Lh / n;
        if (std::fabs(v) < R * 0.08) return 0;
        if (L.mark == MK_RIBS) { const double f = u / sp - std::floor(u / sp); return f < 0.25 ? 1.0 : 0.0; }
        const double row = std::round(u / sp), du = (u / sp - row) * sp, sv = sp * 0.75, al = v / sv + ((int64_t)row & 1) * 0.5, dv = (al - std::round(al)) * sv;
        return du * du + dv * dv < sp * sp * 0.08 ? -1.0 : 0.0;
    }
    if (L.layout == 3) { const double f = v / (2 * R) * n; return std::fabs(f - std::round(f)) < 0.14 ? 1.0 : 0.0; }
    if (L.layout == 4) { const double f = (v + u * 0.7) / (2 * R) * n; return std::fabs(f - std::round(f)) < 0.13 ? 1.0 : 0.0; }
    return 0;
}

// ---- the builder's helpers
P2 Build::at(double u, double v) const {
    if (mirror) v = -v;
    return {x + u * ca - v * sa, y + u * sa + v * ca};
}
double Build::angle(double a) const { return (mirror ? -a : a) + std::atan2(sa, ca); }

void Build::bound(Body& b) {
    if (b.kind == BODY_RADIAL) {
        double mx = 0; for (float r : b.r) mx = std::max(mx, (double)r);
        const double e = b.R * mx * std::max(1.0, b.aspect) + b.look.halo + b.look.wall;
        b.x0 = b.c.x - e; b.x1 = b.c.x + e; b.y0 = b.c.y - e; b.y1 = b.c.y + e;
    } else {
        b.x0 = b.y0 = 1e30; b.x1 = b.y1 = -1e30;
        for (size_t i = 0; i < b.line.size(); i++) {
            const double e = b.rad[i] + b.look.halo + b.look.wall;
            b.x0 = std::min(b.x0, b.line[i].x - e); b.x1 = std::max(b.x1, b.line[i].x + e);
            b.y0 = std::min(b.y0, b.line[i].y - e); b.y1 = std::max(b.y1, b.line[i].y + e);
        }
    }
    out.grow(b.x0, b.y0, b.x1, b.y1);
}

Body& Build::radial(double u, double v, double ang, double R, double aspect, const Look& lk) {
    out.bodies.emplace_back();
    Body& b = out.bodies.back();
    b.kind = BODY_RADIAL; b.look = lk; b.c = at(u, v);
    const double a = angle(ang);
    b.ca = std::cos(a); b.sa = std::sin(a); b.R = std::max(1e-4, R); b.aspect = std::max(0.2, aspect);
    for (float& r : b.r) r = 1;
    b.group = group;
    return b;
}
void Build::finish(Body& b) { bound(b); }

Body& Build::tube(const std::vector<P2>& local, const std::vector<float>& rad, const Look& lk, const std::vector<float>* z) {
    out.bodies.emplace_back();
    Body& b = out.bodies.back();
    b.kind = BODY_TUBE; b.look = lk; b.group = group;
    b.line.reserve(local.size());
    for (const P2& p : local) b.line.push_back(at(p.x, p.y));
    b.rad = rad;
    if (z) { b.z = *z; if (mirror) for (float& q : b.z) q = -q; }
    bound(b);
    return b;
}

void Build::dot(double u, double v, double rx, double ry, double ang, int kind, int bank, float strength) {
    Dot d; d.c = at(u, v); d.rx = std::max(1e-4, rx); d.ry = std::max(1e-4, ry);
    const double a = angle(ang); d.ca = std::cos(a); d.sa = std::sin(a);
    d.kind = kind; d.bank = bank; d.strength = strength; d.group = group;
    const double e = std::max(d.rx, d.ry);
    out.grow(d.c.x - e, d.c.y - e, d.c.x + e, d.c.y + e);
    out.dots.push_back(d);
}

void Build::strand(const std::vector<P2>& local, float w0, float w1, int kind, int bank, float strength) {
    if (local.size() < 2) return;
    Strand s; s.w0 = w0; s.w1 = w1; s.kind = kind; s.bank = bank; s.strength = strength; s.group = group;
    s.pts.reserve(local.size());
    s.x0 = s.y0 = 1e30; s.x1 = s.y1 = -1e30;
    const double e = std::max(w0, w1);
    for (const P2& p : local) {
        const P2 q = at(p.x, p.y);
        s.pts.push_back(q);
        s.x0 = std::min(s.x0, q.x - e); s.x1 = std::max(s.x1, q.x + e); s.y0 = std::min(s.y0, q.y - e); s.y1 = std::max(s.y1, q.y + e);
    }
    out.grow(s.x0, s.y0, s.x1, s.y1);
    out.strands.push_back(std::move(s));
}

// a radial body's outline: the superformula (Gielis 2003) normalised to its largest, the wander's harmonics (moving with the clock at
// `wobble`), the lobes added on
void fillOutline(Body& b, int sym, double n1, double n2, double n3, double wander, uint64_t seed, double t, double wobble, const Lobe* lobes, int nl) {
    double sf[RTAB], mx = 0;
    for (int i = 0; i < RTAB; i++) {
        const double phi = i * TAU / RTAB;
        double r = 1;
        if (sym > 0) {
            const double a = std::fabs(std::cos(sym * phi / 4)), s = std::fabs(std::sin(sym * phi / 4));
            const double x = std::pow(a, n2) + std::pow(s, n3);
            r = x > 1e-12 ? std::pow(x, -1.0 / std::max(0.05, n1)) : 1;
            r = std::min(r, 50.0);
        }
        sf[i] = r; mx = std::max(mx, r);
    }
    double hk[8], ph[8], sp[8];
    for (int k = 2; k < 8; k++) { const uint64_t h = mix64(seed + (uint64_t)k * 0x9E3779B97F4A7C15ULL); hk[k] = (h01(h) * 2 - 1) / k; ph[k] = h01(h >> 9) * TAU; sp[k] = 0.4 + h01(h >> 21); }
    for (int i = 0; i < RTAB; i++) {
        const double phi = i * TAU / RTAB;
        double r = sf[i] / mx;
        if (wander > 0) {
            double w = 0;
            for (int k = 2; k < 8; k++) w += hk[k] * std::cos(k * phi + ph[k] + wobble * t * sp[k]);
            r *= 1 + wander * w * 1.6;
        }
        for (int l = 0; l < nl; l++) {
            double d = std::fabs(phi - lobes[l].phi); d = std::min(d, TAU - d);
            r += lobes[l].amp * std::exp(-(d / lobes[l].w) * (d / lobes[l].w));
        }
        b.r[i] = (float)std::max(0.04, r);
    }
}

// a point on a radial body's outline at an angle (in its own q space) and the outward normal, both on the slide
void edgeOf(const Body& b, double phi, P2& p, P2& n) {
    auto pointAt = [&](double f) {
        const double rr = b.R * outlineAt(b, f);
        const double qx = rr * std::cos(f), qy = rr * std::sin(f);
        const double u = qx * b.aspect;
        const double L = b.R * b.aspect;
        double sc = 1;
        if (b.tail > 0 && u < 0) sc = 1 - b.tail * 0.82 * std::pow(std::min(1.0, -u / L), 1.4);
        else if (b.head > 0 && u > 0) sc = 1 - b.head * 0.82 * std::pow(std::min(1.0, u / L), 1.4);
        const double v = qy * std::max(sc, 0.1) + b.bend * u * u / L;
        return P2{b.c.x + u * b.ca - v * b.sa, b.c.y + u * b.sa + v * b.ca};
    };
    p = pointAt(phi);
    const P2 a = pointAt(phi - 0.01), c = pointAt(phi + 0.01);
    double tx = c.x - a.x, ty = c.y - a.y;
    const double l = std::sqrt(tx * tx + ty * ty);
    if (l > 0) { tx /= l; ty /= l; }
    n = {ty, -tx};
    if ((n.x * (p.x - b.c.x) + n.y * (p.y - b.c.y)) < 0) { n.x = -n.x; n.y = -n.y; }
}

// ---- where a specimen is: closed-form in the clock (its motion's: runs and tumbles, the helical swim's meander, the glide's reversals, the
// twitch's steps, the crawl, the roll, the corkscrew's back and forth; the still ones' Brownian jiggle). Z-04: a specimen held still jiggles
// alone, its beat stopped; its wander's reach scaled; a drift its place is on turns its heading with it (the microscope's crowds and streams)
namespace {
double smoothNoise(uint64_t h, double t, double rate) {   // three sines of hashed rates and phases, -1..1
    double s = 0;
    for (int k = 0; k < 3; k++) { const uint64_t q = mix64(h + (uint64_t)k * 0x51ED27ULL); s += std::sin(t * rate * (0.6 + 0.9 * h01(q)) + h01(q >> 11) * TAU) / 3.0; }
    return s;
}
}   // namespace

double motionRadius(const Morph& m) {
    const double body = std::max(m.lengthUm, m.widthUm);
    switch (m.motion) {
        case MO_RUN_TUMBLE: return std::min(m.speed * m.runS * 1.2, body * 14);
        case MO_HELICAL: return std::min(m.speed * 1.5, body * 6);
        case MO_GLIDING: case MO_CORKSCREW: return std::min(m.speed * 4, body * 3);
        case MO_TWITCHING: return std::min(m.speed * 3, body * 2);
        case MO_CRAWLING: return std::min(m.speed * 8, body * 0.6);
        case MO_ROLLING: return std::min(m.speed * 2, body * 1.5);
        default: return body * 0.15 + 0.15;
    }
}

Pose poseAt(const Morph& m, const Specimen& sp, double tSlide) {
    Pose P; P.x = sp.x; P.y = sp.y; P.angle = sp.angle;
    const double t = sp.clock >= 0 ? sp.clock : tSlide;   // R-438: its moves by its own clock (its kind's pace), its jiggle by the slide's
    const uint64_t h = mix64(sp.seed ^ 0x5EED0F7E57ULL);
    const double R = motionRadius(m) * sp.wander, body = std::max(m.lengthUm, m.widthUm);
    const bool drift = sp.vx != 0 || sp.vy != 0;
    const int motion = sp.still ? (int)MO_NONE : m.motion;
    auto lissajous = [&](double rate) {
        const double w = rate / std::max(1e-6, R);
        const double ax = R * smoothNoise(h, t, w), ay = R * smoothNoise(h ^ 0xABCDEFULL, t, w);
        const double dt = 0.02;
        const double bx = R * smoothNoise(h, t + dt, w), by = R * smoothNoise(h ^ 0xABCDEFULL, t + dt, w);
        P.x += ax; P.y += ay;
        if (drift) P.angle = std::atan2(by - ay + sp.vy * dt, bx - ax + sp.vx * dt);
        else if (std::fabs(bx - ax) + std::fabs(by - ay) > 1e-9) P.angle = std::atan2(by - ay, bx - ax);
    };
    switch (motion) {
        case MO_RUN_TUMBLE: {
            const double T = std::max(0.3, m.runS), f = t / T, k = std::floor(f), q = f - k;
            auto way = [&](double kk) { const uint64_t w = hash2i((int64_t)kk, 7, h); return P2{R * (h01(w) * 2 - 1), R * (h01(w >> 13) * 2 - 1)}; };
            const P2 w0 = way(k), w1 = way(k + 1), wp = way(k - 1);
            const double headIn = std::atan2(w0.y - wp.y, w0.x - wp.x), headOut = std::atan2(w1.y - w0.y, w1.x - w0.x);
            const double tumble = 0.16;
            if (q < tumble) {
                double d = wrapAngle(headOut - headIn);
                const double s = q / tumble;
                P.x += w0.x; P.y += w0.y;
                P.angle = headIn + d * s + 0.6 * std::sin(s * TAU * 2) * (h01(hash2i((int64_t)k, 3, h)) - 0.5);
                P.tumbling = true;
            } else {
                const double s = (q - tumble) / (1 - tumble);
                P.x += w0.x + (w1.x - w0.x) * s; P.y += w0.y + (w1.y - w0.y) * s; P.angle = headOut;
                if (drift) { const double run = T * (1 - tumble); P.angle = std::atan2((w1.y - w0.y) / run + sp.vy, (w1.x - w0.x) / run + sp.vx); }
            }
            break;
        }
        case MO_HELICAL: case MO_ROLLING: case MO_CRAWLING: lissajous(m.speed); break;
        case MO_GLIDING: case MO_CORKSCREW: {
            const double w = m.speed / std::max(1e-6, R);
            const double s = R * std::sin(t * w + h01(h) * TAU);
            const double c = std::cos(t * w + h01(h) * TAU);
            P.x += s * std::cos(sp.angle); P.y += s * std::sin(sp.angle);
            P.backward = c < 0;
            P.angle = sp.angle + 0.06 * smoothNoise(h, t, 0.7);
            break;
        }
        case MO_TWITCHING: {
            const double T = 0.8, f = t / T, k = std::floor(f), q = f - k;
            auto way = [&](double kk) { const uint64_t w = hash2i((int64_t)kk, 11, h); return P2{R * (h01(w) * 2 - 1), R * (h01(w >> 13) * 2 - 1)}; };
            const P2 w0 = way(k), w1 = way(k + 1);
            const double s = q < 0.25 ? q / 0.25 : 1.0;
            const double e = s * s * (3 - 2 * s);
            P.x += w0.x + (w1.x - w0.x) * e; P.y += w0.y + (w1.y - w0.y) * e;
            P.angle = sp.angle + 0.15 * smoothNoise(h, t, 0.5);
            break;
        }
        default: {   // Brownian: a jiggle of its place and its heading, the smaller the more
            const double a = 0.06 + 0.25 / std::sqrt(std::max(0.2, body));
            P.x += a * smoothNoise(h, tSlide, 6.0); P.y += a * smoothNoise(h ^ 0x77ULL, tSlide, 6.0);
            P.angle = sp.angle + (body < 5 ? 0.12 : 0.02) * smoothNoise(h ^ 0x99ULL, tSlide, 2.5);
        }
    }
    P.roll = t * (motion == MO_HELICAL ? 4.0 : (motion == MO_ROLLING ? 0.25 + m.speed / std::max(1.0, body) : 0.0)) + h01(h >> 3) * TAU;
    P.beat = sp.still ? h01(h >> 5) * TAU : t * TAU * std::max(0.5, m.beatHz);
    P.mirror = m.mirrored;
    return P;
}

}   // namespace morph
