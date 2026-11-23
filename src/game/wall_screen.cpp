// R-428 (2026-11-18): the photographs' wall in the quarters, hung by the explorer (the user's: "let you configure photos using in-game photos
// you make"). E at the wall opens its twelve frames as the wall holds them, each print's thumbnail in its colours; the arrows choose a
// frame, Enter opens the photographs (the newest first, one shown large with its caption) and hangs the one chosen, Backspace takes the
// frame's down. Until the first change the wall is I-04's, the gallery's newest twelve; from then on it is the explorer's (`ShipWall`),
// and photographs taken after it no longer move it. The prints are made again on their thread as the quarters are next seen.
#include "game.h"
#include "ui.h"
#include "core/png.h"
#include <fstream>

namespace {
constexpr int CELL_W = 72, CELL_H = 45, CELL_GAP = 6, GRID_Y = 18;   // a frame on the screen (the thumbnail's 16:10), the grid's top
}

void Game::wallEnter() {
    wall.level = 0; wall.sel = clampi(wall.sel, 0, SHIP_PRINTS - 1);
    wall.photos = photographNumbers(shotsDir);
    returnState = GameState::SPACE;
    audio.beep = BEEP_UI;
}

// the photograph a frame shows: the explorer's, else its place among the newest twelve (-1 none)
int Game::wallNumber(int k) const {
    if (k < 0 || k >= SHIP_PRINTS) return -1;
    if (wall.chosen) return wall.frame[k];
    return k < (int)wall.photos.size() ? wall.photos[k] : -1;
}

// the wall made the explorer's at their first change: the frames as they show (the newest twelve)
void Game::wallOwn() {
    if (wall.chosen) return;
    for (int k = 0; k < SHIP_PRINTS; k++) wall.frame[k] = wallNumber(k);
    wall.chosen = true;
}

void Game::updateWall(const Input& in, double dt, double realDt) {
    (void)dt; (void)realDt;
    if (wall.level == 0) {
        if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); return; }
        int col = wall.sel % WALL_COLS, row = wall.sel / WALL_COLS;
        if (in.wasPressed(KEY_LEFT)) col = (col + WALL_COLS - 1) % WALL_COLS;
        if (in.wasPressed(KEY_RIGHT)) col = (col + 1) % WALL_COLS;
        if (in.wasPressed(KEY_UP)) row = (row + WALL_ROWS - 1) % WALL_ROWS;
        if (in.wasPressed(KEY_DOWN)) row = (row + 1) % WALL_ROWS;
        wall.sel = row * WALL_COLS + col;
        if (in.wasPressed(KEY_BACKSPACE) || in.wasPressed(KEY_DELETE)) {
            if (wallNumber(wall.sel) < 0) { refuse("FRAME IS EMPTY", 2); return; }
            wallOwn(); wall.frame[wall.sel] = -1; prints.stale = true;
            status("TAKEN DOWN", 2); audio.beep = BEEP_UI;
            return;
        }
        if (enterKey(in)) {
            if (wall.photos.empty()) { refuse("NO PHOTOGRAPHS YET - P TAKES ONE", 3); return; }
            wall.pick = 0;
            for (size_t i = 0; i < wall.photos.size(); i++) if (wall.photos[i] == wallNumber(wall.sel)) wall.pick = (int)i;   // on the one it holds
            wall.level = 1; wall.shownPick = -1; audio.beep = BEEP_UI;
        }
        return;
    }
    if (in.wasPressed(KEY_ESCAPE)) { wall.level = 0; audio.beep = BEEP_UI; return; }
    const int n = (int)wall.photos.size();
    if (n == 0) { wall.level = 0; return; }
    menuStep(in, wall.pick, n, true);
    if (enterKey(in)) {
        wallOwn(); wall.frame[wall.sel] = wall.photos[wall.pick]; prints.stale = true;
        wall.level = 0; status(fmt("PHOTOGRAPH %d HUNG", wall.photos[wall.pick]), 3); audio.beep = BEEP_UI;
    }
}

void Game::renderWall() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 225);
    const int S = FB_SCALE;
    if (wall.level == 1) {   // the photographs for a frame: the one chosen large, as the gallery shows it
        const int n = (int)wall.photos.size();
        wall.pick = clampi(wall.pick, 0, std::max(0, n - 1));
        drawText(canvas, 6, 4, fmt("PHOTOGRAPH FOR FRAME %d", wall.sel + 1).c_str(), HUD_AMBER);
        if (n == 0) return;
        if (wall.shownPick != wall.pick) {
            wall.shownPick = wall.pick; wall.caption.clear();
            const std::string file = fmt("%s/screenshot_%d.png", shotsDir.c_str(), wall.photos[wall.pick]);
            wall.ok = readPNG(file.c_str(), wall.pix, wall.w, wall.h) && wall.w > 0;
            std::ifstream cap(file.substr(0, file.size() - 4) + ".txt");
            if (cap) std::getline(cap, wall.caption);
        }
        const int fx = 48, fy = 16, fw = 224, fh = 140;
        if (wall.ok)
            for (int y = 0; y < fh * S; y++)
                for (int x = 0; x < fw * S; x++)
                    canvas.px[(size_t)(fy * S + y) * FBW + fx * S + x] = wall.pix[(size_t)(y * wall.h / (fh * S)) * wall.w + x * wall.w / (fw * S)];
        else drawTextCentered(canvas, UW / 2, fy + fh / 2, "CANNOT READ THIS FILE", HUD_RED);
        drawRectRGB(canvas, fx - 1, fy - 1, fx + fw, fy + fh, HUD_DIM);
        drawText(canvas, fx, fy + fh + 4, fmt("PHOTOGRAPH %d   %d OF %d", wall.photos[wall.pick], wall.pick + 1, n).c_str(), HUD_AMBER);
        drawText(canvas, fx, fy + fh + 13, trunc(wall.caption, 37).c_str(), HUD_GREEN);
        drawTextCentered(canvas, UW / 2, UH - 10, "LEFT/RIGHT BROWSE  ENTER HANG  ESC BACK", HUD_DIM);
        return;
    }
    drawText(canvas, 6, 4, "PHOTOGRAPHS", HUD_AMBER);
    const char* head = wall.chosen ? "AS YOU HUNG THEM" : "NEWEST TWELVE";
    drawText(canvas, UW - 6 - textWidth(head), 4, head, HUD_DIM);
    const int x0 = (UW - WALL_COLS * CELL_W - (WALL_COLS - 1) * CELL_GAP) / 2;
    for (int k = 0; k < SHIP_PRINTS; k++) {
        const int x = x0 + (k % WALL_COLS) * (CELL_W + CELL_GAP), y = GRID_Y + (k / WALL_COLS) * (CELL_H + CELL_GAP), num = wallNumber(k);
        const ShipPrint* p = nullptr;
        for (const ShipPrint& sp : prints.shown) if (sp.frame == k && sp.number == num) p = &sp;   // its print, once made
        if (p && p->thumb.size() == (size_t)PRINT_THUMB_W * PRINT_THUMB_H) {
            for (int yy = 0; yy < CELL_H * S; yy++)
                for (int xx = 0; xx < CELL_W * S; xx++)
                    canvas.px[(size_t)(y * S + yy) * FBW + x * S + xx] = p->thumb[(size_t)(yy * PRINT_THUMB_H / (CELL_H * S)) * PRINT_THUMB_W + xx * PRINT_THUMB_W / (CELL_W * S)];
        } else drawTextCentered(canvas, x + CELL_W / 2, y + CELL_H / 2 - 3, num >= 0 ? "..." : "EMPTY", HUD_DIM);
        drawRectRGB(canvas, x - 1, y - 1, x + CELL_W, y + CELL_H, k == wall.sel ? HUD_AMBER : HUD_DIM);
    }
    const int num = wallNumber(wall.sel);
    std::string line = "EMPTY FRAME";
    if (num >= 0) {
        line = fmt("PHOTOGRAPH %d", num);
        for (const ShipPrint& sp : prints.shown) if (sp.frame == wall.sel && sp.number == num && !sp.caption.empty()) line += "  " + sp.caption;
    }
    drawText(canvas, 6, GRID_Y + WALL_ROWS * (CELL_H + CELL_GAP) + 1, trunc(line, 51).c_str(), num >= 0 ? HUD_GREEN : HUD_DIM);
    drawTextCentered(canvas, UW / 2, UH - 10, "ENTER HANG  BACKSPACE TAKE DOWN  ESC CLOSE", HUD_DIM);
}

// U-04: the wall's commands (the controller's first page), its level's
void Game::wallPad(PadMenu& m) const {
    m.title = "PHOTOGRAPHS";
    if (wall.level == 0) { m.item("HANG A PHOTOGRAPH HERE", KEY_ENTER); m.item("TAKE DOWN", KEY_BACKSPACE); m.item("CLOSE", KEY_ESCAPE); return; }
    m.item("HANG", KEY_ENTER); m.item("NEXT", KEY_RIGHT); m.item("PREVIOUS", KEY_LEFT); m.item("BACK", KEY_ESCAPE);
}
