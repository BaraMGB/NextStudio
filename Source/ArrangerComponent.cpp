/*
  ==============================================================================

    ArrangerComponent.cpp
    Created: 7 Jan 2020 8:29:38pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "ArrangerComponent.h"

//==============================================================================
ArrangerComponent::ArrangerComponent(
    OwnedArray<TrackHeaderComponent>& trackComps, tracktion_engine::Edit & edit)
    : m_trackComponents(trackComps)
    , m_edit(edit)
    , m_pixelPerBeats(30)
{
    addAndMakeVisible(m_positionLine);
    setSize(3000, 300);
}

ArrangerComponent::~ArrangerComponent()
{
}

void ArrangerComponent::paint (Graphics& g)
{
    g.fillAll (Colour(0xff181818));
    auto area = getLocalBounds();
    g.setColour (Colour(0xff343434));

    for(auto& track : m_trackComponents)
    {
        auto height = track->getTrackheight();
        area.removeFromTop(height);
        g.drawLine(area.getX(), area.getY(), area.getWidth(), area.getY());
    }

    auto beatLine = 0, barline = 0;
    while (beatLine < getWidth())
    {
        barline++;
        g.setColour(Colour(0xff242424));
        if (barline == 4)
        {
            g.setColour(Colour(0xff343434));
            barline = 0;
        }
        beatLine = beatLine + getPixelPerBeats();
        if (m_pixelPerBeats > 8.0 || barline == 0)
        {
            g.drawLine(beatLine, 0, beatLine, getHeight());
        }
    }
}

void ArrangerComponent::resized()
{
    auto area = getLocalBounds();
    for (auto& trackComp : m_trackComponents)
    {
        for (auto& clipComp : *trackComp->getClipComponents())
        {
            const double length = m_edit.tempoSequence.timeToBeats(clipComp->getLength()) * m_pixelPerBeats;
            const double start  = m_edit.tempoSequence.timeToBeats(clipComp->getStart())  * m_pixelPerBeats;
            auto clipRect = Rectangle<int>(area.getX() + start,
                area.getY(),
                length,
                trackComp->getTrackheight());
            clipComp->setBounds(clipRect.reduced(0,1));
        }
        area.removeFromTop(trackComp->getTrackheight());
    }
    double pos = m_edit.tempoSequence.timeToBeats(m_edit.getTransport().getCurrentPosition()) * static_cast<int>(m_pixelPerBeats);
    m_positionLine.setBounds(pos, 0, 1, getLocalBounds().getHeight());
    m_positionLine.setAlwaysOnTop(true);
}

inline void ArrangerComponent::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
    m_pixelPerBeats = jmax(1.0,(m_pixelPerBeats + (wheel.deltaY * 10.0)));
    sendChangeMessage();
    repaint();
}
