// C-11 (2026-10-10): the ship's player (KI-347). C-04's pieces played only on the decoder's screen; with flights of a minute and a half, the
// ship plays them on. R-431 (2026-11-18): its own screen at the deck in the library (`PLAYER`; the decoder's page and its Tab no more), the
// worlds' music alone: every piece held and every piece the radar caught (`Guide::heard`: as the ship kept it off the air), a folder a world,
// a world's pieces by their peoples and years; Enter plays one and the list on from it, Space pauses, Backspace stops. It plays wherever the
// ship goes (the cabin, the window, the maps, a Vimana flight: a line on the space HUD) and stops at a landing. The synth's piece points into
// the player's own copy of the world (`ShipPlayer::world`), so nothing the decoder opens moves it; while the decoder plays a record of its
// own, the radar listens or the gramophone turns, the player is held and takes up where it was (`AudioState::pieceFrom`).
#include "game.h"
#include "ui.h"
#include "peoples/records.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>

namespace {
int bodyOfKeyP(const std::string& key) { size_t sl = key.find('/'); return sl == std::string::npos ? -1 : atoi(key.c_str() + sl + 1); }
std::string mmss(double s) { const int t = (int)std::floor(std::max(0.0, s)); return fmt("%d:%02d", t / 60, t % 60); }
int playerGuideVersion(const Guide& g) { return (int)(g.shards.size() * 7919 + g.lent.size() * 104729 + g.heard.size()); }   // the list is rebuilt when it moves
constexpr int PLAYER_ROWS = 15, PLAYER_WORLD_ROWS = 8;   // R-431: a world's pieces a page (a line each), the worlds a page (two lines each)
}
// its guide key ("<body>/S<index>")
std::string Game::playerItemKey(const PlayerItem& it) { return it.world + "/S" + std::to_string(shardWorldIndex(it.which, it.local)); }

std::string Game::playerHudLine() const {
    if (player.playing < 0 || player.playing >= (int)player.items.size()) return std::string();
    const std::string when = player.waiting ? std::string("...") : (player.paused ? std::string("PAUSED") : mmss(player.t) + (player.secs > 0 ? "/" + mmss(player.secs) : std::string()));
    const int room = 36 - (int)when.size() - 2;
    const std::string whole = player.title + " - " + player.people;
    return ((int)whole.size() <= room ? whole : ((int)player.title.size() <= room ? player.title : trunc(player.title, room))) + "  " + when;   // the people left out before the title is cut
}

// the synth's piece is the player's
bool Game::playerOwns() const { return audio.piece && audio.piece == &player.world.piece; }

// the list: the pieces the guide holds and those the radar caught, a world's by their peoples and years; each world's first (its folder)
void Game::playerBuildList() {
    const std::string playingKey = player.playing >= 0 && player.playing < (int)player.items.size() ? playerItemKey(player.items[player.playing]) : std::string();
    struct W { bool ok = false; StarSystem sys; int bi = -1; BodyGen g; Lore lore[2]; Tradition T[2]; bool tOk[2] = {false, false}; std::shared_ptr<const RecordCatalogue> C[2]; };
    std::map<std::string, W> worlds;
    auto worldOf = [&](const std::string& key) -> W& {
        auto it = worlds.find(key);
        if (it != worlds.end()) return it->second;
        W& w = worlds[key];
        int64_t sx, sy, sz; Star st;
        if (!Guide::parseStarKey(key, sx, sy, sz) || !placeInSector(sx, sy, sz, st, true)) return w;
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
        if (e.category != RC_MUSIC) continue;   // R-431: music alone (a text, a recording, a chart: the decoder's)
        const bool asHeld = held.count(k) > 0;
        PlayerItem it; it.world = wk; it.which = which; it.local = local; it.heard = !asHeld; it.medium = asHeld ? e.medium : MEDIUM_AIR; it.year = e.year;
        it.people = "THE " + upper(w.lore[which].people);
        auto pinfo = player.pieceInfo.find(k);   // its form and its length, from its draws without the people's texts (once a piece)
        if (pinfo == player.pieceInfo.end()) {
            if (!w.tOk[which]) { w.T[which] = traditionOf(w.g, w.lore[which]); w.tOk[which] = true; }
            Shard sh = shardDrawOf(w.g, w.lore[which], local); sh.recordAge = e.age; sh.medium = e.medium;
            Piece pc; pieceOf(w.T[which], sh, pc);
            pinfo = player.pieceInfo.emplace(k, std::make_pair(pc.form, pc.seconds)).first;
        }
        it.form = pinfo->second.first; it.seconds = pinfo->second.second;
        items.push_back(it);
    }
    std::sort(items.begin(), items.end(), [](const PlayerItem& a, const PlayerItem& b) {
        if (a.world != b.world) return a.world < b.world;
        if (a.which != b.which) return a.which < b.which;
        if (a.year != b.year) return a.year < b.year;
        return a.local < b.local;
    });
    player.items = items;
    player.worldFirst.clear();
    for (size_t i = 0; i < player.items.size(); i++) if (i == 0 || player.items[i].world != player.items[i - 1].world) player.worldFirst.push_back((int)i);
    player.playing = -1;
    for (size_t i = 0; i < player.items.size() && !playingKey.empty(); i++) if (playerItemKey(player.items[i]) == playingKey) player.playing = (int)i;
    if (!playingKey.empty() && player.playing < 0) playerStop();   // what played is no longer held (a lent shard taken back)
    player.sel = std::max(0, std::min(player.sel, (int)player.items.size() - 1));
    player.worldSel = std::max(0, std::min(player.worldSel, (int)player.worldFirst.size() - 1));
    player.guideVersion = playerGuideVersion(guide);
}

// "A LULLABY", "'THE RIVER'", "AIR: A HYMN"
std::string Game::playerItemTitle(const PlayerItem& it, const std::string* name) {
    const std::string t = name ? "'" + upper(*name) + "'" : std::string(PIECE_FORM_NAMES[std::max(0, std::min(PF_COUNT - 1, it.form))]);
    return it.heard ? "AIR: " + t : t;
}

// from its start
void Game::playerPlay(int i) {
    if (i < 0 || i >= (int)player.items.size()) return;
    if (playerOwns()) { audio.piece = nullptr; audio.tradition = nullptr; audio.medium = -1; }
    player.playing = i; player.t = 0; player.syncT = -1; player.paused = false; player.held = false; player.waiting = true; player.secs = player.items[i].seconds;
    const PlayerItem& it = player.items[i];
    bool foreign = false; const std::string* nm = shardNameOf(playerItemKey(it), foreign);
    player.title = playerItemTitle(it, nm); player.people = it.people;
    playerStart();
}

// the playing item onto the synth from `player.t`: its world built (on a thread the first time: `waiting` until its records are written), the
// piece made again (deterministic), the synth told where to begin
void Game::playerStart() {
    if (player.playing < 0 || player.playing >= (int)player.items.size()) return;
    PlayerItem& it = player.items[player.playing];
    ShardWorld& w = player.world;
    if (!(w.valid && w.key == it.world && w.which == it.which)) {
        if (playerOwns()) { audio.piece = nullptr; audio.tradition = nullptr; audio.medium = -1; }   // before the world it points into goes
        int64_t sx, sy, sz; Star st;
        if (!Guide::parseStarKey(it.world, sx, sy, sz) || !placeInSector(sx, sy, sz, st, true)) { playerStop(); return; }
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
    pieceOf(w.tradition, sh, w.piece);
    player.secs = it.seconds = w.piece.seconds;
    audio.piece = &w.piece; audio.tradition = &w.tradition; audio.pieceSpeed = 1;
    audio.pieceFrom = player.t > 0.05 ? player.t * w.piece.bpm / 60.0 : 0;
    audio.pieceStart = true; audio.piecePause = player.paused; audio.pieceDone = false; audio.pieceBeat = -1;
    player.syncT = -1;
    if (player.playing + 1 < (int)player.items.size()) {   // the next one's world written meanwhile, so the list moves on without a wait
        const PlayerItem& nx = player.items[player.playing + 1];
        int64_t sx, sy, sz; Star st;
        if ((nx.world != it.world || nx.which != it.which) && Guide::parseStarKey(nx.world, sx, sy, sz) && placeInSector(sx, sy, sz, st, true)) {
            StarSystem wsys; wsys.generate(st); const int bi = bodyOfKeyP(nx.world);
            if (bi >= 0 && bi < (int)wsys.bodies.size()) {
                const BodyGen g = BodyGen::make(wsys.bodies[bi]);
                shardTextsPrefetch(wsys, wsys.bodies[bi], g, loreQuick(wsys, wsys.bodies[bi], g, nx.which));
            }
        }
    }
}

void Game::playerStop() {
    if (playerOwns()) { audio.piece = nullptr; audio.tradition = nullptr; audio.pieceStart = false; audio.piecePause = false; audio.pieceBeat = -1; audio.medium = -1; }
    player.playing = -1; player.paused = false; player.held = false; player.waiting = false; player.t = 0; player.secs = 0; player.syncT = -1;
}

// every frame: the landing stops it, another's sound holds it, its end moves it on
void Game::updatePlayer(double realDt) {
    if (player.playing < 0) return;
    if (stateHas(state, ST_GROUND) || state == GameState::TITLE) { playerStop(); return; }   // a landing stops it
    const bool mine = playerOwns();
    if (radar.on) {   // the receiver listens: the player holds its place
        if (mine) { audio.piece = nullptr; audio.tradition = nullptr; audio.medium = -1; }
        player.held = true; return;
    }
    if ((audio.piece && !mine) || audio.speech || audio.radio) {   // the decoder's own record, the gramophone: theirs while they play, one sound at a time
        if (mine) { audio.piece = nullptr; audio.tradition = nullptr; }
        player.held = true; return;
    }
    if (player.held || player.waiting || !mine) { player.held = false; playerStart(); return; }   // taken up where it was
    if (player.paused) return;
    // the clock: the synth's while it runs (the main thread reads its report after each block), else the game's (the harness runs no synth)
    if (audio.pieceBeat >= 0 && audio.pieceBeat != player.syncT) { player.syncT = audio.pieceBeat; player.t = audio.pieceBeat * 60.0 / std::max(1.0, player.world.piece.bpm); }
    else player.t += realDt;
    const bool done = (audio.pieceDone && player.t > 0.5) || (player.secs > 0 && player.t >= player.secs + 1.5);
    if (!done) return;
    player.finished++;
    const int next = player.playing + 1;
    if (next < (int)player.items.size()) playerPlay(next);   // the list on
    else playerStop();
}

// R-431: the player's screen at the deck. It opens on its worlds, a folder each, on the world playing (else the one under the ship); Enter
// opens a world's pieces (on the one playing in it, else its first), Enter there plays one and the list on from it; Space pauses and
// Backspace stops at either; Esc goes back to the worlds and from them closes (the player plays on)
void Game::playerEnter() {
    radarOff();   // the player takes the synth
    playerBuildList();
    player.level = 0;
    std::string open;
    if (player.playing >= 0 && player.playing < (int)player.items.size()) open = player.items[player.playing].world;
    else if (sys.valid && ship.parkedBody >= 0) open = Guide::bodyKey(sys.star.sx, sys.star.sy, sys.star.sz, ship.parkedBody);
    for (size_t w = 0; w < player.worldFirst.size() && !open.empty(); w++) if (player.items[player.worldFirst[w]].world == open) player.worldSel = (int)w;
    returnState = GameState::SPACE;
    audio.beep = BEEP_UI;
}

void Game::updatePlayerScreen(const Input& in, double dt, double realDt) {
    (void)dt; (void)realDt;
    if (player.guideVersion != playerGuideVersion(guide)) {   // C-18's drones brought more, or the radar caught one: the list again, the cursor kept
        const std::string selKey = player.sel >= 0 && player.sel < (int)player.items.size() ? playerItemKey(player.items[player.sel]) : std::string();
        const std::string worldKey = player.worldSel >= 0 && player.worldSel < (int)player.worldFirst.size() ? player.items[player.worldFirst[player.worldSel]].world : std::string();
        playerBuildList();
        for (size_t w = 0; w < player.worldFirst.size(); w++) if (player.items[player.worldFirst[w]].world == worldKey) player.worldSel = (int)w;
        for (size_t i = 0; i < player.items.size() && !selKey.empty(); i++) if (playerItemKey(player.items[i]) == selKey) { player.sel = (int)i; break; }
    }
    const int nw = (int)player.worldFirst.size();
    if (in.wasPressed(KEY_ESCAPE)) {
        if (player.level == 1) { player.level = 0; audio.beep = BEEP_UI; } else closeScreen();   // the worlds, then away (it plays on)
        return;
    }
    if (nw == 0) return;
    if (in.wasPressed(KEY_SPACE) && player.playing >= 0) {
        player.paused = !player.paused;
        if (playerOwns()) audio.piecePause = player.paused;
    }
    if (in.wasPressed(KEY_BACKSPACE) || in.wasPressed(KEY_DELETE)) { playerStop(); audio.beep = BEEP_UI; }
    if (player.level == 0) {
        menuStep(in, player.worldSel, nw);
        if (in.wasPressed(KEY_PAGE_UP)) player.worldSel = std::max(0, player.worldSel - PLAYER_WORLD_ROWS);
        if (in.wasPressed(KEY_PAGE_DOWN)) player.worldSel = std::min(nw - 1, player.worldSel + PLAYER_WORLD_ROWS);
        if (enterKey(in)) {
            int first, end; playerWorldRange(player.worldSel, first, end);
            player.sel = player.playing >= first && player.playing < end ? player.playing : first;
            player.level = 1; audio.beep = BEEP_UI;
        }
        return;
    }
    int first, end; playerWorldRange(player.worldSel, first, end);
    int at = std::max(0, std::min(end - first - 1, player.sel - first));
    menuStep(in, at, end - first);
    if (in.wasPressed(KEY_PAGE_UP)) at = std::max(0, at - PLAYER_ROWS);
    if (in.wasPressed(KEY_PAGE_DOWN)) at = std::min(end - first - 1, at + PLAYER_ROWS);
    player.sel = first + at;
    if (enterKey(in)) { playerPlay(player.sel); audio.beep = BEEP_UI; }
}

// a world's pieces in the list: from its first to the next world's
void Game::playerWorldRange(int w, int& first, int& end) const {
    const int nw = (int)player.worldFirst.size();
    first = w >= 0 && w < nw ? player.worldFirst[w] : 0;
    end = w >= 0 && w + 1 < nw ? player.worldFirst[w + 1] : (int)player.items.size();
}

// the worlds: each its name and where it is, how many pieces it holds and its peoples; the one playing marked. A world's pieces: the list,
// the one playing with its clock; under them what the chosen one was made on (or the radar's) and its year
void Game::renderPlayerScreen() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 225);
    const int nw = (int)player.worldFirst.size(), n = (int)player.items.size();
    const int playingWorld = [&] { for (int w = nw - 1; w >= 0; w--) if (player.playing >= player.worldFirst[w]) return w; return -1; }();
    if (n == 0) {
        drawText(canvas, 6, 4, "THE PLAYER", HUD_AMBER);
        drawTextCentered(canvas, UW / 2, 80, "NOTHING TO PLAY YET", HUD_GREEN);
        drawTextCentered(canvas, UW / 2, 92, "THE OLD PEOPLES' MUSIC, TAKEN FROM THEIR RUINS", HUD_DIM);
        drawTextCentered(canvas, UW / 2, 101, "OR CAUGHT BY THE RADAR, PLAYS HERE", HUD_DIM);
        drawTextCentered(canvas, UW / 2, UH - 10, "ESC CLOSE", HUD_DIM);
        return;
    }
    if (player.level == 0) {
        drawText(canvas, 6, 4, "THE PLAYER", HUD_AMBER);
        const int page = player.worldSel / PLAYER_WORLD_ROWS, pages = (nw + PLAYER_WORLD_ROWS - 1) / PLAYER_WORLD_ROWS;
        const std::string head = pages > 1 ? fmt("%d PIECES  PAGE %d/%d", n, page + 1, pages) : fmt("%d PIECE%s FROM %d WORLD%s", n, n == 1 ? "" : "S", nw, nw == 1 ? "" : "S");
        drawText(canvas, UW - 6 - textWidth(head.c_str()), 4, head.c_str(), HUD_DIM);
        for (int w = page * PLAYER_WORLD_ROWS; w < nw && w < (page + 1) * PLAYER_WORLD_ROWS; w++) {
            const int y = 18 + (w - page * PLAYER_WORLD_ROWS) * 19;
            int first, end; playerWorldRange(w, first, end);
            const std::string& key = player.items[first].world;
            const bool sel = w == player.worldSel, playing = w == playingWorld;
            int64_t sx = 0, sy = 0, sz = 0; Guide::parseStarKey(key, sx, sy, sz);
            if (sel) drawText(canvas, 6, y, ">", HUD_AMBER);
            const std::string where = fmt("  (%+lld %+lld %+lld / %d)", (long long)sx, (long long)sy, (long long)sz, bodyOfKeyP(key));
            drawText(canvas, 16, y, (trunc(upper(worldNameOfKey(key)), std::max(8, 49 - (int)where.size())) + where).c_str(),
                     playing ? HUD_AMBER : (sel ? HUD_WHITE : (nameIsForeign(key) ? HUD_CYAN : HUD_GREEN)));
            std::string row = fmt("%d PIECE%s", end - first, end - first == 1 ? "" : "S");
            for (int i = first; i < end; i++) if (i == first || player.items[i].which != player.items[i - 1].which) row += "   " + player.items[i].people;   // its peoples
            if (playing) row = trunc(row, 30) + "   PLAYING";
            drawText(canvas, 16, y + 9, trunc(row, 50).c_str(), playing ? HUD_AMBER : HUD_DIM);
        }
        drawTextCentered(canvas, UW / 2, UH - 10, player.playing >= 0 ? "ENTER OPEN  SPACE PAUSE  BACKSPACE STOP  ESC CLOSE" : "UP/DOWN  ENTER OPENS A WORLD  ESC CLOSE", HUD_DIM);
        return;
    }
    int first, end; playerWorldRange(player.worldSel, first, end);
    const int count = end - first, rows = PLAYER_ROWS;
    drawText(canvas, 6, 4, ("THE PLAYER - " + trunc(upper(worldNameOfKey(player.items[first].world)), 24)).c_str(), HUD_AMBER);
    const std::string head = fmt("%d PIECE%s", count, count == 1 ? "" : "S");
    drawText(canvas, UW - 6 - textWidth(head.c_str()), 4, head.c_str(), HUD_DIM);
    if (player.sel < player.top) player.top = player.sel;
    if (player.sel >= player.top + rows) player.top = player.sel - rows + 1;
    player.top = std::max(first, std::min(player.top, std::max(first, end - rows)));
    for (int i = player.top; i < end && i < player.top + rows; i++) {
        const PlayerItem& it = player.items[i];
        const int y = 18 + (i - player.top) * 10;
        const bool sel = i == player.sel, playing = i == player.playing;
        bool foreign = false; const std::string* nm = shardNameOf(playerItemKey(it), foreign);
        const uint32_t col = playing ? HUD_AMBER : (sel ? HUD_WHITE : (it.heard ? HUD_CYAN : HUD_GREEN));
        if (sel) drawText(canvas, 6, y, ">", HUD_AMBER);
        drawRecordKindGlyph(canvas, 14, y, RC_MUSIC, col);
        drawText(canvas, 24, y, trunc(playerItemTitle(it, nm), 23).c_str(), col);
        drawText(canvas, 168, y, trunc(it.people, 13).c_str(), playing ? HUD_AMBER : HUD_DIM);
        const std::string len = playing ? playerHudLine().substr(playerHudLine().rfind("  ") + 2) : (it.seconds > 0 ? mmss(it.seconds) : std::string());
        drawText(canvas, UW - 6 - textWidth(len.c_str()), y, len.c_str(), playing ? HUD_AMBER : HUD_DIM);
    }
    if (count > rows) {   // where the page is in the world's list
        const int h = 10 * rows, y0 = 18, bar = std::max(6, h * rows / count), off = (h - bar) * (player.top - first) / std::max(1, count - rows);
        fillRectRGB(canvas, UW - 3, y0, UW - 3, y0 + h - 1, HUD_DIM); fillRectRGB(canvas, UW - 3, y0 + off, UW - 2, y0 + off + bar, HUD_GREEN);
    }
    const PlayerItem& s = player.items[std::max(first, std::min(end - 1, player.sel))];
    const std::string where = (s.heard ? std::string("CAUGHT BY THE RADAR") : (s.medium >= 0 && s.medium < RM_COUNT ? std::string(RECORD_MEDIUM_HUD[s.medium]) : std::string())) +
                              fmt(" - YEAR %d", s.year);
    drawText(canvas, 6, UH - 20, trunc(where, 52).c_str(), HUD_DIM);
    drawTextCentered(canvas, UW / 2, UH - 10, "ENTER PLAY  SPACE PAUSE  BACKSPACE STOP  ESC BACK", HUD_DIM);   // 53 characters fill the screen
}

// U-04: the player's commands (the controller's first page), its level's
void Game::playerPad(PadMenu& m) const {
    m.title = "THE PLAYER";
    if (player.level == 0 && !player.worldFirst.empty()) m.item("OPEN THE WORLD", KEY_ENTER);
    if (player.level == 1) m.item("PLAY FROM HERE", KEY_ENTER);
    if (player.playing >= 0) { m.item(player.paused ? "GO ON" : "PAUSE", KEY_SPACE); m.item("STOP", KEY_BACKSPACE); }
    m.item(player.level == 1 ? "BACK TO THE WORLDS" : "CLOSE THE PLAYER", KEY_ESCAPE);
}
