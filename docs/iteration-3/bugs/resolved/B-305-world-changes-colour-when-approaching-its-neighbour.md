---
id: B-305
title: A ringed world next to a felisian world changes colour depending on which one you approach
severity: S2
status: resolved 2026-09-27 (O0-02)
reported: 2026-09-27 (screenshots: TREYSTAL II brown while approaching II-A, pale yellow once in orbit round II-A)
---

## Analysis (Claude)
Since N1-02 the globe of the largest body on screen used banks 2 (rock) and 6 (water) and the second largest 3 and 7, but the family banks (11 forest, 13 sand, 14 snow, 15 grass) were shared by both and tinted by whichever body had the family first, the larger first. When the felisian moon became the larger disc, the shared sand and snow banks took its colours, and the ringed world's dust and ice texels (the sand and snow families) were drawn in the felisian's tan and white. Sixteen banks were all taken (stars 0/4/5, sun 1, bodies 2/3/6/7, families 11/13/14/15, companion 12, cabin 8/9/10), so no fix inside the format could give the second body its own colours.

## Fix
The pixel keeps 16 bits but the bank field is five bits: 32 banks of 2048 intensities (`BANKS`, `BANK_SHIFT` 11, `INTEN_MASK` 2047, `BANK_MASK`, `INTEN_PER_SHADE` 32). Everything that packs pixels goes through the constants; the mush's bank mask, the RGB conversion, the ordered dither, the CCTV noise and the vision grain were adjusted to the new units. Body B owns banks 16 (forest), 17 (sand), 18 (snow), 19 (grass); `globeBank` and `setupPalette` fill them from B's own palette; the belt rocks own bank 20 (KI-303 closed). The surface's bank table is unchanged.

## Verification
`unit`: the bank constants and `globeBank(3, sand) == 17`; `space` and `flow` frames; regress re-blessed (the intensity units changed every frame hash, the pictures did not); consistency 0 of 13 over the threshold as before.
