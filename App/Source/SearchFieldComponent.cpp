#include "SearchFieldComponent.h"

SearchFieldComponent::SearchFieldComponent()
{
    jassert(m_searchFunction != nullptr);


    m_searchField.setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colour(0xff171717));
    m_searchField.setColour(juce::TextEditor::ColourIds::shadowColourId, juce::Colour(0xff171717));
    m_searchField.setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colour(0xffffffff));
    m_searchField.setColour(juce::TextEditor::ColourIds::textColourId, juce::Colour(0xffffffff));
    m_searchField.setColour(juce::TextEditor::ColourIds::highlightColourId, juce::Colour(0xffffffff));

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

void SearchFieldComponent::paint ( juce::Graphics& g) 
{
    g.fillAll(juce::Colour(0xff171717));
    g.setColour(juce::Colour(0xff555555));
    g.drawHorizontalLine(getHeight() - 1, 0, getWidth()) ;
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
