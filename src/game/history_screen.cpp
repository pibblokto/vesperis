// H-06: the decoder reads a history. A world's records on three pages (game/shards_screen.cpp switches them with Tab): the records by their
// years and kinds, the timeline of the people's eras with the records held on it (C-12's timeline grown), and the history as the records
// read so far tell it (peoples/legends.h: the events told in the history's order under their eras, each person only as a record named them,
// the age they reached once the records told it, the last recording closing it). The history page is read through the language as the
// records are: a word not learnt yet in their tongue, a name kept, so the explorer's legends of a people clear as its tongue is learnt.
#include "game.h"
#include "ui.h"
#include "peoples/records.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>

namespace {
const int TL_X0 = 12, TL_X1 = UW - 12;          // the timeline's width
const int TL_BASE = 126;                         // the records' stacks stand on it
const int HIST_TOP = 46, HIST_ROWS = 16;         // the history page's rows
const int HIST_TEXT_X = 54, HIST_MAX_X = UW - 14;
const uint32_t KIND_COL[RC_COUNT] = {rgb(120, 230, 150), rgb(110, 170, 235), rgb(215, 140, 225)};   // read: a text green, a piece blue, a chart violet
const uint32_t KIND_DIM[RC_COUNT] = {rgb(45, 95, 60), rgb(45, 70, 110), rgb(90, 55, 100)};          // held, not read
std::string shownWord(const DecodedWord& w) { return upper(w.pre + (w.known ? w.ours : w.theirs) + w.post); }
int wordsWidth(const std::vector<DecodedWord>& ws) { int x = 0; for (size_t i = 0; i < ws.size(); i++) x += textWidth(shownWord(ws[i]).c_str()) + (i ? FONT_ADV : 0); return x; }
// a line read as the records are: a name amber, a word read in `known`'s colour, one not learnt yet dim in their tongue; returns where it ends
int drawWords(RGBCanvas& c, int x, int y, const std::vector<DecodedWord>& ws, uint32_t known) {
    for (size_t i = 0; i < ws.size(); i++) {
        const std::string sw = shownWord(ws[i]);
        drawText(c, x, y, sw.c_str(), ws[i].name ? HUD_AMBER : (ws[i].known ? known : HUD_DIM));
        x += textWidth(sw.c_str()) + (i + 1 < ws.size() ? FONT_ADV : 0);
    }
    return x;
}
// an era's years as the history page heads them: the old time before the count, the last era open until the last recording is read
std::string eraYears(const HistoryEra& E, bool lastEra, bool ended) {
    if (E.end < 1) return "BEFORE YEAR 1";
    if (lastEra && !ended) return "FROM YEAR " + thousands(std::max(1, E.start));
    return "YEARS " + thousands(std::max(1, E.start)) + " TO " + thousands(E.end);
}
}   // namespace

int recordKindOf(const Shard& s) { return s.music ? RC_MUSIC : (s.chart ? RC_CHART : RC_TEXT); }
void drawRecordKindGlyph(RGBCanvas& fb, int x, int y, int kind, uint32_t col) {   // a page, a note, a star: five pixels wide, seven high
    if (kind == RC_TEXT) { drawRectRGB(fb, x, y, x + 4, y + 6, col); fillRectRGB(fb, x + 1, y + 2, x + 3, y + 2, col); fillRectRGB(fb, x + 1, y + 4, x + 3, y + 4, col); }
    else if (kind == RC_MUSIC) { fillRectRGB(fb, x + 3, y, x + 3, y + 5, col); fillRectRGB(fb, x + 4, y + 1, x + 4, y + 1, col); fillRectRGB(fb, x + 1, y + 5, x + 3, y + 6, col); }
    else { fillRectRGB(fb, x + 2, y + 1, x + 2, y + 5, col); fillRectRGB(fb, x, y + 3, x + 4, y + 3, col);
           for (int d : {-1, 1}) { fillRectRGB(fb, x + 2 + d, y + 2, x + 2 + d, y + 2, col); fillRectRGB(fb, x + 2 + d, y + 4, x + 2 + d, y + 4, col); } }
}

// `told`, the history page's rows and the timeline's eras for the open world (no-op while valid)
// what the open world's read records told, the history page's rows and the timeline's eras (again after every reading: `toldValid`)
void Game::ensureTold() {
    if (toldValid) return;
    toldValid = true;
    const auto t0 = std::chrono::steady_clock::now();
    struct Timed { std::chrono::steady_clock::time_point t0; double& ms; ~Timed() { ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); } } timed{t0, toldMs};
    told = ToldHistory(); legendRows.clear(); legendFirstRow.clear(); eraX0.clear(); eraX1.clear();
    if (!shardWorld.valid || shardWorld.body < 0 || shardWorld.body >= (int)shardWorld.sys.bodies.size()) return;
    const Body& b = shardWorld.sys.bodies[shardWorld.body];
    const BodyGen g = BodyGen::make(b);
    if (!shardWorld.history) shardWorld.history = historyOf(shardWorld.sys, b, g, shardWorld.which);
    if (!shardWorld.texts) shardWorld.texts = shardTextsOf(shardWorld.sys, b, g, shardWorld.lore);
    if (!shardWorld.history || !shardWorld.texts) return;
    const History& H = *shardWorld.history;
    std::vector<int> read; int lastIndex = -1;
    for (size_t i = 0; i < shardWorld.shards.size(); i++) if (shardWorld.shards[i].last) lastIndex = (int)i;
    for (int i : shardHeld) if (guide.decoded.count(heldShardKey(i))) read.push_back(i);
    toldHistoryOf(H, *shardWorld.texts, read, lastIndex, told);
    {   // the timeline: each era its share of the width by the people's records written in it (an era of few or none a sliver), linear within
        std::shared_ptr<const RecordCatalogue> C = recordsOf(g, shardWorld.which);
        const int ne = (int)H.eras.size();
        std::vector<double> share(ne, 0.0);
        int total = 0;
        if (C) for (const RecordEntry& e : C->entries) { const int k = H.eraAt(e.year); if (k >= 0 && k < ne) { share[k] += 1; total++; } }
        double sum = 0;
        for (double& v : share) { v = std::max(v / std::max(1, total), 0.07); sum += v; }
        double x = TL_X0;
        for (int k = 0; k < ne; k++) { eraX0.push_back(x); x += (TL_X1 - TL_X0) * share[k] / std::max(sum, 1e-9); eraX1.push_back(x); }
    }
    {   // the history page: the people's ways as the records told them, then the events told under their eras' headings, each line read as the
        // records are (the names kept)
        std::set<std::string> names; legendNameWords(H, told, names);
        const int held = (int)shardHeld.size();
        auto addWords = [&](int kind, int item, int year, const std::string& label, const std::vector<DecodedWord>& words) {   // a line's words in rows the text's width
            LegendRow row; row.kind = kind; row.item = item; row.year = year; row.first = true; row.text = label;
            int x = HIST_TEXT_X;
            for (const DecodedWord& w : words) {
                const int tw = textWidth(shownWord(w).c_str());
                if (x > HIST_TEXT_X && x + tw > HIST_MAX_X) { legendRows.push_back(row); row = LegendRow(); row.kind = kind; row.item = item; row.year = year; x = HIST_TEXT_X; }
                row.words.push_back(w); x += tw + FONT_ADV;
            }
            legendRows.push_back(row);
        };
        std::vector<std::pair<std::string, std::string>> ways; legendWays(H, told, ways);
        if (!ways.empty()) {
            LegendRow h; h.kind = 0; decodeLine("the " + shardWorld.lore.people, shardWorld.lang, shardWorld.tongue, held, names, h.words); legendRows.push_back(h);
            for (const auto& w : ways) { std::vector<DecodedWord> words; decodeLine(w.second, shardWorld.lang, shardWorld.tongue, held, names, words); addWords(3, -1, 0, w.first, words); }
        }
        int era = -2;
        for (size_t k = 0; k < told.events.size(); k++) {
            const HistoryEvent& e = H.events[told.events[k].event];
            const int ek = H.eraAt(e.year);
            if (ek != era) {
                era = ek;
                LegendRow h; h.kind = 0; h.item = (int)k;
                if (ek >= 0 && ek < (int)H.eras.size() && told.eraRead[ek] > 0) {   // its name read as the records are, its years the ship's
                    decodeLine(H.eras[ek].name, shardWorld.lang, shardWorld.tongue, held, names, h.words);
                    h.text = ", " + eraYears(H.eras[ek], ek + 1 == (int)H.eras.size(), told.ended);
                } else h.text = "AN ERA NO RECORD READ IS FROM";
                legendRows.push_back(h);
            }
            std::string line = legendLine(H, told, told.events[k].event);
            if (!line.empty() && line.back() != '.') line += '.';
            std::vector<DecodedWord> words; decodeLine(line, shardWorld.lang, shardWorld.tongue, held, names, words);
            legendFirstRow.push_back((int)legendRows.size());
            addWords(1, (int)k, e.year, std::string(), words);
        }
        if (told.ended) {   // the last recording closes it
            LegendRow c; c.kind = 2; c.text = fmt("THE LAST RECORDING: THE RECORD ENDS IN YEAR %s", thousands(H.last).c_str());
            legendRows.push_back(c);
        }
    }
    legendSel = std::max(0, std::min((int)told.events.size() - 1, legendSel));
}

// the record that told an event first (by its year), of the held in `shardHeld`'s order; -1 none
int Game::toldRecordOf(int item) const {
    if (item < 0 || item >= (int)told.events.size()) return -1;
    for (int local : told.events[item].by) for (size_t i = 0; i < shardHeld.size(); i++) if (shardHeld[i] == local) return (int)i;
    return -1;
}

// the timeline: the people's eras across the width, each placed and named once a record written in it (B-414: or a history telling it) is read (an era none is from a
// stretch of dots); the records held stacked by their years over the axis in their kinds' colours (read bright, held dim, the one under the
// cursor white, a friend's cyan, the last recording amber once read); the events learnt as ticks under it; the record under the cursor below
void Game::renderShardTimeline() {
    ensureTold();
    const int n = (int)shardHeld.size();
    drawShardPagesStrip("");
    {   // the kinds held by their glyphs in their colours (the stacks' key)
        int count[RC_COUNT] = {0, 0, 0};
        for (int i : shardHeld) count[recordKindOf(shardWorld.shards[i])]++;
        int x = UW - 6;
        for (int k = RC_COUNT - 1; k >= 0; k--) {
            const std::string c = std::to_string(count[k]);
            x -= textWidth(c.c_str()); drawText(canvas, x, 14, c.c_str(), KIND_COL[k]);
            x -= 7; drawRecordKindGlyph(canvas, x, 14, k, KIND_COL[k]);
            x -= 2 * FONT_ADV;
        }
    }
    if (!shardWorld.history || eraX0.empty()) {
        drawTextCentered(canvas, UW / 2, 90, "THE COMPUTER CANNOT PLACE THEIR YEARS", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 10, "TAB PAGE  ESC BACK", HUD_DIM);
        return;
    }
    const History& H = *shardWorld.history;
    const int ne = (int)H.eras.size();
    auto xOf = [&](int year) -> int {
        if (year < H.eras.front().start) return TL_X0;
        const int k = H.eraAt(year);
        if (k < 0 || k >= ne) return TL_X1;
        const HistoryEra& E = H.eras[k];
        const double u = (year - E.start + 0.5) / std::max(1, E.end - E.start + 1);
        return (int)std::lround(eraX0[k] + (eraX1[k] - eraX0[k]) * clampd(u, 0, 1));
    };
    const bool ended = told.ended;
    // the eras: a band each once placed (a record of it read), its name over it or, where that row is taken, under it, read as the records
    // are; an era none is from a row of dots
    std::set<std::string> names; legendNameWords(H, told, names);
    std::vector<std::array<int, 3>> labels;   // placed: x0, x1, row
    for (int k = 0; k < ne; k++) {
        const int x0 = (int)std::lround(eraX0[k]), x1 = (int)std::lround(eraX1[k]) - 1;
        if (k > 0) fillRectRGB(canvas, x0, 33, x0, 43, HUD_DIM);
        const bool known = k < (int)told.eraRead.size() && told.eraRead[k] > 0;
        if (!known) { for (int x = x0 + 1; x <= x1; x += 3) fillRectRGB(canvas, x, 38, x, 38, HUD_DIM); continue; }
        fillRectRGB(canvas, x0 + (k > 0), 35, x1, 41, (k & 1) ? rgb(30, 74, 52) : rgb(40, 96, 64));
        std::vector<DecodedWord> ws; decodeLine(H.eras[k].name, shardWorld.lang, shardWorld.tongue, n, names, ws);
        for (int attempt = 0; attempt < 2; attempt++) {
            if (attempt == 1) { if (ws.size() < 2 || (ws[0].ours != "the" && ws[0].ours != "The")) break; ws.erase(ws.begin()); }   // shorter without its "the"
            const int w = wordsWidth(ws), lx = std::max(TL_X0, std::min(TL_X1 - w, (x0 + x1 + 1) / 2 - w / 2));
            int row = -1;
            for (int r = 0; r < 2 && row < 0; r++) {
                bool free = true;
                for (const auto& b : labels) if (b[2] == r && lx < b[1] + 2 * FONT_ADV && lx + w > b[0] - 2 * FONT_ADV) free = false;
                if (free) row = r;
            }
            if (row < 0) continue;
            labels.push_back({lx, lx + w, row});
            drawWords(canvas, lx, row ? 44 : 25, ws, HUD_WHITE);
            break;
        }
    }
    // the records held, stacked by their years (bins two pixels wide)
    std::vector<int> stack((TL_X1 - TL_X0) / 2 + 2, 0);
    int most = 1;
    for (int i = 0; i < n; i++) { const int b = std::max(0, std::min((int)stack.size() - 1, (xOf(shardWorld.shards[shardHeld[i]].year) - TL_X0) / 2)); most = std::max(most, ++stack[b]); }
    const int h = std::max(2, std::min(12, 74 / most));   // a record a block, the tallest stack under the eras' bands
    std::fill(stack.begin(), stack.end(), 0);
    int selX = -1, selTop = TL_BASE, latest = 1;
    for (int i = 0; i < n; i++) {
        const int idx = shardHeld[i]; const Shard& s = shardWorld.shards[idx];
        latest = std::max(latest, s.year);
        const int x = xOf(s.year), b = std::max(0, std::min((int)stack.size() - 1, (x - TL_X0) / 2));
        const int bx = TL_X0 + 2 * b, y1 = TL_BASE - stack[b] * h, y0 = y1 - h + 1;
        stack[b]++;
        const bool read = guide.decoded.count(heldShardKey(idx)) > 0;
        const int kind = recordKindOf(s);
        uint32_t col = read ? KIND_COL[kind] : KIND_DIM[kind];
        if (shardLent(idx)) col = HUD_CYAN;
        if (s.last && ended && read) col = HUD_AMBER;
        if (i == shardSel) { col = HUD_WHITE; selX = bx; selTop = y0; }
        fillRectRGB(canvas, bx, y0, bx + 1, y1, col);
    }
    if (selX >= 0) { fillRectRGB(canvas, selX, selTop - 4, selX + 1, selTop - 3, HUD_WHITE); fillRectRGB(canvas, selX - 1, selTop - 5, selX + 2, selTop - 5, HUD_WHITE); }
    // the axis: solid to the latest record held, dotted on until the last recording is read, when it closes in amber
    const int ax = 128;
    if (ended) {
        fillRectRGB(canvas, TL_X0, ax, TL_X1, ax, HUD_DIM);
        fillRectRGB(canvas, TL_X1 - 1, ax - 3, TL_X1, ax + 3, HUD_AMBER);
        const std::string endLabel = fmt("THE RECORD ENDS, YEAR %s", thousands(H.last).c_str());
        drawText(canvas, TL_X1 - textWidth(endLabel.c_str()), ax + 4, endLabel.c_str(), HUD_AMBER);
    } else {
        const int xl = std::min(TL_X1, xOf(latest) + 1);
        fillRectRGB(canvas, TL_X0, ax, xl, ax, HUD_DIM);
        for (int x = xl + 3; x <= TL_X1; x += 3) fillRectRGB(canvas, x, ax, x, ax, HUD_DIM);
    }
    {   // the count's start where the old time is on the axis
        const int x1 = xOf(1);
        if (H.eras.front().start < 1 && x1 > TL_X0 + 2) { fillRectRGB(canvas, x1, ax - 2, x1, ax + 2, HUD_DIM); drawText(canvas, std::min(x1 + 2, TL_X1 - 36), ax + 4, "YEAR 1", HUD_DIM); }
        else drawText(canvas, TL_X0, ax + 4, fmt("YEAR %s", thousands(H.eras.front().start).c_str()).c_str(), HUD_DIM);
    }
    // the events learnt, a tick at each one's year; the ones the record under the cursor told white
    std::set<int> selTold;
    if (shardSel >= 0 && shardSel < n) for (const ToldEvent& t : told.events) for (int r : t.by) if (r == shardHeld[shardSel]) selTold.insert(t.event);
    for (const ToldEvent& t : told.events) { const int x = xOf(H.events[t.event].year); fillRectRGB(canvas, x, 140, x, 143, rgb(70, 130, 90)); }
    for (int ev : selTold) { const int x = xOf(H.events[ev].year); fillRectRGB(canvas, x, 138, x, 145, HUD_WHITE); }
    if (!told.events.empty()) { const std::string lab = fmt("%d EVENT%s LEARNT", (int)told.events.size(), told.events.size() == 1 ? "" : "S"); drawText(canvas, TL_X0, 147, lab.c_str(), HUD_DIM); }
    // the record under the cursor
    if (n > 0 && shardSel >= 0 && shardSel < n) {
        const int idx = shardHeld[shardSel]; const Shard& s = shardWorld.shards[idx];
        const bool read = guide.decoded.count(heldShardKey(idx)) > 0;
        drawText(canvas, 12, 158, trunc(upper(shardDate(shardWorld.lore, s)), 49).c_str(), HUD_WHITE);
        drawText(canvas, 12, 167, mediumLine(s).c_str(), HUD_DIM);
        drawText(canvas, 12, 176, recordSummary(idx, 49).c_str(), read ? HUD_GREEN : HUD_DIM);
    }
    drawTextCentered(canvas, UW / 2, UH - 10, "ARROWS  SHIFT FAST  ENTER READ  TAB PAGE  ESC BACK", HUD_DIM);
}

// the history as the records read so far tell it: the age once told, the events learnt under their eras (a name amber, a word read green,
// white under the cursor, one not learnt yet dim in their tongue), the last recording closing it; Enter opens the record that told the one
// under the cursor
void Game::renderShardHistory() {
    ensureTold();
    const int ne = (int)told.events.size();
    drawShardPagesStrip(ne > 0 ? fmt("%d OF %d", legendSel + 1, ne) : std::string());
    const std::string age = told.ageTold ? std::string("THE AGE THEY REACHED: ") + AGE_TOLD_NAMES[std::max(0, std::min(AGE_COUNT - 1, told.age))] : std::string("THE AGE THEY REACHED: NOT TOLD YET");
    drawText(canvas, 12, 26, age.c_str(), told.ageTold ? HUD_AMBER : HUD_DIM);
    drawText(canvas, 12, 35, fmt("%d EVENT%s LEARNT FROM %d RECORD%s READ", ne, ne == 1 ? "" : "S", told.read, told.read == 1 ? "" : "S").c_str(), HUD_DIM);
    if (ne > 0 && legendSel < ne) {
        const int by = (int)told.events[legendSel].by.size();
        if (by > 1) { const std::string t = fmt("TOLD BY %d", by); drawText(canvas, UW - 12 - textWidth(t.c_str()), 35, t.c_str(), HUD_DIM); }
    }
    if (told.read == 0) drawTextCentered(canvas, UW / 2, 90, "NO RECORD OF THEIRS READ YET", HUD_DIM);
    else if (legendRows.empty()) drawTextCentered(canvas, UW / 2, 90, "NOTHING READ SO FAR TELLS OF THEIR HISTORY", HUD_DIM);
    const int rows = (int)legendRows.size();
    if (rows > 0) {
        // the event under the cursor in view, with its era's heading when that is just above it
        if (ne > 0) {
            int r0 = legendFirstRow[legendSel], r1 = r0;
            while (r1 + 1 < rows && legendRows[r1 + 1].kind == 1 && legendRows[r1 + 1].item == legendSel) r1++;
            if (legendSel + 1 == ne && r1 + 1 < rows && legendRows[r1 + 1].kind == 2) r1++;   // the close under the last
            if (r0 > 0 && legendRows[r0 - 1].kind == 0) r0--;
            if (legendSel == 0) r0 = 0;   // the first event shows the people's ways above it
            if (r0 < legendTop) legendTop = r0;
            if (r1 >= legendTop + HIST_ROWS) legendTop = r1 - HIST_ROWS + 1;
        }
        legendTop = std::max(0, std::min(std::max(0, rows - HIST_ROWS), legendTop));
        for (int r = legendTop; r < rows && r < legendTop + HIST_ROWS; r++) {
            const LegendRow& L = legendRows[r];
            const int y = HIST_TOP + (r - legendTop) * 9;
            if (L.kind == 0) {   // a heading: the people's name, an era's (read as the records are) and its years
                int xe = drawWords(canvas, 12, y, L.words, HUD_WHITE);
                if (!L.text.empty()) { drawText(canvas, xe, y, L.text.c_str(), HUD_WHITE); xe += textWidth(L.text.c_str()); }
                xe += FONT_ADV;
                if (xe < HIST_MAX_X) fillRectRGB(canvas, xe, y + 3, HIST_MAX_X, y + 3, rgb(40, 76, 52));
                continue;
            }
            if (L.kind == 2) { drawText(canvas, 12, y, L.text.c_str(), HUD_AMBER); continue; }
            const bool sel = L.kind == 1 && L.item == legendSel;
            if (L.kind == 3 && L.first) drawText(canvas, HIST_TEXT_X - FONT_ADV - textWidth(L.text.c_str()), y, L.text.c_str(), HUD_DIM);   // a way's label
            else if (L.kind == 1 && L.first) {
                const std::string yr = thousands(L.year);
                drawText(canvas, HIST_TEXT_X - FONT_ADV - textWidth(yr.c_str()), y, yr.c_str(), sel ? HUD_WHITE : HUD_DIM);
                if (sel) drawText(canvas, 6, y, ">", HUD_AMBER);
            }
            int x = HIST_TEXT_X;
            for (const DecodedWord& w : L.words) {
                const std::string sw = shownWord(w);
                drawText(canvas, x, y, sw.c_str(), w.name ? HUD_AMBER : (w.known ? (sel ? HUD_WHITE : HUD_GREEN) : HUD_DIM));
                x += textWidth(sw.c_str()) + FONT_ADV;
            }
        }
        if (rows > HIST_ROWS) {   // where the rows shown are in the whole
            const int t0 = HIST_TOP, t1 = HIST_TOP + HIST_ROWS * 9 - 3;
            fillRectRGB(canvas, UW - 8, t0, UW - 8, t1, rgb(30, 50, 36));
            const int a = t0 + (t1 - t0) * legendTop / rows, b = t0 + (t1 - t0) * std::min(rows, legendTop + HIST_ROWS) / rows;
            fillRectRGB(canvas, UW - 9, a, UW - 7, std::max(a + 2, b), HUD_DIM);
        }
    }
    drawTextCentered(canvas, UW / 2, UH - 10, ne > 0 ? "UP/DOWN  SHIFT FAST  ENTER READ  TAB PAGE  ESC BACK" : "TAB PAGE  ESC BACK", HUD_DIM);
}
