// Z-04: the laboratory's state (game/microscope.cpp, microscope_draw.cpp; R-425, docs/archive/ideas/ITERATION-1.5.0.md Z-04): the microscope's page
// (the cold store's vials and the flasks, or the slide on its stage), the vial or the flask there and what it is made of (its worlds' systems
// drawn once, their biotas and forms, its sample or its culture's drop and its mount: made again whenever it is put on the stage), what was
// done to the slide and its clock, the view (its way of seeing, magnification, place, focus, the electron's turn and tilt, a copy tracked), the
// sheet shown. Nothing of it is saved: what was seen of a species is the guide's (`microbe` lines). Z-05: the incubator's flasks, on the
// lab's clock (the drones' kind: real seconds while the game simulates), and the incubator's screen; their save lines
#pragma once
#include "life/slides.h"
#include "life/cultures.h"
#include "life/sequences.h"
#include "life/genes.h"
#include "ship_plan.h"
#include "galaxy/system.h"
#include "surface/bestiary.h"
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

// R-432: a list in folders (the microscope's vials by world, its cultures by folder; the incubator's and the sequencer's choosers): each folder its
// name, its second line and its rows (a vial's place in the cold store, a flask's number, a kept culture's place in the collection, a chooser's
// row), a world's key where it is a world's, a collection's folder's id (0 UNSORTED; FOLDER_WORLD and FOLDER_FLASKS the others)
constexpr int FOLDER_WORLD = -1, FOLDER_FLASKS = -2;
struct LabFolder {
    std::string name, line, key;
    int id = FOLDER_WORLD;
    bool foreign = false;         // its world's name a friend's (cyan)
    std::vector<int> rows;
};
// a chooser's folders and its page: the folders shown, or the one open (its row chosen the chooser's `choiceSel`, an index into its rows)
struct ChooserPage {
    std::vector<LabFolder> folders;
    bool open = false;
    int folder = 0, top = 0;
    // R-441: its folders in two tabs (`chooserTabs`): the vials' worlds (0) and the cultures (1, the flasks' and the collection's), `folders` the
    // one shown
    bool tabbed = false;
    int tab = 0;
    std::vector<LabFolder> tabs[2];
};

struct Input; struct RGBCanvas;
std::string labFolderWhere(const std::string& key);   // a world's place in a folder's head ("  (+195 +0 +46 / 3)"), lab_lists.cpp
// a chooser's keys over its folders: the row chosen (Enter on it) as its index in the chooser's rows, -1 none yet, -2 Esc on the folders
int chooserKeys(const Input& in, ChooserPage& P, int& sel);
// its page drawn in a panel's box: the folders (a name and its count each) or the open one's rows (`lines`, by the chooser's row)
void drawChooser(RGBCanvas& c, int x0, int y0, int x1, int y1, ChooserPage& P, int sel, int& top, const std::vector<std::string>& lines, uint32_t colour);
// R-441: its folders put in the two tabs, the one asked for shown (the other where it holds none); a tab shown
void chooserTabs(ChooserPage& P, int tab);
void chooserTab(ChooserPage& P, int tab);
// B-442: a chooser's rows that read the same in one folder (`keys`) told apart: each its form's colour and motion (`traits`) after its look, then
// its place among those still alike; the rows' words each its look and the rest
void apartRows(std::vector<std::string>& looks, const std::vector<std::string>& rests, const std::vector<std::string>& keys, const std::vector<std::string>& traits);

// the magnifications the zoom steps through: the light ways' objectives (x40, x100, x400, x1,000) with the camera's steps between and on the oil
// one (x2,000, x4,000); the electron's x200 (a large cell whole) to x40,000
constexpr int SCOPE_LIGHT_STEPS = 7, SCOPE_ELECTRON_STEPS = 7;
constexpr double SCOPE_LIGHT[SCOPE_LIGHT_STEPS] = {40, 100, 200, 400, 1000, 2000, 4000};
constexpr double SCOPE_ELECTRON[SCOPE_ELECTRON_STEPS] = {200, 500, 1000, 3000, 10000, 20000, 40000};

struct MicroscopeState {
    int page = 0;                 // 0 the lists (R-432: the vials by world, the cultures by folder), 1 the slide
    int sel = 0, top = 0;         // the open folder's row chosen (an index into its rows) and the first one shown
    int vial = -1;                // the cold store's place on the stage (-1 none)
    int flask = -1;               // Z-05: or the flask whose drop is (-1 none)
    StarSystem sys; int body = -1;
    std::shared_ptr<const SystemLife> life;
    std::vector<Morph> forms;
    // Z-05: the slide's species (a vial's its world's list, a flask's its one or two with their strains' moves), each its guide key and its world;
    // the HUD's two lines for what is on the stage (a vial's world and its line, a flask's number and its stage)
    std::vector<Microbe> species;
    std::vector<std::string> keys, worlds;
    std::vector<std::string> wild;   // J-12: a vial's seeded line's record key in the guide's `wildFound` ("<world key>/L<k>"), "" for the rest
    std::string title, line;
    int lineState[2] = {-1, -1};  // Y-04: a flask's switched lines' state as their drop was taken (-1 none)
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
    // R-432: the lists' tab (0 the vials, a folder a world; 1 the cultures: the flasks, R-433's collection by folder), a folder open or the folders
    // shown, the folder chosen and the first shown; R-433: a kept culture on the stage (its place in the collection, -1 none), M's chooser of
    // the folder it moves to
    int tab = 0;
    bool open = false;
    int folder = 0, folderTop = 0;
    int kept = -1;
    bool moving = false; int moveSel = 0;
    bool copying = false;         // R-440: M's chooser copying (C), the culture kept where it is too
    // R-438: the stage's warmth being typed (H) and what is typed so far; the drop's liquid range (its chemistry's cell's)
    bool typing = false; std::string typed;
    double stageLo = -20, stageHi = 125;
    // R-437: what each kind of the slide was told doing (its SlideAnswerBits, SCOPE_LOOKED once looked at), when each experiment began (by
    // SlideDo, on the slide's clock; -1 none going), whether the view answered it and whether its while was judged (the kinds in view then
    // kept as not answering it), the lines waiting their turn on the HUD, when the magnet was last put or turned (its field's lines drawn a
    // while), the sheets' answers changed since the guide was last saved (saved two seconds apart at most)
    std::vector<uint32_t> told;
    double tried[SD_COUNT] = {-1, -1, -1, -1};
    bool answered[SD_COUNT] = {}, judged[SD_COUNT] = {};
    std::vector<std::string> said;
    double fieldShown = -100;
    bool answersDirty = false; double answersSavedAt = -100;
    // B-502 (KI-437): under the electron the slide is kept while the stage is still (`microscopeElectron`): each mount staged counted, what
    // drew the slide kept (the mount's count, the view, the focus, the things done to it, the frame's size), the slide's pixels and depths
    uint64_t staged = 0;
    struct ElectronKept {
        uint64_t staged = ~0ull, seed = 0; size_t events = 0;
        double cx = 0, cy = 0, um = 0, t = 0, turn = 0, tilt = 0, focus = 0;
        int w = 0, h = 0, step = 0;
    };
    ElectronKept keptAt;
    std::vector<Pix> keptIdx;
    std::vector<float> keptInvz;
};
constexpr uint32_t SCOPE_LOOKED = 1u << 31;   // R-437: a kind's told bits once it was first in view (its state as found, not said)

// Z-05: the incubator (game/incubator.cpp, incubator_draw.cpp): twelve flasks, each a medium and a culture of one or two species put in it on the
// lab's clock
// a flask's species: its world (a vial's star sector and body), its index in that world's list, its strain (0 the wild one: the guide's `strain`
// lines number the rest)
struct FlaskSpecies {
    int64_t sx = 0, sy = 0, sz = 0;
    int body = -1, species = -1;
    int strain = 0;      // its strain now
    int strainIn = 0;    // the strain it was put in as (the flask's strains after it are its events)
    bool sameSpecies(const FlaskSpecies& o) const { return sx == o.sx && sy == o.sy && sz == o.sz && body == o.body && species == o.species; }
};
// the moments a flask's culture told (its HUD lines, the log's): its spores woke, its crowd glowing; R-433: FT_KEPT << k, its k-th species kept
// in the collection as it grew (cleared by a strain of it arising: the strain is kept as it grows on)
enum FlaskTold : uint32_t { FT_WOKEN = 1, FT_GLOW = 2, FT_KEPT = 4 };
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
    // Y-02: a line the gene editor wrote (its line's record SM_EDIT), compared with its parent as it grows and as its drop is looked at: its
    // parent's kind and form, its genesis, the genes its edits changed (and their kinds, its genesis's map's); what of it was told (a GeneKind's
    // bit: said once a flask); Y-03: what its genes need told (a place's bit)
    bool edited[2] = {false, false};
    CultureKind parentKind[2]; Morph parentForm[2];
    uint64_t genesis[2] = {0, 0};
    std::vector<int> changed[2], changedKind[2];
    uint32_t geneTold[2] = {0, 0};
    uint32_t needTold[2] = {0, 0};
    // Y-04: its lines' genes and their parents' (a switched line as its state has it: its drop, what it shows against its parent); the places
    // whose edit is a switch alone (a gene as its parent's while its switch is on)
    MicrobeGenes genes[2], parentGenes[2];
    std::vector<int> switchOnly[2];
};
// J-01: what a flask shows of its world genes now (its screen's lines, its glass in the room, what an edited line is learned by), each look's
// strength 0 to 1 and the species that makes it (0, 1; -1 none): bubbles of a growing culture's gas, its water dyed (the dye stays after its
// cells die), a scum of floating cells, its slime, a sediment of its shells, its cells in clumps, overgrown (packed with cells; tissue where
// they bind), frozen by a culture's ice, the other poisoned, genes passed to the other
struct FlaskShow {
    int gas = WG_NONE, gasWho = -1; double bubbles = 0;
    int dye = WD_NONE, dyeWho = -1; double dyed = 0;
    double scum = 0; int scumWho = -1;
    double slime = 0; int slimeWho = -1;
    int shell = SHL_NONE, shellWho = -1; double sediment = 0;
    double clumps = 0; int clumpWho = -1;
    bool overgrown = false, tissue = false; int overWho = -1;
    bool frozen = false; int iceWho = -1;
    int poisoned = -1, passedTo = -1;
};
// the incubator's screen: the flask chosen and the medium's field, the chooser of what goes in (a vial's species seen, a flask's culture) and its
// rows
struct IncubatorChoice { int vial = -1, species = -1, flask = -1, who = -1; std::string line; int kept = -1; std::string lineKey; };   // R-433: or a kept culture; J-03: or a vial's seeded line (its key)
enum IncubatorPick { PICK_NONE = 0, PICK_NAME, PICK_KEEP };
struct IncubatorState {
    int sel = 0, field = 1;       // the flask, the medium's field (0 the cell .. 4 the salt)
    bool choosing = false;        // what goes in the flask, chosen from the rows
    int choiceSel = 0, choiceTop = 0;
    std::vector<IncubatorChoice> choices;
    ChooserPage pages;            // R-432: the chooser's folders (a world's, the flasks', the collection's)
    bool typing = false; std::string typed;   // R-435: the flask's warmth being typed (its digits) and what is typed so far
    // R-440, R-442: what N names (each species, a strain) or K keeps (each of two alone, both) of the flask chosen, from rows over its page; R-441:
    // the chooser's tab last shown (0 the vials, 1 the cultures)
    int pick = PICK_NONE, pickSel = 0;
    std::vector<std::string> pickLines, pickKeys;
    std::vector<char> pickStrain;
    int lastTab = 0;
};
// Z-06: the sequencer (game/sequencer.cpp): a species seen in a vial or a flask's read in SEQUENCE_S on the lab's clock (saved: what is on it,
// when its reading began, whether its result was told), its screen's chooser; made again from those: its lineage, its genome's length, its
// name and world, a made species' watermark (its letters drawn as they are read), the result's lines
struct SequencerChoice { FlaskSpecies who; std::string line; std::string wild, wildKey; };   // J-03: a vial's seeded line, its genes changed in the wild ("23, 29"); J-12: its record's key (`wildFound`)
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
    ChooserPage pages;            // R-432: the chooser's folders (a world's, the flasks', the collection's)
    int lastTab = 0;              // R-441: its tab last shown
    std::string wild;             // J-03: a seeded line from a vial: its genes changed in the wild ("" none; saved, `seqwild`)
    std::string wildKey;          // J-12: its record in the guide's `wildFound` ("<world key>/L<k>"; saved, `seqwildkey`): read once its reading ends
};
// the tree of kin's screen (game/tree_screen.cpp): every species sequenced and the ship's own, a leaf each in the tree's order (home's genesis
// first, then by genesis, deep branch, a kin cell's boxes, the system's line, the world, its comb), the common ancestor of each with the one
// before it (-1 another genesis: a gap), the geneses counted; its rows (a genesis's head, then its leaves) and the one chosen
struct TreeLeaf {
    std::string key;              // the guide's (`<body key>/M<index>`; the ship's own "SHIP/<k>"; Y-01, a strain's `.../S<n>`)
    Lineage line;
    std::string name, world;
    int64_t sx = 0, sy = 0, sz = 0; int body = -1;
    bool drifter = false;
    // Y-01: a lab's line (a strain: its species' lineage and world): its depth under its species (0 standing in its species' place, a
    // folder's strain kept without it) and the key it hangs under
    bool lab = false; int depth = 0; std::string parent;
    std::string where;            // B-497: an unnamed world's place ("(+195 +0 +46 / 3)"), its words where its world is said
};
// Y-01 (R-444): a page of the tree: the ship's own, a world's (its body key), a folder's (its id); its name in the head
enum TreePageKind { TP_SHIP = 0, TP_WORLD, TP_FOLDER };
struct TreePage { int kind = TP_SHIP; std::string key; int folder = 0; std::string name; };
struct TreeState {
    std::vector<TreeLeaf> leaves; // every species read and the ship's own, in the tree's order (the LIFE page's, the sequencer's words)
    std::vector<double> join;     // join[k]: the common ancestor of leaves k-1 and k (Gyr; -1 a new genesis; join[0] -1)
    int geneses = 0;
    // Y-01: its pages and the one shown: its species and their lines in its rows' order, each species' common ancestor with the one before it
    // (-1 a genesis's first; a line's -1, its depth saying it is one), what is there and not read; A's chooser of a world's lines taken off
    std::vector<TreePage> pages; int page = 0;
    std::vector<TreeLeaf> shown; std::vector<double> shownJoin; int unread = 0;
    bool picking = false; int pickSel = 0; std::vector<std::string> pickKeys;
    int sel = 0, top = 0;         // the row chosen on the page shown
    std::map<std::string, Lineage> known;   // the lineages read so far (a sequenced species' world drawn once), by key
    std::map<std::string, Morph> forms;     // B-443: their forms (a leaf's look), drawn with their lineages
    std::map<std::string, int> kinds;       // Y-01: their worlds' kinds by body key (a world's page's name where it has none)
    std::vector<std::pair<int, int>> donors; // Y-03: a moved gene's line on the page: its donor's row in `shown`, the line's
};
// R-433: the culture collection (game/lab_lists.cpp): every species and strain grown in a flask, kept frozen as labs keep their stocks, without
// limit and after its flask is emptied: its world's species and its strain (the one it goes in as), the medium it grew in and its world's (AS
// ITS WORLD), the explorer's folder it is in (0 UNSORTED), when it was kept; its form's plan made again (not saved: its row's words). The
// explorer's folders, each its id (never reused while it stands) and its name; the save's `kept` and `keptfolder` lines
struct KeptCulture {
    FlaskSpecies who;
    Medium medium, asWorld;
    int folder = 0;
    double t = 0;
    int plan = -1;
    // B-441: grown on its partner's gift (alone in its medium it starves): kept with it, staged and put in with it; R-440: a pair kept by hand
    bool paired = false; FlaskSpecies partner;
    int partnerPlan = -1;   // its plan, looked up once (the list's words)
};
// R-440: a culture kept by hand (K on the microscope's slide or a flask), a copy: what is kept (one species, or a pair), the folder chooser's row
// (UNSORTED, the explorer's folders, A NEW FOLDER) over the screen it was asked on; `pending` while a new folder's name is typed for it.
// R-445: `write` when the folder is for the line the gene editor is about to write (`what` is then its parent)
struct KeepPick {
    bool open = false, pending = false, write = false;
    KeptCulture what;
    int sel = 0;
};
struct CultureFolder { int id = 0; std::string name; };
bool sameCulture(const KeptCulture& a, const KeptCulture& b);   // R-440: one species alone, or the same two either way round (lab_lists.cpp)
// Y-02: the gene editor (game/editor.cpp): a culture on it (a flask's or a kept one, read on the sequencer: its species and line, the medium it
// grew in and its world's, where it came from), the edits made (each gene once), the gene chosen (its place on the map) and its working part's
// letter, the writing (on the lab's clock, EDIT_S; the edits written), its chooser (the cultures' rows by folder); made again (not saved): its
// genes, its genesis's code, its kind and form, the same with its edits, its words
constexpr double EDIT_S = 10;    // the writing of a line: ten seconds on the lab's clock (R-449; Y-02 had two minutes)
struct EditorChoice { FlaskSpecies who; Medium medium, asWorld; std::string line, from; bool read = false; };
struct EditorState {
    bool on = false;
    FlaskSpecies who;
    Medium medium, asWorld;
    std::string from;
    std::vector<GeneEdit> edits;
    int sel = 0;                  // the place chosen on the map (0 .. GENE_PLACES - 1)
    bool letters = false; int cursor = 0;
    bool writing = false; double t0 = 0;
    std::vector<GeneEdit> writ;
    int writeFolder = 0; std::string writeName;   // R-445: the folder the line being written goes in, and its name ("" none)
    bool choosing = false; int choiceSel = 0;
    std::vector<EditorChoice> choices;
    ChooserPage pages;
    bool made = false;
    Genesis genesis; GeneCode code; MicrobeGenes genes, editedGenes;
    CultureKind kind, editedKind; Morph form, editedForm;
    std::string name;
    std::vector<std::pair<std::string, std::string>> said;   // what the genes showed and what it says of them, each waiting its turn on the status
    // Y-03: its kin beside it (K: a species read or a culture kept, of its genesis), what M moves a gene in from: its genes, kind, form and words,
    // its genome's share alike; what the chooser chooses (EC_CULTURE what to edit, EC_KIN a kin, EC_MOVE a kin to move the gene chosen from)
    bool kinOn = false; FlaskSpecies kin; std::string kinFrom, kinName; double kinAlike = 0;
    MicrobeGenes kinGenes; CultureKind kinKind; Morph kinForm;
    int choosingFor = 0;
    // R-450: the guide's choices with the letters open, what each letter of the alphabet at the cursor would make of the gene (made again as the
    // edits, the gene or the cursor change: `editorLetterChoices`)
    std::string guideKey; std::vector<std::string> guideIf;
};
enum EditorChoosing { EC_CULTURE = 0, EC_KIN, EC_MOVE };
struct LabState {
    double clock = 0;             // the lab's clock (s): real seconds while the game simulates, saved
    Flask flasks[INCUBATOR_FLASKS];
    SequencerState seq;           // Z-06
    EditorState edit;             // Y-02
    std::vector<KeptCulture> kept;          // R-433
    std::vector<CultureFolder> folders;
};
// Z-07: the catalogue (game/catalogue.cpp), the guide's page of every species seen (the guide's sheets; a friend's lent ones in cyan): its rows by
// world (a world's head, its species) or by kin (the sequenced in the tree's order, a genesis's head; then those not read), a species' sheet
// beside its form drawn live in a way it was seen in. Nothing of it is saved; its worlds are drawn as it opens (their names and the species'
// read as it draws)
// J-12: a row's kind: a species seen under the microscope, a seeded line found in the wild, an animal species seen on the ground
enum CatalogueRowKind { CR_SPECIES = 0, CR_WILD, CR_ANIMAL };
struct CatalogueRow {
    std::string key;              // a species' (`<body key>/M<index>`); a head's its world's key, or "" (its line its words); J-12: a wild
                                  // line's species' (its sheet's), an animal's (`<body key>/A<index>`)
    std::string line;             // a head's words on the kin page (its genesis, or the unread)
    bool head = false, lent = false;
    uint64_t genesis = 0;         // a kin page's genesis (its head's colour)
    int kind = CR_SPECIES;        // J-12
    std::string wild;             // J-12: a wild line's record (the guide's `wildFound` key, "<world key>/L<k>")
};
// J-12: a seeded line found in the wild: the world it went down on, its line's key, its species' key (its sheet), the line as the wild left it
// (its form, its values), what the wild changed in it ("23, 29"), whether the sequencer read it there, the guide's clock when it went down
struct CatalogueWild { std::string world, line, species, drift; Morph form; Microbe s; bool read = false; double clock = 0; };
// J-12: an animal species (its words, its way and its company). B-500: its species as the ground draws it, its world's two hide colours, where
// it lives (0 the land, 1 the air, 2 the waters) and its fate there as its world's record holds it at the guide's clock (AnimalMark bits)
struct CatalogueAnimal { std::string words; int way = 0, social = 0; Species sp; RGB hide[2]; int lives = 0; uint32_t fate = 0; };
struct CatalogueWorld {           // a world as the catalogue reads it: its kind, its list's species and forms
    bool ok = false;
    int kind = 0;
    std::vector<Microbe> species;
    std::vector<Morph> forms;
    // J-12: a seeded world's made kind's name ("" none), the lines that took there, and its natives died out (by index: the words of the line
    // that did it, and how), at the guide's clock as the catalogue opened; its animals, made once one of them was seen
    std::string made;
    int took = 0;
    std::map<int, std::pair<std::string, int>> diedOut;
    std::vector<CatalogueAnimal> animals; bool animalsMade = false;
    Genesis genesis;              // B-500: its life's genesis (its animals' kin: the kin page's head they go under)
};
// J-12: the LIFE page's seeded worlds, made as it opens: how many, the lines sent and those that took, the made kinds now (a name a world),
// the explorer's species seen that died out (the catalogue's DIED OUT rows) and the animals seen gone
struct CatalogueCounts { int worlds = 0, sent = 0, took = 0, diedOut = 0, gone = 0; std::vector<std::string> kinds; };
struct CatalogueState {
    int page = 0;                 // 0 the list, 1 a species' sheet
    int order = 0;                // 0 by world, 1 by kin
    int sel = 0, top = 0;         // the row chosen (a species', never a head) and the list's first shown
    int seeing = WS_BRIGHT;       // the sheet's form's way of seeing (V: the next of those it was seen in)
    double t0 = 0;                // its form's clock began (the real clock)
    std::vector<CatalogueRow> rows;
    std::map<std::string, CatalogueWorld> worlds;   // by body key
    std::map<std::string, CatalogueWild> wilds;     // J-12: by their records' keys
    CatalogueCounts counts;                         // J-12: the LIFE page's
    std::unique_ptr<Framebuffer> fb;                // the form's slide: a framebuffer of its own (every bank its way's)
    std::vector<uint32_t> rgb;                      // and its colours
    uint64_t still = 0;                             // the still drawn in them (under the electron: its species' and the scale's), 0 none
    int animalPx = 0;                               // B-500: an animal's picture's pixels as last drawn (the unit's)
};
