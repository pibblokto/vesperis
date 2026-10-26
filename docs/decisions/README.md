# Decisions

The rules in force, a line each with the entries behind them, in a file an area of at most 6 KB: read the area's before
changing anything there.

- `rules-rendering.md`: the pixel and the palette, the banks, light, one world in every view, small things and the mush.
- `rules-rendering-frame.md`: draw order and depth, the same pixels at any thread count, the pool, the frame's time, exact
  arithmetic.
- `rules-galaxy.md`: `GEN_VERSION`, draws, hashes and salts, the protected and pinned worlds, stars and drifters.
- `rules-galaxy-worlds.md`: bodies and orbits, kinds and made kinds, the almanac and the giants.
- `rules-surface.md`: the planet function, materials and landforms, traits, water and drainage, sights, weather.
- `rules-surface-site.md`: the site's caches and threads, the rings, what stands and walks, the vehicles, life.
- `rules-peoples.md`: who lived where and when, the plan and the history, what stands on the ground, buildings and the genome.
- `rules-peoples-grammar.md`: where the records lie, signals and charts, texts and their draws, the grammar's files, tongues.
- `rules-game.md`: the explorer's rules, saves and the guide, the HUD and the screens, controls and the frame, auto-explore.
- `rules-game-devices.md`: the cabin's instruments, the radar, the decoder and the player, the probe and photos, flight, maps and
  marks.
- `rules-audio.md`: the synth, levels, voices, music and instruments, the radar's and the drifters' sounds, the tape.
- `rules-harness.md`: the harness's code, the unit, the flow and the tour, frames that repeat.
- `rules-harness-scenes.md`: the scenes, the pinned sites and the test worlds.
- `rules-harness-proof.md`: the regress, the battery, proofs by area, reading a difference, the bench, timing pairs.
- `rules-process.md`: working with the user, proving and fixing, recording the work.
- `rules-process-build.md`: the platform and the names, the builds, warnings, releases.

`superseded.md`: the rules replaced, and by what. `generations.md`: what each `GEN_VERSION` changed. `log/`: every entry
by release, `0-1.0.0.md` to the current release's, the newest last; `grep -n '^## B-428' log/*.md` finds one, then read
that entry alone, never a log whole.
