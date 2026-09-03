# Project Workflow Controller and Interaction Lock

## Purpose

Project file selection remains embedded in the Projects sidebar, but the sidebar view is no longer responsible for representing asynchronous project intent with unrelated booleans. `ProjectWorkflow::Controller` owns the state machine and pending operation. `MainComponent` owns execution and the interaction/engine boundary.

Primary files:

- `App/include/ProjectWorkflow.h`, `App/src/ProjectWorkflow.cpp`
- `App/include/ProjectsBrowser.h`, `App/src/ProjectsBrowser.cpp`
- `App/include/MainComponent.h`, `App/src/MainComponent.cpp`
- `App/include/PluginWindow.h`, `App/src/PluginWindow.cpp`

## State model

`ProjectWorkflow::State` contains:

- `normal`
- `saveProjectAs`
- `confirmOverwrite`
- `saving`
- `committing`
- `operationError`
- `confirmUnsavedChanges`
- `confirmRecovery`

The controller also stores one typed `ProjectWorkflow::Operation`:

- `createNew`
- `load`, with a project file
- `quit`

This pending operation survives a required Save As. A successful save transitions to `committing` and returns the operation to execute. Cancel and failed writes clear both pending intent and continuation. After a failed Save As, returning to the filename form permits a standalone retry but can no longer execute the abandoned New, Load, or Quit operation. There is no `unsavedChangesHandled` snapshot and no separate `resumeLoadAfterSave` flag.

## Operation flow

All user-facing replacement operations enter through `MainComponent::requestProjectOperation()` and are displayed by `ProjectsBrowserComponent::beginProjectOperation()`.

This includes:

- New Project;
- loading by activating a project in the normal Projects directory browser;
- loading from Home or drag-and-drop;
- application quit.

If the edit is dirty, the sidebar presents Save and Continue, Discard and Continue, and Back. Save without a persistent path enters Save As while retaining the typed pending operation. After a successful direct save or Save As, the pending operation continues automatically.

At startup, a valid crash snapshot is loaded and `confirmRecovery` presents Restore Project and Discard Recovery in the same Projects sidebar. This state locks all other main interaction and cannot be dismissed with Escape or an outside click. Resolution is posted asynchronously because discarding reconstructs the edit-bound sidebar. Restoring marks the edit dirty and retains its snapshot across shutdown until the user saves, explicitly discards, or replaces it. A required Setup Wizard is shown only after this choice.

The browser invokes a typed operation callback rather than sending an untyped load request through `ChangeBroadcaster`. Actual replacement remains asynchronous so the browser callback may return before its edit-bound component hierarchy is destroyed. `ExecutionGuard` captures edit identity and `lastSignificantChange`; either changing before the deferred callback rejects replacement.

## Interaction and engine boundary

`ProjectWorkflow::Controller::locksMainInteraction()` is true during unsaved-change or recovery confirmation, Save As, errors belonging to these embedded workflow paths, `saving`, and `committing`.

`MainComponent::setProjectWorkflowActive()` updates the project source in `MainInteractionState`; `MainComponent::updateMainInteractionLock()` is the shared project/setup enforcement boundary. On entry it:

1. stops the current transport;
2. sends Tracktion MIDI panic;
3. frees the edit playback context;
4. disables `EditorContainer` and `LowerRangeComponent` recursively;
5. disables all registered `PluginWindow` instances;
6. detaches the computer MIDI keyboard from the main and plugin windows;
7. disables application commands;
8. shows the dimming/click-catcher overlay.

On exit it allocates the playback context again, enables the component trees and plugin windows, restores keyboard routing, and hides the overlay. Playback that was active before a cancelled or completed save resumes at the captured position. Recording never resumes automatically, and successful edit replacement suppresses playback restoration for the old edit.

`ProjectWorkflowOverlay` is not the security boundary. It is shared with the embedded startup wizard, supplies visual dimming, and prevents an outside click from reaching covered controls. Effective component disabling, command status, plugin-window registration, MIDI detachment, and playback-context lifetime provide the lock. The debug shell observes the project-workflow source of the same runtime boundary: read-only diagnostics, screenshots, `stop`, and harness shutdown remain available, while transport-start and model-mutating commands return `busy`.

The Projects sidebar and splitter remain above the overlay. Sidebar command handling rejects navigation while interaction is locked. During cancellable Save As, an outside click cancels; during `saving` or `committing`, cancellation controls are disabled.

## Load behavior

The normal Projects view is always a filtered directory browser, so loading needs neither a Load button nor a separate browser state. Directory browsing remains non-modal and does not stop the engine. Double-clicking a project stages a typed load operation. The lock begins only when the operation commits after the clean, Save and Continue, or Discard and Continue decision. This closes the previous race in which a boolean unsaved-change decision could become stale before asynchronous setup.

The Projects and Home views use `DirectoryBrowserComponent` for asynchronous scanning, navigation, sorting and filtering. Domain behavior is callback-based: Projects activates only persistent project files, while Home forwards audio selections to `SamplePreviewComponent`. The directory browser itself has no Engine, Edit or preview dependency.

`MainComponent::setupEdit()` no longer opens a modal unsaved-project alert. It assumes every runtime replacement has passed through the workflow controller. The replacement edit is still inspected and constructed before the current edit is destroyed.

## Save target behavior

`ProjectLifecycle::normaliseSaveTarget()` distinguishes direct save from a newly chosen target. A direct save preserves the exact current path, including extension case. This prevents `Song.TRACKTIONEDIT` from becoming a second `Song.tracktionedit` file on case-sensitive file systems.

New targets continue to receive one canonical `.tracktionedit` extension before validation and overwrite checking.

## Plugin windows

`PluginWindow` maintains a registry of live instances. `PluginWindow::setAllInteractionEnabled()` applies workflow state to existing windows, while newly constructed windows inherit the current global interaction state. MIDI keyboard listeners are detached while a plugin window is disabled.

## Tests

`ProjectWorkflowTests` covers:

- immediate clean-operation commit;
- discard continuation;
- Save-As continuation;
- cancellation and failed writes clearing pending intent;
- standalone retry after a failed Save As;
- stale edit identity or significant-change rejection;
- recovery-confirmation interaction locking;
- error return state and interaction-lock behavior.

`ProjectLifecycleTests` additionally verifies that direct save target normalization preserves an existing upper-case extension path.
