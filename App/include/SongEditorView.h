
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

#include "../JuceLibraryCode/JuceHeader.h"
#include "AutomationLaneComponent.h"
#include "RecordingClipComponent.h"
#include "TimeLineComponent.h"
#include "ClipPropertyEdit.h"
#include "TrackLaneComponent.h"
#include "LassoSelectionTool.h"
#include "MenuBar.h"
#include "DragState.h"
#include "EditViewState.h"
#include "Utilities.h"

class SongEditorView
    : public juce::Component
    , public juce::ChangeListener
    , public juce::DragAndDropTarget
{
public:
    /** Overlay component to handle TimeRange interaction exclusively */
    class TimeRangeOverlayComponent : public juce::Component
    {
    public:
        TimeRangeOverlayComponent(SongEditorView &owner);
        void paint(juce::Graphics &g) override;
        bool hitTest(int x, int y) override;
        void mouseMove(const juce::MouseEvent &e) override;
        void mouseExit(const juce::MouseEvent &e) override;
        void mouseDown(const juce::MouseEvent &e) override;
        void mouseDrag(const juce::MouseEvent &e) override;
        void mouseUp(const juce::MouseEvent &e) override;

    private:
        SongEditorView &m_owner;
        bool m_hoveredHandleLeft{false};
        bool m_hoveredHandleRight{false};
    };

    SongEditorView(EditViewState &evs, MenuBar &toolBar, TimeLineComponent &timeLine);
    ~SongEditorView() override;

    void paintOverChildren(juce::Graphics &g) override;
    void resized() override;

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
    void itemDragEnter(const SourceDetails &dragSourceDetails) override;
    void itemDragMove(const SourceDetails &dragSourceDetails) override;
    void itemDragExit(const SourceDetails &dragSourceDetails) override;
    void itemDropped(const SourceDetails &dragSourceDetails) override;
    bool shouldDrawDragImageWhenOver() override { return false; };

    void startLasso(const juce::MouseEvent &e, bool fromAutomation, bool selectRange);
    void updateLasso(const juce::MouseEvent &e);
    void stopLasso();
    void clearSelectedTimeRange();

    int getYForTrack(te::Track *track);
    int getVerticalOffset(te::Track::Ptr sourceTrack, const juce::Point<int> &dropPos);
    double xToSnapedBeat(int x, bool downwards = false);
    double getClipInsertLength() const { return m_timeLine.getClipInsertLength(); }
    tracktion::TimePosition snapTime(tracktion::TimePosition time, bool downwards = false)
    {
        return m_timeLine.snapTime(time, downwards);
    }

    void updateDragGhost(te::Clip::Ptr clip, tracktion::TimeDuration delta, int verticalOffset);
    void setClipPropertyPreview(const juce::Array<ClipPropertyEdit> &);

    void updateTrackHeights(EditViewState &evs);

    void setTool(Tool tool);
    Tool getToolMode() const { return m_toolMode; }

    void duplicateSelectedClipsOrTimeRange();
    void deleteSelectedTimeRange();
    void renderSelectionToNewTrack();
    void renderSelectedTimeRangeToNewTrack();
    void renderSelectedClipsToNewTrack();
    void transposeSelectedClips(float pitchChange);
    void reverseSelectedClips();
    juce::Array<te::Track *> getTracksWithSelectedTimeRange();
    bool hasSelectedTimeRange() const;

    juce::Rectangle<float> getAutomationRect(te::AutomatableParameter::Ptr ap);

    /** Hit-test for TimeRange selection. Returns true if the given position (in SongEditorView coordinates)
        lies within a selected time range on the given track. Also sets outLeftEdge/outRightEdge if near edges. */
    bool hitTestTimeRange(int x, te::Track *track, bool &outLeftEdge, bool &outRightEdge) const;

    /** Hit-test for TimeRange selection on automation lanes. */
    bool hitTestTimeRange(int x, te::AutomatableParameter *param, bool &outLeftEdge, bool &outRightEdge) const;

    /** TimeRange drag delegation - called by child components to manage TimeRange dragging */
    void startTimeRangeDrag();
    /** For move: delta is the time offset. For resize: newEdgeTime is the absolute time for the new edge position. */
    void updateTimeRangeDragMove(tracktion::TimeDuration delta);
    void updateTimeRangeDragResizeLeft(tracktion::TimePosition newEdgeTime, bool snap = true);
    void updateTimeRangeDragResizeRight(tracktion::TimePosition newEdgeTime, bool snap = true);
    void finishTimeRangeDrag(bool copy);
    void cancelTimeRangeDrag();

    void addTrackLaneComponent(std::unique_ptr<TrackLaneComponent> tlc)
    {
        m_trackLanes.add(std::move(tlc));
        resized();
    }

    void updateViews()
    {
        removeAllChildren();

        for (auto v : m_trackLanes)
            addAndMakeVisible(v);

        addChildComponent(m_lassoComponent);
        addAndMakeVisible(m_timeRangeOverlay);
        m_lassoComponent.setAlwaysOnTop(true);
        m_timeRangeOverlay.setAlwaysOnTop(true);

        resized();
    }

    void clear()
    {
        m_trackLanes.clear(true);
        resized();
    }

private:
    tracktion::TimeRange getSelectedTimeRange();
    AutomationLaneComponent *getAutomationLane(tracktion::engine::AutomatableParameter::Ptr ap)
    {
        if (ap == nullptr)
            return nullptr;

        for (auto *trackLane : m_trackLanes)
            if (auto *automationLane = trackLane->getAutomationLane(ap))
                return automationLane;

        return nullptr;
    }

    // converting
    float timeToX(tracktion::TimePosition time);
    float beatToX(tracktion::BeatPosition beat);
    tracktion::BeatPosition xToBeatPosition(int x);
    tracktion::TimePosition xtoTime(int x);
    float timeDurationToPixel(tracktion::TimeDuration duration);
    tracktion::TimeDuration distanceToTime(int distance);

    tracktion::BeatPosition getSnapedBeat(tracktion::BeatPosition beat, bool downwards = false);
    tracktion::TimePosition getSnappedTime(tracktion::TimePosition time, bool downwards = false);

    te::Track::Ptr getTrackAt(int y);

    // LassoSelectionTool
    void updateClipSelection(bool add);
    void updateClipCache();
    void updateAutomationSelection(bool add);
    void updateRangeSelection();
    void setSelectedTimeRange(tracktion::TimeRange tr, bool snapDownAtStart, bool snapDownAtEnd);
    void selectClipsInLasso(const tracktion_engine::Track *track);

    bool moveSelectedTimeRanges(tracktion::TimeDuration td, bool copy);

    void moveSelectedClips(bool copy, double delta, int verticalOffset);
    void addWaveFileToTrack(te::AudioFile audioFile, double dropTime, te::AudioTrack::Ptr track) const;
    void resizeSelectedClips(bool snap, bool fromLeftEdge = false);
    tracktion_engine::MidiClip::Ptr createNewMidiClip(double beatPos, te::Track::Ptr track);

    juce::Rectangle<float> getClipRect(te::Clip::Ptr clip);
    juce::Range<int> getVerticalRangeOfTrack(tracktion_engine::Track::Ptr track, bool withAutomation);

    void setPianoRoll(te::Track *track);

    void buildRecordingClips(te::Track::Ptr track);
    void syncToolButtonsFromState();

    struct DragFileItemInfo
    {
        bool visible{false};
        juce::String name;
        juce::Rectangle<float> drawRect;
        juce::Colour colour;
        bool valid{false};
    };

    juce::OwnedArray<RecordingClipComponent> m_recordingClips;
    // essentials
    EditViewState &m_editViewState;
    juce::OwnedArray<AutomationLaneComponent> m_automationLanes;
    juce::OwnedArray<TrackLaneComponent> m_trackLanes;
    MenuBar &m_toolBar;
    TimeLineComponent &m_timeLine;
    LassoSelectionTool m_lassoComponent;

    // flags
    bool m_isDragging{false};
    bool m_isLassoStartedInAutomation{false};
    bool m_isSelectingTimeRange{false};
    bool m_isDraggingSelectedTimeRange{false};

    Tool m_toolMode{Tool::pointer};

    // caches
    juce::Array<te::Clip *> m_cachedSelectedClips;

    GUIHelpers::SelectedTimeRange m_selectedRange;
    juce::Image m_timeRangeImage;
    juce::Rectangle<float> m_hoveredRectOnAutomation;
    TimeRangeOverlayComponent m_timeRangeOverlay;

    te::Clip::Ptr m_draggedClip;
    juce::Array<ClipPropertyEdit> m_clipPropertyPreview;
    tracktion::TimeDuration m_draggedTimeDelta;
    int m_draggedVerticalOffset{0};

    DragFileItemInfo m_dragItemRect;

    te::Track::Ptr m_hoveredTrack{nullptr};
    tracktion::TimePosition m_timeAtMouseCursor;

    // Centralized drag state
    DragState m_dragState;

    void logMousePositionInfo();

public:
    // Drag state accessors
    DragState &getDragState() { return m_dragState; }
    const DragState &getDragState() const { return m_dragState; }
    bool isDragging() const { return m_dragState.isActive(); }
    void startDrag(DragType type, tracktion::TimePosition time, juce::Point<int> pos);
    void startDrag(DragType type, tracktion::TimePosition time, juce::Point<int> pos, tracktion::EditItemID itemId);
    void updateDrag(tracktion::TimePosition time, juce::Point<int> pos);
    void endDrag();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SongEditorView)
};
