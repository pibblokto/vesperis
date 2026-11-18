// Z-03: the search's state (game/search.cpp): the kit's vials on the ground and the lab's cold store aboard (R-425; docs/ideas/
// ITERATION-1.5.0.md, Z-03). A vial keeps its place and its microhabitat (what it holds is `sampleOf` of them and its world's biota: a pure
// function, made again when it is looked at), the name and the temperature the HUD gave, the activity read; a save line each (`vial`)
#pragma once
#include "life/samples.h"
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

struct Vial {
    int64_t sx = 0, sy = 0, sz = 0;   // its star's sector
    int body = -1;                    // its world in that system
    Vec3 unit;                        // where on it (body-fixed)
    Microhabitat hab;
    std::string name;                 // as the HUD gave it ("WET SILT AT A LAKE'S EDGE")
    double readingC = 0;              // the temperature read at the take
    int activity = VA_NONE;
    double t = 0;                     // the game's time at the take
};
constexpr int KIT_VIALS = 8;      // a descent's vials, refilled aboard
constexpr int COLD_STORE = 64;    // the cold store's places
