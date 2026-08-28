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

namespace ComputerMidiKeyboardLayout
{
struct Mapping
{
    const char *description;
    int noteOffset;
};

inline constexpr int midiChannel = 1;
inline constexpr int baseMidiNote = 48;
inline constexpr int upperCMidiNote = baseMidiNote + 12;
inline constexpr float velocity = 0.8f;

const std::array<Mapping, 24> &getPrimaryMappings();
const std::array<juce::KeyPress, 2> &getUpperCAliases();
void applyTo(juce::MidiKeyboardComponent &keyboard);
bool isMappedPerformanceKey(const juce::KeyPress &key);
} // namespace ComputerMidiKeyboardLayout
