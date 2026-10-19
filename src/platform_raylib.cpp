#include "raylib.h"
#include "platform.h"
#include <vector>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

#ifndef __EMSCRIPTEN__
// R-418: GLFW's joystick calls, inside raylib's static library (raylib's own gamepad calls read only the devices GLFW has a mapping
// for, and only through that mapping; a controller nobody mapped reads as nothing). The C declarations of GLFW 3.3+, raylib 5.5's own.
// Weak on ELF: a Linux build against a shared raylib that keeps GLFW hidden still links, and reads the controller through raylib
#if defined(__ELF__)
#define VESPERIS_GLFW_WEAK __attribute__((weak))
#else
#define VESPERIS_GLFW_WEAK
#endif
extern "C" {
typedef struct GLFWgamepadstate { unsigned char buttons[15]; float axes[6]; } GLFWgamepadstate;
VESPERIS_GLFW_WEAK int glfwJoystickPresent(int jid);
VESPERIS_GLFW_WEAK int glfwJoystickIsGamepad(int jid);
VESPERIS_GLFW_WEAK const float* glfwGetJoystickAxes(int jid, int* count);
VESPERIS_GLFW_WEAK const unsigned char* glfwGetJoystickButtons(int jid, int* count);
VESPERIS_GLFW_WEAK const unsigned char* glfwGetJoystickHats(int jid, int* count);
VESPERIS_GLFW_WEAK const char* glfwGetJoystickName(int jid);
VESPERIS_GLFW_WEAK const char* glfwGetJoystickGUID(int jid);
VESPERIS_GLFW_WEAK int glfwGetGamepadState(int jid, GLFWgamepadstate* state);
}
#endif

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
// R-420: auto-explore's tape beside the synth, raylib's music stream (the file decoded as it plays, at its own rate, in stereo). A thread
// of its own feeds it and ends it. The game's frames can stall longer than a stream's buffers last (a landing's first ground, a star's
// system), and raylib, told not to loop, stops a track as it reads its last block, that block and the one playing still unheard: so the
// stream loops in raylib and the thread stops it 30 ms short of its end, looking every 5 ms. The pump hands the thread the game's
// requests and takes its reports back (the browser has no thread there: the pump does its work)
struct MusicDeck {
    std::mutex m;
    std::string want; double from = 0, gain = 1; bool go = false;   // asked: a file (empty: none) from a second, at a level
    Music music{}; bool loaded = false;                               // the stream
    double t = -1, len = 0; bool ended = false, failed = false;       // told
    std::thread th; bool running = false, quit = false;
};
MusicDeck gMusic;
void musicStep(MusicDeck& D) {   // D.m held
    if (D.go) {
        D.go = false;
        if (D.loaded) { StopMusicStream(D.music); UnloadMusicStream(D.music); D.loaded = false; }
        D.t = -1; D.len = 0;
        if (!D.want.empty()) {
            SetAudioStreamBufferSizeDefault(8192);   // a half of the stream's buffer 0.19 s at 44.1 kHz
            D.music = LoadMusicStream(D.want.c_str());
            SetAudioStreamBufferSizeDefault(2048);
            if (IsMusicValid(D.music)) {
                D.music.looping = true;
                D.len = GetMusicTimeLength(D.music);
                if (D.from > 0 && D.from >= D.len - 2) { UnloadMusicStream(D.music); D.ended = true; return; }   // left at its very end: over
                if (D.from > 0.5) SeekMusicStream(D.music, (float)D.from);
                UpdateMusicStream(D.music);   // both halves filled before it sounds
                SetMusicVolume(D.music, (float)D.gain);
                PlayMusicStream(D.music);
                D.loaded = true;
            } else D.failed = true;
        }
    }
    if (!D.loaded) return;
    SetMusicVolume(D.music, (float)D.gain);
    UpdateMusicStream(D.music);
    const double t = GetMusicTimePlayed(D.music);
    if (t >= D.len - 0.03 || (D.t > 1 && t < D.t - 1)) {   // its end (or just past it, the stream gone round)
        StopMusicStream(D.music); UnloadMusicStream(D.music); D.loaded = false; D.ended = true; D.t = -1;
        return;
    }
    D.t = t;
}
void musicThread() {
    for (;;) {
        { std::lock_guard<std::mutex> lk(gMusic.m); if (gMusic.quit) return; musicStep(gMusic); }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
// the pump's side: the game's request handed over (the thread begun with the first), the thread's reports taken back
void musicPump(AudioState& st) {
    MusicDeck& D = gMusic;
    if (!st.musicGo && !D.running) return;
    std::lock_guard<std::mutex> lk(D.m);
    if (st.musicGo) {
        st.musicGo = false;
        D.want = st.music; D.from = st.musicFrom; D.go = true; D.ended = D.failed = false; D.t = -1;
#ifndef __EMSCRIPTEN__
        if (!D.running) { D.running = true; D.th = std::thread(musicThread); }
#else
        D.running = true;
#endif
    }
    D.gain = std::max(0.0, st.musicGain * st.master);
#ifdef __EMSCRIPTEN__
    musicStep(D);
#endif
    st.musicT = D.t; st.musicLen = D.len;
    if (D.ended) { st.musicEnded = true; D.ended = false; }
    if (D.failed) { st.musicFailed = true; D.failed = false; }
}
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
    // R-418: SDL's controller database beside the game (`gamecontrollerdb.txt`: the community's file, or a line of one's own) teaches GLFW
    // the layout of the controllers its own list lacks
    if (FileExists("gamecontrollerdb.txt")) {
        char* db = LoadFileText("gamecontrollerdb.txt");
        if (db) { const int ok = SetGamepadMappings(db); UnloadFileText(db); printf("controller mappings %s from gamecontrollerdb.txt\n", ok ? "read" : "not understood"); }
    }
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
const char* platformAppDir() { return GetApplicationDirectory(); }
double platformTime() { return GetTime(); }
bool platformFocused() { return IsWindowFocused(); }

// R-418: the first gamepad through raylib's own calls, the standard layout alone (the browser; a build whose raylib keeps GLFW hidden)
[[maybe_unused]] static void padFromRaylib(PlatformPad& p) {
    if (!IsGamepadAvailable(0)) return;
    p.present = p.mapped = true;
    snprintf(p.name, sizeof p.name, "%s", GetGamepadName(0));
    static const int RL_BUTTONS[15] = {GAMEPAD_BUTTON_RIGHT_FACE_DOWN, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, GAMEPAD_BUTTON_RIGHT_FACE_LEFT, GAMEPAD_BUTTON_RIGHT_FACE_UP,
                                       GAMEPAD_BUTTON_LEFT_TRIGGER_1, GAMEPAD_BUTTON_RIGHT_TRIGGER_1, GAMEPAD_BUTTON_MIDDLE_LEFT, GAMEPAD_BUTTON_MIDDLE_RIGHT, GAMEPAD_BUTTON_MIDDLE,
                                       GAMEPAD_BUTTON_LEFT_THUMB, GAMEPAD_BUTTON_RIGHT_THUMB, GAMEPAD_BUTTON_LEFT_FACE_UP, GAMEPAD_BUTTON_LEFT_FACE_RIGHT, GAMEPAD_BUTTON_LEFT_FACE_DOWN, GAMEPAD_BUTTON_LEFT_FACE_LEFT};
    for (int k = 0; k < 15; k++) p.buttons[k] = IsGamepadButtonDown(0, RL_BUTTONS[k]);
    static const int RL_AXES[6] = {GAMEPAD_AXIS_LEFT_X, GAMEPAD_AXIS_LEFT_Y, GAMEPAD_AXIS_RIGHT_X, GAMEPAD_AXIS_RIGHT_Y, GAMEPAD_AXIS_LEFT_TRIGGER, GAMEPAD_AXIS_RIGHT_TRIGGER};
    for (int k = 0; k < 6; k++) p.axes[k] = GetGamepadAxisMovement(0, RL_AXES[k]);
}

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
    // R-418: the first controller: the first GLFW knows the layout of, else the first with four buttons or more (a pad's motion sensor
    // shows up as a joystick of axes alone), read raw always and through its standard layout when known
    in.pad = PlatformPad();
#if defined(__ELF__)
    if (!glfwJoystickPresent || !glfwGetGamepadState || !glfwGetJoystickHats || !glfwGetJoystickGUID) { padFromRaylib(in.pad); return; }   // GLFW hidden: raylib's reading
#endif
#ifndef __EMSCRIPTEN__
    int jid = -1;
    for (int j = 0; j < 16; j++) {
        if (!glfwJoystickPresent(j)) continue;
        if (glfwJoystickIsGamepad(j)) { jid = j; break; }
        int nb = 0;
        if (jid < 0 && glfwGetJoystickButtons(j, &nb) && nb >= 4) jid = j;
    }
    if (jid >= 0) {
        PlatformPad& p = in.pad;
        p.present = true;
        if (const char* nm = glfwGetJoystickName(jid)) snprintf(p.name, sizeof p.name, "%s", nm);
        if (const char* gu = glfwGetJoystickGUID(jid)) snprintf(p.guid, sizeof p.guid, "%s", gu);
        int n = 0;
        if (const unsigned char* b = glfwGetJoystickButtons(jid, &n)) { p.rawButtonCount = std::min(n, 32); for (int k = 0; k < p.rawButtonCount; k++) p.rawButtons[k] = b[k] != 0; }
        if (const float* a = glfwGetJoystickAxes(jid, &n)) { p.rawAxisCount = std::min(n, 16); for (int k = 0; k < p.rawAxisCount; k++) p.rawAxes[k] = a[k]; }
        if (const unsigned char* h = glfwGetJoystickHats(jid, &n)) { p.rawHatCount = std::min(n, 4); for (int k = 0; k < p.rawHatCount; k++) p.rawHats[k] = h[k]; }
        GLFWgamepadstate gs;
        if (glfwJoystickIsGamepad(jid) && glfwGetGamepadState(jid, &gs)) {
            p.mapped = true;
            for (int k = 0; k < 15; k++) p.buttons[k] = gs.buttons[k] != 0;
            for (int k = 0; k < 6; k++) p.axes[k] = gs.axes[k];
        }
    }
#else
    padFromRaylib(in.pad);   // the browser's standard mapping (no raw device there)
#endif
}

void platformSetCapture(bool on) { if (on) DisableCursor(); else EnableCursor(); }
void platformToggleFullscreen() { ToggleFullscreen(); }
void platformSetWindowSize(int w, int h) { if (!IsWindowFullscreen()) SetWindowSize(w, h); }

void platformAudioPump(AudioSynth& synth, AudioState& st) {
    gAudioTimes = PlatformAudioTimes();
    if (!gAudioOk) { if (st.musicGo) { st.musicGo = false; if (!st.music.empty()) st.musicFailed = true; } return; }   // R-420: no device, no tape
    musicPump(st);
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
    if (gMusic.running) {   // R-420: the tape's thread let go, its stream with it
        { std::lock_guard<std::mutex> lk(gMusic.m); gMusic.quit = true; }
        if (gMusic.th.joinable()) gMusic.th.join();
        if (gMusic.loaded) { StopMusicStream(gMusic.music); UnloadMusicStream(gMusic.music); gMusic.loaded = false; }
    }
    if (gAudioOk) { UnloadAudioStream(gStream); CloseAudioDevice(); }
    CloseWindow();
}
