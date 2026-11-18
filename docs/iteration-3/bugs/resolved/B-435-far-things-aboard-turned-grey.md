# B-435: far things aboard turned grey

status: resolved 2026-11-18 (reported 2026-11-18, before the 1.5.0 release)
severity: S3
area: the ship's banks (`game/cabin.cpp`: `cabinSight`, `cabinDeal`; `game/ship_plan.cpp`: `ShipBanks::deal`; `game/scene_draw.cpp`:
`renderSpace`)

## Report

The user, in the hall with the library's arch and its windows in view (EPOC 6011:003.601): "if you are far from
furniture/computer it has one color but if you try to approach it it's another color". Two screenshots a step apart: beyond the
arch, the library's sofa is the hull's dark grey in the first and its red fabric in the second.

## Cause

A frame of the ship deals its colour banks (I-02, I-03: `ShipBanks`). Each seen finish (panels, the dark floor, fabric, brass, the
plants, glass, the prints, the hull outside) and each seen device's screen (each computer its own colour, R-404) asks for one, by
its nearness. The nearest asks of the pool are dealt; past the pool a finish fell to the hull's grey (drawn in bank 8) and a screen
dimmed.

With a window in view the pool was the cabin's own eleven banks, 9 and 21-30. The space view held its nineteen (0-7, 11-20, 31)
whatever showed of it through the glass: a world's six or seven, the second world's, a companion's, the belt rocks'.

The asks outnumbered the pool (z07's `ship rooms`, at the start's world):
- the hall from its arch: 15 asked, 4 dimmed;
- the hall's middle: 13 asked, 2 dimmed;
- the hangar's middle: 21 asked, 10 dimmed.

In the user's view the library's fabric was among the farthest: a step nearer moved it inside the eleven, and it turned from the
hull's grey to its red. The library's windows showed stars only, so the banks of the parked world, never drawn there, stood idle.

## Fix

The banks are dealt once the outside is drawn (`cabinDeal`, between the space view and `cabinPalette`), not in the sight before it.
- **The sight** keeps what asks (`CabinFrame::asks`).
- **The deal** reads the banks the space view left inside the glass's box (`ShipSight`'s `ox0`-`oy1`, the panes in view; a scan of
  their pixels, a bit a bank: `CabinFrame::outsideBanks`). In the hatch's drop it scans the whole frame, since the open hatch is no
  pane; with no glass in view, nothing.
- **The pool** is every bank but 0, the hull's, the light's and those (`ShipBanks::deal` takes the banks kept out).

What shows through the panes keeps its ramps; everywhere else the ship's pieces cover the space view, as `ship rooms` proves for
the views where it is skipped. Looking in, the pool is as before (every bank but 0, 8 and 10).

The rule (`rules-ship.md`, "The frame"): the banks are dealt once the outside is drawn, the pool every bank but 0, 8, 10 and those
the outside shows; the old pool line is in `superseded.md`.

## Verification (2026-11-18)

**A unit check** in the `ship` group: "ship: no finish or screen dimmed from any view (B-435)". From each of the plan's views and
each device's stand, the light on, at the start's world (58 frames, 31 looking out):
- none dims;
- looking out, the least pool is 21 (it was 11).

The `ship rooms` and `ship light` frames:
- the hall from its arch deals 15 of 15 (11 of 15 before);
- the hangar's middle deals 21 of 21 (11 of 21 before).

"ship: every view's banks whole" holds with its rule now: no dealt bank is one the outside shows.

**KI-439 too** (found in Z-05): the incubator's flasks lost their colours with the laboratory's window in view, three of twelve
coloured in `ship life`'s `lab_bench`. All twelve show now, the frame using 26 banks where it used 32.

**The battery** against z07 shows these changes, each also there with B-434 undone:
- `ship`: the hall's middle, the lab bench, the quarters, a photograph taken aboard, and the banks' lines;
- `screens`: the guide's and the help's panels, over the cabin;
- the unit's glass hull: the hull's bank 32% of the frame, where it was 35%.

The rest is in the log entry.

**What stays:** the pool can still fall short where the glass shows a world's disc, a second disc and a second sun at once, and a
view asks more than that leaves (KI-441, offered).
