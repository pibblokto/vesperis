// X-01 (2026-10-07): the probe. C at a giant (the landing screen in the cabin, the flight computer's deploy row) sends a probe
// to the ground under the telescope's reticle, else under the ship; the screen becomes its camera (CAM 05 PROBE): the fall
// from the ship with the giant growing, the entry's plasma, the descent through the decks with the depth, the pressure, the
// temperature and the wind, the chute (Space, once: the pressure grows four and a half times under it at a quarter of the
// pace, so a layer is lingered in), photographs (P), and the end: the last frame held under the loss-of-signal hiss until
// the screen is left. The relay's clock is real time (galaxy/probe.h): the warp turns the sky and not the fall; Shift held
// on the screen runs it eight times as fast (R-406). The ship holds its parking meanwhile; leaving it cuts the relay.
// X-02 (2026-10-07): from the entry's peak the picture is the probe's own (space/probe_view.h): the decks round the aim
// point as fields (galaxy/probe.h: the globe's bands sampled at the launch, the towers, the pouches, the fog inside), the
// sky's in-scatter over the space renderer's frame without the giant (the stars, the sun's disc, the moons), the rings'
// arc, the lightning as places in the decks.
// X-03 (2026-10-07): each giant its own (galaxy/probe.h's character): one to three decks of its own clouds and colours, its
// storms and lightning (an ice giant's quiet), the deep's glow (a young giant's, a brown dwarf's from below everything), the
// polar aurorae's curtains, a great storm's wall, the clear-air hole a descent may fall through, a carbon-rich giant's hail,
// and on one rare giant something that passes the lamp once in the deep.
// X-04 (2026-10-07): what comes back. The probe is lost; its record is kept in the guide (galaxy-side, the descent is a function of
// the probe's numbers): the launch, the explorer's turns of the camera and the game's clock along the fall, the end, the profile it
// measured (the pressure, the temperature, the wind, the light, the lightning, the layers named by what it found) and its final
// image (shots/probe_<n>.png). The gallery plays the recording on the screen (`GameState::RECORDING`: the descent drawn again,
// scrubbed by depth), the giant's data sheet draws the profiles, the log keeps what it crossed with the depths, the statistics the
// probes sent and the deepest, the export lends the recordings like C-14's shards. The storm it fell into can be named (N).
#include "game.h"
#include "ui.h"
#include "core/rng.h"
#include "space/probe_view.h"
#include "core/parallel.h"
#include "core/noise.h"
#include "core/png.h"
#include "core/fs.h"
#include <cmath>
#include <ctime>
#include <cstring>
#include <algorithm>

namespace {
const double FLASH_SLOT = 0.5;                        // the lightning's slots (seconds of the relay's clock)
const double HANDOVER0 = 0.62, HANDOVER1 = 0.92;       // X-02: the entry's share over which the probe's own picture takes over from the space renderer's (the plasma at its height)
inline double atmOf(double bar) { return bar * 0.98692; }
// the local frame at a radial direction: east (along the turn), north (toward the spin axis's pole)
void localFrame(const Vec3& up, const Vec3& axis, Vec3& E, Vec3& N) {
    Vec3 e = cross(axis, up);
    if (length(e) < 1e-6) e = cross(Vec3(0, 1, 0), up);
    if (length(e) < 1e-6) e = cross(Vec3(1, 0, 0), up);
    E = normalize(e); N = cross(up, E);
}
std::string latLonString(double lat, double lon) {
    return fmt("%.1f%c %s %.1f%c %s", std::fabs(lat / DEG), CH_DEGREE, lat >= 0 ? "N" : "S", std::fabs(lon / DEG), CH_DEGREE, lon >= 0 ? "E" : "W");
}
// a flash in a lightning slot: its strength now (0 none), its slot, and (given the decks) where it is (local, km from the
// probe) and whether a channel crosses the clear air from it. X-03: lightning lives in the convective decks (water, the salts):
// a glow under the clouds seen from over them, close by in the fog of a deck, in the towers of a convective deck under the
// clear band (a channel up from one in three; under a quiet deck a glow), over the deep; as often as the giant's rate
struct Flash { double strength = 0; int slot = -1; Vec3 pos; bool bolt = false; Vec3 boltTo; uint64_t seed = 0; };
Flash flashAt(const GiantAtmosphere& a, const ProbeFlight& f, uint64_t seed, double clock, const GiantDecks* D, double yaw, double zp, const double* px, const double* pz) {
    Flash F;
    if (clock < probeDescentStart()) return F;
    const double bar = probeBarAt(f, clock);
    const int st = giantStageAt(a, bar), kc = giantStormDeckFrom(a, bar), kin = giantDeckAt(a, bar), kb = giantDeckAbove(a, bar) + 1;
    double odds = 0;
    switch (st) {
        case PS_ABOVE: odds = kc >= 0 ? 0.02 : 0; break;
        case PS_HAZE: odds = kc >= 0 ? 0.03 : 0; break;
        case PS_DECK1: case PS_DECK2: odds = kin == kc ? 0.1 : 0.03; break;
        case PS_BETWEEN: odds = kc == kb ? 0.16 : (kc >= 0 ? 0.05 : 0.01); break;
        case PS_DEEP: odds = 0.05; break;
        default: break;
    }
    odds *= a.ch.lightning;
    int slot = (int)std::floor(clock / FLASH_SLOT);
    uint64_t h = mix64(seed ^ ((uint64_t)slot * 0x9E3779B97F4A7C15ULL) ^ 0xF1A5ULL);
    if (unitFromHash(h) >= odds) return F;
    double t0 = slot * FLASH_SLOT + unitFromHash(mix64(h ^ 1)) * FLASH_SLOT * 0.5, age = clock - t0;
    if (age < 0 || age > 0.45) return F;
    F.slot = slot; F.seed = h;
    double flick = 0.6 + 0.4 * std::cos(age * 60) * std::cos(age * 23);   // the strokes of one flash
    F.strength = (0.5 + 0.5 * unitFromHash(mix64(h ^ 4))) * std::exp(-age * 7) * flick;
    if (!D) return F;
    double u = unitFromHash(mix64(h ^ 2)), w = unitFromHash(mix64(h ^ 3));
    const bool betweenOver = st == PS_BETWEEN && kb < D->n;
    double az = yaw + (u - 0.5) * 2 * (betweenOver ? 25 : 55) * DEG, r, y;
    switch (st) {
        case PS_ABOVE: case PS_HAZE: r = 40 + 160 * w; break;
        case PS_DECK1: case PS_DECK2: r = 1.0 + 4 * w; break;
        case PS_BETWEEN: r = (35 + 60 * w) * D->sh; break;   // where the camera sees the floor
        default: r = 4 + 20 * w; break;
    }
    const double x = std::sin(az) * r, z = std::cos(az) * r;
    switch (st) {
        case PS_ABOVE: case PS_HAZE: y = D->top(0, x + px[0], z + pz[0], 1.0) - 3 * D->sh; break;
        case PS_DECK1: case PS_DECK2: y = zp + (unitFromHash(mix64(h ^ 5)) - 0.5) * 4 * D->sh; break;
        case PS_BETWEEN: y = betweenOver ? D->top(kb, x + px[kb], z + pz[kb], 1.0) - (kb == kc ? 2 : 3) * D->sh : zp - 20 * D->sh; break;
        default: y = D->lvBase[D->n - 1] + 1.5 * D->sh; break;
    }
    F.pos = Vec3(x, y - zp, z);
    if (betweenOver && kb == kc && kb > 0 && unitFromHash(mix64(h ^ 6)) < 0.35) {
        double ba = az + (unitFromHash(mix64(h ^ 7)) - 0.5) * 2.0, run = (3 + 9 * unitFromHash(mix64(h ^ 8))) * D->sh;
        F.bolt = true;
        F.boltTo = F.pos + Vec3(std::sin(ba) * run, (D->lvBase[kb - 1] - y) * (0.3 + 0.5 * unitFromHash(mix64(h ^ 9))), std::cos(ba) * run);
    }
    return F;
}
// the clock a pressure is reached at (the descent's, by halves)
double clockAtBar(const ProbeFlight& f, double bar) {
    double lo = probeDescentStart(), hi = 1e5;
    for (int i = 0; i < 70; i++) { double m = 0.5 * (lo + hi); if (probeBarAt(f, m) < bar) lo = m; else hi = m; }
    return 0.5 * (lo + hi);
}
// X-04: the flashes over a stretch of the clock (each slot's flash, its strength looked at past its start)
int flashCount(const GiantAtmosphere& a, const ProbeFlight& f, uint64_t seed, double c0, double c1) {
    int n = 0;
    for (int slot = (int)std::floor(c0 / FLASH_SLOT); slot * FLASH_SLOT < c1; slot++) {
        const double c = (slot + 0.52) * FLASH_SLOT;   // just past the latest start a slot's flash can have (half the slot in), within its glow
        if (c > c0 && c <= c1 && flashAt(a, f, seed, c, nullptr, 0, 0, nullptr, nullptr).strength > 0) n++;
    }
    return n;
}
enum ProbeNote { PN_STORM = 1, PN_WALL = 2, PN_AURORA = 4, PN_HOLE = 8, PN_HAIL = 16 };   // X-04: the discoveries logged once each
// X-04: a layer's colour on a recording's depth bar and on the profile's strip (a deck its cloud's)
uint32_t layerColour(const GiantAtmosphere& a, int L) {
    if (L >= GIANT_HOLE_LAYER) return rgb(90, 200, 220);
    if (L == 0) return rgb(40, 50, 100);
    if (L == 1) return rgb(170, 130, 70);
    const int k = (L - 2) / 2;
    if (L & 1) return k >= a.decks - 1 ? rgb(120, 40, 28) : rgb(55, 66, 84);
    const RGB c = CLOUD_SPECIES[a.species[std::min(k, GIANT_MAX_DECKS - 1)]].color;
    return rgb((int)(c.r * 220), (int)(c.g * 220), (int)(c.b * 220));
}
const double CAM_KEY = 0.5 * DEG;       // X-04: the camera's track keeps a turn of half a degree
const double TIME_KEY = 0.5;            // and the clock's a drift of half a second of game time
const double PROBE_FAST = 8;            // R-406: Shift's pace
const double BANDS_KM = 4800;           // X-02: the bands' reach round the aim (the horizon from the entry's end)
const int BANDS_N = 160;
}

bool Game::probeHere() const {
    return probe.active && sys.valid && sys.star.seed == probe.starSeed && ship.mode == ShipState::PARKED && ship.parkedBody == probe.body && ship.parkedBelt < 0 &&
           probe.body >= 0 && probe.body < (int)sys.bodies.size();
}

int Game::testProbeGiant() const {
    if (!sys.valid) return -1;
    for (int i = 0; i < (int)sys.bodies.size(); i++) if (sys.bodies[i].type == PT_GASGIANT) return i;
    return -1;
}

// the ground C sends a probe to: the telescope's reticle on the parked giant, else the ground under the ship
bool Game::probeAimPoint(double& lat, double& lon) const {
    int b = ship.parkedBody;
    if (!sys.valid || ship.mode != ShipState::PARKED || b < 0 || b >= (int)sys.bodies.size() || ship.parkedBelt >= 0) return false;
    if (testAimOverride) { lat = testAimLat; lon = testAimLon; return true; }
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
        // X-03: what the probe was sent to see: a great storm's wall within sight, else the aurora's curtains over the clouds
        double gd, ex, ez;
        if (probeStormWall(gd, ex, ez)) probe.descYaw = std::atan2(ex, ez);
        else if (probeAuroraWay(ex, ez, gd) && std::fabs(gd) < 2800 && probe.atm.ch.aurora > 0.25) probe.descYaw = std::atan2(ex, ez) + (gd < 0 ? PI : 0.0);
    }
    if (tele.on) telescopeOff();
    if (radar.on) radarOff();
    logEvent("PROBE", fmt("PROBE %d INTO %s AT %s", probe.id, upper(bodyNameOf(b)).c_str(), latLonString(lat, lon).c_str()));
    status(aimed ? fmt("PROBE AWAY TO %s", latLonString(lat, lon).c_str()) : "PROBE AWAY, UNDER THE SHIP (THE TELESCOPE AIMS IT)", 5);
    statusNext = "SPACE OPENS THE CHUTE ONCE: WHERE TO LINGER"; statusNextSecs = 5;
    audio.beep = 1;
    state = GameState::PROBE;
}

void Game::watchProbe() {
    if (!probe.active) return;
    state = GameState::PROBE;
    audio.beep = 4;
}

void Game::probeCut(const char* why) {
    if (probe.active && probe.endClock < 0 && probe.body >= 0 && probe.body < (int)sys.bodies.size() && sys.star.seed == probe.starSeed) {   // (a Vimana flight has just set `sys.valid` off: the system is still the giant's)
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
    probeTrack();   // X-04: the tracks and the log's discoveries, to the end
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
        audio.beep = 4;
        return;
    }
    if (saveKey(in)) { saveSlot(currentSlot); return; }
    if (in.wasPressed(KEY_N) && !in.ctrl()) {   // X-04: the storm it falls into, named
        if (probe.stormKey.empty() || probe.clock < probeDescentStart()) { status(probe.stormKey.empty() ? "THE PROBE FALLS INTO NO STORM" : "THE STORM IS NAMED ONCE THE PROBE IS IN ITS CLOUDS", 3); audio.beep = 3; }
        else { textStormKey = probe.stormKey; guideReturn = GameState::PROBE; const std::string n = probeStormName(); beginTextEntry("NAME THE STORM", 9, n.empty() ? n : upper(n)); }
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
        if (probe.clock < probeDescentStart()) { status("THE CHUTE OPENS IN THE AIR, AFTER THE ENTRY", 3); audio.beep = 3; }
        else if (probe.flight.chuteOpen < 0) {
            probe.flight.chuteOpen = probe.clock; probe.flight.chuteClose = probeChuteCloseAt(probe.flight, probe.clock);
            status(fmt("CHUTE OPEN AT %.2f ATM - SPACE CUTS IT", atmOf(bar)), 4); audio.beep = 4;
        } else if (probe.clock < probe.flight.chuteClose) {
            probe.flight.chuteClose = probe.clock; probe.tornNoted = true;
            status(fmt("CHUTE CUT AT %.2f ATM", atmOf(bar)), 3); audio.beep = 4;
        } else { status("THE CHUTE IS GONE", 3); audio.beep = 3; }
    }
}

// the probe's place: in the fall from the ship's place at the launch to the entry over the aim point (the height falling by
// equal ratios, so the giant grows evenly, and the direction swinging from the start's to the aim's), then over the aim point
// at the height of its pressure
Vec3 Game::probeWorldPos(double c, double tt, Vec3& up) const {
    const Body& g = sys.bodies[probe.body];
    Vec3 gp = sys.bodyPos(probe.body, tt);
    Vec3 aimW = normalize(sys.bodyFrame(probe.body, tt).transposed() * StarSystem::bodyFromLatLon(probe.aimLat, probe.aimLon));
    double R = g.radiusKm;
    if (c < PROBE_FALL_S) {
        double u = clampd(c / PROBE_FALL_S, 0, 1);
        double h0 = std::max(length(probe.startRel) - R, 2000.0), h1 = giantAltitudeKm(probe.atm, PROBE_ENTRY_BAR);
        double h = h0 * std::pow(h1 / h0, u);
        up = normalize(lerp(normalize(probe.startRel), aimW, smoothstep(0.0, 1.0, u)));
        return gp + up * (R + h);
    }
    up = aimW;
    return gp + up * (R + giantAltitudeKm(probe.atm, probeBarAt(probe.flight, c)));
}

// X-02: the bands of a probe launched or loaded (the globe's round the aim at the launch), the decks' fields over them. X-03: on
// a giant with clear-air holes the descent falls through one about half the time: a hole in each deck over the last, where the
// probe crosses that deck's top (falling free); on the rare giant, the clock the sight passes the lamp at
void Game::probeEnsureClouds() {
    if (!probe.active || !sys.valid || probe.body < 0 || probe.body >= (int)sys.bodies.size()) return;
    if (!probe.bands.valid()) {
        const Body& g = sys.bodies[probe.body];
        probe.bands = giantBandsOf(g, spaceR.genFor(g), probe.aimLat, probe.aimLon, probe.launchT, BANDS_KM, BANDS_N);
    }
    probe.decks.init(probe.atm, &probe.bands, probe.seed);
    probe.sightClock = -1;
    if (probe.atm.ch.sight) {   // the rare giant: once in the deep (where the sun's light is gone and the lamp is on), at a depth of the probe's own
        const GiantAtmosphere& a = probe.atm;
        const double endBar = probeEndBar(a, probe.flight);
        double dark = endBar;
        for (double l = std::log(a.top[0]); l < std::log(endBar); l += 0.02) if (giantSunlightAt(a, std::exp(l)) < 0.02) { dark = std::exp(l); break; }
        const double lo = std::log(std::max(dark, a.base[0] * 1.3)), hi = std::log(endBar * 0.85);
        if (hi > lo) probe.sightClock = clockAtBar(probe.flight, std::exp(lo + (hi - lo) * (0.3 + 0.6 * unitFromHash(mix64(probe.seed ^ 0x5167ULL)))));   // its clock follows the chute
    }
    probe.holePath = probe.atm.ch.holes > 0 && probe.decks.holeable(0) && unitFromHash(mix64(probe.seed ^ 0x4011EULL)) < 0.55;
    if (probe.holePath) {   // through the first deck, and on through the next while each opens onto a deck close under it
        ProbeFlight free = probe.flight; free.chuteOpen = free.chuteClose = -1;
        const double r = (22 + 30 * unitFromHash(mix64(probe.seed ^ 0x4012EULL))) * probe.decks.sh;
        for (int k = 0; probe.decks.holeable(k); k++) {
            GiantDecks::Hole& h = probe.decks.path[k];
            h.on = true; h.r = r; h.cz = 0;
            h.cx = -probeDeckDrift(k, clockAtBar(free, probe.atm.top[k]));
        }
    }
}

// km a deck has slid east under the probe since the descent began: the shear between the deck's wind (at its middle) and the
// probe's at each moment, summed over the descent
double Game::probeDeckDrift(int k, double c) const {
    const double c0 = probeDescentStart();
    if (c <= c0) return 0;
    const GiantAtmosphere& a = probe.atm;
    double level = k < GIANT_MAX_DECKS ? std::sqrt(a.top[k] * a.base[k]) : 0.3;
    double wDeck = giantWindEast(a, probe.aimLat, level), sum = 0;
    const int n = 48;
    for (int i = 0; i < n; i++) sum += wDeck - giantWindEast(a, probe.aimLat, probeBarAt(probe.flight, c0 + (c - c0) * (i + 0.5) / n));
    return sum * (c - c0) / n * 0.001;
}

// X-03: the great storm's wall within sight of the aim (2,500 km of its edge): the way to its nearest stretch (local, east and
// north: away from the centre inside it, toward it outside) and the edge's distance (km, negative inside)
bool Game::probeStormWall(double& dist, double& ex, double& ez) const {
    const GiantBands& B = probe.bands;
    if (!B.gsOn) return false;
    const double d = B.greatStormAt(0, 0), edge = (d - 1) * std::sqrt(B.gsAKm * B.gsBKm), l = std::hypot(B.gsX, B.gsZ);
    if (std::fabs(edge) > 2500 || l < 1e-6) return false;
    ex = B.gsX / l * (d < 1 ? -1 : 1); ez = B.gsZ / l * (d < 1 ? -1 : 1); dist = edge;
    return true;
}

// X-03: the aurora's way from the aim: the local horizontal toward the nearer magnetic pole and the oval's distance along it (km;
// negative with the aim inside the oval, between it and the pole)
bool Game::probeAuroraWay(double& ex, double& ez, double& p0) const {
    const GiantCharacter& ch = probe.atm.ch;
    if (ch.aurora <= 0 || !sys.valid || probe.body < 0 || probe.body >= (int)sys.bodies.size()) return false;
    const Vec3 aimB = StarSystem::bodyFromLatLon(probe.aimLat, probe.aimLon);
    Vec3 pole = StarSystem::bodyFromLatLon(PI / 2 - ch.magColat, ch.magLon);
    if (dot(aimB, pole) < 0) pole = pole * -1.0;
    const double th = std::acos(clampd(dot(aimB, pole), -1, 1));
    Vec3 E = cross(Vec3(0, 0, 1), aimB);
    E = length(E) < 1e-9 ? Vec3(0, 1, 0) : normalize(E);
    const Vec3 N = cross(aimB, E), tw = pole - aimB * dot(aimB, pole);
    const double l = length(tw);
    ex = l > 1e-9 ? dot(tw, E) / l : 0; ez = l > 1e-9 ? dot(tw, N) / l : 1;
    p0 = (th - ch.ovalRad) * sys.bodies[probe.body].radiusKm;
    return true;
}

std::string Game::probeStageLabel(double c) const {
    int st = probe.endClock >= 0 && c >= probe.endClock ? PS_END : probeStageAt(probe.atm, probe.flight, c);
    return st >= PS_ABOVE && st <= PS_DEEP ? giantStageName(probe.atm, probeBarAt(probe.flight, c)) : PROBE_STAGE_NAMES[st];
}

// the probe's picture at a clock and a game time, into rgbBuf: the space renderer from the probe's place through the fall and
// the entry, handing over to the probe's own picture (space/probe_view.h) as the plasma peaks; the plasma over both; the
// link's static over all of it as the signal weakens
void Game::renderProbeFeed(double c, double tt) {
    const Body& g = sys.bodies[probe.body];
    const GiantAtmosphere& a = probe.atm;
    probeEnsureClouds();
    Vec3 up; Vec3 pos = probeWorldPos(c, tt, up);
    Vec3 E, N; localFrame(up, sys.bodyFrame(probe.body, tt).row(2), E, N);
    Mat3 W = Mat3::fromRows(E, up, N);   // world -> local
    double bar = probeBarAt(probe.flight, c);
    int stage = probeStageAt(a, probe.flight, std::min(c, probeEndClock(a, probe.flight) - 1e-6));
    // the camera: in the fall toward the aim point (the heading held as it comes under the probe), easing in the entry to the
    // heading fourteen degrees under the horizon; the player's turn on top, the chute's swing, the entry's shake
    double yaw = probe.descYaw, pitch = -22 * DEG;   // X-02: high over the clouds the deck fills the frame under a strip of sky
    if (c >= probeDescentStart()) {
        const GiantAtmosphere& A = probe.atm;
        double lb = std::log(std::max(probeBarAt(probe.flight, c), 1e-6));
        pitch = (-22 + 8 * smoothstep(std::log(0.02), std::log(A.hazeBar), lb) + 14 * smoothstep(std::log(A.top[0]), std::log(std::sqrt(A.top[0] * A.base[0])), lb) - 2 * smoothstep(std::log(A.base[0]), std::log(A.base[0] * 1.3), lb)
                 + (A.decks > 1 ? 1 * smoothstep(std::log(A.top[1] * 0.8), std::log(A.top[1] * 1.2), lb) : 0.0)) * DEG;
        if (probe.holePath)   // X-03: falling into a clear-air hole the camera looks down into it, and down the shaft once in it
            pitch -= (30 + 26 * smoothstep(std::log(A.top[0]), std::log(std::sqrt(A.top[0] * A.base[0])), lb)) * DEG * smoothstep(std::log(A.hazeBar * 0.25), std::log(A.hazeBar), lb) *
                     (1 - smoothstep(std::log(A.top[A.decks - 1] * 0.7), std::log(A.top[A.decks - 1]), lb));
    }
    if (c < probeDescentStart()) {
        Vec3 gp = sys.bodyPos(probe.body, tt);
        Vec3 aimPt = gp + normalize(sys.bodyFrame(probe.body, tt).transposed() * StarSystem::bodyFromLatLon(probe.aimLat, probe.aimLon)) * g.radiusKm;
        double u = clampd(c / PROBE_FALL_S, 0, 1);
        Vec3 to = W * normalize(lerp(normalize(gp - pos), normalize(aimPt - pos), smoothstep(0.0, 0.6, u)));
        double h = std::max(1.0, length(pos - gp) - g.radiusKm), dip = std::acos(g.radiusKm / (g.radiusKm + h));   // the limb's angle under the horizontal
        double fp = clampd(std::asin(clampd(to.y, -1, 1)), -(dip + 12 * DEG), 20 * DEG);   // never steeper than a little under the limb: the whole giant at first, then its curve and its sky in the frame as it rushes up
        double fy = std::atan2(to.x, to.z);
        double hold = smoothstep(-20 * DEG, -60 * DEG, fp);
        double fallYaw = probe.headYaw + wrapAngle(fy - probe.headYaw) * (1 - hold);
        double e = c < PROBE_FALL_S ? 0 : smoothstep(0.25, 0.9, (c - PROBE_FALL_S) / PROBE_ENTRY_S);
        yaw = fallYaw + wrapAngle(probe.descYaw - fallYaw) * e;
        pitch = fp + (-22 * DEG - fp) * e;
    }
    if (probe.flight.chuteOpen >= 0 && c >= probe.flight.chuteOpen && c < probe.flight.chuteClose) {   // the swing under the canopy
        yaw += 3 * DEG * std::sin(c * 0.37); pitch += 2 * DEG * std::sin(c * 0.9);
    }
    double shake = 0;
    if (c >= PROBE_FALL_S && c < probeDescentStart()) shake = std::sin(PI * (c - PROBE_FALL_S) / PROBE_ENTRY_S);
    else if (c >= probeDescentStart()) shake = 0.15 * smoothstep(0.3, 1.0, bar) * (probe.flight.chuteOpen >= 0 && c < probe.flight.chuteClose ? 0.5 : 1.0);
    if (shake > 0 && !testProbeQuiet) { yaw += shake * 0.6 * DEG * std::sin(c * 37.0) ; pitch += shake * 0.8 * DEG * std::sin(c * 53.0 + 1.3); }
    yaw += probe.camYaw; pitch = clampd(pitch + probe.camPitch, -88 * DEG, 88 * DEG);
    Mat3 camLocal = cameraBasis(yaw, pitch);
    Mat3 camW = camLocal * W;
    Proj pj = Proj::fromHFov(settings.fovDeg);
    const double mixIn = c < PROBE_FALL_S ? 0.0 : (c < probeDescentStart() ? smoothstep(HANDOVER0, HANDOVER1, (c - PROBE_FALL_S) / PROBE_ENTRY_S) : 1.0);
    static std::vector<uint32_t> spacePic, skyPic;
    if (mixIn < 1) {   // the fall and the entry: the space renderer from the probe's place, the giant and all
        spaceR.proj = pj;
        int other = -1; double best = 0;
        for (int i = 0; i < (int)sys.bodies.size(); i++) if (i != probe.body) { double ang = sys.bodies[i].radiusKm / std::max(1.0, length(sys.bodyPos(i, tt) - pos)); if (ang > best) { best = ang; other = i; } }
        if (best < 1.2 / pj.f) other = -1;
        spaceR.setupPalette(fb, &sys, probe.body, other, 1);
        SpaceContext sc;
        sc.sys = &sys; sc.stars = &nb.stars; sc.t = tt; sc.shipPos = pos; sc.cam = camW;
        sc.bankBodyA = probe.body; sc.bankBodyB = other; sc.detailBody = -1;
        spaceR.render(fb, sc);
        fb.mush(2);
        fb.toRGB(rgbBuf.data(), 1.0, settings.dither);
        if (mixIn > 0) spacePic = rgbBuf;
    }
    if (mixIn > 0) {   // the probe's own picture
        const double zp = giantAltitudeKm(a, std::max(bar, 1e-9));
        const Vec3 sunW = normalize(sys.star.pos - pos), sunL = W * sunW;
        const double lf = SpaceRenderer::lightFactor(sys.star.luminosity, length(sys.star.pos - pos));
        const BodyGen& gg = spaceR.genFor(g);
        const GiantCharacter& ch = a.ch;
        const int nd = a.decks, last = nd - 1;
        const bool ice = ch.kind == GK_ICE;
        const RGB white(1, 1, 1), lightCol = lerp(sys.star.color, white, 0.5f);
        const RGB band = gg.matColor[familyRep(g.type, FAM_ROCK)] * lightCol;   // the globe's colour under the star's light
        auto lpOf = [](double b) { return std::log(std::max(b, 1e-9)); };
        const double lp = lpOf(bar);
        double px[GIANT_MAX_DECKS], pz[GIANT_MAX_DECKS] = {0, 0, 0};
        for (int k = 0; k < GIANT_MAX_DECKS; k++) px[k] = -probeDeckDrift(k, c);   // the probe's place over each deck (km east of the aim in its frame)
        unsigned cut = 0;   // X-03: the decks a clear-air hole takes away over the probe
        for (int k = 0; k < last; k++) if (bar > a.top[k] && probe.decks.holeAt(k, px[k], pz[k]) >= 0.5) cut |= 1u << k;
        const double Lp = giantSunlightAt(a, std::max(bar, 1e-9), cut);
        // the camera's gain takes back most of the dark (the light on the decks is given against it); what it cannot take
        // back dims the colours
        // over the clouds it exposes for the deck under it, in a deck for the fog round it, in the clear band for the ceiling
        // (the light come through it), easing into the next deck's own over the band's last stretch; X-03: the deep's glow
        // counts with the sun's (a brown dwarf's clouds are lit from under them as much as from over them)
        const int kAbove = giantDeckAbove(a, bar), kIn = giantDeckAt(a, bar);
        double Lref = bar < a.top[0] ? giantSunlightAt(a, a.top[0], cut) : Lp;
        if (kIn < 0 && kAbove >= 0 && kAbove < last)
            Lref = std::exp(lerpd(std::log(giantSunlightAt(a, a.base[kAbove], cut)), std::log(Lp), smoothstep(lpOf(a.top[kAbove + 1] * 0.7), lpOf(a.top[kAbove + 1]), lp)));
        const double glowRef = bar < a.top[0] ? giantGlowAt(a, a.top[0]) : 0.6 * giantGlowAt(a, bar, cut);
        const double expo = clampd(std::pow(std::max(Lref + glowRef, 1e-6), bar >= a.base[0] ? -0.95 : -0.85), 0.25, 12.0);
        double albAim = 0.55, stormAim = 0;
        probe.bands.at(px[0], 0, albAim, stormAim);
        const double bandGain = std::pow(0.75 / std::max(albAim, 0.2), 0.42);   // and takes back some of a dark belt's dark (the band's colour stays)
        const float colGain = (float)(clampd(std::pow(std::max(Lp + giantGlowAt(a, bar, cut), 1e-6), ice ? 0.06 : 0.1), 0.45, 1.0) * clampd(lf, 0.6, 1.1));   // (an ice giant's blue light carries deeper)
        // the light come down to a pressure, reddened through the decks (an ice giant's: the methane takes the red, the light goes
        // blue-green)
        const RGB tint1 = ice ? RGB(0.66f, 0.9f, 1.0f) : RGB(1.0f, 0.76f, 0.56f), tint2 = ice ? RGB(0.36f, 0.72f, 0.95f) : RGB(0.95f, 0.56f, 0.38f);
        auto tint = [&](double b) {   // (an ice giant's methane blues the light from high in the air)
            RGB t = lerp(white, tint1, (float)smoothstep(lpOf(ice ? 0.005 : a.top[0]), lpOf(a.base[0]), lpOf(b)));
            return lerp(t, tint2, (float)smoothstep(lpOf(a.base[0]), lpOf(a.base[last]), lpOf(b)));
        };
        auto warm = [&](RGB c0, double w) { return ice ? lerp(c0, RGB(c0.r * 0.45f, c0.g * 0.85f, std::min(1.0f, c0.b * 1.08f)), (float)w) : lerp(c0, RGB(c0.r, c0.g * 0.72f, c0.b * 0.5f), (float)w); };
        // a deck's colour: the first is the globe's (what was seen from orbit), the ones under it their clouds' own in its light
        auto deckCol = [&](int k) { return k == 0 ? band : lerp(band, CLOUD_SPECIES[a.species[k]].color * lightCol, 0.55f); };
        ProbeView v;
        v.cam = camLocal; v.pj = pj; v.zKm = zp; v.radiusKm = g.radiusKm; v.atm = &probe.atm; v.decks = &probe.decks;
        for (int k = 0; k < GIANT_MAX_DECKS; k++) { v.px[k] = px[k]; v.pz[k] = pz[k]; }
        v.sun = sunL;
        v.sunVis = std::sqrt(Lp) * (stage <= PS_HAZE || (cut & 1u) ? 1.0 : 0.0);
        for (int k = 0; k < nd; k++) {   // the light on the decks against the exposure: the sun's own on the first's top only, the light coming down through them
            const double top = giantSunlightAt(a, a.top[k], cut), base = giantSunlightAt(a, a.base[k], cut);
            v.direct[k] = k ? 0.0 : 1.2 * top * expo * bandGain;
            v.diffuse[k] = (k ? 1.0 : 0.32 * bandGain) * top * expo;
            v.inside[k] = top * expo; v.under[k] = base * expo;
            v.glowTop[k] = giantGlowAt(a, a.top[k]) * expo; v.glowUnder[k] = giantGlowAt(a, a.base[k]) * expo;
            const RGB dc = deckCol(k);
            v.deck[k].material(dc * tint(a.top[k]), colGain);
            v.below[k].material(dc * tint(a.base[k]) * (k == last ? 0.8f : 0.9f), colGain);
        }
        for (int k = nd; k < GIANT_MAX_DECKS; k++) { v.deck[k] = v.deck[last]; v.below[k] = v.below[last]; }
        v.glowAir = 0.25 * giantGlowAt(a, bar, cut) * expo; v.glowCol = ch.glowCol;
        // the air's looks by the pressure's logarithm: the dark sky high over the clouds, the haze (amber; an ice giant's
        // cyan), each deck's top and base (the clear bands' light reddened under them), the deep's grey (an ice giant's
        // blue, a brown dwarf's ember)
        struct Look { double lp; RGB zen, hor; double zenS, horS; };
        const RGB ink(0.06f, 0.08f, 0.2f);
        std::vector<Look> looks = {
            {lpOf(PROBE_TOP_BAR), lerp(ink, band, 0.12f), lerp(band, white, 0.5f), 12, 50},
            {lpOf(0.02), lerp(ink, band, 0.2f), lerp(band, white, 0.45f), 20, 52},
            {lpOf(a.hazeBar), lerp(lerp(ink, band, 0.2f), warm(band, 1) * 0.9f, 0.6f), warm(lerp(band, white, 0.4f), 0.35), 34, 54},
            {lpOf(a.top[0]), warm(band, 0.8) * 0.95f, warm(lerp(band, white, 0.35f), 0.55), 44, 54}};
        const RGB deepZ = ice ? RGB(0.2f, 0.28f, 0.34f) : (ch.kind == GK_BROWN ? RGB(0.3f, 0.14f, 0.1f) : RGB(0.32f, 0.27f, 0.26f));
        const RGB deepH = ice ? RGB(0.28f, 0.36f, 0.42f) : (ch.kind == GK_BROWN ? RGB(0.42f, 0.2f, 0.14f) : RGB(0.42f, 0.37f, 0.36f));
        for (int k = 0; k < nd; k++) {
            const RGB dc = deckCol(k);
            if (k > 0) looks.push_back({lpOf(a.top[k]), dc * tint(a.top[k]) * 0.7f, lerp(dc, white, 0.15f) * tint(a.top[k]) * 0.9f, 28, 34});
            if (k == last && k > 0) looks.push_back({lpOf(a.base[k]), deepZ, deepH, 30, 36});
            else looks.push_back({lpOf(a.base[k]), dc * tint(a.base[k]) * 0.8f, lerp(dc, white, 0.2f) * tint(a.base[k]), 32, 38});
        }
        for (size_t q = 1; q < looks.size(); q++) looks[q].lp = std::max(looks[q].lp, looks[q - 1].lp + 1e-3);
        size_t k0 = 0;
        while (k0 + 2 < looks.size() && lp > looks[k0 + 1].lp) k0++;
        const float u = (float)clampd((lp - looks[k0].lp) / (looks[k0 + 1].lp - looks[k0].lp), 0, 1);
        const RGB zen = lerp(looks[k0].zen, looks[k0 + 1].zen, u), hor = lerp(looks[k0].hor, looks[k0 + 1].hor, u);
        v.zenShade = looks[k0].zenS + (looks[k0 + 1].zenS - looks[k0].zenS) * u;
        v.horShade = looks[k0].horS + (looks[k0 + 1].horS - looks[k0].horS) * u;
        v.air.set({{0, RGB(0, 0, 0)}, {12, zen * 0.5f}, {26, zen}, {42, lerp(zen, hor, 0.55f)}, {54, hor}, {63, lerp(hor, white, 0.45f)}}, colGain);
        v.ring.material(band, (float)clampd(lf, 0.6, 1.1));
        if (g.rings) {   // the rings over the probe: their plane through the giant's centre, its normal the spin axis
            v.rings = true; v.ringN = W * g.spinAxis; v.ringC = W * (sys.bodyPos(probe.body, tt) - pos);
            v.ringR0 = g.ringInner * g.radiusKm; v.ringR1 = g.ringOuter * g.radiusKm; v.ringProf = &spaceR.ringProfile(sys, probe.body);
            v.ringLit = std::fabs(dot(g.spinAxis, sunW)) * 0.7 + 0.3;
        }
        const double windE = giantWindEast(a, probe.aimLat, bar);
        v.windAz = windE >= 0 ? 90 * DEG : -90 * DEG;
        v.cirrusKm = giantAltitudeKm(a, std::min(0.3, 0.6 * a.top[0]));   // (X-03: over the first deck, whatever its height)
        v.cirrus = 1 - smoothstep(lpOf(a.top[0]), lpOf(a.base[0]), lp);
        v.cirrusRun = 0.25 * a.windPeak * 0.001 * std::max(0.0, c - probeDescentStart());   // the jet's core racing past
        Flash fl = flashAt(a, probe.flight, probe.seed, c, &probe.decks, probe.descYaw + probe.camYaw, zp, v.px, v.pz);
        v.flash = fl.strength; v.flashPos = fl.pos; v.bolt = fl.bolt; v.boltTo = fl.boltTo; v.boltSeed = fl.seed;
        v.lamp = 1 - smoothstep(std::log(0.03), std::log(0.15), std::log(std::max(Lp, 1e-9)));   // where the sun's light is nearly gone
        v.rain = 0;
        for (int k = 0; k < nd; k++) if (CLOUD_SPECIES[a.species[k]].convective && bar >= a.top[k]) v.rain = std::max(v.rain, 0.7 * (1 - smoothstep(a.base[k], a.base[k] * 2.2, bar)));
        v.motes = smoothstep(a.base[last], a.base[last] * 1.6, bar);
        v.heat = 0.35 * std::pow(clampd((giantTemperatureK(a, bar) - 520) / (a.heatK - 520), 0, 1), 2);   // the deep's air near the probe's end glows dull red
        v.hail = ch.hail ? smoothstep(a.top[last], std::sqrt(a.top[last] * a.base[last]), bar) : 0.0;   // falling out of the lowest deck
        {   // the aurora: the curtains over the oval round the nearer magnetic pole, over the clouds (the haze washes them out)
            double ex, ez, p0;
            if (probeAuroraWay(ex, ez, p0)) {
                v.aurora = ch.aurora * expo * 0.8 * (1 - smoothstep(lpOf(a.hazeBar * 0.5), lpOf(a.top[0]), lp));
                v.auroraDir = Vec3(ex, 0, ez); v.auroraP0 = p0;
                v.auroraH0 = giantAltitudeKm(a, 2e-7); v.auroraH = 60 * probe.decks.sh;
                v.auroraLow = ch.auroraLow; v.auroraHigh = ch.auroraHigh;
            }
        }
        if (probe.sightClock > 0 && std::fabs(c - probe.sightClock) < 6) {   // X-03: it passes across the beam, once
            const double su = (c - probe.sightClock) / 6;   // -1 .. 1
            v.sight = std::pow(std::cos(su * PI / 2), 0.7);
            const double hy = probe.descYaw;
            const Vec3 fwd(std::sin(hy), 0, std::cos(hy)), side(std::cos(hy), 0, -std::sin(hy));
            v.sightPos = fwd * 0.2 + side * (0.42 * su) + Vec3(0, -0.06 - 0.02 * su, 0);
            v.sightAxis = side;
        }
        v.time = c; v.seed = probe.seed;
        const uint32_t* space = nullptr;
        if (probeSpaceShows(v) > 0.003) {   // the sky beyond the air: the space renderer without the giant (its rings are the view's)
            int ba = -1, bb = -1; double da = 1e300, db = 1e300;
            for (int i = 0; i < (int)sys.bodies.size(); i++) {
                if (i == probe.body || sys.bodies[i].type == PT_COMPANION) continue;
                double d = length(sys.bodyPos(i, tt) - pos);
                if (d < da) { bb = ba; db = da; ba = i; da = d; } else if (d < db) { bb = i; db = d; }
            }
            spaceR.proj = pj;
            spaceR.setupPalette(fb, &sys, ba, bb, 1);
            SpaceContext sc;
            sc.sys = &sys; sc.stars = &nb.stars; sc.t = tt; sc.shipPos = pos; sc.cam = camW;
            sc.bankBodyA = ba; sc.bankBodyB = bb; sc.detailBody = -1; sc.excludeBody = probe.body;
            sc.starIntensity = 1 - 0.8 * smoothstep(-0.12, 0.12, sunL.y);
            spaceR.render(fb, sc);
            fb.mush(2);
            skyPic.resize(rgbBuf.size());
            fb.toRGB(skyPic.data(), 1.0, settings.dither);
            space = skyPic.data();
        }
        renderProbeView(v, space, rgbBuf.data());
        if (mixIn < 1) {   // the crossfade
            uint8_t* o = (uint8_t*)rgbBuf.data(); const uint8_t* s = (const uint8_t*)spacePic.data();
            int w = (int)(mixIn * 256);
            for (size_t i = 0, n = rgbBuf.size() * 4; i < n; i++) if ((i & 3) != 3) o[i] = (uint8_t)((s[i] * (256 - w) + o[i] * w) >> 8);
        }
    }
    if (c >= PROBE_FALL_S && c < probeDescentStart()) {   // the entry: the plasma round the heat shield, ahead along the fall (down), streaked; the scene through it in its colour
        double u = (c - PROBE_FALL_S) / PROBE_ENTRY_S, heat = std::pow(std::sin(PI * std::min(1.0, u * 1.15)), 1.5);
        ShadeRamp plasma;
        plasma.set({{0, RGB(0, 0, 0)}, {20, RGB(0.45f, 0.12f, 0.03f)}, {40, RGB(1.0f, 0.45f, 0.12f)}, {54, RGB(1.0f, 0.8f, 0.45f)}, {63, RGB(1, 1, 0.92f)}});
        Vec3 dv = camW * (-up);   // the fall's direction in the view: the plasma is brightest round it
        const double f = pj.f, cx = pj.cx, cy = pj.cy;
        double vx = dv.z > 0.05 ? cx + f * dv.x / dv.z : cx, vy = dv.z > 0.05 ? cy - f * dv.y / dv.z : FBH * 2.0;   // its vanishing point (the streaks run from it)
        uint64_t flick = probe.seed ^ (uint64_t)(c * 30);
        static float streaks[1024];   // the streaks by the angle round the vanishing point, this frame's
        for (int i = 0; i < 1024; i++) streaks[i] = (float)(0.75 + 0.25 * gnoise2((i / 1024.0 * TAU - PI) * 14, c * 2.5, flick));
        parallelFor(FBH, 8, [&](int y0, int y1) {
            float o[3];
            for (int y = y0; y < y1; y++) for (int x = 0; x < FBW; x++) {
                double rx = (x + 0.5 - cx) / f, ry = -(y + 0.5 - cy) / f;
                double cs = (rx * dv.x + ry * dv.y + dv.z) / std::sqrt(rx * rx + ry * ry + 1);
                if (cs < 0.15) continue;
                double ang = std::atan2(y + 0.5 - vy, x + 0.5 - vx);
                double pv = heat * 62 * std::pow((cs - 0.15) / 0.85, 1.4) * streaks[clampi((int)((ang + PI) / TAU * 1024), 0, 1023)];
                if (pv < 2) continue;
                uint32_t& p = rgbBuf[(size_t)y * FBW + x];
                double sceneShade = (0.3 * (p & 255) + 0.59 * ((p >> 8) & 255) + 0.11 * ((p >> 16) & 255)) / 255.0 * 63;
                plasma.at(std::min(63.0, pv + sceneShade * (1 - 0.6 * std::min(1.0, pv / 40))), o);
                p = 0xFF000000u | ((uint32_t)clampi((int)(o[2] * 255 + 0.5f), 0, 255) << 16) | ((uint32_t)clampi((int)(o[1] * 255 + 0.5f), 0, 255) << 8) | (uint32_t)clampi((int)(o[0] * 255 + 0.5f), 0, 255);
            }
        });
    }
    if (testProbeQuiet) return;
    // the link: its static grows as the probe goes deeper (the signal through ever more air)
    double lost = c >= probeDescentStart() ? clampd(std::log(std::max(bar, 1e-3) / 0.5) / std::log(probe.flight.crushBar / 0.5), 0, 1) : 0.0;
    double amp = 4 + 34 * lost * lost;
    uint32_t rs = (uint32_t)(c * 60) * 2654435761u + 0x9B0Bu;
    uint8_t* o = (uint8_t*)rgbBuf.data();
    for (size_t i = 0, n = rgbBuf.size(); i < n; i++) {
        rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
        int d = (int)(rs % (uint32_t)(2 * amp + 1)) - (int)amp;
        if ((rs >> 20) % 4096 < (uint32_t)(30 * lost * lost)) d = 120;   // a fleck of snow
        for (int k = 0; k < 3; k++) o[i * 4 + k] = (uint8_t)clampi(o[i * 4 + k] + d, 0, 255);
    }
}

void Game::renderProbe() {
    if (!probe.active || probe.body < 0 || probe.body >= (int)sys.bodies.size()) { renderSpace(); return; }
    if (probe.endClock >= 0) {   // the last frame, held under the hiss
        if (probe.lastFrame.size() != rgbBuf.size()) { renderProbeFeed(probe.endClock, probe.endT); probe.lastFrame = rgbBuf; }
        rgbBuf = probe.lastFrame;
        probeHiss();
    } else renderProbeFeed(probe.clock, t);
    if (!photoMode) renderProbeHUD();
}

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
    if (!ended && stage >= PS_ABOVE && stage <= PS_DEEP && !photoMode) {   // X-03: the layer the picture shows (a great storm's dome lifts a deck over its pressure; a hole takes one away)
        const ProbeViewStats& vs = probeViewStats();
        if (vs.inHole) st = "A CLEAR-AIR HOLE";
        else if (vs.fog >= 0 && vs.fog < a.decks) st = CLOUD_SPECIES[a.species[vs.fog]].deckName;
    }
    drawTextShadow(canvas, 18, 24, st.c_str(), ended ? HUD_RED : HUD_CYAN, HUD_SHADOW);
    const bool fast = rec ? replay.playing && replay.fast : probe.fast;
    if (!ended && fast) drawTextShadow(canvas, 18 + textWidth(st.c_str()) + 8, 24, "FAST", HUD_AMBER, HUD_SHADOW);   // R-406
    const std::string ep = epocOf(tt);
    drawTextShadow(canvas, UW - 8 - textWidth(ep.c_str()), 8, ep.c_str(), HUD_GREEN, HUD_SHADOW);
    std::string gn = trunc(upper(bodyNameOf(probe.body)), 20);
    drawTextShadow(canvas, UW - 8 - textWidth(gn.c_str()), 16, gn.c_str(), fd, HUD_SHADOW);
    if (!probe.stormKey.empty() && c >= probeDescentStart()) {   // X-04: the storm it fell into (its name when it has one), under the stage (the warp's line is at the right)
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
    if (rec) {   // X-04: the depth bar: the fall and the entry over its first tenth, then the pressure's logarithm to the end, the layers found in their colours
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
        drawTextCentered(canvas, UW / 2, UH / 2 + 5, fmt(rec ? "%d MIN %02d S - %s" : "%d MIN %02d S AFTER THE LAUNCH - %s", mins, secs, cause == 1 ? "THE HULL GAVE WAY" : (cause == 2 ? "THE HEAT TOOK IT" : "THE SHIP LEFT")).c_str(), fd);   // (a recording's clear of its depth bar)
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

// the relay's sound: the screen's when it is watched, a murmur from the landing screen in the cabin while it falls (X-04: a
// recording's on its screen)
void Game::probeAudio() {
    if (state == GameState::RECORDING) { recordingAudio(); return; }
    if (!probe.active || !probeHere()) { audio.probe = 0; audio.probeLost = false; return; }
    probeSound(probe.clock, state == GameState::PROBE, probe.endClock >= 0);
}

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
    audio.probeWind = air * (chute ? 0.55 : 1.0) * (probe.atm.ch.kind == GK_ICE ? 0.6 : 1.0);   // X-03: an ice giant's fall the quieter
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

// X-04: every frame the probe falls (after its clock): the explorer's turns of the camera and the game's clock along the fall (keys
// where they change: a turn of half a degree, a drift of half a second off the pace; the frame before a turn after a still stretch
// kept too, so the stretch replays still), and what it crosses for the log, once each with the depth
void Game::probeTrack() {
    if (!probe.active || probe.endClock >= 0 || probe.body < 0 || probe.body >= (int)sys.bodies.size()) return;
    if (!probe.bands.valid()) probeEnsureClouds();   // (after a load: the decks for the layers)
    const double c = probe.clock;
    if (probe.timeTrack.size() < 2) { probe.timeTrack = {c, t}; probe.prevClock = c; probe.prevT = t; }
    if (probe.camTrack.size() < 3) probe.camTrack = {c, probe.camYaw, probe.camPitch};
    {   // the clock: a key where the game's time leaves the pace of the segment from the last key (its first frame's)
        std::vector<double>& T = probe.timeTrack;
        const double c0 = T[T.size() - 2], t0 = T[T.size() - 1];
        if (c > probe.prevClock + 1e-9) {
            if (probe.prevPace < 0) probe.prevPace = (t - t0) / std::max(1e-9, c - c0);
            else if (std::fabs(t - (t0 + probe.prevPace * (c - c0))) > TIME_KEY) {
                T.push_back(probe.prevClock); T.push_back(probe.prevT);
                probe.prevPace = (t - probe.prevT) / (c - probe.prevClock);
            }
            probe.prevClock = c; probe.prevT = t;
        }
    }
    {   // the camera
        std::vector<double>& K = probe.camTrack;
        const size_t n = K.size();
        const double cl = K[n - 3], yl = K[n - 2], pl = K[n - 1];
        auto off = [&](double y, double p) { return std::fabs(wrapAngle(y - yl)) > CAM_KEY || std::fabs(p - pl) > CAM_KEY; };
        if (off(probe.camYaw, probe.camPitch) && c > cl + 1e-6) {
            if (probe.prevCamClock > cl + 0.05 && !off(probe.prevYaw, probe.prevPitch)) { K.push_back(probe.prevCamClock); K.push_back(probe.prevYaw); K.push_back(probe.prevPitch); }
            K.push_back(c); K.push_back(probe.camYaw); K.push_back(probe.camPitch);
        }
        probe.prevCamClock = c; probe.prevYaw = probe.camYaw; probe.prevPitch = probe.camPitch;
    }
    if (c < probeDescentStart()) return;
    const GiantAtmosphere& a = probe.atm;
    const double bar = probeBarAt(probe.flight, c);
    const std::string giant = upper(bodyNameOf(probe.body));
    auto depth = [&](double b) { double z = giantAltitudeKm(a, b); return fmt("%.2f ATM, %.0f KM %s", atmOf(b), std::fabs(z), z >= 0 ? "UP" : "DOWN"); };
    if (!(probe.notes & PN_AURORA)) {   // the aurora's curtains over the clouds (as the camera turns to them)
        probe.notes |= PN_AURORA;
        double ex, ez, p0;
        if (probeAuroraWay(ex, ez, p0) && std::fabs(p0) < 2800 && a.ch.aurora > 0.25) logEvent("PROBE", fmt("PROBE %d SAW THE AURORA'S CURTAINS OVER %s, %.0f KM OFF", probe.id, giant.c_str(), std::fabs(p0)));
    }
    if (!(probe.notes & PN_WALL)) {   // the great storm's wall in sight from outside it
        probe.notes |= PN_WALL;
        double gd, ex, ez;
        if (probeStormWall(gd, ex, ez) && gd > 0) logEvent("PROBE", fmt("PROBE %d SAW THE WALL OF THE GREAT STORM OF %s, %.0f KM OFF", probe.id, giant.c_str(), gd));
    }
    if (!(probe.notes & PN_STORM) && probe.stormKind > 0 && bar >= a.top[0]) {   // into the storm at the clouds' top
        probe.notes |= PN_STORM;
        const std::string nm = probeStormName(), what = probe.stormKind == 2 ? "THE GREAT STORM" : "A STORM";
        logEvent("PROBE", nm.empty() ? fmt("PROBE %d FELL INTO %s OF %s AT %s", probe.id, what.c_str(), giant.c_str(), depth(bar).c_str())
                                     : fmt("PROBE %d FELL INTO %s, %s OF %s, AT %s", probe.id, upper(nm).c_str(), what.c_str(), giant.c_str(), depth(bar).c_str()));
    }
    if (!(probe.notes & PN_HOLE) && probe.holePath && bar > a.top[0]) {   // a clear-air hole: the layers found over the frame's stretch
        for (double cc = std::max(probeDescentStart(), probe.prevHoleClock); cc <= c + 1e-9; cc += std::max(1e-3, std::min(1.0, c - cc))) {
            const int L = probeLayerAt(cc);
            if (L >= GIANT_HOLE_LAYER) {
                probe.notes |= PN_HOLE;
                const double b = probeBarAt(probe.flight, cc);
                logEvent("PROBE", fmt("PROBE %d FELL THROUGH A CLEAR-AIR HOLE IN %s'S %s, %s", probe.id, giant.c_str(), giantLayerName(a, 2 + 2 * (L - GIANT_HOLE_LAYER), true).c_str(), depth(b).c_str()));
                break;
            }
            if (cc >= c) break;
        }
        probe.prevHoleClock = c;
    }
    if (!(probe.notes & PN_HAIL) && a.ch.hail) {   // diamond hail glinting in the lamp (as the view shows it)
        const int last = a.decks - 1;
        if (smoothstep(a.top[last], std::sqrt(a.top[last] * a.base[last]), bar) > 0.3) {
            probe.notes |= PN_HAIL;
            logEvent("PROBE", fmt("PROBE %d MET DIAMOND HAIL IN %s AT %s", probe.id, giant.c_str(), depth(bar).c_str()));
        }
    }
}

int Game::probeLayerAt(double c) const {
    if (!probe.active || c < probeDescentStart() || !probe.bands.valid()) return 0;
    const double bar = probeBarAt(probe.flight, c);
    double px[GIANT_MAX_DECKS], pz[GIANT_MAX_DECKS] = {0, 0, 0};
    for (int k = 0; k < GIANT_MAX_DECKS; k++) px[k] = -probeDeckDrift(k, c);
    return giantFoundLayer(probe.atm, probe.decks.layersAt(giantAltitudeKm(probe.atm, std::max(bar, 1e-9)), px, pz), bar);
}

// the light at the probe against the sunlight over the clouds at noon: the sun's come down to it (through the holes over it) by its
// height over the aim's horizon, and the deep's glow
double Game::probeLightAt(double c, double tt) const {
    const GiantAtmosphere& a = probe.atm;
    const double bar = std::max(probeBarAt(probe.flight, c), 1e-9);
    unsigned cut = 0;
    for (int k = 0; k + 1 < a.decks; k++) if (bar > a.top[k] && probe.bands.valid() && probe.decks.holeAt(k, -probeDeckDrift(k, c), 0) >= 0.5) cut |= 1u << k;
    const Vec3 up = normalize(sys.bodyFrame(probe.body, tt).transposed() * StarSystem::bodyFromLatLon(probe.aimLat, probe.aimLon));
    const double el = dot(up, normalize(sys.star.pos - sys.bodyPos(probe.body, tt)));
    return giantSunlightAt(a, bar, cut) * std::max(0.0, el) + giantGlowAt(a, bar, cut);
}

std::string Game::nameOfKey(const std::string& key) const {
    auto it = guide.names.find(key);
    if (it != guide.names.end()) return it->second;
    it = guide.inbox.find(key);
    return it != guide.inbox.end() ? it->second : std::string("UNKNOWN");
}

std::string Game::probeStormName() const {
    if (probe.stormKey.empty()) return "";
    std::string n = nameOfKey(probe.stormKey);
    return n == "UNKNOWN" ? std::string() : n;
}

// X-04: the profile down to the record's end, a sample every quarter of an e-fold of the pressure from the clouds' top (the clock,
// the pressure, the temperature, the wind as the screen read it, the light, the layer found, the flashes since the sample over it),
// and the layers it found with their bounds (each change between two samples found by halves)
void Game::probeProfileOf(ProbeRecord& r) const {
    r.profile.clear(); r.layers.clear();
    const GiantAtmosphere& a = probe.atm;
    const double c0 = probeDescentStart();
    if (r.endClock < c0) return;
    const double endBar = probeBarAt(probe.flight, r.endClock);
    double prevC = c0;
    for (double lb = std::log(PROBE_TOP_BAR);; lb += 0.25) {
        const double bar = std::min(std::exp(lb), endBar), c = bar >= endBar ? r.endClock : std::min(clockAtBar(probe.flight, bar), r.endClock);
        ProbeSample q;
        q.clock = c; q.bar = bar; q.tempK = giantTemperatureK(a, bar);
        q.windE = giantWindEast(a, probe.aimLat, bar) * (1 + 0.12 * std::sin(c * 0.7) * std::sin(c * 0.23));   // as the screen read it
        q.light = probeLightAt(c, recordTimeAt(r, c));
        q.layer = probeLayerAt(c);
        q.flashes = r.profile.empty() ? 0 : flashCount(a, probe.flight, probe.seed, prevC, c);
        r.profile.push_back(q);
        prevC = c;
        if (bar >= endBar) break;
    }
    int cur = r.profile[0].layer; double from = r.profile[0].bar;
    for (size_t i = 1; i < r.profile.size(); i++) {
        if (r.profile[i].layer == cur) continue;
        double lo = r.profile[i - 1].clock, hi = r.profile[i].clock;   // the change between them
        for (int it = 0; it < 14; it++) { double m = 0.5 * (lo + hi); if (probeLayerAt(m) == cur) lo = m; else hi = m; }
        const double at = probeBarAt(probe.flight, hi);
        r.layers.push_back(cur); r.layers.push_back(from); r.layers.push_back(at);
        cur = r.profile[i].layer; from = at;
    }
    r.layers.push_back(cur); r.layers.push_back(from); r.layers.push_back(endBar);
}

// X-04: the record at the end (the hull, the heat) or the cut, into the guide (a probe ended again after a load replaces its own);
// its final image drawn now (the last frame the screen holds) and written beside the photographs
void Game::probeKeep(int cause) {
    if (!probe.active || probe.body < 0 || probe.body >= (int)sys.bodies.size() || sys.star.seed != probe.starSeed) return;
    if (!probe.bands.valid()) probeEnsureClouds();
    const double c = probe.clock;
    ProbeRecord r;
    r.id = probe.id; r.body = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, probe.body); r.gen = GEN_VERSION; r.saved = (long long)std::time(nullptr);
    r.launchT = probe.launchT; r.aimLat = probe.aimLat; r.aimLon = probe.aimLon; r.seed = probe.seed;
    r.startX = probe.startRel.x; r.startY = probe.startRel.y; r.startZ = probe.startRel.z; r.headYaw = probe.headYaw; r.descYaw = probe.descYaw;
    r.chuteOpen = probe.flight.chuteOpen; r.chuteClose = probe.flight.chuteClose; r.crushBar = probe.flight.crushBar;
    r.endClock = c; r.endBar = probeBarAt(probe.flight, c); r.endKm = -giantAltitudeKm(probe.atm, std::max(r.endBar, 1e-9)); r.endK = giantTemperatureK(probe.atm, std::max(r.endBar, 1e-9)); r.endCause = cause;
    r.storm = probe.stormKey;
    r.cam = probe.camTrack; r.cam.push_back(c); r.cam.push_back(probe.camYaw); r.cam.push_back(probe.camPitch);
    r.time = probe.timeTrack; r.time.push_back(c); r.time.push_back(t);
    probeProfileOf(r);
    renderProbeFeed(c, t);   // the final image (the frame the screen holds after the end)
    probe.lastFrame = rgbBuf;
    makeDir(shotsDir);
    const std::string fn = fmt("%s/probe_%d.png", shotsDir.c_str(), r.id);
    writePNG(fn.c_str(), rgbBuf.data(), FBW, FBH);
    if (FILE* f = fopen((fn.substr(0, fn.size() - 4) + ".txt").c_str(), "w")) { fprintf(f, "%s - %s\n", recordTitle(r).c_str(), recordLine(r).c_str()); fclose(f); }
    const int at = guide.probeIndexBySeed(r.seed);
    if (at >= 0) guide.probes[at] = r; else guide.probes.push_back(r);
    guide.save(guidePath);
}

// ---------------------------------------------------------------------------
// X-04: the recordings
// ---------------------------------------------------------------------------

template <class F> void Game::asRecording(F f) {
    std::swap(probe, replay.p);
    if (!replay.here) { std::swap(sys, replay.sys); std::swap(nb, replay.nb); }
    f();
    if (!replay.here) { std::swap(sys, replay.sys); std::swap(nb, replay.nb); }
    std::swap(probe, replay.p);
}

// the record's probe as it fell (its numbers into a ProbeState: the descent's functions draw it again), and its star's system and
// sky when it is not the one the ship is in
bool Game::recordingContext(int rec) {
    if (rec < 0 || rec >= (int)guide.probes.size()) return false;
    const ProbeRecord& r = guide.probes[rec];
    int64_t sx, sy, sz;
    const size_t slash = r.body.rfind('/');
    if (slash == std::string::npos || !Guide::parseStarKey(r.body, sx, sy, sz)) return false;
    const int bi = atoi(r.body.c_str() + slash + 1);
    Star s;
    if (!starInSector(sx, sy, sz, s, true)) { status("THE GALAXY WAS REBUILT SINCE THIS RECORDING - ITS STAR IS GONE", 5); audio.beep = 3; return false; }
    const bool here = sys.star.sx == sx && sys.star.sy == sy && sys.star.sz == sz && !sys.bodies.empty();
    if (!here && (replay.here || replay.sys.star.seed != s.seed || replay.sys.bodies.empty())) {
        replay.sys = StarSystem(); replay.sys.generate(s); replay.sys.valid = true;
        replay.nb = StarNeighborhood(); replay.nb.update(s.pos);
        replay.rec = -1;
    }
    replay.here = here;
    const StarSystem& S = here ? sys : replay.sys;
    if (bi < 0 || bi >= (int)S.bodies.size() || !isProbeGiant(S.bodies[bi].type)) { status("THE GIANT OF THIS RECORDING IS GONE: THE WORLDS WERE REGENERATED", 5); audio.beep = 3; return false; }
    if (replay.rec != rec || replay.p.seed != r.seed || replay.p.starSeed != s.seed) {
        ProbeState& p = replay.p;
        p = ProbeState();
        p.active = true; p.body = bi; p.starSeed = s.seed; p.id = r.id;
        p.aimLat = r.aimLat; p.aimLon = r.aimLon; p.launchT = r.launchT; p.startRel = Vec3(r.startX, r.startY, r.startZ);
        p.flight.chuteOpen = r.chuteOpen; p.flight.chuteClose = r.chuteClose; p.flight.crushBar = r.crushBar;
        p.seed = r.seed; p.headYaw = r.headYaw; p.descYaw = r.descYaw; p.stormKey = r.storm;
        p.atm = giantAtmosphereOf(S, bi);
        replay.last.clear();
        replay.rec = rec;
    }
    return true;
}

bool Game::openRecording(int rec) {
    if (!recordingContext(rec)) return false;
    replay.clock = 0; replay.playing = true; replay.fast = false; replay.lookYaw = replay.lookPitch = 0;
    replay.p.lastFlashSlot = -1;
    state = GameState::RECORDING;
    status(fmt("THE RECORDING OF %s", recordTitle(guide.probes[rec]).c_str()), 4);
    audio.beep = 4;
    return true;
}

void Game::updateRecording(const Input& in, double realDt) {
    if (replay.rec < 0 || replay.rec >= (int)guide.probes.size()) { state = GameState::GALLERY; return; }
    const ProbeRecord& r = guide.probes[replay.rec];
    if (in.wasPressed(KEY_ESCAPE) || enterKey(in)) { state = GameState::GALLERY; galleryLoaded = -1; audio.beep = 4; return; }
    const bool atEnd = replay.clock >= r.endClock;
    replay.fast = in.shift();
    if (in.wasPressed(KEY_SPACE)) { if (atEnd) replay.clock = 0, replay.playing = true; else replay.playing = !replay.playing; }
    if (in.wasPressed(KEY_HOME)) replay.clock = 0;
    if (in.wasPressed(KEY_END)) replay.clock = r.endClock;
    {   // the depth: a press a fiftieth of the bar, held a quarter of it a second (Shift four times as far)
        double du = 0;
        if (in.wasPressed(KEY_RIGHT)) du += 0.02;
        if (in.wasPressed(KEY_LEFT)) du -= 0.02;
        if (in.isDown(KEY_RIGHT)) du += 0.25 * realDt * (in.shift() ? 4 : 1);
        if (in.isDown(KEY_LEFT)) du -= 0.25 * realDt * (in.shift() ? 4 : 1);
        if (du != 0) replay.clock = recordClockAtU(r, clampd(recordDepthU(r, replay.clock) + du, 0, 1));
    }
    {   // the layers it found: Down (Page Down) to the next one's top, Up (Page Up) to this one's, or the one over it from its top
        const bool next = in.wasPressed(KEY_DOWN) || in.wasPressed(KEY_PAGE_DOWN), prev = in.wasPressed(KEY_UP) || in.wasPressed(KEY_PAGE_UP);
        if ((next || prev) && !r.layers.empty() && replay.clock >= probeDescentStart()) {
            const ProbeFlight f{r.chuteOpen, r.chuteClose, r.crushBar};
            const double bar = probeBarAt(f, replay.clock);
            double to = -1;
            for (size_t i = 0; i + 2 < r.layers.size(); i += 3) {
                const double from = r.layers[i + 1];
                if (next && from > bar * 1.001 && to < 0) to = from;
                if (prev && from < bar / 1.05) to = from;
            }
            if (to > 0) replay.clock = std::min(r.endClock, clockAtBar(f, to * 1.0005));
            else if (prev) replay.clock = 0;
        }
    }
    if (replay.playing && !atEnd) replay.clock = std::min(r.endClock, replay.clock + realDt * (replay.fast ? PROBE_FAST : 1.0));
    const double rate = 0.0032 * settings.mouseSensitivity;   // the viewer's own look over the recording's
    replay.lookYaw = wrapAngle(replay.lookYaw + in.mouseDx * rate);
    replay.lookPitch = clampd(replay.lookPitch - in.mouseDy * rate * (settings.invertY ? -1 : 1), -60 * DEG, 60 * DEG);
    if (in.wasPressed(KEY_X) && !in.ctrl()) replay.lookYaw = replay.lookPitch = 0;
    if (in.wasPressed(KEY_N) && !in.ctrl()) {   // the storm it fell into
        if (r.storm.empty()) { status("THIS PROBE FELL INTO NO STORM", 3); audio.beep = 3; }
        else { textStormKey = r.storm; guideReturn = GameState::RECORDING; const std::string n = nameOfKey(r.storm); beginTextEntry("NAME THE STORM", 9, n == "UNKNOWN" ? std::string() : upper(n)); }
    }
}

void Game::renderRecording() {
    if (replay.rec < 0 || replay.rec >= (int)guide.probes.size() || !replay.p.active) { renderSpace(); return; }
    const ProbeRecord& r = guide.probes[replay.rec];
    const double c = std::min(replay.clock, r.endClock), tt = recordTimeAt(r, c);
    asRecording([&] {
        double cy, cp;
        recordCamAt(r, c, cy, cp);
        probe.camYaw = wrapAngle(cy + replay.lookYaw); probe.camPitch = clampd(cp + replay.lookPitch, -75 * DEG, 85 * DEG);
        if (replay.clock >= r.endClock) {   // its last frame, held under the hiss
            if (replay.last.size() != rgbBuf.size()) { renderProbeFeed(r.endClock, tt); replay.last = rgbBuf; }
            rgbBuf = replay.last;
            probeHiss();
        } else renderProbeFeed(c, tt);
        if (!photoMode) renderProbeHUD(&r, replay.clock, tt);
    });
}

void Game::recordingAudio() {
    if (replay.rec < 0 || replay.rec >= (int)guide.probes.size() || !replay.p.active) { audio.probe = 0; audio.probeLost = false; return; }
    const ProbeRecord& r = guide.probes[replay.rec];
    const bool ended = replay.clock >= r.endClock;
    asRecording([&] {
        probeEnsureClouds();
        probeSound(std::min(replay.clock, r.endClock), true, ended);
        if (!replay.playing && !ended) audio.probe = 0;   // paused: the screen is quiet
    });
}

// the last frame of a recording (a lent one has no final image on the disk: the gallery draws it)
void Game::recordingPoster(int rec, std::vector<uint32_t>& out) {
    out.clear();
    if (!recordingContext(rec)) return;
    const ProbeRecord& r = guide.probes[rec];
    const std::vector<uint32_t> under = rgbBuf;   // the frame the gallery is drawn over
    asRecording([&] {
        double cy, cp;
        recordCamAt(r, r.endClock, cy, cp);
        probe.camYaw = cy; probe.camPitch = cp;
        renderProbeFeed(r.endClock, recordTimeAt(r, r.endClock));
    });
    out = rgbBuf;
    rgbBuf = under;
}

double Game::recordTimeAt(const ProbeRecord& r, double c) const {
    const std::vector<double>& T = r.time;
    if (T.size() < 2) return r.launchT + c;
    if (c <= T[0]) return T[1] + (c - T[0]);
    for (size_t i = 2; i + 1 < T.size(); i += 2)
        if (c <= T[i]) return T[i - 1] + (T[i + 1] - T[i - 1]) * (T[i] > T[i - 2] ? (c - T[i - 2]) / (T[i] - T[i - 2]) : 1.0);
    return T[T.size() - 1] + (c - T[T.size() - 2]);
}

void Game::recordCamAt(const ProbeRecord& r, double c, double& yaw, double& pitch) const {
    const std::vector<double>& K = r.cam;
    yaw = pitch = 0;
    if (K.size() < 3) return;
    if (c <= K[0]) { yaw = K[1]; pitch = K[2]; return; }
    for (size_t i = 3; i + 2 < K.size(); i += 3)
        if (c <= K[i]) {
            const double u = K[i] > K[i - 3] ? (c - K[i - 3]) / (K[i] - K[i - 3]) : 1.0;
            yaw = wrapAngle(K[i - 2] + wrapAngle(K[i + 1] - K[i - 2]) * u); pitch = K[i - 1] + (K[i + 2] - K[i - 1]) * u;
            return;
        }
    yaw = K[K.size() - 2]; pitch = K[K.size() - 1];
}

// the depth bar: the fall and the entry by the clock over its first tenth, then the pressure's logarithm from the clouds' top to the end
double Game::recordDepthU(const ProbeRecord& r, double c) const {
    const double c0 = probeDescentStart();
    if (r.endClock <= c0) return clampd(c / std::max(1e-9, r.endClock), 0, 1);
    if (c < c0) return 0.1 * clampd(c / c0, 0, 1);
    const ProbeFlight f{r.chuteOpen, r.chuteClose, r.crushBar};
    const double span = std::log(std::max(r.endBar, PROBE_TOP_BAR * 1.01) / PROBE_TOP_BAR);
    return 0.1 + 0.9 * clampd(std::log(std::max(probeBarAt(f, std::min(c, r.endClock)), PROBE_TOP_BAR) / PROBE_TOP_BAR) / span, 0, 1);
}

double Game::recordClockAtU(const ProbeRecord& r, double u) const {
    const double c0 = probeDescentStart();
    if (r.endClock <= c0) return clampd(u, 0, 1) * r.endClock;
    if (u <= 0.1) return clampd(u / 0.1, 0, 1) * c0;
    const ProbeFlight f{r.chuteOpen, r.chuteClose, r.crushBar};
    const double span = std::log(std::max(r.endBar, PROBE_TOP_BAR * 1.01) / PROBE_TOP_BAR);
    return std::min(r.endClock, clockAtBar(f, PROBE_TOP_BAR * std::exp((u - 0.1) / 0.9 * span)));
}

std::string Game::recordTitle(const ProbeRecord& r) const { return fmt("PROBE %d INTO %s", r.id, trunc(upper(nameOfKey(r.body)), 20).c_str()); }

std::string Game::recordLine(const ProbeRecord& r) const {
    std::string end = r.endCause == 3 ? (r.endKm >= 0 ? fmt("CUT %.0f KM DOWN", r.endKm) : (r.endClock < probeDescentStart() ? std::string("CUT IN THE FALL") : fmt("CUT AT %.3f ATM", atmOf(r.endBar))))
                                      : fmt("%.0f KM DOWN, %.1f ATM", r.endKm, atmOf(r.endBar));
    return epocOf(r.launchT) + "  " + end;
}

// X-04: the records of the data sheet's giant (the explorer's and the lent ones, in the guide's order)
std::vector<int> Game::dataProbes() const {
    std::vector<int> out;
    const int bi = ship.localTarget;
    if (!sys.valid || bi < 0 || bi >= (int)sys.bodies.size() || !isProbeGiant(sys.bodies[bi].type)) return out;
    const std::string key = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, bi);
    for (int i = 0; i < (int)guide.probes.size(); i++) if (guide.probes[i].body == key) out.push_back(i);
    return out;
}

// X-04: a probe's profile on its giant's data sheet: the pressure down the page (its logarithm, from the clouds' top to the deepest
// end of the giant's probes), the layers it found by name, the temperature, the wind (east right of the line, west left) and the
// light (its logarithm; the lightning's rate as ticks from the column's right); the giant's other probes dim under it, on the same
// scales, so two places read side by side
void Game::renderProbeProfile(int rec) {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 200);
    const std::vector<int> all = dataProbes();
    if (rec < 0 || rec >= (int)guide.probes.size() || all.empty()) return;
    const ProbeRecord& r = guide.probes[rec];
    const GiantAtmosphere a = giantAtmosphereOf(sys, ship.localTarget);
    int pos = 0;
    for (int i = 0; i < (int)all.size(); i++) if (all[i] == rec) pos = i;
    drawText(canvas, 8, 6, recordTitle(r).c_str(), r.lent ? HUD_CYAN : HUD_AMBER);
    const std::string of = r.lent ? fmt("LENT  %d OF %d", pos + 1, (int)all.size()) : fmt("%d OF %d", pos + 1, (int)all.size());
    drawText(canvas, UW - 8 - textWidth(of.c_str()), 6, of.c_str(), r.lent ? HUD_CYAN : HUD_DIM);
    std::string where = fmt("AIMED AT %s", latLonString(r.aimLat, r.aimLon).c_str());
    if (!r.storm.empty()) { const std::string n = nameOfKey(r.storm); where = "INTO " + (n != "UNKNOWN" ? trunc(upper(n), 20) : std::string(r.storm.substr(r.storm.size() - 2) == "TG" ? "THE GREAT STORM" : "A STORM")); }
    drawText(canvas, 8, 15, (where + "  " + epocOf(r.launchT)).c_str(), HUD_DIM);
    const std::string end = r.endCause == 3 ? std::string("THE RELAY WAS CUT: ") + recordLine(r).substr(epocOf(r.launchT).size() + 2)
                                            : fmt("%.0f KM DOWN, %.1f ATM, %+.0f C - %s", r.endKm, atmOf(r.endBar), r.endK - 273.15, r.endCause == 1 ? "THE HULL GAVE WAY" : "THE HEAT TOOK IT");
    drawText(canvas, 8, 23, end.c_str(), HUD_GREEN);
    drawTextCentered(canvas, UW / 2, UH - 9, "LEFT/RIGHT PROBES  ANY OTHER KEY CLOSES", HUD_DIM);
    if (r.profile.size() < 2) { drawTextCentered(canvas, UW / 2, 100, "THE RELAY WAS CUT OVER THE CLOUDS", HUD_DIM); return; }
    const int y0 = 46, y1 = 176, xL = 38, xT0 = 128, xT1 = 186, xW0 = 192, xW1 = 250, xQ0 = 256, xQ1 = 312;
    double pMax = PROBE_TOP_BAR * 100, tMin = 1e9, tMax = -1e9, wMax = 20;   // the scales: every probe of the giant's
    for (int i : all) for (const ProbeSample& q : guide.probes[i].profile) { pMax = std::max(pMax, q.bar); tMin = std::min(tMin, q.tempK); tMax = std::max(tMax, q.tempK); wMax = std::max(wMax, std::fabs(q.windE)); }
    auto yOf = [&](double bar) { return y0 + (int)std::lround((y1 - y0) * clampd(std::log(std::max(bar, PROBE_TOP_BAR) / PROBE_TOP_BAR) / std::log(pMax / PROBE_TOP_BAR), 0, 1)); };
    auto xT = [&](double k) { return xT0 + (int)std::lround((xT1 - xT0) * clampd((k - tMin) / std::max(1.0, tMax - tMin), 0, 1)); };
    auto xW = [&](double w) { return (xW0 + xW1) / 2 + (int)std::lround((xW1 - xW0) / 2 * clampd(w / wMax, -1, 1)); };
    const int xQs = xQ1 - 14;   // the light's scale; the lightning's ticks in the strip right of it
    auto xQ = [&](double l) { return xQ0 + (int)std::lround((xQs - xQ0) * clampd((std::log10(std::max(l, 1e-5)) + 5) / 5, 0, 1)); };   // from a hundred-thousandth to the full sun
    drawText(canvas, 8, 33, "ATM", HUD_DIM); drawText(canvas, xL, 33, "LAYERS FOUND", HUD_DIM);
    drawText(canvas, xT0, 33, "TEMP C", HUD_DIM); drawText(canvas, xW0, 33, "WIND", HUD_DIM); drawText(canvas, xQ0, 33, "LIGHT", HUD_DIM);
    const uint32_t grid = rgb(48, 62, 54);
    for (double atm = 0.001; atm <= atmOf(pMax) * 1.0001; atm *= 10) {   // the decades
        const int y = yOf(atm / 0.98692);
        for (int x = xL; x <= xQ1; x += 3) fillRectRGB(canvas, x, y, x, y, grid);
        const std::string l = atm < 0.0099 ? "0.001" : (atm < 0.099 ? "0.01" : (atm < 0.99 ? "0.1" : fmt("%.0f", atm)));
        drawText(canvas, 34 - textWidth(l.c_str()), y - 3, l.c_str(), HUD_DIM);
    }
    {   // the wind's zero
        const int x = xW(0);
        for (int y = y0; y <= y1; y += 2) fillRectRGB(canvas, x, y, x, y, grid);
    }
    int nextFree = y0 - 8;   // the layers found: a strip in their colours, a name where its band has room
    for (size_t i = 0; i + 2 < r.layers.size(); i += 3) {
        const int L = (int)r.layers[i], ya = yOf(r.layers[i + 1]), yb = yOf(r.layers[i + 2]);
        fillRectRGB(canvas, xL, ya, xL + 3, std::max(ya, yb), layerColour(a, L));
        const int ty = std::max(nextFree + 8, (ya + yb) / 2 - 3);
        if (ty + 7 <= std::max(yb, ya + 7) + 3 && ty + 7 <= y1 + 4) { drawText(canvas, xL + 6, ty, giantLayerName(a, L, true).c_str(), L >= GIANT_HOLE_LAYER ? HUD_CYAN : HUD_GREEN); nextFree = ty; }
    }
    auto curves = [&](const ProbeRecord& q, uint32_t ct, uint32_t cw, uint32_t cl) {
        for (size_t i = 1; i < q.profile.size(); i++) {
            const ProbeSample &p0 = q.profile[i - 1], &p1 = q.profile[i];
            const int ya = yOf(p0.bar), yb = yOf(p1.bar);
            drawLineRGB(canvas, xT(p0.tempK), ya, xT(p1.tempK), yb, ct);
            drawLineRGB(canvas, xW(p0.windE), ya, xW(p1.windE), yb, cw);
            drawLineRGB(canvas, xQ(p0.light), ya, xQ(p1.light), yb, cl);
        }
    };
    for (int i : all) if (i != rec) curves(guide.probes[i], HUD_DIM, HUD_DIM, HUD_DIM);   // the giant's other probes
    curves(r, HUD_AMBER, HUD_CYAN, HUD_WHITE);
    for (size_t i = 1; i < r.profile.size(); i++) {   // the lightning: flashes a minute over the stretch, a tick from the column's right
        const ProbeSample &p0 = r.profile[i - 1], &p1 = r.profile[i];
        if (p1.flashes <= 0 || p1.clock <= p0.clock) continue;
        const double rate = p1.flashes / (p1.clock - p0.clock) * 60;
        const int len = (int)clampd(1 + rate * 0.8, 1, 11), y = (yOf(p0.bar) + yOf(p1.bar)) / 2;
        drawLineRGB(canvas, xQs + 3, y, xQs + 3 + len, y, rgb(255, 230, 120));
    }
    const int yl = y1 + 4;
    drawText(canvas, xT0, yl, fmt("%+.0f", tMin - 273.15).c_str(), HUD_DIM);
    { const std::string m = fmt("%+.0f", tMax - 273.15); drawText(canvas, xT1 - textWidth(m.c_str()), yl, m.c_str(), HUD_DIM); }
    drawText(canvas, xW0, yl, "W", HUD_DIM); drawText(canvas, xW1 - 6, yl, "E", HUD_DIM);
    { const std::string m = fmt("%.0f KT", wMax * 1.94384); drawText(canvas, (xW0 + xW1) / 2 - textWidth(m.c_str()) / 2, yl, m.c_str(), HUD_DIM); }
    drawText(canvas, xQ0 + 3, yl, "DARK", HUD_DIM); drawText(canvas, xQ1 - textWidth("SUN"), yl, "SUN", HUD_DIM);
}

std::string Game::probeCaption() const {
    if (!probe.active || probe.body < 0 || probe.body >= (int)sys.bodies.size()) return "";
    double c = probe.endClock >= 0 ? probe.endClock : probe.clock;
    int stage = probe.endClock >= 0 ? PS_END : probeStageAt(probe.atm, probe.flight, c);
    std::string name = upper(bodyNameOf(probe.body));
    if (stage == PS_FALL) return fmt("THE PROBE FALLING TO %s", name.c_str());
    if (stage == PS_ENTRY) return fmt("THE PROBE'S ENTRY INTO %s", name.c_str());
    double bar = probeBarAt(probe.flight, c), z = giantAltitudeKm(probe.atm, bar);
    if (stage == PS_END) return fmt("THE PROBE'S LAST FRAME IN %s, %.0f KM DOWN, %.1f ATM", name.c_str(), probe.endKm, atmOf(probe.endBar));
    return fmt("THE PROBE IN %s: %s, %.2f ATM, %s %.1f KM", name.c_str(), probeStageLabel(c).c_str(), atmOf(bar), z >= 0 ? "ALT" : "DEPTH", std::fabs(z));
}

double Game::testProbeFlash(double clock) const { return probe.active ? flashAt(probe.atm, probe.flight, probe.seed, clock, nullptr, 0, 0, nullptr, nullptr).strength : 0; }
bool Game::testProbeBolt(double clock) const {
    if (!probe.active || !probe.bands.valid()) return false;
    double px[GIANT_MAX_DECKS], pz[GIANT_MAX_DECKS] = {0, 0, 0};
    for (int k = 0; k < GIANT_MAX_DECKS; k++) px[k] = -probeDeckDrift(k, clock);
    return flashAt(probe.atm, probe.flight, probe.seed, clock, &probe.decks, probe.descYaw + probe.camYaw, giantAltitudeKm(probe.atm, std::max(probeBarAt(probe.flight, clock), 1e-9)), px, pz).bolt;
}
int Game::testProbeStage() const { return !probe.active ? -1 : (probe.endClock >= 0 ? PS_END : probeStageAt(probe.atm, probe.flight, probe.clock)); }
double Game::testProbeBar() const { return probe.active ? probeBarAt(probe.flight, probe.clock) : 0; }
double Game::testProbeEndClock() const { return probe.active ? probeEndClock(probe.atm, probe.flight) : -1; }
void Game::testProbeSkip(double toClock) {
    if (!probe.active || probe.endClock >= 0 || toClock <= probe.clock) return;
    double endC = probeEndClock(probe.atm, probe.flight);
    probe.clock = std::min(toClock, endC);
    if (probe.flight.chuteOpen >= 0 && probe.clock >= probe.flight.chuteClose) probe.tornNoted = true;
    if (probe.clock >= endC) probeEnd();
}
std::string Game::testProbeViewInfo() const {
    const ProbeViewStats& v = probeViewStats();
    return fmt("floor %d (%.0f%%, shade %.0f) ceiling %d (%.0f%%, shade %.0f) fog %d (T %.2f) pano %dx%d; %.1f + %.1f + %.1f + %.1f ms; sun %.0f deg, light %.2f/%.2f/%.2f, albedo %.2f", v.floor, v.covFloor * 100, v.shadeFloor, v.ceil,
               v.covCeil * 100, v.shadeCeil, v.fog, v.fogT, v.columns, v.rows, v.msPano, v.msFog, v.msNodes, v.msSpread, v.sunElDeg, v.direct0, v.diffuse0, v.inside0, v.alb0) +
           (v.flash > 0 ? fmt("; flash %.2f at %.0f %.0f %.0f km, frame %.0f,%.0f", v.flash, v.flashPos.x, v.flashPos.y, v.flashPos.z, v.flashSx, v.flashSy) : std::string()) +
           (v.inHole ? std::string("; in a hole") : std::string()) + (v.aurora > 0.0005 ? fmt("; aurora %.3f", v.aurora) : std::string());
}
std::string Game::testProbeInfo() const {
    if (!probe.active) return "no probe";
    double c = probe.clock, bar = probeBarAt(probe.flight, c);
    return fmt("body %d aim %.2f/%.2f deg clock %.1f s stage %s bar %.4f alt %.1f km T %.0f K wind %.0f m/s chute %.1f..%.1f crush %.1f bar end %.1f s%s", probe.body, probe.aimLat / DEG, probe.aimLon / DEG, c,
               probeStageLabel(c).c_str(), bar, bar > 0 ? giantAltitudeKm(probe.atm, bar) : 0.0, bar > 0 ? giantTemperatureK(probe.atm, bar) : 0.0, giantWindEast(probe.atm, probe.aimLat, bar),
               probe.flight.chuteOpen, probe.flight.chuteClose, probe.flight.crushBar, probeEndClock(probe.atm, probe.flight), probe.endClock >= 0 ? fmt(" ENDED (%s)", probe.endCause == 1 ? "hull" : "heat").c_str() : "");
}
