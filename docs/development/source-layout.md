# Source Layout

## Repository map

```text
NextStudio/
├── App/
│   ├── include/          C++ headers
│   ├── src/              C++ implementations
│   ├── tests/            console/unit tests
│   ├── resources/        embedded icons, themes, and samples
│   └── CMakeLists.txt    application and test targets
├── docs/
│   ├── architecture/     system design and lifecycle
│   ├── components/       detailed component contracts
│   ├── development/      build, test, and contribution guidance
│   ├── user/             user-facing workflows and features
│   └── changes/          coherent implementation-change notes
├── language/             translation resources
├── modules/              Git submodules and third-party code
├── resources/            install/package resources
├── .github/workflows/    GitHub Actions
├── CMakeLists.txt        project, install, and CPack configuration
├── build.sh              configure/build helper
├── start.sh              build/run/debug helper
├── test.sh               build and CTest helper
├── fetch_submodules.sh   dependency initialization
├── CHANGELOG.md          release history
└── README.md             public project entry point
```

Generated directories such as `autobuild/`, `build/`, `dist/`, and `flatpak-build/` are not source areas.

## `App/include` and `App/src`

The source tree is intentionally flattened into header and implementation directories. Most features use matching names:

```text
App/include/PianoRollEditor.h
App/src/PianoRollEditor.cpp
```

Some header-only utilities and interfaces have no `.cpp`, and `Main.cpp` has no public header.

### Naming conventions

- Classes and files generally use `PascalCase`.
- Member variables use the `m_` prefix.
- Tracktion Engine is commonly aliased as `te`.
- Application-specific `juce::Identifier` constants live in `namespace IDs`.
- Components usually end in `Component`, `View`, `Editor`, `Bar`, `Browser`, or `Panel` according to role.
- Tool implementations end in `Tool` and implement `ToolStrategy`.

### Major source groups

#### Application and lifetime

- `Main.cpp`
- `MainComponent.*`
- `MainInteractionState.h`
- `ApplicationViewState.h`
- `EditViewState.*`
- `ProjectLifecycle.*`
- `InitialContentSetup.*`
- `SetupWizard.*`

#### Arrangement and tracks

- `EditComponent.*`
- `ClipPropertiesBar.*` / `ClipPropertyEdit.h`
- `SongEditorView.*`
- `TrackListView.*`
- `TrackHeadComponent.*`
- `TrackLaneComponent.*`
- `TrackHeightManager.*`
- `TimeLineComponent.*`
- `TimelineOverlayComponent.*`
- `PlayHeadComponent.*`
- `RecordingClipComponent.*`
- `AutomationLaneComponent.*`

#### Lower range

- `LowerRangeComponent.*`
- `LowerRangeTabBar.*`
- `MixerComponent.*`
- `MixerChannelStripComponent.*`
- `PluginChainView.*`
- `PianoRollEditor.*`

#### Piano Roll and MIDI tools

- `MidiViewport.*`
- `KeyboardView.*`
- `VelocityEditor.*`
- `NotePropertiesBar.*`
- `ToolStrategy.h`
- `ToolFactory.cpp`
- `PointerTool.*`
- `DrawTool.*`
- `EraserTool.*`
- `KnifeTool.*`
- `LassoTool.*`
- `RangeTool.*`
- `LassoSelectionTool.*`

#### Plug-in hosting and chain UI

- `PluginBrowser.*`
- `PluginScanner.*`
- `PluginWindow.*`
- `PluginChainView.*` — Track Chain orchestration, state listeners, ordering, selection, layout stages, and scrolling
- `PluginChainRackContent.*` — rack section sizing and component placement
- `PluginChainDragDrop.cpp` — drag-source classification and drop handling
- `PluginChainSections.h` — shared MIDI/audio section definitions
- `PluginChainLayout.h` — pure scroll and reorder calculations
- `PluginChainItemView.*` — individual plug-in editor wrapper
- `RackPluginListItem.*` — plug-in-list rows and row interactions
- `RackPanelToggleButton.*` — expanded/collapsed side-panel control
- `PluginComponent.*`
- `PluginMenu.*`
- `PluginViewComponent.*`
- `InstrumentEffectChooser.*`
- `PresetHelpers.*`
- `PresetManagerComponent.*`
- `PluginPresetInterface.h`

#### Built-in plug-ins

DSP/model classes and their UI components are separate where appropriate. Examples:

- `SimpleSynthPlugin.*` / `SimpleSynthPluginComponent.*`
- `SoundFontPlugin.*` / `SoundFontPluginComponent.*`
- `PeakLimiterPlugin.*` / `PeakLimiterPluginComponent.*`
- `NextDelayPlugin.*` / `DelayPluginComponent.*`
- `NextChorusPlugin.*` / `ChorusPluginComponent.*`
- `NextPhaserPlugin.*` / `PhaserPluginComponent.*`
- `NextFilterPlugin.*` / `FilterPluginComponent.*`
- `NextSaturationPlugin.*` / `SaturationPluginComponent.*`
- `SpectrumAnalyzerPlugin.*` / `SpectrumAnalyzerPluginComponent.*`
- `ArpeggiatorPlugin.*` / `ArpeggiatorPluginComponent.*`

#### Browsers and settings

- `SidebarComponent.*`
- `SidebarMenu.*`
- `Browser_Base.*`
- `ProjectsBrowser.*`
- `FileBrowser.*`
- `SampleBrowser.*`
- `PreviewComponent.*`
- `AudioMidiSettings.*`
- `RenderDialog.h`

#### Shared controls and helpers

- `ClipOverwriteCommand.*`
- `LowerRangeLayout.h`
- `Utilities.*`
- `PositionDisplayHelpers.*`
- `ThemeHelpers.*`
- `AutomatableSlider.*`
- `AutomatableToggle.*`
- `AutomatableComboBox.*`
- `AutomatableParameter.*`
- `AutomationWriteGuard.h`
- `ScopedSaveLock.h`
- `NonAutomatableParameter.*`

## CMake source discovery

Application `.cpp` files are globbed from `App/src/*.cpp` during configuration. This keeps the target list short but means CMake must reconfigure to notice newly created files. `build.sh` always runs the configure step before building.

Headers are not explicitly listed in the application target. They are resolved through `target_include_directories(${TargetName} PRIVATE include)`.

Test targets list their sources explicitly. New tests require a CMake edit. For example, `PluginChainLayoutTests` directly exercises the header-only `PluginChainLayout` calculations without constructing the full GUI or Tracktion engine.

## Resources

### Embedded application resources

`App/resources/CMakeLists.txt` creates binary-data targets.

#### Icons

SVG files are explicitly listed in `juce_add_binary_data(Pictures ...)`. To add an icon:

1. place the file under `App/resources/`;
2. add it to the `Pictures` source list;
3. rebuild so JUCE regenerates binary data;
4. access it through generated `BinaryData` symbols.

Generated symbol names are based on filenames and may normalize punctuation.

#### Themes

`App/resources/Themes/*.nxttheme` files are globbed into the `ThemePresets` binary-data target.

#### Samples

Bundled drum samples are under:

```text
App/resources/Samples/707/
App/resources/Samples/808/
App/resources/Samples/909/
```

Each family becomes a separate binary-data target. `InitialContentSetup` populates user content from embedded resources.

### Packaging resources

The root `resources/` directory contains installation artifacts such as:

- desktop entry;
- scalable and raster application icons;
- Windows icon resources.

These are referenced by root CMake install/CPack configuration and packaging manifests.

## Documentation layout

Documentation is intentionally outside `App/` because it covers the whole repository.

- `docs/user/` describes observable workflows and avoids unnecessary implementation details.
- `docs/components/` documents reusable or complex classes and their contracts.
- `docs/architecture/` documents boundaries, ownership, and cross-component behavior.
- `docs/development/` documents repository workflows.
- `docs/changes/` records the rationale and complete scope of coherent implementation changes.

Release notes belong in `CHANGELOG.md`, not in `docs/changes/`.

## Adding a new UI component

A typical component addition involves:

1. add `App/include/MyComponent.h`;
2. add `App/src/MyComponent.cpp`;
3. include it from its owner;
4. add it with `addAndMakeVisible()` or `addChildComponent()`;
5. assign bounds in `resized()`;
6. source colors from `ApplicationViewState`;
7. register model/selection listeners;
8. remove listeners in the destructor before dependencies disappear;
9. decide which state is persistent and where it belongs;
10. add tests for separable parsing/validation logic;
11. document non-trivial behavior under `docs/components/`.

Because application sources are globbed, no application target edit is normally needed for the `.cpp`; a CMake reconfigure is still required.

## Adding a Piano Roll tool

The tool subsystem uses `ToolStrategy`.

1. add an enum value to `Tool` in `Utilities.h`;
2. implement a class deriving from `ToolStrategy`;
3. implement mouse methods, cursor, activation/deactivation, and `getToolId()`;
4. add it to `ToolFactory::createTool()`;
5. add toolbar controls and icons in `PianoRollEditor` if user-selectable;
6. reset pending interaction state in `toolDeactivated()`;
7. group model mutations into undo transactions;
8. update [Piano Roll](../user/piano-roll.md).

## Adding a built-in plug-in

At a minimum:

1. implement the Tracktion plug-in class;
2. provide its persistent type ID and state handling;
3. add any UI component and parameter controls;
4. register the type with `m_engine.getPluginManager().createBuiltInType<T>()` in `MainComponent`;
5. ensure it appears in the correct instrument/effect browser category;
6. verify mono/stereo and sample-rate/block-size behavior;
7. document controls under `docs/user/` when user-facing;
8. add DSP or helper tests where practical.

## Adding persistent settings

### Application setting

For machine/user preferences:

1. declare an identifier in `ApplicationViewState.h`;
2. add a `CachedValue` member;
3. bind it to the correct child tree with a default;
4. ensure it is written through `saveState()`;
5. update any live listeners and settings UI;
6. document it if user-visible.

### Edit-local view setting

For state that should follow the project but not enter undo history:

1. declare an identifier in `EditViewState.h`;
2. add/bind a cached value in `EditViewState.cpp` using a null undo manager;
3. use the appropriate timeline-specific child for zoom/scroll data;
4. avoid treating it as a significant musical edit.

### Musical project state

Prefer Tracktion's model APIs and state schema. Use the edit undo manager for user operations and avoid duplicating model data in application settings.

## Shared utility caution

`Utilities.h/.cpp` contains broad helper namespaces and common enums. It is convenient but highly connected. New logic should go there only when genuinely shared. Feature-specific pure helpers, such as `PositionDisplayHelpers` or `ProjectLifecycle`, are easier to test and reason about than continuing to grow a monolithic utility file.

## Third-party code

Do not edit code under `modules/` for application behavior unless deliberately maintaining a submodule patch. Prefer adapting through public Tracktion/JUCE APIs. Submodule updates should be isolated and tested because they can affect every platform and audio path.

## Related documents

- [Architecture Overview](../architecture/overview.md)
- [State and Event Model](../architecture/state-and-events.md)
- [Building](building.md)
- [Testing](testing.md)
