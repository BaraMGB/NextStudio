/*
  ==============================================================================

    ClipComponent.h
    Created: 8 Jan 2020 11:53:16pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SongEditorState.h"
 
//==============================================================================
/*
*/
class ClipComponent    : public Component
                       , public ValueTree::Listener
{
public:
    ClipComponent(tracktion_engine::Clip& clip, SongEditorViewState& state)
        : m_engineClip(clip)
        , m_ClipPosAtMouseDown(0)
        , m_state(state)
    {

    }
    ~ClipComponent(){}
    void valueTreePropertyChanged(ValueTree&, const Identifier&) override
    {
    }
    void valueTreeChildAdded(ValueTree& parentTree, ValueTree&) override
    {
    }
    void valueTreeChildRemoved(ValueTree& parentTree, ValueTree&, int) override
    {
    }
    void valueTreeChildOrderChanged(ValueTree& parentTree, int, int) override
    {
    }
    void valueTreeParentChanged(ValueTree&) override {}

    void paint (Graphics&) override;
    void resized() override;

    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;

    void moveToTrack(tracktion_engine::Track& track)
    {
        m_engineClip.moveToTrack(track);
    }

    double getLength();
    double getStart();

    tracktion_engine::Clip& getEngineClip()
    {
        return m_engineClip;
    }

private:
    tracktion_engine::Clip& m_engineClip;
    Colour m_colour;
    double m_ClipPosAtMouseDown;
    SongEditorViewState& m_state;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipComponent)
};
