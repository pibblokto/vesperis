# R-434: the activity vision

status: done 2026-11-19 (1.5.0, before its release; one of three requests for the laboratory and the search, R-432 to R-434)
milestone: 1.5.0

## Request

The user, 2026-11-18: "Vision for higher activity soil/water". Talked through before it was built (2026-11-19): a sight of its own on
Shift+O (beside O, the sample), the ground and the water within about 40 m coloured by the activity a vial would read there, square metre
by square metre; as a sensor view, a thermal camera's way (the near ground and water in false colour, the rest dimmed); on foot and from
the buggy; the lamp's ultraviolet kept as it is. "I agree with everything."

## Done

- Shift+O turns the activity vision on and off (V from it goes back to the eye's; V's own five modes are as they were). The square metres
  within 40 m are each read at their middle as a vial reads them (`readSample`, `sampleOf`'s crowd at work), the nearest first, a share a
  frame on the pool: about a quarter of a second fills its reach. None at work is dark (the ground's shading), faint red to orange, strong
  yellow to white by its crowd; the rest of the frame dimmed in the same ramp. It works from the buggy's and the drone's cameras too; the
  ultraviolet and it are one at a time.
- Proof and records: `docs/decisions/log/7-1.5.0.md`, R-434; the unit's `search` group (the vision as a vial reads, off to the bit);
  `life sight`; the scenes `sight_shelf`, `sight_river`, `sight_flats`, `sight_springs`; the tour.
