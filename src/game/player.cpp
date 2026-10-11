// C-11 (2026-10-10): the ship's player (KI-347). C-04's pieces and C-05's recordings played only on the decoder's screen; with flights of a
// minute and a half and a radar that hears voices thousands of years old, the ship plays them on. The decoder's player page (Tab at its
// worlds) lists every piece held, every recording held (a record made on something that sounds: a wax cylinder, a disc, a reel, a drive, a
// crystal, something that hums; H-05's media) and every recording the radar caught (`Guide::heard`: its opening, as the ship kept it off the
// air), by their worlds and years; Enter plays one and the list on from it, Space pauses, Backspace stops. It plays wherever the ship goes (the
// cabin, the window, the maps, a Vimana flight: a line on the space HUD) and stops at a landing. The synth's piece and speech point into the
// player's own copy of the world (`ShipPlayer::world`), so nothing the decoder opens moves them; while the decoder plays a record of its own,
// the radar listens or the gramophone turns, the player is held and takes up where it was (`AudioState::pieceFrom`, `speechFrom`).
#include "game.h"
#include "ui.h"
#include "galaxy/records.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>

namespace {
int bodyOfKeyP(const std::string& key) { size_t sl = key.find('/'); return sl == std::string::npos ? -1 : atoi(key.c_str() + sl + 1); }
std::string mmss(double s) { const int t = (int)std::floor(std::max(0.0, s)); return fmt("%d:%02d", t / 60, t % 60); }
}
std::string Game::playerItemKey(const PlayerItem& it) { return it.world + "/S" + std::to_string(shardWorldIndex(it.which, it.local)); }

std::string Game::playerHudLine() const {
    if (player.playing < 0 || player.playing >= (int)player.items.size()) return std::string();
    const std::string when = player.waiting ? std::string("...") : (player.paused ? std::string("PAUSED") : mmss(player.t) + (player.secs > 0 ? "/" + mmss(player.secs) : std::string()));
    const int room = 36 - (int)when.size() - 2;
    const std::string whole = player.title + " - " + player.people;
    return ((int)whole.size() <= room ? whole : ((int)player.title.size() <= room ? player.title : trunc(player.title, room))) + "  " + when;   // the people left out before the title is cut
}

bool Game::playerOwns() const { return (audio.piece && audio.piece == &player.world.piece) || (audio.speech && audio.speech == &player.world.speech); }

// the list: what the guide holds that sounds and what the radar caught, each world's pieces and recordings by their years
void Game::playerBuildList() {
    const std::string playingKey = player.playing >= 0 && player.playing < (int)player.items.size() ? playerItemKey(player.items[player.playing]) : std::string();
    struct W { bool ok = false; StarSystem sys; int bi = -1; BodyGen g; Lore lore[2]; Tradition T[2]; bool tOk[2] = {false, false}; std::shared_ptr<const RecordCatalogue> C[2]; };
    std::map<std::string, W> worlds;
    auto worldOf = [&](const std::string& key) -> W& {
        auto it = worlds.find(key);
        if (it != worlds.end()) return it->second;
        W& w = worlds[key];
        int64_t sx, sy, sz; Star st;
        if (!Guide::parseStarKey(key, sx, sy, sz) || !starInSector(sx, sy, sz, st, true)) return w;
        w.sys.generate(st); w.bi = bodyOfKeyP(key);
        if (w.bi < 0 || w.bi >= (int)w.sys.bodies.size()) return w;
        w.g = BodyGen::make(w.sys.bodies[w.bi]);
        w.ok = worldHadCivilisation(w.g);
        return w;
    };
    std::set<std::string> held(guide.shards.begin(), guide.shards.end());
    for (const std::string& k : guide.lent) held.insert(k);
    std::set<std::string> keys(held); for (const std::string& k : guide.heard) keys.insert(k);
    std::vector<PlayerItem> items;
    for (const std::string& k : keys) {
        const size_t at = k.rfind("/S");
        if (at == std::string::npos) continue;
        const int idx = atoi(k.c_str() + at + 2), which = shardPeopleOf(idx), local = shardLocalOf(idx);
        if (which < 0 || which > 1) continue;
        const std::string wk = k.substr(0, at);
        W& w = worldOf(wk);
        if (!w.ok) continue;
        if (!w.C[which]) { w.C[which] = recordsOf(w.g, which); w.lore[which] = loreQuick(w.sys, w.sys.bodies[w.bi], w.g, which); }
        const RecordCatalogue& C = *w.C[which];
        if (local < 0 || local >= C.count || local >= (int)C.entries.size()) continue;
        const RecordEntry& e = C.entries[local];
        if (e.category == RC_CHART) continue;   // a chart is looked at, not played
        const bool music = e.category == RC_MUSIC, asHeld = held.count(k) && (music || recordMediumSounds(e.medium));
        if (!asHeld && !guide.heard.count(k)) continue;
        PlayerItem it; it.world = wk; it.which = which; it.local = local; it.music = music; it.heard = !asHeld; it.medium = asHeld ? e.medium : MEDIUM_AIR; it.year = e.year;
        it.people = "THE " + upper(w.lore[which].people);
        if (music) {   // its form and its length, from its draws without the people's texts (once a piece)
            auto pinfo = player.pieceInfo.find(k);
            if (pinfo == player.pieceInfo.end()) {
                if (!w.tOk[which]) { w.T[which] = traditionOf(w.g, w.lore[which]); w.tOk[which] = true; }
                Shard sh = shardDrawOf(w.g, w.lore[which], local); sh.recordAge = e.age; sh.medium = e.medium;
                Piece pc; pieceOf(w.T[which], sh, pc);
                pinfo = player.pieceInfo.emplace(k, std::make_pair(pc.form, pc.seconds)).first;
            }
            it.form = pinfo->second.first; it.seconds = pinfo->second.second;
        }
        items.push_back(it);
    }
    std::sort(items.begin(), items.end(), [](const PlayerItem& a, const PlayerItem& b) {
        if (a.world != b.world) return a.world < b.world;
        if (a.which != b.which) return a.which < b.which;
        if (a.year != b.year) return a.year < b.year;
        return a.local < b.local;
    });
    for (size_t i = 0; i < items.size() && i < player.items.size(); i++) if (playerItemKey(items[i]) == playerItemKey(player.items[i]) && player.items[i].seconds > 0 && !items[i].music) items[i].seconds = player.items[i].seconds;
    player.items = items;
    player.playing = -1;
    for (size_t i = 0; i < player.items.size() && !playingKey.empty(); i++) if (playerItemKey(player.items[i]) == playingKey) player.playing = (int)i;
    if (!playingKey.empty() && player.playing < 0) playerStop();   // what played is no longer held (a lent shard taken back)
    player.sel = std::max(0, std::min(player.sel, (int)player.items.size() - 1));
    player.guideVersion = (int)(guide.shards.size() * 7919 + guide.lent.size() * 104729 + guide.heard.size());
}

std::string Game::playerItemTitle(const PlayerItem& it, const std::string* name) {
    std::string t = it.music ? (name ? "'" + upper(*name) + "'" : std::string(PIECE_FORM_NAMES[std::max(0, std::min(PF_COUNT - 1, it.form))]))
                             : (it.heard ? std::string("A RECORDING") : std::string(RECORD_MEDIUM_HUD[std::max(0, std::min(RM_COUNT - 1, it.medium))]));
    return it.heard ? "AIR: " + t : t;
}

void Game::playerPlay(int i) {
    if (i < 0 || i >= (int)player.items.size()) return;
    if (playerOwns()) { if (audio.piece == &player.world.piece) { audio.piece = nullptr; audio.tradition = nullptr; } if (audio.speech == &player.world.speech) { audio.speech = nullptr; audio.voice = nullptr; } audio.medium = -1; }
    player.playing = i; player.t = 0; player.syncT = -1; player.paused = false; player.held = false; player.waiting = true; player.secs = player.items[i].seconds;
    const PlayerItem& it = player.items[i];
    bool foreign = false; const std::string* nm = it.music ? shardNameOf(playerItemKey(it), foreign) : nullptr;
    player.title = playerItemTitle(it, nm); player.people = it.people;
    playerStart();
}

// the playing item onto the synth from `player.t`: its world built (on a thread the first time: `waiting` until its records are written), the
// piece or the speech made again (deterministic), the synth told where to begin
void Game::playerStart() {
    if (player.playing < 0 || player.playing >= (int)player.items.size()) return;
    PlayerItem& it = player.items[player.playing];
    ShardWorld& w = player.world;
    if (!(w.valid && w.key == it.world && w.which == it.which)) {
        if (playerOwns()) { audio.piece = nullptr; audio.tradition = nullptr; audio.speech = nullptr; audio.voice = nullptr; audio.medium = -1; }   // before the world they point into goes
        int64_t sx, sy, sz; Star st;
        if (!Guide::parseStarKey(it.world, sx, sy, sz) || !starInSector(sx, sy, sz, st, true)) { playerStop(); return; }
        w = ShardWorld(); w.key = it.world;
        if (!buildShardWorld(w, st, bodyOfKeyP(it.world), it.which, false)) {
            if (w.pending) { player.waiting = true; return; }   // its records are being written: the next frames try again
            playerStop(); return;
        }
    }
    if (it.local < 0 || it.local >= (int)w.shards.size()) { playerStop(); return; }
    player.waiting = false;
    const Shard& sh = w.shards[it.local];
    audio.medium = it.heard ? MEDIUM_AIR : sh.medium;
    if (it.music) {
        pieceOf(w.tradition, sh, w.piece);
        player.secs = it.seconds = w.piece.seconds;
        audio.speech = nullptr; audio.voice = nullptr;
        audio.piece = &w.piece; audio.tradition = &w.tradition; audio.pieceSpeed = 1;
        audio.pieceFrom = player.t > 0.05 ? player.t * w.piece.bpm / 60.0 : 0;
        audio.pieceStart = true; audio.piecePause = player.paused; audio.pieceDone = false; audio.pieceBeat = -1;
    } else {
        std::vector<DecodedWord> words; decodeShard(sh, w.lang, w.tongue, w.lore.records, words);
        if (it.heard) words.resize(shardOpening(words));   // what was on the air: the record's opening
        if (!words.empty()) words[0].par = false;
        const bool own = it.heard || sh.medium < 0 || recordMediumSounds(sh.medium);
        speechOf(own ? w.voice : w.readVoice, words, sh.seed, w.speech);
        player.secs = it.seconds = w.speech.seconds;
        audio.piece = nullptr; audio.tradition = nullptr;
        audio.speech = &w.speech; audio.voice = own ? &w.voice : &w.readVoice;
        audio.speechFrom = player.t > 0.05 ? player.t : 0;
        audio.speechStart = true; audio.speechPause = player.paused; audio.speechDone = false; audio.speechT = -1;
    }
    player.syncT = -1;
    if (player.playing + 1 < (int)player.items.size()) {   // the next one's world written meanwhile, so the list moves on without a wait
        const PlayerItem& nx = player.items[player.playing + 1];
        int64_t sx, sy, sz; Star st;
        if ((nx.world != it.world || nx.which != it.which) && Guide::parseStarKey(nx.world, sx, sy, sz) && starInSector(sx, sy, sz, st, true)) {
            StarSystem wsys; wsys.generate(st); const int bi = bodyOfKeyP(nx.world);
            if (bi >= 0 && bi < (int)wsys.bodies.size()) { const BodyGen g = BodyGen::make(wsys.bodies[bi]); shardTextsPrefetch(wsys, wsys.bodies[bi], g, loreQuick(wsys, wsys.bodies[bi], g, nx.which)); }
        }
    }
}

void Game::playerStop() {
    if (playerOwns()) {
        if (audio.piece == &player.world.piece) { audio.piece = nullptr; audio.tradition = nullptr; audio.pieceStart = false; audio.piecePause = false; audio.pieceBeat = -1; }
        if (audio.speech == &player.world.speech) { audio.speech = nullptr; audio.voice = nullptr; audio.speechStart = false; audio.speechPause = false; audio.speechT = -1; }
        audio.medium = -1;
    }
    player.playing = -1; player.paused = false; player.held = false; player.waiting = false; player.t = 0; player.secs = 0; player.syncT = -1;
}

void Game::updatePlayer(double realDt) {
    if (player.playing < 0) return;
    if (state == GameState::DESCENT || state == GameState::SURFACE || state == GameState::ASCENT || state == GameState::TITLE) { playerStop(); return; }   // a landing stops it
    const bool mineP = audio.piece && audio.piece == &player.world.piece, mineS = audio.speech && audio.speech == &player.world.speech;
    if (radar.on) {   // the receiver listens: the player holds its place
        if (mineP || mineS) { if (mineP) { audio.piece = nullptr; audio.tradition = nullptr; } if (mineS) { audio.speech = nullptr; audio.voice = nullptr; } audio.medium = -1; }
        player.held = true; return;
    }
    const bool other = (audio.piece && !mineP) || (audio.speech && !mineS) || audio.radio;
    if (other) {   // the decoder's own record, the gramophone: theirs while they play, one sound at a time (ours let go of)
        if (mineP) { audio.piece = nullptr; audio.tradition = nullptr; }
        if (mineS) { audio.speech = nullptr; audio.voice = nullptr; }
        player.held = true; return;
    }
    if (player.held || player.waiting || (!mineP && !mineS)) { player.held = false; playerStart(); return; }   // taken up where it was
    if (player.paused) return;
    // the clock: the synth's while it runs (the main thread reads its report after each block), else the game's (the harness runs no synth)
    if (mineP && audio.pieceBeat >= 0 && audio.pieceBeat != player.syncT) { player.syncT = audio.pieceBeat; player.t = audio.pieceBeat * 60.0 / std::max(1.0, player.world.piece.bpm); }
    else if (mineS && audio.speechT >= 0 && audio.speechT != player.syncT) { player.syncT = audio.speechT; player.t = audio.speechT; }
    else player.t += realDt;
    const bool done = (mineP && audio.pieceDone && player.t > 0.5) || (mineS && audio.speechDone && player.t > 0.5) || (player.secs > 0 && player.t >= player.secs + 1.5);
    if (!done) return;
    player.finished++;
    const int next = player.playing + 1;
    if (next < (int)player.items.size()) playerPlay(next);   // the list on
    else playerStop();
}

// the decoder's player page: the list, the one playing marked with its clock, the keys
void Game::renderPlayerPage() {
    drawText(canvas, 6, 4, "THE PLAYER", HUD_AMBER);
    const int n = (int)player.items.size(), rows = 15;
    const std::string head = n == 1 ? "1 PIECE OR RECORDING" : fmt("%d PIECES AND RECORDINGS", n);
    drawText(canvas, UW - 6 - textWidth(head.c_str()), 4, head.c_str(), HUD_DIM);
    if (n == 0) {
        drawTextCentered(canvas, UW / 2, 80, "NOTHING TO PLAY YET", HUD_GREEN);
        drawTextCentered(canvas, UW / 2, 92, "THE PIECES AND THE RECORDINGS TAKEN FROM THE RUINS", HUD_DIM);
        drawTextCentered(canvas, UW / 2, 101, "AND WHAT THE RADAR CAUGHT PLAY HERE", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 10, "TAB THE WORLDS  ESC CLOSE", HUD_DIM);
        return;
    }
    if (player.sel < player.top) player.top = player.sel;
    if (player.sel >= player.top + rows) player.top = player.sel - rows + 1;
    std::string lastWorld;
    for (int i = player.top; i < n && i < player.top + rows; i++) {
        const PlayerItem& it = player.items[i];
        const int y = 18 + (i - player.top) * 10;
        const bool sel = i == player.sel, playing = i == player.playing;
        bool foreign = false; const std::string* nm = it.music ? shardNameOf(playerItemKey(it), foreign) : nullptr;
        const uint32_t col = playing ? HUD_AMBER : (sel ? HUD_WHITE : (it.heard ? HUD_CYAN : HUD_GREEN));
        if (sel) drawText(canvas, 6, y, ">", HUD_AMBER);
        drawRecordKindGlyph(canvas, 14, y, it.music ? RC_MUSIC : RC_TEXT, col);
        drawText(canvas, 24, y, trunc(playerItemTitle(it, nm), 23).c_str(), col);
        drawText(canvas, 168, y, trunc(it.people, 13).c_str(), playing ? HUD_AMBER : HUD_DIM);
        const std::string len = playing ? playerHudLine().substr(playerHudLine().rfind("  ") + 2) : (it.seconds > 0 ? mmss(it.seconds) : std::string());
        drawText(canvas, UW - 6 - textWidth(len.c_str()), y, len.c_str(), playing ? HUD_AMBER : HUD_DIM);
    }
    if (n > rows) {   // where the page is in the list
        const int h = 10 * rows, y0 = 18, bar = std::max(6, h * rows / n), off = (h - bar) * player.top / std::max(1, n - rows);
        fillRectRGB(canvas, UW - 3, y0, UW - 3, y0 + h - 1, HUD_DIM); fillRectRGB(canvas, UW - 3, y0 + off, UW - 2, y0 + off + bar, HUD_GREEN);
    }
    const PlayerItem& s = player.items[std::max(0, std::min(n - 1, player.sel))];
    std::string where = upper(worldNameOfKey(s.world)) + " - " + (s.heard ? "CAUGHT BY THE RADAR" : (s.medium >= 0 && s.medium < RM_COUNT ? RECORD_MEDIUM_HUD[s.medium] : "")) + fmt(" - YEAR %d", s.year);
    drawText(canvas, 6, UH - 20, trunc(where, 52).c_str(), HUD_DIM);
    drawTextCentered(canvas, UW / 2, UH - 10, "ENTER PLAY  SPACE PAUSE  BACKSPACE STOP  TAB WORLDS", HUD_DIM);
}

int Game::testPlayerItems() { playerBuildList(); return (int)player.items.size(); }
std::string Game::testPlayerItemLine(int i) const {
    if (i < 0 || i >= (int)player.items.size()) return std::string();
    const PlayerItem& it = player.items[i];
    bool foreign = false; const std::string* nm = it.music ? shardNameOf(playerItemKey(it), foreign) : nullptr;
    return fmt("%s %s%s, %s, year %d, %s%s", playerItemKey(it).c_str(), it.music ? "music" : "a recording", it.heard ? " off the air" : "", playerItemTitle(it, nm).c_str(), it.year, it.people.c_str(), it.seconds > 0 ? fmt(", %.1f s", it.seconds).c_str() : "");
}
bool Game::testPlayerPlay(int item) { if (player.items.empty()) playerBuildList(); if (item < 0 || item >= (int)player.items.size()) return false; playerPlay(item); return true; }
std::string Game::testPlayerInfo() const {
    if (player.playing < 0) return fmt("the player: nothing plays (%d items, %d played to their end)", (int)player.items.size(), player.finished);
    return fmt("the player: item %d '%s - %s', %.1f of %.1f s%s%s%s; the synth's piece %s, speech %s, medium %d; %d played to their end", player.playing, player.title.c_str(), player.people.c_str(), player.t, player.secs,
               player.paused ? ", paused" : "", player.held ? ", held" : "", player.waiting ? ", waiting for its world" : "",
               audio.piece == &player.world.piece ? "the player's" : (audio.piece ? "another's" : "none"), audio.speech == &player.world.speech ? "the player's" : (audio.speech ? "another's" : "none"), audio.medium, player.finished);
}
