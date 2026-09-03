# Embedded Startup Wizard

## Scope

Initial setup is part of the main-window component hierarchy. `SetupWizard` is not launched through a `DialogWindow` and does not enter a JUCE modal loop.

Primary implementation files:

- `App/include/SetupWizard.h`, `App/src/SetupWizard.cpp`
- `App/include/MainComponent.h`, `App/src/MainComponent.cpp`
- `App/include/MainInteractionState.h`

## Placement and layout

`MainComponent` creates the normal application hierarchy and places a dedicated `Viewport` over the bounds occupied by `EditComponent`. The wizard is the viewport's viewed component. `MainComponent::resized()` converts the nested edit bounds into main-component coordinates and updates the viewport whenever the window layout changes.

The wizard uses two columns when enough width is available and a single vertical column in narrow editor areas. Its minimum content height is larger than the viewport, so all setup sections remain reachable through vertical scrolling without changing the surrounding main-window layout.

## Interaction boundary

The existing full-window dimming and click-catching overlay is shared by startup setup and project workflows. During setup the overlay is moved above the normal application hierarchy, then the wizard viewport is moved above the overlay. As a result, only the wizard remains visually undimmed and mouse-accessible.

`MainInteractionState` combines the independent setup-wizard and project-workflow locks. While either lock is active, `MainComponent`:

1. stops transport and sends MIDI panic;
2. frees the playback context;
3. disables the editor and lower-range component trees;
4. disables plug-in windows;
5. detaches computer-keyboard MIDI routing;
6. disables application commands;
7. shows the interaction overlay.

Setup additionally disables the sidebar and its splitter. Project workflows keep those components available because Save As and unsaved-change controls live in the Projects sidebar. The setup wizard has foreground priority if both lock sources are ever active.

## Completion

`SetupWizard` persists the selected root, theme, scale, plug-in configuration, and audio/MIDI settings, then invokes `onFinished`. The callback posts `MainComponent::completeSetupWizard()` asynchronously so the wizard is not destroyed from inside its own button callback.

Completion removes the viewport content, refreshes content directories and browsers, reapplies theme-dependent UI state, and finally releases the startup interaction lock.

Closing the main window before completion quits directly and leaves `m_setupComplete` false. The wizard therefore appears again on the next launch.

## Non-modal support actions

Folder selection uses `FileChooser::launchAsync()`. Invalid or unwritable roots are reported by a status label inside the wizard rather than an `AlertWindow`. The wizard itself contains no `DialogWindow`, `enterModalState()`, or `runModalLoop()` path.

## Recovery ordering

Crash recovery is resolved in the Projects sidebar before the embedded Setup Wizard is shown. The choice uses Restore Project and Discard Recovery buttons rather than a modal `AlertWindow`. This ordering is intentional: a recovery snapshot must always be offered on the first launch after a crash, even when setup is incomplete or the configured content root disappeared. Deferring the choice until wizard completion is unsafe because closing the application during setup performs normal shutdown cleanup and could otherwise remove a snapshot that was never offered.

## Tests

`MainInteractionStateTests` verifies that setup independently locks the main UI, releases cleanly, and takes foreground priority over a project workflow. Full GUI behavior is additionally validated through a first launch with isolated application settings.
