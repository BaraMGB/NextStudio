
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

#include "LassoSelectionTool.h"
#include "MidiViewport.h"

juce::Rectangle<int> LassoSelectionTool::LassoRect::getRect(EditViewState& evs
                                                            , double viewX1
                                                            , double viewX2
                                                            , int viewWidth) const
{
    int x = evs.timeToX (m_startTime, viewWidth, viewX1, viewX2);
    auto y = (int) m_top;
    int w = evs.timeToX (m_endTime, viewWidth, viewX1, viewX2) - x;
    auto h = (int) m_bottom - (int) m_top;

    return  {x, y, w, h};
}

void LassoSelectionTool::drawLasso(juce::Graphics &g)
{
    if (m_isLassoSelecting && !m_isRangeSelecting)
    {
        auto x1 = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
        auto x2 = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
        g.setColour (juce::Colour(0x99FFFFFF));
        auto rect = m_lassoRect.getRect (m_editViewState, x1, x2, getWidth ());
        g.drawRect (rect);
        g.setColour (juce::Colour(0x22FFFFFF));
        g.fillRect (rect);
    }
}
void LassoSelectionTool::startLasso(const juce::Point<int> mousePos, int startYScroll, bool isRangeTool)
{
    m_isRangeSelecting = isRangeTool;
    setVisible(true);
    if (!isRangeTool)
        setMouseCursor (juce::MouseCursor::CrosshairCursor);
    else
        setMouseCursor (juce::MouseCursor::IBeamCursor);

    m_clickedTime = xToTime (mousePos.getX());
    m_startYScroll = startYScroll;
    m_startPos = mousePos;
}
void LassoSelectionTool::updateLasso(const juce::Point<int> mousePos, int yScroll)
{
    m_isLassoSelecting = true;

    auto oldY = m_startPos.getY() + (yScroll - m_startYScroll);

    double top =    juce::jmin(oldY, mousePos.y);
    double bottom = juce::jmax(oldY, mousePos.y);

    auto currentTime = xToTime(mousePos.x);
    auto start = tracktion::TimePosition::fromSeconds(juce::jmin(currentTime, m_clickedTime));
    auto end = tracktion::TimePosition::fromSeconds(juce::jmax(currentTime, m_clickedTime));

    tracktion::TimeRange tr(start, end);

    m_lassoRect = {tr, top, bottom};

    repaint ();
}

double LassoSelectionTool::xToTime(const int x)
{
    auto x1 = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    auto x2 = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    return m_editViewState.xToTime (x, getWidth (), x1, x2);
}
LassoSelectionTool::LassoRect LassoSelectionTool::getLassoRect() const
{
    return m_lassoRect;
}
void LassoSelectionTool::stopLasso()
{
    setVisible(false);
    m_isLassoSelecting = false;
}
