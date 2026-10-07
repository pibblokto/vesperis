// W-06: the telescope on the ship. Z in space (the flight computer's devices page too) puts the eye to the telescope: the
// view is the ship's own attitude magnified by powers of two, x2 to x1024 (the focal length multiplied: the setting's field
// divided), the cabin left behind as in the radar camera. The mouse and the arrows steer at a rate that falls with the
// magnification; the reticle names what it rests on (a body, the sun, a star of the neighbourhood: the explorer's name or
// UNKNOWN, the range, the apparent width); Enter tracks the body under the reticle (the ship is re-aimed at it every frame,
// the mouse moving the view's offset from its centre); N names the target through the guide; P takes the photo through the
// usual screenshot path, the target in its caption. Nothing of it is saved: the telescope is stowed on a load.
// R-405, the stabiliser (the user's review: "planet still moves and it creates shaking ... it should feel like it's not
// moving at all"): while the eye is at the eyepiece a parked ship holds its parking and the view's axis in the parked
// body's frame (`tele.frameBody`, `parkBF`, `viewBF`: the lap of the orbit mode and the world's turn are both held off), so
// the ground under the reticle stays where it is at any power until the mouse moves it; stowing lets the lap go on from
// where the ship is. Shift+arrows carry the stowed ship round its world (`orbitShip`), and the telescope then holds the
// side faced.
#include "game.h"
#include "ui.h"
#include "galaxy/drainage.h"
#include <cmath>
#include <algorithm>

namespace {
// an angle as the eyepiece reads it: degrees to a tenth, else minutes, else seconds of arc (`plain`: DEG for a text file)
std::string angleString(double rad, bool plain = false) {
    double deg = rad / DEG;
    if (deg >= 1) return plain ? fmt("%.1f DEG", deg) : fmt("%.1f%c", deg, CH_DEGREE);
    if (deg * 60 >= 1) return fmt("%.0f'", deg * 60);
    return fmt("%.0f\"", deg * 3600);
}
}

Proj Game::telescopeProj() const {
    Proj p = Proj::fromHFov(settings.fovDeg);
    p.f *= telescopeZoom();
    return p;
}

double Game::telescopeFieldDeg() const { return 2 * std::atan(std::tan(settings.fovDeg * 0.5 * DEG) / telescopeZoom()) / DEG; }

void Game::telescopeToggle() {
    if (tele.on) { telescopeOff(); status("TELESCOPE STOWED", 3); audio.beep = 4; return; }
    if (ship.mode == ShipState::VIMANA) { status("NOTHING TO SEE IN THE FLIGHT - THE TELESCOPE WAITS", 4); audio.beep = 3; return; }
    if (radar.on) radarOff();
    tele.on = true; tele.track = TELE_NONE; tele.offYaw = 0; tele.offPitch = 0;
    cabin.yaw = 0; cabin.pitch = 0;   // the eye is at the eyepiece: the view is the ship's attitude
    telescopeHoldFrame();             // R-405: the parking and the view held in the parked body's frame from this moment
    telescopeFindTarget();            // what the ship points at is followed from the start: the parked body where the reticle rests (the side faced), another body on its centre
    if (tele.target >= 0 || tele.target == TELE_SUN) { telescopeTrack(tele.target, !(telescopeStabilised() && tele.target == tele.frameBody)); statusNext = statusMsg; statusNextSecs = 3; }
    status("TELESCOPE: +/- ZOOM  ENTER TRACKS  N NAMES  Z STOWS", 4);   // under 52 characters: the line's width (the HUD reads the power)
    audio.beep = 4;
}

void Game::telescopeOff() { tele.on = false; tele.track = TELE_NONE; tele.target = TELE_NONE; tele.frameBody = -1; }

void Game::telescopeZoomStep(int dir) {
    int s = clampi(tele.step + dir, 1, TELE_MAX_STEP);
    if (s == tele.step) { audio.beep = 3; return; }
    tele.step = s;
    audio.beep = 4;
}

namespace { inline Vec3 forwardOf(double yaw, double pitch) { return Vec3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)); } }

// Enter (and the opening) tracks a body: its centre brought under the reticle (`recentre`), or, under the stabiliser on the
// parked body, the view kept where it is and held
void Game::telescopeTrack(int id, bool recentre) {
    tele.track = id; tele.offYaw = 0; tele.offPitch = 0;
    bool held = telescopeStabilised() && id == tele.frameBody;
    if (recentre || !held) telescopeAim();
    if (held) telescopeHoldView();
    status(fmt(held ? "STABILISED ON %s" : "TRACKING %s", trunc(telescopeTargetName(id), 26).c_str()), 3);
    audio.beep = 4;
}

// R-405: the stabiliser. A ship parked at a body holds its parking direction and the view's axis in the body's frame while
// the eye is at the eyepiece: the lap of the orbit mode is held off and the world's turn carries the ship with it, so the
// ground under the reticle does not move at any power. Taken when the telescope opens and when the ship arrives at a parking
// with it out; the view's axis is taken again after every input
bool Game::telescopeStabilised() const {
    return tele.on && tele.frameBody >= 0 && sys.valid && ship.mode == ShipState::PARKED && ship.parkedBody == tele.frameBody && ship.parkedBelt < 0 && tele.frameBody < (int)sys.bodies.size();
}

void Game::telescopeHoldFrame() {
    tele.frameBody = -1;
    if (!tele.on || !sys.valid || ship.mode != ShipState::PARKED || ship.parkedBody < 0 || ship.parkedBody >= (int)sys.bodies.size() || ship.parkedBelt >= 0) return;
    tele.frameBody = ship.parkedBody;
    tele.parkBF = sys.bodyFrame(tele.frameBody, t) * ship.parkDir;
    telescopeHoldView();
}

void Game::telescopeHoldView() {
    if (tele.frameBody < 0) return;
    tele.viewBF = sys.bodyFrame(tele.frameBody, t) * forwardOf(ship.yaw, ship.pitch);
    tele.heldYaw = ship.yaw; tele.heldPitch = ship.pitch;
}

// every frame while tracking a body that is not the held one: the ship re-aimed at the tracked body, the view's offset
// from its centre kept
void Game::telescopeAim() {
    Vec3 at = tele.track == TELE_SUN ? sys.star.pos : sys.bodyPos(tele.track, t);
    Vec3 fwd = normalize(at - ship.pos);
    ship.yaw = wrap2pi(std::atan2(fwd.x, fwd.z) + tele.offYaw);
    ship.pitch = clampd(std::asin(clampd(fwd.y, -1, 1)) + tele.offPitch, -89 * DEG, 89 * DEG);
}

// what the reticle rests on, from the ship's attitude and the magnification: a body or the sun within 18 px of its disc (the
// local target's own pick), else a star of the neighbourhood within 14 px (the aim's); the pick shrinks in angle as the
// magnification grows
void Game::telescopeFindTarget() {
    tele.target = TELE_NONE; tele.targetRangeKm = 0; tele.targetWidth = 0;
    Mat3 cam = viewBasis();
    Proj pj = telescopeProj();
    const double S = FB_SCALE;
    double bestD = 18 * S;
    auto consider = [&](int id, const Vec3& posW, double R) {
        Vec3 v = cam * (posW - ship.pos);
        double dist = length(v);
        if (v.z <= 0 || dist <= R) return;
        double angR = std::asin(clampd(R / dist, 0, 1));
        double sx = pj.f * v.x / v.z, sy = pj.f * v.y / v.z;
        double rpx = pj.f * std::tan(angR) * (dist / v.z);
        double d = std::max(0.0, std::sqrt(sx * sx + sy * sy) - rpx);
        if (d < bestD) { bestD = d; tele.target = id; tele.targetRangeKm = dist - R; tele.targetWidth = 2 * angR; }
    };
    if (sys.valid && ship.mode != ShipState::VIMANA) {
        for (int i = 0; i < (int)sys.bodies.size(); i++) consider(i, sys.bodyPos(i, t), sys.bodies[i].radiusKm);
        consider(TELE_SUN, sys.star.pos, sys.star.radiusKm);
    }
    if (tele.target != TELE_NONE) return;
    bestD = 14 * S;
    for (const Star& s : nb.stars) {
        if (sys.valid && s.seed == sys.star.seed) continue;
        Vec3 v = cam * (s.pos - ship.pos);
        if (v.z <= 0) continue;
        double sx = pj.f * v.x / v.z, sy = pj.f * v.y / v.z, d = std::sqrt(sx * sx + sy * sy);
        if (d < bestD) { bestD = d; tele.target = TELE_STAR; tele.targetStar = s; tele.targetRangeKm = length(s.pos - ship.pos); tele.targetWidth = 0; }
    }
}

std::string Game::telescopeTargetName(int id) const {
    if (id >= 0 && sys.valid && id < (int)sys.bodies.size()) return upper(bodyNameOf(id));
    if (id == TELE_SUN && sys.valid) return upper(starNameOf(sys.star));
    if (id == TELE_STAR) return upper(starNameOf(tele.targetStar));
    return "";
}

// the reticle's line: "<name>  <type>  <range>  <width>" for a body, the class for a sun, light years for a star
std::string Game::telescopeTargetLine(bool plain) const {
    int id = tele.target;
    if (id >= 0 && sys.valid && id < (int)sys.bodies.size()) {
        const Body& b = sys.bodies[id];
        std::string kind = b.type == PT_COMPANION ? std::string(STAR_CLASSES[b.starClass].code) + " " + STAR_CLASSES[b.starClass].name : std::string(shortType(b.type)) + (b.parent >= 0 ? " MOON" : "");
        return fmt("%s  %s  %s  %s", trunc(telescopeTargetName(id), 18).c_str(), kind.c_str(), distanceString(std::max(0.0, tele.targetRangeKm)).c_str(), angleString(tele.targetWidth, plain).c_str());
    }
    if (id == TELE_SUN && sys.valid) return fmt("%s  %s %s  %s  %s", trunc(telescopeTargetName(id), 16).c_str(), STAR_CLASSES[sys.star.cls].code, STAR_CLASSES[sys.star.cls].name, distanceString(std::max(0.0, tele.targetRangeKm)).c_str(), angleString(tele.targetWidth, plain).c_str());
    if (id == TELE_STAR) {
        Star full; starInSector(tele.targetStar.sx, tele.targetStar.sy, tele.targetStar.sz, full, true);
        return fmt("%s  %s %s  %.2f LY", trunc(telescopeTargetName(id), 16).c_str(), STAR_CLASSES[full.cls].code, STAR_CLASSES[full.cls].name, tele.targetRangeKm / SECTOR_KM);
    }
    return "";
}

// N: the target named through the guide, as the guide menu names a world or the star; a star of the neighbourhood gets a name
// of its own (text entry kind 8)
void Game::telescopeName() {
    guideReturn = GameState::SPACE; returnState = GameState::SPACE;
    if (tele.target >= 0 && sys.valid) {
        const Body& b = sys.bodies[tele.target];
        textBody = tele.target;
        const char* what = b.type == PT_COMPANION ? "NAME THE SECOND SUN" : (b.type == PT_COMET ? "NAME THE COMET" : (b.parent >= 0 ? "NAME THIS MOON" : "NAME THIS WORLD"));
        beginTextEntry(what, 2, bodyNamed(tele.target) ? upper(bodyNameOf(tele.target)) : std::string());
    } else if (tele.target == TELE_SUN && sys.valid) beginTextEntry("NAME THIS STAR", 1, starNamed(sys.star) ? upper(starNameOf(sys.star)) : std::string());
    else if (tele.target == TELE_STAR) { textStar = tele.targetStar; beginTextEntry("NAME THE STAR", 8, starNamed(textStar) ? upper(starNameOf(textStar)) : std::string()); }
    else { status("NOTHING UNDER THE RETICLE TO NAME", 3); audio.beep = 3; }
}

// the eyepiece's controls: steering at a rate that falls with the magnification (the offset from the tracked body's centre
// while tracking), the zoom steps (+/-, the wheel, PgUp/PgDn), Enter tracks or lets go, N names
void Game::updateTelescope(const Input& in, double realDt) {
    if (ship.mode == ShipState::VIMANA) { telescopeOff(); return; }
    double z = telescopeZoom();
    double rate = 0.0032 * settings.mouseSensitivity / z;
    double dy = in.mouseDx * rate, dp = -in.mouseDy * rate * (settings.invertY ? -1 : 1);
    if (in.isDown(KEY_LEFT)) dy -= 1.2 * realDt / z;
    if (in.isDown(KEY_RIGHT)) dy += 1.2 * realDt / z;
    if (in.isDown(KEY_UP)) dp += 0.8 * realDt / z;
    if (in.isDown(KEY_DOWN)) dp -= 0.8 * realDt / z;
    if (in.wheel > 0 || in.wasPressed(KEY_EQUAL) || in.wasPressed(KEY_PAGE_UP)) telescopeZoomStep(1);
    if (in.wheel < 0 || in.wasPressed(KEY_MINUS) || in.wasPressed(KEY_PAGE_DOWN)) telescopeZoomStep(-1);
    bool tracking = tele.track != TELE_NONE && sys.valid && ship.mode != ShipState::APPROACH && (tele.track == TELE_SUN || (tele.track >= 0 && tele.track < (int)sys.bodies.size()));
    if (!tracking) tele.track = TELE_NONE;
    bool held = telescopeStabilised();
    if (!held) tele.frameBody = -1;   // off a parking (the frame is taken again when the ship arrives at one, in updateShipMotion)
    if (held) {   // the stabiliser: the view's axis where the body's frame carries it (a turn from elsewhere, X or the harness, taken up instead), then the input, then held anew
        if (ship.yaw != tele.heldYaw || ship.pitch != tele.heldPitch) telescopeHoldView();
        else {
            Vec3 fwd = sys.bodyFrame(tele.frameBody, t).transposed() * tele.viewBF;
            ship.yaw = wrap2pi(std::atan2(fwd.x, fwd.z)); ship.pitch = clampd(std::asin(clampd(fwd.y, -1, 1)), -89 * DEG, 89 * DEG);
        }
    }
    if (tracking && !(held && tele.track == tele.frameBody)) { tele.offYaw += dy; tele.offPitch = clampd(tele.offPitch + dp, -PI / 2, PI / 2); telescopeAim(); }
    else { ship.yaw = wrap2pi(ship.yaw + dy); ship.pitch = clampd(ship.pitch + dp, -89 * DEG, 89 * DEG); }
    if (held) telescopeHoldView();
    telescopeFindTarget();
    if (enterKey(in) && !ship.targeting) {
        if (tele.track != TELE_NONE) { tele.track = TELE_NONE; status("TRACKING OFF", 2); audio.beep = 4; }
        else if (tele.target >= 0 || tele.target == TELE_SUN) telescopeTrack(tele.target);
        else { status("NOTHING UNDER THE RETICLE TO TRACK", 3); audio.beep = 3; }
    }
    if (in.wasPressed(KEY_N) && !in.ctrl() && !ship.targeting) telescopeName();
}

int Game::telescopePlateBody() const {
    if (!tele.on || !sys.valid || ship.mode == ShipState::VIMANA) return -1;
    if (tele.track >= 0 && tele.track < (int)sys.bodies.size()) return tele.track;
    if (tele.target >= 0 && tele.target < (int)sys.bodies.size()) return tele.target;
    return -1;
}

// the camera's frame (the user's review: an instrument of its own, as the radar camera): the visor and the brackets, `CAM 04
// TELESCOPE` with its REC, the power and the field, the EPOC and the system; the reticle (a circle open at the four points
// and a cross that leaves the centre clear, cyan while tracking) with the target's line under it; the plate's exposure while
// it builds; a scale bar for the target's range; what is tracked and the keys at the bottom
void Game::renderTelescopeHUD() {
    const uint32_t fc = HUD_WHITE, fd = HUD_DIM;
    const int cx = UW / 2, cy = UH / 2, r = 28;
    drawVisor(fd);
    {   // brackets inside the visor's
        int m = 14, l = 26;
        drawLineRGB(canvas, m, m, m + l, m, fc); drawLineRGB(canvas, m, m, m, m + l, fc);
        drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m - l, m, fc); drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m, m + l, fc);
        drawLineRGB(canvas, m, UH - 1 - m, m + l, UH - 1 - m, fc); drawLineRGB(canvas, m, UH - 1 - m, m, UH - 1 - m - l, fc);
        drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m - l, UH - 1 - m, fc); drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m, UH - 1 - m - l, fc);
    }
    drawTextShadow(canvas, 18, 16, "CAM 04 TELESCOPE", fc, HUD_SHADOW);   // clear of the bracket's upright (x 14)
    if ((int)(realTime * 2) & 1) drawTextShadow(canvas, 18 + textWidth("CAM 04 TELESCOPE") + 6, 16, "\x07REC", HUD_RED, HUD_SHADOW);
    drawTextShadow(canvas, 18, 24, fmt("X%.0f  FIELD %s", telescopeZoom(), angleString(telescopeFieldDeg() * DEG).c_str()).c_str(), HUD_CYAN, HUD_SHADOW);
    drawTextShadow(canvas, UW - 8 - textWidth(epocString().c_str()), 8, epocString().c_str(), HUD_GREEN, HUD_SHADOW);
    std::string sysName = sys.valid ? trunc(upper(starNameOf(sys.star)), 20) : "INTERSTELLAR SPACE";
    drawTextShadow(canvas, UW - 8 - textWidth(sysName.c_str()), 16, sysName.c_str(), fd, HUD_SHADOW);
    // the plate's exposure: the share of the planet function read into it, while it builds
    int pb = telescopePlateBody();
    double prog = pb >= 0 && spaceR.plate().body == pb ? spaceR.plateProgress() : -1;
    if (prog >= 0 && prog < 1) {
        std::string e = fmt("EXPOSING %3.0f%%", prog * 100);
        drawTextShadow(canvas, UW - 8 - textWidth(e.c_str()), 24, e.c_str(), fd, HUD_SHADOW);
        drawRectRGB(canvas, UW - 8 - 60, 33, UW - 8, 36, fd);
        if (prog > 0.02) fillRectRGB(canvas, UW - 8 - 59, 34, UW - 8 - 59 + (int)(58 * prog), 35, HUD_CYAN);
    }
    // the reticle
    uint32_t col = tele.track != TELE_NONE ? HUD_CYAN : fd;
    for (int k = 0; k < 48; k++) {
        if ((k % 12) < 2 || (k % 12) > 9) continue;
        double a0 = k * TAU / 48, a1 = (k + 1) * TAU / 48;
        drawLineRGB(canvas, cx + (int)std::lround(std::cos(a0) * r), cy + (int)std::lround(std::sin(a0) * r), cx + (int)std::lround(std::cos(a1) * r), cy + (int)std::lround(std::sin(a1) * r), col);
    }
    drawLineRGB(canvas, cx - 12, cy, cx - 5, cy, col); drawLineRGB(canvas, cx + 5, cy, cx + 12, cy, col);
    drawLineRGB(canvas, cx, cy - 12, cx, cy - 5, col); drawLineRGB(canvas, cx, cy + 5, cx, cy + 12, col);
    std::string line = telescopeTargetLine();
    if (!line.empty()) drawTextCentered(canvas, cx, cy + r + 6, trunc(line, 52).c_str(), tele.track != TELE_NONE && tele.track == tele.target ? HUD_CYAN : HUD_GREEN);
    // the scale: forty pixels of the picture at the target's range (inside the bracket's corner, clear of its lines)
    if (tele.target >= 0 || tele.target == TELE_SUN) {
        double km = (tele.targetRangeKm + (tele.target >= 0 ? sys.bodies[tele.target].radiusKm : sys.star.radiusKm)) * 40.0 * FB_SCALE / spaceR.proj.f;
        int bx = UW - 18 - 40, by = UH - 26;
        drawLineRGB(canvas, bx, by, bx + 40, by, fc); drawLineRGB(canvas, bx, by - 2, bx, by + 2, fc); drawLineRGB(canvas, bx + 40, by - 2, bx + 40, by + 2, fc);
        std::string lab = distanceString(km);
        drawTextShadow(canvas, UW - 18 - textWidth(lab.c_str()), by - 10, lab.c_str(), fc, HUD_SHADOW);
    }
    if (tele.track != TELE_NONE) drawTextShadow(canvas, 18, UH - 34, fmt(telescopeStabilised() && tele.track == tele.frameBody ? "STABILISED ON %s" : "TRACKING %s", trunc(telescopeTargetName(tele.track), 22).c_str()).c_str(), HUD_CYAN, HUD_SHADOW);
    else if (tele.target >= 0 || tele.target == TELE_SUN) drawTextShadow(canvas, 18, UH - 34, "ENTER TRACKS THE TARGET", fd, HUD_SHADOW);
    {   // X-01: parked at a giant, the reticle is where a probe goes
        double plat, plon;
        if (!probe.active && sys.valid && ship.mode == ShipState::PARKED && ship.parkedBody >= 0 && ship.parkedBelt < 0 && isProbeGiant(sys.bodies[ship.parkedBody].type) && viewGround(ship.parkedBody, plat, plon))
            drawTextShadow(canvas, 18, UH - 42, fmt("C SENDS A PROBE TO %.1f%c %s %.1f%c %s", std::fabs(plat / DEG), CH_DEGREE, plat >= 0 ? "N" : "S", std::fabs(plon / DEG), CH_DEGREE, plon >= 0 ? "E" : "W").c_str(), HUD_AMBER, HUD_SHADOW);
    }
    drawTextShadow(canvas, 18, UH - 26, "+/- ZOOM  N NAMES  P PHOTO  Z / ESC STOWS", fd, HUD_SHADOW);
}

void Game::testTelescopeSettle() { Input in; telePlateFinish = true; frame(in, 1.0 / 30); }
std::string Game::testTelescopePlate() const { return spaceR.plateInfo(); }
double Game::testTelescopePlateProgress() const { int pb = telescopePlateBody(); return pb >= 0 && spaceR.plate().body == pb ? spaceR.plateProgress() : -1; }

// the plate's own cells (ones that hold their own sample) against the planet function sampled again at the cell's centre
// and the plate's detail: the same function with the same arguments must give the same numbers
void Game::testPlateCheck(int n, double& maxAlb, int& matMiss, double& maxH) {
    const DetailPlate& p = spaceR.plate();
    maxAlb = 0; matMiss = 0; maxH = 0;
    if (!p.valid() || p.body < 0 || !sys.valid || p.body >= (int)sys.bodies.size()) { matMiss = -1; return; }
    const BodyGen& g = spaceR.genFor(sys.bodies[p.body]);
    DrainageOff off(p.noDrainage);
    int checked = 0;
    for (int k = 0; k < n * 8 && checked < n; k++) {
        int i = (k * 37 + 11) % p.W, j = (k * 53 + 7) % p.H;
        size_t o = (size_t)j * p.W + i;
        if (!p.exact[o] || p.material[o] == 255) continue;
        Vec3 u = SpaceRenderer::plateCellUnit(p, i, j);
        SurfaceSample ss = sampleSurface(g, u, p.texelM);
        double lat, lon; StarSystem::latLonFromBody(u, lat, lon);
        double alb = ss.albedo;
        if (g.type == PT_GASGIANT && std::fabs(lat) > 76 * DEG) alb *= 1 + 0.14 * std::sin(6 * lon) * smoothstep(76 * DEG, 86 * DEG, std::fabs(lat));
        maxAlb = std::max(maxAlb, std::fabs(alb - p.albedo[o] / 255.0));
        if (ss.material != p.material[o]) matMiss++;
        maxH = std::max(maxH, std::fabs(ss.height - p.height[o]));
        checked++;
    }
    if (!checked) matMiss = -1;
}

std::string Game::testTelescopeInfo() const {
    if (!tele.on) return "stowed";
    return fmt("x%.0f, field %.3f deg, target %d '%s', tracking %d%s, offsets %.5f/%.5f rad, yaw %.4f pitch %.4f", telescopeZoom(), telescopeFieldDeg(), tele.target, telescopeTargetLine().c_str(), tele.track, telescopeStabilised() ? " (stabilised)" : "", tele.offYaw, tele.offPitch, ship.yaw, ship.pitch);
}

long Game::testPlateBegun() const { return spaceR.plateBegun; }

// the ground under the reticle of the parked body: the central ray's point on its sphere, in the body's frame
bool Game::testTelescopeGround(double& lat, double& lon) const {
    int b = ship.parkedBody;
    if (!sys.valid || ship.mode != ShipState::PARKED || b < 0 || b >= (int)sys.bodies.size()) return false;
    return viewGround(b, lat, lon);
}

// the ground of a body under the view's centre (the central ray's first point on its sphere, in the body's frame); false when
// the ray misses it. X-01 aims the probe with it
bool Game::viewGround(int b, double& lat, double& lon) const {
    if (!sys.valid || b < 0 || b >= (int)sys.bodies.size()) return false;
    Mat3 cam = viewBasis();
    Vec3 cv = cam * (sys.bodyPos(b, t) - ship.pos);
    double R = sys.bodies[b].radiusKm, perp2 = cv.x * cv.x + cv.y * cv.y;
    if (cv.z <= 0 || perp2 >= R * R) return false;
    double tHit = cv.z - std::sqrt(R * R - perp2);
    Vec3 u = normalize(sys.bodyFrame(b, t) * (cam.transposed() * ((Vec3(0, 0, tHit) - cv) / R)));
    StarSystem::latLonFromBody(u, lat, lon);
    return true;
}

bool Game::testGroundScreen(int body, double lat, double lon, double& sx, double& sy) const {
    if (!sys.valid || body < 0 || body >= (int)sys.bodies.size()) return false;
    Vec3 at = sys.bodyPos(body, t) + sys.bodyFrame(body, t).transposed() * StarSystem::bodyFromLatLon(lat, lon) * sys.bodies[body].radiusKm;
    Vec3 v = viewBasis() * (at - ship.pos);
    if (v.z <= 0) return false;
    Proj pj = tele.on ? telescopeProj() : spaceR.proj;
    sx = pj.cx + pj.f * v.x / v.z; sy = pj.cy - pj.f * v.y / v.z;
    return true;
}

bool Game::testSubShip(double& lat, double& lon) const {
    int b = ship.parkedBody;
    if (!sys.valid || ship.mode != ShipState::PARKED || b < 0 || b >= (int)sys.bodies.size() || ship.parkedBelt >= 0) return false;
    StarSystem::latLonFromBody(normalize(sys.bodyFrame(b, t) * ship.parkDir), lat, lon);
    return true;
}

bool Game::testBodyScreen(int body, double& sx, double& sy, double& rpx) const {
    if (body < 0 || body >= (int)spaceR.bodyInfo.size() || spaceR.bodyInfo[body].body != body) return false;
    const BodyScreenInfo& bi = spaceR.bodyInfo[body];
    sx = bi.sx; sy = bi.sy; rpx = bi.radiusPx;
    return bi.inFront;
}

int Game::testTelescopePick() const {
    int best = -1; double bestAng = 0;
    if (!sys.valid) return -1;
    for (int i = 0; i < (int)sys.bodies.size(); i++) {
        if (i == ship.parkedBody) continue;
        double ang = sys.bodies[i].radiusKm / std::max(1.0, length(sys.bodyPos(i, t) - ship.pos));
        if (ang > bestAng) { bestAng = ang; best = i; }
    }
    return best;
}
