# Rules: the harness and the proof: the battery and the bench

The rules in force for vesperis_test, its checks and the proof that a change changes nothing, a line each with the entries
behind them (`log/`, by release). A milestone that sets or changes one edits its line here; a replaced one moves to
`superseded.md` (`docs/CLAUDE.md`). The harness itself: `rules-harness.md`.

## The regress
- Regress hashes match exactly, never within a tolerance, the maps warmed first; each platform is gated against its own `-O2` bless (KI-014) [M7-04], [F-09]
- `regress` passes 24/24 both ways (the workers, `VESPERIS_THREADS=1`) at a close; an additive step blesses nothing [Q-03], [D-01..D-05]
- A moved hash is blessed only once named as the change's own (which frame, which pixels, why); every other hash holds [B-404], [S-03], [S-05], [S-06]
- The regress draws `scene_felisian_forest_4x` and `cabin_1x` again by one thread and fails if they differ; `regress bless` refuses then [F-11]
- The regress sees no shard, vehicle, radar, peoples, front or Wolf-Rayet giant: the unit, flow, scenes and `tail` prove them [C-01..C-12], [W-03], [B-427]

## The battery
- A step is proven by `tools/prove compare <base>`, not by a kit in `build/<id>/`; a close takes `tools/prove snapshot <id>`, the next base [E-02], [E-03]
- Each item of `tools/prove.items` declares the files it writes (`expect`); every file is compared by kind and counted [E-02]
- An item's masks (`mask=`) and known noise (`noise=`, with its reason) are declared in `tools/prove.items` alone [E-02]
- Noise is fixed at its cause: the only declared noise is the unit's six scanner and map frames and its autosave [E-02]
- The unit, the flow, `tour` and `input` start from an empty `shots/tests` (the flow makes its saves); a snapshot runs them twice [F-01], [F-07], [E-02], [U-01]
- Modes the battery leaves out are proven by an A/B items file recorded with the binary before the change (`PROVE_ITEMS`) [E-03]
- A harness change is proven against the base's engine objects relinked with the new harness (`PROVE_BIN`), the base of a new column [E-02]
- A comment-only or build-flag step is proven by `tools/prove objects <base>` after a clean rebuild; the harness's own objects, by behaviour [E-02], [E-03]
- A code move: `tools/mover.py` (the old file rebuilt byte for byte), proven by `tools/samecode.py` (each function's IR; `--rename` new names) [E-03], [E-05]
- Fingerprint lines are keyed (a new field is a new column; the others still prove); `bodies` reaches all 60 kinds, `modules` what the others miss [E-02]

## Proofs by area
- Drainage is proven by `tileHash` (`drain` on three types, `home`, the shore scene), the rings by `capsule` too (no sky below the horizon) [F-03], [F-04]
- The cabin, the screens and the sun are proven by `screens` (by the workers and one thread) and `suncone` [F-05]

## Reading a difference
- A change that moves pixels is judged by `pngdiff`'s count, largest step and bank changes before its frames are looked at [F-01], [F-10]
- Noise does not repeat: a difference or a bench failure is run again alone (other jobs move frames too) before it is believed [F-11], [D-05], [E-02]
- Sweeps depend on their order (one renderer, the landing maps' one clock): a new kind's sweeps are judged against a scratch build skipping it [P-01..P-07]
- Known to move between runs: pictures printing a time (the landforms sheets, the capital map), the bench's people's-world pictures [P-00], [P-03], [E-03]

## The bench
- `bench check`'s 38 budgets hold at every close, each about 1.5x its row's median on the dev Mac with the workers, at least median + 0.5 ms [F-11], [E-03]
- A row's budget falls with its time when a change makes it faster, so a later change that gives the gain back fails [F-03], [F-11]
- A row is budgeted once it holds within a few percent over runs; never a recorded frame, an arrival's first frame or a 1x or 3x row [F-01], [F-11]
- `bench check <factor>` multiplies every limit: another machine or one thread passes its own factor [F-11]
- Budgeted: the densest scene at the default scale (`forest 4x`), the descent's first frame; surface rows print their bands [B-310], [O6-03..O6-07], [F-01]
- The bench descends at the default site (Enter's, never `R`'s random one); each row warms itself, with five dt-0 frames after a scale change [G-02], [F-11]

## Timing pairs
- A timing claim: medians of four AB/BA pairs (`tools/prove bench`) against the previous binary, from paths of one length, on mains [F-02..F-11], [P-04], [E-02]
- A change drawing nothing new on the bench's worlds is level on every row with the previous binary [D-05]
- A row moving where the change runs no code is placement or noise until a scratch build placing the code alike says otherwise [D-02], [D-03]
- The sector map and space rows give no verdict under a tenth (layout and launch move them as much); `buggy 2x` is noisy [P-05], [E-03]
- Rows that touch files (`guide save`, `guide load`, `photo PNG`) move with the disk: judge them beside the old binary in the same runs [P-02], [Q-02], [E-02]

[M7-04]: log/0-1.0.0.md
[B-310]: log/0-1.0.0.md
[O6-03..O6-07]: log/0-1.0.0.md
[G-02]: log/1-1.1.0.md
[B-404]: log/1-1.1.0.md
[S-03]: log/1-1.1.0.md
[S-05]: log/1-1.1.0.md
[S-06]: log/1-1.1.0.md
[C-01..C-12]: log/1-1.1.0.md
[W-03]: log/2-1.2.0.md
[F-01]: log/4-1.3.1.md
[F-02..F-11]: log/4-1.3.1.md
[F-03]: log/4-1.3.1.md
[F-04]: log/4-1.3.1.md
[F-05]: log/4-1.3.1.md
[F-07]: log/4-1.3.1.md
[F-09]: log/4-1.3.1.md
[F-10]: log/4-1.3.1.md
[F-11]: log/4-1.3.1.md
[B-427]: log/5-1.3.2.md
[P-00]: log/6-1.4.0.md
[P-01..P-07]: log/6-1.4.0.md
[P-02]: log/6-1.4.0.md
[P-03]: log/6-1.4.0.md
[P-04]: log/6-1.4.0.md
[P-05]: log/6-1.4.0.md
[Q-02]: log/6-1.4.0.md
[Q-03]: log/6-1.4.0.md
[D-01..D-05]: log/6-1.4.0.md
[D-02]: log/6-1.4.0.md
[D-03]: log/6-1.4.0.md
[D-05]: log/6-1.4.0.md
[E-02]: log/6-1.4.0.md
[E-03]: log/6-1.4.0.md
[E-05]: log/6-1.4.0.md
[U-01]: log/6-1.4.0.md
