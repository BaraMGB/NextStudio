/*
  ==============================================================================

    HeaderComponent.cpp
    Created: 7 Jan 2020 8:31:11pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "HeaderComponent.h"
#include "Utilities.h"

//==============================================================================
HeaderComponent::HeaderComponent(int height, int width, tracktion_engine::Edit * m_edit)
    : m_edit(m_edit)
{
    addAndMakeVisible(m_loadButton);
    m_loadButton.setButtonText("Load");
    m_loadButton.addListener(this);

    addAndMakeVisible(m_saveButton);
    m_saveButton.setButtonText("Save");
    m_saveButton.addListener(this);

    addAndMakeVisible(m_playButton);
    m_playButton.setButtonText("Play");
    m_playButton.addListener(this);

    addAndMakeVisible(m_stopButton);
    m_stopButton.setButtonText("Stop");
    m_stopButton.addListener(this);

    addAndMakeVisible(m_recordButton);
    m_recordButton.setButtonText("Record");
    m_recordButton.addListener(this);

    addAndMakeVisible(m_transportDisplay);

    startTimer(10);

    setSize(height, width);

}

HeaderComponent::~HeaderComponent()
{
}

void HeaderComponent::paint (Graphics& g)
{
    g.fillAll(Colour(0xff242424));

}

void HeaderComponent::resized()
{
    juce::Rectangle<int> area = getLocalBounds();
    FlexBox flexbox{ FlexBox::Direction::row, FlexBox::Wrap::noWrap, FlexBox::AlignContent::center,
                     FlexBox::AlignItems::flexStart, FlexBox::JustifyContent::flexStart };
    int border = area.getHeight() / 6;
    int buttonheight = getHeight() - border;


    flexbox.items.add(FlexItem(buttonheight, buttonheight, m_loadButton).withMargin(FlexItem::Margin(3.0f)));
    flexbox.items.add(FlexItem(buttonheight, buttonheight, m_saveButton).withMargin(FlexItem::Margin(3.0f)));
    flexbox.items.add(FlexItem(buttonheight, buttonheight / 2));
    flexbox.items.add(FlexItem(buttonheight, buttonheight, m_playButton).withMargin(FlexItem::Margin(3.0f)));
    flexbox.items.add(FlexItem(buttonheight, buttonheight, m_stopButton).withMargin(FlexItem::Margin(3.0f)));
    flexbox.items.add(FlexItem(buttonheight, buttonheight, m_recordButton).withMargin(FlexItem::Margin(3.0f)));
    flexbox.items.add(FlexItem(m_transportDisplay.getWidth(), buttonheight, m_transportDisplay).withMargin(FlexItem::Margin(3.0f)));


    flexbox.performLayout(getLocalBounds());
}

void HeaderComponent::buttonClicked(Button* button)
{
    if (button == &m_playButton)
    {
        EngineHelpers::togglePlay(*m_edit);
        m_playButton.setButtonText(m_edit->getTransport().isPlaying() ? "Pause" : "Play");
    }
    if (button == &m_stopButton)
    {
        m_edit->getTransport().stop(false,true);
        m_edit->getTransport().setCurrentPosition(0);
    }
}

void HeaderComponent::timerCallback()
{

    auto posInSec = m_edit->getTransport().getCurrentPosition();
    m_transportDisplay.setPosition(getPPQPos(posInSec));
}

