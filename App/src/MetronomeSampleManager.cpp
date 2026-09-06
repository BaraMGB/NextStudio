/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "MetronomeSampleManager.h"

namespace MetronomeSampleManager
{
juce::File getStorageDirectory(const juce::File &settingsFile)
{
    return settingsFile.getParentDirectory().getChildFile("Metronome");
}

juce::String getRolePrefix(SampleRole role)
{
    return role == SampleRole::accent ? "accent-" : "regular-";
}

bool isManagedSample(const juce::File &file, const juce::File &storageDirectory, SampleRole role)
{
    return file != juce::File()
           && file.getParentDirectory() == storageDirectory
           && file.getFileName().startsWith(getRolePrefix(role));
}

juce::Result validateWaveFile(const juce::File &source)
{
    if (!source.existsAsFile())
        return juce::Result::fail("The selected file does not exist.");

    if (!source.hasFileExtension("wav"))
        return juce::Result::fail("Select a WAV file.");

    auto stream = source.createInputStream();
    if (stream == nullptr)
        return juce::Result::fail("The selected WAV file could not be opened.");

    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatReader> reader(format.createReaderFor(stream.release(), true));
    if (reader == nullptr || reader->sampleRate <= 0.0 || reader->numChannels == 0 || reader->lengthInSamples <= 0)
        return juce::Result::fail("The selected file is not a readable WAV audio file.");

    return juce::Result::ok();
}

ImportResult importSample(const juce::File &source, const juce::File &storageDirectory, SampleRole role)
{
    if (const auto validation = validateWaveFile(source); validation.failed())
        return {{}, validation.getErrorMessage()};

    if (const auto result = storageDirectory.createDirectory(); result.failed())
        return {{}, "The metronome settings folder could not be created:\n" + result.getErrorMessage()};

    auto sourceName = juce::File::createLegalFileName(source.getFileName());
    if (sourceName.isEmpty())
        sourceName = "click.wav";

    auto uniqueId = juce::Uuid().toString().removeCharacters("-").substring(0, 12);
    const auto destination = storageDirectory.getChildFile(getRolePrefix(role) + uniqueId + "-" + sourceName);

    if (!source.copyFileTo(destination) || !destination.existsAsFile())
    {
        destination.deleteFile();
        return {{}, "The metronome sample could not be copied to the settings folder."};
    }

    return {destination, {}};
}

void removeManagedSamples(const juce::File &storageDirectory, SampleRole role, const juce::File &fileToKeep)
{
    if (!storageDirectory.isDirectory())
        return;

    const auto files = storageDirectory.findChildFiles(juce::File::findFiles, false);
    for (const auto &file : files)
        if (isManagedSample(file, storageDirectory, role) && file != fileToKeep)
            file.deleteFile();
}
} // namespace MetronomeSampleManager
