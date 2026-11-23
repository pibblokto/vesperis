# src/peoples/grammar: the records' grammar (109 .vg files on five shelves)

`files.inc` lists them, three lines a file (`{"<path>",`, `#include "<path>"`, `},`): `GRAMMAR_FILES` (`shards.cpp`) in parse order. A
file is a raw string (`R"VG(` its first line, `)VG"` its last). What a guard, a mark or a directive may name:
`vesperis_test grammar schema`.

## The shelves (each file's genres and prefixes: `docs/reference/civilisations/grammar-files.md`)
- `core/`: the root (`records.vg`), the world's and people's words, `culture_*`, `lexicon.vg`, `history.vg`, the charts' grammar. Z-06:
  a made species' watermark (`watermark.vg`, `watermark_far.vg`: the root `watermark`, rules `wm.`, every word `@apart`).
  `registers/`: an event told by kind (`annal*`, `letter_events*`, `news*`, `song*`, `log*`: `<register>.<kind>`).
  `ages/<age>/`: each age's genres. `genres/`: genres for every age. `peoples/sunlost/`: the people who lost their sun.
- A file is named for its topic, uniquely across the shelves. A number means a continuation past the cap (`daycount2.vg`).

## The format (grammar_parse.cpp's `Parser`; `docs/reference/civilisations/records-and-grammar.md`)
- `#` starts a comment. A rule is `name (opts)` at column 0, defined once. Opts: `pl|sing|mass`, `keep`, `steady`, `formula`,
  `words=lo..hi`, `people=K`, `share=F`. Its alternatives are indented, one physical line each: `(weight guards once tense=..) text`.
- Guards: `k=v != >= <= > <`, `has:k`, `!has:k`. `a=1|b<2` is one OR. Marks: `{rule}`, `{fact}`, `[a|b]`, `{$x}` with
  `:obj|poss|refl|name|voc`, `{lemma:$x}`, `{#n|noun}`, `{@directive ...}` (`@news`, `@entries`, `@event`...: genres.cpp).
- `@root <rule|prefix.*>`: what the C++ expands by name (the link fails a rule no root reaches). `@lexicon <age|fact>`. `@apart`: a
  file's lines, or a guard's, kept out of the word counts. `@instead <guard> <rule> <rule2>`: rule2 where the guard holds.

## Rules
- Never edit, rewrap or re-space an alternative's line: the key hashes the rule's name and the trimmed text, and every people's dialect
  and picks read it (a comment may change). A population's own line: `@instead`.
- Never give a one-line rule a second line (that turns on draws). A phrase rule takes no `.?!` and no directive.
- A new population's words go under `@apart`, or every people's word fatigue moves. A word of 4+ letters in 24 lines or fewer is worn
  down by use: taking one to 25 lines switches that off for every people.
- A file stays under 16,000 bytes (MSVC's literal limit, held by a unit check), and no rule continues in another file. A new file goes
  on its shelf, with its three lines at the end of `files.inc`. Files move whole.
- Text: never he/his/him or a person's verb ending (use the marks). A slot a verb follows is singular. Pronouns in three cases are tied
  to one relative per shard. Crops and work items are mass nouns without "the". Never "a" before a `(pl)` or `(mass)` list. A later
  age's word is guarded by age and listed in lexicon.vg. A text that starts with `(` is written `\(`.
- Read fifty outputs (`grammar ages`) before judging: the checks (the `grammar` skill) catch grammar, not sense.

More, with the numbers: `docs/reference/lessons.md`, "The records' text".
