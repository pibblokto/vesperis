// E-08: the peoples' places round the explorer: the settlements' and the works' cells (`ruinCell`, `workCell`), the towns' and
// the works' marks in the ground, the plan's places near, the ruins drawn (`drawRuins`); the records, the instruments and the
// graves near the explorer, their objects and their lights (`drawShards`)
#include "surface_internal.h"

// M4-07's jittered 2 km latitude/longitude grid, so a settlement stays where it is whatever the landing site or the anchor of the local
// frame. C-01: the grid, the draw and the settlements live in `peoples/ruins.*`; the view keeps a cache per cell in local metres.
void SurfaceView::ruinCellAt(double x, double z, int& gLat, int& gLon) const {
    double lat, lon;
    site.latLonAt(x, z, lat, lon);
    ruinCellOf(site.gen, lat, lon, gLat, gLon);
}

const SurfaceView::RuinCell* SurfaceView::ruinCell(int gLat, int gLon) const {
    int nLon = std::max(1, ruinCellsAround(site.gen, gLat));
    int gl = ((gLon % nLon) + nLon) % nLon;
    uint64_t key = hash2i(gLat, gl, 0x5E77ULL);
    auto it = ruinCells.find(key);
    if (it != ruinCells.end()) return &it->second;
    RuinCell& c = ruinCells[key];
    c.has = ruinOfCell(site.gen, gLat, gl, c.spec, true);
    if (c.has) { DrainageOff off; c.has = ruinSiteOk(site.gen, c.spec); }   // the slope and the sea, without building tiles (the water's own check is at the draw)
    if (c.has) {
        Ruin& r = c.r;
        site.localAt(StarSystem::bodyFromLatLon(c.spec.lat, c.spec.lon), r.x, r.z);
        r.heading = c.spec.heading; r.size = c.spec.size; r.spec = &c.spec;
        {   // C-03: and its shards; C-12: and its graves, their stones drawn with the pieces; C-13: in its people's culture and lore
            const Culture& cu = cultures[c.spec.people == 1 && peoples > 1 ? 1 : 0]; const Lore& lo = lores[c.spec.people == 1 && peoples > 1 ? 1 : 0];
            for (int l = 0; l < 3; l++) ruinElements(c.spec, cu, l, c.elems[l]); recordSitesOf(site.gen, c.spec, cu, c.shards, &c.instruments); gravesOf(lo, c.spec, cu, c.graves); graveElements(c.graves, cu, 0, c.elems[0]);
            recordObjects(c);   // T-04
            instrumentObjects(c);   // C-11
            groupPieces(c);   // A-02: each building's pieces, for its own level of detail
        }
    }
    return &c;
}

// T-03: a work's cell: its pieces laid out once (`workSpec`, peoples/works.h) as a settlement's are, in its people's culture, and a necropolis's
// or a battlefield's graves among them
const SurfaceView::RuinCell* SurfaceView::workCell(const Work& w) const {
    auto it = workCells.find(w.id);
    if (it != workCells.end()) return &it->second;
    RuinCell& c = workCells[w.id];
    workSpec(site.gen, w, c.spec);
    c.has = true;
    Ruin& r = c.r;
    site.localAt(w.unit, r.x, r.z);
    r.heading = c.spec.heading; r.size = c.spec.size; r.spec = &c.spec;
    const int k = w.people == 1 && peoples > 1 ? 1 : 0;
    for (int l = 0; l < 3; l++) ruinElements(c.spec, cultures[k], l, c.elems[l]);
    gravesOf(lores[k], c.spec, cultures[k], c.graves); graveElements(c.graves, cultures[k], 0, c.elems[0], true);
    recordSitesOf(site.gen, c.spec, cultures[k], c.shards); recordObjects(c);   // T-04: the works that kept records (the last shelter's last recording)
    groupPieces(c);
    return &c;
}

// T-02: the towns in reach (their cells within two of the explorer, a plan's core reaching in) put their streets, squares and fields into the
// ground, each once (the site keeps them until a landing or a re-anchor); the worker joined first, since the vertices read the list. T-03: and
// the works within six kilometres (of their reach) their ground
void SurfaceView::ensureTownMarks() {
    markCheckX = player.x; markCheckZ = player.z;
    if (!worldHadCivilisation(site.gen)) return;
    std::vector<const RuinCell*> todo;
    forNearbyRuins(player.x, player.z, 2, [&](const Ruin& r, const RuinCell& c, int) {
        if (c.spec.work < 0 && !c.marked && (!c.spec.streets.empty() || !c.spec.plazas.empty() || !c.spec.fields.empty())) todo.push_back(&c);
    });
    std::vector<const WorkNear*> works;
    for (const WorkNear& wn : workNear) {
        const Work& w = *wn.work;
        if (w.edits.empty() || workEdited.count(w.id) || std::hypot(wn.x - player.x, wn.z - player.z) - wn.radius > 6000) continue;
        works.push_back(&wn);
    }
    if (todo.empty() && works.empty()) return;
    joinAhead();
    for (const RuinCell* c : todo) {
        c->marked = true;
        const Culture& cu = cultures[c->spec.people == 1 && peoples > 1 ? 1 : 0];
        site.addTownMarks(c->spec, c->r.x, c->r.z, cu.ageYears + c->spec.before, cu.wet, cu.roadWear);
    }
    for (const WorkNear* wn : works) {
        const Work& w = *wn->work;
        workEdited.insert(w.id);
        const Culture& cu = cultures[w.people == 1 && peoples > 1 ? 1 : 0];
        site.addWorkEdits(w, wn->x, wn->z, cu.ageYears + w.before, cu.wet);
    }
}

void SurfaceView::refreshPlanNear() {   // T-01
    planNear.clear(); workNear.clear(); planNearX = player.x; planNearZ = player.z;
    if (!worldHadCivilisation(site.gen)) return;
    std::shared_ptr<const WorldPlan> P = worldPlanOf(site.gen);
    const Vec3 here = site.unitAt(player.x, player.z);
    std::vector<const PlanPlace*> near; planPlacesNear(*P, here, PLAN_NEAR_M, near);
    for (const PlanPlace* p : near) { PlanNear pn; pn.gLat = p->gLat; pn.gLon = p->gLon; site.localAt(p->unit, pn.x, pn.z); pn.radius = p->radiusM; planNear.push_back(pn); }
    if (worksHere) {   // T-03: the works; B-418: the world's and the land's
        std::vector<std::shared_ptr<const Work>> wn; worksAroundIfReady(site.gen, here, PLAN_NEAR_M, wn, LAND_NEAR_M);
        for (std::shared_ptr<const Work>& w : wn) { WorkNear n; site.localAt(w->unit, n.x, n.z); n.radius = w->radiusM; n.work = std::move(w); workNear.push_back(std::move(n)); }
    }
}

bool SurfaceView::ruinAt(int gLat, int gLon, Ruin& r) const {
    const RuinCell* c = ruinCell(gLat, gLon);
    if (!c->has) return false;
    r = c->r;
    return true;
}

// C-01: a settlement's pieces stand on the ground at their own foot, the near ones on the drawn ground (`groundHeight`, so the walls
// meet what the feet walk on and nothing floats as the near ring blends), the far ones on the planet function at 64 m, read once
// per building. A-02: every settlement in reach is gathered building by building and drawn as one batch (`drawFormItems`)
void SurfaceView::drawRuins(Framebuffer& fb) {
    lastRuinMs = 0; lastRuinElems = 0;
    if (!worldHadCivilisation(site.gen) && placedCells.empty()) return;
    auto tr0 = std::chrono::steady_clock::now();
    formItems.clear();
    for (const RuinCell& c : placedCells) gatherSettlement(c.r, c, std::hypot(c.r.x - camPos.x, c.r.z - camPos.z));   // A-02 (tests)
    static uint32_t drawStamp = 0; drawStamp++;
    forNearbyRuins(camPos.x, camPos.z, 2, [&](const Ruin& r, const RuinCell& cell, int) {
        cell.drawn = drawStamp;
        gatherSettlement(r, cell, std::hypot(r.x - camPos.x, r.z - camPos.z));
    });
    // T-01: the plan's places beyond the cells' reach, to PLAN_NEAR_M: a far city's skyline (each building at its level of detail, the far
    // ones a silhouette where they stand a pixel high)
    for (const PlanNear& pn : planNear) {
        const double d = std::hypot(pn.x - camPos.x, pn.z - camPos.z);
        if (d > PLAN_NEAR_M) continue;
        const RuinCell* c = ruinCell(pn.gLat, pn.gLon);
        if (c->has && c->drawn != drawStamp) { c->drawn = drawStamp; gatherSettlement(c->r, *c, d); }
    }
    gatherRecords();   // T-04
    lastRuinElems = (int)formItems.size();
    drawFormItems(fb);
    lastRuinMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tr0).count();
}

// C-03: the nearest shard the explorer does not have within 2.2 m, on this floor (within 2 m of height), for E and the HUD. T-04: its object,
// its place (a work's kind where it lies in one), whether it is a painted wall, one of an archive's, the last recording
void SurfaceView::findNearShard() {
    nearShard = NearShard();
    if (inVehicle() || !worldHadCivilisation(site.gen)) return;
    double best = 2.2;
    forNearbyRuins(player.x, player.z, 1, [&](const Ruin& r, const RuinCell& cell, int) {
        for (const ShardSite& s : cell.shards) {
            if (shardsFound.count(s.index)) continue;
            double x = r.x + s.x, z = r.z + s.z, d = std::sqrt((x - player.x) * (x - player.x) + (z - player.z) * (z - player.z));
            if (d >= best) continue;
            if (std::fabs(site.groundHeight(x, z) - player.y) > 2.0) continue;
            best = d; nearShard.index = s.index; nearShard.place = s.place; nearShard.sclass = cell.spec.sclass; nearShard.x = x; nearShard.z = z; nearShard.dist = d;
            nearShard.medium = s.medium; nearShard.work = cell.spec.work; nearShard.wall = s.wallD > 0; nearShard.archive = s.archive > 0; nearShard.last = s.last;
        }
    });
}

// C-11: the nearest instrument the explorer does not have within 2.2 m, on this floor, for E and the HUD
void SurfaceView::findNearInstrument() {
    nearInstrument = NearInstrument();
    if (inVehicle() || !worldHadCivilisation(site.gen)) return;
    double best = 2.2;
    forNearbyRuins(player.x, player.z, 1, [&](const Ruin& r, const RuinCell& cell, int) {
        if (cell.instrumentIds.size() != cell.instruments.size()) return;
        const int k = cell.spec.people == 1 && peoples > 1 ? 1 : 0;
        for (size_t i = 0; i < cell.instruments.size(); i++) {
            const int id = cell.instrumentIds[i];
            if (id < 0 || instrumentsFound.count(id)) continue;
            const InstrumentSite& s = cell.instruments[i];
            const double x = r.x + s.x, z = r.z + s.z, d = std::sqrt((x - player.x) * (x - player.x) + (z - player.z) * (z - player.z));
            if (d >= best || std::fabs(site.groundHeight(x, z) - player.y) > 2.0) continue;
            const Instrument* I = instrumentById(instrumentSets[k], id);
            if (!I) continue;
            best = d; nearInstrument.id = id; nearInstrument.name = instrumentName(traditions[k], *I); nearInstrument.people = k; nearInstrument.x = x; nearInstrument.z = z; nearInstrument.dist = d;
        }
    });
}

// C-11: a cell's instruments: which each site holds (its people's instrument of the record's age the pick names) and its object
void SurfaceView::instrumentObjects(RuinCell& c) const {
    c.instrumentIds.clear(); c.instrumentForms.clear(); c.instrumentFirst.assign(1, 0);
    const int k = c.spec.people == 1 && peoples > 1 ? 1 : 0;
    std::vector<Form> one;
    for (const InstrumentSite& s : c.instruments) {
        const int id = instrumentIdOf(instrumentSets[k], s.age, s.pick);
        const bool again = std::find(c.instrumentIds.begin(), c.instrumentIds.end(), id) != c.instrumentIds.end();   // one of each in a settlement
        const Instrument* I = id >= 0 && !again ? instrumentById(instrumentSets[k], id) : nullptr;
        c.instrumentIds.push_back(I ? id : -1);
        if (I) { instrumentForms(*I, traditions[k], s.x, s.z, s.heading, one); c.instrumentForms.insert(c.instrumentForms.end(), one.begin(), one.end()); }
        c.instrumentFirst.push_back((int)c.instrumentForms.size());
    }
}

void SurfaceView::waterRecords() {
    if (!worldHadCivilisation(site.gen)) return;
    forNearbyRuins(player.x, player.z, 1, [&](const Ruin& r, const RuinCell& cc, int) {
        if (cc.watered || std::hypot(r.x - player.x, r.z - player.z) > r.reach + 600) return;
        RuinCell& c = const_cast<RuinCell&>(cc);   // the view's own cache: its records laid again
        c.watered = true;
        if (c.shards.empty()) return;
        auto wetAt = [&](double sx, double sz) {   // a spot of the settlement's metres, and four round it a quarter of a cell off, 0.3 m over the water
            for (int k = 0; k < 5; k++) {
                const double x = r.x + sx + (k == 1 ? 4 : (k == 2 ? -4 : 0)), z = r.z + sz + (k == 3 ? 4 : (k == 4 ? -4 : 0)), w = site.waterAt(x, z);
                if (w > -1e8 && site.groundHeight(x, z) < w + 0.3) return true;
            }
            return false;
        };
        bool any = false;
        for (const ShardSite& s : c.shards) any = any || wetAt(s.x, s.z);
        for (const InstrumentSite& s : c.instruments) any = any || wetAt(s.x, s.z);
        if (!any) return;   // none in the water: as the generation laid them
        const std::function<bool(double, double)> fn = wetAt;
        const int k = c.spec.people == 1 && peoples > 1 ? 1 : 0;
        if (c.spec.work >= 0) recordSitesOf(site.gen, c.spec, cultures[k], c.shards, nullptr, &fn);
        else recordSitesOf(site.gen, c.spec, cultures[k], c.shards, &c.instruments, &fn);
        recordObjects(c);
        if (c.spec.work < 0) instrumentObjects(c);
    });
}

// C-12: the nearest grave within 2.2 m of the explorer on foot, for the HUD and the guide
void SurfaceView::findNearGrave() {
    nearGrave = NearGrave();
    if (inVehicle() || !worldHadCivilisation(site.gen)) return;
    double best = 2.2;
    forNearbyRuins(player.x, player.z, 1, [&](const Ruin& r, const RuinCell& cell, int) {
        for (size_t k = 0; k < cell.graves.size(); k++) {
            const Grave& g = cell.graves[k];
            double x = r.x + g.x, z = r.z + g.z, d = std::sqrt((x - player.x) * (x - player.x) + (z - player.z) * (z - player.z));
            if (d >= best) continue;
            best = d; nearGrave.k = (int)k; nearGrave.id = g.id; nearGrave.sclass = cell.spec.sclass; nearGrave.line = graveLine(lores[cell.spec.people == 1 && peoples > 1 ? 1 : 0], g); nearGrave.hud = g.name + (g.born > 0 ? ", " + std::to_string(g.born) + "-" + std::to_string(g.died) : ", to " + std::to_string(g.died)); nearGrave.x = x; nearGrave.z = z; nearGrave.dist = d;
        }
    });
}

// T-04: a cell's records' objects (`recordForms`, peoples/records.h), each from its own seed, kept with the cell (the frame's forms point at them)
void SurfaceView::recordObjects(RuinCell& c) {
    c.recordForms.clear(); c.recordFirst.assign(1, 0);
    std::vector<Form> one;
    for (const ShardSite& s : c.shards) {
        recordForms(s, mix64(c.spec.seed ^ ((uint64_t)(s.index + 1) * 0x9E3779B97F4A7C15ULL)), one);
        c.recordForms.insert(c.recordForms.end(), one.begin(), one.end());
        c.recordFirst.push_back((int)c.recordForms.size());
    }
}

// T-04: the records within 40 m of the camera into the frame's forms, on the floor where they lie (the drawn ground at the site), the ones the
// explorer has gone (a painted wall stays: it was recorded, not taken)
void SurfaceView::gatherRecords() {
    lastRecordForms = 0;
    if (!worldHadCivilisation(site.gen)) return;
    const double fwx = camLocal.m[2][0], fwz = camLocal.m[2][2];
    forNearbyRuins(camPos.x, camPos.z, 1, [&](const Ruin& r, const RuinCell& cell, int) {
        if (cell.recordFirst.size() != cell.shards.size() + 1) return;
        const int people = cell.spec.people == 1 && peoples > 1 ? 1 : 0;
        for (size_t k = 0; k < cell.shards.size(); k++) {
            const ShardSite& s = cell.shards[k];
            if (shardsFound.count(s.index) && !(s.medium == RM_PAINTED && s.wallD > 0)) continue;
            const double x = r.x + s.x, z = r.z + s.z, dx = x - camPos.x, dz = z - camPos.z, d = std::sqrt(dx * dx + dz * dz);
            if (d > 40 || (d > 3 && dx * fwx + dz * fwz < -1.5)) continue;
            const double gy = site.groundHeight(x, z), w = site.waterAt(x, z);
            if (w > -1e8 && gy < w) continue;
            const double fog = 1 - std::exp(-d / env.fogDistance);
            for (int i = cell.recordFirst[k]; i < cell.recordFirst[k + 1]; i++) {
                const Form& e = cell.recordForms[i];
                formItems.push_back({&e, Vec3(r.x + e.x, gy, r.z + e.z), 0.0, d, fog, 0, people});
                lastRecordForms++;
            }
        }
        if (cell.instrumentFirst.size() != cell.instruments.size() + 1 || cell.instrumentIds.size() != cell.instruments.size()) return;
        for (size_t k = 0; k < cell.instruments.size(); k++) {   // C-11: the instruments beside them, till taken
            const InstrumentSite& s = cell.instruments[k];
            if (cell.instrumentIds[k] < 0 || instrumentsFound.count(cell.instrumentIds[k])) continue;
            const double x = r.x + s.x, z = r.z + s.z, dx = x - camPos.x, dz = z - camPos.z, d = std::sqrt(dx * dx + dz * dz);
            if (d > 40 || (d > 3 && dx * fwx + dz * fwz < -1.5)) continue;
            const double gy = site.groundHeight(x, z), w = site.waterAt(x, z);
            if (w > -1e8 && gy < w) continue;
            const double fog = 1 - std::exp(-d / env.fogDistance);
            for (int i = cell.instrumentFirst[k]; i < cell.instrumentFirst[k + 1]; i++) {
                const Form& e = cell.instrumentForms[i];
                if (d > 7 && (e.kind == FK_CABLE || formReach(e) < 0.012 * d / 7)) continue;   // C-17: far off, its strings and its small parts are under a pixel
                formItems.push_back({&e, Vec3(r.x + e.x, gy, r.z + e.z), 0.0, d, fog, 0, people});
                lastRecordForms++;
            }
        }
    });
}

// C-03: over each shard the explorer does not have, a point of light that pulses (the stars' bank), drawn within 250 m behind the depth test,
// so a glint through a doorway or over a fallen wall is what gives one away. T-04: the object itself is drawn with the forms (`gatherRecords`);
// a painted wall's light is on the wall, at the painting's height
void SurfaceView::drawShards(Framebuffer& fb, double t) {
    lastShardsDrawn = 0;
    if (!worldHadCivilisation(site.gen)) return;
    forNearbyRuins(camPos.x, camPos.z, 2, [&](const Ruin& r, const RuinCell& cell, int) {
        for (const ShardSite& s : cell.shards) {
            if (shardsFound.count(s.index)) continue;
            double x = r.x + s.x, z = r.z + s.z;
            const double gy = site.groundHeight(x, z), w = site.waterAt(x, z);
            if (w > -1e8 && gy < w) continue;
            double y = gy + 0.12;
            if (s.wallD > 0) { x -= std::sin(s.heading) * (s.wallD - 0.05); z -= std::cos(s.heading) * (s.wallD - 0.05); y = gy + 1.25; }
            else if (s.medium == RM_CRYSTAL || s.medium == RM_HUM) y = gy + 0.3;
            double dx = x - camPos.x, dz = z - camPos.z, d = std::sqrt(dx * dx + dz * dz);
            if (d > 250) continue;
            double pulse = 0.5 + 0.5 * std::sin(t * 2.6 + s.index * 1.7);
            Vec3 v = toView(x, y, z);
            if (v.z > NEAR_Z) { RVert q; q.x = v.x; q.y = v.y; q.z = v.z; q.shade = 48 + 15 * pulse; rasterPoint3(fb, q, 4, proj, true, d < 40 ? 2 : 1); }
            lastShardsDrawn++;
        }
        if (cell.instrumentIds.size() != cell.instruments.size()) return;
        for (size_t k = 0; k < cell.instruments.size(); k++) {   // C-11: an instrument's light: a double pulse, so it is told from a record's at a glance
            const int id = cell.instrumentIds[k];
            if (id < 0 || instrumentsFound.count(id)) continue;
            const InstrumentSite& s = cell.instruments[k];
            const double x = r.x + s.x, z = r.z + s.z;
            const double gy = site.groundHeight(x, z), w = site.waterAt(x, z);
            if (w > -1e8 && gy < w) continue;
            const double dx = x - camPos.x, dz = z - camPos.z, d = std::sqrt(dx * dx + dz * dz);
            if (d > 250) continue;
            const double ph = std::fmod(t * 0.9 + id * 0.37, 1.0);
            const double pulse = std::exp(-std::pow((ph - 0.15) / 0.06, 2)) + std::exp(-std::pow((ph - 0.38) / 0.06, 2));
            Vec3 v = toView(x, gy + 0.35, z);
            if (v.z > NEAR_Z) { RVert q; q.x = v.x; q.y = v.y; q.z = v.z; q.shade = 40 + 23 * std::min(1.0, pulse); rasterPoint3(fb, q, 4, proj, true, d < 40 ? 2 : 1); }
            lastShardsDrawn++;
        }
    });
}
