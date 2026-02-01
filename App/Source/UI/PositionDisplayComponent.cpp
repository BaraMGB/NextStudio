
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#include "UI/PositionDisplayComponent.h"
PositionDisplayComponent::PositionDisplayComponent(te::Edit &edit)
    : m_edit(edit)
{
    Helpers::addAndMakeVisible(*this, {&m_bpmLabel, &m_sigLabel, &m_barBeatTickLabel, &m_timeLabel, &m_loopInLabel, &m_loopOutLabel});
    m_bpmLabel.setJustificationType(juce::Justification::centred);
    m_sigLabel.setJustificationType(juce::Justification::centred);
    m_barBeatTickLabel.setJustificationType(juce::Justification::centred);
    m_barBeatTickLabel.setFont(28);
    m_timeLabel.setJustificationType(juce::Justification::centred);
    m_loopInLabel.setJustificationType(juce::Justification::centred);
    m_loopOutLabel.setJustificationType(juce::Justification::centred);

    m_bpmLabel.setInterceptsMouseClicks(false, false);
    m_sigLabel.setInterceptsMouseClicks(false, false);
    m_barBeatTickLabel.setInterceptsMouseClicks(false, false);
    m_timeLabel.setInterceptsMouseClicks(false, false);
    m_loopInLabel.setInterceptsMouseClicks(false, false);
    m_loopOutLabel.setInterceptsMouseClicks(false, false);

    update();
}

void PositionDisplayComponent::paint(juce::Graphics &g)
{
    auto area = getLocalBounds();

    g.setColour(juce::Colour(0xff1c1c1c));
    g.fillRoundedRectangle(area.toFloat(), 5.0f);
    g.setColour(juce::Colour(0xff999999));
    g.drawRoundedRectangle(area.reduced(1).toFloat(), 5.0f, 0.5f);
}

void PositionDisplayComponent::mouseDown(const juce::MouseEvent &event)
{
    m_mousedownPosition = event.getMouseDownPosition();
    m_mousedownBPM = m_edit.tempoSequence.getTempos()[0]->getBpm();
    m_mousedownBeatPosition = m_edit.tempoSequence.toBeats(m_edit.getTransport().getPosition());
    m_mousedownTime = m_edit.getTransport().getPosition();
    m_mousedownNumerator = m_edit.tempoSequence.getTimeSigAt(m_mousedownTime).numerator;
    m_mousedownDenominator = m_edit.tempoSequence.getTimeSigAt(m_mousedownTime).denominator;
    m_mousedownLoopIn = m_edit.tempoSequence.toBeats(m_edit.getTransport().getLoopRange().getStart());
    m_mousedownLoopOut = m_edit.tempoSequence.toBeats(m_edit.getTransport().getLoopRange().getEnd());
}

void PositionDisplayComponent::mouseDrag(const juce::MouseEvent &event)
{
    event.source.enableUnboundedMouseMovement(true);

    auto draggedDist = event.getDistanceFromDragStartY();
    if (m_bmpRect.contains(m_mousedownPosition))
    {
        auto r = m_bmpRect;

        auto &tempo = m_edit.tempoSequence.getTempoAt(m_mousedownTime);
        tempo.setBpm(r.removeFromLeft(r.getWidth() / 2).contains(m_mousedownPosition) ? (int)(m_mousedownBPM - (draggedDist / 10.0)) : m_mousedownBPM - (draggedDist / 1000.0));
        // set the Position back to the Beat Position at Mouse down
        m_edit.getTransport().setPosition(m_edit.tempoSequence.toTime(m_mousedownBeatPosition));
    }
    else if (m_sigRect.contains(m_mousedownPosition))
    {
        auto r = m_sigRect;
        if (r.removeFromLeft(r.getWidth() / 2).contains(m_mousedownPosition))
            m_edit.tempoSequence.getTimeSigAt(m_mousedownTime).numerator = juce::jlimit(1, 16, m_mousedownNumerator - draggedDist);
        else
            m_edit.tempoSequence.getTimeSigAt(m_mousedownTime).denominator = juce::jlimit(1, 16, m_mousedownDenominator - draggedDist);
    }
    else if (m_barBeatTickRect.contains(m_mousedownPosition))
    {

        auto timeAtMd = m_edit.tempoSequence.toTime(m_mousedownBeatPosition);
        te::TimecodeSnapType snapType;
        snapType.type = te::TimecodeType::barsBeats;
        snapType.level = 0;

        auto snapedTime = snapType.roundTimeNearest(timeAtMd, m_edit.tempoSequence);
        auto snapedBeat = m_edit.tempoSequence.toBeats(snapedTime);
        auto r = m_barBeatTickRect;
        auto leftRect = r.removeFromLeft(m_barBeatTickRect.getWidth() / 3);
        auto centerRect = r.removeFromLeft(m_barBeatTickRect.getWidth() / 3);

        if (leftRect.contains(m_mousedownPosition))
            m_edit.getTransport().setPosition(m_edit.tempoSequence.toTime(snapedBeat - (tracktion::BeatDuration::fromBeats(4.0 * draggedDist))));
        else if (centerRect.contains(m_mousedownPosition))
            m_edit.getTransport().setPosition(m_edit.tempoSequence.toTime(snapedBeat - (tracktion::BeatDuration::fromBeats(draggedDist))));
        else
            m_edit.getTransport().setPosition(m_edit.tempoSequence.toTime(snapedBeat - (tracktion::BeatDuration::fromBeats((double)draggedDist / 960.0))));
    }
    else if (m_timeRect.contains(m_mousedownPosition))
    {
        auto r = m_timeRect;
        auto leftRect = r.removeFromLeft(m_timeRect.getWidth() / 3);
        auto centerRect = r.removeFromLeft(m_timeRect.getWidth() / 3);

        if (leftRect.contains(m_mousedownPosition))
            m_edit.getTransport().setPosition(m_mousedownTime - tracktion::TimeDuration::fromSeconds(draggedDist * 60));
        if (centerRect.contains(m_mousedownPosition))
            m_edit.getTransport().setPosition(m_mousedownTime - tracktion::TimeDuration::fromSeconds(draggedDist));
        else
            m_edit.getTransport().setPosition(m_mousedownTime - tracktion::TimeDuration::fromSeconds((double)draggedDist / 1000));
    }
    else if (m_loopInrect.contains(m_mousedownPosition))
    {
        auto r = m_loopInrect;
        auto leftRect = r.removeFromLeft(m_loopInrect.getWidth() / 3);
        auto centerRect = r.removeFromLeft(m_loopInrect.getWidth() / 3);
        if (leftRect.contains(m_mousedownPosition))
            m_edit.getTransport().setLoopIn(m_edit.tempoSequence.toTime(m_mousedownLoopIn - (tracktion::BeatDuration::fromBeats(4.0 * draggedDist))));
        else if (centerRect.contains(m_mousedownPosition))
            m_edit.getTransport().setLoopIn(m_edit.tempoSequence.toTime(m_mousedownLoopIn - (tracktion::BeatDuration::fromBeats(draggedDist))));
        else
            m_edit.getTransport().setLoopIn(m_edit.tempoSequence.toTime(m_mousedownLoopIn - (tracktion::BeatDuration::fromBeats((double)draggedDist / 960.0))));
    }
    else if (m_loopOutRect.contains(m_mousedownPosition))
    {
        auto r = m_loopOutRect;
        auto leftRect = r.removeFromLeft(m_loopOutRect.getWidth() / 3);
        auto centerRect = r.removeFromLeft(m_loopOutRect.getWidth() / 3);

        if (leftRect.contains(m_mousedownPosition))
            m_edit.getTransport().setLoopOut(m_edit.tempoSequence.toTime(m_mousedownLoopOut - (tracktion::BeatDuration::fromBeats(4.0 * draggedDist))));
        else if (centerRect.contains(m_mousedownPosition))
            m_edit.getTransport().setLoopOut(m_edit.tempoSequence.toTime(m_mousedownLoopOut - (tracktion::BeatDuration::fromBeats(draggedDist))));
        else
            m_edit.getTransport().setLoopOut(m_edit.tempoSequence.toTime(m_mousedownLoopOut - (tracktion::BeatDuration::fromBeats((double)draggedDist / 960.0))));
    }
}

void PositionDisplayComponent::mouseUp(const juce::MouseEvent &) { m_edit.getTransport().setUserDragging(false); }

void PositionDisplayComponent::resized()
{
    auto area = getLocalBounds();
    auto leftColumb = area.removeFromLeft(getWidth() / 4);

    m_bmpRect = leftColumb.removeFromTop(leftColumb.getHeight() / 2);
    m_sigRect = leftColumb;

    auto rightColumb = area.removeFromRight(getWidth() / 4);

    m_loopInrect = rightColumb.removeFromTop(rightColumb.getHeight() / 2);
    m_loopOutRect = rightColumb;
    m_barBeatTickRect = area.removeFromTop((getHeight() / 3) * 2);
    m_timeRect = area;

    m_bpmLabel.setBounds(m_bmpRect);
    m_sigLabel.setBounds(m_sigRect);
    m_barBeatTickLabel.setBounds(m_barBeatTickRect);
    m_timeLabel.setBounds(m_timeRect);
    m_loopInLabel.setBounds(m_loopInrect);
    m_loopOutLabel.setBounds(m_loopOutRect);
}

void PositionDisplayComponent::update()
{
    const auto nt = juce::NotificationType::dontSendNotification;
    PlayHeadHelpers::TimeCodeStrings positionStr(m_edit);

    m_bpmLabel.setText(positionStr.bpm, nt);
    m_sigLabel.setText(positionStr.signature, nt);
    m_barBeatTickLabel.setText(positionStr.beats, nt);
    m_timeLabel.setText(positionStr.time, nt);
    m_loopInLabel.setText(positionStr.loopIn, nt);
    m_loopOutLabel.setText(positionStr.loopOut, nt);
}
