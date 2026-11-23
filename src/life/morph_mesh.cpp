// Z-02: the electron's way of seeing (life/morph_draw.h): the cell as a mesh lit from one side, as a scanning microscope's secondary
// electrons show it (the edges bright, the detector's side lit, pores pits and plates ridged), on a filter's grey with its pores, the
// view turned about the slide's normal and tilted toward the eye. One bank: the electron's grey
#include "life/morph_shape.h"
#include "core/raster.h"
#include "core/parallel.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

namespace morph {

namespace {
inline double h01(uint64_t h) { return unitFromHash(mix64(h)); }   // re-mixed: a shifted hash would span [0, 2^-n) alone

struct View {
    double cx, cy, pz, ct, st, cr, sr, D;
    int step = 1;   // Z-04: the slide's pixel step: a mesh half as fine at 2
    Proj pj;
    // a slide point (um, its height z over the slide) into view space
    Vec3 toView(double X, double Y, double Z) const {
        X -= cx; Y -= cy;
        const double X1 = X * cr - Y * sr, Y1 = X * sr + Y * cr;
        const double Y2 = Y1 * ct - Z * st, Z2 = Y1 * st + Z * ct;
        return Vec3(X1, -Y2, D - Z2);
    }
};

// the light on a face: the detector's side lit, the edges bright (the electrons escape where the beam grazes), a little ambient
double semShade(const Vec3& n) {
    static const Vec3 L = normalize(Vec3(-0.55, 0.62, -0.56));
    const double lam = std::max(0.0, dot(n, L));
    const double facing = std::fabs(n.z);
    return 9 + 24 * lam + 30 * std::pow(1 - facing, 2.2);
}

struct Mesh {
    std::vector<Vec3> p;     // view space
    std::vector<double> rel; // a vertex's relief (pits darker)
    std::vector<bool> hole;
    std::vector<double> shade;   // Z-04: each vertex's light, made with the mesh (`shadeGrid`)
    std::vector<float> y0, y1;   // Z-04: the screen rows a row of its quads reaches (a band skips the rows it misses)
    int cols = 0, rows = 0;
    bool wrap = true;
};

// Z-04: a mesh's vertices' light from their normals and its quads' rows on the screen (made with the mesh, on the thread that built it); its
// triangles drawn after (`drawGrid`)
void shadeGrid(Mesh& M, bool wrapCols, const Proj& pj) {
    const int C = M.cols, Rn = M.rows;
    M.wrap = wrapCols;
    M.y0.assign(std::max(0, Rn - 1), 1e30f); M.y1.assign(std::max(0, Rn - 1), -1e30f);
    for (int j = 0; j < Rn; j++)
        for (int i = 0; i < C; i++) {
            RVert q; const Vec3& p = M.p[(size_t)j * C + i]; q.x = p.x; q.y = p.y; q.z = p.z;
            double sx, sy;
            const bool in = projectPoint(q, pj, sx, sy);
            for (int r : {j - 1, j}) {
                if (r < 0 || r >= Rn - 1) continue;
                if (!in) { M.y0[r] = -1e30f; M.y1[r] = 1e30f; continue; }
                M.y0[r] = std::min(M.y0[r], (float)sy); M.y1[r] = std::max(M.y1[r], (float)sy);
            }
        }
    auto at = [&](int i, int j) { return j * C + ((i % C) + C) % C; };
    std::vector<double>& shade = M.shade;
    shade.assign(M.p.size(), 0);
    for (int j = 0; j < Rn; j++)
        for (int i = 0; i < C; i++) {
            const int k = at(i, j);
            const Vec3& a = M.p[k];
            const Vec3 b = M.p[at(wrapCols || i + 1 < C ? i + 1 : i, j)], c = M.p[at(wrapCols || i > 0 ? i - 1 : i, j)];
            const Vec3 d = M.p[at(i, std::min(Rn - 1, j + 1))], e = M.p[at(i, std::max(0, j - 1))];
            Vec3 n = normalize(cross(b - c, d - e));
            const Vec3 ctr = a;
            (void)ctr;
            if (n.z > 0) n = n * -1.0;   // toward the eye
            shade[k] = clampd(semShade(n) + M.rel[k], 1, 63);
        }
}
void drawGrid(Framebuffer& fb, const View& V, const Mesh& M, int bandY0, int bandY1) {
    RasterParams rp; rp.bank = SB_FIELD; rp.ztest = true; rp.zwrite = true;
    const int C = M.cols, Rn = M.rows;
    auto at = [&](int i, int j) { return j * C + ((i % C) + C) % C; };
    const std::vector<double>& shade = M.shade;
    const int cEnd = M.wrap ? C : C - 1;
    for (int j = 0; j + 1 < Rn; j++) {
        if (M.y1[j] < bandY0 - 2 || M.y0[j] > bandY1 + 2) continue;   // its quads reach no row of the band
        for (int i = 0; i < cEnd; i++) {
            const int a = at(i, j), b = at(i + 1, j), c = at(i + 1, j + 1), d = at(i, j + 1);
            if ((int)M.hole[a] + (int)M.hole[b] + (int)M.hole[c] + (int)M.hole[d] >= 2) continue;
            RVert t1[3], t2[3];
            const int ix[4] = {a, b, c, d};
            RVert q[4];
            for (int k = 0; k < 4; k++) { q[k].x = M.p[ix[k]].x; q[k].y = M.p[ix[k]].y; q[k].z = M.p[ix[k]].z; q[k].shade = shade[ix[k]]; }
            t1[0] = q[0]; t1[1] = q[1]; t1[2] = q[2];
            t2[0] = q[0]; t2[1] = q[2]; t2[2] = q[3];
            rasterTriangle(fb, t1, rp, V.pj);
            rasterTriangle(fb, t2, rp, V.pj);
        }
    }
}

// a radial body as a solid: each latitude the outline drawn in toward its centre, its height the body's zScale; the ornament pits and ridges it
void radialMesh(const View& V, const Body& b, Mesh& M) {
    M.cols = M.rows = 0;
    if (b.look.gel) return;
    const double perPx = 2 * PI * b.R * std::max(1.0, b.aspect) / (V.pz * V.step);
    const bool marked = b.look.mark != MK_SMOOTH;
    const int C = (int)clampd(perPx / 2.5, 24, marked ? 256 : 128), Rn = (int)clampd(C / (marked ? 2.2 : 3.5), 8, marked ? 96 : 40);
    M.cols = C; M.rows = Rn;
    M.p.resize((size_t)C * Rn); M.rel.assign(M.p.size(), 0); M.hole.assign(M.p.size(), false);
    const double L = b.R * b.aspect, zs = b.R * b.look.zScale, lift = b.lift >= 0 ? b.lift + zs : zs;
    for (int j = 0; j < Rn; j++) {
        const double th = -PI * 0.5 + PI * (j + 0.5) / Rn;
        const double cth = std::cos(th), sth = std::sin(th);
        for (int i = 0; i < C; i++) {
            const double phi = i * TAU / C;
            const double rr = b.R * outlineAt(b, phi);
            double u = rr * std::cos(phi) * b.aspect, qy = rr * std::sin(phi);
            double sc = 1;
            if (b.tail > 0 && u < 0) sc = 1 - b.tail * 0.82 * std::pow(std::min(1.0, -u / L), 1.4);
            else if (b.head > 0 && u > 0) sc = 1 - b.head * 0.82 * std::pow(std::min(1.0, u / L), 1.4);
            double v = qy * std::max(0.1, sc);
            u *= cth; v *= cth;
            v += b.bend * u * u / L;
            double z = zs * sth;
            // the ornament: the light ways' pattern on the surface (the top's for a flat body)
            double orn = 0;
            if (marked) {
                double uu, vv, ph;
                const double X = b.c.x + u * b.ca - v * b.sa, Y = b.c.y + u * b.sa + v * b.ca;
                radialDistance(b, X, Y, uu, vv, ph);
                if (b.look.layout == 0) orn = markAt(b.look.mark, b.look.marks, b.look.markSeed, uu / std::max(1e-9, L) * b.aspect * 0.5 + uu / L * 0.5, vv / b.R, sth);
                else if (sth > 0.2) orn = flatOrnament(b, uu, vv);   // the flat patterns on its top: centric rows, pennate striae, stripes
            }
            const double disp = orn < 0 ? -0.07 * b.R : (orn > 0 ? 0.035 * b.R * orn : 0);
            const double nx = std::cos(phi) * cth, ny = std::sin(phi) * cth, nz = sth;
            u += nx * disp * 0.6; v += ny * disp * 0.6; z += nz * disp;
            if (b.look.gapW > 0) { const double dp = wrapAngle(phi - b.look.gapPhi); if (dp * dp + th * th < b.look.gapW * b.look.gapW * 0.5) M.hole[(size_t)j * C + i] = true; }
            if (b.look.mark == MK_LATTICE && orn < 0) M.hole[(size_t)j * C + i] = true;
            const double X = b.c.x + u * b.ca - v * b.sa, Y = b.c.y + u * b.sa + v * b.ca;
            M.p[(size_t)j * C + i] = V.toView(X, Y, z + lift);
            M.rel[(size_t)j * C + i] = orn < 0 ? -10 : (orn > 0 ? 4 : 0);
        }
    }
    shadeGrid(M, true, V.pj);
}

// a tube swept round its centreline, capped, lying on the slide (its height the electron's helix)
void tubeMesh(const View& V, const Body& b, Mesh& M) {
    M.cols = M.rows = 0;
    if (b.look.gel) return;
    const int n = (int)b.line.size();
    if (n < 2) return;
    double maxR = 0; for (float r : b.rad) maxR = std::max(maxR, (double)r);
    const double minR = V.pz * 0.9;   // thin threads stay a pixel or two wide
    const double grow = maxR < minR ? minR / std::max(1e-9, maxR) : 1.0;
    maxR *= grow;
    const int around = (int)clampd(2 * PI * maxR / V.pz / 2.5, 10, 32);
    std::vector<P2> pts; std::vector<double> rr, zz;
    auto tangent = [&](int i) { const int a = std::max(0, i - 1), c = std::min(n - 1, i + 1); double tx = b.line[c].x - b.line[a].x, ty = b.line[c].y - b.line[a].y; const double l = std::max(1e-12, std::sqrt(tx * tx + ty * ty)); return P2{tx / l, ty / l}; };
    // the rear cap, the body, the front cap
    const int capN = 5;
    for (int k = capN; k >= 1; k--) {
        const double a = PI * 0.5 * k / capN; const P2 t = tangent(0);
        pts.push_back({b.line[0].x - t.x * b.rad[0] * std::sin(a), b.line[0].y - t.y * b.rad[0] * std::sin(a)}); rr.push_back(b.rad[0] * std::cos(a)); zz.push_back(b.z.empty() ? 0 : b.z[0]);
    }
    for (int i = 0; i < n; i++) { pts.push_back(b.line[i]); rr.push_back(b.rad[i]); zz.push_back(b.z.empty() ? 0 : b.z[i]); }
    for (int k = 1; k <= capN; k++) {
        const double a = PI * 0.5 * k / capN; const P2 t = tangent(n - 1);
        pts.push_back({b.line[n - 1].x + t.x * b.rad[n - 1] * std::sin(a), b.line[n - 1].y + t.y * b.rad[n - 1] * std::sin(a)}); rr.push_back(b.rad[n - 1] * std::cos(a)); zz.push_back(b.z.empty() ? 0 : b.z[n - 1]);
    }
    const int rows = (int)pts.size();
    M.cols = around; M.rows = rows;
    M.p.resize((size_t)around * rows); M.rel.assign(M.p.size(), 0); M.hole.assign(M.p.size(), false);
    double along = 0;
    for (int j = 0; j < rows; j++) {
        const int i = std::max(0, std::min(n - 1, j - capN));
        const P2 t = tangent(i), nr = {-t.y, t.x};
        rr[j] *= grow;
        if (j > 0) along += std::sqrt((pts[j].x - pts[j - 1].x) * (pts[j].x - pts[j - 1].x) + (pts[j].y - pts[j - 1].y) * (pts[j].y - pts[j - 1].y));
        for (int k = 0; k < around; k++) {
            const double a = k * TAU / around;
            double r = rr[j];
            double orn = 0;
            if (b.look.mark != MK_SMOOTH) {
                if (b.look.layout == 3 || b.look.layout == 4) { const double f = (std::cos(a) + (b.look.layout == 4 ? along / std::max(1e-9, 2 * maxR) * 0.7 : 0)) * b.look.marks * 0.25; orn = std::fabs(f - std::round(f)) < 0.14 ? 1 : 0; }
                else orn = markAt(b.look.mark, b.look.marks, b.look.markSeed, along / std::max(1e-9, 2 * maxR), std::cos(a), std::sin(a));
                r *= orn < 0 ? 0.94 : (orn > 0 ? 1.03 : 1);
            }
            const double X = pts[j].x + nr.x * r * std::cos(a), Y = pts[j].y + nr.y * r * std::cos(a);
            const double Z = zz[j] + maxR + r * std::sin(a);
            M.p[(size_t)j * around + k] = V.toView(X, Y, Z);
            M.rel[(size_t)j * around + k] = orn < 0 ? -9 : (orn > 0 ? 4 : 0);
        }
    }
    shadeGrid(M, true, V.pj);
}

// a strand as a thin tube (hairs and pili as lines), lifted to a body's height
void strandMesh(const View& V, const Strand& s, double lift, Mesh& M, std::vector<RVert>& lines) {
    M.cols = M.rows = 0; lines.clear();
    const int n = (int)s.pts.size();
    if (n < 2) return;
    const double minR = V.pz * 0.9;
    if (s.kind == ST_PILUS || s.kind == ST_HAIR || (std::max(s.w0, s.w1) * 0.5 < minR * 0.6 && s.kind != ST_SPINE)) {
        for (int i = 0; i + 1 < n; i++) {
            const Vec3 a = V.toView(s.pts[i].x, s.pts[i].y, lift), b = V.toView(s.pts[i + 1].x, s.pts[i + 1].y, lift);
            RVert ra, rb; ra.x = a.x; ra.y = a.y; ra.z = a.z - V.pz * 0.5; ra.shade = 46; rb.x = b.x; rb.y = b.y; rb.z = b.z - V.pz * 0.5; rb.shade = 46;
            lines.push_back(ra); lines.push_back(rb);
        }
        return;
    }
    const int around = 8;
    M.cols = around; M.rows = n;
    M.p.resize((size_t)around * n); M.rel.assign(M.p.size(), 6); M.hole.assign(M.p.size(), false);
    for (int j = 0; j < n; j++) {
        const int a = std::max(0, j - 1), c = std::min(n - 1, j + 1);
        double tx = s.pts[c].x - s.pts[a].x, ty = s.pts[c].y - s.pts[a].y;
        const double l = std::max(1e-12, std::sqrt(tx * tx + ty * ty)); tx /= l; ty /= l;
        const double r = std::max(minR, (s.w0 + (s.w1 - s.w0) * j / (n - 1.0)) * 0.5);
        for (int k = 0; k < around; k++) {
            const double ang = k * TAU / around;
            M.p[(size_t)j * around + k] = V.toView(s.pts[j].x - ty * r * std::cos(ang), s.pts[j].y + tx * r * std::cos(ang), lift + r * std::sin(ang));
        }
    }
    shadeGrid(M, true, V.pj);
}

// Z-04: what the electron draws, in the order its shapes come: a body's mesh, a strand's (or its lines), a colony's cell on its sphere, a grain's
// point; built each on a thread of the pool (its own slot), then drawn in row bands in this order (each pixel the same writes as one by one)
struct EItem {
    int kind = 0;                  // 0 a body, 1 a strand, 2 a colony's cell, 3 a grain
    const Body* body = nullptr; const Strand* strand = nullptr; Body cell; double lift = 0;
    RVert point; int size = 1;
    Mesh M; std::vector<RVert> lines;
};
}   // namespace

void drawElectron(Framebuffer& fb, const Slide& v, const std::vector<Shape>& shapes) {
    View V;
    V.cx = v.cx; V.cy = v.cy; V.pz = v.umPerPx / FB_SCALE; V.step = v.step >= 2 ? 2 : 1;
    V.ct = std::cos(v.tilt); V.st = std::sin(v.tilt); V.cr = std::cos(v.turn); V.sr = std::sin(v.turn);
    V.D = V.pz * FBW * 400;
    V.pj.f = V.D / V.pz; V.pj.cx = FBW * 0.5; V.pj.cy = FBH * 0.5;
    // the stub: a filter's grey and its pores, on the tilted plane (its depth the plane's). B-502 (KI-437): each pore's cell and each grain's
    // corner hashed once a frame into a table over the view's span (a pixel's sums as they were, bit for bit), not nine and four times a pixel
    const int W = FBW, H = FBH;
    const double pz = V.pz;
    const double gs = 1.1, gsz = std::max(0.15, pz * 3);
    struct Pore { double px, py, rad; bool live; };
    auto poreOf = [&](int64_t a, int64_t b) {
        const uint64_t h = hash2i(a, b, v.seed ^ 0xF117E5ULL);
        Pore p;
        p.live = !(h01(h >> 3) < 0.35);
        p.px = (a + h01(h)) * gs; p.py = (b + h01(h >> 17)) * gs;
        p.rad = 0.1 + 0.12 * h01(h >> 29);
        return p;
    };
    auto grainOf = [&](int64_t a, int64_t b) { return h01(hash2i(a, b, v.seed)); };
    // the view's span on the slide: a point's X and Y are affine in its pixel, so the frame's corners bound them (the tables two cells wider)
    double xlo = 1e300, xhi = -1e300, ylo = 1e300, yhi = -1e300;
    for (int k = 0; k < 4; k++) {
        const int x = (k & 1) ? W - 1 : 0, y = (k & 2) ? H - 1 : 0;
        const double xv = (x + 0.5 - V.pj.cx) * pz, yv = -(y + 0.5 - V.pj.cy) * pz;
        const double Y2 = -yv, Y1 = Y2 / std::max(0.2, V.ct), X1 = xv;
        const double X = X1 * V.cr + Y1 * V.sr + v.cx, Y = -X1 * V.sr + Y1 * V.cr + v.cy;
        xlo = std::min(xlo, X); xhi = std::max(xhi, X); ylo = std::min(ylo, Y); yhi = std::max(yhi, Y);
    }
    struct Table { int64_t a0 = 0, b0 = 0, na = 0, nb = 0; };
    auto span = [&](double cell) {   // a table's cells over the span, none past a cell a pixel
        Table T;
        T.a0 = (int64_t)std::floor(xlo / cell) - 2; T.b0 = (int64_t)std::floor(ylo / cell) - 2;
        T.na = (int64_t)std::floor(xhi / cell) + 3 - T.a0; T.nb = (int64_t)std::floor(yhi / cell) + 3 - T.b0;
        if (T.na <= 0 || T.nb <= 0 || T.na * T.nb > (int64_t)W * H) T.na = T.nb = 0;
        return T;
    };
    const Table PT = span(gs), GT = span(gsz);
    thread_local std::vector<Pore> pores;
    thread_local std::vector<double> grains;
    pores.resize((size_t)(PT.na * PT.nb)); grains.resize((size_t)(GT.na * GT.nb));
    Pore* pt = pores.data(); double* gt = grains.data();
    parallelFor((int)PT.nb, 16, [&](int ja, int jb) {
        for (int j = ja; j < jb; j++) for (int64_t i = 0; i < PT.na; i++) pt[j * PT.na + i] = poreOf(PT.a0 + i, PT.b0 + j);
    });
    parallelFor((int)GT.nb, 16, [&](int ja, int jb) {
        for (int j = ja; j < jb; j++) for (int64_t i = 0; i < GT.na; i++) gt[j * GT.na + i] = grainOf(GT.a0 + i, GT.b0 + j);
    });
    auto pore = [&](int64_t a, int64_t b) {
        const int64_t i = a - PT.a0, j = b - PT.b0;
        return i >= 0 && j >= 0 && i < PT.na && j < PT.nb ? pt[j * PT.na + i] : poreOf(a, b);
    };
    auto grainAt = [&](int64_t a, int64_t b) {
        const int64_t i = a - GT.a0, j = b - GT.b0;
        return i >= 0 && j >= 0 && i < GT.na && j < GT.nb ? gt[j * GT.na + i] : grainOf(a, b);
    };
    parallelFor(H, 16, [&](int ya, int yb) {
        for (int y = ya; y < yb; y++)
            for (int x = 0; x < W; x++) {
                const double xv = (x + 0.5 - V.pj.cx) * pz, yv = -(y + 0.5 - V.pj.cy) * pz;
                const double Y2 = -yv, Y1 = Y2 / std::max(0.2, V.ct), X1 = xv;
                const double X = X1 * V.cr + Y1 * V.sr + v.cx, Y = -X1 * V.sr + Y1 * V.cr + v.cy;
                const int64_t ix = (int64_t)std::floor(X / gs), iy = (int64_t)std::floor(Y / gs);
                double best = 1e9; double rad = 0.15;
                for (int64_t a = ix - 1; a <= ix + 1; a++)
                    for (int64_t b = iy - 1; b <= iy + 1; b++) {
                        const Pore p = pore(a, b);
                        if (!p.live) continue;
                        const double px = p.px, py = p.py;
                        const double d = std::sqrt((px - X) * (px - X) + (py - Y) * (py - Y));
                        if (d < best) { best = d; rad = p.rad; }
                    }
                // the filter's grain, a smooth noise sized in screen pixels (no squares at any zoom)
                const double fx = X / gsz, fy = Y / gsz;
                const int64_t qx = (int64_t)std::floor(fx), qy = (int64_t)std::floor(fy);
                const double tx = fx - qx, ty = fy - qy, sx = tx * tx * (3 - 2 * tx), sy = ty * ty * (3 - 2 * ty);
                auto g = [&](int64_t a, int64_t b) { return grainAt(a, b); };
                const double grain = 3 * ((g(qx, qy) * (1 - sx) + g(qx + 1, qy) * sx) * (1 - sy) + (g(qx, qy + 1) * (1 - sx) + g(qx + 1, qy + 1) * sx) * sy);
                double s = 13 + grain;
                const double ring = std::max(pz, 0.03);
                if (best < rad) s = 4; else if (best < rad + ring) s = 24;
                fb.idx[(size_t)y * W + x] = pix(SB_FIELD, s);
                const double Z2 = Y1 * V.st;
                fb.invz[(size_t)y * W + x] = (float)(1.0 / (V.D - Z2));
            }
    });
    // Z-04: what the shapes are made into, in their order (the scratch kept by the thread: its meshes' room reused frame to frame), built on the
    // pool, then drawn band by band
    thread_local std::vector<EItem> scratch;
    std::vector<EItem>& items = scratch;   // (the pool's threads read the caller's, not their own)
    size_t n = 0;
    auto next = [&]() -> EItem& { if (n == items.size()) items.emplace_back(); EItem& e = items[n++]; e.body = nullptr; e.strand = nullptr; return e; };
    for (const Shape& sh : shapes) {
        double lift = 0;
        for (const Body& b : sh.bodies) lift = std::max(lift, b.kind == BODY_RADIAL ? b.R * b.look.zScale : (double)(b.rad.empty() ? 0 : b.rad[0]));
        for (const Body& b : sh.bodies) { EItem& e = next(); e.kind = 0; e.body = &b; }
        for (const Strand& st : sh.strands) { EItem& e = next(); e.kind = 1; e.strand = &st; e.lift = lift * 0.9; }
        for (const Dot& d : sh.dots) {   // what sits on the surface: a holdfast, a grain, a colony's cell on its sphere
            if (d.lift >= 0) {
                if (d.lift < 0.9 * d.rx) continue;   // its far side, under the sphere
                EItem& e = next(); e.kind = 2;
                Body& cell = e.cell;
                cell = Body(); cell.kind = BODY_RADIAL; cell.c = d.c; cell.R = d.rx; cell.aspect = 1; cell.lift = (float)(d.lift - d.rx);
                for (float& r : cell.r) r = 1;
                continue;
            }
            if (d.kind != DOT_GRAIN) continue;
            EItem& e = next(); e.kind = 3;
            const Vec3 c = V.toView(d.c.x, d.c.y, lift * 0.5);
            e.point.x = c.x; e.point.y = c.y; e.point.z = c.z - lift; e.point.shade = 40;
            e.size = std::max(1, (int)(d.rx / pz / FB_SCALE));
        }
    }
    parallelFor((int)n, 1, [&](int a, int b) {
        for (int i = a; i < b; i++) {
            EItem& e = items[i];
            e.lines.clear(); e.M.cols = e.M.rows = 0;
            switch (e.kind) {
                case 0: if (e.body->kind == BODY_RADIAL) radialMesh(V, *e.body, e.M); else tubeMesh(V, *e.body, e.M); break;
                case 1: strandMesh(V, *e.strand, e.lift, e.M, e.lines); break;
                case 2: radialMesh(V, e.cell, e.M); break;
                default: break;
            }
        }
    }, 1);
    parallelFor(H, 32, [&](int yb, int ye) {
        setRasterBand(yb, ye);
        for (size_t i = 0; i < n; i++) {
            const EItem& e = items[i];
            if (e.kind == 3) { rasterPoint3(fb, e.point, SB_FIELD, V.pj, true, e.size); continue; }
            for (size_t k = 0; k + 1 < e.lines.size(); k += 2) rasterLine3(fb, e.lines[k], e.lines[k + 1], SB_FIELD, V.pj, true, 1);
            if (e.M.cols > 0 && e.M.rows > 0) drawGrid(fb, V, e.M, yb, ye);
        }
        setRasterBand(0, 1 << 30);
    });
}

}   // namespace morph
