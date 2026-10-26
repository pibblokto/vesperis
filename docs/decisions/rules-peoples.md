# Rules: the civilisations and their grammar

The rules in force for the dead peoples and what they built, a line each with the entries behind them (`log/`, by release).
A milestone that sets or changes one edits its line here; a replaced one moves to `superseded.md` (`docs/CLAUDE.md`).
Their records, charts, signals, grammar and tongues: `rules-peoples-grammar.md`.

## Who lived where, and when
- One felisian or desert world in eight has a people by its own hash (`peopleHash`), not the trait draw: not every desert is dead (the user's) [C-01]
- Peoples live only on `KT_PEOPLE` kinds, a dead kind's by its row's salt (`peoplesHash`); the new living kinds hold none [P-04], [D-05]
- A wanderer has no people; a drifter's system none but `drifterHadPeople`'s (`Body::drifter`) [P-04], [P-05], [D-01], [D-05]
- One people a world, two one time in five; the second is a second seed (`peopleGen` changes only `seed`; `BodyGen::worldSeed` stays) [C-13], [R-413]
- The age is drawn by fixed shares (28/32/18/13/7/2%) whatever the world; a second people takes the first's, its own draw set aside [A-01], [R-413]
- Year 1 is the capital's founding (a prehistoric people's first village), earlier years negative; every year shown is in that count [A-01], [H-02], [T-04]
- Causes append (`CAUSE_SUN_LOST` given; picks over `CAUSE_DRAWN`); peoples ending together share one cause, by their age [A-01], [R-413], [D-05]
- Per-cell readers (culture, radar, charts, records' years and kinds) take `peopleAgeOf` alone; `peopleProfileOf` is the whole [A-01], [H-05]
- A people's world is read through `peoplesWorldOf`: a drifter's people read their former system (`formerSystemOf`) and `sunLossOf` alone [D-05], [E-02]

## The plan and the history
- A people's things are functions of the galaxy (body, `BodyGen`, plan), never of a view or site: every reader sees one answer [C-01], [C-07], [C-09]
- No people shapes the planet function (`peopleGen` never reaches it); towns and works edit the site's cache alone [C-01], [C-13], [T-02], [T-03]
- Made things read `unseasoned(g)`, on a dead world `thenWorld` (built by `thenKind`, worn by `nowKind`); sights read the present [B-419], [P-04]
- The plan thread builds a world's plan (`worldPlanOf`: the places a history can name, not the countryside), its histories and works [T-01], [T-02], [T-03]
- A history follows from profile, plan and world alone (the same from decoder or ground: `historyOfWorld`), its places the plan's [H-02], [T-02], [D-05]
- Two peoples meet only where their lands touch; meetings and wars come from the world's seed, alike in both histories [T-01], [H-02]
- Everyday events draw on their own streams after the years and never harm or make a person: every war, death and ruler stays [H-04 second pass]
- Inserting a history event kind (`HE_SUN`) renumbers those after it: event names, keys and grammar numbers move together [D-05]
- A cell of another latitude row is found by `ruinCellLonIn` (each row its own longitude step) [T-01]

## On the ground
- A world shows only its people's last age (`ageFromOf`, the user's): nothing stands of a place left before it; the history keeps every era [B-414]
- Every ruin is a people's, by its genome: `worldHadCivilisation` is the one question (no people, no ruins; no `RuinKind`) [B-417]
- Nothing stands in land no people held, nor in a city's core but its city; settlements and roads keep off water [C-01], [C-09], [T-01]
- A settlement reads the terrain once, at layout (`readSite` into `RuinSpec::site`); finders, roads, landing read its reach alone [C-08], [A-03], [T-02]
- Works lie over the ruin cells (`worldWorksOf`, `landWorkOf`), never in them: `ruinOfCell` is untouched; no work touches another or a town [T-03], [B-418]
- Towns lay streets first (`townStreets`): no building, nor a piece past its footprint, across a street (a unit check) [T-02]
- The road graph (relative neighbourhood) stays symmetric (both ends' cells judge a pair), connected in a region and planar [C-09]
- The ruins' wind is the dune fields' rule (the world's angle, per hemisphere and band): change both together [C-08]

## Buildings, forms, the genome
- Building kinds append (`BK_*`); `addFeatures` draws after the layout's draws; a need takes a form's new shapes before a new kind [C-08], [R-412]
- Each genome variable is its own hash (`genomeOf`) in [0, 1), acting visibly in each age it claims; peoples stay distinct (`genomeDistance`) [A-04], [R-412]
- A `Form` is geometry alone for every reader; nothing rests on air: a raised piece has its building's piece under it (a unit check) [A-02], [A-03], [A-04]
- Copy what a generator needs from a `Form&` before adding a piece: the reference dies with the next one [A-03]
- Decay is the years since the end (`materialDecay`; masonry `decayOf`), never a draw; a drifter's people's stops at the cold [C-08], [A-03], [D-05]
- Each age has its vocabulary and the genome chooses within it (`ageExpressed`); great buildings (`greatKind`) keep the people's forms [B-414], [B-415]
- Industrial is brick, slate, sash, never crenels or onions; modern flat, glazed, concrete; medieval windows small; only the far future glows [A-04], [B-415]
- A building stands on the ground it covers, no bank or rim; pits, cuts and kerbs are the hand-made ages' (`Layout::foundOf`) [B-416]

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
