// The drone bay (game/drones.cpp, game/drone_draw.cpp): a drone's model, the bay's clock and charge, each drone's trip
// and finds, the world's items, the screen's map
#pragma once
#include "core/types.h"
#include "peoples/drones.h"
#include <cstdint>
#include <string>
#include <vector>

// C-18: a drone of the bay as a model in its own metres (x its right, y up, z its nose; its feet at y 0), drawn filled on its rack in the
// cabin and as lines on the bay's screen (game/drone_draw.cpp): a short hull with the dark band of its eye, four ducted fans on arms, two
// arms under its nose with three-fingered hands, a hold, its number in lamps on its back and a beacon
struct DronePose {
    double arms = 0;     // 0 folded under the belly .. 1 reaching down and forward
    double grip = 0;     // 0 the fingers open .. 1 closed on a find
    double spin = 0;     // the fans' angle (radians)
    double blur = 0;     // 0 still blades .. 1 a disc
    int number = 1;      // its lamps (one, two, three)
    bool lit = true;     // its eye and its beacon lit
    double blink = 0;    // the beacon's clock (seconds)
};
enum DroneMat { DM_PAINT = 0, DM_DARK, DM_METAL, DM_ACCENT, DM_EYE, DM_BEACON, DM_NUMBER };
struct DronePrim {
    int kind = 0;        // 0 a face (`n` points), 1 a tube from p[0] to p[1] of radius `r`, 2 a lamp at p[0] (`r` its size in pixels at a metre)
    int n = 0; Vec3 p[6];
    Vec3 nrm;            // a face's outward normal
    double tint = 1;     // a face's paint
    int mat = DM_PAINT;
    double r = 0.01;
};
void droneModel(const DronePose& pose, std::vector<DronePrim>& out);

constexpr double DRONE_SEARCH_S = 120;     // C-18 (the user's, 2026-10-11): two minutes down there, from the first place a drone searches to its last
constexpr double DRONE_COOLDOWN_S = 300;   // C-18: five minutes charging from the drones' return to the next launch (real time; the user's, 2026-10-11:
                                           // "2 minutes + 5 minutes recharge", at first five minutes from launch to launch)

struct DroneFindRec { int kind = 0, index = 0, people = 0, medium = -1, place = -1; double t = 0; std::string name; bool aboard = false; };
struct DroneStopRec { int place = -1, sclass = -1; double lat = 0, lon = 0; double arrive = 0, leave = 0; };
struct DroneRun {
    double out = 0;                                // the clock it leaves the rack at
    double down = 0, up = 0, dock = 0;             // it reaches its first stop, leaves its last, is back on its rack
    bool docked = true;
    int wanted = 0;                                // what it set out to bring (2 to 8)
    std::vector<DroneStopRec> stops;
    std::vector<DroneFindRec> finds;               // in the order found
};
struct DroneBay {
    double clock = -1;                             // real seconds since the last launch (-1: none since the bay was new)
    bool out = false;                              // a trip on: from the launch until the last drone docks
    bool planned = false;                          // its haul drawn (the world's items read)
    bool recalled = false;                         // the last trip was called back
    uint64_t starSeed = 0; int body = -1; std::string worldKey, worldName;
    uint64_t seed = 0;
    double back = 0;                               // the clock when the last trip ended (its last dock, or the recall): the charge runs from it
    double subLat = 0, subLon = 0;                 // the ground under the ship at the launch (radians)
    DroneRun run[DRONE_COUNT];
    // the parked world's items (`droneWorldOf`, read on the thread below)
    DroneWorld world; bool worldValid = false; uint64_t worldStar = 0; int worldBody = -1;
    // the screen: the world's map (its key), the finds list's scroll
    std::vector<uint32_t> map; uint64_t mapSeed = 0; int mapW = 0, mapH = 0;
    int listTop = 0;
};

// a find's guide key ("<body>/S<index>", "<body>/I<id>")
inline std::string findKeyOf(const std::string& world, int kind, int index) { return world + (kind == DF_INSTRUMENT ? "/I" : "/S") + std::to_string(index); }
// the rack's shelves, drone 1 on the top one (the cabin's metres: game/drone_draw.cpp's rack)
const double SHELF_Y[DRONE_COUNT] = {1.00, 0.52, 0.04};
