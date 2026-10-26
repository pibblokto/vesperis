// E-03: the kinds of worlds: `kinds`, `traits` (the quality sheets), `tides`, `datasheet`; E-07: `salt scan`
#include "harness/harness.h"
#include "harness/test_worlds.h"

// Q-01: `datasheet [n]`: the data sheets of the n landable worlds of the scan region whose sheets run longest (their kind's lines, the sentence,
// the traits and the facts; 6 by default), in space and on their ground (`shots/tests/datasheet_<k>_space.png`, `_ground.png`), each with where
// its last line falls: clear of the footer at y 190 when the next line's top is 191 at most
int runDataSheet(int argc, char** argv) {
    const int want = argc > 2 ? atoi(argv[2]) : 6;
    struct Cand { Star s; int bi; size_t score; };
    std::vector<Cand> cands;
    for (int64_t x = 150; x < 340; x++)
        for (int64_t z = 20; z < 160; z++) {
            Star s; if (!starInSector(x, 0, z, s)) continue;
            StarSystem sys; sys.generate(s);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (!KINDS[b.type].landable) continue;
                const BodyGen g = BodyGen::make(b);
                const size_t score = std::strlen(KINDS[b.type].description) + describeBody(b, g).size() + traitList(g).size() + factList(sys, bi).size();
                cands.push_back({s, bi, score});
            }
        }
    std::stable_sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.score > b.score; });
    int over = 0;
    for (int k = 0; k < want && k < (int)cands.size(); k++) {
        const Cand& c = cands[k];
        TestGame game; game.savePrefix = "shots/tests/test_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.keysPath = "shots/tests/test_keys.txt"; game.guidePath = "shots/tests/test_guide.txt";
        game.newGame(); game.setState(GameState::SPACE);
        Input in;
        auto run = [&](double secs) { runFrames(game, in, secs); };
        auto press = [&](int key) { pressKey(game, in, key); };
        game.testParkAt(c.s, c.bi);
        for (const Body& ob : game.testSystem().bodies) if (ob.type != PT_COMPANION) game.testWaitMapOf(ob.index);   // E-02: every map first (F-07)
        run(0.2);
        press(KEY_ESCAPE); run(0.1); press(KEY_I); run(0.1);
        const int ySpace = game.testDataSheetBottom(), pSpace = game.testDataSheetPages();
        writePNG(fmt("shots/tests/datasheet_%d_space.png", k).c_str(), game.output(), FBW, FBH);
        if (pSpace > 1) { press(KEY_RIGHT); run(0.1); writePNG(fmt("shots/tests/datasheet_%d_space2.png", k).c_str(), game.output(), FBW, FBH); }
        press(KEY_ESCAPE); run(0.1);
        game.testParkAt(c.s, c.bi); run(0.2); game.testLandSite(); run(0.2); press(KEY_ENTER); run(8.0);
        int yGround = -1, pGround = 0;
        if (game.state == GameState::SURFACE) {
            press(KEY_I); run(0.1); yGround = game.testDataSheetBottom(); pGround = game.testDataSheetPages();
            writePNG(fmt("shots/tests/datasheet_%d_ground.png", k).c_str(), game.output(), FBW, FBH);
            if (pGround > 1) { press(KEY_RIGHT); run(0.1); writePNG(fmt("shots/tests/datasheet_%d_ground2.png", k).c_str(), game.output(), FBW, FBH); }
        }
        StarSystem sys; sys.generate(c.s);
        const BodyGen g = BodyGen::make(sys.bodies[c.bi]);
        printf("%d %s %s (%s): %d page(s) in space, %d on the ground, the lowest line's next at %d and %d%s; traits [%s], facts [%s]\n", k, c.s.name.c_str(), sys.bodies[c.bi].name.c_str(), KINDS[sys.bodies[c.bi].type].name,
               pSpace, pGround, ySpace, yGround, (ySpace > 191 || yGround > 191) ? " OVER THE FOOTER" : "", traitList(g).c_str(), factList(sys, c.bi).c_str());
        if (ySpace > 191 || yGround > 191) over++;
    }
    printf("datasheet: %d of %d sheets over the footer\n", over, std::min(want, (int)cands.size()));
    return 0;
}

// R-305's gallery (Q-01: every landform, R-304's eighteen and the new slots' eleven): a sheet a landform of the first body carrying it in the
// scan region (`shots/tests/trait_<name>.png`): the hillshade, a frame standing west of the landform's strongest spot and one from 350 m up;
// `traitSheet` draws one for any body (the acidic world's karst). Then the distribution over the six slots
static bool traitSheet(StarSystem& sys, int bi, int tr, const std::string& file, SpaceRenderer& sr, StarNeighborhood& nb) {
    const Body& b = sys.bodies[bi];
    BodyGen g = BodyGen::make(b);
    // the strongest spot on a 2 x 1 degree grid, then the best of a finer grid round it; the scan sees this trait alone (a copy of the body
    // with the others removed), so a canyon's walls do not hide the spires
    BodyGen gOnly = g; for (int k = 0; k < TRAIT_SLOTS; k++) gOnly.traits[k] = 0; gOnly.traits[0] = tr;
    setDrainageEnabled(false);   // O6-03: the scans sample the whole globe at 4-64 m; the frames below turn it back on
    double bestS = 0, bestLat = 0, bestLon = 0;
    const bool smallBlades = tr == TR_PENITENTES && b.type != PT_NITROGEN;
    const bool fine = tr == TR_YARDANGS || tr == TR_POLYGONS || tr == TR_SPIRES || tr == TR_SALT_FLATS || tr == TR_PINGOS || tr == TR_MUD_VOLCANOES || tr == TR_HOLLOWS || tr == TR_COLUMNS;   // the ones the 16 m ring (or the 4 m ring) alone carries
    const double scanD = (tr == TR_SPIRES && b.type != PT_QUARTZ && b.type != PT_CARBON) || tr == TR_POLYGONS || smallBlades ? 4 : (fine ? 16 : 64);
    for (double la = -66; la <= 66; la += 2)
        for (int k = 0; k < 360; k++) {
            double ln = k * TAU / 360;
            SurfaceSample ss = sampleSurface(gOnly, StarSystem::bodyFromLatLon(la * DEG, ln), scanD);
            if (ss.material == MAT_WATER) continue;
            if (ss.landform > bestS) { bestS = ss.landform; bestLat = la * DEG; bestLon = ln; }
        }
    if (bestS < 4) { printf("%-16s %s: nothing over 4 m (best %.1f)\n", TRAITS[tr].name, b.name.c_str(), bestS); return false; }
    double fineS = 0, fLat = bestLat, fLon = bestLon;
    for (int j = -12; j <= 12; j++)
        for (int i = -12; i <= 12; i++) {
            double la = bestLat + j * 800.0 / (b.radiusKm * 1000.0), ln = bestLon + i * 800.0 / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(bestLat)));
            SurfaceSample ss = sampleSurface(gOnly, StarSystem::bodyFromLatLon(la, ln), scanD);
            if (ss.material == MAT_WATER) continue;
            if (ss.landform > fineS) { fineS = ss.landform; fLat = la; fLon = ln; }
        }
    // the standing camera: west of the spot, on dry ground outside the landform's mark (a mesa's or an inselberg's body would swallow it), 100 m
    // past the first such point within 3 km
    double standLon = fLon - 250.0 / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat)));
    // Q-01: a field of small things (blades, pits, a pavement of columns) is seen from inside it, 40 m west of the spot (the 4 m ring's reach)
    const bool inside = smallBlades || tr == TR_HOLLOWS || tr == TR_COLUMNS, low = tr == TR_PINGOS || tr == TR_MUD_VOLCANOES;
    if (inside) standLon = fLon - 40.0 / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat)));
    else for (double dd = 60; dd <= 3000; dd += 60) {
        double ln = fLon - dd / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat)));
        SurfaceSample ss = sampleSurface(gOnly, StarSystem::bodyFromLatLon(fLat, ln), 16);
        if (ss.material == MAT_WATER || (ss.water > -1e8 && ss.height < ss.water + 0.3) || ss.landform >= 1000) continue;
        standLon = ln - 100.0 / (b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat)));
        break;
    }
    // the sheet: the hillshade, a standing frame looking east at the spot, an aerial frame
    setDrainageEnabled(true);   // O6-03: the frames show the real ground
    const int N = 200; std::vector<uint32_t> hs;
    SurfaceSite site; site.init(&sys, bi, fLat, fLon, 1000.0);
    const bool huge = tr == TR_GREAT_SHIELD || (tr == TR_CRATER_CHAINS && b.radiusKm > 1500);
    const bool big = tr == TR_RIFT || tr == TR_ESCARPMENTS || tr == TR_GREAT_BASIN || tr == TR_CORONAE || tr == TR_TESSERAE || tr == TR_LABYRINTH || tr == TR_CRATER_CHAINS || (tr == TR_PENITENTES && !smallBlades);
    const bool mid = tr == TR_CANYONS || tr == TR_CINDER_FIELD || tr == TR_CHAOS || tr == TR_INSELBERGS || tr == TR_ERG || tr == TR_MESAS || tr == TR_MAARS || tr == TR_INVERTED_RIVERS || tr == TR_COLUMNS;
    const double cellM = huge ? 3000.0 : (big ? 800.0 : (mid ? 120.0 : 40.0));
    hillshade(site, N, cellM, huge ? 0.1 : (big ? 0.35 : 1.0), hs);
    std::vector<uint32_t> f1, f2;
    double la2 = fLat, lo2 = standLon;
    const double upM = inside ? 25.0 : (low ? 80.0 : 350.0), downPitch = inside || low ? -0.45 : -0.6;   // Q-01: the small landforms from low over them
    renderSiteFrame(sys, bi, la2, lo2, PI / 2, inside ? 6.0 : -1.0, inside ? -0.12 : 0.04, sr, nb, f1);   // Q-01: among small things, over them (blades block an eye's view)
    renderSiteFrame(sys, bi, la2, lo2, PI / 2, upM, downPitch, sr, nb, f2);
    Sheet sheet(N * 2 + FBW * 2, std::max(N * 2, FBH));
    sheet.blit(hs, N * 2, N * 2, 0, 0);
    sheet.blit(f1, FBW, FBH, N * 2, 0);
    sheet.blit(f2, FBW, FBH, N * 2 + FBW, 0);
    sheet.label(4, 4, fmt("%s  %s (%s)  %.1f%s %.1f%s  strength %.0f", TRAITS[tr].name, b.name.c_str(), KINDS[b.type].name, std::fabs(fLat / DEG), fLat >= 0 ? "N" : "S", std::fabs(fLon / DEG), fLon >= 0 ? "E" : "W", fineS));
    sheet.label(4, N * 2 - 12, fmt("%.0f KM, %.0f M CELLS, NORTH UP", N * cellM / 1000, cellM));
    sheet.label(N * 2 + 4, 4, fmt("%s %.0f M WEST OF THE SPOT", inside ? "6 M UP" : "STANDING", (fLon - standLon) * b.radiusKm * 1000.0 * std::max(0.2, std::cos(fLat))));
    sheet.label(N * 2 + FBW + 4, 4, fmt("%.0f M UP, LOOKING EAST AND DOWN", upM));
    sheet.save(file);
    if (tr == TR_GEYSERS) {   // R-307: the vents the surface will put its geysers on
        std::vector<GeyserVent> vents; geyserVents(g, StarSystem::bodyFromLatLon(bestLat, bestLon), vents);
        int onSinter = 0; double nearest = 1e9;
        for (const GeyserVent& v : vents) {
            double d = std::acos(clampd(dot(v.unit, StarSystem::bodyFromLatLon(bestLat, bestLon)), -1, 1)) * b.radiusKm * 1000.0;
            nearest = std::min(nearest, d);
            if (sampleSurface(g, v.unit, 16).material == MAT_SALT) onSinter++;
        }
        printf("  geyser vents near the spot: %d, %d on sinter, the nearest %.0f m\n", (int)vents.size(), onSinter, nearest);
    }
    {   // Q-01: the sight the scanner hears of the landform within 40 km of the spot (R-304's: their own kinds)
        const int lm = landformSight(tr);
        if (lm < LM_COUNT) {
            std::vector<Landmark> sights; sightsNear(g, StarSystem::bodyFromLatLon(fLat, fLon), 40000.0, 1u << lm, sights);
            double nearest = 1e18; const Landmark* L = nullptr;
            for (const Landmark& m : sights) { const double d = std::acos(clampd(dot(m.unit, StarSystem::bodyFromLatLon(fLat, fLon)), -1, 1)) * b.radiusKm * 1000.0; if (d < nearest) { nearest = d; L = &m; } }
            if (L) printf("  the scanner's %s: %zu within 40 km, the nearest %.1f km off (%.0f m across, %.0f m prominence)\n", LANDMARK_KIND_NAMES[lm], sights.size(), nearest / 1000.0, 2 * L->radiusM, L->prominenceM);
            else printf("  the scanner's %s: none within 40 km\n", LANDMARK_KIND_NAMES[lm]);
        }
    }
    printf("%-16s %s (%s): traits [%s], spot %.2f %.2f, strength %.0f m\n", TRAITS[tr].name, b.name.c_str(), KINDS[b.type].name, traitList(g).c_str(), fLat / DEG, fLon / DEG, fineS);
    return true;
}

// Q-02: the qualities' sheets (the water, the sky, the life and the ground): four frames each of the first body carrying the trait, at the
// times and places it shows (`shots/tests/trait_<name>.png`): a sky's at its hour, a sea's from its shore and from orbit, a ground's among it
struct QualityFrame { std::vector<uint32_t> px; std::string label; };
// a frame of a site at a time: standing (alt < 0) or `alt` metres up looking `pitch`, facing `yaw` (from the north toward the east); `strip`
// takes the trait away first (the frame without it, beside the frame with it)
static void qualityFrame(const StarSystem& sys, int bi, double lat, double lon, double t, double yaw, double alt, double pitch, SpaceRenderer& sr, StarNeighborhood& nb,
                         QualityFrame& out, const std::string& label, int strip = TR_NONE, double px0 = 0, double pz0 = 0) {
    SurfaceView sv;
    sv.init(&sys, bi, lat, lon, t);
    if (strip != TR_NONE) for (int k = 0; k < TRAIT_SLOTS; k++) if (sv.site.gen.traits[k] == strip) sv.site.gen.traits[k] = TR_NONE;
    sv.player.yaw = yaw; sv.player.pitch = alt >= 0 ? 0 : pitch;
    sv.player.x = px0 + std::sin(yaw) * 14; sv.player.z = pz0 + std::cos(yaw) * 14;   // ahead of the capsule, which stays behind the camera
    sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
    if (alt >= 0) { sv.cameraOverrideAlt = alt; sv.cameraOverridePitch = pitch; }
    nb.update(sv.site.worldPos(t, 0, 0, 0));
    Input in; sv.update(0.016, in, t, false);
    Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0);
    fb.mush(2);
    out.px.resize((size_t)FBW * FBH); fb.toRGB(out.px.data(), 1.0);
    out.label = label;
}
// the first time from t0 within `days` of the world's days when `ok` holds of the sun's altitude (degrees) and whether it climbs; -1 none
static double qualityTime(SurfaceSite& site, double t0, double days, const std::function<bool(double altDeg, bool rising, double t)>& ok) {
    const double period = std::max(60.0, std::fabs(site.sys->bodies[site.body].rotPeriod)), dt = period / 720.0;
    for (double tt = t0; tt < t0 + days * period; tt += dt) {
        const double a = effectiveSunAlt(site, tt) / DEG, a2 = effectiveSunAlt(site, tt + dt) / DEG;
        if (ok(a, a2 > a, tt)) return tt;
    }
    return -1;
}
// as `qualityTime` over the first `days` of each twelfth of the year (a season without that light at the place: a polar winter's noon)
static double qualityTimeYear(SurfaceSite& site, double t0, double days, const std::function<bool(double altDeg, bool rising, double t)>& ok) {
    const Body& b = site.sys->bodies[site.body];
    const double year = b.parent >= 0 ? site.sys->bodies[b.parent].orbitPeriod : b.orbitPeriod;
    for (int k = 0; k < 12; k++) { const double t = qualityTime(site, t0 + year * k / 12.0, days, ok); if (t > 0) return t; }
    return -1;
}
// dry land near a latitude (degrees, either sign tried), the first of every 10 degrees of longitude; false none
static bool qualityLand(const BodyGen& g, double latDeg, double& lat, double& lon) {
    for (int open = 1; open >= 0; open--)   // open ground first (no forest over the view of the sky)
        for (int j = 0; j < 8; j++)
            for (int i = 0; i < 36; i++) {
                const double la = (latDeg + (j % 2 ? -1 : 1) * 3.0 * (j / 2)) * DEG, lo = i * 10 * DEG - PI;
                const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 256);
                if (ss.material == MAT_WATER || (ss.water > -1e8 && ss.height <= ss.water + 1)) continue;
                if (open && (ss.material == MAT_FOREST || (ss.material == MAT_GRASS && ss.veg > 0.3))) continue;
                lat = la; lon = lo; return true;
            }
    return false;
}
// the brightest body over a night site within `days` (its time and its direction, local ENU), as the surface's moonlight picks it
static bool qualityMoonTime(SurfaceSite& site, double t0, double days, double& tOut, Vec3& dirOut) {
    const double period = std::max(60.0, std::fabs(site.sys->bodies[site.body].rotPeriod));
    double best = 0;
    for (double tt = t0; tt < t0 + days * period; tt += period / 96.0) {
        if (effectiveSunAlt(site, tt) > -14 * DEG) continue;
        const Mat3 L = site.localFrame(tt); const Vec3 at = site.worldPos(tt, 0, 0, 0.002);
        for (int bi = 0; bi < (int)site.sys->bodies.size(); bi++) {
            if (bi == site.body) continue;
            const Vec3 rel = site.sys->bodyPos(bi, tt) - at; const Vec3 dl = L * (rel / length(rel));
            if (dl.y < 0.35) continue;
            const double il = bodyShineAt(*site.sys, bi, at, tt, site.sun(tt).lightFactor);
            if (il > best) { best = il; tOut = tt; dirOut = dl; }
        }
    }
    printf("  the brightest night body: %.4f of the sun's light\n", best);
    return best > 0.004;
}
// the globe from `distR` radii over a point of it (lat, lon) at a time: on its day side or its night side as the time has it; `strip` takes the
// trait away (the generator's own cache is the renderer's: a map made without it)
static void qualityGlobe(const StarSystem& sys, int bi, double t, double lat, double lon, double distR, SpaceRenderer& sr, StarNeighborhood& nb, QualityFrame& out, const std::string& label) {
    const Body& b = sys.bodies[bi];
    const Vec3 bp = sys.bodyPos(bi, t), up = normalize(sys.surfacePointWorld(bi, t, lat, lon, 0) - bp);
    const Vec3 shipPos = bp + up * (b.radiusKm * distR), fwd = normalize(bp - shipPos);
    nb.update(shipPos);
    Framebuffer fb; SpaceContext c;
    c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(clampd(fwd.y, -1, 1))); c.bankBodyA = bi;
    sr.setSeason(sys, t); warmMaps(sr, sys, bi);
    sr.setupPalette(fb, &sys, bi, -1, 1.0); sr.render(fb, c); fb.mush(2);
    out.px.resize((size_t)FBW * FBH); fb.toRGB(out.px.data(), 1.0);
    out.label = label;
}
// the time within `days` of the world's days from t0 when a point of it faces its sun most (day) or least (night)
static double qualityFacing(const StarSystem& sys, int bi, double lat, double lon, double t0, double days, bool day) {
    const double period = std::max(60.0, std::fabs(sys.bodies[bi].rotPeriod));
    double best = t0, bestV = -2;
    for (double tt = t0; tt < t0 + days * period; tt += period / 180.0) {
        const Vec3 bp = sys.bodyPos(bi, tt), n = normalize(sys.surfacePointWorld(bi, tt, lat, lon, 0) - bp), toStar = normalize(sys.star.pos - bp);
        const double v = (day ? 1 : -1) * dot(n, toStar);
        if (v > bestV) { bestV = v; best = tt; }
    }
    return best;
}
// a sample of the world's water where `score` is greatest on a 1.5-degree grid (within 66 degrees of the equator); false none over 0
static bool qualityWaterSpot(const BodyGen& g, const std::function<double(const SurfaceSample&)>& score, double& lat, double& lon, double detail = 512, double maxLat = 66) {
    double best = 0;
    setDrainageEnabled(false);   // the sea's fields need no rivers (the scan would build every tile of the globe)
    for (double la = -maxLat; la <= maxLat; la += 1.5)
        for (double lo = -180; lo < 180; lo += 1.5) {
            const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, lo * DEG), detail);
            const double v = score(s);
            if (v > best) { best = v; lat = la * DEG; lon = lo * DEG; }
        }
    setDrainageEnabled(true);
    return best > 0;
}
// a lake (the rivers' tiles built round it): the land near a latitude, then a 6 x 6 degree region round it at a tenth of a degree, the deepest
// sample of a lake that `ok` takes; the next land spot further out where it holds none
static bool qualityShore(const BodyGen& g, double wLat, double wLon, double& lat, double& lon, double& yaw, double maxM = 40000);
static bool qualityLakeSpot(const BodyGen& g, const std::function<bool(const SurfaceSample&)>& ok, double& lat, double& lon) {
    for (double lat0 : {25.0, -25.0, 40.0, -40.0, 10.0, -10.0, 55.0, -55.0}) {
        double la0 = 0, lo0 = 0;
        if (!qualityLand(g, lat0, la0, lo0)) continue;
        std::vector<std::pair<double, std::pair<double, double>>> cand;   // the lakes' samples by depth: the deepest that has a shore within 5 km
        for (double dla = -3; dla <= 3; dla += 0.1)
            for (double dlo = -3; dlo <= 3; dlo += 0.1) {
                const double la = la0 + dla * DEG, lo = lo0 + dlo * DEG;
                const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 64);
                if (ok(s)) cand.push_back({-(s.water - s.height), {la, lo}});
            }
        std::sort(cand.begin(), cand.end());
        for (size_t k = 0; k < cand.size() && k < 40; k += 2) {
            double sla, slo, sy;
            if (qualityShore(g, cand[k].second.first, cand[k].second.second, sla, slo, sy, 5000)) { lat = cand[k].second.first; lon = cand[k].second.second; return true; }
        }
    }
    return false;
}
// the nearest dry land to a water point along eight ways (up to 40 km), stepped back to stand 60 m from the water facing it; false none
static bool qualityShore(const BodyGen& g, double wLat, double wLon, double& lat, double& lon, double& yaw, double maxM) {
    const double Rm = g.R * 1000.0;
    for (double dd = 100; dd <= maxM; dd *= 1.25)
        for (int k = 0; k < 16; k++) {
            const double a = k * TAU / 16, la = wLat + std::cos(a) * dd / Rm, lo = wLon + std::sin(a) * dd / (Rm * std::max(0.2, std::cos(wLat)));
            const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 16);
            if (s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water + 0.3)) continue;
            lat = la + std::cos(a) * 60 / Rm; lon = lo + std::sin(a) * 60 / (Rm * std::max(0.2, std::cos(wLat)));
            yaw = wrap2pi(a + PI);   // back toward the water
            return true;
        }
    return false;
}
struct TideSite;
static bool tideFlatSite(const StarSystem& sys, int bi, double& lat, double& lon);
static bool qualitySheet(StarSystem& sys, int bi, int tr, const std::string& file, SpaceRenderer& sr, StarNeighborhood& nb) {
    const Body& b = sys.bodies[bi];
    const BodyGen g = BodyGen::make(b);
    QualityFrame F[4];
    std::string where;
    double lat = 0, lon = 0;
    auto sunAz = [&](SurfaceSite& st, double t) { return st.sun(t).azimuth; };
    switch (tr) {
        case TR_NOCTILUCENT: {   // a high latitude's summer twilight, toward the sun's side; deeper in the twilight; the world without them; the night
            const double tilt = b.axialTilt;
            if (!qualityLand(g, 56, lat, lon)) return false;
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double year = b.parent >= 0 ? sys.bodies[b.parent].orbitPeriod : b.orbitPeriod;
            double t1 = -1;
            for (int k = 0; k < 24 && t1 < 0; k++) {
                const double t0 = 1000.0 + year * k / 24.0;
                if (tilt > 5 * DEG && sys.seasonOf(bi, t0) * (lat >= 0 ? 1 : -1) < 0.25) continue;
                t1 = qualityTime(st, t0, 2, [](double a, bool rising, double) { return !rising && a < -9.0 && a > -10.0; });
            }
            if (t1 < 0) return false;
            const double t2 = qualityTime(st, t1, 1, [](double a, bool rising, double) { return !rising && a < -12.0; });
            const double t3 = qualityTime(st, t1, 1, [](double a, bool, double) { return a < -24.0; });
            qualityFrame(sys, bi, lat, lon, t1, sunAz(st, t1), -1, 0.16, sr, nb, F[0], "THE SUN 9 DEG DOWN, TOWARD IT");
            qualityFrame(sys, bi, lat, lon, t2 > 0 ? t2 : t1, sunAz(st, t2 > 0 ? t2 : t1), -1, 0.16, sr, nb, F[1], "12 DEG DOWN");
            qualityFrame(sys, bi, lat, lon, t1, sunAz(st, t1), -1, 0.16, sr, nb, F[2], "9 DEG DOWN, WITHOUT THEM", tr);
            qualityFrame(sys, bi, lat, lon, t3 > 0 ? t3 : t1, sunAz(st, t1), -1, 0.16, sr, nb, F[3], "THE NIGHT (24 DEG DOWN)");
            break;
        }
        case TR_ICE_HALOS: {   // the sun 18 degrees up and its parhelia; 40 up, the ring; low on the horizon; the moon's ring
            if (!qualityLand(g, 30, lat, lon)) return false;
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double t1 = qualityTime(st, 1000.0, 3, [](double a, bool rising, double) { return rising && a > 17.5 && a < 18.5; });
            const double t2 = qualityTime(st, 1000.0, 3, [](double a, bool, double) { return a > 38.0 && a < 42.0; });
            const double t3 = qualityTime(st, 1000.0, 3, [](double a, bool rising, double) { return !rising && a < 4.0 && a > 2.0; });
            if (t1 < 0) return false;
            qualityFrame(sys, bi, lat, lon, t1, sunAz(st, t1), -1, 18 * DEG, sr, nb, F[0], "THE SUN 18 DEG UP: THE RING AND THE SUNDOGS");
            qualityFrame(sys, bi, lat, lon, t1, sunAz(st, t1), -1, 18 * DEG, sr, nb, F[1], "WITHOUT THEM", tr);
            if (t2 > 0) qualityFrame(sys, bi, lat, lon, t2, sunAz(st, t2), -1, 40 * DEG, sr, nb, F[2], "40 DEG UP");
            if (t3 > 0) qualityFrame(sys, bi, lat, lon, t3, sunAz(st, t3), -1, 0.1, sr, nb, F[3], "3 DEG UP");
            double tm = 0; Vec3 md;
            if (qualityMoonTime(st, 1000.0, 6, tm, md)) qualityFrame(sys, bi, lat, lon, tm, std::atan2(md.x, md.z), -1, std::asin(clampd(md.y, -1, 1)), sr, nb, F[3], "THE NIGHT: THE MOON'S RING");
            break;
        }
        case TR_AIRGLOW: {   // a dark night toward the horizon, overhead, the same without it; the dusk
            if (!qualityLand(g, 20, lat, lon)) return false;
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double t1 = qualityTime(st, 1000.0, 3, [](double a, bool, double) { return a < -30.0; });
            const double t2 = qualityTime(st, 1000.0, 3, [](double a, bool, double) { return a < -7.0 && a > -10.0; });
            if (t1 < 0) return false;
            qualityFrame(sys, bi, lat, lon, t1, 0.6, -1, 0.12, sr, nb, F[0], "THE NIGHT, TOWARD THE HORIZON");
            qualityFrame(sys, bi, lat, lon, t1, 0.6, -1, 0.12, sr, nb, F[1], "WITHOUT IT", tr);
            qualityFrame(sys, bi, lat, lon, t1, 2.6, -1, 0.55, sr, nb, F[2], "HIGHER, ANOTHER WAY");
            if (t2 > 0) qualityFrame(sys, bi, lat, lon, t2, sunAz(st, t2) + PI, -1, 0.12, sr, nb, F[3], "THE DUSK (8 DEG DOWN)");
            break;
        }
        case TR_FOG_SEAS: {   // from a height at dawn over the valleys, the noon after, inside it on a valley's floor, from 500 m up
            if (!qualityLand(g, 25, lat, lon)) return false;
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            // the highest point of the 512 m ring within 4 km, and the way down from it toward the lowest
            double hx = 0, hz = 0, hh = -1e9, lx = 0, lz = 0, lh = 1e9;
            for (int dz = -8; dz <= 8; dz++) for (int dx = -8; dx <= 8; dx++) {
                const TerrainVertex v = st.sampleAt(dx * 512.0, dz * 512.0, 512);
                const double h = v.water > -1e8f ? std::max((double)v.h, (double)v.water) : v.h;
                if (h > hh && v.water < -1e8f) { hh = h; hx = dx * 512.0; hz = dz * 512.0; }
                if (h < lh) { lh = h; lx = dx * 512.0; lz = dz * 512.0; }
            }
            const double t1 = qualityTime(st, 1000.0, 3, [&](double a, bool rising, double) { return rising && a > 10.5 && a < 12.0; });
            const double t2 = qualityTime(st, 1000.0, 3, [](double a, bool, double) { return a > 45.0; });
            if (t1 < 0) return false;
            const double yaw = std::atan2(lx - hx, lz - hz);
            qualityFrame(sys, bi, lat, lon, t1, yaw, 25, -0.10, sr, nb, F[0], fmt("MORNING FROM %.0f M, %.0f M OVER THE LOWEST", hh, hh - lh), TR_NONE, hx, hz);
            if (t2 > 0) qualityFrame(sys, bi, lat, lon, t2, yaw, 25, -0.10, sr, nb, F[1], "NOON, THE SAME VIEW", TR_NONE, hx, hz);
            qualityFrame(sys, bi, lat, lon, t1, yaw, -1, 0.05, sr, nb, F[2], "THE VALLEY FLOOR, IN IT", TR_NONE, lx, lz);
            qualityFrame(sys, bi, lat, lon, t1, yaw, 500, -0.35, sr, nb, F[3], "500 M UP", TR_NONE, hx, hz);
            break;
        }
        case TR_GREAT_AURORAE: {   // the night at 25 degrees toward the pole, the same without them, overhead at 45, the equator
            if (!qualityLand(g, 25, lat, lon)) return false;
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double t1 = qualityTime(st, 1000.0, 3, [](double a, bool, double) { return a < -25.0; });
            if (t1 < 0) return false;
            const double pole = lat >= 0 ? 0.0 : PI;
            qualityFrame(sys, bi, lat, lon, t1, pole, -1, 0.3, sr, nb, F[0], "THE NIGHT AT 25 DEG, POLEWARD");
            qualityFrame(sys, bi, lat, lon, t1, pole, -1, 0.3, sr, nb, F[1], "WITHOUT THEM", tr);
            double la2 = lat, lo2 = lon;
            if (qualityLand(g, lat >= 0 ? 44 : -44, la2, lo2)) {
                SurfaceSite s2; s2.init(&sys, bi, la2, lo2, 1000.0);
                const double t2 = qualityTime(s2, 1000.0, 3, [](double a, bool, double) { return a < -25.0; });
                if (t2 > 0) qualityFrame(sys, bi, la2, lo2, t2, PI / 2, -1, 0.35, sr, nb, F[2], "AT 44 DEG, ALONG THE OVAL");
            }
            if (qualityLand(g, 15, la2, lo2)) {
                SurfaceSite s3; s3.init(&sys, bi, la2, lo2, 1000.0);
                const double t3 = qualityTime(s3, 1000.0, 3, [](double a, bool, double) { return a < -25.0; });
                if (t3 > 0) qualityFrame(sys, bi, la2, lo2, t3, la2 >= 0 ? 0.0 : PI, -1, 0.12, sr, nb, F[3], "AT 15 DEG, POLEWARD");
            }
            break;
        }
        case TR_MILKY_SEAS: {   // the night side from orbit over its brightest patch, its shore at night, the same by day, the night without it
            double wLat = 0, wLon = 0, yaw = 0;
            if (!qualityWaterSpot(g, [](const SurfaceSample& s) { return s.waterKind == 0 && s.material == MAT_WATER && s.water > -1e8 && s.height < s.water - 30 ? s.waterTint : 0.0; }, wLat, wLon, 512, 48)) return false;
            const double tn = qualityFacing(sys, bi, wLat, wLon, 1000.0, 2, false);
            qualityGlobe(sys, bi, tn, wLat, wLon, 2.6, sr, nb, F[0], "THE NIGHT SIDE FROM ORBIT");
            if (!qualityShore(g, wLat, wLon, lat, lon, yaw)) { lat = wLat; lon = wLon; }
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double t1 = qualityTimeYear(st, 1000.0, 3, [](double a, bool, double) { return a < -24.0; }), t2 = qualityTimeYear(st, 1000.0, 3, [](double a, bool, double) { return a > 30.0; });
            if (t1 > 0) {
                qualityFrame(sys, bi, lat, lon, t1, yaw, 14, -0.12, sr, nb, F[1], "ITS SHORE AT NIGHT, 14 M UP");
                qualityFrame(sys, bi, lat, lon, t1, yaw, 14, -0.12, sr, nb, F[3], "THE NIGHT WITHOUT IT", tr);
            }
            if (t2 > 0) qualityFrame(sys, bi, lat, lon, t2, yaw, 14, -0.12, sr, nb, F[2], "BY DAY");
            break;
        }
        case TR_PINK_LAKES: {   // 400 m over its biggest lake by day, its shore, from 3 km up, without them
            double wLat = 0, wLon = 0, yaw = 0;
            if (!qualityLakeSpot(g, [](const SurfaceSample& s) { return s.waterTint > 0.5 && s.material == MAT_WATER; }, wLat, wLon)) return false;
            if (!qualityShore(g, wLat, wLon, lat, lon, yaw)) { lat = wLat; lon = wLon; }
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double t1 = qualityTime(st, 1000.0, 3, [](double a, bool, double) { return a > 35.0; });
            if (t1 < 0) return false;
            qualityFrame(sys, bi, lat, lon, t1, yaw, -1, -0.05, sr, nb, F[0], "ON ITS SHORE");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 400, -0.45, sr, nb, F[1], "400 M UP");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 3000, -0.9, sr, nb, F[2], "3 KM UP");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 400, -0.45, sr, nb, F[3], "400 M UP, WITHOUT THEM", tr);
            break;
        }
        case TR_BLOOMS: {   // the day side from orbit over its strongest streaks, 2 km over them, its shore, from orbit without them
            double wLat = 0, wLon = 0, yaw = 0;
            // a bloom within a few kilometres of a coast (the sea's floor rising toward it)
            if (!qualityWaterSpot(g, [](const SurfaceSample& s) { return s.waterKind == 0 && s.water > -1e8 && s.height < s.water - 20 && s.height > s.water - 150 ? s.lifeTint : 0.0; }, wLat, wLon, 512, 50)) return false;
            const double td = qualityFacing(sys, bi, wLat, wLon, 1000.0, 2, true);
            qualityGlobe(sys, bi, td, wLat, wLon, 2.2, sr, nb, F[0], "THE DAY SIDE FROM ORBIT");
            SurfaceSite sw; sw.init(&sys, bi, wLat, wLon, 1000.0);
            const double t1 = qualityTimeYear(sw, 1000.0, 3, [](double a, bool, double) { return a > 35.0; });
            if (t1 > 0) qualityFrame(sys, bi, wLat, wLon, t1, 0.4, 2000, -0.7, sr, nb, F[1], "2 KM OVER THE SEA");
            if (qualityShore(g, wLat, wLon, lat, lon, yaw)) {
                SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
                const double t2 = qualityTimeYear(st, 1000.0, 3, [](double a, bool, double) { return a > 35.0; });
                if (t2 > 0) { qualityFrame(sys, bi, lat, lon, t2, yaw, 30, -0.15, sr, nb, F[2], "ITS SHORE, 30 M UP"); qualityFrame(sys, bi, lat, lon, t2, yaw, 30, -0.15, sr, nb, F[3], "WITHOUT THEM", tr); }
            } else { lat = wLat; lon = wLon; }
            break;
        }
        case TR_HOT_SPRINGS: {   // standing by a vent's pool, 60 m over its mound, 250 m over the cluster, the same without them
            std::vector<GeyserVent> vents; bool found = false; double vLat = 0, vLon = 0;
            setDrainageEnabled(false);
            for (double la = -50; la <= 50 && !found; la += 1)
                for (double lo = -180; lo < 180 && !found; lo += 1) {
                    const Vec3 u = StarSystem::bodyFromLatLon(la * DEG, lo * DEG);
                    hotSpringsNear(g, u, vents);
                    for (const GeyserVent& v : vents) {
                        if (sampleSurface(g, v.unit, 4).material != MAT_BRINE) continue;
                        vLat = std::asin(clampd(v.unit.z, -1, 1)); vLon = std::atan2(v.unit.y, v.unit.x); found = true; break;
                    }
                }
            setDrainageEnabled(true);
            if (!found) return false;
            const double Rm = b.radiusKm * 1000.0;
            lat = vLat; lon = vLon - 45.0 / (Rm * std::max(0.2, std::cos(vLat)));   // 45 m west of the vent, facing east
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double t1 = qualityTimeYear(st, 1000.0, 3, [](double a, bool, double) { return a > 40.0; });
            if (t1 < 0) return false;
            qualityFrame(sys, bi, lat, lon, t1, PI / 2, 2.5, -0.25, sr, nb, F[0], "BY A VENT'S POOL");
            const double lon2 = vLon - 140.0 / (Rm * std::max(0.2, std::cos(vLat)));
            qualityFrame(sys, bi, vLat, lon2, t1, PI / 2, 60, -0.4, sr, nb, F[1], "60 M UP, 140 M OFF");
            qualityFrame(sys, bi, vLat, lon2, t1, PI / 2, 250, -0.55, sr, nb, F[2], "250 M UP: THE CLUSTER");
            qualityFrame(sys, bi, vLat, lon2, t1, PI / 2, 60, -0.4, sr, nb, F[3], "60 M UP, WITHOUT THEM", tr);
            break;
        }
        case TR_FUNGAL: {   // in a forest of it, at its edge, 300 m over it, the same without it; the globe by day
            double fLat = 0, fLon = 0, best = 0;
            setDrainageEnabled(false);
            for (double la = -50; la <= 50; la += 2)
                for (double lo = -180; lo < 180; lo += 2) {
                    const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, lo * DEG), 512);
                    if (ss.material == MAT_FOREST && ss.veg > best) { best = ss.veg; fLat = la * DEG; fLon = lo * DEG; }
                }
            setDrainageEnabled(true);
            if (best <= 0) return false;
            lat = fLat; lon = fLon;
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double t1 = qualityTimeYear(st, 1000.0, 3, [](double a, bool, double) { return a > 35.0; });
            if (t1 < 0) return false;
            qualityFrame(sys, bi, lat, lon, t1, PI / 2, -1, 0.1, sr, nb, F[0], "IN THE FOREST");
            qualityFrame(sys, bi, lat, lon, t1, PI / 2, 300, -0.5, sr, nb, F[1], "300 M UP");
            qualityFrame(sys, bi, lat, lon, t1, PI / 2, -1, 0.1, sr, nb, F[2], "WITHOUT IT", tr);
            const double td = qualityFacing(sys, bi, lat, lon, 1000.0, 2, true);
            qualityGlobe(sys, bi, td, lat, lon, 2.4, sr, nb, F[3], "FROM ORBIT");
            break;
        }
        case TR_GYPSUM: {   // in the dunes, 120 m over them, 1.5 km over the field, the same without them
            double gLat = 0, gLon = 0, best = 0;
            setDrainageEnabled(false);
            for (double la = -55; la <= 55; la += 1.5)
                for (double lo = -180; lo < 180; lo += 1.5) {
                    const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, lo * DEG), 64);
                    if (ss.landformTrait == TR_GYPSUM && ss.landform > best) { best = ss.landform; gLat = la * DEG; gLon = lo * DEG; }
                }
            setDrainageEnabled(true);
            if (best <= 0) return false;
            lat = gLat; lon = gLon;
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double t1 = qualityTimeYear(st, 1000.0, 3, [](double a, bool, double) { return a > 14.0 && a < 22.0; });   // a low sun on the dunes' faces
            if (t1 < 0) return false;
            const double yaw = g.windAngle + PI / 2;
            qualityFrame(sys, bi, lat, lon, t1, yaw, -1, 0.02, sr, nb, F[0], "AMONG THE DUNES");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 120, -0.45, sr, nb, F[1], "120 M UP");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 1500, -0.7, sr, nb, F[2], "1.5 KM UP");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 120, -0.45, sr, nb, F[3], "120 M UP, WITHOUT THEM", tr);
            break;
        }
        case TR_METEORITES: {   // standing among the stones, 30 m over them, 400 m over the field, the same without them; the scanner's sight
            double mLat = 0, mLon = 0, best = 0;
            setDrainageEnabled(false);
            for (double la = -60; la <= 60; la += 1)
                for (double lo = -180; lo < 180; lo += 1) {
                    const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, lo * DEG), 512);
                    if (ss.material != MAT_WATER && ss.groundTint > best) { best = ss.groundTint; mLat = la * DEG; mLon = lo * DEG; }
                }
            setDrainageEnabled(true);
            if (best < 0.3) return false;
            lat = mLat; lon = mLon;
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double t1 = qualityTimeYear(st, 1000.0, 3, [](double a, bool, double) { return a > 25.0 && a < 45.0; });
            if (t1 < 0) return false;
            const double yaw = sunAz(st, t1) + PI;   // the sun behind: the stones' faces lit (a glint where one faces the eye)
            qualityFrame(sys, bi, lat, lon, t1, yaw + 0.6, -1, -0.2, sr, nb, F[0], "AMONG THE STONES");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 30, -0.6, sr, nb, F[1], "30 M UP");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 400, -0.8, sr, nb, F[2], "400 M UP");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 30, -0.6, sr, nb, F[3], "30 M UP, WITHOUT THEM", tr);
            std::vector<Landmark> sights; sightsNear(g, StarSystem::bodyFromLatLon(lat, lon), 40000.0, 1u << LM_METEORITES, sights);
            double nearest = 1e18; const Landmark* L = nullptr;
            for (const Landmark& m : sights) { const double d = std::acos(clampd(dot(m.unit, StarSystem::bodyFromLatLon(lat, lon)), -1, 1)) * b.radiusKm * 1000.0; if (d < nearest) { nearest = d; L = &m; } }
            if (L) printf("  the scanner's STREWN FIELD: %zu within 40 km, the nearest %.1f km off (%.1f km across)\n", sights.size(), nearest / 1000.0, 2 * L->radiusM / 1000.0);
            else printf("  the scanner's STREWN FIELD: none within 40 km\n");
            break;
        }
        case TR_PETRIFIED: {   // among the stone trunks, 20 m over them, 150 m over the field, the same without them
            double pLat = 0, pLon = 0, best = 0;
            setDrainageEnabled(false);
            for (double la = -55; la <= 55; la += 1)
                for (double lo = -180; lo < 180; lo += 1) {
                    const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, lo * DEG), 64);
                    if (ss.material != MAT_WATER && ss.material != MAT_FOREST && ss.lifeTint > best) { best = ss.lifeTint; pLat = la * DEG; pLon = lo * DEG; }
                }
            setDrainageEnabled(true);
            if (best < 0.3) return false;
            lat = pLat; lon = pLon;
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double t1 = qualityTimeYear(st, 1000.0, 3, [](double a, bool, double) { return a > 20.0 && a < 40.0; });
            if (t1 < 0) return false;
            const double yaw = sunAz(st, t1) + PI * 0.75;
            qualityFrame(sys, bi, lat, lon, t1, yaw, 2.2, -0.12, sr, nb, F[0], "AMONG THE STONE TRUNKS");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 20, -0.45, sr, nb, F[1], "20 M UP");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 150, -0.7, sr, nb, F[2], "150 M UP");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 20, -0.45, sr, nb, F[3], "20 M UP, WITHOUT THEM", tr);
            break;
        }
        case TR_STRATA: {   // facing a striped slope, 80 m over the hills, 600 m over them, the same without them
            double sLat = 0, sLon = 0, best = 0;
            setDrainageEnabled(false);
            for (double la = -55; la <= 55; la += 1)
                for (double lo = -180; lo < 180; lo += 1) {
                    const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, lo * DEG), 64);
                    if (ss.landformTrait == TR_STRATA && ss.relief > best && ss.material != MAT_WATER) { best = ss.relief; sLat = la * DEG; sLon = lo * DEG; }
                }
            setDrainageEnabled(true);
            if (best <= 0) return false;
            lat = sLat; lon = sLon;
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double t1 = qualityTimeYear(st, 1000.0, 3, [](double a, bool, double) { return a > 25.0 && a < 45.0; });
            if (t1 < 0) return false;
            // face the highest ground within a kilometre
            double hx = 0, hz = 0, hh = -1e9;
            for (int j = -8; j <= 8; j++) for (int i = -8; i <= 8; i++) { const double y = st.sampleAt(i * 120.0, j * 120.0, 64).h; if (y > hh) { hh = y; hx = i * 120.0; hz = j * 120.0; } }
            const double yaw = std::atan2(hx, hz);
            qualityFrame(sys, bi, lat, lon, t1, yaw, -1, 0.08, sr, nb, F[0], "FACING THE HILLS");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 80, -0.25, sr, nb, F[1], "80 M UP");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 600, -0.5, sr, nb, F[2], "600 M UP");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 80, -0.25, sr, nb, F[3], "80 M UP, WITHOUT THEM", tr);
            break;
        }
        case TR_REEFS: {   // the day side from orbit over its widest lagoon, 2.5 km over the reef and the lagoon, 300 m over the reef's crest, without it
            // a lagoon within 4 km of deep water outside its reef: the reef's line between them, faced across
            double wLat = 0, wLon = 0, yaw = 0; bool found = false;
            const double Rm = b.radiusKm * 1000.0;
            setDrainageEnabled(false);
            for (double la = -40; la <= 40 && !found; la += 0.5)
                for (double lo = -180; lo < 180 && !found; lo += 0.5) {
                    const SurfaceSample s0 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, lo * DEG), 512);
                    if (s0.material != MAT_WATER || s0.lifeTint < 0.5) continue;
                    for (int k = 0; k < 8 && !found; k++) {
                        const double a = k * TAU / 8, la2 = la * DEG + std::cos(a) * 4000.0 / Rm, lo2 = lo * DEG + std::sin(a) * 4000.0 / (Rm * std::cos(la * DEG));
                        const SurfaceSample s2 = sampleSurface(g, StarSystem::bodyFromLatLon(la2, lo2), 512);
                        if (s2.material == MAT_WATER && s2.lifeTint < 0.5 && s2.height < -40) { wLat = la * DEG; wLon = lo * DEG; yaw = a; found = true; }
                    }
                }
            setDrainageEnabled(true);
            if (!found) return false;
            const double td = qualityFacing(sys, bi, wLat, wLon, 1000.0, 2, true);
            qualityGlobe(sys, bi, td, wLat, wLon, 2.2, sr, nb, F[0], "THE DAY SIDE FROM ORBIT");
            lat = wLat; lon = wLon;
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            const double t1 = qualityTimeYear(st, 1000.0, 3, [](double a, bool, double) { return a > 40.0; });
            if (t1 < 0) return false;
            qualityFrame(sys, bi, lat, lon, t1, yaw, 2500, -0.75, sr, nb, F[1], "2.5 KM OVER THE LAGOON");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 300, -0.3, sr, nb, F[2], "300 M UP, TOWARD THE REEF");
            qualityFrame(sys, bi, lat, lon, t1, yaw, 2500, -0.75, sr, nb, F[3], "2.5 KM UP, WITHOUT IT", tr);
            break;
        }
        case TR_GREAT_TIDES: {   // on the flat at low water and at high water, from 300 m up at both (the day's lowest and highest in daylight)
            if (!tideFlatSite(sys, bi, lat, lon)) return false;
            const Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
            SurfaceSite st; st.init(&sys, bi, lat, lon, 1000.0);
            double tLo = -1, tHi = -1, vLo = 1e9, vHi = -1e9;
            const double span = 6 * std::max(std::fabs(b.rotPeriod), 3600.0);
            for (double t = 1000.0; t < 1000.0 + span; t += 60) {
                if (st.sun(t).altitude < 15 * DEG) continue;
                const double v = tideOffsetM(sys, bi, u, t, g.tideRangeM);
                if (v < vLo) { vLo = v; tLo = t; }
                if (v > vHi) { vHi = v; tHi = t; }
            }
            if (tLo < 0 || tHi < 0) return false;
            double yaw = 0, sh = 1e9;   // toward the sea: the lowest ground within 600 m
            for (int k = 0; k < 16; k++) { const double a = k * TAU / 16; const TerrainVertex v = st.sampleAt(std::sin(a) * 600, std::cos(a) * 600, 16); if (v.h < sh) { sh = v.h; yaw = a; } }
            qualityFrame(sys, bi, lat, lon, tLo, yaw, -1, -0.05, sr, nb, F[0], fmt("LOW WATER (%+.1f M), TOWARD THE SEA", vLo));
            qualityFrame(sys, bi, lat, lon, tHi, yaw, -1, -0.05, sr, nb, F[1], fmt("HIGH WATER (%+.1f M)", vHi));
            qualityFrame(sys, bi, lat, lon, tLo, yaw, 300, -0.55, sr, nb, F[2], "300 M UP, LOW WATER");
            qualityFrame(sys, bi, lat, lon, tHi, yaw, 300, -0.55, sr, nb, F[3], "300 M UP, HIGH WATER");
            printf("  the flat: the spring range here %.1f m, the day's water from %+.2f to %+.2f m\n", tideRangeAt(g, u), vLo, vHi);
            break;
        }
        default: printf("%-18s %s: no sheet yet\n", TRAITS[tr].name, b.name.c_str()); return false;
    }
    Sheet sheet(FBW * 2, FBH * 2 + 12);
    for (int k = 0; k < 4; k++) if (!F[k].px.empty()) { sheet.blit(F[k].px, FBW, FBH, (k % 2) * FBW, 12 + (k / 2) * FBH); sheet.label((k % 2) * FBW + 4, 12 + (k / 2) * FBH + 2, F[k].label); }
    sheet.label(2, 1, fmt("%s  %s (%s)  %.1f%s %.1f%s", TRAITS[tr].name, b.name.c_str(), KINDS[b.type].name, std::fabs(lat / DEG), lat >= 0 ? "N" : "S", std::fabs(lon / DEG), lon >= 0 ? "E" : "W"));
    sheet.save(file);
    printf("%-18s %s (%s): traits [%s], at %.2f %.2f\n", TRAITS[tr].name, b.name.c_str(), KINDS[b.type].name, traitList(g).c_str(), lat / DEG, lon / DEG);
    return true;
}

// Q-02: `tides [scene]`: the great tides of the first ten worlds of the scan region that carry them: each pull's body, its mass (Earth's) and
// distance (the world's radii) and its share of the forcing, the forcing, the trait's spring range, the range met at the equator over three
// days and the time between two high waters; with `scene`, the first one's capsule on its flat over an hour (`shots/tests/tide_shore_<k>.png`
// and a sheet) as the shore runs in or out past it
struct TideSite { StarSystem sys; int bi = -1; double lat = 0, lon = 0; };
static bool tideFlatSite(const StarSystem& sys, int bi, double& lat, double& lon) {
    const BodyGen g = BodyGen::make(sys.bodies[bi]);
    setDrainageEnabled(false);
    double best = 0;
    for (double la = -24; la <= 24; la += 0.5)
        for (double lo = -180; lo < 180; lo += 0.5) {
            const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, lo * DEG), 64);
            const double Rl = tideRangeAt(g, StarSystem::bodyFromLatLon(la * DEG, lo * DEG));
            if (ss.material != MAT_SAND || ss.water < -1e8 || std::fabs(ss.height - ss.water) > 0.15 * Rl) continue;
            const double v = Rl - std::fabs(ss.height - ss.water);
            if (v > best) { best = v; lat = la * DEG; lon = lo * DEG; }
        }
    setDrainageEnabled(true);
    return best > 0;
}
static bool tideFlatSite(const StarSystem& sys, int bi, TideSite& out) { out.sys = sys; out.bi = bi; return tideFlatSite(sys, bi, out.lat, out.lon); }
int runTides(int argc, char** argv) {
    const bool scene = argc > 2 && std::string(argv[2]) == "scene";
    int shown = 0;
    TideSite first;
    for (int64_t x = 150; x < 340 && (shown < 10 || (scene && first.bi < 0)); x++)
        for (int64_t z = 20; z < 160 && (shown < 10 || (scene && first.bi < 0)); z++) {
            Star st; if (!starInSector(x, 0, z, st)) continue;
            StarSystem sys; sys.generate(st);
            for (int bi = 0; bi < (int)sys.bodies.size() && (shown < 10 || (scene && first.bi < 0)); bi++) {
                const Body& b = sys.bodies[bi];
                if (!KINDS[b.type].landable) continue;
                const BodyGen g = BodyGen::make(b);
                if (!g.hasTrait(TR_GREAT_TIDES)) continue;
                if (shown >= 10 && !kindHas(b.type, KT_SEA)) continue;   // past the ten listed: looking for the scene's world alone
                TidePull P[16]; const int n = tidePulls(sys, bi, P, 16);
                printf("%-22s %-12s R %5.0f km, %5.2f Earths, day %5.0f min: forcing %7.0f m, spring range %4.1f m\n", b.name.c_str(), KINDS[b.type].shortName, b.radiusKm, b.massEarths,
                       std::fabs(b.rotPeriod) / 60.0, b.tideForcingM, g.tideRangeM);
                for (int k = 0; k < n; k++) {
                    const bool star = P[k].body < 0 || sys.bodies[P[k].body].type == PT_COMPANION;
                    const double m = P[k].body < 0 ? sys.star.massFactor * 333000.0 : sys.bodies[P[k].body].massEarths;
                    const double d = P[k].body < 0 || P[k].body == b.parent ? (b.parent >= 0 && P[k].body < 0 ? sys.bodies[b.parent].orbitRadiusKm : b.orbitRadiusKm) : sys.bodies[P[k].body].orbitRadiusKm;
                    printf("    pull of %-24s %12.4g Earths at %8.1f radii: %7.0f m (%2.0f%%)\n", star ? (P[k].body < 0 ? "the star" : "the companion") : sys.bodies[P[k].body].name.c_str(),
                           m, d / b.radiusKm, P[k].forcingM, 100 * P[k].share);
                }
                // met at the equator over three days: its range, and the time between its highs
                const Vec3 u = StarSystem::bodyFromLatLon(0, 0.3);
                const double day = std::fabs(b.rotPeriod), dt = day / 400.0;
                double lo = 1e9, hi = -1e9, lastHigh = -1, gaps = 0; int nGaps = 0;
                double prev2 = tideOffsetM(sys, bi, u, 1000.0 - 2 * dt, g.tideRangeM), prev = tideOffsetM(sys, bi, u, 1000.0 - dt, g.tideRangeM);
                for (double t = 1000.0; t < 1000.0 + 3 * day; t += dt) {
                    const double v = tideOffsetM(sys, bi, u, t, g.tideRangeM);
                    lo = std::min(lo, v); hi = std::max(hi, v);
                    if (prev > prev2 && prev >= v) { if (lastHigh > 0) { gaps += t - dt - lastHigh; nGaps++; } lastHigh = t - dt; }
                    prev2 = prev; prev = v;
                }
                const double every = nGaps ? gaps / nGaps / 60.0 : 0.0;
                printf("    at the equator over three days: %.2f to %.2f m (a range of %.1f m), high water every %.0f min\n", lo, hi, hi - lo, every);
                // the scene's: a sea of water whose tide turns slowly enough to be watched over an hour (high water every 1.5 to 15 hours)
                if (scene && first.bi < 0 && kindHas(b.type, KT_SEA) && every > 90 && every < 900) { TideSite ts; if (tideFlatSite(sys, bi, ts)) first = ts; }
                shown++;
            }
        }
    printf("tides: %d worlds with great tides listed\n", std::min(shown, 10));
    if (!scene) return 0;
    if (first.bi < 0) { printf("no flat found\n"); return 1; }
    // the capsule on the flat at the world's next low water, frames every twelve minutes over an hour from behind it, toward the sea
    SpaceRenderer sr; StarNeighborhood nb;
    setFramebufferScale(1);
    const StarSystem& sys = first.sys; const int bi = first.bi;
    const BodyGen g = BodyGen::make(sys.bodies[bi]);
    const Vec3 u = StarSystem::bodyFromLatLon(first.lat, first.lon);
    // the hour round the rising tide's fastest (its mean level crossed going up: the capsule on the flat at the mean level sees the sea come to it
    // and pass it) in daylight (the sun 12 degrees up and over at the hour's start and end)
    double tLow = -1, best = 0;
    { SurfaceSite s0; s0.initFrame(&sys, bi, first.lat, first.lon, 1000.0);
      const double span = 8 * std::max(std::fabs(sys.bodies[bi].rotPeriod), 3600.0);
      double prev = tideOffsetM(sys, bi, u, 1000.0, g.tideRangeM);
      for (double t = 1060.0; t < 1000.0 + span; t += 60) {
          const double v = tideOffsetM(sys, bi, u, t, g.tideRangeM);
          if (prev < 0 && v >= 0 && s0.sun(t - 1800).altitude > 12 * DEG && s0.sun(t + 1800).altitude > 12 * DEG && v - prev > best) { best = v - prev; tLow = t - 1800; }
          prev = v;
      } }
    if (tLow < 0) { printf("no rising tide by day found\n"); return 1; }
    // the sea's way: the lowest ground within 600 m
    SurfaceSite st; st.initFrame(&sys, bi, first.lat, first.lon, tLow);
    double sx = 0, sz = 0, sh = 1e9;
    for (int k = 0; k < 16; k++) { const double a = k * TAU / 16; const TerrainVertex v = st.sampleAt(std::sin(a) * 600, std::cos(a) * 600, 16); if (v.h < sh) { sh = v.h; sx = std::sin(a); sz = std::cos(a); } }
    const double yaw = std::atan2(sx, sz);
    Sheet sheet(FBW * 3, FBH * 2 + 12);
    for (int k = 0; k < 6; k++) {
        const double t = tLow + k * 720.0;
        QualityFrame F;
        SurfaceView sv; sv.init(&sys, bi, first.lat, first.lon, t);
        sv.player.yaw = yaw; sv.player.pitch = -0.08;
        sv.player.x = -std::sin(yaw) * 70; sv.player.z = -std::cos(yaw) * 70;   // 70 m landward of the capsule, 25 m up, looking past it over the flat
        sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
        sv.freeCam = true; sv.freePos = Vec3(sv.player.x, sv.player.y + 25.0, sv.player.z); sv.freeYaw = yaw; sv.freePitch = -0.3;
        nb.update(sv.site.worldPos(t, 0, 0, 0));
        Input in; sv.update(0.016, in, t, false);
        Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
        F.px.resize((size_t)FBW * FBH); fb.toRGB(F.px.data(), 1.0);
        const double tide = sv.site.tideM, edge = sv.site.waterNowAt(0, 0) - sv.site.groundHeight(0, 0);
        writePNG(fmt("shots/tests/tide_shore_%d.png", k).c_str(), F.px.data(), FBW, FBH);
        sheet.blit(F.px, FBW, FBH, (k % 3) * FBW, 12 + (k / 3) * FBH);
        sheet.label((k % 3) * FBW + 4, 12 + (k / 3) * FBH + 2, fmt("+%d MIN: THE SEA %+.2f M, AT THE CAPSULE %s", k * 12, tide, edge > 0 ? fmt("%.2f M DEEP", edge).c_str() : "DRY"));
        printf("  +%2d min: the sea %+.2f m off its mean, at the capsule %s\n", k * 12, tide, edge > 0 ? fmt("%.2f m of water", edge).c_str() : fmt("dry by %.2f m", -edge).c_str());
    }
    sheet.label(2, 1, fmt("GREAT TIDES  %s  THE CAPSULE ON THE FLAT OVER AN HOUR", sys.bodies[bi].name.c_str()));
    sheet.save("shots/tests/tide_shore.png");
    return 0;
}

void renderTraits(const char* want) {
    SpaceRenderer sr; StarNeighborhood nb;
    setFramebufferScale(1);
    for (int tr = 1; tr < TR_COUNT; tr++) {
        const bool quality = TRAITS[tr].kind != TK_FIRST && TRAITS[tr].kind != TK_LANDFORM && TRAITS[tr].kind != TK_SOUND && TRAITS[tr].kind != TK_DARK;   // Q-02: the water, the sky, the life and the ground (Q-03: the sounds are heard: `sounds`, their scenes; D-04: the dark's own are the drifters': `rogues`, their scenes)
        if (TRAITS[tr].cls != TC_LANDFORM && !quality) continue;
        std::string nm = TRAITS[tr].name;
        for (char& ch : nm) ch = ch == ' ' ? '_' : (char)tolower(ch);
        if (want && nm != want && !(std::string(want) == "new" && TRAITS[tr].kind == TK_LANDFORM) && !(std::string(want) == "q02" && quality)) continue;
        bool done = false;
        for (int pass = TRAITS[tr].kind == TK_FIRST ? 1 : 0; pass < 2 && !done; pass++)   // Q-01: a new landform's on a world turning to its sun first (a locked world's spot may lie in its night); Q-02: a quality's too
            for (int64_t x = 150; x < 340 && !done; x++)
                for (int64_t z = 20; z < 160 && !done; z++) {
                    Star s; if (!starInSector(x, 0, z, s)) continue;
                    StarSystem sys; sys.generate(s);
                    for (int bi = 0; bi < (int)sys.bodies.size() && !done; bi++) {
                        const Body& b = sys.bodies[bi];
                        const bool plain = !b.locked && (!quality || (sys.companion < 0 && std::fabs(std::fabs(b.rotPeriod) - b.orbitPeriod) > 0.01 * b.orbitPeriod   // Q-02: a quality's on a
                                                                       && !BodyGen::make(b).hasTrait(TR_STORMS) && !BodyGen::make(b).hasTrait(TR_HAZE)));   // single star's world that turns, its sky clear
                        // Q-02: the tides on a sea of water first (a methane sea's flat is its sand's brown); the aurorae under an air that never rains (rain hides
                        // them); the milky seas on a living world's shores (an ocean world's are its floes)
                        const bool liked = (tr != TR_GREAT_TIDES || kindHas(b.type, KT_SEA)) && (tr != TR_GREAT_AURORAE || KINDS[b.type].rain == RAIN_NONE) && (tr != TR_MILKY_SEAS || kindHas(b.type, KT_FLORA));
                        if (!KINDS[b.type].landable || (pass == 0 && (!plain || !liked)) || !BodyGen::make(b).hasTrait(tr)) continue;
                        if (quality) { done = qualitySheet(sys, bi, tr, "shots/tests/trait_" + nm + ".png", sr, nb); continue; }   // Q-02: the next carrier where one gives no sheet
                        traitSheet(sys, bi, tr, "shots/tests/trait_" + nm + ".png", sr, nb);
                        done = true;
                    }
                }
        if (!done) printf("%-16s: no body carries it in the scanned sectors\n", TRAITS[tr].name);
    }
    if (!want || std::string(want) == "karst" || std::string(want) == "new") {   // Q-01: the karst on an acidic world (it had the name and the sentence and no towers)
        bool done = false;
        for (int64_t x = 150; x < 340 && !done; x++)
            for (int64_t z = 20; z < 160 && !done; z++) {
                Star s; if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size() && !done; bi++)
                    if (sys.bodies[bi].type == PT_ACIDIC && BodyGen::make(sys.bodies[bi]).hasTrait(TR_KARST)) done = traitSheet(sys, bi, TR_KARST, "shots/tests/trait_karst_towers_acidic.png", sr, nb) || true;
            }
        if (!done) printf("KARST TOWERS: no acidic world carries them in the scanned sectors\n");
    }
    if (!want) {   // the distribution over the six slots: the scan region's landable bodies
        int cnt[TR_COUNT] = {0}, n = 0, withFirst[4] = {0, 0, 0, 0}, withNew[4] = {0, 0, 0, 0}, people = 0, peopleNew = 0, kindsNew[TK_COUNT] = {0};
        for (int64_t x = 150; x < 340; x++)
            for (int64_t z = 20; z < 160; z++) {
                Star s; if (!starInSector(x, 0, z, s)) continue;
                StarSystem sys; sys.generate(s);
                for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                    const Body& b = sys.bodies[bi];
                    if (!KINDS[b.type].landable) continue;
                    BodyGen g = BodyGen::make(b);
                    int k1 = 0, k2 = 0;
                    for (int j = 0; j < 3; j++) if (g.traits[j]) { cnt[g.traits[j]]++; k1++; }
                    for (int j = 3; j < TRAIT_SLOTS; j++) if (g.traits[j]) { cnt[g.traits[j]]++; k2++; kindsNew[TRAITS[g.traits[j]].kind]++; }
                    withFirst[k1]++; withNew[k2]++; n++;
                    if (g.hasTrait(TR_CIVILISATION)) { people++; if (k2) peopleNew++; }
                }
            }
        printf("traits over %d landable bodies: R-304's three slots none %d, one %d, two %d, three %d; the new three none %d, one %d, two %d, three %d\n", n,
               withFirst[0], withFirst[1], withFirst[2], withFirst[3], withNew[0], withNew[1], withNew[2], withNew[3]);
        printf("  the new slots by kind:"); for (int k = TK_LANDFORM; k < TK_COUNT; k++) printf(" %d", kindsNew[k]); printf(" (landform, water, sky, life, ground, sound, dark); a people's world %d, with a new trait %d\n", people, peopleNew);
        for (int tr = 1; tr < TR_COUNT; tr++) printf("  %-18s %5d\n", TRAITS[tr].name, cnt[tr]);
    }
}

// P-00: `kinds [name|ground]`: a sheet a kind (shots/tests/kind_<NAME>.png) of its first body in the scan region: the globe from space by day
// and its world map in its own colours (the shared exposure rule at noon), the row's facts, then at its first land site standing and from
// 350 m up, and its event staged where it has one (an eruption, a fountain or a strike set off and faced, a dust devil raised, the cracks'
// or the crusts' nearest vent faced). The argument picks a kind by its name ("THIN_ATMOSPHERE"), the kinds of a ground ("ice") or a made
// kinds' family (P-01: "furnace")
static std::string kindFileName(int k) { std::string n = KINDS[k].name; for (char& c : n) if (c == ' ') c = '_'; return n; }
static bool kindWanted(int k, const char* want) {
    if (!want) return true;
    std::string w = want, n = kindFileName(k), gr = GROUND_NAMES[KINDS[k].ground], fam = KINDS[k].family ? KINDS[k].family : "";
    for (char& c : w) c = (char)toupper(c);
    for (char& c : gr) c = c == ' ' ? '_' : (char)toupper(c);
    for (char& c : fam) c = (char)toupper(c);
    return w == n || w == gr || (!fam.empty() && w == fam);
}
int runKinds(int argc, char** argv) {
    const char* want = argc > 2 ? argv[2] : nullptr;
    SpaceRenderer sr; StarNeighborhood nb;
    setFramebufferScale(1);
    int sheets = 0, missing = 0;
    for (int k = 0; k < PT_COUNT; k++) {
        if (!kindWanted(k, want) || k == PT_COMPANION) continue;
        const KindDef& K = KINDS[k];
        StarSystem sys; int bi = -1;
        if (k == PT_COMET) {   // a comet is no pick of the generator: the first system that has one
            for (int64_t x = 150; x < 340 && bi < 0; x++)
                for (int64_t z = 20; z < 160 && bi < 0; z++) { Star st; if (!starInSector(x, 0, z, st)) continue; sys.generate(st); for (const Body& b : sys.bodies) if (b.type == PT_COMET) { bi = b.index; break; } }
        } else if (!firstBodyOfType(k, sys, bi)) bi = -1;
        if (bi < 0) { printf("%-16s no body in the scan region\n", K.name); missing++; continue; }
        const Body& b = sys.bodies[bi];
        const BodyGen g = BodyGen::make(b);
        Sheet sheet(FBW * 3, FBH * 2 + 12);
        // the globe: parked on the sunlit side, a little off, at 3.5 radii (more past rings)
        {
            const double t = 5000.0;
            const Vec3 bp = sys.bodyPos(bi, t), toStar = sys.sunless() ? Vec3(0.8, 0.0, 0.6) : normalize(sys.star.pos - bp), side = normalize(cross(toStar, Vec3(0, 1, 0)));   // D-04: no star
            const double parkR = b.radiusKm * (k == PT_COMET ? 12.0 : (b.rings ? std::max(3.5, b.ringOuter + 1.3) : 3.5));
            const Vec3 shipPos = bp + normalize(toStar * 0.6 + side * 0.8 + Vec3(0, 0.25, 0)) * parkR, fwd = normalize(bp - shipPos);
            nb.update(shipPos);
            Framebuffer fb; SpaceContext c;
            c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(fwd.y)); c.bankBodyA = bi;
            sr.setSeason(sys, t); warmMaps(sr, sys, bi);   // P-01: the map made for the season the frame draws (else a tilted world's globe was drawn flat)
            sr.setupPalette(fb, &sys, bi, -1, 1.0); sr.render(fb, c); fb.mush(2);
            std::vector<uint32_t> px((size_t)FBW * FBH); fb.toRGB(px.data(), 1.0);
            sheet.blit(px, FBW, FBH, 0, 0);
        }
        // the world map in the body's own colours, 512 x 256 shown at 320 x 160
        {
            PlanetMap m; m.generate(g);
            if (livingGround(k) || (K.tags & KT_FLORA) || K.accent[0] >= 0) {   // P-02: the cover of its land by the map (each cell's area by its latitude)
                double land = 0, cover[MAT_COUNT] = {0};
                for (int y = 0; y < PlanetMap::H; y++) {
                    const double w = std::cos(((y + 0.5) / PlanetMap::H - 0.5) * PI);
                    for (int x = 0; x < PlanetMap::W; x++) { const int mat = m.material[y * PlanetMap::W + x]; if (mat == MAT_WATER || mat == MAT_LAVA) continue; land += w; cover[mat] += w; }
                }
                std::string cv;
                for (int mt = 0; mt < MAT_COUNT; mt++) if (cover[mt] > 0.005 * land) cv += fmt("%s%s %.0f%%", cv.empty() ? "" : ", ", MATERIAL_NAMES[mt], 100 * cover[mt] / std::max(1e-9, land));
                printf("%-16s %s: land cover (by the map) %s\n", K.name, b.name.c_str(), cv.c_str());
            }
            std::vector<uint32_t> px((size_t)FBW * FBH, rgb(12, 12, 14));
            for (int y = 0; y < 160; y++)
                for (int x = 0; x < FBW; x++) {
                    const int i = (y * PlanetMap::H / 160) * PlanetMap::W + x * PlanetMap::W / FBW;
                    const RGB c = materialRampColor(g.matColor[m.material[i]], noonStop(m.albedo[i] / 255.0));
                    px[(size_t)(y + 20) * FBW + x] = rgb((int)(255 * std::min(1.0f, c.r)), (int)(255 * std::min(1.0f, c.g)), (int)(255 * std::min(1.0f, c.b)));
                }
            sheet.blit(px, FBW, FBH, FBW, 0);
        }
        // the row's facts
        {
            int y = 4;
            auto line = [&](const std::string& t) { sheet.label(2 * FBW + 4, y, t); y += 11; };
            line(std::string(K.name) + " (" + K.shortName + ")");
            for (const std::string& w : wrapText(K.description, 50)) line(w);
            line(fmt("GROUND %s, %.0f-%.0f KM, %s", GROUND_NAMES[K.ground], K.minRadiusKm, K.maxRadiusKm, K.atmosphere ? "AIR" : "AIRLESS"));
            std::string tags;
            static const char* const TAG_NAMES[] = {"GIANT", "GAS GIANT", "GLOWS", "DECK", "LAVA", "PLATES", "DRAINAGE", "WET", "FRONTS", "DUST", "FRONT RAIN", "WATER WEATHER", "SEA", "FLORA",
                                                    "FAUNA", "PEOPLE", "DRY", "DEAD SEAS", "TWIN", "GLASSABLE", "INNER MOON", "NO SIGHTS", "CRYSTAL", "CLUSTERS", "FLAT", "FROZEN", "TWILIGHT", "SWIRL",
                                                    "MICROBES", "ICE SHEETS", "FIXED SUN", "VENT LIFE"};
            for (int t = 0; t < (int)(sizeof(TAG_NAMES) / sizeof(TAG_NAMES[0])); t++) if (K.tags & (1ull << t)) tags += std::string(tags.empty() ? "" : ", ") + TAG_NAMES[t];
            for (const std::string& w : wrapText(tags.empty() ? "NO TAGS" : tags, 50)) line(w);
            std::string traits;
            for (int t = 1; t < TR_COUNT; t++) if (traitEligible(k, t)) traits += std::string(traits.empty() ? "" : ", ") + TRAITS[t].name;
            for (const std::string& w : wrapText("TRAITS " + (traits.empty() ? std::string("NONE") : traits), 50)) line(w);
            line(fmt("%s, %s", b.name.c_str(), traitList(g).empty() ? "NO TRAITS" : traitList(g).c_str()));
        }
        if (K.landable) {
            // the first land site: latitude 12 then further out, every 15 degrees of longitude, the first that is not under water (P-04: a dead
            // world's on its old land where its sun can stand at 35 degrees: a world whose day is its year keeps a night side)
            double lat = 12 * DEG, lon = 0; bool land = false;
            const int steps = k == PT_DARK_OCEAN ? 720 : 24;   // D-04: a sea with islands a hundredth of it, every half degree
            for (int j = 0; j < 6 && !land; j++)
                for (int i = 0; i < steps && !land; i++) {
                    const double la = (j % 2 ? -1 : 1) * (12 + 10 * (j / 2)) * DEG, lo = i * (360.0 / steps) * DEG - PI;
                    const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 512);
                    if (ss.material == MAT_WATER || (ss.water > -1e8 && ss.height <= ss.water + 1)) continue;
                    if (deadKind(k)) {
                        if (ss.oldSea > 0.5) continue;
                        SurfaceSite site; site.init(&sys, bi, la, lo, 1000.0);
                        double err = 0; findTime(site, 35 * DEG, 1000.0, &err);
                        if (err > 3 * DEG) continue;
                    }
                    lat = la; lon = lo; land = true;
                }
            std::vector<uint32_t> f1, f2;
            renderSiteFrame(sys, bi, lat, lon, PI / 2, -1, 0.04, sr, nb, f1);
            renderSiteFrame(sys, bi, lat, lon, PI / 2, 350, -0.6, sr, nb, f2);
            sheet.blit(f1, FBW, FBH, 0, FBH + 12);
            sheet.blit(f2, FBW, FBH, FBW, FBH + 12);
            sheet.label(4, FBH + 14, fmt("STANDING %.0f%s %.0f%s, EAST", std::fabs(lat / DEG), lat >= 0 ? "N" : "S", std::fabs(lon / DEG), lon >= 0 ? "E" : "W"));
            sheet.label(FBW + 4, FBH + 14, "350 M UP, EAST AND DOWN");
            // the event, staged: set off at once and run until it shows (a devil fades in over 15 s), then faced; where it stands on a
            // ground of its own (`eventMaterial`), from 120 m off the first such ground of the scan
            std::string what = "NO EVENT";
            if (K.events) {
                double eLat = lat, eLon = lon, tEvent = 1000.0;
                Vec3 eVent; bool plume = false, jets = false, curtain = false;
                if (K.events & EV_PLUMES) plume = plumeStand(sys, bi, eVent, eLat, eLon);   // P-05: 25 km across the wind from a plume
                else if (K.events & EV_CURTAINS) curtain = curtainStand(sys, bi, eVent, eLat, eLon);   // P-06: 3 km from a tiger stripe
                else if (K.events & EV_SPRING_JETS) jets = springStand(sys, bi, tEvent, eVent, eLat, eLon);   // P-05: by a spider in the spring
                else if (K.eventMaterial >= 0) {
                    const double Rm = b.radiusKm * 1000.0;
                    bool found = false;
                    DrainageOff dOff;   // P-01: the scan without the drainage (a tile a sample: minutes on a drained world)
                    for (double la = 12; la < 52 && !found; la += 1.0)
                        for (int i = 0; i < 360 && !found; i++) {
                            const double ln = i * TAU / 360;
                            if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, ln), 16).material != K.eventMaterial) continue;
                            for (int a = 0; a < 8 && !found; a++) {
                                const double dlat = std::cos(a * TAU / 8) * 120.0 / Rm, dlon = std::sin(a * TAU / 8) * 120.0 / (Rm * std::cos(la * DEG));
                                const SurfaceSample s2 = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG + dlat, ln + dlon), 16);
                                if (s2.material == K.eventMaterial || s2.material == MAT_WATER || s2.material == MAT_LAVA) continue;
                                eLat = la * DEG + dlat; eLon = ln + dlon; found = true;
                            }
                        }
                }
                SurfaceView sv;
                sv.init(&sys, bi, eLat, eLon, tEvent);
                double t = findTime(sv.site, (curtain ? 8 : (plume || jets ? 25 : 35)) * DEG, tEvent);
                sv.init(&sys, bi, eLat, eLon, t);
                sv.player.yaw = PI / 2; sv.player.pitch = 0.04;
                sv.player.x = 14; sv.player.z = 0; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                if (plume || jets || curtain) { sv.player.x = 0; sv.player.z = 0; sv.player.y = sv.site.surfaceHeight(0, 0); }
                nb.update(sv.site.worldPos(t, 0, 0, 0));
                Input in;
                sv.update(0.016, in, t, false);
                if (K.events & (EV_ERUPTIONS | EV_FOUNTAINS | EV_STRIKES)) sv.nextEruption = 0;
                if (K.events & (EV_DEVILS | EV_STORM_DEVILS)) sv.nextDevil = 0;
                if (K.events & EV_QUAKES) sv.nextQuake = 0;
                const int frames = (K.events & (EV_DEVILS | EV_STORM_DEVILS)) ? 540 : ((K.events & EV_FOUNTAINS) ? 90 : 45);
                for (int f = 0; f < frames; f++) { t += 1.0 / 30; sv.update(1.0 / 30, in, t, false); }
                double ex = 0, ez = 0; bool at = false;
                if (!sv.eruptions.empty()) { ex = sv.eruptions.back().x; ez = sv.eruptions.back().z; at = true; what = sv.eruptions.back().kind == 1 ? "A FOUNTAIN" : (sv.eruptions.back().kind == 2 ? "A STRIKE" : "AN ERUPTION"); }
                else if (!sv.devils.empty()) {   // walked to 150 m west of it: from farther its dust is a few points against the sand
                    ex = sv.devils.back().x; ez = sv.devils.back().z; at = true; what = "A DUST DEVIL";
                    sv.player.x = ex - 150; sv.player.z = ez; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                }
                else if (K.events & EV_STEAM_BURSTS) {   // P-01: the nearest vent on its sinter (`steamVents`), the clock moved to its burst
                    std::vector<GeyserVent> vents; steamVents(sv.site.gen, sv.site.unitAt(sv.player.x, sv.player.z), vents);
                    double best = 1e18; uint64_t id = 0;
                    for (const GeyserVent& v : vents) {
                        double vx, vz; sv.site.localAt(v.unit, vx, vz);
                        if (sv.site.lod0.at((int)std::floor(vx / 16), (int)std::floor(vz / 16)).material != K.eventMaterial) continue;
                        const double d = (vx - sv.player.x) * (vx - sv.player.x) + (vz - sv.player.z) * (vz - sv.player.z);
                        if (d < best) { best = d; ex = vx; ez = vz; at = true; id = v.id; }
                    }
                    if (at) { const double t0 = t; for (double dtq = 0; dtq < 200 && SurfaceView::burstJetStrength(id, t) < 0.9; dtq += 0.25) t = t0 + dtq; }
                    what = at ? "A STEAM BURST" : "NO VENT NEAR";
                }
                else if (K.events & EV_SPRINGS) {   // P-03: the nearest spring on its pool (`springVents`), walked to 40 m of it
                    std::vector<GeyserVent> vents; springVents(sv.site.gen, sv.site.unitAt(sv.player.x, sv.player.z), vents);
                    double best = 1e18;
                    for (const GeyserVent& v : vents) {
                        double vx, vz; sv.site.localAt(v.unit, vx, vz);
                        if (sv.site.lod0.at((int)std::floor(vx / 16 + 0.5), (int)std::floor(vz / 16 + 0.5)).material != MAT_BRINE) continue;
                        const double d = (vx - sv.player.x) * (vx - sv.player.x) + (vz - sv.player.z) * (vz - sv.player.z);
                        if (d < best) { best = d; ex = vx; ez = vz; at = true; }
                    }
                    if (at && best > 40.0 * 40.0) {
                        const double d = std::sqrt(best);
                        sv.player.x = ex + (sv.player.x - ex) * 40.0 / d; sv.player.z = ez + (sv.player.z - ez) * 40.0 / d; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                    }
                    what = at ? "A HOT SPRING" : "NO SPRING NEAR";
                }
                else if (K.events & (EV_CRACK_VENTS | EV_FUMAROLES)) {   // the nearest vent the surface draws (`drawGeysers`' cells and their ground)
                    const bool cracks = K.events & EV_CRACK_VENTS;
                    const double cs = cracks ? 300 : 200; const int reach = cracks ? 8 : 5;
                    double best = 1e18;
                    for (int cz = -reach; cz <= reach; cz++)
                        for (int cx = -reach; cx <= reach; cx++) {
                            uint64_t h = hash2i(cx, cz, sv.site.gen.seed ^ (cracks ? 0x6E75 : 0xF0DA));
                            if (unitFromHash(h) > (cracks ? 0.35 : 0.4)) continue;
                            double vx = cx * cs + unitFromHash(mix64(h + 1)) * cs, vz = cz * cs + unitFromHash(mix64(h + 2)) * cs;
                            const int mat = sv.site.lod0.at((int)std::floor(vx / 16), (int)std::floor(vz / 16)).material;
                            if (cracks ? mat != MAT_DUST : (mat != MAT_SULPHUR && mat != MAT_LAVA)) continue;
                            double d = (vx - sv.player.x) * (vx - sv.player.x) + (vz - sv.player.z) * (vz - sv.player.z);
                            if (d < best) { best = d; ex = vx; ez = vz; at = true; }
                        }
                    what = at ? "THE NEAREST VENT" : "NO VENT NEAR";
                }
                else if (plume) {   // P-05: the plume faced, the view raised to its flag
                    double vx, vz; sv.site.localAt(eVent, vx, vz);
                    ex = vx; ez = vz; at = true; what = "A PLUME";
                }
                else if (curtain) {   // P-06: the stripe faced, the view raised to its curtain
                    double vx, vz; sv.site.localAt(eVent, vx, vz);
                    ex = vx; ez = vz; at = true; what = "THE CURTAINS";
                }
                else if (jets) {   // P-05: the spider's jet faced, the clock moved to its burst
                    double vx, vz; sv.site.localAt(eVent, vx, vz);
                    std::vector<GeyserVent> vents; springJets(sv.site.gen, eVent, vents);
                    uint64_t id = 0; double bd = 1e18;
                    for (const GeyserVent& v : vents) { const double d = length(v.unit - eVent); if (d < bd) { bd = d; id = v.id; } }
                    if (id) { const double t0 = t; for (double dtq = 0; dtq < 300 && SurfaceView::springJetStrength(id, t) < 0.9; dtq += 0.25) t = t0 + dtq; }
                    ex = vx; ez = vz; at = true; what = sv.springJetsNow(t) ? "A SPRING JET" : "A SPIDER (NO JET NOW)";
                }
                if (K.events & EV_QUAKES) what += ", A QUAKE";
                if (K.events & EV_COMET_JETS) what = "THE JETS";
                if (at) sv.player.yaw = std::atan2(ex - sv.player.x, ez - sv.player.z);
                if (plume || curtain) sv.player.pitch = curtain ? 0.12 : 0.16;
                sv.update(1.0 / 30, in, t, false);
                Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0); fb.mush(2);
                std::vector<uint32_t> px((size_t)FBW * FBH); fb.toRGB(px.data(), 1.0);
                sheet.blit(px, FBW, FBH, 2 * FBW, FBH + 12);
                if (at) what += fmt(" %.0f M OFF", std::sqrt((ex - sv.player.x) * (ex - sv.player.x) + (ez - sv.player.z) * (ez - sv.player.z)));
            }
            sheet.label(2 * FBW + 4, FBH + 14, what);
        } else if (kindHas(k, KT_GIANT)) {   // P-07: a giant's night side (its glow), its limb against space close up (its haze), and the air a probe finds
            const double t = 5000.0;
            const Vec3 bp = sys.bodyPos(bi, t), toStar = sys.sunless() ? Vec3(0.8, 0.0, 0.6) : normalize(sys.star.pos - bp), side = normalize(cross(toStar, Vec3(0, 1, 0)));   // D-04: no star
            for (int view = 0; view < 2; view++) {
                const double parkR = b.radiusKm * (view == 0 ? (b.rings ? std::max(3.5, b.ringOuter + 1.3) : 3.5) : 1.9);
                const Vec3 dir = view == 0 ? normalize(toStar * -0.75 + side * 0.66 + Vec3(0, 0.1, 0)) : normalize(toStar * 0.15 + side * 0.98);
                const Vec3 shipPos = bp + dir * parkR;
                Vec3 fwd = normalize(bp - shipPos);
                if (view == 1) fwd = normalize(fwd * std::cos(0.42) + normalize(cross(fwd, Vec3(0, 1, 0))) * std::sin(0.42) * -1.0);   // toward the lit limb
                nb.update(shipPos);
                Framebuffer fb; SpaceContext c;
                c.sys = &sys; c.stars = &nb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(fwd.y)); c.bankBodyA = bi;
                sr.setupPalette(fb, &sys, bi, -1, 1.0); sr.render(fb, c); fb.mush(2);
                std::vector<uint32_t> px((size_t)FBW * FBH); fb.toRGB(px.data(), 1.0);
                sheet.blit(px, FBW, FBH, view * FBW, FBH + 12);
            }
            sheet.label(4, FBH + 14, "THE NIGHT SIDE");
            sheet.label(FBW + 4, FBH + 14, "THE LIMB AT 1.9 RADII");
            int y = FBH + 16;
            const std::string txt = giantCharacterText(giantAtmosphereOf(sys, bi));
            size_t a0 = 0;
            while (a0 < txt.size()) {
                size_t e = txt.find('\n', a0); if (e == std::string::npos) e = txt.size();
                std::string ln = txt.substr(a0, e - a0); for (char& ch : ln) ch = (char)toupper((unsigned char)ch);
                for (const std::string& w : wrapText(ln, 50)) { sheet.label(2 * FBW + 4, y, w); y += 11; }
                a0 = e + 1;
            }
        } else sheet.label(4, FBH + 14, "NOT LANDABLE");
        sheet.label(4, FBH - 10, fmt("%s, %s, R %.0f KM", b.name.c_str(), STAR_CLASSES[sys.star.cls].code, b.radiusKm));
        sheet.save("shots/tests/kind_" + kindFileName(k) + ".png");
        sheets++;
    }
    printf("kinds: %d sheets%s\n", sheets, missing ? fmt(", %d kinds with no body in the scan region", missing).c_str() : "");
    return missing ? 1 : 0;
}

// E-07: `salt scan [name|ground|family] [tries]`: the made kinds' salts against the protected worlds (`PROTECTED_SYSTEMS`, test_worlds.h),
// the scan P-02 to P-07 each wrote for itself. Every protected system is generated with `t_kindFactsSeen` set, so each world `kindOf`
// read is read with the facts `generate` gave it (a people's world never: it keeps its kind), and walked through the made kinds as
// `kindOf` walks them: a kind it fits (`kindSourceFits`, the first of the row's sources) reaches it, and takes it when its hash (the
// world's seed, its star's for a source `byStar`) falls under the share; a world taken goes no further. For each made kind (or those
// the argument names, as `kinds` reads it): the worlds it reaches and the ones it takes now, and where it takes any, the first salt from
// its own (its high word stepped, as the pinned salts were found) that takes none: a new kind's salt to pin, since a world an older
// kind takes belongs to the generation the unit pins. With an argument, every world reached, its hash and the share. Exit 1 when the
// walk ends anywhere `kindOf` does not (the scan would have drifted from it)
int runSaltScan(int argc, char** argv) {
    if (argc < 3 || std::string(argv[2]) != "scan") { printf("salt scan [name|ground|family] [tries]\n"); return 2; }
    const char* want = argc > 3 ? argv[3] : nullptr;
    const int tries = argc > 4 ? std::max(1, atoi(argv[4])) : 4096;
    struct Q { uint64_t h; double share; std::string world; };
    std::vector<Q> qs[PT_COUNT];
    int systems = 0, worlds = 0, disagree = 0;
    for (const ProtectedSystem& P : PROTECTED_SYSTEMS) {
        Star st;
        if (!starInSector(P.sx, P.sy, P.sz, st) || st.name != P.name) {
            printf("  %s: not at %lld %lld %lld (%s there)\n", P.name, (long long)P.sx, (long long)P.sy, (long long)P.sz, st.valid ? st.name.c_str() : "no star");
            continue;
        }
        std::vector<KindFacts> seen;
        StarSystem sys;
        t_kindFactsSeen = &seen;
        sys.generate(st);
        t_kindFactsSeen = nullptr;
        systems++;
        worlds += (int)seen.size();
        for (const KindFacts& f : seen) {
            std::string name = "?";
            for (const Body& b : sys.bodies) if (b.seed == f.seed) { name = b.name; break; }
            int walked = f.drawn;
            for (int k = PT_DRAWN_COUNT; k < PT_COUNT; k++) {
                const KindDef& K = KINDS[k];
                int src = -1;
                for (int i = 0; i < K.nSources && src < 0; i++) if (kindSourceFits(K.sources[i], (K.tags & KT_GIANT) != 0, f)) src = i;
                if (src < 0) continue;
                const KindSource& S = K.sources[src];
                const uint64_t h = S.byStar ? f.starSeed : f.seed;
                qs[k].push_back({h, S.share, name});
                if (unitFromHash(mix64(h ^ K.salt)) < S.share) { walked = k; break; }   // taken: `kindOf` tries no later kind
            }
            if (walked != kindOf(f)) { disagree++; printf("  %s: the walk ends at %s, kindOf says %s\n", name.c_str(), KINDS[walked].name, KINDS[kindOf(f)].name); }
        }
    }
    printf("salt scan: %d of %d protected systems, %d readings of their worlds by kindOf, walked through the made kinds as kindOf walks them\n",
           systems, (int)(sizeof PROTECTED_SYSTEMS / sizeof PROTECTED_SYSTEMS[0]), worlds);
    auto takes = [&](int k, uint64_t salt, std::string* names) {
        int n = 0;
        for (const Q& q : qs[k])
            if (unitFromHash(mix64(q.h ^ salt)) < q.share) { n++; if (names) *names += (names->empty() ? "" : ", ") + q.world; }
        return n;
    };
    int shown = 0, taking = 0;
    for (int k = PT_DRAWN_COUNT; k < PT_COUNT; k++) {
        if (!kindWanted(k, want)) continue;
        const KindDef& K = KINDS[k];
        if (!K.nSources) continue;   // made by no source of `kindOf` (an `elsewhere` kind, the drifters' own)
        shown++;
        std::string names;
        const int n = takes(k, K.salt, &names);
        printf("  %-18s reaches %2zu, takes %d  0x%016llXULL", K.name, qs[k].size(), n, (unsigned long long)K.salt);
        if (n) {
            taking++;
            uint64_t got = 0;
            int j = 1;
            for (; j < tries && !got; j++) if (!takes(k, K.salt + ((uint64_t)j << 32), nullptr)) got = K.salt + ((uint64_t)j << 32);
            printf(": %s; ", names.c_str());
            if (got) printf("none from 0x%016llXULL (%d steps)\n", (unsigned long long)got, j - 1);
            else printf("none within %d steps\n", tries);
        } else printf("\n");
        if (want)
            for (const Q& q : qs[k]) {
                const double u = unitFromHash(mix64(q.h ^ K.salt));
                printf("      %-22s hash %.3f, share %.3f%s\n", q.world.c_str(), u, q.share, u < q.share ? ": taken" : "");
            }
    }
    if (!shown) { printf("no made kind is \"%s\"\n", want ? want : ""); return 2; }
    printf("%d of %d kinds take a protected world now; the walk agrees with kindOf on %d of %d readings\n", taking, shown, worlds - disagree, worlds);
    return disagree ? 1 : 0;
}
