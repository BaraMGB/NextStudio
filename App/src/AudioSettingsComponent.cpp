/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "AudioSettingsComponent.h"
#include "tracktion_engine/playback/graph/tracktion_ClickNode.h"

namespace
{
void configureFileLabel(juce::Label &label)
{
    label.setJustificationType(juce::Justification::centredLeft);
    label.setMinimumHorizontalScale(0.75f);
}

void configureButton(juce::TextButton &button, ApplicationViewState &appState)
{
    button.setColour(juce::TextButton::buttonColourId, appState.getButtonBackgroundColour());
    button.setColour(juce::TextButton::textColourOffId, appState.getButtonTextColour());
    button.setColour(juce::TextButton::textColourOnId, appState.getButtonTextColour());
}
} // namespace

MetronomeSettingsComponent::MetronomeSettingsComponent(te::Engine &engine, te::Edit &edit, ApplicationViewState &appState)
    : m_engine(engine),
      m_edit(edit),
      m_appState(appState)
{
    addAndMakeVisible(m_group);
    addAndMakeVisible(m_description);
    addAndMakeVisible(m_volumeLabel);
    addAndMakeVisible(m_volumeSlider);
    addAndMakeVisible(m_accentLabel);
    addAndMakeVisible(m_accentFile);
    addAndMakeVisible(m_chooseAccent);
    addAndMakeVisible(m_resetAccent);
    addAndMakeVisible(m_regularLabel);
    addAndMakeVisible(m_regularFile);
    addAndMakeVisible(m_chooseRegular);
    addAndMakeVisible(m_resetRegular);

    m_description.setText("Choose WAV samples for the accented bar beat and regular beats.", juce::dontSendNotification);
    m_description.setJustificationType(juce::Justification::centredLeft);
    m_description.setMinimumHorizontalScale(0.75f);

    m_volumeLabel.setText("Click volume", juce::dontSendNotification);
    m_volumeLabel.setJustificationType(juce::Justification::centredLeft);
    m_volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 54, 22);
    m_volumeSlider.setRange(20.0, 100.0, 1.0);
    m_volumeSlider.setTextValueSuffix(" %");
    m_volumeSlider.setValue(juce::jlimit(20.0, 100.0, static_cast<double>(m_appState.m_metronomeVolume.get() * 100.0f)), juce::dontSendNotification);
    m_volumeSlider.setTooltip("Sets the overall level of both metronome click samples.");
    m_volumeSlider.onValueChange = [this]
    {
        const auto gain = static_cast<float>(m_volumeSlider.getValue() / 100.0);
        m_appState.m_metronomeVolume = gain;
        m_edit.setClickTrackVolume(gain);

        if (!m_volumeSlider.isMouseButtonDown())
            m_appState.saveState();
    };
    m_volumeSlider.onDragEnd = [this] { m_appState.saveState(); };

    m_accentLabel.setText("Accent", juce::dontSendNotification);
    m_regularLabel.setText("Regular", juce::dontSendNotification);
    m_accentLabel.setJustificationType(juce::Justification::centredLeft);
    m_regularLabel.setJustificationType(juce::Justification::centredLeft);
    configureFileLabel(m_accentFile);
    configureFileLabel(m_regularFile);

    m_chooseAccent.onClick = [this] { chooseSample(SampleRole::accent); };
    m_chooseRegular.onClick = [this] { chooseSample(SampleRole::regular); };
    m_resetAccent.onClick = [this] { resetSample(SampleRole::accent); };
    m_resetRegular.onClick = [this] { resetSample(SampleRole::regular); };

    lookAndFeelChanged();
    updateSampleLabels();
}

void MetronomeSettingsComponent::resized()
{
    m_group.setBounds(getLocalBounds());

    auto area = getLocalBounds().reduced(12);
    area.removeFromTop(12);
    m_description.setBounds(area.removeFromTop(30));
    area.removeFromTop(2);

    auto volumeRow = area.removeFromTop(32);
    m_volumeLabel.setBounds(volumeRow.removeFromLeft(76));
    volumeRow.removeFromLeft(6);
    m_volumeSlider.setBounds(volumeRow);
    area.removeFromTop(5);

    const auto layoutSampleRow = [](juce::Rectangle<int> row,
                                    juce::Label &title,
                                    juce::Label &file,
                                    juce::TextButton &choose,
                                    juce::TextButton &reset)
    {
        title.setBounds(row.removeFromTop(20));
        auto controls = row.removeFromTop(28);
        reset.setBounds(controls.removeFromRight(56));
        controls.removeFromRight(5);
        choose.setBounds(controls.removeFromRight(64));
        controls.removeFromRight(6);
        file.setBounds(controls);
    };

    layoutSampleRow(area.removeFromTop(50), m_accentLabel, m_accentFile, m_chooseAccent, m_resetAccent);
    area.removeFromTop(3);
    layoutSampleRow(area.removeFromTop(50), m_regularLabel, m_regularFile, m_chooseRegular, m_resetRegular);
}

void MetronomeSettingsComponent::lookAndFeelChanged()
{
    const auto textColour = m_appState.getTextColour();
    const auto fieldColour = m_appState.getBackgroundColour1();
    const auto borderColour = m_appState.getBorderColour();

    m_group.setColour(juce::GroupComponent::outlineColourId, borderColour);
    m_group.setColour(juce::GroupComponent::textColourId, m_appState.getPrimeColour());

    for (auto *label : {&m_description, &m_volumeLabel, &m_accentLabel, &m_regularLabel})
        label->setColour(juce::Label::textColourId, textColour);

    for (auto *label : {&m_accentFile, &m_regularFile})
    {
        label->setColour(juce::Label::backgroundColourId, fieldColour);
        label->setColour(juce::Label::textColourId, textColour);
        label->setColour(juce::Label::outlineColourId, borderColour);
    }

    for (auto *button : {&m_chooseAccent, &m_resetAccent, &m_chooseRegular, &m_resetRegular})
        configureButton(*button, m_appState);

    m_volumeSlider.setColour(juce::Slider::backgroundColourId, fieldColour);
    m_volumeSlider.setColour(juce::Slider::trackColourId, m_appState.getPrimeColour());
    m_volumeSlider.setColour(juce::Slider::thumbColourId, m_appState.getButtonTextColour());
    m_volumeSlider.setColour(juce::Slider::textBoxTextColourId, textColour);
    m_volumeSlider.setColour(juce::Slider::textBoxBackgroundColourId, fieldColour);
    m_volumeSlider.setColour(juce::Slider::textBoxOutlineColourId, borderColour);
}

void MetronomeSettingsComponent::chooseSample(SampleRole role)
{
    const auto currentPath = te::Click::getClickWaveFile(m_engine, role == SampleRole::accent);
    const auto currentFile = juce::File(currentPath);
    const auto initialDirectory = currentFile.existsAsFile() ? currentFile.getParentDirectory()
                                                              : juce::File(m_appState.m_samplesDir.get());
    const auto title = role == SampleRole::accent ? "Choose Accent Metronome Sample"
                                                   : "Choose Regular Metronome Sample";

    m_fileChooser = std::make_unique<juce::FileChooser>(title, initialDirectory, "*.wav;*.WAV");
    const auto safeThis = juce::Component::SafePointer<MetronomeSettingsComponent>(this);
    m_fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                               [safeThis, role](const juce::FileChooser &chooser)
                               {
                                   if (safeThis == nullptr)
                                       return;

                                   const auto selectedFile = chooser.getResult();
                                   if (selectedFile != juce::File())
                                       safeThis->applySample(role, selectedFile);
                               });
}

void MetronomeSettingsComponent::applySample(SampleRole role, const juce::File &source)
{
    const auto storageDirectory = getSampleStorageDirectory();
    const auto imported = MetronomeSampleManager::importSample(source, storageDirectory, role);
    if (!imported.succeeded())
    {
        showError(imported.errorMessage);
        return;
    }

    te::Click::setClickWaveFile(m_engine, role == SampleRole::accent, imported.file.getFullPathName());
    MetronomeSampleManager::removeManagedSamples(storageDirectory, role, imported.file);
    updateSampleLabels();
}

void MetronomeSettingsComponent::resetSample(SampleRole role)
{
    te::Click::setClickWaveFile(m_engine, role == SampleRole::accent, {});
    MetronomeSampleManager::removeManagedSamples(getSampleStorageDirectory(), role);
    updateSampleLabels();
}

void MetronomeSettingsComponent::updateSampleLabels()
{
    const auto updateLabel = [this](juce::Label &label, juce::TextButton &reset, SampleRole role)
    {
        const auto path = te::Click::getClickWaveFile(m_engine, role == SampleRole::accent);
        const auto file = juce::File(path);
        const auto usingDefault = path.isEmpty();

        label.setText(usingDefault ? "Tracktion default" : file.getFileName(), juce::dontSendNotification);
        label.setTooltip(usingDefault ? "Built-in Tracktion metronome sample" : file.getFullPathName());
        reset.setEnabled(!usingDefault);
    };

    updateLabel(m_accentFile, m_resetAccent, SampleRole::accent);
    updateLabel(m_regularFile, m_resetRegular, SampleRole::regular);
}

void MetronomeSettingsComponent::showError(const juce::String &message)
{
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                           "Metronome Sample",
                                           message,
                                           "OK",
                                           this);
}

juce::File MetronomeSettingsComponent::getSampleStorageDirectory() const
{
    return MetronomeSampleManager::getStorageDirectory(m_appState.getSettingsFile());
}

AudioSettingsComponent::AudioSettingsComponent(te::Engine &engine, te::Edit &edit, ApplicationViewState &appState)
    : m_appState(appState),
      m_deviceSelector(engine.getDeviceManager().deviceManager, 0, 512, 1, 512, false, false, false, false),
      m_metronomeSettings(engine, edit, appState)
{
    addAndMakeVisible(m_viewport);
    m_viewport.setViewedComponent(&m_content, false);
    m_viewport.setScrollBarThickness(m_appState.getScrollbarThickness());
    m_viewport.setScrollBarsShown(true, false, true, false);

    m_content.addAndMakeVisible(m_deviceSelector);
    m_content.addAndMakeVisible(m_metronomeSettings);
}

AudioSettingsComponent::~AudioSettingsComponent()
{
    m_viewport.setViewedComponent(nullptr, false);
}

void AudioSettingsComponent::resized()
{
    m_viewport.setBounds(getLocalBounds());

    const auto contentWidth = juce::jmax(1, m_viewport.getWidth() - m_viewport.getScrollBarThickness());
    m_deviceSelector.setSize(contentWidth, 1);
    const auto deviceHeight = juce::jmax(180, m_deviceSelector.getHeight());
    const auto gap = 8;
    const auto contentHeight = deviceHeight + gap + MetronomeSettingsComponent::preferredHeight + 6;

    m_content.setSize(contentWidth, contentHeight);
    m_deviceSelector.setBounds(0, 0, contentWidth, deviceHeight);
    m_metronomeSettings.setBounds(6, deviceHeight + gap,
                                  juce::jmax(1, contentWidth - 12),
                                  MetronomeSettingsComponent::preferredHeight);
}

void AudioSettingsComponent::paint(juce::Graphics &g)
{
    g.fillAll(m_appState.getBackgroundColour2());
}

