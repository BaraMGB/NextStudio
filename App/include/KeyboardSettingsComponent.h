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
#include "ApplicationViewState.h"
#include "ComputerMidiKeyboardLayout.h"
#include <optional>

class ComputerMidiKeyboardSettingsComponent : public juce::Component
{
public:
    explicit ComputerMidiKeyboardSettingsComponent(ApplicationViewState &appState);

    int getPreferredHeight(int width) const;
    void refreshFromAppState();
    void refreshThemeFromAppState();

    void resized() override;
    bool keyPressed(const juce::KeyPress &key) override;
    void focusLost(FocusChangeType cause) override;

private:
    struct CaptureSlot
    {
        int index = -1;
        bool isUpperCAlias = false;
    };

    void beginCapture(CaptureSlot slot);
    void cancelCapture();
    void applyCapturedDescription(const juce::String &description);
    void updateButtonTexts();
    void updateStatus(const juce::String &text, juce::Colour colour);
    bool isCaptureActiveFor(const CaptureSlot &slot) const;

    ApplicationViewState &m_appState;
    ComputerMidiKeyboardLayout::State m_state;

    juce::Label m_titleLabel;
    juce::Label m_helpLabel;
    juce::Label m_statusLabel;
    juce::Label m_upperCAliasLabel;
    juce::TextButton m_resetButton{"Reset Defaults"};
    std::array<std::unique_ptr<juce::Label>, ComputerMidiKeyboardLayout::noteCount> m_noteLabels;
    std::array<std::unique_ptr<juce::TextButton>, ComputerMidiKeyboardLayout::noteCount> m_keyButtons;
    juce::TextButton m_upperCAliasButton{"Upper C Alias"};
    std::optional<CaptureSlot> m_captureSlot;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComputerMidiKeyboardSettingsComponent)
};

class KeyboardSettingsComponent : public juce::Component
{
public:
    KeyboardSettingsComponent(ApplicationViewState &appState, juce::KeyPressMappingSet &keyMappings);

    void resized() override;
    void refreshThemeFromAppState();

private:
    ApplicationViewState &m_appState;
    juce::Label m_computerMidiHeader;
    ComputerMidiKeyboardSettingsComponent m_computerMidiKeyboardSettings;
    juce::Label m_commandHeader;
    juce::KeyMappingEditorComponent m_keyMappingEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyboardSettingsComponent)
};
