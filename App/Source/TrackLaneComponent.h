
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
#include "AutomationLaneComponent.h"
#include "EditViewState.h"
#include "MouseEventThrottler.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class SongEditorView;

class TrackLaneComponent : public juce::Component
{
public:
    TrackLaneComponent(EditViewState &evs, te::Track::Ptr track, juce::String timelineID, SongEditorView &owner);

    ~TrackLaneComponent() override = default;

    void paint(juce::Graphics &g) override;
    void resized() override;

    void mouseMove(const juce::MouseEvent &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseExit(const juce::MouseEvent &) override;

    te::Track::Ptr getTrack() const { return m_track; }

    void buildAutomationLanes();
    AutomationLaneComponent *getAutomationLane(tracktion::AutomatableParameter::Ptr ap);

private:
    // Helpers
    float timeToX(tracktion::TimePosition time);
    tracktion::TimePosition xtoTime(int x);
    tracktion::TimePosition getSnappedTime(tracktion::TimePosition time, bool downwards = false);
    juce::Rectangle<float> getClipRect(te::Clip::Ptr clip);
    void updateCursor(juce::ModifierKeys mods);

    EditViewState &m_editViewState;
    juce::String m_timeLineID;
    te::Track::Ptr m_track;
    SongEditorView &m_songEditor;
    juce::OwnedArray<AutomationLaneComponent> m_automationLanes;

    // State
    te::Clip::Ptr m_hoveredClip{nullptr};
    bool m_leftBorderHovered{false};
    bool m_rightBorderHovered{false};

    // Note: Dragging state is now managed centrally by SongEditorView via DragState

    // Mouse event throttling
    MouseEventThrottler m_mouseThrottler;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackLaneComponent)
};
