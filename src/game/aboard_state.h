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

// a photograph printed: its number (`screenshot_<n>.png`), its frame on the wall (R-428), its one tone, its luminance on a 64 x 64 tile
// (signed: the print's shade x (1 + tone / 79) where the frame lights it, `Game::drawPrints`); R-428: a thumbnail in its colours and its
// caption for the wall's screen
struct ShipPrint { int number = -1; GrainTexture tone; int frame = 0; std::vector<uint32_t> thumb; std::string caption; };
constexpr int SHIP_PRINTS = 12, WALL_COLS = 4, WALL_ROWS = 3;   // the wall's frames, four across and three down, the first top left
constexpr int PRINT_THUMB_W = 64, PRINT_THUMB_H = 40;
struct ShipPrints {
    std::vector<ShipPrint> shown;            // on the wall, each in its frame
    std::string dir;                         // the photographs' directory they were made from
    bool stale = true;                       // a photograph taken (or none made yet): made again when the quarters are next seen
    std::thread job; std::atomic<bool> busy{false}; std::vector<ShipPrint> made;   // the thread making them, and what it made
    ~ShipPrints() { if (job.joinable()) job.join(); }
};

// R-428: the wall as the explorer hangs it (E at it: `WALL`), a photograph's number a frame (-1 empty). Until they first hang or take
// down one it shows the gallery's newest twelve (I-04's prints), and from then on what they hung, whatever is taken after (`chosen`;
// the save's `wall` line). Its screen's own: the level (0 the frames, 1 the photographs for one), the frame, the photographs' numbers
// (the newest first), the one chosen and the one read to show it
struct ShipWall {
    bool chosen = false;
    int frame[SHIP_PRINTS] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    int level = 0, sel = 0, pick = 0;
    std::vector<int> photos;
    int shownPick = -1; std::vector<uint32_t> pix; int w = 0, h = 0; bool ok = false; std::string caption;
};

// R-429: the garden's four beds, each trimmed back by the explorer to its starting size (a third of its height, as planted) at a moment of
// the time played, grown again to its full height over an hour of play and no further (the save's `garden` line); nothing to trim under
// `GARDEN_TRIM_MIN`
constexpr int GARDEN_BEDS = 4;
constexpr double GARDEN_GROW_S = 3600, GARDEN_START = 0.35, GARDEN_TRIM_MIN = 0.4;

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
