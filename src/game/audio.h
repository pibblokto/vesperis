// Procedural ambient audio: ship hum, Vimana drive, planetary wind, rain, beeps; C-04: the old peoples' music; C-05: their voices.
#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
struct Piece; struct Tradition; struct Note; struct Timbre; struct Ensemble;   // peoples/music.h
struct Speech; struct Voice; struct Segment;                   // peoples/voice.h
struct Signal;                                                 // peoples/signals.h
constexpr int MEDIUM_AIR = 100;   // C-11: `AudioState::medium` for a recording the radar caught (the receiver's band, the air's hiss, its slow fading)

// D-03 what the synth needs of a drifter (SIG_ROGUE) besides its seed (`rogueVoiceFor` reads it off its signal): what its voice is by its
// body (RV_*, peoples/signals.h; a giant's when not told), the period of its turning and of its innermost moon's orbit (the game's seconds;
// 0 no moon) and its power (`drifterPower`)
struct RogueVoice { int cls = 1; double turnS = 0, moonS = 0, power = 0.6; };
RogueVoice rogueVoiceFor(const Signal& s);
// D-03 where the receiver is heard: the radar camera, the cabin's radio at a drifter, the suit's radio on its ground (a small set's band),
// and the radios' levels (`AudioState::radar`; the camera's is 1)
enum RadioSet { RADIO_CAMERA = 0, RADIO_CABIN, RADIO_SUIT };
constexpr double RADIO_CABIN_LEVEL = 0.45, RADIO_SUIT_LEVEL = 0.4;

// what `AudioState::beep` plays once (audio.cpp: a lock 880 Hz, an arrival 660 Hz and longer, a refusal 220 Hz, a key 1320 Hz)
enum Beep { BEEP_NONE = 0, BEEP_LOCK, BEEP_ARRIVE, BEEP_ERROR, BEEP_UI };

struct AudioState {
    double hum = 0;        // ship interior hum 0..1
    double engine = 0;     // vimana / approach drive 0..1
    double wind = 0;       // surface wind 0..1
    double rain = 0;       // 0..1
    double lava = 0;       // rumble 0..1
    int beep = 0;          // a Beep to play (BEEP_NONE: none)
    double thunder = 0;    // >0 triggers thunder
    double master = 0.8;   // master volume 0..1 (settings)
    double breath = 0;     // heavy breathing 0..1 (sprint stamina)
    // M4-09 per-world sound
    double windTone = 0.5; // 0 sea (low, slow surges) .. 1 mountain whistle
    double surf = 0;       // breaking waves nearby 0..1
    double birds = 0;      // bird calls 0..1
    int step = 0;          // >0: one footstep of this kind (1 soft, 2 hard, 3 splash)
    double stepGain = 0.5;
    double leaves = 0;     // wind through leaves and undergrowth rustle 0..1
    // creatures
    int call = 0;          // >0: play a creature call of this kind (1 chirp, 2 low, 3 hoot, 4 click)
    double callPitch = 1;  // pitch multiplier of that call
    double hoofs = 0;      // hoof and paw steps nearby 0..1
    double insects = 0;    // insect hum at dusk near water 0..1
    // the buggy
    double engineRpm = 0;  // 0..1 within the current gear (pitch)
    double skid = 0;       // tyre slide 0..1
    double thud = 0;       // >0: a suspension thump of this strength (one-shot)
    // the drone
    double rotor = 0;      // the thrust pods' spin 0..1 (the whine's level)
    double rotorPitch = 0; // 0..1 with the speed and the climb (the whine's pitch)
    // the sounds of the ground (the world's traits): the singing dunes' hum (0..1: a slip face's sand sliding under the explorer or the
    // buggy) at its note (the grains' and the gravity's), the singing ice round the explorer (0..1: the ice near, the day warming or cooling
    // it), a quake's shaking (0..1, the surface's) and its onset (one-shot: its strength); how much of them the air carries (1 at a third of a
    // bar and over, 0.15 in the thinnest, 0 airless: a quake through the boots alone)
    double dunes = 0, duneHz = 80, ice = 0, quake = 0, quakeHit = 0, air = 1;
    int iceEvents = 0;   // the ice's sounds begun so far (the harness counts them)
    // the ground's sounds off, every level the surface sets (`updateSurface`): the capsule, the ship and a load hear none of
    // them (the ship silenced the wind, the rain and the lava only; the breath, the waves, the leaves, ... went on aboard)
    void surfaceOff() { wind = rain = lava = breath = surf = birds = leaves = hoofs = insects = engineRpm = skid = rotor = 0; dunes = ice = quake = quakeHit = 0; }
    // a piece of the old peoples' music (the decoder screen): the piece and its tradition, which must outlive the play;
    // `pieceStart` plays it from the top, a null `piece` stops it at once; the synth reports the beat it is at (-1 idle) and the end
    const Piece* piece = nullptr;
    const Tradition* tradition = nullptr;
    bool pieceStart = false, piecePause = false, pieceDone = false;
    double pieceGain = 1.0, pieceBeat = -1;
    // a text shard spoken (the decoder screen): the speech and the voice, which must outlive the play; `speechStart` speaks it
    // from the top, a null `speech` stops it at once; the synth reports the second it is at (-1 idle) and the end
    const Speech* speech = nullptr;
    const Voice* voice = nullptr;
    bool speechStart = false, speechPause = false, speechDone = false;
    double speechGain = 1.0, speechT = -1;
    // what the record on the decoder was made on (RM_*, peoples/records.h; -1 none: the radar's programmes, the harness's bare renders):
    // a recording is heard through it (a wax cylinder's horn, crackle and turning, a disc's surface, a reel's hiss and wow, a drive's rot,
    // a crystal's shimmer, the hum under something that hums); a record with no sound of its own is the ship's own rendering, dry.
    // `MEDIUM_AIR`, a recording the radar caught, as the ship kept it off the air
    int medium = -1;
    // the ship's player: a piece or a recording taken up again where it was held (the decoder or the radar had the synth meanwhile):
    // the beat the piece starts at and the second the speech starts at, read with `pieceStart` and `speechStart` (then 0)
    double pieceFrom = 0, speechFrom = 0;
    // an instrument played in the cabin: its timbre (which must outlive the play: its people's tradition holds it), the notes struck
    // since the synth last read them (a pitch, a velocity, the key that holds it down) and the keys let go; a key of 0 is a note that
    // lasts its `hold` seconds; the synth reports the notes sounding (the harness)
    struct InstrumentNote { double hz = 0, vel = 0.7, hold = 0; int key = 0; const Timbre* timbre = nullptr; };   // `timbre`: a drum machine's pad (else the instrument's); `hz` 0: its own tone
    const Timbre* instrument = nullptr;
    std::vector<InstrumentNote> instrumentHits;
    std::vector<int> instrumentUps;
    double instrumentGain = 1.0;
    int instrumentSounding = 0;
    // the signal radar: the receiver's static while it is on, how much of the signal in the beam comes through it, what that
    // signal is (SIG_*, -1 none: a pulsar's pulses, a comet's hiss and crackle, a magnetosphere's whistlers and chorus are the
    // synth's own; a people's recording is the piece or the speech above, which `radio` sends through the receiver's band)
    double radar = 0, radarSignal = 0;
    int radarKind = -1; double radarPulseHz = 1;
    bool radio = false;
    // the user's second call ("MORE sounds for signals, greater variety in that cosmic horror direction"): the programme a
    // people's transmission carries (RP_*, peoples/signals.h; -1 none: the carrier alone) and the signal's seed, from which the
    // synth draws the signal's own voice (no two pulsars thump alike) and the machines' patterns; `pieceSpeed` slows the piece
    int radarVoice = -1; uint64_t radarSeed = 0; double pieceSpeed = 1;
    // how far a near signal through a high gain overdrives the receiver (0 clean .. 1 the sound clipped hard)
    double radarDrive = 0;
    // what the gain lets in: the dial (0 at x1 .. 1 at x32: the floor rises, the static's events come oftener and stranger),
    // the galaxy's own noise in the beam (the band's brightness there, 0..1), how worn the signal heard is (0 within 200 ly .. 1
    // the farthest: a narrower band, deep slow fades, echoes of itself, its pitch drifting, dropouts), a false rise in the beam
    // (0..1: the static gathering into almost a tone) and the age of the people on the air (AGE_*, -1 none: the carrier's colour)
    double radarGainT = 0, radarSky = 0, radarWear = 0, radarGhost = 0;
    int radarAge = -1;
    // the signal under the one centred, bleeding in from the wide lobe (its kind, programme, age, seed, rate and how much of it comes
    // through), the heterodyne whistle between two peoples' carriers (its pitch, sliding as the beam moves, and level) and the
    // intermodulation's metallic ring at the top steps
    int radarKind2 = -1, radarVoice2 = -1, radarAge2 = -1; uint64_t radarSeed2 = 0; double radarPulseHz2 = 1, radarSignal2 = 0;
    double radarHetHz = 800, radarHet = 0, radarIntermod = 0;
    // RP_MORSE's keying (`Programme::code`, which must outlive the play: the synth keys it from the top when the seed changes)
    const std::vector<uint8_t>* radarCode = nullptr; double radarCodeUnit = 0.08;
    // a word of a recording heard before coming back out of the static: the speech above is not a signal's but that word, heard
    // through the receiver as from very far at a level of its own (`radio` with it)
    bool radarGhostWord = false;
    int radarEvents = 0, radarEventKinds = 0;   // the static's events begun so far (the harness counts them) and a bit per kind (0-11)
    // a drifter's voice (SIG_ROGUE): what it is besides its seed, for the signal centred and the one under it (`radarVoice` with a
    // drifter is the beacon under its voice: a programme, D-05's; -1 none); where the receiver is heard (RADIO_*)
    RogueVoice radarRogue, radarRogue2;
    int radarSet = RADIO_CAMERA;
    // the probe's relay: how much of it is heard (1 on the probe's screen, a murmur from the landing screen in the cabin), the
    // stage (PS_*), the air's rush (0..1), the entry's heat, the chute's flutter, the depth (0..1: the dark's rumble and the hull's
    // creaks), the link (1 clear .. 0 failing: the static), a thunderclap's strength (one-shot) and the loss of signal (the hiss)
    double probe = 0, probeWind = 0, probeHeat = 0, probeChute = 0, probeDepth = 0, probeSignal = 1, probeThunder = 0;
    int probeStage = 0; bool probeLost = false;
    // auto-explore's tape (game/tape.cpp): a music file the platform streams beside the synth, at the file's own rate and in stereo
    // (raylib decodes MP3, OGG, WAV and QOA). `music` is the file (empty: none), begun `musicFrom` seconds in when `musicGo` is set (the
    // platform clears it); `musicGain` its level, the master's applied too (above 1 a quiet track raised). The platform reports the
    // second it is at (-1 none), the track's length, its end and a file it could not play
    std::string music; double musicFrom = 0, musicGain = 1; bool musicGo = false;
    double musicT = -1, musicLen = 0; bool musicEnded = false, musicFailed = false;
    // what the synth's own sounds keep of their level under the tape (1 .. TAPE_DUCK), and the cassette deck's mechanics: a sequence
    // (DECK_*) begun when `deck` is set (then 0; -1 hushes the one playing), `deckSeed` its variation; the deck itself is not ducked
    double duck = 1; int deck = 0; uint32_t deckSeed = 0;
};

// R-420 the cassette deck the tape is heard through (game/audio_deck.cpp): a cassette put in and played; a tape's end, the cassette changed
// and the next played; the play key alone (the cassette still in); the stop key
enum DeckKind { DECK_NONE = 0, DECK_INSERT, DECK_CHANGE, DECK_PLAY, DECK_STOP, DECK_KINDS };
double deckTapeAt(int kind, uint32_t seed);                             // the second of the sequence the tape moves (the music begins; DECK_STOP: it stops)
double deckLength(int kind, uint32_t seed);                             // its whole length (the hiss's tail under the music)
std::vector<float> deckRender(int kind, uint32_t seed, int sampleRate);   // its sound, mono

class AudioSynth {
public:
    void render(float* out, int frames, int sampleRate, AudioState& st);
    static const char* mathsName(uint64_t seed);   // C-16: the sequence a beacon of mathematics with that seed sends ("the primes", ...), for the harness
    // a drifter's voice for the harness: in words and numbers (its breath, its moon's pulse, its motif, its drones, its moan, its body's
    // parts), and as its parameters, each 0..1 (the distinctness's spread)
    static std::string rogueLine(uint64_t seed, const RogueVoice& r);
    static void rogueTraits(uint64_t seed, const RogueVoice& r, std::vector<double>& out);
    static double rogueBreathS(uint64_t seed, const RogueVoice& r);   // the breath's period (s): the beam sweeping past once a turning
private:
    double phase1 = 0, phase2 = 0, lfo = 0;
    double humLvl = 0, engLvl = 0, windLvl = 0, rainLvl = 0, lavaLvl = 0, breathLvl = 0, breathPh = 0;
    double surfLvl = 0, birdLvl = 0, chirpT = -1, chirpF = 3000, nextChirp = 1, stepT = -1, stepGainCur = 0.5; int stepKind = 0; double lpS = 0, hpS = 0;
    double leavesLvl = 0, lpL = 0, hpL = 0;
    double callT = -1, callPitchCur = 1; int callKind = 0;
    double hoofLvl = 0, hoofT = 0, insectLvl = 0, lpI = 0, lpI2 = 0;
    double rpmLvl = 0, skidLvl = 0, thudT = -1, thudAmp = 0, lpK = 0;
    double rotorLvl = 0, rotorPitchLvl = 0, rotorPh = 0, lpR = 0;
    // the sounds of the ground: the dunes' hum (its note gliding to the world's, its phase and clock, the sand's hiss), the ice's events (a
    // near crack's chirp, a far one's, the sheet's boom, the snow settling: its kind, clock, length, pitches, loudness, phase, filters), the
    // quake's (its rumble's poles, its clock, the P-wave's thump, the stones' rattle)
    double duneLvl = 0, duneHzCur = 80, dunePh = 0, duneT = 0, duneHp = 0;
    double iceLvl = 0, iceT = -1, iceNext = 1.5, iceDur = 0.2, iceF0 = 3000, iceFLo = 600, iceT0 = 0.05, iceAmp = 0, icePh = 0, iceLp = 0, iceY1 = 0, iceY2 = 0; int iceKind = 0;
    double quakeLvl = 0, qkClock = 0, qkLp1 = 0, qkLp2 = 0, qkLp3 = 0, qkT = -1, qkAmp = 0, qkPh = 0, qkRatT = -1, qkRatA = 0, qkRatF = 2000, qkRY1 = 0, qkRY2 = 0;
    double groundSample(double dt, AudioState& st);
    // the receiver: the content's band and its ghosting (the wow, the ring, the dropouts, the carrier), the bed (the rumble,
    // the drone, the wisp, the hiss), the signals' own sounds (a pulsar's thump and growl and whine; a comet's song, geysers and ice;
    // a magnetosphere's whistlers, chorus, hiss band and choir) and the echo everything heard goes through
    double rdDriveLvl = 0;   // the overdrive, smoothed
    double radarLvl = 0, radarSigLvl = 0, rdHp = 0, rdLp1 = 0, rdLp2 = 0, rdRing = 0, rdDrop = 1, rdDropLvl = 1, rdDropT = 0, rdContent = 0;
    double rdBrown = 0, rdBrown2 = 0, rdN1 = 0, rdWispY1 = 0, rdWispY2 = 0, rdWispT = 0, rdDr1 = 0, rdDr2 = 0, rdDr3 = 0;
    std::vector<float> rdEcho; size_t rdEchoI = 0; double rdEchoLp = 0;
    // the variety (the user's second call): the signal's parameters from its seed, the phantoms (voices that are no one's: the
    // murmur, the moans in the static), the machines of a people's broadcast, the bed's events, the content through a tape's wow
    struct RadarVar {
        double thumpF = 70, interpulse = 0, nullOn = 8, nullOff = 3, growl = 1, whineF = 2400; bool nulling = false, breath = false;   // a pulsar
        double songF = 92, ratio = 1.0595, geyserRate = 1, crackle = 1, moanF = 120; bool glide = false, moan = false;                 // a comet
        double whistlerRate = 1, chorusRate = 1, sfRate = 2, choirRate = 1, hissF = 500; int choirSet = 0; bool sferics = false, roar = false;   // a magnetosphere
        int beacN = 4; double beacF[7] = {0, 0, 0, 0, 0, 0, 0}, beacDur = 0.2, beacGap = 0.15, beacRest = 3, beacSq = 0.3; bool beacSag = false;
        double dataF1 = 700, dataF2 = 950, dataRate = 30, dataBurst = 1.5, dataGap = 2, dataHum = 50;
        double bellF = 240, bellPeriod = 5, bellDecay = 5, bellFar = 0.2;
        bool sirenTwo = false; double sirenF = 400, sirenRatio = 1.4, sirenCycle = 2, sirenSwell = 14;
        double murRate = 3.8, murF0[3] = {110, 150, 210};
        // the ages: the spark gap (its rate, its ring, how quenched, how fast a hand of its own keys), the phonograph (its turn,
        // its horn, its surface), the synthetic voice (the ring under it, the coarse clock), the futuristic carrier's whine, the
        // beacon of mathematics (the sequence, its pitch, a pip, the gaps), and an under carrier's own pitch
        double sparkHz = 500, ringF = 1600, sparkDecay = 3000, morseUnit = 0.08; bool quench = false;
        double turnHz = 1.3, hornF = 1000, surface = 1;
        double ringHz = 50, holdHz = 6000, whineHz = 3800;
        int mathKind = 0; double mathF = 440, mathPip = 0.06, mathGap = 0.09, mathRest = 0.8;
        double carrier2F = 1200;
        // a drifter's (audio_rogue.cpp, `rogueDerive`): its beam (the main lobe's half-width in the turning, the other pole's strength
        // and where it comes, a breath of its own for a drifter of no known spin), its motif (how many whistles, their pitches on a scale of
        // `rgEdo` steps to the octave, their lengths and glides, the figure's length, the vibrato, the timbre), its vowels (the moan's pitch,
        // the three it sweeps through, the moan's and the plasma's paces), its drones (a chord of two to four notes on its scale, each beating
        // against a twin a little off; their six partials by a tilt and a share of the even ones; their level for the chord's size), its storm
        // (the surf's band, the S-bursts' rate, pitch and fall, the roar's pitch and flutter), its chorus (the chirps' base, rise and length),
        // its whistlers' top, its hiss band, its howl's range, its moon's arc, its iron's ring; its parts' weights
        double rgLobe = 0.22, rgPole2 = 0.5, rgPole2At = 0.5, rgBreathU = 0.5;
        int rgN = 4, rgEdo = 12, rgTimbre = 0; double rgNoteF[6] = {0, 0, 0, 0, 0, 0}, rgNoteLen[6] = {0, 0, 0, 0, 0, 0}, rgGlide[6] = {0, 0, 0, 0, 0, 0};
        double rgMotifLen = 1, rgVibHz = 5, rgVib = 0.006, rgAnswer = 1, rgHangS = 12;   // the other pole's figure's key (a ratio), how long a whistle hangs on
        double rgMoanF = 80, rgMoanRate = 0.06, rgPlasmaRate = 0.04; int rgVowel[3] = {0, 2, 4};
        int rgChordN = 3; double rgChordF[4] = {50, 75, 100, 150}, rgBeat[4] = {0.6, 1.1, 0.8, 1.4}, rgPart[6] = {1, 0, 0, 0, 0, 0}, rgTilt = 1.2, rgEven = 0.5, rgDroneAmp = 0.05;
        double rgSurfF = 600, rgSRate = 20, rgSF = 1500, rgSFall = 0.3, rgRoarF = 180, rgRoarFl = 24;
        double rgChorusF = 700, rgChorusRise = 2.4, rgChirp = 0.25, rgWhistF = 3500, rgHissF = 600, rgHowlLo = 550, rgHowlHi = 1300, rgArcF = 800, rgRingF = 220;
        // how its parts are weighed against each other (no two the same balance, `RW_*` in audio_rogue.cpp): the drones, the moan, the plasma's
        // breath; a storm's surf, S-bursts and roar or a small world's chorus, whistlers and hiss; the kilometric howl (a giant's in two of five)
        double rgW[7] = {1, 1, 1, 1, 1, 1, 1}; bool rgGiantHowl = false;
        double rgResF[2] = {400, 1800}, rgResG[2] = {1, 1};   // its plasma's two resonances the whole voice passes through (their centres and gains)
    };
    static void rvDerive(RadarVar& v, uint64_t seed);
    static void rogueDerive(RadarVar& v, uint64_t seed);   // a drifter's part of it, on indices of its own (no other signal's moves)
    struct Phantom {
        double ph = 0, jit = 0, f0 = 120, f0T = 120, f0Base = 120, amp = 0, ampT = 0, sylT = 0, sylDur = 0.2, lpN = 0;
        double F[3] = {500, 1500, 2500}, FT[3] = {500, 1500, 2500}, y1[3] = {0, 0, 0}, y2[3] = {0, 0, 0};
    };
    // a signal's own sound in a channel of its own, so that two are heard at once: the one centred (0) and the one under it in
    // the wide lobe (1). Each keeps its parameters (from its seed) and its oscillators: a pulsar's thump, a comet's song, a
    // magnetosphere's whistlers and choir, a people's carrier and machines, C-16's spark, surface and pips
    struct RadarChannel {
        RadarVar v; uint64_t seed = ~0ULL;
        double clock = 0, carrier = 0, carrierF = 1200;
        double ph = 0, T = -1, thumpPh = 0, growlPh = 0, whine = 0, nullT = 0, T2 = -1, thumpPh2 = 0, growlPh2 = 0, breathLp = 0; bool half = false;   // a pulsar
        double warb = 0, warb2 = 0, geyser = -1, geyserNext = 2, cr = -1;                                                                          // a comet
        double whT = -1, whNext = 1, whPh = 0, chT = -1, chNext = 2, chPh = 0, hissY1 = 0, hissY2 = 0, choirT = -1, choirNext = 3, choirPh[3] = {0, 0, 0}; int chN = 0;   // a magnetosphere
        double roarT = -1, roarNext = 6, roarY1 = 0, roarY2 = 0, sfT = -1, sfPh = 0;
        double beacT = 0, beacPh = 0, beacEnv = 0;                                                                                                // the machines
        double dataT = 0, dataBitT = 0, dataF = 700, dataPh = 0, dataHumPh = 0; uint32_t dataLfsr = 0xACE1u; bool dataOn = false;
        double bellT = -1, bellNext = 1, bellPh[6] = {0, 0, 0, 0, 0, 0}, bellLp = 0;
        double sirT = 0, sirPh = 0, sirY1 = 0, sirY2 = 0, sirSubPh = 0;
        double murLp = 0, shoutT = -1, shoutNext = 6, numPh = 0, loopPh = 0, whBr = 0;
        size_t codeI = 0; double codeT = 0, codeEnv = 0, sparkT = 0, sparkPh = 0, sparkLp = 0; uint64_t codeLcg = 1; int codeLeft = 4, elemLeft = 2, letterLeft = 4; bool codeOn = false;
        double phonoPh = 0, phonoClickT = -1, phonoLp = 0, digPh = 0, digT = 0, digHash = 0, h2Ph = 0;
        int mathTerm = 0, mathPip = 0; double mathT = 0, mathPh = 0, mathPh2 = 0; bool mathRest = false;
        Phantom phan[4];   // the murmur's three and a raised voice (a comet's moan); under another signal, a broadcast's voice; D-03 a drifter's moan
        // a drifter's (audio_rogue.cpp): what it is as last read (its class, turning, moon and power) and what follows from it (the
        // breath's period, the moon's pulse's, the power's share), the turning's phase, the motif (its clock, which way, how loud, its
        // oscillators, a breathy whistle's resonance, its whistles hanging on), the plasma's resonators, the drones, the storm (two swells of surf and their band, an
        // S-burst train and its burst, the roar), the chorus' three chirps and their flock, the whistler, the hiss, the howl (and a brown
        // dwarf's pulse of it), the moon's arc, the iron's ring, the dynamo's rumble, the far voice's band, the beacon under it
        int rgCls = -1; double rgTurn = -1, rgMoon = -1, rgPow = -1, rgBreathS = 40, rgPulseS = 0, rgPw = 0.5;
        double rgPh = -1, rgMotT = -1, rgMotAmp = 1, rgMotPh = 0, rgMotPh2 = 0, rgMotY1 = 0, rgMotY2 = 0; bool rgMotRev = false;
        // the motif's whistles hanging on after it, the main figure's and the answer's (a metallic one's overtone; their pitch, drifting)
        double rgHang[12] = {}, rgHangPh[12] = {}, rgHangPh2[12] = {}, rgHangF[12] = {};
        double rgHowlF = 600;   // the kilometric howl's pitch, wandering
        double rgPlY[4] = {0, 0, 0, 0}, rgDrC[8] = {1, 1, 1, 1, 1, 1, 1, 1}, rgDrS[8] = {}, rgDrRc[8] = {}, rgDrRs[8] = {};   // the drones as turning phasors (each a sample's turn)
        double rgSurfT[2] = {-1, -1}, rgSurfDur[2] = {2, 2}, rgSurfA[2] = {0, 0}, rgSurfNext = 0.3, rgSurfY1 = 0, rgSurfY2 = 0, rgSurfLp = 0;
        double rgTrainT = -1, rgTrainDur = 0.5, rgTrainNext = 0.8, rgBurstT = -1, rgBurstDur = 0.01, rgBurstNext = 0, rgBurstF = 1500, rgBurstA = 1, rgBurstPh = 0, rgRoarY1 = 0, rgRoarY2 = 0;
        double rgChT[3] = {-1, -1, -1}, rgChLen[3] = {0.25, 0.25, 0.25}, rgChF0[3] = {700, 700, 700}, rgChPh[3] = {0, 0, 0}, rgFlockT = -1, rgFlockDur = 1, rgFlockNext = 1.2;
        double rgWhT = -1, rgWhDur = 1.5, rgWhNext = 2.5, rgWhPh = 0, rgHissY1 = 0, rgHissY2 = 0, rgHowlPh = 0, rgPulseT = -1, rgPulsePh = 0;
        double rgArcT = -1, rgMoonT = 0, rgArcPh = 0, rgArcY1 = 0, rgArcY2 = 0, rgRingT = -1, rgRingNext = 2, rgRingPh[4] = {0, 0, 0, 0}, rgRumLp1 = 0, rgRumLp2 = 0;
        double rgWornLp = 0, rgWornHp = 0, rgBeacLp = 0, rgBeacLp2 = 0, rgResY[4] = {0, 0, 0, 0};
        struct BandCo { double a = 0, B = 0, C = 0; } rgCo[9];   // its bands' coefficients: the fixed ones made when it changes, the moving ones every 32 samples
        double rgCoDt = 0; int rgCoN = 0;
    };
    RadarChannel rdCh[2];
    int rdKind2 = -1, rdVoice2 = -1, rdAge2 = -1; uint64_t rdSeed2 = 0; double rdPulse2 = 1;   // the under channel's, kept while it fades
    RogueVoice rdRogue2;                                                                       // a drifter's under it, likewise
    double phantomSample(Phantom& p, double dt, double sylRate, double breathy, double rough);
    double channelSample(RadarChannel& c, double dt, int kind, int voice, double pulseHz, uint64_t seed, int age, AudioState& st, bool main);
    void machineInto(double& sig, RadarChannel& c, double dt, int voice, float n2, AudioState& st, bool main);   // a people's programme's machine (C-07, C-16; D-03 a drifter's beacon too)
    double rogueSample(RadarChannel& c, double dt, const RogueVoice& R, int voice, float n2, AudioState& st, bool main);   // a drifter's voice (audio_rogue.cpp)
    double rdSetHp = 0, rdSetLp1 = 0, rdSetLp2 = 0;   // the suit's radio's band
    double radarSample(double dt, AudioState& st);
    double rdClock = 0;
    double rdEvT = -1, rdEvNext = 7, rdEvDur = 1, rdEvA = 0, rdEvB = 0, rdEvC = 0, rdEvPh = 0, rdEvLp = 0, rdEvY1 = 0, rdEvY2 = 0; int rdEvKind = -1, rdEvN = 0;   // the bed's events
    Phantom rdEvPhan[5];   // the events' voices that are no one's: the murmur's three, the moan's and the scream's; C-16's backwards, counting and choir
    double rdCueT = -1, rdCuePh = 0, rdCueLp = 0;   // the cue at a speech's start: a numbers station's chime, a loop's relay
    double rdChantAmp = 0, rdChantPh[3] = {0, 0, 0};
    std::vector<float> rdWow; size_t rdWowI = 0; double rdWowT = 0;
    // the levels (the dial, the sky, the wear, a false rise, the under signal, the heterodyne, the intermodulation, a ghost
    // word), the rumble's breath, the crackle, the galaxy's noise, the false rise's tone, the whistle, the ring, the worn bus (its
    // fades, dropouts and echoes of itself), the horn and the synthetic voice on the content
    double rdGainLvl = 0, rdSkyLvl = 0, rdWearLvl = 0, rdGhostLvl = 0, rdSig2Lvl = 0, rdHetLvl = 0, rdImLvl = 0, rdGhostWordLvl = 0;
    double rdBreathePh = 0, rdCrkT = -1, rdCrkA = 0, rdGalLp = 0, rdGalHp = 0, rdScint = 0, rdGhostF = 600, rdGhY1 = 0, rdGhY2 = 0;
    double rdHetF = 800, rdHetPh = 0, rdImPh = 0, rdFadeT = 0, rdWDrop = 1, rdWDropLvl = 1, rdWDropT = 0;
    std::vector<float> rdWorn; size_t rdWornI = 0;
    double rdHornY1 = 0, rdHornY2 = 0, rdRmPh = 0, rdShT = 0, rdShV = 0;
    // the probe's relay: the link's hiss and ticks, the air's rush, the entry's roar and crackle, the chute's flutter, the deep's
    // rumble, the hull's creaks, the thunder through the relay
    double prLvl = 0, prHp = 0, prWindLp = 0, prWindLp2 = 0, prHeatLp1 = 0, prHeatLp2 = 0, prRum1 = 0, prRum2 = 0, prTick = 0, prChutePh = 0;
    double prCreakT = -1, prCreakNext = 5, prCreakPh = 0, prCreakF = 80, prThunT = -1, prThunAmp = 0, prThunLp1 = 0, prThunLp2 = 0, prCrackle = 0;
    double probeSample(double dt, AudioState& st);
    double beepT = -1; int beepKind = 0;
    // the music: ten voices, each a note of a timbre, and the piece's clock in beats. H-05 (game/audio_music.cpp): twenty-eight voices of
    // up to thirty-two partials (their ratios, their base and current amplitudes, a per-sample decay, a swarm's own swells), the kit's
    // pieces' state, the families' filters; the music bus's limiter
    static const int MPARTS = 32;
    struct MVoice {
        bool on = false; double t = 0, gate = 0, f = 0, fFrom = 0, slideT = 0, vel = 0, lastEnv = 0;
        const Timbre* tb = nullptr; int fam = 0, kind = 0, np = 0, ctl = 0;
        double ph[MPARTS] = {}; float ratio[MPARTS] = {}, base[MPARTS] = {}, amp[MPARTS] = {}, dec[MPARTS] = {}, lfo[MPARTS] = {}, lfr[MPARTS] = {};
        double lp = 0, lp2 = 0, y1 = 0, y2 = 0, aux = 0, aux2 = 0, aux3 = 0, held = 0, wob = 0, wobT = 0; uint32_t rnd = 1;
    };
    static const int MVOICES = 28;
    MVoice mv[MVOICES];
    const Piece* mPiece = nullptr; size_t mNext = 0; double mBeat = 0, mT = 0; bool mOn = false;
    double mLimEnv = 0, mLimGain = 1;
    void musicTrigger(const Note& nt, const Tradition& T, const Ensemble& E, double bps, double dt);
    void voiceShape(MVoice& v, int sungVowel, double dt);   // a melodic voice's partials from its timbre, its pitch and its velocity (a piece's note and an instrument's)
    double musicVoice(MVoice& v, double dt);
    double musicSample(double dt, AudioState& st);
    // the instrument played in the cabin: voices of its own (a note held while its key is down), a limiter of its own
    static const int IVOICES = 12;
    MVoice iv[IVOICES]; int ivKey[IVOICES] = {}; uint32_t ivCount = 0; const Timbre* ivTimbre = nullptr;
    double iLimEnv = 0, iLimGain = 1;
    void instrumentTrigger(const AudioState::InstrumentNote& h, double dt);
    double instrumentSample(double dt, AudioState& st);
    // the voice: the segment being spoken, the glottal source, the formants' running values, four formants in cascade, a
    // nasal resonator and the noise band (the resonators' two past outputs each)
    const Speech* spSpeech = nullptr; size_t spNext = 0; int spSeg = -1; double spT = 0; bool spOn = false;
    double spF[3] = {500, 1500, 2500}, spVoiceAmp = 0, spBreathAmp = 0, spNzAmp = 0, spNzF = 3000, spNzBw = 1000, spNasal = 0;
    double spPh = 0, spSubPh = 0, spTrillPh = 0, spJit = 1, spShim = 1, spLpB = 0;
    double spY1[6] = {0, 0, 0, 0, 0, 0}, spY2[6] = {0, 0, 0, 0, 0, 0};
    double speechSample(double dt, AudioState& st);
    // the medium a recording on the decoder is heard through (game/audio_medium.cpp): its band (two high-passes, two low-passes, a
    // horn's resonance), its surface (hiss, crackle, the turning's swish), its speed (a delay line wandered by the wow and the flutter),
    // its wear (a reel's dropouts, a drive's stutters and coarse moments), the crystal's shimmer and the hum's drone; the level follows
    // whether anything plays, so the surface falls silent with the record
    int mdKind = -1; double mdLvl = 0, mdT = 0, mdHp1 = 0, mdHp2 = 0, mdLp1 = 0, mdLp2 = 0, mdLp3 = 0, mdHy1 = 0, mdHy2 = 0, mdNzLp = 0, mdNzHp = 0, mdRum = 0;
    double mdCrkT = -1, mdCrkA = 0, mdCrkNext = 0.3, mdCrkLp = 0, mdDropT = 5, mdDrop = 1, mdDropLvl = 1;
    double mdGlT = -1, mdGlDur = 0, mdGlLen = 0, mdGlNext = 6, mdGlHold = 0, mdGlStep = 0; int mdGlKind = 0;
    double mdHumPh[3] = {0, 0, 0}, mdShimPh = 0, mdShimLp = 0;
    std::vector<float> mdBuf; size_t mdI = 0;
    double mediumSample(double x, double dt, AudioState& st);
    // the samples' constants (a smoothing's coefficient, a fixed bandwidth's radius, the medium's poles) made once a block from
    // dt and the medium by the expressions that made them every sample (`coefs` in render, the medium's in audio_medium.cpp)
    struct Coefs {
        double kF, kUp, kDown, kN, rS[4], rNasal;          // the speech: its smoothings, its formants' and its nasal's radii
        double pkF, pkA, pkP, rP[3];                       // a phantom's
        double kBeac, kData, kChant, kCode, kHet;          // the radar's: a beacon's keying, a data carrier's shift, a chant, a code's keying, the heterodyne
        double mdUp, mdDown, mdDropK, mdSr;                // the medium's level, a reel's dropout, the rate
        double mdAh, mdAl, mdHornR, mdHornB, mdHissHi, mdHissLo, mdRum;   // by the medium: its band, its horn, its hiss and rumble
        double kDuneHz, kDuneHiss, kIceFar, kIceThud, kQuake;   // the dunes' note's glide and their sand's hiss, a far crack's dulling, the snow's thud, the quake's rumble
    } co{};
    void coefs(double dt);
    void mediumCoefs(double dt, int medium);
    double thunderT = -1; double thunderAmp = 0;
    std::vector<float> deckBuf; size_t deckI = 0; double duckLvl = 1;   // the deck's sequence (rendered whole as it begins), the duck's level
    double lp1 = 0, lp2 = 0, lp3 = 0, hp = 0;
    uint32_t rs = 12345;
    inline float noise() { rs = rs * 1664525u + 1013904223u; return ((rs >> 8) & 0xFFFF) / 32768.0f - 1.0f; }
};
