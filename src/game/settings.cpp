#include "settings.h"
#include <fstream>
#include <sstream>
#include <algorithm>

void Settings::clampAll() {
    mouseSensitivity = std::max(0.2, std::min(3.0, mouseSensitivity));
    fovDeg = std::max(50.0, std::min(100.0, fovDeg));
    masterVolume = std::max(0.0, std::min(1.0, masterVolume));
    windowScale = std::max(2, std::min(6, windowScale));
    renderScale = std::max(1, std::min(4, renderScale));
    mushMode = std::max(0, std::min(3, mushMode));
    shadingMode = std::max(0, std::min(2, shadingMode));
    sprintSpeed = std::max(2.5, std::min(4.0, sprintSpeed));
    clockMode = std::max(0, std::min(1, clockMode));
    aspect = std::max(0, std::min(1, aspect));
    hudColor = std::max(0, std::min(2, hudColor));
}

bool Settings::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    std::getline(f, line);
    if (line.rfind("vesperis-settings", 0) != 0) return false;
    int version = 1;
    { std::istringstream is(line); std::string key; is >> key >> version; }
    while (std::getline(f, line)) {
        std::istringstream is(line);
        std::string key; is >> key;
        int b = 0;
        if (key == "mouse_sensitivity") is >> mouseSensitivity;
        else if (key == "invert_y") { is >> b; invertY = b != 0; }
        else if (key == "fov") is >> fovDeg;
        else if (key == "master_volume") is >> masterVolume;
        else if (key == "scanlines") { is >> b; scanlines = b != 0; }
        else if (key == "window_scale") is >> windowScale;
        else if (key == "render_scale") is >> renderScale;
        else if (key == "mush") is >> mushMode;
        else if (key == "shading") is >> shadingMode;
        else if (key == "crt") { is >> b; crt = b != 0; }
        else if (key == "bloom") { is >> b; bloom = b != 0; }
        else if (key == "sprint_toggle") { is >> b; sprintToggle = b != 0; }
        else if (key == "sprint_speed") is >> sprintSpeed;
        else if (key == "cabin") { is >> b; cabin = b != 0; }
        else if (key == "clock") is >> clockMode;
        else if (key == "aspect") is >> aspect;
        else if (key == "dither") { is >> b; dither = b != 0; }
        else if (key == "hud_color") is >> hudColor;
    }
    // B-310: a file from before the sharper picture keeps its other settings but takes the new look once
    // (4x with the fine mush, smooth terrain shading); it is saved as version 2 afterwards
    if (version < 2) { renderScale = 4; mushMode = 3; shadingMode = 2; }
    clampAll();
    return true;
}

bool Settings::save(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << "vesperis-settings 2\n";
    f << "mouse_sensitivity " << mouseSensitivity << "\n";
    f << "invert_y " << (invertY ? 1 : 0) << "\n";
    f << "fov " << fovDeg << "\n";
    f << "master_volume " << masterVolume << "\n";
    f << "scanlines " << (scanlines ? 1 : 0) << "\n";
    f << "window_scale " << windowScale << "\n";
    f << "render_scale " << renderScale << "\n";
    f << "mush " << mushMode << "\n";
    f << "shading " << shadingMode << "\n";
    f << "crt " << (crt ? 1 : 0) << "\n";
    f << "bloom " << (bloom ? 1 : 0) << "\n";
    f << "sprint_toggle " << (sprintToggle ? 1 : 0) << "\n";
    f << "sprint_speed " << sprintSpeed << "\n";
    f << "cabin " << (cabin ? 1 : 0) << "\n";
    f << "clock " << clockMode << "\n";
    f << "aspect " << aspect << "\n";
    f << "dither " << (dither ? 1 : 0) << "\n";
    f << "hud_color " << hudColor << "\n";
    return true;
}
