# Plan: movement and vehicles (milestone M8)

## Where movement stands today

`SurfaceView::update` implements a minimal first-person controller:

* Walk 4.2 m/s; holding `Shift` multiplies by 2.3 (so a sprint exists, but it is silent, has no feedback and no cost). User feedback (2026-09-26): the sprint works but needs improvements, and it could be faster.
* Velocity eases toward the target with a time constant of 0.1 s on the ground and 0.5 s in the air.
* Jump at 4.6 m/s regardless of gravity: about 1.1 m on an Earth-like world, 12 m and ten seconds of flight on a 0.9 m/s^2 moon, a hop on a 2 g world.
* Slopes with rise/run above 1.35 stop you dead (no sliding). Small rocks and trees are drawn but have no collision: you walk through them.
* Swimming: automatic when the water is deep, speed x0.45, no diving, no waves felt.
* No head bob, no landing impact, no footsteps, no stamina, no crouch, no autowalk, no vehicle.
* Camera pitch clamped at +-85 deg; mouse sensitivity fixed at 0.0032 rad/px.

Terrain is unbounded, so long trips are possible but slow: 10 km on foot is forty minutes of holding `W`.

## Goals

1. Walking should feel physical: weight, momentum, ground contact, the planet's gravity.
2. Sprinting should be a deliberate choice with feedback (view, sound, breath, pulse) and a small cost.
3. Distances of 5-50 km should be practical and fun: a small buggy, plus a jetpack for vertical exploration.
4. Everything stays deterministic and saveable, and every item has a headless check.

## Items

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| M8-01 | **Walking feel**: head bob driven by a stride phase (amplitude with speed, off in the air), lateral sway, landing dip proportional to fall speed, slide down slopes above the limit with acceleration `g sin(slope)`, step-up onto obstacles below 0.5 m, air control kept small | M | - | a scripted walk over a hill in `flow` shows bob and a slide frame; no jitter when idle |
| M8-02 | **Sprint as a mode**: hold or toggle (setting), stamina 0-100 draining while sprinting (faster on high gravity), recovering when walking or standing; FOV widens 4 deg while sprinting; breath and heart-rate readout (the original showed a pulse); sprint speed scales with gravity (`x2.3` on 1 g, `x1.7` on 2 g, `x2.8` on 0.5 g); **user asked for a higher top speed**: raise the base multiplier to about `x3.2` (13.5 m/s on 1 g, a fast run) with a short acceleration ramp of 0.6 s so it reads as effort, and make the multiplier a setting (2.5-4.0) | S | M0-02 for the setting | HUD shows stamina and BPM; sprint stops when stamina is empty and resumes after recovery; a 100 m dash in the flow test takes about 8 s on 1 g |
| M8-03 | **Jetpack** (moved here from M4-01): hold `Space` in the air to thrust up (3 m/s^2 above gravity), `W/S/A/D` translate, ceiling 300 m above ground, altitude and vertical speed on the HUD, soft landing damping, heat bar that limits continuous burns to ~20 s, cool-down; louder wind when high | M | M8-01 | scripted flight in `flow` reaches 100 m and lands without a hard stop |
| M8-04 | **Autowalk and speed presets**: number keys `1-9` set a held forward speed (like the original's fixed step), `0` stops; `Shift` still sprints on top; any backward input cancels | S | - | walking continues with no key held in the test |
| M8-05 | **Object collision**: rocks larger than 0.6 m and tree trunks become cylinders in the current and neighbouring cells; the player is pushed out along the normal; small rocks are stepped over | M | - | walking straight into a large rock stops in the test; no tunnelling at sprint speed |
| M8-06 | **The buggy** (see design below): deployment from the capsule, enter/exit, arcade driving physics on the terrain, first-person dashboard view and third-person chase view, dust trail and tyre tracks, engine sound, headlights, HUD speedometer and odometer, parked buggy drawn and saved | XL | M8-05 | scripted 2 km drive in `flow` over hills at 60 km/h without stalls; save and reload keep the buggy where it was |
| M8-07 | **Long-range trips**: re-anchor the site frame when 50 km from the origin (KI-007), waypoints set from the sector map with a HUD marker and distance, a "return to capsule" bearing line on the ground, capsule recall (`K`) disabled while the buggy is more than 200 m away unless you drive back (so the buggy is never lost) | M | M8-06, M4-02 | 60 km round trip in the test keeps the horizon level and the sun altitude continuous |
| M8-08 | **Water**: wading speed by depth, diving with `Ctrl` (buoyancy returns you up, eye under the surface gets a blue palette and short fog), wave bob at the surface, swimming stamina | M | M8-02 | underwater frame in `surface 3` |
| M8-09 | **Gravity tuning**: jump velocity capped to a 6 m apex on very low gravity, extra air control below 3 m/s^2 ("hopping" worlds), heavy stomps and slower acceleration above 15 m/s^2, fall speed cap by atmosphere density | S | M8-01 | table of measured jump heights per gravity in the test output |
| M8-10 | **Controls**: mouse sensitivity, invert Y, toggle-or-hold sprint and crouch in the settings; gamepad analogue movement in the platform layer | S | M0-02, M6-02 | settings visible and persisted |
| M8-11 | **Postures**: crouch (`C`) lowers the eye to 0.9 m and slows; "stand on hind legs" (`S` held while stopped) raises it to 2.1 m for a better view, a nod to the original's felisian explorer | S | - | frames at both eye heights |

## Status (2026-09-26)

All items implemented in `SurfaceView::updateWalking` / `updateBuggy` / `drawBuggy` (see `docs/reference/06-surface.md`, "Movement"). Differences from the design above: the sprint multiplier is 3.2 on 1 g scaled by `(9.8/g)^0.44` and clamped to 1.5-4.0 (the user asked for a faster run), with a 0.6 s ramp and a 4-degree FOV kick; hind legs are `Z` held (not `S`); there is no wave bob when swimming; the buggy's dashboard is drawn in the HUD layer (a cowl with a dial) rather than as bank-3 quads; waypoints are set with `M` at the current position until the sector map (M4-02) exists; gamepad input waits for M6-02. The flow test sprints, jumps and hovers on the jetpack, unfolds the buggy, drives 150 m with a turn, switches to the chase view and reloads with the buggy in place.

## Buggy design (M8-06)

**Deployment.** The capsule carries a folded buggy. Within 10 m of the capsule press `B`: it unfolds beside the capsule over two seconds. Walk to it and press `E` to get in; `E` again to get out. It stays where parked (position, heading saved with the surface state) and is drawn at any distance as a low-detail model, with a HUD marker and distance when on foot.

**Physics (arcade, deterministic).** State: position `(x, z)`, heading, signed speed, steering angle, vertical velocity when airborne, wheel spin.

* Throttle `W/S`: acceleration 4 m/s^2 forward, 2.5 reverse; drag `0.02 v^2`; rolling resistance by material (sand 2x, snow 1.5x, dust 1.1x, rock 1x, ice 0.5x).
* Top speed 18 m/s forward (65 km/h), 5 m/s reverse; slope adds `-g sin(slope) 0.8` along the heading; climbs above 30 deg stall.
* Steering `A/D`: angle up to 30 deg, turn rate `v tan(steer) / wheelbase` (2.2 m), reduced traction on ice and snow (lateral slide, delayed heading response), handbrake `Space`.
* Contact: terrain height sampled at the four wheel positions (track 1.6 m); chassis pitch and roll from the front/back and left/right pairs; camera follows with a damped spring (suspension), plus vibration proportional to speed and local roughness (height difference over 1 m).
* Airborne when the chassis rises above the interpolated ground by more than 0.2 m after a crest: ballistic with the body's gravity; landing shakes the camera and costs speed.
* Collision: large rocks and trees as cylinders (M8-05); hitting one stops the buggy and pushes it back; water deeper than 0.4 m stops it at the shore.
* Low gravity: long floaty jumps off dunes; high gravity: slower climbs, stiffer suspension.

**Rendering.** First person from the seat: a dark dashboard silhouette with a speed dial at the bottom of the frame, hood edge and wheel arches at the sides (bank 3 quads). `V` toggles a third-person chase camera 4 m behind and 2 m above; the buggy is a chassis box, four octagonal wheels spinning with the odometer, roll bar and an antenna light. Effects: dust or snow puffs behind the wheels (points that fade over 1.5 s, more at speed), tyre tracks as a ring buffer of the last 400 wheel contact points drawn as small dark ground quads, headlights at night that brighten terrain quads inside a 25 m cone ahead (shade added per quad, cheap because the terrain is flat shaded), engine tone with pitch from speed and load, gravel and wind noise.

**HUD while driving.** Speed in km/h, heading, trip odometer, distance and bearing to the capsule, stamina hidden.

**Persistence.** `buggy <deployed> <x> <z> <heading> <odometer>` line in the save file; the flow test parks it 300 m away, reloads and finds it.

**Tests.** A scripted drive in `vesperis_test flow` (throttle for 40 s across hills, turn, brake) with frames at speed, airborne and at night with headlights; a benchmark that the driving frame stays under 3 ms.

## Order

M8-01 and M8-02 first (they change how every landing feels), then M8-05 (collision, needed by the buggy), then M8-06 the buggy, then the rest. M8-03 (jetpack) is independent and can be slotted anywhere.
