#pragma once
// E-08: what space_view.cpp's parts share, for them alone (space_view, space_sun, space_globe, space_bodies): the includes
// they read and the band's shades (`BAND_SHADES`: the backdrop's, three times them a sunless globe's)
#include "space_view.h"
#include "peoples/plan.h"
#include "peoples/roads.h"
#include "galaxy/probe.h"
#include "galaxy/fronts.h"
#include "galaxy/nights.h"   // the earthshine
#include "galaxy/drifters.h"   // a drifter's aurorae by its power
#include "core/noise.h"
#include "core/rng.h"
#include "core/parallel.h"
#include "galaxy/drainage.h"
#include <cmath>
#include <atomic>
#include <cstdio>
#include <algorithm>
#include <cstring>

static constexpr double BAND_SHADES = 24.0;   // the galactic band at full brightness, in shades of bank 0 (the ramp's dim blue-grey stop is at 20): the bulge 21, the plane 8-10
