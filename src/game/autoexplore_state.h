// Auto-explore (game/autoexplore.cpp, autoexplore_space.cpp, autoexplore_ground.cpp, autoexplore_hands.cpp): the profiles,
// a step of the plan, the mind and the hands' state
#pragma once
#include "core/input.h"
#include "core/rng.h"
#include "core/types.h"
#include <cstdint>
#include <deque>
#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

// R-419: auto-explore's profiles (game/autoexplore.cpp): the wanderer takes another's taste at each star
enum AutoProfile { AP_WANDERER = 0, AP_STARGAZER, AP_PLANETFALL, AP_SURVEYOR, AP_ARCHAEOLOGIST, AP_GIANTS, AP_COUNT };
extern const char* const AUTO_PROFILE_NAMES[AP_COUNT];   // "WANDERER", "STARGAZER", ...
extern const char* const AUTO_PROFILE_LINES[AP_COUNT];   // what each looks for, a line of the chooser

struct AutoStep { const char* what = ""; double maxT = 30; std::function<bool(double)> run; bool timed = false; };   // `run` each frame (dt): true when done; given up after
                                                                                                                  // `maxT` (`timed`: a look that lasts that long, not given up)
struct AutoState {
    bool on = false;
    int profile = AP_WANDERER, focus = AP_WANDERER;   // the chosen profile; the taste it plays with at this star (the wanderer's whim)
    Rng rng{1};
    double clock = 0, stepT = 0, sceneT = 0, captionT = 0, mouseAcc = 0;
    int scene = -1; bool stepFirst = true, abort = false;
    std::deque<AutoStep> plan;                     // the steps ahead, the first running
    std::string doing; int steps = 0, timeouts = 0;
    int stars = 0, bodies = 0, landings = 0, probes = 0, records = 0, outings = 0, unsticks = 0;
    // the system: its star, the bodies visited, how many it means to visit
    uint64_t sysSeed = 0; std::set<int> bodyDone, bodyTried; int visits = 0, quota = 0; bool radarTried = false;
    // the landing: the site chosen, the ground's time and outings, the places and records already sought
    bool siteSet = false; double siteLat = 0, siteLon = 0;
    bool groundBegun = false; double groundUntil = 0; int outingsLeft = 0, lastOuting = -1;
    std::set<uint64_t> placesSeen; std::set<int> recordsTried;
    int recIdx = -1, recPhase = 0, recTaken = 0; double recT = 0;
    int towns = 0; std::string townLine;          // the settlements toured, the last one's line (the harness)
    double probeEndT = -1, chuteAt = 20;
    // the hands: the look's frame and target, the gaze's angles and speeds in it, what the hands set last frame (a change of the game's
    // own is taken up), its pace and its sway at rest; the wandering of the eyes; the stick; the keys
    int lookFrame = -1; bool lookSet = false;
    double wantYaw = 0, wantPitch = 0, gYaw = 0, gPitch = 0, vYaw = 0, vPitch = 0, setYaw = 1e9, setPitch = 1e9, pace = 1, sway = 1, swayPh[6] = {};
    double driftNext = 0, driftYaw = 0, driftPitch = 0;
    double fwd = 0, side = 0, wantFwd = 0, wantSide = 0;
    bool hold[KEY_MAX] = {}; std::vector<int> taps; double sprintTapT = 0, everyT = 0;
    bool shipAim = false; Vec3 shipDir; double sYaw = 1e9, sPitch = 0, svYaw = 0, svPitch = 0;   // the ship's nose eased onto an aim
    double orbitE = 0, orbitN = 0, wantOrbitE = 0, wantOrbitN = 0;                               // a lap round the parked world, eased
    double warpWant = 1;                                                                         // the clock's pace, eased in decades
    // the walk's way (its legs round what stands in it), the drive's and the flight's state, the unsticking
    std::vector<std::pair<double, double>> way; size_t wayI = 0; double goX = 1e18, goZ = 1e18, bestD = 1e18, stillT = 0, jetT = 0; int unstick = 0;
    double driveOff = 0, driveEvalT = 0, reverseT = 0; int circleDir = 1; bool chase = false;
};
