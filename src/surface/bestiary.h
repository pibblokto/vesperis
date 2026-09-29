// N3 (R-202): the bestiary of a world. A deterministic function of the body seed gives each living
// world two to four land species, one or two flyers and a swimmer, each with a body plan, size, gait,
// colours, habitat, diet, activity, temperament and a call. Sites spawn herds from it.
#pragma once
#include "galaxy/planetmap.h"
#include <string>
#include <vector>

enum BodyPlan { PLAN_QUADRUPED = 0, PLAN_LONGNECK, PLAN_HEXAPOD, PLAN_HOPPER, PLAN_BIPED, PLAN_GIANT, PLAN_FLYER, PLAN_SWIMMER, PLAN_COUNT };
extern const char* PLAN_NAMES[PLAN_COUNT];
enum Gait { GAIT_WALK = 0, GAIT_TROT, GAIT_GALLOP, GAIT_HOP, GAIT_CRAWL, GAIT_STRIDE };
enum Temperament { TEMP_SHY = 0, TEMP_CURIOUS, TEMP_INDIFFERENT };
enum Activity { ACT_DAY = 0, ACT_NIGHT, ACT_DUSK };
enum CallKind { CALL_NONE = 0, CALL_CHIRP, CALL_LOW, CALL_HOOT, CALL_CLICK };
extern const char* ACTIVITY_NAMES[3];
extern const char* TEMPERAMENT_NAMES[3];

struct Species {
    std::string name;
    int plan = PLAN_QUADRUPED;
    double size = 1;                 // shoulder height, metres
    double bodyLen = 1.6, neckLen = 0.5, headLen = 0.35, tailLen = 0.6, legLen = 0.8;   // metres
    double walkSpeed = 1.0, runSpeed = 4.0;
    int gait = GAIT_WALK, runGait = GAIT_GALLOP;
    int bank = 3;                    // palette bank of the hide
    double tone = 0.7;               // brightness of the hide 0.4..0.95
    int pattern = 0;                 // 0 plain, 1 stripes, 2 spots, 3 countershade
    int crest = 0;                   // 0 none, 1 horns, 2 ears, 3 crest, 4 antlers
    int habitat = 0;                 // preferred biome (BIO_*), 0 any
    int diet = 0;                    // 0 grass, 1 canopy, 2 water
    int activity = ACT_DAY;
    int temperament = TEMP_SHY;
    int call = CALL_CHIRP;
    double callPitch = 1;
    int herdMin = 3, herdMax = 8;
    uint64_t seed = 0;
};

struct Bestiary {
    std::vector<Species> species;    // land species first, then flyers, then swimmers
    int landCount = 0, flyerCount = 0, swimmerCount = 0;
    static Bestiary make(const Body& b, const BodyGen& g);
    // a land species for a biome: one whose habitat matches or is unrestricted; -1 when the world has none
    int pickLand(uint64_t h, int biome) const;
    int firstFlyer() const { return landCount < (int)species.size() && flyerCount ? landCount : -1; }
    int firstSwimmer() const { return swimmerCount ? landCount + flyerCount : -1; }
};
