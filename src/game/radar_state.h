// The signal radar's state (game/radar.cpp, radar_camera.cpp): the scan, the beam, the lock, the programme on the air, the gain's far scan,
// the false rises and the heard words
#pragma once
#include "shard_world.h"
#include "core/rng.h"
#include "core/types.h"
#include "peoples/signals.h"
#include "peoples/voice.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct RadarState {
    bool on = false;
    std::vector<Signal> far, local, all;    // the transmitters and loud pulsars within reach of the scan's sector; the system's own; both with this frame's directions
    std::vector<double> gain, angle;        // per entry of `all`: the beam's gain times the strength, and the angle off the beam
    int64_t scanSx = 1 << 30, scanSy = 0, scanSz = 0; int scanCells = 0; double scanMs = 0;
    double reading = 0, noiseWalk = 0, traceAccum = 0;
    std::vector<float> trace;               // the meter's last four seconds, for the scope
    int best = -1; double bestGain = 0, bestAngle = 0;
    double holdT = 0, lockSecs = 3;         // the hold within the lock's cone, and what it needs
    int locked = -1; uint64_t lockedSeed = 0; int lockedKind = -1; double lostT = 0;
    double clarity = 0;                     // how much of the signal comes through the static (the synth's radarSignal)
    ShardWorld world; uint64_t worldSeed = 0; int worldWhich = -1;   // the people's world in the beam, built once (C-13: once per people on the air)
    uint64_t pendingSeed = 0; double pendingAt = -10;               // H-04: a world whose records are being written (the carrier alone until they are)
    int shard = -1; double contentT = -1, contentSecs = 0; bool contentMusic = false; double awayT = 0;
    Programme prog; int loopI = 0; double loopAt = 0;   // the programme on the air (peoples/signals.h), the repeat a loop or a numbers station is at and when it began
    Rng rng{0x5161};
    // C-15 the gain: the dial (SIGNAL_GAINS, x1 to x32; kept while the radar is off, not saved), what it hears beyond 200 ly from the
    // scan's sector (the whole dial's at once, each by its `needGain`), found a slice a frame by `deep` (LISTENING DEEPER meanwhile)
    int gainStep = 0;
    std::vector<Signal> deep; SignalScan deepScan; bool deepRunning = false, deepReady = false; double deepMs = 0; int deepFrames = 0;
    std::vector<double> level;              // per entry of `all`: the level at the beam's centre (C-07's strength at x1; 1 and over overloads)
    int aim = -1; double aimAngle = 0;      // the loudest within the lock cone: the one the hold is on
    bool overload = false;                  // the aim overloads the receiver: the meter pinned, the hold refused
    double sum = 0, floorNow = 0;           // the beam's sum over the signals and the static's floor this frame (the harness)
    // C-16 what the gain lets in: the age of the slot's people on the air, the band's brightness in the beam (the galaxy's own
    // noise), the signal under the one heard (an index into `all`: the loudest other in the wide lobe), the rolling band's phase
    int progAge = -1; double sky = 0; int under = -1; double rollPh = 0;
    // a false rise: the noise's ghost in a direction near the beam at the top steps, living some seconds; held, the hold fills
    // part way (`holdMax` of it) and empties (NOTHING THERE for `nothingT`); never a lock
    struct FalseRise { bool on = false, spent = false, held = false; Vec3 dir; double t = 0, life = 8, peak = 0.2, holdMax = 0.6; };
    FalseRise rise; double riseNext = 20, riseGain = 0, riseAngle = PI, nothingT = 0; int rises = 0, riseHolds = 0, nothings = 0;
    // a word of a recording heard before (`Guide::heard`) coming back out of the static at the top steps while nothing is on the
    // air: its world (when it is not the radar's), the word spoken in the world's voice, when the next may come
    ShardWorld wordWorld; Speech wordSpeech; Voice wordVoice; double wordNext = 30, wordT = -1; int words = 0; std::string wordKey, wordSaid;
    std::map<uint64_t, DrifterBeacon> beacons;   // D-05: a drifter's beacon by its signal's seed, decided once (`drifterBeaconIfReady`)
};
