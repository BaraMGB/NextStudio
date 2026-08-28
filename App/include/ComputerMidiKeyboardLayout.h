/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <array>

class ApplicationViewState;

namespace ComputerMidiKeyboardLayout
{
struct Mapping
{
    const char *noteName;
    const char *defaultDescription;
    int noteOffset;
};

inline constexpr int midiChannel = 1;
inline constexpr int baseMidiNote = 48;
inline constexpr int noteCount = 25;
inline constexpr int upperCIndex = 12;
inline constexpr int upperCMidiNote = baseMidiNote + 12;
inline constexpr float velocity = 0.8f;

struct State
{
    std::array<juce::String, noteCount> primaryKeyDescriptions;
    juce::String upperCAliasDescription;
};

const std::array<Mapping, noteCount> &getDefaultMappings();
State createDefaultState();
State loadFrom(const ApplicationViewState &appState);
void saveTo(ApplicationViewState &appState, const State &state);

juce::String normaliseKeyDescription(const juce::String &description);
juce::KeyPress keyPressFromDescription(const juce::String &description);
juce::String getDisplayTextForDescription(const juce::String &description);
juce::String validate(const State &state);

void applyTo(juce::MidiKeyboardComponent &keyboard, const State &state);
bool isMappedPerformanceKey(const juce::KeyPress &key, const State &state);
juce::KeyPress getUpperCAliasKey(const State &state);
} // namespace ComputerMidiKeyboardLayout
