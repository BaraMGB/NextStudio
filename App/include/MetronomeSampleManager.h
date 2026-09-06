/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace MetronomeSampleManager
{
enum class SampleRole
{
    accent,
    regular
};

struct ImportResult
{
    juce::File file;
    juce::String errorMessage;

    [[nodiscard]] bool succeeded() const noexcept { return file.existsAsFile() && errorMessage.isEmpty(); }
};

[[nodiscard]] juce::File getStorageDirectory(const juce::File &settingsFile);
[[nodiscard]] juce::String getRolePrefix(SampleRole role);
[[nodiscard]] bool isManagedSample(const juce::File &file, const juce::File &storageDirectory, SampleRole role);
[[nodiscard]] juce::Result validateWaveFile(const juce::File &source);
[[nodiscard]] ImportResult importSample(const juce::File &source, const juce::File &storageDirectory, SampleRole role);
void removeManagedSamples(const juce::File &storageDirectory, SampleRole role, const juce::File &fileToKeep = {});
} // namespace MetronomeSampleManager
