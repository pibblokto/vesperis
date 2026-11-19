# R-433: the culture collection

status: done 2026-11-19 (1.5.0, before its release; one of three requests for the laboratory and the search, R-432 to R-434)
milestone: 1.5.0

## Request

The user, 2026-11-18: "also custom created folders in a seaparet microsocrop tab for organisms you cultivate". Talked through before it was
built (2026-11-19): a culture collection kept frozen as labs keep their stocks, every species and strain grown in a flask, without limit
and after its flask is emptied, in the microscope's second tab with the flasks, UNSORTED and the explorer's folders (N, M, R, Backspace).
The user: "these 'frozen' organisms should be the regular ones as from planets so you can also work with them. It's for cleaner
separation of what's unchanged from planet and what was touched by our intervention".

## Done

- Every species and strain a flask grows (its curve past its lag) is kept in the collection, once, with the medium it grew in and its
  world's; emptying the flask keeps it. A strain is kept as it arises.
- The microscope's THE CULTURES tab: THE FLASKS (the cultures growing, a drop of each as it is), UNSORTED (where the kept go), then the
  explorer's folders. N makes a folder (its name typed), R renames one, Backspace removes one once empty; in a folder M moves a culture
  to another, Backspace discards it, Enter puts a drop of it on the stage (its cells growing, as a young culture of it shows them).
- Kept cultures are worked as a world's species are: the incubator puts one in a flask (in the medium it grew in), the sequencer reads
  one (its species' genome).
- Saved: the `keptfolder` and `kept` lines.
- Proof and records: `docs/decisions/log/7-1.5.0.md`, R-433; the unit's `lab` group; the tour.
