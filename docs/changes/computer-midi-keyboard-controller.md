# Computer MIDI Keyboard Controller

## Summary

NextStudio no longer routes the computer keyboard through `ApplicationCommandManager` command IDs for MIDI note playback. Sustained note handling now lives in a dedicated `ComputerMidiKeyboardController` built around `juce::MidiKeyboardComponent` and Tracktion's virtual MIDI input `keyboardState`.

This refactor removes the previous command-based note-on/note-off reconstruction path that was prone to duplicated note-ons, missed note-offs, and stuck notes when the OS generated key-repeat events or focus changed while notes were held.

## Source files

| Responsibility | Files |
|---|---|
| Computer-keyboard layout and legacy aliases | `App/include/ComputerMidiKeyboardLayout.h`, `App/src/ComputerMidiKeyboardLayout.cpp` |
| Global computer-MIDI input controller | `App/include/ComputerMidiKeyboardController.h`, `App/src/ComputerMidiKeyboardController.cpp` |
| Main-window binding to the virtual MIDI input | `App/include/MainComponent.h`, `App/src/MainComponent.cpp` |
| Plugin-window participation in the same key listener chain | `App/include/ExtendedUIBehavior.h`, `App/include/PluginWindow.h`, `App/src/PluginWindow.cpp` |
| Piano Roll keyboard display and mouse audition | `App/include/KeyboardView.h`, `App/src/KeyboardView.cpp` |
| Layout regression tests | `App/tests/ComputerMidiKeyboardLayoutTests.cpp`, `App/CMakeLists.txt` |

## New event flow

```text
PC keyboard key event
→ ComputerMidiKeyboardController (global key listener)
→ juce::MidiKeyboardComponent key state logic
→ juce::MidiKeyboardState of Tracktion virtual MIDI input
→ Tracktion normal live-MIDI routing
→ destination instrument / plugin
```

The important architectural change is that held-note state is no longer reconstructed indirectly from command invocations. JUCE's dedicated MIDI-keyboard logic owns the transition state instead.

## Why commands were removed

The old implementation encoded each note as an application command and then:

1. sent note-on from `perform()`;
2. remembered pressed `KeyPress` objects separately;
3. later tried to rediscover the target note in `keyStateChanged()` to send note-off.

This design had several failure modes:

- OS key repeat could retrigger note-on while a note was already held;
- note-off depended on a second mapping lookup instead of the original note state;
- focus loss had no single owner for releasing all held notes;
- duplicate aliases such as `Q` and `,` for the same pitch were awkward to balance;
- plugin windows were outside the main component hierarchy and could miss the same global state handling.

The new controller removes all MIDI note commands from `MainComponent` and keeps command routing only for transport, edit, and project actions.

## Layout and aliases

The default computer-keyboard layout still starts at MIDI note 48 and keeps the historical mapping:

```text
Lower row: Y S X D C V G B H N J M
Upper row: Q 2 W 3 E R 5 T 6 Z 7 U I
```

`Q` and `,` remain aliases for the same upper C. JUCE's `MidiKeyboardComponent` only supports one `KeyPress` per note offset in its built-in map, so the alias pair is handled explicitly by `ComputerMidiKeyboardController` while the remaining 24 keys are configured through `ComputerMidiKeyboardLayout::applyTo()`.

These performance keys are no longer exposed through the generic Settings → Keys command editor because they are not application commands anymore.

## Lifetime and focus behaviour

`MainComponent` owns one `ComputerMidiKeyboardController` for the whole application session.

- The controller attaches as a key listener to the main component.
- `ExtendedUIBehaviour` passes the same controller into each `PluginWindow`, so plug-in editors participate in the same held-note state handling.
- On edit switches, `MainComponent::bindComputerMidiKeyboard()` releases any owned notes, waits for the virtual MIDI input if needed, and then rebinds the controller to the new `juce::MidiKeyboardState`.
- On focus loss outside the attached component roots, the controller releases all of its notes.
- On shutdown it detaches cleanly from all registered roots.

## Piano Roll keyboard interaction

This refactor does not replace the Piano Roll's visible keyboard display.

`KeyboardView` continues to:

- render note highlight state in `PianoKeyboardDisplay`;
- send mouse audition notes through the same Tracktion virtual MIDI input;
- release any auditioned note explicitly on drag transitions, mouse-up, and destruction.

That keeps mouse audition and computer-keyboard performance on the same live-MIDI routing path.

## Tests

`App/tests/ComputerMidiKeyboardLayoutTests.cpp` verifies that:

- the dedicated layout contains the expected 24 primary JUCE mappings;
- the historical `Q` and `,` aliases are still recognised as performance keys;
- the helper clears JUCE's default QWERTY mappings before applying NextStudio's own layout;
- the layout still uses MIDI channel 1 and the expected note offsets.

The project validation command remains:

```bash
BUILD_JOBS=12 ./build.sh rd
ctest --test-dir autobuild/RelWithDebInfo --output-on-failure
```

## Bug description: Issue #52

Issue #52 reported repeated or stuck notes when using the computer keyboard as a virtual MIDI input.

Observed symptoms included:

- repeated retriggering while a key was held;
- occasional hanging notes after key release;
- missed note-offs in multi-key passages;
- timing instability that was much easier to trigger with the command-based implementation.

### Refactor result

The command-based failure modes were removed by this change. The remaining live-MIDI path is now the same for:

- the computer keyboard;
- the Piano Roll mouse keyboard;
- physical MIDI devices routed through Tracktion;
- third-party instrument plug-ins.

### Important diagnostic finding

During Linux diagnostics, raw X11/JUCE key-event delivery was observed to stall before NextStudio's own MIDI routing on some systems. A temporary JUCE event-loop wakeup workaround reduced those stalls during testing, which strongly suggests that at least one class of remaining latency is upstream of NextStudio's MIDI implementation.

That JUCE test patch was intentionally not kept in the production tree. This document records the finding because it explains why a command-system refactor alone cannot eliminate every observed latency case on affected Linux setups.
