# R-403: a drone to reach the ruins

status: done 2026-10-02
milestone: 1.1.0 (between C-05 and C-07, on the user's call)

## Request

Playing the C series on a world with a people's ruins: "it's just super hard to get to ruins on buggy". A small drone or
plane you get into on the surface, built like the buggy and flown through its camera, with a top speed of 320 km/h.

## Done

* **The drone** (`Drone`, `surface/drone.cpp`): the buggy's hull shortened (the wedge, the blacked-out sensor band, the
  camera pod on the nose, the mast with its blinking light, the lidar puck, the light bars) on two skids, with four ducted
  thrust pods on arms where the wheels were (the fans turn, a glow under each pod in flight, the arms unfold with the hull).
  `F` within 10 m of the capsule unfolds one on the capsule's other side from the buggy's (a drone already out, wherever it
  was left, is scrapped, as the buggy by `B`; a new landing starts with none; `K` names a drone left over 50 m away). `E`
  within 3.5 m gets in once it has unfolded; `E` gets out on the ground only (`LAND FIRST - HOLD SHIFT TO COME DOWN`) and
  reports the kilometres flown and the top speed. Refused on a comet like the buggy (at speed it would leave the world).
* **Flying** (`updateDrone`): Space spins the pods up and lifts it off (a moment's spin-up), then climbs at 15 m/s;
  Shift descends at 12 m/s and, touching the ground with the speed under 3 m/s and Space released, lands it; neither held,
  the height is kept. W/S thrust fore and aft (9 m/s^2 forward, 5 back, x1.8 against the motion; a quadratic drag with a
  little base drag, the clamp at 320 km/h and 12 m/s backwards); A/D turn at 70 deg/s at rest shrinking to 24 at the top,
  the hull banking into the turn (a coordinated turn's angle, at most 50 deg) and dipping its nose with the thrust and the
  speed, seen in the outside view only: the nose camera is on a gimbal, so the picture stays level. A ceiling of 400 m over
  the ground (`CEILING` on the HUD). The skids never go under the ground or the water (a hard or fast arrival is a thump
  and most of the speed; the hull lands level on water and the explorer steps out swimming); what the buggy collides with
  at the hull's height stops the drone too (B-405's spans: a wall or a trunk met low, flown over high); small rocks pass
  under the skids. The downwash throws dust, snow or spray up when hovering within 8 m of soft ground or water. No fuel.
* **The picture and the HUD**: the nose camera's CCTV feed (`cameraFeed`) with `CAM 02 DRONE` and its `REC`, the pan
  gauge (+-70 deg) and the tilt (-60..+25: further down than the buggy's, the ground is what you look for), the speed in
  double-size digits (`KM/H BACK` in reverse), `ALT` over the ground and `VS`, the trip, a `TURN` bar, the heading, the state
  (`LANDED`, `LIFTING`, `CLIMB`, `DESCENT`, `CEILING`, `HOVER`, `LIGHTS ON` or the temperature), the capsule and the local
  time; the rangefinder reads the time as a `FLIGHT` at 80 m/s; `V` the chase view from 7 m behind and 2.8 m above. The
  sector map marks `DRONE`; on foot the HUD shows `DRONE 123 M`. The headlight bar lights the ground under a low flight at
  night. The pods are heard (`AudioState::rotor`, `rotorPitch`: a blade-pass whine with two harmonics beating between the
  pods over a rush of air, its pitch rising with the speed and the climb; from outside, fading over 80 m), the wind of
  speed as the buggy's, thumps through the same channel; the birds fall silent in a vehicle.
* **Records**: `stat flown` in the guide (the statistics line `FOOT, DRIVEN, FLOWN`), `FLEW n M` in the launch log entry,
  `BY DRONE` in the sector entries. The save carries a `drone` line like the buggy's (a drone saved in flight loads landed
  where it was). The key bindings name `E` `USE / VEHICLE` and `F` `FIELD AMP. / DRONE`; the help's surface page has both
  vehicles' keys.
* **Tests**: `flow` unfolds it, lifts off, flies 16 s at full thrust with a turn (`drone_seat`, `drone_flight`,
  `drone_chase`), brakes, lands under Shift and gets out (top 320 km/h, 160 m up, landed in 13 s on the home world);
  `unit` flies one on a grassland site for 85 s (lifts, 320 km/h within 25 s, the ceiling, never under the ground, braked,
  landed in 35 s, out, a second one unfolds and scraps the first); `bench check` has an eleventh budget, `drone 2x`
  (60 frames at full thrust 200 m up with the camera tilted down 30 deg): 7.3 ms of 16.
