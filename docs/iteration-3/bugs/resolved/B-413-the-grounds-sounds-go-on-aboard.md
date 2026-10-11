# B-413: the ground's sounds go on aboard

status: resolved 2026-10-10
severity: S2
area: the sound (`AudioState`: the surface's levels, the capsule, the ship, a load)

## Report

The user, after B-412: "when you are on surface and there is sound of wind, or jetpack or other sound and you can to stardrifter
that sound persists on stardrifter unless you quit the game and start it again".

## Cause

`Game::updateSurface` sets the ground's sounds in `Game::audio` every frame, and the synth follows each level to its value and
holds it there: the wind (rising with the height over the ground: the jetpack has no sound of its own, what is heard flying is
the wind), the rain, the lava, and from later milestones the waves and the birds (M4-09), the breath after a sprint (M8-02), the
leaves (N2-06), the hooves and the insects (N3-05), the buggy's gear and skid (N4-04) and the drone's pods (R-403). Off the ground
nothing set them back but the space view's `audio.wind = 0; audio.rain = 0; audio.lava = 0;`, which the later ones never joined.
The capsule's climb kept every level of the last frame on the ground for its seven seconds, the wind with them; aboard, the
breath, the waves, the birds, the leaves, the hooves, the insects and the pods went on at that level for good (the waves and the
breath are the wind's own noise in slow surges and in pulses, heard as wind; the pods spin down for a few seconds after the
explorer steps out of the drone). A load did not touch them (`Game::load` resets the radar, the player, the telescope and the
probe), so only another landing, which sets them all again, or a new start of the game ended them.

## Fix

* **One list** (`game/audio.h`): `AudioState::surfaceOff()` sets every level the surface sets to 0. It stands beside those levels
  in the struct, so a sound added to the ground is added to it.
* **Off the ground** (`game/game_states.cpp`, `game/autopilot.cpp`, `game/persistence.cpp`): every frame of the capsule's descent
  and climb (the ground's sounds fade as it lifts off, as none is heard on the way down), every frame aboard (in place of the space
  view's three), and at a load, whatever it comes back to (the probe's screen, a descent or a climb resumed; a ground loaded sets
  its own on its first frame). The synth's smoothing fades them out in about a second.

The ground's sounds still go on under what is opened on the ground (the menu, the maps, the guide): the explorer is still there.

## Verified

One new unit check (293 in all), a game of its own after the cabin's: landed on the home world (`testLandSite`), five seconds of
sprint, the capsule called with `K` and boarded with `Q`. The last frame on the ground holds wind 0.27, breath 0.45, birds 0.60 and
leaves 0.06, 0.0135 rms over the hum through the synth. Before the fix the climb held all four, the ship held the breath, the birds
and the leaves (0.0064 rms over the ship's hum), and a save of the ship loaded on the ground (after a save of the ground loaded,
its wind blowing) held them too: the check failed alone of the 293. Now a second into the climb, a second aboard and a second after
that load every level is 0, and the game's sound through a fresh synth is the ship's hum alone to the sample over two seconds.

`unit` 0 failures (3:50), `flow` exit 0 (36.8 s), the regress unchanged (22 hashes), `bench check` within every budget
(the people's descent 71.8 ms of 80; 77.8 in a first run, the timing's spread).
