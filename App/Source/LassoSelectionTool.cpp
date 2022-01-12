#include "LassoSelectionTool.h"
#include "EditComponent.h"

juce::Rectangle<int> LassoSelectionTool::LassoRect::getRect(EditViewState& evs,
                                                            int viewWidth) const
{
    auto x = evs.timeToX (m_startTime, viewWidth, evs.m_viewX1, evs.m_viewX2);
    auto y = (int) m_top;
    auto w = evs.timeToX (m_endTime, viewWidth, evs.m_viewX1, evs.m_viewX2) - x;
    auto h = (int) m_bottom - (int) m_top;

    return  {x, y, w, h};
}
void LassoSelectionTool::paint(juce::Graphics &g)
{
    if (m_isLassoSelecting)
    {
        g.setColour (juce::Colour(0x99FFFFFF));
        g.drawRect (m_lassoRect.getRect (m_editViewState, getWidth ()));
        g.setColour (juce::Colour(0x22FFFFFF));
        g.fillRect (m_lassoRect.getRect (m_editViewState, getWidth ()));
    }
}
void LassoSelectionTool::mouseDown(const juce::MouseEvent &e)
{
    m_clickedTime = m_editViewState.xToTime (e.getMouseDownX (), getWidth ()
                               , m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    m_cachedY = m_editViewState.m_viewY;
}
void LassoSelectionTool::mouseDrag(const juce::MouseEvent &e)
{
    m_isLassoSelecting = true;
    auto scrolledYoffset = m_editViewState.m_viewY - m_cachedY;

    auto draggedTime = xToTime(e.x);
    te::EditTimeRange timeRange(juce::jmin(draggedTime, m_clickedTime)
                              , juce::jmax(draggedTime, m_clickedTime));

    auto startY = e.getMouseDownY () + scrolledYoffset;

    double top =    juce::jmin(startY, (double) e.y);
    double bottom = juce::jmax(startY, (double) e.y);

    m_lassoRect = {timeRange, top, bottom};
    updateSelection(e.mods.isCtrlDown ());
    repaint ();
}

void LassoSelectionTool::mouseUp(const juce::MouseEvent &)
{
    m_isLassoSelecting = false;
    repaint();
}

void LassoSelectionTool::updateSelection(bool add)
{
    m_editViewState.m_selectionManager.deselectAll ();

    auto trackPosY = 0;
    for (auto track: te::getAudioTracks(m_editViewState.m_edit))
    {
        auto trackTop = (double) trackPosY;
        auto trackBottom = trackTop + (double) GUIHelpers::getTrackHeight(track, m_editViewState);
        juce::Range<double> trackVerticalRange = {trackTop , trackBottom};

        if (trackVerticalRange.intersects (m_lassoRect.m_verticalRange))
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
        trackPosY += GUIHelpers::getTrackHeight(track, m_editViewState);
    }
}

double LassoSelectionTool::xToTime(const int x)
{
    return m_editViewState.xToTime (x, getWidth (), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
}

