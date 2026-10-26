// E-08: the surface's environment, made each frame (`computeEnvironment`: the suns and the sky's light, the tide, the clouds and
// the fronts' weather, rain, snow, dust and the valley fog, the aurorae, the wind, the temperature, the fog's distance, the
// planetshine; the finders of the records near the explorer it asks), the fog seas' floor, and the ground's songs
#include "surface_internal.h"

void SurfaceView::computeEnvironment(double t) {
    const Body& b = site.sys->bodies[site.body];
    env.sun = site.sun(t);
    env.sunDisc = env.sun;
    env.lightColor = env.sun.color;   // the star's colour (S-01: whiter in a red dwarf's flare)
    env.hasSun2 = site.sun2(t, env.sun2);
    env.sun2Share = 0;
    env.glow = 1; env.primaryRel = 1;
    double sinAlt = env.sun.dirLocal.y;
    env.skyBrightness = site.atmosphere ? smoothstep(-0.12, 0.2, sinAlt) * (1 - 0.9 * env.sun.eclipse * env.sun.eclipse) : 0.0;
    if (env.hasSun2) {
        // the light for shading blends both suns by their strength above the horizon; the sky and the
        // day brightness take the brighter of the two, so a companion still up after the primary set keeps the day alive.
        // each sun by its own light beside the world's own sun's (the far one's is faint: `SurfaceSite::sun2`); a sun under the
        // horizon adds none (the primary's whole light was added to a companion's still up, so a night under a far one was lit as a day)
        const bool ofK = site.sys->ofCompanion(site.body);
        const double ownLf = std::max(ofK ? env.sun2.lightFactor : env.sunDisc.lightFactor, 0.05);
        if (ofK) { env.primaryRel = clampd(env.sunDisc.lightFactor / ownLf, 0, 1); env.skyBrightness *= env.primaryRel; }
        double up1 = smoothstep(-0.12, 0.05, env.sunDisc.dirLocal.y), up2 = smoothstep(-0.12, 0.05, env.sun2.dirLocal.y);
        double w1 = env.sunDisc.lightFactor * up1, w2 = env.sun2.lightFactor * up2;
        if (w1 + w2 > 1e-6) {
            env.sun2Share = w2 / (w1 + w2);
            Vec3 dir = normalize(env.sunDisc.dirLocal * w1 + env.sun2.dirLocal * w2);
            if (env.sun2Share > 0.02) {
                env.sun.dirLocal = dir;
                env.sun.altitude = std::asin(clampd(dir.y, -1, 1));
                env.sun.azimuth = wrap2pi(std::atan2(dir.x, dir.z));
                env.sun.lightFactor = env.sunDisc.lightFactor * (1 - env.sun2Share) + (env.sunDisc.lightFactor * up1 + env.sun2.lightFactor) * env.sun2Share;
                env.sun.eclipse = env.sunDisc.eclipse * (1 - env.sun2Share);
                env.lightColor = lerp(env.sunDisc.color, env.sun2.color, (float)env.sun2Share);
            }
        }
        env.glow = clampd(env.sun.lightFactor / ownLf, 0, 1);
        double day2 = site.atmosphere ? smoothstep(-0.12, 0.2, env.sun2.dirLocal.y) * clampd(env.sun2.lightFactor / ownLf, 0, 1) : 0.0;
        env.skyBrightness = std::max(env.skyBrightness, day2);
        sinAlt = env.sun.dirLocal.y;
    }
    // TR_GREAT_TIDES: the sea's level now (the bulges of the world's moons and sun at the explorer's place: the whole site's, the tide a
    // few kilometres across being one)
    site.tideM = site.gen.tideRangeM > 0 && site.hasWater ? tideOffsetM(*site.sys, site.body, site.unitAt(player.x, player.z), t, site.gen.tideRangeM) : 0.0;
    env.ringShadow = env.sunDisc.ringShadow;
    env.skyBrightness *= 1 - 0.45 * env.ringShadow;
    env.localTime = env.hasSun2 && site.sys->ofCompanion(site.body) ? env.sun2.dayFraction : env.sun.dayFraction;   // the time of the world's own sun
    site.latLonAt(player.x, player.z, env.latDeg, env.lonDeg);
    env.latDeg /= DEG; env.lonDeg /= DEG;
    env.altitude = player.y;
    // weather: cloud cover from the planet map pattern plus slow drift
    double lat, lon;
    site.latLonAt(player.x, player.z, lat, lon);
    double cloud = 0;
    if (site.atmosphere && !hasOpaqueDeck(b.type)) {
        double drift = t * TAU / (std::fabs(b.rotPeriod) * 3.7) + 1.0;
        cloud = sampleCloudPattern(site.gen, lon + drift, lat);
        cloud = clampd(cloud * 1.3 + 0.25 * gnoise2(t / 900.0, site.gen.seed & 1023, site.gen.seed), 0, 1);
    }
    if (hasOpaqueDeck(b.type)) cloud = 1;
    env.cloudPattern = cloud;
    // the fronts. The world's fronts alive now (`frontsOf`, listed again every two minutes: a birth ramps up over
    // hours), their weather at the place (`frontWeatherAt`: the same function the globe's overlay and the forecast read),
    // and the nearest one's line in local metres for the sky, the ground's shadows and the wall of rain
    FrontWeather fw;
    localFront.on = false;
    if (site.atmosphere && !hasOpaqueDeck(b.type) && worldHasFronts(b)) {
        if (frontsSeed != b.seed || frontsT < -1e17 || std::fabs(t - frontsT) > 120) { frontsOf(b, t, fronts, 0); frontsT = t; frontsSeed = b.seed; }
        Vec3 u = site.unitAt(player.x, player.z);
        frontWeatherAt(fronts, u, t, site.R, fw);
        if (fw.nearest >= 0) {
            const Front& f = fronts[fw.nearest];
            Vec3 c, n; frontAt(f, t, c, n);
            Vec3 nt = n - u * dot(n, u);   // the motion along the ground here
            double nx = dot(nt, site.east0), nz = dot(nt, site.north0), L = std::sqrt(nx * nx + nz * nz);
            if (L > 1e-9) {
                localFront.on = true; localFront.nx = nx / L; localFront.nz = nz / L;
                localFront.x0 = player.x; localFront.z0 = player.z; localFront.ahead0 = fw.nearestAhead;
                localFront.width = f.width; localFront.gain = fw.nearestGain; localFront.tex = fw.nearestTex; localFront.strength = f.strength; localFront.dust = f.dust;
                localFront.rainVis = f.dust ? 400 : 1500;
            }
        }
    }
    env.frontCloud = fw.cloud; env.frontRain = fw.precip; env.frontCold = fw.cold; env.frontAhead = fw.nearest >= 0 ? fw.nearestAhead : 1e18;
    cloud = std::max(cloud, fw.cloud);
    env.cloudCover = cloud;
    double cloudP = env.cloudPattern;   // the pattern's cloud rains by its own rule; a front's band rains from its line, not from its first cloud
    const KindDef& K = KINDS[b.type];   // the kind's weather, temperature and pressure are its row's
    env.rain = 0;
    if (K.rain == RAIN_CLOUDS) env.rain = cloudP > K.rainThr ? clampd((cloudP - K.rainThr) / K.rainSpan, 0, 1) : 0;   // the acid rains often
    if (K.rain == RAIN_DRIZZLE) { double dz = gnoise2(t / 1800.0, 2.3, site.gen.seed + 27); env.rain = clampd((dz - 0.45) / 0.3, 0, 0.5); }   // a methane drizzle now and then
    if (K.tags & KT_FRONT_RAIN) env.rain = std::max(env.rain, fw.precip);   // the front's rain
    // the same clouds snow when it is cold; thin air raises dust storms; fog banks drift by
    env.snow = 0; env.dust = 0; env.fogBank = 0; env.hail = 0;
    if (K.tags & KT_WATER_WEATHER) {
        double precip = std::max(cloudP > 0.62 ? clampd((cloudP - 0.62) / 0.3, 0, 1) : 0.0, fw.precip);
        if (precip > 0) {
            if (env.temperatureC < -1) { env.snow = precip; env.rain = 0; }
            else if (env.temperatureC < 8 && precip > 0.8) env.hail = (precip - 0.8) / 0.2;
        }
    }
    if (K.dustThreshold > 0 && site.atmosphere) {   // no dust rises where the air fell as snow
        double storm = gnoise2(t / 2400.0, 5.1, site.gen.seed + 21);
        env.dust = clampd((storm - K.dustThreshold) / 0.3, 0, 1);   // the storm's wind is applied with the wind update below (B-206); R-307: rarer on a desert world
        env.dust = std::max(env.dust, fw.precip);   // a dust front
    }
    if ((K.tags & KT_WATER_WEATHER) && site.atmosphere) {
        double fogN = gnoise2(t / 1500.0 + player.x / 4000.0, player.z / 4000.0, site.gen.seed + 33);
        env.fogBank = clampd((fogN - 0.45) / 0.25, 0, 1) * (1 - env.rain) * clampd(1 - std::fabs(env.temperatureC - 8) / 25.0, 0, 1);
    }
    // TR_FOG_SEAS: the valley fog of the morning. It gathers in the night's calm, fills the valleys to a level over their floors by
    // dawn and burns off as the sun climbs (local times 0.10-0.40, its height with it); no fog in rain, dust or a wind; how deep this
    // morning's is by the world and the day
    env.fogSea = 0; env.fogSeaTop = -1e9;
    if (site.atmosphere && site.gen.hasTrait(TR_FOG_SEAS)) {
        const double lt = env.localTime, day = std::floor(t / std::max(60.0, std::fabs(b.rotPeriod)));
        double fs = smoothstep(0.10, 0.22, lt) * (1 - smoothstep(0.30, 0.40, lt)) * (1 - env.rain) * (1 - env.dust) * smoothstep(28.0, 10.0, env.windKnots);
        if (fs > 0.01) {
            refreshFogFloor();
            const double depth = (60 + 110 * hash01(mix64(site.gen.seed ^ 0xF06A5EAULL))) * (0.6 + 0.4 * hash01(hash2i((int64_t)day, 7, site.gen.seed ^ 0xF06DULL)));
            env.fogSea = fs; env.fogSeaTop = fogFloorM + depth * fs;
        }
    }
    env.aurora = 0; env.auroraStorm = 0;
    env.flare = starFlare(site.sys->star, t);   // a red dwarf's flare opens the exposure (setupPalette) and is a storm of its own (auroralStorm)
    const bool greatAurorae = site.gen.hasTrait(TR_GREAT_AURORAE);   // R-402's field set high
    if (site.atmosphere && !hasOpaqueDeck(b.type) && (std::fabs(env.latDeg) > 40 || greatAurorae)) {
        // the night's potential (the star's class, the world's magnetic field, the star's storm), the latitude ramp
        // pushed equatorward by the storm, the darkness, and the night's own slow variation. Q-02: under great aurorae the field's glow
        // lights most nights whatever the storm, and the ramp reaches the tropics
        env.auroraStorm = auroralStorm(site.sys->star, t);
        double latLo = greatAurorae ? 8 - 8 * env.auroraStorm : 52 - 10 * env.auroraStorm;
        double pot = auroraPotential(*site.sys, b, t);
        if (greatAurorae) pot = std::max(pot, 0.7 + 0.3 * env.auroraStorm);
        env.aurora = pot * smoothstep(latLo, latLo + 16, std::fabs(env.latDeg)) * (1 - env.skyBrightness) * (0.6 + 0.4 * gnoise2(t / 600.0, 8.8, site.gen.seed + 44));
    }
    if (lastWindUpdate < 0 || t - lastWindUpdate > 2.0) {
        // the wind is set from scratch here (base by type, a slow gust term, rain, the dust storm's x3.5): multiplied by
        // the storm factor every frame between these updates, it would run away to 1e13 knots
        lastWindUpdate = t;
        double base = site.atmosphere ? K.wind : 0;
        const bool steady = (K.tags & KT_FIXED_SUN) && site.gen.locked;   // a locked world's surface wind blows toward the substellar point, and hardly changes
        env.windKnots = base * (steady ? 0.85 + 0.3 * (0.5 + 0.5 * gnoise2(t / 400.0, 3.3, site.gen.seed + 5)) : 0.4 + 0.8 * (0.5 + 0.5 * gnoise2(t / 400.0, 3.3, site.gen.seed + 5))) *
                (1 + env.rain * 1.5) * (1 + 2.5 * env.dust) * (1 + 1.0 * fw.windRise);   // the wind rises ahead of a front
        env.windDir = steady ? wrap2pi(std::atan2(env.sunDisc.dirLocal.x, env.sunDisc.dirLocal.z) + 0.2 * gnoise2(t / 2000.0, 7.7, site.gen.seed + 6))
                             : wrap2pi(1.7 + 2.0 * gnoise2(t / 2000.0, 7.7, site.gen.seed + 6));
        if (fw.windWeight > 0.01) {   // the front's wind takes over by its weight: along its line ahead of it, with its motion behind
            double fx = dot(fw.windTo, site.east0), fz = dot(fw.windTo, site.north0);
            double w = clampd(fw.windWeight, 0, 1);
            double vx = (1 - w) * std::sin(env.windDir) + w * fx, vz = (1 - w) * std::cos(env.windDir) + w * fz;
            if (vx * vx + vz * vz > 1e-6) env.windDir = wrap2pi(std::atan2(vx, vz));
        }
    }
    // the clouds' drift (sky layers and ground shadows) integrates the wind: 0.5 m per knot-second (`wind x 0.5 x t`
    // would jump by tens of kilometres whenever the wind changes, the sun flickering: B-206)
    if (lastEnvT < 0) { windDriftX = std::sin(env.windDir) * env.windKnots * 0.5 * t; windDriftZ = std::cos(env.windDir) * env.windKnots * 0.5 * t; }
    else if (t > lastEnvT) { windDriftX += std::sin(env.windDir) * env.windKnots * 0.5 * (t - lastEnvT); windDriftZ += std::cos(env.windDir) * env.windKnots * 0.5 * (t - lastEnvT); }
    lastEnvT = t;
    // temperature
    double base = b.tempK - 273.15;
    if (livingGround(b.type)) {
        // the biome model's mean temperature at this spot (the same number that chose the grass underfoot), with
        // a day-night swing round it (a model of the HUD's own read -10 C on a grassland plateau)
        double winter = clampd((env.latDeg >= 0 ? -site.season : site.season), 0, 1);
        double sub = site.gen.locked ? dot(site.unitAt(player.x, player.z), site.gen.lockedDir) : 0.0;
        base = climateTempC(site.gen, std::fabs(env.latDeg), player.y, winter, sub) - 3 + K.daySwing * sinAlt;   // the kind's swing (a steppe's wide)
    } else if (site.atmosphere && K.reachClimate && site.gen.locked) {   // the fixed sun's height decides it, the sea freezing at the edge of its reach (the eyeball's pupil)
        base = K.tempOffset + K.tempSwing * (sinAlt - std::cos(site.gen.seaReachDeg * DEG));
    } else if (site.atmosphere) {
        double offset = K.tempOffset, swing = K.tempSwing;   // each air has its greenhouse and its day-night swing
        base = base * 0.75 + offset;
        base += swing * sinAlt - 0.4 * std::fabs(env.latDeg);
    } else {
        base = base * 1.3 - 60 + (sinAlt > 0 ? 110 * sinAlt : -70);
        base -= 0.5 * std::fabs(env.latDeg);
    }
    if (!livingGround(b.type)) base -= 6.5 * (player.y / 1000.0);
    if (K.tempAdd != 0) base += K.tempAdd;   // the ground's own heat (a molten world's, a volcanic moon's)
    if (K.nightSideC > -1e8 && site.gen.locked) base = K.nightSideC + (base - K.nightSideC) * smoothstep(-0.12, 0.08, sinAlt);   // no air carries the day round to the night side
    // in the dark the ground is as warm as the world's own heat keeps it (`Body::tempK`: a drifter's by its kind, a moon's own and a
    // brown dwarf's glow on it), no day to swing it; an air cools with height over it
    if (site.sys->sunless()) base = b.tempK - 273.15 - (site.atmosphere ? 6.5 * player.y / 1000.0 : 0.0);
    env.temperatureC = std::max(base - fw.cold, -270.0);   // the cold behind a front
    env.pressureAtm = K.pressureSpan > 0 ? K.pressureLo + K.pressureSpan * hash01(site.gen.seed) : 0;
    if (airFrozen(b)) env.pressureAtm *= 1e-6;   // the vapour over its snow, a trace
    env.pressureAtm *= std::exp(-player.y / 8000.0);
    SurfaceLook L = lookFor(site.gen, site.sys->star);
    // a front's squall closes the view further
    env.fogDistance = L.fogDistance * (1 - 0.7 * env.rain) * (1 - 0.55 * env.snow) * (1 - 0.94 * env.dust) * (1 - 0.9 * env.fogBank) * (1 - 0.5 * env.frontRain);
    if (K.lowFogM > -1e8) env.fogDistance *= clampd((player.y - K.lowFogM) / K.lowFogSpan, 0.03, 1.0);   // walking down into the steam world's lowlands, the fog closes
    // inside a fog sea the fog closes as in the steam world's lowlands (by the eye: `render` once the eye is placed)
    // a comet vents harder the closer it dives to its star (the same activity as the tail seen from space);
    // the dust of the coma hangs over the ground
    env.cometActivity = 0;
    if (b.type == PT_COMET) {
        const StarSystem& S = *site.sys;
        double dStar = length(S.bodyPos(site.body, t) - S.star.pos);
        env.cometActivity = cometActivityAt(S, dStar);
        env.fogDistance *= 1 - 0.85 * env.cometActivity;
    }
    if (player.underwater) env.fogDistance = 22;
    // M10-09: planetshine / moonlight from the brightest body above the horizon
    env.moonLight = 0; env.moonBody = -1; env.moonDir = Vec3(0, 1, 0); env.moonColor = RGB(0.8f, 0.85f, 1.0f);
    {
        Mat3 L = site.localFrame(t);
        Vec3 sitePos = site.worldPos(t, player.x, player.z, 0.002);
        for (int bi = 0; bi < (int)site.sys->bodies.size(); bi++) {
            if (bi == site.body) continue;
            const Body& ob = site.sys->bodies[bi];
            Vec3 bp = site.sys->bodyPos(bi, t);
            Vec3 rel = bp - sitePos;
            double dist = length(rel);
            Vec3 dirL = L * (rel / dist);
            if (dirL.y < 0.02) continue;
            double illum = bodyShineAt(*site.sys, bi, sitePos, t, env.sun.lightFactor);   // the formula a moon's earthshine reads too
            bool glowing = kindHas(ob.type, KT_GLOWS);
            if (illum > env.moonLight) {
                env.moonLight = illum; env.moonBody = bi; env.moonDir = dirL;
                env.moonColor = glowing ? lerp(ob.color, RGB(1, 0.7f, 0.45f), 0.4f) : lerp(ob.color * lerp(site.sys->star.color, RGB(1, 1, 1), 0.5f), RGB(1, 1, 1), 0.35f);
            }
        }
    }
    double dx = capsuleX - player.x, dz = capsuleZ - player.z;
    env.capsuleDist = std::sqrt(dx * dx + dz * dz);
    env.capsuleBearing = wrap2pi(std::atan2(dx, dz));
    env.nearCapsule = env.capsuleDist < 7.0;
    findNearShard();
    findNearGrave();
    findNearInstrument();
}

// Q-02 (TR_FOG_SEAS): the floor of the valleys round the explorer: a low quantile of the 512 m ring's ground over 6 km (a lake's or the sea's
// level where it is water: the fog lies on them as on the land), found again when the explorer has gone 1.5 km
void SurfaceView::refreshFogFloor() {
    if (std::fabs(player.x - fogFloorX) < 1500 && std::fabs(player.z - fogFloorZ) < 1500) return;
    fogFloorX = player.x; fogFloorZ = player.z;
    std::vector<double> hs;
    const int cx0 = (int)std::lround(player.x / site.lod2.cellSize), cz0 = (int)std::lround(player.z / site.lod2.cellSize);
    for (int dz = -6; dz <= 6; dz++)
        for (int dx = -6; dx <= 6; dx++) {
            const TerrainVertex& v = site.lod2.at(cx0 + dx, cz0 + dz);
            hs.push_back(v.water > -1e8f ? std::max((double)v.h, (double)v.water) : (double)v.h);
        }
    std::sort(hs.begin(), hs.end());
    fogFloorM = hs[hs.size() / 5];
}

// Q-03: the note a world's singing dunes hum at: Andreotti's f = 0.4 sqrt(g / d) of its grains' size d (150-320 microns by a hash of the
// world: 70-100 Hz under Earth's gravity, about 50 on a Mars, 30 on a Titan)
double SurfaceView::duneSongHz(const BodyGen& g, double gravity) {
    const double d = (150 + 170 * unitFromHash(mix64(g.worldSeed ^ 0xD0E5ULL))) * 1e-6;
    return 0.4 * std::sqrt(std::max(gravity, 0.1) / d);
}

// Q-03: the cues of the sounds of the ground the world's traits give. The singing dunes: a dune's slip face of sand or dust (`slipFace`: its lee,
// however steep the world's dunes stand) crossed on foot or in the buggy slides; the hum louder the faster it is crossed and the more it is gone
// down, held a few seconds after as the sand runs on. The singing ice: the ice within 64 m (snow over it half), and how fast the day
// warms or cools it (the sun's height's rate over the world's day: the most at dawn and dusk, the cooling's more), a quarter of it always
void SurfaceView::updateGroundSongs(double dt, double t) {
    const BodyGen& g = site.gen;
    const double x = inBuggy ? buggy.x : player.x, z = inBuggy ? buggy.z : player.z;
    double dune = 0;
    if (g.hasTrait(TR_SINGING_DUNES) && site.atmosphere && !inDrone && (inBuggy ? !buggy.airborne : player.onGround && !player.swimming)) {
        const int mat = site.lod0.at((int)std::floor(x / 16), (int)std::floor(z / 16)).material;
        if (mat == MAT_SAND || mat == MAT_DUST) {
            const double gx = (site.groundHeight(x + 2, z) - site.groundHeight(x - 2, z)) / 4, gz = (site.groundHeight(x, z + 2) - site.groundHeight(x, z - 2)) / 4;
            const double slope = std::sqrt(gx * gx + gz * gz);
            const double vx = inBuggy ? std::sin(buggy.heading) * buggy.speed : player.vx, vz = inBuggy ? std::cos(buggy.heading) * buggy.speed : player.vz;
            const double down = slope > 1e-6 ? std::max(0.0, -(vx * gx + vz * gz) / slope) : 0.0;   // m/s down the face
            const double slide = clampd((0.35 * std::hypot(vx, vz) + 0.65 * down) / (inBuggy ? 6.0 : 2.5), 0, 1);
            if (slope > 0.03 && slide > 0.01) dune = slide * smoothstep(0.15, 0.5, sampleSurface(g, site.unitAt(x, z), 4).slipFace);   // the near ring's 4 m: every world's dunes whole
        }
    }
    duneSong = std::max(dune, duneSong * std::exp(-dt / 2.5));
    if (duneSong < 0.002) duneSong = 0;
    double ice = 0;
    if (g.hasTrait(TR_SINGING_ICE) && site.atmosphere && !inDrone) {
        const int cx = (int)std::floor(x / 16), cz = (int)std::floor(z / 16);
        double sum = 0; int n = 0;
        for (int dz = -4; dz <= 4; dz++)
            for (int dx = -4; dx <= 4; dx++) {
                if (dx * dx + dz * dz > 16) continue;
                const int m = site.lod0.at(cx + dx, cz + dz).material;
                sum += m == MAT_ICE ? 1.0 : (m == MAT_SNOW ? 0.5 : 0.0); n++;
            }
        const double share = sum / n;
        if (share > 0.05) {
            const double day = std::max(600.0, std::fabs(site.sys->bodies[site.body].rotPeriod));
            const double rate = (site.sun(t + 30).dirLocal.y - site.sun(t - 30).dirLocal.y) / 60.0 * day / TAU;   // -1..1: falling at dusk
            ice = smoothstep(0.05, 0.6, share) * (0.25 + 0.75 * clampd(std::fabs(rate) * (rate < 0 ? 1.0 : 0.7), 0, 1));
        }
    }
    iceSong += (ice - iceSong) * (1 - std::exp(-dt / 1.5));
    if (iceSong < 0.002 && ice == 0) iceSong = 0;
}
