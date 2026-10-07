# Bug reports, third iteration

| Id | Title | Severity | Status |
| --- | --- | --- | --- |
| B-301 | Rings are invisible from the surface (drawn near black on the sky bank) | S3 | resolved 2026-09-27 (O0-01), `resolved/` |
| B-302 | What you see may not be reachable: a shore in sight from a mountain landing was never reached; no sense of distance | S2 | resolved 2026-09-27 (O1), `resolved/` |
| B-303 | The sector map keeps the previous landing's marks (the trail, the waypoint, even the old terrain image) on a new world or sector | S3 | resolved 2026-09-27, `resolved/` |
| B-304 | A huge ringed parent in a moon's sky vanishes when the camera looks down and comes back when it looks up | S2 | resolved 2026-09-27 (O0-03), `resolved/` |
| B-305 | A ringed world next to a felisian world changes colour depending on which one you approach | S2 | resolved 2026-09-27 (O0-02), `resolved/` |
| B-306 | The moon's shadow seen on the globe is missing from the landing map | S4 | resolved 2026-09-27 (O0-04), `resolved/` |
| B-307 | The buggy hops off every downhill instead of driving it (a "ladder" descent) | S2 | resolved 2026-09-27 (O8-01), `resolved/` |
| B-308 | Space turns yellow when the camera swings past a companion star; the sun explodes near the edge of the view | S2 | resolved 2026-09-27 (O8-02), `resolved/` |
| B-309 | Ugly flickering round the sun when looking at it (the visor glint arcs) | S3 | resolved 2026-09-27 (O8-03), `resolved/` |
| B-310 | The picture is too mushy: a higher resolution with the original's feel | S2 | resolved 2026-09-27 (O8-04), `resolved/` |
| B-311 | The ground is plates in ugly squares; textures need variety and smoothness | S2 | resolved 2026-09-27 (O8-05), `resolved/` |
| B-312 | A huge ringed planet in a moon's sky vanishes when it is left or right of the view (two blind spots) | S2 | resolved 2026-09-28 (O9-01), `resolved/` |
| B-313 | Hills change shape in front of you ("hills morphing"), most of all when getting out of the buggy | S2 | resolved 2026-09-28 (O9-04), `resolved/` |
| B-314 | The parent planet in a moon's sky is sometimes gigantic (90% of the sky) | S3 | resolved 2026-09-28 (O9-03), `resolved/` |
| B-315 | Vegetation is ugly: leaves are pixels, logs are slabs, too little variety | S2 | resolved 2026-09-28 (O9-05), `resolved/` |
| B-316 | No animals to be seen, only the odd flock of birds | S3 | resolved 2026-09-28 (O9-06), `resolved/` |
| B-317 | A moon in the sky stands in the ground when boarding the capsule | S3 | resolved 2026-09-28 (O9-02), `resolved/` |
| B-403 | Green blobs moving in the night sky of a high-latitude site round a pulsar (the aurora) | S2 | resolved 2026-10-01, `resolved/` |
| B-404 | The ground changes colour in front of you: a brown band ends a hundred metres out and moves along (the materials followed the rings' scale fades) | S2 | resolved 2026-10-01, `resolved/` |
| B-405 | Invisible walls in a town: the jetpack cannot fly over a low wall (the colliders had no height) | S2 | resolved 2026-10-02, `resolved/` |
| B-406 | The water pulls the jetpack in: you cannot fly over a lake (swimming was decided from the ground under you, whatever your height) | S2 | resolved 2026-10-02, `resolved/` |
| B-407 | The globe's relief is lit from the wrong side: M9-15's slope term was added to the star's cosine where ground rising toward the star faces away from it (found by the telescope's plate, which copied it) | S4 | resolved 2026-10-06, `resolved/` |
| B-408 | A pale wedge with straight edges round the sun of a comet: the sun's glow lit the galactic band's bank over the band's patch and the airless sky's (black under shade 30) beside it, the band stopped at the horizontal, and the mush rimmed the band's edges against the coma | S2 | resolved 2026-10-07, `resolved/` |
| B-409 | The probe's picture goes black in the clouds of a giant with one deck: its base was tinted by a smoothstep with equal edges (0/0, a NaN through the air's and the fog's colours); with it, a brown dwarf's fog clipped flat at its deck's base | S2 | resolved 2026-10-07, `resolved/` |
| B-410 | A planet approached from a belt pulls the ship back to the belt: the approach kept the belt's parking, which wins over the body's at the approach's end; the flight computer's next body kept the belt targeted | S2 | resolved 2026-10-07, `resolved/` |

`KNOWN-ISSUES.md` lists what stays open. New reports: copy `TEMPLATE.md` to `B-3NN-short-title.md`.
