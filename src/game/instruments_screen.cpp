// C-11 (2026-10-10): the instruments' case on the cabin's left wall (peoples/instruments.h). The instruments taken from the ruins
// (`Guide::instruments`) are kept here and played: the one on the screen is drawn from its own numbers (game/instrument_draw.cpp) and sounds
// with its people's timbre on its people's scale through the synth's instrument voices (`AudioState::instrument`, `instrumentHits`): the keys
// 1 to 9 and 0 play the scale's degrees from the register's first note up (past the scale's last, the next period's), held while the key is
// down; Left and Right move the register a period down or up; Up and Down take another from the case. The gramophone plays its disc instead
// (Enter winds it): a piece of its people's age, heard through its horn. The ship's player plays on under an instrument (one may play along).
// the case opens on its worlds, a folder each as the decoder's shards are kept (the user: "make it so that instruments are
// also separated by planet/world as shards are so you open specific planet as a folder"): Enter opens a world and its instruments are the
// list (Up and Down within it), Esc closes the world and Esc again the case. A world of two peoples is a folder each people, named on its
// line as the decoder names them (the user: "in instruments case split instruments by civilization as a decoder does").
#include "game.h"
#include "ui.h"
#include "peoples/records.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>

namespace {
const int DIGIT_KEYS[10] = {KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0};
int bodyOfKeyI(const std::string& key) { size_t sl = key.find('/'); return sl == std::string::npos ? -1 : atoi(key.c_str() + sl + 1); }
int idOfKey(const std::string& key) { const size_t at = key.rfind("/I"); return at == std::string::npos ? -1 : atoi(key.c_str() + at + 2); }
std::string worldOfKey(const std::string& key) { const size_t at = key.rfind("/I"); return at == std::string::npos ? key : key.substr(0, at); }
int peopleOfKey(const std::string& key) { return std::max(0, idOfKey(key)) / 1000; }   // instruments.h: people x 1000 + age x 100 + slot
// an instrument's key read: its world, its people's lore and tradition, its people's instruments and itself
struct KeyRead { BodyGen g; Lore lore; Tradition T; PeopleInstruments set; Instrument inst; };
bool readInstrumentKey(const std::string& key, KeyRead& out) {
    const std::string wk = worldOfKey(key); const int id = idOfKey(key);
    int64_t sx, sy, sz; Star st;
    if (id < 0 || !Guide::parseStarKey(wk, sx, sy, sz) || !placeInSector(sx, sy, sz, st, true)) return false;
    StarSystem wsys; wsys.generate(st);
    const int bi = bodyOfKeyI(wk);
    if (bi < 0 || bi >= (int)wsys.bodies.size()) return false;
    out.g = BodyGen::make(wsys.bodies[bi]);
    if (!worldHadCivilisation(out.g)) return false;
    out.lore = loreQuick(wsys, wsys.bodies[bi], out.g, id / 1000);
    out.T = traditionOf(out.g, out.lore);
    instrumentsOf(out.T, out.lore.which, out.set);
    const Instrument* I = instrumentById(out.set, id);
    if (!I) return false;
    out.inst = *I;
    return true;
}
}

void Game::openInstruments() {
    radarOff(); if (tele.on) telescopeOff();
    stopInstrument();
    // C-18: the worlds, a folder each (a world of two peoples a folder each people, named as the decoder names them): how many of each
    // the case holds; the one open before kept, else the world under the ship's first
    std::map<std::pair<std::string, int>, int> count;
    for (const std::string& k : guide.instruments) count[{worldOfKey(k), peopleOfKey(k)}]++;
    const bool had = icase.worldSel >= 0 && icase.worldSel < (int)icase.worlds.size() && icase.worldSel < (int)icase.worldWhich.size();
    const std::string wasWorld = had ? icase.worlds[icase.worldSel] : std::string();
    const int wasWhich = had ? icase.worldWhich[icase.worldSel] : 0;
    icase.worlds.clear(); icase.worldWhich.clear(); icase.worldCounts.clear(); icase.worldPeoples.clear();
    const std::string here = sys.valid && ship.parkedBody >= 0 ? Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, ship.parkedBody) : std::string();
    std::string read; std::string names[2]; int peoples = 1;   // the last world's peoples, read once for its folders
    for (const auto& kv : count) {
        const std::string& wk = kv.first.first; const int which = kv.first.second;
        if (wk != read) {
            read = wk; peoples = 1;
            int64_t sx, sy, sz; Star st;
            if (Guide::parseStarKey(wk, sx, sy, sz) && placeInSector(sx, sy, sz, st, true)) {
                StarSystem wsys; wsys.generate(st);
                const int bi = bodyOfKeyI(wk);
                if (bi >= 0 && bi < (int)wsys.bodies.size()) {
                    const BodyGen g = BodyGen::make(wsys.bodies[bi]);
                    if (peoplesOf(g) > 1) { peoples = 2; for (int p = 0; p < 2; p++) names[p] = upper(peopleNameOf(peopleGen(g, p))); }
                }
            }
        }
        icase.worlds.push_back(wk); icase.worldWhich.push_back(which); icase.worldCounts.push_back(kv.second);
        icase.worldPeoples.push_back(peoples > 1 && which < 2 ? "THE " + names[which] : std::string());
    }
    int sel = -1;
    for (size_t i = 0; i < icase.worlds.size(); i++) if (icase.worlds[i] == wasWorld && icase.worldWhich[i] == wasWhich) sel = (int)i;
    if (!here.empty() && (sel < 0 || icase.worlds[sel] != here))
        for (size_t i = 0; i < icase.worlds.size(); i++) if (icase.worlds[i] == here) { sel = (int)i; break; }
    icase.worldSel = std::max(0, sel);
    icase.level = 0; icase.keys.clear(); icase.names.clear(); icase.valid = false;
    icase.notes = 0;
    audio.hum = std::min(audio.hum, 0.2);   // the ship's hum low under what is played (the space view's frames raise it again)
    state = GameState::INSTRUMENTS; audio.beep = BEEP_UI;
}

// C-18: a world's instruments onto the case's list (a world of two peoples: the folder's people's), the first (or the one last played
// of them) on the screen
void Game::openCaseWorld(int w) {
    if (w < 0 || w >= (int)icase.worlds.size() || w >= (int)icase.worldWhich.size()) return;
    icase.worldSel = w; icase.level = 1;
    icase.keys.clear();
    for (const std::string& k : guide.instruments) if (worldOfKey(k) == icase.worlds[w] && peopleOfKey(k) == icase.worldWhich[w]) icase.keys.push_back(k);
    std::sort(icase.keys.begin(), icase.keys.end(), [](const std::string& a, const std::string& b) { return idOfKey(a) < idOfKey(b); });
    icase.names.assign(icase.keys.size(), "?");   // each one's name, read once
    for (size_t i = 0; i < icase.keys.size(); i++) { KeyRead kr; if (readInstrumentKey(icase.keys[i], kr)) icase.names[i] = instrumentName(kr.T, kr.inst); }
    const std::string was = icase.key;
    int sel = 0;
    for (size_t i = 0; i < icase.keys.size(); i++) if (icase.keys[i] == was) sel = (int)i;
    icase.valid = false;
    if (!icase.keys.empty()) loadInstrument(sel);
    audio.beep = BEEP_UI;
}

bool Game::loadInstrument(int sel) {
    stopInstrument();   // the synth lets go of the last one's timbre before its tradition is replaced
    icase.valid = false;
    if (sel < 0 || sel >= (int)icase.keys.size()) return false;
    icase.sel = sel; icase.key = icase.keys[sel];
    KeyRead kr;
    if (!readInstrumentKey(icase.key, kr)) return false;
    icase.tradition = kr.T; icase.set = kr.set; icase.inst = kr.inst;
    const Instrument* I = &icase.inst;
    icase.palette = cultureOf(kr.g, kr.lore.which).palette;
    icase.people = "THE " + upper(kr.lore.people);   // the title's on a world of one (R-414)
    icase.regShift = 0; icase.played.clear();
    icase.glow.assign(std::max(1, instrumentCourses(icase.tradition, icase.inst)), 0.0);
    if (!instrumentPlaysDisc(*I)) audio.instrument = instrumentTimbre(icase.tradition, icase.inst);
    else instrumentDiscOf(icase.tradition, icase.inst, icase.disc);   // the gramophone's disc
    icase.valid = true;
    return true;
}

void Game::stopInstrument() {
    audio.instrument = nullptr; audio.instrumentHits.clear(); audio.instrumentUps.clear();
    if (icase.discOn && audio.piece == &icase.disc) { audio.piece = nullptr; audio.tradition = nullptr; audio.medium = -1; audio.pieceStart = false; audio.pieceBeat = -1; }
    icase.discOn = false; icase.discT = -1;
    for (bool& h : icase.held) h = false;
}

// a note of the one on the screen (`key` holds it; 0 a note of its own length)
void Game::playInstrumentDegree(int degree, int key) {
    if (!icase.valid || instrumentPlaysDisc(icase.inst) || !audio.instrument) return;
    AudioState::InstrumentNote nt;
    nt.hz = instrumentHz(icase.tradition, icase.inst, degree, icase.regShift); nt.vel = 0.8; nt.key = key; nt.hold = 0.7;
    if (icase.inst.pads) { nt.timbre = instrumentTimbre(icase.tradition, icase.inst, degree); nt.hz = 0; }   // a drum machine: the key's pad
    audio.instrumentHits.push_back(nt);
    icase.notes++;
    if (!icase.glow.empty()) icase.glow[degree % (int)icase.glow.size()] = 1.0;
    const int sc = std::max(1, (int)icase.tradition.scale.size());
    icase.played.push_back({degree + sc * icase.regShift, realTime});
    if (icase.played.size() > 28) icase.played.erase(icase.played.begin());
}

void Game::updateInstruments(const Input& in, double realDt) {
    for (double& g : icase.glow) g = std::max(0.0, g - realDt * 1.4);
    if (icase.level == 0) {   // C-18: the worlds
        const int nw = (int)icase.worlds.size();
        if (in.wasPressed(KEY_ESCAPE)) { state = GameState::SPACE; audio.beep = BEEP_UI; return; }
        if (nw == 0) return;
        if (menuStep(in, icase.worldSel, nw)) audio.beep = BEEP_UI;
        if (enterKey(in)) openCaseWorld(icase.worldSel);
        return;
    }
    if (in.wasPressed(KEY_ESCAPE)) { stopInstrument(); icase.level = 0; icase.valid = false; audio.beep = BEEP_UI; return; }   // back to the worlds
    const int n = (int)icase.keys.size();
    if (n == 0) return;
    if (in.wasPressed(KEY_UP)) { loadInstrument((icase.sel + n - 1) % n); audio.beep = BEEP_UI; return; }
    if (in.wasPressed(KEY_DOWN)) { loadInstrument((icase.sel + 1) % n); audio.beep = BEEP_UI; return; }
    if (!icase.valid) return;
    if (instrumentPlaysDisc(icase.inst)) {   // the gramophone: Enter winds it and the disc plays through its horn; again stops it
        const bool mine = icase.discOn && audio.piece == &icase.disc;
        if (enterKey(in) || in.wasPressed(KEY_SPACE)) {
            if (icase.discOn) stopInstrument();
            else {
                audio.speech = nullptr; audio.voice = nullptr;   // the player's, held while the disc turns (it takes up again after)
                audio.piece = &icase.disc; audio.tradition = &icase.tradition; audio.medium = RM_CYLINDER; audio.pieceSpeed = 1; audio.pieceFrom = 0;
                audio.pieceStart = true; audio.piecePause = false; audio.pieceDone = false; audio.pieceBeat = -1;
                icase.discOn = true; icase.discT = 0;
            }
            return;
        }
        if (icase.discOn) {
            if (mine && audio.pieceBeat >= 0) icase.discT = audio.pieceBeat * 60.0 / std::max(1.0, icase.disc.bpm); else icase.discT += realDt;
            const double beat = icase.discT * icase.disc.bpm / 60.0;   // its notes light the picture as they sound
            for (const Note& nt : icase.disc.notes)
                if (nt.voice >= 0 && nt.t <= beat && nt.t > beat - realDt * icase.disc.bpm / 60.0 && !icase.glow.empty())
                    icase.glow[(nt.degree % (int)icase.glow.size() + (int)icase.glow.size()) % (int)icase.glow.size()] = 1.0;
            if (icase.discT > icase.disc.seconds + 1.0 || (mine && audio.pieceDone && icase.discT > 0.5) || !mine) stopInstrument();
        }
        return;
    }
    if (in.wasPressed(KEY_LEFT)) { icase.regShift = std::max(-1, icase.regShift - 1); audio.beep = BEEP_UI; }
    if (in.wasPressed(KEY_RIGHT)) { icase.regShift = std::min(1, icase.regShift + 1); audio.beep = BEEP_UI; }
    for (int d = 0; d < 10; d++) {
        const int k = DIGIT_KEYS[d];
        if (in.wasPressed(k)) { playInstrumentDegree(d, k); icase.held[d] = true; }
        else if (icase.held[d] && !in.isDown(k)) { icase.held[d] = false; audio.instrumentUps.push_back(k); }
    }
}

// C-18: the case's worlds, a folder each: the world's name and where it is (a world of two peoples: the folder's people, as the decoder's
// lines), how many it holds
void Game::renderCaseWorlds() {
    drawText(canvas, 6, 4, "THE INSTRUMENTS", HUD_AMBER);
    const int nw = (int)icase.worlds.size();
    if (nw == 0) {
        drawTextCentered(canvas, UW / 2, 80, "THE CASE IS EMPTY", HUD_GREEN);
        drawTextCentered(canvas, UW / 2, 92, "INSTRUMENTS LIE IN THE RUINS OF THE OLD PEOPLES,", HUD_DIM);
        drawTextCentered(canvas, UW / 2, 101, "BESIDE THEIR MUSIC, TAKEN BY HAND OR BY DRONE", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 10, "ESC BACK", HUD_DIM);
        return;
    }
    const int rows = 9, page = icase.worldSel / rows, pages = (nw + rows - 1) / rows;
    int total = 0; for (int c : icase.worldCounts) total += c;
    const int worlds = (int)std::set<std::string>(icase.worlds.begin(), icase.worlds.end()).size();
    const std::string head = pages > 1 ? fmt("%d IN THE CASE  PAGE %d/%d", total, page + 1, pages) : fmt("%d IN THE CASE FROM %d WORLD%s", total, worlds, worlds == 1 ? "" : "S");
    drawText(canvas, UW - 6 - textWidth(head.c_str()), 4, head.c_str(), HUD_DIM);
    for (int i = page * rows; i < nw && i < (page + 1) * rows; i++) {
        const int y = 18 + (i - page * rows) * 19;
        const std::string& key = icase.worlds[i];
        const bool sel = i == icase.worldSel;
        int64_t sx = 0, sy = 0, sz = 0; Guide::parseStarKey(key, sx, sy, sz);
        if (sel) drawText(canvas, 6, y, ">", HUD_AMBER);
        const std::string where = fmt("  (%+lld %+lld %+lld / %d)", (long long)sx, (long long)sy, (long long)sz, bodyOfKeyI(key));
        const std::string who = i < (int)icase.worldPeoples.size() && !icase.worldPeoples[i].empty() ? "  " + icase.worldPeoples[i] : std::string();
        const int room = std::max(8, std::min(24, 49 - (int)where.size() - (int)who.size()));   // the people's name kept, the world's cut
        drawText(canvas, 16, y, (trunc(upper(worldNameOfKey(key)), room) + where + who).c_str(), sel ? HUD_WHITE : (nameIsForeign(key) ? HUD_CYAN : HUD_GREEN));
        const int c = icase.worldCounts[i];
        drawText(canvas, 16, y + 9, fmt("%d INSTRUMENT%s", c, c == 1 ? "" : "S").c_str(), HUD_DIM);
    }
    drawTextCentered(canvas, UW / 2, UH - 10, "UP/DOWN  ENTER OPENS A WORLD  ESC CLOSE", HUD_DIM);
}

void Game::renderInstruments() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 225);
    if (icase.level == 0) { renderCaseWorlds(); return; }
    const int n = (int)icase.keys.size();
    const bool inRange = icase.worldSel >= 0 && icase.worldSel < (int)icase.worlds.size();
    const std::string wname = inRange ? upper(worldNameOfKey(icase.worlds[icase.worldSel])) : std::string("?");
    // C-18: the open world's and its people's (R-414: the people of a world of one too, the instruments' own: their lines name the instrument alone)
    std::string who = inRange && icase.worldSel < (int)icase.worldPeoples.size() ? icase.worldPeoples[icase.worldSel] : std::string();
    if (who.empty() && icase.valid) who = icase.people;
    drawText(canvas, 6, 4, (who.empty() ? trunc(wname, 30) : trunc(wname, 16) + " - " + trunc(who, 16)).c_str(), HUD_AMBER);
    if (n == 0) { drawTextCentered(canvas, UW / 2, 80, "NOTHING OF THIS WORLD IN THE CASE", HUD_GREEN); drawTextCentered(canvas, UW / 2, UH - 10, "ESC THE WORLDS", HUD_DIM); return; }
    const std::string head = n == 1 ? "1 INSTRUMENT" : fmt("%d INSTRUMENTS", n);
    drawText(canvas, UW - 6 - textWidth(head.c_str()), 4, head.c_str(), HUD_DIM);
    // the world's instruments
    const int rows = 15, top = std::max(0, std::min(icase.sel - rows / 2, n - rows));
    for (int i = top; i < n && i < top + rows; i++) {
        const int y = 18 + (i - top) * 10;
        const bool sel = i == icase.sel;
        const std::string name = i < (int)icase.names.size() ? icase.names[i] : std::string("?");
        if (sel) drawText(canvas, 6, y, ">", HUD_AMBER);
        drawText(canvas, 14, y, trunc(name, 17).c_str(), sel ? HUD_WHITE : HUD_GREEN);
    }
    if (!icase.valid) { drawTextCentered(canvas, 220, 80, "IT CANNOT BE READ", HUD_DIM); return; }
    const Instrument& I = icase.inst;
    const bool disc = instrumentPlaysDisc(I);
    drawInstrumentPicture(122, 16, 192, 104, I, icase.tradition, icase.palette, icase.glow, realTime);
    // the instrument's name alone (the user: "On the right you again see that it's 'From <planet>' and coordinates. No need for that -
    // you already know where it is from when you open a name of the planet/civilization in instruments case"): C-11's "OF THE ..." and
    // C-17's "FROM <world> (<sector> / <body>)" went to the title, the folder's own
    drawText(canvas, 124, 124, trunc(instrumentName(icase.tradition, I), 31).c_str(), HUD_WHITE);
    const char* reg = icase.regShift < 0 ? "LOWER" : (icase.regShift > 0 ? "HIGHER" : "");
    const bool showReg = *reg && !instrumentPlaysDisc(I);
    if (disc) {
        drawText(canvas, 124, 135, trunc(std::string("ITS DISC: ") + PIECE_FORM_NAMES[std::max(0, std::min(PF_COUNT - 1, icase.disc.form))], 31).c_str(), HUD_GREEN);
        const std::string st = icase.discOn ? fmt("TURNING  %d:%02d OF %d:%02d", (int)icase.discT / 60, (int)icase.discT % 60, (int)icase.disc.seconds / 60, (int)icase.disc.seconds % 60) : "STILL";
        drawText(canvas, 124, 144, st.c_str(), icase.discOn ? HUD_AMBER : HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 10, "ENTER WINDS IT  UP/DOWN ANOTHER  ESC THE WORLDS", HUD_DIM);
        return;
    }
    drawText(canvas, 124, 135, trunc(traditionLine(icase.tradition), showReg ? 23 : 31).c_str(), HUD_GREEN);   // room for the register's word
    if (showReg) drawText(canvas, UW - 6 - textWidth(reg), 135, reg, HUD_CYAN);
    // the keys: a bar a degree, as high as its pitch over the register's first note (the scale's own steps), lit while it sounds
    const int sc = std::max(1, (int)icase.tradition.scale.size());
    const double top0 = traditionCents(icase.tradition, 0), span = std::max(1.0, traditionCents(icase.tradition, 10) - top0);
    for (int d = 0; d < 10; d++) {
        const int x = 126 + d * 18, h = icase.inst.pads ? 8 : 2 + (int)std::lround(14 * (traditionCents(icase.tradition, d) - top0) / span);   // a drum machine's pads have no pitch
        const double g = icase.held[d] ? 1.0 : (icase.glow.empty() ? 0.0 : icase.glow[d % (int)icase.glow.size()] * 0.6);
        const uint32_t col = g > 0.05 ? HUD_AMBER : (d % sc == 0 ? HUD_WHITE : HUD_GREEN);
        fillRectRGB(canvas, x, 174 - h, x + 9, 174, col);
        drawText(canvas, x + 2, 177, fmt("%d", (d + 1) % 10).c_str(), g > 0.05 ? HUD_AMBER : HUD_DIM);
    }
    drawTextCentered(canvas, UW / 2, UH - 10, "1-9 AND 0 PLAY  LEFT/RIGHT REGISTER  UP/DOWN  ESC", HUD_DIM);   // Esc to the worlds
}
