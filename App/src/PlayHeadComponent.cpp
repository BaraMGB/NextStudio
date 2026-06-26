
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

#include "PlayHeadComponent.h"

PlayheadComponent::PlayheadComponent(te::Edit &e, EditViewState &evs, TimeLineComponent &tc)
    : m_edit(e),
      m_editViewState(evs),
      m_timeLine(tc)
{
    startTimerHz(60);
}

void PlayheadComponent::paint(juce::Graphics &g)
{
    auto rect = juce::Rectangle<int>(m_xPosition, 0, 2, getHeight());
    auto bounds = getLocalBounds();
    if (!bounds.contains(rect))
        return;

    if (m_edit.getTransport().isPlaying())
    {
        g.setColour(m_editViewState.m_applicationState.getPrimeColour());

        g.drawRect(rect);
    }
    else
    {
        g.setColour(juce::Colour(0x66ffffff));
        g.drawRect(rect.removeFromLeft(1));
    }
}

bool PlayheadComponent::hitTest(int x, int y)
{
    if (std::abs(x - m_xPosition) <= 3 && y <= m_editViewState.m_timeLineHeight)
    {
        return true;
    }
    return false;
}

void PlayheadComponent::mouseDown(const juce::MouseEvent &)
{
    // edit.getTransport().setUserDragging (true);
}

void PlayheadComponent::mouseUp(const juce::MouseEvent &) { m_edit.getTransport().setUserDragging(false); }

void PlayheadComponent::mouseDrag(const juce::MouseEvent &e)
{
    auto x1 = m_timeLine.getCurrentBeatRange().getStart().inBeats();
    auto x2 = m_timeLine.getCurrentBeatRange().getEnd().inBeats();
    double t = m_editViewState.xToTime(e.x, getWidth(), x1, x2);
    m_edit.getTransport().setPosition(tracktion::TimePosition::fromSeconds(t));
    timerCallback();
}

bool PlayheadComponent::isPlaying() const { return m_editViewState.m_edit.getTransport().isPlaying(); }

void PlayheadComponent::timerCallback()
{
    if (isPlaying())
    {
        m_editViewState.updatePositionFollower(m_timeLine.getTimeLineID(), getWidth());
    }

    if (m_firstTimer)
    {
        // On Linux, don't set the mouse cursor until after the Component has appeared
        m_firstTimer = false;
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    }

    auto x1 = m_timeLine.getCurrentBeatRange().getStart().inBeats();
    auto x2 = m_timeLine.getCurrentBeatRange().getEnd().inBeats();
    int newX = m_editViewState.timeToX(m_edit.getTransport().getPosition().inSeconds(), getWidth(), x1, x2);

    if (newX != m_xPosition)
    {
        repaint(juce::jmin(newX, m_xPosition) - 1, 0, juce::jmax(newX, m_xPosition) - juce::jmin(newX, m_xPosition) + 3, getHeight());
        m_xPosition = newX;
    }
}
