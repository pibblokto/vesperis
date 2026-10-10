// C-11 (2026-10-10): the instruments' case on the cabin's left wall (galaxy/instruments.h). The instruments taken from the ruins
// (`Guide::instruments`) are kept here and played: the one on the screen is drawn from its own numbers (game/instrument_draw.cpp) and sounds
// with its people's timbre on its people's scale through the synth's instrument voices (`AudioState::instrument`, `instrumentHits`): the keys
// 1 to 9 and 0 play the scale's degrees from the register's first note up (past the scale's last, the next period's), held while the key is
// down; Left and Right move the register a period down or up; Up and Down take another from the case. The gramophone plays its disc instead
// (Enter winds it): a piece of its people's age, heard through its horn. The ship's player plays on under an instrument (one may play along).
#include "game.h"
#include "ui.h"
#include "galaxy/records.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
const int DIGIT_KEYS[10] = {KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0};
int bodyOfKeyI(const std::string& key) { size_t sl = key.find('/'); return sl == std::string::npos ? -1 : atoi(key.c_str() + sl + 1); }
int idOfKey(const std::string& key) { const size_t at = key.rfind("/I"); return at == std::string::npos ? -1 : atoi(key.c_str() + at + 2); }
std::string worldOfKey(const std::string& key) { const size_t at = key.rfind("/I"); return at == std::string::npos ? key : key.substr(0, at); }
// an instrument's key read: its world, its people's lore and tradition, its people's instruments and itself
struct KeyRead { BodyGen g; Lore lore; Tradition T; PeopleInstruments set; Instrument inst; };
bool readInstrumentKey(const std::string& key, KeyRead& out) {
    const std::string wk = worldOfKey(key); const int id = idOfKey(key);
    int64_t sx, sy, sz; Star st;
    if (id < 0 || !Guide::parseStarKey(wk, sx, sy, sz) || !starInSector(sx, sy, sz, st, true)) return false;
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
    icase.keys.assign(guide.instruments.begin(), guide.instruments.end());
    std::sort(icase.keys.begin(), icase.keys.end(), [](const std::string& a, const std::string& b) { const std::string wa = worldOfKey(a), wb = worldOfKey(b); return wa != wb ? wa < wb : idOfKey(a) < idOfKey(b); });
    icase.notes = 0;
    icase.names.assign(icase.keys.size(), "?");   // each one's name, read once
    for (size_t i = 0; i < icase.keys.size(); i++) { KeyRead kr; if (readInstrumentKey(icase.keys[i], kr)) icase.names[i] = instrumentName(kr.T, kr.inst); }
    const std::string was = icase.key;
    int sel = 0;
    for (size_t i = 0; i < icase.keys.size(); i++) if (icase.keys[i] == was) sel = (int)i;
    icase.valid = false;
    if (!icase.keys.empty()) loadInstrument(sel);
    audio.hum = std::min(audio.hum, 0.2);   // the ship's hum low under what is played (the space view's frames raise it again)
    state = GameState::INSTRUMENTS; audio.beep = 4;
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
    icase.people = "THE " + upper(kr.lore.people);
    {   // the world as the decoder lists it: its name (UNKNOWN till named) and where it is
        const std::string wk = worldOfKey(icase.key); int64_t sx = 0, sy = 0, sz = 0; Guide::parseStarKey(wk, sx, sy, sz);
        icase.world = upper(worldNameOfKey(wk)) + fmt(" (%+lld %+lld %+lld / %d)", (long long)sx, (long long)sy, (long long)sz, bodyOfKeyI(wk));
    }
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
    if (in.wasPressed(KEY_ESCAPE)) { stopInstrument(); state = GameState::SPACE; audio.beep = 4; return; }
    const int n = (int)icase.keys.size();
    if (n == 0) return;
    if (in.wasPressed(KEY_UP)) { loadInstrument((icase.sel + n - 1) % n); audio.beep = 4; return; }
    if (in.wasPressed(KEY_DOWN)) { loadInstrument((icase.sel + 1) % n); audio.beep = 4; return; }
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
            for (const Note& nt : icase.disc.notes) if (nt.voice >= 0 && nt.t <= beat && nt.t > beat - realDt * icase.disc.bpm / 60.0 && !icase.glow.empty()) icase.glow[(nt.degree % (int)icase.glow.size() + (int)icase.glow.size()) % (int)icase.glow.size()] = 1.0;
            if (icase.discT > icase.disc.seconds + 1.0 || (mine && audio.pieceDone && icase.discT > 0.5) || !mine) stopInstrument();
        }
        return;
    }
    if (in.wasPressed(KEY_LEFT)) { icase.regShift = std::max(-1, icase.regShift - 1); audio.beep = 4; }
    if (in.wasPressed(KEY_RIGHT)) { icase.regShift = std::min(1, icase.regShift + 1); audio.beep = 4; }
    for (int d = 0; d < 10; d++) {
        const int k = DIGIT_KEYS[d];
        if (in.wasPressed(k)) { playInstrumentDegree(d, k); icase.held[d] = true; }
        else if (icase.held[d] && !in.isDown(k)) { icase.held[d] = false; audio.instrumentUps.push_back(k); }
    }
}

void Game::renderInstruments() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 225);
    drawText(canvas, 6, 4, "THE INSTRUMENTS", HUD_AMBER);
    const int n = (int)icase.keys.size();
    if (n == 0) {
        drawTextCentered(canvas, UW / 2, 80, "THE CASE IS EMPTY", HUD_GREEN);
        drawTextCentered(canvas, UW / 2, 92, "INSTRUMENTS LIE IN THE RUINS OF THE OLD PEOPLES,", HUD_DIM);
        drawTextCentered(canvas, UW / 2, 101, "BESIDE THEIR MUSIC, TAKEN BY HAND", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 10, "ESC BACK", HUD_DIM);
        return;
    }
    const std::string head = n == 1 ? "1 IN THE CASE" : fmt("%d IN THE CASE", n);
    drawText(canvas, UW - 6 - textWidth(head.c_str()), 4, head.c_str(), HUD_DIM);
    // the case: the instruments by their worlds
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
    drawText(canvas, 124, 124, trunc(instrumentName(icase.tradition, I) + " OF " + icase.people, 31).c_str(), HUD_WHITE);
    const char* reg = icase.regShift < 0 ? "LOWER" : (icase.regShift > 0 ? "HIGHER" : "");
    const bool showReg = *reg && !instrumentPlaysDisc(I);
    drawText(canvas, 124, 133, trunc("FROM " + icase.world, showReg ? 23 : 31).c_str(), HUD_DIM);   // C-17: room for the register's word
    if (showReg) drawText(canvas, UW - 6 - textWidth(reg), 133, reg, HUD_CYAN);
    if (disc) {
        drawText(canvas, 124, 146, trunc(std::string("ITS DISC: ") + PIECE_FORM_NAMES[std::max(0, std::min(PF_COUNT - 1, icase.disc.form))], 31).c_str(), HUD_GREEN);
        const std::string st = icase.discOn ? fmt("TURNING  %d:%02d OF %d:%02d", (int)icase.discT / 60, (int)icase.discT % 60, (int)icase.disc.seconds / 60, (int)icase.disc.seconds % 60) : "STILL";
        drawText(canvas, 124, 155, st.c_str(), icase.discOn ? HUD_AMBER : HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 10, "ENTER WINDS IT  UP/DOWN ANOTHER  ESC BACK", HUD_DIM);
        return;
    }
    drawText(canvas, 124, 146, trunc(traditionLine(icase.tradition), 31).c_str(), HUD_GREEN);
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
    drawTextCentered(canvas, UW / 2, UH - 10, "1-9 AND 0 PLAY  LEFT/RIGHT REGISTER  UP/DOWN  ESC", HUD_DIM);
}

std::string Game::testInstrumentInfo() const {
    if (!icase.valid) return fmt("the case: %d instruments, none on the screen", (int)icase.keys.size());
    const Timbre* tb = instrumentTimbre(icase.tradition, icase.inst);
    return fmt("the case: %d instruments; on the screen %s (id %d, people %d, age %s, voice %d, %s), %d courses, register %+d, %d notes played, the synth's timbre %s%s",
               (int)icase.keys.size(), instrumentName(icase.tradition, icase.inst).c_str(), icase.inst.id, icase.inst.people, AGE_NAMES[std::max(0, std::min(AGE_COUNT - 1, icase.inst.age))], icase.inst.voice,
               tb ? (tb->name + ", " + TIMBRE_FAMILY_NAMES[std::max(0, std::min(TF_COUNT - 1, tb->family))]).c_str() : "its disc", (int)icase.glow.size(), icase.inst.reg + icase.regShift, icase.notes,
               audio.instrument == tb && tb ? "set" : (audio.instrument ? "another's" : "none"), icase.discOn ? ", the disc turning" : "");
}
