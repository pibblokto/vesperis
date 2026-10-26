// E-03: the frames' helpers: the clock, a frame to PNG, the bodies' maps made first, the hour of a sun's height, two frames
// compared, the test scale, the frame hash
#include "harness/harness.h"
double nowSec() {
    using namespace std::chrono;
    return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
}
void saveFB(Framebuffer& fb, const char* path, double gain) {
    std::vector<uint32_t> rgbBuf(FBW * FBH);
    fb.toRGB(rgbBuf.data(), gain);
    writePNG(path, rgbBuf.data(), FBW, FBH);
}

// maps are generated on worker threads (M9-15): the tests wait for the ones they are about to draw
void warmMaps(SpaceRenderer& sr, const StarSystem& sys, int only) {
    for (const Body& b : sys.bodies) {
        if (b.type == PT_COMPANION) continue;
        if (only >= 0 && b.index != only) continue;
        sr.mapFor(b);
    }
}

// find a time when the sun's altitude at the site is close to the target (radians): over one rotation from t0, and when that
// misses by over a degree (G-04: a season without that light at this latitude, a polar day) over eleven more epochs spread
// through the year (the parent's for a moon); `errOut` is the miss, so a finder can tell a world locked to its star (the sun
// never moves: another longitude) from a latitude that never sees that light
// the altitude of the light the scene is lit by: round a binary, the two suns blended by their strength above the horizon
// as `SurfaceView::computeEnvironment` blends them (matching the primary alone would find a "night" under the companion)
double effectiveSunAlt(const SurfaceSite& site, double t) {
    SunInfo s1 = site.sun(t), s2;
    if (!site.sun2(t, s2)) return s1.altitude;
    double w1 = s1.lightFactor * smoothstep(-0.12, 0.05, s1.dirLocal.y), w2 = s2.lightFactor * smoothstep(-0.12, 0.05, s2.dirLocal.y);
    if (w1 + w2 < 1e-6 || w2 / (w1 + w2) <= 0.02) return s1.altitude;
    return std::asin(clampd(normalize(s1.dirLocal * w1 + s2.dirLocal * w2).y, -1, 1));
}
double findTime(SurfaceSite& site, double targetAlt, double t0, double* errOut) {
    const Body& b = site.sys->bodies[site.body];
    double period = std::fabs(b.rotPeriod), year = b.parent >= 0 ? site.sys->bodies[b.parent].orbitPeriod : b.orbitPeriod;
    double best = t0, bestErr = 1e9;
    auto scanDay = [&](double from) {
        for (int i = 0; i < 720; i++) {
            double t = from + period * i / 720.0;
            double err = std::fabs(effectiveSunAlt(site, t) - targetAlt);
            if (err < bestErr) { bestErr = err; best = t; }
        }
    };
    scanDay(t0);
    for (int k = 1; k < 12 && bestErr > 1.0 * DEG; k++) scanDay(t0 + year * k / 12.0);
    if (errOut) *errOut = bestErr;
    return best;
}
FrameDiff diffFrames(const uint32_t* a, const uint32_t* b, int w, int h) {
    FrameDiff d;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            const uint32_t p = a[(size_t)y * w + x], q = b[(size_t)y * w + x];
            if (p == q) continue;
            for (int c = 0; c < 24; c += 8) d.step = std::max(d.step, std::abs((int)((p >> c) & 255) - (int)((q >> c) & 255)));
            if (d.differ++ == 0) { d.firstX = x; d.firstY = y; d.rowMin = y; }
            d.rowMax = y;
            d.eighths[std::min(7, y * 8 / h)]++;
        }
    return d;
}
std::string diffLine(const FrameDiff& d, int w, int h) {
    if (!d.differ) return "identical";
    std::string s = fmt("%ld px differ (%.3f%%), largest step %d, first at %d,%d, rows %d-%d, by eighths of the height", d.differ, 100.0 * d.differ / ((double)w * h), d.step, d.firstX, d.firstY,
            d.rowMin, d.rowMax);
    for (int k = 0; k < 8; k++) s += fmt("%s%ld", k ? " " : ": ", d.eighths[k]);
    return s;
}
// the differing pixels in red over the first frame at a third of its brightness
void writeDiffImage(const char* path, const uint32_t* a, const uint32_t* b, int w, int h) {
    std::vector<uint32_t> out((size_t)w * h);
    for (size_t i = 0; i < out.size(); i++) {
        const uint32_t p = a[i];
        out[i] = p != b[i] ? 0xFF0000FFu : 0xFF000000u | ((((p >> 16) & 255) / 3) << 16) | ((((p >> 8) & 255) / 3) << 8) | ((p & 255) / 3);
    }
    writePNG(path, out.data(), w, h);
}

void applyTestScale(Game& g) { g.settings.renderScale = g_testScale; g.applySettings(); }

// ---- determinism regression (M0-03) ----------------------------------------------
// Hashes generated content that must not change by accident: the first systems met in the
// start region, ten planet maps, and height profiles at three sites. `regress bless`
// rewrites tests/regress_baseline.txt; `regress` compares against it (exit code 1 on change,
// or when one thread draws the forest or the cabin differently: F-11).
uint64_t fnv(uint64_t h, const void* p, size_t n) {
    const uint8_t* b = (const uint8_t*)p;
    for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ULL; }
    return h;
}

// E-03: the scripted games' moves, as the unit's and the modes' lambdas made them (`auto press = [&](int key) { pressKey(game, in, key); };`):
// a key pressed for a frame of 1/30 s; the frames of `secs` seconds at 30 a second; the same, at least one frame
void pressKey(Game& game, Input& in, int key) { in.pressed[key] = true; in.down[key] = true; game.frame(in, 1.0 / 30); in.newFrame(); in.down[key] = false; }
void runFrames(Game& game, Input& in, double secs) { int n = (int)(secs * 30); for (int i = 0; i < n; i++) { game.frame(in, 1.0 / 30); in.newFrame(); } }
void runFramesAtLeastOne(Game& game, Input& in, double secs) {
    int n = std::max(1, (int)(secs * 30));
    for (int i = 0; i < n; i++) { game.frame(in, 1.0 / 30); in.newFrame(); }
}
// U-01 (KI-426): the open menu's row of that label selected by Down or Up, a key a frame: found by its label (the game's own,
// `testMenuRows`), never counted; false when the menu has no such row
bool selectRow(TestGame& game, Input& in, const std::string& label) {
    const std::vector<std::string> rows = game.testMenuRows();
    const int k = (int)(std::find(rows.begin(), rows.end(), label) - rows.begin());
    if (k >= (int)rows.size()) return false;
    for (int i = 0; i < (int)rows.size() && game.testMenuSel() != k; i++) pressKey(game, in, game.testMenuSel() < k ? KEY_DOWN : KEY_UP);
    return game.testMenuSel() == k;
}
