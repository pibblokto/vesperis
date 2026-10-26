// E-03: the scenes that stand somewhere particular: the stands by a plume, a curtain or a spring, the pinned mountain, the
// fronts, the night sky, the ages' ruins, the dark worlds
#include "harness/harness.h"
#include "harness/test_worlds.h"

// P-04: a kind made only where no system is (the frozen felisian: P-05's wanderers, D's drifters) staged on a world of the scan region: a
// world of its living kind without C-01's people made that kind in its own system, its then-world its own (the climate as drawn), its
// present a rogue's cold (40 K, under the sun it keeps); false where the world is not one to stage it on
bool stageElsewhere(StarSystem& sys, int bi, int type) {
    Body& b = sys.bodies[bi];
    if (!KINDS[type].elsewhere || b.type != KINDS[type].thenKind || b.parent >= 0 || b.locked || peoplesHash(b.type, b.seed)) return false;
    if (sys.star.cls != STAR_YELLOW && sys.star.cls != STAR_ORANGE) return false;   // under a white light, its ground's colours its own
    b.drawnType = b.type; b.type = type; b.thenTempK = b.tempK; b.tempK = 40; b.color = madeKindColour(type, b.seed);
    return true;
}

// P-05: a cryovolcanic world's first active plume of a scan of its cap (from 4 degrees past the cap's edge to the pole, every 10 degrees of
// longitude: `plumeVents`, the active half `drawPlumes` draws), seen from 25 km across the wind (its flag side on)
bool plumeStand(const StarSystem& sys, int bi, Vec3& vent, double& lat, double& lon) {
    const Body& b = sys.bodies[bi];
    const BodyGen g = BodyGen::make(b);
    for (double la = -(g.iceCapLat + 4); la > -84; la -= 3)
        for (int k = 0; k < 36; k++) {
            std::vector<GeyserVent> vents; plumeVents(g, StarSystem::bodyFromLatLon(la * DEG, k * 10 * DEG - PI), vents);
            for (const GeyserVent& v : vents) {
                if (!SurfaceView::plumeActive(v.id)) continue;
                const Vec3 across = normalize(cross(v.unit, prevailingWind(g, v.unit)));
                vent = v.unit;
                StarSystem::latLonFromBody(normalize(v.unit + across * (25.0 / b.radiusKm)), lat, lon);
                return true;
            }
        }
    return false;
}
// P-06: on a tiger-striped moon's second stripe (`tigerStripesOf`) a point 4 km along it from its middle and, 1.5 km off its middle across the
// stripes' plane, where to face that point from: the jets in a line along the stripe receding at an angle, the curtains of the others beyond
bool curtainStand(const StarSystem& sys, int bi, Vec3& vent, double& lat, double& lon) {
    const BodyGen g = BodyGen::make(sys.bodies[bi]);
    if (!(KINDS[g.type].events & EV_CURTAINS)) return false;
    const TigerStripes T = tigerStripesOf(g);
    vent = stripePoint(T, g.R, 1, 4.0);
    const double x = T.mid[1] / g.R, y = (stripeYKm(T, g.R, 1, 0) - 1.5) / g.R;
    const Vec3 st = normalize(T.e1 * x + T.e2 * y + Vec3(0, 0, -std::sqrt(std::max(0.0, 1 - x * x - y * y))));
    StarSystem::latLonFromBody(st, lat, lon);
    return true;
}

// P-05: a dry ice world's northern spring (the hour of the year its season stands nearest -0.25 and rising) and, on the ground of that season, the
// first spider under the frost 4 degrees inside its band (every 10 degrees of longitude: `springJets`); the stand 60 m south of it
bool springStand(const StarSystem& sys, int bi, double& t0, Vec3& vent, double& lat, double& lon) {
    const Body& b = sys.bodies[bi];
    const double year = std::fabs(b.parent >= 0 ? sys.bodies[b.parent].orbitPeriod : b.orbitPeriod);
    double best = 1e9; t0 = 1000.0;
    for (int q = 0; q < 240; q++) {
        const double tq = 1000.0 + year * q / 240.0, s0 = sys.seasonOf(bi, tq), s1 = sys.seasonOf(bi, tq + year / 480.0);
        if (s1 > s0 && std::fabs(s0 + 0.25) < best) { best = std::fabs(s0 + 0.25); t0 = tq; }
    }
    const BodyGen g = BodyGen::make(b, sys.seasonOf(bi, t0));
    for (double inset = 4; inset >= 1; inset -= 1.5)
        for (int k = 0; k < 36; k++) {
            std::vector<GeyserVent> vents; springJets(g, StarSystem::bodyFromLatLon((g.iceCapLat - inset) * DEG, k * 10 * DEG - PI), vents);
            if (vents.empty()) continue;
            vent = vents[0].unit;
            double vla, vlo; StarSystem::latLonFromBody(vent, vla, vlo);
            lat = vla - 60.0 / (b.radiusKm * 1000.0); lon = vlo;
            return true;
        }
    return false;
}

// B-313: the scene `felisian_mountains`: the pinned felisian mountain site of the O6 review (LANDFORM_SITES[0]), standing,
// facing the highest ground within a kilometre, for the walking stability test and the frames
bool setupPinnedMountainScene(double alt, double yawOff, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut) {
    std::vector<StarSystem> systems; std::vector<int> bodyOf;
    forTypeBodies(PT_FELISIAN, 1, false, [&](const StarSystem& sy, int bi) { systems.push_back(sy); bodyOf.push_back(bi); });
    if (systems.empty()) return false;
    sys = systems[0]; int bi = bodyOf[0];
    double lat = PIN_MOUNTAIN_LAT * DEG, lon = PIN_MOUNTAIN_LON * DEG;
    sv.init(&sys, bi, lat, lon, 1000.0);
    double t = findTime(sv.site, alt, 1000.0);
    sv.init(&sys, bi, lat, lon, t);
    SiteMetrics m = measureSite(sv.site, false);
    sv.player.yaw = m.hiYaw + yawOff; sv.player.pitch = pitch;
    sv.player.x = 30; sv.player.z = -20; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);   // away from the capsule at the origin
    nb.update(sv.site.worldPos(t, 0, 0, 0));
    Input in; sv.update(0.016, in, t, false);
    tOut = t;
    return true;
}

// W-03: the fronts' scenes' site: on the first felisian world a flat land site by day under a clear pattern sky (the deck must
// be seen coming), with a strong front (the peak rain over a half) reaching it within thirty days; the pin, or a search of the
// hashed land sites printed to pin
static bool frontSceneSite(StarSystem& sys, int& bi, double& lat, double& lon, FrontForecast& fc) {
    if (!firstBodyOfType(PT_FELISIAN, sys, bi)) return false;
    const Body& b = sys.bodies[bi];
    const double t0 = 1000.0;
    BodyGen g = BodyGen::make(b);
    auto patternCloud = [&](double la, double lo, double T) {   // `computeEnvironment`'s pattern cloud
        double drift = T * TAU / (std::fabs(b.rotPeriod) * 3.7) + 1.0;
        return clampd(sampleCloudPattern(g, lo + drift, la) * 1.3 + 0.25 * gnoise2(T / 900.0, g.seed & 1023, g.seed), 0, 1);
    };
    auto frontOf = [&](double la, double lo, FrontForecast& out) {
        std::vector<FrontForecast> list; frontsAhead(b, StarSystem::bodyFromLatLon(la, lo), t0, 30 * 86400.0, list);
        SurfaceSite probe; probe.init(&sys, bi, la, lo, t0);
        for (const FrontForecast& f : list) {
            if (f.tArrive < t0 || f.strength * f.gain < 0.5) continue;
            if (probe.sun(f.tArrive).altitude < 15 * DEG || probe.sun(f.tClear + 3600).altitude < 10 * DEG) continue;
            if (patternCloud(la, lo, f.tArrive - 4 * 3600) > 0.3 || patternCloud(la, lo, f.tArrive) > 0.3 || patternCloud(la, lo, f.tClear + 3600) > 0.3) continue;
            out = f; return true;
        }
        return false;
    };
    if (PIN_FRONT_LAT != 0 || PIN_FRONT_LON != 0) { lat = PIN_FRONT_LAT * DEG; lon = PIN_FRONT_LON * DEG; return frontOf(lat, lon, fc); }
    std::vector<std::pair<double, double>> sites; landSitesOf(sys, bi, 0x3F, 300, sites);
    int measured = 0;
    for (auto& sl : sites) {
        FrontForecast f;
        if (!frontOf(sl.first, sl.second, f)) continue;
        SurfaceSite st; st.init(&sys, bi, sl.first, sl.second, t0);
        SiteMetrics m = measureSite(st, false);
        measured++;
        if (m.cls > 1 || m.relief16k < 150 || m.material == MAT_WATER || m.material == MAT_SNOW || m.material == MAT_ICE) continue;   // open ground with hills in the distance (the wall must come over something)
        lat = sl.first; lon = sl.second; fc = f;
        printf("  front scene site: pin lat %.3f lon %.3f (%s, %s, %.0f m of relief over 16 km; the front from the %s at t %.0f, strength %.2f, the pattern's cloud %.2f)\n", lat / DEG, lon / DEG, CLASS_NAMES[m.cls], MATERIAL_NAMES[m.material], m.relief16k, frontCompass(f.fromBearing), f.tArrive, f.strength, patternCloud(lat, lon, f.tArrive));
        return true;
    }
    printf("  front scene site: none of %zu sites (%d measured)\n", sites.size(), measured);
    return false;
}
// W-03: the fronts' scenes: the site facing where the front comes from, when the deck's solid wall is 18 km off (0), four
// minutes before its line (1), at the rain's peak 25 km behind it (2), and an hour after the rain's end (3), the sky clearing
bool setupFrontScene(int stage, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut) {
    int bi = -1; double lat = 0, lon = 0; FrontForecast fc;
    if (!frontSceneSite(sys, bi, lat, lon, fc)) { printf("  no site with a front for the scene\n"); return false; }
    const Body& b = sys.bodies[bi];
    std::vector<Front> fs; frontsOf(b, fc.tArrive, fs, 0); const Front* f = nullptr;
    for (const Front& q : fs) if (q.slot == fc.slot && q.epoch == fc.epoch) f = &q;
    Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
    double tFar = fc.tArrive - 3 * 3600, tPeak = fc.tArrive + 20 * 60;
    if (f) {
        double R = b.radiusKm * 1000, want = 90e3 * f->width + 18e3;   // the deck's solid wall 18 km off
        for (double T = fc.tArrive; T > fc.tArrive - 12 * 3600; T -= 60) { double ah, al; frontDistance(*f, T, u, R, ah, al); if (ah >= want) { tFar = T; break; } }
        for (double T = fc.tArrive; T < fc.tClear; T += 60) { double ah, al; frontDistance(*f, T, u, R, ah, al); if (ah <= -25e3 * f->width) { tPeak = T; break; } }   // the rain's peak
    }
    double t = stage == 0 ? tFar : stage == 1 ? fc.tArrive - 4 * 60 : stage == 2 ? tPeak : fc.tClear + 60 * 60;
    sv.init(&sys, bi, lat, lon, t);
    nb.update(sv.site.worldPos(t, 0, 0, 0));
    sv.player.yaw = fc.fromBearing; sv.player.pitch = pitch;
    sv.relocateCapsule(sv.player.x - std::sin(sv.player.yaw) * 14, sv.player.z - std::cos(sv.player.yaw) * 14);   // the capsule behind the view
    Input in; sv.update(0.016, in, t, false);
    printf("  %s at %.3f %.3f: the front from the %s reaches it at t %.0f (strength %.2f, gain %.2f, width %.2f), its rain ends at t %.0f; the frame at t %.0f (%+.0f min from the line), facing %.0f deg\n",
           b.name.c_str(), lat / DEG, lon / DEG, frontCompass(fc.fromBearing), fc.tArrive, fc.strength, fc.gain, f ? f->width : 0.0, fc.tClear, t, (t - fc.tArrive) / 60, sv.player.yaw / DEG);
    tOut = t;
    return true;
}

// W-04: the night scenes, each searched for near home in a fixed order (the systems of sectors 150..200 x 20..80, the bodies in
// order), the place and the moment printed:
// 0 the shower: the first landable world with an atmosphere and a stream of 0.7 or more, at its first peak after t 1000, on the land
//   site where the radiant stands highest with the sun 18 degrees down and the pattern's sky clear, facing the radiant's
//   azimuth and looking 22 degrees under it, the frame at the moment of the peak's four minutes with the most streaks in view
// 1 the zodiacal light: the first landable world with an atmosphere and zodiacal dust of 0.7 or more, on a land site within 20
//   degrees of the equator where the sun stands 20 degrees under the western horizon at t 1000, facing where it set, 12 degrees up
// 2 the earthshine: the first landable planet with a moon of a degree or more across, at the first moment the moon is a crescent
//   (25-50 degrees from the sun) high on a land site with the sun 12-20 degrees down, facing the moon
// 3 the moonlit night: the first landable moon of a gas giant whose parent is near full at the moon's midnight (the sun and the
//   parent more than 150 degrees apart), on a land site of 350 m of relief or more where the parent stands 6-14 degrees up, facing
//   it: M10-09's planetshine, the light `bodyShineAt` gives the ground and the earthshine alike
static double altAt(const StarSystem& sys, int bi, double lat, double lon, double t, const Vec3& dirW) {
    Vec3 up = sys.bodyFrame(bi, t).transposed() * StarSystem::bodyFromLatLon(lat, lon);
    return std::asin(clampd(dot(up, dirW), -1, 1));
}
static Vec3 siteKm(const StarSystem& sys, int bi, double lat, double lon, double t) {
    return sys.bodyPos(bi, t) + (sys.bodyFrame(bi, t).transposed() * StarSystem::bodyFromLatLon(lat, lon)) * sys.bodies[bi].radiusKm;
}
static void faceDir(SurfaceView& sv, double t, const Vec3& dirW, double pitchUnder) {   // pitchUnder < 0: the azimuth only, 12 degrees up
    Vec3 l = sv.site.localFrame(t) * dirW;
    sv.player.yaw = std::atan2(l.x, l.z);
    sv.player.pitch = pitchUnder < 0 ? 12 * DEG : clampd(std::asin(clampd(l.y, -1, 1)) - pitchUnder, -0.6, 60 * DEG);
}
bool setupNightScene(int which, SurfaceView& sv, StarSystem& sysOut, StarNeighborhood& nb, double& tOut) {
    const double t0 = 1000;
    for (int64_t x = 150; x < 200; x++)
        for (int64_t z = 20; z < 80; z++) {
            Star st; if (!starInSector(x, 0, z, st)) continue;
            StarSystem sys; sys.generate(st);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& B = sys.bodies[bi];
                if (!KINDS[B.type].landable) continue;
                const bool atm = KINDS[B.type].atmosphere;
                BodyGen g = BodyGen::make(B);
                auto clearSky = [&](double la, double lo, double T) {   // `computeEnvironment`'s pattern cloud (W-03's front scene)
                    if (!atm) return true;
                    double drift = T * TAU / (std::fabs(B.rotPeriod) * 3.7) + 1.0;
                    return clampd(sampleCloudPattern(g, lo + drift, la) * 1.3 + 0.25 * gnoise2(T / 900.0, g.seed & 1023, g.seed), 0, 1) < 0.25;
                };
                double bestLat = 0, bestLon = 0, bestT = 0, bestScore = -1e9; Vec3 faceW; double under = 0; std::string what;
                std::vector<std::pair<double, double>> sites;
                if (which == 0) {
                    if (B.parent >= 0 || !atm) continue;
                    std::vector<MeteorShower> sh; meteorShowersOf(sys, bi, sh);
                    if (sh.empty() || sh[0].strength < 0.7) continue;
                    const MeteorShower& s = sh[0];
                    double tp = showerPeakAfter(s, t0);
                    Vec3 sunW = normalize(sys.star.pos - sys.bodyPos(bi, tp));
                    { DrainageOff off; landSitesOf(sys, bi, 0x5A0E, 400, sites); }
                    for (auto& p : sites) {
                        double ra = altAt(sys, bi, p.first, p.second, tp, s.radiant), sa = altAt(sys, bi, p.first, p.second, tp, sunW);
                        if (sa > -18 * DEG || !clearSky(p.first, p.second, tp)) continue;
                        if (ra > bestScore) { bestScore = ra; bestLat = p.first; bestLon = p.second; bestT = tp; }
                    }
                    if (bestScore < 35 * DEG) continue;
                    faceW = s.radiant; under = 22 * DEG;
                    what = fmt("the shower of %s (strength %.2f, %.0f a minute at the peak, width %.0f min) at its peak t %.0f, the radiant %.0f deg up", sys.bodies[s.comet].name.c_str(), s.strength, s.strength * SHOWER_PEAK_PER_MIN, s.sigma / 60, tp, bestScore / DEG);
                } else if (which == 1) {
                    if (!atm || zodiacalDust(sys, bi) < 0.7 || sys.companion >= 0) continue;   // a second sun up would keep the sky lit
                    Vec3 sunW = normalize(sys.star.pos - sys.bodyPos(bi, t0));
                    { DrainageOff off; landSitesOf(sys, bi, 0x20D1, 600, sites); }
                    for (auto& p : sites) {
                        if (std::fabs(p.first) > 20 * DEG) continue;
                        double sa = altAt(sys, bi, p.first, p.second, t0, sunW), sLater = altAt(sys, bi, p.first, p.second, t0 + 60, sunW);
                        if (sLater > sa || !clearSky(p.first, p.second, t0)) continue;   // the evening: the sun going down
                        double sc = -std::fabs(sa + 20 * DEG);
                        if (sc > bestScore) { bestScore = sc; bestLat = p.first; bestLon = p.second; bestT = t0; }
                    }
                    if (bestScore < -3 * DEG) continue;
                    faceW = sunW; under = -1;   // where it set, looking 12 degrees up
                    what = fmt("the zodiacal dust %.2f (%s, %zu belts), the sun %.1f deg down", zodiacalDust(sys, bi), STAR_CLASSES[sys.star.cls].name, sys.belts.size(), -altAt(sys, bi, bestLat, bestLon, bestT, sunW) / DEG);
                } else if (which == 2) {
                    if (B.parent >= 0 || B.moonCount == 0) continue;
                    int moon = -1;
                    for (int j = 0; j < (int)sys.bodies.size(); j++) {
                        const Body& M = sys.bodies[j];
                        if (M.parent != bi) continue;
                        if (std::asin(clampd(M.radiusKm / (M.orbitRadiusKm - B.radiusKm), 0, 1)) >= 1.0 * DEG) { moon = j; break; }
                    }
                    if (moon < 0) continue;
                    double P = std::fabs(sys.bodies[moon].orbitPeriod);
                    { DrainageOff off; landSitesOf(sys, bi, 0xEA27, 300, sites); }
                    for (double T = t0; T < t0 + 2 * P && bestScore < 0; T += P / 240) {
                        Vec3 bp = sys.bodyPos(bi, T), mp = sys.bodyPos(moon, T);
                        Vec3 sunW = normalize(sys.star.pos - bp), moonW = normalize(mp - bp);
                        double el = std::acos(clampd(dot(sunW, moonW), -1, 1));
                        if (el < 25 * DEG || el > 50 * DEG) continue;
                        for (auto& p : sites) {
                            double sa = altAt(sys, bi, p.first, p.second, T, sunW);
                            if (sa > -12 * DEG || sa < -20 * DEG || !clearSky(p.first, p.second, T)) continue;
                            Vec3 toMoon = normalize(mp - siteKm(sys, bi, p.first, p.second, T));
                            double ma = altAt(sys, bi, p.first, p.second, T, toMoon);
                            if (ma > 12 * DEG && ma > bestScore) { bestScore = ma; bestLat = p.first; bestLon = p.second; bestT = T; }
                        }
                    }
                    if (bestScore < 12 * DEG) continue;
                    faceW = normalize(sys.bodyPos(moon, bestT) - siteKm(sys, bi, bestLat, bestLon, bestT)); under = 6 * DEG;
                    Vec3 bp = sys.bodyPos(bi, bestT);
                    what = fmt("%s %.0f deg from the sun, %.0f deg up, %.1f deg across", sys.bodies[moon].name.c_str(), std::acos(clampd(dot(normalize(sys.star.pos - bp), normalize(sys.bodyPos(moon, bestT) - bp)), -1, 1)) / DEG, bestScore / DEG,
                               2 * std::asin(clampd(sys.bodies[moon].radiusKm / length(sys.bodyPos(moon, bestT) - siteKm(sys, bi, bestLat, bestLon, bestT)), 0, 1)) / DEG);
                } else {
                    if (B.parent < 0 || sys.bodies[B.parent].type != PT_GASGIANT) continue;
                    double P = std::fabs(B.orbitPeriod);
                    { DrainageOff off; landSitesOf(sys, bi, 0x300E, 400, sites); }
                    for (double T = t0; T < t0 + 2 * P && bestScore < 0; T += P / 360) {
                        Vec3 bp = sys.bodyPos(bi, T), pp = sys.bodyPos(B.parent, T);
                        Vec3 sunW = normalize(sys.star.pos - bp), parW = normalize(pp - bp);
                        if (dot(sunW, parW) > std::cos(150 * DEG)) continue;
                        for (auto& p : sites) {
                            double pa = altAt(sys, bi, p.first, p.second, T, parW), sa = altAt(sys, bi, p.first, p.second, T, sunW);
                            if (sa > -15 * DEG || pa < 6 * DEG || pa > 14 * DEG) continue;   // a low parent, in the frame over the horizon
                            DrainageOff off;
                            SurfaceSite probe; probe.init(&sys, bi, p.first, p.second, T);
                            SiteMetrics m = measureSite(probe, false);
                            if (m.relief16k < 350 || m.material == MAT_WATER) continue;
                            bestScore = m.relief16k; bestLat = p.first; bestLon = p.second; bestT = T; break;
                        }
                    }
                    if (bestScore < 0) continue;
                    Vec3 parW = normalize(sys.bodyPos(B.parent, bestT) - sys.bodyPos(bi, bestT));
                    faceW = parW; under = 4 * DEG;   // toward the parent, low over the horizon
                    what = fmt("under %s, %.0f deg up, the shine %.2f, %.0f m of relief over 16 km", sys.bodies[B.parent].name.c_str(), altAt(sys, bi, bestLat, bestLon, bestT, parW) / DEG,
                               bodyShineAt(sys, B.parent, siteKm(sys, bi, bestLat, bestLon, bestT), bestT, 1.0), bestScore);
                }
                sysOut = sys;
                sv.init(&sysOut, bi, bestLat, bestLon, bestT);
                nb.update(sv.site.worldPos(bestT, 0, 0, 0));
                faceDir(sv, bestT, faceW, under);
                if (which == 0) {   // the moment of the peak's four minutes with the most streaks in view
                    Vec3 view(std::sin(sv.player.yaw) * std::cos(sv.player.pitch), std::sin(sv.player.pitch), std::cos(sv.player.yaw) * std::cos(sv.player.pitch));
                    int most = -1; double tp = bestT;
                    for (double T = tp - 120; T <= tp + 120; T += 1.0 / 30) {
                        std::vector<SurfaceView::ShowerMeteor> ms; sv.showerMeteorsAt(T, ms);
                        int n = 0; for (auto& m : ms) if (dot(m.head, view) > std::cos(25 * DEG) && m.bright > 20) n++;
                        if (n > most) { most = n; bestT = T; }
                    }
                    what += fmt(", %d streaks in view at t %.2f", most, bestT);
                }
                sv.relocateCapsule(sv.player.x - std::sin(sv.player.yaw) * 14, sv.player.z - std::cos(sv.player.yaw) * 14);   // the capsule behind the view
                Input in; sv.update(0.016, in, bestT, false);
                printf("  %s (%s, sector %lld 0 %lld, body %d) at %.3f %.3f, t %.2f: %s; facing %.0f deg, pitch %.0f\n", B.name.c_str(), KINDS[B.type].name, (long long)x, (long long)z, bi,
                       bestLat / DEG, bestLon / DEG, bestT, what.c_str(), sv.player.yaw / DEG, sv.player.pitch / DEG);
                tOut = bestT;
                return true;
            }
        }
    printf("  no place for the night scene %d\n", which);
    return false;
}

// A-03: a scene of an age: the people of its scene world (`AGE_SCENE_WORLDS`), the greatest of its settlements within thirty rings of cells round a land
// point deep on its side (a town before a village before a hamlet before a monument; the far future's one structure), seen from outside its
// edge three quarters along its main street, raised to frame what stands tallest
bool setupAgeScene(int age, double alt, double pitch0, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut) {
    const AgeWorld& W = AGE_SCENE_WORLDS[clampi(age, 0, AGE_COUNT - 1)];
    Star st; if (!starInSector(W.sx, 0, W.sz, st, true)) { printf("  no star at %lld 0 %lld\n", (long long)W.sx, (long long)W.sz); return false; }
    sys.generate(st);
    if (W.body >= (int)sys.bodies.size()) return false;
    const int bi = W.body;
    BodyGen g = BodyGen::make(sys.bodies[bi]);
    double lat = 0, lon = 0;
    {   // T-01: from the people's capital (its plan's first place; a land point deep on its side where the plan has none)
        std::shared_ptr<const WorldPlan> P = worldPlanOf(g);
        const PeoplePlan& pp = P->plans[std::min(W.which, std::max(0, P->peoples - 1))];
        Vec3 u = pp.places.empty() ? peopleSidePoint(g, W.which, 20 * DEG) : pp.places[0].unit;
        StarSystem::latLonFromBody(u, lat, lon);
    }
    sv.init(&sys, bi, lat, lon, 1000.0);
    double sunErr = 0, t = findTime(sv.site, alt, 1000.0, &sunErr);
    int gLat0, gLon0; sv.ruinCellAt(0, 0, gLat0, gLon0);
    struct Cand { Ruin ru; RuinSpec sp; int cls; double d; };
    std::vector<Cand> cands;
    for (int ring = 0; ring < 30; ring++)
        for (int dl = -ring; dl <= ring; dl++)
            for (int dn = -ring; dn <= ring; dn++) {
                if (std::max(std::abs(dl), std::abs(dn)) != ring) continue;
                Ruin ru;
                if (!sv.ruinAt(gLat0 + dl, gLon0 + dn, ru) || ru.spec->people != (sv.peoples > 1 ? W.which : 0)) continue;
                cands.push_back({ru, *ru.spec, settlementRank(ru.spec->sclass), std::hypot(ru.x, ru.z)});
            }
    if (cands.empty()) { printf("  %s: no settlement of the %s people within 30 cells\n", sys.bodies[bi].name.c_str(), AGE_NAMES[age]); return false; }
    std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.cls != b.cls ? a.cls > b.cls : a.d < b.d; });
    // the vantage: of the best eight, twelve directions each, a stand on dry open land from which the ground leaves the line of sight to
    // the settlement clear, the sun to the side or behind, the least forest along the way
    const SunInfo sun = sv.site.sun(t);
    double bestScore = -1e18, bestYaw = 0, bestStandX = 0, bestStandZ = 0, bestDist = 0, hMax = 0; int bestC = -1;
    for (int ci = 0; ci < (int)std::min<size_t>(8, cands.size()); ci++) {
        const Cand& C = cands[ci];
        const Culture& cu = sv.cultures[C.sp.people == 1 && sv.peoples > 1 ? 1 : 0];
        std::vector<Form> el; ruinElements(C.sp, cu, 0, el);
        double reach = C.sp.size, hm = 0;
        for (const Form& e : el) { hm = std::max(hm, formTop(e)); reach = std::max(reach, std::hypot(e.x, e.z) * 0.8); }
        const double cy = sv.site.sampleAt(C.ru.x, C.ru.z, 16).h;
        for (int k = 0; k < 36; k++) {   // twelve directions at three distances: inside its edge (a low settlement is seen from among its ruins), at it, outside
            const double far = age == AGE_FAR_FUTURE ? std::max(reach * 2.4, hm * 1.7) : std::max(reach * 0.85 + 20, hm * 0.62);
            const double D = age >= AGE_FUTURISTIC ? far : (k < 12 ? std::max(reach * 0.55, 25.0) : (k < 24 ? std::max(reach * 0.8, 35.0) : far));
            const double ty = cy + std::min(hm * 0.3, D * 0.35);
            const double a = C.sp.heading + 0.6 + (k % 12) * TAU / 12, sx = C.ru.x + std::sin(a) * D, sz = C.ru.z + std::cos(a) * D;
            TerrainVertex sv0 = sv.site.sampleAt(sx, sz, 16);
            if (sv0.water > -1e8f && sv0.h < sv0.water + 0.3) continue;   // in the water
            bool near = !settlementClear(C.sp, sx - C.ru.x, sz - C.ru.z, 5.0);   // in or against a building
            for (double q = 2; q < 26 && !near; q += 2) { double f = q / D; near = !settlementClear(C.sp, sx - C.ru.x + (C.ru.x - sx) * f, sz - C.ru.z + (C.ru.z - sz) * f, 0.3); }   // a wall right before the eyes
            if (near) continue;
            const double ey = sv0.h + 1.7;
            int blocked = 0, forest = 0, wet = 0;
            for (int q = 1; q < 16; q++) {
                double f = q / 16.0, x = sx + (C.ru.x - sx) * f, z = sz + (C.ru.z - sz) * f;
                TerrainVertex v = sv.site.sampleAt(x, z, 16);
                if (v.h > ey + (ty - ey) * f - 0.5 && f < 0.85) blocked++;
                if (v.material == MAT_FOREST) forest++;
                if (v.water > -1e8f && v.h < v.water) wet++;
            }
            if (blocked) continue;
            const double toSun = std::fabs(std::remainder(std::atan2(C.ru.x - sx, C.ru.z - sz) - sun.azimuth, TAU));   // the view's angle off the sun's azimuth
            const double score = C.cls * 10.0 - forest * 2.0 - wet * 0.5 - (toSun < 1.0 ? 4.0 : 0.0) + std::min(3.0, (sv0.h - cy) * 0.2) - ci * 0.3 + (age == AGE_PREHISTORIC ? (k < 12 ? 1.5 : 0.0) : (k >= 12 && k < 24 ? 1.5 : 0.0));   // a camp's low remains from among them
            if (score > bestScore) { bestScore = score; bestC = ci; bestYaw = std::atan2(C.ru.x - sx, C.ru.z - sz); bestStandX = sx; bestStandZ = sz; bestDist = D; hMax = hm; }
        }
    }
    if (bestC < 0) { printf("  %s: no vantage over any of the %zu settlements of the %s people\n", sys.bodies[bi].name.c_str(), cands.size(), AGE_NAMES[age]); return false; }
    const RuinSpec bestSpec = cands[bestC].sp; const double bestD = cands[bestC].d, D = bestDist;
    const Culture& cu = sv.cultures[bestSpec.people == 1 && sv.peoples > 1 ? 1 : 0];
    double la2, lo2; sv.site.latLonAt(bestStandX, bestStandZ, la2, lo2);
    sv.init(&sys, bi, la2, lo2, t);
    sv.player.x = 0; sv.player.z = 0; sv.player.y = sv.site.surfaceHeight(0, 0);
    const double yaw = bestYaw;
    sv.player.yaw = yaw; sv.player.pitch = std::max(pitch0, std::atan2(std::min(hMax * 0.3, D * 0.35) - 1.7, D));
    sv.relocateCapsule(-std::sin(yaw) * 14, -std::cos(yaw) * 14);
    nb.update(sv.site.worldPos(t, 0, 0, 0));
    Input in; sv.update(0.016, in, t, false);
    std::map<std::string, int> kinds; for (const Building& b : bestSpec.buildings) kinds[BUILDING_KIND_NAMES[b.kind]]++;
    std::string ks; for (auto& k : kinds) ks += fmt("%s%d %s", ks.empty() ? "" : ", ", k.second, k.first.c_str());
    printf("  %s of %s (sector %lld 0 %lld), the %s people: a %s of %zu buildings (%s), radius %.0f m, the tallest %.0f m, %.1f km from the side's point; ended %.0f years ago, wet %.2f; seen from %.0f m\n",
           sys.bodies[bi].name.c_str(), st.name.c_str(), (long long)W.sx, (long long)W.sz, AGE_NAMES[cu.age], SETTLEMENT_CLASS_NAMES[bestSpec.sclass], bestSpec.buildings.size(), ks.c_str(), bestSpec.size, hMax, bestD / 1000.0, cu.ageYears, cu.wet, D);
    tOut = t;
    return true;
}

// D-04: the drifters' ground in the dark (`scene <name>`), each on the nearest drifter from home that shows it (no sun: any time is its night):
// 0 rogue_lamp on a rocky drifter at 20 N 40 E, the explorer's lamp on (the runner draws it off too); 1 rogue_embers 220 m from a lava lake of a
// young interior's fields (a rock drifter's EMBERS), facing it, the lamp off; 2 rogue_frozen_air on a desert drifter whose air fell as snow, the
// lamp on, facing the highest ground within two kilometres; 3 dark_ocean on a dark ocean's black sand by its sea (one with lights in its water
// first), the lamp on, facing the most water
bool setupDarkScene(int which, double pitch, SurfaceView& sv, StarSystem& sys, StarNeighborhood& nb, double& tOut) {
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
    auto ang0 = [](int a) { const double ang = a * TAU / 16; return std::atan2(std::sin(ang), std::cos(ang)); };   // a way's yaw (east of north: x = sin, z = cos)
    std::vector<Star> near; driftersNear(home.pos, 4000, near);
    struct DrainOff { bool was; DrainOff() : was(drainageEnabled()) { setDrainageEnabled(false); } ~DrainOff() { setDrainageEnabled(was); } };
    const double t = 1000.0;
    for (int pass = 0; pass < 2; pass++)
        for (const Star& d0 : near) {
            Star d; if (!drifterInSector(d0.sx, d0.sy, d0.sz, d, true)) continue;
            sys.generate(d);
            const Body& b = sys.bodies[0];
            if (!KINDS[b.type].landable) continue;
            const BodyGen g = BodyGen::make(b);
            const bool want = which == 0 ? (pass == 0 && b.type == PT_ROCKY) : which == 1 ? (pass == 0 && g.hasTrait(TR_EMBERS) && !isLavaWorld(b.type))
                            : which == 2 ? (g.hasTrait(TR_FROZEN_AIR) && (pass == 1 || b.type == PT_DESERT)) : (b.type == PT_DARK_OCEAN && (pass == 1 || g.hasTrait(TR_LIGHTS_IN_WATER)));
            if (!want) continue;
            double lat = 20 * DEG, lon = 40 * DEG, yaw = 0.6, Rm = b.radiusKm * 1000.0;
            bool lookAtWater = false;
            if (which == 1) {   // a lake of the strongest fields: a 250 m grid over a field's middle, the first lava sample, the site 220 m south of it
                std::vector<EmberField> fields; emberFieldsOf(g, fields);
                std::sort(fields.begin(), fields.end(), [](const EmberField& a, const EmberField& c) { return a.strength > c.strength; });
                bool found = false;
                DrainOff off;
                for (size_t f = 0; f < fields.size() && f < 6 && !found; f++) {
                    const Vec3 c = fields[f].unit, e1 = normalize(cross(c, std::fabs(c.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), e2 = cross(c, e1);
                    const double spanKm = std::min(15.0, 0.5 * fields[f].radiusKm);
                    for (double a = -spanKm; a <= spanKm && !found; a += 0.25)
                        for (double q = -spanKm; q <= spanKm && !found; q += 0.25) {
                            const Vec3 u = normalize(c * b.radiusKm + e1 * a + e2 * q);
                            if (sampleSurface(g, u, 16).material != MAT_LAVA) continue;
                            double la, lo; StarSystem::latLonFromBody(u, la, lo);
                            double off = 0;   // south to the lava's edge, then onto the rampart beyond it, facing north across the lake
                            for (; off < 800 && sampleSurface(g, StarSystem::bodyFromLatLon(la - off / Rm, lo), 4).material == MAT_LAVA; off += 4) {}
                            lat = la - (off + 14.0) / Rm; lon = lo; yaw = 0.0; found = true;
                        }
                }
                if (!found) continue;
            } else if (which == 3) {   // black sand 0.3-5 m over the sea with water within 150 m one way (the beach finder's rule), the one with the most
                // lights in its water within 60 m (pass 0 asks for two at least)
                bool found = false; int mostLit = -1;
                DrainOff off;
                for (double la = 0; la < 70 && mostLit < 4; la += 0.5)
                    for (int k = 0; k < 720 && mostLit < 4; k++) {
                        const double ln = k * TAU / 720;
                        const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16);
                        if (ss.material != MAT_SAND || ss.height < 0.3 || ss.height > 5) continue;
                        int best = 0, bestLit = 0; double bestYaw = 0;   // the way with the most water within 150 m (pass 0: with lights in it within 60 m)
                        for (int a = 0; a < 16; a++) {
                            int cnt = 0, lit = 0;
                            for (int dd = 10; dd <= 150; dd += 10) {
                                const double ang = a * TAU / 16, dlat = std::cos(ang) * dd / Rm, dlon = std::sin(ang) * dd / (Rm * std::cos(la * DEG));
                                const SurfaceSample w = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), 16);
                                if (w.material != MAT_WATER) continue;
                                if (dd >= 20) cnt++;
                                if (dd <= 60 && w.waterTint > 0.35) lit++;
                            }
                            if (cnt >= 6 && (pass == 1 ? cnt > best : lit > bestLit)) { best = cnt; bestLit = lit; bestYaw = ang0(a); }
                        }
                        if (best >= 6 && (pass == 1 || bestLit >= 2) && bestLit > mostLit) { lat = la * DEG; lon = ln; yaw = bestYaw; found = true; mostLit = bestLit; }
                    }
                if (!found) continue;
                lookAtWater = true;
            }
            sv.init(&sys, 0, lat, lon, t);
            if (which == 2) {   // facing the highest ground within two kilometres
                double best = -1e18;
                for (int a = 0; a < 16; a++) { const double ya = a * TAU / 16, h = sv.site.sampleAt(std::sin(ya) * 1500, std::cos(ya) * 1500, 64).h; if (h > best) { best = h; yaw = ya; } }
            }
            if (lookAtWater) {   // down the beach to four metres from the water's edge
                double dd = 0;
                while (dd < 150 && sv.site.sampleAt(std::sin(yaw) * dd, std::cos(yaw) * dd, 4).material != MAT_WATER) dd += 2;
                dd = std::max(0.0, dd - 4);
                sv.player.x = std::sin(yaw) * dd; sv.player.z = std::cos(yaw) * dd; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
            }
            sv.player.yaw = yaw; sv.player.pitch = pitch;
            sv.relocateCapsule(sv.player.x - std::sin(yaw) * 14, sv.player.z - std::cos(yaw) * 14);   // behind the view
            sv.lamp = which != 1;
            nb.update(sv.site.worldPos(t, 0, 0, 0));
            Input in; sv.update(0.016, in, t, false);
            printf("  the drifter %s (%s) %.0f ly from home at sector %lld %lld %lld; the site %.2f %.2f\n", d.name.c_str(), KINDS[b.type].name, length(d.pos - home.pos) / SECTOR_KM, (long long)d.sx, (long long)d.sy, (long long)d.sz, lat / DEG, lon / DEG);
            if (which == 3) {   // the lights in the water ahead (the samples' tint, the drawn vertices')
                std::string row;
                for (int dd = 10; dd <= 150; dd += 20) {
                    const double x = sv.player.x + std::sin(yaw) * dd, z = sv.player.z + std::cos(yaw) * dd;
                    const SurfaceSample w = sampleSurface(sv.site.gen, sv.site.unitAt(x, z), 16);
                    row += fmt(" %dm:%s/%.2f", dd, MATERIAL_NAMES[w.material], w.waterTint);
                }
                printf("  ahead:%s\n", row.c_str());
            }
            tOut = t;
            return true;
        }
    return false;
}
