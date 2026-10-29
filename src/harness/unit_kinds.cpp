// E-03: the unit's kinds of worlds (groups `kinds`, `dead`, `cold`, `moons`, `giants`, `traits`)
#include "harness/harness.h"
#include "harness/test_worlds.h"

// the kind table, the made kinds, the living worlds, the locked and the deep
void unitKinds(const Check& check) {
    // the kind table. The trait eligibility of 1.3.2's 33 traits over its 22 kinds is kept (the traits by tags: a kind or a trait
    // appended may only add), the old landform-or-world split is the traits' classes, every kind's names fit the screens and the files
    {
        static const uint32_t ELIGIBLE_132[33] = {0x000000, 0x2604bc, 0x241138, 0x200008, 0x2408bd, 0x3614be, 0x000028, 0x040138, 0x06083f, 0x114490, 0x0940a8,
                                                  0x0c0028, 0x0c0028, 0x240028, 0x060d3f, 0x1c14b3, 0x220805, 0x215180, 0x220888, 0x000208, 0x000008, 0x000008,
                                                  0x000208, 0x0010a0, 0x2c022c, 0x240328, 0x000008, 0x000008, 0x000008, 0x260038, 0x260038, 0x260038, 0x000000};
        bool ok = true; std::string why;
        for (int t = 0; t < 33 && t < TR_COUNT; t++)
            for (int k = 0; k < 22 && k < PT_COUNT; k++)
                if (traitEligible(k, t) != ((ELIGIBLE_132[t] >> k & 1u) != 0)) { ok = false; why = std::string(KINDS[k].name) + " / " + TRAITS[t].name; }
        check("kinds: 1.3.2's trait eligibility kept", ok, why);
        bool split = TRAITS[TR_GLASSED].cls == TC_GIVEN && TRAITS[TR_CIVILISATION].cls == TC_GIVEN;
        for (int t = 1; t < TR_GLASSED; t++) if (TRAITS[t].cls != (t <= TR_GEYSERS ? TC_LANDFORM : TC_WORLD)) split = false;
        check("kinds: the traits' classes are 1.3.2's split", split);
        bool names = true; std::string bad;
        for (int k = 0; k < PT_COUNT; k++) {
            const KindDef& K = KINDS[k];
            for (const char* c = K.name; *c; c++) if (!((*c >= 'A' && *c <= 'Z') || *c == ' ' || *c == '-')) { names = false; bad = K.name; }
            if (std::strlen(K.shortName) > 10 || (K.landable && K.ground == GR_NONE)) { names = false; bad = K.name; }
        }
        check("kinds: names safe, short names in ten characters, a ground for every landable kind", names, bad);
    }
    // the made kinds. Over the systems of the scan region nearest home, generated twice: every made body was drawn as one of its
    // kind's sources, no world with a people changed kind, the systems the same twice; about one body in fifty made; the home world felisian
    {
        int systems = 0, bodiesN = 0, made = 0, people = 0, bad = 0, again = 0, perKind[PT_COUNT] = {0};
        std::string why;
        for (int64_t x = 150; x < 260 && systems < 800; x++)
            for (int64_t z = 20; z < 120 && systems < 800; z++) {
                Star st; if (!starInSector(x, 0, z, st)) continue;
                StarSystem a, a2; a.generate(st); a2.generate(st); systems++;
                if (a.bodies.size() != a2.bodies.size()) { again++; continue; }
                for (size_t i = 0; i < a.bodies.size(); i++) {
                    const Body& B = a.bodies[i]; bodiesN++;
                    if (B.type != a2.bodies[i].type || B.radiusKm != a2.bodies[i].radiusKm || B.orbitRadiusKm != a2.bodies[i].orbitRadiusKm || B.color.r != a2.bodies[i].color.r) again++;
                    const int d = drawnKind(B);
                    if (kindHas(d, KT_PEOPLE) && peopleHash(B.seed)) { people++; if (B.type != d) { bad++; why = B.name + " had a people and was made a " + KINDS[B.type].name; } }
                    if (B.type == d) continue;
                    made++; perKind[B.type]++;
                    const KindDef& K = KINDS[B.type];
                    bool from = false; for (int k = 0; k < K.nSources; k++) if (K.sources[k].from >> d & 1u) from = true;
                    if (B.type < PT_DRAWN_COUNT || d >= PT_DRAWN_COUNT || !from) { bad++; why = B.name + " made a " + K.name + " from " + KINDS[d].name; }
                }
            }
        std::string counts;
        for (int k = PT_DRAWN_COUNT; k < PT_COUNT; k++) counts += fmt("%s%s %d", k > PT_DRAWN_COUNT ? ", " : "", KINDS[k].name, perKind[k]);
        check("made kinds: from their sources' draws, never a people's world, the same twice (P-01)", bad == 0 && again == 0 && people > 50,
                fmt("%d systems, %d bodies, %d made (%s); %d people's worlds kept%s%s", systems, bodiesN, made, counts.c_str(), people, again ? fmt(", %d NOT THE SAME TWICE", again).c_str() : "",
                        why.empty() ? "" : (": " + why).c_str()));
        const double share = (double)made / std::max(1, bodiesN);
        check("made kinds: their share of the bodies (P-01's 1.4%, with P-02's living worlds 4.5%, with P-03's locked and deep 6.9%, with P-04's dead worlds 7.2%, with P-05's far cold 9.7%, with "
              "P-06's strange moons 11.3%, with P-07's giants 12.7%)",
                share > 0.07 && share < 0.16, fmt("%.1f%%", 100 * share));
        Star hs; bool home = false; std::string hw = "no home star";
        if (starInSector(HOME_SX, HOME_SY, HOME_SZ, hs)) {
            StarSystem h; h.generate(hs);
            // the new game's own test (`Game::newGame`); P-02: the same world (the salts pinned)
            for (const Body& b : h.bodies) if (b.parent < 0 && b.type == PT_FELISIAN) { home = b.name == "Heilya II"; hw = b.name; break; }
        }
        check("made kinds: the home world felisian, Heilya II (P-01 to P-04)", home, hw);
    }
    // the living worlds. Made only from felisian worlds (the lichen world from thin airs) whose day is not their year, round a star
    // still burning its hydrogen; the archean by its star's hash, so every felisian world of a young star is archean unless a people's;
    // and the first world of each kind in the scan region, sampled on its map's grid, has its ground: the archean no plant and its mats,
    // the hothouse no ice on its low polar land, the tundra its sheets, the lichen world its crusts and moss, the steppe its grass, the
    // superhabitable world its shallow seas
    {
        int made = 0, bad = 0, young = 0; std::string why;
        const uint32_t mainSeq = (1u << STAR_YELLOW) | (1u << STAR_ORANGE) | (1u << STAR_RED_DWARF) | (1u << STAR_BLUE_WHITE) | (1u << STAR_BLUE_GIANT);
        int firstOf[PT_COUNT]; int64_t fx[PT_COUNT], fz[PT_COUNT]; for (int k = 0; k < PT_COUNT; k++) firstOf[k] = -1;
        for (auto& [x, z, st, sys] : scanSystems(150, 260, 20, 120)) {
            bool archean = false;
            for (const Body& B : sys.bodies) if (B.type == PT_ARCHEAN) archean = true;
            for (const Body& B : sys.bodies) {
                if (B.type < PT_ARCHEAN || B.type > PT_LICHEN) {
                    // a young star's felisian worlds are all archean: one left felisian beside an archean world is a people's, or locked, or a giant's star
                    const bool companionWorld = B.parent >= 0 && sys.bodies[B.parent].type == PT_COMPANION;   // judged by its own sun's class
                    if (archean && B.type == PT_FELISIAN && !companionWorld && !peopleHash(B.seed) && !B.locked && !(B.parent < 0 && B.rotPeriod == B.orbitPeriod)) {
                        bad++;
                        why = B.name + " left felisian beside its star's archean worlds";
                    }
                    continue;
                }
                made++;
                if (firstOf[B.type] < 0) { firstOf[B.type] = B.index; fx[B.type] = x; fz[B.type] = z; }
                if (B.type == PT_ARCHEAN) young++;
                const int d = drawnKind(B);
                const bool planetOfStar = B.parent < 0 || sys.bodies[B.parent].type == PT_COMPANION;
                const int cls = B.parent >= 0 && sys.bodies[B.parent].type == PT_COMPANION ? sys.bodies[B.parent].starClass : st.cls;
                if ((B.type == PT_LICHEN ? d != PT_THINATMO : d != PT_FELISIAN) || !(mainSeq >> cls & 1u)) { bad++; why = B.name + " made a " + KINDS[B.type].name + " from " + KINDS[d].name; }
                if (B.type != PT_LICHEN && planetOfStar && B.rotPeriod == B.orbitPeriod) { bad++; why = B.name + " is locked and made a " + KINDS[B.type].name; }
            }
        }
        check("living kinds: from felisian (or thin air) worlds whose day is not their year, round living stars; a young star's alike (P-02)", bad == 0 && made > 400 && young > 50,
              fmt("%d made, %d archean%s", made, young, why.empty() ? "" : (": " + why).c_str()));
        std::string out; bool ok = true;
        DrainageOff dOff;
        for (int k = PT_ARCHEAN; k <= PT_LICHEN; k++) {
            if (firstOf[k] < 0) { ok = false; out += fmt("%s none; ", KINDS[k].name); continue; }
            Star st; starInSector(fx[k], 0, fz[k], st); StarSystem sys; sys.generate(st);
            const BodyGen g = BodyGen::make(sys.bodies[firstOf[k]]);
            double land = 0, sea = 0, shallow = 0, n[MAT_COUNT] = {0}, polarIce = 0, sheet = 0, sheetLand = 0;
            for (int j = 0; j < 64; j++)
                for (int i = 0; i < 128; i++) {
                    const double la = (0.5 - (j + 0.5) / 64) * PI, lo = (i + 0.5) / 128 * TAU - PI, w = std::cos(la);
                    const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 20000);
                    if (ss.material == MAT_WATER) { sea += w; if (ss.height > -300) shallow += w; continue; }
                    land += w; n[ss.material] += w;
                    if (std::fabs(la) > 62 * DEG && ss.height < 1500 && (ss.material == MAT_SNOW || ss.material == MAT_ICE)) polarIce += w;
                    if (k == PT_TUNDRA && la > 0 && la / DEG > iceSheetEdgeDeg(g, StarSystem::bodyFromLatLon(la, lo)) + 3) {
                        sheetLand += w;
                        if (ss.material == MAT_SNOW || ss.material == MAT_ICE) sheet += w;
                    }
                }
            land = std::max(land, 1e-9);
            bool pass = true; std::string what;
            switch (k) {
                case PT_ARCHEAN:
                    pass = n[MAT_FOREST] + n[MAT_GRASS] == 0 && n[MAT_MATS] > 0.05 * land;
                    what = fmt("plants %.0f%%, mats %.0f%%", 100 * (n[MAT_FOREST] + n[MAT_GRASS]) / land, 100 * n[MAT_MATS] / land);
                    break;
                case PT_HOTHOUSE: pass = polarIce == 0 && n[MAT_FOREST] > 0.3 * land; what = fmt("low polar ice %.0f%%, forest %.0f%%", 100 * polarIce / land, 100 * n[MAT_FOREST] / land); break;
                case PT_SUPERHABITABLE:
                    pass = shallow > 0.5 * sea && land < 0.45 * (land + sea);
                    what = fmt("seas shallower than 300 m %.0f%%, land %.0f%%", 100 * shallow / std::max(1e-9, sea), 100 * land / (land + sea));
                    break;
                case PT_STEPPE: pass = n[MAT_GRASS] > 0.4 * land; what = fmt("grass %.0f%%", 100 * n[MAT_GRASS] / land); break;
                case PT_TUNDRA: pass = sheetLand == 0 || sheet > 0.9 * sheetLand; what = fmt("land under its sheets %.0f%% ice", sheetLand > 0 ? 100 * sheet / sheetLand : 100.0); break;
                default: pass = n[MAT_CRUST] + n[MAT_MOSS] > 0.1 * land; what = fmt("crust %.0f%%, moss %.0f%%", 100 * n[MAT_CRUST] / land, 100 * n[MAT_MOSS] / land); break;
            }
            if (!pass) ok = false;
            out += fmt("%s %s: %s%s; ", KINDS[k].name, sys.bodies[firstOf[k]].name.c_str(), what.c_str(), pass ? "" : " FAILS");
        }
        check("living grounds: each kind's signature on its first world (P-02)", ok, out);
    }
    // the locked and the deep. Each made only from its sources: the eyeball from a red dwarf's locked ocean, felisian or icy planet, the
    // terminator from a locked felisian or desert planet, both untilted and neither half of a double planet (a twin takes the lock: `generate`
    // makes it again); the hycean from an ocean, the ammonia world from an ocean, a methane world or an icy one, the snowball from a felisian or
    // an ocean world whose day is not its year, the chemotroph from a tectonic or rocky world of a dead star or a brown dwarf's moon; the test
    // worlds as drawn (the regress's Feeyeithis II-IV and Wyariothmai I-III, the bench's forest on Feeyeithis IV: the salts pinned); and the
    // first world of each kind in the scan region, sampled on its map's grid, has its ground
    {
        int made = 0, bad = 0, perKind[PT_COUNT] = {0}; std::string why;
        const uint32_t dead = (1u << STAR_WHITE_DWARF) | (1u << STAR_PULSAR) | (1u << STAR_NEUTRON);
        int firstOf[PT_COUNT]; int64_t fx[PT_COUNT], fz[PT_COUNT]; for (int k = 0; k < PT_COUNT; k++) firstOf[k] = -1;
        for (auto& [x, z, st, sys] : scanSystems(150, 260, 20, 120)) {
            for (const Body& B : sys.bodies) {
                if (B.type < PT_EYEBALL || B.type > PT_CHEMOTROPH) continue;
                made++; perKind[B.type]++;
                if (firstOf[B.type] < 0) { firstOf[B.type] = B.index; fx[B.type] = x; fz[B.type] = z; }
                const int d = drawnKind(B), pd = B.parent >= 0 ? drawnKind(sys.bodies[B.parent]) : -1;
                const int cls = B.parent >= 0 && sys.bodies[B.parent].type == PT_COMPANION ? sys.bodies[B.parent].starClass : st.cls;   // a companion's world by its own sun (P-01's facts)
                bool ok = true;
                switch (B.type) {
                    case PT_EYEBALL: ok = (d == PT_OCEAN || d == PT_FELISIAN || d == PT_ICY) && cls == STAR_RED_DWARF && B.parent < 0 && B.locked && B.axialTilt == 0 && !B.doublePlanet; break;
                    case PT_TERMINATOR: ok = (d == PT_FELISIAN || d == PT_DESERT) && B.parent < 0 && B.locked && B.axialTilt == 0 && !B.doublePlanet; break;
                    case PT_HYCEAN: ok = d == PT_OCEAN && (B.parent < 0 || sys.bodies[B.parent].type == PT_COMPANION); break;
                    case PT_AMMONIA: ok = d == PT_OCEAN || d == PT_HYDROCARBON || d == PT_ICY; break;
                    case PT_SNOWBALL: ok = (d == PT_FELISIAN || d == PT_OCEAN) && !B.locked; break;
                    default: ok = (d == PT_TECTONIC || d == PT_ROCKY) && ((dead >> cls & 1u) || pd == PT_SUBSTELLAR); break;
                }
                if (!ok) { bad++; why = B.name + " made a " + KINDS[B.type].name + " from " + KINDS[d].name; }
            }
        }
        std::string counts;
        for (int k = PT_EYEBALL; k <= PT_CHEMOTROPH; k++) counts += fmt("%s%s %d", k > PT_EYEBALL ? ", " : "", KINDS[k].name, perKind[k]);
        check("locked and deep kinds: from their sources, the locked ones locked and untilted, none half of a double planet (P-03)", bad == 0 && made > 400,
              fmt("%d made (%s)%s", made, counts.c_str(), why.empty() ? "" : (": " + why).c_str()));
        struct Pin { int64_t sx, sz; const char* name; int type; };
        static const Pin PINS[] = {{150, 58, "Feeyeithis II", PT_FELISIAN}, {150, 58, "Feeyeithis III", PT_FELISIAN}, {150, 58, "Feeyeithis IV", PT_FELISIAN},
                                   {150, 36, "Wyariothmai I", PT_FELISIAN}, {150, 36, "Wyariothmai II", PT_OCEAN}, {150, 36, "Wyariothmai III", PT_OCEAN}};
        std::string pins; bool pinsOk = true;
        for (const Pin& P : PINS) {
            Star st; bool seen = false;
            if (starInSector(P.sx, 0, P.sz, st)) {
                StarSystem sys;
                sys.generate(st);
                for (const Body& B : sys.bodies)
                    if (B.name == P.name) {
                        seen = true;
                        if (B.type != P.type) {
                            pinsOk = false;
                            pins += fmt("%s %s; ", P.name, KINDS[B.type].name);
                        }
                    }
            }
            if (!seen) { pinsOk = false; pins += fmt("%s not found; ", P.name); }
        }
        check("locked and deep kinds: the regress's and the bench's test worlds as drawn (P-03)", pinsOk, pins.empty() ? "Feeyeithis II-IV felisian, Wyariothmai I felisian, II-III oceans" : pins);
        std::string out; bool ok = true;
        DrainageOff dOff;
        for (int k = PT_EYEBALL; k <= PT_CHEMOTROPH; k++) {
            if (firstOf[k] < 0) { ok = false; out += fmt("%s none; ", KINDS[k].name); continue; }
            Star st; starInSector(fx[k], 0, fz[k], st); StarSystem sys; sys.generate(st);
            const BodyGen g = BodyGen::make(sys.bodies[firstOf[k]]);
            const Vec3 sub = g.locked ? g.lockedDir : Vec3(1, 0, 0);
            double land = 0, sea = 0, n[MAT_COUNT] = {0}, pupil = 0, pupilWet = 0, outer = 0, outerIce = 0, day = 0, dayDry = 0, night = 0, nightIce = 0, ring = 0, ringGreen = 0, ice = 0, farWet = 0,
                   level = 0;
            for (int j = 0; j < 64; j++)
                for (int i = 0; i < 128; i++) {
                    const double la = (0.5 - (j + 0.5) / 64) * PI, lo = (i + 0.5) / 128 * TAU - PI, w = std::cos(la);
                    const Vec3 u = StarSystem::bodyFromLatLon(la, lo);
                    const SurfaceSample ss = sampleSurface(g, u, 20000);
                    const double ang = std::acos(clampd(dot(u, sub), -1, 1)) / DEG, sl = dot(u, sub);
                    const bool wet = ss.material == MAT_WATER;
                    if (ss.material == MAT_ICE || ss.material == MAT_SNOW) ice += w;
                    if (ang < g.seaReachDeg - 6) { pupil += w; if (wet) pupilWet += w; }
                    if (ang > g.seaReachDeg + 6) { outer += w; if (!wet) outerIce += w; }
                    if (wet) { sea += w; level = std::max(level, std::fabs(ss.water)); if (std::fabs(la) > 25 * DEG) farWet += w; continue; }
                    land += w; n[ss.material] += w;
                    if (sl > 0.6) { day += w; if (ss.material != MAT_GRASS && ss.material != MAT_FOREST && ss.material != MAT_SNOW && ss.material != MAT_ICE) dayDry += w; }
                    if (sl < -0.6) { night += w; if (ss.material == MAT_SNOW || ss.material == MAT_ICE) nightIce += w; }
                    if (std::fabs(sl) < 0.35) { ring += w; if (ss.material == MAT_GRASS || ss.material == MAT_FOREST) ringGreen += w; }
                }
            land = std::max(land, 1e-9);
            const double all = land + sea;
            bool pass = true; std::string what;
            switch (k) {
                case PT_EYEBALL:
                    pass = pupilWet > 0.8 * pupil && outerIce > 0.97 * outer;
                    what = fmt("the pupil %.0f%% open water, beyond it %.0f%% ice", 100 * pupilWet / std::max(1e-9, pupil), 100 * outerIce / std::max(1e-9, outer));
                    break;
                case PT_TERMINATOR: pass = dayDry > 0.9 * day && nightIce > 0.9 * night && ringGreen > 0.1 * ring;
                    what = fmt("its day %.0f%% dry, its night %.0f%% ice, its ring %.0f%% green", 100 * dayDry / std::max(1e-9, day), 100 * nightIce / std::max(1e-9, night),
                            100 * ringGreen / std::max(1e-9, ring));
                    break;
                case PT_HYCEAN: pass = land < 1e-6 && ice == 0; what = fmt("land %.1f%%, ice %.1f%%", 100 * (land > 1e-6 ? land : 0) / all, 100 * ice / all); break;
                case PT_AMMONIA:
                    pass = sea > 0.1 * all && level < 0.01 && n[MAT_SNOW] > 0;
                    what = fmt("sea %.0f%% at the reference level (%.2f m off), frost %.1f%% of the land", 100 * sea / all, level, 100 * n[MAT_SNOW] / land);
                    break;
                case PT_SNOWBALL: pass = ice > 0.85 * all && farWet == 0; what = fmt("ice %.0f%% of the world, open water past 25 degrees %.1f%%", 100 * ice / all, 100 * farWet / all); break;
                default: {   // the chemotroph: its mats and sinter, and a pool on its own ground near the first of them
                    bool pool = false;
                    for (int j = 0; j < 64 && !pool; j++)
                        for (int i = 0; i < 128 && !pool; i++) {
                            const Vec3 u = StarSystem::bodyFromLatLon((0.5 - (j + 0.5) / 64) * PI, (i + 0.5) / 128 * TAU - PI);
                            const int m512 = sampleSurface(g, u, 512).material;
                            if (m512 != MAT_MATS && m512 != MAT_SALT) continue;
                            std::vector<GeyserVent> vents; springVents(g, u, vents);
                            for (const GeyserVent& v : vents) if (sampleSurface(g, v.unit, 16).material == MAT_BRINE) pool = true;
                        }
                    pass = n[MAT_MATS] + n[MAT_SALT] > 0.01 * land && pool;
                    what = fmt("mats %.1f%%, sinter %.1f%%, %s", 100 * n[MAT_MATS] / land, 100 * n[MAT_SALT] / land, pool ? "a spring's pool" : "NO POOL"); break;
                }
            }
            if (!pass) ok = false;
            out += fmt("%s %s: %s%s; ", KINDS[k].name, sys.bodies[firstOf[k]].name.c_str(), what.c_str(), pass ? "" : " FAILS");
        }
        check("locked and deep grounds: each kind's signature on its first world (P-03)", ok, out);
    }
}

// P-04: the dead worlds, their two times, their peoples
void unitDead(const Check& check) {
    // the dead worlds. The scorched made only from its sources (a giant's felisian worlds, a white dwarf's rocky and desert planets at
    // 180-260 K), never a people's world of C-01, its then-world's climate the drawn felisian's (else a living one's); the frozen made in no
    // system but as a wanderer (P-05; D's drifters), with no people; about one scorched world in eight with a people of its own
    {
        int made = 0, bad = 0, frozen = 0, frozenWandering = 0, withPeople = 0; std::string why;
        for (auto& [x, z, st, sys] : scanSystems(150, 340, 20, 160)) {
            for (const Body& B : sys.bodies) {
                if (B.type == PT_FROZEN_FELISIAN) { frozen++; if (B.wanderer && !worldHadCivilisation(BodyGen::make(B))) frozenWandering++; }
                if (B.type != PT_SCORCHED_FELISIAN) continue;
                made++;
                const int d = drawnKind(B);
                const int cls = B.parent >= 0 && sys.bodies[B.parent].type == PT_COMPANION ? sys.bodies[B.parent].starClass : st.cls;
                const bool giant = cls == STAR_RED_GIANT || cls == STAR_ORANGE_GIANT || cls == STAR_CARBON;
                bool ok = d == PT_FELISIAN ? giant && !peopleHash(B.seed) && B.thenTempK == B.tempK
                                           : (d == PT_ROCKY || d == PT_DESERT) && cls == STAR_WHITE_DWARF && B.tempK >= 180 && B.tempK < 260 &&
                                !(B.parent >= 0 && !B.doublePlanet && sys.bodies[B.parent].type != PT_COMPANION) && B.thenTempK >= 265 && B.thenTempK <= 300 && !(d == PT_DESERT && peopleHash(B.seed));
                if (!ok) { bad++; why = B.name + " made a SCORCHED FELISIAN from " + KINDS[d].name; }
                if (worldHadCivilisation(BodyGen::make(B))) withPeople++;
            }
        }
        const double share = (double)withPeople / std::max(1, made);
        check("dead kinds: the scorched from a giant's felisian worlds and a white dwarf's rocky and desert planets, never C-01's people's; the frozen only as a wanderer, with no people (P-04, P-05)",
                bad == 0 && frozen == frozenWandering && made > 60 && share > 0.06 && share < 0.2,
                fmt("%d scorched, %d of them with a people (%.0f%%), %d frozen (%d wanderers without a people)%s", made, withPeople, 100 * share, frozen, frozenWandering,
                        why.empty() ? "" : (": " + why).c_str()));
    }
    // a dead world's two times. On each scorched test world (`SCORCHED_WORLDS`: the nearest scorched people of each age) and on a frozen world
    // the harness stages with a people (its own felisian world without C-01's people, the first of the scan's with the frozen kind's people): at
    // every place of its peoples' plans on dry ground then (where a town stands: the plan's grid is too coarse for the drainage's lakes) the now's
    // ground stands at the then-world's height (without the drainage, and with it at three), no living ground is left under the cover, the plan
    // reads the then-world's land; and on the frozen world 200 points of the then-world's sea are water to the placement then (`wetSample`) and
    // the old sea now (`oldSea`: its ice at the sea's level)
    {
        struct W { StarSystem sys; int bi = -1; std::string name; };
        std::vector<W> worlds;
        for (const AgeWorld& a : SCORCHED_WORLDS) {
            if (a.body < 0) continue;
            Star st; if (!starInSector(a.sx, 0, a.sz, st, true)) continue;
            W w; w.sys.generate(st); w.bi = a.body; if (w.bi < (int)w.sys.bodies.size()) { w.name = w.sys.bodies[w.bi].name; worlds.push_back(w); }
        }
        int staged = -1;
        for (int64_t x = 150; x < 340 && staged < 0; x++)
            for (int64_t z = 20; z < 160 && staged < 0; z++) {
                Star st; if (!starInSector(x, 0, z, st)) continue;
                W w; w.sys.generate(st);
                for (int bi = 0; bi < (int)w.sys.bodies.size() && staged < 0; bi++) {
                    if (!peoplesHash(PT_FROZEN_FELISIAN, w.sys.bodies[bi].seed) || !stageElsewhere(w.sys, bi, PT_FROZEN_FELISIAN)) continue;
                    w.bi = bi; w.name = w.sys.bodies[bi].name + " (staged frozen)"; worlds.push_back(w); staged = (int)worlds.size() - 1;
                }
            }
        int places = 0, off = 0, offDrained = 0, drained = 0, covered = 0, notPeople = 0, planLand = 0, planLandOk = 0, wetThen = 0; double worst = 0; std::string out;
        auto wetS = [](const SurfaceSample& s) { return s.material == MAT_WATER || (s.water > -1e8 && s.height < s.water) || s.oldSea > 0.5; };   // the placement's test (`wetSample`)
        for (const W& w : worlds) {
            const Body& b = w.sys.bodies[w.bi];
            const BodyGen gNow = BodyGen::make(b);
            BodyGen tmp; const BodyGen& gThen = thenWorld(gNow, tmp);
            if (!deadKind(gNow.type) || gThen.type != PT_FELISIAN || !worldHadCivilisation(gNow)) { notPeople++; continue; }
            std::shared_ptr<const WorldPlan> P = worldPlanOf(gNow);
            int n = 0, nd = 0;
            for (int p = 0; p < P->peoples; p++)
                for (const PlanPlace& pl : P->plans[p].places) {
                    const Vec3 u = StarSystem::bodyFromLatLon(pl.lat, pl.lon);
                    SurfaceSample sn, st;
                    { DrainageOff dOff; sn = sampleSurface(gNow, u, 16); st = sampleSurface(gThen, u, 16); }
                    if (sn.material == MAT_GRASS || sn.material == MAT_FOREST || sn.material == MAT_WATER || sn.water > -1e8) covered++;   // a living ground left under the dead one's cover
                    if (wetS(st)) { wetThen++; continue; }
                    places++; n++;
                    const double d = std::fabs(sn.height - st.height); worst = std::max(worst, d);
                    if (d > 1e-9) off++;
                    if (nd < 3) {
                        const SurfaceSample dn = sampleSurface(gNow, u, 16), dt = sampleSurface(gThen, u, 16);
                        if (!wetS(dt)) { nd++; drained++; if (std::fabs(dn.height - dt.height) > 1e-9) offDrained++; }
                    }
                    const int c = P->grid.cellOf(u); planLand++; if (P->grid.land[c]) planLandOk++;
                }
            out += fmt("%s %d places; ", w.name.c_str(), n);
        }
        int seaPoints = 0, seaWet = 0;
        if (staged >= 0) {
            const BodyGen gNow = BodyGen::make(worlds[staged].sys.bodies[worlds[staged].bi]);
            BodyGen tmp; const BodyGen& gThen = thenWorld(gNow, tmp);
            DrainageOff dOff;
            Rng r(0xF20E5EAULL);
            for (int k = 0; k < 4000 && seaPoints < 200; k++) {
                const double z0 = r.sym(0.95), a = r.range(0, TAU), rr = std::sqrt(1 - z0 * z0);
                const Vec3 u(rr * std::cos(a), rr * std::sin(a), z0);
                const SurfaceSample st = sampleSurface(gThen, u, 64);
                if (st.material != MAT_WATER || st.biome != BIO_OCEAN) continue;   // its sea
                seaPoints++;
                const SurfaceSample sn = sampleSurface(gNow, u, 64);
                const bool wetThen = st.material == MAT_WATER || (st.water > -1e8 && st.height < st.water) || st.oldSea > 0.5;   // the placement's test (`wetSample`)
                if (wetThen && sn.oldSea > 0.5 && (sn.material == MAT_ICE || sn.material == MAT_SNOW) && sn.height > -0.5 && sn.height < 4) seaWet++;
            }
        }
        check("dead kinds: the ground the same in both times at every place of a scorched test world's and a staged frozen world's peoples; a frozen sea water to the placement (P-04)",
                worlds.size() >= 7 && staged >= 0 && notPeople == 0 && places > 200 && off == 0 && offDrained == 0 && covered == 0 && planLandOk == planLand && seaPoints == 200 && seaWet == seaPoints,
                fmt("%zu worlds (%s); %d places on dry ground then (%d in its water): %d off (worst %.3g m), %d of %d drained off, %d under a living ground, %d of %d on the plan's land; the frozen "
                    "sea: %d of %d points water then and the old sea now",
                        worlds.size(), out.c_str(), places, wetThen, off, worst, offDrained, drained, covered, planLandOk, planLand, seaWet, seaPoints));
    }
    // the dead worlds' peoples: each scorched test world's people of its age; its culture built as its time's (a living world's) and weathered as the
    // present's (dry: the desert's wet and burial), its end by the desert's world factors; a system with a people of C-01's speaks for it on the air
    // and in the charts (no 1.3.2 signal moved: the system's people's world is C-01's first)
    {
        int held = 0, wrong = 0, weathered = 0, cultures = 0, systems = 0, sysBad = 0; std::string why;
        for (int a = 0; a < AGE_COUNT; a++) {
            const AgeWorld& W = SCORCHED_WORLDS[a];
            if (W.body < 0) continue;
            Star st; StarSystem sys; if (!systemAt(W.sx, 0, W.sz, st, sys, true)) { wrong++; continue; }
            if (W.body >= (int)sys.bodies.size() || sys.bodies[W.body].type != PT_SCORCHED_FELISIAN) { wrong++; why = fmt("the %s world is no scorched world", AGE_NAMES[a]); continue; }
            const BodyGen g = BodyGen::make(sys.bodies[W.body]);
            if (peoplesOf(g) > W.which && peopleAgeOf(g, W.which).age == a) held++; else { wrong++; why = fmt("the %s world's people of another age", AGE_NAMES[a]); }
            const Culture c = cultureOf(g, W.which); cultures++;
            if (c.wet <= 0.2 && c.paved == (unitFromHash(mix64(peopleGen(g, W.which).seed ^ 0xC09AULL)) < 0.7)) weathered++;
            if (!readWorld(sys, sys.bodies[W.body], g).desert) { wrong++; why = fmt("the %s world's end not the desert's", AGE_NAMES[a]); }
        }
        for (auto& [x, z, st, sys] : scanSystems(150, 340, 20, 160)) {
            int c01 = -1, dead = -1;
            for (const Body& B : sys.bodies) {
                if (B.wanderer || !kindHas(B.type, KT_PEOPLE) || !peoplesHash(B.type, B.seed)) continue;   // a wanderer keeps no people
                if (!deadKind(B.type) && c01 < 0) c01 = B.index;
                if (deadKind(B.type) && dead < 0) dead = B.index;
            }
            if (dead < 0) continue;
            systems++;
            if (systemPeoplesWorld(sys) != (c01 >= 0 ? c01 : dead)) { sysBad++; why = sys.star.name + "'s people's world not C-01's first"; }
        }
        check("dead kinds: the scorched test worlds' peoples of their ages, built as their time's and weathered as the present's, ended as a desert's; C-01's first on the air (P-04)",
              held == 6 && wrong == 0 && weathered == cultures && systems > 0 && sysBad == 0,
              fmt("%d of 6 test worlds hold their ages, %d of %d cultures weathered dry and built as a living world's; %d systems with a scorched people, %d speaking for the wrong world%s",
                  held, weathered, cultures, systems, sysBad, why.empty() ? "" : (": " + why).c_str()));
    }
}

// P-05: the far cold and its grounds
void unitCold(const Check& check) {
    // the far cold. Over the scan region: each far cold kind made only from its sources (the drawn kind, the temperature band, the body's
    // class: a giant's captured moon only the cryovolcanic, no air kind round a black hole or in a Wolf-Rayet wind); the wanderers a few systems
    // in a hundred, each its system's last body, far past the last planet on an eccentric, steep or retrograde orbit, no moon, no people, about
    // one in five a frozen felisian (never round a black hole or in a Wolf-Rayet wind); the regress's and home's cold worlds as drawn
    int coldFirst[PT_COUNT]; int64_t coldX[PT_COUNT], coldZ[PT_COUNT];
    for (int k = 0; k < PT_COUNT; k++) { coldFirst[k] = -1; coldX[k] = coldZ[k] = 0; }
    {
        int made = 0, bad = 0, perKind[PT_COUNT] = {0}, systems = 0, wand = 0, wandFrozen = 0, wandBad = 0, captured = 0, capturedMade = 0;
        std::string why, wwhy;
        for (auto& [x, z, st, sys] : scanSystems(150, 340, 20, 160)) {
systems++;
            double lastOrbit = 0;
            for (const Body& B : sys.bodies) if (B.parent < 0 && !B.wanderer && B.type != PT_COMET && B.type != PT_COMPANION) lastOrbit = std::max(lastOrbit, B.orbitRadiusKm);
            for (const Body& B : sys.bodies) {
                if (coldFirst[B.type] < 0 && B.type >= PT_NITROGEN && B.type <= PT_FROST_DWARF) { coldFirst[B.type] = B.index; coldX[B.type] = x; coldZ[B.type] = z; }
                if (B.wanderer) {
                    wand++;
                    const bool stripped = st.cls == STAR_BLACK_HOLE || st.cls == STAR_WOLF_RAYET;
                    if (B.type == PT_FROZEN_FELISIAN) wandFrozen++;
                    const bool ok = B.index == (int)sys.bodies.size() - 1 && B.parent < 0 && B.moonCount == 0 && B.ecc >= 0.4 && B.ecc <= 0.8 && B.orbitIncl >= 25 * DEG && B.orbitIncl <= 155 * DEG &&
                                    (lastOrbit <= 0 || B.orbitRadiusKm * (1 - B.ecc) >= 1.15 * lastOrbit * (1 - 1e-9)) && (B.type == PT_WANDERER || (B.type == PT_FROZEN_FELISIAN && !stripped)) &&
                                    !worldHadCivilisation(BodyGen::make(B));
                    if (!ok) { wandBad++; wwhy = B.name; }
                    continue;
                }
                const bool moon = B.parent >= 0 && sys.bodies[B.parent].type != PT_COMPANION && !B.doublePlanet;
                const bool giantMoon = moon && kindHas(drawnKind(sys.bodies[B.parent]), KT_GIANT), cap = moon && B.orbitPeriod < 0;
                if (cap) { captured++; if (B.type != drawnKind(B)) { capturedMade++; if (B.type != PT_CRYOVOLCANIC) { bad++; why = B.name + " (a captured moon) made a " + KINDS[B.type].name; } } }
                if (B.type < PT_NITROGEN || B.type > PT_FROST_DWARF) continue;   // the strange moons have checks of their own
                made++; perKind[B.type]++;
                const int d = drawnKind(B);
                const int cls = B.parent >= 0 && sys.bodies[B.parent].type == PT_COMPANION ? sys.bodies[B.parent].starClass : st.cls;
                const bool air = cls != STAR_BLACK_HOLE && cls != STAR_WOLF_RAYET, planetLike = !moon;
                const double T = B.tempK;
                bool ok = false;
                switch (B.type) {
                    case PT_NITROGEN: ok = d == PT_ICY && T < 60 && air && !giantMoon; break;
                    case PT_CRYOVOLCANIC: ok = air && giantMoon && (cap ? d == PT_ICY && T < 120 : (d == PT_ICY || d == PT_EUROPAN) && T < 80); break;
                    case PT_DRY_ICE: ok = air && T >= 140 && T < 200 && !cap && (d == PT_THINATMO || (d == PT_ICY && planetLike && !B.doublePlanet)); break;
                    case PT_BRINE_DWARF: ok = (d == PT_CRATERED || d == PT_ICY) && T >= 120 && T < 260 && planetLike && !B.doublePlanet; break;
                    case PT_FROST_DWARF: ok = (d == PT_ICY || d == PT_CRATERED) && T < 45 && !giantMoon; break;
                    default: ok = false; break;
                }
                (void)planetLike;
                if (!ok) { bad++; why = B.name + " made a " + KINDS[B.type].name + " from " + KINDS[d].name; }
            }
        }
        std::string counts;
        for (int k = PT_NITROGEN; k <= PT_FROST_DWARF; k++) if (k != PT_WANDERER) counts += fmt("%s%s %d", k > PT_NITROGEN ? ", " : "", KINDS[k].name, perKind[k]);
        check("far cold kinds: from their sources, a captured moon only the cryovolcanic, no air round a black hole or in a Wolf-Rayet wind (P-05)", bad == 0 && made > 600 && capturedMade > 0,
              fmt("%d made (%s); %d captured moons, %d made%s", made, counts.c_str(), captured, capturedMade, why.empty() ? "" : (": " + why).c_str()));
        const double share = (double)wand / std::max(1, systems);
        check("far cold kinds: wanderers in a few systems in a hundred, each its system's last body, far out on an eccentric, steep or retrograde orbit, no moon, no people, one in five a frozen "
              "felisian (P-05)",
                wandBad == 0 && share > 0.025 && share < 0.05 && wandFrozen > 0.1 * wand && wandFrozen < 0.3 * wand,
                fmt("%d wanderers in %d systems (%.1f%%), %d frozen felisian%s", wand, systems, 100 * share, wandFrozen, wwhy.empty() ? "" : (", NOT AS A WANDERER IS: " + wwhy).c_str()));
        struct Pin { int64_t sx, sz; const char* name; int type; };
        static const Pin PINS[] = {{150, 26, "Seapheeseeri II", PT_THINATMO}, {150, 26, "Seapheeseeri I-a", PT_THINATMO}, {150, 57, "Iaphiath I", PT_ICY}, {150, 57, "Iaphiath II", PT_ICY},
                                   {150, 57, "Iaphiath III", PT_ICY}, {150, 58, "Feeyeithis V", PT_ICY}, {150, 36, "Wyariothmai IV", PT_CRATERED}, {150, 36, "Wyariothmai V", PT_ICY},
                                   {HOME_SX, HOME_SZ, "Heilya IV", PT_CRATERED}, {HOME_SX, HOME_SZ, "Heilya V", PT_ICY}, {HOME_SX, HOME_SZ, "Heilya IV-a", PT_THINATMO}};
        std::string pins; bool pinsOk = true;
        for (const Pin& P : PINS) {
            Star st; bool seen = false;
            if (starInSector(P.sx, 0, P.sz, st)) {
                StarSystem sys;
                sys.generate(st);
                for (const Body& B : sys.bodies)
                    if (B.name == P.name) {
                        seen = true;
                        if (B.type != P.type) {
                            pinsOk = false;
                            pins += fmt("%s %s; ", P.name, KINDS[B.type].name);
                        }
                    }
            }
            if (!seen) { pinsOk = false; pins += fmt("%s not found; ", P.name); }
        }
        check("far cold kinds: the regress's and home's cold worlds as drawn (P-05)", pinsOk,
                pins.empty() ? "Seapheeseeri II and I-a, Iaphiath I-III, Feeyeithis V, Wyariothmai IV-V, Heilya IV, V and IV-a" : pins);
    }
    // the far cold's grounds, each kind's signature on its first world: the nitrogen's great plain at its level, dark tholin at the low
    // latitudes; the cryovolcanic cap in the south and a plume's dark streak downwind of its vent; the dry ice caps, their band's frost in the
    // hemisphere's winter and gone by its summer, its spiders' jets only under that frost; the wanderer's snow in its lowlands and its highest
    // ground bare; the brine dwarf's lone mountain three kilometres and more over its plain and salt on its young craters' floors;
    // the frost dwarf's frost over most of it, its streaks of tholin
    {
        std::string out; bool ok = true;
        DrainageOff dOff;
        for (int k = PT_NITROGEN; k <= PT_FROST_DWARF; k++) {
            if (coldFirst[k] < 0) { ok = false; out += fmt("%s none; ", KINDS[k].name); continue; }
            Star st; starInSector(coldX[k], 0, coldZ[k], st); StarSystem sys; sys.generate(st);
            const Body& B = sys.bodies[coldFirst[k]];
            const BodyGen g = BodyGen::make(B);
            double all = 0, n[MAT_COUNT] = {0}, lowDust = 0, capS = 0, capSnow = 0, northSnow = 0;
            std::vector<std::pair<double, int>> hs;
            for (int j = 0; j < 64; j++)
                for (int i = 0; i < 128; i++) {
                    const double la = (0.5 - (j + 0.5) / 64) * PI, lo = (i + 0.5) / 128 * TAU - PI, w = std::cos(la);
                    const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 20000);
                    all += w; n[ss.material] += w;
                    if (ss.material == MAT_DUST && std::fabs(la) < 30 * DEG) lowDust += w;
                    if (la / DEG < -(g.iceCapLat + 6)) { capS += w; if (ss.material == MAT_SNOW || ss.material == MAT_DUST) capSnow += w; }
                    if (la / DEG > 10 && ss.material == MAT_SNOW) northSnow += w;
                    hs.push_back({ss.height, ss.material});
                }
            bool pass = true; std::string what;
            switch (k) {
                case PT_NITROGEN: {
                    const SurfaceSample mid = sampleSurface(g, nitrogenBasinOf(g), 512);
                    pass = mid.material == MAT_ICE && std::fabs(mid.height - g.seaLevel) < 120 && lowDust > 0.6 * n[MAT_DUST] && n[MAT_DUST] > 0.03 * all;
                    what = fmt("the great plain's middle %s at %.0f m (its level %.0f), tholin %.0f%% of the world, %.0f%% of it within 30 degrees of the equator", MATERIAL_NAMES[mid.material],
                            mid.height, g.seaLevel, 100 * n[MAT_DUST] / all, 100 * lowDust / std::max(1e-9, n[MAT_DUST]));
                    break;
                }
                case PT_CRYOVOLCANIC: {
                    bool streak = false;
                    for (int j = 0; j < 24 && !streak; j++)
                        for (int i = 0; i < 24 && !streak; i++) {
                            std::vector<GeyserVent> vents; plumeVents(g, StarSystem::bodyFromLatLon(-(g.iceCapLat + 4 + j * 2.5) * DEG, i * 15 * DEG - PI), vents);
                            for (const GeyserVent& v : vents) if (sampleSurface(g, normalize(v.unit + prevailingWind(g, v.unit) * (10.0 / g.R)), 512).material == MAT_DUST) streak = true;
                        }
                    pass = capSnow > 0.9 * capS && northSnow == 0 && streak;
                    what = fmt("the cap %.0f%% frost and streaks south of %.0f degrees, frost north of 10 degrees %.1f%%, %s", 100 * capSnow / std::max(1e-9, capS), g.iceCapLat + 6,
                            100 * northSnow / all, streak ? "a streak 10 km downwind of a vent" : "NO STREAK DOWNWIND");
                    break;
                }
                case PT_DRY_ICE: {
                    const Vec3 band = StarSystem::bodyFromLatLon((g.iceCapLat - 6) * DEG, 0.3), cap = StarSystem::bodyFromLatLon((g.iceCapLat + 8) * DEG, 0.3);
                    const BodyGen winter = BodyGen::make(B, -1.0), summer = BodyGen::make(B, 1.0);
                    const int mW = sampleSurface(winter, band, 512).material, mS = sampleSurface(summer, band, 512).material, mC = sampleSurface(summer, cap, 512).material;
                    std::vector<GeyserVent> jW, jS; springJets(winter, band, jW); springJets(summer, band, jS);
                    pass = mW == MAT_SNOW && mS != MAT_SNOW && mC == MAT_ICE && !jW.empty() && jS.empty();
                    what = fmt("its band %s in the north's winter, %s in its summer, its cap %s; %zu spiders' jets near in winter, %zu in summer", MATERIAL_NAMES[mW], MATERIAL_NAMES[mS],
                            MATERIAL_NAMES[mC], jW.size(), jS.size());
                    break;
                }
                case PT_WANDERER: {
                    std::sort(hs.begin(), hs.end());
                    int top = 0, topSnow = 0;
                    for (size_t q = hs.size() * 9 / 10; q < hs.size(); q++) { top++; if (hs[q].second == MAT_SNOW) topSnow++; }
                    pass = n[MAT_SNOW] > 0.4 * all && n[MAT_SNOW] < 0.97 * all && topSnow < 0.3 * top;
                    what = fmt("snow %.0f%% of the world, %.0f%% of its highest tenth", 100 * n[MAT_SNOW] / all, 100.0 * topSnow / std::max(1, top));
                    break;
                }
                case PT_BRINE_DWARF: {
                    const Vec3 m = loneMountainOf(g), e = normalize(cross(m, std::fabs(m.z) < 0.9 ? Vec3(0, 0, 1) : Vec3(1, 0, 0)));
                    const double top = sampleSurface(g, m, 512).height, foot = sampleSurface(g, normalize(m + e * (30.0 / g.R)), 512).height;
                    int salt = 0;
                    for (int j = 0; j < 90; j++)
                        for (int i = 0; i < 180; i++)
                            if (sampleSurface(g, StarSystem::bodyFromLatLon((j - 44.5) * 2 * DEG, i * 2 * DEG - PI), 512).material == MAT_SALT) salt++;
                    pass = top - foot > 3000 && salt > 0;
                    what = fmt("the lone mountain %.0f m over the plain 30 km off, salt at %d of 16200 places", top - foot, salt);
                    break;
                }
                default: {   // the frost dwarf
                    pass = n[MAT_SNOW] + n[MAT_ICE] > 0.7 * all && n[MAT_DUST] > 0.01 * all;
                    what = fmt("frost %.0f%%, tholin %.1f%%", 100 * (n[MAT_SNOW] + n[MAT_ICE]) / all, 100 * n[MAT_DUST] / all);
                    break;
                }
            }
            if (!pass) ok = false;
            out += fmt("%s %s: %s%s; ", KINDS[k].name, B.name.c_str(), what.c_str(), pass ? "" : " FAILS");
        }
        check("far cold grounds: each kind's signature on its first world (P-05)", ok, out);
    }
}

// P-06: the strange moons and their grounds
void unitMoons(const Check& check) {
    // the strange moons. Over the scan region: each made only from its sources (the drawn kind, a giant's moon for the grooved, the two-faced
    // and the tiger-striped, the moon's place among its planet's moons and its spin, the patchwork and the sponge under 200 K), the grooved large;
    // every two-faced moon keeps its apex forward (a quarter of its month on, within 25 degrees of its motion round its planet); the
    // regress's and home's moons as drawn
    int moonFirst[PT_COUNT]; int64_t moonX[PT_COUNT], moonZ[PT_COUNT];
    for (int k = 0; k < PT_COUNT; k++) { moonFirst[k] = -1; moonX[k] = moonZ[k] = 0; }
    {
        int made = 0, bad = 0, perKind[PT_COUNT] = {0}, facing = 0, apexBad = 0, giantMoons = 0, giantStrange = 0;
        double worstApex = 0; std::string why, awhy;
        for (auto& [x, z, st, sys] : scanSystems(150, 340, 20, 160)) {
            std::vector<int> place(sys.bodies.size(), 0);
            for (const Body& B : sys.bodies) {
                if (B.parent < 0 || B.wanderer || B.type == PT_COMET) continue;
                const Body& P = sys.bodies[B.parent];
                if (P.type == PT_COMPANION) continue;
                const bool cap = B.orbitPeriod < 0, regular = !cap && !B.doublePlanet;
                const int c = cap ? -1 : place[B.parent]++;   // its place as the generator counts it (a twin first)
                const bool giant = kindHas(drawnKind(P), KT_GIANT), facingB = B.rotPeriod == B.orbitPeriod;
                if (giant && regular) { giantMoons++; if (B.type >= PT_GROOVED_ICE && B.type <= PT_SPONGE) giantStrange++; }
                if (B.type == PT_TWO_FACED) {
                    facing++;
                    const double t = 0.25 * B.orbitPeriod;
                    const Vec3 v = normalize(sys.bodyVel(B.index, t) - sys.bodyVel(B.parent, t)), a = sys.bodyFrame(B.index, t).transposed() * B.apexDir;
                    const double off = std::acos(clampd(dot(normalize(a), v), -1, 1));
                    worstApex = std::max(worstApex, off);
                    if (off > 25 * DEG || length(B.apexDir) < 0.99) { apexBad++; awhy = B.name; }   // its tilt, its orbit's inclination, an oval orbit's libration
                }
                if (B.type < PT_GROOVED_ICE || B.type > PT_SPONGE) continue;   // the giants have checks of their own
                made++; perKind[B.type]++;
                if (moonFirst[B.type] < 0) { moonFirst[B.type] = B.index; moonX[B.type] = x; moonZ[B.type] = z; }
                const int d = drawnKind(B); const double T = B.tempK;
                bool ok = regular;
                switch (B.type) {
                    case PT_GROOVED_ICE: ok = ok && d == PT_ICY && giant && B.radiusKm >= 1900; break;
                    case PT_TWO_FACED: ok = ok && d == PT_ICY && giant && c >= 3 && facingB && T >= 60; break;
                    case PT_TIGER_STRIPED: ok = ok && (d == PT_ICY || d == PT_EUROPAN) && giant && c < 2; break;
                    case PT_PATCHWORK: ok = ok && (d == PT_ICY || d == PT_ROCKY) && c >= 1 && T < 200; break;
                    default: ok = ok && (d == PT_CRATERED || d == PT_ICY) && !facingB && T < 200; break;   // the sponge
                }
                if (!ok) { bad++; why = B.name + " made a " + KINDS[B.type].name + " from " + KINDS[d].name; }
            }
        }
        std::string counts; bool every = true;
        for (int k = PT_GROOVED_ICE; k <= PT_SPONGE; k++) { counts += fmt("%s%s %d", k > PT_GROOVED_ICE ? ", " : "", KINDS[k].name, perKind[k]); if (perKind[k] < 30) every = false; }
        check("strange moons: from their sources: a giant's large moons of ice, its outer ones facing it, its inner ones; moons in a resonance, small tumbling ones (P-06)",
                bad == 0 && every && made > 500,
                fmt("%d made (%s); %d of a giant's %d moons (%.0f%%)%s", made, counts.c_str(), giantStrange, giantMoons, 100.0 * giantStrange / std::max(1, giantMoons),
                        why.empty() ? "" : (": " + why).c_str()));
        check("strange moons: a two-faced moon keeps its apex, its dark face's middle, forward along its orbit (P-06)", apexBad == 0 && facing == perKind[PT_TWO_FACED] && facing > 30,
              fmt("%d two-faced moons, the worst %.1f degrees off a quarter month on%s", facing, worstApex / DEG, awhy.empty() ? "" : (", OFF: " + awhy).c_str()));
        struct Pin { int64_t sx, sz; const char* name; int type; };
        static const Pin PINS[] = {{150, 26, "Seapheeseeri II-a", PT_ICY}, {150, 36, "Wyariothmai IV-a", PT_ROCKY}, {150, 36, "Wyariothmai V-b", PT_ICY}, {150, 57, "Iaphiath I-a", PT_ICY},
                                   {150, 58, "Feeyeithis IV-a", PT_ROCKY}, {HOME_SX, HOME_SZ, "Heilya II-b", PT_ROCKY}};
        std::string pins; bool pinsOk = true;
        for (const Pin& P : PINS) {
            Star st; bool seen = false;
            if (starInSector(P.sx, 0, P.sz, st)) {
                StarSystem sys;
                sys.generate(st);
                for (const Body& B : sys.bodies)
                    if (B.name == P.name) {
                        seen = true;
                        if (B.type != P.type) {
                            pinsOk = false;
                            pins += fmt("%s %s; ", P.name, KINDS[B.type].name);
                        }
                    }
            }
            if (!seen) { pinsOk = false; pins += fmt("%s not found; ", P.name); }
        }
        check("strange moons: the regress's and home's moons as drawn (P-06)", pinsOk, pins.empty() ? "Seapheeseeri II-a, Wyariothmai IV-a and V-b, Iaphiath I-a, Feeyeithis IV-a, Heilya II-b" : pins);
    }
    // the strange moons' grounds, each kind's signature on its first moon: the grooved moon's lanes of bright ice and its dark polygons between;
    // the two-faced moon's apex coal-black and the point opposite bright, its ridge kilometres over the plain south of it; the tiger-striped moon
    // white all over, its stripes' middles blue ice in a rift with their jets; the patchwork's great cliff ten kilometres and more over its foot;
    // the sponge's pits kilometres deep, dark dust in their floors
    {
        std::string out; bool ok = true;
        DrainageOff dOff;
        for (int k = PT_GROOVED_ICE; k <= PT_SPONGE; k++) {
            if (moonFirst[k] < 0) { ok = false; out += fmt("%s none; ", KINDS[k].name); continue; }
            Star st; starInSector(moonX[k], 0, moonZ[k], st); StarSystem sys; sys.generate(st);
            const Body& B = sys.bodies[moonFirst[k]];
            const BodyGen g = BodyGen::make(B);
            double all = 0, n[MAT_COUNT] = {0}, alb = 0, low = 0, lowN[MAT_COUNT] = {0};
            for (int j = 0; j < 64; j++)
                for (int i = 0; i < 128; i++) {
                    const double la = (0.5 - (j + 0.5) / 64) * PI, lo = (i + 0.5) / 128 * TAU - PI, w = std::cos(la);
                    const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 20000);
                    all += w; n[ss.material] += w; alb += w * ss.albedo;
                    if (std::fabs(la) < 30 * DEG) { low += w; lowN[ss.material] += w; }
                }
            bool pass = true; std::string what;
            switch (k) {
                case PT_GROOVED_ICE:
                    pass = lowN[MAT_ICE] > 0.25 * low && lowN[MAT_DUST] > 0.2 * low;
                    what = fmt("within 30 degrees of the equator lanes %.0f%%, dark polygons %.0f%%", 100 * lowN[MAT_ICE] / low, 100 * lowN[MAT_DUST] / low);
                    break;
                case PT_TWO_FACED: {
                    const Vec3 a = twoFacedApex(g);
                    const SurfaceSample ap = sampleSurface(g, a, 512), op = sampleSurface(g, a * -1.0, 512);
                    const double alon = std::atan2(a.y, a.x),
                                 rise = sampleSurface(g, StarSystem::bodyFromLatLon(0, alon), 2048).height - sampleSurface(g, StarSystem::bodyFromLatLon(-60.0 / g.R, alon), 2048).height;
                    pass = ap.material == MAT_DUST && ap.albedo < 0.08 && op.material == MAT_ICE && op.albedo > 0.45 && rise > 4000 && n[MAT_DUST] > 0.2 * all && n[MAT_DUST] < 0.6 * all;
                    what = fmt("the apex %s %.2f, opposite %s %.2f, the dark face %.0f%% of the moon, the ridge %.0f m over the plain 60 km south", MATERIAL_NAMES[ap.material], ap.albedo,
                            MATERIAL_NAMES[op.material], op.albedo, 100 * n[MAT_DUST] / all, rise);
                    break;
                }
                case PT_TIGER_STRIPED: {
                    const TigerStripes T = tigerStripesOf(g);
                    int blue = 0, rift = 0, jets = 0;
                    for (int q = 0; q < 4; q++) {
                        const Vec3 m = stripePoint(T, g.R, q, 0);
                        const SurfaceSample mid = sampleSurface(g, m, 16), side = sampleSurface(g, normalize(m + T.e2 * (3.5 / g.R)), 16);   // across the stripe, past its flanking ridge
                        if (mid.material == MAT_ICE) blue++;
                        if (mid.height < side.height - 300) rift++;
                        std::vector<GeyserVent> vents; curtainVents(g, m, vents); if (!vents.empty()) jets++;
                    }
                    pass = n[MAT_SNOW] > 0.85 * all && alb / all > 0.85 && blue == 4 && rift == 4 && jets == 4;
                    what = fmt("snow %.0f%%, mean albedo %.2f; of four stripes' middles %d blue ice, %d in a rift 300 m deep, %d with jets", 100 * n[MAT_SNOW] / all, alb / all, blue, rift, jets);
                    break;
                }
                case PT_PATCHWORK: {
                    Vec3 foot, up; const bool has = greatCliffOf(g, foot, up);
                    const double rise = has ? sampleSurface(g, normalize(foot + up * (8.0 / g.R)), 512).height - sampleSurface(g, foot, 512).height : 0;
                    pass = has && rise > 10000;
                    what = fmt("the great cliff %.0f m over its foot", rise);
                    break;
                }
                default: {   // the sponge
                    double lo = 1e18, hi = -1e18; int dark = 0, tot = 0;
                    const Vec3 c0 = StarSystem::bodyFromLatLon(10 * DEG, 0.3), e1 = normalize(cross(c0, Vec3(0, 0, 1))), e2 = cross(c0, e1);
                    for (int j = 0; j < 40; j++) for (int i = 0; i < 40; i++) {
                        const SurfaceSample ss = sampleSurface(g, normalize(c0 + e1 * ((i - 19.5) / g.R) + e2 * ((j - 19.5) / g.R)), 512);
                        lo = std::min(lo, ss.height); hi = std::max(hi, ss.height); tot++; if (ss.material == MAT_DUST) dark++;
                    }
                    pass = hi - lo > 3000 && dark > 0.05 * tot;
                    what = fmt("over 40 km %.0f m from floor to rim, dark floors %.0f%%", hi - lo, 100.0 * dark / tot);
                    break;
                }
            }
            if (!pass) ok = false;
            out += fmt("%s %s: %s%s; ", KINDS[k].name, B.name.c_str(), what.c_str(), pass ? "" : " FAILS");
        }
        check("strange moons' grounds: each kind's signature on its first moon (P-06)", ok, out);
    }
}

// P-07: the giants
void unitGiants(const Check& check) {
    // the giants. Over the scan region: each made only from its sources (the drawn kind, the temperature band on the game's scale, the body, the
    // star, a hot giant only from a planet locked to its star, never a double planet's half), no people's moon moved by one, a hot giant round about one
    // yellow star in a hundred; each class's air on its first giant (the water-cloud's water on top, the azure clear to the deep, the alkali dark with no
    // stratosphere hotter than where the heat ends a probe, the silicate's rock over iron, the gas dwarf's haze and its rich air, the super-puff's weak
    // pull spreading its air); each class's globe (the water-cloud's bright, the azure's blue, the alkali's and the silicate's night sides glowing, the
    // gas dwarf's without bands); the test systems' bodies as drawn
    {
        int made = 0, bad = 0, perKind[PT_COUNT] = {0}, yellow = 0, yellowHot = 0, peopleMoved = 0;
        int giantFirst[PT_COUNT]; int64_t gX[PT_COUNT], gZ[PT_COUNT];
        for (int k = 0; k < PT_COUNT; k++) { giantFirst[k] = -1; gX[k] = gZ[k] = 0; }
        std::string why;
        for (auto& [x, z, st, sys] : scanSystems(150, 340, 20, 160)) {
            bool hot = false;
            for (const Body& B : sys.bodies) {
                if (B.type < PT_SUPER_PUFF || B.wanderer) continue;
                made++; perKind[B.type]++;
                if (giantFirst[B.type] < 0) { giantFirst[B.type] = B.index; gX[B.type] = x; gZ[B.type] = z; }
                if (B.type == PT_ALKALI || B.type == PT_SILICATE) hot = true;
                const bool compW = B.parent >= 0 && sys.bodies[B.parent].type == PT_COMPANION;
                const uint32_t body = compW ? KB_COMPANION_WORLD : (B.parent < 0 ? KB_PLANET : KB_MOON);
                const int cls = compW ? sys.bodies[B.parent].starClass : st.cls, d = drawnKind(B);
                const KindDef& K = KINDS[B.type];
                bool from = false;
                for (int i = 0; i < K.nSources; i++) {
                    const KindSource& S = K.sources[i];
                    if (!(S.from >> d & 1u) || B.tempK < S.tLo || B.tempK >= S.tHi || !(S.bodies & body) || (S.stars && !(S.stars >> cls & 1u))) continue;
                    if (S.lock > 0 && !(B.locked && B.rotPeriod == B.orbitPeriod)) continue;
                    from = true;
                }
                if (!from || B.doublePlanet) { bad++; why = B.name + " made a " + K.name + " from " + KINDS[d].name + fmt(" at %.0f K", B.tempK); }
                for (const Body& M : sys.bodies) if (M.parent == B.index && M.drawnOrbitKm >= 0 && peoplesHash(M.type, M.seed)) { peopleMoved++; why = M.name + ", a people's world, moved"; }
            }
            if (st.cls == STAR_YELLOW) { yellow++; if (hot) yellowHot++; }
        }
        std::string counts; bool every = true;
        // the giants' (the dark ocean after them)
        for (int k = PT_SUPER_PUFF; k <= PT_GAS_DWARF; k++) { counts += fmt("%s%s %d", k > PT_SUPER_PUFF ? ", " : "", KINDS[k].name, perKind[k]); if (perKind[k] < 2) every = false; }
        const double hotShare = (double)yellowHot / std::max(1, yellow);
        check("giants: from their sources, never a double planet's half, no people's moon moved, a hot giant round a yellow star in a hundred (P-07)",
              bad == 0 && peopleMoved == 0 && every && made > 400 && hotShare > 0.004 && hotShare < 0.025,
              fmt("%d made (%s); %d of %d yellow stars hold a hot giant (%.1f%%)%s", made, counts.c_str(), yellowHot, yellow, 100 * hotShare, why.empty() ? "" : (": " + why).c_str()));
        // the air of each class's first giant
        std::string air; bool airOk = true;
        for (int k = PT_SUPER_PUFF; k <= PT_GAS_DWARF; k++) {
            if (giantFirst[k] < 0) { airOk = false; air += fmt("%s none; ", KINDS[k].name); continue; }
            Star st; starInSector(gX[k], 0, gZ[k], st); StarSystem sys; sys.generate(st);
            const Body& B = sys.bodies[giantFirst[k]];
            const GiantAtmosphere a = giantAtmosphereOf(sys, giantFirst[k]);
            ProbeFlight f; f.crushBar = 25;
            const double endBar = probeEndBar(a, f), topK = giantTemperatureK(a, 1e-6), endK = giantTemperatureK(a, endBar);
            bool pass = a.valid && a.ch.kind == GK_WATER + (KINDS[k].giantClass - GC_WATER);
            std::string what;
            switch (k) {
                case PT_WATER_CLOUD: {
                    bool cold = false; for (int q = 0; q < a.decks; q++) if (a.species[q] == CS_AMMONIA || a.species[q] == CS_METHANE) cold = true;
                    pass = pass && a.species[0] == CS_WATER && !cold;
                    what = fmt("%.0f K at one bar, its first deck %s at %.2f bar", a.t1K, CLOUD_SPECIES[a.species[0]].name, a.top[0]);
                    break;
                }
                case PT_AZURE:
                    pass = pass && a.decks == 1 && a.species[0] == CS_SALT && a.top[0] > 10 && endBar < a.top[0] / 4;
                    what = fmt("%.0f K at one bar, nothing over the salts' haze at %.1f bar, the probe's end at %.2f bar", a.t1K, a.top[0], endBar);
                    break;
                case PT_ALKALI:
                    pass = pass && a.species[0] == CS_SILICATE && a.t1K >= 1100 && topK <= endK + 1 && a.absorb > 1 && a.top[0] > endBar;
                    what = fmt("%.0f K at one bar, %.0f K at the top, the probe's end %.0f K at %.2f bar over the rock deck at %.2f", a.t1K, topK, endK, endBar, a.top[0]);
                    break;
                case PT_SILICATE:
                    pass = pass && a.decks >= 2 && a.species[0] == CS_SILICATE && a.species[1] == CS_IRON && a.t1K >= 1800 && a.ch.glow > 0;
                    what = fmt("%.0f K at one bar, rock at %.2f-%.2f bar, iron at %.2f-%.2f", a.t1K, a.top[0], a.base[0], a.top[1], a.base[1]);
                    break;
                case PT_GAS_DWARF:
                    pass = pass && a.hazeK >= 3 && a.ch.abundance > 1.6 && B.radiusKm <= 24000;
                    what = fmt("haze x%.1f, the condensables x%.1f, R %.0f km", a.hazeK, a.ch.abundance, B.radiusKm);
                    break;
                default:   // the super-puff
                    pass = pass && B.gravity < 2.5 && giantAltitudeKm(a, 0.01) > 600 && a.ch.young;
                    what = fmt("g %.1f m/s2, 0.01 bar %.0f km over one bar", B.gravity, giantAltitudeKm(a, 0.01));
                    break;
            }
            if (!pass) airOk = false;
            air += fmt("%s %s: %s%s; ", KINDS[k].name, B.name.c_str(), what.c_str(), pass ? "" : " FAILS");
        }
        check("giants: each class's air on its first giant (P-07)", airOk, air);
        // each class's globe: its map's albedo and colour, and the hot ones' night sides drawn
        std::string globe; bool globeOk = true;
        SpaceRenderer gsr; StarNeighborhood gnb;
        for (int k = PT_SUPER_PUFF; k <= PT_GAS_DWARF; k++) {
            if (giantFirst[k] < 0) continue;
            Star st; starInSector(gX[k], 0, gZ[k], st); StarSystem sys; sys.generate(st);
            const int bi = giantFirst[k]; const Body& B = sys.bodies[bi];
            const BodyGen g = BodyGen::make(B);
            PlanetMap m; m.generate(g);
            double sum = 0, sum2 = 0; const int nPix = PlanetMap::W * PlanetMap::H;
            for (int i = 0; i < nPix; i++) { const double v = m.albedo[i] / 255.0; sum += v; sum2 += v * v; }
            const double mean = sum / nPix, sd = std::sqrt(std::max(0.0, sum2 / nPix - mean * mean));
            bool pass = true; std::string what = fmt("map albedo %.2f +- %.2f", mean, sd);
            if (k == PT_WATER_CLOUD) pass = mean > 0.6;
            if (k == PT_AZURE) pass = B.color.b > 1.6 * B.color.r;
            if (k == PT_GAS_DWARF) pass = sd < 0.05;
            if (k == PT_ALKALI || k == PT_SILICATE) {   // drawn from its night side: the disc's pixels red and lit
                const double t = 5000.0;
                const Vec3 bp = sys.bodyPos(bi, t), toStar = normalize(sys.star.pos - bp);
                const Vec3 shipPos = bp - toStar * (B.radiusKm * 3.5), fwd = normalize(bp - shipPos);
                gnb.update(shipPos);
                Framebuffer fb; SpaceContext c;
                c.sys = &sys; c.stars = &gnb.stars; c.t = t; c.shipPos = shipPos; c.cam = cameraBasis(std::atan2(fwd.x, fwd.z), std::asin(fwd.y)); c.bankBodyA = bi;
                gsr.mapFor(B); gsr.setupPalette(fb, &sys, bi, -1, 1.0); gsr.render(fb, c);
                std::vector<uint32_t> px((size_t)FBW * FBH); fb.toRGB(px.data(), 1.0);
                int red = 0, disc = 0;
                const int cx = FBW / 2, cy = FBH / 2, r = FBH / 6;
                for (int y = cy - r; y <= cy + r; y++) for (int x = cx - r; x <= cx + r; x++) {
                    const uint32_t q = px[(size_t)y * FBW + x]; const int R = q & 255, G = (q >> 8) & 255, Bc = (q >> 16) & 255;
                    disc++; if (R > 30 && R > G + G / 2 && R > Bc + Bc / 2) red++;
                }
                pass = red > disc / 2;
                what += fmt(", its night side red at %d of %d pixels", red, disc);
            }
            if (!pass) globeOk = false;
            globe += fmt("%s: %s%s; ", KINDS[k].name, what.c_str(), pass ? "" : " FAILS");
        }
        check("giants: each class's globe (P-07)", globeOk, globe);
        struct Pin { int64_t sx, sz; const char* name; int type; };
        static const Pin PINS[] = {{150, 106, "Wailie II", PT_CRATERED}, {150, 106, "Wailie VI", PT_GASGIANT}, {150, 66, "Thilawiane III", PT_GASGIANT},
                                   {150, 36, "Wyariothmai I", PT_FELISIAN}, {150, 58, "Feeyeithis I", PT_QUARTZ}, {HOME_SX, HOME_SZ, "Heilya II", PT_FELISIAN}};
        std::string pins; bool pinsOk = true;
        for (const Pin& P : PINS) {
            Star st; bool seen = false;
            if (starInSector(P.sx, 0, P.sz, st)) {
                StarSystem sys;
                sys.generate(st);
                for (const Body& B : sys.bodies)
                    if (B.name == P.name) {
                        seen = true;
                        if (B.type != P.type) {
                            pinsOk = false;
                            pins += fmt("%s %s; ", P.name, KINDS[B.type].name);
                        }
                    }
            }
            if (!seen) { pinsOk = false; pins += fmt("%s not found; ", P.name); }
        }
        check("giants: the probe's, the regress's and home's worlds as drawn (P-07)", pinsOk, pins.empty() ? "Wailie II and VI, Thilawiane III, Wyariothmai I, Feeyeithis I, Heilya II" : pins);
    }
}

// Q-01: the six slots, the facts and the eleven landforms
void unitTraits(const Check& check) {
    // the six slots, the facts and the eleven landforms. Over the scan region's landable worlds: the first three slots as P-07 drew them (a
    // hash of the first 2,000 worlds' kinds and slots, from P-07's binary), the new three of distinct kinds of trait, each eligible on its kind,
    // none a trait a slot before it excludes, none moving a people's ground, packed; a fifth of the worlds a new landform fits with none; every new
    // landform carried; home's ground as it was. Every body's facts as its orbit and spin say, each fact seen. A sight's kind through the guide's file (R-408's numbers kept).
    // The acidic world's karst standing (its moisture and its warmth the world's). Each new landform's ground marked and its sight heard there
    {
        uint64_t h = 1469598103934665603ULL;
        auto mixIn = [&](int64_t v) { for (int i = 0; i < 8; i++) { h ^= (uint64_t)((v >> (8 * i)) & 0xff); h *= 1099511628211ULL; } };
        int n = 0, fits = 0, fitsNone = 0, carried[TR_COUNT] = {0}, factN[BF_COUNT] = {0}, bodies = 0;
        bool slotsOk = true, factsOk = true; std::string why, fwhy;
        struct Carrier { Star s; int bi = -1; };
        Carrier first[TR_COUNT], acidKarst;
        for (auto& [x, z, st, sys] : scanSystems(150, 340, 20, 160)) {
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type == PT_COMPANION) continue;
                const unsigned f = bodyFacts(sys, bi); bodies++;
                for (int k = 0; k < BF_COUNT; k++) if ((f >> k) & 1u) factN[k]++;
                const bool moon = b.parent >= 0 && sys.bodies[b.parent].type != PT_COMPANION;
                const bool want[BF_COUNT] = {b.axialTilt > 60 * DEG, b.rotPeriod < 0, !b.locked && std::fabs(b.rotPeriod) < 1800, b.locked, (moon ? sys.bodies[b.parent].ecc : b.ecc) > 0.2,
                                             magneticClass(magneticField(b)) == 3, magneticClass(magneticField(b)) == 0, moon && b.orbitPeriod < 0, b.doublePlanet, b.rings};
                for (int k = 0; k < BF_COUNT; k++) if ((((f >> k) & 1u) != 0) != want[k]) { factsOk = false; fwhy = fmt("%s %s", b.name.c_str(), BODY_FACT_NAMES[k]); }
                if (!KINDS[b.type].landable) continue;
                const BodyGen g = BodyGen::make(b);
                if (n < 2000) { mixIn(b.type); mixIn(g.traits[0]); mixIn(g.traits[1]); mixIn(g.traits[2]); }
                n++;
                const bool people = g.hasTrait(TR_CIVILISATION) || (x == HOME_SX && z == HOME_SZ && b.parent < 0 && b.type == PT_FELISIAN);   // a people's world, or home
                bool fit = false;
                for (int t = TR_CIVILISATION + 1; t < TR_COUNT; t++) if (TRAITS[t].kind == TK_LANDFORM && traitEligible(b.type, t) && !people) fit = true;
                int kinds = 0, got = 0;
                for (int j = 3; j < TRAIT_SLOTS; j++) {
                    const int t = g.traits[j];
                    if (!t) { for (int jj = j + 1; jj < TRAIT_SLOTS; jj++) if (g.traits[jj]) { slotsOk = false; why = b.name + ": a gap"; } break; }
                    got++; carried[t]++;
                    if (!first[t].s.seed) { first[t].s = st; first[t].bi = bi; }
                    if (TRAITS[t].kind == TK_FIRST || !traitEligible(b.type, t) || (people && TRAITS[t].moves) || ((kinds >> TRAITS[t].kind) & 1)) {
                        slotsOk = false;
                        why = b.name + ": " + TRAITS[t].name;
                    }
                    kinds |= 1 << TRAITS[t].kind;
                    for (int o = 0; o < TRAIT_SLOTS; o++) {
                        const int u = g.traits[o];
                        if (o == j || !u) continue;
                        if (u == t || TRAITS[t].excludes == u || TRAITS[u].excludes == t || (TRAITS[t].colour && TRAITS[u].colour)) {
                            slotsOk = false;
                            why = b.name + ": " + TRAITS[t].name + " with " + TRAITS[u].name;
                        }
                    }
                }
                if (fit) { fits++; if (!got) fitsNone++; }
                if (b.type == PT_ACIDIC && g.hasTrait(TR_KARST) && !acidKarst.s.seed) { acidKarst.s = st; acidKarst.bi = bi; }
            }
        }
        check("traits: the first three slots as P-07 drew them over 2,000 worlds (Q-01)", h == 0xee430ab584b04f72ULL, fmt("%016llx over %d", (unsigned long long)h, std::min(n, 2000)));
        std::string missing;
        // the dark's own are the drifters' (`rogues dark`)
        for (int t = TR_CIVILISATION + 1; t < TR_COUNT; t++) if (!carried[t] && TRAITS[t].kind != TK_DARK) missing += std::string(" ") + TRAITS[t].name;
        const double noneShare = fits ? (double)fitsNone / fits : 1.0;
        check("traits: the new slots of distinct kinds, eligible, none excluded or moving a people's ground, packed; a fifth with none (Q-01)",
                slotsOk && missing.empty() && noneShare > 0.15 && noneShare < 0.25,
                fmt("%s%s; %d of the %d worlds a new landform fits have none (%.1f%%)", why.c_str(), missing.empty() ? "" : (" not carried:" + missing).c_str(), fitsNone, fits, 100.0 * noneShare));
        std::string fl; bool seenAll = true;
        for (int k = 0; k < BF_COUNT; k++) { fl += fmt("%s %d, ", BODY_FACT_NAMES[k], factN[k]); if (!factN[k]) seenAll = false; }
        check("facts: as the orbit and the spin say, each seen (Q-01)", factsOk && seenAll && factN[BF_RACING] < bodies / 5 && factN[BF_SIDEWAYS] < bodies / 20,
                fwhy + fl + fmt("over %d bodies", bodies));
        bool fileOk = markKindToFile(MARK_PLACE) == 8 && markKindToFile(LM_WORK) == 9 && markKindFromFile(8) == MARK_PLACE && markKindFromFile(9) == LM_WORK && MARK_PLACE == LM_COUNT;
        for (int k = 0; k <= MARK_PLACE; k++) if (markKindFromFile(markKindToFile(k)) != k || (k < LM_WORK && markKindToFile(k) != k)) fileOk = false;
        check("marks: a sight's kind through the guide's file, R-408's numbers kept (Q-01)", fileOk);
        {   // the scans without the rivers (a tile per sample over the globe would take hours)
        DrainageOff off;
        int karstN = 0;
        if (acidKarst.bi >= 0) {
            StarSystem sys; sys.generate(acidKarst.s);
            const BodyGen g = BodyGen::make(sys.bodies[acidKarst.bi]);
            for (double la = -60; la <= 60; la += 3) for (int k = 0; k < 120; k++) if (sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, k * TAU / 120), 64.0).landformTrait == TR_KARST) karstN++;
        }
        check("traits: an acidic world's karst towers stand (Q-01)", karstN > 0,
                acidKarst.bi >= 0 ? fmt("%d of 4920 samples on its towers' walls", karstN) : std::string("no acidic world with karst"));
        std::string sights; bool sightsOk = true;
        for (int t = TR_CIVILISATION + 1; t < TR_COUNT; t++) {
            if (TRAITS[t].kind != TK_LANDFORM || first[t].bi < 0) continue;
            StarSystem sys; sys.generate(first[t].s);
            const BodyGen g = BodyGen::make(sys.bodies[first[t].bi]);
            Vec3 at; int marked = 0;
            const bool points = t == TR_CRATER_CHAINS || t == TR_MUD_VOLCANOES || t == TR_PINGOS || t == TR_MAARS;
            if (t == TR_GREAT_SHIELD) { at = greatShieldOf(g).c; marked = sampleSurface(g, at, 64.0).landformTrait == t; }   // its caldera
            else for (double la = -66; la <= 66 && !marked; la += 3)
                for (int k = 0; k < 120 && !marked; k++) {
                    const Vec3 u = StarSystem::bodyFromLatLon(la * DEG, k * TAU / 120);
                    if (!points) { if (sampleSurface(g, u, 64.0).landformTrait == t) { marked++; at = u; } continue; }
                    std::vector<LandFeature> fs; landformFeatures(g, t, u, 8.0, fs);   // a cone, a pingo, a maar or a chain's crater where its landform stands
                    for (const LandFeature& f : fs) if (sampleSurface(g, f.unit, 64.0).landformTrait == t) { marked++; at = f.unit; break; }
                }
            std::vector<Landmark> found;
            if (marked) sightsNear(g, at, 40000.0, 1u << landformSight(t), found, false);
            if (!marked || found.empty()) sightsOk = false;
            sights += fmt("%s %s %s; ", TRAITS[t].name, marked ? "marked" : "NOT MARKED", found.empty() ? "NO SIGHT" : fmt("%zu %s", found.size(), LANDMARK_KIND_NAMES[landformSight(t)]).c_str());
        }
        check("traits: each new landform marks its ground and the scanner hears its sight there (Q-01)", sightsOk, sights);
        }
        // the water, the sky, the life and the ground. Each carrier fits its world (its kind's process; the tides a forcing over the
        // threshold and a range within 5-16 m, the aurorae a strong field, the reefs a warm sea); the tides as their pulls say (a pull's forcing
        // its m/M R^4/d^3, the range met at the equator over a day the trait's within a tenth, none at a pole) and their flats wet sand under
        // the sea's plane; the waters' colours (a milky sea's patches, a bloom's streaks, a reef's lagoon shallow and turquoise, a pink lake's
        // whole water with the rivers' tiles built round it); the ground's marks (a strewn field heard by the scanner, a stratum's accent on the
        // slopes, the gypsum's dunes white, a petrified forest's density on the land, a hot spring's pool, its terraces' sinter)
        {
            std::string why2; bool fitOk = true;
            for (int t = TR_MAARS + 1; t <= TR_METEORITES; t++) {
                if (first[t].bi < 0) { fitOk = false; why2 += fmt("%s not carried; ", TRAITS[t].name); continue; }
                StarSystem sys; sys.generate(first[t].s);
                const Body& b = sys.bodies[first[t].bi];
                const BodyGen g = BodyGen::make(b);
                bool ok = traitEligible(b.type, t);
                if (t == TR_GREAT_TIDES) ok = ok && b.tideForcingM >= TIDE_FORCING_MIN && g.tideRangeM >= 5 && g.tideRangeM <= 16;
                if (t == TR_GREAT_AURORAE) ok = ok && magneticClass(magneticField(b)) == 3;
                if (t == TR_REEFS) ok = ok && climateTempC(g, 0, 0, 0, 0) >= 22 - 0.01;
                if (!ok) { fitOk = false; why2 += fmt("%s on %s; ", TRAITS[t].name, b.name.c_str()); }
            }
            check("qualities: each carried, on a world it fits (Q-02)", fitOk, why2.empty() ? std::string("16 traits") : why2);
        }
        {
            DrainageOff off;
            StarSystem sys; sys.generate(first[TR_GREAT_TIDES].s);
            const int bi = first[TR_GREAT_TIDES].bi; const Body& b = sys.bodies[bi];
            const BodyGen g = BodyGen::make(b);
            TidePull P[16]; const int np = tidePulls(sys, bi, P, 16);
            bool formula = np > 0;
            for (int k = 0; k < np; k++) {
                const double m = P[k].body < 0 ? sys.star.massFactor * 333000.0 : sys.bodies[P[k].body].massEarths;
                const double d =
                        P[k].body < 0 ? (b.parent >= 0 ? sys.bodies[b.parent].orbitRadiusKm : b.orbitRadiusKm) : (P[k].body == b.parent ? b.orbitRadiusKm : sys.bodies[P[k].body].orbitRadiusKm);
                const double want = m / b.massEarths * std::pow(b.radiusKm, 4) / (d * d * d) * 1000.0;
                if (std::fabs(P[k].forcingM - want) > 1e-6 * want + 1e-9) formula = false;
            }
            double lo = 1e9, hi = -1e9, poleMax = 0;
            const Vec3 eq = StarSystem::bodyFromLatLon(0, 0.4), pole = StarSystem::bodyFromLatLon(PI / 2, 0);
            const double day = std::max(std::fabs(b.rotPeriod), 600.0);
            for (double t = 1000.0; t < 1000.0 + 2 * day; t += day / 300.0) {
                const double v = tideOffsetM(sys, bi, eq, t, g.tideRangeM);
                lo = std::min(lo, v);
                hi = std::max(hi, v);
                poleMax = std::max(poleMax, std::fabs(tideOffsetM(sys, bi, pole, t, g.tideRangeM)));
            }
            int flat = 0;
            for (double la = -20; la <= 20 && flat < 3; la += 0.5)
                for (int k = 0; k < 360 && flat < 3; k++) {
                    const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, k * DEG), 64.0);
                    if (ss.material == MAT_SAND && ss.water > -1e8 && std::fabs(ss.water - (g.liquidLevel > -1e8 ? g.liquidLevel : 0.0)) < 0.01 &&
                            std::fabs(ss.height - ss.water) < 0.5 * tideRangeAt(g, StarSystem::bodyFromLatLon(la * DEG, k * DEG)))
                        flat++;
                }
            const double met = hi - lo;
            check("tides: their pulls' forcing, the range met at the equator the trait's, none at a pole, their flats wet sand under the sea (Q-02)",
                  formula && std::fabs(met - g.tideRangeM) < 0.12 * g.tideRangeM && poleMax < 0.01 * g.tideRangeM && flat >= 3,
                  fmt("%s: %d pulls%s, range %.2f met %.2f, at the pole %.3f, flat samples %d", b.name.c_str(), np, formula ? "" : " (A FORMULA MISMATCH)", g.tideRangeM, met, poleMax, flat));
        }
        {
            auto worldOf = [&](int t, StarSystem& sys, BodyGen& g) { sys.generate(first[t].s); g = BodyGen::make(sys.bodies[first[t].bi]); };
            auto count = [&](const BodyGen& g, const std::function<bool(const SurfaceSample&)>& f, double det) {
                int n = 0; for (double la = -60; la <= 60; la += 2) for (int k = 0; k < 180; k++) if (f(sampleSurface(g, StarSystem::bodyFromLatLon(la * DEG, k * 2 * DEG), det))) n++; return n; };
            StarSystem s1; BodyGen g1;
            int milky = 0, bloom = 0, reef = 0, reefDeep = 0;
            {
                DrainageOff off;
                worldOf(TR_MILKY_SEAS, s1, g1); milky = count(g1, [](const SurfaceSample& s) { return s.waterKind == 0 && s.material == MAT_WATER && s.waterTint > 0.3; }, 512);
                worldOf(TR_BLOOMS, s1, g1); bloom = count(g1, [](const SurfaceSample& s) { return s.material == MAT_WATER && s.lifeTint > 0.5; }, 512);
                worldOf(TR_REEFS, s1, g1); reef = count(g1, [](const SurfaceSample& s) { return s.material == MAT_WATER && s.lifeTint > 0.5 && s.height > -7.0; }, 512);
                reefDeep = count(g1, [](const SurfaceSample& s) { return s.material == MAT_WATER && s.lifeTint > 0.5 && s.height <= -7.0; }, 512);
            }
            int pink = 0;
            {   // a lake's water pink: the rivers' tiles round the world's land at 25 degrees
                worldOf(TR_PINK_LAKES, s1, g1);
                for (double la = -60; la <= 60 && !pink; la += 15)   // a degree's patch every 15 degrees of latitude and 30 of longitude
                    for (double lo = -180; lo < 180 && !pink; lo += 30)
                        for (double d = 0; d < 1.0 && !pink; d += 0.2) for (double e = 0; e < 1.0 && !pink; e += 0.2) {
                            const SurfaceSample ss = sampleSurface(g1, StarSystem::bodyFromLatLon((la + d) * DEG, (lo + e) * DEG), 128);
                            if ((ss.waterKind == 2 || ss.waterKind == 3 || ss.waterKind == 4) && ss.material == MAT_WATER && ss.waterTint > 0.99) pink++;
                        }
            }
            check("qualities: the waters' colours, a milky sea's patches, a bloom's streaks, a reef's lagoon shallow, a pink lake (Q-02)",
                    milky > 20 && bloom > 20 && reef > 10 && reefDeep == 0 && pink > 0,
                    fmt("milky %d, bloom %d, lagoon %d (deeper than 7 m %d), a pink lake's %s (%s)", milky, bloom, reef, reefDeep, pink ? "found" : "NOT FOUND",
                            s1.bodies[first[TR_PINK_LAKES].bi].name.c_str()));
            DrainageOff off;
            worldOf(TR_STRATA, s1, g1); const int strata = count(g1, [](const SurfaceSample& s) { return s.landformTrait == TR_STRATA && s.groundTint > 0.5; }, 16);
            worldOf(TR_GYPSUM, s1, g1); const int gyp = count(g1, [](const SurfaceSample& s) { return s.landformTrait == TR_GYPSUM && s.material == MAT_SALT; }, 64);
            worldOf(TR_PETRIFIED, s1, g1); const int petr = count(g1, [](const SurfaceSample& s) { return s.material != MAT_WATER && s.lifeTint > 0.3; }, 64);
            int springPool = 0, springSinter = 0;
            {
                worldOf(TR_HOT_SPRINGS, s1, g1);
                std::vector<GeyserVent> vents;
                for (double la = -50; la <= 50 && !springPool; la += 2) for (int k = 0; k < 180 && !springPool; k++) {
                    hotSpringsNear(g1, StarSystem::bodyFromLatLon(la * DEG, k * 2 * DEG), vents);
                    for (const GeyserVent& v : vents) if (sampleSurface(g1, v.unit, 4).material == MAT_BRINE) {
                        springPool++;
                        const Vec3 e = normalize(cross(v.unit, Vec3(0, 0, 1)));
                        for (double r = 0.15; r < 0.9; r += 0.05) if (sampleSurface(g1, normalize(v.unit * g1.R + e * (r * v.radiusKm / 0.14)), 4).material == MAT_SALT) springSinter++;
                        break;
                    }
                }
            }
            worldOf(TR_METEORITES, s1, g1);
            int strewn = 0; Vec3 at;
            for (double la = -60; la <= 60 && !strewn; la += 1)
                for (int k = 0; k < 360 && !strewn; k++) {
                    const Vec3 u = StarSystem::bodyFromLatLon(la * DEG, k * DEG);
                    const SurfaceSample ss = sampleSurface(g1, u, 64);
                    if (ss.landformTrait == TR_METEORITES && ss.groundTint > 0.4) {
                        strewn++;
                        at = u;
                    }
                }
            std::vector<Landmark> heard; if (strewn) sightsNear(g1, at, 40000.0, 1u << LM_METEORITES, heard, false);
            {   // the sky's by the hour: a fog sea's world fogged at its dawn and clear by noon; a great aurorae's night lit at 25 degrees, dark without them
                double fogDawn = -1, fogNoon = -1, aur = -1, aurWithout = -1;
                auto envAt = [&](int t, double latDeg, const std::function<bool(double alt, bool rising)>& when, const std::function<void(SurfaceView&)>& read, bool strip) {
                    StarSystem sy; sy.generate(first[t].s); const int bi = first[t].bi;
                    const BodyGen g = BodyGen::make(sy.bodies[bi]);
                    double la = latDeg * DEG, lo = 0;
                    for (int i = 0; i < 72; i++) {
                        const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(la, i * 5 * DEG), 512);
                        if (ss.material != MAT_WATER && ss.water < -1e8) {
                            lo = i * 5 * DEG;
                            break;
                        }
                    }
                    SurfaceSite st; st.initFrame(&sy, bi, la, lo, 1000.0);
                    const double day = std::max(60.0, std::fabs(sy.bodies[bi].rotPeriod));
                    double tt = -1;
                    for (double t2 = 1000.0; t2 < 1000.0 + 3 * day && tt < 0; t2 += day / 720.0) {
                        const double a = st.sun(t2).altitude, a2 = st.sun(t2 + day / 720.0).altitude;
                        if (when(a / DEG, a2 > a)) tt = t2;
                    }
                    if (tt < 0) return;
                    SurfaceView sv; sv.init(&sy, bi, la, lo, tt);
                    if (strip) for (int k = 0; k < TRAIT_SLOTS; k++) if (sv.site.gen.traits[k] == t) sv.site.gen.traits[k] = TR_NONE;
                    Input in; sv.update(0.016, in, tt, false);
                    read(sv);
                };
                envAt(TR_FOG_SEAS, 25, [](double a, bool rising) { return rising && a > 2 && a < 4; }, [&](SurfaceView& sv) { fogDawn = sv.env.fogSea; }, false);
                envAt(TR_FOG_SEAS, 25, [](double a, bool) { return a > 50; }, [&](SurfaceView& sv) { fogNoon = sv.env.fogSea; }, false);
                envAt(TR_GREAT_AURORAE, 25, [](double a, bool) { return a < -25; }, [&](SurfaceView& sv) { aur = sv.env.aurora; }, false);
                envAt(TR_GREAT_AURORAE, 25, [](double a, bool) { return a < -25; }, [&](SurfaceView& sv) { aurWithout = sv.env.aurora; }, true);
                check("qualities: the sky's by the hour, a fog sea at dawn and none at noon, a great aurorae's night at 25 degrees (Q-02)",
                        fogDawn > 0.5 && fogNoon == 0 && aur > 0.05 && aurWithout == 0,
                        fmt("the fog %.2f at dawn, %.2f at noon; the aurora %.2f at 25 degrees, %.2f without the trait", fogDawn, fogNoon, aur, aurWithout));
            }
            check("qualities: the ground's, a stratum's accent, the gypsum's dunes, a petrified forest's trunks, a spring's pool and its terraces, a strewn field heard (Q-02)",
                  strata > 5 && gyp > 5 && petr > 5 && springPool > 0 && springSinter > 0 && !heard.empty(),
                  fmt("strata %d, gypsum %d, petrified %d, a spring's pool %d and sinter %d, strewn fields heard %zu", strata, gyp, petr, springPool, springSinter, heard.size()));
        }
        // the sounds of the ground. Each trait carried on a world it fits (its kind's process; an air for the dunes and the ice; the singing
        // dunes' world has slip faces of sand, the singing ice's ice); each sound through the synth within its levels (none at the synth's clamp, none
        // silent), the dunes at their note or its octave, the ice oftener at dusk than at noon, a quake through the boots without the air's rattle and
        // roar; each cue where its world has it and nowhere else (a slip face walked down hums, standing on it or the same walk without the trait is
        // silent; the ice sings round the explorer, not without the trait; a quake within fifty seconds of the landing, none without the trait)
        {
            std::string why3; bool fit3 = true;
            for (int t = TR_SINGING_DUNES; t <= TR_QUAKES; t++) {
                if (first[t].bi < 0) { fit3 = false; why3 += fmt("%s not carried; ", TRAITS[t].name); continue; }
                StarSystem sys; sys.generate(first[t].s);
                const Body& b = sys.bodies[first[t].bi]; const BodyGen g = BodyGen::make(b);
                bool ok = traitEligible(b.type, t) && (t == TR_QUAKES || KINDS[b.type].pressureLo + KINDS[b.type].pressureSpan > 0.001);
                int seen = 0;
                if (t != TR_QUAKES) {
                    DrainageOff off; Rng r(b.seed ^ 0x503ULL);
                    for (int k = 0; k < 40000 && !seen; k++) {   // a felisian world's dune fields are a few in ten thousand of its ground
                        const SurfaceSample ss = sampleSurface(g, StarSystem::bodyFromLatLon(std::asin(r.range(-1, 1)), r.range(-PI, PI)), 4);
                        if (t == TR_SINGING_DUNES ? ss.slipFace > 0.5 && (ss.material == MAT_SAND || ss.material == MAT_DUST) : ss.material == MAT_ICE) seen++;
                    }
                    ok = ok && seen > 0;
                }
                if (!ok) fit3 = false;
                why3 += fmt("%s on %s (%s)%s; ", TRAITS[t].name, b.name.c_str(), KINDS[b.type].shortName, ok ? "" : " DOES NOT FIT");
            }
            check("sounds: each carried, on a world it fits, the dunes' slip faces and the ice there (Q-03)", fit3, why3);
        }
        {
            std::string lv; bool lvOk = true; int evDusk = 0, evNoon = 0; double zcAir = 0, zcAirless = 0;
            for (const GroundSound& gs : groundSounds()) {
                std::vector<float> x; int ev = 0;
                const RenderStats r = renderGroundSound(gs, nullptr, &ev, &x);
                const std::string nm = gs.name;
                double lo = 0.02, hi = 0.12;
                if (nm == "dunes_mars") { lo = 0.003; hi = 0.03; }
                if (nm == "ice_dusk") { lo = 0.015; hi = 0.08; evDusk = ev; }
                if (nm == "ice_noon") { lo = 0.005; hi = 0.04; evNoon = ev; }
                bool ok = r.nan == 0 && r.peak < 0.75 && r.rms >= lo && r.rms <= hi;
                std::string extra;
                if (nm.rfind("dunes", 0) == 0) {
                    AudioState a; gs.set(a, 2.0);
                    const double f = spectralPeak(x, 1.5, 20, 400);
                    ok = ok && (std::fabs(f - a.duneHz) < 0.04 * a.duneHz + 1 || std::fabs(f - 2 * a.duneHz) < 0.04 * a.duneHz + 1);
                    extra = fmt(" loudest at %.0f Hz (its note %.0f)", f, a.duneHz);
                }
                if (nm == "quake_air" || nm == "quake_airless") {
                    int zc = 0; const int i0 = 22050 / 2, i1 = 22050 * 5;
                    for (int i = i0 + 1; i < i1 && i < (int)x.size(); i++) if ((x[i - 1] < 0) != (x[i] < 0)) zc++;
                    (nm == "quake_air" ? zcAir : zcAirless) = zc / 2.0 / 4.5;
                }
                if (!ok) lvOk = false;
                lv += fmt("%s rms %.3f peak %.2f%s%s; ", gs.name, r.rms, r.peak, extra.c_str(), ok ? "" : " OUT");
            }
            lvOk = lvOk && evDusk > evNoon && evDusk >= 10 && zcAirless < 0.7 * zcAir;
            check("sounds: the ground's within their levels, the dunes at their note, the ice oftener at dusk, a quake through the boots muffled (Q-03)", lvOk,
                  lv + fmt("the ice's sounds %d at dusk, %d at noon; a quake's pitch %.0f Hz with an air, %.0f through the boots", evDusk, evNoon, zcAir, zcAirless));
        }
        {
            std::string cw; bool cuesOk = true;
            StarNeighborhood nb;
            auto without = [](SurfaceView& sv, int t) { for (int& tr : sv.site.gen.traits) if (tr == t) tr = TR_NONE; };
            {   // the dunes: the scene's slip face, walked down, stood on, walked down without the trait
                StarSystem sys; SurfaceView sv; double t = 0;
                if (setupSceneForType(-1, 10, 35 * DEG, 0.0, -0.3, sv, sys, nb, t, SCENE_SINGING_DUNES)) {
                    const double yaw = sv.player.yaw;
                    auto onFace = [&]() { sv.player.x = 0; sv.player.z = 0; sv.player.vx = sv.player.vz = 0; sv.player.y = sv.site.surfaceHeight(0, 0); sv.player.yaw = yaw; sv.duneSong = 0; };
                    sv.relocateCapsule(-std::sin(yaw) * 14, -std::cos(yaw) * 14);
                    onFace(); SceneSound walk; sceneSoundRun(sv, t, 2.5, 2.5, walk);
                    onFace(); SceneSound stand; sceneSoundRun(sv, t, 2.5, 0.0, stand);
                    without(sv, TR_SINGING_DUNES); onFace(); SceneSound none; sceneSoundRun(sv, t, 2.5, 2.5, none);
                    const bool ok = walk.dunes > 0.5 && stand.dunes == 0 && none.dunes == 0;
                    if (!ok) cuesOk = false;
                    cw += fmt("the dunes of %s: the hum %.2f walked down, %.2f stood on, %.2f without the trait; ", sys.bodies[sv.site.body].name.c_str(), walk.dunes, stand.dunes, none.dunes);
                } else { cuesOk = false; cw += "no singing dunes found; "; }
            }
            {   // the ice: the scene's, three seconds stood on, the same without the trait
                StarSystem sys; SurfaceView sv; double t = 0;
                if (setupSceneForType(-1, 30, 4 * DEG, 0.0, 0.0, sv, sys, nb, t, SCENE_SINGING_ICE)) {
                    SceneSound on; sceneSoundRun(sv, t, 3.0, 0.0, on);
                    without(sv, TR_SINGING_ICE); sv.iceSong = 0; SceneSound none; sceneSoundRun(sv, t, 3.0, 0.0, none);
                    const bool ok = on.ice > 0.2 && none.ice == 0;
                    if (!ok) cuesOk = false;
                    cw += fmt("the ice of %s: %.2f, %.2f without the trait; ", sys.bodies[sv.site.body].name.c_str(), on.ice, none.ice);
                } else { cuesOk = false; cw += "no singing ice found; "; }
            }
            {   // the quakes: the scene's world, fifty seconds from the landing, and again without the trait
                StarSystem sys; SurfaceView sv; double t = 0;
                if (setupSceneForType(-1, 10, 30 * DEG, 0.0, 0.0, sv, sys, nb, t, SCENE_QUAKES)) {
                    Input in; double top = 0, topWithout = 0; bool cued = false;
                    for (int k = 0; k < 500; k++) { t += 0.1; sv.update(0.1, in, t, false); top = std::max(top, sv.quake); if (sv.quakeCue > 0) cued = true; }
                    without(sv, TR_QUAKES); sv.init(&sys, sv.site.body, sv.site.lat0, sv.site.lon0, t); without(sv, TR_QUAKES);
                    for (int k = 0; k < 500; k++) { t += 0.1; sv.update(0.1, in, t, false); topWithout = std::max(topWithout, sv.quake); }
                    const bool ok = top > 0.5 && cued && topWithout == 0;
                    if (!ok) cuesOk = false;
                    cw += fmt("the quakes of %s: %.2f within fifty seconds%s, %.2f without the trait", sys.bodies[sv.site.body].name.c_str(), top, cued ? " (its sound cued)" : " (NOT CUED)",
                            topWithout);
                } else { cuesOk = false; cw += "no quaking world found"; }
            }
            check("sounds: each cue where its world has it and nowhere else (Q-03)", cuesOk, cw);
        }
    }
}
