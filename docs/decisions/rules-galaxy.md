# Rules: the galaxy and its generation

The rules in force for the galaxy, its stars and what generates them, a line each with the entries behind them (`log/`, by
release). A milestone that sets or changes one edits its line here; a replaced one moves to `superseded.md` (`docs/CLAUDE.md`).
Systems, kinds of world, the almanac and the probe: `rules-galaxy-worlds.md`.

## GEN_VERSION
- A change to what a star, system, body or surface is (a generation hash, a map, what a save names) bumps `GEN_VERSION`, regress moved or not [M9], [X-03]
- No bump for the drawing alone (the regress re-blessed), a value derived beside the generator, records moved under their keys [B-403], [B-408], [B-426], [F-03]
- Once per series, never per milestone: a change before the release rides the bump made (1.3.0 is 13, 1.4.0's P, Q, D 16) [O10], [B-404], [C-15], [Q-03]
- A bump adds its row and section atop `generations.md` beside its log entry, before any regress bless; saves record it (`rules-game.md`) [M9], [E-02]
- A generation change is proven body by body (`fingerprint bodies`): only the bodies it means to move differ [P-01], [Q-03], [E-02]

## Draws, hashes and salts
- Generation is a pure function of hashed integer coordinates and seeds in double precision; 32-bit `cloudNoise2/3` is the probe decks' alone [table], [X-02]
- A new decision takes a hash or salt of its own, or draws after every existing draw, never inside a stream: nothing made before moves [M5], [S-01], [C-01]
- A fix reuses the draws in their places (a floor, a threshold); a retired, moot or rejected draw is still taken [C-08], [C-12], [B-425], [D-05]
- Saved and drawn enums append (kinds before `PT_COUNT`, star classes, trait ids, `KS_` bits), never inserted or renumbered [S-03], [P-00], [Q-03], [D-01]
- Picks stop at `PT_DRAWN_COUNT` and `STAR_DRAWN_COUNT` (`Rng::pick` falls to the last): what lies past them is given, never drawn [P-01], [D-01], [D-05]
- Moved code keeps each float expression's shape (clang fuses a multiply-add within one); a term a row lacks is skipped, not added as 0 [P-00]
- The noise keeps its bits: `hash3i` composes `hashX`, `hashXY`, `hashXYZ`; `grad3` uses Perlin's selects; `ifloor` is `std::floor`'s everywhere [F-07]

## Protected and pinned
- The home star Heilya is pinned (`HOME_SX/SY/SZ`), yellow; its sector hash 0.142 sits under the density 0.152: a retune over 1/15 loses it [G-02], [S-04]
- The home world Heilya II stays felisian (`PT_STEPPE`'s salt pinned); `newGame` searches only if the home star is gone or has no living world [G-02], [P-02]
- A people's world (`peopleHash`, read first by `kindOf`) is never made another kind; a people's moon keeps its orbit, month and day [P-01], [P-07]
- Regress, bench, screens: Feeyeithis II-V, Wyariothmai I-III, V-b, Seapheeseeri II, I-a, Iaphiath I-II, Heilya V, IV-a stay as drawn [P-03], [P-05], [P-06]
- The probe's Wailie II stays cratered, Thilawiane III and the user's Syaniapheela II gas giants: salts pinned (`pinned:` in kinds.cpp, unit) [P-07]
- Every star class keeps a few members near home; the harness's pinned bodies exist under every generation or are pinned again [G-01], [S-04], [S-06]
- The transmitters' salts and rate are pinned (`SALT_CELL`, `SALT_DEEP`, `SALT_POWER`): home hears 3 at x1, 1-3 more a step [C-07], [A-01], [C-15], [R-413]
- Class salts are fixed: `starVariety` 0x5A11E7, the Wolf-Rayet 0x3E5F, the black hole 0x8B1E, a companion's `hashCombine(seed, 901)` [S-01], [S-05], [S-06]
- The drifter field is pinned: the nearest forty's hash is 0xb00ac446b619bf6f, over their drawn kinds [D-01], [D-04]

## Stars and drifters
- A star is a function of its sector alone, never moved by system or surface draws; `StarNeighborhood::update` keeps a fresh walk's order [M9], [F-02]
- Heliocentric: home keeps its sectors, the centre is sector (12000, 0, -5500) (`galaxyTerms`); a sector, a light year, holds any system [G-01], [table]
- A new class is a variety: `starVariety` claims a share of a family by region by `mix64(seed ^ salt)` (never a plain XOR), from the stars left [S-01], [S-04]
- A star class brings its table row, a `describeStar` sentence and a `starActivity(cls)` entry, the one activity table [S-01], [S-02]
- A cluster lies inside its cell, so a density query hashes one cell, not 27 [G-02]
- A drifter lies only in a sector with no star (`sectorHoldsStar`), on a field of its own salt: no star, system, sky or surface moves [D-01]
- The starfield, the neighbourhood, the aim, `jumpTarget` and a new game hold stars alone; a drifter is reached by its sector (`placeInSector`) [R-407], [D-01]
- `DARK_KINDS` is drawn (a change re-deals every drifter's kind, power and voice) and never living; a dark kind is made by its own salt [D-01], [D-04]
- Sunless (`sunless()`): `lightAt` and `seasonOf` give 0, the almanac skips the sun; `Body::heatK` is set on a drifter's bodies alone [D-01]

[table]: log/0-1.0.0.md
[M5]: log/0-1.0.0.md
[M9]: log/0-1.0.0.md
[O10]: log/0-1.0.0.md
[B-403]: log/1-1.1.0.md
[G-01]: log/1-1.1.0.md
[G-02]: log/1-1.1.0.md
[S-01]: log/1-1.1.0.md
[B-404]: log/1-1.1.0.md
[S-02]: log/1-1.1.0.md
[S-03]: log/1-1.1.0.md
[S-04]: log/1-1.1.0.md
[S-05]: log/1-1.1.0.md
[S-06]: log/1-1.1.0.md
[C-01]: log/1-1.1.0.md
[C-07]: log/1-1.1.0.md
[C-08]: log/1-1.1.0.md
[C-12]: log/1-1.1.0.md
[B-408]: log/2-1.2.0.md
[X-02]: log/2-1.2.0.md
[X-03]: log/2-1.2.0.md
[R-407]: log/2-1.2.0.md
[A-01]: log/3-1.3.0.md
[C-15]: log/3-1.3.0.md
[R-413]: log/3-1.3.0.md
[F-02]: log/4-1.3.1.md
[F-03]: log/4-1.3.1.md
[F-07]: log/4-1.3.1.md
[B-425]: log/5-1.3.2.md
[B-426]: log/5-1.3.2.md
[P-00]: log/6-1.4.0.md
[P-01]: log/6-1.4.0.md
[P-02]: log/6-1.4.0.md
[P-03]: log/6-1.4.0.md
[P-05]: log/6-1.4.0.md
[P-06]: log/6-1.4.0.md
[P-07]: log/6-1.4.0.md
[Q-03]: log/6-1.4.0.md
[D-01]: log/6-1.4.0.md
[D-04]: log/6-1.4.0.md
[D-05]: log/6-1.4.0.md
[E-02]: log/6-1.4.0.md
