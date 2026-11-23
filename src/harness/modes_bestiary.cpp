// J-06 (R-448): `bestiary plates [n]`: the animals of n worlds (64) that have them on land, for the user's verdict before J-07. Each world's
// species framed alone on its own ground at 2x (a cell each, its plan and size under it: `plates/plate_NN.png`), a herd of the world's
// at 18 m (`plates/herd_NN.png`), and every world's species small in one sheet (`plates/plates.png`); a line a species printed
#include "harness/harness.h"
#include "surface/surface_view.h"
#include "peoples/works.h"
#include "peoples/plan.h"
#include "galaxy/drainage.h"
#include "core/rng.h"
#include <sys/stat.h>
#include <cmath>
#include <cstdlib>

namespace {
// a dry open place on a world (no water, forest, snow or ice within 12 m of it or of the 20 m in front; a world with no land animals: open sea)
// and a time its sun stands 25 to 55 degrees up
bool plateSite(StarSystem& sys, int bi, bool sea, double& lat, double& lon, double& t) {
    const Body& b = sys.bodies[bi];
    Rng r(mix64(b.seed ^ 0x9A7E5ULL));
    for (int tries = 0; tries < 30; tries++) {
        lat = r.range(-45 * DEG, 45 * DEG); lon = r.range(-PI, PI);
        SurfaceSite site; site.init(&sys, bi, lat, lon, 1000.0);
        bool ok = true;
        double lo = 1e9, hi = -1e9;
        for (int k = 0; k < 35 && ok; k++) {   // the specimens' ground, the cameras' and the herd's: x -16..16, z -24..48
            const TerrainVertex tv = site.sampleAt((k % 5 - 2) * 8.0, (k / 5) * 12.0 - 24, 16);
            const bool wet = tv.material == MAT_WATER || (tv.water > -1e8f && tv.h < tv.water + 0.5);
            ok = sea ? wet && tv.h < tv.water - 3 : !wet && tv.material != MAT_FOREST && tv.material != MAT_SNOW && tv.material != MAT_ICE;
            lo = std::min(lo, (double)tv.h); hi = std::max(hi, (double)tv.h);
        }
        if (!ok || (!sea && hi - lo > 4)) continue;   // open and nearly flat, or open water
        const double day = std::max(600.0, std::fabs(b.rotPeriod));
        for (int k = 0; k < 192; k++) {
            const double tt = 1000 + day * 2 * k / 192;
            const SunInfo si = site.sun(tt);
            if (si.altitude > 25 * DEG && si.altitude < 55 * DEG) { t = tt; return true; }
        }
    }
    return false;
}

// a frame at 2x halved into a cell (each pixel the mean of four)
std::vector<uint32_t> halved(const Framebuffer& fb, int& w, int& h) {
    std::vector<uint32_t> full((size_t)FBW * FBH);
    fb.toRGB(full.data(), 1.0);
    w = FBW / 2; h = FBH / 2;
    std::vector<uint32_t> out((size_t)w * h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int r = 0, g = 0, bl = 0;
            for (int k = 0; k < 4; k++) {
                const uint32_t p = full[(size_t)(2 * y + k / 2) * FBW + 2 * x + (k & 1)];
                r += p & 0xFF; g += (p >> 8) & 0xFF; bl += (p >> 16) & 0xFF;
            }
            out[(size_t)y * w + x] = 0xFF000000u | (uint32_t)(bl / 4) << 16 | (uint32_t)(g / 4) << 8 | (uint32_t)(r / 4);
        }
    return out;
}

std::vector<uint32_t> shrunk(const std::vector<uint32_t>& px, int w, int h, int k) {
    const int W = w / k, H = h / k;
    std::vector<uint32_t> out((size_t)W * H);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int r = 0, g = 0, bl = 0;
            for (int j = 0; j < k; j++)
                for (int i = 0; i < k; i++) { const uint32_t p = px[(size_t)(y * k + j) * w + x * k + i]; r += p & 0xFF; g += (p >> 8) & 0xFF; bl += (p >> 16) & 0xFF; }
            const int n = k * k;
            out[(size_t)y * W + x] = 0xFF000000u | (uint32_t)(bl / n) << 16 | (uint32_t)(g / n) << 8 | (uint32_t)(r / n);
        }
    return out;
}

// how far a species reaches from its middle: behind (its tail), ahead (its neck and head) and up (its back, its neck, its wings)
void extentOf(const Species& sp, double& back, double& ahead, double& up, double& wide) {
    const AnimalBody& a = sp.body;
    const bool flat = planFlies(a.plan) || planSwims(a.plan);
    back = 0.5 * a.length + a.tailLen * (a.tail == TAIL_CURL || a.tail == TAIL_STINGER ? 0.4 : 0.85);
    ahead = 0.5 * a.length + a.neck * std::cos(a.neckUp) + a.headLen * 1.2;
    up = flat ? std::max(a.depth, 0.5 * a.span * 0.6) : a.size + std::max(0.0, a.neck * std::sin(a.neckUp)) + 0.5 * a.ornamentLen;
    wide = a.width + (a.stance == STANCE_SPRAWL ? 1.2 * a.leg : 0) + (a.wings ? a.span : 0);
    if (a.plan == PLAN_FLOATER) { up = a.size + a.leg; ahead = back = 0.6 * a.length; }
}
}

int runBestiaryPlates(int want) {
    mkdir("shots/tests/plates", 0755);
    const int savedScale = FB_SCALE;
    setFramebufferScale(2);
    // the sites sample at 16 m (a river would take a tile a world): the drainage off on every thread, as the scenes' (`DrainageOff` is the
    // calling thread's alone, and the pool's threads fill the rings' misses)
    struct DrainOff { bool was; DrainOff() : was(drainageEnabled()) { setDrainageEnabled(false); } ~DrainOff() { setDrainageEnabled(was); } } drainOff;
    int worlds = 0, shown = 0;
    long pairs = 0, alikePairs = 0;
    std::vector<std::vector<uint32_t>> rows;   // each world's cells small, for the sheet
    int rowW = 0, cellW = 0, cellH = 0;
    for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 160, [&] { return worlds < want; })) {
        for (int bi = 0; bi < (int)sys.bodies.size() && worlds < want; bi++) {
            const Body& b = sys.bodies[bi];
            if (!kindHas(b.type, KT_FAUNA)) continue;
            const bool sea = !kindHas(b.type, KT_FLORA);   // no land animals: its flyers, floaters and swimmers over and in its sea
            double lat = 0, lon = 0, t = 0;
            if (!plateSite(sys, bi, sea, lat, lon, t)) { printf("%s (%s): no open %s in daylight, left out\n", b.name.c_str(), KINDS[b.type].name, sea ? "water" : "dry ground"); continue; }
            {   // a world that had a people: its plan and works built first, so the view takes them on its first frame (they come when ready)
                const BodyGen g = BodyGen::make(b);
                if (worldHadCivilisation(g)) { worldPlanOf(g); worldWorksOf(g); }
            }
            SurfaceView sv; SpaceRenderer sr; StarNeighborhood nb;
            sv.init(&sys, bi, lat, lon, t);
            nb.update(sv.site.worldPos(t, 0, 0, 0));
            sv.relocateCapsule(0, -260);   // out of every view (they look north from south of the specimens)
            sv.player.x = 0; sv.player.z = -45; sv.player.y = sv.site.surfaceHeight(0, -45);   // behind the cameras: the herd's shy keep still
            const Bestiary B = sv.bestiary;
            const int n = (int)B.species.size();
            for (int i = 0; i < n; i++)
                for (int j = i + 1; j < n; j++) { pairs++; if (animalDistance(B.species[i].body, B.species[j].body) < ANIMAL_ALIKE) alikePairs++; }
            // the species one by one: on its ground side on (a flyer and a swimmer held up as specimens), the camera framing it
            std::vector<std::vector<uint32_t>> cells;
            sv.critters.clear(); sv.flocks.clear(); sv.herds.clear();
            Input in;
            {   // the frame's palette set once, then the world's line (its hide banks)
                sv.freeCam = true; sv.freePos = Vec3(0, sv.site.groundHeight(0, -10) + 1.7, -10); sv.freeYaw = 0; sv.freePitch = 0;
                sv.update(1.0 / 30, in, t, false);
                Framebuffer fb0; sv.render(fb0, t, nb.stars, sr, 1.0);
            }
            printf("%2d %s (%s, sector %lld %lld, lat %.1f lon %.1f): %d land, %d in the air, %d in the water; hides in banks %d and %d\n", worlds, b.name.c_str(), KINDS[b.type].name,
                   (long long)x, (long long)z, lat / DEG, lon / DEG, B.landCount, B.flyerCount, B.swimmerCount, sv.testHideBank(0), sv.testHideBank(1));
            for (int i = 0; i < n; i++) {
                const Species& sp = B.species[i];
                double back, ahead, up, wide;
                extentOf(sp, back, ahead, up, wide);
                const int how = planFlies(sp.plan) ? 1 : (planSwims(sp.plan) ? 2 : 0);
                const double wl = sv.site.waterNowAt(0, 0);
                const double gy = sea && wl > -1e8 ? wl : sv.site.groundHeight(0, 0);
                double lift = how ? std::max(1.0, up * 0.8) : 0;
                if (sea && how == 2) lift = -0.25 * sp.body.depth;   // a swimmer in its sea, its back at the surface
                const double turn = how == 1 ? PI / 2 - 0.15 : PI / 2 - 0.45;   // three quarters on (a flyer nearly side on, its wings raised)
                sv.testSpecimen(i, Vec3(0, gy + lift, 0), turn, how, how == 1 ? 0.85 : 0);
                const double tanH = std::tan(0.5 * 70 * DEG), tanV = tanH * UH / UW;
                const double spanX = std::max(back + ahead, sp.body.span) * 1.4, spanY = (up + 0.15 * sp.size) * 1.5;
                const double D = std::max({0.35, 0.5 * spanX / tanH, 0.5 * spanY / tanV, 0.6 * wide, 1.1 * sp.body.span});   // a near wing kept off the lens
                const double midY = gy + lift + (how ? 0.0 : 0.5 * up);
                const double cx = 0.5 * (ahead - back) * std::sin(turn), cz = 0.5 * (ahead - back) * std::cos(turn) - D;
                sv.freeCam = true;
                sv.freePos = Vec3(cx, std::max(midY + 0.25 * D, sv.site.groundHeight(cx, cz) + 0.3), cz);   // a little above, looking down onto it
                sv.freeYaw = 0; sv.freePitch = -std::atan2(sv.freePos.y - midY, D);
                sv.update(1.0 / 30, in, t, false);
                Framebuffer fb;
                sv.render(fb, t, nb.stars, sr, 1.0);
                fb.mush(2);
                if (std::getenv("VESPERIS_PLATE_FULL")) {   // (a look at one cell at 2x)
                    std::vector<uint32_t> full((size_t)FBW * FBH); fb.toRGB(full.data(), 1.0);
                    writePNG(fmt("shots/tests/plates/cell_%02d_%d.png", worlds, i).c_str(), full.data(), FBW, FBH);
                }
                int w, h;
                std::vector<uint32_t> cell = halved(fb, w, h);
                {   // its plan and size under it
                    RGBCanvas cv{cell.data(), w, h, 1};
                    const std::string l1 = fmt("%s%s", PLAN_NAMES[sp.plan], sp.plan == PLAN_SWIMMER ? fmt(" (%s)", SWIM_NAMES[sp.body.variant]).c_str() : "");
                    const std::string l2 = fmt("%.2f M  %s", sp.size, HIDE_NAMES[sp.body.hide]);
                    fillRectRGB(cv, 2, h - 20, 4 + textWidth(l1.c_str()), h - 11, rgb(0, 0, 0)); drawText(cv, 3, h - 19, l1.c_str(), rgb(255, 220, 120));
                    fillRectRGB(cv, 2, h - 10, 4 + textWidth(l2.c_str()), h - 1, rgb(0, 0, 0)); drawText(cv, 3, h - 9, l2.c_str(), rgb(200, 200, 200));
                }
                cellW = w; cellH = h;
                cells.push_back(cell);
                printf("   %-20s %5.2f m  %s\n", PLAN_NAMES[sp.plan], sp.size, animalWords(sp.body).c_str());
                shown++;
            }
            sv.testNoSpecimens();
            {   // the plate: the cells four across
                const int cols = 4, gap = 4, head = 14;
                const int rowsN = (n + cols - 1) / cols;
                Sheet sh(cols * (cellW + gap) + gap, rowsN * (cellH + gap) + gap + head);
                sh.label(4, 3, fmt("%s (%s): %d SPECIES", upper(b.name).c_str(), KINDS[b.type].name, n));
                for (int i = 0; i < n; i++) sh.blit(cells[i], cellW, cellH, gap + (i % cols) * (cellW + gap), head + gap + (i / cols) * (cellH + gap));
                sh.save(fmt("shots/tests/plates/plate_%02d.png", worlds));
            }
            {   // the sheet's row: the cells a quarter
                const int k = 4, w = cellW / k, h = cellH / k;
                std::vector<uint32_t> row((size_t)(w + 2) * 10 * h, 0xFF0E0C0Cu);
                for (int i = 0; i < n && i < 10; i++) {
                    const std::vector<uint32_t> th = shrunk(cells[i], cellW, cellH, k);
                    for (int y = 0; y < h; y++) for (int xx = 0; xx < w; xx++) row[(size_t)y * (w + 2) * 10 + i * (w + 2) + xx] = th[(size_t)y * w + xx];
                }
                rowW = (w + 2) * 10;
                rows.push_back(row);
            }
            if (sea) {   // a sea's world: a school of its first swimmers at the surface, 10 to 20 m off
                const int spi = B.firstSwimmer();
                const Species& sp = B.species[spi];
                const double wl = sv.site.waterNowAt(0, 14);
                Rng r(mix64(b.seed ^ 0x5C001ULL));
                sv.testNoSpecimens();
                for (int k = 0; k < 6; k++)
                    sv.testSpecimenAdd(spi, Vec3(r.sym(8), wl - (0.2 + 0.3 * r.uni()) * sp.body.depth, 10 + r.uni() * 10), PI / 2 + r.sym(0.6), 2, 0);
                sv.freeCam = true; sv.freePos = Vec3(0, wl + 2.0, 0); sv.freeYaw = 0; sv.freePitch = -std::atan2(2.0, 14.0);
                sv.update(1.0 / 30, in, t, false);
                Framebuffer fb;
                sv.render(fb, t, nb.stars, sr, 1.0);
                fb.mush(2);
                std::vector<uint32_t> px((size_t)FBW * FBH);
                fb.toRGB(px.data(), 1.0);
                const std::string fn = fmt("shots/tests/plates/herd_%02d.png", worlds);
                writePNG(fn.c_str(), px.data(), FBW, FBH);
                printf("   school: 6 %s (%s) -> %s\n", PLAN_NAMES[sp.plan], SWIM_NAMES[sp.body.variant], fn.c_str());
                sv.testNoSpecimens();
            } else {   // a herd: the land species with the biggest herds, at 18 m, a few seconds after it was let be (the explorer far off, so none flees)
                int spi = 0;
                for (int i = 1; i < B.landCount; i++) if (B.species[i].herdMax > B.species[spi].herdMax) spi = i;
                const Species& sp = B.species[spi];
                Rng r(mix64(b.seed ^ 0x4E4D18ULL));
                const int count = (sp.herdMin + sp.herdMax) / 2;
                const double scale = std::max(0.3, std::min(1.5, sp.size));
                const double spread = (2 + 1.2 * count) * scale;
                SurfaceView::Herd hd; hd.cx = 0; hd.cz = std::max(7.0, 10 * scale) + spread * 0.4; hd.origin = true;   // nearer a herd of small ones
                sv.herds.assign(1, hd);
                sv.critters.clear();
                for (int i = 0; i < count; i++) {
                    Critter c;
                    c.herd = 0; c.species = spi; c.x = r.sym(spread); c.z = hd.cz + r.sym(spread * 0.6);
                    c.heading = r.range(0, TAU); c.timer = r.range(0.5, 3); c.size = sp.size * (0.85 + 0.3 * r.uni()); c.seed = r.next(); c.state = CS_IDLE;
                    sv.critters.push_back(c);
                }
                const double gy = sv.site.groundHeight(0, 0);
                sv.freeCam = true; sv.freePos = Vec3(0, gy + 1.7, 0); sv.freeYaw = 0; sv.freePitch = -std::atan2(1.7 - 0.5 * sp.size, hd.cz);
                for (int f = 0; f < 90; f++) sv.update(1.0 / 30, in, t + f / 30.0, false);
                Framebuffer fb;
                sv.render(fb, t + 3, nb.stars, sr, 1.0);
                fb.mush(2);
                std::vector<uint32_t> px((size_t)FBW * FBH);
                fb.toRGB(px.data(), 1.0);
                const std::string fn = fmt("shots/tests/plates/herd_%02d.png", worlds);
                writePNG(fn.c_str(), px.data(), FBW, FBH);
                printf("   herd: %d %s (%s) -> %s\n", count, PLAN_NAMES[sp.plan], sv.testHerdStates().c_str(), fn.c_str());
            }
            worlds++;
        }
    }
    {   // the sheet: a row a world
        const int h = cellH / 4, gap = 2;
        Sheet sh(rowW + 40, (int)rows.size() * (h + gap) + gap);
        for (size_t k = 0; k < rows.size(); k++) {
            sh.blit(rows[k], rowW, h, 38, gap + (int)k * (h + gap));
            sh.label(2, gap + (int)k * (h + gap) + h / 2 - 4, fmt("%02d", (int)k));
        }
        sh.save("shots/tests/plates/plates.png");
    }
    setFramebufferScale(savedScale);
    printf("bestiary plates: %d worlds, %d species; %ld pairs of a world, %ld alike (%.2f%%)\n", worlds, shown, pairs, alikePairs, 100.0 * alikePairs / std::max(1L, pairs));
    return worlds == want ? 0 : 1;
}

// J-07: `bestiary ways`: each way played where a world has it (the scan's first world with such a species and a place for it), its moment
// drawn at 2x (`ways/ways_<way>.png`) and what happened printed; then every plan stepping through its walk and its run (`ways/gaits.png`)
// (J-08: shared with `seed fates`: harness.h)
// a place for a way: open dry ground (0), dry ground off the forest (1: the trees are looked for in the view), water 3 m deep or more (2),
// a shore with water 0.3 to 1.5 m deep (3: walked to from the sea); a time its sun stands 25 to 55 degrees up, or 12 under the horizon at night.
// `salt` another place
bool waysSite(StarSystem& sys, int bi, int need, bool night, double& lat, double& lon, double& t, int salt) {
    const Body& b = sys.bodies[bi];
    Rng r(mix64(b.seed ^ 0x3A75ULL ^ (uint64_t)need ^ (uint64_t)salt * 0x9E37ULL));
    for (int tries = 0; tries < 60; tries++) {
        lat = r.range(-45 * DEG, 45 * DEG); lon = r.range(-PI, PI);
        SurfaceSite site; site.init(&sys, bi, lat, lon, 1000.0);
        auto wet = [&](const TerrainVertex& v) { return v.material == MAT_WATER || (v.water > -1e8f && v.h < v.water + 0.5); };
        const TerrainVertex o = site.sampleAt(0, 0, 16);
        bool ok = false;
        if (need == 2) ok = wet(o) && o.h < o.water - 3;
        else if (need == 3) {   // from the sea toward the land: the first dry ground, where the last 60 m of water before it held a shallow
            if (!wet(o)) continue;
            for (double x = 50; x < 12000 && !ok; x += 50) {
                if (wet(site.sampleAt(x, 0, 16))) continue;
                for (double q = x - 60; q < x && !ok; q += 10) {
                    const TerrainVertex v = site.sampleAt(q, 0, 16);
                    ok = v.water > -1e8f && v.water - v.h > 0.3 && v.water - v.h < 1.5;
                }
                if (ok) { site.latLonAt(x + 15, 0, lat, lon); site.init(&sys, bi, lat, lon, 1000.0); }
                break;
            }
        }
        else if (!wet(o) && o.material != MAT_SNOW && o.material != MAT_ICE && o.material != MAT_FOREST) {
            ok = true;
            double lo = 1e9, hi = -1e9;
            for (int k = 0; k < 24; k++) {
                const double a = k * TAU / 24, d = 16 + 16 * (k % 3);
                const TerrainVertex v = site.sampleAt(std::sin(a) * d, std::cos(a) * d, 16);
                if (need == 0 && (wet(v) || v.material == MAT_FOREST)) ok = false;
                lo = std::min(lo, (double)v.h); hi = std::max(hi, (double)v.h);
            }
            if (hi - lo > (need == 1 ? 12 : 6)) ok = false;
        }
        if (!ok) continue;
        const double day = std::max(600.0, std::fabs(b.rotPeriod));
        for (int k = 0; k < 192; k++) {
            const double tt = 1000 + day * 2 * k / 192;
            const SunInfo si = site.sun(tt);
            if (night ? si.altitude < -12 * DEG : (si.altitude > 25 * DEG && si.altitude < 55 * DEG)) { t = tt; return true; }
        }
    }
    return false;
}
// a frame of the view at 2x, mushed, to a file
void waysShot(SurfaceView& sv, StarNeighborhood& nb, SpaceRenderer& sr, double t, const std::string& fn) {
    Framebuffer fb;
    sv.render(fb, t, nb.stars, sr, 1.0);
    fb.mush(2);
    std::vector<uint32_t> px((size_t)FBW * FBH);
    fb.toRGB(px.data(), 1.0);
    writePNG(fn.c_str(), px.data(), FBW, FBH);
}
// a herd of a species put down round a point, standing, its timers short
void waysHerd(SurfaceView& sv, int spi, int count, double cx, double cz, uint64_t seed) {
    const Species& sp = sv.bestiary.species[spi];
    SurfaceView::Herd hd; hd.cx = cx; hd.cz = cz; hd.origin = true;
    sv.herds.push_back(hd);
    Rng r(seed);
    const double spread = (1.5 + 1.0 * count) * std::max(0.4, std::min(1.5, sp.size));
    for (int i = 0; i < count; i++) {
        Critter c;
        c.herd = (int)sv.herds.size() - 1; c.species = spi; c.x = cx + r.sym(spread); c.z = cz + r.sym(spread * 0.6);
        c.heading = r.range(0, TAU); c.timer = r.range(0.5, 3); c.size = sp.size * (0.9 + 0.2 * r.uni()); c.seed = r.next(); c.state = CS_IDLE;
        c.homeX = c.x; c.homeZ = c.z;
        sv.critters.push_back(c);
    }
}


// J-07: the ways' scenes (`creatures_hunt`, `creatures_flock`, `creatures_swimmers`) staged on the site the scene chain found, the camera free and
// the explorer far off (nothing takes fright): false when its world has no such animal or moment (the next world). A hunt: five of its
// quarry 30 m ahead, the hunters 35 m west of them sent stalking, played till two seconds into the chase; a flock of the world's flyers sent
// over; the first swimmer at the surface (or in a leap) within 120 m. `t` moved on to the moment (at most a few minutes: the light the same)
bool setupWaysScene(SurfaceView& sv, int wantMat, double& t) {
    Input in;
    const Bestiary& B = sv.bestiary;
    const double ox = sv.player.x, oz = sv.player.z, gy = sv.site.groundHeight(ox, oz);
    auto camAt = [&](double cx, double cy, double cz, double lx, double ly, double lz) {
        sv.freeCam = true; sv.freePos = Vec3(cx, std::max(cy, sv.site.groundHeight(cx, cz) + 0.4), cz);
        sv.freeYaw = std::atan2(lx - cx, lz - cz);
        sv.freePitch = std::atan2(ly - sv.freePos.y, std::hypot(lx - cx, lz - cz));
    };
    auto step = [&](int frames) { for (int f = 0; f < frames; f++) { t += 1.0 / 30; sv.update(1.0 / 30, in, t, false); } };
    sv.habitatT = 1e18;   // no habitat cell's herds: the stage holds only what is put on it
    if (wantMat == SCENE_SWIMMERS) {
        std::vector<SurfaceView::SwimmerAt> ws;
        for (int k = 0; k < 600; k++) {
            const double tk = t + k * 0.5;
            sv.swimmersNear(ox, oz, tk, ws);
            for (const SurfaceView::SwimmerAt& w : ws) {
                const Species& sp = B.species[w.species];
                if (std::hypot(w.x - ox, w.z - oz) > 120 || (sp.way == WAY_LEAPER ? (w.leap < 0.35 || w.leap > 0.6) : w.surf < 0.9)) continue;
                t = tk;
                const double side = std::max(4.0, std::min(18.0, 5 * sp.size));
                sv.update(1.0 / 30, in, t, false);
                camAt(w.x - w.hz * side, w.water + 2.5, w.z + w.hx * side, w.x, w.water + (sp.way == WAY_LEAPER ? 0.6 * sp.body.length : 0), w.z);
                printf("  a swimmer: %s (%s) %.0f m out, %s\n", B.words(w.species).c_str(), ANIMAL_WAY_NAMES[sp.way], std::hypot(w.x - ox, w.z - oz),
                       w.leap < 1 ? "in a leap" : "at the surface");
                return true;
            }
        }
        return false;
    }
    {   // open ground: no tree within 30 m of the stage (a forest world's animals were lost among its trunks)
        TreeInst T;
        if (sv.testTreeNear(ox, oz + 20, 30, T, true)) return false;
    }
    sv.player.x = ox; sv.player.z = oz - 400; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
    if (wantMat == SCENE_FLOCK) {
        int spi = -1;
        for (int i = B.landCount; i < B.landCount + B.flyerCount && spi < 0; i++) if (B.species[i].way == WAY_FLOCK && B.species[i].activity == ACT_DAY) spi = i;
        if (spi < 0) return false;
        const Species& sp = B.species[spi];
        Flock f;
        f.species = spi; f.seed = mix64(sv.site.gen.seed ^ 0xF10CULL); f.n = std::max(sp.herdMin, std::min(sp.herdMax, 14));
        f.cx = ox; f.cz = oz + 40; f.radius = 30; f.phase = 0; f.speed = 0.05; f.alt = f.cruiseAlt = 30; f.restTimer = 1e9;
        sv.flocks.assign(1, f);
        step(60);
        const Flock& g = sv.flocks[0];
        const double a = g.phase + t * g.speed, fx = g.cx + std::cos(a) * g.radius, fz = g.cz + std::sin(a) * g.radius;
        camAt(ox, gy + 1.7, oz, fx, std::max(sv.site.groundHeight(fx, fz), sv.site.waterAt(fx, fz)) + g.alt, fz);
        printf("  a flock: %d %s, %.0f m up\n", g.n, B.words(spi).c_str(), g.alt);
        return true;
    }
    int hunter = -1;   // a hunt (by a hunter big enough to see at 1x)
    for (int i = 0; i < B.landCount && hunter < 0; i++)
        if (B.species[i].way == WAY_HUNTER && B.species[i].prey >= 0 && B.species[i].activity == ACT_DAY && B.species[i].size >= 0.6) hunter = i;
    if (hunter < 0) return false;
    const Species& sp = B.species[hunter];
    sv.critters.clear(); sv.herds.clear();
    waysHerd(sv, sp.prey, 5, ox, oz + 30, mix64(sv.site.gen.seed ^ 0x9AE7ULL));
    waysHerd(sv, hunter, std::max(1, sp.herdMin), ox - 35, oz + 30, mix64(sv.site.gen.seed ^ 0x4C7ULL));
    for (Critter& c : sv.critters) if (c.species == hunter) { c.state = CS_STALK; c.quarry = sv.critters[2].seed; c.timer = 25; c.hungerT = 0; }
    double chased = 0;
    for (int f = 0; f < 30 * 40 && chased < 1.0; f++) {
        step(1);
        for (const Critter& c : sv.critters) if (c.species == hunter && c.state == CS_CHASE) { chased += 1.0 / 30; break; }
    }
    if (chased < 1.0) return false;
    double hx = 0, hz = 0, qx = 0, qz = 0; int nh = 0, nq = 0;   // the hunters' middle and their quarry's, both in the frame
    for (const Critter& c : sv.critters) {
        if (c.species == hunter) { hx += c.x; hz += c.z; nh++; }
        else if (c.species == sp.prey) { qx += c.x; qz += c.z; nq++; }
    }
    hx /= std::max(1, nh); hz /= std::max(1, nh); qx /= std::max(1, nq); qz /= std::max(1, nq);
    const double gap = std::hypot(qx - hx, qz - hz), mx = 0.5 * (hx + qx), mz = 0.5 * (hz + qz), D = std::max(10.0, 0.7 * gap + 3 + 2 * sp.size);
    const double ux = gap > 1e-6 ? (qx - hx) / gap : 1, uz = gap > 1e-6 ? (qz - hz) / gap : 0;   // the chase's way: the camera off its side
    // the camera off the chase's side (the other side, then round it, then higher) on dry ground with nothing of the ground between
    const double gm = sv.site.groundHeight(mx, mz), ly = gm + 0.4 * sp.size;
    bool seen = false;
    for (int k = 0; k < 24 && !seen; k++) {
        const double a = (k % 8) * PI / 4, lift = 1.2 + 0.5 * sp.size + 2.5 * (k / 8);
        const double sx = uz * std::cos(a) - ux * std::sin(a), sz = -ux * std::cos(a) - uz * std::sin(a);
        const double cx = mx + sx * D, cz = mz + sz * D, cg = sv.site.groundHeight(cx, cz);
        if (sv.site.waterNowAt(cx, cz) > cg) continue;
        const double cy = cg + lift;
        bool clear = true;
        for (int q = 1; q < 12 && clear; q++) {
            const double f = q / 12.0, x = cx + (mx - cx) * f, z = cz + (mz - cz) * f;
            clear = sv.site.groundHeight(x, z) < cy + (ly - cy) * f - 0.3;
        }
        if (clear) { camAt(cx, cy, cz, mx, ly, mz); seen = true; }
    }
    if (!seen) return false;
    printf("  a hunt: %d %s after %s, %.0f m apart\n", nh, B.words(hunter).c_str(), B.words(sp.prey).c_str(), gap);
    return true;
}

int runBestiaryWays() {
    mkdir("shots/tests/ways", 0755);
    const int savedScale = FB_SCALE;
    setFramebufferScale(2);
    struct DrainOff { bool was; DrainOff() : was(drainageEnabled()) { setDrainageEnabled(false); } ~DrainOff() { setDrainageEnabled(was); } } drainOff;
    // the cases: a name, the species wanted, the place it needs (waysSite's), by night
    struct Case { const char* name; std::function<bool(const Species&)> want; int need; bool night; };
    const std::vector<Case> cases = {
        {"grazer", [](const Species& s) { return s.way == WAY_GRAZER && s.social == SOCIAL_HERD && s.herdMax >= 4 && s.activity == ACT_DAY; }, 0, false},
        {"browser", [](const Species& s) { return s.way == WAY_BROWSER && s.activity == ACT_DAY; }, 1, false},
        {"hunter", [](const Species& s) { return s.way == WAY_HUNTER && s.prey >= 0 && s.activity == ACT_DAY; }, 0, false},
        {"scavenger", [](const Species& s) { return s.way == WAY_SCAVENGER; }, 0, false},
        {"burrower", [](const Species& s) { return s.way == WAY_BURROWER && s.activity == ACT_DAY; }, 0, false},
        {"climber", [](const Species& s) { return s.way == WAY_CLIMBER && s.activity == ACT_DAY; }, 1, false},
        {"territorial", [](const Species& s) { return s.social == SOCIAL_TERRITORIAL && s.activity == ACT_DAY; }, 0, false},
        {"night", [](const Species& s) { return s.activity == ACT_NIGHT && !planFlies(s.plan) && !planSwims(s.plan); }, 0, true},
        {"flock", [](const Species& s) { return s.way == WAY_FLOCK && s.plan == PLAN_FLYER && s.activity == ACT_DAY; }, 0, false},
        {"swarm", [](const Species& s) { return s.way == WAY_SWARM && s.activity == ACT_DAY; }, 0, false},
        {"soarer", [](const Species& s) { return s.way == WAY_SOARER && s.activity == ACT_DAY; }, 0, false},
        {"drifter", [](const Species& s) { return s.way == WAY_DRIFTER; }, 2, false},
        {"shoal", [](const Species& s) { return s.way == WAY_SHOAL; }, 2, false},
        {"shallows", [](const Species& s) { return s.way == WAY_SHALLOWS; }, 3, false},
        {"leaper", [](const Species& s) { return s.way == WAY_LEAPER; }, 2, false},
    };
    int drawn = 0;
    Input in;
    for (const Case& cs : cases) {
        bool done = false;
        int looked = 0;
        for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 160, [&] { return !done && looked < 80; })) {
            for (int bi = 0; bi < (int)sys.bodies.size() && !done && looked < 80; bi++) {
                const Body& b = sys.bodies[bi];
                if (!kindHas(b.type, KT_FAUNA)) continue;
                const Bestiary B0 = Bestiary::make(b, BodyGen::make(b));
                int spi = -1;
                for (int i = 0; i < (int)B0.species.size() && spi < 0; i++) if (cs.want(B0.species[i])) spi = i;
                if (spi < 0) continue;
                looked++;
                double lat, lon, t;
                {   // a world that had a people: its plan and works first (they come when ready)
                    const BodyGen g = BodyGen::make(b);
                    if (worldHadCivilisation(g)) { worldPlanOf(g); worldWorksOf(g); }
                }
                SurfaceView sv; SpaceRenderer sr; StarNeighborhood nb;
                bool placed = false;
                for (int salt = 0; salt < (cs.need == 1 ? 12 : 1) && !placed; salt++) {   // trees wanted: the places tried till one has a tree within 30 m
                    if (!waysSite(sys, bi, cs.need, cs.night, lat, lon, t, salt)) break;
                    sv.init(&sys, bi, lat, lon, t);
                    TreeInst T;
                    placed = cs.need != 1 || sv.testTreeNear(0, 0, 30, T);
                }
                if (!placed) continue;
                nb.update(sv.site.worldPos(t, 0, 0, 0));
                sv.relocateCapsule(0, -300);
                sv.critters.clear(); sv.flocks.clear(); sv.herds.clear();
                sv.habitatT = 1e18;   // no habitat cell's herds: the stage holds only what is put on it
                const Species& sp = sv.bestiary.species[spi];
                const std::string who = fmt("%s (%s, %s, %s, %.1f m)", sv.bestiary.words(spi).c_str(), PLAN_NAMES[sp.plan], ANIMAL_WAY_NAMES[sp.way], SOCIAL_NAMES[sp.social], sp.size);
                sv.player.x = 0; sv.player.z = -400; sv.player.y = sv.site.surfaceHeight(0, -400);   // far off: none takes fright
                const double gy0 = sv.site.groundHeight(0, 0);
                auto camAt = [&](double cx, double cy, double cz, double lx, double ly, double lz) {   // the free camera at a place, looking at a point
                    sv.freeCam = true; sv.freePos = Vec3(cx, std::max(cy, sv.site.groundHeight(cx, cz) + 0.4), cz);
                    sv.freeYaw = std::atan2(lx - cx, lz - cz);
                    sv.freePitch = std::atan2(ly - sv.freePos.y, std::hypot(lx - cx, lz - cz));
                };
                double tt = t;
                auto step = [&](int frames) { for (int f = 0; f < frames; f++) { tt += 1.0 / 30; sv.update(1.0 / 30, in, tt, false); } };
                auto count = [&](int state) { int n = 0; for (const Critter& c : sv.critters) n += c.species == spi && c.state == state; return n; };
                auto bySeed = [&](uint64_t seed) -> Critter* { for (Critter& c : sv.critters) if (c.seed == seed) return &c; return nullptr; };
                const double near = std::max(3.5, 9 * sp.size);   // how far to stand from it: nearer a small one
                std::string what;
                const std::string fn = fmt("shots/tests/ways/ways_%s.png", cs.name);
                bool ok = true;
                if (!strcmp(cs.name, "grazer") || !strcmp(cs.name, "night")) {   // a herd let be for eight seconds (at night the lamp on)
                    double ox = 0;   // the camera where no trunk stands within 6 m of it or of the line to the herd
                    for (int k = 0; k < 12; k++) {
                        TreeInst T;
                        bool clear = true;
                        for (int q = 0; q <= 4 && clear; q++) clear = !sv.testTreeNear(ox, (near + 4 * sp.size) * q / 4.0, 6, T, true);
                        if (clear) break;
                        ox = (k % 2 ? -1 : 1) * 6.0 * (k / 2 + 1);
                    }
                    waysHerd(sv, spi, std::max(3, (sp.herdMin + sp.herdMax) / 2), ox, near + 4 * sp.size, mix64(b.seed ^ 0x6A2EULL));
                    if (cs.night) sv.lamp = true;
                    step(240);
                    camAt(ox, gy0 + 0.5 + sp.size, 0, ox, gy0 + 0.5 * sp.size, near + 4 * sp.size);
                    what = sv.testHerdStates();
                } else if (!strcmp(cs.name, "browser")) {   // one sent to the nearest tree, its neck up into it
                    TreeInst T;
                    if (!sv.testTreeNear(0, 0, 45, T)) ok = false;
                    else {
                        const double ux = -T.x / std::max(1.0, std::hypot(T.x, T.z)), uz = -T.z / std::max(1.0, std::hypot(T.x, T.z));   // its open side, toward the place
                        const double stand = 0.12 + T.h * 0.02 + 0.4 * sp.body.length + 0.5 * sp.body.neck;
                        waysHerd(sv, spi, 1, T.x + ux * (stand + 4), T.z + uz * (stand + 4), mix64(b.seed ^ 0xB2015EULL));
                        Critter* c = &sv.critters.back();
                        c->state = CS_BROWSE; c->timer = 20; c->treeX = T.x; c->treeZ = T.z; c->treeR = 0.12 + T.h * 0.02;
                        const uint64_t cs0 = c->seed;
                        step(240);
                        c = bySeed(cs0);
                        const double sx = uz, sz = -ux;   // across its way in, the tree and the browser side on
                        const double D = 6 + 3 * sp.size;
                        camAt(T.x + ux * stand * 0.5 + sx * D, T.gy + 1.2 + 0.5 * sp.size, T.z + uz * stand * 0.5 + sz * D, T.x + ux * stand * 0.5, T.gy + 0.9 * sp.size, T.z + uz * stand * 0.5);
                        what = c ? fmt("reach %.2f, %.1f m from the trunk (%s)", c->reach, std::hypot(c->x - T.x, c->z - T.z), DOING_NAMES[c->state]) : "lost";
                        ok = c && c->reach > 0.5;
                    }
                } else if (!strcmp(cs.name, "hunter")) {   // a pack (or one) 35 m from a herd of its quarry, sent stalking; the frame in the chase
                    const int prey = sp.prey;
                    waysHerd(sv, prey, 5, 0, 30, mix64(b.seed ^ 0x9AE7ULL));
                    waysHerd(sv, spi, std::max(1, sp.herdMin), -35, 30, mix64(b.seed ^ 0x4C7ULL));
                    const Critter* q = &sv.critters[2];
                    for (Critter& c : sv.critters) if (c.species == spi) { c.state = CS_STALK; c.quarry = q->seed; c.timer = 25; c.hungerT = 0; }
                    double stalked = 0, chased = 0;
                    for (int f = 0; f < 30 * 40 && chased < 2.0; f++) {
                        step(1);
                        if (count(CS_STALK)) stalked += 1.0 / 30;
                        if (count(CS_CHASE)) chased += 1.0 / 30;
                    }
                    double hx = 0, hz = 0, px = 0, pz = 0; int nh = 0, np = 0;   // the hunters' middle and their quarry's, both in the frame
                    for (const Critter& c : sv.critters) {
                        if (c.species == spi) { hx += c.x; hz += c.z; nh++; }
                        else if (c.species == prey) { px += c.x; pz += c.z; np++; }
                    }
                    hx /= std::max(1, nh); hz /= std::max(1, nh); px /= std::max(1, np); pz /= std::max(1, np);
                    const double mx = 0.5 * (hx + px), mz = 0.5 * (hz + pz), D = std::max(12.0, 0.9 * std::hypot(px - hx, pz - hz) + 6 + 4 * sp.size);
                    const double gm = sv.site.groundHeight(mx, mz);
                    camAt(mx, gm + 1.5 + 0.5 * sp.size, mz - D, mx, gm + 0.4 * sp.size, mz);
                    int fled = 0; for (const Critter& c : sv.critters) fled += c.species == prey && c.state == CS_FLEE;
                    what = fmt("stalked %.1f s, chased %.1f s; of its quarry's %s, %d of 5 fleeing", stalked, chased, sv.bestiary.words(prey).c_str(), fled);
                    ok = chased > 0;
                } else if (!strcmp(cs.name, "scavenger")) {   // trailing a hunter of its world, or any other's walking
                    int hunter = -1;
                    for (int i = 0; i < sv.bestiary.landCount; i++) if (sv.bestiary.species[i].way == WAY_HUNTER) hunter = i;
                    if (hunter < 0) ok = false;
                    else {
                        waysHerd(sv, hunter, 1, 0, 30, mix64(b.seed ^ 0x5CA7ULL));
                        Critter* h0 = &sv.critters.back();
                        h0->hungerT = 1e9; h0->state = CS_WALK; h0->speed = sv.bestiary.species[hunter].walkSpeed; h0->heading = PI / 2; h0->timer = 1e9;
                        const uint64_t hs = h0->seed;
                        waysHerd(sv, spi, 1, -45, 30, mix64(b.seed ^ 0x5CA8ULL));
                        Critter* c0 = &sv.critters.back();
                        c0->state = CS_TRAIL; c0->quarry = hs; c0->timer = 1e9;
                        const uint64_t ss = c0->seed;
                        step(300);
                        const Critter* h = bySeed(hs);
                        const Critter* c = bySeed(ss);
                        if (!h || !c) ok = false;
                        else {
                            const double mx = 0.5 * (h->x + c->x), mz = 0.5 * (h->z + c->z), D = std::max(14.0, 0.9 * std::hypot(h->x - c->x, h->z - c->z) + 6);
                            camAt(mx, sv.site.groundHeight(mx, mz) + 2.0, mz - D, mx, sv.site.groundHeight(mx, mz) + 0.5, mz);
                            what = fmt("%.0f m behind %s (%s)", std::hypot(h->x - c->x, h->z - c->z), sv.bestiary.words(hunter).c_str(), DOING_NAMES[c->state]);
                        }
                    }
                } else if (!strcmp(cs.name, "burrower") || !strcmp(cs.name, "climber") || !strcmp(cs.name, "territorial")) {   // the explorer walks up
                    const bool climber = !strcmp(cs.name, "climber");
                    double cx = 0, cz = 18;
                    if (climber) { TreeInst T; if (sv.testTreeNear(0, 0, 45, T)) { cx = T.x + 5; cz = T.z + 3; } else ok = false; }
                    waysHerd(sv, spi, std::max(2, std::min(4, sp.herdMax)), cx, cz, mix64(b.seed ^ 0xB0B0ULL));
                    sv.player.x = cx; sv.player.z = cz - 40; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z);
                    bool seen = false;
                    for (int f = 0; f < 30 * 30 && !seen && ok; f++) {
                        const double gap = std::hypot(sv.player.x - cx, sv.player.z - cz);
                        if (gap > 9) { sv.player.z += 1.4 / 30; sv.player.y = sv.site.surfaceHeight(sv.player.x, sv.player.z); }
                        step(1);
                        for (const Critter& c : sv.critters) {
                            if (!strcmp(cs.name, "burrower") && c.state == CS_UNDER && c.sink > 0.45 && c.sink < 0.7) seen = true;
                            if (climber && c.state == CS_CLIMB && c.climbY > 0.6 * c.treeTop) seen = true;
                            if (!strcmp(cs.name, "territorial") && c.state == CS_CHARGE && c.timer < 1.8) seen = true;
                        }
                    }
                    double ax = cx, az = cz, ay = sv.site.groundHeight(cx, cz) + 0.4 * sp.size;   // the one the moment is of (a climber's up its trunk)
                    for (const Critter& c : sv.critters)
                        if ((c.state == CS_UNDER && c.sink > 0.3) || (c.state == CS_CLIMB && c.climbY > 0) || c.state == CS_CHARGE) {
                            ax = c.x; az = c.z; ay = (c.state == CS_CLIMB ? c.treeGy + c.climbY : sv.site.groundHeight(c.x, c.z) + 0.4 * sp.size);
                            break;
                        }
                    const double D = climber ? 5 + 2 * sp.size : near;
                    camAt(ax + 0.7 * D, ay + 0.6 + 0.3 * sp.size, az - 0.7 * D, ax, ay, az);
                    what = fmt("the explorer %.0f m off: %s", std::hypot(sv.player.x - cx, sv.player.z - cz), sv.testHerdStates().c_str());
                    ok = ok && seen;
                } else if (planFlies(sp.plan)) {   // a flock put over the place: low (a swarm), high (a soarer) or as it flies
                    Flock f;
                    f.species = spi; f.seed = mix64(b.seed ^ 0xF10CULL); f.n = std::max(sp.herdMin, std::min(sp.herdMax, 12));
                    f.cx = 0; f.cz = 40; f.radius = 30; f.phase = 0; f.speed = 0.05; f.alt = f.cruiseAlt = 35; f.restTimer = 1e9;
                    if (sp.way == WAY_SWARM) { f.cz = 9; f.radius = 3; f.alt = f.cruiseAlt = 2.0; f.n = 30; }
                    if (sp.way == WAY_SOARER) { f.cz = 60; f.radius = 50; f.alt = f.cruiseAlt = 70; }
                    if (sp.plan == PLAN_FLOATER) { f.cz = 30; f.radius = 6; f.alt = f.cruiseAlt = 14; f.speed = 0.02; }
                    sv.flocks.push_back(f);
                    step(60);
                    const Flock& g = sv.flocks[0];
                    const double a = g.phase + tt * g.speed, fx = g.cx + std::cos(a) * g.radius, fz = g.cz + std::sin(a) * g.radius;
                    const double fy = std::max(sv.site.groundHeight(fx, fz), sv.site.waterAt(fx, fz)) + g.alt;
                    camAt(fx * 0.3, std::max(gy0, sv.site.waterAt(0, 0)) + 1.7, fz - (sp.way == WAY_SWARM ? 8 : 35), fx, fy, fz);
                    what = fmt("%d over %.0f m up, %.0f m off%s", g.n, g.alt, std::hypot(fx, fz), sp.plan == PLAN_FLOATER ? fmt(", the wind %.0f kt", sv.env.windKnots).c_str() : "");
                } else {   // a swimmer of the way: the first of its spots in reach found at the surface (a leaper in its leap), seen from 18 m
                    std::vector<SurfaceView::SwimmerAt> ws;
                    bool found = false;
                    for (int k = 0; k < 2400 && !found; k++) {
                        const double tk = t + k * 0.5;
                        sv.swimmersNear(0, 0, tk, ws);
                        for (const SurfaceView::SwimmerAt& w : ws) {
                            if (w.species != spi || std::hypot(w.x, w.z) > 140) continue;
                            if (sp.way == WAY_LEAPER ? (w.leap < 0.35 || w.leap > 0.6) : w.surf < 0.9) continue;
                            tt = tk; found = true;
                            const double side = std::max(4.0, std::min(18.0, 5 * sp.size));   // to its side, nearer a small one
                            const double ox = -w.hz * side, oz = w.hx * side;
                            sv.update(1.0 / 30, in, tt, false);
                            camAt(w.x + ox, w.water + 2.5, w.z + oz, w.x, w.water + (sp.way == WAY_LEAPER ? 0.6 * sp.body.length : 0), w.z);
                            what = fmt("at %.0f m, %s, the water %.1f m deep", std::hypot(w.x, w.z), w.leap < 1 ? fmt("leap %.2f", w.leap).c_str() : fmt("surf %.2f", w.surf).c_str(),
                                       w.water - sv.site.groundHeight(w.x, w.z));
                            break;
                        }
                    }
                    ok = found;
                }
                if (!ok) { printf("  %s: %s on %s: no moment (%s), the next world\n", cs.name, who.c_str(), b.name.c_str(), what.c_str()); continue; }
                sv.update(1.0 / 30, in, tt, false);
                waysShot(sv, nb, sr, tt, fn);
                printf("%-11s %s on %s (%s): %s -> %s\n", cs.name, who.c_str(), b.name.c_str(), KINDS[b.type].name, what.c_str(), fn.c_str());
                done = true; drawn++;
            }
        }
        if (!done) printf("%-11s NONE FOUND\n", cs.name);
    }
    // the gaits: a species of each plan that walks, its walk and its run at three moments each, side on
    {
        const int cols = 6;
        std::vector<std::vector<uint32_t>> cells; std::vector<std::string> labels;
        int cw = 0, ch = 0;
        bool have[PLAN_COUNT] = {};
        for (auto& [x, z, s, sys] : scanSystems(150, 320, 20, 160, [&] { int n = 0; for (bool h : have) n += h; return n < PLAN_COUNT - 4; })) {
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (!kindHas(b.type, KT_FAUNA) || !kindHas(b.type, KT_FLORA)) continue;
                const Bestiary B0 = Bestiary::make(b, BodyGen::make(b));
                int spi = -1;
                for (int i = 0; i < B0.landCount && spi < 0; i++) if (!have[B0.species[i].plan]) spi = i;
                if (spi < 0) continue;
                double lat, lon, t;
                if (!waysSite(sys, bi, 0, false, lat, lon, t)) continue;
                SurfaceView sv; SpaceRenderer sr; StarNeighborhood nb;
                sv.init(&sys, bi, lat, lon, t);
                nb.update(sv.site.worldPos(t, 0, 0, 0));
                sv.relocateCapsule(0, -300);
                sv.critters.clear(); sv.flocks.clear(); sv.herds.clear();
                sv.habitatT = 1e18;
                sv.player.x = 0; sv.player.z = -400;
                {   // a first frame, so the ground's heights are the drawn rings' (the cameras are kept over them)
                    sv.freeCam = true; sv.freePos = Vec3(0, sv.site.groundHeight(0, -5) + 1.7, -5); sv.freeYaw = 0; sv.freePitch = 0;
                    sv.update(1.0 / 30, in, t, false);
                    Framebuffer fb0; sv.render(fb0, t, nb.stars, sr, 1.0);
                }
                for (int i = 0; i < sv.bestiary.landCount; i++) {
                    const Species& sp = sv.bestiary.species[i];
                    if (have[sp.plan]) continue;
                    have[sp.plan] = true;
                    double back, ahead, up, wide;
                    extentOf(sp, back, ahead, up, wide);
                    const double g = clampd(sv.site.gravity, 2.0, 25.0);
                    for (int k = 0; k < cols; k++) {
                        const bool run = k >= cols / 2;
                        const double speed = run ? sp.runSpeed : sp.walkSpeed;
                        const int gait = speciesGait(sp, 1, speed, g, run);
                        const double stride = speciesStride(sp, 1, speed, g), phase = (k % (cols / 2)) / 3.0;
                        sv.testSpecimen(i, Vec3(0, sv.site.groundHeight(0, 0), 0), PI / 2, 0, 0);
                        sv.testSpecimenStep(gait, phase, stride, run);
                        const double span = std::max(back + ahead, 0.6 * up) * 1.5, D = std::max(0.6, 0.5 * span / std::tan(35 * DEG));
                        const double midY = sv.site.groundHeight(0, 0) + 0.45 * up;
                        const double rise = sp.body.pairs ? 0.1 * D : 0.9 * D;   // a legless one's waves seen from above
                        const double cx = 0.5 * (ahead - back), cz = sp.body.pairs ? -D : -0.6 * D;
                        sv.freeCam = true; sv.freePos = Vec3(cx, std::max(midY + rise, sv.site.groundHeight(cx, cz) + 0.15), cz);
                        sv.freeYaw = 0; sv.freePitch = -std::atan2(sv.freePos.y - midY, -cz);
                        sv.update(1.0 / 30, in, t, false);
                        Framebuffer fb;
                        sv.render(fb, t, nb.stars, sr, 1.0);
                        fb.mush(2);
                        int w, h;
                        std::vector<uint32_t> cell = halved(fb, w, h);
                        cell = shrunk(cell, w, h, 2);
                        cw = w / 2; ch = h / 2;
                        cells.push_back(cell);
                        labels.push_back(k == 0 ? fmt("%s: %s %.1f M/S", PLAN_NAMES[sp.plan], GAIT_NAMES[gait], speed) : (k == cols / 2 ? fmt("%s %.1f M/S", GAIT_NAMES[gait], speed) : ""));
                    }
                    sv.testNoSpecimens();
                    printf("gaits      %-20s walk %s at %.1f m/s (%.2f m a stride), run %s at %.1f m/s (%.2f m)\n", PLAN_NAMES[sp.plan], GAIT_NAMES[speciesGait(sp, 1, sp.walkSpeed, g, false)], sp.walkSpeed,
                           speciesStride(sp, 1, sp.walkSpeed, g), GAIT_NAMES[speciesGait(sp, 1, sp.runSpeed, g, true)], sp.runSpeed, speciesStride(sp, 1, sp.runSpeed, g));
                }
            }
        }
        const int n = (int)cells.size(), rowsN = (n + cols - 1) / cols, gap = 3;
        Sheet sh(cols * (cw + gap) + gap, rowsN * (ch + gap) + gap);
        for (int i = 0; i < n; i++) {
            sh.blit(cells[i], cw, ch, gap + (i % cols) * (cw + gap), gap + (i / cols) * (ch + gap));
            if (!labels[i].empty()) sh.label(gap + (i % cols) * (cw + gap) + 1, gap + (i / cols) * (ch + gap) + 1, labels[i]);
        }
        sh.save("shots/tests/ways/gaits.png");
    }
    setFramebufferScale(savedScale);
    printf("bestiary ways: %d of %d ways drawn\n", drawn, (int)cases.size());
    return drawn == (int)cases.size() ? 0 : 1;
}
