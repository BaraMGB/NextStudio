# Metronome Settings

## Purpose

The Audio settings tab contains global metronome sound and level controls. They apply to the current edit immediately and become the defaults applied whenever NextStudio creates or loads another edit.

## User interface

The **Metronome** group is displayed below JUCE's audio-device controls in **Settings → Audio**.

- **Click volume** controls the common level of both click sounds from 20% to 100%.
- **Accent** selects the WAV used for the first beat of each bar.
- **Regular** selects the WAV used for all other beats.
- **Default** restores Tracktion's embedded sample for the corresponding role.

NextStudio enables Tracktion's bar emphasis for edits that do not already contain an explicit emphasis setting.

## Managed sample files

A selected sample is validated as a readable WAV and copied before it is activated. Playback therefore does not depend on the original file remaining available. Managed files are stored next to the application settings:

```text
<user application data>/NextStudio/Metronome/
```

Accent and regular files use separate filename prefixes. Import creates a new uniquely named copy, switches Tracktion to that copy, and then removes obsolete managed copies for the same role. Resetting a role clears Tracktion's custom path and removes its managed copies. Files outside the managed directory are never deleted.

`MetronomeSampleManager` in `App/include/MetronomeSampleManager.h` and `App/src/MetronomeSampleManager.cpp` owns validation, import, managed-path recognition, and cleanup. `MetronomeSettingsComponent` in `App/include/AudioSettingsComponent.h` and `App/src/AudioSettingsComponent.cpp` owns the controls and asynchronous file chooser.

## Persistence and engine integration

The common level is stored in the `Metronome/MetronomeVolume` property of `ApplicationViewState` and mirrored to the current edit with `Edit::setClickTrackVolume()`. `MainComponent::setupEdit()` reapplies that global value after loading or creating an edit.

Custom sample paths are persisted by Tracktion's property storage through:

```cpp
tracktion_engine::Click::setClickWaveFile(engine, isAccent, managedPath);
```

An empty path restores Tracktion's embedded `bigclick.wav` or `littleclick.wav`. Calling `setClickWaveFile()` restarts active transports so the replacement is used immediately. No Tracktion submodule source is modified.

## Tests

`App/tests/MetronomeSampleManagerTests.cpp` verifies:

- settings-relative storage paths;
- rejection of missing, non-WAV, and unreadable files;
- independent managed copies that survive deletion of the source;
- role-specific cleanup;
- preservation of the active managed copy during replacement cleanup.
