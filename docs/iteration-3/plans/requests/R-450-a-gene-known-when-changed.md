# R-450: a gene known when changed

status: done 2026-11-22 (1.5.0, before its release)
milestone: 1.5.0

## Request

The user, 2026-11-22: "How to actually find out what gene does? I modify it, new organism comes out but it's dead", "I loaded another
organism from another planet and some of its genes were already known, not cool", "How to know what world gene does immediately", and
"For genes you modified - it's unclear how to understand what gene changes - sometimes you change a world gene/regular gene and it's
still unknown. I want you to make it so that purpose of a gene you change is clear effective immediately after changing. Also explain
what different kind of genes mean and are for". On the plan: "Proceed. What I would like you also to do is to somehow guide what change
will adjust in a given gene once you know what gene is responsible for."

## Done

- **A gene is known the moment it is changed** on the gene editor: its letters, a cut, a gene moved in from a kin (for the kin's species
  too), a copy or a switch. Nothing has to be written or grown first. A line put on the editor has its own changes from its species
  known as well, so a line written before this shows them.
- **Known by species.** The guide keeps what is known by the species and the gene's number (`gene` lines `<species key>/G<number>`).
  Another species of the same origin of life starts with every gene NOT KNOWN. An older guide's lines by origin of life, with their
  NOTHING SEEN and the gene names given by origin, are dropped when it loads.
- **What it is for.** The line under the map names the gene chosen and what its kind is for: `GENE 5: WARMTH - THE WARMTHS IT GROWS AT`,
  `GENE 27: GAS - THE GAS IT GIVES OFF: A WORLD'S AIR`. A gene not known yet says `CHANGE IT TO SEE WHAT IT DOES`.
- **How to change it.** With a gene's letters open (Enter), the map's place shows its guide: how its letters set it (`ITS LETTERS'
  WEIGHTS ADD UP: HEAVIER, WARMER`, `LETTERS 1-2 PICK WHICH, 3-6 HOW MUCH: HEAVIER, MORE`, `ANY CHANGE TO ITS LETTERS KILLS THE STRAIN`),
  each letter's weight in this origin of life (`WEIGHTS: C 0 G 1 T 2 A 3`), its pick and its weight now against the parent's, and what
  every letter of the alphabet at the cursor would make of it (`LETTER 3 SET TO:` then `A BEST 2 C`, `C BEST -10 C`, `G BEST -6 C`,
  `T BEST -2 C`), the current one bright.
- **A vital gene.** Changed, the editor says `A VITAL GENE IS CHANGED: THE STRAIN WOULD DIE`, and W refuses to write it. A switch on a
  vital gene (S) is still allowed, as its line lives where the switch is on.
- **The flask and the microscope** still say what an edited line does against its parent, now with the gene that does it
  (`FLASK 1: GROWS AT 29 C, PARENT DIES: GENE 8: WARMTH GENE`). The guesses (`ITS WARMTH: GENE 2 OR 8`) and NOTHING SEEN are gone.
- **Verified:** see the log entry R-450.
- **Left:** a line written before this, but never put on the editor again, has its changed genes NOT KNOWN until it is.
