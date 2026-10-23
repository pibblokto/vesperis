#include "music.h"
#include "instruments.h"
#include "voice.h"
#include "tongue.h"
#include "core/rng.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cctype>

const char* const TIMBRE_FAMILY_NAMES[TF_COUNT] = {"A PLUCKED STRING", "A PIPE", "A BOWED STRING", "A BELL", "A REED", "A DRUM",
                                                   "VOICES", "BRASS", "KEYS", "AN ORGAN", "A MUSIC BOX", "AN ELECTRIC STRING", "ELECTRIC KEYS", "A SYNTHESISER", "A PAD",
                                                   "A SWARM OF TONES", "A SYNTHETIC VOICE", "A FIELD OF TONES", "A BULLROARER",
                                                   "A LOG DRUM", "RATTLES", "CLAPS", "A BASS DRUM", "A SNARE", "A CYMBAL", "A KICK", "HATS", "CLICKS", "A SUB PULSE", "JINGLES"};
const char* const PIECE_FORM_NAMES[] = {"A DANCE", "A LULLABY", "A WORK SONG", "A HYMN", "A HARVEST SONG", "A LOVE SONG", "A SONG OF THE FESTIVAL",
                                                "A LAMENT", "A MARCH", "A SONG OF WARNING", "THE LAST SONG",
                                                "A CHANT", "A RING DANCE", "A HUNTING SONG", "A KEENING", "A HEALING SONG",
                                                "A PROCESSION", "A BALLAD",
                                                "A TURNING DANCE", "AN ANTHEM", "A PARLOUR SONG", "A MUSIC BOX TUNE", "AN OVERTURE", "A FUNERAL MARCH",
                                                "A SONG", "A DANCE RECORD", "A JINGLE", "A THEME", "A PROTEST SONG", "A REQUIEM",
                                                "A PROCESS", "A SWARM", "A SYNTHETIC SONG", "A DRIFT", "A SIGNAL", "AN ELEGY",
                                                "A FIELD", "A FIELD THAT TURNS", "A FIELD OF BELLS", "A STANDING TONE",
                                                "A DAWN SONG", "THE LAST SUNRISE"};   // D-05
static_assert(sizeof(PIECE_FORM_NAMES) / sizeof(PIECE_FORM_NAMES[0]) == PF_COUNT, "a name a form");
bool timbrePercussive(int family) { return family == TF_DRUM || (family >= TF_LOG && family < TF_COUNT); }

namespace {

double centsOf(double r) { return 1200.0 * std::log2(r); }

// a timbre of the family, with its own numbers: the partials, the envelope, the vibrato, the breath (C-04's five and its drum)
Timbre makeTimbre(int family, Rng& rng) {
    Timbre t; t.family = family;
    switch (family) {
        case TF_PLUCKED: {   // a quick attack, a decay, slightly stretched partials
            double roll = rng.range(1.1, 1.9), inh = rng.range(0.0, 0.002);
            for (int k = 0; k < 6; k++) { t.ratio[k] = (k + 1) * (1 + inh * k * k); t.amp[k] = std::pow(1.0 / (k + 1), roll); }
            t.attack = 0.003; t.decay = rng.range(0.35, 1.1); t.sustain = 0; t.release = 0.4; t.gain = 0.34;
            break;
        }
        case TF_PIPE: {   // a soft attack, a hollow spectrum, breath and a gentle vibrato
            for (int k = 0; k < 6; k++) { t.ratio[k] = k + 1; t.amp[k] = k == 0 ? 1 : (k == 1 ? rng.range(0.15, 0.4) : rng.range(0.02, 0.12) / k); }
            t.attack = rng.range(0.03, 0.09); t.decay = 0.1; t.sustain = 0.85; t.release = rng.range(0.06, 0.14);
            t.vibRate = rng.range(4.0, 6.0); t.vibDepth = rng.range(0.002, 0.006); t.breath = rng.range(0.12, 0.3); t.gain = 0.3;
            break;
        }
        case TF_BOWED: {   // a slow attack, a saw's partials, a wider vibrato
            for (int k = 0; k < 6; k++) { t.ratio[k] = k + 1; t.amp[k] = 1.0 / (k + 1) * (k % 2 ? rng.range(0.6, 1.0) : 1.0); }
            t.attack = rng.range(0.08, 0.18); t.decay = 0.2; t.sustain = 0.9; t.release = rng.range(0.1, 0.2);
            t.vibRate = rng.range(4.5, 6.5); t.vibDepth = rng.range(0.003, 0.008); t.breath = rng.range(0.0, 0.05); t.gain = 0.2;
            break;
        }
        case TF_METAL: {   // a bell: inharmonic partials, a long ring
            const double R[6] = {1.0, 2.0, 3.0, 4.2, 5.4, 6.8};
            double stretch = rng.range(0.0, 0.25);
            for (int k = 0; k < 6; k++) { t.ratio[k] = R[k] + stretch * k * rng.range(0.3, 1.0); t.amp[k] = k == 0 ? 1 : rng.range(0.25, 0.6) / std::sqrt((double)k); }
            t.attack = 0.002; t.decay = rng.range(1.2, 3.0); t.sustain = 0; t.release = 1.2; t.gain = 0.26;
            break;
        }
        case TF_REED: {   // a reed: the odd partials strong, a nasal tone
            for (int k = 0; k < 6; k++) { t.ratio[k] = k + 1; t.amp[k] = (k % 2 == 0 ? 1.0 : rng.range(0.4, 0.8)) / (1 + 0.5 * k); }
            t.attack = rng.range(0.02, 0.05); t.decay = 0.15; t.sustain = 0.8; t.release = 0.08;
            t.vibRate = rng.range(5.0, 7.0); t.vibDepth = rng.range(0.002, 0.005); t.breath = rng.range(0.02, 0.08); t.gain = 0.22;
            break;
        }
        default: {   // the drum: a low skin and a high one (their pitches in Hz), their noise
            t.ratio[0] = rng.range(55, 90); t.ratio[1] = rng.range(160, 320); t.amp[0] = 1; t.amp[1] = rng.range(0.5, 1.0);
            t.attack = 0.001; t.decay = rng.range(0.12, 0.3); t.sustain = 0; t.release = 0.05; t.breath = rng.range(0.3, 0.8); t.gain = 0.5;
            break;
        }
    }
    t.name = TIMBRE_FAMILY_NAMES[family];
    return t;
}

// ---- H-05: the ages' instruments -------------------------------------------------------------------------------------------------

// a sung voice, the people's speakers' (`voiceOf`): its tract, its breath, its roughness, its nasal murmur; one singer or a few together
Timbre singer(Rng& rng, const Voice& V, int copies, const char* name) {
    Timbre t; t.family = TF_VOICE; t.name = name; t.copies = copies;
    t.detune = copies > 1 ? rng.range(7, 18) : 0;
    t.tract = clampd(V.tract, 0.84, 1.26);
    t.attack = rng.range(0.05, 0.1); t.decay = 0.3; t.sustain = 0.88; t.release = rng.range(0.12, 0.22);
    t.vibRate = rng.range(4.8, 6.4); t.vibDepth = rng.range(0.004, 0.01) + 0.006 * V.tremor;
    t.breath = clampd(0.03 + 0.22 * V.breath, 0.02, 0.28);
    t.index = clampd(V.rough, 0, 0.8);   // the pitch's wobble
    t.mod = clampd(V.nasal, 0, 0.8);     // a nasal murmur
    t.bright = rng.range(0.85, 1.15);
    t.gain = copies > 1 ? 0.62 : 0.45;
    return t;
}
Timbre flute(Rng& rng, bool bone, const char* name) {   // a bone flute breathes and wavers; a later flute is steadier
    Timbre t; t.family = TF_PIPE; t.name = name;
    for (int k = 0; k < 6; k++) { t.ratio[k] = k + 1; t.amp[k] = k == 0 ? 1 : (k == 1 ? rng.range(0.1, 0.3) : (k == 2 ? rng.range(0.04, 0.12) : 0.015)); }
    t.attack = rng.range(0.04, 0.09); t.decay = 0.1; t.sustain = 0.85; t.release = rng.range(0.07, 0.14);
    t.vibRate = rng.range(4.0, 5.6); t.vibDepth = rng.range(0.002, 0.006);
    t.breath = bone ? rng.range(0.3, 0.5) : rng.range(0.1, 0.2);
    t.index = bone ? rng.range(0.004, 0.011) : 0;
    t.gain = 0.3;
    return t;
}
Timbre musicalBow(Rng& rng) {   // a string on a bow, the mouth its sound box: a resonance moving over the partials
    Timbre t; t.family = TF_PLUCKED; t.kind = 1; t.name = "A MUSICAL BOW";
    for (int k = 0; k < 6; k++) { t.ratio[k] = k + 1; t.amp[k] = 1.0 / (k + 1); }
    t.attack = 0.004; t.decay = rng.range(0.6, 1.2); t.sustain = 0; t.release = 0.3; t.gain = 0.36;
    t.vibRate = rng.range(0.4, 1.2);
    return t;
}
Timbre bullroarer(Rng& rng) {
    Timbre t; t.family = TF_ROARER; t.name = "A BULLROARER"; t.tone = rng.range(75, 140); t.vibRate = rng.range(0.25, 0.6); t.vibDepth = rng.range(0.3, 0.55);
    t.attack = 1.2; t.decay = 1; t.sustain = 1; t.release = 1.2; t.gain = 0.3;
    return t;
}
Timbre skin(const Timbre& drum, int high, const char* name) {   // the people's own drum (C-04's), its low skin or its high one
    Timbre t = drum; t.family = TF_DRUM; t.kind = high; t.tone = high ? drum.ratio[1] : drum.ratio[0]; t.name = name;
    return t;
}
Timbre logDrum(Rng& rng, double hz) {
    Timbre t; t.family = TF_LOG; t.name = "A LOG DRUM"; t.tone = hz; t.mod = rng.range(2.2, 3.1); t.decay = rng.range(0.15, 0.32); t.gain = 0.42;
    return t;
}
Timbre rattle(Rng& rng) { Timbre t; t.family = TF_RATTLE; t.name = "RATTLES"; t.sweep = rng.range(0.04, 0.09); t.tone = rng.range(2600, 5200); t.decay = 0.006; t.gain = 0.6; return t; }
Timbre clap(Rng& rng, bool sticks) {
    Timbre t; t.family = TF_CLAP; t.kind = sticks ? 1 : 0; t.name = sticks ? "STICKS" : "CLAPPING HANDS";
    t.tone = sticks ? rng.range(1100, 2400) : rng.range(900, 1500); t.decay = sticks ? rng.range(0.03, 0.06) : rng.range(0.05, 0.08); t.gain = sticks ? 0.7 : 0.8;
    return t;
}
Timbre jingles(Rng& rng) { Timbre t; t.family = TF_JINGLE; t.name = "JINGLES"; t.tone = rng.range(380, 520); t.decay = rng.range(0.1, 0.18); t.gain = 0.2; return t; }
Timbre bassDrum(Rng& rng, const char* name) { Timbre t; t.family = TF_BASSDRUM; t.name = name; t.tone = rng.range(48, 66); t.decay = rng.range(0.3, 0.5); t.gain = 0.6; return t; }
Timbre snare(Rng& rng, const char* name, int kind) { Timbre t; t.family = TF_SNARE; t.name = name; t.kind = kind; t.tone = rng.range(160, 240); t.decay = rng.range(0.12, 0.2); t.bright = rng.range(0.8, 1.2); t.gain = 0.55; return t; }
Timbre cymbal(Rng& rng, const char* name, double decay) { Timbre t; t.family = TF_CYMBAL; t.name = name; t.tone = rng.range(330, 520); t.decay = decay; t.gain = 0.3; return t; }
Timbre kick(Rng& rng, const char* name) { Timbre t; t.family = TF_KICK; t.name = name; t.tone = rng.range(44, 58); t.mod = rng.range(110, 170); t.sweep = rng.range(0.02, 0.045); t.decay = rng.range(0.25, 0.45); t.gain = 0.7; return t; }
Timbre hat(Rng& rng, const char* name, bool open) { Timbre t; t.family = TF_HAT; t.name = name; t.kind = open ? 1 : 0; t.tone = rng.range(330, 470); t.decay = open ? rng.range(0.18, 0.3) : rng.range(0.025, 0.055); t.gain = open ? 0.26 : 0.34; return t; }
Timbre clickT(Rng& rng, int kind) { Timbre t; t.family = TF_CLICK; t.name = "CLICKS"; t.kind = kind; t.tone = rng.range(1800, 5000); t.decay = kind ? rng.range(0.008, 0.02) : rng.range(0.003, 0.009); t.gain = 0.3; return t; }
Timbre subPulse(Rng& rng) { Timbre t; t.family = TF_SUB; t.name = "A SUB PULSE"; t.tone = rng.range(38, 52); t.decay = rng.range(0.25, 0.55); t.gain = 0.55; return t; }
Timbre brass(Rng& rng, int type) {   // 0 a bright lead, 1 a mellow middle, 2 the bass
    Timbre t; t.family = TF_BRASS; t.name = "BRASS"; t.kind = type;
    t.bright = type == 0 ? rng.range(0.9, 1.3) : (type == 1 ? rng.range(0.45, 0.7) : rng.range(0.35, 0.5));
    t.attack = type == 2 ? rng.range(0.05, 0.08) : rng.range(0.03, 0.06); t.decay = 0.3; t.sustain = 0.82; t.release = rng.range(0.07, 0.12);
    t.vibRate = rng.range(4.5, 5.8); t.vibDepth = type == 0 ? rng.range(0.001, 0.004) : 0.001;
    t.gain = type == 2 ? 0.48 : (type == 0 ? 0.4 : 0.38);
    return t;
}
Timbre keys(Rng& rng, const char* name) {   // struck strings: inharmonic partials dying high first, two strings a key beating slowly
    Timbre t; t.family = TF_KEYS; t.name = name; t.index = rng.range(0.0002, 0.0007); t.copies = 2; t.detune = rng.range(0.8, 2.4);
    t.attack = 0.002; t.decay = rng.range(2.0, 4.0); t.sustain = 0; t.release = 0.18; t.bright = rng.range(0.8, 1.2); t.gain = 0.5;
    return t;
}
Timbre organ(Rng& rng, int kind, const char* name) {   // 0 a church organ's stops and its chiff, 1 a barrel organ's tremolo, 2 an electric organ's
    Timbre t; t.family = TF_ORGAN; t.name = name; t.kind = kind;
    const double R[6] = {0.5, 1, 2, 3, 4, 6};
    for (int k = 0; k < 6; k++) { t.ratio[k] = R[k]; t.amp[k] = rng.chance(k == 1 ? 1.0 : 0.55) ? rng.range(0.25, 1.0) / (k == 1 ? 1.0 : 1.4) : 0; }
    t.amp[1] = std::max(t.amp[1], 0.75);
    if (kind == 1) { t.amp[3] = std::max(t.amp[3], 0.35); t.amp[5] = std::max(t.amp[5], 0.2); }   // a barrel organ's reedy ranks
    t.attack = kind == 2 ? 0.01 : rng.range(0.03, 0.06); t.decay = 0.2; t.sustain = 1; t.release = kind == 0 ? rng.range(0.15, 0.3) : 0.06;
    t.vibRate = kind == 1 ? rng.range(5, 7) : (kind == 2 ? rng.range(6, 7.5) : 0); t.vibDepth = kind == 1 ? rng.range(0.12, 0.22) : (kind == 2 ? rng.range(0.05, 0.12) : 0);
    t.breath = kind == 0 ? rng.range(0.06, 0.16) : 0.02;
    t.gain = 0.34;
    return t;
}
Timbre musicBox(Rng& rng) { Timbre t; t.family = TF_MUSICBOX; t.name = "A MUSIC BOX"; t.decay = rng.range(0.5, 1.1); t.attack = 0.001; t.mod = rng.range(5.6, 6.6); t.bright = rng.range(0.15, 0.32); t.gain = 0.42; return t; }
Timbre estring(Rng& rng, int type, const char* name) {   // 0 a lead, overdriven; 1 strummed chords; 2 a bass
    Timbre t; t.family = TF_ESTRING; t.name = name; t.kind = type;
    t.index = type == 2 ? 0 : (type == 0 ? rng.range(2.0, 5.0) : rng.range(1.0, 2.2));
    t.attack = 0.003; t.decay = type == 2 ? rng.range(0.8, 1.5) : rng.range(1.2, 2.6); t.sustain = type == 0 ? 0.35 : 0.08; t.release = 0.1;
    t.vibRate = type == 0 ? rng.range(5, 6.5) : 0; t.vibDepth = type == 0 ? rng.range(0.002, 0.006) : 0;
    t.copies = type == 1 ? 2 : 1; t.detune = rng.range(3, 8);
    t.bright = type == 2 ? 0.5 : rng.range(0.8, 1.2);
    t.gain = type == 2 ? 0.55 : (type == 0 ? 0.3 : 0.4);
    return t;
}
Timbre ekeys(Rng& rng, bool glass, const char* name) {   // two operators: a tine (a ratio of one, now and then a bark) or struck glass (a ratio between the partials)
    Timbre t; t.family = TF_EKEYS; t.name = name; t.kind = glass ? 1 : 0;
    t.mod = glass ? rng.range(2.4, 7.3) : (rng.chance(0.7) ? 1.0 : 14.0);
    t.index = glass ? rng.range(1.5, 3.2) : rng.range(1.2, 2.6); t.sweep = glass ? rng.range(0.08, 0.3) : rng.range(0.4, 1.0);
    t.attack = 0.002; t.decay = glass ? rng.range(0.6, 1.6) : rng.range(1.5, 3.2); t.sustain = 0; t.release = 0.2;
    t.vibRate = glass ? 0 : rng.range(3.5, 5.5); t.vibDepth = glass ? 0 : rng.range(0.03, 0.12);
    t.gain = glass ? 0.34 : 0.36;
    return t;
}
Timbre synth(Rng& rng, int type, const char* name) {   // 0 a lead, 1 a bass, 2 a pluck, 3 a sub
    Timbre t; t.family = TF_SYNTH; t.name = name; t.kind = type;
    t.mod = type == 3 ? 2 : (rng.chance(0.5) ? 0 : 1);   // a saw, a square; a sub's sine
    t.bright = type == 0 ? rng.range(5, 10) : (type == 1 ? rng.range(2.5, 5) : (type == 2 ? rng.range(6, 14) : 1.2));
    t.sweep = type == 2 ? rng.range(0.06, 0.18) : rng.range(0.15, 0.6);
    t.index = rng.range(0.3, 1.0);   // the resonance
    t.attack = type == 0 ? rng.range(0.005, 0.04) : 0.003; t.decay = type == 2 ? rng.range(0.25, 0.6) : rng.range(0.3, 0.8);
    t.sustain = type == 2 ? 0 : (type == 0 ? 0.7 : 0.6); t.release = type == 0 ? 0.12 : 0.06;
    t.copies = type == 0 ? 2 : 1; t.detune = rng.range(5, 12);
    t.vibRate = type == 0 ? rng.range(4.5, 6) : 0; t.vibDepth = type == 0 ? rng.range(0.001, 0.004) : 0;
    t.gain = type == 1 ? 0.3 : (type == 3 ? 0.3 : (type == 2 ? 0.34 : 0.26));
    return t;
}
Timbre pad(Rng& rng, const char* name) { Timbre t; t.family = TF_PAD; t.name = name; t.copies = 3; t.detune = rng.range(6, 14); t.bright = rng.range(2.5, 5); t.attack = rng.range(0.4, 1.1); t.decay = 1; t.sustain = 1; t.release = rng.range(0.8, 1.6); t.vibRate = rng.range(0.1, 0.3); t.gain = 0.2; return t; }
Timbre swarm(Rng& rng) { Timbre t; t.family = TF_SWARM; t.name = "A SWARM OF TONES"; t.copies = 10; t.detune = rng.range(18, 50); t.attack = rng.range(0.3, 0.9); t.decay = 1; t.sustain = 1; t.release = rng.range(0.8, 1.5); t.vibRate = rng.range(0.08, 0.35); t.bright = rng.range(0.1, 0.4); t.gain = 0.55; return t; }
Timbre synVoice(Rng& rng, const Voice& V) {
    Timbre t = singer(rng, V, 1, "A SYNTHETIC VOICE"); t.family = TF_SYNVOICE;
    t.vibDepth = 0; t.index = 0; t.breath = 0; t.tone = rng.range(4500, 9000); t.vibRate = rng.range(40, 110); t.sweep = rng.range(0.1, 0.35);
    t.attack = 0.012; t.release = 0.05; t.gain = 0.55;
    return t;
}
Timbre field(Rng& rng, int type, const char* name) { Timbre t; t.family = TF_FIELD; t.name = name; t.kind = type; t.tone = rng.range(0.15, 1.1); t.attack = rng.range(3, 6); t.decay = 1; t.sustain = 1; t.release = rng.range(3.5, 5.5); t.vibRate = rng.range(0.03, 0.09); t.bright = type == 1 ? rng.range(0.3, 0.6) : rng.range(0.05, 0.25); t.gain = 0.16; return t; }

// a medieval drone: a bag's reed or a wheel's bow, held
Timbre droneOf(Rng& rng, const bool* likes) {
    const bool reed = likes[TF_REED] || (!likes[TF_BOWED] && rng.chance(0.5));
    Timbre t = makeTimbre(reed ? TF_REED : TF_BOWED, rng); t.name = "A DRONE"; t.attack = 0.3; t.release = 0.4; t.vibDepth = 0; t.gain *= 0.75;
    return t;
}

// an age's ensemble: what the people played then, leaning to the families its own (C-04's) voices were of
Ensemble makeEnsemble(const Tradition& T, int age, Rng& rng, const Voice& V, int style) {
    Ensemble E; E.age = age;
    auto add = [&](const Timbre& t, int r) { E.voices.push_back(t); E.role.push_back(r); };
    auto kit = [&](const Timbre& t, int r) { E.kit.push_back(t); E.kitRole.push_back(r); };
    bool likes[TF_COUNT] = {};
    for (const Timbre& t : T.voices) likes[t.family] = true;
    switch (age) {
        case AGE_PREHISTORIC: {
            add(singer(rng, V, 1, "A VOICE"), MR_LEAD);    // the caller
            add(singer(rng, V, 3, "VOICES"), MR_CHORUS);   // the chorus that answers
            add(flute(rng, true, "A BONE FLUTE"), MR_ALT);
            if (rng.chance(likes[TF_PLUCKED] ? 0.6 : 0.25)) add(musicalBow(rng), MR_SECOND);
            if (rng.chance(0.3)) add(bullroarer(rng), MR_DRONE);
            kit(skin(T.drum, 0, "SKIN DRUMS"), KR_LOW);
            if (rng.chance(style == 1 ? 0.9 : 0.7)) kit(skin(T.drum, 1, "SKIN DRUMS"), KR_HIGH);
            if (rng.chance(0.5)) { const int n = (int)T.scale.size(); kit(logDrum(rng, traditionHz(T, -n)), KR_TUNED); kit(logDrum(rng, traditionHz(T, T.rest - n)), KR_TUNED); }
            if (rng.chance(0.75)) kit(rattle(rng), KR_SHAKE);
            if (rng.chance(0.5)) kit(clap(rng, rng.chance(0.55)), KR_TICK);
            E.tempo = rng.range(0.95, 1.12); E.drift = T.drift * 1.2;
            break;
        }
        case AGE_MEDIEVAL: {   // C-04's voices first, in their order (the medieval texture is C-04's), then a choir, a singer, a drone, a bell
            for (size_t i = 0; i < T.voices.size(); i++) add(T.voices[i], i == 0 ? MR_LEAD : (i == 1 ? MR_SECOND : MR_ALT));
            add(singer(rng, V, 3, "A CHOIR"), MR_CHORUS);
            add(singer(rng, V, 1, "A VOICE"), MR_SINGER);
            if (T.drone) add(droneOf(rng, likes), MR_DRONE);
            if (!likes[TF_METAL]) { Timbre b = makeTimbre(TF_METAL, rng); b.name = "BELLS"; b.gain *= 0.8; add(b, MR_BELL); }
            kit(skin(T.drum, 0, "A TABOR"), KR_LOW);
            kit(skin(T.drum, 1, "A TABOR"), KR_HIGH);
            if (rng.chance(0.5)) kit(jingles(rng), KR_SHAKE);
            E.tempo = 1; E.drift = T.drift;
            break;
        }
        case AGE_INDUSTRIAL: {
            double bw[5] = {1.5 + (style == 1 ? 0.6 : 0), 1.1 + (likes[TF_BOWED] ? 0.6 : 0), 1.1 + (likes[TF_PIPE] ? 0.5 : 0), 0.8 + (likes[TF_METAL] ? 0.5 : 0), 0.9 + (likes[TF_REED] ? 0.6 : 0)};
            E.band = rng.pick(bw, 5);
            switch (E.band) {
                case 0:   // a brass band
                    add(brass(rng, 0), MR_LEAD); add(brass(rng, 1), MR_SECOND); add(brass(rng, 1), MR_CHORDS); add(brass(rng, 2), MR_BASS);
                    kit(bassDrum(rng, "A BASS DRUM"), KR_LOW); kit(snare(rng, "A SIDE DRUM", 1), KR_BACK); kit(cymbal(rng, "CYMBALS", rng.range(0.8, 1.4)), KR_CRASH);
                    break;
                case 1: {   // strings and keys
                    Timbre v = makeTimbre(TF_BOWED, rng); v.name = "A BOWED STRING"; v.gain = 0.24; add(v, MR_LEAD);
                    Timbre v2 = makeTimbre(TF_BOWED, rng); v2.name = "A BOWED STRING"; add(v2, MR_SECOND);
                    add(keys(rng, "KEYS"), MR_CHORDS); add(keys(rng, "KEYS"), MR_BASS);
                    if (rng.chance(0.5)) kit(skin(T.drum, 0, "A KETTLE DRUM"), KR_LOW);
                    break;
                }
                case 2: {   // an orchestra: a woodwind's lead, a section of strings, a bowed bass, a horn
                    if (likes[TF_REED] || rng.chance(0.4)) { Timbre r = makeTimbre(TF_REED, rng); r.name = "A REED"; add(r, MR_LEAD); }
                    else add(flute(rng, false, "A FLUTE"), MR_LEAD);
                    add(brass(rng, 1), MR_SECOND);
                    Timbre s = makeTimbre(TF_BOWED, rng); s.name = "STRINGS"; s.copies = 3; s.detune = rng.range(6, 12); s.gain = 0.32; add(s, MR_CHORDS);
                    Timbre b = makeTimbre(TF_BOWED, rng); b.name = "STRINGS"; b.gain = 0.3; add(b, MR_BASS);
                    kit(skin(T.drum, 0, "A KETTLE DRUM"), KR_LOW); kit(cymbal(rng, "CYMBALS", rng.range(1.0, 1.8)), KR_CRASH);
                    break;
                }
                case 3: {   // a barrel organ, its pipes for the tune, the chords and the bass, its drum and its cymbal struck by the barrel
                    Timbre o = organ(rng, 1, "A BARREL ORGAN");
                    add(o, MR_LEAD); add(o, MR_SECOND); add(o, MR_CHORDS); Timbre ob = o; ob.gain *= 1.4; add(ob, MR_BASS);
                    kit(bassDrum(rng, "A BARREL ORGAN'S DRUM"), KR_LOW); kit(snare(rng, "A BARREL ORGAN'S DRUM", 1), KR_BACK); kit(cymbal(rng, "A BARREL ORGAN'S DRUM", 0.5), KR_CRASH);
                    break;
                }
                default: {   // free reeds in a box of bellows: the tune, the chords and the bass from one instrument, two reeds a note beating (the musette's waver)
                    Timbre r = makeTimbre(TF_REED, rng); r.name = "FREE REEDS"; r.copies = 2; r.detune = rng.range(10, 26); r.attack = rng.range(0.03, 0.07); r.sustain = 0.9; r.vibDepth = 0; r.gain = 0.24;
                    add(r, MR_LEAD); add(r, MR_SECOND); Timbre rc = r; rc.gain *= 0.8; add(rc, MR_CHORDS); Timbre rb = r; rb.copies = 1; rb.gain *= 1.3; add(rb, MR_BASS);
                    if (rng.chance(0.6)) kit(skin(T.drum, 0, "A FRAME DRUM"), KR_LOW);
                    break;
                }
            }
            add(organ(rng, 0, "AN ORGAN"), MR_ORGAN);
            add(musicBox(rng), MR_BOX);
            add(singer(rng, V, 1, "A SINGER"), MR_SINGER);
            if (E.band != 1) add(keys(rng, "KEYS"), MR_KEYS);
            E.tempo = rng.range(0.95, 1.1); E.drift = T.drift * 0.6;
            break;
        }
        case AGE_MODERN: {
            E.band = rng.chance(0.62) ? 0 : 1;
            const char* kn = E.band == 0 ? "A DRUM MACHINE" : "A DRUM KIT";
            if (E.band == 0) { kit(kick(rng, kn), KR_LOW); kit(rng.chance(0.55) ? snare(rng, kn, 0) : clap(rng, false), KR_BACK); }
            else { kit(bassDrum(rng, kn), KR_LOW); kit(snare(rng, kn, 0), KR_BACK); }
            kit(hat(rng, kn, false), KR_TICK); kit(hat(rng, kn, true), KR_OPEN); kit(cymbal(rng, kn, rng.range(0.9, 1.5)), KR_CRASH);
            for (Timbre& k : E.kit) k.name = kn;
            add(rng.chance(0.55) ? synth(rng, 1, "A SYNTH BASS") : estring(rng, 2, "AN ELECTRIC BASS"), MR_BASS);
            {   double cw[4] = {1.4, 1.2, 1.2, 0.6};
                switch (rng.pick(cw, 4)) {
                    case 0: add(ekeys(rng, false, "ELECTRIC KEYS"), MR_CHORDS); break;
                    case 1: add(pad(rng, "A PAD"), MR_CHORDS); break;
                    case 2: add(estring(rng, 1, "AN ELECTRIC STRING"), MR_CHORDS); break;
                    default: add(organ(rng, 2, "AN ELECTRIC ORGAN"), MR_CHORDS); break;
                }
            }
            double lw[3] = {0.45, 0.3, 0.25};
            const int lead = rng.pick(lw, 3);
            if (lead == 0) add(singer(rng, V, 1, "A SINGER"), MR_LEAD);
            else if (lead == 1) add(synth(rng, 0, "A SYNTHESISER"), MR_LEAD);
            else add(estring(rng, 0, "AN ELECTRIC STRING"), MR_LEAD);
            add(lead == 1 ? estring(rng, 0, "AN ELECTRIC STRING") : synth(rng, rng.chance(0.5) ? 0 : 2, "A SYNTHESISER"), MR_SECOND);   // riffs and fills
            if (E.voiceOf(MR_CHORDS) < 0 || E.voices[E.voiceOf(MR_CHORDS)].family != TF_PAD) add(pad(rng, "A PAD"), MR_ALT);
            if (lead != 0) add(singer(rng, V, 1, "A SINGER"), MR_SINGER);
            { double ww[4] = {1.2, 1.0, 0.8, 0.9}; E.comp = rng.pick(ww, 4); }
            E.tempo = rng.range(1.0, 1.2); E.drift = E.band == 0 ? 0 : T.drift * 0.3;
            break;
        }
        case AGE_FUTURISTIC: {   // a synthetic voice or a pure tone to lead; glass, a pluck or struck metal for the cells; a swarm or a choir of synthetic voices
            if (rng.chance(0.62)) add(synVoice(rng, V), MR_LEAD);
            else { Timbre p = synth(rng, 0, "A PURE TONE"); p.mod = 2; p.copies = 1; p.vibDepth = 0; p.gain = 0.3; add(p, MR_LEAD); }
            auto cellVoice = [&]() { const double u = rng.uni(); if (u < 0.5) return ekeys(rng, true, "STRUCK GLASS"); if (u < 0.8) return synth(rng, 2, "A SYNTHESISER"); Timbre m = makeTimbre(TF_METAL, rng); m.name = "STRUCK METAL"; m.decay = rng.range(0.4, 1.0); m.release = 0.4; m.gain = 0.3; return m; };
            add(cellVoice(), MR_SECOND);
            if (rng.chance(0.7)) add(swarm(rng), MR_SWARM);
            else { Timbre c = synVoice(rng, V); c.name = "SYNTHETIC VOICES"; c.copies = 3; c.detune = rng.range(6, 16); c.attack = rng.range(0.3, 0.8); c.release = rng.range(0.6, 1.2); c.gain = 0.42; add(c, MR_SWARM); }
            if (rng.chance(0.65)) add(pad(rng, "A PAD"), MR_CHORDS);
            add(synth(rng, 3, "A SUB TONE"), MR_BASS);
            add(cellVoice(), MR_ALT);
            kit(clickT(rng, 0), KR_TICK); kit(clickT(rng, 1), KR_HIGH); kit(subPulse(rng), KR_SUB);
            E.tempo = rng.range(0.9, 1.15); E.drift = 0;
            break;
        }
        default: {   // the far future: two fields (full, hollow or of bell-like tones), under them a deep tone or a hum, over them glints or a far choir of swarming tones
            add(field(rng, rng.irange(3), "A FIELD OF TONES"), MR_LEAD);
            add(field(rng, rng.irange(3), "A FIELD OF TONES"), MR_SECOND);
            if (rng.chance(0.7)) { Timbre d = field(rng, 0, "A DEEP TONE"); d.gain = 0.2; d.bright = 0.08; d.tone = rng.range(0.08, 0.25); add(d, MR_BASS); }
            else { Timbre h = bullroarer(rng); h.name = "A HUM"; h.tone = rng.range(42, 70); h.vibRate = rng.range(0.03, 0.08); h.vibDepth = rng.range(0.05, 0.15); h.attack = 4; h.release = 4; h.gain = 0.28; add(h, MR_BASS); }
            if (rng.chance(0.6)) { Timbre gl = makeTimbre(TF_METAL, rng); gl.name = "GLINTS"; gl.decay = rng.range(3, 6); gl.release = 2; gl.gain = 0.09; add(gl, MR_BELL); }
            else { Timbre c = swarm(rng); c.name = "A FAR CHOIR"; c.attack = rng.range(1.5, 3.0); c.release = rng.range(2, 3.5); c.detune = rng.range(8, 20); c.gain = 0.22; add(c, MR_BELL); }
            E.tempo = 1; E.drift = 0;
            break;
        }
    }
    return E;
}

// ---- the piece: forms by the tone and the age, each age's texture ----------------------------------------------------------------

struct FormP { double tempo, density, vel0, vel1, restP; int reg, drum; bool repeat; };   // drum: 0 never, 1 the tradition's, 2 always; repeat: the answer and the return once more
const FormP C04_FP[11] = {
    {1.25, 1.5, 0.65, 0.95, 0.06, 0, 2, true},     // a dance
    {0.62, 0.8, 0.42, 0.62, 0.1, 1, 0, false},     // a lullaby: slow, soft, high
    {1.0, 1.1, 0.6, 0.85, 0.08, 0, 2, true},       // a work song
    {0.7, 0.55, 0.5, 0.75, 0.08, -1, 0, false},    // a hymn: long notes, low
    {1.1, 1.2, 0.6, 0.9, 0.08, 0, 1, true},        // a harvest song
    {0.82, 0.9, 0.5, 0.72, 0.1, 0, 1, false},      // a love song
    {1.35, 1.6, 0.7, 1.0, 0.05, 0, 2, true},       // a song of the festival: quick, loud, drummed
    {0.55, 0.65, 0.45, 0.65, 0.15, -1, 0, false},  // a lament
    {1.0, 1.0, 0.65, 0.95, 0.05, 0, 2, true},      // a march
    {0.9, 0.9, 0.55, 0.85, 0.1, -1, 1, false},     // a song of warning
    {0.5, 0.45, 0.4, 0.6, 0.25, -1, 0, false},     // the last song: slow, sparse, low
};

struct Ev { int unit, len, deg; bool rest; };

// the builder every texture writes through: the tradition's cycle in subdivision units, C-04's walk and phrases, chords stacked from the
// people's own scale, notes added in beats
struct Bld {
    const Tradition& T; const Ensemble& E; Piece& P; Rng& rng;
    int n, sub, beats, cu, cpp;
    std::vector<int> gs;
    int third[16], fifth[16];
    std::vector<int> roots;   // the scale's degrees by how well a chord stands on them (the tonic first)
    Bld(const Tradition& t, const Ensemble& e, Piece& p, Rng& r) : T(t), E(e), P(p), rng(r) {
        n = std::max(1, (int)T.scale.size()); sub = T.sub; beats = T.beats; cu = beats * sub; cpp = beats <= 3 ? 4 : (beats <= 6 ? 2 : 1);
        int u = 0; for (int g : T.groups) { gs.push_back(u); u += g * sub; }
        // the chord on each degree: the degree nearest a third above it (230-470 cents) and the one nearest a fifth, in the people's tuning
        std::vector<std::pair<double, int>> q;
        for (int r0 = 0; r0 < n && r0 < 16; r0++) {
            double c0 = traditionCents(T, r0), best3 = 1e9, best5 = 1e9; int d3 = r0 + std::min(2, n - 1), d5 = r0 + std::min(4, n - 1);
            for (int d = r0 + 1; d < r0 + n; d++) {
                const double c = traditionCents(T, d) - c0;
                if (c >= 230 && c <= 470 && std::fabs(c - 350) < best3) { best3 = std::fabs(c - 350); d3 = d; }
                if (std::fabs(c - 702) < best5) { best5 = std::fabs(c - 702); d5 = d; }
            }
            third[r0] = d3; fifth[r0] = d5;
            q.push_back({best5 + 0.5 * std::min(best3, 200.0) + (r0 == 0 ? -1000 : 0), r0});
        }
        std::sort(q.begin(), q.end());
        for (auto& x : q) roots.push_back(x.second);
    }
    bool isGS(int u) const { u = ((u % cu) + cu) % cu; for (int g : gs) if (g == u) return true; return false; }
    // how many units of `beatsEach` beats make about `secs` seconds at the piece's tempo (a long cycle or a slow tempo takes fewer)
    int bySeconds(double secs, double beatsEach, int lo, int hi) const { return std::max(lo, std::min(hi, (int)std::lround(secs * P.bpm / 60.0 / std::max(1.0, beatsEach)))); }
    int midGroup() const { int best = gs.size() > 1 ? gs[1] : cu / 2; for (int g : gs) if (g > 0 && std::abs(g - cu / 2) < std::abs(best - cu / 2)) best = g; return best; }
    void add(double t, double dur, int voice, int deg, double vel, int slide = -1000, int vowel = -1, double bend = 0) {
        if (voice < 0 || voice >= (int)E.voices.size() || dur <= 1e-6 || t < 0) return;
        Note nt; nt.t = t; nt.dur = dur; nt.voice = voice; nt.degree = clampi(deg, -3 * n, 3 * n); nt.vel = clampd(vel, 0.05, 1.0); nt.slideFrom = slide; nt.vowel = vowel; nt.bend = bend;
        P.notes.push_back(nt);
    }
    void hit(double t, int piece, double vel) {
        if (piece < 0 || piece >= (int)E.kit.size() || t < 0) return;
        Note nt; nt.t = t; nt.dur = 0.25; nt.voice = -1; nt.degree = piece; nt.vel = clampd(vel, 0.05, 1.0); P.notes.push_back(nt);
    }
    int vowel() { return T.vowels.empty() ? 0 : T.vowels[rng.irange((int)T.vowels.size())]; }
    double accent(int unit) const { const int uc = ((unit % cu) + cu) % cu; return uc == 0 ? 1.0 : (isGS(uc) ? 0.92 : 0.82); }
    // C-04's: a group's rhythm, the walk, a phrase and a variation
    void groupRhythm(int gb, double density, bool closing, std::vector<int>& out) {
        out.clear(); int units = gb * sub;
        if (closing && gb <= 2) { if (rng.chance(0.6)) out.push_back(units); else { out.push_back(sub); out.push_back(units - sub); } return; }
        if (closing) units -= 2 * sub;   // H-05: a long group closes on its last two beats held, the beats before it moving (a cycle of one group still sings)
        for (int rem = units; rem > 0;) {
            int lens[4] = {1, sub, 2 * sub, 3 * sub};
            double wl[4] = {density * 0.9, 1.2, 0.7 / density, 0.3 / density};
            for (int k = 0; k < 4; k++) if (lens[k] > rem) wl[k] = 0;
            int len = lens[rng.pick(wl, 4)];
            if (len == 1) { int k = std::min(rem, sub); for (int i = 0; i < k; i++) out.push_back(1); rem -= k; }
            else { out.push_back(len); rem -= len; }
        }
        if (closing) out.push_back(2 * sub);
    }
    int nextDegree(int cur, int lastStep, int centre) {
        double wd[3] = {0.5, 0.15, 0.35};
        if (std::abs(lastStep) >= 2) { wd[0] = 0.8; wd[1] = 0.05; wd[2] = 0.15; }
        int c = rng.pick(wd, 3), d = 0;
        if (c == 0) d = std::abs(lastStep) >= 2 ? (lastStep > 0 ? -1 : 1) : (rng.chance(0.5) ? 1 : -1);
        else if (c == 2) { d = 2 + rng.irange(3); if (rng.chance(cur > centre ? 0.7 : 0.3)) d = -d; }
        int nd = cur + d;
        if (nd < centre - n / 2 - 1) nd = cur + std::abs(d);
        if (nd > centre + n + 1) nd = cur - std::abs(d);
        return nd;
    }
    void phrase(int startDeg, int endDeg, int centre, double density, double restP, int cycles, std::vector<Ev>& ev) {
        ev.clear(); int unit = 0, cur = startDeg, lastStep = 0; bool first = true;
        for (int c = 0; c < cycles; c++)
            for (size_t gi = 0; gi < T.groups.size(); gi++) {
                bool closing = c == cycles - 1 && gi + 1 == T.groups.size();
                std::vector<int> r; groupRhythm(T.groups[gi], density, closing, r);
                for (size_t k = 0; k < r.size(); k++) {
                    bool last = closing && k + 1 == r.size();
                    int deg = first ? startDeg : (last ? endDeg : nextDegree(cur, lastStep, centre));
                    bool rest = !first && !last && r[k] <= sub && rng.chance(restP);   // a rest takes a beat at most
                    ev.push_back({unit, r[k], deg, rest});
                    if (!rest) { lastStep = deg - cur; cur = deg; }
                    first = false; unit += r[k];
                }
            }
    }
    void vary(const std::vector<Ev>& a, int endDeg, std::vector<Ev>& out) {
        out.clear();
        for (size_t i = 0; i < a.size(); i++) {
            Ev e = a[i];
            if (i > 0 && i + 1 < a.size() && !e.rest && rng.chance(0.3)) e.deg += rng.chance(0.5) ? 1 : -1;
            if (i + 1 == a.size()) e.deg = endDeg;
            if (!e.rest && e.len >= 2 && e.len % 2 == 0 && i + 1 < a.size() && rng.chance(0.2)) {
                out.push_back({e.unit, e.len / 2, e.deg, false});
                out.push_back({e.unit + e.len / 2, e.len / 2, e.deg + (rng.chance(0.5) ? 1 : -1), false});
            } else out.push_back(e);
        }
    }
    // a phrase's events as notes of a voice from `pos` (beats); `sung` a vowel to each note (a new one now and then)
    void place(const std::vector<Ev>& ev, double pos, int voice, double vel0, double vel1, int reg, bool sung, double legato = 1.0) {
        int vw = sung ? vowel() : -1;
        for (const Ev& e : ev) {
            if (e.rest) continue;
            if (sung && rng.chance(0.45)) vw = vowel();
            add(pos + (double)e.unit / sub, (double)e.len / sub * legato, voice, e.deg + reg, rng.range(vel0, vel1) * accent(e.unit), -1000, vw);
        }
    }
    // the chord on a root: the root, its third, its fifth (degrees, the root's period)
    void chord(int root, int out[3]) const { const int r = ((root % n) + n) % n, oct = root - r; out[0] = root; out[1] = third[r] + oct; out[2] = fifth[r] + oct; }
    bool inChord(int deg, int root) const { int c[3]; chord(root, c); for (int k = 0; k < 3; k++) if ((((deg - c[k]) % n) + n) % n == 0) return true; return false; }
    int nearestChordTone(int deg, int root) const {
        for (int d = 0; d <= n; d++) { if (inChord(deg - d, root)) return deg - d; if (inChord(deg + d, root)) return deg + d; }
        return deg;
    }
    // a progression of `len` roots: the tonic first; open on the resting degree's chord, closed on the tonic; between them the scale's best
    // roots (the people's own turns: `fav` the progression its songs come back to, drawn once from the tradition)
    std::vector<int> progression(int len, bool closed, Rng& r) const {
        std::vector<int> p(std::max(1, len), 0);
        std::vector<int> mid; for (int x : roots) if (x != 0 && x != T.rest && (int)mid.size() < 3) mid.push_back(x);
        if (mid.empty()) mid.push_back(T.rest);
        for (int i = 1; i < len; i++) {
            if (i == len - 1) { p[i] = closed ? 0 : T.rest; break; }
            int x = mid[r.irange((int)mid.size())];
            if (x == p[i - 1] && mid.size() > 1) x = mid[(r.irange((int)mid.size() - 1) + 1) % mid.size()];
            p[i] = x;
        }
        if (len >= 2 && closed && p[len - 2] == 0) p[len - 2] = T.rest;
        return p;
    }
    // the people's ornament rule on a voice's notes (C-04's), at a share of its chance
    void ornament(int voice, double share) {
        if (T.ornament == 0 || share <= 0) return;
        std::vector<Note> all; all.swap(P.notes);
        int prevDeg = -1000; double prevEnd = -1;
        for (size_t i = 0; i < all.size(); i++) {
            Note nt = all[i];
            if (nt.voice != voice) { P.notes.push_back(nt); continue; }
            bool adjacent = prevDeg != -1000 && std::fabs(nt.t - prevEnd) < 1e-6;
            int step = prevDeg == -1000 ? 0 : nt.degree - prevDeg;
            bool orn = i + 1 < all.size() && rng.chance(T.ornamentChance * share);
            if (orn && T.ornament == 1 && std::abs(step) >= 2 && nt.dur >= 0.5) {
                Note gr = nt; gr.dur = 0.12; gr.degree = nt.degree + (step > 0 ? -1 : 1); gr.vel *= 0.7; P.notes.push_back(gr);
                nt.t += 0.12; nt.dur -= 0.12; P.notes.push_back(nt);
            } else if (orn && T.ornament == 2 && nt.dur >= 1.5) {
                double tt = nt.t, end = nt.t + nt.dur; int k = 0;
                while (end - tt > 1e-6) { Note tr = nt; tr.t = tt; tr.dur = end - tt < 0.5 ? end - tt : 0.25; tr.degree = nt.degree + (k % 2); tr.vel *= k == 0 ? 1.0 : 0.8; P.notes.push_back(tr); tt += tr.dur; k++; }
            } else if (orn && T.ornament == 3 && std::abs(step) == 1 && adjacent) {
                nt.slideFrom = prevDeg; P.notes.push_back(nt);
            } else if (orn && T.ornament == 4 && nt.dur >= 1.0) {
                Note m1 = nt; m1.dur = 0.15; P.notes.push_back(m1);
                Note m2 = nt; m2.t += 0.15; m2.dur = 0.15; m2.degree = nt.degree - 1; m2.vel *= 0.8; P.notes.push_back(m2);
                nt.t += 0.3; nt.dur -= 0.3; P.notes.push_back(nt);
            } else P.notes.push_back(nt);
            prevDeg = all[i].degree; prevEnd = all[i].t + all[i].dur;
        }
    }
};

// a tempo map over the notes: `rate(b)` the tempo at beat b against the piece's (above one quicker); the notes' starts and lengths move
// to the clock's beats (a dance that quickens, a music box winding down, a song's last bars held back)
template <class F> void warp(Piece& P, double total, F rate) {
    const double step = 0.05; std::vector<double> clock(1, 0.0);
    for (double b = 0; b < total + 8; b += step) clock.push_back(clock.back() + step / std::max(0.2, rate(b + step * 0.5)));
    auto at = [&](double b) { if (b <= 0) return 0.0; const double x = b / step; const size_t i = std::min((size_t)x, clock.size() - 2); const double f = x - i; return clock[i] + (clock[i + 1] - clock[i]) * f; };
    for (Note& nt : P.notes) { const double s = at(nt.t), e = at(nt.t + nt.dur); nt.t = s; nt.dur = std::max(0.02, e - s); }
}

// ---- the prehistoric: a caller and a chorus, a cell danced faster and faster, a voice alone ----

// the drums of a cycle at `pos`: the low skin on the groups' first beats, the high on the other beats, the rattle on the subdivisions, the
// sticks on the beats, the log drum's two notes in a pattern of its own; `loud` the piece's level, `fill` the cycle's end crowded
void stoneDrums(Bld& b, double pos, double loud, bool fill, const std::vector<int>& logPat) {
    const int low = b.E.kitOf(KR_LOW), high = b.E.kitOf(KR_HIGH), shake = b.E.kitOf(KR_SHAKE), tick = b.E.kitOf(KR_TICK);
    int tuned[2] = {-1, -1}; { int k = 0; for (size_t i = 0; i < b.E.kitRole.size() && k < 2; i++) if (b.E.kitRole[i] == KR_TUNED) tuned[k++] = (int)i; }
    for (int u = 0; u < b.cu; u++) {
        const double t = pos + (double)u / b.sub;
        const bool g = b.isGS(u), beat = u % b.sub == 0;
        if (g) b.hit(t, low, loud * (u == 0 ? 1.0 : 0.85));
        else if (beat && high >= 0 && (fill || b.rng.chance(0.6))) b.hit(t, high, loud * 0.7);
        else if (fill && high >= 0 && b.rng.chance(0.5)) b.hit(t, high, loud * 0.55);
        if (shake >= 0) b.hit(t, shake, loud * (beat ? 0.55 : 0.35));
        if (tick >= 0 && beat && (g || b.rng.chance(0.35))) b.hit(t, tick, loud * 0.6);
        if (tuned[0] >= 0 && u < (int)logPat.size() && logPat[u] >= 0) b.hit(t, logPat[u] == 1 && tuned[1] >= 0 ? tuned[1] : tuned[0], loud * 0.75);
    }
}

void buildPrehistoric(Bld& b, int form) {
    const Tradition& T = b.T; const Ensemble& E = b.E; Rng& rng = b.rng;
    const int caller = E.voiceOf(MR_LEAD), chorus = E.voiceOf(MR_CHORUS), flute = E.voiceOf(MR_ALT), bow = E.voiceOf(MR_SECOND), roar = E.voiceOf(MR_DRONE);
    const int n = b.n, cyc = b.beats;
    std::vector<int> logPat(b.cu, -1);   // the log drum's two notes: the tonic on the cycle's first beat, the rest on some of the other groups
    for (int u = 0; u < b.cu; u++) if (b.isGS(u)) logPat[u] = u == 0 ? 0 : (rng.chance(0.6) ? 1 : -1); else if (u % b.sub == 0 && rng.chance(0.25)) logPat[u] = rng.irange(2);
    std::vector<Ev> A, R, A2;
    double pos = 0;
    // the refrain: the people's own (the tradition's seed), so every chant of theirs answers alike
    Rng rr((uint64_t)T.refrain);
    auto refrain = [&](std::vector<Ev>& out) { Rng keep = b.rng; b.rng = rr; b.phrase(T.rest, 0, 0, 0.9, 0.04, b.cpp, out); b.rng = keep; };
    switch (form) {
        case PF_CHANT: case PF_WORK: case PF_HEALING: case PF_WARNING: case PF_HARVEST: case PF_FESTIVAL: {
            const bool healing = form == PF_HEALING, war = form == PF_WARNING, work = form == PF_WORK;
            const double loud = war ? 1.0 : (healing ? 0.6 : 0.82);
            const int intro = rng.chance(0.6) ? 1 : 2, pairs = b.bySeconds(rng.range(45, 70), 2.0 * b.cpp * cyc, 3, 10);
            for (int c = 0; c < intro * b.cpp; c++) { stoneDrums(b, pos, loud * 0.9, false, logPat); pos += cyc; }
            refrain(R);
            if (healing && roar >= 0) b.add(0, 1e3, roar, 0, 0.7);   // trimmed below to the piece's end
            for (int p = 0; p < pairs; p++) {
                if (p % 2 == 0) b.phrase(0, T.rest, 0, war ? 1.4 : 1.05, 0.06, b.cpp, A); else b.vary(A, T.rest, A2);
                const std::vector<Ev>& call = p % 2 == 0 ? A : A2;
                b.place(call, pos, caller, war ? 0.8 : 0.6, war ? 1.0 : 0.85, 0, true, war ? 0.6 : 1.0);
                if (rng.chance(0.3) && flute >= 0 && !war) b.place(call, pos, flute, 0.35, 0.5, n, false);   // the flute shadows the caller an octave up
                for (int c = 0; c < b.cpp; c++) stoneDrums(b, pos + c * cyc, loud * 0.85, false, logPat);
                pos += b.cpp * cyc;
                b.place(R, pos, chorus, war ? 0.85 : 0.65, war ? 1.0 : 0.85, 0, true, war ? 0.6 : 1.0);
                if (T.octaves) b.place(R, pos, chorus, 0.4, 0.55, -n, true, war ? 0.6 : 1.0);   // the low voices an octave under the answer
                if (healing && roar < 0) b.add(pos, b.cpp * cyc, chorus, -n, 0.35, -1000, 4);   // the chorus's hum under its answer
                for (int c = 0; c < b.cpp; c++) stoneDrums(b, pos + c * cyc, loud, c + 1 == b.cpp && p % 2 == 1, logPat);
                if (work) for (int c = 0; c < b.cpp; c++) for (int gsu : b.gs) b.hit(pos + c * cyc + (double)gsu / b.sub, E.kitOf(KR_TICK) >= 0 ? E.kitOf(KR_TICK) : E.kitOf(KR_LOW), 0.9);   // the stroke
                pos += b.cpp * cyc;
            }
            b.add(pos, cyc, chorus, 0, 0.8, -1000, b.vowel());   // the chorus holds the tonic
            b.hit(pos, E.kitOf(KR_LOW), 1.0);
            pos += cyc;
            if (form == PF_FESTIVAL || form == PF_HARVEST) warp(b.P, pos, [&](double x) { return 1 + 0.18 * x / std::max(1.0, pos); });
            break;
        }
        case PF_RINGDANCE: case PF_DANCE: {   // a one-cycle cell over the drums, round and round, faster and faster
            const int intro = 2;
            for (int c = 0; c < intro; c++) { stoneDrums(b, pos, 0.85, false, logPat); pos += cyc; }
            const int lead = rng.chance(0.55) && flute >= 0 ? flute : chorus;
            b.phrase(0, T.rest, 0, 1.25, 0.05, 1, A);
            std::vector<Ev> B; b.phrase(T.rest, 0, 1, 1.3, 0.05, 1, B);
            const int rounds = b.bySeconds(rng.range(38, 60), cyc, 8, 28);
            for (int k = 0; k < rounds; k++) {
                const std::vector<Ev>* cell = &A;
                if (k % 4 == 2) cell = &B;
                if (k % 4 == 3) { b.vary(A, 0, A2); cell = &A2; }
                b.place(*cell, pos, lead, 0.62, 0.9, lead == flute ? n : 0, lead == chorus, 0.9);
                if (k >= rounds / 2 && bow >= 0) b.place(*cell, pos, bow, 0.4, 0.6, -n, false);
                else if (k >= rounds / 2 && lead == flute) b.place(*cell, pos, chorus, 0.4, 0.6, 0, true);
                stoneDrums(b, pos, 0.75 + 0.25 * k / rounds, k % 4 == 3, logPat);
                pos += cyc;
            }
            b.hit(pos, E.kitOf(KR_LOW), 1.0); b.add(pos, 0.5, chorus, n, 0.9, -1000, 0);   // the last stamp and the shout
            pos += 1;
            const double total = pos, accel = rng.range(0.2, 0.38);
            warp(b.P, total, [&](double x) { return 1 + accel * std::min(1.0, x / total); });
            break;
        }
        case PF_HUNT: {   // the flute (or the bow) alone, calls high and long, answers low; sticks or the log drum ticking softly
            const int lead = flute >= 0 ? flute : caller;
            const int phrases = b.bySeconds(rng.range(35, 60), (double)b.cpp * cyc, 3, 9);
            for (int p = 0; p < phrases; p++) {
                const bool high = p % 2 == 0;
                b.phrase(high ? n : 0, high ? n + T.rest : 0, high ? n : 0, 0.7, 0.12, b.cpp, A);
                b.place(A, pos, lead, 0.55, 0.85, 0, lead == caller, 1.0);
                if (!high && bow >= 0 && rng.chance(0.5)) b.add(pos, b.cpp * cyc, bow, -n, 0.4);
                for (int c = 0; c < b.cpp; c++) for (int u = 0; u < b.cu; u += b.sub) {
                    const int tk = E.kitOf(KR_TUNED) >= 0 ? E.kitOf(KR_TUNED) : E.kitOf(KR_TICK);
                    if (b.isGS(u) || rng.chance(0.3)) b.hit(pos + c * cyc + (double)u / b.sub, tk >= 0 ? tk : E.kitOf(KR_LOW), 0.45);
                }
                pos += b.cpp * cyc;
            }
            b.add(pos, cyc, lead, 0, 0.6);
            pos += cyc;
            break;
        }
        case PF_KEENING: case PF_LAMENT: case PF_LAST: {   // voices sliding down, the chorus humming the tonic, a slow heartbeat on the low skin or none
            const int phrases = b.bySeconds(rng.range(40, 65), (double)b.cpp * cyc, 3, 8);
            const bool beat = rng.chance(0.5) && form != PF_LAST;
            for (int p = 0; p < phrases; p++) {
                b.phrase(p == 0 ? n : n - 1 - rng.irange(2), p % 2 ? 0 : T.rest, n / 2, 0.55, 0.12, b.cpp, A);
                const int v = p % 2 == 0 ? caller : chorus;
                int vw = b.vowel();
                for (const Ev& e : A) { if (e.rest) continue; b.add(pos + (double)e.unit / b.sub, (double)e.len / b.sub, v, e.deg, rng.range(0.65, 0.9), e.deg + 1 + rng.irange(2), vw); }
                b.add(pos, b.cpp * cyc, chorus, -n, 0.5, -1000, 4);
                if (beat) for (int c = 0; c < b.cpp; c++) b.hit(pos + c * cyc, E.kitOf(KR_LOW), 0.5);
                pos += b.cpp * cyc;
            }
            b.add(pos, 2 * cyc, chorus, 0, 0.5, 1, 4);
            pos += 2 * cyc;
            break;
        }
        default: {   // a lullaby: the caller soft and high, rocking between two notes, a rattle shaken gently
            const int phrases = b.bySeconds(rng.range(35, 55), (double)b.cpp * cyc, 3, 10);
            const int other = rng.chance(0.5) ? 1 : T.rest;
            for (int p = 0; p < phrases; p++) {
                int vw = b.vowel(); int unit = 0;
                for (int c = 0; c < b.cpp; c++) for (size_t gi = 0; gi < b.T.groups.size(); gi++) {
                    const int len = b.T.groups[gi] * b.sub;
                    const bool last = c == b.cpp - 1 && gi + 1 == b.T.groups.size();
                    const int deg = last ? (p % 2 ? 0 : other) : ((gi + p) % 2 ? other : 0) + (rng.chance(0.2) ? 2 : 0);
                    const int l1 = len > b.sub && !last ? len - b.sub : len;
                    b.add(pos + (double)unit / b.sub, (double)l1 / b.sub, caller, n + deg, rng.range(0.38, 0.55), -1000, vw);
                    if (l1 < len) b.add(pos + (double)(unit + l1) / b.sub, (double)(len - l1) / b.sub, caller, n + deg + 1, 0.4, -1000, vw);
                    if (E.kitOf(KR_SHAKE) >= 0) b.hit(pos + (double)unit / b.sub, E.kitOf(KR_SHAKE), 0.25);
                    unit += len;
                }
                pos += b.cpp * cyc;
            }
            b.add(pos, cyc, caller, n, 0.4, -1000, 4);
            pos += cyc;
            const double total = pos;
            warp(b.P, total, [&](double x) { return x < total * 0.75 ? 1.0 : 1 - 0.35 * (x - total * 0.75) / (total * 0.25); });
            break;
        }
    }
    if (form != PF_RINGDANCE && form != PF_DANCE && form != PF_HUNT && form != PF_KEENING && form != PF_LAMENT && form != PF_LAST) b.ornament(caller, 0.8);
    else b.ornament(caller, 0.5);
}

// ---- the medieval: C-04's statement, variation, answer, return and close, its drone and its tabor; a choir in organum for a hymn ----

void buildMedieval(Bld& b, int form) {
    const Tradition& T = b.T; const Ensemble& E = b.E; Rng& rng = b.rng; Piece& P = b.P;
    const int n = b.n, sub = b.sub, beats = b.beats, cycleUnits = b.cu, cpp = b.cpp;
    const FormP F = form < 11 ? C04_FP[form] : (form == PF_PROCESSION ? FormP{0.75, 0.7, 0.55, 0.8, 0.06, -1, 2, false} : FormP{0.9, 0.95, 0.5, 0.78, 0.08, 0, 1, true});   // a procession; a ballad
    const int reg = F.reg * (n / 2);
    const int nv = (int)T.voices.size();
    const int choir = E.voiceOf(MR_CHORUS), solo = E.voiceOf(MR_SINGER), drone = E.voiceOf(MR_DRONE), bell = E.voiceOf(MR_BELL) >= 0 ? E.voiceOf(MR_BELL) : -1;
    int bellV = bell; for (int i = 0; i < nv && bellV < 0; i++) if (T.voices[i].family == TF_METAL) bellV = i;
    // who sings: a hymn and a procession the choir, a love song, a ballad and a lullaby a voice now and then
    int leadV = 0;
    if ((form == PF_HYMN || form == PF_PROCESSION) && choir >= 0 && rng.chance(0.75)) leadV = choir;
    else if ((form == PF_LOVE || form == PF_BALLAD || form == PF_LULLABY || form == PF_LAMENT) && solo >= 0 && rng.chance(0.55)) leadV = solo;
    const bool sung = leadV == choir || leadV == solo;
    std::vector<Ev> A, A2, B, A3;
    b.phrase(0, T.rest, 0, F.density, F.restP, cpp, A);
    b.vary(A, 0, A2);
    b.phrase(rng.chance(0.5) ? n : T.rest, T.rest, 2, F.density * 1.1, F.restP, cpp, B);
    A3 = A; A3.back().deg = 0;
    const int answerVoice = sung ? (nv >= 1 ? 0 : leadV) : (T.second == 0 && nv >= 2 ? 1 : 0), variationVoice = sung ? leadV : (drone < 0 && nv >= 3 ? 2 : 0);
    struct Ph { const std::vector<Ev>* ev; int voice; };
    std::vector<Ph> order = {{&A, leadV}, {&A2, variationVoice}, {&B, answerVoice}, {&A3, leadV}};
    if (F.repeat) { order.push_back({&B, answerVoice}); order.push_back({&A3, leadV}); }
    const int phraseBeats = cpp * beats;
    // a short form comes round again (C-04's pieces ran 28-70 s): its statement varied anew, its answer moved, the return closing
    std::vector<Ev> A4, B2;
    {
        const double est = (order.size() * phraseBeats + beats) * 60.0 / std::max(40.0, P.bpm);
        if (est < 34) {
            b.vary(A, T.rest, A4); b.vary(B, T.rest, B2);
            const size_t first = order.size();
            order.push_back({&A4, leadV}); order.push_back({&B2, answerVoice}); order.push_back({&A3, leadV});
            if (est < 20) for (size_t k = 0; k < first; k++) order.push_back(order[k]);
        }
    }
    double pos = 0;
    int vw = b.vowel();
    for (const Ph& ph : order) {
        for (const Ev& e : *ph.ev) {
            if (e.rest) continue;
            const bool s = ph.voice == choir || ph.voice == solo;
            if (s && rng.chance(0.4)) vw = b.vowel();
            b.add(pos + (double)e.unit / sub, (double)e.len / sub, ph.voice, e.deg + reg, rng.range(F.vel0, F.vel1) * b.accent(e.unit), -1000, s ? vw : -1);
        }
        if (bellV >= 0 && (form == PF_HYMN || form == PF_PROCESSION || form == PF_LAMENT)) b.add(pos + phraseBeats - (double)T.groups.back(), (double)T.groups.back(), bellV, reg + (rng.chance(0.5) ? 0 : T.rest), 0.55);   // a bell at the phrase's end
        pos += phraseBeats;
    }
    b.add(pos, beats, leadV, reg, F.vel1 * 0.9, -1000, sung ? b.vowel() : -1);   // the close: the tonic held a cycle
    pos += beats;
    P.beats = pos;
    b.ornament(leadV, sung ? 0.6 : 1.0);
    // the second voice: organum for a sung hymn (the choir again, the fifth's inversion below), else C-04's
    if (sung && leadV == choir && (form == PF_HYMN || form == PF_PROCESSION)) {
        std::vector<Note> lead = P.notes;
        for (const Note& l : lead) if (l.voice == choir) { Note v = l; v.degree = l.degree - (n - T.rest); v.vel *= 0.7; v.slideFrom = -1000; P.notes.push_back(v); }
    } else if (nv >= 2 && T.second != 0) {
        std::vector<Note> lead = P.notes;
        for (const Note& l : lead) {
            if (l.voice != leadV) continue;
            Note v = l; v.voice = sung ? 0 : 1; v.slideFrom = -1000; v.vowel = -1;
            if (T.second == 1) { v.degree = l.degree - (n - T.rest); v.vel *= 0.6; }
            else if (T.second == 2) { v.t = l.t + phraseBeats; if (v.t + v.dur > pos) continue; v.vel *= 0.55; }
            else { v.degree = l.degree - n; v.vel *= 0.5; v.t += rng.range(0.0, 0.08); if (v.t + v.dur > pos) v.dur = pos - v.t; }
            if (v.dur > 0.02) P.notes.push_back(v);
        }
    }
    // the drone: the tonic a period below a cycle at a time, on the drone if the people had one, else C-04's last timbre
    if (T.drone && (drone >= 0 || nv >= 2)) {
        const int dv = drone >= 0 ? drone : nv - 1;
        for (double t = 0; t < pos - 1e-6; t += beats) b.add(t, beats, dv, reg - n, 0.35);
    }
    // the tabor: C-04's pattern drawn once from the cycle's groups, the phrases' last groups filled, one low hit on the close; jingles in a dance
    if (F.drum == 2 || (F.drum == 1 && T.hasDrum)) {
        const int low = E.kitOf(KR_LOW), high = E.kitOf(KR_HIGH), jing = E.kitOf(KR_SHAKE);
        struct Hit { int unit, kind; double vel; };
        std::vector<Hit> pat;
        double pOff = F.density >= 1.4 ? 0.7 : 0.45, pSub = F.density >= 1.4 ? 0.35 : 0.15;
        for (int u = 0; u < cycleUnits; u++) {
            if (b.isGS(u)) pat.push_back({u, 0, u == 0 ? 1.0 : 0.85});
            else if (u % sub == 0) { if (rng.chance(pOff)) pat.push_back({u, 1, 0.6}); }
            else if (rng.chance(pSub)) pat.push_back({u, 1, 0.4});
        }
        const int cycles = (int)std::lround(pos / beats), lastCycle = cycles - 1, fillFrom = b.gs.back();
        for (int c = 0; c < lastCycle; c++) {
            bool fill = (c + 1) % cpp == 0;
            for (const Hit& h : pat) {
                if (fill && h.unit >= fillFrom) continue;
                b.hit(c * beats + (double)h.unit / sub, h.kind ? high : low, h.vel);
            }
            if (fill) for (int u = fillFrom; u < cycleUnits; u++) b.hit(c * beats + (double)u / sub, u == fillFrom ? low : high, 0.5 + 0.3 * (u - fillFrom) / std::max(1, cycleUnits - fillFrom));
            if (jing >= 0 && (form == PF_DANCE || form == PF_FESTIVAL || form == PF_HARVEST)) for (int u = 0; u < cycleUnits; u += sub) if (!b.isGS(u)) b.hit(c * beats + (double)u / sub, jing, 0.5);
        }
        b.hit(lastCycle * beats, low, 1.0);
    }
}

// ---- the industrial: a melody over chords of the people's scale, a bass, a band's drums; an organ, a music box, a parlour's keys ----

void buildIndustrial(Bld& b, int form) {
    const Tradition& T = b.T; const Ensemble& E = b.E; Rng& rng = b.rng; Piece& P = b.P;
    const int n = b.n, sub = b.sub, cyc = b.beats;
    int lead = E.voiceOf(MR_LEAD), second = E.voiceOf(MR_SECOND), chords = E.voiceOf(MR_CHORDS), bass = E.voiceOf(MR_BASS);
    const int organV = E.voiceOf(MR_ORGAN), box = E.voiceOf(MR_BOX), singerV = E.voiceOf(MR_SINGER), keysV = E.voiceOf(MR_KEYS) >= 0 ? E.voiceOf(MR_KEYS) : (E.band == 1 ? E.voiceOf(MR_CHORDS) : -1);
    const int bd = E.kitOf(KR_LOW), sn = E.kitOf(KR_BACK), cy = E.kitOf(KR_CRASH);
    bool march = form == PF_MARCH || form == PF_FUNERAL || form == PF_WARNING || form == PF_OVERTURE || form == PF_FESTIVAL || form == PF_WORK;
    const bool turning = form == PF_TURNING || form == PF_DANCE || form == PF_HARVEST || form == PF_LOVE;
    const bool anthem = form == PF_ANTHEM, hymn = form == PF_HYMN || form == PF_LAMENT || form == PF_LAST, parlour = form == PF_PARLOUR || form == PF_LULLABY || form == PF_BALLAD;
    const bool musicbox = form == PF_MUSICBOX;
    if (hymn && organV >= 0) { lead = organV; second = organV; chords = organV; bass = organV; }
    if (parlour) { if (singerV >= 0 && rng.chance(0.6)) lead = singerV; if (keysV >= 0) { chords = keysV; bass = keysV; } }
    if (musicbox && box >= 0) { lead = box; second = -1; chords = box; bass = box; }
    const bool sung = lead == singerV;
    // the strains: a phrase of one or two cycles, each with its chords (one a cycle, two in a long cycle); A A B B A, a coda
    // a strain of about eleven seconds (two phrases), the piece five or six of them: a minute or so whatever the cycle and the tempo
    const int strainCycles = b.bySeconds(anthem || form == PF_OVERTURE ? 8.5 : 11.0, cyc, 1, 8), phraseCycles = std::max(1, strainCycles / 2), strainPhrases = strainCycles / phraseCycles;
    const bool twoChords = cyc >= 6;
    Rng fav(mix64(T.seed ^ 0x1D05C0ULL));   // the people's own turns, which their tunes come back to
    std::vector<int> progA = rng.chance(0.6) ? b.progression(phraseCycles * strainPhrases * (twoChords ? 2 : 1), false, fav) : b.progression(phraseCycles * strainPhrases * (twoChords ? 2 : 1), false, rng);
    std::vector<int> progA2 = progA; progA2.back() = 0;
    std::vector<int> progB = b.progression((int)progA.size(), false, rng); progB[0] = b.roots.size() > 1 ? b.roots[1] : T.rest;
    struct Strain { const std::vector<int>* prog; int centre; bool closed; };
    std::vector<Strain> strains = {{&progA, 0, false}, {&progA2, 0, true}, {&progB, 2, false}, {&progA2, 0, true}};
    if (form == PF_OVERTURE || anthem) strains.push_back({&progB, 2, false}), strains.push_back({&progA2, 0, true});
    if (musicbox || parlour) strains.resize(3), strains[2] = {&progA2, 0, true};
    const double tempoVel = form == PF_FUNERAL ? 0.75 : 1.0;
    double pos = 0;
    const int chordsPerCycle = twoChords ? 2 : 1;
    const int half = twoChords ? b.midGroup() : b.cu;
    std::vector<Ev> mel, mel0;
    int vw = b.vowel();
    // an introduction: the last strain's chords, a cycle
    if (!musicbox) {
        int c3[3]; b.chord(T.rest, c3);
        for (int k = 0; k < 3; k++) b.add(0, cyc, chords, c3[k] - n / 2, 0.45 * tempoVel);
        b.add(0, cyc, bass, T.rest - 2 * n + (hymn ? n : 0), 0.6);
        if (march && sn >= 0) for (int u = 0; u < b.cu; u++) b.hit((double)u / sub, sn, 0.35 + 0.4 * u / b.cu);
        pos += cyc;
    }
    for (size_t si = 0; si < strains.size(); si++) {
        const Strain& S = strains[si];
        const int cycles = phraseCycles * strainPhrases;
        // the tune: two phrases over the strain (the first open, the second as the strain ends), its notes on the groups' first beats moved
        // onto the chord sounding; the strains that return vary the first
        {
            const double dens = (musicbox ? 1.25 : (anthem || hymn ? 0.6 : 1.0)) * T.density;
            std::vector<Ev> p1, p2;
            b.phrase(S.centre == 0 ? 0 : n, T.rest, S.centre, dens, 0.06, phraseCycles, p1);
            b.phrase(T.rest, S.closed ? 0 : T.rest, S.centre, dens, 0.06, phraseCycles * (strainPhrases - 1) > 0 ? phraseCycles * (strainPhrases - 1) : phraseCycles, p2);
            mel = p1; const int off = phraseCycles * b.cu; if (strainPhrases > 1) for (Ev e : p2) { e.unit += off; mel.push_back(e); }
        }
        if ((si == 1 || si == 3) && !mel0.empty()) b.vary(mel0, S.closed ? 0 : T.rest, mel);
        if (si == 0) mel0 = mel;
        for (Ev& e : mel) {
            const int cIdx = std::min((int)S.prog->size() - 1, (e.unit / b.cu) * chordsPerCycle + (twoChords && (e.unit % b.cu) >= half ? 1 : 0));
            if (b.isGS(e.unit) && !e.rest) e.deg = b.nearestChordTone(e.deg, (*S.prog)[cIdx]);
        }
        const int reg = musicbox ? n : (hymn ? 0 : (sung ? 0 : n / 2));
        for (const Ev& e : mel) {
            if (e.rest) continue;
            if (sung && rng.chance(0.45)) vw = b.vowel();
            b.add(pos + (double)e.unit / sub, (double)e.len / sub * (march && !anthem ? 0.85 : 1.0), lead, e.deg + reg, rng.range(0.62, 0.9) * b.accent(e.unit) * tempoVel, -1000, sung ? vw : -1);
            if (second >= 0 && second != lead && !musicbox && (b.isGS(e.unit) || e.len >= 2 * sub) && rng.chance(anthem ? 0.9 : 0.6)) {   // the second: a chord tone under the tune
                const int cIdx = std::min((int)S.prog->size() - 1, (e.unit / b.cu) * chordsPerCycle + (twoChords && (e.unit % b.cu) >= half ? 1 : 0));
                int c3[3]; b.chord((*S.prog)[cIdx], c3);
                int under = c3[0]; for (int k = 0; k < 3; k++) { int d = c3[k]; while (d >= e.deg) d -= n; while (d < e.deg - n) d += n; if (d > under || under >= e.deg) under = d; }
                b.add(pos + (double)e.unit / sub, (double)e.len / sub, second, under + reg, 0.5 * tempoVel);
            }
        }
        // the chords and the bass, chord by chord
        for (int c = 0; c < cycles; c++)
            for (int h = 0; h < chordsPerCycle; h++) {
                const int root = (*S.prog)[std::min((int)S.prog->size() - 1, c * chordsPerCycle + h)];
                int c3[3]; b.chord(root, c3);
                const int u0 = h == 0 ? 0 : half, u1 = h + 1 < chordsPerCycle ? half : b.cu;
                const double t0 = pos + c * cyc;
                const int creg = musicbox ? 0 : (hymn ? -n / 2 : -n / 2), breg = musicbox ? -n : -2 * n + (hymn ? n : 0);
                if (march) {   // the bass on the groups' first beats, root and fifth by turns; the chords on the other beats
                    bool alt = false;
                    for (int u = u0; u < u1; u += sub) {
                        if (b.isGS(u)) { b.add(t0 + (double)u / sub, 0.9, bass, (alt ? c3[2] : c3[0]) + breg, 0.75 * tempoVel); alt = !alt; }
                        else for (int k = 0; k < 3; k++) b.add(t0 + (double)u / sub, 0.6, chords, c3[k] + creg, 0.4 * tempoVel);
                    }
                } else if (turning) {   // the bass once a cycle, the chords on the other groups' first beats, held
                    for (int u = u0; u < u1; u++) {
                        if (!b.isGS(u)) continue;
                        if (u == 0) b.add(t0, 1.0, bass, c3[0] + breg, 0.7);
                        else for (int k = 0; k < 3; k++) b.add(t0 + (double)u / sub, 0.9, chords, c3[k] + creg, 0.38);
                    }
                } else if (parlour || musicbox) {   // the bass on the chord's first beat, the chord broken in subdivisions
                    b.add(t0 + (double)u0 / sub, std::max(1.0, (double)(u1 - u0) / sub * 0.5), bass, c3[0] + breg, 0.55);
                    const int pat[4] = {0, 2, 1, 2};
                    for (int u = u0 + (musicbox ? sub : 1); u < u1; u += musicbox ? sub : 1) b.add(t0 + (double)u / sub, musicbox ? 0.5 : (double)1 / sub * 1.6, chords, c3[pat[(u - u0) % 4]] + creg, musicbox ? 0.32 : 0.3);
                } else {   // an anthem, a hymn: the chord held on each group, the bass with it
                    for (int u = u0; u < u1; u++) {
                        if (!b.isGS(u)) continue;
                        int ge = u + 1; while (ge < u1 && !b.isGS(ge)) ge++;
                        const double len = (double)(ge - u) / sub;
                        for (int k = 0; k < 3; k++) b.add(t0 + (double)u / sub, len, chords, c3[k] + creg, (anthem ? 0.5 : 0.42) * tempoVel);
                        b.add(t0 + (double)u / sub, len, bass, c3[0] + breg, (anthem ? 0.7 : 0.55) * tempoVel);
                    }
                }
            }
        // the drums: the bass drum on the groups' first beats, the side drum on the other beats and a roll into the next strain, a crash at its start
        if (march || anthem || (turning && bd >= 0 && rng.chance(0.4))) {
            for (int c = 0; c < cycles; c++) {
                const double t0 = pos + c * cyc;
                for (int u = 0; u < b.cu; u++) {
                    const bool g = b.isGS(u), beat = u % sub == 0;
                    if (g) b.hit(t0 + (double)u / sub, bd, (u == 0 ? 0.95 : 0.8) * tempoVel);
                    else if (march && beat && sn >= 0 && !anthem) b.hit(t0 + (double)u / sub, sn, 0.6 * tempoVel);
                    if (march && sn >= 0 && c == cycles - 1 && u >= b.gs.back()) b.hit(t0 + (double)u / sub, sn, 0.35 + 0.5 * (u - b.gs.back()) / std::max(1, b.cu - b.gs.back()));
                }
                if (c == 0 && cy >= 0) b.hit(t0, cy, anthem ? 0.9 : 0.7);
            }
        }
        pos += cycles * cyc;
    }
    // the close: the tonic chord held a cycle, the bass under it, the bass drum and a crash
    {
        int c3[3]; b.chord(0, c3);
        const int creg = musicbox ? 0 : -n / 2, breg = musicbox ? -n : -2 * n + (hymn ? n : 0);
        b.add(pos, cyc, lead, (musicbox ? n : (hymn ? 0 : n / 2)) + (sung ? -n / 2 : 0), 0.8, -1000, sung ? b.vowel() : -1);
        if (!musicbox) for (int k = 0; k < 3; k++) b.add(pos, cyc, chords, c3[k] + creg, 0.5);
        b.add(pos, cyc, bass, c3[0] + breg, 0.7);
        if (march || anthem) { b.hit(pos, bd, 1.0); b.hit(pos, cy, 0.8); }
        pos += cyc;
    }
    b.ornament(lead, sung ? 0.5 : 0.35);
    const double total = pos;
    if (musicbox) {   // the spring runs down: the last third slows by half, and the box may stop before the tune is done
        const double fall = rng.range(0.4, 0.6);
        warp(P, total, [&](double x) { return x < total * 0.66 ? 1.0 : 1 - fall * (x - total * 0.66) / (total * 0.34); });
        if (rng.chance(0.5)) { std::vector<Note> keep; const double cut = P.notes.empty() ? 0 : total * 0.93; for (const Note& nt : P.notes) if (nt.t < cut) keep.push_back(nt); P.notes.swap(keep); }
    } else if (parlour || hymn || anthem) warp(P, total, [&](double x) { return x < total - cyc * 1.5 ? 1.0 : 1 - 0.3 * (x - (total - cyc * 1.5)) / (cyc * 1.5); });
}

// ---- the modern: a song's sections over a groove of the people's cycle; a dance record that builds and breaks; a ballad, a jingle ----

void groove(Bld& b, double t0, int cycles, double level, int kind, bool fillEnd, bool crashStart) {
    // kind: 0 a song's (the kick on the groups' first beats, the back on the middle group, the hats on the subdivisions); 1 the floor's (the kick
    // on every beat, the open hat between); 2 half (the kick on the cycle's first beat, the back once); 3 the hats alone
    const Ensemble& E = b.E; const int sub = b.sub, cu = b.cu;
    const int kick = E.kitOf(KR_LOW), back = E.kitOf(KR_BACK), hat = E.kitOf(KR_TICK), open = E.kitOf(KR_OPEN), crash = E.kitOf(KR_CRASH);
    const int mid = b.midGroup();
    for (int c = 0; c < cycles; c++) {
        const double tc = t0 + c * b.beats;
        if (crashStart && c == 0) b.hit(tc, crash, 0.75 * level);
        for (int u = 0; u < cu; u++) {
            const double t = tc + (double)u / sub; const bool beat = u % sub == 0, g = b.isGS(u);
            const bool fill = fillEnd && c == cycles - 1 && u >= b.gs.back();
            if (kind == 0) {
                if (g && u != mid) b.hit(t, kick, (u == 0 ? 1.0 : 0.85) * level);
                else if (u + 1 == mid && b.rng.chance(0.3)) b.hit(t, kick, 0.6 * level);
                if (u == mid || (b.gs.size() > 3 && u == b.gs.back())) b.hit(t, back, 0.85 * level);
                if (fill) b.hit(t, back, (0.4 + 0.4 * (u - b.gs.back()) / std::max(1, cu - b.gs.back())) * level);
                else b.hit(t, hat, (beat ? 0.7 : 0.45) * level);
            } else if (kind == 1) {
                if (beat) b.hit(t, kick, (u == 0 ? 1.0 : 0.9) * level);
                if (!beat && sub == 2) b.hit(t, open, 0.55 * level); else if (!beat) b.hit(t, hat, 0.45 * level);
                if (u == mid) b.hit(t, back, 0.8 * level);
            } else if (kind == 2) {
                if (u == 0) b.hit(t, kick, 0.9 * level);
                if (u == mid) b.hit(t, back, 0.7 * level);
                if (beat) b.hit(t, hat, 0.4 * level);
            } else if (beat) b.hit(t, hat, (g ? 0.6 : 0.4) * level);
        }
    }
}

void buildModern(Bld& b, int form) {
    const Tradition& T = b.T; const Ensemble& E = b.E; Rng& rng = b.rng; Piece& P = b.P;
    const int n = b.n, sub = b.sub, cyc = b.beats;
    int lead = E.voiceOf(MR_LEAD); const int second = E.voiceOf(MR_SECOND), chords = E.voiceOf(MR_CHORDS), bass = E.voiceOf(MR_BASS), padV = E.voiceOf(MR_ALT) >= 0 ? E.voiceOf(MR_ALT) : chords, singerV = E.voiceOf(MR_SINGER) >= 0 ? E.voiceOf(MR_SINGER) : (E.voices[lead].family == TF_VOICE ? lead : -1);
    if ((form == PF_BALLAD || form == PF_PROTEST || form == PF_REQUIEM || form == PF_LAMENT || form == PF_LULLABY) && singerV >= 0 && rng.chance(0.75)) lead = singerV;
    const bool sung = E.voices[lead].family == TF_VOICE;
    const bool dance = form == PF_DANCEREC || form == PF_FESTIVAL || form == PF_DANCE, slow = form == PF_BALLAD || form == PF_REQUIEM || form == PF_LAMENT || form == PF_LULLABY || form == PF_LAST;
    const bool jingle = form == PF_JINGLE, theme = form == PF_THEME, anthem = form == PF_ANTHEM;
    // the sections, in cycles: about twelve seconds each (two to eight cycles), so a song runs a minute and a half whatever its cycle and tempo
    const int S = b.bySeconds(12.0, cyc, 1, 8);
    enum { INTRO, VERSE, CHORUS, BRIDGE, OUTRO, BUILD, DROP, BREAK, HOOK };
    struct Sec { int kind, cycles; };
    std::vector<Sec> secs;
    const int H = std::max(1, S / 2), I2 = std::min(2, S);
    if (jingle) { const int J = b.bySeconds(7.0, cyc, 1, 8); secs = {{HOOK, J}, {HOOK, J}, {OUTRO, 1}}; }
    else if (theme) secs = {{INTRO, I2}, {HOOK, S}, {CHORUS, H}, {OUTRO, 1}};
    else if (dance) secs = {{INTRO, H}, {BUILD, S}, {DROP, S}, {BREAK, H}, {BUILD, H}, {DROP, S}, {OUTRO, H}};
    else if (slow) secs = {{INTRO, I2}, {VERSE, S}, {CHORUS, S}, {VERSE, S}, {CHORUS, S}, {OUTRO, I2}};
    else secs = {{INTRO, H}, {VERSE, S}, {CHORUS, S}, {VERSE, S}, {CHORUS, S}, {BRIDGE, H}, {CHORUS, S}, {OUTRO, H}};
    if (!jingle && !theme && !dance && !slow && rng.chance(0.35)) secs.erase(secs.begin() + 5);   // no bridge
    Rng fav(mix64(T.seed ^ 0x1D05C1ULL));
    const int plen = std::min(4, S);
    std::vector<int> progV = rng.chance(0.55) ? b.progression(plen, false, fav) : b.progression(plen, false, rng);
    std::vector<int> progC = b.progression(plen, true, rng); if (progC.size() > 1 && rng.chance(0.5)) progC[0] = b.roots.size() > 1 ? b.roots[1] : T.rest;
    std::vector<int> progB = b.progression(plen, false, rng); progB[0] = b.roots.size() > 2 ? b.roots[2] : T.rest;
    // the hook: the chorus's motif, a cycle long, sung or played twice and varied
    std::vector<Ev> hook, verseA, tmp; b.phrase(n, 0, n, 1.15 * T.density, 0.05, 1, hook);
    const int vc = std::max(1, std::min(2, S / 2));
    b.phrase(0, T.rest, 0, 0.95 * T.density, 0.08, vc, verseA);
    std::vector<Ev> riff; b.phrase(0, 0, 0, 1.6, 0.15, 1, riff);   // the dance record's riff, a cycle of subdivisions
    double pos = 0;
    int vw = b.vowel();
    const double vol = slow ? 0.75 : 1.0;
    for (size_t si = 0; si < secs.size(); si++) {
        const Sec& sc = secs[si];
        const std::vector<int>& prog = sc.kind == CHORUS || sc.kind == HOOK || sc.kind == DROP ? progC : (sc.kind == BRIDGE ? progB : progV);
        const bool full = sc.kind == CHORUS || sc.kind == DROP || sc.kind == HOOK || (sc.kind == BRIDGE && !slow);
        // the drums
        if (sc.kind == INTRO) { if (dance || theme) groove(b, pos, sc.cycles, 0.7, dance ? 3 : 0, true, false); else if (!slow) groove(b, pos, sc.cycles, 0.6, 3, false, false); }
        else if (sc.kind == BREAK || (slow && sc.kind == VERSE && si < 2)) { if (dance) groove(b, pos, sc.cycles, 0.5, 3, false, false); }
        else if (sc.kind == BUILD) groove(b, pos, sc.cycles, 0.8, 0, true, false);
        else if (sc.kind == OUTRO) { if (!slow) groove(b, pos, std::max(1, sc.cycles - 1), 0.6, dance ? 1 : 2, false, false); }
        else groove(b, pos, sc.cycles, vol * (full ? 1.0 : 0.85), dance ? 1 : (slow || (anthem && !full) ? 2 : 0), true, full);   // an anthem's verses at half time
        // the chords and the bass: a chord a cycle, the bass on the cycle's beats with a step into the next root
        for (int c = 0; c < sc.cycles; c++) {
            const int root = prog[c % prog.size()], next = prog[(c + 1) % prog.size()];
            int c3[3]; b.chord(root, c3);
            const double t0 = pos + c * cyc;
            const int chordFam = chords >= 0 ? E.voices[chords].family : TF_PAD;
            if (sc.kind != BREAK || chordFam == TF_PAD) {
                if (chordFam == TF_PAD || (chordFam == TF_ORGAN && E.comp != 1)) for (int k = 0; k < 3; k++) b.add(t0, cyc, chords, c3[k], (full ? 0.6 : 0.48) * vol);
                else if (E.comp == 0 || slow) for (size_t gi = 0; gi < b.gs.size(); gi++) {   // a chord on each group, held to the next
                    const int gsu = b.gs[gi], ge = gi + 1 < b.gs.size() ? b.gs[gi + 1] : b.cu;
                    for (int k = 0; k < 3; k++) b.add(t0 + (double)gsu / sub, (double)(ge - gsu) / sub * 0.9, chords, c3[k], (full ? 0.55 : 0.45) * vol * (gsu == 0 ? 1.0 : 0.85));
                } else for (int u = 0; u < b.cu; u++) {   // 1 on every beat, 2 between the beats, 3 pushed ahead of the groups
                    const bool on = E.comp == 1 ? u % sub == 0 : (E.comp == 2 ? (sub > 1 && u % sub == sub - 1) : (b.isGS((u + 1) % b.cu) || u == 0));
                    if (!on) continue;
                    for (int k = 0; k < 3; k++) b.add(t0 + (double)u / sub, E.comp == 1 ? 0.8 : 0.45, chords, c3[k], (full ? 0.5 : 0.42) * vol * (u == 0 ? 1.0 : 0.85));
                }
            }
            if (padV >= 0 && padV != chords && (full || slow) && sc.kind != INTRO) for (int k = 0; k < 2; k++) b.add(t0, cyc, padV, c3[k] + n, 0.4 * vol);
            if (sc.kind != INTRO && sc.kind != BREAK && bass >= 0) {
                for (int u = 0; u < b.cu; u++) {
                    const double t = t0 + (double)u / sub;
                    if (dance) { if (u % sub == (sub > 1 ? 1 : 0)) b.add(t, 1.0 / sub * 0.9, bass, c3[0] - n, 0.7); continue; }
                    if (u == 0) b.add(t, slow ? cyc * 0.9 : 0.9, bass, c3[0] - n, 0.8 * vol);
                    else if (!slow && (b.isGS(u) || u == b.midGroup())) b.add(t, 0.8, bass, (rng.chance(0.25) ? c3[2] : c3[0]) - n + (rng.chance(0.15) ? n : 0), 0.7 * vol);
                    else if (!slow && u == b.cu - 1 && root != next) b.add(t, 1.0 / sub, bass, next - n + (next > root ? -1 : 1), 0.55 * vol);   // the step into the next root
                }
            }
        }
        // the lead: the verse's line low, the chorus's hook high and twice, the bridge a new line; the riff in the dance record's drops
        if (sc.kind == VERSE) {
            for (int c = 0; c + vc <= sc.cycles; c += vc) {
                if (c > 0 && rng.chance(0.6)) { b.vary(verseA, c + 2 >= sc.cycles ? T.rest : 0, tmp); b.place(tmp, pos + c * cyc, lead, 0.55, 0.8, 0, sung); }
                else b.place(verseA, pos + c * cyc, lead, 0.55, 0.8, 0, sung);
            }
        } else if (sc.kind == CHORUS || sc.kind == HOOK) {
            for (int c = 0; c < sc.cycles; c++) {
                if (c % 2 == 1) { b.vary(hook, c + 1 == sc.cycles ? 0 : T.rest, tmp); b.place(tmp, pos + c * cyc, lead, 0.65, 0.95, 0, sung); }
                else b.place(hook, pos + c * cyc, lead, 0.65, 0.95, 0, sung);
                if (second >= 0 && second != lead && c % 2 == 0) b.place(hook, pos + c * cyc, second, 0.3, 0.45, -n, false, 0.7);
            }
        } else if (sc.kind == BRIDGE) {
            b.phrase(T.rest, T.rest, n / 2, 0.8, 0.1, sc.cycles, tmp); b.place(tmp, pos, lead, 0.55, 0.8, 0, sung);
        } else if (sc.kind == DROP || sc.kind == BUILD || sc.kind == BREAK) {
            const int rv = second >= 0 ? second : lead;
            for (int c = 0; c < sc.cycles; c++) b.place(riff, pos + c * cyc, rv, sc.kind == BUILD ? 0.45 + 0.4 * c / sc.cycles : 0.7, sc.kind == BUILD ? 0.55 + 0.4 * c / sc.cycles : 0.9, 0, false, 0.8);
            if (sc.kind == DROP && sung) for (int c = 0; c < sc.cycles; c += 2) b.place(hook, pos + c * cyc, lead, 0.6, 0.85, 0, true);
        } else if (sc.kind == INTRO && (theme || jingle)) {
            b.phrase(n, n + T.rest, n, 1.0, 0.0, 1, tmp); b.place(tmp, pos, second >= 0 ? second : lead, 0.7, 0.95, 0, false);
        }
        if (sung) for (Note& nt : P.notes) if (nt.voice == lead && nt.vowel < 0) { if (rng.chance(0.4)) vw = b.vowel(); nt.vowel = vw; }
        pos += sc.cycles * cyc;
    }
    // the end: the tonic chord rung out, a crash (a jingle's sting)
    { int c3[3]; b.chord(0, c3); for (int k = 0; k < 3; k++) b.add(pos, cyc, chords, c3[k], 0.55); if (bass >= 0) b.add(pos, cyc, bass, -n, 0.75); if (!slow) { b.hit(pos, E.kitOf(KR_LOW), 0.9); b.hit(pos, E.kitOf(KR_CRASH), 0.8); } pos += cyc; }
    if (sung) b.ornament(lead, 0.3);
    if (slow) { const double total = pos; warp(P, total, [&](double x) { return x < total - 2 * cyc ? 1.0 : 1 - 0.3 * (x - (total - 2 * cyc)) / (2 * cyc); }); }
}

// ---- the futuristic: a cell phasing against itself, swarms gliding between chords, a synthetic voice over an arpeggiator ----

double microBend(const Bld& b, int deg, Rng& rng) {   // a microtone: half the way to the next note of the scale, up or down
    const double up = traditionCents(b.T, deg + 1) - traditionCents(b.T, deg), dn = traditionCents(b.T, deg) - traditionCents(b.T, deg - 1);
    return rng.chance(0.5) ? up * 0.5 : -dn * 0.5;
}

void buildFuturistic(Bld& b, int form) {
    const Tradition& T = b.T; const Ensemble& E = b.E; Rng& rng = b.rng;
    const int n = b.n, sub = b.sub, cyc = b.beats;
    const int voiceV = E.voiceOf(MR_LEAD), glass = E.voiceOf(MR_SECOND), swarmV = E.voiceOf(MR_SWARM), padV = E.voiceOf(MR_CHORDS), sub0 = E.voiceOf(MR_BASS), alt = E.voiceOf(MR_ALT);
    const int tick = E.kitOf(KR_TICK), tick2 = E.kitOf(KR_HIGH), pulse = E.kitOf(KR_SUB);
    const bool sungSwarm = swarmV >= 0 && E.voices[swarmV].family == TF_SYNVOICE;   // a choir of synthetic voices in the swarm's place
    double pos = 0;
    if (form == PF_PROCESS || form == PF_SIGNAL || form == PF_WORK || form == PF_DANCE || form == PF_FESTIVAL || form == PF_HARVEST || form == PF_MARCH) {
        // the cell: m notes over a cycle's subdivisions; the first voice loops it, the second loops it a subdivision longer (they drift apart and
        // meet again), the voice enters halfway with the cell's notes held long; or the cell is built note by note and taken apart again
        const bool signal = form == PF_SIGNAL || form == PF_WARNING || form == PF_MARCH;
        const int units = b.cu, m = signal ? 2 : 4 + rng.irange(4);
        std::vector<int> at, deg;
        if (signal) { at = {0, units / 2}; const int iv = 1 + rng.irange(2); deg = {n + iv, n}; }
        else { std::vector<int> slots(units); for (int i = 0; i < units; i++) slots[i] = i; for (int i = units - 1; i > 0; i--) std::swap(slots[i], slots[rng.irange(i + 1)]); at.assign(slots.begin(), slots.begin() + std::min(m, units)); std::sort(at.begin(), at.end()); for (size_t i = 0; i < at.size(); i++) deg.push_back(rng.irange(n + 2) + (i == 0 ? 0 : 0)); deg[0] = 0; }
        const bool additive = !signal && rng.chance(0.4);
        double posB = 0; const double cellA = (double)units / sub, cellB = (double)(units + 1) / sub;
        const int loops = std::max(8, (int)std::lround(rng.range(signal ? 45 : 55, signal ? 65 : 85) * b.P.bpm / 60.0 / cellA));
        for (int k = 0; k < loops; k++) {
            const int shown = additive ? std::min((int)at.size(), k < loops / 2 ? k + 1 : loops - k) : (int)at.size();
            for (int i = 0; i < shown; i++) b.add(pos + (double)at[i] / sub, signal ? (double)units / 2 / sub * 0.8 : 0.4, glass, deg[i] + n / 2, rng.range(0.55, 0.8));
            if (!signal && k >= 2 && alt >= 0) { for (int i = 0; i < (int)at.size(); i++) b.add(posB + (double)at[i] / sub, 0.4, alt, deg[i] + n / 2 + (k % 4 == 3 ? n : 0), 0.5); posB += cellB; }
            else posB = pos + cellA;
            if (k >= loops / 2 && k % 2 == 0 && voiceV >= 0) b.add(pos, cellA * 2, voiceV, deg[(k / 2) % deg.size()] + (signal ? 0 : -n / 2 + n / 2), 0.55, -1000, b.vowel());
            for (int u = 0; u < units; u++) { if (u % sub == 0 && tick >= 0 && rng.chance(signal ? 1.0 : 0.5)) b.hit(pos + (double)u / sub, tick, 0.5); else if (signal && tick2 >= 0) b.hit(pos + (double)u / sub, tick2, 0.35); }
            if (k % 2 == 0) b.hit(pos, pulse, 0.8);
            if (k % 4 == 0 && swarmV >= 0 && !signal) { int c3[3]; b.chord(0, c3); for (int q = 0; q < 2; q++) b.add(pos, cellA * 4, swarmV, c3[q] - n / 2, 0.35, -1000, sungSwarm ? b.vowel() : -1); }
            pos += cellA;
        }
        pos = std::max(pos, posB);
        b.add(pos, 2.0 * cyc / 2, glass, 0, 0.6); b.hit(pos, pulse, 0.9);
        pos += cyc;
    } else if (form == PF_SYNSONG || form == PF_LOVE || form == PF_HYMN || form == PF_BALLAD) {
        // the voice's phrases over an arpeggiator (the chord's notes up and down on every subdivision), the sub tone on the cycle, clicks on the beats
        Rng fav(mix64(T.seed ^ 0x1D05C2ULL));
        std::vector<int> prog = rng.chance(0.6) ? b.progression(4, true, fav) : b.progression(4, true, rng);
        const int phrases = b.bySeconds(rng.range(45, 70), (double)b.cpp * cyc, 4, 14);
        std::vector<Ev> A, A2;
        b.phrase(0, T.rest, n / 2, 0.85, 0.08, b.cpp, A);
        for (int p = 0; p < phrases; p++) {
            if (p % 2 == 1) b.vary(A, p + 1 == phrases ? 0 : T.rest, A2); else if (p > 0 && rng.chance(0.4)) b.phrase(n / 2, T.rest, n / 2, 0.85, 0.08, b.cpp, A);
            b.place(p % 2 ? A2 : A, pos, voiceV, 0.6, 0.85, 0, true);
            for (int c = 0; c < b.cpp; c++) {
                const int root = prog[(p * b.cpp + c) % prog.size()]; int c3[3]; b.chord(root, c3);
                const double t0 = pos + c * cyc;
                const int arp[6] = {0, 1, 2, 1, 2, 0};
                for (int u = 0; u < b.cu; u++) b.add(t0 + (double)u / sub, 1.0 / sub * 0.9, glass, c3[arp[u % 6] % 3] + n / 2 + (u % 6 >= 3 ? n : 0), 0.32 + 0.12 * (u == 0));
                b.add(t0, cyc, sub0, root - n, 0.7);
                for (int u = 0; u < b.cu; u += sub) b.hit(t0 + (double)u / sub, tick, b.isGS(u) ? 0.55 : 0.35);
                if (padV >= 0) for (int k = 0; k < 3; k++) b.add(t0, cyc, padV, c3[k], 0.3);
            }
            pos += b.cpp * cyc;
        }
        b.add(pos, cyc, voiceV, 0, 0.6, -1000, b.vowel()); b.add(pos, cyc, sub0, -n, 0.6);
        pos += cyc;
    } else {
        // a swarm, a drift, an elegy: chords of three to five tones, some between the people's notes, each held a few cycles and gliding into the next
        const bool elegy = form == PF_ELEGY || form == PF_LAMENT || form == PF_LAST, drift = form == PF_DRIFT || form == PF_LULLABY;
        const int chordsN = 6 + rng.irange(3);
        std::vector<int> prev;
        for (int c = 0; c < chordsN; c++) {
            const int root = c == 0 || c == chordsN - 1 ? 0 : b.roots[1 + rng.irange(std::max(1, std::min(3, (int)b.roots.size() - 1)))];
            int c3[3]; b.chord(root, c3);
            std::vector<int> tones = {c3[0] - n, c3[1], c3[2]};
            if (rng.chance(0.6)) tones.push_back(c3[0] + n);
            if (rng.chance(0.4)) tones.push_back(c3[1] + n);
            const double len = rng.range(5.5, 11.0) * b.P.bpm / 60.0;   // five to eleven seconds a chord
            for (size_t k = 0; k < tones.size(); k++) {
                const double bend = !drift && rng.chance(0.35) ? microBend(b, tones[k], rng) : 0;
                const int from = k < prev.size() && c > 0 ? prev[k] : -1000;
                const int v = drift && padV >= 0 ? padV : swarmV;
                b.add(pos, len + cyc * 0.5, v, tones[k], rng.range(0.4, 0.6), from, v == swarmV && sungSwarm ? b.vowel() : -1, bend);
            }
            if (drift && glass >= 0) for (int u = 0; u < (int)(len * sub); u += sub * (1 + rng.irange(3))) b.add(pos + (double)u / sub, 0.5, glass, tones[rng.irange((int)tones.size())] + n, 0.35);
            if (elegy && voiceV >= 0 && c % 2 == 1) b.add(pos + cyc * 0.5, len - cyc * 0.5, voiceV, tones[std::min((size_t)2, tones.size() - 1)], 0.5, -1000, b.vowel());
            if (rng.chance(0.5)) b.hit(pos, pulse, 0.6);
            prev = tones;
            pos += len;
        }
        pos += cyc;
    }
    b.P.beats = pos;
}

// ---- the far future: tones of the scale across several periods that come and go, a deep tone under them, a rare glint ----

void buildFarFuture(Bld& b, int form) {
    const Tradition& T = b.T; const Ensemble& E = b.E; Rng& rng = b.rng; Piece& P = b.P;
    const int n = b.n;
    P.bpm = 60;   // a beat a second: the field's times are seconds
    const int a = E.voiceOf(MR_LEAD), a2 = E.voiceOf(MR_SECOND), deep = E.voiceOf(MR_BASS), glint = E.voiceOf(MR_BELL);
    const double total = rng.range(70, 115);
    std::vector<int> tones;   // the field's degrees: across three periods, the tonic and the resting degree first
    tones.push_back(0); tones.push_back(T.rest);
    for (int k = 0; k < 4 + rng.irange(4); k++) tones.push_back(rng.irange(n) + (rng.irange(3) - 1) * n);
    std::sort(tones.begin(), tones.end()); tones.erase(std::unique(tones.begin(), tones.end()), tones.end());
    // the deep tone under all of it, swelling twice or three times
    for (double t = 0; t < total - 5;) { const double len = std::min(total - t, rng.range(25, 45)); b.add(t, len, deep, -2 * n, rng.range(0.55, 0.75)); t += len - rng.range(2, 6); }
    if (form == PF_TURNFIELD) {   // the tones enter in turn, low to high, at the cycle's groups stretched to seconds: a slow wheel, twice or three times round
        const double turn = rng.range(2.5, 4.5);
        int k = 0;
        for (double t = 2; t < total - 10; t += turn) {
            const int deg = tones[k % tones.size()]; k++;
            b.add(t, turn * rng.range(2.5, 4.0), k % 2 ? a : a2, deg, rng.range(0.45, 0.7));
            if (k % (int)tones.size() == 0 && rng.chance(0.5)) std::reverse(tones.begin(), tones.end());
        }
    } else if (form == PF_STANDING || form == PF_ELEGY || form == PF_LAMENT || form == PF_LAST) {   // one chord held, a tone of it moved every twenty seconds or so
        std::vector<int> held(tones.begin(), tones.begin() + std::min((size_t)4, tones.size()));
        std::vector<double> since(held.size(), 1.0);
        for (size_t k = 0; k < held.size(); k++) since[k] = 1 + k * 1.5;
        double t = 18;
        while (t < total - 8) {
            const size_t k = rng.irange((int)held.size());
            b.add(since[k], t - since[k] + 3, k % 2 ? a : a2, held[k], 0.6);
            held[k] = tones[rng.irange((int)tones.size())] + (rng.chance(0.3) ? n : 0); since[k] = t;
            t += rng.range(14, 24);
        }
        for (size_t k = 0; k < held.size(); k++) b.add(since[k], total - since[k], k % 2 ? a : a2, held[k], 0.6);
    } else {   // a field: each tone comes and goes on its own, overlapping; a field of bells holds fewer tones and many glints
        const bool bells = form == PF_BELLFIELD;
        for (size_t k = 0; k < tones.size(); k++) {
            if (bells && k % 2) continue;
            for (double t = rng.range(0, 12); t < total - 6;) {
                const double len = std::min(total - t, rng.range(8, 20));
                b.add(t, len, k % 2 ? a : a2, tones[k], rng.range(0.4, 0.65));
                t += len + rng.range(2, 9);
            }
        }
    }
    // the glints: a high bell now and then (often in a field of bells); a far choir's long swells instead where the people heard one
    const bool choir = glint >= 0 && E.voices[glint].family == TF_SWARM;
    for (double t = rng.range(3, 8); t < total - 4; t += choir ? rng.range(10, 22) : (form == PF_BELLFIELD ? rng.range(1.5, 4.5) : rng.range(6, 15))) {
        const int d = tones[rng.irange((int)tones.size())];
        if (choir) b.add(t, std::min(total - 1 - t, rng.range(6, 12)), glint, ((d % n) + n) % n + n, rng.range(0.45, 0.65));
        else b.add(t, 0.5, glint, ((d % n) + n) % n + 2 * n - (rng.chance(0.4) ? n : 0), rng.range(0.3, 0.6));
    }
    b.P.beats = total + 6;   // the last tones' long fade inside the piece
}

// the form by the shard's tone and the record's age. D-05: a drifter's people's by the cold (`dark`: 1 dimming, 2 going, 3 gone): the dawn songs to
// call the sun up while it went, the last sunrise, and in the dark the last songs and the lullabies; every other people's draw among the forms
// it always drew (`PF_DRAWN_COUNT`)
int formOf(int tone, int age, Rng& rng, int dark = 0) {
    double w[PF_COUNT] = {0};
    switch (age) {
        case AGE_PREHISTORIC:
            switch (tone) {
                case ST_ELEGY: w[PF_KEENING] = 3; w[PF_CHANT] = 1; break;
                case ST_WARNING: w[PF_WARNING] = 2; w[PF_CHANT] = 1; break;
                case ST_END: w[PF_LAST] = 1; break;
                default: w[PF_CHANT] = 3; w[PF_RINGDANCE] = 3; w[PF_HUNT] = 2; w[PF_LULLABY] = 1.5; w[PF_HEALING] = 1.2; w[PF_WORK] = 1.2; w[PF_HARVEST] = 0.8; break;
            }
            break;
        case AGE_MEDIEVAL:
            switch (tone) {
                case ST_ELEGY: w[PF_LAMENT] = 3; w[PF_HYMN] = 1; break;
                case ST_WARNING: w[PF_MARCH] = 2; w[PF_WARNING] = 2; w[PF_HYMN] = 1; break;
                case ST_END: w[PF_LAST] = 1; break;
                default: w[PF_DANCE] = 3; w[PF_LULLABY] = 2; w[PF_WORK] = 2; w[PF_HYMN] = 2; w[PF_HARVEST] = 1.2; w[PF_LOVE] = 1.2; w[PF_FESTIVAL] = 1; w[PF_PROCESSION] = 1; w[PF_BALLAD] = 1.2; break;
            }
            break;
        case AGE_INDUSTRIAL:
            switch (tone) {
                case ST_ELEGY: w[PF_FUNERAL] = 2.5; w[PF_HYMN] = 1; break;
                case ST_WARNING: w[PF_MARCH] = 2; w[PF_ANTHEM] = 1.5; break;
                case ST_END: w[PF_LAST] = 1; break;
                default: w[PF_MARCH] = 2; w[PF_TURNING] = 2.5; w[PF_PARLOUR] = 2; w[PF_HYMN] = 1.2; w[PF_MUSICBOX] = 1.3; w[PF_ANTHEM] = 1; w[PF_OVERTURE] = 1; break;
            }
            break;
        case AGE_MODERN:
            switch (tone) {
                case ST_ELEGY: w[PF_REQUIEM] = 2; w[PF_BALLAD] = 1; break;
                case ST_WARNING: w[PF_PROTEST] = 2; w[PF_ANTHEM] = 1; break;
                case ST_END: w[PF_LAST] = 1; break;
                default: w[PF_SONG] = 4; w[PF_DANCEREC] = 2.5; w[PF_BALLAD] = 2; w[PF_JINGLE] = 1; w[PF_THEME] = 1; w[PF_ANTHEM] = 0.8; w[PF_LULLABY] = 0.8; break;
            }
            break;
        case AGE_FUTURISTIC:
            switch (tone) {
                case ST_ELEGY: w[PF_ELEGY] = 2.5; w[PF_SWARM] = 1; break;
                case ST_WARNING: w[PF_SIGNAL] = 2; w[PF_PROCESS] = 1; break;
                case ST_END: w[PF_LAST] = 1; break;
                default: w[PF_PROCESS] = 3; w[PF_SWARM] = 2; w[PF_SYNSONG] = 2.5; w[PF_DRIFT] = 2; break;
            }
            break;
        default:
            switch (tone) {
                case ST_ELEGY: w[PF_STANDING] = 2.5; w[PF_FIELD] = 1; break;
                case ST_WARNING: w[PF_TURNFIELD] = 2; w[PF_BELLFIELD] = 1; break;
                case ST_END: w[PF_LAST] = 1; break;
                default: w[PF_FIELD] = 3; w[PF_TURNFIELD] = 2.5; w[PF_BELLFIELD] = 2; break;
            }
            break;
    }
    if (dark <= 0) return rng.pick(w, PF_DRAWN_COUNT);
    double sum = 0; for (int f = 0; f < PF_COUNT; f++) sum += w[f];
    switch (dark) {
        case 1: w[PF_DAWN] = 0.6 * sum; break;
        case 2: w[PF_DAWN] = 0.5 * sum; w[PF_LASTSUN] = 0.5 * sum; break;
        default: for (int f = 0; f < PF_COUNT; f++) w[f] *= 0.5; w[PF_LAST] += 0.35 * sum; w[PF_LULLABY] += 0.2 * sum; w[PF_LASTSUN] = 0.15 * sum; break;
    }
    return rng.pick(w, PF_COUNT);
}
// D-05: the form whose way a dawn song and the last sunrise are composed in, by the age (an age's hymn or chant, its lament)
int composedAs(int form, int age) {
    static const int DAWN[AGE_COUNT] = {PF_CHANT, PF_HYMN, PF_HYMN, PF_BALLAD, PF_SYNSONG, PF_FIELD};
    static const int SUNSET[AGE_COUNT] = {PF_KEENING, PF_LAMENT, PF_FUNERAL, PF_REQUIEM, PF_ELEGY, PF_STANDING};
    if (form == PF_DAWN) return DAWN[clampi(age, 0, AGE_COUNT - 1)];
    if (form == PF_LASTSUN) return SUNSET[clampi(age, 0, AGE_COUNT - 1)];
    return form;
}

double formTempo(int form, int age) {
    if (form < 11 && age == AGE_MEDIEVAL) return C04_FP[form].tempo;
    switch (form) {
        case PF_RINGDANCE: case PF_DANCE: case PF_FESTIVAL: return 1.2;
        case PF_CHANT: case PF_WORK: case PF_HARVEST: return 1.0;
        case PF_HUNT: return 0.85;
        case PF_KEENING: case PF_LAMENT: case PF_LAST: case PF_REQUIEM: case PF_ELEGY: return 0.6;
        case PF_HEALING: return 0.8;
        case PF_LULLABY: return 0.66;
        case PF_WARNING: return 1.15;
        case PF_PROCESSION: return 0.75;
        case PF_BALLAD: return age >= AGE_MODERN ? 0.72 : 0.85;
        case PF_MARCH: return 1.05;
        case PF_TURNING: return 1.1;
        case PF_ANTHEM: return 0.72;
        case PF_PARLOUR: return 0.85;
        case PF_MUSICBOX: return 1.0;
        case PF_OVERTURE: return 1.2;
        case PF_FUNERAL: return 0.62;
        case PF_HYMN: return 0.68;
        case PF_LOVE: return 0.82;
        case PF_SONG: return 1.0;
        case PF_DANCEREC: return 1.15;
        case PF_JINGLE: return 1.2;
        case PF_THEME: return 1.1;
        case PF_PROTEST: return 1.1;
        case PF_PROCESS: return 1.15;
        case PF_SWARM: return 0.7;
        case PF_SYNSONG: return 0.95;
        case PF_DRIFT: return 0.6;
        case PF_SIGNAL: return 1.2;
        default: return 1.0;
    }
}

}   // namespace

Tradition traditionOf(const BodyGen& g, const Lore& L) {
    Tradition T; T.seed = mix64(loreSeed(g, L) ^ 0x3C04A1ULL);   // C-13: the people's
    Rng rng(T.seed);
    // the period: the octave, now and then a stretched or a narrow one, rarely the twelfth
    double pr = rng.uni();
    T.period = pr < 0.82 ? 2.0 : (pr < 0.95 ? rng.range(1.93, 2.08) : 3.0);
    bool twelfth = T.period > 2.5;
    double perCents = centsOf(T.period);
    static const double NW[5] = {2, 2, 3, 1.2, 0.8};   // five to nine notes
    int n = 5 + rng.pick(NW, 5);
    if (twelfth) n = std::max(n, 7);
    const double minGap = 75, maxGap = 450;   // cents between neighbours: enough to tell them apart, no hole wider than a third and a half
    for (int attempt = 0; attempt < 60 && T.scale.empty(); attempt++) {
        std::vector<double> cents;
        T.tuning = rng.chance(0.55) ? 0 : 1;
        if (T.tuning == 0) {
            static const int EDO[13] = {5, 7, 9, 10, 12, 13, 14, 15, 16, 17, 19, 22, 24};
            static const double EW[13] = {2, 3, 1, 1, 4, 1.5, 1, 1, 1, 1.5, 2, 1, 1};
            T.division = twelfth ? 13 : EDO[rng.pick(EW, 13)];
            if (T.division < n + 1) continue;
            std::vector<int> steps; for (int i = 1; i < T.division; i++) steps.push_back(i);
            for (int i = (int)steps.size() - 1; i > 0; i--) std::swap(steps[i], steps[rng.irange(i + 1)]);
            cents.push_back(0);
            for (int i = 0; i < n - 1; i++) cents.push_back(steps[i] * perCents / T.division);
        } else {
            static const double JI[19] = {16.0 / 15, 10.0 / 9, 9.0 / 8, 8.0 / 7, 7.0 / 6, 6.0 / 5, 5.0 / 4, 9.0 / 7, 4.0 / 3, 11.0 / 8, 7.0 / 5, 3.0 / 2, 14.0 / 9, 8.0 / 5, 5.0 / 3, 7.0 / 4, 16.0 / 9, 9.0 / 5, 15.0 / 8};
            std::vector<double> pool;
            for (double r : JI) if (r < T.period * 0.98) pool.push_back(r);
            if (twelfth) { const double extra[8] = {2.0, 9.0 / 4, 7.0 / 3, 5.0 / 2, 18.0 / 7, 8.0 / 3, 11.0 / 4, 25.0 / 9}; for (double r : extra) pool.push_back(r); }
            for (int i = (int)pool.size() - 1; i > 0; i--) std::swap(pool[i], pool[rng.irange(i + 1)]);
            cents.push_back(0);
            for (int i = 0; i < n - 1 && i < (int)pool.size(); i++) cents.push_back(centsOf(pool[i]));
        }
        std::sort(cents.begin(), cents.end());
        bool ok = (int)cents.size() == n;
        for (size_t i = 1; i < cents.size() && ok; i++) if (cents[i] - cents[i - 1] < minGap || cents[i] - cents[i - 1] > maxGap) ok = false;
        if (ok && (perCents - cents.back() < minGap || perCents - cents.back() > maxGap)) ok = false;
        if (!ok) continue;
        for (double c : cents) T.scale.push_back(std::pow(2.0, c / 1200.0));
    }
    if (T.scale.empty()) { T.tuning = 0; T.division = 12; T.period = 2.0; const double P5[5] = {0, 200, 400, 700, 900}; for (double c : P5) T.scale.push_back(std::pow(2.0, c / 1200.0)); }
    n = (int)T.scale.size();
    // the resting degree: the one nearest a fifth, else a fourth, else the middle
    {
        int best = -1; double bd = 60;
        for (int d = 1; d < n; d++) { double c = centsOf(T.scale[d]); if (std::fabs(c - 702) < bd) { bd = std::fabs(c - 702); best = d; } }
        if (best < 0) { bd = 60; for (int d = 1; d < n; d++) { double c = centsOf(T.scale[d]); if (std::fabs(c - 498) < bd) { bd = std::fabs(c - 498); best = d; } } }
        T.rest = best < 0 ? n / 2 : best;
    }
    T.base = 165.0 * std::pow(2.0, rng.uni());   // 165..330 Hz
    // the cycle
    static const int BEATS[10] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    static const double BW[10] = {0.5, 1.5, 2, 1.5, 3, 2, 1.5, 1, 0.8, 0.8};
    T.beats = BEATS[rng.pick(BW, 10)];
    for (int rem = T.beats; rem > 0;) {
        int gch;
        if (rem <= 3) gch = rem;
        else if (rem == 4) gch = rng.chance(0.3) ? 4 : 2;
        else { double w[3] = {1, 1, rem >= 6 ? 0.25 : 0}; gch = 2 + rng.pick(w, 3); if (rem - gch == 1) gch = gch == 2 ? 3 : 2; }
        T.groups.push_back(gch); rem -= gch;
    }
    T.sub = rng.chance(0.75) ? 2 : 3;
    T.bpm = rng.range(66, 144);
    T.drift = rng.range(0.01, 0.05);
    // the timbres, by the style of the names: hard names lean to struck and plucked things, flowing ones to pipes and bows
    {
        double fw[5] = {3, 2.5, 1.5, 1.5, 1.5};   // plucked, pipe, bowed, metal, reed
        if (L.style == 1) { fw[0] += 1.5; fw[3] += 1.5; }
        if (L.style == 2) { fw[1] += 1.5; fw[2] += 1.5; }
        int count = rng.chance(0.55) ? 2 : 3;
        for (int i = 0; i < count; i++) { int f = rng.pick(fw, 5); fw[f] = 0; T.voices.push_back(makeTimbre(f, rng)); }
    }
    T.drum = makeTimbre(TF_DRUM, rng);
    T.hasDrum = rng.chance(0.75) || L.style == 1;
    T.drone = rng.chance(0.4);
    { double sw[4] = {1.5, 1.5, 1.5, 1}; T.second = rng.pick(sw, 4); }
    { double ow[5] = {1.5, 1.5, 1, 1.2, 1}; T.ornament = rng.pick(ow, 5); }
    T.ornamentChance = rng.range(0.2, 0.5);
    // H-05: what holds through the people's ages (a stream of its own, so C-04's draws above stay), then each age's ensemble from its own
    Rng hr(mix64(T.seed ^ 0x405A6E5ULL));
    T.swing = T.sub == 2 && hr.chance(0.45) ? hr.range(0.08, 0.28) : 0;
    T.density = hr.range(0.88, 1.18);
    T.refrain = (int)(mix64(T.seed ^ 0x4EF4A1ULL) & 0x7FFFFFFF);
    T.octaves = hr.chance(0.35);
    {   // the vowels the singers sing: the people's own tongue's (the first letter of each of its vowels), the open ones first
        const Tongue& tg = tongueOfSeed(loreSeed(g, L));
        bool have[5] = {false, false, false, false, false};
        for (const std::string& v : tg.vows) {
            if (v.empty()) continue;
            switch (std::tolower((unsigned char)v[0])) { case 'a': have[0] = true; break; case 'e': have[1] = true; break; case 'i': case 'y': have[2] = true; break; case 'o': have[3] = true; break; case 'u': have[4] = true; break; default: break; }
        }
        for (int k = 0; k < 5; k++) if (have[k]) T.vowels.push_back(k);
        if (T.vowels.empty()) T.vowels = {0, 3, 4};
        const Voice V = voiceOf(g, L, tg);
        for (int a = 0; a < AGE_COUNT; a++) { Rng er(mix64(T.seed ^ (0xA6E5ULL + (uint64_t)(a + 1) * 0x9E3779B97F4A7C15ULL))); T.ages[a] = makeEnsemble(T, a, er, V, L.style); }
    }
    traditionMakes(T, g, L);   // C-17: the instruments as the people made them (streams of their own: the sound above stays), their names
    return T;
}

double traditionHz(const Tradition& T, int degree) {
    int n = (int)T.scale.size(); if (n == 0) return T.base;
    int oct = degree >= 0 ? degree / n : -((-degree + n - 1) / n);
    return T.base * std::pow(T.period, oct) * T.scale[degree - oct * n];
}
double traditionCents(const Tradition& T, int degree) { return centsOf(traditionHz(T, degree) / T.base); }

bool pieceHasDrum(const Piece& P) { for (const Note& n : P.notes) if (n.voice < 0) return true; return false; }
const Ensemble& pieceEnsemble(const Tradition& T, const Piece& P) { return T.ages[clampi(P.age, 0, AGE_COUNT - 1)]; }

std::string traditionLine(const Tradition& T) {
    char buf[96];
    const char* per = T.period > 2.5 ? "THE TWELFTH" : (T.period > 2.01 ? "A STRETCHED OCTAVE" : (T.period < 1.99 ? "A NARROW OCTAVE" : "THE OCTAVE"));
    snprintf(buf, sizeof buf, "A SCALE OF %d NOTES TO %s", (int)T.scale.size(), per);
    return buf;
}
std::string cycleLine(const Tradition& T, double bpm) {
    std::string g; for (size_t i = 0; i < T.groups.size(); i++) { if (i) g += "+"; g += std::to_string(T.groups[i]); }
    char buf[96]; snprintf(buf, sizeof buf, "A CYCLE OF %s AT %d BEATS A MINUTE", g.c_str(), (int)std::lround(bpm));
    return buf;
}
std::string cycleLine(const Tradition& T, const Piece& P) {
    if (P.age != AGE_FAR_FUTURE) return cycleLine(T, P.bpm);
    int lo = 1 << 20, hi = -(1 << 20); std::vector<int> seen;
    const int n = std::max(1, (int)T.scale.size());
    for (const Note& nt : P.notes) if (nt.voice >= 0) { lo = std::min(lo, nt.degree); hi = std::max(hi, nt.degree); if (std::find(seen.begin(), seen.end(), nt.degree) == seen.end()) seen.push_back(nt.degree); }
    const int periods = hi >= lo ? (hi - lo) / n + 1 : 1;
    char buf[96]; snprintf(buf, sizeof buf, "NO BEAT: %d TONES OVER %d %s", (int)seen.size(), periods, T.period > 2.5 ? (periods == 1 ? "TWELFTH" : "TWELFTHS") : (periods == 1 ? "OCTAVE" : "OCTAVES"));
    return buf;
}
namespace {
std::string listLine(const std::vector<std::string>& parts, size_t maxLen) {
    std::vector<std::string> p = parts;
    for (;;) {
        std::string s;
        for (size_t i = 0; i < p.size(); i++) { if (i) s += i + 1 == p.size() ? " AND " : ", "; s += p[i]; }
        if (s.size() <= maxLen || p.size() <= 1) return s;
        p.pop_back();
    }
}
}
std::string timbresLine(const Tradition& T, bool drum) {
    std::vector<std::string> parts;
    for (const Timbre& t : T.voices) parts.push_back(TIMBRE_FAMILY_NAMES[t.family]);
    if (drum) parts.push_back(TIMBRE_FAMILY_NAMES[TF_DRUM]);
    std::string s;
    for (size_t i = 0; i < parts.size(); i++) { if (i) s += i + 1 == parts.size() ? " AND " : ", "; s += parts[i]; }
    if (T.drone && s.size() + 9 <= 52) s += ", A DRONE";
    return s;
}
std::string timbresLine(const Tradition& T, const Piece& P) {
    const Ensemble& E = pieceEnsemble(T, P);
    std::vector<int> used(E.voices.size(), 0), kitUsed(E.kit.size(), 0);
    for (const Note& nt : P.notes) { if (nt.voice >= 0 && nt.voice < (int)used.size()) used[nt.voice]++; else if (nt.voice < 0 && nt.degree >= 0 && nt.degree < (int)kitUsed.size()) kitUsed[nt.degree]++; }
    std::vector<std::pair<int, std::string>> parts;   // by how much each plays, the most first; a name once
    auto put = [&](int count, const std::string& name) { for (auto& p : parts) if (p.second == name) { p.first += count; return; } parts.push_back({count, name}); };
    for (size_t i = 0; i < E.voices.size(); i++) if (used[i]) put(used[i] + 100000, E.voices[i].name);
    for (size_t i = 0; i < E.kit.size(); i++) if (kitUsed[i]) put(kitUsed[i], E.kit[i].name);
    std::stable_sort(parts.begin(), parts.end(), [](const std::pair<int, std::string>& a, const std::pair<int, std::string>& b) { return a.first > b.first; });
    std::vector<std::string> names; for (auto& p : parts) names.push_back(p.second);
    return listLine(names, 52);
}
std::string ensembleLine(const Ensemble& E) {
    static const char* ROLE[MR_COUNT] = {"lead", "second", "chorus", "chords", "bass", "drone", "alt", "organ", "box", "singer", "bell", "keys", "swarm"};
    static const char* KROLE[KR_COUNT] = {"low", "high", "tuned", "shake", "back", "tick", "open", "crash", "sub"};
    std::string s;
    for (size_t i = 0; i < E.voices.size(); i++) { if (i) s += ", "; s += std::string(ROLE[clampi(E.role[i], 0, MR_COUNT - 1)]) + " " + E.voices[i].name; }
    if (!E.kit.empty()) { s += "; kit:"; for (size_t i = 0; i < E.kit.size(); i++) s += std::string(i ? ", " : " ") + KROLE[clampi(E.kitRole[i], 0, KR_COUNT - 1)] + " " + TIMBRE_FAMILY_NAMES[E.kit[i].family]; }
    return s;
}

void pieceOf(const Tradition& T, const Shard& s, Piece& P) {
    P = Piece(); P.seed = mix64(s.seed ^ 0x3C04ULL);
    Rng rng(P.seed);
    P.age = clampi(s.recordAge >= 0 ? s.recordAge : AGE_MEDIEVAL, 0, AGE_COUNT - 1);   // H-05: the record's age (C-04's pieces were the medieval's)
    const Ensemble& E = T.ages[P.age];
    P.form = formOf(s.tone, P.age, rng, s.dark);
    const int built = composedAs(P.form, P.age);   // D-05: a dawn song or the last sunrise in its age's hymn's or lament's way
    P.drift = E.drift;
    P.bpm = std::min(190.0, std::max(P.age >= AGE_INDUSTRIAL ? 52.0 : 44.0, T.bpm * E.tempo * formTempo(built, P.age) * rng.range(0.92, 1.08)));
    Bld b(T, E, P, rng);
    switch (P.age) {
        case AGE_PREHISTORIC: buildPrehistoric(b, built); break;
        case AGE_MEDIEVAL: buildMedieval(b, built); break;
        case AGE_INDUSTRIAL: buildIndustrial(b, built); break;
        case AGE_MODERN: buildModern(b, built); break;
        case AGE_FUTURISTIC: buildFuturistic(b, built); break;
        default: buildFarFuture(b, built); break;
    }
    if (built != P.form) {   // D-05: the dawn song's line climbs as it goes (the sun called up), the last sunrise's rises to its middle and sinks below where it began
        double len = 1; for (const Note& nt : P.notes) len = std::max(len, nt.t + nt.dur);
        for (Note& nt : P.notes) {
            if (nt.voice < 0) continue;   // the kit keeps its pieces
            const double f = clampd(nt.t / len, 0, 1);
            const int lift = P.form == PF_DAWN ? (int)std::floor(4.0 * f) : (int)std::lround(3.0 * std::sin(PI * f) - 2.0 * f * f);
            nt.degree += lift;
            if (nt.slideFrom > -1000) nt.slideFrom += lift;
        }
    }
    // the people's lilt on the second of two subdivisions (the hand-played ages and the songs; never the machines' processes or the field)
    if (T.swing > 0 && T.sub == 2 && P.age <= AGE_MODERN) {
        const double sw = T.swing * 0.5;
        for (Note& nt : P.notes) {
            const double f = nt.t * 2 - std::floor(nt.t * 2 + 1e-6);
            const int half = (int)std::floor(nt.t * 2 + 1e-6);
            if (f < 1e-4 && half % 2 == 1) { nt.t += sw; nt.dur = std::max(0.03, nt.dur - sw); }
        }
    }
    // the end: every note within the piece, the piece's length its last note's end (a field's long fade inside it), in time order
    double end = 0;
    for (const Note& nt : P.notes) end = std::max(end, nt.t + nt.dur);
    {   // a held note left open (a healing song's bullroarer) ends with the piece
        double last = 0; for (const Note& nt : P.notes) if (nt.dur < 500) last = std::max(last, nt.t + nt.dur);
        for (Note& nt : P.notes) if (nt.t + nt.dur > last + 1e-6) nt.dur = std::max(0.05, last - nt.t);
        end = last;
    }
    P.beats = std::max(P.beats, end);
    for (const Note& nt : P.notes) P.beats = std::max(P.beats, nt.t + nt.dur);
    std::stable_sort(P.notes.begin(), P.notes.end(), [](const Note& a, const Note& b) { return a.t < b.t; });
    P.cycles = std::max(1, (int)std::lround(P.beats / std::max(1, T.beats)));
    P.seconds = P.beats * 60.0 / P.bpm;
}
