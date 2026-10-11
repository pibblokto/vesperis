# B-415: an industrial town read as modern and as medieval

status: resolved 2026-10-11
severity: S2
area: the civilisations (the buildings of every age: the genome read through the age)

## Report

The user, on a world they named BUBAS (Thutakrart III, sector 10549 -8 -5606, a rainy felisian world), with thirteen screenshots of the
Az-Spyzh's two towns (white flat-roofed blocks with dark bands of windows, curved blocks, tall white cylinders, rusted iron frames, a
barrel vault) and of its records (wax cylinders, a star chart photographed on glass, printed pages): "In your opinion based on what I
found + architecture what stage it is? Because in my opinion it's not clear at all." Told it was industrial (its records and its towns'
kinds said so, its buildings did not), they asked for the fix "not for industrial specifically - for all the stages so it's always clear
what stage we are dealing with. You see, for this one it was SUPER unclear what the stage was - it partially read as medieval due to
columns, partially as modern, it was unclear at all. It should be seriously revamped so it's clear (but whole genome thing stays)."

## Cause

BUBAS's first people reached the industrial age (its years 1870-2105; it ended 609 years ago, without radio): its 129 records are all
industrial (64 printed pages, 33 wax cylinders, 25 codices, 7 glass plates), its towns at the user's marks rows, tenements, villas, mills,
a town hall and a station. A-04's genome spoke as loudly in every age as in its own, and B-414 had given the machine ages' houses only
their plans, their roofs in place of domes and cones, and their windows' shapes:

* **Its walls**: a painted people's industrial walls were its base colour, a stucco (BUBAS's white), not brick.
* **Its roofs**: `ageRoof` kept an industrial people's flat roofs (BUBAS's; their masonry slabs outlast a ruin's walls) and a modern
  people's pitched, hipped and saddle ones.
* **Its windows**: one rule for every age: a people's glazing (BUBAS's wide, 0.64) and rhythm (grouped: four lights a bay) gave an
  industrial row nine tenths of each bay in glass, ribbon windows; a medieval people of wide glazing had walls of glass.
* **Its structure**: an expressed frame was iron posts a hand off the walls (in a ruin a rusted pergola); its colonnades along the main
  streets were columns of stone (the user's "medieval due to columns"); its mills' chimneys plain cylinders in the white of its walls,
  standing in rows like columns.
* **Its tops and great buildings**: crenels on industrial and modern buildings; a modern people's temple a ziggurat of concrete, a
  basilica of spires or a domed hall with onion-topped minarets, its round temple in a peristyle; an industrial round temple in its
  ring of columns; pointed arches and onion domes in every age.

Of 20,000 genomes, half are painted, 22% build flat roofs and 24% wide grouped windows: a quarter of all peoples had two of BUBAS's three
traits in their industrial towns, one in forty all three.

## Fix

**The age speaks first, the people second.** Every age has its vocabulary of materials, roofs, windows, tops and great buildings, and
the genome chooses within it; its variables stay the people's own through its ages.

* **The genome read through the age** (`ageExpressed`, galaxy/buildings.h; every generator reads it through `Gen::gv`): a building is one
  of the people's great buildings (its temples, churches, town halls, palaces, keeps and towers, focal accent, monuments, gates, platforms,
  rotundas, joined towers, aqueducts and tunnels: `greatKind`), where its genome speaks fully, or an everyday one (its houses, rows,
  tenements, villas, blocks, halls, mills and works), where the age speaks first. Crenels are the hand-made ages' alone; an industrial
  people's everyday buildings stand upright to their eaves (no battered, bulging or jettied walls, no storey set back on a flat roof)
  with chimneys for their tops, its great ones upright or stepped back; a modern people's everyday buildings upright and bare (no carved
  cornice) with a mast or nothing on their roofs, its great ones any profile but the medieval bulge; a futuristic or a far-future people
  sets no statues or crenels on its tops. Arches (`archSub`) are pointed in the hand-made ages alone (round or flat after), domes
  (`ageDome`) hemispheres in the industrial age where the people's were pointed or onions, saucers in the modern.
* **The industrial age: brick, slate, sash windows and chimneys.** Every industrial people built in brick, painted or not (`BRICK`: its
  base colour's clay, a gault and a cream stock for its whites and creams, yellow stocks, reds, a brown, a grey, a blue, a dark
  engineering brick, a blue-grey, an olive, a pale grey, a plum). Its everyday roofs are slate on timber, always (`ageRoof`: pitched for a
  people of gables, steps, vaults or saddles, hipped for the rest; a people of flat roofs hides a low one behind a parapet:
  `parapetRoof`, `Walls::parapet`), their gables of brick standing on the walls after the roof has gone (`gables`: every pitched roof on
  walls of stone, brick or adobe from the medieval age on), chimney stacks with their pots on every row, tenement and villa (`stack`); a
  mill's roof the age's (a row of gables across it for a people of cells) and its chimney black with soot at its lip; tall sash windows,
  one to a bay (a pair at the most, never scattered), round-headed where the people's openings were arched, round, pointed or keyholed;
  an expressed frame its brick piers between the windows, a piece of wall at a time (each lays its windows from its own end), under a
  ruin's ragged top. Its colonnades are covered walks of cast-iron columns under iron and glass, its platform a bandstand, its warehouses'
  columns iron, a church's windows its own and tall under its gables, a town hall hipped (behind its parapet for a people of flat roofs),
  its round temple a brick rotunda under an iron saucer, a tower's top a slate pyramid where it raised no spire or dome.
* **The modern age: concrete, flat roofs, wide glass.** Its everyday roofs flat always; wide panes or bands; an expressed frame its
  concrete grid (columns between the windows, a floor's slab at every storey); a people of cells a lift house to each module of its
  blocks. Its temple of its age: the stepped mass a low mausoleum of three broad setbacks banded with windows, the round temple a drum under
  a saucer or a cone, the domed hall a saucer and needles for minarets, the basilica under a shell of concrete (a steep gable, a vault, a
  saddle, a saucer, a tent; steps a flat roof) with its towers' tops needles, saucers or masts. Its focal tower a tower of the air with its
  pod; its colonnades pilotis under a slab, its platform a plinth and its mast.
* **The medieval age: small windows.** A third of a bay at the most, a pair or a group of lancets in half of it, scattered where the
  people scattered them; its pitched roofs' gables of stone.
* **The futuristic age**: an arched or keyholed people's windows portholes.
* **The genome's reach** (`GENOME_VARS`): the finish no longer acts in the industrial age (brick, painted or not), the glass's tint no
  longer in the modern (a modern ruin's glass is gone within years, its metal roofs went with the flat ones), the modules act in the
  modern through the lift houses.
* **Found on the way.** A highrise's mast and statue stood at its full height over a shaft that stood a few percent short of it (the
  tops stand on the shaft as it stands; the new tops found it in the unit's check). A record's spot was kept clear of its own building's
  pieces alone, so a neighbour's tower top fallen across a lobby lay over the modern test capital's archive (a town's layout reads its
  buildings' pieces, and moved with them); every piece over the building's ground counts now (`Placer::collide`).

## Verified

* **BUBAS** (`spot` at the user's two marks, by day): the MUSICAL CITY's street (the user's heading 302) a terrace of buff gault brick,
  three storeys of sash windows between brick piers under BUBAS's black bands, the barrel vault brick; the CITY's rows from the street
  (161, 288) and from the drone (35 m) roofless brick shells with their sash windows, their ragged tops without piers over them; the mills'
  chimneys brick.
* **The ages side by side** (`genome street`, new: the eight peoples nearest home in every age but the far future's, from a street half way
  into each town or from the drone, 30 and 700 years after the end): every industrial town brick under slate with its stacks, its sash
  windows round-headed or square, its mills' chimneys, its gasometers, stations and town halls; the round temple of a people of tombs a
  brick rotunda under its saucer; the modern towns flat-roofed with their bands and grids, the stepped temple a setback block, the
  basilica of a people of gables under a steep concrete gable with its campanile; the medieval towns' windows small.
* **The checks**: `genome reach` every variable reaching the drawing in the ages it acts in; `ruins check` (A-03): 126 settlements, 2,403
  buildings, 1,277 rooms, every piece resting on something, every doorway open; `records scenes modern` 0 failures. `unit` 0 failures (294 checks),
  `flow` exit 0 (34.3 s), the regress unchanged (22 hashes), `bench check` within every budget (the people's descent 62.5 ms of 80, the capital
  8.0 of 16).
