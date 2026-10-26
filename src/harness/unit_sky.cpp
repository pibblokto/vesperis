// E-03: the unit's stars and skies (groups `stars`, `sky`)
#include "harness/harness.h"
#include "harness/test_worlds.h"

// the fields, the star classes, the strange stars
void unitStars(const Check& check) {
    {   // magnetic fields and the star's storms: every class occurs; at the oval, a strong field round an active star keeps an
        // aurora on most nights (over the HUD's 0.15), a weak field has one rarely; the storms come and go
        int cls[4] = {0, 0, 0, 0}, n = 0, strongN = 0, weakN = 0, stormy = 0, nights = 0;
        double strongLit = 0, weakLit = 0;
        for (auto& [x, z, s, sys] : scanSystems(150, 200, 20, 70)) {
            for (int d = 0; d < 20; d++) { nights++; if (auroralStorm(s, d * 86400.0 + 500) > 0.3) stormy++; }
            for (const Body& b : sys.bodies) {
                if (!KINDS[b.type].landable || b.type == PT_COMET) continue;
                double m = magneticField(b); cls[magneticClass(m)]++; n++;
                if (!KINDS[b.type].atmosphere || hasOpaqueDeck(b.type)) continue;
                int lit = 0; for (int d = 0; d < 60; d++) if (auroraPotential(sys, b, d * 86400.0 + 1000) > 0.15) lit++;
                bool active = starActivity(s.cls) >= 0.7;   // activity 0.7 and over; S-02: from the table
                if (m >= 0.6 && active) { strongLit += lit / 60.0; strongN++; }
                else if (m >= 0.08 && m < 0.3) { weakLit += lit / 60.0; weakN++; }
            }
        }
        check("magnetic field: every class occurs", n > 50 && cls[0] > 0 && cls[1] > 0 && cls[2] > 0 && cls[3] > 0, fmt("of %d worlds: none %d, weak %d, moderate %d, strong %d", n, cls[0], cls[1], cls[2], cls[3]));
        check("storms on some nights, not most", nights > 0 && stormy > nights / 20 && stormy < nights / 2, fmt("%d of %d nights over 0.3", stormy, nights));
        check("strong field, active star: most nights", strongN > 0 && strongLit / strongN > 0.7, fmt("%.0f%% of nights lit over %d worlds", strongN ? 100 * strongLit / strongN : 0.0, strongN));
        check("weak field: rarely", weakN > 0 && weakLit / weakN < 0.25, fmt("%.0f%% of nights lit over %d worlds", weakN ? 100 * weakLit / weakN : 0.0, weakN));
    }
    {   // the star classes. The varieties take their share of the families near home (red dwarfs the commonest class, every class present)
        int n = 0, cls[STAR_DRAWN_COUNT] = {0};
        for (int64_t x = 150; x < 200; x++) for (int64_t y = -6; y < 6; y++) for (int64_t z = 20; z < 70; z++) { Star s; if (starInSector(x, y, z, s, false)) { n++; cls[s.cls]++; } }
        int most = 0; bool all = true; std::string mixs;
        for (int i = 0; i < STAR_DRAWN_COUNT; i++) { if (cls[i] > cls[most]) most = i; if (cls[i] == 0) all = false; mixs += fmt("%s %.0f%% ", STAR_CLASSES[i].code, n ? 100.0 * cls[i] / n : 0.0); }
        check("star classes: red dwarfs the commonest", n > 1000 && most == STAR_RED_DWARF && cls[STAR_RED_DWARF] > n / 4 && cls[STAR_RED_DWARF] < n / 2, fmt("%d stars: %s", n, mixs.c_str()));
        check("star classes: every class near home", all, mixs);
        // the red dwarf's flares: several a day, a minute each, the same at the same time
        Star rd; bool found = false;
        for (int64_t x = 150; x < 200 && !found; x++) for (int64_t z = 20; z < 70 && !found; z++) if (starInSector(x, 0, z, rd, false) && rd.cls == STAR_RED_DWARF) found = true;
        int samples = 0, flaring = 0, flares = 0; double peak = 0; bool det = true, was = false;
        for (double t = 0; t < 10 * 86400.0 && found; t += 2) {
            double f = starFlare(rd, t); samples++;
            if (f != starFlare(rd, t)) det = false;
            bool on = f > 0.3; if (on) flaring++; if (on && !was) flares++; was = on;
            peak = std::max(peak, f);
        }
        check("red dwarf flares: several a day", found && det && flares >= 50 && flares <= 200 && flaring < samples / 15 && flaring > samples / 500, fmt("%d flares in ten days, %.1f%% of the time over 0.3, peak %.2f", flares, samples ? 100.0 * flaring / samples : 0.0, peak));
        // the red dwarf's near worlds keep one face to it; a carbon star's worlds are carbon more often than a yellow star's
        int rdWorlds = 0, rdLocked = 0, csSys = 0, csCarbon = 0, ySys = 0, yCarbon = 0;
        for (int64_t x = 150; x < 300; x++) for (int64_t z = 20; z < 120; z++) {
            Star s; if (!starInSector(x, 0, z, s, false)) continue;
            if (s.cls != STAR_RED_DWARF && s.cls != STAR_CARBON && s.cls != STAR_YELLOW) continue;
            StarSystem sys; sys.generate(s);
            bool carbon = false; for (auto& b : sys.bodies) if (b.type == PT_CARBON) carbon = true;
            if (s.cls == STAR_RED_DWARF) { int k = 0; for (auto& b : sys.bodies) { if (b.parent >= 0 || b.type == PT_COMET || b.type == PT_COMPANION) continue; if (k++ < 3) { rdWorlds++; if (b.locked) rdLocked++; } } }
            else if (s.cls == STAR_CARBON) { csSys++; if (carbon) csCarbon++; }
            else { ySys++; if (carbon) yCarbon++; }
        }
        check("red dwarf: near worlds locked", rdWorlds > 30 && rdLocked > rdWorlds * 0.7, fmt("%d of %d", rdLocked, rdWorlds));
        check("carbon star: carbon worlds", csSys > 5 && ySys > 5 && (double)csCarbon / csSys > 2.0 * (double)yCarbon / ySys, fmt("%d of %d systems against %d of %d round yellow stars", csCarbon, csSys, yCarbon, ySys));
    }
    {   // the neutron star. A share of the pulsars near home; its systems keep no living, cloudy, oceanic, quartz or giant world
        // and most of those with planets have a glassed outer survivor; on such a world the glass covers 15-85% of the land at 16 m
        // and the 512 m ring (the sheets do not follow the rings' fades, B-404); every neutron star of the scan is described
        int ns = 0, bad = 0, withPlanets = 0, glassedSys = 0, belts = 0;
        std::vector<Body> glassedBodies;   // the first five, for the sheets' share
        for (int64_t x = 150; x < 300; x++) for (int64_t z = 20; z < 120; z++) {
            Star s; if (!starInSector(x, 0, z, s, false) || s.cls != STAR_NEUTRON) continue;
            StarSystem sys; sys.generate(s); ns++; belts += (int)sys.belts.size();
            bool planets = false, glassed = false;
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type == PT_COMET || b.type == PT_COMPANION || (b.parent >= 0 && sys.bodies[b.parent].type == PT_COMPANION)) continue;   // a companion's worlds are its own class's
                if (b.type == PT_FELISIAN || b.type == PT_VENUSIAN || b.type == PT_GASGIANT || b.type == PT_OCEAN || b.type == PT_QUARTZ || b.type == PT_ACIDIC) bad++;
                if (b.parent < 0) planets = true;
                if (b.glassed) { glassed = true; if (glassedBodies.size() < 5) glassedBodies.push_back(b); }
            }
            if (planets) withPlanets++;
            if (glassed) glassedSys++;
        }
        check("neutron stars: some near home, none with a living or giant world", ns >= 5 && bad == 0, fmt("%d stars, %d such worlds, %d belts", ns, bad, belts));
        check("neutron stars: a glassed survivor in most systems with planets", withPlanets > 0 && glassedSys >= withPlanets * 0.6, fmt("%d of %d systems with planets", glassedSys, withPlanets));
        {   // the sheets: over the first five glassed worlds a share of the land between 15 and 85% on average (a crater-saturated small
            // world keeps little flat ground, a metal world half), and on each the same share at 512 m within 8 points
            double sum16 = 0, worst = 0; std::string names;
            for (const Body& gb : glassedBodies) {
                BodyGen g = BodyGen::make(gb);
                int land16 = 0, glass16 = 0, land512 = 0, glass512 = 0;
                for (int k = 0; k < 2000; k++) {
                    double u1 = unitFromHash(hash2i(k, 1, 0x61A5ULL)), u2 = unitFromHash(hash2i(k, 2, 0x61A5ULL));
                    Vec3 unit = StarSystem::bodyFromLatLon(std::asin(2 * u1 - 1), u2 * TAU);
                    SurfaceSample a = sampleSurface(g, unit, 16), b2 = sampleSurface(g, unit, 512);
                    if (a.material != MAT_WATER) { land16++; if (a.material == MAT_GLASS) glass16++; }
                    if (b2.material != MAT_WATER) { land512++; if (b2.material == MAT_GLASS) glass512++; }
                }
                double sh16 = land16 ? (double)glass16 / land16 : 0, sh512 = land512 ? (double)glass512 / land512 : 0;
                if (!g.hasTrait(TR_GLASSED)) worst = 1;
                sum16 += sh16; worst = std::max(worst, std::fabs(sh16 - sh512));
                names += fmt("%s%s %.0f/%.0f%%", names.empty() ? "" : ", ", KINDS[gb.type].name, 100 * sh16, 100 * sh512);
            }
            double mean = glassedBodies.empty() ? 0 : sum16 / glassedBodies.size();
            check("glassed worlds: sheets over 15-85% of the land, the same at 512 m", glassedBodies.size() >= 3 && mean > 0.15 && mean < 0.85 && worst < 0.08,
                  fmt("%zu worlds, %.0f%% of the land on average, the 16 m and 512 m shares %.0f points apart at most (%s)", glassedBodies.size(), 100 * mean, 100 * worst, names.c_str()));
        }
    }
    {   // the protostar. A trickle along the arm (a few near home), a large share of the yellow family in a star-forming complex;
        // its systems are belts with at most two young worlds and no evolved one; its cloud glows toward the star from its worlds
        int armY = 0, armP = 0, nebY = 0, nebP = 0, ps = 0, withBelt = 0, evolved = 0, worlds = 0; bool nebFound = false;
        for (int64_t x = 150; x < 200; x++) for (int64_t y = -6; y < 6; y++) for (int64_t z = 20; z < 70; z++) { Star s; if (!starInSector(x, y, z, s, false)) continue; if (s.cls == STAR_YELLOW) armY++; if (s.cls == STAR_PROTOSTAR) armP++; }
        for (int64_t x = HOME_SX - 200; x <= HOME_SX + 200 && !nebFound; x += 7) for (int64_t z = HOME_SZ - 200; z <= HOME_SZ + 200 && !nebFound; z += 7) {
            if (galaxyRegion(x, 0, z) != REGION_NEBULA) continue;
            nebFound = true;
            for (int64_t dx = -7; dx <= 7; dx++) for (int64_t dy = -7; dy <= 7; dy++) for (int64_t dz = -7; dz <= 7; dz++) {
                if (galaxyRegion(x + dx, dy, z + dz) != REGION_NEBULA) continue;
                Star s; if (!starInSector(x + dx, dy, z + dz, s, false)) continue;
                if (s.cls == STAR_YELLOW) nebY++; if (s.cls == STAR_PROTOSTAR) nebP++;
            }
        }
        double glow = 0; std::string first;
        for (int64_t x = 150; x < 300; x++) for (int64_t z = 20; z < 120; z++) {
            Star s; if (!starInSector(x, 0, z, s, false) || s.cls != STAR_PROTOSTAR) continue;
            StarSystem sys; sys.generate(s); ps++;
            if (!sys.belts.empty()) withBelt++;
            for (const Body& b : sys.bodies) {
                if (b.type == PT_COMET || b.type == PT_COMPANION || (b.parent >= 0 && sys.bodies[b.parent].type == PT_COMPANION)) continue;
                worlds++;
                if (b.type == PT_FELISIAN || b.type == PT_VENUSIAN || b.type == PT_QUARTZ || b.type == PT_OCEAN || b.type == PT_ACIDIC || b.type == PT_DESERT || b.type == PT_HYDROCARBON || b.type == PT_EUROPAN) evolved++;
                if (first.empty() && b.parent < 0) {
                    NebulaPatch np[4]; int nn = starNebulaPatches(s, Vec3(0, 0, 1), np); int tone;
                    glow = nebulaGlow(np, nn, Vec3(0.3, 0.1, 0.95), tone);   // a fifth of a radian from the star
                    first = fmt("%s (%d patches, tone %d)", KINDS[b.type].name, nn, tone);
                }
            }
        }
        check("protostars: a trickle in the arm, half the yellow family in a complex", armP > 0 && armP < armY / 10 && nebFound && nebP > nebY / 2, fmt("near home %d against %d yellow stars; in the complex %d against %d", armP, armY, nebP, nebY));
        check("protostars: belts and young worlds only", ps >= 3 && withBelt >= ps * 0.85 && evolved == 0, fmt("%d stars, %d with a belt, %d worlds, %d evolved", ps, withBelt, worlds, evolved));
        check("protostar: its cloud glows round it", !first.empty() && glow > 0.4, fmt("glow %.2f a fifth of a radian from the star, the first world %s", glow, first.c_str()));
    }
    {   // the Wolf-Rayet star. A share of the blue giants that stayed near home (rare; none in a globular), systems of few
        // bare worlds under a wind that stripped every atmosphere, and the shell of shed gas drawn as a ring round the sun
        int armB = 0, armW = 0, globN = 0, globW = 0, ws = 0, worlds = 0, aired = 0, bomb = 0, withPlanets = 0, withBelt = 0, giants = 0;
        for (int64_t x = 150; x < 200; x++) for (int64_t y = -6; y < 6; y++) for (int64_t z = 20; z < 70; z++) { Star s; if (!starInSector(x, y, z, s, false)) continue; if (s.cls == STAR_BLUE_GIANT) armB++; if (s.cls == STAR_WOLF_RAYET) armW++; }
        for (int64_t x = 485; x <= 499; x++) for (int64_t y = -5854; y <= -5840; y++) for (int64_t z = 2374; z <= 2388; z++) {   // the nearest globular (`galaxy`), only its own sectors
            if (galaxyRegion(x, y, z) != REGION_CLUSTER) continue;
            Star s; if (!starInSector(x, y, z, s, false)) continue;
            globN++; if (s.cls == STAR_WOLF_RAYET) globW++;
        }
        Star w; bool found = false;
        for (int64_t x = 150; x < 300; x++) for (int64_t z = 20; z < 120; z++) {
            Star s; if (!starInSector(x, 0, z, s, false) || s.cls != STAR_WOLF_RAYET) continue;
            if (!found) { starInSector(x, 0, z, w, true); found = true; }
            StarSystem sys; sys.generate(s); ws++;
            bool any = false, hasBomb = false;
            for (const Body& b : sys.bodies) {
                if (b.type == PT_COMET || b.type == PT_COMPANION || (b.parent >= 0 && sys.bodies[b.parent].type == PT_COMPANION)) continue;
                worlds++; any = true;
                if (kindHas(b.type, KT_GIANT)) giants++;   // whatever its class (a far giant this warm an azure or a water-cloud giant)
                else if (KINDS[b.type].atmosphere) aired++;
                if (b.type == PT_BOMBARDED) hasBomb = true;
            }
            if (any) { withPlanets++; if (hasBomb) bomb++; }
            if (!sys.belts.empty()) withBelt++;
        }
        check("Wolf-Rayet stars: rare in the arm, none in a globular", armW > 0 && armW * 2 < armB && globN > 100 && globW == 0, fmt("near home %d against %d blue giants; %d of the globular's %d stars", armW, armB, globW, globN));
        check("Wolf-Rayet stars: bare worlds only, bombarded in most systems", ws >= 3 && aired == 0 && bomb * 2 >= withPlanets, fmt("%d stars, %d worlds (%d giants), %d with an atmosphere, bombarded in %d of %d systems with planets, %d with a belt", ws, worlds, giants, aired, bomb, withPlanets, withBelt));
        // the shell: the sun rendered straight ahead as from the second orbit; the shade along a radius peaks at the ring (five radii)
        // over the glow's skirt either side of it
        Framebuffer fb; fb.clear(); fb.clearDepth();
        Proj pj = Proj::fromHFov(60);
        const double angR = 0.03; Vec3 up(0, 1, 0);
        if (found) SpaceRenderer::drawSun(fb, w, Vec3(0, 0, 1), angR, 1234.0, 1.0, 1, false, 0, false, pj, &up);
        auto radial = [&](double mult) { double r = pj.f * std::tan(angR * mult), sum = 0; int n = 0; for (int k = 0; k < 360; k += 15) { int x = (int)(pj.cx + r * std::cos(k * DEG)), y = (int)(pj.cy + r * std::sin(k * DEG)); if (x < 0 || y < 0 || x >= FBW || y >= FBH) continue; sum += shadeOf(fb.at(x, y)); n++; } return n ? sum / n : 0.0; };
        double atRing = radial(5.0), inside = radial(3.2), outside = radial(7.0);
        check("Wolf-Rayet star: the shell's ring round the sun", found && atRing > inside + 3 && atRing > outside + 3, fmt("shade %.1f at 5 radii, %.1f at 3.2, %.1f at 7 (%s)", atRing, inside, outside, found ? w.name.c_str() : "no star"));
    }
    {   // the envelope the wind strips from a Wolf-Rayet star's giant (S-05), the user's (18071 1 -19950, body 1) from 2.5 radii: in
        // its own bank and dim (the discs heaped to the ramp's white in a blob behind it), hidden by the globe from the sun's side and
        // over the globe from the night side (a second tail drawn over the frame lights the globe's pixels only where it is in front)
        Star s; bool ok = starInSector(18071, 1, -19950, s, true);
        StarSystem sys; if (ok) sys.generate(s);
        ok = ok && sys.star.cls == STAR_WOLF_RAYET && sys.bodies.size() > 1 && sys.bodies[1].type == PT_GASGIANT;
        int white = 0, own[3] = {0, 0, 0}, gain[3] = {0, 0, 0}, globe[3] = {0, 0, 0};
        if (ok) {
            SpaceRenderer sr;
            const double t = 1000.0, R = sys.bodies[1].radiusKm;
            const Vec3 bp = sys.bodyPos(1, t), away = normalize(bp - sys.star.pos), side = normalize(cross(away, Vec3(0, 1, 0)));
            const Vec3 from[3] = {normalize(side - away), side, normalize(away * 0.8 + side * 0.6)};   // the sun's side, side-on, the night side
            sr.mapFor(sys.bodies[1]);
            for (int k = 0; k < 3; k++) {
                const Vec3 ship = bp + from[k] * (2.5 * R), f = normalize(bp - ship);
                SpaceContext c; c.sys = &sys; c.t = t; c.shipPos = ship; c.cam = cameraBasis(std::atan2(f.x, f.z), std::asin(f.y)); c.bankBodyA = 1; c.nebulae = false;
                Framebuffer fb;
                sr.setupPalette(fb, &sys, 1, -1, 1.0);
                sr.render(fb, c);
                for (int i = 0; i < FBW * FBH; i++) {   // no stars drawn: off the bodies only the band (bank 0), the tail and the sun's glow (bank 1)
                    if (fb.invz[i] > 1e-12f) { globe[k]++; continue; }
                    const int bk = bankOf(fb.idx[i]);
                    if ((bk == 0 || bk == 2) && shadeOf(fb.idx[i]) >= 48) white++;
                    else if (bk == 2 && shadeOf(fb.idx[i]) >= 6) own[k]++;
                }
                const std::vector<Pix> before = fb.idx;
                sr.drawStrippedTail(fb, c, 1, 2);
                for (int i = 0; i < FBW * FBH; i++) if (fb.invz[i] > 1e-12f && intenOf(fb.idx[i]) > intenOf(before[i]) + INTEN_PER_SHADE) gain[k]++;
            }
        }
        check("a Wolf-Rayet giant's stripped tail: its own dim cloud, behind the globe by day, over it by night",
              ok && white < 50 && own[1] > FBW * FBH / 50 && own[2] > FBW * FBH / 50 && gain[2] > globe[2] / 10 && gain[0] * 20 < gain[2],
              fmt("off the globe: %d at shade 48 and up in its bank or the band's, %d / %d / %d in its bank at 6-48 from the sun's side, side-on, the night side; a second tail lit %d / %d / %d of the globe's %d / %d / %d pixels",
                  white, own[0], own[1], own[2], gain[0], gain[1], gain[2], globe[0], globe[1], globe[2]));
    }
    {   // the black hole. A share of the pulsars that stayed, most in the core; systems of rubble and rocks with a close companion
        // in half; and the lens: a star right behind the hole becomes an Einstein ring, the shadow is black, the far sky untouched
        int armP = 0, armB = 0, armN = 0, coreN = 0, coreB = 0, bs = 0, worlds = 0, bad = 0, comps = 0, closeComps = 0, withBelt = 0;
        for (int64_t x = 150; x < 200; x++) for (int64_t y = -6; y < 6; y++) for (int64_t z = 20; z < 70; z++) { Star s; if (!starInSector(x, y, z, s, false)) continue; armN++; if (s.cls == STAR_PULSAR) armP++; if (s.cls == STAR_BLACK_HOLE) armB++; }
        for (int64_t x = 12293; x <= 12307; x++) for (int64_t y = -7; y <= 7; y++) for (int64_t z = -5507; z <= -5493; z++) {   // the core cube of `galaxy`
            if (galaxyRegion(x, y, z) != REGION_CORE) continue;
            Star s; if (!starInSector(x, y, z, s, false)) continue;
            coreN++; if (s.cls == STAR_BLACK_HOLE) coreB++;
        }
        Star bh; bool found = false;
        for (int64_t x = 150; x < 300; x++) for (int64_t z = 20; z < 120; z++) {
            Star s; if (!starInSector(x, 0, z, s, false) || s.cls != STAR_BLACK_HOLE) continue;
            if (!found) { starInSector(x, 0, z, bh, true); found = true; }
            StarSystem sys; sys.generate(s); bs++;
            for (const Body& b : sys.bodies) {
                if (b.type == PT_COMET || (b.parent >= 0 && sys.bodies[b.parent].type == PT_COMPANION)) continue;
                if (b.type == PT_COMPANION) { comps++; if (b.orbitRadiusKm < 1e8) closeComps++; continue; }
                worlds++;
                if (KINDS[b.type].atmosphere || b.type == PT_GASGIANT || b.type == PT_SUBSTELLAR || b.type == PT_MOLTEN) bad++;
            }
            if (!sys.belts.empty()) withBelt++;
        }
        double armShare = armN ? (double)armB / armN : 0, coreShare = coreN ? (double)coreB / coreN : 0;
        check("black holes: a few near home, several times as many in the core", armB > 0 && armB * 4 < armP && coreN > 1000 && coreShare > 2.5 * armShare, fmt("near home %d against %d pulsars (%.2f%% of %d stars); in the core %d of %d (%.2f%%)", armB, armP, 100 * armShare, armN, coreB, coreN, 100 * coreShare));
        check("black holes: rocks, rubble and a close companion", bs >= 3 && bad == 0 && comps > 0 && closeComps == comps && withBelt * 2 >= bs, fmt("%d stars, %d worlds, %d with an air, a giant or lava, %d companions (%d close), %d with a belt", bs, worlds, bad, comps, closeComps, withBelt));
        // the lens: a star dot straight behind the hole and the sky dark elsewhere; after the hole the dot is a ring at the Einstein
        // angle, the shadow is black and the sky outside the region keeps its dark
        Framebuffer fb; fb.clear(); fb.clearDepth();
        Proj pj = Proj::fromHFov(60);
        const double angR = 0.02, thE = std::sqrt(angR);
        int cx = (int)pj.cx, cy = (int)pj.cy;
        for (int dy = -2 * FB_SCALE; dy <= 2 * FB_SCALE; dy++) for (int dx = -2 * FB_SCALE; dx <= 2 * FB_SCALE; dx++) fb.at(cx + dx, cy + dy) = pix(0, 60);   // a dot of two logical pixels' radius
        Vec3 up(0, 1, 0);   // the disc edge-on, a line along x through the hole: the ring samples on that axis are skipped (the disc is opaque)
        auto t0 = std::chrono::steady_clock::now();
        if (found) SpaceRenderer::drawSun(fb, bh, Vec3(0, 0, 1), angR, 1234.0, 1.0, 1, false, 0, false, pj, &up);
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        int ringLit = 0, ringN = 0; double rE = pj.f * std::tan(thE);
        auto maxAround = [&](int x, int y) { double m = 0; for (int dy = -FB_SCALE; dy <= FB_SCALE; dy++) for (int dx = -FB_SCALE; dx <= FB_SCALE; dx++) { int xx = x + dx, yy = y + dy; if (xx < 0 || yy < 0 || xx >= FBW || yy >= FBH) continue; m = std::max(m, shadeOf(fb.at(xx, yy))); } return m; };
        for (int k = 30; k < 360; k += 30) { if (k == 180) continue; int x = (int)(pj.cx + rE * std::cos(k * DEG)), y = (int)(pj.cy + rE * std::sin(k * DEG)); if (x < 0 || y < 0 || x >= FBW || y >= FBH) continue; ringN++; if (maxAround(x, y) > 30) ringLit++; }
        double centre = shadeOf(fb.at(cx, cy)), shadowEdge = shadeOf(fb.at(cx + (int)(pj.f * std::tan(2.2 * angR)), cy));
        double far = shadeOf(fb.at(cx + (int)(pj.f * std::tan(3.0 * thE)), cy));
        check("black hole: the star behind it becomes an Einstein ring, the shadow black", found && ringN >= 8 && ringLit >= ringN - 1 && centre < 1 && shadowEdge < 1 && far < 1, fmt("%d of %d ring samples lit, the centre %.1f, the shadow's edge %.1f, far %.1f; %.1f ms at %dx (%s)", ringLit, ringN, centre, shadowEdge, far, ms, FB_SCALE, found ? bh.name.c_str() : "no black hole"));
    }
}

// W-01, W-06, B-413, W-03, W-04: the almanac, the telescope, the ground's sounds, the fronts, the showers
void unitSky(const Check& check) {
    {   // the almanac. On the three test places every event listed is where the geometry says: the site's own sun and
        // frame (`SurfaceSite::sun`, `localFrame`, `worldPos`) re-read at the time given, with the crossing's direction and the
        // extremum's neighbours; from the ship parked at the giant the same with the autopilot's lap; then a game runs to one
        std::vector<AlmanacTestPlace> places; almanacTestPlaces(places);
        check("almanac: the three test places", places.size() == 3, fmt("%zu found", places.size()));
        const double t0 = 3.6e6;
        auto genName = [](const StarSystem& sys) { return [&sys](int j) { return sys.bodies[j].name; }; };
        for (AlmanacTestPlace& tp : places) {
            const StarSystem& sys = tp.sys; int bi = tp.body;
            SurfaceSite site; site.init(&sys, bi, tp.latDeg * DEG, tp.lonDeg * DEG, t0);
            AlmanacPlace p = almanacGroundPlace(site, t0);
            std::vector<AlmanacEvent> ev; double ms0 = nowSec(); almanacOf(p, ev); double ms = (nowSec() - ms0) * 1000;
            int kinds[AL_COUNT] = {0}; int bad = 0; std::string why;
            auto altOf = [&](int j, double T, double& angR) {   // a body over the site's horizon, as the sky draws it
                Mat3 L = site.localFrame(T); Vec3 rel = sys.bodyPos(j, T) - site.worldPos(T, 0, 0, 0); double d = length(rel);
                angR = std::asin(clampd(sys.bodies[j].radiusKm / d, 0, 1)); return std::asin(clampd((L * (rel / d)).y, -1, 1));
            };
            auto litOf = [&](int j, double T) { Vec3 bp = sys.bodyPos(j, T); return 0.5 * (1 + dot(normalize(sys.star.pos - bp), normalize(sys.bodyPos(bi, T) - bp))); };
            for (const AlmanacEvent& e : ev) {
                double T = e.t, err = 0; bool ok = true; kinds[e.kind]++;
                switch (e.kind) {
                    case AL_SUNRISE: case AL_SUNSET: { SunInfo s = site.sun(T), sb = site.sun(T - 20), sa = site.sun(T + 20); err = std::fabs(s.altitude + s.angularRadius); ok = err < 2e-5 && ((sa.altitude > sb.altitude) == (e.kind == AL_SUNRISE)); break; }
                    case AL_NOON: { SunInfo s = site.sun(T); double q = std::fabs(sys.bodies[bi].rotPeriod) / 4; err = std::fabs(s.dayFraction - 0.5); ok = err < 1e-6 && s.altitude > site.sun(T - q).altitude && s.altitude > site.sun(T + q).altitude; break; }
                    case AL_DUSK: case AL_DAWN: { SunInfo s = site.sun(T), sb = site.sun(T - 20), sa = site.sun(T + 20); err = std::fabs(std::sin(s.altitude) + 0.12); ok = err < 2e-5 && ((sa.altitude > sb.altitude) == (e.kind == AL_DAWN)); break; }
                    case AL_RISE: case AL_SET: { double ar, arb, ara, a = altOf(e.body, T, ar), ab = altOf(e.body, T - 20, arb), aa = altOf(e.body, T + 20, ara); err = std::fabs(a + ar); ok = err < 2e-5 && ((aa > ab) == (e.kind == AL_RISE)); break; }
                    case AL_FULL: case AL_NEW: { double d = 0.01 * std::fabs(sys.bodies[e.body].orbitPeriod), k = litOf(e.body, T), kb = litOf(e.body, T - d), ka = litOf(e.body, T + d); err = std::fabs(k - e.value); ok = err < 1e-6 && (e.kind == AL_FULL ? (k >= kb && k >= ka) : (k <= kb && k <= ka)); break; }
                    case AL_ECLIPSE: {   // a disc wholly inside the sun's (a transit) or over it covers the discs' ratio, flat but for the body's
                        // distance: the time listed is the middle, the centres nearest (a solid world's moon far out is smaller than its sun)
                        SunInfo s = site.sun(T); err = std::fabs(s.eclipse - e.value);
                        auto sepOf = [&](double U, double& rb) { Mat3 L = site.localFrame(U); Vec3 rel = sys.bodyPos(e.body, U) - site.worldPos(U, 0, 0, 0); double d = length(rel);
                                                                 rb = std::asin(clampd(sys.bodies[e.body].radiusKm / d, 0, 1)); return std::acos(clampd(dot(site.sun(U).dirLocal, L * (rel / d)), -1, 1)); };
                        double rb, rbo, th = sepOf(T, rb);
                        const bool whole = th <= std::fabs(rb - s.angularRadius);
                        ok = err < 1e-6 && s.eclipse > 0 && (whole ? th <= sepOf(T - 30, rbo) && th <= sepOf(T + 30, rbo) : s.eclipse >= site.sun(T - 30).eclipse && s.eclipse >= site.sun(T + 30).eclipse);
                        break;
                    }
                    case AL_RING_SHADOW: ok = site.sun(T - 30).ringShadow < 1e-4 && site.sun(T + 30).ringShadow >= 1e-4; break;
                    case AL_RING_CLEAR: ok = site.sun(T - 30).ringShadow >= 1e-4 && site.sun(T + 30).ringShadow < 1e-4; break;
                    case AL_PERIAPSIS: { double M = sys.meanAnomaly(e.body, T); err = std::min(M, TAU - M); ok = err < 1e-6; break; }
                    case AL_APOAPSIS: err = std::fabs(sys.meanAnomaly(e.body, T) - PI); ok = err < 1e-6; break;
                    case AL_FRONT: case AL_FRONT_CLEAR: {   // the ground's own rain (or dust) is zero on one side of the moment and above zero on the other
                        std::vector<Front> fs; frontsOf(sys.bodies[bi], T, fs, 0); FrontWeather wb, wa;
                        Vec3 u = StarSystem::bodyFromLatLon(site.lat0, site.lon0);
                        frontWeatherAt(fs, u, T - 20, site.R, wb); frontWeatherAt(fs, u, T + 20, site.R, wa);
                        ok = e.kind == AL_FRONT ? (wb.precip == 0 && wa.precip > 0) : (wb.precip > 0 && wa.precip == 0); err = std::min(wb.precip, wa.precip); break;
                    }
                    default: ok = false; break;
                }
                if (!ok) { bad++; why += fmt(" [%s at +%.0f s, err %.2e]", almanacLabel(e, sys, genName(sys), " deg").c_str(), T - t0, err); }
            }
            std::string counts = fmt("sun %d/%d/%d twilight %d/%d rises %d sets %d full %d new %d eclipses %d ring %d/%d comets %d/%d", kinds[AL_SUNRISE], kinds[AL_SUNSET], kinds[AL_NOON], kinds[AL_DAWN], kinds[AL_DUSK],
                                     kinds[AL_RISE], kinds[AL_SET], kinds[AL_FULL], kinds[AL_NEW], kinds[AL_ECLIPSE], kinds[AL_RING_SHADOW], kinds[AL_RING_CLEAR], kinds[AL_PERIAPSIS], kinds[AL_APOAPSIS]);
            bool plain = std::string(tp.what) == "a plain world";
            bool enough = plain ? (kinds[AL_SUNRISE] && kinds[AL_SUNSET] && kinds[AL_NOON] && kinds[AL_DAWN] && kinds[AL_DUSK] && kinds[AL_RISE] && kinds[AL_SET] && kinds[AL_FULL] && kinds[AL_NEW]) : !ev.empty();
            check(fmt("almanac: %s", tp.what).c_str(), bad == 0 && enough, fmt("%s of %s (%lld 0 %lld): %zu events in %.1f ms: %s%s", sys.bodies[bi].name.c_str(), sys.star.name.c_str(), (long long)sys.star.sx, (long long)sys.star.sz, ev.size(), ms, counts.c_str(), why.c_str()));
        }
        if (!places.empty() && places[0].sys.bodies[places[0].body].parent >= 0) {   // the ship parked at the ringed giant
            const StarSystem& sys = places[0].sys; int B = sys.bodies[places[0].body].parent;
            AlmanacPlace p = almanacShipPlace(sys, B, t0);
            std::vector<AlmanacEvent> ev; double ms0 = nowSec(); almanacOf(p, ev); double ms = (nowSec() - ms0) * 1000;
            auto shipAt = [&](double T) { Vec3 dir = normalize(Mat3::axisAngle(sys.bodies[B].spinAxis, TAU / (600.0 + 200.0 * sys.bodies[B].radiusKm / 6000.0) * (T - t0)) * p.parkDir); return sys.bodyPos(B, T) + dir * p.parkDist; };   // the autopilot's lap
            auto sepOf = [&](int a, int b, double T) { Vec3 S = shipAt(T); return std::acos(clampd(dot(normalize(sys.bodyPos(a, T) - S), normalize(sys.bodyPos(b, T) - S)), -1, 1)); };
            auto angOf = [&](int a, double T) { return std::asin(clampd(sys.bodies[a].radiusKm / length(sys.bodyPos(a, T) - shipAt(T)), 0, 1)); };
            auto gapOf = [&](int axis, int point, double T, double reach) {   // the landing map's shadow rule: the point's centre against the line from the star through `axis`, beyond it
                Vec3 A = sys.bodyPos(axis, T), u = normalize(A - sys.star.pos), r = sys.bodyPos(point, T) - A; double along = dot(r, u);
                return (along > 0 ? std::sqrt(std::max(0.0, length2(r) - along * along)) : length(r)) - reach;
            };
            auto sunSep = [&](int j, double T, double& rs, double& rb) { Vec3 S = shipAt(T); Vec3 ds = sys.star.pos - S, db = sys.bodyPos(j, T) - S; rs = std::asin(clampd(sys.star.radiusKm / length(ds), 0, 1)); rb = std::asin(clampd(sys.bodies[j].radiusKm / length(db), 0, 1)); return std::acos(clampd(dot(normalize(ds), normalize(db)), -1, 1)); };
            int kinds[AL_COUNT] = {0}; int bad = 0; std::string why;
            for (const AlmanacEvent& e : ev) {
                double T = e.t, err = 0; bool ok = true; kinds[e.kind]++;
                switch (e.kind) {
                    case AL_TRANSIT: case AL_OCCULTATION: {
                        double f = sepOf(e.body, e.body2, T) - angOf(e.body, T) - angOf(e.body2, T), fa = sepOf(e.body, e.body2, T + 30) - angOf(e.body, T + 30) - angOf(e.body2, T + 30);
                        Vec3 S = shipAt(T); bool front = length(sys.bodyPos(e.body, T) - S) < length(sys.bodyPos(e.body2, T) - S);
                        err = std::fabs(f); ok = err < 1e-5 && fa < 0 && front == (e.kind == AL_TRANSIT); break;
                    }
                    case AL_SHADOW: { double R = sys.bodies[e.body2].radiusKm + sys.bodies[e.body].radiusKm; ok = gapOf(e.body, e.body2, T - 30, R) > 0 && gapOf(e.body, e.body2, T + 30, R) < 0; err = std::fabs(gapOf(e.body, e.body2, T, R)); break; }
                    case AL_ECLIPSED: { double R = sys.bodies[e.body2].radiusKm; ok = gapOf(e.body2, e.body, T - 30, R) > 0 && gapOf(e.body2, e.body, T + 30, R) < 0; err = std::fabs(gapOf(e.body2, e.body, T, R)); break; }
                    case AL_CONJUNCTION: {   // read from the parked body's centre (the lap would put a minimum into every orbit)
                        auto sepC = [&](double T2) { Vec3 S = sys.bodyPos(B, T2); return std::acos(clampd(dot(normalize(sys.bodyPos(e.body, T2) - S), normalize(sys.bodyPos(e.body2, T2) - S)), -1, 1)); };
                        double d = 0.005 * std::min(std::fabs(sys.bodies[e.body].orbitPeriod), std::fabs(sys.bodies[e.body2].orbitPeriod)), s = sepC(T); err = std::fabs(s - e.value); ok = err < 1e-6 && s < 2 * DEG && s <= sepC(T - d) && s <= sepC(T + d); break; }
                    case AL_SUN_TRANSIT: { double rs, rb, th = sunSep(e.body, T, rs, rb), r2, r3, th2 = sunSep(e.body, T + 20, r2, r3); err = std::fabs(th - rs - rb); ok = err < 1e-5 && th2 < r2 + r3; break; }
                    case AL_PERIAPSIS: { double M = sys.meanAnomaly(e.body, T); err = std::min(M, TAU - M); ok = err < 1e-6; break; }
                    case AL_APOAPSIS: err = std::fabs(sys.meanAnomaly(e.body, T) - PI); ok = err < 1e-6; break;
                    default: ok = false; break;
                }
                if (!ok) { bad++; why += fmt(" [%s at +%.0f s, err %.2e]", almanacLabel(e, sys, genName(sys), " deg").c_str(), T - t0, err); }
            }
            check("almanac: from the ship at the giant", bad == 0 && (kinds[AL_TRANSIT] || kinds[AL_OCCULTATION] || kinds[AL_SHADOW] || kinds[AL_ECLIPSED]),
                  fmt("%s (%d moons): %zu events in %.1f ms: across %d behind %d shadow %d eclipsed %d meetings %d sun %d comets %d/%d%s", sys.bodies[B].name.c_str(), sys.bodies[B].moonCount, ev.size(), ms,
                      kinds[AL_TRANSIT], kinds[AL_OCCULTATION], kinds[AL_SHADOW], kinds[AL_ECLIPSED], kinds[AL_CONJUNCTION], kinds[AL_SUN_TRANSIT], kinds[AL_PERIAPSIS], kinds[AL_APOAPSIS], why.c_str()));
        }
        if (places.size() == 3) {   // the run: on the plain world's surface the clock lands on the first event (within a second), the free row runs for an hour, T stops a run
            TestGame game; game.savePrefix = "shots/tests/unit_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_guide.txt";
            applyTestScale(game);
            Input in;
            auto press = [&](int key, bool ctrl = false) { in.down[KEY_LEFT_CONTROL] = ctrl; in.pressed[key] = true; in.down[key] = true; game.frame(in, 1.0 / 30); in.newFrame(); in.down[key] = false; in.down[KEY_LEFT_CONTROL] = false; };
            auto run = [&](double s) { runFrames(game, in, s); };
            const AlmanacTestPlace& tp = places[2];
            press(KEY_ENTER); run(0.1);
            game.testParkAt(tp.sys.star, tp.body); run(0.3);
            press(KEY_T, true); run(0.1);
            int mapEvents = game.testAlmanacEvents(); std::string mapInfo = game.testAlmanacInfo();
            press(KEY_ESCAPE); run(0.1);
            game.testLandSite(); press(KEY_ENTER); run(8.0);
            bool onSurface = game.state == GameState::SURFACE;
            press(KEY_T, true); run(0.1);
            int n = game.testAlmanacEvents(); std::string info = game.testAlmanacInfo();
            bool opened = game.state == GameState::ALMANAC && game.testAlmanacSelect(-1);
            press(KEY_ENTER);
            double target = game.testAlmanacTarget(), tStart = game.gameTime(), peak = 0; int frames = 0;
            while (game.testAlmanacRunning() && frames++ < 30 * 300) { game.frame(in, 1.0 / 30); in.newFrame(); peak = std::max(peak, game.testTimeWarp()); }
            double err = game.gameTime() - target;
            check("almanac: the run lands on the event", onSurface && opened && n > 0 && mapEvents > 0 && !game.testAlmanacRunning() && std::fabs(err) < 1.0 && game.testTimeWarp() == 1,
                  fmt("%d events at the map's cursor, %d under the feet (%s); the first, %.0f s ahead, reached in %d frames at a peak of x%.0f, %.3f s off, warp x%.0f, '%s'", mapEvents, n, info.c_str(), target - tStart, frames, peak, err, game.testTimeWarp(), game.testStatus().c_str()));
            press(KEY_T, true); run(0.1); press(KEY_UP); press(KEY_ENTER);   // the last row is the free one: an hour by default
            target = game.testAlmanacTarget(); tStart = game.gameTime(); frames = 0;
            while (game.testAlmanacRunning() && frames++ < 30 * 120) { game.frame(in, 1.0 / 30); in.newFrame(); }
            check("almanac: the free row runs for an hour", std::fabs(target - tStart - 3600) < 1e-6 && !game.testAlmanacRunning() && std::fabs(game.gameTime() - target) < 1.0,
                  fmt("%.0f s asked, %.3f s off after %d frames, '%s'", target - tStart, game.gameTime() - target, frames, game.testStatus().c_str()));
            press(KEY_T, true); run(0.1); press(KEY_UP); press(KEY_ENTER); run(0.3);
            bool wasRunning = game.testAlmanacRunning(); press(KEY_T); run(0.1);
            check("almanac: T stops a run", wasRunning && !game.testAlmanacRunning() && game.testTimeWarp() == 1, fmt("'%s'", game.testStatus().c_str()));
            remove("shots/tests/unit_guide.txt");
        }
    }
    {   // the telescope in a game parked at the home world: the magnification multiplies the disc exactly, the world is tracked
        // from the start and held on the reticle while the ship orbits, the mouse steers the offset at the rate of the magnification,
        // Enter lets go, N names what the reticle rests on, the photo's caption says so, the sun is read by its class, Z stows
        TestGame game; game.savePrefix = "shots/tests/unit_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_guide.txt";
        applyTestScale(game);
        Input in;
        auto press = [&](int key) { pressKey(game, in, key); };
        auto run = [&](double s) { runFrames(game, in, s); };
        press(KEY_ENTER); run(0.3);
        int B = game.testParkedBody();
        game.testAimAtBody(B); run(0.1);
        double sx0 = 0, sy0 = 0, r0 = 0, sx = 0, sy = 0, r = 0;
        bool front0 = game.testBodyScreen(B, sx0, sy0, r0);
        press(KEY_Z); run(0.1);
        game.testBodyScreen(B, sx, sy, r);
        std::string line = game.testTelescopeLine();
        check("telescope: x2 doubles the disc and tracks the world", B >= 0 && front0 && game.testTelescopeOn() && game.testTelescopeZoom() == 2 && std::fabs(r / r0 - 2) < 0.01 && game.testTelescopeTrack() == B && game.testTelescopeTarget() == B && line.rfind("UNKNOWN", 0) == 0 && line.size() <= 52,
              fmt("body %d: disc %.1f px at x1, %.1f at x2; %s", B, r0, r, game.testTelescopeInfo().c_str()));
        press(KEY_EQUAL); run(2.0); game.testBodyScreen(B, sx, sy, r);   // x4: the disc still under twice the frame, the centre tracked
        double drift = std::sqrt((sx - FBW / 2.0) * (sx - FBW / 2.0) + (sy - FBH / 2.0) * (sy - FBH / 2.0));
        check("telescope: the stabiliser holds the world on the reticle", game.testTelescopeTrack() == B && game.testTelescopeStabilised() && drift < 1.5 * FB_SCALE, fmt("centre %.2f px off after 2 s at x4; %s", drift, game.testTelescopeInfo().c_str()));
        for (int i = 0; i < 2; i++) press(KEY_EQUAL);
        run(0.1); game.testBodyScreen(B, sx, sy, r);
        check("telescope: the steps are powers of two", game.testTelescopeZoom() == 16 && std::fabs(r / r0 - 16) < 0.01, fmt("x%.0f, disc %.1f px (%.1f at x1)", game.testTelescopeZoom(), r, r0));
        {   // R-405, the stabiliser: at a deep power the ground under the reticle and the ground under the ship both stay where they are (no lap, no turn of the world under the ship), and no plate is begun again
            double glat = 0, glon = 0, slat0 = 0, slon0 = 0; bool g0 = game.testTelescopeGround(glat, glon), s0 = game.testSubShip(slat0, slon0);
            double sxc = 0, syc = 0, rc = 0; game.testBodyScreen(B, sxc, syc, rc);   // the centre as it stands (the lap carried it a pixel off the reticle in the 0.1 s between the aim and Z)
            long begun0 = game.testPlateBegun();
            run(2.0); game.testBodyScreen(B, sx, sy, r);
            double gx = 0, gy = 0, slat1 = 0, slon1 = 0; bool gs = game.testGroundScreen(B, glat, glon, gx, gy); game.testSubShip(slat1, slon1);
            double gdrift = std::sqrt((gx - FBW / 2.0) * (gx - FBW / 2.0) + (gy - FBH / 2.0) * (gy - FBH / 2.0)), cdrift = std::sqrt((sx - sxc) * (sx - sxc) + (sy - syc) * (sy - syc));
            double smove = std::acos(clampd(dot(StarSystem::bodyFromLatLon(slat0, slon0), StarSystem::bodyFromLatLon(slat1, slon1)), -1, 1)) / DEG;
            check("telescope: the stabiliser holds the ground under the reticle and the ship over its ground", game.testTelescopeTrack() == B && game.testTelescopeStabilised() && g0 && s0 && gs && gdrift < 0.1 && cdrift < 0.5 * FB_SCALE && smove < 1e-5 && game.testPlateBegun() == begun0,
                  fmt("after 2 s at x16 the ground under the reticle is %.3f px off, the world's centre moved %.2f px, the ship %.2e deg off its ground, %ld plates begun; %s", gdrift, cdrift, smove, game.testPlateBegun() - begun0, game.testTelescopeInfo().c_str()));
        }
        double sxb = 0, syb = 0, rb = 0; game.testBodyScreen(B, sxb, syb, rb);   // the centre before the move
        in.mouseDx = 100; game.frame(in, 1.0 / 30); in.newFrame(); run(0.05);
        double sx1 = 0, sy1 = 0, r1 = 0; game.testBodyScreen(B, sx1, sy1, r1);
        double expYaw = 100 * 0.0032 * game.settings.mouseSensitivity / 16, f16 = (FBW * 0.5) / std::tan(game.settings.fovDeg * 0.5 * DEG) * 16;
        double expPx = f16 * std::tan(expYaw);
        check("telescope: the mouse steers at the rate of the magnification", std::fabs(std::fabs(sx1 - sxb) - expPx) < 0.05 * expPx + 1.5,
              fmt("100 counts at x16: %.4f rad expected, the world's centre moved %.1f px (%.1f expected); %s", expYaw, sx1 - sxb, expPx, game.testTelescopeInfo().c_str()));
        double sxe = 0, sye = 0, re = 0; game.testBodyScreen(B, sxe, sye, re);
        press(KEY_ENTER); run(2.0); game.testBodyScreen(B, sx, sy, r);
        double drift2 = std::sqrt((sx - sxe) * (sx - sxe) + (sy - sye) * (sy - sye));
        check("telescope: Enter lets go and the stabiliser still holds the view", game.testTelescopeTrack() == -1 && game.testTelescopeStabilised() && drift2 < 0.5 * FB_SCALE, fmt("centre %.2f px moved after 2 s untracked; %s", drift2, game.testTelescopeInfo().c_str()));
        game.testAimAtBody(B); run(0.1);
        press(KEY_N); bool entry = game.state == GameState::TEXT_ENTRY;
        game.testTypeText("HOMEWORLD"); run(0.1);   // the typing ends with Enter
        line = game.testTelescopeLine();
        std::string caption = game.screenshotCaption();
        check("telescope: N names the target and the photo says so", entry && game.state == GameState::SPACE && line.rfind("HOMEWORLD", 0) == 0 && caption.rfind("THROUGH THE TELESCOPE X16: HOMEWORLD", 0) == 0 && line.size() <= 52,
              fmt("'%s'; caption '%s'", line.c_str(), caption.c_str()));
        game.testAimAtSun(); run(0.1);
        line = game.testTelescopeLine();
        check("telescope: the sun on the reticle", game.testTelescopeTarget() == -2 && line.find("STAR") != std::string::npos && line.size() <= 52, fmt("target %d '%s'", game.testTelescopeTarget(), line.c_str()));
        press(KEY_Z); run(0.1); game.testAimAtBody(B); run(0.1); game.testBodyScreen(B, sx, sy, r);
        check("telescope: Z stows and the view is the setting's again", !game.testTelescopeOn() && std::fabs(r / r0 - 1) < 0.01, fmt("disc %.1f px (%.1f before); %s", r, r0, game.testTelescopeInfo().c_str()));
        {   // stowed, the lap goes on from where the ship is; with Shift held the arrows carry the parked ship round its world (east along the parallel, north along the meridian, twenty degrees a second) with the world kept in the window; the telescope opened then holds the side faced
            Vec3 pd0 = game.testParkDir(); run(1.0); Vec3 pd1 = game.testParkDir();
            double lapDeg = std::acos(clampd(dot(pd0, pd1), -1, 1)) / DEG;
            check("telescope: stowed, the lap goes on", lapDeg > 0.2, fmt("the parking moved %.2f deg in a second", lapDeg));
            press(KEY_O); run(0.05);   // the fixed-point chase: the parking still but for the keys
            game.testAimAtBody(B); run(0.1);
            double sxA = 0, syA = 0, rA = 0; game.testBodyScreen(B, sxA, syA, rA);
            Vec3 pdA = game.testParkDir(), ax = game.testSpinAxis(B);
            in.down[KEY_LEFT_SHIFT] = true; in.down[KEY_RIGHT] = true; run(1.0); in.down[KEY_RIGHT] = false; in.down[KEY_LEFT_SHIFT] = false;
            Vec3 pdB = game.testParkDir();
            Vec3 pa = normalize(pdA - ax * dot(pdA, ax)), pb = normalize(pdB - ax * dot(pdB, ax));
            double turned = std::atan2(dot(cross(pa, pb), ax), dot(pa, pb)) / DEG;
            double sxB = 0, syB = 0, rB = 0; game.testBodyScreen(B, sxB, syB, rB);
            double kept = std::sqrt((sxB - sxA) * (sxB - sxA) + (syB - syA) * (syB - syA));
            std::string over = game.testStatus();
            check("telescope: Shift+Right carries the ship round the world, the world kept in the window", std::fabs(turned - 20) < 0.5 && kept < 1.5 * FB_SCALE && over.rfind("OVER ", 0) == 0,
                  fmt("a second of Shift+Right: %.2f deg east round the spin axis, the world's centre moved %.2f px, '%s'", turned, kept, over.c_str()));
            double slatB = 0, slonB = 0, slatC = 0, slonC = 0; game.testSubShip(slatB, slonB);
            in.down[KEY_LEFT_SHIFT] = true; in.down[KEY_UP] = true; run(1.0); in.down[KEY_UP] = false; in.down[KEY_LEFT_SHIFT] = false;
            game.testSubShip(slatC, slonC);
            check("telescope: Shift+Up carries the ship north", std::fabs((slatC - slatB) / DEG - 20) < 0.5, fmt("a second of Shift+Up: from %.1f to %.1f deg of latitude", slatB / DEG, slatC / DEG));
            game.testAimAtBody(B); run(0.1);
            press(KEY_Z); run(0.05);
            double slatD = 0, slonD = 0, slatE = 0, slonE = 0; game.testSubShip(slatD, slonD);
            run(1.0); game.testSubShip(slatE, slonE); game.testBodyScreen(B, sx, sy, r);
            double smove = std::acos(clampd(dot(StarSystem::bodyFromLatLon(slatD, slonD), StarSystem::bodyFromLatLon(slatE, slonE)), -1, 1)) / DEG;
            double cdrift = std::sqrt((sx - FBW / 2.0) * (sx - FBW / 2.0) + (sy - FBH / 2.0) * (sy - FBH / 2.0));
            check("telescope: opened after the turn, it holds the side faced", game.testTelescopeStabilised() && game.testTelescopeTrack() == B && smove < 1e-5 && cdrift < 0.5 * FB_SCALE,
                  fmt("over %.1f %.1f, %.2e deg off after a second, the world's centre %.2f px off; %s", slatD / DEG, slonD / DEG, smove, cdrift, game.testTelescopeInfo().c_str()));
            press(KEY_Z); run(0.05); press(KEY_O); run(0.05);   // stowed, the orbit mode again
        }
        {   // the user's review: the plate. The eyepiece's body gets the planet function sampled at the eyepiece's own footprint,
            // built within a budget of samples a frame (the first frame coarse, the plate whole after some), its cells the function's
            // own numbers; a zoom step begins a finer plate and keeps the last as the fallback; P reads the plate out whole for the photo
            press(KEY_Z); run(0.05);   // the home world again at x16 (the step survives a stow: the plate of x16 is still held), tracked
            press(KEY_EQUAL);          // x32: a power never used, so a plate of its own begins in this frame
            double prog0 = game.testTelescopePlateProgress();
            std::string info0 = game.testTelescopePlate();
            double mapTexelM = TAU * game.testBodyRadiusKm(B) * 1000.0 / PlanetMap::W;
            check("telescope: the plate begins coarse within its first frame", prog0 > 0 && prog0 < 1 && info0.find("blocks of") != std::string::npos, fmt("%.0f%% after one frame at x%.0f: %s", prog0 * 100, game.testTelescopeZoom(), info0.c_str()));
            run(2.0);
            double prog1 = game.testTelescopePlateProgress();
            std::string info1 = game.testTelescopePlate();
            double maxAlb = 0, maxH = 0; int matMiss = 0;
            game.testPlateCheck(40, maxAlb, matMiss, maxH);
            size_t at = info1.find(" cells of "); double texelM = at != std::string::npos ? atof(info1.c_str() + at + 10) : 0;
            check("telescope: the plate is whole within two seconds and finer than the world map", prog1 >= 1 && texelM > 0 && texelM < mapTexelM * 0.5, fmt("%.0f%%, %.0f m a cell against the map's %.0f: %s", prog1 * 100, texelM, mapTexelM, info1.c_str()));
            check("telescope: the plate's cells are the planet function's own numbers", matMiss == 0 && maxAlb < 0.003 && maxH < 0.01, fmt("40 cells sampled again: albedo off by %.4f at most, %d materials differ, heights by %.3f m", maxAlb, matMiss, maxH));
            press(KEY_EQUAL); run(0.05);   // x64: a finer plate begins, the x32 one the fallback
            std::string info2 = game.testTelescopePlate();
            double prog2 = game.testTelescopePlateProgress();
            check("telescope: a zoom step begins a finer plate and keeps the last as the fallback", prog2 < 1 && info2.find("the previous") != std::string::npos && info2.find("100%") != std::string::npos, fmt("%.0f%%: %s", prog2 * 100, info2.c_str()));
            press(KEY_P); double prog3 = game.testTelescopePlateProgress(); bool photo = game.wantsScreenshot; game.wantsScreenshot = false;
            check("telescope: P reads the plate out whole for the photo", photo && prog3 >= 1, fmt("%.0f%% after the P frame: %s", prog3 * 100, game.testTelescopePlate().c_str()));
            press(KEY_Z); run(0.05);
        }
        {   // the user's review: the telescope's own set on the cabin's back wall (E looks through it), and R-404: every computer in its own colour
            game.testCabinGoto(1.55, -1.2, PI); run(0.2);
            int facing = game.testCabinFacing();
            press(KEY_E); run(0.1); bool on = game.testTelescopeOn();
            press(KEY_Z); run(0.05);
            check("cabin: the telescope's set on the back wall, E looks through it", facing == 9 && on && !game.testTelescopeOn(), fmt("facing %d, E: %s, Z: %s", facing, on ? "the eye at the eyepiece" : "nothing", game.testTelescopeOn() ? "still on" : "stowed"));
            double minDist = 0; int n = game.testScreenColours(minDist);
            check("cabin: every computer in a colour of its own", n == 10 && minDist > 0.3, fmt("%d computers, the instruments' case (C-11) and the drone bay (C-18), the nearest two %.2f apart in RGB", n, minDist));
        }
        {   // the light through a glass hull. Y makes the hull glass (the outside tinted through the walls, the floor and the
            // ceiling); what stands in the room stays solid, U lights it and catches a sheen on the glass, and says the hull is glass.
            // Before, the consoles and the desks went to glass with the walls and U changed nothing to be seen: the cabin looked dark
            // and broken whatever the light
            auto frameOf = [&](double& share, double& lum) {   // the hull bank's share of the frame and the frame's mean brightness
                int n = 0; double sum = 0;
                const uint32_t* px = game.output();
                for (int i = 0; i < FBW * FBH; i++) { if (bankOf(game.fb.idx[i]) == 8) n++; const uint32_t c = px[i]; sum += ((c & 255) + ((c >> 8) & 255) + ((c >> 16) & 255)) / 3.0; }
                share = (double)n / (FBW * FBH); lum = sum / (FBW * FBH);
            };
            game.testCabinGoto(0.3, 0.0, -PI / 2, -0.35);   // toward the GOES console from the cage, the head bent to its desk
            const int before = game.testCabinLights();
            if (before & 2) press(KEY_Y);
            if (!(before & 1)) press(KEY_U);
            run(1.5); double sPol, lPol; frameOf(sPol, lPol);                                                       // the hull polarised, the light on
            press(KEY_Y); run(1.5); double sOn, lOn; frameOf(sOn, lOn);                                             // glass, the light on
            press(KEY_U); const std::string said = game.testStatus(); run(2.0); double sOff, lOff; frameOf(sOff, lOff);   // glass, the light off
            press(KEY_U); press(KEY_Y); run(3.0); double sBack, lBack; frameOf(sBack, lBack);                       // polarised and lit again (the light settled)
            check("cabin: a glass hull shows the outside, what stands in the room stays solid (B-411)", sOff < sPol * 0.7 && sOff > 0.08 && game.testCabinLights() == 1 && std::fabs(sBack - sPol) < 0.02 && std::fabs(lBack - lPol) < 0.03 * lPol,
                  fmt("the hull's bank %.0f%% of the frame polarised, %.0f%% glass with the light off (the console and the cage), %.0f%% polarised again; %.1f bright polarised, %.1f again", sPol * 100, sOff * 100, sBack * 100, lPol, lBack));
            check("cabin: the light works through a glass hull, and U says the hull is glass (B-411)", lOn > lOff * 1.5 && sOn > sOff && said.find("GLASS") != std::string::npos,
                  fmt("glass: %.1f bright with the light on, %.1f off; the hull's bank %.0f%% of the frame lit; U said \"%s\"", lOn, lOff, sOn * 100, said.c_str()));
        }
        remove("shots/tests/unit_guide.txt");
    }
    {   // the ground's sounds stay on the ground. Landed on the home world, a sprint winds the explorer (the breathing) in the
        // wind (and whatever else the place has: waves, birds, leaves, hooves, insects); Q at the capsule, and neither the capsule's
        // climb nor the ship hears any of it, nor a save of the ship loaded on the ground. Before, the ship silenced the wind, the rain
        // and the lava only: the rest went on aboard at its last level until another landing, whatever was loaded
        remove("shots/tests/test_b413_guide.txt");
        TestGame game; game.savePrefix = "shots/tests/test_b413_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/test_b413_guide.txt";
        applyTestScale(game);
        Input in;
        auto press = [&](int key) { pressKey(game, in, key); };
        auto run = [&](double s) { runFrames(game, in, s); };
        // the levels the ground sets, and what they sound like: the game's sound through a fresh synth against the same hum and drive
        // alone, two seconds of each (the one-shots aside: a running synth takes them at once)
        static const char* const GROUND_NAMES[16] = {"wind", "rain", "lava", "breath", "waves", "birds", "leaves", "hooves", "insects", "gear", "skid", "pods", "dunes", "ice", "quake", "quake's onset"};   // the ground's songs
        auto levels = [](const AudioState& a, double* v) { const double l[16] = {a.wind, a.rain, a.lava, a.breath, a.surf, a.birds, a.leaves, a.hoofs, a.insects, a.engineRpm, a.skid, a.rotor, a.dunes, a.ice, a.quake, a.quakeHit}; for (int i = 0; i < 16; i++) v[i] = l[i]; };
        auto heard = [&](const AudioState& a) { double v[16]; levels(a, v); std::string s; for (int i = 0; i < 16; i++) if (v[i] != 0) s += fmt("%s%s %.2f", s.empty() ? "" : " ", GROUND_NAMES[i], v[i]); return s.empty() ? std::string("none") : s; };
        auto quiet = [&](const AudioState& a) { double v[16]; levels(a, v); for (int i = 0; i < 16; i++) if (v[i] != 0) return false; return true; };
        auto groundRms = [](AudioState a) {
            AudioState ship; ship.hum = a.hum; ship.engine = a.engine; ship.master = a.master;
            a.beep = 0; a.thunder = 0; a.step = 0; a.call = 0; a.thud = 0;
            AudioSynth s1, s2; std::vector<float> b1(2048), b2(2048); double sum = 0; long n = 0;
            for (int k = 0; k < 22; k++) { s1.render(b1.data(), 2048, 22050, a); s2.render(b2.data(), 2048, 22050, ship); for (int i = 0; i < 2048; i++) { const double d = (double)b1[i] - b2[i]; sum += d * d; n++; } }
            return std::sqrt(sum / n);
        };
        press(KEY_ENTER); run(0.3);
        game.saveSlot(1);                                                                          // the ship, for the load from the ground below
        press(KEY_C); run(0.3); game.testLandSite(); run(0.2); press(KEY_ENTER); run(7.5);
        const bool landed = game.state == GameState::SURFACE;
        in.down[KEY_LEFT_SHIFT] = true; in.down[KEY_W] = true; run(5.0); in.down[KEY_LEFT_SHIFT] = false; in.down[KEY_W] = false;   // a sprint: winded
        game.saveSlot(2);                                                                          // the ground, winded
        press(KEY_K); run(0.2); press(KEY_Q);                                                      // the capsule called, and boarded
        const AudioState ground = game.audio; const bool boarded = game.state == GameState::ASCENT;
        run(1.0); const AudioState climbing = game.audio;
        run(7.0); const AudioState aboard = game.audio; const bool back = game.state == GameState::SPACE;
        game.loadSlot(2); run(0.5); const AudioState again = game.audio; const bool onGround = game.state == GameState::SURFACE;
        game.loadSlot(1); run(1.0); const AudioState loaded = game.audio; const bool inShip = game.state == GameState::SPACE;
        const double rmsGround = groundRms(ground), rmsAboard = groundRms(aboard), rmsLoaded = groundRms(loaded);
        std::string detail = "on the ground: " + heard(ground) + fmt(" (%.4f rms over the hum); climbing: ", rmsGround) + heard(climbing) + "; aboard: " + heard(aboard) + fmt(" (%.4f)", rmsAboard);
        detail += "; on the ground again (a load): " + heard(again) + "; the ship's save loaded there: " + heard(loaded) + fmt(" (%.4f)", rmsLoaded);
        if (!(landed && boarded && back && onGround && inShip)) detail += fmt("; landed %d, boarded %d, back %d, on the ground again %d, in the ship %d", landed, boarded, back, onGround, inShip);
        check("sound: the ground's stay on the ground (B-413)", landed && boarded && back && onGround && inShip && ground.breath > 0.2 && ground.wind > 0 && again.wind > 0 && rmsGround > 0.002 &&
              quiet(climbing) && quiet(aboard) && quiet(loaded) && rmsAboard < 1e-9 && rmsLoaded < 1e-9, detail);
        for (const char* f : {"shots/tests/test_b413_save_1.txt", "shots/tests/test_b413_save_2.txt", "shots/tests/test_b413_save_auto.txt", "shots/tests/test_b413_guide.txt"}) remove(f);
    }
    {   // the fronts. On the pinned plain of the first felisian world the next front's forecast is the moment the ground's own
        // rain begins (the front's rain zero twenty seconds before the time given and above zero twenty after, the other way round
        // at the clearing), the almanac lists the same two moments, the map's band (the globe's overlay) agrees with the ground's
        // cloud under it, and a game landed there reads the forecast on its data sheet with the front's line on its sector map
        StarSystem sys; int bi = -1; FrontForecast fc;
        const double t0 = 3.6e6;
        bool found = pinnedPlainFront(sys, bi, t0, fc, 10 * 86400.0);
        check("fronts: a front reaches the pinned plain within ten days", found, found ? fmt("%s: from the %s in %s, strength %.2f, gain %.2f, %s of rain", sys.bodies[bi].name.c_str(), frontCompass(fc.fromBearing), countdownString(fc.tArrive - t0).c_str(), fc.strength, fc.gain, countdownString(fc.tClear - fc.tArrive).c_str()) : "none");
        if (found) {
            const Body& b = sys.bodies[bi];
            SurfaceView sv;
            sv.init(&sys, bi, PIN_PLAIN_LAT * DEG, PIN_PLAIN_LON * DEG, fc.tArrive - 60);
            auto envAt = [&](double T) { Input in; sv.update(0.016, in, T, false); return sv.env; };
            SurfaceEnvironment eb = envAt(fc.tArrive - 20), ea = envAt(fc.tArrive + 20), ec = envAt(fc.tArrive + 1800), e0 = envAt(fc.tArrive);
            check("fronts: the forecast's arrival is the moment the rain begins", eb.frontRain == 0 && ea.frontRain > 0 && std::fabs(e0.frontAhead) < 50 && eb.frontCloud > 0.5 && ec.frontRain > 0.2,
                  fmt("the line %.1f m from the place at the time given; 20 s before: rain %.4f, cloud %.2f, wind %.0f kt, temp %+.1f C; 20 s after: rain %.1e; 30 min after: rain %.2f, temp %+.1f C, fog %.0f m", e0.frontAhead, eb.frontRain, eb.frontCloud, eb.windKnots, eb.temperatureC, ea.frontRain, ec.frontRain, ec.temperatureC, ec.fogDistance));
            SurfaceEnvironment cb = envAt(fc.tClear - 20), ca = envAt(fc.tClear + 20);
            check("fronts: the clearing is the moment the rain ends", cb.frontRain > 0 && ca.frontRain == 0, fmt("20 s before the clearing: rain %.1e; 20 s after: %.1e, cold %.1f C, cloud %.2f", cb.frontRain, ca.frontRain, ca.frontCold, ca.frontCloud));
            AlmanacPlace p = almanacGroundPlace(sv.site, t0); p.window = 10 * 86400.0;
            std::vector<AlmanacEvent> ev; almanacOf(p, ev);
            double dA = 1e9, dC = 1e9; std::string label;
            for (const AlmanacEvent& e : ev) {
                if (e.kind == AL_FRONT && std::fabs(e.t - fc.tArrive) < dA) { dA = std::fabs(e.t - fc.tArrive); label = almanacLabel(e, sys, [&](int j) { return sys.bodies[j].name; }, " deg"); }
                if (e.kind == AL_FRONT_CLEAR) dC = std::min(dC, std::fabs(e.t - fc.tClear));
            }
            check("fronts: the almanac lists the arrival and the clearing", dA < 1e-3 && dC < 1e-3, fmt("'%s' %.1e s off the forecast, the clearing %.1e s off; %zu events in ten days", label.c_str(), dA, dC, ev.size()));
            double tBand = fc.tArrive - 3600;
            std::vector<uint8_t> fm; buildFrontMap(b, tBand, PlanetMap::W, PlanetMap::H, fm);
            double band = frontMapAt(fm, PlanetMap::W, PlanetMap::H, PIN_PLAIN_LON * DEG, PIN_PLAIN_LAT * DEG);
            SurfaceEnvironment em = envAt(tBand);
            int covered = 0; for (uint8_t v : fm) if (v > 128) covered++;
            // the texel over the plain against the ground's own function at that texel's centre (the overlay is the function on the
            // map's grid), and the bilinear read at the plain against the ground within what 73 km texels can hold of a 10 km wall edge
            int tx = (int)((wrap2pi(PIN_PLAIN_LON * DEG + PI) / TAU) * PlanetMap::W), ty = (int)((0.5 - PIN_PLAIN_LAT * DEG / PI) * PlanetMap::H);
            double latC = (0.5 - (ty + 0.5) / PlanetMap::H) * PI, lonC = ((tx + 0.5) / PlanetMap::W) * TAU - PI;
            std::vector<Front> fsB; frontsOf(b, tBand, fsB, 0); FrontWeather wC; frontWeatherAt(fsB, StarSystem::bodyFromLatLon(latC, lonC), tBand, b.radiusKm * 1000.0, wC);
            double texel = fm[(size_t)ty * PlanetMap::W + tx] / 255.0;
            check("fronts: the map's band and the ground agree", band > 0.5 && std::fabs(texel - wC.cloud) < 1.5 / 255 && std::fabs(band - em.frontCloud) < 0.4,
                  fmt("the texel over the plain reads %.3f, the function at its centre %.3f; the overlay read at the plain %.2f an hour before the line, the ground's front cloud there %.2f (the pattern's %.2f); bands over %.1f%% of the map", texel, wC.cloud, band, em.frontCloud, em.cloudPattern, 100.0 * covered / fm.size()));
            TestGame game; game.savePrefix = "shots/tests/unit_save"; game.settingsPath = "shots/tests/test_settings.txt"; game.guidePath = "shots/tests/unit_guide.txt";
            applyTestScale(game);
            Input in;
            auto press = [&](int key) { pressKey(game, in, key); };
            auto run = [&](double s) { runFrames(game, in, s); };
            press(KEY_ENTER); run(0.3);
            game.testParkAt(sys.star, bi); run(0.3);
            game.testSetTime(tBand); game.testLandCursor(PIN_PLAIN_LAT, PIN_PLAIN_LON); run(0.2);
            double bandG = game.testFrontBandAt(PIN_PLAIN_LAT, PIN_PLAIN_LON);
            press(KEY_ENTER); run(8.0);
            bool onSurface = game.state == GameState::SURFACE;
            run(0.5);
            std::string line = game.testForecastLine(), info = game.testFrontInfo();
            press(KEY_N); run(0.1); bool mapUp = game.state == GameState::SECTOR_MAP; press(KEY_ESCAPE);
            check("fronts: the data sheet forecasts the front, the sector map draws its line", onSurface && bandG > 0.5 && line.rfind("RAIN FROM THE ", 0) == 0 && line.find(frontCompass(fc.fromBearing)) != std::string::npos && info.find("line on") != std::string::npos && mapUp,
                  fmt("the map's band %.2f at the cursor; '%s'; %s", bandG, line.c_str(), info.c_str()));
            remove("shots/tests/unit_guide.txt");
        }
    }
    {   // a shower peaks when the world is at the point of its orbit nearest the comet's: the world there at the peak's
        // time, its distance to the comet's orbit the two orbits' least there and more a few widths either side; the almanac
        // gives the same peak
        int tested = 0, bad = 0; std::string why;
        for (auto& [x, z, st, sys] : scanSystems(150, 200, 20, 80, [&] { return tested < 12; })) {
            for (int bi = 0; bi < (int)sys.bodies.size() && tested < 12; bi++) {
                if (sys.bodies[bi].parent >= 0) continue;
                std::vector<MeteorShower> sh; meteorShowersOf(sys, bi, sh);
                for (const MeteorShower& sw : sh) {
                    tested++;
                    const Body& W = sys.bodies[sw.world];
                    double tp = showerPeakAfter(sw, 3.6e6);
                    Vec3 at = sys.bodyPos(sw.world, tp) - sys.star.pos;
                    double off = length(at - sw.worldKm) / W.orbitRadiusKm;
                    double d0 = orbitDistanceKm(sys.bodies[sw.comet], at);
                    double dm = orbitDistanceKm(sys.bodies[sw.comet], sys.bodyPos(sw.world, tp - 3 * sw.sigma) - sys.star.pos), dp = orbitDistanceKm(sys.bodies[sw.comet], sys.bodyPos(sw.world, tp + 3 * sw.sigma) - sys.star.pos);
                    AlmanacPlace pl; pl.sys = &sys; pl.t = tp - 3600; pl.window = 7200; pl.ground = true; pl.body = bi; pl.lat = 0.3; pl.lon = 1.1;
                    std::vector<AlmanacEvent> ev; almanacOf(pl, ev);
                    double dAl = 1e18; for (const AlmanacEvent& e : ev) if (e.kind == AL_SHOWER && e.body == sw.comet) dAl = std::min(dAl, std::fabs(e.t - tp));
                    bool ok = off < 2e-3 && std::fabs(d0 - sw.moidKm) < 0.01 * W.orbitRadiusKm && dm > d0 && dp > d0 && dAl < 1.0;
                    if (!ok) { bad++; why += fmt("%s/%s: off %.4f, d %.0f (moid %.0f), either side %.0f %.0f, almanac %.1f s; ", W.name.c_str(), sys.bodies[sw.comet].name.c_str(), off, d0, sw.moidKm, dm, dp, dAl); }
                }
            }
        }
        check("showers: the peak is the orbits' crossing (W-04)", tested >= 10 && bad == 0, fmt("%d showers, %d wrong %s", tested, bad, why.c_str()));
    }
    {   // the streaks of the shower scene: dozens over four minutes at the peak, none eight widths off it, each running away from
        // the radiant along a great circle through it
        StarSystem sys; SurfaceView sv; StarNeighborhood nb; double t = 0;
        bool have = setupNightScene(0, sv, sys, nb, t);
        int atPeak = 0, offPeak = 0, wrong = 0;
        if (have && !sv.showers.empty()) {
            const MeteorShower& sw = sv.showers[0];
            double tp = showerPeakAfter(sw, t - 0.5 * sw.period);
            for (double T = tp - 120; T <= tp + 120; T += 0.1) {
                Vec3 rad = sv.site.localFrame(T) * sw.radiant;   // the sky turns: the radiant's place now
                std::vector<SurfaceView::ShowerMeteor> ms; sv.showerMeteorsAt(T, ms);
                for (auto& m : ms) {
                    atPeak++;
                    Vec3 c = cross(m.tail, m.head);
                    if (dot(m.head, rad) > dot(m.tail, rad) + 1e-9 || (length(c) > 1e-9 && std::fabs(dot(c / length(c), rad)) > 1e-3)) wrong++;
                }
            }
            for (double T = tp + 8 * sw.sigma; T <= tp + 8 * sw.sigma + 240; T += 0.1) {
                std::vector<SurfaceView::ShowerMeteor> ms; sv.showerMeteorsAt(T, ms);
                offPeak += (int)ms.size();
            }
        }
        check("showers: streaks from the radiant (W-04)", have && atPeak >= 300 && offPeak == 0 && wrong == 0, fmt("%d streak-frames over the peak's four minutes at ten a second, %d eight widths after it, %d not running from the radiant", atPeak, offPeak, wrong));
    }
    {   // the zodiacal light: brighter on the plane than off it, falling away from the sun, the faint patch opposite it; dust from
        // a young star and a near belt, none in most systems
        Vec3 sun(1, 0, 0);
        auto at = [&](double elDeg, double latDeg) { double e = elDeg * DEG, b = latDeg * DEG; return zodiacalLight(Vec3(std::cos(e) * std::cos(b), std::sin(b), std::sin(e) * std::cos(b)), sun); };
        bool shape = at(30, 0) > 2 * at(30, 25) && at(30, 0) > at(60, 0) && at(60, 0) > at(90, 0) && at(180, 0) > 1.5 * at(150, 0) && at(180, 0) > 1.5 * at(180, 15);
        int sys0 = 0, dusty = 0, young = 0, youngDusty = 0;
        for (auto& [x, z, st, sys] : scanSystems(150, 200, 20, 80)) {
            if (sys.bodies.empty()) continue;
            sys0++;
            if (zodiacalDust(sys, 0) >= ZODIACAL_MIN_DUST) dusty++;
            if (sys.star.cls == STAR_PROTOSTAR || sys.star.cls == STAR_BLUE_GIANT) { young++; if (zodiacalDust(sys, 0) >= ZODIACAL_MIN_DUST) youngDusty++; }
        }
        check("zodiacal light: the cone and its dust (W-04)", shape && young > 0 && youngDusty == young && dusty < sys0 * 0.8 && dusty > sys0 * 0.05,
              fmt("on the plane at 30 deg %.3f, 25 deg off it %.3f, at 60 %.3f, 90 %.3f, 150 %.3f, opposite %.3f; %d of %d systems dusty, %d of %d young ones", at(30, 0), at(30, 25), at(60, 0), at(90, 0), at(150, 0), at(180, 0), dusty, sys0, youngDusty, young));
    }
    {   // the earthshine: on the earthshine scene's crescent the disc's night side is drawn, under the lit side's 43 and over the
        // sky's black
        StarSystem sys; SurfaceView sv; StarNeighborhood nb; double t = 0; SpaceRenderer sr;
        bool have = setupNightScene(2, sv, sys, nb, t);
        int night = 0, lit = 0, disc = 0;
        if (have) {
            Framebuffer fb; sv.render(fb, t, nb.stars, sr, 1.0);
            int moon = -1; double rMax = 0;
            for (int j = 0; j < (int)sr.bodyInfo.size(); j++) if (sr.bodyInfo[j].radiusPx > rMax && sr.bodyInfo[j].inFront) { rMax = sr.bodyInfo[j].radiusPx; moon = j; }
            if (moon >= 0) {
                const BodyScreenInfo& bi = sr.bodyInfo[moon];
                for (int y = 0; y < FBH; y++) for (int x = 0; x < FBW; x++) {
                    double dx = x + 0.5 - bi.sx, dy = y + 0.5 - bi.sy;
                    if (dx * dx + dy * dy > 0.8 * bi.radiusPx * bi.radiusPx || fb.invz[y * FBW + x] <= 0) continue;
                    disc++;
                    Pix p = fb.idx[y * FBW + x];
                    if (bankOf(p) != 1) continue;
                    if (shadeOf(p) >= 30 && shadeOf(p) < 43) night++; else if (shadeOf(p) >= 43) lit++;
                }
            }
        }
        check("earthshine: a crescent's night side (W-04)", have && disc > 0 && night > disc / 3 && lit > 0, fmt("%d pixels of the disc: %d in the earthshine's 30-42, %d lit", disc, night, lit));
    }
}
