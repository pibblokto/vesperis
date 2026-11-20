# Ideas: the genes (an editor for the living small; the trees by world and by folder)

The user's requests of 2026-11-19, talked through first (R-443, R-444; the Y series in `ITERATION-1.5.0.md`, Phase 5). The asking:
"Would be cool if there was a mechanic for changing genes so that it can have different characteristics, behaviour, etc", "make it smart
but not boring so that you can somehow understand and learn what affects what, can experiment, changes behaviours", and trees "per worlds
with the ability to add certain cultures/organisms if you want to and remove", for "cultures you cultivate that are in custom folders and
for those from each world". Their calls on the sketch: a new machine for editing; versions from the species read and the cultures kept,
an edited organism saved back to a flask as a new culture; a gene moved from a relative and a gene changed by hand ("you can just get one
organism and create tons of variations you like from it"); only world and folder trees; all of it 1.5.0.

## 0. What exists

- **The sequencer** (Z-06) reads a species in 90 s: its alphabet and hand, a marker kin share as far back as their common ancestor, its
  nearest kin read. Nothing else uses what it reads.
- **The tree of kin** is one tree of every species read, the ship's own three at its root, a genesis a clade.
- **Strains** (Z-05) are the only change an organism undergoes: twelve generations at a range's edge move one value (`applyStrain`).
- **The collection** (R-433, R-440) keeps every species and strain a flask grows, in the explorer's folders.
- No gene has a function: a genome is a marker and a length, a pure function of the species and its genesis.

## 1. The genes (R-443)

### 1.1 A species' genes

A species' genes are read from what it already is (its `Microbe` and its `Morph`) and its genesis: nothing of generation 17 moves, no
world, species or form changes, and an unedited line is its species bit for bit. A species has the genes it uses, 8 to 14:

- three **vital** genes (its copying, its membrane, its making) and one to three **quiet** ones (a gene whose loss nothing in the lab
  shows: most of a real genome; Baba et al. 2006's Keio collection, where most single knockouts grow);
- **warmth**, **salt** and **size**, every species';
- **pigment** for a light-feeder, **food** for a chemical-eater (what it burns), **motion** for one that moves, a **sense** for each thing
  it turns to (the light, the food, the field, the warmth; a dark-dweller's flight from the light a light sense turned away; three at
  most), **rest** for one that ends as spores, cysts or the gatherers' fruit, **glow** for a luminous one, **shape** for a small cell.

A genesis has one order of its genes' places (its map, about 16 places, each a gene's number): a gene is the same gene, at the same
number, in all its kin, so what is learned of gene 3 in one species holds in every species of its genesis. Each genesis its own order and
its own code: a new origin of life is a new puzzle. A species shows its genes in its genesis's order, a gap where it lacks one.

### 1.2 The working part and its code

Each gene has a working part, the run of letters its effect lies in (2 to 12 letters of its genesis's alphabet: ACGT, six or eight);
the editor changes those letters. A genesis's code says what they do, the same for all its kin:

- **A letter's weight:** the alphabet's letters ranked by the genesis's hash (home's pinned: A, T, C, G, as a G and C rich stem holds at
  a higher heat: Galtier and Lobry 1997). A sum of weights sets an amount: a warmth's range, a salt's band, a size, a motion's power.
- **A table:** a short run of letters (two or three) chooses one of a set: a pigment, a food, a motion's kind, a sense's target, a shape.
  Every choice is in the table at least once; most runs are synonyms.
- **The wild letters** are the species' own: chosen so the code gives its values, a heat-lover's warmth heavy in the heavy letters, a
  cold-lover's light (two kin compared teach the rule). An edit moves a value from the species' own by the change in the code (a warmth
  3 C a weight; a size by 2 to the third of the change), so the wild value is exact and every edit is a step a player can learn.

Changing a vital gene's working part kills the line; a quiet gene's changes nothing seen. The working parts' lengths: warmth 12, salt 6,
motion 6, size, rest, glow and shape 4, food 3, pigment and a sense 2, vital and quiet 8. A gene cut loses its work: its warmth only round
its best, fresh water, half its size, no pigment, food, motion, sense, rest or glow, a sphere (Y-02 as built).

### 1.3 What each gene does, and where it is seen

| Gene | Its code | What changes | Seen |
| --- | --- | --- | --- |
| Warmth | 12 letters' weight | its range, 3 C a weight (its cell's limits) | the incubator: GROWS, TOO HOT, TOO COLD |
| Salt | 6 letters' weight in bands | fresh, the sea's, brine (Lanyi 1974's acid proteins) | the incubator's salt |
| Pigment | 2 letters, a table | the light that feeds it, its colour (Man et al. 2003: one letter) | the incubator's light, the slide |
| Food | 3 letters, a table | what a chemical-eater burns, or nothing | the incubator's energy |
| Motion | 2 letters' kind, 4's power | still or moving, its kind among its plan's, its pace and flagella | the slide |
| Sense | 2 letters, a table | to the light, away from it, to the food, along the field, the warmth, or nothing | the slide's L, F, M |
| Rest | 4 letters' weight | spores, cysts or the gatherers' fruit at its end, or dying | the incubator's decline |
| Glow | 4 letters' weight | glowing at a share of its crowd (0.5 to 30%), or not | the flask in the room |
| Shape | 2 letters, a table; 2 its length | a small cell's plan: sphere, rod, curved, spiral (Jones 2001, Ausmees 2003) | the slide |
| Size | 4 letters' weight | its cells' width, by two to the third a weight (who eats whom) | the slide, two in a flask |
| Vital | 8 letters | changed or cut: it does not live | its flask dead at once |
| Quiet | 8 letters | nothing seen | none |

A warmth sense is read and kept, but the slide has no gradient of warmth: nothing shows it.

### 1.4 How the explorer learns what a gene does

Nothing is named until seen. A gene is NOT KNOWN on the map until one of these shows it, and what showed it is kept (the guide's `gene`
lines, by genesis and number):

1. **An edit seen against its parent.** A line with one gene changed, grown or looked at where the change shows: its flask grows where
   its parent would die (warmth, salt), takes a light or a food its parent cannot (pigment, food), glows or not, rests or dies at its end;
   its drop moves, answers the light, the food or the field, or shows another shape or size. Said as it is seen ("FLASK 5: IT GROWS AT 30
   C - ITS PARENT DIES THERE", "GENE 3 IS ITS WARMTH"). Several genes changed at once: the candidates are said, nothing learned ("ITS
   WARMTH CHANGED: GENE 3, 7 OR 9"), as one variable at a time is how it is known.
2. **A vital gene:** the line dead in its first flask ("IT DID NOT LIVE: GENE 2 IS VITAL").
3. **A strain read:** the sequencer reads a strain's genome; the one gene that differs from its parent's is the one its move was in
   (its highest warmth up: its warmth gene).
4. **A quiet gene:** changed, grown past its lag and looked at in a drop, nothing different: NOTHING SEEN.

N names a gene (the guide's, of its genesis). Two kin's genes compared on the editor (where their letters differ, a share alike, beside what
their sheets say: a hint, never an answer) come with Y-03's genes from others. As built (Y-02): what is seen is said first ("FLASK 2: IT
GROWS AT 29 C - ITS PARENT DIES"), what it says of the genes after it ("GENE 8 IS ITS WARMTH"; "ITS WARMTH: GENE 2 OR 8").

### 1.5 The gene editor (Y-02)

A machine of its own in the laboratory (`EDITOR`, E at it; its row, its stand, its screen, its light), on the bow bench beside the
sequencer where the plan's rows give it room.

```
THE GENE EDITOR
LIGHTUS IGNORUS, FROM FLASK 3
 1   2   3   4   5   6   7   8   9  10  11
[?] [?] [?] [?] [?] [ ] [?] [F] [?] [?] [?]
GENE 3, 2,140 LETTERS: WHAT IT DOES NOT KNOWN
ITS WORKING PART:  A C C G T A G G A T C A
                         ^
- GENE 8 CUT OUT                  ITS FLAGELLA
ARROWS A GENE  ENTER ITS LETTERS  C CUT  BKSP UNDO
N NAMES THE GENE  W WRITES IT INTO A FLASK
```

- **What it takes:** a culture whose species the sequencer read: a flask's or a kept one (the choosers' tab THE CULTURES; a vial's
  species goes to a flask first). One not read is refused: READ IT ON THE SEQUENCER FIRST.
- **The map:** its genes by number in two columns of nine, each NOT KNOWN, NOTHING SEEN, its words (ITS WARMTH ...) or its name, a dash
  where its kin have one it lacks. The arrows a gene; Enter its working part, Left and Right a letter, Up and Down the letter there through
  the alphabet (no letter's key: P and T are the game's own); C cuts the gene out (again: back in); Backspace undoes a gene's changes. The
  edits listed (- cut, = changed by hand; + moved in and ~ switched to come). A gene known shows its value as its letters give it.
- **Writing:** W writes the edit in EDIT_S (120 s) on the lab's clock, the screen open or not (the sequencer's way), refused with nothing
  changed or no flask empty; then the line goes into the first empty flask, in its source's medium, a new culture (with none then, into
  the collection); the collection keeps it as it grows (R-433). Said and logged (EDIT). An edited line is edited again the same way: a
  line of a line (a gene its parent cut stays cut).
- **A line** is its parent's genes with the edits, a strain of its species: its key `<body>/M<index>/S<n>` (the number its species'
  next, strains and edits in one row), drawn E n where a strain's is S n; its name (N on its flask) or its parent's words and E n.
  Everything reads it as a strain is read: its flask's culture (its values), the microscope's drop (its form, motion and answers), the
  sequencer (its genes, what differs from its parent), the collection, the trees (a twig of its parent).

### 1.6 Genes from others (Y-03)

- **M moves a gene in** from a species read or a culture kept of the same genesis: a gene it has (that gene's letters become the donor's,
  its value the donor's), or one at a gap (it gains it). Across geneses refused: ANOTHER ALPHABET, or THE OTHER HAND (a mirrored life).
- **The price:** a gene not its own slows its growth by its donor's distance on the tree (the same world 3%, its system 6%, another star
  10%; Bentley et al. 1990's burden); a motion's power past its own and a glow cost too. Its doubling shows it; beside a rival in a flask
  it can lose (Gause's rule, Z-05).
- **D duplicates a gene** (Ohno 1970): a second copy at the next free place of its kind (a second sense), then changed by hand.
- **What a gene needs:** a sense answers nothing without motion; a pigment feeds only a light-feeder and colours the rest; a food gene
  feeds only a chemical-eater. Found by trying, said when seen.
- On the trees a moved gene is a thin line from its donor's leaf to the line's twig.

### 1.7 The switches (Y-04)

S sets a gene's switch (its promoter: Jacob and Monod 1961): ALWAYS, NEVER, IN THE LIGHT, IN THE DARK, WHEN CROWDED, WHEN STARVING, WHEN
COLD. The wild ones: a glow's WHEN CROWDED (its quorum), a rest's WHEN STARVING, the rest ALWAYS. A gene off costs nothing, so a switch
is the clever edit: swimming only when starving grows faster while fed; a glow in the dark lights a dark flask; spores when cold sleep
through the cryostat. The culture's segments change where the switch's condition does (the medium's light, its warmth against the line's
best, nothing to eat or its crowd's hold, its crowd's share), closed-form as every segment; a drop shows the line as its flask has it.

### 1.8 What stays fixed

- How it lives (light, chemistry, the dead, grazing, hunting) and its plan's family (a small cell, a large cell, a colony) never change by
  an edit: a shape edit moves among the small cells' plans, a motion among its plan's kinds.
- A line's values stay in its cell's range: the warmth within its chemistry's limits, the size within its family's.
- The worlds stay as generated: a line lives only aboard (no culture leaves the ship: 1.5.0's rule).

### 1.9 The record and the save

- **The guide:** a line's `strain` line (its parent, SM_EDIT and its edits: "3=ACCG;8=-"), `gene` (a gene learned: its genesis and its
  number, its kind, how), names (`name`, a line's and a gene's `G<genesis>/<number>`). A line's answers on a slide go to no sheet.
- **The save:** the editor (`editor`, `editoredits`, `editorwrit`, `editorfrom`: what is on it, its edits, the writing's start), the
  flasks' and the collection's lines (`FlaskSpecies` keeps its line). Saves of before load with none.

## 2. The trees by world and by folder (R-444, Y-01)

```
THE TREE OF KIN     < GREENDAY >        2 OF 5
MIRRORED, FOUR LETTERS
       .----------- LIGHTUS FOBUS LONGUS
   .---+
   |   '----------- LIGHTUS FOBUS
 --+             '. LIGHTUS FOBUS, EDITED 1
   '--------------- LIGHTUS IGNORUS
                 '. LIGHTUS IGNORUS, STRAIN 1
 300   200   100   0 MILLION YEARS AGO
4 MORE SEEN HERE, NOT YET READ
A PUTS A LAB CULTURE ON   BACKSPACE TAKES IT OFF
```

- **Pages, Left and Right:** THE SHIP (its own three, home's life), a page for each world with a species read or a line of one, then a
  page for each of the collection's folders (UNSORTED, then the explorer's). No tree of everything: a leaf's nearest kin read anywhere is
  still told under it, and the catalogue's kin page lists every species read in the tree's order.
- **A world's page:** its species read in the tree's order; those seen there and not read counted under it; the lab's lines of its
  species (strains, edited lines) as twigs at today's edge under their parents, a line's line under its line. A line goes on its world's
  tree as it arises; Backspace takes one off, A puts one taken off back (a chooser of that world's lines); the wild species stay.
- **A folder's page:** what the folder holds (a pair two leaves), read or not, each where its kin put it; a line's parent drawn where the
  folder holds both. The folder is sorted on the microscope's lists (M, C, Backspace), the tree with it.
- **Each page its own years:** the axis to its oldest common ancestor (a genesis's stem to the edge, its beginning in its head's words),
  so a world's leaves 300 million years apart are drawn apart.
- The room's screen shows the page last shown; Enter sets a leaf's world's star the remote target, as now; the LIFE page's geneses
  count stays the tree's words (`LIFE BEGAN AT LEAST N TIMES`).

## 3. How it plugs in

- `life/genes.h`, `.cpp` (new): the families, a genesis's map and code, a species' genes and wild letters, a line's values from its edits
  (`CultureKind`, `Morph`, the slide's kind); what an edit changes (for the said and the learned).
- `game/editor.cpp` (new): the machine's screen, chooser, writing; `lab_state.h`'s `EditorState`; the device's row (`cabin.cpp`), its
  fixtures (`ship_plan.cpp`), the tour's stand. `game/tree_screen.cpp`: the pages and twigs.
- `game/incubator.cpp`, `microscope.cpp`, `sequencer.cpp`, `lab_lists.cpp`: a line read as a strain is; the observations learned.
- The harness: `lab genes` (new: a world's species' genes, the code, an edit's values), `lab tree` by pages; the unit's `genes` group.

## 4. Sources

Ausmees, Kuhn and Jacobs-Wagner 2003, crescentin curves Caulobacter (Cell 115); Baba et al. 2006, the Keio collection of single
knockouts (Mol Syst Biol 2); Bentley et al. 1990, a plasmid's metabolic burden (Biotechnol Bioeng 35); Galtier and Lobry 1997, G and C
in RNA stems and growth temperature (J Mol Evol 44); Jacob and Monod 1961, the lac operon (J Mol Biol 3); Jones, Carballido-Lopez and
Errington 2001, MreB and a rod's shape (Cell 104); Lanyi 1974, the halophiles' acid proteins (Bacteriol Rev 38); Man et al. 2003, a
proteorhodopsin's colour by one residue (EMBO J 22); Ohno 1970, evolution by gene duplication.
