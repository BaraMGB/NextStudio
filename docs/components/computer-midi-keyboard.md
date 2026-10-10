# Computer MIDI keyboard controller

- Type: reference
- Audience: contributors
- Scope: current performance-key state, virtual-input binding and root lifetime

## Source and ownership

`MainComponent` owns one `ComputerMidiKeyboardController` for the application session. It binds to the Tracktion virtual MIDI input's `juce::MidiKeyboardState`, not a separate command-per-note system.

Sources: `App/include/ComputerMidiKeyboardController.h`, `App/src/ComputerMidiKeyboardController.cpp`, `App/include/ComputerMidiKeyboardLayout.h`, `App/src/ComputerMidiKeyboardLayout.cpp`, and integration in `App/src/MainComponent.cpp`, `App/include/ExtendedUIBehavior.h` and `App/src/PluginWindow.cpp`.

## Event flow

```text
performance key/state event on an attached component root
→ ComputerMidiKeyboardController
→ JUCE MidiKeyboardComponent held-key transition logic
→ virtual input MidiKeyboardState
→ Tracktion live-MIDI routing
→ destination instrument
```

Transport/edit/project commands remain application commands; sustained note-on/off is not reconstructed from repeated command invocations. JUCE's held-key logic avoids the old indirect mapping lookup on release and command-based OS-repeat retrigger path. The [routing architecture](../architecture/midi-input-routing.md) owns automatic/manual destinations and exclusive focus; the controller does not substitute its own track policy.

## Mapping and aliases

The layout has 25 primary notes beginning at MIDI 48, uses channel 1 and the configured/default note-key descriptions, and clears JUCE's default QWERTY map before applying its own. User defaults and note labels are in [Keyboard Shortcuts](../ui/shortcuts.md#virtual-midi-keyboard-computer-keyboard), not repeated here.

Settings → Keys stores primary descriptions plus an optional upper-C alias in `AppSettings.xml`. Validation rejects duplicate assignments before applying the layout. Setting changes release held notes, update the controller layout and reapply the map. Display labels follow the shared MIDI naming convention (48=C3, 60=C4, 72=C5).

Settings integration lives in `App/include/KeyboardSettingsComponent.h`, `App/src/KeyboardSettingsComponent.cpp` and `App/include/AudioMidiSettings.h`. `KeyboardSettingsComponent` uses one outer `juce::Viewport`; its embedded `juce::KeyMappingEditorComponent` is sized to the complete tree content, so virtual performance keys and application shortcuts share the outer scrollbar rather than competing nested scrolling.

JUCE maps one primary KeyPress per note offset, so the additional upper-C alias is tracked explicitly alongside JUCE's primary held keys. Preserve both release paths; helper/layout tests are not proof of every simultaneous-alias/native-focus combination.

## Root binding and cleanup

- The controller attaches idempotently as a key listener to the main component. `ExtendedUIBehaviour` passes the same controller to plugin windows; attached roots use safe component references.
- `setKeyboardState()` releases owned notes before destroying/rebinding its JUCE keyboard. MainComponent retries virtual-device binding while DeviceManager rebuilds the input list.
- Edit switches, layout changes and focus leaving all attached roots release held notes, including an active explicit alias.
- Detaching a root removes its listener and releases when focus is no longer within an attached root. Shutdown releases notes and removes root/global focus listeners.
- The [project/setup interaction lock](../architecture/project-workflow.md#interaction-and-engine-boundary) detaches performance input from main/plugin roots during protected operations; restoring it must use the current virtual state, not an old edit reference.

The visible Piano Roll keyboard is separate: mouse audition sends messages through the same virtual input but owns/releases its audition pitch. [Piano Roll key lighting](piano-roll-editor.md#live-midi-key-lighting) consumes routed dispatcher events; it is not a direct visual shortcut from the controller.

## Validation and known limits

`App/tests/ComputerMidiKeyboardLayoutTests.cpp` covers primary layout/labels, alias recognition, custom mapping persistence, duplicate rejection, clearing default JUCE mapping and channel/offset policy. It does not provide complete native key-repeat/focus/plugin-window certification.

Focused validation should hold/release primary/alias keys, test multi-key passages and focus/edit/lock changes, and verify MIDI note-off on teardown. Original Linux diagnostics observed event-delivery stalls upstream of NextStudio routing; a temporary JUCE wakeup experiment was **not** retained in production. The controller refactor cannot be claimed to eliminate every OS/JUCE latency case. That experiment remains in [historical context](../archive/changes/computer-midi-keyboard-controller.md), not a supported workaround or new engine patch.
