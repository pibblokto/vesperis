// T-02: a town's plan (see towns.h). The streets are laid in the town's frame (along its axis and across it), layer by layer from the old
// core outward, each layer in its age's way of the people's pattern, clipped to its ring; the main streets run from the centre's square
// out along the ways that leave the town (the plan's) through every layer; the edge of an older layer is a ring street (the boulevard on
// an old wall's line). Deterministic from the spec: one stream of its seed.
#include "towns.h"
#include "peoples.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>
#include <functional>

const char* const STREET_RANK_NAMES[SR_COUNT] = {"lane", "street", "main street", "avenue", "ring road"};
const char* const DISTRICT_NAMES[DI_COUNT] = {"", "the centre", "the old core", "the houses", "the works", "the harbour", "the towers", "the suburbs"};
const char* const FIELD_KIND_NAMES[FI_COUNT] = {"plots", "strips", "walled fields", "enclosed fields", "irrigation circles", "terraced fields"};

double streetHalf(int rank, int age) {
    static const double H[4][SR_COUNT] = {
        {1.4, 2.2, 3.2, 4.0, 4.5},     // the prehistoric and the medieval ages': lanes and alleys, a market street
        {2.0, 3.5, 5.0, 7.0, 8.0},     // the industrial: streets for carts and trams, its boulevards
        {3.0, 5.0, 7.0, 10.0, 13.0},   // the modern: streets, avenues, the ring road's carriageways
        {3.0, 6.0, 10.0, 14.0, 16.0}}; // the futuristic: wide ways between the great places
    const int a = age <= AGE_MEDIEVAL ? 0 : (age == AGE_INDUSTRIAL ? 1 : (age == AGE_MODERN ? 2 : 3));
    return H[a][clampi(rank, 0, SR_COUNT - 1)];
}
double streetSpacing(int age) {
    switch (age) {
        case AGE_PREHISTORIC: case AGE_MEDIEVAL: return 36;
        case AGE_INDUSTRIAL: return 62;
        case AGE_MODERN: return 105;
        default: return 160;
    }
}

namespace {
struct V2 { double x = 0, z = 0; };

// the parameters t in [0, 1] of a segment A + t (B - A) inside the ring r0 <= |p| <= r1: up to two intervals
int clipRing(double ax, double az, double bx, double bz, double r0, double r1, double out[4]) {
    const double dx = bx - ax, dz = bz - az, a = dx * dx + dz * dz;
    if (a < 1e-12) return 0;
    const double b = 2 * (ax * dx + az * dz), c0 = ax * ax + az * az;
    double lo = 0, hi = 1;
    {   // inside the outer circle
        const double c = c0 - r1 * r1, disc = b * b - 4 * a * c;
        if (disc <= 0) return 0;
        const double q = std::sqrt(disc);
        lo = std::max(lo, (-b - q) / (2 * a)); hi = std::min(hi, (-b + q) / (2 * a));
        if (lo >= hi) return 0;
    }
    if (r0 <= 0) { out[0] = lo; out[1] = hi; return 1; }
    const double c = c0 - r0 * r0, disc = b * b - 4 * a * c;   // outside the inner one
    if (disc <= 0) { out[0] = lo; out[1] = hi; return 1; }
    const double q = std::sqrt(disc), t0 = (-b - q) / (2 * a), t1 = (-b + q) / (2 * a);
    int n = 0;
    if (std::min(hi, t0) > lo) { out[2 * n] = lo; out[2 * n + 1] = std::min(hi, t0); n++; }
    if (hi > std::max(lo, t1)) { out[2 * n] = std::max(lo, t1); out[2 * n + 1] = hi; n++; }
    return n;
}

struct Net {
    const TownSpec& T;
    Rng rng;
    std::vector<Street>& out;
    std::vector<Plaza>& plazas;
    double fx, fz, sx, sz;   // along the axis and across it
    double R;
    Net(const TownSpec& t, std::vector<Street>& o, std::vector<Plaza>& p) : T(t), rng(t.seed ^ 0x7020ULL), out(o), plazas(p), R(t.R) {
        fx = std::sin(t.h0); fz = std::cos(t.h0); sx = std::cos(t.h0); sz = -std::sin(t.h0);
    }
    V2 at(double u, double v) const { return {sx * u + fx * v, sz * u + fz * v}; }   // the axis frame (u across, v along) to the town's metres
    double widthK = 1;   // a town's streets narrower than a city's, a village's narrower still
    void push(double ax, double az, double bx, double bz, int rank, int age) {
        if (std::hypot(bx - ax, bz - az) < 2.0) return;
        Street s; s.ax = ax; s.az = az; s.bx = bx; s.bz = bz; s.rank = rank; s.age = age; s.half = std::max(1.2, streetHalf(rank, age) * widthK);
        s.paved = age >= AGE_MEDIEVAL && (age > AGE_MEDIEVAL || rank >= SR_STREET);
        out.push_back(s);
    }
    // a wedge the streets are kept to (an industrial people's quarters, each laid at its own angle): clockwise from wa0 to wa1, under a half turn
    bool wedgeOn = false; double wa0 = 0, wa1 = 0;
    void seg(V2 a, V2 b, int rank, int age, double r0, double r1) {
        double t[4]; const int n = clipRing(a.x, a.z, b.x, b.z, r0, r1, t);
        for (int k = 0; k < n; k++) {
            double t0 = t[2 * k], t1 = t[2 * k + 1];
            if (wedgeOn) {   // two half planes through the centre: clockwise of wa0, anticlockwise of wa1
                const double hs[2] = {wa0, wa1}, sgn[2] = {-1, 1};
                for (int h = 0; h < 2 && t0 < t1; h++) {
                    const double dx = std::sin(hs[h]), dz = std::cos(hs[h]);
                    const double f0 = sgn[h] * (dx * a.z - dz * a.x), f1 = sgn[h] * (dx * (b.z - a.z) - dz * (b.x - a.x));
                    if (std::fabs(f1) < 1e-12) { if (f0 < 0) t1 = t0; continue; }
                    const double ts = -f0 / f1;
                    if (f1 > 0) t0 = std::max(t0, ts); else t1 = std::min(t1, ts);
                }
                if (t1 <= t0) continue;
            }
            push(a.x + (b.x - a.x) * t0, a.z + (b.z - a.z) * t0, a.x + (b.x - a.x) * t1, a.z + (b.z - a.z) * t1, rank, age);
        }
    }
    // a straight way from a to b in pieces of about 16 m, each moved across the way by a wobble of `amp` metres (a medieval street's
    // crookedness: two waves of the line's own phases, none at the ends), clipped to the ring
    void line(V2 a, V2 b, int rank, int age, double r0, double r1, double amp) {
        const double L = std::hypot(b.x - a.x, b.z - a.z);
        if (L < 1) return;
        if (amp <= 0.05) { seg(a, b, rank, age, r0, r1); return; }
        const int n = std::max(1, (int)std::ceil(L / 16.0));
        const double nx = -(b.z - a.z) / L, nz = (b.x - a.x) / L, p1 = rng.uni() * TAU, p2 = rng.uni() * TAU, w1 = 70 + 60 * rng.uni(), w2 = 25 + 20 * rng.uni();
        V2 prev = a;
        for (int i = 1; i <= n; i++) {
            const double t = (double)i / n, s = t * L, taper = std::min(1.0, std::min(s, L - s) / 12.0);
            const double off = i == n ? 0 : amp * taper * (0.65 * std::sin(TAU * s / w1 + p1) + 0.35 * std::sin(TAU * s / w2 + p2));
            V2 q{a.x + (b.x - a.x) * t + nx * off, a.z + (b.z - a.z) * t + nz * off};
            seg(prev, q, rank, age, r0, r1);
            prev = q;
        }
    }
    // a ring's arc round the centre (radians from north) in pieces of about 18 m, wobbled like a line
    void arc(double r, double a0, double a1, int rank, int age, double r0, double r1, double amp) {
        if (r < 4) return;
        const int n = std::max(3, (int)std::ceil(std::fabs(a1 - a0) * r / 18.0));
        const double p1 = rng.uni() * TAU;
        V2 prev{};
        for (int i = 0; i <= n; i++) {
            const double a = a0 + (a1 - a0) * i / n, rr = r + amp * std::sin(a * 3.0 + p1) * 0.8;
            V2 q{std::sin(a) * rr, std::cos(a) * rr};
            if (i) seg(prev, q, rank, age, r0 - 0.5, r1 + 0.5);
            prev = q;
        }
    }
    double crooked(int age) const { return age <= AGE_MEDIEVAL ? 1.6 + 2.0 * unitFromHash(mix64(T.seed ^ 0x7021ULL)) : 0.0; }

    // ---- the patterns, in an age's way, over the ring [r0, r1] ----
    // a grid along the axis and across it, the lines through the centre the main streets; a medieval grid crooked and its lines' spacing
    // uneven, some of its blocks run together (a line broken off); an industrial grid's blocks with a back lane down their middle
    void grid(int age, double r0, double r1, double S, double angle = 0, double offU = 0, double offV = 0, bool backLanes = false, int mainEvery = 4) {
        const double amp = crooked(age);
        const double ca = std::cos(angle), sa = std::sin(angle);
        auto rot = [&](double u, double v) { return at(u * ca - v * sa, u * sa + v * ca); };
        const int n = (int)std::ceil(r1 / S) + 1;
        for (int dir = 0; dir < 2; dir++)
            for (int k = -n; k <= n; k++) {
                double c = k * S + (dir ? offV : offU);
                if (age <= AGE_MEDIEVAL) c += rng.sym(0.22 * S);
                if (std::fabs(c) > r1) continue;
                const double half = std::sqrt(std::max(0.0, r1 * r1 - c * c)) + 2;
                const int rank = k == 0 ? SR_MAIN : (std::abs(k) % mainEvery == 0 ? SR_MAIN : SR_STREET);
                V2 a = dir ? rot(-half, c) : rot(c, -half), b = dir ? rot(half, c) : rot(c, half);
                if (age <= AGE_MEDIEVAL && rank == SR_STREET && rng.chance(0.18)) {   // a street broken off: two blocks run together
                    const double cut = rng.range(-half * 0.6, half * 0.6), gap = S * (0.8 + 0.6 * rng.uni());
                    V2 m1 = dir ? rot(cut - gap * 0.5, c) : rot(c, cut - gap * 0.5), m2 = dir ? rot(cut + gap * 0.5, c) : rot(c, cut + gap * 0.5);
                    line(a, m1, rank, age, r0, r1, amp); line(m2, b, rank, age, r0, r1, amp);
                } else line(a, b, rank, age, r0, r1, amp);
                if (backLanes && k < n) {   // the back lane between two rows of rows
                    const double cb = c + S * 0.5;
                    if (std::fabs(cb) < r1) { const double hb = std::sqrt(std::max(0.0, r1 * r1 - cb * cb)); seg(dir ? rot(-hb, cb) : rot(cb, -hb), dir ? rot(hb, cb) : rot(cb, hb), SR_LANE, age, r0, r1); }
                }
            }
    }
    // spokes from the centre and rings round it (a people of the radial plan); more spokes outward so no arc between two is longer than
    // about two blocks
    void spokes(int age, double r0, double r1, double S, bool ringsToo) {
        const double amp = crooked(age);
        const int n = clampi(T.number, 3, 9);
        for (int k = 0; k < n; k++) { const double a = T.h0 + (k + 0.5) * TAU / n; line({std::sin(a) * std::max(r0, T.plaza), std::cos(a) * std::max(r0, T.plaza)}, {std::sin(a) * (r1 + 2), std::cos(a) * (r1 + 2)}, SR_MAIN, age, r0, r1, amp); }
        if (!ringsToo) return;
        int m = n;
        for (double r = std::max(r0 + S * 0.5, T.plaza + S * 0.6); r < r1 - S * 0.25; r += S) {
            arc(r, T.h0, T.h0 + TAU, SR_STREET, age, r0, r1, amp);
            while (TAU * r / m > 2.2 * S) m *= 2;   // the lesser spokes between the main ones
            if (m > n) for (int k = 0; k < m; k++) { if (k % (m / n) == 0) continue; const double a = T.h0 + (k + 0.5) * TAU / m; line({std::sin(a) * r, std::cos(a) * r}, {std::sin(a) * std::min(r + S, r1 + 2), std::cos(a) * std::min(r + S, r1 + 2)}, SR_STREET, age, r0, r1, amp); }
        }
    }
    // rings close together, joined by short ways from ring to ring (offset as in a maze), and a few main ways through
    void rings(int age, double r0, double r1, double S) {
        const double amp = crooked(age), step = S * 0.8;
        int ring = 0;
        double prevR = std::max(r0, T.plaza);
        for (double r = std::max(r0 + step * 0.5, T.plaza + step * 0.6); r < r1 - step * 0.2; r += step, ring++) {
            arc(r, T.h0, T.h0 + TAU, ring % 3 == 2 ? SR_MAIN : SR_STREET, age, r0, r1, amp);
            const int links = clampi((int)(TAU * r / (3.2 * S)), 3, 24);
            const double off = rng.uni() * TAU;
            for (int k = 0; k < links; k++) { const double a = off + k * TAU / links; line({std::sin(a) * prevR, std::cos(a) * prevR}, {std::sin(a) * r, std::cos(a) * r}, SR_LANE, age, r0 - 1, r1, 0); }
            prevR = r;
        }
        const int links = clampi((int)(TAU * r1 / (3.2 * S)), 3, 24);
        for (int k = 0; k < links && prevR < r1 - 4; k++) { const double a = rng.uni() * TAU; line({std::sin(a) * prevR, std::cos(a) * prevR}, {std::sin(a) * (r1 + 2), std::cos(a) * (r1 + 2)}, SR_LANE, age, r0 - 1, r1, 0); }
    }
    // an organic web: lanes branching from the streets laid at angles of their own, wandering, stopping short of another street or the edge
    void organic(int age, double r0, double r1, double S) {
        const double amp = std::max(crooked(age), age <= AGE_MEDIEVAL ? 2.0 : 0.0);
        const size_t first = out.size();
        // the main ways from the square out: four to six, wandering
        const int mains = 4 + rng.irange(3);
        const double a0 = rng.uni() * TAU;
        for (int k = 0; k < mains; k++) {
            const double a = a0 + (k + rng.sym(0.25)) * TAU / mains;
            V2 p{std::sin(a) * std::max(r0, T.plaza), std::cos(a) * std::max(r0, T.plaza)};
            double hd = a;
            while (std::hypot(p.x, p.z) < r1) {
                hd += rng.sym(0.35);
                const double toOut = std::atan2(p.x, p.z);
                hd = toOut + clampd(std::remainder(hd - toOut, TAU), -0.6, 0.6);
                const double step = 24 + 12 * rng.uni();
                V2 q{p.x + std::sin(hd) * step, p.z + std::cos(hd) * step};
                seg(p, q, SR_MAIN, age, r0, r1 + 2);
                p = q;
            }
        }
        // lanes off what is laid, a few rounds
        const double want = PI * (r1 * r1 - r0 * r0) * 2.0 / S;   // the street length an area of blocks of S holds
        auto laid = [&]() { double L = 0; for (size_t i = first; i < out.size(); i++) L += std::hypot(out[i].bx - out[i].ax, out[i].bz - out[i].az); return L; };
        for (int round = 0; round < 260 && laid() < want; round++) {
            const size_t from = first + rng.irange((int)(out.size() - first));
            const Street s = out[from];
            const double t = rng.uni(), sxp = s.ax + (s.bx - s.ax) * t, szp = s.az + (s.bz - s.az) * t;
            double hd = std::atan2(s.bx - s.ax, s.bz - s.az) + (rng.chance(0.5) ? 1 : -1) * (PI / 2 + rng.sym(0.5));
            V2 p{sxp, szp};
            const int len = 3 + rng.irange(6);
            const size_t laneFirst = out.size();
            for (int k = 0; k < len; k++) {
                hd += rng.sym(0.3);
                const double step = 14 + 10 * rng.uni();
                V2 q{p.x + std::sin(hd) * step, p.z + std::cos(hd) * step};
                if (std::hypot(q.x, q.z) > r1 || std::hypot(q.x, q.z) < std::max(r0, T.plaza)) break;
                bool close = false, cross = false;   // a lane runs into another way and ends there; it keeps its distance from the ways beside it
                for (size_t i = first; i < laneFirst && !cross; i++) {
                    if (i == from) continue;
                    const double d = streetPointDist(out[i], q.x, q.z);
                    if (d < out[i].half + 1.5) cross = true; else if (d < std::min(S * 0.38, 22.0)) close = true;
                }
                if (close && !cross) break;   // too near a way it does not meet: no lane here
                seg(p, q, k == 0 && rng.chance(0.3) ? SR_STREET : SR_LANE, age, r0, r1);
                if (cross) break;
                p = q;
            }
        }
        (void)amp;
    }
    // one main street along the axis, the plots running back from it to lanes behind (a town along its road); a city of the line lays its
    // streets along the axis side by side, crossed rarely
    void lineTown(int age, double r0, double r1, double S, bool contour) {
        const double amp = crooked(age) * (contour ? 1.4 : 1.0);
        const bool big = r1 > 140;
        const int n = big ? (int)std::ceil(r1 / S) : 1;
        for (int k = -n; k <= n; k++) {
            const double u = k * S * (big ? 1.0 : 1.0);
            if (std::fabs(u) > r1) continue;
            const double half = std::sqrt(std::max(0.0, r1 * r1 - u * u)) + 2;
            const int rank = k == 0 ? SR_MAIN : (big ? SR_STREET : SR_LANE);
            line(at(u, -half), at(u, half), rank, age, r0, r1, amp);
            if (!big) { for (int s = -1; s <= 1; s += 2) { const double ub = s * S * 0.95, hb = std::sqrt(std::max(0.0, r1 * r1 - ub * ub)); line(at(ub, -hb), at(ub, hb), SR_LANE, age, r0, r1, amp); } }
        }
        // the cross ways: rare (up the slope on the contour: stairs), staggered
        const double cs = S * (contour ? 2.6 : 3.2);
        for (int k = -(int)(r1 / cs) - 1; k <= (int)(r1 / cs) + 1; k++) {
            const double v = k * cs + rng.sym(0.2 * cs);
            const double half = std::sqrt(std::max(0.0, r1 * r1 - v * v));
            if (half < 8) continue;
            if (contour) {   // short connectors between neighbouring contour streets, staggered
                for (int j = -n - 1; j <= n; j++) { const double u0 = j * S, vv = v + ((j & 1) ? cs * 0.5 : 0); if (std::fabs(vv) < r1) line(at(u0, vv), at(u0 + S, vv), SR_LANE, age, r0, r1, 0); }
            } else line(at(-half, v), at(half, v), k == 0 ? SR_STREET : SR_LANE, age, r0, r1, amp * 0.5);
        }
    }
    // a main street with its branches, the branches ending short (a cul-de-sac's square at the end), and the branches' own in a city
    void branching(int age, double r0, double r1, double S) {
        const double amp = crooked(age);
        const double half = r1 + 2;
        line(at(0, -half), at(0, half), SR_MAIN, age, r0, r1, amp);
        for (double v = -r1 + S * 0.6; v < r1 - S * 0.4; v += S * (1.0 + 0.3 * rng.uni())) {
            for (int s = -1; s <= 1; s += 2) {
                if (rng.chance(0.15)) continue;
                const double len = std::min(std::sqrt(std::max(0.0, r1 * r1 - v * v)), S * (2.0 + 3.0 * rng.uni()) * (r1 > 200 ? 2.0 : 1.0));
                if (len < 12) continue;
                const double vb = v + rng.sym(0.15 * S);
                V2 a = at(s * 2.0, v), b = at(s * len, vb);
                line(a, b, SR_STREET, age, r0, r1, amp);
                if (std::hypot(b.x, b.z) < r1 && std::hypot(b.x, b.z) > r0) { Plaza p; p.x = b.x; p.z = b.z; p.r = streetHalf(SR_STREET, age) * 2.2; p.age = age; plazas.push_back(p); }
                if (r1 > 200)   // the branches' own branches
                    for (double u = S * 0.9; u < len - S * 0.5; u += S * 1.1)
                        for (int s2 = -1; s2 <= 1; s2 += 2) {
                            if (rng.chance(0.35)) continue;
                            const double l2 = S * (0.6 + 0.7 * rng.uni());
                            V2 c = at(s * u, v + (vb - v) * u / len), d = at(s * u, v + (vb - v) * u / len + s2 * l2);
                            line(c, d, SR_LANE, age, r0, r1, amp);
                        }
            }
        }
    }
    // a modern city's suburbs: collector streets out from the ring road, cul-de-sacs off them, each ending in its turning circle
    void suburbs(int age, double r0, double r1) {
        const double S = 75;
        const int collectors = clampi((int)(TAU * r0 / 210.0), 6, 40);
        const double off = rng.uni() * TAU;
        for (int k = 0; k < collectors; k++) {
            const double a = off + (k + rng.sym(0.2)) * TAU / collectors;
            V2 p{std::sin(a) * r0, std::cos(a) * r0};
            double hd = a;
            std::vector<V2> pts{p};
            while (std::hypot(p.x, p.z) < r1 + 5) {
                hd = a + 0.35 * std::sin(std::hypot(p.x, p.z) / 90.0 + k);   // a collector curves gently
                V2 q{p.x + std::sin(hd) * 30, p.z + std::cos(hd) * 30};
                seg(p, q, SR_STREET, age, r0 - 1, r1 + 4);
                pts.push_back(q);
                p = q;
            }
            for (size_t i = 2; i + 1 < pts.size(); i += 2)   // the cul-de-sacs, alternately each side
                for (int s = -1; s <= 1; s += 2) {
                    if (rng.chance(0.25)) continue;
                    const V2 c = pts[i];
                    const double ad = std::atan2(pts[i + 1].x - pts[i - 1].x, pts[i + 1].z - pts[i - 1].z) + s * (PI / 2 + rng.sym(0.25));
                    const double len = S * (0.55 + 0.4 * rng.uni());
                    V2 e{c.x + std::sin(ad) * len, c.z + std::cos(ad) * len};
                    const double re = std::hypot(e.x, e.z);
                    if (re > r1 || re < r0 + 8) continue;
                    bool close = false;
                    for (const Street& o : out) if (o.rank == SR_LANE && o.age == age && streetPointDist(o, e.x, e.z) < 30) { close = true; break; }
                    if (close) continue;
                    seg(c, e, SR_LANE, age, r0, r1);
                    Plaza pl; pl.x = e.x; pl.z = e.z; pl.r = streetHalf(SR_LANE, age) * 2.4; pl.age = age; plazas.push_back(pl);
                }
        }
    }
    // the people's pattern in an age's way over a ring
    void pattern(int age, double r0, double r1, bool outer) {
        if (r1 - r0 < 6) return;
        const double small = T.sclass == SC_CITY || T.sclass == SC_CAPITAL ? 1.0 : (T.sclass == SC_TOWN ? 0.8 : 0.7);   // a town's and a village's blocks smaller
        const double S = streetSpacing(age) * small * T.spacing * (age <= AGE_MEDIEVAL ? 0.85 + 0.3 * unitFromHash(mix64(T.seed ^ 0x7022ULL)) : 1.0);
        const int pat = T.pattern;
        if (age == AGE_MODERN) {
            // the downtown's blocks inside, the ring road, the suburbs beyond it (a town's ring road its edge)
            const double ringR = r0 > 0 ? r0 + std::max(40.0, 0.3 * (r1 - r0)) : std::max(0.55 * r1, std::min(r1, T.plaza + 140));
            const double inner = std::min(ringR, r1);
            if (pat == ST_RADIAL) spokes(age, r0, inner, S, true);
            else if (pat == ST_CONCENTRIC) rings(age, r0, inner, S);
            else if (pat == ST_LINE || pat == ST_CONTOUR) lineTown(age, r0, inner, S * 0.8, pat == ST_CONTOUR);
            else grid(age, r0, inner, S, pat == ST_ORGANIC ? 0.3 : 0, rng.sym(0.3 * S), rng.sym(0.3 * S), false, 3);
            if (outer && ringR < r1 - 20) {
                arc(ringR, 0, TAU, SR_RING, age, 0, r1 + 40, 0);
                suburbs(age, ringR, r1);
            } else if (ringR < r1) grid(age, ringR, r1, S, 0, 0, 0, false, 3);
            return;
        }
        if (age == AGE_INDUSTRIAL) {
            switch (pat) {
                case ST_RADIAL: spokes(age, r0, r1, S, true); break;
                case ST_CONCENTRIC: rings(age, r0, r1, S); break;
                case ST_LINE: case ST_CONTOUR: lineTown(age, r0, r1, S * 0.75, pat == ST_CONTOUR); break;
                case ST_BRANCHING: branching(age, r0, r1, S); break;
                case ST_ORGANIC: {   // planned quarters, each its grid at its own angle, a main way between two
                    const int q = 3 + rng.irange(3);
                    const double base = T.h0 + rng.uni() * TAU;
                    for (int k = 0; k < q; k++) {
                        wedgeOn = true; wa0 = base + k * TAU / q; wa1 = wa0 + TAU / q;
                        grid(age, r0, r1, S, rng.sym(0.6), rng.sym(0.4 * S), rng.sym(0.4 * S), T.spacing < 1.3, 4);
                        wedgeOn = false;
                        line({std::sin(wa0) * std::max(r0, T.plaza), std::cos(wa0) * std::max(r0, T.plaza)}, {std::sin(wa0) * (r1 + 2), std::cos(wa0) * (r1 + 2)}, SR_MAIN, age, r0, r1, 0);
                    }
                    break;
                }
                default: grid(age, r0, r1, S, 0, rng.sym(0.3 * S), rng.sym(0.3 * S), T.spacing < 1.3, 4); break;
            }
            return;
        }
        // the medieval age (and a camp's paths)
        switch (pat) {
            case ST_RADIAL: spokes(age, r0, r1, S, true); break;
            case ST_CONCENTRIC: rings(age, r0, r1, S); break;
            case ST_ORGANIC: organic(age, r0, r1, S); break;
            case ST_LINE: lineTown(age, r0, r1, S, false); break;
            case ST_CONTOUR: lineTown(age, r0, r1, S, true); break;
            case ST_BRANCHING: branching(age, r0, r1, S); break;
            default: grid(age, r0, r1, S, 0, rng.sym(0.3 * S), rng.sym(0.3 * S), false, 4); break;
        }
    }
};
}

void townStreets(const TownSpec& T, std::vector<Street>& streets, std::vector<Plaza>& plazas) {
    streets.clear(); plazas.clear();
    if (T.layers.empty() || T.R < 20) return;
    Net N(T, streets, plazas);
    const bool city = T.sclass == SC_CITY || T.sclass == SC_CAPITAL;
    N.widthK = city ? 1.0 : (T.sclass == SC_TOWN ? 0.75 : 0.6);
    // the centre's square
    { Plaza p; p.r = T.plaza; p.age = T.layers.front().age; plazas.push_back(p); }
    // each layer in its age's way over its ring; the edge of an older layer a ring street (an old wall's line)
    for (size_t k = 0; k < T.layers.size(); k++) {
        const TownLayer& L = T.layers[k];
        const bool outer = k + 1 == T.layers.size();
        const size_t first = streets.size();
        N.pattern(L.age, std::max(L.r0, k == 0 ? T.plaza : 0.0), outer ? T.R : L.r1, outer);
        if (T.walled && (int)k == T.wallK) {   // inside its wall: the lesser streets end short of it, a street runs round inside it
            const double inner = T.wallR - 3.0;
            std::vector<Street> keep(streets.begin(), streets.begin() + first);
            for (size_t i = first; i < streets.size(); i++) {
                const Street& s = streets[i];
                if (s.rank >= SR_MAIN) { keep.push_back(s); continue; }
                double t[4]; const int ni = clipRing(s.ax, s.az, s.bx, s.bz, 0, inner - s.half, t);
                for (int q = 0; q < ni; q++) { Street c = s; c.ax = s.ax + (s.bx - s.ax) * t[2 * q]; c.az = s.az + (s.bz - s.az) * t[2 * q]; c.bx = s.ax + (s.bx - s.ax) * t[2 * q + 1]; c.bz = s.az + (s.bz - s.az) * t[2 * q + 1]; keep.push_back(c); }
            }
            streets.swap(keep);
            N.arc(inner - streetHalf(SR_STREET, L.age) - 0.5, 0, TAU, SR_STREET, L.age, 0, T.R + 4, 0);
        }
        if (!outer && L.r1 < T.R - 10 && (city || k == 0)) {   // a town's later rings run into each other
            const int nextAge = T.layers[k + 1].age;
            const int br = nextAge >= AGE_INDUSTRIAL && city ? SR_AVENUE : SR_MAIN;
            N.arc(L.r1 + streetHalf(br, nextAge) * N.widthK + 1.5, 0, TAU, br, nextAge, 0, T.R + 4, 0);
        }
    }
    // the main ways out: along the axis both ways and along the plan's ways, through every layer in each layer's way; the centre's square's
    // ring
    std::vector<double> ex = T.exits;
    ex.push_back(T.h0); ex.push_back(T.h0 + PI);
    std::vector<double> mains;
    for (double a : ex) {
        bool close = false;
        for (double m : mains) if (std::fabs(std::remainder(a - m, TAU)) < 25 * DEG) close = true;
        if (!close && mains.size() < 8) mains.push_back(a);
    }
    if (T.pattern != ST_RADIAL)   // a radial people's spokes are its ways out already
        for (double a : mains)
            for (size_t k = 0; k < T.layers.size(); k++) {
                const TownLayer& L = T.layers[k];
                const bool outer = k + 1 == T.layers.size();
                const double r0 = std::max(L.r0, T.plaza), r1 = outer ? T.R + 30 : L.r1 + 2;
                if (r1 - r0 < 4) continue;
                const int rank = L.age >= AGE_INDUSTRIAL && city ? SR_AVENUE : SR_MAIN;
                N.line({std::sin(a) * r0, std::cos(a) * r0}, {std::sin(a) * r1, std::cos(a) * r1}, rank, L.age, r0 - 0.5, r1 + 0.5, N.crooked(L.age) * 0.6);
            }
    N.arc(T.plaza + streetHalf(SR_STREET, T.layers.front().age) * 0.6, 0, TAU, SR_STREET, T.layers.front().age, 0, T.R + 10, 0);
    // the leftovers: nothing under eight metres; nothing in the centre's square but its ring
    std::vector<Street> kept;
    kept.reserve(streets.size());
    for (const Street& s : streets) {
        const double L = std::hypot(s.bx - s.ax, s.bz - s.az);
        if (L < 8.0 && s.rank < SR_RING) continue;
        const double mx = (s.ax + s.bx) * 0.5, mz = (s.az + s.bz) * 0.5;
        if (std::hypot(mx, mz) < T.plaza - 2 && std::hypot(s.ax, s.az) < T.plaza - 2 && std::hypot(s.bx, s.bz) < T.plaza - 2) continue;
        kept.push_back(s);
    }
    streets.swap(kept);
    if (T.shore) {   // the water's edge: what of a street reaches past it is cut off
        std::vector<Street> land;
        for (Street s : streets) {
            const double pa = s.ax * T.shoreNx + s.az * T.shoreNz, pb = s.bx * T.shoreNx + s.bz * T.shoreNz, lim = T.shoreLimit;
            if (pa > lim && pb > lim) continue;
            if (pa > lim || pb > lim) {
                const double t = (lim - pa) / (pb - pa), mx = s.ax + (s.bx - s.ax) * t, mz = s.az + (s.bz - s.az) * t;
                if (pa > lim) { s.ax = mx; s.az = mz; } else { s.bx = mx; s.bz = mz; }
                if (std::hypot(s.bx - s.ax, s.bz - s.az) < 6) continue;
            }
            land.push_back(s);
        }
        streets.swap(land);
    }
    // the web: what a clip cut off from the rest (a lane past a wall's line, a piece of a ring beyond the shore) is dropped
    {
        const int n = (int)streets.size();
        std::vector<int> parent(n); for (int i = 0; i < n; i++) parent[i] = i;
        auto find = [&](int i) { while (parent[i] != i) { parent[i] = parent[parent[i]]; i = parent[i]; } return i; };
        StreetGrid G; double reach = 10;
        for (const Street& s : streets) reach = std::max(reach, std::max(std::max(std::fabs(s.ax), std::fabs(s.bx)), std::max(std::fabs(s.az), std::fabs(s.bz))) + 10);
        G.build(streets, reach);
        for (int i = 0; i < n; i++) {
            const Street& a = streets[i];
            G.near(std::min(a.ax, a.bx) - a.half - 2, std::min(a.az, a.bz) - a.half - 2, std::max(a.ax, a.bx) + a.half + 2, std::max(a.az, a.bz) + a.half + 2, [&](int j) {
                if (j <= i) return;
                const Street& b = streets[j];
                const double d = std::min(std::min(streetPointDist(b, a.ax, a.az), streetPointDist(b, a.bx, a.bz)), std::min(streetPointDist(a, b.ax, b.az), streetPointDist(a, b.bx, b.bz)));
                const bool cross = [&] { auto cr = [](double ux, double uz, double vx, double vz) { return ux * vz - uz * vx; };
                    const double d1 = cr(a.bx - a.ax, a.bz - a.az, b.ax - a.ax, b.az - a.az), d2 = cr(a.bx - a.ax, a.bz - a.az, b.bx - a.ax, b.bz - a.az);
                    const double d3 = cr(b.bx - b.ax, b.bz - b.az, a.ax - b.ax, a.az - b.az), d4 = cr(b.bx - b.ax, b.bz - b.az, a.bx - b.ax, a.bz - b.az);
                    return ((d1 > 0) != (d2 > 0)) && ((d3 > 0) != (d4 > 0)); }();
                if (cross || d <= std::max(a.half, b.half) + 0.5) { const int ri = find(i), rj = find(j); if (ri != rj) parent[ri] = rj; }
            });
        }
        std::vector<double> len(n, 0);
        int bestRoot = -1; double best = -1;
        for (int i = 0; i < n; i++) { const int r = find(i); len[r] += std::hypot(streets[i].bx - streets[i].ax, streets[i].bz - streets[i].az); }
        for (int i = 0; i < n; i++) if (len[i] > best) { best = len[i]; bestRoot = i; }
        std::vector<Street> web;
        for (int i = 0; i < n; i++) if (find(i) == bestRoot) web.push_back(streets[i]);
        streets.swap(web);
    }
}

// ---- the fields ----------------------------------------------------------------------------------------------------------------
void townFields(const TownSpec& T, int age, const SiteRead& site, bool desert, std::vector<FieldPatch>& out) {
    out.clear();
    if (age >= AGE_FUTURISTIC) return;
    Rng rng(T.seed ^ 0x7030ULL);
    const bool big = T.sclass == SC_CITY || T.sclass == SC_CAPITAL;
    const double R = T.R;
    // how far the fields reach: a camp's plots close, a town's strips a walk out, a modern city's farmland kilometres
    double reach = 0;
    switch (age) {
        case AGE_PREHISTORIC: reach = 60 + 0.5 * R; break;
        case AGE_MEDIEVAL: reach = big ? 600 : (T.sclass == SC_TOWN ? 360 : 220); break;
        case AGE_INDUSTRIAL: reach = big ? 800 : 400; break;
        default: reach = big ? 1600 : 700; break;
    }
    const double shoreR = site.shoreDist;
    auto wetSide = [&](double x, double z) {   // toward the water within reach: not a field
        if (shoreR > 1e8) return false;
        const double d = std::hypot(x, z), a = std::atan2(x, z);
        return std::fabs(std::remainder(a - site.shoreDir, TAU)) < 0.9 && d > shoreR - 20;
    };
    if (site.slope > 0.12 && age <= AGE_INDUSTRIAL) {   // a slope: terraced fields along the contour, below the town
        const double dx = std::sin(site.downhill), dz = std::cos(site.downhill);
        const double step = clampd(1.4 / site.slope, 8, 30);
        for (double d = R + 10; d < R + reach * 0.6; d += step)
            for (int s = -3; s <= 3; s++) {
                const double a = site.downhill + s * 0.22, x = std::sin(a) * d, z = std::cos(a) * d;
                if (wetSide(x, z)) continue;
                FieldPatch f; f.x = x; f.z = z; f.hw = d * 0.11; f.hd = step * 0.45; f.heading = site.downhill; f.kind = FI_TERRACE; f.age = age;
                out.push_back(f);
            }
        (void)dx; (void)dz;
        return;
    }
    if (age == AGE_PREHISTORIC) {   // small plots round the camp, their banks
        const int n = 6 + rng.irange(10);
        for (int k = 0; k < n; k++) {
            const double a = rng.uni() * TAU, d = R + 12 + reach * std::sqrt(rng.uni());
            FieldPatch f; f.x = std::sin(a) * d; f.z = std::cos(a) * d; f.hw = 10 + 8 * rng.uni(); f.hd = 10 + 8 * rng.uni(); f.heading = T.h0 + rng.sym(0.3); f.kind = FI_PLOT; f.age = age;
            if (!wetSide(f.x, f.z)) out.push_back(f);
        }
        return;
    }
    if (age == AGE_MEDIEVAL) {   // the open fields: furlongs of strips, each furlong its own way
        const int n = big ? 60 : (T.sclass == SC_TOWN ? 26 : 14);
        for (int k = 0, tries = 0; k < n && tries < n * 4; tries++) {
            const double a = rng.uni() * TAU, d = R + 25 + reach * std::sqrt(rng.uni());
            FieldPatch f; f.x = std::sin(a) * d; f.z = std::cos(a) * d; f.hw = 45 + 40 * rng.uni(); f.hd = 50 + 50 * rng.uni();
            f.heading = rng.chance(0.5) ? a : a + PI / 2 + rng.sym(0.2); f.kind = desert ? FI_WALLED : FI_STRIP; f.age = age;
            if (wetSide(f.x, f.z)) continue;
            bool overlap = false;
            for (const FieldPatch& o : out) if (std::hypot(o.x - f.x, o.z - f.z) < 0.8 * (std::max(o.hw, o.hd) + std::max(f.hw, f.hd))) { overlap = true; break; }
            if (overlap) continue;
            out.push_back(f); k++;
        }
        return;
    }
    // the industrial age's enclosures on a grid of the axis; the modern's great fields and, on flat dry ground, its irrigation circles
    const double cell = age == AGE_INDUSTRIAL ? 160 : 420;
    const double ca = std::cos(T.h0), sa = std::sin(T.h0);
    const int n = (int)std::ceil((R + reach) / cell);
    const bool circles = age == AGE_MODERN && site.slope < 0.05 && (desert || unitFromHash(mix64(T.seed ^ 0x7031ULL)) < 0.35);
    for (int i = -n; i <= n; i++)
        for (int j = -n; j <= n; j++) {
            const double u = (i + 0.5) * cell, v = (j + 0.5) * cell;
            const double x = ca * u + sa * v, z = -sa * u + ca * v, d = std::hypot(x, z);
            if (d < R + cell * 0.55 || d > R + reach) continue;
            if (wetSide(x, z) || rng.chance(0.18)) continue;
            FieldPatch f; f.x = x; f.z = z; f.heading = T.h0; f.age = age;
            if (circles && rng.chance(0.75)) { f.kind = FI_CIRCLE; f.hw = f.hd = cell * (0.42 + 0.06 * rng.uni()); }
            else { f.kind = FI_ENCLOSED; f.hw = cell * (0.44 + 0.04 * rng.uni()); f.hd = cell * (0.44 + 0.04 * rng.uni()); }
            out.push_back(f);
        }
}

// ---- the geometry ----------------------------------------------------------------------------------------------------------------
double streetPointDist(const Street& s, double x, double z) {
    const double dx = s.bx - s.ax, dz = s.bz - s.az, L2 = dx * dx + dz * dz;
    const double t = L2 > 1e-12 ? clampd(((x - s.ax) * dx + (z - s.az) * dz) / L2, 0, 1) : 0;
    return std::hypot(x - (s.ax + dx * t), z - (s.az + dz * t));
}
namespace {
// the distance from a point to an axis-aligned box [-hw, hw] x [-hd, hd]
double pointBox(double x, double z, double hw, double hd) {
    const double qx = std::max(0.0, std::fabs(x) - hw), qz = std::max(0.0, std::fabs(z) - hd);
    return std::hypot(qx, qz);
}
double segSeg(double ax, double az, double bx, double bz, double cx, double cz, double dx, double dz) {   // the distance between two segments
    auto cross2 = [](double ux, double uz, double vx, double vz) { return ux * vz - uz * vx; };
    const double d1 = cross2(bx - ax, bz - az, cx - ax, cz - az), d2 = cross2(bx - ax, bz - az, dx - ax, dz - az);
    const double d3 = cross2(dx - cx, dz - cz, ax - cx, az - cz), d4 = cross2(dx - cx, dz - cz, bx - cx, bz - cz);
    if (((d1 > 0) != (d2 > 0)) && ((d3 > 0) != (d4 > 0))) return 0;
    Street s1; s1.ax = ax; s1.az = az; s1.bx = bx; s1.bz = bz;
    Street s2; s2.ax = cx; s2.az = cz; s2.bx = dx; s2.bz = dz;
    return std::min(std::min(streetPointDist(s1, cx, cz), streetPointDist(s1, dx, dz)), std::min(streetPointDist(s2, ax, az), streetPointDist(s2, bx, bz)));
}
}
double streetRectGap(const Street& s, double x, double z, double heading, double hw, double hd) {
    // into the rectangle's frame: x' across the heading (cos h, -sin h), z' along it (sin h, cos h)
    const double ch = std::cos(heading), sh = std::sin(heading);
    auto loc = [&](double px, double pz, double& lx, double& lz) { const double ex = px - x, ez = pz - z; lx = ex * ch - ez * sh; lz = ex * sh + ez * ch; };
    double ax, az, bx, bz; loc(s.ax, s.az, ax, az); loc(s.bx, s.bz, bx, bz);
    // inside the box, or crossing an edge: 0
    double d = std::min(pointBox(ax, az, hw, hd), pointBox(bx, bz, hw, hd));
    if (d > 0) {
        const double cx[4] = {-hw, hw, hw, -hw}, cz[4] = {-hd, -hd, hd, hd};
        for (int k = 0; k < 4 && d > 0; k++) d = std::min(d, segSeg(ax, az, bx, bz, cx[k], cz[k], cx[(k + 1) % 4], cz[(k + 1) % 4]));
    }
    return d - s.half;
}

void StreetGrid::build(const std::vector<Street>& st, double reach) {
    cell = 24; x0 = -reach; z0 = -reach;
    nx = nz = std::max(1, (int)std::ceil(2 * reach / cell));
    cells.assign((size_t)nx * nz, {});
    for (int k = 0; k < (int)st.size(); k++) {
        const Street& s = st[k];
        const double m = s.half + 0.5;
        const int i0 = std::max(0, (int)std::floor((std::min(s.ax, s.bx) - m - x0) / cell)), i1 = std::min(nx - 1, (int)std::floor((std::max(s.ax, s.bx) + m - x0) / cell));
        const int j0 = std::max(0, (int)std::floor((std::min(s.az, s.bz) - m - z0) / cell)), j1 = std::min(nz - 1, (int)std::floor((std::max(s.az, s.bz) + m - z0) / cell));
        for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++) {
            // only the cells the corridor touches
            const double cxm = x0 + (i + 0.5) * cell, czm = z0 + (j + 0.5) * cell;
            if (streetPointDist(s, cxm, czm) > m + cell * 0.7072) continue;
            cells[(size_t)j * nx + i].push_back(k);
        }
    }
}

int buildingStreet(const RuinSpec& r, const Building& b, double margin) {
    for (int k = 0; k < (int)r.streets.size(); k++) {
        const Street& s = r.streets[k];
        const double rb = std::hypot(b.hw, b.hd) + s.half + margin;
        if (streetPointDist(s, b.x, b.z) > rb) continue;
        if (streetRectGap(s, b.x, b.z, b.heading, b.hw + margin, b.hd + margin) < 0) return k;
    }
    return -1;
}

double streetsJoined(const std::vector<Street>& st) {
    const int n = (int)st.size();
    if (n == 0) return 1;
    std::vector<int> parent(n);
    for (int i = 0; i < n; i++) parent[i] = i;
    std::function<int(int)> find = [&](int i) { while (parent[i] != i) { parent[i] = parent[parent[i]]; i = parent[i]; } return i; };
    StreetGrid G;
    double reach = 10;
    for (const Street& s : st) reach = std::max(reach, std::max(std::max(std::fabs(s.ax), std::fabs(s.bx)), std::max(std::fabs(s.az), std::fabs(s.bz))) + 10);
    G.build(st, reach);
    for (int i = 0; i < n; i++) {
        const Street& a = st[i];
        G.near(std::min(a.ax, a.bx) - a.half - 2, std::min(a.az, a.bz) - a.half - 2, std::max(a.ax, a.bx) + a.half + 2, std::max(a.az, a.bz) + a.half + 2, [&](int j) {
            if (j <= i) return;
            const Street& b = st[j];
            if (segSeg(a.ax, a.az, a.bx, a.bz, b.ax, b.az, b.bx, b.bz) <= std::max(a.half, b.half) + 0.5) { const int ri = find(i), rj = find(j); if (ri != rj) parent[ri] = rj; }
        });
    }
    std::vector<double> len(n, 0);
    double total = 0, best = 0;
    for (int i = 0; i < n; i++) { const double L = std::hypot(st[i].bx - st[i].ax, st[i].bz - st[i].az); len[find(i)] += L; total += L; }
    for (int i = 0; i < n; i++) best = std::max(best, len[i]);
    return total > 0 ? best / total : 1;
}
