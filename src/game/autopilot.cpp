// The Stardrifter: remote and local targeting, Vimana flight, fine approach, parking.
#include "game.h"
#include "ui.h"
#include "core/rng.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

void Game::arriveAtStar(const Star& s, const Vec3& fromDir) {
    if (sys.valid) guide.pushHistory(starKeyOf(sys.star));
    Star full = s;
    if (full.name.empty()) starInSector(s.sx, s.sy, s.sz, full, true);
    sys.generate(full);
    ship.mode = ShipState::STANDBY;
    ship.parkedBody = -1;
    ship.localTarget = -1;
    ship.targetBelt = -1; ship.parkedBelt = -1;   // O3
    ship.hasRemote = false;
    radarOff();   // C-07: the signals are scanned afresh here
    double firstOrbit = sys.bodies.empty() ? full.radiusKm * 30 : sys.bodies[0].orbitRadiusKm;
    ship.pos = full.pos + fromDir * std::max(full.radiusKm * 25.0, firstOrbit * 0.55);
    Vec3 fwd = normalize(full.pos - ship.pos);
    ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(fwd.y);
    nb.update(ship.pos);
    visitNoted = false;
    arrivalNote = fmt("ARRIVED AT %s - %d PLANETS, %d MOONS", upper(starNameOf(full)).c_str(), sys.planetCount(),
                      (int)sys.bodies.size() - sys.planetCount());
    logEvent("ARRIVAL", fmt("%s, %s, %d PLANETS %d MOONS, SECTOR %lld %lld %lld", upper(starNameOf(full)).c_str(), sys.classString().c_str(), sys.planetCount(), (int)sys.bodies.size() - sys.planetCount(), (long long)full.sx, (long long)full.sy, (long long)full.sz));
    status(arrivalNote, 8);
    audio.beep = 2;
    arrivalFlash = 1.0;
}

void Game::lockRemoteTarget() {
    // nearest star to the crosshair
    Mat3 cam = viewBasis();
    const Star* best = nullptr;
    double bestD = 1e9;
    for (const Star& s : nb.stars) {
        Vec3 v = cam * (s.pos - ship.pos);
        if (v.z <= 0) continue;
        double sx = spaceR.proj.f * v.x / v.z, sy = -spaceR.proj.f * v.y / v.z;
        double d = std::sqrt(sx * sx + sy * sy);
        if (d < bestD) { bestD = d; best = &s; }
    }
    if (!best || bestD > 14 * FB_SCALE) { status("NO STAR IN THE CROSSHAIR", 3); audio.beep = 3; return; }
    if (sys.valid && best->seed == sys.star.seed) { status("THAT IS THE CURRENT STAR", 3); audio.beep = 3; return; }
    ship.remote = *best;
    starInSector(best->sx, best->sy, best->sz, ship.remote, true);
    ship.hasRemote = true;
    ship.targeting = false;
    double ly = length(ship.remote.pos - ship.pos) / SECTOR_KM;
    status(fmt("REMOTE TARGET: %s (%s) %.2f LY - V TO FLY", upper(starNameOf(ship.remote)).c_str(), STAR_CLASSES[ship.remote.cls].code, ly), 6);
    audio.beep = 1;
}

// R-407: the crosshair's direction in the world: the view's forward (the cabin's head turn on the ship's attitude), the way the
// aim names and locks its stars
Vec3 Game::aimDirection() const { return normalize(viewBasis().transposed() * Vec3(0, 0, 1)); }

// R-407 (the user's request of 2026-10-07: "fly stardrifter in any direction you want for any number of light years ... break the
// loop with just saying 'now I jump 400 light years in that direction' and boom"): the star nearest the end of the line becomes the
// remote target and the Vimana flies at once; where the end lies in the void the jump comes back along the line to the first stars
void Game::startJump(double ly, const Vec3& dir) {
    if (ship.mode == ShipState::VIMANA) { status("CANNOT JUMP DURING VIMANA FLIGHT", 3); audio.beep = 3; return; }
    ly = clampd(ly, 1, JUMP_MAX_LY);
    Star dest; double shortLy = 0;
    if (!jumpTarget(ship.pos, dir, ly, dest, shortLy, sys.valid ? sys.star.seed : 0)) { status("NO STAR TO COME OUT AT ALONG THAT LINE", 4); audio.beep = 3; return; }
    Star full; starInSector(dest.sx, dest.sy, dest.sz, full, true);
    const std::string from = sys.valid ? upper(starNameOf(sys.star)) : std::string("DEEP SPACE");
    const std::string way = galacticHeading(ship.pos / SECTOR_KM, dir);
    ship.targeting = false; jumpDigits.clear();
    setRemoteStar(full);
    if (!ship.hasRemote || ship.remote.seed != full.seed) return;   // refused (the current star: `jumpTarget` leaves it out already)
    const double flyLy = length(full.pos - ship.pos) / SECTOR_KM;
    logEvent("JUMP", fmt("%.0f LY %s FROM %s TO PARSIS %+lld %+lld %+lld, %s%s", ly, way.c_str(), from.c_str(), (long long)full.sx, (long long)full.sy, (long long)full.sz,
                         REGION_NAMES[galaxyRegion(full.sx, full.sy, full.sz)], shortLy > 0 ? fmt(" (%.0f LY SHORT: THE VOID)", shortLy).c_str() : ""));
    toggleVimana();
    if (ship.mode != ShipState::VIMANA) return;
    if (shortLy > 0) status(fmt("NO STARS OUT THERE - THE JUMP ENDS %.0f LY SHORT", shortLy), 6);
    else status(fmt("VIMANA JUMP OF %.0f LY: A STAR %.0f LY AWAY", ly, flyLy), 6);
}

void Game::nearestStars(int n, std::vector<const Star*>& out) const {
    std::vector<std::pair<double, const Star*>> all;
    for (const Star& s : nb.stars) {
        if (sys.valid && s.seed == sys.star.seed) continue;
        all.push_back({length2(s.pos - ship.pos), &s});
    }
    std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    out.clear();
    for (size_t i = 0; i < all.size() && (int)i < n; i++) out.push_back(all[i].second);
}

void Game::cycleTargetStar() {
    std::vector<const Star*> near;
    nearestStars(6, near);
    if (near.empty()) { status("NO STARS NEARBY", 3); audio.beep = 3; return; }
    targetCycle = (targetCycle + 1) % (int)near.size();
    Vec3 fwd = normalize(near[targetCycle]->pos - ship.pos);
    ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1));
    cabin.yaw = 0; cabin.pitch = 0;   // G-04: the view faces the star as X faces the local target; turned in the cabin, the lock never found it
    audio.beep = 4;
}

int Game::pickBodyNearCrosshair() {
    int best = -1; double bestD = 1e9;
    for (const BodyScreenInfo& bi : spaceR.bodyInfo) {
        if (bi.body < 0 || !bi.inFront) continue;
        double d = std::sqrt((bi.sx - FBW / 2) * (bi.sx - FBW / 2) + (bi.sy - FBH / 2) * (bi.sy - FBH / 2));
        d = std::max(0.0, d - bi.radiusPx);
        if (d < bestD) { bestD = d; best = bi.body; }
    }
    return bestD < 18 * FB_SCALE ? best : -1;
}

// N0-01: a comet is parked at 12 nucleus radii, where its jets and the root of the tail are in view
double Game::parkDistanceFor(const Body& b) {
    return b.radiusKm * (b.type == PT_COMET ? 12.0 : (b.rings ? std::max(3.5, b.ringOuter + 1.3) : 3.5));
}

void Game::startApproach(int body) {
    if (body < 0 || body >= (int)sys.bodies.size()) return;
    if (ship.mode == ShipState::PARKED && ship.parkedBody == body) { status("ALREADY IN ORBIT", 3); return; }
    const Body& b = sys.bodies[body];
    ship.localTarget = body;
    ship.targetBelt = -1; ship.parkedBelt = -1;   // B-410: a body's approach leaves the belt (its parking put the ship back beside its rock at the end)
    ship.mode = ShipState::APPROACH;
    ship.flightFrom = ship.pos;
    ship.flightT = 0;
    ship.aimYaw0 = ship.yaw; ship.aimPitch0 = ship.pitch;
    Vec3 bp = sys.bodyPos(body, t);
    double dist = length(bp - ship.pos);
    ship.flightDur = 4.0 + 2.0 * std::log10(1 + dist / 1e5);
    ship.parkDist = parkDistanceFor(b);
    Vec3 toStar = normalize(sys.star.pos - bp);
    Vec3 fromShip = normalize(ship.pos - bp);
    // arrive on the lit side, off to one side for a nice view
    Vec3 side = normalize(cross(toStar, Vec3(0, 1, 0)));
    if (dot(side, fromShip) < 0) side = -side;
    ship.parkDir = normalize(toStar * 0.55 + side * 0.75 + fromShip * 0.3 + Vec3(0, 0.18, 0));
    ship.parkedBody = -1;
    status(fmt("FINE APPROACH: %s", upper(bodyNameOf(body)).c_str()), 4);
    audio.beep = 1;
}

// O3 (R-302): a belt as a destination. The ship keeps its angle round the star, its radius is clamped into the belt,
// and the biggest rock within two cells of that anchor is chosen; the parking point is six rock radii off on the
// sunlit side, a little above, and it co-orbits with the rock.
bool Game::pickBeltRock(int k, double at, int64_t& ir, int64_t& ia, int64_t& iy, int& m) const {
    if (k < 0 || k >= (int)sys.belts.size()) return false;
    const Belt& bl = sys.belts[k];
    Vec3 rel = ship.pos - sys.star.pos;
    double r = std::sqrt(rel.x * rel.x + rel.z * rel.z), ang = std::atan2(rel.z, rel.x);
    double w = bl.outerKm - bl.innerKm;
    r = clampd(r, bl.innerKm + 0.15 * w, bl.outerKm - 0.15 * w);
    Vec3 anchor = sys.star.pos + Vec3(std::cos(ang) * r, 0, std::sin(ang) * r);
    double best = -1;
    sys.forBeltRocksNear(k, anchor, at, 2, 2, 1, [&](const BeltRock& rk) { if (rk.radiusKm > best) { best = rk.radiusKm; ir = rk.ir; ia = rk.ia; iy = rk.iy; m = rk.m; } });
    return best > 0;
}

bool Game::beltRockNow(BeltRock& rk) const {
    int k = ship.parkedBelt >= 0 ? ship.parkedBelt : ship.targetBelt;
    return sys.beltRockAt(k, ship.rockIr, ship.rockIa, ship.rockIy, ship.rockM, t, rk);
}

Vec3 Game::beltParkPos(double at) const {
    BeltRock rk;
    int k = ship.parkedBelt >= 0 ? ship.parkedBelt : ship.targetBelt;
    if (!sys.beltRockAt(k, ship.rockIr, ship.rockIa, ship.rockIy, ship.rockM, at, rk)) return ship.pos;
    Vec3 toStar = normalize(sys.star.pos - rk.pos);
    Vec3 side = normalize(cross(toStar, Vec3(0, 1, 0)));
    Vec3 dir = normalize(toStar * 0.55 + side * 0.75 + Vec3(0, 0.18, 0));
    return rk.pos + dir * ship.rockDist;
}

std::string Game::localTargetLabel() const {
    if (ship.targetBelt >= 0 && ship.targetBelt < (int)sys.belts.size()) return beltNameOf(ship.targetBelt);
    if (ship.localTarget >= 0 && ship.localTarget < (int)sys.bodies.size()) return bodyLabelOf(ship.localTarget);
    return "";
}

void Game::startApproachBelt(int k) {
    if (k < 0 || k >= (int)sys.belts.size()) return;
    if (ship.mode == ShipState::PARKED && ship.parkedBelt == k) { status("ALREADY IN THE BELT", 3); return; }
    if (!pickBeltRock(k, t, ship.rockIr, ship.rockIa, ship.rockIy, ship.rockM)) { status("NO ROCK WITHIN REACH IN THAT BELT", 3); audio.beep = 3; return; }
    BeltRock rk;
    sys.beltRockAt(k, ship.rockIr, ship.rockIa, ship.rockIy, ship.rockM, t, rk);
    ship.rockDist = std::max(6.0 * rk.radiusKm, 2.0);
    ship.targetBelt = k; ship.localTarget = -1;
    ship.parkedBody = -1; ship.parkedBelt = -1;
    ship.mode = ShipState::APPROACH;
    ship.flightFrom = ship.pos;
    ship.flightT = 0;
    ship.aimYaw0 = ship.yaw; ship.aimPitch0 = ship.pitch;
    double dist = length(beltParkPos(t) - ship.pos);
    ship.flightDur = 4.0 + 2.0 * std::log10(1 + dist / 1e5);
    status(fmt("FINE APPROACH: %s - A ROCK %.1f KM ACROSS", upper(beltNameOf(k)).c_str(), 2 * rk.radiusKm), 4);
    audio.beep = 1;
}

void Game::updateShipMotion(double dt) {
    switch (ship.mode) {
        case ShipState::VIMANA: {
            ship.flightT += lastRealDt;
            double u = clampd(ship.flightT / ship.flightDur, 0, 1);
            double s = u * u * (3 - 2 * u);
            ship.pos = lerp(ship.flightFrom, ship.flightTo, s);
            nb.update(ship.pos);
            if (u >= 1) {
                Vec3 fromDir = normalize(ship.flightFrom - ship.remote.pos);
                arriveAtStar(ship.remote, fromDir);
            }
            break;
        }
        case ShipState::APPROACH: {
            ship.flightT += lastRealDt;
            double u = clampd(ship.flightT / ship.flightDur, 0, 1);
            double s = u * u * (3 - 2 * u);
            bool beltRun = ship.targetBelt >= 0;   // O3
            BeltRock rk;
            if (beltRun && !beltRockNow(rk)) { ship.mode = ShipState::STANDBY; ship.targetBelt = -1; break; }
            Vec3 bp = beltRun ? rk.pos : sys.bodyPos(ship.localTarget, t);
            Vec3 dest = beltRun ? beltParkPos(t) : bp + ship.parkDir * ship.parkDist;
            ship.pos = lerp(ship.flightFrom, dest, s);
            // turn toward the body: blend from the initial orientation onto an exact aim
            Vec3 fwd = normalize(bp - ship.pos);
            double ty = std::atan2(fwd.x, fwd.z), tp = std::asin(clampd(fwd.y, -1, 1));
            double w = smoothstep(0.0, std::min(2.5, ship.flightDur * 0.5), ship.flightT);
            ship.yaw = wrap2pi(ship.aimYaw0 + wrapAngle(ty - ship.aimYaw0) * w);
            ship.pitch = ship.aimPitch0 + (tp - ship.aimPitch0) * w;
            if (u >= 1 && beltRun) {
                ship.mode = ShipState::PARKED;
                ship.parkedBelt = ship.targetBelt;
                ship.parkedBody = -1;
                const Belt& bl = sys.belts[ship.parkedBelt];
                status(fmt("IN THE BELT: %s - A ROCK %.1f KM ACROSS, NO GROUND FOR THE CAPSULE", upper(beltNameOf(ship.parkedBelt)).c_str(), 2 * rk.radiusKm), 8);
                logEvent("BELT", fmt("%s, %s - %s FROM THE STAR, A ROCK %.1f KM ACROSS", upper(beltNameOf(ship.parkedBelt)).c_str(), distanceString(bl.innerKm).c_str(), distanceString(bl.outerKm).c_str(), 2 * rk.radiusKm));
                audio.beep = 2;
                break;
            }
            if (u >= 1) {
                ship.mode = ShipState::PARKED;
                ship.parkedBody = ship.localTarget;
                const Body& b = sys.bodies[ship.parkedBody];
                status(fmt("IN ORBIT: %s - %s%s", upper(bodyNameOf(ship.parkedBody)).c_str(), PLANET_TYPES[b.type].name,
                           PLANET_TYPES[b.type].landable ? " - C TO DEPLOY CAPSULE" : (isProbeGiant(b.type) ? " - C LAUNCHES A PROBE" : " - NOT LANDABLE")), 8);   // X-01
                logEvent("ORBIT", fmt("%s, %s, R %.0f KM, %.2f G%s", upper(bodyNameOf(ship.parkedBody)).c_str(), PLANET_TYPES[b.type].name, b.radiusKm, b.gravity / 9.8, b.rings ? ", RINGS" : ""));
                audio.beep = 2;
            }
            break;
        }
        case ShipState::PARKED: {
            if (ship.parkedBelt >= 0) {   // O3: parked beside a rock of the belt, drifting with it
                if (ship.parkedBelt >= (int)sys.belts.size()) { ship.mode = ShipState::STANDBY; ship.parkedBelt = -1; break; }
                ship.pos = beltParkPos(t);
                break;
            }
            if (ship.parkedBody < 0 || ship.parkedBody >= (int)sys.bodies.size()) { ship.mode = ShipState::STANDBY; break; }
            const Body& b = sys.bodies[ship.parkedBody];
            if (tele.on && tele.frameBody != ship.parkedBody) telescopeHoldFrame();   // R-405: the telescope out as the ship arrives at a parking: the frame taken now
            if (telescopeStabilised()) ship.parkDir = normalize(sys.bodyFrame(ship.parkedBody, t).transposed() * tele.parkBF);   // R-405: the stabiliser holds the ship over its ground (the lap held off)
            else if (ship.orbiting) {
                double rate = TAU / (600.0 + 200.0 * b.radiusKm / 6000.0);   // one lap in ~10-20 minutes at x1
                Mat3 rot = Mat3::axisAngle(b.spinAxis, rate * dt);
                ship.parkDir = normalize(rot * ship.parkDir);
            }
            ship.pos = sys.bodyPos(ship.parkedBody, t) + ship.parkDir * ship.parkDist;
            break;
        }
        default: break;
    }
}

// R-405: the parked ship carried round its world by the arrows with Shift held: east and west along its parallel (about the
// spin axis), north and south along its meridian within 85 degrees of the poles, twenty degrees a second (a lap in eighteen
// seconds); the ship's attitude turns with it, so the world stays where it was in the window and its ground streams past.
// The status reads the ground under the ship meanwhile
void Game::orbitShip(double east, double north, double dt) {
    if (!sys.valid || ship.mode != ShipState::PARKED || ship.parkedBody < 0 || ship.parkedBody >= (int)sys.bodies.size() || ship.parkedBelt >= 0) return;
    if (east == 0 && north == 0) return;
    const double RATE = 20 * DEG;
    Mat3 bf = sys.bodyFrame(ship.parkedBody, t);
    Vec3 p = normalize(bf * ship.parkDir);
    double lat, lon; StarSystem::latLonFromBody(p, lat, lon);
    double dlon = east * RATE * dt, dlat = clampd(lat + north * RATE * dt, -85 * DEG, 85 * DEG) - lat;
    Mat3 rot = Mat3::axisAngle(Vec3(0, 0, 1), dlon);   // the body frame's z is the spin axis
    Vec3 meridian = cross(p, Vec3(0, 0, 1));            // the axis that carries p toward the pole
    if (dlat != 0 && length(meridian) > 1e-6) rot = rot * Mat3::axisAngle(meridian, dlat);
    Mat3 rotW = bf.transposed() * rot * bf;
    ship.parkDir = normalize(rotW * ship.parkDir);
    Vec3 fwd = normalize(rotW * Vec3(std::sin(ship.yaw) * std::cos(ship.pitch), std::sin(ship.pitch), std::cos(ship.yaw) * std::cos(ship.pitch)));
    ship.yaw = wrap2pi(std::atan2(fwd.x, fwd.z)); ship.pitch = clampd(std::asin(clampd(fwd.y, -1, 1)), -89 * DEG, 89 * DEG);
    StarSystem::latLonFromBody(normalize(bf * ship.parkDir), lat, lon);
    status(fmt("OVER %.1f%c %s  %.1f%c %s", std::fabs(lat / DEG), CH_DEGREE, lat >= 0 ? "N" : "S", std::fabs(lon / DEG), CH_DEGREE, lon >= 0 ? "E" : "W"), 1);
}

void Game::choosePaletteBodies(int& a, int& b) {
    a = -1; b = -1;
    double sa = 0, sb = 0;
    for (int i = 0; i < (int)sys.bodies.size(); i++) {
        double ang = sys.bodies[i].radiusKm / std::max(1.0, length(sys.bodyPos(i, t) - ship.pos));
        if (ang > sa) { b = a; sb = sa; a = i; sa = ang; }
        else if (ang > sb) { b = i; sb = ang; }
    }
    // bodies without a visible disc do not need a bank
    double minAng = 1.2 / spaceR.proj.f;
    if (sa < minAng) a = -1;
    if (sb < minAng) b = -1;
    // W-06: the telescope's target (tracked, else under the reticle) is drawn in its own colours whatever its size
    int want = tele.on ? (tele.track >= 0 ? tele.track : tele.target) : -1;
    if (want >= 0 && want < (int)sys.bodies.size() && want != a) { b = a; a = want; }
}

void Game::updateSpace(const Input& in, double dt, double realDt) {
    if (in.wasPressed(KEY_Z) && !in.ctrl()) telescopeToggle();   // W-06: the telescope
    if (tele.on) updateTelescope(in, realDt);   // W-06: the mouse and the arrows steer the telescope at a rate that falls with the magnification
    else {
        if (settings.cabin && !radar.on) updateCabin(in, realDt);   // M2: the mouse turns the explorer's head, the arrows the ship
        else {   // the cockpit view; C-07: in the radar camera the mouse turns the ship, the camera being the ship's
            ship.yaw += in.mouseDx * 0.0032 * settings.mouseSensitivity;
            ship.pitch -= in.mouseDy * 0.0032 * settings.mouseSensitivity * (settings.invertY ? -1 : 1);
        }
        bool orbitKeys = in.shift() && !in.ctrl() && ship.mode == ShipState::PARKED && ship.parkedBody >= 0 && ship.parkedBelt < 0;   // R-405: Shift+arrows carry the parked ship round its world
        if (orbitKeys) orbitShip((in.isDown(KEY_RIGHT) ? 1 : 0) - (in.isDown(KEY_LEFT) ? 1 : 0), (in.isDown(KEY_UP) ? 1 : 0) - (in.isDown(KEY_DOWN) ? 1 : 0), realDt);
        else {
            if (in.isDown(KEY_LEFT)) ship.yaw -= 1.2 * realDt;
            if (in.isDown(KEY_RIGHT)) ship.yaw += 1.2 * realDt;
            if (in.isDown(KEY_UP)) ship.pitch += 0.8 * realDt;
            if (in.isDown(KEY_DOWN)) ship.pitch -= 0.8 * realDt;
        }
        ship.pitch = clampd(ship.pitch, -89 * DEG, 89 * DEG);
        ship.yaw = wrap2pi(ship.yaw);
    }
    const bool wasTargeting = ship.targeting;

    if (in.wasPressed(KEY_R)) {
        if (ship.mode == ShipState::VIMANA) status("CANNOT TARGET DURING VIMANA FLIGHT", 3);
        else {
            ship.targeting = !ship.targeting;
            targetCycle = -1; jumpDigits.clear();
            status(ship.targeting ? "AIM: ENTER LOCKS A STAR - OR TYPE LIGHT YEARS TO JUMP" : "TARGETING CANCELLED", 4);
        }
    }
    if (!ship.targeting) jumpDigits.clear();
    if (ship.targeting) {   // R-407: the digits typed in the aim make a jump's light years (a leading zero is no digit)
        for (int k = KEY_0; k <= KEY_9; k++)
            if (in.wasPressed(k) && jumpDigits.size() < 6 && !(jumpDigits.empty() && k == KEY_0)) { jumpDigits += (char)('0' + (k - KEY_0)); audio.beep = 4; }
        if (!jumpDigits.empty() && std::atof(jumpDigits.c_str()) > JUMP_MAX_LY) jumpDigits = fmt("%.0f", JUMP_MAX_LY);
        if (in.wasPressed(KEY_BACKSPACE) && !jumpDigits.empty()) jumpDigits.pop_back();
    }
    if (ship.targeting && in.wasPressed(KEY_N)) cycleTargetStar();
    if (ship.targeting && (enterKey(in) || in.mousePressed[0])) { if (!jumpDigits.empty()) startJump(std::atof(jumpDigits.c_str()), aimDirection()); else lockRemoteTarget(); }
    if (in.wasPressed(KEY_B) && !in.ctrl()) radarToggle();   // C-07: the signal radar
    bool radarEnter = updateRadar(in, realDt);              // C-07: the sweep, the hold, the lock; true when Enter accepted a lock
    if (in.wasPressed(KEY_V)) toggleVimana();
    bool inSystem = sys.valid && !sys.bodies.empty() && ship.mode != ShipState::VIMANA;
    if (in.wasPressed(KEY_L) && !in.ctrl()) {
        if (!inSystem) { status("NO PLANETS HERE", 3); audio.beep = 3; }
        else {
            int picked = pickBodyNearCrosshair();
            int nb = (int)sys.bodies.size(), nbl = (int)sys.belts.size();
            if (picked >= 0) { ship.localTarget = picked; ship.targetBelt = -1; }
            else {   // O3: the cycle runs through the bodies, then the belts
                int cur = ship.targetBelt >= 0 ? nb + ship.targetBelt : ship.localTarget;
                int nxt = (cur + 1) % (nb + nbl);
                if (nxt < nb) { ship.localTarget = nxt; ship.targetBelt = -1; } else { ship.localTarget = -1; ship.targetBelt = nxt - nb; }
            }
            status(fmt("LOCAL TARGET: %s - ENTER TO APPROACH, TAB FOR LIST", upper(localTargetLabel()).c_str()), 5);
            audio.beep = 4;
        }
    }
    if (in.wasPressed(KEY_TAB) && inSystem) { listSel = ship.targetBelt >= 0 ? (int)sys.bodies.size() + ship.targetBelt : std::max(0, ship.localTarget); returnState = GameState::SPACE; state = GameState::SYSTEM_LIST; }
    if (enterKey(in) && !wasTargeting && !radarEnter && !tele.on && inSystem && ship.mode != ShipState::APPROACH) {   // W-06: in the telescope Enter tracks
        if (ship.targetBelt >= 0) startApproachBelt(ship.targetBelt);
        else if (ship.localTarget >= 0) startApproach(ship.localTarget);
    }
    if (in.mousePressed[0] && !wasTargeting && inSystem) {
        int picked = pickBodyNearCrosshair();
        if (picked >= 0) { ship.localTarget = picked; ship.targetBelt = -1; status(fmt("LOCAL TARGET: %s", upper(bodyLabelOf(picked)).c_str()), 4); audio.beep = 4; }
    }
    if (in.wasPressed(KEY_G) && !in.ctrl()) { openGuide(); return; }
    if (in.wasPressed(KEY_M) && !in.ctrl() && ship.mode != ShipState::VIMANA) { guideReturn = GameState::SPACE; returnState = GameState::SPACE; openStarMap(); return; }
    if (in.wasPressed(KEY_J) && !in.ctrl()) { guideReturn = GameState::SPACE; returnState = GameState::SPACE; logPage = 0; state = GameState::LOG; return; }
    if (in.wasPressed(KEY_F) && !in.ctrl()) { fieldAmp = !fieldAmp; status(fieldAmp ? "FIELD AMPLIFICATOR ON" : "FIELD AMPLIFICATOR OFF", 3); audio.beep = 4; }
    if (in.wasPressed(KEY_X) && inSystem && (ship.localTarget >= 0 || ship.targetBelt >= 0)) {
        BeltRock rk;
        Vec3 at = ship.targetBelt >= 0 && beltRockNow(rk) ? rk.pos : sys.bodyPos(std::max(0, ship.localTarget), t);
        // from where the ship will be this frame: a ship parked in a belt moves a hundred kilometres per frame at x10
        Vec3 from = ship.mode == ShipState::PARKED && ship.parkedBelt >= 0 ? beltParkPos(t) : ship.pos;
        Vec3 fwd = normalize(at - from);
        ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1));
        cabin.yaw = 0; cabin.pitch = 0;
    }
    if (in.wasPressed(KEY_O) && ship.mode == ShipState::PARKED) { ship.orbiting = !ship.orbiting; status(ship.orbiting ? "SYNCHRONOUS ORBIT" : "FIXED POINT CHASE", 3); }
    if (in.wasPressed(KEY_C)) deployCapsule();
    if (dataKey(in) && inSystem) { returnState = GameState::SPACE; state = GameState::DATA; }
    updateShipMotion(dt);
    audio.hum = radar.on ? 0.25 : 0.6;   // C-07: the hum under the receiver's static
    audio.engine = ship.mode == ShipState::VIMANA ? 0.9 : (ship.mode == ShipState::APPROACH ? 0.35 : 0.0);
    audio.wind = 0; audio.rain = 0; audio.lava = 0;
}
