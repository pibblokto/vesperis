# Rules: the search and the microscope

Rules for finding microbes and looking at them aboard (1.5.0, R-425: Z-03, Z-04; R-432, R-435 to R-438), one line per rule with its
log entries (`log/`); a replaced rule moves to `superseded.md`. Where life is: `rules-life.md`; incubator and collection:
`rules-life-cultures.md`; sequencer, tree and record: `rules-life-record.md`; the activity vision: `rules-surface.md`.

## The search
- A sample is `sampleOf` its microhabitat and the world's biota; the microhabitat is its square metre's, from what the world fixes, not the landing grid [Z-03]
- Activity measures cells at work: spores and fossils read none, a protocell's chemistry faint (it may mislead); no count of a world's species is shown [Z-03]
- Life seen on the ground lives there (its habitat at most 50 C). Mats no species lives in count as the springs' life; snow and ice read their melt [Z-03]
- Red snow is given by its own hash after the draw, into a free new slot with no life trait, to a cold living world without a people [Z-03]
- The ultraviolet draws after the old result, in bank 15 where its light beats the pixel's; switched off, its pass and ramp leave the frame untouched [Z-03]
- Eight vials per descent, the scoop's included, refused from a vehicle or once used up; the cold store holds any number (its door lit by doublings) [R-432]

## The microscope
- A camera you enter (`CAM 06 MICROSCOPE`): the whole slide in the framebuffer, every bank for its way of seeing, HUD on the canvas, no space view [Z-04]
- E opens the lists on what was last staged (Tab: vials in a folder per world, cultures by folder); the choosers go by folder, a row per species [R-432]
- A slide is its vial's mount (`mountOf`), a pure function of the sample, the microhabitat and the world's biota and forms; nothing of it is saved [Z-04]
- Diluted by tens, the most crowded species covers a sixteenth of the glass (a body over 20 um as 20), none over an eighth; each species gets a tile [Z-04]
- A tile holds a hundred bodies' worth at the species' density: at most 160 cells, at least one, at most 1.5 mm (a rare one is found by looking round) [Z-04]
- Actions on a slide are events on its clock; where a cell is and what it does is closed-form, the same skipped to, stepped or on one thread [Z-04]
- Every cell within reach that turns to light or food answers, packing round it to twice the spot's radius; a field-turner answers only the field [R-438]
- A kind moves while the stage warmth is in its range, at the cardinal pace relative to the vial's warmth; above the top of the range it is stressed [R-438]
- Swimmers of the dark (soil, silt, a vent, rock, a plume; no light-feeders) flee a bright spot; a spot on live light-feeders draws eaters after 10 s [R-438]
- Spores, cysts and microcolonies are still (they only jiggle); a stressed kind, moving or still, acts where it is and ends still in its final form [Z-04]
- The plates hold a spore, a cyst or a fossil lying on the slide still, as the microscope does (`panelSpecimen`) [B-503]
- L or F again undoes it. M puts the magnet by the stage, turns it half round, takes it away. H types the stage warmth, Shift+H sets the vial's [R-435, R-436]
- Each action's purpose is said as it is done; each kind in view is told once per slide as it answers, and its sheet keeps the answers (`answers`) [R-437]
- New fields default to Z-02's plates (`life_look` unchanged); `step` 2 only at 3x-4x. Electron meshes are built on the pool, drawn in row bands [Z-04]
- Under the electron the slide is kept while nothing that draws it moves (the mount, the view, the focus, what was done to it, the frame's size) [B-502]
- The guide keeps a part of a species (`microbe` by `<body>/M<index>`) only when seen in its way, in focus and big enough; the sheet shows only that [Z-04]
- The first sight of a form is said and logged (FORM). N names a species (`TEXT_SPECIES`), and P's photograph is the micrograph, captioned with the slide [Z-04]

[Z-03]: log/7-1.5.0.md
[Z-04]: log/7-1.5.0.md
[R-432]: log/7-1.5.0.md
[R-435]: log/7-1.5.0.md
[R-436]: log/7-1.5.0.md
[R-437]: log/7-1.5.0.md
[R-438]: log/7-1.5.0.md
[B-502]: log/7-1.5.0.md
[B-503]: log/7-1.5.0.md
