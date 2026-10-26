// Where a people's records lie (T-04; docs/reference/civilisations/records-and-grammar.md): 80 to 200 of them (`recordCountOf`, by its age
// and its numbers), the first fifty the shards' (their draws kept), the genres writing them (peoples/genres.h) and the age sorting them
// into texts, music and star charts. Each is dated in the years of the age the people reached, the later the likelier (`recordYear`; the
// last recording is the end's), made in its age's medium (`RecordMedium`: bone and stone, tablets and scrolls, printed pages and cylinders,
// reels and drives, crystals) and lies where its age kept such things (`recordSitesOf`: in a settlement's buildings, a row in an archive,
// the works that kept records, the last recording at the history's last place and in the last shelter). A place left early holds nothing
// written after it was left; a copy taken is gone everywhere on the world (the guide's keys `<body>/S<index>`, a people's records at
// `people x SHARD_STRIDE + index`).
#pragma once
#include "shards.h"
#include "peoples.h"
#include <memory>
#include <vector>
#include <functional>

enum RecordMedium {
    RM_BONE = 0,   // carved bone: a tally, a flute, a sky's marks
    RM_STONE,      // carved stone: a small slab with its signs
    RM_PAINTED,    // a painted wall (or a painted slab where no wall stands): figures, a sky
    RM_TABLET,     // a clay tablet
    RM_SCROLL,     // a scroll on its rods
    RM_CODEX,      // a bound book
    RM_PLATE,      // an engraved plate (an astrolabe's)
    RM_CYLINDER,   // a wax cylinder (a voice, a piece)
    RM_GLASS,      // a photographic plate of glass
    RM_PRINTED,    // printed pages
    RM_REEL,       // a reel of tape
    RM_DISC,       // a disc in its sleeve
    RM_DRIVE,      // a drive
    RM_CRYSTAL,    // a crystal
    RM_HUM,        // something that hums
    RM_COUNT
};
extern const char* const RECORD_MEDIUM_NAMES[RM_COUNT];   // "carved bone", ... "something that hums" (the harness)
extern const char* const RECORD_MEDIUM_HUD[RM_COUNT];     // "A CARVED BONE", ... "SOMETHING THAT HUMS" (the HUD, the log)
enum RecordCategory { RC_TEXT = 0, RC_MUSIC, RC_CHART, RC_COUNT };
extern const char* const RECORD_CATEGORY_NAMES[RC_COUNT];   // "text", "music", "star chart"

constexpr int RECORDS_MIN = 80, RECORDS_MAX = 200;
constexpr int SHARD_STRIDE = 1000;   // a people's records in the world's indices (the guide's keys, the view's sites): people x 1000 + index
inline int shardPeopleOf(int index) { return index / SHARD_STRIDE; }   // C-13: whose records a world index is from
inline int shardLocalOf(int index) { return index % SHARD_STRIDE; }    // C-13: its place in that people's records
inline int shardWorldIndex(int people, int local) { return people * SHARD_STRIDE + local; }

// how many records a people left: 80 to 200 by its age and its numbers at its height (the generator alone: the cells, the radar, the decoder)
int recordCountOf(const BodyGen& g, int people = 0);
int shardsOfWorld(const BodyGen& g);   // C-13: the world's whole collection (its peoples' records together)

struct RecordEntry {
    int year = 1;             // when it was made, in the people's count (1 the founding .. the end)
    int age = AGE_MEDIEVAL;   // its year's age
    int category = RC_TEXT;
    int medium = RM_STONE;
    bool last = false;        // C-12's last recording: the end's year
};
struct RecordCatalogue {
    int people = 0, count = 0, last = -1;
    int firstYear = 1, lastYear = 1;
    int music = 0, charts = 0;
    std::vector<RecordEntry> entries;   // by index
    double buildMs = 0;
};
// a people's records (the world's generator): cached by the world and the people (the last eight)
std::shared_ptr<const RecordCatalogue> recordsOf(const BodyGen& g, int people = 0);
// the age of a year of the people's count (the era it falls in; `f` how far into it, 0 its start .. 1 its end)
int recordAgeAt(const PeopleAge& pa, int year, double* f = nullptr);
// whether a record of a people with `count` records is music, which of its star charts it is (-1 none), how many charts it has (`pg` the
// people's generator, `peopleGen`): the first fifty C-04's and C-10's; H-05: past them by the age of each record's year, the age's share
// (`recordMusicShare`, `recordChartShare`) leaned by the people's own fifty (its pieces against fifteen, its charts against four)
bool recordIsMusic(const BodyGen& pg, int count, int index);
int recordChartIndex(const BodyGen& pg, int count, int index);
int recordChartCount(const BodyGen& pg, int count);
void recordChartRecords(const BodyGen& pg, int count, std::vector<int>& out);   // H-05: the record index of each chart, by the chart's index (its mark's)
double recordMusicShare(int age);   // H-05: 0.36, 0.22, 0.25, 0.32, 0.24, 0.26 from the prehistoric to the far future
double recordChartShare(int age);   // H-05: 0.09, 0.08, 0.07, 0.06, 0.12, 0.15
bool recordMediumFits(int medium, int age, int category);   // a medium an age made a record of the category in (the checks)
// H-05: whether a record made on that medium carries a sound of its own (a wax cylinder, a reel, a disc, a drive, a crystal, something that
// hums): the voice itself or the piece as it was played, heard through the medium; the rest (bone, stone, a painted wall, a tablet, a scroll,
// a codex, a plate, glass, printed pages) have none: the ship reads them, or plays their notation, as its computer rebuilds them
inline bool recordMediumSounds(int medium) { return medium == RM_CYLINDER || medium == RM_REEL || medium == RM_DISC || medium == RM_DRIVE || medium == RM_CRYSTAL || medium == RM_HUM; }

// each record's home: the place of its people's plan it was kept at, by the history (`historyOfWorld`): of the places standing in its year, one
// drawn by its size (the capital and the far future's structures ten, a city four, a town one and a half, a village one); the last recording
// the last place's. The home always holds it (its archive or a room); copies lie anywhere else. Cached (the last eight); -1 none (no history)
struct RecordHomes { std::vector<int> rank; };
std::shared_ptr<const RecordHomes> recordHomesOf(const BodyGen& g, int people = 0);

// A record lying somewhere: in its building's room or at its foot (`buildingRoom`, peoples/buildings.h), named by its place
// (`buildingPlaceName`: "A HOUSE", "A TEMPLE", "THE FOOT OF A STANDING STONE")
struct ShardSite {
    int index = 0;          // the world's index (`shardWorldIndex`)
    int building = -1;      // the settlement's building it lies in (or at)
    int place = 0;          // the building's kind (BuildingKind)
    bool outside = false;   // at the building's foot, in the open (a stela's, a standing stone's, a spike's), not in a room
    double x = 0, z = 0;    // metres east and north of the settlement's centre: where it lies (a painted wall's: where one stands before it)
    double heading = 0;     // the object's orientation (radians from north); a painted wall's: the way its face looks (into the room)
    int medium = RM_STONE;  // the object (its record's)
    int archive = 0;        // 0 alone; 1 one of an archive's row
    bool last = false;      // the people's last recording
    double wallD = 0;       // a painted wall: how far before (x, z) its face is, along the heading's back (0: no wall, a painted slab on the ground)
    double wallTop = 0;     // a painted wall: the wall's height over the floor there
    double wallTilt = 0;    // a painted wall: its lean toward the room (radians: a hut's walls lean; the painting with them)
};
// C-11: an instrument lying beside a music record (peoples/instruments.h: its people's instrument of the record's age that `pick` names), in the
// record's room, clear of the pieces and the records
struct InstrumentSite {
    int record = 0;         // the world index of the music record it lies beside
    int people = 0;
    int age = 0;            // the record's year's age (the instrument's)
    double pick = 0;        // `instrumentIdOf`'s
    int building = -1;
    double x = 0, z = 0;    // metres east and north of the settlement's centre
    double heading = 0;
};
// the records a settlement, or a work, holds (`r.work` >= 0: the works' rules): deterministic from its seed, its people's records and history;
// C-11: and the instruments lying beside its music records (a settlement's only; a stream of their own: the records are where they were).
// never in water: the sea read here (the planet function without its rivers, which a frame cannot wait for); `wet` (if given) the
// water as the view draws it at a spot of the settlement's metres (its rivers and lakes too: the view asks again once it draws them near)
void recordSitesOf(const BodyGen& g, const RuinSpec& r, const Culture& c, std::vector<ShardSite>& out, std::vector<InstrumentSite>* instruments = nullptr,
                   const std::function<bool(double, double)>* wet = nullptr);
// a record's object as forms, in the settlement's metres over the floor at its site (no colliders: it is picked up): the medium's shapes in
// the records' banks (`FORM_PART_*` parts: bone and paper pale, wax and leather dark, ochre, clay, metal) or its people's (a stone's, a light's)
void recordForms(const ShardSite& s, uint64_t seed, std::vector<Form>& out);

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do
// the year a record was made at the share `u` (0 .. 1) of the people's records ordered by their years: B-414, the years of the age the people
// reached (`ageFromOf` to the end), weighed by what the years left of them
int recordYear(const PeopleAge& pa, double u);
