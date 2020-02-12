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
                              , public ValueTree::Listener
                              , public Slider::Listener
{
public:
    TrackHeaderComponent(tracktion_engine::AudioTrack & track);
    ~TrackHeaderComponent();

    void paint(Graphics& g) override;
    void resized() override;
    void valueTreePropertyChanged(ValueTree&, const Identifier&) override
    {
        resized();
    }

    void valueTreeChildAdded(ValueTree& parentTree, ValueTree&) override { resized(); repaint(); }
    void valueTreeChildRemoved(ValueTree& parentTree, ValueTree&, int) override { resized(); repaint(); }
    void valueTreeChildOrderChanged(ValueTree& parentTree, int, int) override { resized(); repaint(); }
    void valueTreeParentChanged(ValueTree&) override {}

    void sliderValueChanged(Slider* slider);
    int getTrackheight()
    {
        return m_height;
    }

    void setTrackHeight(int height)
    {
        m_height = height;
    }

    ClipComponent* createClip(tracktion_engine::Clip& engineClip)
    {
        auto clipComp = new ClipComponent(engineClip);
        m_clipComponents.add(clipComp);
        return clipComp;
    }

    OwnedArray<ClipComponent>* getClipComponents()
    {
        return &m_clipComponents;
    }

private:
    Label                     m_TrackLabel;
    ToggleButton              m_muteButton,
                              m_soloButton,
                              m_armingButton;
    Slider                    m_volumeKnob;
    PeakDisplayComponent      m_peakDisplay;
    int                       m_height;
    OwnedArray<ClipComponent> m_clipComponents;
    tracktion_engine::AudioTrack& m_track;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackHeaderComponent)
};
