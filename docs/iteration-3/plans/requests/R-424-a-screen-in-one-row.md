# R-424: a new screen in one row and a file of its own

status: in progress (planned 2026-10-26 as U-01 to U-06, `docs/ideas/PLAN-screens.md`; U-01 to U-05 done 2026-10-26)
milestone: 1.4.0

## Request

After E-13 (2026-10-26), the user asked whether the reorganisation made Vesperis easier to support and to add features to. The
answer was: easier, but a new game screen or device still touches about fourteen places in the game and the harness. A screen
is something the explorer opens and works, like the telescope, the landing map or the drone bay.

The user replied: "That's sad, I will have tons of those." Offered a table of screens before the first new one, they asked:
"Define those milestones, add them and prepare everything so I can write proceed and you will".

## Plan

`docs/ideas/PLAN-screens.md`: six milestones in 1.4.0, before its release. Nothing generated, drawn, played or saved changes.

* **U-01, the proof reaches every screen:** a tour opens each of the 29 states, pictures it, presses its keys and lists its
  controller menu. `input` joins the battery, and the `settings` item is mended (KI-426).
* **U-02, the screen table:** each state's update, picture and sound off its screen move into its row, and the frame loses its
  switch.
* **U-03, the ways in as rows:** the Esc menu, the guide menu, the flight computer, the settings and the cabin's devices
  become lists of rows with no counts kept in step.
* **U-04, the controller from the row:** a screen's commands, name and buttons come from its row; the general page reads traits.
* **U-05, a screen's life:** one way in (`openScreen`) and one way back (each row's `back`), with the return variables folded
  where the tour proves them alike. `guideReturn` too, now that B-429 fixed KI-427. A screen gets its own save lines and its reset.
* **U-06, the close:** the harness reads the table, the cookbook's recipe shrinks to five places in the code, and the
  game-screen and cabin-device tasks are measured again.

## Done

