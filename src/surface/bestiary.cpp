#include "bestiary.h"
#include "galaxy/starfield.h"
#include "core/rng.h"
#include <cmath>

const char* PLAN_NAMES[PLAN_COUNT] = {"QUADRUPED", "LONG-NECKED BROWSER", "HEXAPOD CRAWLER", "HOPPER", "BIPED STRIDER", "GIANT WALKER", "FLYER", "SWIMMER"};
const char* ACTIVITY_NAMES[3] = {"BY DAY", "BY NIGHT", "AT DUSK"};
const char* TEMPERAMENT_NAMES[3] = {"SHY", "CURIOUS", "INDIFFERENT"};

Bestiary Bestiary::make(const Body& b, const BodyGen& g) {
    Bestiary B;
    if (b.type != PT_FELISIAN && b.type != PT_OCEAN) return B;
    Rng r(b.seed ^ 0xBE57ULL);
    int nLand = b.type == PT_FELISIAN ? 2 + r.irange(3) : 0;
    int nFly = 1 + r.irange(2);
    int nSwim = 1;
    bool giantDone = false;
    int nocturnal = r.chance(0.5) ? r.irange(std::max(1, nLand)) : -1;
    static const int habitats[8] = {0, BIO_GRASSLAND, BIO_SAVANNA, BIO_TEMPERATE, BIO_TROPICAL, BIO_WETLAND, BIO_DESERT, BIO_TUNDRA};
    for (int i = 0; i < nLand; i++) {
        Species s;
        s.seed = r.next();
        double w[6] = {30, 15, 15, 15, 15, giantDone ? 0.0 : 10};
        s.plan = r.pick(w, 6);
        if (s.plan == PLAN_GIANT) giantDone = true;
        switch (s.plan) {
            case PLAN_QUADRUPED: s.size = 0.6 + 1.2 * r.uni(); s.gait = r.chance(0.5) ? GAIT_WALK : GAIT_TROT; s.runGait = GAIT_GALLOP; s.walkSpeed = 0.8 + 0.6 * r.uni(); s.runSpeed = 4 + 3 * r.uni(); s.herdMin = 4; s.herdMax = 9;
                s.bodyLen = s.size * (1.4 + 0.5 * r.uni()); s.neckLen = s.size * (0.35 + 0.3 * r.uni()); s.headLen = s.size * 0.4; s.tailLen = s.size * (0.3 + 0.6 * r.uni()); s.legLen = s.size * 0.95; break;
            case PLAN_LONGNECK: s.size = 1.5 + 2.0 * r.uni(); s.gait = GAIT_WALK; s.runGait = GAIT_TROT; s.walkSpeed = 0.7 + 0.4 * r.uni(); s.runSpeed = 3 + 2 * r.uni(); s.herdMin = 2; s.herdMax = 4;
                s.bodyLen = s.size * 1.3; s.neckLen = s.size * (0.9 + 0.6 * r.uni()); s.headLen = s.size * 0.3; s.tailLen = s.size * 0.7; s.legLen = s.size * 1.0; break;
            case PLAN_HEXAPOD: s.size = 0.3 + 0.5 * r.uni(); s.gait = GAIT_CRAWL; s.runGait = GAIT_CRAWL; s.walkSpeed = 0.3 + 0.4 * r.uni(); s.runSpeed = 1.5 + 1.0 * r.uni(); s.herdMin = 3; s.herdMax = 8;
                s.bodyLen = s.size * 3.0; s.neckLen = s.size * 0.2; s.headLen = s.size * 0.5; s.tailLen = s.size * 1.2; s.legLen = s.size * 0.5; break;
            case PLAN_HOPPER: s.size = 0.4 + 0.8 * r.uni(); s.gait = GAIT_HOP; s.runGait = GAIT_HOP; s.walkSpeed = 1.0 + 0.5 * r.uni(); s.runSpeed = 5 + 3 * r.uni(); s.herdMin = 3; s.herdMax = 6;
                s.bodyLen = s.size * 1.1; s.neckLen = s.size * 0.35; s.headLen = s.size * 0.35; s.tailLen = s.size * (0.8 + 0.5 * r.uni()); s.legLen = s.size * 0.9; break;
            case PLAN_BIPED: s.size = 0.8 + 1.4 * r.uni(); s.gait = GAIT_STRIDE; s.runGait = GAIT_STRIDE; s.walkSpeed = 1.0 + 0.5 * r.uni(); s.runSpeed = 4 + 3 * r.uni(); s.herdMin = 2; s.herdMax = 5;
                s.bodyLen = s.size * 0.9; s.neckLen = s.size * 0.45; s.headLen = s.size * 0.35; s.tailLen = s.size * (0.6 + 0.6 * r.uni()); s.legLen = s.size * 1.0; break;
            default: s.size = 4 + 4 * r.uni(); s.gait = GAIT_WALK; s.runGait = GAIT_WALK; s.walkSpeed = 0.6 + 0.3 * r.uni(); s.runSpeed = 1.5; s.herdMin = 1; s.herdMax = 2;
                s.bodyLen = s.size * 1.6; s.neckLen = s.size * (0.4 + 0.5 * r.uni()); s.headLen = s.size * 0.35; s.tailLen = s.size * 0.9; s.legLen = s.size * 1.0; break;
        }
        s.habitat = habitats[r.irange(8)];
        double wb[5] = {3, 3, 2, 2, 1};   // banks 3 forest, 10 straw, 9 sand, 0 rock, 5 flora2
        if (s.habitat == BIO_GRASSLAND || s.habitat == BIO_SAVANNA) { wb[1] = 5; wb[2] = 4; }
        if (s.habitat == BIO_DESERT) { wb[2] = 8; wb[0] = 1; }
        if (s.habitat == BIO_TEMPERATE || s.habitat == BIO_TROPICAL || s.habitat == BIO_WETLAND) { wb[0] = 5; wb[3] = 3; }
        static const int banks[5] = {3, 10, 9, 0, 5};
        s.bank = banks[r.pick(wb, 5)];
        s.tone = 0.45 + 0.5 * r.uni();
        s.pattern = r.irange(4);
        s.crest = r.irange(5);
        s.diet = (s.plan == PLAN_LONGNECK || s.plan == PLAN_GIANT) ? 1 : (r.chance(0.2) ? 2 : 0);
        s.activity = i == nocturnal ? ACT_NIGHT : (r.chance(0.15) ? ACT_DUSK : ACT_DAY);
        double wt[3] = {55, 25, 20};
        s.temperament = s.plan == PLAN_GIANT ? TEMP_INDIFFERENT : r.pick(wt, 3);
        s.call = s.size < 0.7 ? (r.chance(0.5) ? CALL_CHIRP : CALL_CLICK) : (s.size < 2.0 ? (r.chance(0.6) ? CALL_HOOT : CALL_CHIRP) : CALL_LOW);
        s.callPitch = 0.75 + 0.5 * r.uni();
        s.name = generateName(s.seed, r.chance(0.5) ? 0 : 2);
        B.species.push_back(s);
    }
    B.landCount = nLand;
    for (int i = 0; i < nFly; i++) {
        Species s;
        s.seed = r.next();
        s.plan = PLAN_FLYER;
        s.size = 0.25 + 0.6 * r.uni();
        s.walkSpeed = 6 + 4 * r.uni(); s.runSpeed = 14;
        s.bank = r.chance(0.5) ? 0 : 3; s.tone = 0.3 + 0.4 * r.uni();
        s.activity = r.chance(0.2) ? ACT_DUSK : ACT_DAY;
        s.call = r.chance(0.7) ? CALL_CHIRP : CALL_HOOT; s.callPitch = 0.8 + 0.6 * r.uni();
        s.herdMin = 6; s.herdMax = 19;
        s.name = generateName(s.seed, 2);
        B.species.push_back(s);
    }
    B.flyerCount = nFly;
    for (int i = 0; i < nSwim; i++) {
        Species s;
        s.seed = r.next();
        s.plan = PLAN_SWIMMER;
        s.size = 0.8 + 3.0 * r.uni();
        s.walkSpeed = 1.5; s.runSpeed = 4;
        s.bank = 3; s.tone = 0.25 + 0.3 * r.uni();
        s.call = CALL_LOW; s.callPitch = 0.6 + 0.4 * r.uni();
        s.name = generateName(s.seed, 1);
        B.species.push_back(s);
    }
    B.swimmerCount = nSwim;
    return B;
}

int Bestiary::pickLand(uint64_t h, int biome) const {
    if (!landCount) return -1;
    int fit[8]; int n = 0;
    for (int i = 0; i < landCount; i++) if (species[i].habitat == biome && n < 8) fit[n++] = i;
    if (n == 0) for (int i = 0; i < landCount; i++) if (species[i].habitat == 0 && n < 8) fit[n++] = i;
    if (n == 0) for (int i = 0; i < landCount; i++) if (n < 8) fit[n++] = i;
    return fit[(int)(unitFromHash(h) * n) % n];
}
