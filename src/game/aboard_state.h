// I-04: life aboard (game/cabin_life.cpp): the photographs' prints on the quarters' wall and the thread that makes them, the home world's
// garden in the hall's beds, the shards' worlds on the library's shelves
#pragma once
#include "core/noise.h"
#include "core/types.h"
#include "surface/surface_types.h"
#include <atomic>
#include <string>
#include <thread>
#include <vector>

// a photograph printed: its number (`screenshot_<n>.png`) and its one tone, its luminance on a 64 x 64 tile (signed: the print's shade
// x (1 + tone / 79) where the frame lights it, `Game::drawPrints`)
struct ShipPrint { int number = -1; GrainTexture tone; };
constexpr int SHIP_PRINTS = 12;   // the gallery's last twelve photographs, the newest first
struct ShipPrints {
    std::vector<ShipPrint> shown;            // on the wall, the newest first
    std::string dir;                         // the photographs' directory they were made from
    bool stale = true;                       // a photograph taken (or none made yet): made again when the quarters are next seen
    std::thread job; std::atomic<bool> busy{false}; std::vector<ShipPrint> made;   // the thread making them, and what it made
    ~ShipPrints() { if (job.joinable()) job.join(); }
};

// the home world's flora (its two silhouettes, their looks and colours, `SurfaceView::buildLooks`' and the world's `vegColor`s) for the
// hall's beds: the guide's home star's first world with flora, else the pinned home's
struct ShipGarden {
    std::string key;            // the star it was made for ("" none yet)
    bool ok = false;
    int fam[2] = {0, 0};
    TreeLook look[2];
    RGB col[2] = {RGB(0.3f, 0.56f, 0.24f), RGB(0.45f, 0.58f, 0.22f)};
    uint64_t seed = 0;
};
