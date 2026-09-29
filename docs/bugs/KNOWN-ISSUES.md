# Known issues

Things I already know about. Ids `KI-NNN`. Severity as in `README.md`.

| Id | Sev | Area | Description | Status |
| --- | --- | --- | --- | --- |
| KI-001 | S3 | surface | Tree canopies are stacks of small square quads; the original used sprayed point clusters ("greenmush"). Looks blocky up close. | fixed 2026-09-26 (M1-01 point sprays) |
| KI-002 | S3 | surface | Airless worlds at high sun look flat: no cast shadows, low contrast between crater floors and rims. | fixed 2026-09-26 (M10-06 cast shadows, M10-11 blob shadows) |
| KI-003 | S3 | audio | Audio synthesis was never verified by ear; levels, timbre and click-free buffer transitions are unconfirmed. | open |
| KI-004 | S3 | sky | Eclipses are only handled when the parent body covers the sun's centre pixel; partial eclipses do nothing, and the sky does not darken. | fixed 2026-09-26 (M1-08) |
| KI-005 | S4 | landing map | Equirectangular map distorts the poles; the cursor cannot go beyond +-88 degrees. | open |
| KI-006 | S3 | surface | Small cracks can show between terrain LOD rings (no skirts). Mostly hidden by the mush filter. | fixed 2026-09-26 (skirt walls 1.5 cells deep on the outer edge of every ring) |
| KI-007 | S4 | surface | Walking more than ~100 km from the landing site: the local tangent frame is that of the site, so the horizon and sun altitude drift slightly. | fixed 2026-09-26 (`SurfaceView::reanchor` at 50 km: new site frame, capsule re-projected, heading corrected; `vesperis_test reanchor`) |
| KI-008 | S3 | space | Stars are monochrome while any planet disc is visible (only four palette banks). | fixed 2026-09-26 (M10-03: 16 banks, blue/red star ramps always present) |
| KI-009 | S3 | surface | First frame after landing spends 25-60 ms filling terrain caches (hitch). | fixed 2026-09-26 (no 16 m cells above 800 m, `prefetch` 3 ms per descent frame coarse to fine; `bench`: first descent frame 4 ms, worst of the first 60 about 18 ms) |
| KI-010 | S4 | surface | Birds keep a fixed altitude above the flock centre and can clip through hills; grazers can get stuck against water or steep slopes. | open, roadmap M4-05 |
| KI-011 | S4 | text | Only uppercase glyphs exist; lowercase is drawn as uppercase. Long names are truncated with a dot. | open |
| KI-012 | S4 | rendering | The mush filter shifts the image up by two pixels (faithful to the original's psmooth_64). | by design |
| KI-013 | S3 | save | Single save slot; saving during ascent resumes in orbit, saving during descent resumes at the site origin. | fixed 2026-09-26 (M0-04: three slots, autosave, descent/ascent resume) |
| KI-014 | S4 | generation | Determinism across compilers is not guaranteed: noise uses floating point, so maps could differ on another platform. Fine for solo play. | open, roadmap M7-03 |
| KI-015 | S4 | space | Aborting a Vimana flight leaves the ship in deep space with the nearest star's system loaded; approaching a body from there takes the normal approach time regardless of distance. | by design, revisit |
| KI-016 | S3 | surface | Water has no reflections and no waves; sun glitter only. | fixed 2026-09-26 (M1-04) |
| KI-017 | S4 | surface | Rain lines are drawn in view space and can appear while looking straight down at the ground. | fixed 2026-09-26 (drops placed in the world around the camera, depth-tested, wind drift) |
| KI-018 | S4 | hud | The compass strip and the top line can overlap the target bracket label when the body sits near the top of the view. | fixed 2026-09-26 (label moves below the bracket when it would enter the top 34 rows) |
| KI-019 | S4 | astronomy | The primary star sits still at the system origin: in a multiple system the companion orbits it instead of both circling a barycentre; the companion's own worlds have no moons. | open, by design for now (M5-01) |
| KI-020 | S4 | surface | With two suns up, terrain cast shadows follow the blended light direction (between the suns); only blob shadows under objects double. Rings seen from a moon are drawn in the sky bank, so they are grey rather than the ring's colour. | open (M5-01, M5-04) |
| KI-021 | S3 | platform | The Emscripten build (`web/build_web.sh`) has never been run: the main-loop callback, the IDBFS persistence and the threaded terrain generation in the browser are unverified. | open (M6-04) |
| KI-022 | S4 | photo | The recorder writes one PNG per frame (about 2-8 ms each at 2x, more at 4x), so recording lowers the frame rate; it keeps no audio. | open, by design (M6-05) |
| KI-023 | S4 | photo | Panoramas are three planar views side by side: straight lines kink at the two seams. | open, by design (M6-05) |
