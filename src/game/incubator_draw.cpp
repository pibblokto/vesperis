// Z-05: the incubator's screen (incubator.cpp; lab_state.h), over the room: the twelve flasks (each its species, its row in its stage's colour),
// the chosen one's curve on a log scale (its cells a millilitre on the lab's clock since it was started, a pair's two, its spores dotted), its
// species, its stage and its pace, its pair's turn as the curves show it, its medium (the field chosen lit), what it has come to (woken, a strain,
// its glow); the jars' page (each its world, the chosen one's column as it stands); the chooser of what goes in
#include "game.h"
#include "ui.h"
#include <algorithm>
#include <cmath>

namespace {
uint32_t incColour() { const RGB& c = Game::cabinDevices().row[FACE_INCUBATOR].col; return rgb((int)(c.r * 255), (int)(c.g * 255), (int)(c.b * 255)); }
uint32_t rgbOf(const RGB& c, float k = 1) { return rgb((int)clampd(c.r * k * 255, 0, 255), (int)clampd(c.g * k * 255, 0, 255), (int)clampd(c.b * k * 255, 0, 255)); }
// a stage's colour: growing green, held amber, asleep cyan, dying red, empty dim
uint32_t stageColour(int st) {
    switch (st) {
        case SG_LAG: case SG_GROWING: case SG_COLONY: case SG_CYCLING: return HUD_GREEN;
        case SG_CROWD: case SG_HUNGRY: case SG_WAITING: case SG_GATHERING: case SG_FRUITING: return HUD_AMBER;
        case SG_ASLEEP: case SG_WAKING: return HUD_CYAN;
        case SG_SPORULATING: case SG_ENCYSTING: case SG_DYING: case SG_DEAD: case SG_OUTGROWN: return HUD_RED;
        default: return HUD_DIM;
    }
}
std::string minutes(double s) { return s < 60 ? fmt("%.0f S", std::max(0.0, s)) : fmt("%.0f MIN", s / 60); }
// a line that fits the width, else the shorter one, else none (never cut)
const std::string& fitted(const std::string& a, const std::string& b, int width) {
    static const std::string none;
    if (textWidth(a.c_str()) <= width) return a;
    if (textWidth(b.c_str()) <= width) return b;
    return none;
}
}   // namespace

void Game::renderIncubator() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 218);
    drawText(canvas, 6, 4, "THE INCUBATOR", incColour());
    const char* page = incub.page == 1 ? "THE JARS - TAB THE FLASKS" : "THE FLASKS - TAB THE JARS";
    drawText(canvas, UW - 6 - textWidth(page), 4, page, HUD_DIM);
    if (incub.page == 1) renderIncubatorJars(); else renderIncubatorFlasks();
    if (incub.choosing) renderIncubatorChooser();
}

void Game::renderIncubatorFlasks() {
    // the flasks: a row each in its first species' stage's colour
    const int lx = 6, ly = 16, rowH = 9, listW = 128;
    for (int i = 0; i < INCUBATOR_FLASKS; i++) {
        const Flask& F = lab.flasks[i];
        const int y = ly + i * rowH;
        const bool sel = i == incub.sel;
        if (sel) drawText(canvas, lx, y, ">", incColour());
        drawText(canvas, lx + 8, y, fmt("%d", i + 1).c_str(), sel ? HUD_WHITE : HUD_DIM);
        if (F.n == 0 || !F.made) { drawText(canvas, lx + 24, y, "-", HUD_DIM); continue; }
        const CultureNow now = flaskNow(i);
        const std::string both = F.n > 1 ? F.label[0] + " + " + F.label[1] : F.label[0];
        const std::string& name = fitted(both, F.label[0], listW - 26);
        drawText(canvas, lx + 24, y, name.c_str(), sel ? HUD_WHITE : stageColour(now.one[0].stage));
    }
    // the chosen flask: its curve, its lines
    const Flask& F = lab.flasks[incub.sel];
    const int cx0 = 142, cy0 = 18, cw = UW - 8 - cx0, ch = 80;
    drawRectRGB(canvas, cx0 - 1, cy0 - 1, cx0 + cw, cy0 + ch, rgb(52, 44, 30));
    for (int d = 2; d <= 8; d += 2) {   // the decades
        const int y = cy0 + ch - 1 - (int)std::lround(d * (ch - 1) / 10.0);
        for (int x = cx0; x < cx0 + cw; x += 3) fillRectRGB(canvas, x, y, x, y, rgb(48, 40, 26));
        drawText(canvas, cx0 + 2, y - 3, fmt("%d", d).c_str(), rgb(96, 80, 52));
    }
    const int ty = cy0 + ch + 12;
    if (F.n == 0 || !F.made) {
        drawTextCentered(canvas, cx0 + cw / 2, cy0 + ch / 2 - 4, "AN EMPTY FLASK", HUD_DIM);
        drawText(canvas, cx0, ty, "ENTER PUTS IN A SPECIES", HUD_DIM);
        drawText(canvas, cx0, ty + 9, "SEEN IN A VIAL, OR A", HUD_DIM);
        drawText(canvas, cx0, ty + 18, "FLASK'S CULTURE", HUD_DIM);
    } else {
        double t0 = 0;
        for (const CultureEvent& e : F.events) if (e.kind == CD_START) { t0 = e.t; break; }
        const double span = std::max(600.0, lab.clock - t0);
        const uint32_t cols[2] = {incColour(), HUD_CYAN};
        int lastY[2] = {-1, -1};
        for (int x = 0; x < cw; x++) {
            const double t = t0 + span * x / (cw - 1);
            if (t > lab.clock + 1e-6) break;
            const CultureNow n = cultureAt(F.culture, F.run, t);
            for (int k = 0; k < n.n; k++) {
                auto yOf = [&](double v) { return v >= 1 ? cy0 + ch - 1 - (int)std::lround(std::min(10.0, std::log10(v)) * (ch - 1) / 10.0) : -1; };
                const int y = yOf(n.one[k].cells), ys = yOf(n.one[k].spores);
                if (y >= 0 && lastY[k] >= 0) drawLineRGB(canvas, cx0 + x - 1, lastY[k], cx0 + x, y, cols[k]);
                else if (y >= 0) fillRectRGB(canvas, cx0 + x, y, cx0 + x, y, cols[k]);
                if (ys >= 0 && (x & 1)) fillRectRGB(canvas, cx0 + x, ys, cx0 + x, ys, rgbOf(RGB(0.6f, 0.6f, 0.6f)));   // its spores or cysts, dotted
                lastY[k] = y;
            }
        }
        drawText(canvas, cx0 + cw - 2 - textWidth(minutes(span).c_str()), cy0 + ch + 2, minutes(span).c_str(), HUD_DIM);
        drawText(canvas, cx0, cy0 + ch + 2, "LOG CELLS A ML", HUD_DIM);
        const CultureNow now = flaskNow(incub.sel);
        int y = ty;
        for (int k = 0; k < F.n; k++) {   // each species: its name, its stage and its pace
            const CultureOne& o = now.one[k];
            std::string st = CULTURE_STAGE_NAMES[o.stage];
            const Growth g = growthIn(F.culture.kind[k], F.medium);
            if ((o.stage == SG_GROWING || o.stage == SG_COLONY || o.stage == SG_LAG) && g.mu > 0) st += ", DOUBLING IN " + minutes(std::log(2.0) / g.mu);
            else if (o.fit != GF_GROWS && o.stage != SG_WAITING && o.stage != SG_DEAD && o.stage != SG_EMPTY) st += std::string(" - ") + GROWTH_FIT_NAMES[o.fit];   // why: the medium
            drawText(canvas, cx0, y, fitted(F.label[k] + ": " + st, st, cw).c_str(), cols[k]);
            y += 9;
        }
        if (F.n == 2 && lab.clock - F.run.tAdd > 300) {   // the pair as its curves show it
            std::string rel = CULTURE_PAIR_NAMES[F.run.pair];
            if (F.run.pair == PA_CYCLE && F.run.cycleT >= 0 && lab.clock > F.run.cycleT) rel += ", A TURN OF ABOUT " + minutes(F.run.period);
            if (F.run.pair == PA_CYCLE && !(F.run.cycleT >= 0 && lab.clock > F.run.cycleT)) rel.clear();
            if (F.run.pair == PA_COMPETE && !(F.run.exT >= 0 && lab.clock > F.run.exT)) rel.clear();
            const std::string& r = fitted(rel, CULTURE_PAIR_NAMES[F.run.pair], cw);
            if (!r.empty()) { drawText(canvas, cx0, y, r.c_str(), HUD_WHITE); y += 9; }
        }
        for (int k = 0; k < F.n; k++) {   // what it came to (a line too long for the panel folded onto two)
            std::vector<std::string> said;
            if (F.run.wokeT[k] >= 0 && F.run.wokeT[k] <= lab.clock) {
                const std::string one = F.sleptYears[k] > 0 ? "WOKEN AFTER " + yearsWords(F.sleptYears[k]) : std::string("WOKEN");
                if (textWidth(one.c_str()) <= cw) said.push_back(one); else { said.push_back("WOKEN AFTER"); said.push_back(yearsWords(F.sleptYears[k])); }
            }
            if (F.who[k].strain > 0) said.push_back(fmt("A STRAIN OF ITS OWN (S%d)", F.who[k].strain));
            for (const std::string& l : said) if (y < UH - 30) { drawText(canvas, cx0, y, fitted(l, "", cw).c_str(), HUD_DIM); y += 9; }
        }
        if (now.one[0].glowing || now.one[1].glowing) { drawText(canvas, cx0, y, "GLOWING: ITS CROWD'S OWN LIGHT", HUD_CYAN); y += 9; }
    }
    // the medium, its fields (the one chosen lit)
    {
        const Medium& m = F.medium;
        const int y = UH - 30;
        std::string f[5] = {CULTURE_CELL_NAMES[m.cell], fmt("%.0f C", m.tempC), CULTURE_LIGHT_NAMES[m.light], CULTURE_ENERGY_NAMES[m.energy], CULTURE_SALT_NAMES[m.salt]};
        int x = 6;
        drawText(canvas, x, y, "MEDIUM:", HUD_DIM); x += textWidth("MEDIUM: ");
        for (int k = 0; k < 5; k++) {
            const bool on = k == incub.field;
            drawText(canvas, x, y, f[k].c_str(), F.n == 0 ? HUD_DIM : (on ? HUD_WHITE : HUD_GREEN));
            if (on && F.n > 0) fillRectRGB(canvas, x, y + 8, x + textWidth(f[k].c_str()) - 2, y + 8, incColour());
            x += textWidth(f[k].c_str()) + 8;
        }
    }
    const bool second = ((int)(realTime / 4)) & 1;
    drawTextCentered(canvas, UW / 2, UH - 12, second ? "ENTER PUT IN  W AS ITS WORLD  F FRESH  N NAME" : "<- -> THE MEDIUM  + - CHANGE  BACKSPACE EMPTIES", HUD_DIM);
}

void Game::renderIncubatorJars() {
    const int lx = 6, ly = 16, rowH = 10;
    for (int j = 0; j < JAR_PLACES; j++) {
        const int y = ly + j * rowH;
        const bool sel = j == incub.jar;
        if (sel) drawText(canvas, lx, y, ">", incColour());
        drawText(canvas, lx + 8, y, fmt("%d", j + 1).c_str(), sel ? HUD_WHITE : HUD_DIM);
        if (j >= (int)lab.jars.size()) { drawText(canvas, lx + 24, y, "-", HUD_DIM); continue; }
        const Jar& J = lab.jars[j];
        const std::string w = upper(nameOfKey(Guide::bodyKey(J.sx, J.sy, J.sz, J.body)));
        const std::string& name = fitted(J.name + " (" + w + ")", J.name, 200);
        drawText(canvas, lx + 24, y, name.c_str(), sel ? HUD_WHITE : HUD_GREEN);
    }
    // the chosen jar, as it stands: its water over its silt, its bands coming in
    const int x0 = UW - 70, y0 = 20, w = 40, h = 150;
    drawRectRGB(canvas, x0 - 2, y0 - 2, x0 + w + 1, y0 + h + 1, rgb(90, 100, 104));
    if (incub.jar >= (int)lab.jars.size()) {
        drawTextCentered(canvas, x0 + w / 2, y0 + h / 2 - 4, "EMPTY", HUD_DIM);
        drawText(canvas, 6, UH - 30, "ENTER: A JAR OF A VIAL'S WET SILT, A MAT OR SOIL", HUD_DIM);
    } else {
        Jar& J = lab.jars[incub.jar];
        if (!J.made) jarMake(incub.jar);
        const double age = lab.clock - J.t0;
        const JarNow now = jarAt(J.recipe, age);
        const int waterH = (int)std::lround(h * J.recipe.waterShare);
        for (int y = 0; y < h; y++) {
            RGB c;
            if (y < waterH) c = lerp(J.recipe.water * 0.85f, J.recipe.silt * 0.8f, (float)(0.85 * now.cloud));   // the water, murky while the silt is in it
            else {
                const double d = (y - waterH + 0.5) / (h - waterH);
                c = J.recipe.silt;
                for (size_t b = 0; b < J.recipe.bands.size(); b++) {
                    const JarBand& B = J.recipe.bands[b];
                    if (d >= B.top && d < B.bottom) c = lerp(c, B.col, (float)now.strength[b]);
                }
                c = lerp(c, J.recipe.silt, (float)(0.5 * now.cloud));
            }
            for (int x = 0; x < w; x++) {   // a column lit from within: brighter left of its middle, its edges dark
                const double u = (x + 0.5) / w - 0.42;
                fillRectRGB(canvas, x0 + x, y0 + y, x0 + x, y0 + y, rgbOf(c, (float)(0.62 + 0.5 * std::cos(std::min(1.0, std::fabs(u) * 1.9) * PI * 0.5))));
            }
        }
        fillRectRGB(canvas, x0 + 5, y0 + 3, x0 + 6, y0 + h - 4, rgb(150, 160, 160));   // the glass's highlight
        const std::string st = age >= JAR_SETTLED_S ? "SETTLED" : fmt("SETTLING: %d OF 20 MIN", (int)(age / 60));
        drawText(canvas, 6, UH - 30, fitted(J.name + " - " + st, st, x0 - 14).c_str(), HUD_GREEN);
    }
    drawTextCentered(canvas, UW / 2, UH - 12, "ENTER A JAR FROM A VIAL  BACKSPACE EMPTIES IT", HUD_DIM);
}

// the chooser (R-432: its folders, a world's, the flasks', the collection's; then the open one's rows)
void Game::renderIncubatorChooser() {
    const int x0 = 20, y0 = 30, x1 = UW - 20, y1 = UH - 26;
    drawPanel(canvas, x0, y0, x1, y1, rgb(0, 0, 0), 235, incColour());
    const std::string what = incub.page == 1 ? "A JAR FROM" : fmt("INTO FLASK %d", incub.sel + 1);
    const ChooserPage& P = incub.pages;
    const std::string head = P.open && P.folder < (int)P.folders.size() ? what + ": " + P.folders[P.folder].name : what + ":";
    drawText(canvas, x0 + 8, y0 + 6, fitText(head, x1 - x0 - 16).c_str(), incColour());
    std::vector<std::string> lines;
    for (const IncubatorChoice& c : incub.choices) lines.push_back(c.line);
    drawChooser(canvas, x0 + 6, y0 + 20, x1 - 8, y1, incub.pages, incub.choiceSel, incub.choiceTop, lines, incColour());
}
