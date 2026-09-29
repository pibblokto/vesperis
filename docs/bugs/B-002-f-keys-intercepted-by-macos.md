---
id: B-002
title: F keys do nothing; the laptop's brightness/volume functions trigger instead
severity: S2
status: fixed (2026-09-26)
reported: 2026-09-26
reporter: user
---

## Where
State / screen: everywhere the game binds F keys (F1 help, F2 data, F5 save, F9 load, F10 scanlines, F11 fullscreen, F12 screenshot)
Star / body: any
EPOC: any
Save attached: no

## Steps
1. Press `F1` (or any other F key) on a Mac laptop keyboard.

## Expected
The bound game function (help screen, save, screenshot, ...).

## Actual
macOS performs the hardware function of the key (brightness, keyboard light, volume, Mission Control) and the game never receives the key press.

## Notes
Frequency: always on this machine.

## My notes (Claude)
**Cause.** Not a code defect in the input path: on Mac laptops the top row sends media/hardware functions unless the key is combined with `Fn` or "Use F1, F2, etc. keys as standard function keys" is enabled in System Settings > Keyboard. The window never sees F1/F2 (brightness), F5/F6 (keyboard light), F10-F12 (mute/volume), F3/F4 (Mission Control/Launchpad). Only the OS can change that, so the game must not rely on F keys for anything important.

**Fix.**
1. Add letter and control-key alternatives for every F-key function and show both in the help and on the title screen: help `H` (and `?`), data sheet `I`, save `Ctrl+S`, load `Ctrl+L`, scanlines `Ctrl+K`, fullscreen `Ctrl+F` and `Alt+Enter`, screenshot `P`. F keys stay as secondary bindings.
2. Detect the first frame in which no F key has ever been seen but `Fn`-less presses are likely (macOS build) and print a one-time status line: "F KEYS NEED FN ON MAC - USE H, I, P, CTRL+S".
3. Mention the macOS setting in `README.md`.
4. Full key rebinding is roadmap item M6-02; this fix does not wait for it.

**Size.** S. **Check.** Scripted flow uses the letter bindings; help page screenshot shows both.

**Fixed 2026-09-26.** Twins for every F key: help `H` or `?` (F1), data sheet `I` (F2), save `Ctrl+S` (F5), load `Ctrl+L` (F9, now also on the surface), scanlines `Ctrl+K` (F10, moved from the platform layer into `Game::scanlines`), fullscreen `Ctrl+F` or `Alt+Enter` (F11), screenshot `P` (F12). `Ctrl` chords do not walk, target or recall the capsule; `Alt+Enter` does not approach or land. On macOS the platform layer sets `Game::macHints`, and the first status line after the title is followed by a one-time reminder that the F keys need Fn. Help pages, title, README and the reference docs show the new bindings; the flow test uses `I`, `H` and `Ctrl+S`.
