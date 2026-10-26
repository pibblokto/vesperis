// C-07 (2026-10-02): the signal radar on the Stardrifter (`peoples/signals.h` for what there is to hear). `B` in space (or the
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
// C-16 (2026-10-09): what the gain lets in (R-410: "radio noise, creepy mixed sounds when you are trying to get really far signal").
// The receiver is C-07's; the gain is how much of the void comes through it: the floor rises (`AudioState::radarGainT`), the
// galaxy's own noise is in the beam (the band's brightness round it, `radarSkyAt`: loud toward the core, a far signal there the
// hardest to pull out), the loudest other signal in the wide lobe bleeds in under the one heard (`RadarState::under`: two at once,
// a heterodyne between two carriers that slides as the beam moves, at the top steps an intermodulation's ring), a far signal comes
// worn (`radarWear`), the static's events come oftener and stranger, a word heard before comes back (`radarWordStep`), the noise
// makes false rises that a hold empties (`radarRiseStep`, NOTHING THERE), and the picture's static, the radio sky's glow, a ghost
// of the star field and the tears grow with the gain. The sound tells; the readout never does. The camera's picture is
// radar_camera.cpp.
#include "game.h"
#include "radar_internal.h"
#include "ui.h"
#include "galaxy/planetmap.h"
#include "peoples/ruins.h"
#include "core/rng.h"
#include "core/parallel.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>

namespace {
// C-16: what a people's signal under another sounds like, by its age (its own programme is not built for a bleed: a machine of its
// age, or the shape of a voice)
int underVoiceOf(const Signal& s) {
    uint64_t h = mix64(s.seed ^ 0xC16D0ULL);
    switch (s.age) {
        case AGE_INDUSTRIAL: return (h & 1) ? RP_MORSE : RP_PHONOGRAPH;
        case AGE_FUTURISTIC: { static const int F[3] = {RP_DATA, RP_BEACON, RP_SYNTHETIC}; return F[h % 3]; }
        case AGE_FAR_FUTURE: return RP_MATHS;
        default: { static const int M[6] = {RP_VOICE, RP_MURMUR, RP_BEACON, RP_DATA, RP_TOLL, RP_SIREN}; return M[h % 6]; }
    }
}
}

void Game::radarToggle() {
    if (radar.on) { radarOff(); status("RADAR CAMERA OFF", 3); audio.beep = BEEP_UI; return; }
    if (ship.mode == ShipState::VIMANA) { refuse("NOTHING IS HEARD IN THE FLIGHT - THE RADAR WAITS FOR THE ARRIVAL"); return; }
    if (tele.on) telescopeOff();   // W-06: one instrument at the window at a time
    radar.on = true; radar.holdT = 0; radar.locked = -1; radar.lostT = 0; radar.reading = 0; radar.trace.clear(); radar.clarity = 0; radar.best = -1;
    cabin.yaw = 0; cabin.pitch = 0;   // the camera is the ship's: the view is its attitude, turned with the mouse
    radarScan();
    status("RADAR CAMERA: SWEEP THE SKY, HOLD A RISE TO LOCK", 5);   // under 52 characters: the line's width
    audio.beep = BEEP_UI;
}

void Game::radarOff() {
    radarStopContent();
    radar.on = false; radar.locked = -1; radar.holdT = 0; radar.best = -1; radar.bestGain = 0; radar.reading = 0; radar.clarity = 0; radar.lostT = 0;
    radar.all.clear(); radar.gain.clear(); radar.angle.clear(); radar.level.clear(); radar.aim = -1; radar.overload = false;   // C-15: the dial stays where it was
    radar.under = -1; radar.rise.on = false; radar.nothingT = 0;   // C-16
    radarWordStop();
    audio.radar = 0; audio.radarSignal = 0; audio.radarKind = -1; audio.radarDrive = 0;
    audio.radarGainT = 0; audio.radarSky = 0; audio.radarWear = 0; audio.radarGhost = 0; audio.radarAge = -1; audio.radarKind2 = -1; audio.radarSignal2 = 0; audio.radarHet = 0; audio.radarIntermod = 0;
}

// D-03: a drifter's voice off the camera: the cabin's radio while the ship is at a drifter, the suit's on its ground (each frame after the
// state's update); its voice read once a system
// D-03: a drifter's voice off the radar camera. While the ship is at a drifter (in its system, out of a flight) the cabin's radio carries it
// under the ship's hum; on its ground (the drifter's or a moon's: the explorer out of the capsule, walking, in the buggy or the drone, a sheet
// or the guide opened there) the suit's radio, fainter and narrow. Nothing where the synth is another's (the decoder, the instruments, the
// probe's relay and its recordings, the drone bay, the landing, the title) or while the camera is on (it hears the drifter itself)
void Game::darkRadio() {
    const bool taken = stateHas(state, ST_TAKES_SYNTH);
    if (radar.on || taken || !sys.valid || !sys.sunless() || sys.bodies.empty() || ship.mode == ShipState::VIMANA) {
        if (darkRadioOn && !radar.on) { audio.radar = 0; audio.radarSignal = 0; audio.radarKind = -1; audio.radarSet = RADIO_CAMERA; }
        darkRadioOn = false;
        return;
    }
    if (darkRadioSeed != sys.star.seed) {   // the drifter's voice, read once a system
        Signal s; s.kind = SIG_ROGUE; s.bodyType = sys.bodies[0].type; s.power = drifterPower(sys.star); rogueVoiceOf(sys, s);
        darkRadioVoice = rogueVoiceFor(s); darkRadioSeed = sys.star.seed; darkRadioBeacon = -2;
    }
    if (darkRadioBeacon == -2 && realTime - darkRadioBeaconAt >= 0.25) {   // D-05: its beacon's machine under it, once its people's records are written
        darkRadioBeaconAt = realTime;
        DrifterBeacon B; if (drifterBeaconIfReady(sys, B)) darkRadioBeacon = B.people >= 0 ? B.machine : -1;
    }
    const bool ground = state == GameState::SURFACE || (state != GameState::SPACE && surf.valid && returnState == GameState::SURFACE);
    const bool playing = player.playing >= 0 && !player.held && !player.paused;   // C-11: lower under the ship's player
    audio.radar = (ground ? RADIO_SUIT_LEVEL : RADIO_CABIN_LEVEL) * (playing ? 0.35 : 1.0); audio.radarSet = ground ? RADIO_SUIT : RADIO_CABIN;
    audio.radarSignal = 0.9; audio.radarKind = SIG_ROGUE; audio.radarSeed = sys.star.seed; audio.radarVoice = darkRadioBeacon >= 0 ? darkRadioBeacon : -1; audio.radarRogue = darkRadioVoice; audio.radarPulseHz = 1;
    audio.radarGainT = 0; audio.radarSky = 0; audio.radarWear = 0; audio.radarGhost = 0; audio.radarAge = -1; audio.radarDrive = 0;
    audio.radarKind2 = -1; audio.radarSignal2 = 0; audio.radarHet = 0; audio.radarIntermod = 0;
    darkRadioOn = true;
}

// the far signals from the ship's sector (the transmitters within reach and the loud pulsars near); the current star's own are local.
// C-15: what the gain hears beyond is the sector's too, scanned again (a slice a frame) when the gain is up
void Game::radarScan() {
    auto t0 = std::chrono::steady_clock::now();
    radar.scanSx = sectorOf(ship.pos.x); radar.scanSy = sectorOf(ship.pos.y); radar.scanSz = sectorOf(ship.pos.z);
    radar.scanCells = signalsNear(ship.pos, radar.far);
    if (sys.valid) radar.far.erase(std::remove_if(radar.far.begin(), radar.far.end(), [&](const Signal& s) { return s.star.seed == sys.star.seed; }), radar.far.end());
    radar.scanMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    radar.deep.clear(); radar.deepReady = false; radar.deepRunning = false;
}

// C-15: the dial a step up or down
// C-15: the dial, x1 to x32 in six steps (SIGNAL_GAINS); the reach is the transmitter's, not the radar's, so it is not printed
void Game::radarGainStep(int dir) {
    int s = clampi(radar.gainStep + dir, 0, SIGNAL_GAIN_STEPS - 1);
    if (s == radar.gainStep) { audio.beep = BEEP_ERROR; return; }
    radar.gainStep = s;
    status(fmt("RECEIVER GAIN X%d", SIGNAL_GAINS[s]), 2);
    audio.beep = BEEP_UI;
}

// C-15: the far scan from the ship's sector, the whole dial's at once (each signal kept with the least gain that hears it, so a
// step up or down is a filter), a slice of cells a frame so no frame carries it: begun when the gain first goes over x1 here
void Game::radarDeepStep() {
    RadarState& r = radar;
    if (!r.deepRunning) { r.deepScan.begin(ship.pos, SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]); r.deepRunning = true; r.deepMs = 0; r.deepFrames = 0; }
    auto t0 = std::chrono::steady_clock::now();
    bool done = r.deepScan.step(DEEP_SLICE);
    r.deepMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); r.deepFrames++;
    if (!done) return;
    r.deep = r.deepScan.out;
    if (sys.valid) r.deep.erase(std::remove_if(r.deep.begin(), r.deep.end(), [&](const Signal& s) { return s.star.seed == sys.star.seed; }), r.deep.end());
    r.deepRunning = false; r.deepReady = true;
}

// the world's slot on the air onto the synth through the receiver (`next`: the one after the current): the programme of the
// slot (`programmeFor`: the recording spoken, whispered, chanted or looped, a numbers station, a machine of the world's, or
// rarely its piece slowed), in a voice derived from the world's
void Game::radarStartContent(const Signal& s, bool next) {
    RadarState& r = radar;
    if (s.kind == SIG_ROGUE) { radarStartBeacon(s); return; }   // D-05: a drifter's beacon (the same message again when it ends)
    if (s.kind != SIG_PEOPLE || s.body < 0) return;
    auto build = [&](int which) {   // built once per world and people; a world that fails to build is not tried again every frame
        r.world.key = Guide::bodyKey(s.star.sx, s.star.sy, s.star.sz, s.body);
        Star full = s.star; if (full.name.empty()) placeInSector(s.star.sx, s.star.sy, s.star.sz, full, true);
        buildShardWorld(r.world, full, s.body, which, false);
        if (r.world.pending) { r.pendingSeed = s.seed; r.pendingAt = realTime; return false; }   // H-04: its records are being written: the carrier alone till then
        r.worldSeed = s.seed; r.worldWhich = which;
        return true;
    };
    if (r.worldSeed != s.seed) {
        if (r.pendingSeed == s.seed && realTime - r.pendingAt < 0.25) { r.contentT = -1; return; }   // asked again four times a second
        if (!build(0)) { r.contentT = -1; return; }
        r.shard = -1;
    }
    if (!r.world.valid) { r.contentT = -1; return; }
    radarWordStop();   // C-16: a word back from the static gives way to what is on the air
    audio.piece = nullptr; audio.tradition = nullptr; audio.speech = nullptr; audio.voice = nullptr; audio.pieceSpeed = 1; audio.radarCode = nullptr;
    int slot = next && r.shard >= 0 ? nextTransmittedShard(s.seed, shardLocalOf(r.shard)) : transmittedShard(s.seed, t);
    if (slot < 0 || slot >= SHARDS_PER_WORLD) return;
    // C-13: a world of two peoples has both on the air, the slot's people by a coin of the slot's; the world's machines are one set.
    // A-01: only the peoples that had radio (`slotPeople`): where one of the two had none, every slot is the other's
    int which = std::max(0, slotPeople(BodyGen::make(r.world.sys.bodies[r.world.body]), s.seed, slot));
    if (which != r.worldWhich) { if (!build(which)) { r.worldSeed = 0; r.contentT = -1; return; } if (!r.world.valid) { r.contentT = -1; return; } }
    int idx = shardWorldIndex(which, slot);   // the world index: the guide's `heard` key and the programme's slot
    r.shard = idx;
    const Shard& sh = r.world.shards[slot];
    std::vector<DecodedWord> words;
    if (!sh.music) { decodeShard(sh, r.world.lang, r.world.tongue, r.world.lore.records, words); words.resize(shardOpening(words)); }   // H-04: the opening of a long record
    // C-16: each age on the air: what the slot's people's age could broadcast (`programmeOf`)
    r.progAge = peopleAgeOf(BodyGen::make(r.world.sys.bodies[r.world.body]), which).age;
    programmeFor(s.seed, idx, sh.music, sh.year, r.world.voice, r.world.tongue, words, r.prog, -1, r.progAge);
    r.contentMusic = r.prog.piece; r.loopI = 0; r.loopAt = 0;
    audio.radarVoice = r.prog.kind; audio.radarSeed = r.prog.seed;
    if (r.prog.kind == RP_MORSE) { audio.radarCode = &r.prog.code; audio.radarCodeUnit = r.prog.unit; }
    if (r.prog.piece) {
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

// D-05: a drifter's beacon on the air: its radio people's last recording (`drifterBeaconIfReady`, decided once a drifter, the voice alone while
// its people's records are written), the recording's opening spoken in the people's voice slowed and worn, three times and a long gap, the
// machine under the drifter's voice all the while; locked, it is heard (`Guide::heard`): the one found in the last shelter is "the recording
// the radar caught"
void Game::radarStartBeacon(const Signal& s) {
    RadarState& r = radar;
    auto quiet = [&]() { if (r.contentT >= 0) radarStopContent(); r.contentT = -1; };   // nothing of another's left on the air (a word back out of the static stays)
    auto it = r.beacons.find(s.seed);
    if (it != r.beacons.end() && it->second.people < 0) { quiet(); return; }
    Star full = s.star; if (full.name.empty()) placeInSector(s.star.sx, s.star.sy, s.star.sz, full, true);
    if (it == r.beacons.end()) {
        if (r.pendingSeed == s.seed && realTime - r.pendingAt < 0.25) { quiet(); return; }   // asked again four times a second
        StarSystem dsys; dsys.generate(full);
        DrifterBeacon B;
        if (!drifterBeaconIfReady(dsys, B)) { r.pendingSeed = s.seed; r.pendingAt = realTime; quiet(); return; }
        it = r.beacons.emplace(s.seed, B).first;
    }
    const DrifterBeacon B = it->second;
    if (B.people < 0) { quiet(); return; }
    if (r.worldSeed != s.seed || r.worldWhich != B.people) {
        if (r.pendingSeed == s.seed && realTime - r.pendingAt < 0.25) { quiet(); return; }
        r.world.key = Guide::bodyKey(full.sx, full.sy, full.sz, 0);
        buildShardWorld(r.world, full, 0, B.people, false);
        if (r.world.pending) { r.pendingSeed = s.seed; r.pendingAt = realTime; quiet(); return; }
        r.worldSeed = s.seed; r.worldWhich = B.people; r.shard = -1;
    }
    const int local = r.world.lore.last;
    if (!r.world.valid || local < 0 || local >= (int)r.world.shards.size()) { quiet(); return; }
    radarWordStop();
    audio.piece = nullptr; audio.tradition = nullptr; audio.speech = nullptr; audio.voice = nullptr; audio.pieceSpeed = 1; audio.radarCode = nullptr;
    const int idx = shardWorldIndex(B.people, local);
    r.shard = idx;
    const Shard& sh = r.world.shards[local];
    std::vector<DecodedWord> words; decodeShard(sh, r.world.lang, r.world.tongue, r.world.lore.records, words); words.resize(shardOpening(words));
    r.progAge = -1;   // the drifter's channel: no age's band (its voice the drifter's)
    beaconProgramme(s.seed, B, r.world.voice, words, r.prog);
    r.contentMusic = false; r.loopI = 0; r.loopAt = 0;
    audio.radarVoice = r.prog.kind; audio.radarSeed = s.seed;   // the drifter's voice and its beacon's pattern by its seed
    speechOf(r.prog.voice, r.prog.words, sh.seed, r.world.speech);
    audio.speech = &r.world.speech; audio.voice = &r.prog.voice;
    audio.speechStart = true; audio.speechPause = false; audio.speechDone = false; audio.speechT = -1;
    r.contentSecs = r.prog.repeats * (r.world.speech.seconds + r.prog.gap) - r.prog.gap;
    audio.radio = true; r.contentT = 0; r.awayT = 0;
    if (r.locked >= 0 && r.locked < (int)r.all.size() && r.all[r.locked].seed == s.seed) { guide.heard.insert(r.world.key + "/S" + std::to_string(idx)); guide.save(guidePath); }
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
    audio.radio = false; audio.radarVoice = -1; audio.pieceSpeed = 1; audio.radarCode = nullptr; audio.radarGhostWord = false;
    radar.contentT = -1; radar.shard = -1; radar.awayT = 0; radar.prog = Programme(); radar.loopI = 0; radar.progAge = -1; radar.wordT = -1;
}

// C-16: the galaxy's own noise in the beam: the band's brightness (the map the sky already has) at the beam's centre and round it
// at ten degrees, so the receiver hears more toward the band and the core than toward the poles
double Game::radarSkyAt(const Vec3& dir) const {
    const std::vector<float>& band = spaceR.parkedBand();
    if (band.empty()) return 0;
    Vec3 f = normalize(dir), up = std::fabs(f.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    Vec3 a = normalize(cross(f, up)), b = cross(f, a);
    double s = 0.5 * sampleGalaxyBand(band, f), k = std::tan(10 * DEG);
    for (int i = 0; i < 6; i++) { double ang = i * TAU / 6; s += 0.5 / 6 * sampleGalaxyBand(band, normalize(f + (a * std::cos(ang) + b * std::sin(ang)) * k)); }
    return clampd(s, 0, 1);
}

// C-16: a false rise born, aged and gone
// C-16: false rises. At the top steps (x25, x32) the noise makes ghosts: a rise somewhere 8-30 degrees off the beam, living ten to
// sixteen seconds; held, the hold fills a third to two thirds of the way (as a faint signal's would) and empties (NOTHING THERE)
// and the ghost fades. Never a lock: nothing is there
void Game::radarRiseStep(const Vec3& fwd, double realDt) {
    RadarState& r = radar;
    RadarState::FalseRise& fr = r.rise;
    r.nothingT = std::max(0.0, r.nothingT - realDt);
    if (r.gainStep < SIGNAL_GAIN_STEPS - 2) { fr.on = false; r.riseGain = 0; r.riseAngle = PI; return; }
    if (!fr.on) {
        r.riseNext -= realDt;
        if (r.riseNext <= 0) {
            fr = RadarState::FalseRise(); fr.on = true;
            Vec3 up = std::fabs(fwd.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0), a = normalize(cross(fwd, up)), b = cross(fwd, a);
            double off = (8 + 22 * r.rng.uni()) * DEG, around = r.rng.uni() * TAU;
            fr.dir = normalize(fwd * std::cos(off) + (a * std::cos(around) + b * std::sin(around)) * std::sin(off));
            fr.life = 10 + 6 * r.rng.uni(); fr.peak = 0.16 + 0.14 * r.rng.uni(); fr.holdMax = 0.3 + 0.4 * r.rng.uni();
            r.rises++;
        }
    }
    r.riseGain = 0; r.riseAngle = PI;
    if (!fr.on) return;
    fr.t += realDt;
    if (fr.t >= fr.life) { fr.on = false; r.riseNext = r.gainStep == SIGNAL_GAIN_STEPS - 1 ? 12 + 18 * r.rng.uni() : 20 + 25 * r.rng.uni(); return; }
    r.riseAngle = std::acos(clampd(dot(fwd, fr.dir), -1, 1));
    r.riseGain = beamGain(r.riseAngle) * fr.peak * std::pow(std::sin(PI * fr.t / fr.life), 0.7);
}

// C-16: a word of a recording the explorer has heard (`Guide::heard`, never one not heard) comes back out of the static at the top
// steps, now and then, while nothing is on the air: one word, or two or three, in the world's own voice, as from very far; never logged
void Game::radarWordStep(double realDt) {
    RadarState& r = radar;
    if (r.wordT >= 0) {   // on the air until it is spoken (on the game's clock: the harness never runs the synth)
        r.wordT += realDt;
        if (r.wordT > r.wordSpeech.seconds + 0.8 || !audio.radarGhostWord) radarWordStop();
        return;
    }
    if (r.gainStep < SIGNAL_GAIN_STEPS - 2 || guide.heard.empty() || r.contentT >= 0 || audio.radio) return;
    r.wordNext -= realDt;
    if (r.wordNext > 0) return;
    r.wordNext = 35 + 55 * r.rng.uni();
    auto it = guide.heard.begin(); std::advance(it, std::min((int)(r.rng.uni() * guide.heard.size()), (int)guide.heard.size() - 1));
    const std::string key = *it;   // "<body key>/S<index>"
    size_t at = key.rfind("/S"), sl = key.find('/');
    if (at == std::string::npos || sl == std::string::npos || sl >= at) return;
    std::string bodyKey = key.substr(0, at); int idx = atoi(key.c_str() + at + 2), which = shardPeopleOf(idx), local = shardLocalOf(idx), bi = atoi(key.c_str() + sl + 1);
    ShardWorld* w = nullptr;
    if (r.world.valid && r.world.key == bodyKey && r.world.which == which) w = &r.world;
    else if (r.wordWorld.valid && r.wordWorld.key == bodyKey && r.wordWorld.which == which) w = &r.wordWorld;
    else {
        int64_t sx, sy, sz; Star st;
        if (!Guide::parseStarKey(bodyKey, sx, sy, sz) || !placeInSector(sx, sy, sz, st, true)) return;
        r.wordWorld = ShardWorld(); r.wordWorld.key = bodyKey;
        if (!buildShardWorld(r.wordWorld, st, bi, which, false)) return;   // H-04: not before its records are written (the next word's turn will find them)
        w = &r.wordWorld;
    }
    if (local < 0 || local >= (int)w->shards.size() || w->shards[local].music) return;   // a piece has no words: its turn passes
    const Shard& sh = w->shards[local];
    std::vector<DecodedWord> text, said; decodeShard(sh, w->lang, w->tongue, w->lore.records, text); text.resize(shardOpening(text));
    std::vector<int> spoken; for (int i = 0; i < (int)text.size(); i++) if (!text[i].theirs.empty()) spoken.push_back(i);
    if (spoken.empty()) return;
    int from = spoken[std::min((int)(r.rng.uni() * spoken.size()), (int)spoken.size() - 1)], n = r.rng.uni() < 0.6 ? 1 : 2 + (int)(r.rng.uni() * 1.99);
    for (int i = from; i < (int)text.size() && (int)said.size() < n; i++) if (!text[i].theirs.empty()) { said.push_back(text[i]); said.back().pre.clear(); said.back().post.clear(); }
    said.back().post = ".";
    r.wordVoice = w->voice;
    speechOf(r.wordVoice, said, sh.seed ^ 0xC16ULL, r.wordSpeech);
    audio.piece = nullptr; audio.tradition = nullptr;
    audio.speech = &r.wordSpeech; audio.voice = &r.wordVoice; audio.speechStart = true; audio.speechPause = false; audio.speechDone = false; audio.speechT = -1;
    audio.radio = true; audio.radarGhostWord = true;
    r.wordT = 0; r.words++; r.wordKey = key; r.wordSaid.clear(); for (const DecodedWord& d : said) r.wordSaid += (r.wordSaid.empty() ? "" : " ") + d.theirs;
}

void Game::radarWordStop() {
    if (audio.radarGhostWord) { audio.speech = nullptr; audio.voice = nullptr; audio.radio = false; audio.radarGhostWord = false; }
    radar.wordT = -1;
}

void Game::radarLock(int idx) {
    RadarState& r = radar;
    if (idx < 0 || idx >= (int)r.all.size()) return;
    const Signal& s = r.all[idx];
    r.locked = idx; r.lockedSeed = s.seed; r.lockedKind = s.kind; r.lostT = 0; r.holdT = 0;
    // D-02: a drifter's lock is its sector's, as a pulsar's: the chart's red ring and its A SIGNAL stop at the place
    std::string key = s.kind == SIG_PULSAR || s.kind == SIG_ROGUE ? Guide::starKey(s.star.sx, s.star.sy, s.star.sz) : Guide::bodyKey(s.star.sx, s.star.sy, s.star.sz, s.body);
    bool fresh = !guide.signals.count(key);
    guide.signals[key] = s.kind;
    bool content = (s.kind == SIG_PEOPLE || s.kind == SIG_ROGUE) && r.contentT >= 0 && r.worldSeed == s.seed && r.shard >= 0;   // D-05: a drifter's beacon too
    if (content) guide.heard.insert(r.world.key + "/S" + std::to_string(r.shard));
    // the log and the status name the system only: what sent the signal is heard, not told, and found on arrival
    Star full = s.star; if (full.name.empty()) placeInSector(s.star.sx, s.star.sy, s.star.sz, full, true);
    // D-02: a drifter's says there is no star at its source, and no more (its name only once the explorer gave one)
    const bool rogue = s.kind == SIG_ROGUE && !s.local, visited = guide.visited.count(starKeyOf(full)) > 0;
    std::string what = s.local ? fmt("A SIGNAL FROM THIS SYSTEM: %s", upper(bodyNameOf(s.body)).c_str())
                     : (rogue ? fmt("A SIGNAL WITH NO STAR AT ITS SOURCE%s, %.0f LY (%.0f YEARS OLD)", starNamed(full) ? (": " + upper(starNameOf(full))).c_str() : "", s.distLy, s.distLy)
                              : fmt("A SIGNAL FROM %s (%s), %.0f LY (%.0f YEARS OLD)", upper(starNameOf(full)).c_str(), STAR_CLASSES[full.cls].code, s.distLy, s.distLy));
    (void)content;
    if (fresh) logEvent("SIGNAL", fmt("%s, SECTOR %lld %lld %lld", what.c_str(), (long long)s.star.sx, (long long)s.star.sy, (long long)s.star.sz));
    else guide.save(guidePath);
    status(s.local ? "A SIGNAL LOCKED - ENTER APPROACHES ITS SOURCE"
                   : (rogue ? (visited ? "A SIGNAL LOCKED - NO STAR, VISITED - ENTER FLIES" : "A SIGNAL LOCKED - NO STAR THERE - ENTER FLIES")
                            : (visited ? "A SIGNAL LOCKED - ITS STAR WAS VISITED, ENTER FLIES" : "A SIGNAL LOCKED - ENTER FLIES TO ITS STAR")), 6);   // R-407
    audio.beep = BEEP_LOCK;
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
        Star full = s.star; placeInSector(s.star.sx, s.star.sy, s.star.sz, full, true);
        setRemoteStar(full);
        if (ship.hasRemote && ship.remote.seed == full.seed) toggleVimana();
    }
}

// the sweep: the beam follows the view, the meter the beam; a hold within three degrees locks; true when Enter accepted a lock
bool Game::updateRadar(const Input& in, double realDt) {
    RadarState& r = radar;
    if (!r.on) { audio.radar = 0; audio.radarSignal = 0; audio.radarKind = -1; audio.radarDrive = 0; return false; }
    if (in.wheel > 0 || in.wasPressed(KEY_EQUAL) || in.wasPressed(KEY_PAGE_UP)) radarGainStep(1);     // C-15: the dial (the telescope's keys: the two are never out together)
    if (in.wheel < 0 || in.wasPressed(KEY_MINUS) || in.wasPressed(KEY_PAGE_DOWN)) radarGainStep(-1);
    if (sectorOf(ship.pos.x) != r.scanSx || sectorOf(ship.pos.y) != r.scanSy || sectorOf(ship.pos.z) != r.scanSz) radarScan();
    if (r.gainStep > 0 && !r.deepReady) radarDeepStep();   // C-15: LISTENING DEEPER, a slice a frame
    const double G = radarGain();
    localSignals(sys, ship.pos, t, r.local);
    r.all.clear(); r.all.insert(r.all.end(), r.far.begin(), r.far.end());
    if (r.deepReady) for (const Signal& s : r.deep) {   // C-15: what this gain carries from beyond 200 ly (a star at the edge once: the two scans' points differ within the sector)
        if (s.needGain > G) continue;
        bool near = false; for (const Signal& f : r.far) if (f.star.seed == s.star.seed) near = true;
        if (!near) r.all.push_back(s);
    }
    r.all.insert(r.all.end(), r.local.begin(), r.local.end());
    int n = (int)r.all.size();
    Mat3 cam = viewBasis();
    Vec3 fwd(cam.m[2][0], cam.m[2][1], cam.m[2][2]);
    r.gain.assign(n, 0.0); r.angle.assign(n, PI); r.level.assign(n, 0.0);
    double sum = 0, aimGain = 0, cone = lockCone(G); r.best = -1; r.bestGain = 0; r.bestAngle = PI; r.aim = -1; r.aimAngle = PI;
    for (int i = 0; i < n; i++) {
        Vec3 dir = normalize(r.all[i].pos - ship.pos);
        double ang = std::acos(clampd(dot(fwd, dir), -1, 1));
        double L = signalLevel(r.all[i], G);   // C-07's strength at x1; C-15: a near one a little louder with the gain, a far one a small rise
        double g = beamGain(ang) * L;
        r.gain[i] = g; r.angle[i] = ang; r.level[i] = L; sum += g;
        if (g > r.bestGain) { r.bestGain = g; r.best = i; r.bestAngle = ang; }
        if (ang < cone && g > aimGain) { aimGain = g; r.aim = i; r.aimAngle = ang; }   // the hold's: the loudest at the crosshair
    }
    // the static: a slow wander and a jitter under everything; the meter follows with a little lag. C-15: the gain lifts the floor
    // (0.07 at x1, 0.37 at x32), widens its wander and fills the scope with grass, so a far signal is a small rise over it
    const double tG = gainShare(G);
    r.noiseWalk = clampd(r.noiseWalk + (r.rng.uni() - 0.5) * realDt * 1.2, -1, 1);
    double noise = 0.07 + 0.30 * tG + 0.035 * (1 + 2 * tG) * r.noiseWalk + 0.03 * (1 + 5 * tG) * (r.rng.uni() - 0.5);
    // C-16: the galaxy's own noise in the beam lifts the floor with the gain (up to 0.12 more toward the core at x32), and a false rise
    // at the top steps lifts the meter where nothing is
    r.sky = radarSkyAt(fwd);
    noise += 0.12 * tG * r.sky;
    radarRiseStep(fwd, realDt);
    sum += r.riseGain;
    r.overload = G > 1 && r.aim >= 0 && r.locked < 0 && signalDrive(r.all[r.aim], G) >= 1;   // the beam on a near signal the gain overdrives
    double target = clampd(sum + noise, 0, 1);
    if (G > 1 && target > 0.6) target = 0.6 + 0.38 * (1 - std::exp(-(target - 0.6) / 0.38));   // over x1 the meter's top is soft: full is an overload's alone
    if (r.overload) target = 1.0;
    r.sum = sum; r.floorNow = noise;
    r.reading += (target - r.reading) * (1 - std::exp(-realDt * 12));
    r.traceAccum += realDt;
    while (r.traceAccum >= 1.0 / 40) { r.traceAccum -= 1.0 / 40; r.trace.push_back((float)r.reading); if (r.trace.size() > 158) r.trace.erase(r.trace.begin()); }
    // the lock held, or lost when the beam wanders eight degrees off (C-15: four at x32), or the gain turned down under the signal
    if (r.locked >= 0) {
        int idx = -1;
        for (int i = 0; i < n; i++) if (r.all[i].seed == r.lockedSeed && r.all[i].kind == r.lockedKind) idx = i;
        r.locked = idx;
        if (idx < 0 || r.angle[idx] > keepCone(G)) {
            r.lostT += realDt;
            if (idx < 0 || r.lostT > 1.5) {
                bool sank = false; for (const Signal& d : r.deep) if (idx < 0 && d.seed == r.lockedSeed && d.kind == r.lockedKind && d.needGain > G) sank = true;
                r.locked = -1; r.holdT = 0; r.lostT = 0; refuse(sank ? "LOCK LOST - TOO FAINT AT THIS GAIN" : "LOCK LOST - THE BEAM LEFT THE SIGNAL", 3);
            }
        } else r.lostT = 0;
    }
    if (r.locked >= 0) r.overload = false;
    if (r.locked < 0) {   // C-15: the hold is on the loudest within the lock cone (at x1 C-07's strongest when it is there); an overload refuses it
        RadarState::FalseRise& fr = r.rise;
        if (r.aim >= 0) {
            r.overload = G > 1 && signalDrive(r.all[r.aim], G) >= 1;
            r.lockSecs = (2.5 + 3.0 * (1 - signalStrengthAtGain(r.all[r.aim], G))) * holdFactor(G);
            if (r.overload) r.holdT = std::max(0.0, r.holdT - 2 * realDt);   // the meter pinned: nothing to centre on
            else { r.holdT += realDt; if (r.holdT >= r.lockSecs) radarLock(r.aim); }
        } else if (fr.on && !fr.spent && r.riseAngle < cone) {   // C-16: a false rise held: the hold fills part way, as a faint one's would, and empties
            r.lockSecs = (2.5 + 3.0 * (1 - 0.22)) * holdFactor(G);
            if (!fr.held) { fr.held = true; r.riseHolds++; }
            r.holdT += realDt;
            if (r.holdT >= fr.holdMax * r.lockSecs) { fr.spent = true; r.nothingT = 2.5; r.nothings++; fr.t = std::max(fr.t, fr.life - 1.2); }
        } else r.holdT = std::max(0.0, r.holdT - 2 * realDt);
    }
    // what is heard: the locked signal, else the one at the crosshair, else the strongest within the middle lobe; a people's recording
    // plays through the static
    int src = r.locked >= 0 ? r.locked : (r.aim >= 0 ? r.aim : r.best);
    const Signal* sig = src >= 0 && (r.locked >= 0 || beamGain(r.angle[src]) > 0.25) ? &r.all[src] : nullptr;
    if (sig && (sig->kind == SIG_PEOPLE || sig->kind == SIG_ROGUE)) {   // D-05: a drifter's beacon, where it has one
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
    if (!(sig && (sig->kind == SIG_PEOPLE || sig->kind == SIG_ROGUE) && r.contentT >= 0)) { audio.radarSeed = sig ? sig->seed : 0; audio.radarVoice = -1; }   // the signal's own voice by its seed; a people's carrier alone until its programme is on (D-05: a drifter's voice without its beacon)
    radarWordStep(realDt);   // C-16: a heard word back out of the static, while nothing is on the air
    double want = 0;
    if (sig) {   // C-15: a far signal comes through less (0.3 + half its strength in the beam, 0.6 locked) than C-07's
        bool far = deepOf(*sig); double st = signalStrengthAtGain(*sig, G);
        want = clampd(beamGain(r.angle[src]) * (far ? 0.3 + 0.5 * st : 0.5 + 0.5 * st), 0, 1);
        if (r.locked >= 0) want = std::max(want, far ? 0.6 : 0.85);
        if (far) want *= 1 - 0.35 * r.sky * tG;   // C-16: toward the core a far signal is the hardest to pull out of the galaxy's noise
    }
    r.clarity += (want - r.clarity) * (1 - std::exp(-realDt * 4));
    audio.radar = 1; audio.radarSignal = r.clarity; audio.radarSet = RADIO_CAMERA;
    audio.radarKind = sig ? sig->kind : -1;
    audio.radarRogue = sig && sig->kind == SIG_ROGUE ? rogueVoiceFor(*sig) : RogueVoice();   // D-03: a drifter's voice by what it is
    audio.radarPulseHz = sig && sig->kind == SIG_PULSAR ? std::max(0.2, sig->pulseHz) : 1;
    audio.radarDrive = sig && G > 1 ? clampd((signalDrive(*sig, G) * beamGain(r.angle[src]) - 0.8) / 0.7, 0, 1) : 0;   // C-15: a near signal overdrives the receiver
    // C-16: what the gain lets in. The dial, the galaxy's noise in the beam, how worn the signal heard is, the age on the air (the
    // slot's people's once its programme is on, else the first people's), a false rise in the beam
    audio.radarGainT = tG; audio.radarSky = r.sky;
    audio.radarWear = sig ? wearOf(*sig) : 0;
    audio.radarAge = sig && sig->kind == SIG_PEOPLE ? (r.contentT >= 0 && r.progAge >= 0 ? r.progAge : sig->age) : -1;
    audio.radarGhost = r.rise.on ? clampd(r.riseGain / 0.25, 0, 1) : 0;
    // the loudest other signal in the wide lobe bleeds in under the one heard, more with the gain (none at x1: C-07's receiver); two
    // peoples' carriers beat into a whistle that slides as the beam moves; at the top steps the two ring against each other
    r.under = -1; double ug = 0;
    if (G > 1) for (int i = 0; i < n; i++) if (i != src && r.angle[i] < 40 * DEG && r.gain[i] > ug) { ug = r.gain[i]; r.under = i; }
    if (r.under >= 0) {
        const Signal& u = r.all[r.under];
        double lvl = clampd(ug * (0.3 + 1.2 * tG), 0, 0.6) * (deepOf(u) ? 1 - 0.35 * r.sky * tG : 1.0);
        audio.radarKind2 = u.kind; audio.radarSeed2 = u.seed; audio.radarAge2 = u.kind == SIG_PEOPLE ? u.age : -1; audio.radarSignal2 = lvl;
        audio.radarRogue2 = u.kind == SIG_ROGUE ? rogueVoiceFor(u) : RogueVoice();
        audio.radarPulseHz2 = u.kind == SIG_PULSAR ? std::max(0.2, u.pulseHz) : 1;
        audio.radarVoice2 = u.kind == SIG_PEOPLE ? underVoiceOf(u) : -1;
        bool carriers = sig && sig->kind == SIG_PEOPLE && u.kind == SIG_PEOPLE;
        double phase = (mix64((sig ? sig->seed : 0) ^ u.seed) >> 11) * (TAU / 9007199254740992.0);
        audio.radarHetHz = 350 + 1700 * (0.5 + 0.5 * std::sin(phase + 6 * (r.angle[r.under] - (src >= 0 ? r.angle[src] : 0))));
        audio.radarHet = carriers ? std::sqrt(clampd(r.clarity, 0, 1) * lvl) * (0.3 + 0.7 * tG) : 0;
        audio.radarIntermod = sig ? clampd((tG - 0.55) / 0.45, 0, 1) * std::min(1.0, 2 * lvl) : 0;
    } else { audio.radarKind2 = -1; audio.radarSignal2 = 0; audio.radarHet = 0; audio.radarIntermod = 0; }
    r.rollPh += realDt * 0.17 * (1 + 1.5 * tG);   // the picture's rolling band runs faster with the gain
    if (r.locked >= 0 && enterKey(in) && !ship.targeting) { radarAccept(); return true; }   // while aiming at a star, Enter is the aim's
    return false;
}
