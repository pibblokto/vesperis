// The GUIDE (M3): the explorer's memory. Names given to stars and bodies, the expedition
// log, discovery statistics, the home star and the travel history, and names received
// from other explorers ("inbox"). One text file, `guide.txt`.
#pragma once
#include <string>
#include <vector>
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
    std::vector<double> cam;         // the explorer's turns of the camera where they changed: clock, yaw, pitch
    std::vector<double> time;        // the game's time along the fall where its pace changed: clock, t
    std::vector<ProbeSample> profile;
    std::vector<double> layers;      // the layers it found from the top down: layer, from (bar), to (bar)
};

// R-408: a mark the explorer placed on a world (the sector map's cursor, or `L` at what the crosshair rests on), named or not; the
// maps, the HUD and the scanner (which leaves the marked sights out) read them. `mark` lines; a friend's travel lent (C-14)
constexpr int MARK_PLACE = 8;   // a mark's kind when it marks a place, not one of the sights' (galaxy/landmarks.h's LM_COUNT)
struct SurfaceMark {
    std::string body;            // the world's key
    double lat = 0, lon = 0;     // radians, the body's frame
    int kind = MARK_PLACE;       // what it marks (a LandmarkKind: its glyph on the maps) or MARK_PLACE
    std::string name;            // the explorer's ("" unnamed)
    bool lent = false;           // a friend's, from the inbox
    long long when = 0;          // placed (the wall clock)
};

struct Guide {
    std::map<std::string, std::string> names;     // key -> name (yours)
    std::map<std::string, std::string> inbox;     // key -> name (from imported guides)
    std::vector<LogEntry> log;
    std::set<std::string> visited;                // star keys
    std::set<std::string> landed;                 // body keys
    std::vector<SurfaceMark> marks;               // R-408: the explorer's marks on the worlds (and the friends' lent ones)
    std::set<std::string> shards;                 // C-03: "<body key>/S<index>" of every shard taken from a ruin (`shard` lines)
    int shardWorlds() const;                      // C-03: how many worlds they come from
    std::map<std::string, int> decoded;           // C-06: a shard read on the ship's decoder -> how many of its world's shards the language had then (`decoded` lines)
    std::map<std::string, int> signals;           // C-07: the signals the radar locked: the source's key (a world's or a star's) -> its kind (`signal` lines)
    std::set<std::string> heard;                  // C-07: "<body key>/S<index>" of every recording heard on the radar (`heard` lines): taken from the ruins, it is the one the radar caught
    std::set<std::string> graves;                 // C-12: "<body key>/G<id>" of every grave read in the ruins (`grave` lines), like the landmarks
    std::set<std::string> ended;                  // C-12: the worlds whose last recording the decoder read (`ended` lines): their timeline closed
    std::set<std::string> lent;                   // C-14: "<body key>/S<index>" of the shards a friend's inbox file lent (the ones they found and read; `lent` lines): read on the decoder in cyan, never taken, the ruins still hold them
    bool isLent(const std::string& key) const { return lent.count(key) && !shards.count(key); }   // C-14: a friend's, not yet the explorer's own
    int lentCount() const;                        // C-14: how many are lent and not taken since
    std::set<int> classesSeen, typesSeen;
    std::vector<std::string> history;             // previous stars, oldest first (max 20)
    std::string home;                             // star key
    double furthestFromHomeLY = 0, longestWalkM = 0, highestPointM = 0, totalWalkedM = 0, totalDrivenM = 0;
    double totalFlownM = 0;   // R-403: by drone
    int screenshots = 0;
    std::vector<ProbeRecord> probes;              // X-04: the probes' records, the friends' lent ones among them
    int probesSent = 0;                           // X-04: launched (`stat probes`)
    double deepestProbeKm() const;                // X-04: the deepest of the explorer's own
    int probeIndexBySeed(uint64_t seed) const;    // -1 none
    int creaturesSeen = 0;   // N3-01: first sightings of species (per landing)
    double topSpeedKmh = 0, longestDriveM = 0;   // N4-05: the buggy's records
    int genVersion = 0;      // generation version the guide was last saved with (M9-17)

    static std::string starKey(int64_t sx, int64_t sy, int64_t sz);
    static std::string bodyKey(int64_t sx, int64_t sy, int64_t sz, int body);
    static bool parseStarKey(const std::string& key, int64_t& sx, int64_t& sy, int64_t& sz);
    bool load(const std::string& path);
    bool save(const std::string& path) const;
    // merge names from another guide file into the inbox (your own names win); C-14: and lend the shards that guide found and read
    // (its `shard` keys with a `decoded` line, not its own lent ones), `lentOut` how many are new; -1 when the file cannot be read
    // X-04: and the probes' recordings it holds (its own, not those lent to it), `probesOut` how many are new; R-408: and its marks
    // (its own, not those lent to it, nor one standing where a mark of the explorer's or an earlier loan does), `marksOut`
    int importInbox(const std::string& path, int* lentOut = nullptr, int* probesOut = nullptr, int* marksOut = nullptr);
    void addLog(double t, const std::string& kind, const std::string& text);
    void pushHistory(const std::string& key);
};
