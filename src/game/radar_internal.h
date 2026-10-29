#pragma once
// The radar's (C-07, C-15, C-16) shared pieces, for radar.cpp and radar_camera.cpp: the gain's cones, hold and wear
#include "game.h"

const double LOCK_CONE = 3 * DEG, KEEP_CONE = 8 * DEG;
// C-15: the gain narrows the cones (3 and 8 degrees at x1, 2 and 4 at x32) and lengthens the hold (x2.25 at x32: a faint far
// signal at the top step takes eleven seconds); the far scan reads 40,000 cells a frame (about five frames from home at x32)
const int DEEP_SLICE = 40000;
inline double octaves(double g) { return std::log2(std::max(1.0, g)); }
inline double lockCone(double g) { return LOCK_CONE / (1 + 0.1 * octaves(g)); }
inline double keepCone(double g) { return KEEP_CONE / (1 + 0.2 * octaves(g)); }
inline double holdFactor(double g) { return 1 + 0.25 * octaves(g); }
inline bool deepOf(const Signal& s) { return !s.local && s.needGain > 1; }   // heard only through the gain
// C-16: the dial as 0 (x1) .. 1 (x32); how worn a signal comes (none within 200 ly, half at about 1,000, all at 6,000)
inline double gainShare(double g) { return clampd((g - 1) / (SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1] - 1), 0, 1); }
inline double wearOf(const Signal& s) { return s.local || s.distLy <= SIGNAL_REACH_LY ? 0 : clampd(std::log(s.distLy / SIGNAL_REACH_LY) / std::log(30.0), 0, 1); }
