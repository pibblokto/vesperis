// C-18 (2026-10-11): what the drone bay's three drones bring back from a people's world (the user's request: "three drones with robotic
// hands ... that you can send from stardrifter to felisian/desert planets with civilizations. When you send them each can bring 2-8 items
// ... cooldown is 5 minutes. They can bring both shards (all three kinds) and instruments").
//
// A world holds its peoples' records (T-04: 80 to 200 each, texts, music and star charts by H-05's shares) and the instruments lying beside a
// third of their music records (C-11), each record at its home: the place of its people's plan that always holds it (`recordHomesOf`), an
// instrument at the home of the first music record it lies beside; copies of a record lie in other places too (T-04), the instrument beside
// each. A trip sends each drone down for two to eight of what the explorer does not hold yet: to a place by how much of its own it holds (one
// another drone searches the least likely), three to five there (the place's own first, then the copies it keeps of its people's other
// records), on to a second or a third place for the rest; each find drawn with the rarer kinds leaned toward (a star chart 2.2 times a text,
// an instrument four times), and at one stop in three an instrument of the place's people looked for first (a world holds a few). The game owns the trip's clock and the guide (game/drones.cpp).
#pragma once
#include "galaxy/system.h"
#include <cstdint>
#include <string>
#include <vector>

enum DroneFindKind { DF_TEXT = 0, DF_MUSIC, DF_CHART, DF_INSTRUMENT, DF_COUNT };
extern const char* const DRONE_FIND_NAMES[DF_COUNT];   // "text", "music", "star chart", "instrument" (the harness)

constexpr int DRONE_COUNT = 3, DRONE_MIN_FINDS = 2, DRONE_MAX_FINDS = 8, DRONE_MAX_STOPS = 3;

// a place of a people's plan that is the home of something the drones can bring
struct DronePlace {
    int people = 0, rank = -1;   // the plan's people and its rank (0 the capital)
    int sclass = 0;              // its class (SC_CAPITAL, SC_CITY, SC_TOWN, ...)
    double lat = 0, lon = 0;     // radians, the body's frame
};
struct DroneItem {
    int kind = DF_TEXT;
    int index = 0;               // a record's world index (`shardWorldIndex`), an instrument's id (`Instrument::id`)
    int people = 0;
    int medium = -1;             // a record's medium (RecordMedium); -1 an instrument
    int age = 0;                 // its year's age (a record's; an instrument's the age it was played in)
    int place = -1;              // its home in `DroneWorld::places`, -1 none (the plan or the history could not be read)
    std::string name;            // an instrument's ("A SHELL PIPE"); empty for a record (its medium names it)
};
struct DroneWorld {
    uint64_t seed = 0;           // the world's generator's seed
    int peoples = 0;             // 0: no people lived there
    std::vector<DronePlace> places;
    std::vector<DroneItem> items;
    std::vector<std::string> peopleNames;   // "THE GREIP" by people
    bool homes = false;          // the records' homes were read (else every item is at no place)
    double buildMs = 0;
};
// every record and instrument of the world (a body of `sys`), each at its home; blocks on the world's plan and histories (the game calls it
// on a thread of its own)
void droneWorldOf(const StarSystem& sys, int body, DroneWorld& out);

// a trip's haul: each drone's stops in order, each with the items (indices into `DroneWorld::items`) it brings from there
struct DroneLeg { int place = -1; std::vector<int> items; };
struct DroneHaul {
    std::vector<DroneLeg> legs[DRONE_COUNT];
    int wanted[DRONE_COUNT] = {};   // what each drone set out to bring (2 to 8); it brings fewer only when the world has no more
    int total = 0;
};
// the haul from the items not held (`held[i]` true: the explorer holds item i), drawn from `seed`
void droneHaulOf(const DroneWorld& W, const std::vector<bool>& held, uint64_t seed, DroneHaul& out);
