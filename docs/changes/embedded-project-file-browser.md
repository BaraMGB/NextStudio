# Embedded Project File Browser and Save-As Interaction Boundary

## Summary

Project **Load** and **Save As** no longer create a `juce::FileChooserDialogBox` or any other project-specific top-level file window. Both workflows run inside `ProjectsBrowserComponent`, which already occupies the Projects section of the left sidebar.

The implementation has two deliberately different interaction policies:

- **Load** remains non-modal. The rest of NextStudio stays usable while the user browses for a project.
- **Save As** is application-modal in behavior, but not implemented with a JUCE modal loop or a new window. The rest of the main window is dimmed and blocked while the embedded Save-As browser remains active. The sidebar splitter is the sole exception so the user can widen the browser.

This avoids the Linux/X11 failure mode in which a modal `FileChooserDialogBox` could remain invisible while still blocking the application. It also keeps project paths, validation, overwrite confirmation, and errors inside the normal NextStudio component hierarchy.

## Source map

| Responsibility | Files |
|---|---|
| Embedded project-browser modes and controls | `App/include/ProjectsBrowser.h`, `App/src/ProjectsBrowser.cpp` |
| Sidebar placement, shell dimming, and outside-click dismissal | `App/include/SidebarComponent.h`, `App/src/SidebarComponent.cpp` |
| Main-window interaction blocker and splitter exception | `App/include/MainComponent.h`, `App/src/MainComponent.cpp` |
| Project request and file-validation helpers | `App/include/ProjectLifecycle.h`, `App/src/ProjectLifecycle.cpp` |
| Explicit save-to-file operation | `App/include/Utilities.h`, `App/src/Utilities.cpp` |
| Persisted Load and Save directories | `App/include/ApplicationViewState.h` |
| Debug-shell entry point | `App/include/DebugCommand.h`, `App/src/DebugProtocol.cpp`, `App/src/DebugAppController.cpp`, `App/src/MainComponentDebugHost.cpp` |
| Focused lifecycle tests | `App/tests/ProjectLifecycleTests.cpp`, `App/tests/DebugProtocolTests.cpp` |

## Motivation

The previous Load and Save-As implementations constructed a `juce::FileBrowserComponent`, placed it in a `juce::FileChooserDialogBox`, and synchronously called `show()`. On affected Linux/Cinnamon sessions, the dialog could fail to become visible while its modal state continued to block the main window.

That design also introduced broader constraints:

- an additional top-level window had to participate in focus and stacking;
- multi-monitor placement depended on the window manager;
- project workflow colors and controls did not naturally use the sidebar layout;
- selection, overwrite confirmation, and write errors were split across unrelated dialogs;
- save execution was coupled to target selection in `GUIHelpers::saveEdit()`.

The embedded browser removes the project file chooser entirely. Other unrelated file choosers, such as sample, preset, theme, and setup paths, are outside this change.

## User-visible workflows

## Normal Projects mode

The normal Projects view retains:

- **New**;
- **Load**;
- **Save**;
- **Save As**;
- project sorting;
- project search;
- the recursive list of projects under the configured Projects directory.

A double-click on a project enters the same guarded load path as selecting it in the embedded Load browser.

## Load

Pressing **Load** changes only the Projects sidebar content.

The view contains:

1. an `Open Project` title;
2. back, forward, and parent-folder navigation;
3. an editable current-path field;
4. an asynchronously populated directory list;
5. the complete selected project path;
6. an inline status or error message;
7. **Open** and **Cancel** buttons.

Directories and `.tracktionedit` files are shown. Unsupported files are omitted. Extension matching is case-insensitive.

**Open** is enabled only for an existing `.tracktionedit` file. A double-click on a directory navigates into it; a double-click on a valid project requests loading.

Load is intentionally non-modal:

- the arrangement, transport, lower range, and other sidebar sections are not covered by the Save-As blocker;
- `Escape` returns to normal Projects mode;
- a successful load replaces the edit and reconstructs the edit-bound UI;
- a failed load leaves the current edit and browser selection intact and shows an inline error.

## Save

A normal **Save** does not enter browser mode when the current edit already has a persistent `.tracktionedit` path.

The current path is passed directly to `MainComponent::saveCurrentProjectTo()`. On success:

- Tracktion writes the edit;
- changed status is reset;
- autosave state is cleared;
- obsolete recovery snapshots are cleaned through `EditComponent::projectSaved()`;
- the window title is updated;
- a newly created project file is added to the normal Projects list when it is inside the configured Projects directory.

If the current edit is still backed by a temporary `.nextTemp` file, **Save** enters the embedded Save-As workflow.

A direct-save failure switches to the Projects sidebar and displays the failing path inline.

## Save As

Pressing **Save As** opens the embedded `Save Project As` view. No operating-system or JUCE file dialog is created.

The view contains:

1. a title;
2. back, forward, and parent-folder navigation;
3. the current target directory;
4. folders and existing project files;
5. a `Project name` editor;
6. a preview of the complete normalized target path;
7. validation or operation status;
8. **Save** and **Cancel** buttons.

The suggested name is selected automatically and comes from, in order:

1. the current persistent project filename;
2. the edit name;
3. `Untitled` when no useful name exists.

The `.tracktionedit` extension is added automatically. Entering the extension manually does not duplicate it.

### Save-As interaction boundary

Save As is modal in behavior without using `runModalLoop()`, `enterModalState()`, or a top-level modal window.

While Save As, its overwrite confirmation, or a Save-As error is active:

- `ProjectSaveInteractionBlocker` covers the main component;
- the blocker paints a translucent black layer;
- mouse events over the arrangement, transport, lower range, and other main content are intercepted;
- global application commands are rejected by `MainComponent::perform()`;
- the computer-MIDI keyboard controller is detached from the main window and held notes are released;
- sidebar section buttons and the sidebar header/footer are dimmed and do not switch views;
- the embedded Save-As content remains above the blocker and fully usable.

This is an interaction lock, not a JUCE modal loop. The message thread continues normally.

### Splitter exception

The sidebar splitter is brought above the blocker between the blocker and the sidebar in component Z order:

```text
Projects sidebar
Sidebar splitter
Save-As interaction blocker
Arrangement and lower-range content
```

The splitter remains draggable so long paths and names can be given more horizontal space.

During Save As, splitter dragging only resizes the expanded sidebar. It cannot collapse the sidebar, because collapsing it would hide the only active Save-As controls. The usual collapse behavior resumes after Save As closes.

If the browser temporarily enlarged a narrow sidebar, the previous width is restored only when the user did not resize it manually. A user-selected width is preserved.

### Click-outside dismissal

A mouse press outside the embedded Save-As content cancels Save As and immediately removes the blocker.

The dismissal paths are:

- a click on `ProjectSaveInteractionBlocker` in the main content;
- a click on a dimmed sidebar section button;
- a click on the sidebar shell outside the project content;
- the explicit **Cancel** button;
- `Escape` while no nested text edit has higher priority.

The click that dismisses the blocker is consumed. It does not also activate the covered arrangement or transport control. Clicking or dragging the sidebar splitter is not treated as an outside click.

Cancellation changes neither the edit, its dirty flag, nor its current persistent path.

## Overwrite confirmation

Existence is checked after the project extension has been normalized. Consequently, `Song` and `Song.tracktionedit` resolve to the same target.

If the target exists, the Projects sidebar changes to an inline confirmation state containing:

- a warning;
- the complete target path;
- **Overwrite**;
- **Back**.

No alert window is created. **Back** returns to Save As with the directory and name unchanged. Only **Overwrite** authorizes writing to the existing path.

The Save-As blocker remains active throughout confirmation.

## Unsaved changes before Load

Load preserves the existing data-loss guard, but browser-originated requests avoid opening a second modal prompt.

When a load target is selected while the current edit is dirty, the sidebar shows an inline choice:

- **Save & Open**;
- **Discard & Open**;
- **Back**.

If the current edit has no persistent path, **Save & Open** temporarily enters Save As while retaining the pending load target. After a successful save, loading continues. Cancelling or failing Save As keeps the current edit active.

`ProjectRequest::unsavedChangesHandled` tells `MainComponent` that the browser has already established a clean edit or obtained an explicit discard decision. This prevents `setupEdit()` from opening the legacy unsaved-project dialog for the same browser request.

## Browser state model

`ProjectsBrowserComponent::Mode` defines these states:

| Mode | Purpose |
|---|---|
| `normal` | Project actions, sorting, search, and recursive project list |
| `loadProject` | Embedded directory navigation and load selection |
| `saveProjectAs` | Embedded directory navigation and project-name entry |
| `confirmOverwrite` | Inline authorization to replace an existing target |
| `operationError` | Inline load/save failure with path and return action |
| `confirmUnsavedChanges` | Inline save/discard/back decision before loading |

The principal transitions are:

```text
normal
  ├─ Load ───────────────→ loadProject
  └─ Save As / unsaved Save → saveProjectAs

loadProject
  ├─ Cancel / Escape ────→ normal
  ├─ dirty edit ─────────→ confirmUnsavedChanges
  ├─ successful load ────→ replacement UI / normal
  └─ load failure ───────→ operationError

confirmUnsavedChanges
  ├─ Back ───────────────→ loadProject
  ├─ Discard & Open ─────→ load request
  └─ Save & Open ────────→ direct Save or saveProjectAs

saveProjectAs
  ├─ outside click / Cancel / Escape → normal
  ├─ existing target ─────→ confirmOverwrite
  ├─ successful save ─────→ normal or pending load
  └─ save failure ─────────→ operationError

confirmOverwrite
  ├─ Back ───────────────→ saveProjectAs
  ├─ Overwrite success ──→ normal or pending load
  └─ overwrite failure ──→ operationError

operationError
  ├─ Back ───────────────→ originating mode
  └─ Close ──────────────→ normal
```

`isSaveAsWorkflowActive()` is true for Save As and all states that belong to its confirmation/error path. `MainComponent` and `SidebarComponent` use that query to maintain the interaction boundary.

## Filesystem rules

## Project names

`ProjectLifecycle::projectNameWithoutExtension()` trims the name and repeatedly removes a case-insensitive `.tracktionedit` suffix.

`ProjectLifecycle::isValidProjectName()` rejects:

- an empty or whitespace-only name;
- `.` and `..`;
- trailing dots or spaces;
- control characters;
- `< > : " / \\ | ? *`.

These rules provide one portable baseline across Linux, Windows, and macOS.

## Save targets

`ProjectLifecycle::isValidProjectTarget()` requires:

- a non-empty file;
- the persistent project extension;
- a valid filename;
- an existing parent directory with write access;
- write access to an existing target.

The UI performs this validation before enabling **Save**. `GUIHelpers::saveEditToFile()` validates again at the execution boundary.

## Load targets

`ProjectLifecycle::inspectLoadFile()` validates existence, extension, non-zero size, and an `EDIT` root/type for XML or binary `ValueTree` data. Recovery files are allowed only in an explicit recovery context.

## Last directories

`ApplicationViewState` persists separate paths:

- `ProjectLoadDIR` through `m_projectLoadDir`;
- `ProjectSaveDIR` through `m_projectSaveDir`.

Load and Save As therefore resume independently. Changing the application content root resets both paths to the new Projects directory.

## Asynchronous directory scanning

Embedded directory contents are read through:

- `juce::TimeSliceThread` named `Project directory scanner`;
- `juce::DirectoryContentsList`;
- change notifications back to `ProjectsBrowserComponent`.

The scanner enumerates the selected directory away from the message thread. The component filters the resulting entries to directories and supported project files before updating the `ListBox`.

Navigation itself never recursively descends into child directories, so symbolic-link cycles cannot create a recursive scan loop. A user may still navigate through links according to `juce::File` behavior.

## Separation of selection and execution

The old `GUIHelpers::saveEdit()` selected a path, asked for overwrite confirmation, changed relative paths, and wrote the edit in one function.

The new boundary is:

```cpp
ProjectSaveResult saveEditToFile(EditViewState&, const juce::File& targetFile);
```

Selection, filename entry, validation feedback, and overwrite confirmation belong to `ProjectsBrowserComponent`. Save execution belongs to `MainComponent` and `GUIHelpers::saveEditToFile()`.

This separation ensures that the low-level save function cannot accidentally create a file dialog.

## Save-As rollback and data safety

Before writing a new target, `saveEditToFile()` records the old edit name and current edit file.

For Save As:

1. relative paths are adjusted to the proposed target immediately before writing;
2. the edit name is changed to the normalized target name;
3. Tracktion writes the file;
4. on failure, relative paths are restored to the previous edit file and the old edit name is restored;
5. on success, changed status and autosave state are reset and source-file observers are notified.

An unconfirmed existing file is never intentionally passed to the save operation by the browser. Cancelling before the write has no model side effects.

## Load request and result flow

`ProjectsBrowserComponent` stores a typed `ProjectRequest` in `ProjectRequestState` and sends a change notification.

`MainComponent::changeListenerCallback()`:

1. consumes the request with `take()`;
2. captures safe pointers to the main component and source project browser;
3. posts the load through `MessageManager::callAsync()`;
4. calls `setupEdit()` with an error-message output;
5. reports a failure back to the still-existing browser.

On success, `setupEdit()` replaces the edit-bound component hierarchy, including the source browser itself. A `SafePointer` prevents any callback into the destroyed browser.

The replacement edit is still validated and constructed before the current edit is destroyed. The embedded UI changes target selection, not the defensive replacement order.

## Error presentation

Project load/save errors are shown in `operationError` mode rather than an `AlertWindow`.

Messages include:

- the failed action;
- a human-readable reason when known;
- the complete affected path.

Correctable state is retained. Returning from an error restores the originating Load, Save As, or overwrite-confirmation mode.

## Keyboard and focus

- Save As focuses the project-name editor asynchronously and selects the suggested name.
- `Enter` invokes the currently valid primary action: Open, Save, or Overwrite.
- `Escape` cancels the active embedded mode.
- The text editor owns text-editing keys, so Backspace does not trigger folder navigation.
- The Save-As interaction blocker rejects global commands while active.
- The computer-MIDI keyboard listener is detached from the main window during Save As so typing a project name cannot trigger performance notes.
- Closing Save As reattaches the listener.

## Debug-shell support

The debug shell provides:

```text
project-save-as
```

The command enters the embedded Save-As state through `MainComponentDebugHost` and returns:

```json
{"projectBrowserMode":"saveProjectAs"}
```

It is intended for deterministic UI-state setup before a `screenshot` command. The debug command does not save a file.

Relevant files:

- `App/include/DebugCommand.h`;
- `App/src/DebugProtocol.cpp`;
- `App/src/DebugAppController.cpp`;
- `App/include/DebugHost.h`;
- `App/include/MainComponentDebugHost.h`;
- `App/src/MainComponentDebugHost.cpp`.

## Tests and validation

`App/tests/ProjectLifecycleTests.cpp` covers:

- extension normalization;
- repeated extension removal;
- case-insensitive persistent-project recognition;
- valid and invalid project names;
- writable target validation;
- persistent versus temporary save-target decisions;
- request clearing and typed unsaved-change handling;
- load inspection for missing, unsupported, empty, corrupt, XML, binary, and recovery files.

`App/tests/DebugProtocolTests.cpp` verifies parsing of `project-save-as`.

The implementation was validated with:

```bash
BUILD_JOBS=12 ./build.sh rd
ctest --test-dir autobuild/RelWithDebInfo --output-on-failure
```

The visual Save-As state was also opened through the debug shell and captured with `screenshot`. The snapshot verifies that:

- Save As remains bright and usable in the sidebar;
- the sidebar navigation and main content are dimmed;
- the splitter remains above the dimming layer;
- no additional top-level project file window is visible.

## Invariants

Future changes should preserve these rules:

1. Project Load and Save As do not construct `FileChooserDialogBox`.
2. No project file-selection path enters a JUCE modal loop.
3. Load remains non-modal.
4. Save As blocks main-window interaction without blocking the message thread.
5. The sidebar splitter remains usable during Save As and cannot collapse the active browser.
6. An outside click cancels Save As but does not activate the covered control.
7. Existing project files require explicit inline overwrite confirmation.
8. Save-target normalization occurs before existence checking.
9. A failed Save As restores the previous edit name, edit path context, and relative paths.
10. A failed load never destroys the current edit.
11. Browser-originated unsaved-change decisions are not followed by a second modal prompt.
12. Direct Save to an existing persistent path does not open Save As.

## Related documents

- [Project lifecycle](../architecture/project-lifecycle.md)
- [Side Browser](../ui/side-browser.md)
- [State and event model](../architecture/state-and-events.md)
- [Agent debug system](../agent-debug.md)
- [Testing](../development/testing.md)
