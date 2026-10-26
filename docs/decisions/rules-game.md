# Rules: the game and its screens

The rules in force for the game's states, screens, controls and saves, a line each with the entries behind them (`log/`, by
release). A milestone that sets or changes one edits its line here; a replaced one moves to `superseded.md` (`docs/CLAUDE.md`).
Devices, flight, maps: `rules-game-devices.md`.

## The explorer's rules
- No fuel, damage or fail state: hazards (radiation, a hostile star) only warn; time by a black hole passes as anywhere [table], [S-06]
- Flights, approaches, descents, a probe's fall and drone trips take real seconds (`realTime`); the warp turns only the sky [table], [X-01], [C-18]
- Nothing has a name until the explorer gives one (`starNameOf`, `bodyNameOf`, else UNKNOWN); an aim marker reads a distance or VISITED [R-401], [R-407]
- Generated names stay in the generator (keys, ids), never shown or saved; no world's name derives from its star's [R-401], [H-04 second pass]
- A shard is taken with `E` within 2.2 m, never by walking over it, and nothing hints at one (the scarcity rule) [C-03]

## Saves, the guide and lending
- Player files: `vesperis_save_<n>.txt`, `guide.txt`, `vesperis_settings.txt`, `vesperis_keys.txt`, `vesperis_pad.txt` [Release 1.0.0], [R-418]
- Append-only: `GameState` (the save's `phase`), `ShipState::Mode` (`mode`), `AutoProfile` (`auto_profile`), the probe lines' codes [X-04], [R-419]
- Marks keep R-408's numbers in the file (`markKindToFile`: 8 a place, 9 a work, sights from 10); new sights are appended [T-03], [Q-01]
- A shard is keyed `<body>/S<people x SHARD_STRIDE + index>`, found by hand or by drone: its contents may change, its key never [T-04], [C-18]
- One line a field: `load` skips unknown keys and reads a missing kind of line as none, so the files grow by lines [C-03], [C-14]
- A format change is marked by its own line (the guide's `records 3`), never by `GEN_VERSION`; an older guide is moved or told once [T-04], [B-414]
- Save, guide and settings open with a version line: raising the settings' brings a changed default to older files once [B-310]
- Saves and the guide record `gen`: an older save loads with one notice; one whose star is gone does not load [M9], [G-01]
- The guide's save is byte-stable: loaded and saved again it is the same file, but the lending's clock [F-02]
- Not saved: the telescope, the jump plot, an almanac run (`warp` reads 1), the peoples layer, a probe's held frame [W-06], [G-06], [W-01], [R-415]
- Lending is one text file of the friend's own finds, never what was lent to them, nor pictures; theirs are cyan, in no statistic [C-14], [X-04]

## The HUD and the screens
- The HUD stays pared (the user's ask): on foot the epoch, name and place, scanner, compass, stamina, what E or Q would do; the rest on Tab [R-414]
- Paring deletes, never hides: no ways travelled kept or drawn (no `route` lines, no trail); buggy and drone share one nose-camera HUD [R-414]
- The title is the name and the choices; no version is shown; the credits name the inspiration, Noctis IV, alone [Release 1.0.0]
- A frame-wide line holds 53 characters (52 inside the margins); a list growing with a table wraps; a label with no room is left out, never cut [S-02], [H-06]
- A count is given in the other screen's words (`ABOARD: N OF ITS M SHARDS`), never a third number: the user reads numbers literally [C-18]

## Controls and the frame
- The frame never waits for a thread's result (`mapIfReady`, `shardTextsIfReady`, `worldPlanIfReady`): only the harness blocks [H-04], [F-02], [D-05]
- Both mains join the background threads at exit (`worldPlanThreadsJoin` via `atexit`) before static destructors run [H-04], [C-18]
- The landing map computes no drainage tile: it asks once the cursor rests 0.4 s, its zoom drawn dry till they come [O6-03..O6-07], [B-422]
- Key codes (`core/input.h`) keep raylib's numbers; `vesperis_keys.txt` remaps physical keys at the input layer; no rebinding screen [table], [M6-02]
- The pad is read raw through GLFW (only in `platform_raylib.cpp`), bound by role per device, and only presses keys into `Input` (`padFrame`) [R-418]
- Y opens the command menu everywhere, its keyboard reaching every key; its pages are written per screen: a key changed, its line too [R-418]
- Keys never repeat (`IsKeyPressed`): a held key is coded explicitly; nothing needs PGUP or PGDN alone (a Mac's need FN) [R-416]
- A new key is checked against every binding in every state before it is taken [X-01]

## Auto-explore
- It acts through the frame's input (`autoInput`, after `padFrame`) as a player could, but for the nose and `orbitShip`'s lap [R-419]
- Its HUD is how to stop it (its first five seconds) and the tape's caption at the top left as a track starts; it opens no screen [R-419], [R-420]
- It plays the expedition (log, guide and autosaves keep its acts), names and marks nothing, saves to no slot; any input ends it, unacted on [R-419]
- Its six profiles share one mind and every step; each look eases on a damped spring within a hand's limits [R-419]
- `M` starts and stops the tape only in auto-explore; it fades as auto-explore ends and resumes where it was (`auto_track_at`) [R-420]

[table]: log/0-1.0.0.md
[M6-02]: log/0-1.0.0.md
[M9]: log/0-1.0.0.md
[B-310]: log/0-1.0.0.md
[O6-03..O6-07]: log/0-1.0.0.md
[Release 1.0.0]: log/0-1.0.0.md
[R-401]: log/0-1.0.0.md
[G-01]: log/1-1.1.0.md
[S-02]: log/1-1.1.0.md
[S-06]: log/1-1.1.0.md
[C-03]: log/1-1.1.0.md
[C-14]: log/1-1.1.0.md
[W-01]: log/2-1.2.0.md
[W-06]: log/2-1.2.0.md
[X-01]: log/2-1.2.0.md
[X-04]: log/2-1.2.0.md
[R-407]: log/2-1.2.0.md
[G-06]: log/3-1.3.0.md
[T-03]: log/3-1.3.0.md
[T-04]: log/3-1.3.0.md
[H-04]: log/3-1.3.0.md
[H-04 second pass]: log/3-1.3.0.md
[H-06]: log/3-1.3.0.md
[B-414]: log/3-1.3.0.md
[C-18]: log/3-1.3.0.md
[R-414]: log/3-1.3.0.md
[R-415]: log/3-1.3.0.md
[R-416]: log/3-1.3.0.md
[B-422]: log/3-1.3.0.md
[F-02]: log/4-1.3.1.md
[R-418]: log/5-1.3.2.md
[R-419]: log/5-1.3.2.md
[R-420]: log/5-1.3.2.md
[Q-01]: log/6-1.4.0.md
[D-05]: log/6-1.4.0.md
