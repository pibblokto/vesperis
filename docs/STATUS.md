# Status

The state of the work, in one place; each milestone's close rewrites it (`docs/CLAUDE.md`).

- **Release:** 1.4.0, unreleased, on branch `release/v1.4.0` from main 9c4ebc2. `GEN_VERSION` 16.
- **Done:** the sixteen world milestones P-00..P-07, Q-01..Q-03 and D-01..D-05 (committed 92e580a) and the reorganisation's
  thirteen, E-01 to E-13 (`docs/ideas/PLAN-reorganisation.md`; the last 31cd9f0). B-429 (2026-10-26): a storm named in a probe's
  recording no longer holds the explorer between it and the gallery (KI-427). U-01 (2026-10-26), the screens' first: `tour` opens,
  keys and pictures every state, each menu row found by its label; `settings` mended (KI-426); `tour` and `input` in the battery.
  U-02 (2026-10-26): a state is its row in `Game::STATE_ROWS` (states.cpp), and the frame runs the row's update and picture, with
  no switch on the state. U-03 (2026-10-26): the menus are lists of rows, their counts read from the lists, and a cabin device is
  its row in `cabinDevices` plus its picture. U-04 (2026-10-26): a screen's controller page, its name and a map's buttons are its
  row's `padMenu`, a method in the screen's file, and the general page reads traits (`ST_BASE`, `ST_CLOCK`). U-05 (2026-10-26): a
  screen opens through `openScreen` (its row's `enter`) and Esc closes it through `closeScreen` (its row's `back`: the scene, its
  opener or a screen); four return variables folded; a row's own save lines, load and reset. U-06 (2026-10-27), the close: the
  tour runs every row (`openScreen` or a setup by name, its controller's commands) and `unit states` checks each; `GameState`
  ends with `COUNT`; the cookbook's "Add a screen" (five places in the code). The series is done (R-424).
- **Pushed** 2026-10-27, on the user's word ("commit and push"): `release/v1.4.0` with the screens plan (d39a965), B-429
  (b9145e5), U-01 (8aa9090), U-02 (364ee43) and U-03 to U-06, a commit each; its PR into main (#9) open and not merged until the
  user says.
- **Done** 2026-10-27: the U series, a screen in one row (`docs/ideas/PLAN-screens.md`, R-424): U-01 to U-06, in 1.4.0.
- **Next:** the release (`packaging/RELEASE.md`, the `release` skill) on the user's word, after KI-423. At the release,
  `ITERATION-1.4.0.md`, `IDEAS-worlds-and-wanderers.md`, `PLAN-reorganisation.md` and `PLAN-screens.md` move to
  `docs/archive/ideas/`.
- **Before the release:** KI-423: GCC 9.4, the Linux builds' compiler, cannot compile kinds.cpp's table check, so the Linux
  zips fail until it does; with it, U-02's states table, U-03's rows, U-04's `padMenu` and U-05's columns under GCC 9.4 (Docker,
  asked first).
- **The proof's base:** `build/prove/u06` (30 items: the battery with `input` and `tour`, the tour with its rows).
- **Open with the user:** standing leave to commit each step; the clangd-lsp plugin; KI-423's fix; KI-424 (the grammar's
  picture register, which no call expands); KI-425 (the radar's heard list); the three found by U-01: KI-428 (a T typed at the
  console warps time), KI-429 (the settings' title over their first row), KI-430 (a race in the unit's lent piece frame); U-04's
  KI-431 (the almanac's STOP THE RUN, offered and ignored); U-05's KI-432 (the cabin's screens drawn over a scene left
  behind: the title, the ground) and KI-433 (H at the title forgets a landing); U-06's KI-434 (a load or a new expedition
  notes no visit); game.h at 47.7 KB, the next screen past the plan's 48 KB (section 8 revisits at 60 KB); the
  old proof kits (`build/p00` to `build/d05`, about 9 GB), deleted only on the user's word.
- **The disk:** room enough (about 117 GB free); the U series needs about 6 GB (a snapshot and a compare's outputs about 0.8 GB
  each).
- **Next free ids:** bugs B-430, requests R-425, known issues KI-435. Milestone ids go by letter series in
  `docs/ideas/MILESTONES.md`.
