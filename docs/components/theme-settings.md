# Theme settings

## Purpose

The General settings tab contains a theme editor designed for the narrow sidebar. It replaces JUCE's `ColourSelector` and the modal preset file workflow with controls embedded in the existing General settings viewport.

## Components

| Component | Responsibility |
|---|---|
| `ThemeSettingsComponent` | Quick presets, grouped color rows, inline save operations, and `ThemeState` synchronization |
| `ThemePresetBrowser` | Searchable, fixed-height, virtualized list of built-in and custom presets |
| `InlineColourEditor` | Saturation/value selection, hue, `#RRGGBB` input, current/reference previews, and per-color reset |
| `ThemePresetModel` | Preset discovery, validation, comparison, direct file loading, atomic saving, and built-in overwrite protection |

`GeneralSettings` owns `ThemeSettingsComponent`. Its viewport scrolls the complete form, while the fixed-height preset browser scrolls only its virtualized result list.

## Color editing

The color definitions map the existing `ThemeState` property identifiers to user-facing names and four groups: General, Controls, Timeline, and Tracks. Selecting a row places the editor directly below it. The parent viewport is adjusted only when necessary to keep the editor visible.

The editor keeps hue, saturation, and brightness separately while interacting. The hex field uses conventional `#RRGGBB`. Theme colors are forced to full opacity when settings or presets are loaded and when values are saved. Storage remains JUCE's ARGB string returned by `Colour::toString()`, with `ff` as the alpha byte, so existing `.nxttheme` files remain compatible and older translucent values are migrated safely. The obsolete, unused `BackgroundColour3` property is discarded when settings and presets are loaded.

Changes are written directly to `ApplicationViewState::ThemeState` and are reflected immediately throughout the application. A coalesced asynchronous listener updates the editor and preset dirty state without recursive callbacks.

## Mouse wheel and keyboard behavior

No theme control uses the mouse wheel to change a value. The preset list uses the wheel for browsing and forwards it to the General settings viewport when it reaches a boundary. The search and hex editors forward wheel input directly to the General settings viewport.

Color rows can be traversed with the arrow keys. The inline color area supports arrow-key adjustment, and Return focuses the hex field. Standard keyboard focus remains available for preset buttons, text fields, and inline actions.

## Presets

Built-in themes remain available as quick buttons. All valid `.nxttheme` files appear in a searchable, fixed-height browser grouped into built-in and custom themes. The list is virtualized, so the form height stays constant even with many presets. Clicking a preset applies it immediately. Built-in presets cannot be overwritten.

Custom presets support direct Save and inline Save As with inline overwrite confirmation. A `.nxttheme` file elsewhere on disk can be applied by double-clicking it in the Home browser and then stored in the preset list with Save As.

`ThemePresetModel` writes through `juce::TemporaryFile`, ensuring that an interrupted save does not leave a partially written preset.

## Relevant files

- `App/include/ThemeSettingsComponent.h`
- `App/src/ThemeSettingsComponent.cpp`
- `App/include/ThemePresetBrowser.h`
- `App/src/ThemePresetBrowser.cpp`
- `App/include/InlineColourEditor.h`
- `App/src/InlineColourEditor.cpp`
- `App/include/ThemePresetModel.h`
- `App/src/ThemePresetModel.cpp`
- `App/include/AudioMidiSettings.h`
- `App/tests/ThemePresetModelTests.cpp`
