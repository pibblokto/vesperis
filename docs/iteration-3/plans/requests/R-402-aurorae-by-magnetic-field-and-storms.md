# R-402: aurorae thinner, patchy, coloured, and not every night

status: done 2026-10-01
milestone: release

## Request

After B-403 ("it looks WAY better"): make the aurora less dense and of varying density, of varying colour, and not guaranteed every night even where the conditions hold (Earth does not have one every night); it should depend on the magnetic field's strength as in the real world, so that some worlds have aurorae reliably all the time, some often but weak or strong, some rarely.

## Done

* **A magnetic field per world** (`magneticField`, 0..1): hashed from the body's seed and biased by its spin and size (a big, fast-spinning world has the dynamo; a locked world has little), classed none / weak / moderate / strong (`MAGNETIC_CLASS_NAMES`). A derived property: no system or surface changes, saves untouched. The data sheet shows it (`MAGNETIC    STRONG   AURORAE ON MOST NIGHTS`, `AURORAE IN STORMS`, `A FAINT AURORA IN A GREAT STORM`), the description says "its poles crowned with aurorae" of a strong field under a clear sky.
* **The star's weather** (`auroralStorm`, 0..1): a noise over three days with substorms over four hours, per star; about a tenth of the nights are storms over 0.3, a few in a hundred great ones. The HUD reads `AURORA STORM` over 0.7.
* **The night's potential** (`auroraPotentialAt`): the star's activity by class (blue giant 1, pulsar 0.9, orange 0.55, yellow 0.45, red giant and white dwarf 0.2) times the steady glow a strong field keeps (`0.6 smoothstep(0.5, 1, field)`) plus what the storm drives (`storm^0.7 smoothstep(0.03, 0.4, field) (0.7 + 0.3 field)`). So a strong field round an active star has an aurora on most nights (83% in `unit`), a moderate one in storms, a weak one on 2% of nights and faintly; the storm pushes the latitude ramp and the oval equatorward (the ramp starts at 52 - 10 storm degrees, the oval at `59 + 8 field +- 3` minus 6 storm).
* **The curtains**: half as thick as B-403's (5, 4 and 3.5 km half-widths, a dense core in two thinner envelopes), the outer sheets brought up by the storm, patched along their length by a slow noise (down to a fifth), the tone knee `44 (1 - exp(-4 strength light))`.
* **Two colours per world**: bank 11 the lower curtain's, bank 21 the tops' (the profile's high part, H 70 km, its share of a pixel's light choosing the bank), the tops faint on a quiet night and taking over the heights in a storm (`topGain`, and more on some worlds). By the air and a hash: oxygen airs green (or teal, lime) under red (or violet, pink); thin air green or teal under violet; a desert's lime or green under pink; a tectonic world's blue under magenta; an acid sky gold under orange.
* Scenes: the aurora scenes choose the night of the next thirty with the strongest potential (over 0.45); `thinatmo_aurora` stands at 58 degrees (the oval ahead, a low arc), `thinatmo_aurora_zenith` under the oval, `aurora_moon` a degree equatorward of it with a body 35-65 degrees up (a trial frame counts curtains round it). `unit` checks the field classes, the storm share and the lit-night shares.
