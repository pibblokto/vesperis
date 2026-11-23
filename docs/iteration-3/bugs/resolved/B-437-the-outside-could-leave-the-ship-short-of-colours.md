# B-437: the outside could leave the ship short of colours

status: resolved 2026-11-18 (KI-441, before the 1.5.0 release)
severity: S4
area: the frame's palette aboard (`core/framebuffer.cpp`: `sinkUnder`, `raiseUnder`, the mush, `toRGB`; `game/cabin.cpp`: `cabinDeal`;
`game/scene_draw.cpp`: `renderSpace`)

## Report

The user, 2026-11-18: "fix 441 and 442 please". KI-441, the rest of B-435: "Aboard, the banks can still run short: with two worlds'
discs and a second sun in the glass (the outside keeping up to 18 banks), a view asking more than the rest (the hangar's 21) dims its
farthest finishes."

## Cause

A frame aboard is one picture of 32 banks. The space view is drawn first and the ship over it. Since B-435 the ship's finishes and
screens were dealt the banks the space view had not left inside the glass's box.

The space view has nineteen banks (0-7, 11-20 and 31): the stars, the sun, each disc's six or seven, the companion, the belt's rocks.
With two discs and a second sun in the glass the outside could keep eighteen of them besides 0, leaving the ship eleven. A view asks
up to twenty-one (the hangar's middle), and past the pool a finish fell to the hull's grey and a screen dimmed, as in B-435.

Rare in play, but nothing kept a view from it:
- at the start's world no view came near it;
- six binary systems near home, parked at a moon with its parent's disc in the glass, at 24 attitudes from every view, the hull
  polarised and depolarised: the glass held at most twelve banks, and nothing dimmed.

## Fix

The outside gets a palette of its own. Once the space view is drawn, `cabinDeal` sinks its pixels in the glass's box (the whole frame
in the hatch's drop) into one bank, 7 (`BANK_SPACE`: no home's, nothing aboard draws in it).
- **What is kept:** each pixel's own bank, a byte a pixel (`Framebuffer::under`), and the space view's ramps (`underPal`).
- **The pool:** every bank but 0, 8, 10 and 7, twenty-eight, whatever the outside shows.
- **The ship over it:** a pane darkens a pixel and keeps its bank; the sheen and the solid pieces replace it, as before.
- **The conversion:** `toRGB` reads a pixel of bank 7 through `underPal` at its own bank, found where the mush took the pixel's
  bank from (a row below a pass: the mush counts its passes), so every pixel of the outside is the colour it was.
- **After the conversion** `raiseUnder` puts the outside's banks back, since a flight's next frame fades this one into its streaks.
- **Bank 0** (the cleared frame, the stars' white) stays as it is: no ramp of the ship is written over it.

B-435's scan of the glass's box went with it: the sink reads the same pixels and tells the same banks (`CabinFrame::outsideBanks`).

The rule (`rules-ship.md`, "The frame"): the pool is every bank but 0, 8, 10 and, looking out, 7, the outside in its own palette.
B-435's pool line is in `superseded.md`.

## Verification (2026-11-18)

**A unit check** in the `ship` group: "ship: nothing dimmed whatever the outside shows (B-437)". Every bank of the space view is
painted in stripes across the glass's box before the deal (`harness.shipBusyOutside`), from the plan's 58 views and the devices'
stands.
- With the fix: 31 frames look out, each with at least 18 banks in the box; none dims, and every frame's banks are whole.
- With B-435's deal (a scratch build of this change undone) 8 frames dim: the hangar's middle 10 of 21, the cold store 8 of 19,
  across the bridge 7 of 18, by the stair 6 of 17, the stern bench 4 of 15, the hall's middle 2 of 13, toward the bow and the
  bridge's floor 1 of 12.

**The layer's own check** in the `basics` group: "a frame's layer under its banks reads as the frame whole (B-437)".
- A frame of random pixels and ramps, sunk (whole, or a box of it), every ramp but 0's changed after, mushed and turned to colour,
  is the frame turned to colour as it was wherever a pixel's bank came from the box: both mush kernels, whole, dithered or a box, 6 of
  6. Its banks come back as they were mushed.
- A box sunk leaves the rest of the frame as it was: 0 pixels wrong.
- With the mush's passes not counted (a scratch build) it fails: 0 of 6.

**B-435's check** holds: from the 58 views none dims, the least pool looking out 28 (21 with B-435's deal at the start's world, 11
before B-435).

**The frames:** `shots/tests/b437_busy_outside_before_after.png`, across the bridge with every bank of the space view in stripes in
the bow windows. With B-435's deal (left) the console's amber screen and the green beyond it are dark; with the fix (right) both keep
their colours, and the stripes are the same. The hangar's middle, 10 of 21 dimmed before, draws the same pixels either way: what
dimmed there was out of sight.

**The battery** against b436:
- `ship`: its bank lines alone (each home on its own bank: the panels' 11, where the outside's 11, a world's forest, had pushed them
  to 1; looking out a pool of 28). Every picture of `ship rooms`, `ship light` and `ship life` keeps its pixels: the outside reads
  through its own palette as it read before.
- `unit`: the new checks' lines and the ship's bank lines.
- The rest moved with B-438 (its B file).

**The bench:** the layer costs a tenth to two tenths of a millisecond a frame aboard at 4x.
- Four AB/BA pairs against b436: `bridge 4x` 5.47 to 5.66 ms, `library 4x` and `lab 4x` 2.03 to 2.18, `dome 4x` 4.05 to 4.30, `space
  4x` 5.3 to 5.5; the rows off the ship level.
- The same build with B-435's deal put back, run turn about with it four times: the same tenths, and no slow run on either side.
- The passes, probed with a third of the frame looking out at 4x: +0.23 ms (the sink 0.09, the conversion 0.05, the raise 0.08).
  The first cut moved the whole of `under` a row each pass, raised the whole frame and made the layer's table each frame; now the
  mush counts its passes, the conversion and the raise keep to the box, and the table is kept while its palette holds.
- `bench check` alone, twice: every budget held, `library 4x` 2.18 ms of its 2.8.

**What stays:** now and then the ship's rows run slow in the bench. `library 4x` went past its 2.8 ms in 4 of 26 runs of this build
(3.02 to 3.44 ms) and in 1 of 16 of the old one, never twice in a row (KI-443, offered).
