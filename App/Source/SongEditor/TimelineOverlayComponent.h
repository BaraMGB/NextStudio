
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
#include "SongEditor/TimeLineComponent.h"
#include "Utilities/EditViewState.h"

namespace te = tracktion_engine;

class TimelineOverlayComponent : public juce::Component
{
public:
    TimelineOverlayComponent(EditViewState &evs, te::Track::Ptr track, TimeLineComponent &tlc);
    void paint(juce::Graphics &g) override;

private:
    bool hitTest(int, int) override;
    void mouseMove(const juce::MouseEvent &e) override;
    void mouseExit(const juce::MouseEvent &e) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;

    std::vector<te::MidiClip *> getMidiClipsOfTrack();
    tracktion_engine::MidiClip *getMidiClipAtPoint(juce::Point<int> point);

    int timeToX(double time);
    double xToBeats(int x);
    double getSnappedTime(double time);
    void updateClipRects();
    void moveSelectedClips(bool copy, bool snap);
    juce::Rectangle<int> getClipRect(te::Clip::Ptr);
    EditViewState &m_evs;
    tracktion_engine::Track::Ptr m_track;
    [[maybe_unused]] double m_loop1AtMousedown{}, m_loop2AtMousedown{};
    bool m_leftResized{false};
    bool m_rightResized{false};
    bool m_move{false};
    bool m_drawDraggedClip{false};
    te::ClipPosition m_cachedPos;
    te::MidiClip *m_cachedClip{};
    TimeLineComponent &m_timelineComponent;
    juce::Array<juce::Rectangle<int>> m_clipRects;
    juce::Array<te::MidiClip *> m_clipsForRects;
    juce::Rectangle<int> m_draggedClipRect;
    double m_draggedTimeDelta;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineOverlayComponent)
};
