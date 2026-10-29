#pragma once
// The synth's shared pieces, for audio.cpp and audio_radar.cpp: the constants, the glottal source, Klatt's resonator
#include "audio.h"
#include <cmath>

constexpr double TAU_ = 6.283185307179586, PI_ = 3.141592653589793;
// C-05: the derivative of the glottal flow over one period (phase 0..1): a rise, a sharp fall at the closure, nothing while closed
inline double glottal(double p) {
    const double Tp = 0.4, Tn = 0.16;
    if (p < Tp) return 0.4 * std::sin(PI_ * p / Tp);
    if (p < Tp + Tn) return -std::sin(PI_ * (p - Tp) / (2 * Tn));
    return 0;
}
inline double resonate(double x, double f, double bw, double dt, double& y1, double& y2) {   // Klatt's resonator, unity at the centre
    double r = std::exp(-PI_ * bw * dt), B = 2 * r * std::cos(TAU_ * f * dt), C = -r * r;
    double o = (1 - r) * x + B * y1 + C * y2; y2 = y1; y1 = o; return o;
}
