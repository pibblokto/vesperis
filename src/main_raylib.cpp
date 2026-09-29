// Desktop entry point. Rendering is software at 320x200; the platform layer
// scales it to the window with nearest-neighbour filtering.
#include "platform.h"
#include "game/game.h"
#include "core/png.h"
#include "core/fs.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

int main(int argc, char** argv) {
    int autoFrames = -1;
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--frames") && i + 1 < argc) autoFrames = atoi(argv[++i]);
    makeDir("shots");
    Game game;
#ifdef __APPLE__
    game.macHints = true;
#endif
    game.loadFromDisk();
    if (!platformInit(320 * game.settings.windowScale, 200 * game.settings.windowScale, "VESPERIS")) { fprintf(stderr, "could not open a window\n"); return 1; }
    AudioSynth synth;
    Input in;
    PlatformFrameInput pin;
    KeyMap keymap;                 // M6-02: optional physical-to-logical remap
    if (keymap.load("vesperis_keys.txt")) printf("key map loaded from vesperis_keys.txt\n");
    game.keymap = &keymap;         // N5-05: the key bindings screen edits and saves it
    PadState padPrev;
    bool captured = false;
    int shotCounter = 0, frames = 0;
    while (fileExists("shots/screenshot_" + std::to_string(shotCounter) + ".png")) shotCounter++;
    int panoCounter = 0;
    while (fileExists("shots/panorama_" + std::to_string(panoCounter) + ".png")) panoCounter++;
    double lastTime = platformTime();
    // one frame of the desktop loop; also the callback of the browser's animation loop (M6-04)
    auto step = [&]() {
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
        {
            PadState ps;
            ps.present = pin.pad.present; ps.lx = pin.pad.lx; ps.ly = pin.pad.ly; ps.rx = pin.pad.rx; ps.ry = pin.pad.ry; ps.lt = pin.pad.lt; ps.rt = pin.pad.rt;
            for (int b = 0; b < PAD_BUTTONS; b++) ps.buttons[b] = pin.pad.buttons[b];
            applyPad(ps, padPrev, in, game.settings.mouseSensitivity);
        }
        game.frame(in, dt);

        bool wantCap = game.mouseCaptureWanted() && platformFocused();
        if (wantCap != captured) { platformSetCapture(wantCap); captured = wantCap; }
        if (game.wantsFullscreenToggle) { game.wantsFullscreenToggle = false; platformToggleFullscreen(); }
        if (game.wantsWindowScale > 0) { platformSetWindowSize(320 * game.wantsWindowScale, 200 * game.wantsWindowScale); game.wantsWindowScale = 0; }
        platformAudioPump(synth, game.audio);
        if (game.wantsScreenshot || (autoFrames > 0 && frames == autoFrames - 1)) {
            game.wantsScreenshot = false;
            std::string fn = "shots/screenshot_" + std::to_string(shotCounter++) + ".png";
            writePNG(fn.c_str(), game.output(), FBW, FBH);
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
        if (game.recording) game.recordFrame();
        platformPresent(game.output(), FBW, FBH, game.settings.scanlines, game.settings.crt ? 1.0 : 0.0, game.settings.bloom ? 1.0 : 0.0, game.settings.aspect);
        frames++;
    };
#ifdef __EMSCRIPTEN__
    struct Ctx { decltype(step)* s; Game* g; } ctx{&step, &game};
    emscripten_set_main_loop_arg([](void* p) { Ctx* c = (Ctx*)p; if (!c->g->wantsQuit) (*c->s)(); }, &ctx, 0, 1);
#else
    while (!platformShouldClose() && !game.wantsQuit) { step(); if (autoFrames > 0 && frames >= autoFrames) break; }
#endif
    game.autosave();
    game.settings.save(game.settingsPath);
    platformShutdown();
    return 0;
}
