// W-01: the almanac. The next events at a place, every one computed from the system's geometry (the sun's and the
// bodies' positions the sky already draws), so the list is exact: on the ground the sun's rise, transit and set and
// the sky going dark and light, the bodies' rises and sets, their phases, a body over the sun, the world's own ring
// over the sun; from the ship the moons across, behind, shadowing and shadowed by the parked body, the planets meeting
// in the sky, a body across the sun, a comet's plunge. A registry of finders: W-03's fronts and W-04's showers add
// theirs with `addAlmanacFinder`. The game's screen and the run-to-it live in `game/almanac.cpp`.
#pragma once
#include "system.h"
#include <vector>
#include <string>
#include <functional>

enum AlmanacKind {
    AL_SUNRISE = 0, AL_SUNSET,   // the sun's upper limb over the horizon (the centre at minus its angular radius)
    AL_NOON,                     // the sun's transit of the meridian (the hour angle through zero)
    AL_DUSK, AL_DAWN,            // the sky going dark and light: the sun's sine of altitude through -0.12, where the sky model ends and begins its glow (worlds with an atmosphere)
    AL_RISE, AL_SET,             // a body's upper limb over the horizon
    AL_FULL, AL_NEW,             // a body's lit share at its fullest and its thinnest
    AL_ECLIPSE,                  // a body over the sun at the deepest of its crossing (value: the share of the disc covered, as `SurfaceSite::sun` reads it)
    AL_RING_SHADOW, AL_RING_CLEAR,   // the world's own ring between the place and the sun, and clear of it again (value: the ring's density there)
    AL_PERIAPSIS, AL_APOAPSIS,   // a comet closest to its star and farthest from it
    AL_TRANSIT, AL_OCCULTATION,  // from the ship: a moon of the parked body touches its disc in front of it, and behind it (first contact)
    AL_SHADOW, AL_ECLIPSED,      // a moon's shadow reaches the parked body; a moon enters the parked body's shadow
    AL_CONJUNCTION,              // two planets within two degrees of each other in the ship's sky (value: the least separation, radians)
    AL_SUN_TRANSIT,              // a body touches the sun's disc as seen from the ship (first contact)
    AL_FRONT, AL_FRONT_CLEAR,    // W-03: a front's line reaches the place (the rain or the dust begins; value: the bearing it comes from) and its rain ends
    AL_SHOWER,                   // W-04: a meteor shower's peak (body: the comet whose stream it is; value: the meteors a minute at the peak with the radiant overhead)
    AL_FREE,                     // the menu's free row: "run for N" (no finder gives it)
    AL_COUNT
};

struct AlmanacEvent {
    int kind = 0;
    double t = 0;                // game time (seconds)
    int body = -1, body2 = -1;   // the bodies concerned, by their index in the system
    double value = 0;
};

// where the almanac is read: a place on a world (`ground`), or the ship (parked at `body`, or `pos` in deep space)
struct AlmanacPlace {
    const StarSystem* sys = nullptr;
    double t = 0;                  // now
    double window = 5 * 86400.0;   // how far ahead the finders look (the game: five days on the ground, thirty on the ship)
    bool ground = true;
    int body = -1;                 // the world under the feet; from the ship the parked body (-1 none)
    double lat = 0, lon = 0;       // radians, the place on the ground
    bool atmosphere = false;       // the ground's (a twilight exists)
    std::vector<float> ringProf;   // the world's own ring (`StarSystem::ringProfileOf`), empty without rings
    Vec3 pos;                      // the ship's position (km) when not on the ground
    Vec3 parkDir; double parkDist = 0; bool orbiting = false;   // the ship's parking (the orbit's slow lap is followed)
};

typedef void (*AlmanacFinder)(const AlmanacPlace& place, std::vector<AlmanacEvent>& out);
void addAlmanacFinder(AlmanacFinder fn);   // the registry: a finder appends the events it knows of (the first of each kind per body within the window)
// every finder's events after `place.t` and within the window, sorted by time
void almanacOf(const AlmanacPlace& place, std::vector<AlmanacEvent>& out);

// the sun at the place now: its altitude (radians) and angular radius, and the sine of its altitude the sky model reads
double almanacSunAltitude(const AlmanacPlace& place, double t, double* angularRadius = nullptr);
// the label of an event: "SUNRISE", "<NAME> RISES", "<NAME> OVER THE SUN 85%"; `nameOf` gives a body's name as the reader
// knows it (the game: the explorer's or UNKNOWN; the harness: the generator's), `degree` the degree sign's text
std::string almanacLabel(const AlmanacEvent& e, const StarSystem& sys, const std::function<std::string(int)>& nameOf, const char* degree = "\x01");
// "45 S", "12 MIN", "3 H 05 MIN", "4 D 6 H"
std::string countdownString(double secs);
