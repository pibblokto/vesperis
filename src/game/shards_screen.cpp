// C-06: the shard decoder on the ship. The shards the explorer took (`Guide::shards`) are read here, at the decoder on the
// cabin's back wall (or from the flight computer and the guide menu): the worlds they come from, a world's shards by their
// years, and a shard's text as the ship's computer can read it with the language learnt from that world's shards
// (`peoples/shards.h`: `languageShare`, `decodeShard`). C-05: a text shard is a recording that speaks (`peoples/voice.h`,
// `AudioState::speech`): the people's words in the people's voice, and the word being spoken is marked under the text. A
// shard the computer has not read at the language's current share resolves its words as they are spoken, the end of the
// recording reads it (Enter skips to the end, Esc leaves it unread); the guide keeps the count it was read with (`decoded`),
// so a shard re-read after more of its world were found decodes again with fewer gaps. A word not learnt yet shows dim, in
// the people's own tongue; a name amber; a word read white. C-04: a shard that is a piece of music (`peoples/music.h`) plays
// here through the synth (`AudioState::piece`) with its notes drawn as a roll the playhead crosses, the tradition's
// scale, cycle and timbres named above it; N names the piece (the guide's `names`, keyed like the shard); a piece heard
// is recorded as read at the world's full count, so it never waits again. The ship's hum is ducked under either. C-10: a
// shard that is a star chart (`peoples/charts.h`) draws itself here beside its caption: the hemisphere of the people's sky
// round the star they marked, their figures and their names (words of the language, resolving as it is learnt); a first
// reading draws it over a few seconds; Enter targets the marked star and O holds the chart up to the sky (`drawChartOverlay`).
// a world's records on three pages (Tab): the records by their years with their kinds (Left/Right shows one kind), the timeline of
// the people's eras with the records held on it, and the history as the records read so far tell it (game/history_screen.cpp).
#include "game.h"
#include "ui.h"
#include "peoples/records.h"   // the media
#include "core/rng.h"
#include <cstdlib>
#include <algorithm>
#include <array>
#include <cmath>

namespace {
const int WORLD_ROWS = 8;    // worlds per page (two lines each)
const int SHARD_ROWS = 15;   // a world's shards per page (H-06: under the pages' strip)
int bodyOfKey(const std::string& key) { size_t sl = key.find('/'); return sl == std::string::npos ? -1 : atoi(key.c_str() + sl + 1); }
}

// C-13: of that people's fifty (C-14: the explorer's own and the lent)
int Game::shardsHeldOf(const std::string& key, int which) const {   // of that people's fifty; C-14: the explorer's own and the lent
    std::string prefix = key + "/S"; int n = 0;
    for (const std::string& k : guide.shards) if (k.compare(0, prefix.size(), prefix) == 0 && shardPeopleOf(atoi(k.c_str() + prefix.size())) == which) n++;
    return n + lentOf(key, which);
}
// C-14: how many of that people's fifty are lent and not taken
int Game::lentOf(const std::string& key, int which) const {
    std::string prefix = key + "/S"; int n = 0;
    for (const std::string& k : guide.lent) if (k.compare(0, prefix.size(), prefix) == 0 && shardPeopleOf(atoi(k.c_str() + prefix.size())) == which && !guide.shards.count(k)) n++;
    return n;
}
// C-14: the open people's shard is a friend's (lent by the inbox, not taken since)
bool Game::shardLent(int local) const { return guide.isLent(heldShardKey(local)); }
// C-14: a piece's name, the explorer's own or the inbox's (`foreign`); null when none
const std::string* Game::shardNameOf(const std::string& key, bool& foreign) const {
    foreign = false;
    auto it = guide.names.find(key);
    if (it != guide.names.end()) return &it->second;
    it = guide.inbox.find(key);
    if (it != guide.inbox.end()) { foreign = true; return &it->second; }
    return nullptr;
}
// C-13: the guide's key of the open people's shard (its world index)
std::string Game::heldShardKey(int local) const { return shardWorld.key + "/S" + std::to_string(shardWorld.base + local); }
// C-13: the open people's record in `Guide::ended` (the second people's under "/P1")
std::string Game::endedKey() const { return shardWorld.key + (shardWorld.which ? "/P1" : ""); }
// C-13: the world's name, and the people's where the world had two
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

// C-06: a shard the decoder has not read at its world's current share (the decoder's screen glows)
// a shard the guide holds that the computer has not read at its world's current share
bool Game::shardsPending() const {
    std::map<std::string, int> held;   // per world and people; C-14: the lent count with the own
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

// C-07: a world's shards, language, voice and tradition, for the decoder and the radar alike (H-04: `wait` false: not until its records are
// written, on a thread)
// the world's system from its key, its lore, its fifty and its language; kept while the same world is read
// a world's shards, language, voice and tradition built for the decoder or the radar (the key is the caller's)
bool Game::buildShardWorld(ShardWorld& w, const Star& s, int bi, int which, bool wait) {
    std::string key = w.key;
    w = ShardWorld(); w.key = key; w.which = which;
    if (!s.valid) return false;
    w.sys.generate(s);
    if (bi < 0 || bi >= (int)w.sys.bodies.size()) return false;
    const Body& b = w.sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    if (!wait) {   // a world's records need its history, and its history its plan: written on a thread the first time
        Lore q = loreQuick(w.sys, b, g, which);
        if (!shardTextsIfReady(w.sys, b, g, q)) { w.pending = true; return false; }
    }
    w.body = bi;
    w.lore = loreOf(w.sys, b, g, which);
    w.which = w.lore.which; w.peoples = w.lore.peoples; w.base = shardWorldIndex(w.which, 0);   // the people's fifty, its guide indices from `base`
    w.tongue = tongueOf(g, w.lore);
    shardsOf(w.sys, b, g, w.lore, w.lore.records, w.shards);   // the people's records (C-02's fifty the first)
    w.texts = shardTextsOf(w.sys, b, g, w.lore);                // what each told (the cache's: `shardsOf` read it)
    languageOf(w.lore, w.shards, w.tongue, w.lang);
    w.voice = voiceOf(g, w.lore, w.tongue);   // its voice
    w.readVoice = voiceRead(w.voice);          // and the ship's reading of what has no voice of its own
    w.tradition = traditionOf(g, w.lore);   // its music and the form and length of each piece, for the list
    w.pieceForm.assign(w.shards.size(), -1); w.pieceSeconds.assign(w.shards.size(), 0.0);
    for (size_t i = 0; i < w.shards.size(); i++) if (w.shards[i].music) { Piece pc; pieceOf(w.tradition, w.shards[i], pc); w.pieceForm[i] = pc.form; w.pieceSeconds[i] = pc.seconds; }
    w.valid = true;
    return true;
}

// H-04: the decoder's worlds list asks for the selected world's records early
void Game::prefetchShardWorld(int sel) {
    if (sel < 0 || sel >= (int)shardWorldKeys.size()) return;
    const std::string& key = shardWorldKeys[sel];
    int64_t sx, sy, sz; if (!Guide::parseStarKey(key, sx, sy, sz)) return;
    Star st; if (!placeInSector(sx, sy, sz, st, true)) return;
    StarSystem wsys; wsys.generate(st);
    const int bi = bodyOfKey(key);
    if (bi < 0 || bi >= (int)wsys.bodies.size()) return;
    const BodyGen g = BodyGen::make(wsys.bodies[bi]);
    shardTextsPrefetch(wsys, wsys.bodies[bi], g, loreQuick(wsys, wsys.bodies[bi], g, shardWorldWhich[sel]));
}

bool Game::openShardWorld(const std::string& key, int which) {
    if (shardWorld.valid && shardWorld.key == key && shardWorld.which == which) return true;
    stopPiece(); stopSpeech();
    shardWorld = ShardWorld(); shardWorld.key = key;
    toldValid = false; told = ToldHistory(); legendRows.clear(); legendSel = 0; legendTop = 0; shardsFilter = 0;   // H-06: the pages read the new world
    int64_t sx, sy, sz; if (!Guide::parseStarKey(key, sx, sy, sz)) return false;
    int bi = bodyOfKey(key);
    Star s; if (!placeInSector(sx, sy, sz, s, true)) return false;
    return buildShardWorld(shardWorld, s, bi, which);
}

// C-04: the piece of the open world onto the synth, from the top; off it (the pointers cleared before anything they point at can move)
void Game::startPiece() {
    audio.piece = &shardWorld.piece; audio.tradition = &shardWorld.tradition; audio.pieceSpeed = 1;
    audio.medium = shardWorld.valid && !shardHeld.empty() ? shardWorld.shards[shardHeld[shardSel]].medium : -1;   // heard through what it was made on
    audio.pieceStart = true; audio.piecePause = false; audio.pieceDone = false; audio.pieceBeat = -1;
    pieceT = 0; pieceSyncBeat = -1; piecePaused = false;
    if (humBeforePiece < 0) humBeforePiece = audio.hum;   // the ship's hum ducked under the music
    audio.hum = std::min(audio.hum, 0.2);
}
void Game::stopPiece() {
    if (audio.piece == &shardWorld.piece) {   // C-11: the decoder's own (the ship's player and the gramophone keep theirs)
        audio.piece = nullptr; audio.tradition = nullptr;
        if (!audio.speech) audio.medium = -1;
        audio.pieceStart = false; audio.piecePause = false; audio.pieceBeat = -1;
    }
    pieceT = -1; pieceSyncBeat = -1; piecePaused = false;
    if (humBeforePiece >= 0 && audio.speech != &shardWorld.speech) { audio.hum = humBeforePiece; humBeforePiece = -1; }
}
double Game::pieceBeatNow() const { return pieceT >= 0 ? pieceT * shardWorld.piece.bpm / 60.0 : 0; }
bool Game::pieceEnded() const { return pieceT >= 0 && (audio.pieceDone || pieceBeatNow() >= shardWorld.piece.beats); }

// C-05: the shard on the screen spoken from the top; off (the pointers cleared before anything they point at can move)
void Game::startSpeech() {
    const int medium = shardWorld.valid && !shardHeld.empty() ? shardWorld.shards[shardHeld[shardSel]].medium : -1;
    audio.speech = &shardWorld.speech; audio.voice = medium < 0 || recordMediumSounds(medium) ? &shardWorld.voice : &shardWorld.readVoice;   // a recording's own voice, else the ship's reading
    audio.medium = medium;
    audio.speechStart = true; audio.speechPause = false; audio.speechDone = false; audio.speechT = -1;
    speechT = 0; speechSyncT = -1; speechPaused = false;
    if (humBeforePiece < 0) humBeforePiece = audio.hum;   // the ship's hum ducked under the voice
    audio.hum = std::min(audio.hum, 0.2);
}
void Game::stopSpeech() {
    if (audio.speech == &shardWorld.speech) {   // C-11: the decoder's own
        audio.speech = nullptr; audio.voice = nullptr;
        if (!audio.piece) audio.medium = -1;
        audio.speechStart = false; audio.speechPause = false; audio.speechT = -1;
    }
    speechT = -1; speechSyncT = -1; speechPaused = false;
    if (humBeforePiece >= 0 && audio.piece != &shardWorld.piece) { audio.hum = humBeforePiece; humBeforePiece = -1; }
}
bool Game::speechEnded() const { return speechT >= 0 && (audio.speechDone || speechT >= shardWorld.speech.seconds); }
// a first reading done: the shard is read at the world's current count
void Game::markShardRead() {
    if (!shardFresh || !shardWorld.valid || shardHeld.empty()) return;
    shardFresh = false;
    int idx = shardHeld[shardSel];
    guide.decoded[heldShardKey(idx)] = (int)shardHeld.size();
    toldValid = false;   // H-06: what the records told grows
    if (shardWorld.shards[idx].last && guide.ended.insert(endedKey()).second) {   // the world's last recording read: the log notes it and the timeline closes (`logEvent` saves the guide)
        logEvent("SHARD", fmt("THE LAST RECORDING OF THE %s READ: THE RECORD OF %s ENDS IN YEAR %d OF %s", upper(shardWorld.lore.people).c_str(), upper(worldNameOfKey(shardWorld.key)).c_str(), shardWorld.shards[idx].year, upper(shardWorld.lore.city).c_str()));
    } else if (shardWorld.shards[idx].chart && shardWorld.chart.valid) {   // a chart read goes to the log with the star it marks (the system only: what is there is theirs to tell)
        const ChartMark& m = shardWorld.chart.mark;
        logEvent("CHART", fmt("A STAR CHART OF THE %s READ: THEY MARKED THE STAR AT SECTOR %lld %lld %lld (%s), %.0f LY FROM %s", upper(shardWorld.lore.people).c_str(), (long long)m.star.sx, (long long)m.star.sy, (long long)m.star.sz, STAR_CLASSES[m.star.cls].code, m.ly, upper(worldNameOfKey(shardWorld.key)).c_str()));
    } else guide.save(guidePath);
    audio.beep = BEEP_LOCK;
}

// C-18: `shardWorldKeys` and the rest from the guide, the cursor kept on its world
// an entry a people whose shards are held; a world of two peoples names each (its system generated once here, so a world of two is
// named as such from the first shard of either). C-18: listed again when the guide holds more while the decoder is open (the drones dock),
// the cursor kept on its entry
void Game::listShardWorlds() {
    const bool had = shardWorldSel >= 0 && shardWorldSel < (int)shardWorldKeys.size() && shardWorldSel < (int)shardWorldWhich.size();
    const std::string selKey = had ? shardWorldKeys[shardWorldSel] : std::string();
    const int selWhich = had ? shardWorldWhich[shardWorldSel] : 0;
    std::map<std::string, std::set<int>> held;
    for (const std::string& k : guide.shards) { size_t p = k.rfind("/S"); if (p != std::string::npos) held[k.substr(0, p)].insert(shardPeopleOf(atoi(k.c_str() + p + 2))); }
    for (const std::string& k : guide.lent) { size_t p = k.rfind("/S"); if (p != std::string::npos) held[k.substr(0, p)].insert(shardPeopleOf(atoi(k.c_str() + p + 2))); }   // a friend's shards list their world too
    shardWorldKeys.clear(); shardWorldWhich.clear(); shardWorldLabels.clear(); shardWorldTotals.clear();
    for (const auto& kv : held) {
        int peoples = 1; std::string names[2]; int totals[2] = {SHARDS_PER_WORLD, SHARDS_PER_WORLD};
        int64_t sx, sy, sz; Star s;
        if (Guide::parseStarKey(kv.first, sx, sy, sz) && placeInSector(sx, sy, sz, s, true)) {
            StarSystem wsys; wsys.generate(s); int bi = bodyOfKey(kv.first);
            if (bi >= 0 && bi < (int)wsys.bodies.size()) {
                BodyGen g = BodyGen::make(wsys.bodies[bi]);
                if (peoplesOf(g) > 1) { peoples = 2; for (int k = 0; k < 2; k++) names[k] = peopleNameOf(peopleGen(g, k)); }
                for (int k = 0; k < peoples; k++) totals[k] = recordCountOf(g, k);   // each people's records
            }
        }
        size_t before = shardWorldKeys.size();
        for (int k = 0; k < peoples; k++) {
            if (!kv.second.count(k)) continue;
            shardWorldKeys.push_back(kv.first); shardWorldWhich.push_back(k); shardWorldLabels.push_back(peoples > 1 ? "THE " + upper(names[k]) : std::string()); shardWorldTotals.push_back(totals[k]);
        }
        if (shardWorldKeys.size() == before) { shardWorldKeys.push_back(kv.first); shardWorldWhich.push_back(0); shardWorldLabels.push_back(std::string()); shardWorldTotals.push_back(totals[0]); }   // a key the world cannot place: under the first people
    }
    shardsListed = guide.shards.size() + guide.lent.size();
    for (size_t i = 0; i < shardWorldKeys.size() && had; i++) if (shardWorldKeys[i] == selKey && shardWorldWhich[i] == selWhich) { shardWorldSel = (int)i; break; }
    if (shardWorldSel >= (int)shardWorldKeys.size()) shardWorldSel = 0;
}

// C-18: `shardHeld` of the open world, the cursor kept on its record
// the open world's shards held (C-13: its people's; C-14: the explorer's own and the lent, each once), in the order of their years. C-18:
// listed again as the drones bring more, the cursor kept on its record
void Game::listShardsHeld() {
    const int was = shardSel >= 0 && shardSel < (int)shardHeld.size() ? shardHeld[shardSel] : -1;
    shardHeld.clear();
    const std::string prefix = shardWorld.key + "/S";
    std::set<int> locals;
    for (const std::set<std::string>* src : {&guide.shards, &guide.lent})
        for (const std::string& k : *src) if (k.compare(0, prefix.size(), prefix) == 0) { int i = atoi(k.c_str() + prefix.size()); if (shardPeopleOf(i) != shardWorld.which) continue; i = shardLocalOf(i); if (i >= 0 && i < (int)shardWorld.shards.size()) locals.insert(i); }
    shardHeld.assign(locals.begin(), locals.end());
    std::sort(shardHeld.begin(), shardHeld.end(), [&](int a, int b) { int ya = shardWorld.shards[a].year, yb = shardWorld.shards[b].year; return ya != yb ? ya < yb : a < b; });
    shardSel = 0;
    for (size_t i = 0; i < shardHeld.size(); i++) if (shardHeld[i] == was) { shardSel = (int)i; break; }
    toldValid = false;   // what the records told grows with them
}

void Game::openShards() {
    listShardWorlds();
    radarOff();   // C-07: the decoder takes the synth
    shardsLevel = 0; shardFresh = false; stopPiece(); stopSpeech();
    prefetchShardWorld(shardWorldSel);   // its records written while the list is read
    // the world under the ship first, when its shards are here
    if (sys.valid && ship.parkedBody >= 0) {
        std::string here = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, ship.parkedBody);
        for (size_t i = 0; i < shardWorldKeys.size(); i++) if (shardWorldKeys[i] == here) { shardWorldSel = (int)i; break; }
    }
    if (shardsTop == 1) playerBuildList();   // the player's page, as it was left
    guideReturn = GameState::SPACE; returnState = GameState::SPACE;
    state = GameState::SHARDS;
    audio.beep = BEEP_UI;
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
    if (shardWorld.shards[idx].music) {   // a piece plays from the top; heard once, it is read for good
        shardWords.clear();
        pieceOf(shardWorld.tradition, shardWorld.shards[idx], shardWorld.piece);
        startPiece();
        if (!guide.decoded.count(skey)) { guide.decoded[skey] = shardWorld.lore.records; guide.save(guidePath); toldValid = false; }   // read at the people's whole count
        shardsLevel = 2; audio.beep = BEEP_UI;
        return;
    }
    decodeShard(shardWorld.shards[idx], shardWorld.lang, shardWorld.tongue, n, shardWords);
    auto d = guide.decoded.find(skey);
    shardFresh = d == guide.decoded.end() || d->second < n;
    chartT = -1;
    if (shardWorld.shards[idx].chart) {   // a chart draws itself (no voice: a map, not a sound); a first reading at this share takes CHART_READ_S seconds
        const Body& b = shardWorld.sys.bodies[shardWorld.body];
        chartOf(shardWorld.sys, b, BodyGen::make(b), shardWorld.lore, idx, shardWorld.chart);
        if (shardFresh) { chartT = 0; audio.beep = BEEP_UI; }
        shardsLevel = 2;
        return;
    }
    layoutShardText(12, 33, UW - 12, UH - 36);   // a long text in pages; the voice speaks the opening (C-05: the recording speaks)
    speakShardPage(0);
    if (shardFresh) audio.beep = BEEP_UI;
    shardsLevel = 2;
}

// H-04: a long text in pages (the words' places on the screen and where each page starts); the voice speaks the page on the
// screen (the opening first), from `speechFrom` (the first of its words) to `speechTo`
// the shard on the screen in pages: the words laid out at the text's width as they read at this share (a word read as ours, one
// not learnt in their tongue), a paragraph on a new line a little lower, a page where the text area is full; a page begins at a
// paragraph or a sentence where one is near (`shardPageStart`); the screen flows a page's words as they are shown in the frame
void Game::layoutShardText(int x0, int y0, int maxX, int maxY) {
    shardLayout.assign(shardWords.size(), WordAt()); shardPageStart.assign(1, 0); shardPage = 0;
    int x = x0, y = y0, page = 0, lastStart = 0;   // lastStart: the latest word a page could begin at (a paragraph's or a sentence's first)
    for (size_t i = 0; i < shardWords.size(); i++) {
        const DecodedWord& w = shardWords[i];
        const int tw = textWidth(upper(w.pre + (w.known ? w.ours : w.theirs) + w.post).c_str());
        if (i > 0 && (w.par || shardWords[i - 1].post.find_first_of(".?!") != std::string::npos)) lastStart = (int)i;
        if (w.par && i > 0) { x = x0; y += 12; }
        else if (x > x0 && x + tw > maxX) { x = x0; y += 9; }
        if (y > maxY && i > 0) {   // a new page, from the paragraph or the sentence this line is part of when it began on this page
            int from = lastStart > shardPageStart.back() && (int)i - lastStart < 40 ? lastStart : (int)i;
            page++; shardPageStart.push_back(from);
            x = x0; y = y0;
            for (int k = from; k < (int)i; k++) {   // the words moved over, laid out again
                const DecodedWord& v = shardWords[k];
                const int vw = textWidth(upper(v.pre + (v.known ? v.ours : v.theirs) + v.post).c_str());
                if (k > from && v.par) { x = x0; y += 12; } else if (x > x0 && x + vw > maxX) { x = x0; y += 9; }
                shardLayout[k] = {page, x, y, vw}; x += vw + FONT_ADV;
            }
            if (w.par && from < (int)i) { x = x0; y += 12; } else if (x > x0 && x + tw > maxX) { x = x0; y += 9; }
        }
        shardLayout[i] = {page, x, y, tw};
        x += tw + FONT_ADV;
    }
}
// the voice speaks a page of the text (the opening on a first reading; any page again on Enter)
void Game::speakShardPage(int page) {
    const int pages = (int)shardPageStart.size();
    page = std::max(0, std::min(pages - 1, page));
    speechFrom = shardPageStart.empty() ? 0 : shardPageStart[page];
    speechTo = page + 1 < pages ? shardPageStart[page + 1] : (int)shardWords.size();
    std::vector<DecodedWord> said(shardWords.begin() + speechFrom, shardWords.begin() + speechTo);
    if (!said.empty()) said[0].par = false;
    const Shard& sh = shardWorld.shards[shardHeld[shardSel]];   // H-05: a recording speaks in its own voice; a tablet or a page is read by the ship
    speechOf(sh.medium < 0 || recordMediumSounds(sh.medium) ? shardWorld.voice : shardWorld.readVoice, said, sh.seed + (uint64_t)page * 0x9E37ULL, shardWorld.speech);
    shardPage = page;
    startSpeech();
}

void Game::updateShards(const Input& in, double realDt) {
    if (shardsLevel < 2 && guide.shards.size() + guide.lent.size() != shardsListed) {   // C-18: the drones brought more while the decoder was open (a record on the screen keeps the count it is read with)
        listShardWorlds();
        if (shardsLevel == 1 && shardWorld.valid) listShardsHeld();
        if (shardsTop == 1) {   // C-11's player page: its list again, the cursor on its row
            const std::string selKey = player.sel >= 0 && player.sel < (int)player.items.size() ? playerItemKey(player.items[player.sel]) : std::string();
            playerBuildList();
            for (size_t i = 0; i < player.items.size() && !selKey.empty(); i++) if (playerItemKey(player.items[i]) == selKey) { player.sel = (int)i; break; }
        }
    }
    int nw = (int)shardWorldKeys.size();
    if (shardsLevel == 0 && in.wasPressed(KEY_TAB)) { shardsTop = 1 - shardsTop; if (shardsTop == 1) playerBuildList(); audio.beep = BEEP_UI; return; }   // the worlds, the player
    if (shardsLevel == 0 && shardsTop == 1) {   // the ship's player
        const int n = (int)player.items.size();
        if (in.wasPressed(KEY_ESCAPE)) { state = GameState::SPACE; return; }   // it plays on
        if (n == 0) return;
        menuStep(in, player.sel, n);
        if (in.wasPressed(KEY_PAGE_UP)) player.sel = std::max(0, player.sel - 15);
        if (in.wasPressed(KEY_PAGE_DOWN)) player.sel = std::min(n - 1, player.sel + 15);
        if (enterKey(in)) { playerPlay(player.sel); audio.beep = BEEP_UI; }
        if (in.wasPressed(KEY_SPACE) && player.playing >= 0) {
            player.paused = !player.paused;
            if (audio.piece == &player.world.piece) audio.piecePause = player.paused;
            if (audio.speech == &player.world.speech) audio.speechPause = player.paused;
        }
        if (in.wasPressed(KEY_BACKSPACE) || in.wasPressed(KEY_DELETE)) { playerStop(); audio.beep = BEEP_UI; }
        return;
    }
    if (shardsLevel == 0) {
        if (in.wasPressed(KEY_ESCAPE) || (nw == 0 && enterKey(in))) { stopPiece(); state = GameState::SPACE; return; }
        if (nw == 0) return;
        if (in.wasPressed(KEY_UP)) { shardWorldSel = (shardWorldSel + nw - 1) % nw; prefetchShardWorld(shardWorldSel); }
        if (in.wasPressed(KEY_DOWN)) { shardWorldSel = (shardWorldSel + 1) % nw; prefetchShardWorld(shardWorldSel); }
        if (!enterKey(in)) return;
        if (!openShardWorld(shardWorldKeys[shardWorldSel], shardWorldWhich[shardWorldSel])) { refuse("THE DECODER CANNOT FIND THAT WORLD", 3); return; }
        shardSel = -1; listShardsHeld();   // from its first
        shardsLevel = 1; audio.beep = BEEP_UI;
        if (!shardHeld.empty() && !shardInFilter(shardHeld[0])) stepShard(1);
    } else if (shardsLevel == 1) {   // the world's three pages
        int n = (int)shardHeld.size();
        if (in.wasPressed(KEY_ESCAPE)) { shardsLevel = 0; return; }
        if (in.wasPressed(KEY_TAB)) { shardsPage = (shardsPage + (in.shift() ? 2 : 1)) % 3; audio.beep = BEEP_UI; return; }
        if (n == 0) return;
        if (shardsPage == 0) {   // the records: Up/Down through the kind shown, Left/Right the kind (R-417: with Shift held Up/Down run, stopping at the ends)
            if (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_RIGHT)) {
                shardsFilter = (shardsFilter + (in.wasPressed(KEY_RIGHT) ? 1 : 3)) % 4; audio.beep = BEEP_UI;
                if (!shardInFilter(shardHeld[shardSel])) stepShard(1);
                return;
            }
            if (in.shift()) { const int s = shardRunSteps(in, realDt, false); if (s) stepShard(s, true); }
            else {
                if (in.wasPressed(KEY_UP)) stepShard(-1);
                if (in.wasPressed(KEY_DOWN)) stepShard(1);
            }
            if (in.wasPressed(KEY_PAGE_UP)) stepShard(-SHARD_ROWS);
            if (in.wasPressed(KEY_PAGE_DOWN)) stepShard(SHARD_ROWS);
            if (enterKey(in) && shardInFilter(shardHeld[shardSel])) openShard(shardSel);
        } else if (shardsPage == 1) {   // the timeline: along the years (R-416: with Shift held the arrows run, stopping at the ends)
            if (in.shift()) shardSel = std::max(0, std::min(n - 1, shardSel + shardRunSteps(in, realDt, true)));
            else {
                menuStep(in, shardSel, n, true);
            }
            if (in.wasPressed(KEY_PAGE_UP)) shardSel = std::max(0, shardSel - 10);
            if (in.wasPressed(KEY_PAGE_DOWN)) shardSel = std::min(n - 1, shardSel + 10);
            if (enterKey(in)) openShard(shardSel);
        } else {   // the history: Up/Down through the events told, Enter opens the record that told the one under the cursor
            ensureTold();
            const int ne = (int)told.events.size();
            if (ne == 0) return;
            if (in.shift()) legendSel = std::max(0, std::min(ne - 1, legendSel + shardRunSteps(in, realDt, false)));
            else {
                if (in.wasPressed(KEY_UP)) legendSel = std::max(0, legendSel - 1);
                if (in.wasPressed(KEY_DOWN)) legendSel = std::min(ne - 1, legendSel + 1);
            }
            if (in.wasPressed(KEY_PAGE_UP)) legendSel = std::max(0, legendSel - 8);
            if (in.wasPressed(KEY_PAGE_DOWN)) legendSel = std::min(ne - 1, legendSel + 8);
            if (enterKey(in)) { const int r = toldRecordOf(legendSel); if (r >= 0) openShard(r); }
        }
    } else if (shardWorld.valid && !shardHeld.empty() && shardWorld.shards[shardHeld[shardSel]].music) {   // a piece playing
        if (audio.pieceBeat >= 0 && audio.pieceBeat != pieceSyncBeat) { pieceSyncBeat = audio.pieceBeat; pieceT = audio.pieceBeat * 60.0 / shardWorld.piece.bpm; }   // the synth's clock when it runs
        else if (pieceT >= 0 && !piecePaused && !pieceEnded()) pieceT += realDt;
        if (in.wasPressed(KEY_ESCAPE)) { stopPiece(); shardsLevel = 1; return; }
        if (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_UP)) { stepShard(-1); openShard(shardSel); return; }   // the kind the records' page shows
        if (in.wasPressed(KEY_RIGHT) || in.wasPressed(KEY_DOWN)) { stepShard(1); openShard(shardSel); return; }
        if (in.wasPressed(KEY_SPACE) && !pieceEnded()) { piecePaused = !piecePaused; audio.piecePause = piecePaused; }
        if (enterKey(in) && pieceEnded()) startPiece();
        if (in.wasPressed(KEY_N) && !in.ctrl()) {
            textShardKey = heldShardKey(shardHeld[shardSel]);
            auto nm = guide.names.find(textShardKey);
            guideReturn = GameState::SHARDS;
            beginTextEntry("NAME THE PIECE", TEXT_PIECE, nm != guide.names.end() ? upper(nm->second) : std::string());
        }
    } else if (shardWorld.valid && !shardHeld.empty() && shardWorld.shards[shardHeld[shardSel]].chart) {   // a chart on the screen
        int idx = shardHeld[shardSel];
        if (chartT >= 0) { chartT += realDt; if (chartT >= CHART_READ_S) { markShardRead(); chartT = -1; } }
        bool reading = chartT >= 0;
        if (in.wasPressed(KEY_ESCAPE)) { chartT = -1; shardsLevel = 1; return; }   // a first reading left early is not read: it waits again next time
        if (enterKey(in)) {
            if (reading) { chartT = -1; markShardRead(); }   // Enter skips the wait
            else if (shardWorld.chart.valid) { Star full; if (placeInSector(shardWorld.chart.mark.star.sx, shardWorld.chart.mark.star.sy, shardWorld.chart.mark.star.sz, full, true)) setRemoteStar(full); }   // or targets the marked star
            return;
        }
        if (in.wasPressed(KEY_O) && !reading && shardWorld.chart.valid) { holdChart(!(chartUp && chartHeldKey == shardWorld.key && chartHeldIndex == shardWorld.base + idx)); return; }
        if (in.wasPressed(KEY_LEFT) || in.wasPressed(KEY_UP)) { stepShard(-1); openShard(shardSel); }
        else if (in.wasPressed(KEY_RIGHT) || in.wasPressed(KEY_DOWN)) { stepShard(1); openShard(shardSel); }
    } else {   // a text speaking; on a first reading at this share the words resolve as they are spoken and the end reads the shard
        // the opening is spoken (the first page); the rest on request: UP/DOWN turn a long text's pages, Enter speaks the page shown
        if (audio.speechT >= 0 && audio.speechT != speechSyncT) { speechSyncT = audio.speechT; speechT = audio.speechT; }   // the synth's clock when it runs
        else if (speechT >= 0 && !speechPaused && !speechEnded()) speechT += realDt;
        if (shardFresh && speechEnded()) markShardRead();
        bool speaking = speechT >= 0 && !speechEnded();
        const int pages = (int)shardPageStart.size();
        if (in.wasPressed(KEY_ESCAPE)) { stopSpeech(); shardsLevel = 1; return; }   // a first reading left early is not read: it waits again next time
        if (enterKey(in)) { if (speaking && shardFresh) { stopSpeech(); markShardRead(); } else if (speaking) stopSpeech(); else speakShardPage(shardPage); return; }   // Enter skips the wait, stops, or speaks the page
        if (in.wasPressed(KEY_SPACE) && speaking) { speechPaused = !speechPaused; audio.speechPause = speechPaused; }
        const bool up = in.wasPressed(KEY_UP) || in.wasPressed(KEY_PAGE_UP), down = in.wasPressed(KEY_DOWN) || in.wasPressed(KEY_PAGE_DOWN);
        if (pages > 1 && (up || down)) {   // a page turned: the voice stops (a first reading is read at once)
            int to = std::max(0, std::min(pages - 1, shardPage + (down ? 1 : -1)));
            if (to != shardPage) { if (shardFresh) markShardRead(); stopSpeech(); shardPage = to; audio.beep = BEEP_UI; }
            return;
        }
        if (in.wasPressed(KEY_LEFT) || up) { stepShard(-1); openShard(shardSel); }
        else if (in.wasPressed(KEY_RIGHT) || down) { stepShard(1); openShard(shardSel); }
    }
}

void Game::renderShards() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 225);
    int nw = (int)shardWorldKeys.size();
    if (shardsLevel == 0 && shardsTop == 1) { renderPlayerPage(); return; }
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
                drawText(canvas, 16, y, fmt("%s  (%+lld %+lld %+lld / %d)%s", upper(worldNameOfKey(key)).c_str(), (long long)sx, (long long)sy, (long long)sz, bodyOfKey(key), shardWorldLabels[i].empty() ? "" : ("  " + shardWorldLabels[i]).c_str()).c_str(), sel ? HUD_WHITE : (nameIsForeign(key) ? HUD_CYAN : HUD_GREEN));   // a friend's name in cyan
                if (sel) drawText(canvas, 6, y, ">", HUD_AMBER);
                const int total = i < (int)shardWorldTotals.size() ? shardWorldTotals[i] : SHARDS_PER_WORLD;   // the people's records
                const int pct = (int)std::lround(100 * languageShare(held, total));
                const std::string have = lent > 0 ? fmt("%d OF %d SHARDS (%d LENT)", held, total, lent) : fmt("%d OF %d SHARDS", held, total);
                std::string row = lent > 0 ? fmt("%s   LANGUAGE %d%%   %d READ", have.c_str(), pct, read) : fmt("%s   THE LANGUAGE %d%% LEARNT   %d READ", have.c_str(), pct, read);   // by the people's records
                if (16 + textWidth(row.c_str()) > UW - 4) row = fmt("%s   LANGUAGE %d%%   %d READ", have.c_str(), pct, read);   // three-figure counts ran off the screen ("16 RE")
                if (16 + textWidth(row.c_str()) > UW - 4) row = fmt("%s   %d READ", have.c_str(), read);
                drawText(canvas, 16, y + 9, row.c_str(), HUD_DIM);
            }
        }
        drawTextCentered(canvas, UW / 2, UH - 10, nw ? "UP/DOWN  ENTER A WORLD  TAB THE PLAYER  ESC CLOSE" : "TAB THE PLAYER  ESC CLOSE", HUD_DIM);
        return;
    }
    const std::string& key = shardWorld.key;
    int n = (int)shardHeld.size(), pct = (int)std::lround(100 * languageShare(n, shardWorld.lore.records));
    std::string name = upper(worldNameOfKey(key));
    if (shardsLevel == 1) {
        drawText(canvas, 6, 4, (shardWorld.peoples > 1 ? fmt("%s - THE %s - %d OF %d - %d%% LEARNT", trunc(name, 12).c_str(), trunc(upper(shardWorld.lore.people), 9).c_str(), n, shardWorld.lore.records, pct)   // the people named
                                                     : fmt("%s - %d OF %d SHARDS - LANGUAGE %d%%", trunc(name, 18).c_str(), n, shardWorld.lore.records, pct)).c_str(), HUD_AMBER);   // 52 characters with three-figure counts
        if (shardsPage == 1) renderShardTimeline();        // game/history_screen.cpp
        else if (shardsPage == 2) renderShardHistory();
        else renderShardRecords();
        return;
    }
    if (shardWorld.shards[shardHeld[shardSel]].music) { renderPiece(); return; }
    if (shardWorld.shards[shardHeld[shardSel]].chart) { renderChart(); return; }
    // a shard: the voice named, the words laid out at the text's width, each in its own colour (a name amber, a word read
    // white, one not yet learnt dim in the people's tongue); on a first reading they resolve as the recording speaks them, the
    // one being spoken flickering; on a reading done the word being spoken is underlined
    int idx = shardHeld[shardSel]; const Shard& s = shardWorld.shards[idx];
    bool theLast = s.last && guide.ended.count(endedKey()) > 0;   // named as the last once read, not before
    std::string head = theLast ? fmt("%s - THE LAST RECORDING", shardWorldTitle(22).c_str()) : fmt("%s - SHARD %d OF %d", shardWorldTitle(22).c_str(), shardSel + 1, n);   // short of the medium's picture
    drawText(canvas, 6, 4, head.c_str(), HUD_AMBER);
    if (shardLent(idx)) drawText(canvas, 6 + textWidth(head.c_str()) + 2 * FONT_ADV, 4, "LENT", HUD_CYAN);   // a friend's, credited
    const std::string date = upper(shardDate(shardWorld.lore, s));
    drawText(canvas, 6, 13, date.c_str(), HUD_DIM);   // in the people's reckoning
    drawText(canvas, 6, 22, mediumLine(s).c_str(), HUD_DIM);   // what it was made on (R-414: no more: not the voice, nor who reads it)
    drawMediumPanel(UW - 46, 3, s, speechT >= 0 && !speechEnded() && !speechPaused, realTime);
    int total = (int)shardWords.size();
    bool speaking = speechT >= 0 && !speechEnded();
    int spoken = speaking ? speechWordAt(shardWorld.speech, speechT) : -1;
    if (spoken >= 0) spoken += speechFrom;   // the speech is a page's
    int cursor = shardFresh && speaking ? std::max(speechFrom, spoken) : (shardFresh ? speechFrom : total);
    Rng flick((uint64_t)(realTime * 20));
    const int pages = (int)shardPageStart.size();
    {   // the page on the screen, its words flowed as they are shown now (a first reading's words resolve in place)
        const int from = shardPage < pages ? shardPageStart[shardPage] : 0, to = shardPage + 1 < pages ? shardPageStart[shardPage + 1] : total;
        int x = 12, y = 33; const int maxX = UW - 12;
        for (int i = from; i < to && i < total; i++) {
            const DecodedWord& w = shardWords[i];
            std::string shown; uint32_t col;
            if (i < cursor) { shown = w.pre + (w.known ? w.ours : w.theirs) + w.post; col = w.name ? HUD_AMBER : (w.known ? HUD_WHITE : HUD_DIM); }
            else if (i == cursor && cursor < total) { shown = w.theirs; for (char& c : shown) c = (char)('A' + flick.irange(26)); col = HUD_GREEN; }
            else { shown = w.pre + w.theirs + w.post; col = HUD_DIM; }
            shown = upper(shown);
            int tw = textWidth(shown.c_str());
            if (w.par && i > from) { x = 12; y += 12; } else if (x > 12 && x + tw > maxX) { x = 12; y += 9; }
            if (y > UH - 36) break;
            drawText(canvas, x, y, shown.c_str(), col);
            if (i == spoken && !shardFresh) fillRectRGB(canvas, x, y + 7, x + tw - 1, y + 7, HUD_GREEN);   // the word being spoken, underlined
            x += tw + FONT_ADV;
        }
    }
    if (pages > 1) {   // before the medium's picture, on the medium's line or else the date's, two characters clear of it (the keys say UP/DOWN PAGE)
        std::string pg = fmt("PAGE %d/%d", shardPage + 1, pages); const int px = UW - 50 - textWidth(pg.c_str());
        if (px >= 6 + textWidth(mediumLine(s).c_str()) + 2 * FONT_ADV) drawText(canvas, px, 22, pg.c_str(), HUD_DIM);
        else if (px >= 6 + textWidth(date.c_str()) + 2 * FONT_ADV) drawText(canvas, px, 13, pg.c_str(), HUD_DIM);
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
            drawTextCentered(canvas, UW / 2, UH - 10, pages > 1 ? "ENTER STOPS  UP/DOWN PAGE  LEFT/RIGHT  ESC BACK" : "ENTER STOPS  SPACE PAUSE  LEFT/RIGHT  ESC BACK", HUD_DIM);
        }
    } else {
        drawText(canvas, 12, UH - 22, fmt("%d OF %d WORDS READ   THE LANGUAGE %d%% (%d SHARD%s)", read, total, pct, n, n == 1 ? "" : "S").c_str(), HUD_GREEN);
        drawTextCentered(canvas, UW / 2, UH - 10, pages > 1 ? "ENTER READS THE PAGE  UP/DOWN PAGE  LEFT/RIGHT" : "ENTER PLAYS IT AGAIN  LEFT/RIGHT ANOTHER  ESC BACK", HUD_DIM);
    }
}

// a record of the kind the records' page shows
bool Game::shardInFilter(int local) const {
    if (shardsFilter == 0 || local < 0 || local >= (int)shardWorld.shards.size()) return true;
    return recordKindOf(shardWorld.shards[local]) == shardsFilter - 1;
}
// the cursor to the next record held of the kind shown (stop: a single step stops at the ends too)
// dir +-1: the next record held of the kind the records' page shows, round from the end; more (or `stop`, R-417's run): that many on,
// stopping at the end
void Game::stepShard(int dir, bool stop) {
    const int n = (int)shardHeld.size();
    if (n == 0) return;
    const bool wrap = !stop && (dir == 1 || dir == -1);
    const int sgn = dir > 0 ? 1 : -1;
    for (int left = std::abs(dir); left > 0; left--) {
        int j = shardSel, found = -1;
        for (int k = 0; k < n && found < 0; k++) {
            j += sgn;
            if (j < 0 || j >= n) { if (!wrap) break; j = (j + n) % n; }
            if (shardsPage != 0 || shardInFilter(shardHeld[j])) found = j;
        }
        if (found < 0) break;
        shardSel = found;
    }
}
// R-416: Shift with an arrow held runs the timeline's and the history's cursor (the user's: "a quick scroll ... so you don't have to click
// right/left or up/down one event at a time"; R-417: the records' too, Up and Down): a step as the arrow (or Shift) goes down, then from a
// fifth of a second on ever faster, 12 a second doubling every half second to 100 (a hundred records in about two seconds); the steps
// this frame, back negative
int Game::shardRunSteps(const Input& in, double realDt, bool leftRight) {
    const bool fwd = in.isDown(KEY_DOWN) || (leftRight && in.isDown(KEY_RIGHT)), back = in.isDown(KEY_UP) || (leftRight && in.isDown(KEY_LEFT));
    const int dir = in.shift() ? (fwd ? 1 : 0) - (back ? 1 : 0) : 0;
    if (dir == 0) { shardRun = {}; return 0; }
    const bool fresh = in.wasPressed(KEY_LEFT_SHIFT) || in.wasPressed(KEY_RIGHT_SHIFT) || in.wasPressed(dir > 0 ? KEY_DOWN : KEY_UP) ||
                       (leftRight && in.wasPressed(dir > 0 ? KEY_RIGHT : KEY_LEFT));
    if (fresh || dir != shardRun.dir) { shardRun = {}; shardRun.dir = dir; return dir; }
    const double dt = std::min(realDt, 0.1);   // a slow frame (the history read again) no leap
    shardRun.t += dt;
    if (shardRun.t < 0.2) return 0;
    shardRun.acc += std::min(100.0, 12.0 * std::exp2((shardRun.t - 0.2) / 0.5)) * dt;
    const int steps = (int)shardRun.acc;
    shardRun.acc -= steps;
    return dir * steps;
}
// a record as the list names it ("A LETTER: ...", a piece's form, a chart's caption)
// a record as the lists name it: a piece by its name once given (C-14: or a friend's), else its form and length; a text, or a chart's caption,
// in the words the computer last read it with (an unread one all in the people's tongue)
std::string Game::recordSummary(int idx, int maxChars) const {
    const Shard& s = shardWorld.shards[idx];
    auto d = guide.decoded.find(heldShardKey(idx));
    const int readAt = d == guide.decoded.end() ? 0 : d->second;
    std::string what;
    if (s.music) {
        bool foreign = false; const std::string* nm = shardNameOf(heldShardKey(idx), foreign);
        what = nm ? fmt("'%s' - %s", upper(*nm).c_str(), PIECE_FORM_NAMES[shardWorld.pieceForm[idx]]) : fmt("%s, %d S", PIECE_FORM_NAMES[shardWorld.pieceForm[idx]], (int)std::lround(shardWorld.pieceSeconds[idx]));
    } else { std::vector<DecodedWord> w; decodeShard(s, shardWorld.lang, shardWorld.tongue, readAt, w); what = (s.chart ? "A CHART: " : "") + decodedText(w); }   // a chart by its caption
    return trunc(upper(what), maxChars);
}
// the pages' strip under the title: the three pages, the one shown white; the page's own note at the right
void Game::drawShardPagesStrip(const std::string& right) {
    static const char* const PAGES[3] = {"RECORDS", "TIMELINE", "HISTORY"};
    int x = 6;
    for (int p = 0; p < 3; p++) {
        const int w = textWidth(PAGES[p]);
        drawText(canvas, x, 14, PAGES[p], p == shardsPage ? HUD_WHITE : HUD_DIM);
        if (p == shardsPage) fillRectRGB(canvas, x, 21, x + w - 1, 21, HUD_GREEN);
        x += w + 2 * FONT_ADV;
    }
    if (!right.empty()) drawText(canvas, UW - 6 - textWidth(right.c_str()), 14, right.c_str(), HUD_DIM);
}

// the records page (C-06's list; H-06: under the pages' strip, each row's kind before it, one kind shown on Left/Right)
void Game::renderShardRecords() {
    const int n = (int)shardHeld.size();
    int count[RC_COUNT] = {0, 0, 0};
    for (int i : shardHeld) count[recordKindOf(shardWorld.shards[i])]++;
    std::vector<int> rows;   // the rows shown: the held of the kind shown, in their years' order
    for (int i = 0; i < n; i++) if (shardInFilter(shardHeld[i])) rows.push_back(i);
    int at = 0; for (size_t r = 0; r < rows.size(); r++) if (rows[r] == shardSel) at = (int)r;
    const int page = at / SHARD_ROWS, pages = ((int)rows.size() + SHARD_ROWS - 1) / SHARD_ROWS;
    drawShardPagesStrip(pages > 1 ? fmt("%d/%d", page + 1, pages) : std::string());
    {   // the kinds held, each by its glyph and its count, from the right (the page's own count before them): the kind shown white, all green when all are
        int x = UW - 6 - (pages > 1 ? textWidth(fmt("%d/%d", page + 1, pages).c_str()) + 3 * FONT_ADV : 0);
        for (int k = RC_COUNT - 1; k >= 0; k--) {
            const std::string c = std::to_string(count[k]);
            x -= textWidth(c.c_str());
            const uint32_t col = shardsFilter == 0 ? HUD_GREEN : (shardsFilter - 1 == k ? HUD_WHITE : HUD_DIM);
            drawText(canvas, x, 14, c.c_str(), col);
            x -= 7; drawRecordKindGlyph(canvas, x, 14, k, col);
            x -= 2 * FONT_ADV;
        }
    }
    bool anyPending = false;
    for (int r = page * SHARD_ROWS; r < (int)rows.size() && r < (page + 1) * SHARD_ROWS; r++) {
        const int i = rows[r];
        int y = 25 + (r - page * SHARD_ROWS) * 9;
        int idx = shardHeld[i]; const Shard& s = shardWorld.shards[idx];
        auto d = guide.decoded.find(heldShardKey(idx));
        int readAt = d == guide.decoded.end() ? 0 : d->second;
        bool sel = i == shardSel, pending = s.music ? readAt == 0 : readAt < n;
        anyPending = anyPending || pending;
        const uint32_t col = sel ? HUD_WHITE : (shardLent(idx) ? HUD_CYAN : (readAt == 0 ? HUD_DIM : HUD_GREEN));   // a friend's in cyan
        drawRecordKindGlyph(canvas, 16, y, recordKindOf(s), col);
        const std::string yr = fmt("%6s", thousands(s.year).c_str());   // the year in their count (the strip says what the column is)
        drawText(canvas, 24, y, (yr + "  " + recordSummary(idx, 38)).c_str(), col);
        if (s.last && guide.ended.count(endedKey())) drawText(canvas, 24, y, yr.c_str(), HUD_AMBER);   // the last recording, once read
        if (pending) drawText(canvas, UW - 12, y, "*", HUD_AMBER);
        if (sel) drawText(canvas, 6, y, ">", HUD_AMBER);
    }
    if (rows.empty()) drawTextCentered(canvas, UW / 2, 80, "NO RECORD OF THIS KIND HELD", HUD_DIM);
    {   // the people's calendar and the timeline of what is held: the years from the founding along a rule, a tick a shard (read green, unread
        // dim, the one under the cursor white), the rule dotted past the latest held until the last recording is read, when it closes in amber
        const Lore& L = shardWorld.lore;
        drawText(canvas, 12, 160, upper(calendarLine(L)).c_str(), HUD_DIM);
        bool ended = guide.ended.count(endedKey()) > 0;
        const int tx0 = 12, tx1 = UW - 12, ty = 180;
        int lastYear = L.spanYears + 1, latest = 1;
        for (int i : shardHeld) latest = std::max(latest, shardWorld.shards[i].year);
        auto xOf = [&](int yr) { return tx0 + (int)std::lround((tx1 - tx0) * clampd((yr - 1) / (double)std::max(1, lastYear - 1), 0, 1)); };
        drawText(canvas, tx0, 169, "YEAR 1", HUD_DIM);
        if (ended) {
            fillRectRGB(canvas, tx0, ty, tx1, ty, HUD_DIM);
            fillRectRGB(canvas, tx1 - 1, ty - 3, tx1, ty + 3, HUD_AMBER);
            std::string endLabel = fmt("THE RECORD ENDS, YEAR %s", thousands(lastYear).c_str());   // as the timeline page writes it
            drawText(canvas, tx1 - textWidth(endLabel.c_str()), 169, endLabel.c_str(), HUD_AMBER);
        } else {
            int xl = xOf(latest);
            fillRectRGB(canvas, tx0, ty, xl, ty, HUD_DIM);
            for (int x = xl + 3; x <= tx1; x += 3) fillRectRGB(canvas, x, ty, x, ty, HUD_DIM);
        }
        for (int i = 0; i < n; i++) {
            if (!shardInFilter(shardHeld[i])) continue;
            const Shard& s = shardWorld.shards[shardHeld[i]];
            bool read = guide.decoded.count(heldShardKey(shardHeld[i])) > 0;
            uint32_t col = i == shardSel ? HUD_WHITE : (s.last && ended && read ? HUD_AMBER : (shardLent(shardHeld[i]) ? HUD_CYAN : (read ? HUD_GREEN : HUD_DIM)));   // a friend's tick in cyan
            int x = xOf(s.year);
            fillRectRGB(canvas, x, ty - 3, x, ty + 3, col);
        }
    }
    {   // SHIFT FAST beside the rest: the kind's keys drawn as the font's arrows (LEFT/RIGHT left no room: 59 characters of 53)
        const std::string kind = std::string(1, CH_LEFT) + std::string(1, CH_RIGHT) + " KIND";
        drawTextCentered(canvas, UW / 2, UH - 10, ("ENTER READ  " + kind + "  SHIFT FAST  TAB PAGE" + (anyPending ? "   * UNREAD" : "  ESC BACK")).c_str(), HUD_DIM);
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
    std::string head = fmt("%s - SHARD %d OF %d", shardWorldTitle(22).c_str(), shardSel + 1, n);   // short of the medium's picture
    drawText(canvas, 6, 4, head.c_str(), HUD_AMBER);
    if (shardLent(idx)) drawText(canvas, 6 + textWidth(head.c_str()) + 2 * FONT_ADV, 4, "LENT", HUD_CYAN);   // a friend's, credited
    drawText(canvas, 6, 13, upper(shardDate(shardWorld.lore, s)).c_str(), HUD_DIM);
    drawText(canvas, 6, 22, mediumLine(s).c_str(), HUD_DIM);   // what it was made on (R-414: the object alone)
    drawMediumPanel(UW - 46, 3, s, pieceT >= 0 && !piecePaused && !pieceEnded(), realTime);
    bool foreign = false; const std::string* nm = shardNameOf(heldShardKey(idx), foreign);   // a friend's name for it, in cyan
    std::string title = nm ? fmt("'%s' - %s, %d S", upper(*nm).c_str(), PIECE_FORM_NAMES[P.form], (int)std::lround(P.seconds))
                           : fmt("%s OF THE %s, %d S", PIECE_FORM_NAMES[P.form], upper(shardWorld.lore.people).c_str(), (int)std::lround(P.seconds));
    drawText(canvas, 12, 31, trunc(title, 52).c_str(), foreign ? HUD_CYAN : HUD_WHITE);
    drawText(canvas, 12, 40, traditionLine(T).c_str(), HUD_DIM);
    drawText(canvas, 12, 49, cycleLine(T, P).c_str(), HUD_DIM);     // the field's line in the far future
    drawText(canvas, 12, 58, timbresLine(T, P).c_str(), HUD_DIM);   // the instruments of the piece's age
    const int rx0 = 12, rx1 = UW - 12, ry0 = 69, ry1 = UH - 34;   // under the medium line and the age's instruments (the tune's top row two over its note)
    double beat = pieceBeatNow();
    int minD = 1000, maxD = -1000;
    for (const Note& nt : P.notes) if (nt.voice >= 0) { minD = std::min(minD, nt.degree); maxD = std::max(maxD, nt.degree); }
    if (minD > maxD) { minD = 0; maxD = 1; }
    double span = std::max(1, maxD - minD);
    auto xOf = [&](double t) { return rx0 + (int)std::lround((rx1 - rx0) * clampd(t / std::max(P.beats, 1e-9), 0, 1)); };
    auto yOf = [&](int d) { return ry1 - 8 - (int)std::lround((ry1 - 8 - ry0) * (d - minD) / span); };
    const uint32_t grid = rgb(28, 40, 32), dimNote = HUD_DIM;
    // the parts by what they play in the piece's ensemble: the tune white (three rows), the second voices and the chorus green, the chords
    // and the swarms a darker cyan (one row), the bass and the drones a dim amber, the kit's pieces amber ticks in rows of their own below
    const Ensemble& E = pieceEnsemble(T, P);
    if (P.age != AGE_FAR_FUTURE) for (int c = 0; c <= P.cycles; c++) { int x = xOf(c * T.beats); fillRectRGB(canvas, x, ry0, x, ry1, grid); }
    for (const Note& nt : P.notes) {
        int x0 = xOf(nt.t), x1 = std::max(x0, xOf(nt.t + nt.dur) - 1);
        bool played = nt.t <= beat;
        if (nt.voice < 0) { int y = ry1 - 1 - 2 * std::min(nt.degree, 3); fillRectRGB(canvas, x0, y, x0, y + 1, played ? HUD_AMBER : dimNote); continue; }
        int y = yOf(nt.degree);
        const int role = nt.voice < (int)E.role.size() ? E.role[nt.voice] : MR_ALT;
        const bool tune = role == MR_LEAD || role == MR_SINGER || role == MR_BOX, low = role == MR_BASS || role == MR_DRONE, back = role == MR_CHORDS || role == MR_KEYS || role == MR_ORGAN || role == MR_SWARM;
        uint32_t col = !played ? dimNote : (tune ? HUD_WHITE : (low ? rgb(170, 130, 60) : (back ? rgb(60, 150, 150) : (role == MR_BELL ? HUD_CYAN : HUD_GREEN))));
        if (back) fillRectRGB(canvas, x0, y, x1, y, col);                          // one row a chord's note
        else fillRectRGB(canvas, x0, y - 1, x1, y, col);                           // two rows a note
        if (tune && played) fillRectRGB(canvas, x0, y - 2, x1, y - 2, col);         // the tune three
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

// C-10: the star charts on the decoder and held up to the sky are game/chart_draw.cpp's (H-05)
