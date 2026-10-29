#pragma once
// Auto-explore's (R-419) shared pieces, for its four files (autoexplore.cpp, autoexplore_space.cpp, autoexplore_ground.cpp,
// autoexplore_hands.cpp): the looks it tells apart, the outings, the profiles' tastes, where the cabin's window is
#include "game.h"

enum AutoLook { LF_NONE = 0, LF_CABIN, LF_SHIP, LF_GROUND, LF_VEHICLE, LF_PROBE };
enum AutoOuting { AO_VISTA = 0, AO_WATER, AO_LIFE, AO_SIGHT, AO_DRIVE, AO_FLY, AO_RUINS, AO_SKY, AO_COUNT };

// a profile's taste (the wanderer plays with another's at each star)
struct AutoTaste {
    int bodies0, bodies1;              // the bodies it visits in a system before the next star
    double longJump;                   // a move's chance of being a long jump (60 to 3,000 ly) rather than one to a neighbour
    double land, probe;                // its chance to land at a landable world, to drop a probe into a giant
    double tele, roof, glass, orbit;   // at a parking: the telescope, the roof deck, the glass hull, a lap round the world
    double stay0, stay1;               // seconds of looking at a parked world
    double ground0, ground1;           // seconds on the ground
    double w[AO_COUNT];                // the ground's outings: a view, the water, the life, a sight, a drive, a flight, the ruins, the sky
};
const AutoTaste TASTES[AP_COUNT] = {
    {1, 3, 0.15, 0.5, 0.4, 0.3, 0.3, 0.25, 0.4, 10, 20, 120, 240, {1.5, 1, 1.2, 1.2, 0.8, 0.8, 1.5, 1}},       // WANDERER (its whim's in play)
    {0, 2, 0.35, 0.05, 0.1, 0.6, 0.5, 0.45, 0.3, 14, 28, 60, 100, {2, 0.5, 0.5, 0.5, 0, 0.3, 0, 3}},           // STARGAZER
    {3, 6, 0.08, 0.3, 0.15, 0.4, 0.3, 0.25, 0.7, 14, 26, 60, 110, {2, 1, 1, 1, 0.3, 0.6, 0.3, 1}},             // PLANETFALL
    {1, 2, 0.05, 0.95, 0.0, 0.15, 0.1, 0.1, 0.3, 6, 12, 260, 480, {2, 1.5, 2, 2, 1.5, 1.5, 0.6, 0.8}},        // SURVEYOR
    {1, 2, 0.04, 0.2, 0.0, 0.2, 0.1, 0.1, 0.25, 6, 12, 300, 600, {0.4, 0.2, 0.3, 0.7, 0.2, 0.3, 6, 0.2}},     // ARCHAEOLOGIST
    {1, 3, 0.1, 0.25, 0.85, 0.45, 0.35, 0.3, 0.5, 12, 24, 90, 180, {1, 0.3, 0.5, 0.5, 0.3, 0.5, 0.2, 4}},     // GIANTS
};
const double CAB_WINDOW_X = 0.62, CAB_WINDOW_Z = 1.72;   // where the explorer stands at the front window: between its bars (at 0 and 1.25 m), the desk at 1.95
