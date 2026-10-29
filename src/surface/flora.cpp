// N2 (R-201): vegetation. One deterministic enumerator (forTrees) places every tree from the cell hash,
// the canopy density field (veg x a clearing mask) and the biome; the drawing, the colliders, the forest
// floor and the coverage test all read it. B-315: a tree is a bark-textured tapered trunk with tapered
// branches and a canopy of ragged leaf clusters (camera-facing polygons cut by a leaf-edge noise in the
// rasteriser, shaded by the sun's side and their place in the canopy, textured with a leaf tile), on ten
// silhouettes that each have a look of their own per planet (TreeLook); a fallen log is a six-sided trunk
// with cut ends, broken stubs, a root plate and moss. Undergrowth, meadows, reeds, lily pads, cacti,
// lichen and fireflies come per biome. Everything is a function of position, seed, season and time.
#include "surface_view.h"
#include "core/parallel.h"
#include <mutex>
#include "core/noise.h"
#include "core/clock.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>
#include <chrono>

namespace {
// silhouettes: 0 dome (broadleaf), 1 cone (conifer), 2 umbrella, 3 tiered giant, 4 fibrous stalk, 5 fern tree, 6 mushroom tree,
// 7 weeping, 8 candelabra, 9 spire
const double FAM_H0[10] = {6.0, 8.0, 7.0, 18.0, 4.0, 4.0, 3.0, 7.0, 6.0, 14.0};
const double FAM_H1[10] = {12.0, 16.0, 13.0, 28.0, 8.0, 7.0, 6.0, 13.0, 11.0, 30.0};
const double FAM_R[10] = {0.36, 0.22, 0.45, 0.35, 0.30, 0.42, 0.45, 0.40, 0.42, 0.14};    // canopy radius as a fraction of the height
const double FAM_CY[10] = {0.70, 0.62, 0.90, 0.72, 0.65, 0.85, 0.85, 0.72, 0.80, 0.60};   // canopy centre height as a fraction
inline bool deciduous(int fam) { return fam == 0 || fam == 2 || fam == 3 || fam == 6 || fam == 7 || fam == 8; }
constexpr int BANK_BARK = 16, BANK_WOOD = 17;   // B-315: the vegetation's own banks (SurfaceView::setupPalette)
}

// B-315: the look of each silhouette on this planet, hashed from the seed and the family
void SurfaceView::buildLooks() {
    for (int f = 0; f < 10; f++) {
        uint64_t h = mix64(site.gen.seed ^ (0x100C5ULL + (uint64_t)f * 0x9E3779B97F4A7C15ULL));
        TreeLook& L = looks[f];
        L.aspect = 0.8 + 0.5 * hash01(h);
        L.clusterR = 0.4 + 0.25 * hash01(mix64(h + 1));
        L.count = 0.75 + 0.6 * hash01(mix64(h + 2));
        L.trunkW = 0.75 + 0.7 * hash01(mix64(h + 3));
        L.lean = hash01(mix64(h + 4)) < 0.4 ? 0.0 : 0.03 + 0.1 * hash01(mix64(h + 5));
        L.ragged = 0.7 + 0.8 * hash01(mix64(h + 6));
        L.tone = 0.88 + 0.24 * hash01(mix64(h + 7));
        L.droop = hash01(mix64(h + 8));
    }
}

// The forest's density: the terrain's veg times a clearing mask (worley-ish fbm cells with low density inside)
double SurfaceView::canopyDensityAt(double x, double z, double veg) const {
    double n = 0.5 + 0.5 * fbm2(x / 110.0, z / 110.0, site.gen.seed ^ 0xC1EAULL, 2, 2.0, 0.5);
    double clr = smoothstep(0.30, 0.55, n);
    return veg * (0.12 + 0.88 * clr);
}

double SurfaceView::seasonPhaseLocal() const {
    const Body& b = site.sys->bodies[site.body];
    if (b.axialTilt < 3 * DEG) return 0.5;
    double local = site.lat0 >= 0 ? site.season : -site.season;
    return clampd(local / std::max(0.05, std::sin(b.axialTilt)), -1, 1);
}

void SurfaceView::forTrees(int cx, int cz, const std::function<void(const TreeInst&)>& fn) {
    // P-04: a dead world's old forests (the ground keeps their density in `veg`): standing dead and white with frost on a frozen world, broken
    // stone stumps on a scorched one
    const bool deadWorld = deadKind(site.gen.type), stone = KINDS[site.gen.type].cover == DC_SCORCHED;
    if (!kindHas(site.gen.type, KT_FLORA) && !deadWorld) return;
    const TerrainVertex& tv = site.lod0.at(cx, cz);
    if (tv.water > -1e8f && tv.h < tv.water + 0.5) return;
    if (deadWorld ? tv.veg < 0.3 : (tv.material != MAT_FOREST && tv.material != MAT_GRASS)) return;
    if (tv.veg < 0.2) return;
    const double cs = 16;
    uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0xB0BULL);
    double dens = canopyDensityAt(cx * cs + 8, cz * cs + 8, tv.veg);
    double per = tv.material == MAT_FOREST ? 6.5 : 2.2;
    if (tv.biome == BIO_SAVANNA) per = 3.0;
    if (tv.biome == BIO_TUNDRA) per = 1.5;
    if (deadWorld) per = stone ? 2.0 : 4.5;   // the stone forest's stumps sparse, the frozen one's skeletons a thinned wood
    double nx = (tv.material == MAT_FOREST || deadWorld ? dens : tv.veg) * per * (0.6 + 0.8 * hash01(mix64(h + 5)));
    int n = (int)nx + (hash01(mix64(h + 6)) < nx - (int)nx ? 1 : 0);   // fractional counts as a probability: scattered trees on grass
    if (n <= 0) return;
    double treeScale = std::sqrt(9.8 / std::max(site.gravity, 1.0)) * (site.gen.hasTrait(TR_GIANT_FLORA) ? 1.9 : 1.0);
    double f = seasonPhaseLocal();
    double deadChance = site.gen.hasTrait(TR_DEAD_FOREST) ? 0.6 : 0.03;   // a dying world
    const bool fungal = site.gen.hasTrait(TR_FUNGAL);
    // no tree stands in a settlement's buildings, and its streets and yards keep one in three (the town is overgrown, not gone);
    // a twentieth of them two centuries after the end, a third after ten thousand years
    const RuinCell* near[SETTLEMENTS_NEAR]; int nNear = 0;
    if (worldHadCivilisation(site.gen))
        forNearbyRuins(cx * cs + 8, cz * cs + 8, 1, [&](const Ruin& r, const RuinCell& c, int) {
            if (nNear >= SETTLEMENTS_NEAR) return;
            double dx = r.x - (cx * cs + 8), dz = r.z - (cz * cs + 8);
            if (dx * dx + dz * dz < (r.reach + 50) * (r.reach + 50)) near[nNear++] = &c;
        });
    for (int i = 0; i < n; i++) {
        uint64_t ht = mix64(h + 41 * (uint64_t)(i + 1));
        TreeInst T;
        T.seed = ht;
        T.x = cx * cs + hash01(ht) * cs; T.z = cz * cs + hash01(mix64(ht + 1)) * cs;
        if (!site.roads.empty() && site.roadCover(T.x, T.z) > 0.3) continue;   // nothing grows on the road's bed
        if (!site.marks.empty() && site.markCover(T.x, T.z) > 0.3) continue;   // nor on a town's streets and squares
        if (!site.edits.empty() && site.editCover(T.x, T.z) > 0.3) continue;   // nor on a fresh cut, a line's bed, a plain made flat
        if (nNear) {
            bool skip = false;
            for (int q = 0; q < nNear && !skip; q++) {
                const Ruin& r = near[q]->r;
                double dx = T.x - r.x, dz = T.z - r.z;
                if (dx * dx + dz * dz > (r.reach + 40) * (r.reach + 40)) continue;
                // the street's line, out through the gates and beyond (T-03: a work has none)
                if (near[q]->spec.work < 0 && std::fabs(dx * std::cos(r.heading) - dz * std::sin(r.heading)) < 4.0) { skip = true; break; }
                if (near[q]->spec.work < 0 && dx * dx + dz * dz < r.size * r.size) {
                    // its yards a clearing the forest takes back over the millennia (C-01's one in three left after ten thousand years)
                    const Culture& cu = cultures[near[q]->spec.people == 1 && peoples > 1 ? 1 : 0];
                    double keep = clampd(0.05 + 0.15 * std::log10(std::max(1.0, cu.ageYears / 200.0)), 0.05, 0.33);
                    if (hash01(mix64(ht + 21)) >= keep) { skip = true; break; }   // before the buildings (one hash; it clears 67-95% of the yards' trees)
                }
                skip = inSettlementBuilding(*near[q], T.x, T.z, 1.5);
            }
            if (skip) continue;
        }
        bool second = hash01(mix64(ht + 9)) < 0.3;
        T.fam = second ? site.gen.floraFamily2 : site.gen.floraFamily;
        if (fungal) {   // the fungal forests: caps on stems, parasols, a few tall stalks, whatever the biome
            const double u = hash01(mix64(ht + 10));
            T.fam = u < 0.74 ? 6 : (u < 0.9 ? 2 : 4);
        } else
        switch (tv.biome) {   // biome overrides (B-315: the cold and the dry choose per planet between two silhouettes)
            case BIO_TAIGA: case BIO_TUNDRA: T.fam = hash01(mix64(site.gen.seed ^ 0x7A16AULL)) < 0.7 ? 1 : 9; break;
            case BIO_SAVANNA: T.fam = hash01(mix64(site.gen.seed ^ 0x5AFAULL)) < 0.6 ? 2 : 8; break;
            case BIO_WETLAND: if (hash01(mix64(ht + 10)) < 0.6) { T.fam = hash01(mix64(site.gen.seed ^ 0x3E7ULL)) < 0.6 ? 5 : 7; } break;
            case BIO_TROPICAL: if (hash01(mix64(ht + 10)) < 0.25) { T.fam = 3; } break;
            default: break;
        }
        T.fam = clampi(T.fam, 0, 9);
        const TreeLook& L = looks[T.fam];
        double edge = 0.6 + 0.4 * dens;   // smaller at the forest edge and in clearings
        if (tv.biome == BIO_TUNDRA) edge *= 0.45;
        T.h = (FAM_H0[T.fam] + (FAM_H1[T.fam] - FAM_H0[T.fam]) * hash01(mix64(ht + 2))) * treeScale * edge;
        T.r = T.h * FAM_R[T.fam] * (0.85 + 0.3 * hash01(mix64(ht + 3))) * L.aspect;
        T.cy = T.h * FAM_CY[T.fam];
        {   // the lean: the canopy centre sits off the foot by the tilt, in a hashed direction
            double lean = L.lean * (0.5 + hash01(mix64(ht + 14))), a = hash01(mix64(ht + 15)) * TAU;
            T.lx = std::sin(lean) * T.cy * std::cos(a); T.lz = std::sin(lean) * T.cy * std::sin(a);
        }
        T.dead = deadWorld || ((tv.material == MAT_FOREST || deadChance > 0.1) && hash01(mix64(ht + 77)) < deadChance);
        if (stone) { T.stump = true; T.h *= 0.12 + 0.22 * hash01(mix64(ht + 78)); }   // broken off a metre or three up
        T.bare = !fungal && deciduous(T.fam) && f < -0.6;   // a fungus keeps its cap the year round
        bool autumn = !fungal && deciduous(T.fam) && f < 0 && f >= -0.6;
        double v = hash01(mix64(ht + 12));
        T.bank = (second || autumn) ? 5 : (v < 0.15 ? 10 : 3);
        T.dens = dens;
        T.gy = site.groundHeight(T.x, T.z);
        if (T.gy < site.waterAt(T.x, T.z) + 0.3) continue;
        fn(T);
    }
}

// fallen logs in the forest (colliders too)
void SurfaceView::forLogs(int cx, int cz, const std::function<void(const LogInst&)>& fn) {
    const bool deadWorld = deadKind(site.gen.type), stone = KINDS[site.gen.type].cover == DC_SCORCHED;   // P-04: a dead world's fallen trunks (stone on a scorched one)
    const bool petrified = site.gen.hasTrait(TR_PETRIFIED);   // a petrified forest's trunks where the ground keeps its density (`ltint`)
    if (!kindHas(site.gen.type, KT_FLORA) && !deadWorld && !petrified) return;
    const TerrainVertex& tv = site.lod0.at(cx, cz);
    if (tv.water > -1e8f && tv.h < tv.water + 0.5) return;
    const double cs = 16;
    uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0xB0BULL);
    LogInst L;
    if (petrified && tv.ltint > 25 && tv.material != MAT_FOREST && tv.material != MAT_SNOW && tv.material != MAT_ICE) {
        if (hash01(mix64(h + 87)) > 0.85 * tv.ltint / 255.0) return;   // a log in most cells where they lie thickest
        L.stone = true;
    } else {
        if (!kindHas(site.gen.type, KT_FLORA) && !deadWorld) return;
        if ((!deadWorld && tv.material != MAT_FOREST) || tv.veg < 0.3) return;
        if (hash01(mix64(h + 88)) > (stone ? 0.5 : 0.12) * canopyDensityAt(cx * cs + 8, cz * cs + 8, tv.veg) / 0.5) return;   // most of a petrified forest lies fallen
    }
    L.seed = mix64(h + 89);
    L.x = cx * cs + 2 + hash01(L.seed) * (cs - 4); L.z = cz * cs + 2 + hash01(mix64(L.seed + 1)) * (cs - 4);
    L.heading = hash01(mix64(L.seed + 2)) * TAU;
    L.len = 3.5 + 6 * hash01(mix64(L.seed + 3));
    L.radius = 0.25 + 0.35 * hash01(mix64(L.seed + 4)) * hash01(mix64(L.seed + 6));   // most are slim, a few are giants
    if (L.stone) { L.len *= 0.4 + 0.5 * hash01(mix64(L.seed + 7)); L.radius *= 1.7; }   // the stone trunks broken in sections, thick
    // no log in a room
    { const RuinCell* near[SETTLEMENTS_NEAR]; int nNear = settlementsNear(L.x, L.z, near); for (int q = 0; q < nNear; q++) if (inSettlementBuilding(*near[q], L.x, L.z, 1.5)) return; }
    if (!site.roads.empty() && site.roadCover(L.x, L.z) > 0.3) return;   // nor across the road
    if (!site.marks.empty() && site.markCover(L.x, L.z) > 0.3) return;   // nor on a street
    if (!site.edits.empty() && site.editCover(L.x, L.z) > 0.3) return;   // nor on a work's ground
    fn(L);
}

// canopy coverage of the forest-biome ground within a radius of the explorer (N2-01 acceptance)
double SurfaceView::testCanopyCoverage(double radiusM) {
    const double cs = 16;
    int covered = 0, total = 0;
    int r = (int)std::ceil(radiusM / cs) + 1;
    int pcx = (int)std::floor(player.x / cs), pcz = (int)std::floor(player.z / cs);
    std::vector<TreeInst> trees;
    for (int cz = pcz - r; cz <= pcz + r; cz++)
        for (int cx = pcx - r; cx <= pcx + r; cx++) forTrees(cx, cz, [&](const TreeInst& T) { if (!T.dead && !T.bare) trees.push_back(T); });
    for (double z = player.z - radiusM; z <= player.z + radiusM; z += 4)
        for (double x = player.x - radiusM; x <= player.x + radiusM; x += 4) {
            if ((x - player.x) * (x - player.x) + (z - player.z) * (z - player.z) > radiusM * radiusM) continue;
            const TerrainVertex& tv = site.lod0.at((int)std::floor(x / cs), (int)std::floor(z / cs));
            if (tv.material != MAT_FOREST) continue;
            total++;
            for (const TreeInst& T : trees) if ((T.x - x) * (T.x - x) + (T.z - z) * (T.z - z) < T.r * T.r) { covered++; break; }
        }
    return total ? (double)covered / total : 0.0;
}

// B-423: the flora's bands are pinned, six at 1x and eight at 2x-4x whatever the threads (`parallelBands`): a band's coverage grid
// hides the trees its own canopies cover, so where the bands begin decides which far trees show, and a picture that followed the
// thread count differed between machines (13,921 pixels at 4x between one thread and eight)
static int floraBands() { return std::min(8, FBH / 32); }

// B-310: the flora in parallel bands of rows: every thread walks the same cells and trees, draws what falls into its
// band (the rasteriser clips lines, points and polygons to the band; a canopy blob wholly outside it is skipped before
// its points are placed) and keeps its own coverage grid and counts; the counts are merged, a tree counting in the
// band that holds its canopy's centre
void SurfaceView::drawFlora(Framebuffer& fb, double t) {
    lastFloraBandN = 0;
    const bool deadWorld = deadKind(site.gen.type);   // a dead world's old forests (their trees and fallen trunks alone)
    if (!kindHas(site.gen.type, KT_FLORA) && !deadWorld && !site.gen.hasTrait(TR_PETRIFIED)) return;   // a petrified forest's trunks
    lastTreesDrawn = 0; lastFloraPoints = 0; lastFloraPolys = 0; lastNearTrees = lastMidTrees = lastFarTrees = 0; lastTreesHidden = 0; lastTreesVisited = 0;
    for (double& m : lastFloraMs) m = 0;
    const auto tl0 = std::chrono::steady_clock::now();
    {   // the cells near to far (N2-04), within the 70 degree view with a margin, and their trees with the rows each can touch
        const double cs = 16;
        const int radius = 24;
        int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
        floraCells.clear();
        for (int cz = pcz - radius; cz <= pcz + radius; cz++)
            for (int cx = pcx - radius; cx <= pcx + radius; cx++) {
                double ddx = (cx + 0.5) * cs - camPos.x, ddz = (cz + 0.5) * cs - camPos.z;
                double cellDist = std::sqrt(ddx * ddx + ddz * ddz);
                if (cellDist > radius * cs) continue;
                if (cellDist > cs * 3 && (ddx * camLocal.m[2][0] + ddz * camLocal.m[2][2]) / cellDist < 0.45) continue;   // outside the 70 deg view (with margin)
                floraCells.push_back({(float)cellDist, cx, cz});
            }
        std::sort(floraCells.begin(), floraCells.end(), [](const FloraCell& a, const FloraCell& b) { return a.d < b.d; });
        treeList.clear();
        for (const FloraCell& c : floraCells)
            forTrees(c.cx, c.cz, [&](const TreeInst& T) {
                double dist = std::sqrt((T.x - camPos.x) * (T.x - camPos.x) + (T.z - camPos.z) * (T.z - camPos.z));
                if (T.h / dist * proj.f < 1.2) return;
                // the rows it can touch: from the trunk's foot to the top of the canopy (blobs sit within 0.9 r above the
                // canopy centre), a margin for the sway and the blob radii, and the blob shadow on the ground round the foot
                Vec3 vb = toView(T.x, T.gy - 0.3, T.z), vt = toView(T.x + T.lx, T.gy + std::max(T.h, T.cy + 0.9 * T.r), T.z + T.lz);
                if (vb.z < NEAR_Z && vt.z < NEAR_Z) return;   // wholly behind the camera
                int y0 = 0, y1 = FBH;
                if (vb.z >= NEAR_Z && vt.z >= NEAR_Z) {
                    double sb = proj.cy - proj.f * vb.y / vb.z, stp = proj.cy - proj.f * vt.y / vt.z;
                    double zmin = std::min(vb.z, vt.z);
                    double margin = 0.6 * T.r * proj.f / zmin + 2, shadow = proj.f * 1.7 * 4.5 * T.r / (zmin * zmin);
                    y0 = (int)std::floor(std::min(sb, stp) - margin); y1 = (int)std::ceil(std::max(sb, stp) + margin + shadow) + 1;
                }
                treeList.push_back({T, (float)dist, y0, y1});
            });
        lastTreesListed = (int)treeList.size();
    }
    lastFloraListMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tl0).count();
    std::mutex mu;
    parallelBands(FBH, floraBands(), [&](int yb, int ye) {
        const auto tb0 = std::chrono::steady_clock::now();
        setRasterBand(yb, ye);
        FloraStats st;
        drawFloraBand(fb, t, yb, ye, st);
        setRasterBand(0, 1 << 30);
        const double bandMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tb0).count();
        std::lock_guard<std::mutex> lock(mu);
        lastTreesDrawn += st.treesDrawn; lastFloraPoints += st.floraPoints; lastFloraPolys += st.floraPolys; lastTreesVisited += st.treesVisited;
        lastNearTrees += st.nearTrees; lastMidTrees += st.midTrees; lastFarTrees += st.farTrees; lastTreesHidden += st.treesHidden;
        for (int k = 0; k < 2; k++) lastFloraMs[k] = std::max(lastFloraMs[k], st.ms[k]);
        if (lastFloraBandN < TRACE_BANDS) {
            BandTrace& b = lastFloraBands[lastFloraBandN++];
            b = BandTrace();
            b.y0 = yb;
            b.y1 = ye;
            b.ms = bandMs;
            b.walked = st.treesVisited;
            b.drawn = st.treesDrawn;
        }
    });
    std::sort(lastFloraBands, lastFloraBands + lastFloraBandN, [](const BandTrace& a, const BandTrace& b) { return a.y0 < b.y0; });
}

void SurfaceView::drawFloraBand(Framebuffer& fb, double t, int bandY0, int bandY1, FloraStats& st) {
    const double cs = 16;
    const Vec3& sd = env.sun.dirLocal;
    double sunUp = smoothstep(-0.03, 0.06, sd.y);
    double lf = env.sun.lightFactor;
    double ambient = 0.12 + 0.2 * env.skyBrightness;
    double fogD = env.fogDistance;
    double wallHere = 0;   // W-03: the wall of rain between the camera and the tree or the cell being drawn (`rainWall`, set per tree and per cell)
    auto fogShade = [&](double shade, double dist) { double f = 1 - std::exp(-(dist / fogD + wallHere)); return shade + (63 - shade) * f; };
    double light = ambient + (1 - ambient) * sunUp * lf;
    double leafBase = 48 * std::pow(light, 0.7);
    if (site.gen.hasTrait(TR_LUMINOUS_FLORA)) leafBase = std::max(leafBase, 40 * clampd((0.45 - env.skyBrightness) / 0.45, 0, 1));   // the canopies glow at night
    // the explorer's lamp on a tree or a cell: the band's light raised by the lamp's at the canopy's or the cell's middle (the trees' and the
    // cells' shades all start from `light` and `leafBase`)
    const double light0 = light, leafBase0 = leafBase;
    auto lampOn = [&](const Vec3& p) {
        const double lt = nLamps ? lampAt(p) : 0.0;
        light = light0 + (1 - ambient) * lt;
        leafBase = lt > 0 ? std::max(leafBase0, 48 * std::pow(light, 0.7)) : leafBase0;
        return lt;
    };
    Vec3 right(camLocal.m[0][0], camLocal.m[0][1], camLocal.m[0][2]), up(camLocal.m[1][0], camLocal.m[1][1], camLocal.m[1][2]);
    double wind = clampd(env.windKnots / 25.0, 0, 1.5);
    double wdx = std::sin(env.windDir), wdz = std::cos(env.windDir);
    double f = seasonPhaseLocal();
    bool bloom = f > 0.05;                       // flowers in spring and summer
    bool dusk = env.skyBrightness < 0.35;
    double treeScale = std::sqrt(9.8 / std::max(site.gravity, 1.0)) * (site.gen.hasTrait(TR_GIANT_FLORA) ? 1.9 : 1.0);
    const int TILE = 8 * FB_SCALE, TX = (FBW + TILE - 1) / TILE, TY = (FBH + TILE - 1) / TILE;
    std::vector<uint16_t> cov((size_t)TX * TY, 0);
    double tSec = nowMs();
    auto section = [&](int k) { double n = nowMs(); st.ms[k] += n - tSec; tSec = n; };
    bool markCov = true;   // the coverage grid is read by the trees alone: the cells after them mark nothing
    // a tree (or its hiding) counts once: in the band that holds the row of its canopy centre
    auto inBand = [&](double x, double y, double z) {
        Vec3 v = toView(x, y, z);
        if (v.z < NEAR_Z) return bandY0 == 0;
        int sy = clampi((int)(proj.cy - proj.f * v.y / v.z), 0, FBH - 1);
        return sy >= bandY0 && sy < bandY1;
    };
    auto vert = [&](double x, double y, double z, double shade) { RVert q; Vec3 v = toView(x, y, z); q.x = v.x; q.y = v.y; q.z = v.z; q.shade = shade; return q; };
    auto line = [&](double x0, double y0, double z0, double x1, double y1, double z1, double shade, int bank, int th) {
        RVert a = vert(x0, y0, z0, shade), b = vert(x1, y1, z1, shade);
        if (a.z < NEAR_Z || b.z < NEAR_Z) return;
        rasterLine3(fb, a, b, bank, proj, true, th);
    };
    auto point = [&](double x, double y, double z, double shade, int bank, int size) {
        RVert p = vert(x, y, z, shade);
        if (p.z < NEAR_Z) return;
        if (rasterPoint3(fb, p, bank, proj, true, size) && markCov) {
            double sx, sy;
            if (::projectPoint(p, proj, sx, sy)) { int tx = (int)(sx / TILE), ty = (int)(sy / TILE); if (tx >= 0 && ty >= 0 && tx < TX && ty < TY) cov[ty * TX + tx] += (uint16_t)(size * size); }
        }
        st.floraPoints++;
    };
    // a leaf cluster: a ragged disc facing the camera (a fan round its centre with 6-10 rim vertices of hashed radii),
    // cut by the leaf-edge noise in the rasteriser (the vertex weight is 1.1 at the centre and 0.3 at the rim, so the rim
    // breaks into tufts and a few holes open inside), shaded by which side of the canopy it sits on (litC), by the sun's
    // side of the disc and darker underneath, and textured with the leaf tile at 12 texels a metre (bilinear close by,
    // fading past 60 m). It writes depth: nearer clusters, trunks and branches occlude it and it occludes what is behind
    const int S = FB_SCALE;
    const double LEAF_TEX = 12.0;
    const Vec3 sunV = camLocal * sd;   // the sun's direction in view space (its x and y: across the disc)
    auto cluster = [&](uint64_t hb, double bx, double by, double bz, double rb, double ry, double dist, int bank, double tone0, double litC, double ragged) {
        Vec3 cV = toView(bx, by, bz);
        if (cV.z < NEAR_Z + 0.02) return;
        double pxR = rb * proj.f / cV.z;   // framebuffer pixels
        double fogF = 1 - std::exp(-(dist / fogD + wallHere));
        if (pxR < 0.6 * S) {   // a speck
            RVert p; p.x = cV.x; p.y = cV.y; p.z = cV.z; p.shade = leafBase * tone0 * 0.8; p.shade += (63 - p.shade) * fogF;
            if (rasterPoint3(fb, p, bank, proj, true, 1) && markCov) {
                double sx, sy;
                if (::projectPoint(p, proj, sx, sy)) {
                    int tx = (int)(sx / TILE), ty = (int)(sy / TILE);
                    if (tx >= 0 && ty >= 0 && tx < TX && ty < TY) cov[ty * TX + tx] += 1;
                }
            }
            st.floraPoints++;
            return;
        }
        double syC = proj.cy - proj.f * cV.y / cV.z, ryPx = ry * proj.f / cV.z;
        double rExt = std::max(pxR, ryPx) * 1.3 + 2;
        if (syC + rExt < bandY0 || syC - rExt > bandY1) return;   // wholly outside this band's rows
        double jit = 0.9 + 0.2 * ((hb >> 40) & 255) / 255.0;
        double base = leafBase * tone0 * jit;
        int nOut = pxR > 14 * S ? 10 : (pxR > 5 * S ? 8 : 6);
        RVert q[12];
        double u0 = (double)((hb >> 8) & 127), v0 = (double)((hb >> 16) & 127);   // where on the tile this cluster starts
        auto setv = [&](RVert& q, double ox, double oy, double shade, double w) {
            q.x = cV.x + ox;
            q.y = cV.y + oy;
            q.z = cV.z;
            q.shade = shade + (63 - shade) * fogF;
            q.u = u0 + ox * LEAF_TEX;
            q.v = v0 + oy * LEAF_TEX;
            q.w = w;
        };
        setv(q[0], 0, 0, base * (0.78 + 0.27 * litC), 1.1);
        for (int k = 0; k < nOut; k++) {
            uint64_t hk = mix64(hb + 31 * (k + 1));
            double a = (k + 0.5 * hash01(hk)) * TAU / nOut;
            double rr = 0.78 + 0.4 * hash01(mix64(hk + 1));
            double ca = std::cos(a), sa = std::sin(a);
            double lit = ca * sunV.x + sa * sunV.y;   // toward the sun across the disc
            double sh = base * (0.5 + 0.27 * litC + 0.23 * std::max(0.0, lit)) * (sa < 0 ? 0.82 : 1.0);
            setv(q[1 + k], ca * rb * rr, sa * ry * rr, sh, 0.3);
        }
        q[nOut + 1] = q[1];
        RasterParams rp; rp.bank = bank;
        // the leaf tile fades out under six logical pixels of radius too: a texel a pixel boiled under every far
        // canopy as the camera moved
        double tileF = smoothstep(2.5 * S, 6.0 * S, pxR);
        if (tileF > 0.02) { rp.grain2 = &leafTile; rp.grain2Scale = 1.0; rp.grainWeight = clampd(1.0 - (dist - 60.0) / 80.0, 0.25, 1.0) * tileF; }
        // a small disc keeps a whole rim: below eight pixels of radius the edge noise (a texel a pixel) tore a
        // different set of pixels out of it every time the camera moved a fraction of a pixel, and every far tree
        // sparkled; the ragged edge fades in between 3 and 8 logical pixels
        double ragF = smoothstep(3.0 * S, 8.0 * S, pxR);
        if (ragF > 0.02) { rp.cutout = true; rp.edge = &leafEdge; rp.edgeScale = 1.0; rp.edgeAmp = ragged * ragF; }
        rasterPolygon(fb, q, nOut + 2, rp, proj);
        st.floraPolys++;
        // the coverage grid: the tiles under the inner part of the disc fill up
        if (!markCov) return;
        double sxC = proj.cx + proj.f * cV.x / cV.z;
        int tx0 = clampi((int)((sxC - pxR * 0.7) / TILE), 0, TX - 1), tx1 = clampi((int)((sxC + pxR * 0.7) / TILE), 0, TX - 1);
        int ty0 = clampi((int)((syC - ryPx * 0.7) / TILE), 0, TY - 1), ty1 = clampi((int)((syC + ryPx * 0.7) / TILE), 0, TY - 1);
        for (int ty = ty0; ty <= ty1; ty++) for (int tx = tx0; tx <= tx1; tx++) cov[ty * TX + tx] = (uint16_t)std::min(64, cov[ty * TX + tx] + 40);
    };
    // a tapered limb facing the camera between two points (the trunk, the branches, the stubs); with a tile it is
    // bark, mapped 8 texels a metre across and 2 along, fading with texW
    auto limb = [&](double x0, double y0, double z0, double x1, double y1, double z1, double w0, double w1, double shade, double sideLit, const GrainTexture* tex, double texW) {
        RVert q[4] = {vert(x0 - right.x * w0, y0, z0 - right.z * w0, shade * (1 - 0.3 * sideLit)), vert(x0 + right.x * w0, y0, z0 + right.z * w0, shade * (1 + 0.3 * sideLit)),
                      vert(x1 + right.x * w1, y1, z1 + right.z * w1, shade * (1 + 0.3 * sideLit)), vert(x1 - right.x * w1, y1, z1 - right.z * w1, shade * (1 - 0.3 * sideLit))};
        for (RVert& v : q) if (v.z < NEAR_Z) return;
        RasterParams rp; rp.bank = BANK_BARK;
        if (tex && texW > 0.02) {
            double len = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0) + (z1 - z0) * (z1 - z0));
            double uw0 = 2 * w0 * 8, uw1 = 2 * w1 * 8, vl = len * 2;
            q[0].u = 0; q[0].v = 0; q[1].u = uw0; q[1].v = 0; q[2].u = uw1; q[2].v = vl; q[3].u = 0; q[3].v = vl;
            rp.grain2 = tex; rp.grain2Scale = 1.0; rp.grainWeight = texW;
        }
        rasterPolygon(fb, q, 4, rp, proj);
        st.floraPolys++;
    };
    // a frond (the fern tree's and the undergrowth's): a leaf-shaped strip from the base over a mid point to the tip,
    // widest at the mid point, with a full weight down its centre line and 0.3 at the edges, so the leaf-edge cutout
    // serrates both edges into leaflets; the leaf tile mottles it. The shade comes fogged
    auto frond = [&](double x0, double y0, double z0, double x1, double y1, double z1, double x2, double y2, double z2, double w, double shade, int bank) {
        double dx = x2 - x0, dz = z2 - z0, dl = std::sqrt(dx * dx + dz * dz) + 1e-9;
        double ax = -dz / dl, az = dx / dl;   // across, in the horizontal plane
        Vec3 P[3] = {Vec3(x0, y0, z0), Vec3(x1, y1, z1), Vec3(x2, y2, z2)};
        double ws[3] = {w * 0.55, w, w * 0.12};
        double along = 0;
        for (int seg = 0; seg < 2; seg++) {
            const Vec3& A = P[seg]; const Vec3& B = P[seg + 1];
            double wa = ws[seg], wb = ws[seg + 1];
            double lenS = length(B - A);
            for (int side = -1; side <= 1; side += 2) {
                RVert q[4] = {vert(A.x, A.y, A.z, shade), vert(A.x + ax * wa * side, A.y, A.z + az * wa * side, shade * 0.88),
                              vert(B.x + ax * wb * side, B.y, B.z + az * wb * side, shade * 0.88), vert(B.x, B.y, B.z, shade)};
                bool ok = true; for (RVert& v : q) if (v.z < NEAR_Z) ok = false;
                if (!ok) continue;
                q[0].w = 1.0; q[1].w = 0.3; q[2].w = 0.3; q[3].w = 1.0;
                q[0].u = along * 12; q[0].v = 40; q[1].u = along * 12; q[1].v = 40 + side * wa * 12;
                q[2].u = (along + lenS) * 12; q[2].v = 40 + side * wb * 12; q[3].u = (along + lenS) * 12; q[3].v = 40;
                RasterParams rp; rp.bank = bank;
                rp.cutout = true; rp.edge = &leafEdge; rp.edgeScale = 1.0; rp.edgeAmp = 0.9;
                rp.grain2 = &leafTile; rp.grain2Scale = 1.0; rp.grainWeight = 0.6;
                rasterPolygon(fb, q, 4, rp, proj);
                st.floraPolys++;
            }
            along += lenS;
        }
    };
    double sideLit = dot(right, sd) * sunUp;
    // cells near to far (floraCells, listed by drawFlora), and a coverage grid of 8x8 (1x) tiles: a tree whose
    // canopy falls entirely on tiles already filled by nearer canopies is skipped (a forest is many trees deep; the
    // hidden ones cost the most)
    const std::vector<FloraCell>& cells = floraCells;
    auto covered = [&](double x, double y, double z, double rM) {
        Vec3 v = toView(x, y, z);
        if (v.z < NEAR_Z) return false;
        double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z, r = proj.f * rM / v.z;
        int tx0 = clampi((int)((sx - r) / TILE), 0, TX - 1), tx1 = clampi((int)((sx + r) / TILE), 0, TX - 1);
        int ty0 = clampi((int)((sy - r) / TILE), 0, TY - 1), ty1 = clampi((int)((sy + r) / TILE), 0, TY - 1);
        if (sx + r < 0 || sy + r < 0 || sx - r >= FBW || sy - r >= FBH) return true;   // off screen: nothing to draw either
        for (int ty = ty0; ty <= ty1; ty++) for (int tx = tx0; tx <= tx1; tx++) if (cov[ty * TX + tx] < 64) return false;
        return true;
    };
    // ---- trees (the frame's list, near to far; only those whose rows reach this band)
    const bool fungalWorld = site.gen.hasTrait(TR_FUNGAL);
    auto drawTree = [&](const TreeInst& T, double dist) {
        wallHere = rainWall(T.x, T.z, dist) + fogSeaWall(T.x, T.gy + 0.4 * T.h, T.z, dist);   // Q-02: a tree in the fog sea
        double rPx = T.r / dist * proj.f / FB_SCALE;   // 1x pixels
        double cx = T.x + T.lx, cz = T.z + T.lz;       // the canopy's centre
        const double lampT = lampOn(Vec3(cx, T.gy + T.cy, cz));
        bool counts = inBand(cx, T.gy + T.cy, cz);
        if (dist > 40 && covered(cx, T.gy + T.cy, cz, T.r)) { if (counts) st.treesHidden++; return; }
        if (counts) st.treesDrawn++;
        const TreeLook& L = looks[T.fam];
        double trunkShade = fogShade((T.dead ? 24 : 17) + 11 * env.skyBrightness * sunUp + 20 * std::min(1.0, lampT), dist);   // bark: the dark to mid stops of its own bank (B-315); D-04: the lamp's
        // the canopy sways with the wind (cluster centres shift along the wind, more at the top)
        double swayA = wind * T.r * 0.12 * std::sin(t * 1.2 + T.x * 0.3 + T.z * 0.17), swx = wdx * swayA, swz = wdz * swayA;
        // blob shadows only where trees stand apart: under a closed canopy the darker floor (N2-02) is the shade,
        // and stacked darkening polygons would go black
        if (dist < 120 && !T.dead && T.dens < 0.5) drawBlobShadow(fb, cx, cz, T.r * 0.9, T.cy);
        int leafBank = T.bare ? 0 : T.bank;
        double tone0 = (T.bare ? 0.5 : 1.0) * L.tone;
        double ragged = L.ragged * (fungalWorld && (T.fam == 6 || T.fam == 2) ? 0.25 : 1.0);   // a fungus' cap smooth-edged
        // a cluster's share of the light from its place in the canopy: the top and the sun's side bright, the far side and
        // the underside dark
        auto litOf = [&](double ox, double oy, double oz) { Vec3 d = normalize(Vec3(ox, oy + T.r * 0.35, oz)); return clampd(0.5 + 0.5 * dot(d, sd), 0, 1) * sunUp + 0.5 * (1 - sunUp); };
        double w0 = T.h * (T.fam == 3 ? 0.04 : (T.fam == 6 ? 0.12 : (T.fam == 9 ? 0.015 : 0.02))) * L.trunkW, w1 = T.fam == 6 ? w0 * 0.85 : w0 * 0.45;
        double trunkTop = T.fam == 1 ? T.h * 0.35 : (T.fam == 3 ? T.h * 0.55 : (T.fam == 8 ? T.h * 0.45 : (T.fam == 9 ? T.h * 0.5 : T.cy)));
        if (T.dead) trunkTop = T.h * 0.85;
        if (T.stump) {   // a stone stump, thick and broken, no branches
            if (rPx < 3.5) { line(T.x, T.gy, T.z, T.x, T.gy + T.h, T.z, trunkShade, BANK_BARK, 1); return; }
            // the trunk's girth (its canopy's measure)
            limb(T.x, T.gy - 0.2, T.z, T.x, T.gy + T.h, T.z, T.r * 0.14, T.r * 0.11, trunkShade, sideLit, &barkTile, clampd(1.0 - (dist - 25.0) / 60.0, 0.0, 1.0));
            return;
        }
        double tx = T.x + T.lx * (trunkTop / T.cy), tz = T.z + T.lz * (trunkTop / T.cy);   // the trunk's top follows the lean
        double cR = T.r * L.clusterR;   // a cluster's radius
        if (rPx < 3.5) {   // far: a trunk line and one cluster (a speck below a pixel)
            if (counts) st.farTrees++;
            if (T.dead) { line(T.x, T.gy, T.z, tx, T.gy + trunkTop, tz, trunkShade, BANK_BARK, 1); return; }
            line(T.x, T.gy, T.z, tx, T.gy + T.cy * 0.8, tz, trunkShade, BANK_BARK, 1);
            bool tall = T.fam == 1 || T.fam == 9;
            double rr = T.r * (tall ? 0.8 : 0.9);
            cluster(mix64(T.seed + 100), cx, T.gy + T.cy, cz, rr, tall ? rr * 1.6 : rr * 0.8, dist, leafBank, tone0, 0.55 + 0.35 * sunUp * clampd(sd.y, 0, 1), ragged);
            return;
        }
        bool near = rPx > 16 && dist < 80;   // the full canopy with branches; below, fewer, bigger clusters
        double texW = clampd(1.0 - (dist - 25.0) / 60.0, 0.0, 1.0);
        if (dist < 100 || rPx > 16) limb(T.x, T.gy - 0.2, T.z, tx, T.gy + trunkTop, tz, w0 * 1.15, w1, trunkShade, sideLit, &barkTile, texW);
        else line(T.x, T.gy - 0.2, T.z, tx, T.gy + trunkTop, tz, trunkShade, BANK_BARK, w0 / dist * proj.f / FB_SCALE > 1.2 ? 2 : 1);   // a line is enough this far
        if (T.dead) {   // a bare skeleton: four branches, no canopy
            for (int k = 0; k < 4; k++) {
                uint64_t hb = mix64(T.seed + 200 + k);
                double a = hash01(hb) * TAU, l = T.h * (0.2 + 0.25 * hash01(mix64(hb + 1)));
                double y0 = T.gy + T.h * (0.45 + 0.35 * hash01(mix64(hb + 2)));
                double f = y0 - T.gy; double bx = T.x + T.lx * (f / T.cy), bz = T.z + T.lz * (f / T.cy);
                if (near) limb(bx, y0, bz, bx + std::cos(a) * l, y0 + l * 0.8, bz + std::sin(a) * l, w0 * 0.4, w0 * 0.08, trunkShade, sideLit, nullptr, 0);
                else line(bx, y0, bz, bx + std::cos(a) * l, y0 + l * 0.8, bz + std::sin(a) * l, trunkShade, BANK_BARK, rPx > 8 ? 2 : 1);
            }
            return;
        }
        if (counts) { if (near) st.nearTrees++; else st.midTrees++; }
        auto branchTo = [&](double bx, double by, double bz, double from) {   // a branch from the trunk to a cluster (near trees)
            double f = clampd(from, 0.2, 1.0);
            double sx = T.x + T.lx * (trunkTop * f / T.cy), sz = T.z + T.lz * (trunkTop * f / T.cy);
            limb(sx, T.gy + trunkTop * f, sz, bx, by, bz, w0 * 0.45, w0 * 0.12, trunkShade, sideLit, nullptr, 0);
        };
        int midN = rPx > 6 ? 3 : 2;
        switch (T.fam) {
            case 0: {   // dome: clusters in an ellipsoid, branches to the first three
                int nb = T.bare ? 4 : (near ? (int)((8 + 6 * hash01(mix64(T.seed + 3))) * L.count) : midN);
                for (int k = 0; k < nb; k++) {
                    uint64_t hb = mix64(T.seed + 100 + k);
                    double a = hash01(hb) * TAU, rr = T.r * (near ? 0.6 : 0.45) * std::sqrt(hash01(mix64(hb + 1)));
                    double ox = std::cos(a) * rr + swx, oz = std::sin(a) * rr + swz, oy = (hash01(mix64(hb + 2)) - 0.5) * T.r * (near ? 0.9 : 0.6);
                    if (k < 3 && near) branchTo(cx + ox, T.gy + T.cy + oy, cz + oz, 0.75);
                    double cr = near ? cR * (0.9 + 0.3 * hash01(mix64(hb + 3))) : T.r * 0.62;
                    cluster(hb, cx + ox, T.gy + T.cy + oy, cz + oz, cr, cr * 0.8, dist, leafBank, tone0, litOf(ox, oy, oz), ragged);
                }
                break;
            }
            case 1: {   // cone: clusters stacked up the trunk, narrowing to a tip
                int nb = near ? (int)((6 + 3 * hash01(mix64(T.seed + 3))) * L.count) : 3;
                for (int k = 0; k < nb; k++) {
                    double fk = (k + 0.5) / nb;
                    double rr = T.r * (1.15 - fk) + T.r * 0.12;
                    double oy = T.h * (0.28 + 0.72 * fk) - T.cy;
                    cluster(mix64(T.seed + 100 + k), cx + swx * fk, T.gy + T.cy + oy, cz + swz * fk, rr * 0.95, rr * 0.6, dist, leafBank, tone0, litOf(0, oy, 0) * (0.85 + 0.15 * fk), ragged);
                }
                break;
            }
            case 2: {   // umbrella: a flat crown on a tall trunk
                int nb = T.bare ? 3 : (near ? (int)((7 + 4 * hash01(mix64(T.seed + 3))) * L.count) : midN);
                for (int k = 0; k < nb; k++) {
                    uint64_t hb = mix64(T.seed + 100 + k);
                    double a = hash01(hb) * TAU, rr = T.r * (near ? 0.6 : 0.4) * std::sqrt(hash01(mix64(hb + 1)));
                    double ox = std::cos(a) * rr + swx, oz = std::sin(a) * rr + swz, oy = (hash01(mix64(hb + 2)) - 0.5) * T.h * 0.06;
                    if (k < 3 && near) branchTo(cx + ox, T.gy + T.cy + oy, cz + oz, 0.8);
                    double cr = near ? cR * 1.1 : T.r * 0.7;
                    cluster(hb, cx + ox, T.gy + T.cy + oy, cz + oz, cr, T.h * 0.09, dist, leafBank, tone0, litOf(ox, oy, oz), ragged);
                }
                break;
            }
            case 3: {   // tiered giant: three canopy layers
                for (int layer = 0; layer < 3; layer++) {
                    double ly = T.h * (0.45 + 0.25 * layer) - T.cy, lr = T.r * (layer == 2 ? 0.65 : 1.0);
                    int nb = T.bare ? 2 : (near ? (int)((5 + 3 * hash01(mix64(T.seed + 3 + layer))) * L.count) : 2);
                    for (int k = 0; k < nb; k++) {
                        uint64_t hb = mix64(T.seed + 100 + layer * 20 + k);
                        double a = hash01(hb) * TAU, rr = lr * (near ? 0.65 : 0.45) * std::sqrt(hash01(mix64(hb + 1)));
                        double ox = std::cos(a) * rr + swx * (0.5 + 0.5 * layer), oz = std::sin(a) * rr + swz * (0.5 + 0.5 * layer), oy = ly + (hash01(mix64(hb + 2)) - 0.5) * T.h * 0.08;
                        if (k < 2 && near) branchTo(cx + ox, T.gy + T.cy + oy, cz + oz, (ly + T.cy) * 0.9 / trunkTop);
                        double cr = near ? lr * 0.45 : lr * 0.6;
                        cluster(hb, cx + ox, T.gy + T.cy + oy, cz + oz, cr, cr * 0.65, dist, leafBank, tone0, litOf(ox, oy, oz), ragged);
                    }
                }
                break;
            }
            case 4: {   // fibrous stalks: tufts on stalks
                int nb = near ? (int)((7 + 4 * hash01(mix64(T.seed + 3))) * L.count) : 3;
                for (int k = 0; k < nb; k++) {
                    uint64_t hb = mix64(T.seed + 100 + k);
                    double a = hash01(hb) * TAU, rr = T.r * 0.9 * hash01(mix64(hb + 1));
                    double ox = std::cos(a) * rr, oz = std::sin(a) * rr, oy = T.h * (0.5 + 0.5 * hash01(mix64(hb + 2)));
                    if (near) limb(T.x, T.gy, T.z, T.x + ox, T.gy + oy, T.z + oz, w0 * 0.5, w0 * 0.2, trunkShade, sideLit, &barkTile, texW);
                    else line(T.x, T.gy, T.z, T.x + ox, T.gy + oy, T.z + oz, trunkShade, BANK_BARK, rPx > 8 ? 2 : 1);
                    cluster(hb, T.x + ox, T.gy + oy, T.z + oz, T.r * 0.34, T.r * 0.26, dist, leafBank, tone0, litOf(ox, oy - T.cy, oz), ragged);
                }
                break;
            }
            case 5: {   // fern tree: arching fronds from the top, sprays along them
                int nf = near ? 7 + (int)(3 * hash01(mix64(T.seed + 3))) : 5;
                for (int k = 0; k < nf; k++) {
                    uint64_t hb = mix64(T.seed + 100 + k);
                    double a = hash01(hb) * TAU + 0.15 * std::sin(t * 1.3 + k) * wind, l = T.r * (0.8 + 0.4 * hash01(mix64(hb + 1)));
                    double x0 = cx, y0 = T.gy + T.cy, z0 = cz;
                    double x1 = x0 + std::cos(a) * l * 0.55, y1 = y0 + l * (0.35 - 0.2 * L.droop), z1 = z0 + std::sin(a) * l * 0.55;
                    double x2 = x0 + std::cos(a) * l, y2 = y0 - l * (0.25 + 0.3 * L.droop), z2 = z0 + std::sin(a) * l;
                    double sh = fogShade(leafBase * tone0 * (0.6 + 0.4 * std::max(0.0, dot(normalize(Vec3(std::cos(a), 0.6, std::sin(a))), sd))), dist);
                    if (dist < 70 && rPx > 6) frond(x0, y0, z0, x1, y1, z1, x2, y2, z2, l * 0.22, sh, leafBank);
                    else { line(x0, y0, z0, x1, y1, z1, sh, leafBank, 1); line(x1, y1, z1, x2, y2, z2, sh, leafBank, 1); }
                }
                break;
            }
            case 6: {   // mushroom tree: a cap on a thick stem, dark underside
                int nb = near ? (int)((6 + 3 * hash01(mix64(T.seed + 3))) * L.count) : 3;
                for (int k = 0; k < nb; k++) {
                    uint64_t hb = mix64(T.seed + 100 + k);
                    double a = hash01(hb) * TAU, rr = T.r * (near ? 0.5 : 0.35) * std::sqrt(hash01(mix64(hb + 1)));
                    double ox = std::cos(a) * rr, oz = std::sin(a) * rr;
                    cluster(hb, cx + ox, T.gy + T.cy, cz + oz, near ? T.r * 0.55 : T.r * 0.7, T.r * 0.28, dist, leafBank, tone0, litOf(ox, 0, oz), ragged * 0.7);
                }
                if (near) {   // the gills under the cap
                    for (int k = 0; k < 12; k++) {
                        uint64_t hb = mix64(T.seed + 300 + k);
                        double a = hash01(hb) * TAU, rr = T.r * (0.3 + 0.6 * hash01(mix64(hb + 1)));
                        point(cx + std::cos(a) * rr, T.gy + T.cy - T.r * 0.2, cz + std::sin(a) * rr, fogShade(leafBase * 0.4, dist), leafBank, 1);
                    }
                }
                break;
            }
            case 7: {   // weeping: a dome with strands hanging from its rim, small clusters along them
                int nb = T.bare ? 3 : (near ? (int)((6 + 4 * hash01(mix64(T.seed + 3))) * L.count) : midN);
                for (int k = 0; k < nb; k++) {
                    uint64_t hb = mix64(T.seed + 100 + k);
                    double a = hash01(hb) * TAU, rr = T.r * (near ? 0.55 : 0.4) * std::sqrt(hash01(mix64(hb + 1)));
                    double ox = std::cos(a) * rr + swx, oz = std::sin(a) * rr + swz, oy = (hash01(mix64(hb + 2)) - 0.3) * T.r * 0.7;
                    if (k < 3 && near) branchTo(cx + ox, T.gy + T.cy + oy, cz + oz, 0.75);
                    double cr = near ? cR * (0.85 + 0.3 * hash01(mix64(hb + 3))) : T.r * 0.62;
                    cluster(hb, cx + ox, T.gy + T.cy + oy, cz + oz, cr, cr * 0.75, dist, leafBank, tone0, litOf(ox, oy, oz), ragged);
                }
                if (!T.bare) {
                    int ns = near ? 8 + (int)(4 * hash01(mix64(T.seed + 4))) : 5;
                    double hang = T.cy * (0.5 + 0.4 * L.droop);
                    for (int k = 0; k < ns; k++) {
                        uint64_t hb = mix64(T.seed + 400 + k);
                        double a = hash01(hb) * TAU, rr = T.r * (0.75 + 0.25 * hash01(mix64(hb + 1)));
                        double sx0 = cx + std::cos(a) * rr + swx, sz0 = cz + std::sin(a) * rr + swz, sy0 = T.gy + T.cy + T.r * 0.1;
                        double sway = wind * 0.25 * std::sin(t * 1.5 + k);
                        double sx1 = sx0 + wdx * sway + std::cos(a) * T.r * 0.15, sz1 = sz0 + wdz * sway + std::sin(a) * T.r * 0.15, sy1 = sy0 - hang * (0.7 + 0.3 * hash01(mix64(hb + 2)));
                        double sh = fogShade(leafBase * tone0 * 0.7, dist);
                        line(sx0, sy0, sz0, sx1, sy1, sz1, sh, leafBank, rPx > 10 ? 2 : 1);
                        if (near && dist < 45) {
                            double cr = cR * 0.35;
                            cluster(mix64(hb + 3), (sx0 + sx1) * 0.5, (sy0 + sy1) * 0.5, (sz0 + sz1) * 0.5, cr, cr * 1.3, dist, leafBank, tone0 * 0.9, 0.45, ragged);
                            cluster(mix64(hb + 4), sx1, sy1 + cr, sz1, cr * 0.8, cr, dist, leafBank, tone0 * 0.85, 0.4, ragged);
                        }
                    }
                }
                break;
            }
            case 8: {   // candelabra: a few thick branches rising from the trunk's top, a ball of leaves on each
                int nb = 3 + (int)(2.99 * hash01(mix64(T.seed + 3)));
                double top = T.gy + trunkTop;
                for (int k = 0; k < nb; k++) {
                    uint64_t hb = mix64(T.seed + 100 + k);
                    double a = (k + 0.4 * hash01(hb)) * TAU / nb, l = T.r * (0.55 + 0.35 * hash01(mix64(hb + 1)));
                    double ex = tx + std::cos(a) * l + swx, ez = tz + std::sin(a) * l + swz, ey = top + T.h * (0.35 + 0.2 * hash01(mix64(hb + 2)));
                    if (near || dist < 60) limb(tx, top - w0, tz, ex, ey, ez, w0 * 0.55, w0 * 0.15, trunkShade, sideLit, nullptr, 0);
                    else line(tx, top, tz, ex, ey, ez, trunkShade, BANK_BARK, rPx > 8 ? 2 : 1);
                    if (T.bare) continue;
                    double cr = near ? cR * 1.15 : T.r * 0.5;
                    cluster(hb, ex, ey + cr * 0.3, ez, cr, cr * 0.85, dist, leafBank, tone0, litOf(ex - cx, ey + cr * 0.3 - (T.gy + T.cy), ez - cz), ragged);
                    if (near) {
                        double mx = (tx + ex) * 0.5, mz = (tz + ez) * 0.5, my = (top + ey) * 0.5;
                        cluster(mix64(hb + 5), mx, my + cr * 0.2, mz, cr * 0.55, cr * 0.45, dist, leafBank, tone0 * 0.9, litOf(mx - cx, my - (T.gy + T.cy), mz - cz), ragged);
                    }
                }
                break;
            }
            case 9: {   // spire: small clusters stacked up a thin trunk, a bright tip
                int nb = near ? (int)((6 + 2 * hash01(mix64(T.seed + 3))) * L.count) : 3;
                for (int k = 0; k < nb; k++) {
                    double fk = (k + 0.5) / nb;
                    double rr = T.r * (1.15 - 0.8 * fk);
                    double oy = T.h * (0.25 + 0.7 * fk) - T.cy;
                    cluster(mix64(T.seed + 100 + k), cx + swx * fk, T.gy + T.cy + oy, cz + swz * fk, rr, rr * 1.2, dist, leafBank, tone0, litOf(0, oy, 0) * (0.85 + 0.15 * fk), ragged * 0.85);
                }
                if (near) point(cx + swx, T.gy + T.h, cz + swz, fogShade(leafBase * 1.15, dist), leafBank, rPx > 12 ? 2 : 1);
                break;
            }
        }
    };
    for (const TreeRef& tr : treeList) {
        if (tr.y1 < bandY0 || tr.y0 >= bandY1) continue;
        st.treesVisited++;
        drawTree(tr.T, tr.dist);
    }
    section(0);
    markCov = false;
    for (const FloraCell& cr : cells) {
        wallHere = rainWall((cr.cx + 0.5) * cs, (cr.cz + 0.5) * cs, cr.d) + fogSeaWall((cr.cx + 0.5) * cs, site.lod0.at(cr.cx, cr.cz).h, (cr.cz + 0.5) * cs, cr.d);
        {
            int cx = cr.cx, cz = cr.cz;
            double cellDist = cr.d;
            const TerrainVertex& tv = site.lod0.at(cx, cz);
            lampOn(Vec3((cx + 0.5) * cs, tv.h + 0.5, (cz + 0.5) * cs));
            uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0xB0BULL);
            bool underwater = tv.water > -1e8f && tv.h < tv.water + 0.5;
            // ---- fallen logs (B-315): a six-sided trunk along its heading, sunk a third into the ground, bark-textured,
            // the faces toward the camera and the sky drawn with their own light, a cut end toward the camera (pale wood
            // in a dark bark ring), one or two broken stubs, a root plate on some, moss on the upper faces of others
            if (cellDist < 260)
                forLogs(cx, cz, [&](const LogInst& Lg) {
                    double dist = std::sqrt((Lg.x - camPos.x) * (Lg.x - camPos.x) + (Lg.z - camPos.z) * (Lg.z - camPos.z));
                    if (Lg.radius / dist * proj.f < 0.8) return;
                    double R = Lg.radius;
                    double dx = std::sin(Lg.heading), dz = std::cos(Lg.heading);
                    double x0 = Lg.x - dx * Lg.len * 0.5, z0 = Lg.z - dz * Lg.len * 0.5, x1 = Lg.x + dx * Lg.len * 0.5, z1 = Lg.z + dz * Lg.len * 0.5;
                    Vec3 A(x0, site.groundHeight(x0, z0) + R * 0.7, z0), B(x1, site.groundHeight(x1, z1) + R * 0.7, z1);   // the axis
                    Vec3 axis = normalize(B - A);
                    Vec3 nn = normalize(cross(axis, Vec3(0, 1, 0)));
                    Vec3 upL = normalize(cross(nn, axis)); if (upL.y < 0) { upL = -upL; nn = -nn; }
                    uint64_t hs = mix64(Lg.seed + 5);
                    bool mossy = !Lg.stone && hash01(hs) < 0.45 && (tv.biome == BIO_TEMPERATE || tv.biome == BIO_TROPICAL || tv.biome == BIO_WETLAND || tv.biome == BIO_TAIGA);
                    bool roots = !Lg.stone && hash01(mix64(hs + 1)) < 0.3;
                    const int barkBank = Lg.stone ? 26 : BANK_BARK, woodBank = Lg.stone ? 26 : BANK_WOOD;   // the stone's own jasper colours (`setupPalette`)
                    double texW = clampd(1.0 - (dist - 20.0) / 40.0, 0.0, 1.0);
                    Vec3 mid = (A + B) * 0.5;
                    Vec3 toCam = normalize(Vec3(camPos.x - mid.x, camPos.y - mid.y, camPos.z - mid.z));
                    if (dist < 60) drawBlobShadow(fb, Lg.x, Lg.z, R * 1.3, R * 1.2);
                    auto ring = [&](const Vec3& c, double ang) { return c + nn * (R * std::cos(ang)) + upL * (R * std::sin(ang)); };
                    for (int k = 0; k < 6; k++) {
                        if (k == 3) continue;   // the bottom face is in the ground
                        double a0 = (k + 1) * TAU / 6, a1 = (k + 2) * TAU / 6, am = (a0 + a1) * 0.5;   // face 0 spans 60-120 degrees: the top
                        Vec3 N = nn * std::cos(am) + upL * std::sin(am);
                        if (dot(N, toCam) < -0.05) continue;
                        double amb = ambient * (0.55 + 0.45 * std::max(0.0, N.y));
                        double lt = amb + (1 - amb) * std::max(0.0, dot(N, sd)) * sunUp * lf + (nLamps ? (1 - amb) * lampAt(mid, &N) : 0.0);   // the lamp
                        bool moss = mossy && N.y > 0.45;
                        double shade = fogShade((moss ? 12 : 16) + (moss ? 20 : 26) * std::pow(lt, 0.7), dist);   // never darker than a trunk
                        RVert q[4] = {vert(ring(A, a0).x, ring(A, a0).y, ring(A, a0).z, shade), vert(ring(A, a1).x, ring(A, a1).y, ring(A, a1).z, shade),
                                      vert(ring(B, a1).x, ring(B, a1).y, ring(B, a1).z, shade), vert(ring(B, a0).x, ring(B, a0).y, ring(B, a0).z, shade)};
                        bool ok = true; for (RVert& v : q) if (v.z < NEAR_Z) ok = false;
                        if (!ok) continue;
                        double arc0 = a0 * R * 8, arc1 = a1 * R * 8, vl = Lg.len * 2;
                        q[0].u = arc0; q[0].v = 0; q[1].u = arc1; q[1].v = 0; q[2].u = arc1; q[2].v = vl; q[3].u = arc0; q[3].v = vl;
                        RasterParams rp; rp.bank = moss ? 3 : barkBank;
                        if (texW > 0.02 && !moss) { rp.grain2 = &barkTile; rp.grain2Scale = 1.0; rp.grainWeight = texW; }
                        rasterPolygon(fb, q, 4, rp, proj);
                        st.floraPolys++;
                    }
                    {   // the cut end toward the camera: the bark ring, then the paler wood inside it
                        bool endB = dot(axis, toCam) > 0;
                        const Vec3& E = endB ? B : A;
                        Vec3 N = endB ? axis : -axis;
                        double amb = ambient * 0.8;
                        double lt = amb + (1 - amb) * std::max(0.0, dot(N, sd)) * sunUp * lf + (nLamps ? (1 - amb) * lampAt(E, &N) : 0.0);
                        double wood = fogShade(44 * std::pow(lt, 0.6), dist), bark = fogShade(26 * std::pow(lt, 0.7), dist);
                        RVert q[8]; bool ok = true;
                        for (int k = 0; k < 8; k++) { Vec3 pnt = ring(E, k * TAU / 8); q[k] = vert(pnt.x, pnt.y, pnt.z, bark); if (q[k].z < NEAR_Z) ok = false; }
                        if (ok) {
                            RasterParams rp; rp.bank = barkBank;
                            rasterPolygon(fb, q, 8, rp, proj);
                            for (int k = 0; k < 8; k++) {
                                Vec3 pnt = E + nn * (R * 0.78 * std::cos(k * TAU / 8)) + upL * (R * 0.78 * std::sin(k * TAU / 8));
                                q[k] = vert(pnt.x, pnt.y, pnt.z, Lg.stone ? wood + 6 : wood);
                            }
                            RasterParams rp2; rp2.bank = woodBank; rp2.zbias = 1.0005;   // the same plane: the bias lets it win
                            if (dist < 40) { rp2.grain = &grainFine; rp2.grainScale = 6.0; }
                            for (int k = 0; k < 8; k++) { q[k].u = (q[k].x - E.x) * 4; q[k].v = (q[k].y - E.y) * 4; }
                            rasterPolygon(fb, q, 8, rp2, proj);
                            st.floraPolys += 2;
                        }
                    }
                    if (dist < 90 && !Lg.stone) {   // broken stubs on the upper half
                        int ns = 1 + (hash01(mix64(hs + 2)) < 0.5 ? 1 : 0);
                        double stubShade = fogShade(20 + 8 * env.skyBrightness * sunUp, dist);
                        for (int k = 0; k < ns; k++) {
                            uint64_t hk = mix64(hs + 10 + k);
                            double tt = 0.2 + 0.6 * hash01(hk), ang = (0.25 + 0.5 * hash01(mix64(hk + 1))) * PI;
                            Vec3 base = A + (B - A) * tt + nn * (R * std::cos(ang)) + upL * (R * std::sin(ang));
                            Vec3 dir = normalize(nn * std::cos(ang) + upL * std::sin(ang) + axis * (0.3 * (hash01(mix64(hk + 2)) - 0.5)));
                            double len = R * (1.2 + 1.5 * hash01(mix64(hk + 3)));
                            Vec3 tip = base + dir * len;
                            limb(base.x, base.y, base.z, tip.x, tip.y, tip.z, R * 0.25, R * 0.07, stubShade, sideLit, nullptr, 0);
                        }
                    }
                    if (roots && dist < 70) {   // the root plate at the far end: a fan of roots in the plane of the cut
                        const Vec3& E = dot(axis, toCam) > 0 ? A : B;
                        double rootShade = fogShade(18 + 8 * env.skyBrightness * sunUp, dist);
                        for (int k = 0; k < 9; k++) {
                            double ang = k * TAU / 9 + 0.3 * hash01(mix64(hs + 30 + k)), len = R * (1.4 + 0.9 * hash01(mix64(hs + 40 + k)));
                            Vec3 rd = nn * std::cos(ang) + upL * std::sin(ang);
                            Vec3 tip = E + rd * len;
                            if (tip.y < site.groundHeight(tip.x, tip.z) + 0.05) continue;
                            line(E.x + rd.x * R * 0.3, E.y + rd.y * R * 0.3, E.z + rd.z * R * 0.3, tip.x, tip.y, tip.z, rootShade, BANK_BARK, dist < 25 ? 2 : 1);
                        }
                    }
                });
            if (deadKind(site.gen.type)) continue;   // on a dead world nothing else grows
            if (underwater && !(tv.material == MAT_WATER && tv.biome == BIO_WETLAND)) continue;
            // ---- undergrowth in forests: ferns, bushes with berries, mushrooms
            if (tv.material == MAT_FOREST && cellDist < 60 && tv.veg > 0.3) {
                double dens = canopyDensityAt(cx * cs + 8, cz * cs + 8, tv.veg);
                int nf = (int)(2 + 4 * dens * hash01(mix64(h + 61)));
                for (int i = 0; i < nf; i++) {
                    uint64_t hc = mix64(h + 1300 + 31 * i);
                    double px = cx * cs + hash01(hc) * cs, pz = cz * cs + hash01(mix64(hc + 1)) * cs;
                    double gy = site.groundHeight(px, pz);
                    double dist = std::sqrt((px - camPos.x) * (px - camPos.x) + (pz - camPos.z) * (pz - camPos.z));
                    double fl = (0.5 + 0.6 * hash01(mix64(hc + 2))) * treeScale;
                    if (fl / dist * proj.f < 1.5) continue;
                    double sh = fogShade(leafBase * 0.62 * (0.85 + 0.3 * hash01(mix64(hc + 3))), dist);
                    int nfr = dist < 20 ? 6 : 4;
                    for (int k = 0; k < nfr; k++) {   // arching fronds (B-315: leaf strips close by, lines beyond)
                        double a = k * TAU / nfr + hash01(mix64(hc + 10 + k)) * 0.6 + 0.08 * std::sin(t * 1.7 + px) * wind;
                        double x1 = px + std::cos(a) * fl * 0.5, y1 = gy + fl * 0.55, z1 = pz + std::sin(a) * fl * 0.5;
                        double x2 = px + std::cos(a) * fl, y2 = gy + fl * 0.25, z2 = pz + std::sin(a) * fl;
                        if (dist < 28) frond(px, gy + 0.05, pz, x1, y1, z1, x2, y2, z2, fl * 0.2, sh, 3);
                        else { line(px, gy, pz, x1, y1, z1, sh, 3, 1); line(x1, y1, z1, x2, y2, z2, sh, 3, 1); }
                    }
                }
                if (cellDist < 30 && hash01(mix64(h + 62)) < 0.25) {   // mushrooms: tan caps close to the ground
                    int nm = 2 + (int)(3 * hash01(mix64(h + 63)));
                    for (int i = 0; i < nm; i++) {
                        uint64_t hc = mix64(h + 1400 + i);
                        double px = cx * cs + hash01(hc) * cs, pz = cz * cs + hash01(mix64(hc + 1)) * cs;
                        double gy = site.groundHeight(px, pz);
                        double dist = std::sqrt((px - camPos.x) * (px - camPos.x) + (pz - camPos.z) * (pz - camPos.z));
                        if (0.2 / dist * proj.f < 1.0) continue;
                        point(px, gy + 0.12 + 0.1 * hash01(mix64(hc + 2)), pz, fogShade(40 * std::pow(light, 0.7), dist), 9, dist < 6 ? 2 : 1);
                    }
                }
            }
            // ---- bushes on grass and at the forest edge, some with berries. R-306: shrubland: 0.5-4 bushes a cell on grass
            // (savanna the most, tundra cushions, desert scrub), to 320 m, the big ones two clusters; the plains read empty
            // with 0-2 bushes a cell within 200 m
            if ((tv.material == MAT_GRASS || (tv.material == MAT_FOREST && tv.veg < 0.6) || (tv.material == MAT_SAND && tv.biome == BIO_DESERT)) && (tv.veg > 0.2 || tv.biome == BIO_DESERT) &&
                    cellDist < 320) {
                double per = tv.biome == BIO_SAVANNA ? 1.6 : (tv.biome == BIO_TUNDRA ? 0.7 : (tv.biome == BIO_DESERT ? 0.35 : 1.0));
                double nb = (0.5 + tv.veg * 3.5) * per * hash01(mix64(h + 7)) * 1.6;
                if (tv.material == MAT_FOREST) nb *= 0.6;
                int nbush = (int)nb + (hash01(mix64(h + 8)) < nb - (int)nb ? 1 : 0);
                for (int i = 0; i < nbush; i++) {
                    uint64_t hb = mix64(h + 700 + i);
                    double bx = cx * cs + hash01(hb) * cs, bz = cz * cs + hash01(mix64(hb + 1)) * cs;
                    double dist = std::sqrt((bx - camPos.x) * (bx - camPos.x) + (bz - camPos.z) * (bz - camPos.z));
                    double bh = (0.5 + 1.5 * hash01(mix64(hb + 2)) * (tv.biome == BIO_TUNDRA ? 0.4 : 1.0)) * treeScale;
                    if (bh / dist * proj.f < 1.2) continue;
                    double gy = site.groundHeight(bx, bz);
                    if (gy < site.waterAt(bx, bz) + 0.3) continue;
                    int bbank = hash01(mix64(hb + 5)) < 0.2 ? 10 : (hash01(mix64(hb + 9)) < 0.25 ? 5 : 3);
                    double tone = tv.biome == BIO_DESERT ? 0.75 : 0.95;
                    cluster(hb, bx, gy + bh * 0.5, bz, bh * 0.55, bh * 0.45, dist, bbank, tone, 0.5 + 0.35 * sunUp * clampd(sd.y, 0, 1), 1.1);
                    if (bh > 1.4 && dist < 120) cluster(mix64(hb + 11), bx + bh * 0.35, gy + bh * 0.42, bz + bh * 0.2, bh * 0.4, bh * 0.35, dist, bbank, tone * 0.92, 0.45 + 0.3 * sunUp, 1.2);
                    if (dist < 40 && hash01(mix64(hb + 6)) < 0.3) {   // berries
                        int nberry = 3 + (int)(5 * hash01(mix64(hb + 7)));
                        for (int k = 0; k < nberry; k++) {
                            uint64_t hk = mix64(hb + 40 + k);
                            double a = hash01(hk) * TAU, rr = bh * 0.5 * std::sqrt(hash01(mix64(hk + 1)));
                            point(bx + std::cos(a) * rr, gy + bh * (0.3 + 0.5 * hash01(mix64(hk + 2))), bz + std::sin(a) * rr, fogShade(52 * std::pow(light, 0.6), dist), 5, 1);
                        }
                    }
                }
            }
            // ---- R-306: mounds on warm open ground (one cell in six: a cluster of 1-3 earth cones 0.8-2.5 m), and grass tufts
            // out to 120 m, so a plain has something standing on it at every distance
            if (tv.material == MAT_GRASS && (tv.biome == BIO_SAVANNA || tv.biome == BIO_GRASSLAND) && cellDist < 260 && hash01(mix64(h + 79)) < 0.16) {
                int nm = 1 + (int)(2.99 * hash01(mix64(h + 80)));
                for (int i = 0; i < nm; i++) {
                    uint64_t hm = mix64(h + 2000 + i);
                    double mx = cx * cs + 3 + hash01(hm) * (cs - 6), mz = cz * cs + 3 + hash01(mix64(hm + 1)) * (cs - 6);
                    double dist = std::sqrt((mx - camPos.x) * (mx - camPos.x) + (mz - camPos.z) * (mz - camPos.z));
                    double mh = (0.8 + 1.7 * hash01(mix64(hm + 2))) * treeScale, mw = mh * 0.38;
                    if (mh / dist * proj.f < 1.2) continue;
                    double gy = site.groundHeight(mx, mz);
                    if (gy < site.waterAt(mx, mz) + 0.3) continue;
                    double lit = ambient + (1 - ambient) * std::max(0.0, dot(normalize(Vec3(-right.x, 0.8, -right.z)), sd)) * sunUp * lf + (light - light0);   // the cell's lamp
                    limb(mx, gy - 0.1, mz, mx, gy + mh, mz, mw, mw * 0.18, fogShade(38 * std::pow(lit, 0.7), dist), sideLit, dist < 30 ? &barkTile : nullptr, 0.5);
                }
            }
            // ---- meadows: tall swaying grass and flowers close by
            if ((tv.material == MAT_GRASS || tv.material == MAT_FOREST) && cellDist < 120 && tv.veg > 0.3 && tv.biome != BIO_DESERT) {
                bool tall = tv.material == MAT_GRASS && tv.veg > 0.45 && cellDist < 45;
                int nt = tall ? 8 + (int)(tv.veg * 8) : (cellDist < 60 ? 5 + (int)(tv.veg * 8) : 3 + (int)(tv.veg * 4));   // R-306: fewer, farther
                if (tv.material == MAT_FOREST) nt /= 2;
                for (int i = 0; i < nt; i++) {
                    uint64_t hg = mix64(h + 900 + i);
                    double gx = cx * cs + hash01(hg) * cs, gz = cz * cs + hash01(mix64(hg + 1)) * cs;
                    double gy = site.groundHeight(gx, gz);
                    if (gy < site.waterAt(gx, gz) + 0.1) continue;
                    double hh = (tall ? 0.8 + 0.6 * hash01(mix64(hg + 2)) : 0.25 + 0.45 * hash01(mix64(hg + 2))) * treeScale;
                    double dist = std::sqrt((gx - camPos.x) * (gx - camPos.x) + (gz - camPos.z) * (gz - camPos.z));
                    if (hh / dist * proj.f < 1.0) continue;
                    double sh = fogShade(44 * std::pow(light, 0.7) * (0.7 + 0.4 * hash01(mix64(hg + 3))), dist);
                    double sway = hh * (0.12 + 0.3 * wind) * std::sin(t * 1.6 + gx * 0.35 + gz * 0.2);
                    if (tall) {
                        double mx = gx + wdx * sway * 0.4, mz = gz + wdz * sway * 0.4;
                        line(gx, gy, gz, mx, gy + hh * 0.6, mz, sh, 10, 1);
                        line(mx, gy + hh * 0.6, mz, gx + wdx * sway, gy + hh * 0.95, gz + wdz * sway, sh, 10, 1);
                    } else line(gx, gy, gz, gx + wdx * sway, gy + hh, gz + wdz * sway, sh, 10, 1);
                }
                if (bloom && cellDist < 50 && tv.material == MAT_GRASS && (tv.biome == BIO_GRASSLAND || tv.biome == BIO_TEMPERATE || tv.biome == BIO_SAVANNA || tv.biome == BIO_NONE) &&
                        hash01(mix64(h + 64)) < 0.5) {
                    int nfl = 3 + (int)(6 * hash01(mix64(h + 65)));
                    int fbank = hash01(mix64(h + 66)) < 0.5 ? 5 : 8;
                    for (int i = 0; i < nfl; i++) {
                        uint64_t hc = mix64(h + 1500 + i);
                        double px = cx * cs + hash01(hc) * cs, pz = cz * cs + hash01(mix64(hc + 1)) * cs;
                        double gy = site.groundHeight(px, pz);
                        double dist = std::sqrt((px - camPos.x) * (px - camPos.x) + (pz - camPos.z) * (pz - camPos.z));
                        if (0.3 / dist * proj.f < 1.0) continue;
                        point(px, gy + 0.25 + 0.3 * hash01(mix64(hc + 2)), pz, fogShade(50 * std::pow(light, 0.6), dist), fbank, 1);
                    }
                }
            }
            // ---- water edges: reeds and cattails; still wetland water: lily pads; dusk: fireflies
            bool shore = tv.water > -1e8f && tv.h < tv.water + 1.5 && tv.h > tv.water - 0.6;
            if (shore && cellDist < 80 && tv.biome != BIO_ICE && tv.biome != BIO_TUNDRA) {
                int nr = 6 + (int)(8 * hash01(mix64(h + 71)));
                for (int i = 0; i < nr; i++) {
                    uint64_t hc = mix64(h + 1600 + i);
                    double px = cx * cs + hash01(hc) * cs, pz = cz * cs + hash01(mix64(hc + 1)) * cs;
                    double gy = site.groundHeight(px, pz), wl = site.waterAt(px, pz);
                    if (gy > wl + 1.5 || gy < wl - 0.6) continue;
                    double base = std::max(gy, wl);
                    double dist = std::sqrt((px - camPos.x) * (px - camPos.x) + (pz - camPos.z) * (pz - camPos.z));
                    double hh = (1.2 + 1.2 * hash01(mix64(hc + 2))) * treeScale;
                    if (hh / dist * proj.f < 1.0) continue;
                    double sway = hh * (0.08 + 0.25 * wind) * std::sin(t * 1.4 + px * 0.5);
                    double sh = fogShade(42 * std::pow(light, 0.7) * (0.8 + 0.3 * hash01(mix64(hc + 3))), dist);
                    line(px, base, pz, px + wdx * sway, base + hh, pz + wdz * sway, sh, 10, dist < 12 ? 2 : 1);
                    if (hash01(mix64(hc + 4)) < 0.4) point(px + wdx * sway, base + hh * 0.92, pz + wdz * sway, fogShade(16, dist), 0, dist < 12 ? 2 : 1);   // the cattail head
                }
            }
            if (tv.material == MAT_WATER && tv.biome == BIO_WETLAND && cellDist < 50 && hash01(mix64(h + 72)) < 0.6) {
                int np = 3 + (int)(6 * hash01(mix64(h + 73)));
                for (int i = 0; i < np; i++) {
                    uint64_t hc = mix64(h + 1700 + i);
                    double px = cx * cs + hash01(hc) * cs, pz = cz * cs + hash01(mix64(hc + 1)) * cs;
                    double wl = site.waterAt(px, pz);
                    if (wl < -1e8 || site.groundHeight(px, pz) > wl - 0.2) continue;
                    double dist = std::sqrt((px - camPos.x) * (px - camPos.x) + (pz - camPos.z) * (pz - camPos.z));
                    double rr = 0.25 + 0.2 * hash01(mix64(hc + 2));
                    if (rr / dist * proj.f < 1.2) continue;
                    RVert q[6];
                    bool ok = true;
                    for (int k = 0; k < 6; k++) {
                        double a = k * TAU / 6;
                        q[k] = vert(px + std::cos(a) * rr, wl + 0.03, pz + std::sin(a) * rr, fogShade(leafBase * 0.8, dist));
                        if (q[k].z < NEAR_Z) ok = false;
                    }
                    if (!ok) continue;
                    RasterParams rp; rp.bank = 3;
                    rasterPolygon(fb, q, 6, rp, proj);
                }
            }
            if (dusk && cellDist < 40 && (tv.biome == BIO_WETLAND || (tv.material == MAT_FOREST && tv.veg > 0.5)) && hash01(mix64(h + 74)) < 0.5) {
                int nff = 2 + (int)(4 * hash01(mix64(h + 75)));
                double glow = clampd((0.35 - env.skyBrightness) / 0.3, 0, 1);
                for (int i = 0; i < nff; i++) {
                    uint64_t hc = mix64(h + 1800 + i);
                    double ph = hash01(hc) * TAU, sp = 0.3 + 0.4 * hash01(mix64(hc + 1));
                    double px = cx * cs + hash01(mix64(hc + 2)) * cs + 1.5 * std::sin(t * sp + ph), pz = cz * cs + hash01(mix64(hc + 3)) * cs + 1.5 * std::cos(t * sp * 0.8 + ph);
                    double py = site.surfaceHeight(px, pz) + 0.5 + 1.5 * hash01(mix64(hc + 4)) + 0.3 * std::sin(t * 1.1 + ph);
                    if (std::sin(t * 3.0 + ph * 3) < 0.3) continue;   // blinking
                    RVert p = vert(px, py, pz, 40 + 20 * glow);
                    if (p.z < NEAR_Z) continue;
                    rasterPoint3(fb, p, 11, proj, true, 1);
                }
            }
            // ---- deserts: cacti and succulents; tundra and alpine rock: cushions and lichen
            if (cellDist < 300 && ((tv.material == MAT_SAND && tv.biome == BIO_DESERT) || tv.biome == BIO_TUNDRA || (tv.biome == BIO_ALPINE && cellDist < 60))) {
                bool cactus = tv.biome == BIO_DESERT;
                int np = (int)((cactus ? 1.2 : 2.5) * hash01(mix64(h + 61)));
                for (int i = 0; i < np; i++) {
                    uint64_t hc = mix64(h + 900 + 31 * i);
                    double px = cx * cs + hash01(hc) * cs, pz = cz * cs + hash01(mix64(hc + 1)) * cs;
                    double gy = site.groundHeight(px, pz);
                    double dist = std::sqrt((px - camPos.x) * (px - camPos.x) + (pz - camPos.z) * (pz - camPos.z));
                    if (cactus) {
                        if (hash01(mix64(hc + 9)) < 0.35) {   // a succulent rosette: a flat spray
                            double rr = 0.3 + 0.4 * hash01(mix64(hc + 2));
                            if (rr / dist * proj.f < 1.0) continue;
                            cluster(hc, px, gy + rr * 0.3, pz, rr, rr * 0.5, dist, 3, 0.9, 0.6, 0.8);
                            continue;
                        }
                        double hh = (1.2 + 2.5 * hash01(mix64(hc + 2))) * treeScale;
                        if (hh / dist * proj.f < 1.5) continue;
                        double sh = fogShade(44 * std::pow(light, 0.7), dist);
                        line(px, gy, pz, px, gy + hh, pz, sh, 3, dist < 40 ? 3 : (dist < 120 ? 2 : 1));
                        for (int arm = 0; arm < 2; arm++) {
                            double ay = gy + hh * (0.35 + 0.25 * arm), ad = (arm ? -1 : 1) * 0.35 * hh;
                            line(px, ay, pz, px + ad, ay + 0.3 * hh, pz, sh, 3, dist < 40 ? 2 : 1);
                        }
                    } else {
                        double rr = 0.3 + 0.6 * hash01(mix64(hc + 2));
                        if (rr / dist * proj.f < 1.0) continue;
                        cluster(hc, px, gy + rr * 0.2, pz, rr, rr * 0.35, dist, 3, 0.7, 0.55, 0.9);
                    }
                }
                if (!cactus && cellDist < 30 && tv.material != MAT_SNOW) {   // lichen crusts on cold rock
                    int nl = 3 + (int)(6 * hash01(mix64(h + 76)));
                    for (int i = 0; i < nl; i++) {
                        uint64_t hc = mix64(h + 1900 + i);
                        double px = cx * cs + hash01(hc) * cs, pz = cz * cs + hash01(mix64(hc + 1)) * cs;
                        double dist = std::sqrt((px - camPos.x) * (px - camPos.x) + (pz - camPos.z) * (pz - camPos.z));
                        if (0.25 / dist * proj.f < 0.8) continue;
                        point(px, site.groundHeight(px, pz) + 0.05, pz, fogShade(40 * std::pow(light, 0.7), dist), 9, dist < 8 ? 2 : 1);
                    }
                }
            }
        }
    }
    section(1);   // F-07: the cells' whole time (a clock read per cell and part was ~2,700 a frame)
}
