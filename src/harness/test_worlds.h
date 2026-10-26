#pragma once
// E-03: the worlds and places the modes and the unit stand on, pinned: the ages' worlds, the scorched
// and the drifting peoples' worlds, the review sites
#include "harness/harness.h"
// A-01: the age test worlds, the nearest people of each age to home on the harness's plane (sector y 0, where every mode's
// `<sx> <sz> <body>` reaches; `second` for a second people), found by `peoples ages` and pinned here; the unit checks each still
// has its age, and the mode says when one moved
struct AgeWorld { int64_t sx, sz; int body, which; int64_t sy = 0; };   // D-05: `sy` a drifter's (off the plane)
static const AgeWorld AGE_WORLDS[AGE_COUNT] = {
    {198, 55, 10, 0},   // prehistoric, 9.5 ly: Navio V-a (felisian), the Witniikh
    {200, 41, 1, 0},    // medieval, 7.1 ly: Faiyiari II (desert), the Ta-Zhozo
    {189, 45, 2, 0},    // industrial, 6.1 ly: Meihimeeri III (felisian), the Aachidblukh
    {208, 51, 5, 0},    // modern, 13.9 ly: Yeesheleemiola II-b (desert), the Jengne (R-413: Rieneseirela VI-a's second people, 11.0 ly, became medieval with its first)
    {210, 33, 0, 0},    // futuristic, 19.8 ly: Iamaielyari I (desert), the Irrur
    {202, 26, 2, 0}};   // far future, 21.2 ly: Seanesaila III (desert), the Glaak
// A-03: the ages' scenes stand on peoples that ended lately (what is left of their works stands: `peoples fresh`), the nearest of each age
// on the harness's plane; the test worlds above where their own people did
static const AgeWorld AGE_SCENE_WORLDS[AGE_COUNT] = {
    {198, 55, 10, 0},   // prehistoric, 9.5 ly: Navio V-a (felisian), ended 215 years ago
    {202, 28, 0, 0},    // medieval, 19.3 ly: Reeverilee I (desert), ended 530 years ago
    {200, 16, 0, 0},    // industrial, 30.4 ly: Washainiosh I (desert), ended 405 years ago
    {208, 51, 5, 0},    // modern, 13.9 ly: Yeesheleemiola II-b (desert), ended 421 years ago
    {210, 33, 0, 0},    // futuristic, 19.8 ly: Iamaielyari I (desert), ended 1,928 years ago
    {202, 26, 2, 0}};   // far future, 21.2 ly: Seanesaila III (desert), ended 1,031 years ago
// P-04: the scorched worlds' test worlds, the nearest scorched people of each age to home on the plane (within 300 ly: they are rare), found by
// `peoples ages` and pinned here (body -1: none within reach)
static const AgeWorld SCORCHED_WORLDS[AGE_COUNT] = {
    {222, 75, 3, 0},     // prehistoric, 39.6 ly: Eyeathshyan II-a (a red giant's), the Riav
    {226, 82, 0, 0},     // medieval, 47.5 ly: Lalyenie I (a white dwarf's), the Wohschol
    {228, -44, 1, 0},    // industrial, 95.9 ly: Nisha II (a white dwarf's), the Maqeki
    {122, 123, 14, 0},   // modern, 106.1 ly: Hiephie II-e (a red giant's), the Mosbowla
    {323, 192, 0, 0},    // futuristic, 194.2 ly: Thearel I (a red giant's), the Supmussusj
    {250, 158, 3, 0}};   // far future, 124.8 ly: Neyielaheash I-a (an orange giant's), the Tanha
// D-05: the test drifters, a people that lost its sun on each (`shards drifters`, the unit), found by `peoples rogues` and pinned here
static const AgeWorld TEST_DRIFTERS[2] = {
    {1728, -575, 0, 0, 740},    // 1,812 ly: Balyaphebist (frozen felisian), the Reimempel: medieval, by the intruder
    {177, -1246, 0, 0, 950}};   // 1,604 ly: Vreikheid (desert), the Ghaiwuwutxu and the Ainyiny: modern with radio, by the wanderer; their beacons
// the pinned felisian mountain site (the O6 review site, the `felisian_mountains` scene, `descent`); O6-03: re-pinned on the GEN 10 bodies
static constexpr double PIN_MOUNTAIN_LAT = -48.030, PIN_MOUNTAIN_LON = 152.547;   // S-01: re-pinned on Wyariothmai I, the day side of a world locked to its red dwarf (the G-04 pin's world, Wyariothmai III, is an ocean world since the star was claimed; of the five lit mountain candidates this one has a steady slope for `descent`)
static constexpr double PIN_PLAIN_LAT = -55.154, PIN_PLAIN_LON = -122.371;   // W-03: the pinned felisian plain (`felisian_plain` of `landforms sites`, S-01's re-pin on Wyariothmai I's day side): the fronts' scenes, frames and checks stand on it
static constexpr double PIN_FRONT_LAT = -26.555, PIN_FRONT_LON = 166.809;   // W-03: the fronts' scenes' site on the same world: flat land by day under a clear pattern sky with a strong front coming (0/0: search and print what to pin)
// E-07: the systems whose worlds a made kind must leave as drawn (rules-galaxy.md, "Protected and pinned"; the unit's pins name the
// worlds): the regress's five (the first systems of its scan), the probe's three, the scenes' first molten world's and first yellow
// star's, home, and B-427's Wolf-Rayet giant (off the plane). `salt scan` reads every world of them against the made kinds' sources
struct ProtectedSystem { int64_t sx, sy, sz; const char* name; };
static const ProtectedSystem PROTECTED_SYSTEMS[] = {
    {150, 0, 23, "Lathyari"}, {150, 0, 26, "Seapheeseeri"}, {150, 0, 36, "Wyariothmai"}, {150, 0, 57, "Iaphiath"}, {150, 0, 58, "Feeyeithis"},
    {150, 0, 66, "Thilawiane"}, {150, 0, 73, "Weesies"}, {150, 0, 106, "Wailie"},
    {151, 0, 53, "Shiesaimiori"}, {150, 0, 97, "Salyiesesh"}, {HOME_SX, HOME_SY, HOME_SZ, "Heilya"},
    {18071, 1, -19950, "Syaniapheela"}};
