// N3 (R-202): the bestiary of a world. A deterministic function of the body seed gives each living world its land species, flyers and
// swimmers, each with a body plan, size, gait, habitat, diet, activity, temperament and a call. Sites spawn herds from it. J-06 (generation
// 18): each species from its own seed, its body as parts (animal_body.h) in its world's style, no two of a world alike; sixteen plans, several
// swimmers a world, floaters in a thick air
#pragma once
#include "galaxy/planetmap.h"
#include "animal_body.h"
#include <string>
#include <vector>

enum Temperament { TEMP_SHY = 0, TEMP_CURIOUS, TEMP_INDIFFERENT };
enum Activity { ACT_DAY = 0, ACT_NIGHT, ACT_DUSK };
enum CallKind { CALL_NONE = 0, CALL_CHIRP, CALL_LOW, CALL_HOOT, CALL_CLICK };
extern const char* const ACTIVITY_NAMES[3];
extern const char* const TEMPERAMENT_NAMES[3];

struct Species {
    std::string name;
    int plan = PLAN_QUADRUPED;
    double size = 1;                 // shoulder height, metres (a flyer's, a swimmer's and a floater's body length)
    double bodyLen = 1.6, legLen = 0.8;   // the trunk's length and the legs' reach (a herd's spacing, a stride)
    double walkSpeed = 1.0, runSpeed = 4.0;
    int gait = GAIT_WALK, runGait = GAIT_GALLOP;
    int habitat = 0;                 // preferred biome (BIO_*), 0 any
    int diet = 0;                    // 0 grass, 1 canopy, 2 water
    int activity = ACT_DAY;
    int temperament = TEMP_SHY;
    int call = CALL_CHIRP;
    double callPitch = 1;
    int herdMin = 3, herdMax = 8;
    uint64_t seed = 0;
    AnimalBody body;                 // J-06: its parts
};

struct Bestiary {
    std::vector<Species> species;    // land species first, then flyers, then swimmers
    int landCount = 0, flyerCount = 0, swimmerCount = 0;
    AnimalStyle style;               // J-06: what the world's animals share (its two hide colours among it)
    static Bestiary make(const Body& b, const BodyGen& g);
    // a land species for a biome: one whose habitat matches or is unrestricted; -1 when the world has none
    int pickLand(uint64_t h, int biome) const;
    int firstFlyer() const { return landCount < (int)species.size() && flyerCount ? landCount : -1; }
    int firstSwimmer() const { return swimmerCount ? landCount + flyerCount : -1; }
};
