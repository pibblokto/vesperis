// E-07: the felisian layer, the living worlds' ground (`felisianGround`), the lakes' bowls, the drainage cut into a ground
// (`applyDrainage`, every wet or dry ground's) and the felisian sample (`felisianSample`: the biomes, the water, the cover)
#include "planet_internal.h"
#include "drainage.h"

namespace {

inline double warpedFbm(const Vec3& p, double wl, uint64_t seed, int oct, double warpAmt) {
    Vec3 q = p;
    if (warpAmt > 0) {
        double wx = gnoise3(p / (wl * 0.7), seed + 91), wy = gnoise3(p / (wl * 0.7), seed + 92),
               wz = gnoise3(p / (wl * 0.7), seed + 93);
        q = p + Vec3(wx, wy, wz) * (wl * warpAmt);
    }
    return fbm3(q / wl, seed, oct);
}

// B-320: the felisian ground in one place (the sample, the far rings, the map and the lake levels read it): the
// continents, the plates, the mountain regions, the relief spectrum, the mesas, the islands, the shelf and the dunes.
// `hS` is the smooth ground, the relief's octaves of 2 km and longer over the base: the surface rivers, lakes and pools
// are cut into, so their water lies flat where the fine relief would have made it a bumpy sheet
double lakeBowlAt(const BodyGen& g, const Vec3& p, double detailM);   // the lake bowls the drainage floods
struct FelGround {
    double h = 0, hS = 0, moist = 0.5, landMask = 0, relief0 = 0, hills = 0, slope = 0;
    double cliff = 0, talus = 0;   // O6-04: on a cliff band's riser, on its talus
    Vec3 gradT;                    // the relief's gradient on the tangent plane (rise over run, uphill)
    double slopeM = 0; Vec3 gradM; // the material chain's slope and gradient (the same in every ring): the materials read these
    bool caldera = false;
    double iceSheet = 0;   // the ice age's sheet over the land here (metres thick: the tundra's, KT_ICE_SHEETS)
    bool sheetWall = false;   // on the sheet's terminus wall (its bare blue ice: the bedrock's slope under it said nothing of the ice's)
    double slip = 0;   // the dry lowlands' dunes' slip face here (`SurfaceSample::slipFace`)
    LandOut land;   // the traits' landforms here
};
void felisianGround(const BodyGen& g, const Vec3& unit, double detailM, FelGround& o) {
    const double R = g.R;
    const KindDef& K = KINDS[g.type];
    Vec3 p = unit * R;
    double lat = std::asin(clampd(unit.z, -1, 1));
    auto fb = [&](double wlKm, uint64_t seed, int maxOct) -> double {
        double fade = lodFade(wlKm, detailM);
        if (fade <= 0) return 0.0;
        return fbm3(p / wlKm, seed, std::min(maxOct, octs(wlKm, detailM))) * fade;
    };
    auto rg = [&](double wlKm, uint64_t seed, int maxOct) -> double {
        double fade = lodFade(wlKm, detailM);
        if (fade <= 0) return 0.0;
        return ridged3(p / wlKm, seed, std::min(maxOct, octs(wlKm, detailM))) * fade;
    };
    Tectonics tec = tectonicsAt(unit, g);
    double massif = smoothstep(0.2, 0.6, rg(std::min(110.0, R * 0.3), g.sC + 6, 3));
    double hillsF = smoothstep(-0.15, 0.45, fb(std::min(R * 0.06, 400.0), g.sB + 6, 3));
    double contWl = R * 0.6 * g.contScale;
    double cont = warpedFbm(p, contWl, g.sA, std::min(6, octs(contWl, detailM)), 0.22) * lodFade(contWl, detailM);
    cont += tec.uplift * 0.75;                     // M9-02: plate interiors decide most of the land
    double land = cont - g.seaLevel;               // >0 land
    // the coast's exposure to the prevailing wind (the continent field's gradient against the wind: positive where
    // the wind blows onto the land), evaluated in the coastal band alone
    double exposure = 0;
    if (std::fabs(land) < 0.06 && scaleVisible(0.3, detailM)) {
        Vec3 pole(0, 0, 1);
        Vec3 e = normalize(cross(pole, unit)), n = cross(unit, e);
        if (length2(cross(pole, unit)) > 1e-8) {
            double dk = 2.0;   // km
            double cE = warpedFbm(p + e * dk, contWl, g.sA, std::min(6, octs(contWl, detailM)), 0.22) - warpedFbm(p - e * dk, contWl, g.sA, std::min(6, octs(contWl, detailM)), 0.22);
            double cN = warpedFbm(p + n * dk, contWl, g.sA, std::min(6, octs(contWl, detailM)), 0.22) - warpedFbm(p - n * dk, contWl, g.sA, std::min(6, octs(contWl, detailM)), 0.22);
            double gl = std::sqrt(cE * cE + cN * cN);
            if (gl > 1e-9) {
                int bandIdx = (int)std::floor(std::fabs(lat) / (30 * DEG));
                double a = g.windAngle + (lat < 0 ? PI : 0) + bandIdx * 0.9;   // the wind as the dunes read it
                double wE = std::cos(a), wN = std::sin(a);
                exposure = (cE * wE + cN * wN) / gl;   // +1: the wind blows straight onto this shore
            }
        }
    }
    double landMask = smoothstep(-0.02, 0.25, land);
    double mtnL = rg(R * 0.14, g.sC, 2);           // mountain regions (visible from orbit)
    double hills = fb(5.0, g.sB, 6);               // the hill pattern: beaches and wetland pools read it (it does not shape the ground)
    double h = land * 3200.0;
    // the continental base is 1500 land^0.7 (an interior at 500-900 m, a shield at 1500 m; it was 3200 land^0.8,
    // whose plateaus at 2-3 km put half the land over the snow line); the sea keeps its depth from `land x 3200`
    if (h > 0) h = 1500.0 * std::pow(land, 0.7);
    const double hCont = h;   // the continents' base (a snowball's sheet lies smooth over it)
    double mtnMask = smoothstep(0.35, 0.75, mtnL) * landMask;
    // mountain chains along convergent boundaries, rifts and ridges along divergent ones
    double chainR = rg(45.0, g.sC + 2, 7);
    mtnMask = clampd(mtnMask + tec.chain * landMask * 0.9, 0, 1);
    h += tec.chain * landMask * (chainR * chainR * g.chainAmp + std::fabs(chainR) * 600.0);
    h -= tec.rift * landMask * 700.0 * (0.5 + 0.5 * std::fabs(fb(60.0, g.sC + 3, 3)));
    h += tec.ridge * (1 - landMask) * 900.0 * std::fabs(rg(40.0, g.sC + 4, 5));
    h += tec.arc * (1 - landMask) * 3200.0 * std::max(0.0, fb(28.0, g.sC + 5, 5));   // island arcs
    h += mtnL * mtnL * g.mountainAmp * mtnMask;
    // the relief spectrum in the massifs, in hill country and (faintly) on the plains
    double rel = mtnMask * massif, hf = std::max(hillsF, 0.6 * mtnMask);   // massifs are mountains, the rest of the region hill country
    double relief0 = clampd(rel + 0.25 * hf, 0, 1);
    double moist = 0.5 + 0.5 * fbm3(p / (R * 0.22), g.sF, 3) + g.moistureBias;
    {
        // rain shadow (M9-05): mountains upwind dry the land behind them
        Vec3 wdir = normalize(Vec3(std::cos(g.windAngle), std::sin(g.windAngle), 0.1));
        Vec3 pu = p + wdir * 90.0;
        double mtnUp = ridged3(pu / (R * 0.14), g.sC, 2) * lodFade(R * 0.14, detailM);
        moist -= 0.28 * smoothstep(0.4, 0.8, mtnUp) * landMask;
    }
    if ((K.tags & KT_FIXED_SUN) && g.locked) {   // a locked world's water by its climate at the sea's level (the same in every ring): the ring round
        // 8 C wet with the night's meltwater and the rain the day's air carries over it, the day side dried from 15 C (M5-08's rule is the others')
        const double Tsl = climateTempC(g, std::fabs(lat) / DEG, 0, 0, dot(unit, g.lockedDir)), x = (Tsl - 8.0) / 12.0;
        moist += K.ringWet * std::exp(-x * x) - K.dayDry * smoothstep(15.0, 45.0, Tsl);
    }
    ReliefSum rf = reliefSum(p, unit, g, reliefAmp(g, rel, hf) * (0.05 + 0.95 * landMask), g.reliefRidge * (0.4 + 0.6 * rel), g.sB + 7, detailM);
    double hS = h + rf.hCoarse;
    double hRel = h; h += rf.h;
    {   // cliff bands on the steep ground of the massifs and the chains
        CliffOut co = cliffBands(p, g, h, rf.slopeM, smoothstep(0.12, 0.4, relief0) * landMask, detailM);   // on the material chain's slope
        h += co.dh; o.cliff = co.cliff; o.talus = co.talus;
    }
    // the plains' own small relief, so flat country is not a billiard table underfoot (the shared term:
    // hummocks, outcrops, and gullies where it rains)
    // x1.6: the floodplains of O6-03 are flat, and the flat class keeps a 1% median grade
    h += plainRelief(p, g, 1.6 * smoothstep(0.3, 0.1, relief0) * landMask, smoothstep(0.28, 0.42, moist) * (0.7 + 0.8 * moist), detailM);
    // the mean climate on the smooth ground (the landforms', the permafrost's; P-03: a fixed sun's by its side)
    const double T0 = climateTempC(g, std::fabs(lat) / DEG, hS, 0, (K.tags & KT_FIXED_SUN) && g.locked ? dot(unit, g.lockedDir) : 0.0);
    {   // the traits' landforms, cut into the smooth ground too (rivers and lakes follow them)
        // the ground on the material chain's relief alone (the cliffs and the plains fade by the ring)
        LandCtx lc; lc.landMask = landMask; lc.moist = moist; lc.relief0 = relief0; lc.h = h; lc.hS = hS; lc.T = T0; lc.hM = hRel + rf.hM; lc.slope = rf.slopeM;
        landformsAt(g, p, unit, detailM, lc, o.land);
        h += (hS - h) * o.land.flatten;
        double dS = o.land.dhSmooth + o.land.flatOffset * o.land.flatten;
        h += dS + o.land.dhFine; hS += dS;
    }
    if ((K.tags & KT_ICE_SHEETS) && !g.hasTrait(TR_POLYGONS)) {   // an ice age's permafrost patterned in polygons on the cold flats
        // (the patterned ground of R-305 everywhere it is cold, not as a trait: a honeycomb of low ridges 40-100 m across)
        double mask = smoothstep(2, -4, T0) * landMask * smoothstep(0.25, 0.1, relief0);
        double cell = 0.04 + 0.06 * unitFromHash(mix64(g.seed ^ 0x9071ULL));
        if (mask > 0.01 && scaleVisible(cell, detailM)) {
            Worley3 w = worley3(p / cell, g.sE + 310);
            double ridge = 1 - smoothstep(0.0, 0.006, (w.f2 - w.f1) * cell);
            h += (1.5 * ridge * ridge - 0.4 * (1 - ridge)) * mask * lodFade(cell, detailM);
        }
    }
    if (landMask > 0.5 && hS > 10) {   // the lake bowls, dug into the ground and the smooth ground alike; the drainage floods them
        double bowl = lakeBowlAt(g, p, detailM);
        h -= bowl; hS -= bowl;
    }
    if (moist < 0.32 && relief0 < 0.4 && h > 30 && !g.hasTrait(TR_MESAS)) {   // dry flats terrace into mesas (the badlands trait has its own)
        double step = 60.0, q = std::floor(h / step) * step;
        double f = (h - q) / step;
        double terr = q + step * smoothstep(0.35, 0.65, f);
        h = h + (terr - h) * (1 - smoothstep(0.2, 0.32, moist)) * (1 - smoothstep(0.25, 0.4, relief0));
    }
    // volcanic islands (M9-07) rising from the sea with a caldera lake
    bool caldera = false;
    if (scaleVisible(R * 0.25, detailM) && landMask < 0.5) {
        double vf = volcanoField(p, R, R * 0.25, g.islandDensity, g.sD + 9, 2600.0, caldera) * (1 - landMask) * lodFade(R * 0.25, detailM);
        h += vf;
        hS += vf;
    }
    if (h < 0) { h = -std::pow(-h, 1.15); if (g.seaDepth != 1) h *= g.seaDepth; }   // shallow coastal shelf (P-02: a kind's seas shallower)
    if (hS < 0) { hS = -std::pow(-hS, 1.15); if (g.seaDepth != 1) hS *= g.seaDepth; }
    // the coast by its exposure. Windward: the land within a few metres of the sea stands up as a cliff 8-40 m
    // high with a wave-cut platform a metre or two under the water in front of it; leeward: a beach half as steep with
    // a foredune behind; in the flat shallows a barrier island rises along the shore 300-600 m out
    if (exposure != 0 && std::fabs(h) < 60) {
        double f = lodFade(0.3, detailM);
        double wind = smoothstep(0.25, 0.7, exposure), lee = smoothstep(-0.25, -0.7, exposure);
        if (wind > 0 && h > 0) {
            double cliffH = (8 + 32 * unitFromHash(mix64(g.seed ^ 0xC0A5))) * (0.6 + 0.4 * gnoise3(p / 6.0, g.sF + 44));
            double up = smoothstep(0.0, 2.5, h) * smoothstep(cliffH + 30, cliffH, h);
            h += (std::max(h, cliffH) - h) * up * wind * f;
            hS += (std::max(hS, cliffH) - hS) * up * wind * f * 0.5;
        }
        if (wind > 0 && h < 0 && h > -8) h += (-1.5 - 0.5 * (-h) - h) * smoothstep(-8, -3, h) * wind * f;   // the platform
        if (lee > 0 && h > 0 && h < 12) {
            h *= 1 - 0.5 * lee * f * smoothstep(12, 2, h);                                   // the beach
            double dune = 4.0 * smoothstep(1.5, 4.0, h) * smoothstep(9.0, 5.0, h) * (0.5 + 0.5 * gnoise3(p / 0.25, g.sF + 45));
            h += dune * lee * f * lodFade(0.25, detailM);                                    // the foredune
        }
        if (lee > 0 && h < 0 && h > -6) {   // the barrier island: a ridge 300-600 m off the shore where the shallows are flat
            double dOff = -h / 0.008;   // metres offshore for a 0.8% shelf
            double band = std::exp(-((dOff - 450) * (dOff - 450)) / (130.0 * 130.0));
            double along = gnoise3(p / 1.2 + Vec3(7, 3, 1), g.sF + 46);
            h += (2.5 - h) * band * smoothstep(0.0, 0.35, along) * lee * f;
        }
    }
    // cirques on the high ground of the cold country: hemispherical bites 600-1200 m across and 60-150 m deep
    // with a flat floor (the drainage fills a tarn where the floor is a bowl)
    if (relief0 > 0.3 && scaleVisible(3.0, detailM)) {
        double lf0 = std::fabs(lat) / DEG / 85.0 * g.latGradient;
        double snow0 = g.snowLine * std::max(0.0, 1.0 - lf0 * lf0);
        double high = smoothstep(snow0 - 700, snow0 - 250, h) * landMask;
        if (high > 0.01) {
            double bite = 0;
            visitFeatures(p, R, 3.0, 0.35, g.sE + 380, 0.25, [&](const FeatureHit& f) {
                double r = 0.3 + 0.3 * f.u[3], x = f.d / r;
                if (x >= 1) return;
                bite += (60 + 90 * f.u[4]) * std::max(0.0, 1 - x * x) * (1 - 0.5 * smoothstep(0.7, 1.0, x)) * featFade(2 * r, detailM);   // a cirque's own fade
            });
            h -= bite * high;
            hS -= bite * high * 0.7;
        }
    }
    // dune seas (M9-08) in dry lowlands
    if (moist < 0.28 && h > 5 && h < 700 && relief0 < 0.3) {
        const double share = 1 - smoothstep(0.24, 0.28, moist); double lee = 0;
        h += duneField(p, g, lat, 6.0 + 16.0 * (0.28 - moist) / 0.28, detailM, &lee) * share;
        o.slip = share * lee;
    }
    if (K.tags & KT_ICE_SHEETS) {   // the ice age's sheets stand over the land poleward of the cap's mean edge (no season: the ground
        // is the world's), a dome up to 2.2 km thick at fourteen degrees in, its margin steep (a parabolic profile, the ice's), the land's fine
        // relief buried; at the coast the sheet ends in a wall of ice. P-03: a terminator world's over its night side (`iceSheetIn`); a
        // snowball's surface smooth over the continents' base, its ranges standing out of it as nunataks
        double in = iceSheetIn(g, unit);
        if (in > 0 && landMask > 0.02) {
            double cover = smoothstep(0.02, 0.3, landMask);
            // the terminus: an ice cliff 20-55 m high over its first 30 m (Greenland's on land; P-03: a terminator world's over 80 m, its
            // edge oblique to the grid)
            const double wallIn = K.sheetByCold ? 80.0 * g.subContrast / (25.0 * R * 1000.0) : 30.0 / (14.0 * DEG * R * 1000.0);
            double thick = (K.sheetM * std::sqrt(std::min(1.0, in)) + (20 + 35 * (0.5 + 0.5 * gnoise3(p / 0.8, g.sF + 71))) * smoothstep(0.0, wallIn, in)) * cover;
            o.sheetWall = in < wallIn && cover > 0.5;
            if (K.nunataks) {
                const double surf = std::max(hCont, 0.0) + thick + 0.8 * fbm3(p / 0.12, g.sF + 72, 2) * lodFade(0.12, detailM);   // the sheet's own surface, its sastrugi
                if (h < surf) { o.iceSheet = surf - h; h = surf; }
                hS = std::max(hS, surf);
            } else {
                h = hS + (h - hS) * (1 - smoothstep(0.0, 0.03, in) * cover) + thick;
                hS += thick;
                o.iceSheet = thick;
            }
        }
    }
    o.h = h;
    o.hS = hS;
    o.moist = moist;
    o.landMask = landMask;
    o.relief0 = relief0;
    o.hills = hills;
    o.slope = rf.slope;
    o.gradT = rf.gradT;
    o.slopeM = rf.slopeM;
    o.gradM = rf.gradM;
    o.caldera = caldera;
}

// O6-03: a lake has no level of its own. The 7 km lake cells of B-320 dig bowls into the ground
// and the smooth ground (6-14 m deep, 0.4-1.3 km across, an outline that wobbles round the centre) where the country is
// flat, wet and low, and the drainage's flood fills them to their spill: a lake's level and its shore come from the same
// flood as the rivers' (`drainageAt`). `lakeSiteOk` asks the ground at the bowl's centre (one entry per thread; never
// re-entered: the centre's ground is read without bowls)
static thread_local int t_noBowls = 0;
bool lakeSiteOk(const BodyGen& g, const Vec3& centreUnit, uint64_t id) {
    static thread_local uint64_t lastId = 0; static thread_local uint64_t lastSeed = 0; static thread_local bool lastOk = false;
    if (id == lastId && lastSeed == g.seed) return lastOk;
    if (t_noBowls) return false;
    t_noBowls++;
    FelGround o;
    felisianGround(g, centreUnit, 100.0, o);
    t_noBowls--;
    bool ok = o.landMask > 0.6 && o.hS > 15 && o.relief0 < 0.3 && o.moist > 0.42;   // flat country, wetter than the badlands (0.42)
    lastId = id; lastSeed = g.seed; lastOk = ok;
    return ok;
}
double lakeBowlAt(const BodyGen& g, const Vec3& p, double detailM) {
    if (t_noBowls || !scaleVisible(2.0, detailM)) return 0;
    double bowl = 0;
    visitFeatures(p, g.R, 7.0, g.lakeDensity, g.sF + 50, 0.32, [&](const FeatureHit& f) {
        double r = f.cellKm * (0.06 + 0.13 * f.u[3]);
        if (f.d > r * 1.5) return;
        Vec3 cn = normalize(f.c);
        Vec3 e1 = normalize(cross(cn, std::fabs(cn.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(cn, e1);
        Vec3 d = p - f.c;
        double ang = std::atan2(dot(d, e2), dot(d, e1));
        double rr = r * (0.8 + 0.2 * std::sin(3 * ang + f.u[0] * TAU) + 0.12 * std::sin(5 * ang + f.u[1] * TAU) + 0.08 * std::sin(7 * ang + f.u[2] * TAU));
        double x = f.d / rr;
        if (x > 1.35) return;
        uint64_t id = hash3i((int64_t)std::llround(f.c.x * 64), (int64_t)std::llround(f.c.y * 64), (int64_t)std::llround(f.c.z * 64), 0x1A4EULL);
        if (!lakeSiteOk(g, cn, id)) return;
        double depth = 9.0 + 10.0 * f.u[4];
        bowl = std::max(bowl, depth * smoothstep(1.35, 0.55, x));
    });
    return bowl;
}
}   // namespace
void applyDrainage(const BodyGen& g, const Vec3& unit, double detailM, double& h, double* hS, double slope, bool wet, bool lakesWet, double moist, DrainApply& A, double uShape) {
    (void)slope;
    DrainInfo D = drainageAt(g, unit, detailM, moist);
    double lakeM = D.lake > 0.3 ? smoothstep(0.3, 0.6, D.lake) : 0.0;
    A.wetness = D.wetness;
    if (D.valley > 0) {   // the valley: the ground melts toward its channel's bank height. O6-05: a glaciated valley has the U's flat floor
        double vz = D.valley * (1 - lakeM), bankV = D.valleyLevel + 1.0;
        if (uShape > 0) vz = std::max(vz, std::min(1.0, D.valley * 1.6) * uShape * (1 - lakeM));
        h += (bankV - h) * vz;
        if (hS) *hS += (bankV - *hS) * vz;
    }
    if (D.any) {
        double wh = D.halfWidth, bankH = D.level + 1.0;
        double zone = smoothstep(wh * 2.2, wh * 0.9, D.dist) * D.pres * (1 - lakeM);
        h += (bankH - h) * zone;
        if (lakeM < 0.5) {
            double depth = D.depth * D.pres;
            double v = clampd(1.0 / std::max(depth, 1e-3), 0.0, 1.0), tt = 0.5 - std::sin(std::asin(1.0 - 2.0 * v) / 3.0);
            double shoreM = D.dist - wh * (1.15 - 0.65 * tt);
            // every point that knows its channel measures the edge in the same metres (a corner beyond the plane
            // without an outline falling back on its height over a neighbour's plane reads a dip beside a steep river as wet:
            // a dark triangle on the bank); the bank stands over the plane out to 3.5 half-widths
            if (shoreM > 0) { A.shoreOut = std::min(A.shoreOut, shoreM); if (D.dist < wh * 3.5) A.bankFloor = std::max(A.bankFloor, D.level + 0.05 + 0.3 * clampd(shoreM / 10.0, 0, 1)); }
            if (D.dist < wh * 3.5) {
                double chan = smoothstep(wh * 1.15, wh * 0.5, D.dist) * D.pres;
                h -= chan * depth;
                A.bed = chan;
                if (zone > 0) { A.level = D.level; A.zone = zone; A.kind = 1; A.shore = shoreM; }
                A.bank = smoothstep(wh * 3.5, wh * 1.2, D.dist) * D.pres;
            }
        }
    }
    if (D.lake > 0.02) {
        // the lake reaches over the whole fade of the flood's cell mask (a cell's centre is flagged when it lies under the
        // fill; the true shore lies between the last flagged centre and the first unflagged one), and within it the water
        // is wherever the fine ground lies under the level: the mask's own 0.5 contour cut a lake 13 m deep off along a wall
        A.lake = lakeM;
        double lv = D.lakeLevel;
        double reach = smoothstep(0.02, 0.12, D.lake);
        if (wet && lakesWet) {
            // the height over the level in a lake's own metres (one scale for the whole lake, so the contour is the height's contour), or
            // the fade's outer edge
            double sh = std::max((h - lv) * 20.0, (0.06 - D.lake) * 400.0);
            A.level = lv; A.zone = std::max(A.zone, reach); A.kind = 2; A.shore = sh;
            if (sh > 0) A.bankFloor = std::max(A.bankFloor, lv + 0.05 + 0.3 * clampd(sh / 10.0, 0, 1));
        } else {
            double target = lv - 0.5;   // a playa: dead flat at the fill level, the ground under a metre over it melted down
            A.playa = reach * smoothstep(lv + 1.0, lv - 0.5, h);
            if (h < lv + 1.0) h += (target - h) * A.playa;
            if (hS && *hS < lv + 1.0) *hS += (target - *hS) * reach * smoothstep(lv + 1.0, lv - 0.5, *hS);
        }
    }
    if (wet) {   // the bank stands over its plane, the channel lies under it
        if (A.level > -1e8 && A.shore < 1e8 && A.shore <= 0) h = std::min(h, A.level - 0.3);
        else if (A.bankFloor > -1e8) h = std::max(h, A.bankFloor);
    }
}

void felisianSample(const BodyGen& g, const Vec3& unit, double detailM, SurfaceSample& s) {
    const double R = g.R;
    const KindDef& K = KINDS[g.type];
    Vec3 p = unit * R;
    double lat = std::asin(clampd(unit.z, -1, 1));
    double absLatDeg = std::fabs(lat) / DEG;
    FelGround o;
    felisianGround(g, unit, detailM, o);
    double h = o.h, hS = o.hS, moist = o.moist, landMask = o.landMask, relief0 = o.relief0, hills = o.hills;
    double sub = g.locked ? dot(unit, g.lockedDir) : 0.0;   // M5-08: the sub-stellar point's side (the glacial valleys below read the climate too)
    // seasons: the winter hemisphere's cap grows and its snow line drops
    double winter = clampd((lat >= 0 ? -g.season : g.season), 0, 1);   // 0..1 how deep in winter this hemisphere is
    double capEdge = g.iceCapLat + 6.0 * gnoise3(p / (R * 0.15), g.sF + 7) - 11.0 * winter + 4.0 * clampd((lat >= 0 ? g.season : -g.season), 0, 1);
    // the snow line falls with the square of the latitude (Earth: 5000 m at the equator, 3000 at 45, 1000 at 65,
    // nothing at 85); the old `40 m per degree` put every hill above 500 m under snow from 50 degrees on, and two
    // thirds of the felisian land came out as ice sheet
    double lf = absLatDeg / 85.0 * g.latGradient;   // a kind's climate contrast moves its snow line's fall with latitude too
    double snowLine = g.snowLine * std::max(0.0, 1.0 - lf * lf) - 900.0 * winter;
    // the slope's aspect: a flank facing the equator keeps its snow line up to 8% higher, a shaded one lower
    double steep = clampd(o.slopeM * (1 + 1.6 * o.cliff), 0, 4);   // the material chain's slope: the rock and the scree sit on the same cells in every ring
    {
        Vec3 pole(0, 0, 1);
        Vec3 north = pole - unit * unit.z;
        double nl = length(north);
        if (nl > 1e-6 && o.slopeM > 0.02) {
            Vec3 eq = north / nl * (lat >= 0 ? -1.0 : 1.0);   // toward the equator
            double facing = dot(normalize(o.gradM * -1.0), eq) * clampd(o.slopeM / 0.3, 0, 1);   // downhill toward the equator: sun-facing (B-404: the material chain's)
            snowLine *= 1.0 + 0.08 * facing;
        }
    }
    moist -= 0.12 * smoothstep(0.25, 0.5, o.slopeM) * smoothstep(0.3, 0.6, relief0);   // the ridges are the dry ground (the valley floors gained above); B-404: the material chain's slope
    // inland water. A river is a ribbon cut into the smooth ground along a hashed network (worley cell boundaries
    // at 80 km): across its floodplain the fine relief melts into `hS`, the channel is carved 2.5-11.5 m into it and the
    // water lies a metre under the smooth ground, flat across the ribbon and following the valley along it; the plane
    // extends under the banks (`s.water` is set over the whole zone) and the ground above it wins by depth (a channel a few
    // metres deep in the bumpy ground, the water a few metres over each vertex, would be a scatter of squares). Lakes and wetland pools are cut into the same smooth ground with one flat level each
    // shoreOut: the edge's distance beyond the plane's zone
    double level = -1e9, waterZone = 0, riverBank = 0, shore = 1e9, bankFloor = -1e9, shoreOut = 1e9, wetness = 0; int waterKind = 0; bool dryLake = false;
    if (landMask > 0.2 && hS > -60) {   // the drainage's rivers and lakes (the worley nets and the lake features of B-320 are gone)
        DrainApply A;
        // where the climate is cold the valleys are glacial: U-shaped, and, reaching the sea, fjords (the valley's floor
        // keeps going under the water)
        double uShape = smoothstep(2.0, -6.0, climateTempC(g, absLatDeg, std::max(0.0, hS), 0, sub)) * smoothstep(0.15, 0.4, relief0);
        double hBefore = h;
        applyDrainage(g, unit, detailM, h, &hS, o.slope, true, moist > 0.3, moist, A, uShape);   // a dry basin is a playa, not a lake
        if (uShape > 0.3 && hBefore < 5 && A.wetness > 0.2) { double fj = 45.0 * A.wetness * uShape * smoothstep(5.0, -10.0, hBefore); h -= fj; hS -= fj; }   // the fjord
        if (A.wetness > 0.2 && A.lake < 0.5) {
            // the floodplain's own relief, bars and levee scars of a metre on 90 and 30 m (a dead-flat floor read a 0.007 median grade over
            // the plains; the banks within 3.5 half-widths stay clean)
            double fm = smoothstep(0.2, 0.6, A.wetness) * (1 - A.lake) * (1 - A.bank) * landMask;
            auto ff = [&](double wlKm) { return smoothstep(1.5, 4.0, wlKm * 1000.0 / detailM); };   // full when the feature spans four samples
            double mr = 1.2 * gnoise3(p / 0.09 + Vec3(3.1, 1.4, 7.7), g.sB + 101) * ff(0.09) + 0.4 * gnoise3(p / 0.03 + Vec3(0.4, 6.2, 2.5), g.sB + 102) * ff(0.03);
            h += mr * fm;
        }
        dryLake = A.playa > 0.3;
        if (landMask > 0.35) { level = A.level; waterZone = A.zone; waterKind = A.kind; shore = A.shore; bankFloor = A.bankFloor; shoreOut = A.shoreOut; riverBank = A.bank; wetness = A.wetness; }
    }
    if (o.land.riftFloor > 0.3 && level < -1e8 && moist > 0.3 && hS > 2) {   // the rift floor's lakes, three metres deep
        level = hS - 1.0; waterZone = std::max(waterZone, o.land.riftFloor); waterKind = 4;
    }
    if (landMask > 0.5 && level < -1e8 && moist > 0.7 && hS < 120 && hS > 1 && relief0 < 0.2 && hills < -0.15) {   // wetland pools
        double pool = smoothstep(-0.15, -0.4, hills);
        h += (hS - 0.5 - 1.5 * pool - h) * pool;
        level = hS + 0.2; waterZone = std::max(waterZone, pool); waterKind = 3;
    }
    moist += 0.25 * riverBank + 0.15 * wetness;   // the valley floors are the wetter ground
    // a river's or a lake's bank stands over its plane (out to 3.5 half-widths or 1.6 radii, `bankFloor`) and the
    // channel lies under it, so the water's edge is the outline (`shore`) and nothing else: where the fine relief of the outer
    // floodplain dipped under the plane, the plane would show through it in flat patches a hundred metres wide with edges
    // snapped to the mesh, and again as a moat of cells along the plain's edge
    if (level > -1e8 && shore < 1e8 && shore <= 0) h = std::min(h, level - 0.3);
    else if (bankFloor > -1e8) h = std::max(h, bankFloor);
    double reefTint = 0;
    if (h < 0 && g.hasTrait(TR_REEFS) && absLatDeg < capEdge) {
        // a barrier reef in the warm seas (the Great Barrier Reef's): along the shelf's edge where it falls past 18-40 m (a contour, so every
        // ring and the globe agree, B-404) its crest stands awash, the fore-reef dropping seaward under it, a few passes cut through it; behind it
        // the lagoon's floor of white sand lifted to a few metres, its water turquoise (`lifeTint`); in a province of the coasts where the sea is
        // warm enough (22 C at its level)
        const double warm = smoothstep(20.0, 23.0, climateTempC(g, absLatDeg, 0, 0, sub)) * smoothstep(-0.2, 0.15, fbm3(p / (R * 0.22) + Vec3(13.1, 3.7, 9.2), g.sG + 320, 2));
        if (warm > 0.5) {
            const double D = 18.0 + 22.0 * unitFromHash(mix64(g.seed ^ 0x2EEFULL));
            if (h > -D) { h = -(0.8 + 5.0 * (-h / D)); reefTint = 1; }
            // the crest's shallows turquoise, the fore-reef's drop the deep's
            else if (h > -D - 30.0 && fbm3(p / 1.8, g.sG + 321, 2) > -0.3) { const double k = smoothstep(-D - 30.0, -D - 8.0, h); h += (-0.35 - h) * k; reefTint = h > -4.0 ? 1.0 : 0.0; }
        }
    }
    s.height = h;
    s.relief = clampd(relief0 + 0.5 * o.slope, 0, 1);
    // biomes from temperature (latitude, altitude, the body's climate) and moisture
    double T = climateTempC(g, absLatDeg, h, winter, sub);
    if (g.locked && !(K.tags & KT_FIXED_SUN)) moist -= 0.35 * std::max(0.0, sub - 0.15);   // deserts under a permanent sun (P-03: a fixed sun's kind has its own, `felisianGround`)
    int biome = BIO_GRASSLAND;
    if (T < -8) biome = BIO_ICE;
    else if (T < 2) biome = BIO_TUNDRA;
    else if (T < 10) biome = moist > 0.45 + g.grassBand ? BIO_TAIGA : BIO_TUNDRA;   // a kind's grass band wider (forest wetter, desert drier)
    else if (T < 20) biome = moist > 0.55 + g.grassBand ? BIO_TEMPERATE : (moist > 0.28 - g.grassBand ? BIO_GRASSLAND : BIO_DESERT);
    else biome = moist > 0.55 + g.grassBand ? BIO_TROPICAL : (moist > 0.35 - g.grassBand ? BIO_SAVANNA : BIO_DESERT);
    if (moist > 0.72 && h < 120 && relief0 < 0.2 && biome != BIO_ICE) biome = BIO_WETLAND;
    if (relief0 > 0.45 && h > std::max(700.0, 0.5 * snowLine)) biome = BIO_ALPINE;   // the zone between the treeline and the snow (it began at 700 m everywhere: 13% of the land was bare rock)
    bool inland = level > -1e8 && h < level;   // under a river, a lake or a pool
    if (h < 0) {
        s.material = MAT_WATER; s.water = 0; s.biome = BIO_OCEAN;
        s.albedo = h > -250 ? 0.36 : 0.22;
        s.lifeTint = reefTint;   // a lagoon's or a reef's
        if (g.seaDepth < 1 && h > -40 && h < -1.5) s.albedo += 0.12 * smoothstep(-40, -12, h) * smoothstep(-1.5, -4, h);   // the reefs fringing a shelf sea's coasts, pale under the water
        if (absLatDeg > capEdge + 4 || (g.locked && T < -10)) { s.material = MAT_ICE; s.albedo = 0.85; s.height = 0; s.biome = BIO_ICE; }
        else if (K.daySeasDry && g.locked) {   // the seas boiled away where the day is too hot to hold them (the climate at the sea's level),
            // their floors dry: salt in the deeps, sand on the old shelves
            const double dry = smoothstep(30.0, 36.0, climateTempC(g, absLatDeg, 0, 0, sub) + 3.0 * hills);
            if (dry > 0.5) { s.material = h < -120.0 ? MAT_SALT : MAT_SAND; s.albedo = s.material == MAT_SALT ? 0.80 : 0.60; s.water = -1e9; s.biome = BIO_DESERT; }
        }
        if (K.seaGlaciers) {   // a snowball's frozen seas: glacier ice standing 6-14 m over the water at its edge and rising poleward, its seams
            // ridged, wind-scoured to blue ice in patches; before its edge a band of floes three degrees wide on the slushy sea
            const double over = absLatDeg - (capEdge + 4);
            if (over > 0) {
                const double wallDeg = 30.0 / (DEG * R * 1000.0), cliff = 6.0 + 8.0 * (0.5 + 0.5 * gnoise3(p / 2.0, g.sF + 73));
                double hg = (cliff + 0.6 * std::min(over, 40.0)) * smoothstep(0.0, wallDeg, over);
                if (scaleVisible(1.5, detailM)) { const Worley3 fl = worley3(p / 1.5, g.sF + 3); hg += 3.0 * smoothstep(0.03, 0.0, fl.f2 - fl.f1) * lodFade(1.5, detailM); }   // pressure ridges
                hg += 0.6 * fbm3(p / 0.12, g.sF + 72, 2) * lodFade(0.12, detailM);   // sastrugi
                s.height = hg; s.biome = BIO_ICE;
                const bool blue = fbm3(p / 0.9, g.sF + 74, 2) > 0.38;   // unfaded: the same patches in every ring
                s.material = blue ? MAT_ICE : MAT_SNOW; s.albedo = blue ? 0.62 : 0.88;
            } else if (over > -3.0) {   // the floes: the ocean's plates on a shelf under the slush (the water's edge the plane's contour on the plates)
                const double cover = smoothstep(-3.0, 0.0, over), shelf = smoothstep(0.0, 0.04, cover);
                const double floor = h + (-6.5 - h) * shelf;
                s.height = floor;
                const Worley3 fl = worley3(p / 1.5, g.sF + 3);
                if (unitFromHash(fl.id1) < cover) {
                    const double d = fl.f2 - fl.f1, rim = smoothstep(0.0, 0.13, d), top = 0.9 + 0.35 * smoothstep(0.13, 0.3, d);
                    const double hf = floor + (top - floor) * rim;
                    s.height = hf;
                    if (hf > -0.3) { s.material = MAT_ICE; s.albedo = 0.86; s.biome = BIO_ICE; }
                }
            }
        }
    } else if (inland) {
        s.material = T < -2 ? MAT_ICE : MAT_WATER; s.water = level; s.albedo = 0.3; s.biome = (biome == BIO_ICE || biome == BIO_TUNDRA) ? biome : BIO_WETLAND;
        if (T < -2) { s.height = level; s.albedo = 0.8; }
    } else if (absLatDeg > capEdge || h > snowLine || biome == BIO_ICE || o.iceSheet > 1) {   // an ice sheet's ground is its ice whatever the season
        s.material = MAT_SNOW; s.albedo = 0.92; s.biome = BIO_ICE;
        if (o.iceSheet > 1 && o.sheetWall) { s.material = MAT_ICE; s.albedo = 0.7; }   // the sheet's wall bare blue ice (P-03: its own wall, not the bedrock's slope under it)
        if (waterZone > 0) s.water = level;
    } else if (o.caldera && h > 0) {
        s.material = MAT_WATER; s.albedo = 0.3; s.water = h + 2.0; s.biome = BIO_WETLAND;   // crater lake
    } else if (biome == BIO_ALPINE) {
        s.material = MAT_ROCK; s.albedo = 0.48; s.veg = 0.05; s.biome = BIO_ALPINE;
    } else if (waterZone > 0.3 && h < level + 1.2 && T > -2) {   // the bars and beaches of inland water
        s.material = MAT_SAND; s.albedo = 0.58; s.veg = 0.05; s.biome = biome; s.water = level;
    } else if (h < 12 + 10 * hills) {
        s.material = MAT_SAND; s.albedo = 0.62; s.veg = 0.08; s.biome = biome;
    } else switch (biome) {
        case BIO_DESERT: s.material = MAT_SAND; s.albedo = 0.66; s.veg = 0.02; break;
        case BIO_TROPICAL: s.material = MAT_FOREST; s.albedo = 0.28; s.veg = 0.85 + 0.15 * clampd((moist - 0.55) / 0.3, 0, 1); break;
        case BIO_TEMPERATE: s.material = MAT_FOREST; s.albedo = 0.31; s.veg = 0.6 + 0.3 * clampd((moist - 0.55) / 0.3, 0, 1); break;
        case BIO_TAIGA: s.material = MAT_FOREST; s.albedo = 0.34; s.veg = 0.45 + 0.2 * clampd((moist - 0.45) / 0.3, 0, 1); break;
        case BIO_SAVANNA: s.material = MAT_GRASS; s.albedo = 0.46; s.veg = 0.3; break;
        case BIO_WETLAND: s.material = MAT_GRASS; s.albedo = 0.38; s.veg = 0.5; break;
        case BIO_TUNDRA: s.material = (0.5 + 0.5 * hills) * 0.6 + 0.4 * winter > 0.62 ? MAT_SNOW : MAT_GRASS; s.albedo = s.material == MAT_SNOW ? 0.9 : 0.4; s.veg = 0.1; break;
        default: s.material = MAT_GRASS; s.albedo = 0.42; s.veg = 0.25 + 0.3 * clampd((moist - 0.3) / 0.3, 0, 1); break;
    }
    if (s.material != MAT_WATER && s.material != MAT_ICE && waterZone > 0 && s.water < -1e8) s.water = level;   // the plane under the banks
    if (s.biome == BIO_NONE) s.biome = biome;
    // the landforms' materials: glacier ice, and rock, basalt, sand or salt where a landform says so (never over water)
    if (dryLake && s.material != MAT_WATER && s.material != MAT_SNOW) { s.material = MAT_SALT; s.albedo = 0.86; s.veg = 0; }   // a dry basin's playa
    if (o.land.glacier > 0.5 && s.material != MAT_WATER) { s.material = MAT_ICE; s.albedo = 0.8; s.veg = 0;
    } else if (o.land.mat >= 0 && s.material != MAT_WATER && s.material != MAT_ICE && s.material != MAT_SNOW) {
        s.material = o.land.mat;
        if (o.land.albedo >= 0) s.albedo = o.land.albedo;
        s.veg *= (o.land.mat == MAT_SAND ? 0.3 : 0.0);
    }
    // bare rock where the ground is too steep to hold soil (over 1:1.6, and every riser of the cliff bands), scree on
    // the flanks under it and on the talus: boulder fields, next to no flora
    if (s.material != MAT_WATER && s.material != MAT_ICE && s.material != MAT_SNOW && s.material != MAT_SALT && h > 12 && !inland) {
        if (steep > 0.62 || o.cliff > 0.4) { s.material = MAT_ROCK; s.albedo = 0.46 - 0.1 * o.cliff; s.veg = 0.03; }
        s.scree = clampd(std::max(o.talus, smoothstep(0.42, 0.62, steep) * 0.7), 0, 1);
        if (s.scree > 0.3) s.veg *= 1 - 0.7 * s.scree;
    }
    if (!(K.tags & (KT_FLORA | KT_MICROBIAL)) && K.cover == DC_NONE && (s.material == MAT_FOREST || s.material == MAT_GRASS)) {   // a living ground with nothing
        // growing on it (the snowball's): where the plants would stand, frost-shattered gravel where it is wet enough, bare rock where not
        s.material = moist > 0.4 ? MAT_SAND : MAT_ROCK; s.albedo = s.material == MAT_SAND ? 0.5 : 0.44; s.veg = 0;
    }
    if (K.nunataks && o.iceSheet <= 1 && s.material == MAT_SNOW && steep > 0.45 && h > 0) { s.material = MAT_ROCK; s.albedo = 0.34; }   // the nunataks' flanks scoured bare
    if (K.tags & KT_MICROBIAL) {   // life without plants (the archean). Where a felisian world grows its plants the wetter
        // ground is mats (the land's colour from afar), the drier bare: regolith where it is wet enough to weather, rock where not; the flats
        // along the shore a metre or two over the sea and the bars of the rivers and lakes are mats, and along the shore a reef of
        // stromatolites in patches a few hundred metres long
        const bool plant = s.material == MAT_FOREST || s.material == MAT_GRASS;
        const bool flat = s.material == MAT_SAND && h >= 0 && (h < 3.0 + 1.5 * hills || (waterZone > 0.3 && s.water > -1e8 && h < s.water + 1.2));
        if (plant && (s.biome == BIO_WETLAND || riverBank > 0.35 || (moist > 0.5 && h < 900 && relief0 < 0.45))) { s.material = MAT_MATS; s.albedo = 0.22 + 0.04 * hills; }   // the wet land, its banks
        else if (plant) { s.material = moist > 0.4 && s.biome != BIO_ALPINE ? MAT_DUST : MAT_ROCK; s.albedo = s.material == MAT_DUST ? 0.38 : 0.42; }
        else if (flat && T > 0) {
            const double reef = waterZone > 0.3 ? -1.0 : fbm3(p / 0.22, g.sG + 77, 2);   // the reefs on the sea's flats, by an unfaded field (the same in every ring)
            if (reef > 0.08) { s.material = MAT_STROMATOLITE; s.albedo = 0.40; } else { s.material = MAT_MATS; s.albedo = 0.24; }
        }
        s.veg = 0;
    }
    if (s.material != MAT_WATER) s.albedo += 0.08 * clampd(h / 4000.0, 0, 1);
    s.landform = 1000 * o.land.mark + std::fabs(o.land.dhSmooth) + std::fabs(o.land.dhFine) + 40 * o.land.flatten;
    s.landformTrait = o.land.mark >= 0.5 ? o.land.markTrait : 0;
    s.slipFace = std::max(o.slip, o.land.slip);
    s.groundTint = o.land.tint;
    if (o.land.life > 0 && s.material != MAT_WATER) s.lifeTint = o.land.life;
    if (s.water > 0.5 && waterZone > 0) s.waterKind = waterKind;
    if (waterZone > 0 && s.water > -1e8) s.shore = shore;   // (a snowfield or a glacier over the zone keeps the plane, so it keeps the edge too)
    else if (s.water < -1e8 && shoreOut < 1e8) s.shore = shoreOut;   // the bank beyond the plane's reach still knows how far the water is
    if (K.cover != DC_NONE) {   // a dead world, the present's cover over the living ground it was (above was its then-world's: the land's heights are
        // the same in both times, so its people's towns stand where they stood). No liquid is left; the old forests' density stays in `veg` (their dead trees)
        const double oldForest = s.material == MAT_FOREST ? s.veg : 0.0;
        const int was = s.material;
        s.water = -1e9; s.waterKind = 0; s.shore = 1e9; s.veg = 0;
        if (K.cover == DC_SCORCHED) {   // baked by its swelling sun: the seas boiled off to their salt, the soils cracked clay and dust, the forests stone; a
            // cold one's (a white dwarf's, a giant's far world) frost where its present's mean falls under freezing
            const double tNow = g.nowTempC - 0.4 * absLatDeg - 6.5 * std::max(0.0, h) / 1000.0 + (g.locked ? 25.0 * sub : 0.0);   // a locked one's night side the colder
            s.biome = BIO_DESERT;
            if (h < 0) {   // the old sea's floor, dry at its depth far under the shores: the deep basins' salt, the slopes' silt, reef limestone on the warm shelves
                s.height = h; s.oldSea = 1;
                const double depth = -h;
                if (depth > 1500.0 + 600.0 * fbm3(p / 60.0, g.sG + 82, 2)) { s.material = MAT_SALT; s.albedo = 0.84;
                } else if (depth < 110.0 && climateTempC(g, absLatDeg, 0, 0, sub) > 17.0 &&
                        fbm3(p / 120.0, g.sG + 84, 2) + (matVisible(5.0, detailM) ? 0.5 * fbm3(p / 9.0, g.sG + 81, 2) : 0.0) + (matVisible(0.35, detailM) ? 0.3 * fbm3(p / 0.7, g.sG + 83, 2) : 0.0) >
                                -0.15) {
                    s.material = MAT_STROMATOLITE; s.albedo = 0.66;   // the reefs' fields, their edges where the eye sees them
                } else {
                    s.material = MAT_DUST;
                    s.albedo = 0.42 + 0.08 * smoothstep(1500.0, 200.0, depth);
                }
            } else if (inland) {   // the rivers' beds sand and gravel, the lakes' playas salt, the pools' clay
                s.height = h;
                if (waterKind == 2 || waterKind == 4) { s.material = MAT_SALT; s.albedo = 0.8; }
                else if (waterKind == 3) { s.material = MAT_MUD; s.albedo = 0.46; }
                else { s.material = MAT_SAND; s.albedo = 0.56; }
            } else switch (was) {
                case MAT_FOREST: s.material = MAT_DUST; s.albedo = 0.46; s.veg = oldForest; break;   // the stone forest's floor (its trunks the trees')
                case MAT_GRASS: s.material = moist > 0.38 ? MAT_MUD : MAT_DUST; s.albedo = s.material == MAT_MUD ? 0.46 : 0.5; break;   // the soils cracked, the wet ones clay
                case MAT_SNOW: case MAT_ICE: s.material = steep > 0.4 ? MAT_ROCK : MAT_DUST; s.albedo = s.material == MAT_ROCK ? 0.4 : 0.48; break;   // the old ice's ground, moraine
                case MAT_WATER: s.material = MAT_SALT; s.albedo = 0.8; break;   // a crater lake's
                default: break;
            }
            if (tNow < -2.0 + 4.0 * hills && (s.material == MAT_SALT || s.material == MAT_DUST || s.material == MAT_MUD || s.material == MAT_SAND)) { s.material = MAT_SNOW; s.albedo = 0.86; }
        } else {   // lost its sun: the seas ice at their level (pressure ridges along the floes' seams, snow drifted over them), the rivers and lakes ice,
            // the land under the frost of its fallen air (thicker toward the poles), bare dark rock on the steep, the dead soil where the frost lies thin
            const double polar = smoothstep(20.0, 70.0, absLatDeg);
            s.biome = BIO_ICE;
            if (h < 0) {
                double hi = 0.3 * fbm3(p / 0.15, g.sF + 72, 2) * lodFade(0.15, detailM);
                if (scaleVisible(1.5, detailM)) { const Worley3 fl = worley3(p / 1.5, g.sF + 3); hi += 2.5 * smoothstep(0.03, 0.0, fl.f2 - fl.f1) * lodFade(1.5, detailM); }
                s.height = hi; s.oldSea = 1;
                const double drifts =
                        fbm3(p / 250.0, g.sF + 79, 2) + (matVisible(12.0, detailM) ? 0.6 * fbm3(p / 25.0, g.sF + 75, 2) : 0.0) + (matVisible(0.4, detailM) ? 0.35 * fbm3(p / 0.8, g.sF + 77, 2) : 0.0);
                const bool drift = drifts + 0.5 * polar > 0.32;   // the drifts' fields of hundreds of kilometres, their edges where a ring can draw them (the map's texels are wider)
                s.material = drift ? MAT_SNOW : MAT_ICE; s.albedo = drift ? 0.86 : 0.6;
            } else if (inland) { s.height = level; s.material = MAT_ICE; s.albedo = 0.6; }
            else if (steep > 0.62 || o.cliff > 0.4) { s.material = MAT_ROCK; s.albedo = 0.3; }
            else if (fbm3(p / 400.0, g.sF + 80, 2) + (matVisible(20.0, detailM) ? 0.5 * fbm3(p / 40.0, g.sF + 76, 2) : 0.0) + (matVisible(1.2, detailM) ? 0.3 * fbm3(p / 2.5, g.sF + 78, 2) : 0.0)
                     - 0.7 * polar - 0.25 * smoothstep(300.0, 1500.0, h) > 0.32) {   // thin frost (its fields as the drifts')
                s.material = MAT_DUST; s.albedo = 0.3; s.veg = oldForest;
            }
            else { s.material = MAT_SNOW; s.albedo = 0.88; s.veg = oldForest; }
        }
    }
}
