# src/peoples/grammar: the records' grammar (107 .vg files on five shelves)

`files.inc` lists them, three lines a file (`{"<path>",`, `#include "<path>"`, `},`): `GRAMMAR_FILES` (`shards.cpp`) in the parse's
order. A file is a raw string (`R"VG(` its first line, `)VG"` its last). The engine: `grammar.h`; the model and the directives:
`genres.cpp`; what a guard, a mark or a directive may name: `vesperis_test grammar schema`.

## The shelves (each file's genres and rule prefixes: `docs/reference/civilisations/grammar-files.md`, generated)
- `core/`: the root (`records.vg`), the world's and the people's words, the culture pools (`culture_*`), `lexicon.vg`,
  `history.vg`, the charts' captions. `registers/`: an event told by kind (`annal*`, `letter_events*`, `news*`, `song*`, `log*`,
  `<register>.<kind>`; `picture.vg`, which no call expands: KI-424). `ages/<age>/`: each age's genres. `genres/`: those of every
  age (the letter and its life lines, the diary, the collapse's `ending_*`). `peoples/sunlost/`: the people who lost their sun.
- A file is named for its topic (a genre's name where it holds one), uniquely across the shelves; a number means continued past
  the cap (`daycount2.vg`).

## The format (grammar_parse.cpp's `Parser`; `docs/reference/civilisations/records-and-grammar.md`)
- `#` starts a comment. A rule is `name (opts)` at column 0, defined once; opts: `pl|sing|mass`, `keep`, `steady`, `formula`,
  `words=lo..hi`, `people=K`, `share=F`. Its alternatives are indented, one physical line each: `(weight guards once tense=..) text`.
- Guards: `k=v != >= <= > <`, `has:k`, `!has:k`; `a=1|b<2` is one OR. Marks: `{rule}`, `{fact}`, `[a|b]`, `{$x}` with
  `:obj|poss|refl|name|voc`, `{lemma:$x}`, `{#n|noun}`, `{@directive ...}` (`@news`, `@entries`, `@event`...: genres.cpp).
- `@root <rule|prefix.*>`: what the C++ expands by name (the link fails a rule no root reaches); `@lexicon <age|fact>`; `@apart`
  (a file's lines, or a guard's, out of the word counts); `@instead <guard> <rule> <rule2>` (rule2 where the guard holds).

## Rules
- Never edit, rewrap or re-space an alternative's line: its key hashes its rule's name and its trimmed text, and every people's
  dialect and `people=K`/`share` picks read it (a comment may change). A population's own line: `@instead` to a new rule.
- Never give a one-line rule a second line (it turns on draws). Keep a rule's shape: a phrase rule takes no `.?!` and no directive.
- A new population's words go under `@apart`, or every people's word fatigue moves. A word of 4+ letters in 24 lines or fewer is
  worn down by use: taking one to 25 lines switches that off for every people.
- A file stays under 16,000 bytes (MSVC's literal limit; a unit check) and no rule continues in another. A new file: on its
  shelf, its three lines at the end of `files.inc`. Files move whole.
- Text: never he/his/him or a person's verb ending (use the marks); a slot a verb follows is singular (`{skyone}`); pronouns in
  three cases tied to one relative per shard; crops and work items are mass nouns without "the"; never "a" before a `(pl)` or
  `(mass)` list; a later age's word is guarded by age and listed in lexicon.vg; a text that starts with `(` is written `\(`.
- Read fifty outputs (`grammar ages`) before judging: the checks catch grammar, not sense.

## Checks (the `grammar` skill)
- `vesperis_test grammar where <rule|prefix.*>`: its place, alternatives, what names it, the calls the C++ builds its name
  from, the roots that reach it; `grammar where check`. `grammar` reads the tree (no rebuild) and writes every people's texts
  through the checks (`out F` for a diff; `ages`, `rogues`). After `make`: `grammar check` (the pinned hash 0x13212ef73adea507
  among them) and `unit`. The regress hashes no text.

More, with the numbers: `docs/reference/lessons.md`, "The records' text".
