// R-420 (2026-10-19): auto-explore's tape, the user's request for the 1.3.2 patch: "add these for to auto-explore mode as an optional feature
// - you can enable playlist with these 3 files with M button while in autoexplore and it will cycle. Use some 80s-90s cassete insertion
// sound between tracks. Investigate authorship properly so that when track starts author and name of the track are displayed for a few
// seconds on top left and then fade away. Also when music plays all other sounds are quiter".
//
// M while auto-explore plays (the controller's SELECT, whose own key would end it) starts and stops a tape: the explorer's `music` folder
// when it holds a track (`musicDir`), else the game's own beside the executable (`musicDirsGame`: four tracks, inside the app on macOS).
// A folder's tape is its playlist.txt's tracks (a line each: the file | the author | the title | a gain in dB evening their loudness),
// then its other sound files in the order of their names, an "Author - Title" name read as such. The tracks play round and round through
// a cassette deck (game/audio_deck.cpp): a cassette put in and played, at a tape's end the next cassette, the play key taking up the one
// left in where it was; each track's author and title at the top left as it begins, fading after a few seconds; the game's own sounds
// ducked under it. The platform streams the files (platform_raylib.cpp). When auto-explore ends the music fades with it, the choice and
// the place in the tape kept: the tape is taken up again as auto-explore next begins.
#include "game.h"
#include "ui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>

namespace {
enum TapePhase { TAPE_OFF = 0, TAPE_DECK, TAPE_PLAY, TAPE_FADE };
constexpr double TAPE_LEVEL = 0.8;   // the music's level under the master (a track's gain on it)
constexpr double TAPE_DUCK = 0.35;   // what the game's own sounds keep of their level under the music (about 9 dB down)
constexpr double CAP_IN = 0.4, CAP_HOLD = 5, CAP_OUT = 1.5;   // the caption's fade in, its time whole, its fade out (s)

std::string tapeTrim(const std::string& s) {
    const size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}
bool soundFile(const std::string& name) {
    const size_t d = name.rfind('.');
    if (d == std::string::npos) return false;
    std::string e = name.substr(d + 1);
    for (char& c : e) c = (char)tolower((unsigned char)c);
    return e == "mp3" || e == "ogg" || e == "wav" || e == "qoa";   // raylib's music streams (FLAC is not in its build)
}
bool fileThere(const std::string& path) { FILE* f = fopen(path.c_str(), "rb"); if (!f) return false; fclose(f); return true; }
std::string stemOf(const std::string& name) { const size_t d = name.rfind('.'); return d == std::string::npos ? name : name.substr(0, d); }
}

// a folder's tape: its playlist's tracks that are there, then the sound files it does not list, by name
void Game::tapeRead(const std::string& dir, std::vector<TapeTrack>& out) {
    std::vector<std::string> listed;
    if (FILE* f = fopen((dir + "/playlist.txt").c_str(), "r")) {
        char line[1024];
        while (fgets(line, sizeof line, f)) {
            const std::string s = tapeTrim(line);
            if (s.empty() || s[0] == '#') continue;
            std::vector<std::string> part;
            for (size_t a = 0;;) {
                const size_t b = s.find('|', a);
                part.push_back(tapeTrim(s.substr(a, b == std::string::npos ? std::string::npos : b - a)));
                if (b == std::string::npos) break;
                a = b + 1;
            }
            TapeTrack k;
            k.file = part[0];
            if (k.file.empty()) continue;
            listed.push_back(k.file);
            k.author = part.size() > 1 ? part[1] : "";
            k.title = part.size() > 2 && !part[2].empty() ? part[2] : stemOf(k.file);
            k.gainDb = part.size() > 3 ? clampd(atof(part[3].c_str()), -30, 20) : 0;
            if (!fileThere(dir + "/" + k.file)) { fprintf(stderr, "tape: %s/%s is not there\n", dir.c_str(), k.file.c_str()); continue; }
            out.push_back(k);
        }
        fclose(f);
    }
    std::vector<std::string> names;
    if (DIR* d = opendir(dir.c_str())) {
        while (dirent* e = readdir(d)) {
            const std::string n = e->d_name;
            if (!n.empty() && n[0] != '.' && soundFile(n) && std::find(listed.begin(), listed.end(), n) == listed.end()) names.push_back(n);
        }
        closedir(d);
    }
    std::sort(names.begin(), names.end());
    for (const std::string& n : names) {
        TapeTrack k; k.file = n;
        const std::string stem = stemOf(n);
        const size_t dash = stem.find(" - ");
        if (dash != std::string::npos) { k.author = tapeTrim(stem.substr(0, dash)); k.title = tapeTrim(stem.substr(dash + 3)); }
        else k.title = stem;
        out.push_back(k);
    }
}

// the tracks, read again: the explorer's folder, else the game's own
void Game::tapeList() {
    tape.tracks.clear(); tape.dir.clear();
    std::vector<std::string> dirs{musicDir};
    dirs.insert(dirs.end(), musicDirsGame.begin(), musicDirsGame.end());
    for (const std::string& d : dirs) {
        if (d.empty()) continue;
        tapeRead(d, tape.tracks);
        if (!tape.tracks.empty()) { tape.dir = d; return; }
    }
}

// beside the executable, in a macOS app's Resources, in a Vesperis.app beside a bare binary
void Game::tapeGameDirs(const std::string& appDir) {
    musicDirsGame.clear();
    if (appDir.empty()) return;
    const std::string a = appDir.back() == '/' || appDir.back() == '\\' ? appDir : appDir + "/";
    musicDirsGame = {a + "music", a + "../Resources/music", a + "Vesperis.app/Contents/Resources/music"};
}

// a deck sequence begun
void Game::tapeDeck(int kind) {
    tape.deckSeed++;
    audio.deck = kind;
    audio.deckSeed = tape.deckSeed * 2654435761u ^ (uint32_t)(realTime * 1000);
    tape.tapeAt = deckTapeAt(kind, audio.deckSeed);
    tape.t = 0;
}

// M: on (a cassette put in, or the one in played on) or off (the stop key)
void Game::tapeToggle() {
    if (tape.phase == TAPE_DECK || tape.phase == TAPE_PLAY) tapeHalt(true);
    else tapeStart(true);
}

// `asked`: by M (an empty folder is said), else auto-explore taking it up again
void Game::tapeStart(bool asked) {
    if (tape.phase == TAPE_FADE && audio.musicT >= 0 && !audio.music.empty() && !asked) {   // auto-explore back before its fade ended: the music comes back up
        tape.phase = TAPE_PLAY; tape.t = 1;
        return;
    }
    tapeList();
    if (tape.tracks.empty()) {
        if (asked) {   // nothing to play: said where a track's name would be
            tape.capAuthor = "NO MUSIC IN THE " + musicDir + " FOLDER";
            tape.capTitle = "MP3, OGG OR WAV FILES, OR A PLAYLIST.TXT";
            tape.captionT = 0; audio.beep = 3;
        }
        return;
    }
    const int n = (int)tape.tracks.size();
    tape.track = ((settings.autoTrack % n) + n) % n;
    const bool inDeck = settings.autoTrackAt >= 0;   // a cassette left in the deck: the play key takes it up where it was
    tape.from = inDeck ? settings.autoTrackAt : 0;
    settings.autoMusic = true; settings.autoTrack = tape.track; settings.autoTrackAt = tape.from;
    settings.save(settingsPath);
    tape.failed = 0; tape.gain = 0;
    tape.phase = TAPE_DECK;
    tapeDeck(inDeck ? DECK_PLAY : DECK_INSERT);
}

// the stop key (the choice off), or auto-explore's end (the music fading, the choice kept)
void Game::tapeHalt(bool stopKey) {
    if (tape.phase == TAPE_OFF) return;
    if (tape.phase == TAPE_PLAY && audio.musicT >= 0) settings.autoTrackAt = audio.musicT;   // the place kept: where it goes quiet
    if (stopKey) settings.autoMusic = false;
    settings.save(settingsPath);
    if (tape.captionT >= 0 && tape.captionT < CAP_IN + CAP_HOLD) tape.captionT = CAP_IN + CAP_HOLD;   // the caption fades out now
    const bool moving = tape.phase == TAPE_PLAY || (tape.phase == TAPE_FADE && tape.gain > 0);
    if (stopKey) tapeDeck(DECK_STOP);   // the music cut at the key's moment (`tapeAt`)
    else {                              // auto-explore's end: the music fading from now
        if (tape.phase == TAPE_DECK) audio.deck = -1;   // a cassette going in: the deck falls silent
        tape.t = 0; tape.tapeAt = 0;
    }
    tape.fadeLen = stopKey ? 0.04 : (tape.phase == TAPE_FADE ? std::min(tape.fadeLen, 1.5) : 1.5);   // a stop key's cut not slowed by auto-explore's end
    tape.phase = TAPE_FADE;
    if (!moving) tape.gain = 0;
}

// every frame: the deck's sequence, a track's end, the fades, the duck
void Game::tapeUpdate(double realDt) {
    TapeState& T = tape;
    if (T.captionT >= 0 && (T.captionT += realDt) > CAP_IN + CAP_HOLD + CAP_OUT) T.captionT = -1;
    if (T.phase == TAPE_OFF) { audio.duck = 1; return; }
    if (T.tracks.empty()) { T.phase = TAPE_OFF; audio.duck = 1; return; }
    const int n = (int)T.tracks.size();
    T.track = std::min(T.track, n - 1);
    T.t += realDt;
    auto nextCassette = [&]() {   // the tape's end (or a cassette that would not play): the next one in
        T.track = (T.track + 1) % n; T.from = 0;
        settings.autoTrack = T.track; settings.autoTrackAt = 0; settings.save(settingsPath);
        T.phase = TAPE_DECK; tapeDeck(DECK_CHANGE);
    };
    switch (T.phase) {
        case TAPE_DECK:
            if (T.t >= T.tapeAt) {   // the tape moves: the track begins, its credit shown
                const TapeTrack& k = T.tracks[T.track];
                audio.music = T.dir + "/" + k.file; audio.musicFrom = T.from; audio.musicGo = true;
                audio.musicEnded = audio.musicFailed = false; audio.musicT = -1;
                T.phase = TAPE_PLAY; T.t = 0; T.gain = 1;
                T.capAuthor = k.author; T.capTitle = k.title; T.captionT = 0;
            }
            break;
        case TAPE_PLAY:
            T.gain = std::min(1.0, T.gain + realDt);   // back up after a fade (auto-explore begun again)
            if (audio.musicFailed) {
                audio.musicFailed = false;
                fprintf(stderr, "tape: %s/%s would not play\n", T.dir.c_str(), T.tracks[T.track].file.c_str());
                if (++T.failed >= n) {
                    T.phase = TAPE_OFF; audio.duck = 1; settings.autoMusic = false; settings.save(settingsPath);
                    T.capAuthor = "THE TAPE WOULD NOT PLAY"; T.capTitle = "NONE OF ITS FILES COULD BE READ"; T.captionT = 0; audio.beep = 3;
                    return;
                }
                nextCassette();
            } else if (audio.musicEnded) {
                audio.musicEnded = false;
                T.failed = 0;
                nextCassette();
            } else if (audio.musicT >= 0) settings.autoTrackAt = audio.musicT;   // saved with the settings at the game's end
            break;
        case TAPE_FADE:
            if (T.t >= T.tapeAt) T.gain = std::max(0.0, T.gain - realDt / T.fadeLen);
            if (T.gain <= 0) { audio.music.clear(); audio.musicGo = true; audio.musicT = -1; T.phase = TAPE_OFF; audio.duck = 1; return; }
            break;
    }
    const double trackGain = std::pow(10.0, T.tracks[T.track].gainDb / 20);
    audio.musicGain = TAPE_LEVEL * trackGain * T.gain;
    audio.duck = T.phase == TAPE_FADE ? 1 - (1 - TAPE_DUCK) * T.gain : TAPE_DUCK;
}

// the caption at the top left
// the track as it begins: its author and its title at the top left, fading in, held, fading out
void Game::tapeRender() {
    std::string author, title;
    const double a = tapeCaption(author, title);
    const int A = (int)std::lround(255 * a);
    if (A <= 0) return;
    int y = 6;
    for (const std::string* s : {&author, &title}) {
        if (s->empty()) continue;
        const std::string line = trunc(upper(*s), 50);
        drawTextBlend(canvas, 7, y + 1, line.c_str(), HUD_SHADOW, A * 3 / 4);
        drawTextBlend(canvas, 6, y, line.c_str(), s == &author ? HUD_AMBER : HUD_WHITE, A);
        y += FONT_LINE;
    }
}

// the caption's alpha now (0 none) and its words
double Game::tapeCaption(std::string& author, std::string& title) const {
    author = tape.capAuthor; title = tape.capTitle;
    const double c = tape.captionT;
    if (c < 0) return 0;
    return c < CAP_IN ? c / CAP_IN : (c < CAP_IN + CAP_HOLD ? 1.0 : std::max(0.0, 1 - (c - CAP_IN - CAP_HOLD) / CAP_OUT));
}
