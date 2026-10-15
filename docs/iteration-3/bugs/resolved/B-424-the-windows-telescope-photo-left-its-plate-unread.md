# B-424: the Windows telescope's photo left its plate unread

status: resolved 2026-10-15 (found by F-09's cross-builds on 2026-10-14; the first fix completed the next day)
severity: S3
area: the telescope's detail plate (`space/space_view.cpp`), the Windows builds

## Report

Found by F-09: the llvm-mingw builds of `packaging/build-win.sh` warned `shift count >= width of type` at `SpaceRenderer::plateFill`,
`long left = budget > 0 ? budget : (1L << 40);`. The budget 0 means "all of it": a photo through the telescope (`wantsScreenshot`)
reads the plate out whole (W-06), and the harness's `telePlateFinish` likewise. Since 1.2.0, on both Windows zips.

## Cause

`long` is 32 bits on Windows (LLP64), so `1L << 40` is undefined; clang folds the shift to poison and the select to its other arm, so
`left` was `budget` itself: 0 for a photo, and the plate's loop (`while (!p.done() && left > 0)`) read nothing. The photo showed the
plate as far as the eyepiece's frames had read it (8,000 samples a frame), not whole. The x86_64 assembly of the old line has no
2^40 at all (`movl %r9d, %esi`); macOS and Linux have a 64-bit `long` and were right.

The first fix (2026-10-14) made `left` a `long long` but kept the loop's `int count = (int)std::min<long>(nBlocks - p.next, left)`,
which converts `left` back to `long`: on Windows 2^40 becomes 0, so `count` was 0, `p.next` never moved and `left` never shrank, and a
photo would have spun in that loop for ever (a hang where 1.2.0 and 1.3.0 had only an unread plate; it never left the working tree).
Found on 2026-10-15 by compiling every source for Windows x86_64 and for the Mac with clang's `-Wshorten-64-to-32`: a narrowing that
only the Windows target reports is a 32-bit `long`, and that line was the only one.

## Fix

`long long left = budget > 0 ? budget : (1LL << 40);` and `std::min<long long>` in the loop. No other game code narrows a 64-bit
value on Windows (the scan: none for Windows or the Mac; the other `long`s are small counts and diagnostics). The Windows release
builds now compile with `-Wshorten-64-to-32` (`build-win.sh`), and `build-release.sh` prints each build's warning count.

## Verification (2026-10-15)

The Mac's `vesperis` and `vesperis_test` built with the fix are byte for byte F-08's (`long` and `long long` both 64 bits there); the
Linux build's `long` is 64 bits too. With the fix every game source compiles for Windows x86_64 without a narrowing warning; the first
fix's file gives `implicit conversion loses integer precision: 'long long' to 'const long'` at the loop. In the Windows x86_64
assembly of `plateFill` the first fix took `count` from 32 bits (`cmpl %r12d, %ebp; cmovgel %r12d, %ebp`: 2^40's low half is 0) and
then subtracted it from `left` (`subq %rax, %r12`), so the loop could not end; the fix compares 64 bits
(`cmpq %rbp, %r12; cmovlq %r12, %rbp`). No Windows machine runs the game here: the photo itself is unchecked on Windows (KI-411).
