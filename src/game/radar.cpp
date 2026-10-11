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
// C-16 (2026-10-09): what the gain lets in (R-410: "radio noise, creepy mixed sounds when you are trying to get really far signal").
// The receiver is C-07's; the gain is how much of the void comes through it: the floor rises (`AudioState::radarGainT`), the
// galaxy's own noise is in the beam (the band's brightness round it, `radarSkyAt`: loud toward the core, a far signal there the
// hardest to pull out), the loudest other signal in the wide lobe bleeds in under the one heard (`RadarState::under`: two at once,
// a heterodyne between two carriers that slides as the beam moves, at the top steps an intermodulation's ring), a far signal comes
// worn (`radarWear`), the static's events come oftener and stranger, a word heard before comes back (`radarWordStep`), the noise
// makes false rises that a hold empties (`radarRiseStep`, NOTHING THERE), and the picture's static, the radio sky's glow, a ghost
// of the star field and the tears grow with the gain. The sound tells; the readout never does.
#include "game.h"
#include "ui.h"
#include "galaxy/planetmap.h"
#include "galaxy/ruins.h"
#include "core/rng.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>

namespace {
const double LOCK_CONE = 3 * DEG, KEEP_CONE = 8 * DEG;
// C-15: the gain narrows the cones (3 and 8 degrees at x1, 2 and 4 at x32) and lengthens the hold (x2.25 at x32: a faint far
// signal at the top step takes eleven seconds); the far scan reads 40,000 cells a frame (about five frames from home at x32)
const int DEEP_SLICE = 40000;
inline double octaves(double g) { return std::log2(std::max(1.0, g)); }
inline double lockCone(double g) { return LOCK_CONE / (1 + 0.1 * octaves(g)); }
inline double keepCone(double g) { return KEEP_CONE / (1 + 0.2 * octaves(g)); }
inline double holdFactor(double g) { return 1 + 0.25 * octaves(g); }
inline bool deepOf(const Signal& s) { return !s.local && s.needGain > 1; }   // heard only through the gain
// C-16: the dial as 0 (x1) .. 1 (x32); how worn a signal comes (none within 200 ly, half at about 1,000, all at 6,000)
inline double gainShare(double g) { return clampd((g - 1) / (SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1] - 1), 0, 1); }
inline double wearOf(const Signal& s) { return s.local || s.distLy <= SIGNAL_REACH_LY ? 0 : clampd(std::log(s.distLy / SIGNAL_REACH_LY) / std::log(30.0), 0, 1); }
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
    radar.all.clear(); radar.gain.clear(); radar.angle.clear(); radar.level.clear(); radar.aim = -1; radar.overload = false;   // C-15: the dial stays where it was
    radar.under = -1; radar.rise.on = false; radar.nothingT = 0;   // C-16
    radarWordStop();
    audio.radar = 0; audio.radarSignal = 0; audio.radarKind = -1; audio.radarDrive = 0;
    audio.radarGainT = 0; audio.radarSky = 0; audio.radarWear = 0; audio.radarGhost = 0; audio.radarAge = -1; audio.radarKind2 = -1; audio.radarSignal2 = 0; audio.radarHet = 0; audio.radarIntermod = 0;
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

// C-15: the dial, x1 to x32 in six steps (SIGNAL_GAINS); the reach is the transmitter's, not the radar's, so it is not printed
void Game::radarGainStep(int dir) {
    int s = clampi(radar.gainStep + dir, 0, SIGNAL_GAIN_STEPS - 1);
    if (s == radar.gainStep) { audio.beep = 3; return; }
    radar.gainStep = s;
    status(fmt("RECEIVER GAIN X%d", SIGNAL_GAINS[s]), 2);
    audio.beep = 4;
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
    if (s.kind != SIG_PEOPLE || s.body < 0) return;
    auto build = [&](int which) {   // built once per world and people; a world that fails to build is not tried again every frame
        r.world.key = Guide::bodyKey(s.star.sx, s.star.sy, s.star.sz, s.body);
        Star full = s.star; if (full.name.empty()) starInSector(s.star.sx, s.star.sy, s.star.sz, full, true);
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
    double s = 0.5 * sampleGalaxyBand(band, BAND_MAP_W, BAND_MAP_H, f), k = std::tan(10 * DEG);
    for (int i = 0; i < 6; i++) { double ang = i * TAU / 6; s += 0.5 / 6 * sampleGalaxyBand(band, BAND_MAP_W, BAND_MAP_H, normalize(f + (a * std::cos(ang) + b * std::sin(ang)) * k)); }
    return clampd(s, 0, 1);
}

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
        if (!Guide::parseStarKey(bodyKey, sx, sy, sz) || !starInSector(sx, sy, sz, st, true)) return;
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
                r.locked = -1; r.holdT = 0; r.lostT = 0; status(sank ? "LOCK LOST - TOO FAINT AT THIS GAIN" : "LOCK LOST - THE BEAM LEFT THE SIGNAL", 3); audio.beep = 3;
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
    radarWordStep(realDt);   // C-16: a heard word back out of the static, while nothing is on the air
    double want = 0;
    if (sig) {   // C-15: a far signal comes through less (0.3 + half its strength in the beam, 0.6 locked) than C-07's
        bool far = deepOf(*sig); double st = signalStrengthAtGain(*sig, G);
        want = clampd(beamGain(r.angle[src]) * (far ? 0.3 + 0.5 * st : 0.5 + 0.5 * st), 0, 1);
        if (r.locked >= 0) want = std::max(want, far ? 0.6 : 0.85);
        if (far) want *= 1 - 0.35 * r.sky * tG;   // C-16: toward the core a far signal is the hardest to pull out of the galaxy's noise
    }
    r.clarity += (want - r.clarity) * (1 - std::exp(-realDt * 4));
    audio.radar = 1; audio.radarSignal = r.clarity;
    audio.radarKind = sig ? sig->kind : -1;
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

// the picture as the radar camera gives it: a cold phosphor (the palette toward blue-green, lifted off black), scanlines, a band
// rolling down, a vignette, and static by the clarity (noise on every pixel, snow in flecks) that thins as a signal comes through.
// C-16: with the gain the static and the snow thicken, the radio sky the receiver hears glows through the phosphor (the band's
// brightness, on a coarse grid of directions), the star field ghosts a faint double, the rolling band runs faster and the picture
// tears now and then (a band of rows thrown sideways for an eighth of a second)
void Game::radarCameraFeed(Framebuffer& fb) {
    const double gT = gainShare(radarGain());
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
    // the radio sky: the band's brightness at every eighth pixel of the screen's grid, in intensity units (two shades at x1 where the
    // band is brightest, twelve at x32)
    const int cell = 8 * S, GW = FBW / cell + 2, GH = FBH / cell + 2;
    static std::vector<float> glow, rowGlow, colT; static std::vector<int> colI;
    if ((int)colI.size() != FBW || (int)colT.size() != FBW) { colI.resize(FBW); colT.resize(FBW); }
    for (int x = 0; x < FBW; x++) { double fx = (double)x / cell; colI[x] = std::min(GW - 2, (int)fx); colT[x] = (float)(fx - colI[x]); }   // the grid's column and the share across it, per pixel
    const std::vector<float>& band = spaceR.parkedBand();
    const double glowAmp = band.empty() ? 0 : INTEN_PER_SHADE * (2 + 10 * gT);
    if (glowAmp > 0) {
        glow.assign((size_t)GW * GH, 0.f); rowGlow.assign((size_t)GW, 0.f);
        Mat3 camT = viewBasis().transposed(); double invf = 1.0 / spaceR.proj.f;
        for (int j = 0; j < GH; j++) for (int i = 0; i < GW; i++) {
            Vec3 d = normalize(Vec3((i * cell - spaceR.proj.cx) * invf, -(j * cell - spaceR.proj.cy) * invf, 1.0));
            glow[(size_t)j * GW + i] = (float)(glowAmp * sampleGalaxyBand(band, BAND_MAP_W, BAND_MAP_H, camT * d));
        }
    }
    // the ghost double and the tears read the picture as it was
    static std::vector<Pix> orig;
    const int dblX = 3 * S, dblY = S; const double dblK = 0.45 * gT;
    int tearY0 = -1, tearY1 = -1, tearShift = 0;
    if (gT > 0) {
        uint64_t h = mix64((uint64_t)(realTime * 8) ^ 0x7EA2ULL);
        if ((h >> 11) * (1.0 / 9007199254740992.0) < 0.004 + 0.035 * gT) {
            tearY0 = (int)((h >> 20) % (uint64_t)FBH); tearY1 = tearY0 + (6 + (int)((h >> 40) % 30)) * S;
            tearShift = (4 + (int)((h >> 50) % 16)) * S * ((h >> 63) ? 1 : -1);
        }
    }
    if (dblK > 0 || tearY0 >= 0) orig = fb.idx;
    int bandY = (int)(std::fmod(radar.rollPh, 1.0) * (FBH + 40 * S)) - 20 * S;   // rolls down the picture every 6 s at x1 (C-16: 2.4 s at x32)
    uint32_t rs = (uint32_t)(realTime * 60) * 2654435761u + 0x51A7u;
    int noiseAmp = (int)(40 + 150 * statik + 50 * gT), snowOdds = (int)(2 + 40 * statik + 25 * gT);   // intensity units (32 a shade); flecks per 4096 pixels
    for (int y = 0; y < FBH; y++) {
        double line = ((y / S) & 1) ? 0.8 : 1.0;
        double bnd = std::abs(y - bandY) < 10 * S ? 1.08 : 1.0;
        Pix* row = &fb.idx[(size_t)y * FBW];
        const bool tear = y >= tearY0 && y < tearY1;
        if (glowAmp > 0) {
            double fy = (double)y / cell; int j0 = std::min(GH - 2, (int)fy); double ty = fy - j0;
            for (int i = 0; i < GW; i++) rowGlow[i] = (float)(glow[(size_t)j0 * GW + i] * (1 - ty) + glow[(size_t)(j0 + 1) * GW + i] * ty);
        }
        for (int x = 0; x < FBW; x++) {
            rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5;
            int xs = tear ? std::max(0, std::min(FBW - 1, x - tearShift)) : x;
            Pix p = tear ? orig[(size_t)y * FBW + xs] : row[x];
            double base = intenOf(p);
            if (dblK > 0 && y >= dblY && xs >= dblX) base = std::max(base, dblK * intenOf(orig[(size_t)(y - dblY) * FBW + (xs - dblX)]));
            if (glowAmp > 0) { int i0 = colI[x]; float tx = colT[x]; base += rowGlow[i0] + (rowGlow[i0 + 1] - rowGlow[i0]) * tx; }
            double vig = 1 - 0.5 * std::max(0.0, (double)(vx[x] + vy[y]) - 0.4);
            int inten = (int)(base * line * bnd * vig) + (int)(rs % (2 * noiseAmp + 1)) - noiseAmp;
            if ((int)((rs >> 8) & 4095) < snowOdds) inten = std::max(inten, 32 * (28 + (int)((rs >> 20) % 24)));
            row[x] = pixI(bankOf(p), inten);
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
    const double G = radarGain();
    {   // crosshair and the lock cone: dim while sweeping, cyan with a white arc filling while the hold lasts, amber once locked
        // (C-15: the cone narrows with the gain)
        drawLineRGB(canvas, cx - 10, cy, cx - 4, cy, fd); drawLineRGB(canvas, cx + 4, cy, cx + 10, cy, fd);
        drawLineRGB(canvas, cx, cy - 8, cx, cy - 3, fd); drawLineRGB(canvas, cx, cy + 3, cx, cy + 8, fd);
        double rad = pxPerRad * std::tan(lockCone(G));
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
        if (mh > 0) fillRectRGB(canvas, bx1 + 5, by1 - 1 - mh, bx1 + 11, by1 - 1, r.overload ? HUD_RED : (r.locked >= 0 ? HUD_AMBER : HUD_GREEN));   // C-15: pinned red by an overload
        std::string right = r.locked >= 0 ? "LOCKED" : (r.overload ? "OVERLOAD" : (r.holdT > 0 && r.nothingT <= 0 ? "LOCKING" : "SWEEP"));   // C-16: a false rise's hold empties under SWEEP
        drawText(canvas, bx1 + 12 - textWidth(right.c_str()), by1 + 4, right.c_str(), r.locked >= 0 ? HUD_AMBER : (r.overload ? HUD_RED : fd));
        drawText(canvas, bx0, by1 + 4, "RECEIVER", fd);
        std::string gain = fmt("GAIN X%.0f", G);   // C-15: the dial, by the scope (the reach is the transmitter's: not printed)
        drawText(canvas, bx0 + textWidth("RECEIVER") + 8, by1 + 4, gain.c_str(), G > 1 ? HUD_CYAN : fd);
    }
    // the source: a smudge of static where the beam's middle lobe has it (found, not handed: nothing shows outside ten degrees),
    // the lock's diamond once locked; an edge mark when it is behind
    int src = r.locked >= 0 ? r.locked : (r.aim >= 0 ? r.aim : r.best);
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
                    double dens = (g - 0.35) / 0.65 * std::min(1.0, src < (int)r.level.size() ? r.level[src] : s.strength);   // C-15: by its level at this gain
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
    // C-16: a false rise smudges the static as a source would where nothing outshines it within the middle lobe: the instrument
    // cannot tell, the hold can
    if (r.rise.on && r.locked < 0 && r.riseAngle < 10 * DEG && (src < 0 || r.riseGain > r.gain[src])) {
        Mat3 cam = viewBasis();
        Vec3 v = cam * r.rise.dir;
        double g = beamGain(r.riseAngle);
        if (v.z > 0 && g > 0.35) {
            int sx = (int)(UW / 2 + spaceR.proj.f * v.x / v.z / FB_SCALE), sy = (int)(UH / 2 - spaceR.proj.f * v.y / v.z / FB_SCALE);
            double dens = (g - 0.35) / 0.65 * std::min(1.0, r.rise.peak * 3) * (r.rise.spent ? 0.4 : 1.0);
            uint32_t rs = (uint32_t)(realTime * 30) * 2654435761u + 0x5EEDu;
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
    // the readout: the system only
    std::string l1, l2, l3; uint32_t c1 = fd;
    if (r.locked >= 0 && r.locked < (int)r.all.size()) {
        const Signal& s = r.all[r.locked];
        c1 = HUD_AMBER;
        if (s.local) {
            l1 = "A SIGNAL FROM THIS SYSTEM";
            l2 = fmt("%s", trunc(upper(bodyNameOf(s.body)), 30).c_str());
            l3 = "ENTER APPROACHES IT";
        } else {   // C-15: thousands of light years at the top steps (the line within its 52 characters: the name gives way first)
            Star full = s.star; if (full.name.empty()) starInSector(s.star.sx, s.star.sy, s.star.sz, full, true);
            std::string ly = thousands(s.distLy), tail = fmt(" (%s)  %s LY%s", STAR_CLASSES[full.cls].code, ly.c_str(), guide.visited.count(starKeyOf(full)) ? "  VISITED" : "");   // R-407: a loop back to a system seen says so
            l1 = "A SIGNAL FROM " + trunc(upper(starNameOf(full)), (size_t)clampi(52 - 14 - (int)tail.size(), 6, 16)) + tail;
            std::string sector = fmt("SECTOR %lld %lld %lld", (long long)s.star.sx, (long long)s.star.sy, (long long)s.star.sz);
            l2 = sector + " - THE SIGNAL IS " + ly + " YEARS OLD";
            if (l2.size() > 52) l2 = sector + " - " + ly + " YEARS OLD";
            l3 = "ENTER LOCKS THE STAR AND FLIES THERE";
        }
    } else if (r.overload) { l1 = "OVERLOADED - TURN THE GAIN DOWN"; c1 = HUD_RED; }   // C-15: backing the gain off is part of the play
    else if (r.nothingT > 0) { l1 = "NOTHING THERE"; c1 = fc; }   // C-16: a false rise held empties
    else if (r.holdT > 0) { l1 = fmt("LOCKING %.0f%% - HOLD STEADY", 100 * clampd(r.holdT / std::max(r.lockSecs, 0.1), 0, 1)); c1 = HUD_CYAN; }
    else if (r.gainStep > 0 && !r.deepReady) { l1 = "LISTENING DEEPER..."; c1 = HUD_CYAN; }   // C-15: the far scan, a slice a frame
    else if ((r.best >= 0 && r.bestGain > 0.06 && (G <= 1 || r.bestAngle < 10 * DEG)) || (r.rise.on && !r.rise.spent && r.riseGain > 0.06 && r.riseAngle < 10 * DEG)) { l1 = "A RISE - CENTRE IT AND HOLD TO LOCK"; c1 = HUD_GREEN; }   // C-15: over x1 the side lobes rise everywhere: the middle lobe's (C-16: a false rise's too)
    else l1 = "SWEEP THE SKY - A RISE ON THE METER IS A SIGNAL";
    drawTextCentered(canvas, cx, UH - 54, l1.c_str(), c1);
    if (!l2.empty()) drawTextCentered(canvas, cx, UH - 46, l2.c_str(), fc);
    if (!l3.empty()) drawTextCentered(canvas, cx, UH - 38, l3.c_str(), HUD_GREEN);
    drawTextShadow(canvas, 18, UH - 26, "+/- GAIN  B / ESC LEAVES THE CAMERA", fd, HUD_SHADOW);   // C-15: the telescope's place, clear of the bracket's upright
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
    std::string s = fmt("radar: %s, gain x%.0f, %zu far (%d cells, %.1f ms) + %zu local", r.on ? "on" : "off", radarGain(), r.far.size(), r.scanCells, r.scanMs, r.local.size());
    if (r.gainStep > 0) s += r.deepReady ? fmt(" + %d deep of %zu (%d frames, %.1f ms)", (int)std::count_if(r.deep.begin(), r.deep.end(), [&](const Signal& d) { return d.needGain <= radarGain(); }), r.deep.size(), r.deepFrames, r.deepMs) : fmt(" + deep scan at %d cells", r.deepScan.cells);
    s += fmt(", reading %.2f (the beam's sum %.2f, the floor %.2f)%s", r.reading, r.sum, r.floorNow, r.overload ? " OVERLOAD" : "");
    s += fmt(", the sky %.2f", r.sky);   // C-16
    if (r.under >= 0 && r.under < (int)r.all.size()) s += fmt(", under it %s %.0f ly (%.1f deg off, %.2f through%s)", SIGNAL_KIND_NAMES[r.all[r.under].kind], r.all[r.under].distLy, r.angle[r.under] / DEG, audio.radarSignal2, audio.radarHet > 0 ? fmt(", a whistle at %.0f Hz", audio.radarHetHz).c_str() : "");
    if (r.rise.on) s += fmt(", a false rise %.1f deg off (%.2f%s)", r.riseAngle / DEG, r.riseGain, r.rise.spent ? ", spent" : "");
    if (r.nothingT > 0) s += ", NOTHING THERE";
    if (r.wordT >= 0) s += fmt(", a word back from the static: \"%s\"", r.wordSaid.c_str());
    if (r.best >= 0 && r.best < (int)r.all.size()) s += fmt(", best %d (%s, %.1f deg off, gain %.2f, %.0f ly, strength %.2f)", r.best, SIGNAL_KIND_NAMES[r.all[r.best].kind], r.bestAngle / DEG, r.bestGain, r.all[r.best].distLy, r.all[r.best].strength);
    s += fmt(", hold %.1f of %.1f s, locked %d", r.holdT, r.lockSecs, r.locked);
    if (r.locked >= 0 && r.locked < (int)r.all.size()) s += fmt(" (%s)", SIGNAL_KIND_NAMES[r.all[r.locked].kind]);
    s += fmt(", clarity %.2f, content %s", r.clarity, r.contentT < 0 ? "none" : fmt("S%d %s %.1f of %.0f s%s", r.shard, r.prog.kind >= 0 && r.prog.kind < RP_COUNT ? RADIO_PROGRAMME_NAMES[r.prog.kind] : "?", r.contentT, r.contentSecs, audio.radio ? (audio.piece ? " (a piece set)" : (audio.speech ? (r.prog.repeats > 1 ? fmt(" (a speech set, repeat %d of %d)", r.loopI + 1, r.prog.repeats).c_str() : " (a speech set)") : (r.prog.machine ? " (a machine)" : " (NO pointer)"))) : " (not radio)").c_str());
    return s;
}
