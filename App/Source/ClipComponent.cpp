#include "ClipComponent.h"
 #include <utility> 

//==============================================================================
ClipComponent::ClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : editViewState (evs), clip (std::move(c))
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
    g.setColour(clip->getTrack()->getColour());
    g.fillRect(area);
    area.reduce(1,1);
    g.setColour(getClip().getTrack()->getColour().darker());
    g.fillRect(area.removeFromTop(10));
    g.setColour (juce::Colours::black);
    if (m_isDragging)
    {
        g.setColour(juce::Colours::grey);
    }

    if (editViewState.selectionManager.isSelected (clip.get()))
    {
        g.setColour (juce::Colours::white);
    }

    g.drawRect (getLocalBounds());
}

void ClipComponent::mouseDown (const juce::MouseEvent&event)
{
    m_clickPosTime = editViewState.beatToTime(editViewState.xToBeats(event.x, getParentWidth()));
    if (event.mods.getCurrentModifiers().isCtrlDown())
    {
        m_isCopying = true;
    }
    if(!event.mouseWasDraggedSinceMouseDown())
        {
            if (event.mods.isRightButtonDown())
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
                    clip->removeFromParentTrack();
                    return;
                    // user picked item 1
                }
                else if (result == 2)
                {
                    tracktion_engine::Clipboard::getInstance()->clear();
                    auto clipContent = std::make_unique<te::Clipboard::Clips>();
                    clipContent->addClip(0, clip->state);
                    te::Clipboard::getInstance()->setContent(std::move(clipContent));
                }
            }
            else
            {
                editViewState.selectionManager.selectOnly (getClip ());
                editViewState.selectionManager.addToSelection(getClip().getClipTrack());
                m_clipPosAtMouseDown = clip->edit.tempoSequence.timeToBeats(clip->getPosition().getStart());
                setMouseCursor (juce::MouseCursor::DraggingHandCursor);
            }
        }
    m_isDragging = true;
    tracktion_engine::Clipboard::getInstance()->clear();
    auto clipContent = std::make_unique<te::Clipboard::Clips>();
    clipContent->addClip(0, clip->state);
    te::Clipboard::getInstance()->setContent(std::move(clipContent));
}

void ClipComponent::mouseDrag(const juce::MouseEvent & event)
{
    //editViewState.edit.getTransport ().setUserDragging (true);
    juce::DragAndDropContainer* dragC = juce::DragAndDropContainer::findParentDragContainerFor(this);
    if (!dragC->isDragAndDropActive())
    {
        m_isShiftDown = false;
        if (event.mods.isShiftDown())
        {
            m_isShiftDown = true;
        }
        dragC->startDragging("Clip", this,juce::Image(juce::Image::ARGB,1,1,true),
                             false);

    }
}

void ClipComponent::mouseUp(const juce::MouseEvent &  /*event*/)
{
    editViewState.edit.getTransport ().setUserDragging (false);
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

//==============================================================================

AudioClipComponent::AudioClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : ClipComponent (evs, c)
    , thumbnailComponent (evs)
{
    addAndMakeVisible (thumbnailComponent);
    thumbnailComponent.setFile (getWaveAudioClip ()->getOriginalFile ());
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

    thumbnailComponent.setDrawOffset (clip->getPosition ().offset);
    auto drawOffsetTimeRight = getWaveAudioClip()->getSourceLength()
            - clip->getPosition().getOffset()
            - clip->getPosition().getLength();
    thumbnailComponent.setDrawOffsetRight(drawOffsetTimeRight);
    thumbnailComponent.setBounds (1 + leftOffset,11,(getWidth() - 1 - leftOffset) - rightOffset, getHeight () - 11);
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

void AudioClipComponent::mouseExit(const juce::MouseEvent &e)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void AudioClipComponent::mouseDown(const juce::MouseEvent &e)
{
    m_mouseDownX = e.getMouseDownX();
    m_posAtMouseDown =  clip->getPosition();
    m_clipWidthMouseDown = getWidth();
    ClipComponent::mouseDown(e);
    m_lastOffset = 0.0;
    m_oldDistTime = 0.0;
}

void AudioClipComponent::mouseDrag(const juce::MouseEvent &e)
{
    const auto distanceBeats = editViewState.xToBeats(e.getDistanceFromDragStartX(),getParentWidth());
    const auto distanceTime = editViewState.beatToTime(distanceBeats  - editViewState.viewX1);
    auto distTimeDelta = distanceTime - m_oldDistTime;

    if (m_mouseDownX < 10)
    {
        auto newTime = clip->getPosition().getStart() + distTimeDelta;
        auto newOffset = clip->getPosition().getOffset() + distTimeDelta;

        if ((distTimeDelta > 0
         || clip->getPosition().getOffset() > 0 )
         && !(newTime > clip->getPosition().getEnd()))
        {
            clip->setStart(juce::jmax(0.0, newTime), false, false);
            if (newOffset < 0)
            {
                clip->setStart(juce::jmax(0.0, clip->getPosition().getStart() - newOffset), false, false);
                m_lastOffset = newOffset;
            }
            clip->setOffset(newOffset);
            if (!e.mods.isShiftDown())
            {

//                clip->setStart(clip->edit.getTimecodeFormat().getSnapType(editViewState.snapType)
//                               .roundTimeNearest( clip->getPosition().getStart(), clip->edit.tempoSequence), false, false);
                //clip->setOffset(clip->getPosition().getOffset() + snapTimeOffset);
            }
        }
        else
        {

            m_posAtMouseDown = clip->getPosition();
            m_lastOffset = 0.0;
        }
        m_oldDistTime = distanceTime;
    }
    else if (m_mouseDownX > m_clipWidthMouseDown - 10)
    {
        auto newEndTime = clip->getPosition().getEnd() + distTimeDelta;
        auto clipPos = clip->getPosition();

        if (/*newEndTime < clipPos.getStart()
                            + getWaveAudioClip()->getSourceLength()
                            - clipPos.getOffset()
         &&*/ newEndTime > clipPos.getStart())
        {
            clip->setEnd(newEndTime, true);
        }
        m_oldDistTime = distanceTime;
    }
    else
    {
        ClipComponent::mouseDrag(e);
    }
}

tracktion_engine::WaveAudioClip *AudioClipComponent::getWaveAudioClip()
{
    return dynamic_cast<te::WaveAudioClip*> (clip.get());
}

//==============================================================================

MidiClipComponent::MidiClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : ClipComponent (evs, c)
{
    setBufferedToImage(true);
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
            double sBeat = /*mc->getStartBeat() +*/ n->getStartBeat() - mc->getOffsetInBeats();
            double eBeat = /*mc->getStartBeat() + */n->getEndBeat() - mc->getOffsetInBeats();
            if (auto p = getParentComponent())
            {
                double y = ((1.0 - double (n->getNoteNumber()) / 127.0) * (getHeight() - clipHeader) + clipHeader);

                auto x1 =  editViewState.beatsToX (sBeat + editViewState.viewX1, p->getWidth ());
                auto x2 =  editViewState.beatsToX (eBeat + editViewState.viewX1, p->getWidth ());

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

void MidiClipComponent::mouseExit(const juce::MouseEvent &e)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void MidiClipComponent::mouseDown(const juce::MouseEvent &e)
{
    m_mouseDownX = e.getMouseDownX();
    m_posAtMouseDown =  clip->getPosition();
    m_clipWidthMouseDown = getWidth();
    ClipComponent::mouseDown(e);
    m_lastOffset = 0.0;
    m_oldDistTime = 0.0;
}

void MidiClipComponent::mouseDrag(const juce::MouseEvent &e)
{
    const auto distanceBeats = editViewState.xToBeats(e.getDistanceFromDragStartX(),getParentWidth());
    const auto distanceTime = editViewState.beatToTime(distanceBeats  - editViewState.viewX1);

    if (m_mouseDownX < 10)
    {
        auto distTimeDelta = distanceTime - m_oldDistTime;
        if (distTimeDelta > 0
         || clip->getPosition().getOffset() > 0 )
        {
            clip->setStart(juce::jmax(0.0, clip->getPosition().getStart() + distTimeDelta), false, false);
            if (clip->getPosition().getOffset() + distTimeDelta < 0)
            {
                m_lastOffset = clip->getPosition().getOffset() + distTimeDelta;
                getMidiClip()->getSequence().moveAllBeatPositions(editViewState.edit.tempoSequence.timeToBeats(-m_lastOffset), nullptr);
            }
            clip->setOffset(clip->getPosition().getOffset() + distTimeDelta);
        }
        else
        {
            getMidiClip()->extendStart(juce::jmax (0.0, clip->getPosition().getStart() + distTimeDelta));
            m_posAtMouseDown = clip->getPosition();
            m_lastOffset = 0.0;
        }
        m_oldDistTime = distanceTime;
    }
    else if (m_mouseDownX > m_clipWidthMouseDown - 10)
    {
        auto timeDist = editViewState.beatToTime((editViewState.xToBeats(e.getDistanceFromDragStartX(), getParentWidth())));
        auto timeX = editViewState.beatToTime(editViewState.viewX1);
        clip->setEnd(m_posAtMouseDown.getEnd() - timeX + timeDist, true);
    }
    else
    {
        ClipComponent::mouseDrag(e);
    }
}

//==============================================================================
RecordingClipComponent::RecordingClipComponent (te::Track::Ptr t, EditViewState& evs)
    : track (t), editViewState (evs)
{
    startTimerHz (10);
    initialiseThumbnailAndPunchTime();

}

void RecordingClipComponent::initialiseThumbnailAndPunchTime()
{
    if (auto at = dynamic_cast<te::AudioTrack*> (track.get()))
    {
        for (auto* idi : at->edit.getEditInputDevices().getDevicesForTargetTrack (*at))
        {
            punchInTime = idi->getPunchInTime();

            if (idi->getRecordingFile().exists())
            {
                thumbnail = at->edit.engine.getRecordingThumbnailManager().getThumbnailFor (idi->getRecordingFile());
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
    g.fillRect(area.removeFromTop(clipHeaderHight));


    if (editViewState.drawWaveforms)
        drawThumbnail (g, juce::Colours::black.withAlpha (0.5f));
}

void RecordingClipComponent::drawThumbnail (juce::Graphics& g, juce::Colour waveformColour) const
{
    if (thumbnail == nullptr)
        return;

    juce::Rectangle<int> bounds;
    juce::Range<double> times;
    getBoundsAndTime (bounds, times);
    auto w = bounds.getWidth();

    if (w > 0 && w < 10000)
    {
        g.setColour (waveformColour);
        thumbnail->thumb.drawChannels (g, bounds, w, times, 1.0f);
    }

}


bool RecordingClipComponent::getBoundsAndTime (juce::Rectangle<int>& bounds, juce::Range<double>& times) const
{
    auto editTimeToX = [this] (double t)
    {
        if (auto p = getParentComponent())
        {
            auto beats = editViewState.timeToBeat (t);
            return static_cast<double>( editViewState.beatsToX (beats, p->getWidth ()) - getX ());
        }
        return 0.0;
    };

    auto xToEditTime = [this] (int x)
    {
        if (auto p = getParentComponent())
        {
            auto beats = editViewState.xToBeats (x + getX(), p->getWidth ());
            return editViewState.beatToTime (beats);
            //return editViewState.edit.tempoSequence.beatsToTime(editViewState.xToBeats (x + getX(), p->getWidth()));
        }
        return 0.0;
    };

    bool hasLooped = false;
    auto& edit = track->edit;

    if (auto* playhead = edit.getTransport().getCurrentPlayhead())
    {
        auto localBounds = getLocalBounds();

        auto timeStarted = thumbnail->punchInTime;
        auto unloopedPos = timeStarted + thumbnail->thumb.getTotalLength();

        auto t1 = timeStarted;
        auto t2 = unloopedPos;

        if (playhead->isLooping() && t2 >= playhead->getLoopTimes().end)
        {
            hasLooped = true;

            t1 = juce::jmin (t1, playhead->getLoopTimes().start);
            t2 = playhead->getPosition();

            t1 = juce::jmax (editViewState.beatToTime (editViewState.viewX1.get()), t1);
            t2 = juce::jmin (editViewState.beatToTime (editViewState.viewX2.get()), t2);
        }
        else if (edit.recordingPunchInOut)
        {
            const double in  = thumbnail->punchInTime;
            const double out = edit.getTransport().getLoopRange().getEnd();

            t1 = juce::jlimit (in, out, t1);
            t2 = juce::jlimit (in, out, t2);
        }

        bounds = localBounds.withX (juce::jmax (localBounds.getX(), static_cast<int>(editTimeToX (t1))))
                 .withRight (juce::jmin (localBounds.getRight(), static_cast<int>(editTimeToX (t2))));
        bounds.removeFromTop (clipHeaderHight);

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
    auto& edit = track->edit;

    if (auto playhead = edit.getTransport().getCurrentPlayhead())
    {
        double t1 = punchInTime >= 0 ? punchInTime : edit.getTransport().getTimeWhenStarted();
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

        t1 = juce::jmax (t1,editViewState.beatToTime (editViewState.viewX1.get()));
        t2 = juce::jmin (t2,editViewState.beatToTime (editViewState.viewX2.get()));

        if (auto p = getParentComponent())
        {
            int x1 = editViewState.beatsToX (editViewState.timeToBeat (t1), p->getWidth());
            int x2 = editViewState.beatsToX (editViewState.timeToBeat (t2), p->getWidth());

            setBounds (x1, 0, x2 - x1, p->getHeight());
            return;
        }
    }

    setBounds ({});
}

