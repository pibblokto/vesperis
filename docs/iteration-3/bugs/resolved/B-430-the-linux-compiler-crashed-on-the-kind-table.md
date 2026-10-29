# B-430: the Linux compiler crashed on the kind table and lost its rules

status: resolved 2026-10-29 (found 2026-10-25 by E-10 as KI-423; fixed and proven 2026-10-27 on the user's word)
severity: S2
area: the kind table (`galaxy/kinds.cpp`, `buildKinds` and its `static_assert`), the Linux release builds (GCC 9.4)

## Report

Found by E-10 (2026-10-25) in the arm64 container and offered as KI-423: GCC 9.4, the compiler of the Linux zips (Ubuntu 20.04,
`packaging/build-linux.sh`), stops on kinds.cpp with an internal compiler error while it evaluates
`static_assert(kindsOk(BUILT_KINDS))`, P-01's check of the kind table. So 1.4.0's Linux builds fail, and with them the release,
which makes no zip unless all six build. E-10 and the U series took the line out of the container's copies to compile the rest.
The user: "lets fix KI-423, KI-432, KI-433 and KI-434 one by one starting with 423", and for the container: "Yes, use Docker".

## Cause

Two faults of GCC 9.4's constant evaluation, both in the one table.

- **The rules lost.** `buildKinds` value-initializes its 60 rows at once (`std::array<KindDef, PT_COUNT> K{}`). Each row's `rules`
  (`WeightOp rules[WR_COUNT]`, default `{1, 0}`: the draw's weights) then start as one range of elements every row shares.
  `rule(k, r, ...)` sets a rule past a row's first, and GCC 9.4 cuts that shared range in place, so the rows past the cut lose
  their rules. The table it emitted held 0 where Clang's holds 1: 55 of the 60 kinds differed (rows 5 to 59), the traits not.
  Those rules weight the draw (pulsars, black holes, giants, moons, small bodies), so a Linux build would have given some stars
  other planets than the Mac and Windows builds give. `kindsOk` reads a made kind's rules (all `{1, 0}`), meets a lost
  element in place, and GCC crashes on it (`verify_ctor_sanity`): the crash KI-423 saw. Dropping the check would have shipped
  the other worlds without a word.
- **The copies.** GCC 9.4 copies a constexpr variable whole at each read of it: `kindsOk(BUILT_KINDS)` took 6.6 GB and 6 s, and
  the out-of-memory killer took it in a parallel build. A table built for the check takes 101 MB.

Reproduced in 20 lines (`build/b430/repro/repro.cpp`): rows with a rule set in the middle of their range and at its start. Clang
prints `1 1 1 1` for a row it did not touch, GCC 9.4 `1 1 0 0`, and GCC 9.4 with every rule set first `1 1 1 1`.

## Fix

In `src/galaxy/kinds.cpp`, two lines and their comments:
1. At the top of `buildKinds`, every row's rules are set in order before any kind sets its own
   (`for (KindDef& k : K) for (WeightOp& w : k.rules) w = WeightOp{};`), so no range is shared when a row sets one.
2. The check reads a table built for it: `static_assert(kindsOk(buildKinds()), ...)`, not `BUILT_KINDS`.

Nothing generated moved on the Mac (Clang emits the same object), so `GEN_VERSION` stays 16 and nothing is blessed: the fix makes
the Linux builds draw what the Mac and Windows builds draw. The rule (`rules-process-build.md`): a table a constexpr function
builds sets its rows' arrays of non-zero defaults first, in order.

## Verification (2026-10-27, the sanity check 2026-10-29)

- **The Mac (Clang):** `tools/prove objects u06`: 140 objects the same, 0 differ; `kinds.o` is byte-identical, so the Mac and
  Windows builds cannot move. Clang compiles kinds.cpp in 0.24 s and 93 MB, as before.
- **GCC 9.4** (the arm64 container, `build/b430/linux/host3.sh` and `inner3.sh`, `check3.log`): all 182 units compile, the harness
  and the U series' tables (the states, the menus' rows, `padMenu`, the rows' columns) included, with the 97 warnings of before;
  against u06 with the check taken out only `kinds.o` differs. The kind and trait tables equal Clang's byte for byte
  (`build/b430/kdump.cpp`: each row's texts, then its bytes with the pointers zeroed).
- **Linux's regress** against the Mac's baseline: 21 of 24 the same (7 of 24 before the fix), with the workers and with one
  thread. The three left (`system_1`, `_2`, `_4`) are moons' orbits and periods 1 to 3 ulps apart (`build/b430/sdump.cpp`):
  GCC fusing multiply-adds after inlining (E-10's Linux last bits), not other worlds. Every map, site and scene is the same.
- **The release's own scripts** (`packaging/build-linux.sh`) build both Linux binaries: arm64 in 30 s and amd64 in 342 s
  (emulated), exit 0, 46 compiler warnings each (all old: 38 are parallel.h's misleading indentation).
- **The check still refuses a broken table** (2026-10-29, copies in `build/b430/sanity/`): a short name too long (`MOLTEN WORLD OF
  FIRE`) and a rule set in the last made kind each fail the `static_assert` under Clang and under GCC 9.4; the unbroken copy
  passes both.

Stays open: nothing of this bug. Offered: the tables' dump as a harness mode, run in the container before a release.
