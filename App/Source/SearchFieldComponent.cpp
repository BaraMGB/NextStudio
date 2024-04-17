#include "SearchFieldComponent.h"
#include "ApplicationViewState.h"
#include "Utilities.h"

SearchFieldComponent::SearchFieldComponent(ApplicationViewState& appState)
   : m_appState(appState) 
{
    m_searchField.setColour(juce::TextEditor::ColourIds::backgroundColourId, m_appState.getMenuBackgroundColour());
    m_searchField.setColour(juce::TextEditor::ColourIds::shadowColourId, m_appState.getMenuBackgroundColour().darker(0.3f));
    m_searchField.setColour(juce::TextEditor::ColourIds::outlineColourId, m_appState.getBorderColour());
    m_searchField.setColour(juce::TextEditor::ColourIds::textColourId, m_appState.getTextColour());
    m_searchField.setColour(juce::TextEditor::ColourIds::highlightColourId, m_appState.getPrimeColour());

    addAndMakeVisible(m_searchField);

    m_searchField.onTextChange = [this] {
        sendChangeMessage();
    };

    m_clearButton.setWantsKeyboardFocus(false);

    addAndMakeVisible(m_clearButton);
    m_clearButton.setButtonText("X");
    m_clearButton.onClick = [this] {
        m_searchField.clear();
        sendChangeMessage();
    };

    addAndMakeVisible(m_label);
    m_label.setText(GUIHelpers::translate("Find", m_appState) + ":  ", juce::dontSendNotification);
    m_label.attachToComponent(&m_searchField, true);
}

void SearchFieldComponent::resized()
{
    auto area = getLocalBounds();
    auto labelWidth = 50;
    auto buttonWidth = 40;

    m_label.setBounds(area.removeFromLeft(labelWidth));
    m_searchField.setBounds(area.removeFromLeft(area.getWidth() - buttonWidth).reduced(3));
    m_clearButton.setBounds(area.reduced(5));
}

juce::String SearchFieldComponent::getText()
{
    return m_searchField.getText();
}
