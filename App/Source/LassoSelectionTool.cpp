#include "LassoSelectionTool.h"
#include "MidiViewport.h"

juce::Rectangle<int> LassoSelectionTool::LassoRect::getRect(EditViewState& evs
                                                            , double viewX1
                                                            , double viewX2
                                                            , int viewWidth) const
{
    auto x = evs.timeToX (m_startTime, viewWidth, viewX1, viewX2);
    auto y = (int) m_top;
    auto w = evs.timeToX (m_endTime, viewWidth, viewX1, viewX2) - x;
    auto h = (int) m_bottom - (int) m_top;

    return  {x, y, w, h};
}
void LassoSelectionTool::paint(juce::Graphics &g)
{
    if (m_isLassoSelecting && !m_isRangeSelecting)
    {
        g.setColour (juce::Colour(0x99FFFFFF));
        auto rect = m_lassoRect.getRect (m_editViewState
                                        , m_X1
                                        , m_X2
                                        , getWidth ());
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
    return m_editViewState.xToTime (x, getWidth (), m_X1, m_X2);
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
