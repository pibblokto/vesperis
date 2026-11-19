// Z-01: the living small: `life list [n]`, `life survey [systems]`, `life kin [ly]` (life/biota.h, life/lineages.h); Z-02's `life plates`,
// `strips` and `distinct` are modes_life_look.cpp's; Z-03's `life search` and `life kit`; Z-07's `life auto`
#include "harness/harness.h"
#include "life/biota.h"
#include "galaxy/drifters.h"
#include "galaxy/planet_features.h"
#include "harness/scenes_table.h"
#include "harness/test_game.h"
#include <map>

namespace {
std::string bitsText(uint32_t v, const char* const* names, int n) {
    std::string s;
    for (int i = 0; i < n; i++) if (v & (1u << i)) s += std::string(s.empty() ? "" : " ") + names[i];
    return s.empty() ? "-" : s;
}
std::string genesisText(const Genesis& g) {
    if (!g.id) return "no genesis";
    std::string where = g.origin == GO_CELL ? fmt("cell %lld %lld %lld #%d", (long long)g.cx, (long long)g.cy, (long long)g.cz, g.cellIndex) : GENESIS_ORIGIN_NAMES[g.origin];
    return fmt("genesis %016llx (%s, %s, %s, %d letters, %.1f Gyr)", (unsigned long long)g.id, where.c_str(), CHEMISTRY_NAMES[g.chemistry],
               g.mirrored ? "mirrored" : "ours", g.letters, g.ageGyr);
}
const char* SALT_TEXT[3] = {"fresh", "sea", "brine"};
const char* OXYGEN_TEXT[3] = {"anaerobe", "tolerant", "aerobe"};
std::string microbeLine(const Microbe& s) {
    std::string l = fmt("M%-2d %-9s %-15s in %-6s", s.index, STATE_NAMES[s.state], MICROBE_PLAN_NAMES[s.plan], NICHE_NAMES[s.niche]);
    const uint32_t also = s.niches & ~nicheBit(s.niche);
    if (also) l += " +" + bitsText(also, NICHE_NAMES, NI_COUNT);
    if (s.state == SS_FOSSIL) return l + fmt("  %.2g years old", s.ageYears);
    if (s.state != SS_PROTO) {
        l += fmt("  %s%s%s", WAY_NAMES[s.way], s.substrate ? fmt(" (%s)", SUBSTRATE_NAMES[s.substrate]).c_str() : "",
                 s.pigment ? fmt(", %s", PIGMENT_NAMES[s.pigment]).c_str() : "");
        l += fmt(", %s; %.0f..%.0f..%.0f C, %s-%s, pH %.1f-%.1f, %.0f Gy", OXYGEN_TEXT[s.oxygen], s.tMin, s.tOpt, s.tMax, SALT_TEXT[s.saltLo],
                 SALT_TEXT[s.saltHi], s.phLo, s.phHi, s.doseGy);
        if (s.sporeYears > 0) l += fmt(", spores %.2g y", s.sporeYears);
    }
    l += fmt("; %s; %s; %s", bitsText(s.cycle, CYCLE_NAMES, LC_BITS).c_str(), DIVISION_NAMES[s.division], MOTION_NAMES[s.motion]);
    if (s.taxis) l += " to " + bitsText(s.taxis, TAXIS_NAMES, TX_BITS);
    if (s.glows) l += ", glows";
    l += fmt("; share %.2f", s.abundance);
    if (s.fromBody >= 0) l += fmt("; carried from body %d's M%d %.2g years ago", s.fromBody, s.fromIndex, s.ageYears);
    return l;
}
std::string stageText(const Biota& B) {
    return std::string(LIFE_STAGE_NAMES[B.stage]) + (B.second != LS_STERILE ? std::string(" + ") + LIFE_STAGE_NAMES[B.second] : "");
}
void printWorld(const StarSystem& sys, int i, const Biota& B, bool species) {
    printf("%-34s %-22s %-9s %s\n", sys.bodyLabel(i).c_str(), stageText(B).c_str(), CHEMISTRY_NAMES[B.chemistry], genesisText(B.genesis).c_str());
    printf("    habitats %s; mean %.0f C%s; %d living, %d dormant, %d fossil, %d protocell\n", bitsText(B.habitats, NICHE_NAMES, NI_COUNT).c_str(), B.meanC,
           B.dormantYears > 0 ? fmt("; asleep %.2g years", B.dormantYears).c_str() : "", B.count(SS_LIVING), B.count(SS_DORMANT), B.count(SS_FOSSIL),
           B.count(SS_PROTO));
    if (species) for (const Microbe& s : B.species) printf("      %s\n", microbeLine(s).c_str());
}
}   // namespace

// the stars within r sectors of home (a cube), nearest first
std::vector<Star> starsNearHome(int r) {
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, false);
    std::vector<Star> out;
    for (int64_t x = HOME_SX - r; x <= HOME_SX + r; x++)
        for (int64_t y = HOME_SY - r; y <= HOME_SY + r; y++)
            for (int64_t z = HOME_SZ - r; z <= HOME_SZ + r; z++) { Star s; if (starInSector(x, y, z, s, true)) out.push_back(s); }
    std::sort(out.begin(), out.end(), [&](const Star& a, const Star& b) { return length(a.pos - home.pos) < length(b.pos - home.pos); });
    return out;
}

namespace {

// `life list [n]`: the biota of the n landable worlds nearest home (its own system first), each world's species
int lifeList(int n) {
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, false);
    int shown = 0;
    for (const Star& s : starsNearHome(12)) {
        if (shown >= n) break;
        StarSystem sys; sys.generate(s);
        auto L = lifeOf(sys);
        printf("\n%s, %s, %.1f ly from home, sector %lld %lld %lld\n", s.name.c_str(), STAR_CLASSES[s.cls].name, length(s.pos - home.pos) / SECTOR_KM,
               (long long)s.sx, (long long)s.sy, (long long)s.sz);
        for (int i = 0; i < (int)sys.bodies.size() && shown < n; i++) {
            if (!KINDS[sys.bodies[i].type].landable) continue;
            printWorld(sys, i, L->bodies[i], true);
            shown++;
        }
    }
    printf("\nthe ship's own (home's genesis):\n");
    for (int k = 0; k < SHIP_SPECIES; k++) printf("      %s\n", microbeLine(shipSpecies(k)).c_str());
    return 0;
}

// `life survey [systems]`: the first n systems of the scan region and the forty drifters nearest home, by kind: how many worlds, their stages,
// their species; the living worlds' counts, the dead worlds' spores (how many could wake), the spores carried near home
int lifeSurvey(int systems) {
    const double t0 = nowSec();
    struct Tally { int worlds = 0, stages[LS_COUNT] = {}, fossils = 0, carried = 0, minS = 1 << 30, maxS = 0; long living = 0; };
    std::vector<Tally> T(PT_COUNT);
    long worlds = 0, deadWorlds = 0, deadSpores = 0, wake = 0, carriedWorlds = 0, carriedSpores = 0, livingWorlds = 0;
    int minLiving = 1 << 30, maxLiving = 0, seen = 0;
    auto take = [&](const StarSystem& sys) {
        auto L = lifeOf(sys);
        for (int i = 0; i < (int)sys.bodies.size(); i++) {
            const Body& b = sys.bodies[i];
            if (!KINDS[b.type].landable) continue;
            const Biota& B = L->bodies[i];
            Tally& t = T[b.type];
            t.worlds++; worlds++;
            t.stages[B.stage]++;
            if (B.second == LS_FOSSIL) t.fossils++;
            const int living = B.count(SS_LIVING);
            if (livingStage(B.stage)) { t.minS = std::min(t.minS, living); t.maxS = std::max(t.maxS, living); t.living += living; livingWorlds++;
                                        minLiving = std::min(minLiving, living); maxLiving = std::max(maxLiving, living); }
            int carried = 0;
            for (const Microbe& s : B.species) if (s.fromBody >= 0) carried++;
            if (carried) { t.carried++; carriedWorlds++; carriedSpores += carried; }
            if (B.stage == LS_DORMANT && !carried) {
                deadWorlds++;
                for (const Microbe& s : B.species) if (s.state == SS_DORMANT) { deadSpores++; if (s.sporeYears > B.dormantYears) wake++; }
            }
        }
    };
    for (auto& [x, z, s, sys] : scanSystems(150, 340, 20, 160, [&] { return seen < systems; })) { take(sys); seen++; }
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
    std::vector<Star> near; driftersNear(home.pos, 3000, near);
    int drifters = 0;
    for (const Star& d0 : near) {
        if (drifters >= 40) break;
        Star d; if (!drifterInSector(d0.sx, d0.sy, d0.sz, d, true)) continue;
        StarSystem sys; sys.generate(d); take(sys); drifters++;
    }
    printf("life survey: %d systems of the scan region and %d drifters, %ld landable worlds (%.1f s)\n", seen, drifters, worlds, nowSec() - t0);
    printf("%-18s %6s  %s\n", "kind", "worlds", "STERILE PREBIOTIC SIMPLE COMPLEX COLONIAL DORMANT  +fossils  carried  living species");
    int missing = 0, bare = 0;
    std::string bareKinds, missingKinds;
    for (int t = 0; t < PT_COUNT; t++) {
        if (!KINDS[t].landable) continue;
        const Tally& a = T[t];
        if (a.worlds == 0) { missing++; missingKinds += std::string(missingKinds.empty() ? "" : ", ") + KINDS[t].name; continue; }
        if (!BIOTA[t].furnace && a.stages[LS_STERILE] == a.worlds) { bare++; bareKinds += std::string(bareKinds.empty() ? "" : ", ") + KINDS[t].name; }
        std::string sp = a.maxS > 0 ? fmt("%d-%d (mean %.1f)", a.minS, a.maxS, (double)a.living / std::max(1, a.stages[LS_SIMPLE] + a.stages[LS_COMPLEX] + a.stages[LS_COLONIAL])) : "-";
        printf("%-18s %6d  %7d %9d %6d %7d %8d %7d  %8d  %7d  %s%s\n", KINDS[t].name, a.worlds, a.stages[0], a.stages[1], a.stages[2], a.stages[3], a.stages[4],
               a.stages[5], a.fossils, a.carried, sp.c_str(), BIOTA[t].furnace ? "  (a furnace)" : "");
    }
    printf("something on every landable kind but the furnaces: %s%s\n", bare == 0 ? "yes" : fmt("no, nothing on %s", bareKinds.c_str()).c_str(),
           missing ? fmt(" (not in the survey: %s)", missingKinds.c_str()).c_str() : "");
    printf("a living world's species: %d-%d over %ld living worlds\n", minLiving, maxLiving, livingWorlds);
    printf("the dead worlds' own spores: %ld on %ld worlds, %ld of them able to wake (lasting longer than their world has slept)\n", deadSpores, deadWorlds, wake);
    printf("spores carried to airless and icy neighbours: %ld on %ld worlds\n", carriedSpores, carriedWorlds);
    // near home: the home system's worlds and the spores carried there
    StarSystem hs; hs.generate(home);
    auto H = lifeOf(hs);
    printf("near home (%s):\n", home.name.c_str());
    for (int i = 0; i < (int)hs.bodies.size(); i++) {
        if (!KINDS[hs.bodies[i].type].landable) continue;
        const Biota& B = H->bodies[i];
        std::string from;
        for (const Microbe& s : B.species) if (s.fromBody >= 0) from += fmt(" M%d<-%s M%d", s.index, hs.bodies[s.fromBody].name.c_str(), s.fromIndex);
        printf("  %-34s %-22s%s\n", hs.bodyLabel(i).c_str(), stageText(B).c_str(), from.empty() ? "" : (" spores:" + from).c_str());
    }
    return 0;
}

// `life kin [ly]`: the systems of the plane within ly of home whose worlds carry life, by their water genesis: their own or their kin cell's;
// the cells' kin systems, home's cell first; the drifters near home with the star their life came from
int lifeKin(double ly) {
    const double t0 = nowSec();
    Star home; starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
    struct Kin { Star s; int worlds; };
    std::map<uint64_t, std::vector<Kin>> byGenesis;   // the cells' geneses, their systems
    std::map<uint64_t, Genesis> genesis;
    long stars = 0, lifeSystems = 0, own = 0, cell = 0, other = 0, mirrored = 0, letters[9] = {};
    const int64_t R = (int64_t)std::ceil(ly);
    for (int64_t x = HOME_SX - R; x <= HOME_SX + R; x++)
        for (int64_t z = HOME_SZ - R; z <= HOME_SZ + R; z++) {
            if (double(x - HOME_SX) * (x - HOME_SX) + double(z - HOME_SZ) * (z - HOME_SZ) > ly * ly) continue;
            Star s;
            if (!starInSector(x, 0, z, s, true)) continue;
            stars++;
            StarSystem sys; sys.generate(s);
            auto L = lifeOf(sys);
            int water = 0; bool anyLife = false; Genesis g;
            for (int i = 0; i < (int)sys.bodies.size(); i++) {
                const Biota& B = L->bodies[i];
                if (!B.genesis.id || sys.bodies[i].wanderer) continue;
                bool carried = false;
                for (const Microbe& m : B.species) carried = carried || m.fromBody >= 0;
                if (carried) continue;
                anyLife = true;
                if (B.genesis.chemistry == CH_WATER) { water++; g = B.genesis; } else other++;
            }
            if (!anyLife) continue;
            lifeSystems++;
            if (!water) continue;
            genesis[g.id] = g;
            if (g.mirrored) mirrored++;
            letters[std::min(8, g.letters)]++;
            if (g.origin == GO_CELL) { cell++; byGenesis[g.id].push_back({s, water}); } else own++;
        }
    printf("life kin: %ld stars of the plane within %.0f ly of home, %ld systems with life of their own (%.1f s)\n", stars, ly, lifeSystems, nowSec() - t0);
    printf("  their water life: %ld its own genesis, %ld its kin cell's (%.0f%%); %ld geneses of another chemistry\n", own, cell,
           100.0 * cell / std::max(1L, own + cell), other);
    printf("  the water geneses: %ld mirrored (%.0f%%); %ld of 4 letters, %ld of 6, %ld of 8\n", mirrored, 100.0 * mirrored / std::max(1L, own + cell), letters[4],
           letters[6], letters[8]);
    const Genesis hg = homeGenesis();
    printf("home's: %s%s\n", genesisText(hg).c_str(), hg.id == HOME_GENESIS_ID ? " (pinned)" : " (NOT the pinned genesis)");
    std::vector<std::pair<uint64_t, std::vector<Kin>>> cells(byGenesis.begin(), byGenesis.end());
    std::sort(cells.begin(), cells.end(), [&](const auto& a, const auto& b) {
        if ((a.first == hg.id) != (b.first == hg.id)) return a.first == hg.id;
        const Genesis& ga = genesis[a.first]; const Genesis& gb = genesis[b.first];
        if (ga.cx != gb.cx) return ga.cx < gb.cx;
        if (ga.cz != gb.cz) return ga.cz < gb.cz;
        return ga.cellIndex < gb.cellIndex;
    });
    printf("the kin systems by cell (%zu cells' geneses):\n", cells.size());
    for (auto& [id, list] : cells) {
        const Genesis& g = genesis[id];
        std::sort(list.begin(), list.end(), [&](const Kin& a, const Kin& b) { return length(a.s.pos - home.pos) < length(b.s.pos - home.pos); });
        std::string names;
        for (size_t i = 0; i < list.size() && i < 6; i++) names += fmt("%s%s (%.0f ly)", i ? ", " : "", list[i].s.name.c_str(), length(list[i].s.pos - home.pos) / SECTOR_KM);
        if (list.size() > 6) names += fmt(", %zu more", list.size() - 6);
        printf("  cell %lld %lld %lld #%d %016llx%s: %zu systems: %s\n", (long long)g.cx, (long long)g.cy, (long long)g.cz, g.cellIndex, (unsigned long long)id,
               id == hg.id ? " (home's)" : "", list.size(), names.c_str());
    }
    // the drifters near home: their life is their old star's
    std::vector<Star> near; driftersNear(home.pos, 1500, near);
    printf("the drifters within 1,500 ly of home (the twelve nearest):\n");
    int shown = 0;
    for (const Star& d0 : near) {
        if (shown >= 12) break;
        Star d; if (!drifterInSector(d0.sx, d0.sy, d0.sz, d, true)) continue;
        StarSystem sys; sys.generate(d);
        auto L = lifeOf(sys);
        const DrifterPast past = drifterPast(d);
        int alive = 0, asleep = 0;
        for (const Biota& B : L->bodies) { alive += B.count(SS_LIVING); asleep += B.count(SS_DORMANT); }
        const Biota& B = L->bodies[0];
        printf("  %-14s %5.0f ly %-16s %-20s left %s %.0f years ago (%.0f ly away now; %s); %d living, %d dormant in its system%s\n", d.name.c_str(),
               length(d.pos - home.pos) / SECTOR_KM, KINDS[sys.bodies[0].type].name, stageText(B).c_str(), past.found ? past.star.name.c_str() : "no star",
               past.years, past.travelLy, past.found ? (systemGenesis(past.star, CH_WATER).origin == GO_CELL ? "a kin cell's life" : "its own life") : "-", alive,
               asleep, B.genesis.id ? fmt("; %016llx", (unsigned long long)B.genesis.id).c_str() : "");
        shown++;
    }
    return 0;
}
}   // namespace

// Z-03: `life search <scene|all>`: round a test scene's site, 97 places on a spiral out to 120 m (its feet first), each microhabitat read
// there (its name, niche, mean temperature, salt, shade) once with the places it was read at, and what a vial of its first place holds (each
// species: its index, plan, state, the count's decade, whether it grows there) and its activity; then every place read again from a landing
// 300 m east (the same square metres in another frame), the same twice. The scenes: `all` the search's (the signs, the liquids, the dry, the
// furnace)
static const char* const SEARCH_SCENES[] = {"archean_flats", "uv_mats", "felisian_lake", "felisian_shore", "felisian_river", "felisian_forest", "superhabitable_shelf",
                                            "chemotroph_vents", "spring_bands", "geyser_basin", "salt_basin", "tundra_ice", "red_snow", "lichen_hollow",
                                            "cryovolcanic_plume", "tiger_stripes", "civilisation_desert", "desert_erg", "acidic_shore", "hydrocarbon_shore",
                                            "ammonia_shore", "snowball_belt", "europan_crack", "rust_valley", "scorched_felisian_seabed", "cratered_noon",
                                            "magma_shore"};
static int lifeSearchScene(const std::string& name, int& placesAll, int& sameAll) {
    const CmpScene* sc = findScene(name);
    if (!sc) { printf("%s: no such scene\n", name.c_str()); return 1; }
    StarSystem sys; SurfaceView sv; StarNeighborhood nb; double t = 0;
    if (!setupSceneForType(sc->type, sc->latDeg, sc->alt, sc->yawOff, sc->pitch, sv, sys, nb, t, sc->wantMat)) { printf("%s: no body\n", name.c_str()); return 1; }
    const Biota& B = sv.biota();
    const Body& b = sys.bodies[sv.site.body];
    printf("%s: %s (%s), %s life, %s, %d species\n", name.c_str(), b.name.c_str(), KINDS[b.type].name, LIFE_STAGE_NAMES[B.stage], CHEMISTRY_NAMES[B.chemistry], (int)B.species.size());
    if (std::getenv("VESPERIS_SEARCH_DIFF")) for (const Microbe& m : B.species) printf("    %s\n", microbeLine(m).c_str());
    constexpr int N = 97;
    std::vector<Vec3> units; std::vector<SampleRead> reads;
    std::vector<std::string> order; std::map<std::string, int> count;
    for (int k = 0; k < N; k++) {
        const double r = 120.0 * std::sqrt(k / (double)(N - 1)), a = k * 2.399963;
        const double x = sv.player.x + r * std::sin(a), z = sv.player.z + r * std::cos(a);
        units.push_back(sv.site.unitAt(x, z));
        reads.push_back(sv.readSample(x, z, false));
        const SampleRead& rd = reads.back();
        const std::string key = rd.ok ? rd.name : std::string("REFUSED: ") + (rd.refusal ? rd.refusal : "?");
        if (!count.count(key)) { order.push_back(key); count[key] = 0; }
        count[key]++;
        if (count[key] > 1 || !rd.ok) continue;
        const Sample S = sampleOf(B, rd.hab);
        std::string held;
        for (const Held& h : S.held) {
            const Microbe& m = B.species[h.species];
            held += fmt(" M%d %s %s 1e%.0f%s;", h.species, MICROBE_PLAN_NAMES[m.plan], STATE_NAMES[h.state], std::log10(std::max(1.0, h.cells)), h.active ? " growing" : "");
        }
        printf("  %s (%s, %.1f C, %s%s%s): %s%s, ACTIVITY %s\n", rd.name.c_str(), NICHE_NAMES[rd.hab.niche], rd.hab.meanC, SALT_TEXT[clampi(rd.hab.salt, 0, 2)],
               rd.hab.shade ? ", shade" : "", rd.hab.oldRock && rd.hab.niche == NI_ROCK ? ", old rock" : "", S.held.empty() ? (S.organics ? "cooked organics" : "nothing alive") : fmt("%d held:", (int)S.held.size()).c_str(),
               held.c_str(), VIAL_ACTIVITY_NAMES[S.activity]);
    }
    std::string places;
    for (const std::string& k : order) places += fmt("%s%s x%d", places.empty() ? "" : ", ", k.c_str(), count[k]);
    printf("  places: %s\n", places.c_str());
    // the same square metres from a landing 300 m east
    double la, lo; StarSystem::latLonFromBody(sv.site.unitAt(sv.player.x + 300, sv.player.z), la, lo);
    SurfaceView sv2; sv2.init(&sys, sv.site.body, la, lo, t);
    int same = 0;
    for (int k = 0; k < N; k++) {
        double x2, z2; sv2.site.localAt(units[k], x2, z2);
        const SampleRead r2 = sv2.readSample(x2, z2, false);
        const SampleRead& r1 = reads[k];
        bool ok = r1.ok == r2.ok && r1.name == r2.name && r1.hab.niche == r2.hab.niche && std::fabs(r1.hab.meanC - r2.hab.meanC) < 0.06 && r1.hab.salt == r2.hab.salt &&
                  r1.hab.shade == r2.hab.shade && r1.hab.oldRock == r2.hab.oldRock && r1.hab.place == r2.hab.place;
        if (ok && r1.ok) {
            const Sample a = sampleOf(B, r1.hab), c = sampleOf(sv2.biota(), r2.hab);
            ok = a.activity == c.activity && a.held.size() == c.held.size();
            for (size_t i = 0; ok && i < a.held.size(); i++) ok = a.held[i].species == c.held[i].species && a.held[i].cells == c.held[i].cells && a.held[i].active == c.held[i].active;
        }
        same += ok;
        if (!ok && std::getenv("VESPERIS_SEARCH_DIFF"))
            printf("    differs at %d: %s / %s, niche %d/%d, mean %.6f/%.6f, salt %d/%d, shade %d/%d, place %d\n", k, r1.name.c_str(), r2.name.c_str(), r1.hab.niche, r2.hab.niche,
                   r1.hab.meanC, r2.hab.meanC, r1.hab.salt, r2.hab.salt, (int)r1.hab.shade, (int)r2.hab.shade, (int)(r1.hab.place == r2.hab.place));
    }
    printf("  the same twice: %d of %d places, read again from a landing 300 m east\n", same, N);
    placesAll += N; sameAll += same;
    return same == N ? 0 : 1;
}
static int lifeSearch(const std::string& which) {
    std::vector<std::string> names;
    if (which == "all") for (const char* n : SEARCH_SCENES) names.push_back(n); else names.push_back(which);
    int bad = 0, places = 0, same = 0;
    for (const std::string& n : names) bad += lifeSearchScene(n, places, same);
    printf("life search: %d scenes, %d places, %d read the same twice%s\n", (int)names.size(), places, same, bad ? fmt(", %d scenes not", bad).c_str() : "");
    return bad ? 1 : 0;
}

// Z-03: `life kit`: the kit played through a landing on the scan's first venusian world whose clouds hold life (else an acidic one's): the scoop's line in
// the descent; on the ground a vial of the ground at the crosshair (its line), the eight filled and the ninth refused; the climb aboard and the
// cold store's line. Frames: shots/tests/life_kit_<scoop|line|full|store>.png; the exit code what was not as it should be
static int lifeKit() {
    TestGame game;
    game.savePrefix = "shots/tests/life_kit_save"; game.settingsPath = "shots/tests/life_kit_settings.txt"; game.guidePath = "shots/tests/life_kit_guide.txt";
    game.keysPath = "shots/tests/test_keys.txt";
    remove(game.guidePath.c_str()); remove(game.settingsPath.c_str());
    game.newGame(); game.setState(GameState::SPACE);
    Input in;
    StarSystem sys; int bi = -1;
    for (int type : {PT_VENUSIAN, PT_ACIDIC})   // a venusian's first: its clouds' temperate height over its burning ground
        for (auto& [x, z, s, sy] : scanSystems(150, 340, 20, 160, [&] { return bi < 0; })) {
            for (int i = 0; i < (int)sy.bodies.size() && bi < 0; i++) {
                if (sy.bodies[i].type != type) continue;
                for (const Microbe& m : biotaOf(sy, i).species) if (m.state == SS_LIVING && (m.niches & nicheBit(NI_CLOUD))) { sys = sy; bi = i; break; }
            }
        }
    if (bi < 0) { printf("life kit: no world of cloud life in the scan region\n"); return 1; }
    printf("life kit: %s (%s)\n", sys.bodies[bi].name.c_str(), KINDS[sys.bodies[bi].type].name);
    auto shot = [&](const char* name) {
        writePNG(fmt("shots/tests/life_kit_%s.png", name).c_str(), game.output(), FBW, FBH);
        printf("  %-6s state %d, %d vials, %d in the cold store: '%s'\n", name, (int)game.state, (int)game.testKit().size(), (int)game.testColdStore().size(), game.testStatus().c_str());
    };
    game.testParkAt(sys.star, bi); game.testLandSite();
    pressKey(game, in, KEY_ENTER);
    for (int n = 0; n < 30 * 7 && game.testStatus().rfind("THE SCOOP", 0) != 0; n++) { game.frame(in, 1.0 / 30); in.newFrame(); }
    runFrames(game, in, 0.3); shot("scoop");
    const bool scooped = game.testKit().size() == 1 && game.testKit()[0].hab.niche == NI_CLOUD;
    runFrames(game, in, 8.0);
    game.testSetPitch(-0.8); runFrames(game, in, 0.3);
    pressKey(game, in, KEY_O); runFrames(game, in, 0.3); shot("line");
    for (int k = 0; k < 6; k++) { pressKey(game, in, KEY_O); runFrames(game, in, 0.1); }
    pressKey(game, in, KEY_O); runFrames(game, in, 0.3); shot("full");
    const bool full = game.testKit().size() == 8 && game.testStatus().find("EIGHT VIALS ARE FULL") != std::string::npos;
    pressKey(game, in, KEY_K); runFrames(game, in, 0.3);
    pressKey(game, in, KEY_Q); runFrames(game, in, 9.5); shot("store");
    const bool stored = game.testColdStore().size() == 8 && game.testKit().empty();
    printf("life kit: the scoop %s, the kit full at eight %s, the cold store's eight %s\n", scooped ? "yes" : "NO", full ? "yes" : "NO", stored ? "yes" : "NO");
    return scooped && full && stored ? 0 : 1;
}

// Z-07: a site on land 200 m inland of a sea's coast (the planet function's 16 m samples, the drainage's rivers aside): on a 3-degree grid a wet
// point with dry ground 3 degrees east of it, the coast between them found by halves; false with none
bool coastSite(const BodyGen& g, double radiusKm, double& latDeg, double& lonDeg) {
    const bool drained = drainageEnabled();
    setDrainageEnabled(false);
    auto wet = [&](double la, double lo) { const SurfaceSample s = sampleSurface(g, StarSystem::bodyFromLatLon(la, lo), 16); return s.water > -1e8 && s.height < s.water - 0.5; };
    bool found = false;
    for (double la0 = -45; la0 <= 45 && !found; la0 += 3)
        for (double lo0 = -180; lo0 < 177 && !found; lo0 += 3) {
            const double la = la0 * DEG, lo = lo0 * DEG, east = (lo0 + 3) * DEG;
            if (!wet(la, lo) || wet(la, east)) continue;
            double w = lo, d = east;
            for (int k = 0; k < 30; k++) { const double m = 0.5 * (w + d); if (wet(la, m)) w = m; else d = m; }
            latDeg = la0; lonDeg = (d + 200.0 / (radiusKm * 1000.0 * std::cos(la))) / DEG;
            found = true;
        }
    setDrainageEnabled(drained);
    return found;
}

// Z-07: `life auto`: auto-explore's samples on its walks. Landed on the scan's first felisian world 200 m inland of a coast (`coastSite`), and on
// its first world of terraced hot springs, 80 m south of a vent's pool; each time auto-explore begun on the ground, its water outing planned (a
// spring's pool first where one is in reach) and run till it has taken a sample (150 s at most): its steps, the vial's line and its niche
// (shots/tests/life_auto_water.png, life_auto_springs.png)
static int lifeAuto() {
    struct Place { const char* what; StarSystem sys; int body = -1; double lat = 0, lon = 0; bool site = false; };
    std::vector<Place> places;
    {   // the flow's world's coast
        Place P; P.what = "water";
        if (firstBodyOfType(PT_FELISIAN, P.sys, P.body) && coastSite(BodyGen::make(P.sys.bodies[P.body]), P.sys.bodies[P.body].radiusKm, P.lat, P.lon)) { P.site = true; places.push_back(P); }
    }
    {   // a terraced spring's pool where its landform put it (`springBands`' finder): the site 80 m south of it
        Place P; P.what = "springs";
        for (auto& [x, z, s, sy] : scanSystems(150, 340, 20, 160, [&] { return P.body < 0; }))
            for (int i = 0; i < (int)sy.bodies.size() && P.body < 0; i++) {
                const Body& b = sy.bodies[i];
                if (!KINDS[b.type].landable || b.type == PT_COMET) continue;
                const BodyGen g = BodyGen::make(b);
                if (!g.hasTrait(TR_HOT_SPRINGS)) continue;
                for (double la = -50; la <= 50 && P.body < 0; la += 1)
                    for (double lo = -180; lo < 180 && P.body < 0; lo += 1) {
                        std::vector<GeyserVent> vents;
                        hotSpringsNear(g, StarSystem::bodyFromLatLon(la * DEG, lo * DEG), vents);
                        for (const GeyserVent& v : vents) {
                            if (sampleSurface(g, v.unit, 4).material != MAT_BRINE) continue;
                            double vla, vlo; StarSystem::latLonFromBody(v.unit, vla, vlo);
                            P.sys = sy; P.body = i; P.site = true;
                            P.lat = (vla - 80.0 / (b.radiusKm * 1000.0)) / DEG; P.lon = vlo / DEG;
                            break;
                        }
                    }
            }
        if (P.body >= 0) places.push_back(P);
    }
    int good = 0;
    for (const Place& P : places) {
        TestGame game;
        game.savePrefix = std::string("shots/tests/life_auto_save_") + P.what; game.settingsPath = "shots/tests/life_auto_settings.txt";
        game.guidePath = std::string("shots/tests/life_auto_guide_") + P.what + ".txt"; game.keysPath = "shots/tests/test_keys.txt";
        remove(game.guidePath.c_str()); remove(game.settingsPath.c_str());
        game.newGame(); game.setState(GameState::SPACE);
        Input in;
        game.testParkAt(P.sys.star, P.body);
        if (P.site) game.testLandCursor(P.lat, P.lon); else game.testLandSite();
        pressKey(game, in, KEY_ENTER);
        for (int n = 0; n < 30 * 40 && game.state != GameState::SURFACE; n++) { game.frame(in, 1.0 / 30); in.newFrame(); }
        runFrames(game, in, 2.0);
        game.autoStart(AP_SURVEYOR, 5);
        game.frame(in, 1.0 / 30); in.newFrame();
        const bool planned = game.testAutoWater();
        std::vector<std::string> steps;
        double secs = 0;
        while (planned && secs < 150 && game.autoOn() && game.testAutoCount(8) == 0) {
            game.frame(in, 1.0 / 30); in.newFrame(); secs += 1.0 / 30;
            const std::string d = game.testAutoDoing();
            if (!d.empty() && (steps.empty() || steps.back() != d)) steps.push_back(d);
        }
        runFrames(game, in, 4.0);   // its look over what it sampled
        writePNG(fmt("shots/tests/life_auto_%s.png", P.what).c_str(), game.output(), FBW, FBH);
        const std::vector<Vial>& kit = game.testKit();
        const bool took = game.testAutoCount(8) == 1 && kit.size() == 1;
        std::string way;
        for (const std::string& s : steps) way += (way.empty() ? "" : ", ") + s;
        printf("life auto, %s: %s body %d (%s)%s; %s after %.0f s (%s): %s%s\n", P.what, P.sys.star.name.c_str(), P.body, KINDS[P.sys.bodies[P.body].type].name,
               std::string(P.what) == "water" ? fmt(", landed at %.3f %.3f, 200 m inland of a coast", P.lat, P.lon).c_str() : ", landed 80 m south of a spring's pool",
               took ? "a sample" : "NO SAMPLE", secs, way.c_str(),
               kit.empty() ? "-" : game.testVialLine(kit.back()).c_str(), kit.empty() ? "" : fmt(" (%s)", NICHE_NAMES[kit.back().hab.niche]).c_str());
        good += took;
    }
    pngFlush();
    printf("life auto: %d of %zu places sampled (2 asked: the water, the springs)\n", good, places.size());
    return good == 2 && places.size() == 2 ? 0 : 1;
}

// R-434: `life sight`: the activity vision played on the ground, landed where the scenes' `sight_shelf` stands (a superhabitable world's shelf:
// its dry sand none at work, its wet edge and the sea strong). Shift+O and a second for its square metres to be read (life_sight_on.png, the
// readings' line); a buggy unfolded and boarded, its nose camera with the vision (life_sight_buggy.png); out of it, the explorer at the nearest
// square metre the vision shows strong and a vial taken there (its line, and the vision's heat at it: life_sight_sample.png); Shift+O again, the
// eye's (life_sight_off.png). The exit code: the vial not strong, or the vision's frames not as they should be
static int lifeSight() {
    const CmpScene* sc = findScene("sight_shelf");
    StarSystem sys; SurfaceView look; StarNeighborhood nb; double t0 = 0;
    if (!sc || !setupSceneForType(sc->type, sc->latDeg, sc->alt, sc->yawOff, sc->pitch, look, sys, nb, t0, sc->wantMat)) { printf("life sight: no site\n"); return 1; }
    double la, lo; look.site.latLonAt(look.player.x, look.player.z, la, lo);
    TestGame game;
    game.savePrefix = "shots/tests/life_sight_save"; game.settingsPath = "shots/tests/life_sight_settings.txt"; game.guidePath = "shots/tests/life_sight_guide.txt";
    game.keysPath = "shots/tests/test_keys.txt";
    remove(game.guidePath.c_str()); remove(game.settingsPath.c_str());
    game.newGame(); game.setState(GameState::SPACE);
    Input in;
    game.testParkAt(sys.star, look.site.body); game.testLandCursor(la / DEG, lo / DEG);
    pressKey(game, in, KEY_ENTER);
    for (int n = 0; n < 30 * 40 && game.state != GameState::SURFACE; n++) { game.frame(in, 1.0 / 30); in.newFrame(); }
    runFrames(game, in, 2.0);
    const SurfaceView& sv = game.testSurface();
    printf("life sight: %s body %d (%s), landed at %.3f %.3f\n", sys.star.name.c_str(), look.site.body, KINDS[sys.bodies[look.site.body].type].name, la / DEG, lo / DEG);
    auto shot = [&](const char* name) {
        writePNG(fmt("shots/tests/life_sight_%s.png", name).c_str(), game.output(), FBW, FBH);
        int read = 0, faint = 0, strong = 0; sv.testSightCounts(read, faint, strong);
        printf("  %-6s %s, vision %d, %d square metres read (%d faint, %d strong): '%s'\n", name, stateName(game.state), game.testVisionMode(), read, faint, strong, game.testStatus().c_str());
    };
    auto shiftO = [&] { in.down[KEY_LEFT_SHIFT] = true; pressKey(game, in, KEY_O); in.down[KEY_LEFT_SHIFT] = false; };
    game.testSurfaceFace(sv.player.yaw, -0.35);
    shiftO(); runFrames(game, in, 1.0); shot("on");
    const bool on = game.testVisionMode() == VISION_ACTIVITY && sv.testSightMissing() == 0;
    pressKey(game, in, KEY_B); runFrames(game, in, 2.5); game.testWalkToBuggy(); pressKey(game, in, KEY_E); runFrames(game, in, 1.0); shot("buggy");
    const bool driven = sv.inBuggy;
    pressKey(game, in, KEY_E); runFrames(game, in, 0.5);
    // the nearest square metre the vision shows strong, within 30 m: the explorer a metre short of its middle, looking at it
    double bx = 0, bz = 0, bd = 1e9;
    const double px = std::floor(sv.player.x), pz = std::floor(sv.player.z);
    for (int dz = -30; dz <= 30; dz++)
        for (int dx = -30; dx <= 30; dx++) {
            const double x = px + dx + 0.5, z = pz + dz + 0.5, d = std::hypot(x - sv.player.x, z - sv.player.z);
            if (d > 2.0 && d < bd && sv.sightHeatAt(x, z) >= 0.62f) { bd = d; bx = x; bz = z; }
        }
    std::string line = "-"; float heat = -1; int activity = -1;
    if (bd < 1e9) {
        const double yaw = std::atan2(bx - sv.player.x, bz - sv.player.z);
        game.testWalkTo(bx - std::sin(yaw) * 1.2, bz - std::cos(yaw) * 1.2);
        game.testSurfaceFace(yaw, -0.95);
        runFrames(game, in, 0.3);
        pressKey(game, in, KEY_O); runFrames(game, in, 0.3);
        if (!game.testKit().empty()) {
            const Vial& v = game.testKit().back();
            line = game.testVialLine(v); activity = v.activity;
            heat = sv.sightHeatAt(game.testRangeX(), game.testRangeZ());
        }
        shot("sample");
    }
    shiftO(); runFrames(game, in, 0.3); shot("off");
    const bool off = game.testVisionMode() == 0;
    pngFlush();
    printf("life sight: the vision on %s, from the buggy %s; the nearest strong square metre %.1f m off, the vial there: %s (the vision's heat %.2f); off again %s\n",
           on ? "and read" : "NOT READ", driven ? "yes" : "NO", bd < 1e9 ? bd : -1.0, line.c_str(), heat, off ? "yes" : "NO");
    return on && driven && off && activity == VA_STRONG ? 0 : 1;
}

int runLife(int argc, char** argv) {
    const std::string sub = argc > 2 ? argv[2] : "list";
    if (sub == "search") return lifeSearch(argc > 3 ? argv[3] : "all");
    if (sub == "kit") return lifeKit();
    if (sub == "auto") return lifeAuto();
    if (sub == "sight") return lifeSight();
    if (sub == "list") return lifeList(argc > 3 ? std::max(1, atoi(argv[3])) : 12);
    if (sub == "survey") return lifeSurvey(argc > 3 ? std::max(1, atoi(argv[3])) : 1500);
    if (sub == "kin") return lifeKin(argc > 3 ? std::max(1.0, atof(argv[3])) : 200.0);
    if (sub == "made") return lifeMade(argc > 3 ? std::max(1, atoi(argv[3])) : 100);
    if (sub == "plates" || sub == "strips" || sub == "distinct" || sub == "forms") return runLifeLook(argc, argv);
    printf("life list [n] | survey [systems] | kin [ly] | plates [n] | strips [plan] | distinct [species] | forms [plan] | search [scene|all] | kit | made [n] | auto | sight\n");
    return 2;
}
