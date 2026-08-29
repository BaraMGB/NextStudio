/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "KeyboardSettingsComponent.h"

namespace
{
constexpr int sectionPadding = 8;
constexpr int cellHeight = 56;
constexpr int minimumCellWidth = 92;

juce::Colour withAlpha(juce::Colour colour, float alpha)
{
    return colour.withAlpha(alpha);
}

juce::TreeView *findTreeView(juce::Component &component)
{
    for (int i = 0; i < component.getNumChildComponents(); ++i)
    {
        auto *child = component.getChildComponent(i);
        if (auto *tree = dynamic_cast<juce::TreeView *>(child))
            return tree;
    }

    return nullptr;
}
} // namespace

ComputerMidiKeyboardSettingsComponent::ComputerMidiKeyboardSettingsComponent(ApplicationViewState &appState)
    : m_appState(appState)
{
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(true);

    m_titleLabel.setText("Virtual MIDI Keyboard", juce::dontSendNotification);
    m_titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(m_titleLabel);

    m_helpLabel.setText("Click a slot, then press a key. Backspace/Delete clears. Escape cancels.", juce::dontSendNotification);
    m_helpLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(m_helpLabel);

    m_statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(m_statusLabel);

    m_upperCAliasLabel.setText("Upper C alias", juce::dontSendNotification);
    m_upperCAliasLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(m_upperCAliasLabel);

    m_resetButton.onClick = [this]
    {
        cancelCapture();
        m_state = ComputerMidiKeyboardLayout::createDefaultState();
        ComputerMidiKeyboardLayout::saveTo(m_appState, m_state);
        m_appState.saveState();
        refreshFromAppState();
        updateStatus("Default computer MIDI key layout restored.", m_appState.getTextColour());
    };
    addAndMakeVisible(m_resetButton);

    const auto &mappings = ComputerMidiKeyboardLayout::getDefaultMappings();
    for (int i = 0; i < ComputerMidiKeyboardLayout::noteCount; ++i)
    {
        auto label = std::make_unique<juce::Label>();
        label->setText(mappings[(size_t) i].noteName, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label.get());
        m_noteLabels[(size_t) i] = std::move(label);

        auto button = std::make_unique<juce::TextButton>();
        button->onClick = [this, i] { beginCapture({i, false}); };
        addAndMakeVisible(button.get());
        m_keyButtons[(size_t) i] = std::move(button);
    }

    m_upperCAliasButton.onClick = [this] { beginCapture({ComputerMidiKeyboardLayout::upperCIndex, true}); };
    addAndMakeVisible(m_upperCAliasButton);

    refreshFromAppState();
    refreshThemeFromAppState();
}

int ComputerMidiKeyboardSettingsComponent::getPreferredHeight(int width) const
{
    const auto contentWidth = juce::jmax(1, width - sectionPadding * 2);
    const auto columns = juce::jmax(1, contentWidth / minimumCellWidth);
    const auto rows = (ComputerMidiKeyboardLayout::noteCount + columns - 1) / columns;

    return sectionPadding * 2 + 24 + 24 + 20 + rows * cellHeight + 16 + 28;
}

void ComputerMidiKeyboardSettingsComponent::refreshFromAppState()
{
    m_state = ComputerMidiKeyboardLayout::loadFrom(m_appState);
    updateButtonTexts();

    if (!m_captureSlot.has_value())
        updateStatus("Computer MIDI keyboard keys are stored in AppSettings.xml.", withAlpha(m_appState.getTextColour(), 0.8f));
}

void ComputerMidiKeyboardSettingsComponent::refreshThemeFromAppState()
{
    const auto textColour = m_appState.getTextColour();
    const auto mutedTextColour = withAlpha(textColour, 0.8f);
    const auto captureColour = juce::Colours::orange;

    m_titleLabel.setColour(juce::Label::textColourId, textColour);
    m_helpLabel.setColour(juce::Label::textColourId, mutedTextColour);
    m_upperCAliasLabel.setColour(juce::Label::textColourId, textColour);

    for (auto &label : m_noteLabels)
        label->setColour(juce::Label::textColourId, textColour);

    const auto configureButton = [this, textColour](juce::TextButton &button)
    {
        button.setColour(juce::TextButton::buttonColourId, m_appState.getButtonBackgroundColour());
        button.setColour(juce::TextButton::buttonOnColourId, m_appState.getPrimeColour());
        button.setColour(juce::TextButton::textColourOffId, textColour);
        button.setColour(juce::TextButton::textColourOnId, m_appState.getMainFrameColour().contrasting());
    };

    configureButton(m_resetButton);
    configureButton(m_upperCAliasButton);
    for (auto &button : m_keyButtons)
        configureButton(*button);

    if (m_captureSlot.has_value())
        updateStatus(m_statusLabel.getText(), captureColour);
    else if (m_statusLabel.getText().isNotEmpty())
        updateStatus(m_statusLabel.getText(), mutedTextColour);
}

void ComputerMidiKeyboardSettingsComponent::resized()
{
    auto area = getLocalBounds().reduced(sectionPadding);

    auto header = area.removeFromTop(24);
    m_titleLabel.setBounds(header.removeFromLeft(juce::jmax(120, header.getWidth() - 130)));
    m_resetButton.setBounds(header.removeFromRight(120));

    m_helpLabel.setBounds(area.removeFromTop(24));
    m_statusLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);

    const auto columns = juce::jmax(1, area.getWidth() / minimumCellWidth);
    const auto cellWidth = area.getWidth() / columns;

    for (int i = 0; i < ComputerMidiKeyboardLayout::noteCount; ++i)
    {
        const int row = i / columns;
        const int column = i % columns;
        auto cell = juce::Rectangle<int>(area.getX() + column * cellWidth,
                                         area.getY() + row * cellHeight,
                                         cellWidth,
                                         cellHeight)
                        .reduced(2);

        auto labelBounds = cell.removeFromTop(18);
        m_noteLabels[(size_t) i]->setBounds(labelBounds);
        m_keyButtons[(size_t) i]->setBounds(cell);
    }

    const auto rows = (ComputerMidiKeyboardLayout::noteCount + columns - 1) / columns;
    area.removeFromTop(rows * cellHeight);
    area.removeFromTop(8);

    auto aliasRow = area.removeFromTop(28);
    m_upperCAliasLabel.setBounds(aliasRow.removeFromLeft(110));
    m_upperCAliasButton.setBounds(aliasRow.removeFromLeft(juce::jmin(180, aliasRow.getWidth())));
}

bool ComputerMidiKeyboardSettingsComponent::keyPressed(const juce::KeyPress &key)
{
    if (!m_captureSlot.has_value())
        return false;

    if (key == juce::KeyPress::escapeKey)
    {
        cancelCapture();
        return true;
    }

    if (key == juce::KeyPress::backspaceKey || key == juce::KeyPress::deleteKey)
    {
        applyCapturedDescription({});
        return true;
    }

    if (!key.isValid() || key.getModifiers().isAnyModifierKeyDown())
    {
        updateStatus("Use a single key without modifiers.", juce::Colours::salmon);
        return true;
    }

    applyCapturedDescription(key.getTextDescription());
    return true;
}

void ComputerMidiKeyboardSettingsComponent::focusLost(FocusChangeType cause)
{
    juce::ignoreUnused(cause);
    cancelCapture();
}

void ComputerMidiKeyboardSettingsComponent::beginCapture(CaptureSlot slot)
{
    m_captureSlot = slot;
    updateButtonTexts();
    updateStatus("Press a key for " + (slot.isUpperCAlias ? juce::String("Upper C alias") : m_noteLabels[(size_t) slot.index]->getText()) + ".",
                 juce::Colours::orange);
    grabKeyboardFocus();
}

void ComputerMidiKeyboardSettingsComponent::cancelCapture()
{
    const bool hadCapture = m_captureSlot.has_value();
    m_captureSlot.reset();
    updateButtonTexts();

    if (hadCapture)
        updateStatus("Capture cancelled.", withAlpha(m_appState.getTextColour(), 0.8f));
}

void ComputerMidiKeyboardSettingsComponent::applyCapturedDescription(const juce::String &description)
{
    if (!m_captureSlot.has_value())
        return;

    auto candidateState = m_state;
    const auto normalisedDescription = ComputerMidiKeyboardLayout::normaliseKeyDescription(description);

    if (m_captureSlot->isUpperCAlias)
        candidateState.upperCAliasDescription = normalisedDescription;
    else
        candidateState.primaryKeyDescriptions[(size_t) m_captureSlot->index] = normalisedDescription;

    if (const auto validationError = ComputerMidiKeyboardLayout::validate(candidateState); validationError.isNotEmpty())
    {
        updateStatus(validationError, juce::Colours::salmon);
        return;
    }

    m_state = std::move(candidateState);
    ComputerMidiKeyboardLayout::saveTo(m_appState, m_state);
    m_appState.saveState();
    m_captureSlot.reset();
    updateButtonTexts();
    updateStatus("Computer MIDI keyboard mapping updated.", withAlpha(m_appState.getTextColour(), 0.8f));
}

void ComputerMidiKeyboardSettingsComponent::updateButtonTexts()
{
    for (int i = 0; i < ComputerMidiKeyboardLayout::noteCount; ++i)
    {
        const CaptureSlot slot{i, false};
        auto &button = *m_keyButtons[(size_t) i];
        button.setButtonText(isCaptureActiveFor(slot)
                                 ? "Press key..."
                                 : ComputerMidiKeyboardLayout::getDisplayTextForDescription(m_state.primaryKeyDescriptions[(size_t) i]));
        button.setToggleState(isCaptureActiveFor(slot), juce::dontSendNotification);
    }

    const CaptureSlot aliasSlot{ComputerMidiKeyboardLayout::upperCIndex, true};
    m_upperCAliasButton.setButtonText(isCaptureActiveFor(aliasSlot)
                                          ? "Press key..."
                                          : ComputerMidiKeyboardLayout::getDisplayTextForDescription(m_state.upperCAliasDescription));
    m_upperCAliasButton.setToggleState(isCaptureActiveFor(aliasSlot), juce::dontSendNotification);
}

void ComputerMidiKeyboardSettingsComponent::updateStatus(const juce::String &text, juce::Colour colour)
{
    m_statusLabel.setText(text, juce::dontSendNotification);
    m_statusLabel.setColour(juce::Label::textColourId, colour);
}

bool ComputerMidiKeyboardSettingsComponent::isCaptureActiveFor(const CaptureSlot &slot) const
{
    return m_captureSlot.has_value() && m_captureSlot->index == slot.index && m_captureSlot->isUpperCAlias == slot.isUpperCAlias;
}

KeyboardSettingsComponent::KeyboardSettingsComponent(ApplicationViewState &appState, juce::KeyPressMappingSet &keyMappings)
    : m_appState(appState),
      m_computerMidiKeyboardSettings(appState),
      m_keyMappingEditor(keyMappings, true)
{
    addAndMakeVisible(m_viewport);
    m_viewport.setViewedComponent(&m_content, false);
    m_viewport.setScrollBarThickness(m_appState.getScrollbarThickness());
    m_viewport.setScrollBarsShown(true, false, true, false);

    m_computerMidiHeader.setText("Virtual MIDI keyboard keys", juce::dontSendNotification);
    m_computerMidiHeader.setJustificationType(juce::Justification::centredLeft);
    m_content.addAndMakeVisible(m_computerMidiHeader);

    m_content.addAndMakeVisible(m_computerMidiKeyboardSettings);

    m_commandHeader.setText("Application shortcuts", juce::dontSendNotification);
    m_commandHeader.setJustificationType(juce::Justification::centredLeft);
    m_content.addAndMakeVisible(m_commandHeader);

    m_content.addAndMakeVisible(m_keyMappingEditor);
    if (auto *tree = findTreeView(m_keyMappingEditor))
        tree->getViewport()->setScrollBarsShown(false, false, false, false);

    refreshThemeFromAppState();
    startTimer(100);
}

KeyboardSettingsComponent::~KeyboardSettingsComponent()
{
    stopTimer();
    m_viewport.setViewedComponent(nullptr, false);
}

void KeyboardSettingsComponent::resized()
{
    m_viewport.setBounds(getLocalBounds());

    // Reserve the scrollbar width so changing the content height cannot alter the
    // keyboard grid's column count and cause the layout to oscillate.
    const auto contentWidth = juce::jmax(1, m_viewport.getWidth() - m_viewport.getScrollBarThickness());
    const auto innerWidth = juce::jmax(1, contentWidth - sectionPadding * 2);
    const auto keyboardHeight = m_computerMidiKeyboardSettings.getPreferredHeight(innerWidth);
    const auto contentHeight = sectionPadding * 2 + 24 + keyboardHeight + 8 + 24 + 4 + m_keyMappingEditorHeight;

    m_content.setSize(contentWidth, juce::jmax(m_viewport.getHeight(), contentHeight));
    auto area = m_content.getLocalBounds().reduced(sectionPadding);

    m_computerMidiHeader.setBounds(area.removeFromTop(24));
    m_computerMidiKeyboardSettings.setBounds(area.removeFromTop(keyboardHeight));
    area.removeFromTop(8);
    m_commandHeader.setBounds(area.removeFromTop(24));
    area.removeFromTop(4);
    m_keyMappingEditor.setBounds(area.removeFromTop(m_keyMappingEditorHeight));
}

void KeyboardSettingsComponent::timerCallback()
{
    const auto preferredHeight = getKeyMappingEditorPreferredHeight();
    if (preferredHeight != m_keyMappingEditorHeight)
    {
        m_keyMappingEditorHeight = preferredHeight;
        resized();
    }
}

int KeyboardSettingsComponent::getKeyMappingEditorPreferredHeight()
{
    constexpr int resetButtonAreaHeight = 28;

    if (auto *tree = findTreeView(m_keyMappingEditor))
        if (auto *treeContent = tree->getViewport()->getViewedComponent())
            return juce::jmax(resetButtonAreaHeight, treeContent->getHeight() + resetButtonAreaHeight);

    return resetButtonAreaHeight;
}

void KeyboardSettingsComponent::refreshThemeFromAppState()
{
    const auto textColour = m_appState.getTextColour();

    m_computerMidiHeader.setColour(juce::Label::textColourId, textColour);
    m_commandHeader.setColour(juce::Label::textColourId, textColour);
    m_computerMidiKeyboardSettings.refreshThemeFromAppState();
    m_computerMidiKeyboardSettings.refreshFromAppState();
    m_keyMappingEditor.setColours(m_appState.getBackgroundColour2(), textColour);
    repaint();
}
