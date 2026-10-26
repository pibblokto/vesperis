#include "system.h"
#include "drifters.h"
#include "core/rng.h"
#include "core/noise.h"
#include <cmath>
#include <list>
#include <mutex>

namespace {
constexpr double ORBIT_K = 8.0e-6;         // planet period = K * r^1.5 / sqrt(mass)
constexpr double MOON_K = 1.26e-3;
// B-425 (GEN 14): a companion star keeps clear of the other sun's worlds (`StarSystem::generate`)
constexpr double COMPANION_DISC = 3.0 * DEG;   // the most a far companion spans in the sky of the primary's worlds, and the primary in its own worlds'
constexpr double COMPANION_HEAT_K = 100.0;     // the most a far companion's light alone warms the primary's worlds (an equilibrium temperature)
constexpr double CLOSE_LIGHT = 0.1;            // the most a close companion shines beside the primary (its worlds circle both)
// P-05: the systems that caught a wanderer (a rogue world: `generate`'s last step), by a hash of the star's seed with a salt of its own
constexpr double WANDERER_SHARE = 0.04;
constexpr uint64_t WANDERER_SALT = 0x3A9DE7E200000001ULL;
constexpr double WANDERER_LIVING = 0.2;        // of them, the share that was a living world (the frozen felisian)

double equilibriumTemp(double luminosity, double orbitKm) {
    return 278.0 * std::pow(luminosity, 0.25) * std::sqrt(AU_GAME_KM / orbitKm);
}

// P-00: the weights are the kinds' rows: a kind's weight in the zone of the temperature, then each star-class rule that applies, in
// the order below (KindDef::rules: w x mul + add), the moons' by their parent; the Wolf-Rayet wind strips every air but a giant's
// before its own rule (R-307 GEN 9, S-01's varieties, S-03..S-06's dead and young stars, M9-16's moons)
// (P-07: over the drawn kinds alone: a made kind has no weight in a zone and no rule, `kindsOk`, and walking the made kinds' rows for every
// pick had cost the generator a tenth of its time)
int pickPlanetType(Rng& rng, double tempK, int starClass, bool isMoon, int parentType, double sizeRand) {
    double w[PT_DRAWN_COUNT];
    const int zone = tempK > 600 ? 0 : (tempK > 345 ? 1 : (tempK > 205 ? 2 : (tempK > 120 ? 3 : 4)));
    for (int k = 0; k < PT_DRAWN_COUNT; k++) w[k] = KINDS[k].zones[zone];
    auto apply = [&](int r) { for (int k = 0; k < PT_DRAWN_COUNT; k++) { const WeightOp& o = KINDS[k].rules[r]; if (o.mul != 1 || o.add != 0) w[k] = w[k] * o.mul + o.add; } };
    if (starClass == STAR_WHITE_DWARF || starClass == STAR_PULSAR || starClass == STAR_NEUTRON || starClass == STAR_BLACK_HOLE) {   // a black hole's are the dead stars' and less
        apply(WR_DEAD);   // the dead star stripped the thick airs
        if (starClass == STAR_PULSAR) apply(WR_PULSAR);
        if (starClass == STAR_NEUTRON) apply(WR_NEUTRON);   // the survivors of the supernova: bare cores, captured rocks; no giant kept its envelope
        if (starClass == STAR_BLACK_HOLE) apply(WR_BLACK_HOLE);   // wandering rocks and ice, nothing with an air
    }
    if (starClass == STAR_PROTOSTAR) apply(WR_PROTOSTAR);   // worlds still forming: molten and bombarded, no air or sea had time to come, no giant has finished gathering
    if (starClass == STAR_RED_GIANT) apply(WR_RED_GIANT);
    if (starClass == STAR_BLUE_GIANT) apply(WR_BLUE_GIANT);
    if (starClass == STAR_BLUE_WHITE) apply(WR_BLUE_WHITE);       // bare rock and ice under the ultraviolet
    if (starClass == STAR_ORANGE_GIANT) apply(WR_ORANGE_GIANT);   // the thawed ice worlds of an old star
    if (starClass == STAR_CARBON) apply(WR_CARBON_STAR);          // the soot falls on every world
    if (isMoon) apply(parentType >= 0 && kindHas(parentType, KT_GIANT) ? WR_MOON_OF_GIANT : WR_MOON_OF_WORLD);   // moons draw from their own table; R-307: tides heat a giant's inner moons
    if (starClass == STAR_WOLF_RAYET) {   // the wind stripped every atmosphere and sublimed the near ice; what is left is bare, cratered and bombarded
        for (int k = 0; k < PT_DRAWN_COUNT; k++) if (KINDS[k].atmosphere && !kindHas(k, KT_GIANT)) w[k] = 0;
        apply(WR_WOLF_RAYET);
    }
    if (sizeRand < 0.3) apply(WR_SMALL);
    return rng.pick(w, PT_DRAWN_COUNT);   // the drawn kinds alone (the pick's fall-through is the last of them, as in generation 15)
}

// P-00: the colour from afar is the kind's: each channel base + var h of one draw h (or base + var (1 - h)), or one of four by a second draw
RGB typeColor(int type, Rng& rng) {
    const KindColour& c = KINDS[type].colour;
    float h = (float)rng.uni();
    if (c.palette) return c.pal[rng.irange(4)];
    auto at = [h](const KindChannel& k) { return k.flip ? k.base + k.var * (1 - h) : k.base + k.var * h; };
    return RGB(at(c.ch[0]), at(c.ch[1]), at(c.ch[2]));
}

// P-01: a made kind's colour from afar, its draws taken from hashes of the world's seed (the system drew the drawn kind's)
RGB madeColour(int type, uint64_t seed) {
    const KindColour& c = KINDS[type].colour;
    float h = (float)unitFromHash(mix64(seed ^ 0xC0105EEDULL));
    if (c.palette) return c.pal[(int)(unitFromHash(mix64(seed ^ 0xC0105EEEULL)) * 4) & 3];
    auto at = [h](const KindChannel& k) { return k.flip ? k.base + k.var * (1 - h) : k.base + k.var * h; };
    return RGB(at(c.ch[0]), at(c.ch[1]), at(c.ch[2]));
}

// P-01: a body the generator drew as `drawnType` made the kind `kindOf` gives it (no draw: the stream is the drawn kind's). The radius
// from the made kind's range on the same size draw (a moon's within the cap its parent's drawn size set; a twin's stays its partner's
// share), the mass and the gravity from its density, its colour by hashes, and its rings by its own rule where that differs
void makeKind(Body& b, const KindFacts& f, double sizeRand, double maxR, bool keepRadius) {
    b.drawnType = b.type;
    const int made = kindOf(f);
    if (made == b.type) return;
    const KindDef& D = KINDS[b.type];
    const KindDef& K = KINDS[made];
    b.type = made;
    if (!keepRadius) {
        b.radiusKm = std::min(maxR, K.minRadiusKm + (K.maxRadiusKm - K.minRadiusKm) * sizeRand);
        if (b.radiusKm < 600) b.radiusKm = 600 + 400 * unitFromHash(mix64(b.seed ^ 0x5121ULL));   // a moon's floor (the drawn moon's draw is the drawn size's)
    }
    b.massEarths = std::pow(b.radiusKm / 6371.0, 3) * K.density;
    b.gravity = 9.8 * b.massEarths / std::pow(b.radiusKm / 6371.0, 2);
    b.color = madeColour(made, b.seed);
    if (K.untilted) b.axialTilt = 0;   // the spin frame and the locked face are set from it at the end of `generate`
    if ((f.body & KB_PLANET) && K.ringChance != D.ringChance) b.rings = unitFromHash(mix64(b.seed ^ 0x7121ULL)) < K.ringChance;   // the chthonian's ring of what it lost
    if (K.thenKind >= 0) b.thenTempK = f.drawn == K.thenKind ? f.tempK : 265.0 + 35.0 * unitFromHash(mix64(b.seed ^ 0x7E4E7ULL));   // the world as drawn, else a living climate
}
}

RGB madeKindColour(int type, uint64_t seed) { return madeColour(type, seed); }
bool peopleHash(uint64_t seed) { return unitFromHash(mix64(seed ^ 0xC1D1ULL)) < 0.125; }
bool peoplesHash(int type, uint64_t seed) {
    const KindDef& K = KINDS[type];
    if (!(K.tags & KT_PEOPLE)) return false;
    return K.thenKind >= 0 ? unitFromHash(mix64(seed ^ K.peopleSalt)) < K.peopleShare : peopleHash(seed);
}
bool drifterPeoplesHash(int type, uint64_t seed) {
    if (type == PT_FROZEN_FELISIAN) return unitFromHash(mix64(seed ^ KINDS[type].peopleSalt)) < DRIFTER_PEOPLE_SHARE;
    if (type == PT_DESERT) return unitFromHash(mix64(seed ^ DRIFTER_PEOPLE_SALT)) < DRIFTER_PEOPLE_SHARE;
    return false;
}

namespace {
// P-07: the made kinds' sources in `kindOf`'s order, packed (a kind's row is large: walking the rows for every body had cost the generator a
// tenth of its time): each source with its kind, the kind's salt, whether it is a giant's, and where the next kind's sources begin
struct MadeSource { KindSource s; uint64_t salt; int kind, next; bool giant; };
struct MadeSources { MadeSource src[PT_COUNT * MAX_KIND_SOURCES]; int n = 0; };
const MadeSources& madeSources() {
    static const MadeSources M = [] {
        MadeSources m;
        for (int k = PT_DRAWN_COUNT; k < PT_COUNT; k++) {
            const KindDef& K = KINDS[k];
            const int first = m.n;
            for (int i = 0; i < K.nSources; i++) m.src[m.n++] = MadeSource{K.sources[i], K.salt, k, 0, (K.tags & KT_GIANT) != 0};
            for (int i = first; i < m.n; i++) m.src[i].next = m.n;
        }
        return m;
    }();
    return M;
}
}

bool kindSourceFits(const KindSource& S, bool giant, const KindFacts& f) {
    if (f.paired && giant) return false;   // P-07: a double planet's halves are worlds
    if (!(S.from >> f.drawn & 1u) || f.tempK < S.tLo || f.tempK >= S.tHi || !(S.bodies & f.body)) return false;
    if ((S.stars && !(S.stars >> f.starClass & 1u)) || (S.lock > 0 && !f.locked) || (S.lock < 0 && f.sync)) return false;
    if (S.parents && (f.parent < 0 || !(S.parents >> f.parent & 1u))) return false;   // a moon of the planets it names (a brown dwarf's)
    return (f.moon & S.moon) == S.moon;   // a moon that is what the source names among its planet's moons
}

thread_local std::vector<KindFacts>* t_kindFactsSeen = nullptr;

int kindOf(const KindFacts& f) {
    if (f.drawn < 0 || f.drawn >= PT_DRAWN_COUNT) return f.drawn;
    if (kindHas(f.drawn, KT_PEOPLE) && peopleHash(f.seed)) return f.drawn;   // a people's world keeps its kind (its towns, records and signal stay)
    if (t_kindFactsSeen) t_kindFactsSeen->push_back(f);   // E-07: the harness's `salt scan`
    const MadeSources& M = madeSources();
    for (int i = 0; i < M.n; i++) {
        const MadeSource& m = M.src[i];
        const KindSource& S = m.s;
        if (!kindSourceFits(S, m.giant, f)) continue;
        if (unitFromHash(mix64((S.byStar ? f.starSeed : f.seed) ^ m.salt)) < S.share) return m.kind;   // a young star's worlds alike
        i = m.next - 1;   // the world qualified for this kind and its hash said no: on to the next kind
    }
    return f.drawn;
}

void StarSystem::generate(const Star& s) {
    star = s;
    bodies.clear();
    belts.clear();
    companion = -1;
    valid = s.valid;
    if (!valid) return;
    const StarClassInfo& ci = STAR_CLASSES[s.cls];
    const bool rogue = s.cls == STAR_ROGUE;   // D-01: a drifter's: the drifter on the anchor and its moons, nothing else
    Rng rng(s.seed ^ 0x51A7E11ULL);
    int np = rogue ? 0 : (rng.irange(ci.maxPlanets + 1) + rng.irange(ci.maxPlanets + 1) + 1) / 2;
    if (s.cls == STAR_PULSAR && rng.chance(0.3)) np = 0;
    if (s.cls == STAR_NEUTRON && rng.chance(0.25)) np = 0;   // a quarter of the neutron stars keep nothing but debris
    if (s.cls == STAR_PROTOSTAR && rng.chance(0.35)) np = 0;   // a third of the protostars have no world yet, only the disc
    if (s.cls == STAR_WOLF_RAYET && rng.chance(0.4)) np = 0;   // two Wolf-Rayet systems in five are rubble
    if (s.cls == STAR_BLACK_HOLE && rng.chance(0.5)) np = 0;   // half the black holes keep nothing but rubble
    // orbit radii: Kepler-like accumulation as in the original, limited beyond the 8th orbit
    double key = std::max(s.radiusKm * ci.firstOrbitMult, ci.minFirstOrbitKm) * (0.8 + 0.5 * rng.uni());
    // a Wolf-Rayet star keeps, two times in five, one giant far out beyond its bare worlds (its zones within four orbits are
    // too hot for one to form), the last body at 2.5-3.5 times the next orbit's key: its envelope streams away in the wind
    int farGiant = (s.cls == STAR_WOLF_RAYET && rng.chance(0.45)) ? np : -1;
    if (farGiant >= 0) np++;
    std::vector<double> drawnR;   // each planet's radius as drawn (its moons' orbits and sizes follow it, pushed clear of a made kind's)
    std::vector<char> drawnRings;
    std::vector<Body> asDrawn;    // each planet as drawn and its size draw, to make it again if a twin takes its lock (the moons below)
    std::vector<double> drawnSize;
    std::vector<KindFacts> drawnFacts;   // and the facts it was made by (a giant with a twin is made again by them, with no giant)
    std::vector<double> drawnOrbit;   // every body's orbit as drawn, a moon's before a made world pushed it out (the companion's clearance and
                                      // the comets read these: no star and no comet moves for a made world)
    for (int n = 0; n < np; n++) {
        Body b;
        b.index = (int)bodies.size();
        b.parent = -1;
        b.seed = hashCombine(s.seed, 1000 + n);
        if (n == farGiant) key *= 2.5 + rng.uni();
        b.orbitRadiusKm = key * (1.0 + rng.sym(0.08));
        b.tempK = equilibriumTemp(s.luminosity, b.orbitRadiusKm);
        double sizeRand = rng.uni();
        b.type = n == farGiant ? PT_GASGIANT : pickPlanetType(rng, b.tempK, s.cls, false, -1, sizeRand);
        const KindDef& pt = KINDS[b.type];
        b.radiusKm = pt.minRadiusKm + (pt.maxRadiusKm - pt.minRadiusKm) * sizeRand;
        double densityRel = pt.density;
        if (pt.tags & KT_GLOWS) b.luminosity = 0.0008 + 0.004 * rng.uni();   // it warms its moons and glows at night
        b.massEarths = std::pow(b.radiusKm / 6371.0, 3) * densityRel;
        b.gravity = 9.8 * b.massEarths / std::pow(b.radiusKm / 6371.0, 2);
        b.orbitPeriod = ORBIT_K * std::pow(b.orbitRadiusKm, 1.5) / std::sqrt(s.massFactor);
        b.orbitPhase0 = rng.range(0, TAU);
        b.orbitIncl = rng.sym(4 * DEG) * (1 + rng.uni());
        b.orbitNode = rng.range(0, TAU);
        // rotation: inner planets tend to be tidally locked (S-01: a red dwarf's first three nearly always; their orbits are rounded below)
        bool lockedNear = s.cls == STAR_RED_DWARF ? (n < 3 && rng.chance(0.9)) : (n == 0 && rng.chance(0.5));
        if (lockedNear) b.rotPeriod = b.orbitPeriod;
        else b.rotPeriod = 1200 + 12000 * rng.uni() * rng.uni();
        if (rng.chance(0.08) && !(lockedNear && s.cls == STAR_RED_DWARF)) b.rotPeriod = -b.rotPeriod;   // retrograde (S-01: not a red dwarf's locked world)
        b.rotPhase0 = rng.range(0, TAU);
        double tiltRoll = rng.uni();
        b.axialTilt = tiltRoll < 0.6 ? rng.range(0, 12 * DEG) : (tiltRoll < 0.93 ? rng.range(12 * DEG, 35 * DEG) : rng.range(35 * DEG, 95 * DEG));
        b.axisAzimuth = rng.range(0, TAU);
        b.rings = rng.chance(pt.ringChance);   // GEN 5 (O0-01): one solid world in eight wears rings, a gas giant three in five
        b.ringInner = 1.35 + 0.6 * rng.uni();
        b.ringOuter = b.ringInner + 0.5 + 1.6 * rng.uni();
        b.color = typeColor(b.type, rng);
        {   // eccentricity from its own stream: most orbits are nearly round, a few are clearly oval
            Rng er(b.seed ^ 0x0ECCULL);
            b.ecc = er.chance(0.6) ? er.range(0, 0.06) : (er.chance(0.8) ? er.range(0.06, 0.15) : er.range(0.15, 0.3));
            b.argPeri = er.range(0, TAU);
        }
        if (s.cls == STAR_RED_DWARF && n < 3) b.ecc *= 0.3;   // the tides of a red dwarf round the near orbits, so the locked face holds (M5-08 asks for e <= 0.05)
        drawnR.push_back(b.radiusKm); drawnRings.push_back(b.rings); asDrawn.push_back(b); drawnSize.push_back(sizeRand); drawnOrbit.push_back(b.orbitRadiusKm);
        {   // the kind it is made (every draw above was the drawn kind's), locked as the end of `generate` will find it
            KindFacts f;
            f.drawn = b.type;
            f.tempK = b.tempK;
            f.starClass = s.cls;
            f.body = KB_PLANET;
            f.sync = lockedNear && b.rotPeriod > 0;
            f.locked = f.sync && b.ecc <= 0.05;
            f.seed = b.seed;
            f.starSeed = s.seed;
            drawnFacts.push_back(f);
            makeKind(b, f, sizeRand, 1e18, false);
        }
        b.name = s.name + " " + romanNumeral(n + 1);
        bodies.push_back(b);
        double next = b.orbitRadiusKm * (1.32 + 0.45 * rng.uni());   // tighter spacing, more worlds in the temperate band
        if (pt.tags & KT_GIANT) next *= 1.25;
        key = n < 8 ? next : b.orbitRadiusKm * (1.18 + 0.12 * rng.uni());
    }
    if (rogue) {   // the drifter on the anchor: no orbit (a period no clock reaches), its kind, size and warmth by the dark's table
        // (`drifterBody`), its spin and its rings by a stream of its own; a living world frozen keeps the climate it had, as P-04's do
        const DrifterBody db = drifterBody(s);
        const KindDef& K = KINDS[db.type];
        Rng dr(s.seed ^ 0xD81F7B0DULL);
        Body b;
        b.index = 0; b.parent = -1;
        b.seed = hashCombine(s.seed, 1000);
        b.type = db.type;
        if (db.drawnType != db.type) b.drawnType = db.drawnType;   // a dark ocean, made from a rocky drifter (its moons are the rocky's)
        b.radiusKm = db.radiusKm;
        b.massEarths = std::pow(b.radiusKm / 6371.0, 3) * K.density;
        b.gravity = 9.8 * b.massEarths / std::pow(b.radiusKm / 6371.0, 2);
        if (K.tags & KT_GLOWS) b.luminosity = 0.0008 + 0.004 * dr.uni();   // a brown dwarf's glow lights its moons
        b.orbitRadiusKm = 0; b.orbitPeriod = 1e18;
        b.heatK = b.tempK = db.heatK;
        b.rotPeriod = (1200 + 12000 * dr.uni() * dr.uni()) * (dr.uni() < 0.08 ? -1 : 1);
        b.rotPhase0 = dr.range(0, TAU);
        const double tiltRoll = dr.uni();
        b.axialTilt = tiltRoll < 0.6 ? dr.range(0, 12 * DEG) : (tiltRoll < 0.93 ? dr.range(12 * DEG, 35 * DEG) : dr.range(35 * DEG, 95 * DEG));
        b.axisAzimuth = dr.range(0, TAU);
        b.rings = dr.uni() < K.ringChance;
        b.ringInner = 1.35 + 0.6 * dr.uni(); b.ringOuter = b.ringInner + 0.5 + 1.6 * dr.uni();
        b.color = madeColour(b.type, b.seed);
        if (K.thenKind >= 0) b.thenTempK = 265.0 + 35.0 * dr.uni();
        else if (drifterPeoplesHash(b.type, b.seed)) b.thenTempK = 270.0 + 50.0 * unitFromHash(mix64(b.seed ^ 0xD05E7ULL));   // a desert's people's warm dry climate
        b.name = s.name;
        bodies.push_back(b);
        drawnR.push_back(db.drawnRadiusKm); drawnRings.push_back(b.rings); asDrawn.push_back(b); drawnSize.push_back(0.5); drawnOrbit.push_back(0);
        KindFacts f; f.drawn = drawnKind(b); f.tempK = b.tempK; f.starClass = s.cls; f.body = KB_PLANET; f.seed = b.seed; f.starSeed = s.seed;
        drawnFacts.push_back(f);
    }
    if (s.cls == STAR_NEUTRON) {   // the blast fused the surfaces of the outermost one or two survivors (the near ones were vaporised; ice took none)
        int want = unitFromHash(mix64(s.seed ^ 0x61A55ULL)) < 0.5 ? 2 : 1, marked = 0;
        for (int n = (int)bodies.size() - 1; n >= 0 && marked < want; n--) {
            if (kindHas(drawnKind(bodies[n]), KT_GLASSABLE)) { bodies[n].glassed = true; marked++; }   // as drawn
        }
    }
    // asteroid belts in wide gaps (and sometimes beyond the last planet); S-03: a neutron star's system is mostly debris;
    // a protostar's is the disc itself: a belt inside the first orbit, one in nearly every gap and one beyond
    if (s.cls == STAR_PROTOSTAR && !bodies.empty() && rng.chance(0.7)) {
        Belt bl;
        bl.innerKm = std::max(bodies[0].orbitRadiusKm * 0.45, s.radiusKm * 3.0); bl.outerKm = bodies[0].orbitRadiusKm * 0.8;
        double mid = 0.5 * (bl.innerKm + bl.outerKm);
        bl.period = ORBIT_K * std::pow(mid, 1.5) / std::sqrt(s.massFactor);
        bl.phase0 = rng.range(0, TAU);
        bl.name = s.name + " belt " + romanNumeral((int)belts.size() + 1);
        belts.push_back(bl);
    }
    for (int n = 0; n + 1 < (int)bodies.size(); n++) {
        double r0 = bodies[n].orbitRadiusKm, r1 = bodies[n + 1].orbitRadiusKm;
        if ((r1 / r0 > 1.65 && rng.chance(0.55)) || rng.chance(s.cls == STAR_NEUTRON || s.cls == STAR_BLACK_HOLE ? 0.5 : (s.cls == STAR_PROTOSTAR ? 0.9 : (s.cls == STAR_WOLF_RAYET ? 0.35 : 0.12)))) {
            // the rubble of the stripped worlds; S-06: a black hole's like a neutron star's
            Belt bl;
            bl.innerKm = r0 * 1.22; bl.outerKm = r1 * 0.82;
            double mid = 0.5 * (bl.innerKm + bl.outerKm);
            bl.period = ORBIT_K * std::pow(mid, 1.5) / std::sqrt(s.massFactor);
            bl.phase0 = rng.range(0, TAU);
            bl.name = s.name + " belt " + romanNumeral((int)belts.size() + 1);
            belts.push_back(bl);
        }
    }
    if (!rogue && (!bodies.empty() || s.cls == STAR_NEUTRON || s.cls == STAR_PROTOSTAR || s.cls == STAR_WOLF_RAYET || s.cls == STAR_BLACK_HOLE) &&
            rng.chance(s.cls == STAR_NEUTRON || s.cls == STAR_BLACK_HOLE ? 0.75 : (s.cls == STAR_PROTOSTAR ? 0.95 : (s.cls == STAR_WOLF_RAYET ? 0.5 : 0.3)))) {
        Belt bl;
        double last = bodies.empty() ? ci.minFirstOrbitKm * 2.0 : bodies.back().orbitRadiusKm;   // a neutron star without worlds keeps its debris; S-04: a protostar's disc
        bl.innerKm = last * 1.5; bl.outerKm = last * 2.2;
        double mid = 0.5 * (bl.innerKm + bl.outerKm);
        bl.period = ORBIT_K * std::pow(mid, 1.5) / std::sqrt(s.massFactor);
        bl.phase0 = rng.range(0, TAU);
        bl.name = s.name + " belt " + romanNumeral((int)belts.size() + 1);
        belts.push_back(bl);
    }
    // moons
    int npl = (int)bodies.size();
    for (int n = 0; n < npl; n++) {
        const Rng rngAt = rng; const Body planetAt = bodies[n]; const size_t bodiesAt = bodies.size();   // to draw its moons again (below)
        Body p = bodies[n];   // copy: push_back below would invalidate a reference
        const KindDef& pt = KINDS[drawnKind(p)];   // the moons are drawn as the drawn kind's, in its number, and their orbits and
        const double pR = drawnR[n];               // periods kept round its drawn size (a people's moon keeps its days), pushed clear of a made kind's (`clear`)
        bool made = p.type != drawnKind(p);
        auto clear = [&](double orbitKm, double radii) { return made ? std::max(orbitKm, p.radiusKm * radii) : orbitKm; };
        int maxm = pt.maxMoons;
        bool giant = (pt.tags & KT_GIANT) != 0;
        int nm = maxm > 0 ? (rng.irange(maxm + 1) + rng.irange(maxm + 1)) / 2 : 0;
        if (giant) nm = 2 + rng.irange(maxm - 1);
        if (n == 0 && !giant) nm = std::min(nm, 1);
        if (rogue && p.type == PT_EUROPAN) nm = std::max(nm, 1);   // a europan drifter's ocean is kept by a moon's tides
        if ((int)bodies.size() + nm > 60) nm = 60 - (int)bodies.size();
        // about 7% of the solid mid-sized planets are double planets: a twin of 0.55-0.95 radii at 12-20 radii (B-428; 5-9 before), both locked to each other
        bool twin = (pt.tags & KT_TWIN) && rng.chance(0.07) && (int)bodies.size() < 59;
        if (twin && made && (lockedKind(p.type) || kindHas(p.type, KT_GIANT))) {   // a double planet keeps its face to its partner, not to its star
            // (its day is the twin's month, below): a world made a kind of the locked (the eyeball, the terminator) is made again, as drawn, as one
            // whose day is not its year; P-07: nor is a double planet's half a giant: one made a giant is made again by the facts it was made by,
            // with no giant (so it is what it would have been without them)
            Body& q = bodies[n];
            const Body& d = asDrawn[n];
            q.type = d.type; q.radiusKm = d.radiusKm; q.massEarths = d.massEarths; q.gravity = d.gravity; q.color = d.color; q.axialTilt = d.axialTilt; q.rings = d.rings;
            KindFacts f; f.drawn = q.type; f.tempK = q.tempK; f.starClass = s.cls; f.body = KB_PLANET; f.seed = q.seed; f.starSeed = s.seed;
            if (kindHas(p.type, KT_GIANT)) f = drawnFacts[n];
            f.paired = true;
            makeKind(q, f, drawnSize[n], 1e18, false);
            p = q; made = p.type != drawnKind(p);
        }
        if (twin) nm = std::max(nm, 1);
        p.moonCount = nm;
        bodies[n].moonCount = nm;
        // GEN 7: the first moon orbits at 4.5-6 radii, and outside the rings by a third of their outer radius. From
        // 2.8 radii the parent's disc was 42 degrees across (60% of the view), and a moon inside the rings (they reach
        // 4 radii) saw them fill the sky; from 4.5 radii the disc is 26 degrees, a wall of a planet still, and the rings
        // a band (M5-09 had 2.8-4.2 radii; 3.5-6.5 before that). The same random draw, so the rest of the system holds.
        // GEN 15: a solid world's first moon at 5-40 radii, even in the logarithm (most near 14). At 4.5-6 radii a world
        // and its moon hung side by side from the parked ship (3.5 radii, inside the moon's orbit), a twin 5-9 radii off beside
        // it: a system in a pocket. A giant's stays at 4.5-6 (the giant over its inner moons, as Jupiter over Io)
        const double u = rng.uni();
        double mkey = pR * std::max(giant ? 4.5 + 1.5 * u : 5.0 * std::pow(8.0, u), drawnRings[n] ? p.ringOuter * 1.35 : 0.0);
        double dkey = mkey, dPrev = 0;   // the drawn chain beside the pushed one
        mkey = clear(mkey, std::max(3.0, p.rings ? p.ringOuter * 1.35 : 0.0));   // three of the made world's radii, outside its rings
        double prevMoonR = 0;
        for (int c = 0; c < nm; c++) {
            if (c == 0 && twin) {
                Body m;
                m.index = (int)bodies.size();
                m.parent = n;
                m.seed = hashCombine(p.seed, 500);
                const double tw = pR * (12.0 + 8.0 * rng.uni());
                m.orbitRadiusKm = clear(tw, 3.0);   // as Charon from Pluto (5-9 radii before)
                m.tempK = p.tempK * (0.95 + 0.1 * rng.uni());
                if (rogue) m.heatK = m.tempK = 20.0 + 35.0 * unitFromHash(mix64(m.seed ^ 0x4EA7ULL));   // its own
                m.type = pickPlanetType(rng, m.tempK, s.cls, false, -1, 0.5);
                if (kindHas(m.type, KT_GIANT)) m.type = PT_ROCKY;
                if (rogue && m.type == PT_FELISIAN) m.type = PT_CHEMOTROPH;   // as a moon in the dark
                m.radiusKm = pR * (0.55 + 0.4 * rng.uni());
                double densityRel = KINDS[m.type].density;
                m.massEarths = std::pow(m.radiusKm / 6371.0, 3) * densityRel;
                m.gravity = 9.8 * m.massEarths / std::pow(m.radiusKm / 6371.0, 2);
                double pmass = std::pow(pR / 6371.0, 2.5) + std::pow(m.radiusKm / 6371.0, 2.5);
                m.orbitPeriod = MOON_K * std::pow(m.orbitRadiusKm, 1.5) / std::sqrt(std::max(0.05, pmass));
                m.orbitPhase0 = rng.range(0, TAU);
                m.orbitIncl = rng.sym(3 * DEG);
                m.orbitNode = rng.range(0, TAU);
                m.rotPeriod = m.orbitPeriod;
                bodies[n].rotPeriod = m.orbitPeriod;   // the planet too: they always show each other the same face
                m.rotPhase0 = rng.range(0, TAU);
                m.axialTilt = rng.range(0, 6 * DEG);
                m.axisAzimuth = rng.range(0, TAU);
                m.color = typeColor(m.type, rng);
                {
                    KindFacts f;
                    f.drawn = m.type;
                    f.tempK = m.tempK;
                    f.starClass = s.cls;
                    f.body = KB_TWIN;
                    f.seed = m.seed;
                    f.starSeed = s.seed;
                    f.parent = drawnKind(p);
                    makeKind(m, f, 0.5, 0, true);
                }   // a twin keeps its share of its partner's size
                m.name = p.name + "-" + (char)('a' + c);
                m.doublePlanet = true;
                bodies[n].doublePlanet = true;
                if (tw != m.orbitRadiusKm) m.drawnOrbitKm = tw;
                bodies.push_back(m); drawnOrbit.push_back(tw);
                prevMoonR = m.orbitRadiusKm; dPrev = tw;
                const double gk = 2.2 + 0.8 * rng.uni();
                mkey = m.orbitRadiusKm * gk; dkey = tw * gk;
                continue;
            }
            Body m;
            m.index = (int)bodies.size();
            m.parent = n;
            m.seed = hashCombine(p.seed, 500 + c);
            const double jit = 1.0 + rng.sym(0.05);
            m.orbitRadiusKm = mkey * jit;
            double dO = dkey * jit;
            // resonances: every other moon locks its period to the previous one (2:1 or 3:2)
            bool resonant = false;   // read by the strange moons' sources
            if (c >= 1 && rng.chance(0.45)) {
                const double f = std::pow(rng.chance(0.5) ? 2.0 : 1.5, 2.0 / 3.0);
                m.orbitRadiusKm = std::max(prevMoonR * 1.2, prevMoonR * f);
                dO = std::max(dPrev * 1.2, dPrev * f);
                resonant = true;
            }
            prevMoonR = m.orbitRadiusKm; dPrev = dO;
            m.tempK = p.tempK * (0.9 + 0.1 * rng.uni());
            if (rogue) m.tempK = 20.0 + 30.0 * unitFromHash(mix64(m.seed ^ 0x4EA7ULL));   // its own warmth, not its planet's sunlight
            if (giant && c < 2) m.tempK += 60.0 * (2 - c);   // tidal heating of the inner moons
            if (rogue) m.heatK = m.tempK;
            if (pt.tags & KT_GLOWS) m.tempK = std::pow(std::pow(m.tempK, 4) + std::pow(equilibriumTemp(p.luminosity, m.orbitRadiusKm), 4), 0.25);   // warmed by the parent's glow
            double sizeRand = rng.uni() * rng.uni();
            if (c == 0 && !giant && rng.chance(0.5)) sizeRand = 0.5 + 0.5 * rng.uni();   // half the first moons are big
            m.type = pickPlanetType(rng, m.tempK, s.cls, true, drawnKind(p), sizeRand);
            if (kindHas(m.type, KT_INNER_MOON) && c >= 2) m.type = PT_ICY;   // only the inner moons are heated enough (R-307: tectonic moons too)
            if (rogue && m.type == PT_FELISIAN) m.type = PT_CHEMOTROPH;   // no living world in the dark: a moon a brown dwarf warms lives by its own heat
            const KindDef& mt = KINDS[m.type];
            double maxR = giant ? 6500.0 : pR * 0.5;   // the largest moon may reach half the planet's radius (0.42 before)
            m.radiusKm = std::min(maxR, mt.minRadiusKm + (mt.maxRadiusKm - mt.minRadiusKm) * sizeRand);
            if (m.radiusKm < 600) m.radiusKm = 600 + 400 * rng.uni();
            double densityRel = mt.density;
            m.massEarths = std::pow(m.radiusKm / 6371.0, 3) * densityRel;
            m.gravity = 9.8 * m.massEarths / std::pow(m.radiusKm / 6371.0, 2);
            double pmass = std::pow(pR / 6371.0, 2.5) * (giant ? 0.6 : 1.0);
            m.orbitPeriod = MOON_K * std::pow(m.orbitRadiusKm, 1.5) / std::sqrt(std::max(0.05, pmass));
            m.orbitPhase0 = rng.range(0, TAU);
            m.orbitIncl = rng.sym(6 * DEG);
            { Rng er(m.seed ^ 0x0ECCULL); m.ecc = er.range(0, 0.04); m.argPeri = er.range(0, TAU); }
            m.orbitNode = rng.range(0, TAU);
            m.rotPeriod = rng.chance(0.7) ? m.orbitPeriod : 900 + 7000 * rng.uni();
            m.rotPhase0 = rng.range(0, TAU);
            m.axialTilt = rng.range(0, 8 * DEG);
            m.axisAzimuth = rng.range(0, TAU);
            m.rings = false;
            m.color = typeColor(m.type, rng);
            {   // P-01; P-03: its planet as drawn; P-06: what it is among its planet's moons, as drawn
                KindFacts f; f.drawn = m.type; f.tempK = m.tempK; f.starClass = s.cls; f.body = KB_MOON; f.seed = m.seed; f.starSeed = s.seed; f.parent = drawnKind(p);
                f.moon = (c < 2 ? MF_INNER : 0u) | (c >= 3 ? MF_OUTER : 0u) | (m.rotPeriod == m.orbitPeriod ? MF_FACING : MF_TURNING) | (resonant ? MF_RESONANT : 0u) |
                         (m.radiusKm >= 1500 ? MF_LARGE : 0u) | (m.radiusKm < 1100 ? MF_SMALL : 0u);
                makeKind(m, f, sizeRand, maxR, false);
            }
            m.name = p.name + "-" + (char)('a' + c);
            if (dO != m.orbitRadiusKm) m.drawnOrbitKm = dO;
            bodies.push_back(m); drawnOrbit.push_back(dO);
            double grow = c < 2 ? (giant ? 1.35 + 0.5 * rng.uni() : 1.6 + 0.8 * rng.uni()) : (1.25 + 0.3 * rng.uni());   // giants' inner moons closer together
            mkey = m.orbitRadiusKm * grow; dkey = dO * grow;
        }
        // captured moons: gas giants sometimes hold a small body far out on a tilted, retrograde orbit
        if (giant && rng.chance(0.18) && (int)bodies.size() < 60) {
            Body m;
            m.index = (int)bodies.size();
            m.parent = n;
            m.seed = hashCombine(p.seed, 590);
            const double cap = pR * (20 + 20 * rng.uni());
            m.orbitRadiusKm = clear(cap, 3.0);
            m.tempK = p.tempK;
            if (rogue) m.heatK = m.tempK = 20.0 + 25.0 * unitFromHash(mix64(m.seed ^ 0x4EA7ULL));   // its own
            m.type = rng.chance(0.5) ? PT_CRATERED : PT_ICY;
            m.radiusKm = 300 + 600 * rng.uni();
            double densityRel = KINDS[m.type].density;
            m.massEarths = std::pow(m.radiusKm / 6371.0, 3) * densityRel;
            m.gravity = 9.8 * m.massEarths / std::pow(m.radiusKm / 6371.0, 2);
            double pmass = std::pow(pR / 6371.0, 2.5) * 0.6;
            m.orbitPeriod = -MOON_K * std::pow(m.orbitRadiusKm, 1.5) / std::sqrt(std::max(0.05, pmass));   // retrograde
            m.orbitPhase0 = rng.range(0, TAU);
            m.orbitIncl = rng.range(30 * DEG, 60 * DEG);
            m.orbitNode = rng.range(0, TAU);
            m.rotPeriod = 900 + 7000 * rng.uni();
            m.rotPhase0 = rng.range(0, TAU);
            m.axialTilt = rng.range(0, 40 * DEG);
            m.axisAzimuth = rng.range(0, TAU);
            m.color = typeColor(m.type, rng);
            m.name = p.name + "-" + (char)('a' + nm);
            { Rng er(m.seed ^ 0x0ECCULL); m.ecc = er.range(0.1, 0.4); m.argPeri = er.range(0, TAU); }   // captured orbits are oval
            {   // the kind it is made (a captured moon's own: KB_CAPTURED, so no kind of P-01..P-04 takes one), its size draw from its radius's
                KindFacts f; f.drawn = m.type; f.tempK = m.tempK; f.starClass = s.cls; f.body = KB_CAPTURED; f.seed = m.seed; f.starSeed = s.seed; f.parent = drawnKind(p);
                makeKind(m, f, clampd((m.radiusKm - 300.0) / 600.0, 0, 1), 6500.0, false);
            }
            if (cap != m.orbitRadiusKm) m.drawnOrbitKm = cap;
            bodies.push_back(m); drawnOrbit.push_back(cap);
            bodies[n].moonCount++;
        }
        if (made && kindHas(bodies[n].type, KT_GIANT)) {   // a made giant never moves a people's moon (its month and its day, so its calendar, stay
            // where 1.3.2 put them): one that would is made again by the facts it was made by with no giant, and its moons drawn again (the same
            // draws, as the drawn kind's) in the room its drawn size gave them
            bool moved = false;
            for (size_t i = bodiesAt; i < bodies.size(); i++) if (bodies[i].drawnOrbitKm >= 0 && peoplesHash(bodies[i].type, bodies[i].seed)) moved = true;
            if (moved) {
                rng = rngAt; bodies.resize(bodiesAt); drawnOrbit.resize(bodiesAt); bodies[n] = planetAt;
                Body& q = bodies[n];
                const Body& d = asDrawn[n];
                q.type = d.type; q.radiusKm = d.radiusKm; q.massEarths = d.massEarths; q.gravity = d.gravity; q.color = d.color; q.axialTilt = d.axialTilt; q.rings = d.rings;
                KindFacts f = drawnFacts[n]; f.paired = true;
                makeKind(q, f, drawnSize[n], 1e18, false);
                n--;
                continue;
            }
        }
    }
    // comets: one or two per system on long, oval, tilted orbits that dive in past the planets (D-01: none round a drifter)
    if (!rogue) {
        Rng cr(s.seed ^ 0xC0E7ULL);
        int nc = cr.chance(0.45) ? 1 + cr.irange(2) : 0;
        double outer = bodies.empty() ? ci.minFirstOrbitKm * 12 : drawnOrbit.back();   // as drawn
        for (int k = 0; k < nc && (int)bodies.size() < 60; k++) {
            Body c;
            c.index = (int)bodies.size();
            c.parent = -1;
            c.type = PT_COMET;
            c.seed = hashCombine(s.seed, 700 + k);
            c.radiusKm = 2.0 + 10.0 * cr.uni();
            c.massEarths = std::pow(c.radiusKm / 6371.0, 3) * KINDS[PT_COMET].density;
            c.gravity = 9.8 * c.massEarths / std::pow(c.radiusKm / 6371.0, 2);
            c.ecc = 0.6 + 0.33 * cr.uni();
            c.orbitRadiusKm = std::max(outer * (0.35 + 1.2 * cr.uni()), s.radiusKm * 4.0 / (1 - c.ecc));
            c.orbitPeriod = ORBIT_K * std::pow(c.orbitRadiusKm, 1.5) / std::sqrt(s.massFactor);
            c.orbitPhase0 = cr.range(0, TAU);
            c.argPeri = cr.range(0, TAU);
            c.orbitIncl = cr.sym(40 * DEG);
            c.orbitNode = cr.range(0, TAU);
            c.rotPeriod = 3600 * (3 + 27 * cr.uni());
            c.rotPhase0 = cr.range(0, TAU);
            c.axialTilt = cr.range(0, 60 * DEG);
            c.axisAzimuth = cr.range(0, TAU);
            c.tempK = equilibriumTemp(s.luminosity, c.orbitRadiusKm);
            c.color = typeColor(PT_COMET, cr);
            c.name = s.name + " comet " + romanNumeral(k + 1);
            bodies.push_back(c); drawnOrbit.push_back(c.orbitRadiusKm);
        }
    }
    // companion stars: a second sun as a body, close in (the planets circle both) or far out with worlds of its own (D-01: no drifter)
    if (!rogue) {
        Rng kr(s.seed ^ 0xB1A2ULL);
        // the varieties' chances; S-03: the neutron star's; S-04: young stars come in pairs; S-05: most Wolf-Rayet stars have one; S-06:
        // half the black holes drink a companion
        static const double pMultiple[STAR_DRAWN_COUNT] = {0.18, 0.2, 0.28, 0.12, 0.24, 0.15, 0.22, 0.26, 0.14, 0.1, 0.2, 0.35, 0.4, 0.5};
        if (kr.chance(pMultiple[s.cls]) && (int)bodies.size() < 56) {
            // the families (a variety takes its share below); a black hole's companion is close, never a giant
            double w[STAR_DRAWN_COUNT] = {1.0, 1.6, (s.cls == STAR_BLUE_GIANT || s.cls == STAR_BLUE_WHITE || s.cls == STAR_WOLF_RAYET) ? 0.6 : 0.0, s.cls == STAR_BLACK_HOLE ? 0.0 : 0.15, 0.9, 0.05};
            int cls = starVariety(kr.pick(w, STAR_DRAWN_COUNT), galaxyRegion(s.sx, s.sy, s.sz), hashCombine(s.seed, 901));
            const StarClassInfo& cc = STAR_CLASSES[cls];
            Body k;
            k.index = (int)bodies.size();
            k.parent = -1;
            k.type = PT_COMPANION;
            k.starClass = cls;
            k.seed = hashCombine(s.seed, 900);
            k.radiusKm = cc.radiusKm * (1 + kr.sym(cc.radiusVar));
            k.luminosity = cc.luminosity * std::pow(k.radiusKm / cc.radiusKm, 2.0) * (0.7 + 0.6 * kr.uni());
            k.massEarths = cc.massFactor * 333000.0;
            k.gravity = 274.0 * cc.massFactor / std::pow(k.radiusKm / 7.0e5, 2);
            k.color = cc.color;
            k.tempK = 5000;
            bool worlds = !bodies.empty() && bodies[0].type != PT_COMET;
            double firstOrbit = worlds ? bodies[0].orbitRadiusKm : ci.minFirstOrbitKm;
            double closeR = (s.radiusKm + k.radiusKm) * (3.0 + 3.0 * kr.uni());
            // the worlds circle a close companion with the primary, and their warmth is the primary's alone (`pickPlanetType` read
            // that), so a close one is dim beside it and no wider than it in the first world's sky at its nearest; else it is far (a white
            // dwarf's yellow companion was 20-28 degrees across over worlds it would have scorched)
            double firstPeri = worlds ? firstOrbit * (1 - bodies[0].ecc) : firstOrbit, nearK = firstPeri - closeR * 1.1;
            bool modest = !worlds || (k.luminosity <= CLOSE_LIGHT * s.luminosity && nearK > 0 && k.radiusKm / nearK <= s.radiusKm / firstPeri);
            bool close = (kr.chance(0.35) && closeR < 0.4 * firstOrbit && modest) || s.cls == STAR_BLACK_HOLE;   // a black hole's companion is close, being drawn out
            k.orbitRadiusKm = close ? closeR : outerOrbitKm() * (2.5 + 5.0 * kr.uni());
            k.orbitPhase0 = kr.range(0, TAU);
            k.orbitIncl = kr.sym(close ? 3 * DEG : 25 * DEG);
            k.orbitNode = kr.range(0, TAU);
            { Rng er(k.seed ^ 0x0ECCULL); k.ecc = close ? er.range(0, 0.1) : er.range(0.05, 0.45); k.argPeri = er.range(0, TAU); }
            k.rotPeriod = 3600 * 24 * (5 + 20 * kr.uni());
            k.axialTilt = kr.range(0, 10 * DEG);
            k.axisAzimuth = kr.range(0, TAU);
            k.name = s.name + " B";
            companion = k.index;
            // its own planets when it orbits far out: up to three, lit by both suns
            int np2 = close ? 0 : kr.irange(4);
            if ((int)bodies.size() + np2 > 60) np2 = 0;
            k.moonCount = np2;
            double key2 = std::max(k.radiusKm * cc.firstOrbitMult, cc.minFirstOrbitKm) * (0.8 + 0.5 * kr.uni());
            if (!close) {
                // a far companion keeps its distance. Its orbit was drawn from the primary's widest one alone, so a red giant came within
                // its own radius of a red dwarf's worlds (60 degrees of their sky and more, and on the inner side of its orbit inside it). At its
                // closest approach it now stays far enough from the primary's farthest world (a planet's apoapsis and its widest moon) that it spans
                // COMPANION_DISC of that sky at most and its light alone would warm the world to COMPANION_HEAT_K at most; its own worlds keep
                // within a third of that gap (the widest its up to three can reach: the first at 1.08 of its key, each next 1.9 times out, e 0.1)
                // and see the primary no wider than COMPANION_DISC. The draws are as before: only an orbit under the floor moves out
                double reach = 0;
                for (const Body& b : bodies) {
                    if (b.type == PT_COMET) continue;
                    const Body& p = b.parent < 0 ? b : bodies[b.parent];
                    reach = std::max(reach, p.orbitRadiusKm * (1 + p.ecc) + (b.parent < 0 ? 0.0 : drawnOrbit[b.index] * (1 + b.ecc)));   // a moon's orbit as drawn
                }
                double own = np2 > 0 ? key2 * 1.08 * std::pow(1.9 * 1.08, np2 - 1) * 1.1 : 0;
                double view = k.radiusKm / std::sin(0.5 * COMPANION_DISC);
                double heat = AU_GAME_KM * (278.0 / COMPANION_HEAT_K) * (278.0 / COMPANION_HEAT_K) * std::sqrt(k.luminosity);
                double q = reach + std::max(std::max(view, heat), 3.0 * own);
                if (np2 > 0) q = std::max(q, own + s.radiusKm / std::sin(0.5 * COMPANION_DISC));
                k.orbitRadiusKm = std::max(k.orbitRadiusKm, q / (1 - k.ecc));
            }
            k.orbitPeriod = ORBIT_K * std::pow(k.orbitRadiusKm, 1.5) / std::sqrt(s.massFactor + cc.massFactor);
            bodies.push_back(k);
            for (int n = 0; n < np2; n++) {
                Body b;
                b.index = (int)bodies.size();
                b.parent = companion;
                b.seed = hashCombine(k.seed, 1000 + n);
                b.orbitRadiusKm = key2 * (1.0 + kr.sym(0.08));
                double tA = equilibriumTemp(k.luminosity, b.orbitRadiusKm), tB = equilibriumTemp(s.luminosity, k.orbitRadiusKm);
                b.tempK = std::pow(std::pow(tA, 4) + std::pow(tB, 4), 0.25);
                double sizeRand = kr.uni();
                b.type = pickPlanetType(kr, b.tempK, cls, false, -1, sizeRand);
                if (b.type == PT_SUBSTELLAR) b.type = PT_GASGIANT;
                const KindDef& pt = KINDS[b.type];
                b.radiusKm = pt.minRadiusKm + (pt.maxRadiusKm - pt.minRadiusKm) * sizeRand;
                double densityRel = pt.density;
                b.massEarths = std::pow(b.radiusKm / 6371.0, 3) * densityRel;
                b.gravity = 9.8 * b.massEarths / std::pow(b.radiusKm / 6371.0, 2);
                b.orbitPeriod = ORBIT_K * std::pow(b.orbitRadiusKm, 1.5) / std::sqrt(cc.massFactor);
                b.orbitPhase0 = kr.range(0, TAU);
                b.orbitIncl = kr.sym(4 * DEG);
                b.orbitNode = kr.range(0, TAU);
                b.rotPeriod = (n == 0 && kr.chance(0.5)) ? b.orbitPeriod : 1200 + 12000 * kr.uni() * kr.uni();
                b.rotPhase0 = kr.range(0, TAU);
                b.axialTilt = kr.range(0, 30 * DEG);
                b.axisAzimuth = kr.range(0, TAU);
                b.rings = (pt.tags & KT_GAS_GIANT) && kr.chance(0.5);
                b.ringInner = 1.35 + 0.6 * kr.uni();
                b.ringOuter = b.ringInner + 0.5 + 1.6 * kr.uni();
                b.color = typeColor(b.type, kr);
                { Rng er(b.seed ^ 0x0ECCULL); b.ecc = er.range(0, 0.1); b.argPeri = er.range(0, TAU); }
                {
                    KindFacts f;
                    f.drawn = b.type;
                    f.tempK = b.tempK;
                    f.starClass = cls;
                    f.body = KB_COMPANION_WORLD;
                    f.sync = b.rotPeriod == b.orbitPeriod;
                    f.seed = b.seed;
                    f.starSeed = s.seed;
                    makeKind(b, f, sizeRand, 1e18, false);
                }   // (never locked: `locked` is the primary's planets')
                b.name = s.name + " B " + romanNumeral(n + 1);
                bodies.push_back(b);
                key2 = b.orbitRadiusKm * (1.4 + 0.5 * kr.uni());
            }
        }
    }
    // a wanderer. One system in twenty-five caught a rogue world passing through it (Parker and Quanz 2012; Perets and Kouwenhoven 2012:
    // a few stars in a hundred, most in the cluster they were born in): an extra body after the system's own, drawn from a stream of its own
    // (no draw of the system's, so nothing else moves), its nearest 1.15-1.75 times the last planet's orbit out, eccentric (0.4-0.8), steep or
    // retrograde (25-155 degrees), no moons, clear of a far companion (none where it would break B-425's rule as the farthest world). Most are
    // the wanderer kind (an old world's ground under its air fallen as snow), one in five a living world that froze (the frozen felisian, never
    // round a black hole or in a Wolf-Rayet wind: no air keeps there, S-05 and S-06); none kept a people (the lost sun is the drifters' story)
    if (!rogue) {   // a drifter catches none
        const uint64_t wh = mix64(s.seed ^ WANDERER_SALT);
        if (unitFromHash(wh) < WANDERER_SHARE && (int)bodies.size() < 60) {
            Rng wr(wh);
            const double outer = np > 0 ? bodies[np - 1].orbitRadiusKm : ci.minFirstOrbitKm * 12;
            Body w;
            w.index = (int)bodies.size(); w.parent = -1; w.wanderer = true;
            w.seed = hashCombine(s.seed, 1900);
            w.ecc = 0.4 + 0.4 * wr.uni();
            w.orbitRadiusKm = outer * (1.15 + 0.6 * wr.uni()) / (1 - w.ecc);
            w.orbitIncl = (25 + 130 * wr.uni()) * DEG;   // past 90 degrees it goes round the wrong way
            w.orbitNode = wr.range(0, TAU); w.argPeri = wr.range(0, TAU); w.orbitPhase0 = wr.range(0, TAU);
            w.orbitPeriod = ORBIT_K * std::pow(w.orbitRadiusKm, 1.5) / std::sqrt(s.massFactor);
            w.tempK = equilibriumTemp(s.luminosity, w.orbitRadiusKm);
            const bool living = wr.uni() < WANDERER_LIVING && s.cls != STAR_BLACK_HOLE && s.cls != STAR_WOLF_RAYET;
            w.type = living ? PT_FROZEN_FELISIAN : PT_WANDERER;
            const KindDef& K = KINDS[w.type];
            w.radiusKm = K.minRadiusKm + (K.maxRadiusKm - K.minRadiusKm) * wr.uni();
            w.massEarths = std::pow(w.radiusKm / 6371.0, 3) * K.density;
            w.gravity = 9.8 * w.massEarths / std::pow(w.radiusKm / 6371.0, 2);
            w.rotPeriod = (1200 + 12000 * wr.uni() * wr.uni()) * (wr.uni() < 0.25 ? -1 : 1);
            w.rotPhase0 = wr.range(0, TAU);
            w.axialTilt = wr.range(0, 90 * DEG);
            w.axisAzimuth = wr.range(0, TAU);
            w.rings = wr.uni() < K.ringChance;
            w.ringInner = 1.35 + 0.6 * wr.uni(); w.ringOuter = w.ringInner + 0.5 + 1.6 * wr.uni();
            w.color = madeColour(w.type, w.seed);
            if (living) w.thenTempK = 265.0 + 35.0 * wr.uni();   // the living climate it had (`BodyGen::make`'s then-world)
            w.name = s.name + " " + romanNumeral(np + 1);
            bool clear = true;   // B-425's rule for a far companion with the wanderer the primary's farthest world: none where it would break it
            if (companion >= 0) {
                const Body& k = bodies[companion];
                const bool closeK = np > 0 && k.orbitRadiusKm < bodies[0].orbitRadiusKm;
                if (!closeK) {
                    double own = 0; for (const Body& b : bodies) if (b.parent == companion) own = std::max(own, b.orbitRadiusKm * (1 + b.ecc));
                    const double view = k.radiusKm / std::sin(0.5 * COMPANION_DISC), heat = AU_GAME_KM * (278.0 / COMPANION_HEAT_K) * (278.0 / COMPANION_HEAT_K) * std::sqrt(k.luminosity);
                    clear = k.orbitRadiusKm * (1 - k.ecc) - w.orbitRadiusKm * (1 + w.ecc) >= std::max(std::max(view, heat), 3.0 * own) * 1.01;
                }
            }
            if (clear) bodies.push_back(w);
        }
    }
    // spin frames
    for (Body& b : bodies) {
        Vec3 up(0, 1, 0);
        Vec3 tiltDir(std::cos(b.axisAzimuth), 0, std::sin(b.axisAzimuth));
        b.spinAxis = normalize(up * std::cos(b.axialTilt) + tiltDir * std::sin(b.axialTilt));
        Vec3 any = std::fabs(b.spinAxis.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        b.ref0 = normalize(cross(any, b.spinAxis));
        b.ref1 = cross(b.spinAxis, b.ref0);
    }
    // planets whose day equals their year keep one face toward their sun; remember that face
    for (int i = 0; i < (int)bodies.size(); i++) {
        Body& b = bodies[i];
        if (b.parent >= 0 || b.type == PT_COMET || b.type == PT_COMPANION || b.ecc > 0.05) continue;
        if (std::fabs(1.0 - b.rotPeriod / b.orbitPeriod) > 0.02) continue;
        b.locked = true;
        b.lockedDir = bodyFrame(i, 0) * normalize(star.pos - bodyPos(i, 0));
    }
    // the two-faced moon faces its planet (its day its month), so it travels with one face forward: remember that face as the locked face
    // is (its dark one; the only reader, so the only moons that hold it)
    for (int i = 0; i < (int)bodies.size(); i++) {
        Body& b = bodies[i];
        if (b.type != PT_TWO_FACED || b.parent < 0 || b.rotPeriod != b.orbitPeriod) continue;
        double rOverA;   // its direction of motion round its planet at t = 0 from the orbit's elements (`bodyPos`'s; within e of the velocity's)
        const double ph = (b.ecc > 1e-9 ? trueAnomaly(std::fmod(b.orbitPhase0, TAU), b.ecc, rOverA) : b.orbitPhase0) + b.argPeri;
        const Vec3 u(std::cos(b.orbitNode), 0, std::sin(b.orbitNode)), v = cross(Vec3(0, 1, 0), u) * std::cos(b.orbitIncl) + Vec3(0, 1, 0) * std::sin(b.orbitIncl);
        b.apexDir = bodyFrame(i, 0) * normalize(v * std::cos(ph) - u * std::sin(ph));
    }
    for (Body& b : bodies) { b.starSx = star.sx; b.starSy = star.sy; b.starSz = star.sz; b.starKnown = true; b.drifter = rogue; }   // T-02; D-01
    for (Body& b : bodies) b.tideForcingM = tideForcingM(*this, b.index);   // a derived fact (no draw), the trait's to read
}

// O3: belt rocks. The belt is cut into rings of BELT_CELL_KM in radius; each ring turns as one at the Kepler rate of its
// middle radius (so rings shear against each other, the inner ones outrunning the outer, but a ring keeps its
// formation), and is cut into arc cells of BELT_CELL_KM round the star and height cells. A rock is a hash of its cell,
// so it keeps its identity at any time.
void StarSystem::beltCellOf(int k, const Vec3& posKm, double t, int64_t& ir, int64_t& ia, int64_t& iy) const {
    Vec3 rel = posKm - star.pos;
    double r = std::sqrt(rel.x * rel.x + rel.z * rel.z);
    ir = (int64_t)std::floor(r / BELT_CELL_KM);
    ia = beltArcCell(k, ir, std::atan2(rel.z, rel.x), t);
    iy = (int64_t)std::floor(rel.y / BELT_CELL_KM);
}

int64_t StarSystem::beltArcCell(int k, int64_t ir, double angWorld, double t) const {
    const Belt& bl = belts[k];
    double rRing = (ir + 0.5) * BELT_CELL_KM;
    int64_t n = std::max<int64_t>(1, (int64_t)std::llround(TAU * rRing / BELT_CELL_KM));
    double a = wrap2pi(angWorld - bl.rateAt(rRing) * t);   // the co-rotating angle of this ring
    int64_t ia = (int64_t)std::floor(a * rRing / BELT_CELL_KM);
    return ((ia % n) + n) % n;
}

bool StarSystem::beltRockAt(int k, int64_t ir, int64_t ia, int64_t iy, int m, double t, BeltRock& out) const {
    if (k < 0 || k >= (int)belts.size() || m < 0 || m >= BELT_ROCKS_PER_CELL) return false;
    const Belt& bl = belts[k];
    double r0 = ir * BELT_CELL_KM;
    if (r0 + BELT_CELL_KM < bl.innerKm || r0 > bl.outerKm) return false;
    double rRing = r0 + 0.5 * BELT_CELL_KM;
    int64_t n = std::max<int64_t>(1, (int64_t)std::llround(TAU * rRing / BELT_CELL_KM));
    ia = ((ia % n) + n) % n;
    uint64_t h = hash3i(ir, ia, iy, star.seed ^ (0xBE17ULL + (uint64_t)k * 0x9E37ULL));
    double thick = 0.015 * (r0 + 0.5 * BELT_CELL_KM);   // the belt is thin: |y| within 1.5% of the radius
    if (std::fabs((iy + 0.5) * BELT_CELL_KM) > thick + BELT_CELL_KM) return false;
    int nr = 1 + (int)(unitFromHash(h) * BELT_ROCKS_PER_CELL);   // 1..3 rocks
    double edge = smoothstep(0.0, 0.12, (r0 - bl.innerKm) / (bl.outerKm - bl.innerKm)) * smoothstep(1.0, 0.88, (r0 - bl.innerKm) / (bl.outerKm - bl.innerKm));
    if (unitFromHash(mix64(h + 99)) > 0.35 + 0.65 * edge) nr = std::max(1, nr - 2);   // sparser toward the edges
    if (m >= nr) return false;
    uint64_t hr = mix64(h + 17 * (m + 1));
    double u0 = unitFromHash(hr), u1 = unitFromHash(mix64(hr + 1)), u2 = unitFromHash(mix64(hr + 2)), u3 = unitFromHash(mix64(hr + 3));
    double r = r0 + u0 * BELT_CELL_KM;
    double y = (iy + u2) * BELT_CELL_KM;
    if (std::fabs(y) > thick) return false;
    double ang = (ia + u1) * BELT_CELL_KM / rRing + bl.rateAt(rRing) * t;   // the ring's own rate: the cell mapping stays exact
    out.pos = star.pos + Vec3(std::cos(ang) * r, y, std::sin(ang) * r);
    out.radiusKm = 0.25 + 12.0 * std::pow(u3, 4.0) + (u3 > 0.9995 ? 25.0 : 0.0);   // most under a kilometre, the biggest in reach about 10 km, one in two thousand a 35 km mountain
    out.seed = hr;
    double a1 = unitFromHash(mix64(hr + 4)) * TAU, a2 = unitFromHash(mix64(hr + 5)) * PI;
    out.axis = Vec3(std::cos(a1) * std::sin(a2), std::cos(a2), std::sin(a1) * std::sin(a2));
    out.tumble = unitFromHash(mix64(hr + 6)) * TAU + t * TAU / (1800.0 + 7200.0 * unitFromHash(mix64(hr + 7)));   // one turn in half an hour to two and a half
    out.ir = ir; out.ia = ia; out.iy = iy; out.m = m;
    return true;
}

// O0-01 (M5-04): the ring profile, hashed per body, with gaps carved by shepherd moons inside the ring
std::vector<float> StarSystem::ringProfileOf(int bi) const {
    const Body& b = bodies[bi];
    std::vector<float> prof(256);
    Rng rng(b.seed ^ 0x51465ULL);
    uint64_t sd = rng.next();
    for (int i = 0; i < 256; i++) {
        double x = i / 256.0;
        double n = 0.5 + 0.5 * fbm2(x * 9.0, 0.37, sd, 4);
        double gaps = std::pow(0.5 + 0.5 * std::sin(x * 40.0 + 3.0 * gnoise2(x * 5, 1.5, sd + 9)), 6.0);
        double v = clampd(n * (1.0 - 0.7 * gaps), 0, 1);
        v *= smoothstep(0.0, 0.06, x) * smoothstep(1.0, 0.85, x);
        for (int j = 0; j < (int)bodies.size(); j++) {
            const Body& mo = bodies[j];
            if (mo.parent != bi) continue;
            double rr = mo.orbitRadiusKm / b.radiusKm;
            if (rr < b.ringInner || rr > b.ringOuter) continue;
            double u = (rr - b.ringInner) / (b.ringOuter - b.ringInner);
            v *= 1 - 0.92 * std::exp(-((x - u) * (x - u)) / (0.018 * 0.018));
        }
        prof[i] = (float)v;
    }
    return prof;
}

double StarSystem::outerOrbitKm() const {
    double r = 0;
    for (const Body& b : bodies) if (b.parent < 0 && b.type != PT_COMET) r = std::max(r, b.orbitRadiusKm);
    return r > 0 ? r : STAR_CLASSES[star.cls].minFirstOrbitKm * 12;
}

double StarSystem::trueAnomaly(double M, double e, double& rOverA) {
    if (e < 1e-9) { rOverA = 1; return M; }
    double E = M + e * std::sin(M);
    for (int i = 0; i < 12; i++) {
        double f = E - e * std::sin(E) - M, fp = 1 - e * std::cos(E);
        double d = f / fp;
        E -= d;
        if (std::fabs(d) < 1e-12) break;
    }
    rOverA = 1 - e * std::cos(E);
    return 2 * std::atan2(std::sqrt(1 + e) * std::sin(E * 0.5), std::sqrt(1 - e) * std::cos(E * 0.5));
}

std::string StarSystem::classString() const {
    if (sunless()) return "NO STAR";   // D-01: a drifter's place has no class to show
    if (companion >= 0 && companion < (int)bodies.size())
        return std::string(STAR_CLASSES[star.cls].code) + "+" + STAR_CLASSES[bodies[companion].starClass].code + " MULTIPLE";
    return std::string(STAR_CLASSES[star.cls].code) + " " + STAR_CLASSES[star.cls].name;
}

Star StarSystem::companionStar() const {
    Star k;
    if (companion < 0 || companion >= (int)bodies.size()) return k;
    const Body& b = bodies[companion];
    k.cls = b.starClass; k.radiusKm = b.radiusKm; k.luminosity = b.luminosity; k.color = b.color; k.seed = b.seed;
    k.massFactor = STAR_CLASSES[b.starClass].massFactor; k.name = b.name; k.valid = true;
    k.pulseHz = b.starClass == STAR_PULSAR ? 0.7 : 0;
    return k;
}

double StarSystem::starLightFactor(double luminosity, double distKm, double floor) {
    double f = std::pow(luminosity * (AU_GAME_KM / distKm) * (AU_GAME_KM / distKm), 0.25);
    return clampd(f, floor, 1.15);
}

bool StarSystem::ofCompanion(int i) const {
    if (companion < 0) return false;
    for (int n = 0; i >= 0 && i < (int)bodies.size() && n < 4; n++) { if (bodies[i].parent == companion) return true; i = bodies[i].parent; }
    return false;
}

StarSystem::Light StarSystem::lightAt(const Vec3& posKm, double t) const {
    Light l;
    double d1 = std::max(length(star.pos - posKm), 1.0);
    l.dir = (star.pos - posKm) / d1;
    if (sunless()) { l.dir = normalize(star.pos - posKm); l.factor = 0; l.fromCompanion = false; return l; }   // D-01: no sun, no light (the floor is a sun's; on the anchor a unit way all the same)
    double raw1 = star.luminosity / (d1 * d1);
    l.factor = starLightFactor(star.luminosity, d1);
    l.fromCompanion = false;
    if (companion >= 0 && companion < (int)bodies.size()) {
        Vec3 cp = bodyPos(companion, t);
        double d2 = std::max(length(cp - posKm), 1.0);
        double raw2 = bodies[companion].luminosity / (d2 * d2);
        if (raw2 > raw1) { l.dir = (cp - posKm) / d2; l.factor = starLightFactor(bodies[companion].luminosity, d2); l.fromCompanion = true; }
    }
    return l;
}

Vec3 StarSystem::parentPos(int i, double t) const {
    const Body& b = bodies[i];
    if (b.parent < 0) return star.pos;
    return bodyPos(b.parent, t);
}

Vec3 StarSystem::bodyPos(int i, double t) const {
    const Body& b = bodies[i];
    double M = b.orbitPhase0 + TAU * t / b.orbitPeriod;
    double rOverA = 1;
    double nu = b.ecc > 1e-9 ? trueAnomaly(std::fmod(M, TAU), b.ecc, rOverA) : M;   // M5-03 Kepler orbits
    double ph = nu + b.argPeri;
    Vec3 u(std::cos(b.orbitNode), 0, std::sin(b.orbitNode));
    Vec3 v0 = cross(Vec3(0, 1, 0), u);
    Vec3 v = v0 * std::cos(b.orbitIncl) + Vec3(0, 1, 0) * std::sin(b.orbitIncl);
    Vec3 rel = (u * std::cos(ph) + v * std::sin(ph)) * (b.orbitRadiusKm * rOverA);
    return parentPos(i, t) + rel;
}

Vec3 StarSystem::bodyVel(int i, double t) const {
    double h = std::max(1.0, std::fabs(bodies[i].orbitPeriod) * 1e-4);
    return (bodyPos(i, t + h) - bodyPos(i, t - h)) / (2 * h);
}

double StarSystem::meanAnomaly(int i, double t) const {
    const Body& b = bodies[i];
    double M = std::fmod(b.orbitPhase0 + TAU * t / b.orbitPeriod, TAU);
    return M < 0 ? M + TAU : M;
}

double StarSystem::rotationAngle(int i, double t) const {
    const Body& b = bodies[i];
    return b.rotPhase0 + TAU * t / b.rotPeriod;
}

Mat3 StarSystem::bodyFrame(int i, double t) const {
    const Body& b = bodies[i];
    double th = rotationAngle(i, t);
    Vec3 E0 = b.ref0 * std::cos(th) + b.ref1 * std::sin(th);
    Vec3 E1 = b.ref0 * -std::sin(th) + b.ref1 * std::cos(th);
    return Mat3::fromRows(E0, E1, b.spinAxis);
}

Vec3 StarSystem::bodyFromLatLon(double lat, double lon) {
    return Vec3(std::cos(lat) * std::cos(lon), std::cos(lat) * std::sin(lon), std::sin(lat));
}
void StarSystem::latLonFromBody(const Vec3& b, double& lat, double& lon) {
    lat = std::asin(clampd(b.z, -1, 1));
    lon = std::atan2(b.y, b.x);
}

Vec3 StarSystem::surfacePointWorld(int i, double t, double latRad, double lonRad, double altitudeKm) const {
    Mat3 f = bodyFrame(i, t);
    Vec3 bv = bodyFromLatLon(latRad, lonRad);
    Vec3 w = f.transposed() * bv;
    return bodyPos(i, t) + w * (bodies[i].radiusKm + altitudeKm);
}

double StarSystem::seasonOf(int i, double t) const {
    if (sunless()) return 0;   // D-01: no sun, no season
    const Body& b = bodies[i];
    Vec3 toStar = normalize(parentPos(i, t) - bodyPos(i, t));
    if (b.parent >= 0) toStar = normalize(star.pos - bodyPos(i, t));
    return dot(b.spinAxis, toStar);   // the north pole leaning toward the sun: northern summer
}

std::string StarSystem::bodyLabel(int i) const {
    const Body& b = bodies[i];
    if (b.type == PT_COMPANION) return b.name + " (COMPANION STAR, " + STAR_CLASSES[b.starClass].name + ")";
    if (b.doublePlanet && b.parent >= 0) return b.name + " (" + KINDS[b.type].name + ", twin planet)";
    return b.name + " (" + KINDS[b.type].name + (b.parent >= 0 ? (bodies[b.parent].type == PT_COMPANION ? ", of the companion)" : ", moon)") : ")");
}

// R-402: the magnetic field, the star's storms and the aurora they make
const char* const MAGNETIC_CLASS_NAMES[4] = {"NONE", "WEAK", "MODERATE", "STRONG"};

bool isIceGiant(const Body& b) { return kindHas(b.type, KT_GAS_GIANT) && KINDS[b.type].giantClass == GC_NONE && b.radiusKm < 45000 && b.color.b > b.color.r + 0.05f; }   // P-07: a drawn gas giant's

double cometRefKm(const StarSystem& sys) {
    return sys.bodies.empty() ? AU_GAME_KM : std::max(AU_GAME_KM * 0.5, sys.bodies[0].type == PT_COMET ? AU_GAME_KM : sys.bodies[0].orbitRadiusKm);
}

double cometActivityAt(const StarSystem& sys, double distKm) {
    double refKm = cometRefKm(sys);
    return clampd((refKm * 1.6 / distKm) * (refKm * 1.6 / distKm), 0, 1);
}

double magneticField(const Body& b) {
    double u = unitFromHash(hashCombine(b.seed, 0x3A6F));
    double hours = std::fabs(b.rotPeriod) / 3600.0;
    double spin = clampd(1.4 - 0.8 * std::log10(std::max(hours, 1.0)), 0, 1);   // 1 under 3 h, 0.6 at 10 h, 0.3 at a day, 0 past 56 h
    double size = clampd((b.radiusKm - 1500) / 5000.0, 0, 1);
    return clampd(0.9 * u + 0.3 * spin + 0.25 * size - 0.35 - (b.locked ? 0.3 : 0.0), 0, 1);
}

int magneticClass(double mag) { return mag < 0.08 ? 0 : (mag < 0.3 ? 1 : (mag < 0.6 ? 2 : 3)); }

double auroralStorm(const Star& s, double t) {
    double slow = gnoise2(t / 2.6e5, 1.7, s.seed ^ 0xA5A5), fast = gnoise2(t / 1.5e4, 4.1, s.seed ^ 0x5A5A);   // three days; four hours
    double storm = clampd((0.5 + 0.5 * slow + 0.2 * fast - 0.58) / 0.3, 0, 1);
    return s.cls == STAR_RED_DWARF ? std::max(storm, 0.8 * starFlare(s, t)) : storm;   // S-01: a red dwarf's flare is a storm of its own
}

double starActivity(int cls) {   // S-01: flare stars and hot stars drive the nights; S-03: the neutron star's x-rays nearly as much as the pulsar's beams
    switch (cls) {
        case STAR_BLUE_GIANT: return 1.0;
        case STAR_PULSAR: return 0.9;
        case STAR_NEUTRON: return 0.85;
        case STAR_PROTOSTAR: return 0.9;   // a young star's wind and flares
        case STAR_WOLF_RAYET: return 1.0;   // the wind that strips its worlds
        case STAR_BLACK_HOLE: return 0.9;   // the disc's x-rays
        case STAR_RED_DWARF: return 0.8;
        case STAR_BLUE_WHITE: return 0.7;
        case STAR_ORANGE: return 0.55;
        case STAR_YELLOW: return 0.45;
        default: return 0.2;
    }
}

double auroraPotentialAt(const StarSystem& sys, const Body& b, double storm) {
    if (!bodyHasAir(b) || hasOpaqueDeck(b.type)) return 0;   // D-04: no air left to light
    double activity = starActivity(sys.star.cls);   // the one table
    double mag = magneticField(b);
    double steady = 0.6 * smoothstep(0.5, 1.0, mag);                                            // a strong field keeps a glow every night
    double driven = std::pow(storm, 0.7) * smoothstep(0.03, 0.4, mag) * (0.7 + 0.3 * mag);     // the storm lights the rest
    return activity * clampd(steady + driven, 0, 1);
}

double auroraPotential(const StarSystem& sys, const Body& b, double t) { return auroraPotentialAt(sys, b, auroralStorm(sys.star, t)); }

// Q-02: the tides
int tidePulls(const StarSystem& sys, int i, TidePull* out, int maxOut) {
    const Body& b = sys.bodies[i];
    if (!KINDS[b.type].landable || b.type == PT_COMET) return 0;
    const double M = std::max(1e-9, b.massEarths), R4 = std::pow(b.radiusKm, 4);
    auto eq = [&](double m, double dKm) { return dKm > 1 ? m / M * R4 / (dKm * dKm * dKm) * 1000.0 : 0.0; };
    auto turns = [](double rotS, double orbitS) { return std::fabs(std::fabs(rotS) - std::fabs(orbitS)) > 1e-6 * std::fabs(orbitS); };   // its face not kept toward it
    int n = 0;
    auto add = [&](int body, double f) { if (f > 0 && n < maxOut) { out[n].body = body; out[n].forcingM = f; n++; } };
    if (b.parent < 0 || sys.bodies[b.parent].type == PT_COMPANION) {   // a planet (of the primary or the companion): its sun and its moons
        const double sunMass = b.parent < 0 ? sys.star.massFactor * 333000.0 : sys.bodies[b.parent].massEarths;
        if (!b.locked) add(b.parent < 0 ? -1 : b.parent, eq(sunMass, b.orbitRadiusKm));
        for (const Body& m : sys.bodies) if (m.parent == i && turns(b.rotPeriod, m.orbitPeriod)) add(m.index, eq(m.massEarths, m.orbitRadiusKm));
    } else {   // a moon: its planet (unless it faces it) and its planet's sun
        const Body& p = sys.bodies[b.parent];
        if (turns(b.rotPeriod, b.orbitPeriod)) add(b.parent, eq(p.massEarths, b.orbitRadiusKm));
        const double sunMass = p.parent < 0 ? sys.star.massFactor * 333000.0 : sys.bodies[p.parent].massEarths;
        add(p.parent < 0 ? -1 : p.parent, eq(sunMass, p.orbitRadiusKm));
    }
    double F = 0; for (int k = 0; k < n; k++) F += out[k].forcingM;
    for (int k = 0; k < n; k++) out[k].share = F > 0 ? out[k].forcingM / F : 0;
    return n;
}

double tideForcingM(const StarSystem& sys, int i) { TidePull p[16]; const int n = tidePulls(sys, i, p, 16); double F = 0; for (int k = 0; k < n; k++) F += p[k].forcingM; return F; }

double tideRangeOf(double forcingM) { return forcingM < TIDE_FORCING_MIN ? 0.0 : clampd(5.0 + 4.0 * std::log10(forcingM / TIDE_FORCING_MIN), 5.0, 16.0); }

double tideOffsetM(const StarSystem& sys, int i, const Vec3& unitBody, double t, double rangeM) {
    if (rangeM <= 0) return 0;
    TidePull p[16]; const int n = tidePulls(sys, i, p, 16);
    const Mat3 frame = sys.bodyFrame(i, t);
    const Vec3 at = sys.bodyPos(i, t);
    const double sinPhi2 = unitBody.z * unitBody.z, cosPhi2 = 1 - sinPhi2;
    double eta = 0;
    for (int k = 0; k < n; k++) {
        const Vec3 to = p[k].body < 0 ? sys.star.pos : sys.bodyPos(p[k].body, t);
        const Vec3 d = frame * normalize(to - at);   // the pull's direction in the body's frame
        const double c = dot(unitBody, d), sinDel2 = d.z * d.z;
        eta += p[k].share * (c * c - (sinPhi2 * sinDel2 + 0.5 * cosPhi2 * (1 - sinDel2)));
    }
    return rangeM * eta;
}

// D-05: the system a drifter left and its people's world in it (see system.h)
namespace {
std::mutex g_formerMutex;
std::list<std::pair<uint64_t, std::shared_ptr<const StarSystem>>> g_former;   // the last eight, the newest first
}
std::shared_ptr<const StarSystem> formerSystemOf(const StarSystem& dark) {
    if (!dark.sunless() || dark.bodies.empty()) return nullptr;
    const uint64_t key = dark.star.seed;
    {
        std::lock_guard<std::mutex> lk(g_formerMutex);
        for (auto it = g_former.begin(); it != g_former.end(); ++it)
            if (it->first == key) { auto hit = *it; g_former.erase(it); g_former.push_front(hit); return hit.second; }
    }
    std::shared_ptr<StarSystem> F;
    const DrifterPast past = drifterPast(dark.star);
    if (past.found) {
        F = std::make_shared<StarSystem>();
        F->generate(past.star);
        const Body& d = dark.bodies[0];
        const Star& S = F->star;
        Rng fr(d.seed ^ 0xD05F0ULL);   // its former orbit's own draws
        Body w = d;
        w.index = (int)F->bodies.size(); w.parent = -1; w.former = true; w.heatK = 0;
        w.tempK = d.thenTempK > 0 ? d.thenTempK : 288.0;
        // where the star's light gave that warmth (`equilibriumTemp` turned round), its year by Kepler round the star's mass
        w.orbitRadiusKm = AU_GAME_KM * std::pow(278.0 * std::pow(std::max(S.luminosity, 1e-6), 0.25) / w.tempK, 2);
        w.orbitPeriod = ORBIT_K * std::pow(w.orbitRadiusKm, 1.5) / std::sqrt(std::max(S.massFactor, 1e-3));
        w.ecc = fr.range(0, 0.06); w.argPeri = fr.range(0, TAU);
        w.orbitPhase0 = fr.range(0, TAU); w.orbitIncl = fr.range(-2, 2) * DEG; w.orbitNode = fr.range(0, TAU);
        w.locked = false; w.lockedDir = Vec3();
        w.starSx = d.starSx; w.starSy = d.starSy; w.starSz = d.starSz; w.starKnown = d.starKnown;   // its generator finds the drifter it is now
        const int at = w.index;
        F->bodies.push_back(w);
        for (const Body& m0 : dark.bodies) {   // the moons that came with it
            if (m0.parent != 0) continue;
            Body m = m0;
            m.index = (int)F->bodies.size(); m.parent = at; m.former = true; m.heatK = 0;
            m.tempK = equilibriumTemp(S.luminosity, w.orbitRadiusKm);
            F->bodies.push_back(m);
        }
    }
    std::lock_guard<std::mutex> lk(g_formerMutex);
    g_former.push_front({key, F});
    if (g_former.size() > 8) g_former.pop_back();
    return F;
}
PeoplesWorld peoplesWorldOf(const StarSystem& sys, const Body& b) {
    PeoplesWorld W; W.sys = &sys; W.b = &b;
    if (b.former || !drifterHadPeople(b) || !sys.sunless()) return W;
    std::shared_ptr<const StarSystem> F = formerSystemOf(sys);
    if (!F) return W;
    for (const Body& o : F->bodies) if (o.former && o.parent < 0 && o.seed == b.seed) { W.sys = F.get(); W.b = &o; W.hold = F; return W; }
    return W;
}
