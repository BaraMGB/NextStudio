#include "TimelineOverlayComponent.h"
#include "Utilities.h"

TimelineOverlayComponent::TimelineOverlayComponent(
        EditViewState &evs
      , tracktion_engine::Track::Ptr track
      , TimeLineComponent& tlc)
    : m_editViewState (evs)
    , m_track(track)
    , m_timelineComponent(tlc)
{
    //setInterceptsMouseClicks (false, true);
}

void TimelineOverlayComponent::paint(juce::Graphics &g)
{
    auto colour = m_track->getColour ();
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
    if (auto mc = getMidiclipByPos (e.x))
    {
        m_cachedClip = mc;
        m_cachedPos = mc->getPosition ();
    }
}

void TimelineOverlayComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (e.mouseWasDraggedSinceMouseDown ())
    {
        auto offset = e.getMouseDownX () - timeToX (m_cachedPos.getStart ());
        if (m_cachedClip)
        {
            if (m_leftResized)
            {

            }
            else if (m_rightResized)
            {

            }
            else
            {
                auto newStart = m_editViewState.beatToTime (xToBeats (e.x - offset));
                auto snaped = m_timelineComponent.getBestSnapType ().roundTimeDown (
                            newStart, m_editViewState.m_edit.tempoSequence);
                newStart = e.mods.isShiftDown () ? newStart
                                                 : snaped;
                m_cachedClip->setStart (newStart, false, true);
            }
        }
    }
}

std::vector<tracktion_engine::MidiClip *> TimelineOverlayComponent::getMidiClipsOfTrack()
{
    std::vector<te::MidiClip*> midiClips;
    if (auto at = dynamic_cast<te::AudioTrack*>(&(*m_track)))
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

tracktion_engine::MidiClip *TimelineOverlayComponent::getMidiclipByPos(int x)
{
    for (auto & clip : getMidiClipsOfTrack ())
    {
        if (clip->getStartBeat () < xToBeats (x)
                &&  clip->getEndBeat () > xToBeats (x))
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
    if (auto audiotrack = dynamic_cast<te::AudioTrack*>(&(*m_track)))
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
