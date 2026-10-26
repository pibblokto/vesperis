# Decision log: moved to `docs/decisions/`

E-04 (2026-10-24) moved this log, word for word, to `docs/decisions/`:

- `rules-<area>.md`: the rules in force, one line each with its ids, by area. Start there before changing anything.
- `log/`: the entries by release, `0-1.0.0.md` to `6-1.4.0.md`, the newest last; `cat log/*.md` is this file as it was.
  `grep -n '^## B-428' docs/decisions/log/*.md` finds an entry; read the entry, never a log whole.
- `superseded.md`: the rules that were replaced, and by what. `generations.md`: what each `GEN_VERSION` changed.
