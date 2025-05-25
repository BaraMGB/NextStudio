
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
    DECLARE_ID (isPianoRollVisible)
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

class EditViewState
{
public:
    EditViewState (te::Edit& e, te::SelectionManager& s, ApplicationViewState& avs)
        : m_edit (e), m_selectionManager (s), m_applicationState(avs)
    {
        m_trackHeightManager = std::make_unique<TrackHeightManager>(tracktion::getAllTracks(e));
        m_thumbNailManager = std::make_unique<ThumbNailManager>(m_edit.engine);
        m_state = m_edit.state.getOrCreateChildWithName (IDs::EDITVIEWSTATE, nullptr);
        m_viewDataTree = m_edit.state.getOrCreateChildWithName(IDs::viewData, nullptr);

        auto um = &m_edit.getUndoManager();

        m_showGlobalTrack.referTo (m_state, IDs::showGlobalTrack, um, false);
        m_showMarkerTrack.referTo (m_state, IDs::showMarkerTrack, um, false);
        m_showChordTrack.referTo (m_state, IDs::showChordTrack, um, false);
        m_showArrangerTrack.referTo (m_state, IDs::showArranger, um, false);
        m_showMasterTrack.referTo(m_state, IDs::showMaster, um, false);
        m_drawWaveforms.referTo (m_state, IDs::drawWaveforms, um,/* false);*/true);
        m_showHeaders.referTo (m_state, IDs::showHeaders, um,false);// true);
        m_showFooters.referTo (m_state, IDs::showFooters, um, false);
        m_showMidiDevices.referTo (m_state, IDs::showMidiDevices, um, false);
        m_showWaveDevices.referTo (m_state, IDs::showWaveDevices, um, true);
        m_automationFollowsClip.referTo (m_state, IDs::automationFollowsClip, um, true);

        m_trackHeightMinimized.referTo (m_state, IDs::trackMinimized, um, 30);
        m_isAutoArmed.referTo (m_state, IDs::isAutoArmed, um, true);
        m_trackDefaultHeight.referTo(m_state, IDs::headerHeight, um, 50);
        m_trackHeaderWidth.referTo(m_state, IDs::headerWidth, um, 290);
        m_folderTrackHeight.referTo(m_state, IDs::folderTrackHeight, um, 30);
        m_footerBarHeight.referTo (m_state, IDs::footerBarHeight, um, 20);
        m_isPianoRollVisible.referTo (m_state, IDs::isPianoRollVisible, um, false);
        m_midiEditorHeight.referTo (m_state, IDs::pianorollHeight, um, 400);
        m_lastNoteLength.referTo (m_state, IDs::lastNoteLenght, um, 0);
        m_snapType.referTo(m_state, IDs::snapType, um, 9);
        m_playHeadStartTime.referTo (m_state, IDs::playHeadStartTime, um, 0.0);
        m_followPlayhead.referTo (m_state, IDs::followsPlayhead, um, true);
        m_timeLineHeight.referTo(m_state, IDs::timeLineHeight, um, 50);
        m_editName.referTo(m_state, IDs::name, um, "unknown");
        m_timeLineZoomUnit.referTo(m_state, IDs::timeLineZoomUnit, um, 50);
        m_zoomMode.referTo(m_state, IDs::zoomMode, um, "B");
        m_velocityEditorHeight.referTo(m_state, IDs::velocityEditorHeight, um, 100);
        m_lastVelocity.referTo(m_state, IDs::lastVelocity, um, 100);
        m_keyboardWidth.referTo(m_state, IDs::pianoRollKeyboardWidth, um, 120);
        m_clipHeaderHeight.referTo(m_state, IDs::clipHeaderHeight, um, 20);
        m_syncAutomation.referTo(m_state, IDs::syncAutomation, um, true);
        m_snapToGrid.referTo(m_state, IDs::snapToGrid, um, true);
        m_editNotesOutsideClipRange.referTo(m_state, IDs::editNoteOutsideOfClipRange, um, false);
    }

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

    [[nodiscard]] float beatsToX(double beats, int width, double x1beats, double x2beats) const
    {
        return static_cast<float>(((beats - x1beats) * width) / (x2beats - x1beats));
    }

    [[nodiscard]] double xToBeats(float x, int width, double x1beats, double x2beats) const
    {
        double beats = (static_cast<double>(x) / width) * (x2beats - x1beats) + x1beats;
        return beats;
    }

    [[nodiscard]] float timeToX(double time, int width, double x1beats, double x2beats) const
    {
        double beats = timeToBeat(time);
        return static_cast<float>(((beats - x1beats) * width) / (x2beats - x1beats));
    } 
    [[nodiscard]] double xToTime(float x, int width, double x1beats, double x2beats) const
    {
    double beats = (static_cast<double>(x) / width) * (x2beats - x1beats) + x1beats;
    return beatToTime(beats);
    }

    [[nodiscard]] double beatToTime (double b) const
    {
        auto bp = tracktion::core::BeatPosition::fromBeats(b);
        auto& ts = m_edit.tempoSequence;
        return ts.toTime(bp).inSeconds();
    }

    [[nodiscard]] double timeToBeat (double t) const
    {
        auto tp = tracktion::core::TimePosition::fromSeconds(t);
        auto& ts = m_edit.tempoSequence;
        return ts.toBeats(tp).inBeats();
    }

    void setNewStartAndZoom(juce::String timeLineID, double startBeat, double beatsPerPixel=-1)
    {

        startBeat = juce::jmax(0.0, startBeat);

        if (auto* myViewData = componentViewData[timeLineID])
        {
            if (beatsPerPixel != -1)
                myViewData->beatsPerPixel = beatsPerPixel;
            myViewData->viewX = startBeat;
        }
    }


    void setNewBeatRange(juce::String timeLineID, tracktion::BeatRange beatRange, float width)
    {
        if (auto* myViewData = componentViewData[timeLineID])
        {
            auto startBeat = beatRange.getStart().inBeats();
            auto endBeat = beatRange.getEnd().inBeats();
            auto beatsPerPixel = (endBeat - startBeat) / width;

            if (startBeat < 0)
            {
                startBeat = 0;
                endBeat = startBeat + (beatsPerPixel * width);
            }

            myViewData->viewX = startBeat;
            myViewData->beatsPerPixel = beatsPerPixel;
        }
    }

    void setNewTimeRange(juce::String timeLineID, tracktion::TimeRange timeRange, float width)
    {
        if (auto* myViewData = componentViewData[timeLineID])
        {
            auto startBeat = timeToBeat(timeRange.getStart().inSeconds());
            auto endBeat = timeToBeat(timeRange.getEnd().inSeconds());
            auto beatsPerPixel = (endBeat - startBeat) / width;

            if (startBeat < 0)
            {
                startBeat = 0;
                endBeat = startBeat + (beatsPerPixel * width);
            }

            myViewData->viewX = startBeat;
            myViewData->beatsPerPixel = beatsPerPixel;
        }
    }

    tracktion::BeatRange getVisibleBeatRange(juce::String id, int width)
    {
        if (auto* myViewData = componentViewData[id])
        {
            auto startBeat = myViewData->viewX.get();
            auto endBeat = startBeat + (myViewData->beatsPerPixel * width);

            return {tracktion::BeatPosition::fromBeats(startBeat)
                    , tracktion::BeatPosition::fromBeats(endBeat)};
        }
        return tracktion::BeatRange();
    }

    tracktion::TimeRange getVisibleTimeRange(juce::String id, int width)
    {
        if (auto* myViewData = componentViewData[id])
        {
            auto startBeat = myViewData->viewX.get();
            auto endBeat = startBeat + (myViewData->beatsPerPixel * width);

            auto t1 = beatToTime(startBeat);
            auto t2 = beatToTime(endBeat);

            return {tracktion::TimePosition::fromSeconds(t1)
                    , tracktion::TimePosition::fromSeconds(t2)};
        }
        return tracktion::TimeRange();
    }

    [[nodiscard]] double getSnapedTime (
            double t
          , te::TimecodeSnapType snapType
          , bool downwards = false) const
    {
        auto & temposequ = m_edit.tempoSequence;

        auto tp = tracktion::core::TimePosition::fromSeconds(t);
        return downwards
                ? snapType.roundTimeDown (tp, temposequ).inSeconds()
                : snapType.roundTimeNearest (tp, temposequ).inSeconds();
    }

    [[nodiscard]] te::TimecodeSnapType getBestSnapType(double beat1, double beat2, int width) const
    {
        double x1time = beatToTime (beat1);
        double x2time = beatToTime (beat2);

        auto td = tracktion::core::TimeDuration::fromSeconds(x2time - x1time);

        auto pos = m_edit.getTransport ().getPosition();
        te::TimecodeSnapType snaptype = m_edit.getTimecodeFormat()
                .getBestSnapType (
                    m_edit.tempoSequence.getTempoAt (pos)
                    , td / width
                    , false);
        return snaptype;
    }

    [[nodiscard]] juce::String getSnapTypeDescription(int idx) const
    {
        auto tp = m_edit.getTransport ().getPosition (); 
        tracktion_engine::TempoSetting &tempo = m_edit.tempoSequence.getTempoAt (tp);
        return m_edit.getTimecodeFormat ().getSnapType (idx).getDescription (tempo, false);
    }

    [[nodiscard]] double getEndScrollBeat() const
    {
        return timeToBeat ( m_edit.getLength ().inSeconds()) + (480);
    }

    void followsPlayhead(bool shouldFollow)
    {
        m_followPlayhead = shouldFollow;
    }

    void toggleFollowPlayhead()
    {
        m_followPlayhead = !m_followPlayhead;
    }
    [[nodiscard]] bool viewFollowsPos() const {return m_followPlayhead;}

    SimpleThumbnail* getOrCreateThumbnail (te::WaveAudioClip::Ptr wac)
    {
        return m_thumbNailManager->getOrCreateThumbnail(wac);
    }
    void clearThumbnails()
    {
        m_thumbNailManager->clearThumbnails();
    }
    void removeThumbnail(te::EditItemID id)
    {
        m_thumbNailManager->removeThumbnail(id);
    }


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
                          , m_isPianoRollVisible
                          , m_isAutoArmed
                          , m_automationFollowsClip
                          , m_followPlayhead
                          , m_syncAutomation;
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
    std::map<juce::String, struct ViewData*> componentViewData;
    bool m_isSavingLocked {false}, m_needAutoSave {false};
    ApplicationViewState& m_applicationState;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditViewState)
};
