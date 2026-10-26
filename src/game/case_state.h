// The instruments' case (game/instruments_screen.cpp): its worlds, the instrument on the screen, the notes played, the disc
#pragma once
#include "peoples/forms.h"
#include "peoples/instruments.h"
#include "peoples/music.h"
#include <string>
#include <vector>

struct InstrumentCase {
    // C-18: the case opens on its worlds, a folder each as the decoder's (the user's: "you open specific planet as a folder"), and a
    // world's instruments inside it: Enter opens the world, Esc closes it. A world of two peoples a folder each people, as the decoder
    // lists them (the user's: "split instruments by civilization as a decoder does")
    int level = 0;                                 // 0 the worlds, 1 the open world's instruments
    std::vector<std::string> worlds;               // each folder's world ("<body key>"), the one under the ship selected
    std::vector<int> worldWhich;                   // and its people (0, or 1 the second of a world of two)
    std::vector<int> worldCounts;                  // how many of each the case holds
    std::vector<std::string> worldPeoples;         // whose they are on a world of two ("THE GREIP"), else ""
    int worldSel = 0;
    std::vector<std::string> keys; int sel = 0;    // the open world's instruments: the guide's keys, by id
    std::vector<std::string> names;                // each one's name ("A LUTE")
    std::string key; bool valid = false;           // the one on the screen
    Tradition tradition; PeopleInstruments set; Instrument inst; FormPalette palette; std::string people;
    int regShift = 0;
    bool held[10] = {};                            // its keys down (1..9, 0)
    std::vector<double> glow;                      // per course of the picture: a note just played, fading
    struct Played { int degree = 0; double t = 0; };
    std::vector<Played> played;                    // the last notes (the roll under the picture)
    int notes = 0;                                 // the notes played since the case opened (the harness)
    Piece disc; bool discOn = false; double discT = -1;   // the gramophone's disc playing (its clock, the game's own)
    double humBefore = -1;
};
