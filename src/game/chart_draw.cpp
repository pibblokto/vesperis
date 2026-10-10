// C-10 (2026-10-03): a shard that is a star chart (`galaxy/charts.h`) draws itself on the decoder beside its caption: the hemisphere of
// the people's sky round the star they marked, their figures and their names (words of the language, resolving as it is learnt); a
// first reading draws it over a few seconds; Enter targets the marked star and O holds the chart up to the sky (`drawChartOverlay`).
// H-05 (2026-10-10): moved here from game/shards_screen.cpp, and drawn in the way of its record's age and medium (`StarChart::style`):
// ochre and soot on a cave's ceiling with the animals painted under the stars and the moon's days in a row of soot; the brightest
// scratched on a bone with the moon's nights notched on its edge; a vault painted blue with gold stars; an astrolabe's brass plate with
// its graduated limb, their equator, tropics and ecliptic engraved, the rete's pointers and the rule; a codex's page in brown and red
// ink with the wanderers' paths; an engraved atlas with the grid of their coordinates and the catalogue's numbers; a photograph on
// glass (a negative: dark stars on the pale plate, its reseau, the mark ringed in ink); a survey by magnitudes (on glass, printed or on
// a drive's screen) with the peoples heard ringed; a navigation chart of their neighbourhood seen from above and aslant with the routes
// flown and meant; the far future's chart of the places where nothing is round a mark thousands of light years off. The object is the
// people's; the decoder's own marks over it stay the HUD's (the mark's amber diamond, the names it reads, on a dark plate where the
// object is pale).
#include "game.h"
#include "ui.h"
#include "core/rng.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr int CX = 84, CY = 104, CR = 66;                    // a sky chart's disc on the decoder (logical pixels)
constexpr int BX0 = 4, BY0 = 31, BX1 = 164, BY1 = 176;       // the chart's box: the object lies within it
constexpr int BW = BX1 - BX0 + 1, BH = BY1 - BY0 + 1;

uint32_t col3(int r, int g, int b) { return rgb(std::max(0, std::min(255, r)), std::max(0, std::min(255, g)), std::max(0, std::min(255, b))); }
int chR(uint32_t c) { return c & 255; }
int chG(uint32_t c) { return (c >> 8) & 255; }
int chB(uint32_t c) { return (c >> 16) & 255; }
uint32_t scaled(uint32_t c, double f) { return col3((int)(chR(c) * f), (int)(chG(c) * f), (int)(chB(c) * f)); }
uint32_t mixed(uint32_t a, uint32_t b, double t) { return col3((int)(chR(a) + (chR(b) - chR(a)) * t), (int)(chG(a) + (chG(b) - chG(a)) * t), (int)(chB(a) + (chB(b) - chB(a)) * t)); }
double hashU(int x, int y, uint64_t s) { return unitFromHash(hash2i(x, y, s)); }
double vnoise(double x, double y, uint64_t s) {
    const int x0 = (int)std::floor(x), y0 = (int)std::floor(y);
    double fx = x - x0, fy = y - y0; fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
    const double a = hashU(x0, y0, s), b = hashU(x0 + 1, y0, s), c = hashU(x0, y0 + 1, s), d = hashU(x0 + 1, y0 + 1, s);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}

// the decoder's canvas in its logical pixels, clipped to the screen
struct Painter {
    RGBCanvas& c;
    void px(int x, int y, uint32_t col) { if (x >= 0 && y >= 0 && x < UW && y < UH) fillRectRGB(c, x, y, x, y, col); }
    void mix(int x, int y, uint32_t col, int a) { if (x >= 0 && y >= 0 && x < UW && y < UH) blendRectRGB(c, x, y, x, y, col, a); }
    void line(double x0, double y0, double x1, double y1, uint32_t col, int a = 255, int on = 0, int off = 0) {   // a dashed one: `on` pixels drawn, `off` skipped
        const int ix0 = (int)std::lround(x0), iy0 = (int)std::lround(y0), ix1 = (int)std::lround(x1), iy1 = (int)std::lround(y1);
        const int steps = std::max(std::abs(ix1 - ix0), std::abs(iy1 - iy0));
        if (steps > 2000) return;
        for (int i = 0; i <= steps; i++) {
            if (on > 0 && i % (on + off) >= on) continue;
            const int x = steps ? ix0 + (ix1 - ix0) * i / steps : ix0, y = steps ? iy0 + (iy1 - iy0) * i / steps : iy0;
            if (a >= 255) px(x, y, col); else mix(x, y, col, a);
        }
    }
    void disc(double cx, double cy, double r, uint32_t col, int a = 255) {
        for (int y = (int)std::floor(cy - r); y <= (int)std::ceil(cy + r); y++)
            for (int x = (int)std::floor(cx - r); x <= (int)std::ceil(cx + r); x++)
                if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r + 0.25) { if (a >= 255) px(x, y, col); else mix(x, y, col, a); }
    }
    void ring(double cx, double cy, double rx, double ry, uint32_t col, int a = 255, int dashOn = 0) {
        const int steps = std::max(12, (int)(TAU * std::max(rx, ry) * 1.2));
        int lx = -10000, ly = -10000;
        for (int i = 0; i < steps; i++) {
            if (dashOn > 0 && (i / dashOn) % 2) continue;
            const int x = (int)std::lround(cx + rx * std::cos(TAU * i / steps)), y = (int)std::lround(cy + ry * std::sin(TAU * i / steps));
            if (x == lx && y == ly) continue;
            lx = x; ly = y;
            if (a >= 255) px(x, y, col); else mix(x, y, col, a);
        }
    }
    void glyph(int g, int x, int y, int scale, uint32_t col, int a = 255) {   // a CHART_GLYPH_ROWS glyph, its top left at (x, y)
        if (g < 0 || g >= CHART_GLYPHS) return;
        for (int r = 0; r < CHART_GLYPH_H; r++) for (int k = 0; k < CHART_GLYPH_W; k++) {
            if (CHART_GLYPH_ROWS[g][r][k] != '#') continue;
            for (int sy = 0; sy < scale; sy++) for (int sx = 0; sx < scale; sx++) { if (a >= 255) px(x + k * scale + sx, y + r * scale + sy, col); else mix(x + k * scale + sx, y + r * scale + sy, col, a); }
        }
    }
};

// the object under the chart, built once for the chart on the screen (a pixel of the box each; 0 nothing: the decoder's dark shows)
struct Backdrop { uint64_t key = ~0ULL; std::vector<uint32_t> px; };
Backdrop g_back;

// the bone's shape: a shaft from end to end, a little aslant, with its knobs; `inside` how far within (0 the edge .. 1 the middle)
struct BoneShape {
    double ax = CX - 56, ay = CY + 9, bx = CX + 56, by = CY - 9;
    bool at(double x, double y, double& inside, double& across) const {
        const double dx = bx - ax, dy = by - ay, L2 = dx * dx + dy * dy;
        double t = ((x - ax) * dx + (y - ay) * dy) / L2;
        const double tc = std::max(0.0, std::min(1.0, t));
        const double px = ax + dx * tc, py = ay + dy * tc;
        const double nx = -dy / std::sqrt(L2), ny = dx / std::sqrt(L2);
        across = (x - px) * nx + (y - py) * ny;
        const double half = 19 + 5 * std::pow(std::fabs(t - 0.5) * 2, 3);   // the shaft swells toward its ends
        double best = t >= 0 && t <= 1 ? 1 - std::fabs(across) / half : -1;
        for (int e = 0; e < 2; e++) {   // the knobs: two lobes at each end
            const double ex = e ? bx + dx / std::sqrt(L2) * 9 : ax - dx / std::sqrt(L2) * 9, ey = e ? by + dy / std::sqrt(L2) * 9 : ay - dy / std::sqrt(L2) * 9;
            for (int l = -1; l <= 1; l += 2) {
                const double lx = ex + nx * 11 * l, ly = ey + ny * 11 * l, r = 16.5;
                const double d = std::sqrt((x - lx) * (x - lx) + (y - ly) * (y - ly));
                best = std::max(best, 1 - d / r);
            }
        }
        inside = best;
        return best > 0;
    }
};
// the navigation chart's view: their plane from above (the galaxy's heart up), a star's height over it a tick up or down
constexpr double MAP_SQ = 1.0;   // the plane's rings as seen: round from above
void onMap(const ChartStar& s, double& px, double& py, double& bx, double& by) {
    bx = CX + s.x * CR; by = CY - s.y * CR * MAP_SQ;
    px = bx; py = by;
}

uint32_t classCol(int cls) { const RGB& c = STAR_CLASSES[std::max(0, std::min(STAR_CLASS_COUNT - 1, cls))].color; return col3((int)(c.r * 255), (int)(c.g * 255), (int)(c.b * 255)); }

void buildBackdrop(const StarChart& C, uint64_t key) {
    g_back.key = key;
    g_back.px.assign((size_t)BW * BH, 0);
    const uint64_t s = mix64(C.seed ^ 0xBAC4D0ULL);
    auto put = [&](int x, int y, uint32_t c) { if (x >= BX0 && x <= BX1 && y >= BY0 && y <= BY1) g_back.px[(size_t)(y - BY0) * BW + (x - BX0)] = c; };
    switch (C.style) {
        case CHS_CEILING: {   // the rock of the cave's roof, lit from below by a lamp, with its cracks and old paint
            static const uint32_t ROCK[4] = {col3(112, 94, 76), col3(120, 102, 82), col3(100, 92, 84), col3(118, 96, 70)};
            const uint32_t base = ROCK[s % 4];
            for (int y = BY0; y <= BY1; y++) for (int x = BX0; x <= BX1; x++) {
                const double dx = (x - CX) / 79.0, dy = (y - CY) / 72.0, r = std::sqrt(dx * dx + dy * dy), a = std::atan2(dy, dx);
                const double edge = 0.9 + 0.08 * (vnoise(std::cos(a) * 2.5 + 7, std::sin(a) * 2.5 + 7, s) * 2 - 1) + 0.04 * (vnoise(std::cos(a) * 7 + 3, std::sin(a) * 7 + 3, s + 1) * 2 - 1);
                if (r > edge) continue;
                const double tex = 0.55 * vnoise(x / 10.0, y / 10.0, s + 2) + 0.3 * vnoise(x / 4.0, y / 4.0, s + 3) + 0.15 * hashU(x, y, s + 4);
                double light = 1.0 - 0.55 * r * r;
                const double crack = std::fabs(vnoise(x / 17.0, y / 17.0, s + 5) - 0.5);
                if (crack < 0.016) light *= 0.55;
                if (r > edge - 0.06) light *= 0.65;
                uint32_t c = scaled(base, (0.62 + 0.55 * tex) * light);
                const double old = vnoise(x / 23.0, y / 23.0, s + 6);
                if (old > 0.78) c = mixed(c, col3(130, 60, 40), (old - 0.78) * 1.4);   // the ochre of older paintings under these
                put(x, y, c);
            }
            break;
        }
        case CHS_BONE: {   // a long bone, the light from above, its grain along it
            BoneShape B;
            for (int y = BY0; y <= BY1; y++) for (int x = BX0; x <= BX1; x++) {
                double in, across; if (!B.at(x, y, in, across)) continue;
                const double grain = 0.75 + 0.25 * vnoise(x / 9.0, y * 1.8, s) + 0.08 * hashU(x, y, s + 1);
                const double shade = 1.0 - 0.35 * std::max(-1.0, std::min(1.0, across / 22.0)) - (in < 0.06 ? 0.3 : 0);
                put(x, y, scaled(col3(214, 201, 170), grain * shade));
            }
            break;
        }
        case CHS_VAULT: {   // a bay of the vault: a lunette of ultramarine over a red border with its gold
            const int sx0 = 8, sx1 = 160, spring = 31 + 76;
            for (int y = BY0; y <= BY1; y++) for (int x = sx0; x <= sx1; x++) {
                const double ax = x - (sx0 + sx1) / 2.0;
                const double dIn = y < spring ? 76.0 - std::sqrt(ax * ax + (spring - y) * (spring - y)) : std::min({(double)(x - sx0), (double)(sx1 - x), (double)(BY1 - y)});
                if (y < spring && dIn < 0) continue;
                const double tex = 0.88 + 0.12 * vnoise(x / 6.0, y / 6.0, s) + 0.05 * hashU(x, y, s + 1);
                uint32_t c;
                if (dIn < 5) { c = scaled(col3(138, 52, 38), tex); if (dIn >= 1.5 && dIn < 3.5 && ((x + y) % 4) == 0) c = col3(214, 172, 82); }   // the border, its gold dots
                else c = scaled(col3(24, 38, 104), tex * (0.85 + 0.15 * (y - BY0) / (double)BH));
                put(x, y, c);
            }
            break;
        }
        case CHS_ASTROLABE: {   // the brass plate in its limb, brushed round, the limb graduated, the throne on top
            for (int y = BY0; y <= BY1; y++) for (int x = BX0; x <= BX1; x++) {
                const double dx = x - CX, dy = y - CY, r = std::sqrt(dx * dx + dy * dy), a = std::atan2(dy, dx);
                const bool throne = y >= CY - 75 && y <= CY - CR - 3 && std::fabs(dx) <= 6 + (y - (CY - 75)) * 0.5;
                if (r > CR + 6 && !throne) continue;
                const double brushed = 0.9 + 0.1 * vnoise(a * 40, r / 3.0, s) + 0.05 * hashU(x, y, s + 1);
                uint32_t c = scaled(col3(176, 134, 62), brushed * (1.05 - 0.25 * r / CR));
                if (r > CR || throne) {   // the limb: darker, its ticks every five degrees, longer every fifteen
                    c = scaled(col3(150, 110, 48), brushed);
                    const double deg = (a + PI) / DEG; const double m5 = std::fmod(deg + 0.5, 5.0);
                    if (!throne && m5 < 1.0 && (r < CR + 3 || std::fmod(deg + 0.5, 15.0) < 1.0)) c = col3(84, 58, 24);
                }
                if (std::fabs(r - CR) < 0.6 || std::fabs(r - CR - 6) < 0.6) c = col3(90, 62, 26);
                put(x, y, c);
            }
            break;
        }
        case CHS_PAGE: {   // parchment: mottled, darker at its edges and in the gutter, a stain, a double rule in red
            const int px0 = 8, px1 = 160, py0 = 32, py1 = 174;
            for (int y = BY0; y <= BY1; y++) for (int x = BX0; x <= BX1; x++) {
                const double deckle = 1.5 * vnoise(x / 3.0, y / 3.0, s + 7);
                if (x < px0 - deckle || x > px1 + deckle || y < py0 - deckle || y > py1 + deckle) continue;
                const double edgeD = std::min({x - px0 + 0.0, px1 - x + 0.0, y - py0 + 0.0, py1 - y + 0.0});
                double f = 0.9 + 0.1 * vnoise(x / 8.0, y / 8.0, s) + 0.04 * hashU(x, y, s + 1);
                if (edgeD < 6) f *= 0.82 + 0.03 * edgeD;
                if (x < px0 + 12) f *= 0.72 + 0.28 * (x - px0) / 12.0;   // the gutter
                const double stain = vnoise(x / 21.0, y / 21.0, s + 2);
                if (stain > 0.8) f *= 1 - (stain - 0.8) * 0.8;
                uint32_t c = scaled(col3(210, 190, 148), f);
                if ((std::fabs(edgeD - 5) < 0.5 || std::fabs(edgeD - 7) < 0.5) && x >= px0 + 4) c = col3(158, 50, 38);
                put(x, y, c);
            }
            break;
        }
        case CHS_ATLAS: {   // paper and the engraver's double border
            for (int y = BY0; y <= BY1; y++) for (int x = BX0 + 2; x <= BX1 - 2; x++) {
                const double edgeD = std::min({x - (BX0 + 2.0), BX1 - 2.0 - x, y - (double)BY0, (double)BY1 - y});
                uint32_t c = scaled(col3(229, 222, 204), 0.96 + 0.04 * vnoise(x / 5.0, y / 5.0, s));
                if (std::fabs(edgeD - 3) < 0.5 || std::fabs(edgeD - 5) < 0.5) c = col3(40, 36, 32);
                put(x, y, c);
            }
            break;
        }
        case CHS_PLATE: case CHS_SURVEY: {
            const bool glass = C.style == CHS_PLATE || C.medium == RM_GLASS, screen = C.style == CHS_SURVEY && C.medium == RM_DRIVE;
            const int px0 = glass ? 12 : 6, px1 = glass ? 156 : 162, py0 = glass ? 33 : 31, py1 = glass ? 173 : 176;
            for (int y = BY0; y <= BY1; y++) for (int x = BX0; x <= BX1; x++) {
                if (x < px0 || x > px1 || y < py0 || y > py1) continue;
                if (C.style == CHS_PLATE && (px1 - x) + (y - py0) < 13) continue;   // the corner chipped off
                const double edgeD = std::min({x - (double)px0, (double)px1 - x, y - (double)py0, (double)py1 - y});
                uint32_t c;
                if (screen) { c = col3(6, 11, 16); if (y % 2) c = col3(4, 8, 12); if (edgeD < 1) c = col3(40, 90, 100); }
                else if (glass) {
                    c = scaled(col3(198, 204, 192), 0.95 + 0.05 * vnoise(x / 7.0, y / 7.0, s));
                    if (edgeD < 2) c = col3(160, 168, 152);
                    else if (C.style == CHS_PLATE && ((x - px0) % 12 == 6 || (y - py0) % 12 == 6)) c = col3(178, 186, 172);   // the reseau
                } else {
                    c = scaled(col3(236, 236, 229), 0.97 + 0.03 * vnoise(x / 6.0, y / 6.0, s));
                    if (edgeD < 0.5 || std::fabs(edgeD - 2) < 0.5) c = col3(30, 30, 30);
                    else if (edgeD < 6 && edgeD >= 3 && ((x + y) % 10 == 0)) c = col3(60, 60, 60);   // the border's ticks
                }
                put(x, y, c);
            }
            break;
        }
        case CHS_ROUTES: {   // the pilots' display: a dark panel, the plane's rings aslant
            for (int y = BY0; y <= BY1; y++) for (int x = BX0; x <= BX1; x++) {
                const double edgeD = std::min({x - (double)BX0, (double)BX1 - x, y - (double)BY0, (double)BY1 - y});
                uint32_t c = col3(4, 11, 22);
                if (y % 3 == 0) c = col3(3, 9, 18);
                if (edgeD < 1) c = col3(30, 80, 96);
                put(x, y, c);
            }
            break;
        }
        default: {   // the far future's: a black that is almost violet, a few motes that are not stars
            for (int y = BY0; y <= BY1; y++) for (int x = BX0; x <= BX1; x++) {
                const double dx = x - CX, dy = y - CY, r = std::sqrt(dx * dx + dy * dy);
                if (r > CR + 8) continue;
                const double f = 1 - r / (CR + 8);
                uint32_t c = col3((int)(8 + 10 * f), (int)(4 + 4 * f), (int)(16 + 22 * f));
                if (hashU(x, y, s) > 0.993) c = col3(60, 50, 80);
                put(x, y, c);
            }
            break;
        }
    }
}

void drawBackdrop(Painter& P) {
    for (int y = BY0; y <= BY1; y++) for (int x = BX0; x <= BX1; x++) { const uint32_t c = g_back.px[(size_t)(y - BY0) * BW + (x - BX0)]; if (c) P.px(x, y, c); }
}

// a star's place on the decoder (a sky chart's disc, the bone, the map), false when it is off the object
bool starAt(const StarChart& C, const ChartStar& s, double& px, double& py) {
    if (C.style == CHS_ROUTES) {
        double bx, by; onMap(s, px, py, bx, by);
        const double ex = (px - CX) / CR, ey = (by - CY) / (CR * MAP_SQ);
        return ex * ex + ey * ey <= 1.0001;
    }
    if (C.style == CHS_BONE) {
        px = CX + s.x * 68; py = CY - s.y * 27 - s.x * 11;
        BoneShape B; double in, ac; return B.at(px, py, in, ac) && in > 0.12;
    }
    px = CX + s.x * CR; py = CY - s.y * CR;
    return true;
}

// a 5 x 5 sign of the far future's own, from a hash (mirrored, so it reads as made)
void farSign(Painter& P, uint64_t h, int x, int y, uint32_t col) {
    for (int r = 0; r < 5; r++) for (int k = 0; k < 3; k++) if ((h >> (r * 3 + k)) & 1) { P.px(x + k, y + r, col); P.px(x + 4 - k, y + r, col); }
}
}

// a figure's name ("the hunter") as the computer reads it with that many shards of the open world held: the known words ours,
// the rest the people's
std::string Game::figureLabel(const std::string& name, int held) const {
    Shard tmp; tmp.text = name;
    std::vector<DecodedWord> w; decodeShard(tmp, shardWorld.lang, shardWorld.tongue, held, w);
    return upper(decodedText(w));
}

void Game::holdChart(bool up) {
    if (!up) { chartUp = false; status("THE CHART IS PUT DOWN", 3); audio.beep = 4; return; }
    if (!shardWorld.valid || !shardWorld.chart.valid || shardHeld.empty()) return;
    chartHeld = shardWorld.chart; chartHeldKey = shardWorld.key; chartHeldIndex = shardWorld.base + shardHeld[shardSel]; chartHeldPeople = shardWorld.lore.people;   // C-13: the world index
    chartHeldLabels.clear();
    for (const ChartFigure& f : chartHeld.figures) chartHeldLabels.push_back(figureLabel(f.name, (int)shardHeld.size()));
    chartUp = true;
    status("THE CHART IS HELD UP - ITS FIGURES SHOW ON THE VIEW", 5);   // 50 characters
    audio.beep = 4;
}

// the chart on the decoder: its object (H-05: the age's way) with the stars, the figures' lines and the people's own marks on it, the
// figures' names as the computer reads them at their brightest star, the mark's amber diamond; the caption beside it word by word as a
// text's, and the star marked named as the explorer knows it (or UNKNOWN) with its sector and its distance from here; on a first
// reading at this share the stars appear first, then the lines, then the names, over CHART_READ_S seconds
void Game::renderChart() {
    const std::string& key = shardWorld.key;
    int n = (int)shardHeld.size(), idx = shardHeld[shardSel], pct = (int)std::lround(100 * languageShare(n, shardWorld.lore.records));
    const Shard& s = shardWorld.shards[idx]; const StarChart& C = shardWorld.chart;
    std::string head = fmt("%s - SHARD %d OF %d", shardWorldTitle(30).c_str(), shardSel + 1, n);
    drawText(canvas, 6, 4, head.c_str(), HUD_AMBER);
    if (shardLent(idx)) drawText(canvas, 6 + textWidth(head.c_str()) + 2 * FONT_ADV, 4, "LENT", HUD_CYAN);   // C-14: a friend's, credited
    drawText(canvas, 6, 13, fmt("YEAR %d OF %s", s.year, upper(shardWorld.lore.city).c_str()).c_str(), HUD_DIM);
    {   // H-05: the way it was made
        const std::string way = C.valid ? chartStyleLine(C.style, C.medium) : std::string();
        std::string line = fmt("A STAR CHART OF THE %s", upper(shardWorld.lore.people).c_str()) + (way.empty() ? "" : ", " + way);
        if (line.size() > 52) line = fmt("THE %s, ", trunc(upper(shardWorld.lore.people), 12).c_str()) + way;
        drawText(canvas, 6, 22, trunc(line, 52).c_str(), HUD_DIM);
    }
    bool reading = chartT >= 0;
    double u = reading ? clampd(chartT / CHART_READ_S, 0, 1) : 1;   // the stars by 0.4, the lines by 0.75, the names after
    Rng flick((uint64_t)(realTime * 20));
    bool held = chartUp && chartHeldKey == key && chartHeldIndex == shardWorld.base + idx;
    Painter P{canvas};
    if (!C.valid) drawTextCentered(canvas, CX, CY, "NO CHART", HUD_DIM);
    else {
        const uint64_t bkey = mix64(C.seed ^ (uint64_t)C.style * 0x9E37ULL ^ (uint64_t)C.medium);
        if (g_back.key != bkey) buildBackdrop(C, bkey);
        drawBackdrop(P);
        const uint64_t hs = mix64(C.seed ^ 0xD4A3ULL);
        const int total = (int)C.stars.size(), shown = u >= 0.4 ? total : (int)(total * u / 0.4);
        int lines = 0; for (const ChartFigure& f : C.figures) lines += (int)f.lines.size();
        const int shownL = u >= 0.75 ? lines : (u > 0.4 ? (int)(lines * (u - 0.4) / 0.35) : 0);
        const bool light = C.style == CHS_PAGE || C.style == CHS_ATLAS || C.style == CHS_PLATE || (C.style == CHS_SURVEY && C.medium != RM_DRIVE) || C.style == CHS_BONE || C.style == CHS_ASTROLABE;
        // ---- what the object carries besides its stars: the grid and the circles, the wanderers, the moon's count, the rings ----
        if (C.style == CHS_ASTROLABE || C.style == CHS_PAGE || C.style == CHS_ATLAS) {
            for (const ChartCurve& cv : C.curves) {
                uint32_t col = C.style == CHS_ATLAS ? col3(150, 144, 132) : (C.style == CHS_PAGE ? col3(170, 60, 44) : col3(92, 64, 26));
                int on = 0, off = 0;
                if (C.style == CHS_ATLAS && cv.kind == CC_HOUR) { on = 2; off = 1; }
                if (C.style == CHS_ASTROLABE && cv.kind == CC_TROPIC) { on = 3; off = 2; }
                for (size_t k = 1; k < cv.pts.size(); k++) {
                    P.line(CX + cv.pts[k - 1][0] * CR, CY - cv.pts[k - 1][1] * CR, CX + cv.pts[k][0] * CR, CY - cv.pts[k][1] * CR, col, 255, on, off);
                    if (cv.kind == CC_ECLIPTIC && C.style != CHS_ATLAS)   // the road of the sun as a band
                        P.line(CX + cv.pts[k - 1][0] * CR, CY - cv.pts[k - 1][1] * CR + 2, CX + cv.pts[k][0] * CR, CY - cv.pts[k][1] * CR + 2, col, 150);
                }
                if (C.style == CHS_ATLAS && cv.kind == CC_DECLINATION && cv.pts.size() > 8 && std::fabs(cv.value) > 1e-9) {   // a circle's number where it leaves the field
                    const auto& e = cv.pts.back(); const std::string v = fmt("%+d", (int)std::lround(cv.value));
                    const int x = (int)(CX + e[0] * CR), y = (int)(CY - e[1] * CR);
                    if (x > BX0 + 8 && x < BX1 - 20 && y > BY0 + 6 && y < BY1 - 10) drawText(canvas, x + 2, y - 3, v.c_str(), col3(110, 104, 96));
                }
            }
            if (C.style == CHS_ATLAS) {   // the plate's number in its corner
                const std::string t = fmt("TAB. %d", C.plate);
                drawText(canvas, BX1 - 10 - textWidth(t.c_str()), BY0 + 8, t.c_str(), col3(40, 36, 32));
            }
        }
        if (C.style == CHS_ASTROLABE || C.style == CHS_PAGE) {
            for (const ChartPath& w : C.wanderers) {   // the wanderers' ways: dotted, the wanderer at the way's middle
                const uint32_t col = C.style == CHS_PAGE ? col3(186, 54, 40) : col3(70, 48, 18);
                for (size_t k = 1; k < w.pts.size(); k++) P.line(CX + w.pts[k - 1][0] * CR, CY - w.pts[k - 1][1] * CR, CX + w.pts[k][0] * CR, CY - w.pts[k][1] * CR, col, 255, 1, 1);
                const auto& m = w.pts[w.pts.size() / 2];
                P.ring(CX + m[0] * CR, CY - m[1] * CR, 1.5 + 0.5 * w.size, 1.5 + 0.5 * w.size, col);
            }
        }
        if (C.style == CHS_CEILING || C.style == CHS_BONE) {   // the animals: painted large under the stars on the ceiling, scratched small on the bone
            for (const ChartFigure& f : C.figures) {
                if (f.glyph < 0) continue;
                double mx = 0, my = 0; int k = 0;
                for (int si : f.stars) { double x, y; if (starAt(C, C.stars[si], x, y)) { mx += x; my += y; k++; } }
                if (!k) continue;
                mx /= k; my /= k;
                if (C.style == CHS_CEILING) P.glyph(f.glyph, (int)mx - CHART_GLYPH_W, (int)my - CHART_GLYPH_H, 2, (f.glyph % 2) ? col3(40, 28, 22) : col3(164, 60, 36), 215);
                else P.glyph(f.glyph, (int)mx - 3, (int)my + 5, 1, col3(104, 82, 58), 230);
            }
            if (C.style == CHS_BONE && C.tally == 0)   // notches worn past counting
                for (int i = 0; i < 26; i++) {
                    if (hashU(i, 3, hs) < 0.45) continue;
                    BoneShape B; const double t = 0.14 + 0.72 * (i + 0.5) / 26;
                    const double x = B.ax + (B.bx - B.ax) * t, y = B.ay + (B.by - B.ay) * t + 17;
                    P.line(x, y, x + 1, y + 2, col3(150, 128, 100));
                }
            if (C.tally > 0) {   // the moon's count: a row of soot strokes on the roof, notches cut on the bone's lower edge; the full moon marked
                const int n = std::min(C.tally, 60), per = n > 30 ? (n + 1) / 2 : n;
                for (int i = 0; i < n; i++) {
                    const int row = i / per, k = i % per;
                    if (C.style == CHS_CEILING) {
                        const int x = CX - per * 2 + k * 4, y = CY + 50 + row * 8;
                        if (C.tallyKind == 2) P.ring(x + 0.5, y + 1.5, 1.2, 1.2, col3(30, 24, 20));
                        else { P.px(x, y, col3(30, 24, 20)); P.px(x, y + 1, col3(30, 24, 20)); P.px(x, y + 2, col3(30, 24, 20)); P.px(x + (hashU(i, 0, hs) < 0.5 ? 1 : 0), y + 3, col3(30, 24, 20)); }
                        if (C.tallyKind == 1 && i == n / 2) P.disc(x + 0.5, y - 4, 1.6, col3(222, 214, 196));
                    } else {
                        BoneShape B;
                        const double t = 0.12 + 0.76 * (k + 0.5) / per;
                        const double x = B.ax + (B.bx - B.ax) * t, y = B.ay + (B.by - B.ay) * t + 17 - row * 34;
                        P.line(x, y - 1, x + 1, y + 2, col3(70, 52, 36));
                    }
                }
            }
            {   // the people's own mark on the marked star: a ring of soot dots on the roof (and a hand blown in red beside it), a cut ring on the bone
                const ChartStar& m = C.stars[C.markStar]; double x, y;
                if (starAt(C, m, x, y)) {
                    if (C.style == CHS_CEILING) {
                        for (int i = 0; i < 10; i++) P.px((int)std::lround(x + 5 * std::cos(TAU * i / 10)), (int)std::lround(y + 5 * std::sin(TAU * i / 10)), col3(30, 22, 18));
                        const int hx = (int)x + (x < CX ? 9 : -18), hy = (int)y - 6;   // the hand: a stencil, red blown round a palm and five fingers
                        for (int yy = -3; yy <= 13; yy++) for (int xx = -3; xx <= 11; xx++) {
                            const bool palm = xx >= 1 && xx <= 7 && yy >= 5 && yy <= 11;
                            const bool finger = yy >= 0 && yy < 5 && (xx == 1 || xx == 3 || xx == 5 || xx == 7) && !(xx == 1 && yy < 2) && !(xx == 7 && yy < 1);
                            const bool thumb = xx == 9 && yy >= 6 && yy <= 8;
                            if (palm || finger || thumb) continue;
                            const double d = std::min(std::abs(xx - 4), 6) + std::min(std::abs(yy - 6), 7);
                            if (d < 9 && hashU(hx + xx, hy + yy, hs) < 0.75 - d * 0.06) P.mix(hx + xx, hy + yy, col3(150, 50, 30), 190);
                        }
                    } else P.ring(x, y, 3.5, 3.5, col3(92, 72, 50));
                }
            }
        }
        if (C.style == CHS_ROUTES) {   // the plane's rings at two, five and ten light years and the map's edge, the spokes, their star in the middle
            const uint32_t ringCol = col3(28, 84, 98);
            for (double ly : {2.0, 5.0, 10.0}) P.ring(CX, CY, CR * ly / CHART_MAP_LY, CR * MAP_SQ * ly / CHART_MAP_LY, ringCol, 255, ly < 10 ? 2 : 0);
            P.ring(CX, CY, CR, CR * MAP_SQ, col3(40, 110, 126));
            for (int k = 0; k < 12; k++) P.line(CX, CY, CX + CR * std::cos(TAU * k / 12), CY + CR * MAP_SQ * std::sin(TAU * k / 12), col3(16, 48, 58), 255, 1, 2);
            drawText(canvas, CX + (int)(CR * 0.7 * 10 / CHART_MAP_LY) - 2, CY - (int)(CR * 0.7 * 10 / CHART_MAP_LY) - 9, "10 LY", col3(40, 110, 126));
            P.ring(CX, CY, 3, 1.5, col3(120, 230, 240));
            P.px(CX, CY, col3(255, 240, 200));
        }
        if (C.style == CHS_BEYOND) {   // their rings round the far mark, a little out of true
            for (int k = 1; k <= 4; k++) {
                const double r = CR * (0.2 + 0.24 * (k - 1)) * (0.97 + 0.06 * hashU(k, 1, hs));
                P.ring(CX + hashU(k, 2, hs) * 2 - 1, CY + hashU(k, 3, hs) * 2 - 1, r, r * (0.92 + 0.08 * hashU(k, 4, hs)), col3(64, 40, 104), 255, k % 2 ? 0 : 3);
            }
        }
        // ---- the figures' lines (a navigation chart's routes, the far future's lines to nothing) ----
        int drawnL = 0;
        for (const ChartFigure& f : C.figures) for (size_t li = 0; li < f.lines.size(); li++) {
            const auto& ln = f.lines[li];
            if (drawnL++ >= shownL) break;
            double x0, y0, x1, y1;
            const bool a = starAt(C, C.stars[ln.first], x0, y0), b = starAt(C, C.stars[ln.second], x1, y1);
            switch (C.style) {
                case CHS_CEILING: if (a && b) { P.line(x0, y0, x1, y1, col3(176, 74, 42), 220); P.line(x0, y0 + 1, x1, y1 + 1, col3(150, 60, 36), 110); } break;
                case CHS_BONE: if (a && b) P.line(x0, y0, x1, y1, col3(118, 94, 66)); break;
                case CHS_VAULT: P.line(x0, y0, x1, y1, col3(196, 160, 84), 150); break;
                case CHS_ASTROLABE: P.line(x0, y0, x1, y1, col3(104, 74, 30), 170); break;
                case CHS_PAGE: P.line(x0, y0, x1, y1, col3(92, 64, 40)); break;
                case CHS_ATLAS: P.line(x0, y0, x1, y1, col3(40, 36, 32), 255, 1, 1); break;
                case CHS_PLATE: P.line(x0, y0, x1, y1, col3(60, 130, 90), 140); break;   // the computer's reading over the photograph
                case CHS_SURVEY: P.line(x0, y0, x1, y1, C.medium == RM_DRIVE ? col3(40, 100, 110) : col3(120, 120, 120), 255, 1, 2); break;
                case CHS_ROUTES: {
                    if (!b) {   // a far end: the line to the map's edge and an arrow
                        const double dx = x1 - x0, dy = y1 - y0; double t = 1;
                        for (int it = 0; it < 30; it++) { const double ex = (x0 + dx * t - CX) / CR, ey = (y0 + dy * t - CY) / (CR * MAP_SQ); if (ex * ex + ey * ey <= 1) break; t *= 0.93; }
                        x1 = x0 + dx * t; y1 = y0 + dy * t;
                    }
                    const uint32_t col = f.planned ? col3(90, 170, 190) : col3(140, 245, 250);
                    P.line(x0, y0, x1, y1, col, 255, f.planned ? 3 : 0, f.planned ? 2 : 0);
                    P.line(x0 + 1, y0, x1 + 1, y1, scaled(col, 0.7), 255, f.planned ? 3 : 0, f.planned ? 2 : 0);
                    if (li + 1 == f.lines.size() && b) P.ring(x1, y1, 3, 3, col);
                    if (li + 1 == f.lines.size()) {   // the arrowhead at its end
                        const double L = std::max(1e-6, std::hypot(x1 - x0, y1 - y0)), ux = (x1 - x0) / L, uy = (y1 - y0) / L;
                        P.line(x1, y1, x1 - 4 * ux + 2.5 * uy, y1 - 4 * uy - 2.5 * ux, col); P.line(x1, y1, x1 - 4 * ux - 2.5 * uy, y1 - 4 * uy + 2.5 * ux, col);
                    }
                    break;
                }
                default: P.line(x0, y0, x1, y1, col3(150, 110, 210), 200, 1, 1); break;   // the far future's
            }
        }
        // ---- the stars, brightest first (a reading brightens the sky from its brightest down) ----
        for (int k = 0; k < shown; k++) {
            const ChartStar& cs = C.stars[k]; double x, y;
            const bool on = starAt(C, cs, x, y);
            const double b = cs.bright;
            const int ix = (int)std::lround(x), iy = (int)std::lround(y);
            const double hr = hashU(k, 7, hs);
            switch (C.style) {
                case CHS_CEILING:
                    if (!on) break;
                    if (b > 2) { P.disc(x, y, 1.6, col3(184, 64, 38)); P.px(ix + (hr < 0.5 ? 2 : -2), iy, col3(170, 60, 36)); }
                    else if (b > 0.6) { P.px(ix, iy, col3(226, 218, 200)); P.px(ix + 1, iy, col3(206, 198, 180)); if (hr < 0.5) P.px(ix, iy + 1, col3(206, 198, 180)); }
                    else P.px(ix, iy, col3(32, 26, 22));
                    break;
                case CHS_BONE:
                    if (!on) break;
                    if (b > 2) { P.line(ix - 1, iy - 1, ix + 1, iy + 1, col3(76, 58, 40)); P.line(ix - 1, iy + 1, ix + 1, iy - 1, col3(76, 58, 40)); }
                    else P.px(ix, iy, col3(96, 76, 54));
                    break;
                case CHS_VAULT: {
                    const uint32_t gold = cs.mark ? col3(226, 230, 238) : col3(232, 192, 92), dim = scaled(gold, 0.7);
                    if (b > 2) { P.line(ix - 3, iy, ix + 3, iy, gold); P.line(ix, iy - 3, ix, iy + 3, gold); P.px(ix - 1, iy - 1, dim); P.px(ix + 1, iy + 1, dim); P.px(ix - 1, iy + 1, dim); P.px(ix + 1, iy - 1, dim); }
                    else if (b > 0.6) { P.line(ix - 1, iy, ix + 1, iy, gold); P.line(ix, iy - 1, ix, iy + 1, gold); }
                    else P.px(ix, iy, dim);
                    break;
                }
                case CHS_ASTROLABE:
                    if (k < 16 || cs.mark) {   // the rete's pointer: a little flame, its tip on the star, pointing outward from the middle
                        const double dx = x - CX, dy = y - CY, L = std::max(1.0, std::hypot(dx, dy)), ux = dx / L, uy = dy / L;
                        for (int t = 1; t <= 5; t++) { const double w = (t - 1) * 0.45; P.line(x + ux * t - uy * w, y + uy * t + ux * w, x + ux * t + uy * w, y + uy * t - ux * w, col3(72, 48, 16)); }
                        P.px(ix, iy, col3(232, 200, 120));
                    } else P.px(ix, iy, col3(104, 74, 30));
                    break;
                case CHS_PAGE:
                    if (b > 2) { P.line(ix - 2, iy, ix + 2, iy, col3(52, 34, 22)); P.line(ix, iy - 2, ix, iy + 2, col3(52, 34, 22)); }
                    else if (b > 0.6) { P.px(ix, iy, col3(52, 34, 22)); P.px(ix + 1, iy, col3(70, 48, 30)); }
                    else P.px(ix, iy, col3(96, 70, 46));
                    break;
                case CHS_ATLAS:
                    if (b > 2) P.disc(x, y, 1.5, col3(24, 22, 20)); else if (b > 0.6) { P.px(ix, iy, col3(24, 22, 20)); P.px(ix + 1, iy, col3(24, 22, 20)); P.px(ix, iy + 1, col3(24, 22, 20)); P.px(ix + 1, iy + 1, col3(24, 22, 20)); }
                    else P.px(ix, iy, col3(60, 56, 50));
                    if (cs.number > 0 && cs.number <= 12) { const std::string t = std::to_string(cs.number); drawText(canvas, ix + 2, iy + 2, t.c_str(), col3(70, 64, 56)); }
                    break;
                case CHS_PLATE: {   // a negative: a star a dark blur, the bright ones larger
                    const double r = b > 2 ? 2.3 : (b > 0.6 ? 1.5 : (b > 0.25 ? 0.9 : 0.5));
                    P.disc(x, y, r + 0.8, col3(120, 126, 114), 160); P.disc(x, y, r, col3(26, 28, 26));
                    break;
                }
                case CHS_SURVEY: {
                    const bool scr = C.medium == RM_DRIVE;
                    const double r = cs.mag <= 1 ? 2.0 : (cs.mag <= 2 ? 1.4 : (cs.mag <= 3.5 ? 0.9 : 0.5));
                    const uint32_t col = scr ? mixed(classCol(cs.star.cls), col3(255, 255, 255), 0.3) : col3(22, 22, 24);
                    P.disc(x, y, r, col);
                    if (cs.heard) {   // a people heard: ringed, the radio's arcs beside it
                        const uint32_t hc = scr ? col3(110, 210, 230) : col3(196, 40, 36);
                        P.ring(x, y, 5, 5, hc);
                        for (int a = 0; a < 2; a++) for (int i = -3; i <= 3; i++) { const double an = i * 0.22; P.px((int)std::lround(x + (8 + 3 * a) * std::cos(an - 0.8)), (int)std::lround(y + (8 + 3 * a) * std::sin(an - 0.8)), hc); }
                    }
                    break;
                }
                case CHS_ROUTES: {
                    double bx, by; onMap(cs, x, y, bx, by);
                    const double ex = (bx - CX) / CR, ey = (by - CY) / (CR * MAP_SQ);
                    if (ex * ex + ey * ey > 1.0001) break;
                    if (cs.home) break;
                    {   // a star and its height over their plane, a tick up or down (the faint ones a point alone)
                        bool dest = false; for (const ChartFigure& f : C.figures) if (f.stars.back() == k || (f.stars.size() > 2 && f.stars[1] == k)) dest = true;
                        const int tick = (int)std::lround(std::max(-4.0, std::min(4.0, cs.rise * 12)));
                        if ((b > 0.8 || dest || cs.mark) && tick) P.line(x, y, x, y - tick, col3(40, 96, 112));
                        P.disc(x, y, b > 2 || cs.mark || dest ? 1.2 : 0.5, b > 0.3 || dest ? mixed(classCol(cs.star.cls), col3(255, 255, 255), 0.35) : col3(60, 80, 90));
                    }
                    break;
                }
                default:   // the far future: their sky faint, the places where nothing is as rings with a sign of their own
                    if (cs.sign) { P.ring(x, y, 5, 5, col3(196, 170, 240)); farSign(P, mix64(cs.star.seed), ix - 2, iy - 2, col3(220, 200, 255)); }
                    else if (!cs.mark) P.px(ix, iy, b > 1 ? col3(96, 90, 110) : col3(54, 50, 66));
                    else { P.disc(x, y, 2, col3(255, 250, 230)); P.ring(x, y, 7, 7, col3(140, 110, 200), 200); }
                    break;
            }
        }
        if (C.style == CHS_PLATE && u >= 0.4) {   // the plate's ink: the mark ringed by hand, its number on the label
            double x, y; starAt(C, C.stars[C.markStar], x, y);
            for (int i = 0; i < 24; i++) { const double a = TAU * i / 24, r = 6 + 0.8 * std::sin(a * 3 + hs % 7); P.px((int)std::lround(x + r * std::cos(a)), (int)std::lround(y + r * std::sin(a)), col3(30, 40, 96)); }
            fillRectRGB(canvas, 16, 160, 62, 169, col3(226, 218, 190));
            const std::string t = fmt("N %d", C.plate); drawText(canvas, 19, 161, t.c_str(), col3(30, 40, 96));
        }
        if (C.style == CHS_SURVEY) {   // the field's number and the magnitudes' key
            const bool scr = C.medium == RM_DRIVE;
            const uint32_t ink = scr ? col3(110, 210, 230) : col3(30, 30, 30);
            const std::string t = fmt("FIELD %d", C.plate);
            drawText(canvas, BX1 - 12 - textWidth(t.c_str()), BY0 + 8, t.c_str(), ink);
            const int kx = BX0 + 10, ky = BY1 - 14;
            for (int m = 1; m <= std::min(5, std::max(1, C.magLimit)); m++) {
                const double r = m <= 1 ? 2.0 : (m <= 2 ? 1.4 : (m <= 3 ? 0.9 : 0.5));
                P.disc(kx + (m - 1) * 12, ky, r, scr ? col3(220, 230, 240) : col3(22, 22, 24));
                drawText(canvas, kx + (m - 1) * 12 - 2, ky + 4, std::to_string(m).c_str(), ink);
            }
        }
        if (C.style == CHS_PAGE) {   // the initial: a red square with the people's first letter in gold
            fillRectRGB(canvas, 14, 37, 25, 48, col3(150, 44, 36));
            const std::string L1 = upper(shardWorld.lore.people.substr(0, 1));
            drawText(canvas, 17, 39, L1.c_str(), col3(236, 196, 96));
        }
        if (C.style == CHS_ASTROLABE) {   // the rule across the face
            const double a = (hs % 360) * DEG;
            for (int t = -CR + 4; t <= CR - 4; t++) for (int w = -1; w <= 1; w++) P.mix((int)std::lround(CX + t * std::cos(a) - w * std::sin(a)), (int)std::lround(CY + t * std::sin(a) + w * std::cos(a)), col3(122, 90, 38), w ? 120 : 190);
            P.disc(CX, CY, 2.2, col3(84, 60, 24));
        }
        if (u >= 0.4 && C.markStar >= 0) {   // the computer's mark
            double x, y;
            if (starAt(C, C.stars[C.markStar], x, y)) {
                const int px = (int)std::lround(x), py = (int)std::lround(y);
                P.line(px - 5, py, px, py - 5, HUD_AMBER); P.line(px, py - 5, px + 5, py, HUD_AMBER);
                P.line(px + 5, py, px, py + 5, HUD_AMBER); P.line(px, py + 5, px - 5, py, HUD_AMBER);
            } else if (C.style == CHS_ROUTES) {   // past the map's edge: the diamond on the edge where its route leaves
                double sx, sy, bx, by; onMap(C.stars[C.markStar], sx, sy, bx, by);
                const double dx = sx - CX, dy = sy - CY; double t = 1;
                for (int it = 0; it < 40; it++) { const double ex = dx * t / CR, ey = dy * t / (CR * MAP_SQ); if (ex * ex + ey * ey <= 1) break; t *= 0.95; }
                const int px = (int)std::lround(CX + dx * t), py = (int)std::lround(CY + dy * t);
                P.line(px - 4, py, px, py - 4, HUD_AMBER); P.line(px, py - 4, px + 4, py, HUD_AMBER);
                P.line(px + 4, py, px, py + 4, HUD_AMBER); P.line(px, py + 4, px - 4, py, HUD_AMBER);
                const std::string t2 = fmt("%.0f LY", C.stars[C.markStar].ly);
                drawText(canvas, std::max(BX0 + 2, std::min(BX1 - 2 - textWidth(t2.c_str()), px - textWidth(t2.c_str()) / 2)), py + (py > CY ? -12 : 6), t2.c_str(), HUD_AMBER);
            }
        }
        // ---- the names the computer reads, beside each figure's label star: right of it, left, above or below, wherever the name keeps clear of
        // the others and of the mark, and none where it cannot (the caption names them all); outlined over a pale or busy object ----
        std::vector<std::array<int, 4>> placed;
        if (C.markStar >= 0) { double x, y; if (starAt(C, C.stars[C.markStar], x, y)) placed.push_back({(int)x - 6, (int)y - 6, (int)x + 6, (int)y + 6}); }
        switch (C.style) {   // what the object writes itself keeps its room
            case CHS_PAGE: placed.push_back({12, 35, 27, 50}); break;
            case CHS_ATLAS: placed.push_back({BX1 - 60, BY0 + 6, BX1 - 6, BY0 + 16}); break;
            case CHS_PLATE: placed.push_back({14, 158, 64, 171}); break;
            case CHS_SURVEY: placed.push_back({BX1 - 70, BY0 + 6, BX1 - 6, BY0 + 16}); placed.push_back({BX0 + 6, BY1 - 18, BX0 + 64, BY1 - 2}); break;
            case CHS_ROUTES: placed.push_back({CX + (int)(CR * 0.7 * 10 / CHART_MAP_LY) - 4, CY - (int)(CR * 0.7 * 10 / CHART_MAP_LY) - 11, CX + (int)(CR * 0.7 * 10 / CHART_MAP_LY) + 30, CY - (int)(CR * 0.7 * 10 / CHART_MAP_LY) - 1}); break;
            default: break;
        }
        auto clear = [&](int x0, int y0, int x1, int y1) {
            if (x0 < BX0 + 1 || x1 > BX1 - 1 || y0 < BY0 + 1 || y1 > BY1 - 1) return false;
            for (const auto& b : placed) if (x0 < b[2] + 2 && x1 > b[0] - 2 && y0 < b[3] + 1 && y1 > b[1] - 1) return false;
            return true;
        };
        const bool outline = light || C.style == CHS_CEILING || C.style == CHS_VAULT;
        std::vector<int> order;   // the figure with the mark in it first, then by their brightest star; five names at most (the caption names them all)
        for (size_t i = 0; i < C.figures.size(); i++) order.push_back((int)i);
        auto hasMark = [&](int i) { for (int k : C.figures[i].stars) if (k == C.markStar) return true; return false; };
        std::stable_sort(order.begin(), order.end(), [&](int a, int b) { if (hasMark(a) != hasMark(b)) return hasMark(a); return C.stars[C.figures[a].label].bright > C.stars[C.figures[b].label].bright; });
        int labels = 0;
        if (u >= 0.75) for (int i : order) {
            if (labels >= 5) break;
            const ChartFigure& f = C.figures[i];
            std::string label = figureLabel(f.name, n);
            bool known = true; { Shard tmp; tmp.text = f.name; std::vector<DecodedWord> w; decodeShard(tmp, shardWorld.lang, shardWorld.tongue, n, w); for (const DecodedWord& d : w) if (!d.known) known = false; }
            if (reading && u < 1) for (char& c : label) if (c != ' ' && flick.chance(0.5)) c = (char)('A' + flick.irange(26));
            double x, y; const bool on = starAt(C, C.stars[f.label], x, y);
            if (!on && C.style != CHS_ROUTES) continue;
            if (!on) { double sx, sy, bx, by; onMap(C.stars[f.label], sx, sy, bx, by); const double dx = sx - CX, dy = sy - CY; double t = 1; for (int it = 0; it < 40; it++) { const double ex = dx * t / CR, ey = dy * t / (CR * MAP_SQ); if (ex * ex + ey * ey <= 1) break; t *= 0.95; } x = CX + dx * t; y = CY + dy * t; }
            const int tw = textWidth(label.c_str()), ix = (int)std::lround(x), iy = (int)std::lround(y);
            const int glyphW = (C.style == CHS_PAGE || C.style == CHS_VAULT) && f.glyph >= 0 ? 9 : 0;
            int tries[4][2] = {{ix + 4 + glyphW, iy - 3}, {ix - 4 - tw, iy - 3}, {ix - tw / 2, iy - 11}, {ix - tw / 2, iy + 5}};
            if (C.style == CHS_ROUTES && ix < CX) std::swap(tries[0], tries[1]);   // a route's name outward, away from the crowd at home
            int lx = -1, ly = -1;
            for (const auto& t : tries) if (clear(t[0] - glyphW, t[1] - 1, t[0] + tw, t[1] + 8)) { lx = t[0]; ly = t[1]; break; }
            if (lx < 0) continue;
            placed.push_back({lx - glyphW, ly - 1, lx + tw, ly + 8}); labels++;
            const uint32_t col = reading && u < 1 ? HUD_GREEN : (known ? HUD_WHITE : HUD_DIM);
            if (outline) for (int oy = -1; oy <= 1; oy++) for (int ox = -1; ox <= 1; ox++) if (ox || oy) drawText(canvas, lx + ox, ly + oy, label.c_str(), rgb(0, 0, 0));
            if (outline) drawText(canvas, lx, ly, label.c_str(), col); else drawTextShadow(canvas, lx, ly, label.c_str(), col, HUD_SHADOW);
            if (glyphW) P.glyph(f.glyph, lx - glyphW, ly, 1, C.style == CHS_PAGE ? col3(176, 52, 40) : col3(226, 214, 180));   // the creature beside its name
        }
    }
    // the caption, laid out at the right as a text is (a first reading reveals it with the stars), down to the mark's block at the column's
    // foot; a caption longer than the room ends in "..."
    const bool remoteHere = C.valid && ship.hasRemote && ship.remote.seed == C.mark.star.seed;
    const int blockY = C.valid && u >= 0.4 ? (remoteHere || held ? 131 : 140) : 168;
    {
        int total = (int)shardWords.size(), cursor = reading ? std::min(total, (int)(total * u / 0.75)) : total;
        struct At { int x, y; };
        std::vector<At> at(total);
        int x = 170, y = 33, last = -1; const int x0 = 170, maxX = UW - 8, maxY = blockY - 13;
        for (int i = 0; i < total; i++) {
            const DecodedWord& w = shardWords[i];
            const std::string shown = upper(i < cursor ? w.pre + (w.known ? w.ours : w.theirs) + w.post : w.pre + w.theirs + w.post);
            const int tw = textWidth(shown.c_str());
            if (x > x0 && x + tw > maxX) { x = x0; y += 9; }
            if (y > maxY) break;
            at[i] = {x, y}; last = i; x += tw + FONT_ADV;
        }
        const bool cut = last + 1 < total;
        if (cut) while (last >= 0 && at[last].x + textWidth(upper(shardWords[last].pre + (last < cursor ? (shardWords[last].known ? shardWords[last].ours : shardWords[last].theirs) : shardWords[last].theirs) + shardWords[last].post).c_str()) + 3 * FONT_ADV > maxX) last--;
        for (int i = 0; i <= last; i++) {
            const DecodedWord& w = shardWords[i];
            std::string shown; uint32_t col;
            if (i < cursor) { shown = w.pre + (w.known ? w.ours : w.theirs) + w.post; col = w.name ? HUD_AMBER : (w.known ? HUD_WHITE : HUD_DIM); }
            else if (i == cursor) { shown = w.theirs; for (char& c : shown) c = (char)('A' + flick.irange(26)); col = HUD_GREEN; }
            else { shown = w.pre + w.theirs + w.post; col = HUD_DIM; }
            shown = upper(shown);
            drawText(canvas, at[i].x, at[i].y, shown.c_str(), col);
            if (cut && i == last) drawText(canvas, at[i].x + textWidth(shown.c_str()), at[i].y, "...", HUD_DIM);
        }
    }
    if (C.valid && u >= 0.4) {   // the star they marked, as the explorer knows it
        const ChartMark& m = C.mark;
        Star full = m.star;
        double lyHere = length(m.star.pos - ship.pos) / SECTOR_KM;
        const int yy = blockY;   // the column is 23 characters wide: four short lines
        drawText(canvas, 170, yy, "THEY MARKED", HUD_AMBER);
        drawText(canvas, 170, yy + 9, fmt("%s (%s)", trunc(upper(starNameOf(full)), 16).c_str(), STAR_CLASSES[m.star.cls].code).c_str(), HUD_WHITE);
        drawText(canvas, 170, yy + 18, (lyHere >= 1000 ? thousands(lyHere) + " LY FROM HERE" : fmt("%.1f LY FROM HERE", lyHere)).c_str(), HUD_WHITE);   // H-05: the far future's far
        drawText(canvas, 170, yy + 27, fmt("SECTOR %lld %lld %lld", (long long)m.star.sx, (long long)m.star.sy, (long long)m.star.sz).c_str(), HUD_DIM);
        if (remoteHere || held) drawText(canvas, 170, yy + 36, remoteHere ? (held ? "THE REMOTE TARGET, HELD UP" : "THE REMOTE TARGET") : "HELD UP TO THE SKY", remoteHere ? HUD_CYAN : HUD_GREEN);
    }
    int read = 0; for (const DecodedWord& w : shardWords) if (w.known) read++;
    if (reading) {
        drawText(canvas, 12, UH - 22, fmt("DECODING %3d%%", (int)(100 * u)).c_str(), HUD_AMBER);
        const int bx0 = 100, bx1 = UW - 12;
        drawRectRGB(canvas, bx0, UH - 22, bx1, UH - 16, HUD_DIM);
        if (u > 0) fillRectRGB(canvas, bx0 + 1, UH - 21, bx0 + 1 + (int)((bx1 - bx0 - 2) * u), UH - 17, HUD_GREEN);
        drawTextCentered(canvas, UW / 2, UH - 10, "ENTER SKIPS THE WAIT  ESC STOPS", HUD_DIM);
    } else {
        drawText(canvas, 12, UH - 22, fmt("%d OF %d WORDS READ   THE LANGUAGE %d%% (%d SHARD%s)", read, (int)shardWords.size(), pct, n, n == 1 ? "" : "S").c_str(), HUD_GREEN);
        drawTextCentered(canvas, UW / 2, UH - 10, held ? "O PUT DOWN  ENTER TARGET THE STAR  LEFT/RIGHT  ESC" : "O HOLD UP  ENTER TARGET THE STAR  LEFT/RIGHT  ESC", HUD_DIM);   // 50 and 49 characters
    }
}

// the chart held up: its figures' lines between the real stars as the camera sees them (`cam` world to view, `obs` the eye in
// km), the figures' names at their brightest star, the marked star's diamond (an edge mark when it is behind), a line naming
// the chart; on the surface (`localFrame` given) only above the horizon. H-05: a navigation chart's routes are lines between the real
// stars as any figure's; the far future's places where nothing is are rings where nothing shines
void Game::drawChartOverlay(const Mat3& cam, const Vec3& obs, const Proj& pj, const Mat3* localFrame) {
    chartLinesDrawn = 0;
    if (!chartUp || !chartHeld.valid) return;
    const StarChart& C = chartHeld;
    const double f = pj.f / FB_SCALE; const int cx = UW / 2, cy = UH / 2;
    size_t N = C.stars.size();
    std::vector<double> sx(N), sy(N); std::vector<char> ok(N, 0); std::vector<Vec3> vv(N);
    for (size_t k = 0; k < N; k++) {
        Vec3 d = C.stars[k].star.pos - obs;
        if (length2(d) < 1e-6) continue;   // H-05: the ship at a route's own star
        if (localFrame) { Vec3 l = (*localFrame) * d; if (l.y < 0.01 * length(l)) continue; }
        Vec3 v = cam * d; vv[k] = v;
        if (v.z <= 1e-9) continue;
        sx[k] = cx + f * v.x / v.z; sy[k] = cy - f * v.y / v.z;
        ok[k] = std::fabs(sx[k]) < 4000 && std::fabs(sy[k]) < 4000;
    }
    auto clip = [&](double& x0, double& y0, double& x1, double& y1) {   // Liang-Barsky against the screen with a margin
        const double L = -2, Rr = UW + 1, T = -2, B = UH + 1;
        double t0 = 0, t1 = 1, dx = x1 - x0, dy = y1 - y0;
        auto edge = [&](double p, double q) { if (p == 0) return q >= 0; double r = q / p; if (p < 0) { if (r > t1) return false; if (r > t0) t0 = r; } else { if (r < t0) return false; if (r < t1) t1 = r; } return true; };
        if (!edge(-dx, x0 - L) || !edge(dx, Rr - x0) || !edge(-dy, y0 - T) || !edge(dy, B - y0)) return false;
        double nx0 = x0 + t0 * dx, ny0 = y0 + t0 * dy, nx1 = x0 + t1 * dx, ny1 = y0 + t1 * dy;
        x0 = nx0; y0 = ny0; x1 = nx1; y1 = ny1;
        return true;
    };
    const uint32_t lineCol = C.style == CHS_ROUTES ? rgb(60, 140, 150) : (C.style == CHS_BEYOND ? rgb(110, 80, 150) : rgb(50, 120, 80));
    const int S = FB_SCALE;
    std::vector<std::array<int, 4>> placed;   // the labels' boxes: a name moves down or up rather than over another (from afar a people's whole sky is a knot of a few degrees)
    auto labelY = [&](int lx, int ly, int tw) {
        static const int tries[5] = {0, 9, -9, 18, -18};
        for (int dy : tries) {
            int y0 = ly + dy; bool clear = y0 >= 30 && y0 <= UH - 30;
            for (const auto& b : placed) if (clear && lx < b[2] + 2 && lx + tw > b[0] - 2 && y0 < b[3] + 1 && y0 + 7 > b[1] - 1) clear = false;
            if (clear) { placed.push_back({lx, y0, lx + tw, y0 + 7}); return y0; }
        }
        placed.push_back({lx, ly, lx + tw, ly + 7}); return ly;
    };
    auto skyDot = [&](int x, int y, uint32_t col) {   // a pixel on the sky alone: none where something solid is drawn (the cabin's walls, a globe, the ground)
        if (x < 0 || y < 0 || x >= UW || y >= UH) return;
        if (fb.invz[(size_t)(y * S) * FBW + (size_t)(x * S)] > 0) return;
        fillRectRGB(canvas, x, y, x, y, col);
    };
    auto skyLine = [&](int x0, int y0, int x1, int y1, int on, int off) {
        int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0), steps = std::max(dx, dy);
        for (int i = 0; i <= steps; i++) {
            if (on > 0 && i % (on + off) >= on) continue;
            int x = steps ? x0 + (x1 - x0) * i / steps : x0, y = steps ? y0 + (y1 - y0) * i / steps : y0;
            skyDot(x, y, lineCol);
        }
    };
    for (size_t i = 0; i < C.figures.size(); i++) {
        const ChartFigure& fg = C.figures[i];
        for (const auto& ln : fg.lines) {
            if (!ok[ln.first] || !ok[ln.second]) continue;
            double x0 = sx[ln.first], y0 = sy[ln.first], x1 = sx[ln.second], y1 = sy[ln.second];
            if (!clip(x0, y0, x1, y1)) continue;
            skyLine((int)std::lround(x0), (int)std::lround(y0), (int)std::lround(x1), (int)std::lround(y1), fg.planned || fg.kind == 2 ? 3 : 0, 2);
            chartLinesDrawn++;
        }
        int lb = fg.label;
        if (ok[lb] && sx[lb] > 2 && sy[lb] > 30 && sx[lb] < UW - 40 && sy[lb] < UH - 30 && i < chartHeldLabels.size()) { int tw = textWidth(chartHeldLabels[i].c_str()), lx = (int)sx[lb] + 4; drawText(canvas, lx, labelY(lx, (int)sy[lb] - 3, tw), chartHeldLabels[i].c_str(), HUD_DIM); }
    }
    for (size_t k = 0; k < N; k++) {   // H-05: the far future's places where nothing is
        if (!C.stars[k].sign || !ok[k]) continue;
        const int px = (int)std::lround(sx[k]), py = (int)std::lround(sy[k]);
        for (int i = 0; i < 16; i++) skyDot((int)std::lround(px + 4 * std::cos(TAU * i / 16)), (int)std::lround(py + 4 * std::sin(TAU * i / 16)), lineCol);
    }
    if (C.markStar >= 0) {
        int k = C.markStar;
        if (ok[k] && sx[k] > 6 && sy[k] > 6 && sx[k] < UW - 6 && sy[k] < UH - 6) {
            int px = (int)std::lround(sx[k]), py = (int)std::lround(sy[k]);
            drawLineRGB(canvas, px - 5, py, px, py - 5, HUD_AMBER); drawLineRGB(canvas, px, py - 5, px + 5, py, HUD_AMBER);
            drawLineRGB(canvas, px + 5, py, px, py + 5, HUD_AMBER); drawLineRGB(canvas, px, py + 5, px - 5, py, HUD_AMBER);
            if (py < UH - 30) drawTextCentered(canvas, px, py + 8, "THEY MARKED THIS STAR", HUD_AMBER);
        } else if (!localFrame || (*localFrame * (C.stars[k].star.pos - obs)).y > 0) {
            const Vec3& v = vv[k];
            double ang = std::atan2(-v.y, v.x);
            drawText(canvas, UW / 2 + (int)(std::cos(ang) * 140) - 2, UH / 2 + (int)(std::sin(ang) * 85) - 3, "+", HUD_AMBER);
        }
    }
    std::string line = fmt("A CHART OF THE %s HELD UP", upper(chartHeldPeople).c_str());
    drawTextShadow(canvas, UW - 8 - textWidth(line.c_str()), 32, line.c_str(), HUD_DIM, HUD_SHADOW);
}

std::string Game::testChartInfo() const {
    std::string s = fmt("chart: level %d", shardsLevel);
    if (shardsLevel == 2 && shardWorld.valid && !shardHeld.empty() && shardWorld.shards[shardHeld[shardSel]].chart) {
        const StarChart& C = shardWorld.chart;
        s += fmt(", on the screen S%d (chart %d of the %s, %s): %zu stars, %zu figures, the mark %s at %lld %lld %lld (how %d, %.0f ly, %s), %s", shardHeld[shardSel], C.which, shardWorld.lore.people.c_str(), CHART_STYLE_NAMES[std::max(0, std::min(CHS_COUNT - 1, C.style))], C.stars.size(), C.figures.size(),
                 STAR_CLASSES[C.mark.star.cls].code, (long long)C.mark.star.sx, (long long)C.mark.star.sy, (long long)C.mark.star.sz, C.mark.how, C.mark.ly, C.mark.people.empty() ? "no people" : ("the " + C.mark.people).c_str(),
                 chartT >= 0 ? fmt("reading %.0f%%", 100 * clampd(chartT / CHART_READ_S, 0, 1)).c_str() : (shardFresh ? "unread" : "read"));
    }
    s += chartUp ? fmt("; held up: S%d of %s (%zu stars, %zu figures), %d lines drawn", chartHeldIndex, chartHeldKey.c_str(), chartHeld.stars.size(), chartHeld.figures.size(), chartLinesDrawn) : "; none held up";
    return s;
}

bool Game::testAimAtChartMark(int figure) {
    if (!chartUp || !chartHeld.valid) return false;
    if (figure >= (int)chartHeld.figures.size()) return false;
    const Star& at = figure < 0 ? chartHeld.mark.star : chartHeld.stars[chartHeld.figures[figure].label].star;
    Vec3 fwd = normalize(at.pos - ship.pos);
    ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1));
    cabin.yaw = 0; cabin.pitch = 0;
    return true;
}
