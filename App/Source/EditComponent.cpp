#include "EditComponent.h"

PlayheadComponent::PlayheadComponent (te::Edit& e , EditViewState& evs)
    : m_edit (e), m_editViewState (evs)
{
    startTimerHz (30);
}

void PlayheadComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::antiquewhite);
    g.drawRect (m_xPosition, 0, 2, getHeight());
}

bool PlayheadComponent::hitTest (int x, int)
{
    if (std::abs (x - m_xPosition) <= 3)
    {
        return true;
    }
    return false;
}

void PlayheadComponent::mouseDown (const juce::MouseEvent&)
{
    //edit.getTransport().setUserDragging (true);
}

void PlayheadComponent::mouseUp (const juce::MouseEvent&)
{
    m_edit.getTransport().setUserDragging (false);
}

void PlayheadComponent::mouseDrag (const juce::MouseEvent& e)
{
    double t = m_editViewState.beatToTime(m_editViewState.xToBeats (
                                            e.x, getWidth()));
    m_edit.getTransport().setCurrentPosition (t);
    timerCallback();
}

void PlayheadComponent::timerCallback()
{
    if (m_firstTimer)
    {
        // On Linux, don't set the mouse cursor until after the Component has appeared
        m_firstTimer = false;
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    }

    int newX = m_editViewState.beatsToX (
                m_edit.tempoSequence.timeToBeats(
                    m_edit.getTransport().getCurrentPosition())
                    , getWidth());
    if (newX != m_xPosition)
    {
        repaint (juce::jmin (newX, m_xPosition) - 1
                 , 0
                 , juce::jmax (newX, m_xPosition)
                    - juce::jmin (newX, m_xPosition) + 3
                 , getHeight());
        m_xPosition = newX;
    }
}

//==============================================================================

ToolBarComponent::ToolBarComponent()
{
}

void ToolBarComponent::paint(juce::Graphics &g)
{
    g.setColour(juce::Colour (0xff181818));
    auto cornerSize = 10.0;
    g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);
    g.fillAll ();
}

//==============================================================================

EditComponent::EditComponent (te::Edit& e, te::SelectionManager& sm)
    : m_edit (e), m_editViewState (e, sm), m_scrollbar (true)
{
    m_edit.state.addListener (this);
    m_editViewState.m_selectionManager.addChangeListener (this);

    m_timeLine.setAlwaysOnTop (true);

    m_scrollbar.setAlwaysOnTop (true);
    m_scrollbar.setAutoHide (false);
    m_scrollbar.addListener (this);

    m_lowerRange.setAlwaysOnTop(true);

    m_playhead.setAlwaysOnTop (true);

    m_toolBar.setAlwaysOnTop (true);
    
    addAndMakeVisible (m_timeLine);
    addAndMakeVisible (m_scrollbar);
    addAndMakeVisible (m_lowerRange);
    addAndMakeVisible (m_playhead);
    addAndMakeVisible (m_toolBar);

    markAndUpdate (m_updateTracks);
    m_editViewState.m_selectionManager.selectOnly (
                te::getAllTracks (m_edit).getLast ());
}

EditComponent::~EditComponent()
{
    m_editViewState.m_selectionManager.removeChangeListener (this);
    m_edit.state.removeListener (this);
}

void EditComponent::valueTreePropertyChanged (
        juce::ValueTree& v, const juce::Identifier& i)
{
    if (v.hasType (IDs::EDITVIEWSTATE))
    {
        if (i == IDs::viewX1
            || i == IDs::viewX2
            || i == IDs::viewY)
        {
            markAndUpdate (m_updateZoom);
        }
        else if (i == IDs::showHeaders
                 || i == IDs::showFooters)
        {
            markAndUpdate (m_updateZoom);
        }
        else if (i == IDs::drawWaveforms)
        {
            repaint();
        }
    }
}

void EditComponent::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& c)
{
    if (te::TrackList::isTrack (c))
        markAndUpdate (m_updateTracks);
}

void EditComponent::valueTreeChildRemoved (
        juce::ValueTree&, juce::ValueTree& c, int)
{
    if (te::TrackList::isTrack (c))
        markAndUpdate (m_updateTracks);
}

void EditComponent::valueTreeChildOrderChanged (
        juce::ValueTree& v, int a, int b)
{
    if (te::TrackList::isTrack (v.getChild (a)))
        markAndUpdate (m_updateTracks);
    else if (te::TrackList::isTrack (v.getChild (b)))
        markAndUpdate (m_updateTracks);
}

void EditComponent::mouseDown(const juce::MouseEvent &event)
{
    if (event.mods.isPopupMenu())
    {
        juce::PopupMenu m;
        m.addItem (10, "Add track");

        m.addSeparator();

        const int res = m.show();

        if (res == 10)
        {
            auto red = juce::Random::getSystemRandom().nextInt(juce::Range<int>(0, 255));
            auto gre = juce::Random::getSystemRandom().nextInt(juce::Range<int>(0, 255));
            auto blu = juce::Random::getSystemRandom().nextInt(juce::Range<int>(0, 255));
            if (auto track = EngineHelpers::getOrInsertAudioTrackAt (
                    m_edit, te::getAudioTracks(m_edit).size()))
            {
                 track->state.setProperty(  te::IDs::height
                                          , track->defaultTrackHeight
                                          , &m_edit.getUndoManager());

                 track->setName(
                    "Track " + juce::String(te::getAudioTracks(m_edit).size()));
                 track->setColour(juce::Colour(red, gre, blu));
                 m_editViewState.m_selectionManager.selectOnly(track);
            }
        }
    }
}

void EditComponent::mouseWheelMove(const juce::MouseEvent &event
                                   , const juce::MouseWheelDetails &wheel)
{
    if (event.mods.isShiftDown())
    {
        auto rangeBegin = m_editViewState.beatsToX(
                                m_editViewState.m_viewX1, m_timeLine.getWidth());
        auto visibleLength = m_editViewState.m_viewX2 - m_editViewState.m_viewX1;

        rangeBegin -= static_cast<int>(wheel.deltaY * 300);

        m_editViewState.m_viewX1 = juce::jmax (0.0
                                     , m_editViewState.xToBeats(
                                         rangeBegin, m_timeLine.getWidth()));
        m_editViewState.m_viewX2 = m_editViewState.m_viewX1 + visibleLength;
    }
    else if (event.mods.isCtrlDown())
    {

    }
    else
    {
        m_scrollbar.setCurrentRangeStart(
                    m_scrollbar.getCurrentRangeStart() - wheel.deltaY * 300);
    }
}


void EditComponent::paint (juce::Graphics &g)
{
    g.setColour(juce::Colour(0xff181818));
    auto cornerSize = 10.0;
    g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);

    g.setColour(juce::Colour(0xff4f4f4f));
    g.drawRect(m_editViewState.m_headerWidth, 0, 1, getHeight());
}

void EditComponent::handleAsyncUpdate()
{
    if (compareAndReset (m_updateTracks))
        buildTracks();
    if (compareAndReset (m_updateZoom))
    {
        resized();
        m_timeLine.repaint ();
    }
}

void EditComponent::resized()
{
    jassert (m_headers.size() == m_trackComps.size());
    
    const int timelineHeight = 50;
    const int trackGap = 0;
    const int headerWidth = m_editViewState.m_showHeaders
                          ? m_editViewState.m_headerWidth
                          : 0;
    const int footerWidth = m_editViewState.m_showFooters ? 150 : 0;
    const int pluginRackHeight = 250;



    auto area = getLocalBounds();
    auto pluginRackRect = area.removeFromBottom(pluginRackHeight);
    
    int y = juce::roundToInt (m_editViewState.m_viewY.get()) + timelineHeight;

    int trackHeight = 30;
    int trackHeights = 0;
    for (int i = 0; i < juce::jmin (m_headers.size(), m_trackComps.size()); i++)
    {
        auto trackHeader = m_headers[i];
        auto trackComp = m_trackComps[i];
        auto pluginRack = m_pluginRackComps[i];

        trackHeight = m_trackComps[i]->getTrack()->state.getProperty(
                        tracktion_engine::IDs::height,50);
        trackHeights += trackHeight;
        trackHeader->setBounds (2, y, headerWidth-2, trackHeight);
        trackComp->setBounds (headerWidth + 1
                              , y
                              , getWidth() - headerWidth - footerWidth
                              , trackHeight);
        pluginRack->setBounds (getWidth() - footerWidth
                               , y
                               , footerWidth
                               , trackHeight);

        y += trackHeight + trackGap;
    }

    for (auto t : m_trackComps)
        t->resized();

    m_lowerRange.setBounds (pluginRackRect);
    m_playhead.setBounds (
                area.withTrimmedLeft (headerWidth).withTrimmedRight (footerWidth));
    m_timeLine.setBounds(m_playhead.getBounds().removeFromTop(timelineHeight));
    m_toolBar.setBounds (0, 0, headerWidth, timelineHeight);

    auto songeditorHeight = getHeight() - timelineHeight - pluginRackHeight;
    m_scrollbar.setBounds (getWidth () - 20, timelineHeight, 20, songeditorHeight);
    m_scrollbar.setRangeLimits (0, trackHeights + (songeditorHeight/2));
    m_scrollbar.setCurrentRange (-(m_editViewState.m_viewY), songeditorHeight);
}

void EditComponent::changeListenerCallback(juce::ChangeBroadcaster *)
{
    repaint();
}

void EditComponent::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved
                                   , double newRangeStart)
{
    if (scrollBarThatHasMoved == &m_scrollbar)
    {
        m_editViewState.m_viewY = -newRangeStart;
    }
}

void EditComponent::buildTracks()
{
    m_trackComps.clear();
    m_headers.clear();
    m_pluginRackComps.clear();
    
    for (auto t : getAllTracks (m_edit))
    {
        TrackComponent* c = nullptr;
        
        if (t->isTempoTrack())
        {
            if (m_editViewState.m_showGlobalTrack)
                c = new TrackComponent (m_editViewState, t);
        }
        else if (t->isMarkerTrack())
        {
            if (m_editViewState.m_showMarkerTrack)
                c = new TrackComponent (m_editViewState, t);
        }
        else if (t->isChordTrack())
        {
            if (m_editViewState.m_showChordTrack)
                c = new TrackComponent (m_editViewState, t);
        }
        else if (t->isArrangerTrack())
        {
            if (m_editViewState.m_showArrangerTrack)
                c = new TrackComponent (m_editViewState, t);
        }
        else
        {
            c = new TrackComponent (m_editViewState, t);
        }
        
        if (c != nullptr)
        {
            m_trackComps.add (c);
            addAndMakeVisible (c);
            
            auto h = new TrackHeaderComponent (m_editViewState, t);
            m_headers.add (h);
            addAndMakeVisible (h);
            
            auto f = new PluginRackComponent (m_editViewState, t);
            m_pluginRackComps.add (f);
            addAndMakeVisible (f);
        }
    }
    
    m_playhead.toFront (false);
    resized();
}

juce::OwnedArray<TrackComponent> & EditComponent::getTrackComps()
{
    return m_trackComps;
}
