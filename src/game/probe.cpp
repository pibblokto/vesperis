// X-01 (2026-10-07): the probe. C at a giant (the landing screen in the cabin, the flight computer's deploy row) sends a probe
// to the ground under the telescope's reticle, else under the ship; the screen becomes its camera (CAM 05 PROBE): the fall
// from the ship with the giant growing, the entry's plasma, the descent through the decks with the depth, the pressure, the
// temperature and the wind, the chute (Space, once: the pressure grows four and a half times under it at a quarter of the
// pace, so a layer is lingered in), photographs (P), and the end: the last frame held under the loss-of-signal hiss until
// the screen is left. The relay's clock is real time (galaxy/probe.h): the warp turns the sky and not the fall; Shift held
// on the screen runs it eight times as fast (R-406). The ship holds its parking meanwhile; leaving it cuts the relay.
// from the entry's peak the picture is the probe's own (space/probe_view.h): the decks round the aim
// point as fields (galaxy/probe.h: the globe's bands sampled at the launch, the towers, the pouches, the fog inside), the
// sky's in-scatter over the space renderer's frame without the giant (the stars, the sun's disc, the moons), the rings'
// arc, the lightning as places in the decks.
// each giant its own (galaxy/probe.h's character): one to three decks of its own clouds and colours, its
// storms and lightning (an ice giant's quiet), the deep's glow (a young giant's, a brown dwarf's from below everything), the
// polar aurorae's curtains, a great storm's wall, the clear-air hole a descent may fall through, a carbon-rich giant's hail,
// and on one rare giant something that passes the lamp once in the deep.
// what comes back. The probe is lost; its record is kept in the guide (galaxy-side, the descent is a function of
// the probe's numbers): the launch, the explorer's turns of the camera and the game's clock along the fall, the end, the profile it
// measured (the pressure, the temperature, the wind, the light, the lightning, the layers named by what it found) and its final
// image (shots/probe_<n>.png). The gallery plays the recording on the screen (`GameState::RECORDING`: the descent drawn again,
// scrubbed by depth), the giant's data sheet draws the profiles, the log keeps what it crossed with the depths, the statistics the
// probes sent and the deepest, the export lends the recordings like C-14's shards. The storm it fell into can be named (N).
// The camera's picture is probe_feed.cpp; the track, the profile and the recordings are probe_records.cpp.
#include "game.h"
#include "probe_internal.h"
#include "ui.h"
#include "core/rng.h"
#include "space/probe_view.h"
#include "core/parallel.h"
#include "core/noise.h"
#include "core/png.h"
#include "core/fs.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>

// the ship is parked at the probe's giant (the relay holds)
bool Game::probeHere() const {
    return probe.active && sys.valid && sys.star.seed == probe.starSeed && ship.mode == ShipState::PARKED && ship.parkedBody == probe.body && ship.parkedBelt < 0 &&
           probe.body >= 0 && probe.body < (int)sys.bodies.size();
}

// the ground C sends a probe to: the telescope's reticle on the parked giant, else the ground under the ship
bool Game::probeAimPoint(double& lat, double& lon) const {
    int b = ship.parkedBody;
    if (!sys.valid || ship.mode != ShipState::PARKED || b < 0 || b >= (int)sys.bodies.size() || ship.parkedBelt >= 0) return false;
    if (harness.aimOverride) { lat = harness.aimLat; lon = harness.aimLon; return true; }
    if (tele.on && viewGround(b, lat, lon)) return true;
    StarSystem::latLonFromBody(normalize(sys.bodyFrame(b, t) * ship.parkDir), lat, lon);
    return true;
}

void Game::launchProbe() {
    int b = ship.parkedBody;
    if (probe.active && probe.endClock < 0) { watchProbe(); return; }
    double lat, lon;
    if (!probeAimPoint(lat, lon)) return;
    const Body& g = sys.bodies[b];
    bool aimed = tele.on;
    probe = ProbeState();   // a last frame still held is let go
    probe.active = true; probe.body = b; probe.starSeed = sys.star.seed;
    probe.aimLat = lat; probe.aimLon = lon; probe.clock = 0; probe.launchT = t;
    probe.startRel = ship.pos - sys.bodyPos(b, t);
    probe.seed = hashCombine(g.seed, (uint64_t)(int64_t)std::llround(t * 16)) ^ 0x9B0BEULL;
    probe.flight.crushBar = probeCrushBar(probe.seed);
    probe.atm = giantAtmosphereOf(sys, b);
    probeEnsureClouds();
    {   // X-04: its number, the storm under the aim (at the bands' time), the tracks from the launch
        probe.id = ++guide.probesSent;
        uint64_t sid = 0;
        probe.stormKind = giantStormAt(g, spaceR.genFor(g), lat, lon, t, sid);
        const std::string bk = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, b);
        probe.stormKey = probe.stormKind == 2 ? bk + "/TG" : (probe.stormKind == 1 ? bk + fmt("/T%llx", (unsigned long long)sid) : std::string());
        probe.camTrack = {0.0, 0.0, 0.0}; probe.timeTrack = {0.0, t};
        probe.prevClock = 0; probe.prevT = t; probe.prevPace = -1; probe.prevCamClock = 0;
    }
    {   // the heading the probe comes in along (the camera's default azimuth): its approach over the ground, else toward the sun
        Vec3 aimW = normalize(sys.bodyFrame(b, t).transposed() * StarSystem::bodyFromLatLon(lat, lon)), E, N;
        localFrame(aimW, sys.bodyFrame(b, t).row(2), E, N);
        Vec3 m = aimW - normalize(probe.startRel);
        double me = dot(m, E), mn = dot(m, N);
        Vec3 s = sys.star.pos - (sys.bodyPos(b, t) + aimW * g.radiusKm);
        double sunAz = std::atan2(dot(s, E), dot(s, N));
        probe.headYaw = me * me + mn * mn > 0.02 * 0.02 ? std::atan2(me, mn) : sunAz;
        probe.descYaw = sunAz + 35 * DEG;   // the descent looks a little aside of the sun: the decks lit across, the sun's glow at the side when it is low
        // what the probe was sent to see: a great storm's wall within sight, else the aurora's curtains over the clouds
        double gd, ex, ez;
        if (probeStormWall(gd, ex, ez)) probe.descYaw = std::atan2(ex, ez);
        else if (probeAuroraWay(ex, ez, gd) && std::fabs(gd) < 2800 && probe.atm.ch.aurora > 0.25) probe.descYaw = std::atan2(ex, ez) + (gd < 0 ? PI : 0.0);
    }
    if (tele.on) telescopeOff();
    if (radar.on) radarOff();
    logEvent("PROBE", fmt("PROBE %d INTO %s AT %s", probe.id, upper(bodyNameOf(b)).c_str(), latLonString(lat, lon).c_str()));
    status(aimed ? fmt("PROBE AWAY TO %s", latLonString(lat, lon).c_str()) : "PROBE AWAY, UNDER THE SHIP (THE TELESCOPE AIMS IT)", 5);
    statusNext = "SPACE OPENS THE CHUTE ONCE: WHERE TO LINGER"; statusNextSecs = 5;
    audio.beep = BEEP_LOCK;
    state = GameState::PROBE;
}

void Game::watchProbe() {
    if (!probe.active) return;
    state = GameState::PROBE;
    audio.beep = BEEP_UI;
}

void Game::probeCut(const char* why) {
    if (probe.active && probe.endClock < 0 && probe.body >= 0 && probe.body < (int)sys.bodies.size() && sys.star.seed == probe.starSeed) {
        // (a Vimana flight has just set `sys.valid` off: the system is still the giant's)
        double bar = probeBarAt(probe.flight, probe.clock);
        logEvent("PROBE", bar > 0 ? fmt("RELAY TO PROBE %d IN %s CUT AT %.2f ATM: %s", probe.id, upper(bodyNameOf(probe.body)).c_str(), atmOf(bar), why)
                                  : fmt("RELAY TO PROBE %d FALLING TO %s CUT: %s", probe.id, upper(bodyNameOf(probe.body)).c_str(), why));   // the log wraps a text on two lines of 44
        probeKeep(3);   // X-04: what it sent until then
    }
    if (probe.active && probe.endClock < 0) status(fmt("THE PROBE'S RELAY IS CUT: %s", why), 5);
    probe = ProbeState();
    if (state == GameState::PROBE) state = GameState::SPACE;
}

void Game::probeRelease() {
    probe = ProbeState();
    if (state == GameState::PROBE) state = GameState::SPACE;
}

void Game::probeEnd() {
    probe.endClock = probe.clock; probe.endT = t;
    probe.endBar = probeBarAt(probe.flight, probe.clock);
    probe.endKm = -giantAltitudeKm(probe.atm, probe.endBar);
    probe.endK = giantTemperatureK(probe.atm, probe.endBar);
    probe.endCause = probe.endBar >= probe.flight.crushBar * 0.999 ? 1 : 2;
    int mins = (int)std::lround(probe.clock / 60);
    logEvent("PROBE", fmt("PROBE %d LOST IN %s: %.0f KM DOWN, %.1f ATM, %+.0f C, %d MIN - %s", probe.id, upper(bodyNameOf(probe.body)).c_str(), probe.endKm, atmOf(probe.endBar), probe.endK - 273.15,
                          mins, probe.endCause == 1 ? "THE HULL GAVE WAY" : "THE HEAT TOOK IT"));   // two lines of 44 with a name of twenty
    probeKeep(probe.endCause);   // X-04: the record, its final image held on the screen
    if (state != GameState::PROBE) status("PROBE LOST - ITS LAST FRAME IS ON THE SCREEN (C)", 6);
}

// every simulating frame: the relay holds while the ship stays parked at the giant; the clock, the chute's tearing, the end
void Game::updateProbe(double realDt) {
    if (!probe.active) return;
    if (!probeHere()) { if (probe.endClock < 0) probeCut("THE SHIP LEFT ITS PARKING"); else probeRelease(); return; }
    if (probe.endClock >= 0) return;
    probe.clock += realDt * (probe.fast ? PROBE_FAST : 1.0);   // R-406: `fast` is Shift held on the probe's screen this frame (Game::frame)
    double endC = probeEndClock(probe.atm, probe.flight);
    if (probe.flight.chuteOpen >= 0 && !probe.tornNoted && probe.clock >= probe.flight.chuteClose && probe.flight.chuteClose < endC) {
        probe.tornNoted = true;
        if (state == GameState::PROBE) status(fmt("THE CHUTE TORE AWAY AT %.2f ATM", atmOf(probeBarAt(probe.flight, probe.flight.chuteClose))), 4);
    }
    probe.clock = std::min(probe.clock, endC);
    probeTrack();   // the tracks and the log's discoveries, to the end
    if (probe.clock >= endC) probeEnd();
}

void Game::updateProbeScreen(const Input& in, double dt, double realDt) {
    updateShipMotion(dt);
    if (!probe.active) { state = GameState::SPACE; return; }
    bool ended = probe.endClock >= 0;
    if (in.wasPressed(KEY_ESCAPE) || (in.wasPressed(KEY_E) && !in.ctrl()) || (in.wasPressed(KEY_C) && !in.ctrl()) || (ended && enterKey(in))) {
        state = GameState::SPACE;
        if (ended) { probeRelease(); status("BACK IN THE CABIN", 3); }
        else status("THE PROBE FALLS ON - C WATCHES IT", 5);
        audio.beep = BEEP_UI;
        return;
    }
    if (saveKey(in)) { saveSlot(currentSlot); return; }
    if (in.wasPressed(KEY_N) && !in.ctrl()) {   // X-04: the storm it falls into, named
        if (probe.stormKey.empty() || probe.clock < probeDescentStart()) { refuse(probe.stormKey.empty() ? "THE PROBE FALLS INTO NO STORM" : "THE STORM IS NAMED ONCE THE PROBE IS IN ITS CLOUDS", 3); }
        else { textStormKey = probe.stormKey; guideReturn = GameState::PROBE; const std::string n = probeStormName(); beginTextEntry("NAME THE STORM", TEXT_STORM, n.empty() ? n : upper(n)); }
        return;
    }
    if (ended) return;
    double rate = 0.0032 * settings.mouseSensitivity;
    probe.camYaw += in.mouseDx * rate;
    probe.camPitch -= in.mouseDy * rate * (settings.invertY ? -1 : 1);
    if (in.isDown(KEY_LEFT)) probe.camYaw -= 1.0 * realDt;
    if (in.isDown(KEY_RIGHT)) probe.camYaw += 1.0 * realDt;
    if (in.isDown(KEY_UP)) probe.camPitch += 0.7 * realDt;
    if (in.isDown(KEY_DOWN)) probe.camPitch -= 0.7 * realDt;
    if (in.wasPressed(KEY_X) && !in.ctrl()) { probe.camYaw = 0; probe.camPitch = 0; }
    probe.camYaw = wrapAngle(probe.camYaw); probe.camPitch = clampd(probe.camPitch, -75 * DEG, 85 * DEG);
    if (in.wasPressed(KEY_SPACE)) {
        double bar = probeBarAt(probe.flight, probe.clock);
        if (probe.clock < probeDescentStart()) { refuse("THE CHUTE OPENS IN THE AIR, AFTER THE ENTRY", 3); }
        else if (probe.flight.chuteOpen < 0) {
            probe.flight.chuteOpen = probe.clock; probe.flight.chuteClose = probeChuteCloseAt(probe.flight, probe.clock);
            status(fmt("CHUTE OPEN AT %.2f ATM - SPACE CUTS IT", atmOf(bar)), 4); audio.beep = BEEP_UI;
        } else if (probe.clock < probe.flight.chuteClose) {
            probe.flight.chuteClose = probe.clock; probe.tornNoted = true;
            status(fmt("CHUTE CUT AT %.2f ATM", atmOf(bar)), 3); audio.beep = BEEP_UI;
        } else { refuse("THE CHUTE IS GONE", 3); }
    }
}

// X-03: the screen's stage name (a deck by its cloud)
std::string Game::probeStageLabel(double c) const {
    int st = probe.endClock >= 0 && c >= probe.endClock ? PS_END : probeStageAt(probe.atm, probe.flight, c);
    return st >= PS_ABOVE && st <= PS_DEEP ? giantStageName(probe.atm, probeBarAt(probe.flight, c)) : PROBE_STAGE_NAMES[st];
}

void Game::renderProbe() {
    if (!probe.active || probe.body < 0 || probe.body >= (int)sys.bodies.size()) { renderSpace(); return; }
    if (probe.endClock >= 0) {   // the last frame, held under the hiss
        if (probe.lastFrame.size() != rgbBuf.size()) { renderProbeFeed(probe.endClock, probe.endT); probe.lastFrame = rgbBuf; }
        rgbBuf = probe.lastFrame;
        probeHiss();
    } else renderProbeFeed(probe.clock, t);
    if (!photoMode && !ax.on) renderProbeHUD();   // R-419: the picture alone while auto-explore watches
}

// the last frame's hiss over rgbBuf (the end held on the screen)
void Game::probeHiss() {
    uint32_t rs = (uint32_t)(realTime * 30) * 2654435761u + 0x10557u;
    uint8_t* o = (uint8_t*)rgbBuf.data();
    int roll = (int)(std::fmod(realTime * 0.23, 1.0) * FBH);
    for (int y = 0; y < FBH; y++) {
        int band = std::abs(y - roll) < 6 * FB_SCALE ? 26 : 0;
        for (int x = 0; x < FBW; x++) {
            size_t i = (size_t)y * FBW + x;
            rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
            int d = (int)(rs % 49) - 24 + band;
            for (int k = 0; k < 3; k++) o[i * 4 + k] = (uint8_t)clampi(o[i * 4 + k] * 7 / 8 + d, 0, 255);
        }
    }
}

// the camera's frame: CAM 05 PROBE with the time since the launch, the stage, the EPOC and the giant, the signal's bar; the
// readouts (the fall's range, the entry's speed, then the depth, the pressure, the temperature, the wind, the chute); the keys.
// The end: SIGNAL LOST and the numbers it was lost at. X-04: a recording's (`rec`: at its clock and its time) says PROBE 3 and
// whether it plays, the storm it fell into under the giant's name, the depth bar on the right with the layers it found
void Game::renderProbeHUD(const ProbeRecord* rec, double cRec, double tt) {
    const uint32_t fc = HUD_WHITE, fd = HUD_DIM;
    drawVisor(fd);
    {
        int m = 14, l = 26;
        drawLineRGB(canvas, m, m, m + l, m, fc); drawLineRGB(canvas, m, m, m, m + l, fc);
        drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m - l, m, fc); drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m, m + l, fc);
        drawLineRGB(canvas, m, UH - 1 - m, m + l, UH - 1 - m, fc); drawLineRGB(canvas, m, UH - 1 - m, m, UH - 1 - m - l, fc);
        drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m - l, UH - 1 - m, fc); drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m, UH - 1 - m - l, fc);
    }
    const GiantAtmosphere& a = probe.atm;
    if (!rec) tt = t;
    const bool ended = rec ? cRec >= rec->endClock : probe.endClock >= 0;
    const double c = rec ? std::min(cRec, rec->endClock) : (ended ? probe.endClock : probe.clock);
    int mins = (int)(c / 60), secs = (int)std::fmod(c, 60.0);
    std::string cam = rec ? fmt("PROBE %d  T+%02d:%02d", rec->id, mins, secs) : fmt("CAM 05 PROBE  T+%02d:%02d", mins, secs);
    drawTextShadow(canvas, 18, 16, cam.c_str(), fc, HUD_SHADOW);
    if (rec) { if (!ended) drawTextShadow(canvas, 18 + textWidth(cam.c_str()) + 6, 16, replay.playing ? "PLAYING" : "PAUSED", replay.playing ? HUD_GREEN : HUD_AMBER, HUD_SHADOW); }
    else if (!ended && ((int)(realTime * 2) & 1)) drawTextShadow(canvas, 18 + textWidth(cam.c_str()) + 6, 16, "\x07REC", HUD_RED, HUD_SHADOW);
    int stage = ended ? PS_END : probeStageAt(a, probe.flight, c);
    std::string st = stage == PS_FALL ? fmt("FALLING TO %s", trunc(upper(bodyNameOf(probe.body)), 20).c_str()) : (ended ? PROBE_STAGE_NAMES[PS_END] : probeStageLabel(c));
    if (rec && ended && rec->endCause == 3) st = "THE RELAY WAS CUT";
    if (!ended && stage >= PS_ABOVE && stage <= PS_DEEP && !photoMode) {   // the layer the picture shows (a great storm's dome lifts a deck over its pressure; a hole takes one away)
        const ProbeViewStats& vs = probeViewStats();
        if (vs.inHole) st = "A CLEAR-AIR HOLE";
        else if (vs.fog >= 0 && vs.fog < a.decks) st = CLOUD_SPECIES[a.species[vs.fog]].deckName;
    }
    drawTextShadow(canvas, 18, 24, st.c_str(), ended ? HUD_RED : HUD_CYAN, HUD_SHADOW);
    const bool fast = rec ? replay.playing && replay.fast : probe.fast;
    if (!ended && fast) drawTextShadow(canvas, 18 + textWidth(st.c_str()) + 8, 24, "FAST", HUD_AMBER, HUD_SHADOW);
    const std::string ep = epocOf(tt);
    drawTextShadow(canvas, UW - 8 - textWidth(ep.c_str()), 8, ep.c_str(), HUD_GREEN, HUD_SHADOW);
    std::string gn = trunc(upper(bodyNameOf(probe.body)), 20);
    drawTextShadow(canvas, UW - 8 - textWidth(gn.c_str()), 16, gn.c_str(), fd, HUD_SHADOW);
    if (!probe.stormKey.empty() && c >= probeDescentStart()) {   // the storm it fell into (its name when it has one), under the stage (the warp's line is at the right)
        std::string sn = nameOfKey(probe.stormKey);
        sn = sn == "UNKNOWN" ? (probe.stormKey.substr(probe.stormKey.size() - 2) == "TG" ? "IN THE GREAT STORM" : "IN A STORM") : "IN " + trunc(upper(sn), 26);
        drawTextShadow(canvas, 18, 32, sn.c_str(), HUD_GREEN, HUD_SHADOW);
    }
    double bar = probeBarAt(probe.flight, c);
    double sig = ended ? 0 : 1 - (c >= probeDescentStart() ? clampd(std::log(std::max(bar, 1e-3) / 0.5) / std::log(probe.flight.crushBar / 0.5), 0, 1) : 0.0);
    {   // the signal's bar
        int bx = UW - 8 - 60, by = 34;   // under the warp's line
        drawTextShadow(canvas, bx - textWidth("SIGNAL") - 4, by - 1, "SIGNAL", fd, HUD_SHADOW);
        drawRectRGB(canvas, bx, by, bx + 60, by + 4, fd);
        int w = (int)(58 * sig);
        if (w > 0) fillRectRGB(canvas, bx + 1, by + 1, bx + w, by + 3, sig > 0.35 ? HUD_GREEN : (sig > 0.15 ? HUD_AMBER : HUD_RED));
    }
    if (rec) {   // the depth bar: the fall and the entry over its first tenth, then the pressure's logarithm to the end, the layers found in their colours
        const int bx = UW - 22, y0 = 46, y1 = UH - 58;
        auto yOf = [&](double u) { return y0 + (int)std::lround(clampd(u, 0, 1) * (y1 - y0)); };
        fillRectRGB(canvas, bx, y0, bx + 5, yOf(0.1), rgb(70, 70, 80));
        const ProbeFlight f{rec->chuteOpen, rec->chuteClose, rec->crushBar};
        for (size_t i = 0; i + 2 < rec->layers.size(); i += 3) {
            const int L = (int)rec->layers[i];
            const int ya = yOf(recordDepthU(*rec, clockAtBar(f, rec->layers[i + 1]))), yb = yOf(recordDepthU(*rec, std::min(rec->endClock, clockAtBar(f, rec->layers[i + 2]))));
            fillRectRGB(canvas, bx, ya, bx + 5, std::max(ya, yb), layerColour(a, L));
        }
        drawRectRGB(canvas, bx - 1, y0 - 1, bx + 6, y1 + 1, fd);
        const int yp = yOf(recordDepthU(*rec, c));
        for (int k = 0; k < 3; k++) drawLineRGB(canvas, bx - 6 + k, yp - 2 + k, bx - 6 + k, yp + 2 - k, fc);   // the playhead
    }
    if (ended) {
        const double eKm = rec ? rec->endKm : probe.endKm, eBar = rec ? rec->endBar : probe.endBar, eK = rec ? rec->endK : probe.endK;
        const int cause = rec ? rec->endCause : probe.endCause;
        if ((int)(realTime * 1.5) & 1) drawTextCentered(canvas, UW / 2, UH / 2 - 16, cause == 3 ? "RELAY CUT" : "SIGNAL LOST", HUD_RED);
        drawTextCentered(canvas, UW / 2, UH / 2 - 4, (eKm >= 0 ? fmt("%.0f KM DOWN  %.1f ATM  %+.0f C", eKm, atmOf(eBar), eK - 273.15) : fmt("%.0f KM UP  %.4f ATM", -eKm, atmOf(eBar))).c_str(), fc);
        drawTextCentered(canvas, UW / 2, UH / 2 + 5,
                fmt(rec ? "%d MIN %02d S - %s" : "%d MIN %02d S AFTER THE LAUNCH - %s", mins, secs, cause == 1 ? "THE HULL GAVE WAY" : (cause == 2 ? "THE HEAT TOOK IT" : "THE SHIP LEFT")).c_str(),
                fd);   // (a recording's clear of its depth bar)
        drawTextShadow(canvas, 18, UH - 26, rec ? "SPACE PLAYS AGAIN  LEFT DEPTH  ESC GALLERY" : "ENTER / ESC: BACK TO THE CABIN   P PHOTO", fd, HUD_SHADOW);
        drawCommonHUD(!rec);
        return;
    }
    std::string l1, l2, l3; uint32_t c3 = fd;
    if (stage == PS_FALL) {
        Vec3 up; Vec3 pos = probeWorldPos(c, tt, up);
        double h = length(pos - sys.bodyPos(probe.body, tt)) - sys.bodies[probe.body].radiusKm - giantAltitudeKm(a, PROBE_TOP_BAR);
        l1 = fmt("%s TO THE CLOUD TOPS", distanceString(std::max(0.0, h)).c_str());
        l2 = fmt("ENTRY IN %.0f S", std::max(0.0, PROBE_FALL_S - c));
        l3 = fmt("AIMED AT %s", latLonString(probe.aimLat, probe.aimLon).c_str());
    } else if (stage == PS_ENTRY) {
        double u = (c - PROBE_FALL_S) / PROBE_ENTRY_S;
        double vEsc = std::sqrt(2 * a.gravity * sys.bodies[probe.body].radiusKm * 1000) / 1000;
        double v = vEsc * std::pow(1 - smoothstep(0.0, 1.0, u), 2) + 0.3;
        l1 = fmt("ENTRY %.1f KM/S  %.0f G", v, 230 * std::pow(std::sin(PI * u), 2) + 1);
        l2 = fmt("ALT %.0f KM  %.4f ATM", giantAltitudeKm(a, bar), atmOf(bar));
        l3 = "THE HEAT SHIELD TAKES IT"; c3 = HUD_AMBER;
    } else {
        double z = giantAltitudeKm(a, bar);
        double tK = giantTemperatureK(a, bar), w = giantWindEast(a, probe.aimLat, bar) * (1 + 0.12 * std::sin(c * 0.7) * std::sin(c * 0.23));
        l1 = fmt("PRESS %.2f ATM  %s %.1f KM", atmOf(bar), z >= 0 ? "ALT" : "DEPTH", std::fabs(z));
        l2 = fmt("TEMP %+.0f C  WIND %.0f KT %s", tK - 273.15, std::fabs(w) * 1.94384, w >= 0 ? "E" : "W");
        bool chuteOpen = probe.flight.chuteOpen >= 0 && c >= probe.flight.chuteOpen && c < probe.flight.chuteClose;
        if (chuteOpen) { l3 = rec ? "CHUTE OPEN" : "CHUTE OPEN - SPACE CUTS IT"; c3 = HUD_CYAN; }
        else if (probe.flight.chuteOpen >= 0 && c >= probe.flight.chuteOpen) l3 = "THE CHUTE IS GONE";
        else l3 = rec ? "FALLING FREE" : "SPACE OPENS THE CHUTE";
    }
    drawTextShadow(canvas, 18, UH - 50, l1.c_str(), fc, HUD_SHADOW);
    drawTextShadow(canvas, 18, UH - 42, l2.c_str(), HUD_GREEN, HUD_SHADOW);
    drawTextShadow(canvas, 18, UH - 34, l3.c_str(), c3, HUD_SHADOW);
    const bool nameable = !probe.stormKey.empty() && c >= probeDescentStart();
    drawTextShadow(canvas, 18, UH - 26, rec ? (nameable ? "ARROWS DEPTH  SPACE PAUSES  N NAMES  ESC BACK" : "ARROWS DEPTH  SPACE PAUSES  SHIFT FAST  ESC BACK")   // 49 characters at most from x 18
                                            : (nameable ? "N NAMES THE STORM  SHIFT FAST  P PHOTO  ESC CABIN" : "MOUSE LOOKS  SHIFT FAST  P PHOTO  ESC CABIN"), fd, HUD_SHADOW);
    drawCommonHUD(!rec);
}

// the relay's sound levels for this frame
// the relay's sound: the screen's when it is watched, a murmur from the landing screen in the cabin while it falls (X-04: a
// recording's on its screen)
void Game::probeAudio() {
    if (state == GameState::RECORDING) { recordingAudio(); return; }
    if (!probe.active || !probeHere()) { audio.probe = 0; audio.probeLost = false; return; }
    probeSound(probe.clock, state == GameState::PROBE, probe.endClock >= 0);
}

// X-04: the relay's levels at a clock (the live probe's, a recording's)
void Game::probeSound(double c, bool watching, bool ended) {
    audio.probe = watching ? 1.0 : (ended ? 0.0 : 0.12);
    audio.probeLost = ended;
    double bar = probeBarAt(probe.flight, c);
    int stage = ended ? PS_END : probeStageAt(probe.atm, probe.flight, c);
    audio.probeStage = stage;
    audio.probeHeat = stage == PS_ENTRY ? std::pow(std::sin(PI * (c - PROBE_FALL_S) / PROBE_ENTRY_S), 1.2) : 0.0;
    bool chute = probe.flight.chuteOpen >= 0 && c >= probe.flight.chuteOpen && c < probe.flight.chuteClose;
    audio.probeChute = chute && !ended ? 1.0 : 0.0;
    double air = stage >= PS_ABOVE && !ended ? smoothstep(std::log(PROBE_TOP_BAR), std::log(0.3), std::log(std::max(bar, 1e-6))) : 0.0;
    audio.probeWind = air * (chute ? 0.55 : 1.0) * (probe.atm.ch.kind == GK_ICE ? 0.6 : 1.0);   // an ice giant's fall the quieter
    audio.probeDepth = ended ? 0.0 : clampd(std::log(std::max(bar, 1e-3) / probe.atm.base[0]) / std::log(probe.flight.crushBar / probe.atm.base[0]), 0, 1);
    audio.probeSignal = ended ? 0.0 : 1 - (c >= probeDescentStart() ? clampd(std::log(std::max(bar, 1e-3) / 0.5) / std::log(probe.flight.crushBar / 0.5), 0, 1) : 0.0);
    if (!ended) {   // a thunderclap at a flash's start, heard through the relay (faint from a flash far off)
        double px[GIANT_MAX_DECKS], pz[GIANT_MAX_DECKS] = {0, 0, 0};
        for (int k = 0; k < GIANT_MAX_DECKS; k++) px[k] = -probeDeckDrift(k, c);
        Flash fl = flashAt(probe.atm, probe.flight, probe.seed, c, probe.bands.valid() ? &probe.decks : nullptr, probe.descYaw + probe.camYaw, giantAltitudeKm(probe.atm, std::max(bar, 1e-9)), px, pz);
        if (fl.slot >= 0 && fl.slot != probe.lastFlashSlot && fl.strength > 0.05) {
            probe.lastFlashSlot = fl.slot;
            if (watching) audio.probeThunder = (0.4 + 0.6 * fl.strength) * (probe.bands.valid() ? clampd(8 / (length(fl.pos) + 2), 0.12, 1.0) : 1.0);
        }
    }
}
