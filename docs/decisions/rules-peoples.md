# Rules: the civilisations and their grammar

Rules in force for the dead peoples and what they built, one line per rule with its log entries (`log/`, by release). A milestone
that sets or changes a rule edits its line here; a replaced rule moves to `superseded.md` (`docs/CLAUDE.md`). Their records, charts,
signals, grammar and tongues: `rules-peoples-grammar.md`.

## Who lived where, and when
- One felisian or desert world in eight has a people by its own hash (`peopleHash`), not the trait draw: not every desert is dead (the user's) [C-01]
- Peoples live only on `KT_PEOPLE` kinds, on a dead kind by its row's salt (`peoplesHash`); the new living kinds hold none [P-04], [D-05]
- A wanderer has no people, and a drifter's system none but the one `drifterHadPeople` gives (`Body::drifter`) [P-04], [P-05], [D-01], [D-05]
- One people per world, two one time in five; the second is a second seed (`peopleGen` changes only `seed`, `BodyGen::worldSeed` stays) [C-13], [R-413]
- The age is drawn by fixed shares (28/32/18/13/7/2%) whatever the world; a second people takes the first's age, its own draw set aside [A-01], [R-413]
- Year 1 is the capital's founding (a prehistoric people's first village), earlier years negative; every year shown counts from it [A-01], [H-02], [T-04]
- Causes only append (`CAUSE_SUN_LOST` is given; picks stop at `CAUSE_DRAWN`); peoples ending together share one cause, by their age [A-01], [R-413], [D-05]
- Per-cell readers (culture, radar, charts, records' years and kinds) take only `peopleAgeOf`; `peopleProfileOf` gives the whole profile [A-01], [H-05]
- A people's world is read through `peoplesWorldOf`: a drifter's people read only their former system (`formerSystemOf`) and `sunLossOf` [D-05], [E-02]

## The plan and the history
- A people's things are functions of the galaxy (body, `BodyGen`, plan), never of a view or site: every reader sees one answer [C-01], [C-07], [C-09]
- No people shapes the planet function (`peopleGen` never reaches it); towns and works edit only the site's cache [C-01], [C-13], [T-02], [T-03]
- Made things read `unseasoned(g)`, on a dead world `thenWorld` (built by `thenKind`, worn by `nowKind`); sights read the present [B-419], [P-04]
- The plan thread builds a world's plan (`worldPlanOf`: the places a history can name, not the countryside), its histories and works [T-01], [T-02], [T-03]
- A history follows from profile, plan and world alone (the same from decoder or ground: `historyOfWorld`), and its places are the plan's [H-02], [T-02], [D-05]
- Two peoples meet only where their lands touch; meetings and wars come from the world's seed, alike in both histories [T-01], [H-02]
- Everyday events draw on their own streams after the years and never harm or make a person: every war, death and ruler stays [H-04 second pass]
- Inserting a history event kind (`HE_SUN`) renumbers those after it: event names, keys and grammar numbers move together [D-05]
- A cell in another latitude row is found by `ruinCellLonIn` (each row has its own longitude step) [T-01]

## On the ground
- A world shows only its people's last age (`ageFromOf`, the user's): nothing stands of a place left before it, while the history keeps every era [B-414]
- Every ruin is a people's, by its genome: `worldHadCivilisation` is the one question (no people, no ruins; no `RuinKind`) [B-417]
- Nothing stands in land no people held, nor in a city's core except the city; settlements and roads keep off water [C-01], [C-09], [T-01]
- A settlement reads the terrain once, at layout (`readSite` into `RuinSpec::site`); finders, roads and landing read only its reach [C-08], [A-03], [T-02]
- Works lie over the ruin cells (`worldWorksOf`, `landWorkOf`), never in them, so `ruinOfCell` is untouched; no work touches another or a town [T-03], [B-418]
- Towns lay streets first (`townStreets`): no building, nor a piece past its footprint, crosses a street (a unit check) [T-02]
- The road graph (relative neighbourhood) stays symmetric (both ends' cells judge a pair), connected in a region and planar [C-09]
- The ruins' wind follows the dune fields' rule (the world's angle, per hemisphere and band): change both together [C-08]

## Buildings, forms, the genome
- Building kinds only append (`BK_*`), and `addFeatures` draws after the layout's draws; a need takes new shapes of a form before a new kind [C-08], [R-412]
- Each genome variable is its own hash (`genomeOf`) in [0, 1), acting visibly in each age it claims; peoples stay distinct (`genomeDistance`) [A-04], [R-412]
- A `Form` is only geometry, for every reader; nothing rests on air: a raised piece has a piece of its building under it (a unit check) [A-02], [A-03], [A-04]
- Copy what a generator needs from a `Form&` before adding a piece: the reference dies when the next one is added [A-03]
- Decay is the years since the end (`materialDecay`; masonry `decayOf`), never a draw; for a drifter's people it stops at the cold [C-08], [A-03], [D-05]
- Each age has its vocabulary and the genome chooses within it (`ageExpressed`); great buildings (`greatKind`) keep the people's forms [B-414], [B-415]
- Industrial: brick, slate, sash, never crenels or onions. Modern: flat, glazed, concrete. Medieval: small windows. Only the far future glows [A-04], [B-415]
- A building stands on the ground it covers, with no bank or rim; pits, cuts and kerbs belong to the hand-made ages (`Layout::foundOf`) [B-416]

[C-01]: log/1-1.1.0.md
[C-07]: log/1-1.1.0.md
[C-08]: log/1-1.1.0.md
[C-09]: log/1-1.1.0.md
[C-13]: log/1-1.1.0.md
[A-01]: log/3-1.3.0.md
[A-02]: log/3-1.3.0.md
[A-03]: log/3-1.3.0.md
[A-04]: log/3-1.3.0.md
[H-02]: log/3-1.3.0.md
[H-04 second pass]: log/3-1.3.0.md
[H-05]: log/3-1.3.0.md
[T-01]: log/3-1.3.0.md
[T-02]: log/3-1.3.0.md
[T-03]: log/3-1.3.0.md
[T-04]: log/3-1.3.0.md
[B-414]: log/3-1.3.0.md
[B-415]: log/3-1.3.0.md
[B-416]: log/3-1.3.0.md
[B-417]: log/3-1.3.0.md
[B-418]: log/3-1.3.0.md
[B-419]: log/3-1.3.0.md
[R-412]: log/3-1.3.0.md
[R-413]: log/3-1.3.0.md
[P-04]: log/6-1.4.0.md
[P-05]: log/6-1.4.0.md
[D-01]: log/6-1.4.0.md
[D-05]: log/6-1.4.0.md
[E-02]: log/6-1.4.0.md
