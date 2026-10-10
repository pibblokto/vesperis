// C-17 (2026-10-10): the instruments' constructor (galaxy/instruments.h). A make (its plan, its materials, its people's style, its size)
// becomes parts in its own metres as it lies: u along it, v across it, w up from the floor. Each plan is a few lines over the shared
// pieces: tubes and bells, outlined bodies in the people's outline, holes in the people's shape and rhythm, heads in the people's tops,
// strings, frets and keys by the scale, the ornament of the people's genome. Every part that sounds carries its course (the key that lights it).
#include "instruments.h"
#include "genome.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

namespace {
struct Mk {
    const InstrumentMake& M;
    const Tradition& T;
    std::vector<InstrumentPart>& out;
    Rng rng;
    int n;       // the scale's notes
    double S;    // its size

    Mk(const InstrumentMake& m, const Tradition& t, std::vector<InstrumentPart>& o) : M(m), T(t), out(o), rng(m.seed ^ 0xC17B1DULL), n(std::max(1, (int)t.scale.size())), S(std::max(0.4, m.size)) {}

    // the degree's ratio over the tonic (past the scale, the next period's)
    double ratio(int i) const { const int sc = std::max(1, (int)T.scale.size()); if (T.scale.empty()) return std::pow(2.0, i / 7.0); return std::pow(T.period, i / sc) * T.scale[i % sc]; }

    InstrumentPart& add(int kind, int mat, int course = -1) { out.emplace_back(); InstrumentPart& p = out.back(); p.kind = kind; p.mat = mat; p.course = course; return p; }
    InstrumentPart& tube(double u, double v, double ang, double len, double r0, double r1, double wc, int mat, int course = -1) {
        InstrumentPart& p = add(IPK_TUBE, mat, course); p.u = u; p.v = v; p.ang = ang; p.len = std::max(1e-3, len); p.r0 = r0; p.r1 = r1; p.w0 = wc; p.w1 = wc + std::max(r0, r1); return p;
    }
    InstrumentPart& tubeTo(double u0, double v0, double u1, double v1, double r0, double r1, double wc, int mat, int course = -1) {
        return tube(u0, v0, std::atan2(v1 - v0, u1 - u0), std::hypot(u1 - u0, v1 - v0), r0, r1, wc, mat, course);
    }
    InstrumentPart& plate(double u, double v, double ang, double len, double hw, double w0, double w1, int mat, double taper, double waist, double square, double dome, int sides = 0, int skew = 0) {
        InstrumentPart& p = add(IPK_PLATE, mat); p.u = u; p.v = v; p.ang = ang; p.len = len; p.r0 = hw; p.w0 = w0; p.w1 = w1;
        p.taper = taper; p.waist = waist; p.square = square; p.dome = dome; p.sides = sides; p.skew = skew; return p;
    }
    // a body in the people's outline (its dome a share of its thickness)
    InstrumentPart& body(double u, double v, double ang, double len, double hw, double w0, double w1, int mat, double domeShare = 1.0) {
        return plate(u, v, ang, len, hw, w0, w1, mat, clampd(M.taper, 0.35, 1.6), clampd(M.waist, -0.3, 0.45), clampd(M.square, 1.2, 10), M.dome * (w1 - w0) * domeShare, M.sides, M.skew);
    }
    InstrumentPart& disc(double u, double v, double r0, double r1, double w0, double w1, int mat, int course = -1, int sides = 0) {
        InstrumentPart& p = add(IPK_DISC, mat, course); p.u = u; p.v = v; p.r0 = r0; p.r1 = r1; p.w0 = w0; p.w1 = w1; p.sides = sides; return p;
    }
    InstrumentPart& bowl(double u, double v, double r, double w0, double w1, int mat, double flare, int course = -1) {
        InstrumentPart& p = disc(u, v, r / flare, r / flare, w0, w1, mat, course); p.open = true; p.taper = flare; return p;
    }
    InstrumentPart& ball(double u, double v, double r0, double r1, double w0, double h, int mat, double ang = 0, int course = -1) {
        InstrumentPart& p = add(IPK_BALL, mat, course); p.u = u; p.v = v; p.r0 = r0; p.r1 = r1; p.w0 = w0; p.w1 = w0 + h; p.ang = ang; return p;
    }
    InstrumentPart& box(double u, double v, double ang, double len, double hw, double w0, double w1, int mat, int course = -1) {
        InstrumentPart& p = add(IPK_BOX, mat, course); p.u = u; p.v = v; p.ang = ang; p.len = len; p.r0 = hw; p.w0 = w0; p.w1 = w1; return p;
    }
    InstrumentPart& boxTo(double u0, double v0, double u1, double v1, double hw, double w0, double w1, int mat, int course = -1) {
        return box((u0 + u1) * 0.5, (v0 + v1) * 0.5, std::atan2(v1 - v0, u1 - u0), std::hypot(u1 - u0, v1 - v0), hw, w0, w1, mat, course);
    }
    InstrumentPart& ring(double u, double v, double R, double r, double w0, int mat, int course = -1) {
        InstrumentPart& p = add(IPK_RING, mat, course); p.u = u; p.v = v; p.r0 = R; p.r1 = r; p.w0 = w0; p.w1 = w0 + 2 * r; return p;
    }
    InstrumentPart& band(double u, double v, double ang, double R, double r, double wc, int mat) {
        InstrumentPart& p = add(IPK_RING, mat); p.u = u; p.v = v; p.ang = ang; p.r0 = R; p.r1 = r; p.w0 = wc; p.w1 = wc + R + r; p.standing = true; return p;
    }
    InstrumentPart& str(double u0, double v0, double w0, double u1, double v1, double w1, int mat, int course = -1, double thick = 0.0006) {
        InstrumentPart& p = add(IPK_STRING, mat, course); p.u = u0; p.v = v0; p.ang = std::atan2(v1 - v0, u1 - u0); p.len = std::hypot(u1 - u0, v1 - v0); p.w0 = w0; p.w1 = w1; p.r0 = thick; return p;
    }
    InstrumentPart& mark(double u, double v, double r, double w, int shape, int mat = IM_HOLE, int course = -1, double ang = 0, double len = 0) {
        InstrumentPart& p = add(IPK_MARK, mat, course); p.u = u; p.v = v; p.r0 = r; p.w0 = w; p.w1 = w + 0.0015; p.shape = shape; p.ang = ang; p.len = len > 0 ? len : 2 * r; return p;
    }

    // the height of a body's top at (u, v) (a plate's dome, a ball's cap), and a mark's height on it
    double surfaceAt(const InstrumentPart& b, double u, double v) const {
        const double c = std::cos(b.ang), s = std::sin(b.ang), du = u - b.u, dv = v - b.v;
        const double hl = b.kind == IPK_PLATE ? b.len * 0.5 : b.r1, hw = b.r0;
        const double a = (du * c + dv * s) / std::max(1e-6, hl), q = (-du * s + dv * c) / std::max(1e-6, hw), rho2 = std::min(1.0, a * a + q * q);
        if (b.kind == IPK_PLATE) return b.w1 + std::max(0.0, b.dome) * (1 - rho2);
        if (b.kind == IPK_BALL) { const double cap = std::min(0.45 * (b.w1 - b.w0), std::max(b.r0, b.r1) * 2); return b.w1 - cap + cap * std::sqrt(1 - rho2); }
        return b.w1;
    }
    double onTop(const InstrumentPart& b, double u, double v) const { return surfaceAt(b, u, v) + 0.0008; }

    // ---- the shared pieces ----
    int trimMat() const { return M.trim == IM_LIGHT && M.age < AGE_FUTURISTIC ? M.fit : M.trim; }
    int accentMat(int place) const { return M.accent == place ? (M.age >= AGE_MEDIEVAL || M.painted > 0.3 ? IM_PAINT : M.trim) : trimMat(); }
    // where the holes of a run lie: the people's rhythm (even, in pairs, in threes, by the scale's own steps), from 0 to 1
    std::vector<double> rhythm(int count) {
        std::vector<double> gaps;
        for (int i = 0; i + 1 < count; i++) {
            double g = 1;
            switch (M.rhythm) {
                case RH_PAIRED: g = i % 2 == 0 ? 0.55 : 1.45; break;
                case RH_GROUPED: g = (i % 3 == 2) ? 1.7 : 0.7; break;
                case RH_SCATTERED: g = 0.4 + std::fabs(std::log(ratio(i + 1) / ratio(i))) * 6; break;
                default: break;
            }
            gaps.push_back(g);
        }
        std::vector<double> at(count, 0.5);
        double tot = 0; for (double g : gaps) tot += g;
        double x = 0; if (count > 1) for (int i = 0; i < count; i++) { at[i] = tot > 0 ? x / tot : 0.5; if (i < (int)gaps.size()) x += gaps[i]; }
        return at;
    }
    // holes along a run (from (u, v) along `ang` for `len`) on a surface at w: the far one the first course
    void holesAlong(double u, double v, double ang, double len, double w, double r, int count, int course0 = 0, double side = 0) {
        const std::vector<double> at = rhythm(count);
        const double c = std::cos(ang), s = std::sin(ang);
        for (int i = 0; i < count; i++) {
            const double t = at[count - 1 - i], du = u + c * len * t - s * side * (i % 2 ? 1 : -1), dv = v + s * len * t + c * side * (i % 2 ? 1 : -1);
            mark(du, dv, r, w, M.hole, IM_HOLE, course0 + i, ang + PI / 2, r * 3);
            if (M.accent == AP_DOORS && M.age >= AGE_MEDIEVAL) ring(du, dv, r * 1.5, r * 0.25, w - r * 0.2, IM_PAINT);
        }
    }
    // a sound hole and its rosette by the motif
    void soundHole(double u, double v, double r, double w) {
        mark(u, v, r, w, M.hole == 1 ? 0 : M.hole, IM_HOLE, -1, 0, r * 2.4);
        const int tm = trimMat();
        switch (M.motif) {
            case MO_CIRCLES: ring(u, v, r * 1.35, r * 0.12, w, tm); ring(u, v, r * 1.6, r * 0.08, w, tm); break;
            case MO_STARS: for (int k = 0; k < M.number; k++) { const double a = TAU * k / M.number; mark(u + std::cos(a) * r * 1.5, v + std::sin(a) * r * 1.5, r * 0.22, w, 3, tm); } break;
            case MO_SPIRALS: for (int k = 0; k < 10; k++) { const double a = k * 0.9, rr = r * (1.2 + 0.07 * k); mark(u + std::cos(a) * rr, v + std::sin(a) * rr, r * 0.1, w, 0, tm); } break;
            case MO_STEPS: for (int k = 0; k < 4; k++) { const double a = PI / 4 + k * PI / 2; mark(u + std::cos(a) * r * 1.4, v + std::sin(a) * r * 1.4, r * 0.18, w, 2, tm); } break;
            case MO_GLYPHS: case MO_FIGURES: for (int k = 0; k < 6; k++) { const double a = TAU * k / 6; mark(u + std::cos(a) * r * 1.45, v + std::sin(a) * r * 1.45, r * 0.13, w, k % 2 ? 1 : 2, tm, -1, a, r * 0.4); } break;
            default: ring(u, v, r * 1.25, r * 0.1, w, tm); break;
        }
    }
    // the ornament along a tube: bands, carved rings, studs
    void tubeOrnament(double u, double v, double ang, double len, double r, double wc) {
        const double c = std::cos(ang), s = std::sin(ang);
        const int tm = accentMat(AP_BANDS);
        switch (M.deco) {
            case ON_BANDED: { const int nb = clampi(M.number / 2, 2, 4); for (int i = 0; i < nb; i++) { const double t = nb == 1 ? 0.5 : (i == 0 ? 0.04 : (i == nb - 1 ? 0.96 : (double)i / (nb - 1) * 0.3)); band(u + c * len * t, v + s * len * t, ang, r * 1.04, r * 0.18, wc, tm); } break; }
            case ON_CARVED: for (int i = 0; i < 3; i++) { const double t = 0.06 + 0.04 * i; band(u + c * len * t, v + s * len * t, ang, r * 1.0, r * 0.08, wc, IM_HOLE); band(u + c * len * (1 - t), v + s * len * (1 - t), ang, r * 1.0, r * 0.08, wc, IM_HOLE); } break;
            case ON_ENCRUSTED: for (int i = 0; i < 2 * M.number; i++) { const double t = 0.05 + 0.9 * i / std::max(1, 2 * M.number - 1); if (t > 0.3 && t < 0.88) continue; ball(u + c * len * t, v + s * len * t, r * 0.32, r * 0.32, wc + r * 0.7, r * 0.5, trimMat()); } break;
            default: break;
        }
    }
    // the ornament on a body (an outlined plate): paint, bands across, carved motif, studs round its rim
    void bodyOrnament(const InstrumentPart& b, double avoidR = 0) {
        const double c = std::cos(b.ang), s = std::sin(b.ang), L = b.len * 0.5, H = b.r0;
        auto at = [&](double a, double q, double& u, double& v) { u = b.u + c * a - s * q; v = b.v + s * a + c * q; };
        if (M.painted > 0.55 || M.accent == AP_ROOFS) {   // a painted face in the people's accent
            InstrumentPart p = b; p.r0 *= 0.82; p.len *= 0.86; p.w0 = b.w1 - 0.0005; p.w1 = b.w1 + 0.0008; p.dome *= 0.9; p.mat = IM_PAINT; p.course = -1; out.push_back(p);
        }
        double u, v;
        switch (M.deco) {
            case ON_BANDED: for (int i = 0; i < 2; i++) { at((i ? 1 : -1) * L * 0.7, 0, u, v); box(u, v, b.ang + PI / 2, H * 1.7, L * 0.035, b.w1, b.w1 + 0.002, accentMat(AP_BANDS)); } break;
            case ON_CARVED: {
                const int k = M.number;
                for (int i = 0; i < k; i++) {
                    const double a = -L * 0.75 + 1.5 * L * i / std::max(1, k - 1);
                    if (std::fabs(a) < avoidR) continue;
                    const int shape = M.motif == MO_CIRCLES ? 0 : (M.motif == MO_STEPS ? 2 : (M.motif == MO_STARS ? 6 : (M.motif == MO_LINES ? 1 : 3)));
                    at(a, H * 0.62, u, v); mark(u, v, H * 0.05, onTop(b, u, v), shape, IM_HOLE, -1, b.ang, H * 0.16);
                    at(a, -H * 0.62, u, v); mark(u, v, H * 0.05, onTop(b, u, v), shape, IM_HOLE, -1, b.ang, H * 0.16);
                }
                break;
            }
            case ON_ENCRUSTED: {
                const int k = 3 * M.number;
                for (int i = 0; i < k; i++) { const double t = TAU * i / k; at(std::cos(t) * L * 0.86, std::sin(t) * H * 0.84, u, v); ball(u, v, H * 0.045, H * 0.045, surfaceAt(b, u, v) - 0.002, H * 0.05, trimMat()); }
                break;
            }
            default: break;
        }
    }
    // a head: the people's tops (a knob, a notched head, a spike, a carved figure, a plain end) at (u, v) facing `ang`
    void head(double u, double v, double ang, double w, double sz, int mat) {
        const double c = std::cos(ang), s = std::sin(ang);
        const int m = M.accent == AP_TOPS ? accentMat(AP_TOPS) : mat;
        switch (M.top) {
            case TP_FINIALS: ball(u + c * sz * 0.7, v + s * sz * 0.7, sz * 0.75, sz * 0.75, w, sz * 1.2, m); break;
            case TP_CRENELS: for (int k = -1; k <= 1; k += 2) box(u + c * sz * 0.6 - s * k * sz * 0.45, v + s * sz * 0.6 + c * k * sz * 0.45, ang, sz * 1.0, sz * 0.28, w, w + sz * 0.9, m); break;
            case TP_MASTS: tube(u, v, ang, sz * 2.4, sz * 0.35, sz * 0.05, w + sz * 0.4, m); break;
            case TP_STATUES: ball(u + c * sz * 0.9, v + s * sz * 0.9, sz * 0.7, sz * 0.85, w, sz * 1.1, m, ang); for (int k = -1; k <= 1; k += 2) ball(u + c * sz * 1.35 - s * k * sz * 0.4, v + s * sz * 1.35 + c * k * sz * 0.4, sz * 0.22, sz * 0.22, w + sz * 0.8, sz * 0.3, IM_HOLE); break;
            default: box(u + c * sz * 0.35, v + s * sz * 0.35, ang, sz * 0.7, sz * 0.5, w, w + sz * 0.7, m); break;
        }
    }
    // pegs along a pegbox, out to both sides
    void pegs(double u, double v, double ang, double len, int count, double w, int mat) {
        const double c = std::cos(ang), s = std::sin(ang);
        for (int i = 0; i < count; i++) {
            const double t = (i / 2 + 0.5) / std::max(1, (count + 1) / 2), side = i % 2 ? 1 : -1;
            const double pu = u + c * len * t, pv = v + s * len * t;
            box(pu - s * side * len * 0.18, pv + c * side * len * 0.18, ang + PI / 2, len * 0.36, len * 0.035, w, w + len * 0.06, M.fit);
        }
    }
    // a tube whose radius follows the people's profile (narrowing, bulging, pinched at a waist), in `segs` lengths; cane its nodes
    double profileAt(double t, double taper, double waist) const {
        const double z = (t - 0.5) / 0.22;
        return (1 + (taper - 1) * t) * (1 - waist * std::exp(-z * z));
    }
    void profiledTube(double u, double v, double ang, double len, double r, double wc, int mat, int segs = 6) {
        const double c = std::cos(ang), s = std::sin(ang), tp = clampd(M.taper, 0.6, 1.3), ws = clampd(M.waist * 0.8, -0.35, 0.3);
        for (int i = 0; i < segs; i++) {
            const double t0 = (double)i / segs, t1 = (double)(i + 1) / segs;
            tube(u + c * len * t0, v + s * len * t0, ang, len / segs + (i + 1 < segs ? r * 0.2 : 0), r * profileAt(t0, tp, ws), r * profileAt(t1, tp, ws), wc, mat);
        }
        if (mat == IM_CANE) for (int j = 1; j <= 3; j++) { const double t = j / 4.0; band(u + c * len * t, v + s * len * t, ang, r * profileAt(t, tp, ws) * 1.04, r * 0.12, wc, IM_CANE); }   // its nodes
    }
    // a bell: a flare of four rings of tube
    void bell(double u, double v, double ang, double len, double r0, double r1, double wc, int mat) {
        const double c = std::cos(ang), s = std::sin(ang);
        for (int k = 0; k < 4; k++) {
            const double t0 = k / 4.0, t1 = (k + 1) / 4.0, ra = r0 + (r1 - r0) * std::pow(t0, 2.4), rb = r0 + (r1 - r0) * std::pow(t1, 2.4);
            InstrumentPart& p = tube(u + c * len * t0, v + s * len * t0, ang, len / 4, ra, rb, wc, mat); if (k == 3) p.open = true;
        }
    }
    // a tube along an arc round (cu, cv) from angle a0 to a1, its radius r0 to r1
    void arc(double cu, double cv, double R, double a0, double a1, double r0, double r1, double wc, int mat, int segs, double R1 = -1) {
        if (R1 < 0) R1 = R;
        double pu = cu + std::cos(a0) * R, pv = cv + std::sin(a0) * R;
        for (int k = 1; k <= segs; k++) {
            const double t = (double)k / segs, a = a0 + (a1 - a0) * t, rr = R + (R1 - R) * t, u = cu + std::cos(a) * rr, v = cv + std::sin(a) * rr;
            const double ra = r0 + (r1 - r0) * (k - 1) / segs, rb = r0 + (r1 - r0) * t;
            InstrumentPart& p = tubeTo(pu, pv, u, v, ra, rb, wc, mat); p.len += std::min(ra, rb) * 0.6;   // the joints overlap
            pu = u; pv = v;
        }
    }
    // a beater: a stick and its head
    void beater(double u, double v, double ang, double len, double headR, int stick, int headMat) {
        const double c = std::cos(ang), s = std::sin(ang);
        tube(u, v, ang, len, headR * 0.28, headR * 0.22, headR * 0.3, stick);
        ball(u + c * len, v + s * len, headR, headR, 0, headR * 1.6, headMat);
    }
    // a bow beside a bowed instrument: its stick, its hair, its frog
    void bowBeside(double u, double v, double ang, double len) {
        const double c = std::cos(ang), s = std::sin(ang), cu = -s * 0.012, cv = c * 0.012;
        const int stick = M.stick;
        if (M.age <= AGE_MEDIEVAL && M.variant % 2) arc(u + c * len * 0.5 - s * len * 1.2, v + s * len * 0.5 + c * len * 1.2, len * 1.25, ang - PI / 2 - 0.4, ang - PI / 2 + 0.4, 0.004, 0.003, 0.004, stick, 6);
        else tube(u, v, ang, len, 0.0045, 0.003, 0.0045, stick);
        str(u + cu, v + cv, 0.004, u + c * len * 0.97 + cu, v + s * len * 0.97 + cv, 0.004, M.age >= AGE_INDUSTRIAL ? IM_GUT : IM_FIBRE, -1, 0.0012);
        box(u + c * 0.03 + cu * 0.5, v + s * 0.03 + cv * 0.5, ang, 0.04, 0.008, 0, 0.016, M.fit);
    }
    // frets on a neck from its nut toward the body: the tuning's steps (an equal one's every step, a just one's its scale's notes)
    void frets(double nutU, double v, double scaleLen, double minU, double hw, double w) {
        const int steps = T.tuning == 0 ? T.division : n;
        for (int j = 1; j <= steps * 2 && j < 40; j++) {
            const double r = T.tuning == 0 ? std::pow(T.period, (double)j / std::max(1, T.division)) : ratio(j);
            const double u = nutU - scaleLen * (1 - 1 / r);
            if (u < minU) break;
            box(u, v, PI / 2, hw * 2, 0.0008, w, w + 0.0015, M.age >= AGE_INDUSTRIAL ? IM_WIRE : M.fit);
        }
    }
    // a keyboard along u from (u0, v): `count` keys a degree each (the first course the lowest), a dark key over a wide step
    void keyboard(double u0, double v, double len, double depth, int count, double w, int keyMat, int darkMat) {
        const double kw = len / count;
        for (int i = 0; i < count; i++) {
            box(u0 + kw * (i + 0.5), v, PI / 2, depth, kw * 0.44, w, w + 0.012, keyMat, i);
            const double step = std::log(ratio(i + 1) / ratio(i)) * 1200 / std::log(2.0);
            if (i + 1 < count && step > 160) box(u0 + kw * (i + 1), v + depth * 0.18, PI / 2, depth * 0.6, kw * 0.22, w + 0.012, w + 0.02, darkMat);
        }
    }
    // knobs in a row
    void knobs(double u0, double v, double len, int count, double r, double w, int mat) {
        for (int i = 0; i < count; i++) { const double u = u0 + len * (count == 1 ? 0.5 : (double)i / (count - 1)); disc(u, v, r, r, w, w + r * 0.9, mat); mark(u + r * 0.4, v, r * 0.18, w + r * 0.9, 1, IM_HOLE, -1, 0, r * 0.7); }
    }
    // the late ages' light: dots or lines (the people's light), lighting with their course
    void lights(double u0, double v, double len, int count, double w, bool courses) {
        if (M.age < AGE_FUTURISTIC && M.trim != IM_LIGHT) return;
        for (int i = 0; i < count; i++) {
            const double u = u0 + len * (count == 1 ? 0.5 : (double)i / (count - 1));
            if (M.light == LT_LINES) box(u, v, PI / 2, 0.02, 0.0025, w, w + 0.002, IM_LIGHT, courses ? i : -1);
            else mark(u, v, 0.004, w, 0, IM_LIGHT, courses ? i : -1);
        }
    }
};

// the last element made carries its course (a plate made without one)
void out_course(Mk& k, int i) { InstrumentPart& p = k.out.back(); if (p.course < 0 && p.kind != IPK_MARK) p.course = i; }
bool T_frets(Mk& k) { return k.M.variant % 4 == 3; }

// ---- edge-blown -------------------------------------------------------------------------------------------------------------------

void pPipe(Mk& k, bool cross) {
    const InstrumentMake& M = k.M;
    const bool bone = M.mat == IM_BONE || M.mat == IM_HORN || M.mat == IM_CHITIN;
    const double L = (bone ? k.rng.range(0.16, 0.26) : k.rng.range(0.28, 0.48)) * k.S * (0.85 + 0.35 * M.slender) * (cross ? 1.3 : 1.0);
    const double r = (bone ? 0.011 : 0.0105) * (0.6 + 0.4 * k.S) * (1.6 - 0.9 * M.slender) * k.rng.range(0.8, 1.25), r1 = r * (M.taper < 0.75 ? 0.8 : (M.taper > 1.05 ? 1.12 : 1.0));
    const double u0 = -L / 2, wc = r * std::max(1.0, k.profileAt(0.5, clampd(M.taper, 0.6, 1.3), clampd(M.waist * 0.8, -0.35, 0.3))), top = wc + r;
    (void)r1;
    if (bone) k.tube(u0, 0, 0, L, r, r * (M.taper < 0.8 ? 0.85 : 1.0), wc, M.mat);   // a bone is as it grew
    else k.profiledTube(u0, 0, 0, L, r, wc, M.mat);
    if (bone && k.rng.chance(0.7)) { k.ball(u0 + r * 0.2, 0, r * 1.4, r * 1.2, 0, r * 2.4, M.mat); k.ball(-u0 - r * 0.2, 0, r * 1.25, r * 1.1, 0, r * 2.2, M.mat); }   // a bone's ends
    if (cross) { k.mark(u0 + 0.13 * L, 0, r * 0.6, top, 0); k.disc(u0 + 0.004, 0, r * 1.08, r * 1.08, 0, 2.1 * r, M.mat2); }
    else switch (M.variant % 3) {
        case 0: k.mark(u0 + 0.01, 0, r * 0.45, top, 1, IM_HOLE, -1, 0, 0.022); break;                                              // a notch
        case 1: k.box(u0 + 0.028, 0, 0, 0.05, r * 0.72, top - 0.002, top + 0.004, M.mat2); k.mark(u0 + 0.062, 0, r * 0.45, top, 2); break;   // a block and its window
        default: k.band(u0 + 0.012, 0, 0, r * 1.05, r * 0.4, wc, M.mat2); k.mark(u0 + 0.035, 0, r * 0.35, top, M.hole); break;  // a mouthpiece ring
    }
    const int nh = clampi(k.n - 1 + (M.number >= 7 ? 1 : 0), 3, 8);
    k.holesAlong(u0 + L * 0.36, 0, 0, L * 0.52, top, r * 0.42, nh);
    k.tubeOrnament(u0, 0, 0, L, r, wc);
    if (!bone) switch (M.variant / 3 % 4) {   // its far end: carved in the people's tops, flared, capped, or cut plain
        case 0: k.head(-u0, 0, 0, 0, r * 1.1, M.mat2); break;
        case 1: k.bell(-u0, 0, 0, r * 4, r, r * 2.2, wc, M.mat); break;
        case 2: k.disc(-u0 - 0.003, 0, r * 1.25, r * 0.5, 0, wc + r * 1.25, M.mat2); break;
        default: break;
    }
}

void pVessel(Mk& k) {
    const InstrumentMake& M = k.M;
    const double L = k.rng.range(0.09, 0.16) * k.S, H = L * k.rng.range(0.55, 0.85), th = H * 0.9;
    const size_t bi = k.out.size();
    k.body(0, 0, 0, L, H * 0.5, 0, th * 0.55, M.mat, 1.6);
    k.out[bi].dome = std::max(k.out[bi].dome, th * 0.35);
    const InstrumentPart b = k.out[bi];
    if (M.variant % 3 != 2) k.tube(L * 0.42, 0, 0, L * (M.variant % 3 ? 0.18 : 0.3), H * 0.12, H * 0.09, H * 0.13, M.mat);   // a beak to blow in
    else k.mark(L * 0.2, 0, H * 0.08, k.onTop(b, L * 0.2, 0), 0);                                                         // or a hole on top, blown across
    const int nh = clampi(k.n - 1, 3, 9);
    if (M.sym == SY_RADIAL || M.sym == SY_ROTATIONAL) {
        for (int i = 0; i < nh; i++) { const double a = TAU * i / nh + (M.sym == SY_ROTATIONAL ? 0.3 * i : 0), u = std::cos(a) * L * 0.24, v = std::sin(a) * H * 0.26; k.mark(u, v, H * 0.045, k.onTop(b, u, v), M.hole, IM_HOLE, i, a); }
    } else {
        const std::vector<double> at = k.rhythm(nh);
        for (int i = 0; i < nh; i++) { const double u = -L * 0.34 + L * 0.6 * at[i], v = (i % 2 ? 1 : -1) * H * (M.sym == SY_NONE ? 0.08 + 0.12 * k.rng.uni() : 0.16); k.mark(u, v, H * 0.045, k.onTop(b, u, v), M.hole, IM_HOLE, i); }
    }
    if (M.variant % 3 != 2) k.mark(L * 0.18, 0, H * 0.05, k.onTop(b, L * 0.18, 0), 2);   // the window
    k.bodyOrnament(b, L * 0.3);
}

void pRaft(Mk& k) {
    const InstrumentMake& M = k.M;
    const int nt = clampi(k.n + 1, 5, 10);
    const double L0 = k.rng.range(0.18, 0.32) * k.S, r = k.rng.range(0.007, 0.011) * (0.7 + 0.3 * k.S), gap = 2.15 * r;
    const int style = M.sym == SY_BILATERAL ? (M.variant % 2) : (M.sym == SY_NONE ? 0 : 2);   // graded, longest in the middle (a V), or on an arc
    std::vector<int> order(nt);
    for (int i = 0; i < nt; i++) order[i] = i;
    if (style == 1) { std::vector<int> o; for (int i = nt - 1; i >= 0; i -= 2) o.push_back(i); for (int i = nt % 2 ? 1 : 0; i < nt; i += 2) o.insert(o.begin(), i); order = o; }
    for (int j = 0; j < nt; j++) {
        const int i = order[j];
        const double len = L0 / k.ratio(i), v = -gap * (nt - 1) * 0.5 + gap * j, u0 = style == 2 ? -0.05 * std::cos((j - (nt - 1) * 0.5) / nt * 2.4) : 0;
        k.tube(u0, v, 0, len, r, r, r, M.mat, i);
        k.mark(u0 + 0.004, v, r * 0.6, 2 * r, 0);   // its open top
    }
    const int bind = M.mat2 == IM_FIBRE || M.mat2 == IM_HIDE ? M.mat2 : (M.age <= AGE_MEDIEVAL ? IM_FIBRE : M.mat2);
    for (int b = 0; b < (M.deco == ON_BANDED ? 3 : 2); b++) k.box(0.03 + 0.035 * b, 0, PI / 2, gap * nt + 0.012, 0.004, 2 * r - 0.002, 2 * r + 0.003, b == 1 ? k.accentMat(AP_BANDS) : bind);
    if (M.deco == ON_CARVED) k.box(0.05, 0, PI / 2, gap * nt + 0.02, 0.02, 0, 2 * r + 0.004, M.stick);
}

void pPipeRing(Mk& k) {
    const InstrumentMake& M = k.M;
    const int nt = clampi(k.n + 1, 5, 10);
    const double hub = k.rng.range(0.03, 0.05) * k.S, L0 = k.rng.range(0.13, 0.22) * k.S, r = 0.008 * (0.7 + 0.3 * k.S);
    k.disc(0, 0, hub, hub, 0, 2.6 * r, M.mat2, -1, M.sides);
    k.mark(0, 0, hub * 0.35, 2.6 * r, M.hole);
    for (int i = 0; i < nt; i++) {
        const double a = TAU * i / nt, tw = M.sym == SY_ROTATIONAL ? 0.45 : 0, len = L0 / k.ratio(i);
        k.tube(std::cos(a) * hub * 0.85, std::sin(a) * hub * 0.85, a + tw, len, r, r * 1.1, r, M.mat, i);
        k.mark(std::cos(a) * hub * 0.85 + std::cos(a + tw) * len * 0.92, std::sin(a) * hub * 0.85 + std::sin(a + tw) * len * 0.92, r * 0.5, 2 * r, 0);
    }
}

void pTwinPipe(Mk& k) {
    const InstrumentMake& M = k.M;
    const int pipes = M.number >= 7 ? 3 : 2;
    const double L = k.rng.range(0.24, 0.4) * k.S, r = 0.009 * (0.7 + 0.3 * k.S), spread = M.variant % 2 ? 0 : k.rng.range(0.07, 0.18);
    const int nh = clampi(k.n - 1, 3, 7);
    for (int p = 0; p < pipes; p++) {
        const double a = (p - (pipes - 1) * 0.5) * spread, v0 = (p - (pipes - 1) * 0.5) * 2.2 * r;
        k.tube(-L / 2, v0, a, L * (p == pipes - 1 && pipes == 3 ? 1.25 : 1.0), r, r * 1.05, r, M.mat);
        if (!(pipes == 3 && p == pipes - 1)) k.holesAlong(-L / 2 + std::cos(a) * L * 0.36, v0 + std::sin(a) * L * 0.36, a, L * 0.5, 2 * r, r * 0.4, p == 0 ? (nh + 1) / 2 : nh / 2, p == 0 ? 0 : (nh + 1) / 2);
    }
    k.ball(-L / 2 - r, 0, r * 2.2, r * 1.6, 0, r * 2.6, M.mat2);   // the shared mouthpiece
    k.tubeOrnament(-L / 2, 0, 0, L, r * (pipes + 0.5), r);
}

// ---- reeds ------------------------------------------------------------------------------------------------------------------------

void pReedPipe(Mk& k, bool twin) {
    const InstrumentMake& M = k.M;
    const double L = k.rng.range(0.24, 0.46) * k.S, r0 = 0.0075 * (0.7 + 0.3 * k.S) * k.rng.range(0.8, 1.5), r1 = r0 * (twin ? 1.15 : k.rng.range(1.4, 2.6)), wc = r1 * 1.3;
    const int pipes = twin ? 2 : 1, nh = clampi(k.n - 1, 3, 8), end = M.variant / 2 % 3;   // its end: a bell, a hook curving back, or a bulb
    const double straight = end == 1 ? L * 0.72 : L;
    for (int p = 0; p < pipes; p++) {
        const double v = (p - (pipes - 1) * 0.5) * 2.3 * r1;
        const double rs = r0 + (r1 - r0) * straight / L;
        k.tube(-L / 2, v, 0, straight, r0, rs, wc, M.mat);
        const int bellMat = M.variant % 3 == 0 && M.age >= AGE_MEDIEVAL ? M.mat2 : M.mat;
        if (twin && !(M.variant % 2) && p) { k.holesAlong(-L / 2 + L * 0.3, v, 0, L * 0.55, wc + (r0 + r1) * 0.5, r0 * 0.45, nh / 2, (nh + 1) / 2); continue; }
        double eu = -L / 2 + straight, ev = v, ea = 0;
        if (end == 1) {   // a hook: the last of it curving round
            const double R = L * 0.16, side = M.skew >= 0 ? 1 : -1;
            k.arc(eu, v + side * R, R, -side * PI / 2, -side * PI / 2 + side * 2.2, rs, r1, wc, M.mat, 8);
            const double a1 = -side * PI / 2 + side * 2.2;
            eu = eu + std::cos(a1) * R; ev = v + side * R + std::sin(a1) * R; ea = a1 + side * PI / 2;
        }
        if (end == 2) k.ball(eu + r1 * 1.4, ev, r1 * 2.2, r1 * 2.6, wc - r1 * 2.2, r1 * 4.2, bellMat);   // a bulb, holed
        else k.bell(eu, ev, ea, k.rng.range(0.05, 0.1) * k.S, r1, r1 * k.rng.range(2.0, 4.2), wc, bellMat);
        k.holesAlong(-L / 2 + L * 0.3, v, 0, straight * 0.62, wc + (r0 + r1) * 0.5, r0 * 0.45, twin ? (p == 0 ? (nh + 1) / 2 : nh / 2) : nh, twin && p ? (nh + 1) / 2 : 0);
    }
    k.tube(-L / 2 - 0.035, 0, 0, 0.037, 0.0025, r0 * 0.6, wc, M.fit);   // the reed's staple
    k.box(-L / 2 - 0.045, 0, 0, 0.018, 0.0035, wc - 0.002, wc + 0.002, M.mat2);
    if (M.variant % 4 == 1) k.disc(-L / 2 - 0.005, 0, r0 * 2.2, r0 * 2.2, wc - r0 * 1.2, wc + r0 * 0.3, M.fit);   // a lip rest
    if (M.age >= AGE_INDUSTRIAL && M.variant % 3 != 2) for (int i = 0; i < 4; i++) k.box(-L * 0.05 + L * 0.08 * i, r1 * 1.8, 0, 0.022, 0.004, wc + r0, wc + r0 + 0.004, M.mat2, i);   // keys
    k.tubeOrnament(-L / 2, 0, 0, straight, (r0 + r1) * 0.5 * (twin ? 2 : 1), wc);
}

void pBag(Mk& k) {
    const InstrumentMake& M = k.M;
    const double bl = k.rng.range(0.3, 0.4) * k.S, bw = bl * k.rng.range(0.55, 0.72), th = bw * 0.5;
    InstrumentPart& bag = k.plate(0, 0, 0, bl, bw * 0.5, 0, th * 0.6, IM_HIDE, 0.7, -0.1, 2.2, th * 0.4, 0, M.skew);
    if (M.painted > 0.4 || M.deco >= ON_CARVED) { InstrumentPart cover = bag; cover.mat = IM_PAINT; cover.r0 *= 0.8; cover.len *= 0.85; cover.w0 = cover.w1; cover.w1 += 0.001; k.out.push_back(cover); }
    // the chanter, its holes
    const double cl = k.rng.range(0.24, 0.34) * k.S, cr = 0.009;
    k.tube(bl * 0.42, -bw * 0.1, -0.12, cl, cr, cr * 1.5, th * 0.5, M.mat2);
    k.bell(bl * 0.42 + std::cos(-0.12) * cl, -bw * 0.1 + std::sin(-0.12) * cl, -0.12, 0.04, cr * 1.5, cr * 2.6, th * 0.5, M.mat2);
    k.holesAlong(bl * 0.42 + std::cos(-0.12) * cl * 0.3, -bw * 0.1 + std::sin(-0.12) * cl * 0.3, -0.12, cl * 0.6, th * 0.5 + cr * 1.2, cr * 0.4, clampi(k.n - 1, 3, 8));
    // the drones, fanned from its top
    const int dr = clampi(M.number - 3, 1, 4);
    const double sheaf = PI * k.rng.range(0.66, 0.8);
    for (int d = 0; d < dr; d++) {
        const double a = sheaf + 0.1 * (d - (dr - 1) * 0.5), len = k.rng.range(0.3, 0.45) * k.S * (1 - 0.12 * d);
        const double su = -bl * 0.05 + 0.04 * (d - (dr - 1) * 0.5), sv = bw * 0.36;
        k.tube(su, sv, a, len, 0.008, 0.011, th * 0.55, M.mat2);
        for (int j = 1; j < 3; j++) k.band(su + std::cos(a) * len * j / 3, sv + std::sin(a) * len * j / 3, a, 0.0115, 0.003, th * 0.55, k.accentMat(AP_BANDS));
        k.disc(su + std::cos(a) * len, sv + std::sin(a) * len, 0.016, 0.016, th * 0.55 - 0.014, th * 0.55 + 0.014, M.mat2);
    }
    k.tube(-bl * 0.45, -bw * 0.25, PI + 0.5, 0.14, 0.006, 0.005, th * 0.5, M.mat2);   // the blowpipe
}

void pChamber(Mk& k) {
    const InstrumentMake& M = k.M;
    const double R = k.rng.range(0.06, 0.1) * k.S;
    k.ball(0, 0, R, R * k.rng.range(0.9, 1.2), 0, R * 1.5, M.mat);
    k.tube(-R * 0.9, 0, PI, R * k.rng.range(0.8, 1.6), R * 0.18, R * 0.12, R * 0.5, M.mat);   // the neck to blow in
    const int np = clampi(k.n, 3, 9);
    const double L0 = k.rng.range(0.22, 0.36) * k.S, r = 0.0075;
    for (int i = 0; i < np; i++) {
        double a, v0;
        if (M.sym == SY_RADIAL || M.sym == SY_ROTATIONAL) { a = -0.6 + 1.2 * i / std::max(1, np - 1); v0 = std::sin(a) * R * 0.5; }
        else { a = (i - (np - 1) * 0.5) * 0.06; v0 = (i - (np - 1) * 0.5) * 2.3 * r; }
        const double len = L0 / k.ratio(i) * (i == np / 2 ? 1.15 : 1.0);
        k.tube(R * 0.55, v0, a, len, r, r, R * 0.85, M.mat2, i);
        k.mark(R * 0.55 + std::cos(a) * len * 0.55, v0 + std::sin(a) * len * 0.55, r * 0.42, R * 0.85 + r, M.hole, IM_HOLE, i);
    }
    k.band(R * 0.55, 0, PI / 2, np * r * 1.3, 0.004, R * 0.85, k.accentMat(AP_BANDS));
}

// ---- free reeds -------------------------------------------------------------------------------------------------------------------

void pBellows(Mk& k, bool round) {
    const InstrumentMake& M = k.M;
    const int keys = clampi(2 * k.n, 8, 18);
    if (round) {   // standing on its end: its board, its buttons in a ring, its folds round it
        const double R = k.rng.range(0.09, 0.13) * k.S;
        const int sides = M.sides >= 5 ? M.sides : 6 + 2 * (M.variant % 2);
        for (int f = 3; f >= 1; f--) k.disc(0, 0, R * (1 + 0.04 * f), R * (1 + 0.04 * f), 0, 0.1 - 0.02 * f, f % 2 ? M.mat2 : IM_HOLE, -1, sides);
        k.disc(0, 0, R, R, 0, 0.11, M.mat, -1, sides);
        for (int i = 0; i < keys; i++) { const double a = TAU * i / keys, rr = R * (i % 2 ? 0.62 : 0.78); k.disc(std::cos(a) * rr, std::sin(a) * rr, R * 0.06, R * 0.06, 0.11, 0.118, M.fit, i); }
        k.soundHole(0, 0, R * 0.22, 0.111);
        k.box(0, -R * 0.2, 0, R * 1.2, R * 0.05, 0.11, 0.115, IM_HIDE);   // the strap
        return;
    }
    const double W = k.rng.range(0.1, 0.16) * k.S, D = k.rng.range(0.16, 0.24) * k.S, folds = 5 + M.variant % 5;
    const double fw = 0.012 + 0.004 * k.S;
    for (int e = -1; e <= 1; e += 2) {
        InstrumentPart& b = k.body(e * (folds * fw * 0.5 + 0.012), 0, PI / 2, D, 0.012, 0, W, M.mat, 0);
        b.square = std::max(b.square, 3.0); b.taper = 1; b.waist = 0;
    }
    for (int f = 0; f < folds; f++) k.box(-folds * fw * 0.5 + fw * (f + 0.5), 0, PI / 2, D * 0.94, fw * 0.48, 0, W * 0.96, f % 2 ? M.mat2 : IM_HOLE);
    for (int i = 0; i < keys; i++) { const int row = i % 2; k.disc(folds * fw * 0.5 + 0.012 + (row ? 0.004 : -0.004), -D * 0.4 + D * 0.8 * i / (keys - 1), 0.006, 0.006, W, W + 0.006, M.fit, i); }
    for (int i = 0; i < 4; i++) k.mark(-folds * fw * 0.5 - 0.012, -D * 0.3 + D * 0.2 * i, 0.004, W, 0, M.fit);
    k.box(0, D * 0.45, 0, folds * fw + 0.03, 0.006, W, W + 0.004, IM_HIDE);   // the strap
}

// ---- lips -------------------------------------------------------------------------------------------------------------------------

void valves(Mk& k, double u, double v, double ang, double wc, double r) {
    const InstrumentMake& M = k.M;
    const int nv = clampi(M.number / 2 + 1, 3, 5), kind = M.variant % 4;   // pistons, rotary discs, keys, or none (a natural horn)
    const double c = std::cos(ang), s = std::sin(ang);
    for (int i = 0; i < nv && kind != 3; i++) {
        const double pu = u + c * 0.024 * i, pv = v + s * 0.024 * i;
        if (kind == 0) { k.disc(pu, pv, 0.009, 0.009, 0, wc + 0.05, M.mat, i); k.disc(pu, pv, 0.012, 0.012, wc + 0.05, wc + 0.056, k.trimMat(), i); }
        else if (kind == 1) { k.disc(pu, pv, 0.015, 0.015, wc - 0.012, wc + 0.012, M.mat, i); k.box(pu, pv + 0.02, ang + PI / 2, 0.03, 0.003, wc + 0.008, wc + 0.012, M.mat2, i); }
        else { k.box(pu, pv + r * 1.6, ang, 0.018, 0.006, wc + r, wc + r + 0.004, k.trimMat(), i); k.mark(pu, pv, r * 0.5, wc + r, 0); }
    }
}
void mouthpiece(Mk& k, double u, double v, double ang, double wc) {
    const double c = std::cos(ang), s = std::sin(ang);
    k.tube(u, v, ang, 0.035, 0.004, 0.006, wc, k.M.mat);
    k.disc(u + c * 0.002, v + s * 0.002, 0.011, 0.011, wc - 0.011, wc + 0.004, k.M.mat);
}

void pHorn(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    const double r = 0.0065 * (0.7 + 0.3 * k.S) * (M.reg < 0 ? 1.8 : 1.0), wc = 0.045 * (M.reg < 0 ? 1.6 : 1.0);
    const int tm = M.mat;
    switch (plan) {
        case IP_HORN_COIL: case IP_HORN_UPRIGHT: {
            const double R = k.rng.range(0.07, 0.12) * k.S * (plan == IP_HORN_UPRIGHT ? 1.7 : 1.0), turns = k.rng.range(1.1, 2.3), ovalK = M.square > 5 ? 0.62 : (M.sides >= 5 ? 0.8 : 1.0);
            // the coil (an oval where the people's bodies are boxes), tightening toward the bell
            const int segs = (int)(14 * turns);
            double pu = 0, pv = -R;
            for (int s2 = 1; s2 <= segs; s2++) {
                const double t = (double)s2 / segs, a = -PI / 2 + TAU * turns * t, rr = R * (1 - 0.22 * t);
                const double u = std::cos(a) * rr, v = std::sin(a) * rr * ovalK;
                InstrumentPart& p = k.tubeTo(pu, pv, u, v, r * (1 + 0.6 * (t - 1.0 / segs)), r * (1 + 0.6 * t), wc, tm); p.len += r * 0.6;
                pu = u; pv = v;
            }
            const double bl = k.rng.range(0.16, 0.24) * k.S * (plan == IP_HORN_UPRIGHT ? 1.5 : 1.0), ba = M.variant % 3 == 2 ? PI : (M.variant % 3 == 1 ? -0.5 : 0);
            k.bell(pu, pv, ba, bl, r * 1.6, k.rng.range(0.05, 0.08) * k.S * (plan == IP_HORN_UPRIGHT ? 1.8 : 1.0), wc, tm);
            mouthpiece(k, -R * 1.25, R * 0.3 * ovalK, PI * 0.95, wc);
            k.tubeTo(-R * 1.25, R * 0.3 * ovalK, -R * 0.82, R * 0.25 * ovalK, r * 0.9, r, wc, tm);
            valves(k, -R * 0.2, R * 0.55 * ovalK, 0, wc, r);
            if (M.number >= 6) k.arc(R * 0.55, R * 0.3 * ovalK, R * 0.22, -PI / 2, PI / 2, r * 0.9, r * 0.9, wc + r, tm, 6);   // a tuning loop
            break;
        }
        case IP_HORN_FOLDED: {
            const double L = k.rng.range(0.22, 0.34) * k.S, gap = k.rng.range(0.05, 0.08) * k.S;
            k.tube(-L / 2, gap / 2, 0, L, r, r * 1.2, wc, tm); k.tube(-L / 2, -gap / 2, 0, L, r * 1.2, r * 1.5, wc, tm);
            k.arc(L / 2, 0, gap / 2, -PI / 2, PI / 2, r * 1.2, r * 1.2, wc, tm, 6);
            k.arc(-L / 2, -gap, gap / 2, PI / 2, PI * 1.5, r * 1.5, r * 1.7, wc, tm, 6);
            k.bell(-L / 2, -gap * 1.5, 0, L * 0.9, r * 1.7, k.rng.range(0.045, 0.07) * k.S, wc, tm);
            mouthpiece(k, -L / 2 - 0.035, gap / 2, 0, wc);
            valves(k, -L * 0.1, gap / 2, 0, wc, r);
            break;
        }
        case IP_HORN_STRAIGHT: {
            const double L = k.rng.range(0.55, 0.95) * k.S, bl = L * 0.3;
            k.tube(-L / 2, 0, 0, L - bl, r, r * 1.3, wc, tm);
            k.bell(L / 2 - bl, 0, 0, bl, r * 1.3, k.rng.range(0.05, 0.09) * k.S, wc, tm);
            mouthpiece(k, -L / 2 - 0.03, 0, 0, wc);
            if (M.variant % 4 == 2) { for (int i = 0; i < clampi(k.n - 1, 3, 7); i++) { k.mark(-L * 0.05 + 0.035 * i, 0, r * 0.45, wc + r * 1.2, 0, IM_HOLE, i); k.box(-L * 0.05 + 0.035 * i, r * 2.2, 0, 0.02, 0.005, wc + r, wc + r + 0.004, k.trimMat(), i); } }
            else valves(k, -L * 0.25, 0, 0, wc, r);
            k.tubeOrnament(-L / 2, 0, 0, L - bl, r * 1.15, wc);
            if (M.deco >= ON_CARVED) for (int t = 0; t < 3; t++) k.str(-L * 0.3 + 0.08 * t, 0.01, wc + 0.01, -L * 0.3 + 0.08 * t + 0.05, 0.05 + 0.02 * t, 0.002, IM_FIBRE);   // a cord hanging
            break;
        }
        case IP_HORN_SPIRAL: {
            const double R1 = k.rng.range(0.08, 0.13) * k.S, turns = k.rng.range(1.8, 2.8);
            k.arc(0, 0, R1 * 0.22, 0, TAU * turns, r * 0.8, r * 2.2, wc, tm, (int)(14 * turns), R1);
            const double ea = TAU * turns;
            k.bell(std::cos(ea) * R1, std::sin(ea) * R1, ea + PI / 2, k.rng.range(0.06, 0.1) * k.S, r * 2.2, k.rng.range(0.04, 0.065) * k.S, wc, tm);
            mouthpiece(k, R1 * 0.22, -0.035, -PI / 2, wc);
            if (M.age >= AGE_INDUSTRIAL) valves(k, -R1 * 0.5, R1 * 0.2, 0.3, wc, r);
            break;
        }
        default: {   // forked: a main tube that splits into bells
            const double L = k.rng.range(0.2, 0.3) * k.S;
            const int nb = clampi(M.number / 2, 2, 4);
            k.tube(-L, 0, 0, L, r, r * 1.3, wc, tm);
            mouthpiece(k, -L - 0.035, 0, 0, wc);
            for (int b = 0; b < nb; b++) {
                const double a = (b - (nb - 1) * 0.5) * 0.55, bl = k.rng.range(0.18, 0.28) * k.S;
                k.tube(0, 0, a, bl * 0.6, r * 1.3, r * 1.5, wc, tm);
                k.bell(std::cos(a) * bl * 0.6, std::sin(a) * bl * 0.6, a, bl * 0.5, r * 1.5, k.rng.range(0.035, 0.055) * k.S, wc, tm);
            }
            k.disc(0, 0, r * 3, r * 3, wc - r * 2, wc + r * 2, k.trimMat());
            valves(k, -L * 0.7, 0, 0, wc, r);
            break;
        }
    }
}

// ---- plucked ----------------------------------------------------------------------------------------------------------------------

// a body seen from its face: a gourd's, a shell's or a pot's back under its face of skin or wood, or a box (`box`)
InstrumentPart& faceBody(Mk& k, double u, double len, double hw, double th, bool box, int faceMat) {
    const InstrumentMake& M = k.M;
    if (!box) {
        k.ball(u, 0, hw * 1.02, len * 0.51, 0, th, M.mat);
        InstrumentPart& f = k.body(u, 0, 0, len * 0.94, hw * 0.94, th * 0.55, th * 0.6, faceMat, 0.2);
        return f;
    }
    InstrumentPart& b = k.body(u, 0, 0, len, hw, 0, th, M.mat, 0.15);
    return b;
}

void pNecked(Mk& k, bool bowed) {
    const InstrumentMake& M = k.M;
    const bool box = M.variant % 2 == 1, bass = M.reg < 0;
    const double s = k.S * (bass ? 1.25 : 1.0);
    const double bl = k.rng.range(0.22, 0.36) * s, bw = bl * k.rng.range(0.32, 0.46) * (1.2 - 0.4 * M.slender), th = bw * 0.55;
    const double nl = (bowed ? k.rng.range(0.18, 0.28) : (M.slender > 0.6 ? k.rng.range(0.4, 0.62) : k.rng.range(0.18, 0.34))) * s;
    const int faceMat = !box && (M.mat == IM_GOURD || M.mat == IM_SHELL || M.mat == IM_CHITIN || M.mat == IM_CLAY || M.mat == IM_PITH) ? (k.rng.chance(0.5) ? IM_HIDE : M.mat2) : M.mat;
    InstrumentPart& face = faceBody(k, 0, bl, bw, th, box, faceMat);
    if (bowed && box) { face.waist = std::max(face.waist, 0.22); face.square = std::min(face.square, 3.0); }
    const double fw = face.w1, nw = 0.012 * s, top = fw + 0.012;
    // the sound holes: a rosette, two slits for a bowed one, scattered small ones on an encrusted people's
    if (faceMat != IM_HIDE) {
        if (bowed) for (int e = -1; e <= 1; e += 2) k.mark(-bl * 0.02, e * bw * 0.42, bw * 0.05, fw, 1, IM_HOLE, -1, 0, bl * 0.28);
        else if (M.deco == ON_ENCRUSTED) for (int i = 0; i < M.number; i++) { const double a = TAU * i / M.number; k.mark(bl * 0.12 + std::cos(a) * bw * 0.25, std::sin(a) * bw * 0.25, bw * 0.05, fw, M.hole); }
        else k.soundHole(bl * 0.12, 0, bw * 0.17, fw);
    }
    k.bodyOrnament(face, bw * 0.3);
    // the neck, its frets, its head and pegs
    const double n0 = bl * 0.46, n1 = n0 + nl;
    k.box((n0 + n1) * 0.5, 0, 0, nl, nw, th * 0.5, top, M.mat2);
    if (!bowed && (M.age >= AGE_MEDIEVAL || T_frets(k))) k.frets(n1, 0, (n1 + bl * 0.3) * 1.0, n0 + 0.01, nw, top);
    const int ns = bowed ? clampi(M.number / 2 + 2, 2, 5) : clampi(bass ? 4 : M.number - 1, 3, 7);
    const double hl = k.rng.range(0.07, 0.11) * s, ha = M.variant % 3 == 0 ? 0.0 : (M.variant % 3 == 1 ? 0.35 : -0.25);
    k.box(n1 + std::cos(ha) * hl * 0.5, std::sin(ha) * hl * 0.5, ha, hl, nw * 1.25, th * 0.5, top, M.mat2);
    k.pegs(n1, 0, ha, hl, ns, top, M.fit);
    k.head(n1 + std::cos(ha) * hl, std::sin(ha) * hl, ha, th * 0.5, nw * 1.4, M.mat2);
    // the bridge, the strings
    const double bu = -bl * 0.28;
    k.box(bu, 0, PI / 2, nw * 2.6, 0.003, fw, fw + 0.008, M.fit);
    if (bowed) k.box(-bl * 0.44, 0, 0, bl * 0.12, nw * 0.9, fw, fw + 0.005, M.fit);   // the tailpiece
    for (int i = 0; i < ns; i++) {
        const double v0 = (i - (ns - 1) * 0.5) * nw * 1.7 / std::max(1, ns - 1) * 1.2, v1 = (i - (ns - 1) * 0.5) * nw * 1.4 / std::max(1, ns - 1);
        k.str(bu - (bowed ? bl * 0.12 : 0.01), v0, fw + 0.009, n1 + 0.004, v1, top + 0.002, M.str, ns - 1 - i, bass ? 0.0011 : 0.0007);
    }
    if (bowed) k.bowBeside(-bl * 0.45, -bw * 1.7, 0.08, (bl + nl) * 1.05);
    if (bass) k.tube(-bl * 0.5, 0, PI, 0.12 * s, 0.006, 0.004, th * 0.5, M.fit);   // its spike to stand on
}
void pFrame(Mk& k) {
    const InstrumentMake& M = k.M;
    const double bw = k.rng.range(0.14, 0.24) * k.S, bl = bw * k.rng.range(0.6, 0.9), th = 0.05 * k.S;
    const double armL = k.rng.range(0.24, 0.38) * k.S, spread = bw * k.rng.range(0.42, 0.55);
    const bool body = M.mat == IM_SHELL || M.mat == IM_CHITIN || M.mat == IM_GOURD;
    InstrumentPart& b = body ? k.ball(0, 0, bw * 0.52, bl * 0.55, 0, th * 1.3, M.mat) : k.body(0, 0, 0, bl, bw * 0.5, 0, th, M.mat, 0.4);
    const double fw = b.w1;
    if (!body) k.soundHole(0, 0, bw * 0.08, fw);
    else k.body(0, 0, 0, bl * 0.86, bw * 0.4, th * 0.9, th * 0.95, IM_HIDE, 0);
    // the arms: straight, or curving like horns, or (no symmetry) one longer than the other
    const bool curved = M.mat2 == IM_HORN || M.mat2 == IM_BONE || M.mat2 == IM_CHITIN || M.variant % 3 == 0;
    double tipU[2], tipV[2];
    for (int e = 0; e < 2; e++) {
        const double side = e ? 1 : -1, len = armL * (M.sym == SY_NONE && e ? 1.15 : 1.0), r = 0.011 * (0.7 + 0.3 * k.S);
        if (curved) {   // curving out like horns: an arc round a centre beside the arm's foot
            const double R = len * 0.75, cu = bl * 0.3, cv = side * (spread + R), a0 = -side * PI / 2, a1 = a0 + side * 1.3;
            k.arc(cu, cv, R, a0, a1, r * 1.3, r * 0.8, th * 0.6, M.mat2, 8);
            tipU[e] = cu + std::cos(a1) * R; tipV[e] = cv + std::sin(a1) * R;
        } else {
            const double a = side * k.rng.range(0.14, 0.34);
            k.tube(bl * 0.3, side * spread, a, len, r * 1.2, r, th * 0.6, M.mat2);
            tipU[e] = bl * 0.3 + std::cos(a) * len; tipV[e] = side * spread + std::sin(a) * len;
        }
    }
    // the yoke across the arms' tips, the strings to it
    const double yu = std::min(tipU[0], tipU[1]) - 0.01;
    k.boxTo(yu, tipV[0] - 0.03, yu, tipV[1] + 0.03, 0.009, th * 0.6, th * 0.6 + 0.02, M.mat2);   // the yoke, past the arms
    for (int e = 0; e < 2; e++) k.head(yu, e ? tipV[1] + 0.03 : tipV[0] - 0.03, e ? PI / 2 : -PI / 2, th * 0.6, 0.01, M.mat2);
    const int ns = clampi(k.n, 5, 10);
    k.box(-bl * 0.25, 0, PI / 2, spread * 1.4, 0.004, fw, fw + 0.008, M.fit);   // the tailpiece
    for (int i = 0; i < ns; i++) {
        const double t = (i + 0.5) / ns, v0 = -spread * 0.6 + spread * 1.2 * t, v1 = tipV[0] * 0.8 + (tipV[1] * 0.8 - tipV[0] * 0.8) * t;
        k.str(-bl * 0.25, v0, fw + 0.008, yu, v1, th * 0.6 + 0.02, M.str, i, 0.0007);
        k.ball(yu, v1, 0.004, 0.004, th * 0.6 + 0.018, 0.006, M.fit);
    }
    k.box(bl * 0.15, 0, PI / 2, spread * 1.3, 0.003, fw, fw + 0.01, M.fit);   // the bridge
}

void pHarp(Mk& k) {
    const InstrumentMake& M = k.M;
    const int ns = clampi(2 * k.n, 8, 16);
    const double L = k.rng.range(0.45, 0.75) * k.S * (M.reg < 0 ? 1.2 : 1.0), H0 = L * k.rng.range(0.55, 0.85), th = 0.06 * k.S;
    const int style = M.variant % 3;   // arched, angled, framed
    // the resonator along u at v = 0: a boat of the people's outline
    InstrumentPart& res = k.body(0, 0, 0, L, k.rng.range(0.035, 0.06) * k.S, 0, th, M.mat, 0.6);
    res.taper = std::min(res.taper, 0.75);
    for (int i = 0; i < 3; i++) k.soundHole(-L * 0.25 + L * 0.25 * i, 0, res.r0 * 0.3, res.w1);
    // the strings rise from its middle line to the neck: their lengths the scale's, inverted
    std::vector<double> su(ns), sv(ns);
    for (int i = 0; i < ns; i++) {
        su[i] = -L * 0.4 + L * 0.8 * i / (ns - 1);
        const double len = H0 / std::sqrt(k.ratio(i));
        sv[i] = style == 1 ? H0 * (1 - 0.5 * i / (ns - 1)) : len;
    }
    // the neck through the strings' tops
    for (int i = 0; i + 1 < ns; i++) k.tubeTo(su[i], sv[i] + 0.012, su[i + 1], sv[i + 1] + 0.012, 0.011, 0.011, th * 0.5, M.mat2);
    k.tubeTo(su[0] - 0.03, 0, su[0], sv[0] + 0.012, 0.012, 0.011, th * 0.5, M.mat2);
    if (style == 2) k.tubeTo(su[ns - 1], sv[ns - 1] + 0.012, su[ns - 1] + 0.05, 0, 0.012, 0.014, th * 0.5, M.mat2);   // the pillar
    k.head(su[ns - 1], sv[ns - 1] + 0.012, 0.3, th * 0.5, 0.016, M.mat2);
    for (int i = 0; i < ns; i++) {
        k.str(su[i], 0, th + 0.002, su[i], sv[i] + 0.004, th * 0.5 + 0.01, M.str, i, 0.0008);
        k.disc(su[i], sv[i] + 0.012, 0.004, 0.004, th * 0.5 + 0.008, th * 0.5 + 0.016, M.fit);
    }
    k.bodyOrnament(res, 0);
}

void pBoard(Mk& k, bool bowed) {
    const InstrumentMake& M = k.M;
    const double L = k.rng.range(0.5, 0.9) * k.S, W = L * k.rng.range(0.2, 0.32), th = 0.045 * k.S;
    InstrumentPart& b = k.body(0, 0, 0, L, W * 0.5, 0, th, M.mat, 0.3);
    if (M.square < 3 && M.sides == 0) b.square = 4;   // a board is a board
    const int ns = clampi(bowed ? k.n + 2 : k.n + 4, 6, 16);
    const bool movable = M.variant % 2 == 1;
    k.box(-L * 0.42, 0, PI / 2, W * 0.86, 0.006, th, th + 0.008, M.fit);
    if (!movable) k.box(L * 0.38, 0, PI / 2, W * 0.86, 0.006, th, th + 0.008, M.fit);
    for (int i = 0; i < ns; i++) {
        const double v = -W * 0.4 + W * 0.8 * i / (ns - 1);
        k.str(-L * 0.42, v, th + 0.008, L * 0.44, v, th + 0.008, M.str, i, 0.0007);
        k.disc(L * 0.46, v, 0.004, 0.004, th, th + 0.01, M.fit);   // its pin
        if (movable) { const double u = L * 0.38 - L * 0.7 * (1 - 1 / k.ratio(i)); k.box(u, v, 0, 0.012, 0.004, th, th + 0.01, M.stick); }
    }
    k.soundHole(-L * 0.1, 0, W * 0.12, th);
    k.bodyOrnament(b, W * 0.2);
    if (bowed) k.bowBeside(-L * 0.45, -W * 0.9, 0, L * 0.7);
}

void pTubeZither(Mk& k) {
    const InstrumentMake& M = k.M;
    const double L = k.rng.range(0.5, 0.8) * k.S, R = k.rng.range(0.028, 0.045) * k.S;
    k.tube(-L / 2, 0, 0, L, R, R, R, M.mat);
    for (int e = 0; e < 2; e++) k.band(-L / 2 + (e ? L - 0.02 : 0.02), 0, 0, R * 1.04, R * 0.12, R, M.mat == IM_CANE ? IM_CANE : M.fit);
    const int ns = clampi(k.n + 2, 5, 12);
    for (int i = 0; i < ns; i++) {
        const double v = -R * 0.85 + R * 1.7 * i / (ns - 1), u0 = -L * 0.44, u1 = L * 0.44, bridge = -L * 0.3 + L * 0.2 * (i % 3) / 2.0;
        k.str(u0, v, 2 * R + 0.003, u1, v, 2 * R + 0.003, M.mat == IM_CANE && M.age == AGE_PREHISTORIC ? IM_CANE : M.str, i, 0.0009);
        k.box(bridge, v, 0, 0.01, 0.003, 2 * R - 0.003, 2 * R + 0.003, M.mat2);
    }
    k.mark(0, 0, R * 0.25, 2 * R, 1, IM_HOLE, -1, 0, L * 0.3);   // its slot
    k.tubeOrnament(-L / 2, 0, 0, L, R, R);
}

void pRingHarp(Mk& k) {
    const InstrumentMake& M = k.M;
    const double R = k.rng.range(0.18, 0.28) * k.S, rr = 0.012 * (0.7 + 0.3 * k.S);
    if (M.sides >= 5) for (int i = 0; i < M.sides; i++) { const double a0 = TAU * i / M.sides, a1 = TAU * (i + 1) / M.sides; k.tubeTo(std::cos(a0) * R, std::sin(a0) * R, std::cos(a1) * R, std::sin(a1) * R, rr, rr, rr, M.mat); }
    else k.ring(0, 0, R, rr, 0, M.mat);
    const double off = M.variant % 2 ? R * 0.35 : 0;   // a hub off the middle: its spokes graded
    const double hub = R * 0.18;
    k.ball(off, 0, hub, hub, 0, hub * 1.4, M.mat2);
    k.soundHole(off, 0, hub * 0.35, hub * 1.4);
    const int ns = clampi(k.n + 1, 5, 12);
    for (int i = 0; i < ns; i++) {
        const double a = TAU * (i + 0.5) / ns + (M.sym == SY_ROTATIONAL ? 0.25 : 0), eu = std::cos(a) * R * 0.98, ev = std::sin(a) * R * 0.98;
        const double du = eu - off, dv = ev, dl = std::hypot(du, dv);
        k.str(off + du / dl * hub, dv / dl * hub, hub * 1.2, eu, ev, rr * 1.6, M.str, i, 0.0008);
        k.disc(eu, ev, 0.0045, 0.0045, rr, rr * 2.4, M.fit);
    }
}

void pTongues(Mk& k) {
    const InstrumentMake& M = k.M;
    const bool vessel = M.variant % 2 == 1;
    const double L = k.rng.range(0.14, 0.22) * k.S, W = L * k.rng.range(0.8, 1.1), th = 0.03 * k.S;
    InstrumentPart& b = vessel ? k.ball(0, 0, W * 0.55, L * 0.58, 0, th * 2, M.mat) : k.body(0, 0, 0, L, W * 0.5, 0, th, M.mat, 0.3);
    if (vessel) k.body(0, 0, 0, L * 0.8, W * 0.42, th * 1.6, th * 1.7, M.stick, 0);
    const double fw = vessel ? th * 1.7 : b.w1;
    const int nt = clampi(k.n + 2, 5, 12);
    const int tongueMat = M.tongue;
    k.box(-L * 0.12, 0, PI / 2, W * 0.8, 0.006, fw, fw + 0.01, M.fit);   // the bridge
    for (int i = 0; i < nt; i++) {
        int slot = i;
        if (M.sym == SY_BILATERAL || M.sym == SY_ROTATIONAL) slot = i % 2 ? nt / 2 + (i + 1) / 2 : nt / 2 - i / 2;   // the longest in the middle, out in turn
        const double v = -W * 0.38 + W * 0.76 * slot / std::max(1, nt - 1), len = L * 0.55 / std::sqrt(k.ratio(i));
        k.box(-L * 0.12 + len * 0.5 - 0.02, v, 0, len, W * 0.022, fw + 0.008, fw + 0.011, tongueMat, i);
    }
    if (!vessel) k.soundHole(L * 0.3, 0, W * 0.07, fw);
    k.bodyOrnament(b, 0);
}

void pBow(Mk& k) {
    const InstrumentMake& M = k.M;
    const double L = k.rng.range(0.55, 1.0) * k.S, sag = L * k.rng.range(0.1, 0.18), R = (L * L / 4 + sag * sag) / (2 * sag), a = std::asin(std::min(1.0, L / 2 / R)), r = 0.008;
    k.arc(0, -R + sag, R, PI / 2 - a, PI / 2 + a, r * 0.8, r * 0.8, r, M.mat, 10);
    k.str(-L / 2, 0, r * 1.5, L / 2, 0, r * 1.5, M.str == IM_WIRE && M.age == AGE_PREHISTORIC ? IM_FIBRE : M.str, 0, 0.0008);
    for (int e = -1; e <= 1; e += 2) k.band(e * L / 2 * 0.98, 0, 0, r * 1.1, r * 0.5, r, IM_FIBRE);
    if (M.variant % 3 != 0) {   // its gourd tied on, a third along
        const double gu = -L * 0.2;
        const double gr = k.rng.range(0.05, 0.08) * k.S;
        k.ball(gu, sag * 0.6, gr, gr, 0, gr * 1.5, M.mat2 == M.mat ? M.vessel : M.mat2);
        k.mark(gu, sag * 0.6, gr * 0.45, gr * 1.5, 0);
        k.str(gu, 0, r * 1.5, gu, sag * 0.9, r * 2, IM_FIBRE, -1, 0.0012);
    }
    if (M.deco != ON_BARE) for (int e = -1; e <= 1; e += 2) for (int t = 0; t < 3; t++) k.str(e * L / 2, 0.005, r, e * (L / 2 + 0.03), -0.03 - 0.015 * t, 0.002, k.accentMat(AP_TOPS), -1, 0.0015);
}

void pBowHarp(Mk& k) {   // a box (or a gourd) and its bows rising from it, a string each (the people's number of them, fanned or in a row)
    const InstrumentMake& M = k.M;
    const double L = k.rng.range(0.16, 0.26) * k.S, W = L * k.rng.range(0.6, 0.85), th = 0.06 * k.S;
    const size_t bi = k.out.size();
    if (M.mat == IM_GOURD || M.mat == IM_SHELL || M.mat == IM_CLAY || M.mat == IM_PITH || M.mat == IM_CHITIN) k.ball(0, 0, W * 0.5, L * 0.5, 0, th * 1.4, M.mat);
    else k.body(0, 0, 0, L, W * 0.5, 0, th, M.mat, 0.4);
    const InstrumentPart b = k.out[bi];
    k.soundHole(-L * 0.1, 0, W * 0.1, k.onTop(b, -L * 0.1, 0));
    const int nb = clampi(M.number - 1, 3, 7);
    const bool fan = M.sym == SY_RADIAL || M.sym == SY_ROTATIONAL;
    for (int i = 0; i < nb; i++) {
        const double t = (i + 0.5) / nb, len = k.rng.range(0.22, 0.32) * k.S / std::sqrt(k.ratio(i) / k.ratio(0)), base = L * 0.4;
        const double a = fan ? -0.6 + 1.2 * t : 0.0, sv = fan ? 0 : -W * 0.38 + W * 0.76 * t;
        const double su = base, eu = su + std::cos(a) * len, ev = sv + std::sin(a) * len, mu = (su + eu) * 0.5 - std::sin(a) * len * 0.12, mv = (sv + ev) * 0.5 + std::cos(a) * len * 0.12;
        k.tubeTo(su, sv, mu, mv, 0.005, 0.004, th * 0.7, M.mat2); k.tubeTo(mu, mv, eu, ev, 0.004, 0.003, th * 0.7, M.mat2);   // its bow, bent
        k.str(-L * 0.25, sv * 0.5, k.onTop(b, -L * 0.25, sv * 0.5) + 0.004, eu, ev, th * 0.7 + 0.004, M.str, i, 0.0008);
    }
    k.box(-L * 0.25, 0, PI / 2, W * 0.8, 0.005, k.onTop(b, -L * 0.25, 0) - 0.001, k.onTop(b, -L * 0.25, 0) + 0.006, M.fit);   // the strings' bar
    k.bodyOrnament(b, L * 0.25);
}

void pJaw(Mk& k) {
    const InstrumentMake& M = k.M;
    const double L = k.rng.range(0.07, 0.13) * k.S, W = L * k.rng.range(0.28, 0.5);
    if (!instrumentMaterialMetal(M.mat)) {   // cut from one plate: its tongue a slit round
        const size_t bi = k.out.size();
        k.plate(0, 0, 0, L * 1.3, W * 0.5, 0, 0.004, M.mat, clampd(M.taper, 0.5, 1.2), clampd(M.waist, -0.2, 0.3), clampd(M.square, 1.6, 8), 0, M.sides, M.skew);
        const InstrumentPart b = k.out[bi];
        const int tongues = M.variant % 3 == 1 ? 2 : 1;
        for (int t = 0; t < tongues; t++) {
            const double v = tongues == 1 ? 0 : (t ? 1 : -1) * W * 0.2, hw = W * (tongues == 1 ? 0.14 : 0.08);
            k.mark(-L * 0.05, v + hw + 0.0012, 0.0009, 0.004, 1, IM_HOLE, -1, 0, L * 0.85);
            k.mark(-L * 0.05, v - hw - 0.0012, 0.0009, 0.004, 1, IM_HOLE, -1, 0, L * 0.85);
            k.mark(L * 0.38, v, 0.0009, 0.004, 1, IM_HOLE, -1, PI / 2, hw * 2 + 0.003);
            k.box(-L * 0.05, v, 0, L * 0.8, hw, 0.004, 0.005, M.mat, 0);
        }
        if (M.deco != ON_BARE) for (int i = 0; i < M.number; i++) { const double u = -L * 0.6 + L * 0.12 * (i % 3), v = (i / 3 % 2 ? 1 : -1) * W * 0.36; if (std::fabs(u) < L * 0.62) k.mark(u, v, W * 0.05, b.w1 + 0.0005, M.motif == MO_CIRCLES ? 0 : (M.motif == MO_STARS ? 6 : 3), M.painted > 0.3 ? IM_PAINT : IM_HOLE); }
        if (M.variant % 2) k.ring(-L * 0.62 - 0.012, 0, 0.012, 0.0012, 0, IM_FIBRE);   // a loop of cord to hold it
        else k.str(-L * 0.62, 0, 0.003, -L * 0.62 - 0.07, 0.015 * (M.skew >= 0 ? 1 : -1), 0.001, IM_FIBRE, -1, 0.001);
    } else {   // a metal loop and its tongue
        k.arc(-L * 0.1, 0, W * 0.5, PI * 0.15, PI * 1.85, 0.0025, 0.0025, 0.0025, M.mat, 10);
        k.tube(W * 0.5 * std::cos(PI * 0.15) - L * 0.1, W * 0.5 * std::sin(PI * 0.15), 0, L * 0.6, 0.0025, 0.0022, 0.0025, M.mat);
        k.tube(W * 0.5 * std::cos(PI * 0.15) - L * 0.1, -W * 0.5 * std::sin(PI * 0.15), 0, L * 0.6, 0.0025, 0.0022, 0.0025, M.mat);
        k.box(L * 0.1, 0, 0, L * 1.0, 0.0012, 0.003, 0.0045, M.tongue, 0);
        k.box(L * 0.63, 0, PI / 2, 0.008, 0.0016, 0.003, 0.009, M.tongue, 0);
    }
}

void pSpike(Mk& k) {
    const InstrumentMake& M = k.M;
    const double R = k.rng.range(0.05, 0.08) * k.S * (M.reg < 0 ? 1.5 : 1.0), nl = k.rng.range(0.45, 0.7) * k.S;
    const bool sq = M.sides >= 5 || M.square > 5;
    if (sq) k.disc(0, 0, R, R, 0, R * 1.2, M.mat, -1, M.sides >= 5 ? M.sides : 4); else k.ball(0, 0, R, R, 0, R * 1.4, M.mat);
    k.disc(0, 0, R * 0.85, R * 0.85, R * 1.2, R * 1.25, IM_HIDE, -1, sq ? (M.sides >= 5 ? M.sides : 4) : 0);   // its skin
    k.tube(-R * 1.6, 0, 0, nl + R * 1.6, 0.007, 0.006, R * 0.9, M.mat2);   // the spike through it
    k.head(nl, 0, 0, R * 0.6, 0.01, M.mat2);
    const int ns = clampi(M.number / 3 + 1, 2, 4);
    for (int i = 0; i < ns; i++) {
        k.box(nl - 0.04 - 0.035 * i, (i % 2 ? 1 : -1) * 0.018, PI / 2, 0.04, 0.003, R * 0.9, R * 0.9 + 0.006, M.fit);
        k.str(-R * 0.5, (i - (ns - 1) * 0.5) * 0.004, R * 1.3, nl - 0.04 - 0.035 * i, (i - (ns - 1) * 0.5) * 0.002, R * 0.9 + 0.008, M.str, i, 0.0008);
    }
    k.box(-R * 0.4, 0, PI / 2, 0.02, 0.003, R * 1.25, R * 1.3, M.fit);
    k.bowBeside(-R * 1.5, -R * 1.6, 0.05, nl * 0.95);
}

void pTubeFiddle(Mk& k) {
    const InstrumentMake& M = k.M;
    const double R = k.rng.range(0.04, 0.06) * k.S, bl = k.rng.range(0.1, 0.14) * k.S, nl = k.rng.range(0.4, 0.55) * k.S;
    k.tube(-bl, 0, 0, bl, R, R, R, M.mat);
    k.disc(-bl - 0.002, 0, R, R, 0, 2 * R, IM_HIDE);
    k.tube(-bl * 0.5, 0, 0, nl + bl * 0.5, 0.007, 0.006, R * 1.6, M.mat2);
    k.head(nl, 0, 0, R * 1.6, 0.01, M.mat2);
    for (int i = 0; i < 2; i++) { k.box(nl - 0.04 - 0.04 * i, (i ? 1 : -1) * 0.016, PI / 2, 0.036, 0.003, R * 1.6, R * 1.6 + 0.006, M.fit); k.str(-bl + 0.01, (i ? 1 : -1) * 0.003, 2 * R + 0.003, nl - 0.04 - 0.04 * i, 0, R * 1.6 + 0.008, M.str, i, 0.0008); }
    k.bowBeside(-bl, -R * 2.2, 0.05, nl);
}

void pWheel(Mk& k) {
    const InstrumentMake& M = k.M;
    const double L = k.rng.range(0.45, 0.6) * k.S, W = L * 0.36, th = 0.09 * k.S;
    InstrumentPart& b = k.body(0, 0, 0, L, W * 0.5, 0, th, M.mat, 0.2);
    b.waist = std::max(b.waist, 0.15);
    k.disc(-L * 0.3, 0, W * 0.3, W * 0.3, th, th + 0.004, M.stick);   // the wheel
    k.box(-L * 0.3, 0, 0, W * 0.75, W * 0.35, th, th + 0.03, M.mat == IM_GOURD ? M.stick : M.mat);   // its cover
    k.tube(-L * 0.5, 0, PI, 0.05, 0.005, 0.005, th * 0.6, M.fit); k.box(-L * 0.5 - 0.05, 0.02, PI / 2, 0.04, 0.006, th * 0.6, th * 0.6 + 0.01, M.fit); k.disc(-L * 0.5 - 0.05, 0.04, 0.008, 0.008, th * 0.6, th * 0.6 + 0.03, M.mat2);   // the crank
    const int keys = clampi(k.n + 3, 7, 13);
    k.box(L * 0.08, 0, 0, L * 0.5, W * 0.16, th, th + 0.035, M.stick);   // the key box
    for (int i = 0; i < keys; i++) k.box(L * 0.08 - L * 0.23 + L * 0.46 * i / (keys - 1), -W * 0.2, PI / 2, W * 0.12, 0.004, th + 0.01, th + 0.03, M.fit, i);
    for (int s = 0; s < 3; s++) k.str(-L * 0.42, -W * 0.25 + W * 0.25 * s, th + 0.04, L * 0.44, -W * 0.05 + W * 0.05 * s, th + 0.03, M.str, -1, 0.0007);
    k.pegs(L * 0.42, 0, 0, L * 0.12, 4, th, M.fit);
    k.head(L * 0.55, 0, 0, th * 0.5, 0.02, M.mat2);
}

void pHornFiddle(Mk& k) {
    const InstrumentMake& M = k.M;
    const double nl = k.rng.range(0.42, 0.55) * k.S;
    k.box(nl * 0.15, 0, 0, nl, 0.012, 0.01, 0.035, M.mat);
    k.head(nl * 0.65, 0, 0, 0.02, 0.012, M.mat);
    k.disc(-nl * 0.25, 0, 0.035, 0.035, 0, 0.05, M.mat2);   // the diaphragm
    k.bell(-nl * 0.25, 0.03, PI / 2 + 0.4, k.rng.range(0.14, 0.22) * k.S, 0.012, k.rng.range(0.06, 0.09) * k.S, 0.04, M.mat2);
    for (int i = 0; i < 4; i++) k.str(-nl * 0.32, (i - 1.5) * 0.004, 0.052, nl * 0.6, (i - 1.5) * 0.003, 0.036, M.str, i, 0.0007);
    k.pegs(nl * 0.55, 0, 0, nl * 0.12, 4, 0.035, M.fit);
    k.bowBeside(-nl * 0.4, -0.08, 0.05, nl * 1.0);
}

// ---- struck -----------------------------------------------------------------------------------------------------------------------

int struckCount(Mk& k) { return clampi(k.n + (k.M.number >= 7 ? 2 : 1), 5, 10); }

void pBars(Mk& k, bool glass) {
    const InstrumentMake& M = k.M;
    const int nb = struckCount(k);
    const double L0 = k.rng.range(0.16, 0.26) * k.S, bw = k.rng.range(0.025, 0.04) * k.S, gap = bw * 1.35, th = 0.012 + 0.008 * k.S;
    const double span = gap * nb, wr = 0.03;
    for (int e = -1; e <= 1; e += 2) k.box(0, e * L0 * 0.3, 0, span + 0.04, 0.012, 0, wr, M.mat2);
    if (M.variant % 3 == 1 && !glass) for (int i = 0; i < nb; i++) { const double u = -span * 0.5 + gap * (i + 0.5); k.tube(u, -L0 * 0.25, PI / 2, L0 * 0.9 / std::sqrt(k.ratio(i)) + 0.05, bw * 0.38, bw * 0.38, 0.012, M.mat2); }   // resonators under
    for (int i = 0; i < nb; i++) {
        const double u = -span * 0.5 + gap * (i + 0.5), len = L0 / std::sqrt(k.ratio(i));
        if (glass) k.tube(u, -len * 0.5, PI / 2, len, bw * 0.4, bw * 0.4, wr + bw * 0.4, M.mat, i);
        else if (M.mat == IM_STONE || M.mat == IM_OBSIDIAN || M.mat == IM_ICE) { InstrumentPart& p = k.plate(u, 0, PI / 2, len, bw * 0.5, wr, wr + th, M.mat, k.rng.range(0.85, 1.15), 0, k.rng.range(2.5, 6), 0, 0, 0); (void)p; }
        else k.box(u, 0, PI / 2, len, bw * 0.5, wr, wr + th, M.mat, i);
        if (!glass) { out_course(k, i); k.mark(u, len * 0.28, 0.002, wr + th, 0, IM_HOLE); k.mark(u, -len * 0.28, 0.002, wr + th, 0, IM_HOLE); }
    }
    k.lights(-span * 0.5, -L0 * 0.45, span, nb, 0.012, true);
    for (int b = 0; b < 2; b++) k.beater(span * 0.5 + 0.04 + 0.03 * b, -L0 * 0.3, PI / 2 + 0.1 * b, 0.25 * k.S, 0.014, M.stick, M.age >= AGE_INDUSTRIAL ? IM_FELT : (M.fit == IM_FIBRE ? IM_HIDE : M.fit));
}
void pTubes(Mk& k) {
    const InstrumentMake& M = k.M;
    const int nt = struckCount(k);
    const double L0 = k.rng.range(0.3, 0.5) * k.S, r = k.rng.range(0.008, 0.013) * k.S, gap = r * 3.2, span = gap * nt;
    // the frame lying: its top bar across, its two posts along
    k.box(0, 0, PI / 2, span + 0.06, 0.012, 0, 0.03, M.mat2);
    for (int e = -1; e <= 1; e += 2) k.box(L0 * 0.55, e * (span * 0.5 + 0.03), 0, L0 * 1.1, 0.012, 0, 0.03, M.mat2);
    for (int i = 0; i < nt; i++) {
        const double v = -span * 0.5 + gap * (i + 0.5), len = L0 / std::sqrt(k.ratio(i));
        k.str(0.005, v, 0.03, 0.03, v, r, IM_FIBRE, -1, 0.0008);
        k.tube(0.03, v, 0, len, r, r, r, M.mat, i);
        k.mark(0.033, v, r * 0.55, 2 * r, 0);
    }
    k.beater(L0 * 0.7, -span * 0.25, PI / 2, 0.22 * k.S, 0.012, M.mat2, M.fit);
}

void pBells(Mk& k) {
    const InstrumentMake& M = k.M;
    const int nb = clampi(k.n, 4, 8);
    const double R0 = k.rng.range(0.045, 0.07) * k.S;
    const bool ring = M.sym == SY_RADIAL || M.sym == SY_ROTATIONAL;
    double u = 0;
    for (int i = 0; i < nb; i++) {
        const double R = R0 / std::cbrt(k.ratio(i)), h = R * 1.4;
        double cu, cv, a;
        if (ring) { a = TAU * i / nb; cu = std::cos(a) * R0 * 2.2; cv = std::sin(a) * R0 * 2.2; }
        else { a = PI / 2 + (i % 2 ? 0.25 : -0.25); cu = u + R; cv = (i % 2) * R * 0.5; u += 2 * R + 0.012; }
        const double c = std::cos(a), s = std::sin(a);
        // a bell lying on its side: its crown toward `a`, its mouth away; its handle beyond the crown
        k.bell(cu + c * h * 0.5, cv + s * h * 0.5, a + PI, h, R * 0.45, R, R, M.mat);
        out_course(k, i);
        for (int q = 1; q <= 4; q++) if (k.out[k.out.size() - q].kind == IPK_TUBE) k.out[k.out.size() - q].course = i;
        k.tube(cu + c * h * 0.5, cv + s * h * 0.5, a, R * 1.1, R * 0.16, R * 0.14, R, M.stick);
        k.head(cu + c * (h * 0.5 + R * 1.1), cv + s * (h * 0.5 + R * 1.1), a, R * 0.8, R * 0.22, M.stick);
    }
}

void pBellTree(Mk& k) {
    const InstrumentMake& M = k.M;
    const int nb = clampi(k.n + 2, 6, 11);
    const double L = k.rng.range(0.35, 0.55) * k.S, R0 = k.rng.range(0.035, 0.05) * k.S;
    k.tube(-L * 0.55, 0, 0, L * 1.1, 0.006, 0.006, R0, M.stick);
    k.head(L * 0.55, 0, 0, R0 * 0.5, 0.012, M.mat2);
    for (int i = 0; i < nb; i++) {
        const double R = R0 * (1 - 0.06 * i) / std::cbrt(k.ratio(i) / k.ratio(0)), u = -L * 0.45 + L * 0.9 * i / (nb - 1);
        k.bell(u + R * 0.5, 0, PI, R * 0.9, R * 0.35, R, R0, M.mat);
        for (int q = 1; q <= 4; q++) k.out[k.out.size() - q].course = i;
    }
}

void pBowls(Mk& k, bool glass) {
    const InstrumentMake& M = k.M;
    const int nb = clampi(k.n, 4, 9);
    const double R0 = k.rng.range(0.06, 0.1) * k.S;
    const bool ring = M.sym == SY_RADIAL;
    double u = 0;
    if (glass) {   // on an axle, nested, in a trough: a wheel to turn them
        const double L = R0 * nb * 0.75;
        k.box(L * 0.4, 0, 0, L * 1.3, R0 * 1.2, 0, R0 * 0.8, M.mat2);
        k.tube(-L * 0.3, 0, 0, L * 1.4, 0.004, 0.004, R0 * 0.9, IM_STEEL);
        for (int i = 0; i < nb; i++) { const double R = R0 * (1 - 0.6 * i / nb), x = -L * 0.2 + L * i / nb; k.tube(x, 0, 0, L / nb * 0.9, R, R * 0.95, R0 * 0.9, M.mat, i); }
        k.disc(-L * 0.35, 0, R0 * 0.9, R0 * 0.9, 0, 0.012, M.fit);
        return;
    }
    for (int i = 0; i < nb; i++) {
        const double R = R0 / std::sqrt(k.ratio(i)), h = R * k.rng.range(0.6, 0.8);
        double cu, cv;
        if (ring) { const double a = TAU * i / nb; cu = std::cos(a) * R0 * 2.4; cv = std::sin(a) * R0 * 2.4; }
        else { cu = u + R; cv = M.sym == SY_NONE ? k.rng.sym(R0 * 0.4) : 0; u += 2 * R + 0.015; }
        k.bowl(cu, cv, R, 0, h, M.mat, 1.35, i);
        if (M.deco == ON_BANDED) k.ring(cu, cv, R * 0.98, 0.0025, h - 0.003, k.accentMat(AP_BANDS));
        k.ring(cu, cv, R * 0.6, 0.004, -0.002, M.fit == IM_LIGHT ? IM_FIBRE : M.fit);   // its cushion
    }
    k.beater(u * 0.5, -R0 * 1.6, 0, 0.2 * k.S, 0.012, M.stick, M.age >= AGE_INDUSTRIAL ? IM_FELT : IM_HIDE);
}

void pGongs(Mk& k) {
    const InstrumentMake& M = k.M;
    if (M.variant % 2) {   // one gong in its frame
        const double R = k.rng.range(0.16, 0.3) * k.S;
        k.disc(0, 0, R, R, 0, 0.012, M.mat, 0, M.sides >= 5 ? M.sides : 0);
        k.ring(0, 0, R * 0.98, 0.006, 0.006, M.mat);
        k.ball(0, 0, R * 0.2, R * 0.2, 0.008, R * 0.12, M.mat, 0, 0);
        if (M.deco >= ON_CARVED) for (int i = 0; i < M.number; i++) { const double a = TAU * i / M.number; k.mark(std::cos(a) * R * 0.6, std::sin(a) * R * 0.6, R * 0.05, 0.012, M.motif == MO_STARS ? 6 : 0, k.trimMat()); }
        k.boxTo(-R * 1.25, -R * 1.2, -R * 1.25, R * 1.2, 0.014, 0, 0.03, M.mat2);
        k.boxTo(R * 1.25, -R * 1.2, R * 1.25, R * 1.2, 0.014, 0, 0.03, M.mat2);
        k.boxTo(-R * 1.25, R * 1.2, R * 1.25, R * 1.2, 0.014, 0, 0.03, M.mat2);
        k.str(0, R * 1.2, 0.03, 0, R * 0.95, 0.01, IM_FIBRE, -1, 0.001);
        k.beater(R * 0.6, -R * 1.5, 0.3, 0.3 * k.S, 0.025, M.mat2, IM_FELT == M.fit ? IM_FELT : IM_HIDE);
        return;
    }
    const int ng = clampi(k.n, 4, 9);
    const double R0 = k.rng.range(0.055, 0.09) * k.S;
    if (M.sym == SY_RADIAL || M.sym == SY_ROTATIONAL) {   // a ring of gongs in a round frame, a boss in the middle
        const double RR = R0 * 2.6;
        k.ring(0, 0, RR + R0 * 1.15, 0.008, 0, M.mat2);
        for (int i = 0; i < ng; i++) {
            const double a = TAU * i / ng + (M.sym == SY_ROTATIONAL ? 0.15 * i : 0), R = R0 / std::cbrt(k.ratio(i)), d = RR * (M.sym == SY_ROTATIONAL ? 0.8 + 0.25 * i / ng : 1.0);
            k.disc(std::cos(a) * d, std::sin(a) * d, R, R, 0.01, 0.01 + R * 0.4, M.mat, i);
            k.ball(std::cos(a) * d, std::sin(a) * d, R * 0.3, R * 0.3, 0.01 + R * 0.4, R * 0.22, M.mat, 0, i);
        }
        k.disc(0, 0, R0 * 0.6, R0 * 0.6, 0, 0.03, M.mat2, -1, M.sides);
        return;
    }
    const bool two = M.sym == SY_NONE || ng >= 8;   // two rows staggered
    const int perRow = two ? (ng + 1) / 2 : ng;
    double rowLen = 0; for (int i = 0; i < perRow; i++) rowLen += 2 * R0 / std::cbrt(k.ratio(i)) + 0.012;
    for (int r = 0; r < (two ? 2 : 1); r++) for (int e = -1; e <= 1; e += 2) k.box(rowLen * 0.5, (r - (two ? 0.5 : 0)) * R0 * 2.4 + e * R0 * 0.75, 0, rowLen + 0.06, 0.01, 0, 0.025, M.mat2);
    double u[2] = {0, R0 * (two ? 0.9 : 0)};
    for (int i = 0; i < ng; i++) {
        const int r = two ? i % 2 : 0;
        const double R = R0 / std::cbrt(k.ratio(i)), v = (r - (two ? 0.5 : 0)) * R0 * 2.4;
        k.disc(u[r] + R, v, R, R, 0.01, R * 0.5, M.mat, i);
        k.ball(u[r] + R, v, R * (0.22 + 0.1 * (M.variant % 3)), R * (0.22 + 0.1 * (M.variant % 3)), R * 0.5, R * 0.25, M.mat, 0, i);
        if (M.deco == ON_BANDED) k.ring(u[r] + R, v, R * 0.75, 0.002, R * 0.5, k.accentMat(AP_BANDS));
        u[r] += 2 * R + 0.012;
    }
    k.beater(rowLen * 0.5, -R0 * 3.2, 0.1, 0.24 * k.S, 0.014, M.stick, M.age >= AGE_INDUSTRIAL ? IM_FELT : IM_HIDE);
}

void pStones(Mk& k) {
    const InstrumentMake& M = k.M;
    const int ns = struckCount(k);
    const double L0 = k.rng.range(0.22, 0.36) * k.S, bw = k.rng.range(0.04, 0.06) * k.S, gap = bw * 1.25;
    const int railMat = M.age == AGE_PREHISTORIC && k.rng.chance(0.5) ? IM_FIBRE : M.mat2;
    for (int e = -1; e <= 1; e += 2) { if (railMat == IM_FIBRE) k.str(-gap * ns * 0.55, e * L0 * 0.25, 0.006, gap * ns * 0.55, e * L0 * 0.25, 0.006, IM_FIBRE, -1, 0.002); else k.box(0, e * L0 * 0.25, 0, gap * ns + 0.04, 0.012, 0, 0.025, railMat); }
    for (int i = 0; i < ns; i++) {
        const double u = -gap * ns * 0.5 + gap * (i + 0.5), len = L0 / std::sqrt(k.ratio(i));
        InstrumentPart& p = k.plate(u + k.rng.sym(bw * 0.1), k.rng.sym(len * 0.06), PI / 2 + k.rng.sym(0.12), len, bw * 0.5 * k.rng.range(0.8, 1.2), 0.01, 0.01 + bw * 0.6, M.mat, k.rng.range(0.6, 1.3), k.rng.range(-0.1, 0.15), k.rng.range(1.4, 3.5), bw * 0.15, 0, k.rng.irange(5) - 2);
        p.course = i;
    }
    k.ball(gap * ns * 0.5 + 0.06, 0, 0.03 * k.S, 0.036 * k.S, 0, 0.04, M.mat);   // the striking stone
}

void pSpokes(Mk& k) {
    const InstrumentMake& M = k.M;
    const int nb = struckCount(k);
    const double R = k.rng.range(0.2, 0.3) * k.S, bw = 0.022 * k.S;
    if (M.sides >= 5) for (int i = 0; i < M.sides; i++) { const double a0 = TAU * i / M.sides, a1 = TAU * (i + 1) / M.sides; k.tubeTo(std::cos(a0) * R, std::sin(a0) * R, std::cos(a1) * R, std::sin(a1) * R, 0.012, 0.012, 0.012, M.mat2); }
    else k.ring(0, 0, R, 0.012, 0, M.mat2);
    k.disc(0, 0, R * 0.15, R * 0.15, 0, 0.03, M.mat2, -1, M.sides);
    for (int i = 0; i < nb; i++) {
        const double a = TAU * i / nb + (M.sym == SY_ROTATIONAL ? 0.2 : 0), len = R * 0.8 / std::sqrt(k.ratio(i));
        k.box(std::cos(a) * (R - len * 0.5), std::sin(a) * (R - len * 0.5), a, len, bw * 0.5, 0.024, 0.036, M.mat, i);
    }
}

// ---- whirled and the hum ------------------------------------------------------------------------------------------------------------

void pBlade(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    const double L = k.rng.range(0.14, 0.3) * k.S, W = L * k.rng.range(0.24, 0.5), cord = k.rng.range(0.1, 0.18) * k.S;
    if (plan == IP_BUZZER) {
        const double R = k.rng.range(0.04, 0.07) * k.S;
        k.disc(0, 0, R, R, 0, 0.006, M.mat, 0, M.sides >= 5 ? M.sides : (M.square > 5 ? 4 : 0));
        const double reach = R + cord * 0.6;
        for (int e = -1; e <= 1; e += 2) { k.mark(0, e * R * 0.15, R * 0.06, 0.006, 0); k.str(0, e * R * 0.15, 0.006, -reach, e * 0.012, 0.002, M.mat2, -1, 0.001); k.str(0, e * R * 0.15, 0.006, reach, e * 0.012, 0.002, M.mat2, -1, 0.001); }
        for (int e = -1; e <= 1; e += 2) k.tube(e > 0 ? reach : -reach - 0.05, 0, 0, 0.05, 0.006, 0.006, 0.006, M.stick);
        if (M.deco != ON_BARE) for (int i = 0; i < M.number; i++) { const double a = TAU * i / M.number; k.mark(std::cos(a) * R * 0.6, std::sin(a) * R * 0.6, R * 0.08, 0.006, M.motif == MO_STARS ? 6 : 3, k.trimMat()); }
        return;
    }
    if (plan == IP_WHIRL_TUBE) {
        const double TL = k.rng.range(0.38, 0.6) * k.S, R = k.rng.range(0.016, 0.024) * k.S;
        k.tube(-TL / 2, 0, 0, TL, R, R, R, M.mat, 0);
        const int ridges = 18 + k.rng.irange(12);
        for (int i = 0; i < ridges; i++) k.band(-TL / 2 + TL * (i + 0.5) / ridges, 0, 0, R * 1.02, R * 0.12, R, M.mat);
        k.disc(-TL / 2, 0, R * 1.1, R * 1.1, 0, R * 2, M.mat2);
        return;
    }
    // a blade (the people's outline, sharpened to a leaf, a lozenge, a fish) on its cord, its handle at the cord's end
    static const double SHAPES[5][3] = {{0.35, 0.0, 1.6}, {1.0, 0.0, 1.15}, {0.9, 0.35, 2.4}, {0.75, -0.15, 2.0}, {1.0, 0.0, 4.0}};   // a leaf, a lozenge, a fish, an egg, a slab
    const double* sh = SHAPES[(M.variant + (M.sides >= 5 ? 1 : 0)) % 5];
    InstrumentPart& b = k.plate(0, 0, 0, L, W * 0.5, 0, 0.006, M.mat, sh[0], sh[1], M.sides >= 5 ? 2 : sh[2], 0.002, (M.variant % 5 == 4 && M.sides >= 5) ? M.sides : 0, M.skew);
    b.course = 0;
    k.mark(-L * 0.43, 0, W * 0.07, 0.006, 0);
    if (M.variant % 4 == 2) for (int i = 0; i < 8; i++) for (int e = -1; e <= 1; e += 2) k.mark(-L * 0.35 + L * 0.7 * i / 7, e * W * 0.42, W * 0.06, 0.006, 3, IM_HOLE, -1, e > 0 ? -PI / 2 : PI / 2);   // notched edges
    if (M.deco >= ON_CARVED || M.motif == MO_FIGURES || M.motif == MO_GLYPHS) for (int i = 0; i < M.number; i++) k.mark(-L * 0.25 + L * 0.5 * i / std::max(1, M.number - 1), 0, W * 0.07, 0.006, M.motif == MO_CIRCLES ? 0 : (M.motif == MO_STARS ? 6 : 3), M.painted > 0.3 ? IM_PAINT : IM_HOLE);
    {   // the cord lying in a loose bend to its handle
        const double bend = (M.variant % 2 ? 1 : -1) * cord * 0.25;
        k.str(-L * 0.45, 0, 0.004, -L * 0.45 - cord * 0.5, bend, 0.003, M.mat2, -1, 0.0012);
        k.str(-L * 0.45 - cord * 0.5, bend, 0.003, -L * 0.45 - cord, 0.0, 0.006, M.mat2, -1, 0.0012);
    }
    k.tube(-L * 0.45 - cord - 0.09, 0, 0, 0.09, 0.008, 0.008, 0.008, M.stick);
}

void pHum(Mk& k, bool ring) {
    const InstrumentMake& M = k.M;
    const double R = k.rng.range(0.12, 0.2) * k.S;
    const int sides = M.sides >= 5 ? M.sides : 0, grooves = 1 + M.number % 3;
    if (ring) {   // a ring of the people's section and outline, its grooves of light, a core or none, marks round it by the motif
        const double sec = R * k.rng.range(0.14, 0.34);
        if (sides) for (int i = 0; i < sides; i++) { const double a0 = TAU * i / sides, a1 = TAU * (i + 1) / sides; k.tubeTo(std::cos(a0) * R, std::sin(a0) * R, std::cos(a1) * R, std::sin(a1) * R, sec, sec, sec, M.mat, 0); }
        else k.ring(0, 0, R, sec, 0, M.mat, 0);
        for (int g = 0; g < grooves; g++) { const double rr = R + sec * (grooves == 1 ? 0 : -0.5 + 1.0 * g / (grooves - 1)) * 0.8; k.ring(0, 0, rr, R * 0.018, sec * 1.9, IM_LIGHT, 0); }
        const int marks = M.motif == MO_LINES ? 0 : M.number;
        for (int i = 0; i < marks; i++) { const double a = TAU * i / std::max(1, marks); k.mark(std::cos(a) * (R + sec * 1.4), std::sin(a) * (R + sec * 1.4), R * 0.035, 0.002, M.motif == MO_STARS ? 6 : (M.motif == MO_STEPS ? 2 : 0), IM_LIGHT); }
        switch (M.variant % 3) {
            case 0: k.ball(0, 0, R * 0.3, R * 0.3, 0, R * 0.45, M.mat2); break;
            case 1: k.disc(0, 0, R * 0.45, R * 0.45, 0, R * 0.08, M.mat2, -1, sides); k.mark(0, 0, R * 0.12, R * 0.08, M.hole, IM_LIGHT); break;
            default: break;   // nothing in its middle
        }
        return;
    }
    // an orb in a cradle (a ring, three prongs, a polygon's dish), its seams of light by the motif
    const double orb = R * k.rng.range(0.65, 0.9);
    switch (M.variant % 3) {
        case 0: k.ring(0, 0, orb * 1.18, R * 0.08, 0, M.mat2); break;
        case 1: for (int i = 0; i < 3; i++) { const double a = TAU * i / 3 + 0.3; k.tubeTo(std::cos(a) * orb * 1.35, std::sin(a) * orb * 1.35, std::cos(a) * orb * 0.75, std::sin(a) * orb * 0.75, R * 0.05, R * 0.03, R * 0.1, M.mat2); } break;
        default: k.disc(0, 0, orb * 1.15, orb * 1.15, 0, R * 0.1, M.mat2, -1, sides ? sides : 6); break;
    }
    const size_t oi = k.out.size();
    k.ball(0, 0, orb, orb * (M.slender > 0.6 ? 1.25 : 1.0), R * 0.05, orb * 1.5, M.mat, 0, 0);
    const InstrumentPart o = k.out[oi];
    if (M.motif == MO_LINES || M.motif == MO_STEPS) for (int g = 0; g < grooves + 1; g++) { const double v = -orb * 0.5 + orb * g / std::max(1, grooves); k.box(0, v, 0, orb * 1.6 * std::sqrt(std::max(0.05, 1 - (v / orb) * (v / orb))), R * 0.012, k.surfaceAt(o, 0, v), k.surfaceAt(o, 0, v) + 0.002, IM_LIGHT, 0); }
    else for (int g = 0; g < grooves; g++) k.ring(0, 0, orb * (0.3 + 0.25 * g), R * 0.014, k.surfaceAt(o, orb * (0.3 + 0.25 * g), 0), IM_LIGHT, 0);
}

// ---- the comb's music box ---------------------------------------------------------------------------------------------------------

void pTuneBox(Mk& k, bool disc) {
    const InstrumentMake& M = k.M;
    const double L = k.rng.range(0.14, 0.22) * k.S, W = L * k.rng.range(0.6, 0.8), H = 0.07 * k.S;
    const int form = M.variant % 4;
    if (disc) {
        k.body(0, 0, 0, L * 1.3, L * 0.65, 0, H, M.mat, 0);
        k.disc(-L * 0.1, 0, L * 0.5, L * 0.5, H, H + 0.003, M.mat2);
        for (int i = 0; i < 60; i++) { const double a = i * 0.7, rr = L * (0.12 + 0.36 * i / 60); k.mark(-L * 0.1 + std::cos(a) * rr, std::sin(a) * rr, 0.0018, H + 0.003, 0); }
        k.disc(-L * 0.1, 0, L * 0.06, L * 0.06, H + 0.003, H + 0.01, M.mat2);
        const int teeth = clampi(k.n + 6, 10, 18);
        for (int i = 0; i < teeth; i++) k.box(L * 0.45, -W * 0.3 + W * 0.6 * i / (teeth - 1), 0, L * 0.18 / std::sqrt(k.ratio(i) / k.ratio(0) + 0.3), 0.002, H, H + 0.004, M.mat2, i);
        k.tube(L * 0.65, -W * 0.3, 0, 0.04, 0.004, 0.004, H * 0.5, M.mat2); k.disc(L * 0.69, -W * 0.3, 0.009, 0.009, 0, H * 0.7, M.fit);
        return;
    }
    switch (form) {
        case 1: k.disc(0, 0, W * 0.6, W * 0.6, 0, H, M.mat, -1, 0); break;
        case 2: k.ball(0, 0, W * 0.6, W * 0.6, 0, H * 1.2, M.mat); break;
        case 3: k.disc(0, 0, W * 0.62, W * 0.62, 0, H, M.mat, -1, M.sides >= 5 ? M.sides : 6); break;
        default: { InstrumentPart& b = k.body(0, 0, 0, L, W * 0.5, 0, H, M.mat, 0); b.square = std::max(b.square, 5.0); b.taper = 1; b.waist = 0; break; }
    }
    // the lid open beside it, its inside painted or inlaid
    InstrumentPart& lid = k.body(0, W * 1.1, 0, L, W * 0.5, 0, 0.008, M.mat, 0);
    lid.square = form == 0 ? std::max(lid.square, 5.0) : 2; lid.taper = 1; lid.waist = 0; lid.sides = form == 3 ? (M.sides >= 5 ? M.sides : 6) : 0;
    if (form != 0) { lid.len = W * 1.2; lid.r0 = W * 0.6; }
    k.body(0, W * 1.1, 0, lid.len * 0.75, lid.r0 * 0.75, 0.008, 0.009, M.painted > 0.3 ? IM_PAINT : k.trimMat(), 0).sides = lid.sides;
    // the comb and its pinned drum
    const int teeth = clampi(k.n + 6, 10, 18);
    const double cw = W * 0.75, cz = H + 0.002;
    k.box(-L * 0.05, 0, PI / 2, cw, 0.012, cz, cz + 0.004, M.mat2);
    for (int i = 0; i < teeth; i++) { const double v = -cw * 0.5 + cw * i / (teeth - 1); k.box(-L * 0.05 + 0.022, v, 0, 0.03 * std::pow(k.ratio(0) / k.ratio(i), 0.35), cw / teeth * 0.36, cz, cz + 0.003, M.mat2, i); }
    k.tube(L * 0.12, -cw * 0.55, PI / 2, cw * 1.1, 0.013, 0.013, cz + 0.008, M.mat2);
    for (int i = 0; i < 36; i++) k.mark(L * 0.12 + k.rng.sym(0.01), -cw * 0.5 + cw * k.rng.uni(), 0.0012, cz + 0.021, 0, M.fit);
    k.tube(0, -W * 0.55, -PI / 2, 0.035, 0.004, 0.004, H * 0.5, M.mat2); k.box(0.012, -W * 0.55 - 0.035, 0, 0.03, 0.005, H * 0.45, H * 0.55, M.mat2);   // its key
}

// ---- electric and the machines ------------------------------------------------------------------------------------------------------

void pElectricString(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    const bool bass = M.reg < 0;
    const double bl = k.rng.range(0.3, 0.4) * std::min(1.1, k.S), bw = bl * k.rng.range(0.6, 0.8) * (1.15 - 0.3 * M.slender), th = 0.04;
    const double nl = (bass ? k.rng.range(0.46, 0.56) : k.rng.range(0.38, 0.48)) * std::min(1.1, k.S);
    const int ns = bass ? clampi(M.number / 2 + 2, 4, 5) : clampi(M.number, 5, 8);
    if (plan == IP_LAP) {
        const double L = k.rng.range(0.6, 0.8) * k.S, W = 0.11 * k.S;
        InstrumentPart& b = k.body(0, 0, 0, L, W * 0.5, 0, th, M.mat, 0); b.square = std::max(b.square, 4.0); b.taper = std::max(0.85, b.taper);
        for (int i = 0; i < 18; i++) k.box(-L * 0.35 + L * 0.7 * (1 - 1 / std::pow(2.0, i / 12.0)) * 1.4, 0, PI / 2, W * 0.8, 0.0008, th, th + 0.001, k.trimMat());
        k.box(L * 0.25, 0, PI / 2, W * 0.7, 0.012, th, th + 0.008, IM_STEEL);
        for (int i = 0; i < ns + 1; i++) k.str(-L * 0.45, -W * 0.3 + W * 0.6 * i / ns, th + 0.009, L * 0.4, -W * 0.3 + W * 0.6 * i / ns, th + 0.009, IM_WIRE, i, 0.0006);
        k.knobs(L * 0.33, W * 0.3, 0.06, 2, 0.008, th, M.mat2);
        k.tube(-L * 0.1, W * 0.6, 0, 0.09, 0.008, 0.008, 0.008, IM_STEEL);   // the steel bar beside it
        return;
    }
    InstrumentPart* body = nullptr;
    if (plan == IP_SKELETON) {
        k.box(-bl * 0.1, 0, 0, bl * 1.1, 0.02, 0, th, M.mat);
        Form f = instrumentPlateForm(InstrumentPart());
        double a[24], b[24];
        InstrumentPart outl; outl.kind = IPK_PLATE; outl.len = bl; outl.r0 = bw * 0.5; outl.taper = M.taper; outl.waist = M.waist; outl.square = M.square; outl.sides = M.sides; outl.skew = M.skew;
        f = instrumentPlateForm(outl); formPlateOutline(f, 24, a, b);
        for (int i = 0; i < 24; i += 2) { const int j = (i + 2) % 24; k.tubeTo(b[i] - bl * 0.1, a[i], b[j] - bl * 0.1, a[j], 0.007, 0.007, 0.012, M.mat2); }
        for (int e = -1; e <= 1; e += 2) k.tubeTo(-bl * 0.1, 0, -bl * 0.1 + bl * 0.25, e * bw * 0.35, 0.005, 0.005, 0.012, M.mat2);   // its struts
    } else {
        body = &k.body(-bl * 0.1, 0, 0, bl, bw * 0.5, 0, th, M.mat, 0.1);
        if (M.variant % 3 == 0) { InstrumentPart g = *body; g.mat = M.mat2; g.r0 *= 0.55; g.len *= 0.5; g.u -= bl * 0.08; g.v -= bw * 0.12; g.w0 = th; g.w1 = th + 0.002; g.dome = 0; k.out.push_back(g); }   // its guard
    }
    const double n0 = bl * 0.3, n1 = n0 + nl, nw = 0.022 + 0.002 * ns;
    k.box((n0 + n1) * 0.5, 0, 0, nl, nw * 0.5, th * 0.4, th + 0.004, M.mat2 == M.mat ? M.stick : M.mat2);
    k.frets(n1, 0, (n1 + bl * 0.42) * 1.0, n0, nw * 0.5, th + 0.004);
    for (int i = 0; i < 6; i++) { const double u = n1 - (n1 + bl * 0.42) * (1 - 1 / std::pow(2.0, (3 + 2 * i) / 12.0)); if (u > n0) k.mark(u, 0, 0.0025, th + 0.005, M.motif == MO_STARS ? 6 : (M.motif == MO_STEPS ? 2 : 0), k.trimMat()); }
    if (M.variant % 4 == 1) k.box(n1 + 0.02, 0, 0, 0.04, nw * 0.6, th * 0.4, th, M.mat2);   // headless: its block
    else {
        const double hl = 0.13 * std::min(1.1, k.S), ha = M.variant % 4 == 2 ? 0.2 : 0;
        InstrumentPart& hd = k.plate(n1 + hl * 0.5, std::sin(ha) * hl * 0.5, ha, hl, nw * 0.9, th * 0.4, th * 0.4 + 0.012, M.mat2 == M.mat ? M.stick : M.mat2, M.taper, 0, M.square, 0, M.sides, M.skew);
        (void)hd;
        for (int i = 0; i < ns; i++) { const double t = (i + 0.5) / ns, pu = n1 + hl * t, side = (M.variant % 2 ? 1 : (i < ns / 2 ? -1 : 1)); k.disc(pu, side * nw * 1.05, 0.005, 0.005, th * 0.4, th * 0.4 + 0.02, IM_STEEL); }
    }
    const int pickups = 1 + M.variant % 3;
    for (int p = 0; p < pickups; p++) k.box(-bl * 0.02 + 0.045 * p, 0, PI / 2, nw * 1.8, 0.008, th, th + 0.006, p == 1 && M.age >= AGE_FUTURISTIC ? IM_LIGHT : IM_PLASTIC == M.mat ? IM_STEEL : IM_PLASTIC);
    k.box(-bl * 0.3, 0, PI / 2, nw * 1.6, 0.007, th, th + 0.008, IM_STEEL);   // the bridge
    k.knobs(-bl * 0.42, bw * 0.28, 0.07, clampi(M.number / 2, 2, 4), 0.008, th, IM_STEEL);
    for (int i = 0; i < ns; i++) { const double v0 = (i - (ns - 1) * 0.5) * nw * 0.8 / std::max(1, ns - 1) * 1.1, v1 = (i - (ns - 1) * 0.5) * nw * 0.75 / std::max(1, ns - 1); k.str(-bl * 0.3, v0, th + 0.01, n1, v1, th + 0.006, IM_WIRE, ns - 1 - i, bass ? 0.001 : 0.0006); }
    if (body) k.bodyOrnament(*body, bl * 0.4);
}

void pKeys(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    const int keys = clampi(2 * k.n, 10, 18);
    const int keyMat = M.age >= AGE_FUTURISTIC ? (M.light == LT_NONE ? IM_GLASS : IM_LIGHT) : IM_CERAMIC;
    const int darkMat = keyMat == IM_BONE ? IM_DARKWOOD : (M.age >= AGE_FUTURISTIC ? IM_VOID : IM_LACQUER);
    if (plan == IP_KEY_RING) {
        const double R = k.rng.range(0.16, 0.24) * k.S;
        k.disc(0, 0, R, R, 0, 0.05, M.mat, -1, M.sides >= 5 ? M.sides : 0);
        for (int i = 0; i < keys; i++) { const double a = TAU * i / keys; k.box(std::cos(a) * R * 0.75, std::sin(a) * R * 0.75, a, R * 0.38, R * 0.1, 0.05, 0.06, keyMat, i); }
        k.disc(0, 0, R * 0.32, R * 0.32, 0.05, 0.058, M.mat2); k.knobs(-R * 0.18, 0, R * 0.36, 3, 0.008, 0.058, IM_STEEL);
        k.lights(-R * 0.25, R * 0.15, R * 0.5, 5, 0.058, false);
        return;
    }
    if (plan == IP_KEY_ARC) {
        const double R = k.rng.range(0.32, 0.5) * k.S, span = k.rng.range(0.9, 1.4);
        k.arc(0, -R, R * 1.12, PI / 2 - span / 2 - 0.08, PI / 2 + span / 2 + 0.08, 0.035, 0.035, 0.035, M.mat, 10, R * 1.12);
        for (int i = 0; i < keys; i++) { const double a = PI / 2 + span / 2 - span * (i + 0.5) / keys; k.box(std::cos(a) * R, -R + std::sin(a) * R, a, R * 0.16, R * span / keys * 0.42, 0.04, 0.05, keyMat, i); }
        k.lights(-R * 0.4, R * 0.12 - R * 0.1, R * 0.8, 7, 0.05, false);
        return;
    }
    const double L = k.rng.range(0.55, 0.9) * k.S, D = k.rng.range(0.2, 0.3) * k.S, th = 0.06;
    InstrumentPart& b = k.body(0, 0, 0, L, D * 0.5, 0, th, M.mat, 0.2);
    if (b.square < 3) b.square = 4.5;
    k.keyboard(-L * 0.42, -D * 0.12, L * 0.84, D * 0.5, keys, th, keyMat, darkMat);
    if (M.variant % 3 == 0) k.knobs(-L * 0.38, D * 0.32, L * 0.5, clampi(M.number, 4, 9), 0.008, th, IM_STEEL);
    else for (int i = 0; i < 2 * M.number; i++) k.box(-L * 0.38 + L * 0.03 * i, D * 0.32, PI / 2, 0.04, 0.005, th, th + 0.01, i % 3 == 0 ? IM_PAINT : M.mat2);   // drawbars
    if (M.variant % 2) for (int i = 0; i < 12; i++) k.mark(L * 0.25 + 0.012 * (i % 6), D * 0.25 + 0.012 * (i / 6), 0.003, th, 0);   // its speaker's grille
    k.lights(L * 0.05, D * 0.4, L * 0.3, 6, th, false);
}

void pGlass(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    if (plan == IP_GLASS_BOWLS) { pBowls(k, true); return; }
    if (plan == IP_GLASS_BARS) { pBars(k, true); return; }
    const int nr = clampi(k.n + 2, 6, 12);
    const double L0 = k.rng.range(0.22, 0.36) * k.S, r = 0.006 * k.S * k.rng.range(0.8, 1.4), gap = r * 3.5;
    const int form = M.sym == SY_RADIAL || M.sym == SY_ROTATIONAL ? (M.variant % 2 ? 0 : 2) : (M.variant % 2 ? 1 : 3);   // a fan, a row on its base, a grove round a hub, rods across a frame
    const int tip = M.variant / 2 % 3;   // a bead, a point, a ring at each rod's end
    auto rodEnd = [&](double u, double v, double a, double w, int i) {
        if (tip == 0) k.ball(u, v, r * 1.4, r * 1.4, w - r, r * 2.4, M.mat, 0, i);
        else if (tip == 2) k.ring(u, v, r * 1.6, r * 0.4, w - r * 0.4, k.trimMat(), i);
        (void)a;
    };
    if (form == 0 || form == 2) {
        const double hub = form == 0 ? 0.05 : 0.035;
        k.disc(0, 0, hub, hub, 0, 0.02, M.mat2, -1, M.sides);
        for (int i = 0; i < nr; i++) {
            const double a = form == 0 ? PI * (0.1 + 0.8 * i / (nr - 1)) : TAU * i / nr + (M.sym == SY_ROTATIONAL ? 0.4 : 0), len = L0 / std::sqrt(k.ratio(i)) * (form == 2 ? 0.6 : 1.0);
            const double tw = form == 2 && M.sym == SY_ROTATIONAL ? 0.5 : 0;
            k.tube(std::cos(a) * hub, std::sin(a) * hub, a + tw, len, r, tip == 1 ? r * 0.2 : r, 0.02 + r, M.mat, i);
            rodEnd(std::cos(a) * hub + std::cos(a + tw) * len, std::sin(a) * hub + std::sin(a + tw) * len, a, 0.02 + r, i);
        }
        return;
    }
    const double span = gap * nr;
    if (form == 3) { for (int e = -1; e <= 1; e += 2) k.box(L0 * 0.3, e * span * 0.55, 0, L0 * 1.1, 0.01, 0, 0.02, M.mat2); }   // a frame: the rods laid across its two rails
    else k.box(0, 0, PI / 2, span + 0.04, 0.03, 0, 0.025, M.mat2);
    for (int i = 0; i < nr; i++) {
        const double v = -span * 0.5 + gap * (i + 0.5), len = L0 / std::sqrt(k.ratio(i));
        if (form == 3) { const double u = L0 * 0.3 + (i - (nr - 1) * 0.5) * 0.0; k.tube(u - len * 0.5, v, 0, len, r, r, 0.02 + r, M.mat, i); (void)u; }
        else { k.tube(-0.02, v, 0, len, r, tip == 1 ? r * 0.2 : r, 0.025 + r, M.mat, i); rodEnd(len - 0.02, v, 0, 0.025 + r, i); }
    }
    k.lights(form == 3 ? L0 * 0.3 : 0.02, -span * 0.5, span, nr, 0.026, true);
}

void pSynth(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    const int keys = clampi(2 * k.n, 10, 18);
    const int keyMat = M.age >= AGE_FUTURISTIC ? IM_LIGHT : IM_CERAMIC, darkMat = M.age >= AGE_FUTURISTIC ? IM_GLASS : IM_LACQUER, black = M.age >= AGE_FUTURISTIC ? IM_VOID : IM_LACQUER;
    switch (plan) {
        case IP_RIBBON: {
            const double L = k.rng.range(0.6, 0.9) * k.S, D = 0.1 * k.S;
            InstrumentPart& b = k.body(0, 0, 0, L, D * 0.5, 0, 0.03, M.mat, 0); if (b.square < 3) b.square = 5;
            k.box(0, -D * 0.1, 0, L * 0.84, D * 0.18, 0.03, 0.033, M.mat2 == M.mat ? black : M.mat2);
            for (int i = 0; i < keys; i++) { const double u = -L * 0.42 + L * 0.84 * std::log(k.ratio(i)) / std::log(k.ratio(keys)); k.mark(u, -D * 0.1, 0.0035, 0.033, M.light == LT_LINES ? 1 : 0, keyMat == IM_LIGHT ? IM_LIGHT : k.trimMat(), i, PI / 2, D * 0.2); }
            k.knobs(-L * 0.4, D * 0.3, L * 0.2, 3, 0.007, 0.03, IM_STEEL);
            return;
        }
        case IP_PAD_GRID: {
            const int cols = clampi(M.number, 4, 8), rows = (keys + cols - 1) / cols;
            const double p = 0.034 * k.S, L = cols * p + 0.04, D = rows * p + 0.06;
            InstrumentPart& b = k.body(0, 0, 0, L, D * 0.5, 0, 0.03, M.mat, 0); if (b.square < 3) b.square = 5;
            for (int i = 0; i < keys; i++) { const int c = i % cols, r = i / cols; k.box(-L * 0.5 + 0.02 + p * (c + 0.5), -D * 0.5 + 0.02 + p * (r + 0.5), 0, p * 0.82, p * 0.41, 0.03, 0.036, i % 4 == 0 ? IM_PAINT : M.mat2, i); }
            k.lights(-L * 0.4, D * 0.42, L * 0.8, 8, 0.03, false);
            return;
        }
        case IP_ORB: {
            const double R = k.rng.range(0.1, 0.16) * k.S;
            k.ring(0, 0, R * 1.1, 0.012, 0, M.mat2);
            k.ball(0, 0, R, R, 0.01, R * 0.9, M.mat);
            const int keyM = M.age >= AGE_FUTURISTIC ? IM_LIGHT : (M.painted > 0.3 ? IM_PAINT : IM_CERAMIC);
            for (int i = 0; i < keys; i++) { const int ringN = i < keys / 2 ? 0 : 1; const double a = TAU * (i % (keys / 2)) / (keys / 2) + ringN * 0.3, rr = R * (ringN ? 0.45 : 0.75); k.mark(std::cos(a) * rr, std::sin(a) * rr, R * 0.06, 0.01 + R * 0.9 * std::sqrt(std::max(0.0, 1 - rr * rr / (R * R))), M.light == LT_LINES ? 1 : 0, keyM, i, a, R * 0.15); }
            return;
        }
        case IP_TONE_STRIPS: {
            const double L = k.rng.range(0.24, 0.36) * k.S, D = k.rng.range(0.18, 0.26) * k.S;
            k.body(0, 0, 0, L, D * 0.5, 0, 0.02, M.mat, 0);
            const int ns = clampi(k.n, 5, 9);
            for (int i = 0; i < ns; i++) k.box(-L * 0.4 + L * 0.8 * i / (ns - 1), 0, PI / 2, D * 0.7 * (0.5 + 0.5 / std::sqrt(k.ratio(i))), 0.004, 0.02, 0.024, IM_LIGHT, i);
            return;
        }
        default: break;
    }
    const double L = k.rng.range(0.45, 0.75) * k.S, D = k.rng.range(0.22, 0.32) * k.S, th = 0.05;
    InstrumentPart& b = k.body(0, 0, 0, L, D * 0.5, 0, th, M.mat, 0.1);
    if (b.square < 2.6 && M.sides == 0) b.square = 3.5;
    k.keyboard(-L * 0.36, -D * 0.2, L * 0.76, D * 0.42, keys, th, keyMat, darkMat);
    for (int r = 0; r < 2; r++) k.knobs(-L * 0.36, D * (0.16 + 0.12 * r), L * 0.55, clampi(M.number + r, 4, 10), 0.007, th, r ? M.mat2 : IM_STEEL);
    for (int i = 0; i < 4; i++) { k.box(L * 0.28 + 0.02 * i, D * 0.22, PI / 2, D * 0.18, 0.003, th, th + 0.002, black); k.box(L * 0.28 + 0.02 * i, D * (0.16 + 0.04 * (i % 3)), 0, 0.012, 0.006, th + 0.002, th + 0.008, M.mat2); }   // sliders
    for (int i = 0; i < 2; i++) k.disc(-L * 0.43, -D * 0.22 + 0.04 * i, 0.012, 0.012, th - 0.004, th + 0.006, black);   // the wheels
    k.lights(L * 0.05, D * 0.4, L * 0.35, 6, th, false);
}

void pSwarm(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    const double R = k.rng.range(0.14, 0.22) * k.S;
    switch (plan) {
        case IP_LATTICE: {
            const int g = clampi(M.number - 2, 3, 5);
            const double p = R * 1.6 / g;
            k.body(0, 0, 0, R * 2.1, R * 1.05, 0, 0.02, M.mat2, 0).square = 4;
            int c = 0;
            for (int i = 0; i <= g; i++) for (int j = 0; j <= g; j++) {
                const double u = -R * 0.8 + p * i + (j % 2 && M.sym != SY_BILATERAL ? p * 0.5 : 0), v = -R * 0.8 + p * j;
                if (std::hypot(u, v) > R * 1.05 && M.sym != SY_BILATERAL) continue;
                k.ball(u, v, 0.012, 0.012, 0.03, 0.02, (i + j) % 4 == 0 ? IM_LIGHT : M.mat, 0, c++ % 10);
                if (i < g) k.tubeTo(u, v, u + p, v, 0.003, 0.003, 0.035, M.mat);
                if (j < g) k.tubeTo(u, v, u + (j % 2 && M.sym != SY_BILATERAL ? -p * 0.5 : 0), v + p, 0.003, 0.003, 0.035, M.mat);
            }
            return;
        }
        case IP_CLOUD:
            k.ring(0, 0, R, 0.012, 0, M.mat2);
            for (int i = 0; i < 26; i++) { const double a = k.rng.uni() * TAU, rr = R * std::sqrt(k.rng.uni()) * 0.9, sz = k.rng.range(0.006, 0.016); k.ball(std::cos(a) * rr, std::sin(a) * rr, sz, sz, k.rng.range(0.02, 0.14), sz * 1.8, i % 5 == 0 ? IM_LIGHT : M.mat, 0, i % 10); }
            return;
        default: {   // rods with beads at their tips: an urchin (radial), a feather on its spine (bilateral), a pinwheel (rotational), tufts (none)
            int c = 0;
            auto rod = [&](double su, double sv, double a, double len, int mat) {
                k.tube(su, sv, a, len, 0.0025, 0.0018, 0.03, M.mat);
                k.ball(su + std::cos(a) * len, sv + std::sin(a) * len, 0.006, 0.006, 0.026, 0.01, mat, 0, c++ % 10);
            };
            if (M.sym == SY_BILATERAL) {
                const double L = R * 2.2;
                k.box(0, 0, 0, L, 0.01, 0, 0.025, M.mat2);
                for (int i = 0; i < 14; i++) for (int e = -1; e <= 1; e += 2) { const double u = -L * 0.45 + L * 0.9 * i / 13, len = R * (0.35 + 0.5 * std::sin(PI * (i + 0.5) / 14)); rod(u, 0, e * (PI / 2 - 0.35), len, i % 3 == 0 ? IM_LIGHT : k.trimMat()); }
            } else if (M.sym == SY_ROTATIONAL) {
                k.disc(0, 0, R * 0.22, R * 0.22, 0, 0.025, M.mat2, -1, M.sides);
                for (int arm = 0; arm < clampi(M.number, 3, 6); arm++) for (int j = 0; j < 6; j++) { const double a = TAU * arm / clampi(M.number, 3, 6) + 0.35 * j, d = R * (0.25 + 0.12 * j); rod(std::cos(a) * d, std::sin(a) * d, a + 1.2, R * 0.25, j % 2 ? IM_LIGHT : k.trimMat()); }
            } else if (M.sym == SY_NONE) {
                k.body(0, 0, 0, R * 2.2, R * 0.7, 0, 0.02, M.mat2, 0);
                for (int t = 0; t < 5; t++) { const double tu = k.rng.sym(R * 0.8), tv = k.rng.sym(R * 0.4); for (int i = 0; i < 6; i++) rod(tu, tv, k.rng.uni() * TAU, R * k.rng.range(0.15, 0.4), i % 3 == 0 ? IM_LIGHT : k.trimMat()); }
            } else {
                k.disc(0, 0, R * 0.5, R * 0.5, 0, 0.025, M.mat2, -1, M.sides);
                for (int i = 0; i < 24; i++) { const double a = TAU * i / 24 + k.rng.sym(0.12), len = R * k.rng.range(0.5, 1.0); rod(std::cos(a) * R * 0.3, std::sin(a) * R * 0.3, a + k.rng.sym(0.15), len, i % 3 == 0 ? IM_LIGHT : k.trimMat()); }
            }
            return;
        }
    }
}

void pField(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    const double R = k.rng.range(0.16, 0.28) * k.S;
    switch (plan) {
        case IP_RINGS: {   // rings nested, round or of the people's polygon, about one centre or each off the last, whole or broken, round a core
            const int nr = M.number <= 4 ? 1 + M.variant % 2 : clampi(M.number - 3, 2, 5), form = M.variant % 3;
            const int sides = M.sides >= 5 ? M.sides : 0;
            double cu = 0;
            for (int i = 0; i < nr; i++) {
                const double rr = R * (1 - (nr == 2 ? 0.3 : 0.18) * i), off = form == 1 ? R * 0.06 * i : 0;
                const int mat = i % 2 ? IM_LIGHT : M.mat;
                cu = off;
                if (sides) for (int j = 0; j < sides; j++) { const double a0 = TAU * j / sides + 0.2 * i, a1 = TAU * (j + 1) / sides + 0.2 * i; k.tubeTo(off + std::cos(a0) * rr, std::sin(a0) * rr, off + std::cos(a1) * rr, std::sin(a1) * rr, R * 0.045, R * 0.045, R * 0.05 + 0.004 * i, mat, i); }
                else if (form == 2) k.arc(off, 0, rr, 0.5 * i, 0.5 * i + TAU * 0.82, R * 0.05, R * 0.05, R * 0.05 + 0.004 * i, mat, 16);
                else k.ring(off, 0, rr, R * (0.03 + 0.05 * ((M.variant / 3) % 3)), 0.004 * i, mat, i);
                if (form == 2 || sides) { for (int q = 1; q <= (sides ? sides : 16); q++) k.out[k.out.size() - q].course = i; }
            }
            if (M.variant % 4 < 2) { k.ball(cu, 0, R * 0.12, R * 0.12, 0, R * 0.16, M.mat2, 0, nr); k.mark(cu, 0, R * 0.04, R * 0.16, M.hole, IM_LIGHT); }
            else k.disc(cu, 0, R * 0.22, R * 0.22, 0, R * 0.04, M.mat2, nr, M.sides >= 5 ? M.sides : 3 + M.number % 4);
            if (M.motif != MO_LINES) for (int i = 0; i < M.number; i++) { const double a = TAU * i / M.number + 0.3; k.mark(std::cos(a) * R * 1.12, std::sin(a) * R * 1.12, R * 0.035, 0.002, M.motif == MO_STARS ? 6 : (M.motif == MO_STEPS ? 2 : (M.motif == MO_CIRCLES ? 0 : 3)), i % 2 ? IM_LIGHT : M.mat2); }
            return;
        }
        case IP_MONOLITH: {
            const double L = R * 2.6, W = R * 0.7;
            InstrumentPart& b = k.body(0, 0, 0, L, W * 0.5, 0, W * 0.6, M.mat, 0.4); b.taper = std::min(b.taper, 0.9);
            k.box(0, 0, 0, L * 0.86, 0.003, W * 0.6 + b.dome * 0.8, W * 0.6 + b.dome * 0.8 + 0.002, IM_LIGHT, 0);
            for (int i = 0; i < M.number; i++) k.mark(-L * 0.35 + L * 0.7 * i / std::max(1, M.number - 1), W * 0.25, W * 0.04, W * 0.6, M.hole, IM_LIGHT, -1);
            return;
        }
        case IP_STAR: {   // rays from a heart: plain, forked at their tips, ringed, or curving (a people that turns); a crown of short rays within
            const int nr = clampi(M.number, 3, 9), form = M.variant % 4;
            const double th = R * k.rng.range(0.035, 0.08), heart = R * k.rng.range(0.1, 0.22);
            k.ball(0, 0, heart, heart, 0, heart * 1.6, M.mat2);
            for (int i = 0; i < nr; i++) {
                const double a = TAU * i / nr, len = R * (i % 2 && M.sym != SY_RADIAL ? k.rng.range(0.55, 0.8) : 1.0), su = std::cos(a) * heart * 0.8, sv = std::sin(a) * heart * 0.8;
                if (form == 3 || M.sym == SY_ROTATIONAL) { k.arc(su - std::sin(a) * len * 0.6, sv + std::cos(a) * len * 0.6, len * 0.6, a - PI / 2, a - PI / 2 + 1.4, th, th * 0.3, R * 0.08, M.mat, 6); for (int q = 1; q <= 6; q++) k.out[k.out.size() - q].course = i; continue; }
                k.tube(su, sv, a, len, th, form == 1 ? th * 0.6 : th * 0.15, R * 0.08, M.mat, i);
                const double eu = su + std::cos(a) * len, ev = sv + std::sin(a) * len;
                if (form == 1) for (int e = -1; e <= 1; e += 2) k.tube(eu, ev, a + e * 0.5, len * 0.22, th * 0.6, th * 0.1, R * 0.08, M.mat, i);   // forked
                if (form == 2) k.ring(eu, ev, th * 2.2, th * 0.4, R * 0.06, IM_LIGHT, i);                                                       // ringed
            }
            if (M.number >= 6) for (int i = 0; i < nr; i++) { const double a = TAU * (i + 0.5) / nr; k.tube(std::cos(a) * heart, std::sin(a) * heart, a, R * 0.3, th * 0.6, th * 0.1, R * 0.07, IM_LIGHT, i); }
            return;
        }
        case IP_FIELD_ORB: {   // an orb in its cradle, its moons in the people's order (a ring, a row, a spiral, scattered)
            const int moons = clampi(M.number - 2, 2, 7), cradle = M.variant % 3;
            if (cradle == 0) k.ring(0, 0, R * 0.7, R * 0.04, 0, M.mat2);
            else if (cradle == 1) for (int i = 0; i < 3; i++) { const double a = TAU * i / 3 + 0.5; k.tubeTo(std::cos(a) * R * 0.75, std::sin(a) * R * 0.75, std::cos(a) * R * 0.4, std::sin(a) * R * 0.4, R * 0.04, R * 0.03, R * 0.08, M.mat2); }
            else k.disc(0, 0, R * 0.65, R * 0.65, 0, R * 0.08, M.mat2, -1, M.sides >= 5 ? M.sides : 6);
            const size_t oi = k.out.size();
            k.ball(0, 0, R * 0.5, R * 0.5, R * 0.1, R * 0.8, M.mat, 0, 0);
            const InstrumentPart orb = k.out[oi];
            if (M.deco != ON_BARE) for (int b = 0; b < 2; b++) k.ring(0, 0, R * (0.2 + 0.15 * b), R * 0.012, k.surfaceAt(orb, R * (0.2 + 0.15 * b), 0), b ? IM_LIGHT : k.trimMat());
            for (int i = 0; i < moons; i++) {
                double u, v;
                switch (M.sym) {
                    case SY_BILATERAL: u = -R + 2 * R * i / std::max(1, moons - 1); v = (i % 2 ? 1 : -1) * R * 0.85; break;
                    case SY_ROTATIONAL: { const double a = 0.9 * i, d = R * (0.75 + 0.12 * i); u = std::cos(a) * d; v = std::sin(a) * d; break; }
                    case SY_NONE: { const double a = k.rng.uni() * TAU, d = R * k.rng.range(0.75, 1.15); u = std::cos(a) * d; v = std::sin(a) * d; break; }
                    default: { const double a = TAU * i / moons; u = std::cos(a) * R; v = std::sin(a) * R; break; }
                }
                const double mr = R * (0.06 + 0.05 * ((i * 7 + M.variant) % 3));
                k.ball(u, v, mr, mr, R * 0.3, mr * 1.6, i % 2 ? IM_LIGHT : M.mat, 0, i + 1);
            }
            return;
        }
        default: {   // nested shells: arcs within arcs, each open by the people's measure, turned a little from the last (a spiral where it turns)
            const int ns = clampi(M.number - 2, 2, 6);
            const double open = PI * k.rng.range(0.15, 0.6), turn = M.sym == SY_ROTATIONAL ? 0.7 : (M.sym == SY_NONE ? k.rng.range(0.2, 0.9) : 0.0), th = R * k.rng.range(0.03, 0.08);
            for (int i = 0; i < ns; i++) {
                const double rr = R * (1 - 0.8 * i / ns), a0 = open * 0.5 + turn * i;
                k.arc(0, 0, rr, a0, a0 + TAU - open, th, th * 0.8, th * 1.2, i % 2 ? IM_LIGHT : M.mat, 12);
                for (int q = 1; q <= 12; q++) k.out[k.out.size() - q].course = i;
            }
            if (M.variant % 2) k.ball(0, 0, R * 0.12, R * 0.12, 0, R * 0.2, M.mat2); else k.mark(0, 0, R * 0.08, 0.002, M.hole, IM_LIGHT);
            return;
        }
    }
}

// ---- skins -------------------------------------------------------------------------------------------------------------------------

// a skin seen face on, its painting by the people's motif
int skinFace(Mk& k, double u, double v, double R, double w, int sides) {
    const InstrumentMake& M = k.M;
    k.disc(u, v, R, R, w - 0.002, w, M.skin == IM_PLASTIC && M.age < AGE_MODERN ? IM_HIDE : M.skin, 0, sides);
    const int at = (int)k.out.size() - 1;
    if (M.painted < 0.3 && M.deco < ON_CARVED) return at;
    const int pm = IM_PAINT;
    switch (M.motif) {
        case MO_CIRCLES: k.ring(u, v, R * 0.55, R * 0.04, w, pm); k.mark(u, v, R * 0.18, w, 0, pm); break;
        case MO_STARS: for (int i = 0; i < M.number; i++) { const double a = TAU * i / M.number; k.mark(u + std::cos(a) * R * 0.55, v + std::sin(a) * R * 0.55, R * 0.1, w, 6, pm); } break;
        case MO_SPIRALS: for (int i = 0; i < 16; i++) { const double a = i * 0.75, rr = R * (0.1 + 0.05 * i); k.mark(u + std::cos(a) * rr, v + std::sin(a) * rr, R * 0.04, w, 0, pm); } break;
        case MO_LINES: for (int i = -2; i <= 2; i++) k.box(u + i * R * 0.25, v, PI / 2, R * 1.3 * std::sqrt(std::max(0.0, 1 - i * i * 0.0625)), R * 0.025, w, w + 0.001, pm); break;
        case MO_STEPS: for (int i = 0; i < 3; i++) k.box(u - R * 0.3 + i * R * 0.3, v - R * 0.2 + i * R * 0.2, 0, R * 0.25, R * 0.08, w, w + 0.001, pm); break;
        default: k.mark(u, v, R * 0.3, w, 4, pm); for (int i = 0; i < 4; i++) { const double a = TAU * i / 4 + PI / 4; k.mark(u + std::cos(a) * R * 0.62, v + std::sin(a) * R * 0.62, R * 0.08, w, 1, pm, -1, a, R * 0.3); } break;
    }
    return at;
}
// lacing or pegs round a drum lying on its side, along its length
void lacing(Mk& k, double u0, double u1, double R, double wc) {
    const InstrumentMake& M = k.M;
    const int n = 4 + M.number;
    switch (M.variant % 3) {
        case 0: for (int i = 0; i < n; i++) { const double v = -R * 0.9 + R * 1.8 * i / (n - 1); k.str(u0, v, wc + R * 0.9, u1, -v * 0.6, wc + R * 0.9, IM_FIBRE == M.fit ? IM_FIBRE : IM_HIDE, -1, 0.0012); } break;
        case 1: for (int e = 0; e < 2; e++) for (int i = 0; i < n; i++) k.mark(e ? u1 - 0.02 : u0 + 0.02, -R * 0.85 + R * 1.7 * i / (n - 1), R * 0.05, wc + R * 0.95, 0, M.fit); break;
        default: for (int i = 0; i < n / 2; i++) { const double v = -R * 0.8 + R * 1.6 * i / std::max(1, n / 2 - 1); k.tube(u0 + 0.01, v, 0, u1 - u0 - 0.02, 0.003, 0.003, wc + R * 0.97, M.mat2); } break;
    }
}

void pDrum(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    const double R = k.rng.range(0.1, 0.17) * k.S * (M.reg < 0 ? 1.15 : 1.0);
    switch (plan) {
        case IP_FRAME_DRUM: {
            const double RR = R * 1.4;
            const int sides = M.sides >= 5 ? M.sides : 0;
            k.disc(0, 0, RR, RR, 0, 0.05, M.mat, -1, sides);
            skinFace(k, 0, 0, RR * 0.95, 0.052, sides);
            if (M.variant % 2) for (int i = 0; i < 2 * M.number; i++) { const double a = TAU * i / (2 * M.number); k.disc(std::cos(a) * RR * 1.0, std::sin(a) * RR * 1.0, RR * 0.07, RR * 0.07, 0.015, 0.02, k.trimMat()); }
            k.beater(RR * 0.9, -RR * 1.1, 0.4, 0.25 * k.S, 0.012, M.mat, IM_HIDE);
            return;
        }
        case IP_BARREL: case IP_HOURGLASS: case IP_GOBLET: {
            const double L = R * k.rng.range(2.2, 3.2);
            if (plan == IP_BARREL) { k.tube(-L / 2, 0, 0, L / 2, R, R * 1.15, R * 1.15, M.mat); k.tube(0, 0, 0, L / 2, R * 1.15, R, R * 1.15, M.mat); }
            else if (plan == IP_HOURGLASS) { k.tube(-L / 2, 0, 0, L / 2, R, R * 0.45, R, M.mat); k.tube(0, 0, 0, L / 2, R * 0.45, R, R, M.mat); }
            else { k.tube(-L / 2, 0, 0, L * 0.45, R * 1.1, R * 0.4, R * 1.1, M.mat); k.tube(-L / 2 + L * 0.45, 0, 0, L * 0.4, R * 0.4, R * 0.42, R * 1.1, M.mat); k.tube(L / 2 - L * 0.15, 0, 0, L * 0.15, R * 0.42, R * 0.8, R * 1.1, M.mat); }
            const double wc = plan == IP_BARREL ? R * 1.15 : (plan == IP_GOBLET ? R * 1.1 : R);
            k.disc(-L / 2 - 0.004, 0, R * (plan == IP_GOBLET ? 1.1 : 1.0), R * 0.12, 0, 2 * wc, M.skin, 0);   // the skin seen edge on
            if (plan != IP_GOBLET) k.disc(L / 2 + 0.004, 0, R, R * 0.12, 0, 2 * wc, M.skin, 0);
            k.band(-L / 2 + 0.01, 0, 0, R * (plan == IP_GOBLET ? 1.12 : 1.02), R * 0.08, wc, M.mat2);
            if (plan != IP_GOBLET) k.band(L / 2 - 0.01, 0, 0, R * 1.02, R * 0.08, wc, M.mat2);
            lacing(k, -L / 2 + 0.01, plan == IP_GOBLET ? -L / 2 + L * 0.42 : L / 2 - 0.01, R * (plan == IP_HOURGLASS ? 0.7 : 1.0), wc - R);
            if (M.painted > 0.4) k.band(0, 0, 0, R * (plan == IP_BARREL ? 1.16 : 0.5), R * 0.06, wc, IM_PAINT);
            k.beater(-L * 0.2, -R * 2.0, 0.2, 0.28 * k.S, 0.014, M.stick, IM_HIDE);
            return;
        }
        case IP_POT_DRUM: case IP_KETTLE: {
            const double RR = R * (plan == IP_KETTLE ? 1.6 : 1.1);
            k.ball(0, 0, RR * 1.12, RR * 1.12, 0, RR * 1.1, M.mat);
            skinFace(k, 0, 0, RR * 0.95, RR * 1.12, 0);
            if (plan == IP_KETTLE) { k.ring(0, 0, RR * 0.97, 0.008, RR * 1.1, M.mat2); for (int i = 0; i < 6; i++) { const double a = TAU * i / 6; k.box(std::cos(a) * RR * 1.08, std::sin(a) * RR * 1.08, a, 0.04, 0.005, RR * 1.0, RR * 1.12, M.fit); } }
            else for (int i = 0; i < 2 * M.number; i++) { const double a = TAU * i / (2 * M.number); k.str(std::cos(a) * RR * 0.95, std::sin(a) * RR * 0.95, RR * 1.12, std::cos(a + 0.2) * RR * 1.1, std::sin(a + 0.2) * RR * 1.1, RR * 0.5, IM_FIBRE, -1, 0.0012); }
            return;
        }
        case IP_CARAPACE: {
            const double L = R * 2.8, W = R * k.rng.range(1.7, 2.2);
            const size_t ci = k.out.size();
            k.plate(0, 0, 0, L, W * 0.5, 0, R * 0.5, M.mat, clampd(M.taper, 0.7, 1.1), clampd(M.waist, -0.2, 0.25), clampd(M.square, 1.8, 3.2), R * 0.55, M.sides >= 5 ? M.sides : 0, M.skew);
            const InstrumentPart c = k.out[ci];
            const int seam = M.mat == IM_CHITIN ? IM_SHELL == M.trim ? IM_SHELL : k.trimMat() : IM_HOLE;   // dark seams on a pale shell, pale ones on chitin
            const int rows = 2 + M.number % 3, cols = 3 + M.number / 3;
            for (int i = 0; i < cols; i++) for (int j = 0; j < rows; j++) {
                const double u = -L * 0.32 + L * 0.64 * (i + 0.5) / cols, v = -W * 0.3 + W * 0.6 * (j + 0.5) / rows + (i % 2 ? W * 0.3 / rows : 0);
                if (std::hypot(u / L, v / W) > 0.4) continue;
                k.mark(u, v, W * 0.06, k.onTop(c, u, v), M.motif == MO_STARS ? 6 : (M.motif == MO_STEPS ? 2 : (M.motif == MO_CIRCLES ? 0 : 3)), seam);
            }
            const double su = M.variant % 2 ? L * 0.42 : 0, sr = M.variant % 2 ? W * 0.28 : W * 0.3;
            k.disc(su, 0, sr, sr * (M.variant % 2 ? 0.7 : 1.0), 0, k.surfaceAt(c, su, 0) + 0.004, M.skin, 0);   // its skin: over a mouth at its end, or over its back
            if (!(M.variant % 2)) k.ring(0, 0, sr, 0.003, k.surfaceAt(c, 0, 0) + 0.002, IM_FIBRE);
            return;
        }
        default: {   // a pair, joined
            const double R2 = R * 0.75;
            k.disc(-R * 1.05, 0, R, R, 0, R * 1.5, M.mat); skinFace(k, -R * 1.05, 0, R * 0.95, R * 1.5 + 0.002, 0);
            k.disc(R2 * 1.05 + 0.01, 0, R2, R2, 0, R * 1.3, M.mat); const int second = skinFace(k, R2 * 1.05 + 0.01, 0, R2 * 0.95, R * 1.3 + 0.002, 0);
            k.out[second].course = 1;
            k.box(-R * 0.05, 0, 0, R * 0.4, R * 0.25, R * 0.5, R * 0.9, M.mat2);
            return;
        }
    }
}

void pLog(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    const int nt = clampi(k.n, 3, 9);
    if (plan == IP_SLIT_CANES) {
        const double L0 = k.rng.range(0.3, 0.5) * k.S, r = k.rng.range(0.025, 0.04) * k.S;
        for (int i = 0; i < nt; i++) { const double v = (i - (nt - 1) * 0.5) * r * 2.3, len = L0 / std::sqrt(k.ratio(i)); k.tube(-len / 2, v, 0, len, r, r, r, IM_CANE, i); k.mark(0, v, r * 0.2, 2 * r, 1, IM_HOLE, i, 0, len * 0.6); k.band(-len / 2 + 0.01, v, 0, r * 1.03, r * 0.1, r, IM_CANE); }
        k.beater(L0 * 0.6, 0, PI / 2, 0.22 * k.S, 0.015, M.stick, IM_HIDE);
        return;
    }
    const double L = k.rng.range(0.5, 0.85) * k.S, R = k.rng.range(0.08, 0.13) * k.S;
    double top;
    InstrumentPart body;
    const bool log = plan != IP_TONGUE_BOX && (M.mat == IM_WOOD || M.mat == IM_DARKWOOD);
    if (plan == IP_TONGUE_BOX) {
        const size_t bi = k.out.size();
        k.body(0, 0, 0, L * 0.6, R * 1.1, 0, R * 1.2, M.mat, 0);
        body = k.out[bi]; top = R * 1.2;
        if (M.variant % 3 == 0) for (int e = -1; e <= 1; e += 2) for (int f = -1; f <= 1; f += 2) k.ball(e * L * 0.24, f * R * 0.9, R * 0.12, R * 0.12, 0, R * 0.2, M.stick);   // its feet
    } else if (log) {
        k.tube(-L / 2, 0, 0, L, R, R * 0.95, R, M.mat); top = 2 * R;
        k.head(L / 2, 0, 0, R * 0.6, R * 0.5, M.mat);
        k.disc(-L / 2 - 0.004, 0, R, R * 0.1, 0, 2 * R, M.mat);   // its cut end
    } else {
        const size_t bi = k.out.size();
        k.ball(0, 0, R * 1.5, R * 1.8, 0, R * 1.5, M.mat); body = k.out[bi]; top = R * 1.5;
    }
    const bool pairs = M.variant % 2 == 1, round = !log && (M.sym == SY_RADIAL || M.sym == SY_ROTATIONAL);
    const double span = plan == IP_TONGUE_BOX ? L * 0.5 : (log ? L * 0.7 : R * 2.6);
    for (int i = 0; i < nt; i++) {
        double u, v, a = PI / 2;
        const double len = R * (round ? 0.7 : 1.2) / std::sqrt(k.ratio(i) / k.ratio(0)), wd = (round ? R * 0.5 : span / nt) * 0.36;
        if (round) { const double t = TAU * i / nt; u = std::cos(t) * R * 0.55; v = std::sin(t) * R * 0.55; a = t; }
        else { u = -span * 0.5 + span * (i + 0.5) / nt; v = pairs ? 0 : (i % 2 ? 0.25 : -0.25) * R; }
        const double w = log ? top : k.surfaceAt(body, u, v);
        const double c = std::cos(a), s = std::sin(a);
        k.box(u, v, a, len, wd, w - 0.003, w + 0.004, M.mat, i);
        // its cut: a U round the tongue, or an H between two
        k.mark(u + c * len * 0.5, v + s * len * 0.5, 0.0015, w + 0.0005, 1, IM_HOLE, i, a + PI / 2, wd * 2.2);
        k.mark(u - s * wd * 1.1, v + c * wd * 1.1, 0.0015, w + 0.0005, 1, IM_HOLE, i, a, len);
        k.mark(u + s * wd * 1.1, v - c * wd * 1.1, 0.0015, w + 0.0005, 1, IM_HOLE, i, a, len);
        if (pairs) k.mark(u - c * len * 0.5, v - s * len * 0.5, 0.0015, w + 0.0005, 1, IM_HOLE, i, a + PI / 2, wd * 2.2);
    }
    if (plan == IP_TONGUE_BOX || !log) k.bodyOrnament(body, span * 0.6);
    for (int b = 0; b < 2; b++) k.beater(-L * 0.3 + 0.05 * b, -R * 2.2, 0.15, 0.26 * k.S, 0.016, M.stick, M.age >= AGE_INDUSTRIAL ? IM_FELT : IM_HIDE);
}

void pRattle(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    switch (plan) {
        case IP_VESSEL_RATTLE: {
            const double R = k.rng.range(0.05, 0.08) * k.S;
            k.ball(0, 0, R, R * k.rng.range(0.9, 1.3), 0, R * 1.5, M.mat, 0, 0);
            k.tube(R * 0.9, 0, 0, k.rng.range(0.1, 0.16) * k.S, 0.008, 0.007, 0.008, M.mat2);
            if (M.variant % 3 == 0) for (int i = 0; i < 18; i++) { const double a = TAU * i / 18, rr = R * (i % 2 ? 0.55 : 0.85); k.ball(std::cos(a) * rr, std::sin(a) * rr, R * 0.07, R * 0.07, R * 1.2, R * 0.12, k.trimMat()); }   // a net of beads
            else if (M.deco != ON_BARE) for (int i = 0; i < M.number; i++) { const double a = TAU * i / M.number; k.mark(std::cos(a) * R * 0.5, std::sin(a) * R * 0.5, R * 0.1, R * 1.48, M.motif == MO_STARS ? 6 : 0, IM_PAINT); }
            return;
        }
        case IP_STRUNG_RATTLE: {
            const int nb = 8 + 2 * M.number, form = M.variant % 3;
            auto pendant = [&](double u, double v, double a) {   // a shell, a hoof, a pod or a bone on its thread
                InstrumentPart& p = k.plate(u, v, a, 0.03, 0.012, 0, 0.008, M.mat, 0.6 + 0.1 * (M.variant % 4), M.waist * 0.5, clampd(M.square, 1.5, 4), 0.003);
                p.course = 0;
            };
            if (form == 0) {   // a ring to wear
                const double R = k.rng.range(0.08, 0.14) * k.S;
                k.ring(0, 0, R, 0.004, 0, M.age == AGE_PREHISTORIC ? IM_FIBRE : M.mat2);
                for (int i = 0; i < nb; i++) { const double a = TAU * i / nb, ru = R + 0.02; k.str(std::cos(a) * R, std::sin(a) * R, 0.006, std::cos(a) * ru, std::sin(a) * ru, 0.004, IM_FIBRE, -1, 0.0008); pendant(std::cos(a) * (ru + 0.012), std::sin(a) * (ru + 0.012), a); }
            } else if (form == 1) {   // a bunch at a stick's end
                const double L = k.rng.range(0.2, 0.3) * k.S;
                k.tube(-L, 0, 0, L, 0.009, 0.008, 0.009, M.stick);
                for (int i = 0; i < nb; i++) { const double a = -PI * 0.45 + PI * 0.9 * i / (nb - 1), len = 0.03 + 0.03 * (i % 3); k.str(0, 0, 0.012, std::cos(a) * len, std::sin(a) * len, 0.004, IM_FIBRE, -1, 0.0008); pendant(std::cos(a) * (len + 0.012), std::sin(a) * (len + 0.012), a); }
            } else {   // a string of them between two handles
                const double L = k.rng.range(0.3, 0.45) * k.S;
                k.str(-L / 2, 0, 0.006, L / 2, 0, 0.006, IM_FIBRE, -1, 0.0012);
                for (int e = -1; e <= 1; e += 2) k.tube(e > 0 ? L / 2 : -L / 2 - 0.07, 0, 0, 0.07, 0.008, 0.008, 0.008, M.stick);
                for (int i = 0; i < nb; i++) { const double u = -L * 0.42 + L * 0.84 * i / (nb - 1), side = i % 2 ? 1 : -1; pendant(u, side * 0.018, side * PI / 2); }
            }
            return;
        }
        case IP_STICK_RATTLE: {
            const double L = k.rng.range(0.25, 0.4) * k.S;
            k.tube(-L / 2, 0, 0, L, 0.009, 0.008, 0.009, M.mat);
            const int nd = 2 + M.number / 2;
            for (int i = 0; i < nd; i++) { const double u = -L * 0.1 + L * 0.5 * i / nd; k.tube(u, -0.03, PI / 2, 0.06, 0.002, 0.002, 0.012, M.mat2); for (int e = -1; e <= 1; e += 2) k.disc(u, e * 0.02, 0.014, 0.014, 0.004, 0.008, M.mat2, 0); }
            k.head(L / 2, 0, 0, 0.004, 0.012, M.mat);
            return;
        }
        default: {   // a rain tube: its body the people's profile, its pins in a spiral or in rows, its ends closed in its own way
            const double L = k.rng.range(0.5, 0.95) * k.S, R = k.rng.range(0.02, 0.045) * k.S;
            const size_t ti = k.out.size();
            k.profiledTube(-L / 2, 0, 0, L, R, R * 1.3, M.mat, 8);
            for (size_t q = ti; q < k.out.size(); q++) if (k.out[q].kind == IPK_TUBE) k.out[q].course = 0;
            const bool rows = M.rhythm == RH_GROUPED || M.rhythm == RH_PAIRED;
            for (int i = 0; i < 48; i++) {
                const double t = (i + 0.5) / 48;
                const double v = rows ? (i % 3 - 1) * R * 0.6 : std::cos(i * 2.4) * R * 0.8;
                if (!rows && std::sin(i * 2.4) < 0) continue;
                k.mark(-L / 2 + L * t, v, 0.0018, R * 1.3 + R * k.profileAt(t, clampd(M.taper, 0.6, 1.3), clampd(M.waist * 0.8, -0.35, 0.3)), 0, M.mat2);
            }
            for (int e = 0; e < 2; e++) {
                const double u = e ? L / 2 : -L / 2;
                if (M.variant % 3 == 0) k.disc(u, 0, R * 1.1, R * 1.1, 0, 2.6 * R, M.mat2 == M.mat ? IM_HIDE : M.mat2);
                else if (M.variant % 3 == 1) k.ball(u, 0, R * 1.2, R * 1.2, 0, 2.6 * R, M.mat2);
                else k.head(u, 0, e ? 0 : PI, R * 0.3, R * 0.9, M.mat2);
            }
            k.tubeOrnament(-L / 2, 0, 0, L, R, R * 1.3);
            return;
        }
    }
}

void pJingle(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    const int jm = M.mat2;
    switch (plan) {
        case IP_SISTRUM: {
            const double H = k.rng.range(0.12, 0.18) * k.S, W = H * 0.6;
            k.arc(0, 0, W * 0.5, -PI / 2, PI / 2, 0.006, 0.006, 0.006, M.mat, 10, W * 0.5);
            k.tube(-H * 0.9, 0, 0, H * 0.9, 0.009, 0.008, 0.008, M.mat);
            k.head(-H * 0.9, 0, PI, 0.004, 0.012, M.mat);
            const int rods = clampi(M.number / 2 + 1, 2, 4);
            for (int i = 0; i < rods; i++) {
                const double u = W * 0.1 + (W * 0.32) * i / std::max(1, rods - 1) - W * 0.05, half = std::sqrt(std::max(0.0, W * W * 0.25 - u * u));
                k.tube(u, -half, PI / 2, 2 * half, 0.0018, 0.0018, 0.008, jm);
                for (int j = 0; j < 3; j++) k.disc(u, -half * 0.6 + half * 0.6 * j, 0.008, 0.008, 0.006, 0.01, jm, i);
            }
            return;
        }
        case IP_JINGLE_HOOP: {   // a hoop (round, or the people's polygon), its jingles (discs, little bells or shells), a skin or none, a handle or none
            const double R = k.rng.range(0.1, 0.15) * k.S;
            if (M.sides >= 5) for (int i = 0; i < M.sides; i++) { const double a0 = TAU * i / M.sides, a1 = TAU * (i + 1) / M.sides; k.tubeTo(std::cos(a0) * R, std::sin(a0) * R, std::cos(a1) * R, std::sin(a1) * R, 0.011, 0.011, 0.011, M.mat); }
            else k.ring(0, 0, R, 0.012, 0, M.mat);
            if (M.variant % 2) skinFace(k, 0, 0, R * 0.95, 0.016, M.sides >= 5 ? M.sides : 0);
            const int pairs = clampi(M.number, 4, 9), kind = (M.variant / 2) % 3;
            for (int i = 0; i < pairs; i++) {
                const double a = TAU * (i + 0.5) / pairs, u = std::cos(a) * R, v = std::sin(a) * R;
                if (kind == 0) { k.disc(u, v, R * 0.12, R * 0.12, 0.018, 0.024, jm, i % 10); k.mark(u, v, R * 0.02, 0.025, 0, IM_HOLE); }
                else if (kind == 1) { k.bell(u - std::cos(a) * R * 0.1, v - std::sin(a) * R * 0.1, a, R * 0.2, R * 0.04, R * 0.09, 0.02, jm); for (int q = 1; q <= 4; q++) k.out[k.out.size() - q].course = i % 10; }
                else { InstrumentPart& sh = k.plate(u + std::cos(a) * R * 0.08, v + std::sin(a) * R * 0.08, a, R * 0.24, R * 0.07, 0.012, 0.022, M.trim == IM_LIGHT ? jm : (M.trim == IM_PAINT || M.trim == IM_LACQUER ? jm : M.trim), 0.6, 0, 2, 0.004); sh.course = i % 10; }
            }
            if (M.variant % 4 >= 2) k.tube(R, 0, 0, R * 0.9, 0.009, 0.008, 0.009, M.stick);   // a handle
            return;
        }
        default: {
            const double L = k.rng.range(0.3, 0.45) * k.S;
            k.tube(-L / 2, 0, 0, L, 0.01, 0.009, 0.01, M.mat);
            const int nd = 3 + M.number / 2;
            for (int i = 0; i < nd; i++) { const double u = -L * 0.2 + L * 0.6 * i / nd, side = i % 2 ? 1 : -1; k.tube(u, 0, side * PI / 2, 0.035, 0.002, 0.002, 0.012, jm); k.disc(u, side * 0.036, 0.013, 0.013, 0.006, 0.01, jm, i); }
            k.head(L / 2, 0, 0, 0.004, 0.012, M.mat);
            return;
        }
    }
}

// a cymbal's face: its boss, its rings or its hammer marks, a flanged rim, openings cut round it, the motif
void cymbalFace(Mk& k, double u, double v, double Re, double w, double boss, int sides) {
    const InstrumentMake& M = k.M;
    k.disc(u, v, Re, Re, w, w + 0.006, M.mat, 0, sides);
    if (M.variant % 5 == 1) k.ring(u, v, Re * 0.97, Re * 0.04, w + 0.002, M.mat, 0);   // a flange
    k.ball(u, v, Re * boss, Re * boss, w + 0.004, Re * boss * 0.4, M.mat, 0, 0);
    if (M.top == TP_MASTS) k.tube(u, v, 0, 0.001, Re * 0.05, Re * 0.05, w + Re * boss * 0.5, M.mat, 0);
    if (M.variant % 5 == 2) for (int i = 0; i < 2 * M.number; i++) { const double a = TAU * i / (2 * M.number); k.mark(u + std::cos(a) * Re * 0.72, v + std::sin(a) * Re * 0.72, Re * 0.06, w + 0.006, M.hole, IM_HOLE, -1, a); }   // openings cut through
    else if ((M.deco == ON_CARVED || M.variant % 5 == 3) && Re > 0.06) for (int i = 0; i < 16; i++) { const double a = i * 2.39996, rr = Re * (0.4 + 0.5 * std::sqrt((i + 0.5) / 16)); k.mark(u + std::cos(a) * rr, v + std::sin(a) * rr, Re * 0.03, w + 0.006, 0, IM_HOLE); }   // hammer marks
    else { const int rings = M.deco == ON_BARE ? 1 : 2; for (int r = 0; r < rings; r++) k.ring(u, v, Re * (0.5 + 0.4 * (r + 1) / (rings + 1)), 0.0012, w + 0.005, M.deco == ON_ENCRUSTED ? k.trimMat() : IM_HOLE); }
    if (M.motif == MO_STARS || M.motif == MO_FIGURES || M.motif == MO_GLYPHS) for (int i = 0; i < M.number; i++) { const double a = TAU * i / M.number; k.mark(u + std::cos(a) * Re * 0.55, v + std::sin(a) * Re * 0.55, Re * 0.05, w + 0.006, M.motif == MO_STARS ? 6 : 3, k.trimMat()); }
}

void pCymbalsOther(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    const int sides = M.sides >= 5 ? M.sides : 0;
    if (plan == IP_FINGER_CYMBALS) {   // small ones, two or three pairs, on their cords
        const int pairs = 2 + M.number % 2;
        const double R = k.rng.range(0.025, 0.04) * k.S;
        for (int p = 0; p < pairs; p++) for (int e = 0; e < 2; e++) {
            const double u = p * R * 3.2, v = e * R * 2.6;
            cymbalFace(k, u, v, R, 0, 0.32, sides);
            k.str(u, v, 0.008, u + R * 0.8, v + R * 1.3, 0.004, IM_FIBRE == M.mat2 ? IM_FIBRE : IM_HIDE, -1, 0.0012);
        }
        return;
    }
    // one great cymbal held by a bar across its back, its striker beside it
    const double R = k.rng.range(0.16, 0.24) * k.S;
    cymbalFace(k, 0, 0, R, 0, k.rng.range(0.15, 0.3), sides);
    k.box(0, 0, 0.4, R * 0.5, 0.01, R * 0.12, R * 0.12 + 0.012, M.stick);
    k.beater(R * 0.4, -R * 1.25, 0.2, R * 1.4, 0.016, M.stick, M.age >= AGE_INDUSTRIAL ? IM_FELT : IM_HIDE);
}

void pCymbals(Mk& k) {
    const InstrumentMake& M = k.M;
    const double R = k.rng.range(0.1, 0.2) * k.S, boss = k.rng.range(0.15, 0.4);
    const int sides = M.sides >= 5 ? M.sides : (M.square > 6 ? 4 : 0), hold = M.variant % 3;
    const double small = M.sym == SY_NONE || M.variant % 4 == 3 ? k.rng.range(0.55, 0.8) : 1.0;   // a pair of one size, or a great and a small
    for (int e = 0; e < 2; e++) {
        const double Re = e ? R * small : R;
        const double u = e * (R + Re) * (M.variant % 2 ? 1.05 : 0.6), v = e * R * (M.sym == SY_BILATERAL ? 0.0 : 0.35), w = 0.006 * e;
        cymbalFace(k, u, v, Re, w, boss, sides);
        if (hold == 0) k.box(u, v, 0.3 + e, R * 0.35, 0.012, w + R * boss * 0.4, w + R * boss * 0.4 + 0.004, M.mat2);   // a strap
        else if (hold == 1) k.ball(u, v, R * 0.08, R * 0.08, w + R * boss * 0.4, R * 0.1, M.stick);   // a knob
        else { k.ring(u, v, R * 0.12, 0.004, w + R * boss * 0.4, M.mat2); }   // a loop
    }
}

void pPads(Mk& k, bool ring) {
    const InstrumentMake& M = k.M;
    const int pads = 8 + 4 * (M.variant % 3 == 2);
    if (ring) {   // its pads in one ring or two, round or of the people's polygon, a dial in its middle or a well
        const double R = k.rng.range(0.14, 0.2) * k.S;
        const int padSides = M.sides >= 5 ? M.sides : (M.square > 5 ? 4 : 0), rings = M.number >= 6 ? 2 : 1;
        k.disc(0, 0, R, R, 0, 0.04, M.mat, -1, M.sides >= 5 ? M.sides : 0);
        for (int i = 0; i < pads; i++) {
            const int r = rings == 2 ? i % 2 : 0;
            const double a = TAU * i / pads + (r ? PI / pads : 0), rr = R * (rings == 2 ? (r ? 0.45 : 0.78) : 0.7), pr = R * (rings == 2 ? 0.1 : 0.14);
            k.disc(std::cos(a) * rr, std::sin(a) * rr, pr, pr, 0.04, 0.046, i % 4 == 0 ? IM_PAINT : M.mat2, i, padSides);
        }
        if (M.variant % 2) { k.disc(0, 0, R * 0.22, R * 0.22, 0.04, 0.05, M.age >= AGE_FUTURISTIC ? IM_VOID : IM_LACQUER); k.knobs(-R * 0.13, 0, R * 0.26, 2, 0.008, 0.05, IM_STEEL); }
        else { k.ring(0, 0, R * 0.2, R * 0.03, 0.04, M.mat2); k.lights(-R * 0.1, 0, R * 0.2, 3, 0.04, false); }
        return;
    }
    const int cols = pads == 12 ? 4 : (M.number >= 6 ? 4 : 2), rows = pads / cols;
    const double p = 0.045 * k.S, L = cols * p + 0.12, D = rows * p + 0.06;
    InstrumentPart& b = k.body(0, 0, 0, L, D * 0.5, 0, 0.045, M.mat, 0); if (b.square < 3) b.square = 5;
    for (int i = 0; i < pads; i++) { const int c = i % cols, r = i / cols; k.box(-L * 0.5 + 0.03 + p * (c + 0.5), -D * 0.5 + 0.03 + p * (r + 0.5), 0, p * 0.8, p * 0.4, 0.045, 0.052, i % 4 == 0 ? IM_PAINT : M.mat2, i); }
    k.knobs(L * 0.5 - 0.06, -D * 0.3, D * 0.6, clampi(M.number / 2, 2, 4), 0.008, 0.045, IM_STEEL);
    k.box(L * 0.5 - 0.025, 0, PI / 2, D * 0.7, 0.006, 0.045, 0.047, M.age >= AGE_FUTURISTIC ? IM_VOID : IM_LACQUER);
    k.lights(-L * 0.4, D * 0.44, L * 0.6, 8, 0.045, false);
}

void pMachine(Mk& k, int plan) {
    const InstrumentMake& M = k.M;
    const double L = k.rng.range(0.22, 0.32) * k.S, W = L * k.rng.range(0.55, 0.75), H = 0.1 * k.S;
    InstrumentPart& b = k.body(0, 0, 0, L, W * 0.5, 0, H, M.mat, 0); b.square = std::max(b.square, 4.0); b.taper = std::max(0.85, b.taper); b.waist = 0;
    // the mandrel and its wax cylinder, the reproducer on its arm, the crank
    k.tube(-L * 0.3, -W * 0.15, 0, L * 0.6, 0.026, 0.026, H + 0.03, IM_LACQUER, 0);   // the wax
    k.box(0, W * 0.15, 0, L * 0.7, 0.006, H, H + 0.05, M.mat2);
    k.disc(L * 0.1, -W * 0.02, 0.022, 0.022, H + 0.05, H + 0.065, M.mat2, 0);
    k.tube(L * 0.5, 0, 0, 0.05, 0.005, 0.005, H * 0.5, M.mat2); k.box(L * 0.5 + 0.05, 0.02, PI / 2, 0.05, 0.006, H * 0.5, H * 0.5 + 0.01, M.mat2); k.disc(L * 0.5 + 0.05, 0.045, 0.01, 0.01, H * 0.4, H * 0.5 + 0.04, M.stick);
    switch (plan) {
        case IP_HORN_MACHINE: {
            const int horn = M.variant % 3;   // straight, curved, a flower of petals
            const double hl = k.rng.range(0.3, 0.45) * k.S, hr = k.rng.range(0.12, 0.2) * k.S;
            if (horn == 1) { k.arc(L * 0.1, W * 0.5, W * 0.45, -PI / 2, 0.3, 0.012, 0.03, H + 0.06, M.mat2, 6); k.bell(L * 0.1 + std::cos(0.3) * W * 0.45, W * 0.5 + std::sin(0.3) * W * 0.45, 0.3 + PI / 2, hl * 0.7, 0.03, hr, H + 0.06, M.mat2); }
            else k.bell(L * 0.1, W * 0.05, PI / 2 + (horn == 2 ? 0.5 : 0.25), hl, 0.012, hr, H + 0.06, M.mat2);
            if (horn == 2) {   // a flower of petals: ribs along the bell from its throat to its rim
                const InstrumentPart last = k.out.back();
                const double eu = last.u + std::cos(last.ang) * last.len, ev = last.v + std::sin(last.ang) * last.len, nu = -std::sin(last.ang), nv = std::cos(last.ang);
                const int bu = (int)k.out.size() - 4;
                const double tu = k.out[bu].u, tv = k.out[bu].v;
                for (int i = 0; i < 7; i++) { const double t = (i - 3) / 3.0; k.str(tu, tv, H + 0.07, eu + nu * last.r1 * t * 0.95, ev + nv * last.r1 * t * 0.95, H + 0.06 + last.r1 * (1 - t * t) * 0.9, M.mat2 == IM_BRASS ? IM_COPPER : IM_BRASS, -1, 0.0012); }
            }
            break;
        }
        case IP_CABINET: {
            InstrumentPart& lid = k.body(0, W * 1.05, 0, L, W * 0.5, 0, 0.012, M.mat, 0); lid.square = std::max(lid.square, 4.0); lid.taper = std::max(0.85, lid.taper); lid.waist = 0;
            for (int i = 0; i < 7; i++) k.mark(-L * 0.38 + 0.02 * i, W * 0.35, 0.003, H, 1, IM_HOLE, -1, PI / 2, W * 0.2);   // its grille
            k.bodyOrnament(lid, 0);
            break;
        }
        default: {   // listening tubes to their ear-pieces
            for (int e = 0; e < 2 + M.variant % 3; e++) { const double a = -0.6 + 0.6 * e; k.arc(L * 0.1, W * 0.6, W * 0.6, -PI / 2 + a * 0.3, a, 0.004, 0.004, 0.006, IM_HIDE, 8, W * 0.9); k.ball(L * 0.1 + std::cos(a) * W * 0.9, W * 0.6 + std::sin(a) * W * 0.9, 0.012, 0.012, 0, 0.018, M.mat2); }
            break;
        }
    }
    k.bodyOrnament(b, 0);
}
}   // namespace

void instrumentPartsOf(const Tradition& T, const InstrumentMake& M, std::vector<InstrumentPart>& out) {
    out.clear();
    out.reserve(1024);   // the plans hold references to their bodies while they add to them: never past this
    Mk k(M, T, out);
    switch (M.plan) {
        case IP_PIPE: pPipe(k, false); break;
        case IP_CROSSPIPE: pPipe(k, true); break;
        case IP_VESSEL: pVessel(k); break;
        case IP_RAFT: pRaft(k); break;
        case IP_PIPE_RING: pPipeRing(k); break;
        case IP_TWIN_PIPE: pTwinPipe(k); break;
        case IP_REED_PIPE: pReedPipe(k, false); break;
        case IP_TWIN_REED: pReedPipe(k, true); break;
        case IP_BAG: pBag(k); break;
        case IP_CHAMBER: pChamber(k); break;
        case IP_BELLOWS: pBellows(k, false); break;
        case IP_BELLOWS_ROUND: pBellows(k, true); break;
        case IP_HORN_COIL: case IP_HORN_FOLDED: case IP_HORN_STRAIGHT: case IP_HORN_SPIRAL: case IP_HORN_FORKED: case IP_HORN_UPRIGHT: pHorn(k, M.plan); break;
        case IP_NECKED: pNecked(k, false); break;
        case IP_FRAME: pFrame(k); break;
        case IP_HARP: pHarp(k); break;
        case IP_BOARD: pBoard(k, false); break;
        case IP_TUBE_ZITHER: pTubeZither(k); break;
        case IP_RING_HARP: pRingHarp(k); break;
        case IP_TONGUES: pTongues(k); break;
        case IP_BOW: pBow(k); break;
        case IP_JAW: pJaw(k); break;
        case IP_BOW_HARP: pBowHarp(k); break;
        case IP_FIDDLE: pNecked(k, true); break;
        case IP_SPIKE: pSpike(k); break;
        case IP_TUBE_FIDDLE: pTubeFiddle(k); break;
        case IP_BOWED_BOARD: pBoard(k, true); break;
        case IP_WHEEL: pWheel(k); break;
        case IP_HORN_FIDDLE: pHornFiddle(k); break;
        case IP_BARS: pBars(k, false); break;
        case IP_TUBES: pTubes(k); break;
        case IP_BELLS: pBells(k); break;
        case IP_BELL_TREE: pBellTree(k); break;
        case IP_BOWLS: pBowls(k, false); break;
        case IP_GONGS: pGongs(k); break;
        case IP_STONES: pStones(k); break;
        case IP_SPOKES: pSpokes(k); break;
        case IP_BLADE: case IP_BUZZER: case IP_WHIRL_TUBE: pBlade(k, M.plan); break;
        case IP_HUM_ORB: pHum(k, false); break;
        case IP_HUM_RING: pHum(k, true); break;
        case IP_TUNE_DRUM: pTuneBox(k, false); break;
        case IP_TUNE_DISC: pTuneBox(k, true); break;
        case IP_SOLID: case IP_SKELETON: case IP_LAP: pElectricString(k, M.plan); break;
        case IP_KEY_SLAB: case IP_KEY_ARC: case IP_KEY_RING: pKeys(k, M.plan); break;
        case IP_GLASS_RODS: case IP_GLASS_BARS: case IP_GLASS_BOWLS: pGlass(k, M.plan); break;
        case IP_PANEL: case IP_RIBBON: case IP_PAD_GRID: case IP_ORB: case IP_TONE_STRIPS: pSynth(k, M.plan); break;
        case IP_ROD_FIELD: case IP_LATTICE: case IP_CLOUD: pSwarm(k, M.plan); break;
        case IP_RINGS: case IP_MONOLITH: case IP_STAR: case IP_FIELD_ORB: case IP_SHELLS: pField(k, M.plan); break;
        case IP_FRAME_DRUM: case IP_BARREL: case IP_GOBLET: case IP_POT_DRUM: case IP_HOURGLASS: case IP_KETTLE: case IP_CARAPACE: case IP_DRUM_PAIR: pDrum(k, M.plan); break;
        case IP_SLIT_LOG: case IP_TONGUE_BOX: case IP_SLIT_CANES: pLog(k, M.plan); break;
        case IP_VESSEL_RATTLE: case IP_STRUNG_RATTLE: case IP_STICK_RATTLE: case IP_RAIN_TUBE: pRattle(k, M.plan); break;
        case IP_SISTRUM: case IP_JINGLE_HOOP: case IP_JINGLE_STICK: pJingle(k, M.plan); break;
        case IP_CYMBALS: pCymbals(k); break;
        case IP_FINGER_CYMBALS: case IP_HAND_CYMBAL: pCymbalsOther(k, M.plan); break;
        case IP_PAD_BOX: pPads(k, false); break;
        case IP_PAD_RING: pPads(k, true); break;
        default: pMachine(k, M.plan); break;
    }
    // round its middle, nothing under the floor
    double u0, u1, v0, v1, top; instrumentBounds(out, u0, u1, v0, v1, top);
    const double cu = (u0 + u1) * 0.5, cv = (v0 + v1) * 0.5;
    for (InstrumentPart& p : out) { p.u -= cu; p.v -= cv; p.w0 = std::max(0.0, p.w0); p.w1 = std::max(p.w0, p.w1); }
}
