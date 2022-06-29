#include "EditComponent.h"

#include <utility>
#include "NextLookAndFeel.h"
EditComponent::EditComponent (te::Edit& e, ApplicationViewState& avs, te::SelectionManager& sm)
    : m_edit (e)
  , m_editViewState (e, sm, avs)
    , m_songEditor(m_editViewState)
    , m_trackListView(m_editViewState)
  , m_scrollbar_v (true)
  , m_scrollbar_h (false)
{
    m_edit.state.addListener (this);

    m_scrollbar_v.setAlwaysOnTop (true);
    m_scrollbar_v.setAutoHide (false);
    m_scrollbar_v.addListener (this);

    m_scrollbar_h.setAlwaysOnTop (true);
    m_scrollbar_h.setAutoHide (false);
    m_scrollbar_h.addListener (this);

    m_timeLine.setAlwaysOnTop (true);
    m_playhead.setAlwaysOnTop (true);
    m_footerbar.setAlwaysOnTop (true);
    m_footerbar.toFront (true);


    addAndMakeVisible (m_timeLine);
    addAndMakeVisible (m_scrollbar_v);
    addAndMakeVisible (m_scrollbar_h);
    addAndMakeVisible (m_playhead);
    addAndMakeVisible (m_footerbar);
    addAndMakeVisible (m_songEditor);
    addAndMakeVisible (m_trackListView);

    markAndUpdate (m_updateTracks);
    m_editViewState.m_selectionManager.selectOnly (
                te::getAllTracks (m_edit).getLast ());
}

EditComponent::~EditComponent()
{
    m_edit.state.removeListener (this);
}

void EditComponent::paint (juce::Graphics &g)
{
    g.setColour(juce::Colour(0xff181818));
    g.fillRect(getEditorHeaderRect());
    g.setColour(juce::Colour(0xff272727));

    g.fillRect(getTrackListToolsRect());
    g.fillRect(getTrackListRect());
    g.fillRect(getTimeLineRect());
    g.fillRect(getSongEditorRect());
}

void EditComponent::paintOverChildren(juce::Graphics &g)
{
    g.setColour(juce::Colours::white);
    g.drawHorizontalLine (getEditorHeaderRect ().getBottom (), 0, getWidth ());
    g.drawHorizontalLine (getTimeLineRect ().getBottom () - 1, 0, getWidth ());
    g.drawHorizontalLine (getSongEditorRect ().getBottom (), 0, getWidth ());

    g.drawVerticalLine (getTrackListRect ().getRight (),
                        getTimeLineRect ().getY (),
                        getTimeLineRect ().getBottom ());

    //rounded corners
    g.setColour(juce::Colour(0xff555555));

    juce::Path fakeRoundedCorners;
    auto bounds = getLocalBounds ();

    const float cornerSize = 10.f;
    fakeRoundedCorners.addRectangle(bounds);
    fakeRoundedCorners.setUsingNonZeroWinding(false);
    fakeRoundedCorners.addRoundedRectangle(bounds, cornerSize);

    g.fillPath(fakeRoundedCorners);
}


void EditComponent::resized()
{
    m_timeLine.setBounds(getTimeLineRect());
    m_trackListView.setBounds(getTrackListRect());
    m_trackListView.resized();
    m_songEditor.setBounds(getSongEditorRect());
    m_songEditor.resized();
    m_scrollbar_v.setBounds (getSongEditorRect().removeFromRight(20));
    m_scrollbar_v.setRangeLimits (0, getSongHeight() + (m_songEditor.getHeight() / 2));
    m_scrollbar_v.setCurrentRange (-m_editViewState.m_viewY, getSongEditorRect().getHeight());
    m_footerbar.setBounds(getFooterRect());
    m_playhead.setBounds(getPlayHeadRect());
    m_scrollbar_h.setBounds(getSongEditorRect().removeFromBottom(20));
}
void EditComponent::updateHorizontalScrollBar()
{
    m_scrollbar_h.setRangeLimits (
                {0.0, m_editViewState.getEndScrollBeat ()});
    m_scrollbar_h.setCurrentRange ({m_editViewState.m_viewX1
                                  , m_editViewState.m_viewX2});
}



void EditComponent::mouseWheelMove(const juce::MouseEvent &event
                                   , const juce::MouseWheelDetails &wheel)
{
    if (event.mods.isShiftDown())
    {
        auto rangeBegin = m_editViewState.beatsToX(
                                m_editViewState.m_viewX1
                              , m_timeLine.getWidth()
                              , m_editViewState.m_viewX1
                              , m_editViewState.m_viewX2);
        auto visibleLength = m_editViewState.m_viewX2
                              - m_editViewState.m_viewX1;

        rangeBegin -=
                #if JUCE_MAC
                static_cast<int>(wheel.deltaX * 300);
                #else
                static_cast<int>(wheel.deltaY * 300);
                #endif

        m_editViewState.m_viewX1 = juce::jmax (0.0
                                     , m_editViewState.xToBeats(
                                         rangeBegin, m_timeLine.getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2));
        m_editViewState.m_viewX2 = m_editViewState.m_viewX1 + visibleLength;
    }
    else if (event.mods.isCtrlDown())
    {

    }
    else
    {
        m_scrollbar_v.setCurrentRangeStart(
                    m_scrollbar_v.getCurrentRangeStart() - wheel.deltaY * 300);
    }
}

void EditComponent::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved
                                   , double newRangeStart)
{
    if (scrollBarThatHasMoved == &m_scrollbar_v)
    {
        m_editViewState.m_viewY = -newRangeStart;
    }
    else if(scrollBarThatHasMoved == &m_scrollbar_h)
    {
        GUIHelpers::moveView(m_editViewState, juce::jmax(0.0, newRangeStart));
    }
}

void EditComponent::valueTreePropertyChanged (
        juce::ValueTree& v, const juce::Identifier& i)
{
    if (i == te::IDs::height || i == IDs::isTrackMinimized)
    {
        m_songEditor.resized();
    }

    if (i == te::IDs::loopPoint1
        || i == te::IDs::loopPoint2
        || i == te::IDs::looping)
        markAndUpdate(m_updateZoom);

    if (v.hasType (IDs::EDITVIEWSTATE))
    {
        if (i == IDs::viewX1
            || i == IDs::viewX2
            || i == IDs::isPianoRollVisible
            || i == IDs::pianorollHeight
            || i == IDs::showHeaders
            || i == IDs::showFooters)
            markAndUpdate (m_updateZoom);
        else if (i == IDs::viewY)
            markAndUpdate(m_updateSongEditor);
        else if (i == IDs::drawWaveforms)
            repaint();
    }
}

void EditComponent::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& c)
{
    if (te::MidiClip::isClipState (c))
    {
        markAndUpdate (m_updateZoom);
    }
    if (te::TrackList::isTrack (c))
    {
        markAndUpdate (m_updateTracks);
    }
    if (c.hasType(te::IDs::AUTOMATIONCURVE))
    {
        GUIHelpers::log(c.toXmlString());
        markAndUpdate (m_updateTracks);
    }
}

void EditComponent::valueTreeChildRemoved (
        juce::ValueTree&, juce::ValueTree& c, int)
{
    if (te::MidiClip::isClipState (c))
    {
        resized ();
    }
    if (te::TrackList::isTrack (c))
    {
        m_lowerRange.removePluginRackwithTrack (m_edit.getTrackList ().getTrackFor (c));
        markAndUpdate (m_updateTracks);
    }
    if (c.hasType(te::IDs::POINT))
    {
        markAndUpdate(m_updateTracks);
    }
    if (c.hasType(te::IDs::PLUGIN))
    {
        markAndUpdate(m_updateTracks);
    }

}

void EditComponent::valueTreeChildOrderChanged (
        juce::ValueTree& v, int a, int b)
{
    if (te::TrackList::isTrack (v.getChild (a))
        || te::TrackList::isTrack (v.getChild (b)))
        markAndUpdate (m_updateTracks);
}

void EditComponent::handleAsyncUpdate()
{
    if (compareAndReset (m_updateTracks))
        buildTracks();
    if (compareAndReset (m_updateZoom))
    {
        refreshSnapTypeDesc();

        m_timeLine.repaint ();

        for (auto tv : m_songEditor.getTrackViews())
        {
            tv->repaint ();
            tv->resized ();
        }
        updateHorizontalScrollBar();
    }
    if (compareAndReset(m_updateSongEditor))
    {
        resized();
    }
}


void EditComponent::refreshSnapTypeDesc()
{
    m_footerbar.m_snapTypeDesc =
            m_timeLine.getEditViewState ().getSnapTypeDescription (
                m_timeLine.getBestSnapType ().level);
    m_footerbar.repaint ();
}

void EditComponent::buildTracks()
{
    m_lowerRange.clearPluginRacks ();
    m_songEditor.clear();
    m_trackListView.clear();

    for (auto t : getAllTracks (m_edit))
    {
        TrackComponent* tc = nullptr;

        if (t->isTempoTrack())
        {
            if (m_editViewState.m_showGlobalTrack)
                tc = new TrackComponent (m_editViewState, m_lowerRange, t);
        }
        else if (t->isMarkerTrack())
        {
            if (m_editViewState.m_showMarkerTrack)
                tc = new TrackComponent (m_editViewState,  m_lowerRange, t);
        }
        else if (t->isChordTrack())
        {
            if (m_editViewState.m_showChordTrack)
                tc = new TrackComponent (m_editViewState,  m_lowerRange, t);
        }
        else if (t->isArrangerTrack())
        {
            if (m_editViewState.m_showArrangerTrack)
                tc = new TrackComponent (m_editViewState,  m_lowerRange, t);
        }
        else if (t->isMasterTrack())
        {
            if (m_editViewState.m_showMasterTrack)
                tc = new TrackComponent (m_editViewState,  m_lowerRange, t);
        }
        else
        {
            tc = new TrackComponent (m_editViewState,  m_lowerRange, t);
        }

        if (tc != nullptr)
        {
            auto th = new TrackHeaderComponent (m_editViewState, t);
            th->addChangeListener(&m_lowerRange);
            m_trackListView.addHeaderViews(*th);

            auto pr = new PluginRackComponent (m_editViewState, t);
            m_lowerRange.addPluginRackComp(pr);

            m_songEditor.addTrackView(*tc);
        }
    }

    m_trackListView.updateViews();
    m_songEditor.updateViews();
    m_playhead.toFront (false);
    resized();
}

LowerRangeComponent& EditComponent::lowerRange()
{
    return m_lowerRange;
}
juce::Rectangle<int> EditComponent::getEditorHeaderRect()
{
    return {0,0,getWidth(), m_editViewState.m_timeLineHeight};
}

juce::Rectangle<int> EditComponent::getTimeLineRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getEditorHeaderRect().getHeight());
    area.removeFromLeft(m_editViewState.m_trackHeaderWidth);
    return area.removeFromTop(m_editViewState.m_timeLineHeight);
}
juce::Rectangle<int> EditComponent::getTrackListToolsRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getEditorHeaderRect().getHeight());
    area.removeFromRight(getWidth() - m_editViewState.m_trackHeaderWidth);
    return area.removeFromTop(m_editViewState.m_timeLineHeight);
}
juce::Rectangle<int> EditComponent::getTrackListRect()
{
    auto area = getLocalBounds();

    area.removeFromTop(getEditorHeaderRect().getHeight());
    area.removeFromTop(m_editViewState.m_timeLineHeight);
    area.removeFromBottom(getFooterRect().getHeight());
    return area.removeFromLeft(m_editViewState.m_trackHeaderWidth);
}
juce::Rectangle<int> EditComponent::getSongEditorRect()
{
    auto area = getLocalBounds();

    area.removeFromTop(getEditorHeaderRect().getHeight());
    area.removeFromTop(m_editViewState.m_timeLineHeight);
    area.removeFromBottom(getFooterRect().getHeight());
    return area.removeFromRight(getWidth() - m_editViewState.m_trackHeaderWidth);
}
juce::Rectangle<int> EditComponent::getFooterRect()
{
    auto area = getLocalBounds();
    return area.removeFromBottom(30);
}
juce::Rectangle<int> EditComponent::getPlayHeadRect()
{
    auto h = getTimeLineRect().getHeight() + getSongEditorRect().getHeight();
    auto w = getTimeLineRect().getWidth();
    return {getTimeLineRect().getX(), getTimeLineRect().getY(), w, h};
}
int EditComponent::getSongHeight()
{
    auto h = 0;
    for (auto tc : m_songEditor.getTrackComps())
    {
        h = h + tc->getHeight();
    }
    return h;
}
void EditComponent::loopAroundSelection()
{
    auto& transport = m_edit.getTransport();
    if (getSelectedClipRange().getLength() > 0)
        transport.setLoopRange (getSelectedClipRange());
}
tracktion_engine::EditTimeRange EditComponent::getSelectedClipRange()
{
    if (m_editViewState.m_selectionManager.getItemsOfType<te::Clip>().size() == 0)
        return {0.0, 0.0};

    auto start = m_edit.getLength();
    auto end = 0.0;

    for (auto c: m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
    {
        start = c->getPosition().getStart() < start
            ? c->getPosition().getStart()
            : start;

        end = c->getPosition().getEnd() > end
            ? c->getPosition().getEnd()
            : end;
    }

    return {start, end};
}
