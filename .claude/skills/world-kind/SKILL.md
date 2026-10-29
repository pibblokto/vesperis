---
name: world-kind
description: Add or change a kind of world, a trait, or anything else Vesperis draws from the hashes (a made kind's sources and salt, a trait, a ground's feature) without moving a world that must stay - the kinds row, the salts and the protected worlds (salt scan), the generation protocol (GEN_VERSION, fingerprint bodies, regress bless), the sheets and the proof.
---

# Add a kind of world, a trait, or anything drawn from the hashes

## Read first

- The rules: `docs/decisions/rules-galaxy.md` (the draws and salts, the protected worlds) and `rules-galaxy-worlds.md` (the kinds,
  the made kinds); the recipe in `docs/reference/09-cookbook.md` ("Add a kind of world" and its made kinds, "Add a trait", "Add a
  quality", "Light the ground").
- The code: `galaxy/kinds.h` (the ids: append before `PT_COUNT`, `TR_COUNT`), `kinds.cpp` (`buildKinds`, a row a kind; the checks at
  its end refuse a row left out or incomplete), `system.cpp` (`kindOf`, `makeKind`), the planet function (`galaxy/CLAUDE.md`: a
  ground in `planet_grounds2.cpp`, declared in `planet_internal.h`, its case in `sampleSurface`'s switch in `planetmap.cpp`).

## Nothing made before moves

- A new decision takes a salt of its own or draws after every existing draw; a drawn list is never inserted into; a retired draw
  is still taken. A made kind's row has a `family`, a `salt` of its own and one to three `source(...)`s, never a weight, a zone
  or a rule.
- The protected worlds (rules-galaxy.md, "Protected and pinned"; `PROTECTED_SYSTEMS` in `src/harness/test_worlds.h`) stay as
  drawn. `./vesperis_test salt scan [kind|family]` reads every world of those systems with the facts `generate` gives `kindOf`,
  walks each through the made kinds as `kindOf` does, and says per kind the worlds it reaches and takes; where a new kind takes
  any, it gives the first salt (its high word stepped) that takes none: pin it (`k.salt = ...;   // pinned: ...`). A kind already
  in the table may take some (Heilya III's tundra, the probe's moons): that is the generation the unit pins. Run it before and
  after a changed source: no world's line may move. A world a test stands on goes into the unit's pins too (`unit kinds`).

## A change to what a world is

- It bumps `GEN_VERSION` (`galaxy/system.h`) once per release series, with its row and section atop
  `docs/decisions/generations.md`, before any regress bless (rules-galaxy.md, "GEN_VERSION").
- `fingerprint bodies` (the battery's `fp_bodies`, read with `tools/keydiff.py`) shows body by body that only the bodies it means
  to move did; then `regress bless`, the log entry saying why.

## Look

- `./vesperis_test kinds <NAME|ground|family>` (its sheet: the globe, the map, the row's facts, the ground and its event), `survey`
  (its share and its materials), `maps`, `traits <name>`, `consistency`, `landmaps`; `matlod <kind>` for a material at the far
  rings (B-404: `matVisible`, `featFade`). The `look` skill reads the pictures.

## Prove

- `tools/prove compare <base>`: every difference is one the change means (the `prove` skill); a change to a ground or the
  terrain gets bench pairs (the `bench` skill).
- The unit's kinds groups (`unit list`); the milestone's log entry names the salts pinned and the worlds kept.
