# Rules: the ship's devices, its flight and the maps

The rules in force for the ship's devices (the cabin, the radar, the telescope, the decoder, the player, the case, the probe), its
flight and the maps, a line each with the entries behind them (`log/`, by release). A milestone that sets or changes one edits its
line here; a replaced one moves to `superseded.md` (`docs/CLAUDE.md`). The rest of the game: `rules-game.md`.

## The cabin and its instruments
- Every device has a row on the devices page; each cabin computer a bank and colour of its own, 0.3 apart in RGB (a unit check) [W-01], [W-06], [R-404]
- An instrument looked through is a camera you enter (`radarCameraFeed`, `CAM 04 TELESCOPE`), not a HUD overlay; one at the window at a time [C-07], [W-06]
- At a parking the telescope's stabiliser holds parking and view in the body's frame (`tele.parkBF`, `viewBF`); stowed, the lap resumes [R-405]

## The radar
- A lock tells the system only (star as named, class, sector, distance, age; a drifter: NO STAR AT THE SOURCE): the sound tells, not the readout [C-07], [D-02]
- A signal is found, not handed over: nothing outside the 10-degree lobe, the hold on the loudest in the cone, a false rise never locks [C-07], [C-15], [C-16]
- `SignalKind` is append-only (`SIG_ROGUE` 4): a new kind needs its `SIGNAL_KIND_NAMES` name, `signalSourceLine`, a synth case, `signals wav` names [D-02]
- At x1 the radar is exactly C-07's; within 200 ly every transmitter is heard at every gain; a drifter within 180 sqrt((G - 1)p) ly [C-15], [C-16], [D-02]
- Past 200 ly a people of power p is heard at gain G within 200 sqrt(1 + (G - 1)p) ly while its age, its distance, is in `airYears` [C-07], [C-15], [A-01]
- The steps x1, x5, x11, x18, x25, x32 keep the pinned lists (from home 3 +2 +3 +2 +1 +1 peoples; no drifter to x5, six at x32) [C-15], [P-01], [D-02]
- The far scan runs in slices on the main thread (40,000 cells a frame), never a worker, and finds what the whole scan finds [C-15], [D-02]
- The radar's Enter sets the remote target and flies; a chart record's Enter only sets it [C-07], [C-10]

## The decoder, the player and the case
- Shards are read on the ship alone: a first reading resolves the words as the recording speaks them, Esc leaving it unread [C-06], [C-05]
- Music is never gated by the language: a piece plays whole at once and is recorded read at the full count (`decoded <key> 50`) [C-04]
- The decoder's history and timeline show only what read records told; its last age is named once three of its records, or the last, are read [H-06], [A-01]
- The last recording is never signposted and the timeline's end stays hidden until it is read: the reading is the moment [C-12]
- An instrument is keyed `<body>/I<people x 1000 + age x 100 + slot>`, a storm `<body>/TG` or `<body>/T<id>`: persisted encodings [C-11], [X-04]
- The ship's player plays in every state but a landing's and the title; the case's ten keys play only the instrument's scale degrees [C-11]

## The probe and the photos
- A probe record keeps numbers, never pictures, replayed by the live `renderProbeFeed`; its profile, layers and image stay as measured [X-04], [F-02]
- Probes are unlimited, one falling at a time, its relay cut if the ship leaves the parking; Shift held runs its clock x8, never a toggle [X-01], [R-406]
- `writePNG`'s bytes never change: an encoder change is proved file for file and by every chunk's CRC, the zlib stream and Adler-32 [F-02]
- Photos, recordings and the probe's image go through `writePNGAsync` (one `PngWriter` thread, in order, none dropped), read after `pngFlush` [F-02]

## Flight
- Every jump runs `jumpTarget` along the aim (`R`'s light years, the chart through `Game::plotJump`) and flies the one Vimana flight [R-407], [G-06]
- `vimanaProgress` is a flight's one closed-form curve: `updateShipMotion` moves the ship by it, the flight band's keyframes are built on it [G-07]
- The fine approach is one path for player and auto-explore (`updateShipMotion`, `APPROACH`): distance closed by equal ratios [R-419]
- The ship parks at 3.5 radii (the moons' spread is judged from there) and stands off a drifter at `drifterStandOffKm`, 40-80 radii [B-428], [D-01]
- A drifter is heard, never seen: its screens say NO STAR, S14 stays out of the classes seen, only a known one ends a chart's jump [D-01], [D-02]

## The maps and marks
- Maps show only the explorer's marks (`L` at the crosshair, Enter at the sector map's cursor, which names and removes); nothing by itself [R-408], [B-412]
- One mark at most over the view, none by default: Shift+Enter on the sector map shows or hides it, at any distance and on the compass [B-412]
- The scanner and the chart name nothing: the scanner gives a sight's kind (A CITY, A DAM) and bearing within 40 km, off by up to 8% [R-408], [T-03], [G-05]
- The landing map's peoples layer (`V`, `NET_KIND_COLOURS`) opens off; it shows what held records tell, the whole plan once all are held [R-415]
- The capsule never lands in a settlement (`settlementClearance` moves it out to the radius plus 30 m) [C-08]
- A sunless world's maps are the altimeter's survey (`mapColor`'s `survey`): lit all over, no night side, no clouds [D-02]

[C-04]: log/1-1.1.0.md
[C-05]: log/1-1.1.0.md
[C-06]: log/1-1.1.0.md
[C-07]: log/1-1.1.0.md
[C-08]: log/1-1.1.0.md
[C-10]: log/1-1.1.0.md
[C-12]: log/1-1.1.0.md
[W-01]: log/2-1.2.0.md
[W-06]: log/2-1.2.0.md
[R-404]: log/2-1.2.0.md
[R-405]: log/2-1.2.0.md
[R-406]: log/2-1.2.0.md
[X-01]: log/2-1.2.0.md
[X-04]: log/2-1.2.0.md
[R-407]: log/2-1.2.0.md
[R-408]: log/2-1.2.0.md
[G-05]: log/3-1.3.0.md
[G-06]: log/3-1.3.0.md
[G-07]: log/3-1.3.0.md
[A-01]: log/3-1.3.0.md
[C-15]: log/3-1.3.0.md
[C-16]: log/3-1.3.0.md
[T-03]: log/3-1.3.0.md
[H-06]: log/3-1.3.0.md
[C-11]: log/3-1.3.0.md
[B-412]: log/3-1.3.0.md
[R-415]: log/3-1.3.0.md
[F-02]: log/4-1.3.1.md
[R-419]: log/5-1.3.2.md
[B-428]: log/5-1.3.2.md
[P-01]: log/6-1.4.0.md
[D-01]: log/6-1.4.0.md
[D-02]: log/6-1.4.0.md
