/*
  ==============================================================================

    TrackHeaderComponent.h
    Created: 7 Jan 2020 8:30:09pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ClipComponent.h"


//==============================================================================
/*
*/
class PeakDisplayComponent : public Component
{
public:
    PeakDisplayComponent() {};
    ~PeakDisplayComponent() {};

    void paint(Graphics& g) override
    {
        auto area = getLocalBounds();
        g.setColour(Colour(0xff343434));
        g.drawRect(area);
    }
};

class TrackHeaderComponent    : public Component
                              , public Slider::Listener
                              , public ChangeBroadcaster
{
public:
    TrackHeaderComponent(tracktion_engine::Edit& edit);
    ~TrackHeaderComponent();

    void paint(Graphics& g) override;
    void resized() override;
    void sliderValueChanged(Slider* slider) override;

    Colour trackColour() const;
    void setTrackColour(const Colour& trackColour);
    void setTrackName(String trackName)
    {
        m_trackName = trackName;
        m_TrackLabel.setText(m_trackName, NotificationType::dontSendNotification);
    }
    String getTrackName()
    {
        return m_trackName;
    }

    int getTrackheight()
    {
        return m_height;
    }

    void setTrackHeight(int height)
    {
        m_height = height;
    }

    ClipComponent* createClip(double position, double length)
    {
        auto clip = new ClipComponent(position, length);
        m_clips.add(clip);
        sendChangeMessage();
        return clip;
    }

    OwnedArray<ClipComponent>* getClips()
    {
        return &m_clips;
    }

private:
    Colour m_trackColour;
    Label m_TrackLabel;
    ToggleButton m_muteButton;
    ToggleButton m_soloButton;
    ToggleButton m_armingButton;
    Slider m_volumeKnob;
    PeakDisplayComponent m_peakDisplay;
    String m_trackName;
    int m_height;
    OwnedArray<ClipComponent> m_clips;
    tracktion_engine::Edit& m_edit;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackHeaderComponent)
};
