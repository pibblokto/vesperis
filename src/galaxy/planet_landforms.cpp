// E-07: the traits' landforms (R-305): `landformsAt`, what a world's traits build at a point (canyons, mesas, rifts, spires,
// polygons, glaciers, the new landforms' point features...), and the meteorites' strewn fields it alone reads
#include "planet_internal.h"

namespace {
// the meteorites' strewn fields: on 22% of the cells of a 70 km grid an ellipse 6-25 km long and 1.5-4 km wide along the way the body
// came in and broke up, its middle `c`, its way `dir` (downrange, where the heaviest masses went furthest) and across it `perp`
inline PointGrid strewnGrid(const BodyGen& g) { return {70.0, 0.22, g.sE + 500}; }
struct StrewnField { Vec3 c, dir, perp; double lenKm, widthKm; };
inline StrewnField strewnOf(const FeatureHit& f) {
    StrewnField S; S.c = f.c;
    const Vec3 cn = normalize(f.c);
    const Vec3 e1 = normalize(cross(cn, std::fabs(cn.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(cn, e1);
    const double a = TAU * f.u[0];
    S.dir = e1 * std::cos(a) + e2 * std::sin(a); S.perp = cross(cn, S.dir);
    S.lenKm = 6.0 + 19.0 * f.u[3]; S.widthKm = 1.5 + 2.5 * f.u[4];
    return S;
}
}   // namespace
void landformsAt(const BodyGen& g, const Vec3& p, const Vec3& unit, double detailM, const LandCtx& c, LandOut& o) {
    if (!g.anyTrait()) return;
    const double R = g.R;
    // a province: about a third of the world, its own for every landform
    auto prov = [&](int k, double lo, double hi) { return smoothstep(lo, hi, fbm3(p / (R * 0.22) + Vec3(31.7 * k, 11.3 * k, 7.9 * k), g.sE + 200 + k, 2) * lodFade(R * 0.22, detailM)); };
    auto tu = [&](int k) { return unitFromHash(mix64(g.seed ^ (0x1A4DULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL))); };
    auto setMat = [&](int m, double a) { o.mat = m; o.albedo = a; };
    double land = c.landMask;
    for (int ti = 0; ti < TRAIT_SLOTS; ti++) {
        int t = g.traits[ti];
        if (!t) { if (ti < 3) ti = 2; continue; }   // Q-01: R-304's slots end at their first empty one; the new slots' own after them
        const double mark0 = o.mark; o.mark = -1;   // which trait set the mark (the last to, as before)
        switch (t) {
        case TR_CANYONS: {
            // a province of dry, flat, raised ground incised by a network of canyons (worley boundaries at 20 km, 0.6-1.3 km
            // wide, 150-600 m deep) and their side canyons (5 km, a third as wide and deep); the walls step down in three terraces
            double mask = land * prov(1, 0.1, 0.4) * smoothstep(0.45, 0.25, c.moist) * smoothstep(0.4, 0.2, c.relief0) * smoothstep(30, 150, c.hS);
            double w1 = 0.3 + 0.35 * tu(2), w2 = 0.1 + 0.12 * tu(3);   // the half-widths, km
            if (mask < 0.01 || (featFade(w1 * 2, detailM) <= 0 && !matVisible(w1, detailM))) break;   // by the canyons' own width (the 512 m ring draws them, coarsely; it was `scaleVisible(0.3)`: nothing beyond the 64 m ring)
            double depth = (150 + 450 * tu(1)) * mask;
            double wallness = 0;
            auto cut = [&](double cellKm, uint64_t seed, double width, double dep) {
                double f = featFade(width * 2, detailM);
                bool mv = matVisible(width, detailM);   // the walls' rock wherever a side spans half a cell
                if (f <= 0 && !mv) return 0.0;
                Worley3 w = worley3(p / cellKm, seed);
                double d = (w.f2 - w.f1) * cellKm * 0.5;   // km from the boundary
                double tt = clampd(1 - d / width, 0, 1);    // 1 at the centre line
                double s = tt * 3, k = std::floor(s), fr = s - k;
                double prof = tt >= 1 ? 1.0 : (k + smoothstep(0.25, 0.75, fr)) / 3;
                if (mv && prof > 0.03 && prof < 0.97) wallness = std::max(wallness, 1.0);
                return -dep * prof * f;
            };
            o.dhSmooth += cut(20.0, g.sE + 210, w1, depth) + cut(5.0, g.sE + 211, w2, depth * 0.4);
            if (wallness > 0 && mask > 0.5) { setMat(MAT_ROCK, 0.42); o.mark = 1; }
            break;
        }
        case TR_MESAS: {
            // badlands: flat-topped mesas 0.8-1.6 km across and 40-160 m high on half the cells of a 1.4 km grid, buttes and
            // hoodoos on a sixth, steep rims with a talus foot, in a dry flat province
            double mask = land * prov(2, 0.1, 0.35) * smoothstep(0.5, 0.3, c.moist) * smoothstep(0.4, 0.15, c.relief0);
            if (mask < 0.01 || (featFade(1.1, detailM) <= 0 && !matVisible(0.11, detailM))) break;   // by the biggest cap's width and its rim
            Worley3 w = worley3(p / 1.4, g.sE + 220);
            double u = unitFromHash(mix64(w.id1 + 7));
            double dh = 0; bool rim = false;
            if (u < 0.5) {
                double capH = 60 + 160 * unitFromHash(mix64(w.id1 + 8)), rad = 0.3 + 0.25 * unitFromHash(mix64(w.id1 + 9));
                double x = w.f1 / rad;
                double prof = x < 0.8 ? 1.0 : (x < 1.0 ? 1 - 0.85 * smoothstep(0.8, 1.0, x) : 0.15 * std::max(0.0, 1 - (x - 1) / 0.5));
                dh = capH * prof * featFade(2 * rad, detailM);                              // a cap's own fade
                rim = prof > 0.05 && prof < 0.95 && matVisible(0.2 * rad, detailM);   // the steep rim, a fifth of the radius wide
            } else if (u < 0.66) {
                double capH = 30 + 90 * unitFromHash(mix64(w.id1 + 8)), rad = 0.06 + 0.06 * unitFromHash(mix64(w.id1 + 9));
                double x = w.f1 / rad;
                double prof = x < 0.7 ? 1.0 : (x < 1 ? 1 - 0.9 * smoothstep(0.7, 1.0, x) : 0.1 * std::max(0.0, 1 - (x - 1) / 0.6));
                dh = capH * prof * featFade(2 * rad, detailM);
                rim = prof > 0.05 && prof < 0.95 && matVisible(0.3 * rad, detailM);
            }
            o.dhSmooth += dh * mask;
            if (rim && mask > 0.5) { setMat(MAT_ROCK, 0.4); o.mark = 1; }
            break;
        }
        case TR_KARST: {
            // tower karst: steep towers 50-210 m tall and 70-250 m across, half the cells of a 500 m grid, in warm wet
            // lowlands; sinkholes 60-160 m across between them
            double mask = land * prov(3, 0.1, 0.4) * smoothstep(0.45, 0.6, c.moist) * smoothstep(8, 16, c.T) * smoothstep(0.35, 0.15, c.relief0) * smoothstep(5, 30, c.hS) * smoothstep(900, 500, c.hS);
            if (mask < 0.01 || (featFade(0.25, detailM) <= 0 && !matVisible(0.07, detailM))) break;   // by the biggest tower and its walls
            double dh = 0; bool wall = false;
            visitFeatures(p, R, 0.5, 0.55, g.sE + 230, 0.5, [&](const FeatureHit& f) {
                double r = 0.035 + 0.09 * f.u[3], x = f.d / r;
                if (x >= 1.15) return;
                double hgt = 50 + 160 * f.u[4];
                double prof = x < 1 ? (1 - smoothstep(0.62, 1.0, x)) * (0.75 + 0.25 * std::cos(x * PI * 0.5)) : 0.0;   // a rounded top, walls over the outer 38%
                prof += 0.06 * clampd((1.15 - x) / 0.15, 0, 1) * (x > 1 ? 1 : 0) + (x < 1 ? 0.06 : 0);
                dh += hgt * prof * featFade(2 * r, detailM);   // a tower's own fade
                if (x > 0.5 && x < 1.05 && matVisible(0.55 * r, detailM)) wall = true;   // the walls, the outer 55% of the radius
            });
            visitFeatures(p, R, 0.45, 0.3, g.sE + 231, 0.4, [&](const FeatureHit& f) {
                double r = 0.03 + 0.05 * f.u[3], x = f.d / r;
                if (x >= 1) return;
                dh -= (8 + 25 * f.u[4]) * (1 - x * x) * (1 - x * x) * featFade(2 * r, detailM);
            });
            o.dhSmooth += dh * mask;
            if (wall && mask > 0.5) { setMat(MAT_ROCK, 0.45); o.mark = 1; }
            break;
        }
        case TR_RIFT: {
            // one great rift: the zero line of a broad noise; a flat floor 600-1500 m down between stepped walls, with
            // raised shoulders; the floor (`riftFloor`) takes lakes on living worlds
            double f = lodFade(R * 0.1, detailM);
            double n = gnoise3(p / (R * 0.12) + Vec3(3.1, 7.7, 1.3), g.sE + 240);
            double W = 0.02 + 0.015 * tu(4);
            double a = std::fabs(n);
            double depth = (600 + 900 * tu(5)) * land;
            double floor = smoothstep(W, W * 0.5, a);
            double s = floor * 3, k = std::floor(s), fr = s - k;
            double stepped = floor >= 1 ? 1.0 : (k + smoothstep(0.2, 0.8, fr)) / 3;   // three fault steps down each wall
            double shoulder = smoothstep(W * 2.2, W * 1.15, a) * smoothstep(W * 0.9, W * 1.15, a);
            o.dhSmooth += (-depth * stepped + 250 * land * shoulder) * f;
            double fl = smoothstep(W * 0.5, W * 0.3, a) * land * f;
            o.riftFloor = std::max(o.riftFloor, fl);
            if (fl > 0.5) { o.flatten = std::max(o.flatten, fl); o.flatOffset = -4; }
            if (stepped > 0.05 && stepped < 0.95) { setMat(MAT_ROCK, 0.4); o.mark = land; }
            break;
        }
        case TR_ESCARPMENTS: {
            // one or two cliff lines thousands of kilometres long: a step of 150-600 m across the zero line of a broad noise,
            // a talus foot then the cliff over 300 m (or the sampling scale), the height wandering along the line
            int nS = 1 + (tu(6) < 0.5 ? 1 : 0);
            bool cliff = false;
            for (int k = 0; k < nS; k++) {
                double wl = R * 0.2;
                double n = gnoise3(p / wl + Vec3(11.3 * k + 2.2, 5.1, 9.7 * k), g.sE + 250 + k);
                double stepH = (150 + 450 * tu(7 + k)) * land;
                double w = std::max(0.0003, 1.5 * detailM / (wl * 1000.0));
                double tt = clampd((n + w) / (2 * w), 0, 1);
                double prof = tt < 0.35 ? 0.15 * (tt / 0.35) * (tt / 0.35) : 0.15 + 0.85 * (tt - 0.35) / 0.65;
                double along = 0.7 + 0.3 * gnoise3(p / (wl * 0.15), g.sE + 260 + k);
                o.dhSmooth += stepH * prof * along;
                if (tt > 0.05 && tt < 0.95) cliff = true;
            }
            if (cliff) { setMat(MAT_ROCK, 0.42); o.mark = land; }
            break;
        }
        case TR_GLACIAL: {
            // ice fills the valleys of the cold high country: a tongue six metres under the smooth ground where the ground
            // lies below it, crevassed (`glacier` sets the ice); fjords come from the river layer on living worlds
            double cold = smoothstep(2, -5, c.T) * smoothstep(0.15, 0.4, c.relief0) * land;
            if (cold < 0.01) break;
            double glac = cold * smoothstep(-8, 25, c.hS - c.hM);   // the valley's depth on the material chain, the same in every ring
            if (glac < 0.02 || !matVisible(0.5, detailM)) break;     // a tongue is half a kilometre wide: the rings to 512 m carry it, the far ring and the map read the smooth ground alone
            if (scaleVisible(0.04, detailM)) { Worley3 w = worley3(p / 0.05, g.sE + 270); o.dhFine -= 3.0 * (1 - smoothstep(0.02, 0.08, w.f2 - w.f1)) * glac; }
            o.flatten = std::max(o.flatten, glac); o.flatOffset = -6;
            o.glacier = std::max(o.glacier, glac);
            if (glac > 0.5) o.mark = glac;
            break;
        }
        case TR_INSELBERGS: {
            // lone steep mountains 150-500 m tall and 0.8-3 km across on the plains, one per 25 km cell in three
            double mask = land * smoothstep(0.35, 0.15, c.relief0);
            if (mask < 0.01 || (featFade(3.2, detailM) <= 0 && !matVisible(1.1, detailM))) break;   // by the biggest mountain (the 512 m ring draws the big ones; `lodFade(2.0)` drew none beyond the 64 m ring)
            double dh = 0; bool rock = false;
            visitFeatures(p, R, 25.0, 0.3, g.sE + 280, 0.12, [&](const FeatureHit& f) {
                double r = 0.4 + 1.2 * f.u[3], x = f.d / r;
                if (x >= 1.2) return;
                double hgt = 150 + 350 * f.u[4];
                double prof = x < 1 ? smoothstep(1.0, 0.5, x) * (0.7 + 0.3 * std::cos(x * PI * 0.5)) + 0.06 : 0.06 * (1.2 - x) / 0.2;
                dh += hgt * prof * featFade(2 * r, detailM);   // a mountain's own fade
                if (x > 0.35 && x < 1.05 && matVisible(0.7 * r, detailM)) rock = true;
            });
            o.dhSmooth += dh * mask;
            if (rock && mask > 0.5) { setMat(MAT_ROCK, 0.42); o.mark = 1; }
            break;
        }
        case TR_CINDER_FIELD: {
            // a volcanic province: cinder cones 140 m tall on a 2.5 km grid, fresh lava sheets on a third of the 5 km cells
            // (dark, ropy), strings of collapse pits along fissures
            double mask = land * prov(4, 0.25, 0.5);
            if (mask < 0.01) break;
            double dh = 0; bool dark = false;
            if (featFade(1.0, detailM) > 0 || matVisible(1.0, detailM)) {   // the cones by their width (0.6-1.5 km), the dark cinder wherever one spans half a cell
                bool cal = false;
                double cones = volcanoField(p, R, 2.5, 0.45, g.sE + 290, 140.0, cal);
                dh += cones * featFade(1.0, detailM);
                if (cones > 15 && matVisible(1.0, detailM)) dark = true;
            }
            if (featFade(5.0, detailM) > 0 || matVisible(5.0, detailM)) {   // the sheets: cells of 5 km
                Worley3 w = worley3(p / 5.0, g.sE + 291);
                if (unitFromHash(mix64(w.id1 + 3)) < 0.35) {
                    double edge = smoothstep(0.0, 0.12, w.f2 - w.f1);
                    double ropy = scaleVisible(0.02, detailM) ? 1.5 * ridged3(p / 0.03, g.sE + 292, 2) : 0.0;
                    dh += (6.0 + ropy) * edge * featFade(5.0, detailM);
                    if (edge > 0.5 && matVisible(5.0, detailM)) dark = true;
                }
            }
            if (scaleVisible(0.9, detailM)) {   // the pit chains, 144 m across
                Worley3 w = worley3(p / 0.9, g.sE + 293);
                if (unitFromHash(mix64(w.id1 + 5)) < 0.15 && w.f1 < 0.08) dh -= 12.0 * (1 - w.f1 / 0.08) * (1 - w.f1 / 0.08) * featFade(0.144, detailM);
            }
            o.dhSmooth += dh * mask;
            if (dark && mask > 0.5) { setMat(MAT_BASALT, 0.15); o.mark = 1; }
            break;
        }
        case TR_CHAOS: {
            // the crust broken into tilted blocks 2-6 km across, lifted or dropped by up to 80 m, cracks 90 m deep between
            double f = featFade(4.0, detailM), fc = featFade(0.24, detailM);   // the blocks by their size, the cracks (240 m wide, 90 m deep) by theirs
            bool mv = matVisible(0.24, detailM);
            if (f <= 0 && !mv) break;
            Worley3 w = worley3(p / 4.0, g.sE + 300);
            uint64_t id = w.id1;
            double lift = (unitFromHash(mix64(id + 1)) - 0.5) * 160;
            Vec3 tilt = normalize(Vec3(unitFromHash(mix64(id + 2)) - 0.5, unitFromHash(mix64(id + 3)) - 0.5, unitFromHash(mix64(id + 4)) - 0.5));
            double slope = 0.02 + 0.05 * unitFromHash(mix64(id + 5));
            double dh = lift + dot(p - w.c1 * 4.0, tilt) * slope * 1000.0;
            double crack = 1 - smoothstep(0.03, 0.09, w.f2 - w.f1);
            o.dhSmooth += (dh * f - 90 * crack * fc) * land;
            if (crack > 0.5 && mv) { setMat(MAT_ROCK, 0.3); o.mark = 1; }
            break;
        }
        case TR_POLYGONS: {
            // patterned ground: a honeycomb of low ridges 40-120 m across on cold flat ground
            double cold = kindHas(g.type, KT_FROZEN) ? 1.0 : smoothstep(4, -2, c.T);
            double mask = cold * land * smoothstep(0.25, 0.1, c.relief0);
            double cell = 0.04 + 0.08 * tu(8);
            if (mask < 0.01 || !scaleVisible(cell, detailM)) break;
            Worley3 w = worley3(p / cell, g.sE + 310);
            double ridge = 1 - smoothstep(0.0, 0.006, (w.f2 - w.f1) * cell);
            o.dhFine += (1.5 * ridge * ridge - 0.4 * (1 - ridge)) * mask * lodFade(cell, detailM);
            if (mask > 0.5) o.mark = mask;
            break;
        }
        case TR_YARDANGS: {
            // wind-carved ridges along the wind, 80-200 m apart and 5-20 m high, broken along their length, in the dry flats
            double mask = land * prov(5, 0.1, 0.4) * smoothstep(0.3, 0.16, c.moist) * smoothstep(0.3, 0.1, c.relief0);
            if (mask < 0.01) break;
            double wl = 0.08 + 0.12 * tu(9);
            double f = lodFade(wl, detailM);
            if (f <= 0) break;
            Vec3 dd = normalize(Vec3(std::cos(g.windAngle), std::sin(g.windAngle), 0.1));
            Vec3 dp = normalize(cross(dd, unit));
            double across = dot(p, dp) / wl * TAU + 2.0 * gnoise3(p / 1.5, g.sE + 320);
            double alongN = smoothstep(0.25, 0.75, 0.5 + 0.5 * gnoise3(p / 0.35 + Vec3(0, 0, 9), g.sE + 321));   // the ridges break into segments a few hundred metres long
            o.dhFine += (5 + 15 * tu(10)) * std::pow(0.5 + 0.5 * std::cos(across), 3.0) * alongN * mask * f;
            if (mask > 0.5) o.mark = mask;
            break;
        }
        case TR_ERG: {
            // a sea of giant dunes: draa 1-2.5 km apart and 40-120 m high across the wind in the dry lowlands, sand
            double mask = land * smoothstep(0.34, 0.2, c.moist) * smoothstep(0.35, 0.15, c.relief0) * smoothstep(900, 400, c.hS);
            if (mask < 0.01) break;
            if (mask > 0.5) { setMat(MAT_SAND, livingGround(g.type) ? 0.6 : -1.0); o.mark = mask; }   // the sand sea is kilometres across: sand in every ring and on the maps, whether or not its draa can show (going with the draa, it would be dust beyond the 64 m ring); an airless or dry type keeps its own albedo under it, so its globe keeps its mottling (a flat 0.6 made a dune-sea world one yellow ball)
            double wl = 1.0 + 1.5 * tu(11), amp = 40 + 80 * tu(12);
            double f = lodFade(wl, detailM);   // the draa are a periodic pattern: the octave rule
            if (f <= 0) break;
            Vec3 dd = normalize(Vec3(std::cos(g.windAngle), std::sin(g.windAngle), 0.15 * std::sin(g.windAngle * 2)));
            Vec3 dp = normalize(cross(dd, unit));
            double along = dot(p, dd) / wl * TAU, across = dot(p, dp) / wl * TAU;
            double bend = fbm3(p / 8.0, g.sE + 330, 2) * 3.0;
            double v = std::pow(0.5 + 0.5 * std::sin(along + bend + 0.5 * std::sin(across * 0.7)), 1.6);
            o.dhSmooth += amp * v * mask * f;
            o.slip = std::max(o.slip, mask * f * smoothstep(-0.1, -0.5, std::cos(along + bend + 0.5 * std::sin(across * 0.7))));   // the draa's faces downwind
            break;
        }
        case TR_SALT_FLATS: {
            // white playas in the closed basins of dry country: dead flat three metres under the smooth ground, cracked into
            // polygons a dozen metres across
            if (!matVisible(2.5, detailM)) break;   // the playas (a few kilometres across, the lows of the 5 km basin noise) unfaded wherever one spans half a cell: every ring carries them, the world map does not (the noise would alias on its texels)
            double basin = smoothstep(-0.2, -0.45, fbm3(p / 5.0, g.sB, 3));
            double mask = land * basin * smoothstep(0.34, 0.2, c.moist) * smoothstep(0.25, 0.1, c.relief0) * smoothstep(700, 300, c.hS);
            if (mask < 0.01) break;
            o.flatten = std::max(o.flatten, mask); o.flatOffset = -3;
            if (scaleVisible(0.012, detailM)) { Worley3 w = worley3(p / 0.012, g.sE + 340); o.dhFine += 0.15 * (1 - smoothstep(0.0, 0.5, (w.f2 - w.f1) * 12)) * mask; }   // the cracks: polygons of 12 m, the 4 m ring's (B-404: gated at 5 m before, under every ring)
            if (mask > 0.6) { setMat(MAT_SALT, 0.9); o.mark = mask; }
            break;
        }
        case TR_TRAPS: {
            // stepped terraces of old lava sheets: the ground quantised into steps of 40-90 m with steep risers, in a province
            double mask = land * prov(6, 0.2, 0.45);
            if (mask < 0.01) break;
            double step = 40 + 50 * tu(13);
            double run = step / std::max(c.slope, 0.02);        // a terrace's run in metres on the material chain's slope
            double f = featFade(run / 1000.0, detailM);          // the relief where a terrace spans a few samples (it was `lodFade(1.5)`: none beyond the 64 m ring)
            bool mv = matVisible(0.4 * run / 1000.0, detailM);   // the riser, 40% of the run, decides the basalt wherever it spans half a cell
            if (f <= 0 && !mv) break;
            double q = std::floor(c.h / step) * step, fr = (c.h - q) / step;
            double terr = q + step * smoothstep(0.3, 0.7, fr);
            o.dhFine += (terr - c.h) * mask * f;
            if (mv && fr > 0.3 && fr < 0.7 && mask > 0.5) { setMat(MAT_BASALT, 0.22); o.mark = 1; }
            break;
        }
        case TR_GREAT_BASIN: {
            // one vast impact basin, a fifth to two fifths of the radius across: a bowl 1.5-3 km deep flooded flat, a rim,
            // a second ring, ejecta
            Vec3 cdir = normalize(Vec3(tu(14) - 0.5, tu(15) - 0.5, tu(16) - 0.5));
            double rad = R * (0.22 + 0.18 * tu(17));
            double x = std::acos(clampd(dot(unit, cdir), -1, 1)) * R / rad;
            if (x > 2.2) break;
            double depth = 1500 + 1500 * tu(18);
            double bowl = x < 1 ? -(1 - x * x) : 0;
            if (bowl < -0.55) bowl = -0.55;
            double rim = 0.45 * std::exp(-((x - 1) * (x - 1)) / (0.07 * 0.07));
            double ring2 = 0.18 * std::exp(-((x - 1.55) * (x - 1.55)) / (0.05 * 0.05));
            double ejecta = x > 1 ? 0.12 * std::exp(-(x - 1) / 0.35) : 0;
            o.dhSmooth += depth * (bowl + rim + ring2 + ejecta);
            if (bowl <= -0.55) setMat(MAT_BASALT, 0.18);
            else if (x > 0.93 && x < 1.07) { setMat(MAT_ROCK, 0.4); o.mark = 1; }
            break;
        }
        case TR_CORONAE: {
            // ring-shaped volcanic ridges 100-250 km across with a moat and a sunken centre, half the cells of a grid of 0.3 R
            if (!scaleVisible(20.0, detailM)) break;
            double dh = 0;
            visitFeatures(p, R, R * 0.3, 0.5, g.sE + 350, 0.5, [&](const FeatureHit& f) {
                double r = 50 + 75 * f.u[3], x = f.d / r;
                if (x > 1.5) return;
                double ridge = 0.6 * std::exp(-((x - 1) * (x - 1)) / (0.1 * 0.1));
                double moat = -0.25 * std::exp(-((x - 1.2) * (x - 1.2)) / (0.08 * 0.08));
                double centre = x < 0.9 ? -0.15 * (1 - x / 0.9) : 0;
                double fine = scaleVisible(2.0, detailM) ? 0.08 * ridged3(p / 3.0, g.sE + 351, 2) * smoothstep(1.5, 0.8, x) : 0.0;
                dh += (600 + 400 * f.u[4]) * (ridge + moat + centre + fine);
            });
            o.dhSmooth += dh * lodFade(R * 0.05, detailM);
            if (std::fabs(dh) > 200) o.mark = 1;
            break;
        }
        case TR_SPIRES: {
            if (kindHas(g.type, KT_CRYSTAL)) {   // crystal spires 10-35 m tall in fields
                double mask = prov(7, 0.2, 0.45);
                if (mask < 0.01 || detailM > 5) break;   // the 4 m ring's, like the ice spires (B-404: `scaleVisible(0.01)` wanted a 3 m sample, so no ring ever drew them; the crystal landmarks stood on bare ground)
                Worley3 w = worley3(p / 0.05, g.sE + 360);
                if (unitFromHash(mix64(w.id1 + 2)) < 0.4) {
                    double r = 0.006 + 0.006 * unitFromHash(mix64(w.id1 + 3));
                    double x = w.f1 * 0.05 / r;
                    if (x < 1) { o.dhFine += (10 + 25 * unitFromHash(mix64(w.id1 + 4))) * std::pow(1 - x, 1.4) * mask; if (x < 0.9 && mask > 0.5) { setMat(MAT_QUARTZ, 0.88); o.mark = 1; } }
                }
            } else {   // ice spires: penitentes 3-8 m tall on a 12 m grid on the flats (the 4 m ring's, so within 120 m)
                double mask = prov(7, 0.15, 0.4) * smoothstep(0.25, 0.1, c.relief0);
                if (mask < 0.01 || detailM > 5) break;
                Worley3 w = worley3(p / 0.012, g.sE + 361);
                double x = w.f1 * 0.012 / 0.005;
                if (x < 1) { o.dhFine += (3.0 + 5.0 * unitFromHash(mix64(w.id1 + 4))) * (1 - x) * (1 - x) * mask; if (mask > 0.5) o.mark = mask; }
            }
            break;
        }
        case TR_GEYSERS: {
            // geyser basins: sinter mounds 12 m high and 0.3-0.9 km across with 1.6 m terraces and a pool at the vent, pale crusts
            double mask = land * smoothstep(0.3, 0.1, c.relief0);
            if (mask < 0.01 || (featFade(1.8, detailM) <= 0 && !matVisible(1.8, detailM))) break;   // by the biggest basin
            double dh = 0; bool sinter = false;
            visitFeatures(p, R, 20.0, 0.35, g.sE + 370, 0.1, [&](const FeatureHit& f) {
                double r = 0.3 + 0.6 * f.u[3], x = f.d / r;
                if (x >= 1.0) return;
                double mound = 12.0 * (1 - x * x);
                double terr = std::floor(mound / 1.6) * 1.6 + 1.6 * smoothstep(0.3, 0.7, std::fmod(mound, 1.6) / 1.6);
                dh += (terr * (0.6 + 0.4 * f.u[4]) + (x < 0.12 ? -3.0 * (1 - x / 0.12) : 0.0)) * featFade(2 * r, detailM);   // a mound's own fade
                if (matVisible(2 * r, detailM)) sinter = true;   // the crust wherever the mound spans half a cell
            });
            o.dhFine += dh * mask;
            if (sinter && mask > 0.5) { setMat(MAT_SALT, 0.85); o.mark = 1; }
            break;
        }
        case TR_GREAT_SHIELD: {
            // Olympus Mons: one shield volcano (`greatShieldOf`) 300-600 km across on a world of Mars' size (a tenth to a sixth of a smaller
            // world's radius), 22 km high there and lower as the radius (the gravity) grows: a convex dome of lava lobes running down from the
            // summit, the caldera's nested pits at its top, a cliff ring a fifth of its height at its foot (Olympus' basal scarp), the aureole's
            // ridged lobes spread over the plain beyond; out of a sea it stands as an island (it is hundreds of kilometres: every ring and the
            // map carry it)
            const GreatShield S = greatShieldOf(g);
            const double x = std::acos(clampd(dot(unit, S.c), -1, 1)) * R / S.radKm;
            if (x > 2.3) break;
            const double foot = 0.21;   // the cliff ring, of the height
            const Vec3 e1 = normalize(cross(S.c, std::fabs(S.c.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(S.c, e1);
            const double ang = std::atan2(dot(unit, e2), dot(unit, e1));
            double dh = 0; bool scarp = false, pit = false, lava = false;
            if (x < 1.0) {
                dh = S.heightM * (foot + (1 - foot) * std::pow(1 - std::pow(x, 1.6), 1.15));
                // the lobes: streaks running down from the summit (a noise wrapped round it, 60 a turn, 25 km along), a radius' sixtieth wide
                const double lobe = fbm3(Vec3(std::cos(ang) * 60.0, std::sin(ang) * 60.0, x * S.radKm / 25.0), g.sE + 400, 3);
                const double wKm = std::max(0.5, x * S.radKm / 60.0);
                dh += 80.0 * lobe * lodFade(wKm * 2, detailM);
                lava = lobe > 0.12 && x < 0.9 && matVisible(wKm, detailM);
                double cal = 0;   // the caldera: the deepest of its pits here
                for (int k = 0; k < S.nPits; k++) {
                    const double xp = std::acos(clampd(dot(unit, S.pit[k]), -1, 1)) * R / S.pitKm[k];
                    if (xp >= 1.0) continue;
                    cal = std::min(cal, -S.pitDepthM[k] * (xp < 0.8 ? 1.0 : 1.0 - smoothstep(0.8, 1.0, xp)));
                    if (matVisible(0.2 * S.pitKm[k], detailM)) pit = true;
                }
                dh += cal;
            } else {
                const double sw = std::max(0.025, 1.5 * detailM / (S.radKm * 1000.0));   // the scarp's run: a few samples at least
                dh = S.heightM * foot * smoothstep(1.0 + sw, 1.0, x);
                scarp = x < 1.0 + sw;
                const double lobes = smoothstep(0.15, 0.55, 0.5 + 0.5 * std::sin(5.0 * ang + 3.0 * fbm3(unit * 4.0, g.sE + 401, 2) + TAU * tu(61)));
                dh += 450.0 * lobes * smoothstep(2.3, 1.15, x) * ridged3(p / 18.0, g.sE + 402, 3) * lodFade(18.0, detailM);   // the aureole
            }
            o.dhSmooth += dh;
            if (scarp) { setMat(MAT_ROCK, 0.4); o.mark = 1; }
            else if (pit) { setMat(MAT_BASALT, 0.16); o.mark = 1; }
            else if (lava) setMat(MAT_BASALT, 0.2);
            break;
        }
        case TR_TESSERAE: {
            // Venus' tesserae (Alpha and Ovda Regio): plateaus 1-2.5 km over the plains with steep margins (a province), their crust folded into
            // ridges 8-16 km apart and broken across by graben 3-6 km apart, a third of that wide and 200-400 m deep: two directions crossing at
            // a wide angle (planes through the world: over a province they cross at about the angle between them), a woven ground
            const double plat = land * prov(20, 0.08, 0.2);
            if (plat < 0.01) break;
            const Vec3 dA = normalize(Vec3(tu(62) - 0.5, tu(63) - 0.5, tu(64) - 0.5));
            const Vec3 dB = normalize(cross(dA, normalize(Vec3(tu(65) - 0.5, tu(66) - 0.5, tu(67) - 0.5))));
            const double warp = 5.0 * fbm3(p / 45.0, g.sE + 410, 2);
            const double lamA = 8.0 + 8.0 * tu(68), lamB = 3.0 + 3.0 * tu(69);
            const double sA = (dot(p, dA) + warp) / lamA, sB = (dot(p, dB) + 0.6 * warp) / lamB;
            const double ridge = std::pow(0.5 + 0.5 * std::cos(TAU * sA), 1.5);       // the folds' rounded crests
            const double gd = std::fabs(sB - std::floor(sB) - 0.5) / 0.17;              // across a graben (its half-width 17% of the spacing)
            const double graben = gd < 1 ? 1 - smoothstep(0.55, 1.0, gd) : 0.0;       // a flat floor between steep walls
            const double gW = lamB * 0.34;   // the graben's width, km
            o.dhSmooth += (1000.0 + 1500.0 * tu(70)) * plat + plat * ((300.0 + 300.0 * tu(71)) * (ridge - 0.5) * lodFade(lamA, detailM) - (200.0 + 200.0 * tu(72)) * graben * featFade(gW, detailM));
            if (plat > 0.6 && gd > 0.5 && gd < 1.0 && matVisible(gW * 0.25, detailM)) { setMat(MAT_ROCK, 0.46); o.mark = 1; }   // the graben's walls
            else if (plat > 0.6) o.mark = 0.6;
            break;
        }
        case TR_LABYRINTH: {
            // Noctis Labyrinthus: a rise 0.8-2 km high (a province) cut by a maze of intersecting fault troughs, the boundaries of 30 km cells and
            // of 12 km cells, 6-10 and 2.5-4 km wide, their flat floors 2-5 and 0.9-2.4 km down between walls stepped by three faults; the blocks
            // between stand as plateaus and mesas
            const double mask = land * prov(21, 0.12, 0.3);
            if (mask < 0.01) break;
            bool wall = false, floor = false;
            auto trough = [&](double cellKm, uint64_t seed, double halfW, double dep) {
                const double f = featFade(halfW * 2, detailM);
                const bool mv = matVisible(halfW * 0.15, detailM);   // a fault step's run
                if (f <= 0 && !mv) return 0.0;
                const Worley3 w = worley3(p / cellKm, seed);
                const double d = (w.f2 - w.f1) * cellKm * 0.5;   // km from the boundary
                const double tt = clampd(1 - d / halfW, 0, 1);    // 1 at the middle line
                const double s3 = clampd(tt / 0.5, 0, 1) * 3, k = std::floor(s3), fr = s3 - k;
                const double prof = tt >= 0.5 ? 1.0 : (k + smoothstep(0.25, 0.75, fr)) / 3;   // the floor over the middle half, three faults down each wall
                if (mv && prof > 0.03 && prof < 0.97) wall = true;
                if (mv && prof >= 0.97) floor = true;
                return -dep * prof * f;
            };
            const double a = trough(30.0, g.sE + 420, 3.0 + 2.0 * tu(73), (2000.0 + 3000.0 * tu(74)) * mask);
            const double b = trough(12.0, g.sE + 421, 1.25 + 0.75 * tu(75), (900.0 + 1500.0 * tu(76)) * mask);
            o.dhSmooth += (800.0 + 1200.0 * tu(77)) * mask + std::min(a, b);
            if (mask > 0.5 && (wall || floor)) { if (wall) setMat(MAT_ROCK, 0.42); o.mark = 1; }
            break;
        }
        case TR_HOLLOWS: {
            // Mercury's hollows: clusters a few kilometres across (where a 6 km noise is high, in a province) of shallow flat-floored pits 60-500 m
            // across and 15-40 m deep, their outlines ragged, bright and bluish with a halo where the rock breathed out its volatile (Blewett et al.
            // 2011); the pits stand on the half-kilometre cells of a jittered grid (the 64 and 16 m rings carry them, the 512 m one their halos)
            const double mask = land * prov(22, 0.0, 0.3);
            if (mask < 0.01) break;
            const double cl = smoothstep(0.1, 0.4, fbm3(p / 6.0, g.sE + 431, 2));   // the cluster here
            if (cl <= 0) break;
            if (mask * cl > 0.5) o.mark = 0.6;   // the cluster's field at every scale (the scanner's 500 m scan reads it, the map takes no pit)
            if (!matVisible(0.12, detailM)) break;
            double dh = 0, bright = 0;
            visitFeatures(p, R, 0.5, 0.6, g.sE + 430, 0.9, [&](const FeatureHit& f) {
                const double r = (0.03 + 0.22 * f.u[3] * f.u[3]) * (0.6 + 0.4 * cl);
                const double x = f.d / r;
                if (x > 1.7) return;
                const double rag = 1 + 0.22 * gnoise3((p - f.c) * (1.5 / r) + Vec3(f.u[0] * 50, f.u[1] * 50, f.u[2] * 50), g.sE + 432);   // the ragged outline
                const double xr = x / rag;
                if (xr < 1) dh -= (15.0 + 25.0 * f.u[4]) * (xr < 0.7 ? 1.0 : 1 - smoothstep(0.7, 1.0, xr)) * featFade(2 * r, detailM);
                if (matVisible(2 * r, detailM)) bright = std::max(bright, smoothstep(1.7, 1.0, xr));
            });
            o.dhFine += dh * mask * cl;
            if (bright * mask * cl > 0.5) { setMat(MAT_SALT, 0.78); o.mark = 1; }
            break;
        }
        case TR_CRATER_CHAINS: {
            // catenae: lines of six to seventeen craters of one size, 30-280 km long, where a comet torn apart by its planet's tides struck in a row
            // (Ganymede's Enki Catena, Callisto's Gipul, Shoemaker-Levy 9 on Jupiter), on 15% of the cells of a grid of a fifth of the radius
            // (`chainOf`): bowls a fifth of their width deep, their rims touching
            const PointGrid G = chainGrid(g);
            double dh = 0; bool on = false;
            visitFeatures(p, R, G.cellKm, G.density, G.seed, 0.4, [&](const FeatureHit& f) {
                const double len = f.cellKm * (0.2 + 0.42 * f.u[1]), sp = len / (std::min(17, 6 + (int)(f.u[2] * 12)) - 1);
                if (f.d > 0.5 * len + 1.6 * sp) return;   // beyond the chain's craters
                const CraterChain C = chainOf(f);
                const Vec3 rel = p - C.c;
                const double along = dot(rel, C.dir), across = dot(rel, C.perp);
                for (int k = 0; k < C.n; k++) {
                    const double dx = along - C.at[k];
                    if (std::fabs(dx) > 2.0 * C.rKm[k] || std::fabs(across) > 2.0 * C.rKm[k]) continue;
                    const double x = std::sqrt(dx * dx + across * across) / C.rKm[k];
                    if (x > 2.0) continue;
                    const double bowl = x < 1 ? -(1 - x * x) : 0.0;
                    const double rimv = 0.3 * std::exp(-((x - 1) * (x - 1)) / (0.15 * 0.15));
                    const double ej = x > 1 ? 0.08 * std::exp(-(x - 1) / 0.35) : 0.0;
                    dh += C.depthM[k] * (bowl + rimv + ej) * featFade(2 * C.rKm[k], detailM);
                    if (x < 1.15 && matVisible(0.3 * C.rKm[k], detailM)) on = true;
                }
            });
            o.dhSmooth += dh;
            if (on) o.mark = 1;
            break;
        }
        case TR_PENITENTES: {
            // blades of ice in rows across the sun's path (each a pointed blade longer east and west than across, its rows north and south of
            // one another: they lean to the noon sun), in fields within 60 degrees of the equator: on a living world the snow of its cold dry
            // heights, blades 2-5 m tall in rows 10-14 m apart (the high Andes'; the 4 m ring's); on the ices of an airless or a thin-aired world
            // 4-10 m in rows 14-22 m apart, as Europa's might stand (Hobley et al. 2018); on a nitrogen world Pluto's bladed terrain, a hundred
            // times the size: ridges 150-500 m high on rows 3-6 km apart running east and west, broken into blades (Moore et al. 2018), seen from far
            const double absLat = std::fabs(std::asin(clampd(unit.z, -1, 1)));
            if (absLat > 62 * DEG) break;
            const bool living = KINDS[g.type].ground == GR_FELISIAN, giant = g.type == PT_NITROGEN;
            const double field = living ? land * smoothstep(-2.0, -8.0, c.T) * smoothstep(0.5, 0.3, c.moist) * smoothstep(1200.0, 2500.0, c.hS)
                                        : prov(23, -0.05, 0.25) * smoothstep(0.4, 0.15, c.relief0);
            const double mask = field * smoothstep(62 * DEG, 50 * DEG, absLat);
            if (mask < 0.01) break;
            const double rowM = giant ? 3000.0 + 3000.0 * tu(86) : (living ? 10.0 + 4.0 * tu(86) : 14.0 + 8.0 * tu(86));
            const double bladeM = giant ? 150.0 + 350.0 * tu(87) : (living ? 2.0 + 3.0 * tu(87) : 4.0 + 6.0 * tu(87));
            const double fade = giant ? lodFade(rowM / 1000.0, detailM) : (detailM <= 5 ? 1.0 : 0.0);   // the small blades the 4 m ring's alone, as the ice spires
            if (fade > 0) {
                const double lat = std::asin(clampd(unit.z, -1, 1)), north = lat * R * 1000.0;   // metres
                const double east = std::atan2(unit.y, unit.x) * R * 1000.0 * std::max(0.2, std::cos(lat));   // metres (a seam at the antimeridian, a row's one blade)
                const double alongM = rowM * 1.1;   // a blade's place along its row
                const int64_t i0 = (int64_t)std::floor(north / rowM);
                double best = 0;
                for (int64_t i = i0 - 1; i <= i0 + 1; i++) {   // the blades of the three nearest rows, each pointed: a diamond longer along its row
                    const double off = alongM * unitFromHash(hash2i(i, 0, g.sE + 440));   // the row's own phase
                    const int64_t j0 = (int64_t)std::floor((east - off) / alongM);
                    for (int64_t j = j0 - 1; j <= j0 + 1; j++) {
                        const uint64_t hb = hash2i(i, j, g.sE + 441);
                        const double cn = (i + 0.5 + 0.25 * (unitFromHash(hb) - 0.5)) * rowM, ce = off + (j + 0.5 + 0.3 * (unitFromHash(mix64(hb + 1)) - 0.5)) * alongM;
                        const double d = std::fabs(east - ce) / (0.48 * alongM) + std::fabs(north - cn) / (0.3 * rowM);
                        if (d < 1) best = std::max(best, std::pow(1 - d, 0.7) * (0.6 + 0.4 * unitFromHash(mix64(hb + 2))));
                    }
                }
                const double dh = bladeM * best * mask * fade;
                if (giant) o.dhSmooth += dh; else o.dhFine += dh;
            }
            if (mask > 0.5) { setMat(living ? MAT_SNOW : MAT_ICE, living ? 0.88 : 0.8); o.mark = mask; }
            break;
        }
        case TR_COLUMNS: {
            // columnar basalt: a province of thick lava sheets, lobes on the cells of a 2-5 km grid (two in three raised by their thickness, 15-40
            // m), each lobe's edge a cliff of columns (a colonnade: the Columbia River's coulees, the Giant's Causeway); within the 4 m ring the
            // edge runs along the columns' polygons (cells of 5-8 m: a column stands on whichever lobe its middle does) and the tops by the edge are
            // a pavement of the columns' heads stepped by up to a metre
            const double mask = land * prov(24, 0.12, 0.35) * smoothstep(0.4, 0.15, c.relief0);
            if (mask < 0.01) break;
            const double cellK = 2.0 + 3.0 * tu(93), colKm = (5.0 + 3.0 * tu(94)) / 1000.0;
            const Worley3 w = worley3(p / cellK, g.sE + 445);
            auto lobeH = [&](uint64_t id) { return unitFromHash(mix64(id + 11)) < 0.66 ? 15.0 + 25.0 * unitFromHash(mix64(id + 12)) : 0.0; };
            const double h1 = lobeH(w.id1), h2 = lobeH(w.id2);
            const double eM = (w.f2 - w.f1) * cellK * 0.5 * 1000.0;   // metres to the lobe's edge
            double hh, dFine = 0; bool cliff = false;
            if (detailM <= 5 && eM < 2.5 * colKm * 1000.0 && h1 != h2) {   // the edge along the columns
                const Worley3 cw = worley3(p / colKm, g.sE + 446);
                const Worley3 wc = worley3(cw.c1 * colKm / cellK, g.sE + 445);   // the lobe the column's middle stands on
                hh = lobeH(wc.id1);
                cliff = true;
            } else {
                const double wEdge = std::max(3.0, 0.75 * detailM);   // the cliff's run (a few samples at least)
                hh = h2 + (h1 - h2) * (0.5 + 0.5 * smoothstep(0.0, wEdge, eM));
                cliff = h1 != h2 && eM < 1.5 * wEdge;   // the mark at every scale, the basalt where the cliff spans a sample
            }
            if (detailM <= 5 && hh > 0 && eM < 40.0) {   // the pavement: the columns' heads by the edge, their joints
                const Worley3 cw = worley3(p / colKm, g.sE + 446);
                dFine = 0.9 * (unitFromHash(mix64(cw.id1 + 3)) - 0.5) - 0.3 * (1 - smoothstep(0.0, 0.6, (cw.f2 - cw.f1) * colKm * 1000.0));
            }
            o.dhSmooth += hh * mask;
            o.dhFine += dFine * mask;
            if (mask > 0.5 && cliff) o.mark = 1;
            if (mask > 0.5 && h1 != h2 && eM < 30.0 && matVisible(0.03, detailM)) setMat(MAT_BASALT, 0.2);   // the colonnade, its pavement and its foot: a band of
            break;                                                                                              // 30 m by the unfaded edge, in every ring that can hold it
        }
        case TR_MUD_VOLCANOES: {
            // fields of mud volcanoes on the low plains of a province (Azerbaijan's Gobustan; the mounds on Mars' plains): cones 6-50 m high and
            // 100-700 m across with a crater pool at the top, two to four tongues of grey mud running out one to three radii from the foot, the
            // mud cracked grey (`mudConeOf`)
            const double mask = land * prov(25, 0.1, 0.4) * smoothstep(0.4, 0.15, c.relief0) * (KINDS[g.type].ground == GR_FELISIAN ? smoothstep(1500.0, 400.0, c.hS) : 1.0);
            if (mask < 0.01) break;
            if (featFade(0.2, detailM) <= 0 && !matVisible(0.2, detailM)) break;
            const PointGrid G = mudGrid(g);
            double dh = 0, mud = 0;
            visitFeatures(p, R, G.cellKm, G.density, G.seed, 0.75, [&](const FeatureHit& f) {
                const PointSize S = mudConeOf(f);
                const double x = f.d / S.rKm;
                if (x > 4.0) return;
                if (x < 1) {
                    double cone = std::pow(1 - x, 1.3);
                    if (x < 0.12) cone -= 0.18 * (1 - x / 0.12);   // the crater pool
                    dh += S.heightM * cone * featFade(2 * S.rKm, detailM);
                    if (matVisible(S.rKm, detailM)) mud = 1;
                    return;
                }
                const Vec3 cn = normalize(f.c);
                const Vec3 e1 = normalize(cross(cn, std::fabs(cn.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(cn, e1);
                const Vec3 d = p - f.c;
                const double ang = std::atan2(dot(d, e2), dot(d, e1));
                const int nT = 2 + (int)(f.u[5] * 3);
                for (int k = 0; k < nT; k++) {   // the tongues
                    const double a0 = TAU * std::fmod(f.u[0] + 0.618034 * k, 1.0), len = 1.0 + 2.0 * std::fmod(f.u[1] + 0.414214 * k, 1.0);
                    const double wAng = 0.35 * (1.0 - 0.6 * (x - 1) / len), da = wrapAngle(ang - a0);
                    if (x > 1 + len || std::fabs(da) > wAng) continue;
                    const double shape = (1 - (da / wAng) * (da / wAng)) * smoothstep(1 + len, 1 + len - 0.4, x);
                    const double wKm = 2 * wAng * x * S.rKm;
                    dh += 1.5 * shape * featFade(wKm, detailM);
                    if (matVisible(wKm, detailM)) mud = std::max(mud, shape);
                }
            });
            o.dhFine += dh * mask;
            if (mud * mask > 0.5) { setMat(MAT_MUD, 0.34); o.mark = 1; }
            break;
        }
        case TR_INVERTED_RIVERS: {
            // Mars' Aeolis Dorsa: the beds of old rivers standing as sinuous flat-topped ridges 8-40 m high and 150-500 m wide where the plain
            // round them wore away, branching where two nets meet (the zero lines of a warped noise of 30 km and one of 10), broken where the wind
            // cut them through, on the dry plains of a province; their caps the cemented gravels of the beds
            const double mask = land * prov(26, 0.08, 0.32) * smoothstep(0.4, 0.2, c.moist) * smoothstep(0.35, 0.12, c.relief0);
            if (mask < 0.01) break;
            const double hw = 0.075 + 0.175 * tu(100);   // the main ridges' half-width, km
            const Vec3 q = p / 30.0;
            const Vec3 wq = q + Vec3(gnoise3(q * 0.6 + Vec3(3.1, 0, 0), g.sE + 451), gnoise3(q * 0.6 + Vec3(0, 7.7, 0), g.sE + 452), gnoise3(q * 0.6 + Vec3(0, 0, 5.3), g.sE + 453)) * 0.8;
            auto bed = [&](const Vec3& at, double wl, uint64_t seed, double halfW) {
                const double d = std::fabs(gnoise3(at, seed)) * wl / 1.6;   // km from the line (the noise's slope about 1.6 a wavelength)
                return 1 - smoothstep(0.6 * halfW, 1.15 * halfW, d);        // the flat top over the middle 60%, steep flanks
            };
            const double main = bed(wq, 30.0, g.sE + 454, hw), trib = 0.6 * bed(wq * 3.0, 10.0, g.sE + 455, 0.6 * hw);
            const double prof = std::max(main, trib) * smoothstep(-0.35, 0.05, gnoise3(p / 2.5, g.sE + 456));   // broken where the wind cut through
            o.dhSmooth += (8.0 + 32.0 * tu(101)) * prof * mask * featFade(hw, detailM);
            if (prof * mask > 0.5) { o.mark = 1; if (matVisible(hw * 0.6, detailM)) setMat(MAT_ROCK, 0.36); }   // the mark at every scale (the scanner's)
            break;
        }
        case TR_PINGOS: {
            // pingos on the cold flat lowlands (the Mackenzie delta's: Ibyuk, 49 m): hills of ice under a skin of ground 10-60 m high and 80-400 m
            // across on a quarter of the cells of a 1.2 km grid, steep-sided, the big ones split open at the top where the ice core shows (`pingoOf`)
            const double cold = kindHas(g.type, KT_FROZEN) ? 1.0 : smoothstep(-1.0, -6.0, c.T);
            const double mask = cold * land * smoothstep(0.3, 0.1, c.relief0) * prov(27, -0.15, 0.2) * (KINDS[g.type].ground == GR_FELISIAN ? smoothstep(900.0, 250.0, c.hS) : 1.0);
            if (mask < 0.01) break;
            if (featFade(0.16, detailM) <= 0 && !matVisible(0.1, detailM)) break;
            const PointGrid G = pingoGrid(g);
            double dh = 0; bool ice = false, on = false;
            visitFeatures(p, R, G.cellKm, G.density, G.seed, 0.4, [&](const FeatureHit& f) {
                const PointSize S = pingoOf(f);
                const double x = f.d / S.rKm;
                if (x >= 1) return;
                double prof = std::pow(1 - x * x, 1.6);
                if (S.heightM > 30 && x < 0.3) { prof -= 0.35 * (1 - x / 0.3) * (1 - x / 0.3); if (matVisible(0.6 * S.rKm, detailM)) ice = true; }   // split open
                dh += S.heightM * prof * featFade(2 * S.rKm, detailM);
                if (x < 0.8 && matVisible(1.6 * S.rKm, detailM)) on = true;
            });
            o.dhFine += dh * mask;
            if (mask > 0.5 && on) { if (ice) setMat(MAT_ICE, 0.7); o.mark = 1; }
            break;
        }
        case TR_MAARS: {
            // a volcanic field of maars (the Eifel's): round craters 0.4-2 km across blown by steam where magma met the groundwater, their floors
            // 50-200 m under the plain within steep inner walls, a low ring of tephra 8-28 m high round them and its apron over two radii, on 45%
            // of the cells of a 4 km grid in a province (`maarOf`); on a world it rains on the drainage fills them, crater lakes
            const double mask = land * prov(28, 0.12, 0.38) * smoothstep(0.45, 0.2, c.relief0);
            if (mask < 0.01) break;
            if (featFade(0.4, detailM) <= 0 && !matVisible(0.3, detailM)) break;
            const PointGrid G = maarGrid(g);
            double dh = 0; bool rim = false, in = false;
            visitFeatures(p, R, G.cellKm, G.density, G.seed, 0.75, [&](const FeatureHit& f) {
                const PointSize S = maarOf(f);
                const double x = f.d / S.rKm;
                if (x > 3.0) return;
                const double v = x < 1 ? -S.heightM + (S.heightM + S.rimM) * smoothstep(0.6, 1.0, x) : S.rimM * std::exp(-(x - 1) / 0.35);
                dh += v * featFade(2 * S.rKm, detailM);
                if (x < 1.25 && matVisible(0.4 * S.rKm, detailM)) { in = true; if (x > 0.6) rim = true; }
            });
            o.dhSmooth += dh * mask;
            if (mask > 0.5 && in) { if (rim) setMat(MAT_BASALT, 0.24); o.mark = 1; }
            break;
        }
        case TR_HOT_SPRINGS: {
            // terraced hot springs (Pamukkale's travertine, the Grand Prismatic's rings): mounds of white sinter stepping down in rimmed
            // terraces from the vent's blue pool at the top, the pools between the rims coloured by the mats of the life that lives in their heat
            // (bright near the vent, darker as the water cools), the runoff's mats streaking the ground round them; in clusters in a geothermal
            // province (`springGrid`, `springOf`; their steam: `hotSpringsNear`)
            const double mask = land * prov(41, 0.15, 0.42) * smoothstep(0.45, 0.2, c.relief0);
            if (mask < 0.01) break;
            if (featFade(0.03, detailM) <= 0 && !matVisible(0.03, detailM)) break;
            const PointGrid G = springGrid(g);
            double dh = 0; int mat = -1; double alb = 0;
            visitFeatures(p, R, G.cellKm, G.density, G.seed, 0.7, [&](const FeatureHit& f) {
                if (f.d > 1.3) return;
                const int n = springCount(f);
                for (int k = 0; k < n; k++) {
                    const HotSpring S = springOf(f, k, R);
                    const double x = length(p - S.c) / S.rKm;
                    if (x > 1.8) continue;
                    double h = 0; int m = -1; double a = 0;
                    if (x < 1) {
                        const double dome = S.heightM * (1 - std::pow(x, 1.5));
                        const double u = (1 - x) * S.terraces; const int tk = (int)u; const double frac = u - tk;   // the terrace (0 the outermost) and the place across it
                        double step = S.heightM * (tk + 0.4) / S.terraces + (frac < 0.16 ? 0.45 : -0.1);   // its pool's floor, its rim's crest at its outer edge
                        if (x < 0.14) step = S.heightM - 1.3;   // the vent's pool
                        else if (x < 0.24) step = S.heightM;    // the sinter round it
                        h = dome + (step - dome) * featFade(S.rKm / S.terraces, detailM);
                        if (matVisible(0.3 * S.rKm, detailM)) {   // white sinter terraces holding coloured pools: where the eye sees a terrace's rim, its rim
                            // white and its pool coloured; where it sees the terraces, every other one white; from further, the inner half white
                            const double tw = S.rKm / S.terraces;
                            const bool white = matVisible(0.004, detailM) ? frac < 0.16 : (matVisible(tw, detailM) ? (tk & 1) == 0 : x < 0.55);
                            if (x < 0.14) { m = MAT_BRINE; a = 0.16; }
                            else if (x < 0.24 || white) { m = MAT_SALT; a = 0.86; }
                            else { m = MAT_MATS; a = 0.68 - 0.42 * x; }
                        }
                    } else if (matVisible(0.02, detailM)) {   // the runoff's streaks of mats down the ground round it
                        const Vec3 rel = p - S.c;
                        const double streak = std::sin(std::atan2(dot(rel, cross(normalize(S.c), Vec3(0, 0, 1))), dot(rel, Vec3(0, 0, 1))) * 9.0 + 3.0 * fbm3(p / 0.05, g.sE + 493, 2));
                        if (streak > 0.45 && x < 1.1 + 0.3 * streak) { m = MAT_MATS; a = 0.2; }
                    }
                    if (h > dh) dh = h;
                    if (m >= 0 && (mat < 0 || x < 1)) { mat = m; alb = a; }
                }
            });
            o.dhFine += dh * mask;
            if (mask > 0.5 && mat >= 0) { setMat(mat, alb); o.mark = 1; }
            break;
        }
        case TR_METEORITES: {
            // the strewn fields of iron meteorites (Campo del Cielo; the blue ice of Antarctica's, where the ice brings them up and the wind
            // keeps them bare): an ellipse along the way the body broke up (`strewnOf`), its stones thicker downrange where the heaviest went,
            // where a few fresh pits 6-60 m across mark the biggest's falls; on the open ground, marked in every ring (the scanner finds them), the
            // stones themselves the surface's objects (`SurfaceView::forMeteorites`, by the field's density here)
            const double mask = land * smoothstep(0.5, 0.25, c.relief0);
            if (mask < 0.01) break;
            const PointGrid G = strewnGrid(g);
            double dens = 0, dh = 0; bool pitFloor = false;
            visitFeatures(p, R, G.cellKm, G.density, G.seed, 0.25, [&](const FeatureHit& f) {
                if (f.d > 14.0) return;
                const StrewnField S = strewnOf(f);
                const Vec3 rel = p - S.c;
                const double u = dot(rel, S.dir) / (0.5 * S.lenKm), v = dot(rel, S.perp) / (0.5 * S.widthKm), in = 1 - (u * u + v * v);
                if (in <= 0) return;
                dens = std::max(dens, smoothstep(0.0, 0.3, in) * (0.3 + 0.7 * smoothstep(-1.0, 1.0, u)));
                const int nPits = 3 + (int)(f.u[5] * 6);
                for (int k = 0; k < nPits; k++) {   // the pits downrange: their places along the way, a little either side, bigger furthest
                    const double fu = 0.25 + 0.7 * std::fmod(f.u[1] + 0.618034 * k, 1.0), fv = 0.6 * (std::fmod(f.u[2] + 0.414214 * k, 1.0) - 0.5);
                    const Vec3 pc = S.c + S.dir * (fu * 0.5 * S.lenKm) + S.perp * (fv * 0.5 * S.widthKm);
                    const double rKm = (0.003 + 0.027 * std::fmod(f.u[4] + 0.31 * k, 1.0)) * (0.5 + fu), d = length(p - pc) / rKm;
                    if (d > 2.0) continue;
                    const double depthM = 0.22 * rKm * 1000.0;
                    dh += (d < 1 ? -depthM * (1 - d * d) + 0.12 * depthM : 0.12 * depthM * std::exp(-(d - 1) / 0.3)) * featFade(2 * rKm, detailM);
                    if (d < 0.35 && matVisible(rKm, detailM)) pitFloor = true;
                }
            });
            if (dens > 0) { o.tint = std::max(o.tint, dens * mask); if (dens * mask > 0.1) o.mark = 1; }
            o.dhFine += dh * mask;
            if (pitFloor && mask > 0.5) setMat(MAT_METAL, 0.16);   // the iron at a pit's floor
            break;
        }
        case TR_STRATA: {
            // rainbow strata (Zhangye Danxia, the Painted Hills): the sediments a wet age laid, oxidised layer by layer, bared on the hills'
            // slopes of a province: bands 4-12 m thick, each the rock's own colour or the accent's (a red, an orange or a green by the world) at a
            // brightness of its own (the two hues and their shades the banks hold), the layers' edges wavy; where a ring cannot hold the bands,
            // bands six times as thick (the far hills striped broadly). The material chain's height and slope, so every ring agrees
            const double mask = land * prov(46, 0.18, 0.42) * smoothstep(0.1, 0.22, c.slope) * smoothstep(0.6, 0.3, c.moist);
            if (mask < 0.5) break;
            const double thick = 4.0 + 8.0 * tu(62);
            const double fine = matVisible(0.0006 * thick, detailM) ? 1.0 : 6.0;
            const double hh = c.hM + 3.0 * fbm3(p / 1.5, g.sE + 498, 2);   // the layers dip and wave a little
            const int64_t layer = (int64_t)std::floor(hh / (thick * fine));
            const uint64_t lh = mix64((uint64_t)layer * 0x9E3779B97F4A7C15ULL ^ g.seed ^ 0x57A7AULL);
            const double alb = 0.3 + 0.5 * unitFromHash(mix64(lh + 1));
            if (unitFromHash(lh) < 0.5) { o.tint = 1; setMat(familyRep(g.type, FAM_ROCK), alb); } else setMat(familyRep(g.type, FAM_ROCK), alb);
            o.mark = 1;
            break;
        }
        case TR_PETRIFIED: {
            // a petrified forest (Arizona's Chinle beds): where a forest stood when the world was wet, its trunks buried, turned to stone
            // and bared again by the wind, lying in the open dry ground of a province; their density here for the surface's stone logs
            // (`SurfaceView::forLogs`), the ground pale and banded grey and lilac where they lie thickest
            const double mask = land * prov(45, 0.2, 0.45) * smoothstep(0.5, 0.25, c.relief0) * smoothstep(0.55, 0.3, c.moist);
            if (mask < 0.01) break;
            o.life = std::max(o.life, mask * (0.55 + 0.45 * fbm3(p / 2.5, g.sE + 497, 2)));
            if (mask > 0.5) o.mark = mask;
            break;
        }
        case TR_GYPSUM: {
            // a field of white gypsum dunes (White Sands: the gypsum of a dried lake's beds blown downwind): transverse crests across the wind
            // 100-250 m apart and 8-18 m high, gentle on the windward side and steep on the lee, their tops bending into barchans, the flats between
            // them white too; in a province of the dry low country, white in every ring and on the maps (the field is kilometres across, B-404)
            const double mask = land * prov(43, 0.18, 0.4) * smoothstep(0.46, 0.26, c.moist) * smoothstep(0.4, 0.18, c.relief0) * smoothstep(1400.0, 600.0, c.hS);
            if (mask < 0.01) break;
            if (mask > 0.5) { setMat(MAT_SALT, 0.84); o.mark = mask; }   // gypsum is an evaporite: the salts' white in every view (the sand family's one colour is the world's dust on some)
            const double wl = 0.1 + 0.15 * tu(60), amp = 8.0 + 10.0 * tu(61);
            const double f = lodFade(wl, detailM);
            if (f <= 0) break;
            const Vec3 dd = normalize(Vec3(std::cos(g.windAngle), std::sin(g.windAngle), 0.15 * std::sin(g.windAngle * 2)));
            const Vec3 dp = normalize(cross(dd, unit));
            const double along = dot(p, dd) / wl, across = dot(p, dp) / wl;
            const double bend = 1.2 * fbm3(p / 0.6, g.sE + 494, 2) + 0.35 * std::sin(across * 0.9);   // the crests' bends: barchans' horns downwind
            double ph = along + bend; ph -= std::floor(ph);
            const double prof = ph < 0.8 ? smoothstep(0.0, 0.8, ph) : 1.0 - smoothstep(0.8, 1.0, ph);   // the windward slope, the slip face
            o.dhFine += amp * prof * mask * f * (0.75 + 0.25 * fbm3(p / 0.3, g.sE + 495, 2));
            break;
        }
        case TR_GLASSED: {
            // the blast that made the neutron star melted the surface; the melt pooled on the flats and lows and froze into
            // sheets of dark glass (the fine relief gone, ripples of a metre frozen in), the slopes and highs are scorched rock.
            // The mask reads the land, a province of about half the world and the material chain's slope and relief, all the same
            // in every ring and on the map (B-404), so a sheet is a sheet at every distance; the ripples alone fade by the ring
            double mask = land * prov(19, -0.45, -0.1) * smoothstep(0.25, 0.08, c.slope) * smoothstep(0.5, 0.25, c.relief0);
            if (mask < 0.01) break;
            o.flatten = std::max(o.flatten, 0.85 * mask);   // the fine relief melted into the smooth ground
            o.dhFine += mask * (0.5 * fbm3(p / 0.05, g.sE + 291, 2) * lodFade(0.05, detailM) + 1.5 * fbm3(p / 0.25, g.sE + 292, 2) * lodFade(0.25, detailM));   // frozen ripples and swells
            if (mask > 0.5) { setMat(MAT_GLASS, 0.42); o.mark = mask > 0.8 ? 1 : 0; }   // pale green glass (trinitite, not obsidian: a dark sheet under a dim star was a black screen)
            break;
        }
        default: break;
        }
        if (o.mark < 0) o.mark = mark0; else o.markTrait = t;
    }
}
