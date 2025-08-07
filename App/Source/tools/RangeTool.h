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

#include "../ToolStrategy.h"
#include "../MidiViewport.h"

/**
 * Range tool for selecting a time range.
 */
class RangeTool : public ToolStrategy
{
public:
    explicit RangeTool(EditViewState& evs) : ToolStrategy(evs) {}
    ~RangeTool() override = default;

    void mouseDown(const juce::MouseEvent& event, MidiViewport& viewport) override;
    void mouseDrag(const juce::MouseEvent& event, MidiViewport& viewport) override;
    void mouseUp(const juce::MouseEvent& event, MidiViewport& viewport) override;
    void mouseMove(const juce::MouseEvent& event, MidiViewport& viewport) override;
    void mouseDoubleClick(const juce::MouseEvent& event, MidiViewport& viewport) override;

    juce::MouseCursor getCursor(MidiViewport& viewport) const override;
    
    void toolActivated(MidiViewport& viewport) override;
    void toolDeactivated(MidiViewport& viewport) override;
    Tool getToolId () override { return Tool::range; }
};
