# Agent Debug Interface

## Overview

NextStudio now contains a small internal agent/debug interface intended as a foundation for later agent-driven debugging.

It currently provides three capabilities:

1. filtered state dumps
2. a minimal command surface
3. scaled UI snapshots

The goal is to make the application observable and controllable without dumping large amounts of irrelevant internal data.

---

## Design Principles

### App-first

The interface focuses on NextStudio's own behavior:

- GUI state
- selection
- lower range state
- transport state
- track and plugin summaries
- project/edit context

### Filtered state instead of raw state

Full plugin states, especially VST3 plugin states, can contain large binary payloads or long opaque strings. These are expensive and not useful for agent reasoning.

Therefore the state dump is deliberately filtered and summarized.

### Small, stable command surface

The command API is intentionally minimal for now. It is meant to expose a few reliable, high-value actions before growing further.

### Scaled snapshots

Snapshots are captured with JUCE and scaled down to reduce storage and token cost when used downstream.

---

## Files

Implementation:

- `App/Source/Utilities/AgentDebug.h`
- `App/Source/Utilities/AgentDebug.cpp`

Integration points:

- `App/Source/MainComponent.h`
- `App/Source/MainComponent.cpp`

---

## Public MainComponent API

The following methods were added to `MainComponent`:

- `juce::String createAgentStateDump() const`
- `juce::File writeAgentStateDump() const`
- `juce::File captureAgentSnapshot(int maxWidth = 640) const`
- `bool executeAgentCommand(const juce::String& commandName, const juce::String& argument = {})`
- `bool selectTrackByName(const juce::String& trackName)`
- `void switchLowerRangeView(LowerRangeView view)`
- `juce::File getAgentDebugDirectory() const`

Context getters:

- `getApplicationState()`
- `getCurrentEdit()`
- `getEditViewState()`
- `getEditComponent()`
- `getHeaderComponent()`
- `getLowerRangeComponent()`

---

## State Dump

### Purpose

The state dump is a compact JSON summary of the current application state.

### Output location

Written to the temporary directory managed by the app:

- `agent-debug/state-dump-*.json`

### Included data

The dump currently includes:

- timestamp
- application name and version
- window bounds
- sidebar/setup/workdir information
- current edit file
- track counts
- current lower range view
- autosave flag
- transport state
- selection summary
- track summaries
- plugin summaries per track

### Plugin filtering strategy

The dump does **not** serialize full plugin state trees.

Instead it only stores summary data such as:

- plugin name
- plugin type
- enabled state
- child state count
- property count

### String filtering strategy

Strings are filtered by a small heuristic:

- long strings are truncated
- binary-like strings are replaced with a placeholder

This is intended to avoid token waste from opaque plugin payloads.

---

## Command Surface

### Purpose

The command interface provides a small set of stable actions for agent testing and later automation.

### Currently supported commands

- `play`
- `stop`
- `toggle-record`
- `toggle-metronome`
- `dump-state`
- `capture-snapshot`
- `select-track`
- `show-mixer`
- `show-piano-roll`
- `show-plugin-rack`

### Notes

- `select-track` expects a track name as argument
- lower-range commands switch the visible lower-range mode
- `dump-state` writes a JSON file
- `capture-snapshot` writes a PNG file

Unknown commands are rejected and logged.

---

## Snapshot Capture

### Purpose

The snapshot is a lightweight visual debug artifact.

### Implementation

Snapshots are created with JUCE using `createComponentSnapshot` on `MainComponent`.

### Output location

- `agent-debug/ui-snapshot-*.png`

### Scaling

Snapshots are scaled down to a configurable maximum width.

Current default:

- `640 px`

This keeps image size and downstream token usage under control.

---

## Logging

The agent/debug interface uses the central logging system documented in:

- `docs/logging.md`

Relevant categories used here include:

- `app`
- `workflow`
- `selection`
- `viewstate`

---

## Current Limitations

Current scope is intentionally small.

Not implemented yet:

- external transport protocol
- CLI or socket bridge
- mouse automation
- deep GUI tree export
- audio assertions
- plugin parameter editing commands
- ffmpeg-based post-processing

---

## Recommended Next Steps

Reasonable follow-up work:

1. add a temporary trigger path for manual testing
   - debug menu
   - keyboard shortcut
   - file-based command trigger

2. extend command coverage carefully
   - open project
   - select clip
   - render
   - add track

3. improve dump semantics
   - active clip details
   - visible UI panels
   - plugin window state
   - focused track/plugin context

4. optionally add alternate output formats
   - compact JSON
   - human-readable text summary

---

## Summary

This interface is meant to be a practical first step:

- observable
- filtered
- small
- extensible

It is not a full automation framework yet, but it provides a good base for agent-assisted debugging in a DAW context.
