# Status

Where the work stands. Each milestone, bug or request batch rewrites this file (`docs/CLAUDE.md`). What each item did is in its
log entry (`docs/decisions/log/`), B file or R file.

## Releases

- **Released:** 1.4.0 on 2026-10-29: branch `release/v1.4.0` merged into main by pull request #9, tagged `v1.4.0`, six zips
  (`packaging/RELEASE.md`).
- **In progress:** 1.5.0, "the small and the spacious", on branch `release/v1.5.0` (made from main bb0e58b on 2026-11-16). The plan is
  `docs/ideas/ITERATION-1.5.0.md`, the design `docs/ideas/IDEAS-small-and-spacious.md` and `docs/ideas/IDEAS-genes.md`.

## Done in 1.5.0

All milestones are done. Log entries are in `docs/decisions/log/7-1.5.0.md`.

- **Microbes (Z-01 to Z-07, R-425):** life on every landable world (`src/life/`, `docs/reference/13-life.md`), with `GEN_VERSION` 17;
  each species' form and its slide in six ways of seeing; the search on the ground (O takes a sample); the microscope, the incubator
  and jars, the sequencer and the tree of kin; the catalogue, the LIFE statistics page and the data sheet's microbe line.
- **The ship (I-01 to I-04, R-426):** the new Stardrifter with three decks and eight rooms, its light and materials, and life aboard.
- **Genes (Y-01 to Y-04, R-443, R-444):** trees of kin by world and by folder; the gene editor (cut, change, move genes in from kin,
  copy, switch) and strains written from it into flasks.
- **Player requests:** R-427 to R-442 (the ship's rooms, the lab's lists, folders, experiments, the jars, naming). Each has an R file
  in `docs/iteration-3/plans/requests/`.
- **Bugs fixed:** B-434 to B-444, each with a B file in `docs/iteration-3/bugs/resolved/`.
- **Latest batch (2026-11-20, the user's four points):**
  - B-445 (KI-451): the font draws `~`; the lake landmark mark and the gene editor's switch mark use it.
  - B-446 (KI-452): a switched strain in a predator cycle follows its numbers; the cycle splits where its state changes.
  - R-445: W on the gene editor asks which folder the new strain goes in, then its name (the parent's name and the new strain's
    number typed in to start, "VURYM E2").
  - R-446: plain English. A project skill (`.claude/skills/plain-english/`); the game's text rewritten (the lab screens by hand, the
    other screens' "THE" and "ITS" removed); the CLAUDE.md files, skills, rules files and this file rewritten.
  - The jars: the jar page now says what a jar does. A real use for jars is the user's call (see "Open with the user").

## Committed

Commits on `release/v1.5.0`, each on the user's word, none pushed: 0c90433 (Z, I, B-434 to B-438), 455c7ad (B-439), 82592c5 (B-440,
R-427 to R-434), d12a9ea (R-435 to R-442, B-441, B-442), 20b236e (B-443, Y-01, Y-02). Not committed: Y-03, Y-04, B-444 and the latest
batch.

## Next

- The 1.5.0 release, on the user's word (the `release` skill: pull request, tag `v1.5.0`, six zips; the Linux builds need Docker,
  asked first).
- **Proof base:** `build/prove/r446` (35 items: y04's, with this batch's new text, the two prompts, and the unit's B-446 and R-445
  checks).

## Playing the lab

In the laboratory, E opens a device. Microscope: vials by world and cultures by folder; on a slide L light, M magnet, H warmth, F food,
K keeps a cell. Incubator: flasks (a digit types the warmth, N names, K keeps a copy) and, with Tab, the jars. Sequencer: reads a
genome. Tree of kin: the screen on the port wall. Gene editor: K puts a kin beside it, M moves its gene in, D copies, S switches, Enter
edits letters, C cuts, W writes a strain (folder, then name). `lab microscope`, `lab cultures`, `lab jars`, `lab tree` and `lab genes`
draw them into `shots/tests/`.

## Playing the ship

Shift runs, Page Up takes the lift, U dims the lights, Y makes the hull glass. E works the bunk, the desk, the music player's deck, the
capsule, the quarters' wall of photographs and a garden bed. The buggy and the drone stand in the maintenance room astern of the hangar.
On the ground O takes a sample, Shift+O shows the activity vision, Shift+U the ultraviolet lamp. `ship rooms`, `ship light`, `ship life`
and `life kit` draw them.

## Open with the user

- **Jars:** a jar only shows which species of a sample live on what. Proposed: a band as a source (Enter on a band takes a sample of
  that species alone into the cold store), or remove jars.
- **Docs still in the old style:** the reference notes (`docs/reference/`, 1.4 MB) and the code comments, until each is next edited.
- Standing leave to commit each step; the clangd-lsp plugin; B-440's leftovers in their `shots/` (`probe_1`, `probe_2`).
- Known issues (`docs/iteration-3/bugs/KNOWN-ISSUES.md`): KI-424, KI-425, KI-428 to KI-431, KI-435 to KI-438, KI-440, KI-444,
  KI-446, KI-448 to KI-450, and two found in this batch: KI-453 (the data sheet calls a second sun's planet a moon) and KI-454 (a
  heard sound's status line can run off the screen).
- Offered by B-430: the tables' dump as a harness mode for the container; parallel.h's misleading indentation (38 of the Linux builds'
  46 warnings). Offered by B-431: the `returnState =` lines before a scene's `openScreen`, now redundant.
- Our Docker images `vesperis-build:arm64` and `:amd64` were removed on the user's word. The old proof kits (`build/p00` to
  `build/d05`, about 9 GB) are deleted only on the user's word.

## Disk

About 110 GB free (2026-11-20). `build/prove` keeps the snapshots r434, r438, r442, b443, y01, y02, y03, y04 and r446, the compares'
outputs against r434 to y04, and their logs: about 23 GB. A snapshot and a compare's outputs are about 1.3 GB each.

## Next free ids

Bugs B-447, requests R-447, known issues KI-455. Milestone ids go by letter series in `docs/ideas/MILESTONES.md` (1.5.0 took Z, I
and Y).
