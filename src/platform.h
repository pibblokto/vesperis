// Thin platform layer (implemented with raylib in platform_raylib.cpp). Kept
// free of raylib types so the game code never sees raylib's headers.
#pragma once
#include <cstdint>
#include "game/audio.h"

// M6-02: the first gamepad, raylib's button order (see PadState in core/input.h; kept separate so this
// header stays free of the core key enum, which clashes with raylib's own key names)
struct PlatformPad {
    bool present = false;
    float lx = 0, ly = 0, rx = 0, ry = 0, lt = -1, rt = -1;
    bool buttons[16] = {false};
};

struct PlatformFrameInput {
    bool down[400];
    bool pressed[400];
    float mouseDx, mouseDy;
    int wheel;
    bool mouseDown[3], mousePressed[3];
    PlatformPad pad;
};

bool platformInit(int w, int h, const char* title);
bool platformShouldClose();
double platformTime();
void platformPoll(PlatformFrameInput& in, bool captured);
void platformSetCapture(bool on);
bool platformFocused();
// crt/bloom 0..1 enable the optional post shader (M10-17); fractional window scales use a
// sharp-bilinear path (integer nearest upscale, then bilinear) (M10-16).
// aspect 0 keeps square pixels, 1 shows the picture at 4:3 as the original's 320x200 was (M6-01).
void platformPresent(const uint32_t* rgb, int w, int h, bool scanlines, double crt, double bloom, int aspect);
void platformToggleFullscreen();
void platformSetWindowSize(int w, int h);
void platformAudioPump(AudioSynth& synth, AudioState& st);
void platformShutdown();
