/*
  ==============================================================================

    ArrangerComponent.h
    Created: 7 Jan 2020 8:29:38pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "TrackHeaderComponent.h"
#include "ClipComponent.h"

class PositionLine : public Component
{
    void paint(Graphics& g) override
    {
        g.fillAll(Colours::white);
    }
};

//==============================================================================
/*
*/
class ArrangerComponent    : public Component
                           , public ChangeBroadcaster
                           , public DragAndDropTarget
{
public:
    ArrangerComponent(OwnedArray<TrackHeaderComponent>& trackComps, tracktion_engine::Edit& edit, SongEditorViewState& state);
    ~ArrangerComponent();

    void paint (Graphics&) override;
    void resized() override;
    void updatePositionLine();

    bool isInterestedInDragSource(const SourceDetails& /*dragSourceDetails*/) override { return true; }
    void itemDropped(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;

    void ClipsMoved();
    void mouseDown(const MouseEvent& event) override;
    void mouseWheelMove(const MouseEvent& event,
        const MouseWheelDetails& wheel) override;

    void setYpos(int yPos)
    {
        m_yPos = yPos;
    }
    int getPixelPerBeat()
    {
        return m_state.m_pixelPerBeat;
    }

private:

    OwnedArray<TrackHeaderComponent>& m_trackComponents;
    tracktion_engine::Edit& m_edit;
    SongEditorViewState& m_state;
    PositionLine m_positionLine;
    int m_yPos{ 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArrangerComponent)
};
