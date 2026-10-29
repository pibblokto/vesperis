# Rules: process and release: the builds

The rules in force for how work is planned, proven, recorded, committed and released, a line each with the entries behind them
(`log/`, by release). A milestone that sets or changes one edits its line here; a replaced one moves to `superseded.md`
(`docs/CLAUDE.md`). This file holds the builds and the releases; the rest is `rules-process.md`.

## The platform and the names
- `raylib.h` only in `main_raylib.cpp` and `platform_raylib.cpp` (its `PI` and `KEY_*` clash): game code sees `src/platform.h`, the harness no raylib [table]
- `core/input.h` refuses to compile beside raylib.h; platform_raylib.cpp asserts raylib's key codes and the pad's arrays to input.h's numbers [E-05]
- game.h declares, a line each under `// ----` banners naming files; `game_types.h` and the `*_state.h` hold Game's types; its data never reorders [E-05]
- surface_view.h holds SurfaceView's data first, never reordered, then its functions under a banner a file, the harness's last [E-08]
- An engine header's declarations only the harness needs sit behind its `// ---- the harness's` banner, a comment group whole [E-07]
- `make check` (`tools/check.py`): raylib only in the platform files, the layering, no C++ line over 300, each file in its note [E-02], [E-12]
- src/galaxy reads src/peoples in `landmarks.cpp` and `planet_bodygen.cpp` alone (`make check`) [E-09]
- The names stay: binaries `vesperis`, `vesperis_test`, env `VESPERIS_TRACE`, `_SKIP`, `_NODRAIN`, `_THREADS`, CMake's `VESPERIS_*` [Release 1.0.0]

## The dev build
- The Makefile stays at `-O2` (the bench measures the code, not the flags); the release flags live in `packaging/`'s scripts [F-09]
- Flags come from make's command line only, `CPPFLAGS` apart from `CXXFLAGS` (Homebrew's LLVM exports one); `CXX` is /usr/bin/clang++ by default [E-02]
- The Makefile links raylib's static `libraylib.a`, as the release builds do: Homebrew's shared library hides GLFW's joystick calls [R-418]
- `make clean` removes only the source-derived objects and the binaries (build/toolchains, perf-baselines, prove stay); never an rm of build/ [E-02]
- `compile_flags.txt` gives clangd the build's flags (-xc++, C++17, -Isrc, the warnings, Homebrew's include): a flag change edits it too [E-02]
- A change to `.claude/hooks/guard.py` is proven by `python3 .claude/hooks/test_guard.py`, every case as expected [E-01], [E-02]

## Warnings and portability
- core/, galaxy/ and peoples/ build with warnings as errors: CMake's `VESPERIS_WERROR`, the Makefile's `WERROR` [M7-06], [E-09], [E-12]
- A new engine directory goes in the Makefile's `CORE_SRC`, CMake's two globs, the release scripts' `find`s and `tools/prove`'s objects [E-09]
- A count that can pass 2^31 is `long long` or `int64_t`, never `long` nor narrowed through one: Windows' `long` is 32 bits [B-424]
- Windows builds compile with `-Wshorten-64-to-32` (`build-win.sh`), the tree has none; `build-release.sh` prints each build's warnings [B-424], [F-09]
- A table a constexpr function builds sets its rows' arrays of non-zero defaults first, in order: GCC 9.4 cuts a range they share [B-430]

## Releases
- A release is everything done since the last, nothing held back, and the user calls it [Release 1.1.0..1.3.0]
- Its scope is read from `ideas/MILESTONES.md` and the iteration's file; at the release the rows not done go to the next version [Release 1.1.0..1.3.0]
- A release follows `packaging/RELEASE.md` (the `release` skill): one commit, regress both ways, the unit passing, the six zips from it [E-01], [E-04]
- The PR's merge keeps the built commit's tree (`git rev-parse <sha>^{tree}`), the tag on the merge, the release a draft until its zips are up [E-01]
- Version strings live only in `CMakeLists.txt` and `packaging/make_app.sh`; the game shows no version [Release 1.1.0]
- A release brings the README's paragraph and its keys up to date with what it adds (the README ships in the zips) [Release 1.3.0]
- After it: `MILESTONES.md`'s "Released as" line, the iteration's files to `archive/ideas/`, the next release's entries in a new `log/` file [E-04]
- The public repository is `pibblokto/vesperis`, without the old tree's history or `shots/` [Release 1.0.0]

## The release builds
- Release builds are `packaging/`'s scripts; `toolchains.sh` pins raylib 5.5 and llvm-mingw by SHA-256 (into `build/toolchains` or `VESPERIS_TC`) [F-09]
- Each target links a static raylib 5.5; macOS builds with the Mac's clang, Windows with llvm-mingw, Linux in Ubuntu 20.04 [Release 1.1.0..1.3.0], [F-09]
- `build-release.sh <version> <out dir>` makes no zip unless all six built; the Linux builds run last, one at a time [F-09]
- The Linux image keeps glibc 2.31 and GCC 9.4 (raylib X11 only, the C++ runtime linked in), so its zips start on older systems [F-09]
- Each binary needs only its system's libraries: macOS 11's frameworks, Windows' system DLLs (UCRT), glibc's core (no `libstdc++`) [F-09]
- Release flags: macOS and Windows `-O2 -flto=thin` (clang), Linux `-O3` (GCC 9); Windows takes the Mac's line and nothing more, untimed [F-09]
- A release flag is taken only if it gains alone, never slows and keeps the pictures: no `-march`, PGO, `-fno-plt`, `-no-pie`, `-fno-stack-protector` [F-09]
- A zip holds the binary, `README.md`, `LICENSE`, `vesperis_keys.example.txt` and `music/`'s tracked files, never a player's own (`tape.sh`) [F-09], [R-420]
- The macOS zips carry `Vesperis.app` beside the binary, the music in its `Contents/Resources` [F-09], [R-420]

[table]: log/0-1.0.0.md
[M7-06]: log/0-1.0.0.md
[Release 1.0.0]: log/0-1.0.0.md
[Release 1.1.0]: log/0-1.0.0.md
[Release 1.3.0]: log/0-1.0.0.md
[Release 1.1.0..1.3.0]: log/0-1.0.0.md
[F-09]: log/4-1.3.1.md
[B-424]: log/4-1.3.1.md
[R-418]: log/5-1.3.2.md
[R-420]: log/5-1.3.2.md
[E-01]: log/6-1.4.0.md
[E-02]: log/6-1.4.0.md
[E-04]: log/6-1.4.0.md
[E-05]: log/6-1.4.0.md
[E-07]: log/6-1.4.0.md
[E-08]: log/6-1.4.0.md
[E-09]: log/6-1.4.0.md
[E-12]: log/6-1.4.0.md
[B-430]: log/6-1.4.0.md
