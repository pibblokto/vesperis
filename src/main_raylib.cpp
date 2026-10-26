// Desktop entry point. Rendering is software at 320x200; the platform layer
// scales it to the window with nearest-neighbour filtering.
#include "platform.h"
#include "game/game.h"
#include "peoples/plan.h"
#include "core/png.h"
#include "core/fs.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

// E-05: the platform's frame (platform.h, which keeps clear of core/input.h's key names) is copied into Input and PadState by index
static_assert(sizeof(PlatformFrameInput::down) == KEY_MAX && sizeof(PlatformFrameInput::pressed) == KEY_MAX &&
              sizeof(PlatformFrameInput::mouseDown) == sizeof(Input::mouseDown), "the platform's key and mouse states are Input's");
static_assert(sizeof(PlatformPad::buttons) == PAD_BUTTONS * sizeof(bool) && sizeof(PlatformPad::axes) == PAD_AXES * sizeof(float) &&
              sizeof(PlatformPad::rawButtons) == PAD_RAW_BUTTONS * sizeof(bool) && sizeof(PlatformPad::rawAxes) == PAD_RAW_AXES * sizeof(float) &&
              sizeof(PlatformPad::rawHats) == PAD_RAW_HATS && sizeof(PlatformPad::name) == sizeof(PadState::name) &&
              sizeof(PlatformPad::guid) == sizeof(PadState::guid), "the platform's controller is PadState's");

namespace {
// F-01: VESPERIS_TRACE=1 times the parts of the real game's frame, which the headless bench never runs (the window's upload and
// swap, the synth's blocks, the files): once a second the mean and the worst of each on stderr, and at the quit the percentiles
// of the frames' intervals and of their work without the hold and the swap (F-08: the pacing's wait and the vsync's)
struct FrameTrace {
    enum { POLL, GAME, AUDIO, SYNTH, SHOT, REC, UPLOAD, DRAW, HOLD, SWAP, WORK, INTERVAL, N };
    const char* names[N] = {"poll", "game", "audio", "synth", "shot", "rec", "upload", "draw", "hold", "swap", "work", "interval"};
    double sum[N] = {}, worst[N] = {}, v[N] = {};
    int frames = 0, blocks = 0;
    double lastReport = -1;
    std::vector<float> intervals, works;
    void add(double now) {
        if (lastReport < 0) lastReport = now;
        for (int k = 0; k < N; k++) { sum[k] += v[k]; worst[k] = std::max(worst[k], v[k]); }
        intervals.push_back((float)v[INTERVAL]); works.push_back((float)v[WORK]);
        frames++;
        if (now - lastReport < 1.0) return;
        fprintf(stderr, "frames %d in %.2f s, ms mean/worst:", frames, now - lastReport);
        for (int k = 0; k < N; k++) fprintf(stderr, " %s %.2f/%.2f%s", names[k], sum[k] / frames, worst[k], k == SYNTH ? (" (" + std::to_string(blocks) + " blocks)").c_str() : "");
        fprintf(stderr, "\n");
        for (int k = 0; k < N; k++) sum[k] = worst[k] = 0;
        frames = 0; blocks = 0; lastReport = now;
    }
    void summary() const {
        auto pct = [](std::vector<float> s, const char* what) {
            if (s.empty()) return;
            std::sort(s.begin(), s.end());
            auto at = [&](double p) { return s[std::min(s.size() - 1, (size_t)(p * s.size()))]; };
            int over17 = 0, over20 = 0, over33 = 0;   // F-08: 17.5, as paced frames sit at 16.67 give or take the swap's wake
            for (float x : s) { over17 += x > 17.5f; over20 += x > 20.0f; over33 += x > 33.4f; }
            fprintf(stderr, "%s over %zu frames: p50 %.2f, p90 %.2f, p99 %.2f, max %.2f ms; over 17.5 ms %d, over 20 ms %d, over 33.4 ms %d\n", what, s.size(), at(0.5), at(0.9), at(0.99), s.back(), over17, over20, over33);
        };
        pct(intervals, "frame intervals");
        pct(works, "frame work without the swap");
    }
};
double msBetween(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); }
}

int main(int argc, char** argv) {
    static const bool trace = std::getenv("VESPERIS_TRACE") != nullptr;   // F-01: read once
    FrameTrace ft;
    auto stepStart = std::chrono::steady_clock::now();
    bool stepped = false;
    int autoFrames = -1;
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--frames") && i + 1 < argc) autoFrames = atoi(argv[++i]);
    std::atexit(worldPlanThreadsJoin);   // C-18: a plan's prefetch still running at the quit is waited for before the caches it writes are destroyed
    makeDir("shots");
    Game game;
#ifdef __APPLE__
    game.macHints = true;
#endif
    game.loadFromDisk();
    if (!platformInit(320 * game.settings.windowScale, 200 * game.settings.windowScale, "VESPERIS")) { fprintf(stderr, "could not open a window\n"); return 1; }
    game.tapeGameDirs(platformAppDir());   // R-420: where the game's own tape lies
    AudioSynth synth;
    Input in;
    PlatformFrameInput pin;
    KeyMap keymap;                 // M6-02: optional physical-to-logical remap
    if (keymap.load("vesperis_keys.txt")) printf("key map loaded from vesperis_keys.txt\n");
    game.keymap = &keymap;         // N5-05: the key bindings screen edits and saves it
    bool captured = false;
    int shotCounter = 0, frames = 0;
    while (fileExists("shots/screenshot_" + std::to_string(shotCounter) + ".png")) shotCounter++;
    int panoCounter = 0;
    while (fileExists("shots/panorama_" + std::to_string(panoCounter) + ".png")) panoCounter++;
    double lastTime = platformTime();
    // one frame of the desktop loop; also the callback of the browser's animation loop (M6-04)
    auto step = [&]() {
        using Clock = std::chrono::steady_clock;
        const Clock::time_point c0 = trace ? Clock::now() : Clock::time_point();
        if (trace) { ft.v[FrameTrace::INTERVAL] = msBetween(stepStart, c0); stepStart = c0; }
        double now = platformTime();
        double dt = now - lastTime;
        lastTime = now;
        platformPoll(pin, captured);
        in.newFrame();
        memcpy(in.down, pin.down, sizeof(in.down));
        memcpy(in.pressed, pin.pressed, sizeof(in.pressed));
        in.mouseDx = pin.mouseDx; in.mouseDy = pin.mouseDy; in.wheel = pin.wheel;
        memcpy(in.mouseDown, pin.mouseDown, sizeof(in.mouseDown));
        memcpy(in.mousePressed, pin.mousePressed, sizeof(in.mousePressed));
        game.rawKey = -1;
        for (int k = 32; k < 400; k++) if (pin.pressed[k]) { game.rawKey = k; break; }
        keymap.apply(in);
        {   // R-418: the controller as read; the game translates it (game/pad.cpp)
            const PlatformPad& pp = pin.pad;
            PadState& ps = in.pad;
            ps.present = pp.present; ps.mapped = pp.mapped;
            for (int b = 0; b < PAD_BUTTONS; b++) ps.buttons[b] = pp.buttons[b];
            for (int a = 0; a < PAD_AXES; a++) ps.axes[a] = pp.axes[a];
            ps.rawButtonCount = pp.rawButtonCount; ps.rawAxisCount = pp.rawAxisCount; ps.rawHatCount = pp.rawHatCount;
            for (int b = 0; b < PAD_RAW_BUTTONS; b++) ps.rawButtons[b] = pp.rawButtons[b];
            for (int a = 0; a < PAD_RAW_AXES; a++) ps.rawAxes[a] = pp.rawAxes[a];
            for (int h = 0; h < PAD_RAW_HATS; h++) ps.rawHats[h] = pp.rawHats[h];
            memcpy(ps.name, pp.name, sizeof ps.name); memcpy(ps.guid, pp.guid, sizeof ps.guid);
        }
        const Clock::time_point c1 = trace ? Clock::now() : Clock::time_point();
        game.frame(in, dt);
        const Clock::time_point c2 = trace ? Clock::now() : Clock::time_point();

        bool wantCap = game.mouseCaptureWanted() && platformFocused();
        if (wantCap != captured) { platformSetCapture(wantCap); captured = wantCap; }
        if (game.wantsFullscreenToggle) { game.wantsFullscreenToggle = false; platformToggleFullscreen(); }
        if (game.wantsWindowScale > 0) { platformSetWindowSize(320 * game.wantsWindowScale, 200 * game.wantsWindowScale); game.wantsWindowScale = 0; }
        const Clock::time_point c3 = trace ? Clock::now() : Clock::time_point();
        platformAudioPump(synth, game.audio);
        const Clock::time_point c4 = trace ? Clock::now() : Clock::time_point();
        if (game.wantsScreenshot || (autoFrames > 0 && frames == autoFrames - 1)) {
            game.wantsScreenshot = false;
            std::string fn = "shots/screenshot_" + std::to_string(shotCounter++) + ".png";
            writePNGAsync(fn.c_str(), game.output(), FBW, FBH);   // F-02: encoded and written by the PNG writer's thread
            { FILE* sc = fopen((fn.substr(0, fn.size() - 4) + ".txt").c_str(), "w"); if (sc) { fprintf(sc, "%s\n", game.screenshotCaption().c_str()); fclose(sc); } }
            game.noteScreenshot();
            printf("saved %s\n", fn.c_str());
        }
        if (game.wantsPanorama) {   // M6-05
            std::vector<uint32_t> pano; int pw, ph;
            game.renderPanorama(pano, pw, ph);
            std::string fn = "shots/panorama_" + std::to_string(panoCounter++) + ".png";
            writePNG(fn.c_str(), pano.data(), pw, ph);
            printf("saved %s\n", fn.c_str());
        }
        const Clock::time_point c5 = trace ? Clock::now() : Clock::time_point();
        if (game.recording) game.recordFrame();
        const Clock::time_point c6 = trace ? Clock::now() : Clock::time_point();
        platformPresent(game.output(), FBW, FBH, game.settings.scanlines, game.settings.crt ? 1.0 : 0.0, game.settings.bloom ? 1.0 : 0.0, game.settings.aspect);
        frames++;
        if (trace) {   // F-01
            const Clock::time_point c7 = Clock::now();
            const PlatformAudioTimes at = platformAudioTimes();
            const PlatformPresentTimes pt = platformPresentTimes();
            ft.v[FrameTrace::POLL] = msBetween(c0, c1); ft.v[FrameTrace::GAME] = msBetween(c1, c2); ft.v[FrameTrace::AUDIO] = msBetween(c3, c4);
            ft.v[FrameTrace::SYNTH] = at.synth; ft.blocks += at.blocks;
            ft.v[FrameTrace::SHOT] = msBetween(c4, c5); ft.v[FrameTrace::REC] = msBetween(c5, c6);
            ft.v[FrameTrace::UPLOAD] = pt.upload; ft.v[FrameTrace::DRAW] = pt.draw; ft.v[FrameTrace::HOLD] = pt.hold; ft.v[FrameTrace::SWAP] = pt.swap;
            ft.v[FrameTrace::WORK] = msBetween(c0, c7) - pt.hold - pt.swap;
            if (stepped) ft.add(std::chrono::duration<double>(c7.time_since_epoch()).count());   // the first frame has no interval
            stepped = true;
        }
    };
#ifdef __EMSCRIPTEN__
    struct Ctx { decltype(step)* s; Game* g; } ctx{&step, &game};
    emscripten_set_main_loop_arg([](void* p) { Ctx* c = (Ctx*)p; if (!c->g->wantsQuit) (*c->s)(); }, &ctx, 0, 1);
#else
    while (!platformShouldClose() && !game.wantsQuit) { step(); if (autoFrames > 0 && frames >= autoFrames) break; }
#endif
    if (trace) ft.summary();
    game.autosave();
    game.settings.save(game.settingsPath);
    pngFlush();   // F-02: the photographs and the recording's last frames on disk before the window goes
    platformShutdown();
    return 0;
}
