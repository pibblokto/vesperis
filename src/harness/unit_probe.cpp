// E-03: the unit's probe (group `probe`)
#include "harness/harness.h"

// X-01 to X-04, B-409: the probe
void unitProbe(const Check& check) {
    {   // X-01: the probe's air and clock on the first giants of the scan: the height falls and the temperature rises with the pressure
        // below the tropopause, the one-bar level is at height zero, the stages follow in their order along the clock to the end, which
        // comes at the hull's pressure or the heat's, and the chute holds the pressure's growth to a quarter of its pace over its span
        int giants = 0, bad = 0; std::string why;
        auto clockAt = [](const ProbeFlight& ff, double bar) { double lo = probeDescentStart(), hi = 1e5; for (int i = 0; i < 80; i++) { double m = 0.5 * (lo + hi); if (probeBarAt(ff, m) < bar) lo = m; else hi = m; } return 0.5 * (lo + hi); };
        for (int64_t x = 150; x < 200 && giants < 16; x++) for (int64_t z = 20; z < 80 && giants < 16; z++) {
            Star s; StarSystem sys; if (!systemAt(x, 0, z, s, sys, true)) continue;
            for (const Body& b : sys.bodies) {
                if (!isProbeGiant(b.type) || giants >= 16) continue;
                giants++;
                GiantAtmosphere a = giantAtmosphereOf(sys, b.index);
                ProbeFlight f; f.crushBar = probeCrushBar(b.seed);
                bool ok = a.valid; std::string w;
                double zPrev = 1e9, tPrev = 0;
                for (double lp = std::log(1e-6); lp < std::log(40.0); lp += 0.1) {
                    double p = std::exp(lp), zz = giantAltitudeKm(a, p), tt = giantTemperatureK(a, p);
                    if (zz >= zPrev) { ok = false; w = "height"; }
                    if (p > a.tropoBar * 1.01 && tt <= tPrev) { ok = false; w = "temperature"; }
                    zPrev = zz; tPrev = tt;
                }
                if (std::fabs(giantAltitudeKm(a, 1.0)) > 1e-9) { ok = false; w = "one bar"; }
                int last = -1, lastLayer = -1; double endC = probeEndClock(a, f);
                for (double c = 0; c < endC + 1; c += 0.5) {   // X-03: the layers in order (a stage comes back between three decks)
                    int st = probeStageAt(a, f, c);
                    if (st >= PS_ABOVE && st <= PS_DEEP) { int ly = giantLayerAt(a, probeBarAt(f, c)); if (ly < lastLayer) { ok = false; w = "order"; } lastLayer = ly; }
                    else if (st < last && st != PS_END) { ok = false; w = "order"; }
                    last = st;
                }
                if (last != PS_END) { ok = false; w = "no end"; }
                double endBar = probeBarAt(f, endC), want = probeEndBar(a, f);
                if (std::fabs(endBar - want) > 1e-6 * want || (want < f.crushBar * 0.999 && giantTemperatureK(a, want) < a.heatK * 0.999)) { ok = false; w = "the end"; }
                double c4 = clockAt(f, 0.4);
                ProbeFlight fc = f; fc.chuteOpen = c4; fc.chuteClose = probeChuteCloseAt(f, c4);
                double free12 = clockAt(f, 1.2) - c4, chute12 = clockAt(fc, 1.2) - c4;
                if (std::fabs(chute12 / free12 - PROBE_CHUTE_SLOW) > 0.01 || std::fabs(probeBarAt(fc, fc.chuteClose) / (0.4 * PROBE_CHUTE_SPAN) - 1) > 0.01) { ok = false; w = fmt("the chute %.2f x", chute12 / free12); }
                if (!ok) { bad++; if (why.empty()) why = fmt("(%s body %d: %s)", s.name.c_str(), b.index, w.c_str()); }
            }
        }
        check("probe: the giant's air and the clock (X-01)", giants >= 10 && bad == 0, fmt("%d giants, %d wrong %s", giants, bad, why.c_str()));
    }
    {   // X-01: a probe in a game. Parked at a giant with the telescope's reticle on its disc, C sends the probe to the ground under the
        // reticle and the screen becomes its camera; saved in the descent and loaded, it falls on from the same clock on its screen; the
        // ship leaving its parking cuts the relay; a second probe falls to its end, the last frame held until Enter gives control back
        TestGame game; game.savePrefix = "shots/tests/unit_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_guide.txt";
        applyTestScale(game);
        Input in;
        auto press = [&](int key) { pressKey(game, in, key); };
        auto run = [&](double s) { runFrames(game, in, s); };
        Star gs; int gi = -1, other = -1;
        for (int64_t x = 150; x < 200 && gi < 0; x++) for (int64_t z = 20; z < 80 && gi < 0; z++) {
            Star s; StarSystem sys; if (!systemAt(x, 0, z, s, sys, true)) continue;
            for (int i = 0; i < (int)sys.bodies.size(); i++) if (sys.bodies[i].type == PT_GASGIANT) { gs = s; gi = i; other = i == 0 ? 1 : 0; break; }
        }
        bool aimOk = false, launched = false, resumed = false, cut = false, ended = false, back = false, fastOk = false; std::string d1, d2, d3, dFast;
        if (gi >= 0) {
            game.testParkAt(gs, gi); run(0.3);
            game.testAimAtBody(gi); press(KEY_Z); in.down[KEY_RIGHT] = true; run(0.3); in.down[KEY_RIGHT] = false; run(0.1);
            double tl = 0, tn = 0, pl = 0, pn = 0, sl = 0, sn = 0;
            bool onDisc = game.testTelescopeGround(tl, tn); game.testSubShip(sl, sn);
            press(KEY_C); run(0.1);
            launched = game.testProbeActive() && game.state == GameState::PROBE && !game.testTelescopeOn();
            game.testProbeAimed(pl, pn);
            double off = std::acos(clampd(dot(StarSystem::bodyFromLatLon(tl, tn), StarSystem::bodyFromLatLon(pl, pn)), -1, 1)) / DEG, offShip = std::acos(clampd(dot(StarSystem::bodyFromLatLon(sl, sn), StarSystem::bodyFromLatLon(pl, pn)), -1, 1)) / DEG;
            aimOk = onDisc && off < 0.3 && offShip > 2;
            d1 = fmt("the probe sent %.3f deg from the reticle's ground (%.1f deg from under the ship)", off, offShip);
            {   // R-406: a second watched plainly, a second with Shift held
                double ca = game.testProbeClock(); run(1.0);
                double cb = game.testProbeClock(); in.down[KEY_LEFT_SHIFT] = true; run(1.0); in.down[KEY_LEFT_SHIFT] = false;
                double cc = game.testProbeClock(); run(0.5);
                double cd = game.testProbeClock();
                double plain = cb - ca, fast = cc - cb, after = (cd - cc) / 0.5;
                fastOk = plain > 0.9 && plain < 1.1 && std::fabs(fast / plain - 8) < 0.4 && after < 1.15;
                dFast = fmt("a second plain %.2f s, with Shift %.2f s (x%.2f), after it %.2f s a second", plain, fast, fast / std::max(plain, 1e-9), after);
            }
            game.testProbeSkip(probeDescentStart() + 80); run(0.1);
            double c0 = game.testProbeClock(); int st0 = game.testProbeStage(); double bar0 = game.testProbeBar();
            game.currentSlot = 1; in.down[KEY_LEFT_CONTROL] = true; press(KEY_S); in.down[KEY_LEFT_CONTROL] = false;
            run(1.0);
            bool loaded = game.loadSlot(1);
            resumed = loaded && game.testProbeActive() && game.state == GameState::PROBE && std::fabs(game.testProbeClock() - c0) < 0.1 && game.testProbeStage() == st0;
            d2 = fmt("saved at %.2f s (%s, %.3f bar), loaded at %.2f s, state %d", c0, PROBE_STAGE_NAMES[std::max(0, st0)], bar0, game.testProbeClock(), (int)game.state);
            game.testParkAt(gs, other); run(0.1);   // the ship leaves the giant
            cut = !game.testProbeActive() && !game.guide.log.empty() && game.guide.log.back().text.find("CUT") != std::string::npos;
            game.testParkAt(gs, gi); run(0.3); press(KEY_C); run(0.1);
            game.testProbeSkip(1e6); run(0.2);
            ended = game.testProbeEnded() && game.state == GameState::PROBE && !game.guide.log.empty() && game.guide.log.back().text.find("LOST IN") != std::string::npos;
            d3 = game.testProbeInfo();
            press(KEY_ENTER); run(0.1);
            back = game.state == GameState::SPACE && !game.testProbeActive();
        }
        check("probe: aimed by the telescope (X-01)", gi >= 0 && launched && aimOk, d1);
        check("probe: Shift runs the relay's clock x8 (R-406)", fastOk, dFast);
        check("probe: saved and resumed in the descent (X-01)", resumed, d2);
        check("probe: leaving cuts the relay; the end (X-01)", cut && ended && back, fmt("cut %d, ended %d, back %d; %s", (int)cut, (int)ended, (int)back, d3.c_str()));
    }
    {   // X-02: the descent's clouds read the globe. On the first giants of the scan, round three aim points each: the bands are the
        // globe's own (the planet function at the jets' drift of the time, against the world map's albedo there, which smooths it
        // over its texels), and the decks keep their order everywhere (the ammonia deck's top over its base, its base over the
        // water deck's towers, the water deck's top over its base)
        int giants = 0, badBands = 0, badDecks = 0, points = 0; double worst = 0; std::string why;
        for (int64_t x = 150; x < 200 && giants < 6; x++) for (int64_t z = 20; z < 80 && giants < 6; z++) {
            Star s; StarSystem sys; if (!systemAt(x, 0, z, s, sys, true)) continue;
            for (const Body& b : sys.bodies) {
                if (!isProbeGiant(b.type) || giants >= 6) continue;
                giants++;
                BodyGen g = BodyGen::make(b);
                GiantAtmosphere a = giantAtmosphereOf(sys, b.index);
                PlanetMap map; map.generate(g);
                for (int k = 0; k < 3; k++) {
                    uint64_t h = mix64(b.seed ^ (0xB4D5ULL + k));
                    double lat = (unitFromHash(h) - 0.5) * 140 * DEG, lon = (unitFromHash(mix64(h ^ 1)) - 0.5) * TAU, t = 3.6e6 + 1e5 * k;
                    const int n = 9; const double half = 1600;
                    GiantBands B = giantBandsOf(b, g, lat, lon, t, half, n);
                    Vec3 c0 = StarSystem::bodyFromLatLon(lat, lon), e0 = normalize(cross(Vec3(0, 0, 1), c0)), n0 = cross(c0, e0);
                    for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) {
                        double px = -half + i * 2 * half / (n - 1), pz = -half + j * 2 * half / (n - 1);
                        double la, lo; StarSystem::latLonFromBody(normalize(c0 * b.radiusKm + e0 * px + n0 * pz), la, lo);
                        double globe = map.albedoAt(lo + giantJetAt(b, g, la, lo, t), la), al, st; B.at(px, pz, al, st);
                        worst = std::max(worst, std::fabs(globe - al)); points++;
                        if (std::fabs(globe - al) > 0.1) { badBands++; if (why.empty()) why = fmt("%s body %d: %.2f against the globe's %.2f", s.name.c_str(), b.index, al, globe); }
                    }
                    GiantDecks D; D.init(a, &B, h);
                    for (int q = 0; q < 300; q++) {   // X-03: every deck of the giant's, one to three
                        double px = (unitFromHash(mix64(h ^ (q * 2 + 7))) - 0.5) * 2 * half, pz = (unitFromHash(mix64(h ^ (q * 2 + 8))) - 0.5) * 2 * half;
                        for (int k = 0; k < D.n; k++) {
                            double tk = D.top(k, px, pz, 0), bk = D.base(k, px, pz, 0), tn = k + 1 < D.n ? D.top(k + 1, px, pz, 0) : -1e9;
                            if (!(tk > bk + 0.5 * D.sh && bk > tn + 0.3 * D.sh)) { badDecks++; if (why.empty()) why = fmt("%s body %d deck %d: %.1f %.1f, next %.1f km (sh %.2f)", s.name.c_str(), b.index, k, tk, bk, tn, D.sh); }
                        }
                    }
                }
            }
        }
        check("probe: the clouds are the globe's bands (X-02)", giants >= 4 && badBands == 0, fmt("%d giants, %d points, worst %.3f; %s", giants, points, worst, why.c_str()));
        check("probe: the decks keep their order (X-02)", giants >= 4 && badDecks == 0, fmt("%d giants, %d out of order; %s", giants, badDecks, why.c_str()));
    }
    {   // X-03: every giant its own character, on the first 300 giants of the scan: one to three decks, each where the adiabat
        // crosses its cloud's temperature (the abundance's shift), from the top down with a clear band between; the kinds as
        // defined (an ice giant a small blue gas giant, a brown dwarf the substellar type) and all three among them; a brown
        // dwarf's glow over every gas giant's, a young giant's over the old ones' on the whole; the decks' counts all used
        int giants = 0, bad = 0, kinds[GK_COUNT] = {}, decks[4] = {0, 0, 0, 0}; double glowYoung = 0, glowOld = 0, glowBrownMin = 1e9, glowGasMax = 0; int nYoung = 0, nOld = 0;
        std::string why;
        for (int64_t x = 150; x < 320 && giants < 300; x++) for (int64_t z = 20; z < 140 && giants < 300; z++) {
            Star s; StarSystem sys; if (!systemAt(x, 0, z, s, sys, true)) continue;
            for (int bi = 0; bi < (int)sys.bodies.size() && giants < 300; bi++) {
                const Body& b = sys.bodies[bi];
                if (!isProbeGiant(b.type)) continue;
                giants++;
                const GiantAtmosphere a = giantAtmosphereOf(sys, bi);
                const GiantCharacter& c = a.ch;
                kinds[c.kind]++; decks[std::min(3, std::max(0, a.decks))]++;
                std::string w;
                if (a.decks < 1 || a.decks > GIANT_MAX_DECKS) w = fmt("%d decks", a.decks);
                for (int k = 0; k < a.decks && w.empty(); k++) {
                    if (!(a.top[k] < a.base[k] && (k == 0 || a.top[k] >= a.base[k - 1] * 1.249))) w = fmt("deck %d %.3f-%.3f bar", k, a.top[k], a.base[k]);
                    const double want = CLOUD_SPECIES[a.species[k]].condK * std::pow(c.abundance, 0.6 * 0.286), got = giantTemperatureK(a, a.base[k]);
                    const double reach = a.reach;   // P-07: the heat's end or a hot giant's 32 bar
                    if (w.empty() && std::fabs(reach - ((c.kind == GK_AZURE || c.kind == GK_ALKALI || c.kind == GK_SILICATE) ? 32.0 : std::min(32.0, std::pow(a.heatK / a.t1K, 1 / 0.286)))) > 1e-9) w = "the decks' reach";
                    const bool fallback = a.decks == 1 && a.species[0] == CS_SALT && std::fabs(a.base[0] - 0.8 * reach) < 1e-6 * reach;   // nothing condenses within the reach: the salts' haze over the end
                    if (w.empty() && !fallback && std::fabs(got / want - 1) > 0.02 && a.base[k] < 31.9) w = fmt("deck %d (%s) base at %.0f K, its cloud's %.0f", k, CLOUD_SPECIES[a.species[k]].name, got, want);
                }
                const bool ice = isIceGiant(b);
                if (w.empty() && (c.kind == GK_ICE) != ice) w = "the ice giant's rule";
                if (w.empty() && (c.kind == GK_BROWN) != (b.type == PT_SUBSTELLAR)) w = "the brown dwarf's kind";
                if (c.kind == GK_BROWN) glowBrownMin = std::min(glowBrownMin, c.glow); else if (c.kind == GK_GAS) glowGasMax = std::max(glowGasMax, c.glow);
                if (c.kind == GK_GAS) { if (c.young) { glowYoung += c.glow; nYoung++; } else { glowOld += c.glow; nOld++; } }
                if (!w.empty()) { bad++; if (why.empty()) why = fmt("(%s %s: %s)", s.name.c_str(), b.name.c_str(), w.c_str()); }
            }
        }
        const bool kindsOk = kinds[0] > 0 && kinds[1] > 0 && kinds[2] > 0 && glowBrownMin > glowGasMax && nYoung > 0 && glowYoung / nYoung > 3 * glowOld / std::max(1, nOld) && decks[1] > 0 && decks[2] > 0 && decks[3] > 0;
        check("probe: every giant its own decks and kind (X-03)", giants >= 250 && bad == 0 && kindsOk,
              fmt("%d giants (%d gas, %d ice, %d brown; 1/2/3 decks %d/%d/%d), %d wrong %s; glow brown >= %.2f, gas <= %.2f, young %.3f, old %.3f", giants, kinds[0], kinds[1], kinds[2], decks[1], decks[2], decks[3], bad, why.c_str(),
                  glowBrownMin, glowGasMax, glowYoung / std::max(1, nYoung), glowOld / std::max(1, nOld)));
    }
    {   // X-03: a great storm is the globe's (GEN 12): the planet function's oval differs from the bands round it by its tone, the
        // descent's bands sampled at the storm see the same albedo as the map, its turn keeps it whole a day later (every point of
        // it carried by the jet at its centre's latitude), and the deck stands on its dome and higher on its wall
        int storms = 0, bad = 0; std::string why;
        for (int64_t x = 150; x < 320 && storms < 6; x++) for (int64_t z = 20; z < 140 && storms < 6; z++) {
            Star s; StarSystem sys; if (!systemAt(x, 0, z, s, sys, true)) continue;
            for (int bi = 0; bi < (int)sys.bodies.size() && storms < 6; bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type != PT_GASGIANT) continue;
                const BodyGen g = BodyGen::make(b);
                if (!g.gsOn) continue;
                storms++;
                std::string w;
                const SurfaceSample in = sampleSurface(g, StarSystem::bodyFromLatLon(g.gsLat, g.gsLon), 1000.0);
                double outside = 0; int no = 0;
                for (int q = 0; q < 16; q++) { double ang = q * TAU / 16; outside += sampleSurface(g, StarSystem::bodyFromLatLon(g.gsLat + 1.6 * g.gsB * std::sin(ang), g.gsLon + 1.6 * g.gsA / std::cos(g.gsLat) * std::cos(ang)), 1000.0).albedo; no++; }
                outside /= no;
                if ((in.albedo - outside) * g.gsTone < 0.05) w = fmt("its albedo %.2f against %.2f round it (tone %+.0f)", in.albedo, outside, g.gsTone);
                const double t = 3.6e6, t2 = t + 86400;
                const double lonC = g.gsLon - giantJetTurn(b, g.bandCount, g.gsLat, t), lonC2 = g.gsLon - giantJetTurn(b, g.bandCount, g.gsLat, t2);
                for (int q = 0; q < 8 && w.empty(); q++) {   // its rim a day later, where the pattern carried its centre
                    double ang = q * TAU / 8, la = g.gsLat + 0.8 * g.gsB * std::sin(ang), dlo = 0.8 * g.gsA / std::cos(g.gsLat) * std::cos(ang);
                    double m1 = wrapAngle(lonC + dlo + giantJetAt(b, g, la, lonC + dlo, t) - g.gsLon), m2 = wrapAngle(lonC2 + dlo + giantJetAt(b, g, la, lonC2 + dlo, t2) - g.gsLon);
                    if (std::fabs(m1 - dlo) > 1e-6 || std::fabs(m2 - dlo) > 1e-6) w = fmt("its rim slips by %.4f rad a day", std::fabs(m2 - dlo));
                }
                if (w.empty()) {   // the descent aimed at its centre
                    GiantBands B = giantBandsOf(b, g, g.gsLat, lonC, t, 1600, 9);
                    double al, st; B.at(0, 0, al, st);
                    if (!B.gsOn || std::fabs(al - in.albedo) > 0.08 || B.greatStormAt(0, 0) > 0.05) w = fmt("the descent's bands %.2f against the globe's %.2f (its oval %s, d %.2f)", al, in.albedo, B.gsOn ? "on" : "off", B.greatStormAt(0, 0));
                    else {
                        GiantAtmosphere a = giantAtmosphereOf(sys, bi);
                        GiantDecks D; D.init(a, &B, 7);
                        const double dome = D.lift(0, 0, 0) - (4.0 * (al - 0.55) + 4.5 * st) * D.sh;
                        double crest = -1e9;
                        for (double u = 0.9; u < 1.06; u += 0.002) {   // along the oval's east axis to its rim
                            const double xx = B.gsX + B.gsEx * u * B.gsAKm, zz = B.gsZ + B.gsEz * u * B.gsAKm;
                            double a2, s2; B.at(xx, zz, a2, s2);
                            crest = std::max(crest, D.lift(0, xx, zz) - (4.0 * (a2 - 0.55) + 4.5 * s2) * D.sh);
                        }
                        if (dome < 5 * D.sh || crest < dome + 8 * D.sh) w = fmt("its dome %.1f km and wall's crest %.1f km (sh %.2f)", dome, crest, D.sh);
                    }
                }
                if (!w.empty()) { bad++; if (why.empty()) why = fmt("(%s %s: %s)", s.name.c_str(), b.name.c_str(), w.c_str()); }
            }
        }
        check("probe: the great storm is the globe's (X-03)", storms >= 4 && bad == 0, fmt("%d great storms, %d wrong %s", storms, bad, why.c_str()));
    }
    {   // B-409: no giant's picture black, nor all one clipped colour. A giant with one deck (one in eight: Sashihene VIII, an ice
        // giant with the water deck alone) went black from its deck's top down: the deck's base was tinted by a smoothstep whose
        // edges were one (0/0, a NaN through the air's and the fog's colours); a brown dwarf's fog clipped to a flat orange at its
        // first deck's base. Giants of each kind with one deck and with more, without the HUD and the static: each deck's middle
        // and base and the deep, the frame's mean light over a floor and its clipped share under a ceiling
        TestGame game; game.savePrefix = "shots/tests/unit_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_guide.txt";
        applyTestScale(game);
        game.harness.probeQuiet = true; game.photoMode = true;
        Input in;
        auto press = [&](int key) { pressKey(game, in, key); };
        auto run = [&](double s) { runFrames(game, in, s); };
        const long long pins[][4] = {{139, -105, -43, 8}, {153, 0, 32, 0}, {150, 0, 73, 1}, {150, 0, 73, 0}, {150, 0, 106, 5}};   // one deck: an ice giant, a gas giant, a brown dwarf; two and three
        int giants = 0, oneDeck = 0, frames = 0, bad = 0; double darkest = 1e9, clipMost = 0; std::string why;
        for (const auto& pin : pins) {
            Star st; StarSystem sys; if (!systemAt(pin[0], pin[1], pin[2], st, sys, true)) continue;
            const int bi = (int)pin[3];
            if (bi >= (int)sys.bodies.size() || !isProbeGiant(sys.bodies[bi].type)) continue;
            game.testParkAt(st, bi); run(0.3); press(KEY_C); run(0.1);
            if (!game.testProbeActive()) continue;
            giants++; oneDeck += game.testProbeAtm().decks == 1;
            std::vector<std::pair<double, std::string>> at;
            for (const LayerRun& r : probeLayerRuns(game, game.testProbeEndClock())) {
                if (r.stage == PS_DECK1 || r.stage == PS_DECK2) { at.push_back({r.from + (r.to - r.from) * 0.5, r.tag}); at.push_back({r.from + (r.to - r.from) * 0.95, r.tag + " base"}); }
                else if (r.stage == PS_DEEP) at.push_back({r.from + (r.to - r.from) * 0.5, "deep"});
            }
            for (const auto& [c, tag] : at) {
                if (c <= game.testProbeClock()) continue;
                game.testProbeSkip(c - 1.0 / 30); game.frame(in, 1.0 / 30); in.newFrame();
                const uint32_t* px = game.output(); const size_t n = (size_t)FBW * FBH; size_t clipped = 0; double sum = 0;
                for (size_t i = 0; i < n; i++) {
                    const int r = px[i] & 255, g = (px[i] >> 8) & 255, b = (px[i] >> 16) & 255;
                    sum += 0.3 * r + 0.59 * g + 0.11 * b; clipped += std::max(r, std::max(g, b)) >= 250;
                }
                const double mean = sum / n, clip = (double)clipped / n;
                frames++; darkest = std::min(darkest, mean); clipMost = std::max(clipMost, clip);
                if (mean < 8 || clip > 0.4) { bad++; if (why.size() < 300) why += fmt("; %s body %d %s: mean %.1f, clipped %.0f%%", sys.star.name.c_str(), bi, tag.c_str(), mean, clip * 100); }
            }
            game.testProbeRelease(); run(0.1);
        }
        check("probe: no giant's picture black or clipped flat (B-409)", giants == 5 && oneDeck == 3 && frames >= 20 && bad == 0,
              fmt("%d giants (%d with one deck), %d frames, %d wrong; darkest mean %.1f, most clipped %.0f%%%s", giants, oneDeck, frames, bad, darkest, clipMost * 100, why.c_str()));
    }
    {   // X-04: what comes back. A probe into Wailie VI (150 0 106 body 5), its camera turned on the way, to its end: the record in the
        // guide (the tracks, the profile, the layers it found) and its final image beside the photographs; the guide read back and
        // exported to a friend (C-14's lending): every number the same, the lent one marked and not lent on. The recording from the
        // gallery: its frame at a clock is the live frame of that clock (the descent drawn again), the arrows go deeper. A second and a
        // third probe at other places: their profiles differ; the statistics count three
        mkdir("shots/tests/test_x04", 0755);
        for (const char* f : {"shots/tests/test_x04/guide.txt", "shots/tests/test_x04/probe_1.png", "shots/tests/test_x04/export.txt", "shots/tests/test_x04/friend.txt"}) remove(f);
        TestGame game; game.savePrefix = "shots/tests/test_x04/save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_x04/guide.txt";
        game.shotsDir = "shots/tests/test_x04"; game.harness.probeQuiet = true;
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        auto lum = [](uint32_t c) { return 0.3 * (c & 255) + 0.59 * ((c >> 8) & 255) + 0.11 * ((c >> 16) & 255); };
        Star st; bool have = starInSector(150, 0, 106, st, true);
        std::vector<uint32_t> live; const double cLive = 160;
        if (have) {
            game.testParkAt(st, 5); run(0.3);
            press(KEY_C); run(0.2);
            game.testProbeSkip(70); run(0.1); game.testProbeLookAt(0.5, -0.1); run(0.3);
            game.testProbeSkip(120); game.testProbeLookAt(-0.3, 0.05); run(0.1);
            game.testProbeSkip(cLive - 1.0 / 30); game.photoMode = true; run(1.0 / 30); live.assign(game.output(), game.output() + (size_t)FBW * FBH); game.photoMode = false;
            game.testProbeSkip(1e6); run(0.2);
            press(KEY_ESCAPE); run(0.1);
        }
        const ProbeRecord* r = game.guide.probes.empty() ? nullptr : &game.guide.probes.back();
        const bool kept = r && r->id == 1 && r->endCause == 1 && r->profile.size() >= 20 && r->layers.size() >= 9 && r->cam.size() >= 12 && r->time.size() >= 4 && fileExists("shots/tests/test_x04/probe_1.png");
        auto near = [](double x, double y) { return std::fabs(x - y) <= 1e-7 * std::max(1.0, std::fabs(x)); };
        auto same = [&](const ProbeRecord& a, const ProbeRecord& b) {
            if (a.id != b.id || a.body != b.body || a.seed != b.seed || a.storm != b.storm || a.endCause != b.endCause || a.gen != b.gen) return false;
            const double x[] = {a.launchT, a.aimLat, a.aimLon, a.startX, a.startY, a.startZ, a.headYaw, a.descYaw, a.chuteOpen, a.chuteClose, a.crushBar, a.endClock, a.endBar, a.endKm, a.endK};
            const double y[] = {b.launchT, b.aimLat, b.aimLon, b.startX, b.startY, b.startZ, b.headYaw, b.descYaw, b.chuteOpen, b.chuteClose, b.crushBar, b.endClock, b.endBar, b.endKm, b.endK};
            for (int i = 0; i < 15; i++) if (!near(x[i], y[i])) return false;
            if (a.cam.size() != b.cam.size() || a.time.size() != b.time.size() || a.layers.size() != b.layers.size() || a.profile.size() != b.profile.size()) return false;
            for (size_t i = 0; i < a.cam.size(); i++) if (!near(a.cam[i], b.cam[i])) return false;
            for (size_t i = 0; i < a.time.size(); i++) if (!near(a.time[i], b.time[i])) return false;
            for (size_t i = 0; i < a.layers.size(); i++) if (!near(a.layers[i], b.layers[i])) return false;
            for (size_t i = 0; i < a.profile.size(); i++) {
                const ProbeSample &p = a.profile[i], &q = b.profile[i];
                if (!near(p.clock, q.clock) || !near(p.bar, q.bar) || !near(p.tempK, q.tempK) || !near(p.windE, q.windE) || !near(p.light, q.light) || p.layer != q.layer || p.flashes != q.flashes) return false;
            }
            return true;
        };
        Guide back; const bool loaded = back.load(game.guidePath);
        const bool readBack = loaded && r && back.probes.size() == 1 && same(*r, back.probes[0]) && !back.probes[0].lent && back.probesSent == 1;
        game.guide.save("shots/tests/test_x04/export.txt");   // the guide's export
        Guide fr; int lentN = 0, probesN = 0, onN = -1;
        const int names = fr.importInbox("shots/tests/test_x04/export.txt", &lentN, &probesN);
        const bool lentOk = names >= 0 && probesN == 1 && fr.probes.size() == 1 && r && same(*r, fr.probes[0]) && fr.probes[0].lent && fr.deepestProbeKm() == 0;
        fr.save("shots/tests/test_x04/friend.txt");
        { Guide third; third.importInbox("shots/tests/test_x04/friend.txt", nullptr, &onN); }   // a lent recording is not lent on
        std::string why;
        if (r) {
            why = fmt("probe %d: %zu samples, %zu layers, %zu camera keys, %zu clock keys, end %.0f km", r->id, r->profile.size(), r->layers.size() / 3, r->cam.size() / 3, r->time.size() / 2, r->endKm);
            for (size_t i = 0; i + 2 < r->layers.size(); i += 3) why += (i ? ", " : "; ") + giantLayerName(game.testProbeAtm().valid ? game.testProbeAtm() : giantAtmosphereOf(game.testSystem(), 5), (int)r->layers[i], true);
        }
        check("probe: a recording and its profile kept, read back and lent (X-04)", have && kept && readBack && lentOk && onN == 0,
              why + fmt("; kept %d, read back %d, lent %d (%d recordings), lent on %d", kept, readBack, lentOk, probesN, onN));
        // the recording from the gallery: the newest item; its frame at the live clock against the live frame; the arrows go deeper
        int newest = -2; double diff = 1e9, frac = 1, lumMean = 0, cDeep = -1; bool playing = false;
        if (r) {
            game.testOpenGallery(); run(0.05);
            newest = game.testGalleryRec(game.testGallerySel());
            press(KEY_ENTER); run(0.05);
            playing = game.state == GameState::RECORDING;
            game.testRecordingSeek(cLive, false); game.photoMode = true; run(1.0 / 30); game.photoMode = false;
            const uint32_t* o = game.output();
            double sum = 0; size_t big = 0, n = (size_t)FBW * FBH;
            for (size_t i = 0; i < n && live.size() == n; i++) { const double d = std::fabs(lum(o[i]) - lum(live[i])); sum += d; if (d > 30) big++; lumMean += lum(o[i]) / n; }
            if (live.size() == n) { diff = sum / n; frac = (double)big / n; }
            writePNG("shots/tests/unit_x04_recording.png", o, FBW, FBH);
            gi.down[KEY_RIGHT] = true; run(1.0); gi.down[KEY_RIGHT] = false; run(1.0 / 30);
            cDeep = game.testRecordingClock();
            press(KEY_ESCAPE); run(0.05);
        }
        check("probe: the recording plays from the gallery as it fell (X-04)", newest == 0 && playing && diff < 2.0 && frac < 0.01 && lumMean > 8 && cDeep > cLive + 20 && game.state == GameState::GALLERY,
              fmt("the gallery's newest %d, playing %d; its frame at %.0f s against the live one: %.2f levels a pixel, %.2f%% over 30 (mean %.0f); a second of Right: %.0f s", newest, playing, cLive, diff, frac * 100, lumMean, cDeep));
        // two more probes into the giant at other places
        press(KEY_ESCAPE); run(0.05);
        for (int k = 0; k < 2 && have; k++) {
            game.testParkAt(st, 5); game.setState(GameState::SPACE); run(0.2);
            game.harness.aimOverride = true; game.harness.aimLat = (k ? 36 : 4) * DEG; game.harness.aimLon = (k ? 70 : -20) * DEG;
            press(KEY_C); run(0.1); game.harness.aimOverride = false;
            game.testProbeSkip(1e6); run(0.1); press(KEY_ESCAPE); run(0.05);
        }
        double dw = 0, dTop = 0; int matched = 0;
        if (game.guide.probes.size() == 3) {
            const ProbeRecord &a = game.guide.probes[1], &b = game.guide.probes[2];
            for (const ProbeSample& p : a.profile) for (const ProbeSample& q : b.profile) if (std::fabs(std::log(p.bar / q.bar)) < 1e-3) { matched++; dw = std::max(dw, std::fabs(p.windE - q.windE)); }
            for (size_t i = 0; i + 2 < a.layers.size(); i += 3) for (size_t j = 0; j + 2 < b.layers.size(); j += 3)
                if (a.layers[i] == 2 && b.layers[j] == 2) dTop = std::fabs(std::log(a.layers[i + 1] / b.layers[j + 1]));   // where each met the first deck's top
        }
        check("probe: two places of one giant, two profiles (X-04)", game.guide.probes.size() == 3 && matched >= 20 && (dw > 10 || dTop > 0.02) && game.guide.probesSent == 3 && game.guide.deepestProbeKm() > 50,
              fmt("%zu records, %d depths matched, the wind %.0f m/s apart at most, the first deck's top %.1f%% apart; sent %d, the deepest %.0f km", game.guide.probes.size(), matched, dw, dTop * 100, game.guide.probesSent, game.guide.deepestProbeKm()));
    }
    {   // X-04: what a probe crosses goes to the log with its depth, once: the great storm (named on the probe's screen: the name in the
        // guide and the log), its wall from outside, the aurora's curtains, a clear-air hole, diamond hail
        struct G { int64_t sx, sz; int bi; GiantAtmosphere a; };
        std::vector<G> all;
        for (int64_t x = 150; x < 320 && all.size() < 600; x++)
            for (int64_t z = 20; z < 140 && all.size() < 600; z++) {
                Star s; StarSystem ss; if (!systemAt(x, 0, z, s, ss, true)) continue;
                for (int i = 0; i < (int)ss.bodies.size(); i++) if (isProbeGiant(ss.bodies[i].type)) all.push_back({x, z, i, giantAtmosphereOf(ss, i)});
            }
        const G* gs = nullptr; const G* au = nullptr; const G* ho = nullptr; const G* ha = nullptr;
        for (const G& g : all) {
            const GiantCharacter& c = g.a.ch;
            if (c.greatStorm && c.kind == GK_GAS && !gs) gs = &g;
            if (c.kind == GK_GAS && (!au || c.aurora > au->a.ch.aurora)) au = &g;
            if (c.holes > 0 && c.kind == GK_GAS && g.a.decks >= 2 && !ho) { GiantDecks D; D.init(g.a, nullptr, 1); if (D.holeable(0)) ho = &g; }
            if (c.hail && !ha) ha = &g;
        }
        remove("shots/tests/test_x04/guide.txt");
        TestGame game; game.savePrefix = "shots/tests/test_x04/save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_x04/guide.txt";
        game.shotsDir = "shots/tests/test_x04"; game.harness.probeQuiet = true;
        game.newGame(); game.setState(GameState::SPACE);
        Input gi;
        auto run = [&](double secs) { runFramesAtLeastOne(game, gi, secs); };
        auto press = [&](int key) { pressKey(game, gi, key); };
        auto launch = [&](const G& g, bool aimed, double lat, double lon) {
            Star st; starInSector(g.sx, 0, g.sz, st, true);
            if (game.testProbeActive()) game.testProbeRelease();
            game.testParkAt(st, g.bi); game.setState(GameState::SPACE); run(0.2);
            game.harness.aimOverride = aimed; game.harness.aimLat = lat; game.harness.aimLon = lon;
            press(KEY_C); run(0.05); game.harness.aimOverride = false;
            return game.testProbeActive();
        };
        std::string named;
        auto fall = [&](bool nameIt) {   // in steps (the log's crossings are looked for each frame), the storm named on the way
            const double endC = game.testProbeEndClock();
            for (double c = 55; c < endC; c += 4) {
                game.testProbeSkip(c); run(1.0 / 30);
                if (nameIt && c > 100) { press(KEY_N); run(1.0 / 30); game.testTypeText("RED EYE"); run(1.0 / 30); named = game.testStatus(); nameIt = false; }
            }
            game.testProbeSkip(1e6); run(0.05); press(KEY_ESCAPE); run(0.05);
        };
        if (gs) {
            Star st; starInSector(gs->sx, 0, gs->sz, st, true);
            StarSystem ss; ss.generate(st);
            const Body& b = ss.bodies[gs->bi]; BodyGen g = BodyGen::make(b);
            game.testParkAt(st, gs->bi); run(0.1);
            const double lon = wrapAngle(g.gsLon - giantJetTurn(b, g.bandCount, g.gsLat, game.testGameTime()));
            if (launch(*gs, true, g.gsLat, lon)) fall(true);
            if (launch(*gs, true, g.gsLat + (g.gsLat >= 0 ? 1 : -1) * 1.15 * g.gsB, lon)) fall(false);
        }
        if (au) {
            Star st; starInSector(au->sx, 0, au->sz, st, true); game.testParkAt(st, au->bi); run(0.1);
            const StarSystem& sys = game.testSystem(); const double t = game.testGameTime();
            double sla, slo; StarSystem::latLonFromBody(normalize(sys.bodyFrame(au->bi, t) * normalize(sys.star.pos - sys.bodyPos(au->bi, t))), sla, slo);
            const GiantCharacter& c = au->a.ch;
            Vec3 pole = StarSystem::bodyFromLatLon(PI / 2 - c.magColat, c.magLon), anti = StarSystem::bodyFromLatLon(-sla, slo + PI);
            if (dot(pole, anti) < 0) pole = pole * -1.0;
            const double want = c.ovalRad + 1200 / sys.bodies[au->bi].radiusKm;
            double la, lo; StarSystem::latLonFromBody(normalize(pole * std::cos(want) + normalize(anti - pole * dot(anti, pole)) * std::sin(want)), la, lo);
            if (launch(*au, true, la, lo)) fall(false);
        }
        if (ho) for (int k = 0; k < 10; k++) { if (!launch(*ho, true, (5 + 7 * k) * DEG, (40 * k) * DEG)) break; if (game.testProbeHolePath()) { fall(false); break; } }
        if (ha && launch(*ha, false, 0, 0)) fall(false);
        int counts[5] = {0, 0, 0, 0, 0}; bool depths = true;
        const char* const what[5] = {"FELL INTO RED EYE, THE GREAT STORM", "SAW THE WALL OF THE GREAT STORM", "SAW THE AURORA'S CURTAINS", "FELL THROUGH A CLEAR-AIR HOLE", "MET DIAMOND HAIL"};
        for (const LogEntry& e : game.guide.log) for (int k = 0; k < 5; k++) if (e.kind == "PROBE" && e.text.find(what[k]) != std::string::npos) { counts[k]++; if (k != 1 && k != 2 && e.text.find(" ATM, ") == std::string::npos) depths = false; }
        const std::string stormKey = game.guide.probes.empty() ? std::string() : game.guide.probes[0].storm;
        const bool nameKept = !stormKey.empty() && game.guide.names.count(stormKey) && game.guide.names[stormKey] == "RED EYE";
        check("probe: what it crosses goes to the log with its depth (X-04)", gs && au && ho && ha && counts[0] == 1 && counts[1] == 1 && counts[2] == 1 && counts[3] == 1 && counts[4] == 1 && depths && nameKept,
              fmt("storm %d, wall %d, aurora %d, hole %d, hail %d (each once), depths %d; the storm '%s' named: '%s'", counts[0], counts[1], counts[2], counts[3], counts[4], depths, stormKey.c_str(), named.c_str()));
    }
}
