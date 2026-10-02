// C-06: the shard decoder on the ship. The shards the explorer took (`Guide::shards`) are read here, at the decoder on the
// cabin's back wall (or from the flight computer and the guide menu): the worlds they come from, a world's shards by their
// years, and a shard's text as the ship's computer can read it with the language learnt from that world's shards
// (`galaxy/shards.h`: `languageShare`, `decodeShard`). C-05: a text shard is a recording that speaks (`galaxy/voice.h`,
// `AudioState::speech`): the people's words in the people's voice, and the word being spoken is marked under the text. A
// shard the computer has not read at the language's current share resolves its words as they are spoken, the end of the
// recording reads it (Enter skips to the end, Esc leaves it unread); the guide keeps the count it was read with (`decoded`),
// so a shard re-read after more of its world were found decodes again with fewer gaps. A word not learnt yet shows dim, in
// the people's own tongue; a name amber; a word read white. C-04: a shard that is a piece of music (`galaxy/music.h`) plays
// here through the synth (`AudioState::piece`) with its notes drawn as a roll the playhead crosses, the tradition's
// scale, cycle and timbres named above it; N names the piece (the guide's `names`, keyed like the shard); a piece heard
// is recorded as read at the world's full count, so it never waits again. The ship's hum is ducked under either.
#include "game.h"
#include "ui.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>

namespace {
const int WORLD_ROWS = 8;    // worlds per page (two lines each)
const int SHARD_ROWS = 16;   // a world's shards per page
int bodyOfKey(const std::string& key) { size_t sl = key.find('/'); return sl == std::string::npos ? -1 : atoi(key.c_str() + sl + 1); }
}

int Game::shardsHeldOf(const std::string& key) const {
    std::string prefix = key + "/S"; int n = 0;
    for (const std::string& k : guide.shards) if (k.compare(0, prefix.size(), prefix) == 0) n++;
    return n;
}

std::string Game::worldNameOfKey(const std::string& key) const {   // the explorer's (or the inbox's) name, else UNKNOWN (R-401)
    auto it = guide.names.find(key);
    if (it != guide.names.end()) return it->second;
    it = guide.inbox.find(key);
    if (it != guide.inbox.end()) return it->second;
    return "UNKNOWN";
}

// a shard the guide holds that the computer has not read at its world's current share
bool Game::shardsPending() const {
    std::map<std::string, int> held;
    for (const std::string& k : guide.shards) { size_t p = k.rfind("/S"); if (p != std::string::npos) held[k.substr(0, p)]++; }
    for (const std::string& k : guide.shards) {
        size_t p = k.rfind("/S"); if (p == std::string::npos) continue;
        auto d = guide.decoded.find(k);
        if (d == guide.decoded.end() || d->second < held[k.substr(0, p)]) return true;
    }
    return false;
}

// the world's system from its key, its lore, its fifty and its language; kept while the same world is read
// C-07: a world's shards, language, voice and tradition built for the decoder or the radar (the key is the caller's)
bool Game::buildShardWorld(ShardWorld& w, const Star& s, int bi) {
    std::string key = w.key;
    w = ShardWorld(); w.key = key;
    if (!s.valid) return false;
    w.sys.generate(s);
    if (bi < 0 || bi >= (int)w.sys.bodies.size()) return false;
    const Body& b = w.sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    w.body = bi;
    w.lore = loreOf(w.sys, b, g);
    w.tongue = tongueOf(g, w.lore);
    shardsOf(w.sys, b, g, w.lore, SHARDS_PER_WORLD, w.shards);
    languageOf(w.lore, w.shards, w.tongue, w.lang);
    w.voice = voiceOf(g, w.lore, w.tongue);   // C-05: its voice
    w.tradition = traditionOf(g, w.lore);   // C-04: its music and the form and length of each piece, for the list
    w.pieceForm.assign(w.shards.size(), -1); w.pieceSeconds.assign(w.shards.size(), 0.0);
    for (size_t i = 0; i < w.shards.size(); i++) if (w.shards[i].music) { Piece pc; pieceOf(w.tradition, w.shards[i], pc); w.pieceForm[i] = pc.form; w.pieceSeconds[i] = pc.seconds; }
    w.valid = true;
    return true;
}

bool Game::openShardWorld(const std::string& key) {
    if (shardWorld.valid && shardWorld.key == key) return true;
    stopPiece(); stopSpeech();
    shardWorld = ShardWorld(); shardWorld.key = key;
    int64_t sx, sy, sz; if (!Guide::parseStarKey(key, sx, sy, sz)) return false;
    int bi = bodyOfKey(key);
    Star s; if (!starInSector(sx, sy, sz, s, true)) return false;
    return buildShardWorld(shardWorld, s, bi);
}

// C-04: the piece of the open world onto the synth, from the top; off it (the pointers cleared before anything they point at can move)
void Game::startPiece() {
    audio.piece = &shardWorld.piece; audio.tradition = &shardWorld.tradition; audio.pieceSpeed = 1;
    audio.pieceStart = true; audio.piecePause = false; audio.pieceDone = false; audio.pieceBeat = -1;
    pieceT = 0; pieceSyncBeat = -1; piecePaused = false;
    if (humBeforePiece < 0) humBeforePiece = audio.hum;   // the ship's hum ducked under the music
    audio.hum = std::min(audio.hum, 0.2);
}
void Game::stopPiece() {
    audio.piece = nullptr; audio.tradition = nullptr;
    audio.pieceStart = false; audio.piecePause = false; audio.pieceBeat = -1;
    pieceT = -1; pieceSyncBeat = -1; piecePaused = false;
    if (humBeforePiece >= 0 && !audio.speech) { audio.hum = humBeforePiece; humBeforePiece = -1; }
}
double Game::pieceBeatNow() const { return pieceT >= 0 ? pieceT * shardWorld.piece.bpm / 60.0 : 0; }
bool Game::pieceEnded() const { return pieceT >= 0 && (audio.pieceDone || pieceBeatNow() >= shardWorld.piece.beats); }

// C-05: the shard on the screen spoken from the top; off (the pointers cleared before anything they point at can move)
void Game::startSpeech() {
    audio.speech = &shardWorld.speech; audio.voice = &shardWorld.voice;
    audio.speechStart = true; audio.speechPause = false; audio.speechDone = false; audio.speechT = -1;
    speechT = 0; speechSyncT = -1; speechPaused = false;
    if (humBeforePiece < 0) humBeforePiece = audio.hum;   // the ship's hum ducked under the voice
    audio.hum = std::min(audio.hum, 0.2);
}
void Game::stopSpeech() {
    audio.speech = nullptr; audio.voice = nullptr;
    audio.speechStart = false; audio.speechPause = false; audio.speechT = -1;
    speechT = -1; speechSyncT = -1; speechPaused = false;
    if (humBeforePiece >= 0 && !audio.piece) { audio.hum = humBeforePiece; humBeforePiece = -1; }
}
bool Game::speechEnded() const { return speechT >= 0 && (audio.speechDone || speechT >= shardWorld.speech.seconds); }
// a first reading done: the shard is read at the world's current count
void Game::markShardRead() {
    if (!shardFresh || !shardWorld.valid || shardHeld.empty()) return;
    shardFresh = false;
    guide.decoded[shardWorld.key + "/S" + std::to_string(shardHeld[shardSel])] = (int)shardHeld.size();
    guide.save(guidePath);
    audio.beep = 1;
}

void Game::openShards() {
    std::set<std::string> worlds;
    for (const std::string& k : guide.shards) { size_t p = k.rfind("/S"); if (p != std::string::npos) worlds.insert(k.substr(0, p)); }
    shardWorldKeys.assign(worlds.begin(), worlds.end());
    radarOff();   // C-07: the decoder takes the synth
    shardsLevel = 0; shardFresh = false; stopPiece(); stopSpeech();
    if (shardWorldSel >= (int)shardWorldKeys.size()) shardWorldSel = 0;
    // the world under the ship first, when its shards are here
    if (sys.valid && ship.parkedBody >= 0) {
        std::string here = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, ship.parkedBody);
        for (size_t i = 0; i < shardWorldKeys.size(); i++) if (shardWorldKeys[i] == here) shardWorldSel = (int)i;
    }
    guideReturn = GameState::SPACE; returnState = GameState::SPACE;
    state = GameState::SHARDS;
    audio.beep = 4;
}

// a world's shard onto the screen: a text speaks from the top (C-05), the computer reading it as it goes when it has not at
// this share, else the words whole at once; a piece plays
void Game::openShard(int sel) {
    if (!shardWorld.valid || shardHeld.empty()) return;
    int n = (int)shardHeld.size();
    shardSel = ((sel % n) + n) % n;
    int idx = shardHeld[shardSel];
    stopPiece(); stopSpeech(); shardFresh = false;
    std::string skey = shardWorld.key + "/S" + std::to_string(idx);
    if (shardWorld.shards[idx].music) {   // C-04: a piece plays from the top; heard once, it is read for good
        shardWords.clear();
        pieceOf(shardWorld.tradition, shardWorld.shards[idx], shardWorld.piece);
        startPiece();
        if (!guide.decoded.count(skey)) { guide.decoded[skey] = SHARDS_PER_WORLD; guide.save(guidePath); }
        shardsLevel = 2; audio.beep = 4;
        return;
    }
    decodeShard(shardWorld.shards[idx], shardWorld.lang, shardWorld.tongue, n, shardWords);
    auto d = guide.decoded.find(skey);
    shardFresh = d == guide.decoded.end() || d->second < n;
    speechOf(shardWorld.voice, shardWords, shardWorld.shards[idx].seed, shardWorld.speech);   // C-05: the recording speaks
    startSpeech();
    if (shardFresh) audio.beep = 4;
    shardsLevel = 2;
}

void Game::updateShards(const Input& in, double realDt) {
    int nw = (int)shardWorldKeys.size();
    if (shardsLevel == 0) {
        if (in.wasPressed(KEY_ESCAPE) || (nw == 0 && enterKey(in))) { stopPiece(); state = GameState::SPACE; return; }
        if (nw == 0) return;
        if (in.wasPressed(KEY_UP)) shardWorldSel = (shardWorldSel + nw - 1) % nw;
        if (in.wasPressed(KEY_DOWN)) shardWorldSel = (shardWorldSel + 1) % nw;
        if (!enterKey(in)) return;
        if (!openShardWorld(shardWorldKeys[shardWorldSel])) { status("THE DECODER CANNOT FIND THAT WORLD", 3); audio.beep = 3; return; }
        shardHeld.clear();
        std::string prefix = shardWorld.key + "/S";
        for (const std::string& k : guide.shards) if (k.compare(0, prefix.size(), prefix) == 0) { int i = atoi(k.c_str() + prefix.size()); if (i >= 0 && i < (int)shardWorld.shards.size()) shardHeld.push_back(i); }
        std::sort(shardHeld.begin(), shardHeld.end(), [&](int a, int b) { int ya = shardWorld.shards[a].year, yb = shardWorld.shards[b].year; return ya != yb ? ya < yb : a < b; });
        shardSel = 0; shardsLevel = 1; audio.beep = 4;
    } else if (shardsLevel == 1) {
        int n = (int)shardHeld.size();
        if (in.wasPressed(KEY_ESCAPE)) { shardsLevel = 0; return; }
        if (n == 0) return;
        if (in.wasPressed(KEY_UP)) shardSel = (shardSel + n - 1) % n;
        if (in.wasPressed(KEY_DOWN)) shardSel = (shardSel + 1) % n;
        if (in.wasPressed(KEY_PAGE_UP)) shardSel = std::max(0, shardSel - SHARD_ROWS);
        if (in.wasPressed(KEY_PAGE_DOWN)) shardSel = std::min(n - 1, shardSel + SHARD_ROWS);
        if (enterKey(in)) openShard(shardSel);
    } else if (shardWorld.valid && !shardHeld.empty() && shardWorld.shards[shardHeld[shardSel]].music) {   // C-04: a piece playing
        if (audio.pieceBeat >= 0 && audio.pieceBeat != pieceSyncBeat) { pieceSyncBeat = audio.pieceBeat; pieceT = audio.pieceBeat * 60.0 / shardWorld.piece.bpm; }   // the synth's clock when it runs
        else if (pieceT >= 0 && !piecePaused && !pieceEnded()) pieceT += realDt;
        if (in.wasPressed(KEY_ESCAPE)) { stopPiece(); shardsLevel = 1; return; }
        if (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_UP)) { openShard(shardSel - 1); return; }
        if (in.wasPressed(KEY_RIGHT) || in.wasPressed(KEY_DOWN)) { openShard(shardSel + 1); return; }
        if (in.wasPressed(KEY_SPACE) && !pieceEnded()) { piecePaused = !piecePaused; audio.piecePause = piecePaused; }
        if (enterKey(in) && pieceEnded()) startPiece();
        if (in.wasPressed(KEY_N) && !in.ctrl()) {
            textShardKey = shardWorld.key + "/S" + std::to_string(shardHeld[shardSel]);
            auto nm = guide.names.find(textShardKey);
            guideReturn = GameState::SHARDS;
            beginTextEntry("NAME THE PIECE", 7, nm != guide.names.end() ? upper(nm->second) : std::string());
        }
    } else {   // C-05: a text speaking; on a first reading at this share the words resolve as they are spoken and the end reads the shard
        if (audio.speechT >= 0 && audio.speechT != speechSyncT) { speechSyncT = audio.speechT; speechT = audio.speechT; }   // the synth's clock when it runs
        else if (speechT >= 0 && !speechPaused && !speechEnded()) speechT += realDt;
        if (shardFresh && speechEnded()) markShardRead();
        bool speaking = speechT >= 0 && !speechEnded();
        if (in.wasPressed(KEY_ESCAPE)) { stopSpeech(); shardsLevel = 1; return; }   // a first reading left early is not read: it waits again next time
        if (enterKey(in)) { if (speaking) { stopSpeech(); markShardRead(); } else startSpeech(); return; }   // Enter skips to the end, or speaks it again
        if (in.wasPressed(KEY_SPACE) && speaking) { speechPaused = !speechPaused; audio.speechPause = speechPaused; }
        if (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_UP)) openShard(shardSel - 1);
        else if (in.wasPressed(KEY_RIGHT) || in.wasPressed(KEY_DOWN)) openShard(shardSel + 1);
    }
}

void Game::renderShards() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 225);
    int nw = (int)shardWorldKeys.size();
    if (shardsLevel == 0) {
        drawText(canvas, 6, 4, "THE SHARD DECODER", HUD_AMBER);
        if (nw == 0) {
            drawTextCentered(canvas, UW / 2, 80, "NO SHARD TAKEN YET", HUD_GREEN);
            drawTextCentered(canvas, UW / 2, 92, "THEY LIE IN THE RUINS OF THE OLD PEOPLES,", HUD_DIM);
            drawTextCentered(canvas, UW / 2, 101, "ON THE FLOORS OF THE ROOMS, TAKEN BY HAND", HUD_DIM);
        } else {
            int page = shardWorldSel / WORLD_ROWS, pages = (nw + WORLD_ROWS - 1) / WORLD_ROWS;
            std::string head = pages > 1 ? fmt("%d WORLDS  PAGE %d/%d", nw, page + 1, pages) : fmt("%d WORLD%s", nw, nw == 1 ? "" : "S");
            drawText(canvas, UW - 6 - textWidth(head.c_str()), 4, head.c_str(), HUD_DIM);
            for (int i = page * WORLD_ROWS; i < nw && i < (page + 1) * WORLD_ROWS; i++) {
                int y = 18 + (i - page * WORLD_ROWS) * 19;
                const std::string& key = shardWorldKeys[i];
                bool sel = i == shardWorldSel;
                int held = shardsHeldOf(key), read = 0;
                std::string prefix = key + "/S";
                for (const auto& d : guide.decoded) if (d.first.compare(0, prefix.size(), prefix) == 0) read++;
                int64_t sx = 0, sy = 0, sz = 0; Guide::parseStarKey(key, sx, sy, sz);
                drawText(canvas, 16, y, fmt("%s  (%+lld %+lld %+lld / %d)", upper(worldNameOfKey(key)).c_str(), (long long)sx, (long long)sy, (long long)sz, bodyOfKey(key)).c_str(), sel ? HUD_WHITE : HUD_GREEN);
                if (sel) drawText(canvas, 6, y, ">", HUD_AMBER);
                drawText(canvas, 16, y + 9, fmt("%d OF %d SHARDS   THE LANGUAGE %d%% LEARNT   %d READ", held, SHARDS_PER_WORLD, (int)std::lround(100 * languageShare(held)), read).c_str(), HUD_DIM);
            }
        }
        drawTextCentered(canvas, UW / 2, UH - 10, nw ? "UP/DOWN  ENTER THE WORLD'S SHARDS  ESC CLOSE" : "ESC CLOSE", HUD_DIM);
        return;
    }
    const std::string& key = shardWorld.key;
    int n = (int)shardHeld.size(), pct = (int)std::lround(100 * languageShare(n));
    std::string name = upper(worldNameOfKey(key));
    if (shardsLevel == 1) {
        drawText(canvas, 6, 4, fmt("%s - %d OF %d SHARDS - THE LANGUAGE %d%% LEARNT", trunc(name, 18).c_str(), n, SHARDS_PER_WORLD, pct).c_str(), HUD_AMBER);
        int page = shardSel / SHARD_ROWS, pages = (n + SHARD_ROWS - 1) / SHARD_ROWS;
        bool anyPending = false;
        for (int i = page * SHARD_ROWS; i < n && i < (page + 1) * SHARD_ROWS; i++) {
            int y = 16 + (i - page * SHARD_ROWS) * 9;
            int idx = shardHeld[i]; const Shard& s = shardWorld.shards[idx];
            auto d = guide.decoded.find(key + "/S" + std::to_string(idx));
            int readAt = d == guide.decoded.end() ? 0 : d->second;   // the words as the computer last read them: an unread shard is all in the people's tongue
            std::string what;
            if (s.music) {   // C-04: a piece, by its name once given, else its form and length
                auto nm = guide.names.find(key + "/S" + std::to_string(idx));
                what = nm != guide.names.end() ? fmt("'%s' - %s", upper(nm->second).c_str(), PIECE_FORM_NAMES[shardWorld.pieceForm[idx]]) : fmt("%s, %d S", PIECE_FORM_NAMES[shardWorld.pieceForm[idx]], (int)std::lround(shardWorld.pieceSeconds[idx]));
            } else { std::vector<DecodedWord> w; decodeShard(s, shardWorld.lang, shardWorld.tongue, readAt, w); what = decodedText(w); }
            bool sel = i == shardSel, pending = s.music ? readAt == 0 : readAt < n;
            anyPending = anyPending || pending;
            drawText(canvas, 16, y, fmt("YEAR %3d  %s", s.year, trunc(upper(what), 37).c_str()).c_str(), sel ? HUD_WHITE : (readAt == 0 ? HUD_DIM : HUD_GREEN));
            if (pending) drawText(canvas, UW - 12, y, "*", HUD_AMBER);
            if (sel) drawText(canvas, 6, y, ">", HUD_AMBER);
        }
        if (pages > 1) { std::string pg = fmt("PAGE %d/%d", page + 1, pages); drawText(canvas, UW - 6 - textWidth(pg.c_str()), 4, pg.c_str(), HUD_DIM); }
        drawTextCentered(canvas, UW / 2, UH - 10, anyPending ? "UP/DOWN  ENTER READ  ESC BACK   * UNREAD" : "UP/DOWN  ENTER READ  ESC BACK", HUD_DIM);
        return;
    }
    if (shardWorld.shards[shardHeld[shardSel]].music) { renderPiece(); return; }   // C-04
    // a shard: the voice named, the words laid out at the text's width, each in its own colour (a name amber, a word read
    // white, one not yet learnt dim in the people's tongue); on a first reading they resolve as the recording speaks them, the
    // one being spoken flickering; on a reading done the word being spoken is underlined
    int idx = shardHeld[shardSel]; const Shard& s = shardWorld.shards[idx];
    drawText(canvas, 6, 4, fmt("%s - SHARD %d OF %d", trunc(name, 30).c_str(), shardSel + 1, n).c_str(), HUD_AMBER);
    drawText(canvas, 6, 13, fmt("YEAR %d OF %s", s.year, upper(shardWorld.lore.city).c_str()).c_str(), HUD_DIM);
    drawText(canvas, 6, 22, voiceLine(shardWorld.voice).c_str(), HUD_DIM);   // C-05
    int total = (int)shardWords.size();
    bool speaking = speechT >= 0 && !speechEnded();
    int spoken = speaking ? speechWordAt(shardWorld.speech, speechT) : -1;
    int cursor = shardFresh && speaking ? std::max(0, spoken) : total;
    Rng flick((uint64_t)(realTime * 20));
    int x = 12, y = 33; const int maxX = UW - 12;
    for (int i = 0; i < total; i++) {
        const DecodedWord& w = shardWords[i];
        std::string shown; uint32_t col;
        if (i < cursor) { shown = w.pre + (w.known ? w.ours : w.theirs) + w.post; col = w.name ? HUD_AMBER : (w.known ? HUD_WHITE : HUD_DIM); }
        else if (i == cursor && cursor < total) { shown = w.theirs; for (char& c : shown) c = (char)('A' + flick.irange(26)); col = HUD_GREEN; }
        else { shown = w.pre + w.theirs + w.post; col = HUD_DIM; }
        shown = upper(shown);
        int tw = textWidth(shown.c_str());
        if (x > 12 && x + tw > maxX) { x = 12; y += 9; }
        if (y > UH - 36) break;
        drawText(canvas, x, y, shown.c_str(), col);
        if (i == spoken && !shardFresh) fillRectRGB(canvas, x, y + 7, x + tw - 1, y + 7, HUD_GREEN);   // the word being spoken, underlined
        x += tw + FONT_ADV;
    }
    int read = 0; for (const DecodedWord& w : shardWords) if (w.known) read++;
    if (speaking) {
        double u = clampd(speechT / std::max(shardWorld.speech.seconds, 1e-9), 0, 1);
        if (shardFresh) {
            drawText(canvas, 12, UH - 22, fmt("%s %3d%%", speechPaused ? "PAUSED  " : "DECODING", (int)(100 * u)).c_str(), HUD_AMBER);
            const int bx0 = 100, bx1 = UW - 12;
            drawRectRGB(canvas, bx0, UH - 22, bx1, UH - 16, HUD_DIM);
            if (u > 0) fillRectRGB(canvas, bx0 + 1, UH - 21, bx0 + 1 + (int)((bx1 - bx0 - 2) * u), UH - 17, HUD_GREEN);
            drawTextCentered(canvas, UW / 2, UH - 10, "ENTER SKIPS THE WAIT  SPACE PAUSE  ESC STOPS", HUD_DIM);
        } else {
            drawText(canvas, 12, UH - 22, fmt("%s %3d%%   %d OF %d WORDS READ", speechPaused ? "PAUSED " : "PLAYING", (int)(100 * u), read, total).c_str(), HUD_GREEN);
            drawTextCentered(canvas, UW / 2, UH - 10, "ENTER SKIPS  SPACE PAUSE  LEFT/RIGHT  ESC BACK", HUD_DIM);
        }
    } else {
        drawText(canvas, 12, UH - 22, fmt("%d OF %d WORDS READ   THE LANGUAGE %d%% (%d SHARD%s)", read, total, pct, n, n == 1 ? "" : "S").c_str(), HUD_GREEN);
        drawTextCentered(canvas, UW / 2, UH - 10, "ENTER PLAYS IT AGAIN  LEFT/RIGHT ANOTHER  ESC BACK", HUD_DIM);
    }
}

// C-04: a piece on the screen: its name or form, the tradition in words, the notes as a roll across the piece's length (the lead
// white, the second voice green, the third cyan, the drum amber ticks below; what the playhead has passed in colour, the rest
// dim), the cycles marked; the footer the play's progress and the keys
void Game::renderPiece() {
    const std::string& key = shardWorld.key;
    int n = (int)shardHeld.size(), idx = shardHeld[shardSel]; const Shard& s = shardWorld.shards[idx];
    const Piece& P = shardWorld.piece; const Tradition& T = shardWorld.tradition;
    std::string name = upper(worldNameOfKey(key));
    drawText(canvas, 6, 4, fmt("%s - SHARD %d OF %d", trunc(name, 30).c_str(), shardSel + 1, n).c_str(), HUD_AMBER);
    drawText(canvas, 6, 13, fmt("YEAR %d OF %s", s.year, upper(shardWorld.lore.city).c_str()).c_str(), HUD_DIM);
    auto nm = guide.names.find(key + "/S" + std::to_string(idx));
    std::string title = nm != guide.names.end() ? fmt("'%s' - %s, %d S", upper(nm->second).c_str(), PIECE_FORM_NAMES[P.form], (int)std::lround(P.seconds))
                                                : fmt("%s OF THE %s, %d S", PIECE_FORM_NAMES[P.form], upper(shardWorld.lore.people).c_str(), (int)std::lround(P.seconds));
    drawText(canvas, 12, 24, trunc(title, 52).c_str(), HUD_WHITE);
    drawText(canvas, 12, 34, traditionLine(T).c_str(), HUD_DIM);
    drawText(canvas, 12, 43, cycleLine(T, P.bpm).c_str(), HUD_DIM);
    drawText(canvas, 12, 52, timbresLine(T, pieceHasDrum(P)).c_str(), HUD_DIM);
    const int rx0 = 12, rx1 = UW - 12, ry0 = 66, ry1 = UH - 34;
    double beat = pieceBeatNow();
    int minD = 1000, maxD = -1000;
    for (const Note& nt : P.notes) if (nt.voice >= 0) { minD = std::min(minD, nt.degree); maxD = std::max(maxD, nt.degree); }
    if (minD > maxD) { minD = 0; maxD = 1; }
    double span = std::max(1, maxD - minD);
    auto xOf = [&](double t) { return rx0 + (int)std::lround((rx1 - rx0) * clampd(t / std::max(P.beats, 1e-9), 0, 1)); };
    auto yOf = [&](int d) { return ry1 - 8 - (int)std::lround((ry1 - 8 - ry0) * (d - minD) / span); };
    const uint32_t grid = rgb(28, 40, 32), dimNote = HUD_DIM;
    for (int c = 0; c <= P.cycles; c++) { int x = xOf(c * T.beats); fillRectRGB(canvas, x, ry0, x, ry1, grid); }
    for (const Note& nt : P.notes) {
        int x0 = xOf(nt.t), x1 = std::max(x0, xOf(nt.t + nt.dur) - 1);
        bool played = nt.t <= beat;
        if (nt.voice < 0) { int y = ry1 - (nt.degree == 0 ? 1 : 4); fillRectRGB(canvas, x0, y, x0, y + 1, played ? HUD_AMBER : dimNote); continue; }
        int y = yOf(nt.degree);
        uint32_t col = !played ? dimNote : (nt.voice == 0 ? HUD_WHITE : (nt.voice == 1 ? HUD_GREEN : HUD_CYAN));
        fillRectRGB(canvas, x0, y - 1, x1, y, col);                                  // two rows a note
        if (nt.voice == 0 && played) fillRectRGB(canvas, x0, y - 2, x1, y - 2, col);   // the lead three
    }
    int px = xOf(beat); fillRectRGB(canvas, px, ry0, px, ry1, HUD_AMBER);
    bool ended = pieceEnded();
    if (!ended) {
        double u = clampd(beat / std::max(P.beats, 1e-9), 0, 1);
        drawText(canvas, 12, UH - 22, fmt("%s %3d%%", piecePaused ? "PAUSED " : "PLAYING", (int)(100 * u)).c_str(), HUD_AMBER);
        const int bx0 = 100, bx1 = UW - 12;
        drawRectRGB(canvas, bx0, UH - 22, bx1, UH - 16, HUD_DIM);
        if (u > 0) fillRectRGB(canvas, bx0 + 1, UH - 21, bx0 + 1 + (int)((bx1 - bx0 - 2) * u), UH - 17, HUD_GREEN);
        drawTextCentered(canvas, UW / 2, UH - 10, "N NAME  SPACE PAUSE  LEFT/RIGHT ANOTHER  ESC BACK", HUD_DIM);
    } else {
        drawText(canvas, 12, UH - 22, fmt("THE PIECE ENDED   %d NOTES IN %d CYCLES", (int)P.notes.size(), P.cycles).c_str(), HUD_GREEN);
        drawTextCentered(canvas, UW / 2, UH - 10, "ENTER PLAYS IT AGAIN  N NAME  LEFT/RIGHT  ESC BACK", HUD_DIM);
    }
}

std::string Game::testShardsInfo() const {
    int read = 0; for (const DecodedWord& w : shardWords) if (w.known) read++;
    std::string piece;
    if (shardsLevel == 2 && shardWorld.valid && !shardHeld.empty() && shardWorld.shards[shardHeld[shardSel]].music)
        piece = fmt("; piece: %s, %.0f s, %zu notes, beat %.1f of %.0f, %s", PIECE_FORM_NAMES[shardWorld.piece.form], shardWorld.piece.seconds, shardWorld.piece.notes.size(), pieceBeatNow(), shardWorld.piece.beats,
                    pieceEnded() ? "ended" : (piecePaused ? "paused" : (audio.piece ? "playing" : "stopped")));
    std::string speech;   // C-05
    if (shardsLevel == 2 && shardWorld.valid && !shardHeld.empty() && !shardWorld.shards[shardHeld[shardSel]].music)
        speech = fmt("; speech: %.1f of %.1f s, word %d, %s", speechT >= 0 ? speechT : 0.0, shardWorld.speech.seconds, speechT >= 0 ? speechWordAt(shardWorld.speech, speechT) : -1,
                     speechT < 0 ? "stopped" : (speechEnded() ? "ended" : (speechPaused ? "paused" : "speaking")));
    return fmt("decoder: level %d, %zu worlds, %zu held of %s, shard %d: %d of %zu words read, %s%s%s", shardsLevel, shardWorldKeys.size(), shardHeld.size(), shardWorld.valid ? shardWorld.key.c_str() : "-",
               shardSel, read, shardWords.size(), shardFresh ? fmt("decoding %.0f%%", 100 * clampd(speechT / std::max(shardWorld.speech.seconds, 1e-9), 0, 1)).c_str() : "idle", speech.c_str(), piece.c_str());
}

bool Game::testShardsOpenIndex(int idx) {
    if (shardsLevel != 1 || !shardWorld.valid) return false;
    for (size_t i = 0; i < shardHeld.size(); i++) if (shardHeld[i] == idx) { openShard((int)i); return true; }
    return false;
}
