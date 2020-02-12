/*
  ==============================================================================

    TrackHeaderComponent.cpp
    Created: 7 Jan 2020 8:30:09pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackHeaderComponent.h"

//==============================================================================
TrackHeaderComponent::TrackHeaderComponent(tracktion_engine::AudioTrack& track):
    m_height(50),
    m_track(track)
{
    auto state = m_track.state;
    m_TrackLabel.setText(state[tracktion_engine::IDs::name].toString(),
                                    NotificationType::dontSendNotification);
    addAndMakeVisible(m_TrackLabel);

    addAndMakeVisible(m_muteButton);
    addAndMakeVisible(m_soloButton);
    addAndMakeVisible(m_armingButton);
    m_muteButton.setName("M");
    m_soloButton.setName("S");
    m_armingButton.setName("O");

    addAndMakeVisible(m_volumeKnob);
    m_volumeKnob.addListener(this); 
    m_volumeKnob.setRange(0, 127, 1);
    m_volumeKnob.setValue(m_track.getVolumePlugin()->volume 
        * m_volumeKnob.getMaxValue() - m_volumeKnob.getMinimum());
    m_volumeKnob.setSliderStyle(Slider::RotaryVerticalDrag);
    m_volumeKnob.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    

    addAndMakeVisible(m_peakDisplay);
}

TrackHeaderComponent::~TrackHeaderComponent()
{
}

//==============================================================================
void TrackHeaderComponent::paint(Graphics& g)
{
    Rectangle<float> area = getLocalBounds().toFloat();
    g.setColour(Colour(0xff181818));
    g.fillRect(area);

    auto state = m_track.state;
    g.setColour(Colour::fromString(state[tracktion_engine::IDs::colour].toString()));
    Rectangle<float> trackColorIndicator = area.removeFromLeft(18);
    g.fillRect(trackColorIndicator);
    g.setColour(Colour(0xff343434));
    g.drawRect(trackColorIndicator);
    g.drawRect(area);
}

void TrackHeaderComponent::resized()
{
    auto area = getLocalBounds();
    auto peakDisplay = area.removeFromRight(20);
    peakDisplay.reduce(2, 2);
    m_peakDisplay.setBounds(peakDisplay);
    auto volSlider = area.removeFromRight(area.getHeight());
    m_volumeKnob.setBounds(volSlider);

    auto buttonGroup = area.removeFromRight(area.getHeight());
    auto buttonwidth = buttonGroup.getWidth() / 2;
    auto buttonHeight = buttonGroup.getHeight() / 2;
    m_soloButton.setBounds(buttonGroup.getX(), buttonGroup.getY(), buttonwidth, buttonHeight);
    m_muteButton.setBounds(buttonGroup.getX(), buttonGroup.getY() + buttonHeight, buttonwidth, buttonHeight);
    m_armingButton.setBounds(buttonGroup.getX() + buttonwidth, buttonGroup.getY(), buttonwidth, buttonHeight);

    area.removeFromLeft(20);
    m_TrackLabel.setBounds(area);
}

void TrackHeaderComponent::sliderValueChanged(Slider* slider)
{
    if (slider == &m_volumeKnob)
    {
        m_track.getVolumePlugin()->volume = m_volumeKnob.getValue() / m_volumeKnob.getMaximum();
    }
}





