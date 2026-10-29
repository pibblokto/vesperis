// E-03: the radio: `signals` (the receiver, the programmes, the drifters' and the beacons' voices), `voice`, `sounds` (the
// ground's)
#include "harness/harness.h"
#include "harness/test_worlds.h"
bool peopleWorldOf(const Star& s, int bi, PeopleWorld& w) {
    w = PeopleWorld(); w.sys.generate(s);
    if (bi < 0 || bi >= (int)w.sys.bodies.size()) return false;
    const Body& b = w.sys.bodies[bi]; BodyGen g = BodyGen::make(b);
    if (!g.hasTrait(TR_CIVILISATION)) return false;
    w.body = bi; w.seed = b.seed; w.name = b.name;
    w.lore = loreOf(w.sys, b, g); w.tongue = tongueOf(g, w.lore); w.voice = voiceOf(g, w.lore, w.tongue);
    shardsOf(w.sys, b, g, w.lore, SHARDS_PER_WORLD, w.shards); languageOf(w.lore, w.shards, w.tongue, w.lang); w.tradition = traditionOf(g, w.lore);
    for (int i = 0; i < (int)w.shards.size(); i++) { if (w.shards[i].music) { if (w.firstMusic < 0) w.firstMusic = i; } else if (w.firstText < 0) w.firstText = i; }
    w.valid = w.firstText >= 0 && w.firstMusic >= 0;
    return w.valid;
}
static void applyRx(AudioState& st, const RxOpts& o) {
    st.radar = o.level; st.radarRogue = o.rogue; st.radarRogue2 = o.rogue2; st.radarSet = o.set; st.hum = o.hum;
    st.radarGainT = o.gainT; st.radarSky = o.sky; st.radarWear = o.wear; st.radarDrive = o.drive; st.radarAge = o.age;
    st.radarKind2 = o.kind2; st.radarVoice2 = o.voice2; st.radarAge2 = o.age2; st.radarSeed2 = o.seed2; st.radarPulseHz2 = o.pulse2; st.radarSignal2 = o.sig2;
    st.radarHet = o.het; st.radarHetHz = o.hetHz; st.radarIntermod = o.intermod;
}
double gainShareOf(int g) { return (g - 1) / (double)(SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1] - 1); }
RenderStats renderProgramme(const PeopleWorld& w, int p, double seconds, std::vector<int16_t>* pcm, Programme* progOut, const RxOpts& o) {
    // C-15: the receiver overdriven; C-16: at a gain, worn, mixed
    AudioSynth synth; AudioState st; Programme prog; Speech speech; Piece piece; std::vector<DecodedWord> words;
    int slot = o.slot >= 0 ? o.slot : (p == RP_MUSIC || o.music ? w.firstMusic : w.firstText);
    const Shard& sh = w.shards[slot];
    if (!sh.music) decodeShard(sh, w.lang, w.tongue, SHARDS_PER_WORLD, words);
    programmeFor(w.seed, slot, sh.music, sh.year, w.voice, w.tongue, words, prog, p, o.age >= 0 ? o.age : AGE_MODERN);
    st.radar = 1; st.radarSignal = o.clarity; st.radarKind = SIG_PEOPLE; st.radarVoice = prog.kind; st.radarSeed = prog.seed; st.radio = true; st.master = 0.8;
    applyRx(st, o);
    if (prog.kind == RP_MORSE) { st.radarCode = &prog.code; st.radarCodeUnit = prog.unit; }
    if (prog.piece) { pieceOf(w.tradition, sh, piece); st.piece = &piece; st.tradition = &w.tradition; st.pieceSpeed = prog.pieceSpeed; st.pieceStart = true; }
    else if (!prog.machine) { speechOf(prog.voice, prog.words, sh.seed, speech); st.speech = &speech; st.voice = &prog.voice; st.speechStart = true; }
    int rep = 0; double gapT = -1;
    RenderStats r = renderReceiver(synth, st, seconds, pcm, [&](double dt) {
        if (st.speech && st.speechDone && prog.repeats > 1 && rep < prog.repeats - 1) {
            gapT = gapT < 0 ? 0 : gapT + dt;
            if (gapT >= prog.gap) {
                rep++;
                gapT = -1;
                std::vector<DecodedWord> ww;
                programmeRepeatWords(prog, rep, w.tongue, ww);
                st.speech = nullptr;
                speechOf(prog.voice, ww, sh.seed ^ (uint64_t)(rep * 0x9E37), speech);
                st.speech = &speech;
                st.speechStart = true;
                st.speechDone = false;
            }
        }
    });
    if (progOut) *progOut = prog;
    return r;
}
RenderStats renderKind(int kind, uint64_t seed, double pulseHz, double seconds, std::vector<int16_t>* pcm, const RxOpts& o, int* events, int* eventKinds) {
    AudioSynth synth; AudioState st;
    st.radar = 1; st.radarSignal = kind < 0 ? 0 : o.clarity; st.radarKind = kind; st.radarSeed = seed; st.radarPulseHz = pulseHz; st.master = 0.8;
    applyRx(st, o);
    if (kind == SIG_ROGUE) st.radarVoice = o.voice;   // D-03: a beacon in its slot under a drifter's voice
    RenderStats r = renderReceiver(synth, st, seconds, pcm);
    if (events) *events = st.radarEvents;
    if (eventKinds) *eventKinds = st.radarEventKinds;
    return r;
}
RogueRef rogueRefOf(const Star& d) {
    RogueRef r; r.star = d;
    StarSystem sys; sys.generate(d);
    r.sig.kind = SIG_ROGUE; r.sig.star = d; r.sig.body = 0; r.sig.seed = d.seed; r.sig.bodyType = sys.bodies.empty() ? -1 : sys.bodies[0].type; r.sig.power = drifterPower(d);
    rogueVoiceOf(sys, r.sig);
    r.voice = rogueVoiceFor(r.sig);
    return r;
}
RenderStats renderRogue(const RogueRef& r, double seconds, std::vector<int16_t>* pcm, RxOpts o) { o.rogue = r.voice; return renderKind(SIG_ROGUE, r.sig.seed, 1, seconds, pcm, o); }
RenderStats renderBeacon(const Star& d, double seconds, std::vector<int16_t>* pcm, BeaconInfo* info, RxOpts o) {
    if (info) *info = BeaconInfo();
    StarSystem sys; sys.generate(d);
    const DrifterBeacon B = drifterBeaconOf(sys);
    if (info) info->B = B;
    if (B.people < 0) return RenderStats();
    const RogueRef ref = rogueRefOf(d);
    const Body& b = sys.bodies[0]; const BodyGen g = BodyGen::make(b);
    const Lore L = loreOf(sys, b, g, B.people); const Tongue T = tongueOf(g, L); const Voice V = voiceOf(g, L, T);
    std::vector<Shard> shards; shardsOf(sys, b, g, L, L.records, shards); Language lang; languageOf(L, shards, T, lang);
    if (L.last < 0 || L.last >= (int)shards.size()) return RenderStats();
    const Shard& sh = shards[L.last];
    std::vector<DecodedWord> words; decodeShard(sh, lang, T, L.records, words); words.resize(shardOpening(words));
    Programme prog; beaconProgramme(d.seed, B, V, words, prog);
    Speech speech; speechOf(prog.voice, prog.words, sh.seed, speech);
    AudioSynth synth; AudioState st;
    st.radarSignal = o.clarity; st.radarKind = SIG_ROGUE; st.radarSeed = d.seed; st.radarPulseHz = 1; st.master = 0.8;
    o.rogue = ref.voice; applyRx(st, o);
    st.radarVoice = prog.kind; st.radio = true;
    st.speech = &speech; st.voice = &prog.voice; st.speechStart = true;
    int rep = 0; double gapT = -1;
    const RenderStats r = renderReceiver(synth, st, seconds, pcm, [&](double dt) {
        if (st.speech && st.speechDone && rep < prog.repeats - 1) {
            gapT = gapT < 0 ? 0 : gapT + dt;
            if (gapT >= prog.gap) {
                rep++;
                gapT = -1;
                st.speech = nullptr;
                speechOf(prog.voice, prog.words, sh.seed ^ (uint64_t)(rep * 0x9E37), speech);
                st.speech = &speech;
                st.speechStart = true;
                st.speechDone = false;
            }
        }
    });
    if (info) {
        info->valid = true; info->people = L.people; info->genre = shardKindName(sh.kind); info->words = (int)words.size(); info->speechS = speech.seconds; info->gap = prog.gap;
        for (const DecodedWord& w : words) { info->opening += w.pre + w.ours + w.post + " "; info->theirs += w.pre + (w.theirs.empty() ? w.ours : w.theirs) + w.post + " "; }
    }
    return r;
}
static std::vector<double> voicePrint(const std::vector<int16_t>& pcm) {
    const int N = 4096, sr = 22050, BANDS = 97;
    std::vector<double> power(N / 2, 0.0); std::vector<std::complex<double>> a(N); int frames = 0;
    for (size_t i0 = 0; i0 + N <= pcm.size(); i0 += N / 2, frames++) {
        for (int i = 0; i < N; i++) a[i] = pcm[i0 + i] / 32768.0 * (0.5 - 0.5 * std::cos(TAU * i / (N - 1)));
        fftInPlace(a);
        for (int b = 0; b < N / 2; b++) power[b] += std::norm(a[b]);
    }
    std::vector<double> out(BANDS, 0.0); double mean = 0;
    for (int k = 0; k < BANDS; k++) {
        const double f0 = 30 * std::pow(2.0, k / 12.0), f1 = 30 * std::pow(2.0, (k + 1) / 12.0);
        double p = 0; int n = 0;
        for (int b = std::max(1, (int)std::floor(f0 * N / sr)); b <= (int)std::ceil(f1 * N / sr) && b < N / 2; b++) { p += power[b]; n++; }
        out[k] = 10 * std::log10(p / std::max(1, n) / std::max(1, frames) + 1e-12); mean += out[k];
    }
    for (double& x : out) x -= mean / BANDS;
    return out;
}
static double printDistance(const std::vector<double>& a, const std::vector<double>& b) {
    double s = 0;
    for (size_t i = 0; i < a.size(); i++) s += (a[i] - b[i]) * (a[i] - b[i]);
    return std::sqrt(s / a.size());
}
Distinctness rogueDistinctness(const std::vector<RogueRef>& D, int ref, double refSeconds, int firstKind) {
    Distinctness r; r.n = (int)D.size();
    std::vector<std::vector<double>> P(D.size()), K((SIG_MAGNETOSPHERE - firstKind + 1) * ref);
    parallelFor((int)D.size(), 1,
            [&](int b, int e) {
                for (int i = b; i < e; i++) {
                    std::vector<int16_t> pcm;
                    renderRogue(D[i], std::max(32.0, AudioSynth::rogueBreathS(D[i].sig.seed, D[i].voice) + 2), &pcm);
                    P[i] = voicePrint(pcm);
                }
            },
            1);
    static const double RATES[5] = {0.6, 1.1, 1.7, 2.4, 3.2};
    const int kinds = SIG_MAGNETOSPHERE - firstKind + 1;
    parallelFor(
            kinds * ref, 1,
            [&](int b, int e) {
                for (int i = b; i < e; i++) {
                    const int kind = firstKind + i / ref, k = i % ref;
                    std::vector<int16_t> pcm;
                    renderKind(kind, 0x5EED0000ULL + (uint64_t)k * 0x9E37, RATES[k % 5], refSeconds, &pcm);
                    K[i] = voicePrint(pcm);
                }
            },
            1);
    std::vector<double> all;
    for (size_t i = 0; i < D.size(); i++) for (size_t j = i + 1; j < D.size(); j++) {
        const double d = printDistance(P[i], P[j]); all.push_back(d);
        if (d < r.nearest) { r.nearest = d; r.nearA = (int)i; r.nearB = (int)j; }
    }
    std::sort(all.begin(), all.end()); r.pairs = (int)all.size(); r.median = all.empty() ? 0 : all[all.size() / 2];
    for (int q = 0; q < kinds; q++) {   // `kindMedian` by kind from the pulsars' (0)
        const int kind = firstKind - SIG_PULSAR + q;
        std::vector<double> m;
        for (int i = 0; i < ref; i++)
            for (int j = i + 1; j < ref; j++) {
                const double d = printDistance(K[q * ref + i], K[q * ref + j]);
                m.push_back(d);
                r.kindNearest[kind] = std::min(r.kindNearest[kind], d);
            }
        std::sort(m.begin(), m.end()); r.kindMedian[kind] = m.empty() ? 0 : m[m.size() / 2];
        r.bar = std::max(r.bar, r.kindMedian[kind]);
    }
    r.inUnder.assign(D.size(), 0);
    for (size_t i = 0; i < D.size(); i++) for (size_t j = i + 1; j < D.size(); j++) if (printDistance(P[i], P[j]) <= r.bar) { r.under++; r.inUnder[i]++; r.inUnder[j]++; }
    std::vector<std::vector<double>> T(D.size()); for (size_t i = 0; i < D.size(); i++) AudioSynth::rogueTraits(D[i].sig.seed, D[i].voice, T[i]);
    for (size_t i = 0; i < D.size(); i++) for (size_t j = i + 1; j < D.size(); j++) {
        double s2 = 0; for (size_t k = 0; k < T[i].size(); k++) s2 += (T[i][k] - T[j][k]) * (T[i][k] - T[j][k]);
        if (std::sqrt(s2) < r.traitNearest) { r.traitNearest = std::sqrt(s2); r.traitA = (int)i; r.traitB = (int)j; }
    }
    return r;
}
// D-03: the drifters nearest a place, as many as asked, each with its voice (the field's, by `driftersNear`: heard or not)
std::vector<RogueRef> roguesNearest(const Vec3& at, int count) {
    std::vector<RogueRef> out;
    for (double reach = 300; reach <= 4800 && (int)out.size() < count; reach *= 2) {
        std::vector<Star> near; driftersNear(at, reach, near);
        if ((int)near.size() < count && reach < 4800) continue;
        for (const Star& d : near) { if ((int)out.size() >= count) break; out.push_back(rogueRefOf(d)); }
    }
    return out;
}
static double duneNoteOf(double g) { return 0.4 * std::sqrt(g / 250e-6); }   // Andreotti's, a grain of 250 microns
std::vector<GroundSound> groundSounds() {
    auto slide = [](double t) { return t < 1 ? t : (t < 4 ? 1.0 : std::exp(-(t - 4) / 2.5)); };
    auto quakeAt = [](double t) { return t < 0.3 ? 0.0 : 0.9 * std::exp(-(t - 0.3) / 4.0); };
    std::vector<GroundSound> S;
    for (const auto& w : {std::make_tuple("dunes_earth", 9.8, 1.0, "an Earth's gravity"), std::make_tuple("dunes_titan", 1.35, 1.0, "Titan's gravity under a thick air"),
                          std::make_tuple("dunes_mars", 3.7, 0.15, "a Mars' gravity under a thin air"), std::make_tuple("dunes_heavy", 14.0, 1.0, "a heavy world's gravity")}) {
        const double g = std::get<1>(w), air = std::get<2>(w);
        S.push_back({std::get<0>(w), 8.0, [=](AudioState& a, double t) { a.dunes = slide(t); a.duneHz = duneNoteOf(g); a.air = air; }, std::get<3>(w)});
    }
    S.push_back({"ice_dusk", 20.0, [](AudioState& a, double) { a.ice = 1.0; a.air = 1.0; }, "on the ice at dusk, its cooling the fastest"});
    S.push_back({"ice_noon", 20.0, [](AudioState& a, double) { a.ice = 0.25; a.air = 1.0; }, "on the ice at noon, settling"});
    S.push_back({"quake_air", 12.0, [=](AudioState& a, double t) { a.quake = quakeAt(t); if (t >= 0.3 && t < 0.4) a.quakeHit = 0.9; a.air = 1.0; }, "a quake under an air"});
    S.push_back({"quake_airless", 12.0,
            [=](AudioState& a, double t) {
                a.quake = quakeAt(t);
                if (t >= 0.3 && t < 0.4) a.quakeHit = 0.9;
                a.air = 0.0;
            },
            "a quake on an airless world, through the boots"});
    S.push_back({"quake_tectonic", 12.0,
            [=](AudioState& a, double t) {
                const KindDef& K = KINDS[PT_TECTONIC];
                a.quake = quakeAt(t);
                if (t >= 0.3 && t < 0.4) a.quakeHit = 0.9;
                a.lava = K.rumble + K.rumbleQuake * a.quake;
                a.air = 0.6;
            },
            "a tectonic world's quake, its lava's rumble with it"});
    return S;
}
RenderStats renderGroundSound(const GroundSound& gs, std::vector<int16_t>* pcm, int* iceEvents, std::vector<float>* raw) {
    AudioSynth synth; AudioState st; st.master = 0.8;
    double t = 0;
    gs.set(st, 0);
    std::vector<int16_t> tmp; std::vector<int16_t>* out = pcm ? pcm : (raw ? &tmp : nullptr);
    RenderStats r = renderReceiver(synth, st, gs.seconds, out, [&](double dt) { t += dt; st.quakeHit = 0; gs.set(st, t); });
    if (raw && out) { raw->resize(out->size()); for (size_t i = 0; i < out->size(); i++) (*raw)[i] = (*out)[i] / 32767.0f; }
    if (iceEvents) *iceEvents = st.iceEvents;
    return r;
}
void sceneSoundRun(SurfaceView& sv, double& t, double seconds, double walkS, SceneSound& ss) {
    const double dt = 1.0 / 30; std::vector<float> buf(735);
    for (double s = 0; s < seconds - 1e-9; s += dt) {
        Input in; if (s < walkS) in.down[KEY_W] = true;
        t += dt; sv.update(dt, in, t, true);
        groundSoundsTo(sv, ss.st);
        ss.dunes = std::max(ss.dunes, sv.duneSong); ss.ice = std::max(ss.ice, sv.iceSong); ss.quake = std::max(ss.quake, sv.quake);
        ss.synth.render(buf.data(), (int)buf.size(), 22050, ss.st);
        for (float v : buf) ss.pcm.push_back((int16_t)(std::max(-1.f, std::min(1.f, v)) * 32767));
    }
}
// the first people's world heard from home (the signals' order: the nearest)
bool firstPeopleWorld(PeopleWorld& w, Signal* sigOut) {
    Vec3 obs((HOME_SX + 0.5) * SECTOR_KM, (HOME_SY + 0.5) * SECTOR_KM, (HOME_SZ + 0.5) * SECTOR_KM);
    std::vector<Signal> far; signalsNear(obs, far);
    for (const Signal& sg : far) {
        if (sg.kind != SIG_PEOPLE) continue;
        Star named = sg.star; starInSector(sg.star.sx, sg.star.sy, sg.star.sz, named, true);
        if (peopleWorldOf(named, sg.body, w)) { if (sigOut) *sigOut = sg; return true; }
    }
    return false;
}

// C-07: `signals [sx sy sz] [reach]`: the far signals heard from a sector (home by default): each with its kind, its distance (the
// age), its direction (galactic longitude and latitude from the observer), its star's class and sector, the world and the
// recording on the air at the game's first hour; then the system's own signals there, if the sector holds a star
// the gain's six steps from a sector: the peoples' signals and the pulsars heard at each, what the step adds (with the distances
// of the peoples it adds), the nearest and the farthest, and the scan's cost; `full` one line a step, else one line for the place
static void printGainSteps(int64_t sx, int64_t sy, int64_t sz, const std::string& name, bool full) {
    Vec3 obs((sx + 0.5) * SECTOR_KM, (sy + 0.5) * SECTOR_KM, (sz + 0.5) * SECTOR_KM);
    std::vector<Signal> prev; std::string line = fmt("  %-29s", name.c_str()); double msMax = 0;
    for (int st = 0; st < SIGNAL_GAIN_STEPS; st++) {
        std::vector<Signal> out; double t0 = nowSec(); signalsNear(obs, out, SIGNAL_REACH_LY, SIGNAL_GAINS[st]); double ms = (nowSec() - t0) * 1e3; msMax = std::max(msMax, ms);
        int people = 0, pulsars = 0, added = 0, addedP = 0; double nearest = 1e18, farthest = 0, farPulsar = 0; std::string adds;
        for (const Signal& s : out) {
            bool had = false; for (const Signal& o : prev) if (o.star.seed == s.star.seed) had = true;
            if (s.kind == SIG_PEOPLE) {
                people++;
                nearest = std::min(nearest, s.distLy);
                farthest = std::max(farthest, s.distLy);
                if (!had && st > 0) {
                    added++;
                    adds += (adds.empty() ? "" : ", ") + thousands(s.distLy);
                }
            } else if (s.kind == SIG_PULSAR) {
                pulsars++;
                farPulsar = std::max(farPulsar, s.distLy);
                if (!had && st > 0) addedP++;
            }
        }
        if (full) printf("    x%-2d %2d peoples%s, the nearest %s ly, the farthest %s ly; %d pulsars%s, the farthest %s ly; %.1f ms\n", SIGNAL_GAINS[st], people,
                         st > 0 ? fmt(" (+%d%s%s)", added, adds.empty() ? "" : ": ", adds.c_str()).c_str() : "", people ? thousands(nearest).c_str() : "-", people ? thousands(farthest).c_str() : "-",
                         pulsars, st > 0 && addedP ? fmt(" (+%d)", addedP).c_str() : "", thousands(farPulsar).c_str(), ms);
        else line += st == 0 ? fmt(" x1 %d", people) : fmt("  x%d +%d", SIGNAL_GAINS[st], added);
        if (!full && st == SIGNAL_GAIN_STEPS - 1) line += fmt("  = %d, the farthest %s ly; pulsars %d; %.0f ms", people, thousands(farthest).c_str(), pulsars, msMax);
        prev = out;
    }
    if (!full) printf("%s\n", line.c_str());
}

int runSignals(int argc, char** argv) {
    int64_t sx = HOME_SX, sy = HOME_SY, sz = HOME_SZ; double reach = SIGNAL_REACH_LY, gain = 1; bool wav = false;
    std::vector<std::string> rest;   // C-15: `x32` lists what that gain hears; `wav` renders the receiver
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "wav") wav = true;
        else if (a.size() > 1 && a[0] == 'x' && isdigit((unsigned char)a[1])) gain = atof(a.c_str() + 1);
        else rest.push_back(a);
    }
    if (rest.size() >= 3) { sx = atoll(rest[0].c_str()); sy = atoll(rest[1].c_str()); sz = atoll(rest[2].c_str()); }
    if (rest.size() >= 4) reach = atof(rest[3].c_str());
    Vec3 obs((sx + 0.5) * SECTOR_KM, (sy + 0.5) * SECTOR_KM, (sz + 0.5) * SECTOR_KM);
    std::vector<Signal> far; double t0 = nowSec(); int cells = signalsNear(obs, far, reach, gain); double ms = (nowSec() - t0) * 1e3;
    printf("signals within %.0f ly of sector %lld %lld %lld%s: %zu (%d cells scanned in %.1f ms)\n", reach, (long long)sx, (long long)sy, (long long)sz,
            gain > 1 ? fmt(", and beyond as far as the gain x%.0f carries each", gain).c_str() : "", far.size(), cells, ms);
    for (const Signal& s : far) {
        Vec3 d = normalize(s.pos - obs);
        double lon = std::atan2(d.z, d.x) / DEG, lat = std::asin(clampd(d.y, -1, 1)) / DEG;
        std::string what;
        if (s.kind == SIG_PEOPLE) {
            Star named = s.star; starInSector(s.star.sx, s.star.sy, s.star.sz, named, true);   // the scan leaves the names out; the print wants them
            StarSystem sys; sys.generate(named);
            const Body& b = sys.bodies[s.body];
            BodyGen g = BodyGen::make(b); Lore L = loreOf(sys, b, g);
            int idx = transmittedShard(s.seed, 3.6e6);
            Shard sh = shardOf(sys, b, g, L, idx);
            what = fmt("%s (body %d, the %s): on the air S%d, %s", b.name.c_str(), s.body, L.people.c_str(), idx,
                    sh.music ? "a piece of music" : fmt("a voice: \"%s\"", trunc(sh.text, 60).c_str()).c_str());
        } else if (s.kind == SIG_ROGUE) what = signalSourceLine(s);
        else what = fmt("%.2f pulses a second", s.pulseHz);
        int step = 0; while (step < SIGNAL_GAIN_STEPS - 1 && SIGNAL_GAINS[step] < s.needGain - 1e-9) step++;   // the least step that hears it
        double az = wrap2pi(std::atan2(d.x, d.z)) / DEG, el = std::asin(clampd(d.y, -1, 1)) / DEG;   // where the radar camera points for it (its AZ and EL)
        if (s.kind == SIG_PEOPLE && s.age >= 0) what = fmt("the %s age; ", AGE_NAMES[s.age]) + what;
        printf("  %-24s %6.1f ly  lon %6.1f lat %5.1f  AZ %03.0f EL %+03.0f  x%-2d power %6.3f  strength %.2f  %s %s at %lld %lld %lld: %s\n", SIGNAL_KIND_NAMES[s.kind], s.distLy, lon, lat, az, el,
                SIGNAL_GAINS[step], s.power, signalStrengthAtGain(s, gain), STAR_CLASSES[s.star.cls].code, STAR_CLASSES[s.star.cls].name, (long long)s.star.sx, (long long)s.star.sy,
                (long long)s.star.sz, what.c_str());
    }
    if (rest.size() < 4 && !wav) {   // the gain's steps from this sector, and from home the test worlds' too
        printf("the gain's steps from sector %lld %lld %lld:\n", (long long)sx, (long long)sy, (long long)sz);
        printGainSteps(sx, sy, sz, "here", true);
        if (sx == HOME_SX && sy == HOME_SY && sz == HOME_SZ) {
            printf("  the test worlds (x1 heard, then what each step adds):\n");
            const struct { int64_t x, z; const char* name; } C[3] = {{151, 25, "Aieliaalas II"}, {153, 70, "Leileashphail III"}, {152, 92, "Eleinewai I"}};
            for (const auto& c : C) printGainSteps(c.x, 0, c.z, fmt("%s (%lld 0 %lld)", c.name, (long long)c.x, (long long)c.z), false);
            for (int a = 0; a < AGE_COUNT; a++)
                printGainSteps(AGE_WORLDS[a].sx, 0, AGE_WORLDS[a].sz, fmt("the %s (%lld 0 %lld)", AGE_NAMES[a], (long long)AGE_WORLDS[a].sx, (long long)AGE_WORLDS[a].sz), false);
        }
    }
    if (wav) {   // the receiver's sounds for the ear: the bed with its events, three of each natural kind, every programme of the first people's world
        makeDir("shots"); makeDir("shots/tests");
        static const char* const NAMES[SIG_KIND_COUNT] = {"people", "pulsar", "comet", "magnetosphere", "drifter"};
        static const char* const PNAMES[RP_COUNT] = {
                "voice", "whispers", "chant", "numbers", "loop", "beacon", "data", "bell", "siren", "murmur", "music", "morse", "phonograph", "synthetic", "maths"};
        static const double RATES[3] = {0.7, 1.7, 3.2};
        {
            std::vector<int16_t> pcm;
            RenderStats r = renderKind(-1, 0, 1, 30.0, &pcm);
            std::string fn = "shots/tests/radar_bed.wav";
            writeWav16(fn, pcm, 22050);
            printf("  bed: 30 s rendered (something in the static every twelve to forty seconds), peak %.3f, rms %.3f -> %s\n", r.peak, r.rms, fn.c_str());
        }
        for (int kind = SIG_PULSAR; kind < SIG_KIND_COUNT; kind++)
            for (int k = 0; k < 3; k++) {
                std::vector<int16_t> pcm; RenderStats r = renderKind(kind, 0x51 + (uint64_t)k * 0x1F3, RATES[k], 12.0, &pcm);
                std::string fn = fmt("shots/tests/radar_%s_%d.wav", NAMES[kind], k + 1); writeWav16(fn, pcm, 22050);
                printf("  %s %d: 12 s rendered, peak %.3f, rms %.3f -> %s\n", NAMES[kind], k + 1, r.peak, r.rms, fn.c_str());
            }
        {   // a pulsar overdriven
            RxOpts od; od.drive = 1.0; std::vector<int16_t> pcm; RenderStats r = renderKind(SIG_PULSAR, 0x51, RATES[1], 12.0, &pcm, od);
            writeWav16("shots/tests/radar_overdriven_pulsar.wav", pcm, 22050);
            printf("  pulsar overdriven 1.0: 12 s, peak %.3f, rms %.3f -> shots/tests/radar_overdriven_pulsar.wav\n", r.peak, r.rms);
        }
        PeopleWorld w;
        if (firstPeopleWorld(w)) {
            printf("  the programmes of %s (%s):\n", w.name.c_str(), w.lore.people.c_str());
            for (int p = 0; p < RP_COUNT; p++) {
                std::vector<int16_t> pcm; Programme prog; RenderStats r = renderProgramme(w, p, 12.0, &pcm, &prog);
                std::string fn = fmt("shots/tests/radar_people_%s.wav", PNAMES[p]); writeWav16(fn, pcm, 22050);
                std::string what = prog.machine
                        ? fmt("%.0f s on the air", prog.seconds)
                        : (prog.kind == RP_MUSIC
                                          ? fmt("the piece at %.2f of its speed", prog.pieceSpeed)
                                          : fmt("\"%s\"%s", trunc(decodedText(prog.words), 40).c_str(), prog.repeats > 1 ? fmt(", %d times with %.1f s between", prog.repeats, prog.gap).c_str() : ""));
                printf("    %-18s 12 s, peak %.3f, rms %.3f: %s -> %s\n", RADIO_PROGRAMME_NAMES[p], r.peak, r.rms, what.c_str(), fn.c_str());
            }
            for (int p : {(int)RP_VOICE, (int)RP_BEACON, (int)RP_MUSIC})   // a near signal through a high gain, the receiver overdriven
                for (double drive : {0.5, 1.0}) {
                    RxOpts od; od.drive = drive; std::vector<int16_t> pcm; RenderStats r = renderProgramme(w, p, 12.0, &pcm, nullptr, od);
                    std::string fn = fmt("shots/tests/radar_overdriven_%s_%.0f.wav", PNAMES[p], drive * 100); writeWav16(fn, pcm, 22050);
                    printf("    %-18s overdriven %.1f: 12 s, peak %.3f, rms %.3f -> %s\n", RADIO_PROGRAMME_NAMES[p], drive, r.peak, r.rms, fn.c_str());
                }
            int counts[RP_COUNT] = {0};
            for (int i = 0; i < SHARDS_PER_WORLD; i++) counts[programmeOf(w.seed, i, w.shards[i].music)]++;
            printf("  its broadcast over the fifty slots:"); for (int p = 0; p < RP_COUNT; p++) if (counts[p]) printf(" %s %d;", RADIO_PROGRAMME_NAMES[p], counts[p]); printf("\n");
        } else printf("  no people's world within reach of home: no programmes rendered\n");
        // what the gain lets in, for the ear and for the levels (C-07's lesson: its first bed clipped at an rms of 0.25): the bed at
        // three gains (toward the plane; at x32 toward a pole and the core too), a near voice at three gains and a far one worn at three,
        // two signals mixed, the top step's static long enough for its stranger events, and each age's programmes from its test world
        printf("  C-16, what the gain lets in (the bed toward the band's plane, sky 0.4, unless said):\n");
        auto wavOut = [&](const std::string& name, const RenderStats& r, const std::vector<int16_t>& pcm, const std::string& what) {
            std::string fn = "shots/tests/radar_" + name + ".wav"; writeWav16(fn, pcm, 22050);
            printf("    %-26s peak %.3f, rms %.3f%s%s -> %s\n", name.c_str(), r.peak, r.rms, r.clip ? fmt(", %d CLIPPED", r.clip).c_str() : "", what.c_str(), fn.c_str());
        };
        for (int g : {1, 11, 32}) {
            RxOpts o; o.gainT = gainShareOf(g); o.sky = 0.4; int ev = 0;
            std::vector<int16_t> pcm; RenderStats r = renderKind(-1, 0, 1, 30.0, &pcm, o, &ev);
            wavOut(fmt("bed_x%d", g), r, pcm, fmt(", 30 s, %d events", ev));
        }
        for (double sky : {0.05, 0.9}) {
            RxOpts o; o.gainT = 1; o.sky = sky; int ev = 0;
            std::vector<int16_t> pcm; RenderStats r = renderKind(-1, 0, 1, 30.0, &pcm, o, &ev);
            wavOut(sky < 0.5 ? "bed_x32_pole" : "bed_x32_core", r, pcm, fmt(", 30 s, sky %.2f, %d events", sky, ev));
        }
        {
            RxOpts o; o.gainT = 1; o.sky = 0.4; int ev = 0, kinds = 0;
            std::vector<int16_t> pcm; RenderStats r = renderKind(-1, 0, 1, 120.0, &pcm, o, &ev, &kinds);
            static const char* const EVN[12] = {"a carrier", "a boom", "a moan", "knocks", "a murmur", "a breath", "a scream", "speech backwards", "counting whispered", "a breath at the microphone",
                    "a knock answered", "a choir stopping"};
            std::string heard; for (int k = 0; k < 12; k++) if (kinds & (1 << k)) heard += (heard.empty() ? "" : ", ") + std::string(EVN[k]);
            wavOut("static_x32", r, pcm, fmt(", 120 s, %d events: %s", ev, heard.c_str()));
        }
        if (w.valid) {
            for (int g : {1, 11, 32}) {
                RxOpts o;
                o.gainT = gainShareOf(g);
                o.sky = 0.4;
                o.age = AGE_MODERN;
                std::vector<int16_t> pcm;
                RenderStats r = renderProgramme(w, RP_VOICE, 12.0, &pcm, nullptr, o);
                wavOut(fmt("near_voice_x%d", g), r, pcm, ", a voice within 200 ly");
            }
            for (int g : {11, 25, 32}) {
                RxOpts o;
                o.gainT = gainShareOf(g);
                o.sky = 0.4;
                o.age = AGE_MODERN;
                o.wear = 0.9;
                o.clarity = 0.6;
                std::vector<int16_t> pcm;
                RenderStats r = renderProgramme(w, RP_VOICE, 12.0, &pcm, nullptr, o);
                wavOut(fmt("far_voice_x%d", g), r, pcm, ", a voice from about 4,000 ly, locked");
            }
            { RxOpts o; o.gainT = gainShareOf(18); o.sky = 0.4; o.age = AGE_MODERN; o.kind2 = SIG_PULSAR; o.seed2 = 0x51; o.pulse2 = 1.1; o.sig2 = 0.45; o.intermod = 0;
              std::vector<int16_t> pcm; RenderStats r = renderProgramme(w, RP_VOICE, 12.0, &pcm, nullptr, o); wavOut("mixed_voice_pulsar_x18", r, pcm, ", a pulsar's thump under a voice"); }
            for (int g : {11, 32}) {
                RxOpts o;
                o.gainT = gainShareOf(g);
                o.sky = 0.4;
                o.age = AGE_MODERN;
                o.kind2 = SIG_PEOPLE;
                o.voice2 = RP_VOICE;
                o.age2 = AGE_MODERN;
                o.seed2 = 0x77;
                o.sig2 = 0.5;
                o.het = 0.3 + 0.4 * o.gainT;
                o.hetHz = 900;
                o.intermod = std::max(0.0, (o.gainT - 0.55) / 0.45);
                std::vector<int16_t> pcm;
                RenderStats r = renderProgramme(w, RP_VOICE, 12.0, &pcm, nullptr, o);
                wavOut(fmt("mixed_two_x%d", g), r, pcm, ", two broadcasts, one through the other, the whistle between their carriers");
            }
        }
        {   // each age on the air, from the age test worlds (A-01), at x1
            struct AgeRender { int age; int p; bool music; const char* name; };
            const AgeRender AR[] = {{AGE_INDUSTRIAL, RP_MORSE, false, "industrial_morse"}, {AGE_INDUSTRIAL, RP_PHONOGRAPH, false, "industrial_phonograph_voice"},
                    {AGE_INDUSTRIAL, RP_PHONOGRAPH, true, "industrial_phonograph_piece"}, {AGE_FUTURISTIC, RP_SYNTHETIC, false, "futuristic_synthetic"},
                    {AGE_FUTURISTIC, RP_DATA, false, "futuristic_data"}, {AGE_FUTURISTIC, RP_BEACON, false, "futuristic_beacon"}, {AGE_FAR_FUTURE, RP_MATHS, false, "farfuture_maths_1"},
                    {AGE_FAR_FUTURE, RP_MATHS, false, "farfuture_maths_2"}, {AGE_FAR_FUTURE, RP_MATHS, false, "farfuture_maths_3"}};
            int lastAge = -1, mathsN = 0; PeopleWorld aw;
            for (const AgeRender& a : AR) {
                if (a.age != lastAge) {
                    lastAge = a.age; const AgeWorld& W = AGE_WORLDS[a.age]; Star st;
                    if (!starInSector(W.sx, 0, W.sz, st, true) || !peopleWorldOf(st, W.body, aw)) {
                        printf("    the %s world: no people's world at %lld 0 %lld body %d\n", AGE_NAMES[a.age], (long long)W.sx, (long long)W.sz, W.body);
                        aw.valid = false;
                        continue;
                    }
                    int counts[RP_COUNT] = {0}; for (int i = 0; i < SHARDS_PER_WORLD; i++) counts[programmeOf(aw.seed, i, aw.shards[i].music, a.age)]++;
                    std::string mix; for (int p = 0; p < RP_COUNT; p++) if (counts[p]) mix += fmt("%s%s %d", mix.empty() ? "" : "; ", RADIO_PROGRAMME_NAMES[p], counts[p]);
                    printf("    the %s age (%s, the %s): its broadcast over the fifty slots: %s\n", AGE_NAMES[a.age], aw.name.c_str(), aw.lore.people.c_str(), mix.c_str());
                }
                if (!aw.valid) continue;
                RxOpts o; o.age = a.age; o.music = a.music;
                if (a.p == RP_MATHS) { int k = 0; for (int i = 0; i < SHARDS_PER_WORLD; i++) if (!aw.shards[i].music && k++ == mathsN * 7) { o.slot = i; break; } mathsN++; }
                Programme prog; std::vector<int16_t> pcm; RenderStats r = renderProgramme(aw, a.p, a.p == RP_MORSE || a.p == RP_MATHS ? 30.0 : 15.0, &pcm, &prog, o);
                std::string what = a.p == RP_MORSE ? fmt(", %zu elements at %.0f ms a unit, %.0f s keyed", prog.code.size(), prog.unit * 1000, prog.seconds)
                                                   : (a.p == RP_MATHS ? fmt(", %s", AudioSynth::mathsName(prog.seed)) : "");
                wavOut(a.name, r, pcm, what);
            }
        }
    }
    Star here; if (starInSector(sx, sy, sz, here, true)) {
        StarSystem sys; sys.generate(here);
        std::vector<Signal> local; localSignals(sys, obs, 3.6e6, local);
        printf("the system of %s (%s) here: %zu local signals%s\n", here.name.c_str(), STAR_CLASSES[here.cls].name, local.size(), sectorTransmits(sx, sy, sz) ? " (a transmitter sector)" : "");
        for (const Signal& s : local)
            printf("  %-24s %s (body %d, %s)  strength %.2f  %s\n", SIGNAL_KIND_NAMES[s.kind], sys.bodies[s.body].name.c_str(), s.body, KINDS[s.bodyType].name, s.strength,
                    signalSourceLine(s).c_str());
    } else printf("no star in that sector\n");
    return 0;
}

int runVoice(int argc, char** argv) {
    int which = takePeopleArg(argc, argv);   // C-13: `second` for the world's second people
    if (argc < 5) { printf("voice <sx> <sz> <body index> [index] [wav]\n"); return 1; }
    int64_t sx = atoll(argv[2]), sz = atoll(argv[3]); int bi = atoi(argv[4]);
    int want = argc > 5 && std::string(argv[5]) != "wav" ? atoi(argv[5]) : -1;
    bool wav = (argc > 5 && std::string(argv[5]) == "wav") || (argc > 6 && std::string(argv[6]) == "wav");
    Star s; StarSystem sys; if (!systemAt(sx, 0, sz, s, sys, true)) { printf("no star at %lld 0 %lld\n", (long long)sx, (long long)sz); return 1; }
    if (bi < 0 || bi >= (int)sys.bodies.size()) { printf("no body %d of %zu\n", bi, sys.bodies.size()); return 1; }
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    if (!g.hasTrait(TR_CIVILISATION)) printf("(%s had no people: the voice below is what it would have had)\n", b.name.c_str());
    Lore L = loreOf(sys, b, g, which);
    Tongue T = tongueOf(g, L);
    Voice V = voiceOf(g, L, T);
    static const char* const STRESS[4] = {"the first syllable", "the last syllable", "the last syllable but one", "no syllable (all even)"};
    static const char* const CONTOUR[3] = {"falling through the sentence", "rising to a peak then falling", "level with a drop at the end"};
    printf("%s, %s of %s: the voice of the %s (names of style %d)\n", b.name.c_str(), KINDS[b.type].name, s.name.c_str(), L.people.c_str(), L.style);
    printf("  %s: %.0f Hz with a range of %.2f, %.1f syllables a second, the tract %.2f; breath %.2f, roughness %.2f, nasal %.2f, tremor %.2f, a second tone %.2f at %.2f; the stress on %s, the "
           "melody %s%s%s%s\n",
            voiceLine(V).c_str(), V.pitch, V.range, V.rate, V.tract, V.breath, V.rough, V.nasal, V.tremor, V.sub, V.subRatio, STRESS[V.stress], CONTOUR[V.contour],
            V.tonal ? ", a tone on each syllable" : "", V.clicky ? ", clicks for the voiceless stops" : "", V.trill ? ", the r's rolled" : "");
    printf("  sounds (%zu):", V.inventory.size());
    static const char* const KINDS[PK_COUNT] = {"vowel", "stop", "fricative", "nasal", "liquid", "glide", "pause"};
    for (int p : V.inventory) printf(" %s(%s%s)", PHONES[p].sym, PHONES[p].kind == PK_VOWEL ? "" : (PHONES[p].voiced ? "voiced " : "voiceless "), KINDS[PHONES[p].kind]);
    printf("\n");
    std::vector<Shard> fifty; shardsOf(sys, b, g, L, SHARDS_PER_WORLD, fifty);
    Language lang; languageOf(L, fifty, T, lang);
    int idx = want;
    if (idx < 0) for (int i = 0; i < SHARDS_PER_WORLD && idx < 0; i++) if (!fifty[i].music) idx = i;
    if (idx < 0 || idx >= SHARDS_PER_WORLD || fifty[idx].music) { printf("shard %d is not a text\n", idx); return 1; }
    std::vector<DecodedWord> w; decodeShard(fifty[idx], lang, T, 10, w);
    Speech S; speechOf(V, w, fifty[idx].seed, S);
    printf("shard %d, year %d, %s: %zu words, %d syllables, %.1f s, %zu segments\n", idx, fifty[idx].year, SHARD_TONE_NAMES[fifty[idx].tone], w.size(), S.syllables, S.seconds, S.segs.size());
    printf("  %s\n", fifty[idx].text.c_str());
    for (size_t k = 0; k < w.size(); k++) {
        std::string spoken = w[k].name ? w[k].ours : w[k].theirs;
        printf("  %5.2f-%5.2f  %-14s %-12s %s%s\n", S.wordStart[k], S.wordEnd[k], (w[k].ours + w[k].post).c_str(), spoken.c_str(), S.phones[k].c_str(), w[k].name ? "  (a name)" : "");
    }
    if (wav) {
        AudioSynth synth; AudioState st; st.speech = &S; st.voice = &V; st.speechStart = true;
        const int sr = 22050; std::vector<float> buf(2048); std::vector<int16_t> pcm; int nan = 0, clip = 0; double sum = 0; float peak = 0;
        int frames = 0, total = (int)(sr * (S.seconds + 1));
        while (frames < total) {
            synth.render(buf.data(), (int)buf.size(), sr, st);
            for (float v : buf) {
                if (!std::isfinite(v)) nan++;
                if (std::fabs(v) >= 0.999f) clip++;
                peak = std::max(peak, std::fabs(v));
                sum += (double)v * v;
                pcm.push_back((int16_t)(std::max(-1.f, std::min(1.f, v)) * 32000));
            }
            frames += (int)buf.size();
        }
        makeDir("shots"); makeDir("shots/tests");
        std::string fn = fmt("shots/tests/voice_%lld_%lld_%d_S%d.wav", (long long)sx, (long long)sz, bi, idx);
        FILE* f = fopen(fn.c_str(), "wb");
        if (f) {
            uint32_t dataBytes = (uint32_t)pcm.size() * 2, rate = sr, byteRate = sr * 2; uint16_t ch = 1, bits = 16, blockAlign = 2, fmtTag = 1; uint32_t fmtLen = 16, riffLen = 36 + dataBytes;
            fwrite("RIFF", 1, 4, f);
            fwrite(&riffLen, 4, 1, f);
            fwrite("WAVEfmt ", 1, 8, f);
            fwrite(&fmtLen, 4, 1, f);
            fwrite(&fmtTag, 2, 1, f);
            fwrite(&ch, 2, 1, f);
            fwrite(&rate, 4, 1, f);
            fwrite(&byteRate, 4, 1, f);
            fwrite(&blockAlign, 2, 1, f);
            fwrite(&bits, 2, 1, f);
            fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f); fwrite(pcm.data(), 2, pcm.size(), f); fclose(f);
        }
        printf("rendered %.1f s: peak %.3f, rms %.3f, %d non-finite, %d at the ceiling, %s -> %s\n", (double)pcm.size() / sr, peak, std::sqrt(sum / std::max<size_t>(1, pcm.size())), nan, clip,
                st.speechDone ? "the speech ended" : "the speech NOT ended", fn.c_str());
    }
    return 0;
}

int runSounds(int argc, char** argv) {
    printf("the sounds of the ground (Q-03), 22,050 Hz mono through the synth at the master's 0.8:\n");
    for (const GroundSound& gs : groundSounds()) {
        std::vector<int16_t> pcm; int ev = 0;
        RenderStats r = renderGroundSound(gs, &pcm, &ev);
        const std::string fn = fmt("shots/tests/sound_%s.wav", gs.name);
        writeWav16(fn, pcm, 22050);
        std::string extra;
        if (std::string(gs.name).rfind("dunes", 0) == 0) {
            std::vector<float> x(pcm.size()); for (size_t i = 0; i < pcm.size(); i++) x[i] = pcm[i] / 32767.0f;
            AudioState a; gs.set(a, 2.0);
            extra = fmt(", its note %.0f Hz, the loudest %.0f Hz", a.duneHz, spectralPeak(x, 1.5, 20, 400));
        }
        if (std::string(gs.name).rfind("ice", 0) == 0) extra = fmt(", %d sounds of the ice", ev);
        printf("  %-15s %4.0f s, peak %.3f, rms %.3f%s%s: %s -> %s\n", gs.name, gs.seconds, r.peak, r.rms, r.clip ? fmt(", %d CLIPPED", r.clip).c_str() : "", extra.c_str(), gs.what, fn.c_str());
    }
    {   // beside them, the ground's other sounds at their usual levels
        struct Ref { const char* name; std::function<void(AudioState&)> set; };
        const Ref R[] = {{"the wind at 0.5", [](AudioState& a) { a.wind = 0.5; a.windTone = 0.5; }}, {"the wind at its full", [](AudioState& a) { a.wind = 1.0; a.windTone = 0.5; }},
                         {"waves", [](AudioState& a) { a.surf = 0.6; a.wind = 0.3; a.windTone = 0.25; }}, {"rain", [](AudioState& a) { a.rain = 1.0; a.wind = 0.4; }}};
        printf("  beside them:");
        for (const Ref& rf : R) { AudioSynth s; AudioState a; a.master = 0.8; rf.set(a); RenderStats r = renderReceiver(s, a, 6.0, nullptr); printf(" %s rms %.3f;", rf.name, r.rms); }
        printf("\n");
    }
    return 0;
}
