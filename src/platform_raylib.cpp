#include "raylib.h"
#include "platform.h"
#include <vector>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

namespace {
Texture2D gTex;
int gTexW = 0, gTexH = 0;
RenderTexture2D gRT;
int gRTW = 0, gRTH = 0;
Shader gPost;
bool gPostTried = false, gPostOk = false;
int locTexel = -1, locRows = -1, locCrt = -1, locBloom = -1;
const char* POST_FS = R"GLSL(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;
uniform sampler2D texture0;
uniform vec2 texel;
uniform float rows;
uniform float crt;
uniform float bloom;
void main() {
    vec2 uv = fragTexCoord;
    if (crt > 0.0) { vec2 c = uv * 2.0 - 1.0; c *= 1.0 + 0.04 * crt * dot(c, c); uv = c * 0.5 + 0.5; }
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) { finalColor = vec4(0.0, 0.0, 0.0, 1.0); return; }
    vec3 col = texture(texture0, uv).rgb;
    if (bloom > 0.0) {
        vec3 b = vec3(0.0);
        float wsum = 0.0;
        for (int i = -4; i <= 4; i++) {
            float w = 1.0 - abs(float(i)) / 5.0;
            vec2 o = vec2(float(i) * 2.0, 0.0) * texel;
            vec2 o2 = vec2(0.0, float(i) * 2.0) * texel;
            b += max(texture(texture0, uv + o).rgb - 0.55, 0.0) * w;
            b += max(texture(texture0, uv + o2).rgb - 0.55, 0.0) * w;
            wsum += 2.0 * w;
        }
        col += b / wsum * bloom * 1.8;
    }
    if (crt > 0.0) {
        float line = 0.5 + 0.5 * sin(uv.y * rows * 3.14159265);
        col *= 1.0 - 0.28 * crt * (1.0 - line);
        vec2 d = uv - 0.5;
        col *= 1.0 - 0.45 * crt * dot(d, d);
    }
    finalColor = vec4(col, 1.0);
}
)GLSL";

void ensurePostShader() {
    if (gPostTried) return;
    gPostTried = true;
    gPost = LoadShaderFromMemory(NULL, POST_FS);
    gPostOk = gPost.id != 0;
    if (gPostOk) {
        locTexel = GetShaderLocation(gPost, "texel");
        locRows = GetShaderLocation(gPost, "rows");
        locCrt = GetShaderLocation(gPost, "crt");
        locBloom = GetShaderLocation(gPost, "bloom");
    }
}
AudioStream gStream;
std::vector<float> gAbuf;
bool gAudioOk = false;
PlatformPresentTimes gPresentTimes;   // F-01
PlatformAudioTimes gAudioTimes;
double msSince(std::chrono::steady_clock::time_point t0) { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); }
// F-08: the frames' pacing (KI-407). raylib's SetTargetFPS(60) waited after the swap for 1/60 s less the frame's time, asleep for
// most of it; macOS wakes such a sleep up to a millisecond late, so on the dev Mac's 120 Hz display the frames came every 17.2 ms (58
// a second) and some stayed on the screen a refresh longer. The platform paces them instead: a frame is meant for the refresh a whole
// number of the monitor's refreshes after the last one's (two at 120 Hz, one at 60: floor(hz / 60 + 0.2): about 50 to 110 frames a
// second), and its swap waits until three quarters of a refresh before that one (asleep until PACE_SPIN before, then spinning): past
// the refresh between, so the frame cannot show there, and early enough for the compositor, which under load wants a frame well
// before its refresh (2 ms before missed whole stretches of refreshes). The vsync then meets the refresh. A swap that waited for the
// display returned at its refresh, later only by the thread's wake: the schedule follows a return earlier than it expected, or half a
// refresh or more late (the frame missed its refresh), all the way, and one a little late a tenth of the way (the display's clock
// against this one's, without the wakes' noise). A swap that did not wait (no vsync, a hidden window) leaves the schedule to the
// clock. The browser paces its own frames.
#ifndef __EMSCRIPTEN__
constexpr double PACE_SPIN = 0.001;
double gPacePeriod = 1.0 / 60.0, gPaceRefresh = 1.0 / 60.0, gPaceMargin = 0.0125;   // s
double gPaceNext = 0;              // GetTime() of the refresh the next frame is meant for
int gPaceHz = -1, gPaceFrames = 0;
void pacePeriod() {   // read at the start and every 60 frames: the window may have moved to another monitor
    const int hz = GetMonitorRefreshRate(GetCurrentMonitor());
    if (hz == gPaceHz) return;
    gPaceHz = hz;
    const double rate = hz >= 24 && hz <= 480 ? hz : 60;
    gPaceRefresh = 1.0 / rate;
    gPacePeriod = std::max(1, (int)std::floor(rate / 60.0 + 0.2)) * gPaceRefresh;
    gPaceMargin = 0.75 * gPaceRefresh;
}
#endif
}

PlatformPresentTimes platformPresentTimes() { return gPresentTimes; }
PlatformAudioTimes platformAudioTimes() { return gAudioTimes; }

bool platformInit(int w, int h, const char* title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(w, h, title);
    if (!IsWindowReady()) return false;
#ifdef __EMSCRIPTEN__
    SetTargetFPS(60);
#else
    SetTargetFPS(0);   // F-08: platformPresent paces the frames
    pacePeriod();
#endif
    SetExitKey(0);
    InitAudioDevice();
    gAudioOk = IsAudioDeviceReady();
    if (gAudioOk) {
        SetAudioStreamBufferSizeDefault(2048);
        gStream = LoadAudioStream(22050, 32, 1);
        PlayAudioStream(gStream);
        gAbuf.resize(2048);
    }
    return true;
}

bool platformShouldClose() { return WindowShouldClose(); }
double platformTime() { return GetTime(); }
bool platformFocused() { return IsWindowFocused(); }

void platformPoll(PlatformFrameInput& in, bool captured) {
    for (int k = 0; k < 400; k++) {
        in.down[k] = k >= 32 && IsKeyDown(k);
        in.pressed[k] = k >= 32 && IsKeyPressed(k);
    }
    Vector2 md = GetMouseDelta();
    in.mouseDx = captured ? md.x : 0;
    in.mouseDy = captured ? md.y : 0;
    in.wheel = (int)GetMouseWheelMove();
    for (int b = 0; b < 3; b++) { in.mouseDown[b] = IsMouseButtonDown(b); in.mousePressed[b] = IsMouseButtonPressed(b); }
    // M6-02: the first gamepad
    in.pad = PlatformPad();
    if (IsGamepadAvailable(0)) {
        in.pad.present = true;
        in.pad.lx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        in.pad.ly = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        in.pad.rx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
        in.pad.ry = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
        in.pad.lt = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_TRIGGER);
        in.pad.rt = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER);
        static const int RL_BUTTONS[16] = {GAMEPAD_BUTTON_RIGHT_FACE_DOWN, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, GAMEPAD_BUTTON_RIGHT_FACE_LEFT, GAMEPAD_BUTTON_RIGHT_FACE_UP,
                                                    GAMEPAD_BUTTON_LEFT_TRIGGER_1, GAMEPAD_BUTTON_RIGHT_TRIGGER_1, GAMEPAD_BUTTON_MIDDLE_RIGHT, GAMEPAD_BUTTON_MIDDLE_LEFT,
                                                    GAMEPAD_BUTTON_LEFT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_DOWN, GAMEPAD_BUTTON_LEFT_FACE_LEFT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
                                                    GAMEPAD_BUTTON_LEFT_THUMB, GAMEPAD_BUTTON_RIGHT_THUMB, GAMEPAD_BUTTON_MIDDLE, GAMEPAD_BUTTON_UNKNOWN};
        for (int b = 0; b < 16; b++) in.pad.buttons[b] = RL_BUTTONS[b] != GAMEPAD_BUTTON_UNKNOWN && IsGamepadButtonDown(0, RL_BUTTONS[b]);
    }
}

void platformSetCapture(bool on) { if (on) DisableCursor(); else EnableCursor(); }
void platformToggleFullscreen() { ToggleFullscreen(); }
void platformSetWindowSize(int w, int h) { if (!IsWindowFullscreen()) SetWindowSize(w, h); }

void platformAudioPump(AudioSynth& synth, AudioState& st) {
    gAudioTimes = PlatformAudioTimes();
    if (!gAudioOk) return;
    while (IsAudioStreamProcessed(gStream)) {
        auto t0 = std::chrono::steady_clock::now();
        synth.render(gAbuf.data(), (int)gAbuf.size(), 22050, st);
        auto t1 = std::chrono::steady_clock::now();
        UpdateAudioStream(gStream, gAbuf.data(), (int)gAbuf.size());
        gAudioTimes.blocks++; gAudioTimes.synth += std::chrono::duration<double, std::milli>(t1 - t0).count(); gAudioTimes.update += msSince(t1);
    }
}

void platformPresent(const uint32_t* rgb, int w, int h, bool scanlines, double crt, double bloom, int aspect) {
    if (w != gTexW || h != gTexH) {
        if (gTexW) UnloadTexture(gTex);
        Image img = GenImageColor(w, h, BLACK);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        gTex = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(gTex, TEXTURE_FILTER_POINT);
        gTexW = w; gTexH = h;
    }
    auto t0 = std::chrono::steady_clock::now();
    UpdateTexture(gTex, rgb);
    gPresentTimes.upload = msSince(t0);
    t0 = std::chrono::steady_clock::now();
    BeginDrawing();
    ClearBackground(BLACK);
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    // M6-01: the original's 320x200 filled a 4:3 screen, so its pixels were 1.2 times taller than wide
    float aspectY = aspect == 1 ? 1.2f : 1.0f;
    float scale = std::min((float)sw / w, (float)sh / (h * aspectY));
    float dw = w * scale, dh = h * scale * aspectY;
    Rectangle src = {0, 0, (float)w, (float)h};
    Rectangle dst = {(sw - dw) * 0.5f, (sh - dh) * 0.5f, dw, dh};
    // M10-16: at a fractional window scale, upscale by the integer part with nearest sampling first,
    // then draw that with bilinear filtering ("sharp bilinear": no moire, edges stay crisp)
    Texture2D drawTex = gTex;
    int k = (int)scale;
    bool fractional = scale > 1.05f && (scale - k) > 0.05f && (scale - k) < 0.95f;
    if (fractional) {
        if (k < 1) k = 1;
        if (gRTW != w * k || gRTH != h * k) {
            if (gRTW) UnloadRenderTexture(gRT);
            gRT = LoadRenderTexture(w * k, h * k);
            SetTextureFilter(gRT.texture, TEXTURE_FILTER_BILINEAR);
            gRTW = w * k; gRTH = h * k;
        }
        BeginTextureMode(gRT);
        DrawTexturePro(gTex, src, {0, 0, (float)(w * k), (float)(h * k)}, {0, 0}, 0, WHITE);
        EndTextureMode();
        drawTex = gRT.texture;
        src = {0, 0, (float)(w * k), -(float)(h * k)};   // render textures are stored upside down
    }
    bool post = crt > 0.001 || bloom > 0.001;
    if (post) {
        ensurePostShader();
        if (gPostOk) {
            float texel[2] = {1.0f / drawTex.width, 1.0f / drawTex.height};
            float rows = 200.0f, fcrt = (float)crt, fbloom = (float)bloom;
            SetShaderValue(gPost, locTexel, texel, SHADER_UNIFORM_VEC2);
            SetShaderValue(gPost, locRows, &rows, SHADER_UNIFORM_FLOAT);
            SetShaderValue(gPost, locCrt, &fcrt, SHADER_UNIFORM_FLOAT);
            SetShaderValue(gPost, locBloom, &fbloom, SHADER_UNIFORM_FLOAT);
            BeginShaderMode(gPost);
            DrawTexturePro(drawTex, src, dst, {0, 0}, 0, WHITE);
            EndShaderMode();
        } else DrawTexturePro(drawTex, src, dst, {0, 0}, 0, WHITE);
    } else DrawTexturePro(drawTex, src, dst, {0, 0}, 0, WHITE);
    // scanlines follow the logical 200 rows, whatever the framebuffer scale (the CRT shader has its own)
    float rowH = dh / 200.0f;
    if (scanlines && rowH >= 2 && crt <= 0.001) {
        for (int y = 0; y < 200; y++) {
            float yy = dst.y + (y + 1) * rowH - 1;
            DrawRectangle((int)dst.x, (int)yy, (int)dw, 1, Color{0, 0, 0, 70});
        }
    }
    gPresentTimes.draw = msSince(t0);
    t0 = std::chrono::steady_clock::now();
#ifdef __EMSCRIPTEN__
    EndDrawing();
    gPresentTimes.swap = msSince(t0);
#else
    if (++gPaceFrames >= 60) { gPaceFrames = 0; pacePeriod(); }
    const double now = GetTime();
    if (gPaceNext == 0 || now > gPaceNext + gPacePeriod) gPaceNext = now + gPaceMargin;   // the first frame, or one a whole period late: the schedule starts again
    const double holdUntil = gPaceNext - gPaceMargin;
    if (now + PACE_SPIN < holdUntil) std::this_thread::sleep_for(std::chrono::duration<double>(holdUntil - PACE_SPIN - now));
    while (GetTime() < holdUntil) {}
    gPresentTimes.hold = msSince(t0);
    t0 = std::chrono::steady_clock::now();
    const double swap0 = GetTime();
    EndDrawing();
    const double back = GetTime();
    gPresentTimes.swap = msSince(t0);
    const double late = back - gPaceNext;
    if (late < 0 ? back - swap0 > 0.0005 : late >= 0.5 * gPaceRefresh) gPaceNext = back;
    else if (late > 0) gPaceNext += 0.1 * late;
    gPaceNext += gPacePeriod;
#endif
}

void platformShutdown() {
    if (gTexW) UnloadTexture(gTex);
    if (gRTW) UnloadRenderTexture(gRT);
    if (gPostOk) UnloadShader(gPost);
    if (gAudioOk) { UnloadAudioStream(gStream); CloseAudioDevice(); }
    CloseWindow();
}
