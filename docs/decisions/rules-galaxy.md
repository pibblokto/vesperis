# Rules: the galaxy and its generation

Rules in force for the galaxy, its stars and what generates them, one line per rule with its log entries (`log/`, by release). A
milestone that sets or changes a rule edits its line here; a replaced rule moves to `superseded.md` (`docs/CLAUDE.md`). Systems,
kinds of world, the almanac and the probe are in `rules-galaxy-worlds.md`.

## GEN_VERSION
- A change to what a star, system, body or surface is (a generation hash, a map, what a save names) bumps `GEN_VERSION` even if regress holds [M9], [X-03]
- No bump for drawing alone (regress is re-blessed), a value derived beside the generator, or records moved under their keys [B-403], [B-408], [B-426], [F-03]
- Bump once per series, not per milestone: changes before the release ride the bump made (1.3.0 is 13, 1.4.0's P, Q and D are 16) [O10], [B-404], [C-15], [Q-03]
- A bump adds a row and a section at the top of `generations.md` with its log entry, before any regress bless; saves record it (`rules-game.md`) [M9], [E-02]
- A generation change is proven body by body (`fingerprint bodies`): only the bodies it means to move differ [P-01], [Q-03], [E-02]

## Draws, hashes and salts
- Generation is a pure function of hashed integer coordinates and seeds, in double precision; only the probe decks use 32-bit `cloudNoise2/3` [table], [X-02]
- A new decision takes its own hash or salt, or draws after every existing draw, never inside a stream, so nothing made before moves [M5], [S-01], [C-01]
- A fix reuses draws where they are (a floor, a threshold); a retired, moot or rejected draw is still taken [C-08], [C-12], [B-425], [D-05]
- Saved and drawn enums only append (kinds before `PT_COUNT`, star classes, trait ids, `KS_` bits), never inserted or renumbered [S-03], [P-00], [Q-03], [D-01]
- Picks stop at `PT_DRAWN_COUNT` and `STAR_DRAWN_COUNT` (`Rng::pick` falls back to the last); entries past them are given, never drawn [P-01], [D-01], [D-05]
- Moved code keeps the shape of each float expression (clang fuses a multiply-add within one); a term a row lacks is skipped, not added as 0 [P-00]
- Noise keeps its bits: `hash3i` composes `hashX`, `hashXY` and `hashXYZ`, `grad3` uses Perlin's selects, `ifloor` matches `std::floor` everywhere [F-07]
- Copying an `Rng` forks its stream, so helpers take `Rng&`; the two copies in the code are meant (system.cpp's moons drawn again, a refrain) [E-12]

## Protected and pinned
- The home star Heilya is pinned (`HOME_SX/SY/SZ`) and yellow; its sector hash 0.142 is under the density 0.152, so a retune over 1/15 loses it [G-02], [S-04]
- The home world Heilya II stays felisian (`PT_STEPPE`'s salt pinned); `newGame` searches only if the home star is gone or has no living world [G-02], [P-02]
- A people's world (`peopleHash`, read first by `kindOf`) is never made another kind; a people's moon keeps its orbit, month and day [P-01], [P-07]
- Kept as drawn for regress, bench, screens: Feeyeithis II-V, Wyariothmai I-III, V-b, Seapheeseeri II, I-a, Iaphiath I-II, Heilya V, IV-a [P-03], [P-05], [P-06]
- Pinned salts (`pinned:` in kinds.cpp, the unit) keep the probe's Wailie II cratered and Thilawiane III and the user's Syaniapheela II gas giants [P-07]
- Every star class keeps a few members near home; the harness's pinned bodies exist in every generation or are pinned again [G-01], [S-04], [S-06]
- Transmitter salts and rate are pinned (`SALT_CELL`, `SALT_DEEP`, `SALT_POWER`): home hears 3 at x1 and 1-3 more each step [C-07], [A-01], [C-15], [R-413]
- Class salts are fixed: `starVariety` 0x5A11E7, Wolf-Rayet 0x3E5F, black hole 0x8B1E, a companion `hashCombine(seed, 901)` [S-01], [S-05], [S-06]
- The drifter field is pinned: the hash of the nearest forty's drawn kinds is 0xb00ac446b619bf6f [D-01], [D-04]

## Stars and drifters
- A star depends on its sector alone and is never moved by system or surface draws; `StarNeighborhood::update` keeps a fresh walk's order [M9], [F-02]
- Heliocentric: home keeps its sectors, the centre is sector (12000, 0, -5500) (`galaxyTerms`); a sector is a light year and holds any system [G-01], [table]
- A new class is a variety: `starVariety` claims a share of a family by region with `mix64(seed ^ salt)` (never a plain XOR), from the stars left [S-01], [S-04]
- A star class brings a table row, a `describeStar` sentence and a `starActivity(cls)` entry [S-01], [S-02]
- A cluster lies inside its cell, so a density query hashes one cell, not 27 [G-02]
- A drifter lies only in a sector with no star (`sectorHoldsStar`), on a field with its own salt, so no star, system, sky or surface moves [D-01]
- The starfield, neighbourhood, aim, `jumpTarget` and a new game hold only stars; a drifter is reached by its sector (`placeInSector`) [R-407], [D-01]
- `DARK_KINDS` is drawn (changing it re-deals every drifter's kind, power and voice) and never living; a dark kind is made by its own salt [D-01], [D-04]
- In a sunless system (`sunless()`) `lightAt` and `seasonOf` give 0 and the almanac skips the sun; only a drifter's bodies set `Body::heatK` [D-01]

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
[E-12]: log/6-1.4.0.md
