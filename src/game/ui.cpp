#include "ui.h"
#include <cstdarg>
#include <cstdio>
#include <sstream>
#include <cmath>

std::string fmt(const char* f, ...) {
    char buf[512];
    va_list ap; va_start(ap, f); vsnprintf(buf, sizeof(buf), f, ap); va_end(ap);
    return buf;
}
std::string upper(std::string s) { for (char& c : s) c = (char)toupper((unsigned char)c); return s; }
std::string trunc(const std::string& s, size_t n) { return s.size() <= n ? s : s.substr(0, n - 1) + "."; }
std::vector<std::string> wrapText(const std::string& s, size_t width) {
    std::vector<std::string> out;
    std::string cur;
    std::istringstream is(s);
    std::string w;
    while (is >> w) {
        if (!cur.empty() && cur.size() + 1 + w.size() > width) { out.push_back(cur); cur.clear(); }
        if (!cur.empty()) cur += " ";
        cur += w;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}
uint32_t HUD_GREEN = rgb(120, 230, 150), HUD_DIM = rgb(60, 120, 80), HUD_AMBER = rgb(240, 190, 90),
         HUD_WHITE = rgb(230, 235, 240), HUD_RED = rgb(240, 90, 70), HUD_SHADOW = rgb(0, 0, 0), HUD_CYAN = rgb(110, 200, 230);

void setHudScheme(int scheme) {
    switch (scheme) {
        case 1:   // amber terminal
            HUD_GREEN = rgb(240, 180, 80); HUD_DIM = rgb(130, 90, 40); HUD_AMBER = rgb(255, 230, 150); HUD_CYAN = rgb(240, 210, 130); HUD_WHITE = rgb(255, 245, 220); HUD_RED = rgb(255, 90, 60);
            break;
        case 2:   // cool white
            HUD_GREEN = rgb(220, 228, 235); HUD_DIM = rgb(110, 120, 135); HUD_AMBER = rgb(255, 240, 200); HUD_CYAN = rgb(160, 210, 240); HUD_WHITE = rgb(255, 255, 255); HUD_RED = rgb(255, 100, 90);
            break;
        default:  // classic green
            HUD_GREEN = rgb(120, 230, 150); HUD_DIM = rgb(60, 120, 80); HUD_AMBER = rgb(240, 190, 90); HUD_CYAN = rgb(110, 200, 230); HUD_WHITE = rgb(230, 235, 240); HUD_RED = rgb(240, 90, 70);
            break;
    }
    HUD_SHADOW = rgb(0, 0, 0);
}

const char* shortType(int type) {
    static const char* n[PT_COUNT] = {"MOLTEN", "CRATERED", "VENUSIAN", "FELISIAN", "ROCKY", "THIN ATMOS", "GAS GIANT", "ICY", "QUARTZ", "OCEAN", "METAL", "VOLCANIC", "CARBON", "SUBSTELLAR", "COMET", "COMPANION",
                                      "EUROPAN", "TECTONIC", "DESERT", "HYDROCARB", "BOMBARDED", "ACIDIC"};
    return n[type];
}
const char* compassName(double azRad) {
    static const char* names[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    int i = (int)std::floor(wrap2pi(azRad) / TAU * 8 + 0.5) & 7;
    return names[i];
}
