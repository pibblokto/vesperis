// C-07 (2026-10-02): the signal radar's sources (see signals.h).
#include "signals.h"
#include "planetmap.h"
#include "ruins.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

const char* const SIGNAL_KIND_NAMES[SIG_KIND_COUNT] = {"A PEOPLE'S TRANSMISSION", "A PULSAR", "A COMET OUTGASSING", "A MAGNETOSPHERE"};

namespace {
// the cells' salt is a design choice like the pinned home star (G-02): of forty tried, this one gives the home sector three
// peoples' signals within reach (a desert world of a yellow star at 79 ly, a felisian one of another at 85, a third at 179) and
// the two test worlds three each; fifteen of the forty gave home none (the mean is 1.8 within 200 ly on the arm)
constexpr uint64_t SALT_CELL = 0xC07FULL, SALT_PULSAR = 0x9F5A1ULL, SALT_AIR = 0x0A1ULL;
inline int64_t floorDiv(int64_t a, int64_t b) { int64_t q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) q--; return q; }
}

// a cell holds one candidate with probability 0.09, two with 0.01, three with 0.001 (a mean of 0.113 a cell: 1,023 cells within
// 200 ly give 115 candidate sectors, 17 of them with a star and 2 of those with a people's world, on the arm's density)
int transmitterCell(int64_t cx, int64_t cy, int64_t cz, int64_t* sx, int64_t* sy, int64_t* sz) {
    uint64_t h = hash3i(cx, cy, cz, SALT_CELL);
    double u = unitFromHash(h);
    int n = u < 0.001 ? 3 : (u < 0.011 ? 2 : (u < 0.101 ? 1 : 0));
    for (int k = 0; k < n; k++) {
        uint64_t hk = mix64(h ^ (0x1234ULL * (k + 1)));
        sx[k] = cx * SIGNAL_CELL + (int64_t)(hk % SIGNAL_CELL);
        sy[k] = cy * SIGNAL_CELL + (int64_t)((hk >> 20) % SIGNAL_CELL);
        sz[k] = cz * SIGNAL_CELL + (int64_t)((hk >> 40) % SIGNAL_CELL);
    }
    return n;
}

bool sectorTransmits(int64_t sx, int64_t sy, int64_t sz) {
    int64_t cx[SIGNAL_MAX_CANDIDATES], cy[SIGNAL_MAX_CANDIDATES], cz[SIGNAL_MAX_CANDIDATES];
    int n = transmitterCell(floorDiv(sx, SIGNAL_CELL), floorDiv(sy, SIGNAL_CELL), floorDiv(sz, SIGNAL_CELL), cx, cy, cz);
    for (int k = 0; k < n; k++) if (cx[k] == sx && cy[k] == sy && cz[k] == sz) return true;
    return false;
}

bool starSignal(const Star& s, Signal& out) {
    if (!s.valid) return false;
    if (s.cls == STAR_PULSAR) {
        out = Signal(); out.kind = SIG_PULSAR; out.star = s; out.pulseHz = s.pulseHz; out.seed = s.seed; out.pos = s.pos;
        return true;
    }
    StarSystem sys; sys.generate(s);
    for (const Body& b : sys.bodies) {
        if (b.type != PT_FELISIAN && b.type != PT_DESERT) continue;
        BodyGen g = BodyGen::make(b);
        if (!worldHadCivilisation(g)) continue;
        out = Signal(); out.kind = SIG_PEOPLE; out.star = s; out.body = b.index; out.bodyType = b.type; out.seed = b.seed; out.pos = s.pos;
        return true;
    }
    return false;
}

double signalStrengthAt(double distLy, double reachLy) {
    double u = clampd(distLy / std::max(reachLy, 1e-9), 0, 1);
    return 1.0 - 0.82 * std::sqrt(u);
}

int signalsNear(const Vec3& obsKm, std::vector<Signal>& out, double reachLy) {
    out.clear();
    int64_t ox = sectorOf(obsKm.x), oy = sectorOf(obsKm.y), oz = sectorOf(obsKm.z);
    int64_t R = (int64_t)std::ceil(reachLy);
    int64_t c0x = floorDiv(ox - R, SIGNAL_CELL), c1x = floorDiv(ox + R, SIGNAL_CELL), c0y = floorDiv(oy - R, SIGNAL_CELL), c1y = floorDiv(oy + R, SIGNAL_CELL), c0z = floorDiv(oz - R, SIGNAL_CELL), c1z = floorDiv(oz + R, SIGNAL_CELL);
    int cells = 0;
    double reachKm = reachLy * SECTOR_KM;
    for (int64_t cx = c0x; cx <= c1x; cx++) for (int64_t cy = c0y; cy <= c1y; cy++) for (int64_t cz = c0z; cz <= c1z; cz++) {
        cells++;
        int64_t sx[SIGNAL_MAX_CANDIDATES], sy[SIGNAL_MAX_CANDIDATES], sz[SIGNAL_MAX_CANDIDATES];
        int n = transmitterCell(cx, cy, cz, sx, sy, sz);
        for (int k = 0; k < n; k++) {
            Star s; if (!starInSector(sx[k], sy[k], sz[k], s, false)) continue;
            double d = length(s.pos - obsKm);
            if (d > reachKm || d < 1.0) continue;
            Signal sig; if (!starSignal(s, sig)) continue;
            sig.distLy = d / SECTOR_KM; sig.strength = signalStrengthAt(sig.distLy, reachLy);
            out.push_back(sig);
        }
    }
    // the loud pulsars near: a share of those within their own reach, by their own hash
    int64_t P = (int64_t)std::ceil(SIGNAL_PULSAR_REACH_LY);
    double pReachKm = SIGNAL_PULSAR_REACH_LY * SECTOR_KM;
    for (int64_t x = ox - P; x <= ox + P; x++) for (int64_t y = oy - P; y <= oy + P; y++) for (int64_t z = oz - P; z <= oz + P; z++) {
        if (unitFromHash(hash3i(x, y, z, SALT_PULSAR)) >= SIGNAL_PULSAR_SHARE) continue;
        Star s; if (!starInSector(x, y, z, s, false) || s.cls != STAR_PULSAR) continue;
        double d = length(s.pos - obsKm);
        if (d > pReachKm || d < 1.0) continue;
        bool have = false; for (const Signal& o : out) if (o.star.seed == s.seed) have = true;
        if (have) continue;
        Signal sig; if (!starSignal(s, sig)) continue;
        sig.distLy = d / SECTOR_KM; sig.strength = signalStrengthAt(sig.distLy, SIGNAL_PULSAR_REACH_LY) * 0.8;
        out.push_back(sig);
    }
    std::sort(out.begin(), out.end(), [](const Signal& a, const Signal& b) { return a.distLy < b.distLy; });
    return cells;
}

void localSignals(const StarSystem& sys, const Vec3& obsKm, double t, std::vector<Signal>& out) {
    out.clear();
    if (!sys.valid) return;
    bool transmits = sectorTransmits(sys.star.sx, sys.star.sy, sys.star.sz);
    for (const Body& b : sys.bodies) {
        Vec3 p = sys.bodyPos(b.index, t);
        double d = length(p - obsKm);
        if (d < 1.0) continue;
        if (transmits && (b.type == PT_FELISIAN || b.type == PT_DESERT) && worldHadCivilisation(BodyGen::make(b))) {   // the people of this very system
            Signal sig; sig.kind = SIG_PEOPLE; sig.star = sys.star; sig.body = b.index; sig.bodyType = b.type; sig.pos = p; sig.seed = b.seed; sig.local = true;
            sig.distLy = d / SECTOR_KM; sig.strength = 1.0;
            out.push_back(sig);
            continue;
        }
        if (b.type == PT_COMET) {   // outgassing within 1.5 AU of its sun: the louder the nearer the sun, fading with the ship's distance
            double au = length(p - sys.parentPos(b.index, t)) / AU_GAME_KM;
            if (au > 1.5) continue;
            Signal sig; sig.kind = SIG_COMET; sig.star = sys.star; sig.body = b.index; sig.bodyType = b.type; sig.pos = p; sig.seed = b.seed; sig.local = true;
            sig.distLy = d / SECTOR_KM; sig.detail = au;
            sig.strength = clampd((1.0 - au / 1.5) * 0.9, 0.25, 0.9) * clampd(1.3 - d / (4 * AU_GAME_KM), 0.35, 1.0);
            out.push_back(sig);
        } else {   // a giant's always; a world's only when its field is of the strongest and the world big enough to hold a magnetosphere worth the name
            double mag = magneticField(b);
            bool giant = b.type == PT_GASGIANT || b.type == PT_SUBSTELLAR;
            if (!giant && (mag < 0.8 || b.radiusKm < 2500)) continue;
            Signal sig; sig.kind = SIG_MAGNETOSPHERE; sig.star = sys.star; sig.body = b.index; sig.bodyType = b.type; sig.pos = p; sig.seed = b.seed; sig.local = true;
            sig.distLy = d / SECTOR_KM; sig.detail = giant ? std::max(mag, 0.6) : mag;
            sig.strength = clampd(0.35 + 0.6 * sig.detail, 0.3, 0.95) * clampd(1.4 - d / (6 * AU_GAME_KM), 0.3, 1.0);
            out.push_back(sig);
        }
    }
}

double beamGain(double a) {
    a = std::fabs(a);
    double w = a / (40 * DEG), m = a / (10 * DEG), n = a / (3 * DEG);
    return 0.3 * std::exp(-w * w) + 0.3 * std::exp(-m * m) + 0.4 * std::exp(-n * n);
}

// the broadcast walks the fifty in a stride of the world's own (coprime with fifty, so every recording comes round before any
// repeats), one recording a slot of SIGNAL_RECORDING_S from the game's start; a lock held hears the next in the same walk
namespace { int strideOf(uint64_t seed) { static const int S[20] = {1, 3, 7, 9, 11, 13, 17, 19, 21, 23, 27, 29, 31, 33, 37, 39, 41, 43, 47, 49}; return S[mix64(seed ^ SALT_AIR ^ 0x57E9ULL) % 20]; } }
int transmittedShard(uint64_t worldSeed, double t) {
    int64_t slot = (int64_t)std::floor(std::max(t, 0.0) / SIGNAL_RECORDING_S);
    int start = (int)(mix64(worldSeed ^ SALT_AIR) % 50);
    return (int)((start + (slot % 50) * strideOf(worldSeed)) % 50);
}
int nextTransmittedShard(uint64_t worldSeed, int prev) { return (((prev + strideOf(worldSeed)) % 50) + 50) % 50; }

std::string signalSourceLine(const Signal& s) {
    auto art = [](const std::string& w) { char c = w.empty() ? 'X' : w[0]; return std::string((c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U') ? "AN " : "A ") + w; };
    switch (s.kind) {
        case SIG_PEOPLE: return art(std::string(PLANET_TYPES[s.bodyType].name) + " WORLD OF " + art(STAR_CLASSES[s.star.cls].name));
        case SIG_PULSAR: { char buf[64]; snprintf(buf, sizeof buf, "A PULSAR, %.1f PULSES A SECOND", s.pulseHz); return buf; }
        case SIG_COMET: { char buf[64]; snprintf(buf, sizeof buf, "A COMET OUTGASSING %.1f AU FROM THE SUN", s.detail); return buf; }
        default: return s.bodyType >= 0 ? "THE MAGNETOSPHERE OF " + art(PLANET_TYPES[s.bodyType].name) : "A MAGNETOSPHERE";
    }
}

// ---- the programmes of a people's broadcast (the user's second call: more sounds, in the cosmic-horror direction) ----
const char* const RADIO_PROGRAMME_NAMES[RP_COUNT] = {"A VOICE", "WHISPERS", "A CHANT", "A NUMBERS STATION", "A LOOP", "A BEACON", "DATA BURSTS", "A BELL TOLLING", "A SIREN", "A MURMUR OF VOICES", "A PIECE, SLOWED"};

namespace {
inline double unit(uint64_t h) { return (h >> 11) * (1.0 / 9007199254740992.0); }
// the world's character: two of the five machines and two of the four ways of speaking
void worldCharacter(uint64_t worldSeed, int* m1, int* m2, int* v1, int* v2) {
    uint64_t h = mix64(worldSeed ^ 0x9C07C0DEULL);
    *m1 = (int)(h % 5); *m2 = (*m1 + 1 + (int)((h >> 8) % 4)) % 5;
    *v1 = (int)((h >> 16) % 4); *v2 = (*v1 + 1 + (int)((h >> 24) % 3)) % 4;
}
const int MACHINES[5] = {RP_BEACON, RP_DATA, RP_TOLL, RP_SIREN, RP_MURMUR};
const int SPEAKINGS[4] = {RP_WHISPERS, RP_CHANT, RP_NUMBERS, RP_LOOP};
}

uint64_t programmeSeed(uint64_t worldSeed, int slot) { return mix64(worldSeed ^ ((uint64_t)(slot + 1) * 0x5157A3B1ULL) ^ 0xC07B2ULL); }

// a music slot: the piece three times in ten (the user found the pieces through the static the least of it), else one of the
// world's two machines; a text slot: the voice as it is eleven times in twenty, the world's two ways of speaking three each,
// its machines the rest
int programmeOf(uint64_t worldSeed, int slot, bool music) {
    int m1, m2, v1, v2; worldCharacter(worldSeed, &m1, &m2, &v1, &v2);
    double u = unit(programmeSeed(worldSeed, slot) ^ 0x7E1ULL);
    if (music) return u < 0.3 ? RP_MUSIC : (u < 0.65 ? MACHINES[m1] : MACHINES[m2]);
    if (u < 0.55) return RP_VOICE;
    if (u < 0.70) return SPEAKINGS[v1];
    if (u < 0.85) return SPEAKINGS[v2];
    return u < 0.93 ? MACHINES[m1] : MACHINES[m2];
}

namespace {
// a digit group of four or five in the people's words for the digits, one word spoken digit by digit (as `decodeShard` reads a numeral)
DecodedWord digitGroup(const Tongue& T, uint64_t h, int year, int k) {
    static const char* DIGITS[10] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
    DecodedWord w; w.known = true; w.post = ".";
    std::string digits;
    if (k == 0 && year > 0) { digits = std::to_string(year); while (digits.size() < 4) digits = "0" + digits; }
    else { int n = 4 + (int)((h >> 3) % 2); uint64_t g = mix64(h ^ (uint64_t)(k + 11) * 0x9E37ULL); for (int i = 0; i < n; i++) { digits += (char)('0' + g % 10); g /= 10; } }
    w.ours = digits;
    for (char c : digits) { if (!w.theirs.empty()) w.theirs += '-'; w.theirs += tongueWord(T, DIGITS[c - '0']); }
    return w;
}
}

void programmeRepeatWords(const Programme& p, int k, const Tongue& T, std::vector<DecodedWord>& out) {
    if (p.kind == RP_NUMBERS && k > 0) { out.clear(); out.push_back(digitGroup(T, p.seed, 0, k)); return; }   // a fresh group each time
    out = p.words;
}

void programmeFor(uint64_t worldSeed, int slot, bool music, int year, const Voice& V, const Tongue& T, const std::vector<DecodedWord>& text, Programme& out, int force) {
    out = Programme();
    out.kind = force >= 0 && force < RP_COUNT ? force : programmeOf(worldSeed, slot, music);
    out.seed = programmeSeed(worldSeed, slot);
    uint64_t h = mix64(out.seed ^ 0x51ULL);
    out.voice = V;
    switch (out.kind) {
        case RP_VOICE: out.speaks = true; out.words = text; break;
        case RP_WHISPERS:   // close to the microphone, slower, the melody flattened
            out.speaks = true; out.words = text;
            out.voice.breath = 0.95; out.voice.rate = V.rate * 0.8; out.voice.range = V.range * 0.6; out.voice.rough = std::max(V.rough, 0.3); break;
        case RP_CHANT:      // deep, at half the rate, level, trembling, a tone an octave under
            out.speaks = true; out.words = text;
            out.voice.pitch = V.pitch * 0.72; out.voice.rate = V.rate * 0.5; out.voice.range = 0.35; out.voice.contour = 2; out.voice.stress = 3;
            out.voice.tremor = std::max(V.tremor, 0.6); out.voice.sub = 0.4; out.voice.subRatio = 0.5; out.voice.breath = std::min(V.breath, 0.5); break;
        case RP_NUMBERS:    // read flat and slow, a group at a time
            out.voice.range = 0.3; out.voice.rate = V.rate * 0.75; out.voice.contour = 2; out.voice.breath = std::max(V.breath, 0.1);
            out.words.push_back(digitGroup(T, out.seed, year, 0));
            out.repeats = 5 + (int)(h % 4); out.gap = 1.0 + 0.8 * unit(mix64(h ^ 3)); break;
        case RP_LOOP: {     // the opening words, three to six, again and again
            out.speaks = true; out.voice.range = V.range * 0.8;
            int n = 3 + (int)(h % 4); for (size_t i = 0; i < text.size() && (int)out.words.size() < n; i++) if (!text[i].ours.empty()) out.words.push_back(text[i]);
            if (!out.words.empty()) { out.words.back().post = "."; }
            out.repeats = 4 + (int)((h >> 8) % 4); out.gap = 3.0 + 5.0 * unit(mix64(h ^ 5)); break;
        }
        case RP_MUSIC: out.speaks = true; out.pieceSpeed = 0.5 + 0.25 * unit(mix64(h ^ 7)); break;
        default: out.machine = true; out.seconds = 20 + 25 * unit(mix64(h ^ 9)); break;
    }
    if (out.kind != RP_NUMBERS && out.kind != RP_LOOP) { out.repeats = 1; out.gap = 0; }
}
