# B-443: the tree's leaves unnamed

status: resolved 2026-11-19 (before the 1.5.0 release)
severity: S4
area: the tree of kin and the sequencer (`game/tree_screen.cpp`: `treeLeaves`, `treeLeafLine`, `treeWords`; `game/sequencer.cpp`:
`kinDraw`, `formOfKey`, `sequencerMake`, `sequencerResult`; `game/ui.cpp`: `fitHead`)

## Report

KI-447, found in B-442 (2026-11-19): "The tree of kin's leaves and the sequencer's title still say UNNAMED for a species with no
name (B-442 gave the lab's rows its look): two of a world read alike on the tree; its look needs each leaf's form, its world drawn."
The user, the same day: "Commit and work on KI-447 please". Their own guide has no species read yet (the tree holds the ship's
three), so it was measured on forty worlds' species, every one read.

## Cause

Z-06's tree (`treeBuild`) and the sequencer (`sequencerMake`, `sequencerResult`) said a species by `flaskSpeciesName`: its name, else
UNNAMED. B-442 gave the lab's rows a species' look from its form, but the tree kept each species read by its lineage alone
(`tree.known`, its world drawn once), never its form. Over forty worlds, 138 leaves of species: all 138 UNNAMED, 98 drawn alike
another of their own world ("UNNAMED, UNKNOWN"); the sequencer's title "UNNAMED OF AN UNNAMED WORLD"; the log's "KIN: UNNAMED OF
UNKNOWN AND UNNAMED OF UNKNOWN, A COMMON ANCESTOR ABOUT 78 MILLION YEARS AGO".

## Fix

- **Its form kept:** a species read keeps its form beside its lineage, both from one drawing of its world (`kinDraw`, `tree.forms`,
  `formOfKey`; a world's forms cost about 0.03 ms).
- **The leaves:** a species with no name is its look and size ("A ROD, 2.9 UM BY 0.8 UM, UNKNOWN"), and two of one world that read
  alike take their colour and motion, then a number (`treeLeaves`, with B-442's `apartRows`, a world as a folder).
- **A long look before its world:** a line too long for its row leaves out its world (then its tag) rather than ending in a cut
  ",." (`treeLeafLine`, `fitHead`). The nearest kin under the tree, the sequencer's title and its result's kin do the same.
- **The sequencer** says a species in its words on the tree (`treeWords`): its title, its kin in the result, the log's KIN. A species
  named anywhere (N) is renamed on the sequencer too.
- **Measured and not taken:** told apart over the whole tree instead of within each world, the colours and motions lengthen the lines
  past the row: 20 looks cut and 3 still alike as drawn. Plan words first and the size only where two alike: 76 leaves alike across
  unnamed worlds ("A ROD, UNKNOWN").

## Verification (2026-11-19)

- The unit's `incubator` group, "the tree's leaves and the sequencer in looks" (new): the scan region's first forty living worlds a
  vial each, every species seen and read but one. With the naming switched off: 98 leaves drawn alike another of their world, 138
  UNNAMED, the title "UNNAMED" (a failure). With it: 0 and 0; read last, the title "A ROD, 4.8 UM BY 1.1 UM" (its leaf's words), its kin
  "A SPHERE, 1.3 UM ACROSS" (a leaf's), the log "KIN: A ROD, 4.8 UM BY 1.1 UM OF UNKNOWN AND A SPHERE, 1.3 UM ACROSS OF UNKNOWN, A
  COMMON ANCESTOR ABOUT 78 MILLION YEARS AGO"; named ROSY on its flask, the sequencer's title ROSY.
- `lab tree`: its 17 leaves in looks ("A ROD, 3.5 UM BY 1.2 UM, UNKNOWN", "AN ARMOURED CELL, 65 UM ACROSS"); the frames
  `shots/tests/lab_tree.png` and `lab_sequencer.png`.
- The battery against r442 from a clean build (-Werror, no warnings), 1,609 s: 29 of 35 the same; `lab`, the unit, the flow, the tour
  and `ship` differ as expected, in the tree's and the sequencer's words; `rogues` once (`rogue_radar_lock.png`, 1,077 px), KI-446's
  flake, the same alone against r442 (22 s). No `bench check`: the bench times neither the tree nor the sequencer. Then `tools/prove
  snapshot b443`, the next base.
- Left: KI-448, two leaves of two unnamed worlds can still read alike (12 of 139 over forty worlds, each pair of two geneses under
  their own heads, alike by chance): naming either world tells them apart.
