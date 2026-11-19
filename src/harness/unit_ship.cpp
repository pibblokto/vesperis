// I-02: the unit's ship (group `ship`): the plan's rooms reachable, every device in a room and reachable from the start and faced from
// its stand, no collider in an arch's way, the stair's floor continuous round it, the lift, the banks dealt, a save of the 5 m room read as
// the bridge's start; I-03: every view's banks whole, the lamps' pools and the sun's patches; I-04: the rooms' sounds, the garden, the
// explorer's things
#include "harness/harness.h"

void unitShip(const Check& check) {
    {   // every room reachable from the bridge and back, a way at a time (the arches and the stair's landings)
        int reach = 0, back = 0;
        std::string missing;
        for (int r = 0; r < SHIP_ROOMS; r++) {
            if (r == SHIP_START.room) continue;
            int at = SHIP_START.room, hops = 0;
            while (at != r && hops < SHIP_ROOMS) {
                const int w = shipNextWay(at, r);
                if (w < 0) break;
                const ShipOpening& o = shipOpenings()[w];
                at = o.from == at ? o.to : o.from; hops++;
            }
            if (at == r) reach++; else missing += std::string(" ") + shipRooms()[r].name;
            if (shipNextWay(r, SHIP_START.room) >= 0) back++;
        }
        check("ship: every room reachable from the bridge", reach == SHIP_ROOMS - 1 && back == SHIP_ROOMS - 1,
              fmt("%d of %d rooms reached a way at a time, %d with a way back%s", reach, SHIP_ROOMS - 1, back, missing.empty() ? "" : (", not:" + missing).c_str()));
    }
    {   // every device in a room of the plan, its stand on the walk's floors, reached from the start, faced from its stand
        TestGame game;
        game.savePrefix = "shots/tests/unit_ship_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_ship_guide.txt";
        game.keysPath = "shots/tests/test_keys.txt";
        remove(game.guidePath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input in;
        int devices = 0, placed = 0, faced = 0;
        std::string bad;
        for (const CabinDevice& d : Game::cabinDevices()) {
            if (d.facing == FACE_NOTHING) continue;
            devices++;
            const bool inRoom = d.room >= 0 && d.room < SHIP_ROOMS && d.room != ROOM_STAIR && shipWalkable(shipPlaceOf(d.room, d.stand.x, d.stand.z)) &&
                                (d.room == SHIP_START.room || shipNextWay(SHIP_START.room, d.room) >= 0);
            game.testCabinAtDevice(d.facing);
            game.frame(in, 1.0 / 30); in.newFrame();
            const bool f = game.testCabinFacing() == d.facing;
            placed += inRoom ? 1 : 0; faced += f ? 1 : 0;
            if (!inRoom || !f) bad += fmt(" %s (%s%s)", d.use, inRoom ? "" : "off the floors ", f ? "" : "not faced");
        }
        check("ship: every device in a room and reachable", devices == FACE_COUNT - 1 && placed == devices && faced == devices,
              fmt("%d devices, %d stand on their room's floors with a way from the bridge, %d faced from their stands%s", devices, placed, faced, bad.c_str()));
        // the lift: up from the hall to the observatory, down to the bridge
        game.testCabinView("BY THE STAIR"); game.frame(in, 1.0 / 30); in.newFrame();
        pressKey(game, in, KEY_PAGE_UP);
        const int up = game.testCabinRoom();
        pressKey(game, in, KEY_PAGE_DOWN);
        const int down = game.testCabinRoom();
        check("ship: the lift up to the observatory and down to the bridge", up == ROOM_OBSERVATORY && down == ROOM_BRIDGE,
              fmt("Page Up from the hall: %s; Page Down: %s", shipRooms()[up].name, shipRooms()[down].name));
    }
    {   // no collider in an arch's way: an arch's opening across its slab and 0.9 m into each room, a landing's way off out to 0.45 past its rail
        int ways = 0, blocked = 0;
        std::string which;
        for (int i = 0; i < shipOpenings().count; i++) {
            const ShipOpening& o = shipOpenings()[i];
            if (o.kind != OPEN_ARCH && o.kind != OPEN_STAIR) continue;
            ways++;
            for (int side = 0; side < 2; side++) {
                const int room = side == 0 ? o.from : o.to;
                if (room == ROOM_STAIR) continue;
                double ax, az, bx, bz;
                shipWayPoints(i, room, ax, az, bx, bz);
                bool hit = false;
                for (int k = 0; k <= 20 && !hit; k++) {   // along the way from its point in the room to its point beyond: the walker's place free
                    const double u = k / 20.0, x = ax + (bx - ax) * u, z = az + (bz - az) * u;
                    for (const ShipCollider& c : shipColliders()) {
                        if (c.room != room) continue;
                        if (c.kind == COLLIDE_BOX && x > c.x0 && x < c.x1 && z > c.z0 && z < c.z1) hit = true;
                        if (c.kind == COLLIDE_CIRCLE && std::hypot(x - c.x, z - c.z) < c.r) hit = true;
                        if (c.kind == COLLIDE_RING && std::hypot(x - c.x, z - c.z) < c.r && std::fabs(wrapAngle(std::atan2(x - c.x, z - c.z) - c.gapAt)) > c.gapHalf) hit = true;
                    }
                }
                if (hit) { blocked++; which += fmt(" %s's way %d", shipRooms()[room].name, i); }
            }
        }
        check("ship: no collider in an arch", ways >= 8 && blocked == 0, fmt("%d ways (the arches, the stair's landings), %d blocked%s", ways, blocked, which.c_str()));
    }
    {   // the stair walked round from the hangar's landing to the observatory's in small steps: its floor continuous, at each level at its landing
        const ShipStair& S = shipStair();
        ShipPlace p = shipPlaceOf(ROOM_HANGAR, S.x + 2.2 * std::sin(shipLandingAngle(ROOM_HANGAR)), S.z + 2.2 * std::cos(shipLandingAngle(ROOM_HANGAR)));
        auto toward = [&](double r, double a) { shipWalk(p, S.x + r * std::sin(a), S.z + r * std::cos(a)); };
        for (int i = 0; i < 40 && p.room != ROOM_STAIR; i++) toward(2.2 - 0.05 * i, shipLandingAngle(ROOM_HANGAR));   // in at the hangar's landing
        double worst = 0, lo = 1e9, hi = -1e9, last = shipFloorAt(p);
        int steps = 0;
        const bool on = p.room == ROOM_STAIR;
        while (on && p.room == ROOM_STAIR && steps < 4000) {   // round and up, a hundredth of a radian a step at the middle of the steps
            const double a = p.stairA + 0.01;
            const double r = std::fabs(a - shipLandingAngle(ROOM_OBSERVATORY)) < 0.05 ? 2.4 : 0.95;
            toward(r, a);
            const double y = shipFloorAt(p);
            worst = std::max(worst, std::fabs(y - last)); lo = std::min(lo, y); hi = std::max(hi, y); last = y;
            steps++;
        }
        check("ship: the stair's floor continuous from the hangar to the observatory", on && p.room == ROOM_OBSERVATORY && worst < 0.01 && std::fabs(lo - S.level[0]) < 1e-9 && std::fabs(hi - S.level[2]) < 1e-9,
              fmt("onto it %d, off it in %s after %d steps; its floor %.2f to %.2f m, the largest step %.4f m", (int)on, shipRooms()[p.room].name, steps, lo, hi, worst));
    }
    {   // the banks: a seen screen keeps its home bank; past the pool the farthest dims (eleven left: every bank of the space view kept, as
        // B-435's looking out was at its worst); looking in, every bank but 0, the hull's and the light's; B-437: looking out, those and the
        // outside's one (the space view's pixels in it, read through its own palette)
        std::vector<ShipBankAsk> asks;
        for (int h = 21; h <= 30; h++) asks.push_back({h, RGB(1, 1, 1), (double)h});
        asks.push_back({9, RGB(1, 1, 1), 1.0});
        asks.push_back({2, RGB(1, 1, 1), 40.0}); asks.push_back({3, RGB(1, 1, 1), 50.0});   // two more than the pool of eleven, the farthest
        asks.push_back({7, RGB(1, 1, 1), 60.0});   // the outside's bank asked as a home: looking out it goes to a free one
        const uint32_t fixed = 1u | (1u << BANK_HULL) | (1u << BANK_LIGHT), space = 0xFFu | (0x3FFu << 11) | (1u << 31);
        ShipBanks few, in, out;
        few.deal(asks, fixed | space);
        in.deal(asks, fixed);
        out.deal(asks, fixed | (1u << BANK_SPACE));
        bool homes = true, kept = true;
        for (int h = 21; h <= 30; h++) homes = homes && few.of[h] == h && in.of[h] == h && out.of[h] == h;
        for (int h = 0; h < SHIP_HOMES; h++) kept = kept && out.of[h] != BANK_SPACE;
        check("ship: the banks dealt, home first, the farthest dimmed past the pool", homes && kept && few.of[9] == 9 && few.of[2] < 0 && few.of[3] < 0 && few.of[7] < 0 && few.dimmed == 3 &&
                                                                                           few.pool == 11 && in.dimmed == 0 && in.pool == 29 && in.of[2] >= 0 && in.of[3] >= 0 && in.of[7] == 7 &&
                                                                                           in.of[2] != BANK_HULL && out.dimmed == 0 && out.pool == 28 && out.of[7] >= 0,
              fmt("eleven left: %d asked, %d dealt, %d dimmed; looking in: %d dealt of %d (the far ones to %d, %d and %d); looking out: %d dealt of %d, the outside's %d to %d", few.asked,
                  few.dealt, few.dimmed, in.dealt, in.pool, in.of[2], in.of[3], in.of[7], out.dealt, out.pool, BANK_SPACE, out.of[7]));
    }
    {   // I-03: every view's banks whole: from each of the plan's views and each device's stand, the light on, a frame's dealt banks each
        // one home's (no bank two hues), never 0, the hull's or the light's, looking out never the outside's (B-437); at most 32 banks a frame
        TestGame game;
        game.savePrefix = "shots/tests/unit_ship_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_ship_guide.txt";
        game.keysPath = "shots/tests/test_keys.txt";
        remove(game.guidePath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input in;
        int frames = 0, bad = 0, worst = 0, out = 0, asked = 0, dimmed = 0, pool = 99;
        std::string why, dim;
        auto one = [&](const char* name) {
            game.frame(in, 1.0 / 30); in.newFrame();
            const CabinFrame& f = game.testCabinFrame();
            int used = 0; std::string w;
            frames++; worst = std::max(worst, used);
            if (!shipBanksWhole(f, used, w)) { bad++; why += fmt(" %s:%s", name, w.c_str()); }
            worst = std::max(worst, used); out += f.sight.seesOut ? 1 : 0; asked = std::max(asked, f.banks.asked);
            if (f.banks.dimmed > 0) { dimmed++; dim += fmt(" %s %d of %d", name, f.banks.dimmed, f.banks.asked); }
            if (f.sight.seesOut) pool = std::min(pool, f.banks.pool);
        };
        for (const ShipView& v : shipViews()) { game.testCabinView(v.name); one(v.name); }
        for (const CabinDevice& d : Game::cabinDevices()) if (d.facing != FACE_NOTHING) { game.testCabinAtDevice(d.facing); one(d.use); }
        check("ship: every view's banks whole", frames >= 30 && bad == 0 && worst <= 32,
              fmt("%d frames (%d looking out), %d not whole%s; at most %d banks in use, %d asked", frames, out, bad, why.c_str(), worst, asked));
        // B-435: the user saw a far sofa and a far screen grey until they came near: looking out the pool was the cabin's eleven, and the hall's
        // views asked fifteen. With the banks the outside leaves, none dims from any view at the start's world
        check("ship: no finish or screen dimmed from any view (B-435)", frames >= 30 && dimmed == 0 && pool > 11,
              fmt("%d frames, %d with one dimmed%s; looking out the least pool %d (it was 11)", frames, dimmed, dim.c_str(), pool));
        // B-437 (KI-441): the outside sunk into its own bank, the pool is every bank but four whatever the glass shows. The worst outside, every
        // bank of the space view across the glass's box: kept from the pool, they left eleven, and the hangar's middle asks twenty-one
        game.harness.shipBusyOutside = true;
        int busy = 0, busyDim = 0, busyBad = 0, held = 32;
        std::string busyWhy;
        auto worstOutside = [&](const char* name) {
            game.frame(in, 1.0 / 30); in.newFrame();
            const CabinFrame& f = game.testCabinFrame();
            if (!f.sunk) return;
            busy++;
            int n = 0;
            for (int b = 0; b < 32; b++) n += (f.outsideBanks >> b) & 1u;
            held = std::min(held, n);
            int used = 0; std::string w;
            if (!shipBanksWhole(f, used, w)) { busyBad++; busyWhy += fmt(" %s:%s", name, w.c_str()); }
            if (f.banks.dimmed > 0) { busyDim++; busyWhy += fmt(" %s %d of %d dimmed", name, f.banks.dimmed, f.banks.asked); }
        };
        for (const ShipView& v : shipViews()) { game.testCabinView(v.name); worstOutside(v.name); }
        for (const CabinDevice& d : Game::cabinDevices()) if (d.facing != FACE_NOTHING) { game.testCabinAtDevice(d.facing); worstOutside(d.use); }
        game.harness.shipBusyOutside = false;
        check("ship: nothing dimmed whatever the outside shows (B-437)", busy >= 25 && busyDim == 0 && busyBad == 0 && held >= 18,
              fmt("%d frames looking out on every bank of the space view (at least %d in the glass's box), %d with one dimmed, %d not whole%s", busy, held, busyDim,
                  busyBad, busyWhy.c_str()));
    }
    {   // I-03: the light: each room's lamps make a pool (its brightest vertex over 0.3 of a full light) and a room's mean is under its
        // pool's; the bridge's floor in the sun's patches with the sun over the bow, none with it astern; the dome's floor lit from above
        const ShipMesh& m = shipMesh();
        double top[SHIP_ROOMS] = {}, sum[SHIP_ROOMS] = {}; int count[SHIP_ROOMS] = {};
        for (const ShipPiece& pc : m.pieces) {
            if (pc.room >= SHIP_ROOMS || pc.room == ROOM_STAIR || pc.kind != PIECE_POLYGON || pc.mat == MAT_SCREEN || pc.mat == MAT_PANE) continue;
            for (int i = 0; i < pc.n; i++) { const double l = m.verts[pc.first + i].lamp; top[pc.room] = std::max(top[pc.room], l); sum[pc.room] += l; count[pc.room]++; }
        }
        int pools = 0;
        std::string rooms;
        for (int r = 0; r < SHIP_ROOMS; r++) {
            if (r == ROOM_STAIR) continue;
            const double mean = count[r] ? sum[r] / count[r] : 0;
            if (top[r] > 0.3 && mean < top[r]) pools++;
            rooms += fmt(" %s %.2f/%.2f", shipRooms()[r].name + 4, mean, top[r]);
        }
        std::vector<ShipPatch> ahead, astern, dome;
        shipPatches(ROOM_BRIDGE, normalize(Vec3(0, 0.5, 0.85)), 1, ahead);
        shipPatches(ROOM_BRIDGE, normalize(Vec3(0, 0.5, -0.85)), 1, astern);
        shipPatches(ROOM_OBSERVATORY, normalize(Vec3(0.3, 0.8, 0.2)), 1, dome);
        check("ship: the lamps' pools and the sun's patches", pools == SHIP_ROOMS - 1 && !ahead.empty() && astern.empty() && !dome.empty(),
              fmt("%d rooms of %d with a pool (mean/brightest:%s); the bridge's patches with the sun over the bow %zu, astern %zu; the dome's %zu", pools, SHIP_ROOMS - 1,
                  rooms.c_str(), ahead.size(), astern.size(), dome.size()));
    }
    {   // B-434: no patch of the sun over a hole: the user saw the laboratory's patch on its port wall fill the arch, from the hall (the hall
        // drawn first, the laboratory over it). Every room's patches for 96 suns round the sky, against points inside each arch (in both its
        // walls), window, the well and the oculus on their planes; the patches still on the walls with an arch
        struct HolePoint { int room; Vec3 p; int kind; };   // kind: an arch, a window, the well or the oculus
        const char* const KINDS_OF[3] = {"an arch", "a window", "a hole"};
        std::vector<HolePoint> holes;
        auto wallAt = [](const ShipRoom& R, int side, double along, double y) {
            return side == SIDE_BOW ? Vec3(along, y, R.z1) : (side == SIDE_STERN ? Vec3(along, y, R.z0) : (side == SIDE_PORT ? Vec3(R.x0, y, along) : Vec3(R.x1, y, along)));
        };
        const ShipRows<ShipRoom> rooms = shipRooms();
        for (const ShipOpening& o : shipOpenings()) {
            if (o.kind == OPEN_ARCH) {
                for (int k = 1; k <= 3; k++)
                    for (double h : {0.2, 0.45}) {
                        const double along = o.lo + (o.hi - o.lo) * k / 4, y = rooms[o.from].floor + o.height * h;
                        holes.push_back({o.from, wallAt(rooms[o.from], o.side, along, y), 0});
                        holes.push_back({o.to, wallAt(rooms[o.to], (o.side + 2) % 4, along, y), 0});
                    }
            } else if (o.kind == OPEN_WELL || o.kind == OPEN_OCULUS) {
                const bool well = o.kind == OPEN_WELL;   // the well in its room's floor and its lower room's ceiling, the oculus the other way
                for (int k = 0; k <= 6; k++) {
                    const double a = k * TAU / 6, rr = k == 0 ? 0 : o.r * 0.6, x = o.x + rr * std::cos(a), z = o.z + rr * std::sin(a);
                    holes.push_back({o.from, Vec3(x, well ? rooms[o.from].floor : rooms[o.from].ceiling, z), 2});
                    holes.push_back({o.to, Vec3(x, well ? rooms[o.to].ceiling : rooms[o.to].floor, z), 2});
                }
            }
        }
        for (const ShipWindow& w : shipWindows())
            for (int k = 1; k <= 3; k++)
                for (double h : {0.3, 0.7}) holes.push_back({w.room, wallAt(rooms[w.room], w.side, w.lo + (w.hi - w.lo) * k / 4, w.y0 + (w.y1 - w.y0) * h), 1});
        int suns = 0, over = 0, onArchWalls = 0; size_t patches = 0;
        std::string first;
        for (int i = 0; i < 96; i++) {   // a Fibonacci sphere's directions
            const double y = 1 - 2 * (i + 0.5) / 96, rad = std::sqrt(std::max(0.0, 1 - y * y)), a = i * PI * (3 - std::sqrt(5.0));
            const Vec3 sun(rad * std::cos(a), y, rad * std::sin(a));
            suns++;
            for (int r = 0; r < SHIP_ROOMS; r++) {
                std::vector<ShipPatch> ps;
                shipPatches(r, sun, 1, ps);
                patches += ps.size();
                for (const ShipPatch& q : ps) {
                    Vec3 n(0, 0, 0);   // the patch's plane (Newell's)
                    for (int j = 0; j < q.n; j++) { const Vec3& u = q.p[j]; const Vec3& v = q.p[(j + 1) % q.n]; n = n + Vec3((u.y - v.y) * (u.z + v.z), (u.z - v.z) * (u.x + v.x), (u.x - v.x) * (u.y + v.y)); }
                    if (length(n) < 1e-12) continue;
                    n = normalize(n);
                    bool archWall = false;
                    for (const HolePoint& h : holes) {
                        if (h.room != r || std::fabs(dot(n, h.p - q.p[0])) > 1e-3) continue;
                        if (h.kind == 0) archWall = true;
                        int pos = 0, neg = 0;
                        for (int j = 0; j < q.n; j++) { const double s = dot(cross(q.p[(j + 1) % q.n] - q.p[j], h.p - q.p[j]), n); pos += s > 1e-9; neg += s < -1e-9; }
                        if (pos == 0 || neg == 0) {
                            if (first.empty()) first = fmt(": %s's over %s at (%.1f, %.1f, %.1f), the sun (%.2f %.2f %.2f)", rooms[r].name, KINDS_OF[h.kind], h.p.x, h.p.y, h.p.z, sun.x, sun.y, sun.z);
                            over++;
                        }
                    }
                    onArchWalls += archWall;
                }
            }
        }
        check("ship: no patch of the sun over an arch, a window or a hole (B-434)", over == 0 && onArchWalls > 0,
              fmt("%d suns, %zu patches (%d on a wall with an arch), %zu points in the holes: %d covered%s", suns, patches, onArchWalls, holes.size(), over, first.c_str()));
    }
    {   // I-04: life aboard: the home world's garden found and grown by the time played (R-429); the photographs' wall, the shelves, the
        // case's display, the beds and the core in the plan (R-430: the rooms' own sounds gone, the hum the same in every room)
        TestGame game;
        game.savePrefix = "shots/tests/unit_ship_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_ship_guide.txt";
        game.keysPath = "shots/tests/test_keys.txt";
        remove(game.guidePath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input in;
        game.testCabinView("THE GARDEN"); game.frame(in, 1.0 / 30); in.newFrame();
        const bool home = game.testGardenHome();
        game.testGardenPlanted(); const double g0 = game.testGardenGrowth();
        game.testSetPlayed(GARDEN_GROW_S); const double g60 = game.testGardenGrowth();
        int prints = 0, shelves = 0, display = 0, beds = 0, core = 0, vehicles = 0, rings = 0;
        for (const ShipFixture& F : shipFixtures()) {
            prints += F.kind == FX_PRINTS; shelves += F.kind == FX_SHELF; display += F.kind == FX_DISPLAY; beds += F.kind == FX_BED; core += F.kind == FX_CORE;
            vehicles += F.kind == FX_VEHICLE && F.room == ROOM_MAINTENANCE; rings += F.kind == FX_RING && F.room != SHIP_OUTSIDE;   // R-427
        }
        remove(game.guidePath.c_str());
        check("ship: life aboard: the garden, the explorer's things", home && g60 > g0 + 0.4 && prints == 1 && shelves == 4 && display == 1 && beds == 4 && core == 1 &&
                  vehicles == 2 && rings == 0,
              fmt("the home world's garden %s, grown %.2f at the start and %.2f an hour of play on; %d photographs' wall, %d shelves, %d case's display, %d beds, %d core; "
                  "%d vehicles in the maintenance room, %d drive's ring aboard",
                  home ? "found" : "NOT FOUND", g0, g60, prints, shelves, display, beds, core, vehicles, rings));
    }
    {   // R-429: a bed grown by the time played to its full height and no further; E at it trims it back to its start (that bed alone), again
        // refused until it grows; grown back an hour on; the save's line keeps the time played and the trims
        TestGame game;
        game.savePrefix = "shots/tests/unit_ship_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_ship_guide.txt";
        game.keysPath = "shots/tests/test_keys.txt";
        remove(game.guidePath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input in;
        game.testGardenPlanted();
        const double start = game.testGardenGrowth(3);
        game.testSetPlayed(GARDEN_GROW_S * 0.5); const double half = game.testGardenGrowth(3);
        game.testSetPlayed(GARDEN_GROW_S); const double full = game.testGardenGrowth(3);
        game.testSetPlayed(GARDEN_GROW_S * 3); const double later = game.testGardenGrowth(3);
        game.testCabinAtDevice(FACE_BED_4); game.frame(in, 1.0 / 30); in.newFrame();
        const bool faced = game.testCabinFacing() == FACE_BED_4;
        pressKey(game, in, KEY_E);
        const double cut = game.testGardenGrowth(3), other = game.testGardenGrowth(0);
        pressKey(game, in, KEY_E);
        const std::string again = game.testStatus();
        const double still = game.testGardenGrowth(3);
        game.saveSlot(7);
        TestGame back;
        back.savePrefix = game.savePrefix; back.settingsPath = game.settingsPath; back.guidePath = game.guidePath; back.keysPath = game.keysPath;
        const bool loaded = back.loadSlot(7);
        const bool kept = loaded && std::fabs(back.testPlayed() - game.testPlayed()) < 1e-6 && std::fabs(back.testGardenGrowth(3) - cut) < 1e-3 &&
                          std::fabs(back.testGardenGrowth(0) - other) < 1e-9;
        game.testSetPlayed(game.testPlayed() + GARDEN_GROW_S); const double regrown = game.testGardenGrowth(3);
        remove((game.savePrefix + "_7.txt").c_str()); remove(game.guidePath.c_str());
        const bool grows = std::fabs(start - GARDEN_START) < 1e-9 && half > start + 0.2 && half < full - 0.2 && std::fabs(full - 1) < 1e-9 && later == full;
        const bool trims = faced && cut < GARDEN_START + 0.01 && std::fabs(other - 1) < 1e-9 && std::fabs(still - cut) < 1e-6 && again.find("NOTHING TO TRIM") != std::string::npos;
        check("ship: a bed grown by the time played, trimmed back by E, kept by the save (R-429)", grows && trims && kept && std::fabs(regrown - 1) < 1e-9,
              fmt("grown %.2f as planted, %.2f half an hour on, %.2f an hour on, %.2f three hours on; E at the bed %s: %.2f (the others %.2f), again '%s'; "
                  "%.2f an hour later; the save %s",
                  start, half, full, later, faced ? "faced" : "NOT FACED", cut, other, again.c_str(), regrown, kept ? "kept it" : "DID NOT KEEP IT"));
    }
    {   // R-428: the wall shows the newest twelve until the explorer hangs one at it (E: its screen), then theirs: the oldest hung in the first
        // frame, the second taken down; a photograph taken after moves it no more; the prints in their frames; the save's line keeps it
        const std::string gallery = "shots/tests/unit_wall_gallery";
        makeDir(gallery);
        auto photo = [&](int n) {
            std::vector<uint32_t> px((size_t)128 * 80, 0xFF000000u | (uint32_t)(n * 17 % 256) | (uint32_t)(n * 61 % 256) << 8 | (uint32_t)(n * 97 % 256) << 16);
            writePNG(fmt("%s/screenshot_%d.png", gallery.c_str(), n).c_str(), px.data(), 128, 80);   // a print is made of 64 x 64 at least
            std::ofstream c(fmt("%s/screenshot_%d.txt", gallery.c_str(), n)); c << "PHOTOGRAPH " << n << "\n";
        };
        for (int n = 0; n < 20; n++) { remove(fmt("%s/screenshot_%d.png", gallery.c_str(), n).c_str()); remove(fmt("%s/screenshot_%d.txt", gallery.c_str(), n).c_str()); }
        for (int n = 0; n < 14; n++) photo(n);
        TestGame game;
        game.savePrefix = "shots/tests/unit_ship_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_ship_guide.txt";
        game.keysPath = "shots/tests/test_keys.txt"; game.shotsDir = gallery;
        remove(game.guidePath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input in;
        game.testCabinAtDevice(FACE_PRINTS); game.frame(in, 1.0 / 30); in.newFrame();
        game.testWaitPrints();
        const int printed0 = game.testPrints(), first0 = game.testPrintAt(0);
        const bool newest = printed0 == SHIP_PRINTS && first0 == 13 && game.testPrintAt(11) == 2 && !game.testWallChosen();
        pressKey(game, in, KEY_E);
        const bool screen = game.state == GameState::WALL && game.testWallLevel() == 0;
        pressKey(game, in, KEY_ENTER);
        for (int i = 0; i < 13; i++) pressKey(game, in, KEY_RIGHT);
        pressKey(game, in, KEY_ENTER);
        pressKey(game, in, KEY_RIGHT); pressKey(game, in, KEY_BACKSPACE);
        pressKey(game, in, KEY_ESCAPE);
        const bool closed = game.state == GameState::SPACE;
        photo(14); game.noteScreenshot();   // a photograph taken after
        game.frame(in, 1.0 / 30); in.newFrame(); game.testWaitPrints();
        bool kept = game.testWallChosen() && game.testPrintAt(0) == 0 && game.testPrintAt(1) < 0 && game.testPrints() == SHIP_PRINTS - 1;
        for (int k = 2; k < SHIP_PRINTS; k++) kept = kept && game.testPrintAt(k) == 13 - k;
        game.saveSlot(7);
        TestGame back;
        back.savePrefix = game.savePrefix; back.settingsPath = game.settingsPath; back.guidePath = game.guidePath; back.keysPath = game.keysPath; back.shotsDir = gallery;
        bool saved = back.loadSlot(7) && back.testWallChosen();
        for (int k = 0; k < SHIP_PRINTS; k++) saved = saved && back.testWallNumber(k) == game.testWallNumber(k);
        remove((game.savePrefix + "_7.txt").c_str()); remove(game.guidePath.c_str());
        check("ship: the wall hung by the explorer, kept by the save (R-428)", newest && screen && closed && kept && saved,
              fmt("the newest twelve %s (%d prints, the first %d); E %s; the oldest hung in the first frame and the second taken down: %s (%d prints, the first %d); "
                  "after a new photograph %s; the save %s",
                  newest ? "printed" : "NOT PRINTED", printed0, first0, screen ? "the wall's screen" : "NOT THE WALL'S SCREEN", closed ? "closed" : "NOT CLOSED",
                  game.testPrints(), game.testPrintAt(0), kept ? "kept" : "NOT KEPT", saved ? "kept it" : "DID NOT KEEP IT"));
    }
    {   // a save of the 5 m room (its cabin line, no place line) loads at the bridge's start with its light and hull; a save of the ship
        // keeps the room and the place
        TestGame game;
        game.savePrefix = "shots/tests/unit_ship_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_ship_guide.txt";
        game.keysPath = "shots/tests/test_keys.txt";
        remove(game.guidePath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input in;
        game.testCabinAtDevice(FACE_DECODER); game.frame(in, 1.0 / 30); in.newFrame();
        pressKey(game, in, KEY_U);   // the light off
        game.saveSlot(7);
        const std::string path = game.savePrefix + "_7.txt";
        TestGame back;
        back.savePrefix = game.savePrefix; back.settingsPath = game.settingsPath; back.guidePath = game.guidePath; back.keysPath = game.keysPath;
        const bool loaded = back.loadSlot(7);
        const ShipPlace kept = back.testCabinPlaceNow();
        const bool same = loaded && kept.room == ROOM_LIBRARY && std::fabs(kept.x - Game::cabinDevices().row[FACE_DECODER].stand.x) < 1e-6 && back.testCabinLights() == 0;
        // the same save as 1.4 wrote it: the cabin line of the 5 m room, no place line
        std::string text, line;
        { std::ifstream f(path); while (std::getline(f, line)) if (line.rfind("cabin_place", 0) != 0) text += (line.rfind("cabin ", 0) == 0 ? "cabin -1.1 0.2 1.5 -0.3 0 0 0" : line) + "\n"; }
        { std::ofstream f(path); f << text; }
        TestGame old;
        old.savePrefix = game.savePrefix; old.settingsPath = game.settingsPath; old.guidePath = game.guidePath; old.keysPath = game.keysPath;
        const bool oldLoaded = old.loadSlot(7);
        const ShipPlace start = old.testCabinPlaceNow();
        const bool atStart = oldLoaded && start.room == SHIP_START.room && start.x == SHIP_START.at.x && start.z == SHIP_START.at.z && old.testCabinLights() == 0;
        remove(path.c_str()); remove(game.guidePath.c_str());
        check("ship: a save keeps the room and the place; the 5 m room's at the bridge's start", same && atStart,
              fmt("the ship's save: %s at %.2f %.2f, light %d; the old cabin line: %s at %.2f %.2f, light %d", shipRooms()[kept.room].name, kept.x, kept.z, back.testCabinLights(),
                  shipRooms()[start.room].name, start.x, start.z, old.testCabinLights()));
    }
}
