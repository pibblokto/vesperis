// The galaxy chart, the star map's second page (game/galaxy_chart.cpp): its view and cursor, N's stops, what lies under the
// cursor
#pragma once
#include "core/types.h"
#include "galaxy/starfield.h"
#include <cstdint>
#include <string>
#include <vector>

struct GalaxyChart {
    int level = 0;                      // 0: the whole disc in the face's height; each level halves it
    bool follow = true;                 // the view centred on the ship (C or Space); a pan lets it go
    double cx = 0, cz = 0;              // the view's centre while it does not follow (sector units)
    std::vector<uint32_t> face, edge;   // the view's images as built last: the disc face-on and edge-on, at the logical resolution
    int builtLevel = -1; long long builtI = 0, builtJ = 0; double builtHalf = 0;
    double buildMs = 0;                 // the last build's time (the harness prints it)
    int youX = -9999, youY = -9999;     // where the ship's crosshair was drawn last frame (logical pixels)
    // G-06: the cursor (the arrows, Shift faster, the mouse; the view moves when it nears the face's edge; PgUp/PgDn its height over
    // the plane, on the strip). On the ship it moves with it; let go, it lies in the plane unless its height is what moved
    bool onShip = true;
    double curX = 0, curY = 0, curZ = 0;   // the cursor's point while it is not on the ship (sector units)
    bool plotHere = false;              // a jump plotted at the cursor's point (Enter); a move of the cursor lets it go
    int stop = -1;                      // N's last stop
    bool look = false;                  // the neighbourhood page shows the plotted end (the look before the leap), not the ship's
    StarNeighborhood lookNb;            // the stars round the plotted end
    Vec3 lookCentre;                    // the plotted star (km)
    int delMark = -1; double delUntil = 0;   // Delete twice removes the mark under the cursor
    int curPX = -9999, curPY = -9999;   // where the cursor was drawn last frame (logical pixels; -9999 on the ship)
};

struct ChartStop { Vec3 p; std::string label; uint32_t col = 0; };

struct ChartThere { std::string label; uint32_t col = 0; int kind = 0, mark = -1; std::string key; };   // kind: 0 nothing, 1 a mark, 2 a signal, 3 home, 4 a named star, 5 a star visited
