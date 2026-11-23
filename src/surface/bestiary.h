// N3 (R-202): the bestiary of a world. A deterministic function of the body seed gives each living world its land species, flyers and
// swimmers, each with a body plan, size, gait, habitat, diet, activity, temperament and a call. Sites spawn herds from it. J-06 (generation
// 18): each species from its own seed, its body as parts (animal_body.h) in its world's style, no two of a world alike; sixteen plans, several
// swimmers a world, floaters in a thick air. J-07: each species' way of living and how many live together; no generated names (the explorer
// names them: an unnamed one goes by its words, `Bestiary::words`)
#pragma once
#include "galaxy/planetmap.h"
#include "galaxy/made.h"
#include "animal_body.h"
#include <string>
#include <vector>

enum Temperament { TEMP_SHY = 0, TEMP_CURIOUS, TEMP_INDIFFERENT };
enum Activity { ACT_DAY = 0, ACT_NIGHT, ACT_DUSK };
enum CallKind { CALL_NONE = 0, CALL_CHIRP, CALL_LOW, CALL_HOOT, CALL_CLICK };
extern const char* const ACTIVITY_NAMES[3];
extern const char* const TEMPERAMENT_NAMES[3];
// J-07: how a species lives. On land it grazes, browses up in the trees, hunts another species' herds (stalking, then chasing), trails the
// hunters (a scavenger), drops out of sight into a burrow or climbs a trunk. In the air it flies in a flock, in a swarm low over the
// ground, or circles high (a soarer). In the water it swims in a shoal, in the shallows by the shore, or leaps out. A floater drifts on
// the wind
enum AnimalWay { WAY_GRAZER = 0, WAY_BROWSER, WAY_HUNTER, WAY_SCAVENGER, WAY_BURROWER, WAY_CLIMBER, WAY_FLOCK, WAY_SWARM, WAY_SOARER, WAY_SHOAL,
                 WAY_SHALLOWS, WAY_LEAPER, WAY_DRIFTER, WAY_COUNT };
extern const char* const ANIMAL_WAY_NAMES[WAY_COUNT];
// J-07: how many live together: a herd, one alone, or one or two holding their ground (they face the explorer, call and charge)
enum Social { SOCIAL_HERD = 0, SOCIAL_SOLITARY, SOCIAL_TERRITORIAL, SOCIAL_COUNT };
extern const char* const SOCIAL_NAMES[SOCIAL_COUNT];

struct Species {
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
    int way = WAY_GRAZER;            // J-07: how it lives (`AnimalWay`)
    int social = SOCIAL_HERD;        // J-07: a herd, alone, or alone on its ground (`Social`)
    int prey = -1;                   // J-07: a hunter's prey, a land species of its world; -1 none
};

// J-08: what a seeding makes of a species' young (IDEAS-seeded-worlds.md section 8's list): wasting and wounds (ribs showing, a melting hide, open
// sores, bare patches, flayed, a bloated body), growths (tumours, stalks, stone plates, overgrown tusks, horn growths), the body (an extra leg, a
// leg missing, two heads, extra eyes, none, a twisted spine, a joined pair, short legs, long legs, a long neck, an extra tail), the size (a
// giant, a dwarf), the hide (a new colour, pale, dark, glowing spots, glowing stripes, a new pattern, iridescent, woolly, hairless,
// see-through), the ways (calm, bold, walks at night, circles, stumbles, climbs, swarms, limps)
enum AnimalChange { CHG_RIBS = 0, CHG_MELTING, CHG_SORES, CHG_BARE, CHG_FLAYED, CHG_BLOATED, CHG_TUMOURS, CHG_STALKS, CHG_STONE, CHG_TUSKS, CHG_HORNS,
                    CHG_EXTRA_LEG, CHG_MISSING_LEG, CHG_TWO_HEADS, CHG_EXTRA_EYES, CHG_NO_EYES, CHG_TWISTED, CHG_JOINED, CHG_SHORT_LEGS, CHG_LONG_LEGS,
                    CHG_LONG_NECK, CHG_EXTRA_TAIL, CHG_GIANT, CHG_DWARF, CHG_NEW_COLOUR, CHG_PALE, CHG_DARK, CHG_GLOW_SPOTS, CHG_GLOW_STRIPES,
                    CHG_NEW_PATTERN, CHG_IRIDESCENT, CHG_WOOLLY, CHG_HAIRLESS, CHG_SEE_THROUGH, CHG_CALM, CHG_BOLD, CHG_NIGHT, CHG_CIRCLES, CHG_STUMBLES,
                    CHG_CLIMBS, CHG_SWARMS, CHG_LIMPS, CHG_COUNT };
extern const char* const CHANGE_NAMES[CHG_COUNT];
// J-08: a species' fate where it lives (a herd's place, a flock's, a swimmer's): the share of it that died and what killed it; the sickness its
// survivors show and the share of them (land animals'); the share of its young born changed (a herd's) and how (one to three changes, the
// species' own on its world: `animalChangesOf`); how much it thrives; a dyed hide's dye
struct AnimalFate {
    double dead = 0;
    int death = 0;
    int sickness = 0; double sick = 0;
    double changed = 0;
    int changes[3] = {-1, -1, -1}, nChanges = 0;
    double thrive = 0;
    int dye = 0;
    int age = 0;   // the steps since its fate came where it lives (the remains' age there)
    bool gone() const { return dead >= 0.999; }
    bool on() const { return dead > 0 || sick > 0 || changed > 0 || thrive > 0; }
    bool has(int ch) const { for (int i = 0; i < nChanges; i++) if (changes[i] == ch) return true; return false; }
};

// J-07: the metres an animal goes in a cycle of its gait (scale `k` of its species', gravity `g`): a stride of 2.3 hip heights times
// the Froude number's 0.3th power (Alexander's), a third of its legs to four and a half; a legless one's by its body (a serpent's a wave)
double speciesStride(const Species& sp, double k, double speed, double g);
// J-07: its gait at a speed: its walk, a four-legged runner's trot past a Froude number of 0.4, its run's gait past 2.5 or when it runs
int speciesGait(const Species& sp, double k, double speed, double g, bool running);

// J-08: whether a species bears a shell (a shell's or plates' back, a plated hide): a shell rotter's
bool speciesShelled(const Species& sp);
// J-08: a species' fate under a point's (`madeFateAt`): its class's harm by how it resists (a hash of its seed: a middling one loses the harm's
// share, the hardiest half of it, the frailest half again as much; past 0.95 none is spared), a shell rotter's on the shelled, the food lost on
// the plant eaters (half on the hunters, whose prey thins), the sickness on the survivors, the changed young's share of a herd and its changes
// (`changes`: the species' own, from the world's steering), the thriving. B-469: in the air and the water only where the line meets them (its
// class harmed, or a host's glow or dye on the skin); no sprouting there
AnimalFate animalFateOf(const Species& sp, const MadeFate& f, const int* changes, int nChanges);
// J-08: a species' changes on its world: one to three, the first steered by the lines' genes (`MadeSteer` bits) where one fits its body, the rest
// by a hash of the species, none clashing (a giant never a dwarf) and each one its body takes (no extra leg on a serpent). B-469: a flyer's and
// a swimmer's too, of the body and the hide alone (no ways; a swimmer no legs, no wool)
int animalChangesOf(const Species& sp, int steer, int out[3]);
// J-08: the body its changed young are born with: its own with the changes that shape a body (no eyes, more eyes, legs short or long, a long
// neck, overgrown tusks, a new pattern, woolly, hairless, bloated, a twisted spine, two heads, an extra tail, a joined pair, a leg more or less)
AnimalBody changedBody(const AnimalBody& b, const int* changes, int nChanges);
// J-08: whether a change shows on the hide or the body as drawn (`AnimalView::marks`), rather than in the body's parts or the ways
bool changeDrawn(int ch);

struct Bestiary {
    std::vector<Species> species;    // land species first, then flyers, then swimmers
    int landCount = 0, flyerCount = 0, swimmerCount = 0;
    AnimalStyle style;               // J-06: what the world's animals share (its two hide colours among it)
    static Bestiary make(const Body& b, const BodyGen& g);
    // a land species for a biome: one whose habitat matches or is unrestricted; -1 when the world has none
    int pickLand(uint64_t h, int biome) const;
    int firstFlyer() const { return landCount < (int)species.size() && flyerCount ? landCount : -1; }
    int firstSwimmer() const { return swimmerCount ? landCount + flyerCount : -1; }
    // J-07: an unnamed species' words: its kind ("QUADRUPED", a swimmer's "EEL"), with the first thing about its look that none of its
    // world's others of that kind share ("STRIPED QUADRUPED"), so no two of a world read alike
    std::string words(int i) const;
};
