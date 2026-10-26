// E-02: `fingerprint modules`: what nothing else pins (the regress pins five systems, ten maps, three sites and six frames; the unit the histories,
// the record catalogues, the chart marks and the texts), for a fixed sample, as keyed lines (fingerprint.cpp says how they are compared):
//   * the peoples' worlds: the age test worlds, the ages' scene worlds and the scorched worlds (`fingerprint peoples`' fixed list) and the two
//     test drifters (D-05). Each world's plan; of each people its ways and its roads; its places (the capital, a city, another place of the plan,
//     one left early), the countryside round its capital (a town, a village, a hamlet, a monument) and the block of cells round that village,
//     each laid out as the view lays a cell out (`SurfaceView::ruinCell`): its layout, its town's plan, its pieces at three levels, their
//     colliders, meshes and profiles, its rooms, its decay, its graves, its records and the instruments beside them; the world's works and the
//     land's round the first places, laid out as the view does (`workCell`); a town and a village of each age in the people's way (`ruins
//     ages`); its music and pieces, its instruments (and a make of every plan), its tongue and voice, its genome and culture;
//   * a land site on the first world of each drained kind in the scan region that had no people (two felisian), a forest where the kind grows
//     one, as a landing makes it (`SurfaceView::init`): the trees, logs and boulders of a kilometre square, the colliders, the ground's vertices,
//     and the drainage there;
//   * the almanac of home and of every 40th system of the scan region: a place on every landable body, the ship parked at every planet (of home
//     and every third of them), and every body's showers, zodiacal dust and fronts;
//   * the tables: the materials' decay, the kinds' ages and names, the streets' widths, the edits' fading, the drainage's sink.
// A value is a count or the FNV-1a hash (fpHash's) of the bytes of what a module returned, field by field: a number by its bytes, a string by its
// bytes and its size, a list by its size and then each element; never a struct's memory (its padding) or a pointer. `every` takes every n-th of
// each list of the sample; `full` writes under each line, indented, what it is about as text to read (the names, the counts, the lines).
#include "harness/fingerprint.h"
#include "galaxy/starfield.h"
#include "galaxy/drifters.h"
#include "galaxy/system.h"
#include "galaxy/planetmap.h"
#include "galaxy/kinds.h"
#include "galaxy/drainage.h"
#include "peoples/peoples.h"
#include "peoples/history.h"
#include "peoples/plan.h"
#include "peoples/ruins.h"
#include "peoples/buildings.h"
#include "peoples/towns.h"
#include "peoples/works.h"
#include "peoples/roads.h"
#include "peoples/graves.h"
#include "peoples/records.h"
#include "peoples/forms.h"
#include "peoples/genome.h"
#include "peoples/music.h"
#include "peoples/instruments.h"
#include "peoples/tongue.h"
#include "peoples/voice.h"
#include "galaxy/almanac.h"
#include "galaxy/nights.h"
#include "galaxy/fronts.h"
#include "surface/surface_view.h"
#include "core/rng.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

// a column: its bytes through fpHash's FNV-1a as they come (the hash fpHash gives over them all, without holding a capital's meshes)
struct Bytes {
    uint64_t h = 1469598103934665603ULL;
    void raw(const void* p, size_t n) {
        const unsigned char* c = static_cast<const unsigned char*>(p);
        for (size_t k = 0; k < n; k++) { h ^= c[k]; h *= 1099511628211ULL; }
    }
    void d(double v) { raw(&v, sizeof v); }
    void f(float v) { raw(&v, sizeof v); }
    void i(int64_t v) { raw(&v, sizeof v); }
    void u(uint64_t v) { raw(&v, sizeof v); }
    void bo(bool v) { i(v ? 1 : 0); }
    void n(size_t count) { i((int64_t)count); }
    void s(const std::string& t) { raw(t.data(), t.size()); i((int64_t)t.size()); }
    void cs(const char* t) { s(t ? std::string(t) : std::string()); }
    void v3(const Vec3& v) { d(v.x); d(v.y); d(v.z); }
    void rgb(const RGB& c) { f(c.r); f(c.g); f(c.b); }
};

// the file: a keyed line, and under it in `full` its text, indented and without a field's '='
struct Out {
    FILE* f = nullptr;
    bool full = false;
    long lines = 0;
    void put(const std::string& line, const std::string& text = std::string()) {
        fputs(line.c_str(), f); fputc('\n', f); lines++;
        if (!full || text.empty()) return;
        std::string t = text;
        for (char& ch : t) if (ch == '=') ch = ':';
        for (size_t a = 0; a < t.size();) {
            size_t e = t.find('\n', a);
            if (e == std::string::npos) e = t.size();
            fputs("  ", f); fwrite(t.data() + a, 1, e - a, f); fputc('\n', f);
            a = e + 1;
        }
    }
};
// `<module> <key...> <field>=<value> ...`: the key's words, then the fields (a count, a word, a column's hash)
struct Line {
    std::string s;
    Line(const char* module, const std::string& key) : s(module) { s += ' '; s += key; }
    Line& kn(long long v) { fpPut(s, " %lld", v); return *this; }
    Line& kw(const char* w) { s += ' '; s += w; return *this; }
    Line& n(const char* name, long long v) { fpPut(s, " %s=%lld", name, v); return *this; }
    Line& w(const char* name, const char* word) { fpPut(s, " %s=%s", name, word); return *this; }
    Line& h(const char* name, const Bytes& b) { fpPut(s, " %s=%016llx", name, (unsigned long long)b.h); return *this; }
};

struct Tally { int worlds = 0, peoples = 0, settlements = 0, works = 0, roads = 0, pieces = 0, instruments = 0, sheets = 0, sites = 0, places = 0; long events = 0, forms = 0, discs = 0; };

// ---- the bytes of what the modules return ----------------------------------------------------------------------------------------------------

void vecF(Bytes& h, const std::vector<float>& v) { h.n(v.size()); for (float x : v) h.f(x); }
void vecI(Bytes& h, const std::vector<int>& v) { h.n(v.size()); for (int x : v) h.i(x); }
void vecU8(Bytes& h, const std::vector<uint8_t>& v) { h.n(v.size()); if (!v.empty()) h.raw(v.data(), v.size()); }
void vecS(Bytes& h, const std::vector<std::string>& v) { h.n(v.size()); for (const std::string& x : v) h.s(x); }

void formBytes(Bytes& h, const Form& e) {
    h.i(e.kind); h.i(e.sub); h.d(e.x); h.d(e.z); h.d(e.heading); h.d(e.hx); h.d(e.hz); h.d(e.y0); h.d(e.y1);
    for (double p : e.p) h.d(p);
    h.i(e.sides); h.i(e.part); h.bo(e.glyphs); h.d(e.broken); h.d(e.tilt); h.u(e.seed); h.i(e.slot); h.i(e.pattern); h.i(e.building); h.i(e.material);
    h.bo(e.absolute); h.f(e.tone);
}
void formsBytes(Bytes& h, const std::vector<Form>& el) { h.n(el.size()); for (const Form& e : el) formBytes(h, e); }
// what the explorer meets: each piece's discs (B-405's spans)
long collidersBytes(Bytes& h, const std::vector<Form>& el) {
    std::vector<FormDisc> q;
    long n = 0;
    for (const Form& e : el) {
        q.clear(); formColliders(e, q);
        h.n(q.size());
        for (const FormDisc& c : q) { h.d(c.x); h.d(c.z); h.d(c.r); h.d(c.y0); h.d(c.y1); }
        n += (long)q.size();
    }
    return n;
}
// its silhouette far off and what it stands over
void profileBytes(Bytes& h, const std::vector<Form>& el) {
    static const double T[4] = {0.0, 0.3, 0.7, 1.0};
    for (const Form& e : el) {
        h.d(formTop(e)); h.d(formReach(e)); h.bo(formWalkable(e)); h.d(formHalfWidth(e, 0.0)); h.d(formHalfWidth(e, 1.1));
        for (double t : T) h.d(formProfile(e, t));
    }
}
void meshBytes(Bytes& h, const FormMesh& m) {
    h.n(m.v.size());
    for (const FormMesh::V& v : m.v) { h.v3(v.p); h.v3(v.n); h.d(v.u); h.d(v.v); }
    h.n(m.polys.size());
    for (const FormMesh::Poly& p : m.polys) { h.i(p.first); h.i(p.n); h.i(p.flags); }
    h.n(m.lines.size());
    for (const FormMesh::Line& l : m.lines) { h.v3(l.a); h.v3(l.b); h.d(l.width); h.bo(l.main); }
}
// the drawn surface at a level of detail on flat ground (`walls`' foot and skirt), and the walkable pieces' support at their middle
long meshesBytes(Bytes& h, const std::vector<Form>& el, int lod) {
    FormMesh m;
    long verts = 0;
    for (const Form& e : el) {
        m.clear();
        formMesh(e, lod, Vec3(e.x, 0, e.z), e.y0 < 0.01 && !e.absolute ? 1.0 : 0.0, m, 1e9);
        meshBytes(h, m);
        if (formWalkable(e)) h.d(formSupportAt(m, e.x, e.z));
        verts += (long)m.v.size();
    }
    return verts;
}
void windowBytes(Bytes& h, const WindowSpec& W) {
    h.d(W.bay); h.d(W.storey); h.d(W.w); h.d(W.h); h.d(W.sill); h.i(W.shape); h.d(W.dark); h.bo(W.lit); h.d(W.fromY); h.i(W.group); h.d(W.scatter);
    h.d(W.frame); h.d(W.frameShade); h.d(W.shutter); h.d(W.shutterShade); h.d(W.stripe); h.d(W.stripeShade); h.d(W.dado); h.d(W.dadoShade);
}
void paletteBytes(Bytes& h, const FormPalette& P) {
    for (int k = 0; k < FORM_SLOTS; k++) { h.rgb(P.colour[k]); h.bo(P.own[k]); h.f(P.tint[k]); }
    for (int k = 0; k < 4; k++) windowBytes(h, P.windows[k]);
    h.i(P.nWindows); h.bo(P.glow);
}
void genomeBytes(Bytes& h, const Genome& G) { for (int k = 0; k < GV_COUNT; k++) h.d(G.v[k]); }
void cultureBytes(Bytes& h, const Culture& c) {
    h.i(c.style); h.i(c.family); h.d(c.tall); h.d(c.decay); h.bo(c.stoneRoofs); h.i(c.plan); h.bo(c.towers); h.d(c.buried); h.d(c.ageYears); h.d(c.wet);
    h.i(c.age); h.d(c.weatherYears); h.d(c.roadHalf); h.bo(c.paved); h.d(c.roadWear); h.i(c.people); paletteBytes(h, c.palette); genomeBytes(h, c.genome);
    h.i(c.technique); h.i(c.roof); h.i(c.temple); h.i(c.hut); h.i(c.towerForm); h.i(c.great); h.d(c.life); h.u(c.seed);
}
void buildingBytes(Bytes& h, const Building& b) {
    h.i(b.kind); h.d(b.x); h.d(b.z); h.d(b.heading); h.d(b.hw); h.d(b.hd); h.d(b.hWall); h.d(b.thick); h.d(b.luck); h.i(b.door); h.i(b.age); h.i(b.style);
    h.i(b.form); h.i(b.sides); h.i(b.found); h.bo(b.centred); h.u(b.seed); h.i(b.district); h.d(b.before); h.d(b.base); h.d(b.yard); h.bo(b.joined);
}
void siteBytes(Bytes& h, const SiteRead& s) {
    h.d(s.slope); h.d(s.downhill); h.d(s.shoreDist); h.d(s.shoreDir); h.bo(s.shoreDry); h.d(s.relief); h.d(s.ridgeDir); h.d(s.wind); h.bo(s.desert);
    h.i(s.drift); h.bo(s.shoreIce);
}
void specBytes(Bytes& h, const RuinSpec& r) {
    h.d(r.size); h.d(r.lat); h.d(r.lon); h.d(r.heading); h.u(r.seed); h.i(r.people); h.i(r.sclass); h.i(r.plan); h.bo(r.walled); h.bo(r.harbour);
    h.bo(r.terraced); h.bo(r.cistern); h.bo(r.windwall); h.bo(r.cairns); h.i(r.harbourKind); h.bo(r.reservoir); h.i(r.rank); h.i(r.wanted); h.i(r.buildAge);
    h.d(r.before); h.i(r.founded); h.i(r.work); h.s(featureList(r));
}
// the town's plan (T-02): its streets, squares, fields and layers, how far its streets are one web, the street each building's footprint crosses
void townBytes(Bytes& h, const RuinSpec& r) {
    h.n(r.streets.size());
    for (const Street& s : r.streets) { h.d(s.ax); h.d(s.az); h.d(s.bx); h.d(s.bz); h.d(s.half); h.i(s.rank); h.i(s.age); h.bo(s.paved); }
    h.n(r.plazas.size());
    for (const Plaza& p : r.plazas) { h.d(p.x); h.d(p.z); h.d(p.r); h.i(p.age); }
    h.n(r.fields.size());
    for (const FieldPatch& f : r.fields) { h.d(f.x); h.d(f.z); h.d(f.hw); h.d(f.hd); h.d(f.heading); h.i(f.kind); h.i(f.age); }
    h.n(r.layers.size());
    for (const TownLayer& l : r.layers) { h.i(l.age); h.d(l.r0); h.d(l.r1); }
    if (!r.streets.empty()) {
        h.d(streetsJoined(r.streets));
        for (const Building& b : r.buildings) h.i(buildingStreet(r, b, 0.5));
    }
}
// each building's room (its floor, its doorway, its spots) and its doorway's middle
void roomsBytes(Bytes& h, const RuinSpec& r) {
    for (const Building& b : r.buildings) {
        BuildingRoom R;
        const bool has = buildingRoom(b, R);
        h.bo(has);
        if (has) {
            h.bo(R.round); h.bo(R.outside); h.bo(R.door); h.d(R.cx); h.d(R.cz); h.d(R.hw); h.d(R.hd); h.d(R.r); h.d(R.doorX); h.d(R.doorZ); h.d(R.doorAx);
            h.d(R.doorAz); h.d(R.doorL);
            h.n(R.spots.size());
            for (const std::pair<double, double>& s : R.spots) { h.d(s.first); h.d(s.second); }
        }
        double dx = 0, dz = 0;
        const bool door = buildingDoor(b, dx, dz);
        h.bo(door);
        if (door) { h.d(dx); h.d(dz); }
    }
}
// each building's share gone of every material, in its people's years and its world's wet
void decayBytes(Bytes& h, const RuinSpec& r, const Culture& c) {
    for (const Building& b : r.buildings) for (int m = 0; m < MT_COUNT; m++) h.d(buildingDecay(b, c, m));
}
void gravesBytes(Bytes& h, const std::vector<Grave>& gr, const Lore& L) {
    h.n(gr.size());
    for (const Grave& x : gr) { h.d(x.x); h.d(x.z); h.d(x.heading); h.d(x.height); h.bo(x.fallen); h.s(x.name); h.i(x.born); h.i(x.died); h.u(x.id); h.s(graveLine(L, x)); }
}
// the records lying there and their objects (each from its own seed, as `SurfaceView::recordObjects` makes them)
void recordsBytes(Bytes& h, const std::vector<ShardSite>& ss, uint64_t seed) {
    std::vector<Form> one;
    h.n(ss.size());
    for (const ShardSite& s : ss) {
        h.i(s.index); h.i(s.building); h.i(s.place); h.bo(s.outside); h.d(s.x); h.d(s.z); h.d(s.heading); h.i(s.medium); h.i(s.archive); h.bo(s.last);
        h.d(s.wallD); h.d(s.wallTop); h.d(s.wallTilt);
        one.clear(); recordForms(s, mix64(seed ^ ((uint64_t)(s.index + 1) * 0x9E3779B97F4A7C15ULL)), one);
        formsBytes(h, one);
    }
}
// the instruments beside the music records: which each is (one of each in a settlement) and its object (`SurfaceView::instrumentObjects`)
void instrumentSitesBytes(Bytes& h, const std::vector<InstrumentSite>& is, const PeopleInstruments& PI, const Tradition& T) {
    std::vector<int> ids;
    std::vector<Form> one;
    h.n(is.size());
    for (const InstrumentSite& s : is) {
        h.i(s.record); h.i(s.people); h.i(s.age); h.d(s.pick); h.i(s.building); h.d(s.x); h.d(s.z); h.d(s.heading);
        const int id = instrumentIdOf(PI, s.age, s.pick);
        const bool again = std::find(ids.begin(), ids.end(), id) != ids.end();
        const Instrument* I = id >= 0 && !again ? instrumentById(PI, id) : nullptr;
        ids.push_back(I ? id : -1);
        h.i(I ? id : -1);
        if (I) { one.clear(); instrumentForms(*I, T, s.x, s.z, s.heading, one); formsBytes(h, one); }
    }
}
void workBytes(Bytes& h, const Work& w) {
    h.i(w.kind); h.i(w.age); h.i(w.people); h.i(w.style); h.d(w.lat); h.d(w.lon); h.v3(w.unit); h.d(w.heading); h.d(w.radiusM); h.u(w.seed); h.u(w.id);
    h.i(w.place); h.i(w.planRank); h.i(w.event); h.i(w.year); h.d(w.before); h.d(w.ground); h.d(w.floor); h.d(w.crest); h.d(w.level); h.d(w.length);
    h.d(w.width); h.d(w.height); h.d(w.axis); h.i(w.war); h.v3(w.sideA); h.v3(w.sideB); h.bo(w.heard); h.bo(w.land); h.u(w.group);
}
void editBytes(Bytes& h, const GroundEdit& e) {
    h.i(e.kind); h.d(e.x); h.d(e.z); h.d(e.heading); h.d(e.hw); h.d(e.hd); h.d(e.r); h.d(e.level); h.d(e.depth); h.d(e.step); h.d(e.width); h.d(e.width2);
    h.d(e.bank); h.i(e.tone); h.bo(e.fades); vecF(h, e.pts); h.d(e.gx0); h.d(e.gz0); h.d(e.cell); h.i(e.nx); h.i(e.nz); vecU8(h, e.mask);
}
void gridBytes(Bytes& h, const PlanGrid& G) {
    h.i(G.rows); h.d(G.dLat); h.d(G.R); vecI(h, G.rowStart); vecI(h, G.rowCols); h.i(G.cells); vecF(h, G.height); vecU8(h, G.land); vecU8(h, G.biome);
    vecU8(h, G.material); vecF(h, G.relief); vecF(h, G.veg); vecF(h, G.tempC); vecF(h, G.fert); vecF(h, G.coastKm); vecF(h, G.river); vecI(h, G.down);
    vecF(h, G.res); vecU8(h, G.side); h.d(G.landKm2);
}
void placeBytes(Bytes& h, const PlanPlace& p) {
    h.i(p.rank); h.i(p.sclass); h.i(p.plan); h.d(p.lat); h.d(p.lon); h.v3(p.unit); h.d(p.radiusM); h.i(p.buildings); h.d(p.population); h.d(p.score);
    h.d(p.remote); h.i(p.gLat); h.i(p.gLon); h.u(p.seed); h.i(p.people); h.i(p.cell); h.bo(p.coast); h.bo(p.river);
}
void edgeBytes(Bytes& h, const PlanEdge& e) { h.i(e.a); h.i(e.b); h.i(e.kind); h.bo(e.trunk); h.d(e.lengthKm); h.u(e.id); }
void nodeBytes(Bytes& h, const RoadNode& n) {
    h.i(n.gLat); h.i(n.gLon); h.d(n.lat); h.d(n.lon); h.d(n.size); h.d(n.heading); h.i(n.sclass); h.i(n.plan); h.u(n.seed); h.i(n.people); h.v3(n.unit);
}
void roadBytes(Bytes& ends, Bytes& pts, Bytes& shape, const Road& r) {
    ends.u(r.id); nodeBytes(ends, r.a); nodeBytes(ends, r.b); ends.i(r.people); ends.i(r.kind); ends.bo(r.trunk);
    pts.n(r.pts.size()); for (const Vec3& p : r.pts) pts.v3(p);
    shape.d(r.lengthM); shape.d(r.straightM); shape.d(r.maxGrade); shape.d(r.halfWidth); shape.i(r.bends);
}
void timbreBytes(Bytes& h, const Timbre& t) {
    h.i(t.family); for (double v : t.ratio) h.d(v); for (double v : t.amp) h.d(v);
    h.d(t.attack); h.d(t.decay); h.d(t.sustain); h.d(t.release); h.d(t.vibRate); h.d(t.vibDepth); h.d(t.breath); h.d(t.gain); h.d(t.bright); h.d(t.detune);
    h.i(t.copies); h.d(t.tract); h.d(t.index); h.d(t.mod); h.d(t.sweep); h.d(t.tone); h.i(t.kind); h.s(t.name); h.i(t.make);
}
void ensembleBytes(Bytes& h, const Ensemble& E) {
    h.i(E.age);
    h.n(E.voices.size()); for (const Timbre& t : E.voices) timbreBytes(h, t);
    vecI(h, E.role);
    h.n(E.kit.size()); for (const Timbre& t : E.kit) timbreBytes(h, t);
    vecI(h, E.kitRole);
    h.d(E.tempo); h.d(E.drift); h.i(E.band); h.i(E.comp);
}
void makeBytes(Bytes& h, const InstrumentMake& m) {
    h.i(m.cls); h.i(m.plan); h.i(m.age); h.i(m.reg); h.i(m.mat); h.i(m.mat2); h.i(m.fit); h.i(m.str); h.i(m.skin); h.i(m.trim); h.i(m.stick); h.i(m.vessel);
    h.i(m.tongue); h.d(m.taper); h.d(m.waist); h.d(m.square); h.d(m.dome); h.i(m.sides); h.i(m.skew); h.i(m.hole); h.i(m.top); h.i(m.deco); h.i(m.motif);
    h.i(m.sym); h.i(m.number); h.i(m.rhythm); h.i(m.light); h.i(m.accent); h.d(m.slender); h.d(m.size); h.d(m.painted); h.i(m.variant); h.u(m.seed);
    h.s(m.name); h.s(m.plural);
}
void partsBytes(Bytes& h, const std::vector<InstrumentPart>& parts) {
    h.n(parts.size());
    for (const InstrumentPart& p : parts) {
        h.i(p.kind); h.i(p.mat); h.i(p.course); h.d(p.u); h.d(p.v); h.d(p.ang); h.d(p.len); h.d(p.r0); h.d(p.r1); h.d(p.w0); h.d(p.w1); h.d(p.taper);
        h.d(p.waist); h.d(p.square); h.d(p.dome); h.i(p.sides); h.i(p.skew); h.i(p.shape); h.bo(p.open); h.bo(p.standing);
    }
}
void pieceBytes(Bytes& head, Bytes& score, const Piece& P) {
    head.i(P.form); head.d(P.bpm); head.i(P.cycles); head.d(P.beats); head.d(P.seconds); head.u(P.seed); head.i(P.age); head.d(P.drift);
    score.n(P.notes.size());
    for (const Note& nt : P.notes) {
        score.d(nt.t); score.d(nt.dur); score.i(nt.voice); score.i(nt.degree); score.d(nt.vel); score.i(nt.slideFrom); score.i(nt.vowel); score.d(nt.bend);
    }
}
void sampleBytes(Bytes& h, const SurfaceSample& s) {
    h.d(s.height); h.i(s.material); h.d(s.albedo); h.d(s.veg); h.d(s.glow); h.d(s.relief); h.d(s.water); h.i(s.biome); h.d(s.landform); h.i(s.waterKind);
    h.d(s.shore); h.d(s.scree); h.d(s.oldSea); h.i(s.landformTrait); h.d(s.waterTint); h.d(s.lifeTint); h.d(s.groundTint); h.d(s.slipFace);
}
void drainBytes(Bytes& h, const DrainInfo& d) {
    h.bo(d.near); h.bo(d.any); h.d(d.dist); h.d(d.halfWidth); h.d(d.depth); h.d(d.level); h.d(d.pres); h.d(d.acc); h.d(d.slope); h.d(d.valley);
    h.d(d.valleyLevel); h.d(d.lake); h.d(d.lakeLevel); h.d(d.wetness);
}

// ---- the peoples' worlds -------------------------------------------------------------------------------------------------------------------

struct WorldRef { int64_t sx, sy, sz; int body; };
const WorldRef PEOPLE_WORLDS[] = {
    {198, 0, 55, 10}, {200, 0, 41, 1}, {189, 0, 45, 2}, {208, 0, 51, 5}, {210, 0, 33, 0}, {202, 0, 26, 2},          // the age test worlds
    {202, 0, 28, 0}, {200, 0, 16, 0},                                                                           // the ages' scene worlds besides
    {222, 0, 75, 3}, {226, 0, 82, 0}, {228, 0, -44, 1}, {122, 0, 123, 14}, {323, 0, 192, 0}, {250, 0, 158, 3},   // the scorched worlds
    {1728, 740, -575, 0}, {177, 950, -1246, 0}};                                                                // the test drifters (D-05)

struct World {
    Star st;
    StarSystem sys;
    int body = -1;
    BodyGen g;
    std::string key;   // "sx sy sz body"
};
bool loadWorld(int64_t sx, int64_t sy, int64_t sz, int body, World& w) {
    w.key.clear();
    fpPut(w.key, "%lld %lld %lld %d", (long long)sx, (long long)sy, (long long)sz, body);
    if (!placeInSector(sx, sy, sz, w.st, true)) return false;
    w.sys.generate(w.st);
    if (body < 0 || body >= (int)w.sys.bodies.size()) return false;
    w.body = body;
    w.g = BodyGen::make(w.sys.bodies[body]);
    return true;
}

// a people as the view keeps it (`SurfaceView::init`): its culture, its lore for the graves, its tradition and its instruments
struct People {
    int p = 0;
    Culture cu;
    Lore L;
    Tradition T;
    PeopleInstruments PI;
};

// a settlement or a work laid out as the view lays its cell out (`SurfaceView::ruinCell`, `workCell`): its pieces at three levels, its graves'
// stones with the first (a work's: the stones alone), its records and (a settlement's) the instruments beside its music records
struct Laid {
    std::vector<Form> el[3];
    std::vector<Grave> graves;
    std::vector<ShardSite> records;
    std::vector<InstrumentSite> instruments;
};
void layOut(const BodyGen& g, const RuinSpec& sp, const People& c, Laid& o) {
    for (int l = 0; l < 3; l++) { o.el[l].clear(); ruinElements(sp, c.cu, l, o.el[l]); }
    o.records.clear(); o.instruments.clear(); o.graves.clear();
    const bool work = sp.work >= 0;
    if (work) recordSitesOf(g, sp, c.cu, o.records);
    else recordSitesOf(g, sp, c.cu, o.records, &o.instruments);
    gravesOf(c.L, sp, c.cu, o.graves);
    graveElements(o.graves, c.cu, 0, o.el[0], work);
}
// a laid-out place's columns
struct PlaceCols {
    Bytes spec, site, layout, town, forms, lod1, lod2, colliders, mesh, profile, rooms, decay, graves, records, instruments;
    long discs = 0, verts = 0;
};
void placeCols(const RuinSpec& sp, const Laid& o, const Culture& cu, const Lore& L, const PeopleInstruments& PI, const Tradition& T, PlaceCols& P) {
    specBytes(P.spec, sp);
    siteBytes(P.site, sp.site);
    P.layout.n(sp.buildings.size());
    for (const Building& b : sp.buildings) buildingBytes(P.layout, b);
    townBytes(P.town, sp);
    formsBytes(P.forms, o.el[0]); formsBytes(P.lod1, o.el[1]); formsBytes(P.lod2, o.el[2]);
    P.discs = collidersBytes(P.colliders, o.el[0]);
    for (int l = 0; l < 3; l++) P.verts += meshesBytes(P.mesh, o.el[l], l);
    profileBytes(P.profile, o.el[0]);
    roomsBytes(P.rooms, sp);
    decayBytes(P.decay, sp, cu);
    gravesBytes(P.graves, o.graves, L);
    recordsBytes(P.records, o.records, sp.seed);
    instrumentSitesBytes(P.instruments, o.instruments, PI, T);
}
Line& placeFields(Line& l, const RuinSpec& sp, const Laid& o, const PlaceCols& P) {
    l.n("class", sp.sclass).n("n", (long long)sp.buildings.size()).n("pieces", (long long)o.el[0].size()).n("discs", P.discs).n("verts", P.verts);
    l.h("spec", P.spec).h("site", P.site).h("layout", P.layout).h("town", P.town).h("forms", P.forms).h("lod1", P.lod1).h("lod2", P.lod2);
    l.h("colliders", P.colliders).h("mesh", P.mesh).h("profile", P.profile).h("rooms", P.rooms).h("decay", P.decay);
    return l;
}
// a place to read: its class, size and features, its buildings by kind, its graves and its records
std::string placeText(const RuinSpec& sp, const Laid& o, const Lore& L) {
    std::string t;
    fpPut(t, "%s r %.1f m at %.4f %.4f, plan %d, %zu buildings, %zu streets, %zu fields%s%s\n", SETTLEMENT_CLASS_NAMES[clampi(sp.sclass, 0, 6)], sp.size,
          sp.lat / DEG, sp.lon / DEG, sp.plan, sp.buildings.size(), sp.streets.size(), sp.fields.size(), featureList(sp).empty() ? "" : ": ", featureList(sp).c_str());
    int kinds[BK_COUNT] = {};
    for (const Building& b : sp.buildings) if (b.kind >= 0 && b.kind < BK_COUNT) kinds[b.kind]++;
    std::string k;
    for (int i = 0; i < BK_COUNT; i++) if (kinds[i]) fpPut(k, "%s%d %s", k.empty() ? "" : ", ", kinds[i], BUILDING_KIND_NAMES[i]);
    if (!k.empty()) t += k + "\n";
    for (size_t i = 0; i < o.graves.size() && i < 6; i++) fpPut(t, "grave %s%s\n", graveLine(L, o.graves[i]).c_str(), o.graves[i].fallen ? " (fallen)" : "");
    if (!o.records.empty() || !o.instruments.empty()) {
        fpPut(t, "%zu records, %zu instruments:", o.records.size(), o.instruments.size());
        for (size_t i = 0; i < o.records.size() && i < 12; i++) fpPut(t, " S%d in %s", o.records[i].index, buildingPlaceName(o.records[i].place));
        t += "\n";
    }
    return t;
}

void settlementLine(Out& O, Tally& T, const World& w, const People* ctx, int np, int p, int gLat, int gl, const char* what, const RuinSpec& sp) {
    const People& c = ctx[sp.people == 1 && np > 1 ? 1 : 0];
    bool ok = false;
    { DrainageOff off; ok = ruinSiteOk(w.g, sp); }   // the view's test (the slope and the sea, without building tiles)
    Laid o; layOut(w.g, sp, c, o);
    PlaceCols P; placeCols(sp, o, c.cu, c.L, c.PI, c.T, P);
    Line l("ruins", w.key);
    l.kn(p).kn(gLat).kn(gl).w("what", what).n("people", sp.people).n("ok", ok);
    placeFields(l, sp, o, P).h("graves", P.graves).h("records", P.records).h("instruments", P.instruments);
    O.put(l.s, O.full ? placeText(sp, o, c.L) : std::string());
    T.settlements++; T.forms += (long)o.el[0].size(); T.discs += P.discs;
}

// the plan of a world, and of each people its places, ways and what the cells read of it
void planLines(Out& O, const World& w, const WorldPlan& P) {
    Bytes head, grid, country, index, divide;
    head.u(P.seed); head.i(P.peoples); head.bo(P.works);
    gridBytes(grid, P.grid);
    vecU8(country, P.countryside);
    {   // the places by their cell and the grid's cells' places and ways (the map's order: its keys sorted)
        std::vector<std::pair<uint64_t, uint32_t>> at(P.placeAt.begin(), P.placeAt.end());
        std::sort(at.begin(), at.end());
        index.n(at.size());
        for (const std::pair<uint64_t, uint32_t>& kv : at) { index.u(kv.first); index.u(kv.second); }
        index.n(P.placesIn.size());
        for (const std::vector<uint32_t>& v : P.placesIn) { index.n(v.size()); for (uint32_t x : v) index.u(x); }
        index.n(P.edgesIn.size());
        for (const std::vector<uint32_t>& v : P.edgesIn) { index.n(v.size()); for (uint32_t x : v) index.u(x); }
    }
    // C-13: the peoples' divide and whose side a lattice of points is on
    divide.v3(peopleDivide(w.g)); divide.bo(peoplesEndedTogether(w.g)); divide.i(systemPeoplesWorld(w.sys)); divide.i(peoplesOf(w.g));
    for (int i = 0; i < 12; i++) for (int j = 0; j < 24; j++) divide.i(peopleAt(w.g, (-82.5 + 15 * i) * DEG, (-172.5 + 15 * j) * DEG));
    std::string t;
    if (O.full) fpPut(t, "%s: %d peoples, %d cells, land %.0f km2%s\n", w.sys.bodies[w.body].name.c_str(), P.peoples, P.grid.cells, P.grid.landKm2, P.works ? ", works in its ways" : "");
    O.put(Line("plan", w.key).n("peoples", P.peoples).n("cells", P.grid.cells).h("head", head).h("grid", grid).h("countryside", country).h("index", index)
          .h("divide", divide).s, t);
}
void peoplePlanLines(Out& O, const World& w, const WorldPlan& P, int p) {
    const PeoplePlan& pp = P.plans[p];
    Bytes head, places, edges, terr, remote, score;
    head.i(pp.people); head.i(pp.age); head.d(pp.areaKm2); head.d(pp.heldShare);
    for (int sc = 0; sc < 7; sc++) head.i(pp.count(sc));
    places.n(pp.places.size()); for (const PlanPlace& q : pp.places) placeBytes(places, q);
    edges.n(pp.edges.size()); for (const PlanEdge& e : pp.edges) edgeBytes(edges, e);
    vecU8(terr, pp.territory); vecF(remote, pp.remote); vecF(score, pp.score);
    std::string t;
    if (O.full) {
        fpPut(t, "people %d (%s): %zu places, %zu ways, %.0f km2 (%.3f of the land)\n", p, AGE_NAMES[clampi(pp.age, 0, AGE_COUNT - 1)], pp.places.size(), pp.edges.size(),
              pp.areaKm2, pp.heldShare);
        for (size_t i = 0; i < pp.places.size() && i < 8; i++) {
            const PlanPlace& q = pp.places[i];
            fpPut(t, "rank %d %s at %.3f %.3f: core %.0f m, %d buildings, score %.3f%s%s\n", q.rank, SETTLEMENT_CLASS_NAMES[clampi(q.sclass, 0, 6)], q.lat / DEG, q.lon / DEG,
                  q.radiusM, q.buildings, q.score, q.coast ? ", on a coast" : "", q.river ? ", on a river" : "");
        }
    }
    O.put(Line("plan", w.key).kn(p).n("places", (long long)pp.places.size()).n("edges", (long long)pp.edges.size()).n("age", pp.age).h("head", head)
          .h("place", places).h("edge", edges).h("territory", terr).h("remote", remote).h("score", score).s, t);
    // the ways: each edge's waypoints (the landing map's, the sea's crossings left out), a maglev's spans in the places' cells, what the cells
    // and the finders ask of the plan round the capital
    Bytes ways, spans, near, country, core;
    long points = 0;
    for (size_t e = 0; e < pp.edges.size(); e++) {
        if (pp.edges[e].kind == NK_SEA) continue;
        PlanEdgeRef r; r.people = p; r.index = (int)e; r.e = &pp.edges[e];
        const std::vector<Vec3>& way = planEdgeWay(w.g, P, r);
        ways.n(way.size());
        for (const Vec3& v : way) ways.v3(v);
        points += (long)way.size();
    }
    std::vector<PlanSpan> sp;
    for (const PlanPlace& q : pp.places) {
        sp.clear(); planSpansIn(w.g, P, q.gLat, q.gLon, sp);
        spans.n(sp.size());
        for (const PlanSpan& s : sp) { spans.v3(s.a); spans.v3(s.b); spans.d(s.h); spans.u(s.seed); spans.i(s.people); }
        const PlanPlace* in = planPlaceIn(P, q.gLat, q.gLon);
        core.i(in ? in->people : -1); core.i(in ? in->rank : -1);
        const PlanPlace* at = planCoreAt(P, q.unit, 0);
        core.i(at ? at->people : -1); core.i(at ? at->rank : -1);
    }
    if (!pp.places.empty()) {
        std::vector<const PlanPlace*> ps; planPlacesNear(P, pp.places[0].unit, 60000, ps);
        near.n(ps.size());
        for (const PlanPlace* q : ps) { near.i(q->people); near.i(q->rank); }
        std::vector<PlanEdgeRef> es; planEdgesNear(P, pp.places[0].unit, 30000, es);
        near.n(es.size());
        for (const PlanEdgeRef& r : es) { near.i(r.people); near.i(r.index); }
    }
    for (int i = 0; i < 15; i++) for (int j = 0; j < 24; j++) country.d(planCountryside(P, (-84.0 + 12 * i) * DEG, (-172.5 + 15 * j) * DEG, p));
    O.put(Line("ways", w.key).kn(p).n("edges", (long long)pp.edges.size()).n("points", points).h("ways", ways).h("spans", spans).h("core", core)
          .h("near", near).h("countryside", country).s);
}

// the places of a people looked at: the plan's (the capital, a city, another place, one its history left early), the countryside round the
// capital (`wallPlacesOf`'s ring: a town, a village, a hamlet, a monument of its own) and the block of cells round that village; each cell once
void settlementLines(Out& O, Tally& T, const World& w, const WorldPlan& P, const People* ctx, int np, int p, std::set<std::pair<int, int>>& seen, int& vLat, int& vLon) {
    const PeoplePlan& pp = P.plans[p];
    if (pp.places.empty()) return;
    struct Pick { int gLat, gLon; const char* what; };
    std::vector<Pick> picks;
    const PlanPlace& cap = pp.places[0];
    picks.push_back({cap.gLat, cap.gLon, "capital"});
    for (const PlanPlace& q : pp.places) if (q.sclass == SC_CITY) { picks.push_back({q.gLat, q.gLon, "city"}); break; }
    for (const PlanPlace& q : pp.places) if (q.rank > 0 && q.sclass != SC_CITY && q.sclass != SC_CAPITAL) { picks.push_back({q.gLat, q.gLon, "place"}); break; }
    if (std::shared_ptr<const History> H = historyOfWorld(w.g, p))
        for (int i = 1; i < (int)pp.places.size() && i < (int)H->places.size(); i++)
            if (H->places[i].fate == HP_EARLY) { picks.push_back({pp.places[i].gLat, pp.places[i].gLon, "early"}); break; }
    static const int WANT[4] = {SC_TOWN, SC_VILLAGE, SC_HAMLET, SC_MONUMENT};
    static const char* const WHAT[4] = {"country-town", "country-village", "country-hamlet", "country-monument"};
    bool got[4] = {false, false, false, false};
    int found = 0;
    vLat = cap.gLat; vLon = cap.gLon;
    for (int ring = 1; ring < 40 && found < 4; ring++)
        for (int dl = -ring; dl <= ring && found < 4; dl++)
            for (int dn = -ring; dn <= ring && found < 4; dn++) {
                if (std::max(std::abs(dl), std::abs(dn)) != ring) continue;
                const int row = cap.gLat + dl, col = ruinCellLonIn(w.g, cap.gLat, cap.gLon, row) + dn;
                RuinSpec q;
                if (!ruinOfCell(w.g, row, col, q, false) || q.rank >= 0 || q.people != p) continue;
                int k = -1;
                for (int j = 0; j < 4; j++) if (q.sclass == WANT[j] && !got[j]) k = j;
                if (k < 0) continue;
                got[k] = true; found++;
                picks.push_back({row, col, WHAT[k]});
                if (WANT[k] == SC_VILLAGE) { vLat = row; vLon = col; }
            }
    for (int dl = -2; dl <= 2; dl++) {
        const int row = vLat + dl, c0 = ruinCellLonIn(w.g, vLat, vLon, row);
        for (int dn = -2; dn <= 2; dn++) picks.push_back({row, c0 + dn, "block"});
    }
    for (const Pick& k : picks) {
        const int nLon = std::max(1, ruinCellsAround(w.g, k.gLat)), gl = ((k.gLon % nLon) + nLon) % nLon;
        if (!seen.insert({k.gLat, gl}).second) continue;
        RuinSpec sp;
        if (!ruinOfCell(w.g, k.gLat, gl, sp, true)) {
            if (std::string(k.what) != "block") O.put(Line("ruins", w.key).kn(p).kn(k.gLat).kn(gl).w("what", k.what).w("none", "settlement").s);
            continue;
        }
        settlementLine(O, T, w, ctx, np, p, k.gLat, gl, k.what, sp);
    }
}

// the roads round a point (the landing map's `roadsNear`), each road's ends, centre line, shape and wear; at the capital the plan's legs walked
// as the site walks them (`SurfaceSite::ensureRoads`)
void roadsLine(Out& O, Tally& T, const World& w, const WorldPlan& P, const People* ctx, int np, int p, const char* centre, const Vec3& at, bool legs) {
    std::vector<Road> rs; roadsNear(w.g, at, 6000, rs, true);
    Bytes ends, pts, shape, wear, legb, widths;
    std::string t;
    for (const Road& r : rs) {
        roadBytes(ends, pts, shape, r);
        const Culture& cu = ctx[r.people == 1 && np > 1 ? 1 : 0].cu;
        for (double a = 0; a < r.lengthM; a += 25) wear.d(roadLeft(r.id, a, cu.roadWear));
        if (O.full) fpPut(t, "%016llx %s %d %d -> %s %d %d: %.0f m along, %.0f straight, %d bends, steepest %.3f, half %.2f, %zu points\n", (unsigned long long)r.id,
                          SETTLEMENT_CLASS_NAMES[clampi(r.a.sclass, 0, 6)], r.a.gLat, r.a.gLon, SETTLEMENT_CLASS_NAMES[clampi(r.b.sclass, 0, 6)], r.b.gLat, r.b.gLon, r.lengthM,
                          r.straightM, r.bends, r.maxGrade, r.halfWidth, r.pts.size());
    }
    int nLegs = 0;
    if (legs) {
        std::vector<PlanEdgeRef> es; planEdgesNear(P, at, 11000, es);
        for (const PlanEdgeRef& r : es) {
            if (r.e->kind == NK_MAGLEV || r.e->kind == NK_SEA) continue;
            const std::vector<Vec3>& way = planEdgeWay(w.g, P, r);
            for (size_t k = 0; k + 1 < way.size(); k++) {
                if (roadDistanceM(w.g, way[k], at) > 8000 && roadDistanceM(w.g, way[k + 1], at) > 8000) continue;
                const uint64_t id = mix64(r.e->id ^ ((uint64_t)(k + 1) * 0x9E3779B97F4A7C15ULL));
                Road leg;
                const bool ok = roadLegWalk(w.g, r.e->kind, way[k], way[k + 1], id, leg);
                legb.u(id); legb.bo(ok);
                if (ok) roadBytes(legb, legb, legb, leg);
                nLegs++;
            }
        }
        for (int k = 0; k < NK_COUNT; k++) for (int tr = 0; tr < 2; tr++) for (int q = 0; q < np; q++) widths.d(netHalfWidth(k, ctx[q].cu, tr != 0));
    }
    O.put(Line("roads", w.key).kn(p).kw(centre).n("roads", (long long)rs.size()).n("legs", nLegs).h("ends", ends).h("points", pts).h("shape", shape)
          .h("wear", wear).h("leg", legb).h("widths", widths).s, t);
    T.roads += (int)rs.size();
}

// a work laid out as the view lays it out (`workCell`): the work, its ground's edits, its pieces, its graves' stones and its records
void workItemLine(Out& O, Tally& T, const World& w, const People* ctx, int np, const Work& wk, bool world, int gLat, int gLon) {
    const People& c = ctx[wk.people == 1 && np > 1 ? 1 : 0];
    Bytes work, edits, line;
    workBytes(work, wk);
    edits.n(wk.edits.size());
    for (const GroundEdit& e : wk.edits) editBytes(edits, e);
    const std::string wl = workLine(wk);
    line.s(wl);
    RuinSpec sp; workSpec(w.g, wk, sp);
    Laid o; layOut(w.g, sp, c, o);
    PlaceCols P; placeCols(sp, o, c.cu, c.L, c.PI, c.T, P);
    Line l("works", w.key);
    l.kn(wk.people).kw(world ? "world" : "land");
    if (world) { char id[24]; snprintf(id, sizeof id, "%016llx", (unsigned long long)wk.id); l.kw(id); }
    else l.kn(gLat).kn(gLon);
    l.n("kind", wk.kind).n("age", wk.age).n("breached", wk.kind == WO_DAM ? (int)damBreached(wk, c.cu) : -1).h("work", work).h("edits", edits).h("line", line);
    placeFields(l, sp, o, P).h("graves", P.graves).h("records", P.records);
    O.put(l.s, O.full ? wl + "\n" + placeText(sp, o, c.L) : std::string());
    T.works++; T.forms += (long)o.el[0].size(); T.discs += P.discs;
}
void worksLines(Out& O, Tally& T, const World& w, const WorldPlan& P, const WorldWorks& WW, const People* ctx, int np) {
    Bytes head, around;
    head.u(WW.seed); head.i(WW.peoples); head.d(WW.Rm);
    for (int k = 0; k < 2; k++) for (int j = 0; j < WO_COUNT; j++) head.i(WW.counts[k][j]);
    for (int p = 0; p < np; p++) {   // what the scanner's query finds round each capital (the world's and the land's within 40 km)
        if (P.plans[p].places.empty()) continue;
        std::vector<std::shared_ptr<const Work>> near; worksAround(w.g, P.plans[p].places[0].unit, 40000, near);
        around.n(near.size());
        for (const std::shared_ptr<const Work>& x : near) around.u(x->id);
    }
    O.put(Line("works", w.key).n("count", (long long)WW.works.size()).h("head", head).h("around", around).s);
    for (const Work& wk : WW.works) workItemLine(O, T, w, ctx, np, wk, true, 0, 0);
    // B-418: the land's, in the cells of the land grid round each people's first three places
    std::set<std::pair<int, int>> seen;
    const double Rm = w.g.R * 1000.0;
    for (int p = 0; p < np; p++)
        for (int k = 0; k < 3 && k < (int)P.plans[p].places.size(); k++) {
            const PlanPlace& q = P.plans[p].places[k];
            for (int dl = -2; dl <= 2; dl++)
                for (int dn = -2; dn <= 2; dn++) {
                    const double la = clampd(q.lat + dl * LAND_CELL_M / Rm, -1.55, 1.55), lo = q.lon + dn * LAND_CELL_M / (Rm * std::max(0.05, std::cos(la)));
                    int gLat = 0, gLon = 0; landCellOf(w.g, la, lo, gLat, gLon);
                    if (!seen.insert({gLat, gLon}).second) continue;
                    std::shared_ptr<const Work> lw = landWorkOf(w.g, gLat, gLon);
                    if (lw) workItemLine(O, T, w, ctx, np, *lw, false, gLat, gLon);
                }
        }
}

// a town and a village of each age (the far future's monument) in the people's way, as `ruins ages` lays them out, at two ages since the end
void sheetLines(Out& O, Tally& T, const World& w, const WorldPlan& P, const People& c) {
    int gLat0 = 0, gLon0 = 0;
    if (!P.plans[c.p].places.empty()) { gLat0 = P.plans[c.p].places[0].gLat + 6; gLon0 = P.plans[c.p].places[0].gLon; }
    else ruinCellOf(w.g, 0.2, 0.7, gLat0, gLon0);
    static const double YEARS[2] = {300, 6000};
    for (int a = 0; a < AGE_COUNT; a++)
        for (int k = 0; k < 2; k++) {
            if (a == AGE_FAR_FUTURE && k > 0) continue;
            const int cls = a == AGE_FAR_FUTURE ? SC_MONUMENT : (k ? SC_VILLAGE : SC_TOWN);
            for (int y = 0; y < 2; y++) {
                const Culture cu = cultureAs(w.g, c.p, a, YEARS[y]);
                RuinSpec sp; settlementOf(w.g, gLat0 + a, gLon0 + 4 * k + y, cu, cls, sp);
                Laid o;
                for (int l = 0; l < 3; l++) ruinElements(sp, cu, l, o.el[l]);
                PlaceCols Pc; placeCols(sp, o, cu, c.L, c.PI, c.T, Pc);
                Bytes culture; cultureBytes(culture, cu);
                Line l("sheet", w.key);
                l.kn(c.p).kn(a).kn(cls).kn((long long)YEARS[y]).h("culture", culture);
                placeFields(l, sp, o, Pc);
                O.put(l.s, O.full ? placeText(sp, o, c.L) : std::string());
                T.sheets++; T.forms += (long)o.el[0].size(); T.discs += Pc.discs;
            }
        }
}

// a people's music: its tradition (the scale, the cycle, its own voices, the ensembles of the ages, the makes) and its lines, then each record
// that is a piece (`music`'s: the record's draws, its piece)
void musicLines(Out& O, Tally& T, const World& w, const People& c) {
    const Tradition& tr = c.T;
    Bytes scale, cycle, voices, ages, makes, lines;
    for (double v : tr.scale) scale.d(v);
    scale.n(tr.scale.size()); scale.d(tr.period); scale.d(tr.base); scale.i(tr.tuning); scale.i(tr.division); scale.i(tr.rest);
    const int nd = (int)tr.scale.size();
    for (int dg = -nd; dg <= 2 * nd; dg++) { scale.d(traditionHz(tr, dg)); scale.d(traditionCents(tr, dg)); }
    vecI(cycle, tr.groups);
    cycle.i(tr.beats); cycle.i(tr.sub); cycle.d(tr.bpm); cycle.d(tr.drift); cycle.u(tr.seed); cycle.d(tr.swing); vecI(cycle, tr.vowels); cycle.d(tr.density);
    cycle.i(tr.refrain); cycle.bo(tr.octaves);
    voices.n(tr.voices.size());
    for (const Timbre& t : tr.voices) timbreBytes(voices, t);
    timbreBytes(voices, tr.drum); voices.bo(tr.hasDrum); voices.bo(tr.drone); voices.i(tr.second); voices.i(tr.ornament); voices.d(tr.ornamentChance);
    for (int a = 0; a < AGE_COUNT; a++) ensembleBytes(ages, tr.ages[a]);
    makes.n(tr.makes.size());
    for (const InstrumentMake& m : tr.makes) makeBytes(makes, m);
    makes.i(tr.machine);
    std::string t;
    {
        std::vector<std::string> L = {traditionLine(tr), cycleLine(tr, tr.bpm), timbresLine(tr, true), timbresLine(tr, false)};
        for (int a = 0; a < AGE_COUNT; a++) L.push_back(ensembleLine(tr.ages[a]));
        vecS(lines, L);
        if (O.full) for (const std::string& s : L) t += s + "\n";
    }
    std::vector<std::pair<std::string, std::string>> pieces;   // written after the tradition's line
    for (int i = 0; i < c.L.records; i++) {
        const Shard d = shardDrawOf(w.g, c.L, i);
        if (!d.music) continue;
        Piece P; pieceOf(tr, d, P);
        Bytes draw, head, score, pline;
        draw.i(d.kind); draw.i(d.tone); draw.d(d.age); draw.i(d.year); draw.i(d.month); draw.i(d.day); draw.bo(d.last); draw.u(d.seed); draw.i(d.recordAge);
        draw.i(d.medium); draw.i(d.dark); draw.s(d.text); draw.s(d.child); draw.bo(d.music); draw.bo(d.chart); vecS(draw, d.names);
        pieceBytes(head, score, P);
        head.bo(pieceHasDrum(P)); head.i(pieceEnsemble(tr, P).age);
        const std::string cl = cycleLine(tr, P), tl = timbresLine(tr, P);
        pline.s(cl); pline.s(tl);
        std::string pt;
        if (O.full) fpPut(pt, "S%d, year %d: %s (%s), %.1f s at %.1f bpm, %d cycles, %zu notes; %s; %s\n", i, d.year, PIECE_FORM_NAMES[clampi(P.form, 0, PF_COUNT - 1)],
                          AGE_NAMES[clampi(P.age, 0, AGE_COUNT - 1)], P.seconds, P.bpm, P.cycles, P.notes.size(), cl.c_str(), tl.c_str());
        pieces.push_back({Line("piece", w.key).kn(c.p).kn(i).n("form", P.form).n("age", P.age).n("notes", (long long)P.notes.size()).h("draw", draw)
                              .h("head", head).h("score", score).h("lines", pline).s, pt});
    }
    O.put(Line("music", w.key).kn(c.p).n("records", c.L.records).n("pieces", (long long)pieces.size()).h("scale", scale).h("cycle", cycle).h("voices", voices)
          .h("ages", ages).h("makes", makes).h("lines", lines).s, t);
    for (const std::pair<std::string, std::string>& q : pieces) O.put(q.first, q.second);
    T.pieces += (int)pieces.size();
}

// a people's instruments of each age (the case's, the ruins'): each as it is made, sounds, is named, lies and is drawn; then the people's
// records' instruments, a make of every plan in its way (`instruments plans`), and what its world gave it to make them of
void instrumentLines(Out& O, Tally& T, const World& w, const People& c) {
    const Tradition& tr = c.T;
    const int nd = std::max(1, (int)tr.scale.size());
    for (int a = 0; a < AGE_COUNT; a++)
        for (const Instrument& I : c.PI.byAge[a]) {
            Bytes inst, make, timbre, name, hz, parts, forms, courses, bounds, disc;
            inst.i(I.people); inst.i(I.age); inst.i(I.voice); inst.i(I.kit); inst.bo(I.pads); inst.bo(I.disc); inst.i(I.slot); inst.i(I.id); inst.i(I.make);
            inst.i(I.reg); inst.u(I.seed); inst.bo(instrumentPlaysDisc(I)); inst.bo(instrumentById(c.PI, I.id) != nullptr);
            const InstrumentMake* M = instrumentMake(tr, I);
            make.bo(M != nullptr);
            if (M) makeBytes(make, *M);
            const int keys = I.pads && I.age >= 0 && I.age < AGE_COUNT ? std::max(1, (int)tr.ages[I.age].kit.size()) : 1;
            for (int k = 0; k < keys; k++) { const Timbre* tb = instrumentTimbre(tr, I, k); timbre.bo(tb != nullptr); if (tb) timbreBytes(timbre, *tb); }
            const std::string nm = instrumentName(tr, I);
            name.s(nm);
            for (int dg = 0; dg <= nd + 1; dg++) for (int rs = -1; rs <= 1; rs++) hz.d(instrumentHz(tr, I, dg, rs));
            std::vector<InstrumentPart> ps; instrumentParts(tr, I, ps);
            partsBytes(parts, ps);
            courses.i(instrumentCourses(tr, I)); courses.i(instrumentCoursesOf(ps));
            double u0 = 0, u1 = 0, v0 = 0, v1 = 0, top = 0, lu = 0, lv = 0, tp = 0;
            instrumentBounds(ps, u0, u1, v0, v1, top); instrumentExtent(ps, lu, lv, tp);
            bounds.d(u0); bounds.d(u1); bounds.d(v0); bounds.d(v1); bounds.d(top); bounds.d(lu); bounds.d(lv); bounds.d(tp);
            std::vector<Form> fs; instrumentForms(I, tr, 1.5, -2.0, 0.7, fs);
            formsBytes(forms, fs);
            for (const InstrumentPart& p : ps) if (p.kind == IPK_PLATE) formBytes(forms, instrumentPlateForm(p));
            if (I.disc) { Piece P; instrumentDiscOf(tr, I, P); pieceBytes(disc, disc, P); }
            std::string t;
            if (O.full) fpPut(t, "%s [%d]: %s of %s and %s, %s, reg %+d, %d courses, %zu parts\n", nm.c_str(), I.id, M ? INSTRUMENT_PLANS[clampi(M->plan, 0, IP_COUNT - 1)].name : "?",
                              M ? INSTRUMENT_MATERIAL_NAMES[clampi(M->mat, 0, IM_COUNT - 1)] : "?", M ? INSTRUMENT_MATERIAL_NAMES[clampi(M->mat2, 0, IM_COUNT - 1)] : "?",
                              instrumentTimbre(tr, I) ? TIMBRE_FAMILY_NAMES[clampi(instrumentTimbre(tr, I)->family, 0, TF_COUNT - 1)] : "its cylinder", I.reg,
                              instrumentCourses(tr, I), ps.size());
            O.put(Line("instrument", w.key).kn(c.p).kn(I.id).n("age", I.age).n("parts", (long long)ps.size()).h("inst", inst).h("make", make).h("timbre", timbre)
                  .h("name", name).h("hz", hz).h("partlist", parts).h("forms", forms).h("courses", courses).h("bounds", bounds).h("disc", disc).s, t);
            T.instruments++;
        }
    Bytes records, plans, materials;
    for (int i = 0; i < c.L.records; i++) {
        double pick = 0;
        const bool with = instrumentWithRecord(w.g.seed, c.p, i, pick);
        records.bo(with);
        if (with) { records.d(pick); for (int a = 0; a < AGE_COUNT; a++) records.i(instrumentIdOf(c.PI, a, pick)); }
    }
    std::vector<InstrumentPart> ps;
    for (int p = 0; p < IP_COUNT; p++) {
        const int lo = clampi(INSTRUMENT_PLANS[p].ageMin, 0, AGE_COUNT - 1), hi = clampi(INSTRUMENT_PLANS[p].ageMax, 0, AGE_COUNT - 1);
        for (int a : {lo, hi}) {
            const InstrumentMake M = instrumentMakeOfPlan(w.g, c.L, p, a, (uint64_t)p * 7 + (uint64_t)a);
            makeBytes(plans, M);
            ps.clear(); instrumentPartsOf(tr, M, ps);
            partsBytes(plans, ps);
            if (lo == hi) break;
        }
    }
    for (int m = 0; m < IM_COUNT; m++) {
        for (int a = 0; a < AGE_COUNT; a++) materials.bo(instrumentMaterialGiven(w.g, c.L, m, a));
        materials.rgb(instrumentMaterialColour(m, c.cu.palette));
        int part = 0, slot = 0; instrumentMaterialPart(m, part, slot);
        materials.i(part); materials.i(slot); materials.bo(instrumentMaterialMetal(m));
    }
    O.put(Line("instruments", w.key).kn(c.p).h("records", records).h("plans", plans).h("materials", materials).s);
}

// a people's tongue (its sounds, its spelling, its morphemes), the names, words and terms it makes, and its voice
void tongueLine(Out& O, const World& w, const People& c) {
    const Tongue tg = tongueOf(w.g, c.L);
    Bytes tongue, names, words, terms, voice;
    tongue.u(tg.seed); tongue.u(tg.people); vecS(tongue, tg.onsets); vecS(tongue, tg.nuclei); vecS(tongue, tg.codas); tongue.d(tg.codaChance);
    tongue.d(tg.vowelStart); tongue.s(tg.endS); tongue.s(tg.endPast); tongue.s(tg.endIng); vecS(tongue, tg.cons); vecS(tongue, tg.vows); vecS(tongue, tg.liquids);
    vecS(tongue, tg.sibs); vecS(tongue, tg.finals); vecS(tongue, tg.shapes); tongue.i(tg.minSyl); tongue.i(tg.maxSyl); tongue.d(tg.gem); vecS(tongue, tg.roots);
    for (int k = 0; k < TM_COUNT; k++) tongue.s(tg.morph[k]);
    tongue.s(tg.joiner); tongue.bo(tg.genericFirst); tongue.bo(tg.capSecond); tongue.s(tg.femEnd); tongue.s(tg.mascEnd); tongue.i(tg.family); tongue.s(tg.famAffix);
    tongue.i(tg.style); tongue.s(tg.inventory);
    std::string t, sample;
    for (int kind = 0; kind < TN_COUNT; kind++)
        for (int i = 0; i < 6; i++)
            for (int fem = 0; fem < 2; fem++) {
                const std::string nm = tongueName(tg, mix64(c.L.seed ^ (0xE02A000ULL + (uint64_t)kind * 64 + (uint64_t)i)), kind, fem != 0);
                names.s(nm); names.bo(tongueReadable(nm));
                if (O.full && i < 3 && fem == 0) sample += (sample.empty() ? "" : ", ") + nm;
            }
    for (int sc = 0; sc < 7; sc++) names.i(tongueNameKindOfRank(sc));
    static const char* const WORDS[] = {"write", "written", "the", "and", "river", "remember", "year", "king", "house", "mother", "bread", "water", "go", "come",
                                        "a", "of", "to", "in", "it", "is", "star", "dark", "sun", "child", "war", "song", "stone", "end"};
    static const char* const TERMS[] = {"coin", "bread", "beast", "feast", "god", "boat", "plough", "loom", "engine", "signal"};
    std::string ws;
    for (const char* x : WORDS) for (int at = 0; at < 2; at++) { const std::string v = tongueWord(tg, x, at); words.s(v); if (O.full && at == 0) ws += std::string(" ") + x + ":" + v; }
    for (const char* x : TERMS) terms.s(tongueTerm(tg, x));
    const Voice V = voiceOf(w.g, c.L, tg), R = voiceRead(V);
    for (const Voice* v : {&V, &R}) {
        voice.d(v->pitch); voice.d(v->range); voice.d(v->rate); voice.d(v->tract); voice.d(v->breath); voice.d(v->rough); voice.d(v->nasal); voice.d(v->tremor);
        voice.d(v->sub); voice.d(v->subRatio); voice.i(v->stress); voice.i(v->contour); voice.bo(v->tonal); voice.bo(v->clicky); voice.bo(v->trill);
        vecI(voice, v->inventory); voice.u(v->seed); voice.bo(v->read); voice.s(voiceLine(*v));
    }
    if (O.full) fpPut(t, "%s (%d): %s; %s\n%s\n", tg.inventory.c_str(), tg.style, sample.c_str(), voiceLine(V).c_str(), ws.c_str());
    O.put(Line("tongue", w.key).kn(c.p).h("tongue", tongue).h("names", names).h("words", words).h("terms", terms).h("voice", voice).s, t);
}

// a people's genome (its values, its colours, its techniques, what its age lets it build), its culture and palette, its ways in every age
void genomeLine(Out& O, const World& w, const People& c, const Genome* other) {
    const uint64_t seed = peopleGen(w.g, c.p).seed;
    const Genome G = genomeOf(seed);
    Bytes genome, values, colours, techniques, culture, palette, ages, distance;
    genomeBytes(genome, G);
    for (int k = 0; k < GV_COUNT; k++) { values.s(genomeValue(G, k)); values.i(G.of(k)); }
    values.i(G.number()); values.i(G.accentColour()); values.i(G.accentPlace());
    colours.rgb(genomeBase(G)); colours.rgb(genomeAccent(G)); colours.rgb(genomeTint(G)); colours.rgb(genomeLight(G));
    for (int a = 0; a < AGE_COUNT; a++) { colours.rgb(genomeRoofing(G, a)); techniques.i(techniqueOf(G, a, seed)); }
    for (int k = 0; k < GV_COUNT; k++) for (int great = 0; great < 2; great++) techniques.i(ageExpressed(c.cu, k, great != 0));
    cultureBytes(culture, c.cu);
    culture.d(decayOf(c.cu.ageYears, c.cu.wet));
    for (int sc = 0; sc < 7; sc++) culture.d(settlementRadiusOf(c.cu, sc, mix64(seed ^ (uint64_t)sc)));
    paletteBytes(palette, culturePalette(c.cu, seed));
    for (int a = 0; a < AGE_COUNT; a++) for (double extra : {0.0, 2500.0}) cultureBytes(ages, cultureAtAge(c.cu, a, extra));
    std::string shared;
    distance.d(genomeDistance(G, other ? *other : genomeOf(0xE02D157ULL), &shared)); distance.s(shared); distance.d(genomeMaxDistance());
    std::string t;
    if (O.full) {
        for (int k = 0; k < GV_COUNT; k++) fpPut(t, "%s%s %s", k ? "; " : "", GENOME_VARS[k].name, genomeValue(G, k).c_str());
        fpPut(t, "\nculture: age %d, %.0f years, wet %.2f, decay %.3f, technique %s, roof %d, temple %d, hut %d, towers %d, great %d\n", c.cu.age, c.cu.ageYears, c.cu.wet,
              c.cu.decay, TECHNIQUE_NAMES[clampi(c.cu.technique, 0, TQ_COUNT - 1)], c.cu.roof, c.cu.temple, c.cu.hut, c.cu.towerForm, c.cu.great);
    }
    O.put(Line("genome", w.key).kn(c.p).n("age", c.cu.age).h("genome", genome).h("values", values).h("colours", colours).h("techniques", techniques)
          .h("culture", culture).h("palette", palette).h("ages", ages).h("distance", distance).s, t);
}

void peopleWorld(Out& O, Tally& T, const WorldRef& ref) {
    World w;
    if (!loadWorld(ref.sx, ref.sy, ref.sz, ref.body, w)) { O.put(Line("plan", w.key).w("none", "world").s); return; }
    const Body& b = w.sys.bodies[w.body];
    if (!worldHadCivilisation(w.g)) { O.put(Line("plan", w.key).w("none", "people").s); return; }
    T.worlds++;
    std::shared_ptr<const WorldPlan> P = worldPlanOf(w.g);
    const int np = std::max(1, std::min(peoplesOf(w.g), P->peoples));
    for (int p = 0; p < np; p++) historyOfWorld(w.g, p);   // what the works and the places read, built before them as the game builds them
    std::shared_ptr<const WorldWorks> WW = worldWorksOf(w.g);
    People ctx[2];
    for (int p = 0; p < np; p++) {
        ctx[p].p = p;
        ctx[p].cu = cultureOf(w.g, p);
        ctx[p].L = loreQuick(w.sys, b, w.g, p);
        ctx[p].T = traditionOf(w.g, ctx[p].L);
        instrumentsOf(ctx[p].T, ctx[p].L.which, ctx[p].PI);
        T.peoples++;
    }
    planLines(O, w, *P);
    for (int p = 0; p < np; p++) peoplePlanLines(O, w, *P, p);
    std::set<std::pair<int, int>> seen;
    for (int p = 0; p < np; p++) {
        int vLat = 0, vLon = 0;
        settlementLines(O, T, w, *P, ctx, np, p, seen, vLat, vLon);
        const PeoplePlan& pp = P->plans[p];
        if (pp.places.empty()) continue;
        roadsLine(O, T, w, *P, ctx, np, p, "capital", pp.places[0].unit, true);
        for (const PlanPlace& q : pp.places) if (q.sclass == SC_CITY) { roadsLine(O, T, w, *P, ctx, np, p, "city", q.unit, false); break; }
        const double dLat = ruinCellLat(w.g), cl = (vLat + 0.5) * dLat, dLon = dLat / std::max(std::cos(cl), 0.05);
        roadsLine(O, T, w, *P, ctx, np, p, "country", StarSystem::bodyFromLatLon(cl, (vLon + 0.5) * dLon), false);
    }
    if (WW) worksLines(O, T, w, *P, *WW, ctx, np);
    Genome genomes[2];
    for (int p = 0; p < np; p++) genomes[p] = genomeOf(peopleGen(w.g, p).seed);
    for (int p = 0; p < np; p++) {
        sheetLines(O, T, w, *P, ctx[p]);
        musicLines(O, T, w, ctx[p]);
        instrumentLines(O, T, w, ctx[p]);
        tongueLine(O, w, ctx[p]);
        genomeLine(O, w, ctx[p], np > 1 ? &genomes[1 - p] : nullptr);
    }
    fflush(O.f);
}

// ---- the drained kinds' land: the flora and the drainage --------------------------------------------------------------------------------------

void siteLines(Out& O, Tally& T, int64_t sx, int64_t sz, int body) {
    World w;
    if (!loadWorld(sx, 0, sz, body, w)) { O.put(Line("flora", w.key).w("none", "world").s); return; }
    const Body& b = w.sys.bodies[w.body];
    // the site: a forest where the kind grows one, else grass, else land, drawn from the body's seed (the ground read without the rivers: a
    // draw must not build a drainage tile)
    double lat = 0, lon = 0;
    {
        DrainageOff off;
        Rng r(b.seed ^ 0xE02F10AULL);
        int best = -1;
        const bool flora = kindHas(b.type, KT_FLORA);
        for (int k = 0; k < 64 && !(best == 3 || (best >= 1 && !flora)); k++) {
            const double la = r.range(-55 * DEG, 55 * DEG), lo = r.range(-PI, PI);
            const int m = sampleSurface(w.g, StarSystem::bodyFromLatLon(la, lo), 64).material;
            const int score = m == MAT_FOREST ? 3 : (m == MAT_GRASS ? 2 : (m != MAT_WATER ? 1 : 0));
            if (score > best) { best = score; lat = la; lon = lo; }
        }
    }
    std::unique_ptr<SurfaceView> sv(new SurfaceView());
    sv->init(&w.sys, w.body, lat, lon, 1000.0);
    // the flora of a kilometre square round the capsule, the boulders, the ground's vertices, and the colliders at a lattice of points
    Bytes trees, logs, rocks, colliders, vertices;
    long nt = 0, nl = 0, nr = 0;
    int fams[10] = {};
    for (int cz = -32; cz < 32; cz++)
        for (int cx = -32; cx < 32; cx++) {
            sv->forTrees(cx, cz, [&](const TreeInst& t) {
                trees.d(t.x); trees.d(t.z); trees.d(t.gy); trees.d(t.h); trees.d(t.r); trees.d(t.cy); trees.d(t.lx); trees.d(t.lz); trees.i(t.fam); trees.i(t.bank);
                trees.bo(t.dead); trees.bo(t.bare); trees.bo(t.stump); trees.u(t.seed); trees.d(t.dens);
                nt++; fams[clampi(t.fam, 0, 9)]++;
            });
            sv->forLogs(cx, cz, [&](const LogInst& l) {
                logs.d(l.x); logs.d(l.z); logs.d(l.heading); logs.d(l.len); logs.d(l.radius); logs.u(l.seed); logs.bo(l.stone);
                nl++;
            });
            sv->forRocks(cx, cz, [](double, double, double) { return true; }, [&](const SurfaceView::RockInst& r) {
                rocks.d(r.x); rocks.d(r.z); rocks.d(r.size); rocks.d(r.gy); rocks.u(r.h);
                nr++;
            });
            const TerrainVertex& tv = sv->site.lod0.at(cx, cz);
            vertices.f(tv.h); vertices.f(tv.albedo); vertices.f(tv.veg); vertices.i(tv.material); vertices.i(tv.glow); vertices.i(tv.biome); vertices.f(tv.water);
            vertices.f(tv.shore); vertices.i(tv.scree); vertices.i(tv.road); vertices.i(tv.wtint); vertices.i(tv.ltint); vertices.i(tv.gtint);
        }
    std::vector<SurfaceView::Collider> cs;
    for (int j = -3; j <= 3; j++)
        for (int i = -3; i <= 3; i++) {
            sv->collectColliders(i * 96.0 + 8, j * 96.0 - 8, cs);
            colliders.n(cs.size());
            for (const SurfaceView::Collider& c : cs) { colliders.d(c.x); colliders.d(c.z); colliders.d(c.r); colliders.i(c.kind); colliders.d(c.y0); colliders.d(c.y1); }
        }
    Bytes season; season.d(sv->seasonPhaseLocal()); season.d(sv->site.season);
    char where[64]; snprintf(where, sizeof where, "%.5f", lat / DEG);
    char where2[64]; snprintf(where2, sizeof where2, "%.5f", lon / DEG);
    std::string t;
    if (O.full) {
        fpPut(t, "%s (%s) at %s %s: %ld trees, %ld logs, %ld boulders; by silhouette", b.name.c_str(), KINDS[b.type].name, where, where2, nt, nl, nr);
        for (int k = 0; k < 10; k++) fpPut(t, " %d", fams[k]);
        t += "\n";
    }
    O.put(Line("flora", w.key).n("kind", b.type).w("lat", where).w("lon", where2).n("trees", nt).n("logs", nl).n("rocks", nr).h("tree", trees).h("log", logs)
          .h("rock", rocks).h("colliders", colliders).h("vertices", vertices).h("season", season).s, t);
    // the drainage there, on the view's own generator (the tiles its landing built): the channels at three details and a dry country's
    // catchment, the flood's lakes, the ground with the rivers in it, and the flood's own numbers round the site
    const BodyGen& g = sv->site.gen;
    Bytes at, lakes, ground, stats;
    int channels = 0;
    static const double DETAILS[3] = {16.0, 64.0, 256.0};
    for (int j = -16; j <= 16; j++)
        for (int i = -16; i <= 16; i++) {
            const Vec3 u = sv->site.unitAt(i * 40.0, j * 40.0);
            for (double det : DETAILS) { const DrainInfo d = drainageAt(g, u, det, 0.5); drainBytes(at, d); if (det == 16.0 && d.any) channels++; }
            drainBytes(at, drainageAt(g, u, 16.0, 0.15));
            double level = 0; bool known = false;
            const bool lake = drainageLakeCell(g, u, level, true, known);
            lakes.bo(lake); lakes.bo(known);
            if (lake) lakes.d(level);
            if (((i | j) & 1) == 0) sampleBytes(ground, sampleSurface(g, u, 16.0));
        }
    const DrainStats ds = drainageStats(g, sv->site.up0, 30);
    stats.i(ds.cells); stats.i(ds.channels); stats.i(ds.lakes); stats.i(ds.sea); stats.d(ds.maxAcc); stats.i(ds.monotone); stats.i(ds.tested); stats.i(ds.lakesKept);
    stats.i(ds.lakesBreached); stats.bo(typeHasDrainage(g.type)); stats.i(drainageLatticeN0(b.radiusKm));
    std::string t2;
    if (O.full)
        fpPut(t2,
                "the flood round it: %d cells, %d channels, %d lake cells (%d lakes kept, %d breached), %d sea, the largest %.0f km2, %d of %d downstream lower or level; %d of 1089 points on a "
                "channel at 16 m\n",
                ds.cells, ds.channels, ds.lakes, ds.lakesKept, ds.lakesBreached, ds.sea, ds.maxAcc, ds.monotone, ds.tested, channels);
    O.put(Line("drainage", w.key).n("kind", b.type).n("channels", channels).n("cells", ds.cells).h("at", at).h("lakes", lakes).h("ground", ground).h("stats", stats).s, t2);
    T.sites++;
}

void drainedSites(Out& O, Tally& T, int every) {
    // the first world of each drained kind in the scan region that had no people (two felisian ones), in the scan's order
    struct Site { int64_t sx, sz; int body; };
    std::vector<Site> sites;
    int have[PT_COUNT] = {};
    for (int64_t x = 150; x < 340; x++)
        for (int64_t z = 20; z < 160; z++) {
            Star st;
            if (!starInSector(x, 0, z, st)) continue;
            StarSystem sys; sys.generate(st);
            for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
                const Body& b = sys.bodies[bi];
                if (b.type < 0 || b.type >= PT_COUNT || b.type == PT_COMPANION || !KINDS[b.type].landable || !kindHas(b.type, KT_DRAINAGE)) continue;
                if (have[b.type] >= (b.type == PT_FELISIAN ? 2 : 1) || worldHadCivilisation(BodyGen::make(b))) continue;
                have[b.type]++;
                sites.push_back({x, z, bi});
            }
        }
    for (size_t i = 0; i < sites.size(); i += every) siteLines(O, T, sites[i].sx, sites[i].sz, sites[i].body);
}

// ---- the almanac -----------------------------------------------------------------------------------------------------------------------------

void placeLine(Out& O, Tally& T, const std::string& skey, int bi, const char* where, const AlmanacPlace& p) {
    const StarSystem& sys = *p.sys;
    std::vector<AlmanacEvent> ev; almanacOf(p, ev);
    Bytes list, labels, sun;
    list.n(ev.size());
    std::function<std::string(int)> nameOf = [&](int j) { return j >= 0 && j < (int)sys.bodies.size() ? sys.bodies[j].name : std::string("?"); };
    std::string t;
    for (const AlmanacEvent& e : ev) {
        list.i(e.kind); list.d(e.t); list.i(e.body); list.i(e.body2); list.d(e.value);
        const std::string lb = almanacLabel(e, sys, nameOf), cd = countdownString(e.t - p.t);
        labels.s(lb); labels.s(cd);
        if (O.full) fpPut(t, "in %-12s %s\n", cd.c_str(), almanacLabel(e, sys, nameOf, " deg").c_str());
    }
    if (p.ground)
        for (int hh = 0; hh < 24; hh++) { double ar = 0; sun.d(almanacSunAltitude(p, p.t + hh * 3600.0, &ar)); sun.d(ar); }
    O.put(Line("almanac", skey).kn(bi).kw(where).n("events", (long long)ev.size()).h("list", list).h("labels", labels).h("sun", sun).s, t);
    T.places++; T.events += (long)ev.size();
}
void systemAlmanac(Out& O, Tally& T, const Star& st, bool ship) {
    StarSystem sys; sys.generate(st);
    std::string skey;
    fpPut(skey, "%lld %lld %lld", (long long)st.sx, (long long)st.sy, (long long)st.sz);
    for (int bi = 0; bi < (int)sys.bodies.size(); bi++) {
        const Body& b = sys.bodies[bi];
        if (b.type == PT_COMPANION) continue;
        Rng r(b.seed ^ 0xE02A1AULL);
        const double lat = r.range(-70 * DEG, 70 * DEG), lon = r.range(-PI, PI), t = 3.6e6 + r.range(0, 40 * 86400.0);
        if (KINDS[b.type].landable && b.type != PT_COMET) {   // a place on the ground (the harness's `almanacGroundPlace`)
            AlmanacPlace p; p.sys = &sys; p.t = t; p.window = 5 * 86400.0; p.ground = true; p.body = bi; p.lat = lat; p.lon = lon; p.atmosphere = bodyHasAir(b);
            if (b.rings) p.ringProf = sys.ringProfileOf(bi);
            placeLine(O, T, skey, bi, "ground", p);
        }
        if (ship && b.parent < 0 && b.type != PT_COMET) {   // the ship parked at a planet as `almanacShipPlace` parks it (`Game::parkDistanceFor`'s distance)
            AlmanacPlace p; p.sys = &sys; p.t = t; p.ground = false; p.body = bi; p.window = 30 * 86400.0;
            const Vec3 bp = sys.bodyPos(bi, t), toStar = normalize(sys.star.pos - bp), side = normalize(cross(toStar, Vec3(0, 1, 0)));
            p.parkDir = normalize(toStar * 0.7 + side * 0.7 + Vec3(0, 0.2, 0));
            p.parkDist = b.radiusKm * (b.rings ? std::max(3.5, b.ringOuter + 1.3) : 3.5);
            p.orbiting = true;
            p.pos = bp + p.parkDir * p.parkDist;
            placeLine(O, T, skey, bi, "ship", p);
        }
        if (b.type == PT_COMET) continue;
        // the nights: its showers, its sky's dust, its fronts and what they bring to the place
        std::vector<MeteorShower> sh; meteorShowersOf(sys, bi, sh);
        Bytes showers, dust, fronts, forecast;
        for (const MeteorShower& s : sh) {
            showers.i(s.comet); showers.i(s.world); showers.d(s.moidKm); showers.d(s.strength); showers.d(s.period); showers.d(s.tPeak0); showers.d(s.sigma);
            showers.v3(s.radiant); showers.d(s.speedKms); showers.v3(s.streamKm); showers.v3(s.worldKm); showers.d(showerActivity(s, t)); showers.d(showerPeakAfter(s, t));
        }
        dust.d(zodiacalDust(sys, bi));
        int nf = 0;
        std::string tx;
        if (worldHasFronts(b)) {
            std::vector<Front> fr; frontsOf(b, t, fr, 5 * 86400.0);
            const Vec3 u = StarSystem::bodyFromLatLon(lat, lon);
            fronts.i(frontSlots(b));
            for (const Front& f : fr) {
                fronts.i(f.slot); fronts.i(f.epoch); fronts.d(f.born); fronts.d(f.dies); fronts.v3(f.c0); fronts.v3(f.n0); fronts.v3(f.a); fronts.d(f.speed); fronts.d(f.R);
                fronts.d(f.halfLen); fronts.d(f.width); fronts.d(f.strength); fronts.d(f.tempDrop); fronts.d(f.bow); fronts.u(f.seed); fronts.bo(f.dust);
                fronts.d(frontTexture(f, u)); fronts.d(frontLife(f, t));
            }
            FrontWeather fw; frontWeatherAt(fr, u, t, b.radiusKm * 1000.0, fw);
            fronts.d(fw.cloud); fronts.d(fw.precip); fronts.d(fw.windRise); fronts.d(fw.cold); fronts.d(fw.windWeight); fronts.v3(fw.windTo); fronts.i(fw.nearest);
            fronts.d(fw.nearestAhead); fronts.d(fw.nearestAlong); fronts.d(fw.nearestGain); fronts.d(fw.nearestTex);
            std::vector<FrontForecast> fc; frontsAhead(b, u, t, 5 * 86400.0, fc);
            for (const FrontForecast& x : fc) {
                forecast.d(x.tArrive); forecast.d(x.tClear); forecast.d(x.fromBearing); forecast.d(x.strength); forecast.d(x.gain); forecast.bo(x.dust); forecast.i(x.slot);
                forecast.i(x.epoch);
                if (O.full) fpPut(tx, "a %s front from the %s in %s, clearing in %s\n", x.dust ? "dust" : "rain", frontCompass(x.fromBearing), countdownString(x.tArrive - t).c_str(),
                                  countdownString(x.tClear - t).c_str());
            }
            nf = (int)fr.size();
        }
        if (O.full) for (const MeteorShower& s : sh) fpPut(tx, "a shower from %s: strength %.3f, the next peak in %s\n", sys.bodies[clampi(s.comet, 0, (int)sys.bodies.size() - 1)].name.c_str(),
                                                          s.strength, countdownString(showerPeakAfter(s, t) - t).c_str());
        O.put(Line("nights", skey).kn(bi).n("showers", (long long)sh.size()).n("fronts", nf).h("shower", showers).h("dust", dust).h("front", fronts).h("forecast", forecast).s, tx);
    }
}
void almanacs(Out& O, Tally& T, int every) {
    // home, then every 40th system of the scan region in the scan's order; the ship parked at the planets of every third of them (a parking's
    // thirty days take some 80 ms)
    std::vector<Star> stars;
    { Star h; if (starInSector(HOME_SX, HOME_SY, HOME_SZ, h, true)) stars.push_back(h); }
    int k = 0;
    for (int64_t x = 150; x < 340; x++)
        for (int64_t z = 20; z < 160; z++) {
            Star st;
            if (!starInSector(x, 0, z, st, true)) continue;
            if (k++ % 40 == 0 && !(x == HOME_SX && z == HOME_SZ && HOME_SY == 0)) stars.push_back(st);
        }
    for (size_t i = 0; i < stars.size(); i += every) systemAlmanac(O, T, stars[i], i % 3 == 0);
}

// ---- the tables ------------------------------------------------------------------------------------------------------------------------------

void tablesLine(Out& O) {
    Bytes decay, kinds, names, streets, edits, sink;
    static const double YEARS[6] = {1, 30, 300, 3000, 30000, 300000};
    static const double WETS[4] = {0, 0.3, 0.7, 1};
    for (int m = 0; m < MT_COUNT; m++) for (double y : YEARS) for (double wt : WETS) decay.d(materialDecay(m, y, wt));
    for (double y : YEARS) for (double wt : WETS) { decay.d(decayOf(y, wt)); edits.d(editLeft(y, wt, false)); edits.d(editLeft(y, wt, true)); }
    for (int k = 0; k < BK_COUNT; k++) { kinds.i(buildingAge(k)); kinds.cs(buildingPlaceName(k)); kinds.cs(BUILDING_KIND_NAMES[k]); }
    for (int k = 0; k < 7; k++) names.cs(SETTLEMENT_CLASS_NAMES[k]);
    for (int k = 0; k < WO_COUNT; k++) { names.cs(WORK_KIND_NAMES[k]); names.cs(WORK_HEARD[k]); }
    for (int k = 0; k < FK_COUNT; k++) names.cs(FORM_KIND_NAMES[k]);
    for (int k = 0; k < MT_COUNT; k++) names.cs(BUILT_MATERIAL_NAMES[k]);
    for (int k = 0; k < NK_COUNT; k++) { names.cs(NET_KIND_NAMES[k]); names.cs(NET_KIND_HUD[k]); names.u(NET_KIND_COLOURS[k]); }
    for (int k = 0; k < SR_COUNT; k++) names.cs(STREET_RANK_NAMES[k]);
    for (int k = 0; k < DI_COUNT; k++) names.cs(DISTRICT_NAMES[k]);
    for (int k = 0; k < FI_COUNT; k++) names.cs(FIELD_KIND_NAMES[k]);
    for (int k = 0; k < TF_COUNT; k++) { names.cs(TIMBRE_FAMILY_NAMES[k]); names.bo(timbrePercussive(k)); }
    for (int k = 0; k < PF_COUNT; k++) names.cs(PIECE_FORM_NAMES[k]);
    for (int k = 0; k < IC_COUNT; k++) names.cs(INSTRUMENT_CLASS_NAMES[k]);
    for (int k = 0; k < IP_COUNT; k++) {
        const InstrumentPlanInfo& I = INSTRUMENT_PLANS[k];
        names.i(I.cls); names.cs(I.name); names.i(I.ageMin); names.i(I.ageMax); names.d(I.weight); names.u(I.flags);
    }
    for (int k = 0; k < IM_COUNT; k++) names.cs(INSTRUMENT_MATERIAL_NAMES[k]);
    for (int k = 0; k < TQ_COUNT; k++) names.cs(TECHNIQUE_NAMES[k]);
    for (int k = 0; k < GV_COUNT; k++) {
        const GenomeVarInfo& V = GENOME_VARS[k];
        names.cs(V.name); names.cs(V.group); names.i(V.n);
        for (int j = 0; j < V.n; j++) names.cs(V.values ? V.values[j] : nullptr);
        names.cs(V.lo); names.cs(V.hi); names.u(V.ages); names.d(V.weight);
    }
    for (int p = 0; p < 2; p++) for (int s = 0; s < FORM_SLOTS; s++) names.i(FORM_BANKS[p][s]);
    names.d(genomeMaxDistance()); names.d(GENOME_ALIKE);
    for (int a = 0; a < AGE_COUNT; a++) { streets.d(streetSpacing(a)); for (int rk = 0; rk < SR_COUNT; rk++) streets.d(streetHalf(rk, a)); }
    static const float SINK_V[4] = {1.0f, 0.5f, 123.4f, -7.25f};
    static const long SINK_STEPS[5] = {0, 1, 10, 1000, 100000};
    for (float v : SINK_V) for (long s : SINK_STEPS) sink.f(drainageSink(v, s));
    O.put(Line("tables", "all").h("decay", decay).h("kinds", kinds).h("names", names).h("streets", streets).h("edits", edits).h("sink", sink).s);
}

}   // namespace

int fingerprintModules(FILE* out, int every, bool full) {
    const bool drained = drainageEnabled();
    setDrainageEnabled(true);   // the drainage as the game has it, whatever the environment says
    Out O; O.f = out; O.full = full;
    Tally T;
    every = std::max(1, every);
    tablesLine(O);
    const int nWorlds = (int)(sizeof PEOPLE_WORLDS / sizeof PEOPLE_WORLDS[0]);
    for (int i = 0; i < nWorlds; i += every) peopleWorld(O, T, PEOPLE_WORLDS[i]);
    drainedSites(O, T, every);
    almanacs(O, T, every);
    setDrainageEnabled(drained);
    printf("fingerprint modules: %d worlds, %d peoples: %d settlements, %d works, %d roads, %d pieces, %d instruments, %d sheets (%ld pieces, %ld discs); "
           "%d drained sites; %d almanac places (%ld events); %ld lines\n",   // no timings: the printout is compared too
           T.worlds, T.peoples, T.settlements, T.works, T.roads, T.pieces, T.instruments, T.sheets, T.forms, T.discs, T.sites, T.places, T.events, O.lines);
    return 0;
}
