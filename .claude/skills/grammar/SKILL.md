---
name: grammar
description: Write or change the records' grammar (the .vg files the peoples' texts are written from) without moving another people's texts - the shelves and files.inc, the directives (@root, @apart, @instead), the guards and marks, the append-only lines, the 16 KB cap, `grammar where` and `grammar schema`, the checks and the pinned hash, and reading fifty outputs.
---

# Write or change the records' grammar

## Read first

- `src/peoples/grammar/CLAUDE.md` (the shelves, the format, the rules) and `docs/decisions/rules-peoples-grammar.md` ("What a text
  is", "The grammar's files"); the recipe in `docs/reference/09-cookbook.md` ("Add a record's genre, a register, a rule or a
  word"). The generated map of every file's genres and rule prefixes: `docs/reference/civilisations/grammar-files.md`.
- Find before reading: `./vesperis_test grammar where <rule>` gives a rule's file:line, its alternatives, every alternative that
  names it, the directive calls whose names the C++ builds into it (`{@entries annal}` builds `annal.<kind>`) and the roots that
  reach it; `grammar where <prefix>.*` a family; `grammar where` the C++'s naming conventions. `grammar schema`: the facts, the
  entities, their paths and fields, and the words each takes (a guard's `medium=reel`, `$e.kind=war`).

## Nothing another people wrote moves

- An alternative's line is never edited, rewrapped or re-spaced, nor a one-line rule given a second line: its key is its rule's
  name and its text. A population's own version of a shared line is `@instead <guard> <rule> <rule2>`.
- A population's own words go under `@apart` (a file, or `@apart key=value`): otherwise its words change which words are
  distinctive (the fatigue) for every people.
- A new genre is an alternative of `record` (core/records.vg) guarded so no existing people's draws move; a new event kind's text
  is a rule `<register>.<kind>` in each register that should tell it.

## Files

- A file stays under 16,000 bytes (the unit's check; MSVC's literal limit) and a rule cannot continue in another file: a full file
  names a new rule that a new file defines. A new file goes on its shelf, named for its topic (unique across the shelves; a
  number only for a continuation), and its three lines go at the end of `src/peoples/grammar/files.inc`. Files move whole.
- A comment may change; a file's line count need not hold, but its alternatives' lines do.

## Check, then read

- `./vesperis_test grammar out build/<x>/before.txt` before the change, the same to `after.txt` after: every people's texts
  through the checks (exit 1 on a failure), read from the tree with no rebuild. Diff them: only the texts meant to change differ.
- `./vesperis_test grammar where check`: every rule named, built or stood in for (a `FAIL(<KI>)` line is a known issue's, not
  counted).
- Read fifty outputs (`grammar ages`; `grammar rogues` for the drifters' peoples): the checks catch grammar, not sense.
- After `make`: `grammar check` (the tree equals the compiled copy, every file listed, the sizes, the age and scene worlds' texts
  pinned at 0x13212ef73adea507: it moves only with a change meant for those peoples, the log saying why) and `unit`; the battery's
  `fp_peoples` (the `prove` skill) says which peoples' records moved. `make check` holds the generated map
  (`./vesperis_test grammar where md > docs/reference/civilisations/grammar-files.md`).
