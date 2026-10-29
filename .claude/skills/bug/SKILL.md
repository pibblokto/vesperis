---
name: bug
description: Record and fix a bug the user reports in chat (often with a screenshot or a save) - the next id, the cause found in the code, the fix and its proof, the resolved-style B file and the touch points. Use when the user reports something wrong in the game.
---

# A reported bug

1. **What it is:** read what the user sent. A screenshot: try every body and read the HUD before naming the world. A
   complaint about the map or the marks: read a copy of their `guide.txt` or save first (the "dots" were their own marks).
   Read the rules of its area (`docs/decisions/rules-<area>.md`), then find the cause in the code.
2. **The id:** the next free B in `docs/STATUS.md`.
3. **One question if it blocks:** a design choice that is the user's, the recommended option first, with the evidence.
4. **The fix:** what removes the cause. A change to what is generated follows the generation protocol (`rules-galaxy.md`);
   say it in the report. Prove it: a check that fails before and passes after (a unit check, a scene, a mode's print), and
   the `prove` skill for what must not move.
5. **Written** (`docs/CLAUDE.md`): `docs/iteration-3/bugs/resolved/B-NNN-short-title.md` from `TEMPLATE.md` (the Report
   quoted, the Cause, the Fix, the Verification), a row in `docs/iteration-3/bugs/README.md`, the notes it changes in place,
   a log entry and a rules line only when a rule changed, a KNOWN-ISSUES row for what stays, `docs/STATUS.md`'s next id.
6. **The report:** the cause in a sentence, the fix, the checks with their numbers, the frames worth a look, what the user
   can try in the game now, what stays open. Nothing committed unless asked.

A bug the user settles by a redesign stays a B: quote their request in the Report, and move the rule it replaces to
`docs/decisions/superseded.md`.
