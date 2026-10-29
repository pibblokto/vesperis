// E-03: the unit's signals (group `signals`)
#include "harness/harness.h"
#include "harness/test_worlds.h"

// the signals, the gain, the ages on the air
void unitSignals(const Check& check) {
    {   // the signals are a property of the galaxy: the same twice from home, every people's signal's star has a people's world in a
        // transmitter sector, every pulsar signal's star a pulsar, all within reach and sorted by distance, rare (one to a dozen within
        // 200 ly on the arm); the beam's gain falls off its axis; the broadcast's chain never repeats a recording twice running
        Vec3 home((HOME_SX + 0.5) * SECTOR_KM, (HOME_SY + 0.5) * SECTOR_KM, (HOME_SZ + 0.5) * SECTOR_KM);
        std::vector<Signal> a, b; double t0 = nowSec(); int cells = signalsNear(home, a); double ms = (nowSec() - t0) * 1e3; signalsNear(home, b);
        bool same = a.size() == b.size(); int people = 0, pulsars = 0, bad = 0; double maxLy = 0;
        for (size_t i = 0; i < a.size(); i++) {
            const Signal& s = a[i];
            if (same && (s.seed != b[i].seed || s.kind != b[i].kind || s.distLy != b[i].distLy)) same = false;
            if (i > 0 && s.distLy < a[i - 1].distLy) bad++;
            maxLy = std::max(maxLy, s.distLy);
            if (s.kind == SIG_PEOPLE) {
                people++;
                Signal again; bool has = starSignal(s.star, again);
                if (!has || again.kind != SIG_PEOPLE || again.body != s.body || !sectorTransmits(s.star.sx, s.star.sy, s.star.sz) || s.distLy > SIGNAL_REACH_LY) bad++;
                if (!kindHas(s.bodyType, KT_PEOPLE)) bad++;   // a scorched world's people too
            } else if (s.kind == SIG_PULSAR) { pulsars++; if (s.star.cls != STAR_PULSAR || s.pulseHz <= 0 || s.distLy > SIGNAL_REACH_LY) bad++; }
            else bad++;
            if (s.strength <= 0 || s.strength > 1) bad++;
        }
        bool gainOk = beamGain(0) > 0.99 && beamGain(2 * DEG) > beamGain(5 * DEG) && beamGain(5 * DEG) > beamGain(15 * DEG) && beamGain(15 * DEG) > beamGain(40 * DEG) && beamGain(90 * DEG) < 0.03;
        int chainBad = 0;
        {
            int prev = transmittedShard(0x1234, 3.6e6);
            if (prev < 0 || prev >= 50) chainBad++;
            for (int k = 0; k < 200; k++) {
                int nx = nextTransmittedShard(0x1234, prev);
                if (nx == prev || nx < 0 || nx >= 50) chainBad++;
                prev = nx;
            }
        }
        std::string first = a.empty() ? "none"
                                      : fmt("the nearest %s at %.0f ly (sector %lld %lld %lld, strength %.2f)", SIGNAL_KIND_NAMES[a[0].kind], a[0].distLy, (long long)a[0].star.sx,
                                                (long long)a[0].star.sy, (long long)a[0].star.sz, a[0].strength);
        check("signals: hashed, rare and far (C-07)", same && bad == 0 && people >= 1 && people <= 12 && gainOk && chainBad == 0,
                fmt("%zu signals from home (%d of a people, %d pulsars) over %d cells in %.1f ms, %s, the farthest %.0f ly, %d wrong, the chain %d wrong", a.size(), people, pulsars, cells, ms,
                        first.c_str(), maxLy, bad, chainBad));
    }
    {   // the radar in the game: B switches it on at home, the view turned onto the first people's signal hears its recording through
        // the static (a piece or a speech on the synth, the radar's), the hold locks within eight seconds, Enter sets the remote target on
        // the signal's star (the local target when it is of this system), the view turned forty degrees away loses the lock, B off releases the synth
        TestGame game;
        game.savePrefix = "shots/tests/test_save";
        game.settingsPath = "shots/tests/test_settings.txt";
        game.keysPath = "shots/tests/test_keys.txt";
        game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFrames(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        run(0.2); press(KEY_B); run(0.2);
        bool on = game.testRadarOn(); int n = game.testRadarSignals();
        int who = -1; for (int i = 0; i < n && who < 0; i++) if (game.testRadarKind(i) == SIG_PEOPLE) who = i;
        bool aimed = who >= 0 && game.testAimAtSignal(who);
        run(1.0);
        // a programme on the air: a recording, or one of the world's machines
        auto onAir = [&]() { return game.audio.radio && (game.audio.piece || game.audio.speech || game.testRadarProgramme() >= RP_BEACON); };
        double heardAt = 1.0;   // the world's records are written on a thread the first time (its history and plan: a few hundred milliseconds), the carrier alone till then
        for (double t0 = nowSec(); !onAir() && nowSec() - t0 < 8; heardAt += 0.1) run(0.1);
        bool heard = onAir();
        int programme = game.testRadarProgramme();
        std::string early = game.testRadarInfo(); (void)early;   // the detail stays short: `fmt` has a bound
        double lockAt = -1; for (int i = 0; i < 30 * 8 && lockAt < 0; i++) { game.frame(gi, 1.0 / 30); gi.newFrame(); if (game.testRadarLocked() >= 0) lockAt = (i + 1) / 30.0; }
        bool locked = game.testRadarLocked() == who;
        std::string atLock = game.testRadarInfo(); (void)atLock;
        gi.down[KEY_RIGHT] = true; run(0.6); gi.down[KEY_RIGHT] = false; run(2.0);   // 0.6 s of the arrows: 41 degrees
        bool lost = game.testRadarLocked() < 0;
        bool again = game.testAimAtSignal(who); run(8.0);
        bool relocked = again && game.testRadarLocked() == who;
        Star want; std::string wantName; bool local = game.testRadarLocal(who);
        {   // the signal's star by its sector, for the arrival below
            int64_t sx = 0, sy = 0, sz = 0;
            if (game.testRadarSector(who, sx, sy, sz) && starInSector(sx, sy, sz, want, true)) wantName = want.name;
        }
        press(KEY_ENTER); run(0.1);
        std::string st = game.testStatus();
        bool flying = who >= 0 && (local ? st.rfind("FINE APPROACH", 0) == 0 : st.rfind("VIMANA FLIGHT", 0) == 0);
        bool released = !game.testRadarOn() && !game.audio.radio && !game.audio.piece && !game.audio.speech && game.audio.radar == 0;
        double flewFor = 0; bool arrived = false;
        if (!local) {
            for (int i = 0; i < 30 * 40 && !arrived; i++) {
                game.frame(gi, 1.0 / 30);
                gi.newFrame();
                flewFor = (i + 1) / 30.0;
                if (game.testStarGenName() == wantName && !wantName.empty()) arrived = true;
            }
        }
        bool people = arrived && game.testCivilisedHere();
        check("radar: the sweep hears, locks, flies (C-07)", on && n > 0 && aimed && heard && lockAt > 0 && locked && lost && relocked && flying && released && (local || (arrived && people)),
                fmt("%s, %d signals, the people's %d aimed %s; heard %s %.1f s in (%s); locked after %.1f s; lost after the turn %s, locked again %s; Enter: '%s' (%s), the camera %s; %s",
                        on ? "on" : "NOT on", n, who, aimed ? "yes" : "no", heard ? "yes" : "NO", heardAt, programme >= 0 && programme < RP_COUNT ? RADIO_PROGRAMME_NAMES[programme] : "nothing",
                        lockAt, lost ? "yes" : "NO", relocked ? "yes" : "NO", st.c_str(), flying ? "flying" : "NOT flying", released ? "left" : "STILL on",
                        local ? "a signal of this system"
                              : fmt("arrived at %s after %.0f s: %s", arrived ? wantName.c_str() : "NOWHERE", flewFor, people ? "a people's world there" : "NO people's world").c_str()));
        remove("shots/tests/test_guide.txt");
    }
    {   // the receiver through the synth: the bed alone and each kind of signal at full clarity, three seconds each, finite, under the ceiling, audible; silent once off
        AudioSynth synth; AudioState st; const int sr = 22050; std::vector<float> buf(2048);
        std::string detail; bool ok = true;
        for (int kind = -1; kind < SIG_KIND_COUNT; kind++) {
            st.radar = 1; st.radarSignal = kind < 0 ? 0 : 1; st.radarKind = kind; st.radarPulseHz = 1.7;
            int nan = 0, clip = 0; double sum = 0, peak = 0; long cnt = 0;
            for (int frames = 0; frames < sr * 3; frames += (int)buf.size()) {
                synth.render(buf.data(), (int)buf.size(), sr, st);
                for (float v : buf) { if (!std::isfinite(v)) nan++; if (std::fabs(v) >= 0.999f) clip++; peak = std::max(peak, (double)std::fabs(v)); sum += (double)v * v; cnt++; }
            }
            double rms = std::sqrt(sum / std::max(1L, cnt));
            if (nan || clip > 20 || rms < 0.01 || rms > 0.4) ok = false;
            detail += fmt("%s rms %.3f peak %.2f%s; ", kind < 0 ? "the bed" : SIGNAL_KIND_NAMES[kind], rms, peak, nan ? " NON-FINITE" : (clip > 20 ? " CLIPPING" : ""));
        }
        st.radar = 0; st.radarSignal = 0; st.radarKind = -1;
        double ssum = 0; long scnt = 0;
        for (int k = 0; k < 40; k++) { synth.render(buf.data(), (int)buf.size(), sr, st); if (k >= 30) for (float v : buf) { ssum += (double)v * v; scnt++; } }
        double stopRms = std::sqrt(ssum / std::max(1L, scnt));
        check("radar: the receiver through the synth (C-07)", ok && stopRms < 0.003, detail + fmt("off: %.4f", stopRms));
    }
    {   // (the user's second call, more sounds): the receiver's variety. Three seeds of each natural kind and every programme of the
        // first people's world heard from home, three seconds each, finite, under the ceiling, audible; the world's broadcast has a
        // character (four to six of the eleven programmes over its fifty slots, the voice the commonest, the same twice); the bed's
        // events over forty seconds leave it finite and under the ceiling
        PeopleWorld w; Signal sg; bool have = firstPeopleWorld(w, &sg);
        std::string detail; bool ok = have; int rendered = 0;
        static const double RATES[3] = {0.7, 1.7, 3.2};
        for (int kind = SIG_PULSAR; kind < SIG_KIND_COUNT && ok; kind++)
            for (int k = 0; k < 3; k++) {
                RenderStats r = renderKind(kind, 0x51 + (uint64_t)k * 0x1F3, RATES[k], 3.0, nullptr); rendered++;
                if (r.nan || r.clip > 20 || r.rms < 0.01 || r.rms > 0.4) {
                    ok = false;
                    detail += fmt("%s seed %d rms %.3f peak %.2f%s%s; ", SIGNAL_KIND_NAMES[kind], k, r.rms, r.peak, r.nan ? " NON-FINITE" : "", r.clip > 20 ? " CLIPPING" : "");
                }
            }
        double lo = 1, hi = 0;
        for (int p = 0; p < RP_COUNT && have; p++) {
            RenderStats r = renderProgramme(w, p, 3.0, nullptr); rendered++;
            lo = std::min(lo, r.rms); hi = std::max(hi, r.rms);
            if (r.nan || r.clip > 20 || r.rms < 0.01 || r.rms > 0.4) {
                ok = false;
                detail += fmt("%s rms %.3f peak %.2f%s%s; ", RADIO_PROGRAMME_NAMES[p], r.rms, r.peak, r.nan ? " NON-FINITE" : "", r.clip > 20 ? " CLIPPING" : "");
            }
        }
        {
            RenderStats r = renderKind(-1, 0, 1, 40.0, nullptr);
            rendered++;
            if (r.nan || r.clip > 20 || r.rms < 0.01 || r.rms > 0.4) {
                ok = false;
                detail += fmt("the bed over 40 s rms %.3f peak %.2f; ", r.rms, r.peak);
            }
        }
        int counts[RP_COUNT] = {0}, distinct = 0, most = 0; bool same = true;
        if (have) for (int i = 0; i < SHARDS_PER_WORLD; i++) { int p = programmeOf(w.seed, i, w.shards[i].music); if (p != programmeOf(w.seed, i, w.shards[i].music)) same = false; counts[p]++; }
        for (int p = 0; p < RP_COUNT; p++) { if (counts[p]) distinct++; if (counts[p] > counts[most]) most = p; }
        std::string mix; for (int p = 0; p < RP_COUNT; p++) if (counts[p]) mix += fmt("%s%s %d", mix.empty() ? "" : ", ", RADIO_PROGRAMME_NAMES[p], counts[p]);
        bool character = have && same && distinct >= 4 && distinct <= 6 && most == RP_VOICE;
        check("radar: the receiver's variety (C-07)", ok && character,
                fmt("%s: %d renders%s, the programmes' rms %.3f-%.3f; the broadcast of %s: %s", have ? "ok" : "NO people's world", rendered, detail.empty() ? "" : (" - " + detail).c_str(), lo, hi,
                        have ? w.name.c_str() : "?", mix.c_str()));
    }
    {   // the gain. From home the six steps' lists: x1 is C-07's under A-01's rule (every signal within 200 ly, the loud pulsars
        // within 25, the three peoples A-01 pinned), each list holds the one before, each step adds one to three peoples from home and
        // the three C-series worlds (the pin) and the farthest at x32 is about 6,000 ly; every far signal is heard from its step and not
        // below (within its reach at that gain, beyond it at the step under), its power its sector's; the scan in slices finds what the
        // whole scan finds; the same twice
        Vec3 home((HOME_SX + 0.5) * SECTOR_KM, (HOME_SY + 0.5) * SECTOR_KM, (HOME_SZ + 0.5) * SECTOR_KM);
        std::vector<Signal> L[SIGNAL_GAIN_STEPS]; double ms32 = 0;
        for (int st = 0; st < SIGNAL_GAIN_STEPS; st++) {
            double t0 = nowSec();
            signalsNear(home, L[st], SIGNAL_REACH_LY, SIGNAL_GAINS[st]);
            if (st == SIGNAL_GAIN_STEPS - 1) ms32 = (nowSec() - t0) * 1e3;
        }
        auto has = [](const std::vector<Signal>& v, const Signal& x) { for (const Signal& o : v) if (o.star.seed == x.star.seed && o.kind == x.kind) return true; return false; };
        std::vector<Signal> c07; signalsNear(home, c07);
        bool x1Same = c07.size() == L[0].size(); for (size_t i = 0; x1Same && i < c07.size(); i++) if (c07[i].star.seed != L[0][i].star.seed) x1Same = false;
        int x1Bad = 0, pinned = 0;
        for (const Signal& x : L[0]) {
            if (x.needGain != 1 || x.distLy > SIGNAL_REACH_LY) x1Bad++;
            if (x.kind == SIG_PULSAR && x.power == 0 && x.distLy > SIGNAL_PULSAR_REACH_LY) x1Bad++;
            if (x.kind == SIG_PEOPLE && ((x.star.sx == 113 && x.star.sz == 21) || (x.star.sx == 25 && x.star.sz == -11) || (x.star.sx == 355 && x.star.sz == 155))) pinned++;
        }
        int nestBad = 0, stepBad = 0, far = 0; double far32 = 0; bool addsOk = true; std::string adds;
        const struct { int64_t x, y, z; } places[4] = {{HOME_SX, HOME_SY, HOME_SZ}, {151, 0, 25}, {153, 0, 70}, {152, 0, 92}};
        for (int pl = 0; pl < 4; pl++) {
            Vec3 at((places[pl].x + 0.5) * SECTOR_KM, (places[pl].y + 0.5) * SECTOR_KM, (places[pl].z + 0.5) * SECTOR_KM);
            std::vector<Signal> prev; std::string line;
            for (int st = 0; st < SIGNAL_GAIN_STEPS; st++) {
                std::vector<Signal> cur; if (pl == 0) cur = L[st]; else signalsNear(at, cur, SIGNAL_REACH_LY, SIGNAL_GAINS[st]);
                int added = 0;
                for (const Signal& x : prev) if (!has(cur, x)) nestBad++;
                for (const Signal& x : cur) if (x.kind == SIG_PEOPLE && !has(prev, x)) added++;
                if (st > 0 && (added < 1 || added > 3)) addsOk = false;
                line += st ? fmt(" +%d", added) : fmt("%d", added);
                prev = cur;
            }
            adds += (pl ? ", " : "") + line;
        }
        for (const Signal& x : L[SIGNAL_GAIN_STEPS - 1]) {
            if (x.kind == SIG_PEOPLE) far32 = std::max(far32, x.distLy);
            if (x.needGain <= 1 || x.kind == SIG_ROGUE) continue;   // the drifters by their own law (their own check)
            far++;
            int first = -1; for (int k = 0; k < SIGNAL_GAIN_STEPS && first < 0; k++) if (has(L[k], x)) first = k;
            bool ok = first > 0 && SIGNAL_GAINS[first] >= x.needGain - 1e-9 && SIGNAL_GAINS[first - 1] < x.needGain;
            ok = ok && x.distLy <= signalReachAtGain(x.power, SIGNAL_GAINS[first]) + 1e-6 && x.distLy > signalReachAtGain(x.power, SIGNAL_GAINS[first - 1]);
            double pw = transmitterPower(x.star.sx, x.star.sy, x.star.sz) * (x.kind == SIG_PULSAR ? SIGNAL_PULSAR_POWER : 1.0);
            if (!ok || std::fabs(pw - x.power) > 1e-12 || !sectorTransmits(x.star.sx, x.star.sy, x.star.sz)) stepBad++;
            if (x.kind == SIG_PEOPLE && x.distLy > x.airYears) stepBad++;   // the light from it left after its first broadcast
        }
        int silent = 0, silentHeard = 0;   // the strong population's peoples within reach at x32 whose light left before their radio: none heard
        {
            const int64_t ox = HOME_SX, oy = HOME_SY, oz = HOME_SZ, R = (int64_t)std::ceil(signalReachAtGain(40, SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]));
            auto fdiv = [](int64_t a, int64_t b) { int64_t q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) q--; return q; };
            for (int64_t cx = fdiv(ox - R, SIGNAL_DEEP_CELL); cx <= fdiv(ox + R, SIGNAL_DEEP_CELL); cx++)
                for (int64_t cy = fdiv(oy - R, SIGNAL_DEEP_CELL); cy <= fdiv(oy + R, SIGNAL_DEEP_CELL); cy++)
                    for (int64_t cz = fdiv(oz - R, SIGNAL_DEEP_CELL); cz <= fdiv(oz + R, SIGNAL_DEEP_CELL); cz++) {
                        int64_t sx[SIGNAL_DEEP_MAX], sy[SIGNAL_DEEP_MAX], sz[SIGNAL_DEEP_MAX];
                        double pw[SIGNAL_DEEP_MAX];
                        int nn = deepCell(cx, cy, cz, sx, sy, sz, pw);
                        for (int k = 0; k < nn; k++) {
                            double dx = (double)(sx[k] - ox), dy = (double)(sy[k] - oy), dz = (double)(sz[k] - oz), rr = signalReachAtGain(pw[k], SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]) + 2;
                            if (dx * dx + dy * dy + dz * dz > rr * rr) continue;
                            Star st;
                            if (!starInSector(sx[k], sy[k], sz[k], st, false)) continue;
                            Signal sg;
                            if (!starSignal(st, sg) || sg.kind != SIG_PEOPLE) continue;
                            double d = length(st.pos - home) / SECTOR_KM;
                            if (d <= SIGNAL_REACH_LY || d > signalReachAtGain(transmitterPower(sx[k], sy[k], sz[k]), SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]) || d <= sg.airYears) continue;
                            silent++;
                            if (has(L[SIGNAL_GAIN_STEPS - 1], sg)) silentHeard++;
                        }
                    }
        }
        SignalScan sc; sc.begin(home, SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]); int slices = 1; while (!sc.step(997)) slices++;
        int deepWhole = 0; for (const Signal& x : L[SIGNAL_GAIN_STEPS - 1]) if (x.needGain > 1) deepWhole++;
        bool slicesSame = (int)sc.out.size() == deepWhole; for (const Signal& x : sc.out) if (!has(L[SIGNAL_GAIN_STEPS - 1], x)) slicesSame = false;
        std::vector<Signal> again; signalsNear(home, again, SIGNAL_REACH_LY, SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]);
        bool twice = again.size() == L[SIGNAL_GAIN_STEPS - 1].size();
        for (size_t i = 0; twice && i < again.size(); i++)
            if (again[i].star.seed != L[SIGNAL_GAIN_STEPS - 1][i].star.seed || again[i].needGain != L[SIGNAL_GAIN_STEPS - 1][i].needGain) twice = false;
        check("signals: the gain, x1 to x32 (C-15)",
                x1Same && x1Bad == 0 && pinned == 3 && nestBad == 0 && addsOk && far32 >= 4500 && far32 <= 7000 && far > 0 && stepBad == 0 && silent > 0 && silentHeard == 0 && slicesSame && twice,
                fmt("x1 %s C-07's (%zu signals, %d wrong, the pinned three %d); the peoples each step adds from home and the C-series worlds %s%s; %zu at x32 from home, %d far (%d wrong), the "
                    "farthest people %.0f ly; %d within reach silent (their light left before their radio), %d of them heard; nested %s; the slices %s (%d of 997 cells); twice %s; x32 in %.1f ms",
                        x1Same ? "is" : "is NOT", L[0].size(), x1Bad, pinned, adds.c_str(), addsOk ? "" : " (NOT one to three)", L[SIGNAL_GAIN_STEPS - 1].size(), far, stepBad, far32, silent,
                        silentHeard, nestBad ? "NO" : "yes", slicesSame ? "the same" : "DIFFER", slices, twice ? "the same" : "DIFFERS", ms32));
    }
    {   // the gain in the game. At home B and the dial up five steps: the far scan done within ten frames, the list grown by what x32
        // hears; on the Ghutreig (90 ly) at x32 the receiver overloads (the meter pinned, the hold refused) and at x1 locks within eight
        // seconds, the hold at x32 over twice x1's on the same signal; the farthest people's signal locks at x32 within sixteen seconds,
        // the gain turned down under it loses it, and locked again Enter flies there
        TestGame game;
        game.savePrefix = "shots/tests/test_save";
        game.settingsPath = "shots/tests/test_settings.txt";
        game.keysPath = "shots/tests/test_keys.txt";
        game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto frame = [&]() { game.frame(gi, 1.0 / 30); gi.newFrame(); };
        auto run = [&](double secs) { int n = (int)(secs * 30); for (int i = 0; i < n; i++) frame(); };
        auto press = [&](int key) { gi.pressed[key] = true; gi.down[key] = true; frame(); gi.down[key] = false; };
        auto findPeople = [&](bool farthest) {
            int best = -1;
            double bd = farthest ? -1 : 1e18;
            for (int i = 0; i < game.testRadarSignals(); i++)
                if (game.testRadarKind(i) == SIG_PEOPLE && !game.testRadarLocal(i)) {
                    double d = game.testRadarDist(i);
                    if (farthest ? d > bd : std::fabs(d - 89.8) < 1 && d < bd) {
                        bd = d;
                        best = i;
                    }
                }
            return best;
        };
        run(0.2); press(KEY_B); run(0.2);
        int n1 = game.testRadarSignals();
        for (int i = 0; i < 5; i++) press(KEY_EQUAL);
        int frames = 0; while (!game.testRadarDeepReady() && frames < 30) { frame(); frames++; }
        frame();
        int n32 = game.testRadarSignals(); bool dial = game.testRadarGainStep() == SIGNAL_GAIN_STEPS - 1;
        int gh = findPeople(false); bool aimed = gh >= 0 && game.testAimAtSignal(gh);
        run(3.0);
        bool overload = game.testRadarOverload() && game.testRadarLocked() < 0 && game.testRadarReading() > 0.97;
        double secs32 = game.testRadarLockSecs();
        for (int i = 0; i < 5; i++) press(KEY_MINUS);
        frame(); double secs1 = game.testRadarLockSecs();
        double lockAt = -1; for (int i = 0; i < 30 * 8 && lockAt < 0; i++) { frame(); if (game.testRadarLocked() >= 0) lockAt = (i + 1) / 30.0; }
        bool lockedX1 = lockAt > 0 && !game.testRadarOverload();
        for (int i = 0; i < 5; i++) press(KEY_EQUAL);
        frame();
        int fi = findPeople(true); double farLy = game.testRadarDist(fi), need = game.testRadarNeedGain(fi);
        bool aimedFar = fi >= 0 && game.testAimAtSignal(fi);
        double farAt = -1; for (int i = 0; i < 30 * 16 && farAt < 0; i++) { frame(); if (game.testRadarLocked() >= 0 && game.testRadarDist(game.testRadarLocked()) > 1000) farAt = (i + 1) / 30.0; }
        double farSecs = game.testRadarLockSecs();
        press(KEY_MINUS); frame();
        std::string lost = game.testStatus(); bool sank = game.testRadarLocked() < 0 && lost.find("TOO FAINT") != std::string::npos;
        press(KEY_EQUAL); frame(); fi = findPeople(true); if (fi >= 0) game.testAimAtSignal(fi);
        double again = -1; for (int i = 0; i < 30 * 16 && again < 0; i++) { frame(); if (game.testRadarLocked() >= 0 && game.testRadarDist(game.testRadarLocked()) > 1000) again = (i + 1) / 30.0; }
        press(KEY_ENTER); run(0.1);
        std::string st = game.testStatus(); bool flying = st.rfind("VIMANA FLIGHT", 0) == 0;
        check("radar: the gain in the game (C-15)", dial && n32 > n1 && frames <= 10 && aimed && overload && lockedX1 && secs32 > 2 * secs1 && aimedFar && farAt > 0 && sank && again > 0 && flying,
                fmt("x1 %d signals, x32 %d after %d frames of the far scan; the Ghutreig at x32 %s, the hold %.1f s (x1 %.1f s), at x1 locked after %.1f s; the farthest %.0f ly (heard from x%.1f) "
                    "locked after %.1f s (the hold %.1f s), x25 %s ('%s'), locked again after %.1f s; Enter: '%s'",
                        n1, n32, frames, overload ? "overloads" : "does NOT overload", secs32, secs1, lockAt, farLy, need, farAt, farSecs, sank ? "loses it" : "does NOT lose it", lost.c_str(), again,
                        st.c_str()));
        remove("shots/tests/test_guide.txt");
    }
    {   // what the gain lets in, the receiver. The renders hold C-07's levels at every gain (the bed at x1, x11 and x32 toward a pole,
        // the band's plane and the core; a near voice at three gains and a far one worn at three; two broadcasts mixed with the whistle and
        // the ring; a pulsar under a voice; each age's programmes): finite, never clipped, the rms within C-07's (0.02-0.13); the top
        // step's static has at least twice the events of x1's in the same two minutes, the stranger kinds among them and none at x1
        std::string detail; bool ok = true; int rendered = 0; double lo = 1, hi = 0;
        auto judge = [&](const std::string& what, const RenderStats& r) {
            rendered++; lo = std::min(lo, r.rms); hi = std::max(hi, r.rms);
            if (r.nan || r.clip > 0 || r.peak >= 1.0 || r.rms < 0.02 || r.rms > 0.13) {
                ok = false;
                detail += fmt("%s rms %.3f peak %.2f%s; ", what.c_str(), r.rms, r.peak, r.nan ? " NON-FINITE" : "");
            }
        };
        for (int g : {1, 11, 32})
            for (double sky : {0.05, 0.4, 0.9}) {
                RxOpts o;
                o.gainT = gainShareOf(g);
                o.sky = sky;
                judge(fmt("the bed x%d sky %.2f", g, sky), renderKind(-1, 0, 1, 20.0, nullptr, o));
            }
        PeopleWorld w; bool have = firstPeopleWorld(w);
        if (have) {
            for (int g : {1, 11, 32}) {
                RxOpts o; o.gainT = gainShareOf(g); o.sky = 0.4; o.age = AGE_MODERN; judge(fmt("a near voice x%d", g), renderProgramme(w, RP_VOICE, 6.0, nullptr, nullptr, o));
                o.wear = 0.9; o.clarity = 0.6; judge(fmt("a far voice x%d", g), renderProgramme(w, RP_VOICE, 6.0, nullptr, nullptr, o));
            }
            RxOpts o;
            o.gainT = 1;
            o.sky = 0.4;
            o.age = AGE_MODERN;
            o.kind2 = SIG_PEOPLE;
            o.voice2 = RP_MURMUR;
            o.age2 = AGE_MODERN;
            o.seed2 = 0x77;
            o.sig2 = 0.6;
            o.het = 0.7;
            o.hetHz = 1100;
            o.intermod = 1;
            judge("two broadcasts x32", renderProgramme(w, RP_VOICE, 6.0, nullptr, nullptr, o));
            o.kind2 = SIG_PULSAR; o.seed2 = 0x51; o.pulse2 = 1.3; o.het = 0;
            judge("a pulsar under a voice x32", renderProgramme(w, RP_VOICE, 6.0, nullptr, nullptr, o));
            for (int p : {(int)RP_MORSE, (int)RP_PHONOGRAPH, (int)RP_SYNTHETIC, (int)RP_MATHS}) {
                RxOpts a; a.age = p == RP_MATHS ? AGE_FAR_FUTURE : (p == RP_SYNTHETIC ? AGE_FUTURISTIC : AGE_INDUSTRIAL);
                judge(RADIO_PROGRAMME_NAMES[p], renderProgramme(w, p, 8.0, nullptr, nullptr, a));
            }
            RxOpts a; a.age = AGE_INDUSTRIAL; a.music = true; judge("a phonograph's piece", renderProgramme(w, RP_PHONOGRAPH, 8.0, nullptr, nullptr, a));
        }
        int ev1 = 0, ev32 = 0, k1 = 0, k32 = 0;
        { RxOpts o; o.sky = 0.4; judge("the static x1 over two minutes", renderKind(-1, 0, 1, 120.0, nullptr, o, &ev1, &k1)); }
        { RxOpts o; o.gainT = 1; o.sky = 0.4; judge("the static x32 over two minutes", renderKind(-1, 0, 1, 120.0, nullptr, o, &ev32, &k32)); }
        int strange32 = 0, strange1 = 0; for (int k = 7; k < 12; k++) { if (k32 & (1 << k)) strange32++; if (k1 & (1 << k)) strange1++; }
        bool more = ev32 >= 2 * ev1 && strange32 >= 2 && strange1 == 0;
        check("radar: what the gain lets in, the receiver (C-16)", ok && have && more,
                fmt("%d renders%s, rms %.3f-%.3f; events in two minutes x1 %d, x32 %d (%d of the five stranger kinds there, x1 %d)", rendered, detail.empty() ? "" : (" - " + detail).c_str(), lo, hi,
                        ev1, ev32, strange32, strange1));
    }
    {   // each age on the air. The programmes by age (the age worlds' fifty slots: the industrial Morse or a phonograph, never data;
        // the futuristic a synthetic voice, data, a beacon or the rare piece; the far future mathematics or a synthetic voice; the modern
        // C-07's eleven), every people heard from home at x32 of a radio age, a Morse keying of the words
        int ageBad = 0; std::string ageMix;
        for (int a : {(int)AGE_INDUSTRIAL, (int)AGE_MODERN, (int)AGE_FUTURISTIC, (int)AGE_FAR_FUTURE}) {
            const AgeWorld& W = AGE_WORLDS[a]; Star st; if (!starInSector(W.sx, 0, W.sz, st, true)) { ageBad++; continue; }
            StarSystem sys; sys.generate(st); if (W.body >= (int)sys.bodies.size()) { ageBad++; continue; }
            const Body& b = sys.bodies[W.body]; BodyGen g = BodyGen::make(b);
            int counts[RP_COUNT] = {0};
            for (int i = 0; i < SHARDS_PER_WORLD; i++) {
                int p = programmeOf(b.seed, i, shardIsMusic(g, i), a); counts[p]++;
                bool okp = a == AGE_INDUSTRIAL
                        ? (p == RP_MORSE || p == RP_PHONOGRAPH)
                        : (a == AGE_FUTURISTIC ? (p == RP_SYNTHETIC || p == RP_DATA || p == RP_BEACON || p == RP_MUSIC) : (a == AGE_FAR_FUTURE ? (p == RP_MATHS || p == RP_SYNTHETIC) : p < RP_MORSE));
                if (!okp) ageBad++;
            }
            int distinct = 0; for (int p = 0; p < RP_COUNT; p++) if (counts[p]) distinct++;
            ageMix += fmt("%s%s %d kinds", ageMix.empty() ? "" : ", ", AGE_NAMES[a], distinct);
        }
        int heardAges = 0, heardBad = 0;
        {
            Vec3 home((HOME_SX + 0.5) * SECTOR_KM, (HOME_SY + 0.5) * SECTOR_KM, (HOME_SZ + 0.5) * SECTOR_KM);
            std::vector<Signal> L;
            signalsNear(home, L, SIGNAL_REACH_LY, SIGNAL_GAINS[SIGNAL_GAIN_STEPS - 1]);
            for (const Signal& x : L)
                if (x.kind == SIG_PEOPLE) {
                    if (x.age >= AGE_INDUSTRIAL && x.age < AGE_COUNT) heardAges |= 1 << x.age;
                    else heardBad++;
                }
        }
        int nAges = 0; for (int a = 0; a < AGE_COUNT; a++) if (heardAges & (1 << a)) nAges++;
        bool morseOk = false; std::string morseLine;
        {   PeopleWorld iw; Star st; const AgeWorld& W = AGE_WORLDS[AGE_INDUSTRIAL];
            if (starInSector(W.sx, 0, W.sz, st, true) && peopleWorldOf(st, W.body, iw)) {
                std::vector<DecodedWord> words; decodeShard(iw.shards[iw.firstText], iw.lang, iw.tongue, SHARDS_PER_WORLD, words);
                Programme pr; programmeFor(iw.seed, iw.firstText, false, iw.shards[iw.firstText].year, iw.voice, iw.tongue, words, pr, RP_MORSE, AGE_INDUSTRIAL);
                int on = 0; for (uint8_t e : pr.code) if (!(e & MORSE_OFF)) on++;
                morseOk = pr.speaks && pr.machine && on > 20 && pr.seconds > 10 && pr.seconds < 46;
                morseLine = fmt("%d elements keyed in %.0f s at %.0f ms a unit", on, pr.seconds, pr.unit * 1000);
            }
        }
        check("radar: each age on the air (C-16)", ageBad == 0 && heardBad == 0 && nAges >= 3 && morseOk,
                fmt("the programmes by age %s (%s); the peoples heard from home at x32 of %d radio ages, %d of none; Morse %s (%s)", ageBad ? "WRONG" : "right", ageMix.c_str(), nAges, heardBad,
                        morseOk ? "keyed" : "NOT KEYED", morseLine.c_str()));
    }
    {   // the top step in the game: at home at x32 two signals in the wide lobe both on the air (none under at x1, C-07's receiver) and
        // the synth finite with both; a false rise turned onto fills the hold part way and empties (NOTHING THERE), never a lock; a word of a
        // recording heard comes back out of the static while nothing is on the air, that recording's, never logged
        TestGame game;
        game.savePrefix = "shots/tests/test_save";
        game.settingsPath = "shots/tests/test_settings.txt";
        game.keysPath = "shots/tests/test_keys.txt";
        game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto frame = [&]() { game.frame(gi, 1.0 / 30); gi.newFrame(); };
        auto run = [&](double secs) { int n = (int)(secs * 30); for (int i = 0; i < n; i++) frame(); };
        auto press = [&](int key) { gi.pressed[key] = true; gi.down[key] = true; frame(); gi.down[key] = false; };
        run(0.2); press(KEY_B); run(0.2);
        for (int i = 0; i < 5; i++) press(KEY_EQUAL);
        int frames = 0; while (!game.testRadarDeepReady() && frames < 30) { frame(); frames++; }
        frame();
        // two in the wide lobe: the first signal aimed at with another heard under it
        int pairAt = -1, underAt = -1; bool both = false; double lvl2 = 0;
        for (int i = 0; i < game.testRadarSignals() && pairAt < 0; i++) {
            if (game.testRadarLocal(i)) continue;
            game.testAimAtSignal(i); run(0.4);
            if (game.testRadarUnder() >= 0 && game.audio.radarKind >= 0 && game.audio.radarKind2 >= 0 && game.audio.radarSignal2 > 0.02) {
                pairAt = i;
                underAt = game.testRadarUnder();
                both = true;
                lvl2 = game.audio.radarSignal2;
            }
        }
        RenderStats mixR; { AudioSynth syn; AudioState as = game.audio; as.master = 0.8; mixR = renderReceiver(syn, as, 3.0, nullptr); }
        bool mixOk = both && !mixR.nan && mixR.clip == 0 && mixR.rms > 0.01 && mixR.rms < 0.4;
        int k1 = SIGNAL_GAIN_STEPS - 1; for (int i = 0; i < k1; i++) press(KEY_MINUS);
        run(0.3); bool noneAtX1 = pairAt < 0 || (game.testRadarUnder() < 0 && game.audio.radarSignal2 == 0);
        for (int i = 0; i < k1; i++) press(KEY_EQUAL);
        run(0.3);
        // the empty sky: the direction of the sixty-four on a sphere farthest from every signal heard
        Vec3 empty(0, 0, 1); double bestSep = -1;
        for (int k = 0; k < 64; k++) {
            double zz = 1 - 2 * (k + 0.5) / 64, rr = std::sqrt(1 - zz * zz), ph = k * 2.399963; Vec3 d(rr * std::cos(ph), zz, rr * std::sin(ph));
            double sep = PI; for (int i = 0; i < game.testRadarSignals(); i++) sep = std::min(sep, std::acos(clampd(dot(d, game.testRadarDir(i)), -1, 1)));
            if (sep > bestSep) { bestSep = sep; empty = d; }
        }
        game.testAimAtDir(empty); run(0.2);
        // the false rises: each turned onto as it is born, held until it empties; three of them, none a lock
        int turned = 0, nothingSeen = 0, everLocked = 0; double maxHold = 0, maxRise = 0; bool shotTaken = false;
        for (int f = 0; f < 30 * 240 && turned < 3; f++) {
            frame();
            if (game.testRadarLocked() >= 0) everLocked++;
            Vec3 rd; if (!game.testRadarRise(rd)) continue;
            game.testAimAtDir(rd); turned++;
            int before = game.testRadarNothings();
            for (int g = 0; g < 30 * 18 && game.testRadarNothings() == before; g++) {
                frame(); if (game.testRadarLocked() >= 0) everLocked++;
                maxHold = std::max(maxHold, game.testRadarHold()); maxRise = std::max(maxRise, game.testRadarReading() - game.testRadarFloor());
            }
            if (game.testRadarNothings() > before) { nothingSeen++; if (!shotTaken) { writePNG("shots/tests/radar_nothing_there.png", game.output(), FBW, FBH); shotTaken = true; } }
            game.testAimAtDir(empty); run(0.5);
            while (game.testRadarRise(rd)) frame();   // the old ghost gone before the next is looked for
        }
        // a word heard before: home's first people's first text recording, put in the guide as heard; nothing on the air at the empty sky
        PeopleWorld hw; Signal hs; std::string key;
        if (firstPeopleWorld(hw, &hs)) { key = Guide::bodyKey(hs.star.sx, hs.star.sy, hs.star.sz, hs.body) + "/S" + std::to_string(shardWorldIndex(0, hw.firstText)); game.guide.heard.insert(key); }
        size_t logBefore = game.guide.log.size(), heardBefore = game.guide.heard.size();
        double wordAt = -1; bool wordEnded = false;
        for (int f = 0; f < 30 * 150 && !wordEnded; f++) {
            frame();
            if (wordAt < 0 && game.testRadarWords() > 0) wordAt = f / 30.0;
            if (wordAt >= 0 && !game.testRadarWordOn()) wordEnded = true;
        }
        bool wordOk = !key.empty() && wordAt >= 0 && game.testRadarWordKey() == key && wordEnded && game.guide.log.size() == logBefore && game.guide.heard.size() == heardBefore;
        check("radar: the top step in the game (C-16)", mixOk && noneAtX1 && turned >= 3 && nothingSeen >= 2 && everLocked == 0 && maxRise > 0.08 && wordOk,
                fmt("signal %d with %d under it %s (%.2f through), the synth with both rms %.3f peak %.2f%s, x1 %s; the sky %.0f deg from any signal: %d false rises held, NOTHING THERE %d times, the "
                    "hold to %.1f s, the meter %.2f over the floor, locked %d frames; a word back after %.0f s: '%s' (%s), %s, logged %zu, heard %zu",
                        pairAt, underAt, both ? "both on the air" : "NOT BOTH", lvl2, mixR.rms, mixR.peak, mixR.nan ? " NON-FINITE" : "", noneAtX1 ? "none under" : "ONE UNDER", bestSep / DEG, turned,
                        nothingSeen, maxHold, maxRise, everLocked, wordAt, game.testRadarWordSaid().c_str(), game.testRadarWordKey() == key ? "the one heard" : "NOT the one heard",
                        wordEnded ? "ended" : "NOT ENDED", game.guide.log.size() - logBefore, game.guide.heard.size() - heardBefore));
        remove("shots/tests/test_guide.txt");
    }
}
