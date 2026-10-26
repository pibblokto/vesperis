#pragma once
// C-04 (2026-10-02): a people's music. A world's tradition is drawn from the body's seed (`traditionOf`): a scale of five to
// nine notes from a tuning of its own (an equal division of the octave into 5..24 steps, or just ratios; rarely a stretched
// octave or the twelfth), a rhythmic cycle of beats in groups of two and three, a tempo that wanders, two or three timbres
// drawn from the style of the people's names, an ornament rule, a second voice, a drone, a drum. A shard that is a piece of
// music (`Shard::music`, twelve to eighteen of the fifty) is a phrase structure over that tradition from the shard's seed (`pieceOf`):
// a statement, its variation, an answer, the return, a close; its form (a dance, a lullaby, a hymn, a lament, the last
// song...) by the shard's tone. The notes are beats and scale degrees; `AudioSynth` plays them (`game/audio.h`) and the
// decoder screen draws them (`game/shards_screen.cpp`). After two pieces of a world the listener knows the tradition.
//
// H-05 (2026-10-10): the tradition gains the age as an axis. What a people heard as its own holds through its ages (the scale and
// its tuning, the tonic, the cycle, the ornament, the swing of its subdivisions, the singers' voice: its speakers', `voiceOf`), and
// each age it lived through brings its own ensemble (`Ensemble`, `Tradition::ages`), forms and texture, chosen by the age of the
// record's year (`Shard::recordAge`: a modern people's medieval hymn is medieval):
//
// * prehistoric: voices (a caller and a chorus that answers, singing the vowels of the people's tongue), a bone flute, a musical bow,
//   a bullroarer; skin and log drums, rattles, sticks and claps; chants in cycles, a ring dance that quickens, a keening that slides;
// * medieval: C-04's strings, pipes, reeds and bells, a drone, a choir; modes, hymns in organum, dances over a tabor, laments;
// * industrial: a brass band, strings and keys, an orchestra or a barrel organ, an organ for hymns, a music box; homophony (a melody
//   over chords stacked from the people's own scale and a bass): marches, turning dances, anthems, parlour songs, a music box that
//   winds down;
// * modern: drum machines or a kit, electric basses and synth basses, electric keys, pads and electric strings, a singer or a synth
//   lead; songs with verses and choruses, dance records that build and break, ballads, jingles, a broadcast's theme;
// * futuristic: algorithmic processes (a cell phasing against itself, a line built note by note), microtonal swarms gliding, a
//   synthetic voice over an arpeggiator; clean and exact;
// * the far future: a field of sound rather than a piece (long tones of the scale across several periods, coming and going, beating
//   slowly; a rare glint).
#include "lore.h"
#include "peoples.h"
#include <string>
#include <vector>
struct Shard;   // shards.h

// the timbre families: C-04's five and its drum, then the ages' own (H-05), then the percussion's
enum TimbreFamily {
    TF_PLUCKED = 0, TF_PIPE, TF_BOWED, TF_METAL, TF_REED, TF_DRUM,
    TF_VOICE, TF_BRASS, TF_KEYS, TF_ORGAN, TF_MUSICBOX, TF_ESTRING, TF_EKEYS, TF_SYNTH, TF_PAD, TF_SWARM, TF_SYNVOICE, TF_FIELD, TF_ROARER,
    TF_LOG, TF_RATTLE, TF_CLAP, TF_BASSDRUM, TF_SNARE, TF_CYMBAL, TF_KICK, TF_HAT, TF_CLICK, TF_SUB, TF_JINGLE,
    TF_COUNT
};
extern const char* const TIMBRE_FAMILY_NAMES[TF_COUNT];   // "A PLUCKED STRING", "A PIPE", ... "A DRUM", "VOICES", "BRASS", ...
bool timbrePercussive(int family);                        // the kit's (a skin drum, a log drum, a rattle ... a click, a sub pulse)

struct Timbre {
    int family = TF_PLUCKED;
    double ratio[6] = {1, 2, 3, 4, 5, 6};                  // the partials' frequency ratios (a bell's stretched; the drum's two skins in Hz)
    double amp[6] = {1, 0.5, 0.33, 0.25, 0.2, 0.17};       // their amplitudes
    double attack = 0.01, decay = 0.5, sustain = 0, release = 0.1;   // seconds, seconds, level, seconds
    double vibRate = 0, vibDepth = 0;                      // vibrato: Hz and the share of the pitch
    double breath = 0;                                     // a noise component (a pipe's breath, a drum's skin)
    double gain = 0.3;
    // H-05: what the ages' families read (each family its own meaning; 0 where it has none)
    double bright = 1;      // the spectrum's tilt (a voice's, a synth's cutoff in multiples of the pitch, brass's flare)
    double detune = 0;      // cents between copies (a chorus, a section, a key's two strings, a pad's saws, a swarm's spread)
    int copies = 1;         // copies of the sound (a chorus of three, a section)
    double tract = 1;       // a voice's formants' scale (the people's speakers': `Voice::tract`)
    double index = 0;       // an FM index (electric keys), an overdrive (an electric string), the inharmonicity (keys), a wobble (a bone flute)
    double mod = 1;         // an FM ratio; a synth's wave (0 saw, 1 square); a pitch's fall (a kick's, a log's second partial)
    double sweep = 0.3;     // a filter's or an index's fall, seconds; a rattle's grains' spread
    double tone = 0;        // a percussion piece's pitch, Hz (a log drum's, a snare's tone); a field's beating, Hz
    int kind = 0;           // a variant within the family (a hand clap or sticks; a closed or an open hat; a barrel organ's tremolo)
    std::string name;       // the instrument as the decoder names it ("A BONE FLUTE", "A DRUM MACHINE"; C-17: the made one's, "A SHELL PIPE")
    int make = -1;          // C-17: the instrument it is as its people made it (`Tradition::makes`), -1 none (a voice, a church's organ, a piano)
};

// C-17 (2026-10-10): an instrument as its people made it (peoples/instruments.*): what sounds in it and the plan it is built on, its
// materials, its people's style as it read for it, its size; the name it is known by (the decoder's for its voice, the case's)
struct InstrumentMake {
    int cls = 0, plan = 0;           // what sounds (IC_*) and the plan it is built on (IP_*: peoples/instruments.h)
    int age = 0, reg = 0;            // the age it was made in; its register (-1 a bass, +1 a high one)
    int mat = 0, mat2 = 0, fit = 0, str = 0, skin = 0, trim = 0;   // its materials (IM_*): its body, the second (a neck, a frame, rails, arms),
                                     // the fittings (pegs, bands, keys, valves), the strings, a skin, the ornament's
    int stick = 0, vessel = 0, tongue = 0;   // and what its world gave for a stick (a handle, a beater, a bow), a vessel (a gourd), a tongue
    double taper = 1, waist = 0, square = 2, dome = 0;   // its bodies' outline (the people's: `FK_PLATE`'s numbers)
    int sides = 0, skew = 0;
    int hole = 0, top = 0, deco = 0, motif = 0, sym = 0, number = 5, rhythm = 0, light = 0, accent = 0;   // the people's openings, tops,
                                     // ornament, motif, symmetry, number, rhythm, light and its accent's place (peoples/genome.h)
    double slender = 0.5, size = 1, painted = 0;   // its proportion (0 squat .. 1 slender), its size against its plan's middle, its paint
    int variant = 0;                 // the plan's own choice (a valve's kind, a case's form, a drum's lacing)
    uint64_t seed = 0;
    std::string name, plural;        // "A SHELL PIPE" and "SHELL PIPES" (a kit's pieces on the decoder's line)
};

// H-05: what a part plays in an ensemble
enum MusicRole { MR_LEAD = 0, MR_SECOND, MR_CHORUS, MR_CHORDS, MR_BASS, MR_DRONE, MR_ALT, MR_ORGAN, MR_BOX, MR_SINGER, MR_BELL, MR_KEYS, MR_SWARM, MR_COUNT };
// and a percussion piece
enum KitRole { KR_LOW = 0, KR_HIGH, KR_TUNED, KR_SHAKE, KR_BACK, KR_TICK, KR_OPEN, KR_CRASH, KR_SUB, KR_COUNT };

// H-05: a people's instruments in one of its ages
struct Ensemble {
    int age = AGE_MEDIEVAL;
    std::vector<Timbre> voices;   // the melodic parts
    std::vector<int> role;        // what each plays (MR_*)
    std::vector<Timbre> kit;      // the percussion (`Note::degree` the piece)
    std::vector<int> kitRole;     // KR_*
    double tempo = 1;             // the age's beat against the tradition's
    double drift = 0;             // the tempo's wander (the tradition's in the hand-played ages, none from the machines)
    int band = 0;                 // the age's kind of ensemble (industrial: 0 a brass band, 1 strings and keys, 2 an orchestra, 3 a barrel organ; modern: 0 a drum machine, 1 a kit)
    int comp = 0;                 // the modern chords' way: 0 held on each group, 1 on every beat, 2 between the beats, 3 pushed ahead of the groups
    int voiceOf(int r) const { for (size_t i = 0; i < role.size(); i++) if (role[i] == r) return (int)i; return -1; }
    int kitOf(int r) const { for (size_t i = 0; i < kitRole.size(); i++) if (kitRole[i] == r) return (int)i; return -1; }
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
    std::vector<Timbre> voices;  // C-04's two or three melodic timbres, the first the lead: the people's own (H-05: their medieval instruments, and their leaning in every age)
    Timbre drum;
    bool hasDrum = true, drone = false;
    int second = 0;              // the second voice: 0 none, 1 in parallel below the lead, 2 an answer a phrase later, 3 doubling a period below
    int ornament = 0;            // 0 none, 1 grace notes before a leap, 2 trills on long notes, 3 slides between neighbours, 4 mordents
    double ornamentChance = 0.3;
    int rest = 0;                // the degree the phrases rest on besides the tonic (the one nearest a fifth, else a fourth)
    uint64_t seed = 0;
    // H-05: the people's own through its ages, and its ensemble of each age
    double swing = 0;            // the second of two subdivisions late by this share of one (0 even .. 0.3 a lilt)
    std::vector<int> vowels;     // the vowels its singers sing (0 a, 1 e, 2 i, 3 o, 4 u: the tongue's own)
    double density = 1;          // how busy its lines are against an age's middle
    int refrain = 0;             // a refrain it comes back to: the seed of a phrase its chants and its choruses share
    bool octaves = false;        // its choruses sing in octaves (the low voices under the answer)
    Ensemble ages[AGE_COUNT];
    // C-17: its instruments as made (`traditionMakes`, peoples/instruments.h: each voice's and kit piece's that a hand made), and the machine of
    // the industrial age that played its wax cylinders where it made one (-1 none)
    std::vector<InstrumentMake> makes;
    int machine = -1;
};
Tradition traditionOf(const BodyGen& g, const Lore& L);
double traditionHz(const Tradition& T, int degree);      // a degree's pitch (degrees past the scale wrap by the period, below zero the period before)
double traditionCents(const Tradition& T, int degree);   // its cents above the tonic

enum PieceForm {
    PF_DANCE = 0, PF_LULLABY, PF_WORK, PF_HYMN, PF_HARVEST, PF_LOVE, PF_FESTIVAL, PF_LAMENT, PF_MARCH, PF_WARNING, PF_LAST,   // C-04's
    PF_CHANT, PF_RINGDANCE, PF_HUNT, PF_KEENING, PF_HEALING,                                                               // H-05: the prehistoric
    PF_PROCESSION, PF_BALLAD,                                                                                              // the medieval (and a ballad in later ages)
    PF_TURNING, PF_ANTHEM, PF_PARLOUR, PF_MUSICBOX, PF_OVERTURE, PF_FUNERAL,                                               // the industrial
    PF_SONG, PF_DANCEREC, PF_JINGLE, PF_THEME, PF_PROTEST, PF_REQUIEM,                                                     // the modern
    PF_PROCESS, PF_SWARM, PF_SYNSONG, PF_DRIFT, PF_SIGNAL, PF_ELEGY,                                                       // the futuristic
    PF_FIELD, PF_TURNFIELD, PF_BELLFIELD, PF_STANDING,                                                                     // the far future
    PF_DAWN, PF_LASTSUN,   // D-05: the people who lost their sun: a dawn song to call it up (its line climbing), the last sunrise (an arch, then down)
    PF_COUNT
};
constexpr int PF_DRAWN_COUNT = PF_DAWN;               // D-05: the forms every other people's pieces draw among (a rounding's fall-through stays on the last)
extern const char* const PIECE_FORM_NAMES[PF_COUNT];   // "A DANCE", "A LULLABY", ... "THE LAST SONG", "A CHANT", ... "A STANDING TONE", "A DAWN SONG", "THE LAST SUNRISE"

struct Note {
    double t = 0, dur = 0;   // beats from the piece's start, length in beats
    int voice = 0;           // index into the piece's ensemble's voices; -1 the kit
    int degree = 0;          // the scale degree (0 the tonic, the scale's size the next period); the kit: the piece (index into the kit)
    double vel = 0.7;
    int slideFrom = -1000;   // a slide: the degree the note glides from over its start
    int vowel = -1;          // H-05: a sung note's vowel (0 a .. 4 u), -1 the voice's own
    double bend = 0;         // H-05: cents off the degree (the futuristic swarms' microtones)
};
struct Piece {
    int form = 0;
    double bpm = 100;
    int cycles = 0;              // its length in cycles
    double beats = 0;            // and in beats
    double seconds = 0;          // at its tempo
    std::vector<Note> notes;     // by their start
    uint64_t seed = 0;
    int age = AGE_MEDIEVAL;      // H-05: the ensemble it is played on (`Tradition::ages`)
    double drift = 0.03;         // H-05: the tempo's wander as played (none from a machine)
};
void pieceOf(const Tradition& T, const Shard& s, Piece& out);
bool pieceHasDrum(const Piece& P);
const Ensemble& pieceEnsemble(const Tradition& T, const Piece& P);   // H-05: the ensemble a piece is played on
std::string traditionLine(const Tradition& T);                 // "A SCALE OF 7 NOTES TO THE OCTAVE"
std::string cycleLine(const Tradition& T, double bpm);         // "A CYCLE OF 2+2+3 AT 112 BEATS A MINUTE"
std::string cycleLine(const Tradition& T, const Piece& P);     // H-05: the same, or the field's ("NO BEAT: 7 TONES OVER 3 OCTAVES")
std::string timbresLine(const Tradition& T, bool drum);        // "A PLUCKED STRING, A PIPE AND A DRUM" (C-04's own voices; and the drone, when the line has room)
std::string timbresLine(const Tradition& T, const Piece& P);   // H-05: the instruments the piece is played on ("A BONE FLUTE, VOICES AND RATTLES")
std::string ensembleLine(const Ensemble& E);                   // H-05: an age's whole ensemble, for the harness
