# Cutting a release

How a Vesperis release is made from the dev Mac: six zips (macOS arm64/x86_64 with `Vesperis.app`, Linux x86_64/arm64 built on
Ubuntu 20.04 in Docker, Windows x86_64/arm64 with llvm-mingw), raylib 5.5 built from source per target. Releases so far: 1.0.0
(2026-09-29) to 1.3.2 (2026-10-19), each through a pull request into `main`.

## The toolchains

- `sh packaging/toolchains.sh` once: raylib 5.5 and llvm-mingw 20260922 with pinned sha256 into `$VESPERIS_TC` (default
  `build/toolchains`, gitignored and outside the system's tmp cleaner), raylib static per target (`rl-<mac|win>-<arch>/libraylib.a`).
  The Linux raylibs are built in the container on first use (`rl-linux-<aarch64|x86_64>`), from images `vesperis-build:<arch>` that
  `linux.Dockerfile` makes when missing.
- `make clean` removes only the object directories (E-02): the toolchains stay.
- Docker Desktop must be running, and starting it also starts the user's own containers: ask first, and leave theirs alone. If it
  hangs (the daemon silent, `open -a Docker` only raising the window), killing `com.docker.virtualization` and `com.docker.backend`
  and opening it again brings it back in about 10 s; only on the user's go.

## The steps

1. **The release text first:** the version in `CMakeLists.txt` (`CPACK_PACKAGE_VERSION`) and `packaging/make_app.sh`
   (`CFBundleVersion`, and `CFBundleShortVersionString` as the whole version); the README's paragraph (the README ships in the
   zips); `docs/ideas/MILESTONES.md`'s "Released as" line; `docs/STATUS.md`.
2. **Proven:** `regress` with the workers and with `VESPERIS_THREADS=1`, and `unit`.
3. **One commit** with a fresh random identity as author and committer: `GIT_AUTHOR_NAME`/`GIT_AUTHOR_EMAIL` and
   `GIT_COMMITTER_NAME`/`GIT_COMMITTER_EMAIL` set to an 8-hex name and a 12-hex@example.com address (`openssl rand -hex`); the global
   git config carries a personal address, so never commit or tag without them. No session link in any message.
4. **Built from that commit, before merging:** `VESPERIS_TC=build/toolchains caffeinate -i sh packaging/build-release.sh <version>
   <out dir>` (about 8 minutes; from a checkout or worktree of the commit). It prints `<build>: exit N, W warnings` for each (macOS
   and Windows 0, Linux the 46 old GCC style warnings, 38 of them parallel.h's), runs the Linux builds last and one at a time
   (Docker's VM died beside them once), and makes no zip unless all six built. A failure only one compiler shows (a missing
   `<cstdlib>` for `atoi` broke Windows twice) is fixed on the branch first. An interrupted `docker run` leaves its container
   behind: `docker rm -f` it.
5. **Checked:** each zip's file list against the previous release's public zips
   (`https://github.com/pibblokto/vesperis/releases/download/v<old>/<zip>`); `otool -L` and minos 11.0 on the macOS binaries,
   `objdump -p` DLL names on the Windows ones, NEEDED and the newest GLIBC (2.29) on the Linux ones. `make_app.sh` leaves
   `dist/Vesperis.app` in the checkout.
6. **Merged:** `gh auth switch --user pibblokto` (git's credential helper is gh), push the branch, `gh pr create --base main
   --body-file <notes>`, `gh pr merge --merge`; check that the merge commit's tree equals the built commit's
   (`git rev-parse <sha>^{tree}`).
7. **Tagged:** an annotated `vX.Y.Z` on the merge commit, with the same random identity as tagger (`GIT_COMMITTER_*`); push it.
8. **Released as a draft first:** `gh api --method POST repos/pibblokto/vesperis/releases -f tag_name=vX.Y.Z -f target_commitish=main
   -f name="Vesperis X.Y.Z" -F body=@<notes> -F draft=true`; upload each zip with `gh api --method POST -H "Content-Type:
   application/zip" --input <zip> "https://uploads.github.com/repos/pibblokto/vesperis/releases/<id>/assets?name=<zip>"`; then
   `PATCH` it with `-F draft=false`, so it is never visible without its downloads. `cmp` the six public downloads against the built
   zips. A draft is visible only to the owner's account.
9. **After:** switch gh back to the account that was active; `git fetch origin main:main`; leave the remote branch in place.

To move a release (as 1.3.2 was, re-released with the music): delete the old release on GitHub, `git push origin
:refs/tags/vX.Y.Z`, `git tag -d vX.Y.Z`, then steps 3 to 8 from a new commit on the same branch and a new pull request.

## Traps

- Docker Desktop starts with `open -a Docker` (about a minute). The Linux images are `ubuntu:20.04`: `debian:bullseye` can no
  longer apt-get. GCC 9's flag is `-O3` (its -O2 does not vectorise, and its LTO is slower); an emulated amd64 container runs the
  harness 5-7x slower.
- GCC 9.4 can build a constexpr table unlike Clang without a word (B-430: 55 of the 60 kinds lost their draw rules, other
  worlds on Linux). After a change to how a table is built, dump it in the container and compare it with the Mac's
  byte for byte (`build/b430/kdump.cpp`: each row's texts and bytes, pointers zeroed); the regress shows it too.
- Narrowings show only on Windows (LLP64, a 32-bit `long`): compile the whole tree with `-Wshorten-64-to-32` and fix every one,
  not only the line that warned.
- `make_app.sh` leaves `dist/Vesperis.app` in the checkout: remove it. `build/perf-baselines/tools/f09/check_release.sh` compares
  zip listings against 1.3.0's names only: compare later releases' listings by hand.
- The music tracks are credited by name only: never their permission or licensing in docs, notes, commits or release text, and
  nothing on how they were obtained.
