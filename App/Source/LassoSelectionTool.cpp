#include "LassoSelectionTool.h"


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
    updateClipCache();

    setVisible(true);

    m_clickedTime = xToTime (e.getMouseDownX ());

    m_cachedY = m_editViewState.m_viewY;
}
void LassoSelectionTool::updateClipCache()
{
    m_cachedSelectedClips.clear();

    for (auto c : m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
        m_cachedSelectedClips.add(c);
}
void LassoSelectionTool::updateLasso(const juce::MouseEvent& e)
{
    m_isLassoSelecting = true;

    auto viewOffsetY = m_editViewState.m_viewY - m_cachedY;
    auto startY = e.getMouseDownY () + viewOffsetY;

    double top =    juce::jmin(startY, (double) e.y);
    double bottom = juce::jmax(startY, (double) e.y);

    m_lassoRect = {getDraggedTimeRange(e), top, bottom};

    updateSelection(e.mods.isCtrlDown ());

    repaint ();
}
te::EditTimeRange LassoSelectionTool::getDraggedTimeRange(const juce::MouseEvent& e)
{
    auto draggedTime = xToTime(e.x);
    te::EditTimeRange timeRange(juce::jmin(draggedTime, m_clickedTime)
                              , juce::jmax(draggedTime, m_clickedTime));
    return timeRange;
}

void LassoSelectionTool::updateSelection(bool add)
{

    m_editViewState.m_selectionManager.deselectAll ();

    double trackPosY = m_editViewState.m_viewY;
    for (auto track: te::getAudioTracks(m_editViewState.m_edit))
    {
        if (getVerticalRangeOfTrack(trackPosY, track)
                .intersects (m_lassoRect.m_verticalRange))
        {
            selectCatchedClips(track);
        }
        trackPosY += GUIHelpers::getTrackHeight(track, m_editViewState);
    }

    if (add)
    {
        for (auto c : m_cachedSelectedClips)
            m_editViewState.m_selectionManager.addToSelection(c);
    }
}
void LassoSelectionTool::selectCatchedClips(
    const tracktion_engine::AudioTrack* track)
{
    for (auto c : track->getClips())
    {
        if (m_lassoRect.m_startTime < c->getPosition ().getEnd ()
            && m_lassoRect.m_endTime > c->getPosition ().getStart ())
        {
            m_editViewState.m_selectionManager.addToSelection(c);
        }
    }
}
juce::Range<double> LassoSelectionTool::getVerticalRangeOfTrack(
    double trackPosY, tracktion_engine::AudioTrack* track) const
{
    auto trackTop = (double) trackPosY;
    auto trackBottom = trackTop + (double) GUIHelpers::getTrackHeight(track, m_editViewState, false);

    return {trackTop , trackBottom};
}
double LassoSelectionTool::xToTime(const int x)
{
    return m_editViewState.xToTime (x, getWidth (), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
}
void LassoSelectionTool::stopLasso()
{
    setVisible(false);
    m_isLassoSelecting = false;
}
