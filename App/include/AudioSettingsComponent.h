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
#include "ApplicationViewState.h"
#include "MetronomeSampleManager.h"

namespace te = tracktion_engine;

class MetronomeSettingsComponent final : public juce::Component
{
public:
    MetronomeSettingsComponent(te::Engine &, te::Edit &, ApplicationViewState &);

    void resized() override;
    void lookAndFeelChanged() override;

    static constexpr int preferredHeight = 224;

private:
    using SampleRole = MetronomeSampleManager::SampleRole;

    void chooseSample(SampleRole role);
    void applySample(SampleRole role, const juce::File &source);
    void resetSample(SampleRole role);
    void updateSampleLabels();
    void showError(const juce::String &message);
    [[nodiscard]] juce::File getSampleStorageDirectory() const;

    te::Engine &m_engine;
    te::Edit &m_edit;
    ApplicationViewState &m_appState;

    juce::GroupComponent m_group{"Metronome settings", "Metronome"};
    juce::Label m_description;
    juce::Label m_volumeLabel;
    juce::Slider m_volumeSlider;
    juce::Label m_accentLabel;
    juce::Label m_accentFile;
    juce::TextButton m_chooseAccent{"Choose..."};
    juce::TextButton m_resetAccent{"Default"};
    juce::Label m_regularLabel;
    juce::Label m_regularFile;
    juce::TextButton m_chooseRegular{"Choose..."};
    juce::TextButton m_resetRegular{"Default"};
    std::unique_ptr<juce::FileChooser> m_fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetronomeSettingsComponent)
};

class AudioSettingsComponent final : public juce::Component
{
public:
    AudioSettingsComponent(te::Engine &, te::Edit &, ApplicationViewState &);
    ~AudioSettingsComponent() override;

    void resized() override;
    void paint(juce::Graphics &) override;

private:
    ApplicationViewState &m_appState;
    juce::Viewport m_viewport;
    juce::Component m_content;
    juce::AudioDeviceSelectorComponent m_deviceSelector;
    MetronomeSettingsComponent m_metronomeSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsComponent)
};
