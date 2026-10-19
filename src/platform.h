// Thin platform layer (implemented with raylib in platform_raylib.cpp). Kept
// free of raylib types so the game code never sees raylib's headers.
#pragma once
#include <cstdint>
#include "game/audio.h"

// R-418: the first controller, read whole: GLFW's standard layout when the system knows the device (`mapped`), and its raw buttons,
// axes and hats always (the fields of PadState in core/input.h, copied by the front end; kept apart so this header stays free of the
// core key enum, which clashes with raylib's own key names)
struct PlatformPad {
    bool present = false, mapped = false;
    bool buttons[15] = {};                        // A B X Y LB RB BACK START GUIDE L3 R3 UP RIGHT DOWN LEFT
    float axes[6] = {0, 0, 0, 0, -1, -1};         // LX LY RX RY LT RT
    int rawButtonCount = 0, rawAxisCount = 0, rawHatCount = 0;
    bool rawButtons[32] = {};
    float rawAxes[16] = {};
    unsigned char rawHats[4] = {};
    char name[64] = {};
    char guid[40] = {};
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
// F-01 (VESPERIS_TRACE): the last present's parts in ms (the texture's upload; the drawing up to the swap, which records the
// commands; F-08's hold, the swap held to its refresh; EndDrawing: the commands' flush, the buffer swap with its vsync wait and the
// input poll) and the last audio pump's (the synth's blocks, their ms, the stream's updates)
struct PlatformPresentTimes { double upload = 0, draw = 0, hold = 0, swap = 0; };
struct PlatformAudioTimes { int blocks = 0; double synth = 0, update = 0; };
PlatformPresentTimes platformPresentTimes();
PlatformAudioTimes platformAudioTimes();
