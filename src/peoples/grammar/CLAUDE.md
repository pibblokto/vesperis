# src/peoples/grammar: the records' grammar (107 .vg files on five shelves)

`files.inc` lists them, three lines a file (`{"<path>",`, `#include "<path>"`, `},`): `GRAMMAR_FILES` (`shards.cpp`) in the
parse's order. A file is a raw string (`R"VG(` its first line, `)VG"` its last). What a guard, a mark or a directive may
name: `vesperis_test grammar schema`.

## The shelves (each file's genres and prefixes: `docs/reference/civilisations/grammar-files.md`)
- `core/`: the root (`records.vg`), the world's and people's words, `culture_*`, `lexicon.vg`, `history.vg`, the charts'.
  `registers/`: an event told by kind (`annal*`, `letter_events*`, `news*`, `song*`, `log*`: `<register>.<kind>`).
  `ages/<age>/`: each age's genres. `genres/`: every age's. `peoples/sunlost/`: the people who lost their sun.
- A file is named for its topic, uniquely across the shelves; a number means continued past the cap (`daycount2.vg`).

## The format (grammar_parse.cpp's `Parser`; `docs/reference/civilisations/records-and-grammar.md`)
- `#` starts a comment. A rule is `name (opts)` at column 0, defined once; opts: `pl|sing|mass`, `keep`, `steady`, `formula`,
  `words=lo..hi`, `people=K`, `share=F`. Its alternatives are indented, one physical line each: `(weight guards once tense=..) text`.
- Guards: `k=v != >= <= > <`, `has:k`, `!has:k`; `a=1|b<2` is one OR. Marks: `{rule}`, `{fact}`, `[a|b]`, `{$x}` with
  `:obj|poss|refl|name|voc`, `{lemma:$x}`, `{#n|noun}`, `{@directive ...}` (`@news`, `@entries`, `@event`...: genres.cpp).
- `@root <rule|prefix.*>`: what the C++ expands by name (the link fails a rule no root reaches); `@lexicon <age|fact>`; `@apart`
  (a file's lines, or a guard's, out of the word counts); `@instead <guard> <rule> <rule2>` (rule2 where the guard holds).

## Rules
- Never edit, rewrap or re-space an alternative's line: its key hashes its rule's name and its trimmed text, which every
  people's dialect and picks read (a comment may change). A population's own line: `@instead`.
- Never give a one-line rule a second line (it turns on draws). A phrase rule takes no `.?!` and no directive.
- A new population's words go under `@apart`, or every people's word fatigue moves. A word of 4+ letters in 24 lines or
  fewer is worn down by use: taking one to 25 lines switches that off for every people.
- A file stays under 16,000 bytes (MSVC's literal limit; a unit check) and no rule continues in another. A new file: on its
  shelf, its three lines at the end of `files.inc`. Files move whole.
- Text: never he/his/him or a person's verb ending (use the marks); a slot a verb follows is singular; pronouns in three cases
  tied to one relative per shard; crops and work items are mass nouns without "the"; never "a" before a `(pl)` or `(mass)`
  list; a later age's word is guarded by age and listed in lexicon.vg; a text that starts with `(` is written `\(`.
- Read fifty outputs (`grammar ages`) before judging: the checks (the `grammar` skill) catch grammar, not sense.

More, with the numbers: `docs/reference/lessons.md`, "The records' text".
