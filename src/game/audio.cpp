#include "audio.h"
#include "audio_internal.h"
#include "peoples/music.h"
#include "peoples/voice.h"
#include "peoples/signals.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>

// C-04/H-05: the music's player (`musicTrigger`, `musicVoice`, `musicSample`) is game/audio_music.cpp
// C-07: the radar's receiver (`radarSample`, its channels and machines) is game/audio_radar.cpp

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

// Q-03: the sounds of the ground the world's traits give. The singing dunes: a slip face's sand sliding hums at one low note (Andreotti's
// f = 0.4 sqrt(g / d) of the grains' size, 70-105 Hz on Earth's, lower on a lighter world) with its harmonics, pure and loud, wavering as the
// avalanche runs and buzzing with the grains' collisions, over the hiss of the sand. The singing ice: a sheet cracking near (the crack, then
// the flexural wave's dispersive chirp: the highs first, the pitch falling as 1/t^2), a crack far off across the sheet (a longer, duller
// chirp: lake ice's "pew"), the sheet's boom as it contracts (a low whoop gliding down), the snow over it settling (a dull thud); oftener the
// more the day warms or cools it. A quake: the P-wave's thump, then the rumble under a slow heave and the stones' rattle (the air's: on an
// airless world the ground's rumble alone, through the boots)
double AudioSynth::groundSample(double dt, AudioState& st) {
    double s = 0;
    const double air = st.air;
    auto uni = [&]() { return 0.5 + 0.5 * noise(); };
    if (duneLvl > 0.002) {
        duneT += dt;
        duneHzCur += (st.duneHz - duneHzCur) * co.kDuneHz;
        const double f = duneHzCur * (1 + 0.025 * std::sin(TAU_ * 0.23 * duneT) * std::sin(TAU_ * 0.071 * duneT + 1.0) + 0.008 * std::sin(TAU_ * 2.9 * duneT));
        dunePh += dt * f; dunePh -= std::floor(dunePh);
        static const double PART[5] = {1.0, 0.5, 0.28, 0.14, 0.07};
        const double lift = clampd((70.0 - duneHzCur) / 35.0, 0.0, 1.0);   // a low note (a light world's) heard by its harmonics
        double x = 0;
        for (int i = 0; i < 5; i++) x += PART[i] * (i == 0 ? 1 - 0.4 * lift : 1 + 0.6 * lift) * std::sin(TAU_ * (i + 1) * dunePh);
        const double buzz = 0.8 + 0.2 * std::sin(TAU_ * 7.3 * duneT) * std::sin(TAU_ * 1.3 * duneT);
        x = std::tanh(1.4 * x * buzz) / 1.4;
        const float n = noise(); duneHp += (n - duneHp) * co.kDuneHiss;
        s += air * duneLvl * (0.26 * x + 0.06 * (n - duneHp) * (0.6 + 0.4 * duneLvl));
    } else duneT = 0;
    if (iceLvl > 0.002 || iceT >= 0) {
        if (iceLvl > 0.002) iceNext -= dt * (0.3 + 1.7 * iceLvl);
        if (iceNext <= 0 && iceT < 0) {
            const double r = uni();
            iceKind = r < 0.45 ? 0 : (r < 0.75 ? 1 : (r < 0.9 ? 2 : 3));
            iceT = 0; icePh = 0; iceLp = 0; iceY1 = iceY2 = 0;
            switch (iceKind) {
                case 0: iceF0 = 4000 + 3000 * uni(); iceFLo = 500 + 400 * uni(); iceDur = 0.08 + 0.12 * uni(); iceAmp = 0.35 + 0.35 * uni(); break;   // near: the crack and a quick chirp
                case 1: iceF0 = 2000 + 1500 * uni(); iceFLo = 180 + 220 * uni(); iceDur = 0.4 + 0.8 * uni(); iceAmp = 0.15 + 0.2 * uni(); break;    // far: a long dull chirp
                case 2: iceF0 = 200 + 120 * uni(); iceDur = 0.6 + 0.6 * uni(); iceAmp = 0.2 + 0.15 * uni(); break;                                   // the sheet's boom
                default: iceDur = 0.3 + 0.2 * uni(); iceAmp = 0.25 + 0.2 * uni(); break;                                                            // the snow settling
            }
            iceT0 = iceKind <= 1 ? iceDur / (std::sqrt(iceF0 / iceFLo) - 1) : 0;   // the chirp reaches its low pitch as it ends
            iceNext = 0.4 + 3.2 * uni();
            st.iceEvents++;
        }
        if (iceT >= 0) {
            const double u = iceT / iceDur;
            if (u >= 1) iceT = -1;
            else {
                double x = 0;
                switch (iceKind) {
                    case 0: case 1: {
                        const double q = iceT0 / (iceT + iceT0), f = iceF0 * q * q;
                        icePh += dt * f;
                        x = std::min(1.0, iceT / 0.0015) * std::exp(-1.5 * u) * std::sqrt(1 - u) * (std::sin(TAU_ * icePh) + 0.3 * std::sin(2 * TAU_ * icePh));
                        if (iceKind == 0) x += 0.9 * noise() * std::exp(-iceT * 700);
                        else { iceLp += (x - iceLp) * co.kIceFar; x = 1.6 * iceLp; }
                        break;
                    }
                    case 2: {
                        const double f = iceF0 * (0.4 + 0.6 * std::exp(-3.0 * u)) * (1 + 0.15 * std::sin(PI_ * std::min(1.0, 4 * u)));   // up for a moment, then gliding down
                        icePh += dt * f;
                        const double w = std::tanh(2.2 * std::sin(TAU_ * icePh)) * std::pow(std::sin(PI_ * u), 0.6) * std::exp(-1.5 * u);
                        x = 0.3 * w + 0.9 * resonate(w, 2.4 * f, 0.8 * f, dt, iceY1, iceY2);
                        break;
                    }
                    default: iceLp += (noise() - iceLp) * co.kIceThud; x = 4.0 * iceLp * std::exp(-4.0 * u) * std::min(1.0, iceT / 0.01); break;
                }
                s += air * iceAmp * (0.6 + 0.4 * iceLvl) * 0.5 * x;
                iceT += dt;
            }
        }
    }
    if (st.quakeHit > 0) { qkT = 0; qkAmp = st.quakeHit; qkPh = 0; st.quakeHit = 0; }
    if (quakeLvl > 0.002 || qkT >= 0) {
        const float n = noise();
        qkLp1 += (n - qkLp1) * co.kQuake; qkLp2 += (qkLp1 - qkLp2) * co.kQuake; qkLp3 += (qkLp2 - qkLp3) * co.kQuake;
        qkClock += dt;
        const double heave = 0.55 + 0.45 * std::sin(TAU_ * 0.9 * qkClock) * std::sin(TAU_ * 0.31 * qkClock + 0.7);
        double x = quakeLvl * heave * (6.5 * qkLp3 + (0.3 + 0.7 * air) * 1.8 * (qkLp1 - qkLp3));   // the ground's rumble, the air's roar over it
        if (qkT >= 0) {   // the P-wave: a thump far down and its crack
            if (qkT < 1.5) { qkPh += dt * (34 + 30 * std::exp(-5.0 * qkT)); x += qkAmp * (0.45 * std::exp(-3.5 * qkT) * std::sin(TAU_ * qkPh) + 3.0 * air * std::exp(-25.0 * qkT) * qkLp1); qkT += dt; }
            else qkT = -1;
        }
        if (air > 0.01) {   // the stones' rattle
            if (qkRatT < 0 && uni() < dt * 45 * quakeLvl * quakeLvl) { qkRatT = 0; qkRatA = 0.2 + 0.8 * uni(); qkRatF = 1400 + 2600 * uni(); qkRY1 = qkRY2 = 0; }
            if (qkRatT >= 0) { x += air * qkRatA * 0.5 * resonate(noise(), qkRatF, 700, dt, qkRY1, qkRY2) * std::exp(-180.0 * qkRatT); qkRatT += dt; if (qkRatT > 0.03) qkRatT = -1; }
        }
        s += x;
    } else qkClock = 0;
    return s;
}

void AudioSynth::coefs(double dt) {
    co.kF = 1 - std::exp(-dt / 0.02); co.kUp = 1 - std::exp(-dt / 0.006); co.kDown = 1 - std::exp(-dt / 0.012); co.kN = 1 - std::exp(-dt / 0.003);
    co.kDuneHz = 1 - std::exp(-dt / 0.5); co.kDuneHiss = 1 - std::exp(-TAU_ * 600 * dt); co.kIceFar = 1 - std::exp(-TAU_ * 1500 * dt); co.kIceThud = 1 - std::exp(-TAU_ * 150 * dt); co.kQuake = 1 - std::exp(-TAU_ * 80 * dt);   // Q-03
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
        duneLvl += (st.dunes - duneLvl) * k * 2; iceLvl += (st.ice - iceLvl) * k; quakeLvl += (st.quake - quakeLvl) * k * 4;   // Q-03
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
        if (duneLvl > 0.002 || iceLvl > 0.002 || iceT >= 0 || quakeLvl > 0.002 || qkT >= 0 || st.quakeHit > 0) s += groundSample(dt, st);   // Q-03 the sounds of the ground
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
