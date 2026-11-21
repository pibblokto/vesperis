#include "animal_body.h"
#include "galaxy/kinds.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>

const char* const PLAN_NAMES[PLAN_COUNT] = {"QUADRUPED", "LONG-NECKED BROWSER", "HEXAPOD", "HOPPER", "BIPED STRIDER", "GIANT WALKER", "FLYER",
                                            "SWIMMER", "OCTOPOD", "SERPENT", "MANY-LEGGED CRAWLER", "SLUG", "BURROWER", "CLIMBER", "GLIDER", "FLOATER"};
const char* const GIRTH_NAMES[GIRTH_COUNT] = {"EVEN", "DEEP CHEST", "HEAVY HIPS", "WAISTED", "BEADED", "TAPERING", "ROUND"};
const char* const STANCE_NAMES[STANCE_COUNT] = {"UPRIGHT", "SPRAWLING", "COLUMNS"};
const char* const FOOT_NAMES[FOOT_COUNT] = {"HOOVES", "PADS", "CLAWS", "SPIKES", "FLAT FEET", "FLIPPERS"};
const char* const HEAD_NAMES[HEAD_COUNT] = {"LONG SNOUT", "BLUNT FACE", "BEAK", "TRUNK", "MANDIBLES", "SHOVEL"};
const char* const ORNAMENT_NAMES[ORN_COUNT] = {"NONE", "JAWS", "TUSKS", "HORNS", "CURVED HORNS", "SPIRAL HORNS", "ANTLERS", "NOSE HORN", "EARS",
                                               "FRILL", "FEELERS"};
const char* const TAIL_NAMES[TAIL_COUNT] = {"NONE", "WHIP", "THICK", "CLUB", "FAN", "FIN", "STINGER", "CURL"};
const char* const DORSAL_NAMES[DORSAL_COUNT] = {"NONE", "PLATES", "SPINES", "SAIL", "SHELL", "HUMPS", "FIN"};
const char* const WING_NAMES[WINGS_COUNT] = {"NONE", "FEATHERS", "MEMBRANE", "INSECT WINGS", "GLIDING SKIN"};
const char* const HIDE_NAMES[HIDE_COUNT] = {"FUR", "SCALES", "PLATES", "BARE AND WET", "WARTS", "QUILLS", "LONG HAIR"};
const char* const HIDE_COAT_NAMES[COAT_COUNT] = {"PLAIN", "STRIPES", "SPOTS", "ROSETTES", "BANDS", "SADDLE", "EYESPOTS", "COUNTERSHADED", "MOTTLED",
                                            "BACK STRIPE"};
const char* const SWIM_NAMES[SWIM_COUNT] = {"FISH", "EEL", "RAY", "LONG NECK"};

namespace {
// a weighted pick over the values a plan allows (weight 0: never), the style's value kept `keep` of the time where it fits the plan (a weight
// of 1 or more: a quadruped takes its world's beak only as its own draw)
template <int N>
int pickStyled(Rng& r, double keep, int styled, const double (&w)[N]) {
    const bool keepIt = r.chance(keep);   // (drawn whether or not the style's value fits: the stream keeps its order)
    if (keepIt && styled >= 0 && styled < N && w[styled] >= 1) return styled;
    return r.pick(w, N);
}

// the hides' colours: natural browns, greys and creams; the cold's whites
constexpr RGB HIDE_COLOURS[] = {RGB(0.62f, 0.48f, 0.32f), RGB(0.46f, 0.33f, 0.22f), RGB(0.30f, 0.22f, 0.16f), RGB(0.64f, 0.36f, 0.20f),
                                RGB(0.74f, 0.60f, 0.36f), RGB(0.52f, 0.50f, 0.46f), RGB(0.36f, 0.38f, 0.40f), RGB(0.80f, 0.76f, 0.66f),
                                RGB(0.44f, 0.46f, 0.30f), RGB(0.20f, 0.19f, 0.18f), RGB(0.86f, 0.86f, 0.84f)};
// a second colour against the first: darker marks on a light hide, light ones on a dark hide, now and then an odd one (a blue grey, a
// teal grey, a purple grey, a deep red, a yellow)
constexpr RGB MARK_DARK[] = {RGB(0.20f, 0.16f, 0.13f), RGB(0.14f, 0.13f, 0.12f), RGB(0.30f, 0.22f, 0.16f), RGB(0.36f, 0.38f, 0.42f), RGB(0.48f, 0.24f, 0.16f)};
constexpr RGB MARK_LIGHT[] = {RGB(0.86f, 0.82f, 0.72f), RGB(0.90f, 0.90f, 0.88f), RGB(0.78f, 0.64f, 0.38f), RGB(0.70f, 0.66f, 0.56f)};
constexpr RGB MARK_ODD[] = {RGB(0.34f, 0.38f, 0.50f), RGB(0.30f, 0.42f, 0.40f), RGB(0.42f, 0.32f, 0.40f), RGB(0.58f, 0.22f, 0.18f), RGB(0.74f, 0.66f, 0.26f)};
float lum(const RGB& c) { return 0.3f * c.r + 0.55f * c.g + 0.15f * c.b; }
}

AnimalStyle animalStyleOf(uint64_t worldSeed, int fauna) {
    Rng r(mix64(worldSeed ^ 0x57A1E18ULL));
    AnimalStyle S;
    const bool cold = fauna == FA_COLD;
    {   const double w[9] = {5, 0.3, 60, 4, 12, 0, 8, 0, 8};   // 0 to 8 eyes: a pair most often
        S.eyes = r.pick(w, 9); }
    {   const double w[EYES_COUNT] = {S.eyes <= 2 ? 1.0 : 0.4, S.eyes > 2 ? 0.4 : 0.0, S.eyes > 2 ? 0.3 : 0.0, 0};
        S.eyeLayout = r.pick(w, EYES_COUNT); }
    {   const double w[3] = {45, 45, 10}; S.joints = 2 + r.pick(w, 3); }
    S.kneeBack = r.chance(0.4);
    {   const double w[FOOT_COUNT] = {3, 3, 3, 1, 1, 0}; S.foot = r.pick(w, FOOT_COUNT); }
    {   const double w[HIDE_COUNT] = {cold ? 7.0 : 4.0, cold ? 1.0 : 3.0, 1.5, cold ? 0.2 : 1.0, 1, 0.5, cold ? 4.0 : 1.0}; S.hide = r.pick(w, HIDE_COUNT); }
    {   const double w[COAT_COUNT] = {3, 1.5, 1.5, 0.6, 0.8, 0.8, 0.4, 2, 1, 1}; S.coat = r.pick(w, COAT_COUNT); }
    {   const double w[HEAD_COUNT] = {4, 3, 1.5, 0.5, 0, 0.6}; S.head = r.pick(w, HEAD_COUNT); }
    {   const double w[ORN_COUNT] = {3, 0.5, 0.6, 1.5, 1, 0.6, 0.8, 0.6, 2, 0.8, 0.4}; S.ornament = r.pick(w, ORN_COUNT); }
    {   const double w[TAIL_COUNT] = {1, 4, 2, 0.6, 0.8, 0, 0.3, 0.6}; S.tail = r.pick(w, TAIL_COUNT); }
    {   const double w[DORSAL_COUNT] = {6, 1, 1, 0.6, 0.4, 1, 0}; S.dorsal = r.pick(w, DORSAL_COUNT); }
    {   const double w[WINGS_COUNT] = {0, 4, 3, 1.5, 0}; S.wings = r.pick(w, WINGS_COUNT); }
    S.keep = r.range(0.55, 0.85);
    S.legs = r.range(0.85, 1.2); S.necks = r.range(0.8, 1.3); S.stocky = r.range(0.85, 1.2);
    {   // the first colour: a cold world's white, cream or grey more often
        const double w[11] = {3, 3, 1.5, 1.5, 2, 2, 1.5, cold ? 3.0 : 1.0, 1.5, 0.6, cold ? 5.0 : 0.3};
        S.colour[0] = HIDE_COLOURS[r.pick(w, 11)];
        const bool light = lum(S.colour[0]) > 0.42f;
        const bool odd = r.chance(0.15);
        const int k = r.irange(5);
        S.colour[1] = odd ? MARK_ODD[k] : (light ? MARK_DARK[k] : MARK_LIGHT[k % 4]);
    }
    S.tone = r.range(0.6, 0.85);
    return S;
}

AnimalBody bodyOf(uint64_t seed, int plan, double size, const AnimalStyle& S, int nudge) {
    Rng r(mix64(seed ^ 0xB0D7ULL ^ (uint64_t)nudge * 0x9E3779B97F4A7C15ULL));
    AnimalBody a;
    a.plan = plan; a.size = size; a.seed = mix64(seed ^ 0xC0A7ULL ^ (uint64_t)nudge);
    const double keep = S.keep;
    // what most plans share: the eyes, the hide and the coat by the style; the banks and tones
    a.eyes = r.chance(keep) ? S.eyes : 2;
    a.eyeLayout = a.eyes > 2 ? S.eyeLayout : EYES_PAIR;
    a.joints = r.chance(keep) ? S.joints : 2 + r.irange(2);
    a.kneeBack = r.chance(keep) ? S.kneeBack : r.chance(0.4);
    a.bank = r.chance(0.6) ? 0 : 1;
    a.coatBank = r.chance(0.7) ? 1 - a.bank : a.bank;
    a.tone = clampd(S.tone + r.sym(0.15), 0.4, 0.95);
    a.coatTone = clampd(a.coatBank == a.bank ? a.tone * r.range(0.45, 0.7) : r.range(0.45, 0.85), 0.2, 0.95);
    const double stocky = S.stocky * r.range(0.9, 1.1), legs = S.legs * r.range(0.9, 1.1), necks = S.necks * r.range(0.85, 1.15);
    {   const double w[HIDE_COUNT] = {4, 3, 1.5, 1, 1, 0.5, 1}; a.hide = pickStyled(r, keep, S.hide, w); }
    {   const double w[COAT_COUNT] = {3, 1.5, 1.5, 0.6, 0.8, 0.8, 0.4, 2, 1, 1}; a.coat = pickStyled(r, keep, S.coat, w); }
    // the recipe: the trunk, the legs, the neck and head, the tail and back, the wings
    auto legsFor = [&](double depthShare) {   // the legs' length along their bones from the hips' height (bones bend a little)
        const double reach = std::max(0.05 * size, size - a.depth * depthShare);
        a.leg = reach * (a.joints >= 3 ? 1.18 : 1.08);
    };
    switch (plan) {
        case PLAN_QUADRUPED: case PLAN_CLIMBER: case PLAN_BURROWER: case PLAN_GIANT: case PLAN_LONGNECK: {
            const bool giant = plan == PLAN_GIANT, browse = plan == PLAN_LONGNECK, climb = plan == PLAN_CLIMBER, dig = plan == PLAN_BURROWER;
            a.segs = giant ? 3 + r.irange(3) : (dig ? 3 + r.irange(2) : 3 + r.irange(3));
            {   const double w[GIRTH_COUNT] = {4, dig ? 1.0 : 3.0, 2, 0, 0, 0, dig ? 4.0 : 0.5}; a.girth = r.pick(w, GIRTH_COUNT); }
            a.length = size * (giant ? r.range(1.1, 1.5) : browse ? r.range(0.8, 1.15) : dig ? r.range(1.4, 2.0) : climb ? r.range(0.8, 1.2) : r.range(0.95, 1.45)) /
                       std::sqrt(stocky);
            a.width = size * (giant ? r.range(0.45, 0.55) : browse ? r.range(0.28, 0.38) : dig ? r.range(0.6, 0.8) : climb ? r.range(0.28, 0.4) : r.range(0.32, 0.5)) * stocky;
            a.depth = size * (giant ? r.range(0.45, 0.55) : browse ? r.range(0.32, 0.42) : dig ? r.range(0.55, 0.7) : climb ? r.range(0.3, 0.42) : r.range(0.36, 0.5));
            a.pitch = climb ? r.range(0.12, 0.35) : (browse ? r.range(0.0, 0.18) : r.range(-0.03, 0.08));
            a.pairs = giant && r.chance(0.3) ? 3 : 2;
            if (giant) a.joints = 2;
            a.stance = giant ? STANCE_COLUMN : (dig ? (r.chance(0.6) ? STANCE_SPRAWL : STANCE_UPRIGHT) : (size > 1.6 && r.chance(0.3) ? STANCE_COLUMN : STANCE_UPRIGHT));
            {   const double w[FOOT_COUNT] = {giant ? 1.0 : 4.0, 3, dig || climb ? 6.0 : 2.0, 0, giant ? 4.0 : 0.5, 0}; a.foot = pickStyled(r, keep, S.foot, w); }
            legsFor(dig ? 0.95 : 0.8);
            a.leg *= dig ? 0.9 : (browse ? 1.0 : legs);
            a.legThick = a.width * (giant ? r.range(0.26, 0.34) : dig ? r.range(0.2, 0.28) : r.range(0.15, 0.24));
            a.neck = size * (giant ? r.range(0.3, 1.0) : browse ? r.range(1.0, 1.8) : dig ? r.range(0.05, 0.18) : climb ? r.range(0.12, 0.3) : r.range(0.3, 0.7)) * necks;
            a.neckThick = a.width * (browse ? r.range(0.32, 0.45) : r.range(0.42, 0.6));
            a.neckUp = browse ? r.range(0.9, 1.25) : (dig ? r.range(0.0, 0.2) : r.range(0.25, 0.7));
            a.arched = r.chance(browse ? 0.5 : 0.3);
            {   const double w[HEAD_COUNT] = {5, 4, climb ? 0.2 : 0.4, giant ? 3.0 : 0.6, 0, dig ? 4.0 : 0.6}; a.head = pickStyled(r, keep, S.head, w); }
            a.headLen = size * (browse ? r.range(0.16, 0.24) : giant ? r.range(0.22, 0.32) : r.range(0.3, 0.46)) * (a.head == HEAD_SNOUT ? 1.2 : 1.0);
            a.headW = a.width * r.range(0.55, 0.75) * (a.head == HEAD_SHOVEL ? 1.3 : 1.0);
            a.headH = a.depth * r.range(0.5, 0.66) * (a.head == HEAD_SHOVEL ? 0.6 : 1.0);
            {   const double w[ORN_COUNT] = {3, 0.3, giant ? 3.0 : 0.6, 1.5, 1, 0.6, browse || dig ? 0.3 : 0.8, giant || dig ? 1.5 : 0.6, climb || dig ? 3.0 : 2.0, 0.6, 0};
                a.ornament = pickStyled(r, keep, S.ornament, w); }
            a.ornamentLen = size * r.range(0.2, 0.5) * (giant ? 0.7 : 1.0);
            {   const double w[TAIL_COUNT] = {dig ? 2.0 : 1.0, 4, 2, giant ? 1.0 : 0.6, 0.6, 0, 0.2, climb ? 4.0 : 0.6}; a.tail = pickStyled(r, keep, S.tail, w); }
            a.tailLen = size * (climb ? r.range(0.8, 1.5) : giant ? r.range(0.3, 0.6) : dig ? r.range(0.1, 0.4) : r.range(0.3, 1.0));
            a.tailThick = a.width * (a.tail == TAIL_THICK || a.tail == TAIL_CLUB ? r.range(0.25, 0.4) : r.range(0.1, 0.18));
            {   const double w[DORSAL_COUNT] = {8, giant ? 2.0 : 0.8, dig ? 2.0 : 0.8, 0.5, dig ? 3.0 : 0.2, giant ? 2.0 : 1.0, 0}; a.dorsal = pickStyled(r, keep, S.dorsal, w); }
            a.dorsalH = size * (a.dorsal == DORSAL_SAIL ? r.range(0.25, 0.5) : r.range(0.08, 0.22));
            break;
        }
        case PLAN_HEXAPOD: case PLAN_OCTOPOD: case PLAN_CRAWLER: {
            const bool oct = plan == PLAN_OCTOPOD, crawl = plan == PLAN_CRAWLER;
            a.segs = oct ? 1 + r.irange(3) : (crawl ? 6 + r.irange(7) : 3 + r.irange(3));
            {   const double w[GIRTH_COUNT] = {crawl ? 3.0 : 1.0, 0, oct ? 0.0 : 0.5, crawl ? 0.0 : 4.0, crawl ? 5.0 : 1.0, 0, oct ? 3.0 : 0.0}; a.girth = r.pick(w, GIRTH_COUNT); }
            a.length = size * (oct ? r.range(1.2, 1.8) : crawl ? r.range(4.0, 9.0) : r.range(2.0, 3.2));
            a.width = size * (oct ? r.range(0.8, 1.2) : crawl ? r.range(0.9, 1.5) : r.range(0.45, 0.7)) * stocky;
            a.depth = size * (oct ? r.range(0.45, 0.6) : crawl ? r.range(0.55, 0.8) : r.range(0.3, 0.45));
            a.pairs = oct ? 4 : (crawl ? 4 + r.irange(3) : 3);
            a.joints = crawl ? 2 + r.irange(2) : (r.chance(keep) ? std::max(3, S.joints) : 3 + r.irange(2));
            a.stance = crawl || oct || r.chance(0.8) ? STANCE_SPRAWL : STANCE_UPRIGHT;
            a.foot = r.chance(0.7) ? FOOT_SPIKE : FOOT_CLAW;
            a.leg = size * (oct ? r.range(1.6, 2.4) : crawl ? r.range(0.8, 1.3) : r.range(1.0, 1.6)) * legs;
            a.legThick = a.width * (oct ? r.range(0.05, 0.09) : r.range(0.07, 0.13));
            a.neck = crawl || oct ? 0 : size * r.range(0.0, 0.18);
            a.neckThick = a.width * 0.4; a.neckUp = r.range(0.0, 0.3);
            {   const double w[HEAD_COUNT] = {1, 3, 0.3, 0, 4, 0.5}; a.head = r.pick(w, HEAD_COUNT); }
            a.headLen = size * (oct ? r.range(0.25, 0.4) : r.range(0.35, 0.6)); a.headW = a.width * r.range(0.5, 0.8); a.headH = a.depth * r.range(0.5, 0.7);
            if (a.eyes < 2 || r.chance(0.4)) { a.eyes = oct ? 4 + 2 * r.irange(3) : 2 + 2 * r.irange(3); a.eyeLayout = a.eyes > 2 ? (r.chance(0.5) ? EYES_ROW : EYES_RING) : EYES_PAIR; }
            {   const double w[ORN_COUNT] = {2, oct ? 2.0 : 0.5, 0, 0.6, 0.3, 0, 0, 0.3, 0, 0, oct ? 1.0 : 5.0}; a.ornament = r.pick(w, ORN_COUNT); }
            a.ornamentLen = size * r.range(0.6, 1.4);
            {   const double w[TAIL_COUNT] = {oct ? 10.0 : 3.0, crawl ? 1.0 : 0.5, 0, 0, 0, 0, 1.5, 0}; a.tail = r.pick(w, TAIL_COUNT); }
            a.tailLen = size * r.range(0.4, 1.0); a.tailThick = a.width * r.range(0.1, 0.2);
            {   const double w[DORSAL_COUNT] = {5, crawl ? 1.5 : 0.5, 1, 0, oct ? 0.0 : 1.5, 0, 0}; a.dorsal = r.pick(w, DORSAL_COUNT); }
            a.dorsalH = size * r.range(0.15, 0.4);
            {   const double w[HIDE_COUNT] = {0.3, 2, 5, 1.5, 0.5, 0.5, oct ? 2.0 : 0.3}; a.hide = pickStyled(r, keep, S.hide, w); }
            break;
        }
        case PLAN_BIPED: case PLAN_HOPPER: {
            const bool hop = plan == PLAN_HOPPER;
            a.segs = 3 + r.irange(2);
            {   const double w[GIRTH_COUNT] = {2, hop ? 0.5 : 3.0, hop ? 5.0 : 1.0, 0, 0, 0, 0.5}; a.girth = r.pick(w, GIRTH_COUNT); }
            a.length = size * (hop ? r.range(0.7, 1.0) : r.range(0.6, 0.9)) / std::sqrt(stocky);
            a.width = size * (hop ? r.range(0.3, 0.42) : r.range(0.26, 0.38)) * stocky;
            a.depth = size * (hop ? r.range(0.32, 0.45) : r.range(0.3, 0.42));
            a.pitch = hop ? r.range(0.45, 0.85) : (r.chance(0.6) ? r.range(-0.05, 0.15) : r.range(0.3, 0.6));
            a.pairs = 1;
            a.joints = hop ? 3 + r.irange(2) : std::max(3, a.joints);
            a.kneeBack = hop ? false : (r.chance(keep) ? S.kneeBack : r.chance(0.7));
            {   const double w[FOOT_COUNT] = {1, 3, 4, 0, hop ? 4.0 : 0.5, 0}; a.foot = pickStyled(r, keep, S.foot, w); }
            legsFor(0.6);
            a.leg *= hop ? 1.15 : legs;
            a.legThick = a.width * r.range(0.18, 0.28);
            a.arms = size * (hop ? r.range(0.22, 0.4) : (r.chance(0.7) ? r.range(0.15, 0.45) : 0.0));
            a.neck = size * (hop ? r.range(0.12, 0.28) : r.range(0.35, 0.9)) * necks;
            a.neckThick = a.width * r.range(0.35, 0.55);
            a.neckUp = hop ? r.range(0.3, 0.6) : r.range(0.55, 1.1);
            a.arched = r.chance(0.4);
            {   const double w[HEAD_COUNT] = {4, 3, hop ? 0.3 : 3.0, 0, 0, 0.3}; a.head = pickStyled(r, keep, S.head, w); }
            a.headLen = size * r.range(0.22, 0.38) * (a.head == HEAD_SNOUT ? 1.2 : 1.0); a.headW = a.width * r.range(0.45, 0.6); a.headH = a.depth * r.range(0.42, 0.58);
            {   const double w[ORN_COUNT] = {3, 0.3, 0, 0.8, 0.6, 0.2, 0.2, 0.3, hop ? 5.0 : 1.0, hop ? 0.3 : 2.0, 0}; a.ornament = pickStyled(r, keep, S.ornament, w); }
            a.ornamentLen = size * r.range(0.15, 0.4);
            {   const double w[TAIL_COUNT] = {0.3, 1.5, hop ? 8.0 : 4.0, 0.6, hop ? 0.0 : 2.0, 0, 0, 0.3}; a.tail = pickStyled(r, keep, S.tail, w); }
            a.tailLen = size * (hop ? r.range(0.8, 1.4) : r.range(0.6, 1.3));
            a.tailThick = a.width * r.range(0.25, 0.4);
            {   const double w[DORSAL_COUNT] = {8, 0.5, 0.8, 0.5, 0, 0.3, 0}; a.dorsal = pickStyled(r, keep, S.dorsal, w); }
            a.dorsalH = size * r.range(0.08, 0.2);
            break;
        }
        case PLAN_SERPENT: case PLAN_SLUG: {
            const bool slug = plan == PLAN_SLUG;
            a.segs = slug ? 4 + r.irange(3) : 8 + r.irange(5);
            a.girth = slug ? (r.chance(0.6) ? GIRTH_TAPER : GIRTH_EVEN) : GIRTH_TAPER;
            a.length = size * (slug ? r.range(3.0, 5.0) : r.range(10.0, 22.0));
            a.width = size * (slug ? r.range(0.8, 1.1) : r.range(0.9, 1.2));
            a.depth = size * (slug ? r.range(0.6, 0.85) : r.range(0.85, 1.0));
            a.pairs = !slug && r.chance(0.15) ? 1 : 0;
            a.stance = STANCE_SPRAWL; a.foot = FOOT_CLAW; a.joints = 2;
            a.leg = size * 0.9; a.legThick = a.width * 0.12;
            a.neck = 0; a.neckThick = a.width; a.neckUp = slug ? 0.2 : r.range(0.0, 0.3);
            {   const double w[HEAD_COUNT] = {slug ? 0.0 : 4.0, 3, 0, 0, 0, slug ? 0.0 : 1.5}; a.head = pickStyled(r, keep, S.head, w); }
            a.headLen = size * (slug ? r.range(0.4, 0.7) : r.range(1.2, 2.2)); a.headW = a.width * r.range(0.8, 1.15); a.headH = a.depth * r.range(0.7, 0.9);
            if (slug) { a.eyes = r.chance(0.7) ? 2 : 4; a.eyeLayout = EYES_STALKS; }
            {   const double w[ORN_COUNT] = {4, 0, 0, slug ? 0.0 : 0.5, 0, 0, 0, 0, 0, slug ? 0.0 : 1.0, slug ? 3.0 : 0.5}; a.ornament = r.pick(w, ORN_COUNT); }
            a.ornamentLen = size * r.range(0.5, 1.2);
            a.tail = slug ? TAIL_NONE : TAIL_WHIP; a.tailLen = slug ? 0 : size * r.range(1.0, 3.0); a.tailThick = a.width * 0.4;
            {   const double w[DORSAL_COUNT] = {slug ? 1.5 : 6.0, 0, slug ? 1.0 : 1.5, 0, slug ? 6.0 : 0.0, 0, slug ? 0.0 : 1.0}; a.dorsal = r.pick(w, DORSAL_COUNT); }
            a.dorsalH = size * (a.dorsal == DORSAL_SHELL ? r.range(0.8, 1.6) : r.range(0.2, 0.5));
            {   const double w[HIDE_COUNT] = {0, slug ? 0.0 : 5.0, 1, slug ? 5.0 : 1.0, slug ? 2.0 : 0.3, 0, 0}; a.hide = pickStyled(r, keep, S.hide, w); }
            break;
        }
        case PLAN_FLYER: case PLAN_GLIDER: {
            const bool glide = plan == PLAN_GLIDER;
            a.segs = 2 + r.irange(2);
            a.girth = r.chance(0.5) ? GIRTH_CHEST : GIRTH_EVEN;
            a.length = size * (glide ? r.range(0.5, 0.65) : r.range(0.38, 0.52));
            a.width = size * (glide ? r.range(0.22, 0.3) : r.range(0.16, 0.24));
            a.depth = size * (glide ? r.range(0.16, 0.24) : r.range(0.16, 0.24));
            {   const double w[WINGS_COUNT] = {0, glide ? 0.0 : 4.0, glide ? 0.0 : 3.0, glide ? 0.0 : (size < 0.4 ? 3.0 : 0.5), glide ? 1.0 : 0.0}; a.wings = pickStyled(r, keep, S.wings, w); }
            a.span = size * (glide ? r.range(1.0, 1.5) : r.range(1.7, 3.2));
            a.pairs = a.wings == WINGS_INSECT ? 3 : (glide ? 2 : 1);
            a.stance = glide || a.wings == WINGS_INSECT ? STANCE_SPRAWL : STANCE_UPRIGHT;
            a.foot = a.wings == WINGS_INSECT ? FOOT_SPIKE : FOOT_CLAW;
            a.joints = 2; a.leg = size * r.range(0.25, 0.45); a.legThick = a.width * 0.14;
            a.neck = size * (a.wings == WINGS_INSECT ? 0.02 : r.range(0.08, 0.3)); a.neckThick = a.width * 0.5; a.neckUp = r.range(0.1, 0.4);
            {   const double w[HEAD_COUNT] = {a.wings == WINGS_MEMBRANE || glide ? 4.0 : 1.0, 2, a.wings == WINGS_FEATHERS ? 6.0 : 0.3, 0, a.wings == WINGS_INSECT ? 5.0 : 0.0, 0};
                a.head = pickStyled(r, keep, S.head, w); }
            a.headLen = size * r.range(0.14, 0.24); a.headW = a.width * r.range(0.5, 0.75); a.headH = a.depth * r.range(0.5, 0.75);
            {   const double w[ORN_COUNT] = {4, 0, 0, 0.4, 0.3, 0, 0, 0, a.wings == WINGS_MEMBRANE || glide ? 2.0 : 0.2, a.wings == WINGS_FEATHERS ? 1.5 : 0.2,
                                          a.wings == WINGS_INSECT ? 4.0 : 0.0};
                a.ornament = r.pick(w, ORN_COUNT); }
            a.ornamentLen = size * r.range(0.15, 0.4);
            {   const double w[TAIL_COUNT] = {a.wings == WINGS_INSECT ? 3.0 : 0.5, a.wings == WINGS_MEMBRANE || glide ? 4.0 : 0.5, 0.3, 0, a.wings == WINGS_FEATHERS ? 5.0 : 0.3, 0,
                                          a.wings == WINGS_INSECT ? 0.5 : 0.0, 0.2};
                a.tail = r.pick(w, TAIL_COUNT); }
            a.tailLen = size * r.range(0.25, 0.7); a.tailThick = a.width * 0.2;
            a.dorsal = DORSAL_NONE;
            {   const double w[HIDE_COUNT] = {a.wings == WINGS_FEATHERS ? 5.0 : 3.0, 1.5, a.wings == WINGS_INSECT ? 4.0 : 0.3, 0.5, 0.3, 0, 0}; a.hide = pickStyled(r, keep, S.hide, w); }
            break;
        }
        case PLAN_FLOATER: {
            a.segs = 4 + r.irange(3);
            a.girth = GIRTH_ROUND;
            a.length = size; a.width = size * r.range(0.6, 0.9); a.depth = size * r.range(0.55, 0.85);
            a.pairs = 2 + r.irange(5);   // the strands that hang from the bag
            a.joints = 3 + r.irange(2); a.stance = STANCE_COLUMN; a.foot = FOOT_SPIKE;
            a.leg = size * r.range(0.6, 2.0); a.legThick = size * 0.03;
            a.neck = 0; a.neckThick = a.width * 0.3; a.neckUp = 0;
            a.head = HEAD_BLUNT; a.headLen = size * r.range(0.1, 0.2); a.headW = a.width * 0.35; a.headH = a.depth * 0.35;
            if (r.chance(0.5)) { a.eyes = 2 * r.irange(5); a.eyeLayout = a.eyes > 2 ? EYES_RING : EYES_PAIR; }
            {   const double w[ORN_COUNT] = {3, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2}; a.ornament = r.pick(w, ORN_COUNT); }
            a.ornamentLen = size * r.range(0.2, 0.5);
            a.tail = r.chance(0.4) ? TAIL_FIN : TAIL_NONE; a.tailLen = size * r.range(0.15, 0.35); a.tailThick = a.width * 0.1;
            {   const double w[DORSAL_COUNT] = {3, 0, 0, 1.5, 0, 0, 1.5}; a.dorsal = r.pick(w, DORSAL_COUNT); }
            a.dorsalH = size * r.range(0.1, 0.3);
            {   const double w[HIDE_COUNT] = {0, 0, 1.5, 4, 2, 0, 0}; a.hide = r.pick(w, HIDE_COUNT); }
            break;
        }
        default: {   // PLAN_SWIMMER: a fish, an eel, a ray or a long neck
            const double w[SWIM_COUNT] = {5, 2, 2, 1.2};
            a.variant = r.pick(w, SWIM_COUNT);
            const int v = a.variant;
            a.segs = v == SWIM_EEL ? 8 + r.irange(3) : 4 + r.irange(2);
            a.girth = v == SWIM_LONGNECK ? GIRTH_EVEN : (v == SWIM_RAY ? GIRTH_ROUND : GIRTH_TAPER);
            a.length = size * (v == SWIM_LONGNECK ? r.range(0.35, 0.45) : v == SWIM_RAY ? r.range(0.5, 0.65) : v == SWIM_EEL ? r.range(0.9, 1.0) : r.range(0.7, 0.85));
            a.width = size * (v == SWIM_RAY ? r.range(0.25, 0.35) : v == SWIM_EEL ? r.range(0.05, 0.08) : v == SWIM_LONGNECK ? r.range(0.16, 0.22) : r.range(0.12, 0.2));
            a.depth = size * (v == SWIM_RAY ? r.range(0.06, 0.1) : v == SWIM_EEL ? r.range(0.06, 0.1) : v == SWIM_LONGNECK ? r.range(0.14, 0.18) : r.range(0.2, 0.32));
            a.pairs = v == SWIM_LONGNECK ? 2 : (v == SWIM_FISH ? 1 : 0);
            a.foot = FOOT_FLIPPER; a.stance = STANCE_SPRAWL; a.joints = 2;
            a.leg = size * (v == SWIM_LONGNECK ? r.range(0.2, 0.3) : r.range(0.1, 0.16)); a.legThick = a.width * 0.2;
            a.wings = v == SWIM_RAY ? WINGS_SKIN : WINGS_NONE; a.span = v == SWIM_RAY ? size * r.range(0.9, 1.5) : 0;
            a.neck = v == SWIM_LONGNECK ? size * r.range(0.35, 0.6) : 0; a.neckThick = a.width * 0.35; a.neckUp = v == SWIM_LONGNECK ? r.range(0.3, 0.8) : 0;
            {   const double ww[HEAD_COUNT] = {3, 3, 0, 0, 0, v == SWIM_RAY ? 0.0 : 1.0}; a.head = r.pick(ww, HEAD_COUNT); }
            a.headLen = size * (v == SWIM_LONGNECK ? r.range(0.06, 0.1) : r.range(0.12, 0.2)); a.headW = a.width * r.range(0.7, 0.95); a.headH = a.depth * r.range(0.7, 0.95);
            {   const double ww[ORN_COUNT] = {4, 1.5, 0, 0.3, 0, 0, 0, v == SWIM_FISH ? 0.6 : 0.0, 0, 0.5, 1}; a.ornament = r.pick(ww, ORN_COUNT); }
            a.ornamentLen = size * r.range(0.08, 0.25);
            a.tail = v == SWIM_RAY ? TAIL_WHIP : (v == SWIM_LONGNECK ? TAIL_THICK : TAIL_FIN);
            a.tailLen = size * (v == SWIM_RAY ? r.range(0.5, 1.0) : v == SWIM_LONGNECK ? r.range(0.12, 0.2) : r.range(0.18, 0.3)); a.tailThick = a.width * 0.3;
            a.dorsal = v == SWIM_FISH || v == SWIM_EEL ? (r.chance(0.8) ? DORSAL_FIN : DORSAL_SPINES) : DORSAL_NONE;
            a.dorsalH = size * r.range(0.06, 0.16);
            {   const double ww[HIDE_COUNT] = {0, 5, 1, 4, 0.3, 0, 0}; a.hide = r.pick(ww, HIDE_COUNT); }
            a.eyes = std::min(a.eyes, 4); if (a.eyes > 2) a.eyeLayout = EYES_PAIR;
            break;
        }
    }
    // a coat that needs segments needs a trunk; long hair hangs off nothing under a hand's height
    if (a.segs < 2 && (a.coat == COAT_BANDS || a.coat == COAT_STRIPES)) a.coat = COAT_SPOTS;
    if (a.hide == HIDE_HAIR && size < 0.3) a.hide = HIDE_FUR;
    if (a.eyeLayout == EYES_STALKS && plan != PLAN_SLUG) a.eyeLayout = EYES_PAIR;
    return a;
}

namespace {
struct Weighed { const char* name; double w; };
constexpr Weighed ANIMAL_WEIGHTS[] = {{"plan", 3.0}, {"form", 1.0}, {"size", 1.2}, {"trunk", 1.0}, {"girth", 0.5}, {"legs", 1.0}, {"joints", 0.3},
                                      {"stance", 0.5}, {"feet", 0.4}, {"neck", 0.8}, {"head", 0.8}, {"eyes", 0.5}, {"ornament", 0.8}, {"tail", 0.7},
                                      {"back", 0.8}, {"wings", 1.0}, {"hide", 0.6}, {"coat", 0.8}, {"colour", 0.8}};
double logApart(double a, double b, double decade) { return std::min(1.0, std::fabs(std::log10(std::max(1e-6, a) / std::max(1e-6, b))) / decade); }
}

double animalDistance(const AnimalBody& a, const AnimalBody& b, std::string* shared) {
    double x[sizeof(ANIMAL_WEIGHTS) / sizeof(ANIMAL_WEIGHTS[0])];
    int k = 0;
    x[k++] = a.plan != b.plan;
    x[k++] = a.plan == b.plan ? (a.variant != b.variant) : 1.0;
    x[k++] = logApart(a.size, b.size, 0.4);
    x[k++] = std::min(1.0, logApart(a.length / a.size, b.length / b.size, 0.3) + logApart(a.width / a.size, b.width / b.size, 0.4) + std::abs(a.segs - b.segs) / 6.0 +
                           std::fabs(a.pitch - b.pitch) / 0.6);
    x[k++] = a.girth != b.girth;
    x[k++] = std::min(1.0, std::abs(a.pairs - b.pairs) / 2.0 + logApart(a.leg / a.size, b.leg / b.size, 0.3) + std::fabs(a.arms - b.arms) / std::max(0.2, a.size));
    x[k++] = a.joints != b.joints || a.kneeBack != b.kneeBack;
    x[k++] = a.stance != b.stance;
    x[k++] = a.foot != b.foot;
    x[k++] = std::min(1.0, std::fabs(a.neck / a.size - b.neck / b.size) / 0.5 + std::fabs(a.neckUp - b.neckUp) / 0.6 + (a.arched != b.arched) * 0.3);
    x[k++] = a.head != b.head ? 1.0 : std::min(1.0, logApart(a.headLen / a.size, b.headLen / b.size, 0.25));
    x[k++] = a.eyes != b.eyes || a.eyeLayout != b.eyeLayout;
    x[k++] = a.ornament != b.ornament;
    x[k++] = a.tail != b.tail ? 1.0 : std::min(1.0, std::fabs(a.tailLen / a.size - b.tailLen / b.size) / 0.6);
    x[k++] = a.dorsal != b.dorsal;
    x[k++] = a.wings != b.wings ? 1.0 : std::min(1.0, logApart(a.span / a.size, b.span / b.size, 0.25));
    x[k++] = a.hide != b.hide;
    x[k++] = a.coat != b.coat;
    x[k++] = std::min(1.0, (a.bank != b.bank) * 0.6 + std::fabs(a.tone - b.tone) / 0.3 + (a.coatBank != b.coatBank) * 0.3);
    double d = 0;
    if (shared) shared->clear();
    for (int i = 0; i < k; i++) {
        d += ANIMAL_WEIGHTS[i].w * x[i];
        if (shared && x[i] < 0.35 && ANIMAL_WEIGHTS[i].w >= 0.6) { if (!shared->empty()) *shared += ", "; *shared += ANIMAL_WEIGHTS[i].name; }
    }
    return d;
}

std::string animalWords(const AnimalBody& a) {
    std::string s = PLAN_NAMES[a.plan];
    if (a.plan == PLAN_SWIMMER) s += std::string(" (") + SWIM_NAMES[a.variant] + ")";
    char buf[96];
    snprintf(buf, sizeof buf, " %.2f M, %d SEGMENTS %s", a.size, a.segs, GIRTH_NAMES[a.girth]); s += buf;
    if (a.pairs) { snprintf(buf, sizeof buf, ", %d PAIRS OF LEGS (%d JOINTS%s, %s, %s)", a.pairs, a.joints, a.kneeBack ? ", KNEES BACK" : "", STANCE_NAMES[a.stance], FOOT_NAMES[a.foot]); s += buf; }
    if (a.arms > 0) s += ", ARMS";
    if (a.neck > 0.05 * a.size) s += a.neck > a.size ? ", LONG NECK" : ", NECK";
    s += std::string(", ") + HEAD_NAMES[a.head];
    snprintf(buf, sizeof buf, ", %d EYES", a.eyes); s += buf;
    if (a.ornament) s += std::string(", ") + ORNAMENT_NAMES[a.ornament];
    if (a.tail) s += std::string(", TAIL ") + TAIL_NAMES[a.tail];
    if (a.dorsal) s += std::string(", BACK ") + DORSAL_NAMES[a.dorsal];
    if (a.wings) s += std::string(", ") + WING_NAMES[a.wings];
    s += std::string(", ") + HIDE_NAMES[a.hide] + ", " + HIDE_COAT_NAMES[a.coat];
    snprintf(buf, sizeof buf, ", BANK %d/%d", a.bank, a.coatBank); s += buf;
    return s;
}
