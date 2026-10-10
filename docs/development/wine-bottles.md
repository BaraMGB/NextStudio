# Windows builds under Wine/Bottles

- Type: procedure/reference
- Audience: contributors testing compatibility
- Scope: current application-side renderer/font fallback and repeatable manual validation

## Purpose and prerequisites

The Windows build detects Wine at runtime and adapts JUCE desktop rendering/font selection. This does not make Wine a fully certified audio/MIDI/plugin platform. Original Ubuntu/Bottles/Wine observations are in the [historical validation record](../archive/changes/wine-bottles-validation.md), not mixed into current setup instructions.

To validate: obtain a Windows x64 package for the revision being tested, install Bottles (or a documented Wine runner), use a separate test bottle/project and note runner, graphics options and available fonts. Check the installed CLI help before using examples; runner names and registered program names depend on that installation.

## Current compatibility contract

Sources: `App/include/WineRendererFallback.h`, `App/src/WineRendererFallback.cpp`, ownership/startup in `App/src/Main.cpp`, and font setup in `App/src/MainComponent.cpp`. The implementation is local to NextStudio; no JUCE patch or custom Wine build is required.

### Detection and renderer overrides

Windows detection checks `ntdll.dll` for `wine_get_version`; non-Windows detection returns false. The renderer fallback activates under Wine or an explicit software override unless the default-renderer override is enabled.

| Environment variable | Effect |
|---|---|
| `NEXTSTUDIO_FORCE_SOFTWARE_RENDERER=1` | Request software desktop rendering for diagnosis |
| `NEXTSTUDIO_FORCE_DEFAULT_RENDERER=1` | Bypass that renderer fallback for comparison; takes precedence if both are enabled |

The implementation also accepts `true`/`yes` case-insensitively. These switches control renderer fallback, **not** the independent Wine font selection. Native Windows keeps JUCE's normal backend unless explicitly overridden; Wine and native/RDP validation are separate claims.

`NextStudioApplication` owns the fallback. Main-window `applyTo()` runs before showing the peer. Active fallback selects the available engine named `Software Renderer` rather than assuming a universal numeric index. The embedded setup wizard uses that same peer. Desktop focus changes schedule asynchronous reapplication to other JUCE desktop components; teardown stops listeners/timer and cancels pending updates. Arbitrary third-party native plugin windows are not automatically covered by JUCE-component iteration.

### Wine DXGI guard and repaint handling

On Windows under Wine, startup attempts to guard the application's imported `CreateDXGIFactory2`: the import entry is redirected to an application-local function returning `E_NOTIMPL` with a null factory. This is runtime adaptation of the executable's import table, not a patched JUCE source tree or guarantee about imports inside external plugin DLLs.

If that guard is installed, applying fallback starts a 60 Hz timer that flushes pending software repaints on available desktop peers using safe component references. This source behavior was absent from the older prose; it matters when diagnosing rendering progress. Focus-driven application remains asynchronous; the timer is conditional, not a global timing promise for all platforms.

### Font selection

After installing the default LookAndFeel, `configureFontFallback()` checks installed typefaces under Wine and selects the first available family from Tahoma, Arial, Liberation Sans, then DejaVu Sans. Without an installed candidate it warns rather than pretending text rendering succeeded. Theme/UI font state is resolved in the active LookAndFeel; software rendering alone cannot supply missing glyphs.

JUCE 8's Wine DirectWrite defaults can name Bitstream Vera families absent from a prefix. A usable installed sans-serif family is therefore a separate requirement from changing the renderer.

### Logging

The [central logger](../logging.md) records significant activation/software-renderer/font events at info, missing engines/fonts at warn and guard diagnostics at debug. Focus rechecks must not flood the log. Startup DXGI noise alone neither proves nor disproves that the final software peer is functioning; inspect logs and actual interaction.

## Example Bottles procedure

These are configurable examples, not the historical runner/package path. Use a new test bottle name, an installed runner and the package under test. Do not overwrite a user's existing bottle.

```bash
flatpak install --user flathub com.usebottles.bottles
flatpak run --command=bottles-cli com.usebottles.bottles --help

# Set these to the isolated bottle, installed runner and actual installer.
BOTTLE=NextStudioValidation
RUNNER='<installed-runner-name>'
INSTALLER='/path/to/NextStudio-windows-installer.exe'

flatpak run --command=bottles-cli com.usebottles.bottles new \
  --bottle-name "$BOTTLE" --environment application --arch win64 --runner "$RUNNER"

# Put the installer inside Bottles' accessible app-data area if required.
install -m 644 "$INSTALLER" \
  "$HOME/.var/app/com.usebottles.bottles/data/NextStudioValidation.exe"
flatpak run --command=bottles-cli com.usebottles.bottles run \
  -b "$BOTTLE" -e "$HOME/.var/app/com.usebottles.bottles/data/NextStudioValidation.exe" /S

# Use the program name registered by the installed package.
flatpak run --command=bottles-cli com.usebottles.bottles run \
  -b "$BOTTLE" -p NextStudio
```

Set diagnostic environment overrides for the **Wine application process** via the bottle's environment configuration or its supported launch mechanism; do not assume host Flatpak environment forwarding. Record which override actually reached the process and remove it after comparison. CLI launch helps distinguish a Bottles-management UI hang from a hosted-app hang.

## Verification and failure handling

Record the tested revision/package, OS, runner/prefix, renderer overrides, graphics settings and fonts. Verify separately:

1. first launch/setup and normal main window show usable controls/text;
2. requested renderer/font appears in logs and the UI actually repaints/responds;
3. embedded projects/Save As and relevant additional JUCE/plugin windows are readable;
4. resizing, focus changes and re-opening windows do not leave black/stale surfaces;
5. controlled default-versus-software comparisons distinguish renderer failure from a missing font;
6. shutdown/lock transitions do not leave unsupported peer/timer state.

Use [Testing](testing.md) for build/test commands and evidence/coverage rules. Keep useful screenshots/logs with an issue or retained artifact, not only one agent's temporary directory. A recommended checklist is not evidence of a completed run.

If text is missing, inspect available candidate fonts independently of renderer state. If Bottles claims a hang, inspect the actual application window/process before classifying it. Missing software engine/font and failed guard installation must be recorded as limitations, not hidden by a successful host build.

## Limitations and rationale

- There is no dedicated Wine/native Windows runtime suite in this migration. Earlier RDP success is a historical environment result, not exhaustive current Windows certification.
- Third-party native editors and all focus/opening paths need separate platform validation; desktop JUCE-peer adaptation does not cover every foreign window.
- Wine audio negotiation, MIDI threads/device behavior and real-time DSP are distinct from visible text/rendering and remain separately unvalidated.
- Stock Wine can emit DXGI/DirectComposition initialization warnings even after a usable software fallback. Symptoms and actual peer interaction matter more than one log line.
- Local runtime adaptation is easier to carry/remove than vendored JUCE modifications or requiring a specially patched Wine runner. Revisit it against future Wine/JUCE changes with explicit native/Wine comparison, not assumptions based on the original snapshot.

Relevant upstream implementation areas are JUCE's `juce_Windowing_windows.cpp`, `juce_DirectWriteTypeface_windows.cpp` and `juce_Threads_windows.cpp` under `modules/tracktion_engine/modules/juce/modules/`. Use current source for backend details; historical numeric renderer indices are not a cross-platform contract.
