#pragma once
// A world's unit vectors (the body frame, z the pole) and the lengths along them: what the plan, the works and the landmarks measure
#include "types.h"
#include <algorithm>
#include <cmath>

// the great-circle distance between two unit vectors, in R's unit (km or metres)
inline double arcLength(const Vec3& a, const Vec3& b, double R) { return std::acos(clampd(dot(a, b), -1, 1)) * R; }
// a point `eastM` and `northM` off a unit vector, on the tangent plane's great circles (the plan's, the roads' and the works' frame)
inline Vec3 offsetUnit(const Vec3& u, double eastM, double northM, double Rm) {
    Vec3 pole(0, 0, 1), east = cross(pole, u);
    if (length(east) < 1e-9) east = Vec3(0, 1, 0);
    east = normalize(east);
    const Vec3 north = cross(u, east);
    const double d = std::sqrt(eastM * eastM + northM * northM);
    if (d < 1e-9) return u;
    const Vec3 dir = (east * eastM + north * northM) / d;
    return normalize(u * std::cos(d / Rm) + dir * std::sin(d / Rm));
}
// a point `eastM` and `northM` off a unit vector across its tangent plane, put back on the sphere (the landmarks' search, a cell wide)
inline Vec3 offsetFlat(const Vec3& u, double eastM, double northM, double Rm) {
    Vec3 pole(0, 0, 1);
    Vec3 e = cross(pole, u); double el = length(e);
    if (el < 1e-9) e = Vec3(1, 0, 0); else e = e / el;
    Vec3 n = cross(u, e);
    return normalize(u + (e * eastM + n * northM) / Rm);
}
inline Vec3 slerpUnit(const Vec3& a, const Vec3& b, double t) {
    const double c = clampd(dot(a, b), -1, 1), w = std::acos(c);
    if (w < 1e-9) return a;
    const double s = std::sin(w);
    return normalize(a * (std::sin((1 - t) * w) / s) + b * (std::sin(t * w) / s));
}
// a unit vector moved `m` metres toward another along their great circle: `towardUpTo` stops at it, `towardThrough` goes on past it
// when `m` is longer than the arc
inline Vec3 towardUpTo(const Vec3& from, const Vec3& to, double m, double Rm) {
    const double w = std::acos(clampd(dot(from, to), -1, 1));
    if (w < 1e-12) return from;
    return slerpUnit(from, to, std::min(1.0, m / Rm / w));
}
inline Vec3 towardThrough(const Vec3& from, const Vec3& to, double m, double Rm) {
    const double w = std::acos(clampd(dot(from, to), -1, 1));
    if (w < 1e-12) return from;
    return slerpUnit(from, to, m / Rm / w);
}
// the bearing (radians from north, clockwise) of `to` seen from `from`
inline double bearingOf(const Vec3& from, const Vec3& to) {
    Vec3 pole(0, 0, 1), east = cross(pole, from);
    if (length(east) < 1e-9) east = Vec3(0, 1, 0);
    east = normalize(east);
    const Vec3 north = cross(from, east), d = to - from;
    return std::atan2(dot(d, east), dot(d, north));
}
