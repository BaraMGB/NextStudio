#include "SearchFieldComponent.h"

SearchFieldComponent::SearchFieldComponent()
{
    jassert(m_searchFunction != nullptr);

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
    m_label.setText("Find:", juce::dontSendNotification);
    m_label.attachToComponent(&m_searchField, true);
}

void SearchFieldComponent::resized()
{
    auto area = getLocalBounds();
    auto labelWidth = 50;
    auto buttonWidth = 40;

    m_label.setBounds(area.removeFromLeft(labelWidth));
    m_searchField.setBounds(area.removeFromLeft(area.getWidth() - buttonWidth).reduced(5));
    m_clearButton.setBounds(area.reduced(5));
}

juce::String SearchFieldComponent::getText()
{
    return m_searchField.getText();
}
