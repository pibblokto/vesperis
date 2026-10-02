#pragma once
// C-04 (2026-10-02): a people's music. A world's tradition is drawn from the body's seed (`traditionOf`): a scale of five to
// nine notes from a tuning of its own (an equal division of the octave into 5..24 steps, or just ratios; rarely a stretched
// octave or the twelfth), a rhythmic cycle of beats in groups of two and three, a tempo that wanders, two or three timbres
// drawn from the style of the people's names, an ornament rule, a second voice, a drone, a drum. A shard that is a piece of
// music (`Shard::music`, twelve to eighteen of the fifty) is a phrase structure over that tradition from the shard's seed (`pieceOf`):
// a statement, its variation, an answer, the return, a close; its form (a dance, a lullaby, a hymn, a lament, the last
// song...) by the shard's tone. The notes are beats and scale degrees; `AudioSynth` plays them (`game/audio.h`) and the
// decoder screen draws them (`game/shards_screen.cpp`). After two pieces of a world the listener knows the tradition.
#include "shards.h"
#include <string>
#include <vector>

enum TimbreFamily { TF_PLUCKED = 0, TF_PIPE, TF_BOWED, TF_METAL, TF_REED, TF_DRUM, TF_COUNT };
extern const char* const TIMBRE_FAMILY_NAMES[TF_COUNT];   // "A PLUCKED STRING", "A PIPE", "A BOWED STRING", "A BELL", "A REED", "A DRUM"

struct Timbre {
    int family = TF_PLUCKED;
    double ratio[6] = {1, 2, 3, 4, 5, 6};                  // the partials' frequency ratios (a bell's stretched; the drum's two skins in Hz)
    double amp[6] = {1, 0.5, 0.33, 0.25, 0.2, 0.17};       // their amplitudes
    double attack = 0.01, decay = 0.5, sustain = 0, release = 0.1;   // seconds, seconds, level, seconds
    double vibRate = 0, vibDepth = 0;                      // vibrato: Hz and the share of the pitch
    double breath = 0;                                     // a noise component (a pipe's breath, a drum's skin)
    double gain = 0.3;
};

struct Tradition {
    std::vector<double> scale;   // the degrees as ratios to the tonic within one period, the tonic (1.0) first, ascending; five to nine
    double period = 2.0;         // the ratio at which the scale repeats: the octave; a stretched or a narrow one; rarely the twelfth (3.0)
    double base = 220;           // the tonic, Hz
    int tuning = 0;              // 0 an equal division of the period, 1 just ratios
    int division = 12;           // the equal division's steps (tuning 0)
    std::vector<int> groups;     // the cycle: beats in groups of two, three and (rarely) four
    int beats = 7;               // their sum
    int sub = 2;                 // a beat's subdivision (two or three)
    double bpm = 100;            // the tradition's beat
    double drift = 0.03;         // the tempo's slow wander (a share)
    std::vector<Timbre> voices;  // two or three melodic timbres, the first the lead
    Timbre drum;
    bool hasDrum = true, drone = false;
    int second = 0;              // the second voice: 0 none, 1 in parallel below the lead, 2 an answer a phrase later, 3 doubling a period below
    int ornament = 0;            // 0 none, 1 grace notes before a leap, 2 trills on long notes, 3 slides between neighbours, 4 mordents
    double ornamentChance = 0.3;
    int rest = 0;                // the degree the phrases rest on besides the tonic (the one nearest a fifth, else a fourth)
    uint64_t seed = 0;
};
Tradition traditionOf(const BodyGen& g, const Lore& L);
double traditionHz(const Tradition& T, int degree);      // a degree's pitch (degrees past the scale wrap by the period, below zero the period before)
double traditionCents(const Tradition& T, int degree);   // its cents above the tonic

enum PieceForm { PF_DANCE = 0, PF_LULLABY, PF_WORK, PF_HYMN, PF_HARVEST, PF_LOVE, PF_FESTIVAL, PF_LAMENT, PF_MARCH, PF_WARNING, PF_LAST, PF_COUNT };
extern const char* const PIECE_FORM_NAMES[PF_COUNT];   // "A DANCE", "A LULLABY", ... "THE LAST SONG"

struct Note {
    double t = 0, dur = 0;   // beats from the piece's start, length in beats
    int voice = 0;           // index into the tradition's voices; -1 the drum
    int degree = 0;          // the scale degree (0 the tonic, the scale's size the next period); the drum: 0 the low skin, 1 the high
    double vel = 0.7;
    int slideFrom = -1000;   // a slide: the degree the note glides from over its start
};
struct Piece {
    int form = 0;
    double bpm = 100;
    int cycles = 0;              // its length in cycles
    double beats = 0;            // and in beats
    double seconds = 0;          // at its tempo
    std::vector<Note> notes;     // by their start
    uint64_t seed = 0;
};
void pieceOf(const Tradition& T, const Shard& s, Piece& out);
bool pieceHasDrum(const Piece& P);
std::string traditionLine(const Tradition& T);                 // "A SCALE OF 7 NOTES TO THE OCTAVE"
std::string cycleLine(const Tradition& T, double bpm);         // "A CYCLE OF 2+2+3 AT 112 BEATS A MINUTE"
std::string timbresLine(const Tradition& T, bool drum);        // "A PLUCKED STRING, A PIPE AND A DRUM" (and the drone, when the line has room)
