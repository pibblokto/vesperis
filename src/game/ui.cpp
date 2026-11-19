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
void drawPanel(RGBCanvas& c, int x0, int y0, int x1, int y1, uint32_t fill, int alpha, uint32_t edge) {
    blendRectRGB(c, x0, y0, x1, y1, fill, alpha);
    drawRectRGB(c, x0, y0, x1, y1, edge);
}

void warmthTyping(const Input& in, std::string& s, double now) {
    const bool minus = !s.empty() && s[0] == '-';
    const int d = digitKey(in);
    if (d >= 0 && (int)s.size() - (minus ? 1 : 0) < 3) s += (char)('0' + d);
    if (in.wasPressed(KEY_MINUS) || in.wasPressed(333)) s = minus ? s.substr(1) : "-" + s;   // 333: GLFW's keypad minus
    if (in.wasPressed(KEY_BACKSPACE) && !s.empty()) s.pop_back();
    const int step = in.wasPressed(KEY_UP) ? 1 : (in.wasPressed(KEY_DOWN) ? -1 : (in.wasPressed(KEY_PAGE_UP) ? 10 : (in.wasPressed(KEY_PAGE_DOWN) ? -10 : 0)));
    if (step) s = std::to_string((s.empty() || s == "-" ? (int)std::lround(now) : atoi(s.c_str())) + step);
}

bool menuStep(const Input& in, int& sel, int n, bool sideways) {
    const bool up = in.wasPressed(KEY_UP) || (sideways && in.wasPressed(KEY_LEFT)), down = in.wasPressed(KEY_DOWN) || (sideways && in.wasPressed(KEY_RIGHT));
    if (up) sel = (sel + n - 1) % n;
    if (down) sel = (sel + 1) % n;
    return up || down;
}

std::string upper(std::string s) { for (char& c : s) c = (char)toupper((unsigned char)c); return s; }
std::string thousands(double v) {   // 13050 -> "13,050" (G-05; C-15's radar)
    const long long n = std::llround(std::fabs(v));
    const std::string d = std::to_string(n);
    std::string out;
    for (size_t i = 0; i < d.size(); i++) { if (i && (d.size() - i) % 3 == 0) out += ','; out += d[i]; }
    return (v < -0.5 ? "-" : "") + out;
}

// Z-05: a span of years as the lab says it, to two figures: "ABOUT 12,000 YEARS", "ABOUT 1.2 MILLION YEARS"
std::string yearsWords(double y) {
    auto two = [](double v) { const double p = std::pow(10.0, std::floor(std::log10(std::max(1.0, v))) - 1); return v < 10 ? fmt("%.2g", v) : thousands(std::round(v / p) * p); };
    if (y >= 1e9) return "ABOUT " + two(y / 1e9) + " BILLION YEARS";   // two figures (Z-06: 230 million, not 2.3e+02)
    if (y >= 1e6) return "ABOUT " + two(y / 1e6) + " MILLION YEARS";
    const double p = std::pow(10.0, std::floor(std::log10(std::max(1.0, y))) - 1);
    return "ABOUT " + thousands(std::max(1.0, std::round(y / p) * p)) + " YEARS";
}

std::string fitText(const std::string& s, int width) {
    if (textWidth(s.c_str()) <= width) return s;
    std::string t = s;
    while (!t.empty() && textWidth((t + ".").c_str()) > width) { const size_t sp = t.find_last_of(' '); t = sp == std::string::npos ? t.substr(0, t.size() - 1) : t.substr(0, sp); }
    return t + ".";
}

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

const char* shortType(int type) { return type >= 0 && type < PT_COUNT ? KINDS[type].shortName : "?"; }   // P-00: the kind's short name (ten characters)
const char* compassName(double azRad) {
    static const char* const names[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    int i = (int)std::floor(wrap2pi(azRad) / TAU * 8 + 0.5) & 7;
    return names[i];
}
