# MIDI Input Routing and Exclusive Focus

## Purpose

NextStudio has two independent MIDI-input routing modes:

1. **automatic track focus**, which always routes the virtual PC-keyboard input and can additionally route the configured default input to the selected MIDI track; and
2. **manual persistent assignments**, configured in a track header's **MIDI Input** context menu.

The distinction is intentional. Selecting another track may move only the automatic route. It must never remove or move an input that the user assigned manually.

## User-visible contract

The virtual `virtualMidiIn` input used by the PC keyboard always follows exactly one selected MIDI track, independently of **Exclusive MIDI Focus** and the configured default device.

When **Exclusive MIDI Focus** is enabled, the configured default MIDI input follows that same track. In both setting states:

- selecting another MIDI track moves all applicable automatic routes;
- selecting a MIDI clip keeps focus on that clip's track when no track is selected directly;
- selecting neither a MIDI track nor a MIDI clip removes the automatic routes;
- manually checked MIDI inputs remain routed to their tracks regardless of selection;
- a track may have any number of manual MIDI inputs;
- automatic routes do not appear checked in the track context menu;
- manually checking the default device pins it permanently to that track.

When **Exclusive MIDI Focus** is disabled, only the default-device automatic route is removed immediately. The virtual PC-keyboard input continues following the selected MIDI track. Manual assignments remain unchanged.

When another enabled MIDI input is manually assigned to the focused track, the automatic default-device route yields to that assignment. This prevents an aggregate default such as **All MIDI Ins** from delivering the same physical input a second time. The virtual PC-keyboard route does not yield.

## Source map

| Responsibility | Files |
|---|---|
| Routing model, markers, reconciliation and migration | `App/include/MidiInputRouting.h`, `App/src/MidiInputRouting.cpp` |
| Selection, default-device and settings event integration | `App/include/Utilities.h`, `App/src/Utilities.cpp`, `App/src/MainComponent.cpp` |
| Persistent input checkmarks and user interaction | `App/src/TrackHeadComponent.cpp` |
| Default-input and focus settings UI | `App/include/AudioMidiSettings.h`, `App/src/AudioMidiSettings.cpp` |
| Application-wide focus setting | `App/include/ApplicationViewState.h` |
| Focused regression tests | `App/tests/MidiInputRoutingTests.cpp`, `App/CMakeLists.txt` |
| Underlying input destinations | `modules/tracktion_engine/modules/tracktion_engine/playback/devices/tracktion_InputDevice.h`, `tracktion_InputDevice.cpp` |

## Tracktion routing model

Tracktion owns one engine-level `InputDevice` for each input. Every open `Edit` has an `InputDeviceInstance` for each available device. An instance contains zero or more `INPUTDEVICEDESTINATION` children, each identifying a target track through `targetID`.

```text
InputDevice (engine scope)
└── InputDeviceInstance (edit scope)
    ├── INPUTDEVICEDESTINATION targetID=Track A
    └── INPUTDEVICEDESTINATION targetID=Track B
```

`InputDeviceInstance::setTarget(targetID, false, ...)` permits one device to target multiple tracks. This is required for persistent manual assignments.

Tracktion does not natively distinguish why a destination exists. Earlier NextStudio code therefore treated every destination of the default device as focus-owned and rewrote all of them on selection changes. That destroyed manual assignments and caused checkmarks to reappear or move unexpectedly.

## Destination ownership marker

NextStudio now marks only automatically created destinations with this property:

```text
nextStudioAutomaticMidiFocus = true
```

A destination without this property is manual.

| Destination state | Meaning | Shown checked in MIDI Input menu | May focus reconciliation move/remove it |
|---|---|---:|---:|
| Property absent/false | Manual persistent assignment | Yes | No |
| `nextStudioAutomaticMidiFocus=true` | Automatic focus assignment | No | Yes |

The marker is attached to the Tracktion `INPUTDEVICEDESTINATION` state because the automatic/manual ownership must survive device-list and playback-context rebuilds. A rebuilt `InputDeviceInstance` reads the same destination state and therefore retains the ownership classification.

## Core routing API

`MidiInputRouting` is independent of GUI components and exposes four operations.

### `isManualTarget`

Used by `TrackHeaderComponent` when constructing the context menu. A device is checked only when the target exists and does not carry the automatic marker.

### `setManualTarget`

Handles a context-menu toggle:

- enabling a missing target creates a normal manual destination;
- enabling an automatic target removes its marker, promoting it to a manual destination without rebuilding the graph;
- disabling removes only a manual destination;
- disabling never removes an automatic destination.

The user's manual operation and its immediately required focus reconciliation use the same edit `UndoManager` transaction. Manual routing and the resulting automatic-route replacement are therefore undone and redone atomically.

### `reconcileAutomaticFocus`

Receives:

- the current edit;
- the devices controlled by focus;
- zero or one focused track ID; and
- the focus devices whose automatic route must yield to another enabled manual input.

It scans every current input instance and computes the desired automatic destination:

```text
instance is a current focus device
AND focused track ID is valid
AND the same device is not already manually pinned to that track
AND the instance is not configured to yield to another enabled manual input
→ desired automatic destination = focused track

otherwise
→ no desired automatic destination
```

The reconciliation then:

1. removes marked destinations that are no longer desired;
2. preserves every unmarked destination;
3. creates and marks a missing desired destination;
4. leaves an already correct route untouched.

Selection-, setting-, and device-driven automatic changes pass `nullptr` instead of the edit's `UndoManager`. Merely selecting tracks does not add routing actions to the user's undo history. Reconciliation caused directly by a manual menu toggle receives that toggle's `UndoManager`, keeping removal and replacement destinations in one atomic transaction.

### `clearAutomaticFocus`

This is reconciliation with no focus devices and no target. It removes every marked destination while preserving all manual destinations.

## Focus-device set

The internal `getAutomaticMidiFocusDevices()` helper in `App/src/Utilities.cpp` always adds the `virtualMidiIn` device used by `ComputerMidiKeyboardController`. It adds `DeviceManager::getDefaultMidiInDevice()` only while **Exclusive MIDI Focus** is enabled. The default device is also passed as a yielding focus device, so a manual input assignment on the focused track suppresses only its automatic route.

`addIfNotAlreadyThere` prevents duplication when the virtual computer-keyboard input is itself configured as the default.

The virtual device lookup first searches existing edit instances. It requests asynchronous creation only when the device is absent. `MainComponent::bindComputerMidiKeyboard()` already retries while the DeviceManager rebuilds the device list.

## Selection flow

`MainComponent` listens to the shared `SelectionManager`.

```text
Track or clip selection changes
→ MainComponent::changeListenerCallback
→ EngineHelpers::updateMidiInputFocusToSelection
→ find selected MIDI tracks
→ otherwise use the track of the last selected MIDI clip
→ choose one focused MIDI track
→ always include virtualMidiIn in the focus-device set
→ include the default device only when Exclusive MIDI Focus is enabled
→ MidiInputRouting::reconcileAutomaticFocus
→ restart playback only if a route actually changed
```

Selected `Track` objects marked with `IDs::isMidiTrack` take precedence. Because a normal clip click replaces the track selection, selected MIDI clips provide a fallback focus track. If neither a MIDI track nor a MIDI clip is selected, automatic destinations are cleared.

For a multi-track selection, the last MIDI track returned by `SelectionManager::getItemsOfType<Track>()` becomes the single focus target. The default input is never automatically routed to the whole selection.

Right-clicking a header only opens its context menu; it does not change the selection or automatic focus target.

## Manual context-menu flow

The **MIDI Input** submenu lists physical MIDI input instances. Each item is a checkbox because several devices may be assigned simultaneously.

```text
Open MIDI Input submenu
→ isManualTarget(instance, trackID)
→ show checkmark only for persistent manual destination

Toggle item
→ begin one MIDI-routing undo transaction
→ setManualTarget(instance, trackID, !currentlyManual)
→ reconcile automatic focus with the same UndoManager
→ restart playback once if effective routing changed
```

Reconciliation after the manual operation handles the important default-device cases:

### Pinning the default device

If the selected track currently has an automatic default route, checking that device removes the automatic marker. The route stays active but is now permanent. Selecting another track creates a new automatic default route there while preserving the pinned route.

### Unpinning the default device on the focused track

Unchecking removes the manual destination. Immediate reconciliation recreates it as an automatic hidden destination because the track is still focused. Both operations belong to the same undo transaction, so Undo restores exactly one manual destination and Redo restores exactly one automatic destination. The menu remains unchecked, but the default input continues to play the focused track.

### Additional non-default devices

Checking or unchecking a non-default device changes its persistent assignment. On the focused track, adding such an assignment also removes the yielding automatic default route; removing the last competing manual assignment restores that route. Later focus changes never move or remove the manual assignment.

## Default-device changes and hot-plug events

`MainComponent` also listens to `DeviceManager` changes. This covers default-input changes and MIDI device-list rebuilds.

```text
Settings selects another default input
→ DeviceManager stores the device ID and rebuilds MIDI devices
→ DeviceManager change notification
→ MainComponent reconciles focus
→ marked route of old default is removed
→ current default receives the focused-track route
```

Because reconciliation removes marked destinations from devices outside the current focus-device set, changing the default cannot leave old automatic routes behind. Manual routes of the former default remain valid.

A general MIDI hot-plug notification uses the same idempotent path. No graph restart occurs when the desired routing is already present.

## Focus-setting transitions

`ExclusiveMidiFocusEnabled` is stored application-wide under `ApplicationViewState/Behavior`.

`MainComponent::valueTreePropertyChanged()` applies transitions immediately:

| Transition | Action |
|---|---|
| Off → On | Add/reconcile the default-device route; preserve/reconcile the always-focused virtual input |
| On → Off | Remove the default-device automatic route; preserve/reconcile the always-focused virtual input |

This avoids the earlier behavior where toggling the setting had no effect until a later selection change. The setting never disables focus routing for the virtual PC keyboard.

## Playback graph updates

Routing operations report separate `routingChanged` and `metadataChanged` flags.

- Adding or removing a destination changes effective routing and requires a playback restart.
- Promoting an automatic destination to manual changes only ownership metadata; audio/MIDI graph topology is unchanged.
- Idempotent reconciliation performs no restart.

`TrackHeaderComponent` combines the manual-operation result with the focus-reconciliation result and performs at most one restart.

## Error handling and recording

Tracktion rejects destination changes while an input is recording.

- Context-menu failures are shown through `UIBehaviour::showWarningMessage()`.
- Automatic reconciliation failures are written to the engine log.
- A failed removal or addition is not reported as `routingChanged`, preventing a pointless graph restart.

The existing route remains authoritative after a failed operation. A later selection, device or settings event retries reconciliation.

## Project migration

Projects saved by older versions contain destinations without ownership markers, so old automatic default routes are indistinguishable from manual routes.

The edit root uses:

```text
nextStudioMidiInputRoutingVersion = 1
```

On first initialization of an older project:

1. routes belonging to the currently controlled default/virtual focus devices are cleared;
2. routes belonging to all other devices are preserved as manual assignments;
3. the routing version is written;
4. any marked automatic routes are cleared because no track is initially focused.

This one-time reset is necessary to remove accumulated legacy default routes. A legacy manual assignment of the current default device is also reset because old project data contains no information that could distinguish it from an automatic assignment. It can be pinned again from the context menu. Later project loads preserve the new distinction.

## Persistence boundaries

| State | Scope | Undoable | Persistence behavior |
|---|---|---:|---|
| Default MIDI device ID | Engine/application settings | No | Global across projects |
| Exclusive MIDI Focus enabled | Application settings | No | Global across projects |
| Manual destination | Edit Tracktion state | Yes | Saved with project |
| Automatic destination and marker | Edit Tracktion state | No | May be serialized, but is normalized/cleared during edit initialization |
| MIDI routing schema version | Edit root state | No | Saved with project |

## Invariants

The implementation must preserve these invariants:

1. At most one automatic target exists per focus-controlled device.
2. The virtual PC-keyboard input is always focus-controlled; the default device joins it only while Exclusive MIDI Focus is enabled.
3. All currently focus-controlled devices use the same focused track when available.
4. No automatic destination is shown as a manual menu checkmark.
5. Manual destinations are never removed by focus reconciliation.
6. A device may have manual destinations on several tracks.
7. A track may have manual destinations from several devices.
8. A manually pinned default device does not need a duplicate automatic destination on the same track.
9. The automatic default route yields while another enabled input is manually assigned to the focused track; the virtual keyboard route remains active.
10. Changing the default or disabling default focus removes stale marked default-device routes without removing the virtual route.
11. Selection-, setting-, and device-driven automatic changes do not enter the edit undo history.
12. Automatic route changes caused by a manual toggle share its undo transaction and never create duplicate destinations on Undo/Redo.
13. Playback restarts only when destination topology changes.

## Regression tests

`App/tests/MidiInputRoutingTests.cpp` verifies:

- directly selected MIDI tracks take precedence and selected MIDI clips provide the fallback focus target;
- automatic focus moves between tracks;
- automatic reconciliation is idempotent, enables monitoring only when routing is created, and stays outside Undo for focus-only changes;
- manual add/remove operations are idempotent;
- manual routes survive focus changes;
- automatic routes are not reported as manual and cannot be removed through the manual-disable path;
- pinning the default promotes an automatic route without topology changes;
- a pinned default remains active when focus moves elsewhere;
- clearing focus removes only automatic routes;
- changing the focus device removes the former device's automatic route;
- the yielding default route is suppressed by another enabled manual input and restored after its removal;
- disabled manual inputs do not suppress the default route;
- non-yielding focus devices, including the virtual keyboard, remain active beside manual inputs;
- disabling default focus removes the default route while the always-focused virtual PC-keyboard route moves to the newly selected track;
- legacy migration clears old routes only for focus-controlled devices;
- migration with and without focus devices runs once and preserves unrelated routes;
- automatic reconciliation does not create undo actions for focus-only changes;
- unpinning a focused default remains a single destination across Undo/Redo;
- a manual assignment and the yielding default route undo and redo atomically.

The target is registered as `MidiInputRouting` in CTest.

```bash
BUILD_JOBS=12 ./build.sh rd
ctest --test-dir autobuild/RelWithDebInfo --output-on-failure
```

## Interaction with `All MIDI Ins`

`All MIDI Ins` is a Tracktion virtual aggregate that receives messages from every physical MIDI input. When it is the default device, its automatic focus route yields as soon as another enabled input is manually assigned to the focused track. This prevents the common duplicate-delivery path in which the physical device reaches the track both directly and through `All MIDI Ins`.

Explicit manual assignments remain authoritative and are never removed automatically.

## Related documents

- [State and Event Model](state-and-events.md)
- [Architecture Overview](overview.md)
- [Side Browser](../ui/side-browser.md)
- [Song Editor](../ui/song-editor.md)
- [Computer MIDI Keyboard Controller](../changes/computer-midi-keyboard-controller.md)
- [Piano Roll MIDI Key Lighting](../changes/piano-roll-midi-key-lighting.md)
