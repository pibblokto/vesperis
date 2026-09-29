// Small text and HUD helpers shared by the game files.
#pragma once
#include "core/font.h"
#include "core/input.h"
#include "galaxy/system.h"
#include <string>
#include <vector>

std::string fmt(const char* f, ...);
std::string upper(std::string s);
std::string trunc(const std::string& s, size_t n);
std::vector<std::string> wrapText(const std::string& s, size_t width);
const char* shortType(int type);
const char* compassName(double azRad);
// O2 (R-301): surface sectors are the 1 x 1 degree cells of the latitude/longitude grid, named LON:LAT with the
// longitude index 0..359 from 180 W and the latitude index 0..179 from 90 S; the landing map, the surface HUD and
// the sector map all use these names
inline void sectorIndexOf(double latDeg, double lonDeg, int& sx, int& sy) {
    sx = ((int)std::floor(lonDeg + 180.0) % 360 + 360) % 360;
    sy = std::max(0, std::min(179, (int)std::floor(latDeg + 90.0)));
}
inline std::string sectorName(double latDeg, double lonDeg) { int sx, sy; sectorIndexOf(latDeg, lonDeg, sx, sy); return fmt("%03d:%03d", sx, sy); }
// metres as "420 M" or "8.4 KM"
inline std::string metresString(double m) { return m < 1000 ? fmt("%.0f M", m) : fmt("%.1f KM", m / 1000.0); }

// HUD colours; `setHudScheme` (M6-01) picks the classic green, amber or a cool white scheme
extern uint32_t HUD_GREEN, HUD_DIM, HUD_AMBER, HUD_WHITE, HUD_RED, HUD_SHADOW, HUD_CYAN;
void setHudScheme(int scheme);

// Key twins (B-002): every F-key function has a letter or control-key alternative.
inline bool helpKey(const Input& in) { return in.wasPressed(KEY_F1) || (in.wasPressed(KEY_H) && !in.ctrl()) || (in.wasPressed(KEY_SLASH) && in.shift()); }
inline bool dataKey(const Input& in) { return in.wasPressed(KEY_F2) || (in.wasPressed(KEY_I) && !in.ctrl()); }
inline bool saveKey(const Input& in) { return in.wasPressed(KEY_F5) || (in.ctrl() && in.wasPressed(KEY_S)); }
inline bool loadKey(const Input& in) { return in.wasPressed(KEY_F9) || (in.ctrl() && in.wasPressed(KEY_L)); }
inline bool enterKey(const Input& in) { return in.wasPressed(KEY_ENTER) && !in.alt(); }   // Alt+Enter is fullscreen
