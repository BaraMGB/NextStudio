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
HeaderComponent::HeaderComponent(tracktion_engine::Edit& edit)
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

    m_transportDisplay.setFont(juce::Font(30));
    m_transportDisplay.setFontColour(m_mainColour);
    addAndMakeVisible(m_transportDisplay);
    m_transportDisplay.addSpinBox(3,1,1,999,".");
    m_transportDisplay.addSpinBox(1,1,1,4,".");
    m_transportDisplay.addSpinBox(3,0,0,960,"");

    m_BpmDisplay.setFont(juce::Font(15));
    m_BpmDisplay.setFontColour(m_mainColour);
    addAndMakeVisible(m_BpmDisplay);
    m_BpmDisplay.addSpinBox(3, 130, 20, 999, ".");
    m_BpmDisplay.addSpinBox(2, 0, 0, 99, "");

    m_signatureDisplay.setFont(juce::Font(15));
    m_signatureDisplay.setFontColour(m_mainColour);
    addAndMakeVisible(m_signatureDisplay);
    m_signatureDisplay.addSpinBox(2, 4, 1, 16, "/");
    m_signatureDisplay.addSpinBox(2, 4, 1, 16, "");

    m_LoopBeginDisplay.setFont(juce::Font(15));
    m_LoopBeginDisplay.setFontColour(m_mainColour);
    addAndMakeVisible(m_LoopBeginDisplay);
    m_LoopBeginDisplay.addSpinBox(3, 1, 1, 999, ".");
    m_LoopBeginDisplay.addSpinBox(1, 1, 1, 4, ".");
    m_LoopBeginDisplay.addSpinBox(3, 0, 0, 960, "");

    m_LoopEndDisplay.setFont(juce::Font(15));
    m_LoopEndDisplay.setFontColour(m_mainColour);
    addAndMakeVisible(m_LoopEndDisplay);
    m_LoopEndDisplay.addSpinBox(3, 1, 1, 999, ".");
    m_LoopEndDisplay.addSpinBox(1, 1, 1, 4, ".");
    m_LoopEndDisplay.addSpinBox(3, 0, 0, 960, "");


    startTimer(100);
}

HeaderComponent::~HeaderComponent()
{
}

void HeaderComponent::paint (Graphics& g)
{
    juce::Rectangle<int> area = getLocalBounds();
    auto displayWidth = m_BpmDisplay.getNeededWidth() + 20
        + m_transportDisplay.getNeededWidth() + 20
        + m_LoopBeginDisplay.getNeededWidth() + 20;
    auto display = area.removeFromRight(area.getWidth() / 2 + displayWidth / 2);
    display.removeFromBottom(5);
    display.setWidth(displayWidth);
    display.expand(30, 0);
    g.setColour(Colour(0xff181818));
    g.fillRect(display);

}

void HeaderComponent::resized()
{
    juce::Rectangle<int> area = getLocalBounds();
    auto displayWidth = m_BpmDisplay.getNeededWidth() + 20
                                         + m_transportDisplay.getNeededWidth() + 20
                                         + m_LoopBeginDisplay.getNeededWidth() + 20;
    auto display = area.removeFromRight(area.getWidth() / 2 + displayWidth / 2);


    area.removeFromLeft(5);
    area.removeFromBottom(5);
    m_loadButton.setBounds(area.removeFromLeft(area.getHeight() + 10));
    area.removeFromLeft(5);
    m_saveButton.setBounds(area.removeFromLeft(area.getHeight() + 10));

    area.removeFromRight(50);
    m_recordButton.setBounds(area.removeFromRight(area.getHeight()+10));
    area.removeFromRight(5);
    m_stopButton.setBounds(area.removeFromRight(area.getHeight() + 10));
    area.removeFromRight(5);
    m_playButton.setBounds(area.removeFromRight(area.getHeight() + 10));
    
    auto leftSide = display.removeFromLeft(m_BpmDisplay.getNeededWidth() + 20);
    auto center = display.removeFromLeft(m_transportDisplay.getNeededWidth() + 20);
    auto rightSide = display;

    
    m_BpmDisplay.setBounds(leftSide.removeFromTop(leftSide.getHeight()/2));
    m_signatureDisplay.setBounds(leftSide);

    m_transportDisplay.setBounds(center);

    m_LoopBeginDisplay.setBounds(rightSide.removeFromTop(rightSide.getHeight() / 2));
    m_LoopEndDisplay.setBounds(rightSide);
    
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
        m_playButton.setButtonText("Play");
    }
}

void HeaderComponent::timerCallback()
{
    auto const &tempoSequence = m_edit.tempoSequence;
    double time = m_edit.getTransport().getCurrentPosition();


    if (m_transportDisplay.isDragging())
    {
        m_edit.getTransport().setUserDragging(true);
        tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
        pos.setTime(0);
        pos.addBars((m_transportDisplay.getValue(0) - 1));
        pos.addBeats((m_transportDisplay.getValue(1) - 1.0));
        pos.addBeats((m_transportDisplay.getValue(2) / (double) m_edit.ticksPerQuarterNote));

        m_edit.getTransport().setCurrentPosition(pos.getTime());
    }
    else
    {
        m_edit.getTransport().setUserDragging(false);
        String timeDisplay[4];
        m_edit.getTimecodeFormat().getPartStrings(time, tempoSequence, false, timeDisplay);
        for (auto i = 0; i < 3; i++)
        {
            m_transportDisplay.setValue(m_transportDisplay.getSpinBoxCount() - 1 - i, timeDisplay[i].getIntValue());
        }
    }

    if (m_BpmDisplay.isDragging())
    {
        m_edit.getTransport().setUserDragging(true);
        tracktion_engine::TempoSequencePosition pos(m_edit.tempoSequence);
        tempoSequence.getTempos()[0]->setBpm(static_cast<double>(m_BpmDisplay.getValue(0) + (m_BpmDisplay.getValue(1) * 0.01)));
        pos.setTime(0);
        pos.addBars((m_transportDisplay.getValue(0) - 1));
        pos.addBeats((m_transportDisplay.getValue(1) - 1.0));
        pos.addBeats((m_transportDisplay.getValue(2) / (double)m_edit.ticksPerQuarterNote));

        m_edit.getTransport().setCurrentPosition(pos.getTime());
    }
    else
    {
        m_edit.getTransport().setUserDragging(false);
        double bpm = tempoSequence.getTempos()[0]->bpm;
        int wholeBpm = static_cast<int>(bpm);
        double fractionalBpm = bpm - wholeBpm;
        m_BpmDisplay.setValue(0, wholeBpm);
        m_BpmDisplay.setValue(1, fractionalBpm / 0.01);
    }

    if (m_signatureDisplay.isDragging())
    {
        m_edit.getTransport().setUserDragging(true);
  
        tempoSequence.getTimeSig(0)->numerator = m_signatureDisplay.getValue(0);
        tempoSequence.getTimeSig(0)->denominator = m_signatureDisplay.getValue(1);
    }
    else
    {
        m_edit.getTransport().setUserDragging(false);
  
        m_signatureDisplay.setValue(0, tempoSequence.getTimeSig(0)->numerator);
        m_signatureDisplay.setValue(1, tempoSequence.getTimeSig(0)->denominator);
    }
}

