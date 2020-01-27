/*
  ==============================================================================

    HeaderComponent.cpp
    Created: 7 Jan 2020 8:31:11pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "HeaderComponent.h"


//==============================================================================
HeaderComponent::HeaderComponent(int width, int height, tracktion_engine::Edit& edit)
    : m_edit(edit)
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
    m_transportDisplay.addSpinBox(4,1,1,999,".");
    m_transportDisplay.addSpinBox(1,1,1,4,".");
    m_transportDisplay.addSpinBox(3,0,0,960,"");

    startTimer(100);

    setSize(width, height);

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

    area.reduce(5, 5);
    m_loadButton.setBounds(area.removeFromLeft(area.getHeight()));
    area.removeFromLeft(5);
    m_saveButton.setBounds(area.removeFromLeft(area.getHeight()));
    area.removeFromLeft(5);
    //space
    area.removeFromLeft(area.getHeight());

    m_playButton.setBounds(area.removeFromLeft(area.getHeight()));
    area.removeFromLeft(5);
    m_stopButton.setBounds(area.removeFromLeft(area.getHeight()));
    area.removeFromLeft(5);
    m_recordButton.setBounds(area.removeFromLeft(area.getHeight()));
    area.removeFromLeft(5);
    //space
    area.removeFromLeft(area.getHeight());

    m_transportDisplay.setBounds(area.removeFromLeft(area.getHeight()*m_transportDisplay.getSpinBoxCount()));
    area.removeFromLeft(5);





}

void HeaderComponent::buttonClicked(Button* button)
{
    if (button == &m_playButton)
    {
        auto& transport = m_edit.getTransport();

        if (transport.isPlaying())
            transport.stop(false, false);
        else
            transport.play(false);

        m_playButton.setButtonText(m_edit.getTransport().isPlaying() ? "Pause" : "Play");
    }
    if (button == &m_stopButton)
    {
        m_edit.getTransport().stop(false,true);
        m_edit.getTransport().setCurrentPosition(0);
    }
}

void HeaderComponent::timerCallback()
{
    auto const &tempoSequence = m_edit.tempoSequence;
    double time = m_edit.getTransport().getCurrentPosition();
    if (!m_transportDisplay.isDragging())
    {
        String timeDisplay[4];
        m_edit.getTimecodeFormat().getPartStrings(time,tempoSequence, false, timeDisplay);
        for (auto i = 0; i < 3; i++)
        {
            m_transportDisplay.setValue( m_transportDisplay.getSpinBoxCount() - 1 - i, timeDisplay[i].getIntValue());
        }
    }
    else
    {
        tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
        pos.setTime(0);
        pos.addBars((m_transportDisplay.getValue(0) - 1));
        pos.addBeats((m_transportDisplay.getValue(1) - 1));
        pos.addBeats((m_transportDisplay.getValue(2) / (double) m_edit.ticksPerQuarterNote));

        m_edit.getTransport().setCurrentPosition(pos.getTime());
    }
}

