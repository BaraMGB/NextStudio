# PluginChainView

## Purpose

`PluginChainView` implements the Track Chain shown by the Plugins tab in the Lower Zone. It coordinates the selected track, track presets, modifiers, the plug-in list, horizontally arranged plug-in editors, the channel strip, drag-and-drop, selection, ordering, and scrolling.

User-facing behavior is documented in [Track Chain](../ui/track-chain.md).

## Component structure

```text
PluginChainView
├── track name rail
├── RackPanelToggleButton (Track Presets)
├── PresetManagerComponent
├── RackPanelToggleButton (Modifiers)
├── ModifierSidebar
├── ModifierDetailPanel
├── plugin-list panel
│   ├── juce::Viewport
│   └── RackPluginListItem entries
├── plug-in canvas
│   └── RackContentComponent
│       ├── section frames and add buttons
│       └── PluginChainItemView entries
├── horizontal scrollbar
└── MixerChannelStripComponent
```

The Track Presets and Modifiers panels are collapsed by default. Each keeps a narrow labeled rail for restoring the panel. The controls use the same arrow resources as track headers. Collapsing Modifiers also hides its detail panel without clearing the selected modifier.

## Source responsibilities

| File | Responsibility |
|---|---|
| `App/include/PluginChainView.h`, `App/src/PluginChainView.cpp` | orchestration, selected-track listeners, rack order, rebuilding, selection, layout, and scrolling |
| `App/include/PluginChainRackContent.h`, `App/src/PluginChainRackContent.cpp` | section sizing, plug-in-editor placement, add-button placement, and plug-in-list panel frame |
| `App/src/PluginChainDragDrop.cpp` | typed drag-source classification and plug-in/file drop handling |
| `App/include/RackPluginListItem.h`, `App/src/RackPluginListItem.cpp` | plug-in-list rows, hit testing, menus, row reordering, and cached icons |
| `App/include/RackPanelToggleButton.h`, `App/src/RackPanelToggleButton.cpp` | expanded-header and collapsed-rail controls |
| `App/include/PluginChainSections.h` | MIDI/audio section definitions |
| `App/include/PluginBypassPresentation.h`, `App/src/PluginBypassPresentation.cpp` | renderer-only whole-item grayscale, enabled-state listener, and contrasting vertical bypass badge |
| `App/include/PluginChainLayout.h` | pure scroll-range and reorder-index calculations |
| `App/include/PluginMenu.h`, `App/src/PluginMenu.cpp` | shared plug-in tree construction and deterministic menu ordering |

## State ownership

### Application settings

The following `ApplicationViewState` values live under `AppSettings/Behavior`:

- `TrackPresetPanelCollapsed`;
- `ModifierPanelCollapsed`.

They are application-wide workspace preferences, not musical project data. They apply when switching tracks and persist through `AppSettings.xml`. Both default to `true`.

### Edit-local rack state

`EditViewState::trackPluginChainViewState` stores state associated with the current edit and track, including:

- visual rack-item order;
- collapsed individual plug-in editors;
- selected modifier state.

These view changes do not enter musical undo history.

### Transient state

The current horizontal offset, animated scroll target, selected rack item, drag hover, and asynchronous rebuild flags remain component state.

## Layout flow

`PluginChainView::resized()` delegates to three focused stages:

1. `layoutSidePanels()` lays out the optional preset panel, modifier stack/detail, and collapsed rails;
2. `layoutPluginList()` lays out the fixed-width list and its vertical viewport;
3. `layoutRack()` assigns the remaining width to the plug-in canvas and horizontal scrollbar.

Expanded side panels use named constants rather than local numeric widths. A collapsed side panel occupies only `COLLAPSED_PANEL_WIDTH`, returning the remaining space to the plug-in list and canvas.

### Compact Pitch Shifter editor

`PluginChainItemView` selects `PitchShiftPluginComponent` for `pitchShifter` instead of the generic VST parameter list. It requests width factor 1 and displays an interval map above one standard Semitones control. Dragging the map's output point selects whole semitones in one undoable gesture; the standard knob and automation remain continuous. `EffectEditorLayout::pitchShifter()` bounds the map/control dimensions; the shared rack height is unchanged. See [Compact Pitch Shifter editor](../changes/pitch-shifter-compact-map.md) for parameter-listener ownership and regression coverage, and [Pitch Shifter](../plugins/pitch-shifter.md) for user behavior.

### Whole-item bypass presentation

Each plug-in `PluginChainItemView` owns a `PluginBypassPresentation` observing its `enabled` state. While bypassed, a component image effect desaturates the entire item and its children, including preset controls. A dedicated **BYPASSED** badge occupies the lower vertical rail without covering or moving editor controls. Active/re-enabled items have no filter; modifier items are unchanged. The view detaches the observer/effect before destruction.

Controls remain enabled and interactive; the presentation neither edits engine state nor changes audio processing. See [Complete bypass presentation](../changes/complete-bypass-presentation.md) for rendering ownership, alpha handling, tests, and screenshot verification.

## Rack sections and ordering

MIDI tracks expose these sections in signal-flow order:

1. MIDI Plugins;
2. Instrument;
3. Audio Effects.

Audio tracks expose Audio Effects only. `PluginChainSections.h` is the shared source for both canvas and list section definitions.

The persisted rack order contains stable item IDs. Hidden channel-strip tail plug-ins are excluded from the visible order. Conversion between visible plug-in ordinals, visual rack indices, and Tracktion plug-in-list indices is kept explicit because modifiers and hidden plug-ins occupy different domains.

## Plug-in selection menu ordering

After built-in and external plug-ins have been assembled, `PluginTreeBase::sortSubItemsRecursively()` sorts every group by its displayed name. The comparison is case-insensitive and natural, and equivalent names retain registration order. Sorting the shared tree once preserves the category hierarchy and gives both the unfiltered `PluginMenu` and the role-filtered Track Chain add menus the same deterministic category and entry order. Plug-in identifiers, role classification, and creation remain unchanged.

## Scrolling

The plug-in canvas uses a horizontal content offset and explicit scrollbar. The maximum offset is based on full content width:

```text
max(0, content width - visible canvas width)
```

This guarantees that the complete final plug-in GUI remains reachable on narrow windows. Selection can animate toward an item's left edge; wheel and scrollbar movement stop the animation and clamp the new position.

## Drag-and-drop

`PluginChainDragDrop.cpp` maps external descriptions to a single internal drag-kind enum. Supported sources include:

- plug-in browser entries;
- instrument/effect chooser entries;
- SoundFont files;
- existing plug-in components;
- automation/modifier drags where applicable.

Plug-in creation from browser sources and insertion feedback are centralized so the rack background and add-button paths do not maintain duplicate insertion logic.

## Refresh model

`PluginChainView` observes the selected track, its plug-in list, and its edit-local rack state. `FlaggedAsyncUpdater` separates:

- structural plug-in reconstruction;
- layout-only refreshes.

Callbacks that may outlive a synchronous rebuild use `juce::Component::SafePointer` before touching the component.

## Tests

`PluginChainLayoutTests` covers the pure scroll-range and reorder-destination calculations. `PluginMenuOrderingTests` covers recursive category/entry ordering, case-insensitive natural comparison, stable equivalent names, category preservation, and filtered-order equivalence. `DebugSettingsIsolationTests` covers defaults and persistence for the two side-panel settings. Full component rendering, drag-and-drop, and Tracktion plug-in insertion still require integration or visual testing.

## Related documents

- [Track Chain](../ui/track-chain.md)
- [State and Event Model](../architecture/state-and-events.md)
- [Source Layout](../development/source-layout.md)
- [Testing](../development/testing.md)
