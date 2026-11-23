// Z-02: what it looks like: `life plates [n]` (a plate a world with life nearest home, and one a stage and a chemistry), `life strips [plan]`
// (each act in eight frames over its time), `life distinct [species]` (no two of a world alike; pictures of a thousand compared), `life forms
// [plan]` (every plan's variants in their ways of seeing) (life/morphs.h, life/morph_draw.h)
#include "harness/harness.h"
#include "life/morphs.h"
#include "life/morph_draw.h"
#include <map>
#include <set>

namespace {
struct Panel { std::vector<uint32_t> px; int w = 0, h = 0; };

std::string lowerName(const char* s) { std::string r; for (const char* c = s; *c; c++) r += *c == ' ' ? '_' : (char)std::tolower((unsigned char)*c); return r; }

// a nice length for a scale bar about a fifth of the view
double barUm(double viewUm) {
    static const double STEPS[] = {0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000};
    double best = STEPS[0];
    for (double s : STEPS) if (s <= viewUm * 0.22) best = s;
    return best;
}
std::string umText(double um) { return um < 1 ? fmt("%.1f UM", um) : (um >= 1000 ? fmt("%.0f MM", um / 1000) : fmt("%.0f UM", um)); }

// a panel's specimen: its species in a form doing an act. B-503 (KI-438): a spore, a cyst or a fossil lying on the slide is held still, as
// the microscope holds it (`Specimen::still`: its jiggle alone; a dormant species' spores do not swim)
Specimen panelSpecimen(const Morph& m, int form, int act, uint64_t seed) {
    Specimen s; s.m = &m; s.form = form; s.act = act; s.seed = seed; s.angle = (unitFromHash(mix64(seed >> 3)) - 0.5) * 1.2; s.t0 = 0;
    if (act == AN_TO_FIELD) s.angle = 0.3;
    s.still = act == AN_LIVE && (form == CF_SPORE || form == CF_CYST || form == CF_FOSSIL);
    return s;
}

// a species on a slide in a way of seeing at a time: the specimen and, where it is small in the view, a few more of its kind; its scale bar
// and its line; the mush as the scenes' (two passes). The view follows the specimen (`track`) or looks at a fixed place (`fixX`, `fixY`)
Panel slidePanel(const Morph& m, int seeing, int form, int act, double t, double mag, uint64_t seed, const std::string& line, const Morph* other = nullptr,
                 bool more = true, bool track = true, double fixX = 0, double fixY = 0) {
    Framebuffer fb;
    slidePalette(fb.pal, seeing);
    Slide v; v.seeing = seeing; v.umPerPx = umPerPxAt(mag); v.t = t; v.seed = seed;
    v.turn = 0.4 + unitFromHash(seed) * 1.2; v.tilt = 0.5; v.stage = 0.35 + unitFromHash(mix64(seed >> 7)) * 0.6;
    std::vector<Specimen> cells;
    Specimen s = panelSpecimen(m, form, act, seed);
    s.other = other;
    cells.push_back(s);
    double ang;
    if (track) specimenPlace(s, t, v.cx, v.cy, ang);   // the view follows it
    else { v.cx = fixX; v.cy = fixY; }
    const double viewW = v.umPerPx * UW, viewH = v.umPerPx * UH;
    const double e = m.extentUm();
    if (more && act == AN_LIVE && seeing != WS_ELECTRON && e < viewH * 0.22) {
        const int n = 2 + (int)(unitFromHash(mix64(seed >> 11)) * 4);
        for (int k = 0; k < n; k++) {
            Specimen o = s; o.seed = mix64(seed + (uint64_t)k + 1);
            o.x = v.cx + (unitFromHash(o.seed) - 0.5) * (viewW - 2 * e) * 0.9; o.y = v.cy + (unitFromHash(mix64(o.seed >> 13)) - 0.5) * (viewH - 2 * e) * 0.9;
            if (std::fabs(o.x - v.cx) < e * 2.2 && std::fabs(o.y - v.cy) < e * 2.2) o.x += (o.x < v.cx ? -1 : 1) * e * 3;
            o.angle = unitFromHash(mix64(o.seed >> 27)) * TAU;
            cells.push_back(o);
        }
    }
    if (act == AN_TO_LIGHT) { v.spotX = s.x; v.spotY = s.y; v.spotR = std::max(m.extentUm(), m.widthUm) * 2.5; }
    drawSlide(fb, v, cells);
    fb.mush(2);
    Panel P; P.w = FBW; P.h = FBH; P.px.resize((size_t)FBW * FBH);
    fb.toRGB(P.px.data(), 1.0);
    RGBCanvas cv{P.px.data(), P.w, P.h, FB_SCALE};
    const uint32_t ink = rgb(255, 230, 150);
    const double bar = barUm(viewW);
    const int bx = 8 * FB_SCALE, by = P.h - 10 * FB_SCALE, bw = (int)(bar / viewW * P.w);
    fillRectRGB(cv, bx - FB_SCALE, by - FB_SCALE, bx + bw + FB_SCALE, by + 2 * FB_SCALE, rgb(0, 0, 0));
    fillRectRGB(cv, bx, by, bx + bw, by + FB_SCALE, rgb(255, 255, 255));
    drawTextShadow(cv, 8, UH - 22, umText(bar).c_str(), ink, rgb(0, 0, 0), FB_SCALE);
    drawTextShadow(cv, 6, 5, line.c_str(), ink, rgb(0, 0, 0), FB_SCALE);
    drawTextShadow(cv, UW - 6 - textWidth(fmt("%s X%.0f", SEEING_NAMES[seeing], mag).c_str()), 5, fmt("%s X%.0f", SEEING_NAMES[seeing], mag).c_str(), ink, rgb(0, 0, 0),
                   FB_SCALE);
    return P;
}

// the magnification step that holds a box (um) with a margin
double magnificationForBox(double w, double h) {
    static const double STEPS[] = {4000, 3000, 2000, 1500, 1000, 600, 400, 300, 200, 150, 100, 60, 40, 30, 20, 10};
    for (double st : STEPS) if (w * 1.15 <= umPerPxAt(st) * UW && h * 1.15 <= umPerPxAt(st) * UH) return st;
    return 10;
}

// a panel's magnification: a living cell's by its body (magnificationFor), a spore's or a cyst's by the box it covers
double panelMag(const Morph& m, int way, int form) {
    if (form != CF_SPORE && form != CF_CYST) return magnificationFor(m, way);
    Specimen s; s.m = &m; s.form = form; s.seed = m.seed;
    double x0, y0, x1, y1;
    specimenBox(s, 0, x0, y0, x1, y1);
    const double w = std::max(x1 - x0, 0.2), h = std::max(y1 - y0, 0.2);
    return magnificationForBox(w * 2.2, h * 2.2);
}


void saveGrid(const std::vector<Panel>& panels, int cols, const std::string& title, const std::string& fn) {
    if (panels.empty()) return;
    const int pw = panels[0].w, ph = panels[0].h, gap = 4, head = 14 * FB_SCALE;
    const int rows = ((int)panels.size() + cols - 1) / cols;
    Sheet sh(cols * (pw + gap) + gap, rows * (ph + gap) + gap + head);
    RGBCanvas cv{sh.px.data(), sh.w, sh.h, FB_SCALE};
    drawText(cv, 4, 3, title.c_str(), rgb(255, 220, 120), FB_SCALE);
    for (size_t i = 0; i < panels.size(); i++) sh.blit(panels[i].px, pw, ph, gap + (int)(i % cols) * (pw + gap), head + gap + (int)(i / cols) * (ph + gap));
    sh.save(fn);
}

std::string speciesLine(const Microbe& s, const Morph& m) {
    std::string l = fmt("M%d %s", s.index, MICROBE_PLAN_NAMES[m.plan]);
    if (s.state != SS_LIVING) l += std::string(" ") + STATE_NAMES[s.state];
    return l;
}

// the worlds with species nearest home (the ship's own system first), each its system and body
struct WorldRef { StarSystem sys; int body = 0; };
std::vector<WorldRef> livingWorlds(int n, int radius = 14) {
    std::vector<WorldRef> out;
    for (const Star& st : starsNearHome(radius)) {
        if ((int)out.size() >= n) break;
        WorldRef w; w.sys.generate(st);
        auto L = lifeOf(w.sys);
        for (int i = 0; i < (int)w.sys.bodies.size() && (int)out.size() < n; i++) {
            const Biota& B = L->bodies[i];
            if (B.species.empty()) continue;
            WorldRef r; r.sys = w.sys; r.body = i;
            out.push_back(std::move(r));
        }
    }
    return out;
}

// `life plates [n]`
int lifePlates(int n) {
    const double t0 = nowSec();
    const std::vector<WorldRef> worlds = livingWorlds(n);
    int k = 0;
    // B-503 (KI-438): each spore, cyst and fossil a plate draws, and those held still there (its place at the panel's time within its jiggle of
    // where it lies: a Brownian jiggle's reach, `poseAt`); one that moved fails the mode
    int lying = 0, held = 0;
    auto tally = [&](const Morph& m, int form, double t, uint64_t seed) {
        if (form != CF_SPORE && form != CF_CYST && form != CF_FOSSIL) return;
        const Specimen s = panelSpecimen(m, form, AN_LIVE, seed);
        double x, y, a;
        specimenPlace(s, t, x, y, a);
        const double body = std::max(m.lengthUm, m.widthUm), jiggle = 0.06 + 0.25 / std::sqrt(std::max(0.2, body));
        lying++;
        held += std::hypot(x - s.x, y - s.y) <= jiggle * 1.5 ? 1 : 0;
    };
    for (const WorldRef& w : worlds) {
        auto L = lifeOf(w.sys);
        const Biota& B = L->bodies[w.body];
        const std::vector<Morph> ms = morphsOf(w.sys, w.body);
        std::vector<Panel> panels;
        for (size_t i = 0; i < B.species.size(); i++) {
            const Morph& m = ms[i];
            const int form = B.species[i].state == SS_DORMANT ? CF_SPORE : (B.species[i].state == SS_FOSSIL ? CF_FOSSIL : CF_CELL);
            const std::vector<int> ways = seeingsFor(m);
            for (size_t j = 0; j < ways.size() && j < 2; j++) {
                const double mag = panelMag(m, ways[j], form);
                panels.push_back(slidePanel(m, ways[j], form, AN_LIVE, 1.3 + i * 0.7, mag, m.seed ^ (uint64_t)(j + 1), speciesLine(B.species[i], m)));
                tally(m, form, 1.3 + i * 0.7, m.seed ^ (uint64_t)(j + 1));
            }
        }
        k++;
        saveGrid(panels, 4, fmt("%s  %s%s  %s", w.sys.bodyLabel(w.body).c_str(), LIFE_STAGE_NAMES[B.stage], B.second != LS_STERILE ? fmt(" + %s", LIFE_STAGE_NAMES[B.second]).c_str() : "",
                                CHEMISTRY_NAMES[B.chemistry]),
                 fmt("shots/tests/life_plate_%d.png", k));
    }
    // a plate a stage and a chemistry: species of it from the worlds near home, the plans unseen first
    struct Want { std::string name; std::function<bool(const Biota&, const Microbe&, const Morph&)> fits; };
    const std::vector<Want> wants = {
        {"prebiotic", [](const Biota&, const Microbe& s, const Morph&) { return s.state == SS_PROTO; }},
        {"simple", [](const Biota&, const Microbe& s, const Morph& m) { return s.state == SS_LIVING && smallCellPlan(m.plan); }},
        {"complex", [](const Biota&, const Microbe& s, const Morph& m) { return s.state == SS_LIVING && largeCellPlan(m.plan); }},
        {"colonial", [](const Biota&, const Microbe& s, const Morph& m) { return s.state == SS_LIVING && colonyPlan(m.plan); }},
        {"dormant", [](const Biota&, const Microbe& s, const Morph&) { return s.state == SS_DORMANT; }},
        {"fossil", [](const Biota&, const Microbe& s, const Morph&) { return s.state == SS_FOSSIL; }},
        {"water", [](const Biota& B, const Microbe& s, const Morph&) { return B.chemistry == CH_WATER && s.state == SS_LIVING; }},
        {"acid", [](const Biota& B, const Microbe&, const Morph&) { return B.chemistry == CH_ACID; }},
        {"ammonia", [](const Biota& B, const Microbe&, const Morph&) { return B.chemistry == CH_AMMONIA; }},
        {"hydrocarbon", [](const Biota& B, const Microbe&, const Morph&) { return B.chemistry == CH_HYDROCARBON; }}};
    struct Found { Morph m; Microbe s; std::string where; };
    std::vector<std::vector<Found>> cand(wants.size());
    for (const Star& st : starsNearHome(40)) {
        bool full = true;
        for (const auto& f : cand) if (f.size() < 160) full = false;
        if (full) break;
        StarSystem sys; sys.generate(st);
        auto L = lifeOf(sys);
        for (int b = 0; b < (int)sys.bodies.size(); b++) {
            const Biota& B = L->bodies[b];
            if (B.species.empty()) continue;
            std::vector<Morph> ms;
            for (size_t i = 0; i < B.species.size(); i++)
                for (size_t q = 0; q < wants.size(); q++) {
                    if (cand[q].size() >= 160) continue;
                    if (ms.empty()) ms = morphsOf(sys, b);
                    if (wants[q].fits(B, B.species[i], ms[i])) cand[q].push_back({ms[i], B.species[i], sys.bodyLabel(b)});
                }
        }
    }
    std::vector<std::vector<Found>> found(wants.size());
    for (size_t q = 0; q < wants.size(); q++) {   // each plan (and its variant) once, then each plan once more, in the order found
        std::set<std::pair<int, int>> forms; std::map<int, int> plans;
        for (int pass = 0; pass < 3 && found[q].size() < 16; pass++)
            for (const Found& f : cand[q]) {
                if (found[q].size() >= 16) break;
                const std::pair<int, int> key{f.m.plan, f.m.variant};
                if (pass == 0 && (forms.count(key) || plans[f.m.plan] >= 1)) continue;
                if (pass == 1 && (forms.count(key) || plans[f.m.plan] >= 2)) continue;
                if (pass == 2 && forms.count(key) && plans[f.m.plan] >= 3) continue;
                bool dup = false;
                for (const Found& g : found[q]) if (g.m.seed == f.m.seed) dup = true;
                if (dup) continue;
                forms.insert(key); plans[f.m.plan]++;
                found[q].push_back(f);
            }
    }
    for (size_t q = 0; q < wants.size(); q++) {
        std::vector<Panel> panels;
        for (size_t i = 0; i < found[q].size() && i < 16; i++) {
            const Found& f = found[q][i];
            const int form = f.s.state == SS_DORMANT ? CF_SPORE : (f.s.state == SS_FOSSIL ? CF_FOSSIL : CF_CELL);
            const int way = seeingsFor(f.m)[0];
            panels.push_back(slidePanel(f.m, way, form, AN_LIVE, 2.0 + i * 0.37, panelMag(f.m, way, form), f.m.seed ^ 0x9F, speciesLine(f.s, f.m)));
            tally(f.m, form, 2.0 + i * 0.37, f.m.seed ^ 0x9F);
        }
        saveGrid(panels, 4, "life plates: " + wants[q].name, "shots/tests/life_plates_" + wants[q].name + ".png");
    }
    printf("life plates: %zu worlds near home and %zu stages and chemistries; %d spores, cysts and fossils drawn, %d held still where they lie%s (%.1f s)\n",
           worlds.size(), wants.size(), lying, held, held == lying ? "" : " - SOME MOVED", nowSec() - t0);
    return held == lying ? 0 : 1;
}

// the species near home that can do an act (the ship's own first), of a plan or any, the plans unseen first
std::vector<Morph> actors(int act, int plan, int want) {
    std::vector<Morph> out;
    std::map<int, int> seen;
    auto take = [&](const Morph& m) {
        if ((int)out.size() >= want || !actSuits(m, act) || (plan >= 0 && m.plan != plan) || seen[m.plan] > (plan >= 0 ? want : 0)) return;
        seen[m.plan]++;
        out.push_back(m);
    };
    for (int k = 0; k < SHIP_SPECIES; k++) take(shipMorph(k));
    for (const Star& st : starsNearHome(40)) {
        if ((int)out.size() >= want) break;
        StarSystem sys; sys.generate(st);
        auto L = lifeOf(sys);
        for (int b = 0; b < (int)sys.bodies.size() && (int)out.size() < want; b++) {
            if (L->bodies[b].species.empty()) continue;
            for (const Morph& m : morphsOf(sys, b)) take(m);
        }
    }
    return out;
}

// `life strips [plan]`: each act in eight frames over its time, a row a species
int lifeStrips(int plan) {
    const double t0 = nowSec();
    int strips = 0;
    Microbe preyS; preyS.plan = MP_ROD; preyS.seed = 0x9AE70001ULL; preyS.way = LW_DEAD; preyS.motion = MO_NONE;
    const Morph prey = morphOf(preyS, CH_WATER, homeGenesis());
    for (int act = 0; act < AN_COUNT; act++) {
        const std::vector<Morph> who = actors(act, plan, plan >= 0 ? 2 : 3);
        if (who.empty()) continue;
        std::vector<Panel> panels;
        for (const Morph& m : who) {
            Specimen s; s.m = &m; s.act = act; s.form = act == AN_WAKE ? CF_SPORE : CF_CELL; s.other = &prey; s.seed = m.seed ^ 0x57;
            s.angle = (unitFromHash(mix64(s.seed >> 3)) - 0.5) * 1.2;
            if (act == AN_TO_FIELD) s.angle = 0.3;
            const double T = actSeconds(m, act);
            const bool track = act == AN_LIVE;
            // the box its eight frames cover (relative to where it is, when the view follows it)
            double bx0 = 1e30, by0 = 1e30, bx1 = -1e30, by1 = -1e30;
            for (int f = 0; f < 8; f++) {
                const double t = act == AN_LIVE ? f * 0.125 : T * f / 7.0;
                double x0, y0, x1, y1, px = 0, py = 0, pa;
                specimenBox(s, t, x0, y0, x1, y1);
                if (track) specimenPlace(s, t, px, py, pa);
                bx0 = std::min(bx0, x0 - px); by0 = std::min(by0, y0 - py); bx1 = std::max(bx1, x1 - px); by1 = std::max(by1, y1 - py);
            }
            const double cx = (bx0 + bx1) * 0.5, cy = (by0 + by1) * 0.5;
            const double mag = magnificationForBox(bx1 - bx0, by1 - by0);
            const int way = act == AN_GLOW ? WS_GLOW : (act == AN_GATHER ? WS_DARK : seeingsFor(m)[0] == WS_GLOW ? WS_PHASE : seeingsFor(m)[0]);
            for (int f = 0; f < 8; f++) {
                const double t = act == AN_LIVE ? f * 0.125 : T * f / 7.0;
                panels.push_back(slidePanel(m, way, s.form, act, t, mag, s.seed, fmt("%s %s %.1fS", MICROBE_PLAN_NAMES[m.plan], ACT_NAMES[act], t), &prey, false, track, cx, cy));
            }
        }
        const std::string fn = plan >= 0 ? fmt("shots/tests/life_strip_%s_%s.png", lowerName(MICROBE_PLAN_NAMES[plan]).c_str(), lowerName(ACT_NAMES[act]).c_str())
                                         : fmt("shots/tests/life_strip_%s.png", lowerName(ACT_NAMES[act]).c_str());
        saveGrid(panels, 8, fmt("life strips: %s, eight frames over %.0f s", ACT_NAMES[act], actSeconds(who[0], act)), fn);
        strips++;
    }
    printf("life strips: %d acts (%.1f s)\n", strips, nowSec() - t0);
    return 0;
}

// a species' thumbnail: its form alone in a dark field (its edges, its flagella and its insides bright on black, as C-17's instruments on their
// case), its first look's view fitted to 96 px, at scale 1
std::vector<uint32_t> thumbOf(const Morph& m, int form) {
    const int TW = 96;
    Framebuffer fb;
    slidePalette(fb.pal, WS_DARK);
    Specimen s; s.m = &m; s.form = form; s.seed = 77; s.angle = 0;
    Slide v; v.seeing = WS_DARK; v.t = 0.5;
    double x0, y0, x1, y1, ang;
    specimenPlace(s, v.t, v.cx, v.cy, ang);   // the frame follows it
    // its body fitted to the frame (a spore's or a cyst's box)
    double viewH = 2.2 * m.viewUm();
    if (form != CF_CELL) { specimenBox(s, v.t, x0, y0, x1, y1); viewH = 2.2 * std::max(std::max(v.cx - x0, x1 - v.cx), std::max(v.cy - y0, y1 - v.cy)); }
    v.umPerPx = viewH / TW * FB_SCALE;
    drawSlide(fb, v, {s});
    fb.mush(2);
    std::vector<uint32_t> full((size_t)FBW * FBH), out((size_t)TW * TW);
    fb.toRGB(full.data(), 1.0);
    const int ox = (FBW - TW) / 2, oy = (FBH - TW) / 2;
    for (int y = 0; y < TW; y++) for (int x = 0; x < TW; x++) out[(size_t)y * TW + x] = full[(size_t)(oy + y) * FBW + ox + x];
    return out;
}
double thumbDistance(const std::vector<uint32_t>& a, const std::vector<uint32_t>& b) {
    long d = 0;
    for (size_t i = 0; i < a.size() && i < b.size(); i++) for (int s = 0; s < 24; s += 8) d += std::abs((int)((a[i] >> s) & 255) - (int)((b[i] >> s) & 255));
    return (double)d / std::max<size_t>(1, a.size() * 3);
}
constexpr double THUMB_ALIKE = 6.0;   // C-17's: two pictures nearer than this look alike

// `life distinct [species]`
int lifeDistinct(int want) {
    const double t0 = nowSec();
    // the worlds: no two of a world's species alike
    int worlds = 0, pairs = 0, alike = 0;
    double nearest = 1e9; std::string nearestText;
    struct Item { Morph m; int form; std::string where; };
    std::vector<Item> items;
    for (const Star& st : starsNearHome(40)) {
        if ((int)items.size() >= want && worlds >= 300) break;
        StarSystem sys; sys.generate(st);
        auto L = lifeOf(sys);
        for (int b = 0; b < (int)sys.bodies.size(); b++) {
            const Biota& B = L->bodies[b];
            if (B.species.size() < 2) continue;
            const std::vector<Morph> ms = morphsOf(sys, b);
            worlds++;
            for (size_t i = 0; i < ms.size(); i++) {
                for (size_t j = i + 1; j < ms.size(); j++) {
                    const double d = morphDistance(ms[i], ms[j]);
                    pairs++;
                    if (d < MORPH_ALIKE) { alike++; printf("  alike on %s: M%zu %s and M%zu %s, %.2f\n", sys.bodyLabel(b).c_str(), i, MICROBE_PLAN_NAMES[ms[i].plan], j, MICROBE_PLAN_NAMES[ms[j].plan], d); }
                    if (d < nearest) { std::string sh; morphDistance(ms[i], ms[j], &sh); nearest = d; nearestText = fmt("%s M%zu %s and M%zu %s, sharing %s", sys.bodyLabel(b).c_str(), i, MICROBE_PLAN_NAMES[ms[i].plan], j, MICROBE_PLAN_NAMES[ms[j].plan], sh.c_str()); }
                }
                if ((int)items.size() < want && B.species[i].state != SS_FOSSIL && B.species[i].fromBody < 0)
                    items.push_back({ms[i], B.species[i].state == SS_DORMANT ? CF_SPORE : CF_CELL, sys.bodyLabel(b)});
            }
        }
    }
    printf("life distinct: %d worlds near home, %d pairs of a world's species: %d alike (under %.1f of %.1f); the nearest %.2f, %s\n", worlds, pairs, alike, MORPH_ALIKE,
           morphMaxDistance(), nearest, nearestText.c_str());
    // the pictures: a thousand species' thumbnails, every pair compared
    const int keepScale = FB_SCALE, keepKernel = g_mushKernel;
    setFramebufferScale(1);
    std::vector<std::vector<uint32_t>> th(items.size());
    for (size_t i = 0; i < items.size(); i++) th[i] = thumbOf(items[i].m, items[i].form);
    struct Pair { double d; int i, j; };
    std::vector<std::vector<Pair>> per(items.size());
    parallelFor((int)items.size(), 4, [&](int a, int b) {
        for (int i = a; i < b; i++) {
            std::vector<Pair> best;
            for (size_t j = i + 1; j < items.size(); j++) {
                const double d = thumbDistance(th[i], th[j]);
                if (d < THUMB_ALIKE * 1.6) best.push_back({d, i, (int)j});
            }
            per[i] = std::move(best);
        }
    });
    std::vector<Pair> close;
    for (auto& v : per) close.insert(close.end(), v.begin(), v.end());
    std::sort(close.begin(), close.end(), [](const Pair& a, const Pair& b) { return a.d < b.d || (a.d == b.d && (a.i < b.i || (a.i == b.i && a.j < b.j))); });
    const long allPairs = (long)items.size() * ((long)items.size() - 1) / 2;
    long under = 0; for (const Pair& p : close) under += p.d < THUMB_ALIKE;
    long sameMorph = 0; for (const Pair& p : close) if (p.d < THUMB_ALIKE && morphDistance(items[p.i].m, items[p.j].m) < MORPH_ALIKE) sameMorph++;
    {   // which plans make the alike pairs, and the forms' distance of the alike (the threshold's evidence)
        std::map<std::pair<int, int>, int> byPlans; std::map<int, int> perPlan;
        std::vector<double> md;
        for (const Item& it : items) perPlan[it.m.plan]++;
        for (const Pair& p : close) if (p.d < THUMB_ALIKE) {
            const int a = std::min(items[p.i].m.plan, items[p.j].m.plan), b = std::max(items[p.i].m.plan, items[p.j].m.plan);
            byPlans[{a, b}]++; md.push_back(morphDistance(items[p.i].m, items[p.j].m));
        }
        std::vector<std::pair<int, std::pair<int, int>>> top;
        for (auto& kv : byPlans) top.push_back({kv.second, kv.first});
        std::sort(top.rbegin(), top.rend());
        printf("  the alike pairs by plans:");
        for (size_t k = 0; k < top.size() && k < 8; k++)
            printf(" %s/%s %d (of %d);", MICROBE_PLAN_NAMES[top[k].second.first], MICROBE_PLAN_NAMES[top[k].second.second], top[k].first,
                   top[k].second.first == top[k].second.second ? perPlan[top[k].second.first] * (perPlan[top[k].second.first] - 1) / 2 : perPlan[top[k].second.first] * perPlan[top[k].second.second]);
        std::sort(md.begin(), md.end());
        if (!md.empty()) printf("\n  their forms' distance: median %.2f, 90%% under %.2f, 99%% under %.2f", md[md.size() / 2], md[md.size() * 9 / 10], md[md.size() * 99 / 100]);
        printf("\n");
        // the threshold's evidence the other way: of the pairs whose forms are within a distance, the share whose pictures look alike
        std::vector<int> under(9, 0), both(9, 0);
        std::set<std::pair<int, int>> alikeSet;
        for (const Pair& p : close) if (p.d < THUMB_ALIKE) alikeSet.insert({p.i, p.j});
        for (size_t i = 0; i < items.size(); i++)
            for (size_t j = i + 1; j < items.size(); j++) {
                const double d = morphDistance(items[i].m, items[j].m);
                for (int q = 0; q < 9; q++) if (d < 1.0 + 0.5 * q) { under[q]++; both[q] += alikeSet.count({(int)i, (int)j}) ? 1 : 0; }
            }
        printf("  of the pairs whose forms are within:");
        for (int q = 0; q < 9; q++) printf(" %.1f %d (%.0f%% alike)", 1.0 + 0.5 * q, under[q], 100.0 * both[q] / std::max(1, under[q]));
        printf("\n");
    }
    printf("  %zu species' pictures (96 px, a dark field), %ld pairs: %ld alike (a mean difference under %.0f of 255), %.2f%%; of them %ld alike by their forms too\n",
           items.size(), allPairs, under, THUMB_ALIKE, 100.0 * under / std::max(1L, allPairs), sameMorph);
    {   // a sample of the pictures compared: the first 64
        const int TW = 96, cols = 16, n = (int)std::min<size_t>(64, items.size());
        Sheet sh(cols * (TW + 4) + 4, ((n + cols - 1) / cols) * (TW + 14) + 4);
        RGBCanvas cv{sh.px.data(), sh.w, sh.h, 1};
        for (int k = 0; k < n; k++) {
            const int x = 4 + (k % cols) * (TW + 4), y = 4 + (k / cols) * (TW + 14);
            sh.blit(th[k], TW, TW, x, y);
            drawText(cv, x, y + TW + 2, std::string(MICROBE_PLAN_NAMES[items[k].m.plan]).substr(0, 15).c_str(), rgb(200, 200, 200));
        }
        sh.save("shots/tests/life_distinct_sample.png");
    }
    // the sixteen nearest pairs side by side
    {
        const int TW = 96, gap = 6, cols = 4, n = (int)std::min<size_t>(16, close.size());
        Sheet sh(cols * (TW * 2 + gap * 3), ((n + cols - 1) / cols) * (TW + 22) + 16);
        RGBCanvas cv{sh.px.data(), sh.w, sh.h, 1};
        drawText(cv, 4, 3, fmt("LIFE DISTINCT: THE %d NEAREST PAIRS OF %zu", n, items.size()).c_str(), rgb(255, 220, 120));
        for (int k = 0; k < n; k++) {
            const int x = (k % cols) * (TW * 2 + gap * 3) + gap, y = 16 + (k / cols) * (TW + 22);
            sh.blit(th[close[k].i], TW, TW, x, y);
            sh.blit(th[close[k].j], TW, TW, x + TW + gap, y);
            drawText(cv, x, y + TW + 3, fmt("%.1f %s", close[k].d, MICROBE_PLAN_NAMES[items[close[k].i].m.plan]).c_str(), rgb(200, 200, 200));
        }
        sh.save("shots/tests/life_distinct.png");
    }
    setFramebufferScale(keepScale); g_mushKernel = keepKernel;
    const bool ok = alike == 0 && 100.0 * under / std::max(1L, allPairs) < 1.5;
    printf("  %s (%.1f s)\n", ok ? "DISTINCT" : "NOT DISTINCT", nowSec() - t0);
    return ok ? 0 : 1;
}

// `life forms [plan]`: every plan's variants in the six ways of seeing (`everyForm`)
int lifeForms(int only) {
    const double t0 = nowSec();
    int sheets = 0;
    const std::vector<Morph> forms = everyForm();
    for (int p = 0; p < MP_COUNT; p++) {
        if (only >= 0 && p != only) continue;
        std::vector<Panel> panels;
        for (const Morph& m : forms) {
            if (m.plan != p) continue;
            for (int way : {WS_BRIGHT, WS_PHASE, WS_DARK, WS_GLOW, WS_ELECTRON, WS_SECTION}) {
                const double mag = magnificationFor(m, way);
                panels.push_back(slidePanel(m, way, way == WS_SECTION ? CF_FOSSIL : CF_CELL, AN_LIVE, 1.5, mag, m.seed, fmt("%s V%d", MICROBE_PLAN_NAMES[p], m.variant), nullptr, false));
            }
        }
        saveGrid(panels, 6, fmt("life forms: %s, its variants in the six ways", MICROBE_PLAN_NAMES[p]), fmt("shots/tests/life_forms_%s.png", lowerName(MICROBE_PLAN_NAMES[p]).c_str()));
        sheets++;
    }
    printf("life forms: %d plans, %zu forms (%.1f s)\n", sheets, forms.size(), nowSec() - t0);
    return 0;
}

int planByName(const char* s) {
    if (!s) return -1;
    if (std::isdigit((unsigned char)s[0])) return std::max(-1, std::min(MP_COUNT - 1, atoi(s)));
    for (int p = 0; p < MP_COUNT; p++) if (lowerName(MICROBE_PLAN_NAMES[p]) == lowerName(s)) return p;
    return -1;
}
}   // namespace

// every plan's every variant: a species of the plan (light-fed where its plan lives by light, moving as its plan does, every stage of its life
// cycle) drawn again until each variant has come, of home's genesis
std::vector<Morph> everyForm() {
    std::vector<Morph> out;
    for (int p = 0; p < MP_COUNT; p++) {
        Microbe s; s.plan = p; s.seed = 0xF0A3000ULL + (uint64_t)p * 977; s.state = protocellPlan(p) ? SS_PROTO : SS_LIVING;
        s.way = p == MP_SHELLED || p == MP_ROLLER || p == MP_PLATE || p == MP_CHAIN || p == MP_FILAMENT ? LW_LIGHT : LW_DEAD;
        s.pigment = s.way == LW_LIGHT ? (p == MP_SHELLED ? PG_BROWN : PG_GREEN) : PG_NONE;
        s.motion = p == MP_ROD || p == MP_CURVED ? MO_RUN_TUMBLE : (p == MP_SPIRAL || p == MP_FLAGELLATE || p == MP_CILIATE || p == MP_ARMOURED ? MO_HELICAL
                   : (p == MP_CORKSCREW ? MO_CORKSCREW : (p == MP_ROLLER ? MO_ROLLING : (p == MP_AMOEBA || p == MP_TEST || p == MP_GATHERING ? MO_CRAWLING : MO_NONE))));
        s.cycle = LC_GROWING | LC_DIVIDING | LC_DORMANT | LC_WAKING | LC_DYING | (smallCellPlan(p) ? LC_SPORULATING : 0) | (colonyPlan(p) ? LC_COLONY : 0) |
                  (p == MP_GATHERING ? LC_GATHERING | LC_FRUITING : 0);
        s.division = protocellPlan(p) ? DV_FRAGMENTS : (p == MP_BUDDING ? DV_BUDDING : DV_FISSION); s.clade = 0;
        std::set<int> done;
        for (int nudge = 0; nudge < 80 && (int)done.size() < 4; nudge++) {
            const Morph m = morphOf(s, CH_WATER, homeGenesis(), nudge);
            if (done.count(m.variant)) continue;
            done.insert(m.variant);
            out.push_back(m);
        }
    }
    return out;
}

int runLifeLook(int argc, char** argv) {
    const std::string sub = argv[2];
    makeDir("shots"); makeDir("shots/tests");
    if (sub == "plates") return lifePlates(argc > 3 ? std::max(1, atoi(argv[3])) : 4);
    if (sub == "strips") return lifeStrips(argc > 3 ? planByName(argv[3]) : -1);
    if (sub == "distinct") return lifeDistinct(argc > 3 ? std::max(10, atoi(argv[3])) : 1000);
    if (sub == "forms") return lifeForms(argc > 3 ? planByName(argv[3]) : -1);

    return 2;
}
