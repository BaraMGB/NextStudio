/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "ComputerMidiKeyboardLayout.h"
#include "ApplicationViewState.h"

namespace
{
juce::Identifier getPrimaryKeyProperty(int index)
{
    return juce::Identifier("PrimaryKey" + juce::String(index));
}

ComputerMidiKeyboardLayout::State normaliseState(ComputerMidiKeyboardLayout::State state)
{
    for (auto &description : state.primaryKeyDescriptions)
        description = ComputerMidiKeyboardLayout::normaliseKeyDescription(description);

    state.upperCAliasDescription = ComputerMidiKeyboardLayout::normaliseKeyDescription(state.upperCAliasDescription);
    return state;
}
} // namespace

namespace ComputerMidiKeyboardLayout
{
const std::array<Mapping, noteCount> &getDefaultMappings()
{
    static constexpr std::array mappings{
        Mapping{"C3", "y", 0}, Mapping{"C#3", "s", 1}, Mapping{"D3", "x", 2}, Mapping{"D#3", "d", 3}, Mapping{"E3", "c", 4},
        Mapping{"F3", "v", 5}, Mapping{"F#3", "g", 6}, Mapping{"G3", "b", 7}, Mapping{"G#3", "h", 8}, Mapping{"A3", "n", 9},
        Mapping{"A#3", "j", 10}, Mapping{"B3", "m", 11},
        Mapping{"C4", "q", 12}, Mapping{"C#4", "2", 13}, Mapping{"D4", "w", 14}, Mapping{"D#4", "3", 15}, Mapping{"E4", "e", 16},
        Mapping{"F4", "r", 17}, Mapping{"F#4", "5", 18}, Mapping{"G4", "t", 19}, Mapping{"G#4", "6", 20}, Mapping{"A4", "z", 21},
        Mapping{"A#4", "7", 22}, Mapping{"B4", "u", 23}, Mapping{"C5", "i", 24},
    };

    return mappings;
}

State createDefaultState()
{
    State state;
    const auto &mappings = getDefaultMappings();

    for (int i = 0; i < noteCount; ++i)
        state.primaryKeyDescriptions[(size_t) i] = normaliseKeyDescription(mappings[(size_t) i].defaultDescription);

    state.upperCAliasDescription = normaliseKeyDescription(",");
    return state;
}

State loadFrom(const ApplicationViewState &appState)
{
    auto state = createDefaultState();
    const auto keyboardState = appState.m_applicationStateValueTree.getChildWithName(IDs::ComputerMidiKeyboard);

    if (!keyboardState.isValid())
        return state;

    for (int i = 0; i < noteCount; ++i)
    {
        const auto property = getPrimaryKeyProperty(i);
        if (keyboardState.hasProperty(property))
            state.primaryKeyDescriptions[(size_t) i] = keyboardState[property].toString();
    }

    if (keyboardState.hasProperty(IDs::UpperCAliasKey))
        state.upperCAliasDescription = keyboardState[IDs::UpperCAliasKey].toString();

    return normaliseState(std::move(state));
}

void saveTo(ApplicationViewState &appState, const State &stateToSave)
{
    const auto state = normaliseState(stateToSave);
    auto keyboardState = appState.m_applicationStateValueTree.getOrCreateChildWithName(IDs::ComputerMidiKeyboard, nullptr);

    for (int i = 0; i < noteCount; ++i)
        keyboardState.setProperty(getPrimaryKeyProperty(i), state.primaryKeyDescriptions[(size_t) i], nullptr);

    keyboardState.setProperty(IDs::UpperCAliasKey, state.upperCAliasDescription, nullptr);
}

juce::String normaliseKeyDescription(const juce::String &description)
{
    const auto trimmed = description.trim();
    if (trimmed.isEmpty())
        return {};

    const auto key = juce::KeyPress::createFromDescription(trimmed);
    if (!key.isValid() || key.getModifiers().isAnyModifierKeyDown())
        return {};

    return key.getTextDescription();
}

juce::KeyPress keyPressFromDescription(const juce::String &description)
{
    const auto normalisedDescription = normaliseKeyDescription(description);
    return normalisedDescription.isEmpty() ? juce::KeyPress{} : juce::KeyPress::createFromDescription(normalisedDescription);
}

juce::String getDisplayTextForDescription(const juce::String &description)
{
    const auto key = keyPressFromDescription(description);
    return key.isValid() ? key.getTextDescription() : juce::String("Unassigned");
}

juce::String validate(const State &stateToValidate)
{
    const auto state = normaliseState(stateToValidate);
    juce::StringArray usedKeys;

    for (int i = 0; i < noteCount; ++i)
    {
        const auto &description = state.primaryKeyDescriptions[(size_t) i];
        if (description.isEmpty())
            continue;

        if (usedKeys.contains(description, true))
            return "Duplicate computer MIDI key: " + description;

        usedKeys.add(description);
    }

    if (state.upperCAliasDescription.isNotEmpty())
    {
        if (usedKeys.contains(state.upperCAliasDescription, true))
            return "Duplicate computer MIDI key: " + state.upperCAliasDescription;

        usedKeys.add(state.upperCAliasDescription);
    }

    return {};
}

void applyTo(juce::MidiKeyboardComponent &keyboard, const State &stateToApply)
{
    const auto state = normaliseState(stateToApply);
    keyboard.clearKeyMappings();
    keyboard.setMidiChannel(midiChannel);
    keyboard.setVelocity(velocity, false);
    keyboard.setKeyPressBaseOctave(baseMidiNote / 12);
    keyboard.setWantsKeyboardFocus(false);

    const auto &mappings = getDefaultMappings();
    for (int i = 0; i < noteCount; ++i)
    {
        const auto key = keyPressFromDescription(state.primaryKeyDescriptions[(size_t) i]);
        if (key.isValid())
            keyboard.setKeyPressForNote(key, mappings[(size_t) i].noteOffset);
    }
}

bool isMappedPerformanceKey(const juce::KeyPress &key, const State &state)
{
    for (const auto &description : state.primaryKeyDescriptions)
        if (key == keyPressFromDescription(description))
            return true;

    const auto upperCAliasKey = getUpperCAliasKey(state);
    return upperCAliasKey.isValid() && key == upperCAliasKey;
}

juce::KeyPress getUpperCAliasKey(const State &state)
{
    return keyPressFromDescription(state.upperCAliasDescription);
}
} // namespace ComputerMidiKeyboardLayout
