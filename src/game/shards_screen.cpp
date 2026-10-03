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
// is recorded as read at the world's full count, so it never waits again. The ship's hum is ducked under either. C-10: a
// shard that is a star chart (`galaxy/charts.h`) draws itself here beside its caption: the hemisphere of the people's sky
// round the star they marked, their figures and their names (words of the language, resolving as it is learnt); a first
// reading draws it over a few seconds; Enter targets the marked star and O holds the chart up to the sky (`drawChartOverlay`).
#include "game.h"
#include "ui.h"
#include "core/rng.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
const int WORLD_ROWS = 8;    // worlds per page (two lines each)
const int SHARD_ROWS = 16;   // a world's shards per page
int bodyOfKey(const std::string& key) { size_t sl = key.find('/'); return sl == std::string::npos ? -1 : atoi(key.c_str() + sl + 1); }
}

int Game::shardsHeldOf(const std::string& key, int which) const {   // C-13: of that people's fifty; C-14: the explorer's own and the lent
    std::string prefix = key + "/S"; int n = 0;
    for (const std::string& k : guide.shards) if (k.compare(0, prefix.size(), prefix) == 0 && shardPeopleOf(atoi(k.c_str() + prefix.size())) == which) n++;
    return n + lentOf(key, which);
}
int Game::lentOf(const std::string& key, int which) const {   // C-14
    std::string prefix = key + "/S"; int n = 0;
    for (const std::string& k : guide.lent) if (k.compare(0, prefix.size(), prefix) == 0 && shardPeopleOf(atoi(k.c_str() + prefix.size())) == which && !guide.shards.count(k)) n++;
    return n;
}
bool Game::shardLent(int local) const { return guide.isLent(heldShardKey(local)); }
const std::string* Game::shardNameOf(const std::string& key, bool& foreign) const {
    foreign = false;
    auto it = guide.names.find(key);
    if (it != guide.names.end()) return &it->second;
    it = guide.inbox.find(key);
    if (it != guide.inbox.end()) { foreign = true; return &it->second; }
    return nullptr;
}
std::string Game::heldShardKey(int local) const { return shardWorld.key + "/S" + std::to_string(shardWorld.base + local); }
std::string Game::endedKey() const { return shardWorld.key + (shardWorld.which ? "/P1" : ""); }
std::string Game::shardWorldTitle(int maxName) const {
    std::string name = upper(worldNameOfKey(shardWorld.key));
    return shardWorld.peoples > 1 ? trunc(name, 18) + " - THE " + trunc(upper(shardWorld.lore.people), 9) : trunc(name, maxName);
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
    std::map<std::string, int> held;   // C-13: per world and people; C-14: the lent count with the own
    std::set<std::string> all(guide.shards);
    for (const std::string& k : guide.lent) all.insert(k);
    for (const std::string& k : all) { size_t p = k.rfind("/S"); if (p != std::string::npos) held[k.substr(0, p) + "/P" + std::to_string(shardPeopleOf(atoi(k.c_str() + p + 2)))]++; }
    for (const std::string& k : all) {
        size_t p = k.rfind("/S"); if (p == std::string::npos) continue;
        auto d = guide.decoded.find(k);
        if (d == guide.decoded.end() || d->second < held[k.substr(0, p) + "/P" + std::to_string(shardPeopleOf(atoi(k.c_str() + p + 2)))]) return true;
    }
    return false;
}

// the world's system from its key, its lore, its fifty and its language; kept while the same world is read
// C-07: a world's shards, language, voice and tradition built for the decoder or the radar (the key is the caller's)
bool Game::buildShardWorld(ShardWorld& w, const Star& s, int bi, int which) {
    std::string key = w.key;
    w = ShardWorld(); w.key = key; w.which = which;
    if (!s.valid) return false;
    w.sys.generate(s);
    if (bi < 0 || bi >= (int)w.sys.bodies.size()) return false;
    const Body& b = w.sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    w.body = bi;
    w.lore = loreOf(w.sys, b, g, which);
    w.which = w.lore.which; w.peoples = w.lore.peoples; w.base = shardWorldIndex(w.which, 0);   // C-13: the people's fifty, its guide indices from `base`
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

bool Game::openShardWorld(const std::string& key, int which) {
    if (shardWorld.valid && shardWorld.key == key && shardWorld.which == which) return true;
    stopPiece(); stopSpeech();
    shardWorld = ShardWorld(); shardWorld.key = key;
    int64_t sx, sy, sz; if (!Guide::parseStarKey(key, sx, sy, sz)) return false;
    int bi = bodyOfKey(key);
    Star s; if (!starInSector(sx, sy, sz, s, true)) return false;
    return buildShardWorld(shardWorld, s, bi, which);
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
    int idx = shardHeld[shardSel];
    guide.decoded[heldShardKey(idx)] = (int)shardHeld.size();
    if (shardWorld.shards[idx].last && guide.ended.insert(endedKey()).second) {   // C-12: the world's last recording read: the log notes it and the timeline closes (`logEvent` saves the guide)
        logEvent("SHARD", fmt("THE LAST RECORDING OF THE %s READ: THE RECORD OF %s ENDS IN YEAR %d OF %s", upper(shardWorld.lore.people).c_str(), upper(worldNameOfKey(shardWorld.key)).c_str(), shardWorld.shards[idx].year, upper(shardWorld.lore.city).c_str()));
    } else if (shardWorld.shards[idx].chart && shardWorld.chart.valid) {   // C-10: a chart read goes to the log with the star it marks (the system only: what is there is theirs to tell)
        const ChartMark& m = shardWorld.chart.mark;
        logEvent("CHART", fmt("A STAR CHART OF THE %s READ: THEY MARKED THE STAR AT SECTOR %lld %lld %lld (%s), %.0f LY FROM %s", upper(shardWorld.lore.people).c_str(), (long long)m.star.sx, (long long)m.star.sy, (long long)m.star.sz, STAR_CLASSES[m.star.cls].code, m.ly, upper(worldNameOfKey(shardWorld.key)).c_str()));
    } else guide.save(guidePath);
    audio.beep = 1;
}

void Game::openShards() {
    // C-13: an entry a people whose shards are held; a world of two peoples names each (its system generated once here, so a
    // world of two is named as such from the first shard of either)
    std::map<std::string, std::set<int>> held;
    for (const std::string& k : guide.shards) { size_t p = k.rfind("/S"); if (p != std::string::npos) held[k.substr(0, p)].insert(shardPeopleOf(atoi(k.c_str() + p + 2))); }
    for (const std::string& k : guide.lent) { size_t p = k.rfind("/S"); if (p != std::string::npos) held[k.substr(0, p)].insert(shardPeopleOf(atoi(k.c_str() + p + 2))); }   // C-14: a friend's shards list their world too
    shardWorldKeys.clear(); shardWorldWhich.clear(); shardWorldLabels.clear();
    for (const auto& kv : held) {
        int peoples = 1; std::string names[2];
        int64_t sx, sy, sz; Star s;
        if (Guide::parseStarKey(kv.first, sx, sy, sz) && starInSector(sx, sy, sz, s, true)) {
            StarSystem wsys; wsys.generate(s); int bi = bodyOfKey(kv.first);
            if (bi >= 0 && bi < (int)wsys.bodies.size()) { BodyGen g = BodyGen::make(wsys.bodies[bi]); if (peoplesOf(g) > 1) { peoples = 2; for (int k = 0; k < 2; k++) names[k] = peopleNameOf(peopleGen(g, k)); } }
        }
        size_t before = shardWorldKeys.size();
        for (int k = 0; k < peoples; k++) {
            if (!kv.second.count(k)) continue;
            shardWorldKeys.push_back(kv.first); shardWorldWhich.push_back(k); shardWorldLabels.push_back(peoples > 1 ? "THE " + upper(names[k]) : std::string());
        }
        if (shardWorldKeys.size() == before) { shardWorldKeys.push_back(kv.first); shardWorldWhich.push_back(0); shardWorldLabels.push_back(std::string()); }   // a key the world cannot place: under the first people
    }
    radarOff();   // C-07: the decoder takes the synth
    shardsLevel = 0; shardFresh = false; stopPiece(); stopSpeech();
    if (shardWorldSel >= (int)shardWorldKeys.size()) shardWorldSel = 0;
    // the world under the ship first, when its shards are here
    if (sys.valid && ship.parkedBody >= 0) {
        std::string here = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, ship.parkedBody);
        for (size_t i = 0; i < shardWorldKeys.size(); i++) if (shardWorldKeys[i] == here) { shardWorldSel = (int)i; break; }
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
    std::string skey = heldShardKey(idx);
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
    chartT = -1;
    if (shardWorld.shards[idx].chart) {   // C-10: a chart draws itself (no voice: a map, not a sound); a first reading at this share takes CHART_READ_S seconds
        const Body& b = shardWorld.sys.bodies[shardWorld.body];
        chartOf(shardWorld.sys, b, BodyGen::make(b), shardWorld.lore, idx, shardWorld.chart);
        if (shardFresh) { chartT = 0; audio.beep = 4; }
        shardsLevel = 2;
        return;
    }
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
        if (!openShardWorld(shardWorldKeys[shardWorldSel], shardWorldWhich[shardWorldSel])) { status("THE DECODER CANNOT FIND THAT WORLD", 3); audio.beep = 3; return; }
        shardHeld.clear();
        std::string prefix = shardWorld.key + "/S";
        std::set<int> locals;   // C-13: this people's; C-14: the explorer's own and the lent, each once
        for (const std::set<std::string>* src : {&guide.shards, &guide.lent})
            for (const std::string& k : *src) if (k.compare(0, prefix.size(), prefix) == 0) { int i = atoi(k.c_str() + prefix.size()); if (shardPeopleOf(i) != shardWorld.which) continue; i = shardLocalOf(i); if (i >= 0 && i < (int)shardWorld.shards.size()) locals.insert(i); }
        shardHeld.assign(locals.begin(), locals.end());
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
            textShardKey = heldShardKey(shardHeld[shardSel]);
            auto nm = guide.names.find(textShardKey);
            guideReturn = GameState::SHARDS;
            beginTextEntry("NAME THE PIECE", 7, nm != guide.names.end() ? upper(nm->second) : std::string());
        }
    } else if (shardWorld.valid && !shardHeld.empty() && shardWorld.shards[shardHeld[shardSel]].chart) {   // C-10: a chart on the screen
        int idx = shardHeld[shardSel];
        if (chartT >= 0) { chartT += realDt; if (chartT >= CHART_READ_S) { markShardRead(); chartT = -1; } }
        bool reading = chartT >= 0;
        if (in.wasPressed(KEY_ESCAPE)) { chartT = -1; shardsLevel = 1; return; }   // a first reading left early is not read: it waits again next time
        if (enterKey(in)) {
            if (reading) { chartT = -1; markShardRead(); }   // Enter skips the wait
            else if (shardWorld.chart.valid) { Star full; if (starInSector(shardWorld.chart.mark.star.sx, shardWorld.chart.mark.star.sy, shardWorld.chart.mark.star.sz, full, true)) setRemoteStar(full); }   // or targets the marked star
            return;
        }
        if (in.wasPressed(KEY_O) && !reading && shardWorld.chart.valid) { holdChart(!(chartUp && chartHeldKey == shardWorld.key && chartHeldIndex == shardWorld.base + idx)); return; }
        if (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_UP)) openShard(shardSel - 1);
        else if (in.wasPressed(KEY_RIGHT) || in.wasPressed(KEY_DOWN)) openShard(shardSel + 1);
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
                int which = shardWorldWhich[i], held = shardsHeldOf(key, which), lent = lentOf(key, which), read = 0;   // C-13: this people's; C-14: the lent with the own
                std::string prefix = key + "/S";
                for (const auto& d : guide.decoded) if (d.first.compare(0, prefix.size(), prefix) == 0 && shardPeopleOf(atoi(d.first.c_str() + prefix.size())) == which) read++;
                int64_t sx = 0, sy = 0, sz = 0; Guide::parseStarKey(key, sx, sy, sz);
                drawText(canvas, 16, y, fmt("%s  (%+lld %+lld %+lld / %d)%s", upper(worldNameOfKey(key)).c_str(), (long long)sx, (long long)sy, (long long)sz, bodyOfKey(key), shardWorldLabels[i].empty() ? "" : ("  " + shardWorldLabels[i]).c_str()).c_str(), sel ? HUD_WHITE : (nameIsForeign(key) ? HUD_CYAN : HUD_GREEN));   // C-14: a friend's name in cyan
                if (sel) drawText(canvas, 6, y, ">", HUD_AMBER);
                drawText(canvas, 16, y + 9, (lent > 0 ? fmt("%d OF %d SHARDS (%d LENT)   LANGUAGE %d%%   %d READ", held, SHARDS_PER_WORLD, lent, (int)std::lround(100 * languageShare(held)), read)
                                                      : fmt("%d OF %d SHARDS   THE LANGUAGE %d%% LEARNT   %d READ", held, SHARDS_PER_WORLD, (int)std::lround(100 * languageShare(held)), read)).c_str(), HUD_DIM);
            }
        }
        drawTextCentered(canvas, UW / 2, UH - 10, nw ? "UP/DOWN  ENTER THE WORLD'S SHARDS  ESC CLOSE" : "ESC CLOSE", HUD_DIM);
        return;
    }
    const std::string& key = shardWorld.key;
    int n = (int)shardHeld.size(), pct = (int)std::lround(100 * languageShare(n));
    std::string name = upper(worldNameOfKey(key));
    if (shardsLevel == 1) {
        drawText(canvas, 6, 4, (shardWorld.peoples > 1 ? fmt("%s - THE %s - %d OF %d - %d%% LEARNT", trunc(name, 12).c_str(), trunc(upper(shardWorld.lore.people), 9).c_str(), n, SHARDS_PER_WORLD, pct)   // C-13: the people named
                                                     : fmt("%s - %d OF %d SHARDS - THE LANGUAGE %d%% LEARNT", trunc(name, 18).c_str(), n, SHARDS_PER_WORLD, pct)).c_str(), HUD_AMBER);
        int page = shardSel / SHARD_ROWS, pages = (n + SHARD_ROWS - 1) / SHARD_ROWS;
        bool anyPending = false;
        for (int i = page * SHARD_ROWS; i < n && i < (page + 1) * SHARD_ROWS; i++) {
            int y = 16 + (i - page * SHARD_ROWS) * 9;
            int idx = shardHeld[i]; const Shard& s = shardWorld.shards[idx];
            auto d = guide.decoded.find(heldShardKey(idx));
            int readAt = d == guide.decoded.end() ? 0 : d->second;   // the words as the computer last read them: an unread shard is all in the people's tongue
            std::string what;
            if (s.music) {   // C-04: a piece, by its name once given (C-14: or a friend's), else its form and length
                bool foreign = false; const std::string* nm = shardNameOf(heldShardKey(idx), foreign);
                what = nm ? fmt("'%s' - %s", upper(*nm).c_str(), PIECE_FORM_NAMES[shardWorld.pieceForm[idx]]) : fmt("%s, %d S", PIECE_FORM_NAMES[shardWorld.pieceForm[idx]], (int)std::lround(shardWorld.pieceSeconds[idx]));
            } else { std::vector<DecodedWord> w; decodeShard(s, shardWorld.lang, shardWorld.tongue, readAt, w); what = (s.chart ? "A CHART: " : "") + decodedText(w); }   // C-10: a chart by its caption
            bool sel = i == shardSel, pending = s.music ? readAt == 0 : readAt < n;
            anyPending = anyPending || pending;
            drawText(canvas, 16, y, fmt("YEAR %3d  %s", s.year, trunc(upper(what), 37).c_str()).c_str(), sel ? HUD_WHITE : (shardLent(idx) ? HUD_CYAN : (readAt == 0 ? HUD_DIM : HUD_GREEN)));   // C-14: a friend's in cyan
            if (s.last && guide.ended.count(endedKey())) drawText(canvas, 16, y, fmt("YEAR %3d", s.year).c_str(), HUD_AMBER);   // C-12: the last recording, once read
            if (pending) drawText(canvas, UW - 12, y, "*", HUD_AMBER);
            if (sel) drawText(canvas, 6, y, ">", HUD_AMBER);
        }
        {   // C-12: the people's calendar and the timeline of what is held: the years from the founding along a rule, a tick a shard (read green, unread
            // dim, the one under the cursor white), the rule dotted past the latest held until the last recording is read, when it closes in amber
            const Lore& L = shardWorld.lore;
            drawText(canvas, 12, 161, upper(calendarLine(L)).c_str(), HUD_DIM);
            bool ended = guide.ended.count(endedKey()) > 0;
            const int tx0 = 12, tx1 = UW - 12, ty = 179;
            int lastYear = L.spanYears + 1, latest = 1;
            for (int i : shardHeld) latest = std::max(latest, shardWorld.shards[i].year);
            auto xOf = [&](int yr) { return tx0 + (int)std::lround((tx1 - tx0) * clampd((yr - 1) / (double)std::max(1, lastYear - 1), 0, 1)); };
            drawText(canvas, tx0, 168, "YEAR 1", HUD_DIM);
            if (ended) {
                fillRectRGB(canvas, tx0, ty, tx1, ty, HUD_DIM);
                fillRectRGB(canvas, tx1 - 1, ty - 3, tx1, ty + 3, HUD_AMBER);
                std::string endLabel = fmt("THE RECORD ENDS, YEAR %d", lastYear);
                drawText(canvas, tx1 - textWidth(endLabel.c_str()), 168, endLabel.c_str(), HUD_AMBER);
            } else {
                int xl = xOf(latest);
                fillRectRGB(canvas, tx0, ty, xl, ty, HUD_DIM);
                for (int x = xl + 3; x <= tx1; x += 3) fillRectRGB(canvas, x, ty, x, ty, HUD_DIM);
            }
            for (int i = 0; i < n; i++) {
                const Shard& s = shardWorld.shards[shardHeld[i]];
                bool read = guide.decoded.count(heldShardKey(shardHeld[i])) > 0;
                uint32_t col = i == shardSel ? HUD_WHITE : (s.last && ended && read ? HUD_AMBER : (shardLent(shardHeld[i]) ? HUD_CYAN : (read ? HUD_GREEN : HUD_DIM)));   // C-14: a friend's tick in cyan
                int x = xOf(s.year);
                fillRectRGB(canvas, x, ty - 3, x, ty + 3, col);
            }
        }
        if (pages > 1) { std::string pg = fmt("PAGE %d/%d", page + 1, pages); drawText(canvas, UW - 6 - textWidth(pg.c_str()), 4, pg.c_str(), HUD_DIM); }
        drawTextCentered(canvas, UW / 2, UH - 10, anyPending ? "UP/DOWN  ENTER READ  ESC BACK   * UNREAD" : "UP/DOWN  ENTER READ  ESC BACK", HUD_DIM);
        return;
    }
    if (shardWorld.shards[shardHeld[shardSel]].music) { renderPiece(); return; }   // C-04
    if (shardWorld.shards[shardHeld[shardSel]].chart) { renderChart(); return; }   // C-10
    // a shard: the voice named, the words laid out at the text's width, each in its own colour (a name amber, a word read
    // white, one not yet learnt dim in the people's tongue); on a first reading they resolve as the recording speaks them, the
    // one being spoken flickering; on a reading done the word being spoken is underlined
    int idx = shardHeld[shardSel]; const Shard& s = shardWorld.shards[idx];
    bool theLast = s.last && guide.ended.count(endedKey()) > 0;   // C-12: named as the last once read, not before
    std::string head = theLast ? fmt("%s - THE LAST RECORDING", shardWorldTitle(30).c_str()) : fmt("%s - SHARD %d OF %d", shardWorldTitle(30).c_str(), shardSel + 1, n);
    drawText(canvas, 6, 4, head.c_str(), HUD_AMBER);
    if (shardLent(idx)) drawText(canvas, 6 + textWidth(head.c_str()) + 2 * FONT_ADV, 4, "LENT", HUD_CYAN);   // C-14: a friend's, credited
    drawText(canvas, 6, 13, upper(shardDate(shardWorld.lore, s)).c_str(), HUD_DIM);   // C-12: in the people's reckoning
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
    std::string head = fmt("%s - SHARD %d OF %d", shardWorldTitle(30).c_str(), shardSel + 1, n);
    drawText(canvas, 6, 4, head.c_str(), HUD_AMBER);
    if (shardLent(idx)) drawText(canvas, 6 + textWidth(head.c_str()) + 2 * FONT_ADV, 4, "LENT", HUD_CYAN);   // C-14: a friend's, credited
    drawText(canvas, 6, 13, upper(shardDate(shardWorld.lore, s)).c_str(), HUD_DIM);   // C-12
    bool foreign = false; const std::string* nm = shardNameOf(heldShardKey(idx), foreign);   // C-14: a friend's name for it, in cyan
    std::string title = nm ? fmt("'%s' - %s, %d S", upper(*nm).c_str(), PIECE_FORM_NAMES[P.form], (int)std::lround(P.seconds))
                           : fmt("%s OF THE %s, %d S", PIECE_FORM_NAMES[P.form], upper(shardWorld.lore.people).c_str(), (int)std::lround(P.seconds));
    drawText(canvas, 12, 24, trunc(title, 52).c_str(), foreign ? HUD_CYAN : HUD_WHITE);
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
    int lentN = 0; for (int i : shardHeld) if (shardLent(i)) lentN++;   // C-14
    return fmt("decoder: level %d, %zu worlds, %zu held of %s (people %d of %d, %d lent), shard %d: %d of %zu words read, %s%s%s", shardsLevel, shardWorldKeys.size(), shardHeld.size(), shardWorld.valid ? shardWorld.key.c_str() : "-", shardWorld.which + 1, shardWorld.peoples, lentN,
               shardSel, read, shardWords.size(), shardFresh ? fmt("decoding %.0f%%", 100 * clampd(speechT / std::max(shardWorld.speech.seconds, 1e-9), 0, 1)).c_str() : "idle", speech.c_str(), piece.c_str());
}

bool Game::testShardsOpenIndex(int idx) {
    if (shardsLevel != 1 || !shardWorld.valid) return false;
    for (size_t i = 0; i < shardHeld.size(); i++) if (shardHeld[i] == idx) { openShard((int)i); return true; }
    return false;
}

// ---------------------------------------------------------------------------
// C-10: the star charts
// ---------------------------------------------------------------------------

// a figure's name ("the hunter") as the computer reads it with that many shards of the open world held: the known words ours,
// the rest the people's
std::string Game::figureLabel(const std::string& name, int held) const {
    Shard tmp; tmp.text = name;
    std::vector<DecodedWord> w; decodeShard(tmp, shardWorld.lang, shardWorld.tongue, held, w);
    return upper(decodedText(w));
}

void Game::holdChart(bool up) {
    if (!up) { chartUp = false; status("THE CHART IS PUT DOWN", 3); audio.beep = 4; return; }
    if (!shardWorld.valid || !shardWorld.chart.valid || shardHeld.empty()) return;
    chartHeld = shardWorld.chart; chartHeldKey = shardWorld.key; chartHeldIndex = shardWorld.base + shardHeld[shardSel]; chartHeldPeople = shardWorld.lore.people;   // C-13: the world index
    chartHeldLabels.clear();
    for (const ChartFigure& f : chartHeld.figures) chartHeldLabels.push_back(figureLabel(f.name, (int)shardHeld.size()));
    chartUp = true;
    status("THE CHART IS HELD UP - ITS FIGURES SHOW ON THE VIEW", 5);   // 50 characters
    audio.beep = 4;
}

// the chart on the decoder: the hemisphere of the people's sky round the marked star as a disc (the stars as dots by their
// brightness, the figures' lines, their names at their brightest star, the mark's amber diamond), the caption beside it word by
// word as a text's, and the star marked named as the explorer knows it (or UNKNOWN) with its sector and its distance from
// here; on a first reading at this share the stars appear first, then the lines, then the names, over CHART_READ_S seconds
void Game::renderChart() {
    const std::string& key = shardWorld.key;
    int n = (int)shardHeld.size(), idx = shardHeld[shardSel], pct = (int)std::lround(100 * languageShare(n));
    const Shard& s = shardWorld.shards[idx]; const StarChart& C = shardWorld.chart;
    std::string name = upper(worldNameOfKey(key));
    std::string head = fmt("%s - SHARD %d OF %d", shardWorldTitle(30).c_str(), shardSel + 1, n);
    drawText(canvas, 6, 4, head.c_str(), HUD_AMBER);
    if (shardLent(idx)) drawText(canvas, 6 + textWidth(head.c_str()) + 2 * FONT_ADV, 4, "LENT", HUD_CYAN);   // C-14: a friend's, credited
    drawText(canvas, 6, 13, fmt("YEAR %d OF %s", s.year, upper(shardWorld.lore.city).c_str()).c_str(), HUD_DIM);
    drawText(canvas, 6, 22, fmt("A STAR CHART OF THE %s", upper(shardWorld.lore.people).c_str()).c_str(), HUD_DIM);
    bool reading = chartT >= 0;
    double u = reading ? clampd(chartT / CHART_READ_S, 0, 1) : 1;   // the stars by 0.4, the lines by 0.75, the names after
    const int cx = 82, cy = 108, R = 66;
    const uint32_t rim = rgb(30, 60, 45), lineCol = rgb(60, 130, 90), faint = rgb(70, 120, 90);
    {   // the rim
        int px = cx + R, py = cy;
        for (int k = 1; k <= 72; k++) { double a = k * TAU / 72; int x = cx + (int)std::lround(R * std::cos(a)), y = cy + (int)std::lround(R * std::sin(a)); drawLineRGB(canvas, px, py, x, y, rim); px = x; py = y; }
        drawLineRGB(canvas, cx - 3, cy, cx + 3, cy, rim); drawLineRGB(canvas, cx, cy - 3, cx, cy + 3, rim);
    }
    Rng flick((uint64_t)(realTime * 20));
    bool held = chartUp && chartHeldKey == key && chartHeldIndex == shardWorld.base + idx;
    if (!C.valid) drawTextCentered(canvas, cx, cy, "NO CHART", HUD_DIM);
    else {
        auto at = [&](int k, int& px, int& py) { px = cx + (int)std::lround(C.stars[k].x * R); py = cy - (int)std::lround(C.stars[k].y * R); };
        int total = (int)C.stars.size(), shown = u >= 0.4 ? total : (int)(total * u / 0.4);
        int lines = 0; for (const ChartFigure& f : C.figures) lines += (int)f.lines.size();
        int shownL = u >= 0.75 ? lines : (u > 0.4 ? (int)(lines * (u - 0.4) / 0.35) : 0), drawnL = 0;
        for (const ChartFigure& f : C.figures) for (const auto& ln : f.lines) {
            if (drawnL++ >= shownL) break;
            int x0, y0, x1, y1; at(ln.first, x0, y0); at(ln.second, x1, y1);
            drawLineRGB(canvas, x0, y0, x1, y1, lineCol);
        }
        for (int k = 0; k < shown; k++) {   // the stars (in their order of brightness, so a reading brightens the sky from its brightest down)
            int px, py; at(k, px, py);
            double b = C.stars[k].bright;
            if (b > 2) { fillRectRGB(canvas, px - 1, py - 1, px, py, HUD_WHITE); fillRectRGB(canvas, px - 2, py, px - 2, py, faint); fillRectRGB(canvas, px + 1, py, px + 1, py, faint); fillRectRGB(canvas, px, py - 2, px, py - 2, faint); fillRectRGB(canvas, px, py + 1, px, py + 1, faint); }
            else if (b > 0.6) fillRectRGB(canvas, px - 1, py - 1, px, py, HUD_WHITE);
            else fillRectRGB(canvas, px, py, px, py, HUD_GREEN);
        }
        if (u >= 0.4 && C.markStar >= 0) {   // the mark
            int px, py; at(C.markStar, px, py);
            drawLineRGB(canvas, px - 5, py, px, py - 5, HUD_AMBER); drawLineRGB(canvas, px, py - 5, px + 5, py, HUD_AMBER);
            drawLineRGB(canvas, px + 5, py, px, py + 5, HUD_AMBER); drawLineRGB(canvas, px, py + 5, px - 5, py, HUD_AMBER);
        }
        std::vector<std::array<int, 4>> placed;   // the labels' boxes, so a name moves down or up rather than over another
        auto labelY = [&](int lx, int ly, int tw) {
            static const int tries[5] = {0, 9, -9, 18, -18};
            for (int dy : tries) {
                int y0 = ly + dy; bool clear = y0 >= 30 && y0 <= 2 * cy - 30;
                for (const auto& b : placed) if (clear && lx < b[2] + 2 && lx + tw > b[0] - 2 && y0 < b[3] + 1 && y0 + 7 > b[1] - 1) clear = false;
                if (clear) { placed.push_back({lx, y0, lx + tw, y0 + 7}); return y0; }
            }
            placed.push_back({lx, ly, lx + tw, ly + 7}); return ly;
        };
        if (u >= 0.75) for (size_t i = 0; i < C.figures.size(); i++) {   // the names at the brightest star of each figure, flickering until the reading is done
            const ChartFigure& f = C.figures[i];
            std::string label = figureLabel(f.name, n);
            bool known = true; { Shard tmp; tmp.text = f.name; std::vector<DecodedWord> w; decodeShard(tmp, shardWorld.lang, shardWorld.tongue, n, w); for (const DecodedWord& d : w) if (!d.known) known = false; }
            if (reading && u < 1) for (char& c : label) if (c != ' ' && flick.chance(0.5)) c = (char)('A' + flick.irange(26));
            int px, py; at(f.label, px, py);
            int tw = textWidth(label.c_str()), lx = std::min(std::max(2, px + 4), 2 * cx - 2 - tw), ly = labelY(lx, std::min(std::max(30, py - 3), 2 * cy - 30), tw);
            drawText(canvas, lx, ly, label.c_str(), reading && u < 1 ? HUD_GREEN : (known ? HUD_WHITE : HUD_DIM));
        }
    }
    // the caption, laid out at the right as a text is (a first reading reveals it with the stars)
    int total = (int)shardWords.size(), cursor = reading ? std::min(total, (int)(total * u / 0.75)) : total;
    int x = 170, y = 33; const int x0 = 170, maxX = UW - 8;
    for (int i = 0; i < total; i++) {
        const DecodedWord& w = shardWords[i];
        std::string shown; uint32_t col;
        if (i < cursor) { shown = w.pre + (w.known ? w.ours : w.theirs) + w.post; col = w.name ? HUD_AMBER : (w.known ? HUD_WHITE : HUD_DIM); }
        else if (i == cursor) { shown = w.theirs; for (char& c : shown) c = (char)('A' + flick.irange(26)); col = HUD_GREEN; }
        else { shown = w.pre + w.theirs + w.post; col = HUD_DIM; }
        shown = upper(shown);
        int tw = textWidth(shown.c_str());
        if (x > x0 && x + tw > maxX) { x = x0; y += 9; }
        if (y > 118) break;
        drawText(canvas, x, y, shown.c_str(), col);
        x += tw + FONT_ADV;
    }
    if (C.valid && u >= 0.4) {   // the star they marked, as the explorer knows it
        const ChartMark& m = C.mark;
        Star full = m.star;
        double lyHere = length(m.star.pos - ship.pos) / SECTOR_KM;
        int yy = std::min(std::max(y + 16, 124), 136);   // the column is 23 characters wide: four short lines
        bool isRemote = ship.hasRemote && ship.remote.seed == m.star.seed;
        drawText(canvas, 170, yy, "THEY MARKED", HUD_AMBER);
        drawText(canvas, 170, yy + 9, fmt("%s (%s)", trunc(upper(starNameOf(full)), 16).c_str(), STAR_CLASSES[m.star.cls].code).c_str(), HUD_WHITE);
        drawText(canvas, 170, yy + 18, fmt("%.1f LY FROM HERE", lyHere).c_str(), HUD_WHITE);
        drawText(canvas, 170, yy + 27, fmt("SECTOR %lld %lld %lld", (long long)m.star.sx, (long long)m.star.sy, (long long)m.star.sz).c_str(), HUD_DIM);
        if (isRemote || held) drawText(canvas, 170, yy + 36, isRemote ? (held ? "THE REMOTE TARGET, HELD UP" : "THE REMOTE TARGET") : "HELD UP TO THE SKY", isRemote ? HUD_CYAN : HUD_GREEN);
    }
    int read = 0; for (const DecodedWord& w : shardWords) if (w.known) read++;
    if (reading) {
        drawText(canvas, 12, UH - 22, fmt("DECODING %3d%%", (int)(100 * u)).c_str(), HUD_AMBER);
        const int bx0 = 100, bx1 = UW - 12;
        drawRectRGB(canvas, bx0, UH - 22, bx1, UH - 16, HUD_DIM);
        if (u > 0) fillRectRGB(canvas, bx0 + 1, UH - 21, bx0 + 1 + (int)((bx1 - bx0 - 2) * u), UH - 17, HUD_GREEN);
        drawTextCentered(canvas, UW / 2, UH - 10, "ENTER SKIPS THE WAIT  ESC STOPS", HUD_DIM);
    } else {
        drawText(canvas, 12, UH - 22, fmt("%d OF %d WORDS READ   THE LANGUAGE %d%% (%d SHARD%s)", read, total, pct, n, n == 1 ? "" : "S").c_str(), HUD_GREEN);
        drawTextCentered(canvas, UW / 2, UH - 10, held ? "O PUT DOWN  ENTER TARGET THE STAR  LEFT/RIGHT  ESC" : "O HOLD UP  ENTER TARGET THE STAR  LEFT/RIGHT  ESC", HUD_DIM);   // 50 and 49 characters
    }
}

// the chart held up: its figures' lines between the real stars as the camera sees them (`cam` world to view, `obs` the eye in
// km), the figures' names at their brightest star, the marked star's diamond (an edge mark when it is behind), a line naming
// the chart; on the surface (`localFrame` given) only above the horizon
void Game::drawChartOverlay(const Mat3& cam, const Vec3& obs, const Proj& pj, const Mat3* localFrame) {
    chartLinesDrawn = 0;
    if (!chartUp || !chartHeld.valid) return;
    const StarChart& C = chartHeld;
    const double f = pj.f / FB_SCALE; const int cx = UW / 2, cy = UH / 2;
    size_t N = C.stars.size();
    std::vector<double> sx(N), sy(N); std::vector<char> ok(N, 0); std::vector<Vec3> vv(N);
    for (size_t k = 0; k < N; k++) {
        Vec3 d = C.stars[k].star.pos - obs;
        if (localFrame) { Vec3 l = (*localFrame) * d; if (l.y < 0.01 * length(l)) continue; }
        Vec3 v = cam * d; vv[k] = v;
        if (v.z <= 1e-9) continue;
        sx[k] = cx + f * v.x / v.z; sy[k] = cy - f * v.y / v.z;
        ok[k] = std::fabs(sx[k]) < 4000 && std::fabs(sy[k]) < 4000;
    }
    auto clip = [&](double& x0, double& y0, double& x1, double& y1) {   // Liang-Barsky against the screen with a margin
        const double L = -2, Rr = UW + 1, T = -2, B = UH + 1;
        double t0 = 0, t1 = 1, dx = x1 - x0, dy = y1 - y0;
        auto edge = [&](double p, double q) { if (p == 0) return q >= 0; double r = q / p; if (p < 0) { if (r > t1) return false; if (r > t0) t0 = r; } else { if (r < t0) return false; if (r < t1) t1 = r; } return true; };
        if (!edge(-dx, x0 - L) || !edge(dx, Rr - x0) || !edge(-dy, y0 - T) || !edge(dy, B - y0)) return false;
        double nx0 = x0 + t0 * dx, ny0 = y0 + t0 * dy, nx1 = x0 + t1 * dx, ny1 = y0 + t1 * dy;
        x0 = nx0; y0 = ny0; x1 = nx1; y1 = ny1;
        return true;
    };
    const uint32_t lineCol = rgb(50, 120, 80);
    const int S = FB_SCALE;
    std::vector<std::array<int, 4>> placed;   // the labels' boxes: a name moves down or up rather than over another (from afar a people's whole sky is a knot of a few degrees)
    auto labelY = [&](int lx, int ly, int tw) {
        static const int tries[5] = {0, 9, -9, 18, -18};
        for (int dy : tries) {
            int y0 = ly + dy; bool clear = y0 >= 30 && y0 <= UH - 30;
            for (const auto& b : placed) if (clear && lx < b[2] + 2 && lx + tw > b[0] - 2 && y0 < b[3] + 1 && y0 + 7 > b[1] - 1) clear = false;
            if (clear) { placed.push_back({lx, y0, lx + tw, y0 + 7}); return y0; }
        }
        placed.push_back({lx, ly, lx + tw, ly + 7}); return ly;
    };
    auto skyLine = [&](int x0, int y0, int x1, int y1) {   // a line on the sky alone: no pixel where something solid is drawn (the cabin's walls, a globe, the ground)
        int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0), steps = std::max(dx, dy);
        for (int i = 0; i <= steps; i++) {
            int x = steps ? x0 + (x1 - x0) * i / steps : x0, y = steps ? y0 + (y1 - y0) * i / steps : y0;
            if (x < 0 || y < 0 || x >= UW || y >= UH) continue;
            if (fb.invz[(size_t)(y * S) * FBW + (size_t)(x * S)] > 0) continue;
            fillRectRGB(canvas, x, y, x, y, lineCol);
        }
    };
    for (size_t i = 0; i < C.figures.size(); i++) {
        const ChartFigure& fg = C.figures[i];
        for (const auto& ln : fg.lines) {
            if (!ok[ln.first] || !ok[ln.second]) continue;
            double x0 = sx[ln.first], y0 = sy[ln.first], x1 = sx[ln.second], y1 = sy[ln.second];
            if (!clip(x0, y0, x1, y1)) continue;
            skyLine((int)std::lround(x0), (int)std::lround(y0), (int)std::lround(x1), (int)std::lround(y1));
            chartLinesDrawn++;
        }
        int lb = fg.label;
        if (ok[lb] && sx[lb] > 2 && sy[lb] > 30 && sx[lb] < UW - 40 && sy[lb] < UH - 30 && i < chartHeldLabels.size()) { int tw = textWidth(chartHeldLabels[i].c_str()), lx = (int)sx[lb] + 4; drawText(canvas, lx, labelY(lx, (int)sy[lb] - 3, tw), chartHeldLabels[i].c_str(), HUD_DIM); }
    }
    if (C.markStar >= 0) {
        int k = C.markStar;
        if (ok[k] && sx[k] > 6 && sy[k] > 6 && sx[k] < UW - 6 && sy[k] < UH - 6) {
            int px = (int)std::lround(sx[k]), py = (int)std::lround(sy[k]);
            drawLineRGB(canvas, px - 5, py, px, py - 5, HUD_AMBER); drawLineRGB(canvas, px, py - 5, px + 5, py, HUD_AMBER);
            drawLineRGB(canvas, px + 5, py, px, py + 5, HUD_AMBER); drawLineRGB(canvas, px, py + 5, px - 5, py, HUD_AMBER);
            if (py < UH - 30) drawTextCentered(canvas, px, py + 8, "THEY MARKED THIS STAR", HUD_AMBER);
        } else if (!localFrame || (*localFrame * (C.stars[k].star.pos - obs)).y > 0) {
            const Vec3& v = vv[k];
            double ang = std::atan2(-v.y, v.x);
            drawText(canvas, UW / 2 + (int)(std::cos(ang) * 140) - 2, UH / 2 + (int)(std::sin(ang) * 85) - 3, "+", HUD_AMBER);
        }
    }
    std::string line = fmt("A CHART OF THE %s HELD UP", upper(chartHeldPeople).c_str());
    drawTextShadow(canvas, UW - 8 - textWidth(line.c_str()), 32, line.c_str(), HUD_DIM, HUD_SHADOW);
}

std::string Game::testChartInfo() const {
    std::string s = fmt("chart: level %d", shardsLevel);
    if (shardsLevel == 2 && shardWorld.valid && !shardHeld.empty() && shardWorld.shards[shardHeld[shardSel]].chart) {
        const StarChart& C = shardWorld.chart;
        s += fmt(", on the screen S%d (chart %d of the %s): %zu stars, %zu figures, the mark %s at %lld %lld %lld (how %d, %.0f ly, %s), %s", shardHeld[shardSel], C.which, shardWorld.lore.people.c_str(), C.stars.size(), C.figures.size(),
                 STAR_CLASSES[C.mark.star.cls].code, (long long)C.mark.star.sx, (long long)C.mark.star.sy, (long long)C.mark.star.sz, C.mark.how, C.mark.ly, C.mark.people.empty() ? "no people" : ("the " + C.mark.people).c_str(),
                 chartT >= 0 ? fmt("reading %.0f%%", 100 * clampd(chartT / CHART_READ_S, 0, 1)).c_str() : (shardFresh ? "unread" : "read"));
    }
    s += chartUp ? fmt("; held up: S%d of %s (%zu stars, %zu figures), %d lines drawn", chartHeldIndex, chartHeldKey.c_str(), chartHeld.stars.size(), chartHeld.figures.size(), chartLinesDrawn) : "; none held up";
    return s;
}

bool Game::testAimAtChartMark(int figure) {
    if (!chartUp || !chartHeld.valid) return false;
    if (figure >= (int)chartHeld.figures.size()) return false;
    const Star& at = figure < 0 ? chartHeld.mark.star : chartHeld.stars[chartHeld.figures[figure].label].star;
    Vec3 fwd = normalize(at.pos - ship.pos);
    ship.yaw = std::atan2(fwd.x, fwd.z); ship.pitch = std::asin(clampd(fwd.y, -1, 1));
    cabin.yaw = 0; cabin.pitch = 0;
    return true;
}
