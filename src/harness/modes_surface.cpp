// E-03: the surface's modes: `surface`, `scene` (a scene of scenes_table.h), `capsule`, `spot`, `consistency`, `drive`, `bestiary`,
// `encounters`, `landmaps`, `stability`, `compare`, `sheet`, `gallery`, `reanchor`
#include "harness/harness.h"
#include "harness/scenes_table.h"

void renderSurface(int onlyType) {
    SpaceRenderer sr;
    StarNeighborhood nb;
    bool done[PT_COUNT] = {false};
    int remaining = 0;
    for (int i = 0; i < PT_COUNT; i++) if (KINDS[i].landable && (onlyType < 0 || i == onlyType)) remaining++; else done[i] = true;
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return remaining; })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && remaining; bi++) {
            const Body& b = sys.bodies[bi];
            if (done[b.type]) continue;
            done[b.type] = true; remaining--;
            std::string tn = KINDS[b.type].name;
            for (char& ch : tn) if (ch == ' ') ch = '_';
            struct Scene { const char* tag; double latDeg; double alt; double yawOff; double pitch; };
            Scene scenes[] = {{"noon", 12, 45 * DEG, 0.0, 0.0}, {"sunset", 12, 4 * DEG, 0.0, 0.05}, {"night", 12, -40 * DEG, 0.5, 0.15}, {"north", 82, 5 * DEG, 0.0, 0.0}};
            for (const Scene& sc : scenes) {
                SurfaceView sv;
                double lon = 0.7 + bi * 0.3;
                // choose a forested land site for felisian (full-detail check)
                double latUse = sc.latDeg * DEG;
                if (b.type == PT_FELISIAN) {
                    BodyGen g = BodyGen::make(b);
                    bool found = false;
                    for (double la = sc.latDeg; la < sc.latDeg + 30 && !found; la += 2)
                        for (int k = 0; k < 256 && !found; k++) {
                            double ln = k * TAU / 256;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material == MAT_FOREST && ss.height > 20) { lon = ln; latUse = la * DEG; found = true; }
                        }
                }
                if (b.type == PT_MOLTEN) {
                    // land 400 m from a lava lake
                    BodyGen g = BodyGen::make(b);
                    bool found = false;
                    for (double la = sc.latDeg; la < sc.latDeg + 20 && !found; la += 0.02)
                        for (double ln = 0; ln < TAU && !found; ln += 0.0006) {
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material == MAT_LAVA && ss.glow > 0.9) { lon = ln + 400.0 / (b.radiusKm * 1000.0); latUse = la * DEG; found = true; }
                        }
                }
                sv.init(&sys, bi, latUse, lon, 1000.0);
                double t = findTime(sv.site, sc.alt, 1000.0);
                sv.init(&sys, bi, latUse, lon, t);
                // look toward the sun's azimuth (plus offset)
                SunInfo si = sv.site.sun(t);
                sv.player.yaw = si.azimuth + sc.yawOff;
                sv.player.pitch = sc.pitch;
                nb.update(sv.site.worldPos(t, 0, 0, 0));
                Input in;
                sv.update(0.016, in, t, false);
                for (const Body& ob : sys.bodies)   // the sky's bodies' maps first (made on their thread at the binary's own pace, F-07)
                    if (ob.type != PT_COMPANION) while (!sr.mapIfReady(ob)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
                Framebuffer fb;
                double t0 = nowSec();
                sv.render(fb, t, nb.stars, sr, 1.0);
                double t1 = nowSec();
                fb.mush(2);
                std::string fn = "shots/tests/surf_" + tn + "_" + sc.tag + ".png";
                saveFB(fb, fn.c_str());
                printf("%-16s %-7s %s lat=%.0f sunAlt=%.1f az=%.0f time=%.2f render=%.1f ms  T=%.0fC P=%.2f g=%.1f cloud=%.2f rain=%.2f snow=%.2f sky=%.2f uw=%d rocks=%d/%d\n", KINDS[b.type].name,
                        sc.tag, b.name.c_str(), sc.latDeg, si.altitude / DEG, si.azimuth / DEG, si.dayFraction, (t1 - t0) * 1000, sv.env.temperatureC, sv.env.pressureAtm, sv.site.gravity,
                        sv.env.cloudCover, sv.env.rain, sv.env.snow, sv.env.skyBrightness, (int)sv.player.underwater, sv.lastRocksDrawn, sv.lastRocksCandidates);
            }
        }
    }
}

// N0-02 (B-202): the diagnosis sheet. For every landable type: the landing map around the sub-solar point,
// the globe patch at the same point seen from 3.5 radii, and the ground there at noon looking down 30 deg.
// Prints the average colours and their RGB distances (0-441); N1 has to bring them under the threshold.
void runConsistency() {
    SpaceRenderer sr; StarNeighborhood nb;
    TestGame game;
    game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/test_guide.txt";
    Input in;
    bool done[PT_COUNT] = {false};
    int remaining = 0;
    for (int i = 0; i < PT_COUNT; i++) if (KINDS[i].landable) remaining++; else done[i] = true;
    printf("%-14s %-13s %-13s %-13s %6s %6s %6s\n", "type", "map", "globe", "ground", "m-g", "g-s", "m-s");
    double worst = 0; int over = 0; const double THRESH = 60;
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return remaining; })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && remaining; bi++) {
            const Body& b = sys.bodies[bi];
            if (done[b.type]) continue;
            done[b.type] = true; remaining--;
            double t = 5000.0;
            Vec3 bp = sys.bodyPos(bi, t);
            Vec3 toStar = normalize(sys.star.pos - bp);
            Mat3 frame = sys.bodyFrame(bi, t);
            double lat, lon;
            StarSystem::latLonFromBody(frame * toStar, lat, lon);   // the sub-solar point: noon, sun overhead
            // the site: within 3 deg of that point, standing on the material that covers most of the globe patch
            const PlanetMap& pm = sr.mapFor(b);
            int want = MAT_ROCK;
            {
                int hist[MAT_COUNT] = {0};
                for (int yy = -12; yy < 12; yy++) for (int xx = -12; xx < 12; xx++) hist[pm.materialAt(lon + xx * TAU / PlanetMap::W, clampd(lat + yy * PI / PlanetMap::H, -PI / 2, PI / 2))]++;
                for (int mm = 1; mm < MAT_COUNT; mm++) if (hist[mm] > hist[want]) want = mm;
            }
            {
                BodyGen g0 = BodyGen::make(b);
                bool found = false;
                for (double r = 0; r < 3.0 * DEG && !found; r += 0.15 * DEG)
                    for (double a = 0; a < TAU && !found; a += 0.5) {
                        double la = lat + r * std::cos(a), lo = lon + r * std::sin(a);
                        SurfaceSample ss = sampleSurface(g0, StarSystem::bodyFromLatLon(la, lo), 16);
                        if (ss.material == want && (ss.water < -1e8 || ss.material == MAT_WATER)) { lat = la; lon = lo; found = true; }
                    }
            }
            // 1) the globe from 3.5 radii, straight above the point
            Vec3 shipPos = bp + toStar * (b.radiusKm * 3.5);
            Vec3 fwd = normalize(bp - shipPos);
            nb.update(shipPos);
            Framebuffer fbg; SpaceContext c;
            c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(fwd.y)); c.bankBodyA = bi;
            warmMaps(sr, sys, bi);
            sr.setupPalette(fbg, &sys, bi, -1, 1.0); sr.render(fbg, c); fbg.mush(2);
            std::vector<uint32_t> globeRGB(FBW * FBH); fbg.toRGB(globeRGB.data(), 1.0);
            int wantBank = SpaceRenderer::globeBank(2, sys.bodies[bi].type, want);
            // the patch around the site's own point on the disc
            Vec3 siteW = sys.surfacePointWorld(bi, t, lat, lon, 0);
            Vec3 sv3 = c.cam * (siteW - shipPos);
            int gx = (int)(sr.proj.cx + sr.proj.f * sv3.x / sv3.z), gy = (int)(sr.proj.cy - sr.proj.f * sv3.y / sv3.z);
            // 2) the ground at that point, noon, looking down 30 deg
            SurfaceView sv;
            sv.init(&sys, bi, lat, lon, t);
            sv.player.yaw = 0.7; sv.player.pitch = -30 * DEG;
            nb.update(sv.site.worldPos(t, 0, 0, 0));
            sv.update(0.016, in, t, false);
            Framebuffer fbs; sv.render(fbs, t, nb.stars, sr, 1.0); fbs.mush(2);
            std::vector<uint32_t> surfRGB(FBW * FBH); fbs.toRGB(surfRGB.data(), 1.0);
            // 3) the landing map of the same body, without the cloud overlay
            game.testParkAt(s, bi);
            int tx = (int)(wrap2pi(lon + PI) / TAU * PlanetMap::W), ty = (int)((0.5 - lat / PI) * PlanetMap::H);
            // averages and the sheet: 24 texels of map (x4), 24 px of globe (x4), 96 px of ground (x1), swatches
            const int T = 96, GAP = 4;
            const int SW = 4 * T + 3 * GAP + GAP + 3 * 24 + 2 * 2, SH = T;
            std::vector<uint32_t> sheet(SW * SH, rgb(0, 0, 0));
            auto put = [&](int x0, int y0, uint32_t c) { if (x0 >= 0 && y0 >= 0 && x0 < SW && y0 < SH) sheet[y0 * SW + x0] = c; };
            // averages over the wanted material only (map texels by material, globe pixels by bank), the sheet shows everything
            double avg[3][3] = {{0}};
            int nm = 0, ng = 0;
            for (int yy = 0; yy < 24; yy++)
                for (int xx = 0; xx < 24; xx++) {
                    int mtx = ((tx - 12 + xx) % PlanetMap::W + PlanetMap::W) % PlanetMap::W, mty = clampi(ty - 12 + yy, 0, PlanetMap::H - 1);
                    uint32_t cm = game.testLandingMapTexel(mtx, mty, false);
                    int gxx = gx - 12 + xx, gyy = gy - 12 + yy;
                    bool gin = gxx >= 0 && gyy >= 0 && gxx < FBW && gyy < FBH;
                    uint32_t cg = gin ? globeRGB[gyy * FBW + gxx] : 0;
                    if (pm.material[mty * PlanetMap::W + mtx] == want) { for (int k = 0; k < 3; k++) avg[0][k] += (cm >> (8 * k)) & 255; nm++; }
                    if (gin && bankOf(fbg.idx[gyy * FBW + gxx]) == wantBank) { for (int k = 0; k < 3; k++) avg[1][k] += (cg >> (8 * k)) & 255; ng++; }
                    for (int dy = 0; dy < 4; dy++) for (int dx = 0; dx < 4; dx++) { put(xx * 4 + dx, yy * 4 + dy, cm); put(T + GAP + xx * 4 + dx, yy * 4 + dy, cg); }
                }
            if (nm < 8 || ng < 8) {   // too little of the material in view: fall back to everything
                nm = ng = 0; for (int k = 0; k < 3; k++) avg[0][k] = avg[1][k] = 0;
                for (int yy = 0; yy < 24; yy++)
                    for (int xx = 0; xx < 24; xx++) {
                        uint32_t cm = game.testLandingMapTexel(tx - 12 + xx, ty - 12 + yy, false);
                        int gxx = gx - 12 + xx, gyy = gy - 12 + yy;
                        uint32_t cg = (gxx >= 0 && gyy >= 0 && gxx < FBW && gyy < FBH) ? globeRGB[gyy * FBW + gxx] : 0;
                        for (int k = 0; k < 3; k++) { avg[0][k] += (cm >> (8 * k)) & 255; avg[1][k] += (cg >> (8 * k)) & 255; }
                        nm++; ng++;
                    }
            }
            int sx0 = FBW / 2 - T / 2, sy0 = (int)(FBH * 0.72) - T / 2;
            double shadeSum = 0; int bankHist[BANKS] = {0};
            for (int yy = 0; yy < T; yy++)
                for (int xx = 0; xx < T; xx++) {
                    int px = clampi(sx0 + xx, 0, FBW - 1), py = clampi(sy0 + yy, 0, FBH - 1);
                    uint32_t cs = surfRGB[py * FBW + px];
                    for (int k = 0; k < 3; k++) avg[2][k] += (cs >> (8 * k)) & 255;
                    shadeSum += shadeOf(fbs.idx[py * FBW + px]); bankHist[bankOf(fbs.idx[py * FBW + px])]++;
                    put(2 * (T + GAP) + xx, yy, cs);
                }
            int groundBank = 0; for (int k = 1; k < BANKS; k++) if (bankHist[k] > bankHist[groundBank]) groundBank = k;
            for (int k = 0; k < 3; k++) { avg[0][k] /= nm; avg[1][k] /= ng; avg[2][k] /= (double)T * T; }
            auto dist = [&](int a, int b2) { double d = 0; for (int k = 0; k < 3; k++) d += (avg[a][k] - avg[b2][k]) * (avg[a][k] - avg[b2][k]); return std::sqrt(d); };
            for (int i = 0; i < 3; i++) {
                uint32_t cc = rgb((int)avg[i][0], (int)avg[i][1], (int)avg[i][2]);
                for (int yy = 0; yy < T; yy++) for (int xx = 0; xx < 24; xx++) put(3 * (T + GAP) + i * 26 + xx, yy, cc);
            }
            std::string tn = KINDS[b.type].name;
            for (char& ch : tn) if (ch == ' ') ch = '_';
            std::string fn = "shots/tests/consistency_" + tn + ".png";
            writePNG(fn.c_str(), sheet.data(), SW, SH);
            double dmg = dist(0, 1), dgs = dist(1, 2), dms = dist(0, 2);
            worst = std::max(worst, std::max(dmg, std::max(dgs, dms)));
            // the globe is its cloud deck; the map (C) and the ground are the same ground (P-01: the steam world's too)
            if (hasOpaqueDeck(b.type) && b.type != PT_HYDROCARBON) { if (dms > THRESH) over++; }
            else if (dmg > THRESH || dgs > THRESH || dms > THRESH) over++;
            // the landing map's relief class against the slope measured on the real terrain
            const char* cls = game.testReliefClass(lat, lon);
            double slope = 0;   // the regional grade of Game::siteSlope, on the ground's own caches
            for (int k = 0; k < 4; k++) {
                double dx = (k & 1) ? 0 : (k ? -1 : 1), dz = (k & 1) ? (k == 1 ? 1 : -1) : 0;
                slope += std::fabs(sv.site.groundHeight(dx * 150, dz * 150) - sv.site.groundHeight(0, 0)) / 150.0 / 8;
                slope += std::fabs(sv.site.groundHeight(dx * 600, dz * 600) - sv.site.groundHeight(0, 0)) / 600.0 / 8;
            }
            slope /= 0.64;
            // the map probes points of the function, the ground reads its caches: the classes agree, or the two grades are within 30% (a boundary case)
            double mapSlope = Game::siteSlope(sv.site);
            bool reliefOk = std::string(cls) == reliefClass(slope) || std::fabs(mapSlope - slope) <= 0.3 * std::max(mapSlope, slope);
            printf("%-14s %3.0f,%3.0f,%3.0f   %3.0f,%3.0f,%3.0f   %3.0f,%3.0f,%3.0f   %6.0f %6.0f %6.0f  %s (%s, sun %.0f deg, light %.2f (globe %.2f), %s slope %.3f%s, %s)\n", KINDS[b.type].name,
                    avg[0][0], avg[0][1], avg[0][2], avg[1][0], avg[1][1], avg[1][2], avg[2][0], avg[2][1], avg[2][2], dmg, dgs, dms, b.name.c_str(),
                    MATERIAL_NAMES[sv.site.sampleAt(0, 0, 64).material], sv.site.sun(t).altitude / DEG, sv.site.sun(t).lightFactor,
                    SpaceRenderer::lightFactor(sys.star.luminosity, length(sys.star.pos - bp)), cls, slope, reliefOk ? "" : " RELIEF MISMATCH", fn.c_str());
            printf("%-14s ground crop: mean shade %.1f, bank %d (%d%% of the crop); ramp stops 14/34/47 = %d,%d,%d / %d,%d,%d / %d,%d,%d\n", "", shadeSum / (T * T), groundBank,
                    bankHist[groundBank] * 100 / (T * T), fbs.pal[(groundBank * 64 + 14) * 3], fbs.pal[(groundBank * 64 + 14) * 3 + 1], fbs.pal[(groundBank * 64 + 14) * 3 + 2],
                    fbs.pal[(groundBank * 64 + 34) * 3], fbs.pal[(groundBank * 64 + 34) * 3 + 1], fbs.pal[(groundBank * 64 + 34) * 3 + 2], fbs.pal[(groundBank * 64 + 47) * 3],
                    fbs.pal[(groundBank * 64 + 47) * 3 + 1], fbs.pal[(groundBank * 64 + 47) * 3 + 2]);
            if (!reliefOk) over += 100;
        }
    }
    printf("consistency: %d of %d types over the threshold of %.0f (venusian: map against ground only); worst distance %.0f%s\n", over % 100, landableTypeCount(), THRESH, worst,
            over >= 100 ? "; RELIEF CLASS MISMATCH" : "");
}

// N4-02: a drive on open grassland through the surface renderer: 60 s at full throttle, frames from the seat, airborne and chase
int runDrive() {
    SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
    // a desert (no trees, no rocks on sand) first, else open grassland
    bool haveSite = setupSceneForType(PT_FELISIAN, 10, 40 * DEG, 0.6, 0.0, sv, sys, nb, t, SCENE_BIOME - BIO_DESERT);
    const char* where = "desert";
    if (!haveSite) { haveSite = setupSceneForType(PT_FELISIAN, 12, 40 * DEG, 0.6, 0.0, sv, sys, nb, t, SCENE_BUGGY_PARKED); where = "grassland"; }
    if (!haveSite) { printf("drive: no site\n"); return 1; }
    if (!sv.buggy.deployed) { sv.relocateCapsule(sv.player.x + 4, sv.player.z + 2); if (!sv.deployBuggy()) { printf("drive: could not deploy\n"); return 1; } sv.buggy.unfold = 1; }
    double x, z, heading;
    double run = sv.findOpenRun(x, z, heading);
    sv.buggy.x = x; sv.buggy.z = z; sv.buggy.y = sv.site.surfaceHeight(x, z); sv.buggy.heading = heading; sv.buggy.speed = 0; sv.buggy.unfold = 1;
    sv.player.x = x + 1.5; sv.player.z = z; sv.player.y = sv.buggy.y;
    if (!sv.inBuggy && !sv.toggleBuggy()) { printf("drive: could not get in\n"); return 1; }
    printf("drive: %s site on %s, open run of %.0f m from (%.0f, %.0f) heading %.0f deg\n", where, sv.site.sys->bodies[sv.site.body].name.c_str(), run, x, z, heading / DEG);
    Input in; in.down[KEY_W] = true;
    Framebuffer fb;
    double top = 0, worstMs = 0, sumMs = 0; int n = 0; bool airShot = false, stalled = false;
    double stuckT = 0, reverseT = 0; int steerDir = 1;
    double runX = x, runZ = z, runH = heading, runLen = run; int dir = 1;   // along the run (+1) or back (-1)
    double turnT = 0;
    std::vector<SurfaceView::Collider> cols;
    const int FRAMES = 30 * 90;
    for (int i = 0; i < FRAMES; i++) {
        double tt = t + i / 30.0;
        sv.update(1.0 / 30, in, tt, true);
        in.newFrame();
        in.down[KEY_W] = in.down[KEY_S] = in.down[KEY_A] = in.down[KEY_D] = in.down[KEY_SPACE] = false;
        double hx = std::sin(sv.buggy.heading), hz = std::cos(sv.buggy.heading);
        double along = (sv.buggy.x - runX) * std::sin(runH) + (sv.buggy.z - runZ) * std::cos(runH);   // position along the run
        auto aimAt = [&](double a) { double tx = runX + std::sin(runH) * a, tz = runZ + std::cos(runH) * a; return wrapAngle(std::atan2(tx - sv.buggy.x, tz - sv.buggy.z) - sv.buggy.heading); };
        double want = dir > 0 ? runH : runH + PI;
        double err = aimAt(clampd(along + dir * 40, 0, runLen));
        // U-turn near either end of the run: brake, then swing round until the heading points back
        if (turnT <= 0 && ((dir > 0 && along > runLen - 45) || (dir < 0 && along < 45))) { dir = -dir; turnT = 6.0; }
        if (turnT > 0) {
            turnT -= 1.0 / 30;
            want = dir > 0 ? runH : runH + PI; err = wrapAngle(want - sv.buggy.heading);
            if (std::fabs(sv.buggy.speed) > 6) in.down[KEY_SPACE] = true; else in.down[KEY_W] = true;
            in.down[err > 0 ? KEY_D : KEY_A] = true;
            if (std::fabs(err) < 15 * DEG) turnT = 0;
        } else {
            // a driver's eye: steer round rocks, trunks and water in the corridor ahead, back out when stuck, else hold the run
            double look = 14 + std::fabs(sv.buggy.speed) * 1.4;
            int steer = 0; double nearest = 1e9;
            for (double d = 8; d <= look; d += 12) {
                sv.collectColliders(sv.buggy.x + hx * d, sv.buggy.z + hz * d, cols);
                for (const SurfaceView::Collider& c : cols) {
                    if (c.kind == 0 && c.r < 0.72) continue;
                    double dx = c.x - sv.buggy.x, dz = c.z - sv.buggy.z;
                    double al = dx * hx + dz * hz, ac = -dx * hz + dz * hx;
                    if (al < 0 || al > look || std::fabs(ac) > 2.8 + c.r) continue;
                    if (al < nearest) { nearest = al; steer = ac > 0 ? -1 : 1; }
                }
            }
            for (double d = 10; d <= look && !steer; d += 5)
                if (sv.site.groundHeight(sv.buggy.x + hx * d, sv.buggy.z + hz * d) < sv.site.waterAt(sv.buggy.x + hx * d, sv.buggy.z + hz * d) - 0.3) steer = steerDir;
            if (std::fabs(sv.buggy.speed) < 0.5 && i > 30) stuckT += 1.0 / 30; else stuckT = 0;
            if (stuckT > 1.0 && reverseT <= 0) { reverseT = 1.5; steerDir = -steerDir; }
            if (reverseT > 0) { reverseT -= 1.0 / 30; in.down[KEY_S] = true; in.down[steerDir > 0 ? KEY_D : KEY_A] = true; }
            else {
                in.down[KEY_W] = true;
                if (steer > 0) in.down[KEY_D] = true; else if (steer < 0) in.down[KEY_A] = true;
                else if (std::fabs(err) > 4 * DEG) in.down[err > 0 ? KEY_D : KEY_A] = true;   // hold the run's line
                if (nearest < 12 && std::fabs(sv.buggy.speed) > 14) in.down[KEY_W] = false;   // ease off before something close
            }
        }
        top = std::max(top, std::fabs(sv.buggy.speed));
        if (i % 5 == 0 || i == 30 * 12 || i == 30 * 40) {
            double t0 = nowSec();
            sv.render(fb, tt, nb.stars, sr, 1.0);
            double ms = (nowSec() - t0) * 1000; sumMs += ms; worstMs = std::max(worstMs, ms); n++;
            if (i == 30 * 12) { fb.mush(2); sv.cameraFeed(fb, tt); saveFB(fb, "shots/tests/drive_seat.png"); }
            if (!airShot && sv.buggy.airborne) { fb.mush(2); sv.cameraFeed(fb, tt); saveFB(fb, "shots/tests/drive_air.png"); airShot = true; }
            if (i == 30 * 40) { sv.chaseCam = true; sv.render(fb, tt, nb.stars, sr, 1.0); fb.mush(2); saveFB(fb, "shots/tests/drive_chase.png"); sv.chaseCam = false; }
        }
        if (i % 300 == 299)
            printf("  %2d s: %5.1f m/s, %6.0f m, gear %d, skid %.2f, %.0f m along the run, dir %+d%s\n", (i + 1) / 30, sv.buggy.speed, sv.buggy.odometer, sv.buggy.gear, sv.buggy.skid, along, dir,
                    sv.buggy.airborne ? ", airborne" : "");
        if (i > 30 * 20 && std::fabs(sv.buggy.speed) < 0.5 && turnT <= 0) stalled = true;
    }
    {   // a night frame with the headlights on
        double tn = t + std::fabs(sys.bodies[sv.site.body].rotPeriod) * 0.5;
        Input in2; sv.update(1.0 / 30, in2, tn, true);
        sv.render(fb, tn, nb.stars, sr, 1.0); fb.mush(2); sv.cameraFeed(fb, tn); saveFB(fb, "shots/tests/drive_night.png");
        sv.chaseCam = true; sv.render(fb, tn, nb.stars, sr, 1.0); fb.mush(2); saveFB(fb, "shots/tests/drive_night_chase.png"); sv.chaseCam = false;
    }
    printf("drive: %.0f m in 90 s, top %.0f km/h, render %.1f ms mean / %.1f worst at 1x%s%s; frames drive_seat, drive_chase, drive_night[_chase]\n", sv.buggy.odometer, top * 3.6,
            sumMs / std::max(1, n), worstMs, airShot ? ", airborne once (shots/drive_air.png)" : "", stalled ? " (STALLED at some point)" : "");
    bool ok = top * 3.6 > 100 && sv.buggy.odometer > 2000;
    printf("drive: %s\n", ok ? "ok (over 2 km, over 100 km/h)" : "FAIL: under 2 km or under 100 km/h");
    return ok ? 0 : 1;
}

// N3-01: the bestiary of the first living worlds near the start, and a determinism check
int runBestiary() {
    int shown = 0, fails = 0;
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return shown < 6; })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && shown < 6; bi++) {
            const Body& b = sys.bodies[bi];
            if (b.type != PT_FELISIAN && b.type != PT_OCEAN) continue;
            BodyGen g = BodyGen::make(b);
            Bestiary B = Bestiary::make(b, g), B2 = Bestiary::make(b, g);
            printf("%s (%s): %d land, %d flyers, %d swimmers\n", b.name.c_str(), KINDS[b.type].name, B.landCount, B.flyerCount, B.swimmerCount);
            for (size_t i = 0; i < B.species.size(); i++) {
                const Species& sp = B.species[i];
                printf("  %-12s %-20s %4.1f m  gait %d/%d  bank %2d tone %.2f pattern %d crest %d  habitat %-16s diet %d  %-8s %-11s call %d x%.2f  herd %d-%d\n", sp.name.c_str(), PLAN_NAMES[sp.plan],
                        sp.size, sp.gait, sp.runGait, sp.bank, sp.tone, sp.pattern, sp.crest, sp.habitat ? BIOME_NAMES[sp.habitat] : "ANY", sp.diet, ACTIVITY_NAMES[sp.activity],
                        TEMPERAMENT_NAMES[sp.temperament], sp.call, sp.callPitch, sp.herdMin, sp.herdMax);
                if (sp.name != B2.species[i].name || sp.size != B2.species[i].size || sp.plan != B2.species[i].plan) fails++;
            }
            if (B.species.empty()) fails++;
            shown++;
        }
    }
    printf("bestiary: %d worlds, %s\n", shown, fails ? "NOT DETERMINISTIC (FAIL)" : "deterministic");
    return fails ? 1 : 0;
}

// N3-04: encounter density over random felisian sites: the share with a herd within 200 m, per biome
void runEncounters(int wanted) {
    setDrainageEnabled(false);   // hundreds of sites over the globe, a tile each; the herds do not need the rivers
    int total = 0, withLife = 0, cellHerds = 0, cellAnimals = 0;
    int perBiome[BIO_COUNT] = {0}, lifeBiome[BIO_COUNT] = {0};
    Rng r(0xE7C0);
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return total < wanted; })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && total < wanted; bi++) {
            const Body& b = sys.bodies[bi];
            if (b.type != PT_FELISIAN) continue;
            for (int k = 0; k < 6 && total < wanted; k++) {
                double lat = r.range(-70 * DEG, 70 * DEG), lon = r.range(-PI, PI);
                SurfaceSite site; site.init(&sys, bi, lat, lon, 5000.0);
                TerrainVertex tv = site.sampleAt(0, 0, 16);
                if (tv.material == MAT_WATER) continue;   // sea landings are not in the design
                Bestiary B = Bestiary::make(b, site.gen);
                std::vector<Critter> cr; std::vector<Flock> fl; std::vector<SurfaceView::Herd> hs;
                SurfaceView::planLife(site, B, site.gen.seed ^ (uint64_t)(lat * 1e6) ^ ((uint64_t)(lon * 1e6) << 20), cr, fl, hs);
                bool near = false;
                for (const Critter& c : cr) if (c.x * c.x + c.z * c.z < 200.0 * 200.0) near = true;
                total++; perBiome[tv.biome]++;
                if (near) { withLife++; lifeBiome[tv.biome]++; }
                {   // the habitat cells within 900 m of the landing
                    int gLat0, gLon0; SurfaceView::lifeCellAt(site, 0, 0, gLat0, gLon0);
                    for (int dl = -2; dl <= 2; dl++)
                        for (int dn = -3; dn <= 3; dn++) {
                            SurfaceView::Herd hd; int spi, n;
                            if (!SurfaceView::planCellHerd(site, B, gLat0 + dl, gLon0 + dn, hd, spi, n)) continue;
                            if (hd.cx * hd.cx + hd.cz * hd.cz > 900.0 * 900.0) continue;
                            cellHerds++; cellAnimals += n;
                        }
                }
            }
        }
    }
    printf("encounters: a herd within 200 m at %d of %d felisian land sites (%.0f%%); B-316: %.1f habitat herds (%.0f animals) within 900 m per site\n", withLife, total,
            total ? 100.0 * withLife / total : 0.0, total ? (double)cellHerds / total : 0.0, total ? (double)cellAnimals / total : 0.0);
    for (int bm = 0; bm < BIO_COUNT; bm++) if (perBiome[bm]) printf("  %-16s %3d sites, %3.0f%%\n", BIOME_NAMES[bm], perBiome[bm], 100.0 * lifeBiome[bm] / perBiome[bm]);
}

// One landing-map screen per landable planet type (bug B-004 check).
void renderLandingMaps() {
    TestGame game;
    game.savePrefix = "shots/tests/test_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.guidePath = "shots/tests/test_guide.txt";   // every harness Game keeps off the real guide.txt (2026-09-27)
    applyTestScale(game);
    Input in;
    bool done[PT_COUNT] = {false};
    int remaining = 0;
    for (int i = 0; i < PT_COUNT; i++) if (KINDS[i].landable) remaining++; else done[i] = true;
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return remaining; })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && remaining; bi++) {
            const Body& b = sys.bodies[bi];
            if (done[b.type]) continue;
            done[b.type] = true; remaining--;
            game.testParkAt(s, bi);
            for (int f = 0; f < 3; f++) { game.frame(in, 1.0 / 30); in.newFrame(); }
            std::string tn = KINDS[b.type].name;
            for (char& ch : tn) if (ch == ' ') ch = '_';
            std::string fn = "shots/tests/landmap_" + tn + ".png";
            writePNG(fn.c_str(), game.output(), FBW, FBH);
            printf("landing map %-16s %s state=%d, darkest shadow on the map now %.2f\n", KINDS[b.type].name, b.name.c_str(), (int)game.state, game.testMapShadowMax());
        }
    }
}

static void saveRaw(const char* path, const uint32_t* px, int w, int h) {
    std::ofstream f(path, std::ios::binary);
    f.write((const char*)&w, 4); f.write((const char*)&h, 4);
    f.write((const char*)px, (size_t)w * h * 4);
}
static bool loadRaw(const char* path, std::vector<uint32_t>& px, int& w, int& h) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.read((char*)&w, 4); f.read((char*)&h, 4);
    px.resize((size_t)w * h);
    f.read((char*)px.data(), (size_t)w * h * 4);
    return (bool)f;
}


// B-309: temporal stability of a scene: eight frames a thirtieth of a second apart, the mean absolute change of the
// luminance between consecutive frames, per region (a box of 40 logical pixels round the sun, the sky, the ground),
// and a heat map of where the picture moves (`shots/tests/stability_<scene>.png`, the last frame beside it). What
// should move: water, leaves, weather, creatures, the granulation of the sun. What should not: the sun's glow.
// `stability <scene> <metres per frame>` walks the camera forward instead: the far-ground band (rows 42-62%
// of the frame, ground beyond a few tens of metres) reports the mean change and the share of its pixels that jump by
// more than 40 levels in a frame, which is what a terrain ring popping cell by cell does and parallax does not; the
// sheet is `stability_<scene>_walk.png`
int runStability(const char* want, double walk) {
    SpaceRenderer sr; StarNeighborhood nb;
    int shown = 0;
    for (const CmpScene& sc : CMP_SCENES) {
        if (want && std::string(want) != sc.name) continue;
        StarSystem sys; SurfaceView sv; double t = 0;
        if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { printf("%s: no body\n", sc.name); continue; }
        const int N = walk > 0 ? 16 : 8;
        if (walk > 0) { sv.flocks.clear(); sv.critters.clear(); }   // the skyline count below must not see birds
        // the walking camera is the free camera 1.7 m over the ground, without the explorer's bob; the explorer walks along
        // under it so the rings follow
        if (walk > 0) { sv.enterFreeCam(); sv.freePos = Vec3(sv.player.x, sv.player.y + 1.7, sv.player.z); sv.freeYaw = sv.player.yaw; sv.freePitch = sv.player.pitch; }
        Framebuffer fb; Input in;
        std::vector<uint32_t> px((size_t)FBW * FBH), prev;
        std::vector<double> heat((size_t)FBW * FBH, 0);
        std::vector<int> pops((size_t)FBW * FBH, 0);
        double sunSx = -1, sunSy = -1;
        long flips = 0;   // pixels that swap between the sky bank and anything else from one frame to the next (a far tree flickering against the sky)
        std::vector<uint8_t> banks((size_t)FBW * FBH), prevBanks;
        auto lum = [](uint32_t c) { return 0.3 * (c & 255) + 0.59 * ((c >> 8) & 255) + 0.11 * ((c >> 16) & 255); };
        for (int k = 0; k < N; k++) {
            double tt = t + k / 30.0;
            if (walk > 0 && k > 0) {
                sv.player.x += std::sin(sv.player.yaw) * walk; sv.player.z += std::cos(sv.player.yaw) * walk;
                sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                sv.freePos = Vec3(sv.player.x, sv.player.y + 1.7, sv.player.z);
            }
            sv.update(1.0 / 30, in, tt, false);
            sv.render(fb, tt, nb.stars, sr, 1.0); fb.mush(2); fb.toRGB(px.data(), 1.0);
            if (k == 0) {
                const Vec3& sd = sv.env.sunDisc.dirLocal;
                if (!sv.projectPoint(sv.player.x + sd.x * 1e5, sv.player.y + 1.65 + sd.y * 1e5, sv.player.z + sd.z * 1e5, sunSx, sunSy)) sunSx = -1;
            }
            if (!prev.empty()) for (size_t i = 0; i < px.size(); i++) { double d = std::fabs(lum(px[i]) - lum(prev[i])); heat[i] += d; if (d > 40) pops[i]++; }
            for (size_t i = 0; i < banks.size(); i++) banks[i] = (uint8_t)(bankOf(fb.idx[i]) == 1 && fb.invz[i] == 0 ? 1 : 0);
            if (!prevBanks.empty()) for (size_t i = 0; i < banks.size(); i++) if (banks[i] != prevBanks[i]) flips++;
            prevBanks = banks;
            prev = px;
        }
        double sunSum = 0, skySum = 0, gndSum = 0, farSum = 0; int sunN = 0, skyN = 0, gndN = 0, farN = 0, farPops = 0;
        double box = 20.0 * FB_SCALE;
        for (int y = 0; y < FBH; y++)
            for (int x = 0; x < FBW; x++) {
                double h = heat[(size_t)y * FBW + x] / (N - 1);
                if (sunSx >= 0 && std::fabs(x - sunSx) < box && std::fabs(y - sunSy) < box) { sunSum += h; sunN++; }
                if (y < FBH * 0.45) { skySum += h; skyN++; } else if (y > FBH * 0.55) { gndSum += h; gndN++; }
                if (y >= FBH * 0.42 && y < FBH * 0.62) { farSum += h; farN++; farPops += pops[(size_t)y * FBW + x]; }
            }
        double mx = 0; for (double h : heat) mx = std::max(mx, h);
        std::vector<uint32_t> sheet((size_t)FBW * 2 * FBH);
        for (int y = 0; y < FBH; y++)
            for (int x = 0; x < FBW; x++) {
                sheet[(size_t)y * FBW * 2 + x] = prev[(size_t)y * FBW + x];
                int v = (int)std::min(255.0, heat[(size_t)y * FBW + x] / (N - 1) * 8.0);
                sheet[(size_t)y * FBW * 2 + FBW + x] = 0xFF000000u | ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v;
            }
        std::string fn = std::string("shots/tests/stability_") + sc.name + (walk > 0 ? "_walk" : "") + ".png";
        writePNG(fn.c_str(), sheet.data(), FBW * 2, FBH);
        if (walk > 0)
            printf("%-18s walking %.1f m/frame: far band mean %.2f, %.2f%% of its pixels jump over 40 per frame; skyline flips %.0f px/frame; sky %.2f ground %.2f (peak %.0f) -> %s\n", sc.name, walk,
                    farN ? farSum / farN : 0.0, farN ? 100.0 * farPops / ((double)farN * (N - 1)) : 0.0, (double)flips / (N - 1) / (FB_SCALE * FB_SCALE), skyN ? skySum / skyN : 0.0,
                    gndN ? gndSum / gndN : 0.0, mx / (N - 1), fn.c_str());
        else
            printf("%-18s sun %s%.2f  sky %.2f  ground %.2f  (mean |dL| per frame, 0..255; peak %.0f) -> %s\n", sc.name,
                    sunSx >= 0 ? (sunSx >= 0 && sunSx < FBW && sunSy >= 0 && sunSy < FBH ? "in frame " : "off frame ") : "below ", sunN ? sunSum / sunN : 0.0, skyN ? skySum / skyN : 0.0,
                    gndN ? gndSum / gndN : 0.0, mx / (N - 1), fn.c_str());
        shown++;
    }
    return shown ? 0 : 1;
}


// Renders the fixed scenes at every scale with that scale's kernel: shots/scale/<scene>_<s>x.png (+ .raw).
// B-310: the resolution sheets. Each configuration is a render scale and a mush box; every scene is rendered with each, then
// `sheet` shows them as the 1280 x 800 window would (the integer scales nearest, 3x through the window's bilinear pass,
// the scanlines on), cropped to the same window region, so the eye compares like with like.
struct ScaleCfg { int scale, kernel; const char* tag; const char* label; };
static const ScaleCfg SCALE_CFGS[] = {
    {2, 3, "2x_k3", "A  2X 640X400, 3X3 MUSH (THE OLD DEFAULT)"},
    {3, 3, "3x_k3", "B  3X 960X600, 3X3 MUSH"},
    {3, 4, "3x_k4", "C  3X 960X600, 4X4 MUSH"},
    {4, 3, "4x_k3", "D  4X 1280X800, 3X3 MUSH"},
    {4, 4, "4x_k4", "E  4X 1280X800, 4X4 MUSH"},
    {4, 5, "4x_k5", "F  4X 1280X800, 5X5 MUSH (CLASSIC)"},
};
static const char* const SHEET_SCENES[] = {"felisian_forest", "felisian_shore", "felisian_sunset", "cratered_noon", "thinatmo_sunset", "icy_noon", "buggy_seat", "orbit"};

void renderCompare() {
    mkdir("shots/tests/scale", 0755);
    for (const ScaleCfg& cfg : SCALE_CFGS) {
        setFramebufferScale(cfg.scale);
        g_mushKernel = cfg.kernel;
        SpaceRenderer sr;
        StarNeighborhood nb;
        std::vector<uint32_t> rgbBuf((size_t)FBW * FBH);
        for (const char* name : SHEET_SCENES) {
            if (std::string(name) == "orbit") continue;
            for (const CmpScene& sc : CMP_SCENES) {
                if (std::string(name) != sc.name) continue;
                StarSystem sys;
                SurfaceView sv;
                double t = 0;
                if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { printf("scene %s: no body found\n", sc.name); continue; }
                Framebuffer fb;
                double t0 = nowSec();
                sv.render(fb, t, nb.stars, sr, 1.0);
                fb.mush(2);
                if (sv.inBuggy && !sv.chaseCam) sv.cameraFeed(fb, t);
                double ms = (nowSec() - t0) * 1000;
                fb.toRGB(rgbBuf.data(), 1.0);
                std::string base = std::string("shots/tests/scale/") + sc.name + "_" + cfg.tag;
                saveRaw((base + ".raw").c_str(), rgbBuf.data(), FBW, FBH);
                writePNG((base + ".png").c_str(), rgbBuf.data(), FBW, FBH);
                printf("%-16s %s %dx%d kernel %d  %.1f ms\n", sc.name, cfg.tag, FBW, FBH, g_mushKernel, ms);
            }
        }
        // the felisian globe from orbit
        bool done = false;
        for (auto& [x, z, st, sys] : scanSystems(150, 320, 20, 140, [&] { return !done; })) {
            for (int bi = 0; bi < (int)sys.bodies.size() && !done; bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != PT_FELISIAN || b.parent >= 0) continue;
                double t = 5000.0;
                Vec3 bp = sys.bodyPos(bi, t);
                Vec3 toStar = normalize(sys.star.pos - bp);
                Vec3 side = normalize(cross(toStar, Vec3(0, 1, 0)));
                Vec3 shipPos = bp + normalize(toStar * 0.6 + side * 0.8 + Vec3(0, 0.25, 0)) * (b.radiusKm * 3.5);
                Vec3 fwd = normalize(bp - shipPos);
                nb.update(shipPos);
                Framebuffer fb;
                SpaceContext c;
                c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos;
                c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(fwd.y));
                c.bankBodyA = bi;
                sr.setupPalette(fb, &sys, bi, -1, 1.0);
                double t0 = nowSec();
                sr.render(fb, c);
                fb.mush(2);
                double ms = (nowSec() - t0) * 1000;
                fb.toRGB(rgbBuf.data(), 1.0);
                std::string base = std::string("shots/tests/scale/orbit_") + cfg.tag;
                saveRaw((base + ".raw").c_str(), rgbBuf.data(), FBW, FBH);
                writePNG((base + ".png").c_str(), rgbBuf.data(), FBW, FBH);
                printf("%-16s %s %dx%d kernel %d  %.1f ms\n", "orbit", cfg.tag, FBW, FBH, g_mushKernel, ms);
                done = true;
            }
        }
    }
    setFramebufferScale(1);
}

// the frame as the 1280 x 800 window shows it: nearest for integer factors, bilinear for 3x, the scanlines every four rows
static void windowImage(const std::vector<uint32_t>& px, int w, int h, std::vector<uint32_t>& out) {
    const int W = 1280, H = 800;
    out.assign((size_t)W * H, 0xFF000000u);
    double fx = (double)w / W, fy = (double)h / H;
    bool integer = (W % w) == 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            uint32_t c;
            if (integer) c = px[(size_t)(y * h / H) * w + x * w / W];
            else {
                double sx = (x + 0.5) * fx - 0.5, sy = (y + 0.5) * fy - 0.5;
                int x0 = clampi((int)std::floor(sx), 0, w - 1), y0 = clampi((int)std::floor(sy), 0, h - 1), x1 = std::min(x0 + 1, w - 1), y1 = std::min(y0 + 1, h - 1);
                double tx = clampd(sx - x0, 0, 1), ty = clampd(sy - y0, 0, 1);
                uint32_t p00 = px[(size_t)y0 * w + x0], p10 = px[(size_t)y0 * w + x1], p01 = px[(size_t)y1 * w + x0], p11 = px[(size_t)y1 * w + x1];
                int ch[3];
                for (int k = 0; k < 3; k++) {
                    double a = ((p00 >> (8 * k)) & 255) * (1 - tx) + ((p10 >> (8 * k)) & 255) * tx, b = ((p01 >> (8 * k)) & 255) * (1 - tx) + ((p11 >> (8 * k)) & 255) * tx;
                    ch[k] = (int)(a * (1 - ty) + b * ty + 0.5);
                }
                c = 0xFF000000u | ((uint32_t)ch[2] << 16) | ((uint32_t)ch[1] << 8) | (uint32_t)ch[0];
            }
            if ((y & 3) == 3) {   // the scanline: black at alpha 70/255
                uint32_t r = (c & 255) * 185 / 255, g = ((c >> 8) & 255) * 185 / 255, b = ((c >> 16) & 255) * 185 / 255;
                c = 0xFF000000u | (b << 16) | (g << 8) | r;
            }
            out[(size_t)y * W + x] = c;
        }
}

// Composes shots/tests/scale_<scene>.png: the six configurations as the window shows them, the same 640 x 400 region of the
// window (the lower middle: ground, horizon, the sun) in a 2 x 3 grid; the whole window images go to shots/tests/scale/.
void composeSheets() {
    const int TW = 640, TH = 400, COLS = 2, ROWS = 3, SW = TW * COLS, SH = TH * ROWS;
    const int CX = 320, CY = 300;   // the crop's origin in the window
    for (const char* scene : SHEET_SCENES) {
        std::vector<uint32_t> sheet((size_t)SW * SH, rgb(20, 20, 20));
        RGBCanvas cv{sheet.data(), SW, SH, 1};
        bool any = false;
        int i = 0;
        for (const ScaleCfg& cfg : SCALE_CFGS) {
            int slot = i++;
            std::vector<uint32_t> px; int w = 0, h = 0;
            std::string fn = std::string("shots/tests/scale/") + scene + "_" + cfg.tag + ".raw";
            if (!loadRaw(fn.c_str(), px, w, h)) continue;
            any = true;
            std::vector<uint32_t> win;
            windowImage(px, w, h, win);
            writePNG((std::string("shots/tests/scale/") + scene + "_" + cfg.tag + "_window.png").c_str(), win.data(), 1280, 800);
            int ox = (slot % COLS) * TW, oy = (slot / COLS) * TH;
            for (int y = 0; y < TH; y++)
                for (int x = 0; x < TW; x++) sheet[(size_t)(oy + y) * SW + ox + x] = win[(size_t)(CY + y) * 1280 + CX + x];
            fillRectRGB(cv, ox, oy, ox + textWidth(cfg.label, 2) + 8, oy + 18, rgb(0, 0, 0));
            drawText(cv, ox + 4, oy + 3, cfg.label, rgb(255, 220, 120), 2);
        }
        if (!any) continue;
        std::string out = std::string("shots/tests/scale_") + scene + ".png";
        writePNG(out.c_str(), sheet.data(), SW, SH);
        printf("wrote %s\n", out.c_str());
    }
}

void renderGallery(int type) {
    SpaceRenderer sr;
    StarNeighborhood nb;
    const int TW = 320, TH = 200;
    std::vector<uint32_t> sheet((size_t)TW * 4 * TH * 4, rgb(10, 10, 10));
    RGBCanvas cv{sheet.data(), TW * 4, TH * 4, 1};
    int n = 0;
    Rng r(777 + type);
    setFramebufferScale(1);
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return n < 16; })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && n < 16; bi++) {
            const Body& b = sys.bodies[bi];
            if (b.type != type || !KINDS[b.type].landable) continue;
            if (r.chance(0.5)) continue;
            double lat = std::asin(r.sym(0.9)), lon = r.range(-PI, PI);
            SurfaceView sv;
            sv.init(&sys, bi, lat, lon, 1000.0);
            double t = findTime(sv.site, 35 * DEG, 1000.0);
            sv.init(&sys, bi, lat, lon, t);
            sv.player.yaw = r.range(0, TAU); sv.player.pitch = 0.02;
            nb.update(sv.site.worldPos(t, 0, 0, 0));
            Input in;
            sv.update(0.016, in, t, false);
            Framebuffer fb;
            sv.render(fb, t, nb.stars, sr, 1.0);
            fb.mush(2);
            std::vector<uint32_t> px((size_t)FBW * FBH);
            fb.toRGB(px.data(), 1.0);
            int ox = (n % 4) * TW, oy = (n / 4) * TH;
            for (int y = 0; y < TH; y++) for (int xx = 0; xx < TW; xx++) sheet[(size_t)(oy + y) * TW * 4 + ox + xx] = px[(size_t)y * FBW + xx];
            std::string label = b.name + "  " + std::to_string((int)(lat / DEG)) + "/" + std::to_string((int)(lon / DEG)) + "  " + MATERIAL_NAMES[sv.site.sampleAt(0, 0, 16).material];
            for (char& c : label) c = (char)toupper((unsigned char)c);
            fillRectRGB(cv, ox, oy, ox + textWidth(label.c_str()) + 4, oy + 9, rgb(0, 0, 0));
            drawText(cv, ox + 2, oy + 1, label.c_str(), rgb(255, 220, 120));
            n++;
        }
    }
    std::string tn = KINDS[type].name;
    for (char& ch : tn) if (ch == ' ') ch = '_';
    std::string fn = "shots/tests/gallery_" + tn + ".png";
    writePNG(fn.c_str(), sheet.data(), TW * 4, TH * 4);
    printf("gallery: %d sites of %s -> %s\n", n, KINDS[type].name, fn.c_str());
}

// Re-anchoring check (KI-007): walk 51 km, the frame moves, positions and headings stay consistent.
void testReanchor() {
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140)) {
        for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
            const Body& b = sys.bodies[bi];
            if (b.type != PT_ROCKY) continue;
            SurfaceView sv;
            double t = 1000.0;
            sv.init(&sys, bi, 62 * DEG, 0.4, t);   // high latitude: meridians converge noticeably
            sv.relocateCapsule(120, -80);
            sv.player.x = 51e3; sv.player.z = 3e3; sv.player.yaw = 0.7;
            Input in;
            // "before": measured straight from the old frame (update() would already re-anchor)
            auto measure = [&](double& lat, double& lon, double& cap, double& bear, Vec3& fwd) {
                sv.site.latLonAt(sv.player.x, sv.player.z, lat, lon); lat /= DEG; lon /= DEG;
                double dx = sv.capsuleX - sv.player.x, dz = sv.capsuleZ - sv.player.z;
                cap = std::sqrt(dx * dx + dz * dz); bear = wrap2pi(std::atan2(dx, dz)) / DEG;
                fwd = sv.site.unitAt(sv.player.x + 1000 * std::sin(sv.player.yaw), sv.player.z + 1000 * std::cos(sv.player.yaw));
            };
            double latB, lonB, capB, bearB, yawB = sv.player.yaw; Vec3 fwdB;
            measure(latB, lonB, capB, bearB, fwdB);
            sv.update(0.016, in, t, false);   // re-anchors (distance > 50 km)
            double latA, lonA, capA, bearA, yawA = sv.player.yaw; Vec3 fwdA;
            measure(latA, lonA, capA, bearA, fwdA);
            double fwdErrM = length(fwdA - fwdB) * b.radiusKm * 1000;
            printf("reanchor on %s: site (%.2f,%.2f) -> (%.4f,%.4f), player at (%.0f,%.0f) m; lat/lon %.4f/%.4f -> %.4f/%.4f; capsule %.0f m %.1f deg -> %.0f m %.1f deg; yaw %.3f -> %.3f; forward "
                   "point moved %.1f m\n",
                    b.name.c_str(), 62 * DEG, 0.4, sv.site.lat0, sv.site.lon0, sv.player.x, sv.player.z, latB, lonB, latA, lonA, capB, bearB, capA, bearA, yawB, yawA, fwdErrM);
            return;
        }
    }
}

// F-01: `reanchor comet [scale]`: the comet's re-anchor (every 0.35 radii, KI-302) on the `comet_day` scene: the update that re-anchors and
// the next frames' refill timed (with VESPERIS_TRACE the parts: the site's init, the roads, the tiles asked, each ring's misses and fill)
int runReanchorComet(int argc, char** argv) {
    setFramebufferScale(argc > 3 ? atoi(argv[3]) : 4);
    if (const CmpScene* found = findScene("comet_day")) {
        const CmpScene& sc = *found;
        SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
        if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { printf("no comet\n"); return 1; }
        Framebuffer fb; Input in;
        for (int i = 0; i < 3; i++) sv.render(fb, t, nb.stars, sr, 1.0);
        const double reach = std::min(50e3, 0.35 * sv.site.R);
        printf("reanchor comet: %s, R %.1f km, the frame re-anchored beyond %.1f km at %dx\n", sys.bodies[sv.site.body].name.c_str(), sv.site.R / 1000, reach / 1000, FB_SCALE);
        for (int k = 0; k < 2; k++) {
            sv.player.x = reach * 1.02; sv.player.z = 0; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
            const double t0 = nowSec(); sv.update(1.0 / 60, in, t, false); const double upd = (nowSec() - t0) * 1000;
            double ms[3], firstMs[3] = {0, 0, 0};
            for (int i = 0; i < 3; i++) {
                const double t1 = nowSec(); sv.render(fb, t, nb.stars, sr, 1.0); ms[i] = (nowSec() - t1) * 1000;
                if (i == 0) for (int j = 0; j < 3; j++) firstMs[j] = sv.lastRenderMs[j];
            }
            printf("  re-anchor %d: the update %.1f ms, then frames %.1f, %.1f, %.1f ms (the first: palette+sky %.1f, warm %.1f, terrain %.1f)\n", k + 1, upd, ms[0], ms[1], ms[2], firstMs[0],
                    firstMs[1], firstMs[2]);
        }
        return 0;
    }
    return 1;
}

int runScene(int argc, char** argv) {
    std::string want = argc > 2 ? argv[2] : "felisian_shore";
    if (want == "list") { for (const CmpScene& sc : CMP_SCENES) printf("%s\n", sc.name); return 0; }   // E-02: every scene, in the table's order
    // `scene <name> [scale] [warm]` (B-310): the frame at 1x .. 4x; R-307: warm > 0 runs that many seconds of the world first and stages
    // the type's event in view
    if (argc > 3) setFramebufferScale(atoi(argv[3]));
    double warm = argc > 4 ? atof(argv[4]) : 0;
    SpaceRenderer sr; StarNeighborhood nb;
    if (const CmpScene* found = findScene(want)) {
        const CmpScene& sc = *found;
        StarSystem sys; SurfaceView sv; double t = 0;
        if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { printf("no body\n"); return 1; }
        if (warm > 0) {
            Input in;
            for (double w = 0; w < warm; w += 0.05) { t += 0.05; sv.update(0.05, in, t, false); }
            double fx = std::sin(sv.player.yaw), fz = std::cos(sv.player.yaw);
            if (sc.type == PT_TECTONIC || sc.type == PT_MAGMA_OCEAN) {   // a fountain on the nearest lava cell ahead, six seconds in (P-01: the magma shore's too)
                bool found = false;
                for (double d = 160; d <= 420 && !found; d += 8)
                    for (double s2 = -d * 0.45; s2 <= d * 0.45 && !found; s2 += 8) {   // within the view's cone, far enough for the column to fit the frame
                        double x = sv.player.x + fx * d - fz * s2, z = sv.player.z + fz * d + fx * s2;
                        if (sv.site.lod0.at((int)std::floor(x / 16), (int)std::floor(z / 16)).material == MAT_LAVA) { sv.eruptions.push_back({x, z, t - 6, 1}); found = true; }
                    }
                if (found) {
                    const SurfaceView::Eruption& e = sv.eruptions.back();
                    printf("  fountain staged %.0f m ahead\n", std::sqrt((e.x - sv.player.x) * (e.x - sv.player.x) + (e.z - sv.player.z) * (e.z - sv.player.z)));
                } else printf("  fountain staged: no lava 160-420 m ahead\n");
                if (sc.type == PT_TECTONIC) sv.quake = 0.8;
            }
            if (sc.type == PT_STEAM) {   // the clock moved to a moment the nearest vent ahead bursts (`steamVents` on their sinter)
                uint64_t best = 0; double bd = 1e18;
                std::vector<GeyserVent> vents; steamVents(sv.site.gen, sv.site.unitAt(sv.player.x, sv.player.z), vents);
                for (const GeyserVent& v : vents) {
                    double vx, vz; sv.site.localAt(v.unit, vx, vz);
                    if (sv.site.lod0.at((int)std::floor(vx / 16), (int)std::floor(vz / 16)).material != KINDS[PT_STEAM].eventMaterial) continue;
                    const double d = (vx - sv.player.x) * (vx - sv.player.x) + (vz - sv.player.z) * (vz - sv.player.z);
                    if ((vx - sv.player.x) * fx + (vz - sv.player.z) * fz > 0 && d < bd) { bd = d; best = v.id; }
                }
                if (best) {
                    const double t0 = t;
                    for (double dtq = 0; dtq < 200 && SurfaceView::burstJetStrength(best, t) < 0.9; dtq += 0.25) t = t0 + dtq;
                    Input in2; sv.update(0.05, in2, t, false);
                    printf("  nearest vent ahead %.0f m off, bursting at t+%.0f s\n", std::sqrt(bd), t - t0);
                } else printf("  no vent ahead\n");
            }
            if (sc.type == PT_BOMBARDED) sv.eruptions.push_back({sv.player.x + fx * 220, sv.player.z + fz * 220, t - 1.5, 2});
            if (sc.wantMat == SCENE_GEYSERS) {   // move the clock to a moment the nearest vent's jet is at full strength
                std::vector<GeyserVent> vents; geyserVents(sv.site.gen, sv.site.unitAt(sv.player.x, sv.player.z), vents);
                uint64_t best = 0; double bd = 1e18;
                for (const GeyserVent& v : vents) {
                    double vx, vz;
                    sv.site.localAt(v.unit, vx, vz);
                    double d = (vx - sv.player.x) * (vx - sv.player.x) + (vz - sv.player.z) * (vz - sv.player.z);
                    if (d < bd) {
                        bd = d;
                        best = v.id;
                    }
                }
                double t0 = t;
                for (double dtq = 0; dtq < 400 && SurfaceView::basinJetStrength(best, t) < 0.9; dtq += 0.5) t = t0 + dtq;
                Input in2; sv.update(0.05, in2, t, false);
                printf("  nearest vent %.0f m off, strength %.2f at t+%.0f s\n", std::sqrt(bd), SurfaceView::basinJetStrength(best, t), t - t0);
            }
            if (sc.type == PT_DESERT) {
                SurfaceView::DustDevil d;
                d.x = sv.player.x + fx * 130 - fz * 20;
                d.z = sv.player.z + fz * 130 + fx * 20;
                d.height = 70;
                d.radius = 12;
                d.age = 40;
                d.life = 300;
                d.phase = 1.0;
                sv.devils.push_back(d);
            }
        }
        // the sounds of the ground: the ice's and the quake's cue before the frame, the dunes' walk after it (their frame from the crest)
        SceneSound snd; const bool heard = sc.wantMat <= SCENE_SINGING_DUNES && sc.wantMat >= SCENE_QUAKES;
        double slopeDeg = 0;
        if (heard) {
            if (sc.wantMat == SCENE_QUAKES) sv.nextQuake = 0;
            if (sc.wantMat != SCENE_SINGING_DUNES) sceneSoundRun(sv, t, sc.wantMat == SCENE_SINGING_ICE ? 3.0 : 1.5, 0.0, snd);
            else {   // on the face (the landing put the explorer beside the capsule), the capsule behind the view, the near ring under the explorer
                sv.player.x = 0; sv.player.z = 0; sv.player.y = sv.site.surfaceHeight(0, 0);
                sv.relocateCapsule(-std::sin(sv.player.yaw) * 14, -std::cos(sv.player.yaw) * 14);
                Input in0; sv.update(0.016, in0, t, false);
            }
            for (double d = 0; d <= 12; d += 2) {   // the face ahead, at its steepest
                const double x = sv.player.x + d * std::sin(sv.player.yaw), z = sv.player.z + d * std::cos(sv.player.yaw);
                const double gx = (sv.site.groundHeight(x + 2, z) - sv.site.groundHeight(x - 2, z)) / 4, gz = (sv.site.groundHeight(x, z + 2) - sv.site.groundHeight(x, z - 2)) / 4;
                slopeDeg = std::max(slopeDeg, std::atan(std::sqrt(gx * gx + gz * gz)) / DEG);
            }
        }
        Framebuffer fb;
        sv.render(fb, t, nb.stars, sr, 1.0);
        fb.mush(2);
        if (sv.inBuggy && !sv.chaseCam) sv.cameraFeed(fb, t);   // the nose camera's picture
        if (heard) {
            const double atFrame = sv.quake;
            sceneSoundRun(sv, t, sc.wantMat == SCENE_SINGING_DUNES ? 7.5 : (sc.wantMat == SCENE_SINGING_ICE ? 17.0 : 10.5), sc.wantMat == SCENE_SINGING_DUNES ? 2.5 : 0.0, snd);
            const std::string wf = std::string("shots/tests/scene_") + sc.name + ".wav";
            writeWav16(wf, snd.pcm, 22050);
            double peak = 0, sum = 0; for (int16_t v : snd.pcm) { const double a = v / 32767.0; peak = std::max(peak, std::fabs(a)); sum += a * a; }
            const BodyGen& g = sv.site.gen;
            if (sc.wantMat == SCENE_SINGING_DUNES)
                printf("  the cue: a slip face of %.0f degrees at its steepest ahead of the explorer, walked down; the hum at %.2f, its note %.0f Hz (the grains' under %.2f g)\n", slopeDeg, snd.dunes,
                        sv.duneHz, sv.site.gravity / 9.8);
            if (sc.wantMat == SCENE_SINGING_ICE)
                printf("  the cue: the ice round the explorer singing at %.2f (%d sounds in twenty seconds), the sun %.1f degrees up\n", snd.ice, snd.st.iceEvents, sv.env.sun.altitude / DEG);
            if (sc.wantMat == SCENE_QUAKES) printf("  the cue: a quake of %.2f struck as the scene began, %.2f left at the frame\n", snd.quake, atFrame);
            printf("  heard: %s (%s), the air %.2f atm: %.0f s, peak %.3f, rms %.3f -> %s\n", sys.bodies[sv.site.body].name.c_str(), KINDS[g.type].name, sv.env.pressureAtm, snd.pcm.size() / 22050.0,
                    peak, std::sqrt(sum / std::max<size_t>(1, snd.pcm.size())), wf.c_str());
        }
        std::string fn = std::string("shots/tests/scene_") + sc.name + ".png";
        saveFB(fb, fn.c_str());
        printf("%s -> %s (sun alt %.1f)\n", sc.name, fn.c_str(), sv.env.sun.altitude / DEG);
        if (sc.wantMat <= SCENE_ROGUE_LAMP && sc.wantMat >= SCENE_DARK_OCEAN) {   // the place, its warmth and air, and the lamp's light on the ground ahead
            const Body& db = sys.bodies[sv.site.body];
            const BodyGen& dg = sv.site.gen;
            printf("  %s (%s, %.0f km, own heat %.0f K): %s; %+.0f C, %.2g atm, the lamp %s (its light 8 m ahead on the ground %.2f)\n", db.name.c_str(), KINDS[db.type].name, db.radiusKm, db.heatK,
                    traitList(dg).c_str(), sv.env.temperatureC, sv.env.pressureAtm, sv.lamp ? "on" : "off",
                    sv.lampAt(Vec3(sv.player.x + 8 * std::sin(sv.player.yaw), sv.site.groundHeight(sv.player.x + 8 * std::sin(sv.player.yaw), sv.player.z + 8 * std::cos(sv.player.yaw)),
                            sv.player.z + 8 * std::cos(sv.player.yaw))));
        }
        if (sc.wantMat == SCENE_ROGUE_LAMP) {   // the same without the lamp
            sv.lamp = false;
            Framebuffer fbo; sv.render(fbo, t, nb.stars, sr, 1.0); fbo.mush(2);
            const std::string fo = std::string("shots/tests/scene_") + sc.name + "_off.png";
            saveFB(fbo, fo.c_str());
            printf("  the lamp off -> %s\n", fo.c_str());
        }
        if (sc.wantMat <= SCENE_FRONT_FAR && sc.wantMat >= SCENE_FRONT_BEHIND) printf("  front: %s\n", surfaceFrontInfo(sv).c_str());
        if (sc.wantMat == SCENE_METEOR_SHOWER) {   // twenty seconds of the shower in one picture (each pixel its brightest), the streaks fanning from the radiant
            std::vector<uint32_t> acc(FBW * FBH, 0xFF000000u), one(FBW * FBH);
            int streaks = 0;
            for (int k = 0; k < 200; k++) {
                double T = t - 10 + k * 0.1;
                Input in4; sv.update(0.1, in4, T, false);
                std::vector<SurfaceView::ShowerMeteor> ms; sv.showerMeteorsAt(T, ms); streaks += (int)ms.size();
                Framebuffer f4; sv.render(f4, T, nb.stars, sr, 1.0); f4.mush(2);
                f4.toRGB(one.data(), 1.0);
                for (int i = 0; i < FBW * FBH; i++) {
                    uint32_t a = acc[i], b = one[i];
                    uint32_t r = std::max(a & 0xFF, b & 0xFF), g = std::max((a >> 8) & 0xFF, (b >> 8) & 0xFF), bl = std::max((a >> 16) & 0xFF, (b >> 16) & 0xFF);
                    acc[i] = 0xFF000000u | (bl << 16) | (g << 8) | r;
                }
            }
            std::string fn4 = std::string("shots/tests/scene_") + sc.name + "_20s.png";
            writePNG(fn4.c_str(), acc.data(), FBW, FBH);
            printf("  twenty seconds (%d streak-frames alight in the sky) -> %s\n", streaks, fn4.c_str());
        }
        if (sc.wantMat <= SCENE_METEOR_SHOWER && sc.wantMat >= SCENE_MOONLIT)   // the night's lights
            printf("  night: the moonlight %.2f from %s at %.0f deg (sun %.1f deg), the zodiacal dust %.2f, %zu shower%s (activity %.2f)\n", sv.env.moonLight,
                    sv.env.moonBody >= 0 ? sys.bodies[sv.env.moonBody].name.c_str() : "none", std::asin(clampd(sv.env.moonDir.y, -1, 1)) / DEG, sv.env.sun.altitude / DEG, sv.zodiDust,
                    sv.showers.size(), sv.showers.size() == 1 ? "" : "s", sv.showers.empty() ? 0.0 : showerActivity(sv.showers[0], t));
        if (sc.wantMat <= SCENE_AURORA && sc.wantMat >= SCENE_AURORA_CURTAIN)   // the aurora's numbers
            printf("  aurora %.2f under a %s at lat %.1f (oval at %.1f, centre %.0f km poleward), field %.2f (%s), storm %.2f, R %.0f km, sky %.2f, cloud %.2f, flocks %zu\n", sv.env.aurora,
                    STAR_CLASSES[sys.star.cls].name, sv.env.latDeg, SurfaceView::auroraOvalLat(sys.bodies[sv.site.body]), sv.lastAuroraP0, magneticField(sys.bodies[sv.site.body]),
                    MAGNETIC_CLASS_NAMES[magneticClass(magneticField(sys.bodies[sv.site.body]))], sv.env.auroraStorm, sv.site.R / 1000.0, sv.env.skyBrightness, sv.env.cloudCover, sv.flocks.size());
        if (sc.wantMat == SCENE_TOWN || sc.wantMat == SCENE_SHARD || sc.wantMat == SCENE_HARBOUR || sc.wantMat == SCENE_TERRACES || sc.wantMat == SCENE_ROAD || sc.wantMat == SCENE_GRAVES ||
                sc.wantMat == SCENE_SECOND_PEOPLE ||
                (sc.wantMat <= SCENE_AGE_PREHISTORIC &&
                        sc.wantMat >= SCENE_AGE_FAR_FUTURE))   // the ages' scenes too; C-01; C-03: the shards drawn and the one within reach; C-08; C-09; C-12: the grave within reach
            printf("  ruins: %d pieces drawn in %.2f ms, %d shards drawn%s%s\n", sv.lastRuinElems, sv.lastRuinMs, sv.lastShardsDrawn,
                   sv.nearShard.index >= 0 ? fmt(", shard %d within reach (%s, %.1f m)", sv.nearShard.index, buildingPlaceName(sv.nearShard.place), sv.nearShard.dist).c_str() : "",
                   sv.nearGrave.k >= 0 ? fmt(", grave %d within reach (%s, %.1f m)", sv.nearGrave.k, sv.nearGrave.line.c_str(), sv.nearGrave.dist).c_str() : "");
        if (sc.wantMat == SCENE_HERD || sc.wantMat == SCENE_HERD_NEAR || sc.wantMat == SCENE_HERD_FAR || sc.wantMat == SCENE_CREATURES)   // the herd's states and the cost of life
            printf("  herd: %s; life update %.2f ms, draw %.2f ms\n", sv.testHerdStates().c_str(), sv.lastLifeMs[0], sv.lastLifeMs[1]);
        if (sc.wantMat == SCENE_FOREST || sc.wantMat <= SCENE_BIOME) {   // what the vegetation did
            const TerrainVertex& tvp = sv.site.lod0.at((int)std::floor(sv.player.x / 16), (int)std::floor(sv.player.z / 16));
            double cov = sv.testCanopyCoverage(200);
            printf("  biome %s, veg %.2f; trees drawn %d (near %d, mid %d, far %d), flora points %d; canopy coverage of the forest ground within 200 m: %.0f%%%s\n", BIOME_NAMES[tvp.biome], tvp.veg,
                   sv.lastTreesDrawn, sv.lastNearTrees, sv.lastMidTrees, sv.lastFarTrees, sv.lastFloraPoints, cov * 100, (sc.wantMat == SCENE_FOREST && cov < 0.6) ? " (FAIL: under 60%)" : "");
            printf("  families %d/%d, vegColor %.2f,%.2f,%.2f, vegColor2 %.2f,%.2f,%.2f, season phase %.2f, fog %.0f m, wind %.0f kn; bank 0 stops 14/34 = %d,%d,%d / %d,%d,%d; pixel(160,120) bank %d "
                   "shade %.0f\n",
                    sv.site.gen.floraFamily, sv.site.gen.floraFamily2, sv.site.gen.vegColor.r, sv.site.gen.vegColor.g, sv.site.gen.vegColor.b, sv.site.gen.vegColor2.r, sv.site.gen.vegColor2.g,
                    sv.site.gen.vegColor2.b, sv.seasonPhaseLocal(), sv.env.fogDistance, sv.env.windKnots, fb.pal[14 * 3], fb.pal[14 * 3 + 1], fb.pal[14 * 3 + 2], fb.pal[34 * 3], fb.pal[34 * 3 + 1],
                    fb.pal[34 * 3 + 2], bankOf(fb.idx[(FBH * 3 / 5) * FBW + FBW / 2]), shadeOf(fb.idx[(FBH * 3 / 5) * FBW + FBW / 2]));
        }
        {   // what the lower half is drawn with (bank histogram, mean shade, the majority bank's ramp stops)
            int hist[BANKS] = {0}; double sum = 0; int n = 0;
            for (int y = FBH / 2; y < FBH; y++) for (int x = 0; x < FBW; x++) { hist[bankOf(fb.idx[y * FBW + x])]++; sum += shadeOf(fb.idx[y * FBW + x]); n++; }
            int mb = 0; for (int k = 1; k < BANKS; k++) if (hist[k] > hist[mb]) mb = k;
            printf("  lower half: bank %d %d%%, mean shade %.1f; bank %d stops 14/34/47 = %d,%d,%d / %d,%d,%d / %d,%d,%d; sky %.2f snow %.2f dust %.2f underwater %d\n", mb, hist[mb] * 100 / n,
                    sum / n, mb, fb.pal[(mb * 64 + 14) * 3], fb.pal[(mb * 64 + 14) * 3 + 1], fb.pal[(mb * 64 + 14) * 3 + 2], fb.pal[(mb * 64 + 34) * 3], fb.pal[(mb * 64 + 34) * 3 + 1],
                    fb.pal[(mb * 64 + 34) * 3 + 2], fb.pal[(mb * 64 + 47) * 3], fb.pal[(mb * 64 + 47) * 3 + 1], fb.pal[(mb * 64 + 47) * 3 + 2], sv.env.skyBrightness, sv.env.snow, sv.env.dust,
                    (int)sv.player.underwater);
        }
    }
    return 0;
}

// B-318: the surface as the capsule sees it on the way down: eight altitudes of the descent, the share of sky below the horizon
int runCapsule(int argc, char** argv) {
    std::string want = argc > 2 ? argv[2] : "felisian_mountains";
    SpaceRenderer sr; StarNeighborhood nb;
    if (const CmpScene* found = findScene(want)) {
        const CmpScene& sc = *found;
        StarSystem sys; SurfaceView sv; double t = 0;
        if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { printf("no body\n"); return 1; }
        printf("  site: star %lld %lld %lld body %d lat %.4f lon %.4f t %.1f\n", (long long)sys.star.sx, (long long)sys.star.sy, (long long)sys.star.sz, sv.site.body, sv.site.lat0 / DEG,
                sv.site.lon0 / DEG, t);
        const double alts[8] = {1800, 1300, 900, 600, 400, 250, 120, 40};
        std::vector<uint32_t> sheet((size_t)FBW * 4 * FBH * 2, 0xFF000000u), px((size_t)FBW * FBH);
        bool bad = false;
        for (int k = 0; k < 8; k++) {
            double s = 1 - alts[k] / 1800.0;
            sv.cameraOverrideAlt = alts[k]; sv.cameraOverridePitch = (-60 + 60 * s) * DEG;
            Input in; sv.update(0.016, in, t, false);
            Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0);
            // sky-bank pixels below the horizon row are holes in the ground
            double dip = std::sqrt(2 * alts[k] / sv.site.R);   // the horizon sits below the level line by the curvature's dip at this height
            double horizon = sv.proj.cy - sv.proj.f * std::tan(-sv.cameraOverridePitch - dip) + 1;
            int holes = 0, rows = 0;
            // the beacon beam is on the sky bank too, but has a depth
            for (int y = std::max(0, (int)std::ceil(horizon)); y < FBH; y++) for (int x = 0; x < FBW; x++) { rows++; if (bankOf(fb.idx[y * FBW + x]) == 1 && fb.invz[y * FBW + x] == 0) holes++; }
            double share = rows ? 100.0 * holes / rows : 0.0;
            if (share > 0.5) bad = true;
            printf("  %4.0f m pitch %5.1f: sky below the horizon %.2f%%%s\n", alts[k], sv.cameraOverridePitch / DEG, share, share > 0.5 ? "  <-- HOLES" : "");
            fb.mush(2); fb.toRGB(px.data(), 1.0);
            int ox = (k % 4) * FBW, oy = (k / 4) * FBH;
            for (int y = 0; y < FBH; y++) for (int x = 0; x < FBW; x++) sheet[(size_t)(oy + y) * FBW * 4 + ox + x] = px[(size_t)y * FBW + x];
        }
        sv.cameraOverrideAlt = -1;
        std::string fn = std::string("shots/tests/capsule_") + sc.name + ".png";
        writePNG(fn.c_str(), sheet.data(), FBW * 4, FBH * 2);
        printf("%s: %s -> %s\n", sc.name, bad ? "FAIL (sky through the ground)" : "ok", fn.c_str());
        return bad ? 1 : 0;
    }
    printf("no such scene\n");
    return 1;
}

// B-322: a frame at an exact place: `spot <sx> <sy> <sz> <body> <latDeg> <lonDeg> <t> <swim|ascent|stand> [pitchDeg] [yawDeg] [alt]`
int runSpot(int argc, char** argv) {
    if (argc < 9) { printf("spot <sx> <sy> <sz> <body> <latDeg> <lonDeg> <t> <swim|ascent|stand> [pitchDeg] [yawDeg] [alt]\n"); return 1; }
    Star s;
    if (!starInSector(atoll(argv[2]), atoll(argv[3]), atoll(argv[4]), s)) { printf("no star\n"); return 1; }
    StarSystem sys; sys.generate(s);
    int bi = atoi(argv[5]);
    if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body\n"); return 1; }
    double lat = atof(argv[6]) * DEG, lon = atof(argv[7]) * DEG, t = atof(argv[8]);
    std::string kind = argv[9];
    double pitchDeg = argc > 10 ? atof(argv[10]) : 0, yawDeg = argc > 11 ? atof(argv[11]) : 0, alt = argc > 12 ? atof(argv[12]) : 1500;
    SpaceRenderer sr; StarNeighborhood nb; SurfaceView sv;
    { const BodyGen g = BodyGen::make(sys.bodies[bi]); if (worldHadCivilisation(g)) worldWorksOf(g); }   // the plan and the works as the approach builds them
    sv.init(&sys, bi, lat, lon, t);
    nb.update(sv.site.worldPos(t, 0, 0, 0));
    const Body& b = sys.bodies[bi];
    printf("%s (%s) at %.3f %.3f, t %.1f, sea %s %.1f, ground %.1f water %.1f\n", b.name.c_str(), KINDS[b.type].name, lat / DEG, lon / DEG, t, sv.site.hasWater ? "yes" : "no", sv.site.seaLevel,
            sv.site.groundHeight(0, 0), sv.site.waterAt(0, 0));
    printf("  %zu works within %.0f km (the world's %s); the explorer at %.0f %.0f\n", sv.workNear.size(), SurfaceView::PLAN_NEAR_M / 1000, sv.worksHere ? "built" : "NOT BUILT", sv.player.x,
            sv.player.z);
    for (const SurfaceView::WorkNear& wn : sv.workNear) if (std::hypot(wn.x - sv.player.x, wn.z - sv.player.z) - wn.radius < 4000 || getenv("SPOT_WORKS"))   // the works the view draws round the spot
            printf("  %s%s %.0f m off at %.0f %.0f (its reach %.0f, its ground %.0f m)\n", WORK_KIND_NAMES[wn.work->kind], wn.work->land ? "" : " (the world's)",
                    std::hypot(wn.x - sv.player.x, wn.z - sv.player.z), wn.x, wn.z, wn.radius, wn.work->ground);
    if (kind == "swim") {
        // the nearest water at least 3 m deep within 3 km, the player afloat in it
        double bx = 0, bz = 0, bd = 1e9;
        double minDepth = argc > 12 ? alt : 3.0;   // the twelfth argument: the depth wanted (default 3 m)
        for (double r = 0; r <= 3000 && bd > 1e8; r += 4)
            for (int a = 0; a < 64; a++) {
                double x = std::sin(a * TAU / 64) * r, z = std::cos(a * TAU / 64) * r;
                double w = sv.site.waterAt(x, z), g = sv.site.groundHeight(x, z);
                if (w > -1e8 && w - g > minDepth && r < bd) { bd = r; bx = x; bz = z; }
            }
        if (bd > 1e8) { printf("no water within 3 km\n"); return 1; }
        double w = sv.site.waterAt(bx, bz);
        sv.player.x = bx; sv.player.z = bz; sv.player.y = w - 1.15; sv.player.vy = 0;
        sv.player.yaw = yawDeg * DEG; sv.player.pitch = pitchDeg * DEG;
        Input in;
        for (int k = 0; k < 30; k++) sv.update(0.016, in, t, false);
        { Framebuffer fb0; sv.render(fb0, t, nb.stars, sr, 1.0); }   // the rings (and the near ring's morph of groundHeight) exist after a frame
        if (sv.site.groundHeight(bx, bz) > w - minDepth) {   // the near ring (4 m) puts land here: look again with the drawn ground
            double bd2 = 1e9, bx2 = bx, bz2 = bz;
            for (double r = 0; r <= 400 && bd2 > 1e8; r += 2)
                for (int a = 0; a < 64; a++) {
                    double x = bx + std::sin(a * TAU / 64) * r, z = bz + std::cos(a * TAU / 64) * r;
                    double ww = sv.site.waterAt(x, z), g = sv.site.groundHeight(x, z);
                    if (ww > -1e8 && ww - g > minDepth && r < bd2) { bd2 = r; bx2 = x; bz2 = z; }
                }
            bx = bx2; bz = bz2; w = sv.site.waterAt(bx, bz);
            sv.player.x = bx; sv.player.z = bz; sv.player.y = w - 1.15; sv.player.vy = 0;
            for (int k = 0; k < 30; k++) sv.update(0.016, in, t, false);
        }
        double eyeOff = argc > 13 ? atof(argv[13]) : 1e9;   // the thirteenth argument: the eye's height over the water, forced
        if (eyeOff < 1e8) { sv.player.y = w + eyeOff - sv.player.eyeHeight; sv.player.bobY = 0; sv.player.landDip = 0; sv.player.underwater = eyeOff < -0.05; }
        printf("swimming %d underwater %d at (%.0f, %.0f) eye %.2f water %.2f ground %.2f, %.0f m from the site\n", (int)sv.player.swimming, (int)sv.player.underwater, bx, bz,
                sv.player.y + sv.player.eyeHeight, w, sv.site.groundHeight(bx, bz), bd);
        Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
        {   // the lod0 and near-ring vertices round the swimmer: height / water / material
            int cx0 = (int)std::floor(bx / 16), cz0 = (int)std::floor(bz / 16);
            for (int dz = 2; dz >= -2; dz--) {
                for (int dx = -2; dx <= 2; dx++) { const TerrainVertex& v = sv.site.lod0.at(cx0 + dx, cz0 + dz); printf("  %7.1f/%6.1f/m%d", v.h, v.water > -1e8 ? v.water : -99.0, v.material); }
                printf("\n");
            }
            printf("  near ring (4 m):\n");
            int nx0 = (int)std::floor(bx / 4), nz0 = (int)std::floor(bz / 4);
            for (int dz = 2; dz >= -2; dz--) {
                for (int dx = -2; dx <= 2; dx++) { const TerrainVertex& v = sv.site.lodN.at(nx0 + dx, nz0 + dz); printf("  %7.1f/%6.1f/m%d", v.h, v.water > -1e8 ? v.water : -99.0, v.material); }
                printf("\n");
            }
            printf("  groundHeight %.2f coarse %.2f waterAt %.2f nearBand %d lod0R %d nearR %d nearBlend %.2f\n", sv.site.groundHeight(bx, bz), sv.site.groundHeightCoarse(bx, bz),
                    sv.site.waterAt(bx, bz), sv.site.nearBand, sv.site.lod0R, sv.site.nearR, sv.site.nearBlend);
        }
        std::string fn = std::string("shots/tests/spot_swim_") + argv[10 < argc ? 10 : 9] + "_" + argv[11 < argc ? 11 : 9] + (argc > 13 ? std::string("_e") + argv[13] : std::string()) + ".png";
        saveFB(fb, fn.c_str());
        printf("-> %s\n", fn.c_str());
    } else if (kind == "ascent") {
        sv.cameraOverrideAlt = alt; sv.cameraOverridePitch = pitchDeg * DEG; sv.player.yaw = yawDeg * DEG;
        Input in; sv.update(0.016, in, t, false);
        Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
        {   // the 64 m ring's vertices under the camera: height / material (the water is 0 everywhere on a sea)
            int cx0 = (int)std::floor(sv.player.x / 64), cz0 = (int)std::floor(sv.player.z / 64);
            for (int dz = 6; dz >= -6; dz--) {
                for (int dx = -6; dx <= 6; dx++) { const TerrainVertex& v = sv.site.lod1.at(cx0 + dx, cz0 + dz); printf(" %6.1f/%d", v.h, v.material); }
                printf("\n");
            }
            printf("  (64 m cells; the same at 16 m:)\n");
            cx0 = (int)std::floor(sv.player.x / 16); cz0 = (int)std::floor(sv.player.z / 16);
            for (int dz = 6; dz >= -6; dz--) {
                for (int dx = -6; dx <= 6; dx++) { const TerrainVertex& v = sv.site.lod0.at(cx0 + dx, cz0 + dz); printf(" %6.1f/%d", v.h, v.material); }
                printf("\n");
            }
        }
        std::string fn = std::string("shots/tests/spot_ascent_") + std::to_string((int)alt) + ".png";
        saveFB(fb, fn.c_str());
        printf("-> %s\n", fn.c_str());
    } else if (kind == "dump") {   // the 64 m ring's heights (metres, integer) round the site: W = water material, i = ice, s = sand, . = other
        sv.cameraOverrideAlt = 1000; sv.cameraOverridePitch = -60 * DEG;
        Input in; sv.update(0.016, in, t, false);
        { Framebuffer fb0; sv.render(fb0, t, nb.stars, sr, 1.0); }
        int half = (int)(std::fabs(alt) > 0 ? std::fabs(alt) : 15);
        for (int cz = half; cz >= -half; cz--) {
            for (int cx = -half; cx <= half; cx++) {
                const TerrainVertex& v = sv.site.lod1.at(cx, cz);
                char m = v.material == MAT_WATER
                        ? 'W'
                        : (v.material == MAT_ICE
                                          ? 'i'
                                          : (v.material == MAT_SAND
                                                            ? 's'
                                                            : (v.material == MAT_DUST
                                                                              ? 'd'
                                                                              : (v.material == MAT_LAVA
                                                                                                ? 'L'
                                                                                                : (v.material == MAT_BASALT
                                                                                                                  ? 'b'
                                                                                                                  : (v.material == MAT_SULPHUR ? 'y' : (v.material == MAT_SALT ? 'S' : '.')))))));
                if (alt < 0) {   // negative alt: shore distance / height over the water (metres); "  ----  " = no water here
                    if (v.water > -1e8f) printf(" %4.0f/%+5.1f", v.shore < 1e8f ? v.shore : 9999.0, v.h - v.water); else printf("   ----   ");
                } else printf("%5d%c", (int)std::lround(v.h), m);
            }
            printf("\n");
        }
    } else {
        sv.player.yaw = yawDeg * DEG; sv.player.pitch = pitchDeg * DEG;
        Input in; sv.update(0.016, in, t, false);
        Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
        saveFB(fb, "shots/tests/spot_stand.png");
        printf("-> shots/tests/spot_stand.png\n");
    }
    return 0;
}
