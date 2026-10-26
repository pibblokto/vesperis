// What a people's read records told of its history (H-06; docs/reference/civilisations/records-and-grammar.md): the decoder's history page
// and its timeline hold this and nothing else. The events the read records told (`ShardText::told`), in the history's order, each with the
// records that told it; a person named once a read record wrote the name, else by what they were; an era placed and named once a record of
// it is read; the age named once AGE_TOLD_RECORDS of its records are read, or the last recording; the people's ways as the read records'
// guards tell them (`ShardText::ways`); the last recording closes it.
#pragma once
#include "history.h"
#include "shards.h"
#include <map>
#include <set>
#include <string>
#include <vector>

constexpr int AGE_TOLD_RECORDS = 3;   // the records of the people's last age read that name the age

struct ToldEvent {
    int event = -1;          // the history's
    std::vector<int> by;     // the read records that told it (local indices), by their years
};
struct ToldHistory {
    std::vector<ToldEvent> events;      // in the history's order
    std::set<std::string> names;        // the proper names the read records wrote (`VgRecord::names`)
    std::vector<int> eraRead;           // per era of the history: the records read that were written in it (B-414: or tell it, a late people's history)
    int ageRead[AGE_COUNT] = {};        // the records read by the age of their year
    int read = 0;                       // the records read (every category)
    int texts = 0;                      // of them texts (the ones that tell)
    int age = AGE_MEDIEVAL;             // the age the people reached (its last era's)
    bool ageTold = false;               // that age named
    bool ended = false;                 // the last recording read
    int first = 0, last = 0;            // the years of the earliest and the latest record read (0 none)
    std::map<std::string, std::pair<std::string, int>> ways;   // the people's ways the read records expressed ("faith", "rite", "holy", "economy", "economy2",
                                                               // "value", "value2", "deadway", "cause": `ShardText::ways`): the value, the first record that did
};
// `read`: the local indices of the people's records read (any category: every record dates its era and its age; a text tells events and names)
void toldHistoryOf(const History& H, const ShardTexts& texts, const std::vector<int>& read, int lastIndex, ToldHistory& out);
// an event's line on the history page: `historyEventLine` with the persons the records never named by what they were at the time
std::string legendLine(const History& H, const ToldHistory& T, int event);
// the words of the names a legend's line may hold, lowercase (`decodeLine` keeps them as names): the places', the persons' the records named,
// the people's own titles of their rulers
void legendNameWords(const History& H, const ToldHistory& T, std::set<std::string>& out);
// the people's ways the records told, a line each (its label and its words: "FAITH" "many gods, Khosa among them", "DEAD" "buried in barrows",
// "ENDED" "by the drying"), in a fixed order: their faith, its rite, its holy place, what they lived by, what they valued, their dead, their end
void legendWays(const History& H, const ToldHistory& T, std::vector<std::pair<std::string, std::string>>& out);
extern const char* const AGE_TOLD_NAMES[AGE_COUNT];   // "PREHISTORIC" .. "FAR FUTURE": the age once told (the history page's "THE AGE THEY REACHED")

// ---- the harness's (E-07): vesperis_test calls these; the game and the other engine files never do
// a person's name written by a read record (the name as the history gives it, or the given name a letter calls them by)
bool toldPersonNamed(const History& H, const ToldHistory& T, int person);
