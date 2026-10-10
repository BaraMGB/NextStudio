# Shared directory browser

- Type: reference
- Audience: contributors
- Scope: current Home/Projects navigation and domain integration

## Ownership and sources

`DirectoryBrowserComponent` provides asynchronous directory scanning, path/up navigation, back/forward history, natural name sorting, search, filtering and selection/activation/directory callbacks. It does not own/query an Engine/Edit, play previews or execute project operations. Domain policy belongs to its consumers.

Sources: `App/include/DirectoryBrowser.h`, `App/src/DirectoryBrowser.cpp`, `App/include/FileBrowser.h`, `App/src/FileBrowser.cpp`, `App/include/ProjectsBrowser.h`, `App/src/ProjectsBrowser.cpp`, and `App/src/SidebarComponent.cpp`.

The directory scanner has its own low-priority thread; destruction removes the directory listener and stops that thread. Callback owners must account for edit-bound UI replacement rather than retaining stale component pointers.

## Navigation versus activation

Directories remain navigable regardless of the file filter. Name sorting keeps directories first; a file predicate filters domain entries. Selection and activation are distinct: single selection can drive preview, while left-button double-click navigates a directory or invokes the file-activation callback. Drag-source description is configured by the consumer, not hardcoded as a project/engine operation.

The reusable browser retains/restores selection across asynchronous list updates where possible and owns its navigation history/search presentation. Changes here must not resurrect the removed recursive-project-list or separate Load-browser mode.

## Home integration

`FileBrowserComponent` is the Home configuration without a project-only filter. `SidebarComponent` forwards selection to the edit-aware `SamplePreviewComponent`, where audio format validation, preview edit and BPM synchronization belong. Home activation of a persistent project directly requests a typed `MainComponent::requestProjectOperation()`; there is no intermediate untyped `ProjectRequestState`/ChangeBroadcaster adapter.

Theme-file activation and audio drag sources retain their existing sidebar behavior. Directory navigation itself is not an audio preview or project replacement.

## Projects integration

`ProjectsBrowserComponent` combines the directory browser with New, Save, inline Save As, overwrite confirmation, errors and the typed [project workflow](../architecture/project-workflow.md). Project filtering uses the case-insensitive persistent-project predicate; recovery snapshots are not ordinary project entries. Normal browsing remains non-modal and has no Load button/state.

The current browser directory is the target folder for Save As. The persistent `m_projectLoadDir` property remains for settings compatibility and remembers navigation; a content-root reset assigns the new Projects folder. Initial directory resolution uses the remembered existing directory, then Projects, then working directory. There is no independent Save-directory source of truth.

[Project lifecycle](../architecture/project-lifecycle.md) owns extension/path normalization, load inspection, source-path rollback, autosave and recovery. The browser must not duplicate those rules. New targets have one canonical extension; direct saves preserve the exact existing path, including case.

## Validation and limitations

`ProjectLifecycleTests` cover filtering/extensions and exact path rollback; `ProjectWorkflowTests` cover typed continuation, failure/cancel cleanup, locking and stale deferred-execution guards. They do not constitute full directory-scanner/GUI integration tests.

Focused native validation should distinguish browsing from committed replacement, load through Projects/Home, invalid/deleted files, Save-As folder/name/overwrite/cancel, lock behavior and source-path preservation after failure. The [historical browser record](../archive/changes/embedded-project-file-browser.md) preserves original design/validation context; it is not needed to understand current navigation.

User controls are in [Side Browser](../ui/side-browser.md).
