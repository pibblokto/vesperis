#include "audio.h"
#include "galaxy/music.h"
#include "galaxy/voice.h"
#include "galaxy/signals.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>

namespace {
constexpr double TAU_ = 6.283185307179586, PI_ = 3.141592653589793;
// C-05: the derivative of the glottal flow over one period (phase 0..1): a rise, a sharp fall at the closure, nothing while closed
inline double glottal(double p) {
    const double Tp = 0.4, Tn = 0.16;
    if (p < Tp) return 0.4 * std::sin(PI_ * p / Tp);
    if (p < Tp + Tn) return -std::sin(PI_ * (p - Tp) / (2 * Tn));
    return 0;
}
}

// C-04/H-05: the music's player (`musicTrigger`, `musicVoice`, `musicSample`) is game/audio_music.cpp

// C-05: one sample of the speech: the segment due and its sound's targets (the formants, the voicing, the breath, the noise band)
// smoothed into the running values; the glottal source (jitter and shimmer by the roughness, a tremor, a second tone under);
// four formants in cascade and a nasal resonance beside; the noise through its band. A stop's closure is silence (a voice bar
// when voiced), its burst noise decaying with aspiration after, a click a short sharp burst
double AudioSynth::speechSample(double dt, AudioState& st) {
    if (!spOn || !spSpeech || !st.voice) return 0;
    const Speech& S = *spSpeech; const Voice& V = *st.voice;
    if (st.speechPause) return 0;
    while (spNext < S.segs.size() && S.segs[spNext].t <= spT) { spSeg = (int)spNext; spNext++; }
    const Segment* sg = spSeg >= 0 && spSeg < (int)S.segs.size() ? &S.segs[spSeg] : nullptr;
    if (sg && spT >= sg->t + sg->dur) sg = nullptr;
    double tf[3] = {spF[0], spF[1], spF[2]}, tVoice = 0, tBreath = 0, tNoise = 0, nzF = 0, nzBw = 0, pitch = 1, nasalAmp = 0; bool trill = false;
    if (sg && sg->phone >= 0) {
        const Phone& p = PHONES[sg->phone];
        double u = sg->dur > 1e-9 ? (spT - sg->t) / sg->dur : 1; if (u < 0) u = 0; if (u > 1) u = 1;
        tf[0] = (p.f1 + (p.f1e - p.f1) * u) * V.tract; tf[1] = (p.f2 + (p.f2e - p.f2) * u) * V.tract; tf[2] = (p.f3 + (p.f3e - p.f3) * u) * V.tract;
        pitch = sg->pitch + (sg->pitchEnd - sg->pitch) * u;
        switch (p.kind) {
            case PK_VOWEL: tVoice = sg->amp; tBreath = V.breath; nasalAmp = V.nasal; break;
            case PK_NASAL: tVoice = 0.5 * sg->amp; tBreath = V.breath * 0.4; nasalAmp = 1.0; break;
            case PK_LIQUID: tVoice = 0.7 * sg->amp; tBreath = V.breath * 0.5; break;
            case PK_GLIDE: tVoice = 0.6 * sg->amp; tBreath = V.breath * 0.5; break;
            case PK_FRIC:
                if (p.noiseF <= 0) tBreath = p.noiseAmp * sg->amp * 1.2;   // h: breath through the formants
                else { tNoise = p.noiseAmp * sg->amp; nzF = p.noiseF; nzBw = p.noiseBw; }
                if (p.voiced) tVoice = 0.45 * sg->amp;
                break;
            case PK_STOP:
                if (sg->part == 0) tVoice = p.voiced ? 0.12 * sg->amp : 0;
                else if (sg->part == 1) { double e = std::exp(-u * (p.voiced ? 6 : 3.5)); tNoise = p.noiseAmp * sg->amp * e; nzF = p.noiseF; nzBw = p.noiseBw; tBreath = p.voiced ? 0 : (0.3 + V.breath) * (1 - u); }
                else { tNoise = 1.6 * sg->amp * std::exp(-u * 10); nzF = 2200; nzBw = 1500; }
                break;
            default: break;
        }
        trill = sg->trill;
    }
    const double kF = co.kF, kUp = co.kUp, kDown = co.kDown, kN = co.kN;
    for (int i = 0; i < 3; i++) spF[i] += (tf[i] - spF[i]) * kF;
    spVoiceAmp += (tVoice - spVoiceAmp) * (tVoice > spVoiceAmp ? kUp : kDown);
    spBreathAmp += (tBreath - spBreathAmp) * kUp;
    spNzAmp += (tNoise - spNzAmp) * kN;
    if (nzF > 0) { spNzF = nzF; spNzBw = nzBw; }
    spNasal += (nasalAmp - spNasal) * kF;
    // the source
    double f0 = V.pitch * pitch * spJit * (1 + V.tremor * 0.025 * std::sin(TAU_ * 5.5 * spT));
    spPh += dt * f0;
    if (spPh >= 1) { spPh -= 1; spJit = 1 + V.rough * 0.08 * noise(); spShim = 1 + V.rough * 0.3 * noise(); }
    double voicing = V.breath >= 0.9 ? 0.12 : 1.0;
    double src = glottal(spPh) * spShim * spVoiceAmp * voicing;
    if (V.sub > 0) { spSubPh += dt * f0 * V.subRatio; if (spSubPh >= 1) spSubPh -= 1; src += glottal(spSubPh) * V.sub * spVoiceAmp * voicing; }
    if (trill) src *= 0.55 + 0.45 * std::sin(TAU_ * spTrillPh);
    float nz = noise();
    spLpB += (nz - spLpB) * 0.25;
    double x = src + spLpB * spBreathAmp * 0.5;
    // the cascade: three formants moving and a fourth fixed, then the nasal resonance and the noise band beside
    double y = x;
    for (int i = 0; i < 4; i++) {
        double f = i < 3 ? spF[i] : 3300 * V.tract;
        double r = co.rS[i], B = 2 * r * std::cos(TAU_ * f * dt), C = -r * r, A = 1 - B - C;
        double o = A * y + B * spY1[i] + C * spY2[i];
        spY2[i] = spY1[i]; spY1[i] = o; y = o;
    }
    if (spNasal > 0.001 || std::fabs(spY1[4]) > 1e-7) { double r = co.rNasal, B = 2 * r * std::cos(TAU_ * 280 * V.tract * dt), C = -r * r, A = 1 - B - C; double o = A * src + B * spY1[4] + C * spY2[4]; spY2[4] = spY1[4]; spY1[4] = o; y += 0.8 * spNasal * o; }
    if (spNzAmp > 0.0005 || std::fabs(spY1[5]) > 1e-7) { double r = std::exp(-PI_ * spNzBw * dt), B = 2 * r * std::cos(TAU_ * spNzF * dt), C = -r * r; double o = (1 - r) * nz + B * spY1[5] + C * spY2[5]; spY2[5] = spY1[5]; spY1[5] = o; y += spNzAmp * o * 1.5; }
    spTrillPh += dt * 26; if (spTrillPh >= 1) spTrillPh -= 1;
    spT += dt;
    if (spNext >= S.segs.size() && spT >= S.seconds) { spOn = false; st.speechDone = true; }
    return std::tanh(y * 0.5) * 0.65 * st.speechGain;
}

// ---- C-07 the receiver (the radar camera) ----
namespace {
inline double u01(uint64_t h) { return (h >> 11) * (1.0 / 9007199254740992.0); }
const double VOWELS[7][3] = {{700, 1200, 2600}, {500, 1900, 2600}, {300, 2300, 3000}, {450, 800, 2500}, {320, 700, 2400}, {500, 1400, 2500}, {250, 1000, 2000}};
const double CHOIRS[4][3] = {{220, 226.5, 233}, {220, 330, 440}, {220, 223, 228}, {110, 220, 221.5}};   // quarter-tones, fifths and an octave, a cluster, octaves
const double BELL_PARTIALS[6] = {1.0, 1.19, 1.56, 2.0, 2.51, 2.66};
const double CLUSTER[4] = {1.0, 1.0595, 1.1225, 0.5};   // C-16: the choir in the static, a cluster of semitones over an octave down
inline double resonate(double x, double f, double bw, double dt, double& y1, double& y2) {   // Klatt's resonator, unity at the centre
    double r = std::exp(-PI_ * bw * dt), B = 2 * r * std::cos(TAU_ * f * dt), C = -r * r;
    double o = (1 - r) * x + B * y1 + C * y2; y2 = y1; y1 = o; return o;
}
// C-16: the beacon that is mathematics. Term `k` of a sequence as a group of `n` pips (a count in binary: `bits` the ones, sent high;
// the harmonic series: one pip at `tone` times the beacon's pitch); false past what a beacon sends before it rests and begins again
bool mathsGroup(int kind, int k, int& n, uint32_t& bits, double& tone) {
    static const int PRIMES[9] = {2, 3, 5, 7, 11, 13, 17, 19, 23};
    static const int FIB[8] = {1, 1, 2, 3, 5, 8, 13, 21};
    static const int DIGITS[12] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8};   // a constant's (pi's), a pip a unit
    bits = 0; tone = 1;
    switch (kind) {
        case 0: if (k >= 9) return false; n = PRIMES[k]; return true;                  // the primes
        case 1: if (k >= 5) return false; n = (k + 1) * (k + 1); return true;          // the squares
        case 2: if (k >= 8) return false; n = FIB[k]; return true;                     // Fibonacci's
        case 3: if (k >= 6) return false; n = 1 << k; return true;                     // the powers of two
        case 4: if (k >= 6) return false; n = (k + 1) * (k + 2) / 2; return true;      // the triangles
        case 5: if (k >= 12) return false; n = 1; tone = k + 1; return true;           // the harmonic series, a pitch a term
        case 6: { if (k >= 15) return false; n = 0; while (((k + 1) >> n) > 0) n++; bits = (uint32_t)(k + 1); return true; }   // a count in binary
        default: if (k >= 12) return false; n = DIGITS[k]; return true;
    }
}
inline bool speakingProgramme(int p) { return p == RP_VOICE || p == RP_WHISPERS || p == RP_CHANT || p == RP_NUMBERS || p == RP_LOOP || p == RP_MUSIC || p == RP_PHONOGRAPH || p == RP_SYNTHETIC; }
}

// the signal's own parameters from its seed: no two pulsars thump alike, no two comets sing alike, no two beacons share a pattern
void AudioSynth::rvDerive(RadarVar& v, uint64_t seed) {
    auto u = [&](int k) { return u01(mix64(seed ^ ((uint64_t)k * 0x9E3779B97F4A7C15ULL) ^ 0xC07ULL)); };
    v = RadarVar();
    v.thumpF = 45 + 50 * u(1); v.interpulse = u(2) < 0.5 ? 0 : 0.3 + 0.4 * u(3); v.nulling = u(4) < 0.3; v.nullOn = 6 + 12 * u(5); v.nullOff = 2 + 4 * u(6);
    v.growl = 0.4 + 1.1 * u(7); v.whineF = 1700 + 1500 * u(8); v.breath = u(9) < 0.45;
    static const double RATIOS[5] = {1.0595, 1.03, 1.5, 0.5, 1.4142};   // a semitone, a quarter-tone, a fifth, an octave under, a tritone
    v.songF = 60 + 70 * u(11); v.ratio = RATIOS[(int)(u(12) * 4.99)]; v.glide = u(13) < 0.4; v.geyserRate = 0.5 + 1.5 * u(14); v.crackle = 0.3 + 1.7 * u(15); v.moan = u(16) < 0.4; v.moanF = 80 + 90 * u(17);
    v.whistlerRate = 0.6 + 1.4 * u(21); v.chorusRate = 0.6 + 1.4 * u(22); v.sferics = u(23) < 0.5; v.sfRate = 1 + 4 * u(24); v.choirSet = (int)(u(25) * 3.99); v.choirRate = 0.7 + 0.8 * u(26); v.hissF = 300 + 600 * u(27); v.roar = u(28) < 0.35;
    v.beacN = 3 + (int)(u(31) * 4.99); for (int i = 0; i < 7; i++) v.beacF[i] = 500 * std::pow(2.0, 1.6 * u(40 + i));
    v.beacDur = 0.12 + 0.25 * u(32); v.beacGap = 0.08 + 0.2 * u(33); v.beacRest = 2 + 5 * u(34); v.beacSq = 0.15 + 0.35 * u(35); v.beacSag = u(36) < 0.5;
    v.dataF1 = 550 + 350 * u(51); v.dataF2 = v.dataF1 + 120 + 300 * u(52); v.dataRate = 12 + 50 * u(53); v.dataBurst = 0.8 + 2.5 * u(54); v.dataGap = 0.6 + 3.5 * u(55); v.dataHum = u(56) < 0.5 ? 50 : 60;
    v.bellF = 160 + 200 * u(61); v.bellPeriod = 3 + 5 * u(62); v.bellDecay = 3 + 4 * u(63); v.bellFar = 0.1 + 0.25 * u(64);
    v.sirenTwo = u(71) < 0.45; v.sirenF = 280 + 240 * u(72); v.sirenRatio = 1.25 + 0.4 * u(73); v.sirenCycle = v.sirenTwo ? 0.4 + 0.5 * u(74) : 1.2 + 2.0 * u(74); v.sirenSwell = 9 + 11 * u(75);
    v.murRate = 3 + 1.5 * u(81); v.murF0[0] = 95 + 35 * u(82); v.murF0[1] = 130 + 50 * u(83); v.murF0[2] = 180 + 70 * u(84);
    v.sparkHz = 260 + 480 * u(101); v.ringF = 1100 + 1400 * u(102); v.quench = u(103) < 0.5; v.morseUnit = 0.055 + 0.05 * u(104); v.sparkDecay = 2200 + 2800 * u(105);   // C-16
    v.turnHz = 1.25 + 0.1 * u(111); v.hornF = 850 + 450 * u(112); v.surface = 0.5 + u(113);
    v.ringHz = 38 + 34 * u(121); v.holdHz = 4500 + 3000 * u(122); v.whineHz = 3200 + 1400 * u(131);
    v.mathKind = (int)(u(141) * 7.99); v.mathF = 330 * std::pow(2.0, 1.4 * u(142)); v.mathPip = 0.05 + 0.035 * u(143); v.mathGap = 0.07 + 0.06 * u(144); v.mathRest = 0.55 + 0.5 * u(145);
    v.carrier2F = 1200 + 300 * (u(151) - 0.5);
}

const char* AudioSynth::mathsName(uint64_t seed) {
    static const char* N[8] = {"the primes", "the squares", "Fibonacci's numbers", "the powers of two", "the triangular numbers", "the harmonic series", "a count in binary", "the digits of pi"};
    RadarVar v; rvDerive(v, seed); return N[std::max(0, std::min(7, v.mathKind))];
}

// a phantom: a voice that is no one's. A glottal pulse (or breath) through three formants; with a syllable rate it babbles on its
// own (a new vowel, loudness and pitch each syllable, a third of them silent), without one the caller moves its targets
double AudioSynth::phantomSample(Phantom& p, double dt, double sylRate, double breathy, double rough) {
    if (sylRate > 0) {
        p.sylT += dt;
        if (p.sylT >= p.sylDur) {
            p.sylT = 0; p.sylDur = (0.6 + 0.8 * (0.5 + 0.5 * noise())) / sylRate;
            int v = (int)((0.5 + 0.5 * noise()) * 6.99); for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[v][i];
            p.ampT = 0.5 + 0.5 * noise() < 0.3 ? 0 : 0.5 + 0.5 * (0.5 + 0.5 * noise());
            p.f0T = p.f0Base * (0.85 + 0.3 * (0.5 + 0.5 * noise()));
        }
    }
    const double kF = co.pkF, kA = co.pkA, kP = co.pkP;
    for (int i = 0; i < 3; i++) p.F[i] += (p.FT[i] - p.F[i]) * kF;
    p.amp += (p.ampT - p.amp) * kA; p.f0 += (p.f0T - p.f0) * kP;
    p.ph += dt * p.f0 * (1 + p.jit); if (p.ph >= 1) { p.ph -= 1; p.jit = rough * 0.08 * noise(); }
    float nz = noise(); p.lpN += (nz - p.lpN) * 0.25;
    double src = glottal(p.ph) * p.amp * (1 - 0.7 * breathy) + p.lpN * breathy * p.amp * 0.6;
    double y = src;
    for (int i = 0; i < 3; i++) { double r = co.rP[i], B = 2 * r * std::cos(TAU_ * p.F[i] * dt), C = -r * r, A = 1 - B - C; double o = A * y + B * p.y1[i] + C * p.y2[i]; p.y2[i] = p.y1[i]; p.y1[i] = o; y = o; }
    return std::tanh(y * 0.5) * 0.65;
}

// one sample of a signal's own sound in its channel: by its kind and its seed, and a people's carrier (by the age, C-16) and its
// programme's machine (the hum under a chant and the cue at a loop's or a numbers station's start are the centred one's: `main`).
// Under another (C-16), a people's speaking programme is the shape of a voice: one of the channel's phantoms babbling
double AudioSynth::channelSample(RadarChannel& c, double dt, int kind, int voice, double pulseHz, uint64_t seed, int age, AudioState& st, bool main) {
    if (seed != c.seed) {   // a signal of its own: its parameters, and the keying and the count from their tops
        rvDerive(c.v, seed); c.seed = seed;
        c.codeI = 0; c.codeT = 0; c.codeLcg = seed | 1; c.codeLeft = 4; c.elemLeft = 2; c.letterLeft = 4; c.codeOn = false;
        c.mathTerm = 0; c.mathPip = 0; c.mathT = 0; c.mathRest = false;
    }
    const RadarVar& v = c.v;
    float n2 = noise();
    double sig = 0;
    c.clock += dt;
    auto uni = [&]() { return 0.5 + 0.5 * noise(); };
    switch (kind) {
        case 0: {   // a people's transmission: the carrier, and the programme on the air
            // C-16: the carrier by the age. The modern age's is C-07's (a tone that drifts under the voice, beating slowly); the late
            // industrial age has none, only the spark's rasp, faint, between the keying; the futuristic a thin whine high up that
            // flutters as data runs under it; the far future a pure tone that never moves (the hydrogen line's 1,420, in the audio)
            switch (age) {
                case AGE_INDUSTRIAL: { c.sparkLp += (n2 - c.sparkLp) * 0.3; sig += 0.03 * c.sparkLp * (0.5 + 0.5 * std::sin(TAU_ * 0.6 * c.clock)); break; }
                case AGE_FUTURISTIC: {
                    c.digPh += dt * v.whineHz * (1 + 0.003 * std::sin(TAU_ * 0.4 * c.clock));
                    c.digT -= dt; if (c.digT <= 0) { c.digT = 0.012 + 0.03 * uni(); c.digHash = uni() < 0.5 ? 0.3 : 1.0; }
                    sig += 0.022 * c.digHash * std::sin(TAU_ * c.digPh);
                    break;
                }
                case AGE_FAR_FUTURE: { c.h2Ph += dt * 1420.0; sig += 0.02 * std::sin(TAU_ * c.h2Ph); break; }
                default: {
                    double base = main ? 1200 : v.carrier2F;
                    c.carrierF += (base + 40 * std::sin(lfo * TAU_ * 0.17) - c.carrierF) * 0.001; c.carrier += dt * c.carrierF;
                    sig += 0.04 * std::sin(TAU_ * c.carrier) * (0.6 + 0.4 * std::sin(lfo * TAU_ * 2.3));
                }
            }
            switch (voice) {
                case RP_BEACON: {   // a beacon: its pattern of tones, a rest, again; half of them sag a little, dying
                    double cyc = v.beacN * (v.beacDur + v.beacGap) + v.beacRest;
                    c.beacT += dt; if (c.beacT >= cyc) c.beacT -= cyc;
                    int k = (int)(c.beacT / (v.beacDur + v.beacGap)); double tk = c.beacT - k * (v.beacDur + v.beacGap);
                    bool on = k < v.beacN && tk < v.beacDur;
                    c.beacEnv += ((on ? 1.0 : 0.0) - c.beacEnv) * co.kBeac;
                    double f = v.beacF[std::min(k, 6)] * (v.beacSag ? 0.985 + 0.015 * std::cos(TAU_ * 0.03 * c.clock) : 1.0);
                    c.beacPh += dt * f; double ph = c.beacPh - std::floor(c.beacPh);
                    sig += 0.24 * c.beacEnv * ((1 - v.beacSq) * std::sin(TAU_ * ph) + v.beacSq * (ph < 0.5 ? 1 : -1));
                    break;
                }
                case RP_DATA: {   // data: bursts of two tones switched by a shift register, a buzz of the mains under them
                    c.dataT -= dt;
                    if (c.dataT <= 0) { c.dataOn = !c.dataOn; c.dataT = c.dataOn ? v.dataBurst * (0.7 + 0.6 * uni()) : v.dataGap * (0.6 + 0.8 * uni()); }
                    if (c.dataOn) {
                        c.dataBitT += dt * v.dataRate;
                        if (c.dataBitT >= 1) { c.dataBitT -= 1; uint32_t b = ((c.dataLfsr >> 0) ^ (c.dataLfsr >> 2) ^ (c.dataLfsr >> 3) ^ (c.dataLfsr >> 5)) & 1u; c.dataLfsr = (c.dataLfsr >> 1) | (b << 15); }
                        double want = (c.dataLfsr & 1u) ? v.dataF2 : v.dataF1; c.dataF += (want - c.dataF) * co.kData;
                        c.dataPh += dt * c.dataF; double ph = c.dataPh - std::floor(c.dataPh);
                        sig += 0.17 * (0.55 * std::sin(TAU_ * ph) + 0.45 * (ph < 0.5 ? 1 : -1));
                        c.dataHumPh += dt * v.dataHum; double hp = c.dataHumPh - std::floor(c.dataHumPh);
                        sig += 0.09 * (2 * hp - 1) * (0.7 + 0.3 * std::sin(TAU_ * 0.9 * c.clock));
                    }
                    break;
                }
                case RP_TOLL: {   // a bell tolling far off: six partials, the period a little irregular, now and then struck twice
                    c.bellNext -= dt;
                    if (c.bellNext <= 0) { c.bellT = 0; c.bellNext = uni() < 0.2 ? 0.6 + 0.4 * uni() : v.bellPeriod * (0.8 + 0.4 * uni()); }
                    if (c.bellT >= 0) {
                        double b = 0;
                        for (int i = 0; i < 6; i++) { c.bellPh[i] += dt * v.bellF * BELL_PARTIALS[i] * (i == 3 ? 1.003 : 1.0); b += std::sin(TAU_ * c.bellPh[i]) * std::exp(-c.bellT * (0.7 + 0.45 * i)) / (1 + 0.3 * i); }
                        b *= std::exp(-c.bellT / v.bellDecay) * (c.bellT < 0.004 ? c.bellT / 0.004 : 1.0);
                        c.bellLp += (b - c.bellLp) * v.bellFar;
                        sig += 0.55 * c.bellLp;
                        c.bellT += dt; if (c.bellT > v.bellDecay * 3) c.bellT = -1;
                    }
                    break;
                }
                case RP_SIREN: {   // a siren still running: a slow glide or two tones, a sawtooth through a resonance, carried in and out as if on a wind
                    c.sirT += dt;
                    double f;
                    if (v.sirenTwo) f = ((int)(c.sirT / v.sirenCycle) % 2) ? v.sirenF * v.sirenRatio : v.sirenF;
                    else { double ph = std::fmod(c.sirT, 2 * v.sirenCycle) / v.sirenCycle; double tri = ph < 1 ? ph : 2 - ph; f = v.sirenF * std::pow(v.sirenRatio, tri); }
                    c.sirPh += dt * f; double saw = 2 * (c.sirPh - std::floor(c.sirPh)) - 1;
                    double o = resonate(saw, f * 3, f * 0.6, dt, c.sirY1, c.sirY2);
                    c.sirSubPh += dt * f * 0.5;
                    double wind = std::pow(0.5 + 0.5 * std::sin(TAU_ * c.clock / v.sirenSwell), 2.0);
                    sig += (0.5 * o + 0.1 * std::sin(TAU_ * c.sirSubPh)) * (0.2 + 0.8 * wind);
                    break;
                }
                case RP_MURMUR: {   // a murmur of voices behind a wall, one of them raised now and then
                    double m = 0;
                    for (int i = 0; i < 3; i++) { if (c.phan[i].f0Base != v.murF0[i]) { c.phan[i] = Phantom(); c.phan[i].f0Base = v.murF0[i]; c.phan[i].f0 = c.phan[i].f0T = v.murF0[i]; } m += phantomSample(c.phan[i], dt, v.murRate * (i == 0 ? 1 : (i == 1 ? 1.12 : 0.9)), 0.25, 0.3); }
                    c.shoutNext -= dt;
                    if (c.shoutNext <= 0 && c.shoutT < 0) { c.shoutT = 0; c.shoutNext = 6 + 9 * uni(); c.phan[3] = Phantom(); c.phan[3].f0Base = v.murF0[2] * 1.3; c.phan[3].f0 = c.phan[3].f0T = c.phan[3].f0Base; }
                    if (c.shoutT >= 0) { double u = c.shoutT / 0.7; if (u >= 1) c.shoutT = -1; else { m += 1.6 * std::sin(PI_ * u) * phantomSample(c.phan[3], dt, 6, 0.1, 0.8); c.shoutT += dt; } }
                    c.murLp += (m - c.murLp) * 0.18;
                    sig += 0.9 * c.murLp;
                    break;
                }
                case RP_CHANT: {   // a chant: a hum of three voices an octave, a unison and a fifth round the chanter's pitch, swelling with it
                    if (!main) break;
                    double p = st.voice ? st.voice->pitch : 110;
                    rdChantAmp += (spVoiceAmp - rdChantAmp) * co.kChant;
                    double vib = 1 + 0.008 * std::sin(TAU_ * 4.3 * c.clock);
                    rdChantPh[0] += dt * p * 0.5 * vib; rdChantPh[1] += dt * p * 1.003 * vib; rdChantPh[2] += dt * p * 1.5 * vib;
                    sig += 0.2 * rdChantAmp * (std::sin(TAU_ * rdChantPh[0]) + 0.6 * std::sin(TAU_ * rdChantPh[1]) + 0.4 * std::sin(TAU_ * rdChantPh[2]));
                    break;
                }
                case RP_WHISPERS: {   // whispers: someone breathing close to the microphone
                    c.whBr += (n2 - c.whBr) * 0.06;
                    double env = std::pow(std::max(0.0, std::sin(TAU_ * 0.28 * c.clock)), 2.5);
                    sig += 0.5 * env * c.whBr;
                    break;
                }
                case RP_NUMBERS: {   // a numbers station: a carrier with the mains on it
                    c.numPh += dt * 400; sig += 0.05 * std::sin(TAU_ * c.numPh) * (0.75 + 0.25 * std::sin(TAU_ * 50 * c.clock));
                    break;
                }
                case RP_LOOP: {   // a loop: the mechanism's whir while the phrase runs, tape hiss under
                    c.loopPh += dt * 95; c.sparkLp += (n2 - c.sparkLp) * 0.6;
                    sig += (main && spOn ? 0.05 : 0.0) * (2 * (c.loopPh - std::floor(c.loopPh)) - 1) + 0.05 * c.sparkLp;
                    break;
                }
                case RP_MORSE: {   // C-16: spark-gap Morse. A spark at the gap's rate, each a damped ring and a crack, keyed in the people's code:
                    // the game's keying for the centred signal (from its top when the programme begins), a hand of the synth's own under another
                    bool on = false;
                    if (main && st.radarCode && !st.radarCode->empty()) {
                        const std::vector<uint8_t>& code = *st.radarCode; double unitS = std::max(0.02, st.radarCodeUnit);
                        if (c.codeI < code.size()) {
                            uint8_t e = code[c.codeI];
                            on = !(e & MORSE_OFF);
                            c.codeT += dt;
                            if (c.codeT >= (e & 0x7F) * unitS) { c.codeT -= (e & 0x7F) * unitS; c.codeI++; }
                        }
                    } else {   // letters of one to four elements, a word every three to six letters
                        c.codeT += dt;
                        if (c.codeT >= c.codeLeft * v.morseUnit) {
                            c.codeT = 0;
                            auto next = [&]() { c.codeLcg = c.codeLcg * 6364136223846793005ULL + 1442695040888963407ULL; return (int)(c.codeLcg >> 33); };
                            if (c.codeOn) {
                                c.codeOn = false;
                                if (--c.elemLeft > 0) c.codeLeft = 1;
                                else if (--c.letterLeft > 0) { c.codeLeft = 3; c.elemLeft = 1 + next() % 4; }
                                else { c.codeLeft = 7; c.letterLeft = 3 + next() % 4; c.elemLeft = 1 + next() % 4; }
                            } else { c.codeOn = true; c.codeLeft = (next() & 1) ? 3 : 1; }
                        }
                        on = c.codeOn;
                    }
                    c.codeEnv += ((on ? 1.0 : 0.0) - c.codeEnv) * co.kCode;
                    c.sparkT += dt;
                    double period = 1.0 / v.sparkHz;
                    if (c.sparkT >= period) { c.sparkT -= period * (v.quench ? 1.0 : 0.92 + 0.16 * uni()); c.sparkPh = 0; }   // a quenched gap's sparks even (a note), a plain one's ragged
                    c.sparkPh += dt * v.ringF;
                    if (c.codeEnv > 0.001) {
                        double ring = std::exp(-c.sparkT * (v.quench ? 0.5 : 1.0) * v.sparkDecay) * std::sin(TAU_ * c.sparkPh);
                        double crack = n2 * std::exp(-c.sparkT * v.sparkDecay * 3);
                        sig += 0.6 * c.codeEnv * (ring + (v.quench ? 0.25 : 0.7) * crack);
                    }
                    break;
                }
                case RP_PHONOGRAPH: {   // C-16: the record's surface under the recording: a hiss, a crackle, a tick at every turn, the motor
                    c.phonoPh += dt * v.turnHz;
                    if (c.phonoPh >= 1) { c.phonoPh -= 1; c.phonoClickT = 0; }
                    if (c.phonoClickT >= 0) { sig += 0.5 * v.surface * n2 * std::exp(-c.phonoClickT * 700); c.phonoClickT += dt; if (c.phonoClickT > 0.012) c.phonoClickT = -1; }
                    if (c.cr < 0 && uni() < dt * 10 * v.surface) c.cr = 0;
                    if (c.cr >= 0) { sig += 0.4 * n2 * std::exp(-c.cr * 1500); c.cr += dt; if (c.cr > 0.005) c.cr = -1; }
                    c.phonoLp += (n2 - c.phonoLp) * 0.2;
                    sig += 0.12 * v.surface * c.phonoLp + 0.05 * std::sin(TAU_ * 30 * c.clock) * (0.6 + 0.4 * std::sin(TAU_ * v.turnHz * c.clock));
                    break;
                }
                case RP_MATHS: {   // C-16: the beacon that is mathematics: a group of pure pips a term, a pause, the next term; a rest, again
                    c.mathT += dt;
                    if (c.mathRest) { if (c.mathT >= 2.5 + 3 * v.mathRest) { c.mathRest = false; c.mathT = 0; c.mathTerm = 0; c.mathPip = 0; } break; }
                    int n; uint32_t bits; double tone;
                    if (!mathsGroup(v.mathKind, c.mathTerm, n, bits, tone)) { c.mathRest = true; c.mathT = 0; break; }
                    double pip = v.mathKind == 5 ? 0.32 : v.mathPip, gap = v.mathKind == 5 ? 0.14 : v.mathGap;
                    if (n > 8) { double s = std::max(0.45, std::min(1.0, 4.0 / (n * (pip + gap)))); pip *= s; gap *= s; }   // a long group quicker: four seconds at most
                    if (c.mathPip < n) {
                        double f = v.mathF * (v.mathKind == 5 ? 0.25 * tone : 1.0);
                        if (v.mathKind == 6 && ((bits >> (n - 1 - c.mathPip)) & 1u)) f *= 1.5;   // a one high, a nought low
                        double env = c.mathT < pip ? std::sin(PI_ * c.mathT / pip) : 0;
                        c.mathPh += dt * f; c.mathPh2 += dt * f * 2;
                        sig += 0.42 * env * (std::sin(TAU_ * c.mathPh) + 0.2 * std::sin(TAU_ * c.mathPh2));
                        if (c.mathT >= pip + gap) { c.mathT = 0; c.mathPip++; }
                    } else if (c.mathT >= v.mathRest) { c.mathT = 0; c.mathPip = 0; c.mathTerm++; }
                    break;
                }
                default: break;
            }
            if (!main && speakingProgramme(voice)) {   // C-16: under another, a broadcast's voice is the shape of one, babbling
                Phantom& p = c.phan[0];
                if (p.f0Base != v.murF0[1]) { p = Phantom(); p.f0Base = v.murF0[1]; p.f0 = p.f0T = p.f0Base; }
                double x = phantomSample(p, dt, v.murRate, voice == RP_WHISPERS ? 0.95 : 0.2, voice == RP_SYNTHETIC ? 0.0 : 0.3);
                if (voice == RP_SYNTHETIC) x *= 0.4 + 0.6 * std::sin(TAU_ * v.ringHz * c.clock);
                sig += 0.8 * x;
            }
            if (main && rdCueT >= 0) {   // the cue at a speech's start: a numbers station's chime (three notes falling), a loop's relay and blip
                if (voice == RP_NUMBERS) {
                    int k = (int)(rdCueT / 0.2); double tk = rdCueT - k * 0.2;
                    if (k < 3) { double f = k == 0 ? 1320 : (k == 1 ? 1100 : 880); rdCuePh += dt * f; sig += 0.2 * std::exp(-tk * 7) * std::sin(TAU_ * rdCuePh); }
                    else rdCueT = -1;
                } else if (voice == RP_LOOP) {
                    rdCueLp += (n2 - rdCueLp) * 0.3;
                    if (rdCueT < 0.012) sig += 0.9 * rdCueLp;
                    else if (rdCueT < 0.13) { rdCuePh += dt * 1000; sig += 0.12 * std::sin(TAU_ * rdCuePh); }
                    else rdCueT = -1;
                } else rdCueT = -1;
                if (rdCueT >= 0) rdCueT += dt;
            }
            break;
        }
        case 1: {   // a pulsar: a thump far down at its rate, a growl falling away after each, a thin whine that circles; by its seed an interpulse half a period on, spells of silence (a nulling one), a breath drawn before each thump (the slow ones)
            double rate = std::max(0.2, pulseHz);
            double gate = 1;
            if (v.nulling) { c.nullT += dt; double cyc = v.nullOn + v.nullOff; if (std::fmod(c.nullT, cyc) >= v.nullOn) gate = 0; }
            c.ph += dt * rate;
            if (c.ph >= 1) { c.ph -= 1; c.half = false; if (gate > 0) c.T = 0; }
            if (v.interpulse > 0 && c.ph >= 0.5 && !c.half) { c.half = true; if (gate > 0) c.T2 = 0; }
            auto thump = [&](double& T, double& ph, double& gph, double amp) {
                if (T < 0) return;
                double env = std::exp(-T * 9), f = v.thumpF * std::exp(-T * 2.5) + 28;
                ph += dt * f; sig += 0.75 * amp * env * std::sin(TAU_ * ph);
                double fg = 160 * std::exp(-T * 1.5) + 50; gph += dt * fg; double saw = 2 * (gph - std::floor(gph)) - 1;
                sig += amp * (0.16 * v.growl * std::exp(-T * 4) * saw + 0.5 * std::exp(-T * 60) * n2);
                T += dt; if (T > 1.2) T = -1;
            };
            thump(c.T, c.thumpPh, c.growlPh, 1.0);
            thump(c.T2, c.thumpPh2, c.growlPh2, v.interpulse);
            if (v.breath && rate < 1.3) { double env = c.ph > 0.55 ? std::pow((c.ph - 0.55) / 0.45, 2.0) : 0; c.breathLp += (n2 - c.breathLp) * 0.08; sig += 0.9 * env * gate * c.breathLp; }
            c.whine += dt * (v.whineF + 300 * std::sin(lfo * TAU_ * 0.8));
            sig += 0.03 * std::sin(TAU_ * c.whine) * (0.5 + 0.5 * std::sin(lfo * TAU_ * 0.33));
            break;
        }
        case 2: {   // a comet: its song (a warble far down with a second voice an interval off, by its seed), geysers of hiss, ice cracking, a rumble; some glide, some moan
            double wob = 1 + 0.12 * std::sin(lfo * TAU_ * 1.9) + 0.05 * std::sin(lfo * TAU_ * 0.27);
            double gl = v.glide ? 1 + 0.2 * std::sin(TAU_ * 0.07 * c.clock) * std::sin(TAU_ * 0.031 * c.clock + 0.7) : 1;
            c.warb += dt * v.songF * wob * gl; c.warb2 += dt * v.songF * v.ratio * wob * gl;
            double songEnv = 0.5 + 0.5 * std::sin(lfo * TAU_ * 0.09);
            sig += 0.26 * songEnv * (std::sin(TAU_ * c.warb) + 0.6 * std::sin(TAU_ * c.warb2) + 0.3 * std::sin(TAU_ * c.warb * 2));
            c.geyserNext -= dt;
            if (c.geyserNext <= 0 && c.geyser < 0) { c.geyser = 0; c.geyserNext = (2 + 4 * uni()) / v.geyserRate; }
            c.sparkLp += (n2 - c.sparkLp) * 0.6;
            if (c.geyser >= 0) { double u = c.geyser / 1.4; if (u >= 1) c.geyser = -1; else { sig += 0.7 * std::sin(PI_ * u) * c.sparkLp; c.geyser += dt; } }
            sig += 2.5 * rdBrown2;
            if (c.cr < 0 && uni() < dt * 4 * v.crackle) c.cr = 0;
            if (c.cr >= 0) { sig += 0.7 * n2 * std::exp(-c.cr * 400); c.cr += dt; if (c.cr > 0.03) c.cr = -1; }
            if (v.moan) {
                Phantom& p = c.phan[3];
                p.f0T = v.moanF * gl * (1 + 0.03 * std::sin(TAU_ * 0.13 * c.clock)); p.ampT = 0.35 + 0.35 * std::sin(TAU_ * 0.05 * c.clock);
                double m = 0.5 + 0.5 * std::sin(TAU_ * 0.09 * c.clock); for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[3][i] * (1 - m) + VOWELS[0][i] * m;
                sig += 0.45 * phantomSample(p, dt, 0, 0.35, 0.5);
            }
            break;
        }
        case 3: {   // a magnetosphere: whistlers falling slowly from 2 kHz, bursts of chorus, a band of hiss that breathes, and a choir of three voices (by its seed quarter-tones, fifths, a cluster or octaves) that comes and goes; some crack with lightning (sferics and tweeks), some roar
            c.whNext -= dt;
            if (c.whNext <= 0 && c.whT < 0) { c.whT = 0; c.whNext = (2.0 + 4.0 * uni()) / v.whistlerRate; }
            if (c.whT >= 0) { double u = c.whT / 1.8; if (u >= 1) c.whT = -1; else { double f = 2200 * std::exp(-u * 2.6) + 150; c.whPh += dt * f; sig += 0.45 * std::sin(PI_ * u) * std::sin(TAU_ * c.whPh); c.whT += dt; } }
            c.chNext -= dt;
            if (c.chNext <= 0 && c.chT < 0) { c.chT = 0; c.chN = 2 + (int)(3.99 * uni()); c.chNext = (4 + 6.0 * uni()) / v.chorusRate; }
            if (c.chT >= 0) { const double per = 0.26; int kk = (int)(c.chT / per); if (kk >= c.chN) c.chT = -1; else { double u = c.chT / per - kk; double f = 700 * std::pow(2.5, u); c.chPh += dt * f; sig += 0.3 * std::sin(PI_ * u) * std::sin(TAU_ * c.chPh); c.chT += dt; } }
            sig += 0.4 * resonate(n2, v.hissF, 120, dt, c.hissY1, c.hissY2) * (0.5 + 0.5 * std::sin(lfo * TAU_ * 0.4));
            if (v.sferics) {
                if (c.sfT < 0 && uni() < dt * v.sfRate) { c.sfT = 0; c.sfPh = 0; }
                if (c.sfT >= 0) { double f = 2200 * std::exp(-c.sfT * 4) + 1500; c.sfPh += dt * f; sig += 0.9 * n2 * std::exp(-c.sfT * 600) + 0.25 * std::exp(-c.sfT * 30) * std::sin(TAU_ * c.sfPh); c.sfT += dt; if (c.sfT > 0.25) c.sfT = -1; }
            }
            if (v.roar) {
                c.roarNext -= dt;
                if (c.roarNext <= 0 && c.roarT < 0) { c.roarT = 0; c.roarNext = 6 + 8 * uni(); }
                if (c.roarT >= 0) { double u = c.roarT / 1.6; if (u >= 1) c.roarT = -1; else { sig += 0.6 * std::sin(PI_ * u) * resonate(n2, 300, 150, dt, c.roarY1, c.roarY2) * (0.6 + 0.4 * std::sin(TAU_ * 28 * c.roarT)); c.roarT += dt; } }
            }
            c.choirNext -= dt;
            if (c.choirNext <= 0 && c.choirT < 0) { c.choirT = 0; c.choirNext = (8 + 6 * uni()) / v.choirRate; }
            if (c.choirT >= 0) {
                double u = c.choirT / 7.0;
                if (u >= 1) c.choirT = -1;
                else {
                    double env = std::sin(PI_ * u), vib = 1 + 0.01 * std::sin(TAU_ * 5.3 * c.choirT);
                    const double* ch = CHOIRS[v.choirSet];
                    c.choirPh[0] += dt * ch[0] * vib; c.choirPh[1] += dt * ch[1] * vib; c.choirPh[2] += dt * ch[2] * vib;
                    sig += 0.13 * env * (std::sin(TAU_ * c.choirPh[0]) + std::sin(TAU_ * c.choirPh[1]) + std::sin(TAU_ * c.choirPh[2]));
                    c.choirT += dt;
                }
            }
            break;
        }
        default: break;
    }
    return sig;
}

// one sample of the receiver: the bed of the void (and, now and then, something in it), the signal centred and (C-16) the one under
// it, the content ghosted above, the heterodyne between two carriers and the intermodulation's ring, the far signal worn, all
// through an echo. `rdContent` was set by the caller from the piece or the speech on the air
double AudioSynth::radarSample(double dt, AudioState& st) {
    float n2 = noise();
    double bed = 0, ev = 0, close = 0;   // C-16: `close` is heard dry (a breath at the microphone, a knock's answer), out of the echo
    rdClock += dt;
    auto uni = [&]() { return 0.5 + 0.5 * noise(); };
    const double gT = rdGainLvl;
    // the bed: a rumble far down that breathes, a drone of two tones a quarter-tone apart beating with their octave, a wisp of
    // resonant static wandering between 300 and 1500 Hz, and the hiss; thinned as a signal comes through. C-16: the gain lifts the
    // floor (the hiss up, a crackle of clicks, the rumble breathing harder: deeper and quicker, the whole held near C-07's level)
    rdBrown += (n2 - rdBrown) * 0.012; rdBrown2 += (rdBrown - rdBrown2) * 0.012;
    rdBreathePh += dt * 0.23 * (1 + 1.4 * gT);
    double breathe = 0.55 + 0.45 * std::sin(rdBreathePh * TAU_ * 0.07) * std::sin(rdBreathePh * TAU_ * 0.11 + 1.0);
    if (gT > 0) breathe = std::pow(breathe, 1 + 1.5 * gT) * (1 + 0.5 * gT);
    bed += 3.0 * (1 - 0.3 * gT) * rdBrown2 * breathe;
    rdDr1 += dt * 48.0; rdDr2 += dt * 50.7; rdDr3 += dt * 97.3;
    double droneEnv = 0.45 + 0.55 * std::sin(lfo * TAU_ * 0.05);
    bed += 0.08 * droneEnv * (std::sin(TAU_ * rdDr1) + std::sin(TAU_ * rdDr2) + 0.4 * std::sin(TAU_ * rdDr3));
    rdWispT += dt;
    { double wf = 800 + 550 * std::sin(rdWispT * 0.37) * std::sin(rdWispT * 0.11 + 2.0);
      bed += 1.0 * resonate(n2, wf, 30, dt, rdWispY1, rdWispY2) * (0.5 + 0.5 * std::sin(rdWispT * 0.23)); }
    rdN1 += (n2 - rdN1) * 0.6;
    double swell = 0.75 + 0.25 * std::sin(lfo * TAU_ * 0.31) * std::sin(lfo * TAU_ * 0.53);
    bed += 0.07 * (1 + 1.6 * gT) * (n2 - 0.5 * rdN1) * swell;
    if (gT > 0) {   // C-16: the crackle, clicks at up to fifty-five a second, now and then a louder one
        if (rdCrkT < 0 && uni() < dt * 55 * gT) { rdCrkT = 0; double a = uni(); rdCrkA = (0.08 + 0.4 * a * a) * (uni() < 0.08 ? 2.2 : 1.0); }
        if (rdCrkT >= 0) { bed += rdCrkA * n2 * std::exp(-rdCrkT * 2500); rdCrkT += dt; if (rdCrkT > 0.004) rdCrkT = -1; }
    }
    {   // C-16: the galaxy's own noise in the beam, a rushing that scintillates: loud toward the band and the core, faint at the poles
        double g = rdSkyLvl * (0.15 + 0.85 * gT);
        if (g > 0.002) {
            float n3 = noise();
            rdGalLp += (n3 - rdGalLp) * 0.3; rdGalHp += (rdGalLp - rdGalHp) * 0.03;
            rdScint = std::max(-0.4, std::min(0.5, rdScint + (uni() - 0.5) * dt * 6 - rdScint * dt * 0.8));
            bed += 0.5 * g * (rdGalLp - rdGalHp) * (0.7 + rdScint);
        }
    }
    if (rdGhostLvl > 0.003) {   // C-16: a false rise, the static gathering into almost a tone, wavering, then gone again
        rdGhostF += ((520 + 380 * std::sin(TAU_ * 0.13 * rdClock) * std::sin(TAU_ * 0.041 * rdClock + 1.0)) - rdGhostF) * 0.0005;
        bed += 2.6 * rdGhostLvl * resonate(n2, rdGhostF, 4 + 26 * (1 - rdGhostLvl), dt, rdGhY1, rdGhY2);
    }
    bed *= 1 - 0.6 * radarSigLvl;
    // something in the static, now and then (every twelve to forty seconds): a carrier passing, a boom far off, a moan, knocking, a
    // murmur of voices, a long breath drawn, a scream far away; quieter while a signal is clear. C-16: oftener with the gain (four to
    // thirteen seconds at x32), and at the top steps stranger ones join: speech running backwards, counting whispered in no one's
    // language, a breath close to the microphone, a knock answering a knock, a choir swelling and stopping
    rdEvNext -= dt * (1 + 2.2 * gT);
    if (rdEvNext <= 0 && rdEvT < 0) {
        if (gT > 0.7 && uni() < 0.55 * (gT - 0.7) / 0.3) rdEvKind = 7 + std::min(4, (int)(uni() * 5));
        else { double r = uni(); rdEvKind = r < 0.25 ? 0 : (r < 0.40 ? 1 : (r < 0.60 ? 2 : (r < 0.75 ? 3 : (r < 0.87 ? 4 : (r < 0.95 ? 5 : 6))))); }
        st.radarEvents++; st.radarEventKinds |= 1 << rdEvKind;
        rdEvT = 0; rdEvPh = 0; rdEvLp = 0; rdEvY1 = rdEvY2 = 0;
        Phantom* P = rdEvPhan;
        switch (rdEvKind) {
            case 0: rdEvDur = 2.2 + 1.5 * uni(); rdEvA = 1800 + 1600 * uni(); rdEvB = 4 + 6 * uni(); break;             // the carrier: from where to how fast it beats
            case 1: rdEvDur = 3.5; rdEvA = 36 + 14 * uni(); break;                                                        // the boom's pitch
            case 2: rdEvDur = 2.5 + 2.0 * uni(); rdEvA = 90 + 70 * uni(); rdEvB = 0.75 + 0.2 * uni(); P[4] = Phantom(); P[4].f0 = P[4].f0T = rdEvA; P[4].amp = 0; break;   // the moan: its pitch and where it sinks to
            case 3: rdEvN = 3 + (int)(uni() * 2.99); rdEvA = 0.35 + 0.35 * uni(); rdEvDur = rdEvN * rdEvA + 0.8; rdEvB = 90 + 60 * uni(); break;   // knocks: how many, how spaced, the wood's note
            case 4: rdEvDur = 1.5 + 1.0 * uni(); for (int i = 0; i < 3; i++) { P[i] = Phantom(); P[i].f0Base = rdCh[0].v.murF0[i] * (0.9 + 0.2 * uni()); } break;   // a murmur
            case 5: rdEvDur = 3.2; break;                                                                                  // a breath drawn
            case 6: rdEvDur = 2.4 + 0.6 * uni(); rdEvA = 150 + 60 * uni(); P[4] = Phantom(); P[4].f0 = P[4].f0T = rdEvA; break;   // a scream, far away
            case 7: rdEvDur = 1.6 + 1.2 * uni(); rdEvN = 4 + (int)(uni() * 3.99); rdEvA = 100 + 70 * uni(); rdEvB = uni(); P[0] = Phantom(); P[0].f0 = P[0].f0T = rdEvA; break;   // backwards: the syllables, the pitch
            case 8: rdEvN = 4 + (int)(uni() * 5.99); rdEvA = 0.55 + 0.25 * uni(); rdEvDur = rdEvN * rdEvA + 0.5; rdEvB = uni(); P[0] = Phantom(); break;   // counting: how far, how quick
            case 9: rdEvDur = 3.4; break;                                                                                  // a breath at the microphone
            case 10: rdEvN = 2 + (int)(uni() * 2.99); rdEvA = 0.3 + 0.3 * uni(); rdEvB = 80 + 50 * uni(); rdEvC = 1.3 + 1.4 * uni(); rdEvDur = 2 * rdEvN * rdEvA + rdEvC + 0.8; break;   // a knock, and the answer after a silence
            default: rdEvDur = 3.0 + 2.5 * uni(); rdEvA = 115 + 60 * uni(); for (int i = 0; i < 4; i++) { P[i] = Phantom(); P[i].f0 = P[i].f0T = rdEvA * CLUSTER[i]; } break;   // a choir
        }
    }
    if (rdEvT >= 0) {
        double u = rdEvT / rdEvDur;
        Phantom* P = rdEvPhan;
        if (u >= 1) { rdEvT = -1; rdEvNext = 12 + 28 * uni(); }
        else {
            switch (rdEvKind) {
                case 0: { double f = rdEvA * std::exp(-2.7 * u) + 180; rdEvPh += dt * f; ev += 0.09 * std::sin(PI_ * u) * std::sin(TAU_ * rdEvPh) * (0.7 + 0.3 * std::sin(TAU_ * rdEvB * rdEvT)); break; }
                case 1: { rdEvPh += dt * rdEvA; ev += 0.5 * std::exp(-rdEvT * 1.2) * std::sin(TAU_ * rdEvPh) + 4.0 * rdBrown2 * std::exp(-rdEvT) + 0.4 * n2 * std::exp(-rdEvT * 80); break; }
                case 2: { Phantom& p = P[4]; double env = std::pow(std::sin(PI_ * u), 0.7);
                          p.f0T = rdEvA * (1 - (1 - rdEvB) * u) * (1 + 0.02 * std::sin(TAU_ * 4.7 * rdEvT)); p.ampT = 0.8 * env;
                          double m = 0.5 + 0.5 * std::sin(TAU_ * 0.17 * rdEvT + 1.0); for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[3][i] * (1 - m) + VOWELS[4][i] * m;   // o to u and back
                          ev += 0.55 * phantomSample(p, dt, 0, 0.45, 0.6); break; }
                case 3: { double tk = std::fmod(rdEvT, rdEvA); int k = (int)(rdEvT / rdEvA);
                          double x = k < rdEvN && tk < 0.012 ? n2 : 0;
                          ev += 1.4 * resonate(x, rdEvB, 25, dt, rdEvY1, rdEvY2) * (k < rdEvN ? std::exp(-tk * 18) : 0); break; }
                case 4: { double m = 0; for (int i = 0; i < 3; i++) m += phantomSample(P[i], dt, rdCh[0].v.murRate * (i == 0 ? 1 : (i == 1 ? 1.12 : 0.9)), 0.25, 0.3);
                          rdEvLp += (m - rdEvLp) * 0.18; ev += 0.4 * std::sin(PI_ * u) * rdEvLp; break; }
                case 5: { rdEvLp += (n2 - rdEvLp) * 0.09; double env = u < 0.85 ? std::pow(u / 0.85, 2.0) : 0; ev += 0.7 * env * rdEvLp; break; }
                case 6: { Phantom& p = P[4]; double env = std::pow(std::sin(PI_ * u), 0.5), g = std::pow(u, 1.5);
                          p.f0T = rdEvA * (1 + 1.6 * g); p.ampT = 0.9 * env;
                          for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[0][i] * (1 - g) + VOWELS[2][i] * g;   // a to i
                          ev += 0.35 * phantomSample(p, dt, 0, 0.15, 1.2); break; }
                case 7: {   // speech running backwards: each syllable swelling out of its own echo and cut off, the phrase's melody rising
                    Phantom& p = P[0];
                    int k = std::min(rdEvN - 1, (int)(u * rdEvN)); double su = u * rdEvN - k;
                    uint64_t h = mix64((uint64_t)(rdEvB * 4294967296.0) + (uint64_t)k * 77);
                    int v1 = (int)(u01(h) * 6.99), v2 = (int)(u01(mix64(h ^ 1)) * 6.99);
                    double m = su < 0.7 ? 0 : (su - 0.7) / 0.3;
                    for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[v1][i] * (1 - m) + VOWELS[v2][i] * m;
                    p.ampT = su < 0.93 ? 0.95 * std::pow(su / 0.93, 2.2) : 0;
                    p.f0T = rdEvA * (0.8 + 0.45 * u);
                    ev += 0.6 * phantomSample(p, dt, 0, 0.25 + 0.45 * (1 - su), 0.35);
                    break;
                }
                case 8: {   // counting whispered in no one's language: a word of two syllables for each number, evenly, the last drawn out
                    Phantom& p = P[0];
                    int k = (int)(rdEvT / rdEvA); double tk = rdEvT - k * rdEvA;
                    int syl = tk < 0.19 ? 0 : 1; double t0 = syl ? 0.19 : 0.0, len = (k == rdEvN - 1 && syl) ? 0.32 : 0.16;
                    double env = k < rdEvN && tk - t0 < len ? std::pow(std::sin(PI_ * (tk - t0) / len), 0.8) : 0;
                    int vw = (int)(u01(mix64((uint64_t)(rdEvB * 4294967296.0) ^ ((uint64_t)(k * 2 + syl + 1) * 0x9E37ULL))) * 6.99);
                    for (int i = 0; i < 3; i++) p.FT[i] = VOWELS[vw][i];
                    p.ampT = 0.95 * env;
                    ev += 0.8 * phantomSample(p, dt, 0, 1.0, 0.0);
                    break;
                }
                case 9: {   // a breath close to the microphone: the pop and the lips, a long breath in, a held moment, out; dry
                    double t = rdEvT, x = 0;
                    if (t < 0.06) x += 0.7 * std::exp(-t * 55) * std::sin(TAU_ * 48 * t);
                    if (t > 0.13 && t < 0.135) x += 0.6 * n2;
                    if (t > 0.18 && t < 1.55) x += 1.3 * std::pow(std::sin(PI_ * (t - 0.18) / 1.37), 1.3) * resonate(n2, 1900, 1400, dt, rdEvY1, rdEvY2);
                    if (t > 1.85 && t < 3.3) { rdEvLp += (n2 - rdEvLp) * 0.15; x += 1.1 * std::pow(std::sin(PI_ * (t - 1.85) / 1.45), 0.8) * rdEvLp; }
                    close += 0.5 * x;
                    break;
                }
                case 10: {   // a knock answering a knock: the knocks far off, a silence, the same knocks again, deeper and near, dry
                    double t1 = rdEvN * rdEvA, t2 = t1 + rdEvC;
                    if (rdEvT < t1) { double tk = std::fmod(rdEvT, rdEvA); double x = tk < 0.012 ? n2 : 0; ev += 1.4 * resonate(x, rdEvB, 25, dt, rdEvY1, rdEvY2) * std::exp(-tk * 18); }
                    else if (rdEvT >= t2 && rdEvT < t2 + rdEvN * rdEvA) { double tk = std::fmod(rdEvT - t2, rdEvA); double x = tk < 0.014 ? n2 : 0; close += 1.8 * resonate(x, rdEvB * 0.78, 30, dt, rdEvY1, rdEvY2) * std::exp(-tk * 16); }
                    break;
                }
                default: {   // a choir swelling and stopping: four voices in a cluster, out of nothing to full, then cut off at once
                    double env = std::pow(u, 1.8) * std::min(1.0, (1 - u) * rdEvDur / 0.04), x = 0;
                    for (int i = 0; i < 4; i++) {
                        Phantom& p = P[i];
                        p.f0T = rdEvA * CLUSTER[i] * (1 + 0.006 * std::sin(TAU_ * (4.6 + 0.4 * i) * rdEvT + i));
                        p.ampT = p.amp = 0.8 * env;   // the stop must be a stop: no smoothing
                        for (int j = 0; j < 3; j++) p.FT[j] = VOWELS[0][j] * (1 - u) + VOWELS[3][j] * u;
                        x += phantomSample(p, dt, 0, 0.2, 0.15);
                    }
                    ev += 0.3 * x;
                    break;
                }
            }
            rdEvT += dt;
        }
    }
    ev *= 1 - 0.7 * radarSigLvl;
    close *= 1 - 0.5 * radarSigLvl;
    // the signal centred, and (C-16) the one under it bleeding in from the wide lobe (kept while it fades)
    double sig = channelSample(rdCh[0], dt, st.radarKind, st.radarVoice, st.radarPulseHz, st.radarSeed, st.radarAge, st, true);
    if (st.radarKind2 >= 0) { rdKind2 = st.radarKind2; rdVoice2 = st.radarVoice2; rdAge2 = st.radarAge2; rdSeed2 = st.radarSeed2; rdPulse2 = st.radarPulseHz2; }
    double sig2 = rdSig2Lvl > 0.002 && rdKind2 >= 0 ? channelSample(rdCh[1], dt, rdKind2, rdVoice2, rdPulse2, rdSeed2, rdAge2, st, false) : 0;
    double main = 0.45 * sig * radarSigLvl + 0.8 * rdContent, under = 0.4 * sig2 * rdSig2Lvl, mix = 0;
    // C-16: a far signal worn by the way: deep slow fades, dropouts, and two or three echoes of itself (the paths it came by); a word
    // heard before, back out of the static, echoes as from as far
    double echoes = std::max(rdWearLvl, rdGhostWordLvl > 0.01 ? 1.0 : 0.0);
    if (rdWearLvl > 0.005) {
        rdFadeT += dt;
        double fd = 0.5 + 0.5 * std::sin(TAU_ * 0.057 * rdFadeT) * std::sin(TAU_ * 0.0213 * rdFadeT + 1.3);
        double fade = 1 - rdWearLvl * 0.9 * std::pow(1 - fd, 1.6);
        rdWDropT -= dt;
        if (rdWDropT <= 0) { bool gap = uni() < 0.18 + 0.32 * rdWearLvl; rdWDrop = gap ? 0.0 : 1.0; rdWDropT = gap ? (0.06 + 0.4 * uni()) * (1 + 1.5 * rdWearLvl) : 0.7 + 3.0 * uni(); }
        rdWDropLvl += (rdWDrop - rdWDropLvl) * 0.006;
        main *= fade * (1 - rdWearLvl * (1 - rdWDropLvl));
    }
    if (echoes > 0.005) {
        if (rdWorn.size() != 16384) { rdWorn.assign(16384, 0.0f); rdWornI = 0; }
        rdWorn[rdWornI] = (float)main;
        auto tapW = [&](double s) { size_t back = (size_t)(s / dt) % rdWorn.size(); return (double)rdWorn[(rdWornI + rdWorn.size() - back) % rdWorn.size()]; };
        main += echoes * (0.42 * tapW(0.131) + 0.27 * tapW(0.293) + 0.17 * tapW(0.467));
        rdWornI = (rdWornI + 1) % rdWorn.size();
    }
    // C-16: two peoples' carriers beat into a whistle that slides as the beam moves; at the top steps the two ring against each other
    rdHetF += (st.radarHetHz - rdHetF) * co.kHet;
    rdHetPh += dt * rdHetF;
    if (rdHetLvl > 0.002) mix += rdHetLvl * 0.075 * (std::sin(TAU_ * rdHetPh) + 0.15 * std::sin(2 * TAU_ * rdHetPh)) * (0.75 + 0.25 * std::sin(TAU_ * 5.5 * rdClock));
    if (rdImLvl > 0.002) { rdImPh += dt * (rdHetF * 1.37 + 230); mix += rdImLvl * (0.9 * main * std::sin(TAU_ * rdImPh) + 0.5 * std::tanh(5 * main) * std::tanh(5 * under)); }
    double hot = main + under + mix;
    if (rdDriveLvl > 0.002) {   // C-15: a near signal through a high gain overdrives the receiver: the signal pushed into a hard knee, buzzing and flat
        double sq = 0.22 * std::tanh(hot * 9.0);
        hot += (sq - hot) * rdDriveLvl;
    }
    double dry = bed + ev + hot;
    // the echo: a third of a second behind, half of it back (more under a chant, whispers, a murmur, a bell), dulled; the void everything is heard in
    double fb = st.radarKind == 0 ? (st.radarVoice == RP_CHANT ? 0.72 : (st.radarVoice == RP_WHISPERS ? 0.7 : (st.radarVoice == RP_MURMUR ? 0.72 : (st.radarVoice == RP_TOLL ? 0.65 : 0.5)))) : 0.5;
    if (rdEcho.size() != 8192) { rdEcho.assign(8192, 0.0f); rdEchoI = 0; }
    double e = rdEcho[rdEchoI]; rdEchoLp += (e - rdEchoLp) * 0.3;
    rdEcho[rdEchoI] = (float)(dry + rdEchoLp * fb); rdEchoI = (rdEchoI + 1) % rdEcho.size();
    return radarLvl * ((dry + 0.55 * rdEchoLp) * (1 - 0.15 * gT) + close);   // C-16: the whole held near C-07's level as the floor rises
}

// X-01: the probe's relay. The link is a soft hiss with a tick a second while the probe is in space; the air a rush whose
// brightness and level follow the stage; the entry a roar with a crackle; the chute a flutter on the rush; below the first deck a
// rumble deepening and, past half the depth, the hull's groans; a thunderclap muffled through the relay; at the end only the hiss
// of the lost signal. Everything through the relay's level (`prLvl`)
double AudioSynth::probeSample(double dt, AudioState& st) {
    if (st.probeThunder > 0) { prThunT = 0; prThunAmp = st.probeThunder; st.probeThunder = 0; }
    float n = noise();
    double s = 0;
    prHp += (n - prHp) * 0.35; double hiss = n - prHp;   // a high band of white noise
    if (st.probeLost) {
        prCrackle -= dt;
        double crack = 0;
        if (prCrackle <= 0) { crack = 0.5 * noise(); prCrackle = 0.02 + 0.3 * (0.5 + 0.5 * noise()); }
        return prLvl * (0.11 * hiss + 0.05 * crack);
    }
    double statik = 1 - st.probeSignal;
    s += (0.012 + 0.05 * statik * statik) * hiss;
    if (st.probeStage <= 1) {   // in space: the telemetry's tick
        prTick += dt;
        if (prTick >= 1.0) prTick -= 1.0;
        if (prTick < 0.008) s += 0.05 * std::sin(6.2831853 * 1600 * prTick);
    }
    double cut = 0.02 + 0.10 * st.probeWind * (1 - 0.6 * st.probeDepth);   // the air's rush, darker as it deepens
    prWindLp += (n - prWindLp) * cut; prWindLp2 += (prWindLp - prWindLp2) * cut;
    double gust = 0.75 + 0.25 * std::sin(lfo * 6.2831853 * 0.6) * std::sin(lfo * 6.2831853 * 0.17 + 1.1);
    double flutter = st.probeChute > 0 ? 0.55 + 0.45 * std::sin(prChutePh += dt * 6.2831853 * 7.3) : 1.0;
    s += st.probeWind * 0.55 * prWindLp2 * gust * flutter;
    if (st.probeHeat > 0.01) {   // the entry: a roar and a crackle
        prHeatLp1 += (n - prHeatLp1) * 0.03; prHeatLp2 += (prHeatLp1 - prHeatLp2) * 0.03;
        double crack = (0.5 + 0.5 * noise()) < 0.004 * st.probeHeat ? noise() * 0.6 : 0.0;
        s += st.probeHeat * (1.1 * prHeatLp2 + 0.12 * prHeatLp1 + crack);
    }
    if (st.probeDepth > 0.01) {   // the deep: a rumble, and the hull's groans past half the depth
        prRum1 += (n - prRum1) * 0.006; prRum2 += (prRum1 - prRum2) * 0.006;
        s += st.probeDepth * 1.6 * prRum2;
        if (st.probeDepth > 0.5) {
            prCreakNext -= dt;
            if (prCreakNext <= 0 && prCreakT < 0) { prCreakT = 0; prCreakF = 70 + 40 * (0.5 + 0.5 * noise()); prCreakNext = 2.5 + 6 * (0.5 + 0.5 * noise()) / st.probeDepth; }
            if (prCreakT >= 0) {
                double dur = 0.7;
                if (prCreakT < dur) {
                    double env = std::sin(prCreakT / dur * 3.14159) * (0.7 + 0.3 * std::sin(prCreakT * 90));
                    prCreakPh += dt * prCreakF * (1 - 0.35 * prCreakT / dur);
                    s += st.probeDepth * 0.09 * env * (std::sin(prCreakPh * 6.2831853) + 0.4 * std::sin(prCreakPh * 6.2831853 * 2.7));
                    prCreakT += dt;
                } else prCreakT = -1;
            }
        }
    }
    if (prThunT >= 0) {   // thunder through the relay: a band of the roar, a sharp front and a long tail
        if (prThunT < 2.2) {
            prThunLp1 += (n - prThunLp1) * 0.05; prThunLp2 += (prThunLp1 - prThunLp2) * 0.05;
            double env = std::exp(-prThunT * 1.8) * (prThunT < 0.03 ? prThunT / 0.03 : 1.0);
            s += prThunAmp * 2.2 * env * prThunLp2;
            prThunT += dt;
        } else prThunT = -1;
    }
    return prLvl * s;
}

void AudioSynth::coefs(double dt) {
    co.kF = 1 - std::exp(-dt / 0.02); co.kUp = 1 - std::exp(-dt / 0.006); co.kDown = 1 - std::exp(-dt / 0.012); co.kN = 1 - std::exp(-dt / 0.003);
    static const double BW[4] = {80, 100, 150, 250};
    for (int i = 0; i < 4; i++) co.rS[i] = std::exp(-PI_ * BW[i] * dt);
    co.rNasal = std::exp(-PI_ * 120 * dt);
    co.pkF = 1 - std::exp(-dt / 0.05); co.pkA = 1 - std::exp(-dt / 0.03); co.pkP = 1 - std::exp(-dt / 0.1);
    static const double BWP[3] = {90, 110, 160};
    for (int i = 0; i < 3; i++) co.rP[i] = std::exp(-PI_ * BWP[i] * dt);
    co.kBeac = 1 - std::exp(-dt / 0.008); co.kData = 1 - std::exp(-dt / 0.002); co.kChant = 1 - std::exp(-dt / 0.6); co.kCode = 1 - std::exp(-dt / 0.004);
    co.kHet = 1 - std::exp(-dt / 0.08);
}

void AudioSynth::render(float* out, int frames, int sr, AudioState& st) {
    double dt = 1.0 / sr;
    coefs(dt);
    mediumCoefs(dt, st.medium);
    if (st.beep > 0) { beepT = 0; beepKind = st.beep; st.beep = 0; }
    if (st.thunder > 0) { thunderT = 0; thunderAmp = st.thunder; st.thunder = 0; }
    if (st.step > 0) { stepT = 0; stepKind = st.step; stepGainCur = st.stepGain; st.step = 0; }
    if (st.deck != 0) { if (st.deck > 0) deckBuf = deckRender(st.deck, st.deckSeed, sr); else deckBuf.clear(); deckI = 0; st.deck = 0; }   // R-420: the cassette deck's sequence (-1 hushes it)
    const double kDuck = 1 - std::exp(-dt / 0.5);
    // C-04 the music: a start from the top; a null piece stops every voice at once
    if (st.pieceStart) {
        st.pieceStart = false; mPiece = st.piece; mNext = 0; mBeat = 0; mT = 0; for (MVoice& v : mv) v = MVoice(); mOn = mPiece && st.tradition && !mPiece->notes.empty(); st.pieceDone = !mOn;
        if (mOn && st.pieceFrom > 0) {   // C-11: taken up again where it was held (the notes begun before it are not struck again)
            mBeat = st.pieceFrom; mT = st.pieceFrom * 60.0 / std::max(1.0, mPiece->bpm); mLimEnv = 0; mLimGain = 1;
            while (mNext < mPiece->notes.size() && mPiece->notes[mNext].t < mBeat) mNext++;
        }
        st.pieceFrom = 0;
    }
    if (!st.piece || !st.tradition || st.piece != mPiece) { if (mOn || mPiece) { for (MVoice& v : mv) v = MVoice(); mOn = false; mPiece = nullptr; } }
    // C-05 the voice: a start from the top; a null speech stops it at once, the resonators cleared
    if (st.speechStart && st.radio) { rdCueT = 0; rdCuePh = 0; }   // C-07: the receiver's cue at a programme's start
    if (st.speechStart) {
        st.speechStart = false; spSpeech = st.speech; spNext = 0; spSeg = -1; spT = 0; spVoiceAmp = spBreathAmp = spNzAmp = spNasal = 0; spPh = spSubPh = 0; spJit = spShim = 1; spLpB = 0;
        for (int i = 0; i < 6; i++) spY1[i] = spY2[i] = 0;
        spOn = spSpeech && st.voice && !spSpeech->segs.empty(); st.speechDone = !spOn;
        if (spOn && st.speechFrom > 0) spT = std::min(st.speechFrom, spSpeech->seconds);   // C-11: taken up again where it was held
        st.speechFrom = 0;
    }
    // C-11 the instrument played in the cabin: another instrument (or none) silences the last one's notes; the notes struck, the keys let go
    if (st.instrument != ivTimbre) { for (MVoice& v : iv) v = MVoice(); ivTimbre = st.instrument; iLimEnv = 0; iLimGain = 1; }
    if (!st.instrumentHits.empty()) { for (const AudioState::InstrumentNote& h : st.instrumentHits) instrumentTrigger(h, dt); st.instrumentHits.clear(); }
    if (!st.instrumentUps.empty()) {
        for (int key : st.instrumentUps) for (int i = 0; i < IVOICES; i++) if (iv[i].on && ivKey[i] == key && iv[i].gate > iv[i].t) iv[i].gate = iv[i].t;
        st.instrumentUps.clear();
    }
    if (!st.speech || !st.voice || st.speech != spSpeech) { if (spOn || spSpeech) { spOn = false; spSpeech = nullptr; spVoiceAmp = spBreathAmp = spNzAmp = 0; for (int i = 0; i < 6; i++) spY1[i] = spY2[i] = 0; } }
    double k = 1 - std::exp(-dt * 2.0);
    for (int i = 0; i < frames; i++) {
        humLvl += (st.hum - humLvl) * k;
        engLvl += (st.engine - engLvl) * k;
        windLvl += (st.wind - windLvl) * k;
        leavesLvl += (st.leaves - leavesLvl) * k;
        hoofLvl += (st.hoofs - hoofLvl) * k;
        rpmLvl += (st.engineRpm - rpmLvl) * k * 2;
        skidLvl += (st.skid - skidLvl) * k;
        rotorLvl += (st.rotor - rotorLvl) * k; rotorPitchLvl += (st.rotorPitch - rotorPitchLvl) * k;
        radarLvl += (st.radar - radarLvl) * k; radarSigLvl += (st.radarSignal - radarSigLvl) * k * 2;   // C-07
        rdDriveLvl += (st.radarDrive - rdDriveLvl) * k * 2;   // C-15
        rdGainLvl += (st.radarGainT - rdGainLvl) * k * 2; rdSkyLvl += (st.radarSky - rdSkyLvl) * k; rdWearLvl += (st.radarWear - rdWearLvl) * k;   // C-16
        rdGhostLvl += (st.radarGhost - rdGhostLvl) * k * 3; rdSig2Lvl += (st.radarSignal2 - rdSig2Lvl) * k * 2; rdHetLvl += (st.radarHet - rdHetLvl) * k * 2; rdImLvl += (st.radarIntermod - rdImLvl) * k * 2;
        rdGhostWordLvl += ((st.radarGhostWord ? 1.0 : 0.0) - rdGhostWordLvl) * k * 6;
        if (st.thud > 0 && thudT < 0) { thudT = 0; thudAmp = st.thud; }
        st.thud = 0;
        insectLvl += (st.insects - insectLvl) * k;
        if (st.call > 0 && callT < 0) { callT = 0; callKind = st.call; callPitchCur = st.callPitch; }
        st.call = 0;
        rainLvl += (st.rain - rainLvl) * k;
        lavaLvl += (st.lava - lavaLvl) * k;
        breathLvl += (st.breath - breathLvl) * k;
        surfLvl += (st.surf - surfLvl) * k;
        birdLvl += (st.birds - birdLvl) * k;
        lfo += dt * 0.23;
        double s = 0;
        // hum: two low sines with slow beating
        phase1 += dt * 55.0; phase2 += dt * (110.0 + 0.7 * std::sin(lfo * 6.28));
        s += humLvl * 0.16 * (std::sin(phase1 * 6.2831853) + 0.5 * std::sin(phase2 * 6.2831853)) * (0.8 + 0.2 * std::sin(lfo * 6.28 * 0.5));
        // engine: filtered noise + rising tone
        float n = noise();
        lp1 += (n - lp1) * 0.08f;
        lp2 += (lp1 - lp2) * 0.08f;
        s += engLvl * (0.5 * lp2 + 0.12 * std::sin(phase1 * 6.2831853 * 3.0 * (1 + engLvl)));
        if (engLvl > 0.05 && rpmLvl > 0.001) s += engLvl * 0.08 * std::sin(phase1 * 6.2831853 * (9.0 + 12.0 * rpmLvl));   // N4-04 the gear's pitch on top of the drive
        if (skidLvl > 0.02) { float n4 = noise(); lpK += (n4 - lpK) * 0.25f; s += skidLvl * 0.16 * lpK; }   // N4-04 tyres sliding
        if (rotorLvl > 0.01) {   // R-403 the drone's pods: a blade-pass whine (a fundamental with two harmonics, beating between the four pods) over a low rush of air
            rotorPh += dt * (70.0 + 90.0 * rotorPitchLvl);
            double w = std::sin(rotorPh * 6.2831853) + 0.5 * std::sin(rotorPh * 6.2831853 * 2.01) + 0.25 * std::sin(rotorPh * 6.2831853 * 3.02);
            double beat = 0.8 + 0.2 * std::sin(lfo * 6.2831853 * 6.1) * std::sin(lfo * 6.2831853 * 4.3);
            lpR += (n - lpR) * 0.18;
            s += rotorLvl * (0.09 * w * beat + 0.2 * lpR);
        }
        if (thudT >= 0) { if (thudT < 0.25) { s += thudAmp * 0.35 * std::exp(-thudT * 18) * std::sin(6.2831853 * thudT * 55); thudT += dt; } else thudT = -1; }
        // wind: low-pass noise with gusty amplitude; the tone sets the family (sea: low and slow, mountain: a whistle)
        lp3 += (n - lp3) * (0.012f + 0.06f * (float)st.windTone + 0.03f * (float)windLvl);
        double gust = 0.6 + 0.4 * std::sin(lfo * 6.28 * (0.5 + 1.2 * st.windTone)) * std::sin(lfo * 6.28 * 0.37);
        s += windLvl * 0.9 * lp3 * gust;
        if (st.windTone > 0.85) s += windLvl * 0.06 * std::sin(phase1 * 6.2831853 * 14.0 * (1 + 0.3 * std::sin(lfo * 6.28 * 0.9)));   // the whistle
        if (callT >= 0) {   // N3-05 creature calls: chirp (a rising sine), low (a slow growl with vibrato), hoot (two soft notes), click (four noise ticks)
            double p = callPitchCur, v = 0;
            switch (callKind) {
                case 1: { double dur = 0.18; if (callT < dur) v = 0.14 * std::sin(callT / dur * 3.14159) * std::sin(6.2831853 * callT * (1800 + 1400 * callT / dur) * p); else callT = -2; break; }
                case 2: { double dur = 0.7; if (callT < dur) v = 0.16 * std::sin(callT / dur * 3.14159) * (std::sin(6.2831853 * callT * 110 * p * (1 + 0.04 * std::sin(callT * 40))) + 0.35 * std::sin(6.2831853 * callT * 220 * p) + 0.15 * std::sin(6.2831853 * callT * 330 * p)); else callT = -2; break; }
                case 3: { double dur = 0.55; if (callT < dur) { double f = callT < 0.27 ? 520 : 430; double e = std::sin(std::fmod(callT, 0.27) / 0.27 * 3.14159); v = 0.12 * e * std::sin(6.2831853 * callT * f * p); } else callT = -2; break; }
                default: { double dur = 0.34; if (callT < dur) { double ph = std::fmod(callT, 0.085); v = ph < 0.012 ? 0.2 * noise() : 0; } else callT = -2; break; }
            }
            if (callT >= 0) { s += v; callT += dt; } else callT = -1;
        }
        if (hoofLvl > 0.01) {   // N3-05 hoof and paw steps: soft thumps at a pace that quickens with the level
            hoofT += dt;
            double period = 0.42 / (0.5 + hoofLvl);
            if (hoofT > period) hoofT -= period;
            if (hoofT < 0.06) s += hoofLvl * 0.14 * std::exp(-hoofT * 45) * std::sin(6.2831853 * hoofT * 70);
        }
        if (insectLvl > 0.01) {   // N3-05 insects: a thin buzz, amplitude-modulated
            float n3 = noise();
            lpI += (n3 - lpI) * 0.6f; lpI2 += (lpI - lpI2) * 0.6f;
            s += insectLvl * 0.035 * (lpI - lpI2) * (0.6 + 0.4 * std::sin(6.2831853 * 48 * lfo));
        }
        if (leavesLvl > 0.001) {   // N2-06 leaves: a brighter, band-passed hiss that swells with the gusts
            float n2 = noise();
            lpL += (n2 - lpL) * 0.35f;
            hpL += (lpL - hpL) * 0.04f;
            double gust2 = 0.5 + 0.5 * std::sin(lfo * 6.28 * 0.8 + 1.7) * std::sin(lfo * 6.28 * 0.23);
            s += leavesLvl * 0.5 * (lpL - hpL) * (0.4 + 0.6 * gust2);
        }
        // surf: slow surges of noise
        if (surfLvl > 0.01) { double surge = std::pow(0.5 + 0.5 * std::sin(lfo * 6.28 * 0.14), 3.0); s += surfLvl * 0.7 * lp3 * surge; }
        // bird calls: short sine sweeps now and then
        if (birdLvl > 0.02) {
            nextChirp -= dt;
            if (nextChirp <= 0 && chirpT < 0) { chirpT = 0; chirpF = 1800 + 2200 * (0.5 + 0.5 * noise()); nextChirp = 0.4 + 3.0 * (0.5 + 0.5 * noise()) / birdLvl; }
            if (chirpT >= 0) {
                double dur = 0.09;
                if (chirpT < dur) { double env = std::sin(chirpT / dur * 3.14159); s += birdLvl * 0.12 * env * std::sin(chirpT * (chirpF + 1500 * chirpT / dur) * 6.2831853); chirpT += dt; }
                else chirpT = -1;
            }
        }
        // footsteps: a short filtered burst per stride
        if (stepT >= 0) {
            double dur = stepKind == 3 ? 0.16 : 0.05;
            if (stepT < dur) {
                double env = stepT < 0.005 ? stepT / 0.005 : std::exp(-(stepT - 0.005) * (stepKind == 3 ? 20 : 60));
                lpS += (n - lpS) * (stepKind == 1 ? 0.12f : 0.5f);
                hpS = n - hpS * 0.5;
                double v = stepKind == 1 ? lpS * 1.2 : (stepKind == 2 ? hpS * 0.5 : lpS * 0.9);
                s += stepGainCur * 0.35 * env * v;
                stepT += dt;
            } else stepT = -1;
        }
        // rain: high-passed noise crackle
        hp = n - hp * 0.6;
        s += rainLvl * 0.14 * hp;
        // lava rumble
        s += lavaLvl * 0.25 * lp2 * (0.5 + 0.5 * std::sin(lfo * 6.28 * 2.3));
        // breathing: slow noise pulses, faster when more winded
        if (breathLvl > 0.01) { breathPh += dt * (0.7 + 1.1 * breathLvl); double env = std::pow(std::max(0.0, std::sin(breathPh * 6.2831853)), 3.0); s += breathLvl * 0.22 * env * lp3; }
        {   // C-04 the music, C-05 the voice; C-07: through the radar's receiver when the radar owns them: a band of 250 Hz to 2.6 kHz,
            // a tape's wow (slight under a voice, heavy under a slowed piece), a slow fading, and only as much of it as the beam has
            double m = musicSample(dt, st) + speechSample(dt, st);
            rdContent = 0;
            if (st.radio) {   // and ghosted: a metallic ring under it, a slow fade, dropouts now and then; it joins the receiver's bed and echo below
                // C-16: a far signal's content comes through narrower and drifting in pitch (`radarWear`); a phonograph's through its
                // horn (a hump in the middle, the needle's edge, the turntable's wow); a synthetic voice with a ring under it and its
                // samples held on a coarse clock; a word heard before, back out of the static, as from as far as anything is
                const bool ghost = st.radarGhostWord;
                const double wear = ghost ? 1.0 : rdWearLvl;
                const int pv = ghost ? -1 : st.radarVoice;
                const bool horn = pv == RP_PHONOGRAPH, synthetic = pv == RP_SYNTHETIC;
                rdHp += (m - rdHp) * (0.069 + 0.09 * wear + (horn ? 0.05 : 0.0)); double hp = m - rdHp;
                double lpk = 0.52 - 0.3 * wear - (horn ? 0.12 : 0.0);
                rdLp1 += (hp - rdLp1) * lpk; rdLp2 += (rdLp1 - rdLp2) * lpk;
                double x0 = rdLp2;
                if (horn) { x0 = 0.55 * x0 + 1.5 * resonate(x0, rdCh[0].v.hornF, 420, dt, rdHornY1, rdHornY2); x0 = std::tanh(2.2 * x0) / 2.2; }
                if (synthetic) { rdRmPh += dt * rdCh[0].v.ringHz; x0 *= 0.4 + 0.6 * std::sin(TAU_ * rdRmPh); rdShT += dt * rdCh[0].v.holdHz; if (rdShT >= 1) { rdShT -= 1; rdShV = x0; } x0 = rdShV; }
                bool slowed = pv == RP_MUSIC, whispers = pv == RP_WHISPERS;
                if (rdWow.size() != 4096) { rdWow.assign(4096, 0.0f); rdWowI = 0; }
                rdWow[rdWowI] = (float)x0;
                rdWowT += dt;
                double d = 0.04 + (slowed ? 0.028 * (1 + std::sin(6.2831853 * 0.35 * rdWowT)) : 0.0025 * (1 + std::sin(6.2831853 * 0.8 * rdWowT))) + 0.0015 * std::sin(6.2831853 * 7.3 * rdWowT);
                d += wear * (0.022 * (1 + std::sin(TAU_ * 0.083 * rdWowT)) + 0.012 * (1 + std::sin(TAU_ * 0.027 * rdWowT + 2.0)));   // C-16: the pitch drifting
                if (horn) d += 0.0012 * (1 + std::sin(TAU_ * rdCh[0].v.turnHz * rdWowT));
                auto tap = [&](double back) { double pos = (double)rdWowI - back * sr; while (pos < 0) pos += rdWow.size(); size_t i0 = (size_t)pos % rdWow.size(), i1 = (i0 + 1) % rdWow.size(); double fr = pos - std::floor(pos); return rdWow[i0] * (1 - fr) + rdWow[i1] * fr; };
                double x = tap(d);
                if (whispers) x = 0.75 * x + 0.5 * tap(d + 0.09);   // whispers, plural
                rdWowI = (rdWowI + 1) % rdWow.size();
                double fade = 0.72 + 0.28 * std::sin(lfo * 6.2831853 * 0.9) * std::sin(lfo * 6.2831853 * 1.7);
                rdRing += dt * (slowed ? 23.0 : 37.0);
                rdDropT -= dt;
                if (rdDropT <= 0) { bool gap = 0.5 + 0.5 * noise() < 0.22; rdDrop = gap ? 0.1 : 1.0; rdDropT = gap ? 0.12 + 0.35 * (0.5 + 0.5 * noise()) : 1.5 + 4.0 * (0.5 + 0.5 * noise()); }
                rdDropLvl += (rdDrop - rdDropLvl) * 0.003;
                double through = ghost ? 0.5 * rdGhostWordLvl : radarSigLvl;   // C-16: a ghost's word at a level of its own, never dropped
                rdContent = x * (0.78 + 0.22 * std::sin(6.2831853 * rdRing)) * (whispers ? 2.2 : 1.6) * (1 + 0.6 * wear) * through * fade * (ghost ? 1.0 : rdDropLvl);
                m = 0;
            } else if (st.medium >= 0 || mdLvl > 1e-4) m = mediumSample(m, dt, st);   // H-05: a recording on the decoder through what it was made on
            s += m;
        }
        if (ivTimbre) s += instrumentSample(dt, st);   // C-11: an instrument played in the cabin, dry
        prLvl += (st.probe - prLvl) * k * 2;   // X-01 the probe's relay
        if (prLvl > 0.003 || st.probeThunder > 0) s += probeSample(dt, st);
        if (radarLvl > 0.005) s += radarSample(dt, st);   // C-07 the receiver (the radar camera)
        else { rdContent = 0; rdEvT = -1; rdEvNext = 6 + 8 * (0.5 + 0.5 * noise()); rdClock = 0; }
        // beep
        if (beepT >= 0) {
            double f = beepKind == 1 ? 880 : (beepKind == 2 ? 660 : (beepKind == 3 ? 220 : 1320));
            double dur = beepKind == 2 ? 0.35 : 0.12;
            if (beepT < dur) {
                double env = std::sin(beepT / dur * 3.14159);
                s += 0.25 * env * std::sin(beepT * f * 6.2831853);
                beepT += dt;
            } else beepT = -1;
        }
        if (thunderT >= 0) {
            if (thunderT < 2.5) {
                double env = std::exp(-thunderT * 1.5) * (thunderT < 0.05 ? thunderT / 0.05 : 1.0);
                s += thunderAmp * 0.8 * env * lp2 * 3.0;
                thunderT += dt;
            } else thunderT = -1;
        }
        duckLvl += (st.duck - duckLvl) * kDuck;   // R-420: everything under the tape's music, the deck over it
        s *= duckLvl;
        if (deckI < deckBuf.size()) s += deckBuf[deckI++];
        out[i] = (float)(std::max(-1.0, std::min(1.0, s)) * st.master);
    }
    if (!deckBuf.empty() && deckI >= deckBuf.size()) { deckBuf.clear(); deckI = 0; }
    st.pieceBeat = mOn ? mBeat : -1;
    st.speechT = spOn ? spT : -1;
}
