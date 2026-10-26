#pragma once
// E-10: a people's lore, out of shards.h for the headers that hold or pass one and need nothing else of the records (history,
// charts, graves, music): `Lore` (the names its people shared, their reckoning, the stars their charts mark) and the seed a
// people's functions draw from (`loreSeed`); its functions (`loreOf`, `loreQuick`, `personName`) are shards.h's
#include "galaxy/system.h"
#include "galaxy/planetmap.h"
#include <string>
#include <vector>

constexpr int SHARDS_PER_WORLD = 50;   // C-02's catalogue: a people's first fifty records (C-03 below; T-04: 80 to 200 in all)

// C-10 (2026-10-03): a star the people marked on one of their charts (`peoples/charts.h`): a people's world among the bright
// stars of their sky, the star a transmission they heard came from, or the brightest of their nights, the one the lore names
struct ChartMark {
    Star star;              // the star (its name left out: the view resolves the explorer's)
    int body = -1;          // its people's world, -1 none
    std::string people;     // that world's people (empty when the star has none)
    int how = 0;            // 0 the brightest of their sky (`Lore::star` names the first such), 1 a people's world among their bright stars, 2 a transmitter they heard
    double ly = 0;          // from their star
};

// the proper nouns a world's shards share: the people's name, their god, their river, mountain, sea, two towns, the founder,
// the moon and the star they named, the festival; and the world's reckoning (the span of years the shards cover)
struct Lore {
    std::string people, god, river, mountain, sea, city, city2, founder, moon, star, festival;
    int style = 0;          // the names' phonotactic style (`generateName`): 0 plain, 1 hard, 2 flowing
    double yearDays = 0;    // the world's year in its own days
    double dayHours = 0;    // its day in hours
    int moons = 0;
    bool desert = false;    // the catastrophe's worlds (a felisian world went on without its people)
    bool dark = false;      // D-05: a drifter's people, who lost their sun (their tones darken toward the end: `pickTone`)
    int spanYears = 300;    // the years between the founding and the last shard
    // C-12: their calendar (`loreQuick`): the day is the world's turn, the month the nearest moon's round in those days, the year
    // the orbit; `calendar` 0 days and years, 1 moons, days and years, 2 moons and years (the day is the year: a locked world),
    // 3 years alone; `last` the index of the world's last recording (`lastShardOf`)
    int monthDays = 0, months = 0, calendar = 0, last = -1;
    std::vector<ChartMark> marks;   // C-10: the stars their charts mark, one per chart (`chartShardsOf`), the peoples they knew first
    // C-13: which of the world's peoples this lore is (`peoplesOf`, peoples/ruins.h), the seed its draws read (`peopleGen`: the
    // world's own for the first people), and, where the world had two, the other people's name and their fate against this
    // people's end (-1 they ended first, 0 together, 1 they went on), which the grammar's "other" kinds speak of
    int which = 0, peoples = 1;
    uint64_t seed = 0;
    std::string other;
    int otherFate = 0;
    // T-04: how many records the people left (`recordCountOf`, peoples/records.h: 80 to 200, C-02's fifty the first) and the year of its end
    // in its own count (`PeopleAge::lastYear`): the shards are dated in the people's history now, `spanYears` the end's year less one
    int records = SHARDS_PER_WORLD, endYear = 0;
    int ageFrom = 1;   // B-414: the first year of the age they reached (`ageFromOf`): their graves and records are of its years
};
// C-13: the seed a people's functions draw from: the lore's (the people's), the world's for a lore built without one. The
// functions that take a lore (`shardOf`, `tongueOf`, `voiceOf`, `traditionOf`, `chartOf`) read it and never the generator's
// seed, so they take the world's generator and give the lore's people; the seed-only ones (`shardIsMusic`, `chartShardsOf`,
// `lastShardOf`, `peopleNameOf`...) take the people's generator (`peopleGen`) and count a people's fifty from zero
inline uint64_t loreSeed(const BodyGen& g, const Lore& L) { return L.seed ? L.seed : g.seed; }
