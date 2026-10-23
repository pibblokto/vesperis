// H-02 (2026-10-09): the history (`docs/ideas/IDEAS-civilisation-expansion.md`, section 8; the data model of
// `docs/ideas/RESEARCH-records.md`, section 6). A people gets a history before its towns, its works and its records are made, so
// they can read it (Dwarf Fortress's order: the history first, then the sites' detail and the legends that tell it). Deterministic
// from the profile (A-01: the age, the eras and their governments, the cause of the collapse and its stages, the peak, the values),
// the plan (T-01: the places by their rank in the founding, the territory, the ways) and the world (its rivers and coasts, its
// volcanoes, its comets and belts, a red dwarf's flares, an eccentric orbit's winters, its moons): built in a few milliseconds.
//
// * The eras: one per age passed, each with the people's name for it, its government (changed once within it, by a reform, a
//   revolution or a founding of a kingdom), its faith and what it lived by.
// * The places: the plan's, founded in its order (the capital at year 1, the reckoning's start), each in the era whose ground
//   suited it (a modern people's old cities in the medieval age, its mill towns in the industrial), some left early (sacked,
//   struck by a sickness, a flood, the mine closed), every one left by the end (the collapse's order, the last place last); a
//   futuristic or far-future people's older cities, which its plan no longer draws, kept as places it cleared.
// * The persons: the rulers by the government (a monarchy's houses and heirs, regents, usurpers; chiefs, elders, priests, consuls
//   by their terms; the machine's rule a person of none), the figures (founders, generals, prophets, makers, inventors, explorers,
//   rebels, the last), and ten families followed through the last thousand years: births, marriages, moves and deaths with their
//   causes, so a record can say "my daughter-in-law" and be right.
// * The events: foundings, accessions and deaths of rulers, reforms, wars with their battles, sieges and peace, sicknesses in
//   waves, harvests and famines, floods, droughts, earthquakes, eruptions, impacts, flares, long winters, works finished, ways
//   opened, festivals, omens (the comet's real returns), inventions, discoveries, faiths, institutions, unrest, migrations, the
//   other people's meeting and wars (C-13, shared by both histories), and the collapse by its cause in its four stages (the signs,
//   the onset, the crisis, the end) to its last place and its last person. Each keeps its cause (a famine its failed harvest, a
//   battle its war, a death the sickness or the battle that caused it, in its season).
//
// Not drawn yet: T-02 reads the places' founding ages for the old cores and an early end for an older style, T-03 the battles and
// the works, T-04 the last place for the last recording, H-04 the whole of it. `vesperis_test history <sx> <sz> <body>` prints it
// as Dwarf Fortress's legends would.
#pragma once
#include "peoples.h"
#include "plan.h"
#include "shards.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

// the works a people finished (HE_BUILT's `thing`): the ages that built them and what each needs are history.cpp's table; T-03 lays the ones
// outside the towns on the ground (galaxy/works.h: a dam, a mine, an aqueduct, an observatory, a lighthouse, a launch site, the holy place)
enum HistoryWork { WK_HOLY = 0, WK_TEMPLE, WK_WALLS, WK_FORTRESS, WK_PALACE, WK_BRIDGE, WK_AQUEDUCT, WK_HARBOUR, WK_MARKET, WK_LIBRARY, WK_OBSERVATORY, WK_TOMB,
                   WK_MONUMENT, WK_THEATRE, WK_LIGHTHOUSE, WK_GRANARY, WK_MILL, WK_FOCUS, WK_STONES, WK_DAM, WK_MINE, WK_STATION, WK_FACTORY, WK_SCHOOL,
                   WK_HOSPITAL, WK_TOWER, WK_STADIUM, WK_POWER, WK_MAST, WK_LAUNCH, WK_ARCOLOGY, WK_HABITAT, WK_ELEVATOR, WK_RING, WK_GREAT, WK_COUNT };

// what happened (the slots each kind fills: `a`, `b`, `c` persons; `place`, `place2` the history's places; `thing` the kind's own)
enum HistoryEventKind {
    // the old time before year 1, as the people remembered it (the years round)
    HE_COMING = 0,     // the people came into the land (thing: whence, `HISTORY_WHENCE`)
    HE_FIRST,          // a first of the old time (thing: `HISTORY_FIRSTS`: the first fields, herds, boats, pots, metal)
    HE_STONES,         // the stones raised (place: where; thing: the genome's monuments)
    // the places and the rule
    HE_FOUNDING,       // place founded (a: the founder; place2: the place its settlers came from; thing: why, `HISTORY_FOUNDINGS`)
    HE_ACCESSION,      // a ruler (a) came to the rule after b (thing: how, `HISTORY_ACCESSIONS`; value: the reign's index)
    HE_RULE_END,       // a ruler's rule ended alive (a; thing: how, `HISTORY_RULE_ENDS`)
    HE_REGENCY,        // a ruled for the child b
    HE_GOVERNMENT,     // the form of rule changed (thing: the new form; value: the old; out: how, `HISTORY_GOV_CHANGES`; a: who led it)
    // war
    HE_WAR,            // a war began (thing: the war; a, b: the two sides' leaders; place: what it was over)
    HE_BATTLE,         // a battle of the war (thing) at place (a: the winners' leader, b: the losers'; out: the side that won; value: the dead)
    HE_SIEGE,          // place besieged (thing: the war; out: 1 it fell)
    HE_SACK,           // place sacked (thing: the war; value: the dead)
    HE_PEACE,          // the war (thing) ended (out: `HISTORY_WAR_ENDS`; a, b: who made the peace)
    // the land and the sky
    HE_SICKNESS,       // a sickness (thing: `History::sicknesses`) struck place (out: the wave, 1 the first; value: the dead)
    HE_HARVEST,        // a harvest failed (out: 0 failed, 1 poor) or was rich (4)
    HE_FAMINE,         // famine (place, or -1 the whole land; value: the dead; cause: the failed harvest, the war, the flood)
    HE_FLOOD,          // place flooded (value: the dead; thing: 0 the river, 1 the sea)
    HE_DROUGHT,        // drought (value: its years)
    HE_QUAKE,          // an earthquake at place (value: the dead)
    HE_ERUPTION,       // a mountain broke open near place (value: the dead)
    HE_IMPACT,         // a stone fell from the sky near place (value: the dead; out: 0 a town's ruin .. 2 the world darkened)
    HE_FLARE,          // the sun flared (value: the dead)
    HE_WINTER,         // a long winter (value: its years; cause: the eruption or the impact)
    // what they made and found
    HE_BUILT,          // a work finished at place (thing: `HISTORY_WORKS`; a: its maker; value: the years of work)
    HE_WAY,            // a way opened between place and place2 (thing: the plan's edge; out: its NetKind)
    HE_FESTIVAL,       // place's anniversary (value: its years)
    HE_OMEN,           // a sign in the sky (thing: `HISTORY_OMENS`; place: where it was seen)
    HE_INVENTION,      // an invention (thing: `History::inventions`; a: who; place)
    HE_DISCOVERY,      // a discovery (thing: `HISTORY_DISCOVERIES`; a: who; place, place2)
    HE_FAITH,          // a faith's change (thing: `HISTORY_FAITH_EVENTS`; a: the prophet; value: the faith)
    HE_INSTITUTION,    // an institution founded (thing: `History::institutions`; a: its founder; place)
    HE_DEED,           // a figure's deed (a; thing: `HISTORY_DEEDS`; place)
    HE_UNREST,         // a riot, a strike, a rising at place (thing: `HISTORY_UNREST`; value: the dead)
    HE_MIGRATION,      // many moved from place to place2 (value: how many; thing: why)
    HE_ABANDONED,      // place left (thing: why, `HISTORY_LEFT`; cause: the event that ended it)
    // the other people (C-13: both histories hold the same)
    HE_CONTACT,        // the first meeting with the other people (place: ours; other: theirs; a: our envoy)
    HE_TREATY,         // a treaty with the other people (thing: `HISTORY_TREATIES`)
    HE_OTHER_END,      // word that the other people had ended
    // the collapse
    HE_SIGN,           // a sign of the collapse (thing: `historySignText`'s index for the cause; place; out: the stage)
    HE_STAGE,          // the collapse's stage began (out: the stage)
    HE_EXODUS,         // they left place (value: how many; thing: whither, `HISTORY_WHITHER`)
    HE_END,            // the end: the last place (place), the last person (a)
    // H-04's second pass: the everyday (`HISTORY_EVERYDAY`: each its own stream of a year, no harm and no person made, so the history
    // above stays as H-02 drew it); `thing` indexes the kind's own words (`historyEverydayKey`)
    HE_FAIR,           // a fair or a gathering at place (thing: what it was for; value: how many came)
    HE_FIRE,           // a fire at place (thing: what burnt; value: the houses)
    HE_STORM,          // a storm (thing: its kind; place; value: the dead, 0 often)
    HE_PEST,           // the fields' or the herds' plague (thing: locusts, blight, murrain, rats...; place)
    HE_WONDER,         // a wonder told about (thing: a calf with two heads, a great fish, a hoard found...; place)
    HE_GAMES,          // games or a contest at place (thing: its kind; a: the winner, -1 none told)
    HE_LAW,            // a law or a decree (thing: what it ruled on; a: the ruler or none)
    HE_TRIAL,          // a trial (thing: the charge; a: the accused; out: 0 acquitted, 1 condemned, 2 exiled, 3 fined)
    HE_EMBASSY,        // envoys (thing: 0 from place2 to place, 1 from the other people, 2 sent to them; a: the envoy, -1 none)
    HE_ACCIDENT,       // a works' disaster (thing: a bridge fell, a mine, a ship, a train...; place; value: the dead)
    HE_PRICE,          // a price that rose or fell (thing: bread, salt, oil...; out: 1 rose, 0 fell; value: by how much, a percent)
    HE_BEASTS,         // the herds and the game (thing: late, early, none, many, a great one taken, one that killed; value: the dead)
    HE_SHOW,           // a play, a song, pictures, a film shown first (thing: its kind; a: who made it; place)
    HE_VISIT,          // the ruler at place (a: the ruler)
    HE_SUN,            // D-05: the loss of the sun seen (thing: 0 the sun smaller, 1 the year that did not end, 2 the sea frozen from the shore, 3 the air
                       // falling as snow, 4 the last sunrise, 5 the stars at noon; place: where it was seen)
    // a family's (private)
    HE_BIRTH,          // a born to b (mother) and c (father) at place
    HE_MARRIAGE,       // a married b at place
    HE_MOVE,           // a moved from place to place2 (cause: the place's end, a war...)
    HE_DEATH,          // a died at place (thing: how, `HISTORY_DEATHS`; cause: the sickness, the battle, the famine, the end)
    // H-04's second pass: a family's everyday (private; each person's own stream, after the years are drawn: nothing above moves).
    // `thing` indexes the kind's words (`historyEverydayKey`), the apprenticeship's is the trade (`historyTradeName`)
    HE_ILLNESS,        // a was ill and got well (thing: what it was)
    HE_INJURY,         // a was hurt (thing: where: an arm, a leg...)
    HE_APPRENTICE,     // a (a child) was apprenticed (thing: the trade)
    HE_HOUSE,          // a's household and its house (thing: built, bought, sold, burnt, flooded, mended, a room added)
    HE_QUARREL,        // a quarrelled with b of another family (thing: over what; b may be -1)
    HE_JOURNEY,        // a went to place2 and came back (thing: what for)
    HE_BETROTHAL,      // a and b were promised (the year before their wedding)
    HE_COUNT
};
extern const char* const HISTORY_EVENT_NAMES[HE_COUNT];   // "coming", "first", ... (the harness)

// what a person was to the history
enum HistoryRole { HR_NONE = 0, HR_RULER, HR_REGENT, HR_FOUNDER, HR_GENERAL, HR_PROPHET, HR_MAKER, HR_INVENTOR, HR_EXPLORER, HR_REBEL, HR_ENVOY, HR_ARTIST, HR_LAST, HR_COUNT };
extern const char* const HISTORY_ROLE_NAMES[HR_COUNT];   // "", "ruler", "regent", "founder", "general", "prophet", "maker", ...
// how a person ended: died, left with the exodus, or followed no further (a branch of a family the history let go: its death drawn alone)
enum HistoryFate { HF_DIED = 0, HF_LEFT, HF_LET_GO };
// how a place ended
enum HistoryPlaceFate { HP_COLLAPSE = 0, HP_EARLY, HP_LAST, HP_CLEARED };
// what a war was
enum HistoryWarKind { HW_BANDS = 0, HW_CITIES, HW_REBELLION, HW_SUCCESSION, HW_REVOLUTION, HW_CIVIL, HW_OTHER, HW_CONQUEST, HW_COUNT };
extern const char* const HISTORY_WAR_KINDS[HW_COUNT];   // "a war between bands", "a war between cities", "a rising", ...

struct HistoryEvent {
    int kind = HE_FOUNDING;
    int year = 0;
    int season = -1;          // 0 .. History::seasons - 1 (-1 the year's, no season said)
    bool pub = true;          // public (the chronicle's) or a family's
    float salience = 0.5f;    // how much it mattered (0 .. 1): the chronicle's pick, H-04's
    int place = -1, place2 = -1;   // the history's places
    int a = -1, b = -1, c = -1;    // persons
    int cause = -1;           // the event that caused it
    int thing = 0;            // the kind's own (see the kinds)
    int out = 0;              // an outcome (see the kinds)
    int other = -1;           // the other people's place (its plan's rank), C-13
    int64_t value = 0;        // a count: the dead, the people, the years
};

struct HistoryPerson {
    uint64_t name = 0;        // the name's seed (`historyPersonName`): a ruler named after an ancestor shares it
    int ordinal = 0;          // a ruler's number among the rulers of that name (0 none: the only one)
    bool female = false;
    int born = 0, died = 0;   // years (died: the last year of the record, the year they left for HF_LEFT)
    int fate = HF_DIED;
    int death = -1;           // the HE_DEATH event (-1 none told: a figure of the old time)
    int family = -1;          // the line they were born to or married into
    int mother = -1, father = -1, spouse = -1;   // spouse: the last
    int home = -1;            // the place they lived last (-1 the countryside's or unknown)
    int role = HR_NONE;
    int trade = 0;            // `historyTrade`: by the age
    uint32_t traits = 0;      // `HistoryTrait` bits
};
enum HistoryTrait { HT_BRAVE = 1, HT_CRUEL = 2, HT_PIOUS = 4, HT_WISE = 8, HT_LEARNED = 16, HT_AMBITIOUS = 32, HT_KIND = 64, HT_PROUD = 128 };

struct HistoryPlace {
    int plan = -1;            // its rank in the people's plan, -1 a place the plan does not draw (a later people's cleared city)
    int sclass = SC_TOWN;
    uint64_t name = 0;        // the name's seed (the capital's and the second's are the lore's city and city2)
    int founded = 1, ended = 1;   // the first and the last year anyone lived there
    int fate = HP_COLLAPSE;
    int endEvent = -1;        // the HE_ABANDONED or HE_END event
    int age = AGE_MEDIEVAL;   // the age it was founded in (its old core's way)
    int founder = -1;
    int from = -1;            // the place its first settlers came from
    double lat = 0, lon = 0;
    double peak = 0;          // how many lived there at the height
    bool coast = false, river = false;
};

struct HistoryEra {
    int age = AGE_MEDIEVAL;
    int start = 1, end = 1;
    std::string name;         // the people's own ("the Old Time", "the Kingdom of Kauza", "the Engine Years")
    int government = GOV_CHIEFS;   // at its close (A-01's)
    int govFrom = GOV_CHIEFS;      // at its start (the previous era's close; chiefs before the first)
    int govChange = 0;             // the year it changed (0 it did not)
    int faith = FAITH_ANCESTORS, economy = ECO_FARMING;
};
struct HistoryReign {
    int ruler = -1;           // a person; -1 the machine's rule
    int start = 1, end = 1;
    int government = GOV_CHIEFS;
    int house = -1;           // a monarchy's dynasty (a family)
    int how = 0;              // how it began (`HISTORY_ACCESSIONS`)
    int endHow = 0;           // how it ended (`HISTORY_RULE_ENDS`; 0 died)
    int regent = -1;          // who ruled for a child
};
struct HistoryWar {
    int kind = HW_CITIES;
    int start = 1, end = 1;
    int sideA = -1, sideB = -1;   // the places at their heads (-1 the crown: the capital's; -2 the other people)
    int leaderA = -1, leaderB = -1;
    int winner = -1;          // 0 A, 1 B, -1 neither
    int64_t dead = 0;
    int declared = -1, peace = -1;   // events
    uint64_t name = 0;        // the name's seed (`historyWarName`)
    int battles = 0;
};
struct HistorySickness { uint64_t name = 0; int first = 0, last = 0; int waves = 0; int64_t dead = 0; bool made = false; };
struct HistoryInstitution { int kind = 0; int founded = 0, ended = 0; int place = -1; int founder = -1; uint64_t name = 0; };
enum HistoryInstitutionKind { HI_COUNCIL = 0, HI_PRIESTHOOD, HI_GUILDS, HI_ACADEMY, HI_COMPANY, HI_GAZETTE, HI_UNIVERSITY, HI_ASSEMBLY, HI_BROADCASTER, HI_NETWORK, HI_MIND, HI_ARMY, HI_COUNT };
extern const char* const HISTORY_INSTITUTION_KINDS[HI_COUNT];
struct HistoryFamily {
    uint64_t name = 0;        // the surname's seed (kept from the industrial age, when registers keep them)
    int founder = -1;         // the first of it the history follows (a house's founder names it)
    int head = -1;            // its head now (the main line's: a ruling house's ruler)
    int home = -1;
    int trade = 0;
    int from = 0, to = 0;     // the years it was followed
    bool house = false;       // a ruling house
    int persons = 0;
};
struct HistoryInvention { int id = 0; int year = 0; int person = -1; int place = -1; };

struct History {
    uint64_t seed = 0;
    int people = 0, peoples = 1, age = AGE_MEDIEVAL;
    int first = 1, last = 1;              // the oldest year told and the end (the people's count)
    int cause = CAUSE_FAMINE;
    int stageYear[STAGE_COUNT] = {1, 1, 1, 1};   // the first year of each stage of the collapse
    int famFrom = 1;                      // the families are followed from this year
    int seasons = 4;                      // the world's: four (a tilted world), else two
    Lore lore;                            // the names (C-02's: the people, the god, the capital, the founder...)
    PeopleProfile profile;
    std::vector<HistoryEra> eras;
    std::vector<HistoryPlace> places;     // the plan's by its rank first, then the cleared
    std::vector<HistoryPerson> persons;
    std::vector<HistoryFamily> families;
    std::vector<HistoryReign> reigns;     // in order
    std::vector<HistoryWar> wars;
    std::vector<HistorySickness> sicknesses;
    std::vector<HistoryInstitution> institutions;
    std::vector<HistoryInvention> inventions;   // in order
    std::vector<HistoryEvent> events;     // by year, season, then as they happened
    std::vector<int> wayOpened;           // per edge of the people's plan: the year its way opened (0 never)
    int lastPlace = -1, lastPerson = -1;
    int publicEvents = 0;
    double buildMs = 0;
    // the indices: each person's and each place's events (offsets into the lists), the harvest and the people's number by year
    std::vector<int> personFirst, personEvents, placeFirst, placeEvents;
    int yearsFrom = 1;                    // `harvest` and `population` start here
    std::vector<uint8_t> harvest;         // 0 failed .. 4 rich
    std::vector<float> population;
    std::vector<float> sunlight;          // D-05: a drifter's people's sun by year, its light over their old sun's (1 before the loss, nothing after the
                                          // last sunrise but the stars'); empty for every other people

    int eventsIn(int year, int& from) const;          // the events of a year: their count, the first's index in `from`
    int reignAt(int year) const;                      // the reign of a year (-1 none: before year 1, after the end)
    int eraAt(int year) const;
    int harvestAt(int year) const;                    // 0 .. 4 (2 an ordinary year; 2 where none is kept)
    double populationAt(int year) const;
    double sunlightAt(int year) const;                // D-05: 1 where none is kept
    bool placeStands(int place, int year) const { return place >= 0 && place < (int)places.size() && year >= places[place].founded && year <= places[place].ended; }
    bool alive(int person, int year) const { return person >= 0 && person < (int)persons.size() && year >= persons[person].born && year <= persons[person].died; }
    int inventionYear(int id) const;                  // 0 never
    std::vector<int> eventsOf(int person) const;
    std::vector<int> eventsAt(int place) const;
};

// the history of a world's people: cached by the world and the people (the last eight); builds the world's plan first if it must
std::shared_ptr<const History> historyOf(const StarSystem& sys, const Body& b, const BodyGen& g, int people = 0);
std::shared_ptr<History> buildHistory(const StarSystem& sys, const Body& b, const BodyGen& g, int people = 0);   // not cached
// T-02: the same from the world's planet function alone (its star's sector and its index, `BodyGen::starSx`): the system regenerated when
// the history is not cached yet; null for a generator made by hand (no star). The towns read their places' founding ages and fates here
std::shared_ptr<const History> historyOfWorld(const BodyGen& g, int people = 0);

// the names and the lines (the harness's legends; the decoder's timeline later)
std::string historyPersonName(const History& H, int person, bool withTitle = false);   // "Nilzhos", "King Nilzhos II", "Tess Kauri"
std::string historyPlaceName(const History& H, int place);
std::string historyFamilyName(const History& H, int family);   // H-04's second pass: a family's name in the people's tongue ("" none)
std::string historyRulerTitle(const History& H, int gov, bool female);   // H-04's second pass: the people's own title of a government's ruler ("headman", "archon", a word of theirs)
bool historyTitleNative(const History& H, int gov);   // that title is a word of their tongue (kept as a name)
int historySeat(const History& H, int year);                    // the place the rule sat in that year (a far-future people's old capital until it was cleared)
std::string historyWarName(const History& H, int war);
std::string historySicknessName(const History& H, int sickness);
std::string historyInventionName(int id);
int historyInventionIndex(const char* name);   // D-05: an invention's id by its name ("radio"), -1 none
std::string historyYear(const History& H, int year);           // "year 512", "about 4,000 years before the founding"
std::string historySeason(const History& H, int season);       // "spring", "the dry months"
std::string historyTradeName(int trade);
std::string historyTradePlural(int trade);                      // "millers", "boatmen"
// one sentence, its year left out. H-06: `namer` names the persons in it (the decoder's history page names only those the read records
// named, the rest by what they were: "a woman", "a child"); null: by their names, the harness's legends
using HistoryPersonNamer = std::function<std::string(int person, bool withTitle)>;
std::string historyEventLine(const History& H, int event, const HistoryPersonNamer* namer = nullptr);
const char* historyEverydayKey(int kind, int thing);           // H-04's second pass: an everyday kind's own word for its `thing` ("locusts", "bridge")
// what `from` was to `to` ("mother", "son-in-law", "cousin"; "" none the history knows)
std::string historyKinship(const History& H, int from, int to);
// H-04: the words the records take from the history's own tables: a work's name ("a great temple"; the genome's "a barrow"), a sign
// of the collapse as its cause's script has it ("the wells of {place} sank": the marks `{place}`, `{capital}`, `{river}`, `{sea}`,
// `{mountain}`, `{god}`, `{other}` left for the caller), why a place was left in the collapse ("" where the end took all at once)
std::string historyWorkName(const History& H, int work);
const char* historySignPattern(int cause, int stage, int alt);
const char* historyLeftWhy(int cause);
