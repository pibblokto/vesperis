# src/galaxy/grammar: the records' grammar (107 .vg files)

Each file is a raw string (`R"VG(` its first line, `)VG"` its last) that `src/galaxy/shards.cpp` compiles in: an `SRC_*` include
and a `GRAMMAR_FILES` row, in that order. The engine is `grammar.h/.cpp`; the model and the directives are `genres.cpp`. A number
in a file's name only means its family went on past the 16 KB cap.

## Families
- `records`: the root (`@root record`), which picks the genre by age, medium and history. `world`: the world's words (the sun,
  sky, land, water, crops, weather). `people`: the people's own words (gods, rites, the dead, rulers). `lexicon`: words by age.
- Registers: `annal*` (a year's entries), `letter_events*` and `letters*`, `news*`, `song*` (ballads, laments), `log*` (a
  station's log), `diary`, `history` (a late people's past), `charts` (star-chart captions), `culture*` (people=K pools).
- Ages: `prehistoric*` (no writing: marks, tallies, songs), `medieval*`, `industrial*`, `modern*`, `futuristic*`, `farfuture*`,
  `ending*` (the collapse); `sunlost*`: the peoples who lost their sun (all `@apart`).

## The format (grammar.cpp:399-515; docs/reference/12-civilisations.md:1712-1746)
- `#` starts a comment. A rule is `name (opts)` at column 0, defined once; opts: `pl|sing|mass`, `keep`, `steady`, `formula`,
  `words=lo..hi`, `people=K`, `share=F`. Its alternatives are indented, one physical line each: `(weight guards once tense=..) text`.
- Guards: `k=v != >= <= > <`, `has:k`, `!has:k`; `a=1|b<2` is one OR. Only the schema's keys (genres.cpp:54-68).
- Marks: `{rule}`, `{fact}`, `[a|b]`, `{$x}` with `:obj|poss|refl|name|voc`, `{lemma:$x}`, `{#n|noun}`, `{@directive ...}`.
- File directives: `@root`, `@lexicon <age|fact>`, `@apart`, `@instead <guard> <rule> <rule2>`. Text directives (genres.cpp:
  899-1300): `@cast`, `@news`, `@digest`, `@entries`, `@years`, `@past`, `@death`, `@deed`, `@life`, `@event`, `@suntable`.

## Rules
- Never edit, rewrap or re-space an existing line: an alternative's key hashes its rule's name and its trimmed text, and every
  people's dialect and `people=K`/`share` picks read it. To change a shared line for one population, `@instead` to a new rule.
- Never give a one-line rule a second line (it turns on draws). Keep a rule's shape: a phrase rule takes no `.?!` and no directive.
- A new population's words go under `@apart`, or every people's word fatigue moves. A word of 4+ letters in 24 lines or fewer is
  worn down by use: taking one to 25 lines switches that off for every people.
- A file stays under 16,000 bytes (MSVC's literal limit; a unit check); a full file points to a new rule in another. A new file
  is appended in GRAMMAR_FILES' order (shards.cpp:401 is one 2,575-character line: append at its end). Files move whole.
- Text: never he/his/him or a person's verb ending (use the marks); a slot a verb follows is singular (`{skyone}`); pronouns in
  three cases tied to one relative per shard; crops and work items are mass nouns without "the"; never "a" before a `(pl)` or
  `(mass)` list; a later age's word is guarded by age and listed in lexicon.vg; a text that starts with `(` is written `\(`.
- Read fifty outputs (`grammar ages`) before judging: the checks catch grammar, not sense.

## Checks
- `vesperis_test grammar` reads the tree (no rebuild needed), links it and writes every people's texts through the checks into
  `shots/tests/grammar_texts.txt` (exit 1 on a failure). `grammar out F` first, for a before-and-after diff; then `grammar ages`,
  `grammar rogues`, `grammar late N`.
- After `make`: `grammar check` (the unit's grammar checks: the tree equals the compiled copy, the sizes, every file listed, the
  pinned hash 0x13212ef73adea507 of the age and scene worlds' texts) and `unit`. The regress hashes no text.

More, with the numbers: `docs/reference/lessons.md`, "The records' text".
