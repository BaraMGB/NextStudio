
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

#include "EditComponent.h"
#include "MainComponent.h"
#include "Utilities.h"

EditComponent::EditComponent (te::Edit& e,EditViewState& evs, ApplicationViewState& avs, te::SelectionManager& sm, juce::ApplicationCommandManager& cm)
    : m_edit (e)
    , m_editViewState (evs)
    , m_songEditor(m_editViewState, m_toolBar, m_timeLine)
    , m_commandManager(cm)
    , m_trackListView(m_editViewState, m_timeLine.getTimeLineID())
    , m_scrollbar_v (true)
    , m_scrollbar_h (false)
    , m_addFolderTrackBtn ("Add folder track", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_addAudioTrackBtn("Add audio track", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_addMidiTrackBtn ("Add midi track", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_expandAllBtn("expand all tracks", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_collapseAllBtn("collapse all tracks", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_selectButton("select clips or automation points", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_lassoSelectButton("select clips or automation points with lasso band", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_timeRangeSelectButton("select time range", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_splitClipButton("split clip", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_timeStretchButton("stretch tempo of clips", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_reverseClipButton("reverse clips", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_deleteClipButton("delete clips", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
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
    addAndMakeVisible (m_trackListToolsMenu);
    addAndMakeVisible (m_toolBar);
    addAndMakeVisible (m_trackListControlMenu);

    //TrackListTools 
    GUIHelpers::setDrawableOnButton(m_addAudioTrackBtn, BinaryData::wavetest5_svg,
                                  juce::Colour(0xffffffff));
    m_addAudioTrackBtn.addListener(this);
    m_addAudioTrackBtn.setTooltip(GUIHelpers::translate("Add audio track",m_editViewState.m_applicationState));

    GUIHelpers::setDrawableOnButton(m_addMidiTrackBtn, BinaryData::piano5_svg, juce::Colour(0xffffffff));
    m_addMidiTrackBtn.addListener(this);
    m_addMidiTrackBtn.setTooltip(GUIHelpers::translate("Add MIDI track", m_editViewState.m_applicationState));

    GUIHelpers::setDrawableOnButton(m_addFolderTrackBtn, BinaryData::folderopen_svg, juce::Colour(0xffffffff));
    m_addFolderTrackBtn.addListener(this);
    m_addFolderTrackBtn.setTooltip(GUIHelpers::translate("Add folder track", m_editViewState.m_applicationState));

    m_trackListToolsMenu.addButton(&m_addAudioTrackBtn);
    m_trackListToolsMenu.addButton(&m_addMidiTrackBtn);
    m_trackListToolsMenu.addButton(&m_addFolderTrackBtn);

    //TrackListControl 
    GUIHelpers::setDrawableOnButton(m_expandAllBtn, BinaryData::expand_svg, juce::Colour(0xffffffff));
    m_expandAllBtn.addListener(this);
    m_expandAllBtn.setTooltip(GUIHelpers::translate("Expand all tracks", m_editViewState.m_applicationState));
    
    m_trackListControlMenu.addButton(&m_expandAllBtn);
    m_trackListControlMenu.addButton(&m_collapseAllBtn);
    
    GUIHelpers::setDrawableOnButton(m_collapseAllBtn, BinaryData::collapse_svg, juce::Colour(0xffffffff));
    m_collapseAllBtn.addListener(this);
    m_collapseAllBtn.setTooltip(GUIHelpers::translate("Collapse all tracks", m_editViewState.m_applicationState));

    //SongEditorTools 

    GUIHelpers::setDrawableOnButton(m_selectButton, BinaryData::select_icon_svg, juce::Colour(0xffffffff));
    m_selectButton.setName("select");
    m_selectButton.addListener(this);
    m_selectButton.setTooltip(GUIHelpers::translate("select clips or automation points", m_editViewState.m_applicationState));

    GUIHelpers::setDrawableOnButton(m_lassoSelectButton, BinaryData::rubberband_svg, juce::Colour(0xffffffff));
    m_lassoSelectButton.addListener(this);
    m_lassoSelectButton.setTooltip(GUIHelpers::translate("lasso select clips or automation points", m_editViewState.m_applicationState));

    GUIHelpers::setDrawableOnButton(m_timeRangeSelectButton, BinaryData::select_timerange_svg, juce::Colour(0xffffffff));
    m_timeRangeSelectButton.addListener(this);
    m_timeRangeSelectButton.setTooltip(GUIHelpers::translate("select clips or automation points within a time range", m_editViewState.m_applicationState));

    GUIHelpers::setDrawableOnButton(m_splitClipButton, BinaryData::split_svg, juce::Colour(0xffffffff));
    m_splitClipButton.addListener(this);
    m_splitClipButton.setTooltip(GUIHelpers::translate("split selected clip at the cursor position", m_editViewState.m_applicationState));

    GUIHelpers::setDrawableOnButton(m_timeStretchButton, BinaryData::time_stretch_button_svg, juce::Colour(0xffffffff));
    m_timeStretchButton.addListener(this);
    m_timeStretchButton.setTooltip(GUIHelpers::translate("apply time stretching to the selected clip", m_editViewState.m_applicationState));

    GUIHelpers::setDrawableOnButton(m_reverseClipButton, BinaryData::reverse_clip_svg, juce::Colour(0xffffffff));
    m_reverseClipButton.setCommandToTrigger(&m_commandManager,KeyPressCommandIDs::reverseClip , true);


    GUIHelpers::setDrawableOnButton(m_deleteClipButton, BinaryData::delete_icon_svg, juce::Colour(0xffffffff));
    m_deleteClipButton.setCommandToTrigger(&m_commandManager, KeyPressCommandIDs::deleteSelectedClips, true);

    m_toolBar.addButton(&m_selectButton, 1);
    m_toolBar.addButton(&m_lassoSelectButton, 1);
    m_toolBar.addButton(&m_timeRangeSelectButton, 1);
    m_toolBar.addButton(&m_timeStretchButton, 1);
    m_toolBar.addButton(&m_splitClipButton, 1);
    m_toolBar.addButton(&m_deleteClipButton);
    m_toolBar.addButton(&m_reverseClipButton);

    m_toolBar.setButtonGap(4, 30);

    markAndUpdate (m_updateTracks);
    m_editViewState.m_selectionManager.selectOnly (
                te::getAllTracks (m_edit).getLast ());

    markAndUpdate(m_verticalUpdateSongEditor);
    updateHorizontalScrollBar();
    startTimer (static_cast<int>(m_editViewState.m_applicationState.m_autoSaveInterval));
    trimMidiNotesToClipStart();
}

EditComponent::~EditComponent()
{
    m_timeStretchButton.removeListener(this);
    m_splitClipButton.removeListener(this);
    m_timeRangeSelectButton.removeListener(this);
    m_lassoSelectButton.removeListener(this);
    m_selectButton.removeListener(this);
    m_collapseAllBtn.removeListener(this);
    m_expandAllBtn.removeListener(this);
    m_addFolderTrackBtn.removeListener(this);
    m_addMidiTrackBtn.removeListener(this);
    m_addAudioTrackBtn.removeListener(this);
    m_scrollbar_h.removeListener(this);
    m_scrollbar_v.removeListener(this);
    m_edit.state.removeListener (this);
}

void EditComponent::paint (juce::Graphics &g)
{
    g.setColour(m_editViewState.m_applicationState.getMenuBackgroundColour());
    g.fillRect(getEditorHeaderRect());
    g.fillRect(getFooterRect());
    g.setColour(juce::Colour(0xff272727));

    g.fillRect(getTrackListToolsRect());
    g.fillRect(getTrackListRect());
    g.fillRect(getTimeLineRect());
    g.fillRect(getSongEditorRect());
}

void EditComponent::paintOverChildren(juce::Graphics &g)
{
    g.setColour(m_editViewState.m_applicationState.getBorderColour());
    g.drawHorizontalLine (getEditorHeaderRect ().getBottom (), 0, getWidth ());
    g.drawHorizontalLine (getTimeLineRect ().getBottom () - 1, 0, getWidth ());
    g.drawHorizontalLine (getSongEditorRect ().getBottom (), 0, getWidth ());

    g.drawVerticalLine (getTrackListRect ().getRight (),
                        getTimeLineRect ().getY (),
                        getTimeLineRect ().getBottom ());
    auto background = m_editViewState.m_applicationState.getMainFrameColour();

    auto stroke = m_dragOver ? m_editViewState.m_applicationState.getPrimeColour() : m_editViewState.m_applicationState.getBorderColour();
    GUIHelpers::drawFakeRoundCorners(g, getLocalBounds().toFloat(), background, stroke);
}


void EditComponent::resized()
{
    GUIHelpers::log("EditComponent: resized()");
    m_toolBar.setBounds(getToolBarRect());
    m_timeLine.setBounds(getTimeLineRect());
    m_trackListView.setBounds(getTrackListRect());
    m_trackListView.resized();
    auto rect = getTrackListToolsRect().removeFromRight(getTrackListToolsRect().getWidth()/2);
    rect.removeFromRight(10);
    m_trackListToolsMenu.setBounds(rect);
    m_trackListToolsMenu.resized();
    m_trackListControlMenu.setBounds(getTrackListToolsRect().removeFromLeft(getTrackListToolsRect().getWidth()/2));
    m_trackListControlMenu.resized();
    m_songEditor.setBounds(getSongEditorRect());
    m_songEditor.resized();
    m_scrollbar_v.setBounds (getSongEditorRect().removeFromRight(20));
    m_scrollbar_v.setCurrentRange (-m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()), getSongEditorRect().getHeight()/2.0);
    m_footerbar.setBounds(getFooterRect());
    m_playhead.setBounds(getPlayHeadRect());
    m_scrollbar_h.setBounds(getSongEditorRect().removeFromBottom(20));
}
void EditComponent::updateHorizontalScrollBar()
{
    auto x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), m_timeLine.getWidth()).getStart().inBeats();
    auto x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), m_timeLine.getWidth()).getEnd().inBeats();

    m_scrollbar_h.setRangeLimits ({0.0, m_editViewState.getEndScrollBeat ()}, juce::dontSendNotification);
    m_scrollbar_h.setCurrentRange ({x1, x2}, juce::dontSendNotification);
}



void EditComponent::mouseWheelMove(const juce::MouseEvent &event
                                   , const juce::MouseWheelDetails &wheel)
{
    if (event.mods.isShiftDown())
    {
        auto startBeat = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), m_timeLine.getWidth()).getStart().inBeats();
        auto endBeat = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), m_timeLine.getWidth()).getEnd().inBeats();
        auto visibleLength = endBeat - startBeat;

        int viewStartX = 0 -
                #if JUCE_MAC
                static_cast<int>(wheel.deltaX * 300);
                #else
                static_cast<int>(wheel.deltaY * 300);
                #endif

        auto newViewStartBeats = m_editViewState.xToBeats(viewStartX, m_timeLine.getWidth(), startBeat, endBeat);
        m_editViewState.setNewStartAndZoom(m_timeLine.getTimeLineID(), newViewStartBeats);
    }
    else if (event.mods.isCommandDown())
    {
        const float wheelDelta =
                #if JUCE_MAC
                wheel.deltaX * -(m_editViewState.getTimeLineZoomUnit());
                #else
                wheel.deltaY * -(m_editViewState.getTimeLineZoomUnit());
                #endif

        const auto startBeat = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), m_timeLine.getWidth()).getStart().inBeats();
        const auto endBeat = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), m_timeLine.getWidth()).getEnd().inBeats();
        const auto xPos = event.getPosition().getX() - getTrackListRect().getWidth();
        const auto mouseBeat = m_timeLine.xToBeatPos(xPos).inBeats();
        const auto scaleFactor = wheelDelta > 0 ? 1.1 : 0.9;
        const auto newVisibleLengthBeats = juce::jlimit(0.05, 100240.0, (endBeat - startBeat) * scaleFactor);
        const auto newBeatsPerPixel = newVisibleLengthBeats / m_timeLine.getWidth();
        const auto viewCorrect =  (xPos * m_timeLine.getBeatsPerPixel()) - (xPos * newBeatsPerPixel);
        const auto newStartPos = startBeat + viewCorrect;

        m_editViewState.setNewStartAndZoom(m_timeLine.getTimeLineID(), newStartPos, newBeatsPerPixel);
    }
    else
    {
        m_scrollbar_v.setCurrentRangeStart(
                    m_scrollbar_v.getCurrentRangeStart() - wheel.deltaY * 60);
    }
}

void EditComponent::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved
                                   , double newRangeStart)
{
    if (scrollBarThatHasMoved == &m_scrollbar_v)
    {
        m_editViewState.setYScroll(m_timeLine.getTimeLineID(), -newRangeStart);
        m_songEditor.resized();
        m_trackListView.resized();
    }
    else if(scrollBarThatHasMoved == &m_scrollbar_h)
    {
        m_editViewState.setNewStartAndZoom(m_timeLine.getTimeLineID(), newRangeStart);
    }
}

void EditComponent::buttonClicked(juce::Button* button) 
{
    if (button == &m_addAudioTrackBtn)
    {
        auto colour = m_editViewState.m_applicationState.getRandomTrackColour();
        EngineHelpers::addAudioTrack(false, colour, m_editViewState);
    }
    else if (button == &m_addMidiTrackBtn)
    {
        auto colour = m_editViewState.m_applicationState.getRandomTrackColour();
        EngineHelpers::addAudioTrack(true, colour, m_editViewState);
    }
    else if (button == &m_addFolderTrackBtn)
    {
        auto colour = m_editViewState.m_applicationState.getRandomTrackColour();
        EngineHelpers::addFolderTrack(colour, m_editViewState);
    }
    else if (button == &m_collapseAllBtn)
    {
        m_trackListView.collapseTracks(true);
    }
    else if (button == &m_expandAllBtn)
    {
        m_trackListView.collapseTracks(false);
    }
    else if (button == &m_selectButton)
    {
        m_songEditor.setTool(Tool::pointer);
    }
    else if (button == &m_lassoSelectButton)
    {
        m_songEditor.setTool(Tool::lasso);
    }
    else if (button == &m_timeRangeSelectButton)
    {
        m_songEditor.setTool(Tool::range);
    }
    else if (button == &m_splitClipButton)
    {
        m_songEditor.setTool(Tool::knife);
    }
    else if (button == &m_timeStretchButton)
    {
        m_songEditor.setTool(Tool::timestretch);
    }
}
void EditComponent::timerCallback() 
{
    saveTempFile();
}

void EditComponent::saveTempFile()
{
    if (m_editViewState.m_isSavingLocked)
        return;

    if (m_edit.getTransport().isPlaying() || m_edit.getTransport().isRecording())
        return;

    auto temp = m_editViewState.m_edit.getTempDirectory(false);
    auto editFile = Helpers::findRecentEdit(temp);

    //make sure that the edit finds the wave files relative to this edit file
    EngineHelpers::refreshRelativePathsToNewEditFile(m_editViewState, editFile);

    auto currentFile = te::EditFileOperations(m_editViewState.m_edit).getEditFile();
    auto xml = m_edit.state.toXmlString();

    if (m_editViewState.m_needAutoSave)
    {
        editFile.replaceWithText(xml);
        GUIHelpers::log("current edit XML is saved: " + editFile.getFullPathName());
    }
    else
    {
        GUIHelpers::log("Edit is up to date, don't save.");
    }

    //make sure that the edit finds the wave files relative to currentFile again
    EngineHelpers::refreshRelativePathsToNewEditFile(m_editViewState, currentFile);
    m_edit.sendSourceFileUpdate();
    m_editViewState.m_needAutoSave = false;
    markAndUpdate(m_noteOffAll);
}

void EditComponent::valueTreePropertyChanged (
        juce::ValueTree& v, const juce::Identifier& i)
{
    if (i == te::IDs::loopPoint1
        || i == te::IDs::loopPoint2
        || i == te::IDs::looping)
        markAndUpdate(m_updateZoom);

    if (i == te::IDs::height
     || i == IDs::isTrackMinimized
     || i == IDs::viewY)
    {
        markAndUpdate(m_verticalUpdateSongEditor);
    }

    if (i == te::IDs::lastSignificantChange)
    {
        m_editViewState.m_needAutoSave = true;
    }
    if (v.hasType(m_timeLine.getTimeLineID()))
    {
        markAndUpdate (m_updateZoom);
    }
    if (v.hasType (IDs::EDITVIEWSTATE))
    {
        if (
             i == IDs::isPianoRollVisible
            || i == IDs::pianorollHeight
            || i == IDs::showHeaders
            || i == IDs::showFooters)
            markAndUpdate (m_updateZoom);
        else if (i == IDs::viewY)
            resized();
        else if (i == IDs::drawWaveforms)
            repaint();
    }
}

void EditComponent::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& c)
{
    if (te::MidiClip::isClipState (c))
    {
        // markAndUpdate (m_updateZoom);
        // markAndUpdate(m_verticalUpdateSongEditor);
    }
    if (te::TrackList::isTrack (c))
    {
        markAndUpdate (m_updateTracks);
    }
    if (c.hasType(te::IDs::AUTOMATIONCURVE))
    {
        GUIHelpers::log(c.toXmlString());
        markAndUpdate (m_updateTracks);
        markAndUpdate(m_verticalUpdateSongEditor);
    }
}

void EditComponent::valueTreeChildRemoved (
        juce::ValueTree&, juce::ValueTree& c, int)
{
    if (te::MidiClip::isClipState (c))
    {
        markAndUpdate (m_updateZoom);
    }
    if (te::TrackList::isTrack (c))
    {
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
    if (c.hasType(te::IDs::AUTOMATIONCURVE))
    {
        GUIHelpers::log(c.toXmlString());
        markAndUpdate (m_updateTracks);
        markAndUpdate(m_verticalUpdateSongEditor);
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
    if (compareAndReset(m_noteOffAll))
    {
        sendAllNotedOff();
    }
    if (compareAndReset (m_updateTracks))
    {
        buildTracks();
        m_songEditor.repaint();
    }
    if (compareAndReset (m_updateZoom))
    {
        refreshSnapTypeDesc();

        m_timeLine.repaint ();
        m_songEditor.repaint();

        updateHorizontalScrollBar();
    }
    if (compareAndReset(m_verticalUpdateSongEditor))
    {
        m_editViewState.m_trackHeightManager->regenerateTrackHeightsFromStates(tracktion::getAllTracks(m_edit));
        m_songEditor.resized();
        m_trackListView.resized();
        updateVerticalScrollbar();
    }
}


void EditComponent::refreshSnapTypeDesc()
{
    auto x1 = m_timeLine.getCurrentBeatRange().getStart().inBeats();
    auto x2 = m_timeLine.getCurrentBeatRange().getEnd().inBeats();
    m_footerbar.m_snapTypeDesc =
            m_editViewState.getSnapTypeDescription (
                m_editViewState.getBestSnapType (x1, x2, m_timeLine.getWidth()).level);
    m_footerbar.repaint ();
}

void EditComponent::buildTracks()
{
    m_trackListView.clear();
    m_songEditor.clear();

    for (auto t : tracktion_engine::getAllTracks (m_edit))
    {
        if (m_editViewState.m_trackHeightManager->isTrackShowable(t))
        {
            auto th = std::make_unique<TrackHeaderComponent> (m_editViewState, t);
            auto tl = std::make_unique<TrackLaneComponent>(m_editViewState, t, m_timeLine.getTimeLineID());
            m_trackListView.addHeaderView(std::move(th));
            m_songEditor.addTrackLaneComponent(std::move(tl));
        }
    }

    m_trackListView.updateViews();
    m_songEditor.updateViews();
    m_playhead.toFront (false);
    resized();
}

void EditComponent::getAllCommands (juce::Array<juce::CommandID>& commands) 
{
    juce::Array<juce::CommandID> ids {

            KeyPressCommandIDs::deleteSelectedClips,
            KeyPressCommandIDs::duplicateSelectedClips,
            KeyPressCommandIDs::selectAllClips,
            KeyPressCommandIDs::renderSelectedTimeRangeToNewTrack,
            KeyPressCommandIDs::transposeClipUp,
            KeyPressCommandIDs::transposeClipDown,
            KeyPressCommandIDs::reverseClip,
        };

    commands.addArray(ids);
}
void EditComponent::getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result) 
{
    switch (commandID)
    { 
        case KeyPressCommandIDs::duplicateSelectedClips :
            result.setInfo("Duplicate selected clips", "Duplicate selected clips", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("d").getKeyCode() , juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::deleteSelectedClips :
            result.setInfo("Delete selected clips", "Delete selected clips", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::backspaceKey , 0);
            result.addDefaultKeypress(juce::KeyPress::deleteKey, 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("x").getKeyCode(), juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::selectAllClips :
            result.setInfo("select all Clips","select all Clips", "Song Editor", 0);

            result.addDefaultKeypress(juce::KeyPress::createFromDescription("a").getKeyCode() , juce::ModifierKeys::commandModifier);
            break;

        case KeyPressCommandIDs::renderSelectedTimeRangeToNewTrack :
            result.setInfo("render time range to new track","render time range on new track", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("r").getKeyCode(), juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::transposeClipUp :
            result.setInfo("transpose up", "transpose selected clips up 1 key", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::transposeClipDown :
            result.setInfo("transpose Down", "transpose selected clips Down 1 key", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::commandModifier);
            break;  
        case KeyPressCommandIDs::reverseClip :
            result.setInfo("Reverse wave of clip","Reverse wave of clip", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("b").getKeyCode(), juce::ModifierKeys::commandModifier);
            break;
        default:
            break;
        }

}
bool EditComponent::perform (const juce::ApplicationCommandTarget::InvocationInfo& info) 
{
    GUIHelpers::log("EditComponent perform commandID: ", info.commandID);

    switch (info.commandID)
    { 
        //send NoteOn
        case KeyPressCommandIDs::deleteSelectedClips:
        {
            GUIHelpers::log("deleteSelectedClips Outer");
            if (m_songEditor.getTracksWithSelectedTimeRange().size() > 0)
            {
                GUIHelpers::log("deleteSelectedTimeRange");
                m_songEditor.deleteSelectedTimeRange();
            }
            else 
            {
                GUIHelpers::log("deleteSelectedClips");
                EngineHelpers::deleteSelectedClips (m_editViewState);
            }
            break;
        }
        case KeyPressCommandIDs::duplicateSelectedClips:
            m_songEditor.duplicateSelectedClipsOrTimeRange();
            break;
        case KeyPressCommandIDs::selectAllClips:
            EngineHelpers::selectAllClips(m_editViewState.m_selectionManager, m_editViewState.m_edit);
            break;
        case KeyPressCommandIDs::renderSelectedTimeRangeToNewTrack:
            m_songEditor.renderSelectedTimeRangeToNewTrack();
            break;
        case KeyPressCommandIDs::transposeClipUp:
            GUIHelpers::log("perform: transposeClipUp");
            m_songEditor.transposeSelectedClips( + 1.f);
            break;
        case KeyPressCommandIDs::transposeClipDown:
            m_songEditor.transposeSelectedClips( - 1.f);
            break;
        case KeyPressCommandIDs::reverseClip:
            GUIHelpers::log("reverse!!!!");
            m_songEditor.reverseSelectedClips();
            break;
        default:
            return false;
    }
    return true;
}


bool EditComponent::isInterestedInDragSource (const SourceDetails& dragSourceDetails) 
{
    if (auto b = dynamic_cast<BrowserListBox*>(dragSourceDetails.sourceComponent.get()))
    {
        if (b->getSelectedFile().getFileName().endsWith(".tracktionedit"))
            return true;
    }
    return false;
}


void EditComponent::itemDragEnter (const SourceDetails& dragSourceDetails) 
{
    m_dragOver = true;
    repaint();
}

void EditComponent::itemDragMove (const SourceDetails& dragSourceDetails) 
{
    m_dragOver = false;
    auto f = juce::File();
    if (auto b = dynamic_cast<BrowserListBox*>(dragSourceDetails.sourceComponent.get()))
        f = b->getSelectedFile();
    if (f.existsAsFile())
        m_dragOver = true;

    repaint();

}
void EditComponent::itemDragExit (const SourceDetails& dragSourceDetails) 
{
    m_dragOver = false;
    repaint();

}

void EditComponent::itemDropped (const SourceDetails& dragSourceDetails) 
{
    auto f = juce::File();
    if (auto b = dynamic_cast<BrowserListBox*>(dragSourceDetails.sourceComponent.get()))
        f = b->getSelectedFile();

    if (f.existsAsFile())
        if (auto mc = dynamic_cast<MainComponent*>(getParentComponent()->getParentComponent()))
            mc->setupEdit(f);

    repaint();
}
juce::Rectangle<int> EditComponent::getToolBarRect()
{
    auto rect = getEditorHeaderRect();
    rect.reduce(rect.getWidth()/3, 0);
    return rect;
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

    for (auto track : m_editViewState.m_trackHeightManager->getShowedTracks(m_edit))
    {
        auto trackID = m_editViewState.m_trackHeightManager->getTrackFromID(m_editViewState.m_edit, track);
        h += m_editViewState.m_trackHeightManager->getTrackHeight(trackID, true);
    }

    return h;
}
void EditComponent::loopAroundSelection()
{
    auto& transport = m_edit.getTransport();
    if (getSelectedClipRange().getLength().inSeconds() > 0)
        transport.setLoopRange (getSelectedClipRange());
}
tracktion::core::TimeRange EditComponent::getSelectedClipRange()
{
    if (m_editViewState.m_selectionManager.getItemsOfType<te::Clip>().size() == 0)
        return {EngineHelpers::getTimePos(0.0),EngineHelpers::getTimePos(0.0)};

    auto start = m_edit.getLength().inSeconds();
    auto end = 0.0;

    for (auto c: m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
    {
        start = c->getPosition().getStart().inSeconds() < start
            ? c->getPosition().getStart().inSeconds()
            : start;

        end = c->getPosition().getEnd().inSeconds() > end
            ? c->getPosition().getEnd().inSeconds()
            : end;
    }

    return {EngineHelpers::getTimePos(start), EngineHelpers::getTimePos(end)};
}
void EditComponent::sendAllNotedOff()
{
    for (auto track : tracktion::getAudioTracks(m_edit))
        for (int i = 1; i <= 16; ++i)
            track->injectLiveMidiMessage (juce::MidiMessage::allNotesOff (i), {});

    GUIHelpers::log("EditComponent: ", "All notes off!");
}
