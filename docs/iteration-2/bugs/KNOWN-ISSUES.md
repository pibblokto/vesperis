# Known issues, second iteration

Carried over from `docs/bugs/KNOWN-ISSUES.md` (still open there) plus what the second iteration found. Ids `KI-NNN` are the first iteration's, `KI-2NN` are new.

| Id | Sev | Area | Description | Status |
| --- | --- | --- | --- | --- |
| KI-003 | S3 | audio | Audio synthesis was never verified by ear; levels, timbre and click-free buffer transitions are unconfirmed (the WAV dump of `vesperis_test audio` is the only check). | open |
| KI-005 | S4 | landing map | Equirectangular map distorts the poles; the cursor stops at +-88 degrees. | open |
| KI-010 | S4 | surface | Birds keep a fixed altitude above the flock centre and can clip through hills; grazers can get stuck against steep cells. | partly fixed 2026-09-26 (N3: flocks land and perch, animals turn at steep cells and water); the cruise altitude is still relative to the circle's centre |
| KI-011 | S4 | text | Only uppercase glyphs exist; long names are truncated with a dot. | open |
| KI-014 | S4 | generation | Determinism across compilers is not guaranteed (floating-point noise). | open |
| KI-019 | S4 | astronomy | The primary star sits still: the companion orbits it instead of both circling a barycentre; the companion's worlds have no moons. | open, by design |
| KI-020 | S4 | surface | With two suns up, terrain cast shadows follow the blended light; only blob shadows double. Rings seen from a moon are grey (sky bank). | open |
| KI-021 | S3 | platform | The Emscripten build has never been run (no emsdk on the development machine). | open (N5-04 blocked on tooling) |
| KI-212 | S4 | look | Visor glints are drawn on the surface only (the sun's screen position is known there); in space the cabin glass shows none. | open, by design |
| KI-213 | S4 | space | Nebula patches appear only inside the arms' nebula cells (the `STAR-FORMING REGION` label), never from a neighbouring region looking in. | open, by design |
| KI-214 | S3 | project | The tree is not a git repository and has no GitHub remote, so the CI workflow has never run (N5-04). | open, needs the user |
| KI-022 | S4 | photo | The recorder writes one PNG per frame; recording lowers the frame rate and keeps no audio. | open, by design |
| KI-023 | S4 | photo | Panoramas kink at the two seams (three planar views). | open, by design |
| KI-201 | S2 | look | Three unrelated colour systems decide what a world looks like: `typeColor` for the globe, the fixed `matCol` table for the landing map, `SurfaceLook` per type for the ground. They agree only by accident (B-202). | resolved 2026-09-26 (N1: `BodyGen::matColor` drives all three views; consistency 0 of 13 over the threshold) |
| KI-202 | S3 | space | Comets move at realistic angular rates, which are below one pixel per 20 s at x1 from anywhere but their own vicinity; nothing else about them animates, so they read as frozen (B-201). | resolved 2026-09-26 (N0-01: streaming knots, dust tail, jets, readouts; the rates stay honest) |
| KI-203 | S3 | surface | Vegetation is a spray of 3-5 blobs per tree on a one-pixel trunk with no undergrowth, no forest floor and one vegetation colour per world; biomes are hard to tell apart on the ground (R-201). | resolved 2026-09-26 (N2: `surface/flora.cpp`) |
| KI-206 | S4 | performance | The densest forests (tropical, with giants) cost about 12 ms per frame at 2x on an M4 (trees 5.8 ms of it) against the 6 ms hoped for in N2-04; the bench budget is 14 ms. Plains stay at 3.4 ms. | open |
| KI-207 | S4 | surface | No kelp or water plants under shallow water (the water quad is opaque in the rasteriser). | open |
| KI-208 | S4 | surface | Seasonal leaf colours follow the season value, which is symmetric in time: the spring shoulder shows the same autumn colours as the autumn one (no derivative of the season is available at a site). | open, by design |
| KI-204 | S3 | surface | Creatures are boxes with line legs, birds are V shapes, fins are triangles; three behaviours (wander, herd, flee) and no animation beyond leg swing (R-202). | resolved 2026-09-26 (N3: `surface/bestiary.*`, `surface/creatures.cpp`) |
| KI-215 | S4 | surface | From inside the buggy the only picture is the nose camera's (pan +-70 deg); there is no rear camera, so reversing is done on the outside view (`V`). | open, by design (R-204) |
| KI-216 | S4 | surface | The dust storm's wind factor is refreshed with the two-second wind update, so the HUD wind can lag the storm readout by up to 2 s (B-206). | open, by design |
| KI-209 | S4 | game | No binocular zoom to watch animals from afar, and species cannot be named by the explorer (the guide names stars and worlds only); sightings are logged under the generated name. | open |
| KI-205 | S3 | surface | The buggy is a flat box on octagonal wheels with a roll bar; from the seat only a HUD cowl and a dial show you are inside; top speed 65 km/h (R-203). | resolved 2026-09-26 (N4: `surface/buggy.cpp`) |
| KI-210 | S4 | surface | The buggy's headlights light the terrain cone only; trees, rocks and creatures in the beam stay dark at night. | open |
| KI-211 | S4 | tests | The flow's buggy step lands in a forest with no open run within 800 m, so it only drives a few tens of metres; the long drive lives in `vesperis_test drive` on a desert site. | open, by design |
