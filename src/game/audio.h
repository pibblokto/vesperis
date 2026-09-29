// Procedural ambient audio: ship hum, Vimana drive, planetary wind, rain, beeps.
#pragma once
#include <cstdint>

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
    double beepT = -1; int beepKind = 0;
    double thunderT = -1; double thunderAmp = 0;
    double lp1 = 0, lp2 = 0, lp3 = 0, hp = 0;
    uint32_t rs = 12345;
    inline float noise() { rs = rs * 1664525u + 1013904223u; return ((rs >> 8) & 0xFFFF) / 32768.0f - 1.0f; }
};
