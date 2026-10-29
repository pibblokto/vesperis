# B-431: the cabin's screens over a scene left behind

status: resolved 2026-10-29 (found 2026-10-26 in U-05's survey as KI-432; fixed on the user's word)
severity: S2 (S3 as KI-432; a save that puts the explorer back on the ground they left makes it S2)
area: a screen's way in (`game/states.cpp`, `openScreen`), the photo's caption (`game/names.cpp`, `screenshotCaption`)

## Report

Found in U-05's survey (2026-10-26) and offered as KI-432: "The cabin's screens set no scene: after H at the title, the flight
computer, the console, the drone bay and the case draw over the title; after H or Esc on the ground and the ascent, over the
ground." The user: "lets fix KI-423, KI-432, KI-433 and KI-434 one by one starting with 423". The released 1.3.2 does the same:
its overlays drew over `returnState` too, and these screens never set it.

## Cause

An overlay is drawn over its scene, `returnState` (`renderUnder`: the ground, the title over space, else space), and a save made
off the scenes places the explorer by it (`persistence.cpp`: on the surface when the state is not in play and the scene is the
ground). The keys that open a screen from a scene set it first (`returnState = state; openScreen(...)`): Esc, H, J, I, M, Tab in
space and on the ground, H and C at the title, the guide's own `enter`. The cabin's devices did not: their rows' acts open the
console, the flight computer, the drone bay and the case with `openScreen` alone, as do the flight computer's rows for the case,
the bay and the almanac (the flight computer closes first, so they open from SPACE too). Nothing else sets the scene on the way
back to the ship: the capsule's climb ends with `state = SPACE`, and the title's Enter goes on without it. So the scene stayed the
one left:
- After H or C at the title (`returnState = TITLE`), the cabin's screens drew over the title's picture.
- After H, Esc, J, I, the sector map or a mark named on the ground, and the climb back aboard, they drew over the ground left
  behind, which `surf.valid` still holds.
- Those screens simulate but are not in play, so the autosave runs in them every five minutes, and closing the window there saves
  too: the save's `surface 1` and site lines put the explorer back on the ground at the next start.
- The photo's caption (the text beside a screenshot) read the scene in any state, so in space after the climb it named the
  ground's place and latitude, not the orbit.

## Fix

`openScreen` takes the base a screen opens from as its scene: `if (stateHas(state, ST_BASE)) returnState = state;`, before the
row's `enter` (which can still set its own: the decoder's and the telescope's names set SPACE). A screen opened from the title,
space or the ground is drawn over it and saves by it, whatever was left in the variable before. The explicit
`returnState = state` before the scenes' own `openScreen` calls are now redundant and stay as they were.

The caption reads the scene only off the scenes, as the save does: the ground's place on the ground or over it
(`ST_GROUND`, or not `ST_IN_PLAY` and the scene the ground), the orbit or the system in space. The rule
(`rules-game-screens.md`): a base a screen opens from becomes its scene, and a scene's own state reads no `returnState`.

## Verification (2026-10-29)

A unit check in the `states` group, "the cabin's screens over the scene they open from: after help at the title, and after the
ground (B-431)". A new game at its title: H, Esc, Enter, and the console at its wall (E). Then a landing on the home system's first
world, H and Esc on the ground, K and Q, and eight seconds of the climb back aboard. Then the photo's caption, the console again
and a save in it, and the drone bay from the flight computer's row.
- Before the fix it failed: the console after help at the title over TITLE; the photo "UNKNOWN 28.6S 49.0E FELISIAN"; the console
  after the climb over SURFACE, its save "surface 1"; the bay over SURFACE.
- After it, it passes: the console over SPACE both times, the photo "IN ORBIT OF UNKNOWN", the save "surface 0", the bay over
  SPACE. The group has 4 checks with 0 failures.

`tools/prove compare u06` (30 items, 1,793 files, 1,300 s): 28 items the same. The unit differs only by the new check (its line,
and 408 checks with 0 failures against 407). The flow differs in the two frames its title step makes after the credits:
`flow_console.png` (44% of its pixels) and `flow_shipscreen.png` (83%), where the title's lettering stood behind the console and
the flight computer and the cabin stands now. The tour, the screens, the scenes and every other item are the same. `make check`
passes.

In the game: H at the title, a key, Enter, then the GOES console: space round it, not the title. Or land, press H and a key, take
the capsule back up (K, then Q), and open the console, the flight computer, the drone bay or the case: space round them, and a
save made there loads in orbit.
