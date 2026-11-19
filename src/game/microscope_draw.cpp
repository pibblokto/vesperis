// Z-04: the microscope's picture (microscope.cpp; lab_state.h): a camera entered, the slide drawn by life/morph_draw in the framebuffer with every
// bank the slide's (its way of seeing's ramps; neither the space view nor the cabin drawn), the mush as the scenes', the HUD on the canvas: its
// camera's lines (the way and the magnification, the focus, the stage's warmth, the preparation, the field, the slide's clock), the crosshair
// and the species under it, the spot and the food's drop, the scale bar; the cold store's list on its first page, a species' sheet on Tab
#include "game.h"
#include "ui.h"
#include <algorithm>
#include <cmath>

namespace {
uint32_t scopeColour() { const RGB& c = Game::cabinDevices().row[FACE_MICROSCOPE].col; return rgb((int)(c.r * 255), (int)(c.g * 255), (int)(c.b * 255)); }
// a nice length for a scale bar about a fifth of the view
double barUm(double viewUm) {
    static const double STEPS[] = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000};
    double best = STEPS[0];
    for (double s : STEPS) if (s <= viewUm * 0.22) best = s;
    return best;
}
std::string barWords(double um) { return um < 1 ? fmt("%g UM", um) : (um >= 1000 ? fmt("%.0f MM", um / 1000) : fmt("%.0f UM", um)); }
const char* const WAY_WORDS[WS_COUNT] = {"BRIGHT", "PHASE", "DARK", "GLOW", "ELECTRON", "SECTION"};
}   // namespace

void Game::renderMicroscope() {
    const bool slide = scope.page == 1 && ((scope.vial >= 0 && scope.vial < (int)coldStore.size()) || scope.flask >= 0 || (scope.kept >= 0 && scope.kept < (int)lab.kept.size()));
    Slide v;
    v.seeing = slide ? scope.seeing : WS_BRIGHT;
    v.umPerPx = umPerPxAt(microscopeMag());
    v.cx = scope.cx; v.cy = scope.cy; v.t = scope.t; v.lamp = 1;
    v.turn = scope.turn; v.tilt = scope.tilt; v.stage = 0.35;
    v.seed = slide ? scope.mount.seed : 1; v.lod = true; v.step = FB_SCALE >= 3 ? 2 : 1;
    if (slide) {
        const SlideState st = slideStateAt(scope.mount, scope.events, scope.t);
        v.fieldAngle = st.field;
        if (st.spot) { v.spotX = st.spotX; v.spotY = st.spotY; v.spotR = st.spotR; }
    }
    slidePalette(fb.pal, v.seeing, v.lamp);
    if (slide) drawSlide(fb, v, scope.shown); else drawSlide(fb, v, {});
    fb.mush(2);
    fb.toRGB(rgbBuf.data(), 1.0, settings.dither);
    if (!slide) renderMicroscopeList();
    else if (!photoMode) { renderMicroscopeHUD(); if (scope.sheet) renderMicroscopeSheet(); }
    drawCommonHUD(false);   // the status; no warp's line (the slide runs on real seconds, its column there)
}

// the slide's camera: CAM 06 MICROSCOPE, the way and the magnification, the focus's depth; the vial and its world, the slide's clock; the stage's
// warmth, the preparation, the field; the crosshair and the species under it, a tracked cell's line; the spot and the food's drop where they lie;
// the scale bar
void Game::renderMicroscopeHUD() {
    const uint32_t fc = HUD_WHITE, fd = HUD_DIM, own = scopeColour();
    const double mag = microscopeMag(), um = umPerPxAt(mag);
    drawVisor(fd);
    drawTextShadow(canvas, 12, 10, "CAM 06 MICROSCOPE", own, HUD_SHADOW);
    drawTextShadow(canvas, 12, 18, fmt("%s X%s", WAY_WORDS[scope.seeing], thousands(std::round(mag)).c_str()).c_str(), HUD_CYAN, HUD_SHADOW);
    if (scope.seeing != WS_ELECTRON && scope.seeing != WS_SECTION) drawTextShadow(canvas, 12, 26, fmt("FOCUS %.1f UM", scope.focus).c_str(), fd, HUD_SHADOW);
    const SlideState st = slideStateAt(scope.mount, scope.events, scope.t);
    {   // the stage's warmth (amber while it changes), the preparation, the magnet's field
        const bool moving = std::fabs(st.stageC - (st.warming ? scope.mount.stageC + 30 : scope.mount.stageC)) > 0.5;
        std::string s = fmt("STAGE %.0f C", st.stageC) + (st.warming ? (moving ? " WARMING" : " WARM") : (moving ? " COOLING" : ""));
        drawTextShadow(canvas, 12, 34, s.c_str(), st.warming || moving ? HUD_AMBER : fd, HUD_SHADOW);
        const int p = scope.mount.prep;
        const std::string prep = scope.mount.section ? "A THIN SECTION OF THE ROCK" :
                                 (p > 0 ? fmt("CONCENTRATED X%s", thousands(std::pow(10.0, p)).c_str()) : (p < 0 ? fmt("DILUTED 1:%s", thousands(std::pow(10.0, -p)).c_str()) : "A DROP AS TAKEN"));
        drawTextShadow(canvas, 12, 42, prep.c_str(), fd, HUD_SHADOW);
        if (st.magnet) {
            const int ax = 12 + textWidth("FIELD") + 14, ay = 53, r = 7;
            const double c = std::cos(st.field), sn = std::sin(st.field);
            const int bx = ax + (int)std::lround(c * r), by = ay + (int)std::lround(sn * r);
            drawLineRGB(canvas, ax - (int)std::lround(c * r), ay - (int)std::lround(sn * r), bx, by, HUD_AMBER);
            for (int k : {-1, 1}) drawLineRGB(canvas, bx, by, bx - (int)std::lround(std::cos(st.field + k * 0.5) * 4), by - (int)std::lround(std::sin(st.field + k * 0.5) * 4), HUD_AMBER);
            drawTextShadow(canvas, 12, 50, "FIELD", fd, HUD_SHADOW);
        }
    }
    {   // the right: the epoch, the vial's world and its line, the slide's clock
        const std::string ep = epocString();
        drawTextShadow(canvas, UW - 12 - textWidth(ep.c_str()), 10, ep.c_str(), HUD_GREEN, HUD_SHADOW);
        // a vial's world (a name of 28 at most, the left column's line there 16) and its line; Z-05: a flask's number and its stage
        drawTextShadow(canvas, UW - 12 - textWidth(scope.title.c_str()), 18, scope.title.c_str(), fd, HUD_SHADOW);
        drawTextShadow(canvas, UW - 12 - textWidth(scope.line.c_str()), 26, scope.line.c_str(), fd, HUD_SHADOW);
        const std::string clock = fmt("T+%02d:%02d", (int)(scope.t / 60), (int)std::fmod(scope.t, 60.0));
        drawTextShadow(canvas, UW - 12 - textWidth(clock.c_str()), 34, clock.c_str(), fd, HUD_SHADOW);
    }
    // the spot of light and the food's drop where they lie (the light ways)
    if (scope.seeing != WS_ELECTRON) {
        auto ring = [&](double x, double y, double r, uint32_t col) {
            const double sx = UW * 0.5 + (x - scope.cx) / um, sy = UH * 0.5 + (y - scope.cy) / um, sr = std::max(3.0, r / um);
            if (sx < -sr || sx > UW + sr || sy < -sr || sy > UH + sr) return;
            for (int k = 0; k < 32; k += 2) {
                const double a0 = k * TAU / 32, a1 = (k + 1) * TAU / 32;
                drawLineRGB(canvas, (int)std::lround(sx + std::cos(a0) * sr), (int)std::lround(sy + std::sin(a0) * sr), (int)std::lround(sx + std::cos(a1) * sr),
                            (int)std::lround(sy + std::sin(a1) * sr), col);
            }
        };
        if (st.spot) ring(st.spotX, st.spotY, st.spotR, rgb(255, 240, 170));
        if (st.food) ring(st.foodX, st.foodY, st.foodR, HUD_GREEN);
    }
    {   // the crosshair, cyan while tracking
        const int cx = UW / 2, cy = UH / 2;
        const uint32_t col = scope.tracking ? HUD_CYAN : (scope.under >= 0 ? fc : fd);
        drawLineRGB(canvas, cx - 9, cy, cx - 4, cy, col); drawLineRGB(canvas, cx + 4, cy, cx + 9, cy, col);
        drawLineRGB(canvas, cx, cy - 9, cx, cy - 4, col); drawLineRGB(canvas, cx, cy + 4, cx, cy + 9, col);
    }
    {   // the scale bar, its length beside it
        const double viewUm = UW * um, bar = barUm(viewUm);
        const int bx = 12, by = UH - 21, bw = std::max(2, (int)std::lround(bar / um));
        fillRectRGB(canvas, bx - 1, by - 1, bx + bw + 1, by + 2, rgb(0, 0, 0));
        fillRectRGB(canvas, bx, by, bx + bw, by + 1, fc);
        drawTextShadow(canvas, bx + bw + 5, by - 3, barWords(bar).c_str(), fc, HUD_SHADOW);
    }
    // the species under the crosshair (its name or UNNAMED, its form), a tracked cell's
    const int sp = microscopeSpeciesUnder();
    if (sp >= 0) {   // one line, or the name over the form where both have no room in 52 (never cut)
        const uint32_t col = scope.tracking ? HUD_CYAN : HUD_GREEN;
        const std::string name = speciesName(sp), form = formWords(sp, scope.shown[scope.under].form), l = name + " - " + form;
        if (l.size() <= 52) drawTextShadow(canvas, UW / 2 - textWidth(l.c_str()) / 2, UH - 34, l.c_str(), col, HUD_SHADOW);
        else {
            drawTextShadow(canvas, UW / 2 - textWidth(name.c_str()) / 2, UH - 42, name.c_str(), col, HUD_SHADOW);
            drawTextShadow(canvas, UW / 2 - textWidth(form.c_str()) / 2, UH - 34, form.c_str(), col, HUD_SHADOW);
        }
    }
    if (realTime >= statusUntil) {   // the keys, two lines in turn (the controller's are its Y page)
        const char* keys = scope.tracking ? "ENTER LETS IT GO  TAB ITS SHEET  N NAMES IT"
                                          : (std::fmod(realTime, 8.0) < 4 ? "1-6 WAYS  +/- ZOOM  [ ] FOCUS  ENTER TRACKS" : "L LIGHT  M MAGNET  H WARMTH  F FOOD  TAB SHEET");
        drawTextCentered(canvas, UW / 2, UH - 10, keys, fd);
    }
}

// R-432: the lists: the tabs (the vials, the cultures) over the folders (renderMicroscopeFolders) or an open folder's rows under its name: a vial's
// line and the activity it read; a flask's culture and its stage (Z-05); a kept culture's words and its world (R-433); the one on the stage in cyan
void Game::renderMicroscopeList() {
    const int top = 14, bottom = UH - 16, rowH = 9;
    drawPanel(canvas, 8, top, UW - 8, bottom, rgb(0, 0, 0), 215);
    drawTextShadow(canvas, 16, top + 6, "CAM 06 MICROSCOPE", scopeColour(), HUD_SHADOW);
    {   // the tabs, the one shown in amber
        const char* a = scope.tab == 0 ? "THE VIALS" : "VIALS";
        const char* b = scope.tab == 1 ? "THE CULTURES" : "CULTURES";
        const int x = UW - 16 - textWidth(b);
        drawText(canvas, x, top + 6, b, scope.tab == 1 ? HUD_AMBER : HUD_DIM);
        drawText(canvas, x - 12 - textWidth(a), top + 6, a, scope.tab == 0 ? HUD_AMBER : HUD_DIM);
    }
    const std::vector<LabFolder> F = scope.tab == 0 ? vialFolders() : cultureFolders();
    const int nf = (int)F.size();
    if (scope.open && (scope.folder >= nf || F[scope.folder].rows.empty())) scope.open = false;
    if (!scope.open) { renderMicroscopeFolders(F); return; }
    const LabFolder& f = F[scope.folder];
    drawText(canvas, 16, top + 18, fitText(f.name + " - " + f.line, UW - 32).c_str(), HUD_AMBER);
    const int n = (int)f.rows.size(), rows = (bottom - 14 - (top + 32)) / rowH;
    scope.sel = clampi(scope.sel, 0, n - 1);
    if (scope.sel < scope.top) scope.top = scope.sel;
    if (scope.sel >= scope.top + rows) scope.top = scope.sel - rows + 1;
    scope.top = clampi(scope.top, 0, std::max(0, n - rows));
    for (int i = scope.top; i < n && i < scope.top + rows; i++) {
        const int y = top + 32 + (i - scope.top) * rowH, r = f.rows[i];
        const bool sel = i == scope.sel;
        if (sel) drawText(canvas, 12, y, ">", HUD_AMBER);
        std::string line, right; uint32_t rc = HUD_DIM; bool staged = false;
        if (f.id == FOLDER_WORLD) {   // a vial: its line and its activity
            const Vial& v = coldStore[r];
            line = fmt("%s, %.0f C", v.name.c_str(), std::fabs(v.readingC) < 0.5 ? 0.0 : v.readingC);
            right = VIAL_ACTIVITY_NAMES[v.activity]; rc = v.activity == VA_STRONG ? HUD_AMBER : (v.activity == VA_FAINT ? HUD_GREEN : HUD_DIM);
            staged = r == scope.vial;
        } else if (f.id == FOLDER_FLASKS) {   // Z-05: a flask's culture, its species and its stage
            const Flask& L = lab.flasks[r];
            line = fmt("FLASK %d: ", r + 1) + L.label[0] + (L.n > 1 ? " + " + L.label[1] : std::string());   // the names that fit (never cut)
            if (20 + textWidth(line.c_str()) > UW - 14) line = fmt("FLASK %d: ", r + 1) + L.label[0];
            if (20 + textWidth(line.c_str()) > UW - 14) line = fmt("FLASK %d", r + 1);
            right = CULTURE_STAGE_NAMES[flaskNow(r).one[0].stage];
            staged = r == scope.flask;
        } else {   // R-433: a kept culture, its world
            line = fitText(keptName(r), UW - 34);
            const KeptCulture& c = lab.kept[r];
            right = upper(nameOfKey(Guide::bodyKey(c.who.sx, c.who.sy, c.who.sz, c.who.body)));
            staged = r == scope.kept;
        }
        drawText(canvas, 20, y, line.c_str(), sel ? HUD_WHITE : (staged ? HUD_CYAN : HUD_GREEN));
        if (20 + textWidth(line.c_str()) + 12 <= UW - 14 - textWidth(right.c_str())) drawText(canvas, UW - 14 - textWidth(right.c_str()), y, right.c_str(), rc);   // never cut
    }
    const char* keys = f.id == FOLDER_WORLD ? "ENTER ON THE STAGE  BACKSPACE EMPTIES IT  ESC BACK"
                     : (f.id == FOLDER_FLASKS ? "ENTER A DROP OF IT ON THE STAGE  ESC BACK" : "ENTER ON THE STAGE  M MOVES  BACKSPACE DISCARDS");
    drawTextCentered(canvas, UW / 2, bottom - 10, keys, HUD_DIM);
    if (!scope.moving) return;
    // R-433: M's chooser over the list: UNSORTED and the explorer's folders, the culture's own marked
    const int n2 = 1 + (int)lab.folders.size(), x0 = 70, x1 = UW - 70, h = std::min(10, n2) * rowH + 30, y0 = (UH - h) / 2;
    drawPanel(canvas, x0, y0, x1, y0 + h, rgb(0, 0, 0), 235, scopeColour());
    drawText(canvas, x0 + 8, y0 + 6, "MOVE IT TO:", scopeColour());
    scope.moveSel = clampi(scope.moveSel, 0, n2 - 1);
    const int first = std::max(0, std::min(scope.moveSel - 5, n2 - 10));
    const int kf = lab.kept[f.rows[scope.sel]].folder;
    for (int i = first; i < n2 && i < first + 10; i++) {
        const int y = y0 + 18 + (i - first) * rowH;
        const std::string name = i == 0 ? std::string("UNSORTED") : lab.folders[i - 1].name;
        const bool here = (i == 0 ? 0 : lab.folders[i - 1].id) == kf;
        if (i == scope.moveSel) drawText(canvas, x0 + 6, y, ">", scopeColour());
        drawText(canvas, x0 + 14, y, (name + (here ? "  (HERE)" : "")).c_str(), i == scope.moveSel ? HUD_WHITE : HUD_GREEN);
    }
}

// R-432: the folders: the vials' a world each (its name and where it is; its vials, how many read STRONG), the cultures' a line each (the
// flasks, UNSORTED, the explorer's: its name and its count); the head's count, the keys of the folder chosen
void Game::renderMicroscopeFolders(const std::vector<LabFolder>& F) {
    const int top = 14, bottom = UH - 16, nf = (int)F.size();
    if (scope.tab == 0) {
        if (nf == 0) {
            drawText(canvas, 16, top + 18, "THE COLD STORE IS EMPTY", HUD_AMBER);
            drawTextCentered(canvas, UW / 2, top + 60, "NO VIALS IN THE COLD STORE", HUD_DIM);
            drawTextCentered(canvas, UW / 2, top + 72, "ON THE GROUND O TAKES A SAMPLE", HUD_DIM);
            drawTextCentered(canvas, UW / 2, bottom - 10, "TAB THE CULTURES  ESC BACK TO THE LABORATORY", HUD_DIM);
            return;
        }
        drawText(canvas, 16, top + 18, fmt("THE COLD STORE: %d VIAL%s FROM %d WORLD%s", (int)coldStore.size(), coldStore.size() == 1 ? "" : "S", nf, nf == 1 ? "" : "S").c_str(), HUD_AMBER);
    } else {
        const int kept = (int)lab.kept.size();
        drawText(canvas, 16, top + 18, (kept == 0 ? std::string("THE COLLECTION: NOTHING KEPT YET") : fmt("THE COLLECTION: %d CULTURE%s KEPT", kept, kept == 1 ? "" : "S")).c_str(), HUD_AMBER);
    }
    const int rowH = scope.tab == 0 ? 19 : 9, rows = (bottom - 14 - (top + 32)) / rowH;
    scope.folder = clampi(scope.folder, 0, nf - 1);
    if (scope.folder < scope.folderTop) scope.folderTop = scope.folder;
    if (scope.folder >= scope.folderTop + rows) scope.folderTop = scope.folder - rows + 1;
    scope.folderTop = clampi(scope.folderTop, 0, std::max(0, nf - rows));
    for (int i = scope.folderTop; i < nf && i < scope.folderTop + rows; i++) {
        const LabFolder& f = F[i];
        const int y = top + 32 + (i - scope.folderTop) * rowH;
        const bool sel = i == scope.folder;
        if (sel) drawText(canvas, 12, y, ">", HUD_AMBER);
        const uint32_t col = sel ? HUD_WHITE : (f.foreign ? HUD_CYAN : HUD_GREEN);
        if (scope.tab == 0) {
            const std::string where = labFolderWhere(f.key);
            drawText(canvas, 20, y, (trunc(f.name, std::max(8, 49 - (int)where.size())) + where).c_str(), col);
            drawText(canvas, 20, y + 9, f.line.c_str(), HUD_DIM);
        } else {
            drawText(canvas, 20, y, fitText(f.name, UW - 54 - textWidth(f.line.c_str())).c_str(), col);
            drawText(canvas, UW - 14 - textWidth(f.line.c_str()), y, f.line.c_str(), HUD_DIM);
        }
    }
    const LabFolder& f = F[scope.folder];
    const char* keys = scope.tab == 0 ? "ENTER OPENS A WORLD  TAB CULTURES  ESC BACK"
                     : (f.id <= 0 ? "ENTER OPENS  N NEW FOLDER  TAB VIALS  ESC BACK"
                                  : (f.rows.empty() ? "ENTER OPENS  N NEW  R RENAMES  BACKSPACE REMOVES" : "ENTER OPENS  N NEW  R RENAMES  ESC BACK"));
    drawTextCentered(canvas, UW / 2, bottom - 10, keys, HUD_DIM);
}

// a species' sheet, over the slide's right: its lines as seen (sheetLines), wrapped to the panel
void Game::renderMicroscopeSheet() {
    const std::vector<std::string> lines = sheetLines(scope.sheetSpecies);
    if (lines.empty()) return;
    std::vector<std::pair<std::string, uint32_t>> out;
    for (size_t i = 0; i < lines.size(); i++) {
        const uint32_t col = i == 0 ? HUD_WHITE : (i == 1 ? HUD_DIM : (lines[i].rfind("MORE IN:", 0) == 0 ? HUD_DIM : HUD_GREEN));
        for (const std::string& w : wrapText(lines[i], 26)) out.push_back({w, col});
    }
    const int x0 = UW - 172, y0 = 44, h = (int)out.size() * 9 + 10;
    drawPanel(canvas, x0, y0, UW - 8, y0 + h, rgb(0, 0, 0), 205, scopeColour());
    for (size_t i = 0; i < out.size(); i++) drawText(canvas, x0 + 6, y0 + 6 + (int)i * 9, out[i].first.c_str(), out[i].second);
}
