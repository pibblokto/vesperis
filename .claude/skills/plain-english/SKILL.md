---
name: plain-english
description: Write and edit Vesperis's text in plain English - the game's words on screen, docs, log entries, B and R files, code comments, commit messages and reports to the user. Use before writing or changing any of them, and whenever the user says a text reads like an LLM.
---

# Plain English

The user, 2026-11-20: "Not llmish text with overexplanation. 'Its X', 'Its Y' - it's terrible. Also rule of three, a lot of
THE, etc. ... It's related to everything." That covers the game, the docs, the code comments and every report.

## Rules

1. Write what happens, in the order it happens: who does what.
2. Name the thing. Use "it" or "its" only when there is one thing it can mean, and at most once a sentence.
3. No chains of possessives. "the culture's crowd's share" becomes "the share of the crowd".
4. Drop "the" wherever the text reads fine without it: titles, labels, key hints, lists.
5. Cut every word that adds nothing. Short, common words: "use", "about", "before".
6. Don't explain what the reader can see or already knows. One reason is enough.
7. Don't pad a list to three. Two items are fine, so are four. Don't chain clauses with semicolons.
8. One name for one thing, everywhere (the words below). Don't vary terms for style.
9. Don't make up terms ("the wild law", "the own switch"). If a new term is needed, define it where it first appears.
10. No staged openers ("Here's the thing"), no closing summary, no "not X but Y" for emphasis, no dashes between every clause.
11. Docs say how things work now. History belongs in the log, the B and R files and git.

## Old habits in this repo, and their plain form

| Old | Plain |
|---|---|
| ITS SWITCH: IN THE DARK (WAS WHEN CROWDED) | SWITCH: IN THE DARK (WAS WHEN CROWDED) |
| IT DIES THERE - ITS PARENT LIVES | DIES HERE, PARENT LIVES |
| ITS CELLS ARE STILL - ITS PARENT'S MOVE | CELLS DON'T MOVE, PARENT'S DO |
| THE GENE EDITOR | GENE EDITOR |
| BACK TO THE LABORATORY | BACK |
| a law ending where its crowd crosses its quorum's share | growth changes when the population passes the quorum (3% of the maximum) |
| what is seen of a species the guide's (`microbe` lines) | The guide saves what the player has seen of each species (`microbe` lines). |
| Off, a gene loses its work as one cut and pays nothing. | A gene that is switched off works as if cut out, and costs nothing. |

## Game text

- Upper case. At most 53 characters a line at 1x (320 px, 6 px a letter).
- Titles and labels without "THE" or "ITS": "GENE EDITOR", "SWITCH: ALWAYS", "PARENT: GROWS -22 TO 0 C".
- Key hints are the key and a verb: "W WRITE  S SWITCH  ESC BACK".
- A message says what happened, then what to do: "NO EMPTY FLASK - EMPTY ONE IN THE INCUBATOR".
- Help text uses short sentences and says what to do first: "ENTER PUTS IN A SPECIES", "PUT A CULTURE IN FIRST - ENTER".

## Words

- **species**: a kind of microbe on a world. **strain**: a species changed in the lab; S1, S2 arise in a flask, E1, E2 come from
  the gene editor. **culture**: what grows in a flask. **collection**: the frozen cultures, kept in **folders**.
- **vial**: a sample from the ground.
- **kin**: related species. **gene**, **switch** (when a gene works). **crowd**: the most cells a flask holds.

## Reports and messages to the user

- Lead with the result. Then what they can try in the game, what is still open, and what they decide.
- Plain sentences of normal length. Keep the numbers. Explain a code term in a few words the first time, or leave it out.
- Before sending, reread for "its", "the", lists of three and anything said twice.

## Sources

Based on two published skills (both MIT), read 2026-11-20: b1rdmania/claude-plain-english-skill (Orwell and Gowers's rules, AI
tics) and blader/humanizer (signs of AI writing, after Wikipedia's list). This file is the project's own wording.
