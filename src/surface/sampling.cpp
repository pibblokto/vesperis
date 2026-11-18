// Z-03: the search (docs/ideas/IDEAS-small-and-spacious.md section 1.7): what a vial takes at a place of the ground (`readSample`: its
// microhabitat, its name, the temperature read) and the living films under the lamp's ultraviolet (`drawFilmGlow`). A microhabitat is read
// from what is fixed to the world (the planet function at the square metre, the springs' and vents' finders, the ruins' walls), never from
// what the landing lays (the rocks, the domes, the trees are dealt on its own grid), so the same place gives the same sample from any landing
#include "surface/surface_view.h"
#include "galaxy/planet_features.h"
#include "core/parallel.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

namespace {
// the world's liquid by its life's chemistry (an exotic sea's own where the world has one)
const char* liquidName(int chem, bool exotic) {
    if (exotic) return "LIQUID";
    switch (chem) {
        case CH_ACID: return "ACID";
        case CH_AMMONIA: return "AMMONIA WATER";
        case CH_HYDROCARBON: return "METHANE";
        default: return "WATER";
    }
}
const char* waterPlace(int waterKind) {
    switch (waterKind) {
        case 1: return "A RIVER";
        case 2: return "A LAKE";
        case 3: return "A POOL";
        case 4: return "A RIFT LAKE";
        default: return "THE SEA";
    }
}
bool rockFamily(int m) { return m == MAT_ROCK || m == MAT_BASALT || m == MAT_METAL || m == MAT_GRAPHITE || m == MAT_GLASS || m == MAT_QUARTZ; }
}   // namespace

// the sea's level here now (its tide), -1e9 on a world with none: a beach's wet edge is within 0.6 m of it (its vertices carry no water)
double SurfaceView::seaLevelNow() const {
    if (kindHas(site.gen.type, KT_SEA)) return site.tideM;
    return site.gen.liquidLevel > -1e8 ? site.gen.liquidLevel + site.tideM : -1e9;
}

const Biota& SurfaceView::biota() const {
    static const Biota none;
    if (!life || site.body < 0 || site.body >= (int)life->bodies.size()) return none;
    return life->bodies[site.body];
}

// the site's own formula (`computeEnvironment`) with the year's season at its mean and the day's swing averaged out, at the place's height
// and latitude: a locked world's sun where it stands over the place (its substellar point's dot), an airless world's day and night halves
double SurfaceView::placeMeanC(double x, double z, double h) const {
    const Body& b = site.sys->bodies[site.body];
    const KindDef& K = KINDS[b.type];
    const Vec3 u = site.unitAt(x, z);
    const double absLat = std::fabs(std::asin(clampd(u.z, -1, 1))) / DEG;
    if (site.sys->sunless()) return b.tempK - 273.15 - (site.atmosphere ? 6.5 * h / 1000.0 : 0.0);
    const double sinAlt = site.gen.locked ? dot(u, site.gen.lockedDir) : 0.0;
    double base = b.tempK - 273.15;
    if (livingGround(b.type)) base = climateTempC(site.gen, absLat, h, 0, sinAlt) - 3 + K.daySwing * sinAlt;
    else if (site.atmosphere && K.reachClimate && site.gen.locked) base = K.tempOffset + K.tempSwing * (sinAlt - std::cos(site.gen.seaReachDeg * DEG));
    else if (site.atmosphere) base = base * 0.75 + K.tempOffset + K.tempSwing * sinAlt - 0.4 * absLat;
    else base = base * 1.3 - 60 + (site.gen.locked ? (sinAlt > 0 ? 110 * sinAlt : -70) : 35 * std::cos(absLat * DEG) - 35) - 0.5 * absLat;
    if (!livingGround(b.type)) base -= 6.5 * (h / 1000.0);
    base += K.tempAdd;
    if (K.nightSideC > -1e8 && site.gen.locked) base = K.nightSideC + (base - K.nightSideC) * smoothstep(-0.12, 0.08, sinAlt);
    return std::max(base, -270.0);
}

SampleRead SurfaceView::readSample(double x, double z, bool onWater) {
    SampleRead R;
    const Body& b = site.sys->bodies[site.body];
    const Biota& B = biota();
    const TerrainVertex v = site.sampleAt(x, z, 1.0);
    R.unit = site.unitAt(x, z);
    const int waterKind = sampleSurface(site.gen, R.unit, 1.0).waterKind;   // (the vertex keeps no kind of water)
    Microhabitat& H = R.hab;
    H.place = placeKey(b.seed, R.unit, b.radiusKm);
    H.meanC = placeMeanC(x, z, v.h);
    H.oldRock = B.second == LS_FOSSIL || B.stage == LS_FOSSIL;
    const int chem = B.chemistry != CH_NONE ? B.chemistry : BIOTA[b.type].chemistry;
    const bool exotic = site.gen.hasTrait(TR_EXOTIC_SEAS);
    const double readC = env.temperatureC;
    auto livesIn = [&](int n) { for (const Microbe& m : B.species) if (m.state == SS_LIVING && (m.niches & nicheBit(n))) return true; return false; };
    auto put = [&](int niche, const std::string& name, double meanC, double readingC) {
        H.niche = niche; H.meanC = meanC; R.name = name; R.readingC = readingC; R.ok = true;
        // mats no species of the world's lives in are its springs' life spreading (a vent world's on its hot ground): the springs' outflow
        if (niche == NI_MAT && !livesIn(NI_MAT) && livesIn(NI_SPRING)) { H.niche = NI_SPRING; if (H.meanC > 60) H.meanC = 45; }
    };
    if (v.material == MAT_LAVA) { R.refusal = "THE GROUND IS MOLTEN HERE"; return R; }
    // a world whose life is seen on its ground lives whatever its mean (rules-life.md, Z-01): the visible life's ground (its mats, soil, crusts,
    // wet edges) no hotter than the biota's limits were drawn round (45 C and a span); its square metre's temperature to a tenth of a degree
    const bool seen = livingGround(b.type) || kindHas(b.type, KT_FLORA | KT_FAUNA | KT_MICROBIAL | KT_VENT_LIFE);
    struct Tenth {
        Microhabitat& h; bool seen;
        ~Tenth() {
            if (seen && (h.niche == NI_MAT || h.niche == NI_SOIL || h.niche == NI_CRUST || h.niche == NI_SILT) && h.meanC > 50) h.meanC = 50;
            h.meanC = std::round(h.meanC * 10) / 10;
        }
    } tenth{H, seen};
    // out of the sun the year round: a slope turned to the pole, a cliff's foot (the planet function's own heights, a metre and a half each way)
    {
        const double hn = site.sampleAt(x, z + 1.5, 2.0).h, hs = site.sampleAt(x, z - 1.5, 2.0).h;
        const double northward = -(hn - hs) / 3.0;   // the normal's lean to the north (a slope falling to the north)
        const bool north = std::asin(clampd(R.unit.z, -1, 1)) >= 0;
        H.shade = (north ? northward : -northward) > 0.45 || v.scree > 128;
    }
    const bool hasWater = v.water > -1e8f;
    const bool inLiquid = onWater || (hasWater && (v.material == MAT_WATER || v.h < v.water - 0.05));
    const double freezeC = chem == CH_HYDROCARBON ? -183 : (chem == CH_AMMONIA ? -97 : (v.wtint > 0 || H.salt >= 1 ? -2 : 0));
    // the springs and the vents, by their finders (the ground's material says the feature is there)
    {
        double waterC = 0;
        const int part = springPartAt(site.gen, R.unit, waterC);
        const bool springGround = v.material == MAT_BRINE || v.material == MAT_SALT || v.material == MAT_MATS;
        if (part != SPP_NONE && springGround && (part != SPP_RUNOFF || v.material == MAT_MATS)) {
            static const char* const MAT_BAND[5] = {"CLEAR POOL", "YELLOW MAT", "ORANGE MAT", "BROWN MAT", "BROWN MAT"};
            if (part == SPP_VENT) put(NI_SPRING, "WATER FROM A SPRING'S VENT", waterC, waterC);
            else if (part == SPP_SINTER) put(NI_SPRING, "SINTER AT A SPRING'S RIM", waterC - 4, waterC - 4);
            else if (part == SPP_POOL) {
                const int band = springBand(waterC);
                H.bloom = band != 0;
                put(band == 0 ? NI_SPRING : NI_MAT, band == 0 ? std::string("CLEAR POOL IN A SPRING") : std::string(MAT_BAND[band]) + " IN A SPRING'S POOL", waterC, waterC);
                if (band != 0 && !(B.habitats & nicheBit(NI_MAT))) H.niche = NI_SPRING;   // a world with no mats of its own: the spring's
            } else put(NI_SPRING, "MAT IN A SPRING'S RUNOFF", 0.5 * (waterC + H.meanC), 0.5 * (waterC + readC));
            return R;
        }
        if (KINDS[b.type].events & EV_SPRINGS) {   // the chemotroph's hot pools and their pale mats (its chimneys are the landing's: their sinter is the pool's)
            std::vector<GeyserVent> vents;
            springVents(site.gen, R.unit, vents);
            for (const GeyserVent& gv : vents) {
                const double dKm = length(R.unit - gv.unit) * b.radiusKm;
                const double poolC = 55 + 40 * hash01(mix64(gv.id ^ 0x5A3E1ULL));
                if (dKm < gv.radiusKm && (v.material == MAT_BRINE || inLiquid)) { put(NI_SPRING, "WATER FROM A HOT POOL", poolC, poolC); return R; }
                if (dKm < 2.5 * gv.radiusKm && (v.material == MAT_MATS || v.material == MAT_SALT)) {
                    const double c = poolC - 30 * (dKm / gv.radiusKm - 1) / 1.5;
                    H.bloom = v.material == MAT_MATS;
                    put(v.material == MAT_MATS ? NI_MAT : NI_VENT, v.material == MAT_MATS ? "PALE MAT ROUND A HOT POOL" : "SINTER ROUND A HOT POOL", c, c);
                    if (!(B.habitats & nicheBit(H.niche))) H.niche = NI_VENT;
                    return R;
                }
            }
        }
        if (site.gen.hasTrait(TR_GEYSERS) && v.material == MAT_SALT) {
            std::vector<GeyserVent> vents;
            geyserVents(site.gen, R.unit, vents);
            for (const GeyserVent& gv : vents)
                if (length(R.unit - gv.unit) * b.radiusKm < 1.3 * gv.radiusKm) { put(NI_SPRING, "SINTER BY A GEYSER", 70, std::max(readC, 40.0)); return R; }
        }
    }
    // the plumes' fall on their worlds' ice (the cryovolcanic's vents, the tiger stripes' curtains)
    if ((B.habitats & nicheBit(NI_PLUME)) && (v.material == MAT_SNOW || v.material == MAT_ICE)) {
        std::vector<GeyserVent> vents;
        const uint32_t ev = KINDS[b.type].events;
        if (ev & EV_PLUMES) plumeVents(site.gen, R.unit, vents);
        else if (ev & EV_CURTAINS) curtainVents(site.gen, R.unit, vents);
        for (const GeyserVent& gv : vents)
            if (length(R.unit - gv.unit) * b.radiusKm < std::max(3.0, (ev & EV_PLUMES) ? gv.radiusKm : 3.0)) {
                put(NI_PLUME, "FROST FROM A PLUME'S FALL", H.meanC, std::min(readC, 0.0)); return R;
            }
    }
    // a dead people's walls: their foot (the settlements stand where the world put them, whatever the landing)
    if (B.habitats & nicheBit(NI_RUINS)) {
        std::vector<Collider> walls;
        forNearbyRuins(x, z, 1, [&](const Ruin& r, const RuinCell& cell, int) { settlementColliders(r, cell, x, z, walls); });
        for (const Collider& c : walls)
            if (std::hypot(c.x - x, c.z - z) < c.r + 1.2) { put(NI_RUINS, "OLD WALL", H.meanC, readC); return R; }
    }
    // the liquid and its edge
    if (inLiquid) {
        const std::string what = std::string(liquidName(chem, exotic && waterKind == 0)) + " FROM " + waterPlace(waterKind);
        H.salt = v.material == MAT_BRINE || (exotic && chem == CH_WATER) ? 2 : (waterKind == 0 ? 1 : 0);
        if (site.gen.hasTrait(TR_PINK_LAKES) && v.wtint > 127) H.salt = 2;
        put(NI_LIQUID, what, std::max(H.meanC, freezeC), std::max(readC, freezeC));
        return R;
    }
    const double sea = seaLevelNow();
    const bool shoreMat = v.material == MAT_SAND || v.material == MAT_MUD || v.material == MAT_DUST || v.material == MAT_GRASS || v.material == MAT_ROCK;
    const bool edge = shoreMat && ((hasWater && v.h < v.water + 0.6) || std::fabs(v.h - sea) < 0.6);
    if (edge) {
        H.salt = waterKind == 0 ? 1 : 0;
        const char* what = waterKind == 0 ? "WET SAND AT THE SEA'S EDGE" : (waterKind == 1 ? "MUD ON A RIVER'S BANK" : (waterKind == 3 ? "WET SILT AT A POOL'S EDGE" : "WET SILT AT A LAKE'S EDGE"));
        put(NI_SILT, what, H.meanC, readC);
        return R;
    }
    // the ground's own
    switch (v.material) {
        // the snow's and the ice's life lives in the summer's melt (its films at 0 C, Hoham and Remias 2020): their mean the year's, warmed by the summer
        case MAT_SNOW: H.bloom = v.stint > 127; put(NI_SNOW, H.bloom ? "RED SNOW" : "SNOW", clampd(H.meanC + 8, -40, 0), std::min(readC, 0.0)); break;
        case MAT_ICE: put(NI_ICE, hasWater && waterKind == 0 ? "SEA ICE" : "ICE", clampd(H.meanC + 8, -40, 0), std::min(readC, 0.0)); break;
        case MAT_SALT: case MAT_POTASH: H.salt = 2; put(NI_SALT, "SALT CRUST", H.meanC, readC); break;
        case MAT_BRINE: H.salt = 2; put(NI_SALT, "BRINE FROM A POOL", std::max(H.meanC, -10.0), std::max(readC, -10.0)); break;
        case MAT_MATS: H.bloom = true; put(NI_MAT, kindHas(b.type, KT_VENT_LIFE) ? "PALE MAT ON THE SINTER" : "MICROBIAL MAT ON THE WET FLAT", H.meanC, readC); break;
        case MAT_STROMATOLITE: put(NI_MAT, "STROMATOLITE REEF FLAT", H.meanC, readC); break;
        case MAT_CRUST: put(NI_CRUST, kindHas(b.type, KT_FLORA) || b.type == PT_LICHEN ? "LICHEN CRUST ON STONE" : "CRUST ON THE DRY GROUND", H.meanC, readC); break;
        case MAT_MOSS: put(NI_CRUST, "MOSS IN A WET HOLLOW", H.meanC, readC); break;
        case MAT_FOREST: H.shade = true; put(NI_SOIL, "SOIL UNDER THE TREES", H.meanC, readC); break;
        case MAT_GRASS: put(NI_SOIL, "SOIL UNDER THE GRASS", H.meanC, readC); break;
        case MAT_MUD: put(NI_SILT, "MUD", H.meanC, readC); break;
        case MAT_SAND: put(B.habitats & nicheBit(NI_SOIL) ? NI_SOIL : NI_DUST, "SAND", H.meanC, readC); break;
        case MAT_SULPHUR: put(NI_DUST, "SULPHUR", H.meanC, readC); break;
        case MAT_DUST: put(NI_DUST, site.atmosphere ? (H.shade ? "DUST IN SHADE" : "DUST") : (H.shade ? "REGOLITH IN SHADE" : "REGOLITH"), H.meanC, readC); break;
        default:
            if (b.type == PT_QUARTZ && v.material == MAT_QUARTZ) put(NI_ROCK, "UNDER A TRANSLUCENT STONE", H.meanC, readC);   // hypoliths (Pointing et al. 2009)
            else if (rockFamily(v.material)) put(NI_ROCK, H.shade ? "ROCK IN SHADE" : "WEATHERED ROCK", H.meanC, readC);
            else put(NI_DUST, site.atmosphere ? "DUST" : "REGOLITH", H.meanC, readC);
            break;
    }
    return R;
}

// ---- the lamp's ultraviolet
int SurfaceView::filmNicheOf(const TerrainVertex& v, bool springs, double sea, double& share) {
    share = 0;
    if (v.material == MAT_WATER || v.material == MAT_LAVA || v.material == MAT_GLASS) return -1;
    if ((v.water > -1e8f && v.h < v.water - 0.05f) || v.h < sea - 0.05) return -1;   // under the liquid: its own glow is the sea's (the milky seas)
    if (((v.water > -1e8f && v.h < v.water + 0.6f) || std::fabs(v.h - sea) < 0.6) && v.material != MAT_SNOW && v.material != MAT_ICE) { share = 0.8; return NI_SILT; }   // a shore's line
    switch (v.material) {
        case MAT_MATS: share = 1.0; return springs ? NI_SPRING : NI_MAT;
        case MAT_STROMATOLITE: share = 0.85; return NI_MAT;
        case MAT_CRUST: share = 0.8; return NI_CRUST;
        case MAT_MOSS: share = 0.9; return NI_CRUST;
        case MAT_SNOW: share = v.stint > 127 ? 1.0 : 0.1; return NI_SNOW;
        case MAT_ICE: share = 0.08; return NI_ICE;
        case MAT_SALT: case MAT_POTASH: share = 0.3; return NI_SALT;
        case MAT_BRINE: share = 0.6; return NI_SALT;
        case MAT_GRASS: case MAT_FOREST: share = 0.35; return NI_SOIL;
        case MAT_MUD: share = 0.6; return NI_SILT;
        case MAT_SAND: case MAT_DUST: case MAT_SULPHUR: share = 0.04; return NI_DUST;
        default: share = 0.12; return NI_ROCK;   // the endoliths' faint glow in the rock (Friedmann 1982)
    }
}

void SurfaceView::setupFilmGlow() {
    life = lifeOf(*site.sys);
    const Biota& B = biota();
    for (int n = 0; n < NI_COUNT; n++) filmD[n] = (B.habitats & nicheBit(n)) ? filmDensity(B, n) : 0.0;
    filmColour = filmGlowColour(B);
}

// the frame's grid: the near ring's corners round the eye, their height and their films' glow (their niche's density by their share)
void SurfaceView::prepareFilmGlow() {
    uvFrame = false;
    if (!uvLamp || inVehicle() || cameraOverrideAlt >= 0 || freeCam) return;
    const bool springs = site.gen.hasTrait(TR_HOT_SPRINGS) || (KINDS[site.gen.type].events & EV_SPRINGS);
    const double cs = site.lodN.cellSize, sea = seaLevelNow();
    uvCx0 = (int)std::floor(camPos.x / cs) - UV_GRID / 2; uvCz0 = (int)std::floor(camPos.z / cs) - UV_GRID / 2;
    bool any = false;
    for (int j = 0; j < UV_GRID; j++)
        for (int i = 0; i < UV_GRID; i++) {
            const TerrainVertex& v = site.lodN.at(uvCx0 + i, uvCz0 + j);
            double share = 0;
            const int n = filmNicheOf(v, springs, sea, share);
            const double s = n >= 0 ? share * filmD[n] : 0.0;
            uvH[j * UV_GRID + i] = v.h; uvS[j * UV_GRID + i] = (float)s;
            any = any || s > 0;
        }
    uvFrame = any;
}

// after the old result: within the lamp's narrow cone and reach, a ground pixel whose films glow takes the glow's light added to its own
// (its luminance read from the palette, the sum's shade found in a bank: a pixel changes bank by luminance, never by shade), in bank 15 where
// the glow's light is the brighter, in its own where it is not: the sum is continuous across the change. Patchy as films are (two value
// noises anchored in the world, half a metre and fifteen centimetres); a mat's world's domes and stones wear its mats (their films up to
// 1.6 m over the ground). Rows pure: each pixel reads the frame's grid and writes itself
void SurfaceView::drawFilmGlow(Framebuffer& fb) {
    if (!uvFrame) return;
    const Vec3 R0 = camLocal.row(0), R1 = camLocal.row(1), F = camLocal.row(2);
    constexpr double REACH = 18.0, NEAR = 6.0, COS_IN = 0.9781, COS_OUT = 0.9135;   // full within 12 degrees, gone at 24
    float lum[BANKS][64];
    for (int bk = 0; bk < BANKS; bk++)
        for (int s = 0; s < 64; s++) {
            const uint8_t* c = &fb.pal[(bk * 64 + s) * 3];
            lum[bk][s] = 0.2126f * c[0] + 0.7152f * c[1] + 0.0722f * c[2];
        }
    const float* L15 = lum[15];
    const double cs = site.lodN.cellSize, f = proj.f, pcx = proj.cx, pcy = proj.cy;
    const uint64_t seed = site.gen.seed ^ 0xF11A6ULL;
    const double rise = kindHas(site.gen.type, KT_MICROBIAL) ? 1.6 : 0.35;
    parallelFor(FBH, 16, [&](int y0, int y1) {
        for (int py = y0; py < y1; py++) {
            for (int px = 0; px < FBW; px++) {
                const size_t idx = (size_t)py * FBW + px;
                const float iz = fb.invz[idx];
                if (iz <= 0) continue;
                const double zv = 1.0 / iz;
                if (zv > REACH) continue;
                const double xv = (px + 0.5 - pcx) * zv / f, yv = (pcy - (py + 0.5)) * zv / f;
                const Vec3 d = R0 * xv + R1 * yv + F * zv;
                const double dist = length(d);
                if (dist > REACH || dist < 0.3) continue;
                const double c = dot(d, F) / dist;
                if (c <= COS_OUT) continue;
                const Vec3 p = camPos + d;
                const double gx = p.x / cs - uvCx0, gz = p.z / cs - uvCz0;
                const int ix = (int)std::floor(gx), iz0 = (int)std::floor(gz);
                if (ix < 0 || iz0 < 0 || ix >= UV_GRID - 1 || iz0 >= UV_GRID - 1) continue;
                const double fx = gx - ix, fz = gz - iz0;
                const int k = iz0 * UV_GRID + ix;
                auto bil = [&](const float* a) { return (a[k] * (1 - fx) + a[k + 1] * fx) * (1 - fz) + (a[k + UV_GRID] * (1 - fx) + a[k + UV_GRID + 1] * fx) * fz; };
                const double s = bil(uvS);
                if (s <= 0.002) continue;
                const double above = p.y - bil(uvH);
                if (above > rise || above < -0.5) continue;   // the ground's own pixels (not a trunk, the water's sheet)
                auto vnoise = [&](double cell, uint64_t salt) {
                    const double nx = p.x / cell, nz = p.z / cell;
                    const int qx = (int)std::floor(nx), qz = (int)std::floor(nz);
                    const double tx = nx - qx, tz = nz - qz, sx = tx * tx * (3 - 2 * tx), sz = tz * tz * (3 - 2 * tz);
                    auto hv = [&](int a, int b) { return hash01(hash2i(a, b, seed ^ salt)); };
                    return (hv(qx, qz) * (1 - sx) + hv(qx + 1, qz) * sx) * (1 - sz) + (hv(qx, qz + 1) * (1 - sx) + hv(qx + 1, qz + 1) * sx) * sz;
                };
                const double n = smoothstep(0.22, 0.78, 0.62 * vnoise(0.5, 0) + 0.38 * vnoise(0.15, 1));
                const double cone = smoothstep(COS_OUT, COS_IN, c), fall = (1 - smoothstep(0.7 * REACH, REACH, dist)) / (1 + dist * dist / (NEAR * NEAR));
                const double glow = s * (0.15 + 0.85 * n) * cone * fall;
                const int gs = std::min(58, (int)(64 * glow));
                if (gs < 4) continue;
                const Pix p0 = fb.idx[idx];
                const int bk = bankOf(p0), sh = std::min(63, (int)(intenOf(p0) / INTEN_PER_SHADE));
                const float Lp = lum[bk][sh], Lg = L15[gs], want = Lp + Lg;
                const int to = Lg > Lp ? 15 : bk;
                const float* L = lum[to];
                int t = to == 15 ? gs : sh;
                while (t < 63 && L[t] < want) t++;
                if (to == bk && t == sh) continue;
                fb.idx[idx] = pix(to, t);
            }
        }
    });
}
