# Documentation, third iteration

The second iteration (`docs/iteration-2/`, milestones N0-N5) is complete except the web build. This folder holds the third iteration with the same structure and process, started 2026-09-27 from a play session's findings:

1. rings should be visible from the surface (they were drawn, but nearly black: `bugs/resolved/B-301`),
2. a landing in a "mountains" region with a shore in sight never reached the shore; what you see should be reachable, with a sense of distance (`bugs/resolved/B-302`, milestone O1),
3. sectors you land in should make sense against the map you chose them on, and crossing them should be seamless (`plans/requests/R-301`, milestone O2),
4. asteroid belts exist but the Stardrifter cannot fly to them (`plans/requests/R-302`, milestone O3),
5. comets should be landable, with their small size and activity felt on the ground (`plans/requests/R-303`, milestone O4),
6. the terrain should become drastically more interesting, realistic and worth exploring: investigation and a plan only for now (`plans/PLAN-terrain-3.md`, milestone O6).

| Directory | Who writes | What goes there |
| --- | --- | --- |
| `bugs/` | you | Bug reports `B-3NN` from `TEMPLATE.md`; `KNOWN-ISSUES.md` carries the open issues of the earlier iterations and the new ones (`KI-3NN`). |
| `plans/` | both | `ROADMAP.md` (milestones `O0`-`O6`, items `Ox-NN`), `PLAN-terrain-3.md`, `IDEAS.md`, `PROGRESS.md`, and `requests/` (`R-3NN` from `TEMPLATE-request.md`). |
| `reference/` | me | Nothing new here: the technical notes stay in `docs/reference/` and are updated in place (one source of truth). |

## Process (unchanged)

1. Report a bug: copy `bugs/TEMPLATE.md` to `bugs/B-3NN-short-title.md`. Screenshots go to `shots/` (`P` in the game), a save next to the report if it depends on the place. Note: a screenshot pasted into the terminal arrives as the file's icon, not the picture; drop the PNG into `shots/` instead.
2. Request something: copy `plans/TEMPLATE-request.md` to `plans/requests/R-3NN-short-title.md`.
3. I triage into `KNOWN-ISSUES.md` and `ROADMAP.md`, keep a `status:` line in every file, and log work in `plans/PROGRESS.md`.

**Status (2026-09-28):** O6 complete (O6-03..O6-07: rivers and lakes from a flood of the ground, the landform libraries, landmarks, generation 10; `plans/PLAN-terrain-3.md` section 9); caves stay out of scope, the release preparation is next. Earlier: O0-O4 done the same day (rings, reachability, sectors, belts, comets); O5 (the terrain plan) written; O6-01 (the terrain harness) and O6-02 (the relief spectrum, the near ring, generation version 6) done and stopped for the review of the before/after sheets in `shots/tests/o6/`; the review's five points (O8, B-307..B-311: the buggy's descent, the companion star flooding the frame, the sun flicker, render scale 4 by default with band-parallel drawing, the ground's plates and textures) done the same night; the second review's six points (O9, B-312..B-317, 2026-09-28: the planet vanishing at the edge of the view, the hills morphing, the parent's distance, the vegetation, the animals, the moon in the ground) done; the third review's seven points (O10, B-318..B-321 and R-304..R-306, 2026-09-28: the holes in the ground from the capsule, the far trees' flicker, the water and the rivers, the animals and the climate, thirty-one planet traits with eighteen landforms, the plains' clutter; generation version 8) done; the fourth review's two points (O11, B-322 and R-307, 2026-09-28: the water artefacts when swimming and from the air, six new planet types with their own events; generation version 9) done; O6-03 (drainage) next.
