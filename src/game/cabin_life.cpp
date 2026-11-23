// I-04: life aboard (R-426, IDEAS-small-and-spacious.md section 2.5): what the frame fills with the explorer's things where it comes in its
// room's order (the plan's pieces of PIECE_SHELF and on, ship_plan.h): the photographs printed on the quarters' wall, the shards on the
// library's shelves, the instruments against the case's glass, the player's deck's reels, the home world's plants in the hall's beds, the
// drive's core under the floor glass, the capsule in its cradle over the hatch and its drop through it. Each is lit as the plan's vertices
// are (`shipLampLight`: the lamps, U's level, its night), in the banks the frame dealt (`CabinFrame::banks`)
#include "game.h"
#include "ship_plan.h"
#include "ui.h"
#include "core/png.h"
#include "core/rng.h"
#include "galaxy/kinds.h"
#include "life/morphs.h"
#include <dirent.h>
#include <algorithm>
#include <functional>
#include <cmath>
#include <cstdlib>
#include <fstream>

namespace {
Vec3 rowOf(const Mat3& m, int r) { return Vec3(m.m[r][0], m.m[r][1], m.m[r][2]); }
// a place seen from the frame's eye, its shade, its grain's u and v along a wall (M2's)
RVert vertAt(const CabinFrame& f, const Vec3& p, double shade) {
    const Vec3 w = f.look * (p - f.eye);
    RVert q; q.x = w.x; q.y = w.y; q.z = w.z; q.shade = std::min(63.0, shade); q.u = p.x + p.z * 0.31; q.v = p.y + p.z * 0.17;
    return q;
}
// a box-like fixture's corners (a b c d: the back and the front at one end, the front and the back at the other) and a place on it: along
// its front from b to c, back from its front, up from its floor
struct BoxAt {
    Vec3 A[4];
    explicit BoxAt(const ShipFixture& F) { for (int i = 0; i < 4; i++) A[i] = Vec3(F.x[i], F.y[i], F.z[i]); }
    Vec3 at(double t, double back, double y) const {
        const Vec3 fr = A[1] + (A[2] - A[1]) * t, bk = A[0] + (A[3] - A[0]) * t;
        return fr + (bk - fr) * back + Vec3(0, y, 0);
    }
    Vec3 front() const { return normalize(A[1] - A[0]); }
    double width() const { return length(A[2] - A[1]); }
};
// Z-05: the lab's tones, homes past the banks (`ShipBanks` deals each a free one while it is in view): the pigments' (a home a pigment, green to
// black), a crowd's own light, the incubator's red and blue lights, a broth's cloud; J-01: the ultraviolet's light, a flask's ice, its tissue,
// its dyes (a home a dye) and its sediments (a home a mineral)
constexpr int LAB_TONE = 32, LAB_GLOW = 40, LAB_RED_LIGHT = 41, LAB_BLUE_LIGHT = 42, LAB_UV_LIGHT = 43, LAB_ICE = 44, LAB_CLOUD = 45;
// (R-453: GREEN's and PINK's after the sediments', so no home of J-01's moves)
constexpr int LAB_TISSUE = 49, LAB_DYE = 50, LAB_SEDIMENT = LAB_DYE + WD_FLUORESCENT, LAB_DYE_MORE = LAB_SEDIMENT + SHL_COUNT;
static_assert(LAB_DYE_MORE + WD_COUNT - WD_GREEN <= SHIP_HOMES, "the lab's homes past the banks");
int labDyeHome(int dye) { return dye < WD_GREEN ? LAB_DYE + dye : LAB_DYE_MORE + dye - WD_GREEN; }
RGB labToneColour(int home) {
    if (home > LAB_TONE && home < LAB_TONE + PG_COUNT) return pigmentColour(home - LAB_TONE);
    if (home > LAB_DYE && home <= LAB_DYE + WD_FLUORESCENT) return dyeColour(home - LAB_DYE);
    if (home >= LAB_DYE_MORE && home < LAB_DYE_MORE + WD_COUNT - WD_GREEN) return dyeColour(WD_GREEN + home - LAB_DYE_MORE);
    if (home > LAB_SEDIMENT && home < LAB_SEDIMENT + SHL_COUNT) return shellColour(home - LAB_SEDIMENT);
    switch (home) {
        case LAB_GLOW: return RGB(0.3f, 0.95f, 0.8f);
        case LAB_RED_LIGHT: return RGB(1.0f, 0.28f, 0.18f);
        case LAB_BLUE_LIGHT: return RGB(0.35f, 0.5f, 1.0f);
        case LAB_UV_LIGHT: return RGB(0.55f, 0.3f, 1.0f);
        case LAB_ICE: return ICE_COLOUR;
        case LAB_TISSUE: return TISSUE_COLOUR;
        default: return RGB(0.86f, 0.8f, 0.62f);
    }
}
// a flask's look now: its liquid's home (its crowd's pigment, a broth's cloud; its own light when it glows), how cloudy (0 clear .. 1 a crowd), and
// its light's home (-1 dark; the white the light's bank); J-01: its liquid its dye's, its ice or its tissue (solid: no light through it),
// overgrown up its neck, its sediment's home (-1 none) and depth, a scum on top
struct FlaskLook {
    int liquid = LAB_CLOUD; double cloud = 0; int light = -1; bool glows = false;
    bool solid = false, overgrown = false; int sediment = -1; double settled = 0, scum = 0;
};
FlaskLook flaskLook(const Flask& F, const CultureNow& now, const FlaskShow& S) {
    FlaskLook L;
    if (F.n == 0) return L;
    double best = 0;
    for (int k = 0; k < now.n; k++) {
        const double c = now.one[k].cells + 0.3 * now.one[k].spores;
        if (c > best) { best = c; const int pg = F.culture.kind[k].s.pigment; L.liquid = pg > 0 ? LAB_TONE + pg : LAB_CLOUD; }
        L.glows = L.glows || now.one[k].glowing;
    }
    L.cloud = clampd((std::log10(std::max(1.0, best)) - 3) / 6, 0, 1);
    L.light = F.medium.light == CL_WHITE ? BANK_LIGHT : (F.medium.light == CL_RED ? LAB_RED_LIGHT : (F.medium.light == CL_BLUE ? LAB_BLUE_LIGHT :
              (F.medium.light == CL_UV ? LAB_UV_LIGHT : -1)));
    if (S.frozen) { L.liquid = LAB_ICE; L.solid = true; }
    else if (S.overgrown) { L.overgrown = true; L.solid = true; if (S.tissue) L.liquid = LAB_TISSUE; L.cloud = 1; }
    else if (S.dyed > 0.15) { L.liquid = labDyeHome(S.dye); L.solid = true; }
    if (S.sediment > 0.05) { L.sediment = LAB_SEDIMENT + S.shell; L.settled = S.sediment; }
    L.scum = S.frozen || S.overgrown ? 0 : S.scum;
    return L;
}

// a photograph's tone: its luminance in 64 x 64 cells (each its block's mean), exposed so its brightest fiftieth prints white
void toneOf(const std::vector<uint32_t>& px, int w, int h, GrainTexture& out) {
    out.resize(64);
    std::vector<double> lum(64 * 64, 0.0);
    for (int j = 0; j < 64; j++)
        for (int i = 0; i < 64; i++) {
            const int x0 = i * w / 64, x1 = std::max(x0 + 1, (i + 1) * w / 64), y0 = j * h / 64, y1 = std::max(y0 + 1, (j + 1) * h / 64);
            double s = 0; int n = 0;
            for (int y = y0; y < y1; y++)
                for (int x = x0; x < x1; x++) {
                    const uint32_t c = px[(size_t)y * w + x];
                    s += 0.299 * (c & 255) + 0.587 * ((c >> 8) & 255) + 0.114 * ((c >> 16) & 255);
                    n++;
                }
            lum[j * 64 + i] = s / (std::max(1, n) * 255.0);
        }
    std::vector<double> sorted(lum);
    std::sort(sorted.begin(), sorted.end());
    const double k = 1.0 / std::max(0.25, sorted[(size_t)(0.98 * (sorted.size() - 1))]);
    for (int i = 0; i < 64 * 64; i++) out.v[i] = (int8_t)std::lround((clampd(lum[i] * k, 0, 1) - 0.5) * 126);
}
// R-428: a photograph's thumbnail in its colours, each pixel its block's mean
void thumbOf(const std::vector<uint32_t>& px, int w, int h, std::vector<uint32_t>& out) {
    out.assign((size_t)PRINT_THUMB_W * PRINT_THUMB_H, 0);
    for (int j = 0; j < PRINT_THUMB_H; j++)
        for (int i = 0; i < PRINT_THUMB_W; i++) {
            const int x0 = i * w / PRINT_THUMB_W, x1 = std::max(x0 + 1, (i + 1) * w / PRINT_THUMB_W), y0 = j * h / PRINT_THUMB_H, y1 = std::max(y0 + 1, (j + 1) * h / PRINT_THUMB_H);
            double s[4] = {0, 0, 0, 0}; int n = 0;
            for (int y = y0; y < y1; y++)
                for (int x = x0; x < x1; x++) { const uint32_t c = px[(size_t)y * w + x]; for (int b = 0; b < 4; b++) s[b] += (c >> (8 * b)) & 255; n++; }
            uint32_t c = 0;
            for (int b = 0; b < 4; b++) c |= (uint32_t)std::lround(s[b] / std::max(1, n)) << (8 * b);
            out[(size_t)j * PRINT_THUMB_W + i] = c;
        }
}
}

// R-428: the photographs' numbers in a directory (`screenshot_<n>.png`), the newest first
std::vector<int> Game::photographNumbers(const std::string& dir) {
    std::vector<int> nums;
    if (DIR* d = opendir(dir.c_str())) {
        while (dirent* e = readdir(d)) {
            const std::string n = e->d_name;
            if (n.rfind("screenshot_", 0) == 0 && n.size() > 15 && n.substr(n.size() - 4) == ".png") nums.push_back(atoi(n.c_str() + 11));
        }
        closedir(d);
    }
    std::sort(nums.begin(), nums.end(), [](int a, int b) { return a > b; });
    return nums;
}

// ---- the frame's asks: the prints made, the garden's home, the shelves' worlds -------------------------------------------------------

// what the seen rooms hold this frame, before the banks are dealt: the prints come in from their thread (asked for again when a photograph
// was taken or the directory changed, the quarters seen), the home world's garden (the hall seen), the shards held of each world and the
// decoder's bank for them (the library seen)
void Game::lifeSight(CabinFrame& f, std::vector<ShipBankAsk>& asks) {
    printsCollect();
    if (f.sight.seen[ROOM_QUARTERS] && (prints.stale || prints.dir != shotsDir)) printsAsk();
    if (f.sight.seen[ROOM_HALL]) gardenMake();
    if (f.sight.seen[ROOM_LAB]) {   // Z-05: the flasks' tones and lights (after the screens near them: they dim first)
        for (const ShipFixture& F : shipFixtures()) {
            if (F.kind != FX_INCUBATOR) continue;
            const Vec3 mid(0.25 * (F.x[0] + F.x[1] + F.x[2] + F.x[3]), F.y[0] + 0.5 * F.height, 0.25 * (F.z[0] + F.z[1] + F.z[2] + F.z[3]));
            if ((f.look * (mid - f.eye)).z <= 0) continue;   // behind the eye: nothing of it drawn
            const double near = std::hypot(mid.x - f.eye.x, mid.z - f.eye.z) + 1.5;
            for (int i = 0; i < INCUBATOR_FLASKS; i++) {   // the tone each is drawn in (`drawIncubator`'s), its light's for the shelf
                const Flask& L = lab.flasks[i];
                if (L.n == 0 || !L.made) continue;
                const FlaskLook k = flaskLook(L, flaskNow(i), flaskShow(i));
                const int home = k.glows && k.light < 0 && !k.solid ? LAB_GLOW : (k.light >= 0 && k.cloud <= 0.6 && !k.solid ? k.light : k.liquid);
                if (home >= 32 && home != LAB_ICE) asks.push_back({home, labToneColour(home), near});   // (J-01: ice is the glass's own, pale)
                if (k.light >= 32 && k.light != home) asks.push_back({k.light, labToneColour(k.light), near});
                if (k.sediment >= 32) asks.push_back({k.sediment, labToneColour(k.sediment), near});   // J-01
            }
        }
    }
    if (!f.sight.seen[ROOM_LIBRARY]) return;
    if (shelfShards != guide.shards.size()) {   // a world's shards are its keys' run in the set (`<body key>/S<index>`)
        shelfShards = guide.shards.size();
        shelfCounts.clear();
        std::string last;
        for (const std::string& k : guide.shards) {
            const size_t at = k.rfind("/S");
            if (at == std::string::npos) continue;
            if (shelfCounts.empty() || k.compare(0, at, last) != 0) { shelfCounts.push_back(0); last = k.substr(0, at); }
            shelfCounts.back()++;
        }
    }
    if (shelfCounts.empty()) return;
    const CabinDevice& dec = cabinDevices().row[FACE_DECODER];
    double near = 1e30;
    for (const ShipFixture& F : shipFixtures()) if (F.kind == FX_SHELF) near = std::min(near, std::hypot(F.x[1] - f.eye.x, F.z[1] - f.eye.z));
    asks.push_back({dec.bank, dec.col, near});
}

// the wall's photographs made into prints on a thread of their own (each read, its tone and its thumbnail made): the frames the explorer
// hung (R-428), else the directory's newest twelve in order; the frame never waits for it (`printsCollect` takes what it made)
void Game::printsAsk() {
    if (prints.busy.load(std::memory_order_acquire)) return;
    if (prints.job.joinable()) prints.job.join();
    prints.stale = false; prints.dir = shotsDir;
    prints.busy.store(true, std::memory_order_release);
    std::vector<int> hung;
    if (wall.chosen) hung.assign(wall.frame, wall.frame + SHIP_PRINTS);
    prints.job = std::thread([this, dir = shotsDir, hung]() {
        pngFlush();   // a photograph still with the writer is on disk first
        std::vector<std::pair<int, int>> want;   // (its frame, -1 the next free; its number)
        if (hung.empty()) for (int n : photographNumbers(dir)) want.push_back({-1, n});
        else for (int k = 0; k < SHIP_PRINTS; k++) if (hung[k] >= 0) want.push_back({k, hung[k]});
        std::vector<ShipPrint> out;
        for (const auto& fw : want) {
            if ((int)out.size() >= SHIP_PRINTS) break;
            std::vector<uint32_t> px; int w = 0, h = 0;
            if (!readPNG(fmt("%s/screenshot_%d.png", dir.c_str(), fw.second).c_str(), px, w, h) || w < 64 || h < 64) continue;
            ShipPrint p; p.number = fw.second; p.frame = fw.first >= 0 ? fw.first : (int)out.size();
            toneOf(px, w, h, p.tone); thumbOf(px, w, h, p.thumb);
            std::ifstream cap(fmt("%s/screenshot_%d.txt", dir.c_str(), fw.second));
            if (cap) std::getline(cap, p.caption);
            out.push_back(std::move(p));
        }
        prints.made = std::move(out);
        prints.busy.store(false, std::memory_order_release);
    });
}
void Game::printsCollect() {
    if (!prints.job.joinable() || prints.busy.load(std::memory_order_acquire)) return;
    prints.job.join();
    prints.shown = std::move(prints.made);
    prints.made.clear();
}

// the home world's flora for the garden: the guide's home star's first world with flora (its two silhouettes, their looks, their colours),
// else the pinned home's; made once a home
void Game::gardenMake() {
    const std::string pinned = Guide::starKey(HOME_SX, HOME_SY, HOME_SZ), key = guide.home.empty() ? pinned : guide.home;
    if (garden.key == key) return;
    garden = ShipGarden();
    garden.key = key;
    auto fromStar = [&](const std::string& k) {
        int64_t sx, sy, sz; Star st;
        if (!Guide::parseStarKey(k, sx, sy, sz) || !placeInSector(sx, sy, sz, st, true)) return false;
        StarSystem s; s.generate(st);
        for (const Body& b : s.bodies) {
            if (b.parent >= 0 || !kindHas(b.type, KT_FLORA)) continue;
            const BodyGen g = BodyGen::make(b);
            for (int i = 0; i < 2; i++) garden.fam[i] = clampi(i == 0 ? g.floraFamily : g.floraFamily2, 0, 9);
            for (int i = 0; i < 2; i++) garden.look[i] = treeLookOf(g.seed, garden.fam[i]);
            garden.col[0] = g.vegColor; garden.col[1] = g.vegColor2; garden.seed = g.seed; garden.ok = true;
            return true;
        }
        return false;
    };
    if (!fromStar(key) && key != pinned) fromStar(pinned);
}

// a finish's colour as the frame asks for it: the garden's two the home world's flora's
RGB Game::finishColour(int k) const {
    if (garden.ok && (k == FIN_PLANT || k == FIN_PLANT2)) return garden.col[k == FIN_PLANT ? 0 : 1];
    return shipFinishes()[k].col;
}

// R-429: a bed grown by the time played since it was last trimmed (or the expedition began): a third of its height then, as planted, its
// full height an hour of play on, eased in and out, and no further
double Game::gardenGrowth(int bed) const {
    const double u = clampd((played - bedTrimmed[clampi(bed, 0, GARDEN_BEDS - 1)]) / GARDEN_GROW_S, 0, 1);
    return GARDEN_START + (1 - GARDEN_START) * u * u * (3 - 2 * u);
}
// R-429: E at a bed: trimmed back to its starting size, to grow again
void Game::gardenTrim(int bed) {
    if (bed < 0 || bed >= GARDEN_BEDS) return;
    if (gardenGrowth(bed) < GARDEN_TRIM_MIN) { refuse("NOTHING TO TRIM - STILL GROWING BACK", 3); return; }
    bedTrimmed[bed] = played;
    status("BED TRIMMED - IT GROWS BACK AS YOU PLAY", 4);
    audio.beep = BEEP_UI;
}

// the hatch's drop: a descent's first seconds (before its view) with the ship drawn, seen from the hangar
bool Game::cabinDropping() const { return settings.cabin && state == GameState::DESCENT && transT < 0; }
// the cabin as the frame sees it: the explorer's, or the hatch's drop's camera (the plan's view), the explorer's place kept
Cabin Game::cabinSeen() const {
    if (!cabinDropping()) return cabin;
    Cabin v = cabin;
    const ShipView& w = shipView("THE HATCH'S DROP");
    v.placeAt(shipPlaceOf(w.room, w.at.x, w.at.z));
    v.yaw = w.at.yaw; v.pitch = w.at.pitch;
    return v;
}

// a place's shade as the plan's vertices take theirs: its room's lamps at U's level (the glows' share at its night), a factor
double Game::lifeShade(const CabinFrame& f, int room, const Vec3& p, const Vec3* n, double a) const {
    double lamp, night;
    shipLampLight(room, p, n, lamp, night);
    return CABIN_SHADE_FULL * (CABIN_AMBIENT + f.lit * lamp + (1 - f.lit) * SHIP_NIGHT_GLOW * night) * a;
}

// ---- the pieces ---------------------------------------------------------------------------------------------------------------------

void Game::drawLifePiece(CabinFrame& f, const ShipPiece& pc, const CabinDraw& d) {
    if (pc.fixture < 0 || pc.fixture >= shipFixtures().count) return;
    const ShipFixture& F = shipFixtures()[pc.fixture];
    switch (pc.kind) {
        case PIECE_PRINTS: drawPrints(f, F, d); break;
        case PIECE_SHELF: {
            int unit = 0;
            for (int i = 0; i < pc.fixture; i++) if (shipFixtures()[i].kind == FX_SHELF) unit++;
            drawShelf(f, F, unit, d);
            break;
        }
        case PIECE_GARDEN: drawGarden(f, F, d); break;
        case PIECE_CORE: drawCore(f, F, d); break;
        case PIECE_DISPLAY: drawDisplay(f, F, d); break;
        case PIECE_DECK: drawDeck(f, F, d); break;
        case PIECE_CAPSULE: drawCapsule(f, F, d); break;
        case PIECE_STORE: drawStore(f, F, d); break;
        case PIECE_INCUBATOR: drawIncubator(f, F, d); break;
        case PIECE_TREE: drawTreeWall(f, F, d); break;
        case PIECE_VEHICLE: drawVehicle(f, F); break;   // R-427: vehicles_draw.cpp
        default: break;
    }
}

// Z-04: the cold store's door, lit by how full it is: its lamps in a column by its hinge, lit in the microscope's dealt bank (the laboratory's
// teal), dim the rest; R-432: the store without limit, its lamps by doublings (the first lit by a vial, the second by two, the eighth by 128)
void Game::drawStore(CabinFrame& f, const ShipFixture& F, const CabinDraw& d) {
    const BoxAt B(F);
    if (dot(B.front(), f.eye - B.at(0.5, 0, 1.2)) <= 0) return;
    const int dealt = f.banks.bankOf(cabinDevices().row[FACE_MICROSCOPE].bank);
    RasterParams lp = *d.light;
    if (dealt >= 0) lp.bank = dealt;
    const int n = std::max(1, F.slots), held = (int)coldStore.size();
    const double w = B.width(), ta = 0.1, tb = ta + 0.045 / w;
    for (int k = 0; k < n; k++) {
        const bool lit = held >= (1 << k);
        const double y = 0.95 + k * 0.09;
        const Vec3 p[4] = {B.at(ta, -0.035, y), B.at(tb, -0.035, y), B.at(tb, -0.035, y + 0.045), B.at(ta, -0.035, y + 0.045)};
        RVert q[4];
        for (int i = 0; i < 4; i++) q[i] = vertAt(f, p[i], lit ? 54 : 22);
        f.shapes.polygon(q, 4, lp);
    }
}

// Z-05: the incubator's flasks on its three shelves, four a shelf: each a flask's outline in its glass and its liquid (its culture's tone, as
// cloudy as its crowd; lit through by its light, aglow with a luminous crowd's own), the cabinet's lamp behind them
void Game::drawIncubator(CabinFrame& f, const ShipFixture& F, const CabinDraw& d) {
    const BoxAt B(F);
    if (dot(B.front(), f.eye - B.at(0.5, 0, F.height * 0.5)) <= 0) return;
    const double w = B.width(), H = F.height;
    auto quad = [&](const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& e, double shade, const RasterParams& p) {
        RVert q[4] = {vertAt(f, a, shade), vertAt(f, b, shade), vertAt(f, c, shade), vertAt(f, e, shade)};
        f.shapes.polygon(q, 4, p);
    };
    {   // the lamp along the cabinet's back, under its top
        RasterParams lp = *d.light;
        quad(B.at(0.04, 0.95, H - 0.05), B.at(0.96, 0.95, H - 0.05), B.at(0.96, 0.95, H - 0.02), B.at(0.04, 0.95, H - 0.02), 40, lp);
    }
    for (int i = 0; i < INCUBATOR_FLASKS && i < F.boards * F.slots; i++) {
        const int shelf = F.boards - 1 - i / F.slots, col = i % F.slots;   // flask 1 at the top left
        const double y0 = shelf * H / F.boards + 0.012, tc = (col + 0.5) / F.slots, back = 0.45;
        const double bw = 0.055 / w, nw = 0.017 / w, bodyH = 0.11, neckH = 0.06;
        const Flask& L = lab.flasks[i];
        // its glass: the body and the neck
        const Vec3 g[6] = {B.at(tc - bw, back, y0), B.at(tc + bw, back, y0), B.at(tc + nw, back, y0 + bodyH), B.at(tc + nw, back, y0 + bodyH + neckH),
                           B.at(tc - nw, back, y0 + bodyH + neckH), B.at(tc - nw, back, y0 + bodyH)};
        RVert q[6];
        for (int k = 0; k < 6; k++) q[k] = vertAt(f, g[k], lifeShade(f, F.room, g[k], nullptr, 0.35));
        f.shapes.polygon(q, 6, d.plain[FIN_GLASS]);
        if (L.n == 0 || !L.made) continue;
        // its liquid: its tone, lit through by its light, aglow with its crowd's own (J-01: its dye, its ice or its tissue its own, up its neck
        // where it overgrew; its sediment under it, a scum on top)
        const FlaskLook k = flaskLook(L, flaskNow(i), flaskShow(i));
        int home = k.liquid;
        double shade = 14 + 26 * k.cloud;
        if (k.solid) shade = 30 + 12 * k.cloud;
        else if (k.glows && k.light < 0) { home = LAB_GLOW; shade = 56; }
        else if (k.light >= 0) { home = k.cloud > 0.6 ? k.liquid : k.light; shade = 40 + 14 * (1 - k.cloud); }
        auto paint = [&](int h, double& sh) {
            RasterParams p = h == BANK_LIGHT ? *d.light : d.plain[FIN_GLASS];
            if (h == LAB_ICE) { sh = 54; return p; }   // J-01: ice in the glass's tone, bright
            if (h != BANK_LIGHT) { const int b = f.banks.bankNear(h); if (b < 0) { p = d.plain[FIN_GLASS]; sh *= 0.6; } else p.bank = b; }   // (KI-455: else the
            // nearest dealt colour's)
            return p;
        };
        const RasterParams lp = paint(home, shade);
        const double ly = k.overgrown ? bodyH : 0.065, lw = bw - (bw - nw) * (ly / bodyH);
        const Vec3 p[4] = {B.at(tc - bw * 0.92, back - 0.02, y0 + 0.004), B.at(tc + bw * 0.92, back - 0.02, y0 + 0.004), B.at(tc + lw * 0.92, back - 0.02, y0 + ly),
                           B.at(tc - lw * 0.92, back - 0.02, y0 + ly)};
        quad(p[0], p[1], p[2], p[3], shade, lp);
        if (k.overgrown)   // up its neck
            quad(B.at(tc - nw * 0.85, back - 0.02, y0 + bodyH), B.at(tc + nw * 0.85, back - 0.02, y0 + bodyH), B.at(tc + nw * 0.85, back - 0.02, y0 + bodyH + 0.7 * neckH),
                 B.at(tc - nw * 0.85, back - 0.02, y0 + bodyH + 0.7 * neckH), shade, lp);
        if (k.sediment >= 0) {
            double ss = 34;
            const RasterParams sp = paint(k.sediment, ss);
            const double sy = 0.004 + 0.003 + 0.01 * k.settled, sw = bw - (bw - nw) * (sy / bodyH);
            quad(B.at(tc - bw * 0.92, back - 0.025, y0 + 0.004), B.at(tc + bw * 0.92, back - 0.025, y0 + 0.004), B.at(tc + sw * 0.92, back - 0.025, y0 + sy),
                 B.at(tc - sw * 0.92, back - 0.025, y0 + sy), ss, sp);
        }
        if (k.scum > 0.05) {
            const double sy = ly - 0.004 - 0.004 * k.scum, sw = bw - (bw - nw) * (sy / bodyH);
            quad(B.at(tc - sw * 0.92, back - 0.025, y0 + sy), B.at(tc + sw * 0.92, back - 0.025, y0 + sy), B.at(tc + lw * 0.92, back - 0.025, y0 + ly),
                 B.at(tc - lw * 0.92, back - 0.025, y0 + ly), std::min(63.0, shade + 14), lp);
        }
        if (k.light >= 0) {   // its light's glow on the shelf under it
            RasterParams gl = k.light == BANK_LIGHT ? *d.light : lp;
            if (k.light != BANK_LIGHT) { const int b = f.banks.bankOf(k.light); if (b >= 0) gl.bank = b; }
            quad(B.at(tc - bw * 1.6, back + 0.25, y0 - 0.008), B.at(tc + bw * 1.6, back + 0.25, y0 - 0.008), B.at(tc + bw * 1.6, back - 0.3, y0 - 0.008),
                 B.at(tc - bw * 1.6, back - 0.3, y0 - 0.008), 26, gl);
        }
    }
    // its door: the glass (what is behind a little darker, a faint sheen of the room's light) in its dark frame, its handle
    if (d.tint) {
        RVert q[4];
        const Vec3 c[4] = {B.at(0.02, -0.01, 0.02), B.at(0.98, -0.01, 0.02), B.at(0.98, -0.01, H - 0.02), B.at(0.02, -0.01, H - 0.02)};
        for (int i = 0; i < 4; i++) q[i] = vertAt(f, c[i], lifeShade(f, F.room, c[i], nullptr, 0.1));
        f.shapes.polygon(q, 4, *d.tint);
    }
    const double fr = 0.025 / w, sh = lifeShade(f, F.room, B.at(0.5, 0, H * 0.5), nullptr, 0.3);
    quad(B.at(0, -0.012, 0), B.at(fr, -0.012, 0), B.at(fr, -0.012, H), B.at(0, -0.012, H), sh, d.fin[FIN_PANEL]);
    quad(B.at(1 - fr, -0.012, 0), B.at(1, -0.012, 0), B.at(1, -0.012, H), B.at(1 - fr, -0.012, H), sh, d.fin[FIN_PANEL]);
    quad(B.at(0, -0.012, H - 0.025), B.at(1, -0.012, H - 0.025), B.at(1, -0.012, H), B.at(0, -0.012, H), sh, d.fin[FIN_PANEL]);
    quad(B.at(0, -0.012, 0), B.at(1, -0.012, 0), B.at(1, -0.012, 0.025), B.at(0, -0.012, 0.025), sh, d.fin[FIN_PANEL]);
    quad(B.at(0.93, -0.03, H * 0.38), B.at(0.945, -0.03, H * 0.38), B.at(0.945, -0.03, H * 0.62), B.at(0.93, -0.03, H * 0.62),
         lifeShade(f, F.room, B.at(0.94, 0, H * 0.5), nullptr, 0.9), d.plain[FIN_BRASS]);
}

// the photographs on the quarters' wall, the newest at the top left: each in a dark frame, printed in one tone (the print's bank, its
// luminance the grain's offset, as strong as the light on it)
void Game::drawPrints(CabinFrame& f, const ShipFixture& F, const CabinDraw& d) {
    if (prints.shown.empty()) return;
    const BoxAt B(F);
    const Vec3 out = B.front(), c = B.A[2], b = B.A[1];
    if (dot(out, f.eye - b) <= 0) return;
    const double width = length(b - c), gap = 0.12;
    const Vec3 along = (b - c) * (1.0 / width);
    const double pw = (width - (F.slots - 1) * gap) / F.slots, ph = (F.height - (F.boards - 1) * gap) / F.boards;
    auto P = [&](double u, double v, double proud) { return c + along * u + Vec3(0, v, 0) + out * proud; };
    for (const ShipPrint& sp : prints.shown) {
        const int k = sp.frame;   // R-428: its frame (the explorer's, or its place among the newest)
        if (k < 0 || k >= F.slots * F.boards) continue;
        const int row = k / F.slots, col = k % F.slots;
        const double u0 = col * (pw + gap), u1 = u0 + pw, v1 = F.height - row * (ph + gap), v0 = v1 - ph;
        const Vec3 fr[4] = {P(u0 - 0.035, v0 - 0.035, 0.01), P(u1 + 0.035, v0 - 0.035, 0.01), P(u1 + 0.035, v1 + 0.035, 0.01), P(u0 - 0.035, v1 + 0.035, 0.01)};
        RVert q[4];
        for (int i = 0; i < 4; i++) q[i] = vertAt(f, fr[i], lifeShade(f, F.room, fr[i], &out, 0.55));
        f.shapes.polygon(q, 4, d.fin[FIN_DECK]);
        const Vec3 pt[4] = {P(u0, v0, 0.018), P(u1, v0, 0.018), P(u1, v1, 0.018), P(u0, v1, 0.018)};
        const double tu[4] = {0.5, 63.5, 63.5, 0.5}, tv[4] = {63.5, 63.5, 0.5, 0.5};
        double mean = 0;
        for (int i = 0; i < 4; i++) { q[i] = vertAt(f, pt[i], lifeShade(f, F.room, pt[i], &out, 0.95)); q[i].u = tu[i]; q[i].v = tv[i]; mean += q[i].shade / 4; }
        RasterParams pr = d.plain[FIN_PRINT];
        pr.grain = &sp.tone; pr.grainScale = 1; pr.grainWeight = clampd(mean / 79, 0, 1);
        f.shapes.polygon(q, 4, pr);
    }
}

// a shelf's worlds (its boards from the top, left to right), each world's shards standing on its board: five tablets at most, aglow in the
// decoder's bank by how many of the world's are held (a dozen and more the brightest)
void Game::drawShelf(CabinFrame& f, const ShipFixture& F, int unit, const CabinDraw& d) {
    const int per = F.boards * F.slots, first = unit * per;
    if (first >= (int)shelfCounts.size()) return;
    const BoxAt B(F);
    if (dot(B.front(), f.eye - B.at(0.5, 0, 1)) <= 0) return;
    const int dealt = f.banks.bankOf(cabinDevices().row[FACE_DECODER].bank);
    RasterParams sp; sp.bank = dealt >= 0 ? dealt : BANK_HULL;
    const double width = B.width();
    for (int s = 0; s < per && first + s < (int)shelfCounts.size(); s++) {
        const int board = F.boards - 1 - s / F.slots, slot = s % F.slots, held = shelfCounts[first + s];
        const double y = 0.1 + board * (F.height - 0.1) / F.boards, t0 = 0.05 + slot * 0.9 / F.slots;
        const double glow = (16 + 40 * std::min(1.0, held / 12.0)) * (dealt >= 0 ? 1.0 : 0.45);
        for (int i = 0; i < std::min(held, 5); i++) {
            const uint64_t h = mix64(0x5E1F0ULL + (uint64_t)(first + s) * 977 + (uint64_t)i);
            const double ta = t0 + (0.03 + i * 0.05) / width, tb = ta + 0.03 / width, top = y + 0.15 + 0.1 * hash01(h);
            const Vec3 p[4] = {B.at(ta, 0.35, y), B.at(tb, 0.35, y), B.at(tb, 0.35, top), B.at(ta, 0.35, top)};
            const double shade = glow * (0.85 + 0.15 * hash01(mix64(h + 1)));
            RVert q[4];
            for (int k = 0; k < 4; k++) q[k] = vertAt(f, p[k], shade);
            f.shapes.polygon(q, 4, sp);
        }
    }
}

// the instruments held against the case's lit glass (its back light the case's screens), the top shelf first: a silhouette each in brass,
// its form by its key (a body and a neck, a pipe, a drum, a horn: the make is the case's screen's to read)
void Game::drawDisplay(CabinFrame& f, const ShipFixture& F, const CabinDraw& d) {
    if (guide.instruments.empty()) return;
    const BoxAt B(F);
    const Vec3 out = B.front(), b = B.A[1];
    if (dot(out, f.eye - b) <= 0) return;
    const double width = B.width();
    const Vec3 along = (B.A[2] - b) * (1.0 / width);
    auto P = [&](double u, double y) { return b + along * u + Vec3(0, y, 0) + out * 0.026; };
    auto poly = [&](const Vec3* p, int n) {
        RVert q[10];
        for (int i = 0; i < n; i++) q[i] = vertAt(f, p[i], lifeShade(f, F.room, p[i], &out, 0.9));
        f.shapes.polygon(q, n, d.fin[FIN_BRASS]);
    };
    int i = 0;
    for (const std::string& key : guide.instruments) {
        if (i >= F.boards * F.slots) break;
        const int shelf = F.boards - 1 - i / F.slots, place = i % F.slots;
        const double lo = F.screenLo + shelf * F.rise + 0.03, u = F.depth + (place + 0.5) * (width - 2 * F.depth) / F.slots;
        const uint64_t h = fnv1a(key);
        const double tall = (0.19 + 0.09 * hash01(mix64(h))) * F.screenHi / 0.34;
        switch ((int)(h % 4)) {
            case 0: {   // a body and a neck
                Vec3 body[8];
                for (int k = 0; k < 8; k++) body[k] = P(u + 0.055 * std::cos(k * TAU / 8), lo + 0.06 + 0.055 * std::sin(k * TAU / 8));
                poly(body, 8);
                const Vec3 neck[4] = {P(u - 0.009, lo + 0.1), P(u + 0.009, lo + 0.1), P(u + 0.009, lo + tall), P(u - 0.009, lo + tall)};
                poly(neck, 4);
                break;
            }
            case 1: { const Vec3 pipe[4] = {P(u - 0.013, lo), P(u + 0.013, lo), P(u + 0.013, lo + tall), P(u - 0.013, lo + tall)}; poly(pipe, 4); break; }
            case 2: {
                const double top = lo + 0.11;
                const Vec3 drum[4] = {P(u - 0.06, lo), P(u + 0.06, lo), P(u + 0.06, top), P(u - 0.06, top)};
                poly(drum, 4);
                Vec3 skin[8];
                for (int k = 0; k < 8; k++) skin[k] = P(u + 0.06 * std::cos(k * TAU / 8), top + 0.016 * std::sin(k * TAU / 8));
                poly(skin, 8);
                break;
            }
            default: { const Vec3 horn[4] = {P(u - 0.012, lo), P(u + 0.012, lo), P(u + 0.055, lo + tall), P(u - 0.055, lo + tall)}; poly(horn, 4); break; }
        }
        i++;
    }
}

// the player's deck: two reels on its top (a rim, three spokes, a hub), turning while the player plays, its lamp lit then
void Game::drawDeck(CabinFrame& f, const ShipFixture& F, const CabinDraw& d) {
    const BoxAt B(F);
    const double top = F.height + 0.012;
    if (f.eye.y <= B.A[1].y + top) return;
    const Vec3 up(0, 1, 0);
    const int bank = d.plain[FIN_BRASS].bank;
    const bool playing = player.playing >= 0 && !player.paused && !player.held;
    for (int r = 0; r < 2; r++) {
        const Vec3 c = B.at(r == 0 ? 0.3 : 0.7, 0.5, top);
        const Vec3 u = normalize(B.A[2] - B.A[1]), w = normalize(B.A[0] - B.A[1]);
        const double R = 0.085, turn = cabin.deckTurn * (r == 0 ? 1.0 : 0.8);
        const double shade = lifeShade(f, F.room, c, &up, 0.9);
        auto rim = [&](double a, double rr) { return c + u * (std::cos(a) * rr) + w * (std::sin(a) * rr); };
        RVert disc[8];
        for (int k = 0; k < 8; k++) disc[k] = vertAt(f, rim(turn + k * TAU / 8, R), shade * 0.3);
        f.shapes.polygon(disc, 8, d.fin[FIN_DECK]);
        const Vec3 lift(0, 0.006, 0);
        for (int k = 0; k < 8; k++) {
            RVert a = vertAt(f, rim(turn + k * TAU / 8, R) + lift, shade), e = vertAt(f, rim(turn + (k + 1) * TAU / 8, R) + lift, shade);
            f.shapes.line(a, e, bank, true, 1);
        }
        for (int k = 0; k < 3; k++) {
            RVert a = vertAt(f, rim(turn + k * TAU / 3, 0.015) + lift, shade), e = vertAt(f, rim(turn + k * TAU / 3, R) + lift, shade);
            f.shapes.line(a, e, bank, true, 1);
        }
    }
    RVert lamp = vertAt(f, B.at(0.5, 0.15, top + 0.01), playing ? 60 : 14);
    f.shapes.point(lamp, BANK_LIGHT, true, 1);
}

// a bed of the garden: three of the home world's plants (its two silhouettes, their looks and colours), grown by the time played since the
// bed was trimmed (R-429) and swaying in the air system's draught, and low tufts on its soil; a plant a tapered trunk (four faces) and a
// canopy of leaf clusters, each a disc facing the eye, laid out by its silhouette (round, a cone, a flat crown, a tuft)
void Game::drawGarden(CabinFrame& f, const ShipFixture& F, const CabinDraw& d) {
    if (!garden.ok) return;
    const BoxAt B(F);
    const Vec3 mid = B.at(0.5, 0.5, F.height);
    const int bed = F.slots;
    const double soil = mid.y, g = gardenGrowth(bed);
    const Vec3 right = rowOf(f.look, 0), upv = rowOf(f.look, 1);
    const Vec3 draught = normalize(Vec3(-mid.z, 0, mid.x));   // round the hall, from the air system's vents
    RasterParams leaf[2];   // the two greens with the leaves' own grain
    for (int i = 0; i < 2; i++) { leaf[i] = d.fin[i ? FIN_PLANT2 : FIN_PLANT]; leaf[i].grain = &leafGrain; leaf[i].grainScale = 22.0 * FB_SCALE; leaf[i].grainWeight = 1; }
    auto cluster = [&](const Vec3& C, double cr, double shade, const RasterParams& prm, uint64_t h) {
        RVert q[7];
        const double rot = hash01(h) * TAU;
        for (int j = 0; j < 7; j++) {
            const double a = rot + j * TAU / 7, rr = cr * (0.84 + 0.32 * hash01(mix64(h + 3 + j)));
            q[j] = vertAt(f, C + right * (std::cos(a) * rr) + upv * (std::sin(a) * rr * 0.9), shade);
        }
        f.shapes.polygon(q, 7, prm);
    };
    for (int p = 0; p < 3; p++) {
        const uint64_t h = mix64(0x6A2DE0ULL + garden.seed + (uint64_t)bed * 131 + (uint64_t)p * 17);
        const int which = p == 1 ? 1 : 0, fam = garden.fam[which];
        const TreeLook& L = garden.look[which];
        const FloraShape S = floraShapeOf(fam);
        const RasterParams& leaves = leaf[which];
        const double ang = bed * 1.7 + p * TAU / 3 + 0.4 * hash01(mix64(h + 1)), rad = 0.42 + 0.12 * hash01(mix64(h + 2));
        const Vec3 base(mid.x + rad * std::sin(ang), soil, mid.z + rad * std::cos(ang));
        const double H = clampd(S.h0 * 0.24, 1.1, 3.0) * (0.8 + 0.35 * hash01(mix64(h + 3))) * g;
        const double ph = hash01(mix64(h + 4)) * TAU;
        const double sway = H * (0.02 * std::sin(realTime * 0.7 + ph) + 0.008 * std::sin(realTime * 2.1 + ph * 1.7));
        const Vec3 lean = Vec3(std::sin(ph), 0, std::cos(ph)) * (L.lean * H);
        auto bend = [&](const Vec3& q) { const double k = clampd((q.y - soil) / std::max(0.1, H), 0, 1.5); return q + draught * (sway * k * k) + lean * k; };
        // the trunk, up to the canopy's middle
        const double cy = S.cy * H, w0 = std::max(0.02, 0.03 * H * L.trunkW), w1 = w0 * 0.45;
        const Vec3 top = base + Vec3(0, cy, 0);
        for (int k = 0; k < 4; k++) {
            const double a0 = k * TAU / 4 + 0.6, a1 = (k + 1) * TAU / 4 + 0.6;
            const Vec3 o0(std::sin(a0), 0, std::cos(a0)), o1(std::sin(a1), 0, std::cos(a1)), n = normalize(o0 + o1);
            const Vec3 pts[4] = {base + o0 * w0, base + o1 * w0, bend(top + o1 * w1), bend(top + o0 * w1)};
            if (dot(n, f.eye - pts[0]) < 0) continue;
            RVert q[4];
            for (int i = 0; i < 4; i++) q[i] = vertAt(f, pts[i], lifeShade(f, F.room, pts[i], &n, 0.5));
            f.shapes.polygon(q, 4, d.fin[FIN_PANEL]);
        }
        // the canopy
        const double R = S.r * H * L.aspect;
        const int n = clampi((int)std::lround(7 + 6 * L.count), 7, 13);
        const Vec3 toEye = normalize(Vec3(f.eye.x - top.x, 0, f.eye.z - top.z));
        const int mode = (fam == 1 || fam == 9) ? 1 : ((fam == 2 || fam == 5 || fam == 6) ? 2 : (fam == 4 ? 3 : 0));
        for (int c = 0; c < n; c++) {
            const uint64_t hc = mix64(h + 100 + (uint64_t)c);
            Vec3 o;
            if (mode == 0) {   // round: on a squashed ball (the weeping's rim hanging)
                const double yy = 1 - 2 * (c + 0.5) / n, rr = std::sqrt(std::max(0.0, 1 - yy * yy)), th = c * 2.39996;
                o = Vec3(R * rr * std::sin(th) * 0.75, R * yy * 0.55 - L.droop * R * 0.4 * rr, R * rr * std::cos(th) * 0.75);
            } else if (mode == 1) {   // a cone: rings narrowing up
                const double k = n > 1 ? (double)c / (n - 1) : 0.5, rr = R * (1 - k) * 0.9, th = c * 2.4;
                o = Vec3(rr * std::sin(th), (k - 0.5) * 1.6 * R, rr * std::cos(th));
            } else if (mode == 2) {   // a flat crown, its rim arching down
                const double th = c * TAU / std::max(1, n - 1), rr = c == 0 ? 0.0 : R * 0.65;
                o = Vec3(rr * std::sin(th), R * 0.3 - (c == 0 ? 0.0 : L.droop * R * 0.3), rr * std::cos(th));
            } else {   // a tuft at the stalk's top
                const double th = c * 2.39996, rr = R * 0.35 * hash01(hc);
                o = Vec3(rr * std::sin(th), R * 0.4 * hash01(mix64(hc + 1)), rr * std::cos(th));
            }
            const Vec3 C = bend(top + o);
            const double cr = std::max(0.05, 0.62 * R * L.clusterR * (0.75 + 0.5 * hash01(mix64(hc + 2))));
            const double up = clampd((C.y - soil) / std::max(0.1, H), 0, 1.2), front = R > 1e-6 ? dot(Vec3(o.x, 0, o.z), toEye) / R : 0;   // the eye's side lighter
            cluster(C, cr, lifeShade(f, F.room, C, nullptr, (0.6 + 0.3 * up + 0.18 * front) * L.tone * (0.85 + 0.15 * hash01(mix64(hc + 3)))), leaves, hc);
        }
    }
    for (int k = 0; k < 5; k++) {   // the tufts on the soil
        const uint64_t h = mix64(0x7F0C0ULL + garden.seed + (uint64_t)bed * 41 + (uint64_t)k);
        const Vec3 C(mid.x + (hash01(h) - 0.5) * 1.8, soil + 0.07, mid.z + (hash01(mix64(h + 1)) - 0.5) * 1.8);
        cluster(C, 0.14 + 0.08 * hash01(mix64(h + 2)), lifeShade(f, F.room, C, nullptr, 0.55), leaf[k % 2], mix64(h + 3));
    }
}

// the drive's core in its pit under the floor glass: a ring and its heart, six vanes turning, in the light's bank; brighter and quicker
// with the drive (a flight)
void Game::drawCore(CabinFrame& f, const ShipFixture& F, const CabinDraw& d) {
    const BoxAt B(F);
    const Vec3 mid = B.at(0.5, 0.5, 0.08);
    if (f.eye.y <= mid.y) return;
    const double spin = realTime * (0.25 + 1.6 * f.drive);
    const double ring = std::min(63.0, 26 + 6 * f.pulse + 30 * f.drive), heart = std::min(63.0, 32 + 8 * f.pulse + 23 * f.drive);
    auto at = [&](double r, double a) { return mid + Vec3(r * std::sin(a), 0, r * std::cos(a)); };
    for (int k = 0; k < 16; k++) {
        const double a0 = k * TAU / 16, a1 = (k + 1) * TAU / 16, s = ring * (0.93 + 0.07 * std::cos(3 * a0 + spin));
        RVert q[4] = {vertAt(f, at(0.45, a0), s), vertAt(f, at(0.78, a0), s), vertAt(f, at(0.78, a1), s), vertAt(f, at(0.45, a1), s)};
        f.shapes.polygon(q, 4, *d.light);
    }
    RVert c[12];
    for (int k = 0; k < 12; k++) c[k] = vertAt(f, at(0.3, k * TAU / 12) + Vec3(0, 0.01, 0), heart);
    f.shapes.polygon(c, 12, *d.light);
    for (int k = 0; k < 6; k++) {
        RVert a = vertAt(f, at(0.3, spin + k * TAU / 6) + Vec3(0, 0.012, 0), heart), e = vertAt(f, at(0.45, spin + k * TAU / 6) + Vec3(0, 0.012, 0), heart);
        f.shapes.line(a, e, BANK_LIGHT, true, 1);
    }
    // the floor glass over it: clear, a light tint
    RasterParams glass = d.plain[FIN_GLASS];
    glass.blend = BLEND_DARKEN; glass.darken = 0.82;
    const double y = F.y[0] + F.height - 0.004;
    const Vec3 g[4] = {B.at(0, 0, F.height - 0.004), B.at(1, 0, F.height - 0.004), B.at(1, 1, F.height - 0.004), B.at(0, 1, F.height - 0.004)};
    RVert q[4];
    for (int k = 0; k < 4; k++) q[k] = vertAt(f, g[k], 0);
    if (f.eye.y > y) f.shapes.polygon(q, 4, glass);
}

// the capsule in its cradle as the surface draws it (its dome on its ring, in the light's bank, lit by the hangar's lamps; outside the
// hull by the sun), the hatch's two leaves under it; in the drop the leaves slide apart under the floor and the capsule falls through
// the hole, the outside seen through it (a pane: depth only)
void Game::drawCapsule(CabinFrame& f, const ShipFixture& F, const CabinDraw& d) {
    const double cx = F.x[0], cz = F.z[0], floor = F.y[0], R = F.ringR, drop = f.drop;
    const double open = drop < 0 ? 0 : smoothstep(0.2, 1.3, drop), below = drop > 1.1 ? 1.3 * (drop - 1.1) * (drop - 1.1) : 0;
    const double outside = clampd(below / 1.5, 0, 1);
    auto shadeOf = [&](const Vec3& p, const Vec3& n, double a) {
        const double in = lifeShade(f, F.room, p, &n, a);
        const double sun = 4 + 46 * f.sunLight * std::max(0.0, dot(n, f.sun)) + 30 * f.worldLight * std::max(0.0, dot(n, f.world));
        return in * (1 - outside) + sun * a * outside;
    };
    const int NA = F.domeSegments, NE = F.domeRings;
    const double rad = F.domeR, y0 = F.domeY - below;
    for (int e = 0; e < NE; e++) {
        const double e0 = (double)e / NE * PI * 0.5, e1 = (double)(e + 1) / NE * PI * 0.5;
        for (int a = 0; a < NA; a++) {
            const double a0 = (double)a / NA * TAU, a1 = (double)(a + 1) / NA * TAU;
            const Vec3 s[4] = {Vec3(std::cos(a0) * std::cos(e0), std::sin(e0), std::sin(a0) * std::cos(e0)), Vec3(std::cos(a1) * std::cos(e0), std::sin(e0), std::sin(a1) * std::cos(e0)),
                               Vec3(std::cos(a1) * std::cos(e1), std::sin(e1), std::sin(a1) * std::cos(e1)), Vec3(std::cos(a0) * std::cos(e1), std::sin(e1), std::sin(a0) * std::cos(e1))};
            const Vec3 n = normalize(s[0] + s[1] + s[2] + s[3]);
            Vec3 p[4];
            for (int i = 0; i < 4; i++) p[i] = Vec3(cx + s[i].x * rad, y0 + s[i].y * rad * F.domeSquash, cz + s[i].z * rad);
            if (dot(n, f.eye - p[0]) < 0 && dot(n, f.eye - p[2]) < 0) continue;
            const double shade = 6 + shadeOf((p[0] + p[2]) * 0.5, n, 0.95);
            RVert q[4];
            for (int i = 0; i < 4; i++) q[i] = vertAt(f, p[i], shade);
            f.shapes.polygon(q, 4, *d.light);
        }
    }
    {   // its ring
        RVert q[10];
        const Vec3 up(0, 1, 0);
        for (int a = 0; a < 10; a++) {
            const double an = (double)a / 10 * TAU;
            const Vec3 p(cx + std::cos(an) * rad * 1.3, floor + 0.15 - below, cz + std::sin(an) * rad * 1.3);
            q[a] = vertAt(f, p, shadeOf(p, up, 0.7));
        }
        if (f.eye.y > floor + 0.15 - below) f.shapes.polygon(q, 10, *d.light);
    }
    RVert beacon = vertAt(f, Vec3(cx, y0 + rad * F.domeSquash + 0.02, cz), 40 + 22 * (0.5 + 0.5 * std::sin(realTime * 3.0)));
    f.shapes.point(beacon, BANK_LIGHT, true, 1);
    // the hatch's leaves, sliding apart under the floor, their seam; the pane over the hole while it is open
    const double slide = open * (R + 0.15);
    for (int s = -1; s <= 1; s += 2) {
        Vec3 p[9];
        for (int k = 0; k < 9; k++) {
            const double th = (s < 0 ? PI / 2 : -PI / 2) + k * PI / 8;
            p[k] = Vec3(cx + R * std::cos(th) + s * slide, floor - 0.003 - 0.06 * open, cz + R * std::sin(th));
        }
        const Vec3 up(0, 1, 0);
        RVert q[9];
        for (int k = 0; k < 9; k++) q[k] = vertAt(f, p[k], lifeShade(f, F.room, p[k], &up, 0.5));
        if (f.eye.y > floor) f.shapes.polygon(q, 9, d.fin[FIN_HULL]);
        if (open > 0) continue;
        RVert a = q[0], b = q[8];
        a.shade *= 0.5; b.shade *= 0.5;
        f.shapes.line(a, b, BANK_HULL, true, 1);
    }
    if (open > 0.001 && f.eye.y > floor) {
        RVert q[16];
        for (int k = 0; k < 16; k++) q[k] = vertAt(f, Vec3(cx + R * std::sin(k * TAU / 16), floor, cz + R * std::cos(k * TAU / 16)), 0);
        f.shapes.polygon(q, 16, *d.pane);
    }
}

// Z-06: the tree of kin's lines over its screen on the port wall: a leaf for each species read and the ship's own three (sixteen at most), joined
// as a tree is (each two neighbours at a height of their own), its root to the left; drawn in the screen's bank, brighter than the screen
void Game::drawTreeWall(CabinFrame& f, const ShipFixture& F, const CabinDraw& d) {
    const BoxAt B(F);
    if (dot(B.front(), f.eye - B.at(0.5, 0, F.height * 0.5)) <= 0) return;
    const CabinDevice& D = cabinDevices().row[FACE_TREE];
    RasterParams lp = d.plain[FIN_GLASS];
    double shade = 58;
    const int bank = f.banks.bankOf(D.bank);
    if (bank >= 0) lp.bank = bank; else shade = 30;
    int read = 0;
    for (const auto& kv : guide.microbes) read += (kv.second.seen & SEEN_KIN) != 0;
    const int n = std::min(16, SHIP_SPECIES + read);
    const double w = B.width(), H = F.height, tw = 0.007 / w, th = 0.007;
    auto bar = [&](double t0, double t1, double y0, double y1) {   // a line on the screen's face: across (t) or up (y), a few millimetres thick
        if (std::fabs(t1 - t0) < 1e-9) { t0 -= tw; t1 += tw; } else { y0 -= th; y1 += th; }
        RVert q[4] = {vertAt(f, B.at(t0, 0, y0), shade), vertAt(f, B.at(t1, 0, y0), shade), vertAt(f, B.at(t1, 0, y1), shade), vertAt(f, B.at(t0, 0, y1), shade)};
        f.shapes.polygon(q, 4, lp);
    };
    // the leaves down the right, each two neighbours' height hashed (a comb: their node the highest between them)
    std::vector<double> h(n, 0.0);
    for (int k = 1; k < n; k++) h[k] = 0.15 + 0.8 * unitFromHash(mix64(0x7EE0ULL + (uint64_t)k * 0x9E3779B97F4A7C15ULL));
    auto yOf = [&](int k) { return H * (0.92 - 0.84 * (n > 1 ? k / (double)(n - 1) : 0.5)); };
    auto tOf = [&](double height) { return 0.1 + 0.78 * height; };   // the leaves at the right as the room sees it, the root to the left
    std::function<std::pair<double, double>(int, int)> draw = [&](int lo, int hi) -> std::pair<double, double> {   // a subtree's (y, height)
        if (lo == hi) return {yOf(lo), 0.0};
        int k = lo + 1;
        for (int j = lo + 2; j <= hi; j++) if (h[j] > h[k]) k = j;
        const auto a = draw(lo, k - 1), b = draw(k, hi);
        bar(tOf(h[k]), tOf(a.second), a.first, a.first); bar(tOf(h[k]), tOf(b.second), b.first, b.first);
        bar(tOf(h[k]), tOf(h[k]), std::min(a.first, b.first), std::max(a.first, b.first));
        return {(a.first + b.first) / 2, h[k]};
    };
    const auto root = draw(0, n - 1);
    bar(tOf(root.second), 0.94, root.first, root.first);   // its stem from the beginning
}
