// E-07: the kinds' ground functions, continued (planet_grounds.cpp has the first): the chemotroph to the dark ocean, the
// drifters' dark ground (`darkGround`), the giants
#include "planet_internal.h"

namespace {

// the chemotroph's spring fields: the rifts' floors and scattered fields on the plains (tens of kilometres, so a pool is never cut by
// their edge), a spring a 0.5 km cell at most; `springVents` visits the same features
inline double springField(const GroundCtx& c, double rift) { return std::max(smoothstep(0.5, 0.8, rift), smoothstep(0.3, 0.5, fbm3(c.p / (c.R * 0.06), c.g.sG + 31, 3))); }   // unfaded: the map's and the ground's alike
}   // namespace

// P-03: life on the ground's heat: the tectonic crust (rifts with their floors, fissures, cones, ash plains) cold under a dim star and
// frosted, its fissures steaming instead of running with lava; along the rifts' floors and in fields on the plains, hot springs: a
// pool of blue water 12-30 m across on a low mound of sinter, the sinter's terraces round it, pale mats spreading three to six times as
// far as the pool with a lobed edge, the frost kept off the warm ground round them (`springVents`: the surface's steam and spores)
void groundChemotroph(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const Tectonics& tec = c.tec;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF, absLatDeg = c.absLatDeg;
    double base = c.fb(R * 0.4, g.sA, 5);
    double h = base * g.mountainAmp * 0.6 + tec.uplift * 700.0;
    double zone = clampd(smoothstep(0.1, 0.6, base) + 0.7 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the skeleton
    const double hS = h - (rf.h - rf.hCoarse);
    const Worley3 rw = worley3(p / (R * 0.3), g.sF + 5);
    const double dRift = (rw.f2 - rw.f1) * R * 0.3 * 500.0, rift = smoothstep(11000.0, 7000.0, dRift);
    h += (hS - 800.0 - h) * rift;
    double fissure = 0;
    if (rift > 0.5 && (scaleVisible(5.0, detailM) || matVisible(0.3, detailM))) {   // the fissures: trenches 30 m deep and 400 m wide on 5 km cells, their rims mats
        const Worley3 fw = worley3(p / 5.0, g.sF + 6);
        const double dm = (fw.f2 - fw.f1) * 2500.0;
        h -= 30.0 * smoothstep(220.0, 40.0, dm) * featFade(0.44, detailM);
        if (matVisible(0.3, detailM)) fissure = smoothstep(320.0, 120.0, dm) * rift;
    }
    bool caldera = false;
    if (scaleVisible(R * 0.2, detailM)) h += volcanoField(p, R, R * 0.2, g.volcanoes * 0.5, g.sD, 2200.0, caldera) * lodFade(R * 0.2, detailM);
    if (scaleVisible(20.0, detailM)) h += volcanoField(p, R, 20.0, g.volcanoes * 0.7, g.sD + 3, 500.0, caldera);
    c.craterStack(h);
    double pool = 0, sinter = 0, mats = 0;   // the springs, by their own sizes (unfaded: every ring that can draw one draws the same)
    const double field = springField(c, rift);
    if (field > 0.05 && matVisible(0.012, detailM)) {
        visitFeatures(p, R, SPRING_CELL, SPRING_DENS, g.sF + 33, 0.45, [&](const FeatureHit& f) {
            const double rp = 0.012 + 0.018 * f.u[3], x = f.d / rp;   // the pool's radius, km
            const double reach = 3.0 + 3.0 * f.u[4];   // the mats' reach in pool radii, lobed
            const double lobe = 1.0 + 0.3 * gnoise3(p / 0.03, g.sG + 34 + (uint64_t)(f.u[5] * 8));
            if (x >= reach * lobe * 1.3) return;
            pool = std::max(pool, field * smoothstep(1.05, 0.9, x));
            sinter = std::max(sinter, field * smoothstep(2.4, 2.0, x));
            mats = std::max(mats, field * smoothstep(reach * lobe, reach * lobe * 0.8, x));
            h += (2.2 * smoothstep(2.6, 1.0, x) - 1.0 * smoothstep(1.0, 0.8, x)) * field * featFade(2 * rp, detailM);   // the mound, the pool a metre under its rim
        });
    }
    s.height = h;
    const double ash = c.fb(R * 0.1, g.sC + 8, 3);
    // the fields' cores are geyser basins, sinter and mats between the springs in a patchwork of a kilometre (what the globe shows of them);
    // frost in fields of a hundred kilometres on the cold ground, ragged at their edges, more on the heights and toward the poles, none in a
    // basin (unfaded fields)
    const double basin = smoothstep(0.7, 0.9, field), patch = fbm3(p / 1.2, g.sG + 36, 3);
    const double frost = fbm3(p / 120.0, g.sG + 35, 2) + (matVisible(3.0, detailM) ? 0.15 * fbm3(p / 3.0, g.sG + 37, 2) : 0.0) + 0.4 * smoothstep(400.0, 2000.0, hS) + 0.3 * smoothstep(30.0, 70.0, absLatDeg) - basin;   // the edges' raggedness where a ring draws it (the map's texels are wider)
    if (pool > 0.5) { s.material = MAT_BRINE; s.albedo = 0.30; }
    else if (sinter > 0.5 || (basin > 0.5 && patch > 0.12)) { s.material = MAT_SALT; s.albedo = 0.66; }
    else if (mats > 0.5 || fissure > 0.5 || (basin > 0.5 && patch > -0.2)) { s.material = MAT_MATS; s.albedo = 0.52; }
    else if (frost > 0.45) { s.material = MAT_SNOW; s.albedo = 0.70; }
    else if (rel < 0.45 && ash > 0.05 && rift < 0.5) { s.material = MAT_DUST; s.albedo = 0.30 + 0.08 * ash; }
    else { s.material = MAT_BASALT; s.albedo = 0.16 + 0.06 * (0.5 + 0.5 * base); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope + rift * 0.3, 0, 1);
}

// P-05: Pluto's ground (New Horizons, 2015). A crust of water ice (its rock: cratered, the high ground frosted with methane, dark red lands of
// tholin along the low latitudes, blades of methane ice hundreds of metres tall on the high ground there); in the basins under the plains' level
// (`seaLevel`, metres) a plain of nitrogen ice churning in cells 15-40 km across, domed 50 m at their middles between dark troughs 70 m deep, pits
// in fields where it sublimes, blocks of water ice afloat in it (hills a kilometre and more high along its shores, smaller ones further out);
// nitrogen glaciers down the valleys of the shores into it
void groundNitrogen(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright;
    const double R = c.R, detailM = c.detailM, absLatDeg = c.absLatDeg, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * g.mountainAmp;
    {   // the great plain's basin (Sputnik Planitia's: an old impact basin 15-25 degrees round at a hashed place, its rim ragged), 1.8 km deep
        const Vec3 bc = nitrogenBasinOf(g);
        const double ang = std::acos(clampd(dot(c.unit, bc), -1, 1)) / DEG, rad = (15.0 + 10.0 * unitFromHash(mix64(g.seed ^ 0x51A0ULL))) * (1 + 0.15 * gnoise3(c.unit * 4.0, g.sF + 40));
        h -= 1800.0 * smoothstep(rad, rad * 0.72, ang);
    }
    const double zone = smoothstep(0.1, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    double hRel, hS; ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf), &hRel, &hS);
    const double L = g.seaLevel, depth = L - hRel;
    const double plain = smoothstep(-25.0, 25.0, depth);   // under the level: the nitrogen
    double cr = 0; c.craterStack(cr);
    h += cr * (1 - plain);   // the plain is young: no crater
    double dome = 0, trough = 0, block = 0, pit = 0;
    if (plain > 0) {
        double hp = L;
        const bool cv = scaleVisible(26.0, detailM), tm = matVisible(1.0, detailM);
        if (cv || tm) {   // the cells: 26 km on average, their bounds the troughs
            const Worley3 w = worley3(p / 26.0, g.sF + 41);
            const double dKm = (w.f2 - w.f1) * 13.0;   // km from the bound
            trough = smoothstep(0.9, 0.12, dKm);
            dome = smoothstep(0.5, 9.0, dKm);
            if (cv) hp += 50.0 * dome * lodFade(26.0, detailM) - 70.0 * trough * featFade(1.8, detailM);
        }
        const double shore = smoothstep(450.0, 60.0, depth);   // the blocks crowd its shores
        if (shore > 0 && scaleVisible(7.0, detailM)) block += domeField(p, R, 7.0, 0.55, g.sD + 41, 0.38, 1300.0, 0.7) * shore * lodFade(7.0, detailM);
        if (scaleVisible(18.0, detailM)) block += domeField(p, R, 18.0, 0.10, g.sD + 42, 0.14, 260.0, 0.6) * lodFade(18.0, detailM);   // a few drifting further out
        const double pitField = smoothstep(0.15, 0.35, fbm3(p / (R * 0.12), g.sG + 44, 2));
        if (pitField > 0 && (scaleVisible(0.6, detailM) || matVisible(0.15, detailM))) {   // sublimation pits, 150-350 m across
            const Worley3 pw = worley3(p / 0.6, g.sF + 43);
            if (unitFromHash(pw.id1) < 0.6) pit = pitField * smoothstep(0.42, 0.22, pw.f1);
            hp -= 22.0 * pit * featFade(0.3, detailM);
        }
        hp += block;
        h += (hp - h) * plain;
        hS += (L + block - hS) * plain;
        if (plain > 0.5) c.lo.mat = -1;   // the plain buries the traits' walls and sheets under it
    }
    // the glaciers: the valleys of the shores (under their smooth ground) filled flat with nitrogen ice a few hundred metres up from the plain
    const double shoreUp = (1 - plain) * smoothstep(500.0, 80.0, hRel - L), fine = h - hS;
    const double glacier = shoreUp * smoothstep(-6.0, -25.0, fine);
    if (glacier > 0) h += (hS - 8.0 - h) * glacier;
    // the blades: ridges 3-5 km apart drawn out north to south on the high ground of the low latitudes
    double bladeCrest = 0;
    const double bladeZone = (1 - plain) * smoothstep(L + 500.0, L + 1000.0, hRel) * smoothstep(38.0, 22.0, absLatDeg) * smoothstep(-0.1, 0.25, fbm3(p / (R * 0.15), g.sG + 45, 2));
    if (bladeZone > 0 && (scaleVisible(4.0, detailM) || matVisible(0.4, detailM))) {
        const double r = ridged3(Vec3(p.x, p.y, p.z * 0.2) / 4.0, g.sF + 44, 3);   // stretched along the polar axis: blades north to south
        const double crest = smoothstep(0.55, 0.85, r);
        h += 380.0 * bladeZone * crest * featFade(1.2, detailM);
        if (matVisible(0.4, detailM)) bladeCrest = bladeZone * crest;
    }
    s.height = h;
    // the uplands' colours: tholin by an unfaded field of the low latitudes, methane frost on the heights and toward the poles
    const double tholin = fbm3(p / (R * 0.22), g.sG + 46, 3) + (matVisible(12.0, detailM) ? 0.3 * fbm3(p / 25.0, g.sG + 47, 2) : 0.0) + 0.75 * smoothstep(32.0, 8.0, absLatDeg) - 0.45;
    const double frost = (hRel - L - 700.0) / 500.0 + smoothstep(40.0, 70.0, absLatDeg) + 0.4 * fbm3(p / (R * 0.06), g.sG + 48, 2) - (tholin > 0 ? 0.6 : 0.0);
    if (plain > 0.5) {
        if (block > 120.0) { s.material = MAT_ROCK; s.albedo = 0.62; }   // a block of water ice afloat
        else { s.material = MAT_ICE; s.albedo = 0.80 + 0.05 * dome - 0.28 * trough - 0.1 * pit; }   // the troughs dark with the tholin they gather
    } else if (glacier > 0.5) { s.material = MAT_ICE; s.albedo = 0.78; }
    else if (bladeCrest > 0.4 || frost > 0.5) { s.material = MAT_SNOW; s.albedo = 0.86 + 0.04 * base; }
    else if (tholin > 0) { s.material = MAT_DUST; s.albedo = 0.14 + 0.04 * base + 0.04 * clampd(rimBright, 0, 1); }
    else { s.material = MAT_ROCK; s.albedo = 0.50 + 0.10 * base + 0.08 * clampd(rimBright, 0, 1); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1) * (1 - 0.7 * plain);
}

// P-05: Triton's ground (Voyager 2, 1989): a young crust of ice with few craters. Over most of it the cantaloupe terrain (dimples 25-35 km
// across between low ridges, smaller ones in them, long double ridges crossing it); smooth plains where its ice flooded and froze, set lower
// behind their scarps; south of the cap's edge (`iceCapLat` and a wander) a cap of pink nitrogen frost, the dark streaks of its plumes laid
// downwind of their vents (`plumeVents`: 45-110 km long, widening to 19 km)
void groundCryovolcanic(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.5, g.sA, 4);
    double h = base * g.mountainAmp;
    const double zone = smoothstep(0.2, 0.7, base), rel = zone * massif * 0.3, hf = hillsF * 0.4;
    double hRel, hS; ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf), &hRel, &hS);
    const double cant = smoothstep(-0.3, -0.1, fbm3(p / (R * 0.35), g.sC + 3, 3));   // the cantaloupe terrain (an unfaded field: the plains' edge on every ring)
    if (cant > 0) {
        auto dimples = [&](double cellKm, double deepM, double ridgeM, uint64_t seed) {
            if (!scaleVisible(cellKm, detailM)) return;
            const Vec3 q = p / cellKm, warp(gnoise3(q * 1.3, seed + 1), gnoise3(q * 1.3, seed + 2), gnoise3(q * 1.3, seed + 3));   // rounded, not straight-edged
            const Worley3 w = worley3(q + warp * 0.3, seed);
            h += cant * (-deepM * smoothstep(0.55, 0.0, w.f1) + ridgeM * smoothstep(0.1, 0.0, w.f2 - w.f1)) * lodFade(cellKm, detailM);
        };
        dimples(30.0, 140.0, 50.0, g.sF + 50); dimples(7.0, 35.0, 12.0, g.sF + 53);
        const double rf2 = featFade(1.2, detailM);   // the double ridges: two crests 600 m from the line, a trough between
        if (rf2 > 0) {
            const Worley3 w = worley3(p / (R * 0.3), g.sF + 54);
            const double dm = (w.f2 - w.f1) * R * 0.3 * 500.0;
            h += cant * rf2 * (90.0 * std::exp(-std::pow((dm - 600.0) / 260.0, 2.0)) - 40.0 * std::exp(-std::pow(dm / 300.0, 2.0)));
        }
    }
    h += (hS - 160.0 - h) * 0.85 * (1 - cant);   // the plains: flooded flat and frozen, set lower behind their scarps
    c.craterStack(h);
    s.height = h;
    // the cap and its streaks
    const double latDeg = c.lat / DEG, edge = cryoCapEdge(g, unit);
    const bool cap = latDeg < -edge;
    double streak = 0;
    if (cap || latDeg < -edge + 2.0) {
        visitFeatures(p, R, PLUME_CELL, plumeDensity(g), g.sF + 52, 1.0, [&](const FeatureHit& f) {
            const double len = plumeStreakKm(f.u[3]);
            if (f.d > len + 20) return;
            const Vec3 n = normalize(f.c), w = prevailingWind(g, n), across = cross(n, w), d = p - f.c;
            const double along = dot(d, w), off = std::fabs(dot(d, across));
            if (along < 0 || along > len) return;
            const double half = 0.5 + 0.08 * along;   // km: narrow at the vent, 19 km wide at the end of the longest
            if (off > half || !matVisible(2 * half, detailM)) return;
            if (std::asin(clampd(n.z, -1, 1)) / DEG > -cryoCapEdge(g, n)) return;   // a vent on the cap (the dearest test last)
            streak = std::max(streak, std::pow(1 - along / len, 0.6) * (1 - off / half));
        });
    }
    if (streak > 0.3) { s.material = MAT_DUST; s.albedo = 0.20 + 0.15 * (1 - streak); }
    else if (cap) { s.material = MAT_SNOW; s.albedo = 0.86 + 0.04 * base; }
    else if (cant < 0.5) { s.material = MAT_ROCK; s.albedo = 0.62 + 0.06 * base; }
    else { s.material = MAT_ICE; s.albedo = 0.56 + 0.08 * base + 0.08 * clampd(rimBright, 0, 1); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-05: Mars' south polar cap on a colder world: the thin air's ground (canyons, shields and their dark plains, dunes, the old dry valleys)
// under caps of dry ice to `iceCapLat` (carbon dioxide over water ice, pitted like Swiss cheese: flat-floored pits 150-450 m across and 8 m
// deep); round each cap a band 9 degrees wide of layered dust where the gas under the seasonal ice carved the spiders (radial channels a
// metre or two deep converging on a centre, 150-400 m across: `springJets`); over that band in the cold half of the year the seasonal frost
// (it reaches 12 degrees past the cap's edge in winter and is gone by midsummer), the dark fans the spring's jets laid on it downwind
void groundDryIce(GroundCtx& c) {
    groundThinAir(c);
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, absLatDeg = c.absLatDeg;
    const double edge = dryCapEdge(g, unit);
    if (absLatDeg > edge) {   // the residual cap (`groundThinAir` made it ice): the pits
        if (scaleVisible(0.6, detailM) || matVisible(0.15, detailM)) {
            const Worley3 w = worley3(p / 0.6, g.sF + 60);
            if (unitFromHash(w.id1) < 0.55) {
                const double pit = smoothstep(0.40, 0.30, w.f1);   // a flat floor inside steep walls
                s.height -= 8.0 * pit * featFade(0.3, detailM);
                if (matVisible(0.15, detailM) && pit > 0.5) s.albedo = 0.62;
            }
        }
        return;
    }
    if (absLatDeg < edge - 12.0) return;
    const bool frost = absLatDeg > edge - dryFrostDeg(g, c.lat >= 0);
    const bool band = absLatDeg > edge - 9.0;
    double channel = 0, fan = 0;
    if (band && (scaleVisible(0.05, detailM) || matVisible(0.02, detailM))) {
        visitFeatures(p, R, SPIDER_CELL, SPIDER_DENS, g.sF + 61, 1.0, [&](const FeatureHit& f) {
            const double reach = 0.15 + 0.25 * f.u[3], x = f.d / reach;
            if (x >= 1 && (!frost || f.d > 0.25)) return;   // neither its channels nor its fan reach here
            const Vec3 n = normalize(f.c);
            const double la = std::fabs(std::asin(clampd(n.z, -1, 1))) / DEG, e2 = dryCapEdge(g, n);
            if (la > e2 || la < e2 - 9.0) return;
            if (x < 1) {   // the spider: 6-11 channels with a branch each, deepest at the middle
                const Vec3 e1 = normalize(cross(n, std::fabs(n.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2v = cross(n, e1);
                const Vec3 d = normalize(p - f.c);
                const double ang = std::atan2(dot(d, e2v), dot(d, e1));
                const int arms = 6 + (int)(f.u[1] * 6);
                const double wob = 0.5 * gnoise3(p / 0.05, g.sG + 62);
                const double arm = std::pow(0.5 + 0.5 * std::cos(arms * ang + f.u[0] * TAU + wob), 26.0), twig = std::pow(0.5 + 0.5 * std::cos(2 * arms * ang + f.u[2] * TAU + wob), 40.0) * smoothstep(0.3, 0.6, x);
                const double ch = std::max(arm, twig) * (1 - x) + smoothstep(0.12, 0.0, x);
                channel = std::max(channel, ch);
            }
            if (frost) {   // its fan: dark dust laid downwind on the frost, 60-240 m long
                const Vec3 w = prevailingWind(g, n), across = cross(n, w), dd = p - f.c;
                const double len = (0.06 + 0.18 * f.u[4]), along = dot(dd, w), off = std::fabs(dot(dd, across));
                const double half = 0.012 + 0.25 * along;
                if (along > -0.01 && along < len && off < half) fan = std::max(fan, (1 - along / len) * (1 - off / half));
            }
        });
        s.height -= 1.6 * clampd(channel, 0, 1) * featFade(0.03, detailM);
        if (!matVisible(0.02, detailM)) { channel = 0; fan = 0; }
    }
    if (frost) {
        if (fan > 0.3) { s.material = MAT_BASALT; s.albedo = 0.18 + 0.2 * (1 - fan); }
        else { s.material = MAT_SNOW; s.albedo = 0.72 - 0.08 * clampd(channel, 0, 1); }
    } else if (band) {
        if (channel > 0.45) { s.material = MAT_BASALT; s.albedo = 0.26; }
        else if (s.material != MAT_ROCK) { s.material = MAT_DUST; s.albedo = 0.52 + 0.06 * fbm3(p / 2.0, g.sG + 63, 2); }   // the layered deposits
    }
}

// P-05: a rogue world a star caught: the thin air's ground of the world it was (its mountains, craters and shields, its old river courses
// dry: the drainage), under its air fallen as snow: nitrogen and carbon monoxide snow `snowLine` metres deep on the open ground, the hollows
// of the fine relief filled near flat (the craters, the valleys' floors), drifted into low dunes, bare rock on the steep ground that sheds it;
// domes of new ice where the warmth of its interior pushed up through (`volcanoes`)
void groundWanderer(GroundCtx& c) {
    groundThinAir(c);
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM;
    double h = s.height;
    const double fill = clampd(c.hSmooth - h, 0, 600);
    const double slope = c.slopeM;
    const double shed = smoothstep(0.32, 0.55, slope);
    // the fallen air lies on the low ground (it froze out last there, and the drifts slid down to it): the plains under the old world's middle
    // height deep in it, thinning to nothing 900 m over it (an unfaded field, its edge ragged), the highlands' old rock bare
    const double lowness = smoothstep(900.0, -150.0, c.hSmooth + 350.0 * fbm3(p / (R * 0.08), g.sG + 90, 3) + (matVisible(4.0, detailM) ? 80.0 * fbm3(p / 6.0, g.sG + 91, 2) : 0.0));
    const double depthM = (g.snowLine + 0.85 * fill) * lowness * (1 - shed);
    h += depthM;
    if (depthM > 1.0) h += duneField(p, g, c.lat, 3.0, detailM) * smoothstep(1.0, 8.0, depthM) * smoothstep(0.2, 0.05, slope);   // drifts of snow
    double dome = 0;
    if (scaleVisible(R * 0.08, detailM)) dome += domeField(p, R, R * 0.08, g.volcanoes, g.sD + 31, 0.3, 450.0, 0.85) * lodFade(R * 0.08, detailM);
    if (scaleVisible(6.0, detailM)) dome += domeField(p, R, 6.0, 0.5 * g.volcanoes, g.sD + 32, 0.3, 90.0, 0.8);
    h += dome;
    s.height = h;
    if (dome > 40.0) { s.material = MAT_ICE; s.albedo = 0.78; }
    else if (depthM > 1.5) { s.material = MAT_SNOW; s.albedo = 0.84 + 0.04 * clampd(fill / 50.0, 0, 1); }
    else if (shed > 0.5 || c.slopeM > 0.25) { s.material = MAT_ROCK; s.albedo = 0.24; }
    else { s.material = MAT_DUST; s.albedo = 0.18 + 0.2 * lowness; }   // frosted where the snow begins
}

// P-05: Ceres' ground (Dawn, 2015-18): a dwarf's cratered ground, dark with carbon; on the floors of its young craters the faculae, salt
// dried out of brine that welled up from below (a bright dome at the middle, spots scattered over the rest of the floor); and one lone
// mountain of ice (Ahuna Mons: 4 km high, a top 10 km across on flanks steep to its foot at 12 km, the flanks streaked with salt down
// their fall lines) at a place hashed from the world's seed
void groundBrine(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.5, g.sA, 5);
    double h = base * g.mountainAmp;
    const double zone = smoothstep(0.1, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));
    double cr = 0; c.craterStack(cr);
    h += cr;
    // the lone mountain
    const Vec3 mc = loneMountainOf(g) * R;
    const double dKm = length(p - mc);
    double mtn = 0, streakM = 0;
    if (dKm < 14.0) {
        mtn = smoothstep(12.0, 5.0, dKm);
        const double top = 3600.0 + 400.0 * unitFromHash(mix64(g.seed ^ 0xA4A2ULL));
        h += top * (mtn * mtn * (3 - 2 * mtn)) + 25.0 * c.fb(1.5, g.sG + 70, 3) * smoothstep(6.0, 4.0, dKm);   // a cracked top
        if (dKm > 4.5 && dKm < 12.5 && matVisible(0.3, detailM)) {   // the bright streaks down the flanks
            const Vec3 n = normalize(mc), e1 = normalize(cross(n, std::fabs(n.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(n, e1);
            const Vec3 d = normalize(p - mc);
            const double ang = std::atan2(dot(d, e2), dot(d, e1));
            streakM = std::pow(0.5 + 0.5 * std::cos(ang * 23.0 + 2.0 * std::sin(ang * 5.0)), 6.0) * smoothstep(12.5, 9.0, dKm);
        }
    }
    s.height = h;
    const double young = clampd(cm.young, 0, 1);
    const double spots = young > 0.05 && matVisible(0.4, detailM) ? fbm3(p / 1.2, g.sG + 71, 3) : -1.0;
    if (young > 0.45 || (young > 0.05 && spots > 0.3 - 0.3 * young)) { s.material = MAT_SALT; s.albedo = 0.86 + 0.1 * young; }   // the faculae
    else if (mtn > 0.05) {
        if (streakM > 0.45 || dKm < 5.0) { s.material = MAT_SALT; s.albedo = 0.70; }
        else { s.material = MAT_ICE; s.albedo = 0.36; }
    } else {
        s.material = rf.slopeM > 0.35 ? MAT_ROCK : MAT_DUST;
        s.albedo = 0.14 + 0.04 * base + 0.10 * clampd(cm.rimBright, 0, 1) - 0.03 * clampd(cm.floorDark, 0, 1) + 0.25 * clampd(cm.rays, 0, 1);
    }
    s.relief = clampd(std::fabs(cr) / 400.0 + rel + 0.5 * mtn, 0, 1);
}

// P-05: Eris's and Makemake's ground: smooth plains under a fresh coat of methane frost (the brightest ground there is), low rounded relief,
// few craters, fields of sublimation pits (round pits 100-250 m across and 18 m deep), a glaze of nitrogen ice in the hollows, and red
// streaks of tholin drawn out along the latitudes where the frost lies thin
void groundFrost(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.45, g.sA, 4);
    double h = base * g.mountainAmp;
    const double zone = smoothstep(0.2, 0.7, base), rel = zone * massif * 0.5, hf = hillsF * 0.6;
    double hRel, hS; ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf), &hRel, &hS);
    c.craterStack(h);
    double pit = 0;
    const double pitField = smoothstep(0.05, 0.3, fbm3(p / (R * 0.1), g.sG + 80, 2));
    if (pitField > 0 && (scaleVisible(0.4, detailM) || matVisible(0.1, detailM))) {
        const Worley3 w = worley3(p / 0.4, g.sF + 80);
        if (unitFromHash(w.id1) < 0.55) pit = pitField * smoothstep(0.38, 0.2, w.f1);
        h -= 18.0 * pit * featFade(0.2, detailM);
    }
    s.height = h;
    const Vec3 sq(p.x, p.y, p.z * 10.0);   // drawn out east and west
    const double streak = fbm3(sq / (R * 0.15), g.sG + 81, 3) + (matVisible(5.0, detailM) ? 0.25 * fbm3(sq / (R * 0.03), g.sG + 82, 2) : 0.0);
    if (streak > 0.2) { s.material = MAT_DUST; s.albedo = 0.52 + 0.2 * (0.5 - streak); }   // tholin
    else if (pit > 0.5 && matVisible(0.1, detailM)) { s.material = MAT_ROCK; s.albedo = 0.55; }
    else if (h - hS < -12.0) { s.material = MAT_ICE; s.albedo = 0.86; }   // the glaze in the hollows
    else { s.material = MAT_SNOW; s.albedo = 0.94 + 0.03 * base; }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-06: Ganymede's ground (Galileo, 1996-2000): its oldest crust in great dark polygons (the regiones: dark with dust, heavily cratered, the
// rims of their craters bright with ice, the furrows of ancient impacts in arcs across them), and between them lanes of bright ice (the
// sulci) where the crust pulled apart: each lane a few hundred metres lower than the polygons it parts, filled with tilted blocks whose
// ridges run along it (3-8 km apart, 300-700 m high), a finer set over them; within a lane swaths of grooves at angles of their own cut
// across the rest; fewer craters on the lanes; frost poleward of `iceCapLat` over both
void groundGrooved(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.45, g.sA, 4);
    double h = base * g.mountainAmp;
    const double zone = smoothstep(0.1, 0.6, base), rel = zone * massif * 0.6, hf = std::max(hillsF, 0.5 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));
    // the polygons and the lanes along their boundaries (each boundary its own width, drop, groove spacing and height: both sides alike)
    const double cell = R * 0.55;
    const Vec3 q = p / cell, warp(gnoise3(q * 2.3, g.sF + 81), gnoise3(q * 2.3, g.sF + 82), gnoise3(q * 2.3, g.sF + 83));
    const Worley3 w = worley3(q + warp * 0.06, g.sF + 80);
    const double dKm = (w.f2 - w.f1) * cell * 0.5;   // km from the polygons' boundary
    const uint64_t e = mix64(w.id1 ^ w.id2);
    const double half = cell * (0.07 + 0.11 * unitFromHash(e));
    const double lane = smoothstep(half, half - 6.0, dKm);   // 1 in the lane, its edge a scarp a few kilometres wide
    if (lane > 0) {
        h -= lane * (200.0 + 300.0 * unitFromHash(mix64(e + 1)));
        const double sp = 3.0 + 5.0 * unitFromHash(mix64(e + 2)), amp = 300.0 + 400.0 * unitFromHash(mix64(e + 3));
        const double fg = featFade(sp * 0.5, detailM);
        if (fg > 0) {
            double coord = dKm, spU = sp, ampU = amp;
            {   // a swath of grooves at its own angle (a third of the cells of a grid of 0.12 of the polygons)
                const Worley3 w2 = worley3(p / (cell * 0.12) + warp * 0.3, g.sF + 85);
                if (unitFromHash(mix64(w2.id1 + 7)) < 0.35) {
                    const Vec3 d3 = normalize(Vec3(unitFromHash(mix64(w2.id1 + 8)) - 0.5, unitFromHash(mix64(w2.id1 + 9)) - 0.5, unitFromHash(mix64(w2.id1 + 10)) - 0.5));
                    coord = dot(p, d3); spU = sp * (0.6 + 0.5 * unitFromHash(mix64(w2.id1 + 11))); ampU = amp * 0.7;
                }
            }
            const double rr = coord / spU + unitFromHash(mix64(e + 4)), fr = rr - std::floor(rr);
            const double block = fr < 0.2 ? fr / 0.2 : (1 - fr) / 0.8;   // a tilted block: its scarp, then its long back
            const double vary = 0.6 + 0.4 * gnoise3(p / (sp * 6.0), g.sF + 84);
            h += lane * fg * ampU * vary * (block - 0.5);
            const double ff = featFade(0.4, detailM);   // the finer set: ridges 0.6-1.1 km apart, 40-80 m high
            if (ff > 0) h += lane * ff * (20.0 + 20.0 * unitFromHash(mix64(e + 6))) * std::sin(TAU * coord / (0.6 + 0.5 * unitFromHash(mix64(e + 5))));
        }
    }
    double cr = 0; c.craterStack(cr);
    h += cr * (1 - 0.6 * lane);
    if (lane < 1 && scaleVisible(4.0, detailM)) {   // the furrows of the polygons' ancient impacts: arcs of troughs 12-22 km apart
        double furrow = 0;
        visitFeatures(p, R, R * 0.9, 0.6, g.sF + 86, 0.6, [&](const FeatureHit& f) {
            const double reach = R * (0.3 + 0.25 * f.u[3]);
            if (f.d > reach || f.d < reach * 0.25) return;
            const double fsp = 12.0 + 10.0 * f.u[4], x = f.d / fsp - std::floor(f.d / fsp);
            furrow = std::max(furrow, smoothstep(0.12, 0.0, std::fabs(x - 0.5)) * smoothstep(reach, reach * 0.7, f.d));
        });
        h -= 150.0 * furrow * (1 - lane) * featFade(1.5, detailM);
    }
    s.height = h;
    const double polar = c.absLatDeg + 4.0 * gnoise3(unit * 7.0, g.sF + 87), frost = smoothstep(g.iceCapLat - 4, g.iceCapLat + 6, polar);   // a thin frost brightening
    if (polar > g.iceCapLat + 14) { s.material = MAT_SNOW; s.albedo = 0.72 + 0.08 * base + 0.08 * lane; }   // what lies under it; a cap of it nearer the pole
    else if (lane > 0.5) { s.material = MAT_ICE; s.albedo = 0.55 + 0.08 * base + 0.06 * clampd(cm.rimBright, 0, 1) + 0.2 * clampd(cm.rays, 0, 1) + 0.12 * frost; }
    else { s.material = MAT_DUST; s.albedo = 0.24 + 0.05 * base + 0.18 * clampd(cm.rimBright, 0, 1) + 0.25 * clampd(cm.rays, 0, 1) + 0.2 * frost; }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope + 0.3 * lane, 0, 1);
}

// P-06: Iapetus's ground (Cassini, 2004-07): a moon that faces its giant sweeps dark dust onto the face it leads with (the giant's outer moons'
// debris); the darkened ice warms, sublimes and freezes again on the bright face, a runaway (Spencer and Denk 2010): the leading face coal-black
// round the apex (`twoFacedApex`) to fifty degrees of latitude and a quarter turn and more round the equator either way, the rest bright ice,
// the boundary ragged at every scale; along it the slopes that face the equator dark and those that face the poles bright, the craters' floors
// dark; a ridge along the equator 8-15 km high and 22-32 km across over the dark face and on past its edge, broken into lone peaks beyond; craters at
// every scale, basins half the radius across
void groundTwoFaced(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm;
    const double R = c.R, massif = c.massif, hillsF = c.hillsF, lat = c.lat;
    const double base = c.fb(R * 0.45, g.sA, 4);
    double h = base * g.mountainAmp;
    const double zone = smoothstep(0.1, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.5 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));
    double cr = 0; c.craterStack(cr);
    h += cr;
    const Vec3 a = twoFacedApex(g);
    const double lon = std::atan2(unit.y, unit.x), off = wrapAngle(lon - std::atan2(a.y, a.x));
    {   // the equatorial ridge
        const double ridgeH = 8000.0 + 7000.0 * unitFromHash(mix64(g.seed ^ 0x7F0A1ULL)), ridgeW = 7.0 + 3.0 * unitFromHash(mix64(g.seed ^ 0x7F0A2ULL));   // m; km (its flanks 30-40 degrees)
        const Vec3 ring(std::cos(lon), std::sin(lon), 0);   // the equator's point of this meridian (the ridge's noises run along it)
        const double xKm = (lat - 0.006 * gnoise3(ring * 4.0, g.sF + 92)) * R;   // the crest wanders off the equator
        if (std::fabs(xKm) < 1.6 * ridgeW) {
            const double cont = smoothstep(2.5, 2.0, std::fabs(off)), peaks = smoothstep(0.15, 0.5, gnoise3(ring * 9.0, g.sF + 93));   // on past the dark face's edge
            const double along = (0.75 + 0.25 * gnoise3(ring * 14.0, g.sF + 94)) * (cont + (1 - cont) * peaks);
            const double t = clampd(1 - std::fabs(xKm) / (1.6 * ridgeW), 0, 1);
            h += ridgeH * along * t * t * (3 - 2 * t) + 500.0 * c.rg(6.0, g.sF + 95, 4) * t * along;
        }
    }
    s.height = h;
    // the two faces: dark round the apex and along the equator, the boundary ragged (unfaded fields: the same on every ring)
    const double dd = dot(unit, a), cl = std::cos(lat);
    double dark = 0.45 * dd + 0.7 * cl * cl - 0.6 + 0.3 * fbm3(unit * 1.8, g.sF + 96, 4) + 0.06 * fbm3(p / (R * 0.04), g.sF + 97, 3);
    const double band = smoothstep(0.03, 0.0, std::fabs(dark));   // a band some tens of kilometres wide
    if (band > 0) {   // the runaway's own pattern where the faces meet: dark on the slopes that face the equator and in the craters' floors
        const Vec3 north = normalize(Vec3(0, 0, 1) - unit * unit.z);
        const double eq = dot(rf.gradM, north) * (lat >= 0 ? 1 : -1);   // + where the ground rises toward the pole (it faces the equator)
        dark += band * (0.18 * clampd(eq * 5.0, -1, 1) + 0.12 * clampd(cm.floorDark, 0, 1) - 0.08 * clampd(cm.rimBright, 0, 1));
    }
    if (dark > 0) { s.material = MAT_DUST; s.albedo = 0.04 + 0.02 * clampd(base + 0.5, 0, 1); }
    else { s.material = MAT_ICE; s.albedo = 0.58 + 0.08 * base + 0.08 * clampd(cm.rimBright, 0, 1) + 0.15 * clampd(cm.rays, 0, 1) - 0.12 * smoothstep(-0.25, 0.0, dark); }   // greyer near the dark
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-06: Enceladus's ground (Cassini, 2005-15): the whitest ground there is (the fallout of its own jets), old cratered plains to the north and
// in patches, young plains ridged along a net of 60 km; south of `iceCapLat` the south polar terrain, lower, behind a belt of folds along its
// edge: the four tiger stripes (`tigerStripesOf`), each a rift 500 m deep and 2 km wide between ridges 160 m high, its ice coarse and blue to
// 4-6 km out, the ground between grooved along them; the curtains rise from them (`curtainVents`)
void groundTiger(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF, latDeg = c.lat / DEG;
    const double base = c.fb(R * 0.5, g.sA, 4);
    double h = base * g.mountainAmp;
    const double zone = smoothstep(0.2, 0.7, base), rel = zone * massif * 0.3, hf = hillsF * 0.4;
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));
    const double old = smoothstep(-0.05, 0.2, fbm3(unit * 2.2, g.sC + 60, 3) + 0.4 * std::sin(c.lat));
    double cr = 0; c.craterStack(cr);
    h += cr * (0.2 + 0.8 * old);
    if (old < 1) {
        const double fr = featFade(1.0, detailM);
        if (fr > 0) {
            const Worley3 w = worley3(p / 60.0 + Vec3(gnoise3(p / 90.0, g.sF + 62), gnoise3(p / 90.0, g.sF + 63), 0) * 0.3, g.sF + 64);
            const double dm = (w.f2 - w.f1) * 60.0 * 500.0;   // metres from the line
            h += (1 - old) * fr * 140.0 * std::exp(-(dm / 500.0) * (dm / 500.0));
        }
    }
    const double edge = g.iceCapLat + 3.0 * gnoise3(unit * 5.0, g.sF + 61);
    const double inKm = (-edge - latDeg) * DEG * R;   // km inside the south polar terrain's edge (negative outside)
    double sd = 1e9;
    if (inKm > -40) {
        const double belt = smoothstep(-30, -5, inKm) * smoothstep(40, 10, inKm), fb = featFade(4.5, detailM);
        if (belt > 0 && fb > 0) h += belt * fb * 220.0 * std::sin(TAU * inKm / 9.0);   // the folds along its edge
        h -= 350.0 * smoothstep(-5, 25, inKm);
        if (inKm > 0) {
            const TigerStripes T = tigerStripesOf(g);
            sd = stripeDistanceKm(T, R, unit);
            const double ft = featFade(1.0, detailM);
            if (ft > 0) h += ft * (-500.0 * std::exp(-(sd / 0.8) * (sd / 0.8)) + 160.0 * std::exp(-((sd - 2.0) / 0.7) * ((sd - 2.0) / 0.7)));
            const double fg = featFade(0.5, detailM);
            if (fg > 0) h += fg * 22.0 * std::sin(TAU * dot(unit, T.e2) * R / 1.1) * smoothstep(0, 20, inKm);
        }
    }
    s.height = h;
    const double blue = sd < 1e8 ? smoothstep(6.0, 3.0, sd + 2.0 * fbm3(p / 6.0, g.sG + 65, 2)) : 0.0;
    if (blue > 0.5) { s.material = MAT_ICE; s.albedo = 0.72 + 0.1 * (1 - blue); }
    else { s.material = MAT_SNOW; s.albedo = 0.94 + 0.04 * base + 0.03 * clampd(cm.rimBright, 0, 1) - 0.06 * old; }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-06: Miranda's ground (Voyager 2, 1986): a moon remade in part by the tides of a past resonance: blocks of unlike old ground fitted together
// (rolling cratered country, each block its own shade, roughness and craters, a low scarp where two meet), up to three coronae (`coronaeOf`: a
// belt of parallel ridges and troughs following each edge in bright and dark bands, a rougher core, the floor lower), two chasms where the crust
// pulled apart, and along part of the first corona's edge the great cliff (Verona Rupes): its floor dropped and the old ground lifted, a wall of
// 15-20 km a few kilometres across at its foot
void groundPatchwork(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const Worley3 bw = worley3(p / (R * 0.4), g.sF + 101);   // the old ground's blocks
    const double bEdge = smoothstep(0.0, 0.08, bw.f2 - bw.f1), bShade = (unitFromHash(mix64(bw.id1 + 2)) - 0.5) * 0.16;
    const double bCraters = 0.5 + unitFromHash(mix64(bw.id1 + 3)), bRough = 0.6 + 0.8 * unitFromHash(mix64(bw.id1 + 4));
    const double base = c.fb(R * 0.45, g.sA, 4);
    double h = base * g.mountainAmp + (unitFromHash(mix64(bw.id1 + 1)) - 0.5) * 500.0 * bEdge;
    const double zone = smoothstep(0.1, 0.6, base), rel = zone * massif * bRough, hf = std::max(hillsF, 0.5 * zone) * bRough;
    ReliefSum rf = c.relief(h, clampd(rel, 0, 1), clampd(hf, 0, 1), reliefAmp(g, clampd(rel, 0, 1), clampd(hf, 0, 1)));
    double cr = 0; c.craterStack(cr);
    Corona C[3]; const int nc = coronaeOf(g, C);
    double inCorona = 0, bandV = 0, coreV = 0, faceV = 0;
    for (int k = 0; k < nc; k++) {
        double rho, th, inKm;
        if (!coronaAt(C[k], R, unit, rho, th, inKm) || rho > 1.3) continue;
        const double inside = smoothstep(1.03, 0.97, rho);
        inCorona = std::max(inCorona, inside);
        h -= 600.0 * inside;
        if (inside > 0) {
            const double belt = smoothstep(0.4, 0.5, rho) * inside, fs = featFade(C[k].sp * 0.5, detailM);
            if (fs > 0) { const double x = inKm / C[k].sp - std::floor(inKm / C[k].sp); h += belt * fs * C[k].amp * (std::pow(0.5 - 0.5 * std::cos(TAU * x), 1.5) - 0.4); }
            if (belt > 0.5) bandV = std::sin(TAU * inKm / (0.35 * std::sqrt(C[k].a * C[k].b)) + C[k].band + 0.8 * gnoise3(p / 40.0, g.sF + 102));
            const double core = smoothstep(0.5, 0.4, rho) * inside;
            if (core > 0) { h += core * 400.0 * c.rg(25.0, g.sF + 103, 4); coreV = std::max(coreV, core); }
        }
        if (k == 0) {   // the great cliff
            double th0, span, H; cliffOf(g, th0, span, H);
            const double seg = smoothstep(span, span * 0.6, std::fabs(wrapAngle(th - th0)));
            if (seg > 0) {
                const double sKm = -inKm, face = 3.0, st = smoothstep(-face, face, sKm), fall = std::exp(-std::fabs(sKm) / 70.0);
                h += H * seg * (st - 0.5) * fall;
                faceV = std::max(faceV, seg * (1 - std::fabs(2 * st - 1)));
            }
        }
    }
    h += cr * bCraters * (1 - 0.7 * inCorona);
    for (int k = 0; k < 2; k++) {   // the chasms: graben along arcs of great circles
        auto hu = [&](int j) { return unitFromHash(mix64(g.seed ^ (0x9A7D0ULL + (uint64_t)(k * 16 + j) * 0x9E3779B97F4A7C15ULL))); };
        const Vec3 nrm = normalize(Vec3(hu(0) - 0.5, hu(1) - 0.5, hu(2) - 0.5));
        const double offKm = dot(unit, nrm) * R, wHalf = 6.0 + 6.0 * hu(3);
        if (std::fabs(offKm) > wHalf + 12.0) continue;
        const Vec3 mid = normalize(cross(nrm, normalize(Vec3(hu(4) - 0.5, hu(5) - 0.5, hu(6) - 0.5))));
        const Vec3 inPlane = normalize(unit - nrm * dot(unit, nrm));
        const double ang = std::acos(clampd(dot(inPlane, mid), -1, 1)), halfArc = 0.5 + 0.5 * hu(7);
        const double taper = smoothstep(halfArc, halfArc * 0.8, ang);
        if (taper <= 0) continue;
        const double depth = 2500.0 + 2500.0 * hu(8), x = std::fabs(offKm);
        const double floorP = smoothstep(wHalf + 3.0, wHalf, x), shoulder = smoothstep(wHalf + 12.0, wHalf + 5.0, x) * smoothstep(wHalf + 2.0, wHalf + 5.0, x);
        h += taper * (-depth * floorP + 300.0 * shoulder);
        if (floorP > 0.05 && floorP < 0.95) faceV = std::max(faceV, taper);
    }
    s.height = h;
    if (faceV > 0.3) { s.material = MAT_ROCK; s.albedo = 0.36 + 0.06 * base; }
    else if (inCorona > 0.5 && coreV < 0.5 && bandV < -0.2) { s.material = MAT_DUST; s.albedo = 0.34 + 0.04 * base; }   // a corona's dark bands
    else if (inCorona > 0.5) { s.material = MAT_ICE; s.albedo = (coreV > 0.5 ? 0.42 : 0.58) + 0.06 * base + 0.08 * clampd(cm.rimBright, 0, 1); }
    else { s.material = MAT_ICE; s.albedo = 0.34 + bShade + 0.05 * base + 0.12 * clampd(cm.rimBright, 0, 1) + 0.2 * clampd(cm.rays, 0, 1); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope + faceV, 0, 1);
}

// P-06: the nearest and second nearest of a grid's features projected onto the sphere (km from a point), and the nearest's own number: cells packed
// edge to edge on the surface (`worley3` cuts its cells through the sphere, so its nearest feature is seldom under a point: its cells' middles
// are rare on the ground)
static void surfaceCells(const Vec3& p, double R, double cellKm, uint64_t seed, double& f1, double& f2, double& u1) {
    f1 = f2 = 1e18; u1 = 0.5;
    visitFeatures(p, R, cellKm, 1.0, seed, 3.0, [&](const FeatureHit& f) {
        if (f.d < f1) { f2 = f1; f1 = f.d; u1 = f.u[0]; } else if (f.d < f2) f2 = f.d;
    });
}
// P-06: Hyperion's ground (Cassini, 2005): a porous moon, four tenths of it void, whose craters crushed inward rather than throwing anything out:
// pits packed edge to edge at five scales (a hundred kilometres to half of one), each deep for its width (a twentieth to a fifth of it), its floor
// flat, its walls steepening to a knife-edged rim shared with its neighbours; dark red-brown dust pooled in the floors, the walls bright ice
void groundSponge(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.4, g.sA, 3);
    double h = base * g.mountainAmp;
    const double rel = 0.3 * massif, hf = 0.4 * hillsF;
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));
    static const double CELL[5] = {0.18, 0.045, 0.011, 0.0028, 0.0007}, DEPTH[5] = {0.05, 0.11, 0.16, 0.2, 0.22};   // a fraction of the radius; of the cell
    double dark = 0, rim = 0;
    for (int i = 0; i < 5; i++) {
        const double cellKm = R * CELL[i], f = featFade(cellKm * 0.5, detailM);
        const bool mv = matVisible(cellKm * 0.36, detailM);   // a floor is a third of its pit across
        if (f <= 0 && !mv) continue;
        const Vec3 q = p / cellKm, wp = p + Vec3(gnoise3(q * 1.7, g.sF + 110 + i), gnoise3(q * 1.7, g.sF + 120 + i), gnoise3(q * 1.7, g.sF + 130 + i)) * (0.18 * cellKm);
        double f1, f2, u1; surfaceCells(wp, R, cellKm, g.sF + 140 + i, f1, f2, u1);
        const double r = f2 < 1e17 ? 2 * f1 / std::max(1e-9, f1 + f2) : 1.0;   // 0 at the pit's middle, 1 on its rim
        const double dep = DEPTH[i] * cellKm * 1000.0 * (0.65 + 0.7 * u1);
        h -= dep * (1 - r * r * r) * f;
        if (mv && i < 4) { dark = std::max(dark, smoothstep(0.42, 0.3, r)); rim = std::max(rim, smoothstep(0.88, 0.97, r)); }
    }
    s.height = h;
    if (dark > 0.5) { s.material = MAT_DUST; s.albedo = 0.08 + 0.04 * clampd(base + 0.5, 0, 1); }
    else { s.material = MAT_ICE; s.albedo = 0.38 + 0.06 * base + 0.1 * rim; }
    s.relief = clampd(0.5 + 0.5 * rf.slope, 0, 1);
}

// P-00: a giant's clouds: bands, storms, a great storm (gas giants, brown dwarfs)
// the dark ocean (Stevenson 1999): one sea over the whole world, its floor an abyssal plain a kilometre and a half down, kept liquid by
// its rock's own heat under hundreds of bars of hydrogen. Where its shield volcanoes break the surface, islands of black basalt (lava that met
// the sea: Hawaii's and Iceland's): shields 8-21 km round on a 70 km grid, thickest and tallest over the hot spots' swells (the archipelagos: a
// few hundredths of the world above the sea), their flanks streaked
// with young flows, the waves' benches of black sand round their shores, the summits' fumarole fields crusted pale (`EV_FUMAROLES` steams on
// them); cinder and spatter cones on their flanks
void groundDarkOcean(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    const double base = c.fb(R * 0.4, g.sA, 4);
    const double swell = smoothstep(0.05, 0.45, fbm3(p / (R * 0.12), g.sB + 41, 3));   // the hot spots' swells the archipelagos stand on (unfaded: the map sees them)
    double h = -1600.0 + 500.0 * base + 1150.0 * swell;   // the abyssal plain, half a kilometre down on a swell
    bool caldera = false;
    if (scaleVisible(70.0, detailM)) h += volcanoField(p, R, 70.0, std::min(1.0, g.volcanoes * (0.5 + swell)), g.sD, 1500.0 + g.mountainAmp, caldera);
    const double isle = smoothstep(-300.0, 150.0, h);   // the islands and their shallows carry the relief
    const double rel = isle * massif * 0.7, hf = std::max(hillsF, 0.6 * isle) * isle;
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf) * isle);   // the skeleton: the flanks' relief, the traits' landforms (cinders, geysers, columns, maars)
    if (scaleVisible(8.0, detailM)) h += volcanoField(p, R, 8.0, 0.35 * isle, g.sD + 5, 160.0, caldera);   // cinder and spatter cones on the flanks
    const LavaOut lf = lavaFlows(p, R, 70.0, std::min(1.0, g.volcanoes * (0.5 + swell)), g.sD, detailM);   // the shields' young flows (O6-05's tongues)
    h += lf.dh * isle;
    const double bench = smoothstep(-12.0, -2.0, h) * smoothstep(14.0, 3.0, h);   // the waves cut the shores to benches a few metres over the sea
    h += (0.6 + 0.25 * h - h) * bench * 0.75;
    s.height = h;
    if (h < 0) { s.material = MAT_WATER; s.water = 0; s.albedo = 0.05; s.biome = BIO_OCEAN; s.relief = 0; return; }
    // the summits' fumarole fields: pale crusts in patches round the calderas and the highest ground (unfaded fields)
    const double crustF = fbm3(p / 0.8, g.sG + 41, 3) + (caldera ? 0.5 : 0.0) + 0.6 * smoothstep(600.0, 1600.0, h) - 0.55;
    if (crustF > 0.12) { s.material = MAT_SULPHUR; s.albedo = 0.46 + 0.1 * crustF; }
    else if (h < 4.5 && rf.slopeM < 0.3) { s.material = MAT_SAND; s.albedo = 0.07; }   // black sand on the benches
    else if (lf.flow > 0.4 || lf.pit > 0.3) { s.material = MAT_BASALT; s.albedo = 0.05 + 0.02 * (1 - lf.flow); if (lf.pit > 0.6) s.glow = 0.25; }   // young flows, a tube's skylight
    else if (rf.slopeM > 0.45) { s.material = MAT_ROCK; s.albedo = 0.12; }   // the old flows' cliffs
    else { s.material = MAT_BASALT; s.albedo = 0.08 + 0.04 * (0.5 + 0.5 * base); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// D-04 (TR_EMBERS on a world with no lava of its own): a young interior still erupting in the dark. Fields of it 80-250 km across on 45% of the
// cells of a 1,300 km grid (`emberGrid`: the globe's spots of light), and in a field lava lakes 160-600 m across on half the cells of a 3 km grid
// (fewer toward its edge): the lake's crust-veined lava a few metres under the plain inside a spatter rampart 4-12 m high (seen across the plain),
// fresh black basalt round it to three and a half of its radii, the snow kept off the warm ground (`warm`)
static double emberLakes(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double detailM = c.detailM;
    const double field = emberFieldAt(g, p);
    if (field < 0.02) return 0;
    double warm = 0;
    if (field > 0.3 && s.material != MAT_WATER && s.material != MAT_ICE) { s.material = MAT_BASALT; s.albedo = std::min(s.albedo, 0.10); }   // the field's young lava plains (the globe sees them)
    if (!matVisible(0.08, detailM)) return 0;
    const double ground = c.reliefDone ? c.hSmooth : s.height;   // a lake's level under the smooth ground (its lava flat)
    double dh = 0; bool lava = false, rimRock = false, fresh = false, crack = false;
    visitFeatures(p, g.R, 3.0, 0.5, g.sE + 521, 0.7, [&](const FeatureHit& f) {
        if (f.u[5] > field) return;   // fewer toward the field's edge
        const double rL = 0.08 + 0.22 * f.u[3], x = f.d / rL, depth = 1.0 + 3.0 * f.u[4], rim = 4.0 + 8.0 * f.u[0];
        if (x >= 3.5) return;
        warm = std::max(warm, smoothstep(3.5, 1.2, x));
        const double fd = featFade(2 * rL, detailM);
        if (x < 1.0) { dh = std::min(dh, (ground - depth - s.height) * fd); lava = true; }
        else if (x < 1.25) dh = std::min(dh, (ground - depth - s.height + (depth + rim) * smoothstep(1.0, 1.25, x)) * fd);   // the inner wall
        else if (dh >= 0) dh = std::max(dh, rim * smoothstep(2.4, 1.25, x) * fd);   // the rim's spatter
        if (x >= 1.0 && x < 1.6) rimRock = true;
        if (x >= 1.0 && x < 3.5) fresh = true;
        if (x >= 1.3 && x < 2.6 && fbm3(p / 0.04, g.sG + 522 + (uint64_t)(f.u[1] * 64), 2) > 0.38) crack = true;   // glowing cracks in the warm crust
    });
    s.height += dh;
    if (lava) { s.material = MAT_LAVA; s.glow = 1.0; s.albedo = 1.0; }
    else if (crack) { s.material = MAT_LAVA; s.glow = 0.55; s.albedo = 0.9; }
    else if (rimRock) { s.material = MAT_BASALT; s.albedo = 0.06; }
    else if (fresh) { s.material = MAT_BASALT; s.albedo = 0.05; }
    return warm;
}

// D-04: the ground in the dark (a body of a drifter's system, after its kind's own): its liquid frozen at its level where it is too cold for it
// (`frozenSeas`); a young interior's lava lakes (TR_EMBERS on a world with no lava of its own); its air fallen as snow (TR_FROZEN_AIR: as deep
// on the open ground as its air was heavy, a hundred metres of snow for every five bars, the hollows filled as far as it reaches, shed from the
// steep slopes, frost on the rest; never on lava or the ground warm round it; the felisian layer's frozen cover is its air's snow already)
void darkGround(GroundCtx& c) {
    const BodyGen& g = c.g; SurfaceSample& s = c.s;
    if (g.frozenSeas && (s.material == MAT_WATER || s.water > -1e8)) {
        if (s.material == MAT_WATER) { s.height = std::max(s.height, s.water); s.material = MAT_ICE; s.albedo = 0.62; s.biome = BIO_ICE; s.glow = 0; s.waterTint = 0; s.lifeTint = 0; }
        s.water = -1e9; s.waterKind = 0; s.shore = 1e9;
    }
    const double warm = g.hasTrait(TR_EMBERS) && !isLavaWorld(g.type) ? emberLakes(c) : 0.0;
    if (g.hasTrait(TR_FROZEN_AIR) && c.K.ground != GR_FELISIAN && s.material != MAT_LAVA && s.material != MAT_WATER) {
        const KindDef& K = c.K;
        const double airAtm = K.pressureLo + K.pressureSpan * unitFromHash(g.seed);   // what the surface would read had it kept its air
        const double open = std::min(30.0, 20.0 * airAtm), fill = clampd(c.hSmooth - s.height, 0, 600) * clampd(open / 4.0, 0, 1);
        const double shed = smoothstep(0.32, 0.55, c.slopeM);
        const double depthM = (open + 0.85 * fill) * (1 - shed) * (1 - warm);
        s.height += depthM;
        if (depthM > 0.3) { s.material = MAT_SNOW; s.albedo = 0.80 + 0.06 * clampd(depthM / 10.0, 0, 1); }
        else if (s.material != MAT_ICE && s.material != MAT_SNOW && warm < 0.5) s.albedo = std::max(s.albedo, 0.32 + 0.3 * (1 - shed));   // its frost
    }
}

void groundGiant(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, lat = c.lat;
    double turb = fbm3(p / (R * 0.18), g.sA, 4) * 0.35;
    double turb2 = fbm3(p / (R * 0.06), g.sB, 2) * 0.05;
    double l = lat + turb + turb2;
    double bands = 0.5 + g.bandContrast * (0.28 * std::sin(l * g.bandCount) + 0.15 * std::sin(l * g.bandCount * 2.3 + 1.0) +
                   0.08 * std::sin(l * g.bandCount * 5.1 + 2.0));   // X-03: an ice giant's faint
    if (c.K.giantClass == GC_WATER) bands += 0.16;   // water's clouds the brightest (a Bond albedo of 0.8)
    double storm = giantStormCell(g, unit);   // the giant's own share
    s.height = 0;
    s.material = MAT_GAS;
    s.albedo = clampd(bands + 0.35 * storm, 0.22, 1.0);
    s.veg = storm;   // the storm term kept apart (the map's veg channel) so the globe can let storms grow and fade with time
    if (g.gsOn) {   // the great storm (in the albedo, not the storm term: it does not fade), its bands swirled round its centre
        const double lon = std::atan2(unit.y, unit.x);
        const double ex = wrapAngle(lon - g.gsLon) * std::cos(g.gsLat) / g.gsA, ey = (lat - g.gsLat) / g.gsB, d = std::sqrt(ex * ex + ey * ey);
        if (d < 1.6) {
            const double th = std::atan2(ey, ex) + 2.8 * std::pow(std::max(0.0, 1 - d), 1.5);   // the inner parts turned further round
            const double sw = fbm3(Vec3(3.2 * d * std::cos(th), 3.2 * d * std::sin(th), 7.7), g.sE, 3);
            const double core = 1 - smoothstep(0.82, 1.0, d), collar = std::exp(-((d - 1.0) / 0.075) * ((d - 1.0) / 0.075));
            const double inner = clampd(0.5 + g.gsTone * (0.24 + 0.08 * (1 - d)) + 0.16 * sw, 0.22, 1.0);
            s.albedo = clampd(s.albedo + (inner - s.albedo) * core - 0.16 * g.gsTone * collar * (1 - core), 0.22, 1.0);
            s.veg *= smoothstep(1.0, 1.4, d);   // no small storm inside the great one
        }
    }
}
