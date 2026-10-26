#include "music.h"
#include "music_internal.h"

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
    static const char* const ROLE[MR_COUNT] = {"lead", "second", "chorus", "chords", "bass", "drone", "alt", "organ", "box", "singer", "bell", "keys", "swarm"};
    static const char* const KROLE[KR_COUNT] = {"low", "high", "tuned", "shake", "back", "tick", "open", "crash", "sub"};
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
