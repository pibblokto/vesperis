# R-447: jars taken out

status: done 2026-11-20 (1.5.0, before its release)
milestone: 1.5.0

## Request

The user, 2026-11-20, after asking what to do with a jar on the shelf and hearing that a jar only shows what each band's species
lives on: "Let's commit and remove jars, there is no need for them in my opinion considering incubator in generator and microscop".
A flask already shows what a species grows on (the sheet's GROWS line), and the microscope shows how it answers light and food.

## Done

- **No jars:** the incubator has one page, the flasks. Tab and the pad's SELECT no longer open a jars page. The lab loses the shelf
  over the bow bench and the glow that lit the jars from inside, and the code loses `life/jars` (`jarRecipe`, `jarAt`, `JarLayer`).
- **The lab is a little darker**, about 7% (from the arch the middle brightness goes from 84 to 78 of 255): each surface gets back a
  share of the room's mean light, and the jars' glow raised that mean. Kept without the jars, the glow showed as a bright dot on the
  bare wall.
- **The sheets** lose the IN A JAR line, and the guide no longer keeps `jarband` lines.
- **Old files load:** an older save's `jar` lines and an older guide's `jarband` lines are skipped. A jar never used up its vial, so
  nothing else is lost. The player's guide had one such line (VURYM's band at the top).
- **The harness:** `lab jars` and its two pictures go, with the jar checks (R-439's, and the jar chooser in R-441's), the jars frame
  of `ship life` and the flow's jars frame. The flow waits the same five frames at the incubator, so the frames after it keep their
  times. The lab's save check now loads a save with a jar line and a guide with a `jarband` line and finds both dropped.
- Proof and records: `docs/decisions/log/7-1.5.0.md` (R-447); the jar rules are in `docs/decisions/superseded.md`.
