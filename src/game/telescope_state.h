// The telescope (game/telescope.cpp): what it tracks and aims at, the stabiliser's frame
#pragma once
#include "core/types.h"
#include "galaxy/starfield.h"

enum { TELE_NONE = -1, TELE_SUN = -2, TELE_STAR = -3 };
struct TelescopeState { bool on = false; int step = 1; int track = TELE_NONE; double offYaw = 0, offPitch = 0; int target = TELE_NONE; Star targetStar; double targetRangeKm = 0, targetWidth = 0;
                        // R-405, the stabiliser: the parked body whose frame holds the ship's parking and the view's axis (body-frame unit
                        // vectors; the attitude last set, so a turn from elsewhere is taken up); -1 when not parked at a body
                        int frameBody = -1; Vec3 parkBF, viewBF; double heldYaw = 0, heldPitch = 0; };
