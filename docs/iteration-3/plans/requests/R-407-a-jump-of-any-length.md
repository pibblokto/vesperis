# R-407: a jump of any length along the aim

status: done 2026-10-07
milestone: 1.2.0 (after X-04, the user's request on playing it)

## Request

"Can you add a feature to fly stardrifter in any direction you want for any number of light years you want? There is this
thing that you loop back and forth between systems tricked by signals radar and I literally had an issue with going back to
system I already visited and named a planet in - it would be nice to break the loop with just saying 'now I jump 400 light
years in that direction' and boom. Also there are, what I believe, star chart marks that note with green marker stars as
unknown, maybe it's not that, idk but it's annoying."

The Vimana flew only to a star locked in the aim (the ten light years round the ship), a star of the radar's (200 ly at
most), a name, the home or the previous star; the guide's coordinates wanted a sector that holds a star. The radar's
strongest signal is often the world just left, and nothing in the aim or the radar said a star had been visited. The green
markers: the aim's six nearest stars carry dim green diamonds, each labelled with the star's name and distance, which since
R-401 read `UNKNOWN 4.2` six times over; the star map marks the visited stars with green boxes it did not explain.

## Done

* **The jump.** In the aim (`R`, or the flight computer's new last row `VIMANA JUMP: LIGHT YEARS ALONG THE AIM`, which turns
  the view to the nose) the digits typed are a jump's light years (1 to 100,000; Backspace takes one back, a leading zero is
  none); a panel shows `JUMP 400 LY COREWARD` (or `RIMWARD`, `ALONG THE DISC`, `NORTH` / `SOUTH OUT OF THE DISC`), where its
  end lies (`END: SPIRAL ARM - 13104 LY FROM THE CORE`) and `ENTER JUMPS  BACKSPACE  R CANCELS`. Enter: the star nearest the
  end of the line (`jumpTarget`, galaxy/starfield.*: the cube of 21 sectors round it) becomes the remote target and the Vimana
  flies at once; where the end lies in the void (out of the disc, beyond its rim) the jump comes back along the line, ten
  light years at a time by the density, to the first place that holds a star (`NO STARS OUT THERE - THE JUMP ENDS 580 LY
  SHORT`). The log: `JUMP 400 LY COREWARD FROM <star> TO PARSIS ..., SPIRAL ARM`. The GOES console: `JUMP 400` along the
  ship's nose.
* **The flight's length** (`vimanaSeconds`): `7 + 2 sqrt(ly)` as before to 100 ly (27 s), then 6 s for every doubling: 400
  ly in 39 s, 1,000 in 47, 10,000 in 67, a crossing of the galaxy (100,000) in 87 s instead of ten minutes.
* **The markers.** The aim's six diamonds read the distance alone (`4.2 LY`) where the star has no name, the name where it
  has one, and `VISITED` in amber where the ship has been; the star under the crosshair, the remote target's line and the
  radar's lock (`A SIGNAL FROM UNKNOWN (S00)  34 LY  VISITED`, `A SIGNAL LOCKED - ITS STAR WAS VISITED, ENTER FLIES`) say
  `VISITED` too. While aiming, a key line `N NEXT STAR  ENTER LOCKS  DIGITS JUMP  R CANCELS` replaces the local target's
  label at the bottom. The star map explains its marks (`VISITED`, `HOME` beside the class filter), labels an unnamed visited
  star by a world the explorer named there (`(XOHORT)`) and its card lists them (`YOUR WORLDS THERE: XOHORT`).
* Harness: three unit checks (the jump of 400 ly from home typed in the aim: 2 ly from the line's end, 39 s, logged, arrived;
  3,000 ly north comes back 580 ly to a star 2,313 ly over the plane, 13,000 ly coreward reaches the core; the aim's labels).
* Docs: 07 (the remote target, the Vimana, the flight computer, the console), 04 (the jump's search), 10-decisions, 08.
