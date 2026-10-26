// E-03: the unit's first checks and the noise (groups `noise`, `basics`)
#include "harness/harness.h"

// O6-02, B-404, N1-01: the noise's gradient, the rings' materials agree, the relief, the palette
void unitNoise(const Check& check) {
    {   // O6-02: the analytic gradient of gnoise3d matches central differences and its value is gnoise3's
        double worst = 0, worstV = 0; Rng r(0x616);
        for (int i = 0; i < 200; i++) {
            double x = r.range(-50, 50), y = r.range(-50, 50), z = r.range(-50, 50); uint64_t sd = r.next();
            Vec3 g; double v = gnoise3d(x, y, z, sd, g);
            worstV = std::max(worstV, std::fabs(v - gnoise3(x, y, z, sd)));
            const double e = 1e-5;
            Vec3 fd((gnoise3(x + e, y, z, sd) - gnoise3(x - e, y, z, sd)) / (2 * e), (gnoise3(x, y + e, z, sd) - gnoise3(x, y - e, z, sd)) / (2 * e), (gnoise3(x, y, z + e, sd) - gnoise3(x, y, z - e, sd)) / (2 * e));
            worst = std::max(worst, length(fd - g));
        }
        check("gnoise3d value equals gnoise3", worstV < 1e-12, "worst " + std::to_string(worstV));
        check("gnoise3d gradient by finite differences", worst < 1e-5, "worst " + std::to_string(worst));
    }
    {   // B-404: the walked rings (4, 16 and 64 m) decide the same materials: a material follows no faded value, so no
        // patch ends at a ring's edge in front of the explorer (the europan stain, the lava tongues, the cliffs' rock).
        // The felisian and desert allowances are the drainage's lines, a cell and a half wide at every scale (KI-341)
        struct Want { int type; double max4, max16; };
        static const Want wants[] = {{PT_EUROPAN, 0.05, 0.05}, {PT_VOLCANIC, 0.2, 0.2}, {PT_DESERT, 0.6, 5.0}, {PT_FELISIAN, 0.6, 6.0}};
        for (const Want& w : wants) {
            StarSystem sys; int bi;
            if (!firstBodyOfType(w.type, sys, bi)) { check("B-404 materials agree across the rings", false, std::string(KINDS[w.type].name) + ": no body"); continue; }
            std::vector<std::pair<double, double>> sites; landSitesOf(sys, bi, 0x4A, 2, sites);
            double worst[2] = {0, 0};
            for (auto& st : sites) { double o[2]; materialAgreement(sys, bi, st.first, st.second, 1500.0, 100.0, o); worst[0] = std::max(worst[0], o[0]); worst[1] = std::max(worst[1], o[1]); }
            char buf[200]; snprintf(buf, sizeof buf, "%s (%s): 4/16 %.2f%% of %.2f, 16/64 %.2f%% of %.2f", KINDS[w.type].name, sys.bodies[bi].name.c_str(), worst[0], w.max4, worst[1], w.max16);
            check("B-404 materials agree across the rings", worst[0] <= w.max4 && worst[1] <= w.max16, buf);
        }
    }
    {   // O6-02: the relief spectrum is deterministic and its analytic slope tracks the finite difference of its height
        Body b; b.type = PT_FELISIAN; b.seed = 0xC0FFEEULL; b.radiusKm = 5000; b.tempK = 285; b.color = RGB(0.3f, 0.5f, 0.4f);
        BodyGen g = BodyGen::make(b);
        Rng r(0x617); double sumErr = 0, sumFd = 0; bool det = true;
        for (int i = 0; i < 24; i++) {
            Vec3 u = normalize(Vec3(r.sym(1), r.sym(1), r.sym(1)));
            Relief r1 = reliefAt(g, u, 1400, 0.5, 16), r2 = reliefAt(g, u, 1400, 0.5, 16);
            if (r1.h != r2.h || r1.slope != r2.slope || std::fabs(r1.h) > 1400 * 4) det = false;
            Vec3 e = normalize(cross(u, std::fabs(u.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0))), n = cross(u, e);
            double dm = 2.0 / (g.R * 1000);
            double hx = (reliefAt(g, normalize(u + e * dm), 1400, 0.5, 16).h - reliefAt(g, normalize(u - e * dm), 1400, 0.5, 16).h) / 4.0;
            double hz = (reliefAt(g, normalize(u + n * dm), 1400, 0.5, 16).h - reliefAt(g, normalize(u - n * dm), 1400, 0.5, 16).h) / 4.0;
            double fd = std::sqrt(hx * hx + hz * hz);
            sumErr += std::fabs(fd - r1.slope); sumFd += fd;
        }
        check("relief deterministic and bounded", det);
        check("relief slope tracks finite differences", sumErr < 0.4 * sumFd, "mean error " + std::to_string(sumErr / 24) + " of " + std::to_string(sumFd / 24));
    }
    // N1-01: every material of every type gets a colour, the same for the same seed
    {
        bool ok = true; std::string why;
        for (int ty = 0; ty < PT_COUNT && ok; ty++) {
            Body b; b.type = ty; b.seed = 0xABCD1234ULL + ty; b.radiusKm = 3000; b.tempK = 280;
            b.color = RGB(0.3f + 0.03f * (ty % 20), 0.62f, 0.5f);   // R-307: under 1 (P-00: for any number of kinds)
            BodyGen g1 = BodyGen::make(b), g2 = BodyGen::make(b);
            for (int mm = 0; mm < MAT_COUNT && ok; mm++) {
                const RGB& c = g1.matColor[mm];
                if (!(c.r >= 0 && c.g >= 0 && c.b >= 0 && c.r + c.g + c.b > 0.05 && c.r <= 1.001 && c.g <= 1.001 && c.b <= 1.001)) { ok = false; why = std::string(KINDS[ty].name) + "/" + MATERIAL_NAMES[mm]; }
                if (std::fabs(c.r - g2.matColor[mm].r) > 1e-9 || std::fabs(c.g - g2.matColor[mm].g) > 1e-9) { ok = false; why = "not deterministic"; }
            }
        }
        check("material palette per type", ok, why);
        check("noon exposure rule", noonStop(0.4) > 37 && noonStop(0.4) < 38 && noonStop(1.0) == 60 && noonStop(0.0) == 25, std::to_string(noonStop(0.4)));
        RGB rc = materialRampColor(RGB(1, 0.5f, 0), 47);
        check("material ramp hits c at 47", std::fabs(rc.r - 1) < 1e-6 && std::fabs(rc.g - 0.5) < 1e-6, "");
    }
}

// the first checks: the sphere, Kepler, the noise, names, the landing, the globes, the rings, the drone
void unitBasics(const Check& check) {
    // lat/lon round trips on the sphere
    {
        double worst = 0;
        for (int i = 0; i < 200; i++) {
            double lat = (i / 200.0 - 0.5) * PI * 0.98, lon = std::fmod(i * 0.7, TAU) - PI;
            double la2, lo2; StarSystem::latLonFromBody(StarSystem::bodyFromLatLon(lat, lon), la2, lo2);
            worst = std::max(worst, std::fabs(la2 - lat) + std::fabs(wrapAngle(lo2 - lon)));
        }
        check("lat/lon round trip", worst < 1e-9, "worst " + std::to_string(worst));
    }
    Star s; StarSystem sys; int bi = -1;
    for (int64_t x = 150; x < 300 && bi < 0; x++)
        for (int64_t z = 20; z < 120 && bi < 0; z++) {
            if (!starInSector(x, 0, z, s)) continue;
            sys.generate(s);
            for (int i = 0; i < (int)sys.bodies.size(); i++) if (KINDS[sys.bodies[i].type].landable && sys.bodies[i].parent < 0 && sys.bodies[i].ecc < 0.01) { bi = i; break; }
        }
    check("a landable planet found", bi >= 0);
    if (bi >= 0) {
        // the site frame: local metres -> unit vector -> local metres
        SurfaceSite site; site.init(&sys, bi, 0.4, 1.1, 0.0);
        double worst = 0;
        for (int i = 0; i < 100; i++) {
            double x = (i - 50) * 137.0, z = (i * 71) % 5000 - 2500.0, x2, z2;
            site.localAt(site.unitAt(x, z), x2, z2);
            worst = std::max(worst, std::fabs(x2 - x) + std::fabs(z2 - z));
        }
        check("site local/unit round trip", worst < 1e-6, "worst m " + std::to_string(worst));
        // the sun: at the sub-solar point it stands at the zenith; 30 degrees away along the meridian at 60 degrees
        double t = 5000.0;
        Vec3 sunBody = sys.bodyFrame(bi, t) * normalize(sys.star.pos - sys.bodyPos(bi, t));
        double slat, slon; StarSystem::latLonFromBody(sunBody, slat, slon);
        SurfaceSite sub; sub.init(&sys, bi, slat, slon, t);
        double altZen = sub.sun(t).altitude / DEG;
        double lat2 = slat + 30 * DEG; bool north = lat2 < 85 * DEG; if (!north) lat2 = slat - 30 * DEG;
        SurfaceSite off; off.init(&sys, bi, lat2, slon, t);
        double alt30 = off.sun(t).altitude / DEG, az30 = off.sun(t).azimuth / DEG;
        check("sun at the zenith over the sub-solar point", std::fabs(altZen - 90) < 0.05, "alt " + std::to_string(altZen));
        check("sun at 60 deg 30 deg along the meridian", std::fabs(alt30 - 60) < 0.05, "alt " + std::to_string(alt30) + " az " + std::to_string(az30));
        check("sun due south/north from there", north ? std::fabs(az30 - 180) < 0.5 : (az30 < 0.5 || az30 > 359.5), "az " + std::to_string(az30));
        // local noon: dayFraction 0.5 at the sub-solar longitude
        check("local time at the sub-solar point is noon", std::fabs(sub.sun(t).dayFraction - 0.5) < 0.002, std::to_string(sub.sun(t).dayFraction));
    }
    // Kepler solver residuals
    {
        double worst = 0;
        for (int i = 0; i < 400; i++) {
            double e = (i % 20) / 20.0 * 0.95, M = (i / 20) * 0.31;
            double rOverA; double nu = StarSystem::trueAnomaly(M, e, rOverA);
            double E = 2 * std::atan2(std::sqrt(1 - e) * std::sin(nu / 2), std::sqrt(1 + e) * std::cos(nu / 2));
            double resid = std::fabs(wrapAngle(E - e * std::sin(E) - M));
            worst = std::max(worst, resid);
            worst = std::max(worst, std::fabs(rOverA - (1 - e * std::cos(E))));
        }
        check("Kepler equation residual", worst < 1e-8, "worst " + std::to_string(worst));
        double r0; check("circular orbit is the identity", std::fabs(StarSystem::trueAnomaly(1.234, 0, r0) - 1.234) < 1e-12 && r0 == 1);
    }
    // noise statistics: zero mean, bounded, hash uniform
    {
        double sum = 0, lo = 1e9, hi = -1e9; int n = 0;
        for (int i = 0; i < 20000; i++) { double v = gnoise3(i * 0.371, i * 0.173 + 5, i * 0.091 - 3, 77); sum += v; lo = std::min(lo, v); hi = std::max(hi, v); n++; }
        check("gnoise3 mean near zero", std::fabs(sum / n) < 0.03, std::to_string(sum / n));
        check("gnoise3 within [-1.1, 1.1]", lo > -1.1 && hi < 1.1, std::to_string(lo) + ".." + std::to_string(hi));
        double hs = 0, hs2 = 0;
        for (int i = 0; i < 100000; i++) { double u = unitFromHash(mix64(i * 0x9E3779B97F4A7C15ULL + 11)); hs += u; hs2 += u * u; }
        double mean = hs / 100000, var = hs2 / 100000 - mean * mean;
        check("hash uniform (mean 0.5, var 1/12)", std::fabs(mean - 0.5) < 0.005 && std::fabs(var - 1.0 / 12) < 0.003, std::to_string(mean) + " " + std::to_string(var));
        double a = fbm3(Vec3(1.5, 2.5, 3.5), 9, 5), b = fbm3(Vec3(1.5, 2.5, 3.5), 9, 5);
        check("noise is deterministic", a == b);
    }
    // names: bounded length, letters and spaces only, deterministic
    {
        size_t lo = 99, hi = 0; bool clean = true;
        for (int i = 0; i < 3000; i++) {
            std::string nm = generateName(mix64(i + 1), i % 3);
            lo = std::min(lo, nm.size()); hi = std::max(hi, nm.size());
            for (char ch : nm) if (!(isalpha((unsigned char)ch) || ch == ' ' || ch == '\'')) clean = false;
        }
        check("name lengths 3..16", lo >= 3 && hi <= 16, std::to_string(lo) + ".." + std::to_string(hi));
        check("names are letters, spaces, apostrophes", clean);
        check("names deterministic", generateName(42, 1) == generateName(42, 1));
    }
    // B-205 / R-204: a new landing starts with no buggy, B replaces a buggy left anywhere, the nose camera cannot look back
    if (bi >= 0) {
        SurfaceView sv;
        sv.init(&sys, bi, 0.4, 1.1, 0.0);
        sv.relocateCapsule(sv.player.x + 4, sv.player.z + 2);
        check("buggy deploys at the capsule", sv.deployBuggy() && sv.buggy.deployed);
        sv.buggy.unfold = 1; sv.buggy.x += 300; sv.buggy.odometer = 500;   // as if left 300 m away after a drive
        check("B replaces a buggy left far away", sv.deployBuggy() && sv.buggy.odometer == 0 && sv.buggyDist() < 12, "dist " + std::to_string(sv.buggyDist()));
        sv.buggy.unfold = 1; sv.player.x = sv.buggy.x + 1.5; sv.player.z = sv.buggy.z;
        check("get in", sv.toggleBuggy());
        Input in; sv.player.yaw = sv.buggy.heading + PI; sv.player.pitch = -80 * DEG;
        sv.update(1.0 / 60, in, 0.0, true);
        double look = std::fabs(wrapAngle(sv.player.yaw - sv.buggy.heading));
        check("nose camera pans 70 deg at most", look <= SurfaceView::CAM_PAN + 1e-9 && sv.player.pitch >= -SurfaceView::CAM_TILT_DOWN - 1e-9, "look " + std::to_string(look / DEG) + " tilt " + std::to_string(sv.player.pitch / DEG));
        sv.init(&sys, bi, 0.5, 1.2, 0.0);
        check("no buggy on a new landing", !sv.buggy.deployed && !sv.inBuggy);
    }
    // B-303: a new landing clears the waypoint; a re-anchor carries it to the new frame at the same place (R-414: M4-02's trail is gone)
    if (bi >= 0) {
        SurfaceView sv;
        sv.init(&sys, bi, 0.4, 1.1, 0.0);
        Input in;
        for (int i = 0; i < 40; i++) { sv.player.x += 30; sv.update(1.0 / 30, in, i / 30.0, false); }
        sv.hasWaypoint = true; sv.wpX = 900; sv.wpZ = -400;
        Vec3 wpU = sv.site.unitAt(sv.wpX, sv.wpZ);
        sv.player.x = 51e3; sv.update(1.0 / 30, in, 2.0, false);   // beyond 50 km: reanchor
        double wx, wz; sv.site.localAt(wpU, wx, wz);
        check("reanchor keeps the waypoint in place", sv.hasWaypoint && std::fabs(wx - sv.wpX) < 0.05 && std::fabs(wz - sv.wpZ) < 0.05 && std::fabs(sv.wpX) > 1000, "wp " + std::to_string(sv.wpX) + "," + std::to_string(sv.wpZ));
        int epoch = sv.siteEpoch;
        sv.init(&sys, bi, 0.5, 1.2, 0.0);
        check("a new landing clears the waypoint", !sv.hasWaypoint && sv.siteEpoch > epoch);
    }
    // B-206: on a thin-atmosphere world in a dust storm the wind stays a wind (it compounded every frame to 1e13 knots)
    {
        Star ts; StarSystem tsys; int tb = -1;
        for (int64_t x = 150; x < 300 && tb < 0; x++)
            for (int64_t z = 20; z < 120 && tb < 0; z++) {
                if (!starInSector(x, 0, z, ts)) continue;
                tsys.generate(ts);
                for (int i = 0; i < (int)tsys.bodies.size(); i++) if (tsys.bodies[i].type == PT_THINATMO) { tb = i; break; }
            }
        check("a thin-atmosphere world found", tb >= 0);
        if (tb >= 0) {
            SurfaceView sv;
            double t0 = 0; double dustAt = 0;
            for (double tt = 0; tt < 4e5 && dustAt < 0.3; tt += 300) { sv.init(&tsys, tb, 0.3, 0.9, tt); dustAt = sv.env.dust; t0 = tt; }
            check("a dust storm found", dustAt >= 0.3, "dust " + std::to_string(dustAt));
            Input in; double worst = 0;
            for (int i = 0; i < 300; i++) { sv.update(1.0 / 30, in, t0 + i / 30.0, true); worst = std::max(worst, sv.env.windKnots); }
            check("storm wind under 200 knots", std::isfinite(worst) && worst < 200 && worst > 5, "worst " + std::to_string(worst));
        }
    }
    // the classic case: a body's rotation period sets its day; seasonOf is bounded
    if (bi >= 0) {
        double mn = 2, mx = -2;
        for (int k = 0; k < 64; k++) { double v = sys.seasonOf(bi, k * sys.bodies[bi].orbitPeriod / 64); mn = std::min(mn, v); mx = std::max(mx, v); }
        check("seasonOf bounded by the tilt", mx <= std::sin(sys.bodies[bi].axialTilt) + 1e-9 && mn >= -std::sin(sys.bodies[bi].axialTilt) - 1e-9);
    }
    // M7-02: the map cache evicts the least recently used map once the cap is reached
    {
        SpaceRenderer sr;
        sr.mapCacheCap = 3;
        int n = 0;
        for (auto& [x, z, st, sy] : scanSystems(150, 300, 20, 120, [&] { return n < 6; })) {
            for (const Body& b : sy.bodies) { if (n >= 6) break; if (b.type == PT_COMPANION) continue; sr.mapFor(b); n++; }
        }
        check("map cache honours its cap", sr.mapsHeld() <= 3 && sr.mapStats.evicted >= 3, std::to_string(sr.mapsHeld()) + " held, " + std::to_string(sr.mapStats.evicted) + " evicted");
    }
    // O0-02 (B-305): 32 banks, and the second world's families have their own
    check("32 palette banks of 2048 intensities", BANKS == 32 && INTEN_MASK == 2047 && INTEN_PER_SHADE == 32 && (BANK_MASK >> BANK_SHIFT) == 31);
    check("body B's sand has its own bank", SpaceRenderer::globeBank(3, PT_FELISIAN, MAT_SAND) == 17 && SpaceRenderer::globeBank(2, PT_FELISIAN, MAT_SAND) == 13 && SpaceRenderer::globeBank(3, PT_FELISIAN, MAT_SNOW) == 18 && SpaceRenderer::globeBank(3, PT_FELISIAN, MAT_FOREST) == 16);
    // O0-03 (B-304): a globe whose centre is behind the camera plane still draws its limb
    if (bi >= 0) {
        SpaceRenderer sr; StarNeighborhood nb;
        double t = 3000.0;
        const Body& b = sys.bodies[bi];
        Vec3 bp = sys.bodyPos(bi, t);
        Vec3 dir = normalize(Vec3(0.3, 0.2, 0.93));
        Vec3 shipPos = bp + dir * (1.15 * b.radiusKm);   // the disc is 121 degrees across from here
        Vec3 toBody = -dir, side = normalize(cross(toBody, Vec3(0, 1, 0)));
        Vec3 fwd = normalize(toBody * std::cos(92 * DEG) + side * std::sin(92 * DEG));   // the centre 92 degrees off the view axis: behind the camera plane, the limb 31 degrees off
        nb.update(shipPos);
        Framebuffer fb; SpaceContext c;
        c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
        c.bankBodyA = bi;
        warmMaps(sr, sys, bi);
        sr.setupPalette(fb, &sys, bi, -1, 1.0);
        sr.render(fb, c);
        int lit = 0;
        for (Pix p : fb.idx) { int bk = bankOf(p); if ((bk == 2 || bk == 6 || bk == 11 || bk == 13 || bk == 14 || bk == 15) && intenOf(p) > 0) lit++; }
        check("a globe with its centre behind the camera still shows its limb", lit > 200, std::to_string(lit) + " pixels");
    }
    // B-308: a companion star beside or behind the camera never floods the frame; ahead it is a disc of the right size
    {
        Star cs; StarSystem csys; int ck = -1;
        for (int64_t x = 150; x < 320 && ck < 0; x++)
            for (int64_t z = 20; z < 140 && ck < 0; z++) {
                if (!starInSector(x, 0, z, cs)) continue;
                NebulaPatch np[16]; if (nebulaPatches(Vec3(x + 0.5, 0.5, z + 0.5), np, 16) > 0) continue;   // G-01: a sky without nebula patches, which would raise the mean shade (the test measures the companion's flood)
                csys.generate(cs); if (csys.companion >= 0) ck = csys.companion;
            }
        check("a system with a companion star found", ck >= 0);
        if (ck >= 0) {
            SpaceRenderer sr; StarNeighborhood nb; double t = 3000.0;
            const Body& k = csys.bodies[ck];
            Vec3 kp = csys.bodyPos(ck, t);
            Vec3 dir = normalize(Vec3(0.3, 0.2, 0.93));
            Vec3 shipPos = kp + dir * (3.5 * k.radiusKm);   // where the ship parks (Game::parkDistanceFor)
            Vec3 toStar = -dir, side = normalize(cross(toStar, Vec3(0, 1, 0)));
            nb.update(shipPos);
            double angR = std::asin(1.0 / 3.5);
            double expect = PI * std::pow(sr.proj.f * std::tan(angR), 2) / (FBW * FBH);   // the disc's share of the frame when centred
            const int offs[4] = {0, 70, 95, 150};
            std::vector<uint32_t> strip((size_t)FBW * 4 * FBH, 0xFF101010u), px((size_t)FBW * FBH);
            double frac[4] = {0}, mean[4] = {0};
            for (int i = 0; i < 4; i++) {
                double a = offs[i] * DEG;
                Vec3 fwd = normalize(toStar * std::cos(a) + side * std::sin(a));
                Framebuffer fb; SpaceContext c;
                c.sys = &csys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
                sr.setupPalette(fb, &csys, -1, -1, 1.0);
                sr.render(fb, c);
                int disc = 0; double sum = 0;
                for (Pix q : fb.idx) { if (bankOf(q) == 12 && shadeOf(q) > 25) disc++; sum += shadeOf(q); }
                frac[i] = (double)disc / (FBW * FBH); mean[i] = sum / (FBW * FBH);
                fb.mush(2); fb.toRGB(px.data(), 1.0);
                for (int y = 0; y < FBH; y++) memcpy(&strip[(size_t)y * FBW * 4 + i * FBW], &px[(size_t)y * FBW], FBW * 4);
            }
            writePNG("shots/tests/unit_companion_swing.png", strip.data(), FBW * 4, FBH);
            check("the companion ahead is a disc of the expected size", frac[0] > 0.5 * expect && frac[0] < 1.6 * expect, fmt("%.1f%% of the frame, expected %.1f%%", frac[0] * 100, expect * 100));
            check("the companion 70 deg off axis leaves no disc", frac[1] == 0 && mean[1] < 8, fmt("disc %.2f%%, mean shade %.1f", frac[1] * 100, mean[1]));
            // G-01: measured against the 70-degree frame, so the galaxy band's own brightness (now the whole disc, by direction) does not count
            check("the companion beside the camera (95 deg) does not flood", frac[2] == 0 && mean[2] < mean[1] + 1.5, fmt("disc %.2f%%, mean shade %.1f against %.1f at 70 deg", frac[2] * 100, mean[2], mean[1]));
            check("the companion behind (150 deg) does not flood", frac[3] == 0 && mean[3] < mean[1] + 1.5, fmt("disc %.2f%%, mean shade %.1f against %.1f at 70 deg", frac[3] * 100, mean[3], mean[1]));
        }
    }
    // B-425: a companion star keeps clear of the other sun's worlds. Over the home cube and a cube of the core: a far companion at its closest
    // approach spans 3 degrees at most of the sky of the primary's farthest world and its light alone warms it to 100 K at most, its own worlds
    // keep within a third of that gap and see the primary 3 degrees wide at most; a close one shines a tenth of the primary at most (a black
    // hole's excepted: S-06 keeps it close whatever it is). Then the report: Yaiphuyiabrus, a red dwarf whose red giant's orbit came inside the
    // giant's own radius (60 degrees of its felisian worlds' sky and more) keeps its worlds and has a small sun for a companion; the reported
    // place, on Yaiphuyiabrus IV (its people's world, a day of 2.4 hours) at its night, is a night under it (the companion's light floor and the
    // set primary's light lit it as a day), and a world of a companion keeps its own night under the far primary
    {
        int multiples = 0, far = 0, closeN = 0, holes = 0, wrong = 0;
        double worstDisc = 0, worstHeat = 0, worstOwn = 0, worstPrimary = 0, worstLight = 0;
        auto scan = [&](int64_t x0, int64_t x1, int64_t y0, int64_t y1, int64_t z0, int64_t z1) {
            for (int64_t x = x0; x < x1; x++) for (int64_t y = y0; y < y1; y++) for (int64_t z = z0; z < z1; z++) {
                Star s; StarSystem sys; if (!systemAt(x, y, z, s, sys, false)) continue;
                if (sys.companion < 0) continue;
                multiples++;
                const Body& k = sys.bodies[sys.companion];
                double reach = 0, own = 0, first = 1e30;
                for (const Body& b : sys.bodies) {
                    if (b.type == PT_COMET || b.type == PT_COMPANION) continue;
                    if (sys.ofCompanion(b.index)) { own = std::max(own, b.orbitRadiusKm * (1 + b.ecc)); continue; }
                    const Body& p = b.parent < 0 ? b : sys.bodies[b.parent];
                    reach = std::max(reach, p.orbitRadiusKm * (1 + p.ecc) + (b.parent < 0 ? 0.0 : (b.drawnOrbitKm > 0 ? b.drawnOrbitKm : b.orbitRadiusKm) * (1 + b.ecc)));   // P-03: a moon's orbit as drawn (a made world's push moves no star)
                    if (b.parent < 0) first = std::min(first, b.orbitRadiusKm);
                }
                if (first > 1e29) continue;   // a primary without worlds: nothing to keep clear of (its companion is close or far by the draw alone)
                if (k.orbitRadiusKm < first) {   // close: the worlds circle both suns
                    if (s.cls == STAR_BLACK_HOLE) { holes++; continue; }
                    closeN++;
                    double light = k.luminosity / s.luminosity;
                    worstLight = std::max(worstLight, light);
                    if (light > 0.1 + 1e-9) wrong++;
                    continue;
                }
                far++;
                double gap = k.orbitRadiusKm * (1 - k.ecc) - reach;
                double disc = gap > k.radiusKm ? 2 * std::asin(k.radiusKm / gap) / DEG : 180;
                double heat = gap > 0 ? 278.0 * std::pow(k.luminosity, 0.25) * std::sqrt(AU_GAME_KM / gap) : 1e4;
                double ownShare = gap > 0 ? own / gap : 1;
                double prim = own > 0 ? 2 * std::asin(std::min(1.0, s.radiusKm / (k.orbitRadiusKm * (1 - k.ecc) - own))) / DEG : 0;
                worstDisc = std::max(worstDisc, disc); worstHeat = std::max(worstHeat, heat); worstOwn = std::max(worstOwn, ownShare); worstPrimary = std::max(worstPrimary, prim);
                if (disc > 3.0 + 1e-6 || heat > 100.0 + 1e-6 || ownShare > 1.0 / 3 + 1e-9 || prim > 3.0 + 1e-6) wrong++;
            }
        };
        scan(150, 320, 0, 1, 20, 140);
        scan(12293, 12308, -7, 8, -5507, -5492);   // the core cube of `galaxy`: the giants, the holes
        check("companions keep clear of the other sun's worlds (B-425)", multiples > 500 && far > 300 && closeN > 50 && wrong == 0,
              fmt("%d multiples: %d far (widest disc %.2f deg, warmest %.0f K, own worlds out to %.2f of the gap, the primary %.2f deg from them), %d close (the brightest %.3f of its primary), %d holes' close ones; %d wrong",
                  multiples, far, worstDisc, worstHeat, worstOwn, worstPrimary, closeN, worstLight, holes, wrong));
        Star ys; StarSystem ysys; bool yFound = starInSector(-31900, 3, -35668, ys);
        if (yFound) ysys.generate(ys);
        bool yShape = yFound && ysys.companion == 8 && ysys.bodies[8].starClass == STAR_RED_GIANT && drawnKind(ysys.bodies[0]) == PT_VENUSIAN && drawnKind(ysys.bodies[1]) == PT_FELISIAN &&
                      drawnKind(ysys.bodies[2]) == PT_FELISIAN && drawnKind(ysys.bodies[3]) == PT_FELISIAN && ysys.bodies[2].locked;   // P-03: as drawn (III a terminator world now)
        double yWidest = 0;
        if (yShape) for (int i = 0; i < 2000; i++) {
            double t = i * ysys.bodies[8].orbitPeriod / 2000.0;
            for (int w = 1; w <= 3; w++) yWidest = std::max(yWidest, 2 * std::asin(std::min(1.0, ysys.bodies[8].radiusKm / length(ysys.bodyPos(8, t) - ysys.bodyPos(w, t)))) / DEG);
        }
        check("the reported giant: a small sun over the same worlds (B-425)", yShape && yWidest < 3.0, fmt("%s: %s, worlds I-IV %s %s %s %s; the giant (R %.3g km) %.3g km out, at its widest %.2f deg of their sky (60 and more before)", yFound ? ys.name.c_str() : "no star",
              yFound ? ysys.classString().c_str() : "-", yShape ? "VENUSIAN" : "?", yShape ? "FELISIAN" : "?", yShape ? "FELISIAN" : "?", yShape ? "FELISIAN (locked)" : "?", yShape ? ysys.bodies[8].radiusKm : 0.0, yShape ? ysys.bodies[8].orbitRadiusKm : 0.0, yWidest));
        if (yShape) {   // the report's place and time: Yaiphuyiabrus IV at three in its night with the giant high (EPOC 6011:003.601; B-426: IV, not III)
            SurfaceView sv; sv.init(&ysys, 3, 3.9 * DEG, -99.4 * DEG, 3601000);
            const SurfaceEnvironment& e = sv.env;
            check("a night under a far companion is a night (B-425)", e.hasSun2 && e.sunDisc.altitude < -0.5 && e.sun2.altitude > 0.5 && e.skyBrightness < 0.2 && e.sun.lightFactor < 0.3 && e.glow < 0.3,
                  fmt("the dwarf at %.0f deg, the giant at %.0f deg (%.2f deg across, light %.3f); the sky %.2f, the light %.2f, its glow %.2f (the giant 95 deg across, the sky 1.0 and the light 1.9 before)", e.sunDisc.altitude / DEG, e.sun2.altitude / DEG,
                      2 * e.sun2.angularRadius / DEG, e.sun2.lightFactor, e.skyBrightness, e.sun.lightFactor, e.glow));
        }
        Star ps; StarSystem psys; bool pFound = starInSector(150, 0, 134, ps);
        if (pFound) psys.generate(ps);
        if (pFound && (int)psys.bodies.size() > 25 && psys.ofCompanion(25) && KINDS[psys.bodies[25].type].atmosphere) {   // `cworld`'s find: an ocean world of a red dwarf companion with the blue giant primary 65 degrees up
            SurfaceView sv; sv.init(&psys, 25, 10 * DEG, 30 * DEG, 100000);
            const SurfaceEnvironment& e = sv.env;
            bool midnight = e.localTime < 0.1 || e.localTime > 0.9;
            check("a companion's world keeps its own night (B-425)", e.sunDisc.altitude > 0.5 && e.sun2.altitude < -0.5 && e.skyBrightness < 0.2 && e.sun.lightFactor < 0.3 && e.primaryRel < 0.3 && midnight,
                  fmt("%s: the far primary at %.0f deg (light %.3f, %.2f of its own sun's), its own sun at %.0f deg; the sky %.2f, the light %.2f, local time %.2f (a blue day and 0.62 before)", psys.bodies[25].name.c_str(),
                      e.sunDisc.altitude / DEG, e.sunDisc.lightFactor, e.primaryRel, e.sun2.altitude / DEG, e.skyBrightness, e.sun.lightFactor, e.localTime));
        } else check("a companion's world keeps its own night (B-425)", false, "the world of sector 150 0 134 moved");
    }
    {   // B-428: a system with distance from the ship. Over the stars near home (the survey's sectors): a solid world's first moon at
        // 5-40 of its radii (beyond 10 in most systems), a twin 12-20 off, a giant's first moon at 4.5-6 still; from the game's
        // parking another world wider than 10 degrees in the sky at a fifth of the parkings (43% before: 85% at a solid world's
        // moon, 80% at a twin), and the ground keeps its discs (wider than 2 degrees at a third of the sites, a wall wider than 20
        // degrees still, from a giant's inner moons)
        int firstN = 0, firstFar = 0, firstOut = 0, giantN = 0, giantOut = 0, twinN = 0, twinOut = 0;
        int parks = 0, parkBig = 0, solidN = 0, solidBig = 0, twinP = 0, twinBig = 0, sites = 0, disc2 = 0, disc20 = 0, bodies = 0;
        for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 140, [&] { return bodies < 2500; })) {
            for (const Body& p : sys.bodies) {   // the first moon's orbit (a twin's world: its next moon is keyed off the twin)
                if (p.parent >= 0 || p.type == PT_COMPANION || p.type == PT_COMET || p.type != drawnKind(p)) continue;   // P-01: a made world's moons keep its drawn size's orbits
                const Body* first = nullptr; bool twin = false;
                for (const Body& m : sys.bodies) if (m.parent == p.index) { twin |= m.doublePlanet; if (!first && m.orbitPeriod > 0) first = &m; }
                for (const Body& m : sys.bodies) if (m.parent == p.index && m.doublePlanet) { twinN++; const double x0 = m.orbitRadiusKm / p.radiusKm; if (x0 < 12 || x0 > 20) twinOut++; }
                if (!first || twin) continue;
                const bool giant = p.type == PT_GASGIANT || p.type == PT_SUBSTELLAR;
                const double x0 = first->orbitRadiusKm / p.radiusKm, ring = p.rings ? p.ringOuter * 1.35 : 0;
                const double lo = 0.95 * std::max(giant ? 4.5 : 5.0, ring), hi = 1.05 * std::max(giant ? 6.0 : 40.0, ring);
                if (giant) { giantN++; if (x0 < lo || x0 > hi) giantOut++; }
                else { firstN++; if (x0 < lo || x0 > hi) firstOut++; if (x0 > 10) firstFar++; }
            }
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type == PT_COMPANION || b.type == PT_COMET) continue;
                bodies++;
                Rng pr(b.seed ^ 0xC40DULL);
                double sky, view; parkedNeighbours(sys, bi, pr.range(0, 4e6), sky, view);
                const bool planet = b.parent < 0 || sys.bodies[b.parent].type == PT_COMPANION;
                parks++; if (sky > 5) parkBig++;
                if (b.doublePlanet) { twinP++; if (sky > 5) twinBig++; }
                else if (planet && b.type != PT_GASGIANT && b.type != PT_SUBSTELLAR) { solidN++; if (sky > 5) solidBig++; }
                if (!KINDS[b.type].landable) continue;
                Rng gr(b.seed ^ 0x5C1EULL);   // the survey's first site: the biggest disc over the horizon
                const double lat = std::asin(gr.sym(1.0)), lon = gr.range(0, TAU), t = gr.range(0, 4e6);
                const Vec3 site = sys.surfacePointWorld(bi, t, lat, lon, 0.002), up = normalize(site - sys.bodyPos(bi, t));
                double best = 0;
                for (int j = 0; j < (int)sys.bodies.size(); j++) {
                    if (j == bi || sys.bodies[j].type == PT_COMPANION) continue;
                    const Vec3 rel = sys.bodyPos(j, t) - site; const double d = length(rel);
                    if (dot(rel / d, up) >= 0.02) best = std::max(best, std::asin(clampd(sys.bodies[j].radiusKm / d, 0, 1)) / DEG);
                }
                sites++; if (best > 1) disc2++; if (best > 10) disc20++;
            }
        }
        check("moons spread: a system with distance from the ship, discs still in the sky (B-428)",
              firstN > 100 && firstOut == 0 && firstFar * 2 > firstN && giantN > 30 && giantOut == 0 && twinN > 5 && twinOut == 0 &&
              parkBig * 100 < parks * 30 && solidBig * 10 < solidN && twinBig * 100 < twinP * 15 && disc2 * 100 > sites * 28 && disc20 * 200 > sites,
              fmt("solid worlds' first moons beyond 10 radii %d of %d (%d outside 5-40), giants' %d outside 4.5-6 of %d, twins %d outside 12-20 of %d; from %d parkings "
                  "a world wider than 10 deg in the sky %.0f%% (solid worlds %.0f%%, twins %.0f%%); from %d sites a disc wider than 2 deg %.0f%%, wider than 20 deg %.1f%%",
                  firstFar, firstN, firstOut, giantOut, giantN, twinOut, twinN, parks, 100.0 * parkBig / std::max(1, parks), 100.0 * solidBig / std::max(1, solidN),
                  100.0 * twinBig / std::max(1, twinP), sites, 100.0 * disc2 / std::max(1, sites), 100.0 * disc20 / std::max(1, sites)));
    }
    // B-312: a big ringed globe whose centre lies outside the frame to the left or the right still shows its limb (its
    // projection is a conic that stretches away from the centre; the old box round the projected centre cut it off);
    // B-317: in a sky the bodies' depths are written in metres, beyond the farthest terrain (they were kilometres, so a
    // moon 20,000 km away hid the ground beyond 20 km)
    {
        Star rs; StarSystem rsys; int rb = -1;
        for (int64_t x = 150; x < 320 && rb < 0; x++)
            for (int64_t z = 20; z < 140 && rb < 0; z++) {
                if (!starInSector(x, 0, z, rs)) continue;
                rsys.generate(rs);
                for (int i = 0; i < (int)rsys.bodies.size() && rb < 0; i++) if (rsys.bodies[i].rings && rsys.bodies[i].type != PT_COMPANION) rb = i;
            }
        check("a ringed world found", rb >= 0);
        if (rb >= 0) {
            SpaceRenderer sr; StarNeighborhood nb; double t = 3000.0;
            const Body& b = rsys.bodies[rb];
            Vec3 bp = rsys.bodyPos(rb, t);
            Vec3 dir = normalize(Vec3(0.3, 0.2, 0.93));
            Vec3 shipPos = bp + dir * (3.6 * b.radiusKm);   // the disc is 32 degrees across from here
            Vec3 toBody = -dir, side = normalize(cross(toBody, Vec3(0, 1, 0)));
            nb.update(shipPos);
            warmMaps(sr, rsys, rb);
            int lit[3] = {0, 0, 0};
            float deepest = 0;
            const double offs[3] = {-45 * DEG, 45 * DEG, 45 * DEG};   // the centre 10 degrees outside the frame on either side, the limb 6 degrees inside
            for (int i = 0; i < 3; i++) {
                Vec3 fwd = normalize(toBody * std::cos(offs[i]) + side * std::sin(offs[i]));
                Framebuffer fb; SpaceContext c;
                c.sys = &rsys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1)));
                c.bankBodyA = rb;
                sr.setupPalette(fb, &rsys, rb, -1, 1.0);
                if (i < 2) {
                    sr.render(fb, c);
                    for (Pix p : fb.idx) { int bk = bankOf(p); if ((bk == 2 || bk == 6 || bk == 11 || bk == 13 || bk == 14 || bk == 15) && intenOf(p) > 0) lit[i]++; }
                } else {   // the same view as a moon's sky sees it (drawSkyBodies: the sky bank, depth in metres)
                    c.skyMode = true; c.skyDark = 1; c.excludeBody = -1;
                    sr.drawSkyBodies(fb, c);
                    for (size_t o = 0; o < fb.idx.size(); o++) { if (bankOf(fb.idx[o]) == 1 && shadeOf(fb.idx[o]) > 35) lit[i]++; deepest = std::max(deepest, fb.invz[o]); }
                }
            }
            check("a globe centred left of the frame shows its limb", lit[0] > 100, std::to_string(lit[0]) + " pixels");
            check("a globe centred right of the frame shows its limb", lit[1] > 100, std::to_string(lit[1]) + " pixels");
            check("the same globe in a sky shows its limb", lit[2] > 100, std::to_string(lit[2]) + " pixels");
            check("sky bodies lie beyond the far ring", deepest > 0 && deepest < 1.0f / 53000.0f, fmt("nearest 1/%.0f m", deepest > 0 ? 1.0 / deepest : 0.0));
            // a small body close by: at 3.6 radii of a 1000 km moon the old kilometre depth read as 3,600 m
            int sb = -1;
            for (int i = 0; i < (int)rsys.bodies.size() && sb < 0; i++) if (rsys.bodies[i].type != PT_COMPANION && rsys.bodies[i].radiusKm < 3000) sb = i;
            if (sb >= 0) {
                Vec3 sp = rsys.bodyPos(sb, t);
                Vec3 ship2 = sp + dir * (3.6 * rsys.bodies[sb].radiusKm);
                Framebuffer fb; SpaceContext c;
                c.sys = &rsys; c.stars = &nb.stars; c.t = t; c.shipPos = ship2; c.cam = cameraBasis(std::atan2(toBody.x, toBody.z), std::asin(clampd(toBody.y, -1, 1)));
                c.skyMode = true; c.skyDark = 1; c.excludeBody = -1;
                sr.setupPalette(fb, &rsys, sb, -1, 1.0);
                sr.drawSkyBodies(fb, c);
                float deep2 = 0; for (float z : fb.invz) deep2 = std::max(deep2, z);
                check("a small body close by lies beyond the far ring too", deep2 > 0 && deep2 < 1.0f / 53000.0f, fmt("nearest 1/%.0f m (it would read 1/%.0f in kilometres)", deep2 > 0 ? 1.0 / deep2 : 0.0, 3.6 * rsys.bodies[sb].radiusKm));
            }
        }
    }
    // B-313: the terrain rings meet at their edges (geomorphing): a ring's edge vertices are drawn on the coarser ring's
    // surface, where the unmorphed vertices differed by metres to hundreds of metres (the cells popped as the edge swept)
    {
        SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
        bool ok = setupSceneForType(PT_FELISIAN, 0, 40 * DEG, 0.0, 0.02, sv, sys, nb, t, SCENE_PINNED_MOUNTAIN);
        check("the mountain review site set up", ok);
        if (ok) {
            Framebuffer fb; Input in;
            sv.update(1.0 / 30, in, t, false);
            sv.render(fb, t, nb.stars, sr, 1.0);
            double seam[4], raw[4];
            sv.testRingSeams(seam, raw);
            const char* const names[4] = {"near ring on lod0", "lod0 on lod1", "lod1 on lod2", "lod2 on lod3"};
            for (int k = 0; k < 4; k++)
                check((std::string("rings meet: ") + names[k]).c_str(), seam[k] >= 0 && seam[k] < 0.01, fmt("gap %.4f m (unmorphed %.1f m)", seam[k], raw[k]));
        }
    }
    // B-316: the habitat cells put herds round the explorer wherever the biome has them, not only at the landing
    {
        SpaceRenderer sr; StarNeighborhood nb; StarSystem sys; SurfaceView sv; double t = 0;
        bool ok = setupSceneForType(PT_FELISIAN, 12, 40 * DEG, 0.0, 0.0, sv, sys, nb, t, SCENE_HERD);
        check("a grassland herd site set up", ok);
        if (ok) {
            Input in;
            for (int i = 0; i < 3; i++) sv.update(1.0 / 30, in, t + i, false);
            int origin = 0, cells = 0;
            for (const SurfaceView::Herd& h : sv.herds) (h.origin ? origin : cells)++;
            check("the landing's own herds are there", origin >= 1, std::to_string(origin) + " herds");
            check("habitat herds spawned within 900 m", cells >= 1, std::to_string(cells) + " herds, " + std::to_string(sv.critters.size()) + " animals in all");
            // walk 3 km away: the landing's herds stay listed, the cells behind are forgotten and new ones spawn
            sv.player.x += 3000; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
            for (int i = 0; i < 3; i++) sv.update(1.0 / 30, in, t + 10 + i, false);
            int far = 0, near = 0;
            for (const SurfaceView::Herd& h : sv.herds) { if (h.origin) continue; double dx = h.cx - sv.player.x, dz = h.cz - sv.player.z; (dx * dx + dz * dz > 1300.0 * 1300.0 ? far : near)++; }
            check("3 km on, the herds behind are forgotten", far == 0, std::to_string(far) + " herds beyond 1300 m");
            check("3 km on, new herds are within reach", near >= 1, std::to_string(near) + " herds within 1300 m");
            bool consistent = true;
            for (const Critter& c : sv.critters) if (c.herd < 0 || c.herd >= (int)sv.herds.size()) consistent = false;
            check("every animal belongs to a listed herd", consistent);
        }
    }
    // O0-04 (B-306): the landing map carries the shadow of a moon at some point of the moon's orbit
    {
        Star ms; StarSystem msys; int mb = -1;
        for (int64_t x = 150; x < 320 && mb < 0; x++)
            for (int64_t z = 20; z < 140 && mb < 0; z++) {
                if (!starInSector(x, 0, z, ms)) continue;
                msys.generate(ms);
                for (int i = 0; i < (int)msys.bodies.size(); i++) if (msys.bodies[i].parent < 0 && msys.bodies[i].moonCount > 0 && KINDS[msys.bodies[i].type].landable && msys.bodies[i].orbitIncl < 3 * DEG) { mb = i; break; }
            }
        check("a landable planet with a moon found", mb >= 0);
        if (mb >= 0) {
            TestGame g; g.savePrefix = "shots/tests/unit_save"; g.settingsPath = "shots/tests/test_settings.txt"; g.guidePath = "shots/tests/unit_guide.txt";
            g.testParkAt(ms, mb);
            int moon = -1; for (int i = 0; i < (int)msys.bodies.size(); i++) if (msys.bodies[i].parent == mb) { moon = i; break; }
            double P = msys.bodies[moon].orbitPeriod, mx = 0;
            for (int k = 0; k < 96 && mx < 0.5; k++) { g.testSetTime(3.6e6 + k * P / 96.0); mx = std::max(mx, g.testMapShadowMax()); }
            check("the moon's shadow crosses the landing map", mx >= 0.5, "darkest " + std::to_string(mx));
        }
    }
    // O2 (R-301): sector names
    {
        int sx, sy;
        sectorIndexOf(0, 0, sx, sy); check("sector of 0,0 is 180:090", sx == 180 && sy == 90);
        sectorIndexOf(-89.9, -179.9, sx, sy); check("sector of the south-west corner is 000:000", sx == 0 && sy == 0);
        sectorIndexOf(89.9, 179.99, sx, sy); check("sector of the north-east corner is 359:179", sx == 359 && sy == 179);
    }
    // O0-01 (B-301): a ringed landable planet exists near the start; its ring profile is shared and bounded; the ring's
    // shadow falls on its ground somewhere at low latitude
    {
        Star rs; StarSystem rsys; int rb = -1;
        for (int64_t x = 150; x < 340 && rb < 0; x++)
            for (int64_t z = 20; z < 160 && rb < 0; z++) {
                if (!starInSector(x, 0, z, rs)) continue;
                rsys.generate(rs);
                for (int i = 0; i < (int)rsys.bodies.size(); i++) if (rsys.bodies[i].rings && rsys.bodies[i].parent < 0 && KINDS[rsys.bodies[i].type].landable) { rb = i; break; }
            }
        check("a ringed landable planet found", rb >= 0);
        if (rb >= 0) {
            std::vector<float> prof = rsys.ringProfileOf(rb);
            float mx = 0; bool bounded = true;
            for (float v : prof) { mx = std::max(mx, v); if (v < 0 || v > 1) bounded = false; }
            check("ring profile bounded with a dense part", bounded && mx > 0.3, "max " + std::to_string(mx));
            double worst = 0;
            for (int la = -30; la <= 30 && worst < 0.2; la += 3)
                for (int k = 0; k < 16 && worst < 0.2; k++) {
                    double tt = 2000.0 + k * rsys.bodies[rb].orbitPeriod / 16.0;
                    SurfaceSite site; site.init(&rsys, rb, la * DEG, 0.5, tt);
                    worst = std::max(worst, site.sun(tt).ringShadow);
                }
            check("the ring shadows the ground somewhere", worst >= 0.2, "shadow " + std::to_string(worst));
        }
    }
    // O1 (B-302): the rangefinder: 45 degrees down from eye height on the ground hits the ground a few metres off (Q-01: on ordinary ground, the
    // nearest east of 0.4, 1.1 that no landform shapes: the spot is in Seapheeseeri IV's penitentes, the ray met a blade at once)
    if (bi >= 0) {
        SurfaceView sv; SpaceRenderer sr; StarNeighborhood nb;
        double rlon = 1.1;
        {   // ground the new slots' landform leaves alone within ten metres (the blades thin out with their field, past its mark)
            BodyGen rg = BodyGen::make(sys.bodies[bi]);
            for (int k = 0; k < 3; k++) rg.traits[k] = 0;   // the new slots' alone
            auto shaped = [&](double lon) { for (int k = -2; k <= 2; k++) if (sampleSurface(rg, StarSystem::bodyFromLatLon(0.4 + k * 5.0 / (rg.R * 1000.0), lon), 4.0).landform > 0.05) return true; return false; };
            while (rlon < 1.6 && shaped(rlon)) rlon += 0.002;
        }
        sv.init(&sys, bi, 0.4, rlon, 0.0);
        nb.update(sv.site.worldPos(0, 0, 0, 0));
        sv.player.pitch = -45 * DEG;
        Input in; sv.update(1.0 / 60, in, 0.0, false);
        Framebuffer fb; sv.render(fb, 0.0, nb.stars, sr, 1.0);
        check("rangefinder 45 deg down hits within 1..6 m", sv.lastRange > 1.0 && sv.lastRange < 6.0, "range " + std::to_string(sv.lastRange));
        check("the far ring reaches 53 km", sv.drawRadiusM() >= 53000, std::to_string(sv.drawRadiusM()));
    }
    // O4 (R-303): comets are landable; on one the explorer is held under the escape velocity and the buggy refuses
    {
        Star cs; StarSystem csys; int cb = -1;
        for (int64_t x = 150; x < 320 && cb < 0; x++)
            for (int64_t z = 20; z < 140 && cb < 0; z++) {
                if (!starInSector(x, 0, z, cs)) continue;
                csys.generate(cs);
                for (int i = 0; i < (int)csys.bodies.size(); i++) if (csys.bodies[i].type == PT_COMET) { cb = i; break; }
            }
        check("comets are landable", KINDS[PT_COMET].landable && cb >= 0);
        if (cb >= 0) {
            SurfaceView sv; sv.init(&csys, cb, 0.2, 0.5, 0.0);
            double ve = sv.site.escapeVelocity;
            check("comet escape velocity under 30 m/s", ve > 0.1 && ve < 30, "v " + std::to_string(ve));
            check("small body: terrain rings capped near the horizon", sv.site.smallBody && sv.drawRadiusM() <= 1.2 * sv.site.R, std::to_string(sv.drawRadiusM()));
            sv.relocateCapsule(sv.player.x + 3, sv.player.z + 2);
            check("no buggy on a comet", !sv.deployBuggy());
            Input in; in.down[KEY_W] = true; in.down[KEY_LEFT_SHIFT] = true;
            for (int i = 0; i < 150; i++) sv.update(1.0 / 30, in, i / 30.0, true);
            double spd = std::sqrt(sv.player.vx * sv.player.vx + sv.player.vz * sv.player.vz);
            check("sprinting on a comet stays under 0.3 escape velocity", spd > 0.05 && spd <= 0.3 * ve + 0.01, "speed " + std::to_string(spd));
        }
    }
    // R-403: the drone lifts off, reaches 320 km/h, climbs to its ceiling, never goes under the ground, lands under Shift and lets the
    // explorer out; a second one unfolds beside the capsule and scraps the first
    {
        SpaceRenderer sr; StarNeighborhood nb; StarSystem dsys; SurfaceView sv; double t = 0;
        if (setupSceneForType(PT_FELISIAN, 12, 40 * DEG, 0.6, 0.0, sv, dsys, nb, t, SCENE_BUGGY_PARKED)) {
            if (sv.inBuggy) sv.toggleBuggy();
            sv.relocateCapsule(sv.player.x + 4, sv.player.z + 2);
            bool dep = sv.deployDrone(); sv.drone.unfold = 1;
            sv.player.x = sv.drone.x + 1.5; sv.player.z = sv.drone.z;
            bool got = sv.toggleDrone();
            Input in2; double under = 0, top = 0, altMax = 0; bool ceilingSeen = false; int frames = 0;
            auto step = [&](int n, bool w, bool s, bool space, bool shift, bool a) {
                for (int i = 0; i < n; i++) {
                    in2.down[KEY_W] = w; in2.down[KEY_S] = s; in2.down[KEY_SPACE] = space; in2.down[KEY_LEFT_SHIFT] = shift; in2.down[KEY_A] = a;
                    sv.update(1.0 / 30, in2, t, true); t += 1.0 / 30; in2.newFrame(); frames++;
                    under = std::max(under, sv.site.surfaceHeight(sv.drone.x, sv.drone.z) - sv.drone.y);
                    top = std::max(top, std::fabs(sv.drone.speed)); altMax = std::max(altMax, sv.drone.altAboveGround); ceilingSeen = ceilingSeen || sv.drone.ceiling;
                }
            };
            step(90, false, false, true, false, false);
            bool lifted = !sv.drone.landed && sv.drone.altAboveGround > 5;
            step(30 * 25, true, false, true, false, false);
            double topAt25 = top;
            step(30 * 25, true, false, true, false, true);
            int n = 0; while (std::fabs(sv.drone.speed) > 2 && n++ < 30 * 30) step(1, false, true, false, false, false);
            double braked = std::fabs(sv.drone.speed);
            n = 0; while (!sv.drone.landed && n++ < 30 * 90) step(1, false, false, false, true, false);
            bool landed = sv.drone.landed; double landS = n / 30.0;
            bool out = sv.toggleDrone() && !sv.inDrone;
            sv.relocateCapsule(sv.player.x + 4, sv.player.z + 2);
            double oldX = sv.drone.x; bool redeployed = sv.deployDrone() && sv.drone.x != oldX && sv.drone.odometer == 0;
            check("drone: lifts, 320 km/h, ceiling, lands, out (R-403)", dep && got && lifted && topAt25 * 3.6 > 300 && top <= SurfaceView::DRONE_TOP_SPEED + 0.01 && ceilingSeen && altMax > 380 && braked <= 2 && landed && out && under < 0.01 && redeployed,
                  fmt("top %.0f km/h at 25 s, %.0f m up, under %.3f m, landed in %.0f s, %d frames", topAt25 * 3.6, altMax, under, landS, frames));
        } else check("drone: a grassland site for the flight", false);
    }
    // B-406: the jetpack flies over deep water (it used to be pulled down into it) and lifts out of it. A shore with water over
    // 0.6 m deep within 110 m: the beach scene, else the river bank, else the lake shore; the flight goes there
    {
        struct WaterRun { bool ran = false, flewOver = false, fellIn = false, liftedOut = false; double dist = 0, deepT = 0, maxDeep = 0, swimT = 0, minAlt = 1e9, heat = 0; const char* where = "none"; };
        WaterRun R;
        auto tryScene = [&](int want, double lat, double alt, const char* name) {
            if (R.ran) return;
            SpaceRenderer sr; StarNeighborhood nb; StarSystem wsys; SurfaceView sv; double t = 0;
            if (!setupSceneForType(PT_FELISIAN, lat, alt, 0.0, 0.0, sv, wsys, nb, t, want)) return;
            double bestD = 1e9, yawTo = 0;
            for (int a = 0; a < 32; a++) for (double dd = 6; dd <= 110; dd += 2) {
                double ang = a * TAU / 32, px = sv.player.x + std::sin(ang) * dd, pz = sv.player.z + std::cos(ang) * dd;
                double g = sv.site.groundHeight(px, pz), wl = sv.site.waterAt(px, pz);
                if (wl > -1e8 && g < wl - 0.6) { if (dd + 6 < bestD) { bestD = dd + 6; yawTo = ang; } break; }
            }
            if (bestD > 1e8) return;
            R.ran = true; R.where = name; R.dist = bestD;
            sv.player.yaw = yawTo; sv.player.pitch = 0;
            int flyFrames = (int)(30 * std::min(18.0, 2.0 + bestD / 3.5));
            Input in2; bool overDeep = false;
            auto step = [&](int n, bool w, bool space, bool spaceTap) {
                for (int i = 0; i < n; i++) {
                    in2.down[KEY_W] = w; in2.down[KEY_SPACE] = space; if (spaceTap && i == 0) in2.pressed[KEY_SPACE] = true;
                    sv.update(1.0 / 30, in2, t, true); t += 1.0 / 30; in2.newFrame();
                    double g = sv.site.groundHeight(sv.player.x, sv.player.z), wl = sv.site.waterAt(sv.player.x, sv.player.z);
                    bool deep = wl > -1e8 && g < wl - 0.4;
                    if (deep) { overDeep = true; R.deepT += 1.0 / 30; R.maxDeep = std::max(R.maxDeep, wl - g); if (sv.player.swimming) R.swimT += 1.0 / 30; else R.minAlt = std::min(R.minAlt, sv.player.y - wl); }
                }
            };
            step(1, true, true, true);           // a jump, then the jet held with W: out over the water, a second past its edge
            for (int i = 1; i < flyFrames && R.deepT < 1.0; i++) step(1, true, true, false);
            R.flewOver = overDeep && R.deepT > 0.5 && R.swimT < 0.2 && R.minAlt > 0.0;
            R.heat = sv.player.jetHeat;
            step(30 * 6, false, false, false);   // the jet released: it drops into the water
            R.fellIn = sv.player.swimming;
            step(30 * 3, false, true, false);    // Space from the surface: the jet lifts it out
            R.liftedOut = !sv.player.swimming && sv.player.jetOn && sv.player.vy > 0;
        };
        tryScene(MAT_SAND, 12, 30 * DEG, "beach");
        tryScene(SCENE_RIVER, 10, 40 * DEG, "river bank");
        tryScene(SCENE_LAKE, 10, 40 * DEG, "lake shore");
        check("jetpack over and out of the water (B-406)", R.ran && R.flewOver && R.fellIn && R.liftedOut,
              R.ran ? fmt("%s: deep water %.0f m off; %.1f s over it (%.1f m deep), swam %.1f s of it, lowest %.1f m over it, heat %.0f, fell in %s, lifted out %s", R.where, R.dist, R.deepT, R.maxDeep, R.swimT, R.minAlt < 1e8 ? R.minAlt : 0.0, R.heat, R.fellIn ? "yes" : "no", R.liftedOut ? "yes" : "no") : "no shore scene with deep water within 110 m");
    }
}
