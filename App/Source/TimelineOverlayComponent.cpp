#include "TimelineOverlayComponent.h"
#include "Utilities.h"

TimelineOverlayComponent::TimelineOverlayComponent(
        EditViewState &evs, tracktion_engine::Clip::Ptr clip)
    : m_editViewState (evs)
    , m_defaultClip(clip)
{
    //setInterceptsMouseClicks (false, true);
}

void TimelineOverlayComponent::paint(juce::Graphics &g)
{
    auto colour = m_defaultClip->getClipTrack ()->getColour ();
    updateClipRects ();
    for (auto cr : m_clipRects)
    {
        g.setColour (colour);
        g.fillRect (cr);
        g.setColour (juce::Colours::black);
        g.drawRect (cr);
    }

//    auto loopRange = m_editViewState.m_edit.getTransport ().getLoopRange ();
//    auto loopStartX = timeToX (loopRange.getStart ());
//    auto loopEndX = timeToX (loopRange.getEnd ());

//    g.setColour (juce::Colours::grey);
//    g.fillRect (loopStartX, getHeight () - 10, loopEndX - loopStartX, 10);
}

bool TimelineOverlayComponent::hitTest(int x, int y)
{
    updateClipRects ();
    for (auto cr : m_clipRects)
    {
        if (cr.contains (x, y)) return true;
    }
    return false;
}

void TimelineOverlayComponent::mouseMove(const juce::MouseEvent &e)
{
    updateClipRects ();
    for (auto cr : m_clipRects)
    {
        if (cr.contains (e.x, e.y))
        {
            if (e.x > cr.getHorizontalRange ().getStart ()
            && e.x < cr.getHorizontalRange ().getStart () + 10)
            {
                setMouseCursor (juce::MouseCursor::LeftEdgeResizeCursor);
                m_leftResized = true;
                m_rightResized = false;
            }
            else if (e.x > cr.getHorizontalRange ().getEnd () - 10
                     && e.x < cr.getHorizontalRange ().getEnd ())
            {
                setMouseCursor (juce::MouseCursor::RightEdgeResizeCursor);
                m_rightResized = true;
                m_leftResized = false;
            }
            else
            {
                setMouseCursor (juce::MouseCursor::DraggingHandCursor);
                m_leftResized = false;
                m_rightResized = false;
            }
        }
    }
}

void TimelineOverlayComponent::mouseExit(const juce::MouseEvent &e)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void TimelineOverlayComponent::mouseDown(const juce::MouseEvent &e)
{
    m_posAtMousedown = e.position;

}

void TimelineOverlayComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (auto mc = getMidiclipByPos (m_posAtMousedown.x))
    {
        if (m_leftResized)
        {

        }
        else if (m_rightResized)
        {

        }
        else
        {

        }
    }
}

std::vector<tracktion_engine::MidiClip *> TimelineOverlayComponent::getMidiClipsOfTrack()
{
    std::vector<te::MidiClip*> midiClips;
    if (auto at = dynamic_cast<te::AudioTrack*>(m_defaultClip->getTrack ()))
    {
        for (auto c : at->getClips ())
        {
            if (auto mc = dynamic_cast<te::MidiClip*>(c))
            {
                midiClips.push_back (mc);
            }
        }
    }
    return midiClips;
}

tracktion_engine::MidiClip *TimelineOverlayComponent::getMidiclipByPos(int y)
{
    for (auto & clip : getMidiClipsOfTrack ())
    {
        if (clip->getStartBeat () < xToBeats (y)
                &&  clip->getEndBeat () > xToBeats (y))
        {
            return clip;
        }
    }
    return nullptr;
}

int TimelineOverlayComponent::timeToX(double time)
{
    auto beats = m_editViewState.m_edit.tempoSequence.timeToBeats (time);
    return juce::roundToInt (((beats - m_editViewState.m_pianoX1)
                              *  getWidth())
                             / (m_editViewState.m_pianoX2 - m_editViewState.m_pianoX1));
}

double TimelineOverlayComponent::xToBeats(int x)
{
    return (double (x) / getWidth())
            * (m_editViewState.m_pianoX2 - m_editViewState.m_pianoX1)
            + m_editViewState.m_pianoX1;
}

void TimelineOverlayComponent::updateClipRects()
{
    m_clipRects.clear ();
    auto audiotrack = dynamic_cast<te::AudioTrack*>(m_defaultClip->getTrack ());
    if (audiotrack)
    {
        for (auto c : audiotrack->getClips ())
        {
            auto startX = timeToX (c->getPosition ().getStart ());
            auto endX = timeToX (c->getPosition ().getEnd ());
            juce::Rectangle<int> clipRect = {startX,getHeight () - (getHeight ()/3)
                                             , endX-startX, getHeight ()};
            m_clipRects.add(clipRect);
        }
    }
}
