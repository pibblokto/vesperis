# Plan: more star classes

Status: proposed 2026-09-30. Pairs with `PLAN-galaxy-scale.md` in one generation bump.
**S-01 done 2026-10-01**: the four plain rows (S06 red dwarf with its flares and locked near worlds,
S07 blue-white, S08 orange giant, S09 carbon star) as *varieties* that take a share of the six
families by region (`starVariety`, a hash of the sector's own), so the stars they do not claim keep
their class: the pinned sites, the home star and the regress held. The showpieces (S-03..S-06)
join the same way. **S-02 done 2026-10-02**: the star map's filter cycles through the classes
(`C`/`X`, `0` all), the statistics and the analyzer read the table, `starActivity` is the one
activity table. **S-03 done 2026-10-02**: the neutron star as S10 (the codes follow the order of
arrival: the table below is renumbered), a share of the pulsars, a system of survivors with belts,
the outer one or two worlds glassed (`TR_GLASSED`, `MAT_GLASS`). **S-04 done 2026-10-02**: the
protostar as S11, a share of the yellow stars where stars form (a trickle in the arms), its soft
disc and edge-on dust band in `drawSun`, its cloud as nebula patches in both skies, belts rather
than worlds. **S-05 done 2026-10-02**: the Wolf-Rayet star as S12, a share of the blue giants that
stayed (mostly in the arms, none in the old populations), the shell's ring round the sun in both
views, the wind as the pick (no atmosphere survives) and as the giants' streaming envelopes, the
hazard over the whole system. **S-06 done 2026-10-02**: the black hole as S13, a share of the pulsars
that stayed (most in the core and the bulge), the shadow with the sky lensed round it, the accretion
disc arching over the shadow, a jet on some, the companion close and drawn out with its stream, rocks
and rubble for worlds. The fourteen rows of the table are in.

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
| S10 | **neutron star** | showpiece, done (S-03) | The pulsar's quiet sibling: a point of white light with a faint glow, no beams, x-ray glare, a system of survivors: bare cores and captured rocks, one or two worlds in wide orbits with a surface of glass. |
| S11 | **protostar** | showpiece, done (S-04) | A young star still in its nebula: the sun is a soft glowing disc with an accretion cloud, the system is dust, rings and belts rather than planets, a few molten worlds, the sky full of the nebula's glow. Found only in nebula regions. |
| S12 | **Wolf-Rayet star** | showpiece, done (S-05) | A massive star shedding its outer layers: blinding blue-white, a bright shell of gas round it seen as a ring in the sky, a violent wind that strips atmospheres, radiation hazard everywhere, few and bombarded worlds. Rare, in the arms. |
| S13 | **black hole** | showpiece, done (S-06) | No disc at all: a hole in the star field with the stars round it bent into arcs (lensing in the sky renderer), an accretion glow if it feeds (a thin bright ring seen edge-on, warm on one side), a jet on some. Its "planets" are whatever survived: wandering rocks, a companion star being drawn out (the companion's atmosphere streams toward it). Time warp near it does nothing special, but the data sheet gives the mass. Rare, in the core and the bulge, a few in the disc. |

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
