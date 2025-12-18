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


#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ApplicationViewState.h"
#include "TrackHeightManager.h"
#include "Utilities.h"

namespace te = tracktion_engine;

struct SelectableAutomationPoint  : public te::Selectable
{
    SelectableAutomationPoint (int i, te::AutomationCurve& c)  : index (i), m_curve (c) {}
    ~SelectableAutomationPoint() override { notifyListenersOfDeletion(); }

    juce::String getSelectableDescription() override { return juce::String("AutomationPoint"); }

    int index = 0;
    te::AutomationCurve& m_curve;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SelectableAutomationPoint)
};

// sheetcheat for snapTypes
//SnapTypeNumber 0 : 1 tick
//SnapTypeNumber 1 : 2 ticks
//SnapTypeNumber 2 : 5 ticks
//SnapTypeNumber 3 : 1/64 beat
//SnapTypeNumber 4 : 1/32 beat      0.03125
//SnapTypeNumber 5 : 1/16 beat      0.0625
//SnapTypeNumber 6 : 1/8 beat       0.125
//SnapTypeNumber 7 : 1/4 beat       0.25
//SnapTypeNumber 8 : 1/2 beat       0.5
//SnapTypeNumber 9 : Beat           1
//SnapTypeNumber 10 : Bar           4
//SnapTypeNumber 11 : 2 bars        8
//SnapTypeNumber 12 : 4 bars        16
//SnapTypeNumber 13 : 8 bars        32
//SnapTypeNumber 14 : 16 bars       64
//SnapTypeNumber 15 : 64 bars
//SnapTypeNumber 16 : 128 bars
//SnapTypeNumber 17 : 256 bars
//SnapTypeNumber 18 : 1024 bars


namespace IDs
{
    #define DECLARE_ID(name)  const juce::Identifier name (#name);
    DECLARE_ID (EDITVIEWSTATE)
    DECLARE_ID (showGlobalTrack)
    DECLARE_ID (showMarkerTrack)
    DECLARE_ID (showChordTrack)
    DECLARE_ID (showMidiDevices)
    DECLARE_ID (showWaveDevices)
    DECLARE_ID (viewData)
    DECLARE_ID (viewX)
    DECLARE_ID (beatsPerPixel)
    DECLARE_ID (viewY)
    DECLARE_ID (viewYScale)
    DECLARE_ID (pianorollHeight)
    DECLARE_ID (snapType)
    DECLARE_ID (drawWaveforms)
    DECLARE_ID (showHeaders)
    DECLARE_ID (showFooters)
    DECLARE_ID (showArranger)
    DECLARE_ID (showMaster)
    DECLARE_ID (trackMinimized)
    DECLARE_ID (headerHeight)
    DECLARE_ID (headerWidth)
    DECLARE_ID (isMidiTrack)
    DECLARE_ID (isAutoArmed)
    DECLARE_ID (lowerRangeView)
    DECLARE_ID (timeLineHeight)
    DECLARE_ID (lastNoteLenght)
    DECLARE_ID (name)
    DECLARE_ID (footerBarHeight)
    DECLARE_ID (folderTrackHeight)
    DECLARE_ID (isTrackMinimized)
    DECLARE_ID (automationFollowsClip)
    DECLARE_ID (playHeadStartTime)
    DECLARE_ID (followsPlayhead)
    DECLARE_ID (timeLineZoomUnit)
    DECLARE_ID (zoomMode)
    DECLARE_ID (velocityEditorHeight)
    DECLARE_ID (isHovered)
    DECLARE_ID (lastVelocity)
    DECLARE_ID (pianoRollKeyboardWidth)
    DECLARE_ID (selected)
    DECLARE_ID (selectedRangeStart)
    DECLARE_ID (selectedRangeEnd)
    DECLARE_ID (clipHeaderHeight)
    DECLARE_ID (tmpTrack)
    DECLARE_ID (syncAutomation)
    DECLARE_ID (needAutoSave)
    DECLARE_ID (snapToGrid)
    DECLARE_ID (showLowerRange)
    DECLARE_ID (editNoteOutsideOfClipRange)
    DECLARE_ID (pluginPresetManagerUIStates)
    DECLARE_ID (trackRackViewState)

    #undef DECLARE_ID
}

//==============================================================================
struct ViewData {
    juce::CachedValue<double> viewX;
    juce::CachedValue<double> beatsPerPixel;
    juce::CachedValue<double> viewY;
    juce::CachedValue<double> viewYScale;

    ViewData(juce::ValueTree& state)
        : viewX(state, IDs::viewX, nullptr, 0.0)
        , beatsPerPixel(state, IDs::beatsPerPixel, nullptr, 0.1)
        , viewY(state, IDs::viewY, nullptr, 0.0)
        , viewYScale(state, IDs::viewYScale, nullptr, 20.0)
    {
    }
};

enum class LowerRangeView { none, midiEditor, pluginRack, mixer };

class EditViewState
{
public:
    EditViewState (te::Edit& e, te::SelectionManager& s, ApplicationViewState& avs);
    ~EditViewState();

    void setLowerRangeView (LowerRangeView newView)
    {
        m_lowerRangeView = static_cast<int>(newView);
    }

    LowerRangeView getLowerRangeView()
    {
        return static_cast<LowerRangeView>((int)m_lowerRangeView);
    }

    juce::ValueTree getPresetManagerUIStateForPlugin(const te::Plugin& plugin);
    juce::ValueTree getTrackRackViewState(te::EditItemID trackID);

    double getViewYScale(juce::String timeLineID)
    {
        if (auto* myViewData = componentViewData[timeLineID])
        {
            return myViewData->viewYScale;
        }
        return -1;
    }

    double getViewYScroll(juce::String timeLineID)
    {
        if (auto* myViewData = componentViewData[timeLineID])
        {
            return myViewData->viewY;
        }
        return -1;
    }

    bool setViewYScale(juce::String timeLineID, double newScale)
    {
        if (auto* myViewData = componentViewData[timeLineID])
        {
            myViewData->viewYScale = newScale;
            return true;
        }
        return false; 
    }

    bool setYScroll(juce::String timeLineID, double newScroll)
    {
        if (auto* myViewData = componentViewData[timeLineID])
        {
            myViewData->viewY = newScroll;
            return true;
        }
        return false; 
    }

    float getTimeLineZoomUnit ()
    {
        if(m_zoomMode == "B")
            return m_timeLineZoomUnit;
        return m_timeLineZoomUnit * (-1);
    }

    juce::String getZoomMode ()
    {
        return m_zoomMode;
    }

    [[nodiscard]] float beatsToX(double beats, int width, double x1beats, double x2beats) const;
    [[nodiscard]] double xToBeats(float x, int width, double x1beats, double x2beats) const;
    [[nodiscard]] float timeToX(double time, int width, double x1beats, double x2beats) const;
    [[nodiscard]] double xToTime(float x, int width, double x1beats, double x2beats) const;

    [[nodiscard]] float beatsToX(double beats, const juce::String& timeLineID, int width);
    [[nodiscard]] double xToBeats(int x, const juce::String& timeLineID, int width);
    [[nodiscard]] float timeToX(double time, const juce::String& timeLineID, int width);
    [[nodiscard]] double xToTime(int x, const juce::String& timeLineID, int width);

    [[nodiscard]] double beatToTime (double b) const;
    [[nodiscard]] double timeToBeat (double t) const;

    void setNewStartAndZoom(juce::String timeLineID, double startBeat, double beatsPerPixel=-1);
    void setNewBeatRange(juce::String timeLineID, tracktion::BeatRange beatRange, float width);
    void setNewTimeRange(juce::String timeLineID, tracktion::TimeRange timeRange, float width);

    tracktion::BeatRange getVisibleBeatRange(juce::String id, int width);
    tracktion::TimeRange getVisibleTimeRange(juce::String id, int width);

    [[nodiscard]] double getSnapedTime (
            double t
          , te::TimecodeSnapType snapType
          , bool downwards = false) const;

    [[nodiscard]] te::TimecodeSnapType getBestSnapType(double beat1, double beat2, int width) const;

    [[nodiscard]] juce::String getSnapTypeDescription(int idx) const;

    [[nodiscard]] double getEndScrollBeat() const;

    void followsPlayhead(bool shouldFollow);

    void toggleFollowPlayhead();
    [[nodiscard]] bool viewFollowsPos() const;

    SimpleThumbnail* getOrCreateThumbnail (te::WaveAudioClip::Ptr wac);
    void clearThumbnails();
    void removeThumbnail(te::EditItemID id);


    std::unique_ptr<TrackHeightManager>    m_trackHeightManager;
    std::unique_ptr<ThumbNailManager>      m_thumbNailManager;
    te::Edit& m_edit;
    te::SelectionManager& m_selectionManager;

    juce::CachedValue<bool> m_showGlobalTrack
                          , m_showMarkerTrack
                          , m_showChordTrack
                          , m_showArrangerTrack
                          , m_showMasterTrack
                          , m_drawWaveforms
                          , m_showHeaders
                          , m_showFooters
                          , m_showMidiDevices
                          , m_showWaveDevices
                          , m_isAutoArmed
                          , m_automationFollowsClip
                          , m_followPlayhead
                          , m_syncAutomation;
    juce::CachedValue<int> m_lowerRangeView;
     juce::CachedValue<double>  m_lastNoteLength
                            , m_playHeadStartTime
                            , m_timeLineZoomUnit;
    juce::CachedValue<int> m_midiEditorHeight
                            , m_velocityEditorHeight
                            , m_clipHeaderHeight;
    juce::CachedValue<int>  m_snapType;
    juce::CachedValue<bool> m_snapToGrid,
                            m_editNotesOutsideClipRange;
    juce::CachedValue<int> m_trackHeightMinimized
                         , m_trackDefaultHeight
                         , m_trackHeaderWidth
                         , m_timeLineHeight
                         , m_folderTrackHeight
                         , m_footerBarHeight
                         , m_lastVelocity
                         , m_keyboardWidth;

    juce::CachedValue<juce::String> m_editName
                                    , m_zoomMode;

    juce::ValueTree m_state;
    juce::ValueTree m_viewDataTree;
    juce::ValueTree m_pluginPresetManagerUIStates;
    juce::ValueTree m_trackRackViewState;
    std::map<juce::String, struct ViewData*> componentViewData;
    bool m_isSavingLocked {false}, m_needAutoSave {false};
    ApplicationViewState& m_applicationState;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditViewState)
};
