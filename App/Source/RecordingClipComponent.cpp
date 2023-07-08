
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "RecordingClipComponent.h"
#include "SongEditorView.h"
#include "Utilities.h"


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
    tracktion::TimeRange times;
    getBoundsAndTime (bounds, times);
    auto w = bounds.getWidth();

    if (w > 0 && w < 10000)
    {
        g.setColour (waveformColour);
        m_thumbnail->thumb->drawChannels (g, bounds,times.getStart().inSeconds(), times.getEnd().inSeconds(), 1.0f);

    }
}


bool RecordingClipComponent::getBoundsAndTime (juce::Rectangle<int>& bounds, tracktion::TimeRange& times) const
{
    auto editTimeToX = [this] (tracktion::TimePosition& t)
    {
        if (auto p = getParentComponent())
            return m_editViewState.timeToX (t.inSeconds(), p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2) - getX();
        return 0;
    };
    
    auto xToEditTime = [this] (int x)
    {
        if (auto p = getParentComponent())
            return tracktion::TimePosition::fromSeconds(m_editViewState.xToTime (x + getX(), p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2));
        return tracktion::TimePosition::fromSeconds(0.0);
    };
    
    bool hasLooped = false;
    auto& edit = m_track->edit;
    
    if (auto epc = edit.getTransport().getCurrentPlaybackContext())
    {
        auto localBounds = getLocalBounds();
        
        auto timeStarted = m_thumbnail->punchInTime;
        auto unloopedPos = timeStarted + tracktion::TimeDuration::fromSeconds(m_thumbnail->thumb->getTotalLength());
        
        auto t1 = timeStarted;
        auto t2 = unloopedPos;
        
        if (epc->isLooping() && t2 >= epc->getLoopTimes().getEnd())
        {
            hasLooped = true;
            
            t1 = juce::jmin (t1, epc->getLoopTimes().getStart());
            t2 = epc->getPosition();
            
            auto x1t = edit.tempoSequence.toTime(tracktion::BeatPosition::fromBeats(m_editViewState.m_viewX1.get()));
            auto x2t = edit.tempoSequence.toTime(tracktion::BeatPosition::fromBeats(m_editViewState.m_viewX2.get()));
            t1 = juce::jmax (x1t, t1);
            t2 = juce::jmin (x2t, t2);
        }
        else if (edit.recordingPunchInOut)
        {
            const auto in  = m_thumbnail->punchInTime;
            const auto out = edit.getTransport().getLoopRange().getEnd();
            
            t1 = juce::jlimit (in, out, t1);
            t2 = juce::jlimit (in, out, t2);
        }
        
        bounds = localBounds.withX (juce::jmax (localBounds.getX(), editTimeToX (t1)))
                 .withRight (juce::jmin (localBounds.getRight(), editTimeToX (t2)));
        
        auto loopRange = epc->getLoopTimes();
        const tracktion::TimeDuration recordedTime = unloopedPos - epc->getLoopTimes().getStart();
        const int numLoops = (int) (recordedTime / loopRange.getLength());
        
        const tracktion::TimeRange editTimes (xToEditTime (bounds.getX()),
                                       xToEditTime (bounds.getRight()));
        
        times = (editTimes + (loopRange.getLength()) * numLoops) - toDuration(timeStarted);
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
        auto t1 = m_punchInTime >= tracktion::TimePosition::fromSeconds(0) ? m_punchInTime : edit.getTransport().getTimeWhenStarted();
        auto t2 = juce::jmax (t1, epc->getUnloopedPosition());
        
        if (epc->isLooping())
        {
            auto loopTimes = epc->getLoopTimes();
            
            if (t2 >= loopTimes.getEnd())
            {
                t1 = juce::jmin (t1, loopTimes.getStart());
                t2 = loopTimes.getEnd();
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
        
        auto x1t = edit.tempoSequence.toTime(tracktion::BeatPosition::fromBeats(m_editViewState.m_viewX1.get()));
        auto x2t = edit.tempoSequence.toTime(tracktion::BeatPosition::fromBeats(m_editViewState.m_viewX2.get()));

        t1 = juce::jmax (t1, x1t);
        t2 = juce::jmin (t2, x2t);
    
        if (auto p = dynamic_cast<SongEditorView*>(getParentComponent()))
        {
            int x1 = m_editViewState.timeToX (t1.inSeconds(), p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
            int x2 = m_editViewState.timeToX (t2.inSeconds(), p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
            
            int y = p->getYForTrack(m_track);
            int h = p->getTrackHeight(m_track, m_editViewState, false);

            setBounds (x1, y, x2 - x1, h);
            return;
        }
    }
    
    setBounds ({});
}

