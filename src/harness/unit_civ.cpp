// E-03: the unit's civilisations and what they leave (groups `civilisations`, `ruins`, `charts`, `roads`, `graves`)
#include "harness/harness.h"

// A-03: a building's room as the explorer finds it: its middle clear of the colliders given (0.9 m), and the longest run along its
// doorway's wall line (metres) clear of them at the explorer's radius
static void roomOpenness(const Building& b, const BuildingRoom& R, const std::vector<FormDisc>& discs, bool& centreClear, double& doorRun) {
    auto blockedAt = [&](double x, double z, double margin) { for (const FormDisc& q : discs) if (q.y0 < 1.0 && q.y1 > 0.6 && std::hypot(q.x - x, q.z - z) < q.r + margin) return true; return false; };
    const double cx = b.x + std::cos(b.heading) * R.cx + std::sin(b.heading) * R.cz, cz = b.z - std::sin(b.heading) * R.cx + std::cos(b.heading) * R.cz;
    centreClear = R.outside || !blockedAt(cx, cz, R.round ? 0.6 : 0.9);
    doorRun = 0;
    if (!R.door) return;
    int run = 0, best = 0;
    for (double t = -R.doorL; t <= R.doorL; t += 0.1) { run = blockedAt(R.doorX + R.doorAx * t, R.doorZ + R.doorAz * t, 0.35) ? 0 : run + 1; best = std::max(best, run); }
    doorRun = best * 0.1;
}

// C-01: the civilisations
void unitCivilisations(const Check& check) {
    {   // C-01: the civilisations. One felisian or desert world in eight near home had a people; a dead desert world keeps its seas as dry
        // beds the wadis run down to; its settlements are hamlets, villages and towns whose houses are rooms to walk into
        int fel = 0, felC = 0, des = 0, desC = 0, bodyFel = -1, bodyDes = -1; Star firstFel, firstDes; bool haveFel = false, haveDes = false;
        for (int64_t x = 150; x < 300; x++) for (int64_t z = 20; z < 120; z++) {
            Star s; if (!starInSector(x, 0, z, s, false)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != PT_FELISIAN && b.type != PT_DESERT) continue;
                bool civ = BodyGen::make(b).hasTrait(TR_CIVILISATION);
                if (b.type == PT_FELISIAN) { fel++; if (civ) { felC++; if (!haveFel) { starInSector(x, 0, z, firstFel, true); bodyFel = bi; haveFel = true; } } }
                else { des++; if (civ) { desC++; if (!haveDes) { starInSector(x, 0, z, firstDes, true); bodyDes = bi; haveDes = true; } } }
            }
        }
        check("civilisations: one felisian or desert world in eight", felC > 0 && desC > 0 && felC * 100 >= fel * 5 && felC * 100 <= fel * 22 && desC * 100 >= des * 5 && desC * 100 <= des * 22, fmt("felisian %d of %d, desert %d of %d", felC, fel, desC, des));
        if (haveDes) {   // the old sea: a fifth to two thirds of the face, its bed under the land, salt in its deeps; at its shore the flood's outlet and the wadis
            StarSystem sys; sys.generate(firstDes);
            const Body& b = sys.bodies[bodyDes];
            BodyGen g = BodyGen::make(b);
            int n = 0, sea = 0, salt = 0, nLand = 0; double hSea = 0, hLand = 0; bool shoreFound = false; Vec3 shoreU;
            for (int j = 0; j < 36; j++) for (int i = 0; i < 72; i++) {
                double lat = (-85 + 170.0 * (j + 0.5) / 36) * DEG, lon = (-180 + 360.0 * (i + 0.5) / 72) * DEG;
                Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
                DrainageOff off;   // A-03: the old sea is the planet function's, not the rivers' (a tile a sample would take minutes)
                SurfaceSample s = sampleSurface(g, u, 2048);
                n++;
                if (s.oldSea > 0.5) { sea++; hSea += s.height; if (s.material == MAT_SALT) salt++; } else { nLand++; hLand += s.height; }
                if (!shoreFound && s.oldSea > 0.35 && s.oldSea < 0.65 && std::fabs(lat) < 60 * DEG) { shoreFound = true; shoreU = u; }
            }
            hSea /= std::max(1, sea); hLand /= std::max(1, nLand);
            DrainStats st; if (shoreFound) { bool wasOn = drainageEnabled(); setDrainageEnabled(true); st = drainageStats(g, shoreU, 40); setDrainageEnabled(wasOn); }   // an earlier check leaves the drainage off
            check("dead desert world: dry seas, the bed under the land, wadis to the shore", sea * 100 >= n * 15 && sea * 100 <= n * 70 && hSea < hLand && salt > 0 && shoreFound && st.sea > 0 && st.channels > 0,
                  fmt("%s (the sea stood at %.0f m): the old sea on %d%% of %d points (salt on %d), its bed %.0f m against the land's %.0f; at the shore %d sea cells, %d channels of %d", b.name.c_str(), g.oldSeaM, sea * 100 / n, n, salt, hSea, hLand, st.sea, st.channels, st.cells));
        } else check("dead desert world: dry seas", false, "no dead desert world in the scan");
        if (haveFel) {   // B-422: the landing map's cursor asked for its drainage tiles at every step, and the places it had passed kept every
            // core busy: a superseded prefetch's tiles are abandoned (A's, at a tile's middle: unbuilt once B's is in); a tile a thread
            // waits for is finished all the same (C read while its prefetch is superseded by D's)
            StarSystem sys; sys.generate(firstFel);
            const BodyGen g = BodyGen::make(sys.bodies[bodyFel]);
            const bool wasOn = drainageEnabled(); setDrainageEnabled(true);
            const Vec3 A = StarSystem::bodyFromLatLon(30.5 * DEG, 100.5 * DEG), B = StarSystem::bodyFromLatLon(-35.5 * DEG, -79.5 * DEG);
            const Vec3 C = StarSystem::bodyFromLatLon(20.5 * DEG, -150.5 * DEG), D = StarSystem::bodyFromLatLon(-10.5 * DEG, 160.5 * DEG);
            const bool fresh = !drainageReady(g, A, 0) && !drainageReady(g, B, 0) && !drainageReady(g, C, 0) && !drainageReady(g, D, 0);
            auto waitReady = [&](const Vec3& u) { const double t0 = nowSec(); while (nowSec() - t0 < 30 && !drainageReady(g, u, 0)) std::this_thread::sleep_for(std::chrono::milliseconds(10)); return drainageReady(g, u, 0); };
            drainagePrefetch(g, A, 30000.0, true);
            drainagePrefetch(g, B, 0.0, true);
            const bool bIn = waitReady(B);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            const bool abandoned = !drainageReady(g, A, 0);
            drainagePrefetch(g, C, 0.0, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            DrainInfo dc; std::thread reader([&] { dc = drainageAt(g, C, 64.0); });
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            drainagePrefetch(g, D, 0.0, true);
            reader.join();
            const bool readerHas = drainageReady(g, C, 0), dIn = waitReady(D);
            setDrainageEnabled(wasOn);
            check("drainage: a superseded prefetch's tiles abandoned, a tile waited for finished (B-422)", fresh && bIn && abandoned && readerHas && dIn,
                  fmt("%s: the four places' tiles unbuilt at first %s; A's prefetch superseded by B's: B's in %s, A's middle %s; C's read while its prefetch was superseded: %s, D's in %s",
                      sys.bodies[bodyFel].name.c_str(), fresh ? "yes" : "NO", bIn ? "yes" : "NO", abandoned ? "unbuilt (abandoned)" : "BUILT", readerHas ? "its tile there" : "NO TILE", dIn ? "yes" : "NO"));
        }
        if (haveFel || haveDes) {   // the settlements of the first world: the classes' shares, every house a room with a doorway (the pieces), and the view's colliders
            StarSystem sys; sys.generate(haveFel ? firstFel : firstDes);
            int bi = haveFel ? bodyFel : bodyDes;
            const Body& b = sys.bodies[bi];
            BodyGen g = BodyGen::make(b);
            Culture cu = cultureOf(g);
            int cls[7] = {0, 0, 0, 0, 0, 0, 0}, cells = 0, houses = 0, roomsOk = 0, doorsOk = 0, gLat0, gLon0;
            int shSettle = 0, shTotal = 0, shByClass[7] = {0, 0, 0, 0, 0, 0, 0}, shCountBad = 0, shBad = 0, shDup = 0, shOut = 0, shBlocked = 0, shDoor = 0, shPlaces[BK_COUNT] = {}; bool shDeterm = true;   // C-03
            double la0 = 0.2, lo0 = 0.7; planPlacePoint(g, 0, la0, lo0);   // T-01: the people's countryside, round a town of its plan
            ruinCellOf(g, la0, lo0, gLat0, gLon0);
            std::vector<Form> el;
            for (int dl = -15; dl <= 15; dl++) for (int dn = -15; dn <= 15; dn++) {
                RuinSpec sp; cells++;
                if (!ruinOfCell(g, gLat0 + dl, gLon0 + dn, sp, true)) continue;
                cls[sp.sclass]++;
                ruinElements(sp, cu, 0, el);
                {   // C-03: the settlement's shards: counted by the class, distinct, inside their building, clear of its pieces and its doorway, the same twice
                    std::vector<ShardSite> ss, ss2; recordSitesOf(g, sp, cu, ss); recordSitesOf(g, sp, cu, ss2);
                    shSettle++; shTotal += (int)ss.size(); shByClass[sp.sclass] += (int)ss.size();
                    static const int LO[7] = {0, 1, 2, 0, 4, 6, 0}, HI[7] = {1, 2, 4, 1, 7, 10, 0};   // T-01: a city's and the capital's too
                    int lo = LO[sp.sclass], hi = HI[sp.sclass], alone = 0;
                    for (const ShardSite& x : ss) if (!x.archive && !x.last) alone++;   // T-04: an archive's and the last recording on top; a place of the plan holds its own records
                    if (sp.rank < 0 && (alone < lo || alone > hi)) shCountBad++;
                    if (ss.size() != ss2.size()) shDeterm = false; else for (size_t i = 0; i < ss.size(); i++) if (ss[i].index != ss2[i].index || ss[i].x != ss2[i].x || ss[i].z != ss2[i].z) shDeterm = false;
                    for (size_t i = 0; i < ss.size(); i++) {
                        const ShardSite& s = ss[i];
                        if (s.index < 0 || shardLocalOf(s.index) >= recordCountOf(g, sp.people) || s.place < 0 || s.place >= BK_COUNT) { shBad++; continue; }
                        for (size_t j = 0; j < i; j++) if (ss[j].index == s.index) shDup++;
                        if (s.building < 0 || s.building >= (int)sp.buildings.size()) { shBad++; continue; }
                        const Building& bd = sp.buildings[s.building];
                        double dx = s.x - bd.x, dz = s.z - bd.z, qx = dx * std::cos(bd.heading) - dz * std::sin(bd.heading), qz = dx * std::sin(bd.heading) + dz * std::cos(bd.heading);
                        BuildingRoom R; buildingRoom(bd, R);   // A-03: in its room (a round one's circle, a rectangle round its middle) or at its foot
                        bool in = R.outside ? std::hypot(dx, dz) < std::max(bd.hw, bd.hd) + 6 : (R.round ? std::hypot(dx, dz) < R.r - 0.3 : (std::fabs(qx - R.cx) < R.hw - 0.3 && std::fabs(qz - R.cz) < R.hd - 0.3));
                        if (!in) shOut++;
                        shPlaces[s.place]++;
                        for (const Form& e : el) { if (e.building != s.building || e.kind != FK_BOX || e.y0 > 1.0) continue; double ex = s.x - e.x, ez = s.z - e.z; double px = ex * std::cos(e.heading) - ez * std::sin(e.heading), pz = ex * std::sin(e.heading) + ez * std::cos(e.heading); if (std::fabs(px) < e.hx + 0.4 && std::fabs(pz) < e.hz + 0.4) { shBlocked++; break; } }
                        double ddx, ddz; if (buildingDoor(bd, ddx, ddz) && std::hypot(s.x - ddx, s.z - ddz) < 1.4) shDoor++;
                    }
                }
                for (size_t k = 0; k < sp.buildings.size(); k++) {   // A-03: every building's room (C-01's houses among them), against its own pieces' colliders
                    const Building& bd = sp.buildings[k]; BuildingRoom R;
                    if (!buildingRoom(bd, R) || R.outside) continue;
                    houses++;
                    std::vector<FormDisc> discs; for (const Form& e : el) if (e.building == (int)k) formColliders(e, discs);
                    bool clear; double run; roomOpenness(bd, R, discs, clear, run);
                    if (clear) roomsOk++;
                    if (!R.door || run >= 1.0) doorsOk++;
                }
            }
            check("settlements: hamlets, villages, towns and lone monuments", cls[0] > cls[1] && cls[1] >= cls[2] && cls[2] > 0 && cls[3] > 0, fmt("%s: %d hamlets, %d villages, %d towns, %d monuments in %d cells (B-417: the old ones' monoliths gone)", b.name.c_str(), cls[0], cls[1], cls[2], cls[3], cells));
            check("settlements: every room open, its doorway a metre clear", houses > 0 && roomsOk == houses && doorsOk == houses, fmt("%d rooms (A-03: every building's), %d with the middle clear, %d with a metre of the doorway clear at the explorer's radius", houses, roomsOk, doorsOk));
            check("shards: a few per settlement, in a room or at a stela, clear of the pieces and the doorway, distinct, deterministic",   // C-03
                  shSettle > 0 && shTotal > 0 && shCountBad == 0 && shBad == 0 && shDup == 0 && shOut == 0 && shBlocked == 0 && shDoor == 0 && shDeterm && shByClass[2] > 0 && shTotal * 10 >= shSettle * 5,
                  fmt("%d shards in %d settlements (in hamlets %d, villages %d, towns %d, monuments %d; %s); %d counts out of bounds, %d out of their building, %d in a piece, %d in a doorway, %d repeats, %s",
                      shTotal, shSettle, shByClass[0], shByClass[1], shByClass[2], shByClass[3], [&] { std::string p; for (int k = 0; k < BK_COUNT; k++) if (shPlaces[k]) p += fmt("%s%d in %s", p.empty() ? "" : ", ", shPlaces[k], buildingPlaceName(k)); return p; }().c_str(), shCountBad, shOut, shBlocked, shDoor, shDup, shDeterm ? "the same twice" : "NOT deterministic"));
            {   // the view: the first town's houses, the colliders leaving the centre clear and the doorway open; the town drawn from outside its edge, timed
                int tLat = 0, tLon = 0; bool tFound = false; RuinSpec tsp;
                {   // from a point on low land (the fixed start of the counts above may lie in an ocean): the first of a 2048 m grid under 40 degrees of latitude
                    DrainageOff off;   // A-03: as the view sites them, without the rivers' tiles
                    int lLat = gLat0, lLon = gLon0; bool land = true;   // T-01: the plan's town's countryside is land
                    for (int j = 0; j < 36 && !land; j++) for (int i = 0; i < 72 && !land; i++) {
                        double la = (-85 + 170.0 * (j + 0.5) / 36) * DEG, lo = (-180 + 360.0 * (i + 0.5) / 72) * DEG;
                        if (std::fabs(la) > 40 * DEG) continue;
                        SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 2048);
                        if (ss.material != MAT_WATER && ss.height > 10 && ss.relief < 0.3 && ss.oldSea < 0.5) { ruinCellOf(g, la, lo, lLat, lLon); land = true; }
                    }
                    for (int ring = 0; ring < 40 && !tFound; ring++) for (int dl = -ring; dl <= ring && !tFound; dl++) for (int dn = -ring; dn <= ring && !tFound; dn++) {
                        if (std::max(std::abs(dl), std::abs(dn)) != ring) continue;
                        RuinSpec sp; if (ruinOfCell(g, lLat + dl, lLon + dn, sp, false) && sp.sclass == SC_TOWN && ruinSiteOk(g, sp)) { tLat = lLat + dl; tLon = lLon + dn; tsp = sp; tFound = true; }
                    }
                    if (!tFound) printf("    (no town passed the site test within 40 rings of %d:%d%s)\n", lLat, lLon, land ? "" : ", no low land found");
                }
                int clearN = 0, openN = 0, hN = 0, spanN = 0, spanBad = 0, rocksInRooms = 0; double ms = 0, tallest = 0; int pieces = 0; std::string where = tFound ? "the view found no town at the cell" : "no town within 40 rings of low land";
                int vShards = 0, vClear = 0, vRocks = 0, vDrawn = 0; bool vReach = false, vGone = false;   // C-03
                int gN = 0, gPieces = 0; bool gReach = false; std::string gLine;   // C-12
                if (tFound) {
                    SurfaceView sv;
                    sv.init(&sys, bi, tsp.lat, tsp.lon, 0.0);
                    Ruin ru;
                    if (sv.ruinAt(tLat, tLon, ru)) {
                        std::vector<SurfaceView::Collider> cols;
                        for (size_t k = 0; k < ru.spec->buildings.size(); k++) {   // A-03: every building's room (C-01's houses among them)
                            const Building& bd = ru.spec->buildings[k]; BuildingRoom R;
                            if (!buildingRoom(bd, R) || R.outside) continue;
                            hN++;
                            double hx = ru.x + bd.x + std::cos(bd.heading) * R.cx + std::sin(bd.heading) * R.cz, hz = ru.z + bd.z - std::sin(bd.heading) * R.cx + std::cos(bd.heading) * R.cz;
                            sv.collectColliders(hx, hz, cols);
                            bool clear = true;
                            for (const auto& c : cols) if (c.kind == 3 && c.y0 < sv.site.groundHeight(c.x, c.z) + 1.0 && std::hypot(c.x - hx, c.z - hz) < c.r + (R.round ? 0.6 : 0.9)) clear = false;
                            for (const auto& c : cols) {   // C-08 (KI-345): no rock's collider inside the room
                                if (c.kind != 0) continue;
                                double dx = c.x - hx, dz = c.z - hz, lx = dx * std::cos(bd.heading) - dz * std::sin(bd.heading), lz = dx * std::sin(bd.heading) + dz * std::cos(bd.heading);
                                if (R.round ? std::hypot(lx, lz) < R.r : (std::fabs(lx) < R.hw && std::fabs(lz) < R.hd)) rocksInRooms++;
                            }
                            // B-405: every piece's collider spans its own height over the ground it stands on, so the jetpack clears what it has risen above
                            for (const auto& c : cols) {
                                if (c.kind != 3) continue;
                                double gnd = sv.site.groundHeight(c.x, c.z); spanN++;
                                if (c.y1 - c.y0 < 0.85 || c.y1 - c.y0 > 600 || c.y0 - gnd < -8) spanBad++;   // A-03: a tower's shaft spans its height over its lobby
                                tallest = std::max(tallest, c.y1 - gnd);
                            }
                            if (clear) clearN++;
                            if (!R.door) { openN++; continue; }
                            sv.collectColliders(ru.x + R.doorX, ru.z + R.doorZ, cols);
                            int run = 0, bestRun = 0;
                            for (double tt = -R.doorL; tt <= R.doorL; tt += 0.1) {
                                double px = ru.x + R.doorX + R.doorAx * tt, pz = ru.z + R.doorZ + R.doorAz * tt; bool blocked = false;
                                for (const auto& c : cols) if (c.kind == 3 && c.y0 < sv.site.groundHeight(px, pz) + 1.0 && std::hypot(c.x - px, c.z - pz) < c.r + 0.35) { blocked = true; break; }
                                run = blocked ? 0 : run + 1; bestRun = std::max(bestRun, run);
                            }
                            if (bestRun * 0.1 >= 1.0) openN++;
                        }
                        {   // C-03: the town's shards in the view: no piece's collider within 0.45 m of a site (the explorer can stand at it), the first
                            // within reach when the explorer stands a metre from it, drawn from there, and neither offered once taken
                            const SurfaceView::RuinCell* rc = sv.ruinCell(tLat, tLon);
                            vShards = (int)rc->shards.size();
                            for (const ShardSite& s : rc->shards) {
                                double x = ru.x + s.x, z = ru.z + s.z, gnd = sv.site.groundHeight(x, z);
                                sv.collectColliders(x, z, cols);
                                bool clear = true;
                                for (const auto& c : cols) { if (std::hypot(c.x - x, c.z - z) >= c.r + 0.45 || c.y0 > gnd + 1.0) continue; if (c.kind == 3) clear = false; else vRocks++; }
                                if (clear) vClear++;
                            }
                            if (!rc->shards.empty()) {
                                const ShardSite& s = rc->shards[0]; const Building& bd = rc->spec.buildings[s.building];   // (`tsp` was read without its buildings)
                                double x = ru.x + s.x, z = ru.z + s.z, ax = bd.x - s.x, az = bd.z - s.z, L = std::max(1e-6, std::sqrt(ax * ax + az * az));
                                if (s.outside) { ax = -ax; az = -az; }
                                sv.player.x = x + ax / L; sv.player.z = z + az / L; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                                sv.player.yaw = std::atan2(x - sv.player.x, z - sv.player.z); sv.player.pitch = -0.9;
                                Input in0; sv.update(0.016, in0, 0.0, false);
                                vReach = sv.nearShard.index == s.index;
                                Framebuffer fb0; SpaceRenderer sr0; StarNeighborhood nb0;
                                sv.render(fb0, 0.0, nb0.stars, sr0, 1.0);
                                vDrawn = sv.lastShardsDrawn;
                                saveFB(fb0, "shots/tests/unit_shard_room.png");
                                sv.shardsFound.insert(s.index); sv.update(0.016, in0, 0.0, false); vGone = sv.nearShard.index != s.index; sv.shardsFound.clear();
                            }
                        }
                        {   // C-12: the town's graves in the view: their stones among the pieces, the first within reach when the explorer stands a metre
                            // before its face, its name and years read
                            const SurfaceView::RuinCell* rc = sv.ruinCell(tLat, tLon);
                            gN = (int)rc->graves.size();
                            for (const Form& e : rc->elems[0]) if (e.part == 5 && e.building < 0) gPieces++;
                            if (gN > 0) {
                                const Grave& g = rc->graves[0];
                                double x = ru.x + g.x, z = ru.z + g.z;
                                sv.player.x = x + std::sin(g.heading); sv.player.z = z + std::cos(g.heading); sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                                sv.player.yaw = g.heading + PI; sv.player.pitch = -0.35;
                                Input in0; sv.update(0.016, in0, 0.0, false);
                                gReach = sv.nearGrave.k == 0 && sv.nearGrave.id == g.id; gLine = sv.nearGrave.line;
                                Framebuffer fb0; SpaceRenderer sr0; StarNeighborhood nb0;
                                sv.render(fb0, 0.0, nb0.stars, sr0, 1.0);
                                saveFB(fb0, "shots/tests/unit_grave.png");
                            }
                        }
                        sv.player.x = ru.x - tsp.size - 30; sv.player.z = ru.z; sv.player.yaw = PI / 2; sv.player.pitch = 0.02;
                        Framebuffer fb; SpaceRenderer sr; StarNeighborhood nb;
                        sv.render(fb, 0.0, nb.stars, sr, 1.0);
                        ms = sv.lastRuinMs; pieces = sv.lastRuinElems;
                        where = fmt("%s at %.2f %.2f", b.name.c_str(), tsp.lat / DEG, tsp.lon / DEG);
                    }
                }
                check("settlements: the colliders leave the rooms and doorways open and stand as tall as their walls", tFound && hN > 0 && clearN == hN && openN == hN && spanN > 0 && spanBad == 0 && tallest > 0.9 && tallest < 3000 && rocksInRooms == 0,
                      fmt("%s: %d rooms of a town (A-03: every building's), %d clear, %d open; %d colliders, %d without a wall's span, the tallest %.1f m over its ground, %d rocks in the rooms; the town drawn in %.1f ms (%d pieces)", where.c_str(), hN, clearN, openN, spanN, spanBad, tallest, rocksInRooms, ms, pieces));
                check("shards: the town's sites are clear in the view, within reach at a metre, drawn, and gone once taken", tFound && vShards >= 2 && vClear == vShards && vReach && vDrawn >= 1 && vGone,   // C-03
                      fmt("%d shards in the town, %d clear of the pieces' colliders (%d rocks on them), the first %s within reach at 1 m, %d drawn from inside (shots/tests/unit_shard_room.png), %s", vShards, vClear, vRocks, vReach ? "is" : "is NOT", vDrawn, vGone ? "gone once taken" : "STILL offered once taken"));
                check("graves: the town's stones among its pieces, the first within reach at a metre, its name and years read (C-12)", tFound && gN >= 6 && gPieces == gN && gReach && !gLine.empty(),
                      fmt("%d graves in the town, %d stones among the pieces, the first %s within reach at 1 m: '%s' (shots/tests/unit_grave.png)", gN, gPieces, gReach ? "is" : "is NOT", gLine.c_str()));

            }
        }
    }
}

// C-08: ruins that read the world
void unitRuins(const Check& check) {
    {   // C-08: ruins that read the world. On the two test worlds (Aieliaalas II felisian, Leileashphail III desert) over 31 x 31 cells: a
        // harbour only where a shore is within reach of the edge, terraces only on a slope, a cistern and a wind wall only on the
        // desert world, cairns only in mountain relief, every terrace wall clear of the houses' rooms, the drifts on the desert's walls
        // alone, all of it the same twice; and the weathering: the decay rises with the age and the wet, the cultures near home span
        // from standing to foundations; the capsule's point inside a settlement is moved out past its edge
        struct TW { int64_t sx, sz; int bi; } tws[2] = {{151, 25, 1}, {153, 70, 2}};
        int harbours = 0, dryHarbours = 0, terraced = 0, cisterns = 0, windwalls = 0, cairnsN = 0, settlements = 0, wrong = 0, inRoom = 0, driftsDesert = 0, driftsFelisian = 0, notSame = 0, quaysWet = 0, quaysDry = 0, housesN = 0, doorsBlocked = 0;
        std::string blockedWhat;   // A-03: which rooms' doorways
        std::string wrongWhat, names;
        for (int w = 0; w < 2; w++) {
            Star s; if (!starInSector(tws[w].sx, 0, tws[w].sz, s, true)) continue;
            StarSystem sys; sys.generate(s);
            if (tws[w].bi >= (int)sys.bodies.size()) continue;
            const Body& b = sys.bodies[tws[w].bi]; BodyGen g = BodyGen::make(b); Culture cu = cultureOf(g);
            if (!g.hasTrait(TR_CIVILISATION)) continue;
            names += (names.empty() ? "" : ", ") + b.name;
            DrainageOff off;
            // a coast: two points of a 5-degree grid, one land and one water (or the old sea), the shoreline bisected between them at 2048 m,
            // the land side's cell taken
            double la0 = 0.2, lo0 = 0.7; planPlacePoint(g, 0, la0, lo0);   // T-01: round a town of the people's plan
            int gLat0, gLon0; ruinCellOf(g, la0, lo0, gLat0, gLon0);
            int cLat = gLat0, cLon = gLon0; bool coast = false;
            { double cla, clo; if (planPlacePoint(g, 0, cla, clo, true)) { ruinCellOf(g, cla, clo, cLat, cLon); coast = true; } }   // T-01: by the water where the plan put a place
            auto wetAt = [&](double la, double lo) { SurfaceSample q = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 2048); return q.material == MAT_WATER || q.oldSea > 0.5; };
            for (int j = 0; j < 36 && !coast; j++) for (int i = 0; i < 72 && !coast; i++) {
                double la = (-85 + 170.0 * (j + 0.5) / 36) * DEG, lo = (-180 + 360.0 * (i + 0.5) / 72) * DEG;
                if (std::fabs(la) > 40 * DEG) continue;
                bool w0 = wetAt(la, lo), w1 = wetAt(la, lo + 5 * DEG);   // (a world whose plan has no place by the water)
                if (w0 == w1) continue;
                double dryLo = w0 ? lo + 5 * DEG : lo, wetLo = w0 ? lo : lo + 5 * DEG;
                for (int it = 0; it < 12; it++) { double mid = 0.5 * (dryLo + wetLo); if (wetAt(la, mid)) wetLo = mid; else dryLo = mid; }
                ruinCellOf(g, la, dryLo, cLat, cLon); coast = true;
            }
            std::vector<Form> el;
            struct Reg { int lat, lon, half; } regs[2] = {{gLat0, gLon0, 15}, {cLat, cLon, 12}};
            for (int rg = 0; rg < (coast ? 2 : 1); rg++)
            for (int dl = -regs[rg].half; dl <= regs[rg].half; dl++) for (int dn = -regs[rg].half; dn <= regs[rg].half; dn++) {
                int gl = regs[rg].lat + dl, gn = regs[rg].lon + dn;
                RuinSpec sp; if (!ruinOfCell(g, gl, gn, sp, true) || !ruinSiteOk(g, sp)) continue;
                settlements++;
                RuinSpec sp2; ruinOfCell(g, gl, gn, sp2, true);
                if (featureList(sp2) != featureList(sp) || sp2.buildings.size() != sp.buildings.size()) notSame++;
                if (sp.harbour) { harbours++; if (sp.site.shoreDry) dryHarbours++; if (sp.site.shoreDist >= 500 + sp.size) { wrong++; wrongWhat += "a harbour without a shore; "; } }
                if (sp.terraced) { terraced++; if (sp.site.slope <= (cultureOf(g, sp.people).genome.of(GV_LAND) == LD_TERRACED ? 0.02 : 0.06)) { wrong++; wrongWhat += "terraces on the flat; "; } }   // A-04: a people of terraces terraces gentler slopes
                if (sp.cistern) { cisterns++; if (!sp.site.desert) { wrong++; wrongWhat += "a cistern off the desert; "; } }
                if (sp.windwall) { windwalls++; if (!sp.site.desert) { wrong++; wrongWhat += "a wind wall off the desert; "; } }
                if (sp.cairns) { cairnsN++; if (sp.site.relief <= 0.45) { wrong++; wrongWhat += "cairns on the plain; "; } }
                for (const Building& bd : sp.buildings) {
                    if (bd.kind != BK_TERRACE) continue;
                    for (const Building& h : sp.buildings) {
                        if (h.kind != BK_HOUSE && h.kind != BK_HALL && h.kind != BK_ROTUNDA) continue;
                        double dx = bd.x - h.x, dz = bd.z - h.z, lx = dx * std::cos(h.heading) - dz * std::sin(h.heading), lz = dx * std::sin(h.heading) + dz * std::cos(h.heading);
                        if (std::fabs(lx) < h.hw && std::fabs(lz) < h.hd) inRoom++;
                    }
                }
                ruinElements(sp, cu, 0, el);
                for (const Form& e : el) { if (e.part == 7) { if (g.type == PT_DESERT) driftsDesert++; else driftsFelisian++; } if (e.part == 6) { if (sp.site.shoreDry) quaysDry++; else quaysWet++; } }
                {   // every room's doorway open on both worlds (the features, the drifts, the town wall and its towers keep out of it): A-03, every
                    // building's room against the colliders of every piece of the settlement
                    std::vector<FormDisc> all; for (const Form& e : el) formColliders(e, all);
                    for (size_t k = 0; k < sp.buildings.size(); k++) {
                        const Building& bd = sp.buildings[k]; BuildingRoom R;
                        if (!buildingRoom(bd, R) || !R.door) continue;
                        housesN++;
                        bool clear; double run; roomOpenness(bd, R, all, clear, run);
                        if (run < 1.0) { doorsBlocked++; if (blockedWhat.size() < 160) blockedWhat += fmt("%s%s at %.0f %.0f", blockedWhat.empty() ? "" : ", ", BUILDING_KIND_NAMES[bd.kind], bd.x, bd.z); }
                    }
                }
            }
        }
        // the weathering
        bool mono = decayOf(200, 0.1) < decayOf(2000, 0.1) && decayOf(2000, 0.1) < decayOf(20000, 0.1) && decayOf(2000, 0.1) < decayOf(2000, 0.9) && decayOf(200, 0.0) < 0.3 && decayOf(30000, 0.9) > 0.8;
        double ageMin = 1e9, ageMax = 0, decMin = 1, decMax = 0; int cultures = 0;
        for (int64_t x = 150; x < 200 && cultures < 300; x++) for (int64_t z = 20; z < 70 && cultures < 300; z++) {
            Star s; if (!starInSector(x, 0, z, s, false)) continue;
            StarSystem sys; sys.generate(s);
            for (const Body& b : sys.bodies) { BodyGen g = BodyGen::make(b); if (!g.hasTrait(TR_CIVILISATION)) continue; Culture cu = cultureOf(g); cultures++; ageMin = std::min(ageMin, cu.ageYears); ageMax = std::max(ageMax, cu.ageYears); decMin = std::min(decMin, cu.decay); decMax = std::max(decMax, cu.decay); }
        }
        // the capsule's clearance: a point at the first sited settlement's centre moves out past its edge; a point far from any stays
        bool clearOk = false; double movedM = 0;
        { Star s; if (starInSector(tws[0].sx, 0, tws[0].sz, s, true)) { StarSystem sys; sys.generate(s); BodyGen g = BodyGen::make(sys.bodies[tws[0].bi]); double la0 = 0.2, lo0 = 0.7; planPlacePoint(g, 0, la0, lo0); int gLat0, gLon0; ruinCellOf(g, la0, lo0, gLat0, gLon0); DrainageOff off;
            for (int dl = -15; dl <= 15 && !clearOk; dl++) for (int dn = -15; dn <= 15 && !clearOk; dn++) {
                RuinSpec sp; if (!ruinOfCell(g, gLat0 + dl, gLon0 + dn, sp, false)) continue;
                double la = sp.lat, lo = sp.lon; bool moved = settlementClearance(g, la, lo);
                double mPerRad = g.R * 1000.0; movedM = std::hypot((la - sp.lat) * mPerRad, std::remainder(lo - sp.lon, TAU) * mPerRad * std::cos(sp.lat));
                double la2 = la, lo2 = lo; bool movedAgain = settlementClearance(g, la2, lo2);
                clearOk = moved && movedM >= sp.size + 25 && movedM <= sp.size + 40 && !movedAgain;
            } } }
        check("ruins read the world: harbours, terraces, cisterns, wind walls and cairns where the terrain says (C-08)",
              settlements > 0 && wrong == 0 && notSame == 0 && inRoom == 0 && harbours > 0 && terraced > 0 && cisterns > 0 && windwalls > 0 && driftsDesert > 0 && driftsFelisian == 0 && quaysWet + quaysDry > 0 && housesN > 0 && doorsBlocked == 0,
              fmt("%s: %d sited settlements: %d harbours (%d on the dry shore; %d quay pieces in the water, %d on the dry bed), %d terraced, %d cisterns, %d wind walls, %d with cairns; %d misplaced%s%s; %d terrace walls in a room; drifts: %d on the desert, %d on the felisian; %d of %d doorways blocked%s%s; %s",
                  names.c_str(), settlements, harbours, dryHarbours, quaysWet, quaysDry, terraced, cisterns, windwalls, cairnsN, wrong, wrong ? ": " : "", wrongWhat.c_str(), inRoom, driftsDesert, driftsFelisian, doorsBlocked, housesN, blockedWhat.empty() ? "" : ": ", blockedWhat.c_str(), notSame ? fmt("%d NOT the same twice", notSame).c_str() : "the same twice"));
        check("weathering by age: the decay with the years and the rain, from standing to foundations (C-08)", mono && cultures > 20 && ageMin < 600 && ageMax > 10000 && decMin < 0.35 && decMax > 0.75 && clearOk,
              fmt("decayOf 200/2000/20000 y dry %.2f/%.2f/%.2f, 2000 y wet %.2f; %d cultures near home: the end %.0f-%.0f years ago, decay %.2f-%.2f; the capsule's point moved %.0f m out of a settlement%s", decayOf(200, 0.1), decayOf(2000, 0.1), decayOf(20000, 0.1), decayOf(2000, 0.9), cultures, ageMin, ageMax, decMin, decMax, movedM, clearOk ? "" : " (NOT as asked)"));
    }
}

// C-10: the star charts, by age and on the decoder
void unitCharts(const Check& check) {
    {   // C-10: the star charts. On the two test worlds: three to five of the fifty are charts, none of them music, one mark each in the
        // lore; every chart's stars lie in the cube the game draws round the world's star, the brightest first, within the field of the
        // mark; the figures share no star, each joins its stars with lines within reach, and the caption names every figure and the
        // people marked; a mark of a people has a world and a name, one heard transmits; the same twice. Across forty civilised worlds
        // near home the charts mark the brightest star a quarter to three quarters of the time and a people the rest, the lore under 10 ms
        struct TW { int64_t sx, sz; int bi; } tws[2] = {{151, 25, 1}, {153, 70, 2}};
        int worlds = 0, charts = 0, musicToo = 0, bad = 0, figuresN = 0, starsN = 0; std::string badWhat, names; bool same = true;
        for (int w = 0; w < 2; w++) {
            Star s; if (!starInSector(tws[w].sx, 0, tws[w].sz, s, true)) continue;
            StarSystem sys; sys.generate(s); if (tws[w].bi >= (int)sys.bodies.size()) continue;
            const Body& b = sys.bodies[tws[w].bi]; BodyGen g = BodyGen::make(b); if (!g.hasTrait(TR_CIVILISATION)) continue;
            worlds++; names += (names.empty() ? "" : ", ") + b.name;
            Lore L = loreOf(sys, b, g), L2 = loreOf(sys, b, g);
            int count = chartShardsOf(g);
            if (count < 3 || count > 5) { bad++; badWhat += fmt("%d charts; ", count); }
            const int allCharts = recordChartCount(g, L.records);   // T-04: the charts past the fifty marked too, C-10's first
            if ((int)L.marks.size() != allCharts) { bad++; badWhat += fmt("%zu marks for %d charts; ", L.marks.size(), allCharts); }
            if (L2.marks.size() != L.marks.size()) same = false; else for (size_t j = 0; j < L.marks.size(); j++) if (L2.marks[j].star.seed != L.marks[j].star.seed || L2.marks[j].how != L.marks[j].how) same = false;
            for (const ChartMark& m : L.marks) {
                if (m.how > 0 && (m.body < 0 || m.people.empty())) { bad++; badWhat += "a people's mark without a world or a name; "; }
                if (m.how == 0 && (m.body >= 0 || !m.people.empty())) { bad++; badWhat += "a bright star's mark with a people; "; }
                if (m.how == 2 && !sectorTransmits(m.star.sx, m.star.sy, m.star.sz)) { bad++; badWhat += "a people heard that does not transmit; "; }   // A-01's check adds the radio of both
                if (m.star.seed == s.seed) { bad++; badWhat += "the world's own star marked; "; }
            }
            std::vector<Shard> fifty; shardsOf(sys, b, g, L, SHARDS_PER_WORLD, fifty);
            int found = 0;
            for (int i = 0; i < SHARDS_PER_WORLD; i++) {
                if (!fifty[i].chart) continue;
                found++; charts++;
                if (fifty[i].music) musicToo++;
                StarChart ch, ch2; bool ok1 = chartOf(sys, b, g, L, i, ch), ok2 = chartOf(sys, b, g, L, i, ch2);
                if (!ok1 || !ok2) { bad++; badWhat += fmt("S%d has no chart; ", i); continue; }
                if (ch.stars.size() != ch2.stars.size() || ch.figures.size() != ch2.figures.size() || ch.mark.star.seed != ch2.mark.star.seed) same = false;
                else for (size_t f = 0; f < ch.figures.size(); f++) if (ch.figures[f].stars != ch2.figures[f].stars || ch.figures[f].name != ch2.figures[f].name) same = false;
                if (ch.stars.size() < 10 || (int)ch.stars.size() > CHART_BRIGHT + 1) { bad++; badWhat += fmt("S%d has %zu stars; ", i, ch.stars.size()); }
                if (ch.markStar < 0 || ch.markStar >= (int)ch.stars.size() || ch.stars[ch.markStar].star.seed != ch.mark.star.seed || !ch.stars[ch.markStar].mark) { bad++; badWhat += fmt("S%d's mark is not on it; ", i); }
                for (size_t k = 0; k < ch.stars.size(); k++) {
                    const ChartStar& c = ch.stars[k];
                    if (std::llabs(c.star.sx - s.sx) > CHART_RADIUS_SECTORS || std::llabs(c.star.sy - s.sy) > CHART_RADIUS_SECTORS || std::llabs(c.star.sz - s.sz) > CHART_RADIUS_SECTORS) { if ((int)k != ch.markStar) { bad++; badWhat += fmt("S%d has a star outside the cube; ", i); } }
                    if (c.ang > CHART_FIELD + 1e-9 || std::hypot(c.x, c.y) > 1 + 1e-9) { bad++; badWhat += fmt("S%d has a star outside the field; ", i); }
                    if (k > 0 && (int)k != ch.markStar && (int)(k - 1) != ch.markStar && c.bright > ch.stars[k - 1].bright + 1e-12) { bad++; badWhat += fmt("S%d's stars are not by brightness; ", i); }
                }
                std::vector<char> taken(ch.stars.size(), 0);
                for (const ChartFigure& f : ch.figures) {
                    figuresN++; starsN += (int)f.stars.size();
                    if (f.stars.size() < 2 || f.stars.size() > 6 || f.lines.size() != f.stars.size() - 1) { bad++; badWhat += fmt("S%d's %s has %zu stars and %zu lines; ", i, f.name.c_str(), f.stars.size(), f.lines.size()); }
                    for (int k : f.stars) { if (taken[k]) { bad++; badWhat += fmt("S%d's %s shares a star; ", i, f.name.c_str()); } taken[k] = 1; }
                    for (const auto& ln : f.lines) if (std::acos(clampd(dot(ch.stars[ln.first].dir, ch.stars[ln.second].dir), -1, 1)) > CHART_LINK + 1e-9) { bad++; badWhat += fmt("S%d's %s reaches too far; ", i, f.name.c_str()); }
                    if (fifty[i].text.find(f.name) == std::string::npos) { bad++; badWhat += fmt("S%d's caption lacks %s; ", i, f.name.c_str()); }
                }
                if (ch.figures.empty()) { bad++; badWhat += fmt("S%d has no whole figure in its field; ", i); }
                if (!ch.mark.people.empty() && (fifty[i].text.find(ch.mark.people) == std::string::npos || fifty[i].child != ch.mark.people)) { bad++; badWhat += fmt("S%d's caption does not name the %s; ", i, ch.mark.people.c_str()); }
                if (ch.mark.how == 0 && ch.which == 0 && fifty[i].text.find(L.star) == std::string::npos) { /* the first bright star is the lore's; a later chart gets a phrase */ }
                int nw = shardWords(fifty[i].text); if (nw < 8 || nw > 70) { bad++; badWhat += fmt("S%d's caption has %d words; ", i, nw); }
            }
            if (found != count) { bad++; badWhat += fmt("%d charts among the fifty for %d asked; ", found, count); }
        }
        int far = 0, farCharts = 0, how[4] = {0, 0, 0, 0}; double loreMs = 0, loreMax = 0;
        for (int64_t x = 150; x < 200 && far < 40; x++) for (int64_t z = 20; z < 70 && far < 40; z++) {
            Star s; if (!starInSector(x, 0, z, s, false)) continue;
            StarSystem sys; sys.generate(s);
            for (const Body& b : sys.bodies) {
                BodyGen g = BodyGen::make(b); if (!g.hasTrait(TR_CIVILISATION)) continue;
                far++; double t0 = nowSec(); Lore L = loreOf(sys, b, g); double ms = (nowSec() - t0) * 1e3; loreMs += ms; loreMax = std::max(loreMax, ms);
                for (const ChartMark& m : L.marks) { if (m.how == 3) { how[3]++; continue; } farCharts++; how[std::min(std::max(m.how, 0), 2)]++; }   // H-05: the far future's far marks apart
                break;
            }
        }
        double brightShare = farCharts ? (double)how[0] / farCharts : 0;
        check("star charts: a few of the fifty, the people's sky round a star they marked, figures named in the caption (C-10)",
              worlds == 2 && bad == 0 && same && musicToo == 0 && charts >= 6 && farCharts > 0 && brightShare >= 0.25 && brightShare <= 0.75 && how[1] + how[2] > 0 && loreMs / std::max(far, 1) < 10,
              fmt("%s: %d charts, %d figures of %.1f stars, %d of them music too, %d wrong%s%s, %s; %d worlds near home: %d charts, %d mark the brightest star (%.0f%%), %d a near people, %d a people heard; the lore %.1f ms a world (%.1f at most)",
                  names.c_str(), charts, figuresN, figuresN ? (double)starsN / figuresN : 0.0, musicToo, bad, bad ? ": " : "", badWhat.c_str(), same ? "the same twice" : "NOT the same twice", far, farCharts, how[0], 100 * brightShare, how[1], how[2], loreMs / std::max(far, 1), loreMax));
    }
    { std::string d; bool ok = checkChartsByAge(d); check("star charts by age: each in the way of its record's age and medium, with what that way shows (H-05)", ok, d); }
    {   // C-10: the chart on the decoder: with three shards and the first chart of Aieliaalas II in the guide, the chart opened by index
        // reads over four seconds (Enter skips), goes into the guide and the log, Enter targets the marked star, O holds it up, and in space
        // the figures' lines are drawn on the view turned onto a figure; parked at the people's own star every star of the chart is in the
        // sky the game draws; O on the chart again puts it down
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFrames(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        int ci = -1; Star ws; StarSystem wsys; bool haveWorld = starInSector(151, 0, 25, ws, true);
        if (haveWorld) { wsys.generate(ws); if (wsys.bodies.size() > 1) { BodyGen g = BodyGen::make(wsys.bodies[1]); for (int i = 0; i < SHARDS_PER_WORLD && ci < 0; i++) if (shardIsChart(g, i)) ci = i; } }
        for (int i = 0; i < 3; i++) game.guide.shards.insert("151,0,25/1/S" + std::to_string(i));
        if (ci >= 0) game.guide.shards.insert("151,0,25/1/S" + std::to_string(ci));
        game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1);
        bool atDecoder = game.state == GameState::SHARDS && game.testShardsLevel() == 0;
        press(KEY_ENTER); run(0.1);
        bool opened = ci >= 0 && game.testShardsOpenIndex(ci); run(1.0);
        std::string reading = game.testChartInfo(); bool isReading = reading.find("reading") != std::string::npos;
        press(KEY_ENTER); run(0.2);   // Enter skips the wait: read
        std::string key = "151,0,25/1/S" + std::to_string(ci);
        bool read = game.guide.decoded.count(key) && game.guide.decoded[key] == 4;
        bool logged = false; for (const LogEntry& e : game.guide.log) if (e.kind == "CHART") logged = true;
        press(KEY_ENTER); run(0.1);   // Enter again targets the marked star
        int64_t rx = 0, ry = 0, rz = 0; bool remote = game.testRemote(rx, ry, rz);
        std::string st = game.testStatus();
        press(KEY_O); run(0.1); bool up = game.testChartUp();
        press(KEY_ESCAPE); press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
        bool inSpace = game.state == GameState::SPACE && game.testChartUp();
        bool aimed = game.testAimAtChartMark(0); run(0.2);
        int lines = game.testChartLines();
        bool aimedMark = game.testAimAtChartMark(-1); run(0.2);
        std::string held = game.testChartInfo();
        // at the people's own star every star of the chart is in the sky
        int inSky = -1, chartStars = -1;
        if (haveWorld && wsys.bodies.size() > 1) { game.testParkAt(ws, 1); run(0.3); inSky = game.testChartStarsInSky(); size_t p = held.find("held up: "); chartStars = p == std::string::npos ? -1 : atoi(held.c_str() + held.find("(", p) + 1); }
        bool markOk = false; { int64_t mx = 0, my = 0, mz = 0; Star dummy; (void)dummy; if (remote) { markOk = true; /* the remote is the mark: checked against the lore below */ } (void)mx; (void)my; (void)mz; }
        bool remoteIsMark = false;
        if (haveWorld && wsys.bodies.size() > 1 && remote) { const Body& b = wsys.bodies[1]; BodyGen g = BodyGen::make(b); Lore L = loreOf(wsys, b, g); int which = chartIndexOf(g, ci); if (which >= 0 && which < (int)L.marks.size()) remoteIsMark = L.marks[which].star.sx == rx && L.marks[which].star.sy == ry && L.marks[which].star.sz == rz; }
        // put down again from the decoder
        game.setState(GameState::SPACE); game.testCabinGoto(0.0, -0.8, PI, -0.65); run(0.2); press(KEY_E); run(0.1); press(KEY_ENTER); run(0.1);
        bool reopened = game.testShardsOpenIndex(ci); run(0.1); press(KEY_O); run(0.1);
        bool down = !game.testChartUp();
        press(KEY_ESCAPE); press(KEY_ESCAPE); press(KEY_ESCAPE); run(0.1);
        check("star charts on the decoder: read, targeted, held up to the sky and put down (C-10)",
              ci >= 0 && atDecoder && opened && isReading && read && logged && remote && remoteIsMark && up && inSpace && aimed && lines > 0 && aimedMark && inSky >= 0 && inSky == chartStars && reopened && down,
              fmt("chart S%d: %s; %s a second in; read %s (decoded at %d), logged %s; Enter: '%s' (the remote %s the mark); held up %s, in space %s, %d lines on the view turned onto its first figure; at the people's star %d of %d stars in the sky; put down %s",
                  ci, atDecoder ? (opened ? "opened" : "NOT opened") : "the decoder NOT reached", isReading ? "reading" : "NOT reading", read ? "yes" : "NO", game.guide.decoded.count(key) ? game.guide.decoded[key] : 0, logged ? "yes" : "NO", trunc(st, 40).c_str(), remoteIsMark ? "is" : "is NOT",
                  up ? "yes" : "NO", inSpace ? "yes" : "NO", lines, inSky, chartStars, down ? "yes" : "NO"));
        (void)markOk;
        remove("shots/tests/test_guide.txt");
    }
}

// C-09: the roads, on the ground and on the ship
void unitRoads(const Check& check) {
    {   // C-09: the roads. On the two test worlds (Leileashphail III round its default region, Aieliaalas II round its land region at 12 N
        // 4.22 E) over 13 x 13 cells: every sited settlement has a road, every road joins two sited settlements from edge to edge, the
        // same road from either end (its id and its points) and the same twice, no point of it in the sea, a lake or the old sea, no
        // way much longer than the straight line, no step steeper than a half; the young desert people's roads are nearly whole and the
        // old wet world's a trace; a road walks in well under a millisecond
        struct TW { int64_t sx, sz; int bi; double latDeg, lonDeg; } tws[2] = {{153, 70, 2, 11.46, 40.11}, {151, 25, 1, 12.0, 4.22}};
        int nodesN = 0, roadsN = 0, lonely = 0, badEnds = 0, wetPts = 0, longWays = 0, steep = 0, asym = 0, notSame = 0, pts = 0; double leftYoung = -1, leftOld = -1, msRoad = 0, maxGrade = 0, lenSum = 0; std::string names;
        for (int w = 0; w < 2; w++) {
            Star s; if (!starInSector(tws[w].sx, 0, tws[w].sz, s, true)) continue;
            StarSystem sys; sys.generate(s);
            if (tws[w].bi >= (int)sys.bodies.size()) continue;
            const Body& b = sys.bodies[tws[w].bi]; BodyGen g = BodyGen::make(b);
            if (!g.hasTrait(TR_CIVILISATION)) continue;
            Culture cu = cultureOf(g);
            names += (names.empty() ? "" : ", ") + b.name;
            double la0 = tws[w].latDeg * DEG, lo0 = tws[w].lonDeg * DEG; planPlacePoint(g, 0, la0, lo0);   // T-01: round a town of the people's plan
            int gLat0, gLon0; ruinCellOf(g, la0, lo0, gLat0, gLon0);
            RoadNodeCache nc; nc.reset(&g);
            std::vector<RoadNode> nodes; std::vector<RoadPair> pairs;
            for (int dl = -6; dl <= 6; dl++) for (int dn = -6; dn <= 6; dn++) { RoadNode n; if (nc.node(gLat0 + dl, gLon0 + dn, n)) nodes.push_back(n); nc.pairsOf(gLat0 + dl, gLon0 + dn, pairs); }
            std::sort(pairs.begin(), pairs.end(), [](const RoadPair& x, const RoadPair& y) { return x.id < y.id; });
            pairs.erase(std::unique(pairs.begin(), pairs.end(), [](const RoadPair& x, const RoadPair& y) { return x.id == y.id; }), pairs.end());
            double t0 = nowSec();
            std::vector<Road> roads;
            for (const RoadPair& p : pairs) { Road r; if (roadWay(g, cu, p.a, p.b, r)) roads.push_back(r); }
            msRoad = std::max(msRoad, roads.empty() ? 0.0 : (nowSec() - t0) * 1000 / roads.size());
            std::unordered_map<uint64_t, int> deg;
            double leftSum = 0; int leftN = 0;
            DrainageOff off;
            for (const Road& r : roads) {
                deg[roadCellKey(g, r.a.gLat, r.a.gLon)]++; deg[roadCellKey(g, r.b.gLat, r.b.gLon)]++;
                double endA = roadDistanceM(g, r.pts.front(), r.a.unit), endB = roadDistanceM(g, r.pts.back(), r.b.unit);
                if (endA > 1.1 * r.a.size + 6 || endB > 1.1 * r.b.size + 6 || endA < 0.5 * r.a.size || endB < 0.5 * r.b.size) badEnds++;
                for (const Vec3& p : r.pts) { SurfaceSample q = sampleSurface(g, p, 32.0); if (q.material == MAT_WATER || (q.water > -1e8 && q.height < q.water) || q.oldSea > 0.5) wetPts++; pts++; }
                if (r.lengthM > 1.6 * r.straightM + 100) longWays++;
                if (r.maxGrade > 0.5) steep++;
                maxGrade = std::max(maxGrade, r.maxGrade); lenSum += r.lengthM;
                for (double a = 0; a < r.lengthM; a += 10) { leftSum += roadLeft(r.id, a, cu.roadWear); leftN++; }
            }
            for (const RoadNode& n : nodes) {   // T-01: a settlement with another within the roads' reach (the countryside is the plan's: a lone hamlet has none)
                if (deg.count(roadCellKey(g, n.gLat, n.gLon))) continue;
                bool neighbour = false;
                for (const RoadNode& m : nodes) if ((m.gLat != n.gLat || m.gLon != n.gLon) && std::abs(m.gLat - n.gLat) <= ROAD_REACH && roadDistanceM(g, m.unit, n.unit) <= ROAD_MAX_M) neighbour = true;
                if (neighbour) lonely++;
            }
            // the same road from either end, and the same twice
            for (size_t k = 0; k < roads.size() && k < 6; k++) {
                const Road& r = roads[k * 7 % roads.size()];
                std::vector<Road> fromA, fromB, fromA2; roadsOfCell(g, r.a.gLat, r.a.gLon, fromA); roadsOfCell(g, r.b.gLat, r.b.gLon, fromB); roadsOfCell(g, r.a.gLat, r.a.gLon, fromA2);
                const Road* inA = nullptr; const Road* inB = nullptr; const Road* inA2 = nullptr;
                for (const Road& x : fromA) if (x.id == r.id) inA = &x;
                for (const Road& x : fromB) if (x.id == r.id) inB = &x;
                for (const Road& x : fromA2) if (x.id == r.id) inA2 = &x;
                if (!inA || !inB || inA->pts.size() != inB->pts.size() || inA->pts.size() != r.pts.size()) asym++;
                else if (!inA2 || inA2->pts.size() != inA->pts.size() || length(inA2->pts[inA2->pts.size() / 2] - inA->pts[inA->pts.size() / 2]) > 1e-12) notSame++;
            }
            nodesN += (int)nodes.size(); roadsN += (int)roads.size();
            (w == 0 ? leftYoung : leftOld) = leftN ? leftSum / leftN : -1;
        }
        check("roads: a network between the sited settlements, from edge to edge over dry ground, the same from either end (C-09)",
              nodesN > 20 && roadsN > 20 && lonely == 0 && badEnds == 0 && wetPts == 0 && longWays == 0 && steep * 20 <= roadsN && asym == 0 && notSame == 0 && leftYoung > 0.8 && leftOld > 0.3 && leftOld < 0.75 && msRoad < 3,
              fmt("%s: %d sited settlements, %d roads of %.1f km together (%d points); %d settlements without a road (another in reach), %d roads not from edge to edge, %d points in water, %d ways over 1.6 x the straight line, %d steeper than a half (the steepest %.3f: no switchbacks, KI-352), %d not the same from either end, %d not the same twice; left after the wear: %.0f%% on the young desert world, %.0f%% on the old wet one; %.2f ms a road at most",
                  names.c_str(), nodesN, roadsN, lenSum / 1000, pts, lonely, badEnds, wetPts, longWays, steep, maxGrade, asym, notSame, 100 * leftYoung, 100 * leftOld, msRoad));
    }
    {   // C-09: the roads on the ground and on the ship. The view at a point on a road of Leileashphail III: the site holds roads, the
        // vertex under the point carries the road's share, the paving as its material and no vegetation on the fine rings, the tone alone
        // on the 64 m ring, and a point forty metres across the road carries nothing; the landing map's zoom over the region draws the
        // network's lines; landed by the road, the explorer set on it is told once (the status, a ROAD log line) and the HUD names the way
        Star s; StarSystem sys; bool have = starInSector(153, 0, 70, s, true);
        bool roadsHeld = false, onBed = false, paved = false, bare = false, lod0 = false, lod1Tone = false, offRoad = false, zoomLines = false, told = false, logged = false, hudWay = false;
        int zoomN = 0, siteRoads = 0; double siteMs = 0, latDeg = 0, lonDeg = 0; std::string info, statusLine;
        bool gWalked = false, gTold = false, gLogged = false, gKept = false; std::string gInfo;   // C-12
        if (have) {
            sys.generate(s);
            const Body& b = sys.bodies[2]; BodyGen g = BodyGen::make(b);
            double la0 = 11.46 * DEG, lo0 = 40.11 * DEG; planPlacePoint(g, 0, la0, lo0);   // T-01: round a town of the people's plan
            int gLat0, gLon0; ruinCellOf(g, la0, lo0, gLat0, gLon0);
            std::vector<Road> roads;
            for (int dl = -3; dl <= 3 && roads.empty(); dl++) for (int dn = -3; dn <= 3 && roads.empty(); dn++) { std::vector<Road> rs; roadsOfCell(g, gLat0 + dl, gLon0 + dn, rs); for (const Road& r : rs) if (r.lengthM > 1500) { roads.push_back(r); break; } }
            if (!roads.empty()) {
                const Road& r = roads[0];
                const Vec3& mid = r.pts[r.pts.size() / 2];
                double la, lo; StarSystem::latLonFromBody(mid, la, lo); latDeg = la / DEG; lonDeg = lo / DEG;
                SurfaceView sv; sv.init(&sys, 2, la, lo, 5000.0);
                SurfaceSite probe; probe.init(&sys, 2, la, lo, 5000.0);   // no roads: the ground as it is
                siteRoads = (int)sv.site.roads.size(); siteMs = sv.site.roadsMs; roadsHeld = siteRoads > 0;
                double d, along, hd; const SiteRoad* rd;
                if (sv.site.roadAt(0, 0, 12, d, along, hd, rd)) {
                    // along the road to a point the wear left, within 600 m
                    double ax = 0, az = 0; bool found = false;
                    for (double t = 0; t <= 600 && !found; t += 10) {
                        double px = std::sin(hd) * t, pz = std::cos(hd) * t, d2, al2, h2; const SiteRoad* r2;
                        if (sv.site.roadAt(px, pz, 12, d2, al2, h2, r2) && d2 < 1.0 && roadLeft(r2->id, al2, r2->wear) > 0.9) { ax = px; az = pz; found = true; }
                    }
                    if (found) {
                        TerrainVertex vN = sv.site.sampleAt(ax, az, 4), v0 = sv.site.sampleAt(ax, az, 16), v1 = sv.site.sampleAt(ax, az, 64), p1 = probe.sampleAt(ax, az, 64), pN = probe.sampleAt(ax, az, 4);
                        onBed = vN.road > 100;
                        paved = (int)vN.material == familyRep(g.type, FAM_ROCK) || !sv.site.roadCultures[0].paved;
                        bare = vN.veg < 0.25f * std::max(0.05f, pN.veg) + 0.01f;
                        lod0 = v0.road > 0 && ((int)v0.material == (int)vN.material);
                        lod1Tone = v1.road > 0 && v1.material == p1.material && v1.albedo < p1.albedo;
                        double cx = ax + std::cos(hd) * 40, cz = az - std::sin(hd) * 40;
                        TerrainVertex vOff = sv.site.sampleAt(cx, cz, 4), pOff = probe.sampleAt(cx, cz, 4);
                        offRoad = vOff.road == 0 && vOff.material == pOff.material;
                    }
                }
                // the ship: the zoom over the region, then the landing by the road
                TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
                game.newGame(); game.setState(GameState::SPACE);
                Input gi;
                auto run = [&](double secs) { runFrames(game, gi, secs); };
                auto press = [&](int key) { pressKey(game, gi, key); };
                game.testParkAt(s, 2); run(0.2);
                game.testLandCursor(latDeg, lonDeg); run(0.1);
                game.testBuildZoom(); zoomN = game.testZoomRoads(); zoomLines = zoomN > 0;
                press(KEY_ENTER);   // the landing
                for (int i = 0; i < 30 * 20 && !(game.state == GameState::SURFACE && game.testPlayerAlt() < 3); i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); }
                if (game.state == GameState::SURFACE && game.testWalkToRoad(400)) {
                    run(0.5);
                    statusLine = game.testStatus();
                    told = statusLine.find("OLD ROAD") != std::string::npos;
                    for (const LogEntry& e : game.guide.log) if (e.kind == "ROAD") logged = true;
                    info = game.testRoadInfo();
                    hudWay = info.find("roads met 1") != std::string::npos;
                    if ((gWalked = game.testWalkToGrave())) {   // C-12: a metre before the nearest settlement's first stone: the status, a GRAVE log line, the guide keeps it, the second visit says nothing
                        run(0.5); gInfo = game.testGraveInfo();
                        gTold = game.testStatus().find("A GRAVE:") != std::string::npos;
                        writePNG("shots/tests/unit_grave_hud.png", game.output(), FBW, FBH);   // the HUD's grave line and the status
                        for (const LogEntry& e : game.guide.log) if (e.kind == "GRAVE") gLogged = true;
                        size_t before = game.guide.log.size(); run(0.5);
                        gKept = game.guide.graves.size() == 1 && game.guide.log.size() == before;
                    }
                }
            }
        }
        check("graves on the ground: the explorer before a stone is told once, the guide keeps it (C-12)", have && gWalked && gTold && gLogged && gKept,
              fmt("%s; told %s, logged %s, kept once %s", gWalked ? gInfo.c_str() : "NO grave reached", gTold ? "yes" : "NO", gLogged ? "yes" : "NO", gKept ? "yes" : "NO"));
        check("roads on the ground and on the ship: the bed paved and bare, the tone from the air, the zoom's lines, the explorer told (C-09)",
              have && roadsHeld && onBed && paved && bare && lod0 && lod1Tone && offRoad && zoomLines && told && logged && hudWay,
              fmt("Leileashphail III by the road at %.2f %.2f: %d roads at the site (%.1f ms); on the bed %s, paved %s, bare %s, the 16 m vertex %s, the 64 m vertex the tone alone %s, 40 m across nothing %s; the zoom %d roads; the explorer told %s ('%s'), logged %s; %s",
                  latDeg, lonDeg, siteRoads, siteMs, onBed ? "yes" : "NO", paved ? "yes" : "NO", bare ? "yes" : "NO", lod0 ? "yes" : "NO", lod1Tone ? "yes" : "NO", offRoad ? "yes" : "NO", zoomN, told ? "yes" : "NO", statusLine.c_str(), logged ? "yes" : "NO", info.c_str()));
    }
}

// C-12: the calendar, the last recording and the graves
void unitGraves(const Check& check) {
    {   // C-12: the calendar, the last recording and the graves. On the two test worlds the last recording is a text shard (no piece, no
        // chart), the latest of the fifty (the span's end) and the only one with the end's tone, its date in the calendar like every
        // shard's (a moon within the moons, a day within the month or the year); the quick lore agrees with the full one. The graves of the
        // sited settlements over 13 x 13 cells of each world's region: no more than the class allows, every stone outside the radius and
        // clear of every building, the names distinct within a settlement and nine letters at most, the years in the span, the ids
        // distinct over both worlds, the same twice, a settlement's ground in well under a millisecond
        struct TW { int64_t sx, sz; int bi; double latDeg, lonDeg; } tws[2] = {{153, 70, 2, 11.46, 40.11}, {151, 25, 1, 12.0, 4.22}};
        int worlds = 0, lastOk = 0, endsOk = 0, latestOk = 0, dateBad = 0, settleN = 0, graveN = 0, byClass[4] = {0, 0, 0, 0}, countBad = 0, outsideBad = 0, clearBad = 0, dupNames = 0, yearBad = 0, fallenN = 0, emptyN = 0; bool determ = true, idsDistinct = true; double msGraves = 0;
        std::set<uint64_t> ids; std::string calLines, lastLines;
        for (int w = 0; w < 2; w++) {
            Star s; if (!starInSector(tws[w].sx, 0, tws[w].sz, s, true)) continue;
            StarSystem sys; sys.generate(s);
            if (tws[w].bi >= (int)sys.bodies.size()) continue;
            const Body& b = sys.bodies[tws[w].bi]; BodyGen g = BodyGen::make(b);
            if (!g.hasTrait(TR_CIVILISATION)) continue;
            worlds++;
            Lore L = loreOf(sys, b, g), Q = loreQuick(sys, b, g);
            if (Q.last != L.last || Q.spanYears != L.spanYears || Q.calendar != L.calendar || Q.months != L.months || Q.monthDays != L.monthDays || Q.city != L.city || Q.style != L.style) determ = false;
            std::vector<Shard> sh; shardsOf(sys, b, g, L, SHARDS_PER_WORLD, sh);
            int ends = 0, latest = -1, maxYear = -1;
            for (int i = 0; i < SHARDS_PER_WORLD; i++) {
                const Shard& x = sh[i];
                if (x.tone == ST_END) ends++;
                if (x.year > maxYear) { maxYear = x.year; latest = i; }
                bool mBad = L.months > 0 ? (x.month < 1 || x.month > L.months) : x.month != 0;
                int days = L.calendar == 1 ? L.monthDays : (L.calendar == 0 ? (int)std::lround(L.yearDays) : 0);
                bool dBad = days > 0 ? (x.day < 1 || x.day > days) : x.day != 0;
                if (mBad || dBad || shardDate(L, x).empty()) dateBad++;
            }
            bool lastGood = L.last >= 0 && L.last < SHARDS_PER_WORLD && sh[L.last].last && !sh[L.last].music && !sh[L.last].chart && sh[L.last].tone == ST_END && sh[L.last].year == L.spanYears + 1;
            if (lastGood) lastOk++;
            if (ends == 1) endsOk++;
            if (latest == L.last) latestOk++;
            calLines += fmt("%s%s: %s; dates like '%s'", w ? "; " : "", b.name.c_str(), calendarLine(L).c_str(), shardDate(L, sh[0]).c_str());
            lastLines += fmt("%s%s: S%d (%d ends), year %d of %d: '%s'", w ? "; " : "", b.name.c_str(), L.last, ends, L.last >= 0 ? sh[L.last].year : -1, L.spanYears + 1, L.last >= 0 ? trunc(sh[L.last].text, 50).c_str() : "-");
            Culture c = cultureOf(g);
            double la0 = tws[w].latDeg * DEG, lo0 = tws[w].lonDeg * DEG; planPlacePoint(g, 0, la0, lo0);   // T-01: round a town of the people's plan
            int gLat0, gLon0; ruinCellOf(g, la0, lo0, gLat0, gLon0);
            for (int dl = -6; dl <= 6; dl++) for (int dn = -6; dn <= 6; dn++) {
                RuinSpec sp;
                if (!ruinOfCell(g, gLat0 + dl, gLon0 + dn, sp, true)) continue;
                { DrainageOff off; if (!ruinSiteOk(g, sp)) continue; }
                std::vector<Grave> gr, gr2;
                double t0 = nowSec(); gravesOf(L, sp, c, gr); msGraves += 1000 * (nowSec() - t0); gravesOf(L, sp, c, gr2);
                if (gr.size() != gr2.size()) determ = false; else for (size_t i = 0; i < gr.size(); i++) if (gr[i].name != gr2[i].name || gr[i].x != gr2[i].x || gr[i].id != gr2[i].id) determ = false;
                settleN++; graveN += (int)gr.size(); byClass[std::min(sp.sclass, 3)] += (int)gr.size();
                int hi = sp.sclass == SC_HAMLET ? 3 : (sp.sclass == SC_VILLAGE ? 6 : (sp.sclass == SC_TOWN ? 12 : (sp.sclass == SC_CITY ? 18 : (sp.sclass == SC_CAPITAL ? 24 : 1))));
                if ((int)gr.size() > hi) countBad++;
                if (gr.empty() && (sp.sclass == SC_VILLAGE || townOrGreater(sp.sclass))) emptyN++;
                std::set<std::string> names;
                for (const Grave& x : gr) {
                    if (std::sqrt(x.x * x.x + x.z * x.z) < sp.size * 1.05) outsideBad++;
                    if (!settlementClear(sp, x.x, x.z, 0.9)) clearBad++;
                    if (!names.insert(x.name).second) dupNames++;
                    if (x.died < 1 || x.died > L.spanYears || (x.born > 0 && x.born >= x.died) || x.name.empty() || x.name.size() > 9) yearBad++;
                    if (!ids.insert(x.id).second) idsDistinct = false;
                    if (x.fallen) fallenN++;
                }
            }
        }
        check("the last recording: one a world, a text of the latest year with the end's tone; every date within the calendar (C-12)", worlds == 2 && lastOk == 2 && endsOk == 2 && latestOk == 2 && dateBad == 0 && determ,
              fmt("%d worlds; the last %s, the only end on %d, the latest on %d; %d dates out of the calendar; the quick lore %s; %s; %s", worlds, lastOk == 2 ? "a text of the span's end on both" : fmt("right on %d", lastOk).c_str(), endsOk, latestOk, dateBad, determ ? "agrees" : "DISAGREES", calLines.c_str(), lastLines.c_str()));
        check("graves: a few a settlement outside its edge, clear of the buildings, named and dated in the span, the same twice (C-12)", worlds == 2 && settleN >= 20 && graveN >= 50 && countBad == 0 && emptyN == 0 && outsideBad == 0 && clearBad == 0 && dupNames == 0 && yearBad == 0 && idsDistinct && determ && msGraves / std::max(1, settleN) < 1.0,
              fmt("%d settlements with %d graves (hamlets %d, villages %d, towns %d, monuments %d), %d fallen; %d over the class's count, %d villages or towns without, %d inside the edge, %d in a building, %d names repeated, %d out of the span or too long, ids %s, %s; %.0f us a settlement",
                  settleN, graveN, byClass[0], byClass[1], byClass[2], byClass[3], fallenN, countBad, emptyN, outsideBad, clearBad, dupNames, yearBad, idsDistinct ? "distinct" : "REPEATED", determ ? "the same twice" : "NOT the same twice", 1000 * msGraves / std::max(1, settleN)));
    }
}
