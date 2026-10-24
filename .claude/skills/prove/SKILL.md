---
name: prove
description: Prove that a change leaves everything Vesperis makes, draws, plays and saves identical, with tools/prove (snapshots, compares, object comparisons). Use before and after every refactoring step of a milestone, and at its close.
---

# Prove a change changes nothing

`tools/prove` runs the battery in `tools/prove.items` and compares it with a snapshot kept in `build/prove/<name>/` (the binary,
the objects, every item's outputs). `tools/prove items` lists the battery with each item's time.

## When

- **The start of a milestone:** its base snapshot exists (`ls build/prove`; the previous milestone's close made it). If none
  does, take one before touching anything: `tools/prove snapshot <id>-base`.
- **After each step:**
  - a move or an edit: `tools/prove compare <base> <the items it can touch>`;
  - a comment-only or whitespace-only step: `tools/prove objects <base>` (seconds; the engine's and the game's objects are
    reproducible, the harness's `main_headless.o` is not and is skipped);
  - a hot path: the `bench` skill.
- **The close:** from a clean tree (`make clean && make -j8`), `tools/prove compare <base>` with every item; `unit` and the flow
  pass inside it; `bench check` and pairs (the `bench` skill); then `tools/prove snapshot <id>`, the next milestone's base.

## How to run it

- Through the `prover` agent, so the output stays out of the main context. By hand: `make -j8` first (prove refuses a binary
  older than its sources), then `caffeinate -i tools/prove compare <base> > build/prove/<base>.log 2>&1` in the background.
- Alone: no make, no harness, no other agent building beside it. Frames drawn beside other jobs move (D-05's sweeps).
- A compare of the whole battery takes about 15 minutes, a snapshot 18 (the unit and the flow run twice); one item, seconds
  to minutes (`tools/prove items`).
- `PROVE_HOME` points a worktree at the main checkout's snapshots (the default finds them through git).

## What the items prove

- `fp_bodies`, `fp_stream`, `fp_peoples`, `fp_modules`: what the generators make, as keyed lines (`src/harness/CLAUDE.md`).
- `regress`, `regress_t1`: the 24 frame and determinism hashes, with the workers and with one thread.
- `screens`, `scenes`, `scenes_4x`, `sweeps`, `datasheet`: the pictures (the cabin and menus at every scale, every scene, the
  kinds' sweeps, the data sheets).
- `signals`, `rogues`, `rogues_lore`, `music`, `voice`, `media`, `instruments`, `sounds`, `tape`, `probe`, `audio`: the
  printouts and every WAV.
- `settings`, `unit`, `flow`: the unit's checks, and the flow's saves, guide and pictures from an empty `shots/tests`.

Every file an item writes is compared: fingerprints key by key, text after the masks, PNGs pixel by pixel, the rest byte for
byte. Its exit code, printouts and file count (`expect` in the items file) are compared too.

## The known noise

- Masked in every item's text: timings (`N ms`, `N us`, `(N s)`, `in N s`) and the saves' `saved <unix time>`. An item adds its
  own in `tools/prove.items`: `mask=<regex>` (no spaces: `\s`) and `noise=<glob>`, its files known to move, with the reason
  in a comment there (the unit's: six frames and its autosave move with the pace of the threads it waits on).
- Items marked `twice` (the flow, the unit) run twice at the snapshot, and what moved between those runs is skipped (`NOISE`
  in the item's directory). Two runs back to back can agree where a third differs: name such a file with `noise=` once its cause
  is found, never before.
- Frames drawn beside other jobs; two compiles of the harness (proven by behaviour, never by its object).
- To see an item's own noise, compare a snapshot with its own binary: `PROVE_BIN=build/prove/<base>/vesperis_test tools/prove
  compare <base> <item>`. `PROVE_BIN` also takes an old tree's objects relinked with the newest harness (`make -n -B
  vesperis_test` gives the link order), to give a new fingerprint column a base.

## On a difference

1. Run the one item again alone: `tools/prove compare <base> <item>`. Noise does not repeat; a change does.
2. Look at it: the snapshot's outputs are in `build/prove/<base>/items/<item>/shots/`, the new ones in
   `build/prove/cmp/<base>/<item>/shots/`.
   - A picture: `tools/png.py diff <old> <new> <out.png>`, then Read the out (the `look` skill).
   - A fingerprint: `tools/keydiff.py <old> <new> --show 20`; for the texts behind a hash, run
     `fingerprint <kind> <n> <out> full` with both binaries (the snapshot's is `build/prove/<base>/vesperis_test`) and diff.
3. Find the cause before going on: a draw order moved, link order, an expression's shape, a reset by assignment. Never mask a
   difference away, and never re-snapshot over one.

The report: the compare's verdict line, what was compared, and each difference with its cause.
