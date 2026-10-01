# Plan: more star classes

Status: proposed 2026-09-30, not scheduled. Pairs with `PLAN-galaxy-scale.md` in one
generation bump (the class of every star changes when the table changes).

## Today

Six classes drive everything through `STAR_CLASSES` (`galaxy/starfield.h`): colour,
luminosity, radius, temperature zones and the planet types that form, the first orbit,
the sun's rendering, hazards, the region weights, the description on the analyzer.
`S00` yellow star, `S01` orange dwarf, `S02` blue giant, `S03` red giant, `S04` white
dwarf, `S05` pulsar.

## Proposed table (fourteen classes)

Plain rows are a table entry, a description and a regress bless. Showpieces need
rendering or behaviour of their own, on the scale of the pulsar's work.

| Code | Class | Kind | What it brings |
| --- | --- | --- | --- |
| S00 | yellow star | existing | |
| S01 | orange dwarf | existing | |
| S02 | blue giant | existing | |
| S03 | red giant | existing | |
| S04 | white dwarf | existing | |
| S05 | pulsar | existing | |
| S06 | **red dwarf** | plain | The commonest star of all (two thirds of the galaxy). Dim, deep red, tiny; habitable worlds huddle so close they are tidally locked, with a day side and a night side and life along the terminator. Flares: a minute of doubled light now and then. |
| S07 | **blue-white star** | plain | Between the yellow star and the blue giant: hot, bright, short-lived, wide systems of bare rock and ice, strong ultraviolet (radiation on thin-atmosphere worlds). |
| S08 | **orange giant** | plain | An old star grown large but not enormous: warm amber light, the inner planets scorched, the habitable zone out among what were once the ice worlds (thawed ice shells, boiling-off comets). |
| S09 | **carbon star** | plain, with a look | A deep red giant wrapped in soot: the sun is dim and ruby, the sky around it hazy, the light on the ground turns everything to ember tones. Carbon worlds favoured. |
| S10 | **protostar** | showpiece | A young star still in its nebula: the sun is a soft glowing disc with an accretion cloud, the system is dust, rings and belts rather than planets, a few molten worlds, the sky full of the nebula's glow. Found only in nebula regions. |
| S11 | **Wolf-Rayet star** | showpiece | A massive star shedding its outer layers: blinding blue-white, a bright shell of gas round it seen as a ring in the sky, a violent wind that strips atmospheres, radiation hazard everywhere, few and bombarded worlds. Rare, in the arms. |
| S12 | **neutron star** | showpiece | The pulsar's quiet sibling: a point of white light with a faint glow, no beams, x-ray glare, a system of survivors: bare cores and captured rocks, one or two worlds in wide orbits with a surface of glass. |
| S13 | **black hole** | showpiece | No disc at all: a hole in the star field with the stars round it bent into arcs (lensing in the sky renderer), an accretion glow if it feeds (a thin bright ring seen edge-on, warm on one side), a jet on some. Its "planets" are whatever survived: wandering rocks, a companion star being drawn out (the companion's atmosphere streams toward it). Time warp near it does nothing special, but the data sheet gives the mass. Rare, in the core and the bulge, a few in the disc. |

Possible later rows: a yellow supergiant (a star about to go), a magnetar (a pulsar with a
radiation hazard that reaches the ship), a T Tauri variable, a binary of two white
dwarfs. The table makes any of them cheap once the showpiece machinery exists.

## Region weights

Red dwarfs everywhere and most common (about half of all stars in play; the true two
thirds would make the neighbourhood monotonous). Blue-white and Wolf-Rayet in the arms,
protostars in nebulae, orange giants and carbon stars in the bulge and the clusters,
neutron stars and black holes in the core and the bulge with a trickle in the disc.

## Where the code goes

* `STAR_CLASSES`: colour, luminosity, radius, `firstOrbitMult`, `minFirstOrbitKm`,
  hazard, flare, description; the class weights per region in `starInSector`.
* The system generator: temperature zones follow luminosity already; new switches for
  "no planets, only debris" (protostar, neutron star, black hole), "companion being
  drawn out" (black hole), tidal locking made likely (red dwarf).
* The space renderer: the sun's disc (the pulsar's beams are the model for the
  protostar's cloud and the Wolf-Rayet shell), lensing for the black hole (a screen-space
  warp of the star field within a few disc radii, cheap and convincing), the accretion
  ring as a thin ellipse.
* The surface sky: the sun's colour and size come from the class already; the carbon
  star's haze and the black hole's absence need a case each.
* UI: the star map's class filter is six bits on keys 1 to 6 (becomes a cycling filter
  or two rows), class codes S00 to S13, the guide's "classes seen x/6" reads the count
  from the table, the analyzer's description per class.
* Tests: `survey` prints the class mix per region; `space` renders a sun of every class;
  a `moonsky` or `scene` frame per showpiece; `regress bless`.

## Order of work

1. The four plain rows (red dwarf, blue-white, orange giant, carbon star) and the region
   weights: one afternoon, mostly table.
2. The neutron star and the protostar (debris systems, the nebula sky).
3. The Wolf-Rayet shell.
4. The black hole: lensing, the accretion ring, the drawn-out companion. The showpiece;
   a milestone on its own if it is to look right.
