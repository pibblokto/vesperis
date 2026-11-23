# B-434: the laboratory's sunlight filled its arch

status: resolved 2026-11-18 (reported 2026-11-18, before the 1.5.0 release)
severity: S3
area: the sun's patches aboard (`game/ship_plan.cpp`: the mesh's `Builder` (`receives`), `boxRoom`, `bowRoom`; `shipPatches`)

## Report

The user, playing the 1.5.0 branch, in orbit in their home system (EPOC 6011:003.601): "I found a visual bug where if you are
outside the room you can see a panel of light covering doorway but if you go inside it's not on the doorway". Three screenshots:
- the first two from the hall, looking through the arch into the laboratory (its four window panes beyond): a pale panel with
  straight edges over part of the opening;
- the third from inside the laboratory, looking back at the arch and the stair beyond it: no panel.

## Cause

The sun's patches (I-03) are each glass's outline cast along the light onto the surfaces facing it (`shipPatches`). A room's
surfaces (its `ShipReceiver`s) were the whole rectangles of its floor, ceiling and walls (`receiverRect` and four wall
rectangles), holes and all.

The laboratory's port wall has the arch to the hall. With the sun low through the laboratory's starboard windows, the window's
outline lands on that wall, and the patch covered the arch's opening as if the wall were whole.

A patch is drawn added (`BLEND_ADD`), its depth a hair nearer (`zbias` 1.0015) and unwritten. It shows wherever the surface behind
it is as near or farther: on its own wall, and over a hole, on whatever lies beyond.
- **From inside the laboratory:** the laboratory is drawn first and the hall after it (the rooms by their depth through the ways),
  so the hall's pieces covered the patch in the opening.
- **From the hall:** the hall is drawn first and the laboratory after it, patches last, so the patch lay over the arch, the hall's
  own wall around it hiding the rest.

The same held for every room with glass:
- the bridge's stern wall (the hall's arch);
- the library's and the quarters' arches;
- the hall's four arches and the well in its floor (the sun down through the oculus).

Over 96 suns round the sky, 92 of 106 points inside the arches, the windows, the well and the oculus lay under a patch. The
laboratory's arch was covered under 2 of the 96 suns, the bridge's under 6.

## Fix

A room's receivers are now its surfaces' own pieces, built with them. While a room with glass builds its floor, ceiling, steps and
walls, each polygon is also a receiver (`Builder::receives`, set round `slab`, `rect` and `wall`), with the surface's inward normal
and shade. A wall's pieces are those it is built of round its arches and windows (the strips beside them, the pieces under and over
a window, an arch head's eight segments); a floor's are the slab's round the well and less the core's pit.
- A patch is cut to each piece, so it never lies over an opening.
- The bridge's steps take a patch each at their own height (they shared one at the top step's height).
- `receiverRect` is gone.

The rule (`rules-ship.md`, "The light"): a patch is cast onto the surfaces' pieces facing the sun, round their holes.

## Verification (2026-11-18)

**A unit check** in the `ship` group: "ship: no patch of the sun over an arch, a window or a hole (B-434)". Every room's patches
for 96 suns (a Fibonacci sphere) are tested against points inside each arch (in both its walls), each window, the well and the
oculus, on their planes.
- With the old receivers (a scratch build of this one change undone) it fails: 92 of 106 points covered.
- With the fix it passes: 0 covered, 3,139 patches, 459 of them on walls with an arch.

**A frame:** `ship light` has a new view, THE LABORATORY'S ARCH (the hall facing the arch, a plan's view row). Its sun is low
over the starboard bow (`lab_arch_sun`, 90 degrees, 15 up).
- Before: a flat bright panel over the opening's lower third, the laboratory's floor hidden behind it.
- After: the floor shows, the window's light lying on it between the mullions' shadows (`shots/tests/ship/light_lab_arch_sun_4x.png`).

**The battery** against z07 found the same bug once more, in the flow's `flow_dome` (the observatory, looking down through the
oculus): a small bright patch lay over a hole whose room beyond was not drawn. It is gone now (84 px; with this fix undone the frame
is z07's). The ship's other sun frames are pixel for pixel the same: their patches are cut into more pieces (the bridge's 36 to 38
at the first sun), and the pieces meet without a seam. The rest of the battery is in the log entry. Nothing stays open.
