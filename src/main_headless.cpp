// Headless harness: dumps generation statistics and renders test frames to PNG
// so the look can be checked without opening a window.
#include "core/framebuffer.h"
#include "core/raster.h"
#include "core/font.h"
#include "core/png.h"
#include "core/fs.h"
#include "galaxy/starfield.h"
#include "galaxy/system.h"
#include "galaxy/planetmap.h"
#include "galaxy/drainage.h"
#include "galaxy/landmarks.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <sys/stat.h>
#include <sstream>
#include <cmath>

static int g_testScale = 1;
// the pinned felisian mountain site (the O6 review site, the `felisian_mountains` scene, `descent`); O6-03: re-pinned on the GEN 10 bodies
static constexpr double PIN_MOUNTAIN_LAT = -47.733, PIN_MOUNTAIN_LON = -43.747;   // `scale=N` on the command line renders every Game-based mode at that scale

static double nowSec() {
    using namespace std::chrono;
    return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
}

static void testStars() {
    int counts[STAR_CLASS_COUNT] = {0};
    int total = 0, sectors = 0;
    for (int64_t x = 150; x < 200; x++)
        for (int64_t y = -6; y < 6; y++)
            for (int64_t z = 20; z < 70; z++) {
                Star s;
                sectors++;
                if (starInSector(x, y, z, s)) { total++; counts[s.cls]++; }
            }
    printf("Stars near start: %d in %d sectors (%.1f%%)\n", total, sectors, 100.0 * total / sectors);
    {
        int reg[REGION_COUNT] = {0};
        for (int64_t x = 150; x < 200; x++) for (int64_t y = -6; y < 6; y++) for (int64_t z = 20; z < 70; z++) reg[galaxyRegion(x, y, z)]++;
        printf("regions of those sectors:"); for (int i = 0; i < REGION_COUNT; i++) if (reg[i]) printf("  %s %d", REGION_NAMES[i], reg[i]); printf("\n");
        int belts = 0, captured = 0, sysN = 0;
        for (int64_t x = 150; x < 190; x++) for (int64_t z = 20; z < 60; z++) { Star s; if (!starInSector(x, 0, z, s)) continue; StarSystem sys; sys.generate(s); sysN++; belts += (int)sys.belts.size(); for (auto& b : sys.bodies) if (b.orbitPeriod < 0) captured++; }
        printf("belts: %d in %d systems, captured retrograde moons: %d\n", belts, sysN, captured);
    }
    for (int i = 0; i < STAR_CLASS_COUNT; i++) printf("  %s %-14s %d\n", STAR_CLASSES[i].code, STAR_CLASSES[i].name, counts[i]);
    int typeCounts[PT_COUNT] = {0};
    int moons = 0, planets = 0, systems = 0;
    for (int64_t x = 150; x < 190; x++)
        for (int64_t z = 20; z < 60; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys;
            sys.generate(s);
            systems++;
            for (auto& b : sys.bodies) { typeCounts[b.type]++; if (b.parent < 0) planets++; else moons++; }
        }
    printf("Systems: %d planets: %d moons: %d\n", systems, planets, moons);
    for (int i = 0; i < PT_COUNT; i++) printf("  %-16s %d\n", PLANET_TYPES[i].name, typeCounts[i]);
    // print a few systems in detail
    int shown = 0;
    for (int64_t x = 180; x < 200 && shown < 4; x++) {
        Star s;
        if (!starInSector(x, 0, 40, s)) continue;
        StarSystem sys;
        sys.generate(s);
        printf("\n%s  class %s (%s)  R=%.0f km  L=%.2f  bodies=%zu\n", s.name.c_str(), STAR_CLASSES[s.cls].code,
               STAR_CLASSES[s.cls].name, s.radiusKm, s.luminosity, sys.bodies.size());
        for (auto& b : sys.bodies)
            printf("   %-28s %-16s R=%6.0f km  orbit=%.3g km  T=%.1f h  rot=%.1f h  tilt=%.0f  temp=%.0fK moons=%d %s\n",
                   b.name.c_str(), PLANET_TYPES[b.type].name, b.radiusKm, b.orbitRadiusKm, b.orbitPeriod / 3600,
                   std::fabs(b.rotPeriod) / 3600, b.axialTilt / DEG, b.tempK, b.moonCount, b.rings ? "rings" : "");
        shown++;
    }
}

static void renderMaps() {
    // find one body of each type and dump its map
    bool done[PT_COUNT] = {false};
    bool lockedDone = false;
    int remaining = PT_COUNT;
    std::vector<uint32_t> img(PlanetMap::W * PlanetMap::H * 2);
    for (int64_t x = 150; x < 260 && (remaining > 0 || !lockedDone); x++)
        for (int64_t z = 20; z < 120 && (remaining > 0 || !lockedDone); z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys;
            sys.generate(s);
            for (auto& b : sys.bodies) {
                if (done[b.type] && !(b.type == PT_FELISIAN && b.locked && !lockedDone)) continue;
                if (b.type == PT_COMPANION) { done[b.type] = true; remaining--; continue; }   // a sun has no map
                bool lockedShot = b.type == PT_FELISIAN && b.locked && done[b.type];   // M5-08: one extra map of a locked living world
                if (lockedShot) lockedDone = true;
                else { done[b.type] = true; remaining--; }
                BodyGen g = BodyGen::make(b);
                PlanetMap m;
                double t0 = nowSec();
                m.generate(g);
                double t1 = nowSec();
                printf("map %-16s %s: %.0f ms\n", PLANET_TYPES[b.type].name, b.name.c_str(), (t1 - t0) * 1000);
                for (int y = 0; y < PlanetMap::H; y++)
                    for (int xx = 0; xx < PlanetMap::W; xx++) {
                        int i = y * PlanetMap::W + xx;
                        int a = m.albedo[i];
                        int c = m.cloud[i];
                        RGB col = b.color;
                        double v = a / 255.0;
                        RGB pixc = col * (float)v;
                        pixc = lerp(pixc, RGB(1, 1, 1) * (float)(0.9), (float)(c / 255.0) * 0.8f);
                        img[i] = rgb((int)(pixc.r * 255), (int)(pixc.g * 255), (int)(pixc.b * 255));
                        // second row: height map
                        double h = m.height[i];
                        int hv = clampi((int)(128 + h / 40.0), 0, 255);
                        uint32_t hc = rgb(hv, hv, hv);
                        static const uint32_t matCol[MAT_COUNT] = {rgb(120, 110, 100), rgb(220, 200, 140), rgb(90, 160, 60), rgb(30, 90, 30), rgb(240, 240, 250), rgb(20, 50, 140), rgb(255, 120, 20), rgb(200, 220, 255), rgb(255, 255, 255), rgb(240, 220, 235), rgb(60, 55, 55), rgb(160, 130, 110), rgb(200, 180, 140), rgb(120, 115, 112), rgb(230, 200, 90), rgb(40, 40, 42), rgb(236, 233, 224)};
                        uint32_t mc = matCol[m.material[i]];
                        img[PlanetMap::W * PlanetMap::H + i] = (xx < PlanetMap::W / 2) ? hc : mc;
                    }
                std::string fn = std::string("shots/tests/map_") + (lockedShot ? "LOCKED_" : "") + PLANET_TYPES[b.type].name + ".png";
                for (char& ch : fn) if (ch == ' ') ch = '_';
                writePNG(fn.c_str(), img.data(), PlanetMap::W, PlanetMap::H * 2);
                if (lockedShot) printf("  locked: sub-stellar point lat %.0f lon %.0f -> %s\n", std::asin(b.lockedDir.z) / DEG, std::atan2(b.lockedDir.y, b.lockedDir.x) / DEG, fn.c_str());
            }
        }
}


// ---- space scene tests -------------------------------------------------------
#include "space/space_view.h"
static void saveFB(Framebuffer& fb, const char* path, double gain = 1.0) {
    std::vector<uint32_t> rgbBuf(FBW * FBH);
    fb.toRGB(rgbBuf.data(), gain);
    writePNG(path, rgbBuf.data(), FBW, FBH);
}

// maps are generated on worker threads (M9-15): the tests wait for the ones they are about to draw
static void warmMaps(SpaceRenderer& sr, const StarSystem& sys, int only = -1) {
    for (const Body& b : sys.bodies) {
        if (b.type == PT_COMPANION) continue;
        if (only >= 0 && b.index != only) continue;
        sr.mapFor(b);
    }
}

static uint64_t fnv(uint64_t h, const void* p, size_t n);

static void renderSpace() {
    SpaceRenderer sr;
    StarNeighborhood nb;
    // 1) one sun per class, seen from the first orbit distance
    bool done[STAR_CLASS_COUNT] = {false};
    int remaining = STAR_CLASS_COUNT;
    for (int64_t x = 150; x < 300 && remaining; x++)
        for (int64_t z = 20; z < 120 && remaining; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            if (done[s.cls]) continue;
            done[s.cls] = true; remaining--;
            StarSystem sys; sys.generate(s);
            double d = std::max(s.radiusKm * STAR_CLASSES[s.cls].firstOrbitMult, STAR_CLASSES[s.cls].minFirstOrbitKm);
            Vec3 shipPos = s.pos + Vec3(0, 0, -d);
            nb.update(shipPos);
            Framebuffer fb;
            SpaceContext c;
            c.sys = &sys; c.stars = &nb.stars; c.t = 1234.0; c.shipPos = shipPos;
            c.cam = cameraBasis(0, 0);
            sr.setupPalette(fb, &sys, -1, -1, 1.0);
            sr.render(fb, c);
            fb.mush(2);
            std::string fn = std::string("shots/tests/sun_") + STAR_CLASSES[s.cls].code + ".png";
            saveFB(fb, fn.c_str());
            printf("sun %s %s R=%.0f d=%.3g\n", STAR_CLASSES[s.cls].code, s.name.c_str(), s.radiusKm, d);
        }
    // 2) planet globes: one per type, parked view
    bool doneT[PT_COUNT] = {false};
    int remT = PT_COUNT;
    int gasWithRings = 0;
    for (int64_t x = 150; x < 300 && remT; x++)
        for (int64_t z = 20; z < 120 && remT; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (doneT[b.type] && !(b.type == PT_GASGIANT && b.rings && !gasWithRings)) continue;
                if (b.type == PT_GASGIANT && b.rings) gasWithRings = 1;
                if (!doneT[b.type]) { doneT[b.type] = true; remT--; }
                double t = 5000.0;
                Vec3 bp = sys.bodyPos(bi, t);
                // park: on the sunlit side, slightly off, at 3.5 radii (more for rings)
                Vec3 toStar = normalize(sys.star.pos - bp);
                Vec3 side = normalize(cross(toStar, Vec3(0, 1, 0)));
                double parkR = b.radiusKm * (b.rings ? std::max(3.5, b.ringOuter + 1.3) : 3.5);
                Vec3 shipPos = bp + normalize(toStar * 0.6 + side * 0.8 + Vec3(0, 0.25, 0)) * parkR;
                Vec3 fwd = normalize(bp - shipPos);
                double yaw = std::atan2(fwd.x, fwd.z), pitch = std::asin(fwd.y);
                nb.update(shipPos);
                Framebuffer fb;
                SpaceContext c;
                c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos;
                c.cam = cameraBasis(yaw, pitch);
                c.bankBodyA = bi;
                // second nearest body gets bank 3
                int second = -1; double sd = 1e300;
                for (int j = 0; j < (int)sys.bodies.size(); j++) if (j != bi) { double dd = length2(sys.bodyPos(j, t) - shipPos); if (dd < sd) { sd = dd; second = j; } }
                c.bankBodyB = second;
                warmMaps(sr, sys, bi); if (second >= 0) warmMaps(sr, sys, second);
                sr.setupPalette(fb, &sys, bi, second, 1.0);
                sr.render(fb, c);
                fb.mush(2);
                std::string fn = std::string("shots/tests/globe_") + PLANET_TYPES[b.type].name + (b.rings ? "_rings" : "") + ".png";
                for (char& ch : fn) if (ch == ' ') ch = '_';
                saveFB(fb, fn.c_str());
                printf("globe %-16s %s R=%.0f rings=%d star=%s\n", PLANET_TYPES[b.type].name, b.name.c_str(), b.radiusKm, (int)b.rings, STAR_CLASSES[s.cls].code);
                if (b.type == PT_GASGIANT) {   // N5-03: the storms a day later
                    const uint64_t FNV0 = 1469598103934665603ULL;
                    uint64_t h0 = fnv(FNV0, fb.idx.data(), fb.idx.size() * sizeof(Pix));
                    Framebuffer fb2; SpaceContext c2 = c; c2.t = t + 86400;
                    c2.shipPos = sys.bodyPos(bi, c2.t) + (shipPos - bp);   // ride along
                    sr.setupPalette(fb2, &sys, bi, second, 1.0); sr.render(fb2, c2); fb2.mush(2);
                    saveFB(fb2, "shots/tests/globe_GAS_GIANT_day_later.png");
                    uint64_t h1 = fnv(FNV0, fb2.idx.data(), fb2.idx.size() * sizeof(Pix));
                    printf("  storms a day later: frame %s -> shots/globe_GAS_GIANT_day_later.png\n", h0 != h1 ? "differs" : "IDENTICAL (FAIL)");
                }
            }
        }
    // O3 (R-302): the rocks of a belt seen from beside the biggest rock near an anchor in the belt
    {
        bool found = false;
        for (int64_t x = 150; x < 320 && !found; x++)
            for (int64_t z = 20; z < 140 && !found; z++) {
                Star s;
                if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                if (sys.belts.empty()) continue;
                const Belt& bl = sys.belts[0];
                double t = 4321.0, mid = 0.5 * (bl.innerKm + bl.outerKm);
                Vec3 anchor = sys.star.pos + Vec3(mid, 0, 0);
                BeltRock best; double bestR = -1; int n = 0;
                sys.forBeltRocksNear(0, anchor, t, 2, 2, 1, [&](const BeltRock& rk) { n++; if (rk.radiusKm > bestR) { bestR = rk.radiusKm; best = rk; } });
                if (bestR < 0) continue;
                Vec3 toStar = normalize(sys.star.pos - best.pos), side = normalize(cross(toStar, Vec3(0, 1, 0)));
                Vec3 shipPos = best.pos + normalize(toStar * 0.55 + side * 0.75 + Vec3(0, 0.18, 0)) * std::max(6.0 * best.radiusKm, 2.0);
                Vec3 fwd = normalize(best.pos - shipPos);
                nb.update(shipPos);
                Framebuffer fb; SpaceContext c;
                c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
                sr.setupPalette(fb, &sys, -1, -1, 1.0); sr.render(fb, c); fb.mush(2);
                saveFB(fb, "shots/tests/space_belt.png");
                BeltRock later; bool same = sys.beltRockAt(0, best.ir, best.ia, best.iy, best.m, t + 3600, later);
                printf("belt %s: %d rocks within two cells of the anchor, parked beside one %.1f km across, %d drawn (%d as meshes) -> shots/space_belt.png; an hour later the rock is %s, %.0f km along its orbit\n",
                       bl.name.c_str(), n, 2 * best.radiusKm, sr.lastBeltRocks, sr.lastBeltMeshes, same && std::fabs(later.radiusKm - best.radiusKm) < 1e-9 ? "still there" : "GONE (FAIL)", same ? length(later.pos - best.pos) : 0.0);
                found = true;
            }
        if (!found) printf("belt: none found in the scanned sectors\n");
    }
    // N5-01: the sky from a star-forming region: nebula patches over the star field
    {
        bool found = false;
        for (int64_t x = 100; x < 500 && !found; x += 2)
            for (int64_t z = 20; z < 400 && !found; z += 2) {
                if (galaxyRegion(x, 0, z) != REGION_NEBULA) continue;
                Vec3 pos((x + 0.5) * SECTOR_KM, 0, (z + 0.5) * SECTOR_KM);
                nb.update(pos);
                NebulaPatch np[16]; int n = nebulaPatches(pos / SECTOR_KM, np, 16);
                Framebuffer fb;
                SpaceContext c;
                c.sys = nullptr; c.stars = &nb.stars; c.t = 100.0; c.shipPos = pos;
                double bestYaw = 0; double bestG = -1;   // look toward the strongest patch
                for (int i = 0; i < n; i++) { double g = np[i].inten; if (g > bestG) { bestG = g; bestYaw = std::atan2(np[i].dir.x, np[i].dir.z); } }
                c.cam = cameraBasis(bestYaw, 0);
                sr.setupPalette(fb, nullptr, -1, -1, 1.0);
                sr.render(fb, c);
                fb.mush(2);
                saveFB(fb, "shots/tests/space_nebula.png");
                printf("nebula region at sector %lld,0,%lld: %d patches -> shots/space_nebula.png\n", (long long)x, (long long)z, n);
                found = true;
            }
        if (!found) printf("nebula region: none found in the scanned sectors\n");
    }
    // 3) M5-01 a multiple system: both suns in one frame, and the companion's first world if it has one
    // 4) M5-07 a comet at periapsis, tail streaming away from the star
    bool binaryDone = false, cometDone = false;
    for (int64_t x = 150; x < 320 && !(binaryDone && cometDone); x++)
        for (int64_t z = 20; z < 140 && !(binaryDone && cometDone); z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            if (!binaryDone && sys.companion >= 0) {
                double t = 2000.0;
                const Body& k = sys.bodies[sys.companion];
                Vec3 kp = sys.bodyPos(sys.companion, t);
                Vec3 along = normalize(kp - s.pos);
                Vec3 nrm = normalize(cross(along, Vec3(0, 1, 0)));
                // behind the primary (a real disc at the edge), the companion ahead as the brightest point in the sky
                double R = s.radiusKm;
                Vec3 shipPos = s.pos - along * (10.0 * R) + nrm * (4.0 * R) + Vec3(0, 1.0 * R, 0);
                Vec3 fwd = normalize(along * 10.0 - nrm * 2.5);
                nb.update(shipPos);
                Framebuffer fb;
                SpaceContext c;
                c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos;
                c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(fwd.y));
                sr.setupPalette(fb, &sys, -1, -1, 1.0);
                sr.render(fb, c);
                fb.mush(2);
                saveFB(fb, "shots/tests/space_binary.png");
                printf("binary %s: %s, companion %s R=%.0f L=%.3f at %.3g km (ecc %.2f), %d worlds of its own -> shots/space_binary.png\n", s.name.c_str(), sys.classString().c_str(),
                       STAR_CLASSES[k.starClass].name, k.radiusKm, k.luminosity, k.orbitRadiusKm, k.ecc, k.moonCount);
                for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                    const Body& b = sys.bodies[bi];
                    if (b.parent != sys.companion) continue;
                    Vec3 bp = sys.bodyPos(bi, t);
                    Vec3 toK = normalize(kp - bp);
                    Vec3 side = normalize(cross(toK, Vec3(0, 1, 0)));
                    Vec3 sp = bp + normalize(toK * 0.75 + side * 0.6 + Vec3(0, 0.2, 0)) * (b.radiusKm * 3.5);   // from the lit side
                    Vec3 f2 = normalize(bp - sp);
                    nb.update(sp);
                    Framebuffer fb2;
                    SpaceContext c2;
                    c2.sys = &sys; c2.stars = &nb.stars; c2.t = t; c2.shipPos = sp;
                    c2.cam = cameraBasis(std::atan2(f2.x, f2.z), std::asin(f2.y));
                    c2.bankBodyA = bi;
                    warmMaps(sr, sys, bi);
                    sr.setupPalette(fb2, &sys, bi, -1, 1.0);
                    sr.render(fb2, c2);
                    fb2.mush(2);
                    saveFB(fb2, "shots/tests/space_binary_world.png");
                    printf("  world of the companion: %s (%s) -> shots/space_binary_world.png\n", b.name.c_str(), PLANET_TYPES[b.type].name);
                    break;
                }
                binaryDone = true;
            }
            if (!cometDone)
                for (int bi = 0; bi < (int)sys.bodies.size() && !cometDone; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (b.type != PT_COMET) continue;
                    double t = (TAU - b.orbitPhase0) / TAU * b.orbitPeriod;   // mean anomaly 0: periapsis
                    Vec3 bp = sys.bodyPos(bi, t);
                    Vec3 away = normalize(bp - s.pos);
                    double refKm = sys.bodies[0].type == PT_COMET ? AU_GAME_KM : sys.bodies[0].orbitRadiusKm;
                    double lenKm = refKm * 0.5;
                    // seen from above the orbital plane, so the dust tail's curve along the orbit shows (N0-01)
                    Vec3 vel = sys.bodyVel(bi, t);
                    Vec3 side = normalize(cross(away, vel));
                    Vec3 back = normalize(-vel);
                    Vec3 shipPos = bp + side * (lenKm * 0.7) + away * (lenKm * 0.25) + back * (lenKm * 0.1);
                    Vec3 fwd = normalize(bp + away * (lenKm * 0.3) - shipPos);
                    nb.update(shipPos);
                    Framebuffer fb;
                    SpaceContext c;
                    c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos;
                    c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(fwd.y));
                    c.bankBodyA = bi;
                    sr.setupPalette(fb, &sys, bi, -1, 1.0);
                    sr.render(fb, c);
                    fb.mush(2);
                    saveFB(fb, "shots/tests/space_comet.png");
                    printf("comet %s: a=%.3g km e=%.2f q=%.3g km period %.1f d, at periapsis %.3g km from the star, %.0f km/s -> shots/space_comet.png\n", b.name.c_str(), b.orbitRadiusKm, b.ecc,
                           b.orbitRadiusKm * (1 - b.ecc), b.orbitPeriod / 86400, length(bp - s.pos), length(sys.bodyVel(bi, t)));
                    // N0-01 (B-201): the tail streams (frames 5 s apart differ, the knots moved), the comet drifts against
                    // the sky at x100 seen from the first planet, and the parking view shows the jets moving
                    {
                        const uint64_t FNV0 = 1469598103934665603ULL;
                        uint64_t h0 = fnv(FNV0, fb.idx.data(), fb.idx.size() * sizeof(Pix));
                        Framebuffer fb5; SpaceContext c5 = c; c5.t = t + 5;
                        sr.setupPalette(fb5, &sys, bi, -1, 1.0); sr.render(fb5, c5); fb5.mush(2);
                        uint64_t h5 = fnv(FNV0, fb5.idx.data(), fb5.idx.size() * sizeof(Pix));
                        printf("  tail 5 s later: frame %s; knot 0 at %.3f -> %.3f of the tail\n", h0 != h5 ? "differs" : "IDENTICAL (FAIL)", SpaceRenderer::cometKnot(0, t), SpaceRenderer::cometKnot(0, t + 5));
                        int obs = -1;
                        for (int j = 0; j < (int)sys.bodies.size(); j++) if (sys.bodies[j].parent < 0 && sys.bodies[j].type != PT_COMET && sys.bodies[j].type != PT_COMPANION) { obs = j; break; }
                        if (obs >= 0) {
                            Vec3 d0 = normalize(bp - sys.bodyPos(obs, t)), d1 = normalize(sys.bodyPos(bi, t + 500) - sys.bodyPos(obs, t + 500));
                            double ang = std::acos(clampd(dot(d0, d1), -1, 1));
                            double px = ang / (70 * DEG / 320);
                            printf("  seen from %s at x100 for 500 s: moved %.2f deg = %.1f px at 1x (%s)\n", sys.bodies[obs].name.c_str(), ang / DEG, px, px > 4 ? "ok" : "FAIL");
                        }
                        Vec3 toStar = normalize(s.pos - bp);
                        Vec3 side2 = normalize(cross(toStar, Vec3(0, 1, 0)));
                        Vec3 parkDir = normalize(toStar * 0.55 + side2 * 0.75 + Vec3(0, 0.18, 0));   // as startApproach parks
                        Vec3 sp = bp + parkDir * (b.radiusKm * 12.0);
                        Vec3 f2 = normalize(bp - sp);
                        nb.update(sp);
                        Framebuffer fbc; SpaceContext cc;
                        cc.sys = &sys; cc.stars = &nb.stars; cc.t = t; cc.shipPos = sp; cc.cam = cameraBasis(std::atan2(f2.x, f2.z), std::asin(f2.y)); cc.bankBodyA = bi;
                        warmMaps(sr, sys, bi);
                        sr.setupPalette(fbc, &sys, bi, -1, 1.0); sr.render(fbc, cc); fbc.mush(2);
                        saveFB(fbc, "shots/tests/space_comet_close.png");
                        uint64_t hc0 = fnv(FNV0, fbc.idx.data(), fbc.idx.size() * sizeof(Pix));
                        double nucPx = 2 * sr.bodyInfo[bi].radiusPx;
                        Framebuffer fbc3; SpaceContext cc3 = cc; cc3.t = t + 3;
                        cc3.shipPos = sys.bodyPos(bi, t + 3) + parkDir * (b.radiusKm * 12.0);   // parked: the ship rides along
                        sr.setupPalette(fbc3, &sys, bi, -1, 1.0); sr.render(fbc3, cc3); fbc3.mush(2);
                        uint64_t hc3 = fnv(FNV0, fbc3.idx.data(), fbc3.idx.size() * sizeof(Pix));
                        int changed = 0;
                        for (size_t k = 0; k < fbc.idx.size(); k++) if (fbc.idx[k] != fbc3.idx[k]) changed++;
                        (void)hc0; (void)hc3;
                        printf("  parked at 12 radii (%.0f km): nucleus %.0f px across -> shots/space_comet_close.png; 3 s later %d px changed (jets %s)\n", b.radiusKm * 12, nucPx, changed, changed > 500 ? "stream" : "DO NOT MOVE (FAIL)");
                    }
                    cometDone = true;
                }
        }
}

// ---- surface tests ------------------------------------------------------------
#include "surface/surface_view.h"
#include "core/input.h"

// find a time when the sun altitude at the site is close to the target (radians)
static double findTime(SurfaceSite& site, double targetAlt, double t0) {
    double best = t0, bestErr = 1e9;
    double period = std::fabs(site.sys->bodies[site.body].rotPeriod);
    for (int i = 0; i < 720; i++) {
        double t = t0 + period * i / 720.0;
        double alt = site.sun(t).altitude;
        double err = std::fabs(alt - targetAlt);
        if (err < bestErr) { bestErr = err; best = t; }
    }
    return best;
}

static void renderSurface(int onlyType) {
    SpaceRenderer sr;
    StarNeighborhood nb;
    bool done[PT_COUNT] = {false};
    int remaining = 0;
    for (int i = 0; i < PT_COUNT; i++) if (PLANET_TYPES[i].landable && (onlyType < 0 || i == onlyType)) remaining++; else done[i] = true;
    for (int64_t x = 150; x < 320 && remaining; x++)
        for (int64_t z = 20; z < 140 && remaining; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && remaining; bi++) {
                const Body& b = sys.bodies[bi];
                if (done[b.type]) continue;
                done[b.type] = true; remaining--;
                std::string tn = PLANET_TYPES[b.type].name;
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
                    Framebuffer fb;
                    double t0 = nowSec();
                    sv.render(fb, t, nb.stars, sr, 1.0);
                    double t1 = nowSec();
                    fb.mush(2);
                    std::string fn = "shots/tests/surf_" + tn + "_" + sc.tag + ".png";
                    saveFB(fb, fn.c_str());
                    printf("%-16s %-7s %s lat=%.0f sunAlt=%.1f az=%.0f time=%.2f render=%.1f ms  T=%.0fC P=%.2f g=%.1f cloud=%.2f rain=%.2f snow=%.2f sky=%.2f uw=%d rocks=%d/%d\n",
                           PLANET_TYPES[b.type].name, sc.tag, b.name.c_str(), sc.latDeg, si.altitude / DEG, si.azimuth / DEG, si.dayFraction,
                           (t1 - t0) * 1000, sv.env.temperatureC, sv.env.pressureAtm, sv.site.gravity, sv.env.cloudCover, sv.env.rain, sv.env.snow, sv.env.skyBrightness, (int)sv.player.underwater, sv.lastRocksDrawn, sv.lastRocksCandidates);
                }
            }
        }
}

// ---- scripted game flow --------------------------------------------------------
#include "game/game.h"
#include "game/ui.h"
static void applyTestScale(Game& g) { g.settings.renderScale = g_testScale; g.applySettings(); }

static bool firstBodyOfType(int type, StarSystem& sysOut, int& biOut);   // O6-03: the first body of a type in the scan region (defined with forTypeBodies below)
static void runGameFlow() {
    Game game;
    game.savePrefix = "shots/tests/test_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.guidePath = "shots/tests/test_guide.txt";
    game.keysPath = "shots/tests/test_keys.txt";
    game.shotsDir = "shots/tests/test_gallery";
    game.moviesDir = "shots/tests/test_movies"; game.recFixedTake = true;   // M6-05: the recorder writes into shots/, take 1 overwritten
    remove("shots/tests/test_guide.txt");
    applyTestScale(game);
    Input in;
    auto run = [&](double seconds) {
        int n = (int)(seconds * 30);
        for (int i = 0; i < n; i++) { game.frame(in, 1.0 / 30); if (game.recording) game.recordFrame(); in.newFrame(); }
    };
    auto press = [&](int key) { in.pressed[key] = true; in.down[key] = true; game.frame(in, 1.0 / 30); in.newFrame(); in.down[key] = false; };
    auto shot = [&](const char* name) {
        std::string fn = std::string("shots/tests/flow_") + name + ".png";
        writePNG(fn.c_str(), game.output(), FBW, FBH);
        printf("%-14s state=%d status='%s'\n", name, (int)game.state, game.testStatus().c_str());
    };
    double t0 = nowSec();
    run(0.5); shot("title");
    game.testAttract(30); run(0.3); shot("attract");   // M6-06
    press(KEY_C); run(0.1); shot("credits"); press(KEY_ESCAPE); run(0.1);
    press(KEY_ENTER); run(1.0); shot("space"); printf("  %s\n", game.testDebugInfo().c_str());
    // M2: walk to the GOES console, use it, then the front computer, the roof and the glass hull
    game.testCabinGoto(-1.2, 0.0, -PI / 2); run(0.2); shot("cabin"); printf("  facing %d\n", game.testCabinFacing());
    press(KEY_E); run(0.1); game.testTypeText("SL 4"); run(0.1); shot("console"); press(KEY_ESCAPE);
    game.testCabinGoto(0.0, 1.3, 0.0); run(0.2); press(KEY_E); run(0.1); shot("shipscreen"); press(KEY_ESCAPE);
    press(KEY_PAGE_UP); run(0.3); shot("roof"); press(KEY_PAGE_DOWN); run(0.1);
    press(KEY_Y); press(KEY_U); game.testCabinGoto(0.4, -1.6, 0.3); run(0.6); shot("cabin_glass"); press(KEY_Y); press(KEY_U); run(0.1);
    press(KEY_TAB); run(0.1); shot("list"); press(KEY_ESCAPE);
    press(KEY_I); run(0.1); shot("data"); press(KEY_SPACE);
    press(KEY_H); run(0.1); shot("help"); press(KEY_ESCAPE);
    press(KEY_O); run(0.5);
    // M3: rename the star through the guide, look at the star map
    press(KEY_G); run(0.1); shot("guide"); press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("text_entry");
    for (int i = 0; i < 20; i++) press(KEY_BACKSPACE);
    game.testTypeText("HOME OF CATS"); run(0.1);
    printf("  renamed star: '%s'\n", game.testStarName().c_str());
    press(KEY_M); run(0.3); shot("starmap"); press(KEY_ESCAPE);
    press(KEY_R); press(KEY_N); run(0.2); shot("targeting");
    press(KEY_ENTER); run(0.2);
    press(KEY_V); run(3.0); shot("vimana");
    run(12.0); shot("arrived");
    int lb = game.testLandableBody();
    printf("landable body %d of %s\n", lb, game.testBodyTypes().c_str());
    if (lb >= 0 && game.testBodyType(lb) == PT_COMET) {
        // O6-03: the nearest system holds nothing but comets since GEN 9; the walk, the buggy and the water below need a world
        // with gravity: park at the first felisian world of the scan instead (the approach frames come from the comet section)
        StarSystem fs; int fb = -1;
        if (firstBodyOfType(PT_FELISIAN, fs, fb)) { game.testParkAt(fs.star, fb); lb = fb; run(0.5); printf("  only comets here: parked at %s body %d for the landing\n", fs.star.name.c_str(), fb); }
    }
    if (lb >= 0) {
        if (game.testBodyType(lb) != PT_FELISIAN || game.testStarGenName().find("Skeatoltdos") == std::string::npos) {
            game.testAimAtBody(lb);
            press(KEY_TAB); run(0.1);
            for (int i = 0; i < lb; i++) press(KEY_DOWN);
            press(KEY_ENTER); run(2.0); shot("approach");
            run(9.0); shot("orbit"); printf("  %s\n", game.testDebugInfo().c_str());
            press(KEY_C); run(0.2);
        }
        shot("landing_map");
        press(KEY_R); run(0.1); shot("landing_map2");
        press(KEY_Z); run(0.3); shot("landing_zoom"); press(KEY_Z); run(0.1);   // O2 (R-301): the sector zoom
        press(KEY_J); run(0.1); shot("landing_sky");   // M5-09
        press(KEY_ENTER); run(2.0); shot("descent");
        game.currentSlot = 2; in.down[KEY_LEFT_CONTROL] = true; press(KEY_S); in.down[KEY_LEFT_CONTROL] = false;   // save mid-descent to slot 2
        game.currentSlot = 1;
        run(6.0); shot("surface");
        printf("  %s\n", game.testRangeInfo().c_str());   // O1: the rangefinder
        in.down[KEY_W] = true; run(3.0); in.down[KEY_W] = false;
        in.mouseDx = 400; game.frame(in, 1.0 / 30); in.newFrame();
        run(0.5); shot("surface2");
        // M8: sprint, jump + jetpack, the buggy
        in.down[KEY_W] = true; in.down[KEY_LEFT_SHIFT] = true; run(2.5); shot("sprint"); in.down[KEY_LEFT_SHIFT] = false; in.down[KEY_W] = false;
        printf("  sprint: stamina %.0f, speed %.1f m/s\n", game.testPlayerStamina(), game.testPlayerSpeed());
        press(KEY_SPACE); in.down[KEY_SPACE] = true; run(2.5); shot("jetpack"); in.down[KEY_SPACE] = false;
        printf("  jetpack: %.1f m above ground\n", game.testPlayerAlt());
        run(4.0);
        press(KEY_K); run(0.3);                      // capsule next to us
        press(KEY_B); run(2.5);                      // unfold
        game.testWalkToBuggy();
        press(KEY_E); run(0.2); shot("buggy_seat");
        // N4-02: a long drive on open ground; frames at speed, airborne if it happens, the chase view
        {
            bool airShot = false; double top = 0;
            double openRun = game.testDriveOpen();
            if (openRun > 0) printf("  open run of %.0f m found for the drive\n", openRun);
            else printf("  no open run near this landing (forest): a short smoke drive; vesperis_test drive is the long one\n");
            in.down[KEY_W] = true;
            for (int i = 0; i < 30 * 45; i++) {
                game.frame(in, 1.0 / 30); in.newFrame();
                if (i == 30 * 8) { in.down[KEY_D] = true; }
                if (i == 30 * 9) { in.down[KEY_D] = false; }
                if (i == 30 * 20) shot("buggy_drive");
                top = std::max(top, game.testBuggySpeed());
                if (!airShot && game.testBuggyAirborne()) { shot("buggy_air"); airShot = true; }
            }
            printf("  drive: %s, top %.0f km/h%s\n", game.testBuggyInfo().c_str(), top * 3.6, airShot ? ", airborne once" : "");
        }
        press(KEY_V); run(0.2); shot("buggy_chase"); in.down[KEY_W] = false; run(1.5);
        printf("  buggy: %s\n", game.testBuggyInfo().c_str());
        press(KEY_E); run(0.5);
        {   // B-205 / R-204: the capsule always comes; B at it unfolds a new buggy and scraps the old one
            press(KEY_K); run(0.3);
            bool recalled = game.testStatus().find("RECALLED") != std::string::npos;
            press(KEY_B); run(0.3);
            bool scrapped = game.testStatus().find("SCRAPPED") != std::string::npos;
            printf("  recall with the buggy out: %s; B redeploys: %s (%s)\n", recalled ? "ok" : "FAIL", scrapped ? "ok" : "FAIL", game.testBuggyInfo().c_str());
        }
        press(KEY_M); run(0.2);
        press(KEY_N); run(0.2); shot("sectormap");
        for (int i = 0; i < 5; i++) {   // O1-04: out to the widest level this world allows, a frame per level (B-402: the grid's step)
            press(KEY_MINUS); run(0.3);
            if (i < 4) shot((std::string("sectormap_z") + std::to_string(i + 1)).c_str());
        }
        shot("sectormap_wide"); printf("  %s\n", game.testMarksInfo().c_str());
        for (int i = 0; i < 5; i++) press(KEY_EQUAL);
        press(KEY_ESCAPE);
        press(KEY_X); run(0.2); shot("creatures");
        // N3-03: walk up to the first herd and log how it reacts
        if (game.testGotoHerd(34)) {
            run(0.5); shot("herd_approach"); printf("  herd before: %s\n", game.testHerdStates().c_str());
            in.down[KEY_W] = true; run(4.0); in.down[KEY_W] = false; run(0.5);
            press(KEY_X); run(0.1); shot("herd_flee"); printf("  herd after walking in: %s\n", game.testHerdStates().c_str());
        } else printf("  no herd at this site\n");
        // M6-05 photo tools: photo mode with the free camera, a panorama, three recorded frames
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_P); in.down[KEY_LEFT_CONTROL] = false; run(0.1); shot("photo");
        in.down[KEY_W] = true; in.down[KEY_SPACE] = true; run(0.6); in.down[KEY_W] = false; in.down[KEY_SPACE] = false; shot("photo_flight");
        {
            std::vector<uint32_t> pano; int pw, ph;
            game.renderPanorama(pano, pw, ph);
            writePNG("shots/tests/flow_panorama.png", pano.data(), pw, ph);
            printf("panorama %dx%d -> shots/flow_panorama.png\n", pw, ph);
        }
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_P); in.down[KEY_LEFT_CONTROL] = false; run(0.1);
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_R); in.down[KEY_LEFT_CONTROL] = false; run(0.1);
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_R); in.down[KEY_LEFT_CONTROL] = false;
        printf("recorder: take %d, %d frames%s\n", game.recTake, game.recFrame, game.recFrame >= 3 ? " (ok)" : " FAIL");
        in.down[KEY_LEFT_CONTROL] = true; press(KEY_T); in.down[KEY_LEFT_CONTROL] = false; run(0.5); shot("timelapse");   // M5-06
        press(KEY_T); run(0.1);   // back to x1
        press(KEY_V); press(KEY_V); run(0.1); shot("supervision"); press(KEY_V); press(KEY_V); press(KEY_V);
        // M3: a note, the log, the statistics and the gallery
        press(KEY_G); press(KEY_DOWN); press(KEY_DOWN); press(KEY_DOWN); press(KEY_ENTER); game.testTypeText("STRANGE RED TREES HERE"); run(0.1);
        press(KEY_J); run(0.1); shot("log"); press(KEY_ESCAPE);
        press(KEY_G); for (int i = 0; i < 6; i++) press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("stats"); press(KEY_ESCAPE);
        mkdir("shots/tests/test_gallery", 0755);
        writePNG("shots/tests/test_gallery/screenshot_0.png", game.output(), FBW, FBH);
        { FILE* sc = fopen("shots/tests/test_gallery/screenshot_0.txt", "w"); if (sc) { fprintf(sc, "%s\n", game.screenshotCaption().c_str()); fclose(sc); } }
        press(KEY_G); for (int i = 0; i < 5; i++) press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("gallery"); press(KEY_ESCAPE);
        printf("  log entries: %d\n", game.testLogEntries());
        for (int i = 0; i < 3; i++) press(KEY_T);
        run(4.0); shot("surface3");
        press(KEY_I); run(0.1); shot("surface_data"); press(KEY_SPACE);
        press(KEY_ESCAPE); run(0.1); shot("menu");
        press(KEY_DOWN); press(KEY_DOWN); press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("settings"); press(KEY_ESCAPE); press(KEY_ESCAPE);
        press(KEY_ESCAPE); press(KEY_DOWN); press(KEY_ENTER); run(0.1); shot("slots"); press(KEY_ENTER); run(0.1);   // menu > save > slot 1
        press(KEY_K); run(0.5);
        press(KEY_Q); run(3.0); shot("ascent");
        run(6.0); shot("back_in_space");
        {   // B-205: a second landing (same world, another site) starts with no buggy
            printf("  before the second landing: %s\n", game.testMarksInfo().c_str());
            press(KEY_C); run(0.3); press(KEY_ENTER); run(8.0); shot("second_landing");
            bool none = game.testBuggyInfo().find("deployed=0") != std::string::npos;
            printf("  second landing: %s -> %s\n", game.testBuggyInfo().c_str(), none ? "no buggy (ok)" : "FAIL: the buggy carried over");
            std::string marks = game.testMarksInfo();   // B-303: the trail and the waypoint of the first landing must be gone
            bool fresh = marks.find("trail=1 ") != std::string::npos && marks.find("waypoint=0") != std::string::npos;
            press(KEY_N); run(0.2); shot("second_sectormap"); press(KEY_ESCAPE);
            printf("  marks after the second landing: %s -> %s\n", marks.c_str(), fresh ? "fresh (ok)" : "FAIL: the old landing's marks carried over");
            press(KEY_K); run(0.3); press(KEY_Q); run(9.0);   // back to space for the rest of the flow
        }
    }
    // N0-01 (B-201): park at the first comet near the start; the HUD shows its speed and plunge, the data sheet its distance now
    {
        bool found = false;
        for (int64_t x = 150; x < 320 && !found; x++)
            for (int64_t z = 20; z < 140 && !found; z++) {
                Star s;
                if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && !found; bi++) {
                    if (sys.bodies[bi].type != PT_COMET) continue;
                    game.testParkAt(s, bi); run(0.5); shot("comet_map");   // O4: comets are landable, so parking opens the landing map
                    press(KEY_ESCAPE); run(0.3); shot("comet");
                    printf("  comet HUD: %s\n", game.cometMotionString(bi).c_str());
                    press(KEY_I); run(0.1); shot("comet_data"); press(KEY_SPACE);
                    press(KEY_TAB); run(0.1); shot("comet_list"); press(KEY_ESCAPE);
                    // O4 (R-303): land on the nucleus: microgravity, no buggy, floating jumps that Ctrl brings down, the tail overhead
                    press(KEY_C); run(0.2); press(KEY_ENTER); run(8.0); shot("comet_surface");
                    printf("  on the comet: escape velocity %.1f m/s, %s, '%s'\n", game.testEscapeVelocity(), game.testRangeInfo().c_str(), game.testStatus().c_str());
                    press(KEY_B); run(0.2); printf("  B on the comet: '%s'\n", game.testStatus().c_str());
                    in.down[KEY_W] = true; run(3.0); printf("  walking at %.2f m/s (the cap is %.2f)\n", game.testPlayerSpeed(), 0.3 * game.testEscapeVelocity()); in.down[KEY_W] = false;
                    press(KEY_SPACE); run(4.0); shot("comet_jump"); double up = game.testPlayerAlt();
                    in.down[KEY_LEFT_CONTROL] = true; run(3.0); in.down[KEY_LEFT_CONTROL] = false;
                    printf("  a jump: %.2f m up after 4 s, %.2f m after 3 s of Ctrl\n", up, game.testPlayerAlt());
                    in.mouseDy = -700; game.frame(in, 1.0 / 30); in.newFrame(); run(0.2); shot("comet_sky");
                    in.mouseDy = 700; game.frame(in, 1.0 / 30); in.newFrame();
                    press(KEY_N); run(0.2); shot("comet_sectormap"); press(KEY_ESCAPE);
                    press(KEY_K); run(0.3); press(KEY_Q); run(9.0);
                    found = true;
                }
            }
    }
    // O3 (R-302): fly to the first belt near the start, park beside one of its rocks; the capsule refuses; the sheet
    {
        bool found = false;
        for (int64_t x = 150; x < 320 && !found; x++)
            for (int64_t z = 20; z < 140 && !found; z++) {
                Star s;
                if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                if (sys.belts.empty()) continue;
                game.testParkAt(s, -1); run(0.3);
                press(KEY_TAB); run(0.1);
                for (int i = 0; i < (int)sys.bodies.size(); i++) press(KEY_DOWN);
                shot("belt_list");
                press(KEY_ENTER); run(3.0); shot("belt_approach");
                run(12.0); shot("belt");
                printf("  belt: %s; parked belt %d; '%s'\n", game.testDebugInfo().c_str(), game.testParkedBelt(), game.testStatus().c_str());
                press(KEY_I); run(0.1); shot("belt_data"); press(KEY_SPACE);
                press(KEY_C); run(0.2); printf("  C in the belt: '%s'\n", game.testStatus().c_str());
                press(KEY_X); run(0.3); shot("belt_rock");
                printf("  %s\n", game.testBeltDraw().c_str());
                found = true;
            }
        if (!found) printf("belt: none found\n");
    }
    Game g2; g2.savePrefix = "shots/tests/test_save"; g2.settingsPath = "shots/tests/test_settings.txt"; g2.guidePath = "shots/tests/test_guide.txt";
    g2.guide.load(g2.guidePath);
    printf("load: %d (newest -> slot %d, state %d)\n", (int)g2.loadNewest(), g2.currentSlot, (int)g2.state);
    printf("  guide reloaded: %zu names, %d log entries, star '%s'\n", g2.guide.names.size(), g2.testLogEntries(), g2.testStarName().c_str());
    g2.frame(in, 1.0 / 30);
    writePNG("shots/tests/flow_loaded.png", g2.output(), FBW, FBH);
    Game g3; g3.savePrefix = "shots/tests/test_save"; g3.settingsPath = "shots/tests/test_settings.txt"; g3.guidePath = "shots/tests/test_guide.txt";
    bool ok3 = g3.loadSlot(2);
    int st0 = (int)g3.state;
    for (int i = 0; i < 30 * 8; i++) { g3.frame(in, 1.0 / 30); in.newFrame(); }
    printf("load slot 2 (mid-descent): %d state %d -> after 8 s state %d; autosave: '%s'\n", (int)ok3, st0, (int)g3.state, g3.saveSummary(g3.slotPath(0)).c_str());
    printf("flow done in %.1f s\n", nowSec() - t0);
}

// N0-02 (B-202): the diagnosis sheet. For every landable type: the landing map around the sub-solar point,
// the globe patch at the same point seen from 3.5 radii, and the ground there at noon looking down 30 deg.
// Prints the average colours and their RGB distances (0-441); N1 has to bring them under the threshold.
static void runConsistency() {
    SpaceRenderer sr; StarNeighborhood nb;
    Game game;
    game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/test_guide.txt";
    Input in;
    bool done[PT_COUNT] = {false};
    int remaining = 0;
    for (int i = 0; i < PT_COUNT; i++) if (PLANET_TYPES[i].landable) remaining++; else done[i] = true;
    printf("%-14s %-13s %-13s %-13s %6s %6s %6s\n", "type", "map", "globe", "ground", "m-g", "g-s", "m-s");
    double worst = 0; int over = 0; const double THRESH = 60;
    for (int64_t x = 150; x < 320 && remaining; x++)
        for (int64_t z = 20; z < 140 && remaining; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
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
                int wantBank = SpaceRenderer::globeBank(2, want);
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
                std::string tn = PLANET_TYPES[b.type].name;
                for (char& ch : tn) if (ch == ' ') ch = '_';
                std::string fn = "shots/tests/consistency_" + tn + ".png";
                writePNG(fn.c_str(), sheet.data(), SW, SH);
                double dmg = dist(0, 1), dgs = dist(1, 2), dms = dist(0, 2);
                worst = std::max(worst, std::max(dmg, std::max(dgs, dms)));
                if (b.type == PT_VENUSIAN) { if (dms > THRESH) over++; }   // the globe is its cloud deck; the map (C) and the ground are the same basalt
                else if (dmg > THRESH || dgs > THRESH || dms > THRESH) over++;
                // N1-06: the landing map's relief class against the slope measured on the real terrain
                const char* cls = game.testReliefClass(lat, lon);
                double slope = 0;   // O6-02: the regional grade of Game::siteSlope, on the ground's own caches
                for (int k = 0; k < 4; k++) {
                    double dx = (k & 1) ? 0 : (k ? -1 : 1), dz = (k & 1) ? (k == 1 ? 1 : -1) : 0;
                    slope += std::fabs(sv.site.groundHeight(dx * 150, dz * 150) - sv.site.groundHeight(0, 0)) / 150.0 / 8;
                    slope += std::fabs(sv.site.groundHeight(dx * 600, dz * 600) - sv.site.groundHeight(0, 0)) / 600.0 / 8;
                }
                slope /= 0.64;
                // the map probes points of the function, the ground reads its caches: the classes agree, or the two grades are within 30% (a boundary case)
                double mapSlope = Game::siteSlope(sv.site);
                bool reliefOk = std::string(cls) == reliefClass(slope) || std::fabs(mapSlope - slope) <= 0.3 * std::max(mapSlope, slope);
                printf("%-14s %3.0f,%3.0f,%3.0f   %3.0f,%3.0f,%3.0f   %3.0f,%3.0f,%3.0f   %6.0f %6.0f %6.0f  %s (%s, sun %.0f deg, light %.2f (globe %.2f), %s slope %.3f%s, %s)\n", PLANET_TYPES[b.type].name,
                       avg[0][0], avg[0][1], avg[0][2], avg[1][0], avg[1][1], avg[1][2], avg[2][0], avg[2][1], avg[2][2], dmg, dgs, dms,
                       b.name.c_str(), MATERIAL_NAMES[sv.site.sampleAt(0, 0, 64).material], sv.site.sun(t).altitude / DEG, sv.site.sun(t).lightFactor, SpaceRenderer::lightFactor(sys.star.luminosity, length(sys.star.pos - bp)),
                       cls, slope, reliefOk ? "" : " RELIEF MISMATCH", fn.c_str());
                printf("%-14s ground crop: mean shade %.1f, bank %d (%d%% of the crop); ramp stops 14/34/47 = %d,%d,%d / %d,%d,%d / %d,%d,%d\n", "", shadeSum / (T * T), groundBank, bankHist[groundBank] * 100 / (T * T),
                       fbs.pal[(groundBank * 64 + 14) * 3], fbs.pal[(groundBank * 64 + 14) * 3 + 1], fbs.pal[(groundBank * 64 + 14) * 3 + 2],
                       fbs.pal[(groundBank * 64 + 34) * 3], fbs.pal[(groundBank * 64 + 34) * 3 + 1], fbs.pal[(groundBank * 64 + 34) * 3 + 2],
                       fbs.pal[(groundBank * 64 + 47) * 3], fbs.pal[(groundBank * 64 + 47) * 3 + 1], fbs.pal[(groundBank * 64 + 47) * 3 + 2]);
                if (!reliefOk) over += 100;
            }
        }
    printf("consistency: %d of %d types over the threshold of %.0f (venusian: map against ground only); worst distance %.0f%s\n", over % 100, landableTypeCount(), THRESH, worst, over >= 100 ? "; RELIEF CLASS MISMATCH" : "");
}

static bool setupSceneForType(int type, double latDeg, double alt, double yawOff, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut, int wantMat);

// N4-02: a drive on open grassland through the surface renderer: 60 s at full throttle, frames from the seat, airborne and chase
static int runDrive() {
    SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
    // a desert (no trees, no rocks on sand) first, else open grassland
    bool haveSite = setupSceneForType(PT_FELISIAN, 10, 40 * DEG, 0.6, 0.0, sv, sys, nb, t, -10 - BIO_DESERT);
    const char* where = "desert";
    if (!haveSite) { haveSite = setupSceneForType(PT_FELISIAN, 12, 40 * DEG, 0.6, 0.0, sv, sys, nb, t, -9); where = "grassland"; }
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
            for (double d = 10; d <= look && !steer; d += 5) if (sv.site.groundHeight(sv.buggy.x + hx * d, sv.buggy.z + hz * d) < sv.site.waterAt(sv.buggy.x + hx * d, sv.buggy.z + hz * d) - 0.3) steer = steerDir;
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
        if (i % 300 == 299) printf("  %2d s: %5.1f m/s, %6.0f m, gear %d, skid %.2f, %.0f m along the run, dir %+d%s\n", (i + 1) / 30, sv.buggy.speed, sv.buggy.odometer, sv.buggy.gear, sv.buggy.skid, along, dir, sv.buggy.airborne ? ", airborne" : "");
        if (i > 30 * 20 && std::fabs(sv.buggy.speed) < 0.5 && turnT <= 0) stalled = true;
    }
    {   // a night frame with the headlights on
        double tn = t + std::fabs(sys.bodies[sv.site.body].rotPeriod) * 0.5;
        Input in2; sv.update(1.0 / 30, in2, tn, true);
        sv.render(fb, tn, nb.stars, sr, 1.0); fb.mush(2); sv.cameraFeed(fb, tn); saveFB(fb, "shots/tests/drive_night.png");
        sv.chaseCam = true; sv.render(fb, tn, nb.stars, sr, 1.0); fb.mush(2); saveFB(fb, "shots/tests/drive_night_chase.png"); sv.chaseCam = false;
    }
    printf("drive: %.0f m in 90 s, top %.0f km/h, render %.1f ms mean / %.1f worst at 1x%s%s; frames drive_seat, drive_chase, drive_night[_chase]\n", sv.buggy.odometer, top * 3.6, sumMs / std::max(1, n), worstMs, airShot ? ", airborne once (shots/drive_air.png)" : "", stalled ? " (STALLED at some point)" : "");
    bool ok = top * 3.6 > 100 && sv.buggy.odometer > 2000;
    printf("drive: %s\n", ok ? "ok (over 2 km, over 100 km/h)" : "FAIL: under 2 km or under 100 km/h");
    return ok ? 0 : 1;
}

// N3-01: the bestiary of the first living worlds near the start, and a determinism check
static int runBestiary() {
    int shown = 0, fails = 0;
    for (int64_t x = 150; x < 320 && shown < 6; x++)
        for (int64_t z = 20; z < 140 && shown < 6; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && shown < 6; bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != PT_FELISIAN && b.type != PT_OCEAN) continue;
                BodyGen g = BodyGen::make(b);
                Bestiary B = Bestiary::make(b, g), B2 = Bestiary::make(b, g);
                printf("%s (%s): %d land, %d flyers, %d swimmers\n", b.name.c_str(), PLANET_TYPES[b.type].name, B.landCount, B.flyerCount, B.swimmerCount);
                for (size_t i = 0; i < B.species.size(); i++) {
                    const Species& sp = B.species[i];
                    printf("  %-12s %-20s %4.1f m  gait %d/%d  bank %2d tone %.2f pattern %d crest %d  habitat %-16s diet %d  %-8s %-11s call %d x%.2f  herd %d-%d\n", sp.name.c_str(), PLAN_NAMES[sp.plan], sp.size, sp.gait, sp.runGait,
                           sp.bank, sp.tone, sp.pattern, sp.crest, sp.habitat ? BIOME_NAMES[sp.habitat] : "ANY", sp.diet, ACTIVITY_NAMES[sp.activity], TEMPERAMENT_NAMES[sp.temperament], sp.call, sp.callPitch, sp.herdMin, sp.herdMax);
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
static void runEncounters(int wanted) {
    setDrainageEnabled(false);   // O6-03: hundreds of sites over the globe, a tile each; the herds do not need the rivers
    int total = 0, withLife = 0, cellHerds = 0, cellAnimals = 0;
    int perBiome[BIO_COUNT] = {0}, lifeBiome[BIO_COUNT] = {0};
    Rng r(0xE7C0);
    for (int64_t x = 150; x < 320 && total < wanted; x++)
        for (int64_t z = 20; z < 140 && total < wanted; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
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
                    {   // B-316: the habitat cells within 900 m of the landing
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
    printf("encounters: a herd within 200 m at %d of %d felisian land sites (%.0f%%); B-316: %.1f habitat herds (%.0f animals) within 900 m per site\n", withLife, total, total ? 100.0 * withLife / total : 0.0, total ? (double)cellHerds / total : 0.0, total ? (double)cellAnimals / total : 0.0);
    for (int bm = 0; bm < BIO_COUNT; bm++) if (perBiome[bm]) printf("  %-16s %3d sites, %3.0f%%\n", BIOME_NAMES[bm], perBiome[bm], 100.0 * lifeBiome[bm] / perBiome[bm]);
}

// One landing-map screen per landable planet type (bug B-004 check).
static void renderLandingMaps() {
    Game game;
    game.savePrefix = "shots/tests/test_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.guidePath = "shots/tests/test_guide.txt";   // every harness Game keeps off the real guide.txt (2026-09-27)
    applyTestScale(game);
    Input in;
    bool done[PT_COUNT] = {false};
    int remaining = 0;
    for (int i = 0; i < PT_COUNT; i++) if (PLANET_TYPES[i].landable) remaining++; else done[i] = true;
    for (int64_t x = 150; x < 320 && remaining; x++)
        for (int64_t z = 20; z < 140 && remaining; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && remaining; bi++) {
                const Body& b = sys.bodies[bi];
                if (done[b.type]) continue;
                done[b.type] = true; remaining--;
                game.testParkAt(s, bi);
                for (int f = 0; f < 3; f++) { game.frame(in, 1.0 / 30); in.newFrame(); }
                std::string tn = PLANET_TYPES[b.type].name;
                for (char& ch : tn) if (ch == ' ') ch = '_';
                std::string fn = "shots/tests/landmap_" + tn + ".png";
                writePNG(fn.c_str(), game.output(), FBW, FBH);
                printf("landing map %-16s %s state=%d, darkest shadow on the map now %.2f\n", PLANET_TYPES[b.type].name, b.name.c_str(), (int)game.state, game.testMapShadowMax());   // O0-04
            }
        }
}

// Settings file round trip and the settings screen (M0-02).
static void testSettings() {
    Settings s;
    s.mouseSensitivity = 1.7; s.invertY = true; s.fovDeg = 85; s.masterVolume = 0.35; s.scanlines = false; s.windowScale = 3;
    s.save("shots/tests/test_settings.txt");
    Settings r;
    bool ok = r.load("shots/tests/test_settings.txt");
    bool same = ok && std::fabs(r.mouseSensitivity - 1.7) < 1e-9 && r.invertY && r.fovDeg == 85 && std::fabs(r.masterVolume - 0.35) < 1e-9 && !r.scanlines && r.windowScale == 3;
    printf("settings round trip: %s\n", same ? "ok" : "MISMATCH");
    Game game;
    game.savePrefix = "shots/tests/test_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.guidePath = "shots/tests/test_guide.txt";   // every harness Game keeps off the real guide.txt (2026-09-27)
    game.loadFromDisk();
    printf("loaded into game: fov=%.0f sens=%.1f scale=%d\n", game.settings.fovDeg, game.settings.mouseSensitivity, game.settings.windowScale);
    Input in;
    auto press = [&](int key) { in.pressed[key] = true; in.down[key] = true; game.frame(in, 1.0 / 30); in.newFrame(); in.down[key] = false; };
    press(KEY_ENTER); press(KEY_ESCAPE);
    press(KEY_DOWN); press(KEY_DOWN); press(KEY_DOWN); press(KEY_ENTER);
    press(KEY_DOWN); press(KEY_DOWN); press(KEY_RIGHT);   // field of view +5
    writePNG("shots/tests/settings.png", game.output(), FBW, FBH);
    press(KEY_ESCAPE);
    Settings r2; r2.load("shots/tests/test_settings.txt");
    printf("after +5 on the screen: fov=%.0f (file %.0f) state=%d\n", game.settings.fovDeg, r2.fovDeg, (int)game.state);
    Settings d;
    d.save("shots/tests/test_settings.txt");   // leave defaults for the other tests
}

// ---- determinism regression (M0-03) ----------------------------------------------
// Hashes generated content that must not change by accident: the first systems met in the
// start region, ten planet maps, and height profiles at three sites. `regress bless`
// rewrites tests/regress_baseline.txt; `regress` compares against it (exit code 1 on change).
static uint64_t fnv(uint64_t h, const void* p, size_t n) {
    const uint8_t* b = (const uint8_t*)p;
    for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ULL; }
    return h;
}
static void appendFrameHashes(std::vector<std::pair<std::string, uint64_t>>& lines, std::vector<std::string>& notes);
static int runRegress(bool bless) {
    const uint64_t FNV0 = 1469598103934665603ULL;
    std::vector<std::pair<std::string, uint64_t>> lines;
    std::vector<std::string> notes;
    int nsys = 0, nmap = 0, nsite = 0;
    for (int64_t x = 150; x < 260 && (nsys < 5 || nmap < 10 || nsite < 3); x++)
        for (int64_t z = 20; z < 120 && (nsys < 5 || nmap < 10 || nsite < 3); z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            if (nsys < 5) {
                uint64_t h = FNV0;
                h = fnv(h, s.name.data(), s.name.size()); h = fnv(h, &s.cls, sizeof s.cls); h = fnv(h, &s.radiusKm, 8); h = fnv(h, &s.luminosity, 8);
                for (auto& b : sys.bodies) {
                    h = fnv(h, b.name.data(), b.name.size()); h = fnv(h, &b.type, sizeof b.type); h = fnv(h, &b.radiusKm, 8);
                    h = fnv(h, &b.orbitRadiusKm, 8); h = fnv(h, &b.orbitPeriod, 8); h = fnv(h, &b.rotPeriod, 8); h = fnv(h, &b.axialTilt, 8);
                }
                lines.push_back({"system_" + std::to_string(nsys), h});
                notes.push_back(s.name + " (" + std::to_string(sys.bodies.size()) + " bodies)");
                nsys++;
            }
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (nmap < 10) {
                    PlanetMap m; m.generate(BodyGen::make(b));
                    uint64_t h = FNV0;
                    h = fnv(h, m.height.data(), m.height.size() * sizeof(float));
                    h = fnv(h, m.material.data(), m.material.size()); h = fnv(h, m.albedo.data(), m.albedo.size());
                    h = fnv(h, m.cloud.data(), m.cloud.size()); h = fnv(h, m.veg.data(), m.veg.size());
                    lines.push_back({"map_" + std::to_string(nmap), h});
                    notes.push_back(b.name + " " + PLANET_TYPES[b.type].name);
                    nmap++;
                }
                if (nsite < 3 && PLANET_TYPES[b.type].landable) {
                    SurfaceSite site;
                    site.init(&sys, bi, 0.3, 0.7, 0.0);
                    uint64_t h = FNV0;
                    for (int i = 0; i <= 80; i++) {
                        TerrainVertex v = site.sampleAt(-2000 + i * 50.0, 0.0, 16);
                        h = fnv(h, &v.h, 4); h = fnv(h, &v.albedo, 4); h = fnv(h, &v.material, 1);
                        TerrainVertex w = site.sampleAt(0.0, -2000 + i * 50.0, 16);
                        h = fnv(h, &w.h, 4); h = fnv(h, &w.albedo, 4); h = fnv(h, &w.material, 1);
                    }
                    lines.push_back({"site_" + std::to_string(nsite), h});
                    notes.push_back(b.name + " profile at 0.3 rad N 0.7 rad E");
                    nsite++;
                }
            }
        }
    appendFrameHashes(lines, notes);
    const char* path = "tests/regress_baseline.txt";
    if (bless) {
        std::ofstream f(path);
        f << "# vesperis determinism baseline; regenerate with ./vesperis_test regress bless\n";
        for (size_t i = 0; i < lines.size(); i++) f << lines[i].first << " " << std::hex << lines[i].second << std::dec << "  # " << notes[i] << "\n";
        printf("blessed %zu hashes into %s\n", lines.size(), path);
        return 0;
    }
    std::ifstream f(path);
    if (!f) { printf("no baseline at %s: run ./vesperis_test regress bless\n", path); return 1; }
    std::vector<std::pair<std::string, uint64_t>> base;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::string key; uint64_t v = 0;
        std::istringstream is(line);
        is >> key >> std::hex >> v;
        base.push_back({key, v});
    }
    int changed = 0, missing = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        const uint64_t* bv = nullptr;
        for (auto& b : base) if (b.first == lines[i].first) bv = &b.second;
        if (!bv) { printf("  %-10s new (no baseline)  %s\n", lines[i].first.c_str(), notes[i].c_str()); missing++; }
        else if (*bv != lines[i].second) { printf("  %-10s CHANGED %016llx -> %016llx  %s\n", lines[i].first.c_str(), (unsigned long long)*bv, (unsigned long long)lines[i].second, notes[i].c_str()); changed++; }
    }
    printf("regress: %zu hashes, %d changed, %d without baseline -> %s\n", lines.size(), changed, missing, (changed || missing) ? "FAIL" : "PASS");
    return (changed || missing) ? 1 : 0;
}

// ---- look comparison across resolution scales (M10-02) ---------------------------------
static bool setupPinnedMountainScene(double alt, double yawOff, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut);
static bool setupSceneForType(int type, double latDeg, double alt, double yawOff, double pitch, SurfaceView& sv, StarSystem& sys,
                              StarNeighborhood& nb, double& tOut, int wantMat = -1) {
    if (wantMat == -20) return setupPinnedMountainScene(alt, yawOff, pitch, sv, sys, nb, tOut);   // B-313: the pinned mountain site of the O6 review
    for (int64_t x = 150; x < 320; x++)
        for (int64_t z = 20; z < 140; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (type >= 0 && b.type != type) continue;
                if (type < 0 && (!PLANET_TYPES[b.type].landable || b.type == PT_COMET)) continue;   // R-307: a trait scene on any world
                double lon = 0.7 + bi * 0.3, latUse = latDeg * DEG;
                BodyGen g = BodyGen::make(b);
                if (wantMat == -36 && !g.hasTrait(TR_GEYSERS)) continue;
                // O6-03: the scans below sample a whole planet at 16 m: without the drainage (a tile per sample); the river and lake
                // finders (-21, -22) scan the flood's own cells instead (`drainageStats`) and turn it back on for the fine samples
                struct DrainOff { bool was; DrainOff() : was(drainageEnabled()) { setDrainageEnabled(false); } ~DrainOff() { setDrainageEnabled(was); } } drainOff;
                bool lookAtWater = false;
                double faceYawOut = 1e9;   // R-307: a scene finder may choose the view's direction
                if (wantMat == MAT_SAND) {
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
                    if (!found) continue;
                    lookAtWater = true;
                } else if (wantMat == -36) {
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
                    if (!found) continue;
                } else if (wantMat <= -30) {
                    // R-307: the new types' scenes: -30 a stained crack (europan), -31 a lava fissure (tectonic), -32 dunes
                    // (desert), -33 / -35 the shore of a methane or an acid sea, -34 a rayed plain (bombarded)
                    int mat = wantMat == -30 ? MAT_DUST : (wantMat == -31 ? MAT_LAVA : (wantMat == -32 ? MAT_SAND : MAT_WATER));
                    bool nearOnly = wantMat == -33 || wantMat == -35 || wantMat == -31;   // stand 60-150 m from it
                    bool found = false;
                    double faceYaw = 1e9;
                    double Rm = b.radiusKm * 1000.0;
                    for (double la = latDeg; la < latDeg + 40 && !found; la += 1.0)
                        for (int k = 0; k < 360 && !found; k++) {
                            double ln = k * TAU / 360;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (wantMat == -34) { if (ss.height > 0 && ss.albedo > 0.3) { lon = ln; latUse = la * DEG; found = true; } continue; }
                            if (ss.material != mat) continue;
                            if (wantMat == -30) {   // a narrow linea: ice 120 m out on both sides in some direction; stand on it 70 m off, facing the stain
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
                            for (int a = 0; a < 8 && !found; a++) {   // the nearest firm ground 60-150 m out
                                for (double dd = 60; dd <= 150 && !found; dd += 30) {
                                    double ang = a * TAU / 8;
                                    double dlat = std::cos(ang) * dd / Rm, dlon = std::sin(ang) * dd / (Rm * std::cos(la * DEG));
                                    SurfaceSample s2 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), 16);
                                    if (s2.material == mat || (s2.water > -1e8 && s2.height < s2.water + 0.3)) continue;
                                    lon = ln + dlon; latUse = la * DEG + dlat; found = true;
                                }
                            }
                        }
                    if (!found) continue;
                    lookAtWater = wantMat == -33 || wantMat == -35;
                    if (faceYaw < 1e8) faceYawOut = faceYaw;
                } else if (wantMat <= -10 && wantMat != -21 && wantMat != -22) {
                    // N2: a site of one biome (wantMat = -10 - biome), on land
                    int wantBio = -10 - wantMat;
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 34 && !found; la += 1.5)
                        for (int k = 0; k < 360 && !found; k++) {
                            double ln = k * TAU / 360;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.biome == wantBio && ss.material != MAT_WATER && ss.water < -1e8) { lon = ln; latUse = la * DEG; found = true; }
                        }
                    if (!found) continue;
                } else if (wantMat == -21 || wantMat == -22) {
                    // B-320: inland water: the first river (-21) or lake (-22) of the scan (`SurfaceSample::waterKind`), then
                    // the nearest dry land round it, 20-300 m out; the site stands there facing the water
                    bool found = false;
                    double Rm = b.radiusKm * 1000.0;
                    int wantKind = wantMat == -21 ? 1 : 2;
                    int hits = 0, noLand = 0;
                    // O6-03: a land window first (coarse, no drainage), then its tile's flood: the window is scanned at 16 m only
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
                    if (wla > 1e8) continue;
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
                    if (!found) continue;
                    lookAtWater = true;
                } else if (wantMat == -3 || wantMat == -5 || wantMat == -6 || wantMat == -8 || wantMat == -9) {
                    // N3/N4: open grassland, so a herd or the buggy can be seen
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 30 && !found; la += 1.5)
                        for (int k = 0; k < 360 && !found; k++) {
                            double ln = k * TAU / 360;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material == MAT_GRASS && (ss.biome == BIO_GRASSLAND || ss.biome == BIO_SAVANNA) && ss.height > 2) { lon = ln; latUse = la * DEG; found = true; }
                        }
                    if (!found) continue;
                } else if (wantMat == -4) {
                    // N2: deep forest (dense vegetation), for the coverage and the bench
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 30 && !found; la += 1.5)
                        for (int k = 0; k < 360 && !found; k++) {
                            double ln = k * TAU / 360;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material == MAT_FOREST && ss.veg > 0.75 && ss.height > 5) { lon = ln; latUse = la * DEG; found = true; }
                        }
                    if (!found) continue;
                } else if (b.type == PT_FELISIAN) {
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 30 && !found; la += 2)
                        for (int k = 0; k < 256 && !found; k++) {
                            double ln = k * TAU / 256;
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material == MAT_FOREST && ss.height > 20) { lon = ln; latUse = la * DEG; found = true; }
                        }
                }
                if (b.type == PT_MOLTEN) {
                    bool found = false;
                    for (double la = latDeg; la < latDeg + 20 && !found; la += 0.02)
                        for (double ln = 0; ln < TAU && !found; ln += 0.0006) {
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            if (ss.material == MAT_LAVA && ss.glow > 0.9) { lon = ln + 400.0 / (b.radiusKm * 1000.0); latUse = la * DEG; found = true; }
                        }
                }
                setDrainageEnabled(drainOff.was);   // O6-03: the site samples the real ground
                sv.init(&sys, bi, latUse, lon, 1000.0);
                double t = findTime(sv.site, alt, 1000.0);
                sv.init(&sys, bi, latUse, lon, t);
                SunInfo si = sv.site.sun(t);
                sv.player.yaw = faceYawOut < 1e8 ? faceYawOut : si.azimuth + yawOff;
                sv.player.pitch = pitch;
                if (wantMat == -2) {
                    // stand 60 m west of the nearest ruin of this world (M4-07); the grid is in latitude/longitude
                    bool found = false;
                    int gLat0, gLon0;
                    sv.ruinCellAt(0, 0, gLat0, gLon0);
                    for (int ring = 0; ring < 40 && !found; ring++)
                        for (int dl = -ring; dl <= ring && !found; dl++)
                            for (int dn = -ring; dn <= ring && !found; dn++) {
                                if (std::max(std::abs(dl), std::abs(dn)) != ring) continue;
                                Ruin ru;
                                if (!sv.ruinAt(gLat0 + dl, gLon0 + dn, ru)) continue;
                                TerrainVertex tvr = sv.site.sampleAt(ru.x, ru.z, 16);
                                if (tvr.water > -1e8f && tvr.h < tvr.water) continue;
                                double la2, lo2;
                                sv.site.latLonAt(ru.x - 60, ru.z, la2, lo2);
                                sv.init(&sys, bi, la2, lo2, t);
                                si = sv.site.sun(t);
                                sv.player.yaw = PI / 2; sv.player.pitch = 0.05;   // the ruin lies to the east
                                found = true;
                                sv.ruinAt(gLat0 + dl, gLon0 + dn, ru);
                                printf("ruin kind %d style %d size %.0f m at local (%.0f, %.0f)\n", ru.kind, ru.style, ru.size, ru.x, ru.z);
                            }
                    if (!found) continue;
                }
                if (wantMat == -8 || wantMat == -9) {   // N4: the buggy from the seat (-8) and parked, seen from 5 m (-9), by day on open ground
                    sv.relocateCapsule(sv.player.x + 4, sv.player.z + 2);
                    if (!sv.deployBuggy()) continue;
                    sv.buggy.unfold = 1;
                    sv.buggy.heading = 0.4;
                    if (wantMat == -8) { sv.player.x = sv.buggy.x + 1.5; sv.player.z = sv.buggy.z; if (!sv.toggleBuggy()) continue; sv.buggy.steer = 12 * DEG; sv.player.pitch = -0.06; }
                    else { sv.player.x = sv.buggy.x - 4.2; sv.player.z = sv.buggy.z - 3.0; double dx = sv.buggy.x - sv.player.x, dz = sv.buggy.z - sv.player.z; sv.player.yaw = std::atan2(dx, dz); sv.player.pitch = -0.12; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z); }
                }
                if (wantMat == -7) {   // N3: one creature of each land species 5 m ahead, side on, standing still
                    if (sv.critters.empty()) continue;
                    std::vector<Critter> keep;
                    for (int spi = 0; spi < sv.bestiary.landCount; spi++) {
                        Critter c; c.species = spi; c.herd = 0; c.seed = 1234 + spi; c.state = CS_IDLE; c.timer = 1e9;
                        c.size = sv.bestiary.species[spi].size; c.x = sv.player.x + (spi - (sv.bestiary.landCount - 1) * 0.5) * (2.5 + c.size * 2.5); c.z = sv.player.z + 5 + c.size * 2.5; c.heading = PI / 2;
                        keep.push_back(c);
                    }
                    sv.critters = keep;
                    sv.player.yaw = 0; sv.player.pitch = -0.08;
                    for (const Critter& c : sv.critters) printf("  %s (%s, %.1f m) at (%.1f, %.1f), ground %.1f\n", sv.bestiary.species[c.species].name.c_str(), PLAN_NAMES[sv.bestiary.species[c.species].plan], c.size, c.x, c.z, sv.site.groundHeight(c.x, c.z));
                }
                if (wantMat == -3 || wantMat == -5 || wantMat == -6) {
                    // stand 18 m (M4-05), 6 m or 60 m (N3) south of the first herd, facing it
                    if (sv.critters.empty()) continue;
                    double d = wantMat == -3 ? 18 : (wantMat == -5 ? 6 : 60);
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
                tOut = t;
                return true;
            }
        }
    return false;
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

struct CmpScene { const char* name; int type; double latDeg, alt, yawOff, pitch; int wantMat = -1; };
static const CmpScene CMP_SCENES[] = {
    {"felisian_forest", PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, -4},
    {"biome_tropical", PT_FELISIAN, 0, 40 * DEG, 0.5, -0.03, -10 - BIO_TROPICAL},
    {"biome_savanna", PT_FELISIAN, 5, 40 * DEG, 0.5, -0.03, -10 - BIO_SAVANNA},
    {"biome_desert", PT_FELISIAN, 10, 40 * DEG, 0.5, -0.03, -10 - BIO_DESERT},
    {"biome_temperate", PT_FELISIAN, 25, 35 * DEG, 0.5, -0.03, -10 - BIO_TEMPERATE},
    {"biome_grassland", PT_FELISIAN, 20, 35 * DEG, 0.5, -0.03, -10 - BIO_GRASSLAND},
    {"biome_taiga", PT_FELISIAN, 45, 25 * DEG, 0.5, -0.03, -10 - BIO_TAIGA},
    {"biome_tundra", PT_FELISIAN, 55, 15 * DEG, 0.5, -0.03, -10 - BIO_TUNDRA},
    {"biome_wetland", PT_FELISIAN, 5, 30 * DEG, 0.5, -0.03, -10 - BIO_WETLAND},
    {"tundra_lowsun", PT_FELISIAN, 55, 5 * DEG, 0.0, 0.05, -10 - BIO_TUNDRA},
    {"felisian_ruin", PT_FELISIAN, 12, 35 * DEG, 0.0, 0.0, -2},
    {"felisian_herd", PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, -3},
    {"felisian_herd_near", PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, -5},
    {"felisian_herd_far", PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, -6},
    {"creatures_lineup", PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, -7},
    {"buggy_seat", PT_FELISIAN, 12, 40 * DEG, 0.6, 0.0, -8},
    {"buggy_parked", PT_FELISIAN, 12, 40 * DEG, 0.6, 0.0, -9},
    {"felisian_shore", PT_FELISIAN, 12, 30 * DEG, 0.0, 0.0, MAT_SAND},
    {"felisian_river", PT_FELISIAN, 10, 40 * DEG, 0.0, -0.03, -21},   // B-320: on the bank of a river above the sea, looking across it
    {"felisian_lake", PT_FELISIAN, 10, 40 * DEG, 0.0, -0.03, -22},    // B-320: on a lake shore
    // R-307: the six new types, each at its signature
    {"europan_crack", PT_EUROPAN, 20, 30 * DEG, 0.6, -0.02, -30},
    {"tectonic_fissure", PT_TECTONIC, 15, 35 * DEG, 0.6, -0.03, -31},
    {"desert_erg", PT_DESERT, 15, 40 * DEG, 0.5, -0.02, -32},
    {"hydrocarbon_shore", PT_HYDROCARBON, 10, 30 * DEG, 0.0, -0.02, -33},
    {"bombarded_plain", PT_BOMBARDED, 15, 35 * DEG, 0.5, -0.02, -34},
    {"acidic_shore", PT_ACIDIC, 15, 40 * DEG, 0.0, -0.02, -35},
    {"geyser_basin", -1, 10, 40 * DEG, 0.0, 0.0, -36},   // R-307: 150 m from a geyser basin's vent on any world with the trait, facing it; `warm` waits for its jet
    {"felisian_sunset", PT_FELISIAN, 12, 4 * DEG, 0.0, 0.05},
    {"felisian_mountains", PT_FELISIAN, 0, 40 * DEG, 0.0, 0.02, -20},   // B-313: the pinned mountain review site of O6, facing its highest ground
    {"cratered_noon", PT_CRATERED, 12, 45 * DEG, 0.0, 0.0},
    {"thinatmo_sunset", PT_THINATMO, 12, 4 * DEG, 0.0, 0.05},
    {"molten_night", PT_MOLTEN, 12, -40 * DEG, 0.5, 0.15},
    {"icy_noon", PT_ICY, 12, 40 * DEG, 0.3, 0.05},
    {"comet_day", PT_COMET, 10, 40 * DEG, 0.4, 0.05},       // O4 (R-303): the nucleus by day, jets on the sunlit ground
    {"comet_night", PT_COMET, 10, -30 * DEG, PI, 0.35},     // and at night, looking away from the sun: the tail overhead
};


// B-309: temporal stability of a scene: eight frames a thirtieth of a second apart, the mean absolute change of the
// luminance between consecutive frames, per region (a box of 40 logical pixels round the sun, the sky, the ground),
// and a heat map of where the picture moves (`shots/tests/stability_<scene>.png`, the last frame beside it). What
// should move: water, leaves, weather, creatures, the granulation of the sun. What should not: the sun's glow.
// B-313: `stability <scene> <metres per frame>` walks the camera forward instead: the far-ground band (rows 42-62%
// of the frame, ground beyond a few tens of metres) reports the mean change and the share of its pixels that jump by
// more than 40 levels in a frame, which is what a terrain ring popping cell by cell does and parallax does not; the
// sheet is `stability_<scene>_walk.png`
static int runStability(const char* want, double walk) {
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
        long flips = 0;   // B-319: pixels that swap between the sky bank and anything else from one frame to the next (a far tree flickering against the sky)
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
            if (k == 0) { const Vec3& sd = sv.env.sunDisc.dirLocal; if (!sv.projectPoint(sv.player.x + sd.x * 1e5, sv.player.y + 1.65 + sd.y * 1e5, sv.player.z + sd.z * 1e5, sunSx, sunSy)) sunSx = -1; }
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
                   farN ? farSum / farN : 0.0, farN ? 100.0 * farPops / ((double)farN * (N - 1)) : 0.0, (double)flips / (N - 1) / (FB_SCALE * FB_SCALE), skyN ? skySum / skyN : 0.0, gndN ? gndSum / gndN : 0.0, mx / (N - 1), fn.c_str());
        else
        printf("%-18s sun %s%.2f  sky %.2f  ground %.2f  (mean |dL| per frame, 0..255; peak %.0f) -> %s\n", sc.name,
               sunSx >= 0 ? (sunSx >= 0 && sunSx < FBW && sunSy >= 0 && sunSy < FBH ? "in frame " : "off frame ") : "below ", sunN ? sunSum / sunN : 0.0, skyN ? skySum / skyN : 0.0, gndN ? gndSum / gndN : 0.0, mx / (N - 1), fn.c_str());
        shown++;
    }
    return shown ? 0 : 1;
}
// M7-04 frame hashes (defined after the scene helpers it uses)
static void appendFrameHashes(std::vector<std::pair<std::string, uint64_t>>& lines, std::vector<std::string>& notes) {
    const uint64_t FNV0 = 1469598103934665603ULL;
    // M7-04 frame hashes: three fixed surface scenes and one space frame at 1x (the framebuffer indices, before the RGB conversion)
    {
        int savedScale = g_testScale;
        setFramebufferScale(1);
        SpaceRenderer sr; StarNeighborhood nb;
        const char* want[3] = {"felisian_shore", "cratered_noon", "molten_night"};
        for (const char* name : want)
            for (const CmpScene& sc : CMP_SCENES) {
                if (std::string(name) != sc.name) continue;
                StarSystem sys; SurfaceView sv; double t = 0;
                if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) continue;
                for (const Body& b : sys.bodies) if (b.type != PT_COMPANION) sr.mapFor(b);   // sky bodies drawn textured, never mid-generation
                Framebuffer fb;
                sv.render(fb, t, nb.stars, sr, 1.0);
                fb.mush(2);
                uint64_t h = fnv(FNV0, fb.idx.data(), fb.idx.size() * sizeof(Pix));
                lines.push_back({std::string("scene_") + name, h});
                notes.push_back(std::string("frame of scene ") + name + " at 1x");
            }
        for (int64_t x = 150; x < 300; x++) {
            bool done = false;
            for (int64_t z = 20; z < 120 && !done; z++) {
                Star s;
                if (!starInSector(x, 0, z, s) || s.cls != STAR_YELLOW) continue;
                StarSystem sys; sys.generate(s);
                double d = std::max(s.radiusKm * STAR_CLASSES[s.cls].firstOrbitMult, STAR_CLASSES[s.cls].minFirstOrbitKm);
                Vec3 shipPos = s.pos + Vec3(0, 0, -d);
                nb.update(shipPos);
                Framebuffer fb;
                SpaceContext c;
                c.sys = &sys; c.stars = &nb.stars; c.t = 1234.0; c.shipPos = shipPos; c.cam = cameraBasis(0, 0);
                for (const Body& b : sys.bodies) if (b.type != PT_COMPANION) sr.mapFor(b);
                sr.setupPalette(fb, &sys, -1, -1, 1.0);
                sr.render(fb, c);
                fb.mush(2);
                lines.push_back({"space_S00", fnv(FNV0, fb.idx.data(), fb.idx.size() * sizeof(Pix))});
                notes.push_back("space frame from the first orbit of " + s.name);
                done = true;
            }
            if (done) break;
        }
        setFramebufferScale(savedScale);
    }
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
static const char* SHEET_SCENES[] = {"felisian_forest", "felisian_shore", "felisian_sunset", "cratered_noon", "thinatmo_sunset", "icy_noon", "buggy_seat", "orbit"};

static void renderCompare() {
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
        for (int64_t x = 150; x < 320 && !done; x++)
            for (int64_t z = 20; z < 140 && !done; z++) {
                Star st;
                if (!starInSector(x, 0, z, st)) continue;
                StarSystem sys; sys.generate(st);
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
static void composeSheets() {
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

// ---- survey and gallery (M9-01) ------------------------------------------------------------
// survey [N]: distributions over N bodies of the start region; gallery <type>: 16 random sites in one sheet.
static void runSurvey(int wanted) {
    int types[PT_COUNT] = {0}, planets = 0, moons = 0, systems = 0, felSystems = 0;
    int tempHist[6] = {0}, gravHist[5] = {0}, moonHist[8] = {0};
    double matCov[PT_COUNT][MAT_COUNT] = {{0}};
    int matN[PT_COUNT] = {0};
    int bodies = 0;
    // M5-09 (R-001): what hangs in the sky at random landing sites and times
    int skySites = 0, sky05 = 0, sky1 = 0, sky3 = 0, sky10 = 0, skyPoint = 0, skyParent = 0, skyMoon = 0, skySister = 0, skyOther = 0, skyTwoSuns = 0;
    int multiples = 0, substellar = 0, comets = 0, doubles = 0, lockedN = 0;
    for (int64_t x = 150; x < 320 && bodies < wanted; x++)
        for (int64_t z = 20; z < 140 && bodies < wanted; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            systems++;
            if (sys.companion >= 0) multiples++;
            bool fel = false;
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type == PT_SUBSTELLAR) substellar++;
                if (b.type == PT_COMET) comets++;
                if (b.doublePlanet && b.parent >= 0) doubles++;
                if (b.locked) lockedN++;
                if (!PLANET_TYPES[b.type].landable) continue;
                Rng sr(b.seed ^ 0x5C1EULL);
                for (int k = 0; k < 2; k++) {
                    double lat = std::asin(sr.sym(1.0)), lon = sr.range(0, TAU), t = sr.range(0, 4e6);
                    Vec3 site = sys.surfacePointWorld(bi, t, lat, lon, 0.002);
                    Vec3 up = normalize(site - sys.bodyPos(bi, t));
                    double best = 0; int bestKind = -1; bool point = false; int sunsUp = 0;
                    for (int j = 0; j < (int)sys.bodies.size(); j++) {
                        if (j == bi) continue;
                        Vec3 rel = sys.bodyPos(j, t) - site;
                        double d = length(rel);
                        if (dot(rel / d, up) < 0.02) continue;
                        double ang = std::asin(clampd(sys.bodies[j].radiusKm / d, 0, 1)) / DEG;
                        if (sys.bodies[j].type == PT_COMPANION) { sunsUp++; continue; }
                        if (sys.bodies[j].parent < 0 && ang < 0.25) point = true;
                        if (ang > best) { best = ang; bestKind = j == b.parent ? 0 : (sys.bodies[j].parent == bi ? 1 : (sys.bodies[j].parent == b.parent && b.parent >= 0 ? 2 : 3)); }
                    }
                    if (dot(normalize(sys.star.pos - site), up) > 0.02) sunsUp++;
                    skySites++;
                    if (best > 0.5) { sky05++; if (bestKind == 0) skyParent++; else if (bestKind == 1) skyMoon++; else if (bestKind == 2) skySister++; else skyOther++; }
                    if (best > 1) sky1++;
                    if (best > 3) sky3++;
                    if (best > 10) sky10++;
                    if (point) skyPoint++;
                    if (sunsUp >= 2) skyTwoSuns++;
                }
            }
            for (const Body& b : sys.bodies) {
                bodies++;
                types[b.type]++;
                if (b.parent < 0) { planets++; moonHist[std::min(7, b.moonCount)]++; } else moons++;
                if (b.type == PT_FELISIAN) fel = true;
                tempHist[std::min(5, (int)(b.tempK / 150))]++;
                gravHist[std::min(4, (int)(b.gravity / 5.0))]++;
                if (b.type != PT_GASGIANT && matN[b.type] < 40) {
                    BodyGen g = BodyGen::make(b);
                    double detail = TAU * g.R * 1000.0 / 48.0 * 0.6;
                    int cnt[MAT_COUNT] = {0}, tot = 0;
                    for (int j = 0; j < 12; j++)
                        for (int i = 0; i < 24; i++) {
                            double lat = (0.5 - (j + 0.5) / 12) * PI, lon = ((i + 0.5) / 24) * TAU - PI;
                            cnt[sampleSurface(g, StarSystem::bodyFromLatLon(lat, lon), detail).material]++; tot++;
                        }
                    for (int m = 0; m < MAT_COUNT; m++) matCov[b.type][m] += (double)cnt[m] / tot;
                    matN[b.type]++;
                }
            }
            if (fel) felSystems++;
        }
    printf("survey: %d systems, %d bodies (%d planets, %d moons); felisian in %d systems (%.0f%%)\n", systems, bodies, planets, moons, felSystems, 100.0 * felSystems / std::max(1, systems));
    printf("types:"); for (int i = 0; i < PT_COUNT; i++) printf("  %s %.1f%%", PLANET_TYPES[i].name, 100.0 * types[i] / std::max(1, bodies)); printf("\n");
    printf("temperature K: <150 %d, 150-300 %d, 300-450 %d, 450-600 %d, 600-750 %d, >750 %d\n", tempHist[0], tempHist[1], tempHist[2], tempHist[3], tempHist[4], tempHist[5]);
    printf("gravity m/s2: <5 %d, 5-10 %d, 10-15 %d, 15-20 %d, >20 %d\n", gravHist[0], gravHist[1], gravHist[2], gravHist[3], gravHist[4]);
    printf("moons per planet:"); for (int i = 0; i < 8; i++) printf(" %d:%d", i, moonHist[i]); printf("\n");
    printf("M5: multiple systems %d (%.0f%%), substellar objects %d, comets %d, double planets %d, locked planets %d\n", multiples, 100.0 * multiples / std::max(1, systems), substellar, comets, doubles, lockedN);
    if (skySites) {
        printf("sky at %d random sites (R-001 targets in brackets): disc >0.5 deg %.0f%%, >1 deg %.0f%% [45-50], >3 deg %.0f%% [35], >10 deg %.0f%% [12-15]; a planet as a point %.0f%%; two suns up %.0f%%\n",
               skySites, 100.0 * sky05 / skySites, 100.0 * sky1 / skySites, 100.0 * sky3 / skySites, 100.0 * sky10 / skySites, 100.0 * skyPoint / skySites, 100.0 * skyTwoSuns / skySites);
        printf("  the disc is: the parent %.0f%%, an own moon %.0f%%, a sister moon %.0f%%, another world %.0f%%\n", 100.0 * skyParent / std::max(1, sky05), 100.0 * skyMoon / std::max(1, sky05), 100.0 * skySister / std::max(1, sky05), 100.0 * skyOther / std::max(1, sky05));
    }
    for (int t = 0; t < PT_COUNT; t++) {
        if (!matN[t]) continue;
        printf("  %-16s", PLANET_TYPES[t].name);
        for (int m = 0; m < MAT_COUNT; m++) if (matCov[t][m] / matN[t] > 0.005) printf(" %s %.0f%%", MATERIAL_NAMES[m], 100.0 * matCov[t][m] / matN[t]);
        printf("\n");
    }
}

static void renderGallery(int type) {
    SpaceRenderer sr;
    StarNeighborhood nb;
    const int TW = 320, TH = 200;
    std::vector<uint32_t> sheet((size_t)TW * 4 * TH * 4, rgb(10, 10, 10));
    RGBCanvas cv{sheet.data(), TW * 4, TH * 4, 1};
    int n = 0;
    Rng r(777 + type);
    setFramebufferScale(1);
    for (int64_t x = 150; x < 320 && n < 16; x++)
        for (int64_t z = 20; z < 140 && n < 16; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && n < 16; bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != type || !PLANET_TYPES[b.type].landable) continue;
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
    std::string tn = PLANET_TYPES[type].name;
    for (char& ch : tn) if (ch == ' ') ch = '_';
    std::string fn = "shots/tests/gallery_" + tn + ".png";
    writePNG(fn.c_str(), sheet.data(), TW * 4, TH * 4);
    printf("gallery: %d sites of %s -> %s\n", n, PLANET_TYPES[type].name, fn.c_str());
}

// ---- random-walk fuzz (M0-08): random keys and mouse across every state -------------------
static int runFuzz(int frames) {
    Game game;
    game.savePrefix = "shots/tests/fuzz_save";
    game.moviesDir = "shots/tests/test_movies"; game.recFixedTake = true;   // random Ctrl+R must not litter movies/
    game.settingsPath = "shots/tests/fuzz_settings.txt";
    game.guidePath = "shots/tests/fuzz_guide.txt"; game.keysPath = "shots/tests/fuzz_keys.txt";
    applyTestScale(game);
    Input in;
    Rng r(20260926);
    static const int keys[] = {KEY_ENTER, KEY_ESCAPE, KEY_SPACE, KEY_TAB, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_W, KEY_A, KEY_S, KEY_D,
                               KEY_R, KEY_V, KEY_L, KEY_X, KEY_O, KEY_C, KEY_T, KEY_Q, KEY_E, KEY_K, KEY_I, KEY_H, KEY_N, KEY_P,
                               KEY_LEFT_SHIFT, KEY_LEFT_CONTROL, KEY_F1, KEY_F2, KEY_F5, KEY_F9, KEY_PAGE_UP, KEY_PAGE_DOWN};
    const int nk = sizeof(keys) / sizeof(keys[0]);
    int bad = 0, states[16] = {0};
    double t0 = nowSec();
    for (int f = 0; f < frames; f++) {
        in.newFrame();
        for (int i = 0; i < nk; i++) {
            int k = keys[i];
            if (r.chance(0.02)) { in.pressed[k] = true; in.down[k] = true; }
            else if (in.down[k] && r.chance(0.15)) in.down[k] = false;
        }
        in.mouseDx = r.sym(40); in.mouseDy = r.sym(25); in.wheel = r.irange(3) - 1;
        in.mousePressed[0] = r.chance(0.02); in.mouseDown[0] = in.mousePressed[0];
        game.frame(in, 1.0 / 60);
        game.wantsQuit = false; game.wantsScreenshot = false; game.wantsFullscreenToggle = false; game.wantsWindowScale = 0;
        states[(int)game.state & 15]++;
        std::string h = game.testHealth();
        if (!h.empty()) { printf("frame %d: non-finite %s\n", f, h.c_str()); if (++bad > 5) break; }
    }
    printf("fuzz: %d frames in %.1f s, %d health failures; frames per state:", frames, nowSec() - t0, bad);
    for (int s = 0; s < 12; s++) printf(" %d:%d", s, states[s]);
    printf("\n");
    return bad ? 1 : 0;
}

// Re-anchoring check (KI-007): walk 51 km, the frame moves, positions and headings stay consistent.
static void testReanchor() {
    for (int64_t x = 150; x < 320; x++)
        for (int64_t z = 20; z < 140; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
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
                printf("reanchor on %s: site (%.2f,%.2f) -> (%.4f,%.4f), player at (%.0f,%.0f) m; lat/lon %.4f/%.4f -> %.4f/%.4f; capsule %.0f m %.1f deg -> %.0f m %.1f deg; yaw %.3f -> %.3f; forward point moved %.1f m\n",
                       b.name.c_str(), 62 * DEG, 0.4, sv.site.lat0, sv.site.lon0, sv.player.x, sv.player.z, latB, lonB, latA, lonA, capB, bearB, capA, bearA, yawB, yawA, fwdErrM);
                return;
            }
        }
}

// M4-09: render a few seconds of every sound family into a WAV file and check for NaN and clipping.
static void testAudio() {
    AudioSynth synth;
    const int sr = 22050;
    std::vector<int16_t> pcm;
    int nan = 0, clip = 0;
    struct Stage { const char* name; double hum, engine, wind, tone, rain, lava, surf, birds, breath; int step; double leaves; };
    const Stage stages[] = {{"ship", 0.6, 0, 0, 0.5, 0, 0, 0, 0, 0, 0, 0}, {"vimana", 0.6, 0.9, 0, 0.5, 0, 0, 0, 0, 0, 0, 0}, {"shore", 0.05, 0, 0.4, 0.25, 0, 0, 0.6, 0.6, 0, 1, 0},
                            {"desert storm", 0.05, 0, 1.0, 0.6, 0, 0, 0, 0, 0.4, 2, 0}, {"mountain rain", 0.05, 0, 0.6, 1.0, 0.8, 0, 0, 0, 0, 3, 0}, {"lava", 0.05, 0, 0, 0.5, 0, 0.6, 0, 0, 0, 0, 0},
                            {"forest", 0.05, 0, 0.35, 0.5, 0, 0, 0, 0.4, 0, 1, 0.8},   // N2-06
                            {"buggy", 0.05, 0.6, 0.5, 0.5, 0, 0, 0, 0, 0, 0, 0}};   // N4-04 (engine, rpm and skid set below)
    std::vector<float> buf(2048);
    for (const Stage& sg : stages) {
        AudioState st; st.hum = sg.hum; st.engine = sg.engine; st.wind = sg.wind; st.windTone = sg.tone; st.rain = sg.rain; st.lava = sg.lava; st.surf = sg.surf; st.birds = sg.birds; st.breath = sg.breath; st.leaves = sg.leaves;
        if (std::string(sg.name) == "buggy") { st.engineRpm = 0.7; st.skid = 0.4; st.thud = 0.8; }
        int frames = 0;
        while (frames < sr * 3) {
            if ((frames / 2048) % 6 == 2 && sg.step) st.step = sg.step;
            if (frames == 0 && sg.engine == 0 && sg.lava == 0) st.beep = 1;
            synth.render(buf.data(), (int)buf.size(), sr, st);
            for (float v : buf) { if (!std::isfinite(v)) nan++; if (std::fabs(v) >= 0.999f) clip++; pcm.push_back((int16_t)(std::max(-1.f, std::min(1.f, v)) * 32000)); }
            frames += (int)buf.size();
        }
        printf("audio stage %-14s ok\n", sg.name);
    }
    FILE* f = fopen("shots/tests/audio_test.wav", "wb");
    if (f) {
        uint32_t dataBytes = (uint32_t)pcm.size() * 2, rate = sr, byteRate = sr * 2; uint16_t ch = 1, bits = 16, blockAlign = 2, fmtTag = 1; uint32_t fmtLen = 16, riffLen = 36 + dataBytes;
        fwrite("RIFF", 1, 4, f); fwrite(&riffLen, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmtLen, 4, 1, f); fwrite(&fmtTag, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&rate, 4, 1, f); fwrite(&byteRate, 4, 1, f); fwrite(&blockAlign, 2, 1, f); fwrite(&bits, 2, 1, f);
        fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f); fwrite(pcm.data(), 2, pcm.size(), f); fclose(f);
    }
    printf("audio: %zu samples, %d non-finite, %d clipped -> shots/audio_test.wav\n", pcm.size(), nan, clip);
}

// B-310: `bench forest`: the forest scene at 4x rendered 150 times, for a profiler (`sample vesperis_test 5` on macOS)
static int runBenchForest() {
    setFramebufferScale(4);
    SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
    if (!setupSceneForType(PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, sv, sys, nb, t, -4)) return 1;
    Framebuffer fb; Input in;
    double t0 = nowSec();
    for (int i = 0; i < 150; i++) { sv.update(1.0 / 30, in, t + i * 0.033, false); sv.render(fb, t + i * 0.033, nb.stars, sr, 1.0); fb.mush(2); }
    printf("forest 4x x150: %.2f ms/frame (sections sky %.1f warm %.1f terrain %.1f objects %.1f flora %.1f rest %.1f; trees %.1f ms)\n", (nowSec() - t0) * 1000 / 150,
           sv.lastRenderMs[0], sv.lastRenderMs[1], sv.lastRenderMs[2], sv.lastRenderMs[3], sv.lastRenderMs[4], sv.lastRenderMs[5], sv.lastFloraMs[0]);
    return 0;
}

static int runBench(bool check) {
    Game game;
    double surface2x = 0, space2x = 0, descentFirst = 0, runWorst = 0;
    game.savePrefix = "shots/tests/bench_save";
    game.settingsPath = "shots/tests/test_settings.txt";
    game.guidePath = "shots/tests/bench_guide.txt";
    Input in;
    auto press = [&](int key) { in.pressed[key] = true; in.down[key] = true; game.frame(in, 1.0 / 60); in.newFrame(); in.down[key] = false; };
    auto bench = [&](const char* name, int frames) {
        double t0 = nowSec();
        for (int i = 0; i < frames; i++) { game.frame(in, 1.0 / 60); in.newFrame(); }
        double dt = (nowSec() - t0) / frames * 1000;
        printf("%-12s %.2f ms/frame\n", name, dt);
        return dt;
    };
    press(KEY_ENTER);
    for (int s = 1; s <= 4; s++) { game.settings.renderScale = s; game.applySettings(); double ms = bench(("space " + std::to_string(s) + "x").c_str(), 60); if (s == 2) space2x = ms; }
    game.settings.renderScale = 1; game.applySettings();
    press(KEY_C); press(KEY_R);
    {   // O6-03: a player looks at the map for a moment; the drainage tiles of the site are computed meanwhile (up to 3 s here)
        int waited = 0; double tw0 = nowSec();
        for (; nowSec() - tw0 < 3.0 && !game.testDrainageReady(); waited++) { game.frame(in, 1.0 / 60); in.newFrame(); }
        printf("landing map  %.2f s (%d frames) until the drainage tiles were ready (%s, %d tiles held)\n", nowSec() - tw0, waited, game.testBodyTypes().c_str(), drainageTilesHeld());
    }
    {
        // the first descent frames used to hitch (KI-009): time them
        in.pressed[KEY_ENTER] = true; in.down[KEY_ENTER] = true;
        double worst = 0, first = 0;
        for (int i = 0; i < 60; i++) {
            double t0 = nowSec();
            game.frame(in, 1.0 / 60);
            double ms = (nowSec() - t0) * 1000;
            in.newFrame(); in.down[KEY_ENTER] = false;
            if (i == 0) first = ms;
            worst = std::max(worst, ms);
        }
        printf("descent      first frame %.1f ms, worst of 60: %.1f ms\n", first, worst);
        { const double* ms = game.testRenderMs(); printf("  last frame's render sections: palette+sky %.1f, floor+warm %.1f, terrain %.1f, reflections..objects %.1f, flora %.1f, rest %.1f ms\n", ms[0], ms[1], ms[2], ms[3], ms[4], ms[5]); }
        descentFirst = first;
    }
    for (int i = 0; i < 60 * 7; i++) { game.frame(in, 1.0 / 60); in.newFrame(); }
    {   // M7-01: the first surface frames after landing (the caches were warmed during the descent)
        double first = 0, worst = 0;
        for (int i = 0; i < 30; i++) { double t0 = nowSec(); game.frame(in, 1.0 / 60); double ms = (nowSec() - t0) * 1000; in.newFrame(); if (i == 0) first = ms; worst = std::max(worst, ms); }
        printf("landing      first frame %.1f ms, worst of 30: %.1f ms\n", first, worst);
    }
    for (int s = 4; s >= 2; s--) {
        game.settings.renderScale = s; game.applySettings();
        double ms = bench(("surface " + std::to_string(s) + "x").c_str(), 40);
        if (s == 2) surface2x = ms;
        const double* r = game.testRenderMs();
        printf("             sections: sky %.1f, warm %.1f, terrain %.1f, objects %.1f, flora %.1f, rest %.1f ms\n", r[0], r[1], r[2], r[3], r[4], r[5]);
    }
    game.settings.renderScale = 1; game.applySettings();
    bench("surface", 120);
    writePNG("shots/tests/bench_surface.png", game.output(), FBW, FBH);
    printf("  state=%d %s\n", (int)game.state, game.testStatus().c_str());
    in.down[KEY_W] = true; in.down[KEY_LEFT_SHIFT] = true;
    {   // M7-01: a sprint of four seconds; the worst frame shows whether the caches stall
        double worst = 0, sum = 0;
        for (int i = 0; i < 240; i++) { double t0 = nowSec(); game.frame(in, 1.0 / 60); double ms = (nowSec() - t0) * 1000; in.newFrame(); worst = std::max(worst, ms); sum += ms; }
        printf("surface-run  %.2f ms/frame, worst %.1f ms, %d cells delivered by the prefetch worker\n", sum / 240, worst, game.testAheadCells());
        runWorst = worst;
    }
    in.down[KEY_W] = false; in.down[KEY_LEFT_SHIFT] = false;
    in.mouseDx = 300; game.frame(in, 1.0 / 60); in.newFrame();
    bench("surface2", 120);
    writePNG("shots/tests/bench_surface2.png", game.output(), FBW, FBH);
    in.mouseDx = 300; game.frame(in, 1.0 / 60); in.newFrame();
    bench("surface3", 120);
    writePNG("shots/tests/bench_surface3.png", game.output(), FBW, FBH);
    for (int i = 0; i < 3; i++) press(KEY_T);
    for (int i = 0; i < 60 * 6; i++) { game.frame(in, 1.0 / 60); in.newFrame(); }
    writePNG("shots/tests/bench_surface4.png", game.output(), FBW, FBH);
    // M7-02 map cache counters
    const SpaceRenderer::MapStats& ms = game.testMapStats();
    printf("maps         %d generated (%.0f ms each on the worker), %d hits, %d evicted, %zu held (cap 24)\n", ms.generated, ms.generated ? game.testGenMicros() / 1000.0 / ms.generated : 0.0, ms.hits, ms.evicted, game.testMapsHeld());
    if (!check) return 0;
    // M7-04 performance budgets, generous enough for a CI runner
    int fails = 0;
    auto budget = [&](const char* name, double ms, double limit) { bool ok = ms <= limit; printf("budget %-14s %.1f ms (limit %.0f) %s\n", name, ms, limit, ok ? "ok" : "EXCEEDED"); if (!ok) fails++; };
    {   // N2-04: a forest view at 2x through the surface renderer alone
        int saved = g_testScale;
        setFramebufferScale(2);
        SpaceRenderer sr2; StarNeighborhood nb2; StarSystem sys2; SurfaceView sv2; double t2 = 0;
        if (setupSceneForType(PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, sv2, sys2, nb2, t2, -4)) {
            Framebuffer fb2; Input in2;
            for (int i = 0; i < 5; i++) { sv2.update(1.0 / 30, in2, t2, false); sv2.render(fb2, t2, nb2.stars, sr2, 1.0); }
            double t0 = nowSec();
            for (int i = 0; i < 30; i++) { sv2.update(1.0 / 30, in2, t2 + i * 0.033, false); sv2.render(fb2, t2 + i * 0.033, nb2.stars, sr2, 1.0); fb2.mush(2); }
            double ms = (nowSec() - t0) * 1000 / 30;
            printf("forest 2x    %.2f ms/frame (%d trees: near %d mid %d far %d, %d hidden behind canopy, %d flora points; flora sections trees %.1f, logs+undergrowth %.1f, meadows/shores %.1f, deserts %.1f ms)\n", ms, sv2.lastTreesDrawn,
                   sv2.lastNearTrees, sv2.lastMidTrees, sv2.lastFarTrees, sv2.lastTreesHidden, sv2.lastFloraPoints, sv2.lastFloraMs[0], sv2.lastFloraMs[1], sv2.lastFloraMs[2], sv2.lastFloraMs[3]);
            saveFB(fb2, "shots/tests/bench_forest.png");
            budget("forest 2x", ms, 14);   // N2-04: a tropical giant forest, the densest case (about 12 ms on an M4; CI headroom)
        }
        setFramebufferScale(4);   // the same forest at 4x (the default picture since B-310)
        { SpaceRenderer sr4; StarNeighborhood nb4; StarSystem sys4; SurfaceView sv4; double t4 = 0;
          if (setupSceneForType(PT_FELISIAN, 12, 35 * DEG, 0.6, -0.03, sv4, sys4, nb4, t4, -4)) {
            Framebuffer fb4; Input in4;
            for (int i = 0; i < 5; i++) { sv4.update(1.0 / 30, in4, t4, false); sv4.render(fb4, t4, nb4.stars, sr4, 1.0); }
            double t0 = nowSec();
            for (int i = 0; i < 30; i++) { sv4.update(1.0 / 30, in4, t4 + i * 0.033, false); sv4.render(fb4, t4 + i * 0.033, nb4.stars, sr4, 1.0); fb4.mush(2); }
            double ms = (nowSec() - t0) * 1000 / 30;
            printf("forest 4x    %.2f ms/frame (flora sections trees %.1f, logs+undergrowth %.1f, meadows/shores %.1f, deserts %.1f ms; %d trees listed, %d visits over the bands)\n", ms, sv4.lastFloraMs[0], sv4.lastFloraMs[1], sv4.lastFloraMs[2], sv4.lastFloraMs[3], sv4.lastTreesListed, sv4.lastTreesVisited);
            budget("forest 4x", ms, 24);
        } }
        setFramebufferScale(saved);
    }
    {   // N3: a herd at 18 m at 1x through the surface renderer: the cost of updating and drawing life
        SpaceRenderer sr3; StarNeighborhood nb3; StarSystem sys3; SurfaceView sv3; double t3 = 0;
        if (setupSceneForType(PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, sv3, sys3, nb3, t3, -3)) {
            Framebuffer fb3; Input in3;
            double upd = 0, drw = 0; int n = 0; for (const Critter& c : sv3.critters) if (c.herd == 0) n++;
            for (int i = 0; i < 20; i++) { sv3.update(1.0 / 30, in3, t3 + i * 0.033, false); sv3.render(fb3, t3 + i * 0.033, nb3.stars, sr3, 1.0); if (i >= 5) { upd += sv3.lastLifeMs[0]; drw += sv3.lastLifeMs[1]; } }
            printf("herd 1x      update %.2f ms, draw %.2f ms per frame (%d in the herd, %zu creatures at the site)\n", upd / 15, drw / 15, n, sv3.critters.size());
            budget("herd draw 1x", drw / 15, 1.0);
        }
    }
    {   // N4-02: the driving frame at 2x (the game, seat view, 60 frames at full throttle)
        game.settings.renderScale = 2; game.applySettings();
        if (game.testDriveSetup()) {
            Input in4; in4.down[KEY_W] = true;
            double sum = 0, worst = 0;
            for (int i = 0; i < 90; i++) { double t0 = nowSec(); game.frame(in4, 1.0 / 30); in4.newFrame(); in4.down[KEY_W] = true; double ms = (nowSec() - t0) * 1000; if (i >= 30) { sum += ms; worst = std::max(worst, ms); } }
            printf("buggy 2x     %.2f ms/frame, worst %.1f ms (%s)\n", sum / 60, worst, game.testBuggyInfo().c_str());
            writePNG("shots/tests/bench_buggy.png", game.output(), FBW, FBH);
            budget("buggy 2x", sum / 60, 14);
        }
        game.settings.renderScale = 4; game.applySettings();   // and at 4x, the default picture (B-310)
        if (game.testDriveSetup()) {
            Input in4; in4.down[KEY_W] = true;
            double sum = 0, worst = 0;
            for (int i = 0; i < 90; i++) { double t0 = nowSec(); game.frame(in4, 1.0 / 30); in4.newFrame(); in4.down[KEY_W] = true; double ms = (nowSec() - t0) * 1000; if (i >= 30) { sum += ms; worst = std::max(worst, ms); } }
            printf("buggy 4x     %.2f ms/frame, worst %.1f ms\n", sum / 60, worst);
            budget("buggy 4x", sum / 60, 24);
        }
        game.settings.renderScale = 1; game.applySettings();
    }
    {   // O3: a frame inside a belt at 2x, parked beside a rock (the rock field is the cost)
        bool found = false;
        for (int64_t x = 150; x < 320 && !found; x++)
            for (int64_t z = 20; z < 140 && !found; z++) {
                Star s; if (!starInSector(x, 0, z, s)) continue;
                StarSystem sy; sy.generate(s);
                if (sy.belts.empty()) continue;
                game.settings.renderScale = 2; game.applySettings();
                game.testParkAtBelt(s, 0);
                Input in5;
                for (int i = 0; i < 10; i++) { game.frame(in5, 1.0 / 60); in5.newFrame(); }
                in5.pressed[KEY_X] = true; in5.down[KEY_X] = true; game.frame(in5, 1.0 / 60); in5.newFrame(); in5.down[KEY_X] = false;
                double sum = 0;
                for (int i = 0; i < 60; i++) { double t0 = nowSec(); game.frame(in5, 1.0 / 60); in5.newFrame(); sum += (nowSec() - t0) * 1000; }
                printf("belt 2x      %.2f ms/frame (%s)\n", sum / 60, game.testDebugInfo().c_str());
                writePNG("shots/tests/bench_belt.png", game.output(), FBW, FBH);
                budget("belt 2x", sum / 60, 12);
                game.settings.renderScale = 1; game.applySettings();
                found = true;
            }
    }
    budget("space 2x", space2x, 12);
    budget("surface 2x", surface2x, 16);
    budget("descent first", descentFirst, 80);   // O6-03: the four rings' first fill reads the drainage (12000 samples, +15 ms); the frame is under the fade-in
    budget("sprint worst", runWorst, 40);
    return fails;
}

// ---- moon sky test -------------------------------------------------------------
// M1-08: find a partial solar eclipse from a moon of a gas giant and render it.
static void renderEclipse(SpaceRenderer& sr, StarNeighborhood& nb, StarSystem& sys, int bi) {
    const Body& b = sys.bodies[bi];
    // site at the sub-parent point: the parent hangs overhead and the sun passes behind it
    double t0 = 4000.0;
    Mat3 frame = sys.bodyFrame(bi, t0);
    Vec3 toParent = frame * normalize(sys.bodyPos(b.parent, t0) - sys.bodyPos(bi, t0));
    double plat, plon;
    StarSystem::latLonFromBody(toParent, plat, plon);
    SurfaceSite probe;
    probe.init(&sys, bi, clampd(plat, -70 * DEG, 70 * DEG), plon, t0);
    double bestT = t0, bestErr = 1e9, period = b.orbitPeriod;
    for (int i = 0; i < 2400; i++) {
        double t = t0 + period * 3.0 * i / 2400.0;
        SunInfo si = probe.sun(t);
        if (si.altitude < 5 * DEG) continue;
        double err = std::fabs(si.eclipse - 0.85);
        if (err < bestErr) { bestErr = err; bestT = t; }
    }
    SurfaceView sv;
    sv.init(&sys, bi, probe.lat0, probe.lon0, bestT);
    SunInfo si = sv.site.sun(bestT);
    sv.player.yaw = si.azimuth;
    sv.player.pitch = si.altitude - 4 * DEG;
    nb.update(sv.site.worldPos(bestT, 0, 0, 0));
    Input in;
    sv.update(0.016, in, bestT, false);
    Framebuffer fb;
    sv.render(fb, bestT, nb.stars, sr, 1.0);
    fb.mush(2);
    saveFB(fb, "shots/tests/eclipse.png");
    printf("eclipse on %s: coverage %.2f at t=%.0f, sun alt %.1f, sky brightness %.2f -> shots/eclipse.png\n", b.name.c_str(), si.eclipse, bestT, si.altitude / DEG, sv.env.skyBrightness);
}

static void renderMoonSky() {
    SpaceRenderer sr;
    StarNeighborhood nb;
    int shots = 0;
    bool eclipseDone = false;
    for (int64_t x = 150; x < 320 && shots < 3; x++)
        for (int64_t z = 20; z < 140 && shots < 3; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && shots < 3; bi++) {
                const Body& b = sys.bodies[bi];
                if (b.parent < 0 || !PLANET_TYPES[b.type].landable) continue;
                const Body& par = sys.bodies[b.parent];
                if (par.type != PT_GASGIANT && shots < 2) continue;   // first two: gas giant parents
                if (b.orbitRadiusKm / par.radiusKm > 9) continue;
                // land at the sub-planet point: the parent is at the zenith for a locked moon;
                // choose a spot 60 degrees away so it sits mid-sky.
                double t = 4000.0;
                Mat3 frame = sys.bodyFrame(bi, t);
                Vec3 toParent = frame * normalize(sys.bodyPos(b.parent, t) - sys.bodyPos(bi, t));
                double plat, plon;
                StarSystem::latLonFromBody(toParent, plat, plon);
                double lat = clampd(plat, -60 * DEG, 60 * DEG), lon = plon - 55 * DEG;
                SurfaceView sv;
                sv.init(&sys, bi, lat, lon, t);
                warmMaps(sr, sys, b.parent);
                // aim at the parent
                Mat3 L = sv.site.localFrame(t);
                Vec3 obs = sv.site.worldPos(t, 0, 0, 0.002);
                Vec3 dl = L * normalize(sys.bodyPos(b.parent, t) - obs);
                sv.player.yaw = std::atan2(dl.x, dl.z);
                sv.player.pitch = std::asin(dl.y) - 8 * DEG;
                nb.update(obs);
                Input in;
                sv.update(0.016, in, t, false);
                Framebuffer fb;
                sv.render(fb, t, nb.stars, sr, 1.0);
                fb.mush(2);
                std::string fn = "shots/tests/moonsky_" + std::to_string(shots) + ".png";
                saveFB(fb, fn.c_str());
                double ang = std::asin(par.radiusKm / b.orbitRadiusKm) / DEG;
                printf("moon %s (%s) of %s (%s): parent angular radius %.1f deg, alt %.1f, sunAlt %.1f\n", b.name.c_str(), PLANET_TYPES[b.type].name,
                       par.name.c_str(), PLANET_TYPES[par.type].name, ang, std::asin(dl.y) / DEG, sv.env.sun.altitude / DEG);
                if (shots == 0) {
                    // M5-03: the same sky a quarter of an hour later: close moons visibly move
                    double t2 = t + 900.0;
                    sv.init(&sys, bi, lat, lon, t2);
                    sv.player.yaw = std::atan2(dl.x, dl.z); sv.player.pitch = std::asin(dl.y) - 8 * DEG;
                    sv.update(0.016, in, t2, false);
                    Framebuffer fb2;
                    sv.render(fb2, t2, nb.stars, sr, 1.0);
                    fb2.mush(2);
                    saveFB(fb2, "shots/tests/moonsky_0_later.png");
                    Mat3 L2 = sv.site.localFrame(t2);
                    Vec3 dl2 = L2 * normalize(sys.bodyPos(b.parent, t2) - sv.site.worldPos(t2, 0, 0, 0.002));
                    printf("  15 min later: parent alt %.1f -> %.1f, az %.1f -> %.1f (orbit %.1f h) -> shots/moonsky_0_later.png\n", std::asin(dl.y) / DEG, std::asin(dl2.y) / DEG,
                           wrap2pi(std::atan2(dl.x, dl.z)) / DEG, wrap2pi(std::atan2(dl2.x, dl2.z)) / DEG, b.orbitPeriod / 3600);
                }
                shots++;
                if (!eclipseDone && par.type == PT_GASGIANT) { renderEclipse(sr, nb, sys, bi); eclipseDone = true; }
            }
        }
    // M5-04 / M5-02: a moon of a ringed planet, and a moon of a substellar object at night
    bool ringsDone = false, subDone = false;
    for (int64_t x = 150; x < 340 && !(ringsDone && subDone); x++)
        for (int64_t z = 20; z < 160 && !(ringsDone && subDone); z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && !(ringsDone && subDone); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.parent < 0 || !PLANET_TYPES[b.type].landable) continue;
                const Body& par = sys.bodies[b.parent];
                Vec3 orbN(std::sin(b.orbitIncl) * std::cos(b.orbitNode), std::cos(b.orbitIncl), std::sin(b.orbitIncl) * std::sin(b.orbitNode));
                bool wantRings = !ringsDone && par.rings && b.orbitRadiusKm / par.radiusKm < 12 && std::fabs(dot(par.spinAxis, Vec3(0, 1, 0))) < 0.94;   // rings open, not edge-on
                bool wantSub = !subDone && par.type == PT_SUBSTELLAR && b.orbitRadiusKm / par.radiusKm < 14;
                if (!wantRings && !wantSub) continue;
                // sweep the time so the parent stands mid-sky and, for the substellar case, the sun is down
                double tBest = -1; double bestScore = -1e9; double latB = 0, lonB = 0;
                for (int k = 0; k < 96; k++) {
                    double t = 3000.0 + k * b.orbitPeriod / 96.0 * (wantSub ? 1.0 : 0.5);
                    Mat3 frame = sys.bodyFrame(bi, t);
                    Vec3 toParent = frame * normalize(sys.bodyPos(b.parent, t) - sys.bodyPos(bi, t));
                    double plat, plon;
                    StarSystem::latLonFromBody(toParent, plat, plon);
                    double lat = clampd(plat, -60 * DEG, 60 * DEG), lon = plon - 50 * DEG;
                    SurfaceSite site; site.init(&sys, bi, lat, lon, t);
                    double sunAlt = site.sun(t).altitude / DEG;
                    double phase = 0.5 * (1 + dot(normalize(sys.star.pos - sys.bodyPos(b.parent, t)), normalize(sys.bodyPos(bi, t) - sys.bodyPos(b.parent, t))));
                    double opening = std::fabs(dot(normalize(sys.bodyPos(b.parent, t) - sys.bodyPos(bi, t)), par.spinAxis));   // sine of the ring tilt toward us
                    double score = wantSub ? -sunAlt : (sunAlt < 0 ? -1000 : 0) + 100 * phase + 80 * opening;
                    if (score > bestScore) { bestScore = score; tBest = t; latB = lat; lonB = lon; }
                }
                if (tBest < 0) continue;
                double t = tBest;
                SurfaceView sv;
                sv.init(&sys, bi, latB, lonB, t);
                warmMaps(sr, sys, b.parent);
                Mat3 L = sv.site.localFrame(t);
                Vec3 obs = sv.site.worldPos(t, 0, 0, 0.002);
                Vec3 dl = L * normalize(sys.bodyPos(b.parent, t) - obs);
                sv.player.yaw = std::atan2(dl.x, dl.z);
                sv.player.pitch = std::asin(dl.y) - 6 * DEG;
                nb.update(obs);
                Input in;
                sv.update(0.016, in, t, false);
                Framebuffer fb;
                sv.render(fb, t, nb.stars, sr, 1.0);
                fb.mush(2);
                const char* fn = wantSub ? "shots/tests/moonsky_substellar.png" : "shots/tests/moonsky_rings.png";
                saveFB(fb, fn);
                if (!wantSub) {
                    // the same view rendered by the space renderer (no sky), to compare the ring drawing paths
                    Framebuffer fs;
                    SpaceContext c;
                    c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = obs;
                    c.cam = sv.testCamWorld(t);
                    c.bankBodyA = b.parent;
                    sr.setupPalette(fs, &sys, b.parent, -1, 1.0);
                    sr.render(fs, c);
                    fs.mush(2);
                    saveFB(fs, "shots/tests/moonsky_rings_space.png");
                    double opening = std::fabs(dot(normalize(sys.bodyPos(b.parent, t) - obs), par.spinAxis));
                    printf("  ring opening %.1f deg, ring %.2f-%.2f R -> shots/moonsky_rings_space.png\n", std::asin(opening) / DEG, par.ringInner, par.ringOuter);
                }
                printf("%s: moon %s (%s) of %s (%s), parent alt %.1f, sun alt %.1f, night glow %.2f -> %s\n", wantSub ? "substellar" : "rings", b.name.c_str(), PLANET_TYPES[b.type].name,
                       par.name.c_str(), PLANET_TYPES[par.type].name, std::asin(dl.y) / DEG, sv.env.sun.altitude / DEG, sv.env.moonLight, fn);
                if (wantSub) subDone = true; else ringsDone = true;
            }
        }
    // M5-04: the rings of a ringed world seen from its own surface, by day and by night
    bool surfDone = false;
    for (int64_t x = 150; x < 340 && !surfDone; x++)
        for (int64_t z = 20; z < 160 && !surfDone; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size() && !surfDone; bi++) {
                const Body& b = sys.bodies[bi];
                if (!b.rings || !PLANET_TYPES[b.type].landable || b.parent >= 0) continue;
                for (int k = 0; k < 2; k++) {
                    double lat = 28 * DEG, lon = 1.0;
                    SurfaceView sv;
                    sv.init(&sys, bi, lat, lon, 1000.0);
                    double t = findTime(sv.site, k == 0 ? 30 * DEG : -40 * DEG, 1000.0);
                    sv.init(&sys, bi, lat, lon, t);
                    // look toward the equator (the ring's arc), a little up
                    sv.player.yaw = lat > 0 ? PI : 0; sv.player.pitch = 40 * DEG;
                    Vec3 obs = sv.site.worldPos(t, 0, 0, 0.002);
                    nb.update(obs);
                    Input in;
                    sv.update(0.016, in, t, false);
                    Framebuffer fb;
                    sv.render(fb, t, nb.stars, sr, 1.0);
                    fb.mush(2);
                    std::string fn = k == 0 ? "shots/tests/rings_surface_day.png" : "shots/tests/rings_surface_night.png";
                    saveFB(fb, fn.c_str());
                    printf("rings from the surface of %s (%s, tilt %.0f deg, ring %.2f-%.2f R): sun alt %.1f -> %s\n", b.name.c_str(), PLANET_TYPES[b.type].name, b.axialTilt / DEG, b.ringInner, b.ringOuter, sv.env.sun.altitude / DEG, fn.c_str());
                }
                surfDone = true;
            }
        }
}

// M5-01: a surface with two suns up (part of `moonsky`)
static void renderBinarySurface(SpaceRenderer& sr, StarNeighborhood& nb) {
    for (int64_t x = 150; x < 340; x++)
        for (int64_t z = 20; z < 160; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            if (sys.companion < 0) continue;
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (!PLANET_TYPES[b.type].landable || b.parent >= 0) continue;
                // sweep sites and times for both suns above 12 degrees with the companion a visible disc
                double bestScore = -1e9, tB = 0, latB = 0, lonB = 0;
                for (int k = 0; k < 160; k++) {
                    double t = 2000.0 + k * 1811.0;
                    double lat = 20 * DEG, lon = k * 0.7;
                    SurfaceSite site; site.init(&sys, bi, lat, lon, t);
                    SunInfo s1 = site.sun(t), s2;
                    if (!site.sun2(t, s2)) continue;
                    if (s2.angularRadius < 0.08 * DEG) continue;
                    double score = std::min(s1.altitude, s2.altitude) / DEG;
                    if (s1.altitude < 12 * DEG || s2.altitude < 12 * DEG) score -= 100;
                    if (score > bestScore) { bestScore = score; tB = t; latB = lat; lonB = lon; }
                }
                if (bestScore < 0) continue;
                SurfaceView sv;
                sv.init(&sys, bi, latB, lonB, tB);
                SunInfo s1 = sv.site.sun(tB), s2; sv.site.sun2(tB, s2);
                // look between the two suns
                Vec3 mid = normalize(s1.dirLocal + s2.dirLocal);
                sv.player.yaw = std::atan2(mid.x, mid.z); sv.player.pitch = std::asin(mid.y) - 12 * DEG;
                Vec3 obs = sv.site.worldPos(tB, 0, 0, 0.002);
                nb.update(obs);
                Input in;
                sv.update(0.016, in, tB, false);
                Framebuffer fb;
                sv.render(fb, tB, nb.stars, sr, 1.0);
                fb.mush(2);
                saveFB(fb, "shots/tests/binary_surface.png");
                printf("binary surface: %s (%s) of %s: primary alt %.0f az %.0f, companion %s alt %.0f az %.0f radius %.2f deg, share %.2f -> shots/binary_surface.png\n", b.name.c_str(), PLANET_TYPES[b.type].name,
                       sys.classString().c_str(), s1.altitude / DEG, s1.azimuth / DEG, STAR_CLASSES[sys.bodies[sys.companion].starClass].name, s2.altitude / DEG, s2.azimuth / DEG, s2.angularRadius / DEG, sv.env.sun2Share);
                return;
            }
        }
    printf("binary surface: none found\n");
}

// M7-03 unit tests: frame maths, the sun against analytic cases, noise statistics, name lengths, the Kepler solver
static int testUnit() {
    int fails = 0;
    auto check = [&](const char* name, bool ok, const std::string& detail = "") { printf("  %-34s %s %s\n", name, ok ? "ok" : "FAIL", detail.c_str()); if (!ok) fails++; };
    {   // O6-02: the analytic gradient of gnoise3d matches central differences and its value is gnoise3's
        double worst = 0, worstV = 0; Rng r(0x616);
        for (int i = 0; i < 200; i++) {
            double x = r.range(-50, 50), y = r.range(-50, 50), z = r.range(-50, 50); uint64_t sd = r.next();
            Vec3 g; double v = gnoise3d(x, y, z, sd, g);
            worstV = std::max(worstV, std::fabs(v - gnoise3(x, y, z, sd)));
            const double e = 1e-5;
            Vec3 fd((gnoise3(x + e, y, z, sd) - gnoise3(x - e, y, z, sd)) / (2 * e), (gnoise3(x, y + e, z, sd) - gnoise3(x, y - e, z, sd)) / (2 * e), (gnoise3(x, y, z + e, sd) - gnoise3(x, y, z - e, sd)) / (2 * e));
            worst = std::max(worst, length(fd - g));
        }
        check("gnoise3d value equals gnoise3", worstV < 1e-12, "worst " + std::to_string(worstV));
        check("gnoise3d gradient by finite differences", worst < 1e-5, "worst " + std::to_string(worst));
    }
    {   // O6-02: the relief spectrum is deterministic and its analytic slope tracks the finite difference of its height
        Body b; b.type = PT_FELISIAN; b.seed = 0xC0FFEEULL; b.radiusKm = 5000; b.tempK = 285; b.color = RGB(0.3f, 0.5f, 0.4f);
        BodyGen g = BodyGen::make(b);
        Rng r(0x617); double sumErr = 0, sumFd = 0; bool det = true;
        for (int i = 0; i < 24; i++) {
            Vec3 u = normalize(Vec3(r.sym(1), r.sym(1), r.sym(1)));
            Relief r1 = reliefAt(g, u, 1400, 0.5, 16), r2 = reliefAt(g, u, 1400, 0.5, 16);
            if (r1.h != r2.h || r1.slope != r2.slope || std::fabs(r1.h) > 1400 * 4) det = false;
            Vec3 e = normalize(cross(u, std::fabs(u.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), n = cross(u, e);
            double dm = 2.0 / (g.R * 1000);
            double hx = (reliefAt(g, normalize(u + e * dm), 1400, 0.5, 16).h - reliefAt(g, normalize(u - e * dm), 1400, 0.5, 16).h) / 4.0;
            double hz = (reliefAt(g, normalize(u + n * dm), 1400, 0.5, 16).h - reliefAt(g, normalize(u - n * dm), 1400, 0.5, 16).h) / 4.0;
            double fd = std::sqrt(hx * hx + hz * hz);
            sumErr += std::fabs(fd - r1.slope); sumFd += fd;
        }
        check("relief deterministic and bounded", det);
        check("relief slope tracks finite differences", sumErr < 0.4 * sumFd, "mean error " + std::to_string(sumErr / 24) + " of " + std::to_string(sumFd / 24));
    }
    // N1-01: every material of every type gets a colour, the same for the same seed
    {
        bool ok = true; std::string why;
        for (int ty = 0; ty < PT_COUNT && ok; ty++) {
            Body b; b.type = ty; b.seed = 0xABCD1234ULL + ty; b.radiusKm = 3000; b.tempK = 280;
            b.color = RGB(0.3f + 0.03f * ty, 0.62f, 0.5f);   // R-307: stays under 1 for the 22 types
            BodyGen g1 = BodyGen::make(b), g2 = BodyGen::make(b);
            for (int mm = 0; mm < MAT_COUNT && ok; mm++) {
                const RGB& c = g1.matColor[mm];
                if (!(c.r >= 0 && c.g >= 0 && c.b >= 0 && c.r + c.g + c.b > 0.05 && c.r <= 1.001 && c.g <= 1.001 && c.b <= 1.001)) { ok = false; why = std::string(PLANET_TYPES[ty].name) + "/" + MATERIAL_NAMES[mm]; }
                if (std::fabs(c.r - g2.matColor[mm].r) > 1e-9 || std::fabs(c.g - g2.matColor[mm].g) > 1e-9) { ok = false; why = "not deterministic"; }
            }
        }
        check("material palette per type", ok, why);
        check("noon exposure rule", noonStop(0.4) > 37 && noonStop(0.4) < 38 && noonStop(1.0) == 60 && noonStop(0.0) == 25, std::to_string(noonStop(0.4)));
        RGB rc = materialRampColor(RGB(1, 0.5f, 0), 47);
        check("material ramp hits c at 47", std::fabs(rc.r - 1) < 1e-6 && std::fabs(rc.g - 0.5) < 1e-6, "");
    }
    // lat/lon round trips on the sphere
    {
        double worst = 0;
        for (int i = 0; i < 200; i++) {
            double lat = (i / 200.0 - 0.5) * PI * 0.98, lon = std::fmod(i * 0.7, TAU) - PI;
            double la2, lo2; StarSystem::latLonFromBody(StarSystem::bodyFromLatLon(lat, lon), la2, lo2);
            worst = std::max(worst, std::fabs(la2 - lat) + std::fabs(wrapAngle(lo2 - lon)));
        }
        check("lat/lon round trip", worst < 1e-9, "worst " + std::to_string(worst));
    }
    Star s; StarSystem sys; int bi = -1;
    for (int64_t x = 150; x < 300 && bi < 0; x++)
        for (int64_t z = 20; z < 120 && bi < 0; z++) {
            if (!starInSector(x, 0, z, s)) continue;
            sys.generate(s);
            for (int i = 0; i < (int)sys.bodies.size(); i++) if (PLANET_TYPES[sys.bodies[i].type].landable && sys.bodies[i].parent < 0 && sys.bodies[i].ecc < 0.01) { bi = i; break; }
        }
    check("a landable planet found", bi >= 0);
    if (bi >= 0) {
        // the site frame: local metres -> unit vector -> local metres
        SurfaceSite site; site.init(&sys, bi, 0.4, 1.1, 0.0);
        double worst = 0;
        for (int i = 0; i < 100; i++) {
            double x = (i - 50) * 137.0, z = (i * 71) % 5000 - 2500.0, x2, z2;
            site.localAt(site.unitAt(x, z), x2, z2);
            worst = std::max(worst, std::fabs(x2 - x) + std::fabs(z2 - z));
        }
        check("site local/unit round trip", worst < 1e-6, "worst m " + std::to_string(worst));
        // the sun: at the sub-solar point it stands at the zenith; 30 degrees away along the meridian at 60 degrees
        double t = 5000.0;
        Vec3 sunBody = sys.bodyFrame(bi, t) * normalize(sys.star.pos - sys.bodyPos(bi, t));
        double slat, slon; StarSystem::latLonFromBody(sunBody, slat, slon);
        SurfaceSite sub; sub.init(&sys, bi, slat, slon, t);
        double altZen = sub.sun(t).altitude / DEG;
        double lat2 = slat + 30 * DEG; bool north = lat2 < 85 * DEG; if (!north) lat2 = slat - 30 * DEG;
        SurfaceSite off; off.init(&sys, bi, lat2, slon, t);
        double alt30 = off.sun(t).altitude / DEG, az30 = off.sun(t).azimuth / DEG;
        check("sun at the zenith over the sub-solar point", std::fabs(altZen - 90) < 0.05, "alt " + std::to_string(altZen));
        check("sun at 60 deg 30 deg along the meridian", std::fabs(alt30 - 60) < 0.05, "alt " + std::to_string(alt30) + " az " + std::to_string(az30));
        check("sun due south/north from there", north ? std::fabs(az30 - 180) < 0.5 : (az30 < 0.5 || az30 > 359.5), "az " + std::to_string(az30));
        // local noon: dayFraction 0.5 at the sub-solar longitude
        check("local time at the sub-solar point is noon", std::fabs(sub.sun(t).dayFraction - 0.5) < 0.002, std::to_string(sub.sun(t).dayFraction));
    }
    // Kepler solver residuals
    {
        double worst = 0;
        for (int i = 0; i < 400; i++) {
            double e = (i % 20) / 20.0 * 0.95, M = (i / 20) * 0.31;
            double rOverA; double nu = StarSystem::trueAnomaly(M, e, rOverA);
            double E = 2 * std::atan2(std::sqrt(1 - e) * std::sin(nu / 2), std::sqrt(1 + e) * std::cos(nu / 2));
            double resid = std::fabs(wrapAngle(E - e * std::sin(E) - M));
            worst = std::max(worst, resid);
            worst = std::max(worst, std::fabs(rOverA - (1 - e * std::cos(E))));
        }
        check("Kepler equation residual", worst < 1e-8, "worst " + std::to_string(worst));
        double r0; check("circular orbit is the identity", std::fabs(StarSystem::trueAnomaly(1.234, 0, r0) - 1.234) < 1e-12 && r0 == 1);
    }
    // noise statistics: zero mean, bounded, hash uniform
    {
        double sum = 0, lo = 1e9, hi = -1e9; int n = 0;
        for (int i = 0; i < 20000; i++) { double v = gnoise3(i * 0.371, i * 0.173 + 5, i * 0.091 - 3, 77); sum += v; lo = std::min(lo, v); hi = std::max(hi, v); n++; }
        check("gnoise3 mean near zero", std::fabs(sum / n) < 0.03, std::to_string(sum / n));
        check("gnoise3 within [-1.1, 1.1]", lo > -1.1 && hi < 1.1, std::to_string(lo) + ".." + std::to_string(hi));
        double hs = 0, hs2 = 0;
        for (int i = 0; i < 100000; i++) { double u = unitFromHash(mix64(i * 0x9E3779B97F4A7C15ULL + 11)); hs += u; hs2 += u * u; }
        double mean = hs / 100000, var = hs2 / 100000 - mean * mean;
        check("hash uniform (mean 0.5, var 1/12)", std::fabs(mean - 0.5) < 0.005 && std::fabs(var - 1.0 / 12) < 0.003, std::to_string(mean) + " " + std::to_string(var));
        double a = fbm3(Vec3(1.5, 2.5, 3.5), 9, 5), b = fbm3(Vec3(1.5, 2.5, 3.5), 9, 5);
        check("noise is deterministic", a == b);
    }
    // names: bounded length, letters and spaces only, deterministic
    {
        size_t lo = 99, hi = 0; bool clean = true;
        for (int i = 0; i < 3000; i++) {
            std::string nm = generateName(mix64(i + 1), i % 3);
            lo = std::min(lo, nm.size()); hi = std::max(hi, nm.size());
            for (char ch : nm) if (!(isalpha((unsigned char)ch) || ch == ' ' || ch == '\'')) clean = false;
        }
        check("name lengths 3..16", lo >= 3 && hi <= 16, std::to_string(lo) + ".." + std::to_string(hi));
        check("names are letters, spaces, apostrophes", clean);
        check("names deterministic", generateName(42, 1) == generateName(42, 1));
    }
    // B-205 / R-204: a new landing starts with no buggy, B replaces a buggy left anywhere, the nose camera cannot look back
    if (bi >= 0) {
        SurfaceView sv;
        sv.init(&sys, bi, 0.4, 1.1, 0.0);
        sv.relocateCapsule(sv.player.x + 4, sv.player.z + 2);
        check("buggy deploys at the capsule", sv.deployBuggy() && sv.buggy.deployed);
        sv.buggy.unfold = 1; sv.buggy.x += 300; sv.buggy.odometer = 500;   // as if left 300 m away after a drive
        check("B replaces a buggy left far away", sv.deployBuggy() && sv.buggy.odometer == 0 && sv.buggyDist() < 12, "dist " + std::to_string(sv.buggyDist()));
        sv.buggy.unfold = 1; sv.player.x = sv.buggy.x + 1.5; sv.player.z = sv.buggy.z;
        check("get in", sv.toggleBuggy());
        Input in; sv.player.yaw = sv.buggy.heading + PI; sv.player.pitch = -80 * DEG;
        sv.update(1.0 / 60, in, 0.0, true);
        double look = std::fabs(wrapAngle(sv.player.yaw - sv.buggy.heading));
        check("nose camera pans 70 deg at most", look <= SurfaceView::CAM_PAN + 1e-9 && sv.player.pitch >= -SurfaceView::CAM_TILT_DOWN - 1e-9, "look " + std::to_string(look / DEG) + " tilt " + std::to_string(sv.player.pitch / DEG));
        sv.init(&sys, bi, 0.5, 1.2, 0.0);
        check("no buggy on a new landing", !sv.buggy.deployed && !sv.inBuggy);
    }
    // B-303: a new landing clears the trail and the waypoint; a re-anchor carries both to the new frame at the same place
    if (bi >= 0) {
        SurfaceView sv;
        sv.init(&sys, bi, 0.4, 1.1, 0.0);
        Input in;
        for (int i = 0; i < 40; i++) { sv.player.x += 30; sv.update(1.0 / 30, in, i / 30.0, false); }   // the trail collects points every 25 m
        sv.hasWaypoint = true; sv.wpX = 900; sv.wpZ = -400;
        Vec3 wpU = sv.site.unitAt(sv.wpX, sv.wpZ), trU = sv.site.unitAt(sv.trail[3].first, sv.trail[3].second);
        size_t nTrail = sv.trail.size();
        check("the trail collects points", nTrail >= 30, std::to_string(nTrail));
        sv.player.x = 51e3; sv.update(1.0 / 30, in, 2.0, false);   // beyond 50 km: reanchor
        double wx, wz, tx, tz; sv.site.localAt(wpU, wx, wz); sv.site.localAt(trU, tx, tz);
        check("reanchor keeps the waypoint in place", sv.hasWaypoint && std::fabs(wx - sv.wpX) < 0.05 && std::fabs(wz - sv.wpZ) < 0.05 && std::fabs(sv.wpX) > 1000, "wp " + std::to_string(sv.wpX) + "," + std::to_string(sv.wpZ));
        check("reanchor keeps the trail in place", sv.trail.size() >= nTrail && std::fabs(tx - sv.trail[3].first) < 0.5 && std::fabs(tz - sv.trail[3].second) < 0.5, std::to_string(sv.trail.size()) + " points");   // one more: the point at the new origin
        int epoch = sv.siteEpoch;
        sv.init(&sys, bi, 0.5, 1.2, 0.0);
        check("a new landing clears the trail and the waypoint", sv.trail.empty() && !sv.hasWaypoint && sv.siteEpoch > epoch);
    }
    // B-206: on a thin-atmosphere world in a dust storm the wind stays a wind (it compounded every frame to 1e13 knots)
    {
        Star ts; StarSystem tsys; int tb = -1;
        for (int64_t x = 150; x < 300 && tb < 0; x++)
            for (int64_t z = 20; z < 120 && tb < 0; z++) {
                if (!starInSector(x, 0, z, ts)) continue;
                tsys.generate(ts);
                for (int i = 0; i < (int)tsys.bodies.size(); i++) if (tsys.bodies[i].type == PT_THINATMO) { tb = i; break; }
            }
        check("a thin-atmosphere world found", tb >= 0);
        if (tb >= 0) {
            SurfaceView sv;
            double t0 = 0; double dustAt = 0;
            for (double tt = 0; tt < 4e5 && dustAt < 0.3; tt += 300) { sv.init(&tsys, tb, 0.3, 0.9, tt); dustAt = sv.env.dust; t0 = tt; }
            check("a dust storm found", dustAt >= 0.3, "dust " + std::to_string(dustAt));
            Input in; double worst = 0;
            for (int i = 0; i < 300; i++) { sv.update(1.0 / 30, in, t0 + i / 30.0, true); worst = std::max(worst, sv.env.windKnots); }
            check("storm wind under 200 knots", std::isfinite(worst) && worst < 200 && worst > 5, "worst " + std::to_string(worst));
        }
    }
    // the classic case: a body's rotation period sets its day; seasonOf is bounded
    if (bi >= 0) {
        double mn = 2, mx = -2;
        for (int k = 0; k < 64; k++) { double v = sys.seasonOf(bi, k * sys.bodies[bi].orbitPeriod / 64); mn = std::min(mn, v); mx = std::max(mx, v); }
        check("seasonOf bounded by the tilt", mx <= std::sin(sys.bodies[bi].axialTilt) + 1e-9 && mn >= -std::sin(sys.bodies[bi].axialTilt) - 1e-9);
    }
    // M7-02: the map cache evicts the least recently used map once the cap is reached
    {
        SpaceRenderer sr;
        sr.mapCacheCap = 3;
        int n = 0;
        for (int64_t x = 150; x < 300 && n < 6; x++)
            for (int64_t z = 20; z < 120 && n < 6; z++) {
                Star st;
                if (!starInSector(x, 0, z, st)) continue;
                StarSystem sy; sy.generate(st);
                for (const Body& b : sy.bodies) { if (n >= 6) break; if (b.type == PT_COMPANION) continue; sr.mapFor(b); n++; }
            }
        check("map cache honours its cap", sr.mapsHeld() <= 3 && sr.mapStats.evicted >= 3, std::to_string(sr.mapsHeld()) + " held, " + std::to_string(sr.mapStats.evicted) + " evicted");
    }
    // O0-02 (B-305): 32 banks, and the second world's families have their own
    check("32 palette banks of 2048 intensities", BANKS == 32 && INTEN_MASK == 2047 && INTEN_PER_SHADE == 32 && (BANK_MASK >> BANK_SHIFT) == 31);
    check("body B's sand has its own bank", SpaceRenderer::globeBank(3, MAT_SAND) == 17 && SpaceRenderer::globeBank(2, MAT_SAND) == 13 && SpaceRenderer::globeBank(3, MAT_SNOW) == 18 && SpaceRenderer::globeBank(3, MAT_FOREST) == 16);
    // O0-03 (B-304): a globe whose centre is behind the camera plane still draws its limb
    if (bi >= 0) {
        SpaceRenderer sr; StarNeighborhood nb;
        double t = 3000.0;
        const Body& b = sys.bodies[bi];
        Vec3 bp = sys.bodyPos(bi, t);
        Vec3 dir = normalize(Vec3(0.3, 0.2, 0.93));
        Vec3 shipPos = bp + dir * (1.15 * b.radiusKm);   // the disc is 121 degrees across from here
        Vec3 toBody = -dir, side = normalize(cross(toBody, Vec3(0, 1, 0)));
        Vec3 fwd = normalize(toBody * std::cos(92 * DEG) + side * std::sin(92 * DEG));   // the centre 92 degrees off the view axis: behind the camera plane, the limb 31 degrees off
        nb.update(shipPos);
        Framebuffer fb; SpaceContext c;
        c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
        c.bankBodyA = bi;
        warmMaps(sr, sys, bi);
        sr.setupPalette(fb, &sys, bi, -1, 1.0);
        sr.render(fb, c);
        int lit = 0;
        for (Pix p : fb.idx) { int bk = bankOf(p); if ((bk == 2 || bk == 6 || bk == 11 || bk == 13 || bk == 14 || bk == 15) && intenOf(p) > 0) lit++; }
        check("a globe with its centre behind the camera still shows its limb", lit > 200, std::to_string(lit) + " pixels");
    }
    // B-308: a companion star beside or behind the camera never floods the frame; ahead it is a disc of the right size
    {
        Star cs; StarSystem csys; int ck = -1;
        for (int64_t x = 150; x < 320 && ck < 0; x++)
            for (int64_t z = 20; z < 140 && ck < 0; z++) { if (!starInSector(x, 0, z, cs)) continue; csys.generate(cs); if (csys.companion >= 0) ck = csys.companion; }
        check("a system with a companion star found", ck >= 0);
        if (ck >= 0) {
            SpaceRenderer sr; StarNeighborhood nb; double t = 3000.0;
            const Body& k = csys.bodies[ck];
            Vec3 kp = csys.bodyPos(ck, t);
            Vec3 dir = normalize(Vec3(0.3, 0.2, 0.93));
            Vec3 shipPos = kp + dir * (3.5 * k.radiusKm);   // where the ship parks (Game::parkDistanceFor)
            Vec3 toStar = -dir, side = normalize(cross(toStar, Vec3(0, 1, 0)));
            nb.update(shipPos);
            double angR = std::asin(1.0 / 3.5);
            double expect = PI * std::pow(sr.proj.f * std::tan(angR), 2) / (FBW * FBH);   // the disc's share of the frame when centred
            const int offs[4] = {0, 70, 95, 150};
            std::vector<uint32_t> strip((size_t)FBW * 4 * FBH, 0xFF101010u), px((size_t)FBW * FBH);
            double frac[4] = {0}, mean[4] = {0};
            for (int i = 0; i < 4; i++) {
                double a = offs[i] * DEG;
                Vec3 fwd = normalize(toStar * std::cos(a) + side * std::sin(a));
                Framebuffer fb; SpaceContext c;
                c.sys = &csys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
                sr.setupPalette(fb, &csys, -1, -1, 1.0);
                sr.render(fb, c);
                int disc = 0; double sum = 0;
                for (Pix q : fb.idx) { if (bankOf(q) == 12 && shadeOf(q) > 25) disc++; sum += shadeOf(q); }
                frac[i] = (double)disc / (FBW * FBH); mean[i] = sum / (FBW * FBH);
                fb.mush(2); fb.toRGB(px.data(), 1.0);
                for (int y = 0; y < FBH; y++) memcpy(&strip[(size_t)y * FBW * 4 + i * FBW], &px[(size_t)y * FBW], FBW * 4);
            }
            writePNG("shots/tests/unit_companion_swing.png", strip.data(), FBW * 4, FBH);
            check("the companion ahead is a disc of the expected size", frac[0] > 0.5 * expect && frac[0] < 1.6 * expect, fmt("%.1f%% of the frame, expected %.1f%%", frac[0] * 100, expect * 100));
            check("the companion 70 deg off axis leaves no disc", frac[1] == 0 && mean[1] < 8, fmt("disc %.2f%%, mean shade %.1f", frac[1] * 100, mean[1]));
            check("the companion beside the camera (95 deg) does not flood", frac[2] == 0 && mean[2] < 6, fmt("disc %.2f%%, mean shade %.1f", frac[2] * 100, mean[2]));
            check("the companion behind (150 deg) does not flood", frac[3] == 0 && mean[3] < 4, fmt("disc %.2f%%, mean shade %.1f", frac[3] * 100, mean[3]));
        }
    }
    // B-312: a big ringed globe whose centre lies outside the frame to the left or the right still shows its limb (its
    // projection is a conic that stretches away from the centre; the old box round the projected centre cut it off);
    // B-317: in a sky the bodies' depths are written in metres, beyond the farthest terrain (they were kilometres, so a
    // moon 20,000 km away hid the ground beyond 20 km)
    {
        Star rs; StarSystem rsys; int rb = -1;
        for (int64_t x = 150; x < 320 && rb < 0; x++)
            for (int64_t z = 20; z < 140 && rb < 0; z++) {
                if (!starInSector(x, 0, z, rs)) continue;
                rsys.generate(rs);
                for (int i = 0; i < (int)rsys.bodies.size() && rb < 0; i++) if (rsys.bodies[i].rings && rsys.bodies[i].type != PT_COMPANION) rb = i;
            }
        check("a ringed world found", rb >= 0);
        if (rb >= 0) {
            SpaceRenderer sr; StarNeighborhood nb; double t = 3000.0;
            const Body& b = rsys.bodies[rb];
            Vec3 bp = rsys.bodyPos(rb, t);
            Vec3 dir = normalize(Vec3(0.3, 0.2, 0.93));
            Vec3 shipPos = bp + dir * (3.6 * b.radiusKm);   // the disc is 32 degrees across from here
            Vec3 toBody = -dir, side = normalize(cross(toBody, Vec3(0, 1, 0)));
            nb.update(shipPos);
            warmMaps(sr, rsys, rb);
            int lit[3] = {0, 0, 0};
            float deepest = 0;
            const double offs[3] = {-45 * DEG, 45 * DEG, 45 * DEG};   // the centre 10 degrees outside the frame on either side, the limb 6 degrees inside
            for (int i = 0; i < 3; i++) {
                Vec3 fwd = normalize(toBody * std::cos(offs[i]) + side * std::sin(offs[i]));
                Framebuffer fb; SpaceContext c;
                c.sys = &rsys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
                c.bankBodyA = rb;
                sr.setupPalette(fb, &rsys, rb, -1, 1.0);
                if (i < 2) {
                    sr.render(fb, c);
                    for (Pix p : fb.idx) { int bk = bankOf(p); if ((bk == 2 || bk == 6 || bk == 11 || bk == 13 || bk == 14 || bk == 15) && intenOf(p) > 0) lit[i]++; }
                } else {   // the same view as a moon's sky sees it (drawSkyBodies: the sky bank, depth in metres)
                    c.skyMode = true; c.skyDark = 1; c.excludeBody = -1;
                    sr.drawSkyBodies(fb, c);
                    for (size_t o = 0; o < fb.idx.size(); o++) { if (bankOf(fb.idx[o]) == 1 && shadeOf(fb.idx[o]) > 35) lit[i]++; deepest = std::max(deepest, fb.invz[o]); }
                }
            }
            check("a globe centred left of the frame shows its limb", lit[0] > 100, std::to_string(lit[0]) + " pixels");
            check("a globe centred right of the frame shows its limb", lit[1] > 100, std::to_string(lit[1]) + " pixels");
            check("the same globe in a sky shows its limb", lit[2] > 100, std::to_string(lit[2]) + " pixels");
            check("sky bodies lie beyond the far ring", deepest > 0 && deepest < 1.0f / 53000.0f, fmt("nearest 1/%.0f m", deepest > 0 ? 1.0 / deepest : 0.0));
            // a small body close by: at 3.6 radii of a 1000 km moon the old kilometre depth read as 3,600 m
            int sb = -1;
            for (int i = 0; i < (int)rsys.bodies.size() && sb < 0; i++) if (rsys.bodies[i].type != PT_COMPANION && rsys.bodies[i].radiusKm < 3000) sb = i;
            if (sb >= 0) {
                Vec3 sp = rsys.bodyPos(sb, t);
                Vec3 ship2 = sp + dir * (3.6 * rsys.bodies[sb].radiusKm);
                Framebuffer fb; SpaceContext c;
                c.sys = &rsys; c.stars = &nb.stars; c.t = t; c.shipPos = ship2; c.cam = cameraBasis(std::atan2(toBody.x, toBody.z), std::asin(clampd(toBody.y, -1, 1)));
                c.skyMode = true; c.skyDark = 1; c.excludeBody = -1;
                sr.setupPalette(fb, &rsys, sb, -1, 1.0);
                sr.drawSkyBodies(fb, c);
                float deep2 = 0; for (float z : fb.invz) deep2 = std::max(deep2, z);
                check("a small body close by lies beyond the far ring too", deep2 > 0 && deep2 < 1.0f / 53000.0f, fmt("nearest 1/%.0f m (it would read 1/%.0f in kilometres)", deep2 > 0 ? 1.0 / deep2 : 0.0, 3.6 * rsys.bodies[sb].radiusKm));
            }
        }
    }
    // B-313: the terrain rings meet at their edges (geomorphing): a ring's edge vertices are drawn on the coarser ring's
    // surface, where the unmorphed vertices differed by metres to hundreds of metres (the cells popped as the edge swept)
    {
        SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
        bool ok = setupSceneForType(PT_FELISIAN, 0, 40 * DEG, 0.0, 0.02, sv, sys, nb, t, -20);
        check("the mountain review site set up", ok);
        if (ok) {
            Framebuffer fb; Input in;
            sv.update(1.0 / 30, in, t, false);
            sv.render(fb, t, nb.stars, sr, 1.0);
            double seam[4], raw[4];
            sv.testRingSeams(seam, raw);
            const char* names[4] = {"near ring on lod0", "lod0 on lod1", "lod1 on lod2", "lod2 on lod3"};
            for (int k = 0; k < 4; k++)
                check((std::string("rings meet: ") + names[k]).c_str(), seam[k] >= 0 && seam[k] < 0.01, fmt("gap %.4f m (unmorphed %.1f m)", seam[k], raw[k]));
        }
    }
    // B-316: the habitat cells put herds round the explorer wherever the biome has them, not only at the landing
    {
        SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
        bool ok = setupSceneForType(PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, sv, sys, nb, t, -3);
        check("a grassland herd site set up", ok);
        if (ok) {
            Input in;
            for (int i = 0; i < 3; i++) sv.update(1.0 / 30, in, t + i, false);
            int origin = 0, cells = 0;
            for (const SurfaceView::Herd& h : sv.herds) (h.origin ? origin : cells)++;
            check("the landing's own herds are there", origin >= 1, std::to_string(origin) + " herds");
            check("habitat herds spawned within 900 m", cells >= 1, std::to_string(cells) + " herds, " + std::to_string(sv.critters.size()) + " animals in all");
            // walk 3 km away: the landing's herds stay listed, the cells behind are forgotten and new ones spawn
            sv.player.x += 3000; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
            for (int i = 0; i < 3; i++) sv.update(1.0 / 30, in, t + 10 + i, false);
            int far = 0, near = 0;
            for (const SurfaceView::Herd& h : sv.herds) { if (h.origin) continue; double dx = h.cx - sv.player.x, dz = h.cz - sv.player.z; (dx * dx + dz * dz > 1300.0 * 1300.0 ? far : near)++; }
            check("3 km on, the herds behind are forgotten", far == 0, std::to_string(far) + " herds beyond 1300 m");
            check("3 km on, new herds are within reach", near >= 1, std::to_string(near) + " herds within 1300 m");
            bool consistent = true;
            for (const Critter& c : sv.critters) if (c.herd < 0 || c.herd >= (int)sv.herds.size()) consistent = false;
            check("every animal belongs to a listed herd", consistent);
        }
    }
    // O0-04 (B-306): the landing map carries the shadow of a moon at some point of the moon's orbit
    {
        Star ms; StarSystem msys; int mb = -1;
        for (int64_t x = 150; x < 320 && mb < 0; x++)
            for (int64_t z = 20; z < 140 && mb < 0; z++) {
                if (!starInSector(x, 0, z, ms)) continue;
                msys.generate(ms);
                for (int i = 0; i < (int)msys.bodies.size(); i++) if (msys.bodies[i].parent < 0 && msys.bodies[i].moonCount > 0 && PLANET_TYPES[msys.bodies[i].type].landable && msys.bodies[i].orbitIncl < 3 * DEG) { mb = i; break; }
            }
        check("a landable planet with a moon found", mb >= 0);
        if (mb >= 0) {
            Game g; g.savePrefix = "shots/tests/unit_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.guidePath = "shots/tests/unit_guide.txt";
            g.testParkAt(ms, mb);
            int moon = -1; for (int i = 0; i < (int)msys.bodies.size(); i++) if (msys.bodies[i].parent == mb) { moon = i; break; }
            double P = msys.bodies[moon].orbitPeriod, mx = 0;
            for (int k = 0; k < 96 && mx < 0.5; k++) { g.testSetTime(3.6e6 + k * P / 96.0); mx = std::max(mx, g.testMapShadowMax()); }
            check("the moon's shadow crosses the landing map", mx >= 0.5, "darkest " + std::to_string(mx));
        }
    }
    // O2 (R-301): sector names
    {
        int sx, sy;
        sectorIndexOf(0, 0, sx, sy); check("sector of 0,0 is 180:090", sx == 180 && sy == 90);
        sectorIndexOf(-89.9, -179.9, sx, sy); check("sector of the south-west corner is 000:000", sx == 0 && sy == 0);
        sectorIndexOf(89.9, 179.99, sx, sy); check("sector of the north-east corner is 359:179", sx == 359 && sy == 179);
    }
    // O0-01 (B-301): a ringed landable planet exists near the start; its ring profile is shared and bounded; the ring's
    // shadow falls on its ground somewhere at low latitude
    {
        Star rs; StarSystem rsys; int rb = -1;
        for (int64_t x = 150; x < 340 && rb < 0; x++)
            for (int64_t z = 20; z < 160 && rb < 0; z++) {
                if (!starInSector(x, 0, z, rs)) continue;
                rsys.generate(rs);
                for (int i = 0; i < (int)rsys.bodies.size(); i++) if (rsys.bodies[i].rings && rsys.bodies[i].parent < 0 && PLANET_TYPES[rsys.bodies[i].type].landable) { rb = i; break; }
            }
        check("a ringed landable planet found", rb >= 0);
        if (rb >= 0) {
            std::vector<float> prof = rsys.ringProfileOf(rb);
            float mx = 0; bool bounded = true;
            for (float v : prof) { mx = std::max(mx, v); if (v < 0 || v > 1) bounded = false; }
            check("ring profile bounded with a dense part", bounded && mx > 0.3, "max " + std::to_string(mx));
            double worst = 0;
            for (int la = -30; la <= 30 && worst < 0.2; la += 3)
                for (int k = 0; k < 16 && worst < 0.2; k++) {
                    double tt = 2000.0 + k * rsys.bodies[rb].orbitPeriod / 16.0;
                    SurfaceSite site; site.init(&rsys, rb, la * DEG, 0.5, tt);
                    worst = std::max(worst, site.sun(tt).ringShadow);
                }
            check("the ring shadows the ground somewhere", worst >= 0.2, "shadow " + std::to_string(worst));
        }
    }
    // O1 (B-302): the rangefinder: 45 degrees down from eye height on the ground hits the ground a few metres off
    if (bi >= 0) {
        SurfaceView sv; SpaceRenderer sr; StarNeighborhood nb;
        sv.init(&sys, bi, 0.4, 1.1, 0.0);
        nb.update(sv.site.worldPos(0, 0, 0, 0));
        sv.player.pitch = -45 * DEG;
        Input in; sv.update(1.0 / 60, in, 0.0, false);
        Framebuffer fb; sv.render(fb, 0.0, nb.stars, sr, 1.0);
        check("rangefinder 45 deg down hits within 1..6 m", sv.lastRange > 1.0 && sv.lastRange < 6.0, "range " + std::to_string(sv.lastRange));
        check("the far ring reaches 53 km", sv.drawRadiusM() >= 53000, std::to_string(sv.drawRadiusM()));
    }
    // O4 (R-303): comets are landable; on one the explorer is held under the escape velocity and the buggy refuses
    {
        Star cs; StarSystem csys; int cb = -1;
        for (int64_t x = 150; x < 320 && cb < 0; x++)
            for (int64_t z = 20; z < 140 && cb < 0; z++) {
                if (!starInSector(x, 0, z, cs)) continue;
                csys.generate(cs);
                for (int i = 0; i < (int)csys.bodies.size(); i++) if (csys.bodies[i].type == PT_COMET) { cb = i; break; }
            }
        check("comets are landable", PLANET_TYPES[PT_COMET].landable && cb >= 0);
        if (cb >= 0) {
            SurfaceView sv; sv.init(&csys, cb, 0.2, 0.5, 0.0);
            double ve = sv.site.escapeVelocity;
            check("comet escape velocity under 30 m/s", ve > 0.1 && ve < 30, "v " + std::to_string(ve));
            check("small body: terrain rings capped near the horizon", sv.site.smallBody && sv.drawRadiusM() <= 1.2 * sv.site.R, std::to_string(sv.drawRadiusM()));
            sv.relocateCapsule(sv.player.x + 3, sv.player.z + 2);
            check("no buggy on a comet", !sv.deployBuggy());
            Input in; in.down[KEY_W] = true; in.down[KEY_LEFT_SHIFT] = true;
            for (int i = 0; i < 150; i++) sv.update(1.0 / 30, in, i / 30.0, true);
            double spd = std::sqrt(sv.player.vx * sv.player.vx + sv.player.vz * sv.player.vz);
            check("sprinting on a comet stays under 0.3 escape velocity", spd > 0.05 && spd <= 0.3 * ve + 0.01, "speed " + std::to_string(spd));
        }
    }
    // O3 (R-302): belt rocks keep their identity as the belt shears
    {
        Star bs; StarSystem bsys; bool have = false;
        for (int64_t x = 150; x < 320 && !have; x++)
            for (int64_t z = 20; z < 140 && !have; z++) {
                if (!starInSector(x, 0, z, bs)) continue;
                bsys.generate(bs);
                if (!bsys.belts.empty()) have = true;
            }
        check("a belt found near the start", have);
        if (have) {
            const Belt& bl = bsys.belts[0];
            Vec3 anchor = bsys.star.pos + Vec3(0.5 * (bl.innerKm + bl.outerKm), 0, 0);
            int n = 0; BeltRock first; bool got = false;
            bsys.forBeltRocksNear(0, anchor, 1000.0, 2, 2, 1, [&](const BeltRock& rk) { n++; if (!got) { first = rk; got = true; } });
            check("rocks within two cells of the anchor", n >= 3, std::to_string(n) + " rocks");
            BeltRock later; bool same = got && bsys.beltRockAt(0, first.ir, first.ia, first.iy, first.m, 1000.0 + 7200, later);
            check("a rock is the same rock two hours later", same && std::fabs(later.radiusKm - first.radiusKm) < 1e-12 && later.seed == first.seed);
            double rr = std::sqrt((later.pos.x - bsys.star.pos.x) * (later.pos.x - bsys.star.pos.x) + (later.pos.z - bsys.star.pos.z) * (later.pos.z - bsys.star.pos.z));
            check("the rock stays inside the belt", same && rr >= bl.innerKm && rr <= bl.outerKm);
            // late in an expedition (t = 4e6 s) the rings have sheared by hundreds of cells: the rock beside the ship must still be found
            {
                double tl = 4.0e6; BeltRock rk0; bool ok0 = false;
                bsys.forBeltRocksNear(0, anchor, tl, 1, 1, 1, [&](const BeltRock& rk) { if (!ok0) { rk0 = rk; ok0 = true; } });
                int near = 0;
                if (ok0) bsys.forBeltRocksNear(0, rk0.pos, tl, 4, 4, 1, [&](const BeltRock& rk) { if (length(rk.pos - rk0.pos) < 3 * BELT_CELL_KM) near++; });
                check("rocks are found round a rock after the rings sheared", ok0 && near >= 5, std::to_string(near) + " within three cells");
            }
        }
    }
    printf("unit: %d failures\n", fails);
    return fails;
}

// O5 (the terrain plan): slope and relief statistics of the planet function per landable type: two bodies, three land
// sites each, a 48 x 48 grid at 16 m (what you walk on), a 32 x 32 grid at 64 m and one at 512 m (what you see)
// ---- O6-01: terrain metrics and the landform gallery -------------------------------------------------------------
// The acceptance harness of Terrain III (PLAN-terrain-3.md, section 2). A site is measured on four grids of the planet
// function: 24 x 24 cells at 4 m (the near ring), 48 x 48 at 16 m (770 m: what you walk on), 32 x 32 at 64 m (2 km) and
// 32 x 32 at 512 m (16 km: what you see from a ridge); a slope is the rise over run between neighbouring cells. Sites are
// grouped by the relief class the landing map prints (`Game::siteSlope`, `reliefClass`) and judged against that class's
// targets; the cost of a 16 m sample is timed serially and compared with the O5 figure of the type.
#include "core/parallel.h"

static const char* CLASS_NAMES[3] = {"FLAT", "HILLS", "MOUNTAINS"};
static int classIndex(const char* cls) { return cls[0] == 'F' ? 0 : (cls[0] == 'H' ? 1 : 2); }

struct SiteMetrics {
    int cls = 0;                                   // the landing map's class: 0 flat, 1 hills, 2 mountains
    double probe = 0;                              // its slope probe
    double med4 = 0;                               // median slope of the 4 m grid
    std::vector<double> slopes16, slopes512;       // every cell's slope (pooled per class by the table)
    double relief770 = 0, relief2k = 0, relief16k = 0;
    double floors = 0;                             // share of the lowest quarter of the 64 m cells under a slope of 0.05
    double usPerSample = 0;                        // the 16 m grid's cost, when timed
    double hiYaw = 0;                              // toward the highest ground within a kilometre (the gallery looks there)
    double lat = 0, lon = 0;
    int material = MAT_ROCK;
};

static void gridHeights(const SurfaceSite& site, int N, double cell, std::vector<double>& h) {
    h.resize((size_t)(N + 1) * (N + 1));
    for (int j = 0; j <= N; j++)
        for (int i = 0; i <= N; i++) h[(size_t)j * (N + 1) + i] = site.sampleAt((i - N / 2) * cell, (j - N / 2) * cell, cell).h;
}
static void gridSlopes(const std::vector<double>& h, int N, double cell, std::vector<double>& out) {
    out.resize((size_t)N * N);
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++) {
            double gx = h[(size_t)j * (N + 1) + i + 1] - h[(size_t)j * (N + 1) + i], gz = h[(size_t)(j + 1) * (N + 1) + i] - h[(size_t)j * (N + 1) + i];
            out[(size_t)j * N + i] = std::sqrt(gx * gx + gz * gz) / cell;
        }
}
static double percentile(std::vector<double> v, double p) { if (v.empty()) return 0; std::sort(v.begin(), v.end()); return v[std::min(v.size() - 1, (size_t)(p * v.size()))]; }
static double shareOver(const std::vector<double>& v, double thr) { if (v.empty()) return 0; int n = 0; for (double s : v) if (s > thr) n++; return (double)n / v.size(); }
static double reliefOf(const std::vector<double>& h) { double lo = 1e9, hi = -1e9; for (double v : h) { lo = std::min(lo, v); hi = std::max(hi, v); } return hi - lo; }

static SiteMetrics measureSite(const SurfaceSite& site, bool timeIt) {
    SiteMetrics m;
    m.lat = site.lat0; m.lon = site.lon0;
    m.material = site.sampleAt(0, 0, 16).material;
    m.probe = Game::siteSlope(site);
    m.cls = classIndex(reliefClass(m.probe));
    std::vector<double> h, sl;
    gridHeights(site, 24, 4, h); gridSlopes(h, 24, 4, sl); m.med4 = percentile(sl, 0.5);
    double t0 = nowSec();
    gridHeights(site, 48, 16, h);
    if (timeIt) m.usPerSample = (nowSec() - t0) * 1e6 / (49.0 * 49.0);
    gridSlopes(h, 48, 16, m.slopes16); m.relief770 = reliefOf(h);
    gridHeights(site, 32, 64, h); m.relief2k = reliefOf(h);
    {
        gridSlopes(h, 32, 64, sl);
        // the lowest quarter of the cells and how many of them lie under a 5% slope: valley floors a buggy can follow
        std::vector<std::pair<double, double>> cells;   // height, slope
        int hi = 0; double best = -1e9;
        for (int j = 0; j < 32; j++)
            for (int i = 0; i < 32; i++) { double hv = h[(size_t)j * 33 + i]; cells.push_back({hv, sl[(size_t)j * 32 + i]}); if (hv > best) { best = hv; hi = j * 32 + i; } }
        m.hiYaw = std::atan2((double)(hi % 32 - 16), (double)(hi / 32 - 16));
        std::sort(cells.begin(), cells.end());
        int q = (int)cells.size() / 4, flat = 0;
        for (int k = 0; k < q; k++) if (cells[k].second < 0.05) flat++;
        m.floors = q ? (double)flat / q : 0;
    }
    gridHeights(site, 32, 512, h); gridSlopes(h, 32, 512, m.slopes512); m.relief16k = reliefOf(h);
    return m;
}

// the first `maxBodies` bodies of a type in the start region, in the scan order every harness mode uses; `fn(sys, bi)`
template <class F>
static int forTypeBodies(int type, int maxBodies, bool preferMoon, F fn) {
    int found = 0;
    for (int pass = preferMoon ? 0 : 1; pass < 2 && found < maxBodies; pass++)
        for (int64_t x = 150; x < 340 && found < maxBodies; x++)
            for (int64_t z = 20; z < 160 && found < maxBodies; z++) {
                Star s; if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && found < maxBodies; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (b.type != type) continue;
                    if (pass == 0 && b.parent < 0) continue;   // the first pass looks for moons only
                    found++;
                    fn(sys, bi);
                }
            }
    return found;
}

static bool firstBodyOfType(int type, StarSystem& sysOut, int& biOut) {
    bool found = false;
    forTypeBodies(type, 1, false, [&](const StarSystem& sys, int bi) { sysOut = sys; biOut = bi; found = true; });
    return found;
}

// B-307: the buggy rolls down a mountain slope. The steepest straight descent of 200 m within 500 m of the capsule of
// the pinned felisian mountain site (grade 0.12 .. 0.45, no water), driven at full throttle: it must stay on its wheels
// (under a tenth of the frames airborne, at most two landings) and get to the bottom fast. It used to launch every time
// the ground dropped faster than 3 m/s, land with a thump and 15% of its speed gone, and launch again: a ladder.
static int runDescent() {
    std::vector<StarSystem> systems; std::vector<int> bodyOf;
    forTypeBodies(PT_FELISIAN, 1, false, [&](const StarSystem& sys, int bi) { systems.push_back(sys); bodyOf.push_back(bi); });
    if (systems.empty()) { printf("descent: no felisian body\n"); return 1; }
    const StarSystem& sys = systems[0]; int bi = bodyOf[0];
    double lat = PIN_MOUNTAIN_LAT * DEG, lon = PIN_MOUNTAIN_LON * DEG;   // the landforms review site "felisian_mountains"
    SurfaceView sv; sv.init(&sys, bi, lat, lon, 1000.0);
    double t = findTime(sv.site, 35 * DEG, 1000.0);
    sv.init(&sys, bi, lat, lon, t);
    StarNeighborhood nb; nb.update(sv.site.worldPos(t, 0, 0, 0));
    // the run: start points on a 50 m grid within 500 m, 24 headings, every 10 m step must descend
    double bx = 0, bz = 0, bh = 0, bestGrade = 0;
    const double LEN = 200;
    for (double sx = -500; sx <= 500; sx += 50)
        for (double sz = -500; sz <= 500; sz += 50)
            for (int k = 0; k < 24; k++) {
                double h = k * TAU / 24, hx = std::sin(h), hz = std::cos(h);
                double h0 = sv.site.groundHeight(sx, sz), h1 = sv.site.groundHeight(sx + hx * LEN, sz + hz * LEN);
                double grade = (h0 - h1) / LEN;
                if (grade < 0.12 || grade > 0.45 || grade <= bestGrade) continue;
                bool ok = true; double hp = h0;
                for (double d = 10; d <= LEN + 30 && ok; d += 10) {
                    double x = sx + hx * d, z = sz + hz * d, hh = sv.site.groundHeight(x, z);
                    if (hh > hp + 0.3 || hh < sv.site.waterAt(x, z) + 0.5) ok = false;
                    hp = hh;
                }
                if (ok) { bx = sx; bz = sz; bh = h; bestGrade = grade; }
            }
    if (bestGrade <= 0) { printf("descent: no slope found\n"); return 1; }
    sv.relocateCapsule(bx - std::sin(bh) * 6, bz - std::cos(bh) * 6);
    sv.player.x = bx; sv.player.z = bz; sv.player.y = sv.site.surfaceHeight(bx, bz);   // the buggy deploys beside the capsule, near the explorer
    if (!sv.deployBuggy()) { printf("descent: could not deploy\n"); return 1; }
    sv.buggy.x = bx; sv.buggy.z = bz; sv.buggy.y = sv.site.surfaceHeight(bx, bz); sv.buggy.heading = bh; sv.buggy.speed = 0; sv.buggy.unfold = 1;
    sv.player.x = bx + 1.5; sv.player.z = bz; sv.player.y = sv.buggy.y;
    if (!sv.inBuggy && !sv.toggleBuggy()) { printf("descent: could not get in\n"); return 1; }
    Input in;
    int frames = 0, air = 0, landings = 0; bool wasAir = false; double along = 0, sumSpeed = 0, arriveT = -1, thumps = 0;
    double hx = std::sin(bh), hz = std::cos(bh);
    for (int i = 0; i < 60 * 25 && along < LEN; i++) {
        in.newFrame(); in.down[KEY_W] = true;
        double err = wrapAngle(bh - sv.buggy.heading);
        if (std::fabs(err) > 3 * DEG) in.down[err > 0 ? KEY_D : KEY_A] = true;
        sv.update(1.0 / 60, in, t + i / 60.0, true);
        frames++;
        if (sv.buggy.airborne) air++;
        if (wasAir && !sv.buggy.airborne) landings++;
        wasAir = sv.buggy.airborne;
        thumps = std::max(thumps, sv.buggy.thud);
        along = (sv.buggy.x - bx) * hx + (sv.buggy.z - bz) * hz;
        sumSpeed += std::fabs(sv.buggy.speed);
        if (along >= LEN && arriveT < 0) arriveT = i / 60.0;
    }
    double airFrac = frames ? (double)air / frames : 1.0, meanSpeed = frames ? sumSpeed / frames : 0;
    bool ok = arriveT > 0 && airFrac < 0.1 && landings <= 2 && meanSpeed > 8;
    printf("descent: grade %.2f over %.0f m from (%.0f, %.0f); %.1f s to the bottom, mean %.1f m/s, airborne %.0f%% of the frames, %d landings, worst thump %.2f\n",
           bestGrade, LEN, bx, bz, arriveT, meanSpeed, airFrac * 100, landings, thumps);
    printf("descent: %s\n", ok ? "ok (on its wheels, under 10%% airborne, at most two landings)" : "FAIL");
    return ok ? 0 : 1;
}
// hashed land sites of a body between 60 S and 60 N (water when a world has no land within 50 draws)
static void landSitesOf(const StarSystem& sys, int bi, uint64_t salt, int n, std::vector<std::pair<double, double>>& out) {
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    Rng r(b.seed ^ salt);
    for (int k = 0; k < n; k++) {
        double lat = 0, lon = 0;
        for (int tries = 0; tries < 50; tries++) {
            lat = r.range(-60 * DEG, 60 * DEG); lon = r.range(-PI, PI);
            if (sampleSurface(g, StarSystem::bodyFromLatLon(lat, lon), 64).material != MAT_WATER) break;
        }
        out.push_back({lat, lon});
    }
}

// The targets of section 2 of the plan per class; `later` names the item that owns a target that is not part of the
// spectrum work (cliffs and broken plains come with the landform library), so it is reported but not counted yet.
struct ClassAgg { int n = 0; std::vector<double> s16, s512; double med4 = 0, r770 = 0, r2k = 0, r16k = 0, floors = 0; };
static int judgeClass(int cls, const ClassAgg& a, bool smallBody, bool craterFields, std::string& notes, int& later) {
    int fails = 0;
    auto range = [&](const char* name, double v, double lo, double hi, const char* owner, const char* fmt) {
        bool ok = v >= lo && v <= hi;
        char buf[96]; snprintf(buf, sizeof buf, fmt, v);
        notes += std::string(name) + " " + buf + (ok ? " ok" : (owner ? std::string(" FAIL(") + owner + ")" : " FAIL")) + "; ";
        if (!ok) { if (owner) later++; else fails++; }
    };
    double med16 = percentile(a.s16, 0.5), p90 = percentile(a.s16, 0.9);
    double cliff = shareOver(a.s16, 1.0), walk = 1 - shareOver(a.s16, 1.35);
    if (smallBody) {   // a comet is smaller than the windows and craggy by nature (O4): only the walking rule applies
        range("walkable", walk * 100, 85, 100, nullptr, "%.0f%%");
    } else if (cls == 2) {
        range("median", med16, 0.25, 0.45, nullptr, "%.2f");
        range("2km", a.r2k, 500, 1200, nullptr, "%.0fm");
        range("16km", a.r16k, 1500, craterFields ? 5000 : 3500, nullptr, "%.0fm");   // a basin's wall in the window adds kilometres
        range("cliffs", cliff * 100, 3, 8, nullptr, "%.1f%%");   // O6-04
        range("walkable", walk * 100, 90, 100, nullptr, "%.0f%%");
        range("floors", a.floors * 100, 15, 100, nullptr, "%.0f%%");   // O6-03: the valley floors
    } else if (cls == 1) {
        range("median", med16, 0.08, 0.15, nullptr, "%.2f");
        range("2km", a.r2k, 120, craterFields ? 450 : 250, nullptr, "%.0fm");   // a crater in the window adds its depth
        range("floors", a.floors * 100, 25, 100, craterFields ? "O6-05" : nullptr, "%.0f%%");   // crater floors and terraces come with the second landform library
    } else {
        range("median", med16, 0.01, craterFields ? 0.09 : 0.04, nullptr, "%.3f");   // a plain of a cratered type is a crater field
        range("broken p90", p90, 0.06, 10, nullptr, "%.3f");   // O6-04
        range("broken 770m", a.r770, 6, 1e9, nullptr, "%.0fm");
    }
    return fails;
}

static int g_terrainType = -1;   // `terrain <type id>`: one type only (the tuning loop)
static int runTerrainStats() {
    // O5 (before O6-02): microseconds per 16 m sample on the development Mac, measured by this harness; the budget is 2x
    static const double O5_COST[PT_COUNT] = {0.8, 1.1, 0.5, 0.8, 0.9, 1.3, 0, 0.9, 0.4, 0.2, 0.8, 0.4, 0.5, 0, 1.0, 0};
    printf("%-16s %-9s %3s | %5s | %-40s | %5s %6s %6s | %6s | %-13s | %s\n", "type", "class", "n", "4m", "16 m: med  p90   max  cliff  walk", "770m", "2km", "16km", "floors", "512m: med p90", "cost us (O5, x)");
    int fails = 0, later = 0, rows = 0, rowsOk = 0;
    for (int type = 0; type < PT_COUNT; type++) {
        if (!PLANET_TYPES[type].landable || (g_terrainType >= 0 && type != g_terrainType)) continue;
        std::vector<StarSystem> systems; std::vector<int> bodyOf; std::vector<std::pair<double, double>> sites; std::vector<int> sysOf;
        forTypeBodies(type, 4, false, [&](const StarSystem& sys, int bi) {
            systems.push_back(sys); bodyOf.push_back(bi);
            size_t n0 = sites.size();
            landSitesOf(sys, bi, 0x06, 12, sites);
            for (size_t k = n0; k < sites.size(); k++) sysOf.push_back((int)systems.size() - 1);
        });
        if (sites.empty()) { printf("%-16s no body found\n", PLANET_TYPES[type].name); continue; }
        std::vector<SiteMetrics> ms(sites.size());
        parallelFor((int)sites.size(), 1, [&](int b, int e) {
            for (int i = b; i < e; i++) {
                SurfaceSite st; st.init(&systems[sysOf[i]], bodyOf[sysOf[i]], sites[i].first, sites[i].second, 1000.0); ms[i] = measureSite(st, false);
                if (std::getenv("VESPERIS_TRACE") && ms[i].cls == 0 && typeHasDrainage(type) && i < 6) {   // O6-07: what flattens a plain (the flat class's median grade)
                    std::vector<double> sOther, sValley, sWater, sLake; int n = 0;
                    for (int z = -24; z < 24; z++) for (int x = -24; x < 24; x++) {
                        double h00 = st.sampleAt(x * 16.0, z * 16.0, 16).h, h10 = st.sampleAt(x * 16.0 + 16, z * 16.0, 16).h, h01 = st.sampleAt(x * 16.0, z * 16.0 + 16, 16).h;
                        double sl = std::sqrt((h10 - h00) * (h10 - h00) + (h01 - h00) * (h01 - h00)) / 16.0;
                        TerrainVertex tv = st.sampleAt(x * 16.0, z * 16.0, 16);
                        DrainInfo D = drainageAt(st.gen, st.unitAt(x * 16.0, z * 16.0), 16.0, 0.5);
                        n++;
                        if (tv.water > -1e8f && tv.h < tv.water) sWater.push_back(sl); else if (D.lake > 0.3) sLake.push_back(sl); else if (D.valley > 0.3) sValley.push_back(sl); else sOther.push_back(sl);
                    }
                    auto med = [](std::vector<double> v) { if (v.empty()) return 0.0; std::sort(v.begin(), v.end()); return v[v.size() / 2]; };
                    fprintf(stderr, "  flat site %.3f %.3f: water %zu (med %.4f) lake %zu (%.4f) valley %zu (%.4f) other %zu (%.4f) of %d; site median %.4f\n", sites[i].first / DEG, sites[i].second / DEG,
                            sWater.size(), med(sWater), sLake.size(), med(sLake), sValley.size(), med(sValley), sOther.size(), med(sOther), n, percentile(ms[i].slopes16, 0.5));
                }
                if (std::getenv("VESPERIS_TRACE") && ms[i].cls == 2 && typeHasDrainage(type)) {   // O6-07: what the drainage says over a mountain window (the floors' tuning)
                    int nearN = 0, valleyN = 0, floorN = 0, lakeN = 0; double accMax = 0;
                    for (int z = -16; z < 16; z++) for (int x = -16; x < 16; x++) {
                        DrainInfo D = drainageAt(st.gen, st.unitAt(x * 64.0 + 32, z * 64.0 + 32), 64.0, 0.5);
                        if (D.near) nearN++; if (D.valley > 0.3) valleyN++; if (D.valley > 0.9) floorN++; if (D.lake > 0.5) lakeN++; accMax = std::max(accMax, D.acc);
                    }
                    fprintf(stderr, "  mtn site %.3f %.3f: near %d valley %d floor %d lake %d of 1024, max acc %.1f km2, floors %.0f%%\n", sites[i].first / DEG, sites[i].second / DEG, nearN, valleyN, floorN, lakeN, accMax, ms[i].floors * 100);
                    for (int z = -16; z < 16; z += 8) {   // rows of cells: the nearest channel's distance and accumulation, the valley
                        fprintf(stderr, "    row z=%d:", z);
                        for (int x = -16; x < 16; x += 2) { DrainInfo D = drainageAt(st.gen, st.unitAt(x * 64.0 + 32, z * 64.0 + 32), 64.0, 0.5); fprintf(stderr, " %s%.0f/%.1f/%.2f", D.near ? "" : "-", D.dist, D.acc, D.valley); }
                        fprintf(stderr, "\n");
                    }
                }
            }
        });
        double cost = 1e9;
        {   // the cost, serially, at the first site: the best of three passes over the 16 m grid
            SurfaceSite st; st.init(&systems[sysOf[0]], bodyOf[sysOf[0]], sites[0].first, sites[0].second, 1000.0);
            std::vector<double> h;
            for (int pass = 0; pass < 3; pass++) { double t0 = nowSec(); gridHeights(st, 48, 16, h); cost = std::min(cost, (nowSec() - t0) * 1e6 / (49.0 * 49.0)); }
        }
        ClassAgg agg[3];
        for (const SiteMetrics& m : ms) {
            ClassAgg& a = agg[m.cls];
            a.n++; a.s16.insert(a.s16.end(), m.slopes16.begin(), m.slopes16.end()); a.s512.insert(a.s512.end(), m.slopes512.begin(), m.slopes512.end());
            a.med4 += m.med4; a.r770 += m.relief770; a.r2k += m.relief2k; a.r16k += m.relief16k; a.floors += m.floors;
        }
        bool costOk = O5_COST[type] <= 0 || cost <= 2.0 * O5_COST[type];
        if (!costOk) fails++;
        for (int cls = 2; cls >= 0; cls--) {
            ClassAgg& a = agg[cls];
            if (!a.n) continue;
            a.med4 /= a.n; a.r770 /= a.n; a.r2k /= a.n; a.r16k /= a.n; a.floors /= a.n;
            std::string notes; int f = 0, l = 0;
            bool judged = type != PT_OCEAN && a.n >= 2;
            bool craterFields = type == PT_CRATERED || type == PT_THINATMO || type == PT_METAL || type == PT_ROCKY || type == PT_ICY || type == PT_CARBON || type == PT_BOMBARDED;
            if (judged) { f = judgeClass(cls, a, systems[0].bodies[bodyOf[0]].radiusKm < 300, craterFields, notes, l); fails += f; later += l; rows++; if (!f) rowsOk++; }
            printf("%-16s %-9s %3d | %.3f | %.3f %.3f %5.2f %4.1f%% %4.0f%% | %4.0fm %5.0fm %5.0fm | %4.0f%% | %.4f %.4f | %s\n", PLANET_TYPES[type].name, CLASS_NAMES[cls], a.n, a.med4,
                   percentile(a.s16, 0.5), percentile(a.s16, 0.9), a.s16.empty() ? 0.0 : *std::max_element(a.s16.begin(), a.s16.end()), shareOver(a.s16, 1.0) * 100, (1 - shareOver(a.s16, 1.35)) * 100,
                   a.r770, a.r2k, a.r16k, a.floors * 100, percentile(a.s512, 0.5), percentile(a.s512, 0.9), judged ? notes.c_str() : (a.n < 2 ? "(one site: not judged)" : "(not judged)"));
        }
        printf("%-16s classes of %zu land sites: flat %d, hills %d, mountains %d; cost %.1f us per 16 m sample (O5 %.1f, x%.2f%s)\n", "", ms.size(), agg[0].n, agg[1].n, agg[2].n, cost, O5_COST[type],
               O5_COST[type] > 0 ? cost / O5_COST[type] : 0.0, costOk ? "" : ", OVER 2x");
    }
    printf("terrain: %d of %d class rows meet every target of the spectrum; %d spectrum targets failing, %d targets of later items (cliffs and broken plains: O6-04; mountain floors: O6-03) not yet met\n", rowsOk, rows, fails, later);
    return fails;
}

// ---- the landform gallery ------------------------------------------------------------------------------------------
struct Sheet {
    int w, h; std::vector<uint32_t> px; RGBCanvas cv;
    Sheet(int W, int H) : w(W), h(H), px((size_t)W * H, rgb(12, 12, 14)), cv{px.data(), W, H, 1} {}
    void blit(const std::vector<uint32_t>& src, int sw, int sh, int ox, int oy) {
        for (int y = 0; y < sh; y++) for (int x = 0; x < sw; x++) { int X = ox + x, Y = oy + y; if (X >= 0 && Y >= 0 && X < w && Y < h) px[(size_t)Y * w + X] = src[(size_t)y * sw + x]; }
    }
    void label(int x, int y, const std::string& s, uint32_t c = rgb(255, 220, 120)) { fillRectRGB(cv, x, y, x + textWidth(s.c_str()) + 3, y + 8, rgb(0, 0, 0)); drawText(cv, x + 2, y + 1, s.c_str(), c); }
    void save(const std::string& fn) { writePNG(fn.c_str(), px.data(), w, h); printf("wrote %s\n", fn.c_str()); }
};

// A shaded relief of an N x N grid of `cell` metres round the site, 2 px per cell: the light from the north-west at 45
// degrees, water blue, snow and ice pale; what a DEM hillshade shows a geologist.
static void hillshade(const SurfaceSite& site, int N, double cell, double zFactor, std::vector<uint32_t>& px) {
    std::vector<double> h((size_t)(N + 1) * (N + 1)), w(h.size()); std::vector<uint8_t> mat(h.size());
    for (int j = 0; j <= N; j++)
        for (int i = 0; i <= N; i++) { TerrainVertex v = site.sampleAt((i - N / 2) * cell, (j - N / 2) * cell, cell); size_t k = (size_t)j * (N + 1) + i; h[k] = v.h; w[k] = v.water; mat[k] = v.material; }
    px.assign((size_t)N * N * 4, 0);
    Vec3 L = normalize(Vec3(-1, 1.414, 1));
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++) {
            size_t k = (size_t)j * (N + 1) + i;
            double gx = (h[k + 1] - h[k]) / cell * zFactor, gz = (h[k + N + 1] - h[k]) / cell * zFactor;
            Vec3 n = normalize(Vec3(-gx, 1, -gz));
            double l = 0.18 + 0.82 * std::max(0.0, dot(n, L));
            uint32_t c;
            if (mat[k] == MAT_WATER || (w[k] > -1e8 && w[k] > h[k])) c = rgb((int)(30 + 40 * l), (int)(50 + 70 * l), (int)(110 + 100 * l));
            else if (mat[k] == MAT_SNOW || mat[k] == MAT_ICE) c = rgb((int)(90 + 165 * l), (int)(95 + 160 * l), (int)(110 + 145 * l));
            else c = rgb((int)(20 + 215 * l), (int)(20 + 205 * l), (int)(20 + 195 * l));
            int oy = (N - 1 - j) * 2, ox = i * 2;   // north up
            for (int dy = 0; dy < 2; dy++) for (int dx = 0; dx < 2; dx++) px[(size_t)(oy + dy) * N * 2 + ox + dx] = c;
        }
}

// one frame of a site: standing at eye height (alt < 0), or `alt` metres above the ground looking down `pitch`; the sun at 35 deg
static double renderSiteFrame(const StarSystem& sys, int bi, double lat, double lon, double yaw, double alt, double pitch, SpaceRenderer& sr, StarNeighborhood& nb, std::vector<uint32_t>& px) {
    SurfaceView sv;
    sv.init(&sys, bi, lat, lon, 1000.0);
    double t = findTime(sv.site, 35 * DEG, 1000.0);
    sv.init(&sys, bi, lat, lon, t);
    sv.player.yaw = yaw; sv.player.pitch = pitch;
    sv.player.x = std::sin(yaw) * 14; sv.player.z = std::cos(yaw) * 14;   // 14 m ahead of the capsule, which stays behind the camera
    sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
    if (alt >= 0) { sv.cameraOverrideAlt = alt; sv.cameraOverridePitch = pitch; }
    nb.update(sv.site.worldPos(t, 0, 0, 0));
    Input in; sv.update(0.016, in, t, false);
    Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
    px.resize((size_t)FBW * FBH); fb.toRGB(px.data(), 1.0);
    return t;
}
static std::string metricsShort(const SiteMetrics& m) {
    char buf[128];
    snprintf(buf, sizeof buf, "%s %.2f MED %.2f P90 %.2f 2KM %.0fM 16KM %.0fM", CLASS_NAMES[m.cls], m.probe, percentile(m.slopes16, 0.5), percentile(m.slopes16, 0.9), m.relief2k, m.relief16k);
    return buf;
}
static std::string metricsLine(const SiteMetrics& m) {
    char buf[256];
    snprintf(buf, sizeof buf, "%s %.2f  MED %.2f P90 %.2f CLIFF %.0f%%  770M %.0fM 2KM %.0fM 16KM %.0fM FLOORS %.0f%%", CLASS_NAMES[m.cls], m.probe, percentile(m.slopes16, 0.5), percentile(m.slopes16, 0.9),
             shareOver(m.slopes16, 1.0) * 100, m.relief770, m.relief2k, m.relief16k, m.floors * 100);
    return buf;
}

// B-313: the scene `felisian_mountains`: the pinned felisian mountain site of the O6 review (LANDFORM_SITES[0]), standing,
// facing the highest ground within a kilometre, for the walking stability test and the frames
static bool setupPinnedMountainScene(double alt, double yawOff, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut) {
    std::vector<StarSystem> systems; std::vector<int> bodyOf;
    forTypeBodies(PT_FELISIAN, 1, false, [&](const StarSystem& sy, int bi) { systems.push_back(sy); bodyOf.push_back(bi); });
    if (systems.empty()) return false;
    sys = systems[0]; int bi = bodyOf[0];
    double lat = PIN_MOUNTAIN_LAT * DEG, lon = PIN_MOUNTAIN_LON * DEG;
    sv.init(&sys, bi, lat, lon, 1000.0);
    double t = findTime(sv.site, alt, 1000.0);
    sv.init(&sys, bi, lat, lon, t);
    SiteMetrics m = measureSite(sv.site, false);
    sv.player.yaw = m.hiYaw + yawOff; sv.player.pitch = pitch;
    sv.player.x = 30; sv.player.z = -20; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);   // away from the capsule at the origin
    nb.update(sv.site.worldPos(t, 0, 0, 0));
    Input in; sv.update(0.016, in, t, false);
    tOut = t;
    return true;
}

// R-304/R-305: `traits [name]`: for every landform trait the first body that carries it, the spot on it where the landform
// is strongest (a scan of the sphere at 200 m detail by `SurfaceSample::landform`), a hillshade of 8 km round the spot, a
// standing frame from 250 m west of it and an aerial frame 350 m up; one sheet per trait in `shots/tests/trait_*.png`.
// Then the share of the first 300 bodies carrying each trait
static void renderTraits(const char* want) {
    SpaceRenderer sr; StarNeighborhood nb;
    setFramebufferScale(1);
    for (int tr = 1; tr <= TR_GEYSERS; tr++) {
        std::string nm = TRAIT_NAMES[tr];
        for (char& ch : nm) ch = ch == ' ' ? '_' : (char)tolower(ch);
        if (want && nm != want) continue;
        bool done = false;
        for (int64_t x = 150; x < 340 && !done; x++)
            for (int64_t z = 20; z < 160 && !done; z++) {
                Star s; if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && !done; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (!PLANET_TYPES[b.type].landable) continue;
                    BodyGen g = BodyGen::make(b);
                    if (!g.hasTrait(tr)) continue;
                    // the strongest spot on a 2 x 1 degree grid, then the best of a finer grid round it; the scan sees this
                    // trait alone (a copy of the body with the others removed), so a canyon's walls do not hide the spires
                    BodyGen gOnly = g; gOnly.traits[0] = tr; gOnly.traits[1] = 0; gOnly.traits[2] = 0;
                    setDrainageEnabled(false);   // O6-03: the scans sample the whole globe at 4-64 m; the frames below turn it back on
                    double bestS = 0, bestLat = 0, bestLon = 0;
                    bool fine = tr == TR_YARDANGS || tr == TR_POLYGONS || tr == TR_SPIRES || tr == TR_SALT_FLATS;   // the ones the 16 m ring (or the 4 m ring) alone carries
                    double scanD = (tr == TR_SPIRES && b.type != PT_QUARTZ && b.type != PT_CARBON) || tr == TR_POLYGONS ? 4 : (fine ? 16 : 64);
                    for (double la = -66; la <= 66; la += 2)
                        for (int k = 0; k < 360; k++) {
                            double ln = k * TAU / 360;
                            SurfaceSample ss = sampleSurface(gOnly, StarSystem::bodyFromLatLon(la * DEG, ln), scanD);
                            if (ss.material == MAT_WATER) continue;
                            if (ss.landform > bestS) { bestS = ss.landform; bestLat = la * DEG; bestLon = ln; }
                        }
                    if (bestS < 4) { printf("%-16s %s: nothing over 4 m (best %.1f)\n", TRAIT_NAMES[tr], b.name.c_str(), bestS); done = true; break; }
                    double fineS = 0, fLat = bestLat, fLon = bestLon;
                    for (int j = -12; j <= 12; j++)
                        for (int i = -12; i <= 12; i++) {
                            double la = bestLat + j * 800.0 / (b.radiusKm * 1000.0), ln = bestLon + i * 800.0 / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(bestLat)));
                            SurfaceSample ss = sampleSurface(gOnly, StarSystem::bodyFromLatLon(la, ln), scanD);
                            if (ss.material == MAT_WATER) continue;
                            if (ss.landform > fineS) { fineS = ss.landform; fLat = la; fLon = ln; }
                        }
                    // the standing camera: west of the spot, on dry ground outside the landform's mark (a mesa's or an inselberg's
                    // body would swallow it), 100 m past the first such point within 3 km
                    double standLon = fLon - 250.0 / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat)));
                    for (double dd = 60; dd <= 3000; dd += 60) {
                        double ln = fLon - dd / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat)));
                        SurfaceSample ss = sampleSurface(gOnly, StarSystem::bodyFromLatLon(fLat, ln), 16);
                        if (ss.material == MAT_WATER || (ss.water > -1e8 && ss.height < ss.water + 0.3) || ss.landform >= 1000) continue;
                        standLon = ln - 100.0 / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat)));
                        break;
                    }
                    // the sheet: the hillshade, a standing frame looking east at the spot, an aerial frame
                    setDrainageEnabled(true);   // O6-03: the frames show the real ground
                    const int N = 200; std::vector<uint32_t> hs;
                    SurfaceSite site; site.init(&sys, bi, fLat, fLon, 1000.0);
                    bool big = tr == TR_RIFT || tr == TR_ESCARPMENTS || tr == TR_GREAT_BASIN || tr == TR_CORONAE, mid = tr == TR_CANYONS || tr == TR_CINDER_FIELD || tr == TR_CHAOS || tr == TR_INSELBERGS || tr == TR_ERG || tr == TR_MESAS;
                    double cellM = big ? 800.0 : (mid ? 120.0 : 40.0);
                    hillshade(site, N, cellM, big ? 0.35 : 1.0, hs);
                    std::vector<uint32_t> f1, f2;
                    double la2 = fLat, lo2 = standLon;
                    renderSiteFrame(sys, bi, la2, lo2, PI / 2, -1, 0.04, sr, nb, f1);
                    renderSiteFrame(sys, bi, la2, lo2, PI / 2, 350, -0.6, sr, nb, f2);
                    Sheet sheet(N * 2 + FBW * 2, std::max(N * 2, FBH));
                    sheet.blit(hs, N * 2, N * 2, 0, 0);
                    sheet.blit(f1, FBW, FBH, N * 2, 0);
                    sheet.blit(f2, FBW, FBH, N * 2 + FBW, 0);
                    sheet.label(4, 4, fmt("%s  %s (%s)  %.1f%s %.1f%s  strength %.0f", TRAIT_NAMES[tr], b.name.c_str(), PLANET_TYPES[b.type].name, std::fabs(fLat / DEG), fLat >= 0 ? "N" : "S", std::fabs(fLon / DEG), fLon >= 0 ? "E" : "W", fineS));
                    sheet.label(4, N * 2 - 12, fmt("%.0f KM, %.0f M CELLS, NORTH UP", N * cellM / 1000, cellM));
                    sheet.label(N * 2 + 4, 4, fmt("STANDING %.0f M WEST OF THE SPOT", (fLon - standLon) * b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat))));
                    sheet.label(N * 2 + FBW + 4, 4, "350 M UP, LOOKING EAST AND DOWN");
                    sheet.save("shots/tests/trait_" + nm + ".png");
                    if (tr == TR_GEYSERS) {   // R-307: the vents the surface will put its geysers on
            std::vector<GeyserVent> vents; geyserVents(g, StarSystem::bodyFromLatLon(bestLat, bestLon), vents);
            int onSinter = 0; double nearest = 1e9;
            for (const GeyserVent& v : vents) {
                double la, lo; StarSystem::latLonFromBody(v.unit, la, lo);
                double d = std::acos(clampd(dot(v.unit, StarSystem::bodyFromLatLon(bestLat, bestLon)), -1, 1)) * b.radiusKm * 1000.0;
                nearest = std::min(nearest, d);
                if (sampleSurface(g, v.unit, 16).material == MAT_SALT) onSinter++;
            }
            printf("  geyser vents near the spot: %d, %d on sinter, the nearest %.0f m\n", (int)vents.size(), onSinter, nearest);
        }
        printf("%-16s %s (%s): traits [%s], spot %.2f %.2f, strength %.0f m\n", TRAIT_NAMES[tr], b.name.c_str(), PLANET_TYPES[b.type].name, traitList(g).c_str(), fLat / DEG, fLon / DEG, fineS);
                    done = true;
                }
            }
        if (!done) printf("%-16s: no body carries it in the scanned sectors\n", TRAIT_NAMES[tr]);
    }
    if (!want) {   // the distribution over the first 300 landable bodies
        int cnt[TR_COUNT] = {0}, n = 0, withN[4] = {0, 0, 0, 0};
        for (int64_t x = 150; x < 340 && n < 300; x++)
            for (int64_t z = 20; z < 160 && n < 300; z++) {
                Star s; if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && n < 300; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (!PLANET_TYPES[b.type].landable) continue;
                    BodyGen g = BodyGen::make(b);
                    int k = 0; for (int j = 0; j < 3; j++) if (g.traits[j]) { cnt[g.traits[j]]++; k++; }
                    withN[k]++; n++;
                }
            }
        printf("traits over %d landable bodies: none %d, one %d, two %d, three %d\n", n, withN[0], withN[1], withN[2], withN[3]);
        for (int tr = 1; tr < TR_COUNT; tr++) printf("  %-16s %3d\n", TRAIT_NAMES[tr], cnt[tr]);
    }
}

// `landforms <type> [flat|hills|mountains]`: twelve hashed land sites of the type (of that class) on one sheet, each a
// standing frame toward the highest ground within a kilometre over hillshades of 770 m, 3 km and 24 km
static void renderLandforms(int type, int wantCls) {
    SpaceRenderer sr; StarNeighborhood nb;
    setFramebufferScale(1);
    const int TW = 320, TH = 200 + 96 + 14;
    Sheet sheet(TW * 3, TH * 4);
    int n = 0;
    std::vector<StarSystem> systems; std::vector<int> bodyOf;
    forTypeBodies(type, 4, false, [&](const StarSystem& sys, int bi) { systems.push_back(sys); bodyOf.push_back(bi); });
    for (size_t si = 0; si < systems.size() && n < 12; si++) {
        std::vector<std::pair<double, double>> sites;
        landSitesOf(systems[si], bodyOf[si], 0x60 + wantCls, 60, sites);
        for (auto& s : sites) {
            if (n >= 12) break;
            SurfaceSite st; st.init(&systems[si], bodyOf[si], s.first, s.second, 1000.0);
            SiteMetrics m = measureSite(st, false);
            if (wantCls >= 0 && m.cls != wantCls) continue;
            if (m.material == MAT_WATER || m.material == MAT_SNOW || m.material == MAT_ICE) continue;
            std::vector<uint32_t> px;
            renderSiteFrame(systems[si], bodyOf[si], s.first, s.second, m.hiYaw, -1, 0.02, sr, nb, px);
            int ox = (n % 3) * TW, oy = (n / 3) * TH;
            sheet.blit(px, FBW, FBH, ox, oy);
            const double cells[3] = {16, 64, 512}, zf[3] = {1, 2, 3}; const char* names[3] = {"770 M", "3 KM Z2", "24 KM Z3"};
            for (int k = 0; k < 3; k++) { hillshade(st, 48, cells[k], zf[k], px); sheet.blit(px, 96, 96, ox + 8 + k * 104, oy + 200 + 2); sheet.label(ox + 8 + k * 104, oy + 200 + 2, names[k]); }
            std::string label = systems[si].bodies[bodyOf[si]].name + "  " + std::to_string((int)std::lround(s.first / DEG)) + "/" + std::to_string((int)std::lround(s.second / DEG)) + "  " + MATERIAL_NAMES[m.material];
            for (char& c : label) c = (char)toupper((unsigned char)c);
            sheet.label(ox, oy, label);
            sheet.label(ox, oy + 200 + 98, metricsShort(m), rgb(200, 230, 255));
            printf("%2d %-24s %s\n", n, label.c_str(), metricsLine(m).c_str());
            n++;
        }
    }
    std::string tn = PLANET_TYPES[type].name;
    for (char& ch : tn) if (ch == ' ') ch = '_';
    sheet.save("shots/tests/landforms_" + tn + (wantCls >= 0 ? std::string("_") + CLASS_NAMES[wantCls] : std::string()) + ".png");
    printf("landforms: %d sites of %s%s%s\n", n, PLANET_TYPES[type].name, wantCls >= 0 ? " of class " : "", wantCls >= 0 ? CLASS_NAMES[wantCls] : "");
}

// `landforms sites`: the fixed sites of the O6 review, each on its own sheet at three distances (standing, 150 m up
// looking down 12 deg, 2 km up looking down 30 deg) over the three hillshades and the metrics; the latitude and
// longitude are fixed so the sheets before and after a generator change show the same ground (0/0 searches a site of
// the wanted class on the first body of the type and prints what to pin)
struct LandformSite { const char* name; int type; bool moon; int wantCls; double latDeg, lonDeg; };
static const LandformSite LANDFORM_SITES[] = {
    {"felisian_mountains", PT_FELISIAN, false, 2, PIN_MOUNTAIN_LAT, PIN_MOUNTAIN_LON},   // pinned 2026-09-27 on the generation-5 function; O6-03: re-pinned on the GEN 10 bodies (the type table of GEN 9 had moved every world)
    {"felisian_plain", PT_FELISIAN, false, 0, 7.424, 75.762},
    {"thinatmo_hills", PT_THINATMO, false, 1, 44.433, 135.970},
    {"cratered_moon", PT_CRATERED, true, 1, 43.934, -41.488},
};
static void renderLandformSites() {
    SpaceRenderer sr; StarNeighborhood nb;
    setFramebufferScale(1);
    for (const LandformSite& L : LANDFORM_SITES) {
        std::vector<StarSystem> systems; std::vector<int> bodyOf;
        forTypeBodies(L.type, 1, L.moon, [&](const StarSystem& sys, int bi) { systems.push_back(sys); bodyOf.push_back(bi); });
        if (systems.empty()) { printf("%s: no body\n", L.name); continue; }
        const StarSystem& sys = systems[0]; int bi = bodyOf[0];
        double lat = L.latDeg * DEG, lon = L.lonDeg * DEG;
        if (L.latDeg == 0 && L.lonDeg == 0) {
            std::vector<std::pair<double, double>> sites;
            landSitesOf(sys, bi, 0x51, 400, sites);
            bool found = false;
            for (auto& s : sites) {
                SurfaceSite st; st.init(&sys, bi, s.first, s.second, 1000.0);
                SiteMetrics m = measureSite(st, false);
                if (m.cls != L.wantCls || m.material == MAT_WATER || m.material == MAT_SNOW || m.material == MAT_ICE) continue;
                lat = s.first; lon = s.second; found = true;
                printf("%s: pin lat %.3f lon %.3f (%s)\n", L.name, lat / DEG, lon / DEG, CLASS_NAMES[m.cls]);
                break;
            }
            if (!found) { printf("%s: no site of class %s found\n", L.name, CLASS_NAMES[L.wantCls]); continue; }
        }
        SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
        SiteMetrics m = measureSite(st, true);
        Sheet sheet(960, 200 + 96 + 30);
        const double alts[3] = {-1, 150, 2000}, pitches[3] = {0.02, -12 * DEG, -30 * DEG}; const char* dist[3] = {"STANDING", "150 M UP", "2 KM UP"};
        std::vector<uint32_t> px;
        for (int k = 0; k < 3; k++) { renderSiteFrame(sys, bi, lat, lon, m.hiYaw, alts[k], pitches[k], sr, nb, px); sheet.blit(px, FBW, FBH, k * 320, 0); sheet.label(k * 320, 0, dist[k]); }
        const double cells[3] = {16, 64, 512}, zf[3] = {1, 2, 3}; const char* names[3] = {"770 M", "3 KM Z2", "24 KM Z3"};
        for (int k = 0; k < 3; k++) { hillshade(st, 48, cells[k], zf[k], px); sheet.blit(px, 96, 96, 8 + k * 104, 202); sheet.label(8 + k * 104, 202, names[k]); }
        std::string title = std::string(L.name) + "  " + sys.bodies[bi].name + "  LAT " + std::to_string(lat / DEG).substr(0, 6) + " LON " + std::to_string(lon / DEG).substr(0, 7) + "  " + MATERIAL_NAMES[m.material];
        for (char& c : title) c = (char)toupper((unsigned char)c);
        char l2[160]; snprintf(l2, sizeof l2, "16 M: MEDIAN %.3f  P90 %.3f  MAX %.2f  CLIFF %.1f%%  WALKABLE %.0f%%  4 M MEDIAN %.3f", percentile(m.slopes16, 0.5), percentile(m.slopes16, 0.9), *std::max_element(m.slopes16.begin(), m.slopes16.end()),
                 shareOver(m.slopes16, 1.0) * 100, (1 - shareOver(m.slopes16, 1.35)) * 100, m.med4);
        char l3[160]; snprintf(l3, sizeof l3, "RELIEF 770 M %.0f M   2 KM %.0f M   16 KM %.0f M   FLOORS %.0f%%   512 M: MEDIAN %.4f P90 %.4f", m.relief770, m.relief2k, m.relief16k, m.floors * 100, percentile(m.slopes512, 0.5), percentile(m.slopes512, 0.9));
        char l4[160]; snprintf(l4, sizeof l4, "LANDING MAP CLASS %s (PROBE %.3f)   COST %.1f US PER 16 M SAMPLE   GEN %d", CLASS_NAMES[m.cls], m.probe, m.usPerSample, GEN_VERSION);
        const char* lines[4] = {title.c_str(), l2, l3, l4};
        for (int k = 0; k < 4; k++) drawText(sheet.cv, 330, 204 + k * 11, lines[k], k ? rgb(200, 230, 255) : rgb(255, 220, 120));
        sheet.save(std::string("shots/tests/landforms_site_") + L.name + ".png");
        printf("%-20s %s\n", L.name, metricsLine(m).c_str());
    }
}

// M6-02: the key map file and the gamepad mapping, checked without a window
static int testInput() {
    int fails = 0;
    {
        FILE* f = fopen("shots/tests/test_keys.txt", "w");
        fprintf(f, "# AZERTY: Z acts as W and W as Z, Q as A and A as Q\nZ W\nW Z\nQ A\nA Q\nF7 P\n");
        fclose(f);
        KeyMap km;
        bool ok = km.load("shots/tests/test_keys.txt");
        Input in;
        in.down[KEY_Z] = true; in.pressed[KEY_Z] = true; in.down[KEY_F7] = true; in.pressed[KEY_F7] = true;
        km.apply(in);
        bool good = ok && in.isDown(KEY_W) && in.wasPressed(KEY_W) && !in.isDown(KEY_Z) && in.wasPressed(KEY_P) && !in.isDown(KEY_F7);
        printf("keymap: %s (Z->W, F7->P)\n", good ? "PASS" : "FAIL");
        if (!good) fails++;
        // N5-05: the key bindings screen: open it from the settings, rebind FORWARD to Z, reset it, save
        {
            Game game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys2.txt"; game.guidePath = "shots/tests/test_guide.txt";
            KeyMap km2; game.keymap = &km2;
            Input gi;
            auto press = [&](int key) { gi.pressed[key] = true; gi.down[key] = true; game.frame(gi, 1.0 / 30); gi.newFrame(); gi.down[key] = false; };
            press(KEY_ENTER); for (int i = 0; i < 3; i++) game.frame(gi, 1.0 / 30);
            press(KEY_ESCAPE); press(KEY_DOWN); press(KEY_DOWN); press(KEY_DOWN); press(KEY_ENTER);   // menu > settings
            for (int i = 0; i < 18; i++) press(KEY_DOWN);   // KEY BINDINGS
            press(KEY_ENTER);
            bool opened = game.state == GameState::KEYS;
            press(KEY_ENTER);                                 // rebind FORWARD
            game.rawKey = KEY_Z; game.frame(gi, 1.0 / 30); game.rawKey = -1;
            bool bound = km2.map[KEY_Z] == KEY_W;
            writePNG("shots/tests/keys.png", game.output(), FBW, FBH);
            press(KEY_BACKSPACE);
            bool reset = km2.map[KEY_Z] == KEY_Z;
            press(KEY_ENTER); game.rawKey = KEY_Z; game.frame(gi, 1.0 / 30); game.rawKey = -1;
            press(KEY_ESCAPE);
            KeyMap km3; bool saved = km3.load("shots/tests/test_keys2.txt") && km3.map[KEY_Z] == KEY_W;
            bool ok2 = opened && bound && reset && saved && game.state == GameState::SETTINGS;
            printf("key bindings screen: %s (opened %d, Z->W %d, reset %d, saved and reloaded %d) -> shots/keys.png\n", ok2 ? "PASS" : "FAIL", (int)opened, (int)bound, (int)reset, (int)saved);
            if (!ok2) fails++;
        }
        if (!good) fails++;
        printf("key names: %s %s %s %s\n", keyName(keyFromName("lshift")).c_str(), keyName(keyFromName("F12")).c_str(), keyName(keyFromName("space")).c_str(), keyName(KEY_PAGE_UP).c_str());
    }
    {
        PadState cur, prev;
        cur.present = true; cur.lx = 0.9; cur.ly = -1.0; cur.rx = 0.5; cur.buttons[0] = true; cur.rt = 0.9;
        Input in;
        applyPad(cur, prev, in, 1.0);
        bool good = in.moveY > 0.9 && in.moveX > 0.8 && in.mouseDx > 1 && in.isDown(KEY_ENTER) && in.wasPressed(KEY_ENTER) && in.isDown(KEY_LEFT_SHIFT);
        Input in2;
        applyPad(cur, prev, in2, 1.0);   // held: down but not pressed again
        good = good && in2.isDown(KEY_ENTER) && !in2.wasPressed(KEY_ENTER);
        PadState none;
        Input in3;
        applyPad(none, prev, in3, 1.0);
        good = good && in3.moveX == 0 && !in3.isDown(KEY_ENTER);
        printf("gamepad: %s (stick -> move, A -> enter once, trigger -> sprint, absent pad -> nothing)\n", good ? "PASS" : "FAIL");
        if (!good) fails++;
    }
    return fails;
}

// M5-06: day length at 82 degrees latitude on a tilted felisian world, half an orbit apart
static void testSeasons() {
    for (int64_t x = 150; x < 320; x++)
        for (int64_t z = 20; z < 140; z++) {
            Star s;
            if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != PT_FELISIAN || b.parent >= 0 || b.axialTilt < 18 * DEG || b.locked) continue;
                double lat = 82 * DEG;
                auto dayLen = [&](double t) {
                    double dec = std::asin(clampd(sys.seasonOf(bi, t), -1, 1));
                    double cosH0 = -std::tan(lat) * std::tan(dec);
                    return cosH0 <= -1 ? 1e9 : (cosH0 >= 1 ? 0.0 : std::fabs(b.rotPeriod) * std::acos(cosH0) / PI);
                };
                // find the northern summer solstice by scanning one orbit
                double P = std::fabs(b.orbitPeriod), tBest = 0, best = -2;
                for (int k = 0; k < 96; k++) { double t = 1000.0 + P * k / 96.0; double v = sys.seasonOf(bi, t); if (v > best) { best = v; tBest = t; } }
                double d1 = dayLen(tBest), d2 = dayLen(tBest + P * 0.5);
                auto show = [&](double d) { char buf[64]; if (d > 1e8) return std::string("polar day"); if (d <= 0) return std::string("polar night"); snprintf(buf, sizeof buf, "%.1f h of %.1f", d / 3600, std::fabs(b.rotPeriod) / 3600); return std::string(buf); };
                printf("seasons: %s tilt %.1f deg, at 82 N: summer solstice %s, winter solstice %s (orbit %.1f d)\n", b.name.c_str(), b.axialTilt / DEG, show(d1).c_str(), show(d2).c_str(), P / 86400);
                bool ok = (d1 > 1e8 || d1 > d2 + 3600);
                printf("seasons: %s\n", ok ? "PASS (different day lengths)" : "FAIL");
                return;
            }
        }
    printf("seasons: no tilted felisian world found\n");
}

int main(int argc, char** argv) {
    makeDir("shots");
    makeDir("shots/tests");   // O7-01: every harness output lands here; shots/ itself holds the player's screenshots
    // `scale=N` anywhere on the command line renders at that resolution scale
    std::vector<char*> args;
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "scale=", 6) == 0) g_testScale = atoi(argv[i] + 6); else args.push_back(argv[i]);
    }
    argc = (int)args.size(); argv = args.data();
    if (g_testScale < 1) g_testScale = 1;
    if (g_testScale > 4) g_testScale = 4;
    setFramebufferScale(g_testScale);
    std::string mode = argc > 1 ? argv[1] : "stars";
    if (mode == "scene") {
        std::string want = argc > 2 ? argv[2] : "felisian_shore";
        if (argc > 3) setFramebufferScale(atoi(argv[3]));   // `scene <name> [scale] [warm]` (B-310): the frame at 1x .. 4x; R-307: warm > 0 runs that many seconds of the world first and stages the type's event in view
        double warm = argc > 4 ? atof(argv[4]) : 0;
        SpaceRenderer sr; StarNeighborhood nb;
        for (const CmpScene& sc : CMP_SCENES) {
            if (want != sc.name) continue;
            StarSystem sys; SurfaceView sv; double t = 0;
            if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { printf("no body\n"); return 1; }
            if (warm > 0) {
                Input in;
                for (double w = 0; w < warm; w += 0.05) { t += 0.05; sv.update(0.05, in, t, false); }
                double fx = std::sin(sv.player.yaw), fz = std::cos(sv.player.yaw);
                if (sc.type == PT_TECTONIC) {   // a fountain on the nearest lava cell ahead, six seconds in
                    bool found = false;
                    for (double d = 160; d <= 420 && !found; d += 8)
                        for (double s2 = -d * 0.45; s2 <= d * 0.45 && !found; s2 += 8) {   // within the view's cone, far enough for the column to fit the frame
                            double x = sv.player.x + fx * d - fz * s2, z = sv.player.z + fz * d + fx * s2;
                            if (sv.site.lod0.at((int)std::floor(x / 16), (int)std::floor(z / 16)).material == MAT_LAVA) { sv.eruptions.push_back({x, z, t - 6, 1}); found = true; }
                        }
                    if (found) { const SurfaceView::Eruption& e = sv.eruptions.back(); printf("  fountain staged %.0f m ahead\n", std::sqrt((e.x - sv.player.x) * (e.x - sv.player.x) + (e.z - sv.player.z) * (e.z - sv.player.z))); }
                    else printf("  fountain staged: no lava 160-420 m ahead\n");
                    sv.quake = 0.8;
                }
                if (sc.type == PT_BOMBARDED) sv.eruptions.push_back({sv.player.x + fx * 220, sv.player.z + fz * 220, t - 1.5, 2});
                if (sc.wantMat == -36) {   // move the clock to a moment the nearest vent's jet is at full strength
                    std::vector<GeyserVent> vents; geyserVents(sv.site.gen, sv.site.unitAt(sv.player.x, sv.player.z), vents);
                    uint64_t best = 0; double bd = 1e18;
                    for (const GeyserVent& v : vents) { double vx, vz; sv.site.localAt(v.unit, vx, vz); double d = (vx - sv.player.x) * (vx - sv.player.x) + (vz - sv.player.z) * (vz - sv.player.z); if (d < bd) { bd = d; best = v.id; } }
                    double t0 = t;
                    for (double dtq = 0; dtq < 400 && SurfaceView::basinJetStrength(best, t) < 0.9; dtq += 0.5) t = t0 + dtq;
                    Input in2; sv.update(0.05, in2, t, false);
                    printf("  nearest vent %.0f m off, strength %.2f at t+%.0f s\n", std::sqrt(bd), SurfaceView::basinJetStrength(best, t), t - t0);
                }
                if (sc.type == PT_DESERT) { SurfaceView::DustDevil d; d.x = sv.player.x + fx * 130 - fz * 20; d.z = sv.player.z + fz * 130 + fx * 20; d.height = 70; d.radius = 12; d.age = 40; d.life = 300; d.phase = 1.0; sv.devils.push_back(d); }
            }
            Framebuffer fb;
            sv.render(fb, t, nb.stars, sr, 1.0);
            fb.mush(2);
            if (sv.inBuggy && !sv.chaseCam) sv.cameraFeed(fb, t);   // R-204: the nose camera's picture
            std::string fn = std::string("shots/tests/scene_") + sc.name + ".png";
            saveFB(fb, fn.c_str());
            printf("%s -> %s (sun alt %.1f)\n", sc.name, fn.c_str(), sv.env.sun.altitude / DEG);
            if (sc.wantMat == -3 || sc.wantMat == -5 || sc.wantMat == -6 || sc.wantMat == -7)   // N3: the herd's states and the cost of life
                printf("  herd: %s; life update %.2f ms, draw %.2f ms\n", sv.testHerdStates().c_str(), sv.lastLifeMs[0], sv.lastLifeMs[1]);
            if (sc.wantMat == -4 || sc.wantMat <= -10) {   // N2: what the vegetation did
                const TerrainVertex& tvp = sv.site.lod0.at((int)std::floor(sv.player.x / 16), (int)std::floor(sv.player.z / 16));
                double cov = sv.testCanopyCoverage(200);
                printf("  biome %s, veg %.2f; trees drawn %d (near %d, mid %d, far %d), flora points %d; canopy coverage of the forest ground within 200 m: %.0f%%%s\n", BIOME_NAMES[tvp.biome], tvp.veg,
                       sv.lastTreesDrawn, sv.lastNearTrees, sv.lastMidTrees, sv.lastFarTrees, sv.lastFloraPoints, cov * 100, (sc.wantMat == -4 && cov < 0.6) ? " (FAIL: under 60%)" : "");
                printf("  families %d/%d, vegColor %.2f,%.2f,%.2f, vegColor2 %.2f,%.2f,%.2f, season phase %.2f, fog %.0f m, wind %.0f kn; bank 0 stops 14/34 = %d,%d,%d / %d,%d,%d; pixel(160,120) bank %d shade %.0f\n",
                       sv.site.gen.floraFamily, sv.site.gen.floraFamily2, sv.site.gen.vegColor.r, sv.site.gen.vegColor.g, sv.site.gen.vegColor.b, sv.site.gen.vegColor2.r, sv.site.gen.vegColor2.g, sv.site.gen.vegColor2.b,
                       sv.seasonPhaseLocal(), sv.env.fogDistance, sv.env.windKnots, fb.pal[14 * 3], fb.pal[14 * 3 + 1], fb.pal[14 * 3 + 2], fb.pal[34 * 3], fb.pal[34 * 3 + 1], fb.pal[34 * 3 + 2],
                       bankOf(fb.idx[(FBH * 3 / 5) * FBW + FBW / 2]), shadeOf(fb.idx[(FBH * 3 / 5) * FBW + FBW / 2]));
            }
            {   // N1: what the lower half is drawn with (bank histogram, mean shade, the majority bank's ramp stops)
                int hist[BANKS] = {0}; double sum = 0; int n = 0;
                for (int y = FBH / 2; y < FBH; y++) for (int x = 0; x < FBW; x++) { hist[bankOf(fb.idx[y * FBW + x])]++; sum += shadeOf(fb.idx[y * FBW + x]); n++; }
                int mb = 0; for (int k = 1; k < BANKS; k++) if (hist[k] > hist[mb]) mb = k;
                printf("  lower half: bank %d %d%%, mean shade %.1f; bank %d stops 14/34/47 = %d,%d,%d / %d,%d,%d / %d,%d,%d; sky %.2f snow %.2f dust %.2f underwater %d\n", mb, hist[mb] * 100 / n, sum / n, mb,
                       fb.pal[(mb * 64 + 14) * 3], fb.pal[(mb * 64 + 14) * 3 + 1], fb.pal[(mb * 64 + 14) * 3 + 2], fb.pal[(mb * 64 + 34) * 3], fb.pal[(mb * 64 + 34) * 3 + 1], fb.pal[(mb * 64 + 34) * 3 + 2],
                       fb.pal[(mb * 64 + 47) * 3], fb.pal[(mb * 64 + 47) * 3 + 1], fb.pal[(mb * 64 + 47) * 3 + 2], sv.env.skyBrightness, sv.env.snow, sv.env.dust, (int)sv.player.underwater);
            }
        }
        return 0;
    }
    if (mode == "capsule") {   // B-318: the surface as the capsule sees it on the way down: eight altitudes of the descent, the share of sky below the horizon
        std::string want = argc > 2 ? argv[2] : "felisian_mountains";
        SpaceRenderer sr; StarNeighborhood nb;
        for (const CmpScene& sc : CMP_SCENES) {
            if (want != sc.name) continue;
            StarSystem sys; SurfaceView sv; double t = 0;
            if (!setupSceneForType(sc.type, sc.latDeg, sc.alt, sc.yawOff, sc.pitch, sv, sys, nb, t, sc.wantMat)) { printf("no body\n"); return 1; }
            printf("  site: star %lld %lld %lld body %d lat %.4f lon %.4f t %.1f\n", (long long)sys.star.sx, (long long)sys.star.sy, (long long)sys.star.sz, sv.site.body, sv.site.lat0 / DEG, sv.site.lon0 / DEG, t);
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
                for (int y = std::max(0, (int)std::ceil(horizon)); y < FBH; y++) for (int x = 0; x < FBW; x++) { rows++; if (bankOf(fb.idx[y * FBW + x]) == 1 && fb.invz[y * FBW + x] == 0) holes++; }   // the beacon beam is on the sky bank too, but has a depth
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
    if (mode == "spot") {   // B-322: a frame at an exact place: `spot <sx> <sy> <sz> <body> <latDeg> <lonDeg> <t> <swim|ascent|stand> [pitchDeg] [yawDeg] [alt]`
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
        sv.init(&sys, bi, lat, lon, t);
        nb.update(sv.site.worldPos(t, 0, 0, 0));
        const Body& b = sys.bodies[bi];
        printf("%s (%s) at %.3f %.3f, t %.1f, sea %s %.1f, ground %.1f water %.1f\n", b.name.c_str(), PLANET_TYPES[b.type].name, lat / DEG, lon / DEG, t, sv.site.hasWater ? "yes" : "no", sv.site.seaLevel, sv.site.groundHeight(0, 0), sv.site.waterAt(0, 0));
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
            printf("swimming %d underwater %d at (%.0f, %.0f) eye %.2f water %.2f ground %.2f, %.0f m from the site\n", (int)sv.player.swimming, (int)sv.player.underwater, bx, bz, sv.player.y + sv.player.eyeHeight, w, sv.site.groundHeight(bx, bz), bd);
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
                printf("  groundHeight %.2f coarse %.2f waterAt %.2f nearBand %d lod0R %d nearR %d nearBlend %.2f\n", sv.site.groundHeight(bx, bz), sv.site.groundHeightCoarse(bx, bz), sv.site.waterAt(bx, bz), sv.site.nearBand, sv.site.lod0R, sv.site.nearR, sv.site.nearBlend);
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
                    char m = v.material == MAT_WATER ? 'W' : (v.material == MAT_ICE ? 'i' : (v.material == MAT_SAND ? 's' : (v.material == MAT_DUST ? 'd' : (v.material == MAT_LAVA ? 'L' : (v.material == MAT_BASALT ? 'b' : (v.material == MAT_SULPHUR ? 'y' : (v.material == MAT_SALT ? 'S' : '.')))))));
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
    if (mode == "landmarks") {   // O6-06: the sights round random land sites of a type (default felisian): kinds, the share of sites with one within 10 km, the cost of a cell
        int type = PT_FELISIAN;
        if (argc > 2) for (int k = 0; k < PT_COUNT; k++) { std::string nm = PLANET_TYPES[k].name; for (char& ch : nm) ch = ch == ' ' ? '_' : (char)tolower(ch); if (nm == argv[2]) type = k; }
        int sites = 0, within10 = 0, kinds[LM_COUNT] = {0}; double cellMs = 0; int cells = 0;
        forTypeBodies(type, 3, false, [&](const StarSystem& sys, int bi) {
            const Body& b = sys.bodies[bi];
            BodyGen g = BodyGen::make(b);
            std::vector<std::pair<double, double>> ls; landSitesOf(sys, bi, 0x1A, 8, ls);
            for (auto& st : ls) {
                Vec3 u = StarSystem::bodyFromLatLon(st.first, st.second);
                std::vector<Landmark> found;
                double t0 = nowSec();
                landmarksNear(g, b.name, u, 10000.0, found);
                double dt = (nowSec() - t0) * 1e3;
                int ci, cj; landmarkCellOf(g, u, ci, cj);
                double cellDeg = landmarkCellDeg(g);
                int nCells = (int)std::ceil(20000.0 / (cellDeg * DEG * g.R * 1000.0) + 1); nCells *= nCells;
                cellMs += dt; cells += nCells;
                sites++;
                bool near = false;
                for (const Landmark& L : found) {
                    kinds[L.kind]++;
                    double dlat, dlon; StarSystem::latLonFromBody(L.unit, dlat, dlon);
                    double d = std::acos(clampd(dot(L.unit, u), -1, 1)) * g.R * 1000.0;
                    if (d < 10000.0) near = true;
                    if (sites <= 3) printf("  %-16s %s %-14s %+.3f %+.3f  h %.0f m, prominence %.0f m, radius %.0f m, %.1f km off\n", b.name.c_str(), LANDMARK_KIND_SYMBOLS[L.kind], L.name.c_str(), dlat / DEG, dlon / DEG, L.heightM, L.prominenceM, L.radiusM, d / 1000.0);
                }
                if (near) within10++;
            }
        });
        printf("landmarks (%s): %d sites, %d (%.0f%%) with a landmark within 10 km; kinds:", PLANET_TYPES[type].name, sites, within10, sites ? 100.0 * within10 / sites : 0.0);
        for (int k = 0; k < LM_COUNT; k++) printf(" %s %d", LANDMARK_KIND_NAMES[k], kinds[k]);
        printf("; %.2f ms per grid cell searched (%d searched: scan %.1f ms, lakes %.1f ms each)\n", cells ? cellMs / cells : 0.0, landmarkCellsSearched(), landmarkCellsSearched() ? landmarkScanMs(0) / landmarkCellsSearched() : 0.0, landmarkCellsSearched() ? landmarkScanMs(1) / landmarkCellsSearched() : 0.0);
        return 0;
    }
    if (mode == "drain") {   // O6-03: the flood round the first land sites of a type (default felisian): the tile's cost, its channels, lakes and monotony, and the sample cost with and without it
        int type = PT_FELISIAN;
        if (argc > 2) for (int k = 0; k < PT_COUNT; k++) { std::string nm = PLANET_TYPES[k].name; for (char& ch : nm) ch = ch == ' ' ? '_' : (char)tolower(ch); if (nm == argv[2]) type = k; }
        int shown = 0;
        forTypeBodies(type, 3, false, [&](const StarSystem& sys, int bi) {
            const Body& b = sys.bodies[bi];
            BodyGen g = BodyGen::make(b);
            std::vector<std::pair<double, double>> sites; landSitesOf(sys, bi, 0x3A, 2, sites);
            for (auto& st : sites) {
                Vec3 u = StarSystem::bodyFromLatLon(st.first, st.second);
                double t0 = nowSec();
                DrainStats ds = drainageStats(g, u, 60);
                double tBuild = (nowSec() - t0) * 1e3;
                SurfaceSite site; site.init(&sys, bi, st.first, st.second, 1000.0);
                std::vector<double> h; double c1 = 1e9, c0 = 1e9;
                for (int pass = 0; pass < 2; pass++) { double ta = nowSec(); gridHeights(site, 48, 16, h); c1 = std::min(c1, (nowSec() - ta) * 1e6 / (49.0 * 49.0)); }
                setDrainageEnabled(false);
                for (int pass = 0; pass < 2; pass++) { double ta = nowSec(); gridHeights(site, 48, 16, h); c0 = std::min(c0, (nowSec() - ta) * 1e6 / (49.0 * 49.0)); }
                setDrainageEnabled(true);
                int rivers = 0, lakes = 0, wetCells = 0, salt = 0;
                for (int j = -60; j <= 60; j += 3) for (int i = -60; i <= 60; i += 3) { TerrainVertex tv = site.sampleAt(i * 64, j * 64, 64); if (tv.water > -1e8f && tv.h < tv.water) { wetCells++; if (tv.shore < 1e8f) { SurfaceSample sx = sampleSurface(g, site.unitAt(i * 64, j * 64), 64); if (sx.waterKind == 1) rivers++; else if (sx.waterKind == 2) lakes++; } } if (tv.material == MAT_SALT) salt++; }
                SurfaceSample s0 = sampleSurface(g, u, 16);
                printf("%-16s %6.2f %7.2f [%s] N0 %d: window of %d cells: %d channels (%.1f%%), %d lake cells (tile: %d lakes kept, %d breached), %d sea, largest %.0f km2, downstream lower or level %d/%d; tile %.0f ms (first touch %.0f ms); 16 m sample %.2f us (%.2f without); of 1681 cells at 64 m: %d wet (%d river, %d lake), %d salt (%s here)\n",
                       b.name.c_str(), st.first / DEG, st.second / DEG, traitList(g).c_str(), drainageLatticeN0(b.radiusKm), ds.cells, ds.channels, ds.cells ? 100.0 * ds.channels / ds.cells : 0.0, ds.lakes, ds.lakesKept, ds.lakesBreached, ds.sea, ds.maxAcc, ds.monotone, ds.tested, ds.msBuild, tBuild, c1, c0, wetCells, rivers, lakes, salt,
                       s0.waterKind == 1 ? "river" : (s0.waterKind == 2 ? "lake" : MATERIAL_NAMES[s0.material]));
                shown++;
            }
        });
        printf("drain: %d sites, %d tiles held\n", shown, drainageTilesHeld());
        return 0;
    }
    if (mode == "water") {   // B-320: the inland water of the first felisian worlds at 16 m detail: rivers, lakes, pools per thousand samples, the first of each
        int wanted = argc > 2 ? atoi(argv[2]) : 3, shown = 0;
        for (int64_t x = 150; x < 320 && shown < wanted; x++)
            for (int64_t z = 20; z < 140 && shown < wanted; z++) {
                Star s;
                if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && shown < wanted; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (b.type != PT_FELISIAN) continue;
                    BodyGen g = BodyGen::make(b);
                    long n = 0, land = 0, kinds[5] = {0, 0, 0, 0, 0}, wet[5] = {0, 0, 0, 0, 0};
                    double firstLat[5] = {0, 0, 0, 0, 0}, firstLon[5] = {0, 0, 0, 0, 0}; bool have[5] = {false, false, false, false, false};
                    // O6-03: a window of 1.2 x 1.2 degrees round the first land site of the body, at 16 m (the planet's drainage would
                    // take a tile per sample over the whole globe), 400 x 400 samples
                    std::vector<std::pair<double, double>> sites; landSitesOf(sys, bi, 0x3A, 1, sites);
                    double cla = sites[0].first / DEG, cln = sites[0].second;
                    DrainStats ds = drainageStats(g, StarSystem::bodyFromLatLon(cla * DEG, cln), 40);
                    printf("%-16s flood round %.1f %.1f: %d cells, %d channels, %d lake cells, %d sea; largest %.0f km2; downstream lower or level %d of %d; tile %.0f ms\n", b.name.c_str(), cla, cln / DEG,
                           ds.cells, ds.channels, ds.lakes, ds.sea, ds.maxAcc, ds.monotone, ds.tested, ds.msBuild);
                    for (double la = cla - 0.6; la <= cla + 0.6; la += 0.003)
                        for (int k = 0; k < 400; k++) {
                            double ln = cln + (k - 200) * 0.003 * DEG / std::max(0.2, std::cos(cla * DEG));
                            SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                            n++;
                            if (ss.height > 0) land++;
                            if (ss.waterKind) { kinds[ss.waterKind]++; if (ss.material == MAT_WATER) { wet[ss.waterKind]++; if (!have[ss.waterKind]) { have[ss.waterKind] = true; firstLat[ss.waterKind] = la; firstLon[ss.waterKind] = ln / DEG; } } }
                        }
                    printf("%-16s [%s] land %.0f%%: per 1000 samples rivers %.1f (wet %.1f), lakes %.1f (wet %.1f), pools %.1f (wet %.1f), rift lakes %.1f\n", b.name.c_str(), traitList(g).c_str(), 100.0 * land / n,
                           1000.0 * kinds[1] / n, 1000.0 * wet[1] / n, 1000.0 * kinds[2] / n, 1000.0 * wet[2] / n, 1000.0 * kinds[3] / n, 1000.0 * wet[3] / n, 1000.0 * kinds[4] / n);
                    for (int kd = 1; kd < 5; kd++) if (have[kd]) printf("    first %s at %.2f %.2f\n", kd == 1 ? "river" : (kd == 2 ? "lake" : (kd == 3 ? "pool" : "rift lake")), firstLat[kd], firstLon[kd]);
                    shown++;
                }
            }
        return 0;
    }
    if (mode == "climate") {   // B-321: the biome shares of the first felisian worlds, area-weighted, and why the ice is ice
        int wanted = argc > 2 ? atoi(argv[2]) : 24, shown = 0;
        int totBio[BIO_COUNT] = {0}; long totN = 0; int iceT = 0, iceCap = 0, iceSnow = 0;
        Rng r(0xC11A);
        for (int64_t x = 150; x < 320 && shown < wanted; x++)
            for (int64_t z = 20; z < 140 && shown < wanted; z++) {
                Star s;
                if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && shown < wanted; bi++) {
                    const Body& b = sys.bodies[bi];
                    if (b.type != PT_FELISIAN) continue;
                    BodyGen g = BodyGen::make(b);
                    setDrainageEnabled(false);   // O6-03: 1500 random points over the globe would build a tile each; the biome shares do not need the rivers
                    int bio[BIO_COUNT] = {0}; int n = 0, land = 0, inland = 0, lakes = 0;
                    for (int k = 0; k < 1500; k++) {
                        double lat = std::asin(r.range(-1, 1)), lon = r.range(-PI, PI);
                        SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(lat, lon), 400);
                        n++; bio[ss.biome]++; if (ss.height > 0) land++;
                        if (ss.material == MAT_WATER && ss.water > 0.5) { inland++; if (ss.biome == BIO_WETLAND) lakes++; }
                        if (ss.biome == BIO_ICE && ss.height > 0) {
                            double T = climateTempC(g, std::fabs(lat) / DEG, ss.height, 0, 0);
                            double lf = std::fabs(lat) / DEG / 85.0, snowLine = g.snowLine * std::max(0.0, 1.0 - lf * lf);
                            if (T < -8) iceT++; else if (ss.height > snowLine) iceSnow++; else iceCap++;
                        }
                    }
                    printf("%-14s tempK %3.0f bias %+5.1f sea %+.2f cap %2.0f snow %4.0f  land %3.0f%% inland water %.1f%% [%s]:", b.name.c_str(), b.tempK, g.tempBias, g.seaLevel, g.iceCapLat, g.snowLine, 100.0 * land / n, 100.0 * inland / n, traitList(g).c_str());
                    for (int k = 1; k < BIO_COUNT; k++) if (bio[k] * 100 / n >= 1) printf(" %s %d%%", BIOME_NAMES[k], bio[k] * 100 / n);
                    printf("\n");
                    for (int k = 0; k < BIO_COUNT; k++) totBio[k] += bio[k];
                    totN += n; shown++;
                }
            }
        printf("all %d worlds:", shown);
        for (int k = 1; k < BIO_COUNT; k++) printf(" %s %.0f%%", BIOME_NAMES[k], 100.0 * totBio[k] / totN);
        printf("\n  land ice by cause: cold %d, over the snow line %d, polar cap %d\n", iceT, iceSnow, iceCap);
        return 0;
    }
    if (mode == "compare") { renderCompare(); return 0; }
    if (mode == "sheet") { composeSheets(); return 0; }
    if (mode == "moonsky") { renderMoonSky(); { SpaceRenderer sr2; StarNeighborhood nb2; renderBinarySurface(sr2, nb2); } return 0; }
    if (mode == "bench") return argc > 2 && std::string(argv[2]) == "forest" ? runBenchForest() : runBench(argc > 2 && std::string(argv[2]) == "check");
    if (mode == "flow") { runGameFlow(); return 0; }
    if (mode == "landmaps") { renderLandingMaps(); return 0; }
    if (mode == "consistency") { runConsistency(); return 0; }
    if (mode == "bestiary") return runBestiary();
    if (mode == "drive") return runDrive();
    if (mode == "descent") return runDescent();   // B-307
    if (mode == "stability") { if (argc > 4) setFramebufferScale(atoi(argv[4])); return runStability(argc > 2 ? argv[2] : nullptr, argc > 3 ? atof(argv[3]) : 0.0); }   // B-309; B-313: `stability <scene> [metres per frame] [scale]`
    if (mode == "encounters") { runEncounters(argc > 2 ? atoi(argv[2]) : 300); return 0; }
    if (mode == "settings") { testSettings(); return 0; }
    if (mode == "regress") return runRegress(argc > 2 && std::string(argv[2]) == "bless");
    if (mode == "fuzz") return runFuzz(argc > 2 ? atoi(argv[2]) : 10000);
    if (mode == "reanchor") { testReanchor(); return 0; }
    if (mode == "survey") { runSurvey(argc > 2 ? atoi(argv[2]) : 2000); return 0; }
    if (mode == "audio") { testAudio(); return 0; }
    if (mode == "seasons") { testSeasons(); return 0; }
    if (mode == "input") return testInput();
    if (mode == "unit") return testUnit();
    if (mode == "terrain") { if (argc > 2) g_terrainType = atoi(argv[2]); return runTerrainStats(); }
    if (mode == "landforms") {   // O6-01: `landforms sites` (the fixed review sites) or `landforms <type> [flat|hills|mountains]`
        std::string a = argc > 2 ? argv[2] : "3";
        if (a == "sites") { renderLandformSites(); return 0; }
        int cls = -1;
        if (argc > 3) { std::string c = argv[3]; cls = c == "flat" ? 0 : (c == "hills" ? 1 : (c == "mountains" ? 2 : -1)); }
        renderLandforms(atoi(a.c_str()), cls);
        return 0;
    }
    if (mode == "gallery") { renderGallery(argc > 2 ? atoi(argv[2]) : PT_FELISIAN); return 0; }
    if (mode == "traits") { renderTraits(argc > 2 ? argv[2] : nullptr); return 0; }   // R-304/R-305
    if (mode == "surface") { renderSurface(argc > 2 ? atoi(argv[2]) : -1); return 0; }
    if (mode == "stars") testStars();
    else if (mode == "maps") renderMaps();
    else if (mode == "space") renderSpace();
    else printf("unknown mode\n");
    return 0;
}
