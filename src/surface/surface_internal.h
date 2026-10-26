#pragma once
// E-08: what surface_view.cpp's parts share, for them alone (surface_view, surface_env, surface_places, surface_walk,
// surface_palette, surface_terrain, surface_sky, surface_effects): the includes they read, the look a world's kind and
// palette give (`SurfaceLook`; `lookFor` in surface_palette.cpp)
#include "surface_view.h"
#include "peoples/plan.h"
#include "galaxy/drainage.h"
#include "galaxy/planet_features.h"
#include <cstring>
#include <cstdlib>
#include "core/noise.h"
#include "core/parallel.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <atomic>

struct SurfaceLook {
    RGB ground, secondary, tertiary, sand, snow, grass, flora2, zenith, horizon, nightZenith, nightHorizon;
    double skyDensity;     // brightness of the daytime sky
    double fogDistance;    // metres
    bool cloudsLayer;
};
// the ground's colours from the body's palette, the sky's, the fog's and the night's from the kind's row (surface_palette.cpp)
SurfaceLook lookFor(const BodyGen& g, const Star& star);

