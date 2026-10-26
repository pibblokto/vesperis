// E-07: the kinds' ground functions (P-00), each a kind's base, then the shared skeleton (`GroundCtx::relief`), then its own
// features and materials: cratered to ammonia (the rest in planet_grounds2.cpp)
#include "planet_internal.h"

// the cratered airless ground: maria between rolling highlands, the crater stack (cratered worlds)
void groundCratered(GroundCtx& c) {
    const BodyGen& g = c.g; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright;
    const double R = c.R, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.5, g.sA, 5);
    double mare = smoothstep(0.15, 0.4, base);
    double h = base * g.mountainAmp * (1 - 0.8 * mare);
    // rolling highlands between the maria (the 20 km hills and the 400 m detail of generation 5 are gone)
    double rel = (1 - mare) * massif, hf = std::max(hillsF, 0.6 * (1 - mare));
    c.relief(h, rel, hf, reliefAmp(g, rel, hf) * (1 - 0.7 * mare));   // the skeleton (the maria smooth the relief)
    double cr = 0;
    c.craterStack(cr);   // the kind's eight grids (M9-06: the 0.5 km pass denser in the secondary fields near large craters)
    h += cr * (1 - 0.5 * mare);
    s.height = h;
    double flooded = clampd(cm.flooded, 0, 1);
    s.material = (mare > 0.5 || flooded > 0.5) ? MAT_BASALT : MAT_DUST;
    s.albedo = (0.52 + 0.12 * base) * (1 - 0.45 * std::max(mare, flooded)) + 0.12 * clampd(rimBright, 0, 1) - 0.08 * clampd(cm.floorDark, 0, 1) + 0.3 * clampd(cm.rays, 0, 1);
    s.relief = clampd(std::fabs(cr) / 400.0 + rel, 0, 1);
}

// P-00: the molten ground: lava lakes and glowing cracks, cones and their flows (molten worlds)
void groundMolten(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.4, g.sA, 5);
    double roughL = c.rg(R * 0.15, g.sC, 2);
    bool caldera = false;
    double h = base * 1600.0 + roughL * g.mountainAmp * 0.5;
    double zone = smoothstep(0.45, 0.85, roughL), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // the rough belts carry the relief spectrum
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    if (scaleVisible(R * 0.25, detailM)) h += volcanoField(p, R, R * 0.25, g.volcanoes * 0.6, g.sD, 2800.0, caldera);
    if (scaleVisible(30.0, detailM)) h += volcanoField(p, R, 30.0, g.volcanoes * 0.8, g.sD + 3, 700.0, caldera);
    LavaOut lf = lavaFlows(p, R, 30.0, g.volcanoes * 0.8, g.sD + 3, detailM);   // tongues and pit chains from the cones
    h += lf.dh;
    s.height = h;
    s.relief = clampd(roughL + 0.5 * rel + 0.5 * rf.slope, 0, 1);
    Crack crack = c.crackAt(12.0, g.sF, 0.05, 1.0);
    if (h < g.lavaLevel || caldera) {
        s.material = MAT_LAVA; s.glow = 1.0; s.albedo = 1.0;
        if (h < g.lavaLevel) s.height = g.lavaLevel;
    } else if (crack.m > 0.5 && h < g.lavaLevel + 900) {
        s.material = MAT_LAVA; s.glow = 0.75; s.albedo = 0.9; s.height = h - 6.0 * crack.h;
    } else if (lf.flow > 0.4 || lf.pit > 0.3) {
        s.material = MAT_BASALT; s.albedo = 0.09 + 0.03 * (1 - lf.flow); s.glow = lf.pit > 0.6 ? 0.25 : 0.0;   // fresh flows, the tubes' skylights glowing
    } else {
        s.material = MAT_BASALT; s.albedo = 0.13 + 0.10 * (0.5 + 0.5 * base) + 0.05 * roughL;
    }
}

// P-00: under the deck: the plates, tesserae on the highlands, pancake domes (venusian worlds)
void groundVenusian(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const Tectonics& tec = c.tec;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * g.mountainAmp + tec.uplift * 800.0;
    { double chainR = c.rg(45.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.7) - tec.rift * 800.0 + tec.ridge * 400.0; }
    double zone = clampd(smoothstep(0.1, 0.6, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // tesserae on the highlands and along the chains
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    if (scaleVisible(R * 0.08, detailM)) h += domeField(p, R, R * 0.08, 0.3, g.sD, 0.35, 700.0, 0.85) * lodFade(R * 0.08, detailM);
    if (scaleVisible(3.0, detailM)) h += domeField(p, R, 3.0, 0.3, g.sD + 1, 0.4, 40.0, 0.9);
    s.height = h;
    s.material = MAT_BASALT;
    s.albedo = 0.30 + 0.12 * base;
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-00: creased rock: the plates' chains and rifts, old shields, boulder fields (rocky worlds)
void groundRocky(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const Tectonics& tec = c.tec;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double creaseL = c.rg(R * 0.15, g.sA, 2);
    double base = c.fb(R * 0.45, g.sB, 4);
    double h = creaseL * g.mountainAmp + base * 600.0;
    {   // chains and rifts across the crust
        double chainR = c.rg(40.0, g.sA + 2, 7);
        h += tec.chain * (chainR * chainR * g.chainAmp * 0.8 + std::fabs(chainR) * 500.0) + tec.uplift * 600.0;
        h -= tec.rift * 900.0 * (0.5 + 0.5 * std::fabs(c.fb(50.0, g.sA + 3, 3)));
        h += tec.ridge * 500.0 * std::fabs(c.rg(35.0, g.sA + 4, 5));
    }
    double zone = clampd(smoothstep(0.3, 0.7, creaseL) + 0.7 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // creased ridges (the 20 km crease, the 4 km hills and the detail are gone)
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    if (scaleVisible(R * 0.3, detailM)) h += domeField(p, R, R * 0.3, 0.2, g.sD + 7, 0.4, 900.0, 0.6) * lodFade(R * 0.3, detailM);   // old shield volcanoes
    if (scaleVisible(0.25, detailM)) h += domeField(p, R, 0.25, g.boulderDensity, g.sD, 0.16, 14.0, 0.0);
    if (scaleVisible(0.06, detailM)) h += domeField(p, R, 0.06, g.boulderDensity * 0.7, g.sD + 2, 0.2, 3.0, 0.0);
    c.craterStack(h);   // the kind's crater stack
    s.height = h;
    s.material = MAT_ROCK;
    s.albedo = 0.28 + 0.18 * creaseL + 0.05 * base + 0.06 * clampd(rf.slope * 2.0, 0, 1);
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-00: thin air: canyons, shield volcanoes and their dark plains, dune seas, dry valleys, ice caps (thin-atmosphere worlds)
void groundThinAir(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright; const Tectonics& tec = c.tec; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, lat = c.lat, absLatDeg = c.absLatDeg, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.5, g.sA, 5);
    double mtnL = c.rg(R * 0.14, g.sC, 2);
    double canyonN = c.rg(R * 0.3, g.sF, 2);
    double canyon = std::pow(clampd(canyonN, 0, 1), 9.0);
    double mtnMask = smoothstep(0.3, 0.7, mtnL) * smoothstep(-0.2, 0.4, base);
    double h = base * 1800.0 + mtnL * mtnL * g.mountainAmp * mtnMask - canyon * 2200.0;
    {   // chains and rifts, M9-07 shield volcanoes with dark lava plains around them
        double chainR = c.rg(40.0, g.sC + 2, 7);
        h += tec.chain * (chainR * chainR * g.chainAmp * 0.9 + std::fabs(chainR) * 500.0) + tec.uplift * 900.0;
        h -= tec.rift * 1500.0 * (0.5 + 0.5 * std::fabs(c.fb(50.0, g.sC + 3, 3)));
        h += tec.ridge * 600.0 * std::fabs(c.rg(35.0, g.sC + 4, 5));
    }
    // the relief spectrum (the 30 km ridges, the 8 km hills and the 400 m detail of generation 5 are gone)
    double zone = clampd(mtnMask + 0.8 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    double relief0 = clampd(rel + 0.25 * hf, 0, 1);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    bool shieldCal = false;
    double shield = 0;
    if (scaleVisible(R * 0.35, detailM)) { shield = volcanoField(p, R, R * 0.35, 0.28, g.sD + 11, 1400.0, shieldCal) * lodFade(R * 0.35, detailM); h += shield; }
    c.craterStack(h);   // the kind's crater stack
    if (h < 200) { const double share = smoothstep(200, -300, h); double lee = 0; h += duneField(p, g, lat, g.duneAmp, detailM, &lee) * share; s.slipFace = share * lee; }   // dune seas in the lowlands
    applyDrainage(g, unit, detailM, h, nullptr, rf.slope, false, false, 0.5, DA);   // the old valley networks, dry, with dust flats in the basins
    s.height = h;
    s.relief = clampd(relief0 + 0.5 * rf.slope, 0, 1);
    double capEdge = g.iceCapLat + 5.0 * gnoise3(p / (R * 0.15), g.sF + 7);
    if (absLatDeg > capEdge) { s.material = MAT_ICE; s.albedo = 0.85; }
    else if (DA.playa > 0.3) { s.material = MAT_DUST; s.albedo = 0.56 + 0.06 * base; }   // a dust flat in a closed basin
    else if (relief0 + 0.5 * rf.slopeM > 0.6 && h > 1500) { s.material = MAT_ROCK; s.albedo = 0.35; }   // the material chain's slope, not `s.relief` (whose slope is the ring's own)
    else if (shield > 400 || shieldCal) { s.material = MAT_BASALT; s.albedo = 0.22 + 0.06 * base; }
    else { s.material = MAT_DUST; s.albedo = 0.42 + 0.12 * base + 0.10 * clampd(rimBright, 0, 1) - 0.12 * canyon + 0.25 * clampd(cm.rays, 0, 1) - 0.1 * clampd(cm.flooded, 0, 1); }
}

// P-00: cracked ice: fracture networks, craters, cryovolcanic domes and geyser spots (icy worlds, comets)
void groundIce(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * g.mountainAmp;
    double zone = smoothstep(0.1, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // ridged uplands on the high ground (the 6 km hills and the detail are gone)
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    Crack c1 = c.crackAt(R * 0.15, g.sF, g.fractureWidth, 1.0), c2 = c.crackAt(12.0, g.sF + 3, g.fractureWidth, 0.8), c3 = c.crackAt(1.2, g.sF + 5, g.fractureWidth * 1.3, 0.6);
    double crack = std::max(c1.h, std::max(c2.h, c3.h)), crackM = std::max(c1.m, std::max(c2.m, c3.m));   // the relief by the faded lines, the rock by the unfaded ones
    h -= crack * 60.0;
    c.craterStack(h);   // the kind's crater stack
    // cryovolcanoes (bright fresh domes) and geyser fields (bright spots on the ground)
    double fresh = 0;
    if (scaleVisible(25.0, detailM)) { double cv = domeField(p, R, 25.0, 0.12, g.sD + 21, 0.25, 140.0, 0.0); h += cv; fresh += cv / 140.0; }
    if (scaleVisible(2.0, detailM)) fresh += domeField(p, R, 2.0, 0.08, g.sD + 22, 0.15, 1.0, 0.0);
    s.height = h;
    s.material = crackM > 0.55 ? MAT_ROCK : MAT_ICE;
    s.albedo = (0.72 + 0.12 * base) * (1 - 0.5 * crackM) + 0.1 * clampd(rimBright, 0, 1) + 0.15 * clampd(fresh, 0, 1) + 0.2 * clampd(cm.rays, 0, 1);
    s.relief = clampd(crack + 0.5 * rel + 0.5 * rf.slope, 0, 1);
}

// P-00: milky quartz mounds (quartz worlds)
void groundQuartz(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.35, g.sA, 5);
    double h = base * g.mountainAmp;
    double zone = smoothstep(0.0, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // rounded mounds (ridge near 0, low ground smoothed; the 3.5 km rounded hills are gone)
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    if (scaleVisible(R * 0.06, detailM)) h += domeField(p, R, R * 0.06, 0.4, g.sD, 0.3, 500.0, 0.0) * lodFade(R * 0.06, detailM);
    c.craterStack(h);   // the kind's crater stack
    s.height = h;
    s.material = MAT_QUARTZ;
    s.albedo = 0.78 + 0.12 * base;
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1) * 0.6;
}

// P-00: one global sea and its floes (ocean worlds)
void groundOcean(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, lat = c.lat, absLatDeg = c.absLatDeg;
    // one global sea; ice floes drift toward the poles (plates with water gaps). B-322: the floe zone lies over a shallow
    // shelf sea (the floor at 5-8 m; the open ocean stays at 150 m) and every plate rises out of it through a submerged
    // ice foot, so the shore is the water plane's contour on a continuous surface at every ring (not a jump from +1 m
    // to -150 m between two vertices: a stair of cells from the air and a cliff face from the water)
    double swell = c.fb(2.0, g.sB, 4) * g.hillAmp;
    double capEdge = g.iceCapLat + 8.0 * gnoise3(p / (R * 0.2), g.sF + 7) - 8.0 * clampd(lat >= 0 ? -g.season : g.season, 0, 1);
    double nightIce = g.locked && !c.K.evenClimate ? clampd((-dot(unit, g.lockedDir) - 0.05) / 0.3, 0, 1) : 0.0;   // the night side freezes over (P-03: not under a hycean's air)
    double cover = std::max(smoothstep(capEdge - 12, capEdge + 6, absLatDeg), nightIce);       // the share of the plates that are ice
    double shelf = smoothstep(0.0, 0.04, cover);   // the whole floe zone (the plates appear at any cover) lies on the shelf
    double floor = (-150.0 + swell) + ((-6.5 + 0.3 * swell) - (-150.0 + swell)) * shelf;
    s.height = floor;
    s.material = MAT_WATER; s.water = 0; s.albedo = 0.26; s.biome = BIO_OCEAN;
    if (cover > 0) {
        Worley3 fl = worley3(p / 1.5, g.sF + 3);
        double d = fl.f2 - fl.f1;   // 0 on the plate boundary, growing inward (units of 1.5 km)
        if (unitFromHash(fl.id1) < cover) {
            // an ice plate: the foot climbs from the floor at the boundary to the water line 150 m in, the freeboard
            // 0.9-1.25 m rises over the next 45 m and stays flat; the gap between two ice plates is 300 m of shallow water
            double rim = smoothstep(0.0, 0.13, d);
            double top = 0.9 + 0.35 * smoothstep(0.13, 0.3, d);
            double h = floor + (top - floor) * rim;
            s.height = h;
            if (h > -0.3) { s.material = MAT_ICE; s.albedo = 0.86; s.biome = BIO_ICE; }
            else s.albedo = 0.26 + 0.2 * clampd((h + 6.5) / 6.5, 0, 1);   // the foot shows pale through the shallow water on the map
        }
    }
}

// P-00: an ice shell over a hidden ocean: lineae, chaos rafts (europan worlds)
void groundEuropan(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    // an ice shell over a hidden ocean: a plain a few tens of metres in relief, crossed by lineae (double ridges
    // with a trough between, stained red-brown by what wells up through them), chaos terrain where the shell broke
    // into rafts, a few craters. The water geysers stand on the lineae (`drawGeysers` finds the stained cells)
    double base = c.fb(R * 0.5, g.sA, 4);
    double h = base * 60.0;
    double zone = smoothstep(0.3, 0.7, base), rel = zone * massif * 0.3, hf = hillsF * 0.3;
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    double stain = 0, band = 0;
    auto lineae = [&](double cellKm, double ridgeM, double troughM, double spanM, uint64_t seed, double fadeKm) {
        // the ridges fade with the sampling scale as before; the stain (three spans wide: the dust) is unfaded
        // wherever it spans half a cell, so the three sets' brown bands lie on the same ground in every ring (following
        // the fade, the 6 km set's bands would end at the near ring's edge, the 40 km set's at the 16 m ring's)
        double f = featFade(spanM * 4.0 / 1000.0, detailM);   // the double ridge and its trough, four spans wide (`lodFade(fadeKm)` would leave the 12 m ridges at 41% in the 16 m ring)
        bool mv = matVisible(spanM * 3.0 / 1000.0, detailM);
        (void)fadeKm;
        if (f <= 0 && !mv) return;
        Worley3 w = worley3(p / cellKm, seed);
        double dm = (w.f2 - w.f1) * cellKm * 500.0;   // metres from the line (half the difference of the two distances)
        if (f > 0) {
            double rim = std::exp(-std::pow((dm - spanM) / (spanM * 0.45), 2.0));
            double trough = std::exp(-std::pow(dm / (spanM * 0.6), 2.0));
            h += (ridgeM * rim - troughM * trough) * f;
        }
        if (mv) stain = std::max(stain, smoothstep(spanM * 2.2, spanM * 0.8, dm));
    };
    {   // the great lineae stain a band 6-16 km wide (what a globe shows of them), the ridges themselves are the first set below
        Worley3 w = worley3(p / (R * 0.25), g.sF + 1);
        double dm = (w.f2 - w.f1) * R * 0.25 * 500.0;
        band = smoothstep(8000.0, 2500.0, dm) * (0.6 + 0.4 * gnoise3(p / 60.0, g.sF + 17));
        stain = std::max(stain, band);
    }
    lineae(R * 0.25, 120.0, 40.0, 300.0, g.sF + 1, 1.5);
    lineae(40.0, 45.0, 15.0, 110.0, g.sF + 2, 0.5);
    lineae(6.0, 12.0, 4.0, 30.0, g.sF + 3, 0.12);
    double chaosMask = smoothstep(0.35, 0.6, c.fb(R * 0.15, g.sC + 3, 3));   // rafts of the shell, jumbled where the ocean broke through
    if (chaosMask > 0 && scaleVisible(3.0, detailM)) {
        Worley3 cw = worley3(p / 3.0, g.sF + 9);
        double edge = smoothstep(0.02, 0.07, cw.f2 - cw.f1);
        h += chaosMask * edge * (60.0 * unitFromHash(cw.id1) - 30.0) * featFade(3.0, detailM);   // the rafts by their own size (the 512 m ring draws them)
    }
    c.craterStack(h);   // the kind's crater stack
    s.height = h;
    s.material = stain > 0.5 ? MAT_DUST : MAT_ICE;
    s.albedo = stain > 0.5 ? 0.40 - 0.1 * stain : (0.72 + 0.08 * base + 0.1 * clampd(rimBright, 0, 1) - 0.15 * chaosMask) * (1 - 0.35 * stain);
    s.relief = clampd(chaosMask * 0.5 + 0.5 * rf.slope, 0, 1);
}

// P-00: a seismic crust: rifts and their fissures, cones, ash plains (tectonic worlds)
void groundTectonic(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const Tectonics& tec = c.tec;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    // a seismic world: rift valleys where the crust pulls apart (their floors flat, 800 m down), fissures on the
    // floors running with lava (`drawFountains` stands its fountains on them), fault-block ranges, ash plains, sulphur
    // crusts round the vents, a few volcanoes
    double base = c.fb(R * 0.4, g.sA, 5);
    double h = base * g.mountainAmp * 0.6 + tec.uplift * 700.0;
    double zone = clampd(smoothstep(0.1, 0.6, base) + 0.7 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    double hS = h - (rf.h - rf.hCoarse);
    Worley3 rw = worley3(p / (R * 0.3), g.sF + 5);
    double dRift = (rw.f2 - rw.f1) * R * 0.3 * 500.0;   // metres from the rift axis
    double rift = smoothstep(11000.0, 7000.0, dRift);
    h += (hS - 800.0 - h) * rift;
    double lava = 0, fresh = 0;
    if (rift > 0.5 && (scaleVisible(5.0, detailM) || matVisible(1.0, detailM))) {   // the fissures: trenches 30 m deep and 400 m wide on 5 km cells, lava along their axes (100 m), fresh basalt and sulphur crusts round them (1-1.3 km); B-404: each by its own width
        Worley3 fw = worley3(p / 5.0, g.sF + 6);
        double dm = (fw.f2 - fw.f1) * 2500.0;
        h -= 30.0 * smoothstep(220.0, 40.0, dm) * featFade(0.44, detailM);
        if (matVisible(0.14, detailM)) lava = smoothstep(90.0, 50.0, dm) * rift;
        if (matVisible(1.0, detailM)) fresh = smoothstep(700.0, 150.0, dm) * rift;
    }
    bool caldera = false;
    if (scaleVisible(R * 0.2, detailM)) h += volcanoField(p, R, R * 0.2, g.volcanoes * 0.5, g.sD, 2200.0, caldera) * lodFade(R * 0.2, detailM);
    if (scaleVisible(20.0, detailM)) h += volcanoField(p, R, 20.0, g.volcanoes * 0.7, g.sD + 3, 500.0, caldera);
    LavaOut lf = lavaFlows(p, R, 20.0, g.volcanoes * 0.7, g.sD + 3, detailM);
    h += lf.dh;
    c.craterStack(h);   // the kind's crater stack
    s.height = h;
    double ash = c.fb(R * 0.1, g.sC + 8, 3);
    if (lava > 0.5 || caldera) { s.material = MAT_LAVA; s.glow = 1.0; s.albedo = 1.0; }
    else if (fresh > 0.4 || lf.flow > 0.4 || lf.pit > 0.3) { s.material = MAT_BASALT; s.albedo = 0.12 + 0.06 * (1 - std::max(fresh, lf.flow)); if (lf.pit > 0.6) s.glow = 0.25; }
    else if (fresh > 0.12) { s.material = MAT_SULPHUR; s.albedo = 0.58 + 0.2 * fresh; }
    else if (rel < 0.45 && ash > 0.05 && rift < 0.5) { s.material = MAT_DUST; s.albedo = 0.34 + 0.1 * ash; }
    else { s.material = MAT_BASALT; s.albedo = 0.30 + 0.08 * (0.5 + 0.5 * base); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope + rift * 0.3, 0, 1);
}

// P-00: a dry world: hamada, canyons, mesas, ergs, salt pans and wadis; a dead world's dry seas (desert worlds)
void groundDesert(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright; const Tectonics& tec = c.tec; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, lat = c.lat, massif = c.massif, hillsF = c.hillsF;
    // a dry world under a dusty sky: hamada plains, mesas where the flats terrace, canyons, ergs in the basins
    // with salt pans at their floors (the dust devils and storms are the surface's, `computeEnvironment`)
    double base = c.fb(R * 0.5, g.sA, 5);
    double h = base * 1400.0 + tec.uplift * 600.0;
    { double chainR = c.rg(40.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.6) - tec.rift * 700.0 + tec.ridge * 300.0; }
    double zone = clampd(smoothstep(0.2, 0.7, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.5 * zone);
    double relief0 = clampd(rel + 0.25 * hf, 0, 1);
    double hRel, hS; ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf), &hRel, &hS, false);   // the skeleton without the cliff bands (the mesas terrace below); hS the smooth ground before the landforms
    double canyonN = c.rg(R * 0.3, g.sF, 2);
    double canyon = std::pow(clampd(canyonN, 0, 1), 8.0);
    h -= canyon * 900.0;
    double oldSea = 0, seaDepth = 0, bedH = 0;   // a dead world's seas, dry: the bed a sediment floor keeping a tenth of the relief, deeper offshore
    if (g.oldSeaM > -1e8) {            // (a canyon under the level was a sound of the sea), the old shore a bench of beach ridges a few metres
        double hC = hRel - canyon * 900.0;   // over the level (the coastal flats the people lived on). The level cuts the coarse ground with its
        oldSea = smoothstep(g.oldSeaM + 1.5, g.oldSeaM - 1.5, hC);   // canyons in a band of 3 m: the shore is a low scarp, not a ramp
        seaDepth = std::max(0.0, g.oldSeaM - hC);
        if (oldSea > 0) { bedH = g.oldSeaM - 7.0 - seaDepth * 0.35 + rf.h * 0.1; h += (bedH - h) * oldSea; hS += (bedH - hS) * oldSea; }
        double over = hC - g.oldSeaM;
        if (over > 0.0 && over < 40.0) { double bench = g.oldSeaM + 3.0 + 0.15 * rf.h; h += (bench - h) * 0.7 * smoothstep(40.0, 15.0, over); }
        canyon *= 1 - oldSea;   // the material's reading: no canyon walls on the bed
    }
    if (relief0 < 0.4 && h > 100 && oldSea < 0.5) {   // mesas: the dry flats terrace (M9-03's rule)
        double step = 45.0, q = std::floor(h / step) * step, f = (h - q) / step;
        double terr = q + step * smoothstep(0.35, 0.65, f);
        h += (terr - h) * (1 - smoothstep(0.25, 0.4, relief0)) * smoothstep(-0.1, 0.3, c.fb(R * 0.12, g.sC + 4, 3));
    }
    double basin = smoothstep(0.1, 0.5, -c.fb(R * 0.1, g.sG + 2, 3)) * (1 - smoothstep(300.0, 700.0, h)) * (1 - relief0) * (1 - 0.7 * oldSea);
    double salt = 0;
    if (basin > 0.7 && h < 80) { double pan = smoothstep(0.7, 0.9, basin); h += (hS - 4.0 - h) * pan; salt = pan; }
    if (basin > 0.2 && salt < 0.5) { const double share = smoothstep(0.2, 0.5, basin); double lee = 0; h += duneField(p, g, lat, g.duneAmp * (0.6 + 0.4 * basin), detailM, &lee) * share; s.slipFace = share * lee; }
    c.craterStack(h);   // the kind's crater stack
    applyDrainage(g, unit, detailM, h, &hS, rf.slope, false, false, g.oldSeaM > -1e8 ? 0.75 : 0.5, DA);   // wadis, and playas in the closed basins (C-01: a dead world's rivers ran wetter: a denser net)
    if (oldSea > 0) h += (bedH - h) * oldSea;   // the bed again: a wadi's valley reaching down over the shore lifted the bed toward its banks, and the craters and dunes are the sea's silt now
    s.height = h;
    if (salt > 0.5 || DA.playa > 0.3) { s.material = MAT_SALT; s.albedo = 0.85; }
    else if (DA.bed > 0.4) { s.material = MAT_SAND; s.albedo = 0.6 + 0.1 * base; }   // the wadi's bed
    else if (basin > 0.5) { s.material = MAT_SAND; s.albedo = 0.6 + 0.1 * base; }
    else if (relief0 > 0.5 || canyon > 0.3) { s.material = MAT_ROCK; s.albedo = 0.4 + 0.1 * base; }
    else { s.material = MAT_DUST; s.albedo = 0.46 + 0.12 * base + 0.1 * clampd(rimBright, 0, 1); }
    if (oldSea > 0.5 && DA.bed < 0.4) {   // the seabed's silt, the last brine's salt in the deeps; the old beaches along the shore
        if (seaDepth > 420.0 + 150.0 * c.fb(R * 0.05, g.sG + 9, 2)) { s.material = MAT_SALT; s.albedo = 0.82; }
        else { s.material = MAT_ROCK; s.albedo = 0.36 + 0.05 * base; }   // the silt crust of the floor: the rock's brown, darker than the hamada
    } else if (g.oldSeaM > -1e8 && oldSea < 0.5 && hRel - canyon * 900.0 - g.oldSeaM < 6.0 && DA.bed < 0.4 && salt < 0.5) { s.material = MAT_SAND; s.albedo = 0.62; }
    s.oldSea = oldSea;
    s.relief = clampd(relief0 + 0.5 * rf.slope + canyon, 0, 1);
}

// P-00: under the haze: ice-rock hills, tholin dunes, methane seas and channels (hydrocarbon worlds)
void groundHydrocarbon(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, lat = c.lat, absLatDeg = c.absLatDeg, massif = c.massif, hillsF = c.hillsF;
    // under the orange haze: rounded hills of ice-rock, belts of dark tholin dunes in the low latitudes, seas
    // and lakes of methane in the lowlands and toward the poles (the liquid at the reference level, the post-step
    // below), dry channels cut down to them, cryovolcanic domes
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * 700.0 + 130.0 - 170.0 * smoothstep(50.0, 75.0, absLatDeg);
    double zone = smoothstep(0.2, 0.7, base), rel = zone * massif * 0.6, hf = std::max(hillsF, 0.5 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    double chan = c.crackAt(30.0, g.sF + 4, 0.06, 1.0).h;   // channels: shallow valleys on 30 km cells, deeper toward the sea
    h -= chan * 25.0 * smoothstep(400.0, 50.0, h);
    double dunes = smoothstep(35.0, 20.0, absLatDeg) * smoothstep(0.0, 0.3, c.fb(R * 0.12, g.sG + 5, 3)) * smoothstep(300.0, 60.0, h) * smoothstep(0.0, 20.0, h);
    if (dunes > 0.05) { double lee = 0; h += duneField(p, g, lat, g.duneAmp, detailM, &lee) * dunes; s.slipFace = dunes * lee; }
    if (scaleVisible(25.0, detailM)) h += domeField(p, R, 25.0, 0.08, g.sD + 21, 0.25, 180.0, 0.0);
    c.craterStack(h);   // the kind's crater stack
    applyDrainage(g, unit, detailM, h, nullptr, rf.slope, true, true, 0.5, DA);   // rivers of methane to the seas, lakes in the depressions
    s.height = h;
    s.material = dunes > 0.4 ? MAT_SAND : MAT_ICE;
    if (DA.level > -1e8 && h < DA.level) { s.material = MAT_WATER; s.water = DA.level; s.albedo = 0.2; }
    s.albedo = dunes > 0.4 ? 0.18 : 0.32 + 0.08 * base + 0.06 * clampd(rimBright, 0, 1);
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-00: a young airless surface: craters at every scale, rays (bombarded worlds)
void groundBombarded(GroundCtx& c) {
    const BodyGen& g = c.g; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright;
    const double R = c.R, massif = c.massif, hillsF = c.hillsF;
    // a young surface under a rain of rock: craters at every scale with sharp rims and bright rays over a dark
    // glassy regolith; the strikes themselves are the surface's (`drawImpacts`)
    double base = c.fb(R * 0.4, g.sA, 5);
    double h = base * g.mountainAmp * 0.6;
    double zone = smoothstep(0.2, 0.6, base), rel = zone * massif * 0.7, hf = std::max(hillsF, 0.5 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    double cr = 0;
    c.craterStack(cr, true);   // the kind's crater stack
    h += cr;
    s.height = h;
    s.material = MAT_DUST;
    s.albedo = 0.28 + 0.06 * base + 0.14 * clampd(rimBright, 0, 1) + 0.5 * clampd(cm.rays, 0, 1);
    s.relief = clampd(std::fabs(cr) / 200.0 + rel + 0.5 * rf.slope, 0, 1);
}

// P-00: a corrosive world: karst, sulphur crusts, acid seas and rivers (acidic worlds)
void groundAcidic(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const Tectonics& tec = c.tec; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    // a corrosive world: the rock bleached and eaten into karst (sinkholes at 300 m, towers in the wettest
    // basins), sulphur crusts by the shores, seas of dilute acid at the reference level (the post-step below)
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * g.mountainAmp * 0.8 + 150.0 + tec.uplift * 600.0;
    { double chainR = c.rg(45.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.6) - tec.rift * 600.0; }
    double zone = clampd(smoothstep(0.1, 0.6, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.5 * zone);
    double relief0 = clampd(rel + 0.25 * hf, 0, 1);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    double low = smoothstep(600.0, 150.0, h) * (1 - smoothstep(0.3, 0.5, relief0));
    if (low > 0.1 && scaleVisible(0.3, detailM)) {
        Worley3 kw = worley3(p / 0.3, g.sF + 12);
        h -= 9.0 * low * smoothstep(0.55, 0.15, kw.f1) * lodFade(0.3, detailM);
    }
    if (low > 0.5 && scaleVisible(1.5, detailM)) h += domeField(p, R, 1.5, 0.25, g.sD + 13, 0.12, 70.0, 0.0) * low;
    c.craterStack(h);   // the kind's crater stack
    applyDrainage(g, unit, detailM, h, nullptr, rf.slope, true, true, 0.5, DA);   // rivers of dilute acid, lakes in the karst basins
    s.height = h;
    double crust = smoothstep(40.0, 5.0, h) * smoothstep(0.35, 0.15, relief0);
    if (DA.level > -1e8 && h < DA.level) { s.material = MAT_WATER; s.water = DA.level; s.albedo = 0.22; }
    else if (crust > 0.5) { s.material = MAT_SULPHUR; s.albedo = 0.55 + 0.15 * crust; }
    else { s.material = MAT_ROCK; s.albedo = 0.55 + 0.1 * base - 0.1 * low; }
    s.relief = clampd(relief0 + 0.5 * rf.slope, 0, 1);
}

// P-00: dark iron: sharp ridges and craters (metal worlds)
void groundMetal(GroundCtx& c) {
    const BodyGen& g = c.g; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright;
    const double R = c.R, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.4, g.sA, 5);
    double h = base * g.mountainAmp;
    double zone = smoothstep(-0.2, 0.5, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // sharp iron ridges (the 30 km crease and the 4 km hills are gone)
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    double cr = 0;
    c.craterStack(cr, true);   // the kind's crater stack
    h += cr;
    s.height = h;
    s.material = MAT_METAL;
    s.albedo = 0.12 + 0.08 * base + 0.1 * clampd(rimBright, 0, 1) + 0.25 * clampd(cm.rays, 0, 1);
    s.relief = clampd(std::fabs(cr) / 300.0 + rel + 0.5 * rf.slope, 0, 1);
}

// P-00: a tidally heated moon: cones, lava, sulphur plains (volcanic worlds)
void groundVolcanic(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.4, g.sA, 4);
    bool caldera = false;
    double h = base * g.mountainAmp * 0.5;
    double zone = smoothstep(0.0, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // (the 5 km hills and the detail are gone)
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    if (scaleVisible(R * 0.2, detailM)) h += volcanoField(p, R, R * 0.2, g.volcanoes, g.sD, 3200.0, caldera) * lodFade(R * 0.2, detailM);
    if (scaleVisible(25.0, detailM)) h += volcanoField(p, R, 25.0, g.volcanoes * 0.9, g.sD + 3, 900.0, caldera);
    LavaOut lf = lavaFlows(p, R, 25.0, g.volcanoes * 0.9, g.sD + 3, detailM);
    h += lf.dh;
    if (scaleVisible(4.0, detailM)) h += domeField(p, R, 4.0, 0.25, g.sD + 5, 0.3, 40.0, 0.0);
    s.height = h;
    double plains = fbm3(p / (R * 0.12), g.sF, 3);
    if (h < g.lavaLevel || caldera) {
        s.material = MAT_LAVA; s.glow = 1.0; s.albedo = 1.0;
        if (h < g.lavaLevel) s.height = g.lavaLevel;
    } else if (lf.flow > 0.4 || lf.pit > 0.3) {
        s.material = MAT_BASALT; s.albedo = 0.1 + 0.05 * (1 - lf.flow); if (lf.pit > 0.6) s.glow = 0.25;
    } else if (plains > 0.15) {
        s.material = MAT_SULPHUR; s.albedo = 0.55 + 0.25 * plains;
    } else {
        s.material = MAT_BASALT; s.albedo = 0.2 + 0.1 * (0.5 + 0.5 * base);
    }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-00: graphite plains with diamond crusts (carbon worlds)
void groundCarbon(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright;
    const double R = c.R, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * g.mountainAmp;
    double zone = smoothstep(0.0, 0.6, base), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);   // (the 6 km hills and the detail are gone)
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the relief spectrum, the cliffs and plains (O6-04), the traits' landforms
    c.craterStack(h);   // the kind's crater stack
    s.height = h;
    Worley3 dw = worley3(p / (R * 0.1), g.sF + 2);
    bool diamond = unitFromHash(dw.id1) < 0.18 && dw.f1 < 0.45;
    s.material = diamond ? MAT_QUARTZ : MAT_GRAPHITE;
    s.albedo = diamond ? 0.85 + 0.1 * base : 0.05 + 0.04 * (0.5 + 0.5 * base) + 0.08 * clampd(rimBright, 0, 1);
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-01: a magma ocean: a molten world's crust, and round the substellar point a sea of magma at the reference level, its shore a belt of
// crust rafts breaking up in the lava and cracks running with it; past the terminator black glass on the flats and basalt on the relief,
// a frost of condensed rock over the far night (magma oceans)
void groundMagma(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.4, g.sA, 5);
    double roughL = c.rg(R * 0.15, g.sC, 2);
    double h = base * 1100.0 + roughL * g.mountainAmp * 0.45;
    double zone = smoothstep(0.45, 0.85, roughL), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the skeleton: the relief spectrum, the cliffs and plains, the traits' landforms
    bool caldera = false;
    if (scaleVisible(30.0, detailM)) h += volcanoField(p, R, 30.0, g.volcanoes, g.sD + 3, 600.0, caldera);
    c.craterStack(h);
    // the sea: within its reach of the substellar point (the shore wandering by a few degrees) the crust melts down under the level
    const Vec3 sub = g.locked ? g.lockedDir : Vec3(1, 0, 0);
    const double ang = std::acos(clampd(dot(unit, sub), -1, 1)) / DEG;   // degrees from the substellar point
    const double reach = g.seaReachDeg + 5.0 * c.fb(R * 0.25, g.sF + 7, 3);
    const double melt = smoothstep(reach + 1.5, reach - 4.0, ang);
    if (melt > 0) h += (-80.0 - h) * melt;
    double raft = 0;   // the rafts of crust breaking up along the shore, fewer further out (the ocean world's floes, of rock in lava)
    if (melt > 0.2 && (scaleVisible(0.7, detailM) || matVisible(0.1, detailM))) {
        Worley3 fl = worley3(p / 0.7, g.sF + 11);
        if (unitFromHash(fl.id1) < 0.45 * smoothstep(reach - 10.0, reach - 1.0, ang)) {
            const double rim = smoothstep(0.0, 0.16, fl.f2 - fl.f1), top = 1.5 + 4.0 * unitFromHash(mix64(fl.id1 + 5));
            const double hr = -3.0 + (top + 3.0) * rim * featFade(0.7, detailM);
            if (hr > h) { h = hr; raft = rim; }
        }
    }
    s.height = h;
    s.relief = clampd(roughL + 0.5 * rel + 0.5 * rf.slope, 0, 1);
    const double night = smoothstep(86.0, 100.0, ang), far = smoothstep(105.0, 150.0, ang);
    Crack crack = c.crackAt(12.0, g.sF, 0.05, 1.0);
    if ((h < 0 && melt > 0) || (caldera && night < 0.5)) {
        s.material = MAT_LAVA; s.glow = 1.0; s.albedo = 1.0;
        if (h < 0) s.height = 0;
    } else if (raft > 0) {
        s.material = MAT_BASALT; s.albedo = 0.09 + 0.05 * raft; s.glow = 0.35 * (1 - raft);   // the raft's edge still glows
    } else if (crack.m > 0.5 && ang < reach + 14.0 && h < 900) {
        s.material = MAT_LAVA; s.glow = 0.75; s.albedo = 0.9; s.height = h - 6.0 * crack.h;
    } else if (night > 0.5) {
        const double frost = far > 0 ? smoothstep(0.3 - 0.6 * far, 0.42 - 0.6 * far, c.fb(R * 0.12, g.sG + 5, 4)) : 0.0;   // patches the map sees, joining toward the antistellar point
        if (frost > 0.5) { s.material = MAT_SALT; s.albedo = 0.55 + 0.2 * far; }
        else if (rel < 0.4 && rf.slopeM < 0.15) { s.material = MAT_GLASS; s.albedo = 0.07 + 0.03 * (0.5 + 0.5 * base); }
        else { s.material = MAT_BASALT; s.albedo = 0.11 + 0.05 * (0.5 + 0.5 * base); }
    } else {
        s.material = MAT_BASALT; s.albedo = 0.12 + 0.08 * (0.5 + 0.5 * base) + 0.04 * roughL;
    }
}

// P-01: the bare core of a giant: smooth heavy ground, broad shallow basins floored with pale dust between low ridges of dark iron basalt,
// the iron itself on their crests (chthonian worlds)
void groundChthonian(GroundCtx& c) {
    const BodyGen& g = c.g; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright;
    const double R = c.R, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.5, g.sA, 5);
    double basin = smoothstep(0.05, 0.4, -c.fb(R * 0.15, g.sG + 2, 3));
    const double patch = c.fb(6.0, g.sG + 6, 3);   // the regolith's pale and dark patches on the plains
    double h = base * g.mountainAmp - 350.0 * basin;
    double zone = smoothstep(0.1, 0.6, base) * (1 - basin), rel = zone * massif * 0.6, hf = std::max(hillsF, 0.5 * zone) * 0.7;
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the skeleton
    h += 70.0 * c.crackAt(40.0, g.sF + 4, 0.08, 1.0).h * (0.5 + 0.5 * basin);   // wrinkle ridges, long and low: the crust shrank on the core (Mercury's)
    c.craterStack(h);
    s.height = h;
    double crest = c.rg(30.0, g.sC + 3, 3);
    if (basin > 0.5 || cm.flooded > 0.5 || (rel < 0.3 && patch > 0.25)) { s.material = MAT_DUST; s.albedo = 0.40 + 0.06 * base + 0.08 * patch; }
    else if (crest > 0.55 && rel + rf.slopeM > 0.2) { s.material = MAT_METAL; s.albedo = 0.26 + 0.1 * clampd(rimBright, 0, 1); }
    else { s.material = MAT_BASALT; s.albedo = 0.17 + 0.05 * (0.5 + 0.5 * base) + 0.04 * patch + 0.08 * clampd(rimBright, 0, 1); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}

// P-01: a scorched airless world: the cratered ground with its maria sheets of black glass (cooling cracks, conchoidal ripples), glass
// flows tonguing from obsidian domes, fields of glass beads round the sheets, the highlands' regolith (glass worlds)
void groundGlass(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& p = c.p; SurfaceSample& s = c.s; CraterMarks& cm = c.cm; const double& rimBright = c.cm.rimBright;
    const double R = c.R, detailM = c.detailM, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.5, g.sA, 5);
    double mare = smoothstep(0.02, 0.22, base);   // the sheets: a third of the world
    double h = base * g.mountainAmp * (1 - 0.8 * mare);
    double rel = (1 - mare) * massif, hf = std::max(hillsF, 0.6 * (1 - mare));
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf) * (1 - 0.8 * mare));   // the skeleton (the sheets smooth the relief)
    double cr = 0;
    c.craterStack(cr);
    h += cr * (1 - 0.6 * mare);
    double dome = 0;
    if (scaleVisible(18.0, detailM)) dome = domeField(p, R, 18.0, g.volcanoes, g.sD + 3, 0.25, 260.0, 0.75);   // obsidian domes, flat-topped
    LavaOut lf = lavaFlows(p, R, 18.0, g.volcanoes, g.sD + 3, detailM);   // and their flows (the domes' cells)
    h += dome + 2.0 * lf.dh;
    const double sheet = std::max(mare, clampd(cm.flooded, 0, 1));
    if (sheet > 0.4) {   // the sheets' cooling cracks and conchoidal ripples
        Crack ck = c.crackAt(0.35, g.sF + 5, 0.05, 1.0);
        h += (2.0 * c.rg(0.12, g.sF + 6, 2) - 1.5 * ck.h) * sheet;
    }
    s.height = h;
    const double beads = smoothstep(0.0, 0.15, c.fb(R * 0.08, g.sG + 4, 3)) * smoothstep(-0.14, -0.06, base) * (1 - smoothstep(0.06, 0.1, base));   // fields of them in a band round the sheets' edges
    if (sheet > 0.5 || lf.flow > 0.4 || dome > 30) { s.material = MAT_GLASS; s.albedo = 0.06 + 0.04 * clampd(rimBright, 0, 1) + 0.25 * clampd(cm.rays, 0, 1); }
    else if (beads > 0.5) { s.material = MAT_SAND; s.albedo = 0.22 + 0.05 * base; }
    else { s.material = MAT_DUST; s.albedo = 0.34 + 0.08 * base + 0.12 * clampd(rimBright, 0, 1) - 0.06 * clampd(cm.floorDark, 0, 1) + 0.3 * clampd(cm.rays, 0, 1); }
    s.relief = clampd(std::fabs(cr) / 400.0 + rel + 0.5 * rf.slope, 0, 1);
}

// P-01: under the steam: the plates' highlands cut by scalding rivers, boiling lakes in their hollows, mud in the valleys, sinter round
// the vents; the lowlands sinking into the supercritical fog (the row's `lowFogM`), seas boiling toward the poles (steam worlds)
void groundSteam(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const Tectonics& tec = c.tec; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, absLatDeg = c.absLatDeg, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * g.mountainAmp + 400.0 + tec.uplift * 900.0;
    { double chainR = c.rg(45.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.6) - tec.rift * 900.0 + tec.ridge * 400.0; }
    double zone = clampd(smoothstep(0.1, 0.6, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.6 * zone);
    double relief0 = clampd(rel + 0.25 * hf, 0, 1);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the skeleton
    c.craterStack(h);
    double sinter = 0;   // fields of vents on the high ground, a low mound of sinter round each (`steamVents`: the bursts stand on their centres)
    if (h > 300 && matVisible(0.12, detailM)) {
        const double field = smoothstep(0.2, 0.4, c.fb(R * 0.05, g.sG + 3, 3));
        if (field > 0) visitFeatures(p, R, 2.0, 0.5, g.sF + 8, 0.1, [&](const FeatureHit& f) {
            const double r = 0.06 + 0.12 * f.u[3], x = f.d / r;
            if (x >= 1.0) return;
            sinter = std::max(sinter, field * smoothstep(1.0, 0.7, x));
            h += 4.0 * (1 - x * x) * field * featFade(2 * r, detailM);
        });
    }
    applyDrainage(g, unit, detailM, h, nullptr, rf.slope, true, true, 0.6, DA);   // scalding rivers, boiling lakes
    s.height = h;
    const double capEdge = g.iceCapLat + 4.0 * gnoise3(p / (R * 0.2), g.sF + 7);   // the polar seas from here poleward
    if (h < 0 && absLatDeg > capEdge) { s.material = MAT_WATER; s.water = 0; s.albedo = 0.24; }
    else if (DA.level > -1e8 && h < DA.level) { s.material = MAT_WATER; s.water = DA.level; s.albedo = 0.24; }
    else if (sinter > 0.5) { s.material = MAT_SALT; s.albedo = 0.75; }
    else if ((h < 250 && relief0 < 0.4) || DA.bed > 0.3 || DA.bank > 0.4) { s.material = MAT_MUD; s.albedo = 0.30 + 0.06 * base; }   // the lowlands and the rivers' banks
    else { s.material = MAT_ROCK; s.albedo = 0.26 + 0.08 * base + 0.04 * clampd(rf.slope * 2.0, 0, 1); }
    s.relief = clampd(relief0 + 0.5 * rf.slope, 0, 1);
}

// P-01: a dried ocean: under the old sea's level (the row's sea level, metres) the floor is evaporites in bands by depth (gypsum along the
// old shores, halite, potash in the deeps where the last brine dried), brine pools in the deepest hollows, salt domes rising through the
// floor and the glaciers of salt flowing from them, pillars by the shores; gypsum dunes over the shores, the uplands dry (salt worlds)
void groundSalt(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright; const Tectonics& tec = c.tec; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, lat = c.lat, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.5, g.sA, 5);
    double h = base * 1400.0 + tec.uplift * 600.0;
    { double chainR = c.rg(40.0, g.sC + 2, 6); h += tec.chain * (chainR * chainR * g.chainAmp * 0.6) - tec.rift * 700.0 + tec.ridge * 300.0; }
    double zone = clampd(smoothstep(0.2, 0.7, base) + 0.6 * tec.chain, 0, 1), rel = zone * massif, hf = std::max(hillsF, 0.5 * zone);
    double relief0 = clampd(rel + 0.25 * hf, 0, 1);
    double hRel, hS; ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf), &hRel, &hS);   // the skeleton; hRel the ground before its relief
    const double L = g.seaLevel, depth = std::max(0.0, L - hRel);
    const double oldSea = smoothstep(L + 2.0, L - 2.0, hRel);
    const double bed = L - 3.0 - depth * 0.3 + rf.h * 0.08;   // the floor keeps a tenth of the relief
    if (oldSea > 0) { h += (bed - h) * oldSea; hS += (bed - hS) * oldSea; }
    double dome = 0, pillar = 0, brine = 0;
    LavaOut glacier;
    if (oldSea > 0.3) {   // the domes and their glaciers (the domes' cells)
        if (scaleVisible(14.0, detailM)) dome = domeField(p, R, 14.0, 0.3, g.sD + 7, 0.25, 160.0, 0.8) * oldSea;
        glacier = lavaFlows(p, R, 14.0, 0.3, g.sD + 7, detailM);
        h += dome + 1.5 * glacier.dh * oldSea;
    }
    const double potashDepth = 650.0 + 250.0 * c.fb(R * 0.08, g.sG + 9, 2);   // only the deepest floor held the last brine
    if (oldSea > 0.5 && depth < 60.0 && scaleVisible(0.4, detailM)) { pillar = domeField(p, R, 0.4, 0.25, g.sD + 9, 0.06, 14.0, 0.0); h += pillar; }
    if (oldSea > 0.5 && depth > 120.0 && (scaleVisible(0.25, detailM) || matVisible(0.06, detailM))) {   // the pools on the salt, tens of metres across, flat a metre under the floor
        Worley3 bw = worley3(p / 0.25, g.sF + 10);
        if (unitFromHash(bw.id1) < 0.15) { brine = smoothstep(0.3, 0.18, bw.f1); h += (bed - 1.0 - h) * smoothstep(0.3, 0.6, brine); }
    }
    if (oldSea < 0.5 && hRel > L && hRel - L < 80.0) h += duneField(p, g, lat, g.duneAmp, detailM) * smoothstep(80.0, 20.0, hRel - L);   // gypsum dunes over the shores
    c.craterStack(h);
    applyDrainage(g, unit, detailM, h, &hS, rf.slope, false, false, 0.5, DA);   // wadis down to the floor, playas
    s.height = h;
    if (oldSea > 0.5) {
        if (brine > 0.5) { s.material = MAT_BRINE; s.albedo = 0.34; }
        else if (dome > 40 || glacier.flow > 0.4 || pillar > 4) { s.material = MAT_SALT; s.albedo = 0.80; }
        else if (depth < 40.0 + 20.0 * c.fb(R * 0.05, g.sG + 11, 2)) { s.material = MAT_DUST; s.albedo = 0.68 + 0.05 * base; }   // gypsum, the first to settle
        else if (depth < potashDepth) { s.material = MAT_SALT; s.albedo = 0.86 + 0.04 * base; }
        else { s.material = MAT_POTASH; s.albedo = 0.55 + 0.05 * base; }
    } else if (DA.playa > 0.3) { s.material = MAT_SALT; s.albedo = 0.85; }
    else if (relief0 > 0.5 || DA.bed > 0.4) { s.material = MAT_ROCK; s.albedo = 0.45 + 0.1 * base; }
    else { s.material = MAT_DUST; s.albedo = 0.56 + 0.1 * base + 0.08 * clampd(rimBright, 0, 1); }
    s.oldSea = oldSea;
    s.relief = clampd(relief0 + 0.5 * rf.slope, 0, 1);
}

// P-02: the lichen world: the thin air's ground, its life in crusts and mosses. Moss in the wet hollows (the basins' floors, the dry valleys'
// beds and banks, the meltwater under the ice caps' edges), crusts in patches on the stable ground round them (rock, gravel, the dark lava)
// and more of them toward the water; by unfaded fields, so every ring and the map agree
void groundLichen(GroundCtx& c) {
    groundThinAir(c);
    const BodyGen& g = c.g; SurfaceSample& s = c.s; const DrainApply& DA = c.DA;
    if (s.material == MAT_ICE) return;
    const double capEdge = g.iceCapLat + 5.0 * gnoise3(c.p / (c.R * 0.15), g.sF + 7);   // `groundThinAir`'s
    const double melt = smoothstep(capEdge - 3.5, capEdge - 0.3, c.absLatDeg);
    const double wet = std::max(std::max(DA.playa, DA.bed), std::max(0.7 * DA.bank, std::max(0.8 * DA.wetness, melt)));
    const double patch = fbm3(c.p / 0.09, g.sG + 41, 2), hollow = fbm3(c.p / 0.6, g.sG + 42, 2);
    if (wet > 0.35 || (hollow < -0.35 && patch > -0.1)) { s.material = MAT_MOSS; s.albedo = 0.16 + 0.05 * patch; }
    else if (patch > 0.12 - 0.45 * wet && (s.material == MAT_ROCK || s.material == MAT_DUST || s.material == MAT_BASALT)) { s.material = MAT_CRUST; s.albedo = 0.26 + 0.06 * patch; }
}

// P-03: an eyeball: the ocean's one sea locked to a red dwarf and frozen over but within its reach of the substellar point (the pupil, its
// shore wandering by a few degrees), floes scattered over the last three degrees of the open water (the iris, a pale ring from orbit), a band
// of brash a few kilometres wide along the ice front, and past the front the sea glacier: unbroken ice standing 8-20 m over the water at its
// cliff and rising slowly away from the sun under its snow, pressure ridges along the old floes' seams, blue ice where the wind scours it
void groundEyeball(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s;
    const double R = c.R, detailM = c.detailM;
    const Vec3 sub = g.locked ? g.lockedDir : Vec3(1, 0, 0);
    const double ang = std::acos(clampd(dot(unit, sub), -1, 1)) / DEG;   // degrees from the substellar point
    const double edge = g.seaReachDeg + 4.0 * gnoise3(p / (R * 0.2), g.sF + 7), front = edge + 0.05;   // the front 5-7 km past the pupil's edge
    const double swell = c.fb(2.0, g.sB, 4) * g.hillAmp;
    const double cover = 0.3 * smoothstep(edge - 3.0, edge, ang) + 0.6 * smoothstep(edge - 0.02, front, ang);   // the share of the plates that are ice
    const double shelf = smoothstep(0.0, 0.04, cover);         // the iris lies on a shelf (the ocean's B-322: the plates rise through an ice foot)
    const double floor = (-150.0 + swell) + ((-6.5 + 0.3 * swell) - (-150.0 + swell)) * shelf;
    s.height = floor; s.material = MAT_WATER; s.water = 0; s.albedo = 0.26; s.biome = BIO_OCEAN;
    if (ang > front) {
        const double over = ang - front, wallDeg = 30.0 / (DEG * R * 1000.0);
        const double cliff = 8.0 + 12.0 * (0.5 + 0.5 * gnoise3(p / 3.0, g.sF + 9));
        double h = (cliff + 0.35 * over) * smoothstep(0.0, wallDeg, over);   // the front's cliff over its first 30 m, then a few tens of metres toward the night
        if (scaleVisible(1.5, detailM)) { const Worley3 fl = worley3(p / 1.5, g.sF + 3); h += 3.5 * smoothstep(0.035, 0.0, fl.f2 - fl.f1) * lodFade(1.5, detailM); }   // the old floes' seams
        h += 0.7 * fbm3(p / 0.12, g.sF + 72, 2) * lodFade(0.12, detailM);   // sastrugi
        s.height = h; s.biome = BIO_ICE;
        const bool blue = fbm3(p / 0.9, g.sF + 74, 2) > 0.36 + 0.2 * smoothstep(90.0, 130.0, ang);   // unfaded; fewer on the night side, under its snow
        s.material = blue ? MAT_ICE : MAT_SNOW; s.albedo = blue ? 0.60 : 0.88;
        return;
    }
    if (cover > 0) {   // the floes: plates of 0.6 km with leads of water between them, the brash's thicker
        const Worley3 fl = worley3(p / 0.6, g.sF + 3);
        if (unitFromHash(fl.id1) < cover) {
            const double d = fl.f2 - fl.f1, rim = smoothstep(0.0, 0.2, d), top = 0.7 + 0.3 * smoothstep(0.2, 0.4, d) + 0.6 * smoothstep(edge - 0.02, front, ang);
            const double h = floor + (top - floor) * rim;
            s.height = h;
            if (h > -0.3) { s.material = MAT_ICE; s.albedo = 0.84; s.biome = BIO_ICE; }
            else s.albedo = 0.26 + 0.2 * clampd((h + 6.5) / 6.5, 0, 1);
        }
    }
}

// P-03: ammonia seas: the methane world's rounded hills of ice and rock (no dunes: no tholin rains here), its channels and domes, a sea of
// ammonia water at the reference level that never freezes, slabs of ice afloat in its shallows (the ocean's floes, few and apart, on a
// shelf near the shore), dark silt along the shores and ammonia frost over the hills' frost line, lower toward the poles
void groundAmmonia(GroundCtx& c) {
    const BodyGen& g = c.g; const Vec3& unit = c.unit; const Vec3& p = c.p; SurfaceSample& s = c.s; const double& rimBright = c.cm.rimBright; DrainApply& DA = c.DA;
    const double R = c.R, detailM = c.detailM, absLatDeg = c.absLatDeg, massif = c.massif, hillsF = c.hillsF;
    double base = c.fb(R * 0.45, g.sA, 5);
    double h = base * 700.0 + 60.0 - 160.0 * smoothstep(45.0, 75.0, absLatDeg);   // the seas reach furthest toward the poles
    double zone = smoothstep(0.2, 0.7, base), rel = zone * massif * 0.6, hf = std::max(hillsF, 0.5 * zone);
    ReliefSum rf = c.relief(h, rel, hf, reliefAmp(g, rel, hf));   // the skeleton
    const double chan = c.crackAt(30.0, g.sF + 4, 0.06, 1.0).h;   // channels deepening toward the sea
    h -= chan * 25.0 * smoothstep(400.0, 50.0, h);
    if (scaleVisible(25.0, detailM)) h += domeField(p, R, 25.0, 0.08, g.sD + 21, 0.25, 180.0, 0.0);   // cryovolcanic domes
    c.craterStack(h);
    bool slab = false;
    if (h < 0 && h > -14.0) {   // the shallows: slabs on a shelf (the floor at 6 m under the plates' zone), 30-60 m in from their edges at 0.6 m freeboard
        const Worley3 fl = worley3(p / 0.5, g.sF + 13);
        const double near = smoothstep(-14.0, -6.0, h);
        if (unitFromHash(fl.id1) < 0.32 * near) {
            const double floorS = h + (-6.0 - h) * near, d = fl.f2 - fl.f1, rim = smoothstep(0.0, 0.2, d);
            const double hs = floorS + (0.6 + 0.2 * smoothstep(0.2, 0.4, d) - floorS) * rim;
            if (hs > h) { h = hs; slab = h > -0.3; }
        }
    }
    applyDrainage(g, unit, detailM, h, nullptr, rf.slope, true, true, 0.5, DA);   // rivers of ammonia water to the sea, lakes in the hollows
    s.height = h;
    const double frostLine = g.snowLine * (1.0 - 0.7 * smoothstep(20.0, 80.0, absLatDeg)) + 90.0 * gnoise3(p / 6.0, g.sF + 15);
    if (slab) { s.material = MAT_ICE; s.albedo = 0.80; }
    else if (DA.level > -1e8 && h < DA.level) { s.material = MAT_WATER; s.water = DA.level; s.albedo = 0.24; }
    else if (h > frostLine) { s.material = MAT_SNOW; s.albedo = 0.82 + 0.06 * base; }
    else if (h < 6.0 && h > -30.0 && rf.slopeM < 0.12) { s.material = MAT_SAND; s.albedo = 0.26 + 0.04 * base; }   // the shores' silt
    else { s.material = MAT_ROCK; s.albedo = 0.40 + 0.08 * base + 0.06 * clampd(rimBright, 0, 1); }
    s.relief = clampd(rel + 0.25 * hf + 0.5 * rf.slope, 0, 1);
}
