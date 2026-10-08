
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

#include "EditViewState.h"
#include "TimelineSnapResolver.h"
#include "TimelineInteractionPreview.h"
#include "MouseGestureInput.h"

class TimeLineComponent
    : public juce::Component
    , public juce::SettableTooltipClient
    , private juce::AsyncUpdater
    , private juce::ValueTree::Listener
{
public:
    TimeLineComponent(EditViewState &, juce::String timeLineID, bool usePianoRollSnapSettings = false);
    ~TimeLineComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;
    void moved() override;
    bool keyPressed(const juce::KeyPress&) override;
    void parentHierarchyChanged() override;
    void zoomByFactor(double factor, double anchorX);

    void mouseMove(const juce::MouseEvent &e) override;
    void mouseExit(const juce::MouseEvent &e) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &event) override;
    void modifierKeysChanged(const juce::ModifierKeys&) override;

    te::TimecodeSnapType getBestSnapType();
    EditViewState &getEditViewState();

    tracktion::BeatRange getCurrentBeatRange();
    tracktion::TimeRange getCurrentTimeRange();
    double getBeatsPerPixel();

    float timeToX(double time);
    float beatsToX(double beats);
    tracktion::TimeDuration xToTimeDuration(float x);
    tracktion::TimePosition beatToTime(tracktion::BeatPosition beats);
    tracktion::TimePosition xToTimePos(float x);
    tracktion::BeatPosition xToBeatPos(float x);

    juce::String getTimeLineID() { return m_timeLineID; }
    void setTimeLineID(juce::String timeLineID);

    void setLastNoteLength(double length);
    double getLastNoteLength() const;
    double getNoteInsertLength() const;
    double getClipInsertLength() const;

    // snapes relative to clip start
    double getQuantisedNoteBeat(double beat, const te::MidiClip *c, bool down = true) const;
    double getQuantisedBeat(double beat, bool down) const;
    te::TimecodeSnapType getBestSnapType() const;
    bool isSnappingEnabled() const;
    bool isUsingFixedSnap() const;
    double getSnapIntervalBeats() const;
    double getAdaptiveSnapIntervalBeats() const;
    double getNudgeDeltaBeats(double beat, int direction) const;
    tracktion::TimePosition snapTime(tracktion::TimePosition time, bool down = false) const;
    double getSnappedTime(double time);
    TimelineSnapResolver getMouseSnapResolver() const;
    void setMouseFeedback(std::optional<TimelineInteractionFeedback>, std::function<void()> refresh = {});
    void clearMouseFeedback(TimelineFeedbackOwner);
    void drawMouseFeedback(juce::Graphics&, TimelineFeedbackOwner);
    void drawRulerMouseFeedback(juce::Graphics&, juce::Rectangle<float> rulerBounds);
    const std::optional<TimelineInteractionFeedback>& mouseFeedback() const { return m_mouseFeedback; }
    void mouseFeedbackGeometryChanged() { ++m_feedbackGeometryRevision; m_feedbackGeometryDirty = true; triggerAsyncUpdate(); }
    std::function<void(std::optional<TimelineInteractionFeedback>)> onMouseFeedback;
    NoteTimingPreviewHandler onNoteInteractionPreview;
    std::function<void()> onNoteInteractionBeginning;
    double snapBeatForMouse(double globalBeat) const;
    tracktion::TimePosition snapTimeForMouse(tracktion::TimePosition) const;

private:
    void handleAsyncUpdate() override;
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void updateViewportContext();
    void drawLoopRange(juce::Graphics &g);
    void updateViewRange(const juce::MouseEvent &e);

    juce::Rectangle<float> getTimeRangeRect(tracktion::TimeRange tr);

    juce::String m_timeLineID;

    EditViewState &m_evs;
    juce::MouseCursor m_drawLoopCursor;
    const bool m_usePianoRollSnapSettings;
    juce::ValueTree m_tree;
    double m_cachedBeat{};
    bool m_pointerInteractionActive = false;
    bool m_isMouseDown{false}, m_isSnapping{true}, m_leftResized{false}, m_rightResized{false}, m_changeLoopRange{false}, m_loopRangeClicked{false};

    tracktion::TimeRange m_cachedLoopRange;
    tracktion::TimeRange m_newLoopRange;
    std::optional<TimelineInteractionFeedback> m_mouseFeedback;
    bool m_feedbackGeometryDirty = false;
    uint64_t m_feedbackGeometryRevision = 0;
    class FeedbackMovementWatcher : public juce::ComponentMovementWatcher
    {
    public:
        explicit FeedbackMovementWatcher(TimeLineComponent& owner) : ComponentMovementWatcher(&owner), m_owner(owner) {}
        void componentMovedOrResized(bool, bool) override { m_owner.mouseFeedbackGeometryChanged(); }
        void componentPeerChanged() override { m_owner.mouseFeedbackGeometryChanged(); }
        void componentVisibilityChanged() override { m_owner.mouseFeedbackGeometryChanged(); }
    private:
        TimeLineComponent& m_owner;
    };
    FeedbackMovementWatcher m_feedbackMovementWatcher;
    std::optional<TimelineSnapResolver::Context> m_feedbackContext;
    std::function<void()> m_refreshMouseFeedback;
    TimelineMouseGesture m_loopGesture;
    MouseGestureInput m_mouseInput;
    double m_loopCreationStartBeat = 0;
    int m_oldDragDistanceY, m_oldDragDistanceX;
    bool m_cachedFollowPlayhead;
    bool m_playheadClickPending{false};
    juce::NativeScaleFactorNotifier m_scaleNotifier;
    TimelineViewGeometry::ZoomIntent m_zoomIntent;
    uint64_t m_zoomRevision = 0;
    uint64_t m_musicalSnapRevision = 0;
    double m_zoomStart = -1;
    double m_dragRequestedZoom = 0;
    TimelineViewGeometry::ViewportContext m_dragViewport;
    uint64_t m_dragRevision = 0;
    double m_dragViewStart = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeLineComponent)
};
