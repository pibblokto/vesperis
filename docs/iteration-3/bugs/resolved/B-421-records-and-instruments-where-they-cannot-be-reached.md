# B-421: records and instruments where they cannot be reached

status: resolved 2026-10-11
severity: S2
area: the civilisations (where T-04's records and C-11's instruments lie in a place or a work)

## Report

The user, asking for R-412: "Thoroughly test for instruments/shards placements and for invisible walls since I encountred a few". The
records and the instruments are laid by T-04's and C-11's rules (`recordSitesOf`: a building's room's spots, an archive's rows, the foot of
a structure); a new walk through the view (`walls view`, `walls view near`) went to every one of them on foot.

## Cause

The rules kept a record clear of the pieces' colliders and of the doorway, which was not enough:

* **In water**: a lighthouse's foot on its headland stood one at the sea's edge, a city across a river stood buildings in it with their
  records. The generation cannot wait for the drainage's tiles (a place is laid out before its rivers are known at 16 m).
* **Inside or under a piece**: a record inside a drawn piece whose colliders were inset from it (a heap's flank, a tumbled block, a jamb's
  foot), under a bank's, a heap's or a barrow's slope, on an earthwork's own bank (a barrow's, a ring bank's, a redoubt's spots stood on
  its earth).
* **Shut in**: the works' buildings (a keep, a house, a hall, a tower) stood with no doorway and their rooms' records shut in; a hill
  fort's huts stood through each other, one's doorway in the other's wall; a holy spring's cistern a metre before its temple's door; a
  tower's top fallen across its lobby's doorway shut the record laid in it (found on R-412's run).
* **An instrument over a record**: an archive's rows were 0.45 m off and an instrument's body stood on them.
* **Inside a hill fort** (found on R-412's run): a big piece stands on the highest ground under it with a skirt down to the lowest, so a
  rampart's stretch on a hillside stood a cliff downhill; and a quarter of the forts' rings had no gap at their gate (a stretch was left
  out only where its middle fell within 0.1 rad of the gate's bearing).

## Fix

* The records' placer (`galaxy/records.cpp`, `Placer`): a spot is clear of the drawn pieces at the floor (each piece's slice 0.06 m over
  its foot, `floorSliceOf`, with 0.3 m round it), of the heaps' surfaces (a bank's, a heap's or a barrow's ground, or within a stride of its
  toe: `formSupportAt` at five points), and of the water: the view's where it gives it, else the sea's and the planet function's at 16 m
  round it without the rivers (`DrainageOff`); the view lays a place's records again with its drawn water once the explorer is within 600 m
  (`SurfaceView::waterRecords`). A room whose doorway is shut (no run of a metre along its line clear at the knee, the A-03 check's rule)
  holds none (`Placer::doorOpen`). An instrument stands 0.7 m from a record (`beside`).
* The earthworks' rooms (`buildingRoom`): a heap's, a long barrow's and a bank's spots beyond their length, a ring bank's inside its ring
  and beyond its foot, a redoubt's by its casemate inside its bank.
* The works (`galaxy/works.cpp`): every building's doorway in its front (`Lay::add`); a holy spring's temple 17 m off (a way between); a
  hill fort's huts apart, its rings' stretch nearest the gate always left out.
* Earth stands on the lowest ground under it, sunk into the slope above (`SurfaceView::pieceFootNear` for a `formWalkable` piece, drawn
  and stood on alike): a rampart, a barrow or a heap is climbed from any side its slope allows.

## Verified

* The unit's check (`walls check`): 128 records and 8 instruments in the scene worlds' places and works, none in water, inside a piece,
  under a heap, hidden in a drawn piece, out of reach, an instrument through a wall or over a record.
* `walls view near 40`: 2,474 walks (58,781 m) from round the places and works of the forty nearest peoples' worlds, none stopped by
  anything undrawn (151 stopped: by a drawn piece 96, a trunk 27, a log 12, a boulder 9, an earthwork's side 7, down from 29); 1,592
  records and 106 instruments, none in water, inside a piece, a boulder or a trunk, under a heap, hidden in a drawn piece, out of reach, an
  instrument through a wall or over a record.
