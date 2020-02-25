/*
  ==============================================================================

    ClipComponent.h
    Created: 8 Jan 2020 11:53:16pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

 
//==============================================================================
/*
*/
class ClipComponent    : public Component
                       , public ValueTree::Listener
{
public:
    ClipComponent(tracktion_engine::Clip& clip, const int& pixelPerBeat)
        : m_engineClip(clip)
        , m_ClipPosAtMouseDown(0)
        , m_pixelPerBeat(pixelPerBeat)
    {

    }
    ~ClipComponent(){}
    void valueTreePropertyChanged(ValueTree&, const Identifier&) override
    {
        resized();
    }

    void valueTreeChildAdded(ValueTree& parentTree, ValueTree&) override { resized(); repaint(); }
    void valueTreeChildRemoved(ValueTree& parentTree, ValueTree&, int) override { resized(); repaint();}
    void valueTreeChildOrderChanged(ValueTree& parentTree, int, int) override { resized(); repaint(); }
    void valueTreeParentChanged(ValueTree&) override {}

    void paint (Graphics&) override;
    void resized() override;

    void mouseDown(const MouseEvent& event);
    void mouseDrag(const MouseEvent& event);

    double getLength();
    double getStart();

private:
    tracktion_engine::Clip& m_engineClip;
    Colour m_colour;
    double m_ClipPosAtMouseDown;
    const int& m_pixelPerBeat;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipComponent)
};
