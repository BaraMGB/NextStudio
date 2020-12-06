#include "EditComponent.h"




//==============================================================================
PlayheadComponent::PlayheadComponent (te::Edit& e , EditViewState& evs)
    : edit (e), editViewState (evs)
{
    startTimerHz (30);
}

void PlayheadComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::antiquewhite);
    g.drawRect (xPosition, 0, 2, getHeight());
    //g.drawArrow(juce::Line<float>(xPosition, 0, xPosition,editViewState.headerHeight),1,3,10);
}

bool PlayheadComponent::hitTest (int x, int)
{
    if (std::abs (x - xPosition) <= 3)
        return true;
    
    return false;
}

void PlayheadComponent::mouseDown (const juce::MouseEvent&)
{
    //edit.getTransport().setUserDragging (true);
}

void PlayheadComponent::mouseUp (const juce::MouseEvent&)
{
    edit.getTransport().setUserDragging (false);
}

void PlayheadComponent::mouseDrag (const juce::MouseEvent& e)
{
    double t = editViewState.beatToTime(editViewState.xToBeats (e.x, getWidth()));
    edit.getTransport().setCurrentPosition (t);
    timerCallback();
}

void PlayheadComponent::timerCallback()
{
    if (firstTimer)
    {
        // On Linux, don't set the mouse cursor until after the Component has appeared
        firstTimer = false;
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    }

    int newX = editViewState.beatsToX (edit.tempoSequence.timeToBeats(edit.getTransport().getCurrentPosition()) , getWidth());
    if (newX != xPosition)
    {
        repaint (juce::jmin (newX, xPosition) - 1
                 , 0
                 , juce::jmax (newX, xPosition) - juce::jmin (newX, xPosition) + 3
                 , getHeight());
        xPosition = newX;
    }
}

//==============================================================================

ToolBarComponent::ToolBarComponent()
{
}

void ToolBarComponent::paint(juce::Graphics &g)
{
    g.setColour(juce::Colour (0xff181818));
    auto cornerSize = 10;
    g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);
    g.fillAll ();
}

//===============================================================================

EditComponent::EditComponent (te::Edit& e, te::SelectionManager& sm)
    : edit (e), editViewState (e, sm), m_scrollbar (true)
{
    edit.state.addListener (this);
    editViewState.selectionManager.addChangeListener (this);

    
    addAndMakeVisible (timeLine);
    timeLine.setAlwaysOnTop (true);
    addAndMakeVisible (m_scrollbar);
    m_scrollbar.setAlwaysOnTop (true);
    m_scrollbar.setAutoHide (false);
    m_scrollbar.addListener (this);
    addAndMakeVisible (pluginRack);
    pluginRack.setAlwaysOnTop(true);
    addAndMakeVisible (playhead);
    playhead.setAlwaysOnTop (true);
    addAndMakeVisible (toolBar);
    toolBar.setAlwaysOnTop (true);
    
    markAndUpdate (updateTracks);
    editViewState.selectionManager.selectOnly (te::getAllTracks (edit).getLast ());




}

EditComponent::~EditComponent()
{
    editViewState.selectionManager.removeChangeListener (this);
    edit.state.removeListener (this);
}

void EditComponent::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i)
{
    if (v.hasType (IDs::EDITVIEWSTATE))
    {
        if (i == IDs::viewX1
            || i == IDs::viewX2
            || i == IDs::viewY)
        {
            markAndUpdate (updateZoom);
        }
        else if (i == IDs::showHeaders
                 || i == IDs::showFooters)
        {
            markAndUpdate (updateZoom);
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
        markAndUpdate (updateTracks);
}

void EditComponent::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree& c, int)
{
    if (te::TrackList::isTrack (c))
        markAndUpdate (updateTracks);
}

void EditComponent::valueTreeChildOrderChanged (juce::ValueTree& v, int a, int b)
{
    if (te::TrackList::isTrack (v.getChild (a)))
        markAndUpdate (updateTracks);
    else if (te::TrackList::isTrack (v.getChild (b)))
        markAndUpdate (updateTracks);
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
            if (auto track = EngineHelpers::getOrInsertAudioTrackAt (edit, tracktion_engine::getAudioTracks(edit).size()))
            {
                 track->state.setProperty(te::IDs::height, track->defaultTrackHeight,&edit.getUndoManager());
                 track->setName("Track " + juce::String(tracktion_engine::getAudioTracks(edit).size()));
                 track->setColour(juce::Colour(red, gre, blu));
                 editViewState.selectionManager.selectOnly(track);
            }
        }
    }
    //editViewState.selectionManager.deselectAll();
}

void EditComponent::mouseWheelMove(const juce::MouseEvent &event
                                   , const juce::MouseWheelDetails &wheel)
{
    if (event.mods.isShiftDown())
    {
        double rangeBegin = editViewState.beatsToX(
                                editViewState.viewX1, timeLine.getWidth());
        double visibleLength = editViewState.viewX2 - editViewState.viewX1;

        rangeBegin -= wheel.deltaY * 300;

        editViewState.viewX1 = juce::jmax (0.0
                                     , editViewState.xToBeats(
                                         rangeBegin, timeLine.getWidth()));
        editViewState.viewX2 = editViewState.viewX1 + visibleLength;
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
    auto rect = getLocalBounds();
    g.setColour(juce::Colour(0xff181818));
    auto cornerSize = 10;
    g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);

    g.setColour(juce::Colour(0xff4f4f4f));
    g.drawRect(editViewState.headerWidth, 0, 1, getHeight());
}

void EditComponent::handleAsyncUpdate()
{
    if (compareAndReset (updateTracks))
        buildTracks();
    if (compareAndReset (updateZoom))
    {
        resized();
        timeLine.repaint ();
    }
}

void EditComponent::resized()
{
    jassert (headers.size() == tracks.size());
    
    const int timelineHeight = 50;
    const int trackGap = 0;
    const int headerWidth = editViewState.showHeaders ? editViewState.headerWidth : 0;
    const int footerWidth = editViewState.showFooters ? 150 : 0;
    const int pluginRackHeight = 250;



    auto area = getLocalBounds();
    auto pluginRackRect = area.removeFromBottom(pluginRackHeight);
    
    int y = juce::roundToInt (editViewState.viewY.get()) + timelineHeight;

    int trackHeight = 30;
    int trackHeights = 0;
    for (int i = 0; i < juce::jmin (headers.size(), tracks.size()); i++)
    {
        auto h = headers[i];
        auto t = tracks[i];
        auto f = footers[i];
        trackHeight = tracks[i]->getTrack()->state.getProperty(tracktion_engine::IDs::height,50);
        trackHeights += trackHeight;
        h->setBounds (2, y, headerWidth-2, trackHeight);
        t->setBounds (headerWidth + 1, y, getWidth() - headerWidth - footerWidth, trackHeight);
        f->setBounds (getWidth() - footerWidth, y, footerWidth, trackHeight);

        y += trackHeight + trackGap;
    }

    for (auto t : tracks)
        t->resized();

    pluginRack.setBounds (pluginRackRect);
    playhead.setBounds (area.withTrimmedLeft (headerWidth).withTrimmedRight (footerWidth));
    timeLine.setBounds(playhead.getBounds().removeFromTop(timelineHeight));
    toolBar.setBounds (0, 0, headerWidth, timelineHeight);

    auto songeditorHeight = getHeight() - timelineHeight - pluginRackHeight;
    m_scrollbar.setBounds (getWidth () - 20, timelineHeight, 20, songeditorHeight);
    m_scrollbar.setRangeLimits (0, trackHeights + (songeditorHeight/2));
    m_scrollbar.setCurrentRange (-(editViewState.viewY), songeditorHeight);
}

void EditComponent::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
    if (scrollBarThatHasMoved == &m_scrollbar)
    {
        editViewState.viewY = -(newRangeStart);
    }
}

void EditComponent::buildTracks()
{
    tracks.clear();
    headers.clear();
    footers.clear();
    
    for (auto t : getAllTracks (edit))
    {
        TrackComponent* c = nullptr;
        
        if (t->isTempoTrack())
        {
            if (editViewState.showGlobalTrack)
                c = new TrackComponent (editViewState, t);
        }
        else if (t->isMarkerTrack())
        {
            if (editViewState.showMarkerTrack)
                c = new TrackComponent (editViewState, t);
        }
        else if (t->isChordTrack())
        {
            if (editViewState.showChordTrack)
                c = new TrackComponent (editViewState, t);
        }
        else if (t->isArrangerTrack())
        {
            if (editViewState.showArrangerTrack)
                c = new TrackComponent (editViewState, t);
        }
        else
        {
            c = new TrackComponent (editViewState, t);
        }
        
        if (c != nullptr)
        {
            tracks.add (c);
            addAndMakeVisible (c);
            
            auto h = new TrackHeaderComponent (editViewState, t);
            headers.add (h);
            addAndMakeVisible (h);
            
            auto f = new PluginRackComponent (editViewState, t);
            footers.add (f);
            addAndMakeVisible (f);
        }
    }
    
    playhead.toFront (false);
    resized();
}

juce::OwnedArray<TrackComponent> & EditComponent::getTrackComps()
{
    return tracks;
}

//--------------------------------------------------------------------------------------

