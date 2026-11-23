# Rules: the harness and the proof: scenes and test worlds

Rules in force for the harness's scenes, pinned worlds and test worlds, one line per rule with its log entries (`log/`, by release).
A milestone that sets or changes a rule edits its line here; a replaced rule moves to `superseded.md` (`docs/CLAUDE.md`). The
harness itself is in `rules-harness.md`; regress, the battery and the bench are in `rules-harness-proof.md`.

## Scenes and test worlds
- A new scene appends a `SceneCode` (values fixed) and its finder in the chain, outside the trait range (`<= SCENE_STAINED_CRACK`) [S-04], [P-02..P-06], [E-03]
- Finders read the light as the view does (`effectiveSunAlt`), scanning the year (`findTime`) and, on a locked world, the longitude [G-04]
- A scene's site shows what the scene is named for (the light, the canopy): a placed site keeps its longitude, and a miss moves to the next body [G-04], [S-01]
- Every scene prints its site (body, sector, latitude, longitude, light); on a small moon the stand is within the 1-2 km horizon [G-04], [P-06]
- `forTypeBodies` and the scenes try every real world before a staged one (`stageElsewhere`) [P-05]
- Scans of every people's world read `KT_PEOPLE`; searches for the C-01 test worlds and the scene worlds keep C-01's kinds [P-04]
- Pinned sites (`PIN_*`, on Wyariothmai I) go stale as the ground moves: re-pin them on lit ground (`landforms sites search`) [G-04], [S-01], [W-03]
- The unit and the scenes use the civilisation test worlds Aieliaalas II, Leileashphail III and Eleinewai I (152 0 92, two peoples) [C-08], [C-12], [C-13]
- `AGE_WORLDS` pins the nearest people of each age on sector y 0 (modern: the first on Yeesheleemiola II-b); `peoples ages` says when one moves [A-01], [R-413]
- The history checks use two worlds with two peoples, Leanmya V-a (met) and Hiameshseine I-a (conquest); the dam check also reads Shioleesein I-a [R-413]

[G-04]: log/1-1.1.0.md
[S-01]: log/1-1.1.0.md
[S-04]: log/1-1.1.0.md
[C-08]: log/1-1.1.0.md
[C-12]: log/1-1.1.0.md
[C-13]: log/1-1.1.0.md
[W-03]: log/2-1.2.0.md
[A-01]: log/3-1.3.0.md
[R-413]: log/3-1.3.0.md
[P-02..P-06]: log/6-1.4.0.md
[P-04]: log/6-1.4.0.md
[P-05]: log/6-1.4.0.md
[P-06]: log/6-1.4.0.md
[E-03]: log/6-1.4.0.md
