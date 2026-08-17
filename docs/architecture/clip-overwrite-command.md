# Central clip overwrite command

## Purpose

NextStudio arrangement tracks use an incoming-wins rule: clips on the same
track may touch but must not overlap. Tracktion Engine permits overlaps, so all
NextStudio arrangement mutations are routed through `ClipEditing::applyOverwrite`
in `App/src/ClipOverwriteCommand.cpp`.

The command replaces the former strategy of duplicating clips, moving them past
the end of the edit, deleting destination material, and moving them back.

## Contract

- Clip ranges are half-open: `[start, end)`.
- Incoming placements win over clips present in the destination snapshot.
- Move and resize preserve the original clip object and item ID.
- Copy and state insertion create remapped item IDs.
- All move sources in one command are protected while destination victims are
  resolved. This permits block moves through their own source positions.
- Placements in one command may not overlap one another.
- Existing destination clips are removed, trimmed, or split.
- Split victim fragments are not added to the selection.
- Expected validation failures make no model changes; commit failures roll back
  the current undo transaction.

## Planning

Before changing the edit, the command validates every placement, captures copy
states and automation sections, groups destination masks by track, merges
adjacent masks, and snapshots the exact victim clips. Copy sources are not
protected, which means copying a clip exactly on itself replaces the original
with the copy.

Explicit source and destination removals support time-range editing. Multiple
masks for one victim are processed as a fragment set so a clip can retain more
than two disjoint pieces.

## Commit

The commit is guarded by:

- `Edit::UndoTransactionInhibitor`, preventing timer-created transaction
  boundaries;
- `TransportControl::ReallocationInhibitor`, delaying graph reconstruction;
- `ScopedSaveLock` in the `EditViewState` overload.

Victims are edited first without passing a `SelectionManager` to Tracktion.
Existing winners are then moved/resized directly and copies/inserts are created
at their final destinations. When `automationFollowsClip` is enabled, captured
automation is moved or copied to the mapped destination track. Automation and
final selection are applied once.
Every affected track is checked for overlaps before success is returned.

## Entry points

The command is used by:

- selected clip move/copy/duplicate;
- selected clip resize and time stretch;
- MIDI clip creation;
- audio-file insertion;
- time-range move/copy.

Arrangement recording is configured to Tracktion's replace-existing mode before
recording starts, preserving the same incoming-wins rule for engine-owned
recording finalisation.

## Tests

`App/tests/ClipOverwriteCommandTests.cpp` covers insertion splitting, identity-
preserving moves, copy-on-self, protected block moves, winner conflict
validation, multiple removal masks, and atomic undo/redo.
