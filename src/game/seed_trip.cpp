// J-03 (R-448): the drone bay's seed trip (docs/ideas/IDEAS-seeded-worlds.md section 1; the bay: game/drones.cpp). Parked in orbit of a
// landable world, S on the bay opens the seed page: C chooses a culture alive in a flask (B-473: growing, full, starving or asleep as
// spores), the arrows move the place on the world's map (Shift five times as far), Page Down and Page Up its depth down a drill hole (J-13:
// the rock's, or a sea under the ice), Enter sends drone 1 alone. It falls to the place, lets
// the culture go (its `seeded` line in the guide, SEEDED in the log), watches two minutes and climbs; its report is the verdict
// (life/seeding.h). The flask keeps its culture, and the bay is ready again at once (R-449). Refused at home, at a giant, out of orbit,
// with the drones away, and where three lines live already
#include "game.h"
#include "ui.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
std::string latLonWords(double lat, double lon) {
    return fmt("%.0f %s %.0f %s", std::fabs(lat / DEG), lat >= 0 ? "N" : "S", std::fabs(wrapAngle(lon) / DEG), wrapAngle(lon) >= 0 ? "E" : "W");
}
}   // namespace

// the world every game starts at: the pinned home star's first felisian planet (`newGame`'s; the generator's protected home), whatever star the
// explorer calls home since
bool Game::homeWorld(int body) const {
    if (!sys.valid || body < 0 || body >= (int)sys.bodies.size()) return false;
    if (sys.star.sx != HOME_SX || sys.star.sy != HOME_SY || sys.star.sz != HOME_SZ) return false;
    for (const Body& b : sys.bodies) if (b.type == PT_FELISIAN && b.parent < 0) return b.index == body;
    return false;
}

std::string Game::seedRefusal(int body) {
    if (bay.out) return "DRONES ARE AWAY - THEY COME BACK ON THEIR OWN";
    if (!sys.valid || ship.mode != ShipState::PARKED || body < 0 || body >= (int)sys.bodies.size() || ship.parkedBelt >= 0) return "NOT IN ORBIT - APPROACH A WORLD FIRST";
    if (!KINDS[sys.bodies[body].type].landable) return "NO GROUND DOWN THERE";
    if (homeWorld(body)) return "HOME STAYS AS IT IS";
    int living = 0;
    for (const Seeding& s : seedingsOf(body)) living += s.verdict.takes();
    if (living >= MADE_LINES) return "THREE LINES LIVE THERE - IT TAKES NO MORE";
    return "";
}

// B-473: any culture alive in a flask can go down, its cells or its spores, whatever its stage; an empty or a dead one cannot
bool Game::seedCultureLives(int pick) const {
    const int i = pick / 2, k = pick % 2;
    if (pick < 0 || i >= INCUBATOR_FLASKS || k >= lab.flasks[i].n || !lab.flasks[i].made) return false;
    const CultureNow now = flaskNow(i);
    return k < now.n && (now.one[k].cells > 0 || now.one[k].spores > 0);
}

// the page opens on the culture last chosen while it lives, else the first that shows FULL (the one most likely sent), else the first alive
int Game::seedFirstCulture() const {
    for (int p = 0; p < 2 * INCUBATOR_FLASKS; p++)
        if (seedCultureLives(p)) {
            const int st = flaskNow(p / 2).one[p % 2].stage;
            if (st == SG_CROWD || st == SG_OVERGROWN) return p;
        }
    return seedNextCulture(0);
}

int Game::seedNextCulture(int from) const {
    const int n = 2 * INCUBATOR_FLASKS;
    for (int d = 0; d < n; d++) {
        const int p = ((from < 0 ? 0 : from) + d) % n;
        if (seedCultureLives(p)) return p;
    }
    return -1;
}

std::string Game::seedCultureWords(int pick) const {
    if (!seedCultureLives(pick)) return "NO CULTURE IN THE FLASKS";
    const int i = pick / 2, k = pick % 2;
    return fmt("%s, FLASK %d, %s", lab.flasks[i].label[k].c_str(), i + 1, CULTURE_STAGE_NAMES[flaskNow(i).one[k].stage]);
}

// J-13: the depth chosen on the page (km; 0 the ground), within the world's own (`seedDepthsOf`: the rock's, or a sea under its ice); J-14: a
// height in the clouds under 0 (`seedHeightsOf`: -1 the lowest)
double Game::seedPageDepthKm() {
    const int b = ship.parkedBody;
    if (!sys.valid || b < 0 || b >= (int)sys.bodies.size()) return 0;
    const std::vector<double> d = seedDepthsOf(sys, b), h = seedHeightsOf(sys, b);
    bay.pageDepth = clampi(bay.pageDepth, -(int)h.size(), (int)d.size() - 1);
    return bay.pageDepth >= 0 ? d[bay.pageDepth] : -h[-bay.pageDepth - 1];
}

void Game::seedPagePlace() {
    bay.pagePlace.clear();
    const int b = ship.parkedBody;
    if (!sys.valid || b < 0 || b >= (int)sys.bodies.size()) return;
    const auto life = lifeOf(sys);
    const BodyGen& gen = spaceR.genFor(sys.bodies[b]);
    const double km = seedPageDepthKm();
    bay.pagePlace = seedPlaceOf(sys, b, gen, life->bodies[b], bay.pageLat, bay.pageLon, km).name;
    bay.pageWhere = seedPlaceWords(seedPlaceFacts(sys, b, bay.pageLat, bay.pageLon, km, gen.made));   // R-451: what it gives a culture
}

// S on the bay: the page where the world may take a seeding (else the refusal's status, the bay stays)
void Game::seedPageOpen() {
    const int b = ship.parkedBody;
    const std::string no = seedRefusal(b);
    if (!no.empty()) { refuse(no, 5); return; }
    openScreen(GameState::SEEDING);
}

void Game::seedPageEnter() {
    const int b = ship.parkedBody;
    flaskMadeAll();
    if (!seedCultureLives(bay.pagePick)) bay.pagePick = seedFirstCulture();
    StarSystem::latLonFromBody(normalize(sys.bodyFrame(b, t) * ship.parkDir), bay.pageLat, bay.pageLon);   // the ground under the ship
    if (bay.mapSeed != sys.bodies[b].seed) bay.pageDepth = 0;   // (J-13: another world's page opens on its ground)
    seedPagePlace();
    if (bay.mapSeed != sys.bodies[b].seed || bay.mapMade != spaceR.madeOf(bay.mapSeed).key) buildDroneMap(b);
    audio.beep = BEEP_UI;
}

void Game::updateSeedPage(const Input& in, double dt, double realDt) {
    const int b = ship.parkedBody;
    if (!seedRefusal(b).empty() && !bay.out) { closeScreen(); return; }   // the ship left the parking (or the world took its third line)
    if (sys.valid && b >= 0 && b < (int)sys.bodies.size() && (bay.mapSeed != sys.bodies[b].seed || bay.mapMade != spaceR.madeOf(bay.mapSeed).key)) buildDroneMap(b);
    if (in.wasPressed(KEY_ESCAPE)) { closeScreen(); audio.beep = BEEP_UI; return; }
    if (in.wasPressed(KEY_C)) {
        const int next = seedNextCulture(bay.pagePick + 1);
        if (next < 0) refuse("NO CULTURE IN THE FLASKS - GROW ONE FIRST", 4);
        else if (next == bay.pagePick) refuse("NO OTHER CULTURE IN THE FLASKS", 3);
        else { bay.pagePick = next; audio.beep = BEEP_UI; }
    }
    const double step = (in.shift() ? 10.0 : 2.0) * DEG;
    double dLat = 0, dLon = 0;
    if (in.wasPressed(KEY_UP)) dLat += step;
    if (in.wasPressed(KEY_DOWN)) dLat -= step;
    if (in.wasPressed(KEY_RIGHT)) dLon += step;
    if (in.wasPressed(KEY_LEFT)) dLon -= step;
    if (dLat != 0 || dLon != 0) {
        bay.pageLat = clampd(bay.pageLat + dLat, -88 * DEG, 88 * DEG); bay.pageLon = wrapAngle(bay.pageLon + dLon);
        seedPagePlace();
        audio.beep = BEEP_UI;
    }
    // J-13: Page Down takes the drill deeper, Page Up back toward the ground; J-14: and on up into the clouds of a world with a deck
    const int dDepth = in.wasPressed(KEY_PAGE_DOWN) ? 1 : (in.wasPressed(KEY_PAGE_UP) ? -1 : 0);
    if (dDepth != 0 && sys.valid && b >= 0 && b < (int)sys.bodies.size()) {
        const int n = (int)seedDepthsOf(sys, b).size(), up = (int)seedHeightsOf(sys, b).size();
        if (bay.pageDepth + dDepth < -up) refuse(up > 0 ? "AS HIGH AS THE CLOUDS GO" : "ON THE GROUND ALREADY", 2);
        else if (bay.pageDepth + dDepth >= n) refuse(n == 2 ? "THE SEA UNDER THE ICE IS AS DEEP AS IT GOES" : "AS DEEP AS THE DRILL GOES", 3);
        else { bay.pageDepth += dDepth; seedPagePlace(); audio.beep = BEEP_UI; }
    }
    if (enterKey(in)) seedLaunch();
}

// U-04: the seed page's commands
void Game::seedPagePad(PadMenu& m) const {
    m.title = "SEED A WORLD"; m.item("CULTURE", KEY_C); m.item("DEEPER", KEY_PAGE_DOWN); m.item("HIGHER", KEY_PAGE_UP); m.item("SEND", KEY_ENTER); m.item("BACK", KEY_ESCAPE);
}

// Enter on the page: drone 1 sent with the flask's culture (the flask keeps it)
void Game::seedLaunch() {
    const int b = ship.parkedBody;
    const std::string no = seedRefusal(b);
    if (!no.empty()) { refuse(no, 5); return; }
    if (!seedCultureLives(bay.pagePick)) { refuse("NO CULTURE IN THE FLASKS - GROW ONE FIRST", 5); return; }
    const int i = bay.pagePick / 2, k = bay.pagePick % 2;
    const Flask& F = lab.flasks[i];
    bay.seedLine = flaskStrainKey(F.who[k]);
    const auto rec = guide.strains.find(bay.seedLine);
    bay.seedEdits = rec != guide.strains.end() && rec->second.what == SM_EDIT ? rec->second.edits : std::string();
    bay.seedName = F.label[k]; bay.seedMedium = F.medium;
    bay.seedLat = bay.pageLat; bay.seedLon = bay.pageLon; bay.seedPlace = bay.pagePlace.empty() ? std::string("THE GROUND") : bay.pagePlace;
    bay.seedDepth = seedPageDepthKm();   // J-13
    bay.seedTrip = true; bay.seedLet = false; bay.seedFate = -1; bay.seedWhy.clear();
    bay.out = true; bay.planned = false; bay.recalled = false; bay.clock = 0;
    bay.starSeed = sys.star.seed; bay.body = b; bay.worldKey = worldKeyOf(b); bay.worldName = upper(bodyNameOf(b));
    StarSystem::latLonFromBody(normalize(sys.bodyFrame(b, t) * ship.parkDir), bay.subLat, bay.subLon);
    dronePlanSeed();
    bay.listTop = 0;
    logEvent("DRONES", fmt("A DRONE SENT DOWN TO %s WITH %s, FROM FLASK %d", bay.worldName.c_str(), bay.seedName.c_str(), i + 1));
    status("DRONE AWAY - E AT DRONE BAY TO FOLLOW", 6);
    audio.beep = BEEP_LOCK;
    closeScreen();
}

// the drone at its place: the culture let go (the guide's `seeded` line at the guide's clock now), its verdict kept for its report
void Game::seedLetGo() {
    bay.seedLet = true;
    SeedRecord r;
    r.body = bay.worldKey; r.lat = bay.seedLat; r.lon = bay.seedLon; r.depthKm = bay.seedDepth; r.line = bay.seedLine; r.edits = bay.seedEdits;
    r.cell = bay.seedMedium.cell; r.tempC = bay.seedMedium.tempC; r.light = bay.seedMedium.light; r.energy = bay.seedMedium.energy; r.salt = bay.seedMedium.salt;
    r.gen = GEN_VERSION; r.clock = guide.clock;
    guide.addSeeding(r);
    guide.save(guidePath);
    if (sys.valid && sys.star.seed == bay.starSeed && bay.body >= 0 && bay.body < (int)sys.bodies.size()) {
        const std::vector<Seeding>& ss = seedingsOf(bay.body);
        if (!ss.empty()) { bay.seedFate = ss.back().verdict.fate; bay.seedWhy = seedWhyWords(ss.back().verdict, ss.back().sent.kind); }   // R-451: in full
    }
    logEvent("SEEDED", fmt("%s: %s LET GO IN %s AT %s", bay.worldName.c_str(), bay.seedName.c_str(), bay.seedPlace.c_str(), latLonWords(bay.seedLat, bay.seedLon).c_str()));
    audio.beep = BEEP_UI;
}

// the drone docked: its report, in the incubator's words
void Game::seedReport() {
    std::string words, why;
    if (!bay.seedLet) words = "CALLED BACK BEFORE IT LET THE CULTURE GO";
    else if (bay.seedFate < 0) words = "CULTURE LET GO";
    else { SeedVerdict v; v.fate = bay.seedFate; words = seedVerdictWords(v); if (!v.takes()) why = bay.seedWhy; }
    logEvent("DRONES", fmt("THE DRONE CAME BACK FROM %s%s: %s", bay.worldName.c_str(), bay.recalled ? " AS THE SHIP LEFT" : "", (why.empty() ? words : why).c_str()));
    status(fmt("%s: %s", trunc(bay.worldName, 12).c_str(), words.c_str()), 6);
    if (!why.empty()) { statusNext = fitText(why, UW - 12); statusNextSecs = 8; }   // R-451: why, in full
}

std::string Game::seedLineWords(const std::string& lineKey) {
    FlaskSpecies w;
    if (!speciesOfKey(lineKey, w)) return "A CULTURE";
    const size_t sp = lineKey.rfind("/S");
    if (sp != std::string::npos && sp > lineKey.rfind("/M")) w.strain = w.strainIn = atoi(lineKey.c_str() + sp + 2);
    const Morph* f = formOfKey(flaskSpeciesKey(w));
    return f ? flaskSpeciesWords(w, *f, false) : flaskSpeciesName(w);
}

std::string Game::madeWords(int body) {
    if (!sys.valid || body < 0 || body >= (int)sys.bodies.size() || guide.seeded.empty()) return "";
    std::string w;
    for (const std::string& t : madeTraitWords(madeNow(body), kindHas(sys.bodies[body].type, KT_SEA))) w += (w.empty() ? "" : ", ") + t;
    return w;
}

void Game::madeLanding(int body) {
    const std::string now = madeWords(body);
    if (now.empty()) return;
    const std::string key = worldKeyOf(body);
    if (guide.madeSeen[key] != now) logEvent("LANDING", fmt("%s SINCE THE LAST LANDING: %s", upper(bodyNameOf(body)).c_str(), now.c_str()));
    guide.madeSeen[key] = now;
}

void Game::renderSeedPage() {
    blendRectRGB(canvas, 0, 0, UW - 1, UH - 1, rgb(0, 0, 0), 225);
    drawText(canvas, 6, 4, "DRONE BAY", HUD_AMBER);
    drawText(canvas, UW - 6 - textWidth("SEED PAGE"), 4, "SEED PAGE", HUD_GREEN);
    const int b = ship.parkedBody;
    const bool world = sys.valid && b >= 0 && b < (int)sys.bodies.size();
    drawText(canvas, 6, 13, trunc(world ? "SEED " + upper(bodyNameOf(b)) : std::string("NO WORLD BELOW"), 52).c_str(), HUD_DIM);
    // the world's map: the ground under the ship, the places of its lines, the cursor
    const int mx = 60, my = 24, mw = 200, mh = 100;
    drawRectRGB(canvas, mx - 1, my - 1, mx + mw, my + mh, rgb(40, 52, 40));
    if (world && bay.mapW > 0 && bay.mapSeed == sys.bodies[b].seed) {
        const int S = std::max(1, canvas.scale);
        for (int y = my * S; y < (my + mh) * S; y++) {
            const int sy = std::min(bay.mapH - 1, (y - my * S) * bay.mapH / (mh * S));
            for (int x = mx * S; x < (mx + mw) * S; x++) canvas.px[(size_t)y * canvas.w + x] = bay.map[(size_t)sy * bay.mapW + std::min(bay.mapW - 1, (x - mx * S) * bay.mapW / (mw * S))];
        }
        auto toMap = [&](double lat, double lon, int& x, int& y) {
            x = clampi(mx + (int)(wrap2pi(lon + PI) / TAU * mw), mx, mx + mw - 1);
            y = clampi(my + (int)((0.5 - lat / PI) * mh), my, my + mh - 1);
        };
        for (const Seeding& s : seedingsOf(b)) {   // its lines' places: a living one's in green, a dead one's dim
            int x, y; toMap(s.lat, s.lon, x, y);
            drawRectRGB(canvas, x - 1, y - 1, x + 1, y + 1, s.verdict.takes() ? HUD_GREEN : HUD_DIM);
        }
        double sLat = 0, sLon = 0;
        StarSystem::latLonFromBody(normalize(sys.bodyFrame(b, t) * ship.parkDir), sLat, sLon);
        { int x, y; toMap(sLat, sLon, x, y); fillRectRGB(canvas, x, y, x, y, HUD_WHITE); }
        int x, y; toMap(bay.pageLat, bay.pageLon, x, y);
        const uint32_t c = std::fmod(realTime * 2, 1.0) < 0.7 ? HUD_AMBER : HUD_WHITE;
        fillRectRGB(canvas, x - 4, y, x - 2, y, c); fillRectRGB(canvas, x + 2, y, x + 4, y, c);
        fillRectRGB(canvas, x, y - 4, x, y - 2, c); fillRectRGB(canvas, x, y + 2, x, y + 4, c);
    } else drawTextCentered(canvas, mx + mw / 2, my + mh / 2 - 3, world ? "THE MAP IS BEING DRAWN" : "NO WORLD BELOW", HUD_DIM);
    // the culture, the place, the lines living there
    // its lines eight pixels apart, a long one (R-451's NEEDS and THERE) wrapped to a second
    int y = my + mh + 6;
    auto row = [&](const char* label, const std::string& text, uint32_t col, int most) {
        const std::vector<std::string> lines = wrapText(text, 43);
        drawText(canvas, 6, y, label, HUD_DIM);
        for (int i = 0; i < (int)lines.size() && i < most; i++) { drawText(canvas, 51, y, lines[i].c_str(), col); y += 8; }
    };
    const bool alive = seedCultureLives(bay.pagePick);
    row("CULTURE", seedCultureWords(bay.pagePick), alive ? HUD_WHITE : HUD_RED, 1);
    if (alive) row("NEEDS", seedNeedsWords(lab.flasks[bay.pagePick / 2].culture.kind[bay.pagePick % 2]), HUD_GREEN, 2);   // R-451: what it needs,
    row("PLACE", latLonWords(bay.pageLat, bay.pageLon) + (bay.pagePlace.empty() ? std::string() : ", " + bay.pagePlace), HUD_WHITE, 1);
    if (world && !bay.pageWhere.empty()) row("THERE", bay.pageWhere, HUD_GREEN, 2);   // and what the place gives: compared by the explorer
    if (world) {
        int living = 0, sent = 0;
        for (const Seeding& s : seedingsOf(b)) { sent++; living += s.verdict.takes(); }
        if (sent) row("LINES", fmt("%d OF %d LIVE THERE, %d SENT", living, MADE_LINES, sent), HUD_GREEN, 1);
    }
    const bool clouds = world && seedCloudDeck(sys.bodies[b]);   // (J-14: up into its clouds)
    if (!drawMapStatus())
        drawTextCentered(canvas, UW / 2, UH - 10, clouds ? "C CULTURE  ARROWS  PGUP CLOUDS  ENTER SEND  ESC BACK" : "C CULTURE  ARROWS  PGDN DEEPER  ENTER SEND  ESC BACK", HUD_DIM);
}
