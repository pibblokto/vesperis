# R-446: plain English everywhere

status: done 2026-11-20 for the game's text and the working docs (1.5.0, before its release); the reference notes are open
milestone: 1.5.0

## Request

The user, 2026-11-20: "Not llmish text with overexplanation. "Its X", "Its Y" - it's terrible. Also rule of three, a lot of THE, etc.
Some plain english skill also if you can find and apply it. It's related to everything".

## Done

- **A skill:** `.claude/skills/plain-english/SKILL.md`, the project's rules for game text, docs, comments, commits and reports. It
  draws on two published skills found with `npx skills find` (b1rdmania/claude-plain-english-skill and blader/humanizer, both MIT),
  in its own words, with this repo's old habits and their plain forms.
- **The game's text:** the lab screens rewritten by hand (gene editor, microscope, incubator and jars, sequencer, tree of kin, lists,
  catalogue), with the names they share: gene kinds (WARMTH, not ITS WARMTH), culture stages (FULL, ADAPTING), pair names (PREDATOR
  AND PREY), the editor's findings ("DIES HERE, PARENT LIVES"). The other screens had "THE" dropped from titles, menus and hints and
  "ITS" taken out where a thing could be named. Strings the code or the harness compares were kept.
- **The working docs:** the CLAUDE.md files, the skills, the rules files and `STATUS.md`.
- **Open:** the reference notes (`docs/reference/`, 1.4 MB) and the code comments keep the old style until each is next edited; the
  log, the B and R files and the archive are history and stay as written.
- Proof and records: `docs/decisions/log/7-1.5.0.md` (this batch's entry); the battery against `y04` names every changed frame.
