// H-02 (2026-10-09): the history (see history.h).
#include "history.h"
#include "starfield.h"
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

const char* const HISTORY_EVENT_NAMES[HE_COUNT] = {"coming", "first", "stones", "founding", "accession", "rule end", "regency", "government", "war", "battle", "siege",
    "sack", "peace", "sickness", "harvest", "famine", "flood", "drought", "quake", "eruption", "impact", "flare", "winter", "built", "way", "festival", "omen",
    "invention", "discovery", "faith", "institution", "deed", "unrest", "migration", "abandoned", "contact", "treaty", "other end", "sign", "stage", "exodus",
    "end", "fair", "fire", "storm", "pest", "wonder", "games", "law", "trial", "embassy", "accident", "price", "beasts", "show", "visit", "birth", "marriage", "move", "death",
    "illness", "injury", "apprentice", "house", "quarrel", "journey", "betrothal"};
const char* const HISTORY_ROLE_NAMES[HR_COUNT] = {"", "ruler", "regent", "founder", "general", "prophet", "maker", "inventor", "explorer", "rebel", "envoy", "artist", "the last"};
const char* const HISTORY_WAR_KINDS[HW_COUNT] = {"a war between bands", "a war between cities", "a rising", "a war of succession", "a revolution", "a civil war",
                                                 "a war with the other people", "a conquest"};
const char* const HISTORY_INSTITUTION_KINDS[HI_COUNT] = {"the council", "the priesthood", "the guilds", "the academy", "the company", "the gazette", "the university",
                                                         "the assembly", "the broadcaster", "the network", "the mind", "the army"};

namespace {
constexpr int ALIVE = INT_MAX;
constexpr int FAMILIES = 10, FAMILY_YEARS = 600, FAMILY_CAP = 8;
constexpr uint64_t NAME_FOUNDER = 1, NAME_CITY = 2, NAME_CITY2 = 3;   // name seeds under 16 are the lore's names
double nowMs() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
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
int inventionIndex(const char* name) { for (int i = 0; i < INVENTION_COUNT; i++) if (std::string(INVENTIONS[i].name) == name) return i; return -1; }

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
enum Hazard { HZ_NONE = 0, HZ_FAMINE, HZ_COLD, HZ_FLOOD, HZ_SICKNESS, HZ_WAR, HZ_DRYING, HZ_IMPACT, HZ_FLARE, HZ_UNREST, HZ_LEAVING, HZ_FIRE, HZ_CHILDREN };
enum Order { OR_REMOTE = 0, OR_COAST, OR_COLD, OR_DRY, OR_CROWDED, OR_RANDOM, OR_SMALL };
enum Whither { WH_PLACE = 0, WH_MOUNTAINS, WH_SEA, WH_SOUTH, WH_NORTH, WH_SKY, WH_ELSEWHERE, WH_MACHINES };
struct CauseScript { int hazard; int order; int whither; const char* signs[STAGE_COUNT][2]; };
const CauseScript SCRIPTS[CAUSE_COUNT] = {
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
                                    {"what happened next", "the words fail"}, {"the last record at {place}", "nothing"}}}};

// why a place was left in the collapse, by the cause ("" where the end took everything at once)
const char* const LEFT_BY_CAUSE[CAUSE_COUNT] = {
    "its fields could not feed it", "the ice came", "the ash and the cold", "the water came", "the sickness emptied it", "the bands' war", "its people went away",
    "the great sickness emptied it", "it fell to the enemy", "its fields failed", "its wells failed", "the cold drove its people south", "the war", "the dark years after the stone",
    "the poison in its air and water", "there was no coal to warm it", "the war", "the disorder", "the sickness emptied it", "the fire and the poison after it",
    "the made sickness emptied it", "the heat and the sea", "no food came to it", "nothing worked in it any more", "the war for the water", "there was no one left to keep it",
    "the machines closed it", "the grey took it", "the sky failed it", "the burning", "the weapons", "its people went into the machines", "its people went into the sky",
    "its people went elsewhere", "", "its people let it go", ""};

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
}   // namespace
int historySeat(const History& H, int year);
namespace {
std::string withCommas(int64_t n) {
    std::string s = std::to_string(n < 0 ? -n : n), out;
    for (size_t i = 0; i < s.size(); i++) { if (i > 0 && (s.size() - i) % 3 == 0) out += ','; out += s[i]; }
    return n < 0 ? "-" + out : out;
}
std::string ordinal(int n) {
    const int m100 = n % 100, m10 = n % 10;
    const char* suf = (m100 >= 11 && m100 <= 13) ? "th" : (m10 == 1 ? "st" : (m10 == 2 ? "nd" : (m10 == 3 ? "rd" : "th")));
    return withCommas(n) + suf;
}
std::string fillText(const History& H, std::string s, int place, int year = 1 << 30) {
    auto rep = [&](const char* key, const std::string& v) { size_t at; while ((at = s.find(key)) != std::string::npos) s.replace(at, std::string(key).size(), v); };
    rep("{place}", place >= 0 ? historyPlaceName(H, place) : historyPlaceName(H, 0));
    rep("{capital}", H.places.empty() ? H.lore.city : historyPlaceName(H, historySeat(H, year)));
    rep("{river}", H.lore.river); rep("{sea}", H.lore.sea); rep("{mountain}", H.lore.mountain); rep("{god}", H.lore.god);
    rep("{other}", H.lore.other.empty() ? std::string("the enemy") : "the " + H.lore.other);
    return s;
}
}   // namespace

std::string historyInventionName(int id) { return id >= 0 && id < INVENTION_COUNT ? INVENTIONS[id].name : "something"; }

// ---- the founding schedule (shared with the other people's meeting: both histories read the same years) -------------------------
namespace {
struct Schedule {
    std::vector<int> founded, age;   // per place of the plan
};
// the eras' indices of the historic ages (the medieval and later; a prehistoric people's one era)
// `scores`: the older ages' scores over the people's territory, filled where asked for and kept (the history's cleared places read the medieval)
Schedule foundingSchedule(const BodyGen& g, const PlanGrid& G, const PeoplePlan& pp, const PeopleProfile& prof, int people, std::vector<std::vector<float>>* scores = nullptr) {
    Schedule S;
    const int n = (int)pp.places.size();
    S.founded.assign(n, 1); S.age.assign(n, prof.age);
    if (n == 0) return S;
    Rng r(prof.seed ^ 0x40FD5ULL);
    std::vector<int> eraOf(n, -1);
    int firstHist = 0;
    while (firstHist + 1 < (int)prof.eras.size() && prof.eras[firstHist].age == AGE_PREHISTORIC && prof.age != AGE_PREHISTORIC) firstHist++;
    const int lastEra = (int)prof.eras.size() - 1;
    if (prof.age <= AGE_MEDIEVAL) for (int i = 0; i < n; i++) eraOf[i] = firstHist;
    else {
        // which places were old: their ground's score for each older age, the best of the age's share founded in it
        std::vector<int> left;
        for (int i = 1; i < n; i++) left.push_back(i);
        // the share of the places each era founded: the old cities, the mill towns, the new
        double share[AGE_COUNT] = {0, 0, 0, 0, 0, 0};
        switch (prof.age) {
            case AGE_INDUSTRIAL: share[AGE_MEDIEVAL] = 0.72; break;
            case AGE_MODERN: share[AGE_MEDIEVAL] = 0.55; share[AGE_INDUSTRIAL] = 0.25 / 0.45; break;
            case AGE_FUTURISTIC: share[AGE_MEDIEVAL] = 0.45; share[AGE_INDUSTRIAL] = 0.2 / 0.55; share[AGE_MODERN] = 0.15 / 0.35; break;
            default: break;   // the far future: its works are its own
        }
        std::vector<std::vector<float>> own(AGE_COUNT);
        std::vector<std::vector<float>>& sc = scores ? *scores : own;
        sc.resize(AGE_COUNT);
        for (int e = firstHist; e <= lastEra; e++) { const int a = prof.eras[e].age; if (a < prof.age && share[a] > 0 && sc[a].empty()) planScores(g, G, people, a, sc[a], &pp.territory); }
        for (int e = firstHist; e < lastEra; e++) {
            const int a = prof.eras[e].age;
            if (share[a] <= 0 || sc[a].empty()) continue;
            std::vector<std::pair<double, int>> k;
            for (int i : left) {
                const PlanPlace& p = pp.places[i];
                if (prof.age == AGE_FUTURISTIC && p.sclass == SC_VILLAGE) continue;   // a futuristic people's lesser places are its own age's
                const double big = (p.sclass == SC_CITY || (prof.age == AGE_FUTURISTIC && p.sclass == SC_TOWN)) ? 0.25 : 0.0;
                k.push_back({sc[a][p.cell] + big + 0.15 * unitFromHash(mix64(p.seed ^ (0x40FDAULL + a))), i});
            }
            std::sort(k.begin(), k.end(), [](const std::pair<double, int>& x, const std::pair<double, int>& y) { return x.first != y.first ? x.first > y.first : x.second < y.second; });
            const int take = (int)std::lround(share[a] * left.size());
            std::vector<int> rest;
            std::vector<char> took(n, 0);
            for (int t = 0; t < take && t < (int)k.size(); t++) { eraOf[k[t].second] = e; took[k[t].second] = 1; }
            for (int i : left) if (!took[i]) rest.push_back(i);
            left.swap(rest);
        }
        int newest = lastEra;
        if (prof.age != AGE_FAR_FUTURE && lastEra > firstHist && prof.collapseYear - prof.eras[lastEra].start < 40) newest = lastEra - 1;
        for (int i : left) eraOf[i] = newest;
        eraOf[0] = prof.age == AGE_FAR_FUTURE ? lastEra : firstHist;
    }
    if (prof.age != AGE_FAR_FUTURE) eraOf[0] = firstHist;
    // the years: in the plan's order within each era, most in its first half (the land filled quickly), none in the collapse
    for (int e = 0; e <= lastEra; e++) {
        std::vector<int> list;
        for (int i = 0; i < n; i++) if (eraOf[i] == e) list.push_back(i);
        if (list.empty()) continue;
        const Era& E = prof.eras[e];
        int start = E.start;
        const bool capHere = eraOf[0] == e && prof.age != AGE_FAR_FUTURE;
        if (capHere) { S.founded[0] = 1; start = std::max(start, 2); list.erase(std::remove(list.begin(), list.end(), 0), list.end()); }
        int stop = E.end;
        if (e == lastEra) stop = std::min(stop, prof.collapseYear - 1);
        const int len = std::max(1, stop - start + 1);
        const double span = (prof.age == AGE_FAR_FUTURE && e == lastEra) ? 0.6 * len : 0.7 * len;
        const int m = (int)list.size();
        std::vector<int> ys(m);
        // slow, then the land filling, then slowing (the inverse of a smoothstep), the first daughter towns a few decades on
        auto invSmooth = [](double u) { double lo = 0, hi = 1; for (int it = 0; it < 30; it++) { const double mid = 0.5 * (lo + hi); if (mid * mid * (3 - 2 * mid) < u) lo = mid; else hi = mid; } return 0.5 * (lo + hi); };
        for (int k = 0; k < m; k++) {
            const double t = 0.04 + 0.96 * invSmooth((k + 0.5) / m) + r.sym(0.015);
            ys[k] = clampi(start + (int)std::floor(span * clampd(t, 0, 1)), start, std::max(start, stop));
        }
        std::sort(ys.begin(), ys.end());
        for (int k = 0; k < m; k++) S.founded[list[k]] = ys[k];
    }
    for (int i = 0; i < n; i++) S.age[i] = prof.eras[std::max(0, eraOf[i])].age;
    if (prof.age == AGE_PREHISTORIC) S.founded[0] = 1;
    return S;
}

// a battle's dead at most, by the age that fought it (an age's armies and weapons: a band's feud to a modern people's war)
constexpr double BATTLE_CAP[AGE_COUNT] = {300, 30000, 200000, 600000, 100000, 10000};
// the other people (C-13): the meeting, the treaties and the wars both histories hold, in years before the world's present
struct JointEvent { int kind = HE_CONTACT; int64_t ago = 0; int place[2] = {-1, -1}; int war = -1; int out = 0; int64_t value = 0; int season = -1; float salience = 0.5f; };
struct JointWar { int kind = HW_OTHER; int64_t from = 0, to = 0; int winner = -1; int64_t dead = 0; uint64_t name = 0; int aggressor = 0; };
struct Joint { std::vector<JointEvent> events; std::vector<JointWar> wars; bool met = false; };
int64_t agoOf(const PeopleProfile& P, int year) { return (int64_t)std::llround(P.endedYearsAgo) + (P.lastYear - year); }
int yearOf(const PeopleProfile& P, int64_t ago) { return P.lastYear - (int)(ago - (int64_t)std::llround(P.endedYearsAgo)); }
// `own` and `ownSchedule` are the asking people's (each history builds its own; the other's are built here), so both histories reach the
// same list
Joint jointOf(const StarSystem& sys, const Body& b, const BodyGen& g, const WorldPlan& WP, int ownPeople, const PeopleProfile& own, const Schedule& ownSchedule) {
    Joint J;
    if (WP.peoples < 2) return J;
    const int other = 1 - ownPeople;
    PeopleProfile P[2];
    P[ownPeople] = own; P[other] = peopleProfileOf(sys, b, g, other);
    const PeoplePlan* pp[2] = {&WP.plans[0], &WP.plans[1]};
    if (pp[0]->places.empty() || pp[1]->places.empty()) return J;
    // the years both lived in: from the later year 1 to the earlier end
    const int64_t from = std::min(agoOf(P[0], 1), agoOf(P[1], 1)), to = std::max(agoOf(P[0], P[0].lastYear), agoOf(P[1], P[1].lastYear));
    if (from <= to) return J;
    // a people the other conquered (its end a conquest or a war, the other going on and alive when its collapse began)
    int conquered = -1;
    for (int k = 0; k < 2; k++) {
        const int c2 = P[k].cause;
        const bool otherOn = agoOf(P[1 - k], P[1 - k].lastYear) < agoOf(P[k], P[k].lastYear) && agoOf(P[1 - k], 1) > agoOf(P[k], P[k].collapseYear);
        if (otherOn && (c2 == CAUSE_CONQUEST || c2 == CAUSE_BAND_WAR || c2 == CAUSE_INDUSTRIAL_WAR)) conquered = k;
    }
    const int64_t conquestFrom = conquered >= 0 ? agoOf(P[conquered], P[conquered].collapseYear) : INT64_MIN;
    // could they meet: lands that touch, or a people that sailed (the medieval age and later) by the time of the meeting
    bool touch = false;
    for (int c = 0; c < WP.grid.cells && !touch; c++) {
        if (!pp[0]->territory[c]) continue;
        int nb[8]; const int k = WP.grid.neighbours(c, nb);
        for (int t = 0; t < k; t++) if (pp[1]->territory[nb[t]]) { touch = true; break; }
    }
    Schedule S[2];
    S[ownPeople] = ownSchedule; S[other] = foundingSchedule(g, WP.grid, *pp[other], P[other], other);
    Rng r(g.seed ^ 0x40C13ULL);
    int64_t meet = from - (int64_t)(touch ? r.range(0, 40) : r.range(30, 300));
    if (!touch) {   // by sea: once one of them sailed (its age at that time the medieval or later)
        int64_t sail = INT64_MIN;
        for (int k = 0; k < 2; k++) for (const Era& e : P[k].eras) if (e.age >= AGE_MEDIEVAL) { sail = std::max(sail, agoOf(P[k], e.start)); break; }
        meet = sail == INT64_MIN ? INT64_MIN : std::min(meet, sail - (int64_t)r.range(50, 400));
    }
    // a conquest needs the conqueror to have come: a meeting a few decades before it at the latest
    if (conquered >= 0 && meet <= conquestFrom) meet = std::min(from, conquestFrom + (int64_t)r.range(10, 80));
    if (meet <= to) return J;
    // the frontier: the pair of places nearest each other, and the next few pairs
    struct Pair { double d; int a, b; };   // d: the negated dot (the nearer the smaller)
    auto nearer = [](const Pair& x, const Pair& y) { return x.d != y.d ? x.d < y.d : (x.a != y.a ? x.a < y.a : x.b < y.b); };
    std::vector<Pair> pairs;   // a heap of the 24 nearest so far, the farthest of them on top
    for (int a = 0; a < (int)pp[0]->places.size(); a++) for (int c = 0; c < (int)pp[1]->places.size(); c++) {
        const Pair q{-dot(pp[0]->places[a].unit, pp[1]->places[c].unit), a, c};
        if (pairs.size() < 24) { pairs.push_back(q); std::push_heap(pairs.begin(), pairs.end(), nearer); }
        else if (nearer(q, pairs.front())) { std::pop_heap(pairs.begin(), pairs.end(), nearer); pairs.back() = q; std::push_heap(pairs.begin(), pairs.end(), nearer); }
    }
    std::sort(pairs.begin(), pairs.end(), nearer);
    auto standing = [&](int k, int place, int64_t ago) { const int y = yearOf(P[k], ago); return y >= S[k].founded[place] && y <= P[k].lastYear; };
    const int nearN = (int)pairs.size();
    auto pairAt = [&](int64_t ago, int salt) {   // one of the nearest pairs that stood then; else the nearest pair that did, kept with them
        for (int t = 0; t < nearN; t++) {
            const int i = (t + salt) % nearN;
            if (standing(0, pairs[i].a, ago) && standing(1, pairs[i].b, ago)) return i;
        }
        Pair best{1e9, -1, -1};
        for (int a = 0; a < (int)pp[0]->places.size(); a++) {
            if (!standing(0, a, ago)) continue;
            for (int c = 0; c < (int)pp[1]->places.size(); c++) {
                if (!standing(1, c, ago)) continue;
                const Pair q{-dot(pp[0]->places[a].unit, pp[1]->places[c].unit), a, c};
                if (nearer(q, best)) best = q;
            }
        }
        if (best.a < 0) return -1;
        for (int i = nearN; i < (int)pairs.size(); i++) if (pairs[i].a == best.a && pairs[i].b == best.b) return i;
        pairs.push_back(best);
        return (int)pairs.size() - 1;
    };
    int pi = pairAt(meet, 0);
    if (pi < 0) { pi = 0; meet = std::min(meet, std::min(agoOf(P[0], S[0].founded[pairs[0].a]), agoOf(P[1], S[1].founded[pairs[0].b])) - 1); if (meet <= to) return J; }
    J.met = true;
    JointEvent c; c.kind = HE_CONTACT; c.ago = meet; c.place[0] = pairs[pi].a; c.place[1] = pairs[pi].b; c.salience = 0.8f;
    J.events.push_back(c);
    // the years after: treaties and wars by what both valued; the one that went on conquered the other where its cause says so
    const double warW = 0.2 + P[0].values[VAL_WAR] + P[1].values[VAL_WAR], tradeW = 0.2 + P[0].values[VAL_TRADE] + P[1].values[VAL_TRADE];
    for (int64_t ago = meet - 1; ago > to; ago--) {
        if (ago <= conquestFrom) break;
        if (r.chance(tradeW / 260.0)) {
            const int q = pairAt(ago, r.irange(4)); if (q < 0) continue;
            JointEvent t; t.kind = HE_TREATY; t.ago = ago; t.place[0] = pairs[q].a; t.place[1] = pairs[q].b; t.out = r.irange(3); t.salience = 0.45f;
            J.events.push_back(t);
        }
        if (r.chance(warW / 160.0)) {
            JointWar w; w.kind = HW_OTHER; w.from = ago; w.to = std::max(to + 1, ago - (int64_t)std::lround(std::exp(r.range(0, std::log(8.0)))) + 1);
            w.aggressor = r.irange(2); w.name = mix64(g.seed ^ (0x40C1AULL + J.wars.size()));
            const double strength = std::log(std::max(1.0, P[0].peak) / std::max(1.0, P[1].peak));
            w.winner = r.chance(0.2) ? -1 : (r.chance(1.0 / (1.0 + std::exp(-0.6 * strength))) ? 0 : 1);
            const int wi = (int)J.wars.size(); J.wars.push_back(w);
            const int q0 = pairAt(ago, r.irange(3)); if (q0 < 0) { J.wars.pop_back(); continue; }
            JointEvent d; d.kind = HE_WAR; d.ago = ago; d.war = wi; d.place[0] = pairs[q0].a; d.place[1] = pairs[q0].b; d.salience = 0.8f; J.events.push_back(d);
            const int nb = 1 + r.irange(3);
            for (int k = 0; k < nb; k++) {
                const int64_t ba = w.from - (int64_t)((w.from - w.to) * (k + 0.5) / nb);
                const int q = pairAt(ba, r.irange(6)); if (q < 0) continue;
                JointEvent bt; bt.kind = HE_BATTLE; bt.ago = ba; bt.war = wi; bt.place[0] = pairs[q].a; bt.place[1] = pairs[q].b; bt.salience = 0.6f;
                bt.out = w.winner >= 0 && r.chance(0.7) ? w.winner : r.irange(2);
                const int fought = std::max(P[0].eras[P[0].eraAt(yearOf(P[0], ba))].age, P[1].eras[P[1].eraAt(yearOf(P[1], ba))].age);
                bt.value = (int64_t)std::min(std::min(P[0].peak, P[1].peak) * r.range(2e-4, 3e-3), BATTLE_CAP[fought] * r.range(0.15, 1.0));
                J.wars[wi].dead += bt.value;
                bt.season = 1 + r.irange(2);
                J.events.push_back(bt);
            }
            JointEvent pe; pe.kind = HE_PEACE; pe.ago = w.to; pe.war = wi; pe.out = w.winner < 0 ? 2 : w.winner; pe.salience = 0.6f;
            const int qp = pairAt(w.to, 0); if (qp >= 0) { pe.place[0] = pairs[qp].a; pe.place[1] = pairs[qp].b; }
            J.events.push_back(pe);
            ago = w.to;
        }
    }
    // the conquest: through the conquered people's collapse, the conqueror taking its places from the frontier in to its capital
    if (conquered >= 0 && conquestFrom < meet) {
        const int k = conquered;
        JointWar w; w.kind = HW_CONQUEST; w.from = conquestFrom; w.to = agoOf(P[k], P[k].lastYear); w.winner = 1 - k; w.aggressor = 1 - k; w.name = mix64(g.seed ^ 0x40C0AULL);
        const int wi = (int)J.wars.size(); J.wars.push_back(w);
        const int q0 = std::max(0, pairAt(w.from, 0));
        JointEvent d; d.kind = HE_WAR; d.ago = w.from; d.war = wi; d.place[0] = pairs[q0].a; d.place[1] = pairs[q0].b; d.salience = 0.9f; J.events.push_back(d);
        const int nb = std::max(2, std::min(8, (int)((w.from - w.to) / 3) + 1));
        for (int t = 0; t < nb; t++) {
            const int64_t ba = w.from - (int64_t)((w.from - w.to) * (t + 0.5) / nb);
            const int q = pairAt(ba, t); if (q < 0) continue;
            JointEvent bt; bt.kind = HE_BATTLE; bt.ago = ba; bt.war = wi; bt.place[0] = pairs[q].a; bt.place[1] = pairs[q].b; bt.out = r.chance(0.8) ? 1 - k : k; bt.salience = 0.7f;
            const int fought = std::max(P[0].eras[P[0].eraAt(yearOf(P[0], ba))].age, P[1].eras[P[1].eraAt(yearOf(P[1], ba))].age);
            bt.value = (int64_t)std::min(P[k].peak * r.range(5e-4, 4e-3), BATTLE_CAP[fought] * r.range(0.15, 1.0)); J.wars[wi].dead += bt.value; bt.season = 1 + r.irange(2);
            J.events.push_back(bt);
        }
    }
    // word of the other's end, where one went on (the meeting before it)
    const int64_t end0 = agoOf(P[0], P[0].lastYear), end1 = agoOf(P[1], P[1].lastYear);
    if (end0 != end1) {
        const int gone = end0 > end1 ? 0 : 1;
        const int64_t when = std::max(std::min(end0, end1) + 1, std::max(end0, end1) - (int64_t)r.range(1, 30));
        JointEvent o; o.kind = HE_OTHER_END; o.ago = when; o.place[gone] = -1; o.out = gone; o.salience = 0.7f;
        if (when < meet) J.events.push_back(o);
    }
    return J;
}

// ---- the builder ---------------------------------------------------------------------------------------------------------------
struct Builder {
    const StarSystem& sys; const Body& body; const BodyGen& g; const int people;
    History& H;
    std::shared_ptr<const WorldPlan> WP;
    const PeoplePlan* PP = nullptr;
    const PlanGrid* G = nullptr;
    Genome gene;
    WorldRead W;
    Rng rng;
    Schedule sched;
    std::vector<std::vector<float>> ageScores;   // the older ages' scores over the territory (the founding's eras, the cleared places)
    std::vector<uint8_t> mode;           // per person: 0 not stepped, 1 its death alone, 2 its whole life (marriage, children)
    std::vector<int> stepped;
    std::vector<double> hz;              // the year's harm per place (index + 1; 0 the whole land): the share it kills
    std::vector<int> hzEv, hzHow;
    double hzBattle = 0; int hzBattleEv = -1;   // the year's battles: the share of the fighting age they killed
    int reign = -1;
    bool vacant = false;
    int govNow = GOV_CHIEFS;
    int primo = 0, residence = 0, termRepublic = 4, termAssembly = 3;
    int seatOld = -1;                    // the far future's old capital (a cleared place): the seat until it was cleared
    double deficit = 0;
    std::vector<std::pair<int, double>> anchors;   // the people's number: (year, how many) between which it grew
    int earlyLeft = 0;
    int lastHazardEv = -1;               // the collapse's latest harm (a place left in it names it)
    int ruleEnds = 0;                    // the year the rule ended in the collapse
    int vacancyEv = -1;                  // what opened the rule (a ruler's death, a rule's end): the next accession names it
    int lastFamine = -100;
    int lastAccessionEv = -1;
    int leftThisYear = 0;                 // the year a place was last left early (the year's end rehouses its people)
    int stageEv[STAGE_COUNT] = {-1, -1, -1, -1};
    size_t plannedAtM = 0;                // the planned list's first of the year (the collapse's step reads its signs)
    int lastSignEv = -1;
    std::set<int> lawGivers;
    int lastHouse = -1;                  // the house that ruled last
    struct WarRun { int war; std::vector<int> battles; int siegeAt = -1; int target = -1; int end = 0; int lastBattle = -1; };
    std::vector<WarRun> warsOn;
    struct Wave { int year; int sickness; int place; int n; double share; int first; };
    std::vector<Wave> waves;
    struct Planned { int year; int kind; int a; int b; };   // inventions, institutions, faiths, discoveries, signs
    std::vector<Planned> planned;
    std::vector<std::vector<int>> wayAt;  // per year offset: the plan's edges opened then (sparse map instead)
    std::map<int, std::vector<int>> waysByYear;
    std::map<int, std::vector<int>> endsByYear;   // the collapse's: the places left in a year
    Joint joint; std::map<int, std::vector<int>> jointByYear;   // joint events by this people's year
    std::vector<int> jointWar;            // joint war -> this history's war
    std::vector<int> jointLastBattle;     // joint war -> its last battle here
    int cometPeriod = 0, cometPhase = 0;
    bool auroral = false;
    std::vector<int> famOf;               // per family: its living count (rebuilt each year)
    std::vector<std::vector<int>> kids;   // per person: the children, the eldest first
    std::map<int, std::vector<int>> foundByYear, festByYear, clearByYear;
    std::map<int, std::vector<std::pair<int, int>>> naturalEnds;   // a place left early with no event to blame: (place, why)
    std::vector<int> standBig, standSmall;   // the places founded (some since left: dropped when met)

    Builder(const StarSystem& s, const Body& b, const BodyGen& gg, int p, History& h) : sys(s), body(b), g(gg), people(p), H(h) {}

    // ---- small helpers
    int ageAt(int y) const { return H.eras[H.eraAt(y)].age; }
    double base(int y) const {
        if (anchors.empty()) return 1;
        if (y <= anchors.front().first) return anchors.front().second;
        for (size_t k = 1; k < anchors.size(); k++) {
            if (y > anchors[k].first) continue;
            const double t = smoothstep(0, 1, (double)(y - anchors[k - 1].first) / std::max(1, anchors[k].first - anchors[k - 1].first));
            return std::exp(std::log(anchors[k - 1].second) + t * (std::log(anchors[k].second) - std::log(anchors[k - 1].second)));
        }
        return anchors.back().second;
    }
    double pop(int y) const { return std::max(1.0, base(y) * (1 - deficit)); }
    double placePop(int p, int y) const {
        if (p < 0 || p >= (int)H.places.size()) return pop(y);
        const double share = H.places[p].peak / std::max(1.0, H.profile.peak);
        return std::max(40.0, share * pop(y) * (H.places[p].sclass == SC_CAPITAL ? 1.0 : 1.2));
    }
    int seasonFor(int kind) {
        if (H.seasons == 4) {
            switch (kind) {
                case HE_FOUNDING: case HE_FLOOD: if (rng.chance(0.6)) return 0; break;
                case HE_HARVEST: case HE_FAMINE: if (rng.chance(0.7)) return kind == HE_HARVEST ? 2 : 3; break;
                case HE_BATTLE: case HE_SIEGE: case HE_SACK: if (rng.chance(0.7)) return 1 + rng.irange(2); break;
                case HE_SICKNESS: if (rng.chance(0.5)) return 1; break;
                case HE_WINTER: return 3;
                default: break;
            }
            return rng.irange(4);
        }
        if (kind == HE_FLOOD && rng.chance(0.7)) return 0;
        if (kind == HE_HARVEST && rng.chance(0.7)) return 1;
        return rng.irange(2);
    }
    int ev(int kind, int year, int place = -1, float sal = 0.5f, int season = -2) {
        HistoryEvent e; e.kind = kind; e.year = year; e.place = place; e.salience = sal;
        e.pub = kind < HE_BIRTH;
        e.season = season == -2 ? seasonFor(kind) : season;
        H.events.push_back(e);
        return (int)H.events.size() - 1;
    }
    HistoryEvent& E(int i) { return H.events[i]; }
    bool stands(int p, int y) const { return H.placeStands(p, y); }
    int seat(int y) const {   // the seat of the rule
        if (seatOld >= 0 && stands(seatOld, y)) return seatOld;
        if (!H.places.empty() && stands(0, y)) return 0;
        for (int p = 0; p < (int)H.places.size(); p++) if (stands(p, y)) return p;
        return -1;
    }
    bool big(int p) const { return p >= 0 && (H.places[p].sclass == SC_CAPITAL || H.places[p].sclass == SC_CITY || (H.age == AGE_FUTURISTIC && H.places[p].sclass == SC_TOWN)); }
    // a standing place by weight (`w` may return 0 to skip one); -1 none
    template <class F> int pickPlace(int y, F w) {
        double tot = 0;
        for (int p = 0; p < (int)H.places.size(); p++) if (stands(p, y)) tot += w(p);
        if (tot <= 0) return -1;
        double u = rng.uni() * tot;
        for (int p = 0; p < (int)H.places.size(); p++) {
            if (!stands(p, y)) continue;
            const double x = w(p);
            if (x <= 0) continue;
            u -= x;
            if (u < 0) return p;
        }
        for (int p = (int)H.places.size() - 1; p >= 0; p--) if (stands(p, y) && w(p) > 0) return p;
        return -1;
    }
    // a standing place, the seat a quarter of the time, a great place `bigW` times as likely as a small one (from the kept lists)
    int anyPlace(int y, double bigW = 3, int notThis = -1) {
        const int s = seat(y);
        if (s >= 0 && s != notThis && rng.chance(0.25)) return s;
        for (int t = 0; t < 12; t++) {
            const double wb = bigW * standBig.size(), ws = (double)standSmall.size();
            if (wb + ws <= 0) break;
            std::vector<int>& L = rng.uni() * (wb + ws) < wb ? standBig : standSmall;
            if (L.empty()) continue;
            const int k = rng.irange((int)L.size());
            const int p = L[k];
            if (!stands(p, y)) { L[k] = L.back(); L.pop_back(); continue; }
            if (p == notThis) continue;
            return p;
        }
        return pickPlace(y, [&](int p) { return p == notThis ? 0.0 : (big(p) ? bigW : 1.0); });
    }
    void onFound(int p) { (big(p) ? standBig : standSmall).push_back(p); }
    void cleanStanding(int y) {
        for (std::vector<int>* L : {&standBig, &standSmall}) {
            std::vector<int> keep;
            for (int p : *L) if (H.places[p].ended >= y) keep.push_back(p);
            L->swap(keep);
        }
    }
    std::vector<Vec3> unit;               // per place: where it stands
    double distKm(int a, int b) const { return std::acos(clampd(dot(unit[a], unit[b]), -1, 1)) * g.R; }
    int nearestStanding(int from, int y, bool wantBig = false) {
        int best = -1; double bd = 1e300;
        for (int p = 0; p < (int)H.places.size(); p++) {
            if (p == from || !stands(p, y) || (wantBig && !big(p))) continue;
            const double d = from >= 0 ? -dot(unit[from], unit[p]) : p;   // the nearer the greater the dot
            if (d < bd) { bd = d; best = p; }
        }
        return best;
    }

    // ---- persons
    int person(int born, bool female, int family, int home, int role, int md, uint64_t name = 0) {
        HistoryPerson p;
        p.name = name ? name : mix64(H.seed ^ (0x4E50ULL + H.persons.size() * 0x9E3779B97F4A7C15ULL));
        p.female = female; p.born = born; p.died = ALIVE; p.family = family; p.home = home; p.role = role;
        const int a = ageAt(std::max(H.first, born + 20));
        p.trade = family >= 0 && rng.chance(0.6) ? H.families[family].trade : a * 10 + rng.irange(10);
        uint32_t t = 0;
        const double* v = H.profile.values;
        if (rng.chance(0.2 + v[VAL_WAR])) t |= HT_BRAVE;
        if (rng.chance(0.08 + 0.3 * v[VAL_WAR])) t |= HT_CRUEL;
        if (rng.chance(0.1 + v[VAL_PIETY])) t |= HT_PIOUS;
        if (rng.chance(0.15)) t |= HT_WISE;
        if (rng.chance(0.08 + v[VAL_LEARNING])) t |= HT_LEARNED;
        if (rng.chance(0.15)) t |= HT_AMBITIOUS;
        if (rng.chance(0.2)) t |= HT_KIND;
        if (rng.chance(0.12)) t |= HT_PROUD;
        p.traits = t;
        H.persons.push_back(p);
        mode.push_back((uint8_t)md);
        kids.emplace_back();
        if (md > 0) stepped.push_back((int)H.persons.size() - 1);
        if (family >= 0) H.families[family].persons++;
        return (int)H.persons.size() - 1;
    }
    void link(int child, int mother, int father) {
        H.persons[child].mother = mother; H.persons[child].father = father;
        if (mother >= 0) kids[mother].push_back(child);
        if (father >= 0) kids[father].push_back(child);
    }
    int figure(int y, int role, int home, int minAge, int maxAge, bool fromFamilies = true) {
        if (fromFamilies && y >= H.famFrom && rng.chance(0.2)) {   // one of the families followed, now and then
            std::vector<int> c;
            for (int q : stepped) { const HistoryPerson& P = H.persons[q]; if (mode[q] == 2 && P.died == ALIVE && P.role == HR_NONE && y - P.born >= minAge && y - P.born <= maxAge && (home < 0 || P.home == home)) c.push_back(q); }
            if (!c.empty()) { const int q = c[rng.irange((int)c.size())]; H.persons[q].role = role; return q; }
        }
        return person(y - minAge - rng.irange(std::max(1, maxAge - minAge)), rng.chance(0.5), -1, home, role, 1);
    }
    double hazard(int age, int years) const {
        const Mortality& M = MORTALITY[clampi(age, 0, AGE_COUNT - 1)];
        if (years < 1) return M.infant;
        if (years < 15) return M.child;
        if (years >= M.maxAge) return 1;
        return std::min(1.0, M.A + M.B * std::exp(M.c * years));
    }
    void die(int p, int y, int how, int cause, int place = -2) {
        HistoryPerson& P = H.persons[p];
        if (P.died != ALIVE) return;
        P.died = y; P.fate = HF_DIED;
        const int at = place == -2 ? P.home : place;
        const int role = P.role;
        bool pubDeath = role == HR_RULER || role == HR_REGENT || role == HR_GENERAL || role == HR_PROPHET || role == HR_INVENTOR || role == HR_EXPLORER || role == HR_REBEL || role == HR_LAST;
        if (role == HR_RULER && !(reign >= 0 && H.reigns[reign].ruler == p && H.reigns[reign].end == ALIVE)) {
            for (int k = (int)H.reigns.size() - 1; k >= 0; k--) if (H.reigns[k].ruler == p) {
                const int gv = H.reigns[k].government;
                if (gv == GOV_REPUBLIC || gv == GOV_ASSEMBLY || gv == GOV_CORPORATE) pubDeath = false;
                break;
            }
        }
        if (role == HR_FOUNDER) for (int q = 0; q < (int)H.places.size() && !pubDeath; q++) if (H.places[q].founder == p && big(q)) pubDeath = true;
        const int e = ev(HE_DEATH, y, at >= 0 && stands(at, y) ? at : -1, pubDeath ? (role == HR_RULER ? 0.6f : 0.35f) : 0.2f, cause >= 0 ? H.events[cause].season : -2);
        E(e).a = p; E(e).thing = how; E(e).cause = cause; E(e).pub = pubDeath;
        H.persons[p].death = e;
        mode[p] = 0;
        if (reign >= 0 && H.reigns[reign].ruler == p && H.reigns[reign].end == ALIVE) { H.reigns[reign].end = y; H.reigns[reign].endHow = 0; vacant = true; vacancyEv = e; }
    }
    void letGo(int p, int y) {   // followed no further: its death drawn alone (natural), told nowhere (never a person of the rule or a figure)
        HistoryPerson& P = H.persons[p];
        if (P.died != ALIVE || P.role != HR_NONE) return;
        const int a = ageAt(y);
        int d = y;
        while (d < H.last && !rng.chance(hazard(a, d - P.born))) d++;
        P.died = std::min(d, H.last); P.fate = HF_LET_GO; mode[p] = 0;
    }
    std::vector<int> childrenOf(int p) const { return p >= 0 && p < (int)kids.size() ? kids[p] : std::vector<int>(); }

    // ---- the rule
    int newReign(int ruler, int y, int how, int house = -1, int predecessor = -1) {
        HistoryReign R; R.ruler = ruler; R.start = y; R.end = ALIVE; R.government = govNow; R.house = house; R.how = how;
        H.reigns.push_back(R);
        reign = (int)H.reigns.size() - 1;
        vacant = false;
        if (ruler >= 0) { H.persons[ruler].role = HR_RULER; if (mode[ruler] == 0 && H.persons[ruler].died == ALIVE) { mode[ruler] = 1; stepped.push_back(ruler); } }
        const int e = ev(HE_ACCESSION, y, seat(y), how == 10 ? 0.25f : 0.55f);
        E(e).a = ruler; E(e).b = predecessor; E(e).thing = how; E(e).value = reign; E(e).cause = vacancyEv;
        vacancyEv = -1;
        lastAccessionEv = e;
        // a house that lost the rule: its members are followed no further (its branches would grow without end)
        if (lastHouse >= 0 && lastHouse != house) for (int q : stepped) if (mode[q] == 2 && H.persons[q].family == lastHouse && H.persons[q].died == ALIVE) letGo(q, y);
        if (house >= 0) { lastHouse = house; if (ruler >= 0) H.families[house].head = ruler; }
        return reign;
    }
    void endReign(int y, int how, int cause = -1) {
        if (reign < 0 || H.reigns[reign].end != ALIVE) return;
        H.reigns[reign].end = y; H.reigns[reign].endHow = how;
        const int r = H.reigns[reign].ruler;
        if (how != 0) { const int e = ev(HE_RULE_END, y, seat(y), how == 6 ? 0.7f : (how == 3 ? 0.15f : 0.4f)); E(e).a = r; E(e).thing = how; E(e).cause = cause; vacancyEv = e; if (how == 3) E(e).pub = false; }
        else vacancyEv = cause;
        vacant = true;
    }
    // a monarchy's heir: the late ruler's children by the people's rule, then the siblings, then the house's nearest
    int heirOf(int ruler, int y) {
        if (ruler < 0) return -1;
        auto order = [&](std::vector<int>& v) {
            std::stable_sort(v.begin(), v.end(), [&](int a, int b) {
                const HistoryPerson& A = H.persons[a]; const HistoryPerson& B = H.persons[b];
                if (primo != 1 && A.female != B.female) return primo == 0 ? !A.female : A.female;
                return A.born < B.born;
            });
        };
        auto livingOf = [&](const std::vector<int>& v) { std::vector<int> o; for (int q : v) if (H.persons[q].died == ALIVE) o.push_back(q); return o; };
        std::vector<int> kids = livingOf(childrenOf(ruler));
        order(kids);
        if (!kids.empty()) return kids[0];
        const HistoryPerson& R = H.persons[ruler];
        // the grandchildren through a child who died
        std::vector<int> grand;
        for (int c : childrenOf(ruler)) for (int gc : livingOf(childrenOf(c))) grand.push_back(gc);
        order(grand);
        if (!grand.empty()) return grand[0];
        const int par = R.father >= 0 ? R.father : R.mother;
        if (par >= 0) {
            std::vector<int> sib;
            for (int s : childrenOf(par)) if (s != ruler && H.persons[s].died == ALIVE) sib.push_back(s);
            order(sib);
            if (!sib.empty()) return sib[0];
            std::vector<int> neph;
            for (int s : childrenOf(par)) if (s != ruler) for (int n : livingOf(childrenOf(s))) neph.push_back(n);
            order(neph);
            if (!neph.empty()) return neph[0];
        }
        return -1;
    }
    int newHouse(int y, int founder) {
        HistoryFamily F; F.name = H.persons[founder].name; F.head = founder; F.founder = founder; F.home = seat(y); F.trade = 0; F.from = y; F.to = H.last; F.house = true;
        H.families.push_back(F);
        const int f = (int)H.families.size() - 1;
        H.persons[founder].family = f;
        H.families[f].persons++;
        if (mode[founder] < 2) { if (mode[founder] == 0) stepped.push_back(founder); mode[founder] = 2; }
        household(founder, y, f);
        return f;
    }
    // a person's household as it stood when the history took them up: a spouse and the children living (those who died young are not told)
    void household(int who, int y, int fam) {
        const int a = ageAt(y);
        const Mortality& M = MORTALITY[a];
        const int born = H.persons[who].born, years = y - born;
        if (H.persons[who].spouse >= 0 || years < M.marry + 3) return;
        const bool female = H.persons[who].female;
        const int home = H.persons[who].home;
        const int married = born + M.marry + rng.irange(std::max(1, std::min(10, years - M.marry - 2)));
        const int sp = person(std::min(born + rng.irange(9) - 4 + (female ? 3 : -3), married - M.marry), !female, fam, home, HR_NONE, 2);
        H.persons[who].spouse = sp; H.persons[sp].spouse = who;
        const int me = ev(HE_MARRIAGE, married, home >= 0 && stands(home, married) ? home : -1, 0.15f); E(me).a = who; E(me).b = sp;
        const int mother = female ? who : sp, father = female ? sp : who;
        const int mBorn = H.persons[mother].born;
        for (int c = married + 1; c < y && c - mBorn <= M.fertileTo; c++) {
            if (!rng.chance(M.births)) continue;
            double surv = 1;
            for (int t = 0; t < y - c; t++) surv *= 1 - hazard(a, t);
            if (!rng.chance(surv)) continue;
            const int k = person(c, rng.chance(0.5), fam, home, HR_NONE, 2);
            link(k, mother, father);
            const int be = ev(HE_BIRTH, c, home >= 0 && stands(home, c) ? home : -1, 0.1f); E(be).a = k; E(be).b = mother; E(be).c = father;
        }
    }
    // the family's head now: the first followed, then the eldest living child of the head before (a grandchild through a dead child, a
    // living member at last)
    int familyHead(int fam) {
        int h = H.families[fam].head;
        for (int guard = 0; guard < 64 && h >= 0 && H.persons[h].died != ALIVE; guard++) {
            int next = -1;
            std::vector<int> frontier = kids[h];
            for (int depth = 0; depth < 4 && next < 0 && !frontier.empty(); depth++) {
                for (int c : frontier) if (H.persons[c].died == ALIVE && H.persons[c].family == fam && mode[c] == 2) { next = c; break; }
                std::vector<int> nx; for (int c : frontier) for (int gc : kids[c]) nx.push_back(gc);
                frontier.swap(nx);
            }
            if (next < 0) for (int q : stepped) if (mode[q] == 2 && H.persons[q].family == fam && H.persons[q].died == ALIVE && (next < 0 || H.persons[q].born < H.persons[next].born)) next = q;
            h = next;
        }
        H.families[fam].head = h;
        return h;
    }
    void succeed(int y, int how) {
        const int prev = reign >= 0 ? H.reigns[reign].ruler : -1;
        const int s = seat(y);
        if (govNow == GOV_MACHINE) { newReign(-1, y, 8, -1, prev); return; }
        if (govNow == GOV_MONARCHY) {
            const int house = reign >= 0 ? H.reigns[reign].house : -1;
            int heir = house >= 0 && H.reigns[reign].government == GOV_MONARCHY ? heirOf(prev, y) : -1;
            if (heir >= 0) {
                const int r = newReign(heir, y, 1, house, prev);
                H.persons[heir].home = s;
                if (y - H.persons[heir].born < 16) {   // a child: the other parent, an uncle or aunt, or a lord of the realm rules for it
                    int regent = -1;
                    const int km = H.persons[heir].mother, kf = H.persons[heir].father;
                    for (int par : {km, kf}) if (par >= 0 && par != prev && H.persons[par].died == ALIVE) { regent = par; break; }
                    if (regent < 0) regent = figure(y, HR_REGENT, s, 30, 55, false);
                    if (H.persons[regent].role == HR_NONE) H.persons[regent].role = HR_REGENT;
                    H.reigns[r].regent = regent;
                    const int e = ev(HE_REGENCY, y, s, 0.4f); E(e).a = regent; E(e).b = heir; E(e).cause = lastAccessionEv;
                }
                return;
            }
            // no heir: a new house, chosen by the lords or by a war between two claimants
            if (prev >= 0 && rng.chance(0.4) && H.places.size() > 1) {
                const int a = figure(y, HR_RULER, s, 25, 50, false);
                const int other = pickPlace(y, [&](int p) { return p != s && big(p) ? 1.0 : 0.0; });
                const int b = figure(y, HR_REBEL, other >= 0 ? other : s, 25, 50, false);
                newHouse(y, a);
                newReign(a, y, 4, (int)H.families.size() - 1, prev);
                startWar(y, HW_SUCCESSION, -1, other >= 0 ? other : s, a, b);
                return;
            }
            const int a = figure(y, HR_RULER, s, 25, 50, false);
            const int f = newHouse(y, a);
            newReign(a, y, rng.chance(0.6) ? 2 : 4, f, prev);
            return;
        }
        int lo = 30, hi = 50, h = 2;
        switch (govNow) {
            case GOV_CHIEFS: lo = 28; hi = 50; h = 2; break;
            case GOV_COUNCIL: lo = 50; hi = 72; h = 2; break;
            case GOV_THEOCRACY: lo = 38; hi = 60; h = 2; break;
            case GOV_REPUBLIC: lo = 40; hi = 65; h = 3; break;
            case GOV_ASSEMBLY: lo = 35; hi = 65; h = 3; break;
            case GOV_CORPORATE: lo = 40; hi = 60; h = 5; break;
            default: break;
        }
        if (how == 9) h = 9;
        const int a = figure(y, HR_RULER, s, lo, hi, true);
        newReign(a, y, h, -1, prev);
    }
    // a ruler's term (the elected and appointed forms), 0 for life
    int termOf(int y) {
        const int a = ageAt(y);
        const int k = a == AGE_FAR_FUTURE ? 12 : (a == AGE_FUTURISTIC ? 2 : (a == AGE_PREHISTORIC ? 3 : 1));
        switch (govNow) {
            case GOV_REPUBLIC: return termRepublic * k;
            case GOV_ASSEMBLY: return termAssembly * k;
            case GOV_CORPORATE: return (5 + rng.irange(11)) * k;
            default: return 0;
        }
    }

    // ---- war
    int startWar(int y, int kind, int sideA, int sideB, int leaderA, int leaderB, int cause = -1, int length = 0) {
        HistoryWar w; w.kind = kind; w.start = y; w.sideA = sideA; w.sideB = sideB; w.leaderA = leaderA; w.leaderB = leaderB; w.dead = 0;
        w.name = mix64(H.seed ^ (0x40A5ULL + H.wars.size() * 0x9E3779B97F4A7C15ULL));
        const int a = ageAt(y);
        if (length <= 0) {
            static const double LEN[HW_COUNT] = {2, 6, 4, 4, 3, 5, 4, 10};
            length = std::max(1, (int)std::lround(std::exp(rng.range(0, std::log(LEN[kind] * (a >= AGE_MODERN ? 0.6 : 1.0) * 2)))));
        }
        w.end = std::min(H.last, y + length - 1);
        if (kind == HW_REBELLION) w.winner = rng.chance(0.68) ? 0 : 1;
        else if (kind == HW_REVOLUTION) w.winner = 1;
        else w.winner = rng.chance(0.15) ? -1 : rng.irange(2);
        H.wars.push_back(w);
        const int wi = (int)H.wars.size() - 1;
        const int target = sideB >= 0 ? sideB : (sideA >= 0 ? sideA : seat(y));
        const int e = ev(HE_WAR, y, target >= 0 && stands(target, y) ? target : -1, kind == HW_BANDS ? 0.45f : 0.7f);
        E(e).thing = wi; E(e).a = leaderA; E(e).b = leaderB; E(e).cause = cause;
        H.wars[wi].declared = e;
        WarRun run; run.war = wi; run.end = w.end; run.target = target;
        const int nb = std::max(1, std::min(8, 1 + (int)std::lround(rng.range(0, 0.6 * length))));
        for (int k = 0; k < nb; k++) run.battles.push_back(y + (int)std::floor((w.end - y + 1) * (k + rng.uni() * 0.9) / nb));
        std::sort(run.battles.begin(), run.battles.end());
        if (kind != HW_BANDS && rng.chance(0.45)) run.siegeAt = run.battles.back();
        warsOn.push_back(run);
        return wi;
    }
    int warLeader(int w, int side, int y) {
        HistoryWar& W2 = H.wars[w];
        int& L = side == 0 ? W2.leaderA : W2.leaderB;
        if (L >= 0 && H.persons[L].died == ALIVE && y - H.persons[L].born < 75) return L;
        if (side == 0 && W2.sideA == -1 && reign >= 0 && H.reigns[reign].ruler >= 0 && rng.chance(0.3)) return H.reigns[reign].ruler;
        const int home = side == 0 ? (W2.sideA >= 0 ? W2.sideA : seat(y)) : W2.sideB;
        L = figure(y, side == 0 ? HR_GENERAL : (W2.kind == HW_REBELLION || W2.kind == HW_REVOLUTION ? HR_REBEL : HR_GENERAL), home >= 0 && stands(home, y) ? home : seat(y), 25, 50);
        return L;
    }
    void stepWars(int y) {
        for (size_t k = 0; k < warsOn.size();) {
            WarRun& run = warsOn[k];
            HistoryWar& w = H.wars[run.war];
            const int a = ageAt(y);
            for (int by : run.battles) {
                if (by != y) continue;
                int at = run.target;
                if (at < 0 || !stands(at, y) || rng.chance(0.4)) {
                    const int near = w.sideB >= 0 && stands(w.sideB, y) ? w.sideB : seat(y);
                    at = pickPlace(y, [&](int p) { return near >= 0 && p != near ? 1.0 / (1.0 + distKm(near, p) / 60.0) : (p == near ? 0.6 : 0.0); });
                }
                const int side = w.winner >= 0 && rng.chance(0.72) ? w.winner : rng.irange(2);
                const int la = warLeader(run.war, side, y), lb = warLeader(run.war, 1 - side, y);
                static const double DEAD[AGE_COUNT][2] = {{1e-3, 1e-2}, {1e-4, 1.5e-3}, {5e-4, 4e-3}, {1e-3, 6e-3}, {1e-4, 1e-3}, {1e-5, 1e-4}};
                const double drawn = pop(y) * rng.range(DEAD[a][0], DEAD[a][1]) * (w.kind == HW_BANDS ? 0.3 : 1.0);
                const int64_t dead = std::max<int64_t>(3, (int64_t)std::min(drawn, BATTLE_CAP[a] * rng.range(0.15, 1.0)));
                const int e = ev(HE_BATTLE, y, at, 0.55f);
                E(e).thing = run.war; E(e).a = la; E(e).b = lb; E(e).out = side; E(e).value = dead; E(e).cause = w.declared;
                run.lastBattle = e;
                w.dead += dead; w.battles++;
                deficit += dead / pop(y);
                const double share = dead / std::max(1.0, pop(y) * 0.12);
                if (share > hzBattle) { hzBattle = share; hzBattleEv = e; }
                if (la >= 0 && H.persons[la].died == ALIVE && rng.chance(0.04)) die(la, y, 3, e, at);
                if (lb >= 0 && H.persons[lb].died == ALIVE && rng.chance(0.12)) die(lb, y, 3, e, at);
                if (run.siegeAt == y && at >= 0) {
                    const int s2 = ev(HE_SIEGE, y, at, 0.5f); E(s2).thing = run.war; E(s2).cause = e;
                    const bool fell = rng.chance(0.55);
                    E(s2).out = fell ? 1 : 0;
                    if (fell && at != seat(y) && rng.chance(0.55)) {
                        const int64_t sd = (int64_t)(placePop(at, y) * rng.range(0.03, 0.25));
                        const int sk = ev(HE_SACK, y, at, 0.55f); E(sk).thing = run.war; E(sk).value = sd; E(sk).cause = s2;
                        harm(at, sd / std::max(1.0, placePop(at, y)), sk, 3);
                        maybeLeave(at, y, sk, H.places[at].sclass == SC_CITY ? 0.06 : 0.16);
                    }
                }
            }
            if (y >= run.end) {
                const int e = ev(HE_PEACE, y, run.target >= 0 && stands(run.target, y) ? run.target : -1, 0.5f);
                E(e).thing = run.war; E(e).out = w.winner < 0 ? 2 : w.winner; E(e).cause = run.lastBattle >= 0 ? run.lastBattle : w.declared;
                const int la = w.leaderA >= 0 && H.persons[w.leaderA].died == ALIVE ? w.leaderA : -1, lb = w.leaderB >= 0 && H.persons[w.leaderB].died == ALIVE ? w.leaderB : -1;
                E(e).a = la; E(e).b = lb;
                w.peace = e; w.end = y;
                // a war of succession: the claimant who won rules
                if (w.kind == HW_SUCCESSION && w.winner == 1 && lb >= 0 && govNow == GOV_MONARCHY) {
                    endReign(y, 1, e);
                    const int prev = reign >= 0 ? H.reigns[reign].ruler : -1;
                    const int f = newHouse(y, lb);
                    newReign(lb, y, 6, f, prev);
                }
                // a rising that won deposes the ruler now and then
                if ((w.kind == HW_REBELLION || w.kind == HW_CIVIL) && w.winner == 1 && rng.chance(0.35) && reign >= 0 && H.reigns[reign].end == ALIVE && govNow != GOV_MACHINE) {
                    endReign(y, 1, e);
                    if (lb >= 0 && govNow == GOV_MONARCHY) { const int prev = H.reigns[reign].ruler; const int f = newHouse(y, lb); newReign(lb, y, 6, f, prev); }
                }
                warsOn.erase(warsOn.begin() + k);
                continue;
            }
            k++;
        }
    }

    // ---- harm: the share of a place's people (or the whole land's) an event kills this year, for the persons' deaths
    std::vector<int> hzTouched;
    void harm(int place, double share, int ev2, int how) {
        const int k = place + 1;
        if (k < 0 || k >= (int)hz.size()) return;
        if (hz[k] == 0 && share > 0) hzTouched.push_back(k);
        if (share > hz[k]) { hz[k] = share; hzEv[k] = ev2; hzHow[k] = how; }
    }
    void maybeLeave(int p, int y, int cause, double chance) {
        if (p < 0 || p == seat(y) || p == 0 || H.places[p].plan < 0) return;
        if (y >= H.profile.collapseYear - 2 || y - H.places[p].founded < 30) return;
        if (earlyLeft * 7 > (int)H.places.size()) return;
        if (!rng.chance(chance)) return;
        leave(p, y, HP_EARLY, cause, 0);
        earlyLeft++;
    }
    void leave(int p, int y, int fate, int cause, int why) {
        HistoryPlace& P = H.places[p];
        if (P.ended < y) return;
        P.ended = y; P.fate = fate;
        leftThisYear = y;
        const int e = ev(HE_ABANDONED, y, p, big(p) ? 0.6f : 0.3f);
        E(e).cause = cause; E(e).thing = why;
        P.endEvent = e;
        // its people move to the nearest place that stands on
        const int to = nearestStanding(p, y + 1, false);
        for (int q : stepped) {
            HistoryPerson& Q = H.persons[q];
            if (Q.died != ALIVE || Q.home != p) continue;
            Q.home = to;
            if (mode[q] == 2 && to >= 0 && y - Q.born >= 16) { const int m = ev(HE_MOVE, y, p, 0.15f); E(m).a = q; E(m).place2 = to; E(m).cause = e; }
        }
    }

    // ---- the build
    void setup();
    void setupPlaces();
    void setupCollapse();
    void setupPlanned();
    void oldTime();
    void yearLoop();
    void stepPersons(int y);
    void everyday(int y);                // H-04's second pass: the everyday's own stream (no harm, no person made)
    void family();                       // H-04's second pass: the families' own everyday, after the years (each person's stream)
    size_t evScan = 0; bool everContact = false, everMine = false;   // what the everyday has seen of the events so far
    void randomEvents(int y);
    void collapseYear(int y);
    void importJoint();
    void finish();
};

void Builder::setup() {
    const PeopleProfile& P = H.profile;
    H.seed = P.seed; H.age = P.age; H.cause = P.cause;
    H.last = P.lastYear;
    gene = genomeOf(P.seed);
    W = readWorld(sys, body, g);
    rng = Rng(P.seed ^ 0x40A11ULL);
    H.seasons = std::fabs(body.axialTilt) / DEG > 8 ? 4 : 2;
    primo = rng.pick(std::vector<double>{0.45, 0.35, 0.2}.data(), 3);
    residence = rng.chance(0.7) ? 0 : 1;
    static const int TERMS[6] = {3, 4, 4, 5, 6, 7};
    termRepublic = TERMS[rng.irange(6)]; termAssembly = 3 + rng.irange(4);
    // the eras: A-01's, named, with the faith and what they lived by (the last era's the profile's, the earlier drawn by their age, often kept)
    H.eras.clear();
    for (size_t k = 0; k < P.eras.size(); k++) {
        const Era& e = P.eras[k];
        HistoryEra he; he.age = e.age; he.start = e.start; he.end = e.end; he.government = e.government;
        he.govFrom = k == 0 ? (e.age == AGE_PREHISTORIC ? GOV_CHIEFS : e.government) : P.eras[k - 1].government;
        H.eras.push_back(he);
    }
    for (int k = (int)H.eras.size() - 1; k >= 0; k--) {
        HistoryEra& he = H.eras[k];
        if (k == (int)H.eras.size() - 1) { he.faith = P.faith; he.economy = P.economy; continue; }
        const HistoryEra& nx = H.eras[k + 1];
        static const double FW[AGE_COUNT][FAITH_COUNT] = {{4, 4, 3, 1, 0.2, 0, 0}, {2, 1, 1, 4, 4, 0.5, 0}, {1, 0.3, 0.3, 2, 4, 2, 0}, {1, 0.2, 0.2, 1, 3, 4, 0.2}, {1, 0.2, 0.2, 0.3, 1, 5, 2}, {0.2, 0.2, 1, 0.2, 0.2, 3, 3}};
        static const double EW[AGE_COUNT][ECO_COUNT] = {{4, 3, 3, 0.3, 0, 0}, {0.3, 2, 6, 2, 0.2, 0}, {0, 0.3, 2, 2, 6, 0}, {0, 0, 0.5, 3, 4, 3}, {0, 0, 0.2, 1, 3, 6}, {0, 0, 0.1, 0.5, 0.5, 8}};
        he.faith = rng.chance(0.45) && FW[he.age][nx.faith] > 0 ? nx.faith : rng.pick(FW[he.age], FAITH_COUNT);
        he.economy = rng.pick(EW[he.age], ECO_COUNT);
    }
    // the first government of a medieval people's first era is often the chiefs' (the kingdom founded later in it)
    for (size_t k = 0; k < H.eras.size(); k++) {
        HistoryEra& he = H.eras[k];
        if (k > 0 && H.eras[k - 1].age == AGE_PREHISTORIC && he.age >= AGE_MEDIEVAL && rng.chance(0.5)) he.govFrom = GOV_CHIEFS;
        if (he.age == AGE_PREHISTORIC && H.age == AGE_PREHISTORIC) he.govFrom = GOV_CHIEFS;
        const bool oldTimeOnly = he.age == AGE_PREHISTORIC && H.age != AGE_PREHISTORIC;
        if (he.govFrom != he.government && !oldTimeOnly) {
            const int len = he.end - he.start + 1;
            int stop = he.end;
            if (k == H.eras.size() - 1) stop = std::min(stop, P.collapseYear - 1);
            he.govChange = clampi(he.start + (int)std::lround(len * rng.range(0.12, 0.65)), he.start + 1, std::max(he.start + 1, stop));
        }
    }
    // the names: the government's and the values' first where they speak, else the age's own
    for (size_t k = 0; k < H.eras.size(); k++) {
        HistoryEra& he = H.eras[k];
        const int a = he.age;
        int pickIdx = (int)(unitFromHash(mix64(P.seed ^ (0x40E4AULL + k))) * 6);
        if (a == AGE_MEDIEVAL) {
            if (he.government == GOV_MONARCHY) pickIdx = rng.chance(0.5) ? 0 : 1;
            else if (he.government == GOV_THEOCRACY) pickIdx = 2;
            else if (he.government == GOV_COUNCIL) pickIdx = 3;
            else if (P.values[VAL_LEARNING] > 0.3) pickIdx = 4;
            else if (P.values[VAL_TRADE] > 0.3) pickIdx = 5;
        }
        if (a == AGE_INDUSTRIAL && he.government == GOV_CORPORATE) pickIdx = 5;
        he.name = ERA_NAMES[a][clampi(pickIdx, 0, 5)];
        if (a == AGE_MEDIEVAL && he.government == GOV_MONARCHY && pickIdx == 0) he.name = "the Kingdom of " + H.lore.city;
        if (a == AGE_PREHISTORIC && H.age == AGE_PREHISTORIC) { static const char* const OWN[4] = {"the Time of the Fires", "the Gathering", "the Years of the Bands", "the Herds' Years"}; he.name = OWN[pickIdx % 4]; }
        {   // H-04's second pass: half the peoples' eras take one of the age's further names (their own way of naming them; the draws above stand)
            const uint64_t eh = mix64(P.seed ^ 0xE4A5ULL ^ (uint64_t)(k + 1) * 0x9E3779B97F4A7C15ULL);
            if (unitFromHash(eh) < 0.55) {
                std::string nm = ERA_MORE[clampi(a, 0, AGE_COUNT - 1)][(eh >> 20) % 10];
                auto fill = [&](const char* key, const std::string& v) { size_t at = nm.find(key); if (at != std::string::npos) nm.replace(at, std::strlen(key), v); };
                fill("{city}", H.lore.city); fill("{god}", H.lore.god); fill("{river}", H.lore.river); fill("{mountain}", H.lore.mountain); fill("{founder}", H.lore.founder);
                fill("{moon}", H.lore.moons > 0 ? H.lore.moon : H.lore.star);
                he.name = nm;
            }
        }
    }
    // the years told: the old time's start (a prehistoric people's coming, a few hundred years before its first village)
    H.first = P.eras.front().start;
    if (H.age == AGE_PREHISTORIC) H.first = 1 - 100 * (3 + rng.irange(28));
    H.famFrom = std::max(1, H.last - FAMILY_YEARS);
    // the sky: the comet that returns in a span the people could count, the lights
    const double yearSec = std::max(1.0, body.orbitPeriod);
    double bestP = 0;
    for (const Body& o : sys.bodies) {
        if (o.type != PT_COMET) continue;
        const double per = o.orbitPeriod / yearSec;
        if (per >= 30 && per <= 2500 && (bestP == 0 || per < bestP)) bestP = per;
    }
    if (bestP > 0) { cometPeriod = (int)std::lround(bestP); cometPhase = (int)(unitFromHash(mix64(P.seed ^ 0x40C0EULL)) * cometPeriod); }
    auroral = auroraPotentialAt(sys, body, 0.7) > 0.25;
}

void Builder::setupPlaces() {
    const PeopleProfile& P = H.profile;
    const int n = PP ? (int)PP->places.size() : 0;
    sched = n ? foundingSchedule(g, *G, *PP, P, people, &ageScores) : Schedule();
    std::vector<std::string> used = {H.lore.people, H.lore.god, H.lore.river, H.lore.mountain, H.lore.sea, H.lore.city, H.lore.city2, H.lore.founder, H.lore.moon, H.lore.star, H.lore.festival};
    const Tongue& TG = tongueOfSeed(H.lore.seed);   // H-04's second pass: the places' names in the people's tongue, by their rank
    auto uniqueName = [&](uint64_t seed, int sclass) {
        for (int t = 0; t < 24; t++) {
            const uint64_t s = mix64(seed + t * 0x9E3779B97F4A7C15ULL);
            const std::string nm = tongueName(TG, s, tongueNameKindOfRank(sclass));
            if (std::find(used.begin(), used.end(), nm) == used.end()) { used.push_back(nm); return s; }
        }
        return mix64(seed ^ 0x55);
    };
    const bool farFuture = H.age == AGE_FAR_FUTURE;
    for (int i = 0; i < n; i++) {
        const PlanPlace& p = PP->places[i];
        HistoryPlace hp;
        hp.plan = i; hp.sclass = p.sclass; hp.lat = p.lat; hp.lon = p.lon; hp.peak = p.population; hp.coast = p.coast; hp.river = p.river;
        hp.founded = sched.founded[i]; hp.age = sched.age[i]; hp.ended = H.last;
        if (!farFuture && i == 0) hp.name = NAME_CITY;
        else if (!farFuture && i == 1) hp.name = NAME_CITY2;
        else hp.name = uniqueName(p.seed ^ 0x40A3EULL, p.sclass);
        H.places.push_back(hp);
    }
    // a futuristic or far-future people's older cities, which its plan no longer draws: the land it cleared, kept as places
    if (H.age >= AGE_FUTURISTIC && PP && !PP->territory.empty()) {
        if (ageScores.size() < AGE_COUNT) ageScores.resize(AGE_COUNT);
        if (ageScores[AGE_MEDIEVAL].empty()) planScores(g, *G, people, AGE_MEDIEVAL, ageScores[AGE_MEDIEVAL], &PP->territory);
        const std::vector<float>& sm = ageScores[AGE_MEDIEVAL];
        std::vector<std::pair<double, int>> cand;
        for (int c = 0; c < G->cells; c++) if (PP->territory[c] && sm[c] > 0.25f) cand.push_back({sm[c] + 0.2 * unitFromHash(hash2i(c, 0x40C1, P.seed)), c});
        std::sort(cand.begin(), cand.end(), [](const std::pair<double, int>& a, const std::pair<double, int>& b) { return a.first != b.first ? a.first > b.first : a.second < b.second; });
        const int want = farFuture ? 16 + rng.irange(10) : 12 + rng.irange(10);
        std::vector<Vec3> taken;
        for (const PlanPlace& p : PP->places) taken.push_back(p.unit);
        const size_t planN = taken.size();
        const double minLost = 220.0 / g.R, minPlan = 110.0 / g.R;
        int made = 0;
        for (const auto& cv : cand) {
            if (made >= want) break;
            const Vec3 u = G->unitOf(cv.second);
            bool near = false;
            for (size_t t = 0; t < taken.size() && !near; t++) near = std::acos(clampd(dot(u, taken[t]), -1, 1)) < (t < planN ? minPlan : minLost);
            if (near) continue;
            taken.push_back(u);
            HistoryPlace hp;
            hp.plan = -1; hp.fate = HP_CLEARED;
            hp.sclass = made == 0 && farFuture ? SC_CAPITAL : (made < want / 3 ? SC_CITY : SC_TOWN);
            G->latLonOf(cv.second, hp.lat, hp.lon);
            hp.coast = G->coastKm[cv.second] < 1.5 * G->sizeKm(); hp.river = G->river[cv.second] > 15;
            hp.peak = H.profile.peak * (hp.sclass == SC_CAPITAL ? 0.02 : (hp.sclass == SC_CITY ? 0.006 : 0.0015));
            hp.name = made == 0 && farFuture ? NAME_CITY : (made == 1 && farFuture ? NAME_CITY2 : uniqueName(mix64(P.seed ^ (0x40C2ULL + cv.second)), hp.sclass));
            H.places.push_back(hp);
            made++;
        }
        // founded through the older ages (the medieval most), cleared in the people's own (the far future's old capital the last)
        int firstHist = 0; while (firstHist + 1 < (int)H.eras.size() && H.eras[firstHist].age == AGE_PREHISTORIC) firstHist++;
        int clearEra = (int)H.eras.size() - 1; while (clearEra > 0 && H.eras[clearEra].age > AGE_FUTURISTIC) clearEra--;
        const HistoryEra& CE = H.eras[clearEra];
        const HistoryEra& LE = H.eras.back();
        for (int i = n; i < (int)H.places.size(); i++) {
            HistoryPlace& hp = H.places[i];
            const bool oldCap = farFuture && i == n;
            int e = firstHist;
            if (!oldCap && rng.chance(0.25)) e = std::min((int)H.eras.size() - 1, firstHist + 1 + rng.irange(2));
            if (H.eras[e].age >= H.age) e = firstHist;
            const HistoryEra& FE = H.eras[e];
            hp.founded = oldCap ? 1 : clampi(FE.start + (int)(rng.range(0.02, 0.6) * (FE.end - FE.start + 1)), std::max(2, FE.start), std::max(2, FE.end));
            hp.age = FE.age;
            const HistoryEra& when = oldCap ? LE : (farFuture && rng.chance(0.5) ? LE : CE);
            const int stop = std::min(when.end, P.collapseYear - 1);
            hp.ended = clampi(when.start + (int)(rng.range(0.1, oldCap ? 0.5 : 0.8) * (stop - when.start + 1)), std::max(hp.founded + 20, when.start), std::max(hp.founded + 20, stop));
            hp.ended = std::min(hp.ended, P.collapseYear - 1);
            if (hp.ended <= hp.founded) hp.ended = std::min(P.collapseYear - 1, hp.founded + 50);
        }
        if (farFuture && (int)H.places.size() > n) seatOld = n;
    }
    unit.clear();
    for (const HistoryPlace& hp : H.places) unit.push_back(StarSystem::bodyFromLatLon(hp.lat, hp.lon));
    // the settlers' places: the nearest standing before each founding
    for (int i = 0; i < (int)H.places.size(); i++) {
        HistoryPlace& hp = H.places[i];
        int best = -1; double bd = 1e300;
        for (int j = 0; j < (int)H.places.size(); j++) {
            if (j == i || H.places[j].founded >= hp.founded || H.places[j].ended < hp.founded) continue;
            const double d = -dot(unit[i], unit[j]);
            if (d < bd) { bd = d; best = j; }
        }
        hp.from = best;
    }
    // the ways: the year each of the plan's opened (both its places standing, the kind's invention made)
    H.wayOpened.assign(PP ? PP->edges.size() : 0, 0);
}

void Builder::setupCollapse() {
    const PeopleProfile& P = H.profile;
    // the stages' first years
    H.stageYear[STAGE_WARNING] = P.collapseYear;
    for (int s = 1; s < STAGE_COUNT; s++) {
        int y = H.stageYear[s - 1];
        while (y < H.last && P.stageAt(y) < s) y++;
        H.stageYear[s] = y;
    }
    // the order the places were left in by the cause, the last place the least exposed
    const CauseScript& CS = SCRIPTS[H.cause];
    std::vector<std::pair<double, int>> exp;
    double latMax = 1e-9;
    for (const HistoryPlace& p : H.places) latMax = std::max(latMax, std::fabs(p.lat));
    for (int i = 0; i < (int)H.places.size(); i++) {
        const HistoryPlace& p = H.places[i];
        if (p.ended < P.collapseYear) continue;   // cleared before
        double x = 0;
        const bool capital = i == 0 || p.sclass == SC_CAPITAL;
        const double remote = p.plan >= 0 ? PP->places[p.plan].remote : 0.5;
        const double sz = p.sclass == SC_CAPITAL ? 1 : (p.sclass == SC_CITY ? 0.7 : (p.sclass == SC_TOWN ? 0.4 : 0.2));
        double h = 0; if (p.plan >= 0) h = G->height[PP->places[p.plan].cell];
        switch (CS.order) {
            case OR_REMOTE: x = remote - (capital ? 1 : 0); break;
            case OR_COAST: x = (p.coast ? 0.6 : 0) + (p.river ? 0.3 : 0) - h / 3000.0; break;
            case OR_COLD: x = std::fabs(p.lat) / latMax + h / 4000.0; break;
            case OR_DRY: x = (p.river ? -0.5 : 0) + (p.coast ? -0.2 : 0) + 0.5 * remote; break;
            case OR_CROWDED: x = sz + (p.coast ? 0.15 : 0); break;
            case OR_SMALL: x = 1 - sz - (capital ? 0.5 : 0); break;
            default: x = 0; break;
        }
        x += 0.25 * unitFromHash(mix64(P.seed ^ (0x40C7ULL + i)));
        if (H.age == AGE_FAR_FUTURE && p.plan >= 0) x -= 5;   // a far-future people's works outlast everything else
        exp.push_back({x, i});
    }
    std::sort(exp.begin(), exp.end(), [](const std::pair<double, int>& a, const std::pair<double, int>& b) { return a.first != b.first ? a.first > b.first : a.second < b.second; });
    if (exp.empty()) return;
    H.lastPlace = exp.back().second;
    const int C = H.stageYear[STAGE_CRISIS], L = H.last;
    const int m = (int)exp.size();
    for (int k = 0; k < m; k++) {
        const int i = exp[k].second;
        int y = L;
        if (k < m - 1) y = C + (int)std::floor((L - C) * std::pow((k + 0.5) / m, 0.7));
        y = clampi(y, std::max(C, H.places[i].founded), L);
        if (k < m - 1 && y >= L && L > C) y = L - 1;
        y = std::max(y, H.places[i].founded);
        H.places[i].ended = y;
        H.places[i].fate = k == m - 1 ? HP_LAST : HP_COLLAPSE;
        endsByYear[y].push_back(i);
    }
}

void Builder::setupPlanned() {
    const PeopleProfile& P = H.profile;
    // the inventions: each age's, at its share of the whole age, as far as the people got (radio at its year)
    for (size_t k = 0; k < H.eras.size(); k++) {
        const HistoryEra& he = H.eras[k];
        if (he.age == AGE_PREHISTORIC && H.age != AGE_PREHISTORIC) continue;
        const double full = he.age == AGE_PREHISTORIC ? std::max(400.0, (double)H.last) : std::max(1.0, P.fullYears[he.age]);
        const int stop = std::min(he.end, P.collapseYear - 1);
        for (int i = 0; i < INVENTION_COUNT; i++) {
            const InventionInfo& I = INVENTIONS[i];
            if (I.age != he.age) continue;
            int y;
            if (I.at < -5) { if (!P.radio || P.radioYear <= 0) continue; y = P.radioYear; }
            else y = he.start + (int)std::lround(I.at * full);
            if (y < 1) y = 1 + rng.irange(20);   // writing: with the first towns
            if (y > stop) continue;
            planned.push_back({y, HE_INVENTION, i, 0});
        }
    }
    // the institutions, at the eras' and the governments' turns
    auto inst = [&](int kind, int y) { if (y >= std::max(1, H.first) && y < P.collapseYear) planned.push_back({y, HE_INSTITUTION, kind, 0}); };
    for (size_t k = 0; k < H.eras.size(); k++) {
        const HistoryEra& he = H.eras[k];
        const int len = he.end - he.start + 1;
        auto at = [&](double lo, double hi) { return he.start + (int)std::lround(len * rng.range(lo, hi)); };
        if (he.age == AGE_MEDIEVAL) {
            if (P.values[VAL_PIETY] > 0.15) inst(HI_PRIESTHOOD, at(0.03, 0.25));
            if (P.values[VAL_TRADE] > 0.12) inst(HI_GUILDS, at(0.25, 0.6));
            if (P.values[VAL_LEARNING] > 0.12) inst(HI_ACADEMY, at(0.35, 0.8));
            if (P.values[VAL_WAR] > 0.22 && he.government == GOV_MONARCHY) inst(HI_ARMY, at(0.2, 0.7));
        }
        if (he.age == AGE_INDUSTRIAL) {
            inst(HI_COMPANY, at(0.04, 0.3));
            inst(HI_GAZETTE, at(0.08, 0.3));
            if (P.values[VAL_LEARNING] > 0.1) inst(HI_UNIVERSITY, at(0.15, 0.5));
        }
        if (he.age >= AGE_INDUSTRIAL && P.radio && P.radioYear > 0 && P.radioYear >= he.start && P.radioYear <= he.end) inst(HI_BROADCASTER, P.radioYear + 3 + rng.irange(15));
        if (he.age == AGE_MODERN) inst(HI_NETWORK, at(0.5, 0.75));
        if (he.age == AGE_FUTURISTIC) inst(HI_MIND, at(0.0, 0.15));
        if (he.govChange > 0) {
            if (he.government == GOV_COUNCIL) inst(HI_COUNCIL, he.govChange);
            if (he.government == GOV_ASSEMBLY || he.government == GOV_REPUBLIC) inst(HI_ASSEMBLY, he.govChange);
        } else if (k == 0 || H.eras[k - 1].age == AGE_PREHISTORIC) {
            if (he.government == GOV_COUNCIL) inst(HI_COUNCIL, std::max(1, he.start + 2));
            if (he.government == GOV_ASSEMBLY || he.government == GOV_REPUBLIC) inst(HI_ASSEMBLY, std::max(1, he.start + 2));
        }
    }
    // the faiths: a new one preached where an era's faith was not the one before
    for (size_t k = 1; k < H.eras.size(); k++) {
        const HistoryEra& he = H.eras[k];
        if (he.faith == H.eras[k - 1].faith) continue;
        const int len = he.end - he.start + 1;
        const int y = he.start + (int)std::lround(len * rng.range(-0.04, 0.18));
        if (y >= 1 && y < P.collapseYear) planned.push_back({y, HE_FAITH, 0, he.faith});
    }
    // the discoveries
    int firstHist = 0; while (firstHist + 1 < (int)H.eras.size() && H.eras[firstHist].age == AGE_PREHISTORIC && H.age != AGE_PREHISTORIC) firstHist++;
    for (size_t k = 0; k < H.eras.size(); k++) {
        const HistoryEra& he = H.eras[k];
        const int len = he.end - he.start + 1;
        const int stop = std::min(he.end, P.collapseYear - 1);
        auto at = [&](double lo, double hi) { return he.start + (int)std::lround(len * rng.range(lo, hi)); };
        auto disc = [&](int kind, int y) { if (y >= 1 && y <= stop) planned.push_back({y, HE_DISCOVERY, kind, 0}); };
        if (he.age == AGE_MEDIEVAL) {
            if (!H.places.empty() && H.places[0].river && rng.chance(0.6)) disc(1, at(0.15, 0.6));
            if (rng.chance(0.45)) disc(2, at(0.1, 0.7));
            if (P.values[VAL_LEARNING] > 0.15) { const double full = std::max(1.0, P.fullYears[AGE_MEDIEVAL]); disc(3, he.start + (int)std::lround(full * rng.range(0.7, 0.95))); }
        }
        if (he.age == AGE_INDUSTRIAL) { if (P.landShare < 0.85) disc(4, at(0.3, 0.6)); disc(5, at(0.55, 0.95)); }
        if (he.age == AGE_FUTURISTIC) disc(8, at(0.25, 0.6));
    }
    // the far shore: the first place founded over the water from an older one
    if (PP) {
        int best = -1; int by = INT_MAX;
        for (size_t e = 0; e < PP->edges.size(); e++) {
            const PlanEdge& pe = PP->edges[e];
            if (pe.kind != NK_SEA) continue;
            const int a = pe.a, b = pe.b;
            const int later = H.places[a].founded > H.places[b].founded ? a : b;
            if (H.places[later].founded < by && H.places[later].founded > 1) { by = H.places[later].founded; best = later; }
        }
        if (best >= 0) planned.push_back({std::max(1, by - 1 - rng.irange(4)), HE_DISCOVERY, 0, best});
    }
    // the ways: the year each opened
    if (PP) {
        const int railway = inventionIndex("the railway");
        int railYear = 0;
        for (const Planned& q : planned) if (q.kind == HE_INVENTION && q.a == railway) railYear = q.year;
        int indStart = 0, modStart = 0, futStart = 0;
        for (const HistoryEra& he : H.eras) { if (he.age == AGE_INDUSTRIAL) indStart = he.start; if (he.age == AGE_MODERN) modStart = he.start; if (he.age == AGE_FUTURISTIC) futStart = he.start; }
        for (size_t e = 0; e < PP->edges.size(); e++) {
            const PlanEdge& pe = PP->edges[e];
            const HistoryPlace& A = H.places[pe.a]; const HistoryPlace& B = H.places[pe.b];
            int y = std::max(A.founded, B.founded) + 1;
            int lo = 1, hi = 30;
            switch (pe.kind) {
                case NK_RAIL: y = std::max(y, railYear > 0 ? railYear : indStart + 10); hi = 40; break;
                case NK_CANAL: y = std::max(y, indStart > 0 ? indStart : y); hi = 60; break;
                case NK_HIGHWAY: y = std::max(y, modStart > 0 ? modStart + 5 : y); hi = 30; break;
                case NK_MAGLEV: y = std::max(y, futStart > 0 ? futStart + 15 : y); hi = 120; break;
                default: break;
            }
            y += lo + rng.irange(hi);
            const int stop = std::min(std::min(A.ended, B.ended), P.collapseYear - 1);
            if (y > stop) y = std::max(std::max(A.founded, B.founded) + 1, stop);
            if (y > std::min(A.ended, B.ended)) y = std::max(A.founded, B.founded);
            H.wayOpened[e] = y;
            const bool great = big(pe.a) && big(pe.b);
            if ((pe.trunk && great) || pe.kind == NK_CANAL || pe.kind == NK_RAIL || pe.kind == NK_MAGLEV || (pe.kind == NK_HIGHWAY && great)) waysByYear[y].push_back((int)e);
        }
    }
    // the collapse's signs: one or two in each stage, at a place by the cause's order
    for (int s = 0; s < STAGE_COUNT; s++) {
        if (s == STAGE_ONSET && SCRIPTS[H.cause].hazard == HZ_FIRE) continue;   // its harm is its sign
        const int from = H.stageYear[s], to = s + 1 < STAGE_COUNT ? std::max(from, H.stageYear[s + 1] - 1) : H.last;
        if (s == STAGE_END) { planned.push_back({H.last, HE_SIGN, s * 2 + rng.irange(2), s}); continue; }   // the end's sign closes the last year
        const int k = 1 + (to - from >= 6 ? 1 : 0);
        for (int t = 0; t < k; t++) planned.push_back({from + (to - from) * t / std::max(1, k) + (t > 0 ? rng.irange(std::max(1, (to - from) / 3)) : 0), HE_SIGN, s * 2 + (t + rng.irange(2)) % 2, s});
    }
    std::stable_sort(planned.begin(), planned.end(), [](const Planned& a, const Planned& b) { return a.year < b.year; });
    // the foundings, the clearings and the festivals by year; the year a place would have been left with no event to blame (an
    // exponential wait from its fortieth year at its rate: a band's seat, a mine town, a town on poor ground, a river's town)
    for (int p = 0; p < (int)H.places.size(); p++) {
        const HistoryPlace& hp = H.places[p];
        foundByYear[hp.founded].push_back(p);
        if (hp.fate == HP_CLEARED) clearByYear[hp.ended].push_back(p);
        const bool capital = p == 0 || p == seatOld;
        if (capital) for (int y = hp.founded + 100; y <= std::min(hp.ended, H.last); y += 100) festByYear[y].push_back(p);
        if (p == 0 || hp.plan < 0) continue;
        double rate = 0; int why = 0;
        const PlanPlace& pl = PP->places[hp.plan];
        if (H.age == AGE_PREHISTORIC) { rate = 1 / 2500.0; why = 6; }
        else if (hp.age == AGE_INDUSTRIAL && G->res[pl.cell] > 0.5) { rate = 1 / 1500.0; why = 3; }
        else if (hp.age == AGE_MEDIEVAL && pl.score < 0.3) { rate = 1 / 6000.0; why = 4; }
        else if (hp.river) { rate = 1 / 12000.0; why = 1; }
        if (hp.sclass == SC_CITY) rate *= 0.3;
        if (rate <= 0) continue;
        const double u = unitFromHash(mix64(H.seed ^ (0x40E1ULL + p)));
        const double wait = -std::log(1 - u * 0.999999) / rate;
        if (wait > 1e6) continue;
        const int y = hp.founded + 40 + (int)wait;
        if (y < P.collapseYear - 2) naturalEnds[y].push_back({p, why});
    }
    // the people's number: from the old time through each era's close to the peak at the warning, then to nothing at the end
    static const double DENS[AGE_COUNT] = {0.045, 4.9, 17.9, 42, 31.6, 7.1};
    const double peak = std::max(100.0, P.peak);
    anchors.clear();
    if (H.age == AGE_PREHISTORIC) { anchors.push_back({H.first, peak * 0.05}); anchors.push_back({1, peak * 0.15}); }
    else {
        anchors.push_back({H.first, peak * DENS[AGE_PREHISTORIC] / DENS[H.age] * 0.08});
        for (size_t k = 0; k + 1 < H.eras.size(); k++) anchors.push_back({H.eras[k].end, peak * DENS[H.eras[k].age] / DENS[H.age]});
    }
    if (H.stageYear[STAGE_WARNING] > anchors.back().first) anchors.push_back({H.stageYear[STAGE_WARNING], peak});
    anchors.push_back({H.last + 1, peak * 0.001});
}

// the old time before year 1, as the people remembered it (the years round)
void Builder::oldTime() {
    const int first = H.first;
    if (first >= 1) return;
    const int len = 1 - first;
    auto round = [&](double y) { const int step = len > 3000 ? 100 : (len > 600 ? 50 : 10); return std::min(-step, (int)(std::lround(y / step) * step)); };
    // whence they came: over the sea to an island people, down the rivers, from beyond the mountains, out of a quarter of the sky
    int whence = 2 + rng.irange(4);
    if (!H.places.empty()) {
        if (H.places[0].coast && rng.chance(0.4)) whence = 1;
        else if (H.places[0].river && rng.chance(0.4)) whence = 6;
        else if (W.desert && rng.chance(0.4)) whence = 7;
        else if (rng.chance(0.3)) whence = 0;
    }
    int e = ev(HE_COMING, first, -1, 0.6f, -1); E(e).thing = whence;
    const int eco = H.eras.front().economy;
    e = ev(HE_FIRST, round(first + len * rng.range(0.3, 0.5)), -1, 0.4f, -1); E(e).thing = eco == ECO_HERDING ? 1 : 0;
    if (!H.places.empty() && H.places[0].coast) { e = ev(HE_FIRST, round(first + len * rng.range(0.5, 0.7)), -1, 0.3f, -1); E(e).thing = 2; }
    e = ev(HE_FIRST, round(first + len * rng.range(0.55, 0.8)), -1, 0.3f, -1); E(e).thing = rng.chance(0.5) ? 3 : 4;
    // a great flood, a long drought or a long winter remembered
    const int k = W.desert ? HE_DROUGHT : (W.cold + W.ecc > 0.5 ? HE_WINTER : HE_FLOOD);
    e = ev(k, round(first + len * rng.range(0.1, 0.9)), -1, 0.45f, -1); E(e).value = k == HE_FLOOD ? 0 : 3 + rng.irange(30); E(e).thing = 0;
    if (H.age != AGE_PREHISTORIC && !H.places.empty()) {
        // the stones raised at the capital's site, and the first village there
        const int legend = person(round(first + len * 0.7) - 40, rng.chance(0.5), -1, -1, HR_MAKER, 0);
        H.persons[legend].died = round(first + len * 0.7) + 10;
        e = ev(HE_STONES, round(first + len * rng.range(0.65, 0.8)), -1, 0.45f, -1); E(e).a = legend; E(e).thing = gene.of(GV_MONUMENTS);
        if (E(e).year > H.persons[legend].died || E(e).year < H.persons[legend].born) E(e).year = H.persons[legend].born + 30;
        e = ev(HE_FIRST, -10 * (5 + rng.irange(55)), -1, 0.5f, -1); E(e).thing = 5;
    }
}

void Builder::randomEvents(int y) {
    const PeopleProfile& P = H.profile;
    const int a = ageAt(y);
    const int s = seat(y);
    const double* v = P.values;
    const int stage = P.stageAt(y);
    const bool calm = stage < STAGE_ONSET;
    const int standingN = (int)(standBig.size() + standSmall.size()), bigN = (int)standBig.size();   // a few left lately among them
    // war
    {
        static const double RATE[AGE_COUNT] = {1 / 90.0, 1 / 60.0, 1 / 50.0, 1 / 70.0, 1 / 400.0, 1 / 3000.0};
        static const double GOVF[GOV_COUNT] = {1.2, 1.2, 0.8, 0.9, 0.8, 0.7, 0.8, 0.2};
        double rate = RATE[a] * (0.4 + 2.5 * v[VAL_WAR]) * GOVF[govNow];
        if (reign >= 0 && H.reigns[reign].ruler >= 0 && (H.persons[H.reigns[reign].ruler].traits & (HT_CRUEL | HT_AMBITIOUS))) rate *= 1.4;
        if (calm && standingN >= 2 && warsOn.size() < 2 && rng.chance(rate)) {
            int kind = a == AGE_PREHISTORIC ? HW_BANDS : (a == AGE_MEDIEVAL ? (rng.chance(0.55) ? HW_REBELLION : HW_CITIES) : (rng.chance(0.6) ? HW_CIVIL : HW_REBELLION));
            int A = -1, Bp = -1;
            if (kind == HW_BANDS || kind == HW_CITIES) {
                A = pickPlace(y, [&](int p) { return big(p) ? 2.0 : 1.0; });
                Bp = A >= 0 ? pickPlace(y, [&](int p) { return p != A ? 1.0 / (1.0 + distKm(A, p) / 80.0) : 0.0; }) : -1;
                if (Bp < 0) { kind = HW_REBELLION; A = -1; }
            }
            if (kind == HW_REBELLION || kind == HW_CIVIL) {
                A = -1;
                Bp = pickPlace(y, [&](int p) { return p == s ? 0.0 : (H.places[p].plan >= 0 ? 0.3 + PP->places[H.places[p].plan].remote : 0.5) * (big(p) ? 2.0 : 1.0); });
            }
            if (Bp >= 0) startWar(y, kind, A, Bp, -1, -1);
        }
    }
    // sickness
    {
        static const double RATE[AGE_COUNT] = {1 / 200.0, 1 / 85.0, 1 / 70.0, 1 / 150.0, 1 / 700.0, 1 / 6000.0};
        if (calm && rng.chance(RATE[a] * (0.6 + 1.5 * v[VAL_TRADE]) * (1 + 0.01 * std::min(bigN, 40)))) {
            int sk = -1;
            for (int k = (int)H.sicknesses.size() - 1; k >= 0 && sk < 0; k--) if (y - H.sicknesses[k].last < 220 && !H.sicknesses[k].made && rng.chance(0.35)) sk = k;
            if (sk < 0) { HistorySickness S; S.name = mix64(H.seed ^ (0x405CULL + H.sicknesses.size() * 0x9E3779B97F4A7C15ULL)); S.first = y; H.sicknesses.push_back(S); sk = (int)H.sicknesses.size() - 1; }
            const int origin = pickPlace(y, [&](int p) { return (H.places[p].coast ? 2.0 : 1.0) * (big(p) ? 3.0 : 1.0); });
            const int n = 1 + (rng.chance(0.4) ? 1 + rng.irange(3) : 0);
            int wy = y;
            for (int k = 0; k < n; k++) { waves.push_back({wy, sk, origin, k + 1, rng.range(0.004, 0.05), -1}); wy += 1 + rng.irange(12); }
        }
    }
    // the land and the sky
    const double wet = std::max(0.0, W.wet);
    if (rng.chance(1 / 35.0 * wet)) {
        const int p = pickPlace(y, [&](int q) { return H.places[q].river ? 2.0 : (H.places[q].coast ? 1.0 : 0.0); });
        if (p >= 0) {
            const int64_t dead = (int64_t)(placePop(p, y) * rng.range(0.001, 0.03));
            const int e = ev(HE_FLOOD, y, p, 0.4f); E(e).value = dead; E(e).thing = H.places[p].river ? 0 : 1;
            harm(p, dead / placePop(p, y), e, 5);
            maybeLeave(p, y, e, H.places[p].sclass == SC_TOWN || H.places[p].sclass == SC_VILLAGE ? 0.04 : 0.01);
        }
    }
    int drought = 0;
    if (a <= AGE_INDUSTRIAL && rng.chance((W.desert ? 1 / 60.0 : 1 / 140.0) * (1.2 - std::min(1.0, wet)))) {
        drought = 1 + rng.irange(4);
        const int e = ev(HE_DROUGHT, y, -1, 0.4f); E(e).value = drought;
    }
    if (rng.chance(1 / 130.0 * (0.25 + W.volc))) {
        const int p = pickPlace(y, [&](int q) { return H.places[q].plan >= 0 ? 0.3 + G->relief[PP->places[H.places[q].plan].cell] : 0.5; });
        if (p >= 0) {
            const int64_t dead = (int64_t)(placePop(p, y) * rng.range(0.002, 0.05));
            const int e = ev(HE_QUAKE, y, p, 0.45f); E(e).value = dead;
            harm(p, dead / placePop(p, y), e, 6);
            maybeLeave(p, y, e, 0.05);
        }
    }
    int eruption = -1;
    if (W.volc > 0 && rng.chance(1 / 700.0 * W.volc)) {
        const int p = pickPlace(y, [&](int q) { return H.places[q].plan >= 0 ? G->relief[PP->places[H.places[q].plan].cell] : 0.3; });
        if (p >= 0) {
            const int64_t dead = (int64_t)(placePop(p, y) * rng.range(0.01, 0.2));
            eruption = ev(HE_ERUPTION, y, p, 0.6f); E(eruption).value = dead;
            harm(p, dead / placePop(p, y), eruption, 12);
            maybeLeave(p, y, eruption, 0.2);
        }
    }
    int impact = -1;
    if (calm && rng.chance(1 / 9000.0 * (0.3 + W.impacts))) {
        const int p = anyPlace(y, 1);
        const int out = rng.chance(0.6) ? 1 : (rng.chance(0.7) ? 0 : 2);
        const int64_t dead = (int64_t)(out == 2 ? pop(y) * rng.range(0.02, 0.08) : placePop(p, y) * (out == 0 ? 0.5 : rng.range(0.01, 0.1)));
        impact = ev(HE_IMPACT, y, p, out == 2 ? 0.85f : 0.65f); E(impact).value = dead; E(impact).out = out;
        harm(out == 2 ? -1 : p, out == 2 ? dead / pop(y) : dead / placePop(p, y), impact, 12);
        deficit += dead / pop(y);
        if (out == 0 && p >= 0) maybeLeave(p, y, impact, 0.6);
    }
    if (W.redDwarf && rng.chance(1 / 250.0)) {
        const int64_t dead = (int64_t)(pop(y) * rng.range(0.0005, 0.01));
        const int e = ev(HE_FLARE, y, -1, 0.55f); E(e).value = dead;
        harm(-1, dead / pop(y), e, 12);
    }
    if ((eruption >= 0 && rng.chance(0.4)) || (impact >= 0 && E(impact).out == 2) || rng.chance(1 / 350.0 * (W.cold + 2 * W.ecc))) {
        const int e = ev(HE_WINTER, y, -1, 0.5f); E(e).value = 1 + rng.irange(eruption >= 0 || impact >= 0 ? 3 : 6); E(e).cause = eruption >= 0 ? eruption : impact;
        drought = std::max(drought, (int)E(e).value);
    }
    // the harvest (the ages that lived by their fields and herds), its failures and the famines after them
    const int yi = y - H.yearsFrom;
    if (yi >= 0 && yi < (int)H.harvest.size()) {
        double q = 2.6 + 1.25 * ((rng.uni() + rng.uni() + rng.uni()) - 1.5);
        if (drought > 0) q -= 2.2;
        if (eruption >= 0) q -= 1.2;
        if (impact >= 0) q -= 2.5;
        if (!warsOn.empty()) q -= 0.3;
        if (stage >= 0) {
            const int hzd = SCRIPTS[H.cause].hazard;
            const bool fields = hzd == HZ_FAMINE || hzd == HZ_COLD || hzd == HZ_DRYING || hzd == HZ_IMPACT || hzd == HZ_FIRE || H.cause == CAUSE_CLIMATE || H.cause == CAUSE_WATER_WAR || H.cause == CAUSE_POISONED;
            if (fields) { q -= 2.5 * clampd((y - H.stageYear[0]) / (double)std::max(1, H.last - H.stageYear[0]), 0, 1) + (stage >= STAGE_ONSET ? 0.6 : 0); q = std::min(q, 2.4); }
        }
        const int hv = clampi((int)std::lround(q), 0, 4);
        H.harvest[yi] = (uint8_t)hv;
        const bool fields = a <= AGE_INDUSTRIAL;
        const int prev = yi > 0 ? H.harvest[yi - 1] : 2;
        if (fields && (hv == 0 || (hv == 1 && prev <= 1))) {
            const bool famine = y - lastFamine >= (stage >= 0 ? 3 : 2) && rng.chance(hv == 0 ? 0.75 : 0.5);
            int he = -1;
            if ((hv == 0 && stage < STAGE_ONSET) || famine) { he = ev(HE_HARVEST, y, -1, 0.25f); E(he).out = hv; }
            if (famine) {
                static const double F[AGE_COUNT] = {1, 1, 0.3, 0.05, 0.02, 0.01};
                const int at = rng.chance(0.4) ? anyPlace(y) : -1;
                const int64_t dead = (int64_t)(pop(y) * rng.range(0.002, 0.025) * F[a]);
                const int fe = ev(HE_FAMINE, y, at, 0.5f); E(fe).value = dead; E(fe).cause = he;
                lastFamine = y;
                harm(-1, dead / pop(y), fe, 4);
                if (stage >= STAGE_ONSET) lastHazardEv = fe;
                deficit += dead / pop(y);
                if (bigN > 0 && rng.chance(0.3)) { const int rp = pickPlace(y, [&](int p) { return big(p) ? 1.0 : 0.0; }); const int u = ev(HE_UNREST, y, rp, 0.3f); E(u).thing = 0; E(u).cause = fe; E(u).value = rng.irange(40); }
            }
        } else if (fields && hv == 4 && rng.chance(0.2)) { const int he = ev(HE_HARVEST, y, -1, 0.1f); E(he).out = 4; }
    }
    // the works
    {
        static const double RATE[AGE_COUNT] = {1 / 50.0, 1 / 22.0, 1 / 9.0, 1 / 8.0, 1 / 25.0, 1 / 400.0};
        if (calm && standingN > 0 && rng.chance(RATE[a] * (0.6 + 0.01 * std::min(bigN, 60)))) {
            double w[WK_COUNT];
            const int railway = inventionIndex("the railway"), radio = inventionIndex("radio"), rocket = inventionIndex("the rocket"), habitat = inventionIndex("the first habitat"), elevator = inventionIndex("the elevator to orbit");
            bool anyRiver = false, anyCoast = false;
            for (int p = 0; p < (int)H.places.size(); p++) if (stands(p, y)) { anyRiver |= H.places[p].river; anyCoast |= H.places[p].coast; }
            for (int k = 0; k < WK_COUNT; k++) {
                const WorkInfo& I = WORKS[k];
                w[k] = 0;
                if (!(I.ages & ageBit(a))) continue;
                double x = 0.2;
                for (int vv = 0; vv < VAL_COUNT; vv++) x += I.w[vv] * v[vv];
                switch (I.need) {
                    case ND_RIVER: if (!anyRiver) x = 0; break;
                    case ND_COAST: if (!anyCoast) x = 0; break;
                    case ND_RAILWAY: if (H.inventionYear(railway) == 0 || H.inventionYear(railway) > y) x = 0; break;
                    case ND_RADIO: if (H.inventionYear(radio) == 0 || H.inventionYear(radio) > y) x = 0; break;
                    case ND_ROCKET: if (H.inventionYear(rocket) == 0 || H.inventionYear(rocket) > y) x = 0; break;
                    case ND_HABITAT: if (H.inventionYear(habitat) == 0 || H.inventionYear(habitat) > y) x = 0; break;
                    case ND_ELEVATOR: if (H.inventionYear(elevator) == 0 || H.inventionYear(elevator) > y) x = 0; break;
                    case ND_MONARCH: if (govNow != GOV_MONARCHY) x = 0; break;
                    case ND_DEAD: if (gene.of(GV_DEAD) == DE_NOTHING) x = 0; break;
                    default: break;
                }
                if (k == WK_GREAT) x = 0;   // the far future's works are its plan's (founded, then finished)
                w[k] = std::max(0.0, x);
            }
            double tot = 0; for (int k = 0; k < WK_COUNT; k++) tot += w[k];
            if (tot > 0) {
                const int k = rng.pick(w, WK_COUNT);
                const WorkInfo& I = WORKS[k];
                const int p = pickPlace(y, [&](int q) {
                    if (I.need == ND_RIVER && !H.places[q].river) return 0.0;
                    if (I.need == ND_COAST && !H.places[q].coast) return 0.0;
                    return q == s ? 4.0 : (big(q) ? 2.0 : 0.5);
                });
                if (p >= 0) {
                    const int years = (int)std::lround(std::exp(rng.range(std::log(I.years[0]), std::log(I.years[1]))));
                    int maker = -1;
                    if ((k == WK_PALACE || k == WK_WALLS || k == WK_FOCUS) && reign >= 0 && H.reigns[reign].ruler >= 0 && rng.chance(0.5)) maker = H.reigns[reign].ruler;
                    else maker = figure(y, HR_MAKER, p, 30, 60);
                    const int e = ev(HE_BUILT, y, p, k == WK_FOCUS || k == WK_TEMPLE || k == WK_ARCOLOGY || k == WK_RING ? 0.55f : 0.35f);
                    E(e).thing = k; E(e).a = maker; E(e).value = years;
                }
            }
        }
    }
    // the far future's works finished (begun at their founding)
    if (H.age == AGE_FAR_FUTURE) for (int p = 0; p < (int)H.places.size(); p++) {
        if (H.places[p].plan < 0) continue;
        const int done = H.places[p].founded + 60 + (int)(unitFromHash(mix64(H.seed ^ (0x40F7ULL + p))) * 500);
        if (y == std::min(done, std::max(H.places[p].founded + 1, P.collapseYear - 1)) && stands(p, y)) {
            const int e = ev(HE_BUILT, y, p, 0.8f); E(e).thing = WK_GREAT; E(e).value = y - H.places[p].founded; E(e).a = figure(y, HR_MAKER, p, 40, 200, false);
        }
    }
    // the sky
    if (cometPeriod > 0 && (y - cometPhase) % cometPeriod == 0) { const int e = ev(HE_OMEN, y, s, 0.12f); E(e).thing = 0; }
    if (body.moonCount > 0 && rng.chance(1 / 150.0)) { const int e = ev(HE_OMEN, y, anyPlace(y), 0.12f); E(e).thing = rng.chance(0.6) ? 1 : 5; }
    if (auroral && rng.chance(1 / 120.0)) { const int e = ev(HE_OMEN, y, anyPlace(y), 0.1f); E(e).thing = 2; }
    if (rng.chance(1 / 250.0)) { const int e = ev(HE_OMEN, y, anyPlace(y), 0.1f); E(e).thing = 3; }
    if (rng.chance(1 / 5000.0)) { const int e = ev(HE_OMEN, y, -1, 0.3f); E(e).thing = 4; }
    // deeds
    {
        static const double RATE[AGE_COUNT] = {1 / 80.0, 1 / 35.0, 1 / 25.0, 1 / 25.0, 1 / 50.0, 1 / 500.0};
        if (calm && standingN > 0 && rng.chance(RATE[a] * (0.6 + v[VAL_ART] + v[VAL_LEARNING]))) {
            static const unsigned DA[13] = {AP | AM | AI | AMO | AF, AM | AI, AM | AI, AM | AI, AP, AM | AI, AP | AM | AI, AP | AM, AM | AI | AMO, AI | AMO, AI | AMO | AF, AMO, AM | AI | AMO | AF};
            const double DW[13] = {v[VAL_ART] * 2, v[VAL_LEARNING], 0.4, v[VAL_LEARNING], 0.6, v[VAL_TRADE], 0.4, v[VAL_ART], v[VAL_LEARNING], v[VAL_ART], v[VAL_LEARNING], v[VAL_ART], v[VAL_ART] * 0.5};
            double w[13]; double tot = 0;
            for (int k = 0; k < 13; k++) { w[k] = (DA[k] & ageBit(a)) ? DW[k] + 0.05 : 0; tot += w[k]; }
            if (tot > 0) {
                int k = rng.pick(w, 13);
                if (k == 2 && (reign < 0 || H.reigns[reign].ruler < 0 || lawGivers.count(H.reigns[reign].ruler))) k = 0;
                const int p = anyPlace(y);
                int who;
                if (k == 2) { who = H.reigns[reign].ruler; lawGivers.insert(who); }
                else who = figure(y, k == 3 || k == 5 ? HR_EXPLORER : (k == 6 || k == 8 || k == 10 || k == 1 ? HR_MAKER : HR_ARTIST), p, 22, 60);
                const int e = ev(HE_DEED, y, p, 0.3f); E(e).a = who; E(e).thing = k;
            }
        }
    }
    // unrest: strikes in the mill towns, risings under a cruel ruler
    {
        double rate = a == AGE_INDUSTRIAL ? 1 / 12.0 : (a == AGE_MODERN ? 1 / 20.0 : (a == AGE_MEDIEVAL ? 1 / 90.0 : 0));
        if (reign >= 0 && H.reigns[reign].ruler >= 0 && (H.persons[H.reigns[reign].ruler].traits & HT_CRUEL)) rate += 1 / 60.0;
        if (calm && rng.chance(rate)) {
            const int p = pickPlace(y, [&](int q) { return big(q) ? 2.0 : 0.5; });
            if (p >= 0) {
                const int k = a >= AGE_INDUSTRIAL ? (rng.chance(0.7) ? 1 : 3) : 2;
                const int e = ev(HE_UNREST, y, p, 0.3f); E(e).thing = k; E(e).value = k == 1 ? 0 : rng.irange(60);
            }
        }
    }
    // the countryside to the cities, once in the industrial age
    if (a == AGE_INDUSTRIAL) {
        const HistoryEra& he = H.eras[H.eraAt(y)];
        if (y == he.start + (he.end - he.start) / 3 && bigN > 0) {
            const int to = s >= 0 ? s : anyPlace(y);
            const int e = ev(HE_MIGRATION, y, -1, 0.5f); E(e).place2 = to; E(e).value = (int64_t)(pop(y) * rng.range(0.05, 0.2)); E(e).thing = 0;
        }
    }
    // a schism or a reform of the rites now and then
    {
        static const double RATE[AGE_COUNT] = {1 / 400.0, 1 / 150.0, 1 / 200.0, 1 / 300.0, 1 / 600.0, 0};
        if (calm && rng.chance(RATE[a] * (0.3 + 3 * v[VAL_PIETY]))) {
            const int k = rng.chance(0.55) ? 1 : (rng.chance(0.6) ? 2 : 4);
            const int p = anyPlace(y);
            const int e = ev(HE_FAITH, y, p, 0.35f); E(e).thing = k; E(e).value = H.eras[H.eraAt(y)].faith;
            if (k == 2) E(e).a = figure(y, HR_PROPHET, p, 30, 60);
        }
    }
}

// H-04's second pass: the everyday of a year, from its own stream (the year's and the people's seed): what the news, the annals and the
// letters of a quiet year are made of. Nothing here harms anyone or makes a person, so the history above is H-02's whatever is drawn here
void Builder::everyday(int y) {
    Rng dr(mix64(H.seed ^ 0xDA11E5ULL ^ (uint64_t)(uint32_t)y * 0x9E3779B97F4A7C15ULL));
    const int a = ageAt(y);
    const int stage = H.profile.stageAt(y);
    const double busy = stage >= STAGE_CRISIS ? 0.35 : (stage >= 0 ? 0.7 : 1.0);   // the collapse's years have less of it
    bool coast = false, river = false, rail = false, mineTown = false, contact = false;
    for (int p = 0; p < (int)H.places.size(); p++) if (stands(p, y)) { coast |= H.places[p].coast; river |= H.places[p].river; }
    const int railway = inventionIndex("the railway"), flight = inventionIndex("flight"), reactor = inventionIndex("the reactor"), habitat = inventionIndex("the first habitat");
    auto had = [&](int inv) { const int iy = inv >= 0 ? H.inventionYear(inv) : 0; return iy > 0 && iy <= y; };
    rail = had(railway);
    for (; evScan < H.events.size(); evScan++) { const HistoryEvent& e = H.events[evScan]; if (e.kind == HE_CONTACT) everContact = true; if (e.kind == HE_BUILT && e.thing == WK_MINE) everMine = true; }
    contact = everContact; mineTown = everMine;
    const bool fields = H.profile.economy == ECO_FARMING || H.profile.economy == ECO_HERDING || H.profile.economy2 == ECO_FARMING || H.profile.economy2 == ECO_HERDING || a <= AGE_MEDIEVAL;
    const bool cold = W.cold > 0.3 || H.seasons == 4, desert = W.desert;
    auto fits = [&](const EverydayWord& w) {
        if (!(w.ages & ageBit(a))) return false;
        switch (w.need) {
            case 1: return coast; case 2: return coast || river; case 3: return rail; case 4: return had(flight); case 5: return had(reactor); case 6: return had(habitat);
            case 7: return cold; case 8: return desert; case 9: return fields; case 10: return contact && !H.lore.other.empty(); case 11: return mineTown; case 12: return !desert;
            default: return true;
        }
    };
    auto placeBy = [&](double bigW, bool coastOnly, bool riverOrCoast) {   // a standing place by weight, from the year's own stream
        double tot = 0;
        for (int p = 0; p < (int)H.places.size(); p++) {
            if (!stands(p, y) || (coastOnly && !H.places[p].coast) || (riverOrCoast && !H.places[p].coast && !H.places[p].river)) continue;
            tot += big(p) ? bigW : 1.0;
        }
        if (tot <= 0) return -1;
        double u = dr.uni() * tot;
        for (int p = 0; p < (int)H.places.size(); p++) {
            if (!stands(p, y) || (coastOnly && !H.places[p].coast) || (riverOrCoast && !H.places[p].coast && !H.places[p].river)) continue;
            u -= big(p) ? bigW : 1.0;
            if (u < 0) return p;
        }
        return -1;
    };
    auto someone = [&](int minAge, int maxAge, int home) {   // a person alive in the year, of an age, at a place where one is asked (none: -1)
        int n = 0, pick = -1;
        for (int i = 0; i < (int)H.persons.size(); i++) {
            const HistoryPerson& P = H.persons[i];
            if (P.born > y || P.died < y || y - P.born < minAge || y - P.born > maxAge || P.role == HR_RULER || P.role == HR_REGENT) continue;
            if (home >= 0 && P.home != home) continue;
            if (dr.irange(++n) == 0) pick = i;   // a reservoir of one
        }
        return pick;
    };
    const int ruler = reign >= 0 && !vacant && H.reigns[reign].ruler >= 0 && H.persons[H.reigns[reign].ruler].died >= y && H.persons[H.reigns[reign].ruler].born <= y ? H.reigns[reign].ruler : -1;
    for (int k = 0; k < EVERYDAY_N; k++) {
        const EverydayKind& K = EVERYDAY[k];
        if (!dr.chance(K.rate[a] * busy)) continue;
        double w[24]; int n = std::min(K.n, 24); double tot = 0;
        for (int i = 0; i < n; i++) { w[i] = fits(K.words[i]) ? 1.0 : 0.0; tot += w[i]; }
        if (tot <= 0) continue;
        const int thing = dr.pick(w, n);
        const char* key = K.words[thing].key;
        const int need = K.words[thing].need;
        int place = placeBy(K.kind == HE_FAIR || K.kind == HE_SHOW || K.kind == HE_GAMES || K.kind == HE_TRIAL || K.kind == HE_LAW ? 3.0 : 1.5, need == 1, need == 2);
        if (K.kind == HE_LAW || K.kind == HE_EMBASSY) place = seat(y);
        if (place < 0 && K.kind != HE_STORM && K.kind != HE_PRICE && K.kind != HE_LAW) continue;
        const int season = dr.irange(H.seasons);
        const int e = ev(K.kind, y, place, K.sal, season);
        E(e).thing = thing;
        switch (K.kind) {
            case HE_FAIR: {   // how many came: a share of the place, at most what a fair of the age drew
                static const double CAP[AGE_COUNT] = {600, 30000, 120000, 300000, 200000, 50000};
                E(e).value = (int64_t)std::min(placePop(place, y) * dr.range(0.05, 0.6), CAP[a] * dr.range(0.3, 1.0));
                break;
            }
            case HE_FIRE: {
                const double share = std::strcmp(key, "half") == 0 ? dr.range(0.3, 0.6) : (std::strcmp(key, "street") == 0 ? dr.range(0.01, 0.08) : dr.range(0.002, 0.02));
                E(e).value = std::max<int64_t>(1, (int64_t)(placePop(place, y) / 5.0 * share));
                if (std::strcmp(key, "half") == 0) E(e).salience = 0.45f;
                break;
            }
            case HE_STORM: E(e).value = dr.chance(0.6) ? 0 : 1 + dr.irange(a >= AGE_MODERN ? 40 : 12); E(e).season = std::strcmp(key, "snow") == 0 ? (H.seasons == 4 ? 3 : season) : season; break;
            case HE_PEST: if (std::strcmp(key, "locusts") == 0 || std::strcmp(key, "blight") == 0) E(e).salience = 0.35f; break;
            case HE_GAMES: E(e).a = someone(16, 34, -1); break;
            case HE_LAW: E(e).a = ruler; break;
            case HE_TRIAL: E(e).a = someone(18, 70, -1); E(e).out = dr.irange(4); if (E(e).a < 0) E(e).out = 1; break;
            case HE_EMBASSY: {
                if (thing == 0) { int from = placeBy(2.0, false, false); if (from == place) from = -1; if (from < 0) { H.events.pop_back(); continue; } E(e).place2 = from; }
                E(e).a = someone(30, 70, -1);
                break;
            }
            case HE_ACCIDENT: E(e).value = std::strcmp(key, "habitat") == 0 || std::strcmp(key, "reactor") == 0 ? 20 + dr.irange(4000) : 1 + dr.irange(std::strcmp(key, "ship") == 0 || std::strcmp(key, "train") == 0 ? 160 : 60);
                if (E(e).value > 100) E(e).salience = 0.5f;
                break;
            case HE_PRICE: {
                const int hv = H.harvestAt(y - 1);
                E(e).out = (std::strcmp(key, "bread") == 0 || std::strcmp(key, "meat") == 0) ? (hv <= 1 ? 1 : (hv >= 3 ? 0 : dr.irange(2))) : (dr.chance(0.65) ? 1 : 0);
                E(e).value = 5 + dr.irange(E(e).out ? 60 : 30);
                break;
            }
            case HE_BEASTS: if (std::strcmp(key, "killed") == 0) E(e).value = 1 + dr.irange(3); if (std::strcmp(key, "none") == 0) E(e).salience = 0.4f; break;
            case HE_SHOW: E(e).a = someone(20, 70, place); if (E(e).a < 0) E(e).a = someone(20, 70, -1); break;
            case HE_VISIT: if (ruler < 0 || place == seat(y)) { H.events.pop_back(); continue; } E(e).a = ruler; E(e).thing = 0; break;
            default: break;
        }
    }
}

// H-04's second pass: what a family's letters are made of besides births, weddings, moves and deaths: an illness got over, a hurt, a child
// apprenticed, the house built, sold, burnt or mended, a quarrel with the neighbours, a journey and back, a betrothal before a wedding. Each
// person followed (the families' members) draws from a stream of their own after the years are done, so nothing the years drew moves;
// a person's place is where their birth and their moves put them in that year
void Builder::family() {
    const int np = (int)H.persons.size();
    std::vector<std::vector<std::pair<int, int>>> homes(np);   // (year, place)
    std::vector<int> wedding(np, 0), weddingWith(np, -1);
    for (const HistoryEvent& e : H.events) {
        if (e.a < 0 || e.a >= np) continue;
        if (e.kind == HE_BIRTH && e.place >= 0) homes[e.a].push_back({e.year, e.place});
        else if (e.kind == HE_MOVE && e.place2 >= 0) homes[e.a].push_back({e.year, e.place2});
        else if (e.kind == HE_MARRIAGE) { wedding[e.a] = e.year; weddingWith[e.a] = e.b; }
    }
    for (auto& h : homes) std::sort(h.begin(), h.end());
    auto homeAt = [&](int p, int y) { int h = -1; for (const auto& m : homes[p]) { if (m.first > y) break; h = m.second; } if (h < 0) h = H.persons[p].home; return h >= 0 && stands(h, y) ? h : -1; };
    std::vector<int> adultsAt;   // for a quarrel's other party
    for (int p = 0; p < np; p++) {
        const HistoryPerson& P = H.persons[p];
        if (P.family < 0 || P.family >= (int)H.families.size() || H.families[P.family].house || P.fate == HF_LET_GO || P.role == HR_RULER) continue;   // the families followed
        Rng fr(mix64(H.seed ^ 0xFA111ULL ^ (uint64_t)(p + 1) * 0x9E3779B97F4A7C15ULL));
        const int last = std::min(P.died == ALIVE ? H.last : P.died - 1, H.last);   // the year of a death has its own news
        bool apprenticed = false;
        for (int y = std::max(P.born + 1, H.first); y <= last; y++) {
            if (H.profile.stageAt(y) >= STAGE_CRISIS && fr.chance(0.5)) continue;
            const int age = y - P.born;
            const int home = homeAt(p, y);
            const int season = fr.irange(H.seasons);
            if (home < 0) continue;
            // an illness got over (the young and the old more), a hurt
            if (fr.chance(age < 12 || age > 55 ? 0.03 : 0.012)) { const int e = ev(HE_ILLNESS, y, home, 0.12f, season); E(e).a = p; E(e).thing = fr.irange((int)(sizeof(PRIVATE_ILLNESS) / sizeof(PRIVATE_ILLNESS[0]))); }
            if (age >= 4 && fr.chance(0.01)) { const int e = ev(HE_INJURY, y, home, 0.12f, season); E(e).a = p; E(e).thing = fr.irange((int)(sizeof(PRIVATE_INJURY) / sizeof(PRIVATE_INJURY[0]))); }
            // a child apprenticed once (the ages that had trades to learn)
            if (!apprenticed && age >= 11 && age <= 15 && ageAt(y) >= AGE_MEDIEVAL && fr.chance(0.25)) {
                apprenticed = true;
                const int e = ev(HE_APPRENTICE, y, home, 0.14f, season); E(e).a = p; E(e).thing = ageAt(y) * 10 + fr.irange(10);
            }
            if (age >= 22 && age <= 70) {
                if (P.spouse >= 0 && fr.chance(0.012)) { const int e = ev(HE_HOUSE, y, home, 0.12f, season); E(e).a = p; E(e).thing = fr.irange((int)(sizeof(PRIVATE_HOUSE) / sizeof(PRIVATE_HOUSE[0]))); }
                if (fr.chance(0.01)) {   // a quarrel with someone of another family at the same place
                    int other = -1, n = 0;
                    for (int q = 0; q < np; q++) {
                        const HistoryPerson& Q = H.persons[q];
                        if (q == p || Q.family < 0 || Q.family == P.family || Q.fate == HF_LET_GO || Q.born > y - 20 || (Q.died != ALIVE && Q.died < y) || homeAt(q, y) != home) continue;
                        if (fr.irange(++n) == 0) other = q;
                    }
                    const int e = ev(HE_QUARREL, y, home, 0.12f, season); E(e).a = p; E(e).b = other; E(e).thing = fr.irange((int)(sizeof(PRIVATE_QUARREL) / sizeof(PRIVATE_QUARREL[0])));
                }
            }
            if (age >= 18 && age <= 66 && fr.chance(0.014)) {   // a journey there and back
                int to = -1, n = 0;
                for (int q = 0; q < (int)H.places.size(); q++) if (q != home && stands(q, y) && fr.irange(++n) == 0) to = q;
                if (to >= 0) { const int e = ev(HE_JOURNEY, y, home, 0.12f, season); E(e).a = p; E(e).place2 = to; E(e).thing = fr.irange((int)(sizeof(PRIVATE_JOURNEY) / sizeof(PRIVATE_JOURNEY[0]))); }
            }
        }
        // a betrothal the year before the wedding (once a couple: told for the one the wedding names first)
        if (wedding[p] > 0 && weddingWith[p] >= 0 && wedding[p] - 1 >= std::max(H.first, P.born + 15) && fr.chance(0.7)) {
            const int y = wedding[p] - 1, home = homeAt(p, y);
            if (home >= 0) { const int e = ev(HE_BETROTHAL, y, home, 0.14f, fr.irange(H.seasons)); E(e).a = p; E(e).b = weddingWith[p]; }
        }
    }
}

void Builder::collapseYear(int y) {
    const PeopleProfile& P = H.profile;
    const CauseScript& CS = SCRIPTS[H.cause];
    for (int s = 0; s < STAGE_COUNT; s++) if (y == H.stageYear[s]) {   // a stage that begins in the year of the one before still marks it
        const int e = ev(HE_STAGE, y, -1, 0.8f, -1); E(e).out = s; E(e).thing = H.cause;
        stageEv[s] = e;
        if (s == 0) lastHazardEv = e;
    }
    const int stage = P.stageAt(y);
    if (stage < 0) return;
    // the year's signs: the warning's and the onset's before its harm, the crisis's and the end's after it; each names the one before it
    // (or its stage), so a year's keep their order
    auto signs = [&](int fromStage, int toStage) {
        for (size_t k = plannedAtM; k < planned.size() && planned[k].year <= y; k++) {
            const Planned& q = planned[k];
            if (q.kind != HE_SIGN || q.year != y || q.b < fromStage || q.b > toStage) continue;
            int p = -1;
            for (const auto& kv : endsByYear) { for (int x : kv.second) if (stands(x, y)) { p = x; break; } if (p >= 0) break; }
            if (q.b == STAGE_END) p = H.lastPlace;
            if (p < 0 || !stands(p, y)) p = seat(y);
            const int e = ev(HE_SIGN, y, p, q.b == STAGE_WARNING ? 0.6f : 0.75f); E(e).thing = q.a; E(e).out = q.b;
            E(e).cause = lastSignEv >= 0 && H.events[lastSignEv].year == y ? lastSignEv : stageEv[q.b];
            lastSignEv = e;
        }
    };
    signs(STAGE_WARNING, STAGE_ONSET);
    const int hazardBefore = (int)H.events.size();
    const int s = seat(y);
    const double prog = clampd((y - H.stageYear[0] + 1) / (double)std::max(1, H.last - H.stageYear[0] + 1), 0, 1);
    const int a = ageAt(y);
    auto exposed = [&]() {   // one of the next few places in the cause's order that still stand
        int next[5], n = 0;
        for (const auto& kv : endsByYear) { for (int p : kv.second) if (stands(p, y) && n < 5) next[n++] = p; if (n >= 5) break; }
        return n ? next[rng.irange(n)] : s;
    };
    // the hazard of the cause, from the onset on
    if (stage >= STAGE_ONSET) {
        const bool onset = y == H.stageYear[STAGE_ONSET];
        switch (CS.hazard) {
            case HZ_FAMINE: if (onset) {
                const int he = ev(HE_HARVEST, y, -1, 0.35f); E(he).out = 0;
                const int fe = ev(HE_FAMINE, y, exposed(), 0.65f); E(fe).cause = he; E(fe).value = (int64_t)(pop(y) * rng.range(0.01, 0.06));
                harm(-1, rng.range(0.01, 0.06), fe, 4); lastHazardEv = fe; } break;
            case HZ_COLD: if (onset || rng.chance(0.07)) {
                int cause = -1;
                if (onset && H.cause == CAUSE_VOLCANIC_WINTER) { const int p = exposed(); cause = ev(HE_ERUPTION, y, p, 0.85f); E(cause).value = (int64_t)(placePop(p, y) * rng.range(0.05, 0.3)); harm(p, 0.15, cause, 12); }
                const int e = ev(HE_WINTER, y, -1, 0.6f); E(e).value = onset ? std::max(1, H.last - y + 1) : 1 + rng.irange(4); E(e).cause = cause;
                harm(-1, rng.range(0.005, 0.03), e, 13); lastHazardEv = e; } break;
            case HZ_FLOOD: if (onset || rng.chance(0.3)) {
                const int p = exposed();
                const int e = ev(HE_FLOOD, y, p, 0.65f); E(e).value = (int64_t)(placePop(p, y) * rng.range(0.01, 0.1)); E(e).thing = p >= 0 && H.places[p].coast ? 1 : 0;
                harm(p, 0.05, e, 5); lastHazardEv = e; } break;
            case HZ_SICKNESS: if (onset || rng.chance(0.3)) {
                if (H.sicknesses.empty() || onset) { HistorySickness S; S.name = mix64(H.seed ^ 0x40DEADULL ^ (uint64_t)y); S.first = y; S.made = H.cause == CAUSE_ENGINEERED_PLAGUE; H.sicknesses.push_back(S); }
                const int sk = (int)H.sicknesses.size() - 1;
                const double share = (H.cause == CAUSE_GREAT_SICKNESS || H.cause == CAUSE_ENGINEERED_PLAGUE ? rng.range(0.08, 0.3) : rng.range(0.02, 0.1));
                waves.push_back({y, sk, exposed(), H.sicknesses[sk].waves + 1, share, -1}); } break;
            case HZ_WAR: if (onset) {
                const int kind = H.cause == CAUSE_CONQUEST ? HW_CONQUEST : (H.cause == CAUSE_BAND_WAR ? HW_BANDS : HW_CIVIL);
                bool joint = false;
                for (const HistoryWar& w : H.wars) if (w.kind == HW_CONQUEST && w.sideB == -2) joint = true;
                if (!joint) {
                    const int B = exposed();
                    const int wi = startWar(y, kind, -1, B, -1, -1, lastHazardEv, std::max(1, H.last - y));
                    H.wars[wi].winner = 1;
                    WarRun& run = warsOn.back();
                    run.battles.clear();
                    for (int t = y; t < H.last; t += 1 + rng.irange(3)) run.battles.push_back(t);
                    run.siegeAt = run.battles.empty() ? -1 : run.battles.back();
                }
            } break;
            case HZ_DRYING: if (onset) { const int e = ev(HE_DROUGHT, y, -1, 0.6f); E(e).value = std::max(1, H.last - y + 1); lastHazardEv = e; } break;
            case HZ_IMPACT: if (onset) {
                const int p = anyPlace(y, 1);
                const int e = ev(HE_IMPACT, y, p, 1.0f); E(e).out = 2; E(e).value = (int64_t)(pop(y) * rng.range(0.1, 0.4));
                harm(-1, rng.range(0.1, 0.4), e, 12); deficit += 0.2;
                const int w2 = ev(HE_WINTER, y, -1, 0.7f); E(w2).value = std::max(1, std::min(H.last - y + 1, 3 + rng.irange(8))); E(w2).cause = e;
                lastHazardEv = e; } break;
            case HZ_FLARE: if (onset || rng.chance(0.2)) {
                const int e = ev(HE_FLARE, y, -1, onset ? 1.0f : 0.6f); E(e).value = (int64_t)(pop(y) * (onset ? rng.range(0.1, 0.5) : rng.range(0.01, 0.05)));
                harm(-1, onset ? rng.range(0.1, 0.5) : 0.03, e, 12); lastHazardEv = e; } break;
            case HZ_UNREST: if (onset || rng.chance(0.35)) {
                const int p = pickPlace(y, [&](int q) { return big(q) ? 2.0 : 0.3; });
                const int e = ev(HE_UNREST, y, p, 0.5f); E(e).thing = a >= AGE_INDUSTRIAL ? (rng.chance(0.5) ? 1 : 3) : 2; E(e).value = (int64_t)(placePop(std::max(0, p), y) * rng.range(0.0005, 0.005));
                harm(p, 0.005, e, 7); lastHazardEv = e; } break;
            case HZ_LEAVING: if (onset || rng.chance(0.3)) {
                const int p = exposed();
                const int e = ev(HE_EXODUS, y, p, 0.6f); E(e).value = (int64_t)(placePop(p, y) * rng.range(0.2, 0.6)); E(e).thing = CS.whither;
                deficit += 0.02; lastHazardEv = e; } break;
            case HZ_FIRE: if (onset || rng.chance(0.15)) {
                const int p = exposed();
                const int e = ev(HE_SIGN, y, p, onset ? 1.0f : 0.6f); E(e).thing = STAGE_ONSET * 2 + (onset ? 0 : 1); E(e).out = STAGE_ONSET;
                const double share = onset ? rng.range(0.15, 0.6) : rng.range(0.01, 0.05);
                E(e).value = (int64_t)(pop(y) * share); harm(-1, share, e, 12); deficit += share * 0.5; lastHazardEv = e; } break;
            case HZ_CHILDREN: break;
            default: break;
        }
        if (onset && lastSignEv >= 0 && H.events[lastSignEv].year == y)
            for (int i = hazardBefore; i < (int)H.events.size(); i++) if (H.events[i].cause < 0) H.events[i].cause = lastSignEv;
        if (lastHazardEv >= hazardBefore) lastSignEv = lastHazardEv;   // the later signs of the year follow the harm
    }
    signs(STAGE_CRISIS, STAGE_END);
    // the places left in the collapse, in the cause's order, the people with them; the great ones' people leave in a body
    auto it = endsByYear.find(y);
    if (it != endsByYear.end()) for (int p : it->second) {
        if (H.places[p].ended != y) continue;   // left early
        if (p == H.lastPlace) continue;          // the end's
        const int e = ev(HE_ABANDONED, y, p, big(p) ? 0.7f : 0.2f); E(e).cause = lastHazardEv; E(e).thing = 8;
        H.places[p].endEvent = e;
        if (big(p) && placePop(p, y) > 200) {
            const int x = ev(HE_EXODUS, y, p, 0.5f); E(x).value = (int64_t)(placePop(p, y) * rng.range(0.1, 0.4)); E(x).thing = CS.whither; E(x).cause = e;
            if (CS.whither == WH_PLACE) E(x).place2 = H.lastPlace;
        }
        const int to = nearestStanding(p, y + 1, false);
        for (int q : stepped) {
            HistoryPerson& Q = H.persons[q];
            if (Q.died != ALIVE || Q.home != p) continue;
            Q.home = to >= 0 ? to : H.lastPlace;
            if (mode[q] == 2 && y - Q.born >= 16) { const int m = ev(HE_MOVE, y, p, 0.15f); E(m).a = q; E(m).place2 = Q.home; E(m).cause = e; }
        }
    }
    // the rule ends in the crisis or the end (the last ruler named so)
    if (ruleEnds > 0 && y == ruleEnds && reign >= 0 && H.reigns[reign].end == ALIVE) endReign(y, 6, lastHazardEv);
    // the people's harm through the crisis by the cause's speed (a sudden end's at its onset)
    const int speed = causeSpeed(H.cause);
    if (stage >= STAGE_CRISIS) {
        int how = 15;
        switch (CS.hazard) {
            case HZ_FAMINE: case HZ_DRYING: how = 4; break;
            case HZ_COLD: how = 13; break;
            case HZ_SICKNESS: how = 1; break;
            case HZ_FLOOD: how = 5; break;
            case HZ_IMPACT: case HZ_FLARE: case HZ_FIRE: how = 12; break;
            default: break;
        }
        harm(-1, speed == 2 ? 0.01 : (speed == 1 ? 0.04 : 0.06), lastHazardEv, how);
    }
    (void)prog;
}

void Builder::stepPersons(int y) {
    const PeopleProfile& P = H.profile;
    const int a = ageAt(y);
    const Mortality& M = MORTALITY[a];
    const int stage = P.stageAt(y);
    double fert = M.births;
    if (stage >= 0) {
        const double prog = clampd((y - H.stageYear[0] + 1) / (double)std::max(1, H.last - H.stageYear[0] + 1), 0, 1);
        fert *= SCRIPTS[H.cause].hazard == HZ_CHILDREN ? std::max(0.0, 1 - 1.3 * prog) : (stage >= STAGE_CRISIS ? 0.5 : 0.85);
    }
    if (hz[0] > 0.01) fert *= 0.7;
    // the families begin: ten households in the places standing, a couple each with their first children
    if (y == H.famFrom) {
        for (int f = 0; f < FAMILIES; f++) {
            int home = anyPlace(y, 1.5);
            for (int t = 0; t < 8 && home >= 0 && H.places[home].ended <= y; t++) home = anyPlace(y, 1.5);
            HistoryFamily F; F.name = mix64(H.seed ^ (0x40FAULL + f * 0x9E3779B97F4A7C15ULL)); F.home = home; F.trade = a * 10 + rng.irange(10); F.from = y; F.to = H.last;
            H.families.push_back(F);
            const int fi = (int)H.families.size() - 1;
            const int married = y - 2 - rng.irange(10);
            const int hb = person(married - M.marry - rng.irange(8), false, fi, home, HR_NONE, 2);
            const int wf = person(married - M.marry - rng.irange(6), true, fi, home, HR_NONE, 2);
            H.persons[hb].spouse = wf; H.persons[wf].spouse = hb;
            H.families[fi].head = hb; H.families[fi].founder = hb;
            const int me = ev(HE_MARRIAGE, std::max(H.first, married), home >= 0 && stands(home, married) ? home : -1, 0.1f); E(me).a = hb; E(me).b = wf;
            for (int c = married + 1; c < y; c++) if (rng.chance(M.births * 1.2)) {
                const int k = person(c, rng.chance(0.5), fi, home, HR_NONE, 2);
                link(k, wf, hb);
                const int be = ev(HE_BIRTH, c, home >= 0 && stands(home, c) ? home : -1, 0.1f); E(be).a = k; E(be).b = wf; E(be).c = hb;
            }
        }
    }
    // a family that died out: another household of the same place taken up in its place (the records need writers to the end)
    if (y > H.famFrom && y < H.stageYear[STAGE_CRISIS] && (y - H.famFrom) % 10 == 0) {
        std::vector<int> livingIn(H.families.size(), 0);
        for (int q : stepped) if (mode[q] == 2 && H.persons[q].died == ALIVE && H.persons[q].family >= 0) livingIn[H.persons[q].family]++;
        int followed = 0;
        for (size_t f = 0; f < H.families.size(); f++) if (!H.families[f].house && livingIn[f] > 0) followed++;
        for (int k = followed; k < FAMILIES; k++) {
            int home = anyPlace(y, 1.5);
            for (int t = 0; t < 8 && home >= 0 && H.places[home].ended <= y; t++) home = anyPlace(y, 1.5);
            HistoryFamily F; F.name = mix64(H.seed ^ (0x40FBULL + H.families.size() * 0x9E3779B97F4A7C15ULL)); F.home = home; F.trade = a * 10 + rng.irange(10); F.from = y; F.to = H.last;
            H.families.push_back(F);
            const int fi = (int)H.families.size() - 1;
            const int hb = person(y - M.marry - 2 - rng.irange(12), false, fi, home, HR_NONE, 2);
            H.families[fi].head = hb; H.families[fi].founder = hb;
            household(hb, y, fi);
        }
    }
    // the persons stepped: the families followed, a ruling house (its heirs, outside the families' years too), the figures (their deaths)
    std::vector<int> now;
    now.reserve(stepped.size());
    for (int q : stepped) if (mode[q] > 0 && H.persons[q].died == ALIVE) now.push_back(q);
    stepped.swap(now);
    famOf.assign(H.families.size(), 0);
    for (int q : stepped) if (mode[q] == 2 && H.persons[q].family >= 0) famOf[H.persons[q].family]++;
    auto alive = [&](int p) { return p >= 0 && H.persons[p].died == ALIVE; };
    const size_t n0 = stepped.size();
    for (size_t k = 0; k < n0; k++) {
        const int q = stepped[k];
        if (!alive(q) || mode[q] == 0) continue;
        const int years = y - H.persons[q].born;
        const bool female = H.persons[q].female;
        const int home = H.persons[q].home;
        // death: the age's, and the year's harm where they live (the battles for those of fighting age)
        const double nat = hazard(a, years);
        const int hk = home + 1;
        double ext = hz[0] + (hk > 0 && hk < (int)hz.size() ? hz[hk] : 0);
        const bool fighting = years >= 17 && years <= 45 && (!female || a >= AGE_MODERN);
        if (fighting) ext += hzBattle * 0.5;
        if (rng.chance(std::min(1.0, nat + ext))) {
            int cause = -1, how = years < 15 ? 11 : 0;
            if (ext > 0 && rng.chance(ext / (nat + ext))) {
                double best = hz[0]; int ce = hzEv[0], ch = hzHow[0];
                if (hk > 0 && hk < (int)hz.size() && hz[hk] > best) { best = hz[hk]; ce = hzEv[hk]; ch = hzHow[hk]; }
                if (fighting && hzBattle * 0.5 > best) { ce = hzBattleEv; ch = 3; }
                cause = ce; how = ch;
                if (cause < 0) how = years < 15 ? 11 : 0;
            } else if (years >= 15 && years < 50 && rng.chance(0.08)) how = 7;   // an accident
            die(q, y, how, cause);
            continue;
        }
        if (mode[q] != 2) continue;
        // marriage: within the household's place, now and then into another family followed
        const int sp0 = H.persons[q].spouse;
        if (sp0 < 0 || !alive(sp0)) {
            const bool widowed = sp0 >= 0;
            if (years >= M.marry && years <= 45 && rng.chance(widowed ? 0.06 : 0.16)) {
                int sp = -1;
                const int fam = H.persons[q].family;
                if (rng.chance(0.3)) {
                    for (int r2 : stepped) {
                        const HistoryPerson& R = H.persons[r2];
                        if (r2 == q || mode[r2] != 2 || R.died != ALIVE || R.female == female || R.family == fam || R.home != home) continue;
                        if (R.spouse >= 0 && H.persons[R.spouse].died == ALIVE) continue;
                        if (std::abs((y - R.born) - years) > 10 || y - R.born < M.marry) continue;
                        if (R.mother >= 0 && (R.mother == H.persons[q].mother || R.father == H.persons[q].father)) continue;
                        sp = r2; break;
                    }
                }
                if (sp < 0) {   // a husband a few years older as often as not, every spouse of the age to marry
                    const int spAge = clampi(years + (female ? rng.irange(9) - 2 : 2 - rng.irange(9)), M.marry, 60);
                    sp = person(y - spAge, !female, fam, home, HR_NONE, 2);
                }
                H.persons[q].spouse = sp; H.persons[sp].spouse = q;
                // the couple's home: the husband's or the wife's household by the people's custom
                const int husband = female ? sp : q, wife = female ? q : sp;
                const int to = residence == 0 ? H.persons[husband].home : H.persons[wife].home;
                for (int mv : {q, sp}) if (H.persons[mv].home != to && to >= 0) {
                    const int m = ev(HE_MOVE, y, H.persons[mv].home, 0.12f); E(m).a = mv; E(m).place2 = to; H.persons[mv].home = to;
                }
                const int me = ev(HE_MARRIAGE, y, to >= 0 && stands(to, y) ? to : -1, 0.15f); E(me).a = q; E(me).b = sp;
                // too many in the household: a branch off the family's main line (its head and the head's eldest living child; a ruling
                // house's head is its ruler) is let go, its children not followed
                if (fam >= 0 && fam < (int)famOf.size() && famOf[fam] > FAMILY_CAP) {
                    const int head = familyHead(fam);
                    bool mainLine = q == head || head < 0;
                    if (!mainLine) for (int c : kids[head]) if (H.persons[c].died == ALIVE) { mainLine = c == q; break; }
                    if (!mainLine) { letGo(q, y); letGo(sp, y); continue; }
                }
            }
        }
        // a child: a wife in her years with her husband alive
        const int hb = H.persons[q].spouse;
        if (female && alive(hb) && years >= M.marry && years <= M.fertileTo && rng.chance(fert)) {
            const int fam = residence == 0 ? H.persons[hb].family : H.persons[q].family;
            bool girl = rng.chance(0.5);
            uint64_t nm = 0;
            if (rng.chance(0.18)) {   // named after a grandparent
                const int gps[4] = {H.persons[q].mother, H.persons[q].father, H.persons[hb].mother, H.persons[hb].father};
                const int gp = gps[rng.irange(4)];
                if (gp >= 0) { nm = H.persons[gp].name; girl = H.persons[gp].female; }
            }
            const int c = person(y, girl, fam, home, HR_NONE, 2, nm);
            link(c, q, hb);
            const int be = ev(HE_BIRTH, y, home >= 0 && stands(home, y) ? home : -1, 0.1f); E(be).a = c; E(be).b = q; E(be).c = hb;
            if (rng.chance(a <= AGE_MEDIEVAL ? 0.012 : (a == AGE_INDUSTRIAL ? 0.006 : 0.0005))) die(q, y, 2, be);
            continue;
        }
        // a move to another place: the young to the cities, more in the industrial age
        static const double MOVE[AGE_COUNT] = {0.004, 0.006, 0.02, 0.03, 0.02, 0.01};
        if (years >= 18 && years <= 40 && home >= 0 && rng.chance(MOVE[a])) {
            const int to = anyPlace(y, 4, home);
            if (to >= 0 && to != home && H.places[to].ended > y) {
                const int m = ev(HE_MOVE, y, home, 0.12f); E(m).a = q; E(m).place2 = to;
                H.persons[q].home = to;
                const int sp = H.persons[q].spouse;
                if (alive(sp) && H.persons[sp].home == home) { const int m2 = ev(HE_MOVE, y, home, 0.1f); E(m2).a = sp; E(m2).place2 = to; H.persons[sp].home = to; }
                for (int c : kids[q]) if (alive(c) && y - H.persons[c].born < 16) H.persons[c].home = to;
            }
        }
    }
}

void Builder::importJoint() {
    if (!WP || WP->peoples < 2) return;
    joint = jointOf(sys, body, g, *WP, people, H.profile, sched);
    if (!joint.met) return;
    jointWar.assign(joint.wars.size(), -1);
    jointLastBattle.assign(joint.wars.size(), -1);
    for (size_t i = 0; i < joint.events.size(); i++) {
        const int y = yearOf(H.profile, joint.events[i].ago);
        if (y < 1 || y > H.last) continue;
        jointByYear[y].push_back((int)i);
    }
}

void Builder::yearLoop() {
    const PeopleProfile& P = H.profile;
    const int n = (int)H.places.size();
    hz.assign(n + 1, 0); hzEv.assign(n + 1, -1); hzHow.assign(n + 1, 0);
    H.yearsFrom = std::max(1, H.last - 25000);
    H.harvest.assign(H.last - H.yearsFrom + 1, 2);
    H.population.assign(H.last - H.yearsFrom + 1, 0);
    size_t plannedAt = 0;
    int termEnd = 0;
    ruleEnds = H.stageYear[STAGE_CRISIS] + (int)std::lround(rng.range(0.3, 1.0) * (H.last - H.stageYear[STAGE_CRISIS]));
    if (ruleEnds >= H.last) ruleEnds = H.last;
    int lastEra = -1;
    for (int y = 1; y <= H.last; y++) {
        for (int k : hzTouched) { hz[k] = 0; hzEv[k] = -1; }
        hzTouched.clear();
        hzBattle = 0; hzBattleEv = -1;
        const int ei = H.eraAt(y);
        const HistoryEra& era = H.eras[ei];
        if (ei != lastEra) {
            if (lastEra < 0 || era.govFrom != govNow) govNow = era.govFrom;
            lastEra = ei;
        }
        if (y % 16 == 0) cleanStanding(y);
        // the foundings
        auto fy = foundByYear.find(y);
        if (fy != foundByYear.end()) for (int p : fy->second) {
            int founder;
            if (p == 0 && y == 1 && H.age != AGE_FAR_FUTURE) founder = person(1 - 30 - rng.irange(15), rng.chance(0.5), -1, 0, HR_FOUNDER, 1, NAME_FOUNDER);
            else if (H.age == AGE_FAR_FUTURE && p == seatOld) founder = person(1 - 30 - rng.irange(15), rng.chance(0.5), -1, p, HR_FOUNDER, 1, NAME_FOUNDER);
            else founder = figure(y, HR_FOUNDER, H.places[p].from >= 0 && stands(H.places[p].from, y) ? H.places[p].from : -1, 22, 50);
            H.persons[founder].home = p;
            if (H.persons[founder].role == HR_NONE) H.persons[founder].role = HR_FOUNDER;
            H.places[p].founder = founder;
            const int e = ev(HE_FOUNDING, y, p, p == 0 ? 0.9f : (big(p) ? 0.55f : 0.25f));
            if (y == 1 && (p == 0 || p == seatOld)) vacancyEv = e;   // the first ruler's rule begins with the capital
            E(e).a = founder; E(e).place2 = H.places[p].from >= 0 && stands(H.places[p].from, y) ? H.places[p].from : -1;
            int why = 0;
            const int sc = H.places[p].sclass;
            if (H.age == AGE_FAR_FUTURE && H.places[p].plan >= 0) why = 11;
            else if (p == 0 || p == seatOld) why = 12;
            else if (H.age == AGE_PREHISTORIC) why = 9;
            else if (H.age == AGE_FUTURISTIC && sc == SC_TOWN && H.places[p].age == AGE_FUTURISTIC) why = 10;
            else if (H.age == AGE_FAR_FUTURE && H.places[p].plan >= 0) why = 11;
            else if (H.places[p].age == AGE_INDUSTRIAL && H.places[p].plan >= 0 && G->res[PP->places[H.places[p].plan].cell] > 0.5) why = 4;
            else if (H.places[p].age == AGE_INDUSTRIAL && rng.chance(0.4)) why = 7;
            else if (H.places[p].coast && rng.chance(0.5)) why = 2;
            else if (H.places[p].river && rng.chance(0.4)) why = 3;
            else if (P.values[VAL_WAR] > 0.25 && H.places[p].age <= AGE_MEDIEVAL && rng.chance(0.35)) why = 1;
            else if (P.values[VAL_TRADE] > 0.25 && rng.chance(0.35)) why = 5;
            else if (P.values[VAL_PIETY] > 0.25 && rng.chance(0.35)) why = 6;
            E(e).thing = why;
            if (H.age == AGE_PREHISTORIC && p == 0) E(e).thing = 8;
            onFound(p);
        }
        // the clearings of the places a later people's plan does not draw
        auto cy = clearByYear.find(y);
        if (cy != clearByYear.end()) for (int p : cy->second) {
            if (H.places[p].ended != y) continue;
            const int e = ev(HE_ABANDONED, y, p, p == seatOld ? 0.7f : 0.4f); E(e).thing = 7;
            H.places[p].endEvent = e;
            const int to = nearestStanding(p, y + 1, false);
            for (int q : stepped) if (H.persons[q].died == ALIVE && H.persons[q].home == p) H.persons[q].home = to;
        }
        // the places left early with no event to blame (a band moved on, a mine worked out, the soil spent, a river's new course)
        auto ny = naturalEnds.find(y);
        if (ny != naturalEnds.end()) for (const auto& pw : ny->second) {
            const int p = pw.first;
            if (!stands(p, y) || p == seat(y) || earlyLeft * 7 > (int)H.places.size()) continue;
            leave(p, y, HP_EARLY, -1, pw.second); earlyLeft++;
        }
        // the festivals: the capital's hundreds, a city's hundredth and its five hundreds
        auto fe = festByYear.find(y);
        if (fe != festByYear.end()) for (int p : fe->second) {
            if (!stands(p, y)) continue;
            const int age2 = y - H.places[p].founded;
            const bool cap = p == seat(y);
            if (!cap && age2 % 1000 != 0) continue;
            const int e = ev(HE_FESTIVAL, y, p, cap ? (age2 % 500 == 0 ? 0.4f : 0.15f) : 0.1f); E(e).value = age2;
        }
        // the rule: the first ruler at year 1, the government's change, a term's end, a death, the odd deposition
        if (y == 1) {
            int founder = H.places.empty() ? -1 : H.places[seatOld >= 0 ? seatOld : 0].founder;
            if (founder < 0) founder = person(-30 - rng.irange(15), rng.chance(0.5), -1, seat(1), HR_FOUNDER, 1, NAME_FOUNDER);
            if (govNow == GOV_MONARCHY) { const int f = newHouse(y, founder); newReign(founder, y, 0, f); }
            else if (govNow == GOV_MACHINE) newReign(-1, y, 8);
            else newReign(founder, y, 0);
            termEnd = termOf(y) > 0 ? y + termOf(y) : 0;
        }
        if (era.govChange == y && govNow != era.government) {
            const int from = govNow, to = era.government;
            int how = 0;
            if (to == GOV_MONARCHY && from == GOV_CHIEFS) how = 2;
            else if (to == GOV_THEOCRACY) how = 3;
            else if (to == GOV_CORPORATE) how = 4;
            else if (to == GOV_MACHINE) how = 5;
            else if (to == GOV_COUNCIL) how = 7;
            else if (to == GOV_ASSEMBLY) how = 8;
            else if (to == GOV_MONARCHY) how = 6;
            else if (from == GOV_MONARCHY && rng.chance(0.6)) how = 1;
            const int ruler = reign >= 0 ? H.reigns[reign].ruler : -1;
            int leader = how == 2 && ruler >= 0 && H.persons[ruler].died == ALIVE ? ruler : -1;
            if (how == 1) {   // a revolution: a short war first
                const int rebel = figure(y, HR_REBEL, seat(y), 25, 45);
                leader = rebel;
                const int wi = startWar(y, HW_REVOLUTION, -1, seat(y), ruler >= 0 && H.persons[ruler].died == ALIVE ? ruler : -1, rebel, -1, 1 + rng.irange(3));
                (void)wi;
            } else if (leader < 0 && how != 5) leader = figure(y, how == 3 ? HR_PROPHET : HR_RULER, seat(y), 35, 60);
            const int e = ev(HE_GOVERNMENT, y, seat(y), 0.85f); E(e).thing = to; E(e).value = from; E(e).out = how; E(e).a = leader;
            govNow = to;
            if (how == 2 && leader >= 0) {   // the chief made a kingdom: the same person, a new reign and a house
                endReign(y, 7, e);
                const int f = newHouse(y, leader);
                newReign(leader, y, 0, f, leader);
            } else {
                endReign(y, how == 1 ? 4 : 7, e);
                if (how == 1 && ruler >= 0 && H.persons[ruler].died == ALIVE && rng.chance(0.3)) die(ruler, y, 9, e, seat(y));
                if (to == GOV_MACHINE) newReign(-1, y, 8, -1, ruler);
                else if (leader >= 0 && (how == 1 || how == 6 || how == 3)) {
                    if (to == GOV_MONARCHY) { const int f = newHouse(y, leader); newReign(leader, y, 4, f, ruler); }
                    else newReign(leader, y, how == 1 ? 9 : 2, -1, ruler);
                } else succeed(y, 2);
            }
            termEnd = termOf(y) > 0 ? y + termOf(y) : 0;
        }
        const bool ruleOver = ruleEnds > 0 && y >= ruleEnds;
        if (!ruleOver && reign >= 0 && H.reigns[reign].end == ALIVE) {
            if (termEnd > 0 && y >= termEnd) {
                const int prev = H.reigns[reign].ruler;
                const bool again = (govNow == GOV_REPUBLIC || govNow == GOV_ASSEMBLY) && prev >= 0 && H.persons[prev].died == ALIVE && rng.chance(govNow == GOV_REPUBLIC ? 0.3 : 0.4)
                                   && (reign < 1 || H.reigns[reign - 1].ruler != prev);
                if (again) { H.reigns[reign].end = y; H.reigns[reign].endHow = 3; newReign(prev, y, 10, -1, prev); }
                else { endReign(y, 3); succeed(y, 3); }
                termEnd = termOf(y) > 0 ? y + termOf(y) : 0;
            }
            else if (govNow != GOV_MACHINE && H.reigns[reign].ruler >= 0) {
                static const double DEPOSE[GOV_COUNT] = {0.008, 0.003, 0.002, 0.002, 0.002, 0.002, 0.005, 0};
                static const double MURDER[GOV_COUNT] = {0.004, 0.002, 0.0005, 0.001, 0.0005, 0.0005, 0.0005, 0};
                const int r = H.reigns[reign].ruler;
                if (rng.chance(DEPOSE[govNow])) {
                    endReign(y, 1);
                    if (govNow == GOV_MONARCHY) { const int a2 = figure(y, HR_RULER, seat(y), 25, 50, false); const int f = newHouse(y, a2); newReign(a2, y, 4, f, r); }
                    else succeed(y, 4);
                    termEnd = termOf(y) > 0 ? y + termOf(y) : 0;
                } else if (rng.chance(MURDER[govNow])) die(r, y, 8, -1, seat(y));
            }
            // a regency ends when the heir comes of age
            if (reign >= 0 && H.reigns[reign].regent >= 0 && H.reigns[reign].ruler >= 0 && y - H.persons[H.reigns[reign].ruler].born == 16 && H.reigns[reign].end == ALIVE) {
                const int e = ev(HE_ACCESSION, y, seat(y), 0.3f); E(e).a = H.reigns[reign].ruler; E(e).b = H.reigns[reign].regent; E(e).thing = 7; E(e).value = reign;
            }
        }
        // the inventions, the institutions, the faiths, the discoveries, the signs of the collapse
        while (plannedAt < planned.size() && planned[plannedAt].year < y) plannedAt++;
        plannedAtM = plannedAt;
        for (size_t k = plannedAt; k < planned.size() && planned[k].year == y; k++) {
            const Planned& q = planned[k];
            switch (q.kind) {
                case HE_INVENTION: {
                    int id = q.a;
                    const int p = pickPlace(y, [&](int x) { return x == seat(y) ? 4.0 : (big(x) ? 2.0 : 0.3); });
                    const int who = figure(y, HR_INVENTOR, p, 25, 60);
                    const int e = ev(HE_INVENTION, y, p, 0.6f); E(e).thing = id; E(e).a = who;
                    HistoryInvention I; I.id = id; I.year = y; I.person = who; I.place = p;
                    H.inventions.push_back(I);
                    break;
                }
                case HE_INSTITUTION: {
                    const int p = q.a == HI_COMPANY || q.a == HI_GUILDS ? pickPlace(y, [&](int x) { return (big(x) ? 2.0 : 0.3) * (H.places[x].coast ? 1.5 : 1.0); }) : seat(y);
                    HistoryInstitution I; I.kind = q.a; I.founded = y; I.ended = H.last; I.place = p; I.name = mix64(H.seed ^ (0x4015ULL + H.institutions.size()));
                    I.founder = q.a == HI_COUNCIL || q.a == HI_ASSEMBLY ? -1 : figure(y, q.a == HI_PRIESTHOOD ? HR_PROPHET : HR_MAKER, p, 30, 60);
                    H.institutions.push_back(I);
                    const int e = ev(HE_INSTITUTION, y, p, 0.45f); E(e).thing = (int)H.institutions.size() - 1; E(e).a = I.founder;
                    break;
                }
                case HE_FAITH: {
                    const int p = anyPlace(y);
                    const int who = figure(y, HR_PROPHET, p, 25, 55);
                    const int e = ev(HE_FAITH, y, p, 0.6f); E(e).thing = 0; E(e).a = who; E(e).value = q.b;
                    break;
                }
                case HE_DISCOVERY: {
                    const int p = q.a == 0 && q.b >= 0 ? (H.places[q.b].from >= 0 && stands(H.places[q.b].from, y) ? H.places[q.b].from : seat(y)) : anyPlace(y);
                    const int who = figure(y, HR_EXPLORER, p, 20, 50);
                    const int e = ev(HE_DISCOVERY, y, p, 0.5f); E(e).thing = q.a; E(e).a = who; E(e).value = q.a == 0 ? q.b : -1;
                    break;
                }
                case HE_SIGN: break;   // after the year's stages (below)
                default: break;
            }
        }
        // the ways opened
        auto wy = waysByYear.find(y);
        if (wy != waysByYear.end()) for (int e2 : wy->second) {
            const PlanEdge& pe = PP->edges[e2];
            if (!stands(pe.a, y) || !stands(pe.b, y)) continue;
            const int e = ev(HE_WAY, y, pe.a, pe.trunk ? 0.4f : 0.25f); E(e).place2 = pe.b; E(e).thing = e2; E(e).out = pe.kind;
        }
        // the other people's meeting, treaties and wars (C-13: both histories hold them)
        auto jy = jointByYear.find(y);
        if (jy != jointByYear.end()) for (int ji : jy->second) {
            const JointEvent& J = joint.events[ji];
            int mine = J.place[people];
            if (mine >= 0 && !stands(mine, y)) mine = nearestStanding(mine, y);
            if (mine < 0 && J.kind != HE_OTHER_END) mine = seat(y);
            const int theirs = J.place[1 - people];
            switch (J.kind) {
                case HE_CONTACT: { const int e = ev(HE_CONTACT, y, mine, J.salience); E(e).other = theirs; E(e).a = figure(y, HR_ENVOY, mine, 25, 55); break; }
                case HE_TREATY: { const int e = ev(HE_TREATY, y, mine, J.salience); E(e).other = theirs; E(e).thing = J.out; break; }
                case HE_WAR: {
                    const JointWar& JW = joint.wars[J.war];
                    HistoryWar w; w.kind = JW.kind; w.start = y; w.end = clampi(yearOf(P, JW.to), y, H.last); w.sideA = -1; w.sideB = -2;
                    w.winner = JW.winner < 0 ? -1 : (JW.winner == people ? 0 : 1); w.name = JW.name;
                    w.leaderA = reign >= 0 && H.reigns[reign].ruler >= 0 && H.persons[H.reigns[reign].ruler].died == ALIVE && rng.chance(0.4) ? H.reigns[reign].ruler : figure(y, HR_GENERAL, seat(y), 28, 55);
                    H.wars.push_back(w);
                    jointWar[J.war] = (int)H.wars.size() - 1;
                    const int e = ev(HE_WAR, y, mine, J.salience); E(e).thing = jointWar[J.war]; E(e).a = H.wars.back().leaderA; E(e).other = theirs; E(e).out = JW.aggressor == people ? 0 : 1;
                    H.wars.back().declared = e;
                    break;
                }
                case HE_BATTLE: {
                    const int wi = J.war >= 0 ? jointWar[J.war] : -1;
                    if (wi < 0) break;
                    HistoryWar& w = H.wars[wi];
                    const int side = J.out == people ? 0 : 1;
                    int L = w.leaderA;
                    if (L < 0 || H.persons[L].died != ALIVE) { L = figure(y, HR_GENERAL, seat(y), 28, 55); w.leaderA = L; }
                    const int e = ev(HE_BATTLE, y, mine, J.salience, J.season); E(e).thing = wi; E(e).out = side; E(e).value = J.value; E(e).other = theirs; E(e).cause = w.declared;
                    if (side == 0) E(e).a = L; else E(e).b = L;
                    w.dead += J.value; w.battles++;
                    jointLastBattle[J.war] = e;
                    const double share = J.value / std::max(1.0, pop(y) * 0.12);
                    if (share > hzBattle) { hzBattle = share; hzBattleEv = e; }
                    if (rng.chance(side == 0 ? 0.04 : 0.12) && H.persons[L].died == ALIVE) die(L, y, 3, e, mine);
                    if (w.kind == HW_CONQUEST && side == 1 && mine >= 0 && mine != H.lastPlace && rng.chance(0.4)) { const int sk = ev(HE_SACK, y, mine, 0.6f); E(sk).thing = wi; E(sk).cause = e; E(sk).value = (int64_t)(placePop(mine, y) * rng.range(0.05, 0.3)); lastHazardEv = sk; harm(mine, 0.1, sk, 3); }
                    break;
                }
                case HE_PEACE: {
                    const int wi = J.war >= 0 ? jointWar[J.war] : -1;
                    if (wi < 0) break;
                    HistoryWar& w = H.wars[wi];
                    const int e = ev(HE_PEACE, y, mine, J.salience); E(e).thing = wi; E(e).out = J.out == 2 ? 2 : (J.out == people ? 0 : 1); E(e).other = theirs;
                    E(e).cause = jointLastBattle[J.war] >= 0 ? jointLastBattle[J.war] : w.declared;
                    E(e).a = w.leaderA >= 0 && H.persons[w.leaderA].died == ALIVE ? w.leaderA : -1;
                    w.peace = e; w.end = y;
                    break;
                }
                case HE_OTHER_END: if (J.out != people) { const int e = ev(HE_OTHER_END, y, -1, J.salience); (void)e; } break;
                default: break;
            }
        }
        // a conquest by the other people closes when the conquered people ends
        for (HistoryWar& w : H.wars) if (w.kind == HW_CONQUEST && w.sideB == -2 && w.peace < 0 && y == H.last && w.end >= H.last) w.end = H.last;
        randomEvents(y);
        everyday(y);
        if (y >= P.collapseYear) collapseYear(y);
        stepWars(y);
        // a sickness's waves: where it strikes this year, and the places near it
        for (size_t k = 0; k < waves.size();) {
            Wave& wv = waves[k];
            if (wv.year != y) { k++; continue; }
            HistorySickness& S = H.sicknesses[wv.sickness];
            int at = wv.place >= 0 && stands(wv.place, y) ? wv.place : anyPlace(y);
            const int64_t dead = (int64_t)(pop(y) * wv.share);
            int first = -1;
            for (int t = 0; t < (wv.share > 0.06 ? 2 : 1) && at >= 0; t++) {
                const int e = ev(HE_SICKNESS, y, at, wv.n == 1 ? 0.6f : 0.45f); E(e).thing = wv.sickness; E(e).out = wv.n; E(e).value = t == 0 ? dead : dead / 3;
                if (first < 0) first = e; else E(e).cause = first;
                harm(at, wv.share * 3, e, 1);
                if (t == 0) maybeLeave(at, y, e, H.places[at].sclass == SC_TOWN ? 0.03 : 0.01);
                at = nearestStanding(at, y, false);
            }
            harm(-1, wv.share * 0.5, first, 1);
            if (P.stageAt(y) >= STAGE_ONSET) lastHazardEv = first;
            S.last = y; S.waves = std::max(S.waves, wv.n); S.dead += dead;
            deficit += wv.share;
            waves.erase(waves.begin() + k);
        }
        // the rule after a death or a term: the next ruler (the collapse's end leaves none)
        if (vacant && !(ruleEnds > 0 && y >= ruleEnds)) { succeed(y, govNow == GOV_REPUBLIC || govNow == GOV_ASSEMBLY ? 3 : 2); termEnd = termOf(y) > 0 ? y + termOf(y) : 0; }
        stepPersons(y);
        if (vacant && !(ruleEnds > 0 && y >= ruleEnds)) { succeed(y, 2); termEnd = termOf(y) > 0 ? y + termOf(y) : 0; }
        // anyone whose place ended this year (moved in late, born there, a household taken up there) goes on to the nearest place that
        // stands the next year
        bool ended = false;
        if (endsByYear.count(y) || clearByYear.count(y) || leftThisYear == y) ended = true;
        if (y < H.last && ended) for (int q : stepped) {
            HistoryPerson& Q = H.persons[q];
            if (Q.died != ALIVE || Q.home < 0 || H.places[Q.home].ended > y) continue;
            const int to = nearestStanding(Q.home, y + 1, false);
            Q.home = to >= 0 ? to : H.lastPlace;
        }
        // the people's number
        deficit = std::min(0.6, deficit) * 0.96;
        const int yi = y - H.yearsFrom;
        if (yi >= 0 && yi < (int)H.population.size()) H.population[yi] = (float)pop(y);
    }
    // the end: the last place, the last person, everyone left
    const int lp = H.lastPlace >= 0 ? H.lastPlace : seat(H.last);
    int lastP = -1;
    for (int q : stepped) if (H.persons[q].died == ALIVE && mode[q] == 2 && H.persons[q].home == lp && H.last - H.persons[q].born >= 14) { if (lastP < 0 || H.persons[q].born < H.persons[lastP].born) lastP = q; }
    if (lastP < 0) for (int q : stepped) if (H.persons[q].died == ALIVE && H.last - H.persons[q].born >= 14) { lastP = q; break; }
    if (lastP < 0) lastP = person(H.last - 25 - rng.irange(40), rng.chance(0.5), -1, lp, HR_LAST, 1);
    if (H.persons[lastP].role == HR_NONE) H.persons[lastP].role = HR_LAST;
    if (H.persons[lastP].home != lp && lp >= 0) { const int m = ev(HE_MOVE, H.last, H.persons[lastP].home, 0.3f); E(m).a = lastP; E(m).place2 = lp; H.persons[lastP].home = lp; }
    if (reign >= 0 && H.reigns[reign].end == ALIVE) endReign(H.last, 6);
    const int endE = ev(HE_END, H.last, lp, 1.0f, H.seasons - 1); E(endE).a = lastP; E(endE).cause = lastHazardEv;
    if (lp >= 0) { H.places[lp].ended = H.last; H.places[lp].fate = HP_LAST; H.places[lp].endEvent = endE; }
    H.lastPerson = lastP;
    const int whither = SCRIPTS[H.cause].whither;
    const bool leaving = SCRIPTS[H.cause].hazard == HZ_LEAVING;
    for (int q = 0; q < (int)H.persons.size(); q++) {
        HistoryPerson& Q = H.persons[q];
        if (Q.died != ALIVE) continue;
        if (leaving && q != lastP) { Q.died = H.last; Q.fate = HF_LEFT; mode[q] = 0; continue; }
        die(q, H.last, 10, endE, lp);
    }
    (void)whither;
    for (HistoryReign& R : H.reigns) if (R.end == ALIVE) R.end = H.last;
    for (HistoryInstitution& I : H.institutions) I.ended = std::min(I.ended, H.last);
    for (HistoryWar& w : H.wars) if (w.end > H.last) w.end = H.last;
}

void Builder::finish() {
    // an effect never in a season before its cause's (a death in its sickness's season, an accession after the death that opened the rule)
    for (HistoryEvent& e : H.events) if (e.cause >= 0 && H.events[e.cause].year == e.year && H.events[e.cause].season > e.season) e.season = H.events[e.cause].season;
    // the events by their year and season, then in the order they happened; every index into them remapped
    const int n = (int)H.events.size();
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        const HistoryEvent& A = H.events[a]; const HistoryEvent& B = H.events[b];
        if (A.year != B.year) return A.year < B.year;
        return A.season < B.season;
    });
    std::vector<int> at(n);
    for (int i = 0; i < n; i++) at[order[i]] = i;
    std::vector<HistoryEvent> sorted(n);
    for (int i = 0; i < n; i++) sorted[i] = H.events[order[i]];
    for (HistoryEvent& e : sorted) if (e.cause >= 0) e.cause = at[e.cause];
    H.events.swap(sorted);
    for (HistoryPerson& p : H.persons) if (p.death >= 0) p.death = at[p.death];
    for (HistoryPlace& p : H.places) if (p.endEvent >= 0) p.endEvent = at[p.endEvent];
    for (HistoryWar& w : H.wars) { if (w.declared >= 0) w.declared = at[w.declared]; if (w.peace >= 0) w.peace = at[w.peace]; }
    // the rulers' numbers: a name the rulers shared counted in order (the first a I only where a second came)
    std::map<uint64_t, int> seen, total;
    for (const HistoryReign& R : H.reigns) if (R.ruler >= 0) {
        bool repeat = false; for (const HistoryReign& Q : H.reigns) { if (&Q == &R) break; if (Q.ruler == R.ruler) repeat = true; }
        if (!repeat) total[H.persons[R.ruler].name]++;
    }
    for (const HistoryReign& R : H.reigns) if (R.ruler >= 0) {
        HistoryPerson& p = H.persons[R.ruler];
        if (p.ordinal) continue;
        const int k = ++seen[p.name];
        if (total[p.name] > 1) p.ordinal = k;
    }
    // the indices
    H.publicEvents = 0;
    for (const HistoryEvent& e : H.events) if (e.pub) H.publicEvents++;
    const int np = (int)H.persons.size(), nl = (int)H.places.size();
    std::vector<int> cp(np + 1, 0), cl(nl + 1, 0);
    for (const HistoryEvent& e : H.events) {
        for (int x : {e.a, e.b, e.c}) if (x >= 0 && x < np) cp[x + 1]++;
        for (int x : {e.place, e.place2}) if (x >= 0 && x < nl) cl[x + 1]++;
    }
    for (int i = 0; i < np; i++) cp[i + 1] += cp[i];
    for (int i = 0; i < nl; i++) cl[i + 1] += cl[i];
    H.personFirst = cp; H.placeFirst = cl;
    H.personEvents.assign(cp[np], 0); H.placeEvents.assign(cl[nl], 0);
    std::vector<int> fp(cp.begin(), cp.end() - 1), fl(cl.begin(), cl.end() - 1);
    for (int i = 0; i < (int)H.events.size(); i++) {
        const HistoryEvent& e = H.events[i];
        int prevP = -2;
        for (int x : {e.a, e.b, e.c}) if (x >= 0 && x < np && x != prevP) { H.personEvents[fp[x]++] = i; prevP = x; }
        if (e.place >= 0 && e.place < nl) H.placeEvents[fl[e.place]++] = i;
        if (e.place2 >= 0 && e.place2 < nl && e.place2 != e.place) H.placeEvents[fl[e.place2]++] = i;
    }
    // trim the unfilled tails (a person named twice in one event counted once)
    for (int i = 0; i < np; i++) for (int k = fp[i]; k < cp[i + 1]; k++) H.personEvents[k] = -1;
    for (int i = 0; i < nl; i++) for (int k = fl[i]; k < cl[i + 1]; k++) H.placeEvents[k] = -1;
}
}   // namespace

// ---- the history's queries ----------------------------------------------------------------------------------------------------
int History::eventsIn(int year, int& from) const {
    auto lo = std::lower_bound(events.begin(), events.end(), year, [](const HistoryEvent& e, int y) { return e.year < y; });
    auto hi = std::upper_bound(lo, events.end(), year, [](int y, const HistoryEvent& e) { return y < e.year; });
    from = (int)(lo - events.begin());
    return (int)(hi - lo);
}
int History::reignAt(int year) const {
    for (int k = (int)reigns.size() - 1; k >= 0; k--) if (reigns[k].start <= year && reigns[k].end >= year) return k;
    return -1;
}
int History::eraAt(int year) const {
    for (int k = 0; k < (int)eras.size(); k++) if (year <= eras[k].end) return k;
    return eras.empty() ? 0 : (int)eras.size() - 1;
}
int History::harvestAt(int year) const { const int i = year - yearsFrom; return i >= 0 && i < (int)harvest.size() ? harvest[i] : 2; }
double History::populationAt(int year) const {
    const int i = year - yearsFrom;
    if (i >= 0 && i < (int)population.size() && population[i] > 0) return population[i];
    return 0;
}
int History::inventionYear(int id) const { for (const HistoryInvention& I : inventions) if (I.id == id) return I.year; return 0; }
std::vector<int> History::eventsOf(int person) const {
    std::vector<int> o;
    if (person < 0 || person + 1 >= (int)personFirst.size()) return o;
    for (int k = personFirst[person]; k < personFirst[person + 1]; k++) if (personEvents[k] >= 0) o.push_back(personEvents[k]);
    return o;
}
std::vector<int> History::eventsAt(int place) const {
    std::vector<int> o;
    if (place < 0 || place + 1 >= (int)placeFirst.size()) return o;
    for (int k = placeFirst[place]; k < placeFirst[place + 1]; k++) if (placeEvents[k] >= 0) o.push_back(placeEvents[k]);
    return o;
}

std::shared_ptr<History> buildHistory(const StarSystem& sys, const Body& b, const BodyGen& g, int people) {
    const double t0 = nowMs();
    auto H = std::make_shared<History>();
    const int n = std::max(1, peoplesOf(g));
    people = people > 0 && n > 1 ? 1 : 0;
    H->people = people; H->peoples = n;
    H->profile = peopleProfileOf(sys, b, g, people);
    H->lore = loreQuick(sys, b, g, people);
    Builder B(sys, b, g, people, *H);
    B.WP = worldPlanOf(g);
    B.PP = &B.WP->plans[std::min(people, B.WP->peoples - 1)];
    B.G = &B.WP->grid;
    const double tPlan = nowMs();
    B.setup();
    B.setupPlaces();
    B.setupCollapse();
    B.setupPlanned();
    B.importJoint();
    B.oldTime();
    B.yearLoop();
    B.family();
    B.finish();
    H->buildMs = nowMs() - tPlan;
    (void)t0;
    return H;
}

namespace {
struct HistorySlot { std::shared_ptr<const History> h; uint64_t used = 0; };
std::mutex g_histMutex;
std::map<std::pair<uint64_t, int>, HistorySlot> g_hist;
uint64_t g_histTick = 0;
}
std::shared_ptr<const History> historyOf(const StarSystem& sys, const Body& b, const BodyGen& g, int people) {
    const std::pair<uint64_t, int> key(g.seed, people > 0 ? 1 : 0);
    {
        std::lock_guard<std::mutex> lk(g_histMutex);
        auto it = g_hist.find(key);
        if (it != g_hist.end()) { it->second.used = ++g_histTick; return it->second.h; }
    }
    std::shared_ptr<const History> h = buildHistory(sys, b, g, people);
    std::lock_guard<std::mutex> lk(g_histMutex);
    if (g_hist.size() >= 8) {
        auto old = g_hist.begin();
        for (auto it = g_hist.begin(); it != g_hist.end(); ++it) if (it->second.used < old->second.used) old = it;
        g_hist.erase(old);
    }
    g_hist[key] = HistorySlot{h, ++g_histTick};
    return h;
}

std::shared_ptr<const History> historyOfWorld(const BodyGen& g, int people) {   // T-02
    const std::pair<uint64_t, int> key(g.seed, people > 0 ? 1 : 0);
    {
        std::lock_guard<std::mutex> lk(g_histMutex);
        auto it = g_hist.find(key);
        if (it != g_hist.end()) { it->second.used = ++g_histTick; return it->second.h; }
    }
    if (!g.starKnown || g.bodyIndex < 0) return nullptr;
    Star st;
    if (!starInSector(g.starSx, g.starSy, g.starSz, st, true)) return nullptr;
    StarSystem sys; sys.generate(st);
    if (g.bodyIndex >= (int)sys.bodies.size() || sys.bodies[g.bodyIndex].seed != g.seed) return nullptr;
    return historyOf(sys, sys.bodies[g.bodyIndex], g, people);
}

// ---- the names and the lines ----------------------------------------------------------------------------------------------------
int historySeat(const History& H, int year) {
    for (int p = 0; p < (int)H.places.size(); p++) if (H.places[p].sclass == SC_CAPITAL && H.placeStands(p, year)) return p;
    for (int p = 0; p < (int)H.places.size(); p++) if (H.placeStands(p, year)) return p;
    for (int p = 0; p < (int)H.places.size(); p++) if (H.places[p].sclass == SC_CAPITAL) return p;   // before the founding: the capital to be
    return H.places.empty() ? -1 : 0;
}
std::string historyPlaceName(const History& H, int place) {
    if (place < 0 || place >= (int)H.places.size()) return "the land";
    const uint64_t s = H.places[place].name;
    if (s == NAME_CITY) return H.lore.city;
    if (s == NAME_CITY2) return H.lore.city2;
    return tongueName(tongueOfSeed(H.lore.seed), s, tongueNameKindOfRank(H.places[place].sclass));   // H-04's second pass: the tongue's
}
std::string historyPersonName(const History& H, int person, bool withTitle) {
    if (person < 0 || person >= (int)H.persons.size()) return "someone";
    const HistoryPerson& P = H.persons[person];
    std::string nm = P.name == NAME_FOUNDER ? H.lore.founder : tongueName(tongueOfSeed(H.lore.seed), P.name, TN_PERSON, P.female);
    if (P.ordinal > 0) nm += " " + romanNumeral(P.ordinal);
    // a surname from the industrial age, when the registers keep them (a ruling house is named for its founder instead)
    if (P.family >= 0 && P.family < (int)H.families.size() && !H.families[P.family].house && P.born + 20 >= 1) {
        const int e = H.eraAt(P.born + 20);
        if (e >= 0 && H.eras[e].age >= AGE_INDUSTRIAL) nm += " " + historyFamilyName(H, P.family);
    }
    if (withTitle && P.role == HR_RULER) {
        for (const HistoryReign& R : H.reigns) if (R.ruler == person) {
            std::string t = historyRulerTitle(H, R.government, P.female);
            if (!t.empty() && t.compare(0, 4, "the ") != 0) for (size_t i = 0; i < t.size(); i++) if (i == 0 || t[i - 1] == ' ') t[i] = (char)toupper(t[i]);
            return t + " " + nm;
        }
    }
    return nm;
}
// H-04's second pass: a people's title for whoever held the rule (each people its own of the government's: a headman, a war-leader, an
// archon; a monarchy's ruler often by a word of their own tongue, as ours keep a khan or a pharaoh)
std::string historyRulerTitle(const History& H, int gov, bool female) {
    struct TitleSet { const char* m; const char* f; };
    static const TitleSet T[GOV_COUNT][6] = {
        {{"chief", "chief"}, {"headman", "headwoman"}, {"elder", "elder"}, {"speaker", "speaker"}, {"war-leader", "war-leader"}, {nullptr, nullptr}},
        {{"king", "queen"}, {"high king", "high queen"}, {"prince", "princess"}, {"lord", "lady"}, {"emperor", "empress"}, {nullptr, nullptr}},
        {{"first elder", "first elder"}, {"first councillor", "first councillor"}, {"archon", "archon"}, {"warden", "warden"}, {"provost", "provost"}, {nullptr, nullptr}},
        {{"high priest", "high priestess"}, {"oracle", "oracle"}, {"holy father", "holy mother"}, {"keeper", "keeper"}, {"prophet", "prophetess"}, {nullptr, nullptr}},
        {{"consul", "consul"}, {"president", "president"}, {"first citizen", "first citizen"}, {"chancellor", "chancellor"}, {"magistrate", "magistrate"}, {nullptr, nullptr}},
        {{"speaker", "speaker"}, {"chair", "chair"}, {"first delegate", "first delegate"}, {"convener", "convener"}, {nullptr, nullptr}, {nullptr, nullptr}},
        {{"director", "director"}, {"chairman", "chairwoman"}, {"chief executive", "chief executive"}, {"president", "president"}, {nullptr, nullptr}, {nullptr, nullptr}},
        {{"the mind", "the mind"}, {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr}}};
    gov = clampi(gov, 0, GOV_COUNT - 1);
    const uint64_t h = mix64(H.lore.seed ^ 0x717E5ULL ^ (uint64_t)(gov + 1) * 0x9E3779B97F4A7C15ULL);
    const bool native = (gov == GOV_MONARCHY || gov == GOV_CHIEFS || gov == GOV_THEOCRACY) && unitFromHash(h) < (gov == GOV_MONARCHY ? 0.45 : 0.25);
    if (native) {   // their own word (the decoder keeps it as a name)
        const Tongue& TG = tongueOfSeed(H.lore.seed);
        std::string w = tongueTerm(TG, gov == GOV_MONARCHY ? (female ? "queen" : "king") : (gov == GOV_CHIEFS ? "chief" : (female ? "priestess" : "priest")));
        if (female && gov != GOV_MONARCHY && !TG.femEnd.empty() && TG.femEnd.size() <= 2) w += TG.femEnd;
        return w;
    }
    int n = 0; while (n < 6 && T[gov][n].m) n++;
    const int k = (int)((h >> 8) % (uint64_t)std::max(1, n));
    return female ? T[gov][k].f : T[gov][k].m;
}
bool historyTitleNative(const History& H, int gov) {
    gov = clampi(gov, 0, GOV_COUNT - 1);
    const uint64_t h = mix64(H.lore.seed ^ 0x717E5ULL ^ (uint64_t)(gov + 1) * 0x9E3779B97F4A7C15ULL);
    return (gov == GOV_MONARCHY || gov == GOV_CHIEFS || gov == GOV_THEOCRACY) && unitFromHash(h) < (gov == GOV_MONARCHY ? 0.45 : 0.25);
}
std::string historyFamilyName(const History& H, int family) {
    if (family < 0 || family >= (int)H.families.size()) return "";
    return tongueName(tongueOfSeed(H.lore.seed), H.families[family].name, TN_FAMILY);
}
std::string historyWarName(const History& H, int war) {
    if (war < 0 || war >= (int)H.wars.size()) return "the war";
    const HistoryWar& w = H.wars[war];
    const int pick = (int)(w.name % (sizeof(WAR_NAMES) / sizeof(WAR_NAMES[0])));
    switch (w.kind) {
        case HW_REBELLION: return "the Rising of " + historyPlaceName(H, w.sideB);
        case HW_REVOLUTION: return "the Revolution of " + std::to_string(w.start);
        case HW_SUCCESSION: return "the War of the Two Claimants";
        case HW_OTHER: return "the War with the " + (H.lore.other.empty() ? std::string("Others") : H.lore.other);
        case HW_CONQUEST: return "the Conquest";
        case HW_BANDS: return "the Feud of " + historyPlaceName(H, w.sideA) + " and " + historyPlaceName(H, w.sideB);
        default: break;
    }
    if (w.name % 3 == 0 && w.sideB >= 0) return "the " + historyPlaceName(H, w.sideB) + " War";
    {   // H-04's second pass: each people its own way with the rest (its own third of the names, by its seed)
        const int nw = (int)(sizeof(WAR_NAMES) / sizeof(WAR_NAMES[0]));
        const uint64_t ps = mix64(H.lore.seed ^ 0x3A2ULL);
        return WAR_NAMES[(int)((ps + (uint64_t)pick * 3) % (uint64_t)nw)];
    }
}
std::string historySicknessName(const History& H, int sickness) {
    if (sickness < 0 || sickness >= (int)H.sicknesses.size()) return "a sickness";
    const HistorySickness& S = H.sicknesses[sickness];
    if (S.made) return "the made sickness";
    const uint64_t s = S.name;
    // H-04's second pass: a people's own looks and kinds (four looks and three kinds of the tables, by its seed), and a place's or a river's
    // name for one sickness in four (where it first struck, as ours name a flu)
    const uint64_t ps = mix64(H.lore.seed ^ 0x51C4ULL);
    const int nl = (int)(sizeof(SICK_LOOK) / sizeof(SICK_LOOK[0])), nk = (int)(sizeof(SICK_KIND) / sizeof(SICK_KIND[0]));
    const int look = (int)((ps + (s % 4) * 7) % (uint64_t)nl), kind = (int)(((ps >> 16) + ((s >> 8) % 3) * 5) % (uint64_t)nk);
    if (((s >> 24) & 3) == 0) {
        int at = -1;
        for (const HistoryEvent& e : H.events) if (e.kind == HE_SICKNESS && e.thing == sickness && e.place >= 0) { at = e.place; break; }
        if (at >= 0) return "the " + historyPlaceName(H, at) + " " + SICK_KIND[kind];
        return "the " + H.lore.river + " " + SICK_KIND[kind];
    }
    return std::string("the ") + SICK_LOOK[look] + " " + SICK_KIND[kind];
}
std::string historyYear(const History& H, int year) {
    (void)H;
    if (year >= 1) return "year " + withCommas(year);
    if (year == 0) return "the year before the first";
    return "about " + withCommas(-year) + " years before the first";
}
std::string historySeason(const History& H, int season) {
    if (season < 0) return "";
    if (H.seasons == 4) return SEASONS4[season & 3];
    return SEASONS2[H.lore.desert ? 1 : 0][season & 1];
}
std::string historyTradeName(int trade) { return TRADES[clampi(trade / 10, 0, AGE_COUNT - 1)][clampi(trade % 10, 0, 9)]; }
std::string historyTradePlural(int trade) {
    std::string t = historyTradeName(trade);
    if (t.size() > 3 && t.compare(t.size() - 3, 3, "man") == 0) return t.substr(0, t.size() - 3) + "men";
    return t + "s";
}

namespace {
std::string workName(const History& H, int k) {
    const Genome G = genomeOf(H.seed);
    switch (k) {
        case WK_HOLY: { static const char* const HOLY[HOLY_COUNT] = {"a temple", "a sacred grove", "a high place", "a shrine at the spring", "a cave shrine", "a house of the god", "the great tombs", "an open altar", "a hall of the faith"}; return HOLY[clampi(H.profile.holy, 0, HOLY_COUNT - 1)]; }
        case WK_TOMB: { static const char* const DEAD[5] = {"a barrow", "a tomb", "a cemetery", "a tower of the dead", "a tomb"}; return DEAD[clampi(G.of(GV_DEAD), 0, 4)]; }
        case WK_MONUMENT: { static const char* const MN[5] = {"a row of stelae", "an obelisk", "a colossus", "a triumphal arch", "a pillar"}; return MN[clampi(G.of(GV_MONUMENTS), 0, 4)]; }
        case WK_FOCUS: { static const char* const FO[10] = {"a great spire", "a great dome", "a great gate", "a great tower", "a colossus", "a great mast", "a great ring", "a great obelisk", "a sacred pool", "a planted tree"}; return FO[clampi(G.of(GV_FOCUS), 0, 9)]; }
        default: return WORKS[clampi(k, 0, WK_COUNT - 1)].name;
    }
}
std::string cap1(std::string s) { if (!s.empty()) s[0] = (char)toupper(s[0]); return s; }
std::string people(const History& H) { return "the " + H.lore.people; }
std::string other(const History& H) { return "the " + (H.lore.other.empty() ? std::string("others") : H.lore.other); }
std::string dead(int64_t n) { return n <= 0 ? std::string() : "; " + withCommas(n) + " died"; }
const char* ofKind(int kind) {
    switch (kind) {
        case NK_PATH: return "path"; case NK_ROAD: return "road"; case NK_RAIL: return "railway"; case NK_CANAL: return "canal";
        case NK_HIGHWAY: return "highway"; case NK_MAGLEV: return "maglev"; default: return "crossing";
    }
}
}

std::string historyWorkName(const History& H, int work) { return workName(H, work); }   // H-04
const char* historyEverydayKey(int kind, int thing) {   // H-04's second pass
    for (int k = 0; k < EVERYDAY_N; k++) if (EVERYDAY[k].kind == kind) return thing >= 0 && thing < EVERYDAY[k].n ? EVERYDAY[k].words[thing].key : EVERYDAY[k].words[0].key;
    switch (kind) {
        case HE_ILLNESS: return wordAt(PRIVATE_ILLNESS, thing);
        case HE_INJURY: return wordAt(PRIVATE_INJURY, thing);
        case HE_HOUSE: return wordAt(PRIVATE_HOUSE, thing);
        case HE_QUARREL: return wordAt(PRIVATE_QUARREL, thing);
        case HE_JOURNEY: return wordAt(PRIVATE_JOURNEY, thing);
        default: return "";
    }
}
const char* historySignPattern(int cause, int stage, int alt) { return SCRIPTS[clampi(cause, 0, CAUSE_COUNT - 1)].signs[clampi(stage, 0, STAGE_COUNT - 1)][alt & 1]; }
const char* historyLeftWhy(int cause) { return LEFT_BY_CAUSE[clampi(cause, 0, CAUSE_COUNT - 1)]; }

std::string historyKinship(const History& H, int from, int to) {
    if (from < 0 || to < 0 || from >= (int)H.persons.size() || to >= (int)H.persons.size() || from == to) return "";
    const HistoryPerson& A = H.persons[from]; const HistoryPerson& B = H.persons[to];
    const bool f = A.female;
    auto parentsOf = [&](int p) { return std::pair<int, int>(H.persons[p].mother, H.persons[p].father); };
    auto isParent = [&](int p, int c) { return c >= 0 && (H.persons[c].mother == p || H.persons[c].father == p); };
    auto siblings = [&](int a, int b) { const auto pa = parentsOf(a), pb = parentsOf(b); return (pa.first >= 0 && pa.first == pb.first) || (pa.second >= 0 && pa.second == pb.second); };
    if (A.spouse == to || B.spouse == from) return f ? "wife" : "husband";
    if (isParent(from, to)) return f ? "mother" : "father";
    if (isParent(to, from)) return f ? "daughter" : "son";
    if (siblings(from, to)) return f ? "sister" : "brother";
    const auto pb = parentsOf(to), pa = parentsOf(from);
    for (int p : {pb.first, pb.second}) if (p >= 0 && isParent(from, p)) return f ? "grandmother" : "grandfather";
    for (int p : {pa.first, pa.second}) if (p >= 0 && isParent(to, p)) return f ? "granddaughter" : "grandson";
    for (int p : {pb.first, pb.second}) if (p >= 0 && siblings(from, p)) return f ? "aunt" : "uncle";
    for (int p : {pa.first, pa.second}) if (p >= 0 && siblings(to, p)) return f ? "niece" : "nephew";
    for (int p : {pa.first, pa.second}) for (int q : {pb.first, pb.second}) if (p >= 0 && q >= 0 && siblings(p, q)) return "cousin";
    if (B.spouse >= 0 && isParent(from, B.spouse)) return f ? "mother-in-law" : "father-in-law";
    if (A.spouse >= 0 && isParent(to, A.spouse)) return f ? "daughter-in-law" : "son-in-law";
    if (B.spouse >= 0 && siblings(from, B.spouse)) return f ? "sister-in-law" : "brother-in-law";
    if (A.spouse >= 0 && siblings(to, A.spouse)) return f ? "sister-in-law" : "brother-in-law";
    return "";
}

std::string historyEventLine(const History& H, int i, const HistoryPersonNamer* namer) {
    if (i < 0 || i >= (int)H.events.size()) return "";
    const HistoryEvent& e = H.events[i];
    // H-06: a person by the namer where one is given (the decoder's history page: only the persons the records named), else by their name
    auto P = [&](int p) { return namer ? (*namer)(p, true) : historyPersonName(H, p, true); };
    auto N = [&](int p) { return namer ? (*namer)(p, false) : historyPersonName(H, p, false); };
    auto L = [&](int p) { return historyPlaceName(H, p); };
    const std::string at = e.place >= 0 ? " at " + L(e.place) : "";
    std::string s;
    switch (e.kind) {
        case HE_COMING: {
            static const char* const WH[8] = {"from beyond {mountain}", "over {sea}", "out of the north", "out of the south", "out of the east", "out of the west", "down {river}", "across the dry country"};
            s = people(H) + " came into the land " + fillText(H, WH[clampi(e.thing, 0, 7)], -1, e.year); break;
        }
        case HE_FIRST: {
            static const char* const FI[6] = {"the first fields were sown", "the first herds were kept", "the first boats were made", "the first pots were fired", "the first copper was worked", "the first village stood where {capital} would be"};
            s = fillText(H, FI[clampi(e.thing, 0, 5)], -1, e.year); break;
        }
        case HE_STONES: s = "the stones were raised where " + L(0) + " would be" + (e.a >= 0 ? ", by " + P(e.a) : ""); break;
        case HE_FOUNDING: {
            static const char* const WHY[13] = {"", " as a fort", " as a harbour", " at a crossing of the river", " at a mine", " round a market", " round a shrine", " round a station", " as the gathering place", " as a band's winter seat", " as a great place", "", ""};
            if (e.thing == 11) { s = "the work at " + L(e.place) + " was begun" + (e.a >= 0 ? " by " + P(e.a) : ""); break; }
            if (e.thing == 12 || (e.place == 0 && e.year == 1)) { s = P(e.a) + " founded " + L(e.place) + (e.year == 1 ? ", and the years were counted from it" : ""); break; }
            s = P(e.a) + " founded " + L(e.place) + WHY[clampi(e.thing, 0, 12)] + (e.place2 >= 0 ? ", with settlers from " + L(e.place2) : ""); break;
        }
        case HE_ACCESSION: {
            const int g = e.value >= 0 && e.value < (int)H.reigns.size() ? H.reigns[e.value].government : GOV_CHIEFS;
            const std::string t = e.a >= 0 ? historyRulerTitle(H, g, H.persons[e.a].female) : "the mind";
            const std::string nm = N(e.a);
            switch (e.thing) {
                case 0: s = nm + " ruled as " + t; break;
                case 1: {
                    const std::string kin = e.b >= 0 ? historyKinship(H, e.b, e.a) : "";
                    s = nm + " became " + t + (e.b >= 0 ? " after " + N(e.b) + (kin.empty() ? "" : std::string(H.persons[e.a].female ? ", her " : ", his ") + kin) : "");
                    break;
                }
                case 2: s = nm + " was chosen " + t; break;
                case 3: s = nm + " was elected " + t; break;
                case 4: s = nm + " seized the rule as " + t; break;
                case 5: s = nm + " was appointed " + t; break;
                case 6: s = nm + " won the rule in the war and ruled as " + t; break;
                case 7: s = nm + " came of age and ruled alone" + (e.b >= 0 ? ", after the regency of " + N(e.b) : ""); break;
                case 8: s = "the rule was given to the mind"; break;
                case 9: s = nm + " was raised by the revolution as " + t; break;
                default: s = nm + " was elected " + t + " again"; break;
            }
            break;
        }
        case HE_RULE_END: {
            const std::string nm = P(e.a);
            switch (e.thing) {
                case 1: s = nm + " was deposed"; break;
                case 2: s = nm + " stepped down"; break;
                case 3: s = nm + "'s term ended"; break;
                case 4: s = nm + " was overthrown"; break;
                case 5: s = nm + " fled"; break;
                case 6: s = e.a >= 0 ? "the rule ended with " + nm + ", the last to hold it" : "the rule ended"; break;
                default: s = nm + " gave the rule over"; break;
            }
            break;
        }
        case HE_REGENCY: s = N(e.a) + " ruled for the child " + P(e.b); break;
        case HE_GOVERNMENT: {
            static const char* const HOW[9] = {" by a reform", " by a revolution", ": a kingdom was founded", ": the priests took the rule", ": the companies bought the rule", ": the machine was given the rule", " by a coup", ": the elders took the rule", ": the assembly was called"};
            s = "the rule passed from " + std::string(GOVERNMENT_NAMES[clampi((int)e.value, 0, GOV_COUNT - 1)]) + " to " + GOVERNMENT_NAMES[clampi(e.thing, 0, GOV_COUNT - 1)] + HOW[clampi(e.out, 0, 8)] + (e.a >= 0 ? ", led by " + N(e.a) : "");
            break;
        }
        case HE_WAR: {
            const HistoryWar& w = H.wars[clampi(e.thing, 0, (int)H.wars.size() - 1)];
            const std::string nm = historyWarName(H, e.thing);
            switch (w.kind) {
                case HW_REBELLION: s = L(w.sideB) + " rose against " + (H.reigns.empty() ? std::string("the rule") : "the rule of " + L(historySeat(H, e.year))) + " (" + nm + ")"; break;
                case HW_CIVIL: s = "civil war: " + L(w.sideB) + " against " + L(historySeat(H, e.year)) + " (" + nm + ")"; break;
                case HW_SUCCESSION: s = "two claimants fought for the rule: " + N(w.leaderA) + " and " + N(w.leaderB) + " (" + nm + ")"; break;
                case HW_REVOLUTION: s = "the revolution began" + at + (e.b >= 0 ? ", led by " + N(e.b) : ""); break;
                case HW_OTHER: s = "war with " + other(H) + (e.out == 0 ? ", begun by " + people(H) : ", begun by " + other(H)) + at; break;
                case HW_CONQUEST: s = other(H) + " came to conquer" + at; break;
                case HW_BANDS: s = "a feud between the bands of " + L(w.sideA) + " and " + L(w.sideB); break;
                default: s = nm + " began between " + L(w.sideA) + " and " + L(w.sideB); break;
            }
            break;
        }
        case HE_BATTLE: {
            const std::string nm = historyWarName(H, e.thing);
            const HistoryWar& w = H.wars[clampi(e.thing, 0, (int)H.wars.size() - 1)];
            std::string won;
            if (w.sideB == -2) won = e.out == 0 ? people(H) + " won" : other(H) + " won";
            else {
                const std::string sideA = w.sideA >= 0 ? L(w.sideA) : (w.kind == HW_REVOLUTION ? "the old rule" : "the rule of " + L(historySeat(H, e.year)));
                const std::string sideB = w.kind == HW_REVOLUTION ? "the revolution" : (w.kind == HW_SUCCESSION ? N(w.leaderB) : L(w.sideB));
                won = (e.out == 0 ? sideA : sideB) + " won";
            }
            const std::string under = e.a >= 0 ? N(e.a) : "";
            s = "the battle of " + L(e.place) + " in " + nm + ": " + won + (!under.empty() && won.compare(0, under.size(), under) != 0 ? " under " + under : "") + dead(e.value);
            break;
        }
        case HE_SIEGE: s = L(e.place) + " was besieged in " + historyWarName(H, e.thing) + (e.out ? " and fell" : " and held"); break;
        case HE_SACK: s = L(e.place) + " was sacked in " + historyWarName(H, e.thing) + dead(e.value); break;
        case HE_PEACE: {
            const HistoryWar& w = H.wars[clampi(e.thing, 0, (int)H.wars.size() - 1)];
            const std::string nm = historyWarName(H, e.thing);
            std::string res;
            if (e.out == 2) res = "neither side won";
            else switch (w.kind) {
                case HW_REBELLION: res = e.out == 0 ? L(w.sideB) + " submitted" : L(w.sideB) + " kept its own law"; break;
                case HW_REVOLUTION: res = e.out == 0 ? "the revolution was put down" : "the revolution won"; break;
                case HW_SUCCESSION: res = N(e.out == 0 ? w.leaderA : w.leaderB) + " won"; break;
                case HW_OTHER: case HW_CONQUEST: res = e.out == 0 ? people(H) + " won" : other(H) + " won"; break;
                default: res = (e.out == 0 ? (w.sideA >= 0 ? L(w.sideA) : L(historySeat(H, e.year))) : L(w.sideB)) + " won"; break;
            }
            s = nm + " ended: " + res + (w.dead > 0 ? " (" + withCommas(w.dead) + " dead in it)" : "");
            break;
        }
        case HE_SICKNESS: s = (e.out <= 1 ? historySicknessName(H, e.thing) + " came to " : historySicknessName(H, e.thing) + " came back to ") + L(e.place) + (e.out > 1 ? " (its " + ordinal(e.out) + " wave)" : "") + dead(e.value); break;
        case HE_HARVEST: {
            const int eco = H.eras[H.eraAt(e.year)].economy;
            const char* what = eco == ECO_HERDING ? "the herds" : (eco == ECO_HUNTING ? "the hunt" : "the harvest");
            if (e.out == 0) s = std::string(what) + (eco == ECO_HERDING ? " failed" : " failed");
            else if (e.out == 1) s = std::string("a poor year for ") + what;
            else s = std::string("a rich year for ") + what;
            break;
        }
        case HE_FAMINE: {
            const int eco = H.eras[H.eraAt(e.year)].economy;
            const char* after = eco == ECO_HERDING ? " after the herds failed" : (eco == ECO_HUNTING ? " after the hunt failed" : " after the harvest failed");
            s = "famine" + (e.place >= 0 ? " at " + L(e.place) + " and round it" : std::string(" in the land")) + (e.cause >= 0 && H.events[e.cause].kind == HE_HARVEST ? after : "") + dead(e.value);
            break;
        }
        case HE_FLOOD: s = e.year < 1 ? "the great flood, remembered ever after" : ((e.thing == 1 ? "the sea came over " : (e.place == 0 ? H.lore.river + " flooded " : "the river flooded ")) + L(e.place) + dead(e.value)); break;
        case HE_DROUGHT: s = e.year < 1 ? "the long drought, remembered ever after" : "drought for " + withCommas(e.value) + (e.value == 1 ? " year" : " years"); break;
        case HE_QUAKE: s = "an earthquake at " + L(e.place) + dead(e.value); break;
        case HE_ERUPTION: s = H.lore.mountain + " broke open near " + L(e.place) + dead(e.value); break;
        case HE_IMPACT: s = e.out == 2 ? "a great stone fell from the sky and darkened the world" + dead(e.value) : (e.out == 0 ? "a stone fell from the sky and ruined " + L(e.place) + dead(e.value) : "a stone fell from the sky near " + L(e.place) + dead(e.value)); break;
        case HE_FLARE: s = "the sun flared and burned the day side" + dead(e.value); break;
        case HE_WINTER:
            if (e.year < 1) s = "the long winter, remembered ever after";
            else if (e.value >= 25) s = "the long cold began, and lasted to the end";
            else s = "a winter of " + withCommas(e.value) + (e.value == 1 ? " year" : " years") + (e.cause >= 0 ? (H.events[e.cause].kind == HE_ERUPTION ? ", after the eruption" : ", after the stone from the sky") : "");
            break;
        case HE_BUILT:
            if (e.thing == WK_GREAT) { s = "the work at " + L(e.place) + " was finished after " + withCommas(e.value) + " years" + (e.a >= 0 ? ", in the keeping of " + P(e.a) : ""); break; }
            s = cap1(workName(H, e.thing)) + (e.thing == WK_WALLS || (e.thing == WK_HOLY && H.profile.holy == HOLY_TOMBS) ? " were finished at " : " was finished at ") + L(e.place) + " after " + withCommas(e.value) + (e.value == 1 ? " year" : " years") + (e.a >= 0 ? ", the work of " + P(e.a) : ""); break;
        case HE_WAY: s = std::string("the ") + ofKind(e.out) + " between " + L(e.place) + " and " + L(e.place2) + " was opened"; break;
        case HE_FESTIVAL: s = L(e.place) + " kept the " + ordinal((int)e.value) + " year of its founding"; break;
        case HE_OMEN: {
            static const char* const OM[6] = {"the comet returned", "the sun went dark at noon", "lights filled the night sky", "stars fell all night", "a new star shone for a season", "the moon turned red"};
            s = OM[clampi(e.thing, 0, 5)] + (e.thing != 0 && e.thing != 4 ? std::string(" over ") + L(e.place) : std::string());
            break;
        }
        case HE_INVENTION: {
            std::string nm = historyInventionName(e.thing);
            if (e.thing == INV_MOON) nm = H.lore.moons > 0 ? "a landing on " + H.lore.moon : "the first craft to another world";
            s = P(e.a) + " of " + L(e.place) + " made " + nm; break;
        }
        case HE_DISCOVERY: {
            static const char* const DI[9] = {"the far shore", "the source of {river}", "the pass over {mountain}", "that the world is round", "the far side of the world", "the poles", "the way to orbit", "the moon", "the other worlds of the sun"};
            s = P(e.a) + " found " + fillText(H, DI[clampi(e.thing, 0, 8)], e.place, e.year) + (e.thing == 0 && e.value >= 0 ? ", where " + L((int)e.value) + " would be founded" : ""); break;
        }
        case HE_FAITH: {
            switch (e.thing) {
                case 0: s = P(e.a) + " preached " + (e.value == FAITH_ONE_GOD ? "the faith of " + H.lore.god + ", the one god," : std::string("the faith of ") + FAITH_NAMES[clampi((int)e.value, 0, FAITH_COUNT - 1)]) + at; break;
                case 1: s = "a schism split the faith" + at; break;
                case 2: s = P(e.a) + " reformed the rites" + at; break;
                case 3: s = "the temples were closed"; break;
                default: s = "a persecution" + at; break;
            }
            break;
        }
        case HE_INSTITUTION: {
            const HistoryInstitution& I = H.institutions[clampi(e.thing, 0, (int)H.institutions.size() - 1)];
            static const char* const NM[HI_COUNT] = {"the Council of {place}", "the priesthood of {god}", "the guilds of {place}", "the Academy at {place}", "the Company of {place}", "the {place} Gazette",
                                                     "the University of {place}", "the Assembly", "the Broadcast of {place}", "the Network", "the Mind", "the Army of {place}"};
            s = cap1(fillText(H, NM[clampi(I.kind, 0, HI_COUNT - 1)], e.place, e.year)) + (I.kind == HI_GUILDS ? " were founded" : " was founded") + (e.a >= 0 ? " by " + P(e.a) : "");
            break;
        }
        case HE_DEED: {
            static const char* const DE[13] = {"made a song that outlived its maker", "wrote the chronicle of {capital}", "gave the laws", "drew the map of the land", "led the great hunt",
                                               "sailed beyond the known coasts", "healed the sick of {place}", "carved the great frieze at {place}", "taught at {place}", "wrote a book that everyone read",
                                               "made a theory of the world", "made the pictures that move", "made the gardens of {place}"};
            s = P(e.a) + " " + fillText(H, DE[clampi(e.thing, 0, 12)], e.place, e.year); break;
        }
        case HE_UNREST: {
            static const char* const UN[4] = {"a riot over bread", "a strike", "a rising", "a riot"};
            const bool early = H.eras[H.eraAt(e.year)].age == AGE_PREHISTORIC;
            s = (early ? (e.thing == 0 ? std::string("a fight over the food") : std::string("a quarrel between the bands")) : std::string(UN[clampi(e.thing, 0, 3)])) + at + dead(e.value); break;
        }
        case HE_MIGRATION: s = withCommas(e.value) + " moved from " + (e.place >= 0 ? L(e.place) : std::string("the countryside")) + " to " + (e.place2 >= 0 ? L(e.place2) : std::string("the cities")); break;
        case HE_ABANDONED: {
            static const char* const WHY[10] = {"", ": the river changed its course", ": the wells failed", ": the mine was worked out", ": the soil was spent", ": its people went to the cities",
                                                ": the band moved on", ": it was cleared and given back to the land", "", ": it was taken"};
            s = L(e.place) + " was left";
            if (e.thing == 8 || (e.thing == 0 && e.cause >= 0)) {
                // the harm that struck it, where it struck there; else the collapse's own reason
                std::string after;
                if (e.cause >= 0) {
                    const HistoryEvent& c = H.events[e.cause];
                    const bool here = c.place == e.place || c.place < 0;
                    switch (c.kind) {
                        case HE_SACK: if (c.place == e.place) after = " after the sack"; break;
                        case HE_SICKNESS: if (here) after = " after " + historySicknessName(H, c.thing); break;
                        case HE_FLOOD: if (c.place == e.place) after = " after the flood"; break;
                        case HE_QUAKE: if (c.place == e.place) after = " after the earthquake"; break;
                        case HE_ERUPTION: if (c.place == e.place) after = " after the eruption"; break;
                        case HE_IMPACT: if (here) after = " after the stone from the sky"; break;
                        case HE_FAMINE: if (c.place == e.place) after = " in the famine"; break;
                        default: break;
                    }
                }
                if (!after.empty()) s += after;
                else if (e.thing == 8) { const std::string why = LEFT_BY_CAUSE[clampi(H.cause, 0, CAUSE_COUNT - 1)]; s += why.empty() ? std::string(" in the end") : ": " + why; }
            } else s += WHY[clampi(e.thing, 0, 9)];
            break;
        }
        case HE_CONTACT: s = people(H) + " met " + other(H) + at + (e.a >= 0 ? "; " + P(e.a) + " went to them" : ""); break;
        case HE_TREATY: { static const char* const TR[3] = {"a pact of friendship", "a treaty of trade", "a border agreed"}; s = std::string(TR[clampi(e.thing, 0, 2)]) + " with " + other(H) + at; break; }
        case HE_OTHER_END: s = "word came that " + other(H) + " were gone"; break;
        case HE_SIGN: {
            const CauseScript& CS = SCRIPTS[clampi(H.cause, 0, CAUSE_COUNT - 1)];
            const int st = clampi(e.thing / 2, 0, STAGE_COUNT - 1), alt = e.thing & 1;
            s = fillText(H, CS.signs[st][alt], e.place, e.year) + (e.value > 0 ? dead(e.value) : "");
            break;
        }
        case HE_STAGE: s = std::string("the collapse: ") + STAGE_TITLES[clampi(e.out, 0, STAGE_COUNT - 1)] + " (" + CAUSE_NAMES[clampi(H.cause, 0, CAUSE_COUNT - 1)] + ")"; break;
        case HE_EXODUS: {
            static const char* const WHI[8] = {"for ", "for the mountains", "over the sea", "for the south", "for the north", "into the sky", "elsewhere", "into the machines"};
            const int w = clampi(e.thing, 0, 7);
            s = withCommas(e.value) + " left " + L(e.place) + " " + (w == 0 ? std::string("for ") + (e.place2 >= 0 ? L(e.place2) : std::string("the last places")) : std::string(WHI[w]));
            break;
        }
        case HE_END: s = "the end of " + people(H) + ": " + N(e.a) + " was the last of them, at " + L(e.place); break;
        // H-04's second pass: the everyday (the legends' plain line; the records tell them in their registers)
        case HE_FAIR: s = std::string(std::strcmp(historyEverydayKey(e.kind, e.thing), "gathering") == 0 ? "the bands gathered" : "a fair") + at + " (" + historyEverydayKey(e.kind, e.thing) + "; " + withCommas(e.value) + " came)"; break;
        case HE_FIRE: s = std::string("a fire") + at + " (" + historyEverydayKey(e.kind, e.thing) + "; " + withCommas(e.value) + " houses)"; break;
        case HE_STORM: s = std::string("a storm of ") + historyEverydayKey(e.kind, e.thing) + at + dead(e.value); break;
        case HE_PEST: s = std::string(historyEverydayKey(e.kind, e.thing)) + at; break;
        case HE_WONDER: s = std::string("a wonder: ") + historyEverydayKey(e.kind, e.thing) + at; break;
        case HE_GAMES: s = std::string("games of ") + historyEverydayKey(e.kind, e.thing) + at + (e.a >= 0 ? ", won by " + P(e.a) : ""); break;
        case HE_LAW: s = std::string("a law on ") + historyEverydayKey(e.kind, e.thing) + (e.a >= 0 ? " by " + P(e.a) : ""); break;
        case HE_TRIAL: { static const char* const V[4] = {"acquitted", "condemned", "exiled", "fined"}; s = (e.a >= 0 ? P(e.a) : std::string("someone")) + " tried for " + historyEverydayKey(e.kind, e.thing) + at + ", " + V[clampi(e.out, 0, 3)]; break; }
        case HE_EMBASSY: s = e.thing == 0 ? "envoys from " + L(e.place2) + " came to " + L(e.place) : (e.thing == 1 ? "envoys of " + other(H) + " came to " + L(e.place) : "envoys went to " + other(H)) + (e.a >= 0 ? " (" + P(e.a) + ")" : ""); break;
        case HE_ACCIDENT: s = std::string("a disaster: ") + historyEverydayKey(e.kind, e.thing) + at + dead(e.value); break;
        case HE_PRICE: s = std::string("the price of ") + historyEverydayKey(e.kind, e.thing) + (e.out ? " rose " : " fell ") + std::to_string(e.value) + "%"; break;
        case HE_BEASTS: s = std::string("the herds: ") + historyEverydayKey(e.kind, e.thing) + at + (e.value > 0 ? dead(e.value) : ""); break;
        case HE_SHOW: s = std::string("a new ") + historyEverydayKey(e.kind, e.thing) + at + (e.a >= 0 ? " by " + P(e.a) : ""); break;
        case HE_VISIT: s = P(e.a) + " came to " + L(e.place); break;
        case HE_BIRTH: s = N(e.a) + " was born to " + N(e.b) + " and " + N(e.c) + at; break;
        case HE_MARRIAGE: s = N(e.a) + " married " + N(e.b) + at; break;
        case HE_MOVE: s = N(e.a) + " moved from " + L(e.place) + " to " + L(e.place2); break;
        case HE_ILLNESS: s = N(e.a) + " was ill (" + historyEverydayKey(e.kind, e.thing) + ") and got well" + at; break;
        case HE_INJURY: s = N(e.a) + " hurt " + std::string(H.persons[e.a].female ? "her " : "his ") + historyEverydayKey(e.kind, e.thing) + at; break;
        case HE_APPRENTICE: s = N(e.a) + " was apprenticed to a " + historyTradeName(e.thing) + at; break;
        case HE_HOUSE: s = N(e.a) + "'s house: " + historyEverydayKey(e.kind, e.thing) + at; break;
        case HE_QUARREL: s = N(e.a) + " quarrelled" + (e.b >= 0 ? " with " + N(e.b) : std::string()) + " over the " + historyEverydayKey(e.kind, e.thing) + at; break;
        case HE_JOURNEY: s = N(e.a) + " went to " + L(e.place2) + " (" + historyEverydayKey(e.kind, e.thing) + ") and came back"; break;
        case HE_BETROTHAL: s = N(e.a) + " and " + N(e.b) + " were promised" + at; break;
        case HE_DEATH: {
            static const char* const HOW[16] = {"", " of a sickness", " in childbirth", " in battle", " in the famine", " in the flood", " in the earthquake", " in an accident", "", "",
                                               " in the end", " young", " in the fire from the sky", " of the cold", " at sea", " in the hard years"};
            const HistoryPerson& Q = H.persons[clampi(e.a, 0, (int)H.persons.size() - 1)];
            const int years = e.year - Q.born;
            std::string how = HOW[clampi(e.thing, 0, 15)];
            if (e.thing == 1 && e.cause >= 0 && H.events[e.cause].kind == HE_SICKNESS) how = " of " + historySicknessName(H, H.events[e.cause].thing);
            if (e.thing == 3 && e.cause >= 0 && H.events[e.cause].kind == HE_BATTLE) how = " at the battle of " + L(H.events[e.cause].place);
            std::string who = P(e.a);
            if (e.pub) {   // who they were
                switch (Q.role) {
                    case HR_FOUNDER: for (int q = 0; q < (int)H.places.size(); q++) if (H.places[q].founder == e.a) { who += ", who founded " + L(q) + ","; break; } break;
                    case HR_GENERAL: who = "the general " + who; break;
                    case HR_PROPHET: who = "the prophet " + who; break;
                    case HR_EXPLORER: who = "the explorer " + who; break;
                    case HR_REBEL: who = "the rebel " + who; break;
                    case HR_REGENT: who = "the regent " + who; break;
                    case HR_INVENTOR: for (const HistoryInvention& I : H.inventions) if (I.person == e.a) { who += ", who made " + historyInventionName(I.id) + ","; break; } break;
                    default: break;
                }
            }
            const char* verb = e.thing == 8 ? " was murdered" : (e.thing == 9 ? " was put to death" : " died");
            s = who + verb + how + (e.place >= 0 && e.thing != 3 ? " at " + L(e.place) : "") + (e.thing == 11 || years < 1 ? "" : ", aged " + std::to_string(years));
            break;
        }
        default: s = HISTORY_EVENT_NAMES[clampi(e.kind, 0, HE_COUNT - 1)]; break;
    }
    return cap1(s);
}
