// Player settings, persisted as a small text file (`vesperis_settings.txt`).
#pragma once
#include <string>

struct Settings {
    double mouseSensitivity = 1.0;   // 0.2 .. 3.0
    bool invertY = false;
    double fovDeg = 70;              // 50 .. 100 horizontal
    double masterVolume = 0.8;       // 0 .. 1
    bool scanlines = true;
    int windowScale = 4;             // window = 320*s x 200*s (2 .. 6)
    int renderScale = 4;             // framebuffer = 320*s x 200*s (1 .. 4); 4x with the fine (3x3) mush is the default since B-310 (2x with a 3x3 mush before)
    int mushMode = 3;                // 0 classic (box = scale + 1), 1 soft (+2), 2 sharp (max(2, scale)), 3 fine (max(2, scale - 1); the default since B-310)
    int shadingMode = 2;             // 0 flat cells, 1 stepped gradient (M10-12), 2 smooth gradient (B-311, the default since the 4 m ring made flat cells a mosaic)
    bool crt = false;                // optional CRT post shader (M10-17), off by default
    bool bloom = false;              // optional bloom post shader, off by default
    bool sprintToggle = false;       // M8-10: Shift toggles the sprint instead of holding it
    double sprintSpeed = 3.2;        // sprint multiplier on 1 g (2.5 .. 4.0)
    bool cabin = true;               // M2: walk inside the Stardrifter (false: the classic cockpit camera)
    int clockMode = 0;               // M5-05: 0 game time (warpable), 1 real time (the wall clock since 2026-01-01 UTC drives the sky)
    int aspect = 0;                  // M6-01: 0 square pixels, 1 the original's 4:3 (pixels 1.2 taller)
    bool dither = false;             // M6-01: ordered 2x2 dither instead of smooth palette interpolation
    int hudColor = 0;                // M6-01: 0 green, 1 amber, 2 white

    void clampAll();
    bool load(const std::string& path);
    bool save(const std::string& path) const;
};
