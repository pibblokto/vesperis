// W-03: weather that arrives. A front is a line on a world that moves: a point, a heading and a speed, hashed from the body
// and the time (a new stream: nothing older moves), so the same front is the band on the globe from orbit, the band on the
// landing map, the deck's edge and the wall of rain on the ground, the almanac's `A FRONT FROM THE W` and the data
// sheet's forecast: one function of (body, place, time). A world's fronts come in slots, each a sequence of fronts a few
// days apart; a front is born on a mid-latitude, runs along a great circle (east in the mid-latitudes, west in the
// tropics) at 8-25 m/s for a day or two and dies. Its band, by the distance ahead of its line: cloud from 420 km ahead,
// the wind rising from 350, the rain (or the dust) from the line itself to 150 km behind, the cold behind it.
#pragma once
#include "system.h"
#include <vector>
#include <cstdint>
#include <algorithm>

struct Front {
    int slot = 0; int64_t epoch = 0;
    double born = 0, dies = 0;      // game seconds
    Vec3 c0, n0, a;                 // body frame: the centre at birth, the heading there (unit, tangent), the axis of the motion (c0 x n0)
    double speed = 15;              // m/s along the great circle
    double R = 6000e3;              // the world's radius, metres
    double halfLen = 2000e3;        // metres, half the line's length (the band tapers over its outer fifth)
    double width = 1;               // the band's scale (1: the profile's distances as written)
    double strength = 1;            // 0.45..1: the peak rain or dust, the wind's rise
    double tempDrop = 6;            // C behind the line
    double bow = 0;                 // metres the line's ends trail its centre by (negative: lead), a parabola along it
    uint64_t seed = 0;              // the world's (the band's texture)
    bool dust = false;              // a dust front (desert and thin-atmosphere worlds) rather than rain
};
// the band's texture at a body-frame point: a cloud noise of the world thresholded to 0..1 (holes, streaks and patches), so a band
// reads as cloud from orbit and the ground's cover under its outer parts varies the same way
double frontTexture(const Front& f, const Vec3& p);

// the profile of the band, by the distance ahead of the line in metres over the front's width (positive where the front has
// not come yet): the cloud (0..1), the rain or dust (0..1 before the strength), the wind's rise (0..1), the cold (0..1)
double frontCloudProfile(double x);   // the deck: 1 from 180 km ahead of the line to 200 km behind, ramps of 100-130 km either side
double frontCloudCore(double x);      // the deck's solid core (a wall from 90 km ahead of the line to 130 km behind, its edge ten kilometres): the texture thins the deck outside it only
double frontVeilProfile(double x);    // the thin high veil ahead of the deck (from 520 km ahead), gone behind it
double frontRainProfile(double x);
double frontWindProfile(double x);
double frontColdProfile(double x);
inline double frontCloudAt(double x, double tex) { double c = frontCloudProfile(x); return c > 0 ? c * std::max(tex, frontCloudCore(x)) : 0.0; }   // the cloud of the band at x with the texture there
constexpr double FRONT_RAIN_END = -150e3;    // the rain ends this far behind the line (times the width)
constexpr double FRONT_REACH_AHEAD = 520e3;  // the band's first veil, ahead of the line
constexpr double FRONT_REACH_BEHIND = -900e3;   // the cold has eased by here

bool worldHasFronts(const Body& b);
int frontSlots(const Body& b);
// the fronts of a world alive at some time within [t, t + ahead] (ahead 0: alive now)
void frontsOf(const Body& b, double t, std::vector<Front>& out, double ahead = 0);
// the front's centre and heading at t (body frame)
void frontAt(const Front& f, double t, Vec3& c, Vec3& n);
// 0..1: the front's life at t (0 before its birth and after its death, a ramp over the first 15% and the last 20%)
double frontLife(const Front& f, double t);
// the signed distance ahead of the front's line (metres, positive where it has not come yet) and the distance along the line
// from its centre, of a body-frame unit vector at t; 1e18 on the far side of the world
void frontDistance(const Front& f, double t, const Vec3& p, double R, double& ahead, double& along);
// 1 inside the line's length, tapering to 0 over its outer fifth
double frontLateral(const Front& f, double along);

// the weather of a world's fronts at a body-frame point and time (the maxima over the fronts; the wind from the one that weighs most)
struct FrontWeather {
    double cloud = 0, precip = 0, windRise = 0, cold = 0;
    double windWeight = 0; Vec3 windTo;   // body frame, tangent: where the front's wind blows
    int nearest = -1;                     // the index in the list of the front whose line is nearest, within its length
    double nearestAhead = 1e18, nearestAlong = 0, nearestGain = 0, nearestTex = 1;   // its distances, its life x lateral gain there and the band's texture there
};
void frontWeatherAt(const std::vector<Front>& fronts, const Vec3& p, double t, double R, FrontWeather& out);

// the next fronts at a place: when the line reaches it (the rain begins), when the rain ends, the bearing it comes from
struct FrontForecast {
    double tArrive = 0, tClear = 0;
    double fromBearing = 0;   // radians from north, clockwise: the direction the front comes from
    double strength = 0, gain = 0;   // the front's strength, and its life x lateral at the arrival
    bool dust = false;
    int slot = 0; int64_t epoch = 0;
};
// the fronts that reach `p` (body frame, unit) between t - 8 h and t + window, by their arrival (the first may be over the
// place already: tArrive < t <= tClear)
void frontsAhead(const Body& b, const Vec3& p, double t, double window, std::vector<FrontForecast>& out);

// the overlay of the globe and the landing map: W x H texels on the planet map's grid of the fronts' cloud at t (0..255)
void buildFrontMap(const Body& b, double t, int W, int H, std::vector<uint8_t>& out);
double frontMapAt(const std::vector<uint8_t>& m, int W, int H, double lon, double lat);   // bilinear, 0..1

const char* frontCompass(double bearing);   // "N", "NE", ... "NW"
