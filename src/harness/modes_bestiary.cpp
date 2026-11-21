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
