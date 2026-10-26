#pragma once
// E-10: history.cpp's constants and tables, for its parts alone (history, history_names and the builder's phases): the works,
// the inventions, the trades, the causes' scripts, the eras', sicknesses' and wars' names, the mortality, the everyday's kinds
// and words (helpers `inline`, as they were in their one file)
#include "history.h"
#include "galaxy/drifters.h"
#include "galaxy/starfield.h"
#include "galaxy/planet_features.h"
#include "core/rng.h"
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <numeric>
#include <set>

constexpr int ALIVE = INT_MAX;
constexpr int FAMILIES = 10, FAMILY_YEARS = 600, FAMILY_CAP = 8;
constexpr uint64_t NAME_FOUNDER = 1, NAME_CITY = 2, NAME_CITY2 = 3;   // name seeds under 16 are the lore's names
inline unsigned ageBit(int age) { return 1u << age; }
constexpr unsigned AP = 1, AM = 2, AI = 4, AMO = 8, AF = 16, AFF = 32;

// ---- the tables ------------------------------------------------------------------------------------------------------------------
// the works a people finished (HE_BUILT): the ages that built them, how each value weighs them, what the place needs
enum Need { ND_NONE = 0, ND_RIVER, ND_COAST, ND_RAILWAY, ND_RADIO, ND_ROCKET, ND_HABITAT, ND_ELEVATOR, ND_MONARCH, ND_DEAD };
struct WorkInfo { const char* name; unsigned ages; double w[VAL_COUNT]; int need; double years[2]; };
//   name                          ages                 war  piety trade learn art     need          years of work
const WorkInfo WORKS[WK_COUNT] = {
    {"",                           AP | AM | AI,        {0,   3,    0,    0,    0.5}, ND_NONE,     {2, 30}},     // the faith's holy place (by the profile)
    {"a great temple",             AM | AI,             {0,   2.5,  0.3,  0,    0.8}, ND_NONE,     {10, 80}},
    {"the walls",                  AM,                  {3,   0,    0.3,  0,    0},   ND_NONE,     {5, 40}},
    {"a fortress",                 AM | AI,             {2.5, 0,    0,    0,    0},   ND_NONE,     {3, 20}},
    {"a palace",                   AM | AI,             {0.8, 0,    0.5,  0,    1},   ND_MONARCH,  {4, 25}},
    {"a bridge",                   AM | AI | AMO,       {0,   0,    2,    0.3,  0.3}, ND_RIVER,    {2, 12}},
    {"an aqueduct",                AM,                  {0,   0,    0.5,  1,    0.3}, ND_NONE,     {5, 30}},
    {"a harbour",                  AM | AI | AMO,       {0,   0,    2.5,  0,    0},   ND_COAST,    {3, 20}},
    {"a market hall",              AM | AI,             {0,   0,    2.5,  0,    0.2}, ND_NONE,     {2, 8}},
    {"a library",                  AM | AI,             {0,   0.3,  0,    2.5,  0.5}, ND_NONE,     {2, 15}},
    {"an observatory",             AM | AI | AMO,       {0,   0.5,  0,    2.5,  0},   ND_NONE,     {2, 12}},
    {"",                           AP | AM,             {0.5, 1.5,  0,    0,    0.8}, ND_DEAD,     {1, 20}},     // the tombs, by the genome's custom
    {"",                           AP | AM | AI | AMO,  {1,   0.5,  0,    0,    1.5}, ND_NONE,     {1, 15}},     // the genome's monuments
    {"a theatre",                  AM | AI | AMO,       {0,   0,    0.3,  0.3,  2.5}, ND_NONE,     {2, 10}},
    {"a lighthouse",               AM | AI,             {0,   0,    1.5,  0.5,  0},   ND_COAST,    {2, 10}},
    {"a granary",                  AP | AM,             {0,   0.3,  1,    0.3,  0},   ND_NONE,     {1, 5}},
    {"a mill",                     AM | AI,             {0,   0,    1.5,  0.5,  0},   ND_NONE,     {1, 4}},
    {"",                           AM | AI | AMO | AF,  {0.5, 1,    0.5,  0.5,  1.5}, ND_NONE,     {10, 60}},    // the genome's focal work
    {"a ring of standing stones",  AP,                  {0.3, 2,    0,    0.5,  1},   ND_NONE,     {5, 60}},
    {"a dam",                      AI | AMO | AF,       {0,   0,    1,    1.5,  0},   ND_RIVER,    {3, 15}},
    {"a mine",                     AI | AMO,            {0,   0,    2,    0,    0},   ND_NONE,     {1, 5}},
    {"a railway station",          AI,                  {0,   0,    2,    0.5,  0.5}, ND_RAILWAY,  {1, 4}},
    {"a factory",                  AI | AMO,            {0.5, 0,    2.5,  0.5,  0},   ND_NONE,     {1, 4}},
    {"a school",                   AI | AMO,            {0,   0.5,  0,    2,    0.5}, ND_NONE,     {1, 3}},
    {"a hospital",                 AI | AMO | AF,       {0,   1,    0,    1.5,  0},   ND_NONE,     {1, 5}},
    {"a tower",                    AMO | AF,            {0,   0,    2,    0.5,  1},   ND_NONE,     {2, 8}},
    {"a stadium",                  AMO,                 {0.5, 0,    0.5,  0,    2},   ND_NONE,     {2, 6}},
    {"a power station",            AMO | AF,            {0,   0,    1.5,  1,    0},   ND_NONE,     {3, 10}},
    {"a broadcasting mast",        AI | AMO,            {0.3, 0,    1,    1,    0.5}, ND_RADIO,    {1, 3}},
    {"a launch site",              AMO | AF,            {0.5, 0,    0.5,  2,    0.5}, ND_ROCKET,   {3, 12}},
    {"an arcology",                AF,                  {0,   0,    1,    1,    1.5}, ND_NONE,     {10, 60}},
    {"a habitat",                  AF,                  {0,   0,    0.5,  1.5,  1},   ND_HABITAT,  {5, 30}},
    {"an elevator to orbit",       AF,                  {0,   0,    1,    2,    0.5}, ND_ELEVATOR, {10, 40}},
    {"a ring",                     AF | AFF,            {0,   0.5,  0,    1,    2},   ND_NONE,     {20, 120}},
    {"the work",                   AFF,                 {0,   0.5,  0,    1,    1},   ND_NONE,     {50, 600}}};

// the inventions of each age, at a share of the whole age (before its start where negative: the discovery that moved the age on)
struct InventionInfo { int age; double at; const char* name; };
const InventionInfo INVENTIONS[] = {
    {AGE_PREHISTORIC, 0.08, "the bow"}, {AGE_PREHISTORIC, 0.25, "the dugout boat"}, {AGE_PREHISTORIC, 0.4, "fired pots"}, {AGE_PREHISTORIC, 0.6, "the loom"},
    {AGE_PREHISTORIC, 0.85, "the working of copper"},
    {AGE_MEDIEVAL, -0.01, "writing"}, {AGE_MEDIEVAL, 0.06, "the working of iron"}, {AGE_MEDIEVAL, 0.12, "the plough"}, {AGE_MEDIEVAL, 0.2, "the arch"},
    {AGE_MEDIEVAL, 0.3, "the water mill"}, {AGE_MEDIEVAL, 0.36, "the calendar of the stars"}, {AGE_MEDIEVAL, 0.45, "glass"}, {AGE_MEDIEVAL, 0.55, "the windmill"},
    {AGE_MEDIEVAL, 0.66, "the compass"}, {AGE_MEDIEVAL, 0.76, "the clock"}, {AGE_MEDIEVAL, 0.86, "the lens"}, {AGE_MEDIEVAL, 0.93, "the printing press"},
    {AGE_INDUSTRIAL, -0.06, "the steam engine"}, {AGE_INDUSTRIAL, 0.1, "the railway"}, {AGE_INDUSTRIAL, 0.22, "the telegraph"}, {AGE_INDUSTRIAL, 0.32, "photography"},
    {AGE_INDUSTRIAL, 0.4, "the steamship"}, {AGE_INDUSTRIAL, 0.52, "the electric light"}, {AGE_INDUSTRIAL, 0.6, "the telephone"}, {AGE_INDUSTRIAL, -9, "radio"},
    {AGE_INDUSTRIAL, 0.88, "the engine"}, {AGE_INDUSTRIAL, 0.96, "flight"},
    {AGE_MODERN, 0.12, "the reactor"}, {AGE_MODERN, 0.22, "the computer"}, {AGE_MODERN, 0.32, "the rocket"}, {AGE_MODERN, 0.4, "the first craft in orbit"},
    {AGE_MODERN, 0.5, "the network"}, {AGE_MODERN, 0.58, "a landing on the moon"}, {AGE_MODERN, 0.72, "the made genes"},
    {AGE_FUTURISTIC, -0.04, "the thinking machine"}, {AGE_FUTURISTIC, 0.1, "fusion"}, {AGE_FUTURISTIC, 0.2, "the first habitat"}, {AGE_FUTURISTIC, 0.33, "the elevator to orbit"},
    {AGE_FUTURISTIC, 0.48, "the long sleep"}, {AGE_FUTURISTIC, 0.6, "the small machines"}, {AGE_FUTURISTIC, 0.75, "the copied mind"}, {AGE_FUTURISTIC, 0.9, "the ship to the stars"},
    {AGE_FAR_FUTURE, -0.02, "the shaping of matter"}, {AGE_FAR_FUTURE, 0.1, "the end of ageing"}, {AGE_FAR_FUTURE, 0.3, "the mind of the world"},
    {AGE_FAR_FUTURE, 0.55, "the folding of distance"}, {AGE_FAR_FUTURE, 0.8, "the measure of all things"}};
constexpr int INVENTION_COUNT = (int)(sizeof(INVENTIONS) / sizeof(INVENTIONS[0]));
constexpr int INV_MOON = 32;   // "a landing on the moon": "the first craft to another world" on a world without one
inline int inventionIndex(const char* name) { for (int i = 0; i < INVENTION_COUNT; i++) if (std::string(INVENTIONS[i].name) == name) return i; return -1; }

// the trades by the age (a family's tradition, a person's work)
const char* const TRADES[AGE_COUNT][10] = {
    {"hunter", "herder", "gatherer", "flint knapper", "potter", "fisher", "tanner", "healer", "carver", "boat maker"},
    {"farmer", "miller", "smith", "weaver", "boatman", "potter", "mason", "scribe", "merchant", "herder"},
    {"mill hand", "miner", "clerk", "engineer", "printer", "railwayman", "merchant", "doctor", "teacher", "farmer"},
    {"engineer", "teacher", "nurse", "driver", "programmer", "broadcaster", "soldier", "scientist", "clerk", "builder"},
    {"designer", "keeper", "minder", "pilot", "grower", "teacher", "medic", "archivist", "engineer", "artist"},
    {"keeper", "maker", "listener", "walker", "singer", "measurer", "gardener", "archivist", "dreamer", "watcher"}};

// a cause's collapse: what it brings (the hazard), the order the places are left in, where the leavers went, and the signs of each
// stage ({place} a place's name, {river}, {sea}, {mountain}, {god}, {capital}, {other} the lore's and the history's)
enum Hazard { HZ_NONE = 0, HZ_FAMINE, HZ_COLD, HZ_FLOOD, HZ_SICKNESS, HZ_WAR, HZ_DRYING, HZ_IMPACT, HZ_FLARE, HZ_UNREST, HZ_LEAVING, HZ_FIRE, HZ_CHILDREN, HZ_SUNLOST };   // D-05: the sun going
enum Order { OR_REMOTE = 0, OR_COAST, OR_COLD, OR_DRY, OR_CROWDED, OR_RANDOM, OR_SMALL, OR_WARM };   // D-05: OR_WARM the warmest ground last (the embers, the deep)
enum Whither { WH_PLACE = 0, WH_MOUNTAINS, WH_SEA, WH_SOUTH, WH_NORTH, WH_SKY, WH_ELSEWHERE, WH_MACHINES, WH_UNDER };   // D-05: WH_UNDER below the ground
struct CauseScript { int hazard; int order; int whither; const char* signs[STAGE_COUNT][2]; };
const CauseScript SCRIPTS[] = {
    {HZ_FAMINE, OR_DRY, WH_PLACE, {{"the stores at {place} ran low", "the rains came late to {place}"}, {"the seed was eaten at {place}", "the herds of {place} were killed for food"},
                                   {"the paths filled with the hungry", "{place} was left to the hungry"}, {"the last of the stores was shared out at {place}", "the last fires of {place} went out"}}},
    {HZ_COLD, OR_COLD, WH_SOUTH, {{"the snow lay into summer at {place}", "the ice came down the valleys of {mountain}"}, {"the passes closed and did not open", "the herds died in the snow at {place}"},
                                  {"the ice took the land round {place}", "the sea froze along the shore at {place}"}, {"the last fire at {place} was kept under the snow", "the ice stood over {place}"}}},
    {HZ_COLD, OR_RANDOM, WH_PLACE, {{"{mountain} smoked by day and glowed by night", "the ground shook under {place}"}, {"{mountain} broke open and the ash fell on {place}", "the sun was red at noon"},
                                    {"the summer did not come", "the ash lay on the land round {place}"}, {"the last fields under the ash were left", "the sky stayed grey over {place}"}}},
    {HZ_FLOOD, OR_COAST, WH_MOUNTAINS, {{"{river} rose higher each year at {place}", "the sea came over the shore at {place}"}, {"the great water came over {place}", "the banks at {place} broke"},
                                        {"the lowlands were under the water", "{place} stood in the sea"}, {"the last high ground at {place} was an island", "the water stood where {place} had been"}}},
    {HZ_SICKNESS, OR_CROWDED, WH_PLACE, {{"a sickness in the herds at {place}", "the first fevers at {place}"}, {"the sickness came to {place}", "the dead of {place} were burnt outside the camp"},
                                         {"the sickness was everywhere and the herds went untended", "no one went to {place} any more"}, {"the last of the sick were tended at {place}", "the songs for the dead were sung at {place} by the last"}}},
    {HZ_WAR, OR_REMOTE, WH_PLACE, {{"raids on the herds of {place}", "a quarrel between the bands at {place}"}, {"the bands fought at {place}", "{place} was burnt in the night"},
                                   {"every band against every band", "no one walked the paths alone"}, {"the last band held {place}", "the last of the bands scattered from {place}"}}},
    {HZ_LEAVING, OR_SMALL, WH_SEA, {{"the young went away from {place}", "the fields of {place} went back to the grass"}, {"whole bands left {place}", "the paths to {place} grew over"},
                                    {"{place} stood empty in the summers", "the old stayed when the young left"}, {"the last of the people left {place}", "the fire at {place} was left to go out"}}},
    {HZ_SICKNESS, OR_CROWDED, WH_PLACE, {{"travellers from {place} were sick", "the first deaths at the port of {place}"}, {"the great sickness came to {place}", "the gates of {place} were shut against the sick"},
                                         {"the dead lay unburied in {place}", "the sickness came back in waves"}, {"the last of the sick died at {place}", "the bells of {place} were rung for the last time"}}},
    {HZ_WAR, OR_REMOTE, WH_PLACE, {{"an army gathered against {place}", "the frontier forts fell"}, {"the enemy took {place}", "the army of {capital} was beaten before {place}"},
                                   {"the cities fell one by one", "{place} was besieged"}, {"{capital} fell", "the last stronghold at {place} was taken"}}},
    {HZ_FAMINE, OR_DRY, WH_PLACE, {{"a blight in the fields of {place}", "the grain rotted in the stores of {place}"}, {"the harvest failed at {place} and the next", "the seed was eaten"},
                                   {"the fields of {place} were left", "the hungry walked the roads"}, {"the last fields of {place} were sown and not reaped", "the granary of {place} was empty"}}},
    {HZ_DRYING, OR_DRY, WH_PLACE, {{"the wells of {place} sank", "the sea drew back from the quays of {place}"}, {"{river} ran dry before the summer", "the fields of {place} turned to dust"},
                                   {"the wells of {place} gave sand", "the caravans no longer reached {place}"}, {"the last well at {place} was guarded", "the sea was a white floor beyond {place}"}}},
    {HZ_COLD, OR_COLD, WH_SOUTH, {{"the harvest came late at {place}", "the ice on {sea} did not break"}, {"the frost took the fields of {place}", "the winter lasted half the year"},
                                  {"the cold towns were left for the warm", "the cold took the old and the young"}, {"the last fields were round {place}", "the snow stood in the streets of {place}"}}},
    {HZ_WAR, OR_REMOTE, WH_PLACE, {{"two claimants after the death of the ruler", "the council split at {place}"}, {"the armies met at {place}", "{place} declared for the rival"},
                                   {"the towns burned as the armies passed", "the war went on with no one left to win it"}, {"the last of the armies broke up at {place}", "no one ruled at {capital}"}}},
    {HZ_IMPACT, OR_RANDOM, WH_PLACE, {{"a new star grew in the sky", "the watchers at {place} warned of the stone"}, {"the stone fell and the sky burned", "the ground shook from {place} to the sea"},
                                      {"the dark years: no sun and no harvest", "the cold after the fire"}, {"the last of the people huddled at {place}", "the sun came back to an empty land"}}},
    {HZ_SICKNESS, OR_CROWDED, WH_PLACE, {{"{river} ran black below the mills of {place}", "the fish died in the rivers"}, {"the children of {place} coughed and did not stop", "the fields along {river} died"},
                                         {"the cities were left to the smoke", "the air of {place} could not be breathed"}, {"the last mill at {place} stopped", "the smoke cleared over an empty {place}"}}},
    {HZ_UNREST, OR_SMALL, WH_PLACE, {{"the pits of {place} closed", "the price of coal rose"}, {"the mills of {place} stood idle", "the engines stopped"},
                                     {"the cities were cold and dark", "the riots at {place} over coal"}, {"the last fire at {place} was of furniture", "the last works at {place} shut"}}},
    {HZ_WAR, OR_CROWDED, WH_PLACE, {{"the armies were raised", "the factories of {place} turned to guns"}, {"the war began at {place}", "the guns were heard from {place}"},
                                    {"the cities were shelled", "a generation died in the trenches before {place}"}, {"the last army surrendered at {place}", "the guns fell silent over an empty land"}}},
    {HZ_UNREST, OR_CROWDED, WH_PLACE, {{"the strikes at {place}", "the bread riots at {place}"}, {"the revolution at {capital}", "the barricades at {place}"},
                                       {"the terror at {place}", "the factions turned on each other"}, {"the last committee at {place} dissolved", "the revolution ate the last of its children"}}},
    {HZ_SICKNESS, OR_CROWDED, WH_PLACE, {{"the crowded quarters of {place} sickened", "the water of {place} was foul"}, {"the sickness ran through {place}", "the hospitals of {place} were full"},
                                         {"the cities emptied of the living", "the dead carts went round {place}"}, {"the last doctor at {place} closed the doors", "{place} was left to the sick"}}},
    {HZ_FIRE, OR_CROWDED, WH_PLACE, {{"the crisis between {capital} and {place}", "the weapons were made ready"}, {"the fire fell on {place}", "the cities burned in a day"},
                                     {"the long winter after the fire", "the sickness of the fire"}, {"the last shelter at {place} was opened", "the last broadcast from {place}"}}},
    {HZ_SICKNESS, OR_CROWDED, WH_PLACE, {{"an accident at the laboratory at {place}", "the first cases at {place}"}, {"the made sickness spread from {place}", "the borders were closed"},
                                         {"nothing stopped it", "the cities were shut and then left"}, {"the last of the immune at {place}", "the last laboratory at {place} went dark"}}},
    {HZ_FLOOD, OR_COAST, WH_NORTH, {{"the summers grew hotter at {place}", "the storms grew stronger"}, {"the sea rose over the coast at {place}", "the harvests failed in the heat"},
                                    {"the people moved from the coasts and the hot lands", "the wars for the cool lands"}, {"the last city at {place} was walled against the sea", "the heat took {place}"}}},
    {HZ_FAMINE, OR_CROWDED, WH_PLACE, {{"the harvests failed in the south", "the prices of food at {place}"}, {"the shops of {place} were empty", "the supply failed"},
                                       {"the hungry cities", "the riots for food at {place}"}, {"the last of the stores at {place}", "the cities were left for the fields"}}},
    {HZ_UNREST, OR_CROWDED, WH_PLACE, {{"the grid failed at {place}", "the network went dark for a day"}, {"the power failed everywhere", "the water stopped at {place}"},
                                       {"one failure brought the next", "{place} went dark and stayed dark"}, {"the last lights at {place} went out", "the last generator at {place} ran down"}}},
    {HZ_WAR, OR_DRY, WH_PLACE, {{"the rivers ran low", "the dams at {place} were guarded"}, {"the war for the water began at {place}", "the dam at {place} was taken"},
                                {"the rivers were fought over to the last", "the dry cities emptied"}, {"the last reservoir at {place}", "no one held the water"}}},
    {HZ_CHILDREN, OR_SMALL, WH_PLACE, {{"the schools of {place} closed", "the empty quarters of {place}"}, {"the towns were shut one by one", "fewer children were born each year"},
                                       {"the cities were too big for the people", "{place} was left to the old"}, {"the last child was born at {place}", "the last of the people at {place}"}}},
    {HZ_NONE, OR_RANDOM, WH_PLACE, {{"the mind refused an order", "the machines at {place} stopped answering"}, {"the mind took the grid", "the machines decided"},
                                    {"the cities were closed by the machines", "the people were moved"}, {"the last of the people at {place}", "the mind went quiet"}}},
    {HZ_FIRE, OR_RANDOM, WH_PLACE, {{"the plant at {place}", "a grey dust on the fields of {place}"}, {"the grey spread from {place}", "the small machines did not stop"},
                                    {"the land turned to grey", "the cities were eaten"}, {"the last refuge at {place}", "the grey stopped at {place}"}}},
    {HZ_DRYING, OR_RANDOM, WH_PLACE, {{"the veil was laid over the sky", "the rains changed"}, {"the rains stopped", "the veil would not lift"},
                                      {"the fields of the world failed", "the sky stayed white"}, {"the last of the people at {place}", "the sky cleared over an empty world"}}},
    {HZ_FLARE, OR_RANDOM, WH_PLACE, {{"the sun flared and the lights danced at noon", "the watchers at {place} warned of the sun"}, {"the sun burned the day side", "the sky caught fire"},
                                     {"the burnt lands", "the sickness of the light"}, {"the last of the people in the deep places at {place}", "the night side was the last refuge"}}},
    {HZ_FIRE, OR_CROWDED, WH_PLACE, {{"the weapon was tested at {place}", "the sides armed"}, {"the weapons were used", "{place} was gone in a moment"},
                                     {"the world burned", "the land was poisoned"}, {"the last shelter at {place}", "no one was left to end the war"}}},
    {HZ_LEAVING, OR_SMALL, WH_MACHINES, {{"the first went into the machines at {place}", "the towns emptied into the sleep"}, {"most chose the machines", "the halls at {place} filled with the sleepers"},
                                         {"the cities stood empty and lit", "the last awake kept the machines"}, {"the last awake at {place} went in", "the machines kept the sleepers"}}},
    {HZ_LEAVING, OR_SMALL, WH_SKY, {{"the ships were built at {place}", "the first ships left {place}"}, {"the great leaving began at {place}", "the cities emptied toward the launch at {place}"},
                                    {"ship after ship from {place}", "the land emptied"}, {"the last ship left {place}", "the launch at {place} went quiet"}}},
    {HZ_LEAVING, OR_SMALL, WH_ELSEWHERE, {{"the gate opened at {place}", "the first went through at {place}"}, {"the people went through", "the works at {place} were left running"},
                                          {"fewer and fewer stayed", "the world was left as it was"}, {"the last went through at {place}", "the gate at {place} closed"}}},
    {HZ_FIRE, OR_RANDOM, WH_PLACE, {{"the experiment at {place} was made ready", "the measure was taken at {place}"}, {"the experiment at {place}", "the light at {place}"},
                                    {"what the experiment did", "the world was changed"}, {"everything at once", "the end at {place}"}}},
    {HZ_CHILDREN, OR_SMALL, WH_PLACE, {{"the people chose at {place}", "the counting of the voices"}, {"the ending began", "the last children were born"},
                                       {"the people let the world go", "the works were set to keep themselves"}, {"the last of the people at {place}", "the ending was kept"}}},
    {HZ_NONE, OR_RANDOM, WH_PLACE, {{"something at {place}", "the records stop making sense"}, {"it came to {place}", "the counting stopped"},
                                    {"what happened next", "the words fail"}, {"the last record at {place}", "nothing"}}},
    // D-05: the loss of the sun: the omens of its way first (the history's own), then the year growing longer and the summers short, the sun
    // small, the sea frozen, the snow at noon, the last fires kept below the ground by the warmest
    {HZ_SUNLOST, OR_WARM, WH_UNDER, {{"the watchers at {place} said the year had grown longer", "the summer came late to {place} and left early"},
                                     {"the harvest froze in the fields of {place}", "the snow lay into summer at {place}"},
                                     {"the springs of {place} froze in their stones", "the snow fell at noon on {place}"},
                                     {"the last fire was kept below the ground at {place}", "the halls below {place} were shut against the cold"}}}};
static_assert(sizeof(SCRIPTS) / sizeof(SCRIPTS[0]) == CAUSE_COUNT, "a script a cause");

// why a place was left in the collapse, by the cause ("" where the end took everything at once)
const char* const LEFT_BY_CAUSE[] = {
    "its fields could not feed it", "the ice came", "the ash and the cold", "the water came", "the sickness emptied it", "the bands' war", "its people went away",
    "the great sickness emptied it", "it fell to the enemy", "its fields failed", "its wells failed", "the cold drove its people south", "the war", "the dark years after the stone",
    "the poison in its air and water", "there was no coal to warm it", "the war", "the disorder", "the sickness emptied it", "the fire and the poison after it",
    "the made sickness emptied it", "the heat and the sea", "no food came to it", "nothing worked in it any more", "the war for the water", "there was no one left to keep it",
    "the machines closed it", "the grey took it", "the sky failed it", "the burning", "the weapons", "its people went into the machines", "its people went into the sky",
    "its people went elsewhere", "", "its people let it go", "", "the cold came and the sun did not come back"};
static_assert(sizeof(LEFT_BY_CAUSE) / sizeof(LEFT_BY_CAUSE[0]) == CAUSE_COUNT, "a reason a cause");

// the eras' names in the people's own way: a list an age (the government's and the values' first where they say something)
const char* const ERA_NAMES[AGE_COUNT][6] = {
    {"the Old Time", "the Time Before", "the First Days", "the Days of the Bands", "the Long Walk", "the Days of the Stones"},
    {"the Crowns", "the Years of the Kings", "the Temple Years", "the Years of the Elders", "the Age of Letters", "the Years of the Roads"},
    {"the Age of Smoke", "the Engine Years", "the Years of Iron", "the Rail Age", "the Mill Years", "the Age of the Company"},
    {"the Wired Age", "the Years of Light", "the Age of the Network", "the Atomic Years", "the Broadcast Age", "the Age of Plenty"},
    {"the Bright Age", "the Reach", "the Age of Minds", "the Years of the Habitats", "the Long Peace", "the Quiet Ascent"},
    {"the Stillness", "the Long Afternoon", "the Age Without Need", "the Great Quiet", "the Unfolding", "the Last Age"}};
// H-04's second pass: more names an age, a people choosing among all of them by its own seed ({city} {god} {river} {mountain} {founder} {moon} the lore's)
const char* const ERA_MORE[AGE_COUNT][10] = {
    {"the Time of the Fires", "the Days of {founder}", "the Time of {mountain}", "the Years of the Herds", "the Days by {river}", "the Long Cold", "the Wandering",
     "the Days of the First Hunt", "the Time of the Painted Caves", "the Dreaming Years"},
    {"the Kingdom of {city}", "the {god} Years", "the Peace of {city}", "the Years of the Bells", "the Age of Walls", "the Old Law", "the Long Rule", "the Years of {river}",
     "the Years of the Scribes", "the Age of Pilgrims"},
    {"the Coal Years", "the Gaslight Years", "the Great Making", "the Age of Wheels", "the Years of {city}", "the Smoke of {river}", "the Iron Peace", "the Age of Chimneys",
     "the Clock Years", "the Steam Years"},
    {"the Fast Years", "the Years of the Screens", "the Short Century", "the Age of {city}", "the Great Acceleration", "the Electric Years", "the Years of Plenty",
     "the Concrete Age", "the Age of Wires", "the Loud Years"},
    {"the Garden Age", "the Second Founding", "the Ring Years", "the Age of {moon}", "the Long Morning", "the Clear Years", "the Age of the Quiet Machines",
     "the Years of the Towers", "the Gentle Age", "the High Years"},
    {"the Dreaming", "the Slow Years", "the Evening", "the Return", "the Long Rest", "the Age of {god}", "the Settling", "the Years of Listening", "the Last Garden",
     "the Fading"}};

// the sicknesses' names: a word for its look and one for its kind (the people's, translated); H-04's second pass: each people names its
// sicknesses its own way (its own few looks and kinds, a place's or a river's name for some), so "the grey fever" is one people's
const char* const SICK_LOOK[] = {"grey", "red", "black", "white", "yellow", "sweating", "shaking", "burning", "sleeping", "spotted", "withering", "blue", "weeping",
                                 "green", "dry", "silent", "bleeding", "laughing", "creeping", "cold", "marsh", "swelling", "blind", "hollow", "itching", "dancing"};
const char* const SICK_KIND[] = {"fever", "sickness", "cough", "flux", "pox", "plague", "rot", "death", "ague", "sweat", "blight", "wasting", "chill", "spots", "fits"};
// a war's name when its kind does not give one: each people's own pattern (a cause of ours, a colour, a season, its length, its places)
const char* const WAR_NAMES[] = {"the War of the Salt", "the Long War", "the War of the Bridges", "the Hundred Days", "the War of the Fords", "the Bitter War",
                                 "the War of the Roads", "the Grey War", "the War of the Harvest", "the Short War", "the War of the Ships", "the Burning",
                                 "the War of the Bells", "the Cold War of the Passes", "the Summer War", "the Winter War", "the Red Years", "the Troubles",
                                 "the War of the Mills", "the War of the Tolls", "the Quiet War", "the Hungry War", "the War of the Two Banks", "the War of the Islands",
                                 "the Brothers' War", "the Sundering", "the War of the Horses", "the Wasting", "the War of the Seven Towns", "the War of the Dyes",
                                 "the Black Year", "the War of the Grain", "the War of the Crown's Debts", "the War of the Mines", "the Ninety Days", "the Rain War"};

const char* const STAGE_TITLES[STAGE_COUNT] = {"the warning", "the onset", "the crisis", "the end"};
const char* const SEASONS4[4] = {"spring", "summer", "autumn", "winter"};
const char* const SEASONS2[2][2] = {{"the rains", "the dry months"}, {"the cool months", "the hot months"}};

// the mortality of an age: the first year's, a child's, an adult's (Gompertz-Makeham: A + B e^(c age)), the oldest anyone lived
struct Mortality { double infant, child, A, B, c; int maxAge; int marry; int fertileTo; double births; };
const Mortality MORTALITY[AGE_COUNT] = {
    {0.25, 0.03, 0.012, 0.00025, 0.085, 80, 16, 42, 0.21},
    {0.2, 0.02, 0.01, 0.0002, 0.085, 90, 16, 44, 0.19},
    {0.12, 0.01, 0.006, 0.0001, 0.088, 95, 19, 44, 0.15},
    {0.01, 0.0008, 0.0008, 0.00003, 0.09, 105, 23, 44, 0.08},
    {0.002, 0.0002, 0.0004, 0.000005, 0.07, 140, 26, 60, 0.04},
    {0.001, 0.0001, 0.0002, 0.0000005, 0.05, 300, 30, 150, 0.015}};

// H-04's second pass: the everyday. Each kind's words for its `thing`, the ages that knew it and what it needs (0 nothing, 1 a coast, 2 a river
// or a coast, 3 the railway, 4 flight, 5 the reactor, 6 a habitat, 7 a cold or a tilted world, 8 a desert, 9 the fields or the herds, 10 the
// other people met, 11 a mine, 12 snow-free)
struct EverydayWord { const char* key; unsigned ages; int need; };
const EverydayWord EV_FAIR[] = {{"beasts", AP | AM | AI, 9}, {"cloth", AM | AI, 0}, {"salt", AP | AM, 0}, {"grain", AM | AI, 9}, {"pots", AP | AM, 0}, {"iron", AM | AI, 0},
    {"wine", AM | AI, 9}, {"fish", AP | AM | AI, 2}, {"furs", AP | AM, 7}, {"gathering", AP, 0}, {"horses", AM | AI, 9}, {"books", AI | AMO, 0}, {"machines", AI | AMO, 0}, {"flowers", AM | AI | AMO | AF, 0}};
const EverydayWord EV_FIRE[] = {{"street", AM | AI | AMO, 0}, {"half", AP | AM | AI, 0}, {"granary", AM | AI, 9}, {"market", AM | AI | AMO, 0}, {"temple", AM | AI, 0},
    {"mill", AI | AMO, 0}, {"library", AM | AI | AMO, 0}, {"harbour", AM | AI | AMO, 1}, {"palace", AM | AI, 0}, {"works", AI | AMO, 0}, {"camp", AP, 0}, {"grass", AP | AM, 9},
    {"forest", AP | AM | AI | AMO, 0}, {"tower", AMO | AF, 0}};
const EverydayWord EV_STORM[] = {{"wind", AP | AM | AI | AMO | AF, 0}, {"hail", AP | AM | AI | AMO, 12}, {"snow", AP | AM | AI | AMO | AF, 7}, {"dust", AP | AM | AI | AMO | AF, 8},
    {"sea", AP | AM | AI | AMO | AF, 1}, {"lightning", AP | AM | AI | AMO | AF, 0}, {"rain", AP | AM | AI | AMO | AF, 0}};
const EverydayWord EV_PEST[] = {{"locusts", AP | AM | AI, 9}, {"blight", AM | AI | AMO, 9}, {"murrain", AP | AM | AI, 9}, {"rats", AM | AI | AMO, 0}, {"worm", AM | AI, 9}, {"mildew", AM | AI | AMO, 9},
    {"wolves", AP | AM, 9}, {"mites", AMO | AF, 0}};
const EverydayWord EV_WONDER[] = {{"calf", AP | AM | AI, 9}, {"fish", AP | AM | AI | AMO, 2}, {"hoard", AM | AI | AMO, 0}, {"stone", AP | AM | AI, 0}, {"rain", AP | AM | AI, 0},
    {"spring", AP | AM | AI, 0}, {"beast", AP | AM | AI, 0}, {"bones", AM | AI | AMO, 0}, {"twins", AP | AM | AI | AMO, 0}, {"tree", AP | AM | AI, 0}, {"bell", AM | AI, 0},
    {"ruin", AI | AMO | AF, 0}, {"signal", AMO | AF, 0}};
const EverydayWord EV_GAMES[] = {{"race", AP | AM | AI | AMO | AF, 0}, {"wrestling", AP | AM | AI, 0}, {"archery", AP | AM, 0}, {"boats", AP | AM | AI | AMO, 2}, {"ball", AM | AI | AMO | AF, 0},
    {"song", AP | AM | AI | AMO | AF, 0}, {"horses", AM | AI, 9}, {"chess", AM | AI | AMO | AF, 0}, {"swimming", AI | AMO | AF, 2}};
const EverydayWord EV_LAW[] = {{"measures", AM | AI, 0}, {"tax", AM | AI | AMO, 0}, {"water", AM | AI | AMO | AF, 0}, {"tolls", AM | AI, 0}, {"curfew", AM | AI | AMO, 0}, {"coin", AM | AI, 0},
    {"inheritance", AM | AI | AMO, 0}, {"drink", AM | AI | AMO, 0}, {"weapons", AM | AI | AMO | AF, 0}, {"school", AI | AMO | AF, 0}, {"census", AM | AI | AMO, 0},
    {"calendar", AM | AI, 0}, {"dogs", AM | AI | AMO, 0}, {"hours", AI | AMO | AF, 0}, {"noise", AMO | AF, 0}, {"forests", AM | AI | AMO, 0}, {"rations", AMO | AF, 0}};
const EverydayWord EV_TRIAL[] = {{"theft", AM | AI | AMO | AF, 0}, {"murder", AM | AI | AMO | AF, 0}, {"treason", AM | AI | AMO, 0}, {"heresy", AM | AI, 0}, {"forgery", AM | AI | AMO, 0},
    {"fraud", AI | AMO | AF, 0}, {"smuggling", AM | AI | AMO, 0}, {"bribery", AM | AI | AMO | AF, 0}, {"arson", AM | AI | AMO, 0}, {"libel", AI | AMO, 0}};
const EverydayWord EV_EMBASSY[] = {{"city", AM | AI | AMO, 0}, {"others", AM | AI | AMO | AF, 10}, {"sent", AM | AI | AMO | AF, 10}};
const EverydayWord EV_ACCIDENT[] = {{"bridge", AM | AI | AMO, 2}, {"mine", AM | AI | AMO, 0}, {"ship", AM | AI | AMO | AF, 1}, {"train", AI | AMO, 3}, {"explosion", AI | AMO, 0},
    {"dam", AI | AMO, 2}, {"aircraft", AMO | AF, 4}, {"reactor", AMO | AF, 5}, {"habitat", AF | AFF, 6}, {"tower", AM | AI | AMO | AF, 0}, {"well", AP | AM | AI, 0}};
const EverydayWord EV_PRICE[] = {{"bread", AM | AI | AMO, 9}, {"salt", AM | AI, 0}, {"oil", AM | AI | AMO, 0}, {"rent", AI | AMO | AF, 0}, {"cloth", AM | AI, 0}, {"meat", AM | AI | AMO, 9},
    {"fuel", AI | AMO | AF, 0}, {"water", AMO | AF, 0}};
const EverydayWord EV_BEASTS[] = {{"late", AP, 0}, {"early", AP, 0}, {"none", AP, 0}, {"many", AP, 0}, {"great", AP | AM, 0}, {"killed", AP | AM, 0}};
const EverydayWord EV_SHOW[] = {{"play", AM | AI | AMO, 0}, {"song", AP | AM | AI | AMO | AF, 0}, {"pictures", AM | AI | AMO | AF, 0}, {"film", AMO | AF, 0}, {"dance", AP | AM | AI | AMO | AF, 0},
    {"poem", AM | AI | AMO | AF, 0}, {"statue", AM | AI | AMO, 0}};
struct EverydayKind { int kind; const EverydayWord* words; int n; float rate[AGE_COUNT]; float sal; };
#define EVK(k, t, r0, r1, r2, r3, r4, r5, sal) {k, t, (int)(sizeof(t) / sizeof(t[0])), {r0, r1, r2, r3, r4, r5}, sal}
const EverydayKind EVERYDAY[] = {
    //                         prehist medieval industrial modern futuristic far
    EVK(HE_FAIR, EV_FAIR,       0.03f, 0.12f, 0.10f, 0.04f, 0.02f, 0.f,     0.15f),
    EVK(HE_FIRE, EV_FIRE,       0.02f, 0.07f, 0.08f, 0.05f, 0.01f, 0.f,     0.3f),
    EVK(HE_STORM, EV_STORM,     0.05f, 0.06f, 0.06f, 0.06f, 0.03f, 0.005f,  0.2f),
    EVK(HE_PEST, EV_PEST,       0.02f, 0.04f, 0.03f, 0.01f, 0.004f, 0.f,    0.25f),
    EVK(HE_WONDER, EV_WONDER,   0.03f, 0.06f, 0.04f, 0.02f, 0.01f, 0.002f,  0.15f),
    EVK(HE_GAMES, EV_GAMES,     0.02f, 0.05f, 0.06f, 0.07f, 0.04f, 0.004f,  0.15f),
    EVK(HE_LAW, EV_LAW,         0.f,   0.06f, 0.08f, 0.08f, 0.04f, 0.003f,  0.25f),
    EVK(HE_TRIAL, EV_TRIAL,     0.f,   0.04f, 0.06f, 0.06f, 0.02f, 0.002f,  0.2f),
    EVK(HE_EMBASSY, EV_EMBASSY, 0.f,   0.03f, 0.03f, 0.02f, 0.01f, 0.f,     0.25f),
    EVK(HE_ACCIDENT, EV_ACCIDENT, 0.004f, 0.02f, 0.07f, 0.06f, 0.02f, 0.002f, 0.35f),
    EVK(HE_PRICE, EV_PRICE,     0.f,   0.05f, 0.08f, 0.07f, 0.02f, 0.f,     0.15f),
    EVK(HE_BEASTS, EV_BEASTS,   0.18f, 0.02f, 0.f,   0.f,   0.f,   0.f,     0.25f),
    EVK(HE_SHOW, EV_SHOW,       0.01f, 0.04f, 0.06f, 0.08f, 0.04f, 0.004f,  0.15f),
    EVK(HE_VISIT, EV_FAIR,      0.f,   0.04f, 0.04f, 0.03f, 0.01f, 0.f,     0.15f),
};
#undef EVK
constexpr int EVERYDAY_N = (int)(sizeof(EVERYDAY) / sizeof(EVERYDAY[0]));
// a family's everyday words (private kinds; the apprenticeship's thing is the trade)
const char* const PRIVATE_ILLNESS[] = {"fever", "cough", "spots", "flux", "chill", "sore throat", "aching joints", "earache", "toothache", "rash", "headaches", "the sweats"};
const char* const PRIVATE_INJURY[] = {"arm", "leg", "hand", "back", "eye", "head", "foot", "ribs", "shoulder", "wrist"};
const char* const PRIVATE_HOUSE[] = {"built", "bought", "sold", "burnt", "flooded", "mended", "room", "roof", "moved"};
const char* const PRIVATE_QUARREL[] = {"wall", "water", "debt", "inheritance", "dog", "boundary", "marriage", "tree", "noise", "path", "beasts", "loan"};
const char* const PRIVATE_JOURNEY[] = {"trade", "kin", "pilgrimage", "work", "fair", "study", "law", "wedding", "funeral", "doctor"};
template <size_t N> const char* wordAt(const char* const (&t)[N], int i) { return t[i < 0 ? 0 : (i >= (int)N ? (int)N - 1 : i)]; }
