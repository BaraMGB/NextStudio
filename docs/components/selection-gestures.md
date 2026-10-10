# Lasso and TimeRange selection

- Type: reference
- Audience: contributors
- Scope: rectangle selection, musical anchors and independent range gestures

## Responsibilities

`SelectionGestures.h` defines two independent value types:

- `LassoGesture` stores an anchor and endpoint in source/content coordinates.
  It supplies normalized bounds and projection into the current view. It knows
  nothing about selected objects, time-range operations or snapping.
- `TimeRangeGesture` stores a resolved musical interval and vertical lane bounds.
  Its caller owns snapping and the selected lanes. It does not invoke lasso hit
  testing or create a rectangle component.

`LassoSelectionComponent` wraps the first value type as a lightweight foreground
drawer, not a hidden JUCE child. Its projection and clipping bounds belong to the
owning editor; the owner also repaints after updates. Activity is explicit gesture
state. The former combined `LassoSelectionTool` no longer exists.

## Anchors and view refresh

The horizontal anchor is an unsnapped project beat, not a mouse-down pixel or
clip-relative note beat. MIDI vertical anchors are continuous pitch coordinates;
arrangement vertical anchors are normalized positions within the visible track
layout. The source supplies the mapping, so drawing and hit testing use the same
current view rectangle.

On pan, zoom, vertical scroll or viewport resize, the owner reprojects the saved
anchor and recomputes the endpoint from the actual stationary pointer using
`MouseGestureInput::forContext()`. Modifier-only refresh retains the last raw
pointer position. Changing the source layout/track set discards active gestures
before rebuilding/destroying their children; it does not reinterpret the anchor
as a different track after reordering.

TimeRange has its own anchors and input capture. Arrangement range endpoints use
the existing [magnetic snapping contract](timeline-snapping.md), with the corrected
downward creation anchor and no release-time re-quantization. MIDI Range preserves
its unsnapped time/pitch selection semantics and has a translucent preview.

## Sources and selection policy

`MidiViewport` hit-tests painted note rectangles (including clip clipping).
`SongEditorView` hit-tests clip rectangles. `AutomationLaneComponent` returns
curve-child identities for marker centres inside the projected rectangle, with
inclusive centre boundaries. Finding hits never mutates the model or selection.

`combineLassoSelection()` combines each current hit set with the original gesture
snapshot, not with the preceding frame's result:

- no modifier: replace;
- Shift: union/add;
- Ctrl or Command: symmetric difference/toggle;
- Shift takes precedence; Alt is not a selection modifier.

Modifiers are evaluated during the gesture. Arrangement clip/automation lanes
explicitly forward JUCE modifier dispatch to `SongEditorView`, so a standing
pointer does not require another drag event. Shrinking the rectangle or releasing
a modifier recomputes the result from the same original set, without sticky hits
or repeated toggling. This policy belongs to object lasso; Range is an independent
interval selection, and modifiers such as arrangement Shift snap bypass retain
that operation's meaning.

## Identity, model and lifetime

`MidiSelectionSnapshot.h` captures note ValueTree identities by scanning live
clips. Resolving a snapshot scans current notes: deleted/recreated notes with the
same musical properties are not the old identity. The resolved set is applied
through `SelectedMidiEvents::setSelected()` as one batch. Unchanged membership is
not repeatedly broadcast; empty MIDI membership retains the existing track
selection fallback. Clip-source changes discard the active gesture before the
viewport replaces its selection object.

Arrangement hit-policy snapshots retain track/clip states and automation point
child states, not mutable point indices. `applyObjectSelection()` resolves live
sources and applies a complete `SelectableList`.

Cancellation additionally uses `SharedSelectionSnapshot` in both editors. It
captures every shared-manager object through Tracktion safe references, plus
MIDI event membership and guarded clip sources. Restoration filters deleted or
detached objects and resolves event identities against live clips; a canceled
Song gesture can restore MIDI selection, and vice versa. A temporary Tracktion
manager hydrates event owners through the batch API without clearing another
owner; its destructor removes listeners without clearing event membership.
Only the shared application manager receives the final restored object list.

Automation lanes and snapshots share reference-counted point proxies; each proxy
retains its parameter to keep its curve reference valid. Cleanup does not remove
unselected proxies pinned by an active snapshot. Removed/unselected, unpinned
proxies are cleaned up, and restoration validates point identity/current index.
Restoring a previous TimeRange validates track/parameter membership against the
current edit; retained references prevent recycled original sources.

`SelectionIdentity.h` provides per-update lookup indices for policy combination,
live-note resolution, automation indices and proxy reuse. During a read-only pass,
the address of a tree's first property storage is a shared identity hash anchor;
identical-content leaves do not collide by content. Propertyless utility trees
use a type bucket. `ValueTree::operator==` remains authoritative in both cases.
Indices are rebuilt for each update and never survive property/model mutations,
which can relocate storage. No pointer key is persisted or dereferenced. These
replace application-level nested linear searches; Tracktion's own batch-selection
internals are unchanged.

Selection changes and rectangle/range previews are transient UI operations. They
create no notes, clips, automation points, musical ValueTree flags, edit undo
transactions or persisted gesture settings. Existing note/clip/automation edit
commands retain their own mutation and undo contracts.

## Input and cancellation

Explicit Lasso consumes left-button gestures and returns to Pointer after a
completed selection. MIDI defers that strategy replacement until the current
strategy call has returned. Pointer empty-space selection stays on Pointer and
retains the original down anchor; the drag threshold controls display, not the
anchor or selection snapshot. Double-click insertion ends that pending gesture
without replacing its tool.

Escape and tool changes end either selection gesture and restore valid original
membership. MIDI and arrangement input owners suppress late drag/release events
so they cannot reapply a canceled rectangle or clear the restored selection.
Gesture ownership includes the original mouse-down time and mouse-source index,
not a global consume-next-release flag. MIDI retains press ownership separately
from its hover/replay cache, checks it before drag/release dispatch and clears it
on completion/cancellation; hover cannot revive a canceled press. A new
press ends the previous pending selection before starting a clip/range edit;
its release cannot be swallowed by that old selection's cancellation. Normal
`endDrag()` clears edit preview/input state only: it neither restores a selection
snapshot nor commits an already finished range a second time.
Track/view teardown discards state without restoration into a disappearing
source. Destructors do not invoke restoration callbacks.

An arrangement lasso never implicitly creates a TimeRange on release. Arrangement
Range still returns to Pointer; MIDI Range remains active. Range snapshots and
lasso snapshots are independent, as are their begin/update/end APIs.

## Source and validation

- `App/include/SelectionGestures.h`
- `App/include/LassoSelectionComponent.h`, `App/src/LassoSelectionComponent.cpp`
- `App/include/MidiSelectionSnapshot.h`, `App/src/MidiViewport.cpp`
- `App/include/SharedSelectionSnapshot.h`, `App/include/SelectableAutomationPoint.h`
- `App/include/SelectionIdentity.h`
- `App/src/LassoTool.cpp`, `App/src/RangeTool.cpp`, `App/src/PointerTool.cpp`
- `App/src/SongEditorView.cpp`, `App/src/TrackLaneComponent.cpp`, `App/src/AutomationLaneComponent.cpp`
- `App/include/MouseGestureInput.h` — input ownership and gesture-scoped cancellation
- `App/tests/SelectionGestureTests.cpp`
- `tools/native-selection-regression.py`, `tools/native-arrangement-drag-regression.py`

[Testing](../development/testing.md#selection-gesture-validation) owns repeatable
console/native procedures and coverage boundaries. The velocity lane is not yet
a lasso source; [#84](https://github.com/BaraMGB/NextStudio/issues/84) follows this
foundation after maintainer testing. The [approved plan](../plans/lasso-time-range-separation.md)
remains active until both implementation stages are accepted.
