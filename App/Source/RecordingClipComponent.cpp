#include "RecordingClipComponent.h"


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

