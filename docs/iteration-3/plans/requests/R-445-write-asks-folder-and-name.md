# R-445: writing a strain asks for its folder and its name

status: done 2026-11-20 (1.5.0, before its release; with R-446, B-445 and B-446)
milestone: 1.5.0

## Request

The user, 2026-11-20: "When you write modified organism to flask it HAS to prompt you what folder to put modified organism in AND has
to prompt you to enter the name with original organisms name prefilled (you can still name it whatever you want). Otherwise it will
get confusing really fast".

## Done

- **W on the gene editor** opens the folder list first (UNSORTED, the player's folders, NEW FOLDER). The folder that holds the parent
  is chosen to start, marked PARENT HERE. N or NEW FOLDER asks for a new folder's name first.
- **Then the name:** a text entry, NAME THE NEW STRAIN, holding the parent's name and the new strain's number: "VURYM E2", or "A ROD
  E1" when the species has no name. The number keeps the default from repeating the parent's name: the flow's first try, prefilled with
  the parent's name alone, named an amoeba's strain "AN AMOEBA", the same as its parent. The player can keep the default, add to it or
  replace it. An empty name is refused and the entry stays open. Esc at either step writes nothing.
- **The writing** then takes its two minutes as before. When it ends, the strain gets the name and is kept in the chosen folder at
  once, as well as going into an empty flask. The flask's own keeping finds it already kept, so there is no second copy in UNSORTED.
- **Saved** while writing: the editor's line `editorwriting <folder> <name>`.
- Proof and records: `docs/decisions/log/7-1.5.0.md` (this batch's entry); the unit's `genes` group ("W asks for the new strain's
  folder, then its name"); `lab genes` and the flow show the two prompts.
