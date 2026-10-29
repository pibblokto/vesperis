---
name: release
description: Cut a Vesperis release - the versions and release text, one commit, the six builds, the PR into main, the tag, the draft release and its uploads - under a random author, committer and tagger with no session link. Use when the user asks to prepare a release branch, push, or release a version.
---

# Cut a release

The steps and their traps are `packaging/RELEASE.md`: read it first. The guard rails:

- **Do exactly the asked part.** "Prepare branch" is fetch, fast-forward main, branch `release/vX.Y.Z`; "commit and push the
  branch, don't open a PR yet" stops there. A push, a PR, a tag or a release happens only on the user's word.
- **Identity:** every commit and tag under a fresh random identity, `GIT_AUTHOR_NAME`/`GIT_AUTHOR_EMAIL` and
  `GIT_COMMITTER_NAME`/`GIT_COMMITTER_EMAIL` (an 8-hex name, a 12-hex address at example.com: `openssl rand -hex`); the tag's
  tagger is the committer. The global git config holds the user's own address: never commit or tag without them. The gh
  account to push and release with is in the memory's release recipe; switch back after.
- **No session link** in a commit, tag, PR or release text; the guard refuses one. The Co-Authored-By line is fine.
- **Docker** (the Linux builds) starts the user's own containers: ask first. The Linux builds run last, one at a time.
- **The music** is credited by name only: never its permission, its licensing or how it was obtained.
- **Before the tag:** `regress` both ways and the unit pass on the release commit, and the six zips are built from it.
- **After:** `docs/STATUS.md`, `docs/ideas/MILESTONES.md`'s "Released as" line; the release's iteration file and its ideas
  file move to `docs/archive/ideas/`; the next release's entries start a new file in `docs/decisions/log/`.
