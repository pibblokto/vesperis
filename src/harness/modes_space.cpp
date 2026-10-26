// E-03: space's modes: `maps`, `space` (with the telescope's and the fronts' frames), `moonsky`, `almanac`, `showers`, `seasons`
#include "harness/harness.h"
#include "harness/test_worlds.h"

void renderMaps() {
    // find one body of each type and dump its map
    bool done[PT_COUNT] = {false};
    bool lockedDone = false;
    int remaining = PT_COUNT;
    std::vector<uint32_t> img(PlanetMap::W * PlanetMap::H * 2);
    for (auto& [x, z, s, sys] : scanSystems(150, 260, 20, 120, [&] { return (remaining > 0 || !lockedDone); })) {
        for (auto& b : sys.bodies) {
            if (done[b.type] && !(b.type == PT_FELISIAN && b.locked && !lockedDone)) continue;
            if (b.type == PT_COMPANION) { done[b.type] = true; remaining--; continue; }   // a sun has no map
            bool lockedShot = b.type == PT_FELISIAN && b.locked && done[b.type];   // one extra map of a locked living world
            if (lockedShot) lockedDone = true;
            else { done[b.type] = true; remaining--; }
            BodyGen g = BodyGen::make(b);
            PlanetMap m;
            double t0 = nowSec();
            m.generate(g);
            double t1 = nowSec();
            printf("map %-16s %s: %.0f ms\n", KINDS[b.type].name, b.name.c_str(), (t1 - t0) * 1000);
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
                    static const uint32_t matCol[] = {rgb(120, 110, 100), rgb(220, 200, 140), rgb(90, 160, 60), rgb(30, 90, 30), rgb(240, 240, 250), rgb(20, 50, 140), rgb(255, 120, 20),
                            rgb(200, 220, 255), rgb(255, 255, 255), rgb(240, 220, 235), rgb(60, 55, 55), rgb(160, 130, 110), rgb(200, 180, 140), rgb(120, 115, 112), rgb(230, 200, 90), rgb(40, 40, 42),
                            rgb(236, 233, 224), rgb(30, 42, 36), rgb(110, 92, 76), rgb(196, 118, 106), rgb(118, 152, 78), rgb(120, 58, 92), rgb(150, 140, 104), rgb(200, 118, 40),
                            rgb(78, 104, 40)};   // mats, stromatolite, crust, moss
                    static_assert(sizeof(matCol) / sizeof(matCol[0]) == MAT_COUNT, "a map colour for every material (P-00)");
                    uint32_t mc = matCol[m.material[i]];
                    img[PlanetMap::W * PlanetMap::H + i] = (xx < PlanetMap::W / 2) ? hc : mc;
                }
            std::string fn = std::string("shots/tests/map_") + (lockedShot ? "LOCKED_" : "") + KINDS[b.type].name + ".png";
            for (char& ch : fn) if (ch == ' ') ch = '_';
            writePNG(fn.c_str(), img.data(), PlanetMap::W, PlanetMap::H * 2);
            if (lockedShot) printf("  locked: sub-stellar point lat %.0f lon %.0f -> %s\n", std::asin(b.lockedDir.z) / DEG, std::atan2(b.lockedDir.y, b.lockedDir.x) / DEG, fn.c_str());
        }
    }
}

static void renderTelescopeFrames(SpaceRenderer& sr, StarNeighborhood& nb);   // W-06, below
static void renderFrontFrames(SpaceRenderer& sr, StarNeighborhood& nb);       // W-03, below
void renderSpace() {
    SpaceRenderer sr;
    StarNeighborhood nb;
    // 1) one sun per class, seen from the first orbit distance
    bool done[STAR_DRAWN_COUNT] = {false};
    int remaining = STAR_DRAWN_COUNT;
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
    // S-06: the first black hole with a companion, from 20 degrees above the plane of its worlds at six tenths of the first orbit:
    // the shadow, the disc arching over and under it, the bent stars and the companion's stream into the disc
    for (int64_t x = 150; x < 300; x++) {
        bool got = false;
        for (int64_t z = 20; z < 120 && !got; z++) {
            Star s;
            if (!starInSector(x, 0, z, s) || s.cls != STAR_BLACK_HOLE) continue;
            StarSystem sys; sys.generate(s);
            if (sys.companion < 0) continue;
            got = true;
            double d = STAR_CLASSES[s.cls].minFirstOrbitKm * 0.6;
            Vec3 shipPos = s.pos + Vec3(0, 0.34 * d, -0.94 * d);
            nb.update(shipPos);
            Framebuffer fb;
            SpaceContext c;
            c.sys = &sys; c.stars = &nb.stars; c.t = 1234.0; c.shipPos = shipPos;
            c.cam = cameraBasis(0, -0.35);
            sr.setupPalette(fb, &sys, -1, -1, 1.0);
            sr.render(fb, c);
            fb.mush(2);
            saveFB(fb, "shots/tests/space_black_hole.png");
            const Body& k = sys.bodies[sys.companion];
            printf("black hole %s with a companion (%s, %.3g km out, %d bodies, %d belts): shots/tests/space_black_hole.png\n", s.name.c_str(), STAR_CLASSES[k.starClass].name, k.orbitRadiusKm,
                    (int)sys.bodies.size(), (int)sys.belts.size());
        }
        if (got) break;
    }
    // 2) planet globes: one per type, parked view
    bool doneT[PT_COUNT] = {false};
    int remT = PT_COUNT;
    int gasWithRings = 0;
    for (auto& [x, z, s, sys] : scanSystems(150, 300, 20, 120, [&] { return remT; })) {
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
            std::string fn = std::string("shots/tests/globe_") + KINDS[b.type].name + (b.rings ? "_rings" : "") + ".png";
            for (char& ch : fn) if (ch == ' ') ch = '_';
            saveFB(fb, fn.c_str());
            printf("globe %-16s %s R=%.0f rings=%d star=%s\n", KINDS[b.type].name, b.name.c_str(), b.radiusKm, (int)b.rings, STAR_CLASSES[s.cls].code);
            if (b.type == PT_GASGIANT) {   // the storms a day later
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
    // the rocks of a belt seen from beside the biggest rock near an anchor in the belt
    {
        bool found = false;
        for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return !found; })) {
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
            printf("belt %s: %d rocks within two cells of the anchor, parked beside one %.1f km across, %d drawn (%d as meshes) -> shots/space_belt.png; an hour later the rock is %s, %.0f km along "
                   "its orbit\n",
                    bl.name.c_str(), n, 2 * best.radiusKm, sr.lastBeltRocks, sr.lastBeltMeshes, same && std::fabs(later.radiusKm - best.radiusKm) < 1e-9 ? "still there" : "GONE (FAIL)",
                    same ? length(later.pos - best.pos) : 0.0);
            found = true;
        }
        if (!found) printf("belt: none found in the scanned sectors\n");
    }
    // the sky from a star-forming region: nebula patches over the star field
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
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return !(binaryDone && cometDone); })) {
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
                printf("  world of the companion: %s (%s) -> shots/space_binary_world.png\n", b.name.c_str(), KINDS[b.type].name);
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
                // seen from above the orbital plane, so the dust tail's curve along the orbit shows
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
                // the tail streams (frames 5 s apart differ, the knots moved), the comet drifts against
                // the sky at x100 seen from the first planet, and the parking view shows the jets moving
                {
                    const uint64_t FNV0 = 1469598103934665603ULL;
                    uint64_t h0 = fnv(FNV0, fb.idx.data(), fb.idx.size() * sizeof(Pix));
                    Framebuffer fb5; SpaceContext c5 = c; c5.t = t + 5;
                    sr.setupPalette(fb5, &sys, bi, -1, 1.0); sr.render(fb5, c5); fb5.mush(2);
                    uint64_t h5 = fnv(FNV0, fb5.idx.data(), fb5.idx.size() * sizeof(Pix));
                    printf("  tail 5 s later: frame %s; knot 0 at %.3f -> %.3f of the tail\n", h0 != h5 ? "differs" : "IDENTICAL (FAIL)", SpaceRenderer::cometKnot(0, t),
                            SpaceRenderer::cometKnot(0, t + 5));
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
                    printf("  parked at 12 radii (%.0f km): nucleus %.0f px across -> shots/space_comet_close.png; 3 s later %d px changed (jets %s)\n", b.radiusKm * 12, nucPx, changed,
                            changed > 500 ? "stream" : "DO NOT MOVE (FAIL)");
                }
                cometDone = true;
            }
    }
    renderTelescopeFrames(sr, nb);
    renderFrontFrames(sr, nb);
}

// W-06: a frame through the telescope: the view from `shipPos` on `aim`, magnified `zoom` times (the focal length multiplied,
// as the game does); the body's disc radius in pixels is returned through `rpxOut` for the body `bankA`
// `plate`: the body `bankA` through the plate (the planet function at the eyepiece's footprint, read out whole before the frame: the
// user's review of W-06), else the world map alone (the eyepiece before the review, for the comparison)
double telescopeFrame(
        SpaceRenderer& sr, StarNeighborhood& nb, StarSystem& sys, double t, const Vec3& shipPos, const Vec3& aim, double zoom, int bankA, int bankB, const char* file, double* rpxOut, bool plate) {
    nb.update(shipPos);
    Vec3 fwd = normalize(aim - shipPos);
    Framebuffer fb; SpaceContext c;
    c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
    c.bankBodyA = bankA; c.bankBodyB = bankB; c.starIntensity = 1.5;
    c.detailBody = plate ? bankA : -1; c.detailBudget = 0;
    if (bankA >= 0) warmMaps(sr, sys, bankA);
    if (bankB >= 0) warmMaps(sr, sys, bankB);
    Proj saved = sr.proj; sr.proj = Proj::fromHFov(70); sr.proj.f *= zoom;
    sr.setupPalette(fb, &sys, bankA, bankB, 1.0);
    double t0 = nowSec(); sr.render(fb, c); double ms = (nowSec() - t0) * 1000;
    fb.mush(2); saveFB(fb, file);
    if (rpxOut) *rpxOut = bankA >= 0 && bankA < (int)sr.bodyInfo.size() ? sr.bodyInfo[bankA].radiusPx : 0;
    if (plate) printf("  %s (%.1f ms with the build)\n", sr.plateInfo().c_str(), ms);
    sr.proj = saved;
    return ms;
}
// the ground a telescope's ground frame looks at: the highest point (land on a world with seas) within sixty degrees of the
// sub-ship point, the nearer the better
static bool telescopeGroundPoint(SpaceRenderer& sr, const StarSystem& sys, int bi, double t, const Vec3& ship, double& latOut, double& lonOut) {
    const Body& b = sys.bodies[bi];
    const PlanetMap& m = sr.mapFor(b);
    Mat3 frame = sys.bodyFrame(bi, t);
    Vec3 sub = normalize(frame * (ship - sys.bodyPos(bi, t)));
    double best = -1e18; bool found = false;
    for (int y = 1; y < PlanetMap::H - 1; y += 2)
        for (int x = 0; x < PlanetMap::W; x += 2) {
            int i = y * PlanetMap::W + x;
            if (matFamily(m.material[i]) == FAM_WATER || m.material[i] == MAT_ICE) continue;
            double lat = (0.5 - (y + 0.5) / PlanetMap::H) * PI, lon = ((x + 0.5) / PlanetMap::W) * TAU - PI;
            double cc = dot(StarSystem::bodyFromLatLon(lat, lon), sub);
            if (cc < 0.5) continue;   // within sixty degrees of the sub-ship point
            double score = m.height[i] + 1500 * cc;
            if (score > best) { best = score; latOut = lat; lonOut = lon; found = true; }
        }
    return found;
}
// the power of two that shows a disc of `angR` (radians) `share` of the frame's height tall, within x2..x1024
static double telescopeFit(double angR, double share) {
    double rpx1 = (FBW * 0.5) / std::tan(35 * DEG) * std::tan(angR);
    double want = share * FBH * 0.5 / std::max(rpx1, 1e-9);
    int step = (int)std::lround(std::log2(std::max(want, 2.0)));
    return (double)(1 << std::max(1, std::min(10, step)));
}

// W-06: four frames through the telescope: a moon from its giant's parking, the ringed giant from the innermost planet's
// parking, the companion sun from the primary's first orbit, a comet near periapsis from the first planet; each at the
// power of two that fits it, the disc measured against the plain view's (the magnification must multiply it exactly)
static void renderTelescopeFrames(SpaceRenderer& sr, StarNeighborhood& nb) {
    auto parkAt = [](const StarSystem& sys, int bi, double t) {
        const Body& b = sys.bodies[bi];
        Vec3 bp = sys.bodyPos(bi, t);
        Vec3 toStar = normalize(sys.star.pos - bp), side = normalize(cross(toStar, Vec3(0, 1, 0)));
        return bp + normalize(toStar * 0.6 + side * 0.8 + Vec3(0, 0.25, 0)) * (b.radiusKm * (b.rings ? std::max(3.5, b.ringOuter + 1.3) : 3.5));
    };
    auto firstPlanet = [](const StarSystem& sys, int except) {
        for (int j = 0; j < (int)sys.bodies.size(); j++) if (j != except && sys.bodies[j].parent < 0 && sys.bodies[j].type != PT_COMET && sys.bodies[j].type != PT_COMPANION) return j;
        return -1;
    };
    auto lastPlanet = [](const StarSystem& sys, int except) {
        for (int j = (int)sys.bodies.size() - 1; j >= 0; j--) if (j != except && sys.bodies[j].parent < 0 && sys.bodies[j].type != PT_COMET && sys.bodies[j].type != PT_COMPANION) return j;
        return -1;
    };
    bool moonDone = false, companionDone = false, cometDone = false, worldDone = false;
    for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160, [&] { return !(moonDone && companionDone && cometDone && worldDone); })) {
        const double t = 7000.0;
        if (!worldDone)   // a living world from its own parking: the disc at the power that fits it, then a patch of its ground at x256 (rivers, coasts, forests from the function)
            for (int bi = 0; bi < (int)sys.bodies.size() && !worldDone; bi++) {
                const Body& w = sys.bodies[bi];
                if (w.type != PT_FELISIAN || w.parent >= 0) continue;
                Vec3 ship = parkAt(sys, bi, t), wp = sys.bodyPos(bi, t);
                double dist = length(wp - ship), angR = std::asin(clampd(w.radiusKm / dist, 0, 1));
                double zoom = telescopeFit(angR, 0.8), rz = 0;
                telescopeFrame(sr, nb, sys, t, ship, wp, zoom, bi, -1, "shots/tests/telescope_world_map.png", &rz, false);
                double ms = telescopeFrame(sr, nb, sys, t, ship, wp, zoom, bi, -1, "shots/tests/telescope_world.png", &rz);
                printf("telescope world: %s (%s, R %.0f km) from its parking, %.0f km off: x%.0f, disc %.0f px, %.1f ms -> shots/tests/telescope_world.png (telescope_world_map.png: the map alone)\n",
                       w.name.c_str(), KINDS[w.type].name, w.radiusKm, dist, zoom, rz, ms);
                double glat = 0, glon = 0; Vec3 aim = wp;
                if (telescopeGroundPoint(sr, sys, bi, t, ship, glat, glon)) aim = sys.surfacePointWorld(bi, t, glat, glon, 0);
                telescopeFrame(sr, nb, sys, t, ship, aim, 64, bi, -1, "shots/tests/telescope_world_land_map.png", &rz, false);
                double ms1 = telescopeFrame(sr, nb, sys, t, ship, aim, 64, bi, -1, "shots/tests/telescope_world_land.png", &rz);
                double footprint = (dist - w.radiusKm) * 1000.0 / ((FBW * 0.5) / std::tan(35 * DEG));
                printf("telescope world land: its high ground at %.1f N %.1f E at x64 (%.0f m a pixel), %.1f ms -> shots/tests/telescope_world_land.png (telescope_world_land_map.png: the map "
                       "alone)\n",
                        glat / DEG, glon / DEG, footprint / 64, ms1);
                double ms2 = telescopeFrame(sr, nb, sys, t, ship, aim, 256, bi, -1, "shots/tests/telescope_world_ground.png", &rz);
                printf("telescope world ground: the same ground at x256 (%.0f m a pixel), %.1f ms -> shots/tests/telescope_world_ground.png\n", footprint / 256, ms2);
                double ms3 = telescopeFrame(sr, nb, sys, t, ship, aim, 1024, bi, -1, "shots/tests/telescope_world_close.png", &rz);
                printf("telescope world close: the same ground at x1024 (%.0f m a pixel), %.1f ms -> shots/tests/telescope_world_close.png\n", footprint / 1024, ms3);
                worldDone = true;
            }
        if (!moonDone)
            for (int bi = 0; bi < (int)sys.bodies.size() && !moonDone; bi++) {
                const Body& g = sys.bodies[bi];
                if (g.type != PT_GASGIANT || !g.rings || g.parent >= 0) continue;
                int moon = -1;
                for (int j = 0; j < (int)sys.bodies.size(); j++) if (sys.bodies[j].parent == bi && (moon < 0 || sys.bodies[j].radiusKm > sys.bodies[moon].radiusKm)) moon = j;
                if (moon < 0) continue;
                Vec3 ship = parkAt(sys, bi, t), mp = sys.bodyPos(moon, t);
                double dist = length(mp - ship), angR = std::asin(clampd(sys.bodies[moon].radiusKm / dist, 0, 1));
                double zoom = telescopeFit(angR, 0.55), r1 = 0, rz = 0;
                telescopeFrame(sr, nb, sys, t, ship, mp, 1.0, moon, bi, "shots/tests/telescope_moon_plain.png", &r1, false);
                double ms = telescopeFrame(sr, nb, sys, t, ship, mp, zoom, moon, bi, "shots/tests/telescope_moon.png", &rz);
                printf("telescope moon: %s (%s, R %.0f km) of the ringed giant %s from its parking, %.0f km off, %.2f deg across: x%.0f fits it, disc %.1f px (%.1f at x1: %s), %.1f ms -> "
                       "shots/tests/telescope_moon.png\n",
                        sys.bodies[moon].name.c_str(), KINDS[sys.bodies[moon].type].name, sys.bodies[moon].radiusKm, g.name.c_str(), dist, 2 * angR / DEG, zoom, rz, r1,
                        std::fabs(rz - zoom * r1) < 0.02 * rz ? "the magnification exact" : "NOT THE MAGNIFICATION (FAIL)", ms);
                {   // the same moon four times closer, the map alone and through the plate, and its ground at x1024 (the plate's step against the map's)
                    double z4 = std::min(1024.0, zoom * 4);
                    telescopeFrame(sr, nb, sys, t, ship, mp, z4, moon, bi, "shots/tests/telescope_moon_near_map.png", &rz, false);
                    double ms4 = telescopeFrame(sr, nb, sys, t, ship, mp, z4, moon, bi, "shots/tests/telescope_moon_near.png", &rz);
                    printf("telescope moon near: the same moon at x%.0f, disc %.0f px, %.1f ms -> shots/tests/telescope_moon_near.png (telescope_moon_near_map.png: the map alone)\n", z4, rz, ms4);
                    // the moon's ground from the moon's own parking (from the giant's, 3.7 km a pixel at x1024 is above the function's relief): its high
                    // ground within sixty degrees of the sub-ship point at the power that puts a pixel near 80 m
                    Vec3 shipM = parkAt(sys, moon, t);
                    double footprint1 = (length(sys.bodyPos(moon, t) - shipM) - sys.bodies[moon].radiusKm) * 1000.0 / ((FBW * 0.5) / std::tan(35 * DEG));   // metres a pixel at x1
                    int step = (int)std::lround(std::log2(std::max(2.0, footprint1 / 80.0)));
                    double zg = (double)(1 << std::max(1, std::min(10, step)));
                    double glat = 0, glon = 0; Vec3 aim = mp;
                    if (telescopeGroundPoint(sr, sys, moon, t, shipM, glat, glon)) aim = sys.surfacePointWorld(moon, t, glat, glon, 0);
                    telescopeFrame(sr, nb, sys, t, shipM, aim, zg, moon, bi, "shots/tests/telescope_ground_map.png", &rz, false);
                    double msg = telescopeFrame(sr, nb, sys, t, shipM, aim, zg, moon, bi, "shots/tests/telescope_ground.png", &rz);
                    printf("telescope ground: the same moon from its own parking at x%.0f (%.0f m a pixel) on its high ground at %.1f N %.1f E, %.1f ms -> shots/tests/telescope_ground.png "
                           "(telescope_ground_map.png: the map alone)\n",
                            zg, footprint1 / zg, glat / DEG, glon / DEG, msg);
                }
                int from = firstPlanet(sys, bi);
                if (from >= 0) {   // the giant with its rings from across the system
                    Vec3 ship2 = parkAt(sys, from, t), gp = sys.bodyPos(bi, t);
                    double d2 = length(gp - ship2), angRings = std::asin(clampd(g.ringOuter * g.radiusKm / d2, 0, 1));
                    double zoom2 = telescopeFit(angRings, 0.6), rg = 0;
                    double ms2 = telescopeFrame(sr, nb, sys, t, ship2, gp, zoom2, bi, from, "shots/tests/telescope_giant.png", &rg);
                    printf("telescope giant: %s from the parking of %s, %.3g km off (%.2f deg across, the rings %.2f): x%.0f, disc %.1f px, %.1f ms -> shots/tests/telescope_giant.png\n",
                           g.name.c_str(), sys.bodies[from].name.c_str(), d2, 2 * std::asin(clampd(g.radiusKm / d2, 0, 1)) / DEG, 2 * angRings / DEG, zoom2, rg, ms2);
                } else printf("telescope giant: %s has no planet to look from\n", g.name.c_str());
                moonDone = true;
            }
        if (!companionDone && sys.companion >= 0) {
            const Body& k = sys.bodies[sys.companion];
            double d = std::max(s.radiusKm * STAR_CLASSES[s.cls].firstOrbitMult, STAR_CLASSES[s.cls].minFirstOrbitKm);
            Vec3 ship = s.pos + Vec3(0, 0, -d), kp = sys.bodyPos(sys.companion, t);
            double dist = length(kp - ship), angR = std::asin(clampd(k.radiusKm / dist, 0, 1));
            double zoom = telescopeFit(angR, 0.35), rz = 0;
            double ms = telescopeFrame(sr, nb, sys, t, ship, kp, zoom, sys.companion, -1, "shots/tests/telescope_companion.png", &rz);
            printf("telescope companion: %s's %s (R %.0f km) from the first orbit, %.3g km off, %.3f deg across: x%.0f, disc %.1f px, %.1f ms -> shots/tests/telescope_companion.png\n",
                   s.name.c_str(), STAR_CLASSES[k.starClass].name, k.radiusKm, dist, 2 * angR / DEG, zoom, rz, ms);
            companionDone = true;
        }
        if (!cometDone)
            for (int bi = 0; bi < (int)sys.bodies.size() && !cometDone; bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != PT_COMET) continue;
                int from = lastPlanet(sys, bi);   // from the outermost planet: the comet at its star is a telescope's object, not the window's
                if (from < 0) continue;
                double tp = (TAU - b.orbitPhase0) / TAU * b.orbitPeriod + 0.08 * b.orbitPeriod;   // a little past periapsis: the tail streams away from the star
                Vec3 ship = parkAt(sys, from, tp), cp = sys.bodyPos(bi, tp);
                double dist = length(cp - ship);
                double refKm = sys.bodies[0].type == PT_COMET ? AU_GAME_KM : sys.bodies[0].orbitRadiusKm;
                double field = 0.8 * (refKm * 0.5) / dist;   // the coma and the first part of the tail (its length as the comet frame draws it) in the frame
                int step = (int)std::floor(std::log2(std::max(std::tan(35 * DEG) / std::tan(std::min(field, 1.2) * 0.5), 2.0)));
                double zoom = (double)(1 << std::max(1, std::min(10, step))), rz = 0;
                double ms = telescopeFrame(sr, nb, sys, tp, ship, cp, zoom, bi, from, "shots/tests/telescope_comet.png", &rz);
                printf("telescope comet: %s from the parking of %s, %.3g km off, %.3g km from its star: x%.0f, nucleus %.2f px, %.1f ms -> shots/tests/telescope_comet.png\n",
                       b.name.c_str(), sys.bodies[from].name.c_str(), dist, length(cp - s.pos), zoom, rz, ms);
                cometDone = true;
            }
    }
    if (!moonDone) printf("telescope moon: no ringed giant with a moon in the scanned sectors\n");
    if (!worldDone) printf("telescope world: no living world in the scanned sectors\n");
    if (!companionDone) printf("telescope companion: no binary in the scanned sectors\n");
    if (!cometDone) printf("telescope comet: none in the scanned sectors\n");
}

// W-03: the pinned plain's world from a parking over the plain an hour before a front's line reaches it, at x1 and x4: the
// fronts' bands on the globe (the same overlay the landing map draws), the plain under the centre of the disc
static void renderFrontFrames(SpaceRenderer& sr, StarNeighborhood& nb) {
    StarSystem sys; int bi = -1; FrontForecast fc;
    if (!pinnedPlainFront(sys, bi, 1000.0, fc)) { printf("fronts: no front reaches the pinned plain by day within sixty days\n"); return; }
    const Body& b = sys.bodies[bi];
    double t = fc.tArrive - 3600;
    Vec3 up = sys.bodyFrame(bi, t).transposed() * StarSystem::bodyFromLatLon(PIN_PLAIN_LAT * DEG, PIN_PLAIN_LON * DEG);
    Vec3 ship = sys.bodyPos(bi, t) + up * (b.radiusKm * 3.5);
    double rpx = 0;
    double ms1 = telescopeFrame(sr, nb, sys, t, ship, sys.bodyPos(bi, t), 1, bi, -1, "shots/tests/space_fronts.png", &rpx, false);
    double ms4 = telescopeFrame(sr, nb, sys, t, ship, sys.bodyPos(bi, t), 4, bi, -1, "shots/tests/space_fronts_x4.png", nullptr, false);
    std::vector<Front> fs; frontsOf(b, t, fs, 0); int alive = 0; for (const Front& f : fs) if (frontLife(f, t) > 0) alive++;
    const SpaceRenderer::FrontMap* fm = sr.frontsFor(b, t);
    double band = fm ? frontMapAt(fm->cloud, PlanetMap::W, PlanetMap::H, PIN_PLAIN_LON * DEG, PIN_PLAIN_LAT * DEG) : 0;
    int covered = 0; if (fm) for (uint8_t v : fm->cloud) if (v > 128) covered++;
    printf("fronts: %s (%s, R %.0f km, %d slots): %d fronts alive at t %.0f, bands over %.1f%% of the map, %.2f over the plain an hour before the line (from the %s, strength %.2f); disc %.0f px at "
           "x1 (%.1f ms), x4 %.1f ms -> shots/tests/space_fronts.png, space_fronts_x4.png\n",
            b.name.c_str(), KINDS[b.type].name, b.radiusKm, frontSlots(b), alive, t, fm ? 100.0 * covered / fm->cloud.size() : 0.0, band, frontCompass(fc.fromBearing), fc.strength, rpx, ms1, ms4);
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

void renderMoonSky() {
    SpaceRenderer sr;
    StarNeighborhood nb;
    int shots = 0;
    bool eclipseDone = false;
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return shots < 3; })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && shots < 3; bi++) {
            const Body& b = sys.bodies[bi];
            if (b.parent < 0 || !KINDS[b.type].landable) continue;
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
            printf("moon %s (%s) of %s (%s): parent angular radius %.1f deg, alt %.1f, sunAlt %.1f\n", b.name.c_str(), KINDS[b.type].name,
                   par.name.c_str(), KINDS[par.type].name, ang, std::asin(dl.y) / DEG, sv.env.sun.altitude / DEG);
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
    // a moon of a ringed planet, and a moon of a substellar object at night
    bool ringsDone = false, subDone = false;
    for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160, [&] { return !(ringsDone && subDone); })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && !(ringsDone && subDone); bi++) {
            const Body& b = sys.bodies[bi];
            if (b.parent < 0 || !KINDS[b.type].landable) continue;
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
            printf("%s: moon %s (%s) of %s (%s), parent alt %.1f, sun alt %.1f, night glow %.2f -> %s\n", wantSub ? "substellar" : "rings", b.name.c_str(), KINDS[b.type].name,
                   par.name.c_str(), KINDS[par.type].name, std::asin(dl.y) / DEG, sv.env.sun.altitude / DEG, sv.env.moonLight, fn);
            if (wantSub) subDone = true; else ringsDone = true;
        }
    }
    // the rings of a ringed world seen from its own surface, by day and by night
    bool surfDone = false;
    for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160, [&] { return !surfDone; })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && !surfDone; bi++) {
            const Body& b = sys.bodies[bi];
            if (!b.rings || !KINDS[b.type].landable || b.parent >= 0) continue;
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
                printf("rings from the surface of %s (%s, tilt %.0f deg, ring %.2f-%.2f R): sun alt %.1f -> %s\n", b.name.c_str(), KINDS[b.type].name, b.axialTilt / DEG, b.ringInner, b.ringOuter,
                        sv.env.sun.altitude / DEG, fn.c_str());
            }
            surfDone = true;
        }
    }
}

// M5-01: a surface with two suns up (part of `moonsky`)
void renderBinarySurface(SpaceRenderer& sr, StarNeighborhood& nb) {
    for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160)) {
        if (sys.companion < 0) continue;
        for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
            const Body& b = sys.bodies[bi];
            if (!KINDS[b.type].landable || b.parent >= 0) continue;
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
            printf("binary surface: %s (%s) of %s: primary alt %.0f az %.0f, companion %s alt %.0f az %.0f radius %.2f deg, share %.2f -> shots/binary_surface.png\n", b.name.c_str(),
                    KINDS[b.type].name, sys.classString().c_str(), s1.altitude / DEG, s1.azimuth / DEG, STAR_CLASSES[sys.bodies[sys.companion].starClass].name, s2.altitude / DEG, s2.azimuth / DEG,
                    s2.angularRadius / DEG, sv.env.sun2Share);
            return;
        }
    }
    printf("binary surface: none found\n");
}
void almanacTestPlaces(std::vector<AlmanacTestPlace>& out) {
    AlmanacTestPlace moon, locked, plain; bool fm = false, fl = false, fp = false;
    for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160, [&] { return !(fm && fl && fp); })) {
        for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
            const Body& b = sys.bodies[bi];
            if (!KINDS[b.type].landable || b.type == PT_COMET) continue;
            if (!fm && b.parent >= 0 && sys.bodies[b.parent].type == PT_GASGIANT && sys.bodies[b.parent].rings) { moon.sys = sys; moon.body = bi; moon.what = "a moon of a ringed giant"; fm = true; }
            if (!fl && b.parent < 0 && b.locked && b.moonCount > 0) { locked.sys = sys; locked.body = bi; locked.what = "a locked world"; fl = true; }
            if (!fp && b.parent < 0 && !b.locked && b.moonCount > 0 && b.axialTilt > 2 * DEG && KINDS[b.type].atmosphere) { plain.sys = sys; plain.body = bi; plain.what = "a plain world"; fp = true; }
        }
    }
    if (fm) out.push_back(moon);
    if (fl) out.push_back(locked);
    if (fp) out.push_back(plain);
}
AlmanacPlace almanacGroundPlace(const SurfaceSite& site, double t) {
    AlmanacPlace p;
    p.sys = site.sys;
    p.t = t;
    p.ground = true;
    p.body = site.body;
    p.lat = site.lat0;
    p.lon = site.lon0;
    p.atmosphere = site.atmosphere;
    p.ringProf = site.ringProf;
    p.window = 5 * 86400.0;
    return p;
}
AlmanacPlace almanacShipPlace(const StarSystem& sys, int body, double t) {   // parked as `Game::testParkAt` parks, the orbit's lap on
    AlmanacPlace p; p.sys = &sys; p.t = t; p.ground = false; p.body = body; p.window = 30 * 86400.0;
    const Body& b = sys.bodies[body];
    Vec3 bp = sys.bodyPos(body, t), toStar = normalize(sys.star.pos - bp), side = normalize(cross(toStar, Vec3(0, 1, 0)));
    p.parkDir = normalize(toStar * 0.7 + side * 0.7 + Vec3(0, 0.2, 0)); p.parkDist = Game::parkDistanceFor(b); p.orbiting = true;
    p.pos = bp + p.parkDir * p.parkDist;
    return p;
}
static void printAlmanac(const AlmanacPlace& p, const std::vector<AlmanacEvent>& ev) {
    const StarSystem& sys = *p.sys;
    auto name = [&](int j) { return sys.bodies[j].name; };
    if (p.ground) {
        const Body& b = sys.bodies[p.body];
        std::string of = b.parent >= 0 ? fmt(", moon of %s%s", sys.bodies[b.parent].name.c_str(), sys.bodies[b.parent].rings ? " (ringed)" : "") : "";
        double angR, alt = almanacSunAltitude(p, p.t, &angR);
        printf("the almanac of %s (%s%s%s%s%s) at %.1f %s %.1f %s, t %.0f: the sun at %+.1f deg, %d moon%s, day %.1f h, year %.1f h\n", b.name.c_str(), KINDS[b.type].name, of.c_str(),
                b.locked ? ", locked" : "", b.rings ? ", rings" : "", KINDS[b.type].atmosphere ? "" : ", airless", std::fabs(p.lat / DEG), p.lat >= 0 ? "N" : "S", std::fabs(p.lon / DEG),
                p.lon >= 0 ? "E" : "W", p.t, alt / DEG, b.moonCount, b.moonCount == 1 ? "" : "s", std::fabs(b.rotPeriod) / 3600, std::fabs(b.orbitPeriod) / 3600);
    } else
        printf("the almanac from the ship %s, t %.0f\n",
                p.body >= 0 ? fmt("parked at %s (%s, %d moons)", sys.bodies[p.body].name.c_str(), KINDS[sys.bodies[p.body].type].name, sys.bodies[p.body].moonCount).c_str() : "in deep space", p.t);
    for (const AlmanacEvent& e : ev) printf("  in %-12s %s\n", countdownString(e.t - p.t).c_str(), almanacLabel(e, sys, name, " deg").c_str());
    if (ev.empty()) printf("  nothing within the window\n");
}
// W-04: the meteor showers: `showers` scans the systems near home (how many worlds have one, the strongest), `showers <sx> <sy>
// <sz> <body>` lists one world's (the comet, the stream's strength, the orbits' least distance, the peak, its width, the radiant)
static void printShower(const StarSystem& sys, const MeteorShower& s, double t) {
    double tp = showerPeakAfter(s, t);
    double ra = std::atan2(s.radiant.z, s.radiant.x) / DEG, dec = std::asin(clampd(s.radiant.y, -1, 1)) / DEG;
    printf("  %-22s strength %.2f (%.0f a minute at the peak), orbits %.0f km apart (%.2f%% of the world's orbit), the year %.1f h, the next peak in %.1f h, width %.0f min, the radiant at %.1f / "
           "%+.1f (ecliptic), %.1f km/s\n",
            sys.bodies[s.comet].name.c_str(), s.strength, s.strength * SHOWER_PEAK_PER_MIN, s.moidKm, 100 * s.moidKm / sys.bodies[s.world].orbitRadiusKm, s.period / 3600, (tp - t) / 3600,
            s.sigma / 60, ra, dec, s.speedKms);
}

int runShowers(int argc, char** argv) {
    double t = 3.6e6;
    if (argc >= 6) {
        Star st; StarSystem sys; if (!systemAt(atoll(argv[2]), atoll(argv[3]), atoll(argv[4]), st, sys, true)) { printf("no star\n"); return 1; }
        int bi = atoi(argv[5]);
        if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
        std::vector<MeteorShower> sh; double t0 = nowSec(); meteorShowersOf(sys, bi, sh);
        printf("%s (%s): %zu shower%s [%.1f ms]\n", sys.bodies[bi].name.c_str(), KINDS[sys.bodies[bi].type].name, sh.size(), sh.size() == 1 ? "" : "s", (nowSec() - t0) * 1000);
        for (const MeteorShower& s : sh) printShower(sys, s, t);
        return 0;
    }
    int systems = 0, withComets = 0, worlds = 0, worldsWith = 0, showers = 0; double ms = 0;
    struct Ex { double strength; int64_t sx, sz; int body; std::string name; };
    std::vector<Ex> best;
    for (auto& [x, z, st, sys] : scanSystems(150, 200, 20, 80)) {
systems++;
        bool comet = false; for (const Body& b : sys.bodies) if (b.type == PT_COMET) comet = true;
        if (comet) withComets++;
        for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
            const Body& b = sys.bodies[bi];
            if (b.parent >= 0 || b.type == PT_COMET || b.type == PT_COMPANION) continue;
            worlds++;
            std::vector<MeteorShower> sh; double t0 = nowSec(); meteorShowersOf(sys, bi, sh); ms += (nowSec() - t0) * 1000;
            if (sh.empty()) continue;
            worldsWith++; showers += (int)sh.size();
            best.push_back({sh[0].strength, x, z, bi, b.name});
        }
    }
    std::sort(best.begin(), best.end(), [](const Ex& a, const Ex& b) { return a.strength > b.strength; });
    printf("showers near home: %d systems (%d with comets), %d planets, %d with a shower (%.1f%%), %d showers; %.2f ms a world\n",
           systems, withComets, worlds, worldsWith, 100.0 * worldsWith / std::max(1, worlds), showers, ms / std::max(1, worlds));
    for (int i = 0; i < (int)best.size() && i < 12; i++) {
        Star st; starInSector(best[i].sx, 0, best[i].sz, st); StarSystem sys; sys.generate(st);
        std::vector<MeteorShower> sh; meteorShowersOf(sys, best[i].body, sh);
        printf("%s (sector %lld 0 %lld, body %d, %s):\n", best[i].name.c_str(), (long long)best[i].sx, (long long)best[i].sz, best[i].body, KINDS[sys.bodies[best[i].body].type].name);
        for (const MeteorShower& s : sh) printShower(sys, s, t);
    }
    return 0;
}

int runAlmanac(int argc, char** argv) {   // `almanac [<sx> <sy> <sz> <body> [latDeg lonDeg [t [ship]]]]`
    double t = 3.6e6;
    if (argc >= 6) {
        int64_t sx = atoll(argv[2]), sy = atoll(argv[3]), sz = atoll(argv[4]); int body = atoi(argv[5]);
        double latDeg = argc > 7 ? atof(argv[6]) : 12, lonDeg = argc > 7 ? atof(argv[7]) : 0.5 / DEG;
        if (argc > 8) t = atof(argv[8]);
        bool shipView = argc > 9 && std::string(argv[9]) == "ship";
        Star s; StarSystem sys; if (!systemAt(sx, sy, sz, s, sys, true)) { printf("no star at %lld %lld %lld\n", (long long)sx, (long long)sy, (long long)sz); return 1; }
        if (body < 0 || body >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", body, sys.bodies.size()); return 1; }
        AlmanacPlace p; SurfaceSite site;
        if (shipView || !KINDS[sys.bodies[body].type].landable) p = almanacShipPlace(sys, body, t);
        else { site.init(&sys, body, latDeg * DEG, lonDeg * DEG, t); p = almanacGroundPlace(site, t); }
        std::vector<AlmanacEvent> ev; double t0 = nowSec(); almanacOf(p, ev);
        printf("[%.1f ms]\n", (nowSec() - t0) * 1000);
        printAlmanac(p, ev);
        return 0;
    }
    std::vector<AlmanacTestPlace> places; almanacTestPlaces(places);
    for (AlmanacTestPlace& tp : places) {
        SurfaceSite site; site.init(&tp.sys, tp.body, tp.latDeg * DEG, tp.lonDeg * DEG, t);
        AlmanacPlace p = almanacGroundPlace(site, t);
        std::vector<AlmanacEvent> ev; double t0 = nowSec(); almanacOf(p, ev);
        printf("[%s: %s (sector %lld 0 %lld), %.1f ms]\n", tp.what, tp.sys.star.name.c_str(), (long long)tp.sys.star.sx, (long long)tp.sys.star.sz, (nowSec() - t0) * 1000);
        printAlmanac(p, ev);
    }
    if (!places.empty() && places[0].sys.bodies[places[0].body].parent >= 0) {
        AlmanacPlace p = almanacShipPlace(places[0].sys, places[0].sys.bodies[places[0].body].parent, t);
        std::vector<AlmanacEvent> ev; double t0 = nowSec(); almanacOf(p, ev);
        printf("[the ship at the giant, %.1f ms]\n", (nowSec() - t0) * 1000);
        printAlmanac(p, ev);
    }
    return 0;
}

// M5-06: day length at 82 degrees latitude on a tilted felisian world, half an orbit apart
void testSeasons() {
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
                auto show = [&](double d) {
                    char buf[64];
                    if (d > 1e8) return std::string("polar day");
                    if (d <= 0) return std::string("polar night");
                    snprintf(buf, sizeof buf, "%.1f h of %.1f", d / 3600, std::fabs(b.rotPeriod) / 3600);
                    return std::string(buf);
                };
                printf("seasons: %s tilt %.1f deg, at 82 N: summer solstice %s, winter solstice %s (orbit %.1f d)\n", b.name.c_str(), b.axialTilt / DEG, show(d1).c_str(), show(d2).c_str(), P / 86400);
                bool ok = (d1 > 1e8 || d1 > d2 + 3600);
                printf("seasons: %s\n", ok ? "PASS (different day lengths)" : "FAIL");
                return;
            }
        }
    printf("seasons: no tilted felisian world found\n");
}
