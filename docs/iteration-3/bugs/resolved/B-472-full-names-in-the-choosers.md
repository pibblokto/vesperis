# B-472: full names in the choosers

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: game (game/lab_lists.cpp: `keptRow`; the gene editor's and the sequencer's choosers)

## Report

The user, 2026-11-22: "In genes modification machine cultures are now shown by their full name, just parents name instead "Star Virus"
instead off all the suffixes you can see in microscope". A strain kept in the collection, "STAR VIRUS E2" on the microscope's list,
showed as "STAR VIRUS" in the gene editor's chooser.

## Cause

`keptRow` makes a kept culture's row in a chooser. Asked for one species of a kept culture (the gene editor and the sequencer take one
at a time), it cleared the strain's number before it named it, so the row named the species. The choice itself kept the strain, so
the right line was edited or read under its parent's name.

## Fix

`keptRow` names the strain it was asked for: its own name, or its species' name and its number ("STAR VIRUS E2"), as the microscope
and the incubator's chooser do. The editor's and the sequencer's collection rows read as the microscope's list.

## Verification (2026-11-22)

- The unit's `genes` group, new check "a kept line by its own name in the editor's and the sequencer's choosers, as on the microscope":
  the rod written as the strain ROUND and kept is the row "ROUND" in the editor's chooser and "ROUND (SEQUENCED)" in the sequencer's.
  By the old code both rows were the rod's looks ("A ROD, 2.6 UM BY 0.5 UM").
