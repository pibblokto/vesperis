#include "bestiary.h"
#include "galaxy/starfield.h"
#include "core/rng.h"
#include <cmath>

const char* const ACTIVITY_NAMES[3] = {"BY DAY", "BY NIGHT", "AT DUSK"};
const char* const TEMPERAMENT_NAMES[3] = {"SHY", "CURIOUS", "INDIFFERENT"};
const char* const GAIT_NAMES[GAIT_COUNT] = {"WALK", "TROT", "GALLOP", "HOP", "CRAWL", "STRIDE", "WAVE OF LEGS", "SLITHER", "BOUND", "FLAP", "GLIDE",
                                            "SWIM", "DRIFT"};

namespace {
// J-06: the land plans and their weights by the kind's style of animal (P-02's three): the felisian's, the cold's (big grazers, a giant
// walker among them more often, burrowers), the open grass's herds
constexpr int LAND_PLANS[12] = {PLAN_QUADRUPED, PLAN_LONGNECK, PLAN_HEXAPOD, PLAN_HOPPER, PLAN_BIPED, PLAN_GIANT, PLAN_OCTOPOD, PLAN_SERPENT,
                                PLAN_CRAWLER, PLAN_SLUG, PLAN_BURROWER, PLAN_CLIMBER};
constexpr double LAND_W[3][12] = {{24, 11, 9, 9, 10, 7, 5, 6, 5, 4, 5, 5}, {40, 6, 3, 5, 8, 20, 2, 2, 2, 2, 8, 2}, {40, 16, 3, 12, 8, 4, 2, 3, 2, 2, 5, 3}};
// a species' seed: its world's, its kind of place (land, air, water) and its place among them; nothing else
uint64_t speciesSeed(uint64_t world, int cat, int i) { return mix64(world ^ 0xBE57A18ULL ^ (uint64_t)(cat * 16 + i + 1) * 0x9E3779B97F4A7C15ULL); }
// the size a plan's animals come in (shoulder heights; a flyer's, a floater's and a swimmer's lengths): a hand to a house
double sizeOf(int plan, Rng& e) {
    switch (plan) {
        case PLAN_QUADRUPED: return e.range(0.3, 2.0);
        case PLAN_LONGNECK: return e.range(1.5, 4.0);
        case PLAN_HEXAPOD: return e.range(0.08, 0.9);
        case PLAN_HOPPER: return e.range(0.15, 1.4);
        case PLAN_BIPED: return e.range(0.6, 2.4);
        case PLAN_GIANT: return e.range(4.0, 9.0);
        case PLAN_OCTOPOD: return e.range(0.1, 1.1);
        case PLAN_SERPENT: return e.range(0.06, 0.45);
        case PLAN_CRAWLER: return e.range(0.04, 0.5);
        case PLAN_SLUG: return e.range(0.04, 0.5);
        case PLAN_BURROWER: return e.range(0.1, 0.8);
        case PLAN_CLIMBER: return e.range(0.2, 1.3);
        case PLAN_GLIDER: return e.range(0.15, 0.8);
        case PLAN_FLYER: return e.range(0.1, 1.6);
        case PLAN_FLOATER: return e.range(1.0, 6.0);
        default: return e.range(0.3, 6.0);   // a swimmer
    }
}
}

Bestiary Bestiary::make(const Body& b, const BodyGen& g) {
    Bestiary B;
    if (!kindHas(b.type, KT_FAUNA)) return B;
    const KindDef& K = KINDS[b.type];
    const int fa = K.fauna;
    Rng r(mix64(b.seed ^ 0xBE5718ULL));
    const int nLand = kindHas(b.type, KT_FLORA) ? 3 + r.irange(3) : 0;   // land animals where there are plants
    const int nFly = 1 + r.irange(2);
    const int nSwim = (nLand ? 2 : 3) + r.irange(2);
    const int nocturnal = r.chance(0.5) ? r.irange(std::max(1, nLand)) : -1;
    const bool thickAir = K.pressureSpan > 0 && K.pressureLo + K.pressureSpan * hash01(g.seed) >= 1.3;
    B.style = animalStyleOf(b.seed, fa);
    // P-02's habitats by the kind's style
    static const int habitats[3][8] = {{0, BIO_GRASSLAND, BIO_SAVANNA, BIO_TEMPERATE, BIO_TROPICAL, BIO_WETLAND, BIO_DESERT, BIO_TUNDRA},
                                       {0, BIO_TUNDRA, BIO_TAIGA, BIO_GRASSLAND, BIO_TUNDRA, BIO_TAIGA, BIO_TUNDRA, BIO_GRASSLAND},
                                       {0, BIO_GRASSLAND, BIO_SAVANNA, BIO_GRASSLAND, BIO_SAVANNA, BIO_TEMPERATE, BIO_DESERT, BIO_GRASSLAND}};
    bool giantDone = false;
    auto ecology = [&](Species& s, Rng& e) {   // speeds, gaits and herds by the plan; habitat, diet, activity, temperament and call
        switch (s.plan) {
            case PLAN_QUADRUPED: s.gait = e.chance(0.5) ? GAIT_WALK : GAIT_TROT; s.runGait = s.size < 0.6 ? GAIT_BOUND : GAIT_GALLOP; s.walkSpeed = e.range(0.8, 1.4); s.runSpeed = e.range(4, 7); s.herdMin = 4; s.herdMax = 9; break;
            case PLAN_LONGNECK: s.gait = GAIT_WALK; s.runGait = GAIT_TROT; s.walkSpeed = e.range(0.7, 1.1); s.runSpeed = e.range(3, 5); s.herdMin = 2; s.herdMax = 4; break;
            case PLAN_HEXAPOD: s.gait = s.runGait = GAIT_CRAWL; s.walkSpeed = e.range(0.3, 0.7); s.runSpeed = e.range(1.5, 2.5); s.herdMin = 3; s.herdMax = 8; break;
            case PLAN_HOPPER: s.gait = s.runGait = GAIT_HOP; s.walkSpeed = e.range(1.0, 1.5); s.runSpeed = e.range(5, 8); s.herdMin = 3; s.herdMax = 6; break;
            case PLAN_BIPED: s.gait = s.runGait = GAIT_STRIDE; s.walkSpeed = e.range(1.0, 1.5); s.runSpeed = e.range(4, 7); s.herdMin = 2; s.herdMax = 5; break;
            case PLAN_GIANT: s.gait = s.runGait = GAIT_WALK; s.walkSpeed = e.range(0.6, 0.9); s.runSpeed = 1.5; s.herdMin = 1; s.herdMax = 2; break;
            case PLAN_OCTOPOD: s.gait = s.runGait = GAIT_WAVE; s.walkSpeed = e.range(0.3, 0.8); s.runSpeed = e.range(2, 3.5); s.herdMin = 1; s.herdMax = 4; break;
            case PLAN_SERPENT: s.gait = s.runGait = GAIT_SLITHER; s.walkSpeed = e.range(0.3, 0.6); s.runSpeed = e.range(1.5, 2.5); s.herdMin = 1; s.herdMax = 2; break;
            case PLAN_CRAWLER: s.gait = s.runGait = GAIT_WAVE; s.walkSpeed = e.range(0.2, 0.5); s.runSpeed = e.range(0.8, 1.5); s.herdMin = 2; s.herdMax = 6; break;
            case PLAN_SLUG: s.gait = s.runGait = GAIT_CRAWL; s.walkSpeed = e.range(0.03, 0.08); s.runSpeed = 0.12; s.herdMin = 2; s.herdMax = 6; break;
            case PLAN_BURROWER: s.gait = GAIT_WALK; s.runGait = GAIT_TROT; s.walkSpeed = e.range(0.4, 0.8); s.runSpeed = e.range(2, 3); s.herdMin = 1; s.herdMax = 4; break;
            case PLAN_CLIMBER: s.gait = GAIT_WALK; s.runGait = GAIT_BOUND; s.walkSpeed = e.range(0.8, 1.2); s.runSpeed = e.range(3, 5); s.herdMin = 2; s.herdMax = 6; break;
            case PLAN_GLIDER: s.gait = s.runGait = GAIT_GLIDE; s.walkSpeed = e.range(4, 7); s.runSpeed = 10; s.herdMin = 2; s.herdMax = 6; break;
            case PLAN_FLYER: s.gait = s.runGait = GAIT_FLAP; s.walkSpeed = e.range(6, 10); s.runSpeed = 14; s.herdMin = 6; s.herdMax = 19; break;
            case PLAN_FLOATER: s.gait = s.runGait = GAIT_DRIFT; s.walkSpeed = e.range(0.5, 1.5); s.runSpeed = 2; s.herdMin = 1; s.herdMax = 5; break;
            default: s.gait = s.runGait = GAIT_SWIM; s.walkSpeed = 1.5; s.runSpeed = 4; s.herdMin = 1; s.herdMax = 3; break;
        }
        const bool land = !planFlies(s.plan) && !planSwims(s.plan);
        if (land && fa == FA_COLD && s.plan != PLAN_HEXAPOD && s.plan != PLAN_CRAWLER && s.plan != PLAN_SLUG) { s.size *= 1.3; s.herdMin += 2; s.herdMax += 6; }
        if (land && fa == FA_HERDS && (s.plan == PLAN_QUADRUPED || s.plan == PLAN_LONGNECK || s.plan == PLAN_HOPPER)) { s.herdMin += 4; s.herdMax += 12; }
        s.habitat = land ? habitats[fa][e.irange(8)] : 0;
        s.diet = (s.plan == PLAN_LONGNECK || s.plan == PLAN_GIANT) ? 1 : (e.chance(0.2) ? 2 : 0);
        double wt[3] = {55, 25, 20};
        s.temperament = s.plan == PLAN_GIANT ? TEMP_INDIFFERENT : e.pick(wt, 3);
        s.call = planSwims(s.plan) ? CALL_LOW : (s.size < 0.7 ? (e.chance(0.5) ? CALL_CHIRP : CALL_CLICK) : (s.size < 2.0 ? (e.chance(0.6) ? CALL_HOOT : CALL_CHIRP) : CALL_LOW));
        s.callPitch = e.range(0.75, 1.25);
    };
    auto add = [&](int cat, int i, int plan, Rng& e) {
        Species s;
        s.seed = speciesSeed(b.seed, cat, i);
        s.plan = plan;
        s.size = sizeOf(plan, e);
        ecology(s, e);
        s.activity = cat == 0 && i == nocturnal ? ACT_NIGHT : (e.chance(0.15) ? ACT_DUSK : ACT_DAY);
        s.name = generateName(s.seed, cat == 2 ? 1 : (e.chance(0.5) ? 0 : 2));
        // the body: drawn again while it looks like one of its world's kept before it, else the draw farthest from them
        double bestD = -1;
        for (int nudge = 0; nudge < 24; nudge++) {
            const AnimalBody a = bodyOf(s.seed, s.plan, s.size, B.style, nudge);
            double nearest = 1e9;
            for (const Species& o : B.species) nearest = std::min(nearest, animalDistance(a, o.body));
            if (nearest > bestD) { bestD = nearest; s.body = a; }
            if (nearest >= ANIMAL_ALIKE) break;
        }
        s.bodyLen = s.body.length;
        s.legLen = s.body.pairs ? std::max(0.05, s.size - 0.8 * s.body.depth) : std::max(0.1, 0.25 * s.body.length);
        B.species.push_back(s);
    };
    for (int i = 0; i < nLand; i++) {
        Rng e(mix64(speciesSeed(b.seed, 0, i) ^ 0xEC0ULL));
        double w[12];
        for (int k = 0; k < 12; k++) w[k] = LAND_W[fa][k];
        if (giantDone) w[5] = 0;   // one giant a world at most
        const int plan = LAND_PLANS[e.pick(w, 12)];
        if (plan == PLAN_GIANT) giantDone = true;
        add(0, i, plan, e);
    }
    B.landCount = nLand;
    for (int i = 0; i < nFly; i++) {
        Rng e(mix64(speciesSeed(b.seed, 1, i) ^ 0xEC0ULL));
        const double roll = e.uni();
        const int plan = thickAir && i == nFly - 1 && roll < 0.6 ? PLAN_FLOATER : (nLand && roll > 0.72 ? PLAN_GLIDER : PLAN_FLYER);
        add(1, i, plan, e);
    }
    B.flyerCount = nFly;
    for (int i = 0; i < nSwim; i++) {
        Rng e(mix64(speciesSeed(b.seed, 2, i) ^ 0xEC0ULL));
        add(2, i, PLAN_SWIMMER, e);
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
