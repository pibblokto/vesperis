// Z-05: the incubator's screen (incubator.cpp; lab_state.h), over the room: the twelve flasks (each its species, its row in its stage's colour),
// the chosen one's curve on a log scale (its cells a millilitre on the lab's clock since it was started, a pair's two, its spores dotted), its
// species, its stage and its pace, its pair's turn as the curves show it, its medium (the field chosen lit), what it has come to (woken, a strain,
// its glow); the chooser of what goes in. J-01: what its world genes show (bubbles, its water dyed, a scum, slime, a sediment, clumps, overgrown,
// frozen, genes passed), in words and in a picture of the flask under the list
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
        case SG_SPORULATING: case SG_ENCYSTING: case SG_DYING: case SG_DEAD: case SG_OUTGROWN: case SG_POISONED: return HUD_RED;
        case SG_OVERGROWN: return HUD_AMBER;   // J-01
        case SG_FROZEN: return HUD_CYAN;
        default: return HUD_DIM;
    }
}
std::string minutes(double s) { return s < 60 ? fmt("%.0f S", std::max(0.0, s)) : fmt("%.0f MIN", s / 60); }
RGB mixC(const RGB& a, const RGB& b, double t) { return RGB((float)(a.r + (b.r - a.r) * t), (float)(a.g + (b.g - a.g) * t), (float)(a.b + (b.b - a.b) * t)); }
uint32_t rgbMix(const RGB& a, const RGB& b, double t, float k = 1) { return rgbOf(mixC(a, b, t), k); }
// J-01: what a flask's world genes show, in words (packed into lines as they fit)
std::vector<std::string> showWords(const FlaskShow& S, const std::string* label) {
    std::vector<std::string> w;
    if (S.overgrown) w.push_back(S.tissue ? "OVERGROWN: TISSUE FILLS IT" : "OVERGROWN: PACKED WITH CELLS");
    if (S.frozen) w.push_back("FROZEN SOLID");
    if (S.bubbles > 0.03) w.push_back(std::string("BUBBLES: ") + WORLD_GAS_NAMES[S.gas]);
    if (S.dyed > 0.05) w.push_back(std::string("WATER DYED ") + WORLD_DYE_NAMES[S.dye]);
    if (S.scum > 0.05) w.push_back("SCUM ON TOP");
    if (S.slime > 0.05) w.push_back("THICK WITH SLIME");
    if (S.sediment > 0.05) w.push_back(std::string(WORLD_SHELL_NAMES[S.shell]) + " SEDIMENT");
    if (S.clumps > 0.05) w.push_back("CELLS IN CLUMPS");
    if (S.passedTo >= 0) w.push_back("GENES PASS TO " + label[S.passedTo]);
    return w;
}
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
    drawText(canvas, 6, 4, "INCUBATOR", incColour());
    renderIncubatorFlasks();
    if (incub.choosing) renderIncubatorChooser();
    if (incub.pick) renderIncubatorPick();   // R-440, R-442: what K keeps or N names
    renderKeepPick(incColour());             // R-440: the folder a copy goes in
}

// R-440, R-442: what N names or K keeps of the flask chosen, its rows over the page
void Game::renderIncubatorPick() {
    const int n = (int)incub.pickLines.size(), x0 = 36, x1 = UW - 36, h = n * 9 + 34, y0 = (UH - h) / 2;
    drawPanel(canvas, x0, y0, x1, y0 + h, rgb(0, 0, 0), 235, incColour());
    drawText(canvas, x0 + 8, y0 + 6, fmt(incub.pick == PICK_NAME ? "NAME IN FLASK %d:" : "KEEP A COPY FROM FLASK %d:", incub.sel + 1).c_str(), incColour());
    incub.pickSel = clampi(incub.pickSel, 0, std::max(0, n - 1));
    for (int i = 0; i < n; i++) {
        const int y = y0 + 20 + i * 9;
        if (i == incub.pickSel) drawText(canvas, x0 + 6, y, ">", incColour());
        drawText(canvas, x0 + 14, y, fitText(incub.pickLines[i], x1 - x0 - 22).c_str(), i == incub.pickSel ? HUD_WHITE : HUD_GREEN);
    }
    drawTextCentered(canvas, (x0 + x1) / 2, y0 + h - 11, "ENTER CHOOSE  ESC BACK", HUD_DIM);
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
        const std::string both = F.n > 1 ? F.label[0] + " + " + F.label[1] : F.label[0], first = F.n > 1 ? F.label[0] + " +" : F.label[0];
        std::string name = fitted(both, first, listW - 26);   // B-442: a second never hidden (its first and a plus)
        if (name.empty()) name = fitText(first, listW - 26);   // (J-01: a long name cut, not left out: E10 and on)
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
        drawTextCentered(canvas, cx0 + cw / 2, cy0 + ch / 2 - 4, "EMPTY FLASK", HUD_DIM);
        drawText(canvas, cx0, ty, "ENTER PUTS IN A SPECIES", HUD_DIM);
        drawText(canvas, cx0, ty + 9, "SEEN IN A VIAL, OR A", HUD_DIM);
        drawText(canvas, cx0, ty + 18, "CULTURE IN A FLASK", HUD_DIM);
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
            const double mu = F.culture.kind[k].switches.empty() ? g.mu : o.mu;   // Y-04: a switched line's as its state has it
            if ((o.stage == SG_GROWING || o.stage == SG_COLONY || o.stage == SG_LAG) && mu > 0) st += ", DOUBLING IN " + minutes(std::log(2.0) / mu);
            else if (o.fit != GF_GROWS && o.stage != SG_WAITING && o.stage != SG_DEAD && o.stage != SG_EMPTY && o.stage != SG_FROZEN)
                st += std::string(" - ") + GROWTH_FIT_NAMES[o.fit];   // why: the medium
            std::string line = fitted(F.label[k] + ": " + st, st, cw);
            if (line.empty()) line = fitText(st, cw);   // (J-01: a stage with its doubling too long cut, not left out)
            drawText(canvas, cx0, y, line.c_str(), cols[k]);
            y += 9;
        }
        if (F.n == 2 && lab.clock - F.run.tAdd > 300) {   // the pair as its curves show it
            std::string rel = CULTURE_PAIR_NAMES[F.run.pair];
            if (F.run.pair == PA_CYCLE && F.run.cycleT >= 0 && lab.clock > F.run.cycleT) rel += ", CYCLE OF ABOUT " + minutes(F.run.period);
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
            if (F.who[k].strain > 0) said.push_back(lineEdited(flaskStrainKey(F.who[k])) ? "MADE ON THE GENE EDITOR (" + lineTag(F.who[k]) + ")"
                                                                                        : "NEW STRAIN (" + lineTag(F.who[k]) + ")");
            for (const std::string& l : said) if (y < UH - 30) { drawText(canvas, cx0, y, fitted(l, "", cw).c_str(), HUD_DIM); y += 9; }
        }
        if (now.one[0].glowing || now.one[1].glowing) {   // (Y-04: a glow lit by its switch, its cells')
            const bool cells = (now.one[0].glowing && F.culture.kind[0].glowAny) || (now.one[1].glowing && F.culture.kind[1].glowAny);
            drawText(canvas, cx0, y, cells ? "GLOWING: SWITCHED ON" : "GLOWING: CROWDED", HUD_CYAN); y += 9;
        }
        // J-01: its world genes' looks, packed into lines
        std::string line;
        for (const std::string& w : showWords(flaskShow(incub.sel), F.label)) {
            const std::string next = line.empty() ? w : line + ", " + w;
            if (textWidth(next.c_str()) <= cw) { line = next; continue; }
            if (!line.empty() && y < UH - 30) { drawText(canvas, cx0, y, line.c_str(), HUD_WHITE); y += 9; }
            line = w;
        }
        if (!line.empty() && y < UH - 30) drawText(canvas, cx0, y, fitted(line, "", cw).c_str(), HUD_WHITE);
        renderFlaskPicture(incub.sel, 20, 124);
    }
    // the medium, its fields (the one chosen lit)
    {
        const Medium& m = F.medium;
        const int y = UH - 30;
        std::string f[5] = {CULTURE_CELL_NAMES[m.cell], fmt("%.0f C", m.tempC), CULTURE_LIGHT_NAMES[m.light], CULTURE_ENERGY_NAMES[m.energy], CULTURE_SALT_NAMES[m.salt]};
        if (incub.typing) f[1] = incub.typed + "_ C";   // R-435: its warmth as typed
        int x = 6;
        drawText(canvas, x, y, "MEDIUM:", HUD_DIM); x += textWidth("MEDIUM: ");
        for (int k = 0; k < 5; k++) {
            const bool on = k == incub.field;
            drawText(canvas, x, y, f[k].c_str(), F.n == 0 ? HUD_DIM : (on ? HUD_WHITE : HUD_GREEN));
            if (on && F.n > 0) fillRectRGB(canvas, x, y + 8, x + textWidth(f[k].c_str()) - 2, y + 8, incColour());
            x += textWidth(f[k].c_str()) + 8;
        }
    }
    const int phase = ((int)(realTime / 4)) % 4;   // R-435: a third line, the digits; R-440, R-442: a fourth, N and K
    const char* const LINES[4] = {"<- -> MEDIUM  + - CHANGE  BKSP EMPTY", "ENTER PUT IN  W LIKE ITS WORLD  F FRESH BROTH",
                                  "0-9 TYPE WARMTH  ENTER SET", "N NAME  K KEEP A COPY IN A FOLDER"};
    const std::string keys = incub.typing ? fmt("TYPE WARMTH  ENTER SET  ESC KEEP %.0f C", F.medium.tempC) : std::string(LINES[phase]);
    drawTextCentered(canvas, UW / 2, UH - 12, keys.c_str(), incub.typing ? HUD_AMBER : HUD_DIM);
}

// J-01: the chosen flask as it looks under the list: its glass, its liquid (its culture's tone as cloudy as its crowd, its dye, ice, tissue to the
// neck), its scum, its sediment and clumps, its bubbles rising, its lamp's light about it (the ultraviolet's violet)
void Game::renderFlaskPicture(int i, int x0, int y0) {
    const Flask& F = lab.flasks[clampi(i, 0, INCUBATOR_FLASKS - 1)];
    const int W = 30, H = 38, cx = x0 + W / 2, neck = 5, neckH = 9, base = y0 + H;
    if (F.medium.light != CL_DARK) {   // its lamp's light about it
        const RGB lamp = F.medium.light == CL_UV ? RGB(0.55f, 0.3f, 1.0f) : (F.medium.light == CL_RED ? RGB(1.0f, 0.3f, 0.2f) :
                         (F.medium.light == CL_BLUE ? RGB(0.35f, 0.5f, 1.0f) : RGB(1.0f, 0.95f, 0.8f)));
        blendRectRGB(canvas, x0 - 6, y0 - 2, x0 + W + 6, base + 2, rgbOf(lamp, 0.5f), 40);
    }
    const FlaskShow S = flaskShow(i);
    const CultureNow now = flaskNow(i);
    double best = 0; int pg = 0;
    for (int k = 0; k < now.n; k++) if (now.one[k].cells > best) { best = now.one[k].cells; pg = F.culture.kind[k].s.pigment; }
    const double cloud = clampd((std::log10(std::max(1.0, best)) - 3) / 6, 0, 1);
    const RGB broth(0.86f, 0.8f, 0.62f), water(0.18f, 0.2f, 0.24f);
    RGB tone = pg > 0 ? pigmentColour(pg) : broth;
    const int top = S.overgrown ? y0 + 2 : y0 + neckH + 8;   // its liquid's top (overgrown: up its neck)
    auto halfAt = [&](int y) { return y < y0 + neckH ? neck : neck + (W / 2 - neck) * (y - y0 - neckH) / std::max(1, H - neckH); };
    for (int y = top; y < base; y++) {
        const int h = halfAt(y);
        uint32_t c;
        if (S.frozen) c = rgbMix(water, ICE_COLOUR, 0.85, 0.9f);
        else if (S.overgrown) c = S.tissue ? rgbMix(TISSUE_COLOUR, RGB(0.95f, 0.6f, 0.6f), 0.25 * std::sin(y * 1.7 + x0) + 0.25)
                                           : rgbMix(water, tone, 0.95, 0.85f);
        else if (S.dyed > 0.05) c = rgbMix(mixC(water, tone, 0.6 * cloud), dyeColour(S.dye), std::min(1.0, 0.3 + S.dyed));
        else c = rgbMix(water, tone, 0.15 + 0.7 * cloud);
        fillRectRGB(canvas, cx - h + 1, y, cx + h - 1, y, c);
        if (S.slime > 0.05 && !S.overgrown && ((y + (int)(realTime * 3)) % 7 == 0)) fillRectRGB(canvas, cx - h / 2, y, cx + h / 3, y, rgbMix(water, tone, 0.9, 1.1f));
    }
    if (S.scum > 0.05 && !S.overgrown && !S.frozen) {   // a band of floating cells on top
        const int h = halfAt(top);
        fillRectRGB(canvas, cx - h + 1, top, cx + h - 1, top + 1 + (int)(2 * S.scum), rgbMix(tone, RGB(0.95f, 0.95f, 0.85f), 0.3, 1.1f));
    }
    if (S.sediment > 0.05) {   // its shells settled
        const int n = 1 + (int)std::lround(3 * S.sediment), h = halfAt(base - 1);
        fillRectRGB(canvas, cx - h + 1, base - n, cx + h - 1, base - 1, rgbOf(shellColour(S.shell)));
    }
    if (S.clumps > 0.05 && !S.frozen) for (int k = 0; k < 6; k++) {   // clumps settling
        const int h = halfAt(base - 3), x = cx - h + 2 + (int)((k * 37 + 11) % std::max(1, 2 * h - 4)), y = base - 3 - (k % 3) * 3 - (int)(4 * S.clumps);
        fillRectRGB(canvas, x, y, x + 1, y + 1, rgbMix(tone, RGB(0.1f, 0.1f, 0.1f), 0.4));
    }
    if (S.bubbles > 0.03 && !S.frozen) {   // bubbles rising from the bottom, more of them the more it gives off
        const int n = 2 + (int)std::lround(8 * S.bubbles);
        for (int k = 0; k < n; k++) {
            const double ph = std::fmod(realTime * (0.5 + 0.13 * (k % 4)) + k * 0.37, 1.0);
            const int y = base - 2 - (int)(ph * (base - 2 - top)), h = halfAt(y);
            const int x = cx - h + 2 + (int)((k * 53 + 7) % std::max(1, 2 * h - 4));
            fillRectRGB(canvas, x, y, x, y, rgb(230, 240, 255));
        }
    }
    // its glass
    const uint32_t glass = rgb(120, 140, 150);
    for (int y = y0; y < base; y++) { const int h = halfAt(y); fillRectRGB(canvas, cx - h, y, cx - h, y, glass); fillRectRGB(canvas, cx + h, y, cx + h, y, glass); }
    drawLineRGB(canvas, cx - W / 2, base, cx + W / 2, base, glass);
    drawLineRGB(canvas, cx - neck - 1, y0, cx + neck + 1, y0, glass);
    drawText(canvas, x0 + W + 8, y0 + H / 2 - 3, fmt("FLASK %d", i + 1).c_str(), HUD_DIM);
}

// the chooser (R-432: its folders, a world's, the flasks', the collection's; then the open one's rows)
void Game::renderIncubatorChooser() {
    const int x0 = 20, y0 = 30, x1 = UW - 20, y1 = UH - 26;
    drawPanel(canvas, x0, y0, x1, y1, rgb(0, 0, 0), 235, incColour());
    const std::string what = fmt("INTO FLASK %d", incub.sel + 1);
    const ChooserPage& P = incub.pages;
    const std::string head = P.open && P.folder < (int)P.folders.size() ? what + ": " + P.folders[P.folder].name : what + ":";
    drawText(canvas, x0 + 8, y0 + 6, fitText(head, x1 - x0 - 16).c_str(), incColour());
    std::vector<std::string> lines;
    for (const IncubatorChoice& c : incub.choices) lines.push_back(c.line);
    drawChooser(canvas, x0 + 6, y0 + 20, x1 - 8, y1, incub.pages, incub.choiceSel, incub.choiceTop, lines, incColour());
}
