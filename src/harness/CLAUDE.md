# src/harness: the harness's own files

Linked only into `vesperis_test` (the Makefile's `HARNESS_SRC`, CMake's `HARNESS_SOURCES`), never into the game. E-02 began it;
E-03 moves the rest of `src/main_headless.cpp` here, whose traps are `.claude/rules/harness.md` until then.

- `fingerprint.cpp`, `fingerprint.h`: `fingerprint bodies|stream|peoples|modules [n] [out] [full]` (`runFingerprint`), what the
  generators make as keyed lines for `tools/prove`; `fpHash` (FNV-1a) and `fpPut` (printf onto a string).
  - bodies: Q-03's fp5, every body of four scan regions; its `all` column is fp5's hash, the others its groups, `gen2` and
    `ground2` the fields fp5 left out; `dark` lines for the drifters nearest home (all 60 kinds).
  - stream: P-01's draw-stream tool, the scan region and a far strip of every class.
  - peoples: D-05's recdump, the fixed worlds and every 6th people's world round home; `full` is recdump's text.
- `fingerprint_modules.cpp`: `fingerprint modules`, for a fixed sample of worlds what nothing else pins: buildings, ruin
  layouts, works, roads and the plan, music, instruments, the forms' colliders, tongues, flora, drainage tiles, the almanac.

Rules:
- A keyed line is `<what> <key...> <field>=<value> ...`; a value holds no space. A new field goes in a new column, never into
  an old one's hash, so the old columns still prove the old fields (`tools/keydiff.py` reports a new column apart).
- The output must be the same at any thread count and between runs: `tools/prove` compares it byte for byte by key.
- No dynamically initialised globals and nothing self-registering: link order must not move the engine's start-up.
- Outputs go to `shots/tests/` relative to the working directory (the repo root, or a scratch directory with `src/` linked, as
  `tools/prove` runs every item).
