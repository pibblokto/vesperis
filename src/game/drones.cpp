// C-18 (2026-10-11): the drone bay (game.h's DroneBay; what a world holds and a trip's haul: peoples/drones.h; the drones drawn:
// game/drone_draw.cpp). The user's request: "add three drones with robotic hands (don't model them exactly like you modeled surface drone
// but still in similar design) that you can send from stardrifter to felisian/desert planets with civilizations. When you send them each can
// bring 2-8 items on stardrifter each ... cooldown is 5 minutes. They can bring both shards (all three kinds) and instruments".
//
// Parked at a people's world, the bay's Enter sends the three down, one after another out of the rack's ports: each falls from the orbit to
// the place that holds the most of what the ship lacks, searches it (its arms down, its hands closing on each find), flies on to another
// when the first is spent, climbs back and docks; what it found goes into the guide as it docks (the decoder's records, the case's
// instruments) and the ruins hold it no more. Each searches for two minutes from its first place to its last, and the bay charges for five
// minutes once they are back (the user's, after the first trips: "make them look for stuff longer, maybe for 2 minutes + 5 minutes
// recharge"), both on real time (the warp does not hurry them, as the probe's relay). Leaving the parking calls them back aboard with what
// they had found.
#include "game.h"
#include "ui.h"
#include "peoples/records.h"
#include "peoples/ruins.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <thread>

namespace {
const double BAY_OUT_S = 2.2, DOCK_S = 2.2, STAGGER_S = 0.7;   // out of the rack into the hull (drone_draw.cpp's), one after another
const double DESCENT_S = 14, CLIMB_S = 14;                     // from the orbit to the ground and back
const uint32_t DRONE_COL[DRONE_COUNT] = {0xFF5ABEF5u, 0xFFEBE66Eu, 0xFF78FABEu};   // 1 amber, 2 cyan, 3 lime (0xAABBGGRR)
const char* placeWord(int sclass) {
    switch (sclass) {
        case SC_CAPITAL: return "A GREAT CITY";
        case SC_CITY: return "A CITY";
        case SC_TOWN: return "A TOWN";
        case SC_VILLAGE: return "A VILLAGE";
        case SC_HAMLET: return "A HAMLET";
        case SC_MONUMENT: return "A MONUMENT";
        default: return "THE RUINS";
    }
}
double smooth01(double u) { u = clampd(u, 0, 1); return u * u * (3 - 2 * u); }
double arcBetween(double lat0, double lon0, double lat1, double lon1) {
    const double c = std::sin(lat0) * std::sin(lat1) + std::cos(lat0) * std::cos(lat1) * std::cos(lon1 - lon0);
    return std::acos(clampd(c, -1, 1));
}
void lerpLatLon(double lat0, double lon0, double lat1, double lon1, double u, double& lat, double& lon) {
    lat = lat0 + (lat1 - lat0) * u;
    lon = lon0 + wrapAngle(lon1 - lon0) * u;
}
std::string kmString(double km) { return km >= 10 ? fmt("%.0f KM", km) : fmt("%.1f KM", std::max(0.0, km)); }
}

// the ship is parked at the trip's world
bool Game::droneBayHere() const {
    return sys.valid && sys.star.seed == bay.starSeed && ship.mode == ShipState::PARKED && ship.parkedBody == bay.body && ship.parkedBelt < 0 &&
           bay.body >= 0 && bay.body < (int)sys.bodies.size();
}
// not out and charged (five minutes from the last trip's end)
bool Game::droneBayReady() const { return !bay.out && (bay.clock < 0 || bay.clock >= bay.back + DRONE_COOLDOWN_S); }
double Game::droneCooldownLeft() const { return bay.clock < 0 || bay.out ? 0.0 : std::max(0.0, bay.back + DRONE_COOLDOWN_S - bay.clock); }
// seconds until the last of them docks (-1: not out, or its trip not laid out yet)
double Game::droneBackIn() const {   // the last dock's clock from now
    if (!bay.out || !bay.planned) return -1;
    double last = bay.clock;
    for (const DroneRun& R : bay.run) if (!R.docked) last = std::max(last, R.dock);
    return last - bay.clock;
}
std::string Game::droneFindKey(const DroneFindRec& f) const { return findKeyOf(bay.worldKey, f.kind, f.index); }
// "A CLAY TABLET", "A WAX CYLINDER", "A SHELL PIPE"
std::string Game::droneFindLabel(const DroneFindRec& f) const {
    if (f.kind == DF_INSTRUMENT) return f.name.empty() ? std::string("AN INSTRUMENT") : f.name;
    return RECORD_MEDIUM_HUD[clampi(f.medium, 0, RM_COUNT - 1)];
}

// the world's items asked for on a thread of their own (the plan and the histories they need may still be on the approach's thread)
void Game::droneWorldAsk(int body) {
    if (!sys.valid || body < 0 || body >= (int)sys.bodies.size()) return;
    if (droneWorldHere(sys.star.seed, body) || droneJobBusy) return;
    if (droneJob.joinable()) droneJob.join();
    droneJobBusy = true; droneJobStar = sys.star.seed; droneJobBody = body;
    droneJob = std::thread([this, s = sys, body]() {
        DroneWorld w;
        droneWorldOf(s, body, w);
        droneJobOut = std::move(w);
        droneJobBusy = false;
    });
}

// the drones sent down (or the refusal's status)
void Game::droneBayLaunch() {
    if (bay.out) { refuse("THE DRONES ARE DOWN THERE - THEY COME BACK BY THEMSELVES", 5); return; }
    if (!droneBayReady()) { const int s = (int)std::ceil(droneCooldownLeft()); refuse(fmt("THE DRONES ARE CHARGING - READY IN %d:%02d", s / 60, s % 60), 5); return; }
    if (!sys.valid || ship.mode != ShipState::PARKED || ship.parkedBody < 0 || ship.parkedBody >= (int)sys.bodies.size() || ship.parkedBelt >= 0) {
        refuse("THE DRONES GO DOWN FROM AN ORBIT: PARK AT A WORLD", 5); return;
    }
    const int b = ship.parkedBody;
    const BodyGen& g = spaceR.genFor(sys.bodies[b]);
    if (!worldHadCivilisation(g)) { refuse("NO PEOPLE LIVED DOWN THERE: NOTHING TO BRING", 5); return; }
    const std::string wk = worldKeyOf(b);
    if (droneWorldHere(sys.star.seed, b)) {   // all of it held already
        int left = 0;
        for (const DroneItem& it : bay.world.items)
            if (!(it.kind == DF_INSTRUMENT ? guide.instruments.count(findKeyOf(wk, it.kind, it.index)) : guide.shards.count(findKeyOf(wk, it.kind, it.index)))) left++;
        if (left == 0) { refuse("THE SHIP HOLDS ALL THEY LEFT DOWN THERE", 5); return; }
    }
    bay.out = true; bay.planned = false; bay.recalled = false; bay.clock = 0; bay.back = 0;
    bay.starSeed = sys.star.seed; bay.body = b; bay.worldKey = wk; bay.worldName = upper(bodyNameOf(b));
    guide.droneTrips++;
    bay.seed = hashCombine(g.seed ^ 0xC18B47ULL, (uint64_t)guide.droneTrips * 0x9E3779B97F4A7C15ULL ^ (uint64_t)std::llround(t * 16));
    StarSystem::latLonFromBody(normalize(sys.bodyFrame(b, t) * ship.parkDir), bay.subLat, bay.subLon);
    for (int k = 0; k < DRONE_COUNT; k++) { bay.run[k] = DroneRun(); bay.run[k].out = k * STAGGER_S; bay.run[k].docked = false; }
    bay.listTop = 0;
    if (bay.mapSeed != sys.bodies[b].seed) buildDroneMap(b);
    if (droneWorldHere(sys.star.seed, b)) dronePlanTrip(); else droneWorldAsk(b);
    logEvent("DRONES", fmt("THE THREE DRONES SENT DOWN TO %s FROM ITS ORBIT", bay.worldName.c_str()));
    status("THE DRONES ARE AWAY - E AT THEIR RACK FOLLOWS THEM", 6);
    audio.beep = BEEP_LOCK;
}

// the haul drawn and each drone's clock laid out: out of its port, the fall to its first stop, two minutes searching (DRONE_SEARCH_S: the flights
// between its stops in them, the rest shared by the stops, a share each and one a find, the finds spread through each), the climb, the dock
void Game::dronePlanTrip() {
    if (bay.planned || !droneWorldHere(bay.starSeed, bay.body) || bay.body < 0 || bay.body >= (int)sys.bodies.size()) return;
    const DroneWorld& W = bay.world;
    std::vector<bool> held(W.items.size(), false);
    for (size_t i = 0; i < W.items.size(); i++) {
        const std::string key = findKeyOf(bay.worldKey, W.items[i].kind, W.items[i].index);
        held[i] = W.items[i].kind == DF_INSTRUMENT ? guide.instruments.count(key) > 0 : guide.shards.count(key) > 0;
    }
    DroneHaul H;
    droneHaulOf(W, held, bay.seed, H);
    Rng rng(bay.seed ^ 0x7105ULL);
    const double radiusKm = sys.bodies[bay.body].radiusKm;
    for (int k = 0; k < DRONE_COUNT; k++) {
        DroneRun& R = bay.run[k];
        R.wanted = H.wanted[k]; R.stops.clear(); R.finds.clear();
        double t1 = std::max(R.out + BAY_OUT_S + DESCENT_S + 0.4 * k, bay.clock + 1.0);
        R.down = t1;
        std::vector<DroneStopRec> stops; std::vector<const DroneLeg*> legs;
        if (H.legs[k].empty()) {   // nothing left for it: a look round the ruins under the ship
            DroneStopRec st; st.lat = clampd(bay.subLat + rng.sym(0.03), -1.5, 1.5); st.lon = bay.subLon + rng.sym(0.03);
            stops.push_back(st); legs.push_back(nullptr);
        }
        for (const DroneLeg& leg : H.legs[k]) {
            DroneStopRec st; st.place = leg.place;
            if (leg.place >= 0 && leg.place < (int)W.places.size()) { st.lat = W.places[leg.place].lat; st.lon = W.places[leg.place].lon; st.sclass = W.places[leg.place].sclass; }
            else { st.lat = clampd(bay.subLat + rng.sym(0.05), -1.5, 1.5); st.lon = bay.subLon + rng.sym(0.05); }
            stops.push_back(st); legs.push_back(&leg);
        }
        std::vector<double> hop(stops.size(), 0.0);   // across the land to the next
        double hops = 0, shares = 0;
        for (size_t l = 1; l < stops.size(); l++) { hop[l] = 4 + std::min(6.0, arcBetween(stops[l - 1].lat, stops[l - 1].lon, stops[l].lat, stops[l].lon) * radiusKm / 300.0); hops += hop[l]; }
        for (size_t l = 0; l < stops.size(); l++) shares += 1 + (legs[l] ? (double)legs[l]->items.size() : 0.0);
        const double ground = std::max(20.0, DRONE_SEARCH_S - hops);
        for (size_t l = 0; l < stops.size(); l++) {
            DroneStopRec& st = stops[l];
            const int m = legs[l] ? (int)legs[l]->items.size() : 0;
            const double stay = ground * (1 + m) / shares, slot = (stay - 2.6) / std::max(1, m);   // settled, a find in each slot, a moment before it goes on
            t1 += hop[l];
            st.arrive = t1;
            for (int j = 0; j < m; j++) {
                const DroneItem& it = W.items[legs[l]->items[j]];
                DroneFindRec f; f.kind = it.kind; f.index = it.index; f.people = it.people; f.medium = it.medium; f.place = it.place; f.name = it.name;
                f.t = t1 + 1.2 + slot * (j + rng.range(0.2, 0.8));
                R.finds.push_back(f);
            }
            t1 += stay; st.leave = t1;
            R.stops.push_back(st);
        }
        R.up = t1;
        R.dock = t1 + CLIMB_S + DOCK_S + 0.3 * k;
    }
    bay.planned = true;
}

// a drone on its rack again: its finds into the guide (a copy taken is gone from the whole world, as one taken by hand)
void Game::droneDock(int k) {
    DroneRun& R = bay.run[k];
    if (R.docked) return;
    R.docked = true;
    for (DroneFindRec& f : R.finds) {
        if (f.t > bay.clock) continue;   // a recall: what it had not found yet stays down there
        const std::string key = droneFindKey(f);
        if (f.kind == DF_INSTRUMENT) guide.instruments.insert(key); else guide.shards.insert(key);
        f.aboard = true;
    }
    R.finds.erase(std::remove_if(R.finds.begin(), R.finds.end(), [&](const DroneFindRec& f) { return !f.aboard; }), R.finds.end());
    if (surf.valid && sys.valid && sys.star.seed == bay.starSeed && surf.site.body == bay.body) syncShards();   // the explorer on that world: gone from the ruins
    guide.save(guidePath);
    audio.beep = BEEP_UI;
}

// the ship left the parking: the drones aboard with what they had found
void Game::droneRecall() {
    for (int k = 0; k < DRONE_COUNT; k++) if (!bay.run[k].docked) { bay.run[k].dock = bay.clock; droneDock(k); }
    bay.recalled = true;
    bay.planned = true;
    droneTripEnd();
}

// the last docked: the log, the status
void Game::droneTripEnd() {
    bay.out = false; bay.back = bay.clock;   // the charge from now
    int n[DF_COUNT] = {}, total = 0;
    std::vector<std::string> where;
    for (const DroneRun& R : bay.run) {
        for (const DroneFindRec& f : R.finds) if (f.aboard) { n[clampi(f.kind, 0, DF_COUNT - 1)]++; total++; }
        for (const DroneStopRec& s : R.stops) if (s.arrive <= bay.clock) where.push_back(placeWord(s.sclass));
    }
    guide.droneFinds += total;
    std::vector<std::string> parts;
    if (n[DF_TEXT]) parts.push_back(fmt("%d TEXT%s", n[DF_TEXT], n[DF_TEXT] == 1 ? "" : "S"));
    if (n[DF_MUSIC]) parts.push_back(fmt("%d PIECE%s", n[DF_MUSIC], n[DF_MUSIC] == 1 ? "" : "S"));
    if (n[DF_CHART]) parts.push_back(fmt("%d CHART%s", n[DF_CHART], n[DF_CHART] == 1 ? "" : "S"));
    if (n[DF_INSTRUMENT]) parts.push_back(fmt("%d INSTRUMENT%s", n[DF_INSTRUMENT], n[DF_INSTRUMENT] == 1 ? "" : "S"));
    std::string list;
    for (size_t i = 0; i < parts.size(); i++) list += (i ? (i + 1 == parts.size() ? " AND " : ", ") : "") + parts[i];
    if (list.empty()) list = "NOTHING";
    logEvent("DRONES",
            fmt("THE DRONES CAME BACK FROM %s%s WITH %s FROM %d PLACE%s SEARCHED", bay.worldName.c_str(), bay.recalled ? " AS THE SHIP LEFT" : "", list.c_str(), (int)where.size(),
                    where.size() == 1 ? "" : "S"));
    if (bay.recalled) status(fmt("THE DRONES CAME BACK AS THE SHIP LEFT: %d FINDS", total), 6);
    else status(total ? fmt("THE DRONES ARE BACK WITH %d FINDS", total) : std::string("THE DRONES ARE BACK WITH NOTHING"), 5);
    if (total) { std::string s; for (size_t i = 0; i < parts.size(); i++) s += (i ? ", " : "") + parts[i]; statusNext = s; statusNextSecs = 5; }
}

// the thread's result taken (once it is done)
void Game::droneCollect() {
    if (droneJobBusy || !droneJob.joinable()) return;
    droneJob.join();
    bay.world = std::move(droneJobOut); droneJobOut = DroneWorld();
    bay.worldValid = true; bay.worldStar = droneJobStar; bay.worldBody = droneJobBody;
}

// every simulating frame: the thread's result, the clock, the docks, the recall
void Game::updateDrones(double realDt) {
    droneCollect();   // the world's items, read on the thread
    if (bay.clock < 0) return;
    bay.clock += realDt;
    if (!bay.out) return;
    if (!droneBayHere()) { droneRecall(); return; }
    if (!bay.planned) {
        if (droneWorldHere(bay.starSeed, bay.body)) dronePlanTrip(); else droneWorldAsk(bay.body);
        if (!bay.planned) return;
    }
    bool all = true;
    for (int k = 0; k < DRONE_COUNT; k++) { if (!bay.run[k].docked && bay.clock >= bay.run[k].dock) droneDock(k); all = all && bay.run[k].docked; }
    if (all) droneTripEnd();
}

// ---- where each is and what it does ---------------------------------------------------------------------------------------------------

// where on the world the drone is (false: aboard or in the bay's hatch)
bool Game::dronePosition(int k, double& lat, double& lon) const {
    lat = bay.subLat; lon = bay.subLon;
    if (!bay.out || k < 0 || k >= DRONE_COUNT) return false;
    const DroneRun& R = bay.run[k];
    const double c = bay.clock, t0 = R.out + BAY_OUT_S;
    if (R.docked || c < t0) return false;
    if (!bay.planned || R.stops.empty()) return true;   // over the ground under the ship until it knows where
    if (c < R.down) { lerpLatLon(bay.subLat, bay.subLon, R.stops[0].lat, R.stops[0].lon, smooth01((c - t0) / std::max(0.1, R.down - t0)), lat, lon); return true; }
    for (size_t i = 0; i < R.stops.size(); i++) {
        const DroneStopRec& s = R.stops[i];
        if (c <= s.leave) {
            if (c >= s.arrive) { lat = s.lat + 0.004 * std::sin(c * 0.9 + k); lon = s.lon + 0.004 * std::cos(c * 0.7 + k); return true; }   // searching: wandering over the place
            const DroneStopRec& p = R.stops[i - 1 < R.stops.size() ? i - 1 : 0];
            lerpLatLon(p.lat, p.lon, s.lat, s.lon, smooth01((c - p.leave) / std::max(0.1, s.arrive - p.leave)), lat, lon);
            return true;
        }
    }
    if (c >= R.dock - DOCK_S) return false;
    const DroneStopRec& l = R.stops.back();
    lerpLatLon(l.lat, l.lon, bay.subLat, bay.subLon, smooth01((c - R.up) / std::max(0.1, R.dock - DOCK_S - R.up)), lat, lon);
    return true;
}

// its arms, its hands, its fans now
DronePose Game::dronePoseAt(int k) const {
    DronePose p;
    p.number = k + 1; p.blink = realTime + k * 0.43; p.spin = realTime * 7.5 * (1 + 0.13 * k);
    if (bay.clock < 0 || k < 0 || k >= DRONE_COUNT || !bay.out) return p;
    const DroneRun& R = bay.run[k];
    if (R.docked || bay.clock < R.out) return p;
    p.blur = 1;
    if (!bay.planned) return p;
    for (const DroneStopRec& s : R.stops) {   // at a stop: the arms down, a hand closing on each find and opening after
        if (bay.clock < s.arrive || bay.clock > s.leave) continue;
        p.arms = smooth01(std::min(bay.clock - s.arrive, s.leave - bay.clock) / 1.0);
        for (const DroneFindRec& f : R.finds) {
            const double d = bay.clock - f.t;
            if (d > -1.0 && d < 0) p.grip = std::max(p.grip, smooth01((d + 1.0) / 0.8));
            else if (d >= 0 && d < 0.7) p.grip = std::max(p.grip, 1 - smooth01(d / 0.7));
        }
    }
    return p;
}

// "DESCENDING 4,200 KM", "SEARCHING A CITY", "ABOARD"
std::string Game::droneStateLine(int k) const {
    if (k < 0 || k >= DRONE_COUNT) return std::string();
    const DroneRun& R = bay.run[k];
    if (bay.clock < 0 || !bay.out || R.docked) return droneBayReady() ? "ABOARD, READY" : (bay.out ? "ABOARD" : "ABOARD, CHARGING");
    const double c = bay.clock;
    if (c < R.out) return "WAITING ITS TURN";
    if (c < R.out + BAY_OUT_S) return "LEAVING THE BAY";
    if (!bay.planned || c < R.down) return "DESCENDING";
    for (size_t i = 0; i < R.stops.size(); i++) {
        const DroneStopRec& s = R.stops[i];
        if (c <= s.leave) return std::string(c >= s.arrive ? "IN " : "FLYING TO ") + placeWord(s.sclass);
    }
    return c < R.dock - DOCK_S ? "CLIMBING" : "DOCKING";
}

// ---- the screen ------------------------------------------------------------------------------------------------------------------

// the body the screen shows: the trip's, else the one the ship is parked at (-1 none)
int Game::droneBayWorld() const { return bay.out ? bay.body : (sys.valid && ship.mode == ShipState::PARKED && ship.parkedBelt < 0 ? ship.parkedBody : -1); }

void Game::openDroneBay() {
    radarOff(); if (tele.on) telescopeOff();
    const int b = droneBayWorld();
    if (sys.valid && b >= 0 && b < (int)sys.bodies.size()) {
        if (bay.mapSeed != sys.bodies[b].seed) buildDroneMap(b);
        if (worldHadCivilisation(spaceR.genFor(sys.bodies[b]))) droneWorldAsk(b);
    }
    bay.listTop = 0;
    state = GameState::DRONES; audio.beep = BEEP_UI;
}

// R-414 (2026-10-11): a trip's finds go from the rack once its screen has shown them (the user: "all the findings should be cleared after you
// took a look at them ... it will appear in decoder/instruments case. So drones are back, you open, see what they found, close rack and that's
// it"): closed after the drones are back aboard, the list, the drones' counts and the places searched on the map are gone; a trip still on
// keeps them (they are not all back)
void Game::droneFindsSeen() {
    if (bay.out) return;
    for (DroneRun& R : bay.run) { R.finds.clear(); R.stops.clear(); }
    bay.listTop = 0;
}

void Game::updateDroneBay(const Input& in, double dt, double realDt) {
    {   // F-02: the world's map once its thread has made it (the screen opens at once: THE MAP IS BEING DRAWN until then)
        const int b = droneBayWorld();
        if (sys.valid && b >= 0 && b < (int)sys.bodies.size() && bay.mapSeed != sys.bodies[b].seed) buildDroneMap(b);
    }
    if (in.wasPressed(KEY_ESCAPE) || (in.wasPressed(KEY_E) && !in.ctrl())) { droneFindsSeen(); state = GameState::SPACE; audio.beep = BEEP_UI; return; }
    if (in.wasPressed(KEY_UP)) { bay.listTop = std::max(0, bay.listTop - 1); audio.beep = BEEP_UI; }
    if (in.wasPressed(KEY_DOWN)) { bay.listTop++; audio.beep = BEEP_UI; }
    if (enterKey(in)) {
        const bool was = bay.out;
        droneBayLaunch();
        if (!was && bay.out) state = GameState::SPACE;   // back at the rack to see them go
    }
}

// U-04: the drone bay's commands (the controller's first page)
void Game::droneBayPad(PadMenu& m) const { m.title = "THE DRONE BAY"; m.item("SEND THE DRONES", KEY_ENTER); m.item("BACK TO THE CABIN", KEY_ESCAPE); }

void Game::renderDroneBay() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 225);
    drawText(canvas, 6, 4, "THE DRONE BAY", HUD_AMBER);
    {   // the bay: out, charging, ready
        std::string st; uint32_t col;
        if (bay.out) { const double in = droneBackIn(); const int s = (int)std::ceil(std::max(0.0, in)); st = in >= 0 ? fmt("BACK IN  %d:%02d", s / 60, s % 60) : std::string("AWAY"); col = HUD_CYAN; }
        else if (!droneBayReady()) { const int s = (int)std::ceil(droneCooldownLeft()); st = fmt("CHARGING  %d:%02d", s / 60, s % 60); col = HUD_AMBER; }
        else { st = "READY"; col = HUD_GREEN; }
        drawText(canvas, UW - 6 - textWidth(st.c_str()), 4, st.c_str(), col);
    }
    // the world: the trip's, else the one under the ship
    const int b = droneBayWorld();
    const bool world = sys.valid && b >= 0 && b < (int)sys.bodies.size();
    bool people = false;
    {
        std::string line;
        if (!world) line = "NO WORLD UNDER THE SHIP";
        else {
            people = worldHadCivilisation(spaceR.genFor(sys.bodies[b]));
            line = (bay.out ? "DOWN ON " : "OVER ") + trunc(upper(bodyNameOf(b)), 20);
            if (!people) line += " - NO PEOPLE LIVED THERE";
            else if (droneWorldHere(sys.star.seed, b) && !bay.world.peopleNames.empty()) {   // the peoples named once the ship holds a record of theirs
                const std::string prefix = worldKeyOf(b) + "/S";
                bool known[2] = {false, false};
                for (const std::string& k : guide.shards) if (k.compare(0, prefix.size(), prefix) == 0) known[std::min(1, shardPeopleOf(atoi(k.c_str() + prefix.size())))] = true;
                const int np = (int)bay.world.peopleNames.size();
                if (np > 1 && known[0] && known[1]) line += " - THE " + upper(bay.world.peopleNames[0]) + " AND THE " + upper(bay.world.peopleNames[1]);
                else if (known[0] || (np > 1 && known[1])) line += " - THE " + upper(bay.world.peopleNames[known[0] ? 0 : 1]) + (np > 1 ? " AND ANOTHER PEOPLE" : "");
                else line += np > 1 ? " - TWO PEOPLES LIVED THERE" : " - A PEOPLE LIVED THERE";
            }
        }
        drawText(canvas, 6, 13, trunc(line, 52).c_str(), HUD_DIM);
    }
    // ---- the three drones: each in lines (turning while aboard, three-quarters on while out), its number, what it carries, where it is
    for (int k = 0; k < DRONE_COUNT; k++) {
        const int x0 = 6, y0 = 24 + k * 54, w = 100, h = 44;
        const DroneRun& R = bay.run[k];
        drawRectRGB(canvas, x0 - 1, y0 - 1, x0 + w, y0 + h, rgb(40, 52, 40));
        const bool aboard = bay.clock < 0 || !bay.out || R.docked;
        const DronePose pose = dronePoseAt(k);
        // out: its face three-quarters on
        drawDroneWire(x0, y0, w, h, pose, aboard ? realTime * 0.35 + k * 2.1 : 2.5 + 0.18 * std::sin(realTime * 0.4 + k), aboard && !droneBayReady() ? rgb(150, 140, 80) : rgb(255, 240, 140));
        drawText(canvas, x0 + 2, y0 + 2, fmt("%d", k + 1).c_str(), DRONE_COL[k]);
        std::string right;
        int found = 0; for (const DroneFindRec& f : R.finds) if (f.t <= bay.clock || f.aboard) found++;
        if (bay.clock >= 0 && (bay.out || !R.finds.empty())) right = bay.planned ? fmt("%d/%d", found, std::max(R.wanted, found)) : "?";
        if (bay.out && !R.docked && world) {   // its height over the ground while it falls or climbs
            const double alt = std::max(0.0, ship.parkDist - sys.bodies[b].radiusKm);
            const double c = bay.clock, t0 = R.out + BAY_OUT_S;
            if (c >= t0 && (!bay.planned || c < R.down)) right = kmString(alt * (1 - smooth01((c - t0) / std::max(0.1, (bay.planned ? R.down : t0 + DESCENT_S) - t0))));
            else if (bay.planned && c >= R.up && c < R.dock - DOCK_S) right = kmString(alt * smooth01((c - R.up) / std::max(0.1, R.dock - DOCK_S - R.up)));
        }
        if (!right.empty()) drawText(canvas, x0 + w - 2 - textWidth(right.c_str()), y0 + 2, right.c_str(), DRONE_COL[k]);
        drawText(canvas, x0, y0 + h + 2, trunc(droneStateLine(k), 16).c_str(), aboard ? HUD_DIM : HUD_GREEN);
    }
    // ---- the map: the world, the ground under the ship, the places searched, the drones' ways and the drones
    const int mx = 112, my = 24, mw = 200, mh = 100;
    drawRectRGB(canvas, mx - 1, my - 1, mx + mw, my + mh, rgb(40, 52, 40));
    if (world && bay.mapW > 0 && bay.mapSeed == sys.bodies[b].seed) {
        const int S = std::max(1, canvas.scale);
        for (int y = my * S; y < (my + mh) * S; y++) {
            const int sy = std::min(bay.mapH - 1, (y - my * S) * bay.mapH / (mh * S));
            for (int x = mx * S; x < (mx + mw) * S; x++) canvas.px[(size_t)y * canvas.w + x] = bay.map[(size_t)sy * bay.mapW + std::min(bay.mapW - 1, (x - mx * S) * bay.mapW / (mw * S))];
        }
        auto toMap = [&](double lat, double lon, int& x, int& y) {
            x = mx + (int)(wrap2pi(lon + PI) / TAU * mw);
            y = my + (int)((0.5 - lat / PI) * mh);
            x = clampi(x, mx, mx + mw - 1);
            y = clampi(y, my, my + mh - 1);
        };
        auto dotted = [&](double lat0, double lon0, double lat1, double lon1, uint32_t col) {
            for (int i = 0; i <= 24; i++) {
                if (i % 2) continue;
                double la, lo;
                lerpLatLon(lat0, lon0, lat1, lon1, i / 24.0, la, lo);
                int x, y;
                toMap(la, lo, x, y);
                fillRectRGB(canvas, x, y, x, y, col);
            }
        };
        double sLat = bay.subLat, sLon = bay.subLon;
        if (!bay.out && ship.mode == ShipState::PARKED && ship.parkedBody == b) StarSystem::latLonFromBody(normalize(sys.bodyFrame(b, t) * ship.parkDir), sLat, sLon);
        const bool trip = bay.clock >= 0 && bay.planned && bay.starSeed == sys.star.seed && bay.body == b;
        if (trip) for (int k = 0; k < DRONE_COUNT; k++) {
            const DroneRun& R = bay.run[k];
            double pLat = bay.subLat, pLon = bay.subLon;
            for (const DroneStopRec& s : R.stops) {
                if (s.arrive > bay.clock + 1e-6 && bay.out) break;   // the ways flown so far
                dotted(pLat, pLon, s.lat, s.lon, rgb(90, 110, 90));
                int x, y; toMap(s.lat, s.lon, x, y);
                drawRectRGB(canvas, x - 2, y - 2, x + 2, y + 2, HUD_AMBER);
                pLat = s.lat; pLon = s.lon;
            }
        }
        {   // the ground under the ship: a cross
            int x, y; toMap(sLat, sLon, x, y);
            fillRectRGB(canvas, x - 3, y, x + 3, y, HUD_WHITE); fillRectRGB(canvas, x, y - 3, x, y + 3, HUD_WHITE);
        }
        if (bay.out) for (int k = 0; k < DRONE_COUNT; k++) {
            double lat, lon;
            if (!dronePosition(k, lat, lon)) continue;
            int x, y; toMap(lat, lon, x, y);
            const bool on = std::fmod(realTime * 2 + k * 0.3, 1.0) < 0.75;
            fillRectRGB(canvas, x - 1, y - 1, x + 1, y + 1, on ? DRONE_COL[k] : HUD_DIM);
            drawText(canvas, std::min(mx + mw - 6, x + 3), std::max(my, y - 8), fmt("%d", k + 1).c_str(), DRONE_COL[k]);
        }
    } else drawTextCentered(canvas, mx + mw / 2, my + mh / 2 - 3, world ? "THE MAP IS BEING DRAWN" : "NO WORLD UNDER THE SHIP", HUD_DIM);
    // ---- under the map: what the ship holds of what the world left, in the decoder's words (its peoples' shards together: the decoder lists
    // each people's apart) and the case's; the finds, the newest first
    {
        std::string l1, l2;
        if (world && people) {
            if (droneWorldHere(sys.star.seed, b)) {
                int rec = 0, recHeld = 0, ins = 0, insHeld = 0; const std::string wk = worldKeyOf(b);
                for (const DroneItem& it : bay.world.items) {
                    const std::string key = findKeyOf(wk, it.kind, it.index);
                    if (it.kind == DF_INSTRUMENT) { ins++; if (guide.instruments.count(key)) insHeld++; } else { rec++; if (guide.shards.count(key)) recHeld++; }
                }
                l1 = fmt("ABOARD: %d OF ITS %d SHARDS", recHeld, rec);
                l2 = ins ? fmt("AND %d OF ITS %d INSTRUMENTS", insHeld, ins) : std::string("IT LEFT NO INSTRUMENTS");
            } else l1 = "READING THE WORLD...";
        }
        drawText(canvas, mx, my + mh + 4, trunc(l1, 33).c_str(), HUD_DIM);
        if (!l2.empty()) drawText(canvas, mx, my + mh + 13, trunc(l2, 33).c_str(), HUD_DIM);
    }
    std::vector<const DroneFindRec*> finds; std::vector<int> whose;
    for (int k = 0; k < DRONE_COUNT; k++) for (const DroneFindRec& f : bay.run[k].finds) if (f.aboard || f.t <= bay.clock) { finds.push_back(&f); whose.push_back(k); }
    std::vector<int> order(finds.size());
    for (size_t i = 0; i < order.size(); i++) order[i] = (int)i;
    std::sort(order.begin(), order.end(), [&](int a, int c) { return finds[a]->t > finds[c]->t; });
    const int rows = 4, y0 = my + mh + 24;
    if (finds.empty()) {
        static const char* const help[] = {"EACH DRONE BRINGS 2 TO 8 OF WHAT", "THE SHIP LACKS, FROM THE PLACES", "THAT KEEP THEM: 2 MINUTES DOWN", "THERE, THEN 5 MINUTES TO CHARGE"};
        for (int i = 0; i < rows; i++) drawText(canvas, mx, y0 + i * 9, help[i], HUD_DIM);
    } else {
        bay.listTop = std::max(0, std::min(bay.listTop, (int)finds.size() - rows));
        for (int r = 0; r < rows && bay.listTop + r < (int)order.size(); r++) {
            const int i = order[bay.listTop + r];
            const DroneFindRec& f = *finds[i];
            const int y = y0 + r * 9;
            const uint32_t col = f.aboard ? HUD_WHITE : HUD_GREEN;
            if (f.kind == DF_INSTRUMENT) {   // a little harp: its frame and two strings
                fillRectRGB(canvas, mx, y, mx, y + 6, col); fillRectRGB(canvas, mx, y + 6, mx + 4, y + 6, col);
                for (int d = 1; d < 5; d++) fillRectRGB(canvas, mx + d, y + d - 1, mx + d, y + d - 1, col);
                fillRectRGB(canvas, mx + 2, y + 2, mx + 2, y + 5, col);
            } else drawRecordKindGlyph(canvas, mx, y, f.kind, col);
            drawText(canvas, mx + 9, y, trunc(droneFindLabel(f), 28).c_str(), col);
            drawText(canvas, mx + mw - 5, y, fmt("%d", whose[i] + 1).c_str(), DRONE_COL[whose[i]]);
            const char arrow[2] = {r == 0 && bay.listTop > 0 ? CH_UP : (r == rows - 1 && bay.listTop + rows < (int)finds.size() ? CH_DOWN : ' '), 0};   // more above, more below
            if (arrow[0] != ' ') drawText(canvas, mx + mw - 13, y, arrow, HUD_AMBER);
        }
    }
    const bool can = droneBayReady() && world && people;
    if (!drawMapStatus()) drawTextCentered(canvas, UW / 2, UH - 10, can ? "ENTER SENDS THE DRONES  UP/DOWN THE FINDS  ESC" : "UP/DOWN THE FINDS  ESC BACK", HUD_DIM);
}
