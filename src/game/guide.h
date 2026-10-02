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

struct Guide {
    std::map<std::string, std::string> names;     // key -> name (yours)
    std::map<std::string, std::string> inbox;     // key -> name (from imported guides)
    std::vector<LogEntry> log;
    std::set<std::string> visited;                // star keys
    std::set<std::string> landed;                 // body keys
    std::set<std::string> landmarksSeen;          // O6-06: "<body key>/L<id>" of every sight logged (its name lives in `names` when renamed)
    std::set<std::string> shards;                 // C-03: "<body key>/S<index>" of every shard taken from a ruin (`shard` lines)
    int shardWorlds() const;                      // C-03: how many worlds they come from
    std::map<std::string, int> decoded;           // C-06: a shard read on the ship's decoder -> how many of its world's shards the language had then (`decoded` lines)
    std::map<std::string, int> signals;           // C-07: the signals the radar locked: the source's key (a world's or a star's) -> its kind (`signal` lines)
    std::set<std::string> heard;                  // C-07: "<body key>/S<index>" of every recording heard on the radar (`heard` lines): taken from the ruins, it is the one the radar caught
    std::set<int> classesSeen, typesSeen;
    std::vector<std::string> history;             // previous stars, oldest first (max 20)
    std::string home;                             // star key
    double furthestFromHomeLY = 0, longestWalkM = 0, highestPointM = 0, totalWalkedM = 0, totalDrivenM = 0;
    double totalFlownM = 0;   // R-403: by drone
    int screenshots = 0;
    int creaturesSeen = 0;   // N3-01: first sightings of species (per landing)
    double topSpeedKmh = 0, longestDriveM = 0;   // N4-05: the buggy's records
    int genVersion = 0;      // generation version the guide was last saved with (M9-17)

    static std::string starKey(int64_t sx, int64_t sy, int64_t sz);
    static std::string bodyKey(int64_t sx, int64_t sy, int64_t sz, int body);
    static bool parseStarKey(const std::string& key, int64_t& sx, int64_t& sy, int64_t& sz);
    bool load(const std::string& path);
    bool save(const std::string& path) const;
    // merge names from another guide file into the inbox (your own names win)
    int importInbox(const std::string& path);
    void addLog(double t, const std::string& kind, const std::string& text);
    void pushHistory(const std::string& key);
};
