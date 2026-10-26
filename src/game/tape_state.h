// Auto-explore's tape (game/tape.cpp): the tracks and the deck's phase, the fades, the caption
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct TapeTrack { std::string file, author, title; double gainDb = 0; };
struct TapeState {
    std::vector<TapeTrack> tracks; std::string dir;   // and the folder they were read from
    int phase = 0;                    // TAPE_OFF, TAPE_DECK (the deck's sequence before the tape moves), TAPE_PLAY, TAPE_FADE (going quiet)
    double t = 0, tapeAt = 0;         // seconds in the phase; when the deck's sequence moves (or stops) the tape
    int track = 0; double from = 0;   // the track in the deck, the second it begins at
    double gain = 0, fadeLen = 1;     // the music's level (0..1 of the track's), a fade's length
    double captionT = -1; std::string capAuthor, capTitle;   // the caption's clock (-1 none) and its words
    int failed = 0; uint32_t deckSeed = 0;   // tracks in a row that would not play; the deck's sequences begun
};
