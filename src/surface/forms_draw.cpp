// A-02: the settlements' forms drawn (galaxy/forms.h). The cells in reach are gathered building by building, each at its own
// level of detail by its distance; the forms are built, lit and fogged in chunks on the workers, then drawn in bands of rows,
// every band walking the same polygons in the same order, so the frame is the serial loop's (B-310). C-01's boxes and domes
// keep their arithmetic and their light, and draw as they did; the peoples' colours are their banks (`FORM_BANKS`).
#include "surface_view.h"
#include "core/parallel.h"
#include <cmath>
#include <algorithm>
#include <functional>

void SurfaceView::groupPieces(RuinCell& c) {
    const int nb = (int)c.spec.buildings.size();
    // B-420: how far its pieces reach (a capital's harbour stood 650 m out on a radius of 473: its hall without colliders, a boulder in it)
    c.r.reach = c.spec.size;
    for (const Form& e : c.elems[0]) c.r.reach = std::max(c.r.reach, std::hypot(e.x, e.z) + formReach(e));
    for (const Building& b : c.spec.buildings) c.r.reach = std::max(c.r.reach, std::hypot(b.x, b.z) + std::hypot(b.hw, b.hd));
    for (int l = 0; l < 3; l++) {
        std::vector<int>& g = c.groups[l];
        g.assign(nb + 2, (int)c.elems[l].size());
        // the pieces come building by building (`ruinElements`), the graves' stones (building -1) after them
        int i = 0, n = (int)c.elems[l].size();
        for (int b = 0; b < nb; b++) {
            g[b] = i;
            while (i < n && c.elems[l][i].building == b) i++;
        }
        g[nb] = i;
        g[nb + 1] = n;
    }
}

void SurfaceView::testPlaceSettlement(double x, double z, const RuinSpec& spec, const std::vector<Form> pieces[3]) {
    placedCells.emplace_back();
    RuinCell& c = placedCells.back();
    c.has = true; c.placed = true; c.spec = spec;
    for (int l = 0; l < 3; l++) c.elems[l] = pieces[l];
    c.r.x = x; c.r.z = z; c.r.heading = spec.heading; c.r.size = spec.size; c.r.spec = &c.spec;
    groupPieces(c);
}

namespace {
// A-02: the points of the ground under a big form's footprint: its corners and along its long sides every 15 m or so
template <class F> void footPoints(const Form& e, double ex, double ez, F fn) {
    const double ca = std::cos(e.heading), sa = std::sin(e.heading), L = std::max(e.hx, e.hz);
    const int along = clampi((int)std::ceil(L / 15), 1, 8);
    for (int side = -1; side <= 1; side += 2)
        for (int j = 0; j <= along; j++) {
            double t = (double)j / along * 2 - 1;
            double a = (e.hx >= e.hz ? t * e.hx : side * e.hx) * 0.92, bb = (e.hx >= e.hz ? side * e.hz : t * e.hz) * 0.92;
            fn(ex + ca * a + sa * bb, ez - sa * a + ca * bb);
        }
}
inline bool c01Form(const Form& e) { return e.kind <= FK_DOME && e.sub == 0 && e.tilt == 0; }
// B-420: a ring standing on its rim touches the ground at one point, its middle: it stands there (on the highest ground of its square it floated
// over a slope, the explorer walking under it)
inline bool bigForm(const Form& e) { return !c01Form(e) && std::max(e.hx, e.hz) > 6 && e.kind != FK_CABLE && !(e.kind == FK_TORUS && e.tilt > 0.3); }
}   // namespace

// B-420: a piece's foot as the near level draws it (the colliders and the explorer's footing read the same)
bool SurfaceView::pieceFootNear(const Form& e, double ex, double ez, double& gy, double& skirt) {
    const bool big = bigForm(e);
    gy = site.groundHeight(ex, ez);
    const double w = site.waterAt(ex, ez);
    double lo = gy, hi = gy;
    if (big && e.y0 < 0.01) footPoints(e, ex, ez, [&](double x, double z) { double h = site.groundHeight(x, z); lo = std::min(lo, h); hi = std::max(hi, h); });
    if (w > -1e8 && gy < w) { if (e.part != 6) return false; gy = w; }   // C-08: the harbour works stand in the water, their foot at its level
    skirt = 0;
    if (e.y0 < 0.01) {
        skirt = 1.0;   // C-01: a piece on the ground is sunk a metre
        if (big) { lo = std::min(lo, gy); hi = std::max(hi, gy); gy = hi; skirt = hi - lo + 1.0; }   // A-02: a big form stands on the highest ground under it, its foot reaching down to the lowest
        if (big && formWalkable(e)) { gy = lo; skirt = 1.0; }   // R-412 (B-420): earth on the lowest, sunk into the slope above (lifted, a hill fort's bank stood a cliff downhill)
    }
    return true;
}

// a settlement's forms into the frame's list: each building at the level its distance asks, beyond 4 km its silhouette where it
// stands at least a pixel high
void SurfaceView::gatherSettlement(const Ruin& r, const RuinCell& cell, double dist) {
    const RuinSpec& sp = *r.spec;
    const int nb = (int)sp.buildings.size();
    if (cell.groups[0].size() != (size_t)nb + 2) return;
    const int people = cell.placed ? (sp.people == 1 ? 1 : 0) : (sp.people == 1 && peoples > 1 ? 1 : 0);   // C-13: the settlement's people's banks
    const double R = sp.size;
    double tallest = 0; for (const Form& e : cell.elems[2]) tallest = std::max(tallest, formTop(e) - (e.absolute ? e.y0 : 0.0));
    if (dist - R > 4000 && tallest * proj.f / std::max(1.0, dist - R) < 1.2) return;   // nothing of it stands a pixel high
    const double fwx = camLocal.m[2][0], fwz = camLocal.m[2][2];   // the camera's forward on the ground
    for (int b = 0; b <= nb; b++) {   // b == nb: the graves' stones, with the settlement
        double bx = r.x, bz = r.z, brad = R;
        if (b < nb) { const Building& B = sp.buildings[b]; bx = r.x + B.x; bz = r.z + B.z; brad = std::max(B.hw, B.hd); }
        double db = std::max(0.0, std::hypot(bx - camPos.x, bz - camPos.z) - brad);
        int lod = db < 350 ? 0 : (db < 1300 ? 1 : (db < 4000 ? 2 : 3));
        if (testForceLod >= 0) lod = testForceLod;
        if (b == nb && lod > 0) continue;
        const int L = std::min(lod, 2);
        const std::vector<Form>& el = cell.elems[L];
        const int i0 = cell.groups[L][b], i1 = cell.groups[L][b + 1];
        if (i0 >= i1) continue;
        for (int i = i0; i < i1; i++) {
            const Form& e = el[i];
            double ex = r.x + e.x, ez = r.z + e.z;
            double ddx = ex - camPos.x, ddz = ez - camPos.z, d = std::sqrt(ddx * ddx + ddz * ddz);
            const bool c01 = e.kind <= FK_DOME && e.sub == 0 && e.tilt == 0;
            double ext = c01 ? 2 * std::max(std::max(e.hx, e.hz), (e.y1 - e.y0) * 0.5) : 2 * std::max(formReach(e), (formTop(e) - e.y0) * 0.5);
            if (lod == 3) {   // the silhouette: only what stands a pixel high
                if ((formTop(e) - (e.absolute ? e.y0 : 0.0)) * proj.f / std::max(1.0, d) < 1.2 || e.kind == FK_CABLE) continue;
            } else if (d > 5 && ext / d * proj.f < 1.2) continue;
            if (d > ext + 2 && ddx * fwx + ddz * fwz < -ext) continue;   // behind the camera
            if (e.absolute) {   // T-03: heights over the reference level (a dam's wall, an aqueduct's arches): where they are, whatever the ground does
                const double fog = 1 - std::exp(-d / env.fogDistance);
                if (lod == 3 && fog > 0.985) continue;
                formItems.push_back({&e, Vec3(ex, 0, ez), 0.0, d, fog, lod, people});
                continue;
            }
            const bool big = bigForm(e);
            double gy, w, lo, hi;
            if (lod == 0) {   // the near ground, as drawn this frame (B-420: the colliders' foot too)
                double skirt;
                if (!pieceFootNear(e, ex, ez, gy, skirt)) continue;
                const double fog = 1 - std::exp(-d / env.fogDistance);
                formItems.push_back({&e, Vec3(ex, gy, ez), skirt, d, fog, lod, people});
                continue;
            } else {   // the ring that draws the ground there, read once while that ring does
                std::vector<FootCache>& fc = cell.foot[L];
                if (fc.size() != el.size()) fc.assign(el.size(), FootCache());
                FootCache& F = fc[i];
                const int ring = drawnRing(d);
                if (F.ring != ring) {
                    double wq; int r1; double g0 = drawnGround(ex, ez, d, wq, r1);
                    F.ring = ring; F.y = (float)g0; F.water = (float)wq; F.lo = F.hi = (float)g0;
                    if (big && e.y0 < 0.01) footPoints(e, ex, ez, [&](double x, double z) { double w2; int r2; double h = drawnGround(x, z, d, w2, r2); F.lo = std::min(F.lo, (float)h); F.hi = std::max(F.hi, (float)h); });
                }
                gy = F.y; w = F.water; lo = F.lo; hi = F.hi;
            }
            if (w > -1e8 && gy < w) { if (e.part != 6) continue; gy = w; }   // C-08: the harbour works stand in the water, their foot at its level
            double fog = 1 - std::exp(-d / env.fogDistance);
            if (lod == 3 && fog > 0.985) continue;
            double skirt = 0;
            if (e.y0 < 0.01) {
                skirt = 1.0;   // C-01: a piece on the ground is sunk a metre
                if (big) { lo = std::min(lo, gy); hi = std::max(hi, gy); gy = hi; skirt = hi - lo + 1.0; }   // A-02: a big form stands on the highest ground under it, its foot reaching down to the lowest
                if (big && formWalkable(e)) { gy = lo; skirt = 1.0; }   // R-412: earth on the lowest (`pieceFootNear`)
                if (lod > 0 && !c01) skirt += 0.004 * d;   // the far rings' ground is coarser than the 64 m samples
            }
            formItems.push_back({&e, Vec3(ex, gy, ez), skirt, d, fog, lod, people});
        }
    }
}

namespace {
inline int formRpIndex(int people, int slot, int windows) { return 1 + ((people * 4 + clampi(slot, 0, 3)) * 5 + clampi(windows, 0, 4)); }
inline int formPlainRp(int people, int slot) { return 41 + people * 4 + clampi(slot, 0, 3); }
}

void SurfaceView::drawFormItems(Framebuffer& fb) {
    lastFormPolys = lastFormLines = lastSilhouettes = 0;
    if (formItems.empty()) return;
    // the raster parameters: [0] the drifts' sand; [1, 41) a people's slot with its windows (0 none, k the people's k-th); [41, 49)
    // a people's slot plain (the silhouettes: no grain far off, where it would only shimmer); [49] A-03's earth (a mound, a bank, a
    // spoil heap): the ground's own soil, the grass of a living world or a desert's sand
    formRps.assign(58, RasterParams());
    formRps[0].bank = 9;
    formRps[49].bank = site.gen.type == PT_DESERT ? 9 : 10;
    formRps[50].bank = 2;    // A-04: a pool's water (the water's bank)
    formRps[51].bank = 16;   // A-04: a great tree's bark and its leaves (the vegetation's banks)
    formRps[52].bank = 3;
    formRps[53].bank = 29; formRps[54].bank = 30; formRps[55].bank = 31; formRps[56].bank = 17; formRps[57].bank = 7;   // T-04: a record's pale, dark, ochre, clay, metal
    for (int p = 0; p < 2; p++) {
        const Culture& cu = cultures[p];
        const FormPalette& pal = palettes[p];
        for (int k = 0; k < 4; k++) {
            const WindowSpec& W = pal.windows[k];
            FacePattern& P = formPatterns[p][k];
            P.bay = std::max(0.5, W.bay); P.storey = std::max(0.5, W.storey);
            double ww = clampd(W.w, 0.05, 1.0), hh = clampd(W.h, 0.05, 1.0);
            P.x0 = W.shape == 4 ? 0.0 : 0.5 - ww * 0.5; P.x1 = W.shape == 4 ? 1.0001 : 0.5 + ww * 0.5;
            P.y0 = clampd(W.sill, 0, 0.95); P.y1 = std::min(0.995, P.y0 + hh);
            P.shape = W.shape; P.dark = W.dark; P.litBank = W.lit ? FORM_BANKS[p][3] : -1; P.litShade = 50; P.fromV = W.fromY;
            P.group = std::max(1, W.group); P.scatter = clampd(W.scatter, 0, 0.9);
            P.meanShare = (P.x1 - P.x0) * (P.y1 - P.y0) * (W.shape == 3 ? 0.785 : (W.shape == 1 ? 0.93 : (W.shape == 5 ? 0.5 : (W.shape == 6 ? 0.62 : 1.0))))
                        * (P.group > 1 ? 0.8 : 1.0) * (1 - P.scatter);
            // R-412: the dress round the windows and up the wall
            P.frame = W.frame; P.frameShade = W.frameShade; P.shutter = W.shutter; P.shutterShade = W.shutterShade;
            P.stripe = W.stripe; P.stripeShade = W.stripeShade; P.dado = W.dado; P.dadoShade = W.dadoShade;
            {
                const double wx = P.x1 - P.x0, wy = P.y1 - P.y0, fvh = P.frame * P.bay / std::max(0.1, P.storey);
                const double frameArea = std::max(0.0, (wx + 2 * P.frame) * (wy + 2 * fvh) - wx * wy) - (P.shutter > 0 ? 2 * std::min(P.frame, P.shutter) * wy : 0.0);
                P.meanDress = ((P.frame > 0 ? frameArea * P.frameShade : 0.0) + (P.shutter > 0 ? 2 * P.shutter * wy * P.shutterShade : 0.0)) * (1 - P.scatter);
            }
        }
        for (int s = 0; s < 4; s++) {
            for (int w = 0; w < 5; w++) {
                RasterParams& rp = formRps[formRpIndex(p, s, w)];
                rp.bank = FORM_BANKS[p][s];
                if (cu.style == 1 && s == 0 && pal.own[0]) { rp.grain2 = mesoFor(MAT_ROCK); rp.grain2Scale = 3.0; rp.grain = &grain; rp.grainScale = 3.0 * FB_SCALE; }   // C-01: walls cut from the striated rock
                if (w > 0) rp.pattern = &formPatterns[p][w - 1];
            }
            formRps[formPlainRp(p, s)].bank = FORM_BANKS[p][s];
        }
    }
    const Vec3& sd = env.sun.dirLocal;
    const double sunUp = smoothstep(-0.03, 0.06, sd.y), lf = env.sun.lightFactor;
    const double ambient = site.atmosphere ? (0.12 + 0.2 * env.skyBrightness) : 0.07;
    // C-01's light on a face: the sky's share by how much it looks up, the sun's by the cosine, toward the fog colour by the distance
    auto lightOf = [&](const Vec3& n) { double amb = ambient * (0.7 + 0.3 * std::max(0.0, n.y)); return amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf; };
    auto insideOf = [&](const Vec3& n) { double amb = ambient * (0.7 + 0.3 * std::max(0.0, n.y)); return amb * 0.85 + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf * 0.4; };
    auto shadeOf = [&](double light, double fog) { double sh = 46 * std::pow(light, 0.6); sh += (63 - sh) * fog; return sh; };
    const int N = (int)formItems.size();
    const int K = clampi(N / 16 + 1, 1, 64);
    if ((int)formChunks.size() < K) formChunks.resize(K);
    const std::string& starName = site.sys->star.name;
    auto rowsOf = [&](const RVert* q, int n, int& y0, int& y1) {   // the rows a polygon can touch (all of them when it crosses the near plane)
        double lo = 1e18, hi = -1e18;
        for (int k = 0; k < n; k++) {
            if (q[k].z < NEAR_Z) { y0 = 0; y1 = FBH - 1; return; }
            double sy = proj.cy - proj.f * q[k].y / q[k].z;
            lo = std::min(lo, sy); hi = std::max(hi, sy);
        }
        y0 = (int)std::floor(lo) - 1; y1 = (int)std::ceil(hi) + 1;
    };
    parallelFor(K, 1, [&](int c0, int c1) {
        static thread_local FormMesh mesh;
        for (int c = c0; c < c1; c++) {
            FormChunk& ch = formChunks[c];
            ch.v.clear(); ch.polys.clear(); ch.lines.clear(); ch.silhouettes = 0;
            const int a0 = (int)((long)N * c / K), a1 = (int)((long)N * (c + 1) / K);
            for (int it = a0; it < a1; it++) {
                const FormItem& I = formItems[it];
                const Form& e = *I.f;
                const int slot = clampi(e.slot, 0, 3);
                const bool lit = slot == 3 && palettes[I.people].glow;   // A-04: slot 3 is light only where the people's palette glows
                if (I.lod == 3) {   // a silhouette across the line of sight, its profile up its height
                    Vec3 dd(I.foot.x - camPos.x, 0, I.foot.z - camPos.z);
                    double hdg = std::atan2(dd.x, dd.z), hw = formHalfWidth(e, hdg);
                    if (hw <= 0) continue;
                    Vec3 right(std::cos(hdg), 0, -std::sin(hdg));
                    if (e.kind == FK_TRUSS) hw *= 0.5;   // a lattice shows the sky through it: a narrower shape
                    if (hw * proj.f / I.d < 0.5) hw = 0.5 * I.d / proj.f;   // at least a pixel of the framebuffer wide; the mush averages it into its neighbours
                    double sh = lit ? 50 + (63 - 50) * I.fog : shadeOf(lightOf(normalize(dd * -1.0)), I.fog) + e.tone * (1 - I.fog);
                    const bool profiled = !(e.kind == FK_BOX || e.kind == FK_ARCH || e.kind == FK_SWEEP || e.kind == FK_VAULT || e.kind == FK_SHELL || (e.kind == FK_LATHE && e.sub == 0));
                    const int Kp = profiled ? 4 : 1;
                    const double top = formTop(e), yb = e.y0 - (5 + 0.003 * I.d);
                    for (int k = 0; k < Kp; k++) {
                        double t0 = (double)k / Kp, t1 = (double)(k + 1) / Kp;
                        double w0 = hw * formProfile(e, t0), w1 = hw * formProfile(e, t1);
                        double ya = I.foot.y + (k == 0 ? yb : e.y0 + (top - e.y0) * t0), ybb = I.foot.y + e.y0 + (top - e.y0) * t1;
                        Vec3 pts[4] = {I.foot - right * w0 + Vec3(0, ya - I.foot.y, 0), I.foot + right * w0 + Vec3(0, ya - I.foot.y, 0), I.foot + right * w1 + Vec3(0, ybb - I.foot.y, 0), I.foot - right * w1 + Vec3(0, ybb - I.foot.y, 0)};
                        RVert q[4];
                        for (int j = 0; j < 4; j++) { Vec3 v = toView(pts[j].x, pts[j].y, pts[j].z); q[j].x = v.x; q[j].y = v.y; q[j].z = v.z; q[j].shade = sh; }
                        int y0, y1; rowsOf(q, 4, y0, y1);
                        ch.polys.push_back({(int)ch.v.size(), 4, formPlainRp(I.people, slot), y0, y1});
                        for (int j = 0; j < 4; j++) ch.v.push_back(q[j]);
                    }
                    ch.silhouettes++;
                    continue;
                }
                formMesh(e, I.lod, I.foot, I.skirt, mesh, proj.f / FB_SCALE / std::max(1.0, I.d));
                const int partRp = e.part == 7 ? 0 : (e.part == 8 ? 49 : (e.part == 9 ? 50 : (e.part == 10 ? 51 : (e.part == 11 ? 52 : (e.part >= FORM_PART_PALE && e.part <= FORM_PART_METAL ? 53 + e.part - FORM_PART_PALE : -1)))));   // the ground's and the world's own banks; T-04 the records
                const double tone = lit ? 0.0 : e.tone * (1 - I.fog);   // R-412: a house painted lighter or darker than its neighbours
                const int rpBase = partRp >= 0 ? partRp : formRpIndex(I.people, slot, 0), rpPat = partRp >= 0 ? partRp : formRpIndex(I.people, slot, e.pattern);
                for (const FormMesh::Poly& P : mesh.polys) {
                    const FormMesh::V* vs = &mesh.v[P.first];
                    Vec3 n = normalize(cross(vs[1].p - vs[0].p, vs[P.n - 1].p - vs[0].p));
                    const bool back = dot(n, vs[0].p - camPos) > 0;   // the far side
                    if (back && !(P.flags & FormMesh::TWO_SIDED)) continue;
                    const double sgn = back ? -1.0 : 1.0;
                    RVert q[8];
                    double flatShade = 0;
                    if (!(P.flags & FormMesh::SMOOTH)) flatShade = lit ? 50 + (63 - 50) * I.fog : shadeOf(back ? insideOf(n * sgn) : lightOf(n), I.fog) + tone;
                    for (int k = 0; k < P.n; k++) {
                        const Vec3& p = vs[k].p;
                        Vec3 v = toView(p.x, p.y, p.z);
                        q[k].x = v.x; q[k].y = v.y; q[k].z = v.z;
                        if (P.flags & FormMesh::SMOOTH) q[k].shade = lit ? 50 + (63 - 50) * I.fog : shadeOf(back ? insideOf(vs[k].n * sgn) : lightOf(vs[k].n), I.fog) + tone;
                        else q[k].shade = flatShade;
                        if (P.flags & FormMesh::FACE_UV) { q[k].u = vs[k].u; q[k].v = vs[k].v; }
                        else { q[k].u = p.x + p.y * 0.5; q[k].v = p.z + p.y * 0.5; }
                    }
                    int y0, y1; rowsOf(q, P.n, y0, y1);
                    ch.polys.push_back({(int)ch.v.size(), P.n, (P.flags & FormMesh::PATTERN) ? rpPat : rpBase, y0, y1});
                    for (int k = 0; k < P.n; k++) ch.v.push_back(q[k]);
                }
                if (!mesh.lines.empty()) {   // members and cables: lines a pixel or more thick; under a fifth of a pixel only the legs, chords and cables
                    Vec3 side = normalize(Vec3(sd.x, 0, sd.z));
                    double sh = lit ? 50 + (63 - 50) * I.fog : shadeOf(0.5 * (lightOf(side) + ambient), I.fog) + tone;
                    for (const FormMesh::Line& l : mesh.lines) {
                        Vec3 va = toView(l.a.x, l.a.y, l.a.z), vb = toView(l.b.x, l.b.y, l.b.z);
                        if (va.z < NEAR_Z && vb.z < NEAR_Z) continue;
                        double zm = std::max(NEAR_Z, 0.5 * (std::max(va.z, NEAR_Z) + std::max(vb.z, NEAR_Z)));
                        double px = l.width * proj.f / zm / FB_SCALE;
                        if (px < 0.2 && !l.main) continue;
                        int thick = clampi((int)std::lround(px), 1, 4);
                        FormLineOut L; L.a.x = va.x; L.a.y = va.y; L.a.z = va.z; L.b.x = vb.x; L.b.y = vb.y; L.b.z = vb.z;
                        L.a.shade = L.b.shade = sh;
                        L.bank = partRp >= 0 ? formRps[partRp].bank : FORM_BANKS[I.people][slot]; L.thick = thick;   // C-17: an instrument's strings in their part's bank
                        RVert ab[2] = {L.a, L.b}; rowsOf(ab, 2, L.y0, L.y1); L.y1 += thick * FB_SCALE;
                        ch.lines.push_back(L);
                    }
                }
                if (e.glyphs && I.d < 70 && I.lod == 0 && e.kind == FK_BOX) {   // C-01: a line of glyphs cut into the front face: the three low bits of each letter of the star's name as strokes
                    const double gy = I.foot.y;
                    Vec3 f(std::sin(e.heading), 0, std::cos(e.heading)), s(std::cos(e.heading), 0, -std::sin(e.heading));
                    double wall = e.y1 - e.y0, width = 2 * e.hx * 0.8, rowY = gy + e.y0 + std::min(wall * 0.55, 1.6), gh = std::min(0.12 * wall, 0.35);
                    Vec3 origin = Vec3(I.foot.x, rowY, I.foot.z) + f * (e.hz + 0.02) - s * (width * 0.5);
                    double step = width / (double)std::max<size_t>(1, std::min<size_t>(starName.size(), 14));
                    for (size_t i = 0; i < starName.size() && i < 14; i++) {
                        int code = (unsigned char)starName[i];
                        for (int seg = 0; seg < 3; seg++) {
                            if (!((code >> seg) & 1)) continue;
                            Vec3 p0 = origin + s * (i * step + seg * step * 0.3) + Vec3(0, seg * gh * 0.4, 0), p1 = p0 + Vec3(0, gh, 0) + s * (step * 0.15);
                            FormLineOut L; Vec3 va = toView(p0.x, p0.y, p0.z), vb = toView(p1.x, p1.y, p1.z);
                            L.a.x = va.x; L.a.y = va.y; L.a.z = va.z; L.a.shade = 18; L.b.x = vb.x; L.b.y = vb.y; L.b.z = vb.z; L.b.shade = 18;
                            L.bank = e.part == 7 ? 9 : FORM_BANKS[I.people][slot]; L.thick = 1;
                            RVert ab[2] = {L.a, L.b}; rowsOf(ab, 2, L.y0, L.y1); L.y1 += FB_SCALE;
                            ch.lines.push_back(L);
                        }
                    }
                }
            }
        }
    });
    for (int c = 0; c < K; c++) { lastFormPolys += (int)formChunks[c].polys.size(); lastFormLines += (int)formChunks[c].lines.size(); lastSilhouettes += formChunks[c].silhouettes; }
    // the bands: every thread walks every chunk in order and draws what reaches its rows
    parallelFor(FBH, 32, [&](int yb, int ye) {
        setRasterBand(yb, ye);
        for (int c = 0; c < K; c++) {
            const FormChunk& ch = formChunks[c];
            for (const FormPolyOut& P : ch.polys) { if (P.y1 < yb || P.y0 >= ye) continue; rasterPolygon(fb, &ch.v[P.first], P.n, formRps[P.rp], proj); }
        }
        for (int c = 0; c < K; c++)
            for (const FormLineOut& L : formChunks[c].lines) { if (L.y1 < yb || L.y0 >= ye) continue; rasterLine3(fb, L.a, L.b, L.bank, proj, true, L.thick); }
        setRasterBand(0, 1 << 30);
    });
}
