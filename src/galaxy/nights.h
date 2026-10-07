// W-04: the nights, from the system's geometry.
// Meteor showers: a comet sheds dust along its whole orbit; a world whose orbit about the star passes within the stream's
// width of the comet's crosses the stream once a year at the same place of its orbit: a shower, peaking when the world is
// nearest the comet's orbit, its meteors coming from a radiant fixed among the stars (the stream's velocity relative to
// the world there, reversed). Every number comes from the two orbits but the stream's age (a hash of the comet's own): the
// same shower each year, the almanac's `METEORS PEAK`, the sky's streaks.
// The zodiacal light: the dust of a young system or of a belt near the world's orbit, a cone along the plane of the worlds
// from the sun. The light a body sheds on a place (M10-09's planetshine, the earthshine on a moon's night side).
#pragma once
#include "system.h"
#include <vector>

struct MeteorShower {
    int comet = -1;          // the comet's index in the system
    int world = -1;          // the body whose orbit about the star crosses the stream (a moon's parent planet)
    double moidKm = 0;       // the least distance between the two orbits
    double strength = 0;     // 0..1: the stream's density on the world's path at the peak (1 the densest stream, met at its core)
    double period = 0;       // s: the world's year
    double tPeak0 = 0;       // s: a peak (the world at the point of its orbit nearest the comet's); the others a year apart
    double sigma = 0;        // s: the activity's width (the stream's width over the world's speed)
    Vec3 radiant;            // unit, in the system's axes: the direction the meteors come from
    double speedKms = 0;     // their speed relative to the world (game km/s)
    Vec3 streamKm;           // the point of the comet's orbit nearest the world's, relative to the star
    Vec3 worldKm;            // the point of the world's orbit nearest the comet's, relative to the star
};

constexpr double SHOWER_PEAK_PER_MIN = 60;   // meteors a minute at a full stream's peak with the radiant overhead (the sporadic ones are about seven)
constexpr double SHOWER_MIN_STRENGTH = 0.08; // weaker streams make no shower

// the showers of a body (a planet's, a moon's through its parent's orbit), strongest first; none for a comet, a companion
// star or a body of a system without comets
void meteorShowersOf(const StarSystem& sys, int body, std::vector<MeteorShower>& out);
double showerActivity(const MeteorShower& s, double t);   // 0..strength: the share of the full rate at time t
double showerPeakAfter(const MeteorShower& s, double t);  // the first peak at or after t
// the least distance (km) from a point relative to the star to a body's orbit about the star (sampled, then refined)
double orbitDistanceKm(const Body& b, const Vec3& relKm);
// a body's position on its orbit about its parent at the true anomaly nu (km, relative to the parent)
Vec3 orbitPointAt(const Body& b, double nu);

// the dust of a world's sky for the zodiacal light (0..1): a young star's (a protostar's most, the young blue stars some) and a
// belt's near the world's orbit (a moon: its parent's), varied by a hash of the system; under ZODIACAL_MIN_DUST none is drawn
double zodiacalDust(const StarSystem& sys, int body);
constexpr double ZODIACAL_MIN_DUST = 0.12;
// the zodiacal light (0..~1, of the galactic band's full brightness) in a direction, from the direction to the sun, both unit
// in the system's axes (the worlds orbit near the plane y = 0): a cone along the plane, brightest toward the sun and widening
// away from it, and the faint patch opposite the sun
double zodiacalLight(const Vec3& dirW, const Vec3& sunW);

// M10-09: the light body bi sheds on a place (km) at t, 0..0.5 of the sun's: its lit share seen from there times its albedo
// times its apparent size (5 degrees of radius a full measure), a substellar body's own glow where that is more; `lightFactor`
// the sun's light there. The ground's planetshine at night and (W-04) the earthshine on a moon's night side read it
double bodyShineAt(const StarSystem& sys, int bi, const Vec3& placeKm, double t, double lightFactor);
