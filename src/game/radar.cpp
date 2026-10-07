// C-07 (2026-10-02): the signal radar on the Stardrifter (`galaxy/signals.h` for what there is to hear). `B` in space (or the
// radar set beside the decoder, or the flight computer's devices page) enters the radar camera: the ship's own instrument, seen
// through like the vehicles' nose cameras (the cabin left behind, the mouse turning the ship, the picture cold and full of static
// that thins as a signal comes through); the meter reads the beam's gains over the static, a wide lobe rising within forty degrees
// of a source, a narrow one within three, so a sweep finds a rise and turning onto it sharpens it; the scope keeps the last seconds
// of the meter. Held within three degrees for a few seconds (longer for a faint one) the beam locks: the readout names only the
// system the signal comes from (the star, its sector, the distance: the signal's age; never the world or what sent it), the lock's
// marker sits on it, and Enter locks the star and flies there (a body of this system: the fine approach). A people's world in the
// beam is heard through the static before the lock: its recording on the air (`transmittedShard`, one of its fifty, the next when
// it ends), a voice or a piece of the world's, ghosted by the receiver, so the people are known by their voice first and the
// nature of a signal by its sound; what the radar hears goes into the guide (`heard`), and the shard found later in the ruins is
// "the recording the radar caught".
#include "game.h"
#include "ui.h"
#include "galaxy/planetmap.h"
#include "galaxy/ruins.h"
#include "core/rng.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace {
const double LOCK_CONE = 3 * DEG, KEEP_CONE = 8 * DEG;
}

void Game::radarToggle() {
    if (radar.on) { radarOff(); status("RADAR CAMERA OFF", 3); audio.beep = 4; return; }
    if (ship.mode == ShipState::VIMANA) { status("NOTHING IS HEARD IN THE FLIGHT - THE RADAR WAITS FOR THE ARRIVAL", 4); audio.beep = 3; return; }
    if (tele.on) telescopeOff();   // W-06: one instrument at the window at a time
    radar.on = true; radar.holdT = 0; radar.locked = -1; radar.lostT = 0; radar.reading = 0; radar.trace.clear(); radar.clarity = 0; radar.best = -1;
    cabin.yaw = 0; cabin.pitch = 0;   // the camera is the ship's: the view is its attitude, turned with the mouse
    radarScan();
    status("RADAR CAMERA: SWEEP THE SKY, HOLD A RISE TO LOCK", 5);   // under 52 characters: the line's width
    audio.beep = 4;
}

void Game::radarOff() {
    radarStopContent();
    radar.on = false; radar.locked = -1; radar.holdT = 0; radar.best = -1; radar.bestGain = 0; radar.reading = 0; radar.clarity = 0; radar.lostT = 0;
    radar.all.clear(); radar.gain.clear(); radar.angle.clear();
    audio.radar = 0; audio.radarSignal = 0; audio.radarKind = -1;
}

// the far signals from the ship's sector (the transmitters within reach and the loud pulsars near); the current star's own are local
void Game::radarScan() {
    auto t0 = std::chrono::steady_clock::now();
    radar.scanSx = sectorOf(ship.pos.x); radar.scanSy = sectorOf(ship.pos.y); radar.scanSz = sectorOf(ship.pos.z);
    radar.scanCells = signalsNear(ship.pos, radar.far);
    if (sys.valid) radar.far.erase(std::remove_if(radar.far.begin(), radar.far.end(), [&](const Signal& s) { return s.star.seed == sys.star.seed; }), radar.far.end());
    radar.scanMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

// the world's slot on the air onto the synth through the receiver (`next`: the one after the current): the programme of the
// slot (`programmeFor`: the recording spoken, whispered, chanted or looped, a numbers station, a machine of the world's, or
// rarely its piece slowed), in a voice derived from the world's
void Game::radarStartContent(const Signal& s, bool next) {
    RadarState& r = radar;
    if (s.kind != SIG_PEOPLE || s.body < 0) return;
    auto build = [&](int which) {   // built once per world and people; a world that fails to build is not tried again every frame
        r.world.key = Guide::bodyKey(s.star.sx, s.star.sy, s.star.sz, s.body);
        Star full = s.star; if (full.name.empty()) starInSector(s.star.sx, s.star.sy, s.star.sz, full, true);
        buildShardWorld(r.world, full, s.body, which);
        r.worldSeed = s.seed; r.worldWhich = which;
    };
    if (r.worldSeed != s.seed) { build(0); r.shard = -1; }
    if (!r.world.valid) { r.contentT = -1; return; }
    audio.piece = nullptr; audio.tradition = nullptr; audio.speech = nullptr; audio.voice = nullptr; audio.pieceSpeed = 1;
    int slot = next && r.shard >= 0 ? nextTransmittedShard(s.seed, shardLocalOf(r.shard)) : transmittedShard(s.seed, t);
    if (slot < 0 || slot >= SHARDS_PER_WORLD) return;
    // C-13: a world of two peoples has both on the air, the slot's people by a coin of the slot's; the world's machines are one set
    int which = r.world.peoples > 1 && (mix64(s.seed ^ ((uint64_t)(slot + 1) * 0x9E3779B97F4A7C15ULL) ^ 0xC13A1ULL) & 1) ? 1 : 0;
    if (which != r.worldWhich) { build(which); if (!r.world.valid) { r.contentT = -1; return; } }
    int idx = shardWorldIndex(which, slot);   // the world index: the guide's `heard` key and the programme's slot
    r.shard = idx;
    const Shard& sh = r.world.shards[slot];
    std::vector<DecodedWord> words;
    if (!sh.music) decodeShard(sh, r.world.lang, r.world.tongue, SHARDS_PER_WORLD, words);
    programmeFor(s.seed, idx, sh.music, sh.year, r.world.voice, r.world.tongue, words, r.prog);
    r.contentMusic = r.prog.kind == RP_MUSIC; r.loopI = 0; r.loopAt = 0;
    audio.radarVoice = r.prog.kind; audio.radarSeed = r.prog.seed;
    if (r.prog.kind == RP_MUSIC) {
        pieceOf(r.world.tradition, sh, r.world.piece);
        audio.piece = &r.world.piece; audio.tradition = &r.world.tradition; audio.pieceSpeed = r.prog.pieceSpeed;
        audio.pieceStart = true; audio.piecePause = false; audio.pieceDone = false; audio.pieceBeat = -1;
        r.contentSecs = r.world.piece.seconds / r.prog.pieceSpeed;
    } else if (r.prog.machine) {
        r.contentSecs = r.prog.seconds;
    } else {
        speechOf(r.prog.voice, r.prog.words, sh.seed, r.world.speech);
        audio.speech = &r.world.speech; audio.voice = &r.prog.voice;
        audio.speechStart = true; audio.speechPause = false; audio.speechDone = false; audio.speechT = -1;
        r.contentSecs = r.prog.repeats * (r.world.speech.seconds + r.prog.gap) - r.prog.gap;
    }
    audio.radio = true; r.contentT = 0; r.awayT = 0;
    if (r.prog.speaks && r.locked >= 0 && r.locked < (int)r.all.size() && r.all[r.locked].seed == s.seed) { guide.heard.insert(r.world.key + "/S" + std::to_string(idx)); guide.save(guidePath); }
}

// a loop's phrase or a numbers station's next group again after the gap (on the game's own clock: the harness never runs the synth)
void Game::radarRepeatContent() {
    RadarState& r = radar;
    if (r.prog.repeats <= 1 || r.loopI >= r.prog.repeats - 1 || !audio.speech) return;
    if (r.contentT - r.loopAt < r.world.speech.seconds + r.prog.gap) return;
    r.loopI++; r.loopAt = r.contentT;
    std::vector<DecodedWord> words; programmeRepeatWords(r.prog, r.loopI, r.world.tongue, words);
    const Shard& sh = r.world.shards[shardLocalOf(r.shard)];
    audio.speech = nullptr;   // the synth drops the old one before the words move
    speechOf(r.prog.voice, words, sh.seed ^ (uint64_t)(r.loopI * 0x9E37), r.world.speech);
    audio.speech = &r.world.speech; audio.speechStart = true; audio.speechDone = false; audio.speechT = -1;
}

void Game::radarStopContent() {
    if (audio.radio) { audio.piece = nullptr; audio.tradition = nullptr; audio.speech = nullptr; audio.voice = nullptr; audio.pieceStart = false; audio.speechStart = false; }
    audio.radio = false; audio.radarVoice = -1; audio.pieceSpeed = 1;
    radar.contentT = -1; radar.shard = -1; radar.awayT = 0; radar.prog = Programme(); radar.loopI = 0;
}

void Game::radarLock(int idx) {
    RadarState& r = radar;
    if (idx < 0 || idx >= (int)r.all.size()) return;
    const Signal& s = r.all[idx];
    r.locked = idx; r.lockedSeed = s.seed; r.lockedKind = s.kind; r.lostT = 0; r.holdT = 0;
    std::string key = s.kind == SIG_PULSAR ? Guide::starKey(s.star.sx, s.star.sy, s.star.sz) : Guide::bodyKey(s.star.sx, s.star.sy, s.star.sz, s.body);
    bool fresh = !guide.signals.count(key);
    guide.signals[key] = s.kind;
    bool content = s.kind == SIG_PEOPLE && r.contentT >= 0 && r.worldSeed == s.seed && r.shard >= 0;
    if (content) guide.heard.insert(r.world.key + "/S" + std::to_string(r.shard));
    // the log and the status name the system only: what sent the signal is heard, not told, and found on arrival
    Star full = s.star; if (full.name.empty()) starInSector(s.star.sx, s.star.sy, s.star.sz, full, true);
    std::string what = s.local ? fmt("A SIGNAL FROM THIS SYSTEM: %s", upper(bodyNameOf(s.body)).c_str())
                               : fmt("A SIGNAL FROM %s (%s), %.0f LY (%.0f YEARS OLD)", upper(starNameOf(full)).c_str(), STAR_CLASSES[full.cls].code, s.distLy, s.distLy);
    (void)content;
    if (fresh) logEvent("SIGNAL", fmt("%s, SECTOR %lld %lld %lld", what.c_str(), (long long)s.star.sx, (long long)s.star.sy, (long long)s.star.sz));
    else guide.save(guidePath);
    status(s.local ? "A SIGNAL LOCKED - ENTER APPROACHES ITS SOURCE" : (guide.visited.count(starKeyOf(full)) ? "A SIGNAL LOCKED - ITS STAR WAS VISITED, ENTER FLIES" : "A SIGNAL LOCKED - ENTER FLIES TO ITS STAR"), 6);   // R-407
    audio.beep = 1;
}

void Game::radarAccept() {
    RadarState& r = radar;
    if (r.locked < 0 || r.locked >= (int)r.all.size()) return;
    const Signal s = r.all[r.locked];   // a copy: the radar goes off below
    if (s.local) {   // a body of this system: the fine approach to it, the camera left
        radarOff();
        ship.localTarget = s.body; ship.targetBelt = -1;
        startApproach(s.body);
    } else {   // the star: the remote target, and the Vimana flight at once (which leaves the camera)
        Star full = s.star; starInSector(s.star.sx, s.star.sy, s.star.sz, full, true);
        setRemoteStar(full);
        if (ship.hasRemote && ship.remote.seed == full.seed) toggleVimana();
    }
}

// the sweep: the beam follows the view, the meter the beam; a hold within three degrees locks; true when Enter accepted a lock
bool Game::updateRadar(const Input& in, double realDt) {
    RadarState& r = radar;
    if (!r.on) { audio.radar = 0; audio.radarSignal = 0; audio.radarKind = -1; return false; }
    if (sectorOf(ship.pos.x) != r.scanSx || sectorOf(ship.pos.y) != r.scanSy || sectorOf(ship.pos.z) != r.scanSz) radarScan();
    localSignals(sys, ship.pos, t, r.local);
    r.all.clear(); r.all.insert(r.all.end(), r.far.begin(), r.far.end()); r.all.insert(r.all.end(), r.local.begin(), r.local.end());
    int n = (int)r.all.size();
    Mat3 cam = viewBasis();
    Vec3 fwd(cam.m[2][0], cam.m[2][1], cam.m[2][2]);
    r.gain.assign(n, 0.0); r.angle.assign(n, PI);
    double sum = 0; r.best = -1; r.bestGain = 0; r.bestAngle = PI;
    for (int i = 0; i < n; i++) {
        Vec3 dir = normalize(r.all[i].pos - ship.pos);
        double ang = std::acos(clampd(dot(fwd, dir), -1, 1));
        double g = beamGain(ang) * r.all[i].strength;
        r.gain[i] = g; r.angle[i] = ang; sum += g;
        if (g > r.bestGain) { r.bestGain = g; r.best = i; r.bestAngle = ang; }
    }
    // the static: a slow wander and a jitter under everything; the meter follows with a little lag
    r.noiseWalk = clampd(r.noiseWalk + (r.rng.uni() - 0.5) * realDt * 1.2, -1, 1);
    double noise = 0.07 + 0.035 * r.noiseWalk + 0.03 * (r.rng.uni() - 0.5);
    double target = clampd(sum + noise, 0, 1);
    r.reading += (target - r.reading) * (1 - std::exp(-realDt * 12));
    r.traceAccum += realDt;
    while (r.traceAccum >= 1.0 / 40) { r.traceAccum -= 1.0 / 40; r.trace.push_back((float)r.reading); if (r.trace.size() > 158) r.trace.erase(r.trace.begin()); }
    // the lock held, or lost when the beam wanders eight degrees off
    if (r.locked >= 0) {
        int idx = -1;
        for (int i = 0; i < n; i++) if (r.all[i].seed == r.lockedSeed && r.all[i].kind == r.lockedKind) idx = i;
        r.locked = idx;
        if (idx < 0 || r.angle[idx] > KEEP_CONE) {
            r.lostT += realDt;
            if (idx < 0 || r.lostT > 1.5) { r.locked = -1; r.holdT = 0; r.lostT = 0; status("LOCK LOST - THE BEAM LEFT THE SIGNAL", 3); audio.beep = 3; }
        } else r.lostT = 0;
    }
    if (r.locked < 0) {
        if (r.best >= 0 && r.bestAngle < LOCK_CONE) {
            r.lockSecs = 2.5 + 3.0 * (1 - r.all[r.best].strength);
            r.holdT += realDt;
            if (r.holdT >= r.lockSecs) radarLock(r.best);
        } else r.holdT = std::max(0.0, r.holdT - 2 * realDt);
    }
    // what is heard: the locked signal, else the strongest within the middle lobe; a people's recording plays through the static
    int src = r.locked >= 0 ? r.locked : r.best;
    const Signal* sig = src >= 0 && (r.locked >= 0 || beamGain(r.angle[src]) > 0.25) ? &r.all[src] : nullptr;
    if (sig && sig->kind == SIG_PEOPLE) {
        bool same = r.contentT >= 0 && r.worldSeed == sig->seed && audio.radio;
        if (!same) radarStartContent(*sig, false);
        else {
            r.contentT += realDt;
            radarRepeatContent();
            bool spoken = audio.speech && audio.speechDone && r.loopI >= r.prog.repeats - 1;
            if (r.contentT > r.contentSecs + 1.2 || audio.pieceDone || spoken) radarStartContent(*sig, true);
        }
        r.awayT = 0;
    } else if (r.contentT >= 0) {
        r.awayT += realDt; r.contentT += realDt;
        if (r.awayT > 2.0 || (sig && sig->kind != SIG_PEOPLE)) radarStopContent();
    }
    if (!(sig && sig->kind == SIG_PEOPLE && r.contentT >= 0)) { audio.radarSeed = sig ? sig->seed : 0; audio.radarVoice = -1; }   // the signal's own voice by its seed; a people's carrier alone until its programme is on
    double want = 0;
    if (sig) { want = clampd(beamGain(r.angle[src]) * (0.5 + 0.5 * sig->strength), 0, 1); if (r.locked >= 0) want = std::max(want, 0.85); }
    r.clarity += (want - r.clarity) * (1 - std::exp(-realDt * 4));
    audio.radar = 1; audio.radarSignal = r.clarity;
    audio.radarKind = sig ? sig->kind : -1;
    audio.radarPulseHz = sig && sig->kind == SIG_PULSAR ? std::max(0.2, sig->pulseHz) : 1;
    if (r.locked >= 0 && enterKey(in) && !ship.targeting) { radarAccept(); return true; }   // while aiming at a star, Enter is the aim's
    return false;
}

// the picture as the radar camera gives it: a cold phosphor (the palette toward blue-green, lifted off black), scanlines, a band
// rolling down, a vignette, and static by the clarity (noise on every pixel, snow in flecks) that thins as a signal comes through
void Game::radarCameraFeed(Framebuffer& fb) {
    double statik = 1 - 0.8 * clampd(radar.clarity, 0, 1);
    for (int i = 0; i < BANKS * 64; i++) {
        uint8_t* c = fb.pal + i * 3;
        double lum = 0.3 * c[0] + 0.59 * c[1] + 0.11 * c[2];
        c[0] = (uint8_t)clampd(10 + lum * 0.55, 0, 255); c[1] = (uint8_t)clampd(14 + lum * 1.0, 0, 255); c[2] = (uint8_t)clampd(16 + lum * 0.92, 0, 255);
    }
    int S = FB_SCALE;
    static std::vector<float> vx, vy;
    if ((int)vx.size() != FBW) { vx.resize(FBW); for (int x = 0; x < FBW; x++) { double u = (x + 0.5) / FBW * 2 - 1; vx[x] = (float)(u * u); } }
    if ((int)vy.size() != FBH) { vy.resize(FBH); for (int y = 0; y < FBH; y++) { double u = (y + 0.5) / FBH * 2 - 1; vy[y] = (float)(u * u); } }
    int bandY = (int)(std::fmod(realTime * 0.17, 1.0) * (FBH + 40 * S)) - 20 * S;   // rolls down the picture every 6 s
    uint32_t rs = (uint32_t)(realTime * 60) * 2654435761u + 0x51A7u;
    int noiseAmp = (int)(40 + 150 * statik), snowOdds = (int)(2 + 40 * statik);   // intensity units (32 a shade); flecks per 4096 pixels
    for (int y = 0; y < FBH; y++) {
        double line = ((y / S) & 1) ? 0.8 : 1.0;
        double band = std::abs(y - bandY) < 10 * S ? 1.08 : 1.0;
        Pix* row = &fb.idx[(size_t)y * FBW];
        for (int x = 0; x < FBW; x++) {
            rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
            double vig = 1 - 0.5 * std::max(0.0, (double)(vx[x] + vy[y]) - 0.4);
            int inten = (int)(intenOf(row[x]) * line * band * vig) + (int)(rs % (2 * noiseAmp + 1)) - noiseAmp;
            if ((int)((rs >> 8) & 4095) < snowOdds) inten = std::max(inten, 32 * (28 + (int)((rs >> 20) % 24)));
            row[x] = pixI(bankOf(row[x]), inten);
        }
    }
}

// the camera's frame over the picture: the vehicles' brackets and channel, the ship's attitude as the camera's azimuth and
// elevation, the scope and the meter, the lock cone round the crosshair, the source's smudge within the middle lobe and its
// diamond once locked, the readout (the hint, the hold, or the lock: the system only)
void Game::renderRadarCamera() {
    const RadarState& r = radar;
    const uint32_t fc = HUD_WHITE, fd = HUD_DIM;
    int cx = UW / 2, cy = UH / 2;
    drawVisor(fd);
    {   // brackets inside the visor's
        int m = 14, l = 26;
        drawLineRGB(canvas, m, m, m + l, m, fc); drawLineRGB(canvas, m, m, m, m + l, fc);
        drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m - l, m, fc); drawLineRGB(canvas, UW - 1 - m, m, UW - 1 - m, m + l, fc);
        drawLineRGB(canvas, m, UH - 1 - m, m + l, UH - 1 - m, fc); drawLineRGB(canvas, m, UH - 1 - m, m, UH - 1 - m - l, fc);
        drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m - l, UH - 1 - m, fc); drawLineRGB(canvas, UW - 1 - m, UH - 1 - m, UW - 1 - m, UH - 1 - m - l, fc);
    }
    drawTextShadow(canvas, 8, 16, "CAM 03 RADAR", fc, HUD_SHADOW);
    if ((int)(realTime * 2) & 1) drawTextShadow(canvas, 8 + textWidth("CAM 03 RADAR") + 6, 16, "\x07REC", HUD_RED, HUD_SHADOW);
    drawTextShadow(canvas, 8, 24, fmt("AZ %03.0f  EL %+03.0f", wrap2pi(ship.yaw) / DEG, ship.pitch / DEG).c_str(), fd, HUD_SHADOW);
    drawTextShadow(canvas, UW - 8 - textWidth(epocString().c_str()), 8, epocString().c_str(), HUD_GREEN, HUD_SHADOW);
    std::string sysName = sys.valid ? trunc(upper(starNameOf(sys.star)), 20) : "INTERSTELLAR SPACE";
    drawTextShadow(canvas, UW - 8 - textWidth(sysName.c_str()), 16, sysName.c_str(), fd, HUD_SHADOW);
    double pxPerRad = spaceR.proj.f / FB_SCALE;
    auto arc = [&](double rad, double from, double to, uint32_t col) {   // fractions of the turn from the top, clockwise
        int segs = 40; double a0 = -PI / 2 + from * TAU, a1 = -PI / 2 + to * TAU;
        int px = (int)std::round(cx + rad * std::cos(a0)), py = (int)std::round(cy + rad * std::sin(a0));
        for (int k = 1; k <= segs; k++) {
            double a = a0 + (a1 - a0) * k / segs;
            int x = (int)std::round(cx + rad * std::cos(a)), y = (int)std::round(cy + rad * std::sin(a));
            drawLineRGB(canvas, px, py, x, y, col); px = x; py = y;
        }
    };
    {   // crosshair and the lock cone: dim while sweeping, cyan with a white arc filling while the hold lasts, amber once locked
        drawLineRGB(canvas, cx - 10, cy, cx - 4, cy, fd); drawLineRGB(canvas, cx + 4, cy, cx + 10, cy, fd);
        drawLineRGB(canvas, cx, cy - 8, cx, cy - 3, fd); drawLineRGB(canvas, cx, cy + 3, cx, cy + 8, fd);
        double rad = pxPerRad * std::tan(LOCK_CONE);
        arc(rad, 0, 1, r.locked >= 0 ? HUD_AMBER : (r.holdT > 0 ? HUD_CYAN : fd));
        if (r.locked < 0 && r.holdT > 0) arc(rad + 2, 0, clampd(r.holdT / std::max(r.lockSecs, 0.1), 0, 1), HUD_WHITE);
    }
    {   // the scope: the meter's last four seconds, and the meter beside it
        int bx0 = cx - 80, bx1 = cx + 80, by0 = 36, by1 = 56;
        blendRectRGB(canvas, bx0 - 2, by0 - 2, bx1 + 14, by1 + 2, rgb(0, 0, 0), 150);
        drawRectRGB(canvas, bx0, by0, bx1, by1, fd);
        int h = by1 - by0 - 2;
        for (size_t i = 1; i < r.trace.size(); i++) {
            int x0 = bx0 + 1 + (int)(i - 1) + (158 - (int)r.trace.size()), x1 = x0 + 1;
            int y0 = by1 - 1 - (int)(clampd(r.trace[i - 1], 0, 1) * h), y1 = by1 - 1 - (int)(clampd(r.trace[i], 0, 1) * h);
            drawLineRGB(canvas, x0, y0, x1, y1, HUD_GREEN);
        }
        int mh = (int)(clampd(r.reading, 0, 1) * h);
        drawRectRGB(canvas, bx1 + 4, by0, bx1 + 12, by1, fd);
        if (mh > 0) fillRectRGB(canvas, bx1 + 5, by1 - 1 - mh, bx1 + 11, by1 - 1, r.locked >= 0 ? HUD_AMBER : HUD_GREEN);
        std::string right = r.locked >= 0 ? "LOCKED" : (r.holdT > 0 ? "LOCKING" : "SWEEP");
        drawText(canvas, bx1 + 12 - textWidth(right.c_str()), by1 + 4, right.c_str(), r.locked >= 0 ? HUD_AMBER : fd);
        drawText(canvas, bx0, by1 + 4, "RECEIVER", fd);
    }
    // the source: a smudge of static where the beam's middle lobe has it (found, not handed: nothing shows outside ten degrees),
    // the lock's diamond once locked; an edge mark when it is behind
    int src = r.locked >= 0 ? r.locked : r.best;
    if (src >= 0 && src < (int)r.all.size()) {
        const Signal& s = r.all[src];
        Mat3 cam = viewBasis();
        Vec3 v = cam * (s.pos - ship.pos);
        double g = beamGain(r.angle[src]);
        if (v.z > 0) {
            int sx = (int)(UW / 2 + spaceR.proj.f * v.x / v.z / FB_SCALE), sy = (int)(UH / 2 - spaceR.proj.f * v.y / v.z / FB_SCALE);
            if (sx > 4 && sy > 4 && sx < UW - 4 && sy < UH - 4) {
                if (r.locked >= 0) {
                    drawLineRGB(canvas, sx - 5, sy, sx, sy - 5, HUD_AMBER); drawLineRGB(canvas, sx, sy - 5, sx + 5, sy, HUD_AMBER);
                    drawLineRGB(canvas, sx + 5, sy, sx, sy + 5, HUD_AMBER); drawLineRGB(canvas, sx, sy + 5, sx - 5, sy, HUD_AMBER);
                } else if (g > 0.35) {
                    double dens = (g - 0.35) / 0.65 * s.strength;
                    uint32_t rs = (uint32_t)(realTime * 30) * 2654435761u + (uint32_t)src;
                    int rad = 4 + (int)(5 * dens);
                    for (int dy = -rad; dy <= rad; dy++) for (int dx = -rad; dx <= rad; dx++) {
                        if (dx * dx + dy * dy > rad * rad) continue;
                        rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
                        int x = sx + dx, y = sy + dy;
                        if (x < 0 || y < 0 || x >= canvas.w / canvas.scale || y >= canvas.h / canvas.scale) continue;
                        if ((rs & 255) < (uint32_t)(60 + 120 * dens) * (uint32_t)(rad * rad - dx * dx - dy * dy) / (uint32_t)(rad * rad)) fillRectRGB(canvas, x, y, x, y, HUD_GREEN);
                    }
                }
            }
        } else if (r.locked >= 0) { double ang = std::atan2(-v.y, v.x); drawText(canvas, UW / 2 + (int)(std::cos(ang) * 140) - 2, UH / 2 + (int)(std::sin(ang) * 85) - 3, "+", HUD_AMBER); }
    }
    // the readout: the system only
    std::string l1, l2, l3; uint32_t c1 = fd;
    if (r.locked >= 0 && r.locked < (int)r.all.size()) {
        const Signal& s = r.all[r.locked];
        c1 = HUD_AMBER;
        if (s.local) {
            l1 = "A SIGNAL FROM THIS SYSTEM";
            l2 = fmt("%s", trunc(upper(bodyNameOf(s.body)), 30).c_str());
            l3 = "ENTER APPROACHES IT";
        } else {
            Star full = s.star; if (full.name.empty()) starInSector(s.star.sx, s.star.sy, s.star.sz, full, true);
            l1 = fmt("A SIGNAL FROM %s (%s)  %.0f LY%s", trunc(upper(starNameOf(full)), 16).c_str(), STAR_CLASSES[full.cls].code, s.distLy, guide.visited.count(starKeyOf(full)) ? "  VISITED" : "");   // R-407: a loop back to a system seen says so
            l2 = fmt("SECTOR %lld %lld %lld - THE SIGNAL IS %.0f YEARS OLD", (long long)s.star.sx, (long long)s.star.sy, (long long)s.star.sz, s.distLy);
            l3 = "ENTER LOCKS THE STAR AND FLIES THERE";
        }
    } else if (r.holdT > 0) { l1 = fmt("LOCKING %.0f%% - HOLD STEADY", 100 * clampd(r.holdT / std::max(r.lockSecs, 0.1), 0, 1)); c1 = HUD_CYAN; }
    else if (r.best >= 0 && r.bestGain > 0.06) { l1 = "A RISE - CENTRE IT AND HOLD TO LOCK"; c1 = HUD_GREEN; }
    else l1 = "SWEEP THE SKY - A RISE ON THE METER IS A SIGNAL";
    drawTextCentered(canvas, cx, UH - 54, l1.c_str(), c1);
    if (!l2.empty()) drawTextCentered(canvas, cx, UH - 46, l2.c_str(), fc);
    if (!l3.empty()) drawTextCentered(canvas, cx, UH - 38, l3.c_str(), HUD_GREEN);
    drawTextShadow(canvas, 8, UH - 20, "B / ESC LEAVES THE CAMERA", fd, HUD_SHADOW);
}

bool Game::testCivilisedHere() const {
    if (!sys.valid) return false;
    for (const Body& b : sys.bodies) if ((b.type == PT_FELISIAN || b.type == PT_DESERT) && worldHadCivilisation(BodyGen::make(b))) return true;
    return false;
}

bool Game::testAimAtSignal(int which) {
    if (which < 0 || which >= (int)radar.all.size()) return false;
    Vec3 fwd = normalize(radar.all[which].pos - ship.pos);
    ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1));
    cabin.yaw = 0; cabin.pitch = 0;
    return true;
}

std::string Game::testRadarInfo() const {
    const RadarState& r = radar;
    std::string s = fmt("radar: %s, %zu far (%d cells, %.1f ms) + %zu local, reading %.2f", r.on ? "on" : "off", r.far.size(), r.scanCells, r.scanMs, r.local.size(), r.reading);
    if (r.best >= 0 && r.best < (int)r.all.size()) s += fmt(", best %d (%s, %.1f deg off, gain %.2f, %.0f ly, strength %.2f)", r.best, SIGNAL_KIND_NAMES[r.all[r.best].kind], r.bestAngle / DEG, r.bestGain, r.all[r.best].distLy, r.all[r.best].strength);
    s += fmt(", hold %.1f of %.1f s, locked %d", r.holdT, r.lockSecs, r.locked);
    if (r.locked >= 0 && r.locked < (int)r.all.size()) s += fmt(" (%s)", SIGNAL_KIND_NAMES[r.all[r.locked].kind]);
    s += fmt(", clarity %.2f, content %s", r.clarity, r.contentT < 0 ? "none" : fmt("S%d %s %.1f of %.0f s%s", r.shard, r.prog.kind >= 0 && r.prog.kind < RP_COUNT ? RADIO_PROGRAMME_NAMES[r.prog.kind] : "?", r.contentT, r.contentSecs, audio.radio ? (audio.piece ? " (a piece set)" : (audio.speech ? (r.prog.repeats > 1 ? fmt(" (a speech set, repeat %d of %d)", r.loopI + 1, r.prog.repeats).c_str() : " (a speech set)") : (r.prog.machine ? " (a machine)" : " (NO pointer)"))) : " (not radio)").c_str());
    return s;
}
