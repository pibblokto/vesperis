#pragma once
// C-07 (2026-10-02): the signal radar. A few of the dead peoples' worlds still have a transmission on its way out through the
// galaxy; the Stardrifter's receiver, swept round the sky, catches it as a rise in the static that sharpens toward the source
// and, held on it, locks: the distance in light years is the age of what is heard, and what is heard is one of the world's
// fifty recordings (`galaxy/shards.h`: a voice, C-05, or a piece, C-04), so the world's people are known by their voice before
// their ruins are walked. Which worlds transmit is a property of the galaxy, not of the sweep: the transmitters are hashed
// sectors (`transmitterCell`: a few in a cube of SIGNAL_CELL sectors, about one in a million sectors), and the sector's
// star transmits when it has a people's world (`TR_CIVILISATION`, `galaxy/ruins.h`). So the signals are rare (one or two
// within SIGNAL_REACH_LY of home, on the arm; more in the core) and far, and two explorers sweeping the same sky hear the
// same ones. The radar is honest as an instrument: it also catches the pulsars (the loud ones among the near, by a hash of
// their own, and any pulsar in a transmitter sector), a comet outgassing near its sun and the magnetosphere of a giant or a
// strong-fielded world of the system the ship is in (`localSignals`), none of them a story.
#include "system.h"
#include <vector>

constexpr double SIGNAL_REACH_LY = 200;       // how far a people's transmission is heard (the signal's age in years, at most)
constexpr int SIGNAL_CELL = 32;               // the transmitter hash's cube of sectors
constexpr double SIGNAL_PULSAR_REACH_LY = 25; // a near pulsar's reach
constexpr double SIGNAL_PULSAR_SHARE = 1.0 / 320;   // the share of the near pulsars loud enough to be heard (one or two within 25 ly on the arm)
constexpr int SIGNAL_MAX_CANDIDATES = 3;      // transmitter candidates a cell may hold

enum SignalKind { SIG_PEOPLE = 0, SIG_PULSAR, SIG_COMET, SIG_MAGNETOSPHERE, SIG_KIND_COUNT };
extern const char* const SIGNAL_KIND_NAMES[SIG_KIND_COUNT];   // "A PEOPLE'S TRANSMISSION", "A PULSAR", "A COMET OUTGASSING", "A MAGNETOSPHERE"

struct Signal {
    int kind = SIG_PEOPLE;
    Star star;              // the source's star (a people's world's, the pulsar itself; the local signals' the system's)
    int body = -1;          // the people's world, the comet, the giant; -1 a pulsar
    int bodyType = -1;      // its PlanetType
    Vec3 pos;               // the source, km
    double distLy = 0;      // from the observer: a people's and a pulsar's age in years
    double strength = 1;    // 0..1 at the beam's centre: by the distance, so a far signal is a small rise in the static
    double pulseHz = 0;     // a pulsar's
    double detail = 0;      // a comet's distance from its sun in AU; a magnetosphere's field 0..1
    uint64_t seed = 0;      // the source's own (the world's, the star's, the body's)
    bool local = false;     // of the system the ship is in
};

// the transmitter candidates of a cell of SIGNAL_CELL sectors (the cell's own hash: a few sectors or none); the cell's
// corner is cx * SIGNAL_CELL ...; returns how many sectors it wrote
int transmitterCell(int64_t cx, int64_t cy, int64_t cz, int64_t* sx, int64_t* sy, int64_t* sz);
bool sectorTransmits(int64_t sx, int64_t sy, int64_t sz);   // whether that sector is a transmitter candidate (one of its cell's)
// the signal of a star, if it has one: a people's world's transmission (the first world with the trait; `body`, `seed` the
// world's) or a pulsar's (`pulseHz`); the system is generated here. `pos`, `distLy` and `strength` are left for the caller
bool starSignal(const Star& s, Signal& out);
// the far signals heard from a position: the transmitters within SIGNAL_REACH_LY and the loud pulsars within
// SIGNAL_PULSAR_REACH_LY, with their direction, distance and strength; sorted by distance; returns the cells scanned
int signalsNear(const Vec3& obsKm, std::vector<Signal>& out, double reachLy = SIGNAL_REACH_LY);
// the system's own signals at time t: a comet within 1.5 AU of its sun outgassing, the magnetosphere of every gas giant and of
// every big world with a field of the strongest (0.8 and over), and, when the system's sector is a transmitter, its people's world (a
// local SIG_PEOPLE), with their direction and strength from the observer
void localSignals(const StarSystem& sys, const Vec3& obsKm, double t, std::vector<Signal>& out);
// what the beam reads off a signal that is `angleRad` off its axis: the gain 0..1 (a wide lobe that rises within forty
// degrees, a middle one within ten, a narrow one within three)
double beamGain(double angleRad);
// the recording a people's world has on the air at game time t (seconds): an index into its fifty, changing every
// SIGNAL_RECORDING_S
constexpr double SIGNAL_RECORDING_S = 75;
int transmittedShard(uint64_t worldSeed, double t);
int nextTransmittedShard(uint64_t worldSeed, int prev);   // the recording after that one in the world's broadcast (a hashed chain through the fifty)
double signalStrengthAt(double distLy, double reachLy);   // 1 near, 0.18 at the reach
std::string signalSourceLine(const Signal& s);   // "A DESERT WORLD OF AN ORANGE DWARF", "A PULSAR, 1.7 PULSES A SECOND", ...

// C-07 (2026-10-02, the user's second call: "give MORE sounds for signals, greater variety in that cosmic horror direction"):
// what a people's transmission carries. A world's broadcast is a walk through its fifty slots (`transmittedShard`), but a
// slot is not always the recording read out: it is a programme of the world's dead machines, hashed from the world and the
// slot (`programmeOf`), so each world has a character of its own (two of the five machine programmes and two of the four
// ways of speaking are its). A text slot is spoken as it is (RP_VOICE), whispered (RP_WHISPERS), intoned slowly over a hum
// (RP_CHANT), read as digit groups with a chime between (RP_NUMBERS: a numbers station, the groups from the slot's seed and
// the shard's year), or its opening words repeated on a loop (RP_LOOP); a music slot is a piece rarely (RP_MUSIC, slowed and
// drowned) and otherwise a machine: a beacon's pattern of tones (RP_BEACON), bursts of data (RP_DATA), a bell tolling
// (RP_TOLL), a siren still running (RP_SIREN) or a murmur of many voices (RP_MURMUR). The synth plays the machines from the
// programme's seed (`AudioState::radarVoice`, `radarSeed`); the speaking ones go through the voice synth in a voice derived
// from the world's (`programmeFor`). The recording counts as heard when its words or its piece were on the air (`speaks`).
#include "voice.h"
enum RadioProgramme { RP_VOICE = 0, RP_WHISPERS, RP_CHANT, RP_NUMBERS, RP_LOOP, RP_BEACON, RP_DATA, RP_TOLL, RP_SIREN, RP_MURMUR, RP_MUSIC, RP_COUNT };
extern const char* const RADIO_PROGRAMME_NAMES[RP_COUNT];   // "A VOICE", "WHISPERS", ... for the harness
struct Programme {
    int kind = -1;              // RP_*
    uint64_t seed = 0;          // the programme's own: the machines' patterns, the groups' digits
    bool speaks = false;        // the shard's words or piece are on the air (the recording is heard)
    bool machine = false;       // no speech and no piece: the synth's own
    Voice voice;                // the speaking programmes': the world's voice, or a copy whispering, chanting, reading flat
    std::vector<DecodedWord> words;   // what is spoken (the shard's words, its opening phrase, the first digit group)
    int repeats = 1;            // RP_LOOP, RP_NUMBERS: the phrase or the groups spoken that many times, `gap` seconds between
    double gap = 0;
    double seconds = 0;         // a machine programme's length on the air (the speaking ones' is the speech's, times the repeats)
    double pieceSpeed = 1;      // RP_MUSIC: the piece played that much slower
};
int programmeOf(uint64_t worldSeed, int slot, bool music);   // the programme in that slot of the world's broadcast
uint64_t programmeSeed(uint64_t worldSeed, int slot);
// the programme of a slot, ready to play: `text` is the shard decoded (empty for a music slot), `year` the shard's
void programmeFor(uint64_t worldSeed, int slot, bool music, int year, const Voice& V, const Tongue& T, const std::vector<DecodedWord>& text, Programme& out, int force = -1);   // `force`: that programme regardless of the hash (the harness)
// RP_NUMBERS: the words of repeat `k` (a fresh digit group each time); the others repeat `words`
void programmeRepeatWords(const Programme& p, int k, const Tongue& T, std::vector<DecodedWord>& out);
