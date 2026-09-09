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

#include <utility>
#include <vector>

class ThemePresetModel
{
public:
    struct Preset
    {
        juce::String name;
        juce::File file;
        juce::ValueTree state;
        bool builtIn = false;
    };

    ThemePresetModel(juce::File directory, juce::StringArray builtInNames);

    const juce::File &getDirectory() const noexcept { return m_directory; }
    void setDirectory(juce::File directory) { m_directory = std::move(directory); }
    std::vector<Preset> getPresets() const;
    juce::Result savePreset(const juce::String &name, const juce::ValueTree &state, bool overwrite, Preset *savedPreset = nullptr) const;

    static juce::Result validateName(const juce::String &name);
    static juce::ValueTree loadThemeState(const juce::File &file);
    static bool areStatesEquivalent(const juce::ValueTree &a, const juce::ValueTree &b);

private:
    bool isBuiltInName(const juce::String &name) const;
    juce::File findFileForName(const juce::String &name) const;

    juce::File m_directory;
    juce::StringArray m_builtInNames;
};
