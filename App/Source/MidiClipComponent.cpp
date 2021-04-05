#include "MidiClipComponent.h"

MidiClipComponent::MidiClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : ClipComponent (evs, c)
{
    setBufferedToImage(true);
}

MidiClipComponent::~MidiClipComponent()
{
    removeAllChangeListeners ();
}

void MidiClipComponent::paint (juce::Graphics& g)
{
    auto startX = m_editViewState.timeToX(getClip()->getPosition().getStart(),
                                          getParentComponent()->getWidth());
    auto endX = m_editViewState.timeToX(getClip()->getPosition().getEnd(),
                                        getParentComponent()->getWidth());
    if (!(endX < 0 || startX > getParentComponent()->getWidth()))
    {
        ClipComponent::paint(g);
        auto clipHeader = 10;
        if (auto mc = getMidiClip())
        {
            auto& seq = mc->getSequence();
            for (auto n: seq.getNotes())
            {
                double sBeat = n->getStartBeat() - mc->getOffsetInBeats();
                double eBeat = n->getEndBeat() - mc->getOffsetInBeats();
                if (auto p = getParentComponent())
                {
                    double y = ((1.0 - double(n->getNoteNumber()) / 127.0)
                                * (getHeight() - clipHeader)
                                + clipHeader);

                    auto x1 = m_editViewState.beatsToX(
                                sBeat + m_editViewState.m_viewX1, p->getWidth());
                    auto x2 = m_editViewState.beatsToX(
                                eBeat + m_editViewState.m_viewX1, p->getWidth());

                    g.setColour(juce::Colours::white);
                    g.drawLine(float(x1), float(y), float(x2), float(y));
                }
            }
        }
    }
}

void MidiClipComponent::mouseExit(const juce::MouseEvent &/*e*/)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void MidiClipComponent::mouseDown(const juce::MouseEvent &e)
{
    m_mouseDownX = e.getMouseDownX();
    if (!(m_mouseDownX < 10
            || m_mouseDownX > getWidth () - 10)
       && m_editViewState.m_isPianoRollVisible)
    {
        auto pianorollZoom = m_editViewState.m_pianoX2
                           - m_editViewState.m_pianoX1;

        m_editViewState.m_pianoX1 = juce::jmax(0.0
                                             , m_clip->getStartBeat ()
                                                 - (pianorollZoom/2)
                                                 + (m_clip->getLengthInBeats ()/2));
        m_editViewState.m_pianoX2 = m_editViewState.m_pianoX1 + pianorollZoom;
    }
    m_posAtMouseDown =  m_clip->getPosition();
    m_clipWidthMouseDown = getWidth();
    m_oldDistTime = 0.0;
    ClipComponent::mouseDown(e);
    if (e.getNumberOfClicks () > 1
     || m_editViewState.m_isPianoRollVisible)
    {
        m_editViewState.m_isPianoRollVisible = true;
        sendChangeMessage ();
    }
}

void MidiClipComponent::mouseDrag(const juce::MouseEvent &e)
{
    const auto distanceBeats = m_editViewState.xToBeats(
                e.getDistanceFromDragStartX(),getParentWidth());
    auto snapType = m_editViewState.getBestSnapType (
                m_editViewState.m_viewX1
              , m_editViewState.m_viewX2
              , getParentWidth ());

    const auto distanceTime = e.mods.isShiftDown ()
            ? m_editViewState.beatToTime(
                  distanceBeats  - m_editViewState.m_viewX1)
            : m_editViewState.getSnapedTime (
                  m_editViewState.beatToTime(
                      distanceBeats  - m_editViewState.m_viewX1), snapType);
    if (m_mouseDownX < 10 && m_clipWidthMouseDown > 30)
    {
        auto distTimeDelta = distanceTime - m_oldDistTime;

        auto resizeTime = juce::jmax(0.0, m_clip->getPosition().getStart()
                                     + distTimeDelta);
        //move left
        if (distTimeDelta > 0.0)
        {
            auto firstNoteTime = m_editViewState.beatToTime (
                        getMidiClip ()->getSequence ().getFirstBeatNumber ());
            if ((firstNoteTime - m_clip->getPosition ().getOffset ())
                    - distTimeDelta < 0.0)
            {
                m_clip->setStart(resizeTime, true, false);
            }
            else
            {
                m_clip->setStart(resizeTime, false, false);
                getMidiClip ()->getSequence ()
                        .moveAllBeatPositions (
                            m_editViewState.timeToBeat (-distTimeDelta)
                            , nullptr);
            }
        }//or Right
        else if (distTimeDelta < 0.0)
        {
            if (m_clip->getPosition ().getOffset () > 0.0)
            {
                m_clip->setOffset (m_clip->getPosition ().getOffset () + distTimeDelta);
                m_clip->setStart(resizeTime, false, false);
            }
            else
            {
                getMidiClip()->extendStart(
                            juce::jmax (
                                0.0, m_clip->getPosition().getStart() + distTimeDelta));
            }
            m_posAtMouseDown = m_clip->getPosition();
        }
        m_oldDistTime = distanceTime;
    }
    else if (m_mouseDownX > m_clipWidthMouseDown - 10
             && m_clipWidthMouseDown > 30)
    {
        auto snapType = m_editViewState.getBestSnapType (
                    m_editViewState.m_viewX1
                  , m_editViewState.m_viewX2
                  , getParentWidth ());
        auto snapedTime = m_editViewState.getSnapedTime (
                    m_posAtMouseDown.getEnd ()
                  , snapType);
        m_clip->setEnd(snapedTime + distanceTime, true);
    }
    else
    {
        ClipComponent::mouseDrag(e);
    }
}
