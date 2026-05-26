# Agent Debug and Debug Shell

## Overview

NextStudio currently exposes two related debugging layers:

1. **Agent debug utilities** inside the app
2. **Debug shell** for simple agent control via stdin/stdout

The current direction for app control is the **debug shell**.

The goal is to keep the system:

- small
- deterministic
- easy to test
- safe for normal user sessions

---

## Current Architecture

## Pi Extension

A project-local pi extension is included for driving the debug shell from pi.

Location:

- `.pi/extensions/nextstudio-debug.ts`

This means the tool is currently available only inside this repository when pi loads project-local extensions.
It is **not** installed globally under `~/.pi/agent/extensions/`.

The extension exposes these pi tools:

- `nextstudio_debug_start`
- `nextstudio_debug_command`
- `nextstudio_debug_stop`

These tools wrap a persistent NextStudio `--debug-shell` process and allow command-by-command interaction without rebuilding an ad-hoc external harness for every test.

### Internal model

The extension keeps a process session map in memory.

Each session stores:

- a generated `sessionId`
- the spawned `NextStudio --debug-shell` process
- buffered stdout/stderr lines
- pending waiters for future shell responses
- exit state (`exited`, `exitCode`, `exitSignal`)

Stdout and stderr are collected line-by-line.

- stdout lines are stored as-is
- stderr lines are stored with the prefix `[stderr] `

The extension keeps a rolling buffer of recent lines for diagnostics.

### Tool: `nextstudio_debug_start`

Purpose:

- start a fresh persistent NextStudio debug-shell process
- wait until the shell reports readiness

Parameters:

- `binaryPath` (optional)
- `cwd` (optional)
- `timeoutMs` (optional)

Default binary path:

- `autobuild/RelWithDebInfo/App/NextStudio_artefacts/RelWithDebInfo/NextStudio`

Behavior:

1. spawn `NextStudio --debug-shell`
2. begin collecting stdout/stderr lines
3. wait for a line starting with:
   - `ok code=ready`
4. return session metadata and recent output

Important returned fields:

- `ok`
- `sessionId`
- `readyLine`
- `binaryPath`
- `cwd`
- `recentLines`

Typical success shape:

```json
{
  "ok": true,
  "sessionId": "19a26866-edb6-473d-801f-976c4559837f",
  "readyLine": "ok code=ready message=\"debug shell started\""
}
```

### Tool: `nextstudio_debug_command`

Purpose:

- send exactly one command line to a running debug-shell session
- wait for the next shell response line

Parameters:

- `sessionId`
- `command`
- `timeoutMs` (optional)

Behavior:

1. locate the running session by `sessionId`
2. write `command + "\n"` to the process stdin
3. wait for the next line that starts with either:
   - `ok `
   - `error `
4. return the matching response plus all newly collected lines since the command was sent

Important returned fields:

- `ok`
- `command`
- `responseLine`
- `newLines`
- session status fields (`sessionId`, `exited`, `exitCode`, `exitSignal`)

Typical example:

```json
{
  "sessionId": "19a26866-edb6-473d-801f-976c4559837f",
  "command": "ping",
  "timeoutMs": 10000
}
```

Typical success shape:

```json
{
  "ok": true,
  "command": "ping",
  "responseLine": "ok code=ok app=NextStudio version=0.01 mode=debug-shell",
  "newLines": [
    "ok code=ok app=NextStudio version=0.01 mode=debug-shell"
  ]
}
```

`responseLine` is the primary shell result.

`newLines` contains all newly observed shell and log lines since the command was issued, including stderr-prefixed lines.

This is intentionally verbose enough to support debugging and validation from pi.

### Tool: `nextstudio_debug_stop`

Purpose:

- stop a tracked debug-shell session
- remove it from the extension's in-memory session map

Parameters:

- `sessionId`
- `force` (optional)

Behavior:

- sends `SIGTERM` by default
- sends `SIGKILL` when `force=true`
- removes the session from the extension map regardless

Typical success shape:

```json
{
  "ok": true,
  "force": false,
  "sessionId": "19a26866-edb6-473d-801f-976c4559837f"
}
```

### Typical pi workflow

A normal pi-driven flow looks like this:

1. call `nextstudio_debug_start`
2. store the returned `sessionId`
3. call `nextstudio_debug_command` with `ping`
4. call `nextstudio_debug_command` with `play`
5. optionally wait, inspect logs, or request screenshots
6. call `nextstudio_debug_command` with `stop`
7. call `nextstudio_debug_command` with `quit`
8. optionally call `nextstudio_debug_stop` as cleanup if the process is still alive or the session must be discarded explicitly

Example command sequence:

```text
nextstudio_debug_start
nextstudio_debug_command(sessionId, "ping")
nextstudio_debug_command(sessionId, "play")
nextstudio_debug_command(sessionId, "screenshot")
nextstudio_debug_command(sessionId, "stop")
nextstudio_debug_command(sessionId, "quit")
```

### Relationship to NextStudio debug shell

The pi extension is only a transport/persistence layer.

It does **not** implement application behavior itself.

It simply wraps the existing NextStudio debug shell and forwards command lines into:

- `help`
- `ping`
- `play`
- `stop`
- `screenshot`
- `quit`

This separation is important:

- NextStudio owns the command semantics
- the pi extension owns persistent process/session handling inside pi

### Testing implications

When validating pi-driven control, both layers should be considered:

1. NextStudio debug-shell behavior
2. pi extension session/transport behavior

A successful test should verify:

- the session starts and returns `ready`
- commands return the expected `responseLine`
- `newLines` contain plausible supporting output where relevant
- screenshots are copied or inspected before the debug-shell session is terminated

---

### 1. Agent debug utilities

These are internal helpers used by the app:

- filtered state dump creation
- UI snapshot capture
- a few convenience integration points on `MainComponent`

Relevant files:

- `App/Source/Utilities/AgentDebug.h`
- `App/Source/Utilities/AgentDebug.cpp`
- `App/Source/MainComponent.h`
- `App/Source/MainComponent.cpp`

### 2. Debug shell

This is the current agent-facing control path.

It starts NextStudio in a dedicated debug mode and accepts simple commands over `stdin`, returning one response line per command over `stdout`.

Relevant files:

- `App/Source/Debug/DebugCommand.h`
- `App/Source/Debug/DebugResult.h`
- `App/Source/Debug/DebugAppController.h`
- `App/Source/Debug/DebugAppController.cpp`
- `App/Source/Debug/DebugShell.h`
- `App/Source/Debug/DebugShell.cpp`
- `App/Source/Main.cpp`

---

## Debug Shell

### Start

Run NextStudio with:

```bash
NextStudio --debug-shell
```

The app starts normally with GUI, but also opens a small command loop over `stdin/stdout`.

### Command model

The shell is intentionally minimal.

Each command is a single line.

Current supported commands:

- `help`
- `ping`
- `play`
- `stop`
- `screenshot`
- `screenshot 800`
- `quit`

### Response model

Each command returns exactly one response line.

Typical responses:

```text
ok code=ready message="debug shell started"
ok code=ok app=NextStudio version=0.01 mode=debug-shell
ok code=ok playing=true
ok code=ok playing=false
ok code=ok path=/tmp/.../ui-snapshot-....png
ok code=ok quitting=true
error code=unknown-command message="Unknown command. Try 'help'."
```

The shell is intended to be machine-readable, not interactive in a human shell-like sense.

---

## Debug Shell Session Isolation

### Separate temp sandbox

`--debug-shell` runs in its own session-specific temporary sandbox.

It does **not** use the normal recovery/temp area of regular user sessions.

This is important because debug-shell sessions are disposable and must not interfere with real recovery data.

### Guarantees

In debug-shell mode:

- no normal recovery dialog is shown
- normal user recovery data is not loaded
- normal user recovery data is not deleted
- autosave and debug artifacts are written only inside the debug-shell session sandbox

Typical location:

- `/tmp/NextStudio/debug-shell/session-...`

This session directory is temporary and should be treated as disposable.

---

## Public MainComponent API

The following methods exist as internal integration points:

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

Note: the long-term control path should prefer the debug shell over ad-hoc keyboard shortcuts or temporary command hooks.

---

## State Dump

### Purpose

The state dump is a compact JSON summary of the current application state.

### Output location

Written to the app-managed temporary area:

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

### Filtering strategy

The dump intentionally avoids full raw plugin state serialization.

Instead it stores compact summaries such as:

- plugin name
- plugin type
- enabled state
- child state count
- property count

Strings are also filtered:

- long strings are truncated
- binary-like strings are replaced with a placeholder

---

## Snapshot Capture

### Purpose

The snapshot is a lightweight visual debug artifact.

### Implementation

Snapshots are created with JUCE using `createComponentSnapshot` on `MainComponent`.

### Output location

- `agent-debug/ui-snapshot-*.png`

In debug-shell mode this path lives inside the debug session sandbox.

### Scaling

Snapshots are scaled down to a configurable maximum width.

Current default:

- `640 px`

This keeps image size and downstream token usage under control.

### Testing requirement

A successful screenshot test is **not** just a successful command response.

The test must also inspect the produced PNG and confirm that it contains a plausible NextStudio UI capture:

- the image file exists
- the PNG is readable
- the image is not empty or corrupt
- the visible content matches a real NextStudio window state

In debug-shell mode, screenshots are written inside the session-specific temporary sandbox and are typically removed when the session exits.

Because of that, screenshot tests must inspect or copy the PNG **before** sending `quit` or otherwise terminating the debug-shell session.

---

## Logging

The agent/debug code uses the central logging system documented in:

- `docs/logging.md`

Relevant categories commonly seen here include:

- `app`
- `workflow`
- `selection`
- `viewstate`
- `transport`
- `autosave`

For reliable agent control, logs should be kept separate from command responses whenever possible.

---

## Current Limitations

Current scope is intentionally small.

Not implemented yet:

- track creation commands
- clip insertion commands
- MIDI note insertion commands
- test sample insertion commands
- socket or IPC transport
- deep GUI tree export
- audio assertions
- plugin parameter editing commands
- headless offscreen rendering mode

---

## Recommended Next Steps

Reasonable next additions:

1. extend debug-shell command coverage carefully
   - create track
   - insert clip
   - insert MIDI note
   - insert test sample

2. keep commands deterministic
   - prefer `set`-style behavior over toggle-style behavior

3. preserve shell simplicity
   - one command per line
   - one response per command
   - no scripting language

4. keep debug-shell isolated from normal user recovery flows

---

## Summary

The current system is intentionally modest:

- internal debug utilities for state and snapshots
- a small debug shell for agent control
- isolated debug sessions that do not touch normal recovery data

It is not a full automation framework yet, but it is now a clean base for incremental agent-driven testing and debugging.
