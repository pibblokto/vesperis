// Z-03: the search (docs/ideas/ITERATION-1.5.0.md, Z-03; search_state.h): a vial of the ground at the crosshair (O on foot), the capsule's
// scoop in a cloud world's descent, the kit's vials to the cold store at the climb aboard, their save lines
#include "game.h"
#include "ui.h"
#include <cmath>
#include <ostream>
#include <sstream>

// the take's line: its name, the temperature read, the activity; 52 characters at most (the word ACTIVITY left out where it has no room)
std::string Game::vialLine(const Vial& v) const {
    const std::string head = fmt("%s, %.0f C", v.name.c_str(), std::fabs(v.readingC) < 0.5 ? 0.0 : v.readingC);
    const std::string s = head + " - ACTIVITY " + VIAL_ACTIVITY_NAMES[v.activity];
    return s.size() <= 52 ? s : head + " - " + VIAL_ACTIVITY_NAMES[v.activity];
}

// O on foot: the crosshair's spot within 2 m of the feet (a swimmer's water at hand), refused from a vehicle and with the vials used up (R-432:
// the cold store aboard takes any number)
void Game::takeSample() {
    if (surf.inVehicle()) { refuse(surf.inBuggy ? "NOT FROM THE BUGGY - GET OUT TO TAKE A SAMPLE" : "NOT FROM THE DRONE - LAND AND GET OUT"); return; }
    if ((int)kit.size() >= KIT_VIALS) { refuse("EIGHT VIALS ARE FULL - STORE THEM ABOARD"); return; }
    double x = surf.player.x, z = surf.player.z; bool water = true;
    if (!surf.player.swimming) {
        if (surf.lastRange < 0 || std::hypot(surf.lastRangeX - surf.player.x, surf.lastRangeZ - surf.player.z) > 2.0) { refuse("LOOK AT GROUND WITHIN 2 M TO TAKE A SAMPLE"); return; }
        x = surf.lastRangeX; z = surf.lastRangeZ; water = surf.lastRangeWater;
    }
    const SampleRead r = surf.readSample(x, z, water);
    if (!r.ok) { refuse(r.refusal ? r.refusal : "NOTHING TO TAKE HERE"); return; }
    Vial v;
    v.sx = sys.star.sx; v.sy = sys.star.sy; v.sz = sys.star.sz; v.body = surf.site.body;
    v.unit = r.unit; v.hab = r.hab; v.name = r.name; v.readingC = r.readingC; v.t = t; v.clock = guide.clock;
    v.activity = sampleOf(surf.biota(), r.hab).activity;
    kit.push_back(v);
    sampleTaken(v, "");
    status(vialLine(v), 6);
    audio.beep = BEEP_UI;
}

// Z-07: a sample taken: counted, its world among those sampled, logged (SAMPLE: what it is, where, its reading)
void Game::sampleTaken(const Vial& v, const char* how) {
    guide.samplesTaken++;
    guide.sampled.insert(Guide::bodyKey(v.sx, v.sy, v.sz, v.body));
    logEvent("SAMPLE", fmt("%s%s ON %s, %.0f C - ACTIVITY %s", how, v.name.c_str(), upper(bodyNameOf(v.body)).c_str(), std::fabs(v.readingC) < 0.5 ? 0.0 : v.readingC,
                           VIAL_ACTIVITY_NAMES[v.activity]));
}

// the capsule's scoop: on a world whose life lives in its clouds (the venusian's, the acid world's), the descent takes their air at the height
// where the column is temperate (35 C, a lapse of 8 C a kilometre from the ground's mean: Venus' 50-60 km, Seager et al. 2021), or over a
// cooler ground at the clouds' base, 3 km up and 24 C colder; one vial
void Game::scoopClouds(double groundC) {
    scooped = true;
    auto line = [&](const std::string& s) { status(s, 6); scoopLine = s; scoopUntil = realTime + 6; };
    if ((int)kit.size() >= KIT_VIALS) { line("THE SCOOP STAYED SHUT - THE EIGHT VIALS ARE FULL"); return; }
    const Body& b = sys.bodies[surf.site.body];
    const double altKm = groundC > 59 ? std::min(90.0, (groundC - 35.0) / 8.0) : 3.0, cloudC = std::round(groundC - 8.0 * altKm);
    Vial v;
    v.sx = sys.star.sx; v.sy = sys.star.sy; v.sz = sys.star.sz; v.body = surf.site.body;
    v.unit = surf.site.unitAt(surf.player.x, surf.player.z);
    v.hab.niche = NI_CLOUD; v.hab.meanC = cloudC; v.hab.salt = 0;
    v.hab.place = placeKey(b.seed ^ 0xC10D5C00ULL, v.unit, b.radiusKm + std::round(altKm));   // the cloud's own cubic metre, a kilometre's layer
    v.name = fmt("CLOUD DROPLETS AT %.0f KM", altKm); v.readingC = cloudC; v.t = t; v.clock = guide.clock;
    v.activity = sampleOf(surf.biota(), v.hab).activity;
    kit.push_back(v);
    sampleTaken(v, "THE SCOOP: ");
    const std::string head = fmt("THE SCOOP: %s, %.0f C", v.name.c_str(), v.readingC);
    const std::string s = head + " - ACTIVITY " + VIAL_ACTIVITY_NAMES[v.activity];
    line(s.size() <= 52 ? s : head + " - " + VIAL_ACTIVITY_NAMES[v.activity]);
}

// the climb aboard: the vials to the cold store (R-432: every one; it has no limit)
void Game::storeVials() {
    if (kit.empty()) return;
    const int n = (int)kit.size();
    for (const Vial& v : kit) coldStore.push_back(v);
    kit.clear();
    status(fmt("%d SAMPLE%s TO THE COLD STORE", n, n == 1 ? "" : "S"), 5);
}

// a line a vial: `vial <0 the kit, 1 the cold store> <sector> <body> <unit> <niche> <mean C> <salt> <flags: 1 shade, 2 a bloom> <old rock>
// <place> <read C> <activity> <t> <name>` (Z-04: a bloom kept, its crowd in what the vial holds)
void Game::saveVials(std::ostream& f) const {
    auto line = [&](int where, const Vial& v) {
        f << "vial " << where << " " << v.sx << " " << v.sy << " " << v.sz << " " << v.body << " " << v.unit.x << " " << v.unit.y << " " << v.unit.z << " " << v.hab.niche
          << " " << v.hab.meanC << " " << v.hab.salt << " " << ((v.hab.shade ? 1 : 0) | (v.hab.bloom ? 2 : 0)) << " " << (v.hab.oldRock ? 1 : 0) << " " << v.hab.place << " " << v.readingC << " "
          << v.activity << " " << v.t << " " << v.name << "\n";
        if (!guide.seeded.empty() && v.clock >= 0) f << "vialclock " << where << " " << v.clock << "\n";   // J-03: a seeded guide's (its lines' spread)
    };
    for (const Vial& v : kit) line(0, v);
    for (const Vial& v : coldStore) line(1, v);
}
bool Game::loadVial(const std::string& text) {
    std::istringstream is(text);
    std::string key; int where = -1, shade = 0, old = 0;
    Vial v;
    is >> key >> where >> v.sx >> v.sy >> v.sz >> v.body >> v.unit.x >> v.unit.y >> v.unit.z >> v.hab.niche >> v.hab.meanC >> v.hab.salt >> shade >> old >> v.hab.place >> v.readingC >>
            v.activity >> v.t;
    if (is.fail() || key != "vial" || where < 0 || where > 1 || v.hab.niche < 0 || v.hab.niche >= NI_COUNT || v.activity < 0 || v.activity >= VA_COUNT) return false;
    std::getline(is, v.name); if (!v.name.empty() && v.name[0] == ' ') v.name.erase(0, 1);
    v.hab.shade = (shade & 1) != 0; v.hab.bloom = (shade & 2) != 0; v.hab.oldRock = old != 0;
    std::vector<Vial>& to = where == 0 ? kit : coldStore;
    if (where == 1 || (int)to.size() < KIT_VIALS) to.push_back(v);
    return true;
}
