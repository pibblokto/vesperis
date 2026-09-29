#include "raylib.h"
#include "platform.h"
#include <vector>
#include <algorithm>

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
}

bool platformInit(int w, int h, const char* title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(w, h, title);
    if (!IsWindowReady()) return false;
    SetTargetFPS(60);
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
    if (!gAudioOk) return;
    while (IsAudioStreamProcessed(gStream)) {
        synth.render(gAbuf.data(), (int)gAbuf.size(), 22050, st);
        UpdateAudioStream(gStream, gAbuf.data(), (int)gAbuf.size());
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
    UpdateTexture(gTex, rgb);
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
    EndDrawing();
}

void platformShutdown() {
    if (gTexW) UnloadTexture(gTex);
    if (gRTW) UnloadRenderTexture(gRT);
    if (gPostOk) UnloadShader(gPost);
    if (gAudioOk) { UnloadAudioStream(gStream); CloseAudioDevice(); }
    CloseWindow();
}
