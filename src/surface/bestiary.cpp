#include "bestiary.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

const char* const ACTIVITY_NAMES[3] = {"BY DAY", "BY NIGHT", "AT DUSK"};
const char* const TEMPERAMENT_NAMES[3] = {"SHY", "CURIOUS", "INDIFFERENT"};
const char* const GAIT_NAMES[GAIT_COUNT] = {"WALK", "TROT", "GALLOP", "HOP", "CRAWL", "STRIDE", "WAVE OF LEGS", "SLITHER", "BOUND", "FLAP", "GLIDE",
                                            "SWIM", "DRIFT"};
const char* const ANIMAL_WAY_NAMES[WAY_COUNT] = {"GRAZER", "BROWSER", "HUNTER", "SCAVENGER", "BURROWER", "CLIMBER", "FLOCK", "SWARM", "SOARER",
                                                 "SHOAL", "SHALLOWS", "LEAPER", "DRIFTER"};
const char* const SOCIAL_NAMES[SOCIAL_COUNT] = {"IN HERDS", "ALONE", "TERRITORIAL"};

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
// J-07: how a species lives, by its plan and its body (a swimmer's form, a flyer's wings and size); the walkers that may hunt by weights
int wayOf(const Species& s, Rng& e) {
    switch (s.plan) {
        case PLAN_LONGNECK: return WAY_BROWSER;
        case PLAN_GIANT: return e.chance(0.6) ? WAY_BROWSER : WAY_GRAZER;
        case PLAN_BURROWER: return WAY_BURROWER;
        case PLAN_CLIMBER: return WAY_CLIMBER;
        case PLAN_SLUG: return WAY_GRAZER;
        case PLAN_SERPENT: return WAY_HUNTER;
        case PLAN_GLIDER: return WAY_FLOCK;
        case PLAN_FLOATER: return WAY_DRIFTER;
        case PLAN_FLYER:
            if (s.body.wings == WINGS_INSECT && s.size < 0.35) return WAY_SWARM;
            return e.chance(s.size > 0.8 ? 0.6 : 0.12) ? WAY_SOARER : WAY_FLOCK;
        case PLAN_SWIMMER:
            switch (s.body.variant) {
                case SWIM_EEL: return WAY_SHALLOWS;
                case SWIM_RAY: return e.chance(0.7) ? WAY_SHALLOWS : WAY_SHOAL;
                case SWIM_LONGNECK: return e.chance(0.5) ? WAY_SHALLOWS : WAY_SHOAL;
                default: return e.chance(0.45) ? WAY_LEAPER : WAY_SHOAL;
            }
        default: break;
    }
    // grazing, hunting, scavenging, browsing (or climbing: an octopod's)
    static const double W[5][4] = {{60, 22, 10, 8}, {45, 25, 30, 0}, {80, 10, 10, 0}, {35, 45, 20, 0}, {0, 50, 30, 20}};
    const int row = s.plan == PLAN_HEXAPOD || s.plan == PLAN_CRAWLER ? 1 : (s.plan == PLAN_HOPPER ? 2 : (s.plan == PLAN_BIPED ? 3 : (s.plan == PLAN_OCTOPOD ? 4 : 0)));
    double w[4] = {W[row][0], W[row][1], W[row][2], W[row][3]};
    if (s.plan == PLAN_CRAWLER) { w[0] = 40; w[1] = 0; w[2] = 60; }   // a crawler grazes or scavenges
    const int k = e.pick(w, 4);
    return k == 0 ? WAY_GRAZER : (k == 1 ? WAY_HUNTER : (k == 2 ? WAY_SCAVENGER : (s.plan == PLAN_OCTOPOD ? WAY_CLIMBER : WAY_BROWSER)));
}
// J-07: how many live together, and the herds' sizes that follow: a lone one, a pair holding its ground, a pack of hunters, a soarer's few, a
// swarm's many
int socialOf(Species& s, Rng& e) {
    int social = SOCIAL_HERD;
    const bool land = !planFlies(s.plan) && !planSwims(s.plan);
    if (land) {
        const double roll = e.uni();
        if (s.plan == PLAN_SERPENT) social = SOCIAL_SOLITARY;
        else if (s.way == WAY_HUNTER || s.way == WAY_SCAVENGER) social = roll < 0.5 ? SOCIAL_SOLITARY : SOCIAL_HERD;
        else if (roll < 0.12 && s.plan != PLAN_GIANT && s.plan != PLAN_SLUG && s.plan != PLAN_CRAWLER) social = SOCIAL_TERRITORIAL;
        else if (roll < 0.2) social = SOCIAL_SOLITARY;
    }
    if (social == SOCIAL_SOLITARY) s.herdMin = s.herdMax = 1;
    else if (social == SOCIAL_TERRITORIAL) { s.herdMin = 1; s.herdMax = 2; }
    else if (s.way == WAY_HUNTER) { s.herdMin = 2; s.herdMax = 4; }   // a pack
    else if (s.way == WAY_SCAVENGER) { s.herdMin = 2; s.herdMax = 3; }
    else if (s.way == WAY_SOARER) { s.herdMin = 1; s.herdMax = 3; }
    else if (s.way == WAY_SWARM) { s.herdMin = 20; s.herdMax = 40; }
    return social;
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
        s.way = wayOf(s, e);   // J-07: after every draw before it (the body's are its own stream's)
        s.social = socialOf(s, e);
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
    // J-07: a hunter's prey, the land species nearest its size among those that neither hunt, crawl as slugs nor walk as giants, at most
    // twice its size; a hunter with none scavenges
    for (int i = 0; i < nLand; i++) {
        Species& h = B.species[i];
        if (h.way != WAY_HUNTER) continue;
        double best = 1e9;
        for (int j = 0; j < nLand; j++) {
            const Species& p = B.species[j];
            if (j == i || p.way == WAY_HUNTER || p.plan == PLAN_SLUG || p.plan == PLAN_GIANT || p.size > 2.0 * h.size || p.size < 0.08 * h.size) continue;
            const double d = std::fabs(std::log(p.size / h.size));
            if (d < best) { best = d; h.prey = j; }
        }
        if (h.prey < 0) { h.way = WAY_SCAVENGER; if (h.social == SOCIAL_HERD) { h.herdMin = 2; h.herdMax = 3; } }
    }
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

double speciesStride(const Species& sp, double k, double speed, double g) {
    if (sp.body.pairs == 0) return (sp.plan == PLAN_SERPENT ? 0.67 : 0.3) * sp.body.length * k;
    const double h = std::max(0.03, sp.legLen * k), fr = speed * speed / (std::max(g, 0.5) * h);
    return clampd(2.3 * h * std::pow(std::max(fr, 1e-4), 0.3), 0.35 * h, 4.5 * h) * (sp.gait == GAIT_HOP ? 1.4 : 1.0);
}

int speciesGait(const Species& sp, double k, double speed, double g, bool running) {
    if (sp.gait == sp.runGait) return sp.gait;
    const double h = std::max(0.03, sp.legLen * k), fr = speed * speed / (std::max(g, 0.5) * h);
    if (running || fr > 2.5) return sp.runGait;
    if (fr > 0.4 && sp.body.pairs == 2 && sp.runGait != GAIT_TROT) return GAIT_TROT;
    return sp.gait;
}

int Bestiary::pickLand(uint64_t h, int biome) const {
    if (!landCount) return -1;
    int fit[8]; int n = 0;
    for (int i = 0; i < landCount; i++) if (species[i].habitat == biome && n < 8) fit[n++] = i;
    if (n == 0) for (int i = 0; i < landCount; i++) if (species[i].habitat == 0 && n < 8) fit[n++] = i;
    if (n == 0) for (int i = 0; i < landCount; i++) if (n < 8) fit[n++] = i;
    return fit[(int)(unitFromHash(h) * n) % n];
}

namespace {
// a species' kind in a word or two (a swimmer's by its form)
std::string kindWords(const Species& s) {
    static const char* const KIND[PLAN_COUNT] = {"QUADRUPED", "BROWSER", "HEXAPOD", "HOPPER", "STRIDER", "GIANT", "FLYER", "SWIMMER", "OCTOPOD",
                                                 "SERPENT", "CRAWLER", "SLUG", "BURROWER", "CLIMBER", "GLIDER", "FLOATER"};
    if (s.plan == PLAN_SWIMMER) return s.body.variant == SWIM_LONGNECK ? "LONG-NECKED SWIMMER" : SWIM_NAMES[clampi(s.body.variant, 0, SWIM_COUNT - 1)];
    return KIND[clampi(s.plan, 0, PLAN_COUNT - 1)];
}
// what tells a species from others of its kind, in the order the eye takes it: its coat, its head's ornament, its back, its hide, its tail,
// its eyes; "" where the part says nothing (no ornament)
std::string lookWord(const AnimalBody& a, int part) {
    static const char* const COAT[COAT_COUNT] = {"PLAIN", "STRIPED", "SPOTTED", "ROSETTED", "BANDED", "SADDLED", "EYE-SPOTTED", "PALE-BELLIED",
                                                 "MOTTLED", "BACK-STRIPED"};
    static const char* const ORN[ORN_COUNT] = {"", "BIG-JAWED", "TUSKED", "HORNED", "CURVE-HORNED", "SPIRAL-HORNED", "ANTLERED", "NOSE-HORNED",
                                               "LONG-EARED", "FRILLED", "FEELERED"};
    static const char* const BACK[DORSAL_COUNT] = {"", "PLATED", "SPINED", "SAIL-BACKED", "SHELLED", "HUMPED", "FINNED"};
    static const char* const HIDE[HIDE_COUNT] = {"FURRED", "SCALED", "ARMOURED", "BARE", "WARTY", "QUILLED", "SHAGGY"};
    static const char* const TAIL[TAIL_COUNT] = {"TAILLESS", "WHIP-TAILED", "THICK-TAILED", "CLUB-TAILED", "FAN-TAILED", "FIN-TAILED", "STINGING",
                                                 "CURL-TAILED"};
    static const char* const EYES[9] = {"EYELESS", "ONE-EYED", "", "THREE-EYED", "FOUR-EYED", "FIVE-EYED", "SIX-EYED", "SEVEN-EYED", "EIGHT-EYED"};
    switch (part) {
        case 0: return COAT[clampi(a.coat, 0, COAT_COUNT - 1)];
        case 1: return ORN[clampi(a.ornament, 0, ORN_COUNT - 1)];
        case 2: return BACK[clampi(a.dorsal, 0, DORSAL_COUNT - 1)];
        case 3: return HIDE[clampi(a.hide, 0, HIDE_COUNT - 1)];
        case 4: return TAIL[clampi(a.tail, 0, TAIL_COUNT - 1)];
        default: return EYES[clampi(a.eyes, 0, 8)];
    }
}
}

std::string Bestiary::words(int i) const {
    if (i < 0 || i >= (int)species.size()) return "ANIMAL";
    const Species& s = species[i];
    const std::string kind = kindWords(s);
    std::vector<int> others;
    for (int j = 0; j < (int)species.size(); j++) if (j != i && kindWords(species[j]) == kind) others.push_back(j);
    if (others.empty()) return kind;
    for (int part = 0; part < 6; part++) {   // the first part whose word is its own among them
        const std::string w = lookWord(s.body, part);
        if (w.empty()) continue;
        bool own = true;
        for (int j : others) if (lookWord(species[j].body, part) == w) { own = false; break; }
        if (own) return w + " " + kind;
    }
    int smaller = 0;   // alike in all of those: by size among them
    for (int j : others) smaller += species[j].size < s.size;
    return std::string(smaller == 0 ? "SMALL " : (smaller == (int)others.size() ? "LARGE " : "MIDDLING ")) + kind;
}

// ---- J-08: the fates
const char* const CHANGE_NAMES[CHG_COUNT] = {"RIBS SHOWING", "MELTING HIDE", "OPEN SORES", "BARE PATCHES", "FLAYED", "BLOATED BODY", "TUMOURS",
                                            "STALKS", "STONE PLATES", "OVERGROWN TUSKS", "HORN GROWTHS", "EXTRA LEG", "MISSING LEG", "TWO HEADS",
                                            "EXTRA EYES", "NO EYES", "TWISTED SPINE", "JOINED PAIR", "SHORT LEGS", "LONG LEGS", "LONG NECK",
                                            "EXTRA TAIL", "GIANT", "DWARF", "NEW COLOUR", "PALE", "DARK", "GLOWING SPOTS", "GLOWING STRIPES",
                                            "NEW PATTERN", "IRIDESCENT", "WOOLLY", "HAIRLESS", "SEE-THROUGH", "CALM", "BOLD", "WALKS AT NIGHT",
                                            "CIRCLES", "STUMBLES", "CLIMBS", "SWARMS", "LIMPS"};

bool speciesShelled(const Species& sp) { return sp.body.dorsal == DORSAL_SHELL || sp.body.dorsal == DORSAL_PLATES || sp.body.hide == HIDE_PLATES; }

AnimalFate animalFateOf(const Species& sp, const MadeFate& f, const int* changes, int nChanges) {
    AnimalFate a;
    if (!f.on()) return a;
    const bool air = planFlies(sp.plan), water = planSwims(sp.plan), land = !air && !water;
    double h = f.harm[air ? MFC_AIR : (water ? MFC_WATER : MFC_LAND)];
    if (land && f.shelled > 0 && speciesShelled(sp)) h = 1 - (1 - h) * (1 - f.shelled);
    if (land && f.starve > 0) {   // the plant eaters' food lost, the hunters' prey thinned
        const double eats = sp.way == WAY_GRAZER || sp.way == WAY_BROWSER || sp.way == WAY_CLIMBER ? 1.0 : (sp.way == WAY_BURROWER ? 0.7 : 0.5);
        h = 1 - (1 - h) * (1 - eats * f.starve);
    }
    const double frail = 0.5 + hash01(mix64(sp.seed ^ 0xFA7E0001ULL));   // how it resists: 0.5 the hardiest, 1.5 the frailest
    a.dead = h >= 0.95 ? 1.0 : clampd(h * frail, 0, 1);
    a.death = a.dead > 0 ? f.death : 0;
    // B-469: a flyer's and a swimmer's survivors sick and their young changed too where the line meets them: its class harmed, or a host on
    // the skin (a glow, a dye), which meets every animal; a sprouting one's climb to the high ground is the land's alone
    const bool meets = land || h > 0 || f.sickness == MF_GLOWING || f.sickness == MF_DYED;
    if (meets && !a.gone() && f.sickness && f.sick > 0 && (land || f.sickness != MF_SPROUTING)) { a.sickness = f.sickness; a.sick = f.sick; }
    if (meets && !a.gone() && f.changed > 0 && nChanges > 0) {
        a.changed = f.changed;
        a.nChanges = std::min(nChanges, 3);
        for (int i = 0; i < a.nChanges; i++) a.changes[i] = changes[i];
    }
    a.dye = f.dye;
    a.age = f.age;
    a.thrive = a.dead > 0 ? 0.0 : f.thrive;
    return a;
}

namespace {
// whether a change fits a species' body and ways, and whether two clash. B-469: a flyer's or a swimmer's young take the body's and the hide's
// changes alone (the ways are a herd's members'), a swimmer's no leg's nor wool
bool changeFits(const Species& sp, int ch) {
    const AnimalBody& b = sp.body;
    const bool legs = b.pairs > 0;
    if (planFlies(sp.plan) || planSwims(sp.plan))
        switch (ch) {
            case CHG_CALM: case CHG_BOLD: case CHG_NIGHT: case CHG_CIRCLES: case CHG_STUMBLES: case CHG_CLIMBS: case CHG_SWARMS: case CHG_LIMPS: case CHG_JOINED:
                return false;
            case CHG_EXTRA_LEG: case CHG_MISSING_LEG: case CHG_SHORT_LEGS: case CHG_LONG_LEGS: case CHG_WOOLLY:
                if (planSwims(sp.plan)) return false;
                break;
            default: break;
        }
    switch (ch) {
        case CHG_TUSKS: return sp.plan != PLAN_SLUG && b.head != HEAD_BEAK && b.head != HEAD_MANDIBLES;
        case CHG_EXTRA_LEG: case CHG_SHORT_LEGS: case CHG_LONG_LEGS: case CHG_CIRCLES: case CHG_STUMBLES: case CHG_LIMPS: return legs;
        case CHG_MISSING_LEG: return b.pairs >= 2;
        case CHG_EXTRA_EYES: return b.eyes >= 1 && b.eyes <= 6;
        case CHG_NO_EYES: return b.eyes >= 1;
        case CHG_JOINED: return legs && sp.plan != PLAN_GIANT;
        case CHG_LONG_NECK: return b.neck > 0.03 * b.size;
        case CHG_EXTRA_TAIL: return b.tail != TAIL_NONE && b.tailLen > 0.02 * b.size;
        case CHG_WOOLLY: return b.hide != HIDE_HAIR;
        case CHG_HAIRLESS: return b.hide == HIDE_FUR || b.hide == HIDE_HAIR || b.hide == HIDE_QUILLS;
        case CHG_CALM: return sp.temperament != TEMP_INDIFFERENT;
        case CHG_BOLD: return sp.temperament != TEMP_CURIOUS && sp.social != SOCIAL_TERRITORIAL;
        case CHG_NIGHT: return sp.activity != ACT_NIGHT;
        case CHG_CLIMBS: return legs && sp.way != WAY_CLIMBER;
        case CHG_SWARMS: return sp.social == SOCIAL_HERD;
        case CHG_GIANT: return sp.plan != PLAN_GIANT;
        default: return true;
    }
}
bool changesClash(int a, int b) {
    static const int PAIRS[][2] = {{CHG_GIANT, CHG_DWARF}, {CHG_PALE, CHG_DARK}, {CHG_SHORT_LEGS, CHG_LONG_LEGS}, {CHG_WOOLLY, CHG_HAIRLESS}, {CHG_EXTRA_LEG, CHG_MISSING_LEG},
                                   {CHG_CALM, CHG_BOLD}, {CHG_EXTRA_EYES, CHG_NO_EYES}, {CHG_RIBS, CHG_BLOATED}, {CHG_SEE_THROUGH, CHG_STONE}, {CHG_JOINED, CHG_TWO_HEADS}};
    for (const auto& p : PAIRS) if ((a == p[0] && b == p[1]) || (a == p[1] && b == p[0])) return true;
    return a == b;
}
}

int animalChangesOf(const Species& sp, int steer, int out[3]) {
    const uint64_t h = mix64(sp.seed ^ 0xC4A9E0ULL);
    const int want = 1 + (int)(h % 3);
    int n = 0;
    auto take = [&](int ch) {
        if (n >= want || !changeFits(sp, ch)) return;
        for (int i = 0; i < n; i++) if (changesClash(out[i], ch)) return;
        out[n++] = ch;
    };
    // the first by the lines' genes: a big cell's giants, a small one's dwarfs, a dye's new colour, a glow's spots or stripes, a spiral's twist, a
    // line that never stops tumours or a leg more, a flesh rotter's sores, a shell's stone plates or horns, a weak nerve poison's circling
    int steered[12], ns = 0;
    const bool odd = (h >> 8) & 1;
    if (steer & MSR_BIG) steered[ns++] = CHG_GIANT;
    if (steer & MSR_SMALL) steered[ns++] = CHG_DWARF;
    if (steer & MSR_DYE) steered[ns++] = CHG_NEW_COLOUR;
    if (steer & MSR_GLOW) steered[ns++] = odd ? CHG_GLOW_STRIPES : CHG_GLOW_SPOTS;
    if (steer & MSR_SPIRAL) steered[ns++] = CHG_TWISTED;
    if (steer & MSR_STOP) steered[ns++] = odd ? CHG_EXTRA_LEG : CHG_TUMOURS;
    if (steer & MSR_ROT) steered[ns++] = CHG_SORES;
    if (steer & MSR_SHELL) steered[ns++] = odd ? CHG_HORNS : CHG_STONE;
    if (steer & MSR_NERVE) steered[ns++] = CHG_CIRCLES;
    for (int k = 0; k < ns && n == 0; k++) take(steered[(k + (int)((h >> 12) % (uint64_t)ns)) % ns]);
    for (int k = 0; k < 64 && n < want; k++) take((int)(mix64(h + 0x9E37ULL * (uint64_t)(k + 1)) % CHG_COUNT));
    return n;
}

AnimalBody changedBody(const AnimalBody& b0, const int* changes, int nChanges) {
    AnimalBody b = b0;
    const uint64_t h = mix64(b0.seed ^ 0xC4A9E1ULL);
    for (int i = 0; i < nChanges; i++)
        switch (changes[i]) {
            case CHG_BLOATED: b.width *= 1.3; b.depth *= 1.25; break;
            case CHG_TUSKS:
                if (b.ornament != ORN_TUSKS) { b.ornament = ORN_TUSKS; b.ornamentLen = std::max(b.ornamentLen, 1.2 * b.headLen); }
                b.ornamentLen *= 2.2;
                break;
            case CHG_STONE: if (b.dorsal == DORSAL_NONE) { b.dorsal = DORSAL_PLATES; b.dorsalH = 0.3 * b.depth; } break;
            case CHG_EXTRA_LEG: b.extraLeg = true; break;
            case CHG_MISSING_LEG: b.missingLeg = (int)(h % (uint64_t)(2 * std::min(b.pairs, 6))); break;
            case CHG_TWO_HEADS: b.heads = 2; break;
            case CHG_EXTRA_EYES: b.eyes = std::min(8, b.eyes + 2 + (int)((h >> 4) % 3)); break;
            case CHG_NO_EYES: b.eyes = 0; break;
            case CHG_TWISTED: b.twist = 0.35; break;
            case CHG_JOINED: b.joined = true; break;
            case CHG_SHORT_LEGS:
                b.size = std::max(0.35 * b.size, b.size - 0.4 * b.leg); b.leg *= 0.6;
                if (b.wings != WINGS_NONE) b.span *= 0.7;   // (B-469: a flyer's wings stunted with them)
                break;
            case CHG_LONG_LEGS:
                b.size += 0.4 * b.leg; b.leg *= 1.4;
                if (b.wings != WINGS_NONE) b.span *= 1.25;
                break;
            case CHG_LONG_NECK: b.neck = std::max(2.0 * b.neck, 0.6 * b.size); b.neckUp = std::max(b.neckUp, 0.6); break;
            case CHG_EXTRA_TAIL: b.extraTail = true; break;
            case CHG_NEW_PATTERN: b.coat = (b.coat + 1 + (int)((h >> 9) % (COAT_COUNT - 1))) % COAT_COUNT; break;
            case CHG_WOOLLY: b.hide = HIDE_HAIR; break;
            case CHG_HAIRLESS: b.hide = HIDE_WET; b.coat = COAT_PLAIN; b.tone = std::min(1.0, b.tone * 1.15); break;
            case CHG_PALE: b.tone = std::min(1.0, b.tone * 1.35); b.coatTone = std::min(1.0, b.coatTone * 1.35); break;
            case CHG_DARK: b.tone *= 0.45; b.coatTone *= 0.5; break;
            default: break;
        }
    return b;
}

bool changeDrawn(int ch) {
    switch (ch) {
        case CHG_RIBS: case CHG_MELTING: case CHG_SORES: case CHG_BARE: case CHG_FLAYED: case CHG_TUMOURS: case CHG_STALKS: case CHG_STONE: case CHG_HORNS:
        case CHG_GLOW_SPOTS: case CHG_GLOW_STRIPES: case CHG_IRIDESCENT: case CHG_SEE_THROUGH: case CHG_NEW_COLOUR: return true;
        default: return false;
    }
}
