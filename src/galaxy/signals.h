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
    double power = 0;       // C-15: the transmitter's power (`transmitterPower`; a pulsar's third), 0 for the loud near pulsars and the local signals
    double needGain = 1;    // C-15: the least gain that hears it (1 within SIGNAL_REACH_LY: C-07's list)
    double airYears = 0;    // C-15: a people's: how long ago its world's first broadcast went out (the end's years plus the radio's, the earlier of two peoples')
    int age = -1;           // C-16: a people's: the age its first people on the air reached (AGE_*, `galaxy/peoples.h`): its carrier's colour
};

// the transmitter candidates of a cell of SIGNAL_CELL sectors (the cell's own hash: a few sectors or none); the cell's
// corner is cx * SIGNAL_CELL ...; returns how many sectors it wrote
int transmitterCell(int64_t cx, int64_t cy, int64_t cz, int64_t* sx, int64_t* sy, int64_t* sz);
bool sectorTransmits(int64_t sx, int64_t sy, int64_t sz);   // whether that sector is a transmitter candidate (one of its cell's)
// the signal of a star, if it has one: a people's world's transmission (the first world with the trait; `body`, `seed` the
// world's) or a pulsar's (`pulseHz`); the system is generated here. `pos`, `distLy` and `strength` are left for the caller
bool starSignal(const Star& s, Signal& out);
// the far signals heard from a position: the transmitters within SIGNAL_REACH_LY and the loud pulsars within
// SIGNAL_PULSAR_REACH_LY, with their direction, distance and strength; sorted by distance; returns the cells scanned.
// C-15: at a gain over 1 the transmitters beyond the reach that the gain carries are added (`SignalScan`, the whole scan at once)
int signalsNear(const Vec3& obsKm, std::vector<Signal>& out, double reachLy = SIGNAL_REACH_LY, double gain = 1);

// C-15 (2026-10-08): the gain (R-410, deep listening). Every transmitter sector has a power (`transmitterPower`): the sectors of
// C-07's cells a weak one (0.005-0.05, log-uniform), and a second, sparse population of strong ones hashed in cells of
// SIGNAL_DEEP_CELL sectors (`deepCell`: a Poisson count of mean 1.4 a cell, powers 1-40 log-uniform), so most are weak and a
// long tail is strong. At gain G a transmitter of power p is heard to 200 sqrt(1 + (G - 1) p) ly: within 200 ly every one at
// every gain (C-07's list), beyond as far as its power carries it, the received power falling with the square of the distance.
// The steps are even in G (x1, x5, x11, x18, x25, x32), so each adds about the same area of the disc, about two peoples' signals
// a step on the arm; the strongest are heard to about 7,000 ly at x32. Beyond the reach a people is heard only where the light
// of its world's first broadcast has reached the ship (`airYears`: a signal d ly off left d years ago, and before their radio
// there was nothing to hear); within it C-07's rule stands (the ends run from 200 years, so the light is always after them:
// the machines). A pulsar in a transmitter sector has a third of its sector's power; the loud near pulsars (C-07's) keep
// their 25 ly at every gain
constexpr int SIGNAL_GAIN_STEPS = 6;
extern const int SIGNAL_GAINS[SIGNAL_GAIN_STEPS];   // 1, 5, 11, 18, 25, 32
constexpr int SIGNAL_DEEP_CELL = 256;               // the strong transmitters' cells
constexpr int SIGNAL_DEEP_MAX = 6;                  // candidates a strong cell may hold
constexpr double SIGNAL_PULSAR_POWER = 0.35;        // a pulsar's share of its sector's power
int deepCell(int64_t cx, int64_t cy, int64_t cz, int64_t* sx, int64_t* sy, int64_t* sz, double* power);   // the strong transmitters of a cell
double transmitterPower(int64_t sx, int64_t sy, int64_t sz);   // the sector's power (the stronger where both populations hold it); 0 when it is no transmitter
double signalReachAtGain(double power, double gain, double reachLy = SIGNAL_REACH_LY);   // reachLy sqrt(1 + (gain - 1) power)
double signalNeedGain(double distLy, double power, double reachLy = SIGNAL_REACH_LY);    // the least gain that hears it (1 within the reach; 1e9 a power of 0 beyond)
double signalStrengthAtGain(const Signal& s, double gain);   // the 0..1 strength at that gain: C-07's within the reach and in the system, a far one's by its margin over the floor
double signalLevel(const Signal& s, double gain);            // what the meter reads off it at the beam's centre: C-07's strength at x1, a near or a local one a little louder with the gain (x1.4 at x32), a far one its strength (a small rise over the gain's floor)
double signalDrive(const Signal& s, double gain);            // how hard it drives the receiver: a near or a local one's strength times gain^0.4 (1 and over overloads it), a far one 0
// the scan beyond the reach, resumable so the radar can spread it over frames: the far parts of C-07's cells and the strong
// cells within the reach of their strongest at `gain`; `out` holds the signals the gain hears there (by `needGain`), sorted when done
struct SignalScan {
    std::vector<Signal> out;
    bool done = true;
    int cells = 0;            // the cells scanned so far
    void begin(const Vec3& obsKm, double gain, double reachLy = SIGNAL_REACH_LY);
    bool step(int budget);    // up to `budget` cells; true when the scan is done
private:
    Vec3 obs; double gain = 1, reachLy = SIGNAL_REACH_LY;
    int64_t o[3] = {0, 0, 0}, lo[3] = {0, 0, 0}, span[3] = {0, 0, 0};
    int layer = 0; int64_t next = 0, total = 0; double layerReach = 0;
    void beginLayer();
    void consider(int64_t x, int64_t y, int64_t z, double power);
};
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
struct BodyGen;
// C-13: whose recording a slot of a world's broadcast is (the slot's coin where both peoples are on the air); A-01: only the peoples
// that had radio are on the air (`peoplesOnAir`), so where one of the two had none every slot is the other's. -1: nobody is
int slotPeople(const BodyGen& g, uint64_t worldSeed, int slot);
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
//
// C-16 (2026-10-09): each age on the air (`docs/ideas/IDEAS-civilisation-expansion.md`, section 10). C-07's programmes are the
// modern age's (voices, music, numbers, a carrier's buzz); a slot's people of another age broadcasts what its age could: the late
// industrial age the recording played into the transmitter from a phonograph through its horn (RP_PHONOGRAPH: a voice, or the
// piece) and its text keyed by a spark gap in a code of the people's own (RP_MORSE: each letter of their words a pattern of dots
// and dashes by the world's hash, after the station's call); the futuristic age data and beacons (C-07's machines) and the
// recording read by a synthetic voice (RP_SYNTHETIC); the far future a beacon that is mathematics (RP_MATHS: the primes, the
// squares, the Fibonacci numbers, the powers of two, the harmonic series, a count in binary, the digits of a constant, by the
// world). What a programme carries is the people's own catalogue, so the records of later milestones reach the air from here.
#include "voice.h"
#include "peoples.h"
enum RadioProgramme { RP_VOICE = 0, RP_WHISPERS, RP_CHANT, RP_NUMBERS, RP_LOOP, RP_BEACON, RP_DATA, RP_TOLL, RP_SIREN, RP_MURMUR, RP_MUSIC,
                      RP_MORSE, RP_PHONOGRAPH, RP_SYNTHETIC, RP_MATHS, RP_COUNT };   // C-16: the ages' four after C-07's eleven
extern const char* const RADIO_PROGRAMME_NAMES[RP_COUNT];   // "A VOICE", "WHISPERS", ... for the harness
// C-16: RP_MORSE's keying, a byte an element: under 0x80 the key down that many units, MORSE_OFF | n the key up n units
constexpr uint8_t MORSE_OFF = 0x80;
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
    bool piece = false;         // C-16: the slot's piece is on the air (RP_MUSIC, and RP_PHONOGRAPH in a music slot)
    std::vector<uint8_t> code;  // C-16 RP_MORSE: the keying (MORSE_OFF), `unit` seconds a unit
    double unit = 0.08;
    int age = AGE_MODERN;       // C-16: the age of the slot's people (what its age could put on the air)
};
// the programme in that slot of the world's broadcast; C-16: by the age of the slot's people (C-07's draw is the modern age's)
int programmeOf(uint64_t worldSeed, int slot, bool music, int age = AGE_MODERN);
uint64_t programmeSeed(uint64_t worldSeed, int slot);
// the programme of a slot, ready to play: `text` is the shard decoded (empty for a music slot), `year` the shard's
void programmeFor(uint64_t worldSeed, int slot, bool music, int year, const Voice& V, const Tongue& T, const std::vector<DecodedWord>& text, Programme& out, int force = -1, int age = AGE_MODERN);   // `force`: that programme regardless of the hash (the harness)
double morseSeconds(const std::vector<uint8_t>& code, double unit);   // C-16: how long a keying lasts
// RP_NUMBERS: the words of repeat `k` (a fresh digit group each time); the others repeat `words`
void programmeRepeatWords(const Programme& p, int k, const Tongue& T, std::vector<DecodedWord>& out);
