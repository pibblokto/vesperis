// E-08: the sky from the ground: the galaxy's band (`buildBandMap`, `bandAt`), `drawSky` (the day's light and the suns' glows,
// the halos, the clouds and the fronts' decks, the aurorae, the companion sun, the ring's arc, a comet's coma, the wall of
// rain; kept whole), the shower meteors and the veiling glare (`drawVeil`)
#include "surface_internal.h"

// The galaxy seen from inside, integrated once per landing (shared code in galaxy/starfield).
void SurfaceView::buildBandMap() {
    Vec3 obs = site.worldPos(0, 0, 0, 0) / SECTOR_KM;
    buildGalaxyBand(obs, bandMap, BAND_MAP_W, BAND_MAP_H);
    nebN = nebulaPatches(obs, nebP, 16);   // N5-01
}
double SurfaceView::bandAt(const Vec3& d) const { return sampleGalaxyBand(bandMap, d); }

// F-10: the sky's powers of a cosine as products of squares (libm's pow was the sky's largest cost; these are within a few dozen ulp of
// it): the day's two glows round a sun, 9 c^6 + 14 c^50; the sun's disc in an airless sky, c^300; a comet's tail, c^18
static inline double sunGlow(double c) { const double c2 = c * c, c6 = c2 * c2 * c2, c12 = c6 * c6, c24 = c12 * c12; return c6 * 9 + c24 * c24 * c2 * 14; }
static inline double pow300(double c) { const double c2 = c * c, c6 = c2 * c2 * c2, c12 = c6 * c6, c24 = c12 * c12, c50 = c24 * c24 * c2, c150 = c50 * c50 * c50; return c150 * c150; }
static inline double pow18(double c) { const double c2 = c * c, c6 = c2 * c2 * c2; return c6 * c6 * c6; }

void SurfaceView::drawSky(Framebuffer& fb, double t, const std::vector<Star>& stars, SpaceRenderer& sr) {
    const Body& b = site.sys->bodies[site.body];
    SurfaceLook L = lookFor(site.gen, site.sys->star);
    Mat3 camT = camLocal.transposed();
    const Vec3 sd = env.sun.dirLocal;
    double sinAlt = sd.y;
    double day = env.skyBrightness;
    double sunUp = smoothstep(-0.28, 0.0, sinAlt) * env.glow;   // B-425: a far sun alone up glows as faintly as it lights
    bool venus = hasOpaqueDeck(b.type);   // R-307: the hydrocarbon haze is a deck like the venusian clouds
    double density = L.skyDensity * (1 - 0.3 * env.rain);
    const double limbHaze = KINDS[b.type].limbHaze;
    // Q-02 (TR_ICE_HALOS): an air whose ice crystals hang aloft whatever the ground's warmth: the 22-degree ring brighter and every day the
    // sun is up through a sky not overcast, the parhelia (sundogs) beside the sun at its height, the parhelic circle through it round the
    // sky, the 46-degree ring faint, and at night the ring round the brightest moon
    const bool iceHalos = site.gen.hasTrait(TR_ICE_HALOS) && site.atmosphere && !venus;
    double halo = (site.atmosphere && !venus && env.temperatureC < -5 && env.cloudCover < 0.35 && sinAlt > 0.05) ? 0.8 : 0.0;
    if (iceHalos && env.cloudCover < 0.85 && sinAlt > 0.02) halo = 1.5 * (1 - 0.6 * env.rain);
    Vec3 dogL, dogR; double dogs = 0, moonHalo = 0;
    if (iceHalos && halo > 0 && sinAlt < 0.86) {   // the plates' parhelia: at the sun's height, a little outside the ring as the sun climbs (none past 60 degrees)
        const double h = std::asin(clampd(sinAlt, -1, 1)), rp = (22.0 + 0.15 * std::max(0.0, h / DEG)) * DEG;
        const double ch = std::cos(h), dA = std::acos(clampd((std::cos(rp) - sinAlt * sinAlt) / std::max(1e-6, ch * ch), -1, 1)), az = std::atan2(sd.x, sd.z);
        dogL = Vec3(ch * std::sin(az - dA), sinAlt, ch * std::cos(az - dA)); dogR = Vec3(ch * std::sin(az + dA), sinAlt, ch * std::cos(az + dA));
        dogs = halo * smoothstep(0.86, 0.6, sinAlt);
    }
    if (iceHalos && env.moonBody >= 0 && env.moonLight > 0.004 && sinAlt < -0.05) moonHalo = clampd(std::sqrt(env.moonLight / 0.05), 0, 1) * (1 - env.rain) * (1 - 0.7 * env.cloudCover);   // a moon of a twentieth of the sun's light at its full
    const Vec3 moonD = env.moonDir;
    const double cosMoonRingLo = std::cos(24.01 * DEG), cosMoonRingHi = std::cos(19.99 * DEG);
    // Q-02 (TR_NOCTILUCENT): silver-blue clouds 83 km up in a high latitude's summer twilight, lit from below the horizon after the sun has
    // set (6-16 degrees under it) while the air under them is in the planet's shadow: low in the sky toward the sun's side, in bands and billows
    double nlc = 0, nlcLimitDeg = 0;
    const double nlcH = 83.0, RkmS = site.R / 1000.0;
    if (site.gen.hasTrait(TR_NOCTILUCENT) && site.atmosphere && !venus) {
        const double dep = -std::asin(clampd(sinAlt, -1, 1)) / DEG, summer = clampd(0.5 + (env.latDeg >= 0 ? site.season : -site.season), 0, 1);
        nlc = smoothstep(3.5, 6.5, dep) * smoothstep(17.0, 12.5, dep) * smoothstep(35.0, 50.0, std::fabs(env.latDeg)) * (0.45 + 0.55 * summer) * (1 - env.rain) * (1 - 0.85 * env.cloudCover);
        nlcLimitDeg = std::acos(RkmS / (RkmS + nlcH)) / DEG;   // the depression past which the layer lies in the shadow (9 degrees on a world Earth's size)
    }
    // N5-02: a sun pillar in cold, clear air when the sun is low (ice crystals in the air mirror it into a column)
    double pillar = (site.atmosphere && !venus && env.temperatureC < -5 && env.cloudCover < 0.45 && sinAlt > -0.03 && sinAlt < 0.2) ? (1 - std::max(0.0, sinAlt) / 0.2) : 0.0;
    double sunAltA = std::asin(clampd(sinAlt, -1, 1));
    double wedge = (site.atmosphere && !venus && sinAlt > -0.17 && sinAlt < 0.06) ? 1.0 - std::fabs(sinAlt + 0.05) / 0.11 : 0.0;
    if (wedge < 0) wedge = 0;
    Vec3 sdh = normalize(Vec3(sd.x, 0, sd.z + 1e-9));
    // cloud grid
    bool clouds = site.atmosphere && L.cloudsLayer && (env.cloudCover > 0.03 || localFront.on);   // W-03: a front's deck in view counts
    double aurora = env.aurora;
    const int CELL = 4 * FB_SCALE;
    const int GX = FBW / CELL + 1, GY = FBH / CELL + 1;
    if (clouds) {
        cloudGrid.assign(GX * GY * 3, 0.f);   // per cell: the density, the index, and (W-03) the share that is a front's deck
        double thrP = 1.0 - env.cloudPattern * 0.75;   // W-03: the pattern's threshold; a front's deck is read per cell from its line
        double wx = windDriftX, wz = windDriftZ;
        // two layers (M10-14): low cumulus at 2600 m with lit tops and shaded bases, high thin veils at 6000 m. F-05: the grid's rows on the
        // pool, as the aurora's (a cell reads pure functions and writes its own three floats)
        parallelFor(GY, 4, [&](int gy0, int gy1) {
        for (int gy = gy0; gy < gy1; gy++)
            for (int gx = 0; gx < GX; gx++) {
                int px = std::min(gx * CELL, FBW - 1), py = std::min(gy * CELL, FBH - 1);
                Vec3 d = camT * dirLUT[py * FBW + px];
                double el = d.y;
                float dens = 0, idx = 0, storm = 0;
                if (el > 0.012) {
                    double tt = 2600.0 / el;
                    double x = camPos.x + d.x * tt + wx, z = camPos.z + d.z * tt + wz;
                    double thr = thrP, lp = 0, veil = 0, fcl = 0;
                    if (localFront.on) {   // W-03: the front's deck where this cell's ray meets the layer (the band moves with the front, its texture with the wind), and its veil ahead
                        double xw = camPos.x + d.x * tt, zw = camPos.z + d.z * tt;
                        fcl = localFront.cloud(xw, zw);
                        thr = 1.0 - std::max(env.cloudPattern, fcl) * 0.75;
                        lp = localFront.precip(xw, zw);
                        veil = localFront.veil(xw, zw);
                    }
                    double n = 0.5 + 0.5 * fbm2(x / 1800.0, z / 1800.0, site.gen.seed + 77, 4, 2.2, 0.55);
                    double dLow = smoothstep(thr, thr + 0.18, n) * smoothstep(0.012, 0.09 - 0.05 * clampd(fcl, 0, 1), el);   // W-03: a front's solid deck is drawn nearer the horizon (a dark band over it when the wall is far)
                    double cosSun = dot(d, sd);
                    // denser toward the sun means this part of the cloud is its shaded underside
                    double nSun = 0.5 + 0.5 * fbm2((x + sd.x * 260) / 1800.0, (z + sd.z * 260) / 1800.0, site.gen.seed + 77, 3, 2.2, 0.55);
                    double shadeSide = clampd((nSun - n) * 4.0, 0, 1) * (0.6 + 0.4 * sd.y);
                    double lit = 0.62 + 0.38 * cosSun - 0.32 * shadeSide;
                    double structure = 0.8 + 0.35 * clampd((n - thr) / 0.35, 0, 1) - 0.15 * clampd((n - thr - 0.4) / 0.3, 0, 1);
                    double idxLow = day * (33 + 9 * lit) * structure + (1 - day) * 4;
                    // high veil: thin, bright, slow
                    double th = 6000.0 / el;
                    double hx = camPos.x + d.x * th + wx * 0.4, hz = camPos.z + d.z * th + wz * 0.4;
                    double nh = 0.5 + 0.5 * fbm2(hx / 4200.0 + 31.0, hz / 4200.0, site.gen.seed + 78, 3, 2.0, 0.5);
                    double thrH = thr + 0.05 - 0.35 * veil;   // W-03: the milky veil hundreds of kilometres ahead of a front
                    double dHigh = smoothstep(thrH, thr + 0.3, nh) * (0.4 + 0.3 * veil) * smoothstep(0.012, 0.06, el);
                    double idxHigh = day * (38 + 6 * cosSun) + (1 - day) * 5;
                    double dAll = dLow + dHigh * (1 - dLow);
                    dens = (float)dAll;
                    storm = (float)(clampd(fcl * 1.5, 0, 1) * (dAll > 1e-6 ? dLow / dAll : 0.0));   // W-03: the deck's share of the cell (the veil is the sky's)
                    idx = (float)(dAll > 1e-6 ? (idxLow * dLow + idxHigh * dHigh * (1 - dLow)) / dAll : 0);
                    { double rr = std::max(env.rain, lp); if (rr > 0 || fcl > 0) idx *= (float)std::max(0.3, 1 - 0.35 * rr - 0.35 * fcl - 0.1 * lp); }   // W-03: a front's deck is dark underneath, darker over its rain (without a front: the rain's 0.35 as before)
                }
                cloudGrid[(gy * GX + gx) * 3] = dens;
                cloudGrid[(gy * GX + gx) * 3 + 1] = idx;
                cloudGrid[(gy * GX + gx) * 3 + 2] = storm;
            }
        });
    }
    // B-403: the aurora as curtains. Three sheets of light hang 90-250 km up along the auroral oval, which lies a few degrees
    // of latitude poleward of the site (over it and beyond, inside the oval): slabs 10-16 km thick, their light a gamma profile
    // in height (nothing under 90 km, a bright lower border at 114, mostly gone by 250 with a faint tail to 400), folded by two slow
    // waves, rayed by a noise along them, breathing. A cell's value is the light along its ray: the heights where the ray
    // enters and leaves each slab give the profile's integral, divided by the ray's climb at the border (the ground curving
    // away under it), so a sheet seen along its length is bright and one crossed steeply is faint; a ray under the border
    // sees nothing. Sampled on the cloud grid and drawn after the stars, on sky pixels only, with no threshold: it fades into
    // the night sky. It used to be five lobes of azimuth, opaque and hard-edged, which met in a flower at the zenith.
    const bool auroraOn = aurora > 0.02 && day < 0.4;
    if (auroraOn) {
        auroraGrid.assign(GX * GY * 2, 0.f);
        const double Rkm = site.R / 1000.0, kmDeg = Rkm * DEG;
        const double pole = env.latDeg >= 0 ? 1.0 : -1.0;
        const double storm = env.auroraStorm;
        // km poleward of the site to the oval's centre (nearer the pole on a strong field, pushed equatorward by a storm), kept
        // within sight on every world, drifting over five minutes
        const double P0 = clampd((auroraOvalNow(b) - 6 * storm - std::fabs(env.latDeg)) * kmDeg, -120.0, 450.0) + 30.0 * std::sin(t / 310.0);   // Q-02: a great aurorae's oval lower
        lastAuroraP0 = P0;
        // the profile in height, in two parts: the lower curtain f(u) = u / H^2 exp(-u / H), u = height - h0, its peak at h0 + H
        // (H 24 km: nothing under 90 km, a bright border at 114, gone by 200), in bank 11's colour; and the tops, the same shape
        // with H 70 km reaching 400 km, in bank 21's. G is a part's integral from the ground
        const double h0 = 90.0, H = 24.0, H2 = 70.0;
        auto G = [](double u, double Hk) { return u <= 0 ? 0.0 : 1.0 - std::exp(-u / Hk) * (1 + u / Hk); };
        struct Sheet { double off, w, gain, ph; };
        // km from the centre, half-thickness, gain, phase: a storm brings the outer sheets up
        const Sheet sheets[3] = {{0, 5, 1.0, 0.0}, {150, 4, 0.3 + 0.4 * storm, 2.1}, {-110, 3.5, 0.25 + 0.5 * storm, 4.2}};
        const double camKmX = camPos.x / 1000.0, camKmZ = camPos.z / 1000.0;
        const double hPeak = h0 + H;
        // the tops: faint on a quiet night, their colour taking over the heights in a storm, more on some worlds than others
        const double topGain = (0.25 + 0.75 * storm) * (0.6 + 0.8 * unitFromHash(hashCombine(site.gen.seed, 0xA0D))) * (site.gen.hasTrait(TR_GREAT_AURORAE) ? 1.5 : 1.0);   // Q-02: a strong field's red tops
        parallelFor(GY, 4, [&](int gy0, int gy1) {
        for (int gy = gy0; gy < gy1; gy++)
            for (int gx = 0; gx < GX; gx++) {
                int px = std::min(gx * CELL, FBW - 1), py = std::min(gy * CELL, FBH - 1);
                Vec3 d = camT * dirLUT[py * FBW + px];
                double el = d.y;
                if (el < 0.005) continue;
                double hl = std::sqrt(std::max(1e-9, 1 - el * el));
                // the height over the curved ground at slant s is s el + (s hl)^2 / (2 R); the ray's climb per km of slant at the border
                double a = hl * hl / (2 * Rkm);
                double sPeak = 2 * hPeak / (el + std::sqrt(el * el + 4 * a * hPeak));
                double climb = el + 2 * a * sPeak;
                double dz = d.z * pole;   // the ray's rate of going poleward
                if (std::fabs(dz) < 1e-4) dz = dz < 0 ? -1e-4 : 1e-4;
                // the light along the ray through a slab |P - Pf| < w, both parts: the profiles' integrals between the heights
                // where the ray enters and leaves it, over the climb
                auto slab = [&](double Pf, double w, double wgt, double& low, double& top) {
                    double sA = (Pf - w) / dz, sB = (Pf + w) / dz;
                    if (sA > sB) std::swap(sA, sB);
                    if (sB <= 0) return;      // the slab lies behind
                    if (sA < 0) sA = 0;       // the ray starts inside it
                    double uA = sA * el + sA * sA * a - h0, uB = sB * el + sB * sB * a - h0;
                    low += wgt * (G(uB, H) - G(uA, H)) / climb;
                    top += wgt * (G(uB, H2) - G(uA, H2)) / climb;
                };
                double E = sPeak * d.x + camKmX;   // km east along the sheets where the ray reaches the border's height
                double sumLow = 0, sumTop = 0;
                for (const Sheet& sh : sheets) {
                    double Pc = P0 + sh.off - camKmZ * pole;
                    double Pf = Pc + 14 * std::sin(E / 45.0 + t * 0.07 + sh.ph) + 6 * std::sin(E / 16.0 - t * 0.11 + 2 * sh.ph);   // the folds
                    double low = 0, top = 0;   // a dense core in thinner envelopes: soft edges
                    slab(Pf, sh.w, 0.5, low, top); slab(Pf, 1.7 * sh.w, 0.3, low, top); slab(Pf, 2.6 * sh.w, 0.2, low, top);
                    if (low + top < 1e-4) continue;
                    // the rays: two octaves of noise along the sheet, read where the ray crosses it (so from under a curtain they
                    // converge toward the zenith), strong on a sheet near by, a far arc a smooth band; a ray running along the
                    // sheet passes many of them and sees their average
                    double Er = clampd(std::fabs(Pf / dz), 0.6 * sPeak, 2.5 * sPeak) * d.x + camKmX;
                    double rayAmp = 0.7 * clampd(1.3 - sPeak / 450.0, 0.2, 1.0) * clampd(1.2 - sh.w * std::fabs(d.x / dz) / 20.0, 0.25, 1.0);
                    double n = 0.5 + 0.67 * (gnoise2(Er / 12.0, t / 40.0 + sh.ph, site.gen.seed + 46) + 0.5 * gnoise2(Er / 4.5, t / 25.0 + sh.ph, site.gen.seed + 47));
                    double rays = 1 - rayAmp * (1 - clampd(n, 0, 1));
                    // the patches: a slow noise along the sheet thins it to a fifth here and there; a breathing over seventeen seconds
                    double patch = 0.2 + 0.8 * std::pow(clampd(0.5 + 0.67 * gnoise2(E / 260.0, t / 120.0 + sh.ph, site.gen.seed + 48), 0, 1), 1.4);
                    double breath = 0.7 + 0.3 * std::sin(t / 17.0 + sh.ph);
                    double m = sh.gain * rays * patch * breath;
                    sumLow += m * low; sumTop += m * top * topGain;
                }
                double sum = (sumLow + sumTop) * (1 - env.rain) * (1 - env.dust);   // rain and a dust storm take it away, as the rain takes the stars (the night clouds hide neither: KI-336)
                if (sum > 1e-4) {
                    auroraGrid[(gy * GX + gx) * 2] = (float)(44.0 * (1 - std::exp(-4.0 * aurora * sum)));   // a soft knee: the sheets seen along their length saturate, a steep crossing stays faint
                    auroraGrid[(gy * GX + gx) * 2 + 1] = (float)(sumTop / (sumLow + sumTop));                  // the tops' share: the colour
                }
            }
        });
    }
    const int step = FB_SCALE >= 3 ? 2 : 1;   // the sky is smooth: half resolution at 3x and 4x
    Mat3 toWorld = site.localFrame(t).transposed();
    // M5-01: the companion sun's own glow in the sky
    const bool twoSuns = env.hasSun2 && env.sun2.dirLocal.y > -0.3;
    const Vec3 sd2 = twoSuns ? env.sun2.dirLocal : Vec3(0, -1, 0);
    const double sun2Up = twoSuns ? smoothstep(-0.28, 0.0, sd2.y) * clampd(env.sun2.lightFactor / std::max(env.sunDisc.lightFactor, 0.05), 0.15, 1.0) : 0.0;
    // M5-04: the ring plane of a ringed world seen from its own surface, an arc across the sky
    const bool ringArc = b.rings && !venus;
    Vec3 ringOrig; Vec3 ringN = b.spinAxis; Vec3 ringSunW; double ringR0 = 0, ringR1 = 0, ringLf = 1;
    const std::vector<float>* ringProf = nullptr;
    if (ringArc) {
        ringOrig = site.worldPos(t, camPos.x, camPos.z, camPos.y / 1000.0) - site.sys->bodyPos(site.body, t);   // km, from the centre
        ringSunW = toWorld * env.sunDisc.dirLocal;
        ringR0 = b.ringInner * b.radiusKm; ringR1 = b.ringOuter * b.radiusKm;
        ringProf = &sr.ringProfile(*site.sys, site.body);
        ringLf = clampd(env.sun.lightFactor, 0.3, 1.1);
    }
    // O0-01: which face of the ring we see: the lit face when the sun is on our side of the ring plane, else the
    // unlit face, dim with the light that comes through
    const double ringFace = ringArc ? ((dot(ringOrig, ringN) * dot(ringSunW, ringN) >= 0) ? 1.0 : 0.5) : 1.0;
    double ringPow[256];   // F-07: the arc's pow of each of the profile's 256 densities, once a frame
    if (ringArc) for (int i = 0; i < 256; i++) ringPow[i] = std::pow(clampd((double)(*ringProf)[i] * 1.3, 0, 1), 0.35);
    const double cosHaloLo = std::cos((22.0 + 1.401) * DEG), cosHaloHi = std::cos((22.0 - 1.401) * DEG);
    const double pillarElLo = std::sin(sunAltA - 2.001 * DEG), pillarElHi = std::sin(sunAltA + 11.001 * DEG), pillarTanAz = std::tan(2.501 * DEG);
    // O4: on a comet the coma hazes the whole sky, brightest toward the sun, and the ion tail streams from the anti-solar
    // point; both grow with the activity
    const bool comet = b.type == PT_COMET && env.cometActivity > 0.02;
    const double act = env.cometActivity;
    double knotT[5]; for (int k = 0; k < 5; k++) knotT[k] = SpaceRenderer::cometKnot(k, t);
    // W-03: the wall of rain (or dust) under a front's deck: toward the band the haze thickens to the horizon's grey between
    // the horizon and the cloud base, by the rain the ray crosses (`LocalFront::wall`, the same box the ground is fogged by)
    const bool curtain = site.atmosphere && localFront.on;
    const double curtainV = day * 20 + (1 - day) * 3;   // a dark wall of the sky's own colour, between the zenith's and the horizon's
    int rowsTotal = (FBH + step - 1) / step;
    parallelFor(rowsTotal, 40, [&](int rb, int re) {
    for (int y = rb * step; y < re * step && y < FBH; y += step) {
        for (int x = 0; x < FBW; x += step) {
            int o = y * FBW + x;
            Vec3 d = camT * dirLUT[o];
            double el = d.y;
            double v;
            double tailV = 0, stormW = 0;   // W-03: how much of the pixel is a front's deck or its rain (bank 23 past a half)
            double paleV = 0;   // Q-02: a noctilucent cloud's or the moon's ring's shade in the galactic band's bank (22: the sky's colour to a pale silver)
            if (!site.atmosphere) {
                // F-07: each pow only where its base is over 0 (elsewhere its term is +0, as the formula gives)
                double cosSun = dot(d, sd);
                v = cosSun > 0 ? pow300(cosSun) * 22 * env.glow : 0.0;
                if (twoSuns) { const double c2 = dot(d, sd2); if (c2 > 0) v += pow300(c2) * 22 * sun2Up; }
                if (comet) {
                    double cs = std::max(0.0, cosSun);
                    v += act * (6 + 14 * cs * cs * cs);
                    double tail = cosSun < 0 ? pow18(-cosSun) * act : 0.0;   // a cone some 15 degrees wide round the anti-solar point
                    if (tail > 0.02 && el > -0.05) {
                        // the knots of the ion tail stream outward: brightness along the anti-solar distance
                        double f = std::acos(clampd(-cosSun, -1, 1)) / (0.5 * PI);   // 0 at the anti-solar point .. 1 at 90 degrees
                        double knots = 0;
                        for (int k = 0; k < 5; k++) { double dd = f - knotT[k]; knots += std::exp(-dd * dd / (2 * 0.06 * 0.06)); }
                        tailV = 6 + 26 * tail * (0.8 + 0.3 * knots);
                    }
                }
            } else {
                double hz = el <= 0 ? 1.0 : std::pow(1 - clampd(el, 0, 1), 2.2);   // F-07: pow(1, 2.2) is 1 under the horizon
                double base;
                if (venus) base = 26 + 14 * hz;
                else base = day * (15 + 25 * hz) + (1 - day) * (2 + 5 * hz);
                base += 6.0 * day * smoothstep(0.09, 0.0, el);   // M1-07: haze band on the horizon
                base *= density;
                if (limbHaze > 0) base += limbHaze * day * smoothstep(0.08, 0.0, el);   // P-05: a haze's layers low over a dark sky
                double cosSun = dot(d, sd);
                double cs = std::max(0.0, cosSun);
                // F-07: the glows' pows only where the sun is up and the pixel faces it (elsewhere the term is +0)
                double g = sunUp > 0 && cs > 0 ? sunGlow(cs) * sunUp * (venus ? 1.6 : 1.0) : 0.0;
                if (twoSuns) { double cs2 = std::max(0.0, dot(d, sd2)); if (sun2Up > 0 && cs2 > 0) g += sunGlow(cs2) * sun2Up; }
                v = base + g;
                // M10-14: 22-degree halo in cold, clear air; twilight wedge opposite a low sun. F-07: the ring's and the pillar's tests
                // first on the cosine, the height and the azimuth's tangent (their bounds a thousandth of a degree outside the ones after)
                if (halo > 0 && cosSun > cosHaloLo && cosSun < cosHaloHi) { double ang = std::acos(clampd(cosSun, -1, 1)) / DEG; double dh = std::fabs(ang - 22.0); if (dh < 1.4) v += 5.0 * halo * (1 - dh / 1.4); }
                if (iceHalos && halo > 0) {   // Q-02: the sundogs, the parhelic circle, the 46-degree ring
                    if (dogs > 0) for (const Vec3& dg : {dogL, dogR}) {
                        const double cd = dot(d, dg);
                        if (cd > 0.9945) {   // within 6 degrees: a bright spot drawn out along the sun's height, a white tail outward
                            const double dAlt = (std::asin(clampd(el, -1, 1)) - std::asin(clampd(dg.y, -1, 1))) / DEG, dAz = std::acos(clampd(cd, -1, 1)) / DEG;
                            const double outward = dot(d, dg) > 0 && dot(d - dg, dg - sd) > 0 ? 1.0 : 0.0;
                            v += dogs * (17.0 * std::exp(-(dAz * dAz) / 0.6 - (dAlt * dAlt) / 0.3) + 5.0 * std::exp(-(dAz * dAz) / 4.0 - (dAlt * dAlt) / 1.2) + 3.0 * outward * std::exp(-std::fabs(dAlt) / 0.4) * std::exp(-dAz / 5.0));
                        }
                    }
                    const double dh = std::fabs(std::asin(clampd(el, -1, 1)) - sunAltA) / DEG;
                    if (dh < 0.7 && cosSun < 0.94) v += 2.2 * halo * (1 - dh / 0.7) * smoothstep(0.94, 0.85, cosSun);   // the parhelic circle, from past the ring
                    if (cosSun > cosHaloHi) v -= 2.5 * halo * smoothstep(cosHaloHi, 0.95, cosSun) * smoothstep(0.995, 0.97, cosSun);   // the sky inside the ring darker (no light is bent into it)
                    if (cosSun > 0.66 && cosSun < 0.72) { const double d46 = std::fabs(std::acos(clampd(cosSun, -1, 1)) / DEG - 46.0); if (d46 < 2.0) v += 1.8 * halo * (1 - d46 / 2.0); }
                }
                if (moonHalo > 0) {   // Q-02: the moon's ring at night
                    const double cm = dot(d, moonD);
                    if (cm > cosMoonRingLo && cm < cosMoonRingHi) {   // pale in the night's ramp (black under its shade 36, as the noctilucent clouds are drawn), soft
                        const double dh = std::fabs(std::acos(clampd(cm, -1, 1)) / DEG - 22.0);
                        if (dh < 2.0) { const double f = 1 - dh / 2.0, k = moonHalo * f * f; paleV = std::max(paleV, 30.0 * k); }
                    }
                }
                if (nlc > 0 && el > 0.012 && el < 0.5) {   // Q-02: the noctilucent clouds where the ray meets their layer
                    const double sl = nlcH / std::max(el, 0.03), hl = std::sqrt(d.x * d.x + d.z * d.z);
                    const double X = camPos.x / 1000.0 + d.x * sl, Z = camPos.z / 1000.0 + d.z * sl;
                    const double along = hl > 1e-6 ? (d.x * sdh.x + d.z * sdh.z) / hl * sl : 0.0;   // km toward the sun's azimuth
                    const double depHere = -sunAltA / DEG - along / RkmS / DEG;                      // the sun's depression under the cloud
                    const double lit = smoothstep(nlcLimitDeg + 1.5, nlcLimitDeg - 1.0, depHere) * smoothstep(-2.0, 1.5, depHere);
                    if (lit > 0.01) {
                        // long bands tens of kilometres apart drawn out along one way, fine billows across them where the ray is steep enough
                        // to resolve them (a distant layer seen edge-on keeps the bands alone), the patches hundreds of kilometres wide
                        const double patch = smoothstep(0.38, 0.68, 0.5 + 0.5 * fbm2(X / 420.0, Z / 420.0, site.gen.seed + 91, 2, 2.0, 0.5));
                        const double bands = 0.5 + 0.5 * std::sin((X * 0.8 + Z * 0.6) / 14.0 + 2.5 * fbm2(X / 90.0, Z / 90.0, site.gen.seed + 92, 2, 2.0, 0.5));
                        const double billows = 1.0 - 0.3 * smoothstep(0.04, 0.12, el) * (0.5 + 0.5 * std::sin((Z * 0.8 - X * 0.6) / 5.5 + t * 0.003));
                        const double nv = nlc * lit * patch * (0.45 + 0.55 * bands * billows) * (1.15 - 0.5 * smoothstep(0.05, 0.4, el));   // brightest low
                        if (nv > 0.02) paleV = std::max(paleV, 46.0 * nv);
                    }
                }
                if (pillar > 0 && cosSun > 0.85 && el > pillarElLo && el < pillarElHi && std::fabs(d.x * sdh.z - d.z * sdh.x) < pillarTanAz * (d.x * sdh.x + d.z * sdh.z)) {
                    double dAz = std::atan2(d.x * sdh.z - d.z * sdh.x, d.x * sdh.x + d.z * sdh.z);
                    double dAlt = std::asin(clampd(el, -1, 1)) - sunAltA;
                    if (std::fabs(dAz) < 2.5 * DEG && dAlt > -2 * DEG && dAlt < 11 * DEG)
                        v += 8.0 * pillar * std::exp(-(dAz / (0.9 * DEG)) * (dAz / (0.9 * DEG))) * std::exp(-std::max(0.0, dAlt) / (4.5 * DEG)) * (dAlt < 0 ? std::exp(dAlt / (1.0 * DEG)) : 1.0);
                }
                if (wedge > 0 && el > 0 && el < 0.12) {
                    double opp = -(d.x * sdh.x + d.z * sdh.z);
                    if (opp > 0.5) v *= 1 - 0.28 * wedge * (opp - 0.5) / 0.5 * (1 - el / 0.12);
                }
                if (clouds) {
                    int gx = x / CELL, gy = y / CELL;
                    double fx = (x - gx * CELL) / (double)CELL, fy = (y - gy * CELL) / (double)CELL;
                    const float* c00 = &cloudGrid[(gy * GX + gx) * 3];
                    const float* c10 = &cloudGrid[(gy * GX + gx + 1) * 3];
                    const float* c01 = &cloudGrid[((gy + 1) * GX + gx) * 3];
                    const float* c11 = &cloudGrid[((gy + 1) * GX + gx + 1) * 3];
                    double dens = (c00[0] * (1 - fx) + c10[0] * fx) * (1 - fy) + (c01[0] * (1 - fx) + c11[0] * fx) * fy;
                    double ci = (c00[1] * (1 - fx) + c10[1] * fx) * (1 - fy) + (c01[1] * (1 - fx) + c11[1] * fx) * fy;
                    v = v + (ci - v) * dens;
                    paleV *= 1 - dens;   // Q-02: the clouds under them hide them
                    stormW = ((c00[2] * (1 - fx) + c10[2] * fx) * (1 - fy) + (c01[2] * (1 - fx) + c11[2] * fx) * fy) * dens;   // W-03
                }
                if (curtain && el < 0.45) {   // over the clouds: the rain hangs in front of the deck's base, up to where the ray meets it
                    double hl = std::sqrt(d.x * d.x + d.z * d.z);
                    if (hl > 1e-6) {
                        double g2 = (d.x * localFront.nx + d.z * localFront.nz) / hl;   // metres ahead per metre of ground travelled
                        double reach = el > 0.012 ? std::min(2600.0 / el * hl, 60000.0) : 60000.0;   // the ground distance to the cloud base, or far
                        double ext = localFront.wall(localFront.ahead0, localFront.ahead0 + g2 * reach, reach / hl, env.frontRain);
                        if (ext > 1e-4) { double e = 1 - std::exp(-ext); v = v + (curtainV - v) * e; stormW = std::max(stormW, e); }
                    }
                }
            }
            if (ringArc && el > 0.0) {
                Vec3 dW = toWorld * d;
                double denom = dot(dW, ringN);
                if (std::fabs(denom) > 1e-6) {
                    double s = -dot(ringOrig, ringN) / denom;
                    if (s > 0) {
                        Vec3 P = ringOrig + dW * s;
                        double rr = length(P);
                        if (rr >= ringR0 && rr <= ringR1) {
                            double u = (rr - ringR0) / (ringR1 - ringR0);
                            double dens = (*ringProf)[clampi((int)(u * 255), 0, 255)];
                            // the world's own shadow falls across the ring at night: that part stays dark
                            double tcs = -dot(P, ringSunW);
                            bool shadowed = tcs > 0 && length2(P + ringSunW * tcs) < b.radiusKm * b.radiusKm;
                            if (dens > 0.02 && !shadowed) {
                                // O0-01 (B-301): the arc reads like a moon: pale and bright on the sky bank (it used to peak at
                                // shade 39, which is near black on that bank). Opening angle, the face we see and the light factor
                                double litRing = std::fabs(dot(ringN, ringSunW)) * 0.7 + 0.3;
                                double lit = (0.55 + 0.45 * litRing) * ringFace * std::max(ringLf, 0.5);
                                double rv = 36 + 24 * ringPow[clampi((int)(u * 255), 0, 255)] * lit;   // the sky bank is black below 30
                                if (site.atmosphere) rv = rv * (1 - 0.3 * day) + v * 0.45 * day;   // washed by daylight like the day moon
                                if (rv > v) v = rv;
                            }
                        }
                    }
                }
            }
            Pix pv = pix(stormW > 0.5 ? 23 : 1, clampd(v, 0, 51.0));   // W-03: a front's deck in the storm bank
            if (paleV > 0.6 * v && paleV > 1.5 && stormW <= 0.5) pv = pix(22, clampd(std::max(paleV, 0.6 * v), 0, 50.0));   // Q-02: over the night's dark sky
            if (tailV > v) pv = pix(15, clampd(tailV, 0, 50));   // O4: the comet's tail in its own blue bank
            if (step == 1) fb.idx[o] = pv;
            else {
                for (int yy = y; yy < y + step && yy < FBH; yy++)
                    for (int xx = x; xx < x + step && xx < FBW; xx++) fb.idx[yy * FBW + xx] = pv;
            }
        }
    }
    });
    // other bodies of the system in the sky (they occlude stars)
    if (!venus) {
        SpaceContext sc;
        sc.sys = site.sys; sc.t = t;
        sc.shipPos = site.worldPos(t, player.x, player.z, camPos.y / 1000.0);
        sc.cam = camLocal * site.localFrame(t);
        sc.excludeBody = site.body;
        sc.skyMode = true;
        sc.skyDark = site.atmosphere ? 1 - day : 1;
        sr.drawSkyBodies(fb, sc);
    }
    // stars, dimmed by sky brightness
    double starScale = (site.atmosphere ? (1.0 - 0.9 * day) * (1 - env.rain) : 1.0) * (site.atmosphere ? KINDS[site.gen.type].starLight : 1.0);   // D-04: a thick air's share
    if (starScale > 0.03 && !venus) {
        Mat3 L2 = site.localFrame(t);
        Mat3 cam = camLocal * L2;
        Vec3 obs = site.worldPos(t, player.x, player.z, camPos.y / 1000.0);
        drawStarField(fb, stars, obs, cam, proj, starScale * 0.85, 4, false, fb.idx.data(), 22.0, 0, Vec3(), site.atmosphere ? 1.0 : 0.0, t);
    }
    // G-03: the galactic band (and the nebula patches, N5-01) over the sky pixels, after the bodies and the stars, as the aurora
    // below: the map's brightness in bank 22, hidden by the clouds, written where its light beats the sky pixel's own (both
    // read from the palette, since the two ramps differ); a star in front keeps its pixel, a sky body's disc is brighter.
    // The band used to be added to the sky's value in the sky bank, whose night ramp is too dark to show it.
    // B-408: an airless sky is the band's bank all over, the sky's own light (a comet's coma, the halo round the sun) carried
    // into it by its light. The suns' glows add on what lies under them in the pixel's bank: they lit the band's pixels in the
    // band's ramp and the sky's in the sky's, which is black under shade 30, so where the band's patch ended the glow round a
    // sun had a hard edge (a pale wedge with straight sides across the sun of a comet); now it lands on one bank
    // whatever lies behind it. It reaches down to the horizon of the drawn ground, not to the horizontal (where it ended in a
    // straight edge across the sky of a small body, whose sky shows under the horizontal all round), and lies behind the bodies
    const double bandGain = venus ? 0 : 20.0 * (site.atmosphere ? (1.0 - day) * (1 - env.rain) * KINDS[site.gen.type].starLight : 1.0);   // the bulge 17 shades on a dark night, the plane 6-8, the wings under 2 left to the sky
    NebulaPatch nebAll[18]; int nebAllN = nebN;   // S-04: the field's patches and a protostar's own cloud round the star's direction now
    for (int k = 0; k < nebN; k++) nebAll[k] = nebP[k];
    nebAllN += starNebulaPatches(site.sys->star, normalize(site.sys->star.pos - site.sys->bodyPos(site.body, t)), nebAll + nebAllN);
    double bandMinEl = -1.0;
    if (!site.smallBody) { double c = (site.R + floorH) / (site.R + std::max(camPos.y, floorH)); bandMinEl = -std::sqrt(std::max(0.0, 1 - c * c)); }
    // W-04: the zodiacal light, the dust of a young system or a near belt lit by the sun (`zodiacalLight`: a cone along the plane
    // of the worlds from the sun, the faint patch opposite it), added to the band's light as the band is drawn and faded by day
    // as the band is; smooth, so it is read on the cell grid and interpolated
    const bool zodiOn = zodiDust >= ZODIACAL_MIN_DUST && bandGain > 0.05;
    if (zodiOn) {
        zodiGrid.assign(GX * GY, 0.f);
        const Vec3 sunW = toWorld * env.sunDisc.dirLocal;
        const double gain = bandGain * zodiDust;
        parallelFor(GY, 4, [&](int gy0, int gy1) {   // F-05: the rows on the pool (zodiacalLight is pure)
            for (int gy = gy0; gy < gy1; gy++)
                for (int gx = 0; gx < GX; gx++) {
                    int px = std::min(gx * CELL, FBW - 1), py = std::min(gy * CELL, FBH - 1);
                    Vec3 dW = toWorld * (camT * dirLUT[py * FBW + px]);
                    zodiGrid[gy * GX + gx] = (float)(gain * zodiacalLight(dW, sunW));
                }
        });
    }
    const bool airless = !site.atmosphere;
    if (bandGain > 0.05) {
        double lumSky[64], lumBand[64];
        for (int k = 0; k < 64; k++) {
            const uint8_t* c1 = &fb.pal[(1 * 64 + k) * 3]; lumSky[k] = 0.2126 * c1[0] + 0.7152 * c1[1] + 0.0722 * c1[2];
            const uint8_t* c2 = &fb.pal[(22 * 64 + k) * 3]; lumBand[k] = 0.2126 * c2[0] + 0.7152 * c2[1] + 0.0722 * c2[2];
        }
        // airless: the intensity in the band's bank with the light of each intensity in the sky's (both ramps rise from black;
        // the halo the airless sky draws round a sun is no light of a hole's)
        std::vector<int> skyToBand;
        const double cosDisc1 = site.sys->star.cls == STAR_BLACK_HOLE ? 2.0 : std::cos(env.sunDisc.angularRadius), cosDisc2 = std::cos(env.sun2.angularRadius);   // a hole has no disc (and its lens shows what lies behind it)
        if (airless) {
            skyToBand.assign(INTEN_MASK + 1, 0);
            auto lumAt = [](const double* lum, double sh) { int k = std::min(62, (int)sh); return lum[k] + (lum[k + 1] - lum[k]) * std::min(1.0, sh - k); };
            const bool ownLight = site.sys->star.cls != STAR_BLACK_HOLE;
            int k = 0;
            for (int i = 0; i <= INTEN_MASK && ownLight; i++) {
                double L = lumAt(lumSky, std::min(63.0, i / (double)INTEN_PER_SHADE));
                while (k < 62 && lumBand[k + 1] < L) k++;
                double f = lumBand[k + 1] > lumBand[k] ? clampd((L - lumBand[k]) / (lumBand[k + 1] - lumBand[k]), 0, 1) : 0.0;
                skyToBand[i] = (int)((k + f) * INTEN_PER_SHADE + 0.5);
            }
        }
        parallelFor(rowsTotal, 40, [&](int rb, int re) {
        for (int y = rb * step; y < re * step && y < FBH; y += step) {
            for (int x = 0; x < FBW; x += step) {
                int o = y * FBW + x;
                if (bankOf(fb.idx[o]) != 1) continue;
                Vec3 d = camT * dirLUT[o];
                if (d.y <= bandMinEl) continue;
                Vec3 dW = toWorld * d;
                double band = bandAt(dW);
                if (nebAllN) { int tone; band = std::max(band, 0.9 * nebulaGlow(nebAll, nebAllN, dW, tone)); }
                double bs = band * bandGain;
                if (zodiOn) {   // W-04
                    int gx = x / CELL, gy = y / CELL;
                    double fx = (x - gx * CELL) / (double)CELL, fy = (y - gy * CELL) / (double)CELL;
                    const float* z0 = &zodiGrid[gy * GX + gx]; const float* z1 = z0 + GX;
                    bs += (z0[0] * (1 - fx) + z0[1] * fx) * (1 - fy) + (z1[0] * (1 - fx) + z1[1] * fx) * fy;
                }
                if (clouds) {   // the clouds hide it
                    int gx = x / CELL, gy = y / CELL;
                    double fx = (x - gx * CELL) / (double)CELL, fy = (y - gy * CELL) / (double)CELL;
                    double dens = (cloudGrid[(gy * GX + gx) * 3] * (1 - fx) + cloudGrid[(gy * GX + gx + 1) * 3] * fx) * (1 - fy) + (cloudGrid[((gy + 1) * GX + gx) * 3] * (1 - fx) + cloudGrid[((gy + 1) * GX + gx + 1) * 3] * fx) * fy;
                    bs *= 1 - dens;
                }
                if (airless) {   // B-408: every sky pixel, the band added to the sky's own light
                    // but those a sun's disc covers: its disc is never dimmer than the sky round it, read from the shade there
                    if (dot(d, env.sunDisc.dirLocal) > cosDisc1 || (env.hasSun2 && dot(d, env.sun2.dirLocal) > cosDisc2)) continue;
                    for (int yy = y; yy < y + step && yy < FBH; yy++)
                        for (int xx = x; xx < x + step && xx < FBW; xx++) {
                            Pix& p = fb.idx[yy * FBW + xx];
                            if (bankOf(p) != 1 || fb.invz[yy * FBW + xx] > 1e-12f) continue;
                            p = pixI(22, std::min(INTEN_MASK, skyToBand[intenOf(p)] + (int)(bs * INTEN_PER_SHADE + 0.5)));
                        }
                    continue;
                }
                if (bs < 1.5) continue;   // the wings would only lift the sky by a level or two: leave the sky bank its pixels
                int bi = std::min(63, (int)bs);
                Pix pv = pix(22, bs);
                for (int yy = y; yy < y + step && yy < FBH; yy++)
                    for (int xx = x; xx < x + step && xx < FBW; xx++) {
                        Pix& p = fb.idx[yy * FBW + xx];
                        if (bankOf(p) != 1 || lumBand[bi] <= lumSky[std::min(63, intenOf(p) / INTEN_PER_SHADE)]) continue;
                        if (fb.invz[yy * FBW + xx] > 1e-12f) continue;   // B-408: behind every body, as the stars (a night side is the sky's own pixel with its depth)
                        p = pv;
                    }
            }
        }
        });
    }
    // Q-02 (TR_AIRGLOW): the oxygen's own glow over the night sky, as the aurora is drawn (banks 11 and 21 over the sky's pixels where it beats
    // them): the green line's layer 97 km up brightest some ten degrees over the horizon, where the eye looks along it, the red line's
    // fainter higher up, both rippled by the gravity waves that cross the layer in bands tens of kilometres apart; gone with the day, the rain
    // and the clouds
    const double airglow = site.gen.hasTrait(TR_AIRGLOW) && site.atmosphere && !venus ? (1 - smoothstep(0.02, 0.18, day)) * (1 - env.rain) * (1 - 0.8 * env.cloudCover) : 0.0;
    if (airglow > 0.02) {
        const double wx0 = camPos.x / 1000.0, wz0 = camPos.z / 1000.0, ph = t / 900.0;
        parallelFor(rowsTotal, 40, [&](int rb, int re) {
        for (int y = rb * step; y < re * step && y < FBH; y += step) {
            for (int x = 0; x < FBW; x += step) {
                const int o = y * FBW + x;
                const int pb = bankOf(fb.idx[o]);
                if ((pb != 1 && pb != 22) || fb.invz[o] > 1e-12f) continue;
                const Vec3 d = camT * dirLUT[o];
                const double el = d.y;
                if (el < 0.005) continue;
                const double sl = 97.0 / std::max(el, 0.06);   // where the ray meets the layer (km): the waves' place
                const double X = wx0 + d.x * sl, Z = wz0 + d.z * sl;
                // the waves where the ray is steep enough to resolve their 40 km (low down they average out)
                const double waves = 1.0 - 0.3 * smoothstep(0.12, 0.3, el) * (0.5 + 0.5 * std::sin((X * 0.6 + Z * 0.8) / 6.5 + ph + 1.5 * gnoise2(X / 120.0, Z / 120.0, site.gen.seed + 95)));
                const double green = airglow * 11.0 * smoothstep(0.0, 0.06, el) * (0.35 + 0.65 * std::exp(-(el - 0.15) * (el - 0.15) / 0.015)) * waves;
                const double red = airglow * 4.5 * smoothstep(0.2, 0.35, el) * std::exp(-(el - 0.5) * (el - 0.5) / 0.04) * waves;
                const bool g = green >= red;
                const double ae = g ? green : red;
                if (ae < 1.5) continue;
                for (int yy = y; yy < y + step && yy < FBH; yy++)
                    for (int xx = x; xx < x + step && xx < FBW; xx++) {
                        Pix& p = fb.idx[yy * FBW + xx];
                        const int b0 = bankOf(p);
                        if (b0 == 1) { if (shadeOf(p) < ae) p = pix(g ? 11 : 21, ae); }
                        else if (b0 == 22) p = pix(g ? 11 : 21, ae + 0.6 * shadeOf(p));   // the galactic band's light shows through it (added, not hidden)
                    }
            }
        }
        });
    }
    // B-403: the aurora over the sky pixels, after the bodies and the stars: the grid's value interpolated per pixel, written
    // in bank 11 (or 21 where the tops' colour has the greater share) where it beats the pixel's own shade on the sky bank. The
    // stars (bank 4) show through it; a sky body's lit disc is brighter than it and stays, its night side (depth only, the
    // sky's own pixel) shows it in front: the curtains hang 100-250 km up, far under any moon or parent
    if (auroraOn) {
        parallelFor(rowsTotal, 40, [&](int rb, int re) {
        for (int y = rb * step; y < re * step && y < FBH; y += step) {
            int gy = y / CELL;
            double fy = (y - gy * CELL) / (double)CELL;
            const float* r0 = &auroraGrid[gy * GX * 2];
            const float* r1 = &auroraGrid[(gy + 1) * GX * 2];
            for (int x = 0; x < FBW; x += step) {
                int gx = x / CELL;
                float c00 = r0[gx * 2], c10 = r0[gx * 2 + 2], c01 = r1[gx * 2], c11 = r1[gx * 2 + 2];
                if (c00 <= 0 && c10 <= 0 && c01 <= 0 && c11 <= 0) continue;
                double fx = (x - gx * CELL) / (double)CELL;
                double w00 = (1 - fx) * (1 - fy), w10 = fx * (1 - fy), w01 = (1 - fx) * fy, w11 = fx * fy;
                double av = c00 * w00 + c10 * w10 + c01 * w01 + c11 * w11;
                if (av < 0.5) continue;
                double tops = (c00 * r0[gx * 2 + 1] * w00 + c10 * r0[gx * 2 + 3] * w10 + c01 * r1[gx * 2 + 1] * w01 + c11 * r1[gx * 2 + 3] * w11) / av;
                int bank = tops > 0.5 ? 21 : 11;
                for (int yy = y; yy < y + step && yy < FBH; yy++)
                    for (int xx = x; xx < x + step && xx < FBW; xx++) {
                        int o = yy * FBW + xx;
                        Pix& p = fb.idx[o];
                        if (bankOf(p) != 1 && bankOf(p) != 22) continue;   // G-03: over the galactic band's pixels too (its ramp is as bright per shade)
                        double sky = shadeOf(p), ae = bankOf(p) == 1 ? av * smoothstep(26.0, 6.0, sky) : av;   // the twilight glow washes it out smoothly rather than cutting holes in it
                        if (sky >= ae) continue;
                        p = pix(bank, ae);
                    }
            }
        }
        });
    }
    // M10-14: meteors at night: a short streak that lives half a second, a few per minute
    if (starScale > 0.3 && !venus) {
        uint64_t slot = (uint64_t)(t * 2.0);
        Rng mr(slot ^ (site.gen.seed << 1));
        if (mr.chance(0.06)) {
            double az = mr.range(0, TAU), el = mr.range(0.25, 1.2), len = mr.range(0.05, 0.12), dirA = mr.range(0, TAU);
            double f = t * 2.0 - (double)slot;
            Vec3 p0(std::cos(el) * std::sin(az), std::sin(el), std::cos(el) * std::cos(az));
            Vec3 tang = normalize(cross(p0, Vec3(std::sin(dirA), 0.3, std::cos(dirA))));
            Vec3 a = normalize(p0 + tang * (len * f)), bb = normalize(p0 + tang * (len * (f + 0.25)));
            Vec3 va = camLocal * a, vb = camLocal * bb;
            RVert ra, rb;
            ra.x = va.x * 900; ra.y = va.y * 900; ra.z = va.z * 900; ra.shade = 40 * starScale;
            rb.x = vb.x * 900; rb.y = vb.y * 900; rb.z = vb.z * 900; rb.shade = 55 * starScale;
            if (va.z > 0.05 && vb.z > 0.05) rasterLine3(fb, ra, rb, 4, proj, true, 1);
        }
    }
    // W-04: a meteor shower's streaks (`showerMeteorsAt`), drawn as the sporadic ones in the stars' bank
    if (starScale > 0.3 && !venus && !showers.empty()) {
        std::vector<ShowerMeteor> ms; showerMeteorsAt(t, ms);
        for (const ShowerMeteor& m : ms) {
            Vec3 va = camLocal * m.tail, vb = camLocal * m.head;
            if (va.z <= 0.05 || vb.z <= 0.05 || m.bright * starScale <= 4) continue;
            RVert ra, rb;
            ra.x = va.x * 900; ra.y = va.y * 900; ra.z = va.z * 900; ra.shade = 0.55 * m.bright * starScale;
            rb.x = vb.x * 900; rb.y = vb.y * 900; rb.z = vb.z * 900; rb.shade = m.bright * starScale;
            rasterLine3(fb, ra, rb, 4, proj, true, 1);
        }
    }
    // M5-01: the companion sun's disc, in its own colour (bank 12), drawn first so the primary's flare wins
    if (twoSuns) {
        Vec3 v = camLocal * sd2;
        if (sd2.y > -0.05 || !site.atmosphere) {
            double haze = site.atmosphere ? (0.6 + 0.4 * env.cloudCover) : 0.0;
            double sunI = (venus ? 0.55 : 1.0) * (site.atmosphere ? clampd(0.55 + 0.45 * smoothstep(-0.05, 0.1, sd2.y), 0, 1) : 1.0) * clampd(0.5 + 0.5 * sun2Up, 0.5, 1.0);
            SpaceRenderer::drawSun(fb, site.sys->companionStar(), v, env.sun2.angularRadius, t, sunI, 12, site.atmosphere, haze, true, proj);   // B-308: by angle
        }
    }
    // the sun
    sunVeil = 0;
    if (site.sys->sunless()) return;   // D-01: a drifter's sky has none (its place at the nadir is only where the readers of the sun find night)
    {
        const Vec3 sdd = env.sunDisc.dirLocal;
        const double sinAltD = sdd.y;
        Vec3 v = camLocal * sdd;
        {
            // the sun is depth-tested against the bodies drawn before it, so a moon or the parent
            // planet carves a partial eclipse out of the disc (M1-08); the corona grows as the disc goes
            double inten = venus ? 0.55 : 1.0;
            double haze = site.atmosphere ? (0.6 + 0.4 * env.cloudCover) : 0.0;
            if (sinAltD > -0.05 || !site.atmosphere) {
                double sunI = inten * (site.atmosphere ? clampd(0.55 + 0.45 * smoothstep(-0.05, 0.1, sinAltD), 0, 1) : 1.0) * clampd(0.5 + 0.5 * env.primaryRel, 0.5, 1.0);   // B-425: a far primary as the companion's disc is drawn
                Vec3 discUp = camLocal * (site.localFrame(t) * Vec3(0, 1, 0));   // S-04: the orbital plane's normal, for a protostar's disc
                // B-408: on an airless sky in the band's bank, which the sky lies in (a hole's disc and a protostar's keep the sky bank's colours)
                const int sunBank = !site.atmosphere && site.sys->star.cls != STAR_BLACK_HOLE && site.sys->star.cls != STAR_PROTOSTAR ? 22 : 1;
                SpaceRenderer::drawSun(fb, site.sys->star, v, env.sunDisc.angularRadius, t, sunI, sunBank, site.atmosphere, haze, true, proj, &discUp);   // B-308: by angle
                if (v.z <= 0.001) return;   // the eclipse glow and the flare need the projected centre
                double sx = proj.cx + proj.f * v.x / v.z, sy = proj.cy - proj.f * v.y / v.z;
                double rpx = proj.f * std::tan(env.sunDisc.angularRadius) / v.z;
                // B-309: the veiling glare over the finished picture (drawVeil), from the sun's place and brightness
                sunSx = sx; sunSy = sy; sunRpx = rpx;
                sunVeil = site.sys->star.cls == STAR_BLACK_HOLE ? 0.0 : sunI * (1 - env.sunDisc.eclipse) * (site.atmosphere ? (1 - 0.6 * env.cloudCover) * (1 - env.rain) * smoothstep(-0.02, 0.12, sinAltD) : 0.7) * env.primaryRel;   // S-06: no glare from a hole; B-425: nor much from a far primary
                if (env.sunDisc.eclipse > 0.3 && !venus) {
                    double r = std::max(rpx, 1.5 * FB_SCALE), e = env.sunDisc.eclipse;
                    fb.glowDisc(sx, sy, r * 4.0, r * 1.05, (int)(34 * e * e), sunBank, true);
                    fb.glowDisc(sx, sy, r * 9.0, r * 1.1, (int)(10 * e * e), sunBank, true);
                }
                int ix = (int)sx, iy = (int)sy;
                bool centreFree = ix >= 0 && iy >= 0 && ix < FBW && iy < FBH && fb.invz[iy * FBW + ix] <= 1e-12f;
                // M1-03 lens flare, dimmed by haze and rain, only with the sun well up and unobstructed
                double flareI = sunI * (1 - env.sunDisc.eclipse) * (site.atmosphere ? (1 - 0.5 * env.cloudCover) * (1 - env.rain) * smoothstep(0.0, 0.15, sinAltD) : 0.8) * env.primaryRel;
                if (!venus && centreFree && flareI > 0.05 && site.sys->star.cls != STAR_BLACK_HOLE) SpaceRenderer::drawLensFlare(fb, sx, sy, std::max(rpx, 2.0 * FB_SCALE), flareI, sunBank, proj);   // S-06: nor a flare
            }
        }
    }
}

// W-04: the shower meteors alight at t. While a stream is active (`showerActivity`) meteors light up round its radiant (fixed
// among the stars: the system's axes through the local frame, so it rises and sets with them) and run away from it, up to
// SHOWER_PEAK_PER_MIN a minute at the peak with the radiant overhead (the rate times the sine of its altitude; none while it
// is under the horizon). Quarter-second slots hashed with the stream decide each one, so the sky is the same at a time; the
// nearer the radiant the shorter the path (it comes at you), a quick fade in and a slower one out, faster streams brighter
void SurfaceView::showerMeteorsAt(double t, std::vector<ShowerMeteor>& out) const {
    out.clear();
    if (showers.empty()) return;
    Mat3 Lw = site.localFrame(t);
    for (int k = 0; k < (int)showers.size(); k++) {
        const MeteorShower& s = showers[k];
        double act = showerActivity(s, t);
        if (act < 0.004) continue;
        Vec3 rad = Lw * s.radiant;
        if (rad.y <= 0.02) continue;
        double perSec = SHOWER_PEAK_PER_MIN / 60.0 * act * rad.y;
        const double SLOT = 0.25, LIFE = 1.0;
        Vec3 e1 = normalize(cross(rad, std::fabs(rad.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0)));
        Vec3 e2 = cross(rad, e1);
        int64_t s0 = (int64_t)std::floor((t - LIFE) / SLOT), s1 = (int64_t)std::floor(t / SLOT);
        for (int64_t sl = s0; sl <= s1; sl++) {
            Rng r(mix64((uint64_t)sl * 0x9E3779B97F4A7C15ULL) ^ mix64((uint64_t)s.comet + 0x5A0EULL) ^ site.gen.seed);
            if (!r.chance(std::min(0.9, perSec * SLOT))) continue;
            double start = (sl + r.uni()) * SLOT, life = 0.3 + 0.6 * r.uni(), age = t - start;
            if (age < 0 || age > life) continue;
            double th0 = r.range(6, 70) * DEG, psi = r.uni() * TAU;
            double rate = r.range(16, 34) * DEG * (0.35 + 0.65 * std::sin(th0 + 0.3));   // radians a second along the sky
            double f = age / life;
            double head = th0 + rate * age, tail = std::max(th0, head - rate * 0.35);
            Vec3 side = e1 * std::cos(psi) + e2 * std::sin(psi);
            ShowerMeteor m;
            m.tail = rad * std::cos(tail) + side * std::sin(tail);
            m.head = rad * std::cos(head) + side * std::sin(head);
            if (m.tail.y < 0 && m.head.y < 0) continue;
            double fade = smoothstep(0.0, 0.25, f) * (1 - smoothstep(0.7, 1.0, f));
            m.bright = r.range(42, 62) * (0.6 + 0.4 * std::min(1.0, s.speedKms / 120.0)) * fade;
            m.shower = k;
            out.push_back(m);
        }
    }
}

// B-309: a veiling glare round the sun over the finished picture: a broad, faint brightening that the mush melts into
// the scene, in place of the HUD glint arcs (which spun round the screen centre and wobbled with every head bob). Its
// strength follows how much of the disc is still open to the sky once the ground, the trees and the objects are drawn,
// so it fades smoothly behind a ridge or a canopy instead of switching.
void SurfaceView::drawVeil(Framebuffer& fb) {
    if (sunVeil < 0.02 || sunSx < -FBW || sunSx > 2 * FBW || sunSy < -FBH || sunSy > 2 * FBH) return;
    double rr = std::max(sunRpx, 2.0 * FB_SCALE);
    int open = 0, n = 0;
    for (int j = -2; j <= 2; j++)
        for (int i = -2; i <= 2; i++) {
            int x = (int)(sunSx + i * rr * 0.4), y = (int)(sunSy + j * rr * 0.4);
            if (x < 0 || y < 0 || x >= FBW || y >= FBH) continue;
            n++;
            if (fb.invz[y * FBW + x] <= 1e-12f) open++;
        }
    if (!n) return;
    double veil = sunVeil * open / n;
    if (veil < 0.02) return;
    double radius = 0.42 * FBW;
    fb.glowDisc(sunSx, sunSy, radius, rr * 1.5, (int)((site.atmosphere ? 5.0 : 3.0) * veil + 0.5), -1, false);
}
