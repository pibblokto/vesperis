// The landing map's peoples layer (game/landing_map.cpp): the places the zoom shows, what the records held tell
#pragma once
#include "core/types.h"
#include <set>
#include <string>

struct ZoomPlace { int people = 0, rank = 0, sclass = 0; Vec3 unit; double radiusT = 0; };

struct LandKnown {
    bool ready = false;                    // the world's items read (`droneWorldAsk`: the drone bay's reading of a world's records and their homes)
    int peoples = 0;
    std::string name[2];                   // "THE JENGNE" once a record of theirs is held, else ""
    int held[2] = {0, 0}, total[2] = {0, 0};
    std::set<int> ranks[2];                // the plan's places the held records were kept at
    bool all(int k) const { return k >= 0 && k < peoples && total[k] > 0 && held[k] >= total[k]; }
};
