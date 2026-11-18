# R-425: microorganisms on every world, searched for and studied aboard

status: done 2026-11-17 (planned 2026-11-16 as block Z of `docs/ideas/ITERATION-1.5.0.md`: Z-01 to Z-07; the design in
`docs/ideas/IDEAS-small-and-spacious.md` section 1); Z-01 (where life is, generation 17) and Z-02 (what it looks like) done 2026-11-16;
Z-03 (the search), Z-04 (the microscope), Z-05 (the cultures), Z-06 (the genome and the tree) and Z-07 (the record) done 2026-11-17
milestone: 1.5.0

## Request

The user's request of 2026-11-16, whole: "Ok, let's prepare a milestones for release 1.5.0. What I want you to do is to add
microogranisms mechanic that would allow you to search for and investigate microorganisms that can be found on any planet. They
should also have some sort of complex procedurall look generation and in game animations when you bring them to the ship and
investigate. They should be on different stages, you can explore them, you should come up with some interesting mechanics for what
you can do with them on the ship. Also another thing I want you do include is expanding stardridter so that it's way way more
specious - right now it's "utilitarian" as one of the players described. For now prepare milestones please and create a 1.5.0
release branch"

The microorganisms are R-425, the ship R-426. The branch `release/v1.5.0` was made the same day from main bb0e58b.

What exists: life is seen in bulk on the ground and never studied. The archean world has mats and stromatolites; the chemotroph has
vent mats, sinter chimneys and spores glowing at night; the lichen world has crusts and moss; Q-02's traits give the hot springs'
thermophile mats, the pink lakes, the milky seas and the blooms. Flora and fauna live on eight kinds. Nothing small exists, and
nothing found on foot is carried; no body has an age or an origin of life, and nothing passes between worlds. The 5 m cabin has no
room for a laboratory.

## Plan

`docs/ideas/IDEAS-small-and-spacious.md` section 1 holds the stages, the chemistries, the species, the geneses and kin, the form
genome and its six ways of seeing, the animations, the search and the lab's mechanics, with their sources. The milestones are in
the iteration file, after the ship's I-01 to I-04 (R-426), which give the lab its room:

* **Z-01, where life is:** every landable world's biota (the stage its life reached, one of four chemistries, the genesis it descends
  from, 3-16 species with niches, ways to live, limits, life cycles and behaviours); kin across a system's worlds, across stars in
  64-sector cells, and from a drifter's origin; generation 17, nothing of 16 moving.
* **Z-02, what it looks like:** a form genome of about forty values by plan and lineage, drawn six ways (brightfield, phase contrast,
  darkfield, fluorescence, the electron microscope, a polarised thin section), and the life cycle's animations in closed form;
  plates for the user to judge before the microscope is built on them.
* **Z-03, the search:** the microhabitat at the crosshair, a kit of eight vials, the activity read on the spot, the helmet lamp's
  ultraviolet, the ground's signs (the hot springs' bands and red snow among them), the capsule's scoop for cloud life.
* **Z-04, the microscope:** a camera you enter, its six ways of seeing, zoom, focus, tracking, a spot of light, a magnet, a warm stage,
  a drop of food; micrographs; names and the species' sheets.
* **Z-05, the cultures:** the incubator's twelve flasks and media, growth through each species' stages, spores woken after thousands
  of years, two species in a flask (winners, cycles, feeding, mirrored strangers), strains, the world in a jar.
* **Z-06, the genome and the tree:** the sequencer, kin across worlds and stars, the geneses counted, a drifter's life beside its
  origin's; the organisms a futuristic people made, their watermark read by the decoder.
* **Z-07, the close:** the catalogue, the LIFE statistics, lending, auto-explore's samples, the docs and the battery.

## Done

* **Z-01 (2026-11-16), where life is:** `src/life/` (`docs/reference/13-life.md`): every landable world's biota, its stage, chemistry,
  genesis, habitats and 3-16 species (2-4 protocells, 1-4 spores, 2-4 fossils), each with its niche, way, plan, limits, cycle,
  behaviour and seed; the furnaces sterile, nothing in a giant's clouds, nothing fed by light in the dark or under ice; the
  geneses by system and kin cell, home's pinned; spores carried to airless neighbours and kept by drifters; generation 17, nothing
  of 16 moving (`docs/decisions/log/7-1.5.0.md`).
* **Z-02 (2026-11-16), what it looks like:** `life/morphs` and the slide (`life/morph_draw.h`; `docs/reference/13-life.md`): every
  species' form of about forty values by plan (27, with 75 variants), stage, chemistry and its genesis's style; no two of a world
  alike; drawn in six ways of seeing (bright field, phase, dark field, fluorescence, the electron's mesh, a polarised section); the
  life cycle's forms and seventeen acts closed-form in the slide's clock; `life plates`, `strips`, `distinct` and `forms` for the
  user to judge before Z-04 builds the microscope on them. Judged on 2026-11-16: "microbes pictures you made today looked great".
* **Z-03 (2026-11-17), the search:** a sample of the ground at the crosshair (`O` on foot, eight vials a descent; `surface/sampling`,
  `life/samples`): its microhabitat read from what the world fixes at its square metre, named on the HUD with its temperature and the
  activity read on the spot (NONE, FAINT, STRONG); what it holds a pure function of the place and the biota; the capsule's scoop in a
  venusian or acid world's clouds; the vials to the lab's cold store (64 places, the save's lines) at the climb aboard; the lamp's
  ultraviolet (`Shift+U`), the living films glowing in its reach; red snow, a life trait given to cold living worlds without a
  people, and the hot springs' mats in bands by their heat; `life search`, `life kit` (`docs/decisions/log/7-1.5.0.md`).
* **Z-04 (2026-11-17), the microscope:** the lab's first device, a camera entered on the cold store's vials: a vial's slide (`life/slides`)
  in six ways of seeing, zoomed, focused, a cell tracked, the four things done to it; what is seen of a species the guide's (its sheet),
  its form's first sight logged (FORM), names (N) and micrographs (P); `lab microscope`, `lab time`.
* **Z-05 (2026-11-17), the cultures:** the incubator's twelve flasks, each a medium and a culture of one or two species closed-form on the
  lab's clock (`life/cultures`): its stages, spores woken with their years, a luminous crowd's glow, two in a flask, strains; the jars'
  shelf (`life/jars`); `lab cultures`, `lab jars`.
* **Z-06 (2026-11-17), the genome and the tree:** a species' genome and its lineage in closed form (`life/sequences`: kin by world, system
  and the kin cell's boxes, none across geneses); the sequencer (90 s on the lab's clock, its letters in its genesis's alphabet, KIN) and
  the tree of kin's screen (every species read and the ship's own, the geneses counted, a kin's star the target); on the late peoples'
  worlds a made species with a watermark, a record of its people read on the decoder (`peoples/made`, the grammar's `watermark`);
  `lab tree`, `lab kin`, `life made`.
* **Z-07 (2026-11-17), the record:** the catalogue in the guide (every species seen, by world or by kin, a sheet beside its form drawn
  live, names given from it); the statistics' LIFE page and the data sheet's microbe line, every count the explorer's own; samples
  counted and logged (SAMPLE); a friend's species' sheets lent by their inbox file, in cyan and in no count; auto-explore's samples at
  the water and the springs; the reference note (`13-life.md`), the cookbook's recipes, the battery's items; `lab catalogue`, `life auto`.
