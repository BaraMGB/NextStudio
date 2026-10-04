# Complete plug-in bypass presentation

## Root cause

`PluginChainItemView::paint()` previously darkened only its own vertical header when `Plugin::isEnabled()` was false. Embedded editors, graph components, preset panels, and rotary controls derived colours independently. `NextLookAndFeel` also reads the parameter's owning track colour directly, so changing just `PluginViewComponent::getTrackColour()` would not cover the complete GUI.

A baseline debug-shell reproduction with Next Delay confirmed `enabled: false` while its graph header, tap markers, and knob arcs remained coloured.

## Approved visual and interaction policy

Bypass is a signal-processing state, not a control lock. The maintainer requested an unambiguous visual indication and screenshot verification while retaining usable controls.

A bypassed rack item now combines:

- full grayscale rendering of the vertical rail, embedded editor, graphs, knobs, instrument preset panel, and editor-open button;
- a high-contrast **BYPASSED** badge in the vertical rail, including when the item is collapsed;
- the existing crossed-out eye indicator in the plug-in list.

The badge remains meaningful even for an already monochrome theme or track colour. It occupies a separate part of the rail below the name, leaving the editor-open button clear. No editor controls are covered or moved. There is no extra opacity reduction, control disabling, or audio/parameter-state change. Modifiers and separately opened native plug-in windows are outside this rack-item treatment.

## Ownership and rendering

`PluginBypassPresentation` observes the plug-in's `enabled` ValueTree property, with the same default of true as the engine. It reads the property directly to avoid notification ordering relative to Tracktion's CachedValue. Initial bypass, enable/disable, and undo/redo update the effect attachment immediately. Parameter-state changes do not trigger extra bypass work.

Only bypassed items receive `PluginBypassEffect` through `Component::setComponentEffect()`. JUCE renders the component and all of its children into one offscreen image at physical pixel resolution, then the filter desaturates a private copy and composites it using the existing alpha. Shared editor images and source colours are never modified. Active items have no component effect or offscreen-effect cost; removing the effect restores their original rendering.

`PluginChainItemView` owns the presentation object and explicitly destroys it before the child editors and base Component. The presentation unregisters its listener and detaches its owned filter. Modifier items never create a presentation object. Existing per-editor rendering and control behavior remain unchanged.

`getHeaderAreas()` reserves the editor-open button's top area and calculates disjoint name/status rectangles inside the rail. `paintHeader()` draws a dark badge with light lettering independently of the track colour. The old header-only darkening is replaced by the uniform grayscale treatment.

## Regression coverage

`PluginBypassPresentationTests` checks RGB/ARGB desaturation, source and alpha preservation, whole parent/child/grandchild rendering, default active and initially bypassed states, undo/redo transitions, exact restored colours, listener/filter teardown, unchanged state values and control enablement/hit testing, and badge bounds/contrast. The same controller is exercised for all three plug-in roles without type-specific branches.

## Screenshot and runtime verification

An isolated debug-shell session verified active, bypassed, and re-enabled screenshots for:

- Next Delay (audio effect);
- Arpeggiator (MIDI effect);
- SoundFont Player (instrument), including its preset panel;
- Simple Synth (large instrument), including a separate view of its trailing envelope/voice/master controls after horizontal scrolling.

Pixel checks found no coloured pixels in the inspected opaque bypassed editor interiors. The corresponding active/re-enabled interiors were pixel-identical for all four editors. Transparent antialiased outer corners are excluded from this check because preserving alpha intentionally allows the surrounding rack background to show through.

The badge was visually reviewed at the normal rack height and in the collapsed Delay rail. A physical knob drag while Delay remained bypassed changed Feedback from 0.35 to approximately 0.388, confirmed by a state dump. Track switching, bypass toggling, scrolling, and large-instrument screenshot rendering remained responsive in this session; this is focused runtime review, not a formal frame-time benchmark.

Screenshot files are preserved under `/tmp/NextStudio-bypass-validation/`. The active/bypassed/re-enabled contact sheet is supplied as `/home/ai/Gemeinsam/NextStudio-bypass-review.png` alongside the shared application artifact. The maintainer accepted the final appearance and explicitly approved commit, push, and marking #65 fixed.

Validation commands: `BUILD_JOBS=12 ./build.sh rd`, `BUILD_JOBS=12 ./test.sh rd` (all 23 tests pass), and `./build_and_copy_shared.sh`.

User behavior: [Track Chain](../ui/track-chain.md).
