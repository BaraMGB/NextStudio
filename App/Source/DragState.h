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
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

enum class DragType
{
    None,
    Playhead,
    Clip,
    CurvePoint,
    TimeRangeLeft,
    TimeRangeRight,
    TimeRangeMove,
    Selection,
    Lasso
};

struct DragState
{
    DragType type = DragType::None;
    tracktion::TimePosition startTime{tracktion::TimePosition::fromSeconds(0.0)};
    tracktion::TimePosition currentTime{tracktion::TimePosition::fromSeconds(0.0)};
    juce::Point<int> startPos{0, 0};
    tracktion::EditItemID draggedItemId;
    te::Clip::Ptr draggedClip{nullptr};
    te::AutomatableParameter::Ptr draggedParameter{nullptr};
    tracktion::TimeDuration timeDelta{tracktion::TimeDuration::fromSeconds(0.0)};
    int verticalOffset{0};
    bool isLeftEdge{false};
    bool isRightEdge{false};
    bool playheadWasMoved{false};

    bool isActive() const { return type != DragType::None; }

    bool isTimeRangeDrag() const { return type == DragType::TimeRangeLeft || type == DragType::TimeRangeRight || type == DragType::TimeRangeMove; }

    bool isClipDrag() const { return type == DragType::Clip; }

    bool isPointDrag() const { return type == DragType::CurvePoint; }

    void reset()
    {
        type = DragType::None;
        startTime = tracktion::TimePosition::fromSeconds(0.0);
        currentTime = tracktion::TimePosition::fromSeconds(0.0);
        startPos = juce::Point<int>(0, 0);
        draggedItemId = tracktion::EditItemID();
        draggedClip = nullptr;
        draggedParameter = nullptr;
        timeDelta = tracktion::TimeDuration::fromSeconds(0.0);
        verticalOffset = 0;
        isLeftEdge = false;
        isRightEdge = false;
        playheadWasMoved = false;
    }

    void startDrag(DragType dragType, tracktion::TimePosition time, juce::Point<int> pos)
    {
        type = dragType;
        startTime = time;
        currentTime = time;
        startPos = pos;
        draggedItemId = tracktion::EditItemID();
    }

    void startDrag(DragType dragType, tracktion::TimePosition time, juce::Point<int> pos, tracktion::EditItemID itemId)
    {
        type = dragType;
        startTime = time;
        currentTime = time;
        startPos = pos;
        draggedItemId = itemId;
    }
};
