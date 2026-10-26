// E-08: what the world does round the explorer: a comet's vents, the geysers and fountains (`drawJet`), the spores, the plumes,
// the dry ice's spring jets, the tiger stripes' curtains, the lamps left burning, and the weather (`drawWeather`: the dust
// devils, the eruptions, rain, snow and dust)
#include "surface_internal.h"

// on an active comet the sunlit ground vents: hashed vents on a 250 m grid, each a column of dust
// points rising a few metres a second for half a minute and bending away from the sun (radiation pressure)
void SurfaceView::drawCometJets(Framebuffer& fb, double t) {
    double act = env.cometActivity;
    const Vec3& sd = env.sun.dirLocal;
    if (act < 0.05 || sd.y < 0.03) return;
    double horiz = std::sqrt(sd.x * sd.x + sd.z * sd.z);
    double ax = horiz > 1e-4 ? -sd.x / horiz : 1, az = horiz > 1e-4 ? -sd.z / horiz : 0;   // away from the sun, along the ground
    const double cs = 250;
    int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
    for (int cz = pcz - 3; cz <= pcz + 3; cz++)
        for (int cx = pcx - 3; cx <= pcx + 3; cx++) {
            uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0x1E75);
            if (hash01(h) > 0.06 + 0.16 * act) continue;
            double vx = cx * cs + hash01(mix64(h + 1)) * cs, vz = cz * cs + hash01(mix64(h + 2)) * cs;
            double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
            if (dist > 750) continue;
            double gy = site.groundHeight(vx, vz);
            double strength = 0.5 + 0.5 * hash01(mix64(h + 3));
            double rise = (3.0 + 4.0 * strength) * (0.5 + 0.5 * act);   // m/s
            int n = (int)(18 + 30 * act * strength);
            Rng r(h);
            for (int i = 0; i < n; i++) {
                double ph = r.uni(), a = r.range(0, TAU), spread = r.range(0.3, 1.0);
                double age = std::fmod(t * 0.5 + ph * 30.0, 30.0);   // seconds since this puff left the vent
                double up = rise * age, out = (0.6 + 0.6 * age * 0.25) * spread;
                double px = vx + std::cos(a) * out + ax * age * 1.2 * spread, pz = vz + std::sin(a) * out + az * age * 1.2 * spread;
                Vec3 v = toView(px, gy + up, pz);
                if (v.z < NEAR_Z) continue;
                RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 60 - age * 1.3;
                rasterPoint3(fb, p, 8, proj, true, dist < 120 && age < 8 ? 2 : 1);
            }
            {   // the vent itself: a bright spot of fresh ice
                Vec3 v = toView(vx, gy + 0.2, vz);
                if (v.z > NEAR_Z) { RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 62; rasterPoint3(fb, p, 8, proj, true, 2); }
            }
        }
}

// R-307: the geysers and the fountains. A vent's jet (`drawJet`) is a dense core column of camera-facing quads, cut ragged
// at its edges by the boundary tile and streaked by the fine grain scrolling up at the jet's own speed, bright at the throat
// and fraying toward the top; a crown of spray where it breaks; and a veil of drops falling back on ballistic arcs in the
// world's gravity, widening as they fall. Its height and width follow the vent's class (small, tall, huge by hash) and its
// strength this frame (`jetStrength`): about a third of the vents blow steadily, pulsing a little, the rest in cycles of one
// to three minutes that start hard (a third over full height in the first seconds) and die over the last tenth
double SurfaceView::jetStrength(uint64_t h, double t, bool constant, double cycle, double duty) {
    double phase = hash01(mix64(h + 5));
    if (constant) return 0.82 + 0.18 * std::sin(t * 0.9 + phase * TAU) + 0.04 * std::sin(t * 6.3 + phase * 11.0);
    double ph = std::fmod(t / cycle + phase, 1.0);
    if (ph > duty) return 0;
    double build = smoothstep(0.0, 0.035, ph), burst = 1.0 + 0.35 * std::exp(-ph / 0.05);
    return build * burst * smoothstep(duty, duty - 0.1, ph);
}

double SurfaceView::burstJetStrength(uint64_t id, double t) { return jetStrength(id, t, false, 30 + 60 * hash01(mix64(id + 3)), 0.15 + 0.15 * hash01(mix64(id + 4))); }

double SurfaceView::basinJetStrength(uint64_t id, double t) {
    bool constant = hash01(mix64(id + 9)) < 0.3;
    return jetStrength(id, t, constant, 60 + 120 * hash01(mix64(id + 3)), 0.2 + 0.3 * hash01(mix64(id + 4)));
}

void SurfaceView::drawJet(Framebuffer& fb, double t, const JetSpec& j, double strength) {
    if (strength <= 0.02) return;
    double gAcc = std::max(0.3, site.gravity);
    double sunUp = smoothstep(-0.03, 0.06, env.sun.dirLocal.y);
    double H = j.heightM * (0.55 + 0.45 * std::min(strength, 1.35)), W = j.widthM * (0.7 + 0.3 * std::min(strength, 1.0));
    double gy = site.groundHeight(j.x, j.z);
    double baseShade = j.lava ? 61 : (site.sys->sunless() ? 20 : 38 + 22 * sunUp) + j.shadeAdd;   // steam and ice are lit by the sun; lava lights itself (D-04: in a drifter's dark only faintly seen)
    double vJet = std::sqrt(2 * gAcc * H);                // the ejection speed: the streaks climb the column at it
    // the core column: eight segments, each two quads round the axis so the weight peaks at the centre and the tile cuts the sides ragged
    {
        RasterParams rp;
        rp.bank = j.bank; rp.grain = &grainFine; rp.grainScale = 0.9; rp.grainWeight = 1.0;
        rp.cutout = true; rp.edge = &grainEdge; rp.edgeScale = 0.35 / std::max(1.0, W * 0.15); rp.edgeAmp = 0.75;
        const int NSEG = 8;
        for (int k = 0; k < NSEG; k++) {
            double f0 = (double)k / NSEG, f1 = (double)(k + 1) / NSEG;
            double y0 = H * f0, y1 = H * f1;
            double r0 = W * 0.5 * (0.7 + 1.4 * std::pow(f0, 1.6)), r1 = W * 0.5 * (0.7 + 1.4 * std::pow(f1, 1.6));
            Vec3 c0 = toView(j.x, gy + y0, j.z), c1 = toView(j.x, gy + y1, j.z);
            if (c0.z < NEAR_Z || c1.z < NEAR_Z) continue;
            double core0 = 1.15 * (1 - 0.55 * f0 * f0), core1 = 1.15 * (1 - 0.55 * f1 * f1);   // the column frays toward the top
            double sh0 = baseShade - (j.lava ? 9 : 14) * f0, sh1 = baseShade - (j.lava ? 9 : 14) * f1;
            double v0 = (y0 - t * vJet) * 0.35, v1 = (y1 - t * vJet) * 0.35;
            for (int side = -1; side <= 1; side += 2) {
                RVert q[4];
                q[0].x = c0.x; q[0].y = c0.y; q[0].z = c0.z; q[0].w = core0; q[0].shade = sh0 + 1; q[0].u = 0; q[0].v = v0;
                q[1].x = c0.x + side * r0; q[1].y = c0.y; q[1].z = c0.z; q[1].w = 0.2; q[1].shade = sh0 - 4; q[1].u = side * r0 * 1.5; q[1].v = v0;
                q[2].x = c1.x + side * r1; q[2].y = c1.y; q[2].z = c1.z; q[2].w = 0.2; q[2].shade = sh1 - 4; q[2].u = side * r1 * 1.5; q[2].v = v1;
                q[3].x = c1.x; q[3].y = c1.y; q[3].z = c1.z; q[3].w = core1; q[3].shade = sh1 + 1; q[3].u = 0; q[3].v = v1;
                rasterPolygon(fb, q, 4, rp, proj);
            }
        }
    }
    bool near = j.dist < 350;
    double s = std::min(strength, 1.35);
    // the crown: spray thrown out where the column breaks, falling from it for a few seconds
    {
        int n = (int)((near ? 340 : 130) * s);
        Rng r(j.h ^ 0xC20);
        for (int i = 0; i < n; i++) {
            double u = r.uni(), a = r.range(0, TAU), rr = W * (0.5 + 2.8 * std::sqrt(u));
            double tau = std::fmod(t * 1.3 + r.uni() * 3.0, 3.0);   // seconds since this drop left the crown
            double y = H * (0.86 + 0.22 * r.uni()) + 0.15 * vJet * tau - 0.5 * gAcc * tau * tau;
            if (y < 0) continue;
            double out = rr + tau * (2.0 + 0.1 * vJet) * (0.4 + 0.6 * u);
            Vec3 v = toView(j.x + std::cos(a) * out, gy + y, j.z + std::sin(a) * out);
            if (v.z < NEAR_Z) continue;
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = baseShade - 6 - 8 * (tau / 3.0);
            rasterPoint3(fb, p, j.bank, proj, true, near ? 2 : 1);
        }
    }
    // the veil: drops on full ballistic arcs from the throat, the outer ones widening as they fall
    {
        int n = (int)((near ? 300 : 110) * s);
        double flight = 2 * vJet / gAcc;
        Rng r(j.h ^ 0x7E1);
        for (int i = 0; i < n; i++) {
            double u = r.uni(), a = r.range(0, TAU), spread = r.range(0.15, 1.0), tilt = r.range(0.02, 0.2);
            double age = std::fmod(t * 0.7 + u * flight, flight);
            double up = vJet * std::cos(tilt) * age - 0.5 * gAcc * age * age;
            if (up < 0) continue;
            double out = W * 0.4 + vJet * std::sin(tilt) * age * spread;
            Vec3 v = toView(j.x + std::cos(a) * out, gy + up, j.z + std::sin(a) * out);
            if (v.z < NEAR_Z) continue;
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = baseShade - 4 - 10 * (age / flight);
            rasterPoint3(fb, p, j.bank, proj, true, near && age < flight * 0.5 ? 2 : 1);
        }
    }
    {   // the throat: a bright pool with the fallout splashing round it
        Vec3 v = toView(j.x, gy + 0.3, j.z);
        if (v.z > NEAR_Z) { RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = j.lava ? 62 : baseShade + 2; rasterPoint3(fb, p, j.bank, proj, true, near ? 3 : 2); }
    }
}

void SurfaceView::drawGeysers(Framebuffer& fb, double t) {
    // a vent's class by hash: small, tall or huge, and whether it blows steadily
    auto classOf = [](uint64_t h, double& hM, double& wM, const double hs[3], const double ws[3]) {
        double u = hash01(mix64(h + 7)), v = hash01(mix64(h + 8));
        int c = u < 0.45 ? 0 : (u < 0.85 ? 1 : 2);
        hM = hs[c] * (0.8 + 0.4 * v); wM = ws[c] * (0.8 + 0.4 * v);
    };
    const uint32_t events = KINDS[site.gen.type].events;
    if (events & EV_CRACK_VENTS) {   // the cracks: vents hashed on 300 m cells where the 16 m ring is stained
        static const double hs[3] = {160, 400, 800}, ws[3] = {7, 15, 30};
        const double cs = 300;
        int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
        for (int cz = pcz - 8; cz <= pcz + 8; cz++)
            for (int cx = pcx - 8; cx <= pcx + 8; cx++) {
                uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0x6E75);
                if (hash01(h) > 0.35) continue;
                double vx = cx * cs + hash01(mix64(h + 1)) * cs, vz = cz * cs + hash01(mix64(h + 2)) * cs;
                if (site.lod0.at((int)std::floor(vx / 16), (int)std::floor(vz / 16)).material != MAT_DUST) continue;
                double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
                if (dist > 2500) continue;
                JetSpec j; j.x = vx; j.z = vz; j.h = h; j.dist = dist; j.bank = 8; j.lava = false;
                classOf(h, j.heightM, j.widthM, hs, ws);
                bool constant = hash01(mix64(h + 9)) < 0.35;
                drawJet(fb, t, j, jetStrength(h, t, constant, 90 + 90 * hash01(mix64(h + 3)), 0.25 + 0.3 * hash01(mix64(h + 4))));
            }
    }
    if (site.gen.hasTrait(TR_GEYSERS)) {   // the basins: a hot spring at each sinter mound's vent
        static const double hs[3] = {30, 80, 180}, ws[3] = {2.5, 5, 10};
        std::vector<GeyserVent> vents;
        geyserVents(site.gen, site.unitAt(camPos.x, camPos.z), vents);
        for (const GeyserVent& gv : vents) {
            double vx, vz;
            site.localAt(gv.unit, vx, vz);
            double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
            if (dist > 2500) continue;
            if (site.lod0.at((int)std::floor(vx / 16), (int)std::floor(vz / 16)).material != MAT_SALT) continue;   // where the landform put its sinter
            JetSpec j; j.x = vx; j.z = vz; j.h = gv.id; j.dist = dist; j.bank = 8; j.lava = false;
            classOf(gv.id, j.heightM, j.widthM, hs, ws);
            drawJet(fb, t, j, basinJetStrength(gv.id, t));
        }
    }
    if (events & EV_STEAM_BURSTS) {   // P-01: steam bursting now and then from the vents of the kind's own ground (the steam world's sinter mounds)
        const int vent = KINDS[site.gen.type].eventMaterial;
        std::vector<GeyserVent> vents;
        steamVents(site.gen, site.unitAt(camPos.x, camPos.z), vents);
        for (const GeyserVent& gv : vents) {
            double vx, vz;
            site.localAt(gv.unit, vx, vz);
            double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
            if (dist > 2500) continue;
            if (site.lod0.at((int)std::floor(vx / 16), (int)std::floor(vz / 16)).material != vent) continue;   // a mound in a field of vents
            JetSpec j; j.x = vx; j.z = vz; j.h = gv.id; j.dist = dist; j.bank = 8; j.lava = false;
            j.heightM = 50 + 150 * hash01(mix64(gv.id + 7)); j.widthM = 5 + 8 * hash01(mix64(gv.id + 8));
            drawJet(fb, t, j, burstJetStrength(gv.id, t));
        }
    }
    if (site.gen.hasTrait(TR_HOT_SPRINGS)) {   // steam rising steadily off each terraced spring's vent pool
        std::vector<GeyserVent> vents;
        hotSpringsNear(site.gen, site.unitAt(camPos.x, camPos.z), vents);
        for (const GeyserVent& gv : vents) {
            double vx, vz;
            site.localAt(gv.unit, vx, vz);
            const double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
            if (dist > 1500) continue;
            if (site.lod0.at((int)std::floor(vx / 16 + 0.5), (int)std::floor(vz / 16 + 0.5)).material != MAT_BRINE) continue;   // where the landform put its pool
            JetSpec j; j.x = vx; j.z = vz; j.h = gv.id; j.dist = dist; j.bank = 8; j.lava = false;
            j.heightM = 6 + 14 * hash01(mix64(gv.id + 7)); j.widthM = gv.radiusKm * 1000.0 * 0.9;
            drawJet(fb, t, j, jetStrength(gv.id, t, true, 1, 1));
        }
    }
    if (events & EV_SPRINGS) {   // the chemotroph's hot springs: steam rising steadily off every pool its ground made (`springVents`), one in five a geyser now and then
        std::vector<GeyserVent> vents;
        springVents(site.gen, site.unitAt(camPos.x, camPos.z), vents);
        for (const GeyserVent& gv : vents) {
            double vx, vz;
            site.localAt(gv.unit, vx, vz);
            const double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
            if (dist > 1200) continue;
            if (site.lod0.at((int)std::floor(vx / 16 + 0.5), (int)std::floor(vz / 16 + 0.5)).material != MAT_BRINE) continue;   // a pool of the spring fields (the vertex nearest its centre)
            JetSpec j; j.x = vx; j.z = vz; j.h = gv.id; j.dist = dist; j.bank = 8; j.lava = false;
            const bool geyser = hash01(mix64(gv.id + 6)) < 0.2;
            j.heightM = geyser ? 20 + 40 * hash01(mix64(gv.id + 7)) : 3 + 7 * hash01(mix64(gv.id + 7));
            j.widthM = gv.radiusKm * 1000.0 * (geyser ? 0.25 : 0.7);
            drawJet(fb, t, j, geyser ? jetStrength(gv.id, t, false, 90 + 120 * hash01(mix64(gv.id + 3)), 0.15 + 0.15 * hash01(mix64(gv.id + 4))) : jetStrength(gv.id, t, true, 1, 1));
        }
    }
    if (events & EV_FUMAROLES) {   // fumaroles and hot springs on the sulphur crusts, steady fountains on the lava (the great ones come and go through the eruption list)
        const double cs = 200;
        int pcx = (int)std::floor(camPos.x / cs), pcz = (int)std::floor(camPos.z / cs);
        for (int cz = pcz - 5; cz <= pcz + 5; cz++)
            for (int cx = pcx - 5; cx <= pcx + 5; cx++) {
                uint64_t h = hash2i(cx, cz, site.gen.seed ^ 0xF0DA);
                if (hash01(h) > 0.4) continue;
                double vx = cx * cs + hash01(mix64(h + 1)) * cs, vz = cz * cs + hash01(mix64(h + 2)) * cs;
                int mat = site.lod0.at((int)std::floor(vx / 16), (int)std::floor(vz / 16)).material;
                if (mat != MAT_SULPHUR && mat != MAT_LAVA) continue;
                double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
                if (dist > 1100) continue;
                JetSpec j; j.x = vx; j.z = vz; j.h = h; j.dist = dist;
                if (mat == MAT_LAVA) {   // a steady fountain of 30-90 m
                    j.bank = 2; j.lava = true; j.heightM = 30 + 60 * hash01(mix64(h + 7)); j.widthM = 3 + 5 * hash01(mix64(h + 8));
                    drawJet(fb, t, j, jetStrength(h, t, true, 1, 1));
                } else {
                    bool spring = hash01(mix64(h + 6)) < 0.25;   // one crust vent in four is a periodic hot spring, the rest steam steadily
                    j.bank = 8; j.lava = false;
                    j.heightM = spring ? 60 + 90 * hash01(mix64(h + 7)) : 15 + 25 * hash01(mix64(h + 7)); j.widthM = spring ? 4 + 4 * hash01(mix64(h + 8)) : 2 + 2 * hash01(mix64(h + 8));
                    drawJet(fb, t, j, jetStrength(h, t, !spring, 70 + 80 * hash01(mix64(h + 3)), 0.2 + 0.25 * hash01(mix64(h + 4))));
                }
            }
    }
}

// P-03: the chemotroph's spores at night: a haze of glowing points over each spring near the explorer (its pool and its mats), rising slowly on
// the warm air, drifting with the wind, pulsing and fading as they climb; in their own light (bank 15), so only the dark shows them
void SurfaceView::drawSpores(Framebuffer& fb, double t) {
    const double dark = 1 - smoothstep(0.2, 0.55, env.skyBrightness);
    if (dark <= 0.01) return;
    std::vector<GeyserVent> vents;
    springVents(site.gen, site.unitAt(camPos.x, camPos.z), vents);
    const double wx = std::sin(env.windDir), wz = std::cos(env.windDir), lean = 0.04 * env.windKnots;   // metres downwind a metre of rise
    for (const GeyserVent& gv : vents) {
        double vx, vz;
        site.localAt(gv.unit, vx, vz);
        const double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
        if (dist > 500) continue;
        if (site.lod0.at((int)std::floor(vx / 16 + 0.5), (int)std::floor(vz / 16 + 0.5)).material != MAT_BRINE) continue;
        const double gy = site.groundHeight(vx, vz), r = gv.radiusKm * 1000.0;
        const int n = (int)((dist < 150 ? 260 : 90) * dark);
        Rng sr(gv.id ^ 0x5B0EULL);
        for (int i = 0; i < n; i++) {
            const double a = sr.range(0, TAU), rr = r * (0.2 + 2.8 * std::pow(sr.uni(), 0.9)), top = 4 + 12 * sr.uni(), speed = 0.12 + 0.25 * sr.uni(), ph = sr.uni();
            const double age = std::fmod(t * speed / top + ph, 1.0), up = age * top;   // 0 at the ground .. 1 at its top
            const double sx = 0.5 * std::sin(t * 0.6 + 9.0 * ph), sz = 0.5 * std::cos(t * 0.5 + 7.0 * ph);
            const Vec3 v = toView(vx + std::cos(a) * rr + wx * lean * up + sx, gy + 0.2 + up, vz + std::sin(a) * rr + wz * lean * up + sz);
            if (v.z < NEAR_Z) continue;
            const double pulse = 0.65 + 0.35 * std::sin(t * (1.2 + 2.0 * ph) + 11.0 * ph);
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 30 + 33 * dark * pulse * (1 - 0.6 * age);
            rasterPoint3(fb, p, 15, proj, true, dist < 70 && ph < 0.4 ? 2 : 1);
        }
    }
}

// P-05: the cryovolcanic world's plumes (Triton's: Soderblom et al. 1990): from each active vent on its cap within 220 km (`plumeVents`; half of
// them active, by hash) a dark column 6-9 km high and 200-500 m wide, its top carried downwind along the prevailing wind (the ground's streaks'
// direction) as a flag 40-90 km long, thickening from a few hundred metres to two kilometres and fading; grey by day, a dim shape at night;
// the curvature hides the far columns' feet (`toView`); a far column is kept a pixel wide
void SurfaceView::drawPlumes(Framebuffer& fb, double t) {
    std::vector<GeyserVent> vents;
    plumeVents(site.gen, site.unitAt(camPos.x, camPos.z), vents);
    const double sunUp = smoothstep(-0.05, 0.15, env.sun.dirLocal.y);
    RasterParams rp;
    rp.bank = 9; rp.grain = &grainFine; rp.grainScale = 0.9; rp.grainWeight = 1.0;
    rp.cutout = true; rp.edge = &grainEdge; rp.edgeAmp = 0.75;
    for (const GeyserVent& v : vents) {
        if (!plumeActive(v.id)) continue;
        double vx, vz; site.localAt(v.unit, vx, vz);
        const double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
        if (dist > 220000) continue;
        double wx, wz; site.localAt(normalize(v.unit + prevailingWind(site.gen, v.unit) * (1000.0 / site.R)), wx, wz);   // a kilometre downwind
        double dx = wx - vx, dz = wz - vz; const double dl = std::max(1e-6, std::sqrt(dx * dx + dz * dz)); dx /= dl; dz /= dl;
        double water = 0;
        const double gy = dist < 40000 ? heightNear(vx, vz, dist, water) : (double)site.sampleAt(vx, vz, 2048).h;
        const double H = 6000 + 3000 * hash01(mix64(v.id + 2)), W = std::max(600 + 900 * hash01(mix64(v.id + 3)), dist * 0.006);
        const double Lf = std::min(v.radiusKm * 800.0, 90000.0), sway = 0.08 * std::sin(t * 0.05 + 6.0 * hash01(mix64(v.id + 4)));
        const double base = 20 + 16 * sunUp, ph = TAU * hash01(mix64(v.id + 5));
        const int NSEG = 10;
        rp.edgeScale = 0.35 / std::max(1.0, W * 0.15);
        for (int k = 0; k < NSEG; k++) {   // the column, a ribbon facing the camera, widening a little toward its top
            const double f0 = (double)k / NSEG, f1 = (double)(k + 1) / NSEG;
            const Vec3 c0 = toView(vx, gy + H * f0, vz), c1 = toView(vx, gy + H * f1, vz);
            if (c0.z < NEAR_Z || c1.z < NEAR_Z) continue;
            const double r0 = W * 0.5 * (1 + 0.5 * f0), r1 = W * 0.5 * (1 + 0.5 * f1), v0 = (H * f0 - t * 60) * 0.35, v1 = (H * f1 - t * 60) * 0.35;
            for (int side = -1; side <= 1; side += 2) {
                RVert q[4];
                q[0].x = c0.x; q[0].y = c0.y; q[0].z = c0.z; q[0].w = 1.15; q[0].shade = base - 4 * f0; q[0].u = 0; q[0].v = v0;
                q[1].x = c0.x + side * r0; q[1].y = c0.y; q[1].z = c0.z; q[1].w = 0.2; q[1].shade = base - 6 - 4 * f0; q[1].u = side * W * 0.75; q[1].v = v0;
                q[2].x = c1.x + side * r1; q[2].y = c1.y; q[2].z = c1.z; q[2].w = 0.2; q[2].shade = base - 6 - 4 * f1; q[2].u = side * W * 0.75; q[2].v = v1;
                q[3].x = c1.x; q[3].y = c1.y; q[3].z = c1.z; q[3].w = 1.15; q[3].shade = base - 4 * f1; q[3].u = 0; q[3].v = v1;
                rasterPolygon(fb, q, 4, rp, proj);
            }
        }
        // the flag: a cross of two ribbons along the wind (an upright one, seen from the side, and a flat one, seen from under it), each a
        // dense middle and ragged edges, billowing a little, thickening from 600 m to three kilometres and fraying out toward its end
        const int NF = 16;
        const double px = -dz, pz = dx;   // across the wind
        auto at = [&](double f, double& x, double& y, double& z, double& th) {
            const double sq = Lf * f, a = sway * f;
            x = vx + (dx + px * a) * sq; z = vz + (dz + pz * a) * sq;
            y = gy + H - 700 * (1 - smoothstep(0.0, 0.12, f)) + 900 * f + 260 * std::sin(sq / 9000.0 + ph + t * 0.02) * f;   // bending over from the column's top
            th = std::max(W * 0.7 + 1300 * f, (dist + sq) * 0.004);
        };
        for (int k = 0; k < NF; k++) {
            const double f0 = (double)k / NF, f1 = (double)(k + 1) / NF;
            double x0, y0, z0, th0, x1, y1, z1, th1;
            at(f0, x0, y0, z0, th0); at(f1, x1, y1, z1, th1);
            const double sh0 = base - 2 - 12 * f0, sh1 = base - 2 - 12 * f1, w0 = 1.15 - 0.6 * f0, w1 = 1.15 - 0.6 * f1;
            rp.edgeScale = 0.35 / std::max(1.0, th0 * 0.3);
            for (int ribbon = 0; ribbon < 2; ribbon++)
                for (int side = -1; side <= 1; side += 2) {
                    const Vec3 m0 = toView(x0, y0, z0), m1 = toView(x1, y1, z1);
                    const Vec3 e0 = ribbon == 0 ? toView(x0, y0 + side * th0, z0) : toView(x0 + px * side * th0, y0, z0 + pz * side * th0);
                    const Vec3 e1 = ribbon == 0 ? toView(x1, y1 + side * th1, z1) : toView(x1 + px * side * th1, y1, z1 + pz * side * th1);
                    if (m0.z < NEAR_Z || m1.z < NEAR_Z || e0.z < NEAR_Z || e1.z < NEAR_Z) continue;
                    RVert q[4];
                    q[0].x = m0.x; q[0].y = m0.y; q[0].z = m0.z; q[0].w = w0; q[0].shade = sh0; q[0].u = 0; q[0].v = Lf * f0 * 0.35 - t * 3;
                    q[1].x = e0.x; q[1].y = e0.y; q[1].z = e0.z; q[1].w = 0.15; q[1].shade = sh0 - 5; q[1].u = side * th0; q[1].v = Lf * f0 * 0.35 - t * 3;
                    q[2].x = e1.x; q[2].y = e1.y; q[2].z = e1.z; q[2].w = 0.15; q[2].shade = sh1 - 5; q[2].u = side * th1; q[2].v = Lf * f1 * 0.35 - t * 3;
                    q[3].x = m1.x; q[3].y = m1.y; q[3].z = m1.z; q[3].w = w1; q[3].shade = sh1; q[3].u = 0; q[3].v = Lf * f1 * 0.35 - t * 3;
                    rasterPolygon(fb, q, 4, rp, proj);
                }
        }
    }
}

// P-05: the dry ice world's spring (Kieffer 2006): while the hemisphere's sun climbs out of its winter (its season between -0.75 and 0.35 and
// rising) and the sun is up, gas under the seasonal frost bursts out at the spiders' centres (`springJets`) in jets of dark dust 15-60 m high,
// a minute or two each, a third of their cycle (the fans on the frost are what they lay)
bool SurfaceView::springJetsNow(double t) const {
    if (env.sun.dirLocal.y < 0.05 || !site.sys || site.body < 0) return false;
    const Body& b = site.sys->bodies[site.body];
    const double dt = std::max(3600.0, 0.002 * std::fabs(b.orbitPeriod));
    const double north = site.unitAt(camPos.x, camPos.z).z >= 0 ? 1 : -1;
    const double s0 = north * site.sys->seasonOf(site.body, t), s1 = north * site.sys->seasonOf(site.body, t + dt);
    return s1 > s0 && s0 > -0.75 && s0 < 0.35;
}
double SurfaceView::springJetStrength(uint64_t id, double t) { return jetStrength(id, t, false, 50 + 90 * hash01(mix64(id + 3)), 0.25 + 0.2 * hash01(mix64(id + 4))); }
void SurfaceView::drawSpringJets(Framebuffer& fb, double t) {
    if (!springJetsNow(t)) return;
    std::vector<GeyserVent> vents;
    springJets(site.gen, site.unitAt(camPos.x, camPos.z), vents);
    for (const GeyserVent& gv : vents) {
        double vx, vz; site.localAt(gv.unit, vx, vz);
        const double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
        if (dist > 1500) continue;
        JetSpec j; j.x = vx; j.z = vz; j.h = gv.id; j.dist = dist; j.bank = 10; j.lava = false; j.shadeAdd = -24;   // the fans' dark dust (the accent's bank)
        j.heightM = 15 + 45 * hash01(mix64(gv.id + 7)); j.widthM = 2 + 3 * hash01(mix64(gv.id + 8));
        drawJet(fb, t, j, springJetStrength(gv.id, t));
    }
}

// P-06: the tiger stripes' curtains (Enceladus's: Porco et al. 2006, Spitale et al. 2015). Along each stripe within 300 km a sheet of fine ice
// rising 25-60 km, densest low and thinning upward, its top wavering along the stripe and with time, bright in the sun (a glow over the dark sky:
// the brighter of it and what is behind, so it never darkens the ground), drawn in segments of 4 km; and within 5 km of the explorer its jets one
// by one (`curtainVents`: R-307's columns, 0.8-2.5 km high, their fallout drifting down for minutes in the moon's weak gravity)
void SurfaceView::drawCurtains(Framebuffer& fb, double t) {
    const Vec3 here = site.unitAt(camPos.x, camPos.z);
    if (here.z > -0.3) return;
    const TigerStripes T = tigerStripesOf(site.gen);
    const double Rk = site.gen.R, sunUp = smoothstep(-0.05, 0.15, env.sun.dirLocal.y), base = 18 + 18 * sunUp;
    RasterParams rp;
    rp.bank = 8; rp.grain = &grainFine; rp.grainScale = 0.9; rp.grainWeight = 1.0;
    rp.cutout = true; rp.edge = &grainEdge; rp.edgeAmp = 0.75; rp.blend = BLEND_MAX; rp.zwrite = false;
    for (int k = 0; k < 4; k++) {
        const double half = 0.5 * T.len[k];
        double xp = 0, zp = 0, gp = 0, Hp = 0; bool have = false;
        for (double a = -half; a <= half + 1e-6; a += 4.0) {
            double x, z; site.localAt(stripePoint(T, Rk, k, a), x, z);
            const double d = std::sqrt((x - camPos.x) * (x - camPos.x) + (z - camPos.z) * (z - camPos.z));
            if (d > 300000) { have = false; continue; }
            double water = 0;
            const double gy = d < 40000 ? heightNear(x, z, d, water) : (double)site.sampleAt(x, z, 2048).h;
            const double H = 1000.0 * (25.0 + 35.0 * (0.5 + 0.5 * std::sin(a / 23.0 + 1.7 * k))) * (0.85 + 0.15 * std::sin(t * 0.05 + a / 7.0 + k));
            if (have && d > 6000 && std::sqrt((xp - camPos.x) * (xp - camPos.x) + (zp - camPos.z) * (zp - camPos.z)) > 6000) {   // the near part is its jets
                rp.edgeScale = 1.0 / (0.012 * d); rp.grainScale = rp.edgeScale;   // the veil's grain three pixels or so across at any distance (u and v in metres)
                const int NS = 4;
                for (int j = 0; j < NS; j++) {
                    const double f0 = (double)j / NS, f1 = (double)(j + 1) / NS;
                    const Vec3 a0 = toView(xp, gp + Hp * f0, zp), b0 = toView(x, gy + H * f0, z), b1 = toView(x, gy + H * f1, z), a1 = toView(xp, gp + Hp * f1, zp);
                    if (a0.z < NEAR_Z || b0.z < NEAR_Z || b1.z < NEAR_Z || a1.z < NEAR_Z) continue;
                    const double w0 = 0.8 - 0.6 * f0, w1 = 0.8 - 0.6 * f1, s0 = base - 12 * f0, s1 = base - 12 * f1;   // a veil: thin, thinner upward
                    RVert q[4];
                    q[0].x = a0.x; q[0].y = a0.y; q[0].z = a0.z; q[0].w = w0; q[0].shade = s0; q[0].u = a * 1000.0; q[0].v = f0 * Hp - t * 300.0;
                    q[1].x = b0.x; q[1].y = b0.y; q[1].z = b0.z; q[1].w = w0; q[1].shade = s0; q[1].u = (a + 4.0) * 1000.0; q[1].v = f0 * H - t * 300.0;
                    q[2].x = b1.x; q[2].y = b1.y; q[2].z = b1.z; q[2].w = w1; q[2].shade = s1; q[2].u = (a + 4.0) * 1000.0; q[2].v = f1 * H - t * 300.0;
                    q[3].x = a1.x; q[3].y = a1.y; q[3].z = a1.z; q[3].w = w1; q[3].shade = s1; q[3].u = a * 1000.0; q[3].v = f1 * Hp - t * 300.0;
                    rasterPolygon(fb, q, 4, rp, proj);
                }
            }
            xp = x; zp = z; gp = gy; Hp = H; have = true;
        }
    }
    std::vector<GeyserVent> vents;
    curtainVents(site.gen, here, vents);
    for (const GeyserVent& gv : vents) {
        double vx, vz; site.localAt(gv.unit, vx, vz);
        const double dist = std::sqrt((vx - camPos.x) * (vx - camPos.x) + (vz - camPos.z) * (vz - camPos.z));
        if (dist > 5000) continue;
        JetSpec j; j.x = vx; j.z = vz; j.h = gv.id; j.dist = dist; j.bank = 8; j.lava = false;
        j.heightM = 800 + 1700 * gv.radiusKm; j.widthM = 15 + 25 * gv.radiusKm;
        drawJet(fb, t, j, jetStrength(gv.id, t, true, 1, 1));
    }
}

// D-04 (TR_LIGHTS_LEFT_ON, D-05's late peoples): the lamps a dead people left burning along its streets, one every 30 m on each side in turn
// within a kilometre and a half (its towns' streets: T-02's marks), each a point of the lamps' light (bank 15) on a 4 m post, more of them dark
// where the years left less of the street; the lit windows are the forms' (`drawFormItems`)
void SurfaceView::drawStreetLamps(Framebuffer& fb) {
    for (const GroundMark& m : site.marks) {
        if (m.kind != 0) continue;
        const double dx = m.bx - m.ax, dz = m.bz - m.az, L = std::sqrt(dx * dx + dz * dz);
        if (L < 1) continue;
        const double mx = 0.5 * (m.ax + m.bx) - camPos.x, mz = 0.5 * (m.az + m.bz) - camPos.z;
        if (std::sqrt(mx * mx + mz * mz) > 1500 + 0.5 * L) continue;
        const double ux = dx / L, uz = dz / L;
        const int n = (int)(L / 30.0);
        for (int k = 0; k <= n; k++) {
            const uint64_t h = mix64((uint64_t)m.id * 977 + (uint64_t)k);
            if (hash01(h) > 0.25 + 0.7 * m.left) continue;   // a broken lamp
            const double side = (k & 1) ? 1.0 : -1.0, along = std::min(L, 15.0 + 30.0 * k);
            const double x = m.ax + ux * along - uz * side * (m.half + 1.0), z = m.az + uz * along + ux * side * (m.half + 1.0);
            const double d = std::sqrt((x - camPos.x) * (x - camPos.x) + (z - camPos.z) * (z - camPos.z));
            if (d > 1500) continue;
            const Vec3 v = toView(x, site.groundHeight(x, z) + 4.0, z);
            if (v.z < NEAR_Z) continue;
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 58 + 4 * hash01(mix64(h + 1));
            rasterPoint3(fb, p, 15, proj, true, d < 60 ? 3 : (d < 180 ? 2 : 1));
        }
    }
}

void SurfaceView::drawWeather(Framebuffer& fb, double t) {
    const uint32_t events = KINDS[site.gen.type].events;   // P-00: the kind's vents and jets
    if (site.gen.hasTrait(TR_LIGHTS_LEFT_ON)) drawStreetLamps(fb);
    if (events & EV_COMET_JETS) drawCometJets(fb, t);
    if (events & EV_PLUMES) drawPlumes(fb, t);
    if (events & EV_SPRING_JETS) drawSpringJets(fb, t);
    if (events & EV_CURTAINS) drawCurtains(fb, t);
    if ((events & (EV_CRACK_VENTS | EV_FUMAROLES | EV_STEAM_BURSTS | EV_SPRINGS)) || site.gen.hasTrait(TR_GEYSERS) || site.gen.hasTrait(TR_HOT_SPRINGS)) drawGeysers(fb, t);   // R-307; P-01; P-03; Q-02
    if (events & EV_SPRINGS) drawSpores(fb, t);
    // dust devils: a column of dust points spiralling up, leaning with the wind, a skirt at the foot
    for (const DustDevil& d : devils) {
        double dist = std::sqrt((d.x - camPos.x) * (d.x - camPos.x) + (d.z - camPos.z) * (d.z - camPos.z));
        if (dist > 1400) continue;
        double life = smoothstep(0, 15, d.age) * smoothstep(d.life, d.life - 25, d.age);
        double gy = site.groundHeight(d.x, d.z);
        double lean = 0.15 * clampd(env.windKnots / 20.0, 0, 1);
        int n = (int)((dist < 300 ? 420 : 160) * life);
        Rng r((uint64_t)(d.phase * 1000));
        for (int i = 0; i < n; i++) {
            double u = r.uni(), a0 = r.range(0, TAU), spin = r.range(2.0, 4.0);
            double yy = u * u * d.height;   // dense at the foot, thinning up
            double rr = d.radius * (0.3 + 1.0 * u) * (0.75 + 0.25 * std::sin(t * 3 + u * 20)) * r.range(0.5, 1.0);
            double a = a0 + t * spin + u * 6.0;
            Vec3 v = toView(d.x + std::cos(a) * rr + std::sin(env.windDir) * lean * yy, gy + yy, d.z + std::sin(a) * rr + std::cos(env.windDir) * lean * yy);
            if (v.z < NEAR_Z) continue;
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 30 + 14 * env.skyBrightness - 8 * u;
            rasterPoint3(fb, p, 9, proj, true, dist < 250 && u < 0.6 ? 2 : 1);
        }
    }
    // eruptions: a column of glowing points that rises and spreads for eight seconds; R-307: the tectonic fountain
    // (kind 1) throws its lava two hundred metres and more for half a minute, the meteorite strike (kind 2) flashes and then
    // fountains dark ejecta for eight seconds
    for (const Eruption& e : eruptions) {
        double age = t - e.start;
        if (e.kind == 1) {
            if (age < 0 || age > 30) continue;
            Rng er((uint64_t)(e.start * 10));
            double ramp = smoothstep(0, 2.5, age) * (1.0 + 0.35 * std::exp(-age / 3.0)) * smoothstep(30, 22, age);
            JetSpec j; j.x = e.x; j.z = e.z; j.h = (uint64_t)(e.start * 10); j.bank = 2; j.lava = true;
            j.heightM = 250 + 200 * er.uni(); j.widthM = 14 + 12 * er.uni();
            j.dist = std::sqrt((e.x - camPos.x) * (e.x - camPos.x) + (e.z - camPos.z) * (e.z - camPos.z));
            drawJet(fb, t, j, ramp);
            continue;
        }
        if (e.kind == 2) {
            if (age < 0 || age > 8) continue;
            double gy = site.groundHeight(e.x, e.z);
            if (age < 0.35) {   // the flash: a white burst that swells and fades
                Vec3 v = toView(e.x, gy + 3, e.z);
                if (v.z > NEAR_Z) { RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 63; rasterPoint3(fb, p, 1, proj, false, (int)(2 + 6 * (1 - age / 0.35))); }
            }
            Rng er((uint64_t)(e.start * 10));
            double gAcc = std::max(0.5, site.gravity);
            int n = (int)(140 * std::min(1.0, age / 0.6));
            for (int i = 0; i < n; i++) {
                double a = er.range(0, TAU), sp = er.range(0.3, 1.0), tilt = er.range(0.3, 1.2), v0 = er.range(25, 70);
                double fa = age - 0.1;
                if (fa < 0) continue;
                double up = v0 * std::cos(tilt) * fa - 0.5 * gAcc * fa * fa;
                if (up < 0) continue;
                double out = v0 * std::sin(tilt) * fa * sp;
                Vec3 v = toView(e.x + std::cos(a) * out, gy + up, e.z + std::sin(a) * out);
                if (v.z < NEAR_Z) continue;
                RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = i % 5 == 0 ? 56 : 18 + 6 * sp;   // dark rock with a few glowing chips
                rasterPoint3(fb, p, 0, proj, true, 1);
            }
            continue;
        }
        if (age < 0 || age > 8) continue;
        Rng er((uint64_t)(e.start * 10));
        int n = (int)(60 * std::min(1.0, age / 2.0));
        for (int i = 0; i < n; i++) {
            double ph = er.uni(), a = er.range(0, TAU), sp = er.range(0.5, 2.0);
            double life = std::fmod(age * 0.8 + ph * 4.0, 4.0);
            double up = 30.0 * life - 3.0 * life * life * (site.gravity / 9.8), out = sp * life * 3.0;
            Vec3 v = toView(e.x + std::cos(a) * out, site.groundHeight(e.x, e.z) + up, e.z + std::sin(a) * out);
            if (v.z < NEAR_Z) continue;
            RVert p; p.x = v.x; p.y = v.y; p.z = v.z; p.shade = 62 - 14 * life;
            rasterPoint3(fb, p, 2, proj, true, life < 2 ? 2 : 1);
        }
    }
    // snow, hail and dust
    if (env.snow > 0.02 || env.hail > 0.02 || env.dust > 0.05) {
        Rng r((uint64_t)(t * 20.0));
        int n = (int)(env.snow * 220 + env.hail * 120 + env.dust * 260);
        double wx = std::sin(env.windDir) * env.windKnots * 0.03, wz = std::cos(env.windDir) * env.windKnots * 0.03;
        for (int i = 0; i < n; i++) {
            double x = camPos.x + r.range(-6, 6), z = camPos.z + r.range(-6, 6), y = camPos.y + r.range(-3, 4);
            // flakes fall slowly and swing; dust streams sideways
            bool dustP = env.dust > 0.05 && r.uni() < env.dust * 260.0 / std::max(1, n);
            double sway = dustP ? 0 : std::sin(t * 2 + x) * 0.3;
            Vec3 va = toView(x + sway, y - (dustP ? 0 : std::fmod(t * 0.9 + i * 0.37, 7.0)), z);
            Vec3 vb = toView(x + sway + wx * (dustP ? 3.0 : 0.3), y - (dustP ? 0.05 : std::fmod(t * 0.9 + i * 0.37, 7.0) + 0.12), z + wz * (dustP ? 3.0 : 0.3));
            if (va.z < NEAR_Z && vb.z < NEAR_Z) continue;
            RVert a, b;
            a.x = va.x; a.y = va.y; a.z = va.z; a.shade = dustP ? 30 + 10 * env.skyBrightness : 44 + 12 * env.skyBrightness;
            b.x = vb.x; b.y = vb.y; b.z = vb.z; b.shade = a.shade;
            if (dustP) rasterLine3(fb, a, b, 0, proj, true, 1);
            else rasterPoint3(fb, a, site.hasWater ? 8 : 1, proj, true, env.hail > 0.02 ? 1 : (r.chance(0.3) ? 2 : 1));
        }
    }
    if (env.rain <= 0.02) return;
    Rng r((uint64_t)(t * 60.0));
    int n = (int)(env.rain * 300);
    // drops live in the world around the camera (KI-017): depth-tested against the terrain, so
    // they never paint over the ground when looking down, and the wind drifts them
    double wx = std::sin(env.windDir) * env.windKnots * 0.03, wz = std::cos(env.windDir) * env.windKnots * 0.03;
    for (int i = 0; i < n; i++) {
        double x = camPos.x + r.range(-6, 6), z = camPos.z + r.range(-6, 6), y = camPos.y + r.range(-3, 4);
        Vec3 va = toView(x, y, z), vb = toView(x + wx * 0.2, y - 0.5, z + wz * 0.2);
        if (va.z < NEAR_Z && vb.z < NEAR_Z) continue;
        RVert a, b;
        a.x = va.x; a.y = va.y; a.z = va.z; a.shade = 38 + 10 * env.skyBrightness;
        b.x = vb.x; b.y = vb.y; b.z = vb.z; b.shade = a.shade;
        rasterLine3(fb, a, b, 1, proj, true, 1);
    }
    // lightning
    if (env.rain > 0.5) {
        if (lightningFlash > 0) lightningFlash *= 0.6;
        if (r.chance(0.004 * env.rain)) lightningFlash = 1.0;
        if (lightningFlash < 0.02) lightningFlash = 0;
    } else lightningFlash = 0;
}
