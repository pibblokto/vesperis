// H-06 (2026-10-10): what a people's read records told of its history (`docs/archive/ideas/ITERATION-1.3.0.md`, H-06): the decoder's history page
// and its timeline of the eras hold this and nothing else, so a people's history is the explorer's own legends of it, assembled as its
// records are read.
//
// * The events: a text record told some of the history's events (`ShardText::told`: what its genre's directives picked and wrote: a
//   chronicle's years, a letter's news, an epitaph's death and its cause, a ballad's battle). The page holds the events the read records
//   told, in the history's order, each with the records that told it (one event told from several sides is one line).
// * The persons: a person is named only once a read record wrote their name; until then by what they were ("a woman", "a child", "a
//   ruler"), so a letter's "your sister had a girl" is a child born to a woman and a man.
// * The eras: an era is placed and named once a record written in it is read (the ship dates the records; the era is the people's name for
//   those years). An era no read record is from is a stretch of years without a name.
// * The age: the age the people had reached when it ended is named once `AGE_TOLD_RECORDS` of the records of that age are read, or the last
//   recording (A-01's rule: the age is shown in what they built, never named until the records tell it).
// * The people's ways: a text written for a people of many gods, for one that buried its dead in barrows, for the drying that ended it (its
//   drawn alternatives' guards: `ShardText::ways`) tells that much of them: the page's portrait of the people grows with them.
// * The end: the last recording read closes the history.
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
