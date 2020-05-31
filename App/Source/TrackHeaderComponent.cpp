/*
  ==============================================================================

    TrackHeaderComponent.cpp
    Created: 7 Jan 2020 8:30:09pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackHeaderComponent.h"
#include "Utilities.h"

//==============================================================================
TrackHeaderComponent::TrackHeaderComponent(SongEditorViewState& songEditorState, tracktion_engine::Track* track)
    : m_track(track)
    , m_songEditorState(songEditorState)
    , m_height(track->defaultTrackHeight)

{

    auto trackstate = m_track->state;
    trackstate.addListener(this);
    m_TrackLabel.setText(m_track->getName(), NotificationType::dontSendNotification);
    m_TrackLabel.setColour(Label::textColourId, Colours::white);
    m_TrackLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(m_TrackLabel);
    addAndMakeVisible(m_muteButton);
    addAndMakeVisible(m_soloButton);
    addAndMakeVisible(m_armingButton);
    m_muteButton.setName("M");
    m_soloButton.setName("S");
    m_soloButton.setComponentID("solo");
    m_muteButton.setComponentID("mute");
    m_soloButton.getToggleStateValue().referTo(trackstate.getPropertyAsValue("solo",nullptr));
    m_muteButton.getToggleStateValue().referTo(trackstate.getPropertyAsValue("mute",nullptr));
    m_armingButton.setName("O");
    auto audioTrack = dynamic_cast<tracktion_engine::AudioTrack*>(m_track);
    if (audioTrack)
    {
        m_armingButton.setToggleState (EngineHelpers::isTrackArmed (*audioTrack), dontSendNotification);
        m_armingButton.onClick = [this, audioTrack]
        {
            EngineHelpers::armTrack (*audioTrack, !EngineHelpers::isTrackArmed (*audioTrack));
            m_armingButton.setToggleState (EngineHelpers::isTrackArmed (*audioTrack), dontSendNotification);
        };
        m_volumeKnob.setOpaque(false);
        addAndMakeVisible(m_volumeKnob);
        m_volumeKnob.addListener(this); 
        m_volumeKnob.setRange(0.0f, 3.0f, 0.01f);
        m_volumeKnob.setSkewFactorFromMidPoint(1.0f);
        if (audioTrack->getVolumePlugin())
        {
            m_volumeKnob.getValueObject().referTo(audioTrack->getVolumePlugin()->volume.getPropertyAsValue());

        }
        m_volumeKnob.setSliderStyle(Slider::RotaryVerticalDrag);
        m_volumeKnob.setTextBoxStyle(Slider::NoTextBox, 0, 0, false);
        addAndMakeVisible(m_peakDisplay);
    }
}

TrackHeaderComponent::~TrackHeaderComponent()
{
}

//==============================================================================
void TrackHeaderComponent::paint(Graphics& g)
{
    Rectangle<float> area = getLocalBounds().toFloat();
    
    g.setColour(m_track->getColour());
    Rectangle<float> trackColorIndicator = area.removeFromLeft(18);
    g.fillRect(trackColorIndicator);
    g.setColour(Colour(0xff343434));
    g.drawRect(trackColorIndicator);
    g.drawRect(area);
    area.reduce(1, 1);
    if (m_songEditorState.m_selectionManager.isSelected(m_track))
    {
        g.setColour(Colour(0xff383838));
    }
    else
    {
        g.setColour(Colour(0xff181818));
    }

    g.fillRect(area);
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

void TrackHeaderComponent::mouseDown(const MouseEvent& event)
{
    if(!event.mouseWasDraggedSinceMouseDown())
    {
        if (event.mods.isRightButtonDown())
        {
            PopupMenu m;
            m.addItem(1, "delete Track");
            m.addItem(2, "item 2");

            const int result = m.show();

            if (result == 0)
            {
                // user dismissed the menu without picking anything
            }
            else if (result == 1)
            {
                m_track->edit.deleteTrack(m_track);
                // user picked item 1
            }
            else if (result == 2)
            {
                // user picked item 2
            }
        }
        else if (event.mods.isShiftDown())
        {
            if (m_songEditorState.m_selectionManager.getNumObjectsSelected())
            {
                m_songEditorState.m_selectionManager.addToSelection(m_track);
            }
        }
        else
        {
            m_songEditorState.m_selectionManager.selectOnly(m_track);
        }
    }
}

void TrackHeaderComponent::sliderValueChanged(Slider* slider)
{
    if (slider == &m_volumeKnob)
    {
        auto audioTrack = dynamic_cast<tracktion_engine::AudioTrack*>(m_track);
        if (audioTrack)
        {
        }
    }
}
