# Wine/Bottles compatibility on Windows builds

This document explains the current compatibility layer used when a Windows build of NextStudio is executed under Wine, typically through Bottles on Linux.

It covers:

- the original failure mode;
- why JUCE 8 behaves differently under Wine;
- the runtime workaround currently implemented in NextStudio;
- the practical Bottles/Wine test setup used during validation;
- current limitations and open follow-up areas.

## Summary

NextStudio uses JUCE 8. On Windows, JUCE 8 prefers the **Direct2D** renderer for top-level windows. Under stock Wine this causes severe problems because the required Direct2D/DirectComposition path is only partially implemented.

The observed failures were:

- black or blank windows;
- unreadable or missing text;
- repeated `DxgiFactory::CreateSwapChainForComposition: Not implemented` messages;
- misleading hangs reported by the Bottles GUI even when the application process was still drawing.

The current NextStudio workaround is:

1. detect Wine at runtime;
2. switch JUCE desktop windows to the **Software Renderer** instead of Direct2D;
3. choose a real installed sans-serif font under Wine instead of JUCE's default Wine fallback family.

Native Windows Remote Desktop sessions keep JUCE's default Direct2D renderer. This path was validated successfully and does not require the Wine workaround.

This logic is implemented entirely in NextStudio and does **not** require patching JUCE or requiring a custom Wine build.

## Why this is necessary

### JUCE 8 renderer change

JUCE 8 changed the default Windows rendering backend to Direct2D. In the JUCE Windows peer implementation, top-level peers are created with renderer index `1`, which maps to Direct2D.

Relevant JUCE source:

- `modules/tracktion_engine/modules/juce/modules/juce_gui_basics/native/juce_Windowing_windows.cpp`

Important details from JUCE:

- renderer index `0` = `Software Renderer`
- renderer index `1` = `Direct2D`

### Wine limitation

Under stock Wine, the Direct2D/DirectComposition pipeline is incomplete for JUCE 8's windowing path. During investigation the typical runtime error was:

```text
DxgiFactory::CreateSwapChainForComposition: Not implemented
```

This error was reproducible with:

- the original upstream Windows package;
- Bottles + Wine 11 runtime;
- DXVK enabled;
- normal Windows launch and debug-shell launch.

Even when the main window eventually became visible, the application still depended on a renderer path that Wine did not fully support.

## Missing text under Wine

After forcing the software renderer, the black-window problem was solved, but text was still missing.

The reason was JUCE's own Wine-specific font fallback in DirectWrite handling. Under Wine, JUCE prefers these font family names:

- `Bitstream Vera Sans`
- `Bitstream Vera Serif`
- `Bitstream Vera Sans Mono`

Relevant JUCE source:

- `modules/tracktion_engine/modules/juce/modules/juce_graphics/native/juce_DirectWriteTypeface_windows.cpp`

In the tested Wine/Bottles prefix, those families were not present. As a result:

- layout was still created;
- controls, backgrounds, and buttons rendered;
- text glyphs were effectively unavailable or blank.

The fix in NextStudio is to select a font that actually exists in the Wine prefix.

## Current implementation in NextStudio

### Files

The implementation currently lives in:

- `App/include/WineRendererFallback.h`
- `App/src/WineRendererFallback.cpp`
- `App/src/Main.cpp`
- `App/src/MainComponent.cpp`

### 1. Runtime Wine detection

Wine is detected at runtime by checking whether `ntdll.dll` exports `wine_get_version`.

Implementation:

- `App/src/WineRendererFallback.cpp`

Conceptually:

```cpp
juce::DynamicLibrary ntdll("ntdll.dll");
return ntdll.getFunction("wine_get_version") != nullptr;
```

This keeps the behavior:

- Windows-native on real Windows;
- Wine-specific only when the app is actually executed under Wine.

No compile-time fork is required.

### 2. Force JUCE software rendering under Wine

`WineRendererFallback` switches JUCE peers to the `Software Renderer`.

The class is started from application startup in `App/src/Main.cpp`.

Current behavior:

- `NextStudioApplication` creates and owns a `NextStudio::WineRendererFallback` instance;
- `start()` enables the workaround when Wine is detected;
- setting `NEXTSTUDIO_FORCE_SOFTWARE_RENDERER=1` enables it manually for diagnosis;
- setting `NEXTSTUDIO_FORCE_DEFAULT_RENDERER=1` bypasses the fallback for comparison testing under Wine;
- the main `DocumentWindow` is switched before it is shown;
- the setup wizard is embedded in the software-rendered main-window peer and creates no separate modal peer;
- focus changes trigger asynchronous re-application so additional JUCE desktop windows are also corrected.

This was intentionally done as a **runtime adaptation layer** instead of patching JUCE internals.

### 3. Configure a usable UI font under Wine

`WineRendererFallback::configureFontFallback()` is called in `MainComponent` immediately after the default `LookAndFeel` is installed.

Current candidate order:

1. `Tahoma`
2. `Arial`
3. `Liberation Sans`
4. `DejaVu Sans`

The first available family found in `juce::Font::findAllTypefaceNames()` is installed as the default sans-serif family for the active `LookAndFeel`.

This is important because the software renderer only solves the rendering backend problem. Text still requires a usable font family.

### 4. Logging

The workaround emits log lines through the central logging system.

The fallback records only significant, one-time events at `info` level:

- software rendering was requested, including the activation reason;
- the software renderer was enabled successfully;
- the Wine font fallback was configured.

A missing software renderer or suitable fallback font is logged at `warn` level. Technical details such as the Wine DXGI guard are limited to `debug` logging. Renderer checks triggered by focus changes do not emit repeated messages.

Relevant source:

- `App/src/WineRendererFallback.cpp`
- `App/src/Logging.cpp`

## Validation environment

The implementation was validated with the following practical setup:

- Ubuntu 24.04
- Flatpak Bottles from Flathub
- Bottles CLI (`bottles-cli`)
- Bottles Wine 11 runtime (`sys-wine-11.0`)
- Windows x64 NSIS installer builds produced by GitHub Actions

Bottles app identifier:

- `com.usebottles.bottles`

Bottle name used in testing:

- `NextStudio`

## Bottles/Wine test workflow used during development

### Install Bottles

Example:

```bash
flatpak install --user flathub com.usebottles.bottles
```

### Create a bottle

Example:

```bash
mkdir -p "$HOME/.var/app/com.usebottles.bottles/data/bottles/bottles"
flatpak run --command=bottles-cli com.usebottles.bottles new \
  --bottle-name NextStudio \
  --environment application \
  --arch win64 \
  --runner sys-wine-11.0
```

### Install a Windows package into the bottle

Because the Flatpak sandbox cannot always read arbitrary host paths, the installer was copied into the Flatpak app data directory first and then executed from there.

Example:

```bash
install -m 644 NextStudio-0.04-win64.exe \
  "$HOME/.var/app/com.usebottles.bottles/data/NextStudio.exe"

flatpak run --command=bottles-cli com.usebottles.bottles run \
  -b NextStudio \
  -e "$HOME/.var/app/com.usebottles.bottles/data/NextStudio.exe" \
  /S
```

### Launch the installed application

CLI launch proved more reliable than launching through the Bottles GUI:

```bash
flatpak run --command=bottles-cli com.usebottles.bottles run \
  -b NextStudio \
  -p NextStudio
```

## What was observed during testing

### Before the workaround

Observed symptoms included:

- a completely black top-level window;
- repeated DXGI / DirectComposition errors;
- missing or broken first-run dialogs;
- unusable setup wizard.

### After forcing software rendering

Observed improvements:

- main window visible;
- dialogs visible;
- setup wizard visible;
- alert text and button labels visible once the font fallback was added.

### Bottles GUI false hang

A very important observation is that the Bottles GUI itself may report:

- "Bottles is not responding"

while the Windows application continues to render and accept interaction.

In testing this was verified by:

- capturing the X11 window contents;
- simulating clicks directly on the NextStudio window;
- seeing the UI continue to update after Bottles itself claimed a hang.

Therefore, a Bottles desktop-window freeze report is **not** sufficient evidence that NextStudio itself is frozen.

## Current limitations

### 1. Stock Wine still logs Direct2D/DXGI errors

Even with the software renderer fallback in place, Wine may still emit startup noise such as:

```text
DxgiFactory::CreateSwapChainForComposition: Not implemented
```

This appears during initialization of JUCE/Wine/driver paths, but the software fallback still allows the app to become usable.

So the presence of this log line alone does **not** mean the workaround failed.

### 2. Bottles desktop integration is noisy

Bottles may:

- surface its own responsiveness warning;
- conflate its management UI with the hosted Wine process;
- make debugging harder than direct CLI launching.

For debugging, `bottles-cli` is preferred.

### 3. Wine audio/MIDI integration still needs separate investigation

During testing there were also Wine-side warnings around:

- PulseAudio format negotiation;
- MIDI notification threads;
- Windows audio helper windows.

These are **separate** from the black-window / missing-font issue.

The current document only covers the renderer/font compatibility layer.

## Design rationale

### Why not patch JUCE directly?

A JUCE patch would be more invasive and harder to carry across JUCE updates.

The chosen approach keeps the workaround:

- local to NextStudio;
- runtime-only;
- easy to remove later if Wine support improves;
- easy to inspect in a small number of application files.

### Why not require patched Wine?

Patched Wine builds with better Direct2D/DirectComposition support do exist, but relying on them would:

- increase setup complexity for testers;
- make reproduction harder;
- move the burden from NextStudio to the user environment.

The current goal was to make the stock Bottles/Wine path as usable as possible.

## Future work

Possible next steps:

1. verify behavior on real Windows independently from Wine;
2. add an explicit regression test note for Wine/Bottles manual validation;
3. investigate whether the workaround should also be applied to plugin-related top-level windows under all paths;
4. investigate Wine-side audio/MIDI notification issues separately from rendering;
5. consider a dedicated user-facing troubleshooting document for Linux users running the Windows build through Bottles.

## Source index

Main implementation files:

- `App/include/WineRendererFallback.h`
- `App/src/WineRendererFallback.cpp`
- `App/src/Main.cpp`
- `App/src/MainComponent.cpp`

Relevant JUCE internals:

- `modules/tracktion_engine/modules/juce/modules/juce_gui_basics/native/juce_Windowing_windows.cpp`
- `modules/tracktion_engine/modules/juce/modules/juce_graphics/native/juce_DirectWriteTypeface_windows.cpp`
- `modules/tracktion_engine/modules/juce/modules/juce_core/native/juce_Threads_windows.cpp`

## Practical conclusion

For Wine/Bottles compatibility, the currently supported approach is:

- keep the Windows build unchanged for real Windows users;
- detect Wine dynamically at runtime;
- switch JUCE windows to the software renderer;
- override the default sans-serif font with an actually installed Wine font.

This is the minimum application-side workaround that made the Windows build visible and readable under the tested Bottles/Wine environment.
