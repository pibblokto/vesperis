# B-497: unnamed worlds told apart by their place

status: resolved 2026-11-23 (J-16)
severity: S4
area: game (game/tree_screen.cpp: `treePageWhere`, `treeLeaves`, `treeLeafLine`, `treePage`, `renderTree`; catalogue.cpp:
`catalogueLine`; lab_lists.cpp: `labFolderWhere`)

## Report

KI-448, found in B-443 (2026-11-19): "Two leaves of the tree of two unnamed worlds can read alike (B-443 tells a world's apart): 12 of
139 over forty worlds' species read, each pair of two geneses; naming either world tells them apart." Y-01 added: "a folder's page
alone holds both".

KI-449, found in Y-01 (2026-11-19): "The tree's pages of two unnamed worlds of a kind read alike ("UNKNOWN - FELISIAN", four in `lab
tree`), as the catalogue's world heads and the microscope's world folders do; their place among the pages tells them apart, a name
ends it."

## Cause

A world has no name until the explorer gives it one, so every unnamed world reads UNKNOWN. The tree's page of an unnamed world was
"UNKNOWN" and its kind, and so was its head in the catalogue: two unnamed worlds of one kind read alike. On a folder's page the leaves
of every world stand together, each with its world where its row has room (B-443), and B-443's way of telling leaves apart worked
within one world only. So two species of two unnamed worlds with the same look and size read alike there ("A CELL OF CHANGING SHAPE,
2.8 UM."). The microscope's lists of worlds and the incubator's and the sequencer's choosers already told worlds apart: R-432 gave
them each world's place, its star's sector and its body ("UNKNOWN  (+195 +0 +46 / 3)").

## Fix

An unnamed world is told apart by the same place, which the explorer can read (the sector is the HUD's PARSIS, the body its row on the
analyzer) and which never changes. It is not a name: it goes once the explorer names the world.

- The tree: an unnamed world's page shows its place under its name, on the right of the second row (the count of species not read
  stays on the left, cut to fit beside it).
- A leaf of an unnamed world says its place where its row has room, else UNKNOWN as before, and the nearest kin under the tree does
  the same.
- On a folder's page two leaves that still read alike take their colour and motion, then a number, as two of one world do (B-442,
  B-443). A number at the end of a look is now kept when the look is cut for room; before, it was cut with the look's end.
- The catalogue: an unnamed world's head is its kind and then its place ("UNKNOWN - HAIR WORLD  (+157 +0 +137 / 1)"); where both run
  past the row, the kind is left out.
- An animal's sheet in the catalogue says an unnamed world's place under FROM UNKNOWN (B-500).

## Verification (2026-11-23)

- The unit's `incubator` group, new check "unnamed worlds told apart by their place": the scan region's first forty living worlds a
  vial each, every species seen and read and kept in the collection's UNSORTED folder.
  - By the old code: 40 world pages on the tree, all unnamed, 31 reading as another does ("UNKNOWN - AMMONIA"); the folder's page
    139 leaves, 23 alike ("A CELL OF CHANGING SHAPE, 2.8 UM."); the catalogue's 40 world heads, 31 alike; the microscope's 40 world
    folders, none alike (R-432's places).
  - With the fix: none alike on the tree's pages, the folder's page, the catalogue's heads or the microscope's folders.
- `lab tree`: the two unnamed felisian worlds' pages, "UNKNOWN - FELISIAN" with "(+150 +0 +36 / 0)" and with "(+194 +5 +48 / 18)"
  (`shots/tests/lab_tree.png`). `seed record`: the catalogue's head "UNKNOWN - HAIR WORLD  (+157 +0 +137 / 1)"
  (`shots/tests/record/catalogue.png`).
