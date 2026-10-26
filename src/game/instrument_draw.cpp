// C-11 (2026-10-10): an instrument of the old peoples drawn on the cabin's case. C-17 (2026-10-10): drawn from its parts (peoples/instruments.h)
// as it lies, seen from above: the same parts the ruins draw as its object. Each part in its material's colour (its people's own where it takes
// their banks: stone their structure's, glass their glass, paint their accent, light their light), lit from the upper left (a tube's light
// side and its shadow, a body's bevel and its dome's sheen, a ball's sheen, a bowl's dark inside), over its shadow on the case's dark cloth;
// what a key sounds glows as it is played and a string shivers; a ruler under it gives its size. Logical units of the 320 x 200 HUD.
#include "game.h"
#include "peoples/instruments.h"
#include <cmath>
#include <algorithm>

namespace {
uint32_t mixc(uint32_t a, uint32_t b, double t) {
    t = std::max(0.0, std::min(1.0, t));
    auto ch = [&](int s) { return (int)std::lround(((a >> s) & 255) * (1 - t) + ((b >> s) & 255) * t); };
    return rgb(ch(0), ch(8), ch(16));
}
uint32_t ofRGB(const RGB& c) { return rgb((int)std::lround(std::max(0.0f, std::min(1.0f, c.r)) * 255), (int)std::lround(std::max(0.0f, std::min(1.0f, c.g)) * 255), (int)std::lround(std::max(0.0f, std::min(1.0f, c.b)) * 255)); }
const uint32_t WHITE = rgb(255, 255, 255), BLACK = rgb(0, 0, 0), LIT = rgb(255, 236, 160);

// the picture's frame: the instrument's metres (u along, v across, up the picture) to the canvas's units, turned a quarter where it lies tall
struct Pic {
    RGBCanvas& c;
    double ox = 0, oy = 0, s = 1;
    bool turn = false;
    void at(double u, double v, double& x, double& y) const {
        if (turn) { x = ox + v * s; y = oy + u * s; } else { x = ox + u * s; y = oy - v * s; }
    }
    // a filled polygon of points in the canvas's units (scanlines at the logical pixels' middles)
    void fill(const double* xs, const double* ys, int n, uint32_t col) {
        if (n < 3) return;
        double ymin = 1e9, ymax = -1e9;
        for (int i = 0; i < n; i++) { ymin = std::min(ymin, ys[i]); ymax = std::max(ymax, ys[i]); }
        std::vector<double> cut;
        for (int y = (int)std::floor(ymin); y <= (int)std::ceil(ymax); y++) {
            const double yc = y + 0.5;
            cut.clear();
            for (int i = 0; i < n; i++) {
                const double ya = ys[i], yb = ys[(i + 1) % n];
                if ((yc >= ya && yc < yb) || (yc >= yb && yc < ya)) cut.push_back(xs[i] + (yc - ya) * (xs[(i + 1) % n] - xs[i]) / (yb - ya));
            }
            std::sort(cut.begin(), cut.end());
            for (size_t i = 0; i + 1 < cut.size(); i += 2) {
                const int xa = (int)std::lround(cut[i]), xb = (int)std::lround(cut[i + 1]) - 1;
                if (xb >= xa) fillRectRGB(c, xa, y, xb, y, col);
            }
        }
    }
    void polyUV(const double* u, const double* v, int n, uint32_t col, double dx = 0, double dy = 0) {
        double xs[96], ys[96]; n = std::min(n, 96);
        for (int i = 0; i < n; i++) { at(u[i], v[i], xs[i], ys[i]); xs[i] += dx; ys[i] += dy; }
        fill(xs, ys, n, col);
    }
    void lineUV(double u0, double v0, double u1, double v1, uint32_t col, double dx = 0, double dy = 0) {
        double x0, y0, x1, y1; at(u0, v0, x0, y0); at(u1, v1, x1, y1);
        drawLineRGB(c, (int)std::floor(x0 + dx), (int)std::floor(y0 + dy), (int)std::floor(x1 + dx), (int)std::floor(y1 + dy), col);
    }
    // the outline's edges: those facing the light lighter, the others darker (a bevel)
    void bevelUV(const double* u, const double* v, int n, uint32_t lightCol, uint32_t darkCol) {
        double xs[96], ys[96]; n = std::min(n, 96);
        double cx = 0, cy = 0;
        for (int i = 0; i < n; i++) { at(u[i], v[i], xs[i], ys[i]); cx += xs[i]; cy += ys[i]; }
        cx /= n; cy /= n;
        for (int i = 0; i < n; i++) {
            const int j = (i + 1) % n;
            const double mx = (xs[i] + xs[j]) * 0.5 - cx, my = (ys[i] + ys[j]) * 0.5 - cy;
            const bool lit = mx + my < 0;   // facing up and left
            drawLineRGB(c, (int)std::floor(xs[i]), (int)std::floor(ys[i]), (int)std::floor(xs[j]), (int)std::floor(ys[j]), lit ? lightCol : darkCol);
        }
    }
};

double glowOf(const std::vector<double>& glow, int i) { return i >= 0 && !glow.empty() ? glow[i % (int)glow.size()] : 0.0; }

// a part's outline in the instrument's metres (a tube's quad, a plate's own outline, a disc's or a ball's ellipse or polygon, a box, a mark)
int outlineOf(const InstrumentPart& p, double* u, double* v, double grow = 0) {
    const double c = std::cos(p.ang), s = std::sin(p.ang);
    auto put = [&](int i, double along, double across) { u[i] = p.u + c * along - s * across; v[i] = p.v + s * along + c * across; };
    switch (p.kind) {
        case IPK_TUBE: { const double r0 = p.r0 + grow, r1 = p.r1 + grow; put(0, 0, r0); put(1, p.len, r1); put(2, p.len, -r1); put(3, 0, -r0); return 4; }
        case IPK_PLATE: {
            double a[48], b[48]; const int n = 40; formPlateOutline(instrumentPlateForm(p), n, a, b);
            for (int i = 0; i < n; i++) { const double l = std::hypot(a[i], b[i]) + 1e-9; put(i, b[i] + b[i] / l * grow, a[i] + a[i] / l * grow); }
            return n;
        }
        case IPK_DISC: case IPK_BALL: {
            const double k = p.kind == IPK_DISC && p.open ? std::max(1.0, p.taper) : 1.0, ra = p.r0 * k + grow, rb = p.r1 * k + grow;
            const int sides = p.kind == IPK_DISC && p.sides >= 3 ? std::min(p.sides, 12) : 0, n = sides ? sides : 28;
            for (int i = 0; i < n; i++) {
                const double th = TAU * i / n + (sides ? PI / sides : 0);
                double r = 1;
                if (sides) r = 1 / std::cos(PI / sides);
                put(i, std::cos(th) * rb * r, std::sin(th) * ra * r);
            }
            return n;
        }
        case IPK_MARK: {
            const double r = p.r0 + grow, L = p.len * 0.5 + grow;
            switch (p.shape) {
                case 1: put(0, -L, r); put(1, L, r); put(2, L, -r); put(3, -L, -r); return 4;                       // a slit
                case 2: put(0, -r, r); put(1, r, r); put(2, r, -r); put(3, -r, -r); return 4;                       // square
                case 3: for (int i = 0; i < 3; i++) { const double a = PI / 2 + TAU * i / 3; put(i, std::sin(a) * r * 1.2, std::cos(a) * r * 1.2); } return 3;
                case 4: { for (int i = 0; i < 12; i++) { const double a = TAU * i / 12; put(i, std::cos(a) * r * 0.75 + r * 0.45, std::sin(a) * r * 0.75); } put(12, -r * 1.1, r * 0.45); put(13, -r * 1.1, -r * 0.45); return 14; }   // a keyhole
                case 5: { for (int i = 0; i <= 8; i++) { const double a = PI * i / 8; put(i, std::sin(a) * r * 0.6 + r * 0.35, std::cos(a) * r); } put(9, -r, -r); put(10, -r, r); return 11; }   // an arch
                case 6: for (int i = 0; i < 10; i++) { const double a = TAU * i / 10, rr = i % 2 ? r * 0.45 : r * 1.15; put(i, std::cos(a) * rr, std::sin(a) * rr); } return 10;   // a star
                default: for (int i = 0; i < 14; i++) { const double a = TAU * i / 14; put(i, std::cos(a) * r, std::sin(a) * r); } return 14;
            }
        }
        default: { const double L = p.len * 0.5 + grow, r = p.r0 + grow; put(0, -L, r); put(1, L, r); put(2, L, -r); put(3, -L, -r); return 4; }   // a box
    }
}
double topOf(const InstrumentPart& p) {
    switch (p.kind) {
        case IPK_TUBE: return p.w0 + std::max(p.r0, p.r1);
        case IPK_RING: return p.standing ? p.w0 + p.r0 + p.r1 : p.w0 + 2 * p.r1;
        case IPK_PLATE: return p.w1 + std::max(0.0, p.dome);
        case IPK_STRING: return std::max(p.w0, p.w1) + 0.0005;
        case IPK_MARK: return p.w0 + 0.0002;
        default: return p.w1;
    }
}
// the order it is drawn in: a mark by the surface it lies on, a domed body or a ball below its peak (what lies on its slope comes after)
double orderOf(const InstrumentPart& p) {
    switch (p.kind) {
        case IPK_PLATE: return p.w1 + std::max(0.0, p.dome) * 0.2;
        case IPK_BALL: return p.w0 + (p.w1 - p.w0) * 0.55;
        case IPK_DISC: return p.open ? p.w0 + (p.w1 - p.w0) * 0.3 : p.w1;
        default: return topOf(p);
    }
}
}   // namespace

void drawInstrumentParts(RGBCanvas& cv, int x0, int y0, int w, int h, const std::vector<InstrumentPart>& parts, const FormPalette& pal, const std::vector<double>& glow, double t, bool ruler) {
    // the case's cloth
    fillRectRGB(cv, x0, y0, x0 + w - 1, y0 + h - 1, rgb(42, 48, 47));   // a felt between the dark and the pale
    drawRectRGB(cv, x0, y0, x0 + w - 1, y0 + h - 1, rgb(66, 74, 72));
    if (parts.empty()) return;
    double u0, u1, v0, v1, top; instrumentBounds(parts, u0, u1, v0, v1, top);
    const double lu = std::max(1e-3, u1 - u0), lv = std::max(1e-3, v1 - v0);
    const int foot = ruler ? 10 : 0;
    const double bw = w - 10, bh = h - 8 - foot;
    Pic P{cv};
    P.turn = lv > lu * 1.25 && bw > bh;   // lying tall in a wide box: a quarter turn
    const double eu = P.turn ? lv : lu, ev = P.turn ? lu : lv;
    P.s = std::min(bw / eu, bh / ev);
    const double cx = x0 + w * 0.5, cy = y0 + 4 + bh * 0.5;
    if (P.turn) { P.ox = cx - (v0 + v1) * 0.5 * P.s; P.oy = cy - (u0 + u1) * 0.5 * P.s; }
    else { P.ox = cx - (u0 + u1) * 0.5 * P.s; P.oy = cy + (v0 + v1) * 0.5 * P.s; }
    // the order: lowest first
    std::vector<int> order(parts.size());
    for (size_t i = 0; i < parts.size(); i++) order[i] = (int)i;
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return orderOf(parts[a]) < orderOf(parts[b]); });
    double U[96], V[96];
    // its shadow on the cloth
    const uint32_t shadow = rgb(24, 28, 28);
    for (int i : order) {
        const InstrumentPart& p = parts[i];
        if (p.kind == IPK_MARK || p.kind == IPK_STRING || topOf(p) < 0.004) continue;
        const double off = std::min(3.0, 1.0 + topOf(p) * P.s * 0.08);
        if (p.kind == IPK_RING && !p.standing) { for (int k = 0; k < 32; k++) { const double a0 = TAU * k / 32, a1 = TAU * (k + 1) / 32; P.lineUV(p.u + std::cos(a0) * p.r0, p.v + std::sin(a0) * p.r0, p.u + std::cos(a1) * p.r0, p.v + std::sin(a1) * p.r0, shadow, off, off); } continue; }
        if (p.kind == IPK_RING) continue;
        const int n = outlineOf(p, U, V);
        P.polyUV(U, V, n, shadow, off, off);
    }
    // the parts
    for (int i : order) {
        const InstrumentPart& p = parts[i];
        const double g = glowOf(glow, p.course) * (p.course >= 0 ? 1.0 : 0.0);
        uint32_t base = ofRGB(instrumentMaterialColour(p.mat, pal));
        if (p.mat == IM_LIGHT) base = mixc(mixc(base, BLACK, 0.4), base, g * 1.5 + (p.course < 0 ? 0.6 : 0));
        if (g > 0.02) base = mixc(base, LIT, g * 0.75);
        const uint32_t hi = mixc(base, WHITE, 0.32), lo = mixc(base, BLACK, 0.42), edge = mixc(base, BLACK, 0.62);
        const double c = std::cos(p.ang), s = std::sin(p.ang);
        switch (p.kind) {
            case IPK_TUBE: {
                const int n = outlineOf(p, U, V);
                double xa, ya, xb, yb; P.at(p.u, p.v, xa, ya); P.at(p.u + c * p.len, p.v + s * p.len, xb, yb);
                if (std::max(p.r0, p.r1) * P.s < 0.7) { P.lineUV(p.u, p.v, p.u + c * p.len, p.v + s * p.len, base); break; }
                P.polyUV(U, V, n, base);
                // the side toward the light: the one whose normal points up the picture (or left)
                double nx, ny; P.at(p.u - s, p.v + c, nx, ny); nx -= xa; ny -= ya;   // the +across normal on the canvas
                const double side = (nx + ny) < 0 ? 1 : -1;
                auto stripe = [&](double f0, double f1, uint32_t col) {
                    const double q[4][2] = {{0, f0}, {p.len, f0}, {p.len, f1}, {0, f1}};
                    double su[4], sv[4];
                    for (int k = 0; k < 4; k++) { const double r = k == 1 || k == 2 ? p.r1 : p.r0; su[k] = p.u + c * q[k][0] - s * q[k][1] * r * side; sv[k] = p.v + s * q[k][0] + c * q[k][1] * r * side; }
                    P.polyUV(su, sv, 4, col);
                };
                if (std::max(p.r0, p.r1) * P.s >= 1.6) { stripe(0.25, 0.7, hi); stripe(-0.55, -1.0, lo); }
                else if (std::max(p.r0, p.r1) * P.s >= 1.0) stripe(-0.4, -1.0, lo);
                if (p.open && p.r1 * P.s >= 1.5) P.lineUV(p.u + c * p.len - s * p.r1 * 0.9, p.v + s * p.len + c * p.r1 * 0.9, p.u + c * p.len + s * p.r1 * 0.9, p.v + s * p.len - c * p.r1 * 0.9, edge);
                break;
            }
            case IPK_PLATE: case IPK_BOX: case IPK_DISC: {
                const int n = outlineOf(p, U, V);
                P.polyUV(U, V, n, base);
                const double size = (p.kind == IPK_DISC ? std::max(p.r0, p.r1) : std::max(p.r0, p.len * 0.5)) * P.s;
                const bool domed = p.kind == IPK_PLATE && p.dome > 0.002;
                if (p.kind == IPK_DISC && p.open) {   // a bowl: its dark inside, the light on its far wall
                    double Ui[96], Vi[96];
                    InstrumentPart in = p; in.taper = 1; in.r0 = p.r0 * std::max(1.0, p.taper) * 0.82; in.r1 = p.r1 * std::max(1.0, p.taper) * 0.82; in.open = false;
                    const int m = outlineOf(in, Ui, Vi);
                    P.polyUV(Ui, Vi, m, lo);
                    for (int k = 0; k < m; k++) { Ui[k] = p.u + (Ui[k] - p.u) * 0.7 + 0.12 * in.r0; Vi[k] = p.v + (Vi[k] - p.v) * 0.7 - 0.12 * in.r0; }
                    P.polyUV(Ui, Vi, m, mixc(lo, base, 0.45));
                } else if (domed && size > 3) {   // a dome's sheen toward the light
                    double Ui[96], Vi[96];
                    for (int k = 0; k < n; k++) { Ui[k] = p.u + (U[k] - p.u) * 0.72; Vi[k] = p.v + (V[k] - p.v) * 0.72; }
                    double dx, dy; P.at(p.u, p.v, dx, dy);
                    double sx[96], sy[96];
                    for (int k = 0; k < n; k++) { P.at(Ui[k], Vi[k], sx[k], sy[k]); sx[k] -= size * 0.1; sy[k] -= size * 0.1; }
                    P.fill(sx, sy, n, mixc(base, WHITE, 0.14));
                    for (int k = 0; k < n; k++) { sx[k] = dx + (sx[k] - dx) * 0.45 - size * 0.12; sy[k] = dy + (sy[k] - dy) * 0.45 - size * 0.12; }
                    P.fill(sx, sy, n, mixc(base, WHITE, 0.28));
                }
                if (size > 2.5) P.bevelUV(U, V, n, hi, edge);
                break;
            }
            case IPK_BALL: {
                const int n = outlineOf(p, U, V);
                P.polyUV(U, V, n, mixc(base, BLACK, 0.18));
                double x, y; P.at(p.u, p.v, x, y);
                const double size = std::max(p.r0, p.r1) * P.s;
                double sx[96], sy[96];
                for (int ring = 1; ring <= 3; ring++) {
                    const double f = 1 - ring * 0.22, sh = size * 0.12 * ring;
                    for (int k = 0; k < n; k++) { P.at(U[k], V[k], sx[k], sy[k]); sx[k] = x + (sx[k] - x) * f - sh; sy[k] = y + (sy[k] - y) * f - sh; }
                    P.fill(sx, sy, n, mixc(base, WHITE, 0.06 + 0.13 * (ring - 1)));
                }
                break;
            }
            case IPK_RING: {
                if (p.standing) {   // a band round a tube: across its axis
                    const double R = p.r0 + p.r1, q[4][2] = {{-p.r1, R}, {p.r1, R}, {p.r1, -R}, {-p.r1, -R}};
                    for (int k = 0; k < 4; k++) { U[k] = p.u + c * q[k][0] - s * q[k][1]; V[k] = p.v + s * q[k][0] + c * q[k][1]; }
                    P.polyUV(U, V, 4, base);
                    if (p.r1 * P.s >= 1.2) P.lineUV(U[0], V[0], U[3], V[3], hi);
                    break;
                }
                const double thick = p.r1 * P.s;
                const int segs = 40;
                for (int k = 0; k < segs; k++) {
                    const double a0 = TAU * k / segs, a1 = TAU * (k + 1) / segs, ri = p.r0 - p.r1, ro = p.r0 + p.r1;
                    if (thick < 0.8) { P.lineUV(p.u + std::cos(a0) * p.r0, p.v + std::sin(a0) * p.r0, p.u + std::cos(a1) * p.r0, p.v + std::sin(a1) * p.r0, base); continue; }
                    const double qu[4] = {p.u + std::cos(a0) * ri, p.u + std::cos(a1) * ri, p.u + std::cos(a1) * ro, p.u + std::cos(a0) * ro};
                    const double qv[4] = {p.v + std::sin(a0) * ri, p.v + std::sin(a1) * ri, p.v + std::sin(a1) * ro, p.v + std::sin(a0) * ro};
                    const double lit = std::cos(a0 - PI * 0.75);   // toward the upper left (v up: the upper left is +v, -u)
                    P.polyUV(qu, qv, 4, lit > 0.3 ? mixc(base, WHITE, 0.25 * lit) : (lit < -0.3 ? mixc(base, BLACK, -0.3 * lit) : base));
                }
                break;
            }
            case IPK_STRING: {
                const double sh = g > 0.02 ? g * 1.1 * std::sin(t * 55 + i * 1.3) / P.s : 0;
                const uint32_t col = g > 0.02 ? mixc(mixc(base, WHITE, 0.3), LIT, g) : mixc(base, WHITE, 0.15);
                P.lineUV(p.u - s * sh, p.v + c * sh, p.u + c * p.len - s * sh, p.v + s * p.len + c * sh, col);
                break;
            }
            case IPK_MARK: {
                const int n = outlineOf(p, U, V);
                const double size = std::max(p.r0, p.len * 0.5) * P.s;
                if (size < 0.6) { double x, y; P.at(p.u, p.v, x, y); fillRectRGB(cv, (int)std::floor(x), (int)std::floor(y), (int)std::floor(x), (int)std::floor(y), base); break; }
                P.polyUV(U, V, n, base);
                if (p.mat == IM_HOLE && size > 0.9) P.bevelUV(U, V, n, mixc(base, BLACK, 0.5), rgb(150, 140, 124));   // an incised edge: its lower right catches the light (a hole in a dark body shows by it)
                else if (p.mat == IM_LIGHT && g > 0.05) { double Ug[96], Vg[96]; const int m = outlineOf(p, Ug, Vg, 1.2 / P.s); P.bevelUV(Ug, Vg, m, LIT, LIT); }   // a light played: a halo round it
                break;
            }
            default: break;
        }
    }
    if (ruler) {   // its size: the longest round length that fits a third of the box
        static const double LEN[6] = {0.02, 0.05, 0.1, 0.2, 0.5, 1.0};
        static const char* LBL[6] = {"2 CM", "5 CM", "10 CM", "20 CM", "50 CM", "1 M"};
        int k = 0; for (int j = 0; j < 6; j++) if (LEN[j] * P.s <= w * 0.34) k = j;
        const int len = std::max(2, (int)std::lround(LEN[k] * P.s)), x = x0 + 5, y = y0 + h - 6;
        const uint32_t col = rgb(128, 138, 136);
        drawLineRGB(cv, x, y, x + len, y, col); drawLineRGB(cv, x, y - 2, x, y, col); drawLineRGB(cv, x + len, y - 2, x + len, y, col);
        drawText(cv, x + len + 4, y - 5, LBL[k], col);
    }
}

// C-17: its parts (`drawInstrumentParts`)
void Game::drawInstrumentPicture(int x0, int y0, int w, int h, const Instrument& I, const Tradition& T, const FormPalette& pal, const std::vector<double>& glow, double t) {
    std::vector<InstrumentPart> parts; instrumentParts(T, I, parts);
    drawInstrumentParts(canvas, x0, y0, w, h, parts, pal, glow, t, true);
}
