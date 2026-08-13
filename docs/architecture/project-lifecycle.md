# Project Lifecycle

## Scope

This document describes creation, loading, saving, save-as, unsaved-change handling, autosave, crash recovery, and project replacement. The main implementation is split between:

- `App/include/ProjectLifecycle.h`
- `App/src/ProjectLifecycle.cpp`
- `App/include/MainComponent.h`
- `App/src/MainComponent.cpp`
- `App/include/EditComponent.h`
- `App/src/EditComponent.cpp`
- `App/src/ProjectsBrowser.cpp`

## File types

### Persistent project

Normal projects use:

```text
.tracktionedit
```

`ProjectLifecycle::isPersistentProjectFile()` performs a case-insensitive extension check. Save targets are normalized with `withProjectExtension()`.

### Recovery snapshot

Autosave/recovery files use:

```text
.nextTemp
```

They are stored in Tracktion Engine's temporary directory and are intentionally not treated as normal persistent projects. The loader permits this extension only when the file is known to be a recovery file in the active temporary directory.

## Startup flow

`MainComponent` initializes engine services and built-in plug-ins, then calls `openValidStartEdit()`.

```text
Resolve/create engine temp directory
→ find most recent recovery edit
→ if recovery exists, ask whether to restore
   ├── Yes: setupEdit(recovery file)
   └── No: delete recovery temp directory and recreate it
→ if no restore: setupEdit(empty file argument)
```

An empty `juce::File` argument means “create a new project.” It is converted into a new `.nextTemp` target before the Tracktion edit is created.

## Project requests from the browser

`ProjectsBrowserComponent` does not replace the edit directly. It writes a request into `ProjectLifecycle::ProjectRequestState` and emits a change message.

The request has one of three actions:

- `none`;
- `newProject`;
- `loadProject` with a file.

`MainComponent::changeListenerCallback()` consumes the request with `take()` and posts `setupEdit()` through `MessageManager::callAsync()`.

### Why requests are cleared before opening a chooser

The load button calls `m_projectRequest.clear()` before showing the file chooser. If the user cancels, no previous load request can accidentally be replayed by a later change notification.

`requestLoadProject()` also rejects files that do not exist or do not have the persistent project extension.

## Load validation

Before replacing the current edit, `ProjectLifecycle::inspectLoadFile()` checks:

1. the file exists;
2. the extension is supported in the current context;
3. the file is not empty;
4. XML data has an `EDIT` root, or binary `ValueTree` data has type `EDIT`.

Possible statuses are:

- `valid`;
- `missing`;
- `unsupportedExtension`;
- `empty`;
- `invalidData`.

`MainComponent::setupEdit()` maps invalid statuses to a user-visible reason and leaves the current project untouched.

## Unsaved-change decision

Before switching away from an existing edit, `handleUnsavedEdit()` checks `Edit::hasChangedSinceSaved()`.

The dialog offers:

- **Yes** — save and continue only if saving succeeds;
- **No** — discard and continue;
- **Cancel** — keep the current project and stop the action.

The pure helper `shouldProceedAfterUnsavedChoice()` encodes this decision matrix. A cancelled or failed save never proceeds with project replacement.

The same guard is used when the operating system requests application shutdown.

## Safe project replacement

`setupEdit()` follows a defensive order designed to preserve the old project until a valid replacement exists.

### 1. Determine request type

- empty input file: new edit;
- input file inside the engine temp directory: recovery edit;
- other supported file: persistent edit.

### 2. Validate the input

Non-new projects are inspected before any current UI or model is destroyed.

### 3. Handle unsaved current work

If the user cancels or saving fails, replacement stops immediately.

### 4. Construct the replacement first

The replacement is created in a local `std::unique_ptr<te::Edit>`:

- `te::createEmptyEdit()` for a new project;
- `te::loadEditFromFile()` for an existing or recovery project.

Exceptions are logged. A null replacement shows an error and leaves the current edit active.

### 5. Detach current state

Once a replacement exists:

- global selection is cleared;
- custom UI behavior is detached from the old edit;
- edit and transport listeners are removed.

### 6. Destroy old dependents

All components referencing the old edit are destroyed before `m_edit`:

```text
m_editorContainer
m_header
m_editComponent
m_lowerRange
m_sideBarBrowser
m_editViewState
m_edit
```

### 7. Manage recovery files

When loading a recovery edit, its source file must survive replacement. For other switches, old `*.nextTemp` snapshots are removed. The temporary directory is then ensured to exist.

### 8. Install and initialize the replacement

The new edit becomes `m_edit`; focused UI behavior is updated; a new project has its default audio tracks cleared; the temp directory is assigned; playback and transport are initialized; input devices are configured.

If the edit target does not yet exist, Tracktion writes its initial state.

### 9. Recreate edit-bound UI

`EditViewState`, `EditComponent`, `LowerRangeComponent`, `HeaderComponent`, `EditorContainer`, and `SidebarComponent` are reconstructed and connected to the replacement edit.

### 10. Reset startup bookkeeping

Initial setup should not be user-undoable, so undo history is cleared and the edit's changed status is reset.

## Saving and Save As

The Projects sidebar exposes **Save** and **Save As**. `Ctrl/Cmd+S` is registered as the normal save command.

`MainComponent::saveCurrentProject(bool saveAs)` delegates to `GUIHelpers::saveEdit()` with the configured projects directory.

A save chooser is required when:

- Save As is explicitly requested; or
- the current edit file is not a persistent `.tracktionedit` file, which includes a new project currently backed by `.nextTemp`.

On success:

1. `EditComponent::projectSaved()` stops/invalidates autosave work and removes recovery snapshots;
2. the window title is updated from the persistent project filename;
3. sidebar browsers are refreshed so the saved project appears.

Save results are `saved`, `cancelled`, or `failed` and are compatible with the lifecycle decision helper.

## Autosave

### Triggering dirty state

`EditComponent` listens for `te::IDs::lastSignificantChange`. Each significant change:

- increments `m_autoSaveGeneration`;
- sets `EditViewState::m_needAutoSave`;
- marks another write as queued if a write is already in progress.

A timer runs at `ApplicationViewState::m_autoSaveInterval` milliseconds and calls `saveTempFile()`. Construction also marks the new edit dirty and requests an initial recovery snapshot.

### Conditions that suppress autosave

Autosave returns without writing when:

- `m_isSavingLocked` is true;
- the transport is recording;
- no autosave is needed.

`ScopedSaveLock` is used around operations that should not be snapshotted in an intermediate state.

### Snapshot creation

The edit's `ValueTree` is copied on the message thread. Source paths in the copy are rewritten relative to the recovery target directory where possible. The copy, not the live edit state, is sent to the worker thread.

### Background write

A one-thread pool named `Autosave` serializes the copied tree. `juce::TemporaryFile` writes to a temporary path and atomically overwrites the target when complete. This avoids exposing a partially written recovery file.

Completion returns to the message thread via `MessageManager::callAsync()` and a `Component::SafePointer`.

### Generation counter

The generation identifies the model state represented by a snapshot.

- If the completed write still matches the current generation, `m_needAutoSave` can be cleared.
- If changes occurred during copying or writing, the dirty state remains and another save is queued.
- `projectSaved()` increments the generation, cancels queued work, waits briefly for jobs, and removes recovery files only after autosave has stopped.

This prevents an older background snapshot from incorrectly marking newer changes as saved.

## Recovery cleanup and shutdown

On clean `MainComponent` destruction:

- edit-bound UI and edit objects are destroyed in dependency order;
- application settings are saved;
- the engine temporary directory is removed.

If the process crashes, normal shutdown cleanup does not run, leaving a `.nextTemp` file that `openValidStartEdit()` can discover on the next launch.

## Tests

`App/tests/ProjectLifecycleTests.cpp` covers the pure lifecycle rules:

- unsaved-choice decision matrix;
- extension normalization and persistent/recovery distinction;
- save-target selection;
- request-state consumption and cancellation;
- rejection of missing and unsupported load requests;
- load inspection for missing, unsupported, empty, corrupt, wrong-root, XML, binary, and recovery files.

The GUI orchestration, Tracktion edit construction, and asynchronous autosave worker are not currently integration-tested.

## Invariants

Contributors changing project handling should preserve these invariants:

1. Cancelling a chooser never changes the current project.
2. Invalid input never destroys the current project.
3. Cancelling or failing an unsaved-project save never proceeds.
4. The replacement edit is constructed before the current edit is destroyed.
5. Objects referencing an edit are destroyed before that edit.
6. Recovery files are accepted only in an explicit recovery context.
7. Background autosave cannot clear dirty state for a newer generation.
8. A normal successful save removes obsolete recovery files.
9. View/setup bookkeeping does not pollute initial undo history.
10. Clean shutdown removes temporary recovery data; crashes leave recoverable data.

## Related documents

- [Architecture Overview](overview.md)
- [State and Event Model](state-and-events.md)
- [Testing](../development/testing.md)
- [Getting Started](../user/getting-started.md)
