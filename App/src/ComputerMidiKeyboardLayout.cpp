/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "ComputerMidiKeyboardLayout.h"

namespace ComputerMidiKeyboardLayout
{
const std::array<Mapping, 24> &getPrimaryMappings()
{
    static constexpr std::array mappings{
        Mapping{"y", 0}, Mapping{"s", 1}, Mapping{"x", 2}, Mapping{"d", 3}, Mapping{"c", 4},
        Mapping{"v", 5}, Mapping{"g", 6}, Mapping{"b", 7}, Mapping{"h", 8}, Mapping{"n", 9},
        Mapping{"j", 10}, Mapping{"m", 11},
        Mapping{"2", 13}, Mapping{"w", 14}, Mapping{"3", 15}, Mapping{"e", 16}, Mapping{"r", 17},
        Mapping{"5", 18}, Mapping{"t", 19}, Mapping{"6", 20}, Mapping{"z", 21}, Mapping{"7", 22},
        Mapping{"u", 23}, Mapping{"i", 24},
    };

    return mappings;
}

const std::array<juce::KeyPress, 2> &getUpperCAliases()
{
    static const std::array aliases{
        juce::KeyPress::createFromDescription("q"),
        juce::KeyPress::createFromDescription(",")
    };

    return aliases;
}

void applyTo(juce::MidiKeyboardComponent &keyboard)
{
    keyboard.clearKeyMappings();
    keyboard.setMidiChannel(midiChannel);
    keyboard.setVelocity(velocity, false);
    keyboard.setKeyPressBaseOctave(baseMidiNote / 12);
    keyboard.setWantsKeyboardFocus(false);

    for (const auto &mapping : getPrimaryMappings())
        keyboard.setKeyPressForNote(juce::KeyPress::createFromDescription(mapping.description), mapping.noteOffset);
}

bool isMappedPerformanceKey(const juce::KeyPress &key)
{
    for (const auto &alias : getUpperCAliases())
        if (key == alias)
            return true;

    for (const auto &mapping : getPrimaryMappings())
        if (key == juce::KeyPress::createFromDescription(mapping.description))
            return true;

    return false;
}
} // namespace ComputerMidiKeyboardLayout
