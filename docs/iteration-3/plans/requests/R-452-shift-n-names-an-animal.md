# R-452: Shift+N names an animal

status: done 2026-11-22 (1.5.0, before its release)
milestone: 1.5.0

## Request

J-07 let N name the animal under the crosshair and open the map otherwise, and offered Shift+N instead. The user, 2026-11-22: "yes,
let's move to shift + N".

## Done

- On the ground N always opens the sector map. Shift+N names the animal the crosshair rests on; with none there it opens the map too, so
  a player running with Shift held still gets the map.
- The crosshair's words say `A STRIPED HOPPER - SHIFT+N`; the pad's menu has SECTOR MAP always and NAME THE ANIMAL (Shift+N) while an
  animal is under the crosshair; the controls list reads `MAP / NEXT STAR (SHIFT: NAME ANIMAL)`.
- **Verified:** the unit's `animals` group, "the record and its names; N the map, Shift+N names": N on an animal opened the map, Shift+N
  named it. By the old code N named it.
