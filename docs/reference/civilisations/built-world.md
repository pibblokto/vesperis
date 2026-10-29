# Civilisations: the built world

One of the five notes on the dead peoples (`peoples-and-ages.md`, `built-world.md`, `records-and-grammar.md`, `sound.md`,
`radar.md`), split by topic from `12-civilisations.md` in E-04; its sections are as they were, in their order.

<!-- contents -->
Contents:
- Ruins that read the world (C-08)
- The roads (C-09)
- The forms the renderer needs (A-02)
- What each age builds (A-03)
- The architecture genome (A-04)
- A people's map of its world (T-01)
- Towns by age (T-02)
- The works (T-03)
- Where the records lie (T-04)
- No one's ruins, the land's works, and not the season's (B-417, B-418, B-419)
- The genome within each age, the walls and the records (R-412, B-420, B-421)
<!-- /contents -->

## Ruins that read the world (C-08)

The ideas note (section 3) asked that "a ruin's layout follows what the people had: a coastal culture leaves harbours on
the old shoreline, a mountain one terraces and cairns on the ridges, a desert one cisterns and long walls against the
wind. The terrain already knows all this, so the buildings come from the same data", and section 4 that the ruins weather
by the world's own climate "so the ruin tells you how long ago without a number". C-08 (2026-10-02) builds both on
C-01's settlements (`peoples/ruins.*`; `06-surface.md` has the pieces and the drawing).

*The terrain read.* A settlement reads the ground round it once, when its layout is made (`readSite`, kept in
`RuinSpec::site`): the grade across it and the heading it falls toward, the highest ground, the planet function's
relief at the centre, the nearest water or old sea within 500 m of its edge (the shoreline bisected to a few metres) and
the prevailing wind's heading (the dune fields' rule: the world's wind angle, flipped per hemisphere, turned per
30-degree band). From that it gets its features (`addFeatures`, `featureList`): a harbour on a shore (a quay along the
shoreline, a mole out into the water, a warehouse behind; on a dead desert world the quay and the mole stand over the dry
bed, which is the old shore made visible), terraces on a slope (retaining walls along the contour between the houses),
on a desert a cistern with its channel at the upslope edge and a crescent of wall against the wind, and cairns out along
the ridge in mountain country. Nothing is placed that the terrain did not ask for: a settlement on a flat inland plain
is as C-01 left it. On the desert test world's region 317 settlements are sited, 19 terraced, 123 with a cistern, 134
with a wind wall; the harbours are on the coasts (the scenes find them 60 km from the types' spots).

*Weathering by age.* The culture's decay was a draw; it is the age's now (`decayOf`): the years since the end are
hashed from the world's seed, 200 to 40,000 log-uniform, and a wet world (rain) decays a fifth more, so the two test
worlds sit at the ends: Leileashphail III ended 209 years ago and stands to its lintels (decay 0.19), Aieliaalas II
22,045 years ago and is foundations (0.92). The walls sink the deeper the older, and on a desert the sand has drifted
against every wall the wind meets (a wedge in the sand's colour). The plans, the roofs and the towers are the draws
they were (C-01's stream is kept), so a known settlement keeps its buildings. The age is not told anywhere: the
description line still says "the ruins of a people that is gone"; the shards' own years (`Shard::year`) count from
the founding and are a different clock (C-12's timeline).

*The explorer among them.* No rock is drawn or stands inside a room and no fallen log lies in one (KI-345's half that
could be done without flattening ground), and the capsule sets down outside a settlement's walls
(`settlementClearance`, `THE CAPSULE KEEPS OUTSIDE THE RUINS`). What stays by design is in KI-345 (the walls step at
their joints on a slope: there is no plinth and no ground is flattened, the terraces are walls on the slope) and KI-350.
The age also exposed two C-01 defects, fixed here (`docs/decisions/log/1-1.1.0.md`): the doorway's midpoint was mirrored on two of the
four walls (rubble could lie in a real doorway, a shard could sit in one), and a town wall could run through a house.

## The roads (C-09)

The ideas note (section 4) asked for roads that still go somewhere: "a dead civilisation's roads survive as faint lines in
the terrain (compacted ground, a different material), readable from the landing zoom. Following one on foot or by buggy
leads between the ruins of a world: a network to trace rather than points to find". C-09 (2026-10-03) builds them in
`peoples/roads.*` on C-01's settlements and the planet function, so the surface, the landing zoom and the harness read one
network.

*The network.* The nodes are the sited settlements of the ruin grid (`roadNodeOfCell`: C-01's cell draw without the
layout and its site test without the drainage, as the view sites them). Two settlements within two cells and six and a
half kilometres of each other are joined when no third settlement lies closer to both than they are to each other (the
relative neighbourhood graph, `roadJoined`): every settlement reaches its nearest neighbour, a town gathers three or four
roads, the graph is planar and the same from either end, and a pair is judged by the union of its two ends' blocks of
cells so the answer never depends on where it was asked from. On the desert test world's region 45 settlements have 66
roads (209 km together), on the felisian's land region 58 have 89; none is without a road, the degrees run one to four.

*The way.* A road runs from one settlement's edge to the other's (`roadWay`): the end of the street's axis that faces
the other settlement where the plan is a street or a grid, a town's gate at 1.04 radii, the edge toward it otherwise.
Between them it is a walk in fifty-metre steps that takes the gentlest of five headings each step (straight at the
goal, 22 and 45 degrees either side: the grade squared, the turn and the way off the goal weighed together, and the
straight step taken without looking further where the ground rises under six per cent), never stepping into the sea, a
lake or a dead desert's old sea, which stood when the roads were used (a pair whose every way is wet has no road); the
kinks are averaged out and a waver of a few metres over a few hundred keeps a road over a plain from being a ruled
line. A road is about a fifth of a millisecond to walk, so the view walks its block at a landing and the zoom its window
on the worker pool.

*The road on the ground.* The culture's roads (`Culture::roadHalf`, `paved`, `roadWear`) are five to eight metres
wide (a metre more between towns), paved with the rock family's material or beaten (the ground's own, the bare earth of
the sand family through grass and forest), and worn by the age: a noise along the way under the culture's wear leaves
gaps of forty to a hundred metres (`roadLeft`), a tenth of a young desert people's length and half of a
twenty-millennia people's on a wet world (Leileashphail III keeps 94 per cent of its roads, Aieliaalas II 51). The
site grows its network as the explorer moves (`SurfaceSite::ensureRoads`: the cells within five of the explorer's,
walked once each, again two cells on; a probe site builds none) and every terrain vertex within a hundred metres of
detail reads it (`sampleAt`): the road's share of the vertex's footprint is `TerrainVertex::road`; on the 4 and 16 m
rings a vertex on the bed takes the paving as its material, loses its vegetation and darkens a tenth (a beaten track a
fifth), so the two-material contour of B-311 draws the road's edge through the cells; on the 64 m ring the tone alone
darkens, a faint line from the air; the 512 m ring sees nothing. No tree, log or rock stands on the bed
(`roadCover`), and the buggy rolls on rock where the road is paved. Under the feet the readings say `OLD ROAD NE-SW` (R-414: `Tab`;
the HUD's own line before), the first step onto each road of a landing is a status line and a `ROAD` entry in the log, and on the
landing map the zoom draws the window's roads as faint lines in the ground's own colour (`Game::zoomRoads`: the villages' and the
towns' roads, since the whole web of lanes between hamlets was a honeycomb over the land), the one map of a world that shows something
not yet found: the roads are terrain, as the rivers are. R-415: with the peoples (`V`), off as the map opens.

*The peoples on the landing map* (R-415, 2026-10-11, `game/landing_map.cpp`; the user: "make them turned of by-default and make it a
separate button ... something like whole map autocompleted when you find all the shards for a civilization"). `V` shows the layer: the
explorer's marks of ruins and works (small points; the names of the three nearest the cursor beside it), the zoom's old roads and the
plan's cores (below), the places the records held were kept at (each record's home, `recordHomesOf`: of the places standing in its
year, by their size; read with the world's items, `droneWorldAsk`, the drone bay's reading, from the map's opening) and, of a people
whose records are all held, its whole plan: its land tinted to its frontier, every place (a town a point in the people's colour, the
first amber and the second cyan, a city paler and rimmed, the capital white and named) and every way, coloured by kind as the plan's
maps (`NET_KIND_COLOURS`). The homes spread over the land (the capital weighs ten, a city four, a town one and a half): on the drone
bay's world of two, 21 of the first people's 147 records showed 21 places and its capital among them, all of them its 454 places and
847 ways. The legend counts each people's records held of its count, its name once a record of theirs is held.

*The harness.* `vesperis_test roads <sx> <sz> <body> [latDeg lonDeg]` prints the culture's road style, the region's
network (the nodes, the pairs, each road's length against the straight line, its bends and steepest step, the share
left after the wear, the cost) and a sketch; `vesperis_test zoom <sx> <sz> <body> <latDeg> <lonDeg> [scale]` renders
the landing map's zoom over a point with the roads drawn (at 4x the roads are hairlines in the ground's own colour,
which is the game's default; at 1x, the flow's scale, a pixel each, so the test frame reads denser than the game); the
scenes `civilisation_road` and `civilisation_road_desert` stand on a road looking along it (the print gives the road
under the camera and twenty metres ahead against the ground as it is); the unit checks the network and the ground
(`08-testing-and-tools.md`); the flow's `roads_zoom` frame shows the zoom. What stays by design is in KI-352.

## The forms the renderer needs (A-02)

C-01's ruins were boxes and domes. The ages need far more, and A-02 is the renderer's half of it: the shapes, their broken
states, how they stand on the ground, how they block the explorer, how far off they are drawn and in what colours. Who builds
what is A-03's (the ages' buildings) and A-04's (the genome); where is T-01's and T-02's. Until A-03 the game's settlements are
C-01's and look exactly as they did (the nine civilisation scenes are pixel-identical); what A-02 adds is seen in the harness's
sheet, test city and skyline.

* **The forms** (`peoples/forms.*`, `Form` replaces C-01's `RuinElem`: its `shape` became `kind`, and C-01's boxes and
  hemispheres keep their own arithmetic, so they draw to the bit). Twelve kinds, most with variants:
  * the box (C-01's walls, columns, slabs and blocks) and the dome (C-01's hemisphere, a saucer, a pointed dome, an onion);
  * turned forms: a cylinder, a frustum or a cone, a spire whose flank bows in to a needle, a cooling tower's waist (a
    hyperboloid), a tank with domed ends, a chimney with its lip; round, or a prism of three to eight flat faces (a square
    or an octagonal tower), on an oval plan if asked;
  * the pyramid and the stepped mass: smooth, a ziggurat's tiers, a setback tower's upright tiers;
  * vaults (round, pointed, parabolic) springing from walls or from the ground, open or closed at their ends (a hall, a
    hangar);
  * arches: a gate, a free arch, an arcade of any number of openings (an aqueduct, a viaduct), round, pointed or flat-headed;
  * thin shells: a saddle (a hyperbolic paraboloid) and a dish on its mast, aimed;
  * trusses: a lattice mast or pylon, a girder;
  * cables hung between two points;
  * rings: lying on the ground or standing on their rim;
  * sweeps: a section along an arc or a line (a curved wall, an amphitheatre's tiers, a battered dam or embankment);
  * grown forms: a lumpy body and its lobes.
  * plates (C-17): a flat body of a drawn outline (a superellipse with a taper, a waist, a lean and a dome, or a polygon), an instrument's
    body; its outline one function the mesh and the case's picture share (`formPlateOutline`).
* **Broken.** Every kind has its broken state, from a share (`broken`, 0 whole to 1) and a seed for its jags: a wall's top
  ragged in steps; a dome caved in, its rim jagged and its inside showing; a tower snapped, open, holed under the break (and
  leaning by `tilt`, which also lays a form on its side); a pyramid's top tiers gone and its corners worn back; a stretch of
  a vault fallen; an arch's keystone gone and a pier of an arcade snapped (the arches it carried with it); a shell holed and
  sagging; a mast bent over above its buckle with members gone, a girder sagging; a cable snapped, both ends hanging; a
  ring with a sector gone; a sweep with gaps and a ragged top; a grown form cracked open. What falls (the rubble, the
  fallen top of a tower) is a building's, A-03's.
* **Levels of detail.** Each building is drawn at its own level by its distance: whole within 350 m (a round form 12-28
  faces), simpler to 1.3 km (8-14), coarse to 4 km (6), and beyond that as its silhouette where it stands a pixel high: a
  flat outline across the line of sight with the form's own profile up its height (a dome's curve, a spire's taper, a
  setback's steps), at least a pixel of the framebuffer wide (the mush averages it into its neighbours), a lattice's
  outline half as wide. That is the skyline: a city of towers stands on the horizon as far as the air lets it be seen.
  C-01 drew a whole settlement at one level by the distance to its edge; a city two kilometres across needs each building's
  own.
* **On the ground.** A small piece stands on the ground at its middle, sunk a metre (C-01's rule). A big one (over 6 m
  across) stands on the highest ground under its footprint and reaches down to the lowest, so an aqueduct's deck stays
  level while its piers go down into the valley, and a block on a slope stands on its plinth. Near, the ground is the one
  drawn this frame; farther, the coarse ring's surface that draws the ground there, read once while that ring does (the
  64 m samples without the rivers that C-01 used for its far towns floated a mast 22 m over a wadi's floor).
* **Colliders** (`formColliders`): discs with the span of heights they stand over (B-405), C-01's rows along a wall kept:
  slices up a tower, a cone and a pyramid by their profile (prisms and pyramids along their outline), a dome's rim where it
  springs from the ground (a roof leaves it to its walls), an arch's and an arcade's piers with the openings free, a vault's
  flanks and closed ends with the hall free, a shell's low corners, a mast's legs, a ring's tube (standing, its foot), a
  sweep's two edges and its ends. A broken form's missing parts have none (a ring's lost sector, a vault's fallen stretch,
  a sweep's gaps). The unit walks the explorer into every sample, whole and broken: it touches each and never enters one,
  and goes through a gate, an arcade, a hall open at its ends, between masts and a lattice's legs and under a saddle.
* **A people's colours** (`FormPalette`, in `Culture::palette`, A-04's genome's since): four ramps, the structure, the accent,
  metal and glass, and light, in the surface's free banks (the first people's 18, 19, 20 and 24, the second's 25-28,
  `FORM_BANKS`). A slot can be the ground's own material: C-01's walls are the rock (or the sand of adobe), and their bank's
  ramp is that bank's, so they look as they did. The light's ramp glows whatever the sun. The striated rock's grain stays
  on walls cut from the rock; painted and built materials are smooth.
* **Windows** (`WindowSpec`, four a people) are a pattern on a face by the face's own metres (along it, up it), like the
  grain: a bay, a storey, a share of each, a shape (a rectangle, an arched head, a slit, round, a band of glass along the
  storey), how dark, lit or not, and from what height. A tower of thirty storeys costs what a blank one does. Where a bay
  spans fewer than two pixels the pattern fades to its mean, so a far tower does not shimmer. They go on boxes, turned
  forms, a setback's upright tiers and a sweep's outer face.
* **What it costs.** The forms are gathered building by building, built, lit and fogged in chunks on the workers, and drawn
  in bands of rows that every thread walks in the same order, so the frame is the serial loop's (`VESPERIS_THREADS=1` gives
  the same hash). The test city (1,348 buildings, 1,555 forms) costs about 2.5 ms of a frame at 2x from its edge.

Where to see it (the harness, nothing in the game until A-03):

| Mode | What |
| --- | --- |
| `forms sheet [scale]` | 33 samples, each whole and broken near, its middle and far meshes up close, and at 600 m and 2.5 km (magnified twice): `shots/tests/forms_<scale>x_<n>.png` |
| `forms city [scale] [desert]` | the test city from 300 m outside its edge on Phiyiesh II (desert): `forms_city_desert_<scale>x.png` |
| `forms skyline [scale] [desert\|felisian\|airless] [km]` | the test city's skyline from a summit, at 30 km by default |
| `stability skyline [scale]` | the skyline while walking toward it: the desert at 15 km, an airless world at 30 km |

The air decides how far a city is seen. The forms take the terrain's fog (one formula, so a tower and the hill beside it
fade alike), and on the peoples' worlds that is 9-12 km: at 15 km the test city is a pale cluster on a desert's horizon, at
30 km a ghost, where the airless world's 45 km shows its towers clearly. KI-373.

## What each age builds (A-03)

A people's settlements are built as its age built (`docs/archive/ideas/IDEAS-civilisation-expansion.md`, section 2), from A-02's forms, and
weather as their materials do. `peoples/buildings.*` holds the generators, the materials' decay and the rooms; `peoples/ruins.*` lays
the buildings out in C-01's plans (T-02 laid towns by age since: below). Every civilised world's settlements change with it (the medieval
peoples' least: C-01's buildings are theirs, roofed); generation 13 (A-01's) holds, nothing else moved.

* **The kinds** (`BuildingKind`, a generator each, `buildingForms`), by the age that built them (`buildingAge`):
  * prehistoric: round huts of wattle on posts or of dry stone under a cone of thatch, with a hearth of four stones; longhouses of
    turf or planks under a ridge of thatch on two rows of posts; tent rings; stone circles (a lintel over a pair here and there, an
    altar stone, a bank round the big ones); standing stones alone or in rows, the tallest at the head; dolmens (a chamber of slabs
    under its capstone, the ring of its mound); palisades on their banks;
  * medieval: C-01's houses, halls, keeps (merlons, slits), rotundas, platforms, colonnades (an arcade for some), stelae, rubble and
    town walls, now roofed by the culture (a gable or a hipped roof of timber, or C-01's slab, or a stone barrel vault); the temple at a
    town's centre (a basilica with its tower and spire, a hall under a dome with its minarets, or a stepped mass with its shrine and
    a house at the stair's foot); round towers on the town wall; gatehouses (an arch between two towers);
  * industrial: terraced rows of brick with sash windows, a slate gable and stacks on the ridge; tenements; villas under hipped roofs;
    the church with its spire, the town hall with its clock tower, the station (a front block and its iron shed), the mill with its
    chimney, the gasometer, the water tower, the headframe with its engine house and spoil heap, the observatory;
  * modern: slabs of flats in bands of windows; suburban houses; towers of 100-500 m (a slab, setbacks, a cylinder or a prism, the
    people's form in three of five) over a lobby; the stadium; masts on their guys; the power station's cooling towers and stack;
    tanks and silos;
  * futuristic: the arcology (a stepped pyramid, a cone, or tapering towers grown together), the habitat dome of diamond glass on its
    ring, spires of a vertical town with their collars, the ring, the maglev line on its pylons, pods, a wreck fallen from orbit; a
    gate hall before each great structure;
  * the far future: one structure: a spike, a ring standing on its rim, a row of pillars, the mouth of a shaft, a solid (a cube, a
    tetrahedron, a sphere half sunk).

  C-08's features are built in the age's way where the age built them (a prehistoric people the cairns; an industrial one no
  cairns; a modern one its harbours and terraces in mass concrete; the futuristic and far-future ages none). The culture says how
  its age built (`Culture::roof`, `temple`, `hut`, `towerForm`, `great`: A-03's hashes, the genome's since A-04, below), and
  `Building::style` carries the choice into the building.
* **The plans by age** (`Layout`): C-01's plans hold the age's buildings, each place asking the age for a dwelling, a centre, a hall,
  a tower, a stela or a monument; the spacing and the radius scale with the age (0.85 a camp's, 1 a medieval town's, 1.85 an
  industrial town's, 2.4 a modern one's). A prehistoric people has no streets (camps, rings of huts round a stone circle or a
  longhouse, a "town" of two rings in a palisade); a medieval town's plaza holds its temple (C-01's platform, colonnade and stelae in
  one town of three); an industrial town has its station beside the town hall or the church and up to four mills and towers toward
  its edge; a modern town its towers in the middle, lower outward, and a stadium; a futuristic people lays out a few great structures
  (`SP_GREAT`: an arcology, two or three spires, a ring and a maglev line in a town of 300-450 m; a dome, a spire and pods in a
  village; pods in a hamlet); a lone monument is the age's (a stone circle, a dolmen, a row of stones; C-01's; an observatory, a
  gasometer, a water tower, a headframe; a mast, a power station, tanks, a tower; a ring, a wreck, a spire; the far future's one
  structure), drawn with the class's size so its reach is the radius the finders read without the layout.
* **How many** (`ruinOfCell`): the share of the 2 km cells holding a settlement follows the age (the lone monuments in the draw's first
  twentieth; B-417: on every people's world, the old ones' monoliths gone):

  | Age | Hamlets | Villages | Towns |
  | --- | --- | --- | --- |
  | Prehistoric (camps days apart) | 9% | 3% | 1% |
  | Medieval (C-01's) | 16% | 9% | 5% |
  | Industrial | 13% | 10% | 7% |
  | Modern | 10% | 10% | 10% |
  | Futuristic (a few great places) | 3% | 2% | 1.5% |
  | Far future | one structure in about seven hundred cells | | |

  A far-future world's "one to a few" structures and a modern people's megacities wait for the plan of a world (T-01).
* **The decay is the material's** (`materialDecay`: 0 whole, 1 gone, a log-logistic in the years; `buildingDecay` spreads it by the
  building's luck, C-01's quarter where the material is half gone, nothing where it is whole or gone):

  | Material | Half gone (a middling rain) | A tenth to nine tenths | The rain | At 100 / 1,000 / 10,000 / 100,000 years |
  | --- | --- | --- | --- | --- |
  | Timber, thatch, hide | 80 years | 1.2 decades | 0.8 decades faster wet | 0.56 / 1 / 1 / 1 |
  | Earth (mud brick, turf) | 400 | 2 | 1.6 | 0.26 / 0.66 / 1 / 1 |
  | Dry stone, megaliths | 20,000 | 2.5 | 0.4 | 0 / 0.08 / 0.40 / 0.72 |
  | Masonry, brick | 1,166 (C-08's `decayOf`) | 2.7 | 0.67 | 0.18 / 0.48 / 0.78 / 1 |
  | Iron, steel | 400 | 2 | 0.8 | 0.26 / 0.66 / 1 / 1 |
  | Reinforced concrete | 500 | 2 | 0.6 | 0.22 / 0.62 / 1 / 1 |
  | Mass concrete | 8,000 | 2 | 0.5 | 0 / 0.14 / 0.54 / 0.94 |
  | Glass | 5 | 1 | none | 1 / 1 / 1 / 1 |
  | Composite, ceramic, grown stone | 15,000 | 1.5 | 0.4 | 0 / 0 / 0.41 / 0.94 |
  | The far future's | never | | | 0 |

  A building reads each of its materials: a medieval house's walls stand for centuries while its timber roof is gone in one; an
  industrial row's brick stands roofless; a station's iron shed stands whole only while its glass would, then as its ribs, then
  nothing. Low walls stand to C-01's share; tall things (towers, chimneys, spires, masts, walls over 8 m) by a share that keeps them
  whole through their first decay and falls to a stump, their tops ragged; what fell of a tower lies across the ground away from the
  town's middle, broken, and a heap of rubble with tumbled blocks lies at its foot (the fallen top breaks up with its material; the
  heap stays). A roof, a dome, a lintel, a spire never outlast what holds them up. Posts rot to stumps and then to their holes (a
  dark ring of soil at the nearest level); dry stone leans, then lies where it fell; mounds and banks of earth last ten times a wall.
  The A-01 stopgap (the years over the age's durability) is the roads' and the sand's alone now; the buildings read the real years.
* **Rooms** (`buildingRoom`): every building with a room says where its floor is, its doorway (its midpoint and the wall's line) and
  the spots where a record can lie, the best first: a hut's and a rotunda's side opposite the doorway, a house's far corners, a
  hall's, a temple's and a church's nave, a tower's lobby, a station's front block, a mill's floor, an engine house, the gate hall
  before a great structure; the foot of a standing stone, a stela, a ring or a far-future structure in the open. C-03's shards lie
  there now (`shardSitesOf`, a spot 0.42 m clear of every collider of the building's pieces and 1.5 m from the doorway), and their
  place is the building's kind (`buildingPlaceName`: "A SHARD OF THE OLD PEOPLE TAKEN FROM A HUT", "... FROM THE FOOT OF A
  STANDING STONE"). T-04 lays the records' places by age in them.
* **Colours and windows** (A-03's `agePalette`; the genome's `culturePalette` since A-04, below): the prehistoric and medieval walls the ground's own rock (C-01's), timber and
  thatch, the roofs' tiles, slates, shingles or thatch (an onion of copper or gilt); brick or the dressed rock, slate and rusted iron;
  concrete, an accent, steel; composite, an accent, diamond glass; the far future's one material (black, white, bronze or blue-black)
  with its accent. The windows by the age: arched and slits, sash and tall arched and a mill's grid, bands and a curtain's empty frames
  and a grid, the futuristic age's thin bands and round lights; none before the medieval age nor in the far future. Earth (a mound, a
  bank, a spoil heap, a wreck's furrow: `Form::part` 8) draws in the ground's own soil: the grass on a living world, the sand on a
  desert.
* **The forest takes them back** (`forTrees`): no tree stands in a building; a settlement's yards keep a twentieth of their trees two
  centuries after the end, a third after ten thousand years (C-01's one in three), so a young ruin is a clearing. F-02 (2026-10-13): the
  building test (`inSettlementBuilding`, the logs' and the rocks' too) passes over a building whose circle (with the margin) the point
  lies beyond before it turns the point into the building's frame, and the yards' hash is drawn before the buildings: the same trees.
* **Every age's buildings are generators**: `cultureAs(g, people, age, years)` builds a people's culture as if it had reached an age and
  ended so long ago, `settlementOf` lays out a settlement of a class in it, and a building keeps the age whose way built it
  (`Building::age`). B-414: a world's buildings are all its people's last age's (T-02's old cores and the old time's megaliths went with
  their ages: "One age a world", below); a work's pieces keep their work's.

Where to see it:

| Mode | What |
| --- | --- |
| `ruins ages [scale] [town\|village\|hamlet\|monument] [desert\|felisian] [wide\|near] [years ...]` | a settlement of each age (rows) at 100, 1,000, 10,000 and 100,000 years since the end (columns), from outside its edge (`near`: among its first dwellings): `shots/tests/ruins_ages_<class>[_near]_<scale>x.png` |
| `ruins kinds [scale] [years ...]` | every kind of building alone, a page an age, at 1, 300, 3,000 and 30,000 years since the end: `shots/tests/ruins_kinds_<age>_<scale>x.png` |
| `scene civilisation_<age>` | a settlement of each age on a people that ended lately (`AGE_SCENE_WORLDS`, found by `peoples fresh`), from a vantage on dry open ground with the light from the side |
| `ruins check` | the unit's four checks of the ages alone |

## The architecture genome (A-04)

A people's buildings are its own (`docs/archive/ideas/IDEAS-civilisation-expansion.md`, section 3): every people draws thirty-five deep
variables (R-412: fifty-one, below) from its seed (`peoples/genome.*`: `genomeOf`, one hash a variable, so a variable added later moves none of the others) and
keeps them through its ages, and its culture reads them through the age it reached (`cultureWays`, peoples/buildings.h): a people that
loved domes builds them in dry stone, in masonry, in iron and glass, in concrete and in diamond glass, and a people of blue spires
raises one over its huts, its church and its tower block. Every civilised world's settlements change with it; generation 13 (A-01's)
holds. A variable is a number in [0, 1): a category its share of the range (`Genome::of`), a measure the number itself.

| Group | Variable (as printed) | Values | What it does | Ages |
| --- | --- | --- | --- | --- |
| Mass | MASSING | compact, sprawling, vertical, buried, stepped | the dwellings' footprints and heights (compact: smaller and taller, the town tighter; sprawling: wider, one storey, the town wider, the futuristic age's habitats spread; vertical: tower houses; buried: sunk 40% with their doorways kept; stepped: an upper storey set back on the walls, the towers' setbacks) | all but the far future |
| | PROPORTION | squat .. slender | every building's height (C-01's `tall`, 0.75 .. 1.3), the roofs' pitch, the standing stones, the domes' rise | all |
| | AMBITION | modest .. colossal | the centre's and the focal accent's size (0.75 .. 1.55, heights to 1.7), the lone monuments, the far future's structure | all |
| | HEIGHT | low .. lofty | how many towers and how tall (keeps, chimneys, spires, minarets, highrises, futuristic spires), the modern downtown's share of towers; C-01's `towers` over 0.2 | all but the far future |
| Form | PLAN | rectangle, circle, polygon, ellipse, free | the houses', huts', villas', blocks' and keeps' plan (a ring of the people's number of sides, an oval, an L), the towers' sections, the round temple, the arcology's sides, the far future's solid (a cube, a prism, an ellipsoid, a tetrahedron, a sphere) | all |
| | ROOF | flat, pitched, hipped, domed, conical, vaulted, saddle, stepped, open | every roof in its age's material (a corbelled beehive of dry stone, a turf mound, thatch, tiles, a masonry dome, an iron sheet, a concrete shell, diamond glass), the temple's choice, the towers' tops, the habitats' domes | all but the far future |
| | PROFILE | upright, battered, bulging, leaning out, stepped back | walls and rings battered on a thicker foot, bulging with rounded buttresses at the corners, an upper storey jettied out or set back (a plinth for a low wall), towers as frustums, barrels, corbelled tops or tiers, the futuristic spires' flanks | all but the far future |
| | SYMMETRY | bilateral, radial, rotational, none | doorways in the middle (bilateral, radial), the temple's and the church's towers (a pair at the front, one over the crossing, two at opposite corners, one at a corner), the minarets round a dome, the futuristic spires' placement | medieval to futuristic |
| | NUMBER | three .. nine | a polygon's sides, the stepped temple's and the setbacks' tiers, the platform's steps, the colonnade's columns, the stone circles' stones, the gasometer's columns, the cooling towers, the tanks, the grown arcology's towers, the spokes of a radial town, the far future's pillars and facets | all |
| | MODULES | one-offs, cells | a house and a row roofed cell by cell (a row of gables), crow steps on the gables, a mill's gables across it; a modern block's lift house a module (B-415) | medieval to modern |
| Surface | OPENINGS | square, arched, round, slit, triangular, keyhole | the windows' shape (`FacePattern` 0, 1, 3, 2 and A-04's 5 and 6; B-415: the industrial age's sash square or round-headed, the modern age's bands, portholes and slits on its temples, the futuristic age's portholes for arches and keyholes), the gates', arcades' and aqueducts' arches (round, pointed, flat; B-415: pointed in the hand-made ages alone), the domes' profile (B-415: an industrial people's hemispheres, a modern people's saucers) | medieval to futuristic |
| | RHYTHM | regular, paired, grouped, scattered | the windows' lights a bay (a pair, three or four split by mullions) or cells left blank by their hash (B-415: the industrial age's a pair at the most, never scattered) | medieval to futuristic |
| | GLAZING | few .. wide | the windows' share of the wall within its age's (B-415: a medieval window a third of a bay at the most, an industrial sash a third to a half, a modern pane half to the whole; bands in a modern people's frame) | medieval to futuristic |
| | ORNAMENT | bare, banded, carved, encrusted | bands of the accent at the walls' tops and storeys, a cornice, glyphs round every side, the door's surround, capitals | medieval to futuristic |
| | MOTIF | lines, circles, steps, spirals, glyph bands, figures, stars | on the great buildings: ribs up the walls, spheres on the corners, steps along the top, a ramp climbing round, glyph bands, statues at the front, the points of a star round the foot; the domes' onion profile (spirals, figures), the spires' collars | medieval to futuristic |
| | STRUCTURE | heavy masonry, expressed frame, smooth monolith | thick walls on the rock's striated grain (C-01's `style`), small windows; posts and beams on the walls (half timbering of daub between timber; B-415: an industrial people's brick piers between the windows, a modern people's concrete grid of columns and floor slabs; a futuristic exoskeleton); smooth walls, fewer windows | medieval to futuristic |
| | TOPS | finials, crenels, masts, statues, nothing | what tops the roofs, walls and towers (finials, spheres on a people of circles; merlons; poles; figures). B-415: crenels the hand-made ages' alone; an industrial people's everyday buildings topped by their chimneys, a modern people's by a mast or nothing; no statues after the modern age | medieval to futuristic |
| Colour | BASE | sixteen colours | the structure's colour: the ground's rock tinted toward it, or painted in it; the brick it fires (B-415: `BRICK`, one a colour, from a gault and a cream stock to reds, a blue, a dark engineering brick and a plum); its concrete, its composite, the far future's material at the colour's extreme | all |
| | ACCENT | seven colours on bands, roofs, doors or tops | slot 1's colour and the pieces that take it (bands always) | all |
| | FINISH | its own, painted | the structure the material's own (the rock tinted a quarter toward the base, concrete toward the base) or painted the base (B-415: an industrial people's brick, painted or not) | prehistoric, medieval, modern |
| | TINT | clear, bronze, green, blue, gold, smoke | metal and glass (slot 2) of the futuristic age: the frames, the domes of diamond glass (B-415: a modern ruin's glass is gone within years, its metal roofs went with its flat ones) | futuristic |
| | LIGHT | none, lamps, lines | a far-future structure's lights: glowing lines up it, lamps scattered on it (a dead city is dark) | far future |
| Place | LAND | ridges, valleys, terraced, cliffs, stilts, mounds, pits | a high place toward the ridge (a stepped platform and its shrine; B-415: an industrial people's bandstand, a modern people's plinth and mast), an aqueduct down from the high ground (a leat before the medieval age), terraces on any slope and a kerb round each dwelling, a cut in the rock round each, houses on stilts (the records under their floor; a modern people's blocks on pilotis), dwellings sunk in pits (B-416: the kerbs, the cuts and the pits the hand-made ages' alone, the mounds' earth banks and the pits' rims gone); T-01's plan sites the towns by it | prehistoric to modern |
| | FACING | sunrise, a star, the moon, the river, the wind, none | the settlement's axis (`orientedHeading`: east; a bearing of the people's own; the moon's rising; the ground's fall; the wind's; the cell's draw); the roads read it | all |
| | STREETS | grid, radial, concentric, organic, a line, contour, branching | the plan: C-01's grid; spokes from the centre (the people's number); rings; houses scattered round the centre; a street; a street across the fall; a street with side streets; T-02's street plans in each age's way (towns by age, below) | medieval to modern |
| | PUBLIC | plazas, courts, gardens, none | a plaza kept open round the centre; houses, villas and blocks round inward courts; walled gardens with their pools; none (denser) | medieval to modern |
| | GROUPING | compounds, rows, detached | houses in compounds of two or three round a court with low walls, in rows of three joined, or apart | prehistoric to modern |
| | EDGE | walls, ditches, nothing | the town's edge: walls with their towers and gates (a palisade before the medieval age), a bank and its ditch, nothing; towns and a village in three (T-02: an industrial town has none, its medieval core keeps its own) | prehistoric, medieval |
| | LINKS | arcades, bridges, tunnels, colonnades, none | arcaded fronts round the plaza and along the street, two towers joined by a bridge (keeps, brick towers, tower blocks, spires), tunnels' mouths, colonnades along the main street | medieval to futuristic |
| Meaning | FOCUS | spire, dome, gate, tower, colossus, mast, ring, obelisk, pool, great tree | the focal accent before the centre (`BK_FOCUS`, where the centre does not carry it), in its age's way (a menhir, a great mound, a trilithon, a broch, a carved stone, a pole; a lattice tower; a guyed mast); the temple's choice; the far future's structure (a spike, a halo, pillars, a shaft, a solid) | all |
| | CENTRE | temple, palace, assembly, market, tomb, machine | what stands at the centre: a stone circle, a great longhouse, a wood henge, an avenue of stones, a passage tomb, an alignment; a temple, a palace round its court with corner towers, an amphitheatre, a market hall, a stepped or domed tomb, a windmill; a church, a town hall, its domed assembly, an iron market hall, a mausoleum, a mill; a modern temple, the tallest tower, a stadium, a mall, a mausoleum, a power station; a spire, the arcology, a dome, a ring | all but the far future |
| | MONUMENTS | stelae, obelisks, colossi, arches, pillars | the monuments before the centre and on the plaza, and the lone monuments (standing stones, a trilithon before the medieval age) | prehistoric to modern |
| | THE DEAD | barrows, tombs, cemeteries, towers of silence, nothing | round C-12's stones: a barrow behind each, a small tomb, a low wall round the cemetery, a tower of silence (a ring of high wall), the stones alone | all but the far future |
| | HOUSES | alike .. varied | the dwellings' sizes' spread, another plan or roof now and then | all but the far future |
| | BUILT | light .. to last | its works' lifetimes over the age's materials' (`Culture::life`, half to twice), a light people's houses of timber, its huts of wattle (to last: dry stone) | prehistoric to modern |

* **The age's own** (`techniqueOf`): the technique a people used in an age, drawn among the age's by the deep variables' leanings:
  post and lintel (the prehistoric age's only), or arch and vault (domes, vaults, arched openings lean to it) in the medieval age; arch
  and vault or frame (iron) in the industrial; frame, cantilever (a modern people's towers stacked and slid off their core), tension
  (masts and cables) or shell (a tower's hyperbolic waist, a mall's shell) in the modern; shell, tension, printed (the stepped arcology)
  or grown (the grown arcology) in the futuristic. The materials, heights and spans are A-03's, scaled by the deep variables.
* **The palette** (`culturePalette`): slot 0 the structure, 1 the accent, 2 the roofing (thatch, tiles, slate, metal sheet) or the
  glass in its tint, 3 timber and iron (dark, rust) where the age has no light, the light's ramp in the futuristic and the far future's
  (`FormPalette::glow`); a slot of the ground's own is tinted toward its colour (`FormPalette::tint`). A piece's slot is what it is
  (`Gen::roofSlot`, `topSlot`, `doorSlot`, `timberSlot`): the accent goes where the people put it. A pool's water draws in the water's
  bank, a great tree in the bark's and the leaves' (`Form::part` 9, 10, 11).
* **The genome's works** (`BK_FOCUS` with its focus as its style, `BK_WINDMILL`, `BK_AQUEDUCT`, `BK_TWIN`, `BK_PORTAL`) and the kinds'
  new styles (a market hall and a palace of `BK_HALL`, a round temple, an amphitheatre of `BK_STADIUM` in stone, a town hall's dome or
  spire, an iron market hall, a mall, the monuments of `BK_STELA`), each with its room or the spots at its foot. Every building keeps
  what its room reads of the genome (`Building::form`, `sides`, `found`, `centred`), so a room is still known without the culture.
* **Found on the way**: C-01's rotundas and A-03's dry-stone huts, cisterns, observatories and stone circles laid their ring segments
  across the ring, as fins (and the stone circles' lintels radially); they lie along it now. A tower's or a roof's tops stand only on
  a roof that stands (a stone finial outlived its timber roof). A sunk house keeps its doorway 2.1 m over the ground outside.
* **The uniqueness is measured** (`genomeDistance`): a weighted distance on the strongest variables (the base colour 2, the roof 1.5,
  the plan and the massing 1.2, the accent, the proportion and the focus 1, the profile 0.8, the height, the openings and the structure
  0.6, the centre, the tops and the streets 0.5, the ornament 0.4: 13.4 at most; colours by a distance the eye weighs, measures a third
  of their range apart counting as different). Two peoples are alike under a quarter of it (`GENOME_ALIKE`, 3.4): sharing all the
  strongest variables but the weaker few. Of the hundred peoples nearest home (4,950 pairs) the nearest pair is 4.93 apart; of 200,000
  random pairs none is under 3.5 and five in a million under 4.0.

Where to see it:

| Mode | What |
| --- | --- |
| `genome sheet [scale] [years] [town\|village\|hamlet] [desert\|felisian]` | the architecture sheet, a page an age (`shots/tests/genome_<age>_<scale>x.png`): the twelve peoples nearest home as if each had reached the page's age, a town at the same cell under the same light 30 years after the end (unless given), its genome printed under it |
| `genome ages [scale] [peoples] [years]` | the same peoples (rows, six unless given) in every age (columns): does a people read as itself? (`genome_ages_<scale>x.png`) |
| `genome distinct [n]` | the distinctness of the n peoples nearest home (100): the pairs by distance, the nearest eight and what they share, the random pairs' tail, and the command for the nearest pair |
| `genome pair <sx> <sy> <sz> <body> <people>` (twice) | two peoples in every age, side by side (`genome_pair_<scale>x.png`, `scale=N`) |
| `genome print <sx> <sz> <body> [people]` | a people's genome and its techniques by age |
| `genome reach` | every variable's values set in turn in three peoples' genomes, in every age it acts in: which reach the drawing |
| `genome street [scale] [peoples] [years] [desert\|felisian] [drone]` | B-415: the same peoples (rows, six unless given) in every age but the far future's, each town seen from inside it (a street half way in, at a walker's height; `drone` 35 m up at its edge): does every age read as itself? (`genome_street[_drone]_<scale>x.png`) |
| `ruins kinds [scale]` | its last page (`ruins_kinds_genome_<scale>x.png`): the focal accents, the monuments, the windmill, the market halls, the palace, the round temple, the amphitheatre, the aqueduct, the joined towers, the tunnel, the town halls' dome and spire, the mall, a modern temple |

## A people's map of its world (T-01)

Until T-01 every 2 km cell of a civilised world was hashed on its own: a third of them held a settlement, the whole world over. A
people's world has a plan now (`peoples/plan.*`, `docs/archive/ideas/IDEAS-civilisation-expansion.md` section 5), built once a world on a
thread of its own while the ship comes in (`worldPlanPrefetch` at the approach, the parking, the landing map and a load; `worldPlanOf`
waits for it or builds it; four worlds kept). Generation 13 (A-01's) holds.

* **The reading** (`PlanGrid`): the planet function on a grid of half-degree rows, each row's cells as wide as it is tall (165,018
  cells, 33-65 km across on the peoples' worlds), at half a cell's detail without the rivers' tiles: the land of the people's time (no
  sea, lake or old sea), the height, the roughness, the vegetation, the climate, the fertility by the biome, the coast's distance,
  coal and ore (a field of the world's own at the mountains' feet and in them), the side of C-13's divide, and the rivers of a flow
  over the grid (every pit filled from the sea, each cell gathering the rain upstream: a dead desert's rivers ran into its seas).
* **The score** (`scoreOf`, by the age): water and good land always; the hills for defence in the medieval age; the rivers, the coal
  and the ore in the industrial; the coasts in the modern; a choice of the people's own in the futuristic and the far future's (a
  mountain to ring or a plain to make flat); the genome's relation to the land on top (KI-375's: a people of ridges on the ridges, of
  valleys by the rivers, of cliffs in the rough, of stilts by the water, of terraces on the hills); the climate (a world too hot or
  cold everywhere was the people's own) and the height under it.
* **The territory**: grown from the capital (the side's best ground, with a little of the people's own choice) over the land it could
  reach, the dearer the poorer, the rougher or across the water (the age's boats: a prehistoric people's coast is its end), until it
  holds its profile's share of the world's land (`PeopleExtent::held`, A-01's draw, now readable without the system); two peoples grow
  on their own sides of the divide and meet only where both reach it.
* **The places**, by rank (`PlanPlace`): the capital (`SC_CAPITAL`), the cities (`SC_CITY`) on the age's best ground (its top half; a
  modern people's top third: its coasts) and the towns (`SC_TOWN`) on any but the poorest, spaced as the age travelled (cities 260 km
  apart in the medieval age, 170 in the industrial, 140 in the modern; towns 70, 45, 45), their counts by the territory's size and
  capped (40, 80, 120 cities; 400, 700, 900 towns: a great territory's stand farther apart); a prehistoric people's gathering place
  and its bands' seats (`SC_VILLAGE`, 90 km apart, 40 at most); a futuristic people's capital, its great places (`SC_TOWN` in
  `SP_GREAT`, 500 km apart, 24) and the lesser (`SC_VILLAGE`, 250 km, 60); the far future's one to three works (`SC_MONUMENT`, 1,500 km
  apart). Each on the fine ground of its cell: dry where it stands and round its core (C-01's site test at 64 m and, for a capital or
  a city, two rings more), the gentlest of a lattice of spots, by the water where the age wanted it. Each has its rank in the founding
  (the capital first, then the near and the good before the far and the poor, a city before a town: H-02's order), its population at
  the height (the age's share of the people in its places, 5% to all of them, the capital the most) and the core the ground draws:
  a capital 110, 470, 820 and 1,150 m across its radius in the four ages to the modern (480 m a futuristic one), a city 270, 500 and
  700 m, holding C-01's town's density at the age's spacing (1,100 to 1,400 buildings in a capital, 350 to 550 in a city); a town,
  a seat, a great place and a work their layout's own draw.
* **The countryside** stays the cells' own (`planCountryside`): C-01's hamlets, villages and market towns, a lone monument in the draw's
  first twentieth (a desert's; B-417: every people's world's), by a density the plan gives each grid cell (the territory's ground, thinning toward the frontier, half a
  cell out beyond it) and the nearness of the people's places; at full density a medieval people's cells hold 16% hamlets, 10%
  villages and 2% towns (C-01's third, a little thinner: its towns are the plan's), the prehistoric 5% and 3%, the industrial 12, 10
  and 4, the modern 8, 8 and 4; a futuristic people's pods round its great places alone; the far future none. No settlement in the
  land no people held (round a medieval capital about one cell in six is settled), none within a core and 250 m round it but the city.
* **The networks** (`PlanEdge`): a greedy spanner over each place's eight nearest (the cheapest first, a link kept where the ways laid
  go round by more than the age's detour: 3.0 a prehistoric people's paths, 1.7, 1.6 and 1.5 to the modern, 2.2 the futuristic),
  dearer over water so a crossing is laid only where the land does not join; the components left apart joined by their cheapest pair;
  the trunk the network's routes between the capital and the cities along the spanning tree of their costs. The kinds: paths; roads
  (the trunk paved and broader); an industrial people's rail on its trunk and canals between two river places in flat country (two in
  five); a modern people's highways on its trunk; a futuristic people's maglev; none in the far future; a way over water a crossing
  (three or more of the grid's water in a row, or over half its length), not drawn. Every place reaches the capital.
* **The ways on the ground** (`planEdgeWay`, `roadLegWalk`): waypoints every 3 km on dry ground (the line's point or the nearest dry one
  across it, within 2.4 km), made when first asked and kept; the legs between them walked as C-09 walked its roads, by the kind's
  weights (a path wanders and climbs, rail and a highway keep their grades low and turn wide, a canal keeps level), drawn into the
  terrain by their kind (C-09's paving or beaten earth, a railway's dark ballast, a canal's silted bed, a highway's pale slabs, a path's
  worn line; `SiteRoad::kind`, `netHalfWidth`), worn as C-09's are. The HUD names the kind underfoot (`OLD RAILWAY NE-SW`); a maglev
  runs on its pylons, A-03's spans of about 150 m in every cell it crosses (with its settlement, or a cell of works alone, `SC_WORKS`:
  no shards, no graves).
* **The cells read the plan** (`ruinOfCell`): a place's own 2 km cell holds it (its layout from C-01's cell seed: a capital or a city
  `SC_CAPITAL`/`SC_CITY` in the town's plans at the plan's radius and count, a people of the line's on a grid, the gathering place in
  rings; an industrial or modern city's middle built up, its tenements, blocks and towers by the people's appetite for height; the wall
  in stretches of a town's); the rest the countryside's. The view reaches a core from every cell it spans and draws the plan's places to
  24 km (`SurfaceView::planNear`, `PLAN_NEAR_M`): a far city's towers on the skyline; the colliders, the rocks, the flora and the shards
  read the core wherever the explorer stands in it. A landing inside a core is moved out past its edge.
* **What is seen from orbit is what is walked on.** The landing zoom draws the plan's ways by their kind and the places' cores as
  discs of built ground (a core's share of a texel, less what the years took); the telescope's plate draws the same over its cells
  (the cores' built ground; the ways where a cell is under 300 m); the whole-world map shows nothing at its scale (a core is a
  thousandth of a texel). The scanner hears the plan's places by their size (a capital from 400 km, a city from 200, a town of the
  plan from 80, a band's seat from 40, a futuristic great place from 300, a far-future work from 1,500: `A CITY`, `A GREAT CITY`).
* **Found on the way.** C-09's blocks of cells stepped by the same longitude index in every row, but rows have their own steps: at 50
  degrees and an index of 5,000 the next row's cell of that index lies two and a half cells off, so a hamlet's neighbour 1.8 km away
  was outside its block (`ruinCellLonIn` now: the roads' blocks, `roadsNear`, the site's road growth, the landing's clearance).
* **What moved.** Every civilised world's settlements (where they stand, how many, the cities), its roads (the plan's ways, and C-09's
  lanes in the countryside only), its shards' and graves' places, the civilisation scenes (from a town of the plan; the ages' from the
  capital). C-01's checks read the countryside round a town of the plan; C-13's frontier is asked for only where the two territories
  meet (Eleinewai I's do not: a prehistoric people's and a medieval people's lands far apart; R-413: two prehistoric peoples' now).

Where to see it:

| Mode | What |
| --- | --- |
| `plan <sx> <sz> <body> [second]` | the world's plan over its map for each age (the people as if it had reached it, over one reading of the world: `shots/tests/plan_<world>_<age>.png`, the six on `plan_<world>_ages.png`), the territory tinted and outlined, the capital white, the cities red, the towns yellow, the ways by kind (paths brown, roads pale, rail red, canals blue, highways orange, maglev magenta, crossings dotted); its places, ways, held share, people and the capital's site printed, each plan's time; then the plan the game builds on one thread |
| `plan ages` | the six age worlds' plans at their own ages |
| `plan view <sx> <sz> <body> [second]` | the people's capital through the telescope from above it under a high sun at x64, x256 and x1024 (`plan_view_<world>_x<power>.png`, `_map.png` the map alone) |
| `zoom <sx> <sz> <body> <latDeg> <lonDeg> [scale]` | the landing zoom: the cores and the ways with C-09's lanes |
| `scene civilisation_<age>` | the capital of the age's scene world |

## Towns by age (T-02)

A settlement used to be C-01's plans (a cluster, a street, rings, a grid of 17 by 15 m blocks) scaled by the age, its streets the gaps
between its houses. A village, a town and a city are laid out now as their ages laid them (`peoples/towns.*` and `layoutTown` in
`peoples/ruins.cpp`, `docs/archive/ideas/IDEAS-civilisation-expansion.md` section 6): streets first, then the buildings on them, by district, in
layers by the history's eras. Every civilised world's villages, towns and cities change (and with them the shards' and graves' places);
generation 13 holds. A camp and a hamlet keep C-01's and A-03's plans; a futuristic people keeps its few great places in the land left wild;
the far future has no towns.

* **The streets** (`Street`: a segment, its half width, its rank: a lane, a street, a main street, an avenue, a ring road; the age that
  laid it; paved or beaten), in the people's pattern (`GV_STREETS`), each age in its own way:

  | Pattern | Medieval (crooked, narrow) | Industrial (straight) | Modern (blocks of about 100 m) |
  | --- | --- | --- | --- |
  | grid | a grid of uneven lines, some broken off | a grid with a back lane down each block | a downtown grid |
  | radial | spokes and rings, more spokes outward | spokes and ring boulevards | spokes and rings |
  | concentric | rings close together, joined from ring to ring | the same, straight | the same |
  | organic | wandering main ways from the square, lanes off them | quarters, each its grid at its own angle | a grid at an angle |
  | a line, the contour | one main street with lanes behind (a city of the line: streets side by side, crossed rarely; on the contour, stairs between them) | long terraced streets | the same |
  | branching | a main street with side streets ending short | the same, the side streets' own branches | the same |

  A modern town's ring road (a city's on its older rings' edge) and beyond it the suburbs' collector streets and cul-de-sacs, each with its
  turning circle. The main ways out run from the centre's square along the axis and toward the plan's places the ways go to. A village is a
  street or two, a green, or lanes. A medieval layer's lesser streets end inside its wall, a street runs round inside it, the main streets
  go out by its gates. The streets end before the water (and the harbour's room); what a clip cut off from the rest is dropped (the web is
  one). The blocks widen where a town's buildings could not line its streets (a city keeps its class's count: the frame's budget).
* **The layers** (`TownLayer`): a place of the plan reads its history (`historyOfWorld`: the world's system found again from its star's
  sector). B-414: one layer, its people's last age, whatever age founded it (T-02 drew the ages from its founding to its end, a ring each:
  a modern capital's medieval core to 470 m, its industrial ring to 820, its modern to 1,150); a place the history left before that age
  began is not on the ground (`TownHist::gone`), one left early within it is built weathered from when it was left. The countryside's
  villages, towns and lone monuments are their people's age (T-02 gave a later people's an older core by the history's shares, and its
  lone monuments and its capital's fields the megaliths of its old time: gone with that age).
* **The centre and the districts** (`District`, on each building): the centre's square with its centre in the oldest layer's way (B-414: the
  town's one), its monuments and its focal accent; the old core (none since B-414); the houses, built up densest near the centre and
  along the main streets (a medieval town's main streets a row of joined houses whatever the people's grouping); the works downwind (an
  industrial ring's mills, warehouses, water towers, gasworks and workers' rows; a modern one's tanks, works halls and masts); the harbour
  toward the water; a modern city's towers round its old core and its suburbs beyond its ring road, thinning into the fields; the station on
  a main street of an industrial ring; a medieval town's market hall by the square; a modern city's stadium and power station beyond its
  edge. The houses' grouping is the people's (rows joined along the street, compounds round a court, detached), their heights falling from
  a city's middle.
* **The edge**: a medieval layer's wall with its towers and its gates where the main streets go through (a bank and its ditch for a people
  of ditches); an industrial town has none (its old core keeps its own); a modern city its ring road and suburbs; nothing.
* **C-08's reading of the site by age**: the harbour a quay along the shore and its mole (medieval), a dock between two piers with its jib
  cranes and its warehouse (industrial: `BK_CRANE` style 0), a container port with its gantries and stacks of containers (modern: style 1,
  `BK_CONTAINERS`), a city's berths side by side along its shore; terraces on a slope; a desert's cistern and wind wall (medieval), a
  reservoir upslope (industrial: a basin of masonry in its bank; modern: a round tank of concrete; `BK_RESERVOIR`), on a living world a
  town's on a slope; the fields round it (`FieldPatch`): a camp's plots, a medieval town's strips in furlongs (walled fields on a desert),
  the industrial age's enclosures, a modern people's great fields and, on flat dry ground, its irrigation circles, terraced fields below a
  town on a slope.
* **In the ground**: a town's streets, squares and fields are drawn into the terrain (`SurfaceSite::addTownMarks`, `applyMarks`) on the 4,
  16 and 64 m rings as the town comes within two cells: the paving (setts, a modern street's asphalt, a lane's beaten earth) and on the air
  the streets' tone, a field's strips or tone and its edge (a hedge on a living world, a wall or a bank on a dry one), an irrigation
  circle's darker soil, faded by the years; the trees and the rocks keep off the streets.
* **Kept off the streets**: every building is tested against every street's corridor as its rectangle and, where its pieces reach past it,
  by its own colliders (set back by as much once, else not built); the unit checks the streets walkable along every line.

Where to see it:

| Mode | What |
| --- | --- |
| `towns <sx> <sz> <body> [second]` | a people's capital, its first city, its oldest town, a place it left early, a town and a village of its countryside: each printed (its layers, its buildings by district and age, its streets by rank and how joined, its fields, its works, the time to lay it out) and drawn from above (`shots/tests/towns_<world>_<what>.png`: the streets by rank, the squares, the layers' rings, the buildings by district, the wall red, the fields) |
| `towns ages` | the six age worlds' |
| `towns view [<sx> <sz> <body> [second]] [scale=N]` | the ages' scene worlds' capitals through the renderer: from the air over the edge, low over the old core, along a street of the middle (`towns_view_<world>_<air\|core\|street>.png`); then the site's reading by age, a harbour, a reservoir or cistern and the fields of each age from the air (`towns_feature_<age>_<what>.png`) |
| `towns check` | the unit's T-02 checks alone |
| `scene civilisation_<age>` | the capital of the age's scene world, from its edge |

## The works (T-03)

Everything else a people made outside its towns (`peoples/works.*`, the expansion note's section 6: "it can be virtually anything"), each
placed by a rule of the terrain or where the history (H-02) says it was made, and built in the way of the age that made it (`Work::age`,
weathered from when it was left, `before`). A world's works are made once (`worldWorksOf`, the last four kept) on the approach's plan thread
after the peoples' histories, in 44 to 350 ms; the surface takes them when they are built (a render never waits). Twenty-four kinds (B-418:
the fort; D-05: a dome over warm ground, glasshouses); generation 13 holds (the works are a layer over the ruin cells: no settlement, shard, grave or road moved). B-418 made a people's
everyday works its land's (the land's works, below): the rules marked "the land's" place one in a cell of 8 km where its ground offers it.

| Work | Where its rule puts it | Its age's way |
| --- | --- | --- |
| quarry | the land's: where a cell's rock shows, a face where the hill stands steep, a pit in flat rock (a stone-built people's); a medieval or later people's pit on any dry ground, the forest grown over it since (clay, marl, gravel, sand: an adobe people's only these); a prehistoric people's flint pits on any ground but snow | flint pits and their spoil rings (prehistoric); a face or a stone pit of three-metre steps, cut blocks, a timber derrick, a hut (medieval); benches, an iron derrick, a shed and a spoil tip (industrial); great benches or an open pit, its crusher's hall, tailings and trucks (modern) |
| mine | the land's where the plan read coal and ore; the history's by the places it sank one | adits along a hillside, their spoil tipped below (medieval); headframes with their engine houses and stacks, a great tip, the miners' houses (industrial); an open pit in benches, its headframe, mill, tailings and trucks (modern) |
| dam | where the history built one (a river town's): the narrowest cross-section of a valley near the town or up its river, its crest at the lower shoulder (the age's height at most), never in a basin the drainage fills; the reservoir behind it as high as it holds | a masonry gravity dam and its buttresses (industrial); a concrete gravity dam with its spillway, intake tower and power house, or an arch bowed into the reservoir (modern); a wall of composite, its light along the crest (futuristic). Breached when its material is half gone, the reservoir drained to its silt; a dead desert's an old pale bed |
| aqueduct | where the history built one: from a spring on higher ground 3 to 13 km off to its town's edge, the channel falling all the way, on arches where the ground falls under it | arcades of one to three tiers standing to the channel's line, the spring's house, the castellum; its conduit's line across the ground |
| observatory | the highest summit within 25 km of the town that built one | a platform, a tower and its ring of horizon stones (medieval); domes (industrial); dishes and a dome (modern); a ring of dishes round a dome (futuristic) |
| shrine, the pilgrims' way | the people's holy place (`PeopleProfile::holy`): a summit, a spring's hollow, a cliff (a cave's mouth), a grove (in its clearing), the open sky; a beaten track from its town with a cairn or a stela every 600 m. The land's: the lesser shrines where a cell's ground is its holy place's kind (no way to them) | stones and a cairn, a temple, a chapel, a monument |
| necropolis | the land's: the slope that looks at the greatest settlement in or round its cell (a village or more; a prehistoric people's any) across the low ground (its river's where it has one); the genome's way with its dead | barrows and a long barrow (a people of barrows, the old time), tombs cut into the slope over houses of the dead and rows of graves (a people of tombs), a walled garden of graves with its tombs, obelisks and chapel (industrial), a grid of graves and a columbarium (modern), a field of memorial pillars (futuristic), towers of silence |
| lines on a plain | the flattest dry plains of a prehistoric or medieval people's land | a creature drawn in cleared bands ten metres wide, rays from a cairn, cleared trapezoids: read from the air (the drone) |
| henge, standing stones | the land's: open ground on a slight rise wherever a prehistoric people lived | a circle in its ditch and bank with its avenue, rows of standing stones, a great circle with its trilithons, a field of dolmens and barrows |
| fortified line | along the frontier with the other people, or the border nearest the capital facing land not held; in stretches of 2.5 km | a wall with its towers and forts (medieval); a ditch, a rampart and redoubts (industrial); an anti-tank ditch, dragon's teeth, pillboxes and a blockhouse (modern) |
| battlefield | each war's greatest battle (two of a great war): between the battle's place and the place the other side came from, on the line between them, on open ground | cairns (prehistoric); barrows and a memorial stone (medieval); trenches facing each other, shell craters, an obelisk and war graves (industrial); craters, wrecked tanks, an aircraft, pillboxes, war graves (modern); glassed craters and fallen machines (futuristic); its dead's graves all of the battle's year |
| launch site | where the history built one: the flattest dry ground of the land nearest the equator, the sea to the east | pads with their towers and aprons, the crawlerways, the assembly hall, tanks, a blockhouse (modern); a mass driver rising east (futuristic); an elevator's anchor and its ribbon to the sky |
| antenna field | a radio people's high plain away from its towns | a row of tall masts (industrial); a field of dishes or of masts (modern); a ring of dishes (futuristic) |
| wreck | the land's: a ship on a cell's shore; a modern people's aircraft on open flat ground; a futuristic people's wreckage fallen from orbit in its crater | a hull's ribs and keel, an iron steamer listing, a freighter broken in two; an aircraft; a hull in its furrow |
| colossus | a people of colossi (the genome's focus or monuments): its capital's harbour mouth, else the height over it | its figure on its plinth, an arm raised; fallen along the ground, its head off |
| terraformed basin | a low basin of a futuristic people's land | the ground levelled in its rim, channels across it, pumping stations |
| the last shelter | the steep rock nearest the place the collapse ended in (H-02's last place); D-05: a people that lost its sun's by the warmest ground, and a shelter by each of the two places it left last in the dark | a rock shelter's mouth (prehistoric), a citadel on the height (medieval), a casemate (industrial), a bunker's door with its vents, masts and guardhouse (modern), a vault under its dome (futuristic) |
| something of the far future | anywhere on its land, two to four | a plain made flat and glassy, pillars to the horizon, a mountain cut into terraces, a lattice over a valley, a sphere on its point |
| wind farm | the land's: a modern people's cell's highest ridge | rows of turbines into the wind |
| lighthouse | where the history lit one: the headland nearest its harbour; the land's on a cell's headland (the water round a third of it at 400 m) by a settlement | its tower and lantern, the keeper's house |
| fort | the land's (B-418): the cell's height, a crest it can stand on (a prehistoric people's on a hill; a later people's anywhere, likelier on a hill), its gate toward the settlement it guarded | a hill fort's one to three rings of bank in their ditches round the huts of its last people (prehistoric); a castle of five to eight sides, a tower at each corner, the gatehouse, the keep, behind a moat on the flat (medieval); a redoubt, its ring bank round a casemate in its ditch (industrial); pillboxes round a blockhouse, dragon's teeth across the way up (modern); its towers and walls in the people's genome |
| dome over warm ground | D-05, a people that lost its sun's: the warmest open ground near its last place and near each of the two left last (a field of embers where the world has them, else the lowest dry ground: the deep) | a vaulted hall over its spring (medieval); iron ribs and their glass on a ring of brick (industrial); steel and glass on a ring of concrete (modern); a habitat (futuristic); the far future's whole. The glass went at once, the ribs stand |
| glasshouses | D-05: beside each dome, rows of three to six on flat dry ground, their long sides toward it (an industrial people's and later) | iron hoops on low walls of brick (industrial); tunnels of steel hoops (modern); composite vaults (futuristic, far future) |

* **The pieces** are buildings of the works' kinds (`BK_MOUND`, `BK_DAM`, `BK_DISH`, `BK_TOMB`, `BK_FORT`, `BK_BUNKER`, `BK_TEETH`,
  `BK_VEHICLE`, `BK_AIRCRAFT`, `BK_SHIP`, `BK_LAUNCHPAD`, `BK_HANGAR`, `BK_COLOSSUS`, `BK_TURBINE`, `BK_LIGHTHOUSE`, `BK_LATTICE`, `BK_DERRICK`,
  `BK_BLOCKS`, `BK_SPHERE`, `BK_TETHER`, `BK_TRILITHON`, `BK_CONVEYOR`; D-05's `BK_DOME`, `BK_GREENHOUSE`) and of the ages' (a headframe, a
  mast, a stone circle, a portal),
  laid out by `workSpec` as a settlement's (`RuinSpec::work`, `sclass` `SC_WORKS`) and drawn, decayed and walked into as one. A dam's wall,
  an aqueduct's arches and the far future's lattice stand at heights over the reference level (`Building::base`, `Form::absolute`): they
  span their valley whatever the ground under each piece does. A necropolis's and a battlefield's graves are C-12's stones in their plots,
  named, found and logged like a town's.
* **The ground** (`GroundEdit`, drawn by `SurfaceSite::addWorkEdits` and `applyEdits` as the work comes within 6 km): a face's floor and
  its benches, a pit's, a ditch silting up with its bank, a crater filling (scorched, or glassed), a plain made flat (paved, glassy, or a
  basin levelled), the reservoir's water at its level over the cells it filled (or its old bed), lines (cleared and paler, beaten and darker,
  paved), a mountain's terraces; the trees and the rocks keep off a fresh cut, a line and a plain made flat; the landing capsule is set
  down outside a compact work.
* **Heard and marked**: the scanner's WORKS mode, after RUINS (`worksHeardNear`): the nearest work's kind (`A DAM`, `A BATTLEFIELD`,
  `LINES ON THE GROUND`, `SOMETHING MADE`); a line's lesser stretches and a pilgrims' way are not heard. A mark's kind is `LM_WORK` (`%` on the
  maps); the guide's file keeps R-408's numbers (8 a place, 9 a work).

Where to see it:

| Mode | What |
| --- | --- |
| `works <sx> <sz> <body>` | a world's works: each kind's count by people, what its history finished, and every work's line (its age, its place, its rule's numbers: a dam's height, length and reservoir; a quarry's face or pit; a battlefield's war and year); B-418: the land's round each people's first ten places and at forty points of its land, and what the scanner hears from those points |
| `works ages` | the six age worlds' and the six scene worlds' |
| `works gallery [scale=N]` | each kind of work of each age's scene world through the renderer, where its rule put it (`shots/tests/works_<age>_<kind>.png`, a sheet `works_<age>_sheet.png`) |
| `works map` | the same works on insets of the ground with the place each serves (`works_map_<age>.png`: the terrain, the cores, the ways, its reach, a battlefield's line between its sides, the rule in a line) |
| `works view <sx> <sz> <body> <kind's word> [index]` | one work through the renderer (`WORK_D`, `WORK_H`, `WORK_A` set the camera) and its pieces printed; the world's then the land's, a kind's whole name matched whole (`"a fort"`) |
| `works check` | the unit's T-03 checks alone (B-418: and the land's, and the scanner's from the peoples' land) |

## Where the records lie (T-04)

A people left between 80 and 200 records (`recordCountOf`, `peoples/records.*`, the expansion note's section 7), by its age and by how many
they were at their height: the literate and the many left more (the age worlds: the prehistoric Taarkhaur 86, the medieval Othleas 101,
the industrial Xoocu 135, the modern Veathiash 143, the futuristic Balaxiorn 175, the far-future Xoguskax 178). The count is fixed here,
before H-04 writes the texts: C-02's fifty are the first fifty and keep their texts, their music and their charts; the rest are drawn the
same way, music and star charts at the people's own shares of the fifty (H-05 sorts the three categories by the age). `Lore::records`
carries it: the decoder's lists, the taking and the radar's world read it (`shardsOfWorld` both peoples').

* **Dated in the history** (`recordsOf`, a catalogue a people, the last eight kept). Each record is dated in the people's own count
  (`recordYear`). B-414: in the years of the age the people reached (`ageFromOf` to the end), weighed by what the years left (e every 1,500
  years before the end); T-04 spread them over the eras by what each age wrote a year, so a modern people's records were a fifth medieval
  books and scrolls, a fifth industrial print and wax. Its earlier ages are told by its own histories (`genre.history`, "One age a world",
  below). C-02's draw of a shard's share of the years is the record's share, so the order is C-02's and the last recording is the end's year. The shards' years
  (`Shard::year`, the `{year}` in a text, the decoder's dates and timeline, the graves' years: the lore's span is the end's year less one)
  are the history's now: A-01 had kept C-02's span of 150 to 600 years until H-04, but the medium follows the date.
* **Made in its year's way** (`RecordMedium`, `mediaOf`; B-414: the age reached's): carved bone (a tally, a flute), carved stone and a painted wall (prehistoric); clay
  tablets early, scrolls, codices late, carved stones, an engraved plate for a chart (medieval); printed pages, codices, wax cylinders (a
  voice, a piece), a glass plate of the sky (industrial); printed pages and reels early, discs, drives late, a glass plate (modern); drives
  and crystals (futuristic); crystals and something that hums (the far future).
* **Each its home** (`recordHomesOf`): a place of the people's plan standing in the record's year, drawn by its size (the capital and a far
  future structure ten, a city four, a town one and a half, a village one): where it was kept. The home always holds it; copies lie
  anywhere else (C-03's rule kept: a copy taken is gone everywhere), so every record lies somewhere on the world: the medieval Othleas'
  101 at 1,656 sites of their plan's places, works and the countryside round their capital (the commonest at 27), the far future's 178
  at its two structures.
* **Where by age** (`recordSitesOf`): a settlement holds C-03's few by its class (one a building, in the buildings an age kept its writings
  in: a chief's longhouse, a stone chamber, a temple, a hall, a church, a town hall, a station, offices, a station's hut at a mast's foot,
  a gate hall, a structure's foot, and the houses), a place of the plan its own records on top; the capital an archive of 8 to 14 and
  each city one of 5 to 8, some towns (one in three) one of 3 to 5 (a prehistoric capital 4 to 6 in its chief's longhouse): a row of
  records across the middle of the room its age kept them in (a temple's, a palace's hall, a town hall, a tower's ground floor, a spire's
  gate hall), the place's own first; the far future's structures keep theirs in rings round their feet (the archive that keeps itself).
  B-426: the archive holds its count and no more; the place's own beyond it lie one a building over the rest of the place (C-03's weights),
  then a second and a third in a room, then one at a time across the rooms or round the feet of the buildings holding one (a lone menhir's
  foot holds two), so every place holds all its own (a prehistoric capital that kept 38 of its people's 80: four in its longhouse's
  archive, the last recording in the same longhouse, the other 33 one each in 33 huts).
  The works that kept records hold a few (an observatory its charts, a necropolis its dead's, a shrine, standing stones, an antenna
  field's mast, a launch site's blockhouse, a lighthouse). A place left early holds nothing written after it was left; the old time's
  stones, left before year 1, nothing.
* **The last recording** lies where the history ended: at its last place, in the building its age would keep its last words in (a chief's
  longhouse, the temple or the palace's hall, the town hall or a station, the capital's mast, a spire's gate hall, the far future's
  structure), and in the last shelter by it (T-03); nowhere else.
* **A painted wall** is painted on the face of the room's wall away from its doorway (`paint`: the first wall box the line meets, the
  painting centred on that face and leaning with it): creatures, hands, a sky of points joined by lines, in ochre and white; recorded
  (`E`), it stays painted. Where no wall stands it is painted on a slab on the ground.

In the view (`06-surface.md`) each record is its object, drawn with the settlement's forms within 40 m (`gatherRecords`, the records' banks
29 to 31, clay the cut wood's 17, metal the chrome's 7, a stone and a sleeve in the people's colours, a crystal and a sphere in its light),
with C-03's point of light over it to 250 m; the HUD names it (`A CODEX - E`, `A PAINTED WALL - E`, a row over the sun's line), the taking
says what and where (`A CODEX OF THE OLD PEOPLE - 3 OF 147 ON THIS WORLD`; the log: `... TAKEN FROM AN ARCHIVE IN A TEMPLE IN THE RUINS OF A
CITY ON <WORLD>`, a painted wall `RECORDED`, a work's `AT A SHELTER`).

*The guide across the change.* A people's records are at `people x 1000 + index` in the world's indices (`shardWorldIndex`; C-13's second
people was at 50 + index): a guide saved before (no `records 2` line) is moved when it is loaded (`Guide::load`: the shards, decoded, heard,
lent, and the pieces' names), so the shards found before generation 13 are kept by their index, the counts and the language kept, the
contents the records' now; the game says once `RECORDS REGENERATED - 23 SHARDS KEPT, CONTENTS NEW` and logs it, and saves the guide.

Where to see it:

| Mode | What |
| --- | --- |
| `records <sx> <sz> <body> [second]` | a people's records: the count, by the age of their years and by medium, the last recording and the last place; where they lie over the plan's places, the works and the countryside round the capital (archives, painted walls, by place), the copies, and the sites of the capital, the first cities, a place left early, the last place, the works and a few villages |
| `records ages` | the six age worlds' |
| `records scenes [age]` | through the game, landed by each age's scene world's capital: an archive and each common medium's object, a frame before it, `E`, the frame after (`shots/tests/records_<age>_*.png`, a sheet `records_<age>_sheet.png`) |
| `records check` | the unit's T-04 checks alone |

## No one's ruins, the land's works, and not the season's (B-417, B-418, B-419)

**A world's ruins are its people's (B-417).** M4-07's monoliths of the old ones (a ring of eight columns, a cube, a dome, three walls, a
rare 40 m cube, in one 2 km cell in twenty of every felisian, quartz and ocean world, a people there or not) were nobody's: no age, no
genome, no record, drawn by code of their own. They are gone. `worldHadCivilisation` is the one question (`worldHasRuins` is gone); a world
without a people (seven felisian worlds in eight, every quartz and ocean world) has no ruins and its scanner no RUINS mode. On a people's
world the draw's first twentieth of a cell goes to the countryside's lone monuments, its people's in its age and genome, as on a dead
desert world before. A ruin's echo carries its settlement's class (`Landmark::sub`).

**What is made on a world is not its season's (B-419).** The plan, the people's extent, its histories, its settlements, roads and works
and the scanner's sights read the planet at the year's mean (`unseasoned`, galaxy/planetmap.h); the game's generators carry the season of
the moment (a winter's sea ice had counted as land, and nearly every place of a plan moved with the season of a session's first approach).

**A people's everyday works are its land's (B-418).** T-03's rules made a handful of each kind a people (24 quarries by its towns, four
necropolises by its capital and cities, four wrecks, three henges...), and T-01's plan gives a people hundreds of places over a third of its
world or more: over 41 people's worlds near home a tenth of the plan's places and one point of the peoples' land in two hundred had a work
within the scanner's 40 km. The world's list keeps the history's works and the world's great ones (the battlefields, the dams, the
aqueducts, the observatories, the lighthouses the history lit, the launch sites, the holy place and its way, the colossus, the fortified
line, the antenna field, the terraformed basins, the far future's, the lines on the plains, the last shelter, the mines the history sank);
the everyday ones are the land's:

* **The grid** (`landCellOf`, `landCellsAround`): rows of `LAND_CELL_M` (8 km) of latitude, each row's cells at least as wide; a cell's
  people the side of the divide its middle lies on.
* **Whether a cell holds one**: its hash under the countryside's density at its middle (`planCountryside`: the people's land, thicker near
  its places, none beyond its frontier) times its age's appetite (`LAND_RATE`: 0.34 prehistoric and medieval, 0.36 industrial and modern,
  0.5 futuristic round its great places, none in the far future, whose works are the world's).
* **What its ground offers** (`landWork`: a 5 x 5 reading of the cell and the plan's coarse grid at its middle): rock, open ground, forest,
  snow, a shore (water that is not an old sea), coal and ore (the plan's `res`), a hill (`relief`), the greatest settlement in or round it
  (the plan's places reaching in, the countryside's in its 2 km cells, not a lone monument). A prehistoric people's flint pits (on any
  ground but snow), standing stones (open ground), a hill fort (a hill); a medieval to modern people's quarry (rock for a stone-built
  people, else a pit on any dry ground), a mine (coal and ore), a wreck (a shore; a modern people's aircraft on open flat ground), a
  lighthouse (a shore by a settlement), a wind farm (a modern people's ridges), a fort (anywhere, likelier on a hill); a futuristic people's
  wreckage fallen from orbit; for every people but the far future's a necropolis by a village or more (a prehistoric people's by any
  settlement) where its genome buries its dead, and a shrine where the cell's ground is its holy place's kind. One is drawn by their
  weights; one that finds no site gives way to the next.
* **Where in it** (the kind's rule over a jittered grid of the cell, `landPoints`): its reach and 60 m inside the cell (no two of the land's
  touch: `landInside`), clear of the 2 km cells' settlements (their radius and 60 m), the cities' cores (150 m) and the world's works (40 m),
  as T-03's rules kept clear of each other (`WB::clearOf`, the world's works its `world`).
* **Made when asked** (`landWorkOf`): a cell's work (or none) made the first time it is asked for, in a few milliseconds, and kept (the last
  two worlds' cells, a world's cleared at 40,000); the world's works waited for first (the land's keep clear of them). `worksAround` gives
  the world's and the land's within a reach (the scanner's `worksHeardNear`, the landing's `settlementClearance`, the harness);
  `worksAroundIfReady` the view's, which never waits (its `WorkNear` holds each as a shared `Work`). `Work::land` marks the land's.
* **What the explorer meets**: from every point of a prehistoric to a modern people's land sampled by the unit (216) and every place (108)
  the scanner hears a work within 40 km, eleven on the whole; a scan's first hearing of a region takes 5 to 10 ms on the scanner's thread.
  A futuristic people's land is wild between its great places (T-01's countryside), and so are its works (KI-396).
* **The fort** (`WO_FORT`, `A FORT`, B-418's new kind): a prehistoric people's hill fort (one to three rings of bank, `BK_MOUND` style 4,
  inside their ditches, the huts of its last people within), a medieval people's castle (a curtain wall of five to eight sides, a wall tower
  at each corner, the gatehouse in the side toward the settlement it guarded, a keep back from it, a moat on the flat), an industrial
  people's redoubt (`BK_FORT`: its ring bank round a casemate, its ditch), a modern people's strongpoint (pillboxes round a blockhouse,
  dragon's teeth across the way up); one record in its keep or casemate (`recordSitesOf`).

## The genome within each age, the walls and the records (R-412, B-420, B-421)

R-412 (2026-10-11): B-415 let the age speak first; within an age a people's everyday buildings drew from fewer variables, and towns of one
age looked more alike. Sixteen more deep variables (`peoples/genome.*`, fifty-one in all; each its own hash, so A-04's thirty-five did not
move) act on what the eye picks out along a street, each read through the age as B-415 reads the rest (its expression the age's, its
choice the people's). `docs/iteration-3/plans/requests/R-412-the-genome-grows-within-each-age.md` has the table; in short:

* **Roofs**: PITCH (every pitched, hipped and conical roof and gable, a hut's cone: `Gen::pitchK`), EAVES (the overhang, a bellcast
  skirt, a modern slab's lip, deep slab or cornice: `Gen::eave`, `slabEave`), ROOFLINE (`rooflineOf`: dormers, a cross gable, cresting,
  a bell-cote or a ventilator; a dome's or a cone's lantern, a vault's crest, a cell roof's crest and lantern; on a hand-made people's
  flat roof a pavilion, a stair house, a balustrade, a cupola; on a modern people's a pergola, a water tank, rails, a penthouse), GABLES
  (`gables`: crow steps, a curved neck and its cap, a pediment's cornice and raking frame, a half hip), ROOFING (palette slot 2 in the
  age's material: `genomeRoofing`, eight colours by age).
* **Walls**: POLYCHROMY (a dado and courses through the windows' pattern, `WindowSpec::dado`, `stripe`; each house its own shade and a
  third in the accent, `Gen::tone`, `painted`, `Form::tone`; a lighthouse's bands), TRIM (`WindowSpec::frame`, `shutter`), PROJECTIONS
  (`projectionsOf`: balconies, galleries and a one-storey house's verandah, bays, oriels, each on the front as `walls4x` leans or jetties
  it; `ringDress` for a round house; `podDress` for a pod), ENTRANCES (`entranceAt`: porch, stoop, portico, hood or canopy; `hutEntrance`;
  a pod's arch, step, pylons, shell), CORNERS (`cornersOf`: quoins, pilasters, corbelled turrets or a low house's corner tower; a modern
  people's glazed corners, fins, rounded corners).
* **Town**: STOREYS (`Layout::heightK`), FRONTAGE (`Layout::frontage`; an industrial row a gable a house), SETBACK (the lots' setback,
  `Building::yard`), BOUNDARIES (`yardOf`, `yardRun`, `hutRing`: each under C-01's 0.9 m, stepped over), FURNITURE (`BK_FURNITURE`,
  `genFurniture`, `placeFurniture`: laid last in a town by a stream of its own, round a futuristic people's great structure), POLES
  (`hutPole`).
* **A new form**: `FK_FENCE` (pales, bars, wattle, a balustrade; its pales one by one near, its stretches as bands far; its colliders discs
  where its pales stand). A building in a row or round a compound's court is `Building::joined` (no turrets at its sides, no garden behind).
* **The works**: every pitched roof of a work takes the roofs' variables; a lighthouse its section, taper, bands, lantern and rail, a
  headframe its structure (a lattice, a brick tower, an A-frame), a turbine a lattice tower, a rounded nacelle and its blades by number,
  an observatory, a pillbox and a casemate the people's section, a tomb its gables and doorway, a hangar its roof; a church, a town
  hall, a station and a mill their doorways, corners and roofline.
* **The uniqueness** (`genomeDistance`): R-412's own among the strongest variables (the roofing 1, the pitch, the polychromy, the
  projections and the storeys 0.6, the eaves 0.5, the roofline, the corners, the frontage and the boundaries 0.4, the gables and the
  trim 0.3, the entrances 0.3): 19.8 at most, alike under a quarter of it (`GENOME_ALIKE` 4.9). The hundred peoples nearest home are
  10.0 apart at the nearest.

**B-420, the walls the explorer meets are the walls drawn** (`docs/iteration-3/bugs/resolved/B-420-*.md`): every kind's colliders follow
its drawn mesh (`formColliders`: a lathe's slices by their narrowest outline, a lying piece's section where it lies, a vault's flanks,
an arch's piers, a truss's legs, a sweep's faces band by band); a big piece stands on the highest ground under it with its colliders
(`pieceFootNear`); a heap, a bank, a barrow or a tier is stood on (`formWalkable`, `formSupportAt`); the rocks drawn are the rocks
collided with (`forRocks`). **B-421, the records lie where they can be reached**: off the drawn pieces at the floor and the heaps'
slopes (`Placer::clear`), out of the water (the view lays a place's records again with its drawn water within 600 m: `waterRecords`),
off the earthworks' banks, and not behind a doorway something fell across (`Placer::doorOpen`).
