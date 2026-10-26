// A-02: the forms (see forms.h). Every kind is built in its own frame: a across the heading, y up, b along it (right-handed,
// so a polygon's normal is (v1 - v0) x (v[n-1] - v0) with its corners wound counter-clockwise seen from outside), tipped by
// the tilt about the across axis and set at the foot. C-01's box and hemisphere keep their own arithmetic, so the settlements
// of the C series draw to the bit as they did.
#include "forms.h"
#include "forms_internal.h"

const char* const FORM_KIND_NAMES[FK_COUNT] = {"BOX", "DOME", "LATHE", "PYRAMID", "VAULT", "ARCH", "SHELL", "TRUSS", "CABLE", "TORUS", "SWEEP", "BLOB", "PLATE", "FENCE"};
const int FORM_BANKS[2][FORM_SLOTS] = {{18, 19, 20, 24}, {25, 26, 27, 28}};

namespace {
struct PPt { double r, y; bool crease; };   // a profile's point: the radius and the height over the foot; a crease turns the surface sharply

// the frame of a form and the polygons it writes
struct Build {
    FormMesh& m;
    Vec3 foot, ax, ay, az;   // local (a, y, b) -> foot + ax a + ay y + az b
    Build(FormMesh& mesh, const Form& F, const Vec3& footIn) : m(mesh), foot(footIn) {
        const double h = F.heading, t = F.tilt;
        Vec3 s(std::cos(h), 0, -std::sin(h)), f(std::sin(h), 0, std::cos(h)), up(0, 1, 0);
        ax = s; ay = up * std::cos(t) + f * std::sin(t); az = f * std::cos(t) - up * std::sin(t);
    }
    Vec3 P(double a, double y, double b) const { return foot + ax * a + ay * y + az * b; }
    Vec3 D(double a, double y, double b) const { return ax * a + ay * y + az * b; }   // a direction
    // a flat polygon wound so its normal leans toward `hint` (a direction in the view's metres); u, v the face's metres when given
    void flat(const Vec3* p, int n, const Vec3& hint, int flags = 0, const double* u = nullptr, const double* v = nullptr) {
        Vec3 q[8]; double uu[8], vv[8]; int k = 0;
        for (int i = 0; i < n && k < 8; i++) {   // consecutive corners that coincide are one (an apex)
            if (k > 0 && length2(p[i] - q[k - 1]) < 1e-12) continue;
            q[k] = p[i]; uu[k] = u ? u[i] : 0; vv[k] = v ? v[i] : 0; k++;
        }
        while (k > 2 && length2(q[k - 1] - q[0]) < 1e-12) k--;
        if (k < 3) return;
        Vec3 nn(0, 0, 0);   // Newell's normal: the winding as a whole, whatever its first corner
        for (int i = 0; i < k; i++) { const Vec3& a = q[i]; const Vec3& b = q[(i + 1) % k]; nn.x += (a.y - b.y) * (a.z + b.z); nn.y += (a.z - b.z) * (a.x + b.x); nn.z += (a.x - b.x) * (a.y + b.y); }
        const bool flip = dot(nn, hint) < 0;
        const int first = (int)m.v.size();
        for (int i = 0; i < k; i++) { int j = flip ? (k - i) % k : i; FormMesh::V w; w.p = q[j]; w.u = uu[j]; w.v = vv[j]; m.v.push_back(w); }
        m.polys.push_back({first, k, flags & ~FormMesh::SMOOTH});
    }
    // a smooth polygon: each corner its own normal; wound to agree with them
    void smooth(const FormMesh::V* c, int n, int flags) {
        FormMesh::V q[8]; int k = 0;
        for (int i = 0; i < n && k < 8; i++) { if (k > 0 && length2(c[i].p - q[k - 1].p) < 1e-12) continue; q[k++] = c[i]; }
        while (k > 2 && length2(q[k - 1].p - q[0].p) < 1e-12) k--;
        if (k < 3) return;
        Vec3 nn(0, 0, 0), mean(0, 0, 0);
        for (int i = 0; i < k; i++) { const Vec3& a = q[i].p; const Vec3& b = q[(i + 1) % k].p; nn.x += (a.y - b.y) * (a.z + b.z); nn.y += (a.z - b.z) * (a.x + b.x); nn.z += (a.x - b.x) * (a.y + b.y); mean += q[i].n; }
        const bool flip = dot(nn, mean) < 0;
        const int first = (int)m.v.size();
        for (int i = 0; i < k; i++) m.v.push_back(q[flip ? (k - i) % k : i]);
        m.polys.push_back({first, k, flags | FormMesh::SMOOTH});
    }
    void line(const Vec3& a, const Vec3& b, double width, bool main = false) { m.lines.push_back({a, b, width, main}); }
};

int roundSides(double r, int lod) {   // a round form's faces: more for a big one near
    if (lod == 0) return clampi((int)std::lround(10 + 0.8 * r), 12, 28);
    if (lod == 1) return clampi((int)std::lround(6 + 0.3 * r), 8, 14);
    return 6;
}

// a profile turned about the axis: `n` faces (round: smooth, a prism: flat), the oval's along radius `kz` times the across one;
// `prism` turns the profile's radius into the apothem (the faces' middles at it, the corners out); `open` leaves the top without
// a cap and draws the inside; `capBottom` closes the foot (a tilted or raised form); the jagged top of a broken one by `jag`
// (metres down, by the corner); `pattern` the windows on the faces steeper than 45 degrees
void revolve(Build& B, const std::vector<PPt>& prof, int n, bool round, double kz, bool open, bool capBottom, double jag, uint32_t seed, bool pattern, double holes = 0) {
    const int K = (int)prof.size();
    if (K < 2 || n < 3) return;
    const double c0 = round ? 0.0 : PI / n, scale = round ? 1.0 : 1.0 / std::cos(PI / n);
    std::vector<double> ca(n + 1), sa(n + 1);
    for (int j = 0; j <= n; j++) { double th = c0 + TAU * j / n; ca[j] = std::cos(th); sa[j] = std::sin(th); }
    // the profile's normals in (r, y): each segment's outward, blended at a smooth point
    std::vector<double> snr(K - 1), sny(K - 1);
    for (int k = 0; k + 1 < K; k++) { double dr = prof[k + 1].r - prof[k].r, dy = prof[k + 1].y - prof[k].y, l = std::sqrt(dr * dr + dy * dy) + 1e-12; snr[k] = dy / l; sny[k] = -dr / l; }
    auto vnorm = [&](int k, int seg, double& nr, double& ny) {   // the normal at point k for the faces of segment `seg`
        int other = seg == k ? k - 1 : k;   // the neighbouring segment sharing point k
        nr = snr[seg]; ny = sny[seg];
        if (!prof[k].crease && other >= 0 && other < K - 1) { nr += snr[other]; ny += sny[other]; double l = std::sqrt(nr * nr + ny * ny) + 1e-12; nr /= l; ny /= l; }
    };
    const int flagsBase = open ? FormMesh::TWO_SIDED : 0;
    auto yAt = [&](int k, int j) { double y = prof[k].y; if (k == K - 1 && jag > 0) y -= jag * H01(seed, j % n); return y; };
    for (int k = 0; k + 1 < K; k++) {
        const bool wall = std::fabs(prof[k + 1].y - prof[k].y) > std::fabs(prof[k + 1].r - prof[k].r);
        for (int j = 0; j < n; j++) {
            if (holes > 0 && k + 2 >= K && H01(seed, 300 + j) < holes) continue;   // a broken form's holes under its top
            const int jj[2] = {j, j + 1}, kk[2] = {k, k + 1};
            FormMesh::V c[4];
            const int order[4][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};   // (θ, k): up the profile, then round
            for (int q = 0; q < 4; q++) {
                int jq = jj[order[q][0]], kq = kk[order[q][1]];
                double r = prof[kq].r * scale, y = yAt(kq, jq);
                c[q].p = B.P(r * ca[jq], y, r * sa[jq] * kz);
                double nr, ny; vnorm(kq, k, nr, ny);
                c[q].n = normalize(B.D(nr * ca[jq], ny, nr * sa[jq] / std::max(kz, 1e-3)));
                c[q].u = prof[kq].r * (c0 + TAU * jq / n); c[q].v = y;
            }
            int fl = flagsBase | ((pattern && wall) ? (FormMesh::FACE_UV | FormMesh::PATTERN) : 0);
            if (round) B.smooth(c, 4, fl);
            else {
                Vec3 p[4] = {c[0].p, c[1].p, c[2].p, c[3].p}; double u[4] = {c[0].u, c[1].u, c[2].u, c[3].u}, v[4] = {c[0].v, c[1].v, c[2].v, c[3].v};
                Vec3 mid = (c[0].n + c[1].n + c[2].n + c[3].n);
                B.flat(p, 4, mid, fl, u, v);
            }
        }
    }
    auto cap = [&](int k, bool top) {   // a flat cap at profile point k, its normal along the axis
        double r = prof[k].r * scale;
        if (r < 1e-6) return;
        Vec3 hint = B.D(0, top ? 1 : -1, 0);
        if (n <= 8) {
            Vec3 p[8]; for (int j = 0; j < n; j++) p[j] = B.P(r * ca[j], yAt(k, j), r * sa[j] * kz);
            B.flat(p, n, hint);
        } else {
            Vec3 cen = B.P(0, prof[k].y, 0);
            for (int j = 0; j < n; j += 2) {
                int j2 = std::min(n, j + 2);
                Vec3 p[4] = {cen, B.P(r * ca[j], yAt(k, j), r * sa[j] * kz), B.P(r * ca[j + 1], yAt(k, j + 1), r * sa[j + 1] * kz), B.P(r * ca[j2], yAt(k, j2), r * sa[j2] * kz)};
                B.flat(p, j2 == j + 1 ? 3 : 4, hint);
            }
        }
    };
    if (!open) cap(K - 1, true);
    if (capBottom) cap(0, false);
}

// ---- the kinds -------------------------------------------------------------------------------------------------------------

void boxMesh(FormMesh& m, const Form& e, const Vec3& foot, double skirt) {
    if (e.tilt == 0 && e.broken == 0 && e.pattern == 0) {   // C-01's box, its arithmetic and its faces in its order
        double y0 = foot.y + e.y0 - skirt, hy = (e.y1 - e.y0) + skirt;
        Vec3 c(foot.x, y0, foot.z);
        Vec3 f(std::sin(e.heading), 0, std::cos(e.heading)), s(std::cos(e.heading), 0, -std::sin(e.heading));
        Vec3 p[8];
        for (int i = 0; i < 8; i++) p[i] = c + f * ((i & 1) ? e.hz : -e.hz) + s * ((i & 2) ? e.hx : -e.hx) + Vec3(0, (i & 4) ? hy : 0, 0);
        static const int F[5][4] = {{4, 5, 7, 6}, {0, 1, 5, 4}, {2, 6, 7, 3}, {1, 3, 7, 5}, {0, 4, 6, 2}};
        for (int k = 0; k < 5; k++) {
            int first = (int)m.v.size();
            for (int q = 0; q < 4; q++) { FormMesh::V w; w.p = p[F[k][q]]; m.v.push_back(w); }
            m.polys.push_back({first, 4, 0});
        }
        return;
    }
    Build B(m, e, foot);
    // a broken wall stands in steps along its length (each its own height); a whole one is one step
    const bool alongLong = e.hz >= e.hx;
    const double L = alongLong ? e.hz : e.hx, W = alongLong ? e.hx : e.hz;
    const int steps = boxSteps(e);
    for (int i = 0; i < steps; i++) {
        double t0 = -L + 2 * L * i / steps, t1 = -L + 2 * L * (i + 1) / steps;
        const double top = boxStepTop(e, i);
        double ylo = e.y0 - skirt;
        auto P = [&](double along, double across, double y) { return alongLong ? B.P(across, y, along) : B.P(along, y, across); };
        Vec3 alongD = alongLong ? B.D(0, 0, 1) : B.D(1, 0, 0), acrossD = alongLong ? B.D(1, 0, 0) : B.D(0, 0, 1);
        int pat = e.pattern ? (FormMesh::FACE_UV | FormMesh::PATTERN) : 0;
        {   // the two long faces
            for (int sd = -1; sd <= 1; sd += 2) {
                Vec3 p[4] = {P(t0, sd * W, ylo), P(t1, sd * W, ylo), P(t1, sd * W, top), P(t0, sd * W, top)};
                double u[4] = {t0 + L, t1 + L, t1 + L, t0 + L}, v[4] = {ylo, ylo, top, top};
                B.flat(p, 4, acrossD * (double)sd, pat, u, v);
            }
        }
        for (int sd = -1; sd <= 1; sd += 2) {   // the ends of the step (inside a whole wall they face a neighbour; culled from where they are hidden)
            double t = sd < 0 ? t0 : t1;
            Vec3 p[4] = {P(t, -W, ylo), P(t, W, ylo), P(t, W, top), P(t, -W, top)};
            double u[4] = {-W + W, W + W, W + W, -W + W}, v[4] = {ylo, ylo, top, top};
            if ((sd < 0 && i > 0) || (sd > 0 && i + 1 < steps)) {   // between two steps only the taller one's end shows, over the lower
                const double nb = boxStepTop(e, sd < 0 ? i - 1 : i + 1);
                if (nb >= top) continue;
                p[0] = P(t, -W, nb); p[1] = P(t, W, nb); v[0] = v[1] = nb;
            }
            B.flat(p, 4, alongD * (double)sd, pat, u, v);
        }
        Vec3 q[4] = {P(t0, -W, top), P(t1, -W, top), P(t1, W, top), P(t0, W, top)};
        B.flat(q, 4, B.D(0, 1, 0));
        if (e.tilt != 0 || e.y0 > 0.3) {   // its underside where it shows (a box lying on its side: its foot's end was open, the inside seen through it)
            Vec3 u[4] = {P(t0, -W, ylo), P(t0, W, ylo), P(t1, W, ylo), P(t1, -W, ylo)};
            B.flat(u, 4, B.D(0, -1, 0));
        }
    }
}

void domeProfile(const Form& e, int lod, std::vector<PPt>& prof) {   // sub 1..3 (the hemisphere is C-01's own)
    const double R = e.hx, y0 = e.y0, H = std::max(0.1, e.y1 - e.y0);
    const int m = lod == 0 ? 6 : (lod == 1 ? 4 : 2);
    prof.clear();
    if (e.sub == 1) {   // a saucer: a spherical cap
        for (int k = 0; k <= m; k++) { double t = (double)k / m, a = t * PI / 2; prof.push_back({R * std::cos(a), y0 + H * std::sin(a), k == 0}); }
    } else if (e.sub == 2) {   // pointed: two arcs from the springing meeting at the apex
        const double c = 0.5 * R, rho = R + c, apex = std::sqrt(rho * rho - c * c), psiTop = std::acos(c / rho);
        for (int k = 0; k <= m; k++) { double psi = psiTop * k / m; prof.push_back({std::max(0.0, -c + rho * std::cos(psi)), y0 + H * (rho * std::sin(psi)) / apex, k == 0 || k == m}); }
    } else {   // an onion: swelling past its foot, drawn in to a point
        static const double OR[] = {1.0, 1.12, 1.16, 1.08, 0.86, 0.55, 0.26, 0.08, 0.0}, OY[] = {0.0, 0.12, 0.25, 0.38, 0.52, 0.66, 0.8, 0.92, 1.0};
        const int step = lod == 0 ? 1 : (lod == 1 ? 2 : 4);
        for (int k = 0; k <= 8; k += step) prof.push_back({R * OR[k], y0 + H * OY[k], k == 0 || k == 8});
        if (prof.back().y < y0 + H - 1e-9) prof.push_back({0, y0 + H, true});
    }
}

void domeMesh(FormMesh& m, const Form& e, int lod, const Vec3& foot) {
    if (e.sub == 0 && e.tilt == 0) {   // C-01's hemisphere: bands of quads, the top ones gone where it fell in
        Vec3 base(foot.x, foot.y + e.y0, foot.z);
        double rr = e.hx;
        int keep = e.broken > 0 ? (int)std::floor(4 * (1 - e.broken)) : 4;
        for (int eb = 0; eb < keep; eb++)
            for (int a = 0; a < 10; a++) {
                double e0 = eb * PI / 8, e1 = (eb + 1) * PI / 8, a0 = a * TAU / 10, a1 = (a + 1) * TAU / 10;
                Vec3 pts[4] = {Vec3(std::cos(a0) * std::cos(e0), std::sin(e0), std::sin(a0) * std::cos(e0)), Vec3(std::cos(a1) * std::cos(e0), std::sin(e0), std::sin(a1) * std::cos(e0)),
                               Vec3(std::cos(a1) * std::cos(e1), std::sin(e1), std::sin(a1) * std::cos(e1)), Vec3(std::cos(a0) * std::cos(e1), std::sin(e1), std::sin(a0) * std::cos(e1))};
                const Vec3 q[4] = {base + pts[0] * rr, base + pts[3] * rr, base + pts[2] * rr, base + pts[1] * rr};
                int first = (int)m.v.size();
                for (int k = 0; k < 4; k++) { FormMesh::V w; w.p = q[k]; m.v.push_back(w); }
                m.polys.push_back({first, 4, 0});
            }
        return;
    }
    Build B(m, e, foot);
    std::vector<PPt> prof;
    if (e.sub == 0) { const int mm = lod == 0 ? 6 : (lod == 1 ? 4 : 2); for (int k = 0; k <= mm; k++) { double a = PI / 2 * k / mm; prof.push_back({e.hx * std::cos(a), e.y0 + (e.y1 - e.y0) * std::sin(a), k == 0}); } }
    else domeProfile(e, lod, prof);
    double jag = 0;
    if (e.broken > 0) {   // caved: the profile cut under the top, the rim jagged, the inside showing
        double yc = e.y0 + (e.y1 - e.y0) * (1 - 0.8 * e.broken);
        std::vector<PPt> cut;
        for (size_t k = 0; k < prof.size(); k++) {
            if (prof[k].y <= yc) { cut.push_back(prof[k]); continue; }
            const PPt& a = prof[k - 1]; const PPt& b = prof[k];
            double t = (yc - a.y) / std::max(1e-9, b.y - a.y);
            cut.push_back({a.r + (b.r - a.r) * t, yc, true});
            break;
        }
        prof = cut;
        jag = 0.12 * (e.y1 - e.y0) * e.broken;
    }
    revolve(B, prof, roundSides(e.hx, lod), true, e.hz > 0 ? e.hz / std::max(1e-6, e.hx) : 1.0, e.broken > 0, false, jag, e.seed, false);
}

void latheProfile(const Form& e, int lod, double skirt, std::vector<PPt>& prof) {
    const double R = e.hx, y0 = e.y0, y1 = e.y1, H = std::max(0.1, y1 - y0);
    prof.clear();
    const bool grounded = e.y0 < 0.01 && e.tilt == 0;
    auto foot = [&]() { if (grounded && skirt > 0) prof.push_back({R, y0 - skirt, true}); };
    switch (e.sub) {
        case 0: foot(); prof.push_back({R, y0, true}); prof.push_back({R, y1, true}); break;
        case 1: foot(); prof.push_back({R, y0, true}); prof.push_back({R * std::max(0.0, e.p[0]), y1, true}); break;
        case 2: {   // a spire: the flank's power bows it in (a needle)
            foot();
            double pw = e.p[0] > 0 ? e.p[0] : 1.0;
            int m = pw == 1.0 ? 1 : (lod == 0 ? 6 : (lod == 1 ? 3 : 1));
            for (int k = 0; k <= m; k++) { double t = (double)k / m; prof.push_back({R * std::pow(1 - t, pw), y0 + H * t, k == 0 || k == m}); }
            break;
        }
        case 3: {   // a cooling tower: a hyperboloid's waist
            foot();
            double w = e.p[0] > 0 ? e.p[0] : 0.62, tw = e.p[1] > 0 ? e.p[1] : 0.72;
            int m = lod == 0 ? 8 : (lod == 1 ? 4 : 2);
            for (int k = 0; k <= m; k++) { double t = (double)k / m, q = (t - tw) / tw; prof.push_back({R * std::sqrt(w * w + (1 - w * w) * q * q), y0 + H * t, k == 0 || k == m}); }
            break;
        }
        case 4: {   // a tank: a drum with domed ends
            double c = std::min(0.45 * H, R * (e.p[0] > 0 ? e.p[0] : 0.35));
            int m = lod == 0 ? 4 : (lod == 1 ? 2 : 1);
            for (int k = 0; k <= m; k++) { double a = -PI / 2 + PI / 2 * k / m; prof.push_back({R * std::cos(a), y0 + c + c * std::sin(a), k == m}); }
            for (int k = 0; k <= m; k++) { double a = PI / 2 * k / m; prof.push_back({R * std::cos(a), y1 - c + c * std::sin(a), k == 0}); }
            break;
        }
        default: {   // a chimney: tapering to a lip
            foot();
            double top = R * (e.p[0] > 0 ? e.p[0] : 0.65), lip = std::min(1.5, 0.04 * H);
            prof.push_back({R, y0, true}); prof.push_back({top, y1 - lip, true}); prof.push_back({top * 1.18, y1 - lip, true}); prof.push_back({top * 1.18, y1, true});
            break;
        }
    }
}

void latheMesh(FormMesh& m, const Form& e, int lod, const Vec3& foot, double skirt) {
    Build B(m, e, foot);
    std::vector<PPt> prof; latheProfile(e, lod, skirt, prof);
    const bool round = e.sides < 3;
    const int n = round ? roundSides(std::max(e.hx, e.hz), lod) : std::min(e.sides, 8);
    bool open = (e.p[3] > 0 || e.sub == 5 || e.sub == 3) && e.tilt <= 0.3;   // B-420: one lying closed (its hollow a way in its body barred)
    double jag = 0, holes = 0;
    if (e.broken > 0) {   // snapped: the profile cut, its rim jagged, open, holed under the break
        double yc = e.y0 + (e.y1 - e.y0) * (1 - 0.85 * e.broken);
        std::vector<PPt> cut;
        for (size_t k = 0; k < prof.size(); k++) {
            if (prof[k].y <= yc) { cut.push_back(prof[k]); continue; }
            const PPt& a = prof[k - 1]; const PPt& b = prof[k];
            double t = (yc - a.y) / std::max(1e-9, b.y - a.y);
            cut.push_back({a.r + (b.r - a.r) * t, yc, true});
            break;
        }
        prof = cut; open = e.tilt <= 0.3; jag = std::min(0.25 * (yc - e.y0), 0.06 * (e.y1 - e.y0) + 0.5 * e.hx) * e.broken; holes = e.tilt > 0.3 ? 0.0 : 0.25 * e.broken;   // a fallen shaft whole underneath and closed at its break (its hollow showed a way in that its body barred)
        // the holes in the top stretch of the wall alone, over 2.2 m (a stretch from the foot left a gap down to the ground that the
        // explorer saw through and could not walk through)
        const double hy = e.y0 + 2.2;
        const size_t K = prof.size();
        if (K >= 2 && prof[K - 2].y < hy) {
            if (prof[K - 1].y > hy + 0.3) { const PPt a = prof[K - 2], b = prof[K - 1]; const double t = (hy - a.y) / std::max(1e-9, b.y - a.y); prof.insert(prof.end() - 1, PPt{a.r + (b.r - a.r) * t, hy, false}); }
            else holes = 0;
        }
    }
    revolve(B, prof, n, round, e.hz > 0 ? e.hz / std::max(1e-6, e.hx) : 1.0, open, e.tilt > 0.3 || (e.y0 > 0.01 && e.sub != 4), jag, e.seed, e.pattern > 0, holes);
}

void pyramidMesh(FormMesh& m, const Form& e, const Vec3& foot, double skirt) {
    Build B(m, e, foot);
    const int n = e.sides >= 3 ? std::min(e.sides, 8) : 4;
    const int T = std::max(1, (int)std::lround(e.p[0]));
    const double top = clampd(e.p[1], 0, 1), rs = (e.p[0] > 0) ? clampd(e.p[2], 0, 1) : 0.0, H = std::max(0.1, e.y1 - e.y0);
    const double kz = e.hz / std::max(1e-6, e.hx), R = e.hx / std::cos(PI / n), c0 = PI / n;
    const double cutY = e.broken > 0 ? e.y0 + H * (1 - 0.65 * e.broken) : 1e18;
    const bool grounded = e.y0 < 0.01 && e.tilt == 0;
    // a corner at a level (the foot of tier i is level 2i, the top of its riser 2i + 1; a smooth flank's top is the next foot)
    auto corner = [&](double sigma, int j, int level, double y) {
        double th = c0 + TAU * j / n, sg = sigma;
        if (e.broken > 0) sg *= 1 - 0.18 * e.broken * H01(e.seed, level * 16 + (j % n));   // the corners worn back
        return B.P(R * sg * std::cos(th), y, R * sg * std::sin(th) * kz);
    };
    const bool pat = e.pattern > 0 && rs > 0.5;
    for (int i = 0; i < T; i++) {
        double yb = e.y0 + H * i / T, yt = e.y0 + H * (i + 1) / T;
        if (yb >= cutY - 1e-9) break;
        double s0 = 1 - (1 - top) * i / T, s1 = 1 - (1 - top) * (i + 1) / T, sTop = s0 - (s0 - s1) * (1 - rs);
        const int lb = 2 * i, lt = rs > 0 ? 2 * i + 1 : 2 * i + 2;
        const bool cut = yt > cutY;
        if (cut) { sTop = s0 + (sTop - s0) * (cutY - yb) / (yt - yb); yt = cutY; }
        const double ybDraw = yb;
        for (int j = 0; j < n; j++) {
            if (i == 0 && grounded && skirt > 0) {   // B-420: the skirt upright under the foot, the first tier's face as drawn on flat ground (tilted
                                                     // by the skirt it was steeper the deeper it went: a heap on a slope too steep to climb)
                Vec3 q[4] = {corner(s0, j, lb, yb - skirt), corner(s0, j, lb, yb), corner(s0, j + 1, lb, yb), corner(s0, j + 1, lb, yb - skirt)};
                double th = c0 + TAU * (j + 0.5) / n, side = length(q[3] - q[0]);
                double u[4] = {0, 0, side, side}, v[4] = {yb - skirt, yb, yb, yb - skirt};
                B.flat(q, 4, B.D(std::cos(th), 0, std::sin(th) / std::max(kz, 1e-3)), 0, u, v);
            }
            Vec3 p[4] = {corner(s0, j, lb, ybDraw), corner(sTop, j, lt, yt), corner(sTop, j + 1, lt, yt), corner(s0, j + 1, lb, ybDraw)};
            double th = c0 + TAU * (j + 0.5) / n, side = length(p[3] - p[0]);
            double u[4] = {0, 0, side, side}, v[4] = {ybDraw, yt, yt, ybDraw};
            B.flat(p, 4, B.D(std::cos(th), (1 - rs) * 0.6, std::sin(th) / std::max(kz, 1e-3)), pat ? (FormMesh::FACE_UV | FormMesh::PATTERN) : 0, u, v);
        }
        if (cut || i == T - 1) {   // the cap over the last tier (a broken one's cut), unless it ends in an apex
            if (sTop < 1e-6) break;
            Vec3 p[8]; for (int j = 0; j < n; j++) p[j] = corner(sTop, j, lt, yt);
            B.flat(p, n, B.D(0, 1, 0));
            break;
        }
        if (rs > 0) for (int j = 0; j < n; j++) {   // the tread in to the next tier's foot
            Vec3 p[4] = {corner(sTop, j, lt, yt), corner(s1, j, 2 * i + 2, yt), corner(s1, j + 1, 2 * i + 2, yt), corner(sTop, j + 1, lt, yt)};
            B.flat(p, 4, B.D(0, 1, 0));
        }
    }
}

// a vault's arch in (a, y): from the right springing over the crown to the left
void vaultProfile(const Form& e, int lod, double hx, double rise, double y0, std::vector<std::pair<double, double>>& pts) {
    const int m = lod == 0 ? 12 : (lod == 1 ? 6 : 4);
    pts.clear();
    for (int k = 0; k <= m; k++) {
        double t = (double)k / m, a, y;
        if (e.sub == 1) {   // pointed: two arcs from centres inside the opposite springings
            const double c = 0.5 * hx, rho = hx + c, apex = std::sqrt(rho * rho - c * c), psiTop = std::acos(c / rho);
            bool right = t <= 0.5; double psi = psiTop * (right ? t * 2 : (1 - t) * 2);
            double x = -c + rho * std::cos(psi);
            a = right ? x : -x; y = y0 + rise * (rho * std::sin(psi)) / apex;
        } else if (e.sub == 2) { a = hx * (1 - 2 * t); y = y0 + rise * (1 - (a / hx) * (a / hx)); }
        else { double phi = PI * t; a = hx * std::cos(phi); y = y0 + rise * std::sin(phi); }
        pts.push_back({a, y});
    }
}

void vaultMesh(FormMesh& m, const Form& e, int lod, const Vec3& foot, double skirt) {
    Build B(m, e, foot);
    const double th = e.p[0] > 0 ? e.p[0] : std::max(0.25, 0.04 * e.hx), rise = std::max(0.2, e.y1 - e.y0);
    std::vector<std::pair<double, double>> out, in;
    vaultProfile(e, lod, e.hx, rise, e.y0, out);
    vaultProfile(e, lod, std::max(0.1, e.hx - th), std::max(0.1, rise - th), e.y0, in);
    const bool grounded = e.y0 < 0.01 && e.tilt == 0;
    if (grounded && skirt > 0) {   // springing from the ground: the foot sunk
        out.insert(out.begin(), {out.front().first, e.y0 - skirt}); out.push_back({out.back().first, e.y0 - skirt});
        in.insert(in.begin(), {in.front().first, e.y0 - skirt}); in.push_back({in.back().first, e.y0 - skirt});
    }
    const int K = (int)out.size();
    // the gap a broken vault has fallen through, its ends jagged by the segment
    double g0 = 1e9, g1 = -1e9;
    if (e.broken > 0) { g0 = -e.hz + 2 * e.hz * (0.2 + 0.25 * H01(e.seed, 1)); g1 = std::min(e.hz * (e.broken > 0.85 ? 0.85 : 1.0), g0 + 2 * e.hz * 0.6 * e.broken); }
    const Vec3 axis = B.P(0, e.y0, 0);
    for (int k = 0; k + 1 < K; k++) {
        double jagA = e.broken > 0 ? 1.5 * e.broken * H01(e.seed, 10 + k) : 0, jagB = e.broken > 0 ? 1.5 * e.broken * H01(e.seed, 60 + k) : 0;
        double spans[2][2] = {{-e.hz, e.broken > 0 ? std::min(e.hz, g0 - jagA) : e.hz}, {e.broken > 0 ? std::max(-e.hz, g1 + jagB) : e.hz, e.hz}};
        for (int s = 0; s < (e.broken > 0 ? 2 : 1); s++) {
            double b0 = spans[s][0], b1 = spans[s][1];
            if (b1 - b0 < 0.05) continue;
            Vec3 mid = B.P(0.5 * (out[k].first + out[k + 1].first), 0.5 * (out[k].second + out[k + 1].second), 0);
            Vec3 o[4] = {B.P(out[k].first, out[k].second, b0), B.P(out[k].first, out[k].second, b1), B.P(out[k + 1].first, out[k + 1].second, b1), B.P(out[k + 1].first, out[k + 1].second, b0)};
            B.flat(o, 4, mid - axis);
            Vec3 midIn = B.P(0.5 * (in[k].first + in[k + 1].first), 0.5 * (in[k].second + in[k + 1].second), 0);
            Vec3 q[4] = {B.P(in[k].first, in[k].second, b0), B.P(in[k].first, in[k].second, b1), B.P(in[k + 1].first, in[k + 1].second, b1), B.P(in[k + 1].first, in[k + 1].second, b0)};
            B.flat(q, 4, axis - midIn);
            // the band's ends: at the vault's two ends and at a broken stretch's cut
            for (int sd = 0; sd < 2; sd++) {
                double b = sd == 0 ? b0 : b1;
                Vec3 p[4] = {B.P(out[k].first, out[k].second, b), B.P(out[k + 1].first, out[k + 1].second, b), B.P(in[k + 1].first, in[k + 1].second, b), B.P(in[k].first, in[k].second, b)};
                B.flat(p, 4, B.D(0, 0, sd == 0 ? -1 : 1));
            }
        }
    }
    if (e.p[1] > 0) {   // closed ends: a wall under each arch, in strips down to the foot
        for (int sd = -1; sd <= 1; sd += 2) {
            double b = sd * e.hz;
            for (int k = 0; k + 1 < (int)in.size(); k++) {
                double yb = e.y0 - (grounded ? skirt : 0);
                Vec3 p[4] = {B.P(in[k].first, in[k].second, b), B.P(in[k + 1].first, in[k + 1].second, b), B.P(in[k + 1].first, yb, b), B.P(in[k].first, yb, b)};
                B.flat(p, 4, B.D(0, 0, sd));
            }
        }
    }
}

void archMesh(FormMesh& m, const Form& e, int lod, const Vec3& foot, double skirt) {
    Build B(m, e, foot);
    const int N = std::max(1, (int)std::lround(e.p[0] > 0 ? e.p[0] : 1));
    const double W = 2 * e.hx / N, w = W * clampd(e.p[1] > 0 ? e.p[1] : 0.32, 0.08, 0.46), Hh = e.y1 - e.y0;
    const double rise = e.sub == 2 ? 0.0 : (e.sub == 1 ? 1.35 * w : w);
    double s = e.p[2] > 0 ? e.p[2] : 0.5 * Hh;
    s = std::min(s, Hh - rise - std::min(0.6, 0.15 * Hh)); s = std::max(s, 0.3);
    const int m0 = e.sub == 2 ? 1 : (lod == 0 ? 8 : (lod == 1 ? 4 : 2));
    const double ylo = (e.y0 < 0.01 && e.tilt == 0) ? e.y0 - skirt : e.y0;
    // the head's curve of an opening, from its right springing to its left
    auto head = [&](double ac, int k) -> std::pair<double, double> {
        double t = (double)k / m0;
        if (e.sub == 2) return {ac + w * (1 - 2 * t), e.y0 + s};
        if (e.sub == 1) { const double c = 0.5 * w, rho = w + c, apex = std::sqrt(rho * rho - c * c), psiTop = std::acos(c / rho); bool right = t <= 0.5; double psi = psiTop * (right ? t * 2 : (1 - t) * 2); double x = -c + rho * std::cos(psi); return {ac + (right ? x : -x), e.y0 + s + rise * (rho * std::sin(psi)) / apex}; }
        double phi = PI * t; return {ac + w * std::cos(phi), e.y0 + s + rise * std::sin(phi)};
    };
    // the wall as intervals across its width: piers (from the foot) and strips over the heads (from the curve), each its top
    struct Iv { double xL, xR, botL, botR, top; bool gone; int bay; };
    std::vector<Iv> iv;
    std::vector<double> pierTop(N + 1, e.y1);
    std::vector<bool> fallen(N, false);
    if (e.broken > 0) {
        for (int j = 0; j < N; j++) fallen[j] = N == 1 ? e.broken > 0.3 : H01(e.seed, j) < e.broken * 0.75;
        for (int j = 0; j <= N; j++) if (N > 1 && H01(e.seed, 40 + j) < e.broken * 0.3) { pierTop[j] = e.y0 + Hh * (0.25 + 0.5 * H01(e.seed, 80 + j)); if (j > 0) fallen[j - 1] = true; if (j < N) fallen[j] = true; }
    }
    for (int j = 0; j < N; j++) {
        double ac = -e.hx + W * (j + 0.5);
        double xl = j == 0 ? -e.hx : ac - W * 0.5;
        iv.push_back({xl, ac - w, ylo, ylo, pierTop[j], false, -1});
        for (int k = m0; k > 0; k--) {   // left to right over the head
            auto p0 = head(ac, k), p1 = head(ac, k - 1);
            bool mid = fallen[j] && (k > m0 / 2 - 1 - (int)(e.broken * m0 / 3)) && (k - 1 < m0 / 2 + 1 + (int)(e.broken * m0 / 3));
            double top = e.y1;
            if (fallen[j] && !mid) top = std::max(std::max(p0.second, p1.second) + 0.2, e.y1 - (e.y1 - std::max(p0.second, p1.second)) * (0.3 + 0.6 * H01(e.seed, 120 + j * 16 + k)));
            iv.push_back({p0.first, p1.first, p0.second, p1.second, top, mid, j});
        }
        double xr = j == N - 1 ? e.hx : ac + W * 0.5;
        if (j == N - 1) iv.push_back({ac + w, xr, ylo, ylo, pierTop[N], false, -1});
        else iv.push_back({ac + w, xr, ylo, ylo, pierTop[j + 1], false, -1});
    }
    for (Iv& v : iv) if (v.bay < 0 && v.top < e.y1) v.top = std::max(v.top, e.y0 + 0.5);
    const Vec3 front = B.D(0, 0, 1), up = B.D(0, 1, 0), right = B.D(1, 0, 0);
    for (size_t i = 0; i < iv.size(); i++) {
        const Iv& v = iv[i];
        if (v.gone || v.xR - v.xL < 1e-6) continue;
        for (int sd = -1; sd <= 1; sd += 2) {
            double b = sd * e.hz;
            Vec3 p[4] = {B.P(v.xL, v.botL, b), B.P(v.xR, v.botR, b), B.P(v.xR, v.top, b), B.P(v.xL, v.top, b)};
            B.flat(p, 4, front * (double)sd);
        }
        Vec3 t[4] = {B.P(v.xL, v.top, -e.hz), B.P(v.xR, v.top, -e.hz), B.P(v.xR, v.top, e.hz), B.P(v.xL, v.top, e.hz)};
        B.flat(t, 4, up);
        if (v.bay >= 0) {   // the soffit under a strip
            Vec3 c = B.P(0.5 * (v.xL + v.xR), e.y0, 0);
            Vec3 q[4] = {B.P(v.xL, v.botL, -e.hz), B.P(v.xR, v.botR, -e.hz), B.P(v.xR, v.botR, e.hz), B.P(v.xL, v.botL, e.hz)};
            B.flat(q, 4, c - B.P(0.5 * (v.xL + v.xR), 0.5 * (v.botL + v.botR) + 1.0, 0));
        }
        // the faces between this interval and the next: a step between two tops, a break beside a fallen strip
        const Iv* nx = i + 1 < iv.size() ? &iv[i + 1] : nullptr;
        auto side = [&](double x, double y0s, double y1s, double dir) {
            if (y1s - y0s < 1e-6) return;
            Vec3 p[4] = {B.P(x, y0s, -e.hz), B.P(x, y0s, e.hz), B.P(x, y1s, e.hz), B.P(x, y1s, -e.hz)};
            B.flat(p, 4, right * dir);
        };
        if (nx && !nx->gone && std::fabs(nx->xL - v.xR) < 1e-6 && std::fabs(nx->top - v.top) > 1e-6) {
            if (nx->top < v.top) side(v.xR, nx->top, v.top, 1); else side(v.xR, v.top, nx->top, -1);
        }
        if (nx && nx->gone) side(v.xR, v.botR, v.top, 1);
        if (i > 0 && iv[i - 1].gone) side(v.xL, v.botL, v.top, -1);
        if (i == 0) side(v.xL, ylo, v.top, -1);
        if (i + 1 == iv.size()) side(v.xR, ylo, v.top, 1);
    }
    for (int j = 0; j < N; j++) {   // the jambs, from the foot to the springing
        double ac = -e.hx + W * (j + 0.5);
        for (int sd = -1; sd <= 1; sd += 2) {
            double x = ac + sd * w;
            Vec3 p[4] = {B.P(x, ylo, -e.hz), B.P(x, ylo, e.hz), B.P(x, e.y0 + s, e.hz), B.P(x, e.y0 + s, -e.hz)};
            B.flat(p, 4, right * (double)(-sd));
        }
    }
}

void shellMesh(FormMesh& m, const Form& e, int lod, const Vec3& foot) {
    Build B(m, e, foot);
    const int g = lod == 0 ? 8 : (lod == 1 ? 4 : 2);
    const double H = std::max(0.1, e.y1 - e.y0);
    if (e.sub == 0) {   // a saddle: a hyperbolic paraboloid, its low corners at y0
        auto at = [&](int i, int j, FormMesh::V& w) {
            double a = e.hx * (2.0 * i / g - 1), b = e.hz * (2.0 * j / g - 1);
            double y = e.y0 + 0.5 * H * (1 + (a / e.hx) * (b / e.hz));
            double sag = e.broken > 0 ? 0.35 * H * e.broken * std::max(0.0, (a / e.hx + b / e.hz) * 0.5) : 0;
            y -= sag;
            double dya = 0.5 * H * b / (e.hx * e.hz), dyb = 0.5 * H * a / (e.hx * e.hz);
            w.p = B.P(a, y, b); w.n = normalize(B.D(-dya, 1, -dyb));
        };
        for (int i = 0; i < g; i++) for (int j = 0; j < g; j++) {
            if (e.broken > 0 && H01(e.seed, i * 16 + j) < 0.45 * e.broken) continue;
            FormMesh::V c[4]; at(i, j, c[0]); at(i, j + 1, c[1]); at(i + 1, j + 1, c[2]); at(i + 1, j, c[3]);
            B.smooth(c, 4, FormMesh::TWO_SIDED);
        }
        if (e.y0 > 0.3) for (int sd = -1; sd <= 1; sd += 2) {   // raised: posts under the low corners
            double a = sd * e.hx * 0.96, b = -sd * e.hz * 0.96, r = std::max(0.15, 0.03 * std::min(e.hx, e.hz));
            Vec3 ps[4] = {B.P(a - r, -0.5, b - r), B.P(a + r, -0.5, b - r), B.P(a + r, -0.5, b + r), B.P(a - r, -0.5, b + r)};
            for (int q = 0; q < 4; q++) { Vec3 p0 = ps[q], p1 = ps[(q + 1) % 4]; Vec3 p[4] = {p0, p1, p1 + B.D(0, e.y0 + 0.5, 0), p0 + B.D(0, e.y0 + 0.5, 0)}; Vec3 mid = (p0 + p1) * 0.5 - B.P(a, -0.5, b); B.flat(p, 4, mid); }
        }
        return;
    }
    // a dish on a mast: a paraboloid aimed by the tilt about its vertex (the mast stays upright)
    const double R = e.hx, depth = e.p[0] > 0 ? e.p[0] : 0.3 * R, mastH = e.y0;
    const int rings = lod == 0 ? 6 : (lod == 1 ? 3 : 2), segs = lod == 0 ? 16 : (lod == 1 ? 10 : 6);
    Form upright = e; upright.tilt = 0;
    Build U(m, upright, foot);
    const double ct = std::cos(e.tilt), st = std::sin(e.tilt);
    auto aim = [&](double a, double y, double b) {   // dish frame (vertex at the origin, axis up) -> tipped toward +along about the vertex
        double yy = y * ct - b * st, bb = y * st + b * ct;
        return U.P(a, mastH + yy, bb);
    };
    auto aimD = [&](double a, double y, double b) { return normalize(U.D(a, y * ct - b * st, y * st + b * ct)); };
    for (int k = 0; k < rings; k++) for (int j = 0; j < segs; j++) {
        if (e.broken > 0 && H01(e.seed, k * 32 + j) < 0.45 * e.broken) continue;
        double r0 = R * k / rings, r1 = R * (k + 1) / rings, a0 = TAU * j / segs, a1 = TAU * (j + 1) / segs;
        double bend = e.broken > 0 ? 0.3 * depth * e.broken : 0;
        auto pt = [&](double r, double a, FormMesh::V& w) {
            double y = depth * (r / R) * (r / R) - bend * (r / R) * std::max(0.0, std::cos(a));
            w.p = aim(r * std::cos(a), y, r * std::sin(a));
            double dy = 2 * depth * r / (R * R);
            w.n = aimD(-dy * std::cos(a), 1, -dy * std::sin(a));
        };
        FormMesh::V c[4]; pt(r0, a0, c[0]); pt(r1, a0, c[1]); pt(r1, a1, c[2]); pt(r0, a1, c[3]);
        B.smooth(c, k == 0 ? 4 : 4, FormMesh::TWO_SIDED);
    }
    if (mastH > 0.2) {   // the mast
        double r = std::max(0.12, 0.05 * R);
        for (int q = 0; q < 4; q++) {
            double a0 = TAU * q / 4 + PI / 4, a1 = TAU * (q + 1) / 4 + PI / 4;
            Vec3 p[4] = {U.P(r * std::cos(a0), -1.0, r * std::sin(a0)), U.P(r * std::cos(a1), -1.0, r * std::sin(a1)), U.P(r * std::cos(a1), mastH, r * std::sin(a1)), U.P(r * std::cos(a0), mastH, r * std::sin(a0))};
            B.flat(p, 4, U.D(std::cos(0.5 * (a0 + a1)), 0, std::sin(0.5 * (a0 + a1))));
        }
    }
}

// a member as a thin prism (near) or a line (farther; a main one, a leg or a chord, drawn however thin)
void member(Build& B, const Vec3& a, const Vec3& b, double t, bool solid, bool main = false) {
    if (!solid) { B.line(a, b, 2 * t, main); return; }
    Vec3 d = b - a; double L = length(d); if (L < 1e-6) return; d = d / L;
    Vec3 e1 = cross(d, Vec3(0, 1, 0)); if (length2(e1) < 1e-6) e1 = cross(d, Vec3(1, 0, 0)); e1 = normalize(e1);
    Vec3 e2 = normalize(cross(e1, d));
    Vec3 o[4] = {e1 * t + e2 * t, e1 * -t + e2 * t, e1 * -t + e2 * -t, e1 * t + e2 * -t};
    for (int q = 0; q < 4; q++) {
        Vec3 p[4] = {a + o[q], b + o[q], b + o[(q + 1) % 4], a + o[(q + 1) % 4]};
        B.flat(p, 4, o[q] + o[(q + 1) % 4]);
    }
}

void trussMesh(FormMesh& m, const Form& e, int lod, const Vec3& foot, double skirt, double pxPerM) {
    Build B(m, e, foot);
    const double H = std::max(0.5, e.y1 - e.y0);
    const double tMember = e.sub == 0 ? (e.p[2] > 0 ? e.p[2] : clampd(0.04 * e.hx, 0.04, 0.5)) : (e.p[2] > 0 ? e.p[2] : clampd(0.05 * H, 0.04, 0.6));
    const bool solid = lod == 0 && 2 * tMember * pxPerM >= 1.5;   // prisms where a member spans a pixel and a half, lines thinner
    if (e.sub == 0) {   // a mast: legs, rings and the faces' crosses
        const int L = e.sides >= 3 ? std::min(e.sides, 4) : 4;
        const double topShare = e.p[0] > 0 ? e.p[0] : 0.3, t = e.p[2] > 0 ? e.p[2] : clampd(0.04 * e.hx, 0.04, 0.5);
        const int K = e.p[1] > 0 ? (int)e.p[1] : clampi((int)std::lround(H / std::max(0.8, e.hx * (1 + topShare))), 2, 40);
        const int kb = e.broken > 0 ? std::max(1, (int)(K * (1 - 0.7 * e.broken))) : K + 1;
        const double bend = 0.75 * e.broken, ybend = e.y0 + H * kb / K;
        auto pt = [&](int i, int k) {
            // R-412: a foot on the ground goes on down its leg's own line for the skirt (pushed straight down, a squat stump's leaning legs
            // bent away from their colliders)
            const double kk = k == 0 && e.y0 < 0.01 ? -skirt * K / H : (double)k;
            double sh = 1 - (1 - topShare) * kk / K, r = e.hx / std::cos(PI / L) * sh, th = PI / L + TAU * i / L;
            double a = r * std::cos(th), b = r * std::sin(th), y = e.y0 + H * kk / K;
            if (k > kb) { double dy = y - ybend, yy = ybend + dy * std::cos(bend) - b * std::sin(bend), bb = b * std::cos(bend) + dy * std::sin(bend); y = yy; b = bb; }   // bent over above the buckle
            return B.P(a, y, b);
        };
        auto gone = [&](int id) { return e.broken > 0 && H01(e.seed, id) < 0.3 * e.broken; };
        for (int k = 0; k < K; k++)
            for (int i = 0; i < L; i++) {
                if (!gone(k * 64 + i)) member(B, pt(i, k), pt(i, k + 1), t, solid, true);
                if (lod == 2) continue;
                if (!gone(k * 64 + 16 + i)) member(B, pt(i, k + 1), pt((i + 1) % L, k + 1), t * 0.7, solid);
                if (!gone(k * 64 + 32 + i)) member(B, pt(i, k), pt((i + 1) % L, k + 1), t * 0.55, solid);
                if (!gone(k * 64 + 48 + i)) member(B, pt((i + 1) % L, k), pt(i, k + 1), t * 0.55, solid);
            }
        return;
    }
    // a girder along the heading: chords, posts, diagonals and the cross members, sagging where broken
    const int K = e.p[1] > 0 ? (int)e.p[1] : clampi((int)std::lround(2 * e.hz / std::max(1.0, H)), 2, 60);
    const double t = e.p[2] > 0 ? e.p[2] : clampd(0.05 * H, 0.04, 0.6);
    auto pt = [&](int side, int k, bool top) {
        double b = -e.hz + 2 * e.hz * k / K, y = top ? e.y1 : e.y0;
        if (e.broken > 0) y -= 0.8 * H * e.broken * std::sin(PI * k / K);
        return B.P(side * e.hx, y, b);
    };
    auto gone = [&](int id) { return e.broken > 0 && H01(e.seed, id) < 0.3 * e.broken; };
    for (int k = 0; k < K; k++)
        for (int sd = -1; sd <= 1; sd += 2) {
            member(B, pt(sd, k, false), pt(sd, k + 1, false), t, solid, true);
            member(B, pt(sd, k, true), pt(sd, k + 1, true), t, solid, true);
            if (lod == 2) continue;
            if (!gone(k * 8 + sd + 1)) member(B, pt(sd, k, false), pt(sd, k, true), t * 0.7, solid);
            if (!gone(k * 8 + sd + 4)) member(B, (k & 1) ? pt(sd, k, true) : pt(sd, k, false), (k & 1) ? pt(sd, k + 1, false) : pt(sd, k + 1, true), t * 0.6, solid);
        }
    if (lod < 2) for (int k = 0; k <= K; k++) { member(B, pt(-1, k, false), pt(1, k, false), t * 0.6, solid); member(B, pt(-1, k, true), pt(1, k, true), t * 0.6, solid); }
    if (lod < 2) for (int sd = -1; sd <= 1; sd += 2) member(B, pt(sd, K, false), pt(sd, K, true), t * 0.7, solid);
}

void cableMesh(FormMesh& m, const Form& e, int lod, const Vec3& foot) {
    Build B(m, e, foot);
    const int K = lod == 0 ? 16 : (lod == 1 ? 8 : 4);
    const Vec3 A = foot + Vec3(0, e.y1, 0), Bp = foot + Vec3(e.p[0] - e.x, e.p[2], e.p[1] - e.z);
    const double sag = e.p[3], w = 2 * std::max(0.0005, e.hx);   // C-17: an instrument's string a millimetre (the buildings' cables are 3 cm and more)
    auto at = [&](double t) { return lerp(A, Bp, t) - Vec3(0, 4 * sag * t * (1 - t), 0); };
    if (e.broken <= 0) { for (int k = 0; k < K; k++) B.line(at((double)k / K), at((double)(k + 1) / K), w, true); return; }
    // snapped: each end hangs from its mount and lies along the ground toward the other (the ground taken as the foot's)
    double ts = 0.4 + 0.2 * H01(e.seed, 1), len = length(Bp - A);
    for (int end = 0; end < 2; end++) {
        Vec3 P0 = end == 0 ? A : Bp, toward = normalize(Vec3((end == 0 ? Bp : A).x - P0.x, 0, (end == 0 ? Bp : A).z - P0.z));
        double hang = (end == 0 ? ts : 1 - ts) * len;
        double drop = std::min(hang, P0.y - foot.y);
        Vec3 P1 = P0 - Vec3(0, drop, 0);
        B.line(P0, P1, w, true);
        if (hang > drop) B.line(P1, P1 + toward * (hang - drop), w, true);
    }
}

void torusMesh(FormMesh& m, const Form& e, int lod, const Vec3& foot) {
    Build B(m, e, foot);
    const double R = e.hx, r = e.p[0] > 0 ? e.p[0] : 0.15 * e.hx;
    const int U = lod == 0 ? 32 : (lod == 1 ? 16 : 10), V = lod == 0 ? 8 : (lod == 1 ? 6 : 4);
    double gap0 = 0, gap1 = -1;   // a broken ring's missing sector
    if (e.broken > 0) { gap0 = TAU * H01(e.seed, 1); gap1 = gap0 + 1.8 * e.broken; }
    auto inGap = [&](double a) { if (gap1 < gap0) return false; double d = std::fmod(a - gap0 + 4 * TAU, TAU); return d < gap1 - gap0; };
    const size_t first = m.v.size();
    auto pt = [&](double u, double v, FormMesh::V& w) {
        double cu = std::cos(u), su = std::sin(u), cv = std::cos(v), sv = std::sin(v);
        w.p = B.P((R + r * cv) * cu, r * sv, (R + r * cv) * su);
        w.n = normalize(B.D(cv * cu, sv, cv * su));
    };
    for (int i = 0; i < U; i++) {
        double u0 = TAU * i / U, u1 = TAU * (i + 1) / U;
        if (inGap(0.5 * (u0 + u1))) continue;
        for (int j = 0; j < V; j++) {
            double v0 = TAU * j / V, v1 = TAU * (j + 1) / V;
            FormMesh::V c[4]; pt(u0, v0, c[0]); pt(u1, v0, c[1]); pt(u1, v1, c[2]); pt(u0, v1, c[3]);
            B.smooth(c, 4, 0);
        }
        // a cap where the ring breaks off
        for (int sd = 0; sd < 2; sd++) {
            double ua = sd == 0 ? u0 : u1, ub = sd == 0 ? u0 - 1e-3 : u1 + 1e-3;
            if (!inGap(ub) || inGap(0.5 * (u0 + u1))) continue;
            Vec3 p[8]; int n = std::min(V, 8);
            for (int j = 0; j < n; j++) { double v = TAU * j / n; p[j] = B.P((R + r * std::cos(v)) * std::cos(ua), r * std::sin(v), (R + r * std::cos(v)) * std::sin(ua)); }
            Vec3 tang = B.D(-std::sin(ua), 0, std::cos(ua));
            B.flat(p, n, sd == 0 ? tang * -1.0 : tang);
        }
    }
    // resting on the ground: the lowest point at y0
    double minY = 1e18; for (size_t k = first; k < m.v.size(); k++) minY = std::min(minY, m.v[k].p.y);
    if (minY < 1e17) { double dy = foot.y + e.y0 - minY; for (size_t k = first; k < m.v.size(); k++) m.v[k].p.y += dy; }
}

// a closed section (r out from the path, y up) swept along an arc or a line
void sweepMesh(FormMesh& m, const Form& e, int lod, const Vec3& foot, double skirt) {
    Build B(m, e, foot);
    const double H = std::max(0.1, e.y1 - e.y0), w0 = e.p[1] > 0 ? e.p[1] : 0.6, w1 = e.p[2] > 0 ? e.p[2] : w0;
    const int T = std::max(0, (int)std::lround(e.p[3]));
    const double ylo = (e.y0 < 0.01 && e.tilt == 0) ? e.y0 - skirt : e.y0;
    // the section, counter-clockwise in (r, y): the outer foot, up the outer face, over the top, down the inner face
    std::vector<std::pair<double, double>> sec;
    if (T > 0) {   // the outer face upright, the top 2 w1 wide, the inner side stepping down in tiers to the inner foot
        double rTop = w0 - 2 * w1;
        sec.push_back({w0, ylo}); sec.push_back({w0, e.y1}); sec.push_back({rTop, e.y1});
        for (int k = T; k >= 1; k--) {
            double rk = -w0 + (rTop + w0) * k / T, rk1 = -w0 + (rTop + w0) * (k - 1) / T, yk1 = e.y0 + H * (k - 1) / T;
            sec.push_back({rk, yk1});    // down the riser
            sec.push_back({rk1, yk1});   // in along the tread
        }
        sec.push_back({-w0, ylo});
    } else if (ylo < e.y0 - 1e-6) {   // B-420: its skirt upright under its foot, its slopes as they are drawn on flat ground (the skirt tilted them, steeper the
                                       // deeper it went: a bank on a slope too steep to climb)
        sec = {{w0, ylo}, {w0, e.y0}, {w1, e.y1}, {-w1, e.y1}, {-w0, e.y0}, {-w0, ylo}};
    } else {
        sec = {{w0, ylo}, {w1, e.y1}, {-w1, e.y1}, {-w0, ylo}};
    }
    // tidy: no repeated corner
    std::vector<std::pair<double, double>> s2; for (auto& p : sec) if (s2.empty() || std::hypot(p.first - s2.back().first, p.second - s2.back().second) > 1e-6) s2.push_back(p);
    if (s2.size() > 1 && std::hypot(s2.front().first - s2.back().first, s2.front().second - s2.back().second) < 1e-6) s2.pop_back();
    sec = s2;
    const int S = (int)sec.size();
    // the path
    const bool arc = e.sub == 0;
    const double ang = arc ? (e.p[0] > 0 ? std::min(e.p[0], TAU) : TAU) : 0, Rc = e.hx;
    const bool closed = arc && ang > TAU - 1e-6;
    const double len = arc ? Rc * ang : 2 * e.hz;
    const double stepM = lod == 0 ? 6.0 : (lod == 1 ? 20.0 : 60.0);
    // a broken one keeps the near pieces at every level, so its gaps stay where they are (and where its colliders leave them out)
    const int P = e.broken > 0 ? clampi((int)std::ceil(len / 6.0), arc ? 8 : 1, 64) : clampi((int)std::ceil(len / stepM), arc ? (lod == 0 ? 8 : 4) : 1, 64);
    auto frameAt = [&](double t, Vec3& o, Vec3& out) {   // the path's point and its outward (the section's +r) at t in [0, 1]
        if (arc) { double th = ang * t; o = B.P(Rc * std::sin(th), 0, Rc * std::cos(th)); out = B.D(std::sin(th), 0, std::cos(th)); }
        else { o = B.P(0, 0, -e.hz + 2 * e.hz * t); out = B.D(1, 0, 0); }
    };
    std::vector<double> topOf(P, e.y1);
    std::vector<bool> gone(P, false);
    if (e.broken > 0) for (int i = 0; i < P; i++) { gone[i] = H01(e.seed, i) < 0.35 * e.broken; topOf[i] = e.y1 - H * 0.6 * e.broken * H01(e.seed, 100 + i); }
    auto secPtTop = [&](int i, double t, double top) {   // a section corner with the top lowered (corners above it pulled down)
        Vec3 o, out; frameAt(t, o, out);
        double y = std::min(sec[i].second, top);
        return o + out * sec[i].first + Vec3(0, y, 0);
    };
    const bool pat = e.pattern > 0;
    for (int p = 0; p < P; p++) {
        if (gone[p]) continue;
        double t0 = (double)p / P, t1 = (double)(p + 1) / P, top = topOf[p];
        for (int i = 0; i < S; i++) {
            int i2 = (i + 1) % S;
            if (sec[i].second >= top && sec[i2].second >= top && e.broken > 0 && std::fabs(sec[i].second - sec[i2].second) > 1e-6) continue;
            Vec3 a = secPtTop(i, t0, top), b = secPtTop(i2, t0, top), c = secPtTop(i2, t1, top), d = secPtTop(i, t1, top);
            Vec3 o, out; frameAt(0.5 * (t0 + t1), o, out);
            double dr = sec[i2].first - sec[i].first, dy = sec[i2].second - sec[i].second;
            Vec3 hint = out * dy + Vec3(0, -dr, 0);   // the edge's outward in the section (counter-clockwise: (dy, -dr))
            bool wall = std::fabs(dy) > std::fabs(dr) && sec[i].first > 0 && sec[i2].first > 0;   // the outer face
            double u0 = (arc ? (Rc + sec[i].first) * ang : 2 * e.hz) * t0, u1 = (arc ? (Rc + sec[i].first) * ang : 2 * e.hz) * t1;
            Vec3 q[4] = {a, b, c, d}; double u[4] = {u0, u0, u1, u1}, v[4] = {a.y - foot.y, b.y - foot.y, c.y - foot.y, d.y - foot.y};
            B.flat(q, 4, hint, (pat && wall) ? (FormMesh::FACE_UV | FormMesh::PATTERN) : 0, u, v);
        }
        // the section's ends: at a path's ends, a gap's edges and between pieces of a ragged top
        bool endA = (!closed && p == 0) || (p > 0 && (gone[p - 1] || (e.broken > 0 && topOf[p - 1] < top))) || (closed && p == 0 && (gone[P - 1] || (e.broken > 0 && topOf[P - 1] < top)));
        bool endB = (!closed && p == P - 1) || (p + 1 < P && (gone[p + 1] || (e.broken > 0 && topOf[p + 1] < top))) || (closed && p == P - 1 && (gone[0] || (e.broken > 0 && topOf[0] < top)));
        for (int sd = 0; sd < 2; sd++) {
            if (!(sd == 0 ? endA : endB)) continue;
            double t = sd == 0 ? t0 : t1;
            Vec3 o, out; frameAt(t, o, out);
            Vec3 along = arc ? B.D(std::cos(ang * t), 0, -std::sin(ang * t)) : B.D(0, 0, 1);
            Vec3 hint = sd == 0 ? along * -1.0 : along;
            // the section in horizontal bands between its distinct heights, each its own trapezoid (B-420: a band was a rectangle of its middle's
            // width: a roof's gable end stood out over its slopes, a bank's end short of its toes)
            std::vector<double> ys; for (auto& q : sec) ys.push_back(std::min(q.second, top)); std::sort(ys.begin(), ys.end()); ys.erase(std::unique(ys.begin(), ys.end(), [](double a, double b) { return std::fabs(a - b) < 1e-6; }), ys.end());
            auto extent = [&](double y, double& rmin, double& rmax) {   // the section's extent at a height: where its edges cross it
                rmin = 1e9; rmax = -1e9;
                for (int i = 0; i < S; i++) {
                    auto p0 = sec[i], p1 = sec[(i + 1) % S];
                    double y0s = std::min(p0.second, top), y1s = std::min(p1.second, top);
                    if ((y - y0s) * (y - y1s) > 0 || std::fabs(y1s - y0s) < 1e-9) continue;
                    double rr = p0.first + (p1.first - p0.first) * (y - y0s) / (y1s - y0s);
                    rmin = std::min(rmin, rr); rmax = std::max(rmax, rr);
                }
            };
            for (size_t k = 0; k + 1 < ys.size(); k++) {
                const double ya = ys[k], yb = ys[k + 1], eps = std::min(1e-4, 0.25 * (yb - ya));
                double la, ha, lb, hb; extent(ya + eps, la, ha); extent(yb - eps, lb, hb);
                if (ha <= la || hb <= lb) continue;
                Vec3 q[4] = {o + out * la + Vec3(0, ya, 0), o + out * ha + Vec3(0, ya, 0), o + out * hb + Vec3(0, yb, 0), o + out * lb + Vec3(0, yb, 0)};
                B.flat(q, 4, hint);
            }
        }
    }
}

void blobMesh(FormMesh& m, const Form& e, int lod, const Vec3& foot, double skirt) {
    Build B(m, e, foot);
    const int rows = lod == 0 ? 8 : (lod == 1 ? 6 : 4), cols = lod == 0 ? 14 : (lod == 1 ? 9 : 6);
    const double lump = clampd(e.p[1] > 0 ? e.p[1] : 0.5, 0, 1);
    const double ph[6] = {TAU * H01(e.seed, 1), TAU * H01(e.seed, 2), TAU * H01(e.seed, 3), TAU * H01(e.seed, 4), TAU * H01(e.seed, 5), TAU * H01(e.seed, 6)};
    auto body = [&](double ra, double rh, double rb, double ca, double cy, double cb, int salt, double cutShare) {
        auto at = [&](int i, int j) {   // i the row (latitude), j the column
            double lat = -PI / 2 + PI * i / rows, lon = TAU * j / cols;
            double k = 1 + lump * 0.22 * (std::sin(2 * lon + ph[salt % 6]) * std::sin(3 * lat + ph[(salt + 1) % 6]) + 0.6 * std::sin(3 * lon + ph[(salt + 2) % 6]) * std::cos(2 * lat + ph[(salt + 3) % 6]));
            double a = ca + ra * k * std::cos(lat) * std::cos(lon), y = cy + rh * k * std::sin(lat), b = cb + rb * k * std::cos(lat) * std::sin(lon);
            y = std::max(y, e.y0 - skirt);
            return Vec3(a, y, b);
        };
        std::vector<Vec3> P((rows + 1) * (cols + 1));
        for (int i = 0; i <= rows; i++) for (int j = 0; j <= cols; j++) P[i * (cols + 1) + j] = at(i, j % cols);
        auto nrm = [&](int i, int j) {
            int ip = std::min(rows, i + 1), im = std::max(0, i - 1);
            Vec3 du = P[i * (cols + 1) + (j + 1) % cols] - P[i * (cols + 1) + (j + cols - 1) % cols];
            Vec3 dv = P[ip * (cols + 1) + j % cols] - P[im * (cols + 1) + j % cols];
            Vec3 n = cross(dv, du);
            if (length2(n) < 1e-12) n = Vec3(0, i > rows / 2 ? 1 : -1, 0);
            Vec3 c(ca, cy, cb), p = P[i * (cols + 1) + j % cols];
            if (dot(n, p - c) < 0) n = n * -1.0;
            return normalize(n);
        };
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++) {
                if (cutShare > 0 && (double)(i + 1) / rows > 1 - cutShare * (0.7 + 0.6 * H01(e.seed, salt * 64 + j))) continue;
                FormMesh::V c[4];
                const int ii[4] = {i, i + 1, i + 1, i}, jj[4] = {j, j, j + 1, j + 1};
                for (int q = 0; q < 4; q++) { const Vec3& p = P[ii[q] * (cols + 1) + jj[q]]; c[q].p = B.P(p.x, p.y, p.z); c[q].n = normalize(B.D(nrm(ii[q], jj[q]).x, nrm(ii[q], jj[q]).y, nrm(ii[q], jj[q]).z)); }
                B.smooth(c, 4, cutShare > 0 ? FormMesh::TWO_SIDED : 0);
            }
    };
    const double Hh = std::max(0.1, e.y1 - e.y0) * 0.5;
    body(e.hx, Hh, e.hz, 0, e.y0 + Hh * 0.9, 0, 0, e.broken > 0 ? 0.55 * e.broken : 0);
    const int lobes = clampi((int)std::lround(e.p[0]), 0, 4);
    for (int l = 0; l < lobes; l++) {
        double ang = TAU * (l + H01(e.seed, 20 + l)) / std::max(1, lobes), sh = 0.38 + 0.25 * H01(e.seed, 30 + l), up = 0.35 + 0.5 * H01(e.seed, 40 + l);
        body(e.hx * sh, Hh * sh * 1.1, e.hz * sh, e.hx * 0.7 * std::cos(ang), e.y0 + Hh * (0.9 + up), e.hz * 0.7 * std::sin(ang), l + 1, 0);
    }
}
}   // namespace

// C-17: a flat body of a drawn outline, its sides upright, its top flat (a fan from its middle) or domed (rings rising to it)
static void plateMesh(FormMesh& m, const Form& e, int lod, const Vec3& foot) {
    Build B(m, e, foot);
    const int n = lod == 0 ? 28 : (lod == 1 ? 16 : 10);
    double a[64], b[64]; formPlateOutline(e, n, a, b);
    double ca = 0, cb = 0; for (int i = 0; i < n; i++) { ca += a[i]; cb += b[i]; } ca /= n; cb /= n;
    const double y0 = e.y0, y1 = std::max(e.y0 + 1e-4, e.y1), dome = std::max(0.0, e.p[3]);
    double na[64], nb[64];   // the outward normals in the plane: the two edges' at each corner, blended
    for (int i = 0; i < n; i++) {
        const int p = (i + n - 1) % n, q = (i + 1) % n;
        double ea = b[q] - b[p], eb = -(a[q] - a[p]);
        if (ea * (a[i] - ca) + eb * (b[i] - cb) < 0) { ea = -ea; eb = -eb; }
        const double l = std::sqrt(ea * ea + eb * eb) + 1e-12; na[i] = ea / l; nb[i] = eb / l;
    }
    for (int i = 0; i < n; i++) {
        const int j = (i + 1) % n;
        FormMesh::V c[4];
        c[0].p = B.P(a[i], y0, b[i]); c[1].p = B.P(a[j], y0, b[j]); c[2].p = B.P(a[j], y1, b[j]); c[3].p = B.P(a[i], y1, b[i]);
        c[0].n = c[3].n = normalize(B.D(na[i], 0, nb[i])); c[1].n = c[2].n = normalize(B.D(na[j], 0, nb[j]));
        B.smooth(c, 4, 0);
    }
    if (dome <= 1e-5) {
        const Vec3 cen = B.P(ca, y1, cb);
        for (int i = 0; i < n; i++) { const int j = (i + 1) % n; Vec3 p[3] = {cen, B.P(a[i], y1, b[i]), B.P(a[j], y1, b[j])}; B.flat(p, 3, B.D(0, 1, 0)); }
        return;
    }
    // the dome: rings at shares of the outline, the height dome (1 - s^2) over the rim, each corner's normal from the slope
    const int R = lod == 0 ? 3 : (lod == 1 ? 2 : 1);
    auto pt = [&](int i, double sh, FormMesh::V& w) {
        const double da = a[i] - ca, db = b[i] - cb, rr = std::sqrt(da * da + db * db) + 1e-9, g = 2 * dome * sh / rr;
        w.p = B.P(ca + da * sh, y1 + dome * (1 - sh * sh), cb + db * sh);
        w.n = normalize(B.D(da / rr * g, 1, db / rr * g));
    };
    for (int k = 0; k < R; k++) {
        const double s0 = 1 - (double)k / R, s1 = 1 - (double)(k + 1) / R;
        for (int i = 0; i < n; i++) {
            const int j = (i + 1) % n;
            FormMesh::V c[4]; pt(i, s0, c[0]); pt(j, s0, c[1]);
            if (k + 1 < R) { pt(j, s1, c[2]); pt(i, s1, c[3]); B.smooth(c, 4, 0); }
            else { pt(i, 0, c[2]); c[2].n = normalize(B.D(0, 1, 0)); B.smooth(c, 3, 0); }
        }
    }
}

namespace {
double fenceTip(const Form& e) { return e.p[3] > 0 && (e.sub == 0 || e.sub == 1) ? std::min(0.18, e.p[3]) : 0.0; }
}   // namespace

// R-412: a fence: its stretches of standing pales, each pale where a pale's spacing spans a pixel and a half, else each stretch one panel (a
// band, as a fence is seen from afar)
static void fenceMesh(FormMesh& m, const Form& e, int lod, const Vec3& foot, double skirt, double pxPerM) {
    Build B(m, e, foot);
    const int n = fencePales(e);
    const double L = e.hz, T = std::max(0.015, e.hx), sp = 2 * L / n, y1 = std::max(e.y0 + 0.1, e.y1), ylo = e.y0 < 0.01 ? e.y0 - skirt : e.y0, tip = fenceTip(e);
    auto box = [&](double a0, double a1, double ya, double yb, double b0, double b1, bool top) {   // its four sides and its top
        Vec3 p[4];
        p[0] = B.P(a1, ya, b0); p[1] = B.P(a1, ya, b1); p[2] = B.P(a1, yb, b1); p[3] = B.P(a1, yb, b0); B.flat(p, 4, B.D(1, 0, 0));
        p[0] = B.P(a0, ya, b0); p[1] = B.P(a0, ya, b1); p[2] = B.P(a0, yb, b1); p[3] = B.P(a0, yb, b0); B.flat(p, 4, B.D(-1, 0, 0));
        p[0] = B.P(a0, ya, b1); p[1] = B.P(a1, ya, b1); p[2] = B.P(a1, yb, b1); p[3] = B.P(a0, yb, b1); B.flat(p, 4, B.D(0, 0, 1));
        p[0] = B.P(a0, ya, b0); p[1] = B.P(a1, ya, b0); p[2] = B.P(a1, yb, b0); p[3] = B.P(a0, yb, b0); B.flat(p, 4, B.D(0, 0, -1));
        if (top) { p[0] = B.P(a0, yb, b0); p[1] = B.P(a1, yb, b0); p[2] = B.P(a1, yb, b1); p[3] = B.P(a0, yb, b1); B.flat(p, 4, B.D(0, 1, 0)); }
    };
    std::vector<std::pair<int, int>> runs;   // the stretches [k0, k1) of standing pales
    for (int k = 0; k < n;) { if (!fenceStanding(e, k)) { k++; continue; } int j = k; while (j < n && fenceStanding(e, j)) j++; runs.push_back({k, j}); k = j; }
    if (!(lod == 0 && sp * pxPerM >= 1.5)) {
        for (const auto& r : runs)   // a broken fence's stretches ragged (lower one by one)
            box(e.sub == 3 ? -T - 0.04 : -T, e.sub == 3 ? T + 0.04 : T, ylo, y1 - 0.5 * tip - (e.broken > 0 ? 0.3 * (y1 - e.y0) * e.broken * H01(e.seed, 950 + r.first) : 0.0), -L + r.first * sp, -L + r.second * sp, true);
        return;
    }
    const double H = y1 - e.y0;
    switch (e.sub) {
        case 1: {   // bars between a bottom and a top rail, spear-tipped
            const double w = std::max(0.012, 0.5 * sp * (e.p[1] > 0 ? e.p[1] : 0.25)), yTop = y1 - tip, rb = std::min(0.1, 0.08 * H);
            for (const auto& r : runs) {
                const double b0 = -L + r.first * sp, b1 = -L + r.second * sp;
                box(-T * 1.4, T * 1.4, yTop - rb, yTop, b0, b1, true);
                box(-T * 1.4, T * 1.4, ylo, e.y0 + rb + 0.04, b0, b1, true);
                for (int k = r.first; k < r.second; k++) {
                    const double bc = -L + (k + 0.5) * sp;
                    box(-w, w, e.y0 + 0.04, yTop - rb, bc - w, bc + w, false);
                    if (tip > 0) {   // a spear's point over the rail
                        const double q = w * 1.6; const Vec3 ap = B.P(0, y1, bc);
                        Vec3 c[4] = {B.P(-q, yTop, bc - q), B.P(q, yTop, bc - q), B.P(q, yTop, bc + q), B.P(-q, yTop, bc + q)};
                        static const double OUT[4][2] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
                        for (int i = 0; i < 4; i++) { Vec3 t[3] = {c[i], c[(i + 1) % 4], ap}; B.flat(t, 3, B.D(OUT[i][0], 0.5, OUT[i][1])); }
                    }
                }
            }
            break;
        }
        case 2: {   // wattle: hurdles between their stakes, their tops ragged where broken
            for (const auto& r : runs)
                for (int k = r.first; k < r.second; k++) {
                    const double b0 = -L + k * sp, b1 = b0 + sp, top = y1 - 0.12 - (e.broken > 0 ? 0.35 * H * e.broken * H01(e.seed, 900 + k) : 0.0);
                    box(-T, T, ylo, top, b0 + 0.05, b1 - 0.05, true);
                    box(-0.05, 0.05, ylo, y1, b0 - 0.05, b0 + 0.05, true);
                    if (k + 1 == r.second) box(-0.05, 0.05, ylo, y1, b1 - 0.05, b1 + 0.05, true);
                }
            break;
        }
        case 3: {   // a balustrade: its plinth, its balusters, its handrail
            const double w = std::max(0.03, 0.5 * sp * (e.p[1] > 0 ? e.p[1] : 0.45)), hp = std::min(0.16, 0.14 * H), hr = std::min(0.14, 0.12 * H);
            for (const auto& r : runs) {
                const double b0 = -L + r.first * sp, b1 = -L + r.second * sp;
                box(-T - 0.04, T + 0.04, ylo, e.y0 + hp, b0, b1, true);
                box(-T - 0.05, T + 0.05, y1 - hr, y1, b0, b1, true);
                for (int k = r.first; k < r.second; k++) { const double bc = -L + (k + 0.5) * sp; box(-T * 0.8, T * 0.8, e.y0 + hp, y1 - hr, bc - w, bc + w, false); }
            }
            break;
        }
        default: {   // pales on two rails behind them, pointed
            const double w = std::max(0.02, 0.5 * sp * (e.p[1] > 0 ? e.p[1] : 0.6));
            for (const auto& r : runs) {
                const double b0 = -L + r.first * sp, b1 = -L + r.second * sp;
                for (int q = 0; q < 2; q++) { const double yr = e.y0 + H * (q ? 0.72 : 0.22); box(-T - 0.05, -T, yr - 0.04, yr + 0.04, b0, b1, true); }
                for (int k = r.first; k < r.second; k++) {
                    const double bc = -L + (k + 0.5) * sp, ys = y1 - tip;
                    box(-T, T, ylo, ys, bc - w, bc + w, tip <= 0);
                    if (tip > 0) {   // its point: two slopes to a ridge across it, a gable each side
                        Vec3 q1[4] = {B.P(-T, ys, bc - w), B.P(T, ys, bc - w), B.P(T, y1, bc), B.P(-T, y1, bc)}; B.flat(q1, 4, B.D(0, 1, -1));
                        Vec3 q2[4] = {B.P(-T, ys, bc + w), B.P(T, ys, bc + w), B.P(T, y1, bc), B.P(-T, y1, bc)}; B.flat(q2, 4, B.D(0, 1, 1));
                        Vec3 t1[3] = {B.P(T, ys, bc - w), B.P(T, ys, bc + w), B.P(T, y1, bc)}; B.flat(t1, 3, B.D(1, 0, 0));
                        Vec3 t2[3] = {B.P(-T, ys, bc - w), B.P(-T, ys, bc + w), B.P(-T, y1, bc)}; B.flat(t2, 3, B.D(-1, 0, 0));
                    }
                }
            }
            break;
        }
    }
}

void formPlateOutline(const Form& f, int n, double* a, double* b) {
    n = std::max(3, std::min(64, n));
    const double sq = f.p[2] > 0 ? f.p[2] : 2.0, ex = 2.0 / std::max(0.6, sq);
    const double far = f.p[0] > 0 ? f.p[0] : 1.0, wmax = std::max(1.0, far), waist = clampd(f.p[1], -0.6, 0.7), skew = 0.1 * f.sub;
    double gap0 = 0, gapW = -1;   // a broken plate's bite
    if (f.broken > 0) { gap0 = TAU * H01(f.seed, 7); gapW = 0.4 + 1.2 * f.broken; }
    const int sides = f.sides >= 3 ? std::min(f.sides, 12) : 0;
    for (int k = 0; k < n; k++) {
        const double th = TAU * k / n, c = std::cos(th), s = std::sin(th);
        double t, q;
        if (sides) {   // a regular polygon, a flat side toward the far end
            const double seg = TAU / sides, off = PI / sides, rel = std::fmod(th - off + 8 * TAU, seg) - seg * 0.5;
            const double r = std::cos(PI / sides) / std::cos(rel);
            t = r * c; q = r * s;
        } else { t = (c < 0 ? -1 : 1) * std::pow(std::fabs(c), ex); q = (s < 0 ? -1 : 1) * std::pow(std::fabs(s), ex); }
        const double wf = (1 + (far - 1) * (t + 1) * 0.5) / wmax, z = (t - 0.12) / 0.34, wz = 1 - waist * std::exp(-z * z);
        a[k] = f.hx * (q * wf * wz + skew * (1 - t * t));
        b[k] = f.hz * t;
        if (gapW > 0) { const double d = std::fmod(th - gap0 + 8 * TAU, TAU); if (d < gapW) { const double bite = 1 - 0.45 * f.broken * std::sin(PI * d / gapW); a[k] *= bite; b[k] *= bite; } }
    }
}

void formMesh(const Form& f, int lod, const Vec3& foot, double skirt, FormMesh& out, double pxPerM) {
    out.clear();
    switch (f.kind) {
        case FK_BOX: boxMesh(out, f, foot, skirt); break;
        case FK_DOME: domeMesh(out, f, lod, foot); break;
        case FK_LATHE: latheMesh(out, f, lod, foot, skirt); break;
        case FK_PYRAMID: pyramidMesh(out, f, foot, skirt); break;
        case FK_VAULT: vaultMesh(out, f, lod, foot, skirt); break;
        case FK_ARCH: archMesh(out, f, lod, foot, skirt); break;
        case FK_SHELL: shellMesh(out, f, lod, foot); break;
        case FK_TRUSS: trussMesh(out, f, lod, foot, skirt, pxPerM); break;
        case FK_CABLE: cableMesh(out, f, lod, foot); break;
        case FK_TORUS: torusMesh(out, f, lod, foot); break;
        case FK_SWEEP: sweepMesh(out, f, lod, foot, skirt); break;
        case FK_BLOB: blobMesh(out, f, lod, foot, skirt); break;
        case FK_PLATE: plateMesh(out, f, lod, foot); break;
        case FK_FENCE: fenceMesh(out, f, lod, foot, skirt, pxPerM); break;
        default: break;
    }
    if (f.tilt > 0.3 && f.kind != FK_TORUS && f.kind != FK_SHELL && !out.v.empty()) {   // lying on its side: resting on the ground
        double minY = 1e18; for (const FormMesh::V& v : out.v) minY = std::min(minY, v.p.y);
        for (const FormMesh::Line& l : out.lines) minY = std::min(minY, std::min(l.a.y, l.b.y));
        double dy = foot.y + f.y0 - minY;
        for (FormMesh::V& v : out.v) v.p.y += dy;
        for (FormMesh::Line& l : out.lines) { l.a.y += dy; l.b.y += dy; }
    }
}

// ---- the silhouette ---------------------------------------------------------------------------------------------------------

double formProfile(const Form& e, double t) {
    t = clampd(t, 0, 1);
    switch (e.kind) {
        case FK_DOME: {
            if (e.sub == 2) return std::pow(std::max(0.0, 1 - std::pow(t, 1.6)), 0.7);
            if (e.sub == 3) { static const double OR[] = {1.0, 1.12, 1.16, 1.08, 0.86, 0.55, 0.26, 0.08, 0.0}, OY[] = {0.0, 0.12, 0.25, 0.38, 0.52, 0.66, 0.8, 0.92, 1.0}; for (int k = 0; k < 8; k++) if (t <= OY[k + 1]) return (OR[k] + (OR[k + 1] - OR[k]) * (t - OY[k]) / (OY[k + 1] - OY[k])) / 1.16; return 0; }
            return std::sqrt(std::max(0.0, 1 - t * t));
        }
        case FK_LATHE: {
            switch (e.sub) {
                case 1: return 1 - (1 - std::max(0.0, e.p[0])) * t;
                case 2: return std::pow(1 - t, e.p[0] > 0 ? e.p[0] : 1.0);
                case 3: { double w = e.p[0] > 0 ? e.p[0] : 0.62, tw = e.p[1] > 0 ? e.p[1] : 0.72, q = (t - tw) / tw; return std::sqrt(w * w + (1 - w * w) * q * q); }
                case 5: { double top = e.p[0] > 0 ? e.p[0] : 0.65; return 1 - (1 - top) * t; }
                default: return 1;
            }
        }
        case FK_PYRAMID: {
            int T = std::max(1, (int)std::lround(e.p[0])); double top = clampd(e.p[1], 0, 1), rs = e.p[0] > 0 ? clampd(e.p[2], 0, 1) : 0.0;
            int i = std::min(T - 1, (int)(t * T)); double f = t * T - i;
            double s0 = 1 - (1 - top) * i / T, s1 = 1 - (1 - top) * (i + 1) / T, sTop = s0 - (s0 - s1) * (1 - rs);
            return s0 + (sTop - s0) * f;
        }
        case FK_TRUSS: return e.sub == 0 ? 1 - (1 - (e.p[0] > 0 ? e.p[0] : 0.3)) * t : 1;
        case FK_TORUS: return e.tilt > 0.3 ? std::sqrt(std::max(0.0, 1 - (2 * t - 1) * (2 * t - 1))) : 1;
        case FK_BLOB: { double q = (t - 0.45) / 0.55; return std::sqrt(std::max(0.0, 1 - q * q)); }
        case FK_CABLE: return 0;
        default: return 1;
    }
}

double formHalfWidth(const Form& e, double sightHeading) {
    const double d = sightHeading - e.heading, c = std::fabs(std::cos(d)), s = std::fabs(std::sin(d));
    double hz = e.hz > 0 ? e.hz : e.hx;
    switch (e.kind) {
        case FK_DOME: case FK_LATHE: case FK_BLOB: case FK_TORUS: { double hx = e.kind == FK_TORUS ? e.hx + (e.p[0] > 0 ? e.p[0] : 0.15 * e.hx) : e.hx; if (e.kind == FK_TORUS) hz = hx; return std::sqrt(hx * hx * c * c + hz * hz * s * s); }
        case FK_SWEEP: return e.sub == 0 ? e.hx + std::max(e.p[1], e.p[2]) : e.hx * c + e.hz * s;
        case FK_CABLE: return 0;
        default: return e.hx * c + hz * s;
    }
}

double formTop(const Form& e) {
    switch (e.kind) {
        case FK_TORUS: { double r = e.p[0] > 0 ? e.p[0] : 0.15 * e.hx; return e.y0 + (e.tilt > 0.3 ? 2 * (e.hx + r) : 2 * r); }
        case FK_BLOB: return e.y0 + (e.y1 - e.y0) * (e.p[0] >= 1 ? 1.25 : 1.08);   // its lumps, and the lobes riding on it
        case FK_CABLE: return std::max(e.y1, e.p[2]);
        case FK_SHELL: return e.sub == 1 ? e.y0 + (e.p[0] > 0 ? e.p[0] : 0.3 * e.hx) + e.hx * std::sin(std::fabs(e.tilt)) : e.y1;
        case FK_PLATE: return e.y1 + std::max(0.0, e.p[3]);
        default: break;
    }
    // tipped about the foot's across axis: the box (a, y0..y1, b) rises to y cos t - b sin t; lying, it was set down on its lowest point
    const double ct = std::cos(e.tilt), st = std::fabs(std::sin(e.tilt)), hz = std::max(e.hx, e.hz);
    if (e.tilt > 0.3) return e.y0 + (e.y1 - e.y0) * ct + 2 * hz * st;
    return e.y1 * ct + hz * st;
}

double formReach(const Form& e) {
    double r;
    switch (e.kind) {
        case FK_DOME: case FK_LATHE: case FK_BLOB: r = std::max(e.hx, e.hz) * 1.2; break;
        case FK_PLATE: r = std::hypot(e.hx * (1 + 0.1 * std::abs(e.sub)) * std::max(1.0, 1 - e.p[1]), e.hz); break;
        case FK_TORUS: r = e.hx + (e.p[0] > 0 ? e.p[0] : 0.15 * e.hx); break;
        case FK_SWEEP: r = e.sub == 0 ? e.hx + std::max(e.p[1], e.p[2]) : std::hypot(e.hx + std::max(e.p[1], e.p[2]), e.hz); break;
        case FK_CABLE: r = std::hypot(e.p[0] - e.x, e.p[1] - e.z); break;
        case FK_TRUSS: r = std::hypot(e.hx, e.hz) + (e.sub == 0 && e.broken > 0 ? 0.45 * (e.y1 - e.y0) * e.broken : 0); break;   // a bent mast's top swings out
        default: r = std::hypot(e.hx, e.hz); break;
    }
    if (e.tilt != 0) r += (e.y1 - e.y0) * std::fabs(std::sin(e.tilt));
    return r;
}
