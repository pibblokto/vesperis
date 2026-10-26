// E-03: the scenes' placement: setupSceneForType puts a scene's world, site, hour and view by its type and code (scenes_table.h)
#include "harness/harness.h"
#include "harness/test_worlds.h"
// the first felisian world's pinned plain and the first front worth a scene that reaches it after t0: a strong one (the
// peak rain over a half) arriving by day (the sun over ten degrees); false when none within the window
bool pinnedPlainFront(StarSystem& sys, int& bi, double t0, FrontForecast& fc, double window) {
    if (!firstBodyOfType(PT_FELISIAN, sys, bi)) return false;
    const Body& b = sys.bodies[bi];
    std::vector<FrontForecast> list; frontsAhead(b, StarSystem::bodyFromLatLon(PIN_PLAIN_LAT * DEG, PIN_PLAIN_LON * DEG), t0, window, list);
    SurfaceSite probe; probe.init(&sys, bi, PIN_PLAIN_LAT * DEG, PIN_PLAIN_LON * DEG, t0);
    for (const FrontForecast& f : list) {
        if (f.tArrive < t0 || f.strength * f.gain < 0.5) continue;
        if (probe.sun(f.tArrive).altitude < 10 * DEG) continue;
        fc = f; return true;
    }
    return false;
}
std::string surfaceFrontInfo(const SurfaceView& sv) {   // W-03: the fronts' numbers at the feet
    const SurfaceEnvironment& e = sv.env;
    return fmt("line %s, ahead %s, cloud %.2f (pattern %.2f), rain %.2f, snow %.2f, cold %.1f C, wind %.0f kt, temp %+.1f C, fog %.0f m, sky %.2f", sv.localFront.on ? "on" : "off",
            e.frontAhead < 1e17 ? fmt("%.1f km", e.frontAhead / 1000).c_str() : "none", e.frontCloud, e.cloudPattern, e.frontRain, e.snow, e.frontCold, e.windKnots, e.temperatureC, e.fogDistance,
            e.skyBrightness);
}
// E-03: one body of the scene finder's scan (setupSceneForType's loop), for its finders: what the call asked, the body and its
// generator, and what a finder sets (where to stand, which way to look, the hour to start from, whether it placed the spot). The
// loop's variables are held by reference, so the placement after the finders reads what they set; a finder's false sends the scan
// to the next body (the `continue` it was)
struct SceneFind {
    const double latDeg;
    double& alt;
    SurfaceView& sv;
    StarSystem& sys;
    StarNeighborhood& nb;
    const int wantMat, bi;
    const Body& b;
    BodyGen& g;
    double& lon;
    double& latUse;
    bool& lookAtWater;
    double& faceYawOut;
    double& facePitchOut;
    double& t0Use;
    bool& placed;
    // a clear hour: of `tries` starts `stepS` apart, the first whose hour of the light asked has no rain or snow and the sky not
    // overcast (a hycean's haze is the sight, an eyeball's open sea; the weather hides them), or -1
    double clearHour(double la, double ln, double t0, double stepS, int tries) {
        for (int d = 0; d < tries; d++) {
            const double tq0 = t0 + d * stepS;
            sv.init(&sys, bi, la, ln, tq0);
            const double tq = findTime(sv.site, alt, tq0);
            sv.init(&sys, bi, la, ln, tq);
            Input in; sv.update(0.016, in, tq, false);
            if (sv.env.rain < 0.02 && sv.env.snow < 0.02 && sv.env.cloudCover < 0.5) return tq0;
        }
        return -1.0;
    }
    bool sand();
    bool geysers();
    bool groundSounds();
    bool aurora();
    bool rustValley();
    bool archeanFlats();
    bool tundraIce();
    bool hothouseOrSteppe();
    bool eyeballShore();
    bool terminatorRing();
    bool hyceanSea();
    bool ammoniaShore();
    bool snowballBelt();
    bool chemotrophVents();
    bool deadForest();
    bool scorchedSeabed();
    bool nitrogenCells();
    bool wandererSnow();
    bool brineFaculae();
    bool frostDwarf();
    bool groovedLanes();
    bool twoFacedBoundary();
    bool twoFacedRidge();
    bool spongePits();
    bool traitGround();
    bool neutronGlass();
    bool biome();
    bool inlandWater();
    bool herdOrBuggy();
    bool forest();
    bool settlement(double& t, SunInfo& si);
    bool deadSea(double& t, SunInfo& si);
};

bool SceneFind::sand() {   // MAT_SAND: a sandy shore
    // a beach: sand near the sea level, the view turned toward the water
    bool found = false;
    for (double la = latDeg; la < latDeg + 40 && !found; la += 0.5)
        for (int k = 0; k < 720 && !found; k++) {
            double ln = k * TAU / 720;
            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
            if (ss.material != MAT_SAND || ss.height < 0.5 || ss.height > 4) continue;
            // enough water within 150 m in some direction
            int best = 0;
            for (int a = 0; a < 16; a++) {
                int cnt = 0;
                for (int dd = 20; dd <= 150; dd += 10) {
                    double ang = a * TAU / 16;
                    double dlat = std::cos(ang) * dd / (b.radiusKm * 1000.0), dlon = std::sin(ang) * dd / (b.radiusKm * 1000.0 * std::cos(la * DEG));
                    if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), 16).material == MAT_WATER) cnt++;
                }
                best = std::max(best, cnt);
            }
            if (best >= 6) { lon = ln; latUse = la * DEG; found = true; }
        }
    if (!found) return false;
    lookAtWater = true; placed = true;
    return true;
}

bool SceneFind::geysers() {   // a geyser basin
    // R-307: a geyser basin: the first sinter sample of the scan, then the nearest vent of `geyserVents` that stands on
    // sinter; the site 150 m from it, facing it
    bool found = false;
    double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 50 && !found; la += 1.0)
        for (int k = 0; k < 360 && !found; k++) {
            double ln = k * TAU / 360;
            Vec3 u = StarSystem::bodyFromLatLon(la * DEG, ln);
            if (sampleSurface(g, u, 16).material != MAT_SALT) continue;
            std::vector<GeyserVent> vents; geyserVents(g, u, vents);
            for (const GeyserVent& v : vents) {
                if (sampleSurface(g, v.unit, 16).material != MAT_SALT) continue;
                double vla, vlo; StarSystem::latLonFromBody(v.unit, vla, vlo);
                double dlat = 150.0 / Rm;   // stand 150 m south of the vent, looking north
                latUse = vla - dlat; lon = vlo; faceYawOut = 0.0; found = true;
                break;
            }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::groundSounds() {   // Q-03: the dunes, the ice, the quakes
    // -99 a slip face of the singing dunes: the first sand or dust of the latitudes asked on a dune's lee (4 m samples, the near
    // ring's), then within 120 m of it the steepest of the lee's points as the ground is drawn (the drainage on: it softens some
    // worlds' dunes; the slope over 4, 8 and 16 m either side, the least: a face, not a ledge), 15 degrees at least (the felisian
    // worlds' dunes are gentle: the frame would show a plain), the site on it facing down; -100 the singing ice: the first flat ice; -101 a world with quakes: the first land
    bool found = false;
    const double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 50 && !found; la += 1.0)
        for (int k = 0; k < 180 && !found; k++) {
            const double ln = k * TAU / 180, cl = std::cos(la * DEG);
            const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), wantMat == SCENE_SINGING_DUNES ? 4 : 64);
            if (ss.water > -1e8 && ss.height < ss.water + 2) continue;
            if (wantMat == SCENE_SINGING_ICE) { if (ss.material == MAT_ICE && ss.relief < 0.25) { latUse = la * DEG; lon = ln; found = true; } continue; }
            if (wantMat == SCENE_QUAKES) { if (ss.height > 5 && ss.material != MAT_ICE && ss.material != MAT_WATER) { latUse = la * DEG; lon = ln; found = true; } continue; }
            if ((ss.material != MAT_SAND && ss.material != MAT_DUST) || ss.slipFace < 0.3) continue;
            auto at = [&](double x, double z) { return StarSystem::bodyFromLatLon(la * DEG + z / Rm, ln + x / (Rm * cl)); };
            setDrainageEnabled(true);
            double best = 0.03, bx = 0, bz = 0, bgx = 0, bgz = 0;
            for (int j = -15; j <= 15; j++)
                for (int i = -15; i <= 15; i++) {
                    const double x = i * 8.0, z = j * 8.0;
                    const SurfaceSample c0 = sampleSurface(g, at(x, z), 4);
                    if ((c0.material != MAT_SAND && c0.material != MAT_DUST) || c0.slipFace < 0.6) continue;
                    auto grad = [&](double r, double& gx, double& gz) {   // over r either side, at the drawn ground's 4 m
                        gx = (sampleSurface(g, at(x + r, z), 4).height - sampleSurface(g, at(x - r, z), 4).height) / (2 * r);
                        gz = (sampleSurface(g, at(x, z + r), 4).height - sampleSurface(g, at(x, z - r), 4).height) / (2 * r);
                    };
                    double gx, gz, gx4, gz4, gx16, gz16; grad(4, gx4, gz4); grad(8, gx, gz); grad(16, gx16, gz16);
                    const double s4 = std::hypot(gx4, gz4), s8 = std::hypot(gx, gz), s16 = std::hypot(gx16, gz16);
                    const double sl = std::max(s4, std::max(s8, s16)) < 1.0 ? std::min(s4, std::min(s8, s16)) : 0.0;   // a face 32 m long, not a ledge nor a cliff
                    if (sl > best && sl < 1.0) { best = sl; bx = x; bz = z; bgx = gx; bgz = gz; }
                }
            setDrainageEnabled(false);
            if (best <= 0.27) continue;   // a face of 15 degrees at least: a dune the frame shows
            const double ux = bgx / best, uz = bgz / best;   // up the face
            latUse = la * DEG + bz / Rm; lon = ln + bx / (Rm * cl); faceYawOut = std::atan2(-ux, -uz); found = true;
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::aurora() {   // B-403, R-402: under an aurora
    // the aurora scenes: any site of the type at the latitude asked (the star was chosen above); -42 wants a
    // clear-air world with another body of the system (a moon, the parent) at least 0.6 degrees across and 35-65
    // degrees up at the darkest hour (the main curtain hangs overhead there), the biggest of them, and faces it
    if (!KINDS[b.type].atmosphere || hasOpaqueDeck(b.type)) return false;
    {   // a night with curtains: of the next thirty nights, the darkest hour with the strongest potential, over 0.45
        SurfaceSite probe; probe.init(&sys, bi, latUse, lon, 1000.0);
        double bestPot = 0, period = std::fabs(b.rotPeriod);
        for (int dd = 0; dd < 30; dd++) {
            double t0 = 1000.0 + dd * period, pot = auroraPotential(sys, b, findTime(probe, alt, t0));
            if (pot > bestPot) { bestPot = pot; t0Use = t0; }
        }
        if (bestPot < 0.45) return false;
    }
    if (wantMat == SCENE_AURORA_CURTAIN) {
        SurfaceSite probe; probe.init(&sys, bi, latUse, lon, t0Use);
        double tp = findTime(probe, alt, t0Use);
        Vec3 obs = probe.worldPos(tp, 0, 0, 0);
        Mat3 L = probe.localFrame(tp);
        double bestR = 0; int bestJ = -1; Vec3 bestDl;
        for (int bj = 0; bj < (int)sys.bodies.size(); bj++) {
            if (bj == bi || sys.bodies[bj].type == PT_COMPANION) continue;
            Vec3 dW = sys.bodyPos(bj, tp) - obs;
            double dist = std::max(length(dW), 1.0), ang = sys.bodies[bj].radiusKm / dist;
            Vec3 dl = L * (dW / dist);
            double hl = std::sqrt(std::max(1e-9, 1 - dl.y * dl.y));
            (void)hl;
            if (ang < 0.005 || dl.y < 0.57 || dl.y > 0.9 || ang < bestR) continue;
            bestR = ang; bestJ = bj; bestDl = dl;
        }
        if (bestJ < 0) return false;
        // the curtains must be in front of it: a trial frame, counting aurora pixels within 1.6 radii of its centre
        double yaw = std::atan2(bestDl.x, bestDl.z), pitchB = std::asin(bestDl.y) - 0.08;
        sv.init(&sys, bi, latUse, lon, tp);
        nb.update(sv.site.worldPos(tp, 0, 0, 0));
        sv.player.yaw = yaw; sv.player.pitch = pitchB;
        Input in; sv.update(0.016, in, tp, false);
        SpaceRenderer srTrial; Framebuffer fb; sv.render(fb, tp, nb.stars, srTrial, 1.0);
        double sx, sy; int over = 0, disc = 0;
        if (sv.projectPoint(sv.player.x + bestDl.x * 1e5, sv.player.y + 1.65 + bestDl.y * 1e5, sv.player.z + bestDl.z * 1e5, sx, sy)) {
            double rpx = 1.6 * bestR * sv.proj.f;
            for (int y = std::max(0, (int)(sy - rpx)); y < std::min(FBH, (int)(sy + rpx) + 1); y++)
                for (int x = std::max(0, (int)(sx - rpx)); x < std::min(FBW, (int)(sx + rpx) + 1); x++) {
                    if ((x - sx) * (x - sx) + (y - sy) * (y - sy) > rpx * rpx) continue;
                    disc++; if (bankOf(fb.idx[y * FBW + x]) == 11) over++;
                }
        }
        printf("  %s: %s in the sky, %.1f deg across, %.0f deg up, %.0f deg from the pole, aurora over %d%% of its surroundings\n", b.name.c_str(), KINDS[sys.bodies[bestJ].type].name, 2 * bestR / DEG,
                std::asin(bestDl.y) / DEG, std::acos(clampd(bestDl.z * (latUse >= 0 ? 1 : -1) / std::sqrt(std::max(1e-9, 1 - bestDl.y * bestDl.y)), -1, 1)) / DEG, disc ? 100 * over / disc : 0);
        if (disc == 0 || over < disc / 12) return false;
        faceYawOut = yaw; facePitchOut = pitchB; placed = true;
    }
    return true;
}

bool SceneFind::rustValley() {   // P-02: a rust valley
    // a dry valley: a land window with a network of channels (the flood's own cells, as the river finder's), then in it
    // a valley floor (the wadi's sand, the drainage on) 10 m and more under the ground 150 m out to both sides; the site 90 m
    // up one side, facing across (a planet: a moon's sun at the asked height can be its parent's eclipse)
    if (b.parent >= 0) return false;
    bool found = false;
    const double Rm = b.radiusKm * 1000.0;
    double wla = 1e9, wln = 0;
    for (double la = latDeg; la < latDeg + 40 && wla > 1e8; la += 1.0)
        for (int k = 0; k < 360 && wla > 1e8; k++) {
            const double ln = k * TAU / 360;
            if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 512).material == MAT_WATER) continue;
            setDrainageEnabled(true);
            const DrainStats ds = drainageStats(g, StarSystem::bodyFromLatLon(la * DEG, ln), 12);
            setDrainageEnabled(false);
            if (ds.channels >= 6) { wla = la; wln = ln; }
        }
    if (wla > 1e8) return false;
    setDrainageEnabled(true);
    const double span = 12 * 400.0 / Rm / DEG, cl = std::cos(wla * DEG);
    for (double la = wla - span; la < wla + span && !found; la += span / 40)
        for (double ln = wln - span / cl; ln < wln + span / cl && !found; ln += span / cl / 40) {
            const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
            if (ss.material != MAT_SAND) continue;
            for (int a = 0; a < 8 && !found; a++) {   // eight directions over a half turn: the sides are (ang, ang + pi)
                const double ang = a * TAU / 16, dlat = std::cos(ang) / Rm, dlon = std::sin(ang) / (Rm * std::cos(la * DEG));
                const SurfaceSample sA = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + 150 * dlat, ln + 150 * dlon), 16);
                const SurfaceSample sB = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG - 150 * dlat, ln - 150 * dlon), 16);
                if (sA.height < ss.height + 10 || sB.height < ss.height + 10) continue;
                latUse = la * DEG + 90 * dlat; lon = ln + 90 * dlon; found = true;
                faceYawOut = std::atan2(-dlon * Rm * std::cos(la * DEG), -dlat * Rm);   // back across the floor
            }
        }
    setDrainageEnabled(false);
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::archeanFlats() {   // P-02: archean flats
    // an archean reef flat: on its stromatolites, the reef going on 16-48 m in some direction and the sea beyond within 200 m;
    // faced that way, across the reef to the water
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 40 && !found; la += 0.25)
        for (int k = 0; k < 1440 && !found; k++) {
            const double ln = k * TAU / 1440;
            const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
            if (ss.material != MAT_STROMATOLITE || ss.water > -1e8) continue;
            for (int a = 0; a < 16 && !found; a++) {
                const double ang = a * TAU / 16, dl = std::cos(ang) / Rm, dn = std::sin(ang) / (Rm * std::cos(la * DEG));
                int reef = 0, wet = 0;
                for (double dd : {16.0, 32.0, 48.0}) if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dl * dd, ln + dn * dd), 16).material == MAT_STROMATOLITE) reef++;
                if (reef < 2) continue;
                for (double dd = 64; dd <= 200; dd += 16) if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dl * dd, ln + dn * dd), 16).material == MAT_WATER) wet++;
                if (wet < 3) continue;
                lon = ln; latUse = la * DEG; found = true; faceYawOut = ang;
            }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::tundraIce() {   // P-02: tundra ice
    // a tundra's ice sheet: along each meridian the sheet's own edge (`iceSheetEdgeDeg`, north of the equator), and there
    // flat land (grass, under 900 m, not steep) 220 m short of it with the sheet's wall standing 15 m over it 60 m past the edge;
    // the site there, facing the wall
    bool found = false; const double Rm = b.radiusKm * 1000.0, back = 220.0 / Rm / DEG, past = 60.0 / Rm / DEG;
    if (!(KINDS[b.type].tags & KT_ICE_SHEETS)) return false;
    for (int k = 0; k < 720 && !found; k++) {
        const double ln = k * TAU / 720;
        double prev = 1e9;
        for (double la = g.iceCapLat - 8; la < g.iceCapLat + 8 && !found; la += 0.01) {
            const double in = la - iceSheetEdgeDeg(g, StarSystem::bodyFromLatLon(la * DEG, ln));
            if (!(prev < 0 && in >= 0)) { prev = in; continue; }
            prev = in;
            const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon((la - back) * DEG, ln), 16), sw = sampleSurface(g, StarSystem::bodyFromLatLon((la + past) * DEG, ln), 16);
            if (ss.material != MAT_GRASS || ss.water > -1e8 || ss.height < 5 || ss.height > 900 || ss.relief > 0.3) continue;
            if ((sw.material != MAT_SNOW && sw.material != MAT_ICE) || sw.height < ss.height + 15) continue;
            lon = ln; latUse = (la - back) * DEG; found = true; faceYawOut = 0;   // north, toward the pole
        }
    }
    if (!found) return false;
    {   // the clock from the northern midsummer (in winter the cap's snow lies far over the tundra)
        const double year = b.parent >= 0 ? sys.bodies[b.parent].orbitPeriod : b.orbitPeriod;
        double best = -2;
        for (int q = 0; q < 48; q++) { const double tq = 1000.0 + year * q / 48.0, se = sys.seasonOf(bi, tq); if (se > best) { best = se; t0Use = tq; } }
    }
    placed = true;
    return true;
}

bool SceneFind::hothouseOrSteppe() {   // P-03: a hothouse swamp or a steppe
    // -68 a hothouse's swamp: wetland grass past the asked latitude with forest within 300 m in some direction, faced; -69 a
    // steppe's open plain: grass on the grassland or the savanna with grass at the eight samples 250 and 600 m round it
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 30 && !found; la += 0.5)
        for (int k = 0; k < 720 && !found; k++) {
            const double ln = k * TAU / 720;
            const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
            if (ss.material != MAT_GRASS || ss.water > -1e8 || ss.height < 2) continue;
            if (wantMat == SCENE_HOTHOUSE_SWAMP && ss.biome != BIO_WETLAND) continue;
            if (wantMat == SCENE_STEPPE && ss.biome != BIO_GRASSLAND && ss.biome != BIO_SAVANNA) continue;
            int bestN = 0; double bestA = 0; bool open = true;
            for (int a = 0; a < 8; a++) {
                const double ang = a * TAU / 8; int n = 0;
                for (double dd : {60.0, 120.0, 250.0, 600.0}) {
                    const SurfaceSample s2 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + std::cos(ang) * dd / Rm, ln + std::sin(ang) * dd / (Rm * std::cos(la * DEG))), 16);
                    if (s2.material == MAT_FOREST) n++;
                    if ((dd == 250.0 || dd == 600.0) && s2.material != MAT_GRASS) open = false;
                }
                if (n > bestN) { bestN = n; bestA = ang; }
            }
            if (wantMat == SCENE_HOTHOUSE_SWAMP && bestN >= 2) { lon = ln; latUse = la * DEG; found = true; faceYawOut = bestA; }
            if (wantMat == SCENE_STEPPE && open) { lon = ln; latUse = la * DEG; found = true; }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::eyeballShore() {   // P-03: an eyeball world's shore
    // an eyeball's ice front: along 72 great circles from the substellar point, round the pupil's reach, the first ground of the
    // sea glacier over the floes (its cliff's top, 5 m over the sea, found to a metre); the site 40 m in from it, facing the open sea
    // and the fixed sun, whose height there is the light (`alt`)
    if (!g.locked || g.hasTrait(TR_HAZE)) return false;   // a haze hides the sea from its edge
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    const Vec3 sub = g.lockedDir, any = std::fabs(sub.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0);
    const Vec3 e1 = normalize(cross(sub, any)), e2 = cross(sub, e1);
    for (int k = 0; k < 72 && !found; k++) {
        const Vec3 dir = e1 * std::cos(k * TAU / 72) + e2 * std::sin(k * TAU / 72);
        auto at = [&](double thDeg) { return normalize(sub * std::cos(thDeg * DEG) + dir * std::sin(thDeg * DEG)); };
        double th = -1;
        // the shore wanders four degrees round the reach
        for (double q = std::max(0.5, g.seaReachDeg - 8); q < g.seaReachDeg + 8 && th < 0; q += 0.01) if (sampleSurface(g, at(q), 16).height > 5.0) th = q;
        if (th < 0) continue;
        double lo = th - 0.01, hi = th;
        for (int it = 0; it < 30; it++) { const double mid = 0.5 * (lo + hi); if (sampleSurface(g, at(mid), 16).height > 5.0) hi = mid; else lo = mid; }
        const double stand = hi + 40.0 / Rm / DEG;
        const Vec3 u = at(stand);
        const SurfaceSample ss = sampleSurface(g, u, 16);
        if (ss.material != MAT_SNOW && ss.material != MAT_ICE) continue;
        double la, ln; StarSystem::latLonFromBody(u, la, ln);
        latUse = la; lon = ln; found = true;
        alt = (90.0 - stand) * DEG;
        const Vec3 east = normalize(cross(Vec3(0, 0, 1), u)), north = cross(u, east), d = sub - u * dot(sub, u);
        faceYawOut = std::atan2(dot(d, east), dot(d, north));
    }
    if (!found) return false;
    { const double tc = clearHour(latUse, lon, 1000.0, 10800.0, 40); if (tc >= 0) t0Use = tc; }   // the open sea, not its haze
    placed = true;
    return true;
}

bool SceneFind::terminatorRing() {   // P-03: the terminator ring
    // a terminator world's ring: grass where the fixed sun stands at the light asked (within a degree), with forest within 300 m
    // toward the sun (within 90 degrees of it), faced: the trees against the glow of a sunset that never ends
    if (!g.locked) return false;
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 40 && !found; la += 0.5)
        for (int k = 0; k < 1440 && !found; k++) {
            const double ln = k * TAU / 1440;
            const Vec3 u = StarSystem::bodyFromLatLon(la * DEG, ln);
            if (std::fabs(std::asin(clampd(dot(u, g.lockedDir), -1, 1)) - alt) > 1.0 * DEG) continue;
            const SurfaceSample ss = sampleSurface(g, u, 16);
            if (ss.material != MAT_GRASS || ss.water > -1e8 || ss.height < 2) continue;
            const Vec3 east = normalize(cross(Vec3(0, 0, 1), u)), north = cross(u, east), d = g.lockedDir - u * dot(g.lockedDir, u);
            const double away = std::atan2(dot(d, east), dot(d, north));   // toward the sun that never sets: its forest against the glow
            int bestN = 0; double bestA = 0;
            for (int a = 0; a < 8; a++) {
                const double ang = a * TAU / 8; int n = 0;
                if (std::fabs(wrapAngle(ang - away)) > 1.6) continue;
                for (double dd : {60.0, 120.0, 200.0, 300.0})
                    if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + std::cos(ang) * dd / Rm, ln + std::sin(ang) * dd / (Rm * std::cos(la * DEG))), 16).material == MAT_FOREST) n++;
                if (n > bestN) { bestN = n; bestA = ang; }
            }
            if (bestN >= 2) { lon = ln; latUse = la * DEG; found = true; faceYawOut = bestA; }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::hyceanSea() {   // P-03: a hycean sea
    // a hycean's sea in a clear hour of the next twenty days (its haze is the sight; its rain hides it)
    const double tc = clearHour(latUse, lon, 1000.0, std::fabs(b.rotPeriod), 20);
    if (tc < 0) return false;
    t0Use = tc;
    return true;
}

bool SceneFind::ammoniaShore() {   // P-05: an ammonia shore
    // an ammonia sea's shore: silt 0.5-4 m over the sea with its water within 150 m in some direction and a slab of ice afloat
    // there; faced
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 50 && !found; la += 0.5)
        for (int k = 0; k < 720 && !found; k++) {
            const double ln = k * TAU / 720;
            const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
            if (ss.material != MAT_SAND || ss.height < 0.5 || ss.height > 4) continue;
            for (int a = 0; a < 16 && !found; a++) {
                const double ang = a * TAU / 16; int wet = 0, slab = 0;
                for (int dd = 20; dd <= 150; dd += 10) {
                    const SurfaceSample s2 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + std::cos(ang) * dd / Rm, ln + std::sin(ang) * dd / (Rm * std::cos(la * DEG))), 16);
                    if (s2.material == MAT_WATER) wet++;
                    if (s2.material == MAT_ICE && s2.height < 2) slab++;
                }
                if (wet >= 6 && slab >= 1) { lon = ln; latUse = la * DEG; found = true; faceYawOut = ang; }
            }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::snowballBelt() {   // P-05: a snowball's belt
    // a snowball's belt: along each meridian north from the equator, the first ice standing 5 m over the sea with open water 300 m
    // south of it (the sea glacier's cliff or the sheet's), found to a metre; the site 40 m north of it, facing south over the water;
    // at an equinox (the belt where the generator's own ground puts it)
    bool found = false; const double Rm = b.radiusKm * 1000.0, step = 300.0 / Rm / DEG;
    {   const double year = b.parent >= 0 ? sys.bodies[b.parent].orbitPeriod : b.orbitPeriod;
        double best = 2;
        for (int q = 0; q < 96; q++) { const double tq = 1000.0 + year * q / 96.0, se = std::fabs(sys.seasonOf(bi, tq)); if (se < best) { best = se; t0Use = tq; } }
    }
    auto edgeOn = [&](const BodyGen& gq, double ln, double la0, double la1, double& edge) {   // the first such ice on the meridian
        for (double la = la0; la < la1; la += 0.02) {
            const SurfaceSample ss = sampleSurface(gq, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
            if (ss.height <= 5 || ss.biome != BIO_ICE) continue;
            if (sampleSurface(gq, StarSystem::bodyFromLatLon((la - step) * DEG, ln), 16).material != MAT_WATER) return false;
            double lo = la - 0.02, hi = la;
            for (int it = 0; it < 30; it++) { const double mid = 0.5 * (lo + hi); if (sampleSurface(gq, StarSystem::bodyFromLatLon(mid * DEG, ln), 16).height > 5.0) hi = mid; else lo = mid; }
            edge = hi; return true;
        }
        return false;
    };
    for (int k = 0; k < 720 && !found; k++) {
        const double ln = k * TAU / 720;
        double edge0;
        if (!edgeOn(g, ln, 0, 30, edge0)) continue;
        // the season at the scene's own hour moves the sea ice by kilometres: the edge again on the ground of that hour
        SurfaceSite probe; probe.init(&sys, bi, edge0 * DEG, ln, t0Use);
        const double tq = findTime(probe, alt, t0Use);
        const BodyGen gq = BodyGen::make(b, sys.seasonOf(bi, tq));
        double edge;
        if (!edgeOn(gq, ln, std::max(0.0, edge0 - 3), edge0 + 3, edge)) continue;
        latUse = (edge + 40.0 / Rm / DEG) * DEG; lon = ln; faceYawOut = PI; found = true;
    }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::chemotrophVents() {   // P-05: chemotroph vents
    // a chemotroph's spring: the first mats or sinter of the scan (the basins a globe shows), then of `springVents` round it the
    // nearest pool on its own ground; the site 35 m from it on the mats or the sinter, facing it
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 50 && !found; la += 0.5)
        for (int k = 0; k < 720 && !found; k++) {
            const double ln = k * TAU / 720;
            const Vec3 u = StarSystem::bodyFromLatLon(la * DEG, ln);
            const int m512 = sampleSurface(g, u, 512).material;
            if (m512 != MAT_MATS && m512 != MAT_SALT) continue;
            std::vector<GeyserVent> vents; springVents(g, u, vents);
            for (const GeyserVent& v : vents) {
                if (found || sampleSurface(g, v.unit, 16).material != MAT_BRINE) continue;
                double vla, vlo; StarSystem::latLonFromBody(v.unit, vla, vlo);
                for (int a = 0; a < 8 && !found; a++) {
                    const double ang = a * TAU / 8, dlat = std::cos(ang) * 35.0 / Rm, dlon = std::sin(ang) * 35.0 / (Rm * std::cos(vla));
                    const int ms = sampleSurface(g, StarSystem::bodyFromLatLon(vla + dlat, vlo + dlon), 16).material;
                    if (ms != MAT_MATS && ms != MAT_SALT) continue;
                    latUse = vla + dlat; lon = vlo + dlon; found = true;
                    faceYawOut = std::atan2(-dlon * Rm * std::cos(vla), -dlat * Rm);   // back toward the pool
                }
            }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::deadForest() {   // P-04: a frozen or scorched forest
    // a dead world's old forest (its density kept in `veg`): along each meridian from the asked latitude, the first ground whose
    // forest was dense at 512 m and at 16 m, facing where it is densest 60 m round
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 40 && !found; la += 0.25)
        for (int k = 0; k < 720 && !found; k++) {
            const double ln = k * TAU / 720;
            if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 512).veg < 0.6) continue;
            if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16).veg < 0.6) continue;
            double best = -1;
            for (int a = 0; a < 8; a++) {
                const double ang = a * TAU / 8, v = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + std::cos(ang) * 60.0 / Rm, ln + std::sin(ang) * 60.0 / (Rm * std::cos(la * DEG))), 16).veg;
                if (v > best) { best = v; faceYawOut = ang; }
            }
            latUse = la * DEG; lon = ln; found = true;
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::scorchedSeabed() {   // P-04: a scorched seabed
    // a scorched world's old seabed under its giant: along each meridian from the asked latitude, the first old shore (land at 512 m
    // with the bed 900 m south of it and 80 m under it: a shore that drops, not a flat), the site on the bed 900 m out, facing the shore
    if (sys.star.cls != STAR_RED_GIANT && sys.star.cls != STAR_ORANGE_GIANT && sys.star.cls != STAR_CARBON) return false;
    bool found = false; const double Rm = b.radiusKm * 1000.0, dla = 900.0 / Rm;
    for (double la = latDeg; la < latDeg + 40 && !found; la += 0.1)
        for (int k = 0; k < 720 && !found; k++) {
            const double ln = k * TAU / 720;
            const SurfaceSample land = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 512);
            if (land.oldSea > 0.5 || land.height < 2) continue;
            const SurfaceSample bed = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG - dla, ln), 512);
            if (bed.oldSea < 0.5 || bed.height > land.height - 80) continue;
            latUse = la * DEG - dla; lon = ln; faceYawOut = 0; found = true;   // north, up to the old shore
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::nitrogenCells() {   // P-05: nitrogen cells
    // a nitrogen plain among its cells under a sun-like light: from the great plain's middle out along each bearing (every 10
    // degrees), the first plain ground at 512 m with a block of water ice standing 400 m over it 9 km further out, facing it
    if (sys.star.cls != STAR_YELLOW && sys.star.cls != STAR_ORANGE && sys.star.cls != STAR_RED_DWARF) return false;
    bool found = false; const double Rkm = b.radiusKm;
    const Vec3 bc = nitrogenBasinOf(g), e1 = normalize(cross(bc, std::fabs(bc.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(bc, e1);
    for (double r = 6.0; r < 30.0 && !found; r += 0.25)
        for (int a = 0; a < 36 && !found; a++) {
            const Vec3 dir = e1 * std::cos(a * 10 * DEG) + e2 * std::sin(a * 10 * DEG);
            const Vec3 u = normalize(bc * std::cos(r * DEG) + dir * std::sin(r * DEG)), u3 = normalize(bc * std::cos(r * DEG + 9.0 / Rkm) + dir * std::sin(r * DEG + 9.0 / Rkm));
            if (sampleSurface(g, u, 512).material != MAT_ICE) continue;
            const SurfaceSample blk = sampleSurface(g, u3, 512);
            if (blk.material != MAT_ROCK || blk.height < g.seaLevel + 400) continue;   // a block standing 400 m over the plain
            StarSystem::latLonFromBody(u, latUse, lon);
            double la3, lo3; StarSystem::latLonFromBody(u3, la3, lo3);
            faceYawOut = std::atan2(wrapAngle(lo3 - lon) * std::cos(latUse), la3 - latUse); found = true;
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::wandererSnow() {   // P-05: a wanderer's snow
    // a wanderer's snow plain: along each meridian from the asked latitude, the first snow at 512 m with the old world's highland
    // bare (rock or its dark ground at 512 m, 250 m higher) 1.5 km off in one of eight directions, facing it
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 50 && !found; la += 0.5)
        for (int k = 0; k < 360 && !found; k++) {
            const double ln = k * TAU / 360;
            const SurfaceSample here = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 512);
            if (here.material != MAT_SNOW) continue;
            for (int a = 0; a < 8 && !found; a++) {
                const double ang = a * TAU / 8;
                const SurfaceSample hi = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + std::cos(ang) * 1500.0 / Rm, ln + std::sin(ang) * 1500.0 / (Rm * std::cos(la * DEG))), 512);
                if ((hi.material != MAT_ROCK && hi.material != MAT_DUST) || hi.height < here.height + 250) continue;
                latUse = la * DEG; lon = ln; faceYawOut = ang; found = true;
            }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::brineFaculae() {   // P-05: brine faculae
    // a brine dwarf's faculae: the first salt of a scan at 64 m that the 512 m ring does not see (a young crater a few kilometres
    // across, 30 km and more from the lone mountain); along the first of eight bearings where the salt ends within 4 km, the stand on the
    // salt 300 m in from its edge, facing across it to the crater's far wall
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    const Vec3 mtn = loneMountainOf(g);
    for (double la = latDeg; la < latDeg + 60 && !found; la += 0.2)
        for (int k = 0; k < 1440 && !found; k++) {
            const double ln = k * TAU / 1440;
            const Vec3 u = StarSystem::bodyFromLatLon(la * DEG, ln);
            if (std::acos(clampd(dot(u, mtn), -1, 1)) * b.radiusKm < 30.0) continue;
            if (sampleSurface(g, u, 64).material != MAT_SALT || sampleSurface(g, u, 512).material == MAT_SALT) continue;
            for (int a = 0; a < 8 && !found; a++) {
                const double ang = a * TAU / 8;
                double edge = -1;
                for (double d = 100; d <= 4000 && edge < 0; d += 100)
                    if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + std::cos(ang) * d / Rm, ln + std::sin(ang) * d / (Rm * std::cos(la * DEG))), 64).material != MAT_SALT) edge = d;
                if (edge < 1000) continue;
                const double st = edge - 300;
                latUse = la * DEG + std::cos(ang) * st / Rm; lon = ln + std::sin(ang) * st / (Rm * std::cos(la * DEG)); faceYawOut = ang + PI; found = true;
            }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::frostDwarf() {   // P-05: a frost dwarf
    // a frost dwarf under a sun-like light: the first tholin of a scan at 512 m, the stand 60 m off on the frost, facing it
    if (sys.star.cls != STAR_YELLOW && sys.star.cls != STAR_ORANGE && sys.star.cls != STAR_RED_DWARF) return false;
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 60 && !found; la += 0.5)
        for (int k = 0; k < 720 && !found; k++) {
            const double ln = k * TAU / 720;
            if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 512).material != MAT_DUST) continue;
            for (int a = 0; a < 8 && !found; a++) {
                const double ang = a * TAU / 8, dlat = std::cos(ang) * 60.0 / Rm, dlon = std::sin(ang) * 60.0 / (Rm * std::cos(la * DEG));
                if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), 16).material != MAT_SNOW) continue;
                latUse = la * DEG + dlat; lon = ln + dlon; faceYawOut = ang + PI; found = true;
            }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::groovedLanes() {   // P-06: a grooved moon's lanes
    // a grooved moon's lane: along each meridian from the asked latitude, the first dark ground at 512 m with the lane's bright ice
    // 4 and 8 km off in one of eight directions and lower; the stand 6 km into the lane (it lies below the dark ground's horizon), facing on
    // across its grooves
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 30 && !found; la += 0.5)
        for (int k = 0; k < 720 && !found; k++) {
            const double ln = k * TAU / 720;
            const SurfaceSample here = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 512);
            if (here.material != MAT_DUST) continue;
            for (int a = 0; a < 8 && !found; a++) {
                const double ang = a * TAU / 8, cs = std::cos(ang), sn = std::sin(ang) / std::cos(la * DEG);
                const SurfaceSample ln4 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + cs * 4000.0 / Rm, ln + sn * 4000.0 / Rm), 512);
                const SurfaceSample ln8 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + cs * 8000.0 / Rm, ln + sn * 8000.0 / Rm), 512);
                if (ln4.material != MAT_ICE || ln8.material != MAT_ICE || ln8.height > here.height - 150) continue;
                latUse = la * DEG + cs * 6000.0 / Rm; lon = ln + sn * 6000.0 / Rm; found = true;
            }
        }
    if (!found) return false;
    // across the grooves: the bearing whose ground varies most over 6 km either way; the stand in the lowest trough along it within 4 km,
    // facing along the trough (a quarter turn), between the ridges
    double bestVar = -1, across = 0;
    for (int a = 0; a < 16; a++) {
        const double ang = a * PI / 16, cs = std::cos(ang), sn = std::sin(ang) / std::cos(latUse);
        double sum = 0, sum2 = 0;
        for (int q = -30; q <= 30; q++) {
            const double hh = sampleSurface(g, StarSystem::bodyFromLatLon(latUse + cs * q * 200.0 / Rm, lon + sn * q * 200.0 / Rm), 64).height;
            sum += hh;
            sum2 += hh * hh;
        }
        const double var = sum2 / 61 - (sum / 61) * (sum / 61);
        if (var > bestVar) { bestVar = var; across = ang; }
    }
    {   const double cs = std::cos(across), sn = std::sin(across) / std::cos(latUse);
        double low = 1e18, at = 0;
        for (int q = -20; q <= 20; q++) {
            const double hh = sampleSurface(g, StarSystem::bodyFromLatLon(latUse + cs * q * 200.0 / Rm, lon + sn * q * 200.0 / Rm), 64).height;
            if (hh < low) {
                low = hh;
                at = q * 200.0;
            }
        }
        latUse += cs * at / Rm; lon += sn * at / Rm; }
    faceYawOut = across + PI / 2;
    placed = true;
    return true;
}

bool SceneFind::twoFacedBoundary() {   // P-06: a two-faced moon's boundary
    // a two-faced moon's boundary: along each meridian from the asked latitude, the first bright ice at 64 m with bright ice 300 m behind it
    // and the dark face from 100 m ahead to 2 km (a small moon's horizon is a kilometre or two away) in one of eight directions; facing along the edge
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 40 && !found; la += 0.5)
        for (int k = 0; k < 720 && !found; k++) {
            const double ln = k * TAU / 720;
            if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 64).material != MAT_ICE) continue;
            for (int a = 0; a < 8 && !found; a++) {
                const double ang = a * TAU / 8, cs = std::cos(ang), sn = std::sin(ang) / std::cos(la * DEG);
                bool ok = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG - cs * 300.0 / Rm, ln - sn * 300.0 / Rm), 64).material == MAT_ICE;
                for (double d = 100; d <= 2000 && ok; d += 100) ok = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + cs * d / Rm, ln + sn * d / Rm), 64).material == MAT_DUST;
                if (!ok) continue;
                latUse = la * DEG; lon = ln; faceYawOut = ang - PI / 2; found = true;   // along the edge: the bright ice to the left, the dark to the right
            }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::twoFacedRidge() {   // P-06: a two-faced moon's ridge
    const Vec3 ap = twoFacedApex(g);   // past the dark face's edge (its relief seen in the light; on the coal-black face it hardly shows), facing it
    double best = -1e9, bestLon = 0;
    for (int k = 95; k <= 145; k++)
        for (int side = -1; side <= 1; side += 2) {
            const double ln = std::atan2(ap.y, ap.x) + side * k * DEG;
            const SurfaceSample cr = sampleSurface(g, StarSystem::bodyFromLatLon(0, ln), 2048);
            const double rise = cr.height - sampleSurface(g, StarSystem::bodyFromLatLon(-25.0 / b.radiusKm, ln), 2048).height;
            if (cr.material != MAT_ICE || rise <= best) continue;
            bool clear = true;   // no dark ground between the stand and the crest, nor round the stand (the faces' mixed band)
            for (int q = 0; q <= 25 && clear; q++) clear = sampleSurface(g, StarSystem::bodyFromLatLon(-q * 1.0 / b.radiusKm, ln), 64).material == MAT_ICE;
            for (int q = -8; q <= 8 && clear; q++) clear = sampleSurface(g, StarSystem::bodyFromLatLon(-25.0 / b.radiusKm, ln + q * 0.5 / b.radiusKm), 64).material == MAT_ICE;
            if (!clear) continue;
            best = rise; bestLon = ln;
        }
    if (best < -1e8) return false;
    latUse = -25.0 / b.radiusKm; lon = bestLon; faceYawOut = 0; placed = true;
    return true;
}

bool SceneFind::spongePits() {   // P-06: sponge pits
    // a sponge's pit from its rim: along each meridian from the asked latitude, the first dark floor at 512 m; along the first of eight
    // bearings whose ground rises 1.5 km and more within 12 km, the highest point of it (the rim), the stand on it, facing back over the floor
    bool found = false; const double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 40 && !found; la += 0.5)
        for (int k = 0; k < 360 && !found; k++) {
            const double ln = k * TAU / 360;
            const SurfaceSample fl = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 512);
            if (fl.material != MAT_DUST) continue;
            for (int a = 0; a < 8 && !found; a++) {
                const double ang = a * TAU / 8, cs = std::cos(ang), sn = std::sin(ang) / std::cos(la * DEG);
                double top = -1e9, at = 0;
                for (double d = 250; d <= 12000; d += 250) {
                    const double hh = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + cs * d / Rm, ln + sn * d / Rm), 512).height;
                    if (hh > top) { top = hh; at = d; }
                }
                if (top < fl.height + 1500) continue;
                const double st = at;
                latUse = la * DEG + cs * st / Rm; lon = ln + sn * st / Rm; faceYawOut = ang + PI; found = true;
            }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::traitGround() {   // R-307, P-01, P-02: a kind's own ground (a crack, a fissure, dunes, a shore, a vent, a flow, moss)
    // the new types' scenes: -30 a stained crack (europan), -31 a lava fissure (tectonic), -32 dunes
    // (desert), -33 / -35 the shore of a methane or an acid sea, -34 a rayed plain (bombarded); P-01: -37 the magma sea (the
    // standing point where the fixed sun stands at the light asked), -38 a steam vent's sinter, -39 a brine pool, -55 a glass
    // flow, each faced; P-02: -59 a lichen world's moss, faced from 20-50 m
    int mat = wantMat == SCENE_STAINED_CRACK
            ? MAT_DUST
            : (wantMat == SCENE_LAVA_FISSURE || wantMat == SCENE_MAGMA_SHORE
                              ? MAT_LAVA
                              : (wantMat == SCENE_DUNES
                                                ? MAT_SAND
                                                : (wantMat == SCENE_STEAM_VENT
                                                                  ? MAT_SALT
                                                                  : (wantMat == SCENE_BRINE_POOL ? MAT_BRINE
                                                                                                 : (wantMat == SCENE_GLASS_FLOW ? MAT_GLASS : (wantMat == SCENE_LICHEN ? MAT_MOSS : MAT_WATER))))));
    const int standOn = wantMat == SCENE_LICHEN ? MAT_CRUST : -1;   // the moss faced from its crusts
    // -59 a lichen world's moss
    const bool faceIt = wantMat == SCENE_MAGMA_SHORE || wantMat == SCENE_STEAM_VENT || wantMat == SCENE_BRINE_POOL || wantMat == SCENE_GLASS_FLOW || wantMat == SCENE_LICHEN;
    bool nearOnly = wantMat == SCENE_METHANE_SHORE || wantMat == SCENE_ACID_SHORE || wantMat == SCENE_LAVA_FISSURE || faceIt;   // stand 60-150 m from it
    bool found = false;
    double faceYaw = 1e9;
    double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 40 && !found; la += 1.0)
        for (int k = 0; k < 360 && !found; k++) {
            double ln = k * TAU / 360;
            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
            if (wantMat == SCENE_RAYED_PLAIN) { if (ss.height > 0 && ss.albedo > 0.3) { lon = ln; latUse = la * DEG; found = true; } continue; }
            if (ss.material != mat) continue;
            if (wantMat == SCENE_STAINED_CRACK) {   // a narrow linea: ice 120 m out on both sides in some direction; stand on it 70 m off, facing the stain
                for (int a = 0; a < 8 && !found; a++) {
                    double ang = a * TAU / 16;   // eight directions over a half turn: the pair is (ang, ang + pi)
                    double dlat = std::cos(ang) * 120.0 / Rm, dlon = std::sin(ang) * 120.0 / (Rm * std::cos(la * DEG));
                    SurfaceSample sA = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), 16);
                    SurfaceSample sB = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG - dlat, ln - dlon), 16);
                    if (sA.material != MAT_ICE || sB.material != MAT_ICE) continue;
                    lon = ln + dlon * 70.0 / 120.0; latUse = la * DEG + dlat * 70.0 / 120.0; found = true;
                    faceYaw = std::atan2(-dlon * Rm * std::cos(la * DEG), -dlat * Rm);   // toward the stain (east = x, north = z)
                }
                continue;
            }
            if (!nearOnly) { lon = ln; latUse = la * DEG; found = true; continue; }
            for (int a = 0; a < 8 && !found; a++) {   // the nearest firm ground 60-150 m out (P-02: the moss's 20-50 m)
                for (double dd = wantMat == SCENE_LICHEN ? 20 : 60; dd <= (wantMat == SCENE_LICHEN ? 50 : 150) && !found; dd += wantMat == SCENE_LICHEN ? 10 : 30) {
                    double ang = a * TAU / 8;
                    double dlat = std::cos(ang) * dd / Rm, dlon = std::sin(ang) * dd / (Rm * std::cos(la * DEG));
                    SurfaceSample s2 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), 16);
                    if (s2.material == mat || (s2.water > -1e8 && s2.height < s2.water + 0.3)) continue;
                    if (standOn >= 0 && s2.material != standOn) continue;
                    if (wantMat == SCENE_MAGMA_SHORE && std::fabs(90.0 - std::acos(clampd(dot(StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), g.lockedDir), -1, 1)) / DEG - alt / DEG) > 2.0)
                        continue;
                    if (faceIt && wantMat != SCENE_MAGMA_SHORE) {   // clear of it for 40 m round (a pool or a flow at the feet filled the view)
                        bool clearAround = true;
                        for (int q = 0; q < 8 && clearAround; q++) {
                            const double qr = wantMat == SCENE_LICHEN ? 12.0 : 40.0, qa = q * TAU / 8, ql = std::cos(qa) * qr / Rm, qn = std::sin(qa) * qr / (Rm * std::cos(la * DEG));
                            if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat + ql, ln + dlon + qn), 16).material == mat) clearAround = false;
                        }
                        if (!clearAround) continue;
                    }
                    lon = ln + dlon; latUse = la * DEG + dlat; found = true;
                    if (faceIt) faceYaw = std::atan2(-dlon * Rm * std::cos(la * DEG), -dlat * Rm);   // back toward it
                }
            }
        }
    if (!found) return false;
    lookAtWater = wantMat == SCENE_METHANE_SHORE || wantMat == SCENE_ACID_SHORE; placed = true;
    if (faceYaw < 1e8) faceYawOut = faceYaw;
    return true;
}

bool SceneFind::neutronGlass() {   // S-03: a neutron star's glass
    // a glassed world: the first glass sheet of the scan at 16 m with glass on the eight samples 100 m round it
    if (!g.hasTrait(TR_GLASSED)) return false;
    bool found = false; double Rm = b.radiusKm * 1000.0;
    for (double la = latDeg; la < latDeg + 40 && !found; la += 1.0)
        for (int k = 0; k < 360 && !found; k++) {
            double ln = k * TAU / 360;
            if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16).material != MAT_GLASS) continue;
            int glassy = 0;
            for (int j = -1; j <= 1; j++)
                for (int i = -1; i <= 1; i++)
                    if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + j * 100.0 / Rm, ln + i * 100.0 / (Rm * std::cos(la * DEG))), 16).material == MAT_GLASS) glassy++;
            if (glassy >= 9) { lon = ln; latUse = la * DEG; found = true; }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::biome() {   // N2: a site of one biome
    // a site of one biome (wantMat = -10 - biome), on land (S-04: the codes under -21 are the other finders', and
    // -44 wants the type's default spot: this branch swallowed it and found no biome 34)
    int wantBio = SCENE_BIOME - wantMat;
    bool found = false;
    for (double la = latDeg; la < latDeg + 34 && !found; la += 1.5)
        for (int k = 0; k < 360 && !found; k++) {
            double ln = k * TAU / 360;
            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
            if (ss.biome == wantBio && ss.material != MAT_WATER && ss.water < -1e8) { lon = ln; latUse = la * DEG; found = true; }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::inlandWater() {   // B-320: a river or a lake
    // inland water: the first river (-21) or lake (-22) of the scan (`SurfaceSample::waterKind`), then
    // the nearest dry land round it, 20-300 m out; the site stands there facing the water
    bool found = false;
    double Rm = b.radiusKm * 1000.0;
    int wantKind = wantMat == SCENE_RIVER ? 1 : 2;
    int hits = 0, noLand = 0;
    // a land window first (coarse, no drainage), then its tile's flood: the window is scanned at 16 m only
    // where the flood has channels (or lakes), the drainage on
    double wla = 1e9, wln = 0;
    for (double la = latDeg; la < latDeg + 40 && wla > 1e8; la += 1.0)
        for (int k = 0; k < 360 && wla > 1e8; k++) {
            double ln = k * TAU / 360;
            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 512);
            if (ss.material == MAT_WATER || ss.height < 20 || ss.biome == BIO_ICE || ss.biome == BIO_DESERT) continue;
            setDrainageEnabled(true);
            DrainStats ds = drainageStats(g, StarSystem::bodyFromLatLon(la * DEG, ln), 12);
            setDrainageEnabled(false);
            if ((wantKind == 1 && ds.channels >= 6) || (wantKind == 2 && ds.lakes >= 12)) { wla = la; wln = ln; }
        }
    if (wla > 1e8) return false;
    setDrainageEnabled(true);
    double span = 12 * 400.0 / Rm / DEG;   // the window's half-size in degrees
    for (double la = wla - span; la < wla + span && !found; la += span / 40)
        for (double ln = wln - span / std::cos(wla * DEG); ln < wln + span / std::cos(wla * DEG) && !found; ln += span / std::cos(wla * DEG) / 40) {
            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
            if (ss.material != MAT_WATER || ss.waterKind != wantKind) continue;
            hits++;
            double bestD = 1e9, bla = 0, bln = 0;
            for (int a = 0; a < 16; a++)
                for (double dd = 20; dd <= 300 && dd < bestD; dd += 20) {
                    double ang = a * TAU / 16;
                    double dlat = std::cos(ang) * dd / Rm, dlon = std::sin(ang) * dd / (Rm * std::cos(la * DEG));
                    SurfaceSample s2 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), 16);
                    if (s2.material == MAT_WATER || (s2.water > -1e8 && s2.height < s2.water + 0.3)) continue;
                    bestD = dd; bla = la * DEG + dlat; bln = ln + dlon; break;
                }
            if (bestD > 1e8) { noLand++; continue; }
            lon = bln; latUse = bla; found = true;
        }
    if (hits && !found) printf("  water finder: %s: %d hits of kind %d, none with land within 300 m\n", b.name.c_str(), hits, wantKind);
    (void)noLand;
    setDrainageEnabled(false);
    if (!found) return false;
    lookAtWater = true; placed = true;
    return true;
}

bool SceneFind::herdOrBuggy() {   // N3, N4: a herd, the buggy
    // open grassland, so a herd or the buggy can be seen
    bool found = false;
    for (double la = latDeg; la < latDeg + 30 && !found; la += 1.5)
        for (int k = 0; k < 360 && !found; k++) {
            double ln = k * TAU / 360;
            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
            if (ss.material == MAT_GRASS && (ss.biome == BIO_GRASSLAND || ss.biome == BIO_SAVANNA) && ss.height > 2) { lon = ln; latUse = la * DEG; found = true; }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::forest() {   // G-04: a deep forest
    // deep forest (dense vegetation), for the coverage and the bench
    bool found = false;
    for (double la = latDeg; la < latDeg + 30 && !found; la += 1.5)
        for (int k = 0; k < 360 && !found; k++) {
            double ln = k * TAU / 360;
            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
            if (ss.material != MAT_FOREST || ss.veg <= 0.75 || ss.height <= 5) continue;
            // the eight samples 100 m round it as well: the scan's first dense cell stood at a wood's edge (a fifth of the ground under canopy)
            int dense = 0; double Rm = b.radiusKm * 1000.0;
            for (int j = -1; j <= 1; j++)
                for (int i = -1; i <= 1; i++) {
                    if (!i && !j) continue;
                    SurfaceSample s2 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + j * 100.0 / Rm, ln + i * 100.0 / (Rm * std::cos(la * DEG))), 16);
                    if (s2.material == MAT_FOREST && s2.veg > 0.75) dense++;
                }
            if (dense >= 8) { lon = ln; latUse = la * DEG; found = true; }
        }
    if (!found) return false;
    placed = true;
    return true;
}

bool SceneFind::settlement(double& t, SunInfo& si) {   // C-01, C-08 to C-13: a people's place, its settlement faced
    // cells), from 30 m outside its edge, facing its centre; C-08: -50 the nearest with a harbour, -51 the nearest terraced (within 30 rings); C-09: -52 the nearest with a road, on the road
    bool found = false, roadStand = false; int gLat0, gLon0, bestClass = -1; double bestD = 1e18; Ruin best; RuinSpec bestSpec; std::vector<Road> bestRoads;
    sv.ruinCellAt(0, 0, gLat0, gLon0);
    int maxRing = wantMat == SCENE_HARBOUR || wantMat == SCENE_TERRACES || wantMat == SCENE_SECOND_PEOPLE ? 30 : 14;
    for (int ring = 0; ring < maxRing && !(found && ring > 4); ring++)
        for (int dl = -ring; dl <= ring; dl++)
            for (int dn = -ring; dn <= ring; dn++) {
                if (std::max(std::abs(dl), std::abs(dn)) != ring) continue;
                Ruin ru;
                if (!sv.ruinAt(gLat0 + dl, gLon0 + dn, ru)) continue;
                if (wantMat == SCENE_HARBOUR && !ru.spec->harbour) continue;
                if (wantMat == SCENE_SECOND_PEOPLE && ru.spec->people != 1) continue;   // the second people's
                if (wantMat == SCENE_TERRACES && !ru.spec->terraced) continue;
                std::vector<Road> rds;
                if (wantMat == SCENE_ROAD) { roadsOfCell(sv.site.gen, gLat0 + dl, gLon0 + dn, rds); if (rds.empty()) continue; }
                // a burial ground of three stones at least
                if (wantMat == SCENE_GRAVES) { std::vector<Grave> gr; gravesOf(sv.lores[ru.spec->people], *ru.spec, sv.cultures[ru.spec->people], gr); if (gr.size() < 3) continue; }
                double d = std::sqrt(ru.x * ru.x + ru.z * ru.z);
                int cls = settlementRank(ru.spec->sclass);
                if (cls > bestClass || (cls == bestClass && d < bestD)) { best = ru; bestSpec = *ru.spec; bestClass = cls; bestD = d; found = true; bestRoads = rds; }
            }
    if (!found) {
        printf("  %s: no %s within %d cells\n", b.name.c_str(),
                wantMat == SCENE_HARBOUR
                        ? "harbour"
                        : (wantMat == SCENE_TERRACES ? "terraced settlement"
                                                     : (wantMat == SCENE_ROAD ? "settlement with a road"
                                                                              : (wantMat == SCENE_GRAVES ? "settlement with three graves"
                                                                                                         : (wantMat == SCENE_SECOND_PEOPLE ? "settlement of the second people" : "settlement")))),
                maxRing);
        return false;
    }
    double h0 = bestSpec.heading, ext = 0;   // the buildings' reach along the street's axis (a grid town is more compact than its radius)
    for (const Building& bd : bestSpec.buildings) {
        double along = bd.x * std::sin(h0) + bd.z * std::cos(h0), across = bd.x * std::cos(h0) - bd.z * std::sin(h0);
        if (std::fabs(across) < 12) ext = std::max(ext, along + std::max(bd.hw, bd.hd));
    }
    double la2, lo2, out = ext + 9;   // outside the gate (the street's axis), looking down the street
    double standX = best.x + std::sin(h0) * out, standZ = best.z + std::cos(h0) * out, yaw = h0 + PI, pitch = 0.0;
    std::string shardLine;
    if (wantMat == SCENE_SHARD) {   // on the floor of the room of the settlement's first shard, 1.9 m from it toward the room's middle (a stela: in front of it), looking down at it
        std::vector<ShardSite> sites; recordSitesOf(sv.site.gen, bestSpec, sv.cultures[bestSpec.people], sites);
        if (sites.empty()) { printf("  %s: the settlement holds no shard\n", b.name.c_str()); return false; }
        const ShardSite& s = sites[0]; const Building& bd = bestSpec.buildings[s.building];
        double shx = best.x + s.x, shz = best.z + s.z;
        double ax = bd.x - s.x, az = bd.z - s.z, L = std::sqrt(ax * ax + az * az);
        if (L < 1e-6) { ax = 0; az = 1; L = 1; }
        if (s.outside) { ax = -ax; az = -az; }
        standX = shx + ax / L * 1.9; standZ = shz + az / L * 1.9;
        yaw = std::atan2(shx - standX, shz - standZ); pitch = -0.42;
        shardLine = fmt("  shard %d of the settlement's %zu: %s (%s %d), %.1f m from the building's centre; the explorer 1.9 m from it", s.index, sites.size(), buildingPlaceName(s.place),
                BUILDING_KIND_NAMES[bd.kind], s.building, L);
    }
    if (wantMat == SCENE_HARBOUR) {   // at the quay's end away from the mole, a few metres up the beach, looking along the quay at the mole's far end
        const SiteRead& st = bestSpec.site;
        const Building* quay = nullptr; const Building* mole = nullptr;
        for (const Building& bd : bestSpec.buildings) { if (bd.kind == BK_QUAY && !quay) quay = &bd; if (bd.kind == BK_MOLE && !mole) mole = &bd; }
        double sx = std::sin(st.shoreDir), sz = std::cos(st.shoreDir);
        if (quay && mole) {
            double ax = std::sin(quay->heading), az = std::cos(quay->heading);   // along the quay
            double toMole = (mole->x - quay->x) * ax + (mole->z - quay->z) * az, end = toMole > 0 ? -1 : 1;
            standX = best.x + quay->x + ax * end * (quay->hw * 0.5) - sx * 14; standZ = best.z + quay->z + az * end * (quay->hw * 0.5) - sz * 14;   // half way to the quay's far end, 14 m up the beach
            double mx = best.x + mole->x + sx * mole->hd * 0.8, mz = best.z + mole->z + sz * mole->hd * 0.8;
            yaw = std::atan2(mx - standX, mz - standZ); pitch = -0.02;
        } else { double q = st.shoreDist - 3.0; standX = best.x + sx * (q - 20); standZ = best.z + sz * (q - 20); yaw = st.shoreDir; pitch = -0.03; }
        // out on the sea's ice, facing the harbour
        if (st.shoreIce) { const double q = st.shoreDist + 70.0; standX = best.x + sx * q; standZ = best.z + sz * q; yaw = st.shoreDir + PI; pitch = 0.0; }
        shardLine = fmt("  the harbour: the shore %.0f m from the centre at %.0f deg%s; features: %s", st.shoreDist, st.shoreDir / DEG,
                st.shoreDry ? " (the old sea, dry)" : (st.shoreIce ? " (the sea frozen)" : ""), featureList(bestSpec).c_str());
    } else if (wantMat == SCENE_ROAD) {   // on the settlement's longest road, 60 m out from its edge, looking along it toward the other end
        const Road* rd = &bestRoads[0];
        for (const Road& r : bestRoads) if (r.lengthM > rd->lengthM) rd = &r;
        bool fromA = roadCellKey(sv.site.gen, rd->a.gLat, rd->a.gLon) == roadCellKey(sv.site.gen, gLat0, gLon0) ||
                roadDistanceM(sv.site.gen, rd->a.unit, sv.site.unitAt(best.x, best.z)) < roadDistanceM(sv.site.gen, rd->b.unit, sv.site.unitAt(best.x, best.z));
        std::vector<Vec3> way = rd->pts; if (!fromA) std::reverse(way.begin(), way.end());
        double along = 0; size_t k = 0;
        for (; k + 1 < way.size() && along < 60; k++) along += roadDistanceM(sv.site.gen, way[k], way[k + 1]);
        size_t ahead = std::min(way.size() - 1, k + 3);
        sv.site.localAt(way[k], standX, standZ);
        double ax, az; sv.site.localAt(way[ahead], ax, az);
        yaw = std::atan2(ax - standX, az - standZ); pitch = -0.04;
        shardLine = fmt("  the road: %s %d %d -> %s %d %d, %.0f m along (%.0f straight), %d bends, steepest %.3f, %.1f m wide, %s; the culture's roads %s, wear %.2f; standing %.0f m along it%s",
                SETTLEMENT_CLASS_NAMES[rd->a.sclass], rd->a.gLat, rd->a.gLon, SETTLEMENT_CLASS_NAMES[rd->b.sclass], rd->b.gLat, rd->b.gLon, rd->lengthM, rd->straightM, rd->bends, rd->maxGrade,
                2 * rd->halfWidth, fromA ? "from its a end" : "from its b end", sv.cultures[bestSpec.people].paved ? "paved" : "beaten", sv.cultures[bestSpec.people].roadWear, along,
                bestRoads.size() > 1 ? fmt(" (%zu roads at this settlement)", bestRoads.size()).c_str() : "");
        roadStand = true;
    } else if (wantMat == SCENE_GRAVES) {   // two metres before the first row of stones, at its end, looking along the row toward the ruins
        std::vector<Grave> gr; gravesOf(sv.lores[bestSpec.people], bestSpec, sv.cultures[bestSpec.people], gr);
        const Grave& g0 = gr[0]; const Grave& g1 = gr[std::min<size_t>(2, gr.size() - 1)];
        double fx = std::sin(g0.heading), fz = std::cos(g0.heading);   // the stones face the settlement
        standX = best.x + g0.x + fx * 2.2 + (g0.x - g1.x) * 0.6; standZ = best.z + g0.z + fz * 2.2 + (g0.z - g1.z) * 0.6;
        double ax = best.x + (g0.x + g1.x) * 0.5, az = best.z + (g0.z + g1.z) * 0.5;
        yaw = std::atan2(ax - standX, az - standZ); pitch = -0.12;
        int fallenN = 0; for (const Grave& x : gr) if (x.fallen) fallenN++;
        shardLine = fmt("  the graves: %zu stones (%d fallen), %.0f m out from the centre, facing %.0f deg; the first %s; the calendar: %s; the span %d years", gr.size(), fallenN,
                std::sqrt(g0.x * g0.x + g0.z * g0.z), g0.heading / DEG, graveLine(sv.lores[bestSpec.people], g0).c_str(), calendarLine(sv.lores[bestSpec.people]).c_str(),
                sv.lores[bestSpec.people].spanYears + 1);
    } else if (wantMat == SCENE_TERRACES) {   // below the slope, looking up across the terraces
        const SiteRead& st = bestSpec.site;
        double dx = std::sin(st.downhill), dz = std::cos(st.downhill);
        standX = best.x + dx * (bestSpec.size + 12); standZ = best.z + dz * (bestSpec.size + 12); yaw = st.downhill + PI; pitch = 0.06;
        int terr = 0; for (const Building& bd : bestSpec.buildings) if (bd.kind == BK_TERRACE) terr++;
        shardLine = fmt("  the terraces: a grade of %.3f falling to %.0f deg, %d retaining walls; features: %s", st.slope, st.downhill / DEG, terr, featureList(bestSpec).c_str());
    }
    sv.site.latLonAt(standX, standZ, la2, lo2);
    sv.init(&sys, bi, la2, lo2, t);
    si = sv.site.sun(t);
    sv.player.yaw = yaw; sv.player.pitch = pitch;
    if (wantMat == SCENE_SHARD || wantMat == SCENE_HARBOUR || wantMat == SCENE_TERRACES || wantMat == SCENE_ROAD || wantMat == SCENE_GRAVES) {
        sv.player.x = 0;
        sv.player.z = 0;
        sv.player.y = sv.site.surfaceHeight(0, 0);
    }   // at the stand point itself (`init` puts the explorer 6, -4 from the origin)
    sv.relocateCapsule(-std::sin(yaw) * 14, -std::cos(yaw) * 14);   // the capsule behind the camera, out of the picture
    if (!shardLine.empty()) printf("%s\n", shardLine.c_str());
    if (roadStand) {   // the road under the camera and twenty metres ahead, against the ground as it is (a probe site has no roads)
        SurfaceSite probe; probe.init(&sys, bi, la2, lo2, t);
        for (int q = 0; q < 2; q++) {
            double px = std::sin(yaw) * 20 * q, pz = std::cos(yaw) * 20 * q, d, along, hd; const SiteRoad* rd;
            bool near = sv.site.roadAt(px, pz, 30, d, along, hd, rd);
            TerrainVertex v = sv.site.sampleAt(px, pz, 4), w = probe.sampleAt(px, pz, 4);
            printf("  %s: %zu roads at the site (%.1f ms); the nearest %s; the 4 m vertex: road %d, %s albedo %.2f veg %.2f (the ground's %s albedo %.2f veg %.2f)\n",
                    q ? "20 m ahead" : "under the camera", sv.site.roads.size(), sv.site.roadsMs,
                    near ? fmt("%.1f m off, left %.2f", d, roadLeft(rd->id, along, rd->wear)).c_str() : "none within 30 m", v.road, MATERIAL_NAMES[v.material], v.albedo, v.veg,
                    MATERIAL_NAMES[w.material], w.albedo, w.veg);
        }
    }
    int houses = 0, towers = 0; for (const Building& bd : bestSpec.buildings) { if (bd.kind == BK_HOUSE) houses++; if (bd.kind == BK_TOWER) towers++; }
    printf("  settlement: a %s of %zu buildings (%d houses, %d towers%s), plan %d, radius %.0f m, %.1f km from the type's spot; people %d of %d, culture style %d, family %d, tall %.2f, decay %.2f "
           "(the end %.0f years ago), %s roofs, buried %.1f m; the terrain: slope %.3f, the shore %s, relief %.2f%s%s\n",
            SETTLEMENT_CLASS_NAMES[bestSpec.sclass], bestSpec.buildings.size(), houses, towers, bestSpec.walled ? ", walled" : "", bestSpec.plan, bestSpec.size, bestD / 1000.0, bestSpec.people + 1,
            sv.peoples, sv.cultures[bestSpec.people].style, sv.cultures[bestSpec.people].family, sv.cultures[bestSpec.people].tall, sv.cultures[bestSpec.people].decay,
            sv.cultures[bestSpec.people].ageYears, sv.cultures[bestSpec.people].stoneRoofs ? "stone" : "no", sv.cultures[bestSpec.people].buried, bestSpec.site.slope,
            bestSpec.site.shoreDist < 1e8 ? fmt("%.0f m%s", bestSpec.site.shoreDist, bestSpec.site.shoreDry ? " (dry)" : "").c_str() : "none", bestSpec.site.relief,
            featureList(bestSpec).empty() ? "" : "; features: ", featureList(bestSpec).c_str());
    return true;
}

bool SceneFind::deadSea(double& t, SunInfo& si) {   // C-01: a dead desert world's old shore
    // 2048 m for the shore nearest the asked latitude, then 30 km round it at 512 m), standing 250 m up the land side (the beach
    // ridges), facing out over the bed
    const BodyGen& gg = sv.site.gen;
    double bestScore = -1e18, sLat = 0, sLon = 0; bool found = false;
    { DrainageOff off; for (int j = 0; j < 180; j++) for (int i = 0; i < 360; i++) {   // the steepest shore within 25 degrees of the asked latitude (a flat coast is a ramp)
        double la = (-89.5 + j) * DEG, lo = (-179.5 + i) * DEG;
        if (std::fabs(la - latUse) > 25 * DEG) continue;
        SurfaceSample ss = sampleSurface(gg, StarSystem::bodyFromLatLon(la, lo), 2048);
        if (ss.oldSea < 0.3 || ss.oldSea > 0.7) continue;
        double dA = 1500.0 / (gg.R * 1000.0), lo2 = dA / std::max(std::cos(la), 0.05);   // a shore that drops 20-120 m over 3 km (a scarp, not a canyon wall nor a ramp)
        double s4[4] = {sampleSurface(gg, StarSystem::bodyFromLatLon(la + dA, lo), 2048).height, sampleSurface(gg, StarSystem::bodyFromLatLon(la - dA, lo), 2048).height,
                        sampleSurface(gg, StarSystem::bodyFromLatLon(la, lo + lo2), 2048).height, sampleSurface(gg, StarSystem::bodyFromLatLon(la, lo - lo2), 2048).height};
        double drop = std::max(std::fabs(s4[0] - s4[1]), std::fabs(s4[2] - s4[3]));
        double score = -std::fabs(std::log(std::max(drop, 1.0) / 50.0)) - 0.004 * std::fabs(la - latUse) / DEG;
        if (score > bestScore) { bestScore = score; sLat = la; sLon = lo; found = true; }
    } }
    if (!found) { printf("  %s: no old shore near the asked latitude\n", b.name.c_str()); return false; }
    sv.init(&sys, bi, sLat, sLon, t);
    double bestD = 1e18, bx = 0, bz = 0; found = false;
    { DrainageOff off; for (int iz = -30; iz <= 30; iz++) for (int ix = -30; ix <= 30; ix++) {
        double px = ix * 1000.0, pz = iz * 1000.0;
        SurfaceSample ss = sampleSurface(gg, sv.site.unitAt(px, pz), 512);
        if (ss.oldSea < 0.35 || ss.oldSea > 0.65) continue;
        double d = px * px + pz * pz;
        if (d < bestD) { bestD = d; bx = px; bz = pz; found = true; }
    } }
    if (!found) { printf("  %s: no old shore within 30 km of the coarse one\n", b.name.c_str()); return false; }
    double gx, gz;
    { DrainageOff off;
      gx = sampleSurface(gg, sv.site.unitAt(bx + 400, bz), 512).oldSea - sampleSurface(gg, sv.site.unitAt(bx - 400, bz), 512).oldSea;
      gz = sampleSurface(gg, sv.site.unitAt(bx, bz + 400), 512).oldSea - sampleSurface(gg, sv.site.unitAt(bx, bz - 400), 512).oldSea; }
    double gl = std::sqrt(gx * gx + gz * gz); if (gl < 1e-9) { gx = 1; gz = 0; gl = 1; }
    gx /= gl; gz /= gl;
    double la2, lo2, up = 60;   // the lip of the shore: the last point up the land side where the bed's mask is under 0.1, within 400 m; then 120 m back from it
    { DrainageOff off; for (double q = 60; q <= 400; q += 20) { up = q; if (sampleSurface(gg, sv.site.unitAt(bx - gx * q, bz - gz * q), 64).oldSea < 0.1) break; } }
    up += 120;
    sv.site.latLonAt(bx - gx * up, bz - gz * up, la2, lo2);
    sv.init(&sys, bi, la2, lo2, t);
    { double err2 = 0; t = findTime(sv.site, alt, t0Use, &err2); sv.init(&sys, bi, la2, lo2, t); }   // the shore's own daylight (it may lie half a world from the type's spot)
    si = sv.site.sun(t);
    sv.player.yaw = std::atan2(gx, gz); sv.player.pitch = -0.1;
    sv.relocateCapsule(-gx * 14, -gz * 14);
    printf("  the ground at the camera %.0f m, 20 m ahead %.0f, 100 m ahead %.0f, 300 m ahead %.0f, 1 km ahead %.0f (eye %.1f m up)\n", sv.site.groundHeight(0, 0),
            sv.site.sampleAt(gx * 20, gz * 20, 16).h, sv.site.sampleAt(gx * 100, gz * 100, 16).h, sv.site.sampleAt(gx * 300, gz * 300, 16).h, sv.site.sampleAt(gx * 1000, gz * 1000, 64).h,
            sv.player.y - sv.site.groundHeight(0, 0));
    SurfaceSample here = sampleSurface(gg, sv.site.unitAt(0, 0), 16), sea = sampleSurface(gg, sv.site.unitAt(gx * 600, gz * 600), 16),
                  deep = sampleSurface(gg, sv.site.unitAt(gx * 3000, gz * 3000), 64);
    printf("  old shore at lat %.1f lon %.1f; the sea stood at %.0f m: here h %.0f m %s (sea %.2f), 600 m out h %.0f m %s (sea %.2f), 3 km out h %.0f m %s (sea %.2f)\n", sv.site.lat0 / DEG,
            sv.site.lon0 / DEG, gg.oldSeaM, here.height, MATERIAL_NAMES[here.material], here.oldSea, sea.height, MATERIAL_NAMES[sea.material], sea.oldSea, deep.height, MATERIAL_NAMES[deep.material],
            deep.oldSea);
    return true;
}

bool setupSceneForType(int type, double latDeg, double alt, double yawOff, double pitch, SurfaceView& sv, StarSystem& sys,
                              StarNeighborhood& nb, double& tOut, int wantMat) {
    if (wantMat == SCENE_PINNED_MOUNTAIN) return setupPinnedMountainScene(alt, yawOff, pitch, sv, sys, nb, tOut);   // B-313: the pinned mountain site of the O6 review
    if (wantMat <= SCENE_FRONT_FAR && wantMat >= SCENE_FRONT_BEHIND) return setupFrontScene(SCENE_FRONT_FAR - wantMat, pitch, sv, sys, nb, tOut);   // the fronts' scenes on the pinned plain
    if (wantMat <= SCENE_METEOR_SHOWER && wantMat >= SCENE_MOONLIT) return setupNightScene(SCENE_METEOR_SHOWER - wantMat, sv, sys, nb, tOut);
    if (wantMat <= SCENE_AGE_PREHISTORIC && wantMat >= SCENE_AGE_FAR_FUTURE) return setupAgeScene(SCENE_AGE_PREHISTORIC - wantMat, alt, pitch, sv, sys, nb, tOut);   // A-03   // W-04
    if (wantMat <= SCENE_ROGUE_LAMP && wantMat >= SCENE_DARK_OCEAN) return setupDarkScene(SCENE_ROGUE_LAMP - wantMat, pitch, sv, sys, nb, tOut);
    const int stagePasses = type >= 0 && KINDS[type].elsewhere ? 2 : 1;   // a kind made only where no system is: the real ones first (the wanderers), then staged
    for (int stagePass = 0; stagePass < stagePasses; stagePass++)
    for (int64_t x = 150; x < 320; x++)
        for (int64_t z = 20; z < 140; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            sys.generate(s);
            // an active star (S-02: the activity table, so every class that drives the nights counts)
            if (wantMat <= SCENE_AURORA && wantMat >= SCENE_AURORA_CURTAIN && starActivity(sys.star.cls) < 0.7) continue;
            if (wantMat == SCENE_NEUTRON_GLASS && sys.star.cls != STAR_NEUTRON) continue;   // a glassed world belongs to a neutron star
            if (wantMat == SCENE_PROTOSTAR && sys.star.cls != STAR_PROTOSTAR) continue;   // a world of a protostar
            if (wantMat == SCENE_WOLF_RAYET && sys.star.cls != STAR_WOLF_RAYET) continue;   // a world of a Wolf-Rayet star
            if (wantMat == SCENE_BLACK_HOLE && sys.star.cls != STAR_BLACK_HOLE) continue;   // a world of a black hole
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (type >= 0 && (stagePass == 0 ? b.type != type : !stageElsewhere(sys, bi, type))) continue;   // a kind made where no system is, staged (P-05: after the real ones)
                if (type < 0 && (!KINDS[b.type].landable || b.type == PT_COMET)) continue;   // a trait scene on any world
                double lon = 0.7 + bi * 0.3, latUse = latDeg * DEG;
                if (wantMat == SCENE_AURORA_ZENITH) latUse = SurfaceView::auroraOvalLat(b) * DEG;   // right under the auroral oval
                if (wantMat == SCENE_AURORA_CURTAIN) latUse = (SurfaceView::auroraOvalLat(b) - 1) * DEG;   // the main curtain a degree poleward: overhead and across the sky
                BodyGen g = BodyGen::make(b);
                if (wantMat == SCENE_GEYSERS && !g.hasTrait(TR_GEYSERS)) continue;
                if ((wantMat == SCENE_SINGING_DUNES && !g.hasTrait(TR_SINGING_DUNES)) || (wantMat == SCENE_SINGING_ICE && !g.hasTrait(TR_SINGING_ICE)) ||
                        (wantMat == SCENE_QUAKES && !g.hasTrait(TR_QUAKES)))
                    continue;
                if (wantMat <= SCENE_SINGING_DUNES && wantMat >= SCENE_QUAKES && KINDS[b.type].pressureLo < 0.3) continue;   // an air that carries the sound whole
                if (wantMat == SCENE_SINGING_DUNES && (b.type == PT_HYDROCARBON || hasOpaqueDeck(b.type))) continue;   // dunes no haze hides
                if ((wantMat == SCENE_TOWN || wantMat == SCENE_DEAD_SEA || wantMat == SCENE_SHARD || wantMat == SCENE_HARBOUR || wantMat == SCENE_TERRACES || wantMat == SCENE_ROAD ||
                            wantMat == SCENE_GRAVES || wantMat == SCENE_SECOND_PEOPLE) &&
                        !g.hasTrait(TR_CIVILISATION))
                    continue;   // a world that had a people; C-09; C-12; C-13
                if (wantMat == SCENE_SECOND_PEOPLE && peoplesOf(g) < 2) continue;   // a world of two peoples
                // the scans below sample a whole planet at 16 m: without the drainage (a tile per sample); the river and lake
                // finders (-21, -22) scan the flood's own cells instead (`drainageStats`) and turn it back on for the fine samples
                struct DrainOff { bool was; DrainOff() : was(drainageEnabled()) { setDrainageEnabled(false); } ~DrainOff() { setDrainageEnabled(was); } } drainOff;
                bool lookAtWater = false;
                double faceYawOut = 1e9, facePitchOut = 1e9;   // a scene finder may choose the view's direction
                double t0Use = 1000.0;   // the aurora scenes choose the night of the month with the strongest potential
                bool placed = false;     // the finder chose the spot (or the longitude matters): the light must come from the time alone
                SceneFind f{latDeg, alt, sv, sys, nb, wantMat, bi, b, g, lon, latUse, lookAtWater, faceYawOut, facePitchOut,
                            t0Use, placed};   // the finders below are its member functions
                if (wantMat == MAT_SAND) {
                    if (!f.sand()) continue;
                } else if (wantMat == SCENE_GEYSERS) {
                    if (!f.geysers()) continue;
                } else if (wantMat <= SCENE_SINGING_DUNES && wantMat >= SCENE_QUAKES) {
                    if (!f.groundSounds()) continue;
                } else if (wantMat <= SCENE_AURORA && wantMat >= SCENE_AURORA_CURTAIN) {
                    if (!f.aurora()) continue;
                } else if (wantMat == SCENE_RUST_VALLEY) {
                    if (!f.rustValley()) continue;
                } else if (wantMat == SCENE_ARCHEAN_FLATS) {
                    if (!f.archeanFlats()) continue;
                } else if (wantMat == SCENE_TUNDRA_ICE) {
                    if (!f.tundraIce()) continue;
                } else if (wantMat == SCENE_HOTHOUSE_SWAMP || wantMat == SCENE_STEPPE) {
                    if (!f.hothouseOrSteppe()) continue;
                } else if (wantMat == SCENE_EYEBALL_SHORE) {
                    if (!f.eyeballShore()) continue;
                } else if (wantMat == SCENE_TERMINATOR_RING) {
                    if (!f.terminatorRing()) continue;
                } else if (wantMat == SCENE_HYCEAN_SEA) {
                    if (!f.hyceanSea()) continue;
                } else if (wantMat == SCENE_AMMONIA_SHORE) {
                    if (!f.ammoniaShore()) continue;
                } else if (wantMat == SCENE_SNOWBALL_BELT) {
                    if (!f.snowballBelt()) continue;
                } else if (wantMat == SCENE_CHEMOTROPH_VENTS) {
                    if (!f.chemotrophVents()) continue;
                } else if (wantMat == SCENE_DEAD_FOREST) {
                    if (!f.deadForest()) continue;
                } else if (wantMat == SCENE_SCORCHED_SEABED) {
                    if (!f.scorchedSeabed()) continue;
                } else if (wantMat == SCENE_NITROGEN_CELLS) {
                    if (!f.nitrogenCells()) continue;
                } else if (wantMat == SCENE_CRYO_PLUME) {   // 25 km across the wind from a cryovolcanic world's plume, facing it
                    Vec3 vent; if (!plumeStand(sys, bi, vent, latUse, lon)) continue;
                    double vla, vlo; StarSystem::latLonFromBody(vent, vla, vlo);
                    faceYawOut = std::atan2(wrapAngle(vlo - lon) * std::cos(latUse), vla - latUse);
                    placed = true;
                } else if (wantMat == SCENE_SPIDERS) {   // 60 m south of a spider on a dry ice world's spring frost, facing it (the clock moved to its jet below)
                    Vec3 vent; if (!springStand(sys, bi, t0Use, vent, latUse, lon)) continue;
                    faceYawOut = 0; placed = true;
                } else if (wantMat == SCENE_WANDERER_SNOW) {
                    if (!f.wandererSnow()) continue;
                } else if (wantMat == SCENE_BRINE_FACULAE) {
                    if (!f.brineFaculae()) continue;
                } else if (wantMat == SCENE_BRINE_MOUNTAIN) {   // the brine dwarf's lone mountain from 25 km across its plain, facing it
                    const Vec3 mtn = loneMountainOf(g);
                    double mla, mlo; StarSystem::latLonFromBody(mtn, mla, mlo);
                    const double Rm = b.radiusKm * 1000.0;
                    latUse = mla - 25000.0 / Rm; lon = mlo; faceYawOut = 0; placed = true;
                } else if (wantMat == SCENE_FROST_DWARF) {
                    if (!f.frostDwarf()) continue;
                } else if (wantMat == SCENE_GROOVED_LANES) {
                    if (!f.groovedLanes()) continue;
                } else if (wantMat == SCENE_TWO_FACED_BOUNDARY) {
                    if (!f.twoFacedBoundary()) continue;
                } else if (wantMat == SCENE_TWO_FACED_RIDGE) {   // a two-faced moon's equatorial ridge from 25 km south of its crest where it stands highest over its foot on the bright ice
                    if (!f.twoFacedRidge()) continue;
                } else if (wantMat == SCENE_TIGER_STRIPES) {   // 1.5 km from the middle of a tiger-striped moon's second stripe, facing 4 km along it
                    Vec3 vent; if (!curtainStand(sys, bi, vent, latUse, lon)) continue;
                    double vla, vlo; StarSystem::latLonFromBody(vent, vla, vlo);
                    faceYawOut = std::atan2(wrapAngle(vlo - lon) * std::cos(latUse), vla - latUse);
                    placed = true;
                } else if (wantMat == SCENE_PATCHWORK_CLIFF) {   // the patchwork's great cliff from its corona's floor 25 km in from its foot, facing it
                    Vec3 foot, upDir; if (!greatCliffOf(g, foot, upDir)) continue;
                    const Vec3 st = normalize(foot - upDir * (25.0 / b.radiusKm));
                    StarSystem::latLonFromBody(st, latUse, lon);
                    double fla, flo; StarSystem::latLonFromBody(foot, fla, flo);
                    faceYawOut = std::atan2(wrapAngle(flo - lon) * std::cos(latUse), fla - latUse);
                    placed = true;
                } else if (wantMat == SCENE_SPONGE_PITS) {
                    if (!f.spongePits()) continue;
                } else if (wantMat <= SCENE_STAINED_CRACK && wantMat != SCENE_NEUTRON_GLASS && wantMat != SCENE_PROTOSTAR && wantMat != SCENE_WOLF_RAYET && wantMat != SCENE_BLACK_HOLE &&
                        wantMat != SCENE_TOWN && wantMat != SCENE_DEAD_SEA && wantMat != SCENE_SHARD && wantMat != SCENE_HARBOUR && wantMat != SCENE_TERRACES && wantMat != SCENE_ROAD &&
                        wantMat != SCENE_GRAVES && wantMat != SCENE_SECOND_PEOPLE) {   // -43 (the glassed world) has its own branch below; S-04/S-05/S-06: -44 to -46 take the type's default spot
                    if (!f.traitGround()) continue;
                } else if (wantMat == SCENE_NEUTRON_GLASS) {
                    if (!f.neutronGlass()) continue;
                } else if (wantMat <= SCENE_BIOME && wantMat > SCENE_RIVER) {
                    if (!f.biome()) continue;
                } else if (wantMat == SCENE_RIVER || wantMat == SCENE_LAKE) {
                    if (!f.inlandWater()) continue;
                } else if (wantMat == SCENE_HERD || wantMat == SCENE_HERD_NEAR || wantMat == SCENE_HERD_FAR || wantMat == SCENE_BUGGY_SEAT || wantMat == SCENE_BUGGY_PARKED) {
                    if (!f.herdOrBuggy()) continue;
                } else if (wantMat == SCENE_FOREST) {
                    if (!f.forest()) continue;
                } else if ((wantMat == SCENE_TOWN || wantMat == SCENE_SHARD || wantMat == SCENE_HARBOUR || wantMat == SCENE_TERRACES || wantMat == SCENE_ROAD || wantMat == SCENE_GRAVES ||
                                   wantMat == SCENE_SECOND_PEOPLE) &&
                        g.hasTrait(TR_CIVILISATION) && planPlacePoint(g, wantMat == SCENE_SECOND_PEOPLE ? 1 : 0, latUse, lon, wantMat == SCENE_HARBOUR)) {
                    // the settlements stand in the peoples' lands: from a town of the plan (a place by the water for a harbour)
                    placed = true;
                } else if (wantMat == SCENE_SECOND_PEOPLE) {   // deep on the second people's side of the divide, on land near the asked latitude; the nearest settlement of theirs from there
                    Vec3 u = peopleSidePoint(g, 1, latUse); StarSystem::latLonFromBody(u, latUse, lon); placed = true;
                } else if (b.type == PT_FELISIAN) {
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 30 && !found; la += 2)
                        for (int k = 0; k < 256 && !found; k++) {
                            double ln = k * TAU / 256;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material == MAT_FOREST && ss.height > 20) { lon = ln; latUse = la * DEG; found = true; placed = true; }
                        }
                }
                if (b.type == PT_MOLTEN) {
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 20 && !found; la += 0.02)
                        for (double ln = 0; ln < TAU && !found; ln += 0.0006) {
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material == MAT_LAVA && ss.glow > 0.9) { lon = ln + 400.0 / (b.radiusKm * 1000.0); latUse = la * DEG; found = true; placed = true; }
                        }
                }
                setDrainageEnabled(drainOff.was);   // the site samples the real ground
                sv.init(&sys, bi, latUse, lon, t0Use);
                double sunErr = 0, t = findTime(sv.site, alt, t0Use, &sunErr);
                if (sunErr > 1.0 * DEG && !placed) {
                    // a world locked to its star has one longitude with that light (the scan's first cratered world, the regress's
                    // cratered_noon, kept its night side): the longitude whose sun stands nearest the target, then the time there
                    double bestLon = lon;
                    for (int k = 0; k < 72; k++) {
                        double ln = k * TAU / 72; SurfaceSite probe; probe.init(&sys, bi, latUse, ln, t);
                        double e2 = std::fabs(effectiveSunAlt(probe, t) - alt);
                        if (e2 < sunErr) { sunErr = e2; bestLon = ln; }
                    }
                    if (bestLon != lon) { lon = bestLon; sv.init(&sys, bi, latUse, lon, t0Use); t = findTime(sv.site, alt, t0Use, &sunErr); }
                }
                if (sunErr > 3.0 * DEG) {   // no such light at this latitude in any season: the next body
                    printf("  %s: the light never stands at %.0f degrees at latitude %.0f (closest %.0f): skipped\n", b.name.c_str(), alt / DEG, latUse / DEG, effectiveSunAlt(sv.site, t) / DEG);
                    continue;
                }
                sv.init(&sys, bi, latUse, lon, t);
                SunInfo si = sv.site.sun(t);
                sv.player.yaw = faceYawOut < 1e8 ? faceYawOut : si.azimuth + yawOff;
                sv.player.pitch = facePitchOut < 1e8 ? facePitchOut : pitch;
                if (type >= PT_ARCHEAN) sv.relocateCapsule(sv.player.x - std::sin(sv.player.yaw) * 14, sv.player.z - std::cos(sv.player.yaw) * 14);   // the capsule behind the view
                printf("  site: %s of %s (sector %lld 0 %lld), lat %.2f lon %.2f, the light at %.1f degrees (%.0f asked)\n", b.name.c_str(), s.name.c_str(), (long long)x, (long long)z, latUse / DEG,
                        lon / DEG, effectiveSunAlt(sv.site, t) / DEG, alt / DEG);
                if (wantMat == SCENE_TOWN || wantMat == SCENE_SHARD || wantMat == SCENE_HARBOUR || wantMat == SCENE_TERRACES || wantMat == SCENE_ROAD || wantMat == SCENE_GRAVES ||
                        wantMat == SCENE_SECOND_PEOPLE) {   // the nearest settlement (a town before a village before a hamlet, within the first five rings of
                    if (!f.settlement(t, si)) continue;
                }
                if (wantMat == SCENE_DEAD_SEA) {   // the old shore of a dead desert world (its seas are the size of continents, so the whole world is scanned at
                    if (!f.deadSea(t, si)) continue;
                }
                if (wantMat == SCENE_BUGGY_SEAT || wantMat == SCENE_BUGGY_PARKED) {   // the buggy from the seat (-8) and parked, seen from 5 m (-9), by day on open ground
                    sv.relocateCapsule(sv.player.x + 4, sv.player.z + 2);
                    if (!sv.deployBuggy()) continue;
                    sv.buggy.unfold = 1;
                    sv.buggy.heading = 0.4;
                    if (wantMat == SCENE_BUGGY_SEAT) { sv.player.x = sv.buggy.x + 1.5; sv.player.z = sv.buggy.z; if (!sv.toggleBuggy()) continue; sv.buggy.steer = 12 * DEG; sv.player.pitch = -0.06;
                    } else {
                        sv.player.x = sv.buggy.x - 4.2;
                        sv.player.z = sv.buggy.z - 3.0;
                        double dx = sv.buggy.x - sv.player.x, dz = sv.buggy.z - sv.player.z;
                        sv.player.yaw = std::atan2(dx, dz);
                        sv.player.pitch = -0.12;
                        sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                    }
                }
                if (wantMat == SCENE_CREATURES) {   // one creature of each land species 5 m ahead, side on, standing still
                    if (sv.critters.empty()) continue;
                    std::vector<Critter> keep;
                    for (int spi = 0; spi < sv.bestiary.landCount; spi++) {
                        Critter c; c.species = spi; c.herd = 0; c.seed = 1234 + spi; c.state = CS_IDLE; c.timer = 1e9;
                        c.size = sv.bestiary.species[spi].size;
                        c.x = sv.player.x + (spi - (sv.bestiary.landCount - 1) * 0.5) * (2.5 + c.size * 2.5);
                        c.z = sv.player.z + 5 + c.size * 2.5;
                        c.heading = PI / 2;
                        keep.push_back(c);
                    }
                    sv.critters = keep;
                    sv.player.yaw = 0; sv.player.pitch = -0.08;
                    for (const Critter& c : sv.critters)
                        printf("  %s (%s, %.1f m) at (%.1f, %.1f), ground %.1f\n", sv.bestiary.species[c.species].name.c_str(), PLAN_NAMES[sv.bestiary.species[c.species].plan], c.size, c.x, c.z,
                                sv.site.groundHeight(c.x, c.z));
                }
                if (wantMat == SCENE_HERD || wantMat == SCENE_HERD_NEAR || wantMat == SCENE_HERD_FAR) {
                    // stand 18 m (M4-05), 6 m or 60 m (N3) south of the first herd, facing it
                    if (sv.critters.empty()) continue;
                    double d = wantMat == SCENE_HERD ? 18 : (wantMat == SCENE_HERD_NEAR ? 6 : 60);
                    if (!sv.testGotoHerd(d)) continue;
                    if (sv.site.sampleAt(sv.player.x, sv.player.z + d, 16).material == MAT_FOREST) continue;   // the herd must stand in the open
                    int n = 0; for (const Critter& c : sv.critters) if (c.herd == 0) n++;
                    const Species& sp = sv.bestiary.species[sv.critters[0].species];
                    printf("herd of %d %s (%s, %.1f m, %s, %s) at %.0f m\n", n, sp.name.c_str(), PLAN_NAMES[sp.plan], sp.size, ACTIVITY_NAMES[sp.activity], TEMPERAMENT_NAMES[sp.temperament], d);
                }
                if (lookAtWater) {
                    // face the direction with the most water within 150 m
                    double bestYaw = 0; int bestWater = -1;
                    for (int a = 0; a < 16; a++) {
                        double yaw = a * TAU / 16; int cnt = 0;
                        for (int d = 20; d <= 150; d += 10) if (sv.site.sampleAt(std::sin(yaw) * d, std::cos(yaw) * d, 16).material == MAT_WATER) cnt++;
                        if (cnt > bestWater) { bestWater = cnt; bestYaw = yaw; }
                    }
                    sv.player.yaw = bestYaw; sv.player.pitch = -0.02;
                }
                nb.update(sv.site.worldPos(t, 0, 0, 0));
                Input in;
                sv.update(0.016, in, t, false);
                if (wantMat == SCENE_FOREST) {   // the canopy the scene's check wants (three fifths of the forest ground within 200 m), or the next world: a temperate family covers a fifth
                    double cov = sv.testCanopyCoverage(200);
                    if (cov < 0.6) { printf("  %s: %.0f%% of the forest ground under canopy at its densest wood: the next world\n", b.name.c_str(), cov * 100); continue; }
                }
                if (type >= PT_ARCHEAN && type < PT_NITROGEN && sv.site.atmosphere && alt > 10 * DEG && sv.env.skyBrightness < 0.3) {
                    // the light asked is a dim companion's (a binary's): the next world (P-05: the far cold's skies are dark)
                    printf("  %s: the sky at %.2f under the light asked: the next world\n", b.name.c_str(), sv.env.skyBrightness); continue;
                }
                if (wantMat == SCENE_SPIDERS) {   // the clock moved to the moment the nearest spider's jet ahead bursts
                    std::vector<GeyserVent> vents; springJets(sv.site.gen, sv.site.unitAt(sv.player.x, sv.player.z), vents);
                    uint64_t bestId = 0; double bd = 1e18;
                    for (const GeyserVent& v : vents) {
                        double vx, vz;
                        sv.site.localAt(v.unit, vx, vz);
                        const double d = (vx - sv.player.x) * (vx - sv.player.x) + (vz - sv.player.z) * (vz - sv.player.z);
                        if (d < bd) {
                            bd = d;
                            bestId = v.id;
                        }
                    }
                    if (!bestId || !sv.springJetsNow(t)) { printf("  %s: no jet in the spring's daylight: the next world\n", b.name.c_str()); continue; }
                    const double tq0 = t;
                    for (double dtq = 0; dtq < 300 && SurfaceView::springJetStrength(bestId, t) < 0.9; dtq += 0.25) t = tq0 + dtq;
                    sv.update(0.016, in, t, false);
                    printf("  the nearest spider %.0f m off, its jet at t+%.0f s\n", std::sqrt(bd), t - tq0);
                }
                tOut = t;
                return true;
            }
        }
    return false;
}
