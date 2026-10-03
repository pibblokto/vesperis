// Procedural ambient audio: ship hum, Vimana drive, planetary wind, rain, beeps; C-04: the old peoples' music; C-05: their voices.
#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
struct Piece; struct Tradition; struct Note; struct Timbre;   // galaxy/music.h
struct Speech; struct Voice; struct Segment;                   // galaxy/voice.h

struct AudioState {
    double hum = 0;        // ship interior hum 0..1
    double engine = 0;     // vimana / approach drive 0..1
    double wind = 0;       // surface wind 0..1
    double rain = 0;       // 0..1
    double lava = 0;       // rumble 0..1
    int beep = 0;          // >0: play a beep of this kind (1 lock, 2 arrive, 3 error, 4 ui)
    double thunder = 0;    // >0 triggers thunder
    double master = 0.8;   // master volume 0..1 (settings)
    double breath = 0;     // heavy breathing 0..1 (sprint stamina)
    // M4-09 per-world sound
    double windTone = 0.5; // 0 sea (low, slow surges) .. 1 mountain whistle
    double surf = 0;       // breaking waves nearby 0..1
    double birds = 0;      // bird calls 0..1
    int step = 0;          // >0: one footstep of this kind (1 soft, 2 hard, 3 splash)
    double stepGain = 0.5;
    double leaves = 0;     // N2-06: wind through leaves and undergrowth rustle 0..1
    // N3-05 creatures
    int call = 0;          // >0: play a creature call of this kind (1 chirp, 2 low, 3 hoot, 4 click)
    double callPitch = 1;  // pitch multiplier of that call
    double hoofs = 0;      // hoof and paw steps nearby 0..1
    double insects = 0;    // insect hum at dusk near water 0..1
    // N4-04 the buggy
    double engineRpm = 0;  // 0..1 within the current gear (pitch)
    double skid = 0;       // tyre slide 0..1
    double thud = 0;       // >0: a suspension thump of this strength (one-shot)
    // R-403 the drone
    double rotor = 0;      // the thrust pods' spin 0..1 (the whine's level)
    double rotorPitch = 0; // 0..1 with the speed and the climb (the whine's pitch)
    // C-04 a piece of the old peoples' music (the decoder screen): the piece and its tradition, which must outlive the play;
    // `pieceStart` plays it from the top, a null `piece` stops it at once; the synth reports the beat it is at (-1 idle) and the end
    const Piece* piece = nullptr;
    const Tradition* tradition = nullptr;
    bool pieceStart = false, piecePause = false, pieceDone = false;
    double pieceGain = 1.0, pieceBeat = -1;
    // C-05 a text shard spoken (the decoder screen): the speech and the voice, which must outlive the play; `speechStart` speaks it
    // from the top, a null `speech` stops it at once; the synth reports the second it is at (-1 idle) and the end
    const Speech* speech = nullptr;
    const Voice* voice = nullptr;
    bool speechStart = false, speechPause = false, speechDone = false;
    double speechGain = 1.0, speechT = -1;
    // C-07 the signal radar: the receiver's static while it is on, how much of the signal in the beam comes through it, what that
    // signal is (SIG_*, -1 none: a pulsar's pulses, a comet's hiss and crackle, a magnetosphere's whistlers and chorus are the
    // synth's own; a people's recording is the piece or the speech above, which `radio` sends through the receiver's band)
    double radar = 0, radarSignal = 0;
    int radarKind = -1; double radarPulseHz = 1;
    bool radio = false;
    // the user's second call ("MORE sounds for signals, greater variety in that cosmic horror direction"): the programme a
    // people's transmission carries (RP_*, galaxy/signals.h; -1 none: the carrier alone) and the signal's seed, from which the
    // synth draws the signal's own voice (no two pulsars thump alike) and the machines' patterns; `pieceSpeed` slows the piece
    int radarVoice = -1; uint64_t radarSeed = 0; double pieceSpeed = 1;
};

class AudioSynth {
public:
    void render(float* out, int frames, int sampleRate, AudioState& st);
private:
    double phase1 = 0, phase2 = 0, lfo = 0;
    double humLvl = 0, engLvl = 0, windLvl = 0, rainLvl = 0, lavaLvl = 0, breathLvl = 0, breathPh = 0;
    double surfLvl = 0, birdLvl = 0, chirpT = -1, chirpF = 3000, nextChirp = 1, stepT = -1, stepGainCur = 0.5; int stepKind = 0; double lpS = 0, hpS = 0;
    double leavesLvl = 0, lpL = 0, hpL = 0;   // N2-06
    double callT = -1, callPitchCur = 1; int callKind = 0;   // N3-05
    double hoofLvl = 0, hoofT = 0, insectLvl = 0, lpI = 0, lpI2 = 0;
    double rpmLvl = 0, skidLvl = 0, thudT = -1, thudAmp = 0, lpK = 0;   // N4-04
    double rotorLvl = 0, rotorPitchLvl = 0, rotorPh = 0, lpR = 0;       // R-403
    // C-07 the receiver: the content's band and its ghosting (the wow, the ring, the dropouts, the carrier), the bed (the rumble,
    // the drone, the wisp, the hiss), the signals' own sounds (a pulsar's thump and growl and whine; a comet's song, geysers and ice;
    // a magnetosphere's whistlers, chorus, hiss band and choir) and the echo everything heard goes through
    double radarLvl = 0, radarSigLvl = 0, rdHp = 0, rdLp1 = 0, rdLp2 = 0, rdRing = 0, rdDrop = 1, rdDropLvl = 1, rdDropT = 0, rdCarrier = 0, rdCarrierF = 1200, rdContent = 0;
    double rdBrown = 0, rdBrown2 = 0, rdN1 = 0, rdWispY1 = 0, rdWispY2 = 0, rdWispT = 0, rdDr1 = 0, rdDr2 = 0, rdDr3 = 0;
    double rdPh = 0, rdT = -1, rdThumpPh = 0, rdGrowlPh = 0, rdWhine = 0;
    double rdWarb = 0, rdWarb2 = 0, rdGeyser = -1, rdGeyserNext = 2, rdCr = -1;
    double whT = -1, whNext = 1, whPh = 0, chT = -1, chNext = 2, chPh = 0, rdHissY1 = 0, rdHissY2 = 0, rdChoirT = -1, rdChoirNext = 3; int chN = 0; double rdChoirPh[3] = {0, 0, 0};
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
    };
    RadarVar rv; uint64_t rvSeed = ~0ULL; void rvDerive(uint64_t seed);
    struct Phantom { double ph = 0, jit = 0, f0 = 120, f0T = 120, f0Base = 120, amp = 0, ampT = 0, sylT = 0, sylDur = 0.2, lpN = 0; double F[3] = {500, 1500, 2500}, FT[3] = {500, 1500, 2500}, y1[3] = {0, 0, 0}, y2[3] = {0, 0, 0}; };
    Phantom rdPhan[5];   // 0-2 the murmur, 3 the comet's moan, 4 the static's own
    double phantomSample(Phantom& p, double dt, double sylRate, double breathy, double rough);
    double radarSample(double dt, AudioState& st);
    double rdClock = 0, rdNullT = 0, rdT2 = -1, rdThumpPh2 = 0, rdGrowlPh2 = 0, rdBreathLp = 0; bool rdHalf = false;   // a pulsar's nulling, interpulse and breath
    double rdRoarT = -1, rdRoarNext = 6, rdRoarY1 = 0, rdRoarY2 = 0, rdSfT = -1, rdSfPh = 0;   // a magnetosphere's roar and sferics
    double rdEvT = -1, rdEvNext = 7, rdEvDur = 1, rdEvA = 0, rdEvB = 0, rdEvPh = 0, rdEvLp = 0, rdEvY1 = 0, rdEvY2 = 0; int rdEvKind = -1, rdEvN = 0;   // the bed's events
    double rdBeacT = 0, rdBeacPh = 0, rdBeacEnv = 0;
    double rdDataT = 0, rdDataBitT = 0, rdDataF = 700, rdDataPh = 0, rdDataHumPh = 0; uint32_t rdDataLfsr = 0xACE1u; bool rdDataOn = false;
    double rdBellT = -1, rdBellNext = 1, rdBellPh[6] = {0, 0, 0, 0, 0, 0}, rdBellLp = 0;
    double rdSirT = 0, rdSirPh = 0, rdSirY1 = 0, rdSirY2 = 0, rdSirSubPh = 0;
    double rdMurLp = 0, rdShoutT = -1, rdShoutNext = 6;
    double rdCueT = -1, rdCuePh = 0, rdCueLp = 0;   // the cue at a speech's start: a numbers station's chime, a loop's relay
    double rdChantAmp = 0, rdChantPh[3] = {0, 0, 0}, rdNumPh = 0, rdLoopPh = 0, rdWhBr = 0;
    std::vector<float> rdWow; size_t rdWowI = 0; double rdWowT = 0;
    double beepT = -1; int beepKind = 0;
    // C-04 the music: ten voices, each a note of a timbre, and the piece's clock in beats
    struct MVoice { bool on = false; double t = 0, gate = 0, f = 0, fFrom = 0, vel = 0, lp = 0, lastEnv = 0; double ph[6] = {0, 0, 0, 0, 0, 0}; const Timbre* tb = nullptr; int kind = -1; };
    static const int MVOICES = 10;
    MVoice mv[MVOICES];
    const Piece* mPiece = nullptr; size_t mNext = 0; double mBeat = 0, mT = 0; bool mOn = false;
    void musicTrigger(const Note& nt, const Tradition& T, double bps);
    double musicSample(double dt, AudioState& st);
    // C-05 the voice: the segment being spoken, the glottal source, the formants' running values, four formants in cascade, a
    // nasal resonator and the noise band (the resonators' two past outputs each)
    const Speech* spSpeech = nullptr; size_t spNext = 0; int spSeg = -1; double spT = 0; bool spOn = false;
    double spF[3] = {500, 1500, 2500}, spVoiceAmp = 0, spBreathAmp = 0, spNzAmp = 0, spNzF = 3000, spNzBw = 1000, spNasal = 0;
    double spPh = 0, spSubPh = 0, spTrillPh = 0, spJit = 1, spShim = 1, spLpB = 0;
    double spY1[6] = {0, 0, 0, 0, 0, 0}, spY2[6] = {0, 0, 0, 0, 0, 0};
    double speechSample(double dt, AudioState& st);
    double thunderT = -1; double thunderAmp = 0;
    double lp1 = 0, lp2 = 0, lp3 = 0, hp = 0;
    uint32_t rs = 12345;
    inline float noise() { rs = rs * 1664525u + 1013904223u; return ((rs >> 8) & 0xFFFF) / 32768.0f - 1.0f; }
};
