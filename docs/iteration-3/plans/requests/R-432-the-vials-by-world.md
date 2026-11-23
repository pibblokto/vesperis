# R-432: the vials by world

status: done 2026-11-19 (1.5.0, before its release; one of three requests for the laboratory and the search, R-432 to R-434)
milestone: 1.5.0

## Request

The user, 2026-11-18: "Per planet categorization in microscop (also custom created folders in a seaparet microsocrop tab for organisms
you cultivate), unlimited samples on the ship". Talked through before it was built (2026-11-19): the microscope in two tabs, the vials a
folder a world as the player's worlds; the cold store without its 64 places, the kit's eight vials a descent kept; the incubator's and the
sequencer's choosers in the same folders, a row a species. "I agree with everything." The second tab: R-433.

What was there (Z-04, Z-05): E at the microscope opened one list, every vial of the cold store (its line and its world at the right),
then each flask with a culture; the cold store held 64 vials (the door's eight lamps an eighth each); the choosers listed a row for each
species seen in each vial.

## Done

- The microscope's lists in two tabs (Tab): THE VIALS and THE CULTURES (R-433). The vials open on their worlds, a folder each in the cold
  store's order (its name and where it is, its vials and how many read STRONG); Enter opens a world's vials (each its line and the activity
  it read), Enter puts one on the stage, Backspace empties it, Esc goes back a level; E reopens on the vial last on the stage.
- The cold store holds any number of vials; the kit stays at eight a descent. Its door's eight lamps light by doublings (1 to 128 vials).
- The incubator's and the sequencer's choosers open on folders (a world's, the flasks', the collection's), a row a species (and its
  state: cells or spores) from the first vial holding it.
- Proof and records: `docs/decisions/log/7-1.5.0.md`, R-432; the unit's `lab` and `search` groups; the flow's `microscope_list` and
  `microscope_world`; the tour.
