# Rules: the game and its screens

Rules in force for the game's states, screens, controls and saves, one line per rule with its log entries (`log/`, by release). A
milestone that sets or changes a rule edits its line here; a replaced rule moves to `superseded.md` (`docs/CLAUDE.md`). Devices,
flight and maps: `rules-game-devices.md`.

## The explorer's rules
- No fuel, damage or fail state: hazards (radiation, a hostile star) only warn, and time near a black hole passes as anywhere [table], [S-06]
- Flights, approaches, descents, a probe's fall and drone trips take real seconds (`realTime`); the warp only turns the sky [table], [X-01], [C-18]
- Nothing has a name until the explorer gives one (`starNameOf`, `bodyNameOf`, else UNKNOWN); an aim marker shows a distance or VISITED [R-401], [R-407]
- Generated names stay in the generator (keys, ids), never shown or saved; no world's name comes from its star's [R-401], [H-04 second pass]
- A shard is taken with `E` within 2.2 m, never by walking over it, and nothing hints at one (the scarcity rule) [C-03]

## Saves, the guide and lending
- Player files: `vesperis_save_<n>.txt`, `guide.txt`, `vesperis_settings.txt`, `vesperis_keys.txt`, `vesperis_pad.txt` [Release 1.0.0], [R-418]
- Append-only: `GameState` (the save's `phase`), `ShipState::Mode` (`mode`), `AutoProfile` (`auto_profile`) and probe line codes [X-04], [R-419]
- Marks keep R-408's numbers in the file (`markKindToFile`: 8 for a place, 9 for a work, sights from 10); new sights are appended [T-03], [Q-01]
- A shard is keyed `<body>/S<people x SHARD_STRIDE + index>`, found by hand or by drone: the contents may change, the key never [T-04], [C-18]
- One line per field: `load` skips unknown keys and reads a missing kind of line as none, so the files grow by lines [C-03], [C-14]
- A format change gets a line of its own (the guide's `records 3`), never a `GEN_VERSION` bump; an older guide is migrated or reported once [T-04], [B-414]
- Save, guide and settings open with a version line; raising the settings version brings a changed default to older files once [B-310]
- Saves and the guide record `gen`: an older save loads with one notice, and one whose star is gone does not load [M9], [G-01]
- The guide's save is byte-stable: loaded and saved again it is the same file, except for the lending clock [F-02]
- Not saved: the telescope, the jump plot, an almanac run (`warp` reads 1), the peoples layer and a probe's held frame [W-06], [G-06], [W-01], [R-415]
- A lent guide holds only the friend's own finds (Z-07: species sheets): no lent finds, pictures, samples or flasks; cyan, uncounted [C-14], [X-04], [Z-07]

## The HUD and the screens
- The HUD stays minimal (the user's ask): on foot the epoch, name and place, scanner, compass, stamina and what E or Q would do; the rest on Tab [R-414]
- What the HUD drops is deleted, not hidden: no path travelled is kept or drawn (no `route` lines or trail); buggy and drone share one nose-camera HUD [R-414]
- The title screen shows the name and the choices, no version; the credits name only the inspiration, Noctis IV [Release 1.0.0]
- A frame-wide line holds 53 characters (52 inside the margins). A list that grows with a table wraps; a label with no room is dropped, never cut [S-02], [H-06]
- A count uses the other screen's words (`ABOARD: N OF ITS M SHARDS`), never a third number: the user reads numbers literally [C-18]

## Controls and the frame
- The frame never waits for a thread's result (`mapIfReady`, `shardTextsIfReady`, `worldPlanIfReady`): only the harness blocks [H-04], [F-02], [D-05]
- Both mains join the background threads at exit (`worldPlanThreadsJoin` via `atexit`) before static destructors run [H-04], [C-18]
- The landing map builds no drainage tile: it asks once the cursor rests 0.4 s and draws the zoom dry until tiles come [O6-03..O6-07], [B-422]
- Key codes (`core/input.h`) keep raylib's numbers, and `vesperis_keys.txt` remaps physical keys at the input layer; no rebinding screen [table], [M6-02]
- The pad is read raw through GLFW (only in `platform_raylib.cpp`), bound by role per device, and only presses keys into `Input` (`padFrame`) [R-418]
- Y opens the command menu everywhere and its keyboard reaches every key; the pages are written per screen, so a changed key changes its line [R-418]
- Keys never repeat (`IsKeyPressed`), so a held key is coded explicitly; nothing needs PGUP or PGDN alone (a Mac needs FN) [R-416]
- A new key is checked against every binding in every state before it is taken [X-01]

## Auto-explore
- Auto-explore acts through the frame's input (`autoInput`, after `padFrame`) as a player could, except for the nose and `orbitShip`'s lap [R-419]
- Its HUD shows how to stop it (for the first five seconds) and the tape's caption at top left as a track starts; it opens no screen [R-419], [R-420]
- It plays the expedition (log, guide and autosaves keep what it does), names and marks nothing, saves to no slot; any input ends it, unacted on [R-419]
- Its six profiles share one mind and every step; each look eases on a damped spring within the limits of a hand [R-419]
- `M` starts and stops the tape only in auto-explore; the tape fades as auto-explore ends and resumes where it was (`auto_track_at`) [R-420]

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
[Z-07]: log/7-1.5.0.md
