#include "ClipComponent.h"
 #include <utility> 

//==============================================================================
ClipComponent::ClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : m_editViewState (evs), m_clip (std::move(c))
{
}


void ClipComponent::paint (juce::Graphics& g)
{
    auto alpha = 1.0f;
    if (m_isDragging)
    {
        alpha = 0.2f;
    }

    auto area = getLocalBounds();
    g.setColour(m_clip->getTrack()->getColour());
    g.fillRect(area);
    area.reduce(1,1);
    g.setColour(getClip().getTrack()->getColour().darker());
    g.fillRect(area.removeFromTop(10));
    g.setColour (juce::Colours::black);
    if (m_isDragging)
    {
        g.setColour(juce::Colours::grey);
    }

    if (m_editViewState.m_selectionManager.isSelected (m_clip.get()))
    {
        g.setColour (juce::Colours::white);
    }

    g.drawRect (getLocalBounds());
}

void ClipComponent::mouseDown (const juce::MouseEvent&event)
{
    m_clickPosTime = m_editViewState.beatToTime(
                m_editViewState.xToBeats(event.x, getParentWidth()));
    if (event.mods.getCurrentModifiers().isCtrlDown())
    {
        m_isCopying = true;
    }
    if(!event.mouseWasDraggedSinceMouseDown())
        {
            if (event.mods.isRightButtonDown())
            {
                showContextMenu ();
            }
            else
            {
                m_clipPosAtMouseDown = m_clip->edit.tempoSequence.timeToBeats(
                            m_clip->getPosition().getStart());
                setMouseCursor (juce::MouseCursor::DraggingHandCursor);
            }
        }
    m_editViewState.m_selectionManager.selectOnly (getClip ());
    m_editViewState.m_selectionManager.addToSelection(
                getClip().getClipTrack());
    m_isDragging = true;
    tracktion_engine::Clipboard::getInstance()->clear();
    auto clipContent = std::make_unique<te::Clipboard::Clips>();
    clipContent->addClip(0, m_clip->state);
    te::Clipboard::getInstance()->setContent(std::move(clipContent));
}

void ClipComponent::mouseDrag(const juce::MouseEvent & event)
{
    //editViewState.edit.getTransport ().setUserDragging (true);
    juce::DragAndDropContainer* dragC =
            juce::DragAndDropContainer::findParentDragContainerFor(this);
    if (!dragC->isDragAndDropActive())
    {
        m_isShiftDown = false;
        if (event.mods.isShiftDown())
        {
            m_isShiftDown = true;
        }
        dragC->startDragging("Clip", this
                             , juce::Image(juce::Image::ARGB,1,1,true), false);
    }
}

void ClipComponent::mouseUp(const juce::MouseEvent &  /*event*/)
{
    m_editViewState.m_edit.getTransport ().setUserDragging (false);
    m_isDragging = false;
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

bool ClipComponent::isCopying() const
{
    return m_isCopying;
}

void ClipComponent::setIsCopying(bool isCopying)
{
    m_isCopying = isCopying;
}

double ClipComponent::getClickPosTime() const
{
    return m_clickPosTime;
}

void ClipComponent::setClickPosTime(double clickPosTime)
{
    m_clickPosTime = clickPosTime;
}

bool ClipComponent::isShiftDown() const
{
    return m_isShiftDown;
}

void ClipComponent::showContextMenu()
{
    juce::PopupMenu m;
    m.addItem(1, "Delete clip");
    m.addItem(2, "Copy clip");

    const int result = m.show();

    if (result == 0)
    {
        // user dismissed the menu without picking anything
    }
    else if (result == 1)
    {
        m_clip->removeFromParentTrack();
        return;
        // user picked item 1
    }
    else if (result == 2)
    {
        tracktion_engine::Clipboard::getInstance()->clear();
        auto clipContent = std::make_unique<te::Clipboard::Clips>();
        clipContent->addClip(0, m_clip->state);
        te::Clipboard::getInstance()->setContent(
                    std::move(clipContent));
    }
}

//==============================================================================

AudioClipComponent::AudioClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : ClipComponent (evs, c)
    , m_thumbnailComponent (evs)
{
    addAndMakeVisible (m_thumbnailComponent);
    m_thumbnailComponent.setFile (getWaveAudioClip ()->getOriginalFile ());
}

void AudioClipComponent::paint (juce::Graphics& g)
{
    ClipComponent::paint (g);

}

void AudioClipComponent::resized()
{
    auto leftOffset = 0;
    if(getBoundsInParent ().getX () < 0)
    {
        leftOffset = 0 - getBoundsInParent ().getX ();
    }

    auto rightOffset = 0;
    if (getBoundsInParent().getRight() > getParentWidth())
    {
        rightOffset = getBoundsInParent().getRight() - getParentWidth();
    }

    m_thumbnailComponent.setDrawOffset (m_clip->getPosition ().offset);
    auto drawOffsetTimeRight = getWaveAudioClip()->getSourceLength()
            - m_clip->getPosition().getOffset()
            - m_clip->getPosition().getLength();
    m_thumbnailComponent.setDrawOffsetRight(drawOffsetTimeRight);
    m_thumbnailComponent.setBounds (1 + leftOffset
                                    ,11
                                    , (getWidth() - 1 - leftOffset)
                                        - rightOffset
                                    , getHeight () - 11);
}

void AudioClipComponent::mouseMove(const juce::MouseEvent &e)
{
    if (e.getPosition().getX() < 10)
    {
        setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
    }
    else if (e.getPosition().getX() > getWidth() - 10)
    {
        setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void AudioClipComponent::mouseExit(const juce::MouseEvent &/*e*/)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void AudioClipComponent::mouseDown(const juce::MouseEvent &e)
{
    m_mouseDownX = e.getMouseDownX();
    m_posAtMouseDown =  m_clip->getPosition();
    m_clipWidthMouseDown = getWidth();
    ClipComponent::mouseDown(e);
    m_lastOffset = 0.0;
    m_oldDistTime = 0.0;
}

void AudioClipComponent::mouseDrag(const juce::MouseEvent &e)
{
    auto distanceBeats = m_editViewState.xToBeats(
                e.getDistanceFromDragStartX(),getParentWidth());
    const auto distanceTime = e.mods.isShiftDown ()
                            ? m_editViewState.beatToTime(
                                 distanceBeats  - m_editViewState.m_viewX1)
                            : m_editViewState.getSnapedTime (
                                  m_editViewState.beatToTime(
                                      distanceBeats  - m_editViewState.m_viewX1));
    auto distTimeDelta = distanceTime - m_oldDistTime;

    //shrink left
    if (m_mouseDownX < 10)
    {
        const auto newTime = m_clip->getPosition().getStart() + distTimeDelta;
        const auto newOffset = m_clip->getPosition().getOffset() + distTimeDelta;

        if ((distTimeDelta > 0
         || m_clip->getPosition().getOffset() > 0 )
         && !(newTime > m_clip->getPosition().getEnd()))
        {
            m_clip->setStart(juce::jmax(0.0, newTime), false, false);
            if (newOffset < 0)
            {
                m_clip->setStart(juce::jmax(0.0
                                 , m_clip->getPosition().getStart() - newOffset)
                                 , false, false);
                m_lastOffset = newOffset;
            }
            m_clip->setOffset(newOffset);
        }
        else
        {
            m_posAtMouseDown = m_clip->getPosition();
            m_lastOffset = 0.0;
        }
        m_oldDistTime = distanceTime;
    }
    //shrink right
    else if (m_mouseDownX > m_clipWidthMouseDown - 10)
    {
        m_clip->setEnd(m_editViewState.getSnapedTime (m_posAtMouseDown.getEnd ())
                       + distanceTime, true);
    }
    else
    {
        ClipComponent::mouseDrag(e);
    }
}

tracktion_engine::WaveAudioClip *AudioClipComponent::getWaveAudioClip()
{
    return dynamic_cast<te::WaveAudioClip*> (m_clip.get());
}

//==============================================================================

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
    ClipComponent::paint (g);
    auto clipHeader = 10;
    if (auto mc = getMidiClip())
    {
        auto& seq = mc->getSequence();
        for (auto n : seq.getNotes())
        {
            double sBeat = n->getStartBeat() - mc->getOffsetInBeats();
            double eBeat = n->getEndBeat() - mc->getOffsetInBeats();
            if (auto p = getParentComponent())
            {
                double y = ((1.0 - double (n->getNoteNumber()) / 127.0)
                            * (getHeight() - clipHeader) + clipHeader);

                auto x1 =  m_editViewState.beatsToX (
                            sBeat + m_editViewState.m_viewX1, p->getWidth ());
                auto x2 =  m_editViewState.beatsToX (
                            eBeat + m_editViewState.m_viewX1, p->getWidth ());

                g.setColour (juce::Colours::white);
                g.drawLine (float (x1), float (y), float (x2), float (y));
            }
        }
    }
}

void MidiClipComponent::mouseMove(const juce::MouseEvent &e)
{
    if (e.getPosition().getX() < 10)
    {
        setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
    }
    else if (e.getPosition().getX() > getWidth() - 10)
    {
        setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void MidiClipComponent::mouseExit(const juce::MouseEvent &/*e*/)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void MidiClipComponent::mouseDown(const juce::MouseEvent &e)
{
    m_mouseDownX = e.getMouseDownX();
    m_posAtMouseDown =  m_clip->getPosition();
    m_clipWidthMouseDown = getWidth();
    m_lastOffset = 0.0;
    m_oldDistTime = 0.0;
    ClipComponent::mouseDown(e);
    if (e.getNumberOfClicks () > 1
     || m_editViewState.m_isPianoRollVisible)
    {
        sendChangeMessage ();
    }
}

void MidiClipComponent::mouseDrag(const juce::MouseEvent &e)
{
    const auto distanceBeats = m_editViewState.xToBeats(
                e.getDistanceFromDragStartX(),getParentWidth());
    const auto distanceTime = e.mods.isShiftDown ()
                            ? m_editViewState.beatToTime(
                                 distanceBeats  - m_editViewState.m_viewX1)
                            : m_editViewState.getSnapedTime (
                                  m_editViewState.beatToTime(
                                      distanceBeats  - m_editViewState.m_viewX1));
    if (m_mouseDownX < 10)
    {
        auto distTimeDelta = distanceTime - m_oldDistTime;
        if (distTimeDelta > 0
         || m_clip->getPosition().getOffset() > 0 )
        {
            m_clip->setStart(
                        juce::jmax(0.0, m_clip->getPosition().getStart()
                                   + distTimeDelta)
                             , false, false);
            if (m_clip->getPosition().getOffset() + distTimeDelta < 0)
            {
                m_lastOffset = m_clip->getPosition().getOffset() + distTimeDelta;
                getMidiClip()->getSequence().moveAllBeatPositions(
                            m_editViewState.m_edit.tempoSequence.timeToBeats(-m_lastOffset)
                            , nullptr);
            }
            m_clip->setOffset(m_clip->getPosition().getOffset()
                              + distTimeDelta);
        }
        else
        {
            getMidiClip()->extendStart(
                        juce::jmax (
                            0.0, m_clip->getPosition().getStart() + distTimeDelta));
            m_posAtMouseDown = m_clip->getPosition();
            m_lastOffset = 0.0;
        }
        m_oldDistTime = distanceTime;
    }
    else if (m_mouseDownX > m_clipWidthMouseDown - 10)
    {
        m_clip->setEnd(m_editViewState.getSnapedTime (m_posAtMouseDown.getEnd ())
                       + distanceTime, true);
    }
    else
    {
        ClipComponent::mouseDrag(e);
    }
}

//==============================================================================
RecordingClipComponent::RecordingClipComponent (te::Track::Ptr t, EditViewState& evs)
    : m_track (t), m_editViewState (evs)
{
    startTimerHz (10);
    initialiseThumbnailAndPunchTime();

}

void RecordingClipComponent::initialiseThumbnailAndPunchTime()
{
    if (auto at = dynamic_cast<te::AudioTrack*> (m_track.get()))
    {
        for (auto* idi : at->edit.getEditInputDevices().getDevicesForTargetTrack (*at))
        {
            m_punchInTime = idi->getPunchInTime();

            if (idi->getRecordingFile().exists())
            {
                m_thumbnail = at->edit.engine.getRecordingThumbnailManager()
                        .getThumbnailFor (idi->getRecordingFile());
            }
        }
    }
}

void RecordingClipComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::red);
    g.setColour (juce::Colours::black);
    g.drawRect (getLocalBounds());

    auto area = getLocalBounds();
    area.reduce(1,1);
    g.setColour(juce::Colours::red.darker());
    g.fillRect(area.removeFromTop(m_clipHeaderHight));


    if (m_editViewState.m_drawWaveforms)
        drawThumbnail (g, juce::Colours::black.withAlpha (0.5f));
}

void RecordingClipComponent::drawThumbnail (juce::Graphics& g
                                            , juce::Colour waveformColour) const
{
    if (m_thumbnail == nullptr)
        return;

    juce::Rectangle<int> bounds;
    juce::Range<double> times;
    getBoundsAndTime (bounds, times);
    auto w = bounds.getWidth();

    if (w > 0 && w < 10000)
    {
        g.setColour (waveformColour);
        m_thumbnail->thumb.drawChannels (g, bounds, w, times, 1.0f);
    }

}


bool RecordingClipComponent::getBoundsAndTime (juce::Rectangle<int>& bounds
                                               , juce::Range<double>& times) const
{
    auto editTimeToX = [this] (double t)
    {
        if (auto p = getParentComponent())
        {
            auto beats = m_editViewState.timeToBeat (t);
            return static_cast<double>( m_editViewState.beatsToX (
                                            beats, p->getWidth ()) - getX ());
        }
        return 0.0;
    };

    auto xToEditTime = [this] (int x)
    {
        if (auto p = getParentComponent())
        {
            auto beats = m_editViewState.xToBeats (x + getX(), p->getWidth ());
            return m_editViewState.beatToTime (beats);
        }
        return 0.0;
    };

    bool hasLooped = false;
    auto& edit = m_track->edit;

    if (auto* playhead = edit.getTransport().getCurrentPlayhead())
    {
        auto localBounds = getLocalBounds();

        auto timeStarted = m_thumbnail->punchInTime;
        auto unloopedPos = timeStarted + m_thumbnail->thumb.getTotalLength();

        auto t1 = timeStarted;
        auto t2 = unloopedPos;

        if (playhead->isLooping() && t2 >= playhead->getLoopTimes().end)
        {
            hasLooped = true;

            t1 = juce::jmin (t1, playhead->getLoopTimes().start);
            t2 = playhead->getPosition();

            t1 = juce::jmax (m_editViewState.beatToTime (
                                 m_editViewState.m_viewX1.get()), t1);
            t2 = juce::jmin (m_editViewState.beatToTime (
                                 m_editViewState.m_viewX2.get()), t2);
        }
        else if (edit.recordingPunchInOut)
        {
            const double in  = m_thumbnail->punchInTime;
            const double out = edit.getTransport().getLoopRange().getEnd();

            t1 = juce::jlimit (in, out, t1);
            t2 = juce::jlimit (in, out, t2);
        }

        bounds = localBounds.withX (juce::jmax (
                                        localBounds.getX()
                                        , static_cast<int>(editTimeToX (t1))))
                 .withRight (juce::jmin (
                                 localBounds.getRight()
                                 , static_cast<int>(editTimeToX (t2))));

        bounds.removeFromTop (m_clipHeaderHight);

        auto loopRange = playhead->getLoopTimes();
        const double recordedTime = unloopedPos - playhead->getLoopTimes().start;
        const int numLoops = (int) (recordedTime / loopRange.getLength());

        const juce::Range<double> editTimes (xToEditTime (bounds.getX()),
                                       xToEditTime (bounds.getRight()));

        times = (editTimes + (numLoops * loopRange.getLength())) - timeStarted;
    }

    return hasLooped;
}

void RecordingClipComponent::timerCallback()
{
    updatePosition();
}

void RecordingClipComponent::updatePosition()
{
    auto& edit = m_track->edit;

    if (auto playhead = edit.getTransport().getCurrentPlayhead())
    {
        double t1 = m_punchInTime >= 0
                                  ? m_punchInTime
                                  : edit.getTransport().getTimeWhenStarted();

        double t2 = juce::jmax (t1, playhead->getUnloopedPosition());

        if (playhead->isLooping())
        {
            auto loopTimes = playhead->getLoopTimes();

            if (t2 >= loopTimes.end)
            {
                t1 = juce::jmin (t1, loopTimes.start);
                t2 = loopTimes.end;
            }
        }
        else if (edit.recordingPunchInOut)
        {
            auto mr = edit.getTransport().getLoopRange();
            auto in  = mr.getStart();
            auto out = mr.getEnd();

            t1 = juce::jlimit (in, out, t1);
            t2 = juce::jlimit (in, out, t2);
        }

        t1 = juce::jmax (t1
                         ,m_editViewState.beatToTime (
                             m_editViewState.m_viewX1.get()));
        t2 = juce::jmin (t2
                         ,m_editViewState.beatToTime (
                             m_editViewState.m_viewX2.get()));

        if (auto p = getParentComponent())
        {
            int x1 = m_editViewState.beatsToX (
                        m_editViewState.timeToBeat (t1), p->getWidth());
            int x2 = m_editViewState.beatsToX (
                        m_editViewState.timeToBeat (t2), p->getWidth());

            setBounds (x1, 0, x2 - x1, p->getHeight());
            return;
        }
    }
    setBounds ({});
}
