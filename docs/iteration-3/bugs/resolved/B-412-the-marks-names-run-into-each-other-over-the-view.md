# B-412: the marks' names run into each other over the view

status: resolved 2026-10-10
severity: S3
area: the surface's HUD and the sector map (R-408's marks)

## Report

The user, after B-411, with a screenshot (on a people's world at 45.7 N 8.5 W, the scanner on RUINS saying `HERE`; over the
ruins two names written one over the other, `FALLEN` and `TOWERS` legible, `0.22 KM` at the end): "overlapping markers when you
name a landmark. In general, I don't like that when you name a landmark there is immediately a marker. What I want instead is a
way to select what marker to highlight and so that you can have one active at a time (let's say you named a few of them, a ruin,
a crater, a lake and you just select what to show) but by default there should be none and you can toggle it on or off - it
should be optional and no need to show more than one. Also I suggest you select it by clicking in N map, maybe with shift, I don't
know".

## Cause

R-408 drew the explorer's marks over the view: the eight nearest within 25 km in front, each a diamond 6 m over the ground with
its name and distance to its right, or to its left near the right edge (`drawMarksInView`). Nothing kept two names apart, as the
sector map's names are kept. Two marks on one line of sight had their names on top of each other: the user's world holds TOWERS
and FALLEN CITY a few kilometres apart, one behind the other from where they stood, both names pushed left of their diamonds
and ending at nearly the same pixel. That world holds seven of the user's marks within a few kilometres (TOWERS, FALLEN CITY,
HENGE, SLEEPING CITY, LIBRIUM, AGING CITY, RESTLESS), every one of them over the view whenever it stood in front. A mark was
drawn as soon as it was placed: `L` or the map's Enter put it over the view.

## Fix

* **None over the view by default** (`game/marks.cpp`): over the view only the mark the explorer shows, `Guide::shownMark`, -1 at
  first and for a new mark. It is drawn wherever it stands on the world (the 25 km went with the eight), a diamond with its name
  and distance as before, and a hollow diamond on the compass strip gives its bearing (an arrow at the strip's end beyond sixty
  degrees; the scanner's echo is the filled cyan one, both drawn by `drawCompassMark`), so it is found when it is behind.
* **Shown from the sector map** (`game/hud.cpp`): `Shift+Enter`, or a click with `Shift`, on a mark shows it in place of any other
  (`SHOWN OVER THE VIEW: FALLEN CITY`); on the shown one it hides it (`NO MARK SHOWN OVER THE VIEW`); on no mark it says `NO MARK
  UNDER THE CURSOR`. A plain Enter or click marks and renames as before. The map boxes the shown mark in its colour and draws its
  name first; the panel says `SHIFT+ENTER SHOWS` or `SHIFT+ENTER HIDES` under the mark at the cursor. The help's `N` line reads
  `MAP: ENTER MARKS, SHIFT+ENTER SHOWS`.
* **Kept** (`game/guide.*`): a `shownmark <index>` line after the marks in the guide; removing a mark before it moves it down,
  removing it clears it.

The maps keep every mark and the rangefinder still names the one the crosshair rests on; the shown mark is one for every world and
save slot, and is chosen on the map only (KI-391).

## Verified

Two new unit checks (292 in all), on the R-408 landing: two marks placed from the sector map in a line 350 m and 1.8 km ahead
(the user's case), the view pitched to put the near one's diamond in the middle of the frame. The HUD's white in the frame is
0 px with the marks and none shown, as with the marks taken away (R-408 drew both names there, over each other); 210 px with the
near one shown by `Shift+Enter` on it (its diamond, name and distance, its diamond on the compass), `SHOWN OVER THE VIEW: NEAR ONE`.
A click with `Shift` on the far one shows it alone; `Shift+Enter` on it hides it (0 px); on no mark it does nothing and opens no
entry. The near one shown again, a mark placed before it and removed leaves it shown, the guide's file reads it back shown, and
removing it shows none. The flow: `scan_marked` (after `L`, nothing over the view, the rangefinder naming the mark under the
crosshair), `scan_map` (the mark boxed on the map) and `mark_shown` (`FLOW MARK 35 M` over the ground, the hollow diamond on the
compass).

`unit` 0 failures (3:35), `flow` exit 0 (36.6 s), the regress unchanged (22 hashes), `bench check` within every budget (the
people's descent 72.4 ms of 80).
