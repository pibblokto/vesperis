// E-02: the fingerprint modes, what the generators make written as keyed lines, so that a step meant to change nothing is proven by
// comparing two runs' files (`tools/prove`, `tools/keydiff.py`). `fingerprint bodies|stream|peoples|modules [n] [out] [full]`:
//   bodies   every body of four scan regions (Q-03's fp5): its generated state, its made generator, its data sheet's sentence and
//            traits, its sky and climate, its ground at four scales; the drifters nearest home with their moons (the dark bodies
//            and the dark ocean, which no scan reaches); three bodies of each drained kind with the drainage on
//   stream   every body of the scan region and of a strip of far sectors of every class (P-01's draw-stream tool): the draws, and what
//            a made kind sets
//   peoples  every people of D-05's fixed worlds and of every n-th people's world round home (D-05's recdump): its profile, lore,
//            culture, history, record catalogue, homes, records and texts
//   modules  for a fixed sample of worlds, what nothing else pins: see fingerprint_modules.cpp
// A line is `<what> <key...> <field>=<value> ...`, a field's value a number or the FNV-1a hash of its bytes: a new field adds a
// column instead of changing every line. `n` thins the sample (every n-th; the defaults are the battery's); `full` writes the
// texts the hashes are taken over (bodies has none), to read what moved. Files go to shots/tests/fingerprint_<kind>.txt by default.
#include "harness/fingerprint.h"
#include "galaxy/starfield.h"
#include "galaxy/system.h"
#include "galaxy/planetmap.h"
#include "galaxy/drainage.h"
#include "galaxy/drifters.h"
#include "galaxy/kinds.h"
#include "peoples/peoples.h"
#include "peoples/history.h"
#include "peoples/records.h"
#include "peoples/shards.h"
#include "peoples/charts.h"
#include "peoples/music.h"
#include "peoples/ruins.h"
#include "core/rng.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

uint64_t fpHash(const std::string& bytes) {
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : bytes) { h ^= c; h *= 1099511628211ULL; }
    return h;
}

void fpPut(std::string& s, const char* fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    const int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (n < (int)sizeof buf) { s.append(buf, n); return; }
    std::string big(n + 1, '\0');
    va_start(ap, fmt);
    vsnprintf(&big[0], n + 1, fmt, ap);
    va_end(ap);
    s.append(big.data(), n);
}

namespace {

// The bodies: fp5's fields, each group's bytes kept apart so that `all` (fp5's hash, the groups' bytes in fp5's order) and each group's
// own hash come from one pass
struct Bytes {
    std::string b;
    void raw(const void* p, size_t n) { b.append((const char*)p, n); }
    void d(double v) { raw(&v, sizeof v); }
    void f(float v) { raw(&v, sizeof v); }
    void i(int64_t v) { raw(&v, sizeof v); }
    void rgb(const RGB& c) { f(c.r); f(c.g); f(c.b); }
    void v3(const Vec3& v) { d(v.x); d(v.y); d(v.z); }
    void s(const std::string& t) { raw(t.data(), t.size()); i((int64_t)t.size()); }
};

void bodyBytes(Bytes& h, const Body& b) {
    h.i(b.index); h.i(b.parent); h.i(b.type); h.i((int64_t)b.seed); h.s(b.name);
    h.d(b.radiusKm); h.d(b.massEarths); h.d(b.gravity); h.d(b.orbitRadiusKm); h.d(b.orbitPeriod); h.d(b.orbitPhase0); h.d(b.orbitIncl);
    h.d(b.orbitNode); h.d(b.rotPeriod); h.d(b.rotPhase0); h.d(b.axialTilt); h.d(b.axisAzimuth); h.i(b.rings); h.d(b.ringInner);
    h.d(b.ringOuter); h.d(b.tempK); h.i(b.moonCount); h.rgb(b.color); h.v3(b.spinAxis); h.v3(b.ref0); h.v3(b.ref1); h.d(b.ecc);
    h.d(b.argPeri); h.d(b.luminosity); h.i(b.starClass); h.i(b.doublePlanet); h.i(b.locked); h.v3(b.lockedDir); h.i(b.glassed);
    h.i(drawnKind(b));
}

void genBytes(Bytes& h, const BodyGen& g) {
    h.i(g.type); h.i((int64_t)g.seed); h.i((int64_t)g.worldSeed); h.d(g.R); h.d(g.seaLevel); h.d(g.mountainAmp); h.d(g.hillAmp);
    h.d(g.detailAmp); h.d(g.craterDensity); h.d(g.craterDepth); h.d(g.iceCapLat); h.d(g.snowLine); h.d(g.moistureBias); h.d(g.lavaLevel);
    h.d(g.fractureWidth); h.d(g.duneAmp); h.d(g.cloudCover); h.d(g.cloudScale); h.d(g.roughness); h.d(g.boulderDensity); h.d(g.volcanoes);
    h.d(g.bandCount); h.d(g.stormShare); h.d(g.bandContrast); h.i(g.gsOn); h.d(g.gsLat); h.d(g.gsLon); h.d(g.gsA); h.d(g.gsB); h.d(g.gsTone);
    h.rgb(g.color); h.rgb(g.color2); h.rgb(g.skyTint); h.rgb(g.vegColor); h.rgb(g.vegColor2); h.i(g.tectonic); h.d(g.plateK);
    h.d(g.chainAmp); h.d(g.duneWl); h.d(g.windAngle); h.i(g.duneStyle); h.d(g.season); h.d(g.tempBias); h.d(g.riverDensity);
    h.d(g.lakeDensity); for (int k = 0; k < 3; k++) h.i(g.traits[k]);
    h.d(g.liquidLevel); h.d(g.contScale); h.d(g.islandDensity); h.d(g.oldSeaM); h.i(g.floraFamily); h.i(g.floraFamily2); h.i(g.locked);
    h.v3(g.lockedDir); h.d(g.selfGlow); h.d(g.seaReachDeg);
    for (int m = 0; m < 25; m++) h.rgb(g.matColor[m]);
    h.d(g.reliefWl0); h.d(g.reliefH); h.d(g.reliefRidge); h.d(g.reliefWarp); h.d(g.reliefErosion); h.d(g.reliefHybrid); h.d(g.reliefFlat);
    h.d(g.reliefHill); h.d(g.reliefMtn); h.d(g.cliffStep); h.d(g.cliffAmount);
    h.i((int64_t)g.sA); h.i((int64_t)g.sB); h.i((int64_t)g.sC); h.i((int64_t)g.sD); h.i((int64_t)g.sE); h.i((int64_t)g.sF); h.i((int64_t)g.sG);
}

void sampleBytes(Bytes& h, const SurfaceSample& s) {
    h.d(s.height); h.i(s.material); h.d(s.albedo); h.d(s.veg); h.d(s.glow); h.d(s.relief); h.d(s.water); h.i(s.biome); h.d(s.landform);
    h.i(s.waterKind); h.d(s.shore); h.d(s.scree); h.d(s.oldSea);
}

// E-02: what fp5 did not hash, in columns of their own (`gen2`, `ground2`), so that `all` stays fp5's
void gen2Bytes(Bytes& h, const BodyGen& g) {
    h.d(g.latGradient); h.d(g.seaDepth); h.d(g.grassBand); h.d(g.subContrast); h.d(g.tideRangeM); h.rgb(g.waterTintColour); h.rgb(g.lifeTintColour);
    h.v3(g.apexDir); h.i(g.starSx); h.i(g.starSy); h.i(g.starSz); h.i(g.bodyIndex); h.i(g.starKnown); h.d(g.nowTempC); h.i(g.deadAs); h.i(g.dark);
    h.i(g.frozenSeas); h.i(g.drifter);
}

void sample2Bytes(Bytes& h, const SurfaceSample& s) {
    h.i(s.landformTrait); h.d(s.waterTint); h.d(s.lifeTint); h.d(s.groundTint); h.d(s.slipFace);
}

// a body's line: fp5's groups and hash, the columns it lacked, the trait slots
void bodyLine(FILE* out, const char* what, int64_t x, int64_t y, int64_t z, const StarSystem& sys, int bi) {
    const Body& b = sys.bodies[bi];
    Bytes body, label, gen, sheet, traits, sky, climate, ground, gen2, ground2;
    bodyBytes(body, b); label.s(sys.bodyLabel(bi));
    if (b.type != PT_COMPANION) {
        const double season = (bi % 3 == 0) ? 0.0 : sys.seasonOf(bi, 1000.0 * bi);
        BodyGen g = BodyGen::make(b, season);
        genBytes(gen, g); gen2Bytes(gen2, g); sheet.s(describeBody(b, g)); traits.s(traitList(g));
        RGB zen, hor;
        const bool hasSky = typeSky(g, zen, hor);
        sky.i(hasSky);
        if (hasSky) { sky.rgb(zen); sky.rgb(hor); }
        climate.d(climateTempC(g, 33.0, 250.0, 0.3, 0.1)); climate.d(climateTempC(g, 71.0, 0.0, 0.0, 0.0));
        if (KINDS[b.type].landable || (KINDS[b.type].tags & KT_GIANT)) {
            Rng r(b.seed ^ 0xF1A9ULL);
            static const double details[4] = {20000.0, 2500.0, 512.0, 16.0};
            for (int k = 0; k < 6; k++) {
                const double z0 = r.sym(1.0), a = r.range(0, TAU), rr = std::sqrt(std::max(0.0, 1 - z0 * z0));
                const Vec3 u(rr * std::cos(a), rr * std::sin(a), z0);
                for (int dd = 0; dd < 4; dd++) {
                    const SurfaceSample smp = sampleSurface(g, u, details[dd]);
                    sampleBytes(ground, smp); sample2Bytes(ground2, smp);
                }
                ground.d(sampleCloudPattern(g, a - PI, std::asin(z0)));
            }
        }
    }
    std::string slots = "S";
    if (b.type != PT_COMPANION) {
        const BodyGen g2 = BodyGen::make(b);
        for (int k = 0; k < TRAIT_SLOTS; k++) slots += std::to_string(g2.traits[k]) + (k + 1 < TRAIT_SLOTS ? "," : "");
    }
    const std::string all = body.b + label.b + gen.b + sheet.b + traits.b + sky.b + climate.b + ground.b;
    fprintf(out, "%s %lld %lld %lld %d type=%d all=%016llx body=%016llx label=%016llx gen=%016llx sheet=%016llx traits=%016llx "
                 "sky=%016llx climate=%016llx ground=%016llx gen2=%016llx ground2=%016llx slots=%s\n",
            what, (long long)x, (long long)y, (long long)z, bi, b.type, (unsigned long long)fpHash(all), (unsigned long long)fpHash(body.b),
            (unsigned long long)fpHash(label.b), (unsigned long long)fpHash(gen.b), (unsigned long long)fpHash(sheet.b),
            (unsigned long long)fpHash(traits.b), (unsigned long long)fpHash(sky.b), (unsigned long long)fpHash(climate.b),
            (unsigned long long)fpHash(ground.b), (unsigned long long)fpHash(gen2.b), (unsigned long long)fpHash(ground2.b), slots.c_str());
}

int fingerprintBodies(FILE* out, int every) {
    setDrainageEnabled(false);
    struct Scan { int64_t x0, x1, y, z0, z1; };
    const Scan scans[] = {{150, 340, 0, 20, 160}, {4000, 4060, 0, 4000, 4060}, {-20000, -19940, 0, 3000, 3060}, {195, 196, 0, 46, 47}};
    long bodies = 0, drained = 0;
    for (const Scan& sc : scans)
        for (int64_t x = sc.x0; x < sc.x1; x += every)
            for (int64_t z = sc.z0; z < sc.z1; z++) {
                Star st;
                if (!starInSector(x, sc.y, z, st)) continue;
                StarSystem sys; sys.generate(st);
                for (int bi = 0; bi < (int)sys.bodies.size(); bi++) { bodyLine(out, "body", x, sc.y, z, sys, bi); bodies++; }
            }
    // the dark bodies (D-04), which no scan reaches: the drifters nearest home, each with its moons, until a dark ocean (made only on a
    // drifter) is among them and sixteen drifters are taken
    long dark = 0;
    {
        Star home;
        starInSector(HOME_SX, HOME_SY, HOME_SZ, home, true);
        std::vector<Star> near;
        driftersNear(home.pos, 3000.0, near);
        int taken = 0;
        bool ocean = false;
        for (const Star& n0 : near) {
            if (taken >= 16 && ocean) break;
            Star d;
            if (!drifterInSector(n0.sx, n0.sy, n0.sz, d, true)) continue;
            StarSystem sys; sys.generate(d);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                bodyLine(out, "dark", d.sx, d.sy, d.sz, sys, bi);
                ocean = ocean || sys.bodies[bi].type == PT_DARK_OCEAN;
                dark++;
            }
            taken++;
        }
    }
    // the drained kinds, three of each in the scan region, with the drainage at the fine scales
    setDrainageEnabled(true);
    int done[64] = {0};
    for (int64_t x = 150; x < 330; x++)
        for (int64_t z = 20; z < 140; z++) {
            Star st;
            if (!starInSector(x, 0, z, st)) continue;
            StarSystem sys; sys.generate(st);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type == PT_COMPANION || b.type >= 64 || done[b.type] >= 3 || !(KINDS[b.type].tags & KT_DRAINAGE)) continue;
                done[b.type]++;
                BodyGen g = BodyGen::make(b);
                Rng r(b.seed ^ 0xD8A1ULL);
                Bytes hd, hd2;
                for (int k = 0; k < 3; k++) {
                    const double z0 = r.sym(0.8), a = r.range(0, TAU), rr = std::sqrt(std::max(0.0, 1 - z0 * z0));
                    const Vec3 u(rr * std::cos(a), rr * std::sin(a), z0);
                    for (double det : {64.0, 16.0}) {
                        const SurfaceSample smp = sampleSurface(g, u, det);
                        sampleBytes(hd, smp); sample2Bytes(hd2, smp);
                    }
                }
                fprintf(out, "drained %lld 0 %lld %d type=%d all=%016llx ground2=%016llx\n", (long long)x, (long long)z, bi, b.type,
                        (unsigned long long)fpHash(hd.b), (unsigned long long)fpHash(hd2.b));
                drained++;
            }
        }
    printf("fingerprint bodies: %ld bodies, %ld dark, %ld drained\n", bodies, dark, drained);
    return 0;
}

// The stream: the draw-stream tool's two lines a body (P-01: the draws, the tilt apart; what a made kind may change), hashed
int fingerprintStream(FILE* out, int every, bool full) {
    long systems = 0, bodies = 0;
    auto one = [&](int64_t x, int64_t y, int64_t z, const Star& st) {
        StarSystem sys; sys.generate(st); systems++;
        std::string name = st.name;
        for (char& c : name) if (c == ' ') c = '_';   // a value holds no space
        fprintf(out, "system %lld %lld %lld name=%s cls=%d bodies=%zu\n", (long long)x, (long long)y, (long long)z, name.c_str(), st.cls,
                sys.bodies.size());
        for (const Body& b : sys.bodies) {
            std::string draws, made;
            fpPut(draws, "%d %d %llx %s %d | %.17g %.17g %.17g %.17g %.17g %.17g | %.17g %.17g %.17g | %d %.17g %.17g %.17g %d %d %d\n", b.index,
                  b.parent, (unsigned long long)b.seed, b.name.c_str(), drawnKind(b), b.orbitPhase0, b.orbitIncl, b.orbitNode, b.rotPhase0, 0.0,
                  b.axisAzimuth, b.ringInner, b.ringOuter, b.tempK, b.moonCount, b.ecc, b.argPeri, b.luminosity, b.starClass, (int)b.doublePlanet,
                  (int)b.glassed);
            const bool planet = b.parent < 0 || (b.parent >= 0 && sys.bodies[b.parent].type == PT_COMPANION);
            fpPut(made, "%d %d W%d type %d R %.17g m %.17g g %.17g col %.9g %.9g %.9g rings %d | orbit %.17g period %.17g rot %.17g locked %d tilt %.17g\n",
                  b.index, (int)planet, (int)b.wanderer, b.type, b.radiusKm, b.massEarths, b.gravity, b.color.r, b.color.g, b.color.b, (int)b.rings,
                  b.orbitRadiusKm, b.orbitPeriod, b.rotPeriod, (int)b.locked, b.axialTilt);
            bodies++;
            if (full) { fprintf(out, "  draws %s", draws.c_str()); fprintf(out, "  made %s", made.c_str()); continue; }
            fprintf(out, "stream %lld %lld %lld %d draws=%016llx made=%016llx\n", (long long)x, (long long)y, (long long)z, b.index,
                    (unsigned long long)fpHash(draws), (unsigned long long)fpHash(made));
        }
    };
    // the scan region of the harness, and a strip toward the centre and one up the arm (every class, the rare ones included)
    for (int64_t x = 150; x < 340; x += every)
        for (int64_t z = 20; z < 160; z++) { Star st; if (starInSector(x, 0, z, st)) one(x, 0, z, st); }
    for (int64_t x = -2000; x < 12000; x += 37 * every)
        for (int64_t z = -5500; z < 400; z += 211) {
            const int64_t y = (x / 37) % 7 - 3;
            Star st;
            if (starInSector(x, y, z, st)) one(x, y, z, st);
        }
    printf("fingerprint stream: %ld systems, %ld bodies\n", systems, bodies);
    return 0;
}

// The peoples: recdump's print of a people, section by section; the keyed line hashes each section, `full` writes them
struct PeopleText { std::string head, profile, lore, culture, history, records, homes, shards, texts; };

void peopleText(PeopleText& o, const StarSystem& sys, const Body& b, const BodyGen& g, int which) {
    fpPut(o.head, "== %s people %d (of %d)\n", sys.bodyLabel(b.index).c_str(), which, peoplesOf(g));
    const PeopleProfile P = peopleProfileOf(sys, b, g, which);
    fpPut(o.profile, "profile age %d last %d ended %.6f radio %d/%d cause %d collapse %d+%d stages %.6f %.6f %.6f land %.6f held %.6f peak %.6f gov %d eco %d %d faith %d rite %d holy %d values",
          P.age, P.lastYear, P.endedYearsAgo, P.radio, P.radioYear, P.cause, P.collapseYear, P.collapseYears, P.stage[1], P.stage[2], P.stage[3],
          P.landShare, P.held, P.peak, P.government, P.economy, P.economy2, P.faith, P.rite, P.holy);
    for (double v : P.values) fpPut(o.profile, " %.6f", v);
    fpPut(o.profile, "\n eras");
    for (const Era& e : P.eras) fpPut(o.profile, " [%d %d..%d g%d]", e.age, e.start, e.end, e.government);
    fpPut(o.profile, "\n");
    const Lore L = loreOf(sys, b, g, which);
    fpPut(o.lore, "lore %s %s %s %s %s %s %s %s %s %s %s style %d year %.6f day %.6f moons %d desert %d span %d month %d/%d cal %d last %d records %d end %d from %d other '%s' %d\n",
          L.people.c_str(), L.god.c_str(), L.river.c_str(), L.mountain.c_str(), L.sea.c_str(), L.city.c_str(), L.city2.c_str(), L.founder.c_str(),
          L.moon.c_str(), L.star.c_str(), L.festival.c_str(), L.style, L.yearDays, L.dayHours, L.moons, L.desert, L.spanYears, L.monthDays,
          L.months, L.calendar, L.last, L.records, L.endYear, L.ageFrom, L.other.c_str(), L.otherFate);
    for (const ChartMark& m : L.marks)
        fpPut(o.lore, " mark %lld %lld %lld body %d how %d ly %.6f '%s'\n", (long long)m.star.sx, (long long)m.star.sy, (long long)m.star.sz, m.body,
              m.how, m.ly, m.people.c_str());
    const Culture C = cultureOf(g, which);
    fpPut(o.culture, "culture age %d years %.6f weather %.6f wet %.6f decay %.6f buried %.6f wear %.6f paved %d\n", C.age, C.ageYears,
          C.weatherYears, C.wet, C.decay, C.buried, C.roadWear, C.paved);
    std::shared_ptr<const History> H = historyOf(sys, b, g, which);
    if (H) {
        Bytes hh;
        for (const HistoryEvent& e : H->events) {
            hh.i(e.kind); hh.i(e.year); hh.i(e.season); hh.i(e.pub); hh.d(e.salience); hh.i(e.place); hh.i(e.place2); hh.i(e.a); hh.i(e.b);
            hh.i(e.c); hh.i(e.cause); hh.i(e.thing); hh.i(e.out); hh.i(e.other); hh.i(e.value);
        }
        for (const HistoryPerson& p : H->persons) {
            hh.i((int64_t)p.name); hh.i(p.born); hh.i(p.died); hh.i(p.fate); hh.i(p.family); hh.i(p.role); hh.i(p.trade); hh.i(p.traits);
        }
        for (const HistoryPlace& p : H->places) {
            hh.i(p.plan); hh.i(p.sclass); hh.i((int64_t)p.name); hh.i(p.founded); hh.i(p.ended); hh.i(p.fate); hh.d(p.lat); hh.d(p.lon); hh.d(p.peak);
        }
        fpPut(o.history, "history first %d last %d events %zu persons %zu places %zu lastPlace %d lastPerson %d hash %016llx\n", H->first, H->last,
              H->events.size(), H->persons.size(), H->places.size(), H->lastPlace, H->lastPerson, (unsigned long long)fpHash(hh.b));
        for (const HistoryEra& e : H->eras) fpPut(o.history, " era %s\n", e.name.c_str());
        for (int k = 0; k < (int)H->events.size(); k += 37) fpPut(o.history, " event %d: %s\n", k, historyEventLine(*H, k).c_str());
    }
    std::shared_ptr<const RecordCatalogue> RC = recordsOf(g, which);
    if (RC) {
        fpPut(o.records, "records %d last %d years %d..%d music %d charts %d\n", RC->count, RC->last, RC->firstYear, RC->lastYear, RC->music, RC->charts);
        for (const RecordEntry& e : RC->entries) fpPut(o.records, " %d/%d/%d/%d/%d", e.year, e.age, e.category, e.medium, e.last);
        fpPut(o.records, "\n");
    }
    std::shared_ptr<const RecordHomes> RH = recordHomesOf(g, which);
    if (RH) {
        fpPut(o.homes, "homes");
        for (int r : RH->rank) fpPut(o.homes, " %d", r);
        fpPut(o.homes, "\n");
    }
    std::shared_ptr<const ShardTexts> T = shardTextsOf(sys, b, g, L);
    const Tradition TR = traditionOf(g, L);
    for (int i = 0; i < L.records; i++) {
        const Shard s = shardOf(sys, b, g, L, i);
        fpPut(o.shards, "[%d] kind %d tone %d year %d %d/%d last %d music %d chart %d age %d medium %d\n", i, s.kind, s.tone, s.year, s.month,
              s.day, s.last, s.music, s.chart, s.recordAge, s.medium);
        if (s.music) {
            Piece pc; pieceOf(TR, s, pc);
            Bytes hh;
            for (const Note& nt : pc.notes) {
                hh.d(nt.t); hh.d(nt.dur); hh.i(nt.voice); hh.i(nt.degree); hh.d(nt.vel); hh.i(nt.slideFrom); hh.i(nt.vowel); hh.d(nt.bend);
            }
            fpPut(o.shards, " piece form %d bpm %.6f beats %.6f notes %zu age %d hash %016llx\n", pc.form, pc.bpm, pc.beats, pc.notes.size(), pc.age,
                  (unsigned long long)fpHash(hh.b));
        } else if (s.chart) {
            StarChart ch;
            if (chartOf(sys, b, g, L, i, ch)) fpPut(o.shards, " chart style %d stars %zu figures %zu mark %d\n", ch.style, ch.stars.size(), ch.figures.size(), ch.markStar);
            fpPut(o.shards, " %s\n", s.text.c_str());
        } else
            fpPut(o.shards, " %s\n", s.text.c_str());
    }
    if (T) {
        int k = 0;
        for (const ShardText& t : T->items) {
            if (!t.text.empty()) fpPut(o.texts, "{%d %d %d %d %d %d} %s\n", k, t.kind, t.age, t.year, t.writer, t.subject, t.text.c_str());
            k++;
        }
    }
}

int fingerprintPeoples(FILE* out, int every, bool full) {
    struct W { int64_t sx, sz; int body; };
    std::vector<W> worlds = {{198, 55, 10}, {200, 41, 1}, {189, 45, 2}, {208, 51, 5}, {210, 33, 0}, {202, 26, 2}, {202, 28, 0},
                             {200, 16, 0}, {222, 75, 3}, {226, 82, 0}, {228, -44, 1}, {122, 123, 14}, {323, 192, 0}, {250, 158, 3}};
    int seen = 0;
    for (int64_t x = 150; x < 260; x++)   // every `every`-th people's world of the plane round home, in sector order
        for (int64_t z = 0; z < 120; z++) {
            Star st;
            if (!starInSector(x, 0, z, st)) continue;
            StarSystem sys; sys.generate(st);
            for (const Body& b : sys.bodies) {
                if (!kindHas(b.type, KT_PEOPLE)) continue;
                if (!worldHadCivilisation(BodyGen::make(b))) continue;
                if (seen++ % every == 0) worlds.push_back({x, z, b.index});
            }
        }
    int peoples = 0;
    for (const W& w : worlds) {
        Star st;
        if (!starInSector(w.sx, 0, w.sz, st)) {
            fprintf(out, full ? "no star at %lld %lld\n" : "world %lld 0 %lld none=star\n", (long long)w.sx, (long long)w.sz);
            continue;
        }
        StarSystem sys; sys.generate(st);
        if (w.body < 0 || w.body >= (int)sys.bodies.size()) continue;
        const Body& b = sys.bodies[w.body];
        const BodyGen g = BodyGen::make(b);
        if (!worldHadCivilisation(g)) {
            if (full) fprintf(out, "%s: no people\n", sys.bodyLabel(w.body).c_str());
            else fprintf(out, "world %lld 0 %lld %d none=people\n", (long long)w.sx, (long long)w.sz, w.body);
            continue;
        }
        for (int k = 0; k < peoplesOf(g); k++) {
            PeopleText t;
            peopleText(t, sys, b, g, k);
            peoples++;
            if (full) {
                fputs((t.head + t.profile + t.lore + t.culture + t.history + t.records + t.homes + t.shards + t.texts).c_str(), out);
                continue;
            }
            fprintf(out, "people %lld 0 %lld %d %d head=%016llx profile=%016llx lore=%016llx culture=%016llx history=%016llx records=%016llx "
                         "homes=%016llx shards=%016llx texts=%016llx\n",
                    (long long)w.sx, (long long)w.sz, w.body, k, (unsigned long long)fpHash(t.head), (unsigned long long)fpHash(t.profile),
                    (unsigned long long)fpHash(t.lore), (unsigned long long)fpHash(t.culture), (unsigned long long)fpHash(t.history),
                    (unsigned long long)fpHash(t.records), (unsigned long long)fpHash(t.homes), (unsigned long long)fpHash(t.shards),
                    (unsigned long long)fpHash(t.texts));
        }
        fflush(out);
    }
    printf("fingerprint peoples: %d people's worlds seen round home, %zu taken, %d peoples\n", seen, worlds.size(), peoples);
    return 0;
}

}   // namespace

int runFingerprint(int argc, char** argv) {
    const std::string kind = argc > 2 ? argv[2] : "";
    std::vector<std::string> rest;
    bool full = false;
    for (int i = 3; i < argc; i++) {
        if (std::strcmp(argv[i], "full") == 0) full = true;
        else rest.push_back(argv[i]);
    }
    const int defaults = kind == "peoples" ? 6 : kind == "modules" ? 1 : 1;
    const int every = !rest.empty() && atoi(rest[0].c_str()) > 0 ? atoi(rest[0].c_str()) : defaults;
    const std::string path = rest.size() > 1 ? rest[1] : "shots/tests/fingerprint_" + kind + (full ? "_full" : "") + ".txt";
    if (kind != "bodies" && kind != "stream" && kind != "peoples" && kind != "modules") {
        fprintf(stderr, "fingerprint bodies|stream|peoples|modules [n] [out] [full]\n");
        return 2;
    }
    FILE* out = fopen(path.c_str(), "w");
    if (!out) { fprintf(stderr, "fingerprint: cannot write %s\n", path.c_str()); return 1; }
    int rc = 0;
    if (kind == "bodies") rc = fingerprintBodies(out, every);
    else if (kind == "stream") rc = fingerprintStream(out, every, full);
    else if (kind == "peoples") rc = fingerprintPeoples(out, every, full);
    else rc = fingerprintModules(out, every, full);
    fclose(out);
    printf("fingerprint %s -> %s\n", kind.c_str(), path.c_str());
    return rc;
}
