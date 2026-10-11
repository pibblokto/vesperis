# B-414: a disc in a medieval village, a lattice tower over its ruins

status: resolved 2026-10-10
severity: S2
area: the civilisations (the ages on the ground: towns, works, records, graves; the late ages' buildings)

## Report

The user, on a world they named SLEEPING (Trokliltstom I, sector 15774 196 -7100, a desert): "when I was on a planet with I read as
medieval civilization (arthictecture of buildings, scrolls, codexes, etc, screenshots attached) I found a disk! Immediately broke the
immersion. Another weird thing I found later was this modern/industrial tower that spawned on one of the ruins (it's same planet). [...]
I don't know, maybe I misread but for me it was meant to be super clear that prehistoric have one set of shards, medieval another set,
modern its set, futuristic its and so on. And also building design. For me it all seems like a bug or two bugs (or more) that mixed
buildings and types of shards."

Asked whether a late people's world should keep its history in layers made legible (the 1.3.0 plan: "a modern people's world keeps its
medieval towns as the old cores of its cities and its megaliths in the fields") or show only the age its people reached, the user chose
**one age per world**.

## Cause

SLEEPING's people reached the modern age (medieval to year 1873 of their count, industrial to 2006, modern to 2163; ended 6,460 years
ago; its radio was the signal heard 199 ly off). What the user read as a medieval world was three things of the 1.3.0 design and two
bugs in it:

* **T-02's layers.** A later people's countryside villages and towns took a founding age by the history's shares (a modern people's 55%
  medieval) and stood in rings: the village by "SLEEPING CITY" a medieval core of 22 houses inside 13 industrial rows and villas and 6
  modern suburban houses. A place the history left early stood in the age it was left in; a later people's lone monuments were megaliths
  three times in five, and a capital kept its old time's stones in its fields (the user's HENGE).
* **T-04's dates.** A record was dated over the people's eras and made in its year's way: SLEEPING's 150 were 38 medieval, 29
  industrial and 83 modern. T-03's works were built in their era's way too (a medieval aqueduct and three observatories, an industrial
  dam, three mines and four battlefields, a prehistoric henge), and the graves' years ran over the whole count.
* **The records ignored the building they lay in** (the first bug): C-03's few were drawn by the buildings' weight alone, so the disc
  the user found (year 2012) lay in a medieval house of the old core, a reel (2033) at a medieval stela, wax cylinders and printed pages in
  medieval houses.
* **The late ages read as medieval** (the second bug): A-04's genome kept a people's plan, roof and openings through every age. This
  people's (a polygon plan, conical roofs, triangular windows) made its modern suburban houses polygon towers under cones with pointed
  windows, its industrial villas the same in brick; and the view gave a people one set of colour banks (its last age's), the older
  buildings only the age before's windows, so a medieval core, an industrial villa and a modern suburb looked alike. Six peoples drawn in
  every age showed the same: their modern houses round huts under cones and domes.

The lattice tower was the hamlet's own: a modern radio mast (`BK_MAST`, 170 m) among five modern suburban houses that looked medieval,
at 1.63 N 7.30 W.

## Fix

**A world shows only the age its people reached** (`ageFromOf`, galaxy/peoples.h: the first year of that age in the people's count).
What it built, wrote and buried in its earlier ages is gone; its history (H-02) keeps them and its own histories tell them.

* **Towns** (`galaxy/ruins.cpp`): a place of the plan stands in its people's last age's way whatever age founded it (one layer); a place
  the history left before that age began is not on the ground (`TownHist::gone`: its cell holds nothing, for the view and the finders
  alike); one left early within it stands weathered from when it was left. The countryside is its people's age (no older core, no
  megalith for a lone monument); the old time's stones in a capital's fields are gone.
* **Works** (`galaxy/works.cpp`): every work stands in its people's last age's way (a modern people's industrial dam a modern dam); a
  work of a place left before that age goes with it, a battle of an earlier age left nothing, a later people's old stones (henges) are
  gone, an aqueduct is a medieval or an industrial people's (in its age's way), a necropolis of barrows raises its own age's stelae
  where a prehistoric people raised menhirs.
* **Records** (`galaxy/records.cpp`): every record was made in the years of the age reached (`recordYear`: the later the likelier, as
  before), so its medium, its category's share, its genre, its music, its chart's way and the instrument beside it are that age's. A late
  people tells its earlier ages in **histories** (`galaxy/grammar/history.vg`, `genre.history`, the `@past` directive in
  `galaxy/genres.cpp`): printed pages or a codex of an industrial people, printed pages or a drive of a modern one, a drive or a crystal
  of a futuristic one, a crystal of the far future's, each a run of an earlier era's years from the history with that year's facts and
  ruler and its events in the annal's register ("From a history of the Oup: the Steam Years. Year 1883. ...").
* **Graves** (`galaxy/graves.cpp`): a death in the years of the age reached while the place stood (`Lore::ageFrom`).
* **The machine ages' buildings their age's** (`galaxy/buildings.cpp`, `Layout::formOf`): an industrial or a modern people's villas,
  suburban houses and blocks on the age's plans (a rectangle, an L, a court); their roofs the age's (`ageRoof`: an industrial row's
  hipped slate for a people of domes and cones, a modern house's flat roof for a people of domes, cones, vaults and steps); their
  windows the age's (an industrial people's sash windows, round-headed where its openings were arched, round, pointed or keyholed; a
  modern people's wide panes and bands). The people's own plan, roof and openings stay on its temples, palaces and focal accent (the
  window slots: 0 the dwellings, 1 the public and tall buildings, 2 the people's own; the suburbs, the mills, the malls and the
  lighthouses moved off the people's slot); the age before's window slot is gone with the old cores.
* **Found on the way.** All-industrial towns put more mills side by side, and two stood against a neighbour's doorway (a mill's door was
  on its side, its chimney beside it outside its ground): a mill's doorway is in its front to the street now and its chimney stands in its
  back end. An industrial sermon of the collapse's warning stage found no turn of its exhortation (`sermon.turn` covered the years before
  the warning and after it): the ordinary turns serve the warning too.
* **Saves**: a guide saved before keeps its found shards by their index, their contents new; the game says so once (`records 3`).

## Verified

* **SLEEPING** (probes against the build's objects): its 150 records all modern (printed pages 36, discs 42, reels 23, drives 44, glass
  plates 5; 7 of them histories, one "From a history of the Oup: the Steam Years. Year 1883. ..."), its works all modern (24 quarries, 7
  mines, 4 battlefields, 4 fortified lines, 3 observatories, a dam, a wind farm, 2 wrecks, an antenna field, the last shelter: no henge, no
  aqueduct, no older battle), the villages the user walked all modern (the "SLEEPING CITY" village 19 suburban houses, a temple, its focus
  and stelae, its reservoir), the hamlet by the mast five flat-roofed concrete houses with ribbon windows under it (rendered from inside
  the hamlet); its graves' dead died in 2008-2162 (88 round the user's landings; their names as they were).
* **The ages drawn** (`genome pair`, `genome ages 1 6 30`): the people of SLEEPING's modern town flat-roofed rectangular houses with wide
  windows and its industrial one brick rows, mills and villas with hipped slate and round-headed sash windows (its temple keeping its
  polygon, cone and pointed windows); six peoples near home the same (their modern suburbs no longer huts under cones and domes). The age
  scenes (`scene civilisation_<age>`) keep their vantages: the industrial town stucco and brick with round-headed sash windows, the modern
  one a skyline of ruined towers.
* **The checks**: T-02's old-core check replaced by the one-age check (`towns check`: 39 towns all of their people's age; 40 places of the
  six age worlds left before their people's last age, none standing; 2 left early within it, weathered longer); a guide of `records 2`
  keeps its keys, its 3 shards said new once, saved as `records 3` (one new check, 294 in all). The records' every check at zero over the
  test worlds' 1,242 records with the histories among them; the decoder's history page of the futuristic age world places the Years of
  Sarmarusch and the Engine Years from the histories read (`decoder pages`, its H-06 checks with them). The C-08 worlds' 2,680 and 817
  doorways open (two mills' were blocked before their doors moved: the first run's two failures) and no record on a heap's block (the
  third run's one).
* `unit` 0 failures (3:47), `flow` exit 0 (35.9 s), the regress unchanged (22 hashes), `bench check` within every budget (the people's
  descent 67.0 ms of 80, the capital 8.8 of 16).
