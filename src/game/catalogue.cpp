// Z-07: the catalogue (R-425; docs/ideas/ITERATION-1.5.0.md Z-07, IDEAS-small-and-spacious.md 1.9; lab_state.h), the guide's page of every
// species seen under the microscope (the guide's sheets) and of a friend's lent by their inbox file (in cyan): by world (a world's head, then
// its species) or, Tab, by kin (the sequenced in the tree's order, a genesis's head before its first; then those not read). Enter shows a
// species' sheet beside its form drawn live in a way of seeing it was seen in (V the next), Up and Down the next species; N names it. Nothing
// of it is saved: its worlds are drawn as it opens, the names read as it draws
#include "game.h"
#include "ui.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
constexpr int CAT_Y0 = 17, CAT_ROW = 9, CAT_ROWS = 18;           // the list: a row every 9 px under the title
constexpr int FORM_X = 6, FORM_Y = 16, FORM_W = 150, FORM_H = 120;   // the sheet's form, live, in a box of the slide's middle
std::string worldOfKey(const std::string& key) { const size_t m = key.rfind("/M"); return m == std::string::npos ? key : key.substr(0, m); }
int indexOfKey(const std::string& key) { const size_t m = key.rfind("/M"); return m == std::string::npos ? -1 : atoi(key.c_str() + m + 2); }
// the form seen of it (its sheet's): a cell where any was seen, else its spore or its cyst, else its fossil
int formSeen(const MicrobeSheet& seen) {
    const uint32_t cells = (1u << CF_CELL) | (1u << CF_DIVIDING) | (1u << CF_COLONY) | (1u << CF_SLUG) | (1u << CF_STALK) | (1u << CF_GHOST);
    if (seen.forms & cells) return CF_CELL;
    if (seen.forms & (1u << CF_SPORE)) return CF_SPORE;
    if (seen.forms & (1u << CF_CYST)) return CF_CYST;
    return CF_FOSSIL;
}
// the ways of seeing its form was seen in: the light ways on any slide, the dark field its flagella, the glow its DNA, the electron its wall; a
// fossil's the section alone
std::vector<int> waysSeen(const MicrobeSheet& seen) {
    if (formSeen(seen) == CF_FOSSIL) return {WS_SECTION};
    std::vector<int> w = {WS_BRIGHT, WS_PHASE};
    if (seen.seen & SEEN_FLAGELLA) w.push_back(WS_DARK);
    if (seen.seen & SEEN_DNA) w.push_back(WS_GLOW);
    if (seen.seen & SEEN_WALL) w.push_back(WS_ELECTRON);
    return w;
}
// a scale bar's length: the roundest under three tenths of the box
double barUm(double widthUm) {
    static const double STEPS[] = {0.2, 0.5, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000};
    double b = STEPS[0];
    for (double s : STEPS) if (s <= widthUm * 0.3) b = s;
    return b;
}
uint64_t keySeed(const std::string& key) { uint64_t h = 1469598103934665603ULL; for (char c : key) h = (h ^ (uint8_t)c) * 1099511628211ULL; return mix64(h); }
}   // namespace

// its sheet: the explorer's own, else a friend's lent (none: empty)
const MicrobeSheet& Game::catalogueSheet(const std::string& key) const {
    static const MicrobeSheet none;
    const auto it = guide.microbes.find(key);
    if (it != guide.microbes.end()) return it->second;
    const auto jt = guide.lentMicrobes.find(key);
    return jt != guide.lentMicrobes.end() ? jt->second : none;
}

void Game::catalogueEnter() {
    cat.page = 0;
    catalogueBuild();
    audio.beep = BEEP_UI;
}

// the rows: every species with a sheet (the explorer's, a friend's lent), its world drawn once (one no longer holding it left out), by world
// or by kin; the one chosen kept where it is still listed
void Game::catalogueBuild() {
    CatalogueState& C = cat;
    std::vector<std::string> keys;
    for (const auto& kv : guide.microbes) keys.push_back(kv.first);
    for (const auto& kv : guide.lentMicrobes) if (!guide.microbes.count(kv.first)) keys.push_back(kv.first);
    LabSystems systems;
    for (const std::string& k : keys) {
        const std::string w = worldOfKey(k);
        if (C.worlds.count(w)) continue;
        CatalogueWorld& W = C.worlds[w];
        FlaskSpecies f;
        if (!speciesOfKey(k, f)) continue;
        const StarSystem* S = systems.get(sys, f.sx, f.sy, f.sz);
        if (!S || f.body >= (int)S->bodies.size()) continue;
        W.kind = S->bodies[f.body].type;
        W.species = lifeOf(*S)->bodies[f.body].species;
        W.forms = morphsOf(*S, f.body);
        W.ok = true;
    }
    auto held = [&](const std::string& k) {
        const auto it = C.worlds.find(worldOfKey(k));
        const int i = indexOfKey(k);
        return it != C.worlds.end() && it->second.ok && i >= 0 && i < (int)it->second.species.size() && i < (int)it->second.forms.size();
    };
    keys.erase(std::remove_if(keys.begin(), keys.end(), [&](const std::string& k) { return !held(k); }), keys.end());
    std::sort(keys.begin(), keys.end(), [](const std::string& a, const std::string& b) {
        const std::string wa = worldOfKey(a), wb = worldOfKey(b);
        return wa != wb ? wa < wb : indexOfKey(a) < indexOfKey(b);
    });
    const std::string was = C.sel >= 0 && C.sel < (int)C.rows.size() ? C.rows[C.sel].key : std::string();
    C.rows.clear();
    auto species = [&](const std::string& k, uint64_t genesis) { CatalogueRow r; r.key = k; r.lent = guide.isLentMicrobe(k); r.genesis = genesis; C.rows.push_back(r); };
    if (C.order == 0) {   // by world: a world's head, then its species by their index
        std::string world;
        for (const std::string& k : keys) {
            if (worldOfKey(k) != world) { world = worldOfKey(k); CatalogueRow h; h.head = true; h.key = world; C.rows.push_back(h); }
            species(k, 0);
        }
    } else {   // by kin: the sequenced in the tree's order (`treeBefore`), a genesis's head before its first; then those not read
        std::vector<std::pair<Lineage, std::string>> read;
        std::vector<std::string> rest;
        for (const std::string& k : keys) {
            if (catalogueSheet(k).seen & SEEN_KIN) {
                const Lineage& L = lineageOfKey(k);
                if (L.valid) { read.push_back({L, k}); continue; }
            }
            rest.push_back(k);
        }
        std::stable_sort(read.begin(), read.end(), [](const std::pair<Lineage, std::string>& a, const std::pair<Lineage, std::string>& b) { return treeBefore(a.first, b.first); });
        for (size_t i = 0; i < read.size(); i++) {
            const Genesis& g = read[i].first.genesis;
            if (i == 0 || g.id != read[i - 1].first.genesis.id) { CatalogueRow h; h.head = true; h.genesis = g.id; h.line = genesisWords(g); C.rows.push_back(h); }
            species(read[i].second, g.id);
        }
        if (!rest.empty()) {
            CatalogueRow h; h.head = true; h.line = "NOT YET SEQUENCED"; C.rows.push_back(h);
            for (const std::string& k : rest) species(k, 0);
        }
    }
    C.sel = -1;
    for (int i = 0; i < (int)C.rows.size(); i++) if (!C.rows[i].head && C.rows[i].key == was) C.sel = i;
    if (C.sel < 0) for (int i = 0; i < (int)C.rows.size(); i++) if (!C.rows[i].head) { C.sel = i; break; }
}

// the next species' row each way (heads passed over), at the list's ends none
void Game::catalogueStep(int d) {
    CatalogueState& C = cat;
    int i = C.sel;
    for (int n = std::abs(d); n > 0; n--) {
        int j = i + (d > 0 ? 1 : -1);
        while (j >= 0 && j < (int)C.rows.size() && C.rows[j].head) j += d > 0 ? 1 : -1;
        if (j < 0 || j >= (int)C.rows.size()) break;
        i = j;
    }
    if (i != C.sel) { C.sel = i; audio.beep = BEEP_UI; }
}

void Game::catalogueName() {
    if (cat.sel < 0 || cat.sel >= (int)cat.rows.size()) { refuse("NO SPECIES TO NAME", 3); return; }
    textSpeciesKey = cat.rows[cat.sel].key;
    const std::string named = nameOfKey(textSpeciesKey);
    beginTextEntry("NAME THE SPECIES", TEXT_SPECIES, named == "UNKNOWN" ? std::string() : upper(named));
}

void Game::updateCatalogue(const Input& in, double dt, double realDt) {
    CatalogueState& C = cat;
    const bool any = C.sel >= 0 && C.sel < (int)C.rows.size();
    if (C.page == 1 && any) {   // a species' sheet: back to the list, the next species, the next way of seeing, a name
        if (in.wasPressed(KEY_ESCAPE) || in.wasPressed(KEY_BACKSPACE) || enterKey(in)) { C.page = 0; audio.beep = BEEP_UI; return; }
        const int was = C.sel;
        if (in.wasPressed(KEY_DOWN) || in.wasPressed(KEY_RIGHT)) catalogueStep(1);
        if (in.wasPressed(KEY_UP) || in.wasPressed(KEY_LEFT)) catalogueStep(-1);
        const std::vector<int> ways = waysSeen(catalogueSheet(C.rows[C.sel].key));
        if (C.sel != was) { C.seeing = ways[0]; C.t0 = realTime; }
        if (in.wasPressed(KEY_V)) {
            const auto it = std::find(ways.begin(), ways.end(), C.seeing);
            C.seeing = it == ways.end() || it + 1 == ways.end() ? ways[0] : *(it + 1);
            if (ways.size() < 2) refuse("SEEN ONLY THIS WAY - THE MICROSCOPE SHOWS MORE", 3);
            else audio.beep = BEEP_UI;
        }
        if (in.wasPressed(KEY_N)) catalogueName();
        return;
    }
    C.page = 0;
    if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); return; }
    if (in.wasPressed(KEY_TAB)) { C.order ^= 1; catalogueBuild(); audio.beep = BEEP_UI; return; }
    if (!any) return;
    if (in.wasPressed(KEY_DOWN)) catalogueStep(1);
    if (in.wasPressed(KEY_UP)) catalogueStep(-1);
    if (in.wasPressed(KEY_PAGE_DOWN)) catalogueStep(CAT_ROWS - 2);
    if (in.wasPressed(KEY_PAGE_UP)) catalogueStep(-(CAT_ROWS - 2));
    if (enterKey(in)) { C.page = 1; C.seeing = waysSeen(catalogueSheet(C.rows[C.sel].key))[0]; C.t0 = realTime; audio.beep = BEEP_UI; }
    if (in.wasPressed(KEY_N)) catalogueName();
}

// a row's words: a world's head (its name or UNKNOWN, its kind), a genesis's; a species' name and its form as seen, on the kin page its world
// where it has room (`world` false: without it)
std::string Game::catalogueLine(const CatalogueRow& R, bool world) {
    if (R.head) {
        if (!R.line.empty()) return R.line;
        const auto it = cat.worlds.find(R.key);
        const std::string kind = it != cat.worlds.end() ? KINDS[it->second.kind].name : "";
        return upper(nameOfKey(R.key)) + (kind.empty() ? "" : " - " + kind);
    }
    const CatalogueWorld& W = cat.worlds[worldOfKey(R.key)];
    const MicrobeSheet& seen = catalogueSheet(R.key);
    const int form = formSeen(seen);
    const std::string named = nameOfKey(R.key);
    std::string s = (named == "UNKNOWN" ? std::string("UNNAMED") : upper(named)) + " - " +
                    (form == CF_CELL ? std::string(planWords(W.forms[indexOfKey(R.key)].plan)) : (form == CF_FOSSIL ? "FOSSILS" : (form == CF_SPORE ? "SPORES" : "CYSTS")));
    if ((seen.seen & SEEN_KIN) && lineageOfKey(R.key).made) s += " - MADE";   // before its world: a long line cuts the world first
    if (cat.order == 1 && world) s += ", " + upper(nameOfKey(worldOfKey(R.key)));
    return s;
}

void Game::renderCatalogue() {
    CatalogueState& C = cat;
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 222);
    if (C.page == 1 && C.sel >= 0 && C.sel < (int)C.rows.size()) { renderCatalogueSheet(); return; }
    drawText(canvas, 6, 4, "CATALOGUE", HUD_AMBER);
    const char* order = C.order == 0 ? "BY WORLD" : "BY KIN";
    drawText(canvas, UW - 6 - textWidth(order), 4, order, HUD_WHITE);
    if (C.rows.empty()) {
        drawTextCentered(canvas, UW / 2, 90, "NO SPECIES SEEN YET", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 10, "ESC CLOSE", HUD_DIM);
        return;
    }
    C.top = clampi(C.top, std::max(0, C.sel - CAT_ROWS + 2), std::max(0, C.sel - 1));   // its head in view above it where it fits
    C.top = clampi(C.top, 0, std::max(0, (int)C.rows.size() - CAT_ROWS));
    for (int r = C.top; r < (int)C.rows.size() && r < C.top + CAT_ROWS; r++) {
        const int y = CAT_Y0 + (r - C.top) * CAT_ROW;
        const CatalogueRow& R = C.rows[r];
        if (R.head) { drawText(canvas, 8, y, fitText(catalogueLine(R), UW - 16).c_str(), R.genesis ? genesisColour(R.genesis) : HUD_AMBER); continue; }
        const bool sel = r == C.sel;
        if (sel) drawText(canvas, 10, y, ">", HUD_AMBER);
        std::string line = catalogueLine(R);
        if (textWidth(line.c_str()) > UW - 24) line = catalogueLine(R, false);   // its world left out before its words are cut
        drawText(canvas, 18, y, fitText(line, UW - 24).c_str(), sel ? HUD_WHITE : (R.lent ? HUD_CYAN : HUD_GREEN));
    }
    drawTextCentered(canvas, UW / 2, UH - 10, C.order == 0 ? "ENTER SHEET  TAB BY KIN  N NAME  ESC CLOSE" : "ENTER SHEET  TAB BY WORLD  N NAME  ESC CLOSE",
                     HUD_DIM);
}

// a species' sheet: its form live on the left (its slide's middle), its lines on the right
void Game::renderCatalogueSheet() {
    CatalogueState& C = cat;
    const CatalogueRow& R = C.rows[C.sel];
    const CatalogueWorld& W = C.worlds[worldOfKey(R.key)];
    const int i = indexOfKey(R.key);
    const MicrobeSheet& seen = catalogueSheet(R.key);
    const double viewUm = renderCatalogueForm(W.forms[i], seen, keySeed(R.key));
    drawRectRGB(canvas, FORM_X - 1, FORM_Y - 1, FORM_X + FORM_W, FORM_Y + FORM_H, R.lent ? HUD_CYAN : HUD_DIM);
    {   // its scale bar, its way of seeing
        const double bar = barUm(viewUm), widthUm = viewUm * FORM_W / FORM_H;
        const int bx = FORM_X + 6, by = FORM_Y + FORM_H - 8, bw = std::max(2, (int)std::lround(bar / widthUm * FORM_W));
        fillRectRGB(canvas, bx - 1, by - 1, bx + bw + 1, by + 2, rgb(0, 0, 0));
        fillRectRGB(canvas, bx, by, bx + bw, by + 1, rgb(255, 255, 255));
        drawText(canvas, bx + bw + 4, by - 3, bar < 1 ? fmt("%.1f UM", bar).c_str() : fmt("%.0f UM", bar).c_str(), HUD_WHITE);
    }
    const std::vector<int> ways = waysSeen(seen);
    drawText(canvas, FORM_X, FORM_Y + FORM_H + 4, SEEING_NAMES[C.seeing], HUD_AMBER);
    if (ways.size() > 1) drawText(canvas, FORM_X, FORM_Y + FORM_H + 13, fmt("V NEXT OF %d WAYS SEEN", (int)ways.size()).c_str(), HUD_DIM);
    if (R.lent) drawText(canvas, FORM_X, FORM_Y + FORM_H + 22, "LENT BY A FRIEND", HUD_CYAN);
    const std::string world = nameOfKey(worldOfKey(R.key));
    const std::vector<std::string> lines = sheetLinesOf(R.key, W.species[i], W.forms[i], world, seen);
    int y = FORM_Y;
    const int x0 = FORM_X + FORM_W + 8, cols = (UW - 6 - x0) / 6;
    for (size_t k = 0; k < lines.size(); k++) {
        const uint32_t col = k == 0 ? (R.lent || nameIsForeign(R.key) ? HUD_CYAN : HUD_WHITE) : (k == 1 || lines[k].rfind("MORE IN:", 0) == 0 ? HUD_DIM : HUD_GREEN);
        for (const std::string& w : wrapText(lines[k], (size_t)cols)) {
            if (y > UH - 22) break;
            drawText(canvas, x0, y, w.c_str(), col);
            y += 8;
        }
    }
    drawTextCentered(canvas, UW / 2, UH - 10, "UP/DOWN NEXT  V SEEING  N NAME  ESC LIST", HUD_DIM);
}

// its form live: one specimen in the way chosen, the view following it, drawn in the catalogue's framebuffer (every bank its way's) and its
// middle put in the box; the slide's height in view (um). Under the electron it is still, as on the microscope's stage (its clock stopped
// under the beam): drawn once for the species and kept (a large cell's mesh takes 35 ms at 4x)
double Game::renderCatalogueForm(const Morph& m, const MicrobeSheet& seen, uint64_t seed) {
    CatalogueState& C = cat;
    if (!C.fb || C.fb->idx.size() != (size_t)FBW * FBH) { C.fb = std::make_unique<Framebuffer>(); C.still = 0; }
    Framebuffer& fb = *C.fb;
    const int form = formSeen(seen);
    const bool still = C.seeing == WS_ELECTRON;
    Slide v; v.seeing = C.seeing; v.t = still ? 0.0 : std::max(0.0, realTime - C.t0); v.seed = seed;
    v.step = FB_SCALE >= 3 ? 2 : 1;   // as the microscope's at the higher scales
    v.turn = 0.6; v.tilt = 0.55; v.stage = 0.35 + 0.1 * std::sin(v.t * 0.5);   // the section's stage turning
    Specimen s; s.m = &m; s.form = form; s.act = AN_LIVE; s.seed = seed; s.angle = 0.3;
    s.still = form != CF_CELL || !(seen.seen & SEEN_MOTION);   // its jiggle alone where its motion was never seen
    double ang;
    specimenPlace(s, v.t, v.cx, v.cy, ang);   // the view follows it
    double viewH = 2.2 * m.viewUm();
    if (form != CF_CELL) {
        double x0, y0, x1, y1;
        specimenBox(s, v.t, x0, y0, x1, y1);
        if (x0 <= x1) viewH = 2.2 * std::max(std::max(v.cx - x0, x1 - v.cx), std::max(v.cy - y0, y1 - v.cy));
    }
    viewH = std::max(viewH, 0.5);
    v.umPerPx = viewH / FORM_H;
    const uint64_t drawn = still ? mix64(seed ^ (uint64_t)FBW) : 0;
    if (!still || C.still != drawn || C.rgb.size() != (size_t)FBW * FBH) {
        slidePalette(fb.pal, C.seeing);
        drawSlide(fb, v, {s});
        fb.mush(2);
        C.rgb.resize((size_t)FBW * FBH);
        fb.toRGB(C.rgb.data(), 1.0);
        C.still = drawn;
    }
    const int S = FB_SCALE, ox = (FBW - FORM_W * S) / 2, oy = (FBH - FORM_H * S) / 2;
    for (int y = 0; y < FORM_H * S; y++)
        std::memcpy(&canvas.px[(size_t)(FORM_Y * S + y) * FBW + FORM_X * S], &C.rgb[(size_t)(oy + y) * FBW + ox], (size_t)FORM_W * S * sizeof(uint32_t));
    return viewH;
}

void Game::cataloguePad(PadMenu& m) const {
    m.title = "CATALOGUE";
    if (cat.page == 1) {
        m.item("NEXT", KEY_DOWN); m.item("PREVIOUS", KEY_UP); m.item("NEXT WAY OF SEEING", KEY_V); m.item("NAME SPECIES", KEY_N);
        m.item("BACK TO LIST", KEY_ESCAPE);
        m.button(PR_B, KEY_ESCAPE, "LIST"); m.button(PR_X, KEY_V, "SEEING"); m.button(PR_Y, KEY_N, "NAME");
        return;
    }
    if (!cat.rows.empty()) { m.item("SHEET", KEY_ENTER); m.item("NAME SPECIES", KEY_N); }
    m.item(cat.order == 0 ? "BY KIN" : "BY WORLD", KEY_TAB);
    m.item("CLOSE", KEY_ESCAPE);
    m.button(PR_A, KEY_ENTER, "SHEET"); m.button(PR_B, KEY_ESCAPE, "CLOSE"); m.button(PR_SELECT, KEY_TAB, cat.order == 0 ? "BY KIN" : "BY WORLD");
}
