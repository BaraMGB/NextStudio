/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

*/

#include "MetronomeSampleManager.h"

#include <iostream>

namespace
{
int failures = 0;

#define REQUIRE(cond) \
    do { if (!(cond)) { std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; ++failures; } } while (0)

class ScopedTestDirectory
{
public:
    ScopedTestDirectory()
        : directory(juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getNonexistentChildFile("NextStudioMetronomeTests", {}, false))
    {
        REQUIRE(directory.createDirectory().wasOk());
    }

    ~ScopedTestDirectory() { directory.deleteRecursively(); }

    juce::File directory;
};

bool writeWaveFile(const juce::File &file)
{
    juce::AudioBuffer<float> buffer(1, 256);
    buffer.clear();
    buffer.setSample(0, 0, 0.5f);

    auto writer = std::unique_ptr<juce::AudioFormatWriter>(
        juce::WavAudioFormat().createWriterFor(file.createOutputStream().release(),
                                                44100.0, 1, 16, {}, 0));
    return writer != nullptr && writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

void testStorageDirectoryFollowsSettingsFile()
{
    ScopedTestDirectory test;
    const auto settingsFile = test.directory.getChildFile("Profile").getChildFile("AppSettings.xml");
    REQUIRE(MetronomeSampleManager::getStorageDirectory(settingsFile)
            == settingsFile.getParentDirectory().getChildFile("Metronome"));
}

void testValidationRejectsInvalidFiles()
{
    ScopedTestDirectory test;
    const auto missing = test.directory.getChildFile("missing.wav");
    REQUIRE(MetronomeSampleManager::validateWaveFile(missing).failed());

    const auto wrongExtension = test.directory.getChildFile("click.txt");
    REQUIRE(wrongExtension.replaceWithText("not audio"));
    REQUIRE(MetronomeSampleManager::validateWaveFile(wrongExtension).failed());

    const auto invalidWave = test.directory.getChildFile("click.wav");
    REQUIRE(invalidWave.replaceWithText("not audio"));
    REQUIRE(MetronomeSampleManager::validateWaveFile(invalidWave).failed());
}

void testImportCreatesIndependentManagedCopies()
{
    using MetronomeSampleManager::SampleRole;

    ScopedTestDirectory test;
    const auto source = test.directory.getChildFile("Original Click.WAV");
    const auto storage = test.directory.getChildFile("Settings").getChildFile("Metronome");
    REQUIRE(writeWaveFile(source));

    const auto accent = MetronomeSampleManager::importSample(source, storage, SampleRole::accent);
    const auto regular = MetronomeSampleManager::importSample(source, storage, SampleRole::regular);

    REQUIRE(accent.succeeded());
    REQUIRE(regular.succeeded());
    REQUIRE(accent.file != source);
    REQUIRE(regular.file != source);
    REQUIRE(MetronomeSampleManager::isManagedSample(accent.file, storage, SampleRole::accent));
    REQUIRE(MetronomeSampleManager::isManagedSample(regular.file, storage, SampleRole::regular));
    REQUIRE(accent.file.getSize() == source.getSize());

    REQUIRE(source.deleteFile());
    REQUIRE(accent.file.existsAsFile());
    REQUIRE(regular.file.existsAsFile());

    MetronomeSampleManager::removeManagedSamples(storage, SampleRole::accent);
    REQUIRE(!accent.file.existsAsFile());
    REQUIRE(regular.file.existsAsFile());
}

void testCleanupKeepsActiveSampleOnly()
{
    using MetronomeSampleManager::SampleRole;

    ScopedTestDirectory test;
    const auto source = test.directory.getChildFile("click.wav");
    const auto storage = test.directory.getChildFile("Metronome");
    REQUIRE(writeWaveFile(source));

    const auto first = MetronomeSampleManager::importSample(source, storage, SampleRole::accent);
    const auto second = MetronomeSampleManager::importSample(source, storage, SampleRole::accent);
    REQUIRE(first.succeeded());
    REQUIRE(second.succeeded());

    MetronomeSampleManager::removeManagedSamples(storage, SampleRole::accent, second.file);
    REQUIRE(!first.file.existsAsFile());
    REQUIRE(second.file.existsAsFile());
}
} // namespace

int main()
{
    testStorageDirectoryFollowsSettingsFile();
    testValidationRejectsInvalidFiles();
    testImportCreatesIndependentManagedCopies();
    testCleanupKeepsActiveSampleOnly();

    if (failures != 0)
    {
        std::cerr << failures << " metronome sample manager test(s) failed.\n";
        return 1;
    }

    std::cout << "All metronome sample manager tests passed.\n";
    return 0;
}
