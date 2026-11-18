// Z-04: the laboratory's state (game/microscope.cpp, microscope_draw.cpp; R-425, docs/ideas/ITERATION-1.5.0.md Z-04): the microscope's page
// (the cold store's vials and the flasks, or the slide on its stage), the vial or the flask there and what it is made of (its worlds' systems
// drawn once, their biotas and forms, its sample or its culture's drop and its mount: made again whenever it is put on the stage), what was
// done to the slide and its clock, the view (its way of seeing, magnification, place, focus, the electron's turn and tilt, a copy tracked), the
// sheet shown. Nothing of it is saved: what was seen of a species is the guide's (`microbe` lines). Z-05: the incubator's flasks and the jars'
// shelf, on the lab's clock (the drones' kind: real seconds while the game simulates), and the incubator's screen; their save lines
#pragma once
#include "life/slides.h"
#include "life/cultures.h"
#include "life/jars.h"
#include "life/sequences.h"
#include "ship_plan.h"
#include "galaxy/system.h"
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

// Z-05: a world's system as the lab reads it (a vial's, a flask's species', a sequenced one's): the explorer's own while it is the one there,
// else drawn and kept for the next of the same star (Z-06: the sequencer's and the tree's too)
struct LabSystems {
    std::map<std::tuple<int64_t, int64_t, int64_t>, std::shared_ptr<StarSystem>> drawn;
    const StarSystem* get(const StarSystem& here, int64_t sx, int64_t sy, int64_t sz) {
        if (here.valid && here.star.sx == sx && here.star.sy == sy && here.star.sz == sz) return &here;
        auto& slot = drawn[{sx, sy, sz}];
        if (!slot) {
            Star st;
            if (!placeInSector(sx, sy, sz, st, true)) return nullptr;
            slot = std::make_shared<StarSystem>();
            slot->generate(st);
        }
        return slot.get();
    }
};

// the magnifications the zoom steps through: the light ways' objectives (x40, x100, x400, x1,000) with the camera's steps between and on the oil
// one (x2,000, x4,000); the electron's x200 (a large cell whole) to x40,000
constexpr int SCOPE_LIGHT_STEPS = 7, SCOPE_ELECTRON_STEPS = 7;
constexpr double SCOPE_LIGHT[SCOPE_LIGHT_STEPS] = {40, 100, 200, 400, 1000, 2000, 4000};
constexpr double SCOPE_ELECTRON[SCOPE_ELECTRON_STEPS] = {200, 500, 1000, 3000, 10000, 20000, 40000};

struct MicroscopeState {
    int page = 0;                 // 0 the cold store's vials and the flasks, 1 the slide
    int sel = 0, top = 0;         // the list's row (the vials', then the flasks') and the first one shown
    int vial = -1;                // the cold store's place on the stage (-1 none)
    int flask = -1;               // Z-05: or the flask whose drop is (-1 none)
    StarSystem sys; int body = -1;
    std::shared_ptr<const SystemLife> life;
    std::vector<Morph> forms;
    // Z-05: the slide's species (a vial's its world's list, a flask's its one or two with their strains' moves), each its guide key and its world;
    // the HUD's two lines for what is on the stage (a vial's world and its line, a flask's number and its stage)
    std::vector<Microbe> species;
    std::vector<std::string> keys, worlds;
    std::string title, line;
    Sample sample;
    SlideMount mount;
    std::vector<SlideEvent> events;
    double t = 0;                 // the slide's clock (s): real seconds on its page, stopped under the electron's beam
    int seeing = WS_BRIGHT, step = 3;
    double logMag = 0;            // the magnification shown, its logarithm easing to its step's
    double cx = 0, cy = 0, focus = 0;   // the stage's place under the objective (um), the depth in focus (um over the slide's glass)
    double turn = 0.6, tilt = 0.55;     // the electron's
    bool tracking = false; SlideRef track;
    bool sheet = false;           // Tab: the sheet of the species under the crosshair
    int sheetSpecies = -1;        // its species (in the world's list)
    // the frame's: its specimens and what each is, the one under the crosshair (-1 none)
    std::vector<Specimen> shown; std::vector<SlideRef> refs;
    int under = -1;
};

// Z-05: the incubator (game/incubator.cpp, incubator_draw.cpp): twelve flasks, each a medium and a culture of one or two species put in it on the
// lab's clock; the jars' shelf, a world's silt in a column a world
// a flask's species: its world (a vial's star sector and body), its index in that world's list, its strain (0 the wild one: the guide's `strain`
// lines number the rest)
struct FlaskSpecies {
    int64_t sx = 0, sy = 0, sz = 0;
    int body = -1, species = -1;
    int strain = 0;      // its strain now
    int strainIn = 0;    // the strain it was put in as (the flask's strains after it are its events)
    bool sameSpecies(const FlaskSpecies& o) const { return sx == o.sx && sy == o.sy && sz == o.sz && body == o.body && species == o.species; }
};
// the moments a flask's culture told (its HUD lines, the log's): its spores woke, its crowd glowing
enum FlaskTold : uint32_t { FT_WOKEN = 1, FT_GLOW = 2 };
struct Flask {
    int n = 0;                          // its species (0: empty)
    FlaskSpecies who[2];
    std::vector<CultureEvent> events;   // its culture's, on the lab's clock
    Medium medium;                      // its controls now
    Medium asWorld;                     // AS ITS WORLD: its first species' place's (its first guess, W again)
    uint32_t told = 0;
    // made again from those (nothing saved): its culture (the kinds read from their worlds), its laws laid out, the forms the microscope draws,
    // the words its lines say (each species' name or UNNAMED and what it looks like), its worlds' names
    bool made = false;
    FlaskCulture culture; CultureRun run;
    std::vector<Morph> forms;
    std::string label[2];
    double sleptYears[2] = {0, 0};      // a vial's spores' sleep: its world's end, its drifter's leaving, the impact that carried them
};
// a world in a jar (Winogradsky's column): the vial's world and place it was made from, its line, when it was made
struct Jar {
    int64_t sx = 0, sy = 0, sz = 0;
    int body = -1;
    Microhabitat hab;
    std::string name;
    double t0 = 0;
    // made again (not saved): its sample's bands
    bool made = false;
    JarRecipe recipe;
};
// the incubator's screen: its page (the flasks, the jars), the flask chosen and the medium's field, the chooser of what goes in (a vial's species
// seen, a flask's culture) and its rows
struct IncubatorChoice { int vial = -1, species = -1, flask = -1, who = -1; std::string line; };
struct IncubatorState {
    int page = 0;                 // 0 the flasks, 1 the jars
    int sel = 0, field = 1;       // the flask, the medium's field (0 the cell .. 4 the salt)
    int jar = 0;
    bool choosing = false;        // what goes in the flask (or the jar), chosen from the rows
    int choiceSel = 0, choiceTop = 0;
    std::vector<IncubatorChoice> choices;
};
// Z-06: the sequencer (game/sequencer.cpp): a species seen in a vial or a flask's read in SEQUENCE_S on the lab's clock (saved: what is on it,
// when its reading began, whether its result was told), its screen's chooser; made again from those: its lineage, its genome's length, its
// name and world, a made species' watermark (its letters drawn as they are read), the result's lines
struct SequencerChoice { FlaskSpecies who; std::string line; };
struct SequencerState {
    bool on = false;
    FlaskSpecies who;
    double t0 = 0;
    bool told = false;
    bool choosing = false;
    int choiceSel = 0, choiceTop = 0;
    std::vector<SequencerChoice> choices;
    bool made = false;
    Lineage line; Microbe species;
    double letters = 0;
    bool artificial = false;      // a people's made species: its genome too regular, a watermark in it
    std::string name, world, mark;
    std::vector<std::string> result;
};
// the tree of kin's screen (game/tree_screen.cpp): every species sequenced and the ship's own, a leaf each in the tree's order (home's genesis
// first, then by genesis, deep branch, a kin cell's boxes, the system's line, the world, its comb), the common ancestor of each with the one
// before it (-1 another genesis: a gap), the geneses counted; its rows (a genesis's head, then its leaves) and the one chosen
struct TreeLeaf {
    std::string key;              // the guide's (`<body key>/M<index>`; the ship's own "SHIP/<k>")
    Lineage line;
    std::string name, world;
    int64_t sx = 0, sy = 0, sz = 0; int body = -1;
    bool drifter = false;
};
struct TreeState {
    std::vector<TreeLeaf> leaves;
    std::vector<double> join;     // join[k]: the common ancestor of leaves k-1 and k (Gyr; -1 a new genesis; join[0] -1)
    int geneses = 0;
    int sel = 0, top = 0;
    std::map<std::string, Lineage> known;   // the lineages read so far (a sequenced species' world drawn once), by key
};
struct LabState {
    double clock = 0;             // the lab's clock (s): real seconds while the game simulates, saved
    Flask flasks[INCUBATOR_FLASKS];
    std::vector<Jar> jars;
    SequencerState seq;           // Z-06
};
// Z-07: the catalogue (game/catalogue.cpp), the guide's page of every species seen (the guide's sheets; a friend's lent ones in cyan): its rows by
// world (a world's head, its species) or by kin (the sequenced in the tree's order, a genesis's head; then those not read), a species' sheet
// beside its form drawn live in a way it was seen in. Nothing of it is saved; its worlds are drawn as it opens (their names and the species'
// read as it draws)
struct CatalogueRow {
    std::string key;              // a species' (`<body key>/M<index>`); a head's its world's key, or "" (its line its words)
    std::string line;             // a head's words on the kin page (its genesis, or the unread)
    bool head = false, lent = false;
    uint64_t genesis = 0;         // a kin page's genesis (its head's colour)
};
struct CatalogueWorld {           // a world as the catalogue reads it: its kind, its list's species and forms
    bool ok = false;
    int kind = 0;
    std::vector<Microbe> species;
    std::vector<Morph> forms;
};
struct CatalogueState {
    int page = 0;                 // 0 the list, 1 a species' sheet
    int order = 0;                // 0 by world, 1 by kin
    int sel = 0, top = 0;         // the row chosen (a species', never a head) and the list's first shown
    int seeing = WS_BRIGHT;       // the sheet's form's way of seeing (V: the next of those it was seen in)
    double t0 = 0;                // its form's clock began (the real clock)
    std::vector<CatalogueRow> rows;
    std::map<std::string, CatalogueWorld> worlds;   // by body key
    std::unique_ptr<Framebuffer> fb;                // the form's slide: a framebuffer of its own (every bank its way's)
    std::vector<uint32_t> rgb;                      // and its colours
    uint64_t still = 0;                             // the still drawn in them (under the electron: its species' and the scale's), 0 none
};
