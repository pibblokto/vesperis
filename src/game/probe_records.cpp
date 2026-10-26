// What the probe sends back (X-04): its track, the profile it measured, the recordings the gallery plays
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

namespace {
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
const double CAM_KEY = 0.5 * DEG;       // X-04: the camera's track keeps a turn of half a degree
const double TIME_KEY = 0.5;            // and the clock's a drift of half a second of game time
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

// the layer the probe found itself in at a clock (galaxy/probe.h's `giantFoundLayer`)
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
    const double el = sys.sunless() ? 0.0 : dot(up, normalize(sys.star.pos - sys.bodyPos(probe.body, tt)));   // D-01: no sun over a drifter's clouds (it sits on the anchor)
    return giantSunlightAt(a, bar, cut) * std::max(0.0, el) + giantGlowAt(a, bar, cut);
}

// a key's name (yours, else the inbox's), else UNKNOWN: a world of another system
std::string Game::nameOfKey(const std::string& key) const {
    auto it = guide.names.find(key);
    if (it != guide.names.end()) return it->second;
    it = guide.inbox.find(key);
    return it != guide.inbox.end() ? it->second : std::string("UNKNOWN");
}

// the storm's name, else "" (the great storm, a storm: the explorer's or the inbox's)
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
    writePNGAsync(fn.c_str(), rgbBuf.data(), FBW, FBH);   // F-02: on the writer's thread (the gallery flushes before it lists)
    if (FILE* f = fopen((fn.substr(0, fn.size() - 4) + ".txt").c_str(), "w")) { fprintf(f, "%s - %s\n", recordTitle(r).c_str(), recordLine(r).c_str()); fclose(f); }
    const int at = guide.probeIndexBySeed(r.seed);
    if (at >= 0) guide.probes[at] = r; else guide.probes.push_back(r);
    guide.save(guidePath);
}

// ---------------------------------------------------------------------------
// X-04: the recordings
// ---------------------------------------------------------------------------

// the call with the recording's probe, system and sky in place
template <class F> void Game::asRecording(F f) {
    std::swap(probe, replay.p);
    if (!replay.here) { std::swap(sys, replay.sys); std::swap(nb, replay.nb); }
    f();
    if (!replay.here) { std::swap(sys, replay.sys); std::swap(nb, replay.nb); }
    std::swap(probe, replay.p);
}

// the record's probe, its system when it is another star's: false when it cannot be drawn
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
    if (!placeInSector(sx, sy, sz, s, true)) { refuse("THE GALAXY WAS REBUILT SINCE THIS RECORDING - ITS STAR IS GONE", 5); return false; }
    const bool here = sys.star.sx == sx && sys.star.sy == sy && sys.star.sz == sz && !sys.bodies.empty();
    if (!here && (replay.here || replay.sys.star.seed != s.seed || replay.sys.bodies.empty())) {
        replay.sys = StarSystem(); replay.sys.generate(s); replay.sys.valid = true;
        replay.nb = StarNeighborhood(); replay.nb.update(s.pos);
        replay.rec = -1;
    }
    replay.here = here;
    const StarSystem& S = here ? sys : replay.sys;
    if (bi < 0 || bi >= (int)S.bodies.size() || !isProbeGiant(S.bodies[bi].type)) { refuse("THE GIANT OF THIS RECORDING IS GONE: THE WORLDS WERE REGENERATED", 5); return false; }
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

// X-04: a probe's recording on the screen (`GameState::RECORDING`, opened from the gallery): the descent drawn again from the
// record's numbers by the probe's own functions, the record's probe (and another star's system and sky) swapped in for the call
// the screen, from the start
bool Game::openRecording(int rec) {
    if (!recordingContext(rec)) return false;
    replay.clock = 0; replay.playing = true; replay.fast = false; replay.lookYaw = replay.lookPitch = 0;
    replay.p.lastFlashSlot = -1;
    state = GameState::RECORDING;
    status(fmt("THE RECORDING OF %s", recordTitle(guide.probes[rec]).c_str()), 4);
    audio.beep = BEEP_UI;
    return true;
}

void Game::updateRecording(const Input& in, double realDt) {
    if (replay.rec < 0 || replay.rec >= (int)guide.probes.size()) { state = GameState::GALLERY; return; }
    const ProbeRecord& r = guide.probes[replay.rec];
    if (in.wasPressed(KEY_ESCAPE) || enterKey(in)) { state = GameState::GALLERY; galleryLoaded = -1; audio.beep = BEEP_UI; return; }
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
        if (r.storm.empty()) { refuse("THIS PROBE FELL INTO NO STORM", 3); }
        else { textStormKey = r.storm; guideReturn = GameState::RECORDING; const std::string n = nameOfKey(r.storm); beginTextEntry("NAME THE STORM", TEXT_STORM, n == "UNKNOWN" ? std::string() : upper(n)); }
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

// the depth bar's place of a clock (0 the launch .. 1 the end)
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

// "PROBE 3 INTO <giant>"
std::string Game::recordTitle(const ProbeRecord& r) const { return fmt("PROBE %d INTO %s", r.id, trunc(upper(nameOfKey(r.body)), 20).c_str()); }

// the date, the depth and the end
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
