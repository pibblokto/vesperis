#pragma once
// The probe's (X-01..X-04) shared pieces, for its three files (probe.cpp, probe_feed.cpp, probe_records.cpp): the units
// and places it reads out, the clock at a pressure, a layer's colour, Shift's pace
#include "game.h"
#include "ui.h"

inline double atmOf(double bar) { return bar * 0.98692; }
// the local frame at a radial direction: east (along the turn), north (toward the spin axis's pole)
inline void localFrame(const Vec3& up, const Vec3& axis, Vec3& E, Vec3& N) {
    Vec3 e = cross(axis, up);
    if (length(e) < 1e-6) e = cross(Vec3(0, 1, 0), up);
    if (length(e) < 1e-6) e = cross(Vec3(1, 0, 0), up);
    E = normalize(e); N = cross(up, E);
}
inline std::string latLonString(double lat, double lon) {
    return fmt("%.1f%c %s %.1f%c %s", std::fabs(lat / DEG), CH_DEGREE, lat >= 0 ? "N" : "S", std::fabs(lon / DEG), CH_DEGREE, lon >= 0 ? "E" : "W");
}
// the clock a pressure is reached at (the descent's, by halves)
inline double clockAtBar(const ProbeFlight& f, double bar) {
    double lo = probeDescentStart(), hi = 1e5;
    for (int i = 0; i < 70; i++) { double m = 0.5 * (lo + hi); if (probeBarAt(f, m) < bar) lo = m; else hi = m; }
    return 0.5 * (lo + hi);
}
// X-04: a layer's colour on a recording's depth bar and on the profile's strip (a deck its cloud's)
inline uint32_t layerColour(const GiantAtmosphere& a, int L) {
    if (L >= GIANT_HOLE_LAYER) return rgb(90, 200, 220);
    if (L == 0) return rgb(40, 50, 100);
    if (L == 1) return rgb(170, 130, 70);
    const int k = (L - 2) / 2;
    if (L & 1) return k >= a.decks - 1 ? rgb(120, 40, 28) : rgb(55, 66, 84);
    const RGB c = CLOUD_SPECIES[a.species[std::min(k, GIANT_MAX_DECKS - 1)]].color;
    return rgb((int)(c.r * 220), (int)(c.g * 220), (int)(c.b * 220));
}
const double PROBE_FAST = 8;            // R-406: Shift's pace
