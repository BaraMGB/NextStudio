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

Each debug-shell start attempt also carries a unique request id so single-instance launch rejections can be attributed to the correct failed launch attempt.

Each session stores:

- a generated `sessionId`
- the spawned `NextStudio --debug-shell` process
- buffered stdout/stderr line entries with monotonic line ids
- pending waiters for future shell responses
- exit state (`exited`, `exitCode`, `exitSignal`)

Stdout and stderr are collected line-by-line.

- stdout lines are stored as-is
- stderr lines are stored with the prefix `[stderr] `

The extension keeps a rolling buffer of recent lines for diagnostics. On pi `session_shutdown`, it rejects pending waiters, terminates every tracked child process, and clears the session map; cleanup is idempotent across reload, session replacement, and quit.

Line ids matter because response matching should be based on "the next matching response after command send", not on response text uniqueness. This avoids mis-correlating repeated `ok ...` lines in longer or noisier sessions.

Command execution is also serialized per session inside the extension. The shell protocol is inherently request/response over a single stdin/stdout stream, so serialisation avoids cross-correlating overlapping commands that would otherwise compete for the same next response.

If a command times out waiting for a shell response, the extension treats the session as desynchronised. That session must then be stopped and restarted before sending more commands, because a late response from the timed-out command can no longer be attributed safely.

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
3. wait for a JSON response object with:
   - `status == "ok"`
   - `code == "ready"`
4. if the process exits before readiness, return a startup failure and do not keep the dead session registered
5. if another NextStudio instance is already running, surface this specifically as a single-instance conflict
6. otherwise return session metadata and recent output

Single-instance conflict detection is explicit, not heuristic:

- the debug-shell launcher adds a unique `--debug-shell-request-id=...` argument to each start attempt
- if JUCE rejects that launch during its single-instance check, the short-lived app process reaches `shutdown()` without ever entering `initialise(...)`
- in that path NextStudio writes a rejection marker file containing the matching request id
- the launcher only reports `startup-single-instance-conflict` when that marker matches the current request id

Important note:

- NextStudio intentionally keeps JUCE single-instance protection enabled for normal DAW safety
- `--debug-shell` does **not** bypass that policy
- if a regular NextStudio instance is already open, debug-shell startup must fail with a clear agent-facing error
- the expected tool error code for this case is:
  - `startup-single-instance-conflict`

Important returned fields:

- `ok`
- `sessionId`
- `readyLine`
- `parsed`
- `binaryPath`
- `cwd`
- `recentLines`

Typical success shape:

```json
{
  "ok": true,
  "sessionId": "19a26866-edb6-473d-801f-976c4559837f",
  "readyLine": "{\"status\":\"ok\",\"code\":\"ready\",\"message\":\"debug shell started\",\"fields\":{}}"
}
```

### Tool: `nextstudio_debug_command`

Purpose:

- send exactly one command line to a running debug-shell session
- wait for the next shell response line after that command was sent

Parameters:

- `sessionId`
- `command`
- `timeoutMs` (optional)

Behavior:

1. locate the running session by `sessionId`
2. reject empty/whitespace-only or multi-line command strings
3. write one validated command line plus `"\n"` to the process stdin
4. wait for the next valid JSON response object whose `status` is either:
   - `ok`
   - `error`
5. return the matching response plus all newly collected lines since the command was sent

A single tool call must map to exactly one shell command line. Embedded `\n` or `\r` are rejected explicitly.

Important returned fields:

- `ok`
- `command`
- `responseLine`
- `parsed`
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
  "responseLine": "{\"status\":\"ok\",\"code\":\"ok\",\"fields\":{\"app\":\"NextStudio\",\"version\":\"0.04\",\"mode\":\"debug-shell\"}}",
  "newLines": [
    "{\"status\":\"ok\",\"code\":\"ok\",\"fields\":{\"app\":\"NextStudio\",\"version\":\"0.04\",\"mode\":\"debug-shell\"}}"
  ]
}
```

`responseLine` is the primary shell result.

`parsed` contains the decoded JSON response object.

`newLines` contains all newly observed shell and log lines since the command was issued, including stderr-prefixed lines.

After a successful `quit` response, the extension marks the session as closing immediately. Additional commands on that session are rejected instead of being allowed to time out against a shell that has already stopped reading input.

This is intentionally verbose enough to support debugging and validation from pi.

### Tool: `nextstudio_debug_stop`

Purpose:

- stop a tracked debug-shell session
- remove it from the extension's in-memory session map

Parameters:

- `sessionId`
- `force` (optional)

Behavior:

- marks the session as closing immediately so no further queued commands are accepted
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
4. call `nextstudio_debug_command` with `system-state`
5. verify or wait until `readyForPlayback=true`
6. call `nextstudio_debug_command` with `play`
7. optionally wait, inspect logs, or request screenshots
8. call `nextstudio_debug_command` with `transport-state`
9. call `nextstudio_debug_command` with `stop`
10. call `nextstudio_debug_command` with `quit`
11. optionally call `nextstudio_debug_stop` as cleanup if the process is still alive or the session must be discarded explicitly

Example command sequence:

```text
nextstudio_debug_start
nextstudio_debug_command(sessionId, "ping")
nextstudio_debug_command(sessionId, "system-state")
nextstudio_debug_command(sessionId, "play")
nextstudio_debug_command(sessionId, "transport-state")
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
- `system-state`
- `transport-state`
- `state-dump`
- `play`
- `stop`
- `screenshot`
- `project-save-as`
- `ensure-track`
- `select-track`
- `ensure-midi-clip`
- `ensure-midi-note`
- `set-plugin-parameter`
- `quit`

This separation is important:

- NextStudio owns the command semantics
- the pi extension owns persistent process/session handling inside pi

`system-state` is useful for readiness checks. It also reports `projectWorkflowActive`; while that field is `true`, state-changing debug commands return `busy`. Read-only diagnostics, screenshots, `stop`, and the harness-level `quit` command remain available.

`transport-state` is especially useful for validation because it exposes a compact transport snapshot without relying only on logs or visual inspection.

### Testing implications

When validating pi-driven control, both layers should be considered:

1. NextStudio debug-shell behavior
2. pi extension session/transport behavior

A successful test should verify:

- the session starts and returns `ready`
- `system-state` eventually reports `readyForPlayback=true`
- commands return the expected `responseLine`
- `transport-state` confirms the expected playback transition (`false -> true -> false`) and forward position movement while playing
- `newLines` contain plausible supporting output where relevant
- screenshots are copied or inspected before the debug-shell session is terminated

### Official test client

A repository-local test client is included for stable debug-shell validation.

Location:

- `tools/debug-shell-client.js`

Purpose:

- provide one maintained test harness instead of repeatedly creating ad-hoc scripts
- start a persistent debug-shell session
- wait for shell readiness and playback readiness
- send commands and parse response lines
- copy screenshots out of the temporary debug-shell sandbox before shutdown

The client exports:

- `NextStudioDebugShellClient`
- `parseResponseLine(...)`
- `runTransportSmokeTest(...)`
- `runCommandErrorSmokeTest(...)`
- `runStateDumpSmokeTest(...)`
- `runBasicShellSmokeTest(...)`
- `runEofSmokeTest(...)`
- `runSettingsIsolationSmokeTest(...)`
- `runEditingSmokeTest(...)`
- `runClientProtocolRegressionTest(...)`
- `runAllSmokeTests(...)`

Important client capabilities:

- `start()` waits for a JSON response with `status="ok"` and `code="ready"` and rejects cleanly if process startup fails
- `start()` should report an explicit single-instance conflict if another NextStudio process already holds the JUCE app lock
- `waitForSystemReady()` polls `system-state` until `readyForPlayback=true`
- `command()` sends one command, safely matches the next valid JSON response even in long noisy sessions, and parses it
- `parseResponseLine(...)` uses the platform JSON parser, including standard escaping for Unicode and control characters
- `copyFile()` preserves artifacts such as screenshots or state dumps before session exit
- `stop()` terminates the underlying process if cleanup is needed

Built-in smoke test modes are available via:

```bash
node tools/debug-shell-client.js smoke-transport
node tools/debug-shell-client.js smoke-errors
node tools/debug-shell-client.js smoke-state
node tools/debug-shell-client.js smoke-basic
node tools/debug-shell-client.js smoke-eof
node tools/debug-shell-client.js smoke-settings
node tools/debug-shell-client.js smoke-editing
node tools/debug-shell-client.js smoke-protocol
node tools/debug-shell-client.js smoke-all
```

`smoke-transport` performs a standard transport validation flow:

1. start debug shell
2. wait for system readiness
3. query `transport-state`
4. send `play`
5. wait briefly
6. query `transport-state` again
7. capture and copy a screenshot
8. send `stop`
9. query `transport-state` again
10. send `quit`

The smoke tests are intentionally assertive.

- `smoke-transport` validates readiness, playback transitions, forward transport movement, PNG structure and dimensions, and clean shutdown; `NEXTSTUDIO_REQUIRE_AUDIO_CLOCK=0` disables the sustained-playing and clock-advance assertions on hosts without an audio clock, while command acknowledgements and the stopped state remain required
- `smoke-errors` validates invalid screenshot arguments, unknown-command handling, and repeated identical `transport-state` responses
- `smoke-state` validates `state-dump` output and copies the dump to a persistent location before session teardown
- `smoke-basic` validates startup, ten repeated `ping` commands, `quit`, and process exit; CI runs it on Windows
- `smoke-eof` closes stdin after `ping` and verifies deterministic clean application exit; CI runs it on Windows
- `smoke-settings` verifies that settings live inside the session sandbox and the normal `AppSettings.xml` remains byte-for-byte unchanged
- `smoke-editing` creates and reuses a stable track and clip, inserts and updates a MIDI note, sets a plugin parameter, and confirms all mutations through state dumps
- `smoke-protocol` uses a temporary Node-based fake shell to validate repeated identical JSON responses, adversarial escaped values, malformed unrelated output, and process exit while a response wait is in flight

This client is the preferred automated validation path for transport, command-error handling, state-dump behavior, and shell protocol regressions inside this repository.

---

### 1. Agent debug utilities

These are internal helpers used by the app:

- filtered state dump creation
- UI snapshot capture
- a narrow host abstraction for debug-only access to app state

Relevant files:

- `App/include/AgentDebug.h`
- `App/src/AgentDebug.cpp`
- `App/include/DebugHost.h`
- `App/include/MainComponentDebugHost.h`
- `App/src/MainComponentDebugHost.cpp`
- `App/include/DebugSessionEnvironment.h`
- `App/src/DebugSessionEnvironment.cpp`

### 2. Debug shell

This is the current agent-facing control path.

It starts NextStudio in a dedicated debug mode and accepts simple commands over `stdin`, returning one response line per command over `stdout`.

Relevant files:

- `App/include/DebugCommand.h`
- `App/include/DebugResult.h`
- `App/include/DebugProtocol.h`
- `App/src/DebugProtocol.cpp`
- `App/include/DebugSnapshotWriter.h`
- `App/src/DebugSnapshotWriter.cpp`
- `App/include/DebugStateFilter.h`
- `App/src/DebugStateFilter.cpp`
- `App/include/DebugAppController.h`
- `App/src/DebugAppController.cpp`
- `App/include/DebugShell.h`
- `App/src/DebugShell.cpp`
- `App/src/Main.cpp`

---

## Debug Shell

### Start

Run NextStudio with:

```bash
NextStudio --debug-shell
```

The app starts normally with GUI, but also opens a small command loop over `stdin/stdout`.

### Command model

The shell is intentionally minimal. Each request is exactly one physical line.

Diagnostic and transport commands support the legacy command-name form. Except for `screenshot`, trailing legacy arguments are rejected:

| Command | Argument | Success fields | Errors | Aliases |
| --- | --- | --- | --- | --- |
| `help` | none | `commands` | `invalid-argument` | — |
| `ping` | none | `app`, `version`, `mode` | `invalid-argument` | — |
| `system-state` | none | paths, component readiness, project-workflow state, and transport summary | `invalid-argument` | `system_state` |
| `transport-state` | none | `playing`, `recording`, `looping`, `positionSeconds` | `not-ready`, `invalid-argument` | `transport_state` |
| `state-dump` | none | `path` | `invalid-argument`, `io-error` | `state_dump` |
| `play` | none | `playing` | `not-ready`, `invalid-argument`, `busy` | — |
| `stop` | none | `playing` | `not-ready`, `invalid-argument` | — |
| `screenshot` | optional integer `maxWidth` from `1` to `8192`; default `640` | `path` | `invalid-argument`, `io-error` | — |
| `project-save-as` | none | `projectBrowserMode` | `not-ready`, `invalid-argument`, `busy` | — |
| `quit` | none | `quitting` | `invalid-argument` | `exit` |

Editing requests use a JSON request object so names and numeric values do not need a second custom escaping grammar:

```json
{"command":"ensure-track","arguments":{"type":"midi","name":"Agent MIDI"}}
{"command":"select-track","arguments":{"trackId":"1010"}}
{"command":"ensure-midi-clip","arguments":{"trackId":"1010","name":"Intro","startSeconds":0,"lengthSeconds":4}}
{"command":"ensure-midi-note","arguments":{"clipId":"1013","noteNumber":60,"startBeats":0,"lengthBeats":1,"velocity":100}}
{"command":"set-plugin-parameter","arguments":{"pluginId":"1011","parameterId":"volume","value":0.75}}
```

| Command | Required arguments | Deterministic behavior | Success fields |
| --- | --- | --- | --- |
| `ensure-track` | `type`: `midi` or `audio`; non-empty `name` | Reuses a same-name track of the requested type | `trackId`, `name`, `type`, `created` |
| `select-track` | `trackId` | Selects exactly the stable Tracktion item ID | `trackId`, `name`, `selected` |
| `ensure-midi-clip` | `trackId`, `name`, `startSeconds >= 0`, `lengthSeconds > 0` | Reuses a same-name MIDI clip at the same range | `clipId`, `trackId`, `created` |
| `ensure-midi-note` | `clipId`, MIDI `noteNumber`, `startBeats`, `lengthBeats`, `velocity` | Reuses the pitch/start/length tuple and sets its velocity | `clipId`, `noteKey`, `created`, `velocity` |
| `set-plugin-parameter` | `pluginId`, `parameterId`, native-range numeric `value` | Sets a specific stable plugin/parameter ID | IDs, native and normalised values |

Malformed JSON requests return `invalid-request`. Type/range errors return `invalid-argument`; missing IDs return `not-found`; wrong track types return `wrong-type`; engine mutation failures return `edit-error`. Editing requests return `busy` while a project workflow is active. Unknown command names return `unknown-command`. Empty lines are ignored and do not produce a response.

### Response model

Each command returns exactly one response line.

Responses use JSON Lines: one compact JSON object followed by one newline.

```json
{"status":"ok","code":"ready","message":"debug shell started","fields":{}}
{"status":"ok","code":"ok","fields":{"app":"NextStudio","version":"0.04","mode":"debug-shell"}}
{"status":"ok","code":"ok","fields":{"playing":"true","recording":"false","looping":"false","positionSeconds":"2.370"}}
{"status":"ok","code":"ok","fields":{"path":"/tmp/.../agent-debug/state-dump-....json"}}
{"status":"error","code":"invalid-argument","message":"screenshot expects an optional integer maxWidth from 1 to 8192","fields":{}}
{"status":"error","code":"unknown-command","message":"Unknown command. Try 'help'.","fields":{}}
```

Schema:

| Property | Type | Meaning |
| --- | --- | --- |
| `status` | `"ok"` or `"error"` | Result class |
| `code` | string | Stable machine-readable result code |
| `message` | string, optional | Human-readable diagnostic |
| `fields` | object of string values | Command-specific payload |

JSON escaping is the complete wire escaping contract. Spaces, quotes, backslashes, equals signs, Unicode, tabs, carriage returns, and logical newlines round-trip through standard JSON escaping without introducing extra physical protocol lines. Unknown stdout lines are not responses and are ignored by the maintained clients while they wait for the next response object.

The shell is intended to be machine-readable, not interactive in a human shell-like sense.

---

## Debug Shell Session Isolation

### Separate temp sandbox

`--debug-shell` runs in its own session-specific temporary sandbox.

This only applies after a debug-shell session has actually started.
If another NextStudio instance is already running, JUCE single-instance protection prevents the debug-shell process from starting at all.

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

### Settings boundary

The Tracktion temporary directory, recovery data, workspace, state dumps, screenshots, and `ApplicationViewState` settings all live inside the same session sandbox. Debug mode neither reads nor writes the normal user `NextStudio/AppSettings.xml`. `system-state` reports `settingsPath` and `debugArtifactsPath` for assertions. The complete session directory is deleted during application shutdown after clients have had the opportunity to copy requested artifacts.

---

## Debug host abstraction

The debug system no longer reaches directly into `MainComponent` through a wide public API.

Instead it uses a small internal host interface:

- `App/include/DebugHost.h`

Current concrete adapter:

- `App/include/MainComponentDebugHost.h`
- `App/src/MainComponentDebugHost.cpp`

This keeps the debug-shell integration cleaner by isolating:

- app/edit access
- screenshot capture
- debug artifact directory access
- quit requests

`MainComponent` remains the backing implementation, but the debug stack now depends on the host abstraction rather than on a broad set of debug-specific public methods. The host is non-owning, must outlive `DebugAppController`, and may only be called on the JUCE message thread. Returned pointers are valid only during synchronous command execution.

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
- track IDs, types, selection, and clip summaries
- clip IDs, ranges, and MIDI-note summaries
- plugin IDs, compact state counts, and parameter IDs/current values per track

### Filtering strategy

The dump intentionally avoids full raw plugin state serialization.

Instead it stores compact summaries such as:

- plugin name
- plugin type
- enabled state
- child state count
- property count
- stable plugin and parameter IDs
- current native and normalised parameter values

Strings are also filtered:

- long strings are truncated
- binary-like strings are replaced with a placeholder

---

## Snapshot Capture

### Purpose

The snapshot is a lightweight visual debug artifact.

### Implementation

Snapshots are created with JUCE using `createComponentSnapshot` through the active debug host implementation.

At the moment this is backed by `MainComponentDebugHost`, which delegates to `MainComponent`.

### Output location

- `agent-debug/ui-snapshot-*.png`

In debug-shell mode this path lives inside the debug session sandbox.

### Scaling

Snapshots are scaled down to a configurable maximum width.

Current default:

- `640 px`

This keeps image size and downstream token usage under control.

### Testing requirement

A complete screenshot test is **not** just a successful command response.

Production code rejects invalid component bounds or images, checks output stream creation and PNG encoding, flushes the stream, verifies non-zero file size, decodes the PNG again, and compares decoded dimensions. Any failure deletes partial output and returns `io-error`.

`smoke-transport` copies the artifact before shutdown and validates the PNG signature, IHDR record, non-zero size, and dimensions. Automated tests do not judge visual semantics; visual inspection is still required when UI appearance itself is under test:

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

## Build and validation

From the repository root:

```bash
BUILD_JOBS=12 ./build.sh rd
ctest --test-dir autobuild/RelWithDebInfo --output-on-failure
node tools/debug-shell-client.js smoke-all
```

Focused C++ tests cover command parsing, JSON response round-trips and malformed input, fake-host controller validation/error/quit behavior, state-string filtering and truncation, validated PNG writing/failure paths, and explicit settings-file isolation. The Node suite covers complete process/session behavior and editing mutations.

CI behavior:

- all platforms build and run CTest
- Linux runs `smoke-all` under Xvfb; because hosted runners expose no audio device/clock and immediately stop playback, CI sets `NEXTSTUDIO_REQUIRE_AUDIO_CLOCK=0` while still checking play/stop acknowledgements, the final stopped state, and all other assertions
- Windows runs `smoke-basic` and `smoke-eof`, including startup, repeated redirected-pipe commands, EOF, `quit`, and clean process exit
- macOS runs the client protocol regression without launching the GUI

### Platform behavior

- A dedicated blocking worker reads stdin on every platform; it never executes application commands itself.
- Linux and macOS use `poll()` plus `read()` with a bounded wake interval so shutdown can join the reader deterministically.
- Windows uses `ReadFile()` for console or redirected pipe input and `CancelSynchronousIo()` during shutdown. EOF, broken pipes, cancellation, and parent termination are handled without blocking the message thread.
- Complete lines are posted to the JUCE message thread. All parsing side effects, host access, and command execution occur there.
- `stop()` cancels or wakes the reader and joins it before the shell is destroyed.

## Current Limitations

Current scope is intentionally small.

Not implemented yet:

- audio-file or generated test-sample insertion
- plugin insertion/removal commands
- clip deletion, movement, and audio-clip editing
- MIDI-note deletion and bulk operations
- socket or IPC transport
- deep GUI tree export
- rendered-audio assertions
- headless offscreen rendering mode

---

## Recommended Next Steps

Reasonable next additions:

1. add generated test-sample and audio-clip insertion with explicit fixture ownership
2. add deterministic deletion and movement operations using stable IDs
3. add rendered-audio assertions only after defining tolerances and artifact retention
4. preserve one request and one JSON response per physical line; do not grow a scripting language
5. keep every new mutation idempotent where practical and confirmable through state dumps

---

## Summary

The current system is intentionally modest:

- internal debug utilities for state and snapshots
- a small debug shell for agent control
- isolated debug sessions that do not touch normal recovery data

It is not a full automation framework yet, but it is now a clean base for incremental agent-driven testing and debugging.
