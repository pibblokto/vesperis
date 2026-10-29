#pragma once
// The steady clock in milliseconds: what the timings and the profiling print, never what is drawn or drawn from
#include <chrono>

inline double nowMs() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
