#include "RecordingClipComponent.h"

#include <utility>


RecordingClipComponent::RecordingClipComponent (te::Track::Ptr t, EditViewState& evs)
    : m_track (std::move(t)), m_editViewState (evs)
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
    g.fillRect(area.removeFromTop(m_ClipHeaderHeight));

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


bool RecordingClipComponent::getBoundsAndTime (juce::Rectangle<int>& bounds, juce::Range<double>& times) const
{
    auto editTimeToX = [this] (double t)
    {
        if (auto p = getParentComponent())
            return m_editViewState.timeToX (t, p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2) - getX();
        return 0;
    };
    
    auto xToEditTime = [this] (int x)
    {
        if (auto p = getParentComponent())
            return m_editViewState.xToTime (x + getX(), p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
        return 0.0;
    };
    
    bool hasLooped = false;
    auto& edit = m_track->edit;
    
    if (auto epc = edit.getTransport().getCurrentPlaybackContext())
    {
        auto localBounds = getLocalBounds();
        
        auto timeStarted = m_thumbnail->punchInTime;
        auto unloopedPos = timeStarted + m_thumbnail->thumb.getTotalLength();
        
        auto t1 = timeStarted;
        auto t2 = unloopedPos;
        
        if (epc->isLooping() && t2 >= epc->getLoopTimes().end)
        {
            hasLooped = true;
            
            t1 = juce::jmin (t1, epc->getLoopTimes().start);
            t2 = epc->getPosition();
            
            t1 = juce::jmax (m_editViewState.m_viewX1.get(), t1);
            t2 = juce::jmin (m_editViewState.m_viewX2.get(), t2);
        }
        else if (edit.recordingPunchInOut)
        {
            const double in  = m_thumbnail->punchInTime;
            const double out = edit.getTransport().getLoopRange().getEnd();
            
            t1 = juce::jlimit (in, out, t1);
            t2 = juce::jlimit (in, out, t2);
        }
        
        bounds = localBounds.withX (juce::jmax (localBounds.getX(), editTimeToX (t1)))
                 .withRight (juce::jmin (localBounds.getRight(), editTimeToX (t2)));
        
        auto loopRange = epc->getLoopTimes();
        const double recordedTime = unloopedPos - epc->getLoopTimes().start;
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
    
    if (auto epc = edit.getTransport().getCurrentPlaybackContext())
    {
        double t1 = m_punchInTime >= 0 ? m_punchInTime : edit.getTransport().getTimeWhenStarted();
        double t2 = juce::jmax (t1, epc->getUnloopedPosition());
        
        if (epc->isLooping())
        {
            auto loopTimes = epc->getLoopTimes();
            
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
        
        t1 = juce::jmax (t1, m_editViewState.m_viewX1.get());
        t2 = juce::jmin (t2, m_editViewState.m_viewX2.get());
    
        if (auto p = getParentComponent())
        {
            int x1 = m_editViewState.timeToX (t1, p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
            int x2 = m_editViewState.timeToX (t2, p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
            
            setBounds (x1, 0, x2 - x1, p->getHeight());
            return;
        }
    }
    
    setBounds ({});
}

