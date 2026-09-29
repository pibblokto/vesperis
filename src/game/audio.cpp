#include "audio.h"
#include <cmath>
#include <algorithm>

void AudioSynth::render(float* out, int frames, int sr, AudioState& st) {
    double dt = 1.0 / sr;
    if (st.beep > 0) { beepT = 0; beepKind = st.beep; st.beep = 0; }
    if (st.thunder > 0) { thunderT = 0; thunderAmp = st.thunder; st.thunder = 0; }
    if (st.step > 0) { stepT = 0; stepKind = st.step; stepGainCur = st.stepGain; st.step = 0; }
    double k = 1 - std::exp(-dt * 2.0);
    for (int i = 0; i < frames; i++) {
        humLvl += (st.hum - humLvl) * k;
        engLvl += (st.engine - engLvl) * k;
        windLvl += (st.wind - windLvl) * k;
        leavesLvl += (st.leaves - leavesLvl) * k;
        hoofLvl += (st.hoofs - hoofLvl) * k;
        rpmLvl += (st.engineRpm - rpmLvl) * k * 2;
        skidLvl += (st.skid - skidLvl) * k;
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
        out[i] = (float)(std::max(-1.0, std::min(1.0, s)) * st.master);
    }
}
