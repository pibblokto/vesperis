// E-08: the terrain rings: the material tiles, a vertex's height, light and colour (`vertexOf`: the shadow march, the geomorph
// onto the coarser ring, the water), the vertex cache's warm (`warmTerrainLOD`), the bin and the band draw (`binTerrainLOD`,
// `drawTerrainLOD`), the reflections, the floor beyond the far ring, and the cached ground the camera's march and the forms
// read (`heightNear`, `drawnGround`, `rangeToGround`)
#include "surface_internal.h"

// the material tiles of `want` (a bit a material) not made yet, made together, a material to a worker (a tile is a function of
// its material and the seed, each into its own slot; ~1 ms each, five or six at a landing's first ring on a peopled world)
void SurfaceView::buildMesoTiles(uint32_t want) {
    static_assert(MAT_COUNT <= 32, "a bit a material");
    int mats[MAT_COUNT], k = 0;
    for (int m = 0; m < MAT_COUNT; m++) if ((want >> m & 1u) && !mesoBuilt[m] && materialHasMesoTile(m)) mats[k++] = m;
    parallelFor(k, 1, [&](int b, int e) { for (int i = b; i < e; i++) buildMesoTile(mesoTiles[mats[i]], mats[i], site.gen.seed); });
    for (int i = 0; i < k; i++) mesoBuilt[mats[i]] = true;
}

const GrainTexture* SurfaceView::mesoFor(int material) {
    if (material < 0 || material >= MAT_COUNT || !materialHasMesoTile(material)) return nullptr;
    if (!mesoBuilt[material]) { buildMesoTile(mesoTiles[material], material, site.gen.seed); mesoBuilt[material] = true; }
    return &mesoTiles[material];
}

// Fraction of direct sunlight reaching a vertex (M10-06): march toward the sun over the
// height cache; anything rising above the sun ray shadows it, with a soft edge.
double SurfaceView::castShadowLit(TerrainCache& cache, int cx, int cz, double h, const Vec3& sd) {
    double horiz = std::sqrt(sd.x * sd.x + sd.z * sd.z);
    if (horiz < 1e-4 || sd.y <= 0) return 1.0;
    double tanAlt = sd.y / horiz;
    if (tanAlt > 2.5) return 1.0;               // above 68 degrees nothing casts at this resolution
    double dx = sd.x / horiz, dz = sd.z / horiz;
    double cs = cache.cellSize;
    double x0 = cx * cs, z0 = cz * cs;
    double limit2 = (double)curRadius * cs * curRadius * cs;
    double maxDepth = 0;
    for (int i = 1; i <= 9; i++) {
        double s = cs * i * (1.0 + 0.12 * i);
        double px = x0 + dx * s, pz = z0 + dz * s;
        double ex = px - camPos.x, ez = pz - camPos.z;
        if (ex * ex + ez * ez > limit2) break;   // stay inside the cached ring
        double th = cache.at((int)std::lround(px / cs), (int)std::lround(pz / cs)).h;
        double rayH = h + s * tanAlt + 0.35;
        if (th - rayH > maxDepth) maxDepth = th - rayH;
    }
    return 1.0 - 0.6 * smoothstep(0.0, 3.5 + 0.03 * cs, maxDepth);   // never fully black: light bounces around
}

// F-04, F-05: the rows a convex polygon can fill: clipped at the near plane as rasterPolygon clips it, projected as it projects, a row of
// slack each side; false when it fills none (wholly beside, above or below the frame: the rasteriser rejects it). `every` is set when a
// point projects to no finite place
static bool polygonRows(const Proj& pj, const Vec3* p, int np, int& y0, int& y1, bool& every) {
    double minx = 1e300, maxx = -1e300, miny = 1e300, maxy = -1e300;
    int m = 0;
    auto at = [&](double x, double y, double z) {
        const double iz = 1.0 / z, sx = pj.cx + pj.f * x * iz, sy = pj.cy - pj.f * y * iz;
        if (!std::isfinite(sx) || !std::isfinite(sy)) every = true;
        minx = std::min(minx, sx); maxx = std::max(maxx, sx); miny = std::min(miny, sy); maxy = std::max(maxy, sy);
        m++;
    };
    for (int i = 0; i < np; i++) {   // rasterPolygon's clip against z = NEAR_Z
        const Vec3& a = p[i]; const Vec3& b = p[(i + 1) % np];
        const bool ain = a.z >= NEAR_Z, bin = b.z >= NEAR_Z;
        if (ain) at(a.x, a.y, a.z);
        if (ain != bin) { const double t = (NEAR_Z - a.z) / (b.z - a.z); at(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t); }
    }
    if (m < 3 || maxx < -1 || minx > FBW + 1) return false;
    const double ylo = std::ceil(miny - 0.5) - 1, yhi = std::floor(maxy - 0.5) + 1;
    if (yhi < 0 || ylo > FBH - 1) return false;
    y0 = (int)std::max(-1.0, ylo); y1 = (int)std::min((double)FBH, yhi);
    return true;
}

// M1-04: the land mirrored about the sea level, blended into water pixels only (bank 2), far to near. F-05: the polygons gathered here
// (serially) with their rows, then drawn by the terrain's band threads (drawReflectionBand)
void SurfaceView::gatherReflections() {
    reflVerts.clear(); reflPolys.clear();
    struct Cell { double d2; int cx, cz; int lod; double level; };
    std::vector<Cell> cells;
    double sea = site.seaLevel + site.tideM;   // the tide's
    for (int lod = 1; lod >= 0; lod--) {
        TerrainCache& cache = lod == 0 ? site.lod0 : site.lod1;
        double cs = cache.cellSize;
        int R = lod == 0 ? 20 : 20;
        int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
        for (int cz = pcz - R; cz < pcz + R; cz++)
            for (int cx = pcx - R; cx < pcx + R; cx++) {
                double dx = cx + 0.5 - camPos.x / cs, dz = cz + 0.5 - camPos.z / cs;
                double d2 = dx * dx + dz * dz;
                if (d2 > (double)R * R) continue;
                if (lod == 1 && d2 < 5.5 * 5.5) continue;   // covered by lod0
                double wx = (cx + 0.5) * cs - camPos.x, wz = (cz + 0.5) * cs - camPos.z;
                double dd = std::sqrt(wx * wx + wz * wz);
                if (dd > cs * 3 && (wx * camLocal.m[2][0] + wz * camLocal.m[2][2]) / dd < -0.25) continue;
                double hmax = std::max(std::max(cache.at(cx, cz).h, cache.at(cx + 1, cz).h), std::max(cache.at(cx, cz + 1).h, cache.at(cx + 1, cz + 1).h));
                // a cell on the shore of a lake or a river (its vertices carry that water's level) mirrors round
                // that level; every cell above the sea mirrors round the sea
                double wl = std::max(std::max(site.waterOf(cache.at(cx, cz)), site.waterOf(cache.at(cx + 1, cz))), std::max(site.waterOf(cache.at(cx, cz + 1)), site.waterOf(cache.at(cx + 1, cz + 1))));
                if (wl > sea + 0.5 && hmax > wl + 0.3) cells.push_back({d2 * cs * cs, cx, cz, lod, wl});
                if (hmax < sea + 0.3) continue;
                cells.push_back({d2 * cs * cs, cx, cz, lod, sea});
            }
    }
    std::sort(cells.begin(), cells.end(), [](const Cell& a, const Cell& b) { return a.d2 > b.d2; });
    for (const Cell& c : cells) {
        TerrainCache& cache = c.lod == 0 ? site.lod0 : site.lod1;
        std::vector<VtxCache>& vcache = c.lod == 0 ? vc0 : vc1;
        double cs = cache.cellSize;
        uint8_t bk;
        const RVert* vs[4] = {&vertexOf(cache, vcache, c.cx, c.cz, bk), &vertexOf(cache, vcache, c.cx + 1, c.cz, bk),
                              &vertexOf(cache, vcache, c.cx + 1, c.cz + 1, bk), &vertexOf(cache, vcache, c.cx, c.cz + 1, bk)};
        int ox[4] = {0, 1, 1, 0}, oz[4] = {0, 0, 1, 1};
        double flat = 0, hs[4], xs[4], zs[4];
        for (int i = 0; i < 4; i++) {
            xs[i] = (c.cx + ox[i]) * cs; zs[i] = (c.cz + oz[i]) * cs;
            hs[i] = cache.at(c.cx + ox[i], c.cz + oz[i]).h;
            flat += vs[i]->shade * 0.25;
        }
        // only the part of the cell that stands above the water is mirrored: each of the cell's two triangles (the
        // split drawTerrainLOD draws) clipped against the water level, then folded under it. The old quad put the wet
        // corners at the level itself, a slab under the whole cell: from a low eye (swimming) the slabs of the shore cells
        // passed under the camera and, clipped at the near plane, stood in the water as tall dark bars
        static const int tris[2][3] = {{0, 1, 3}, {1, 2, 3}};
        // the cell's dry side first (the shore cut the ground is drawn with), then the part of it above the level
        const TerrainVertex* tvc[4] = {&cache.at(c.cx, c.cz), &cache.at(c.cx + 1, c.cz), &cache.at(c.cx + 1, c.cz + 1), &cache.at(c.cx, c.cz + 1)};
        double wMean = 0; int wN = 0;
        for (int i = 0; i < 4; i++) if (tvc[i]->water > -1e8f) { wMean += site.waterOf(*tvc[i]); wN++; }
        if (wN) wMean /= wN;
        for (int k = 0; k < 2; k++) {
            double px[4], pz[4], ph[4]; int n = 0;
            if (wN) {
                double xs3[3], zs3[3], hs3[3], wl3[3], sh3[3];
                bool anyWet = false, anyDry = false;
                for (int e = 0; e < 3; e++) { int a = tris[k][e]; xs3[e] = xs[a]; zs3[e] = zs[a]; hs3[e] = hs[a]; wl3[e] = tvc[a]->water > -1e8f ? site.waterOf(*tvc[a]) : wMean; sh3[e] = vertexShore(cache, c.cx + ox[a], c.cz + oz[a]); if (sh3[e] < 0) anyWet = true; else anyDry = true; }
                if (anyWet && anyDry) {
                    ShoreSplit sp; shoreSplitTriangle(xs3, zs3, hs3, wl3, sh3, sp);
                    for (int i = 0; i < sp.nDry; i++) { px[n] = sp.dry[i].x; pz[n] = sp.dry[i].z; ph[n] = sp.dry[i].h; n++; }
                } else if (anyWet) continue;   // wholly under its water: nothing to mirror
            }
            if (n == 0) for (int e = 0; e < 3; e++) { int a = tris[k][e]; px[n] = xs[a]; pz[n] = zs[a]; ph[n] = hs[a]; n++; }
            RVert q[6]; int m = 0;
            for (int e = 0; e < n; e++) {
                int a = e, b = (e + 1) % n;
                bool ain = ph[a] >= c.level, bin = ph[b] >= c.level;
                auto emit = [&](double x, double h, double z) {
                    Vec3 v = toView(x, 2 * c.level - h, z);   // the curvature comes with toView
                    q[m].x = v.x; q[m].y = v.y; q[m].z = v.z; q[m].shade = flat * 0.85; m++;
                };
                if (ain) emit(px[a], ph[a], pz[a]);
                if (ain != bin) { double t = (c.level - ph[a]) / (ph[b] - ph[a]); emit(px[a] + (px[b] - px[a]) * t, c.level, pz[a] + (pz[b] - pz[a]) * t); }
            }
            if (m < 3) continue;
            ReflPoly rpo; rpo.first = (int)reflVerts.size(); rpo.n = m;
            Vec3 pv[6]; for (int e = 0; e < m; e++) pv[e] = Vec3(q[e].x, q[e].y, q[e].z);
            bool every = false;
            const bool fills = polygonRows(proj, pv, m, rpo.y0, rpo.y1, every);
            if (every) { rpo.y0 = -(1 << 30); rpo.y1 = 1 << 30; }
            else if (!fills) continue;   // it fills no row of the frame (the rasteriser rejects it)
            reflVerts.insert(reflVerts.end(), q, q + m);
            reflPolys.push_back(rpo);
        }
    }
}

// F-05: the gathered reflections that reach this band's rows, in order, clipped to it (the caller's raster band)
void SurfaceView::drawReflectionBand(Framebuffer& fb, int bandY0, int bandY1) const {
    RasterParams rp;
    rp.blend = BLEND_REFLECT; rp.maskBank = 2; rp.reflectMix = 0.45; rp.ztest = false; rp.zwrite = false;
    for (const ReflPoly& p : reflPolys) {
        if (p.y1 < bandY0 || p.y0 >= bandY1) continue;
        rasterPolygon(fb, &reflVerts[p.first], p.n, rp, proj);
    }
}

void SurfaceView::materialLook(const TerrainVertex& v, uint8_t& bank, double& albedo) const {
    albedo = v.albedo;
    bank = 0;
    // N1-03: one bank per material family, the same families as the globe (P-00: a kind's accents in the grass and forest banks)
    if (tintBankSprings >= 0 && v.material == MAT_MATS) { bank = (uint8_t)tintBankSprings; return; }   // the hot springs' mats in their own bank
    if (tintBankStrata >= 0 && v.gtint > 127) { bank = (uint8_t)tintBankStrata; return; }             // a rainbow stratum's accent layer
    switch (matFamilyOn(site.gen.type, v.material)) {
        case FAM_WATER: bank = 2; break;
        case FAM_FOREST: bank = 3; if (v.material == MAT_FOREST) albedo = 0.28 + 0.1 * v.veg; break;
        case FAM_GRASS: bank = 10; if (v.material == MAT_GRASS) albedo = 0.36 + 0.2 * v.veg; break;
        case FAM_SNOW: bank = 8; if (v.material == MAT_SNOW) albedo = KINDS[site.gen.type].snowAlbedo; break;
        case FAM_SAND: bank = 9; break;
        default: bank = v.material == MAT_LAVA ? 2 : 0; break;
    }
}

const RVert& SurfaceView::vertexOf(TerrainCache& cache, std::vector<VtxCache>& vcache, int cx, int cz, uint8_t& bank) {
    int n = 1 << cache.logN;
    int idx = ((cz & (n - 1)) << cache.logN) | (cx & (n - 1));
    VtxCache& vc = vcache[idx];
    if (vc.stamp == frameStamp && vc.v.u == (double)(cx * cache.cellSize) && vc.v.v == (double)(cz * cache.cellSize)) {
        bank = vc.bank;
        return vc.v;
    }
    const TerrainVertex& tv = cache.at(cx, cz);   // F-04: after the hit test (a current vertex's cell was sampled when it was made)
    double cs = cache.cellSize;
    double x = cx * cs, z = cz * cs;
    double h = tv.h;
    if (tv.material == MAT_WATER && tv.water > -1e8f) h = std::min(h, (double)tv.water - 0.5);
    // geomorphing: within MORPH_CELLS of the ring's edge the vertex slides onto the coarser ring's surface (the
    // height of its drawn triangle at this spot, and its Gouraud shade when the coarser vertices were computed this
    // frame), by the vertex's continuous distance from the camera, so nothing pops as the edge sweeps the ground
    double morph = 0, hC = 0, shC = 0; bool shOk = false;
    double wy = site.waterOf(tv);   // the water level as drawn (Q-02: the tide's)
    double shore = vertexShore(cache, cx, cz);   // the side of the water's edge, morphed with the ground below
    if (morphCoarse && morphBand > 0) {
        double ddx = x - camPos.x, ddz = z - camPos.z;
        double dc = std::sqrt(ddx * ddx + ddz * ddz) / cs;
        morph = smoothstep(curRadius - morphBand, curRadius - 0.5, dc);
        if (morph > 0) {
            double csC = morphCoarse->cellSize;
            CoarseHit ch = coarseAt(*morphCoarse, x, z);
            int X = ch.X, Z = ch.Z; double fx = ch.fx, fz = ch.fz; bool lower = ch.lower;
            hC = ch.h;
            h += (hC - h) * morph;
            if (wy > -1e8f) {   // the water slides with the ground
                double wc = coarseWaterAt(*morphCoarse, x, z);
                if (wc > -1e8) wy += (wc - wy) * morph;
                else {   // the coarser ring has no water here (a stream too small for its scale): the vertex dries out toward the edge, its plane sinking under the coarse ground, so the clamp below cannot hold it under a plane the coarser ring never drew (a 27 m seam)
                    wy += (hC - 1.0 - wy) * morph;
                    if (shore < 1e8) shore += (std::max(1.0, std::fabs(shore)) - shore) * morph;
                }
            }
            if (shore < 1e8) { double sc = coarseShoreAt(*morphCoarse, x, z); if (sc < 1e8) shore += (sc - shore) * morph; }   // and so does the edge
            int lc = morphCoarse->logN, mC = (1 << lc) - 1;
            const VtxCache& c00 = (*morphVc)[((Z & mC) << lc) | (X & mC)];
            const VtxCache& c10 = (*morphVc)[((Z & mC) << lc) | ((X + 1) & mC)];
            const VtxCache& c01 = (*morphVc)[(((Z + 1) & mC) << lc) | (X & mC)];
            const VtxCache& c11 = (*morphVc)[(((Z + 1) & mC) << lc) | ((X + 1) & mC)];
            auto cur = [&](const VtxCache& vcc, int cxx, int czz) { return vcc.stamp == frameStamp && vcc.v.u == (double)(cxx * csC) && vcc.v.v == (double)(czz * csC); };
            if (cur(c00, X, Z) && cur(c10, X + 1, Z) && cur(c01, X, Z + 1) && cur(c11, X + 1, Z + 1)) {
                shC = lower ? c00.v.shade + fx * (c10.v.shade - c00.v.shade) + fz * (c01.v.shade - c00.v.shade)
                            : c11.v.shade + (1 - fx) * (c01.v.shade - c11.v.shade) + (1 - fz) * (c10.v.shade - c11.v.shade);
                shOk = true;
            }
        }
    }
    // normal from neighbours (the curvature of the far ring is applied by toView, O1)
    double hl = cache.at(cx - 1, cz).h, hr = cache.at(cx + 1, cz).h, hd = cache.at(cx, cz - 1).h, hu = cache.at(cx, cz + 1).h;
    Vec3 nrm = normalize(Vec3(hl - hr, 2 * cs, hd - hu));
    const Vec3& sd = env.sun.dirLocal;
    double lambert = std::max(0.0, dot(nrm, sd));
    double sunUp = smoothstep(-0.03, 0.06, sd.y);
    double lf = env.sun.lightFactor;
    double cloud = site.atmosphere ? env.cloudCover : 0.0;
    // M10-07: hemisphere ambient (the sky lights faces that look up), stronger and flatter when overcast
    double ambient = site.atmosphere ? (0.08 + 0.2 * env.skyBrightness) : 0.11;
    ambient *= (0.55 + 0.45 * clampd(nrm.y, 0, 1)) * (1 + 0.6 * cloud);
    double sunTerm = lambert * sunUp * lf * (1 - 0.6 * cloud);
    // M10-06: cast shadows of the terrain itself (lod0 and lod1 only)
    if (sunTerm > 0.01 && cache.cellSize <= 64 && !hasOpaqueDeck(site.gen.type)) sunTerm *= castShadowLit(cache, cx, cz, h, sd);
    // M10-14: cloud shadows on the ground (the sky's cloud layer, offset along the sun ray)
    if (cloudShadows && sunTerm > 0.01 && sd.y > 0.05) {
        double up = (2600.0 - h) / sd.y;
        double px = x + sd.x * up + cloudWx, pz = z + sd.z * up + cloudWz;
        double n = 0.5 + 0.5 * fbm2(px / 1800.0, pz / 1800.0, site.gen.seed + 77, 3, 2.2, 0.55);
        double thr = cloudThr;
        if (localFront.on) thr = 1.0 - std::max(env.cloudPattern, localFront.cloud(x + sd.x * up, z + sd.z * up)) * 0.75;   // the front's deck shadows the ground under it
        sunTerm *= 1 - 0.55 * smoothstep(thr, thr + 0.18, n);
    }
    double light = ambient + (1 - ambient) * sunTerm;
    // M10-09 secondary lights: planetshine at night, the capsule beacon pool, lava glow
    if (env.moonLight > 0.005) light += (1 - ambient) * env.moonLight * 0.6 * std::max(0.0, dot(nrm, env.moonDir)) * (1 - sunUp);
    if (nLamps) light += (1 - ambient) * lampAt(Vec3(x, h, z), &nrm);   // the lamp, the floodlights
    if (buggy.deployed && buggy.lights) {
        // headlights: a 25 m cone ahead of the buggy
        double hx = std::sin(buggy.heading), hz = std::cos(buggy.heading);
        double rx = x - buggy.x, rz = z - buggy.z;
        double f = rx * hx + rz * hz, l = std::fabs(-rx * hz + rz * hx);
        if (f > 0 && f < 25 && l < 0.35 * f + 1.5) light += 0.55 * (1 - f / 25) * (1 - sunUp);
    }
    if (drone.deployed && drone.lights && drone.altAboveGround < 30) {   // the drone's bar lights a pool under and ahead of it when it flies low
        double hx = std::sin(drone.heading), hz = std::cos(drone.heading);
        double rx = x - drone.x, rz = z - drone.z;
        double f = rx * hx + rz * hz, l = std::fabs(-rx * hz + rz * hx), reach = 25 + drone.altAboveGround;
        if (f > -4 && f < reach && l < 0.35 * (f + 4) + 1.5) light += 0.5 * (1 - (f + 4) / (reach + 4)) * (1 - drone.altAboveGround / 30) * (1 - sunUp);
    }
    if (sunUp < 0.9) {
        double bdx = x - capsuleX, bdz = z - capsuleZ;
        double bd2 = bdx * bdx + bdz * bdz;
        if (bd2 < 14.0 * 14.0) { double f = 1 - std::sqrt(bd2) / 14.0; light += 0.35 * f * f * (0.7 + 0.3 * std::sin(lastT * 3.0)) * (1 - sunUp); }
    }
    double albedo; uint8_t bk;
    materialLook(tv, bk, albedo);
    double fog0 = 0;   // the tone tile fades with distance (the haze flattens far ground)
    { double ddx = x - camPos.x, ddz = z - camPos.z; fog0 = clampd(std::sqrt(ddx * ddx + ddz * ddz) / (site.atmosphere ? env.fogDistance : 20000.0), 0, 1); }
    if (tv.water > -1e8f && tv.material != MAT_WATER && h < tv.water + (tv.material == MAT_ICE ? 0.25 : 1.5)) albedo *= 0.8;   // wet shore (B-322: a hand's breadth on ice, or every floe was dark)
    double emissive = 0;
    if (lavaGlow && tv.material != MAT_LAVA && cache.cellSize <= 16) {
        // lava glow: the brightest lava cell within two cells lights the rock around it
        for (int oz = -2; oz <= 2; oz++)
            for (int ox = -2; ox <= 2; ox++) {
                if (!ox && !oz) continue;
                const TerrainVertex& nv = cache.at(cx + ox, cz + oz);
                if (nv.material != MAT_LAVA) continue;
                double d = std::sqrt((double)(ox * ox + oz * oz)) * cs;
                emissive = std::max(emissive, (nv.glow / 255.0) * (1 - d / 40.0));
            }
    }
    // M10-10 quantised speculars on ice, quartz and snow
    double spec = 0;
    if ((tv.material == MAT_ICE || tv.material == MAT_QUARTZ || tv.material == MAT_SNOW || tv.material == MAT_METAL || tv.material == MAT_GLASS) && sunUp > 0.05) {
        Vec3 toCam = normalize(Vec3(camPos.x - x, camPos.y - h, camPos.z - z));
        Vec3 half = normalize(toCam + sd);
        double sp = std::pow(std::max(0.0, dot(nrm, half)), 48.0) * sunUp * lf;
        spec = sp > 0.55 ? (tv.material == MAT_GLASS ? 18 : 12) : (sp > 0.25 ? (tv.material == MAT_GLASS ? 9 : 6) : 0);   // the glass's glint is brighter
    }
    double shade;
    if (tv.material == MAT_LAVA) {
        // dark crust plates with bright veins between them
        Worley3 w = worley3(Vec3(x / 11.0, z / 11.0, env.sun.dayFraction * 3.0 + lastT * 0.03), site.gen.seed);   // the crust drifts
        double vein = 1 - smoothstep(0.03, 0.14, w.f2 - w.f1);
        double n = 0.5 + 0.5 * gnoise2(x / 5.0, z / 5.0, site.gen.seed + 3);
        double glow = tv.glow / 255.0;
        shade = 8 + 12 * n + 36 * vein * glow + 6 * glow;
    } else {
        shade = exposureStop(albedo, light, site.atmosphere);   // the one exposure rule
        if (tv.material == MAT_FOREST) shade *= 1 - 0.38 * (cache.cellSize <= 64 ? canopyDensityAt(x, z, tv.veg) : tv.veg * 0.55);   // the floor is darker under the canopy (far ring: the average)
        shade += 16 * emissive + spec;
        // the broad tone tile (patches of 23-85 m, +-4 shades; a texel is two metres, or a cell for the far rings),
        // sampled per vertex: the cells are smaller than its patches, so the gradient carries it across them
        if (tv.material != MAT_WATER) {
            double g3s = cs <= 64 ? 0.5 : 32.0 / cs;
            shade += (cs <= 64 ? 0.4 : 0.25) * grainMacro.atLerp(x * g3s, z * g3s) * (1 - 0.7 * fog0);
        }
        if (shade > 62) shade = 62;
    }
    // the drawn height keeps to the drawn side of the edge: the morph blends heights and shore distances by the same
    // factor, and the two cross zero at different places, so a corner could turn dry by its edge yet sink under the plane
    if (wy > -1e8 && shore < 1e8) {   // the margins close to the cut's own lip (0.03 under the level) at the ring's edge, where the coarse cut surface is exact
        double wetLip = 0.3 * (1 - morph), dryLip = 0.02 * (1 - morph);   // no lip at the edge: the coarse cut surface respects its water already, and a lip there opened the seam by 3 cm on a lake bed a millimetre under its level
        if (shore < 0) h = std::min(h, wy - wetLip); else h = std::max(h, wy + dryLip);
    }
    Vec3 vv = toView(x, h, z);
    double dist = length(vv);
    // M10-13 scattering lite: haze thicker toward the sun, thinner away from it, more when humid
    double fogD = env.fogDistance;
    if (site.atmosphere && dist > 1) {
        double toSun = dot(Vec3(x - camPos.x, h - camPos.y, z - camPos.z) / dist, sd);
        fogD *= (1 - 0.35 * std::max(0.0, toSun) * sunUp) * (1 + 0.25 * std::max(0.0, -toSun)) * (1 - 0.25 * cloud);
    }
    double fog = 1 - std::exp(-(dist / fogD + rainWall(x, z, dist) + fogSeaWall(x, h, z, dist)));   // the wall of rain between the camera and this ground; Q-02: a fog sea over it
    shade = shade + (63 - shade) * fog;
    if (shOk) shade += (shC - shade) * morph;   // onto the coarser ring's shade at the edge
    vc.v.x = vv.x; vc.v.y = vv.y; vc.v.z = vv.z;
    vc.v.shade = shade;
    vc.v.u = x; vc.v.v = z;
    vc.bank = bk;
    vc.wy = (float)wy;
    vc.hy = (float)h;
    vc.sh = (float)shore;
    vc.front = vv.z >= NEAR_Z;
    vc.sy = vc.front ? (float)(proj.cy - proj.f * vv.y / vv.z) : 0.f;
    if (site.hasWater && (double)vc.wy > -1e8) vc.wv = waterCorner(x, z, (double)vc.wy, (double)vc.hy);   // the stored floats, as the cells read them
    vc.stamp = frameStamp;
    bank = bk;
    return vc.v;
}

RVert SurfaceView::waterCorner(double x, double z, double wl, double hy) const {
    double ddx = x - camPos.x, ddz = z - camPos.z;
    Vec3 vv = toView(x, wl, z);
    double depth = clampd(wl - hy, 0, 8);   // B-322: the depth under the drawn ground (morphed at the ring edges), or the shade jumped at every seam
    Vec3 toV = Vec3(ddx, wl - camPos.y, ddz);
    double dist = length(toV);
    Vec3 rd = normalize(Vec3(toV.x, -toV.y, toV.z));
    double glint = std::pow(std::max(0.0, dot(rd, env.sun.dirLocal)), 28.0) * 24 * waterSunUp * env.sun.lightFactor;
    // wave rings coming in with the wind
    double wave = 2.5 * std::sin((x * waterWdx + z * waterWdz) / 2.6 - lastT * 1.4 + 0.8 * std::sin((x * waterWdz - z * waterWdx) / 7.0));
    // the water's own colour seen from above (deep dark, shallow lighter), the sky's colour at grazing
    // angles (a Fresnel term: nearly all of the far water mirrors the sky), the ramp darkened by the night
    double cosT = clampd(std::fabs(toV.y) / std::max(dist, 0.1), 0.04, 1);   // the waves' slopes keep the mirror short of grazing: from an eye at the surface the water stays water, not a sheet of sky
    double fres = 0.04 + 0.96 * std::pow(1 - cosT, 4.0);
    double body = 24 + (1 - depth / 8) * 8, skyR = 55;
    double base = body + (skyR - body) * fres + glint + wave * (0.5 + 0.5 * clampd(env.windKnots / 15.0, 0, 1));
    double fog = 1 - std::exp(-(dist / env.fogDistance + rainWall(x, z, dist) + fogSeaWall(x, wl, z, dist)));   // W-03; Q-02
    RVert w;
    w.x = vv.x; w.y = vv.y; w.z = vv.z;
    w.shade = base + (63 - base) * fog;
    w.u = x + lastT * 0.7; w.v = z + lastT * 0.4;
    return w;
}

// B-313: the coarser ring's drawn surface at (x, z): the cell's two triangles are split along the (cx + 1, cz) - (cx, cz + 1)
// diagonal (drawTerrainLOD), and a water vertex sits half a metre under its water, as vertexOf draws it
// the coarser ring's water level and shore value at a point, interpolated over the triangle of the coarse cell that
// holds it (the split drawTerrainLOD draws, a corner without water taking the mean of the others), exactly as the cut of that
// cell reads them: so a finer vertex morphed onto the coarse cut surface lands on the same side of the same edge
static void coarseTri(TerrainCache& c, double x, double z, int& X, int& Z, int ox[3], int oz[3], double& u, double& v) {
    double csC = c.cellSize;
    double fx = x / csC, fz = z / csC;
    X = (int)std::floor(fx); Z = (int)std::floor(fz);
    fx -= X; fz -= Z;
    if (fx + fz <= 1.0) { ox[0] = 0; oz[0] = 0; ox[1] = 1; oz[1] = 0; ox[2] = 0; oz[2] = 1; u = fx; v = fz; }   // (00, 10, 01): h = h0 + u (h1 - h0) + v (h2 - h0)
    else { ox[0] = 1; oz[0] = 1; ox[1] = 0; oz[1] = 1; ox[2] = 1; oz[2] = 0; u = 1 - fx; v = 1 - fz; }         // (11, 01, 10)
}
double SurfaceView::coarseWaterAt(TerrainCache& c, double x, double z) {
    int X, Z, ox[3], oz[3]; double u, v;
    coarseTri(c, x, z, X, Z, ox, oz, u, v);
    double wMean = 0; int wN = 0;
    for (int k = 0; k < 4; k++) { const TerrainVertex& t = c.at(X + (k & 1), Z + (k >> 1)); if (t.water > -1e8f) { wMean += site.waterOf(t); wN++; } }
    if (!wN) return -1e9;
    wMean /= wN;
    double w[3];
    for (int k = 0; k < 3; k++) { const TerrainVertex& t = c.at(X + ox[k], Z + oz[k]); w[k] = t.water > -1e8f ? site.waterOf(t) : wMean; }
    return w[0] + u * (w[1] - w[0]) + v * (w[2] - w[0]);
}

SurfaceView::CoarseHit SurfaceView::coarseAt(TerrainCache& c, double x, double z) {
    CoarseHit r;
    double csC = c.cellSize;
    double fx = x / csC, fz = z / csC;
    r.X = (int)std::floor(fx); r.Z = (int)std::floor(fz);
    r.fx = fx - r.X; r.fz = fz - r.Z;
    r.lower = r.fx + r.fz <= 1.0;
    r.h = cacheHeightAt(c, x, z);   // B-322: the coarser ring's surface as drawn: its two triangles cut at the water's edge
    return r;
}

double SurfaceView::coarseShoreAt(TerrainCache& c, double x, double z) {
    int X, Z, ox[3], oz[3]; double u, v;
    coarseTri(c, x, z, X, Z, ox, oz, u, v);
    double s[3];
    for (int k = 0; k < 3; k++) { s[k] = vertexShore(c, X + ox[k], Z + oz[k]); if (s[k] > 1e8) return 1e9; }
    return s[0] + u * (s[1] - s[0]) + v * (s[2] - s[0]);
}

void SurfaceView::testRingSeams(double seam[4], double raw[4]) {
    struct R { TerrainCache* c; std::vector<VtxCache>* vc; int r; };
    R rings[5] = {{&site.lodN, &vcN, nearBlend > 0 ? ringN : 0}, {&site.lod0, &vc0, ringR[0]}, {&site.lod1, &vc1, ringR[1]}, {&site.lod2, &vc2, ringR[2]}, {&site.lod3, &vc3, ringR[3]}};
    Mat3 camT = camLocal.transposed();
    for (int k = 0; k < 4; k++) {
        seam[k] = raw[k] = -1;
        if (rings[k].r <= 0) continue;
        R* coarse = nullptr;
        for (int j = k + 1; j < 5 && !coarse; j++) if (rings[j].r > 0) coarse = &rings[j];
        if (!coarse) continue;
        TerrainCache& c = *rings[k].c;
        double cs = c.cellSize; int r = rings[k].r;
        int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
        int n = 1 << c.logN, mk = n - 1;
        double worst = 0, worstRaw = 0; int cnt = 0;
        for (int cz = pcz - r - 1; cz <= pcz + r + 1; cz++)
            for (int cx = pcx - r - 1; cx <= pcx + r + 1; cx++) {
                double x = cx * cs, z = cz * cs;
                double dc = std::sqrt((x - camPos.x) * (x - camPos.x) + (z - camPos.z) * (z - camPos.z)) / cs;
                if (dc < r - 0.5 || dc > r + 0.75) continue;   // the edge vertices: morphed wholly onto the coarser surface
                const VtxCache& vc = (*rings[k].vc)[((cz & mk) << c.logN) | (cx & mk)];
                if (vc.stamp != frameStamp || vc.v.u != x || vc.v.v != z) continue;
                Vec3 local = camT * Vec3(vc.v.x, vc.v.y, vc.v.z);
                double d2 = local.x * local.x + local.z * local.z;
                double h = local.y + camPos.y + (site.smallBody ? 0.0 : d2 * invTwoR);   // toView inverted (planets: the paraboloid drop)
                double hC = coarseAt(*coarse->c, x, z).h;
                const TerrainVertex& tv = c.at(cx, cz);
                double own = tv.h; if (tv.material == MAT_WATER && tv.water > -1e8f) own = std::min(own, (double)tv.water - 0.5);
                if (std::fabs(h - hC) > worst && std::getenv("VESPERIS_TRACE")) fprintf(stderr, "    seam %s: cell %d %d h %.3f coarse %.3f own %.3f water %.2f shore %.2f sh(drawn) %.2f wy %.2f\n", c.cellSize == 512 ? "lod2" : "ring", cx, cz, h, hC, own, tv.water > -1e8f ? tv.water : -999.0, tv.shore < 1e8f ? tv.shore : 9999.0, vc.sh, vc.wy);
                worst = std::max(worst, std::fabs(h - hC)); worstRaw = std::max(worstRaw, std::fabs(own - hC)); cnt++;
            }
        if (cnt) { seam[k] = worst; raw[k] = worstRaw; }
    }
}

// The ground beyond the last terrain ring, out to the horizon: the sea, or a plain 300 m under the lowest far cell. O1: a
// fan of rings that follows the curvature (toView), from just inside the far ring's edge to ten times its radius, so
// from a mountain the sea or the plain really dips under the horizon instead of a flat plate hanging at eye level.
double SurfaceView::floorHeight() {
    if (site.smallBody) return 0;
    TerrainCache& far = ringR[3] ? site.lod3 : site.lod2;
    int farR = ringR[3] ? ringR[3] : ringR[2];
    double cs = far.cellSize;
    int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
    double hmin = 1e9;
    for (int cz = pcz - farR; cz <= pcz + farR; cz += 2)
        for (int cx = pcx - farR; cx <= pcx + farR; cx += 2) {
            double ddx = cx + 0.5 - camPos.x / far.cellSize, ddz = cz + 0.5 - camPos.z / far.cellSize;
            if (ddx * ddx + ddz * ddz > (double)farR * farR) continue;   // the disc the ring draws (the corners lie beyond the prefetched tiles)
            hmin = std::min(hmin, (double)far.at(cx, cz).h);
        }
    double h = hmin - 300.0;
    if (site.hasWater) h = site.seaLevel + site.tideM;
    return h;
}

void SurfaceView::drawFloor(Framebuffer& fb) {
    if (site.smallBody) return;   // O4: a small body's terrain disc reaches past its own horizon
    TerrainCache& far = ringR[3] ? site.lod3 : site.lod2;
    int farR = ringR[3] ? ringR[3] : ringR[2];
    double cs = far.cellSize;
    double h = floorH;
    const int floorFam = KINDS[site.gen.type].floorFamily;   // the dominant family (P-00: the kind's row)
    uint8_t bank = site.hasWater ? 2 : (floorFam == FAM_SNOW ? 8 : (floorFam == FAM_SAND ? 9 : 0));
    double shade = site.atmosphere ? 63 : 8;
    RasterParams rp; rp.bank = bank; rp.ztest = true; rp.zwrite = true;
    double edge = farR * cs * 0.92;
    const double mul[7] = {1.0, 1.45, 2.1, 3.1, 4.6, 7.0, 10.5};
    const int NA = 24;
    for (int i = 0; i + 1 < 7; i++)
        for (int a = 0; a < NA; a++) {
            double a0 = a * TAU / NA, a1 = (a + 1) * TAU / NA, r0 = edge * mul[i], r1 = edge * mul[i + 1];
            double xs[4] = {std::cos(a0) * r0, std::cos(a1) * r0, std::cos(a1) * r1, std::cos(a0) * r1};
            double zs[4] = {std::sin(a0) * r0, std::sin(a1) * r0, std::sin(a1) * r1, std::sin(a0) * r1};
            RVert q[4];
            for (int k = 0; k < 4; k++) { Vec3 v = toView(camPos.x + xs[k], h - 0.3, camPos.z + zs[k]); q[k].x = v.x; q[k].y = v.y; q[k].z = v.z; q[k].shade = shade; }
            rasterPolygon(fb, q, 4, rp, proj);
        }
}

// B-310: fills the vertex cache (heights, shades, projections) and the material tiles of a ring, with the same cell walk
// as drawTerrainLOD, so that the band threads only read
// a coarser ring skips a cell only when the whole cell lies under the finer ring: its farthest corner within
// `coverM` metres of the camera, the finer ring's guaranteed cover (`coverOf` in render). A scalar skip radius on the
// cell centres left the outer corners of the skipped cells bare: sawtooth holes at every ring edge, plain from the
// capsule on the way down (`vesperis_test capsule`)
bool SurfaceView::cellCovered(double cs, int cx, int cz, double coverM) const {
    if (coverM <= 0) return false;
    double px = camPos.x / cs, pz = camPos.z / cs;
    double dxm = std::max(std::fabs(cx - px), std::fabs(cx + 1 - px)), dzm = std::max(std::fabs(cz - pz), std::fabs(cz + 1 - pz));
    return (dxm * dxm + dzm * dzm) * cs * cs < coverM * coverM;
}

void SurfaceView::warmTerrainLOD(TerrainCache& cache, std::vector<VtxCache>& vcache, int radius, double coverM, TerrainCache* coarse, std::vector<VtxCache>* coarseVc, int band) {
    double cs = cache.cellSize;
    int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
    curRadius = radius;
    morphCoarse = coarse; morphVc = coarseVc; morphBand = coarse ? band : 0;   // B-313: the coarser ring is warmed already (render warms coarse to fine)
    // the vertices are warmed three cells into the skip disc too: the finer ring's morph band reads this ring's
    // vertex shades there (its edge lies inside this ring's skipped cells), and a stale one would leave the vertex on
    // its own shade, a tooth on the skyline
    double r2 = (double)radius * radius, warmCover = std::max(0.0, coverM - 3 * cs);
    static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr;
    auto t0 = std::chrono::steady_clock::now();
    // the terrain cells first (a miss samples the planet function and writes the cache; the shadow march of vertexOf reads cells
    // anywhere within the radius), then the tiles of their materials
    // within the disc (plus two cells for the normals): the square's corners lay 75 km out on the far ring, beyond
    // the drainage tiles a landing prefetches, and the first frame computed those tiles (270 ms)
    // the disc's misses listed first; over FILL_SERIAL_MAX of them they are sampled on the pool (a landing's first frame:
    // 12,000 cells, 14 ms here before), each listed cell by one worker into its own slot: the disc spans 2r + 3 cells a side, under
    // the torus's 1 << logN for every ring (the unit checks it), so distinct cells have distinct slots, and a sample is a pure
    // function of the cell and the site, whatever its thread asked before (the drainage's tiles and neighbourhood read through each
    // thread's own caches, the neighbourhood a function of its lattice cell: `buildNeigh`). The pool's join is the barrier.
    // A few misses (a walk's edge) are sampled by the walk below as before. The material tiles the disc's cells want are made here,
    // after, as before (a band thread reads a tile, it never makes one), the ones not made yet together (`buildMesoTiles`)
    // the disc reaches every cell the vertex pass reads (the pool's threads share its slots, so a cell it missed
    // would be sampled by two threads into one slot): a drawn cell's centre lies within the radius, its corners read their neighbours
    // for the normals and the shore's side (`vertexShore`: up to two cells on from the cell, 2.83 from its centre), on a lava world the
    // near rings' glow two cells round a corner (4.24); the shadow's march stays within the radius. Distinct slots still: 2r + 5 cells
    const bool glowReach = lavaGlow && cache.cellSize <= 16;
    const int edge = radius + (glowReach ? 2 : 1);
    const double rFill = radius + (glowReach ? 4.5 : 3.0), rFill2 = rFill * rFill;
    auto forDisc = [&](auto&& fn) {
        for (int cz = pcz - edge; cz <= pcz + edge; cz++)
            for (int cx = pcx - edge; cx <= pcx + edge; cx++) {
                double ddx = cx + 0.5 - camPos.x / cs, ddz = cz + 0.5 - camPos.z / cs;
                if (ddx * ddx + ddz * ddz > rFill2) continue;
                fn(cx, cz);
            }
    };
    fillMisses.clear();
    forDisc([&](int cx, int cz) { const TerrainVertex& tv = cache.v[cache.index(cx, cz)]; if (tv.cx != cx || tv.cz != cz) fillMisses.push_back({cx, cz}); });
    const int misses = (int)fillMisses.size();
    lastWarmMisses += misses;
    if (misses > FILL_SERIAL_MAX)
        parallelFor(misses, 16, [&](int b, int e) { for (int k = b; k < e; k++) cache.at(fillMisses[k].first, fillMisses[k].second); });
    uint32_t want = 0;
    forDisc([&](int cx, int cz) { const int m = cache.at(cx, cz).material; if (m < MAT_COUNT && !mesoBuilt[m]) want |= 1u << m; });
    if (want) buildMesoTiles(want);
    auto t1 = std::chrono::steady_clock::now();
    // then the vertices, the four corners of every cell the draw can reach, in parallel by rows of vertices (F-04: the thread of
    // vertex row vz warms the corners on it of the cells either side, cell rows vz - 1 and vz, so every vertex is written by one
    // thread; the cells' top corners had a serial walk of their own)
    auto warmed = [&](int cx, int cz) {
        if (cz < pcz - radius || cz >= pcz + radius) return false;
        double dx = cx + 0.5 - (camPos.x / cs), dz = cz + 0.5 - (camPos.z / cs);
        double d2 = dx * dx + dz * dz;
        if (d2 > r2) return false;
        if (cellCovered(cs, cx, cz, warmCover)) return false;
        double wx = (cx + 0.5) * cs - camPos.x, wz = (cz + 0.5) * cs - camPos.z;
        double fwdx = camLocal.m[2][0], fwdz = camLocal.m[2][2];
        double dd = std::sqrt(wx * wx + wz * wz);
        return !(dd > cs * 3 && (wx * fwdx + wz * fwdz) / dd < -0.25);
    };
    parallelFor(2 * radius + 1, 4, [&](int rb, int re) {
        for (int vz = pcz - radius + rb; vz < pcz - radius + re; vz++)
            for (int cx = pcx - radius; cx < pcx + radius; cx++) {
                if (!warmed(cx, vz) && !warmed(cx, vz - 1)) continue;
                uint8_t bk;
                vertexOf(cache, vcache, cx, vz, bk);
                vertexOf(cache, vcache, cx + 1, vz, bk);
            }
    });
    if (trace && (frameStamp <= 3 || frameStamp - anchorFrame <= 2)) { auto t2 = std::chrono::steady_clock::now(); fprintf(stderr, "    warm %4d m: %d misses, fill %.1f ms, vertices %.1f ms\n", (int)cs, misses, std::chrono::duration<double, std::milli>(t1 - t0).count(), std::chrono::duration<double, std::milli>(t2 - t1).count()); }
}

// F-04: a terrain cell's pieces, the same for the bin (the rows they reach) and the draw. B-320: the cell's water surface per corner
// (00, 10, 11, 01; the level as drawn, morphed with the ground); a corner without water takes the mean of the others, so the sheet is
// one continuous plane cut by the ground. Returns how many corners have their own level
static int cellWaterLevels(double w00, double w10, double w11, double w01, double wCorner[4]) {
    wCorner[0] = w00; wCorner[1] = w10; wCorner[2] = w11; wCorner[3] = w01;
    double wMean = 0; int wN = 0;
    for (int i = 0; i < 4; i++) if (wCorner[i] > -1e8) { wMean += wCorner[i]; wN++; }
    if (wN) { wMean /= wN; for (int i = 0; i < 4; i++) if (wCorner[i] < -1e8) wCorner[i] = wMean; }
    return wN;
}
// the corners of a cell's two triangles (0: 00 10 01, 1: 10 11 01) in the order 00, 10, 01, 11
static const int CELL_TRIS[2][3] = {{0, 1, 2}, {1, 3, 2}};
// B-322: triangle k of a cell that straddles its water's edge, split along the edge (`shoreSplitTriangle`); hy, wl and sh per corner
// in the order 00, 10, 01, 11 (the drawn height, the water level, the side of the edge)
static void cellShoreSplit(int k, int cx, int cz, double cs, const double hy[4], const double wl[4], const double sh[4], ShoreSplit& sp) {
    static const int oxv[4] = {0, 1, 0, 1}, ozv[4] = {0, 0, 1, 1};
    double xs[3], zs[3], hs[3], wls[3], shs[3];
    for (int e = 0; e < 3; e++) { int c = CELL_TRIS[k][e]; xs[e] = (cx + oxv[c]) * cs; zs[e] = (cz + ozv[c]) * cs; hs[e] = hy[c]; wls[e] = wl[c]; shs[e] = sh[c]; }
    shoreSplitTriangle(xs, zs, hs, wls, shs, sp);
}

void SurfaceView::binTerrainLOD(TerrainBin& bin, TerrainCache& cache, std::vector<VtxCache>& vcache, int radius, double coverM, bool water,
                                bool skirt, int rowB, int rowE, int& stale, int& allRows) {
    const double cs = cache.cellSize;
    const int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs), mk = (1 << cache.logN) - 1;
    const double r2 = (double)radius * radius;
    auto outside = [&](int ncx, int ncz) { double ex = ncx + 0.5 - (camPos.x / cs), ez = ncz + 0.5 - (camPos.z / cs); return ex * ex + ez * ez > r2; };
    auto current = [&](const VtxCache& vc, int vx, int vz) { return vc.stamp == frameStamp && vc.v.u == (double)(vx * cache.cellSize) && vc.v.v == (double)(vz * cache.cellSize); };
    for (int j = rowB; j < rowE; j++) {
        const int cz = pcz - radius + j;
        TerrainCellRef* out = &bin.cells[(size_t)j * bin.cols];
        int n = 0;
        for (int cx = pcx - radius; cx < pcx + radius; cx++) {
            double dx = cx + 0.5 - (camPos.x / cs), dz = cz + 0.5 - (camPos.z / cs);
            double d2 = dx * dx + dz * dz;
            if (d2 > r2) continue;
            if (cellCovered(cs, cx, cz, coverM)) continue;   // B-318: wholly under the finer ring
            // frustum-ish cull: cell centre direction vs camera forward
            {
                double wx = (cx + 0.5) * cs - camPos.x, wz = (cz + 0.5) * cs - camPos.z;
                double fwdx = camLocal.m[2][0], fwdz = camLocal.m[2][2];
                double dd = std::sqrt(wx * wx + wz * wz);
                if (dd > cs * 3 && (wx * fwdx + wz * fwdz) / dd < -0.25) continue;
            }
            const VtxCache& c00 = vcache[((cz & mk) << cache.logN) | (cx & mk)];
            const VtxCache& c10 = vcache[((cz & mk) << cache.logN) | ((cx + 1) & mk)];
            const VtxCache& c01 = vcache[(((cz + 1) & mk) << cache.logN) | (cx & mk)];
            const VtxCache& c11 = vcache[(((cz + 1) & mk) << cache.logN) | ((cx + 1) & mk)];
            // the warm made every corner of every cell drawn (the same culls, its cover three cells shorter); a corner it missed is made
            // here as the draw made it before (counted: the tests want none)
            stale += !current(c00, cx, cz) + !current(c10, cx + 1, cz) + !current(c01, cx, cz + 1) + !current(c11, cx + 1, cz + 1);
            uint8_t bk;
            const RVert& v00 = vertexOf(cache, vcache, cx, cz, bk);
            const RVert& v10 = vertexOf(cache, vcache, cx + 1, cz, bk);
            const RVert& v01 = vertexOf(cache, vcache, cx, cz + 1, bk);
            const RVert& v11 = vertexOf(cache, vcache, cx + 1, cz + 1, bk);
            if (v00.z < NEAR_Z && v10.z < NEAR_Z && v01.z < NEAR_Z && v11.z < NEAR_Z) continue;
            double wCorner[4];
            const int wN = cellWaterLevels(c00.wy, c10.wy, c11.wy, c01.wy, wCorner);
            bool anyWet = false, anyDry = false;
            const double shc[4] = {c00.sh, c10.sh, c01.sh, c11.sh};
            if (water && wN > 0) for (int k = 0; k < 4; k++) { if (shc[k] < 0) anyWet = true; else anyDry = true; }
            const bool wet = water && wN > 0 && anyWet, cut = wet && anyDry;
            // the rows: each piece (a triangle of the ground or of its shore cut, a wall, the water's sheet) projected as the rasteriser
            // projects it, clipped at the near plane where it clips it; a piece wholly beside, above or below the frame adds nothing (the
            // rasteriser rejects it: a deep sea's bed under its sheet), and the foam's sparkles put the cell in every band
            int y0 = 1 << 30, y1 = -(1 << 30);
            bool every = false;
            auto piece = [&](const Vec3* p, int np) {
                int a, b;
                if (polygonRows(proj, p, np, a, b, every)) { y0 = std::min(y0, a); y1 = std::max(y1, b); }
            };
            auto V = [](const RVert& r) { return Vec3(r.x, r.y, r.z); };
            const RVert* vs[4] = {&v00, &v10, &v01, &v11};
            if (!cut) {
                const Vec3 tA[3] = {V(v00), V(v10), V(v01)}, tB[3] = {V(v10), V(v11), V(v01)};
                piece(tA, 3); piece(tB, 3);
            } else {
                const double hyv[4] = {c00.hy, c10.hy, c01.hy, c11.hy}, wlv[4] = {wCorner[0], wCorner[1], wCorner[3], wCorner[2]};
                for (int k = 0; k < 2; k++) {
                    ShoreSplit sp;
                    cellShoreSplit(k, cx, cz, cs, hyv, wlv, shc, sp);
                    auto cutPiece = [&](const ShorePt* pts, int np, bool dry) {
                        if (np < 3) return;
                        Vec3 q[4];
                        for (int i = 0; i < np; i++) q[i] = pts[i].a == pts[i].b ? V(*vs[CELL_TRIS[k][pts[i].a]]) : shoreCutView(pts[i], dry);
                        piece(q, np);
                    };
                    cutPiece(sp.dry, sp.nDry, true);
                    cutPiece(sp.wet, sp.nWet, false);
                }
            }
            if (skirt) {
                auto wall = [&](const RVert& a, const RVert& b, double ax, double az, double bx, double bz) {
                    const Vec3 q[4] = {V(a), V(b), skirtFoot(cache, bx, bz), skirtFoot(cache, ax, az)};   // the draw's a, b and their feet
                    piece(q, 4);
                };
                if (outside(cx + 1, cz)) wall(v10, v11, (cx + 1) * cs, cz * cs, (cx + 1) * cs, (cz + 1) * cs);
                if (outside(cx - 1, cz)) wall(v00, v01, cx * cs, cz * cs, cx * cs, (cz + 1) * cs);
                if (outside(cx, cz + 1)) wall(v01, v11, cx * cs, (cz + 1) * cs, (cx + 1) * cs, (cz + 1) * cs);
                if (outside(cx, cz - 1)) wall(v00, v10, cx * cs, cz * cs, (cx + 1) * cs, cz * cs);
            }
            if (wet) {
                const VtxCache* vcw[4] = {&c00, &c10, &c11, &c01};
                static const int ox[4] = {0, 1, 1, 0}, oz[4] = {0, 0, 1, 1};
                Vec3 q[4];
                for (int i = 0; i < 4; i++) q[i] = (double)vcw[i]->wy > -1e8 ? V(vcw[i]->wv) : toView((cx + ox[i]) * cs, wCorner[i], (cz + oz[i]) * cs);   // waterCorner's place
                piece(q, 4);
                double cdist = std::sqrt(d2) * cs;
                if (cdist < 90 && cache.cellSize <= 16 && (cs >= 16 || ((cx & 3) == 0 && (cz & 3) == 0)))
                    for (int i = 0; i < 4; i++) if (!(std::fabs(cache.at(cx + ox[i], cz + oz[i]).h - wCorner[i]) > 0.6)) every = true;   // the foam
            }
            if (!every && y0 > y1) continue;   // nothing of it reaches the frame
            TerrainCellRef& r = out[n];
            r.cx = cx; r.cz = cz;
            if (every) { r.y0 = INT16_MIN; r.y1 = INT16_MAX; allRows++; }
            else { r.y0 = (int16_t)y0; r.y1 = (int16_t)y1; }
            n++;
        }
        bin.count[j] = n;
    }
}

void SurfaceView::drawTerrainLOD(Framebuffer& fb, const TerrainBin& bin, TerrainCache& cache, std::vector<VtxCache>& vcache, int radius,
                                 double coverM, bool water, double t, int bandY0, int bandY1, bool skirt, BandTrace* bt) {
    using TraceClock = std::chrono::steady_clock;   // F-01: with `bt` (VESPERIS_TRACE), the band's cells counted and its time split
    auto traceMs = [](TraceClock::time_point a, TraceClock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
    double cs = cache.cellSize;
    RasterParams rp;
    rp.grain = &grain;
    rp.grainScale = (cache.cellSize <= 16 ? 3.0 : 0.25) * FB_SCALE;   // same on-screen grain size at every scale
    rp.edge = &grainEdge;                                              // the boundary wobble
    if (coverM > 0) rp.zbias = 0.999;   // where a coarser ring overlaps the finer one's morphed edge (coincident surfaces) the finer wins
    if (shadingMode == 1) rp.quantize = 63.0 / 12.0;
    RasterParams rpw;
    rpw.grain = &grainFine; rpw.grainScale = 0.5 * FB_SCALE;
    rpw.zbias = rp.zbias;   // the water of a ring is pushed back with its ground, or from a kilometre away the metre-high shore lost to the plane in every wet cell and the coast snapped to the grid
    double r2 = (double)radius * radius;
    const int mk = (1 << cache.logN) - 1;
    for (int j = 0; j < (int)bin.count.size(); j++) {   // the ring's cells as binned, in the walk's order
        const TerrainCellRef* row = &bin.cells[(size_t)j * bin.cols];
        for (int s = 0; s < bin.count[j]; s++) {
            const TerrainCellRef& ref = row[s];
            if (bt) bt->walked++;
            if (ref.y1 < bandY0 || ref.y0 >= bandY1) continue;   // its pieces miss this band's rows
            const int cx = ref.cx, cz = ref.cz;
            double dx = cx + 0.5 - (camPos.x / cs), dz = cz + 0.5 - (camPos.z / cs);
            double d2 = dx * dx + dz * dz;
            // the corners as the warm made them (the bin found each current: vertexOf's hit)
            const VtxCache& c00 = vcache[((cz & mk) << cache.logN) | (cx & mk)];
            const VtxCache& c10 = vcache[((cz & mk) << cache.logN) | ((cx + 1) & mk)];
            const VtxCache& c01 = vcache[(((cz + 1) & mk) << cache.logN) | (cx & mk)];
            const VtxCache& c11 = vcache[(((cz + 1) & mk) << cache.logN) | ((cx + 1) & mk)];
            const RVert& v00 = c00.v; const RVert& v10 = c10.v; const RVert& v01 = c01.v; const RVert& v11 = c11.v;
            const uint8_t b00 = c00.bank, b10 = c10.bank, b01 = c01.bank, b11 = c11.bank;
            auto outside = [&](int ncx, int ncz) { double ex = ncx + 0.5 - (camPos.x / cs), ez = ncz + 0.5 - (camPos.z / cs); return ex * ex + ez * ez > r2; };
            const TerrainVertex& t00 = cache.at(cx, cz);
            const TerrainVertex& t10 = cache.at(cx + 1, cz);
            const TerrainVertex& t01 = cache.at(cx, cz + 1);
            const TerrainVertex& t11 = cache.at(cx + 1, cz + 1);
            double wCorner[4];   // the water surface per corner (00, 10, 11, 01), one continuous plane cut by the ground
            const int wN = cellWaterLevels(c00.wy, c10.wy, c11.wy, c01.wy, wCorner);
            // the sheet is drawn where a corner lies on the water's side of its edge (`shoreValue`), not wherever a corner lies
            // within half a metre of the level, which reaches over the low ground beyond a river's plain as a moat of cells
            bool anyWet = false, anyDry = false;
            double shc[4] = {c00.sh, c10.sh, c01.sh, c11.sh};   // in the vs order: 00, 10, 01, 11 (as drawn: morphed at the ring's edge)
            if (water && wN > 0) for (int k = 0; k < 4; k++) { if (shc[k] < 0) anyWet = true; else anyDry = true; }
            bool wet = water && wN > 0 && anyWet;
            TraceClock::time_point tA;
            if (bt) { bt->drawn++; tA = TraceClock::now(); }
            RVert tri[3];
            // the bank by majority; B-311: a cell whose corners differ in family also gets the second bank and per-corner
            // weights, and the rasteriser draws the boundary as a noisy contour through the cell (at the cell's
            // square edge, sand, rock and grass would meet in staircases of plates)
            const uint8_t bks[4] = {b00, b10, b01, b11};
            const RVert* vs[4] = {&v00, &v10, &v01, &v11};
            int counts[BANKS] = {0};
            for (int k = 0; k < 4; k++) counts[bks[k]]++;
            int bank = b00;
            for (int k = 0; k < BANKS; k++) if (counts[k] > counts[bank]) bank = k;
            int bank2 = -1;
            for (int k = 0; k < BANKS; k++) if (k != bank && counts[k] > 0 && (bank2 < 0 || counts[k] > counts[bank2])) bank2 = k;
            rp.bank = bank; rp.bank2 = bank2; rp.shade2 = 0; rp.grain2b = nullptr;
            // M10-05: the material's meso tile on top of the micro grain, both fading with distance; B-311: the far rings
            // carry the tile too, stretched so that it spans two cells
            {
                double cd = std::sqrt(d2) * cs;
                rp.grainWeight = clampd(1.0 - (cd - 120.0) / 500.0, 0.2, 1.0);
                rp.grain2Scale = cs <= 64 ? 4.0 : 128.0 / (2 * cs);
            }
            // flat shading per quadrant, as the original's polymap did; the grain texture and the
            // mush filter give the surface its grain and soften the steps (M10-12: or a quantised gradient)
            double flat, w[4] = {1, 1, 1, 1};
            if (bank2 < 0) {
                rp.grain2 = mesoFor(cache.at(cx, cz).material);
                flat = 0.25 * (v00.shade + v10.shade + v01.shade + v11.shade);
            } else {
                int matA = -1, matB = -1, nA = 0, nB = 0; double sA = 0, sB = 0;
                for (int k = 0; k < 4; k++) {
                    const TerrainVertex& tk = cache.at(cx + (k & 1), cz + (k >> 1));
                    if (bks[k] == bank) { w[k] = 1; sA += vs[k]->shade; nA++; if (matA < 0) matA = tk.material; }
                    else { w[k] = 0; sB += vs[k]->shade; nB++; if (matB < 0 || bks[k] == bank2) matB = tk.material; }
                }
                flat = sA / nA;
                if (shadingMode == 0) rp.shade2 = sB / nB - flat;   // the gradient modes carry each corner's own shade
                rp.grain2 = mesoFor(matA); rp.grain2b = mesoFor(matB);
                rp.edgeScale = 4.0 / cs;   // four texels of wobble per cell
            }
            // a cell that straddles a water's edge is cut along it (`shoreSplitTriangle`), the cut at the water level:
            // the shoreline follows the shore distance's contour instead of the two triangles' crossing of the plane
            bool cut = wet && anyDry;
            static const char* skipEnv = std::getenv("VESPERIS_SKIP");   // diagnostics (B-322's method): "cut", "wq" (the water quads), "refl", "near" turn one pass off
            if (skipEnv && std::strstr(skipEnv, "cut")) cut = false;
            if (!cut) {
                tri[0] = v00; tri[1] = v10; tri[2] = v01;
                tri[0].w = w[0]; tri[1].w = w[1]; tri[2].w = w[2];
                if (shadingMode == 0) tri[0].shade = tri[1].shade = tri[2].shade = flat;
                rasterTriangle(fb, tri, rp, proj);
                tri[0] = v10; tri[1] = v11; tri[2] = v01;
                tri[0].w = w[1]; tri[1].w = w[3]; tri[2].w = w[2];
                if (shadingMode == 0) tri[0].shade = tri[1].shade = tri[2].shade = flat;
                rasterTriangle(fb, tri, rp, proj);
            } else {
                const double hyv[4] = {c00.hy, c10.hy, c01.hy, c11.hy};
                const double wlv[4] = {wCorner[0], wCorner[1], wCorner[3], wCorner[2]};   // wCorner runs 00, 10, 11, 01
                for (int k = 0; k < 2; k++) {
                    ShoreSplit sp;
                    cellShoreSplit(k, cx, cz, cs, hyv, wlv, shc, sp);
                    auto emitPiece = [&](const ShorePt* pts, int n, bool dry) {
                        if (n < 3) return;
                        RVert q[4];
                        for (int i = 0; i < n; i++) {
                            int a = CELL_TRIS[k][pts[i].a], b = CELL_TRIS[k][pts[i].b];
                            const RVert& va = *vs[a]; const RVert& vb = *vs[b];
                            double t = pts[i].t;
                            if (pts[i].a == pts[i].b) { q[i] = va; q[i].w = w[a]; }
                            else {
                                // the dry piece's edge stands SHORE_LIP over the water, the wet piece's lies 3 cm under it: with one shared
                                // point under the plane, the sheet covered the dry piece out to wherever it rose through the plane, which on a
                                // flat shore was near the next corner, and the water's edge ran along the cells' edges in stairs
                                Vec3 vv = shoreCutView(pts[i], dry);
                                q[i].x = vv.x; q[i].y = vv.y; q[i].z = vv.z;
                                q[i].shade = va.shade + (vb.shade - va.shade) * t;
                                q[i].u = va.u + (vb.u - va.u) * t; q[i].v = va.v + (vb.v - va.v) * t;
                                q[i].w = w[a] + (w[b] - w[a]) * t;
                            }
                            if (shadingMode == 0) q[i].shade = flat;
                        }
                        rasterPolygon(fb, q, n, rp, proj);
                    };
                    emitPiece(sp.dry, sp.nDry, true);
                    emitPiece(sp.wet, sp.nWet, false);
                }
            }
            // skirts (KI-006): cells on the outer edge of the outermost ring hang a wall down over the floor. B-322: the inner
            // rings hang none: their edges lie on the coarser ring's surface, and a wall there would peek through
            // the coincident ground as a thin bright line along the ring's edge (its depth bias differs from the coarser ring's)
            if (skirt) {
                auto wall = [&](const RVert& a, const RVert& b, double ax, double az, double bx, double bz) {
                    Vec3 la = skirtFoot(cache, ax, az), lb = skirtFoot(cache, bx, bz);
                    RVert q[4] = {a, b, b, a};
                    q[2].x = lb.x; q[2].y = lb.y; q[2].z = lb.z; q[3].x = la.x; q[3].y = la.y; q[3].z = la.z;
                    for (int k = 0; k < 4; k++) q[k].shade = flat * 0.8;
                    rasterPolygon(fb, q, 4, rp, proj);
                };
                if (outside(cx + 1, cz)) wall(v10, v11, (cx + 1) * cs, cz * cs, (cx + 1) * cs, (cz + 1) * cs);
                if (outside(cx - 1, cz)) wall(v00, v01, cx * cs, cz * cs, cx * cs, (cz + 1) * cs);
                if (outside(cx, cz + 1)) wall(v01, v11, cx * cs, (cz + 1) * cs, (cx + 1) * cs, (cz + 1) * cs);
                if (outside(cx, cz - 1)) wall(v00, v10, cx * cs, cz * cs, (cx + 1) * cs, cz * cs);
            }
            if (bt) { const TraceClock::time_point tB = TraceClock::now(); bt->fillMs += traceMs(tA, tB); tA = tB; }
            if (water && !(skipEnv && std::strstr(skipEnv, "wq"))) {
                if (wet) {
                    RVert w[4];
                    const TerrainVertex* tv[4] = {&t00, &t10, &t11, &t01};
                    const VtxCache* vcw[4] = {&c00, &c10, &c11, &c01};
                    int ox[4] = {0, 1, 1, 0}, oz[4] = {0, 0, 1, 1};
                    for (int i = 0; i < 4; i++)   // a corner with its own level was made with its vertex; one without takes the cell's mean
                        w[i] = (double)vcw[i]->wy > -1e8 ? vcw[i]->wv : waterCorner((cx + ox[i]) * cs, (cz + oz[i]) * cs, wCorner[i], vcw[i]->hy);
                    if (bt) { const TraceClock::time_point tC = TraceClock::now(); bt->waterMs += traceMs(tA, tC); bt->wet++; tA = tC; }
                    rpw.bank = 2; rpw.bank2 = -1;
                    if (tintBankW >= 0 || tintBankL >= 0) {   // the water's trait colour over the cell's corners (a bloom's or a lagoon's first, else
                        // a pink lake's or the milky glow), its edge wobbled as two materials' are; the glow lit by its share
                        double tw[4] = {0, 0, 0, 0}, mx = 0; int tb = -1;
                        if (tintBankL >= 0) for (int i = 0; i < 4; i++) { tw[i] = tv[i]->ltint / 255.0; mx = std::max(mx, tw[i]); }
                        if (mx > 0.25) tb = tintBankL;
                        else if (tintBankW >= 0) {
                            mx = 0;
                            for (int i = 0; i < 4; i++) {
                                tw[i] = tv[i]->wtint / 255.0;
                                if (tintBankW == 15) tw[i] *= waterLights ? 0.5 + 0.5 * (0.5 + 0.5 * gnoise2((cx + ox[i]) * cs / 260.0 + lastT * 0.02, (cz + oz[i]) * cs / 260.0 - lastT * 0.013, site.gen.seed + 0x6108))   // drifting
                                                                    : 0.7 + 0.3 * (0.5 + 0.5 * gnoise2((cx + ox[i]) * cs / 700.0, (cz + oz[i]) * cs / 700.0, site.gen.seed + 0x6107));
                                mx = std::max(mx, tw[i]);
                            }
                            if (mx > (waterLights ? 0.12 : 0.25)) tb = tintBankW;
                        }
                        if (tb >= 0) {
                            double wmin = 1, wmax = 0;
                            const double lo = waterLights && tb == 15 ? 0.12 : 0.25, hi = waterLights && tb == 15 ? 0.42 : 0.75;   // a dark ocean's lights a fainter field, its patches whole sooner
                            for (int i = 0; i < 4; i++) {
                                w[i].w = 1 - smoothstep(lo, hi, tw[i]);   // a corner well inside is the colour's whatever the wobble (it flipped pixels back to the sea)
                                wmin = std::min(wmin, w[i].w); wmax = std::max(wmax, w[i].w);
                                if (tb == 15) w[i].shade = waterLights ? 15 + 18 * tw[i] : 20 + 19 * tw[i] * tintGlow;
                            }
                            // a cell wholly the colour's (the wobble's 0.35 never lifts a pixel to a half) is drawn in its bank alone, as the split draws
                            // it, without the split's noise a pixel; one wholly the water's stays the water's
                            if (wmax < 0.149) rpw.bank = tb;
                            else if (wmin < 0.851) { rpw.bank2 = tb; rpw.edge = &grainEdge; rpw.edgeScale = 4.0 / cs; rpw.edgeAmp = 0.35; rpw.shade2 = 0; }
                        }
                    }
                    rasterPolygon(fb, w, 4, rpw, proj);
                    // foam sparkle where the shore is shallow and close (Q-02: none where the sea glows, its dark night bank would dot it)
                    double cdist = std::sqrt(d2) * cs;
                    if (cdist < 90 && cache.cellSize <= 16 && (cs >= 16 || ((cx & 3) == 0 && (cz & 3) == 0)) && rpw.bank2 != 15 && rpw.bank != 15) {   // one near cell in sixteen keeps the density
                        for (int i = 0; i < 4; i++) {
                            if (std::fabs(tv[i]->h - wCorner[i]) > 0.6) continue;
                            uint64_t hf = hash2i(cx + ox[i], cz + oz[i], site.gen.seed ^ 0xF0A);
                            for (int k = 0; k < 3; k++) {
                                double fx = (cx + ox[i]) * cs + (hash01(mix64(hf + k)) - 0.5) * cs * 0.9;
                                double fz = (cz + oz[i]) * cs + (hash01(mix64(hf + k + 7)) - 0.5) * cs * 0.9;
                                double ph = std::sin(t * 2.2 + hash01(mix64(hf + k + 13)) * TAU);
                                if (ph < 0.3) continue;
                                Vec3 fv = toView(fx, wCorner[i] + 0.05, fz);
                                if (fv.z < NEAR_Z) continue;
                                RVert p; p.x = fv.x; p.y = fv.y; p.z = fv.z; p.shade = 50 + 12 * ph;
                                rasterPoint3(fb, p, 2, proj, true, 1);
                            }
                        }
                    }
                    if (bt) bt->fillMs += traceMs(tA, TraceClock::now());
                }
            }
        }
    }
}

// the cached height under a point at `dist` from the camera: bilinear lod0 close by, the coarser rings further out
// (so the march never thrashes the fine cache), with the water level of the same cell
double SurfaceView::heightNear(double x, double z, double dist, double& water) {
    if (dist < ringR[0] * 16.0 * 0.9) { water = site.waterNowAt(x, z); return site.groundHeight(x, z); }
    TerrainCache& c = dist < ringR[1] * 64.0 * 0.9 ? site.lod1 : (dist < ringR[2] * 512.0 * 0.9 || !ringR[3] ? site.lod2 : site.lod3);
    const TerrainVertex& tv = c.at((int)std::lround(x / c.cellSize), (int)std::lround(z / c.cellSize));
    water = site.waterOf(tv);
    return tv.h;
}

double SurfaceView::drawnGround(double x, double z, double dist, double& water, int& ring) {
    ring = drawnRing(dist);
    if (ring == 0) { water = site.waterNowAt(x, z); return site.groundHeight(x, z); }
    TerrainCache& c = ring == 1 ? site.lod1 : (ring == 2 ? site.lod2 : site.lod3);
    water = site.waterOf(c.at((int)std::lround(x / c.cellSize), (int)std::lround(z / c.cellSize)));
    return cacheHeightAt(c, x, z);
}

double SurfaceView::rangeToGround(double maxDist, double& hitX, double& hitZ, bool& water) {
    Vec3 f = camLocal.row(2);   // the view direction in local metres
    double fh = std::sqrt(f.x * f.x + f.z * f.z);
    double dirx = fh > 1e-9 ? f.x / fh : 0, dirz = fh > 1e-9 ? f.z / fh : 0;
    // the ground's height relative to the camera at a horizontal advance sH along the sight line (with the curvature)
    auto groundRel = [&](double sH, double& px, double& pz, bool& w) -> double {
        double arc = sH, ang = 0;
        if (site.smallBody) { ang = std::asin(std::min(1.0, sH / site.R)); arc = site.R * ang; }
        px = camPos.x + dirx * arc; pz = camPos.z + dirz * arc;
        double wl; double h = heightNear(px, pz, arc, wl);
        w = wl > -1e8 && wl > h;
        if (w) h = wl;
        if (site.smallBody) return (site.R + h) * std::cos(ang) - (site.R + camPos.y);
        return h - camPos.y - sH * sH * invTwoR;
    };
    double prev = 0, s = 0.6, px, pz; bool w;
    while (s < maxDist) {
        double sH = s * fh;
        if (site.smallBody && sH >= site.R * 0.999) break;
        if (s * f.y <= groundRel(sH, px, pz, w)) {
            double lo = prev, hi = s;
            for (int i = 0; i < 9; i++) { double mid = 0.5 * (lo + hi); if (mid * f.y <= groundRel(mid * fh, px, pz, w)) hi = mid; else lo = mid; }
            groundRel(hi * fh, hitX, hitZ, water);
            return hi;
        }
        prev = s; s += std::max(0.6, s * 0.03);
    }
    return -1;
}
