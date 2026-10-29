// What the drone bay's three drones bring back from a people's world (C-18; docs/reference/civilisations/records-and-grammar.md): its
// records and the instruments beside a third of its music records, each at its home (`recordHomesOf`), copies elsewhere. A trip sends each
// drone down for two to eight of what the explorer does not hold yet: to a place by how much of its own it holds, three to five there, on
// to a second or a third place for the rest, the rarer kinds leaned toward (a star chart 2.2 times a text, an instrument four times), an
// instrument looked for first at one stop in three. The game owns the trip's clock and the guide (game/drones.cpp).
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
