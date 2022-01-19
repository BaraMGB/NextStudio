#include "LassoSelectionTool.h"
#include "PianoRollContentComponent.h"

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
    if (m_isLassoSelecting)
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
void LassoSelectionTool::startLasso(const juce::MouseEvent& e)
{
    setMouseCursor (juce::MouseCursor::CrosshairCursor);

    setVisible(true);

    m_clickedTime = xToTime (e.getMouseDownX ());
}
void LassoSelectionTool::updateLasso(const juce::MouseEvent& e, int newTop)
{
    m_isLassoSelecting = true;
    std::cout << newTop << std::endl;
    double top =    juce::jmin(newTop, e.y);
    double bottom = juce::jmax(newTop, e.y);

    m_lassoRect = {getDraggedTimeRange(e), top, bottom};

    repaint ();
}
te::EditTimeRange LassoSelectionTool::getDraggedTimeRange(const juce::MouseEvent& e)
{
    auto draggedTime = xToTime(e.x);
    te::EditTimeRange timeRange(juce::jmin(draggedTime, m_clickedTime)
                              , juce::jmax(draggedTime, m_clickedTime));
    return timeRange;
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
