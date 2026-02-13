# Next Delay Plugin - Anforderungen und Implementierungsplan

## 1. Zielbild

Wir ersetzen das bisher verwendete Tracktion-interne Delay (`type = "delay"`) durch ein eigenes NextStudio-Delay-Plugin auf Basis von `juce::dsp`, mit:

- automatisierbarer/modulierbarer Delay-Zeit
- mehreren Betriebsmodi
- musikalischem Sync-Verhalten
- sauberem Preset/State-Verhalten
- Rueckwaertskompatibilitaet fuer bestehende Projekte

## 2. Scope

### In Scope (MVP+)
1. Neues internes Plugin `NextDelayPlugin` (`xmlTypeName = "next_delay"`).
2. DSP mit JUCE (`juce::dsp::DelayLine` pro Kanal).
3. Modi:
   - `Mono`
   - `Stereo`
   - `PingPong`
   - `Dual`
4. Parameter:
   - `timeMs` (Basiszeit)
   - `feedback`
   - `mix`
   - `mode`
   - `syncEnabled`
   - `syncDivision`
   - `stereoOffsetMs` (oder alternativ `stereoWidth`)
   - `pingPongAmount`
   - `hpCutoff`
   - `lpCutoff`
5. Modulierbare Zeit (Automation/Modifier/LFO) mit Smoothing ohne Klicks.
6. UI-Komponente fuer das neue Delay.
7. Plugin-Menue/Browser-Integration fuer neue Instanzen.
8. Legacy-Support: altes Tracktion-Delay bleibt ladbar.

### Out of Scope (spaeter)
- Tape/Saturation-Charakter
- Flutter/Wow
- Ducking/Sidechain
- Shimmer/Pitch im Feedbackpfad
- Mehrtap-Sektion

## 3. Produktanforderungen (funktional)

### 3.1 Modi

#### Mono
- L/R nutzen identische Delayzeit und Feedbackpfad.
- Center-stabil.

#### Stereo
- Beide Kanaele getrennt, mit Zeitversatz durch `stereoOffsetMs`.
- Kein erzwungener Crossfeed.

#### PingPong
- Signal startet auf einer Seite und wandert durch Cross-Feedback.
- `pingPongAmount` steuert Anteil des kanaluebergreifenden Feedbacks.

#### Dual
- Zwei unabhaengige Delays (L/R), getrennte Delaypfade.
- Geeignet fuer polyrhythmische Breiten.

### 3.2 Zeitverhalten

- `syncEnabled = false`: Zeit in Millisekunden (`timeMs`), frei automierbar.
- `syncEnabled = true`: Zeit aus Tempo + `syncDivision` berechnet.
- `syncDivision` enthaelt mindestens:
  - 1/1, 1/2, 1/4, 1/8, 1/16, 1/32
  - dotted: 1/4D, 1/8D, 1/16D
  - triplet: 1/4T, 1/8T, 1/16T

### 3.3 Tonalitaet im Feedbackpfad

- Highpass und Lowpass im Feedbackpfad.
- Stabilitaetsgrenzen:
  - `feedback` intern geklemmt (z. B. max < 1.0 linear), um unendliches Explodieren zu verhindern.
  - Filterwerte sinnvoll begrenzt.

### 3.4 Automation/Modulation

- `timeMs` und weitere kritische Parameter werden geglaettet (smoothed), um Zipper Noise und harte Spruenge zu vermeiden.
- Ziel: hoerbar musikalische Sweeps ohne Klick-Artefakte.

### 3.5 Bypass/Enable-Verhalten

- Wenn Plugin disabled ist: transparentes Through.
- `reset()` loescht Delaybuffer.
- Bei `initialise/deinitialise` korrekte Ressourcenverwaltung.

## 4. Technische Architektur

### 4.1 Plugin-Klasse

Neue Dateien:
- `App/Source/Plugins/Delay/NextDelayPlugin.h`
- `App/Source/Plugins/Delay/NextDelayPlugin.cpp`

Klasse:
- `class NextDelayPlugin : public te::Plugin`
- `static constexpr const char* xmlTypeName = "next_delay";`
- `static const char* getPluginName() { return "Delay"; }` (UI-Name kann "Delay" bleiben)

### 4.2 Parameter- und State-Modell

Analog zu bestehenden Custom-Plugins:
- `AutomatableParameter::Ptr` + `CachedValue<float/int>`
- `attachToCurrentValue` / `detachFromCurrentValue`
- `restorePluginStateFromValueTree` + `updateFromAttachedValue`

### 4.3 DSP-Kern

- `juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>` je Kanal
- Optional ring-buffer-naher Zugriff fuer PingPong-Routing ueber Feed-Signale
- Feedbackpfad mit `juce::dsp`-Filtern (IIR oder StateVariable)
- Dry/Wet-Mix mit konstantem Lautheitsverhalten

### 4.4 Threading

- Parameterwerte fuer Audio-Thread in atomics / lokale Snapshot-Werte pro Block
- keine Allocations im Audio-Thread
- keine Locks im Audio-Thread

## 5. UI/UX-Anforderungen

### 5.1 Komponente

Bestehende Delay-Komponente auf neues Plugin mappen oder neue Klasse:
- `NextDelayPluginComponent` (empfohlen, klar getrennt)
- oder `DelayPluginComponent` erweitern auf beide Typen

### 5.2 Controls

Mindestens:
- Mode
- Time
- Sync On/Off
- Division
- Feedback
- Mix
- Stereo Offset
- PingPong Amount
- HP / LP

### 5.3 Sichtbarkeit nach Modus

- PingPong Amount nur bei `PingPong` prominent
- Stereo Offset bei `Stereo`/`Dual`
- klare Defaults pro Modus

## 6. Integration im Host

### 6.1 Registrierung

In `MainComponent.cpp`:
- `m_engine.getPluginManager().createBuiltInType<NextDelayPlugin>();`

Wichtig:
- `xmlTypeName` darf nicht `delay` sein (Konflikt mit Tracktion Delay).

### 6.2 Plugin-Listen

Anpassen:
- `EngineHelpers::getInternalPlugins()` in `App/Source/Utilities/Utilities.cpp`
- Builtin-Menue in `App/Source/UI/PluginMenu.cpp`

Verhalten:
- Neue Inserts verwenden `next_delay`.
- Legacy `delay` bleibt ladbar (nicht entfernen).

### 6.3 Rack-Mapping

In `App/Source/LowerRange/PluginChain/RackItemView.cpp`:
- Branch fuer `next_delay` -> Delay-UI
- Legacy `delay` ebenfalls auf Delay-UI routen (optional begrenzt)

## 7. Rueckwaertskompatibilitaet

- Alte Projekte mit `type = "delay"` muessen weiterhin oeffnen.
- Keine automatische harte Migration im MVP.
- Optional spaeter: Migrationsfunktion `delay -> next_delay` mit Parametermapping.

## 8. Parameterranges & Defaults (Vorschlag)

- `mode`: Mono
- `timeMs`: 250 ms (20 .. 2000)
- `feedback`: 0.35 (0 .. 0.95 intern clamp)
- `mix`: 0.25 (0 .. 1)
- `syncEnabled`: false
- `syncDivision`: 1/8
- `stereoOffsetMs`: 15 ms (-100 .. +100)
- `pingPongAmount`: 1.0 (0 .. 1)
- `hpCutoff`: 20 Hz (20 .. 20000)
- `lpCutoff`: 18000 Hz (20 .. 20000)

## 9. Implementierungsplan (Schritte)

### Phase 1 - Plugin-Basis
1. `NextDelayPlugin` Klasse anlegen.
2. Parameter + CachedValues + Attach/Detach.
3. `initialise/deinitialise/reset/applyToBuffer` Grundgeruest.
4. Trockenes Durchreichen + einfacher Delaybetrieb.

### Phase 2 - Modus-Engine
5. Mono und Stereo sauber implementieren.
6. PingPong-Routing (Cross-feedback) ergaenzen.
7. Dual-Modus ergaenzen.
8. Sicherheitsklemmen fuer Feedback und Filter.

### Phase 3 - Sync & Modulation
9. Tempo-sync Division-Mapping implementieren.
10. Zeit-Smoothing fuer harte Automationskurven.
11. Verifikation: Modulation von `timeMs` ohne Klicks.

### Phase 4 - UI
12. UI-Komponente bauen/erweitern.
13. Mode-abhaengige Controls/Labels.
14. Preset/FactoryDefault handling pruefen.

### Phase 5 - Host-Integration
15. Plugin registrieren (`createBuiltInType<NextDelayPlugin>`).
16. Interne Pluginlisten auf neues Delay ausrichten.
17. RackItemView-Mapping fuer `next_delay` + `delay`.

### Phase 6 - QA
18. Manuelle Audio-Tests (siehe Abschnitt 10).
19. Preset Save/Load, Projekt Reload, Undo/Redo.
20. Edge cases (Bypass, Stop/Play, SR-Wechsel).

## 10. Testplan (Abnahme)

### 10.1 Audiofunktion
- Delay hoerbar in allen 4 Modi.
- PingPong wandert korrekt L<->R.
- Stereo/Dual erzeugen reproduzierbare Breite.

### 10.2 Modulation
- Automation auf `timeMs` erzeugt hoerbare Time-Sweeps.
- Keine harten Klicks bei moderaten Kurven.
- LFO/Modifier auf `timeMs` funktioniert.

### 10.3 Sync
- Bei BPM-Aenderung folgt Delayzeit korrekt.
- Division-Umschaltung musikalisch plausibel.
- Sync off/on ohne Instabilitaet.

### 10.4 State/Preset
- Presets speichern/laden alle Parameter.
- Projekt schliessen/oeffnen: identischer Klang.
- Undo/Redo fuer zentrale Parameter.

### 10.5 Kompatibilitaet
- Alte Projekte mit Tracktion-Delay oeffnen weiterhin.
- Neue Projekte instanziieren standardmaessig `next_delay`.

## 11. Risiken und Gegenmassnahmen

- **Artefakte bei schneller Time-Modulation**
  -> konsequentes Smoothing, begrenzte Modulationsgeschwindigkeit, Interpolation.
- **Feedback-Instabilitaet**
  -> interne Clamp + optional Soft-Limiting im Feedbackpfad.
- **Konflikt mit bestehendem `delay` Typ**
  -> strikt eigenes `xmlTypeName = "next_delay"`.

## 12. Akzeptanzkriterien (Definition of Done)

1. Neues internes Delay-Plugin ist auswaehlbar und produktiv nutzbar.
2. `timeMs` ist automierbar/modulierbar und musikalisch stabil.
3. Modi Mono/Stereo/PingPong/Dual funktionieren hoerbar korrekt.
4. Sync/Division funktionieren zuverlaessig.
5. Preset- und Projekt-State sind stabil.
6. Legacy-Projekte mit altem Delay bleiben funktionsfaehig.

## 13. Optionales Follow-up (nach MVP)

- Character/Saturation Schalter
- Ducking Delay
- Modulation-LFO im Delay selbst
- Tape-Mode (wow/flutter, diffusion)
- Reverb in Feedbackpfad
