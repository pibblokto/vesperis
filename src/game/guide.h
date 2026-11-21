// The GUIDE (M3): the explorer's memory. Names given to stars and bodies, the expedition
// log, discovery statistics, the home star and the travel history, and names received
// from other explorers ("inbox"). One text file, `guide.txt`.
#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <cstdint>

struct LogEntry {
    double t = 0;            // game time
    std::string kind;        // ARRIVAL, ORBIT, LANDING, LAUNCH, NOTE
    std::string text;
};

// X-04: what a probe sent back. The descent is a function of the probe's numbers (X-01's clock, X-02's fields, X-03's character),
// so its recording is the descent drawn again from them: the ship keeps the launch, the explorer's turns of the camera and the
// game's clock along the fall, the end, and the profile the probe measured (`probe` lines, each followed by its `probecam`,
// `probetime`, `probeprofile` and `probelayers` lines)
struct ProbeSample {   // the profile at a depth: the clock, the pressure (bar), the temperature (K), the wind (m/s, east), the light (of
                       // the sunlight over the clouds at noon: the sun's at the depth and its height, the deep's glow), the layer found
                       // there (galaxy/probe.h's `giantFoundLayer`), the flashes since the sample over it
    double clock = 0, bar = 0, tempK = 0, windE = 0, light = 0; int layer = 0, flashes = 0;
};
struct ProbeRecord {
    int id = 0;                      // the explorer's probe number (PROBE 3; a lent one keeps its sender's)
    bool lent = false;               // a friend's, from the inbox (C-14's lending)
    std::string body;                // the giant's key
    int gen = 0;                     // the generation it fell into
    long long saved = 0;             // the wall clock it came back at (a lent one: its lending), the gallery's order
    double launchT = 0, aimLat = 0, aimLon = 0;
    uint64_t seed = 0;
    double startX = 0, startY = 0, startZ = 0;   // the ship's place from the giant's centre at the launch (km)
    double headYaw = 0, descYaw = 0;
    double chuteOpen = -1, chuteClose = -1, crushBar = 25;
    double endClock = 0, endBar = 0, endKm = 0, endK = 0; int endCause = 0;   // 1 the hull gave way, 2 the heat, 3 the relay was cut
    std::string storm;               // the key of the storm it fell into ("" none)
    // the tracks and the profile never change once the record is made (`probeKeep`, the inbox's copy, the load): F-02 writes their
    // lines from `lines`, so a path that changed them in place would save the old ones
    std::vector<double> cam;         // the explorer's turns of the camera where they changed: clock, yaw, pitch
    std::vector<double> time;        // the game's time along the fall where its pace changed: clock, t
    std::vector<ProbeSample> profile;
    std::vector<double> layers;      // the layers it found from the top down: layer, from (bar), to (bar)
    // the four lines that follow the `probe` line in the guide (`probecam`, `probetime`, `probeprofile`, `probelayers`), made once
    // (`probeLines`) where the record is made: they were 566 KB of the player's 732 KB guide, formatted again at every logged event
    mutable std::string lines;
};
std::string probeLines(const ProbeRecord& r);   // F-02: the record's four lines as `Guide::save` writes them (precision 12)

// R-408: a mark the explorer placed on a world (the sector map's cursor, or `L` at what the crosshair rests on), named or not; the
// maps, the HUD and the scanner (which leaves the marked sights out) read them. `mark` lines; a friend's travel lent
constexpr int MARK_PLACE = 21;  // a mark's kind when it marks a place, not one of the sights' (galaxy/landmarks.h's LM_COUNT; T-03: 8 is a work's, LM_WORK;
                                // Q-01: 9-19 the new landforms' sights; Q-02: 20 a strewn field). The guide's file keeps R-408's numbers: 8 a place, 9 a work, the
                                // sights after them 10 on (`markKindToFile`/`markKindFromFile`)
constexpr int MARK_WORK = 8;    // LM_WORK
inline int markKindToFile(int kind) { return kind == MARK_PLACE ? 8 : (kind == MARK_WORK ? 9 : (kind > MARK_WORK ? kind + 1 : kind)); }
inline int markKindFromFile(int k) { return k == 8 ? MARK_PLACE : (k == 9 ? MARK_WORK : (k > 9 ? std::min(k - 1, MARK_PLACE) : k)); }
struct SurfaceMark {
    std::string body;            // the world's key
    double lat = 0, lon = 0;     // radians, the body's frame
    int kind = MARK_PLACE;       // what it marks (a LandmarkKind: its glyph on the maps) or MARK_PLACE
    std::string name;            // the explorer's ("" unnamed)
    bool lent = false;           // a friend's, from the inbox
    long long when = 0;          // placed (the wall clock)
};

// G-06: a mark the explorer placed on the galaxy chart (`L` at its cursor: a void, a cluster, a stretch of arm), named or not; the chart
// draws it with its name and `N` steps to it. `gmark` lines; a friend's lent like R-408's
struct GalaxyMark {
    double x = 0, y = 0, z = 0;   // the place (sector units, light years; y over the plane)
    std::string name;             // the explorer's ("" unnamed)
    bool lent = false;            // a friend's, from the inbox
    long long when = 0;           // placed (the wall clock)
};

// Z-04: what was seen of a species under the microscope, keyed `<body key>/M<index>` (its index its world's list's, the biota's saved key): a bit
// for each part of its sheet seen (its form and size on any slide, its motion on a living one, its flagella in a dark field, its DNA in the
// glow, its wall under the electron; Z-05 its growth, Z-06 its kin), and the forms of its life cycle seen (`CycleForm`'s bits). `microbe` lines,
// after the `microbes 1` line that marks the keys' scheme. Z-05: the stages of its life seen on a culture's curve (`LifeCycle`'s bits), and its
// growth (the quickest doubling seen in a flask and the medium it grew in), after a sheet's two numbers where there are any. Z-06: its genome
// read on the sequencer (its kin: the tree's). Z-07: the years its spores slept before a flask woke them (the longest seen), after the growth's
enum SheetSeen : uint32_t { SEEN_FORM = 1, SEEN_MOTION = 2, SEEN_FLAGELLA = 4, SEEN_DNA = 8, SEEN_WALL = 16, SEEN_KIN = 32 };
// R-437: what a species answered on the slide (the sheet's TO LIGHT, TO FOOD and TO THE FIELD lines): gathering in the light, fleeing it, no
// answer seen; following the food's scent, gathering round lit cells, none seen; swimming along the field, none seen
enum AnswerSeen : uint32_t { AW_LIGHT = 1, AW_FLEES = 2, AW_LIGHT_NONE = 4, AW_FOOD = 8, AW_FOOD_NONE = 16, AW_LIT = 32, AW_FIELD = 64, AW_FIELD_NONE = 128 };
struct MicrobeSheet {
    uint32_t seen = 0, forms = 0;
    uint32_t stages = 0;
    double doublingS = 0;        // 0: never seen growing
    int cell = 0, light = 0, energy = 0, salt = 0;
    double tempC = 0;
    double sleptYears = 0;       // 0: never woken (or woken with no years: a living world's)
    // R-437: its answers (AnswerSeen) and the stage's warmths it was seen at: moving (the lowest and the highest), still under its range (the
    // highest) and over it (the lowest), its stress (and its act, life's AN_ number; -1 none seen); the guide's `answers` line
    uint32_t answers = 0;
    double movingLo = 1e9, movingHi = -1e9, stillCold = -1e9, stillHot = 1e9, stressC = 1e9;
    int stressAct = -1;
    bool answered() const { return answers || movingHi >= movingLo || stillCold > -1e8 || stillHot < 1e8 || stressAct >= 0; }
};
// Z-05: a strain arisen in a flask, keyed `<body key>/M<index>/S<n>` (its species' key and its number): the strain it came of (0 the wild),
// its move (life/cultures.h's `StrainMove`) and the value moved to; its name the explorer's (`names`). `strain` lines. Y-02: a line the gene
// editor wrote is one of them (its move SM_EDIT, its number its species' next), its edits after its value (life/genes.h's text: "3=ACCG;8=-")
struct StrainRecord { int parent = 0, what = 0; double value = 0; std::string edits; };
// Y-02: what a gene does, as the explorer learned it, keyed `<genesis id, hex>/<its number>` (its genesis's map: every kin's): its kind
// (life/genes.h's `GeneKind`; -1 none yet: NOTHING SEEN) and how (GENE_SEEN_*). `gene` lines
enum GeneHow { GENE_SEEN_EDIT = 1, GENE_SEEN_DEATH, GENE_SEEN_STRAIN, GENE_SEEN_NOTHING };
struct GeneKnown { int kind = -1, how = 0; };
// J-02 (R-448): a culture let go on a world (J-03's drone trip), kept by its world: the place, the line (its key, and its edits copied, a
// strain's number being the guide's own), the flask's medium it grew in, the generation and the guide's clock when it went down. `seeded`
// lines in the order they went down; a loan carries none (a friend's seeded worlds never reach the borrower's game)
struct SeedRecord {
    std::string body;                                                  // the world's key
    double lat = 0, lon = 0;                                           // the place (radians)
    std::string line, edits;                                           // the line's key; its edits (life/genes.h's text, "" none)
    int cell = 0, light = 0, energy = 0, salt = 0; double tempC = 0;   // the medium (life/cultures.h's `Medium`)
    int gen = 0;                                                       // the generation it went down in
    double clock = 0;                                                  // the guide's clock then
};
std::string seededLine(const SeedRecord& r);                 // its `seeded` line, without the newline
bool parseSeeded(const std::string& rest, SeedRecord& r);    // the line after its key

// J-07: what the explorer saw of an animal species, keyed `<body key>/A<index>` (its index its world's bestiary's, generation 18's): a bit for
// each thing it was seen doing (surface_types.h's `AnimalDoing`), the nearest it came (metres) and the most seen at once. `animal` lines, after
// the `animals 1` line that marks the keys' scheme (both only once there is one); its name the explorer's (`names`), which a loan carries
struct AnimalRecord { uint32_t doing = 0; int nearest = 0, most = 0; };

struct Guide {
    std::map<std::string, std::string> names;     // key -> name (yours)
    std::map<std::string, std::string> inbox;     // key -> name (from imported guides)
    std::vector<LogEntry> log;
    std::set<std::string> visited;                // star keys
    std::set<std::string> landed;                 // body keys
    std::vector<SurfaceMark> marks;               // R-408: the explorer's marks on the worlds (and the friends' lent ones)
    int shownMark = -1;                           // the one of them shown over the view (an index in marks; -1 none, the default; `shownmark`)
    std::vector<GalaxyMark> galaxyMarks;          // the explorer's marks of the galaxy (and the friends' lent ones)
    std::set<std::string> shards;                 // "<body key>/S<index>" of every shard taken from a ruin (`shard` lines)
    int shardWorlds() const;                      // how many worlds they come from
    std::map<std::string, int> decoded;           // a shard read on the ship's decoder -> how many of its world's shards the language had then (`decoded` lines)
    std::map<std::string, int> signals;           // the signals the radar locked: the source's key (a world's or a star's) -> its kind (`signal` lines)
    std::set<std::string> heard;                  // "<body key>/S<index>" of every recording heard on the radar (`heard` lines): taken from the ruins, it is the one the radar caught
    std::set<std::string> graves;                 // "<body key>/G<id>" of every grave read in the ruins (`grave` lines), like the landmarks
    std::set<std::string> ended;                  // the worlds whose last recording the decoder read (`ended` lines): their timeline closed
    std::set<std::string> instruments;            // "<body key>/I<id>" of every instrument taken from a ruin (`instrument` lines; peoples/instruments.h's ids): kept in the cabin's case
    std::map<std::string, MicrobeSheet> microbes; // Z-04: "<body key>/M<index>" of every species seen under the microscope, what of it was seen (`microbe` lines)
    std::map<std::string, StrainRecord> strains;  // Z-05: "<body key>/M<index>/S<n>" of every strain a flask gave (`strain` lines)
    std::set<std::string> treeOff;                // Y-01: the lab's lines ("<body key>/M<index>/S<n>") taken off their world's tree (`treeoff` lines)
    std::map<std::string, GeneKnown> genes;       // Y-02: what each gene of a genesis was seen to do ("<genesis>/<number>": `gene` lines)
    std::set<std::string> watermarks;             // Z-06: "<body key>/M<index>" of every made species sequenced: its watermark the decoder's (`watermark` lines)
    std::map<std::string, AnimalRecord> animals;  // J-07: "<body key>/A<index>" of every animal species seen (`animal` lines)
    // J-02: the worlds seeded (`seeded` lines) and the guide's clock, the seconds the game ran across its saves and expeditions on real time
    // (the warp never hurries it: `clock`), which the spreads read; both written once a world is seeded, so a guide without them is as before
    std::vector<SeedRecord> seeded;
    double clock = 0;
    std::map<std::string, std::string> madeSeen;   // J-03: what each seeded world showed at its last landing (`madeseen` lines, a seeded guide's)
    void addSeeding(const SeedRecord& r);         // kept as its line reads back (the file's precision): a reload gives the same layer
    // Z-07: the samples taken (`stat samples`) and the worlds they came from (`sampled` lines); a friend's sheets of the species they saw, lent by
    // their inbox file (`lentmicrobe` lines, as `microbe` lines): in the catalogue in cyan, in no statistic; the explorer's own sheet wins
    int samplesTaken = 0;
    std::set<std::string> sampled;
    std::map<std::string, MicrobeSheet> lentMicrobes;
    bool isLentMicrobe(const std::string& key) const { return lentMicrobes.count(key) && !microbes.count(key); }
    // "<body key>/S<index>" of the shards a friend's inbox file lent (the ones they found and read; `lent` lines): read on the decoder in
    // cyan, never taken, the ruins still hold them
    std::set<std::string> lent;
    bool isLent(const std::string& key) const { return lent.count(key) && !shards.count(key); }   // a friend's, not yet the explorer's own
    int lentCount() const;                        // how many are lent and not taken since
    std::set<int> classesSeen, typesSeen;
    std::vector<std::string> history;             // previous stars, oldest first (max 20)
    std::string home;                             // star key
    double furthestFromHomeLY = 0, longestWalkM = 0, highestPointM = 0, totalWalkedM = 0, totalDrivenM = 0;
    double totalFlownM = 0;   // by drone
    int screenshots = 0;
    std::vector<ProbeRecord> probes;              // the probes' records, the friends' lent ones among them
    int probesSent = 0;                           // launched (`stat probes`)
    int droneTrips = 0, droneFinds = 0;           // the drone bay's trips and what they brought aboard (`stat drones`, `stat dronefinds`)
    double deepestProbeKm() const;                // the deepest of the explorer's own
    int probeIndexBySeed(uint64_t seed) const;    // -1 none
    int creaturesSeen = 0;   // first sightings of species (per landing)
    double topSpeedKmh = 0, longestDriveM = 0;   // the buggy's records
    int genVersion = 0;      // generation version the guide was last saved with
    // the shards' keys are a people's records at people x 1000 + index (peoples/records.h's `shardWorldIndex`); a guide saved before
    // (no `records 2` line) had C-13's second people at 50 + index, moved here when it is loaded: the shards found are kept by their index (the
    // counts and the language kept, the contents the records' now), `migratedShards` how many of the explorer's own were (the game says so once).
    // a guide saved before `records 3` keeps its keys, its found shards' contents new (a people's records all of the age it reached)
    int migratedShards = 0;

    static std::string starKey(int64_t sx, int64_t sy, int64_t sz);
    static std::string bodyKey(int64_t sx, int64_t sy, int64_t sz, int body);
    static bool parseStarKey(const std::string& key, int64_t& sx, int64_t& sy, int64_t& sz);
    bool load(const std::string& path);
    bool save(const std::string& path) const;
    // merge names from another guide file into the inbox (your own names win); C-14: and lend the shards that guide found and read
    // (its `shard` keys with a `decoded` line, not its own lent ones), `lentOut` how many are new; -1 when the file cannot be read
    // and the probes' recordings it holds (its own, not those lent to it), `probesOut` how many are new; R-408: and its marks
    // (its own, not those lent to it, nor one standing where a mark of the explorer's or an earlier loan does), `marksOut`; G-06: and its
    // marks of the galaxy likewise (one within a light year of a mark already there is the same place), `galaxyOut`; Z-07: and the sheets of the
    // species it saw (its own, not those lent to it, nor one the explorer saw), `microbesOut` how many are new
    int importInbox(const std::string& path, int* lentOut = nullptr, int* probesOut = nullptr, int* marksOut = nullptr, int* galaxyOut = nullptr,
                    int* microbesOut = nullptr);
    void addLog(double t, const std::string& kind, const std::string& text);
    void pushHistory(const std::string& key);
};
