// A-02: the settlements' forms drawn (peoples/forms.h). The cells in reach are gathered building by building, each at its own
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
    // how far its pieces reach (a capital's harbour stood 650 m out on a radius of 473: its hall without colliders, a boulder in it)
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

void SurfaceView::testPlaceSettlement(double x, double z, const RuinSpec& spec, const std::vector<Form> pieces[3], const std::vector<FormOpening>* openings) {
    placedCells.emplace_back();
    RuinCell& c = placedCells.back();
    c.has = true; c.placed = true; c.spec = spec; c.key = mix64((uint64_t)placedCells.size() ^ 0x71ACEDULL);
    for (int l = 0; l < 3; l++) c.elems[l] = pieces[l];
    if (openings) c.openings = *openings;
    c.r.x = x; c.r.z = z; c.r.heading = spec.heading; c.r.size = spec.size; c.r.spec = &c.spec;
    overgrowCell(c);   // J-10
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
// J-10: a membrane grown across an opening: a sheet bulging out or in at its middle (by its hash), smooth, seen from both sides, its foot at `foot`;
// KI-478: across an arch's bay its top the arch's head (p0 its shape plus one, p1 its springing over its foot, p2 the head's rise)
double archHeadAt(int shape, double sa, double rise) {   // the head's height over the springing at sa (-1..1 across the bay)
    const double x = std::fabs(sa);
    if (shape == 2 || rise <= 0) return 0;   // flat
    if (shape == 1) {   // pointed: two arcs of radius 1.5 bays' halves, centred half a half-bay off the middle (`archMesh`'s)
        const double c = 0.5, rho = 1.5, apex = std::sqrt(rho * rho - c * c), q = (x + c) / rho;
        return rise * rho * std::sqrt(std::max(0.0, 1 - q * q)) / apex;
    }
    return rise * std::sqrt(std::max(0.0, 1 - x * x));   // round
}
void membraneMesh(const Form& e, const Vec3& foot, FormMesh& out) {
    out.clear();
    const bool arched = e.p[0] >= 1;
    const int NA = arched ? 8 : 4, NY = 4;
    const double ch = std::cos(e.heading), sh = std::sin(e.heading);
    for (int j = 0; j <= NY; j++)
        for (int i = 0; i <= NA; i++) {
            const double sa = 2.0 * i / NA - 1, a = e.hx * sa;
            const double top = arched ? std::min(e.y1, e.y0 + e.p[1] + archHeadAt((int)e.p[0] - 1, sa, e.p[2])) : e.y1;   // (its column's top)
            const double hy = 0.5 * (top - e.y0), ym = 0.5 * (e.y0 + top);
            const double bulge = (e.seed & 1 ? 1 : -1) * std::min(0.25, 0.14 * std::min(e.hx, 0.5 * (e.y1 - e.y0)));
            const double y = e.y0 + (top - e.y0) * j / NY, sy = hy > 1e-6 ? (y - ym) / hy : 0;
            const double b = bulge * (1 - sa * sa) * (1 - sy * sy), da = -2 * bulge * sa / e.hx * (1 - sy * sy), dy = hy > 1e-6 ? -2 * bulge * sy / hy * (1 - sa * sa) : 0;
            FormMesh::V v;
            v.p = Vec3(foot.x + ch * a + sh * b, foot.y + y, foot.z - sh * a + ch * b);
            const Vec3 nl(-da, 1, -dy);   // (across, along, up) in its frame
            v.n = normalize(Vec3(ch * nl.x + sh * nl.y, nl.z, -sh * nl.x + ch * nl.y));
            out.v.push_back(v);
        }
    const int grid = (int)out.v.size();
    for (int side = 0; side < 2; side++)   // each face lit as an outer one (a room's inside is lit dimmer)
        for (int j = 0; j < NY; j++)
            for (int i = 0; i < NA; i++) {
                const int k = (int)out.v.size(), q0 = j * (NA + 1) + i;
                const int c[4] = {q0, q0 + NA + 1, q0 + NA + 2, q0 + 1};
                for (int m = 0; m < 4; m++) {
                    FormMesh::V v = out.v[c[side ? 3 - m : m]];
                    if (side) v.n = v.n * -1.0;
                    out.v.push_back(v);
                }
                out.polys.push_back({k, 4, FormMesh::SMOOTH});
            }
    out.v.erase(out.v.begin(), out.v.begin() + grid);   // (the grid's own points: the faces have their copies)
    for (FormMesh::Poly& P : out.polys) P.first -= grid;
}
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
    if (w > -1e8 && gy < w) { if (e.part != 6) return false; gy = w; }   // the harbour works stand in the water, their foot at its level
    skirt = 0;
    if (e.y0 < 0.01) {
        skirt = 1.0;   // a piece on the ground is sunk a metre
        if (big) { lo = std::min(lo, gy); hi = std::max(hi, gy); gy = hi; skirt = hi - lo + 1.0; }   // a big form stands on the highest ground under it, its foot reaching down to the lowest
        if (big && formWalkable(e)) { gy = lo; skirt = 1.0; }   // earth on the lowest, sunk into the slope above (lifted, a hill fort's bank stood a cliff downhill)
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
        const bool grownOn = cell.climb.size() == (size_t)nb + 1 && cell.climb[b] >= 0;   // J-10: an overgrowth's cover on it
        const int gline = b < (int)cell.lines.size() ? cell.lines[b] : cell.growthLine;   // KI-501: the line whose cover lies at it
        for (int i = i0; i < i1; i++) {
            const Form& e = el[i];
            if (e.part == FORM_PART_GROWTH && growthBank < 0) continue;   // (no bank for it this frame)
            const float climb = grownOn ? (float)pieceClimb(cell, b, e.x, e.z, e.seed) : -1.f;   // KI-477: this piece's (its silhouettes' too)
            double ex = r.x + e.x, ez = r.z + e.z;
            double ddx = ex - camPos.x, ddz = ez - camPos.z, d = std::sqrt(ddx * ddx + ddz * ddz);
            const bool c01 = e.kind <= FK_DOME && e.sub == 0 && e.tilt == 0;
            double ext = c01 ? 2 * std::max(std::max(e.hx, e.hz), (e.y1 - e.y0) * 0.5) : 2 * std::max(formReach(e), (formTop(e) - e.y0) * 0.5);
            if (lod == 3) {   // the silhouette: only what stands a pixel high
                if ((formTop(e) - (e.absolute ? e.y0 : 0.0)) * proj.f / std::max(1.0, d) < 1.2 || e.kind == FK_CABLE) continue;
            } else if (d > 5 && ext / d * proj.f < 1.2) continue;
            if (d > ext + 2 && ddx * fwx + ddz * fwz < -ext) continue;   // behind the camera
            if (e.absolute) {   // heights over the reference level (a dam's wall, an aqueduct's arches): where they are, whatever the ground does
                const double fog = 1 - std::exp(-d / env.fogDistance);
                if (lod == 3 && fog > 0.985) continue;
                formItems.push_back({&e, Vec3(ex, 0, ez), 0.0, d, fog, lod, people, climb, gline});
                continue;
            }
            const bool big = bigForm(e);
            double gy, w, lo, hi;
            if (lod == 0) {   // the near ground, as drawn this frame (B-420: the colliders' foot too)
                double skirt;
                if (!pieceFootNear(e, ex, ez, gy, skirt)) continue;
                const double fog = 1 - std::exp(-d / env.fogDistance);
                formItems.push_back({&e, Vec3(ex, gy, ez), skirt, d, fog, lod, people, climb, gline});
                continue;
            } else {   // the ring that draws the ground there, read once while that ring does
                std::vector<FootCache>& fc = cell.foot[L];
                if (fc.size() != el.size()) fc.assign(el.size(), FootCache());
                FootCache& F = fc[i];
                const int ring = drawnRing(d);
                if (F.ring != ring) {
                    double wq; int r1; double g0 = drawnGround(ex, ez, d, wq, r1);
                    F.ring = ring; F.y = (float)g0; F.water = (float)wq; F.lo = F.hi = (float)g0;
                    if (big && e.y0 < 0.01)
                        footPoints(e, ex, ez, [&](double x, double z) {
                            double w2;
                            int r2;
                            double h = drawnGround(x, z, d, w2, r2);
                            F.lo = std::min(F.lo, (float)h);
                            F.hi = std::max(F.hi, (float)h);
                        });
                }
                gy = F.y; w = F.water; lo = F.lo; hi = F.hi;
            }
            if (w > -1e8 && gy < w) { if (e.part != 6) continue; gy = w; }   // the harbour works stand in the water, their foot at its level
            double fog = 1 - std::exp(-d / env.fogDistance);
            if (lod == 3 && fog > 0.985) continue;
            double skirt = 0;
            if (e.y0 < 0.01) {
                skirt = 1.0;   // a piece on the ground is sunk a metre
                if (big) { lo = std::min(lo, gy); hi = std::max(hi, gy); gy = hi; skirt = hi - lo + 1.0; }   // a big form stands on the highest ground under it, its foot reaching down to the lowest
                if (big && formWalkable(e)) { gy = lo; skirt = 1.0; }   // earth on the lowest (`pieceFootNear`)
                if (lod > 0 && !c01) skirt += 0.004 * d;   // the far rings' ground is coarser than the 64 m samples
            }
            formItems.push_back({&e, Vec3(ex, gy, ez), skirt, d, fog, lod, people, climb, gline});
        }
    }
}

namespace {
inline int formRpIndex(int people, int slot, int windows) { return 1 + ((people * 4 + clampi(slot, 0, 3)) * 5 + clampi(windows, 0, 4)); }
inline int formPlainRp(int people, int slot) { return 41 + people * 4 + clampi(slot, 0, 3); }
// J-10: an overgrowth's cover: [58, 116) each of the people's with the cover in its second bank, then the cover alone, a drape, a membrane;
// KI-477 and on: the cover plain (the far silhouettes), a torn membrane, and the cover alone over each people's windows grown shut
constexpr int RP_COVER = 116, RP_DRAPE = 117, RP_MEMBRANE = 118, RP_COVER_FAR = 119, RP_TORN = 120, RP_COVER_WIN = 121;
// KI-501: [58, 129) again for each other growing line (its bank, its tile, its night light), from RP_LINE0: a building takes its own line's
constexpr int RP_LINE0 = RP_COVER_WIN + 8, RP_LINE_N = RP_LINE0 - 58;
}

void SurfaceView::drawFormItems(Framebuffer& fb) {
    lastFormPolys = lastFormLines = lastSilhouettes = 0;
    if (formItems.empty()) return;
    // the raster parameters: [0] the drifts' sand; [1, 41) a people's slot with its windows (0 none, k the people's k-th); [41, 49)
    // a people's slot plain (the silhouettes: no grain far off, where it would only shimmer); [49] A-03's earth (a mound, a bank, a
    // spoil heap): the ground's own soil, the grass of a living world or a desert's sand
    formRps.assign(RP_LINE0 + MADE_LINES * RP_LINE_N, RasterParams());
    const bool frozen = deadCoverOf(site.gen) == DC_FROZEN;   // a frozen world's drifts are snow, its earthworks under frost
    formRps[0].bank = frozen ? 8 : 9;
    formRps[49].bank = frozen ? 8 : (kindHas(site.gen.type, KT_FLORA) ? 10 : 9);
    formRps[50].bank = 2;    // a pool's water (the water's bank)
    formRps[51].bank = 16;   // a great tree's bark and its leaves (the vegetation's banks)
    formRps[52].bank = 3;
    formRps[53].bank = 29; formRps[54].bank = 30; formRps[55].bank = 31; formRps[56].bank = 17; formRps[57].bank = 7;   // a record's pale, dark, ochre, clay, metal
    const bool leftOn = site.gen.hasTrait(TR_LIGHTS_LEFT_ON);   // every window lit (a people's own light where it glows, else the lamps' bank 15)
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
            P.shape = W.shape; P.dark = W.dark; P.litBank = W.lit ? FORM_BANKS[p][3] : (leftOn ? (pal.glow ? FORM_BANKS[p][3] : 15) : -1); P.litShade = 50; P.fromV = W.fromY;
            P.litShare = leftOn && !W.lit ? 0.3 : 0.0;   // a third of a dead people's windows still lit (the far future's own all)
            P.group = std::max(1, W.group); P.scatter = clampd(W.scatter, 0, 0.9);
            P.meanShare = (P.x1 - P.x0) * (P.y1 - P.y0) * (W.shape == 3 ? 0.785 : (W.shape == 1 ? 0.93 : (W.shape == 5 ? 0.5 : (W.shape == 6 ? 0.62 : 1.0))))
                        * (P.group > 1 ? 0.8 : 1.0) * (1 - P.scatter);
            // the dress round the windows and up the wall
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
                // walls cut from the striated rock
                if (cu.style == 1 && s == 0 && pal.own[0]) { rp.grain2 = mesoFor(MAT_ROCK); rp.grain2Scale = 3.0; rp.grain = &grain; rp.grainScale = 3.0 * FB_SCALE; }
                if (w > 0) rp.pattern = &formPatterns[p][w - 1];
            }
            formRps[formPlainRp(p, s)].bank = FORM_BANKS[p][s];
        }
    }
    // J-10: a seeded overgrowth's cover in its bank and tile: [58, 116) each of the above with the cover in its second bank under an item's climb
    // (its edge wobbled by the ground's edge noise, the windows on it grown shut); [116] the cover alone (a face wholly under the climb, a
    // nodule), lit as the cover on the ground is, at night lit dimly all over and brightly in its veins (KI-492: the ground's, its second bank
    // under the veins' tile); [117] a drape, cut out under its ragged foot; [118] a membrane, its veins broader; [119] the cover plain (the
    // silhouettes far off); [120] a torn membrane, slit up its middle and ragged; [121, 129) the cover alone over each people's windows, grown
    // shut. KI-501: the latest growing line's, then each other's from RP_LINE0
    const MadeWorld& gm = site.gen.made;
    const bool grown = growthBank >= 0 && gm.grown() && gm.growthLine >= 0 && gm.growthLine < MADE_LINES;
    // the weight a metre up a face over the climb; the edge's wobble (weight units: metres); the cover's edge a little darker than its wall (its
    // foot in the wall's shade, where the face's own light is not read); the weight its veins' light is drawn under
    constexpr double GROWN_W = 1.0, GROWN_EDGE = 0.8, GROWN_DARK = 2.0, GLOW_W = 0.75;
    auto lineGlowOf = [&](int l) { return l >= 0 && l < MADE_LINES && growthLineBank[l] >= 0 ? growthLineGlow[l] : growthGlow; };
    auto lineBlock = [&](int l) { return grown && l >= 0 && l < MADE_LINES && l != gm.growthLine && gm.spread[l].growth.on() ? RP_LINE0 + l * RP_LINE_N : -1; };
    if (grown) {
        auto coverRps = [&](int base, int bank, const GrainTexture* tile, double glow) {
            for (int i = 0; i < 58; i++) {
                RasterParams& o = formRps[base + i];
                o = formRps[i];
                o.bank2 = bank; o.grain2b = tile; o.grain2Scale = 3.0; o.edge = &grainEdge; o.edgeScale = 1.3; o.edgeAmp = GROWN_EDGE; o.patternAbove = true;
                o.shade2 = -GROWN_DARK;
                if (o.pattern) o.patternShut = 1;
            }
            RasterParams& all = formRps[base + RP_COVER - 58];
            all.bank = bank; all.grain2 = tile; all.grain2Scale = 3.0;
            RasterParams& drape = formRps[base + RP_DRAPE - 58];
            drape = all; drape.cutout = true; drape.edge = &grainEdge; drape.edgeScale = 2.5; drape.edgeAmp = 0.45;
            RasterParams& membrane = formRps[base + RP_MEMBRANE - 58];   // its veins broader
            membrane = all; membrane.grain2Scale = 1.4;
            RasterParams& torn = formRps[base + RP_TORN - 58];   // (its weight by how far across from its middle: under a half, where the explorer went through, none)
            torn = membrane; torn.cutout = true; torn.edge = &grainEdge; torn.edgeScale = 3.0; torn.edgeAmp = 0.3;
            formRps[base + RP_COVER_FAR - 58].bank = bank;
            if (glow > 0.02) {   // its veins lit (the second bank where the veins' tile falls under the weight), lifted into the ramp's glow as the ground's
                buildGrowthVeins();   // (once a landing, here on the main thread)
                all.bank2 = bank; all.grain2b = tile; all.edge = &growthVeins; all.edgeScale = GROWTH_VEIN_SCALE; all.edgeAmp = 0.45; all.shade2 = 36 * glow;
            }
            for (int p = 0; p < 2; p++)
                for (int w = 0; w < 4; w++) { RasterParams& o = formRps[base + RP_COVER_WIN - 58 + p * 4 + w]; o = all; o.pattern = &formPatterns[p][w]; o.patternShut = 2; }
        };
        coverRps(58, growthBank, mesoFor(gm.spread[gm.growthLine].growth.material), growthGlow);
        for (int l = 0; l < MADE_LINES; l++)
            if (lineBlock(l) >= 0) coverRps(lineBlock(l), growthBankOf(l), mesoFor(gm.spread[l].growth.material), lineGlowOf(l));
    }
    const Vec3& sd = env.sun.dirLocal;
    const double sunUp = smoothstep(-0.03, 0.06, sd.y), lf = env.sun.lightFactor;
    const double ambient = site.atmosphere ? (0.12 + 0.2 * env.skyBrightness) : 0.07;
    // C-01's light on a face: the sky's share by how much it looks up, the sun's by the cosine, toward the fog colour by the distance
    // the explorer's lamp on a face at p (`lampAt`)
    auto lightOf = [&](const Vec3& n, const Vec3& p) {
        double amb = ambient * (0.7 + 0.3 * std::max(0.0, n.y));
        return amb + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf + (nLamps ? (1 - amb) * lampAt(p, &n) : 0.0);
    };
    auto insideOf = [&](const Vec3& n, const Vec3& p) {
        double amb = ambient * (0.7 + 0.3 * std::max(0.0, n.y));
        return amb * 0.85 + (1 - amb) * std::max(0.0, dot(n, sd)) * sunUp * lf * 0.4 + (nLamps ? (1 - amb) * lampAt(p, &n) : 0.0);
    };
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
                const bool lit = slot == 3 && palettes[I.people].glow;   // slot 3 is light only where the people's palette glows
                if (I.lod == 3) {   // a silhouette across the line of sight, its profile up its height
                    Vec3 dd(I.foot.x - camPos.x, 0, I.foot.z - camPos.z);
                    double hdg = std::atan2(dd.x, dd.z), hw = formHalfWidth(e, hdg);
                    if (hw <= 0) continue;
                    Vec3 right(std::cos(hdg), 0, -std::sin(hdg));
                    if (e.kind == FK_TRUSS) hw *= 0.5;   // a lattice shows the sky through it: a narrower shape
                    if (hw * proj.f / I.d < 0.5) hw = 0.5 * I.d / proj.f;   // at least a pixel of the framebuffer wide; the mush averages it into its neighbours
                    double sh = lit ? 50 + (63 - 50) * I.fog : shadeOf(lightOf(normalize(dd * -1.0), I.foot), I.fog) + e.tone * (1 - I.fog);
                    const bool profiled = !(e.kind == FK_BOX || e.kind == FK_ARCH || e.kind == FK_SWEEP || e.kind == FK_VAULT || e.kind == FK_SHELL || (e.kind == FK_LATHE && e.sub == 0));
                    const int Kp = profiled ? 4 : 1;
                    const double top = formTop(e), yb = e.y0 - (5 + 0.003 * I.d);
                    // KI-477: under its climb in the cover's bank (a slice its climb crosses split there), so a taken town stays taken far off
                    const bool coverFar = grown && I.climb >= 0 && !lit && e.part != FORM_PART_GROWTH;
                    const double coverTop = I.foot.y + (e.absolute ? e.y0 : 0.0) + I.climb;
                    const int farRp = lineBlock(I.gline) >= 0 ? lineBlock(I.gline) + RP_COVER_FAR - 58 : RP_COVER_FAR;   // (KI-501: its building's line's)
                    for (int k = 0; k < Kp; k++) {
                        double t0 = (double)k / Kp, t1 = (double)(k + 1) / Kp;
                        double w0 = hw * formProfile(e, t0), w1 = hw * formProfile(e, t1);
                        double ya = I.foot.y + (k == 0 ? yb : e.y0 + (top - e.y0) * t0), ybb = I.foot.y + e.y0 + (top - e.y0) * t1;
                        auto slice = [&](double y0s, double y1s, double wa, double wb, int rpS) {
                            Vec3 pts[4] = {I.foot - right * wa + Vec3(0, y0s - I.foot.y, 0), I.foot + right * wa + Vec3(0, y0s - I.foot.y, 0), I.foot + right * wb + Vec3(0, y1s - I.foot.y, 0),
                                    I.foot - right * wb + Vec3(0, y1s - I.foot.y, 0)};
                            RVert q[4];
                            for (int j = 0; j < 4; j++) { Vec3 v = toView(pts[j].x, pts[j].y, pts[j].z); q[j].x = v.x; q[j].y = v.y; q[j].z = v.z; q[j].shade = sh; }
                            int y0, y1; rowsOf(q, 4, y0, y1);
                            ch.polys.push_back({(int)ch.v.size(), 4, rpS, y0, y1});
                            for (int j = 0; j < 4; j++) ch.v.push_back(q[j]);
                        };
                        const int own = formPlainRp(I.people, slot);
                        if (!coverFar || coverTop <= ya) slice(ya, ybb, w0, w1, own);
                        else if (coverTop >= ybb) slice(ya, ybb, w0, w1, farRp);
                        else {
                            const double f = (coverTop - ya) / std::max(1e-6, ybb - ya), wm = w0 + (w1 - w0) * f;
                            slice(ya, coverTop, w0, wm, farRp); slice(coverTop, ybb, wm, w1, own);
                        }
                    }
                    if (leftOn && e.kind != FK_TRUSS) {   // its lit windows from afar, a point of light for every eight metres of its height
                        const uint64_t hf = mix64((uint64_t)(int64_t)std::llround(I.foot.x * 4) * 0x9E3779B97F4A7C15ULL ^ (uint64_t)(int64_t)std::llround(I.foot.z * 4));
                        const int nl = clampi((int)((top - e.y0) / 8.0), 1, 12);
                        const Vec3 toward = normalize(dd * -1.0);
                        for (int k = 0; k < nl; k++) {
                            const double fy = (k + 0.5) / nl, fx = 2 * unitFromHash(mix64(hf + 3 * (uint64_t)k)) - 1;
                            if (unitFromHash(mix64(hf + 3 * (uint64_t)k + 1)) < 0.35) continue;   // a dark window
                            const Vec3 at = I.foot + Vec3(0, e.y0 + (top - e.y0) * fy, 0) + right * (0.8 * fx * hw * formProfile(e, fy)) + toward * 0.5;
                            const Vec3 va = toView(at.x, at.y, at.z);
                            if (va.z < NEAR_Z) continue;
                            FormLineOut L; L.a.x = va.x; L.a.y = va.y; L.a.z = va.z; L.a.shade = 56 + 6 * unitFromHash(mix64(hf + 3 * (uint64_t)k + 2)); L.b = L.a;
                            L.bank = palettes[I.people].glow ? FORM_BANKS[I.people][3] : 15; L.thick = 1;
                            RVert ab[2] = {L.a, L.b}; rowsOf(ab, 2, L.y0, L.y1); L.y1 += FB_SCALE;
                            ch.lines.push_back(L);
                        }
                    }
                    ch.silhouettes++;
                    continue;
                }
                if (e.growth == FG_MEMBRANE) membraneMesh(e, I.foot, mesh);
                else formMesh(e, I.lod, I.foot, I.skirt, mesh, proj.f / FB_SCALE / std::max(1.0, I.d));
                const bool torn = e.growth == FG_MEMBRANE && e.broken > 0;   // KI-479: one the explorer went through
                const int partRp = e.part == FORM_PART_GROWTH ? (e.growth == FG_DRAPE ? RP_DRAPE : (e.growth == FG_MEMBRANE ? (torn ? RP_TORN : RP_MEMBRANE) : RP_COVER))
                                 : e.part == 7 ? 0
                                               : (e.part == 8 ? 49
                                                              : (e.part == 9 ? 50
                                                                             : (e.part == 10 ? 51
                                                                                             : (e.part == 11 ? 52
                                                                                                             : (e.part >= FORM_PART_PALE && e.part <= FORM_PART_METAL
                                                                                                                               ? 53 + e.part - FORM_PART_PALE
                                                                                                                               : -1)))));   // the ground's and the world's own banks; T-04 the records
                // a house painted lighter or darker than its neighbours; J-10: a membrane across an opening darker (the room behind it)
                const double tone = lit ? 0.0 : (e.growth == FG_MEMBRANE ? -6.0 : e.tone) * (1 - I.fog);
                const int rpBase = partRp >= 0 ? partRp : formRpIndex(I.people, slot, 0), rpPat = partRp >= 0 ? partRp : formRpIndex(I.people, slot, e.pattern);
                // J-10: the cover up to its climb over the ground at the foot (a work's wall at its own base), on the people's pieces, their earth and
                // their drifts; a drape's weight down its length
                const bool covered = grown && I.climb >= 0 && !lit && (partRp < 0 || partRp == 0 || partRp == 49);
                const double coverBase = I.foot.y + (e.absolute ? e.y0 : 0.0) + I.climb;
                // KI-481: a piece of the cover, and a face wholly under it, lit as the cover on the ground is (its gloss, its veins' light at night)
                const bool growthPiece = e.part == FORM_PART_GROWTH;
                const uint64_t pieceHash = mix64((uint64_t)e.seed ^ 0x6E0E90ULL);
                const double ech = std::cos(e.heading), esh = std::sin(e.heading);   // (a membrane's across: its slit by how far from its middle)
                int polyNo = 0;
                const int block = lineBlock(I.gline);   // KI-501: its building's own line's cover
                const double nightFloor = 14 * lineGlowOf(I.gline);   // KI-492: the cover's dim light all over at night, as the ground's
                auto coverShade = [&](const Vec3& p, const Vec3& nn) {
                    const Vec3 half = normalize(normalize(camPos - p) + sd);
                    const double s0 = std::min(62.0, std::max(growthShade(p, nn, half, ambient, sunUp, lf, 0, I.gline), nightFloor > 0.28 ? nightFloor : 0.0));
                    return s0 + (63 - s0) * I.fog;
                };
                for (const FormMesh::Poly& P : mesh.polys) {
                    const FormMesh::V* vs = &mesh.v[P.first];
                    Vec3 n = normalize(cross(vs[1].p - vs[0].p, vs[P.n - 1].p - vs[0].p));
                    const bool back = dot(n, vs[0].p - camPos) > 0;   // the far side
                    if (back && !(P.flags & FormMesh::TWO_SIDED)) continue;
                    const double sgn = back ? -1.0 : 1.0;
                    RVert q[8];
                    double flatShade = 0;
                    if (!(P.flags & FormMesh::SMOOTH)) flatShade = lit ? 50 + (63 - 50) * I.fog : shadeOf(back ? insideOf(n * sgn, vs[0].p) : lightOf(n, vs[0].p), I.fog) + tone;
                    for (int k = 0; k < P.n; k++) {
                        const Vec3& p = vs[k].p;
                        Vec3 v = toView(p.x, p.y, p.z);
                        q[k].x = v.x; q[k].y = v.y; q[k].z = v.z;
                        if (P.flags & FormMesh::SMOOTH) q[k].shade = lit ? 50 + (63 - 50) * I.fog : shadeOf(back ? insideOf(vs[k].n * sgn, p) : lightOf(vs[k].n, p), I.fog) + tone;
                        else q[k].shade = flatShade;
                        if (P.flags & FormMesh::FACE_UV) { q[k].u = vs[k].u; q[k].v = vs[k].v; }
                        else { q[k].u = p.x + p.y * 0.5; q[k].v = p.z + p.y * 0.5; }
                    }
                    int rp = (P.flags & FormMesh::PATTERN) ? rpPat : rpBase;
                    bool coverLit = growthPiece && grown;
                    float across = 1e9f;   // KI-492: a face across the cover's edge: its cover's light less the face's own (the second bank's offset)
                    if (covered) {   // wholly under its climb: the cover alone (its windows grown shut); its edge across the face: the second bank under it
                        double wlo = 1e18, whi = -1e18;
                        for (int k = 0; k < P.n; k++) { q[k].w = 0.5 + (vs[k].p.y - coverBase) * GROWN_W; wlo = std::min(wlo, q[k].w); whi = std::max(whi, q[k].w); }
                        if (whi < 0.5 - GROWN_EDGE) {
                            rp = (P.flags & FormMesh::PATTERN) && e.pattern > 0 ? RP_COVER_WIN + I.people * 4 + clampi(e.pattern - 1, 0, 3) : RP_COVER;
                            coverLit = true;
                        } else if (wlo < 0.5 + GROWN_EDGE) {
                            rp += 58;
                            if (!lit) {   // (its gloss and its night light at the face's middle, as a face wholly under it has them)
                                Vec3 mid(0, 0, 0); double own = 0;
                                for (int k = 0; k < P.n; k++) { mid = mid + vs[k].p; own += q[k].shade; }
                                mid = mid * (1.0 / P.n); own /= P.n;
                                const double cov = std::max(coverShade(mid, n * sgn), growthGlowShade(pieceHash + (uint64_t)polyNo, lastT, I.gline));
                                across = (float)(cov - own);
                            }
                        }
                    } else if (e.growth == FG_DRAPE)
                        for (int k = 0; k < P.n; k++) q[k].w = (vs[k].p.y - I.foot.y - e.y0) / std::max(0.1, e.y1 - e.y0);
                    else if (torn)   // a slit up its middle as high as the explorer went through, its sheet whole over it
                        for (int k = 0; k < P.n; k++) {
                            const Vec3 d = vs[k].p - (I.foot + Vec3(0, 0, 0));
                            const double across = std::fabs(d.x * ech - d.z * esh) / std::max(0.1, e.hx), up = vs[k].p.y - I.foot.y - e.y0;
                            q[k].w = std::max(1.6 * across, 0.5 + 2.0 * (up - 1.9));
                        }
                    if (coverLit) {   // the cover's own light at each corner (a sheet's both faces as the ground's structures), its glowing patches
                        const double glow = growthGlowShade(pieceHash + (uint64_t)polyNo, lastT, I.gline);
                        for (int k = 0; k < P.n; k++) {
                            const Vec3 nn = (P.flags & FormMesh::SMOOTH) ? vs[k].n * sgn : n * sgn;
                            q[k].shade = std::max(coverShade(vs[k].p, nn), glow) + (growthPiece ? tone : 0.0);
                            if (rp == RP_COVER || rp >= RP_COVER_WIN) q[k].w = GLOW_W;
                        }
                    }
                    polyNo++;
                    int y0, y1; rowsOf(q, P.n, y0, y1);
                    ch.polys.push_back({(int)ch.v.size(), P.n, block >= 0 && rp >= 58 && rp < RP_LINE0 ? block + rp - 58 : rp, y0, y1});   // (KI-473, KI-501)
                    ch.polys.back().s2 = across;
                    for (int k = 0; k < P.n; k++) ch.v.push_back(q[k]);
                }
                if (!mesh.lines.empty()) {   // members and cables: lines a pixel or more thick; under a fifth of a pixel only the legs, chords and cables
                    Vec3 side = normalize(Vec3(sd.x, 0, sd.z));
                    double sh = lit ? 50 + (63 - 50) * I.fog : shadeOf(0.5 * (lightOf(side, I.foot) + ambient), I.fog) + tone;
                    for (const FormMesh::Line& l : mesh.lines) {
                        Vec3 va = toView(l.a.x, l.a.y, l.a.z), vb = toView(l.b.x, l.b.y, l.b.z);
                        if (va.z < NEAR_Z && vb.z < NEAR_Z) continue;
                        double zm = std::max(NEAR_Z, 0.5 * (std::max(va.z, NEAR_Z) + std::max(vb.z, NEAR_Z)));
                        double px = l.width * proj.f / zm / FB_SCALE;
                        if (px < 0.2 && !l.main) continue;
                        int thick = clampi((int)std::lround(px), 1, 4);
                        FormLineOut L; L.a.x = va.x; L.a.y = va.y; L.a.z = va.z; L.b.x = vb.x; L.b.y = vb.y; L.b.z = vb.z;
                        L.a.shade = L.b.shade = sh;
                        L.bank = partRp >= 0 ? formRps[partRp].bank : FORM_BANKS[I.people][slot]; L.thick = thick;   // an instrument's strings in their part's bank
                        RVert ab[2] = {L.a, L.b}; rowsOf(ab, 2, L.y0, L.y1); L.y1 += thick * FB_SCALE;
                        ch.lines.push_back(L);
                    }
                }
                if (e.glyphs && I.d < 70 && I.lod == 0 && e.kind == FK_BOX) {   // a line of glyphs cut into the front face: the three low bits of each letter of the star's name as strokes
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
    // the bands: every thread walks every chunk in order and draws what reaches its rows. F-06: in chunks of eight rows or a sixty-fourth of
    // the frame, dealt: a chunk costs little more than a test per polygon, and a town fills a few rows of the frame (city 4x 7.16 -> 6.83 ms
    // against the pool's own quarter of a thread's share)
    parallelFor(FBH, 32, [&](int yb, int ye) {
        setRasterBand(yb, ye);
        for (int c = 0; c < K; c++) {
            const FormChunk& ch = formChunks[c];
            for (const FormPolyOut& P : ch.polys) {
                if (P.y1 < yb || P.y0 >= ye) continue;
                if (P.s2 > 1e8f) { rasterPolygon(fb, &ch.v[P.first], P.n, formRps[P.rp], proj); continue; }
                RasterParams r = formRps[P.rp];   // (KI-492: a face across the cover's edge, its cover in its own light)
                r.shade2 = P.s2;
                rasterPolygon(fb, &ch.v[P.first], P.n, r, proj);
            }
        }
        for (int c = 0; c < K; c++)
            for (const FormLineOut& L : formChunks[c].lines) { if (L.y1 < yb || L.y0 >= ye) continue; rasterLine3(fb, L.a, L.b, L.bank, proj, true, L.thick); }
        setRasterBand(0, 1 << 30);
    }, std::max(8, FBH / 64));
}

void SurfaceView::testCoverLight(int& whole, int& wholeVeined, int& other, int& otherVeined, int& across, int& acrossLit) const {
    whole = wholeVeined = other = otherVeined = across = acrossLit = 0;
    for (const FormChunk& ch : formChunks)
        for (const FormPolyOut& P : ch.polys) {
            if (P.rp < 58 || P.rp >= (int)formRps.size()) continue;
            const bool own = P.rp < RP_LINE0;
            const int rp = own ? P.rp : 58 + (P.rp - RP_LINE0) % RP_LINE_N;
            const bool veined = formRps[P.rp].bank2 >= 0 && formRps[P.rp].edge == &growthVeins;
            if (rp == RP_COVER || rp >= RP_COVER_WIN) { (own ? whole : other)++; (own ? wholeVeined : otherVeined) += veined; }
            else if (rp < RP_COVER) { across++; acrossLit += P.s2 < 1e8f; }
        }
}

void SurfaceView::testCoverFaces(int& covered, int& shut, int& lit) const {
    covered = shut = lit = 0;
    for (const FormChunk& ch : formChunks)
        for (const FormPolyOut& P : ch.polys) {
            if (P.rp < 0 || P.rp >= (int)formRps.size()) continue;
            const int rp = P.rp >= RP_LINE0 ? 58 + (P.rp - RP_LINE0) % RP_LINE_N : P.rp;   // (KI-501: another line's as the latest's)
            const bool whole = rp == RP_COVER || (rp >= RP_COVER_WIN && rp < RP_LINE0), piece = rp >= RP_COVER && rp <= RP_TORN;
            covered += whole;
            shut += (rp >= RP_COVER_WIN && rp < RP_LINE0) || (rp >= 58 && rp < RP_COVER && formRps[P.rp].patternShut == 1);
            if (whole || piece) { bool up = false; for (int k = 0; k < P.n; k++) up = up || ch.v[P.first + k].shade >= 45; lit += up; }
        }
}
