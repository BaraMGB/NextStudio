# Piano Roll MIDI Key Lighting

## Summary

The Piano Roll keyboard now visualizes live MIDI input. A key lights while the corresponding note is held and returns to its normal color after note-off. The feedback follows the MIDI routing of the track currently displayed in the Piano Roll and works with both physical MIDI devices and NextStudio's virtual MIDI input.

## Source files

| Responsibility | Files |
|---|---|
| MIDI event subscription and active-track filtering | `App/include/PianoRollEditor.h`, `App/src/PianoRollEditor.cpp` |
| Active-note state and keyboard rendering | `App/include/KeyboardView.h`, `App/src/KeyboardView.cpp` |
| MIDI event source | `modules/tracktion_engine/modules/tracktion_engine/playback/devices/tracktion_MidiInputDevice.h`, `modules/tracktion_engine/modules/tracktion_engine/playback/devices/tracktion_MidiInputDevice.cpp` |

## Event source

`PianoRollEditor` implements `te::MidiInputDevice::MidiKeyChangeDispatcher::Listener`. The Tracktion dispatcher reports MIDI key-state changes after input processing and supplies:

- the destination `te::AudioTrack*`;
- an array of note numbers switched on;
- an array of note velocities;
- an array of note numbers switched off.

The dispatcher is shared by Tracktion MIDI input devices, so the Piano Roll does not need separate listeners for every physical device. Events from the virtual input used by the computer keyboard and the clickable Piano Roll keyboard pass through the same path.

`PianoRollEditor` owns a `juce::SharedResourcePointer<te::MidiInputDevice::MidiKeyChangeDispatcher>`. Its constructor registers the editor in the dispatcher's listener list. Its destructor removes the listener before the editor and its child components are destroyed. This symmetric registration prevents callbacks to a dead editor.

## Active-track filtering

`PianoRollEditor::midiKeyStateChanged()` updates the keyboard only when all of the following conditions hold:

1. a `KeyboardView` exists;
2. a track-specific `MidiViewport` exists;
3. the callback's destination track equals `m_pianoRollViewPort->getTrack()`.

This is important because the dispatcher is shared globally. MIDI routed to another track must not light the keyboard of the track currently shown in the Piano Roll.

The velocity array is intentionally unused. Lighting currently represents the binary held/released state; it does not encode velocity as brightness or opacity.

## Active-note state

`PianoKeyboardDisplay` stores active MIDI notes in a `juce::BigInteger`, with bit positions `0..127` corresponding directly to MIDI note numbers.

`setNoteDown(note, isDown)`:

1. rejects note numbers outside `0..127`;
2. compares the requested state with the current bit;
3. changes the bit only when the state actually differs;
4. repaints only the rectangle returned by `getRectangleForKey(note)`.

Avoiding a full keyboard repaint keeps updates small even when MIDI input is frequent.

The active-note state is transient UI state. It is not written to `ApplicationViewState`, `EditViewState`, the Tracktion edit, or undo history. Destroying the track-specific `KeyboardView` clears the state naturally.

## Batched MIDI events and rapid dragging

Tracktion's `MidiKeyChangeDispatcher` coalesces key changes over a short timer interval. Fast dragging across the clickable Piano Roll keyboard can therefore put several note-ons and note-offs in one callback. A note may occur in both arrays when it was pressed and released within the same dispatch interval.

`KeyboardView::setMidiNotesDown()` deliberately applies the arrays in this order:

1. note-ons;
2. note-offs.

Applying note-offs last ensures that keys already released during a rapid drag do not remain visually stuck. This ordering fixes the case where a fast mouse gesture previously left a vertical run of illuminated keys.

## Rendering and theme integration

`PianoKeyboardDisplay` receives `ApplicationViewState&` from `KeyboardView` through `EditViewState::m_applicationState`.

The active colors are resolved during painting:

- active white key: `ApplicationViewState::getPrimeColour()`;
- active black key: `ApplicationViewState::getPrimeColour().darker()`;
- inactive white key: the existing light grey keyboard color;
- inactive black key: black.

Resolving the PrimeColour at paint time avoids a duplicated cached theme value. The black-key variant is darkened to retain the visual distinction between black and white keys while using the same application theme color.

## Complete event flow

```text
Physical or virtual MIDI input
→ Tracktion MidiInputDevice processes note-on/note-off
→ MidiKeyChangeDispatcher batches the key-state change
→ PianoRollEditor::midiKeyStateChanged()
→ destination track is compared with the displayed Piano Roll track
→ KeyboardView::setMidiNotesDown()
→ PianoKeyboardDisplay::setNoteDown()
→ active-note bit changes
→ only the affected key rectangle is repainted
→ drawWhiteKey()/drawBlackKey() uses ApplicationViewState PrimeColour
```

## Mouse audition path

Clicking the Piano Roll keyboard continues to send notes through `EngineHelpers::getVirtualMidiInputDevice()`:

- `mouseDown()` sends note-on;
- crossing a key during `mouseDrag()` sends note-off for the old pitch and note-on for the new pitch;
- `mouseUp()` sends note-off for the final pitch.

Those messages are no longer handled by a separate visual shortcut. They return through Tracktion's normal MIDI routing and dispatcher, so mouse audition and external MIDI input share the same track filtering and visual state path.

## Lifetime behavior

`KeyboardView` is track-specific. `PianoRollEditor::setTrack()` creates it after the new `MidiViewport`; `clearTrack()` destroys it when the displayed track is cleared or replaced. Dispatcher callbacks received while no keyboard or viewport exists are ignored.

The dispatcher listener itself belongs to the longer-lived `PianoRollEditor`, not to each temporary `KeyboardView`. This avoids repeated global listener registration whenever the selected track changes.

## Validation

The implementation was validated with the repository build command:

```bash
BUILD_JOBS=12 ./build.sh rd
```

The build completes successfully. The relevant manual behavior to verify is:

1. route a MIDI input to the track displayed in the Piano Roll;
2. hold one or several external MIDI notes and confirm that only those keys use PrimeColour;
3. release the notes and confirm that the normal key colors return;
4. send MIDI to another track and confirm that the displayed keyboard does not react;
5. click and rapidly drag across the Piano Roll keyboard, then release the mouse and confirm that no keys remain lit;
6. change the application theme and confirm that subsequent active-key painting uses the current PrimeColour.

## Current limitations

- Lighting is binary and does not visualize velocity.
- The dispatcher reports batched state changes rather than the original total event order. Processing note-offs last is optimized for the common rapid-drag and released-key case.
- Lighting represents routed live MIDI input, not MIDI notes produced by arrangement playback.
