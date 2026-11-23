# Rules: the civilisations and their grammar (the records)

Rules in force for what the dead peoples wrote and left, one line per rule with its log entries (`log/`, by release). A milestone
that sets or changes a rule edits its line here; a replaced rule moves to `superseded.md` (`docs/CLAUDE.md`). Peoples and what they
built: `rules-peoples.md`.

## Where the records lie
- A people's first fifty records keep C-04's music, C-10's charts, C-12's last recording: a found shard stays what it was [T-04], [H-05]
- Moving records changes no key (a record is its index), needs no bump; a place keeps nothing written after it was left [T-04], [B-426]
- A record's year (`recordYear`) is C-02's first draw of it over the last age's years: C-02's order holds, the last recording latest [T-04], [B-414]
- Every record has a home (`recordHomesOf`), a place standing in its year; copies lie on top, one taken is gone everywhere [C-03], [T-04]
- The last recording lies only at `History::lastPlace` and in the last shelter (`recordSitesOf`'s `before`, 0 in the others) [C-12], [T-04], [D-05]
- Records lie on the ground in rooms or at a holder's foot, clear of doorways, pieces, heaps and water, each in reach [C-03], [A-04], [B-421]
- An archive holds its class's count, the place's other records one per building, never heaped [B-426]
- Instruments go in `recordSitesOf` after the records, on their own stream, beside music records: no record moves [C-11]
- Instruments are made by their peoples (plan, materials, style by the genome), not picked or named by age; only hand-held ones are found [C-11], [C-17]
- Drones fetch from the records' homes, copies filling their hands; their finds leave the ruins [C-18]

## Signals and charts
- A transmitter is a people's world with radio: a system's first C-01 people's, then a dead one's (`systemPeoplesWorld`) [C-07], [A-01], [P-04]
- A slot's programme follows its people's age (`programmeOf`); a modern people's are C-07's [C-16]
- A chart shows its people's one set of sky figures (`skyFiguresOf`), marking no voice they could not hear, C-10's marks first [C-10], [A-01], [H-05], [T-04]
- `chartSkyOf` reads exactly `StarNeighborhood`'s cube (every charted star in the people's sky): change both together [C-10]
- `peopleNameOf` reads only the lore's first draw: another world's whole `loreOf` would recurse forever [C-10]

## What a text is
- Texts come from the grammar over the history, offline, deterministic by world and index, never a model [C-02], [H-01], [H-02]
- Each people's texts and names are their own (the user's): few shared frames, no stock word on every world [H-04 second pass]
- Texts are translations (our nouns, their names, `{~key}` for theirs) and use no unit of ours (the world's day, moon, year) [C-02], [C-12], [H-04 second pass]
- Texts fit their object and world and tell what the explorer finds; a drifter's people tell of their lost sun (the user's) [H-04], [H-04 second pass], [D-05]
- Most records are everyday; the collapse's registers only in its years (`stage`), the end's tone in the last recording (`pickTone`) [C-02], [C-12], [H-04]
- Anachronism fails by age and invention: `@lexicon <age>` lists the forms texts use, `@lexicon inv.<x>` one awaiting an invention [H-03], [H-04]

## The catalogue and its draws
- A feature never moves another people's records: a latent fault of the shared grammar is a bug of its own [D-05]
- The shard stream keeps C-02's draws (herd, kin, child, tone) and reads `Lore::style`; the text's own seed moves no tone or piece [H-03], [H-04 second pass]
- A people's records are one catalogue in index order with the engine's memory (`shardTextsOf`); a lone shard reads the same [H-01], [H-03]
- The age and scene worlds' texts are pinned (`13212ef73adea507`); every check at zero, every name the history's at its year [H-04], [D-05]

## The grammar's files
- An alternative's line is never edited (its key is the rule's name and text); a people's variant is `@instead <guard> <rule> <rule'>` [D-05]
- A new alternative keeps its rule's flags (no directive or stop it lacks) and never joins a one-alternative rule (that changes `many`) [D-05]
- `@apart` keeps a population's words out of the distinctive-word counts; a word in 24 alternatives or fewer wears with use [H-04 second pass], [D-05]
- A text outside the catalogue (a caption, a watermark) takes no memory, tells no event, keeps its words apart: no people's records move [Z-06]
- A `.vg` file (in `R"VG(` `)VG"`, read from disk alike) stays under 16 KB, split at a rule: no rule runs across files [H-01], [H-03], [D-05]
- The files sit on src/peoples/grammar's shelves by role, named by topic; `files.inc` lists them, append-only [E-11]
- The realiser owns the English: texts name entities by marks (`:obj`, `:poss`, `:refl`, `vgName`), never pronouns; no place is "it" [H-01], [H-03], [H-04]
- Variety is pools and composition, not weights (`people=K`, `share=F` per people); a dialect leans only on the genres' root [H-01], [H-04 second pass]

## Tongues and the language
- A people's names (lore, persons, places, graves, decoder words, voice) come from their tongue (`tongueOfSeed`: the seed alone) [C-13], [H-04 second pass]
- Tongue words have at least two letters; a name reading as a rude or famous word of ours, or stuttering, is redrawn [C-06], [H-04 second pass]
- The share of language read follows the shards held (`languageShare`: lemmas, nothing else stored); unlearnt words show in their tongue [C-06], [H-01], [H-04]

[C-02]: log/1-1.1.0.md
[C-03]: log/1-1.1.0.md
[C-06]: log/1-1.1.0.md
[C-07]: log/1-1.1.0.md
[C-10]: log/1-1.1.0.md
[C-12]: log/1-1.1.0.md
[C-13]: log/1-1.1.0.md
[A-01]: log/3-1.3.0.md
[A-04]: log/3-1.3.0.md
[C-11]: log/3-1.3.0.md
[C-16]: log/3-1.3.0.md
[C-17]: log/3-1.3.0.md
[C-18]: log/3-1.3.0.md
[H-01]: log/3-1.3.0.md
[H-02]: log/3-1.3.0.md
[H-03]: log/3-1.3.0.md
[H-04]: log/3-1.3.0.md
[H-04 second pass]: log/3-1.3.0.md
[H-05]: log/3-1.3.0.md
[T-04]: log/3-1.3.0.md
[B-414]: log/3-1.3.0.md
[B-421]: log/3-1.3.0.md
[B-426]: log/5-1.3.2.md
[P-04]: log/6-1.4.0.md
[D-05]: log/6-1.4.0.md
[E-11]: log/6-1.4.0.md
[Z-06]: log/7-1.5.0.md
