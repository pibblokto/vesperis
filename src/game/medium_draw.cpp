// H-05 (2026-10-10): the decoder names what a record was made on and draws it: a carved bone, a clay tablet, a scroll on its rods, a wax
// cylinder turning, a disc spinning, a reel's spools, a drive's light, a crystal's glow, something that hums. A record with no sound of its
// own (a tablet, a page, a bone flute) is read or played by the ship (`recordMediumSounds`, galaxy/records.h); R-414: the line names the
// object alone. The picture sits at the right of the shard's date and medium lines (40 x 26 of the 320 x 200 HUD).
#include "game.h"
#include "galaxy/records.h"
#include <cmath>

namespace {
void disc(RGBCanvas& c, int cx, int cy, int r, uint32_t col) { for (int dy = -r; dy <= r; dy++) { int w = (int)std::floor(std::sqrt((double)(r * r - dy * dy)) + 0.3); fillRectRGB(c, cx - w, cy + dy, cx + w, cy + dy, col); } }
void ellipse(RGBCanvas& c, int cx, int cy, int rx, int ry, uint32_t col) { for (int dy = -ry; dy <= ry; dy++) { double f = 1 - (double)(dy * dy) / std::max(1, ry * ry); int w = (int)std::floor(rx * std::sqrt(std::max(0.0, f)) + 0.3); fillRectRGB(c, cx - w, cy + dy, cx + w, cy + dy, col); } }
void ring(RGBCanvas& c, int cx, int cy, int r, uint32_t col) {
    int px = cx + r, py = cy;
    for (int k = 1; k <= 24; k++) { double a = k * 6.283185307179586 / 24; int x = cx + (int)std::lround(r * std::cos(a)), y = cy + (int)std::lround(r * std::sin(a)); drawLineRGB(c, px, py, x, y, col); px = x; py = y; }
}
uint32_t mixc(uint32_t a, uint32_t b, double t) {
    auto ch = [&](int s) { return (int)std::lround(((a >> s) & 255) * (1 - t) + ((b >> s) & 255) * t); };
    return rgb(ch(0), ch(8), ch(16));
}
// a hash for the marks on an object (the same object draws the same marks)
inline uint32_t mh(uint64_t s, int k) { uint64_t x = s + (uint64_t)k * 0x9E3779B97F4A7C15ULL; x ^= x >> 31; x *= 0xBF58476D1CE4E5B9ULL; x ^= x >> 29; return (uint32_t)x; }
}

// R-414 (2026-10-11): what the record is and no more (the user: "there is this overexplanation, like 'The Reel: Rough voice that clicks' - no
// need please, just say what kind of medium this is ... For all the shards just explanation of what it is"): the medium, and for a piece on
// a medium without a sound of its own what it is (a bone flute, notation, printed music); who reads or plays it, and the voice, went
std::string Game::mediumLine(const Shard& s) const {
    const int m = s.medium;
    if (m < 0 || m >= RM_COUNT) return std::string();
    if (s.music && !recordMediumSounds(m)) {
        switch (m) {
            case RM_BONE: return "A BONE FLUTE";
            case RM_SCROLL: return "NOTATION ON A SCROLL";
            case RM_CODEX: return "NOTATION IN A CODEX";
            case RM_PRINTED: return "PRINTED MUSIC";
            default: break;
        }
    }
    return RECORD_MEDIUM_HUD[m];
}

// the object, in the 40 x 26 box from (x0, y0): its shape and marks from the record's seed; turning, spinning, blinking or glowing while
// it is played (`playing`), `t` the screen's clock
void Game::drawMediumPanel(int x0, int y0, const Shard& s, bool playing, double t) {
    const int m = s.medium;
    if (m < 0 || m >= RM_COUNT) return;
    const uint64_t sd = s.seed;
    const int cx = x0 + 20, cy = y0 + 13;
    switch (m) {
        case RM_BONE: {   // a long bone with its knobbed ends; a tally's cuts, a flute's holes, a sky's dots
            const uint32_t bone = rgb(222, 212, 182), shade = rgb(160, 148, 118), mark = rgb(80, 66, 48);
            fillRectRGB(canvas, x0 + 6, cy - 2, x0 + 33, cy + 2, bone); fillRectRGB(canvas, x0 + 6, cy + 2, x0 + 33, cy + 2, shade);
            disc(canvas, x0 + 5, cy - 2, 3, bone); disc(canvas, x0 + 5, cy + 3, 3, bone); disc(canvas, x0 + 34, cy - 2, 3, bone); disc(canvas, x0 + 34, cy + 3, 3, bone);
            if (s.music) for (int k = 0; k < 5; k++) fillRectRGB(canvas, x0 + 10 + 5 * k, cy - 1, x0 + 11 + 5 * k, cy, rgb(40, 30, 22));
            else if (s.chart) for (int k = 0; k < 9; k++) { uint32_t h = mh(sd, k); fillRectRGB(canvas, x0 + 8 + (int)(h % 24), cy - 2 + (int)((h >> 8) % 4), x0 + 8 + (int)(h % 24), cy - 2 + (int)((h >> 8) % 4), mark); }
            else for (int k = 0; k < 11; k++) { int x = x0 + 9 + 2 * k + (k / 4); fillRectRGB(canvas, x, cy - 2, x, cy + (mh(sd, k) % 3 == 0 ? 2 : 0), mark); }
            break;
        }
        case RM_STONE: {   // a slab with its rows of signs
            const uint32_t st = rgb(132, 128, 118), hi = rgb(172, 168, 156), cut = rgb(62, 60, 56);
            fillRectRGB(canvas, x0 + 9, y0 + 2, x0 + 30, y0 + 24, st); fillRectRGB(canvas, x0 + 9, y0 + 2, x0 + 30, y0 + 2, hi); fillRectRGB(canvas, x0 + 9, y0 + 2, x0 + 9, y0 + 24, hi);
            for (int r = 0; r < 5; r++) for (int k = 0; k < 5; k++) { uint32_t h = mh(sd, r * 8 + k); int x = x0 + 11 + 4 * k, y = y0 + 5 + 4 * r; fillRectRGB(canvas, x, y, x + (int)(h % 2) + 1, y + (int)((h >> 3) % 2), cut); }
            break;
        }
        case RM_PAINTED: {   // a patch of rock wall: ochre and soot, a creature, hands
            const uint32_t rock = rgb(104, 82, 64), rock2 = rgb(88, 70, 56), ochre = rgb(204, 112, 52), red = rgb(162, 52, 36), soot = rgb(36, 30, 28);
            fillRectRGB(canvas, x0 + 2, y0 + 3, x0 + 37, y0 + 23, rock);
            for (int k = 0; k < 14; k++) { uint32_t h = mh(sd, k + 40); fillRectRGB(canvas, x0 + 3 + (int)(h % 33), y0 + 4 + (int)((h >> 8) % 18), x0 + 4 + (int)(h % 33), y0 + 4 + (int)((h >> 8) % 18), rock2); }
            if (s.chart) for (int k = 0; k < 10; k++) { uint32_t h = mh(sd, k); fillRectRGB(canvas, x0 + 5 + (int)(h % 30), y0 + 5 + (int)((h >> 8) % 16), x0 + 5 + (int)(h % 30), y0 + 5 + (int)((h >> 8) % 16), k % 3 ? ochre : rgb(230, 220, 200)); }
            else {
                fillRectRGB(canvas, x0 + 8, y0 + 11, x0 + 20, y0 + 14, red); fillRectRGB(canvas, x0 + 19, y0 + 8, x0 + 22, y0 + 11, red);   // a beast: body and head
                for (int k = 0; k < 4; k++) fillRectRGB(canvas, x0 + 9 + 3 * k, y0 + 15, x0 + 9 + 3 * k, y0 + 18, red);
                fillRectRGB(canvas, x0 + 27, y0 + 9, x0 + 31, y0 + 14, ochre); for (int k = 0; k < 4; k++) fillRectRGB(canvas, x0 + 27 + k + (k > 0), y0 + 6, x0 + 27 + k + (k > 0), y0 + 8, ochre);   // a hand
                fillRectRGB(canvas, x0 + 6, y0 + 20, x0 + 32, y0 + 20, soot);
            }
            break;
        }
        case RM_TABLET: {   // a clay tablet, rounded, its wedges in rows
            const uint32_t clay = rgb(178, 122, 78), dark = rgb(122, 80, 50), wedge = rgb(92, 58, 34);
            fillRectRGB(canvas, x0 + 9, y0 + 2, x0 + 30, y0 + 24, clay); fillRectRGB(canvas, x0 + 8, y0 + 3, x0 + 31, y0 + 23, clay);
            fillRectRGB(canvas, x0 + 9, y0 + 24, x0 + 30, y0 + 24, dark); fillRectRGB(canvas, x0 + 31, y0 + 3, x0 + 31, y0 + 23, dark);
            for (int r = 0; r < 6; r++) for (int k = 0; k < 6; k++) { if (mh(sd, r * 9 + k) % 5 == 0) continue; int x = x0 + 11 + 3 * k + (r % 2), y = y0 + 5 + 3 * r; fillRectRGB(canvas, x, y, x + 1, y, wedge); fillRectRGB(canvas, x, y + 1, x, y + 1, wedge); }
            break;
        }
        case RM_SCROLL: {   // parchment between its rods; the writing in lines, or a music's marks
            const uint32_t parch = rgb(214, 194, 150), ink = rgb(70, 54, 38), rod = rgb(102, 66, 36), knob = rgb(152, 110, 60);
            fillRectRGB(canvas, x0 + 6, y0 + 4, x0 + 33, y0 + 22, parch);
            fillRectRGB(canvas, x0 + 4, y0 + 2, x0 + 35, y0 + 3, rod); fillRectRGB(canvas, x0 + 4, y0 + 23, x0 + 35, y0 + 24, rod);
            fillRectRGB(canvas, x0 + 2, y0 + 2, x0 + 3, y0 + 3, knob); fillRectRGB(canvas, x0 + 36, y0 + 2, x0 + 37, y0 + 3, knob); fillRectRGB(canvas, x0 + 2, y0 + 23, x0 + 3, y0 + 24, knob); fillRectRGB(canvas, x0 + 36, y0 + 23, x0 + 37, y0 + 24, knob);
            for (int r = 0; r < 5; r++) {
                if (s.music) { fillRectRGB(canvas, x0 + 8, y0 + 7 + 3 * r, x0 + 31, y0 + 7 + 3 * r, rgb(176, 120, 96)); for (int k = 0; k < 5; k++) { uint32_t h = mh(sd, r * 7 + k); fillRectRGB(canvas, x0 + 9 + 5 * k, y0 + 5 + 3 * r + (int)(h % 3), x0 + 10 + 5 * k, y0 + 5 + 3 * r + (int)(h % 3), ink); } }
                else { int w = 18 + (int)(mh(sd, r) % 6); fillRectRGB(canvas, x0 + 9, y0 + 6 + 3 * r, x0 + 9 + w, y0 + 6 + 3 * r, ink); }
            }
            break;
        }
        case RM_CODEX: {   // an open book: two pages, the spine, the lines, a red initial
            const uint32_t page = rgb(226, 216, 190), cover = rgb(112, 50, 34), ink = rgb(82, 72, 60);
            fillRectRGB(canvas, x0 + 2, y0 + 4, x0 + 37, y0 + 24, cover);
            fillRectRGB(canvas, x0 + 4, y0 + 3, x0 + 19, y0 + 22, page); fillRectRGB(canvas, x0 + 21, y0 + 3, x0 + 35, y0 + 22, page);
            fillRectRGB(canvas, x0 + 20, y0 + 3, x0 + 20, y0 + 23, rgb(80, 36, 24));
            fillRectRGB(canvas, x0 + 6, y0 + 5, x0 + 8, y0 + 7, rgb(170, 40, 30));
            for (int r = 0; r < 6; r++) { fillRectRGB(canvas, x0 + (r ? 6 : 10), y0 + 6 + 3 * r, x0 + 17 - (int)(mh(sd, r) % 3), y0 + 6 + 3 * r, ink); fillRectRGB(canvas, x0 + 23, y0 + 6 + 3 * r, x0 + 33 - (int)(mh(sd, r + 9) % 3), y0 + 6 + 3 * r, ink); }
            break;
        }
        case RM_PLATE: {   // an engraved plate: a brass disc, its rings and its rule
            const uint32_t brass = rgb(192, 150, 70), cut = rgb(112, 80, 32), hi = rgb(232, 202, 122);
            disc(canvas, cx, cy, 12, brass); ring(canvas, cx, cy, 12, cut); ring(canvas, cx, cy, 8, cut); ring(canvas, cx, cy, 4, cut);
            const double a = 0.6 + (playing ? t * 0.4 : 0);
            drawLineRGB(canvas, cx - (int)std::lround(11 * std::cos(a)), cy - (int)std::lround(11 * std::sin(a)), cx + (int)std::lround(11 * std::cos(a)), cy + (int)std::lround(11 * std::sin(a)), hi);
            fillRectRGB(canvas, cx - 1, y0, cx + 1, y0 + 1, brass);
            break;
        }
        case RM_CYLINDER: {   // a wax cylinder on its mandrel, its grooves turning by while it plays
            const uint32_t wax = rgb(96, 62, 34), hi = rgb(156, 112, 68), groove = rgb(64, 40, 22), metal = rgb(150, 150, 158);
            fillRectRGB(canvas, x0 + 2, cy - 1, x0 + 37, cy, metal);
            fillRectRGB(canvas, x0 + 8, y0 + 5, x0 + 31, y0 + 21, wax);
            ellipse(canvas, x0 + 8, cy, 3, 8, rgb(120, 82, 48)); ellipse(canvas, x0 + 31, cy, 3, 8, wax);
            fillRectRGB(canvas, x0 + 9, y0 + 7, x0 + 30, y0 + 7, hi);
            const int off = playing ? (int)(t * 18) % 3 : 0;
            for (int x = x0 + 10 + off; x <= x0 + 29; x += 3) fillRectRGB(canvas, x, y0 + 8, x, y0 + 20, groove);
            fillRectRGB(canvas, x0 + 22, y0 + 1, x0 + 23, y0 + 5, metal);   // the stylus' arm
            break;
        }
        case RM_GLASS: {   // a glass plate: pale, its edges bright, the image dark on it
            const uint32_t glass = rgb(168, 186, 196), edge = rgb(206, 220, 228), img = rgb(46, 52, 58);
            fillRectRGB(canvas, x0 + 5, y0 + 3, x0 + 34, y0 + 23, glass); drawRectRGB(canvas, x0 + 5, y0 + 3, x0 + 34, y0 + 23, edge);
            for (int k = 0; k < 16; k++) { uint32_t h = mh(sd, k); int x = x0 + 7 + (int)(h % 26), y = y0 + 5 + (int)((h >> 8) % 17); fillRectRGB(canvas, x, y, x + ((h >> 16) % 4 == 0), y + ((h >> 16) % 4 == 0), img); }
            break;
        }
        case RM_PRINTED: {   // printed pages: a headline, columns of type
            const uint32_t paper = rgb(222, 218, 204), ink = rgb(42, 42, 42), grey = rgb(120, 118, 110);
            fillRectRGB(canvas, x0 + 7, y0 + 3, x0 + 32, y0 + 24, grey); fillRectRGB(canvas, x0 + 6, y0 + 2, x0 + 31, y0 + 23, paper);
            fillRectRGB(canvas, x0 + 8, y0 + 4, x0 + 29, y0 + 5, ink);
            for (int col = 0; col < 3; col++) for (int r = 0; r < 7; r++) { if (s.music && r % 3 == 2) continue; int x = x0 + 8 + 8 * col; fillRectRGB(canvas, x, y0 + 8 + 2 * r, x + 5 - (int)(mh(sd, col * 9 + r) % 2), y0 + 8 + 2 * r, s.music ? grey : ink); }
            if (s.music) for (int k = 0; k < 6; k++) fillRectRGB(canvas, x0 + 9 + 4 * k, y0 + 9 + (int)(mh(sd, k) % 4), x0 + 10 + 4 * k, y0 + 10 + (int)(mh(sd, k) % 4), ink);
            break;
        }
        case RM_REEL: {   // two spools, the tape between, their spokes turning while it plays
            const uint32_t metal = rgb(154, 154, 160), tape = rgb(92, 62, 40), dark = rgb(40, 40, 44);
            const double a = playing ? t * 5.0 : 0.4;
            for (int sp = 0; sp < 2; sp++) {
                int sx = x0 + 9 + 22 * sp;
                disc(canvas, sx, cy - 2, 8, metal); disc(canvas, sx, cy - 2, sp ? 4 : 6, tape); disc(canvas, sx, cy - 2, 2, dark);
                for (int k = 0; k < 3; k++) { double b = a * (sp ? 1.4 : 1.0) + k * 2.094; drawLineRGB(canvas, sx, cy - 2, sx + (int)std::lround(7 * std::cos(b)), cy - 2 + (int)std::lround(7 * std::sin(b)), dark); }
            }
            fillRectRGB(canvas, x0 + 9, y0 + 22, x0 + 31, y0 + 22, tape);
            break;
        }
        case RM_DISC: {   // a disc in its grooves, the label, a glint going round while it plays
            const uint32_t black = rgb(24, 24, 28), groove = rgb(48, 48, 54), label = mixc(rgb(170, 44, 40), rgb(60, 90, 170), (mh(sd, 1) % 100) / 100.0);
            disc(canvas, cx, cy, 12, black); for (int r = 5; r <= 11; r += 2) ring(canvas, cx, cy, r, groove); disc(canvas, cx, cy, 4, label); fillRectRGB(canvas, cx, cy, cx, cy, black);
            const double a = playing ? t * 3.5 : 0.8;
            drawLineRGB(canvas, cx + (int)std::lround(5 * std::cos(a)), cy + (int)std::lround(5 * std::sin(a)), cx + (int)std::lround(11 * std::cos(a)), cy + (int)std::lround(11 * std::sin(a)), rgb(150, 150, 160));
            break;
        }
        case RM_DRIVE: {   // a drive: a dark box, its face, its light blinking while it is read
            const uint32_t body = rgb(52, 56, 62), face = rgb(74, 80, 88), pins = rgb(170, 150, 80);
            fillRectRGB(canvas, x0 + 6, y0 + 6, x0 + 33, y0 + 21, body); fillRectRGB(canvas, x0 + 8, y0 + 8, x0 + 31, y0 + 17, face);
            for (int k = 0; k < 6; k++) fillRectRGB(canvas, x0 + 10 + 4 * k, y0 + 22, x0 + 11 + 4 * k, y0 + 23, pins);
            const bool lit = playing ? ((int)(t * 9) % 3 != 0) : true;
            fillRectRGB(canvas, x0 + 28, y0 + 18, x0 + 30, y0 + 19, lit ? (playing ? rgb(90, 230, 130) : rgb(60, 120, 80)) : rgb(30, 50, 36));
            break;
        }
        case RM_CRYSTAL: {   // a crystal: a long prism of facets, glowing while it is read
            const double g = playing ? 0.5 + 0.5 * std::sin(t * 3.0) : 0.25;
            const uint32_t a = mixc(rgb(70, 140, 160), rgb(170, 245, 250), g), b = mixc(rgb(50, 100, 130), rgb(120, 210, 235), g), c = mixc(rgb(110, 170, 190), rgb(220, 255, 255), g);
            for (int dy = 0; dy <= 22; dy++) { int w = dy < 4 ? dy : (dy > 18 ? 22 - dy : 4); fillRectRGB(canvas, cx - w * 2, y0 + 2 + dy, cx - 1, y0 + 2 + dy, a); fillRectRGB(canvas, cx, y0 + 2 + dy, cx + w * 2, y0 + 2 + dy, b); }
            fillRectRGB(canvas, cx - 1, y0 + 4, cx - 1, y0 + 20, c);
            break;
        }
        default: {   // something that hums: an ovoid of no colour we name, its rings pulsing out while it plays
            const uint32_t body = rgb(44, 34, 66), rim = rgb(96, 70, 140);
            ellipse(canvas, cx, cy, 11, 9, body);
            const double ph = playing ? std::fmod(t * 0.9, 1.0) : 0.3;
            for (int k = 0; k < 3; k++) { double u = std::fmod(ph + k / 3.0, 1.0); int r = 3 + (int)(u * 12); ring(canvas, cx, cy, r, mixc(rgb(200, 150, 255), rgb(30, 24, 40), u)); }
            ellipse(canvas, cx, cy, 2, 2, rim);
            break;
        }
    }
}
