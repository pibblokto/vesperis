// 5x7 pixel font drawn straight into the RGB output buffer (after the mush
// filter, so text stays crisp while the world stays soft).
#pragma once
#include <cstdint>
#include <string>

constexpr int FONT_W = 5, FONT_H = 7, FONT_ADV = 6, FONT_LINE = 9;
// Special glyph codes usable inside strings.
constexpr char CH_DEGREE = '\x01', CH_UP = '\x02', CH_DOWN = '\x03', CH_LEFT = '\x04', CH_RIGHT = '\x05',
               CH_BLOCK = '\x06', CH_DOT = '\x07';

// The HUD is laid out in 320x200 logical units; `scale` maps them to the framebuffer
// (glyphs and lines grow with it, so the HUD looks the same at every resolution).
struct RGBCanvas {
    uint32_t* px;   // w x h, 0xAABBGGRR
    int w, h;
    int scale = 1;
};

// Returns the width in pixels of the text.
int textWidth(const char* s, int scale = 1);
void drawText(RGBCanvas& c, int x, int y, const char* s, uint32_t color, int scale = 1);
void drawTextShadow(RGBCanvas& c, int x, int y, const char* s, uint32_t color, uint32_t shadow, int scale = 1);
void drawTextCentered(RGBCanvas& c, int cx, int y, const char* s, uint32_t color, int scale = 1);
void fillRectRGB(RGBCanvas& c, int x0, int y0, int x1, int y1, uint32_t color);
void blendRectRGB(RGBCanvas& c, int x0, int y0, int x1, int y1, uint32_t color, int alpha255);
void drawRectRGB(RGBCanvas& c, int x0, int y0, int x1, int y1, uint32_t color);
void drawLineRGB(RGBCanvas& c, int x0, int y0, int x1, int y1, uint32_t color);
inline uint32_t rgb(int r, int g, int b) { return 0xFF000000u | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r; }
