// J-06 (R-448): an animal's body as parts, drawn from its species' own seed: a trunk in segments with a girth along them, the legs with their
// joints and feet, the neck and the head, the tail and the back, the wings, the hide and its coat, the size. Sixteen plans, each a recipe for
// the parts (`bodyOf`), never its own drawing code. A world's animals share a style (`AnimalStyle`: eyes, joints, feet, hide, colours), so
// they look related; `animalDistance` under ANIMAL_ALIKE says two look alike. The rig a body makes and its poses are animal_mesh.h
#pragma once
#include "core/types.h"
#include <cstdint>
#include <string>

// the plans (the first eight are generation 17's and keep their numbers)
enum BodyPlan { PLAN_QUADRUPED = 0, PLAN_LONGNECK, PLAN_HEXAPOD, PLAN_HOPPER, PLAN_BIPED, PLAN_GIANT, PLAN_FLYER, PLAN_SWIMMER, PLAN_OCTOPOD,
                PLAN_SERPENT, PLAN_CRAWLER, PLAN_SLUG, PLAN_BURROWER, PLAN_CLIMBER, PLAN_GLIDER, PLAN_FLOATER, PLAN_COUNT };
extern const char* const PLAN_NAMES[PLAN_COUNT];
// what lives where: on the ground in herds, in the air in flocks, in the water
inline bool planFlies(int p) { return p == PLAN_FLYER || p == PLAN_GLIDER || p == PLAN_FLOATER; }
inline bool planSwims(int p) { return p == PLAN_SWIMMER; }

enum Girth { GIRTH_EVEN = 0, GIRTH_CHEST, GIRTH_HIPS, GIRTH_WAIST, GIRTH_BEADS, GIRTH_TAPER, GIRTH_ROUND, GIRTH_COUNT };
enum Stance { STANCE_UPRIGHT = 0, STANCE_SPRAWL, STANCE_COLUMN, STANCE_COUNT };
enum FootKind { FOOT_HOOF = 0, FOOT_PAD, FOOT_CLAW, FOOT_SPIKE, FOOT_FLAT, FOOT_FLIPPER, FOOT_COUNT };
enum HeadKind { HEAD_SNOUT = 0, HEAD_BLUNT, HEAD_BEAK, HEAD_TRUNK, HEAD_MANDIBLES, HEAD_SHOVEL, HEAD_COUNT };
enum EyeLayout { EYES_PAIR = 0, EYES_ROW, EYES_RING, EYES_STALKS, EYES_COUNT };
enum HeadOrnament { ORN_NONE = 0, ORN_JAWS, ORN_TUSKS, ORN_HORNS, ORN_CURVED, ORN_SPIRAL, ORN_ANTLERS, ORN_NOSE_HORN, ORN_EARS, ORN_FRILL, ORN_FEELERS,
                ORN_COUNT };
enum TailKind { TAIL_NONE = 0, TAIL_WHIP, TAIL_THICK, TAIL_CLUB, TAIL_FAN, TAIL_FIN, TAIL_STINGER, TAIL_CURL, TAIL_COUNT };
enum BackKind { DORSAL_NONE = 0, DORSAL_PLATES, DORSAL_SPINES, DORSAL_SAIL, DORSAL_SHELL, DORSAL_HUMPS, DORSAL_FIN, DORSAL_COUNT };
enum WingKind { WINGS_NONE = 0, WINGS_FEATHERS, WINGS_MEMBRANE, WINGS_INSECT, WINGS_SKIN, WINGS_COUNT };
enum HideKind { HIDE_FUR = 0, HIDE_SCALES, HIDE_PLATES, HIDE_WET, HIDE_WARTS, HIDE_QUILLS, HIDE_HAIR, HIDE_COUNT };
enum CoatKind { COAT_PLAIN = 0, COAT_STRIPES, COAT_SPOTS, COAT_ROSETTES, COAT_BANDS, COAT_SADDLE, COAT_EYESPOTS, COAT_COUNTERSHADE, COAT_MOTTLED,
                COAT_BACKSTRIPE, COAT_COUNT };
enum SwimForm { SWIM_FISH = 0, SWIM_EEL, SWIM_RAY, SWIM_LONGNECK, SWIM_COUNT };
// how a body moves (the first six are generation 17's): a leg's place in its cycle, a serpent's and a swimmer's wave, the wings'
enum Gait { GAIT_WALK = 0, GAIT_TROT, GAIT_GALLOP, GAIT_HOP, GAIT_CRAWL, GAIT_STRIDE, GAIT_WAVE, GAIT_SLITHER, GAIT_BOUND, GAIT_FLAP, GAIT_GLIDE,
            GAIT_SWIM, GAIT_DRIFT, GAIT_COUNT };
extern const char* const GAIT_NAMES[GAIT_COUNT];
extern const char* const GIRTH_NAMES[GIRTH_COUNT];
extern const char* const STANCE_NAMES[STANCE_COUNT];
extern const char* const FOOT_NAMES[FOOT_COUNT];
extern const char* const HEAD_NAMES[HEAD_COUNT];
extern const char* const ORNAMENT_NAMES[ORN_COUNT];
extern const char* const TAIL_NAMES[TAIL_COUNT];
extern const char* const DORSAL_NAMES[DORSAL_COUNT];
extern const char* const WING_NAMES[WINGS_COUNT];
extern const char* const HIDE_NAMES[HIDE_COUNT];
extern const char* const HIDE_COAT_NAMES[COAT_COUNT];
extern const char* const SWIM_NAMES[SWIM_COUNT];

// a body's parts in metres. Body space: forward along the trunk, up, and the side; the trunk's middle over the ground. `size` is the top of
// the back over the ground standing (a swimmer's, a flyer's and a floater's: the body's length)
struct AnimalBody {
    int plan = PLAN_QUADRUPED;
    int variant = 0;               // a swimmer's form (`SwimForm`)
    double size = 1;
    // the trunk: `segs` segments (segs + 1 rings) along `length`, the widest ring `width` across and `depth` high, its girth by `girth`
    int segs = 3, girth = GIRTH_EVEN;
    double length = 1.6, width = 0.45, depth = 0.5;
    double pitch = 0;              // the trunk tilted up toward the front (radians): a strider's, a hopper's
    // the legs: `pairs` pairs that walk, `joints` bones each, the knee bent forward or back; their stance, feet, length along the bones and thickness
    int pairs = 2, joints = 2;
    bool kneeBack = false;
    int stance = STANCE_UPRIGHT, foot = FOOT_PAD;
    double leg = 0.8, legThick = 0.1;
    double arms = 0;               // a pair of arms off the ground (a strider's, a hopper's, a climber's): their length; 0 none
    // the neck and the head
    double neck = 0.5, neckThick = 0.2, neckUp = 0.4;   // length, thickness, rise (radians)
    bool arched = false;
    int head = HEAD_SNOUT;
    double headLen = 0.35, headW = 0.2, headH = 0.2;
    int eyes = 2, eyeLayout = EYES_PAIR;
    int ornament = ORN_NONE;
    double ornamentLen = 0.3;
    // the tail and the back
    int tail = TAIL_WHIP;
    double tailLen = 0.6, tailThick = 0.1;
    int dorsal = DORSAL_NONE;
    double dorsalH = 0;
    // the wings (tip to tip)
    int wings = WINGS_NONE;
    double span = 0;
    // the hide: its kind and coat, each in one of the world's two hide banks (0, 1) at a brightness (0..1)
    int hide = HIDE_FUR, coat = COAT_PLAIN;
    int bank = 0, coatBank = 1;
    double tone = 0.7, coatTone = 0.45;
    uint64_t seed = 0;             // where the coat's spots fall
    // J-08: a changed young's (bestiary.h's `changedBody`): a second head beside the first, an extra tail, a twin joined at its side, a leg more
    // hanging from its flank, a leg gone (its place among the legs drawn; -1 none), the spine twisted (a sideways S and a roll along it, radians)
    int heads = 1;
    bool extraTail = false, joined = false, extraLeg = false;
    int missingLeg = -1;
    double twist = 0;
};

// what a world's animals share, so they look related and each world looks its own: the eyes, the joints and the feet, the hide and its
// coat, the head, its ornament and the tail most species keep (`keep` of them), and the two hide banks' colours
struct AnimalStyle {
    int eyes = 2, eyeLayout = EYES_PAIR, joints = 2, foot = FOOT_PAD, hide = HIDE_FUR, coat = COAT_PLAIN, head = HEAD_SNOUT, ornament = ORN_NONE,
        tail = TAIL_WHIP, dorsal = DORSAL_NONE, wings = WINGS_FEATHERS;
    bool kneeBack = false;
    double keep = 0.7;
    double legs = 1, necks = 1, stocky = 1;   // the world's leg length, neck length and width against a plan's own
    RGB colour[2];                            // the hide banks' colours
    double tone = 0.7;
};
// a world's style: from its seed, its kind's fauna (`FaunaStyle`: the cold's fur and white coats) and whether it has seas
AnimalStyle animalStyleOf(uint64_t worldSeed, int fauna);
// a species' body: its plan's recipe drawn from its seed (`nudge` draws it again), the world's style kept where it fits; `size` its shoulder
// height (a swimmer's or a flyer's length), a giant's when the plan is
AnimalBody bodyOf(uint64_t seed, int plan, double size, const AnimalStyle& style, int nudge);
// two bodies measured as the microbes' forms are (the plan first, then each part, colours by the hide banks and tones): under ANIMAL_ALIKE they
// look alike; `shared` (when given) names the parts they share
double animalDistance(const AnimalBody& a, const AnimalBody& b, std::string* shared = nullptr);
constexpr double ANIMAL_ALIKE = 2.5;
// "QUADRUPED, 4 SEGMENTS, 2 PAIRS OF LEGS, PADS, ..." (the plates' and `bestiary`'s line)
std::string animalWords(const AnimalBody& a);
