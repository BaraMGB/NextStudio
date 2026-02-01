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

#include "Tools/tools/RangeTool.h"

void RangeTool::mouseDown(const juce::MouseEvent &event, MidiViewport &viewport) { viewport.startLasso(event, true); }

void RangeTool::mouseDrag(const juce::MouseEvent &event, MidiViewport &viewport) { viewport.updateLasso(event); }

void RangeTool::mouseUp(const juce::MouseEvent &event, MidiViewport &viewport) { viewport.stopLasso(); }

void RangeTool::mouseMove(const juce::MouseEvent &event, MidiViewport &viewport) { viewport.setMouseCursor(getCursor(viewport)); }

void RangeTool::mouseDoubleClick(const juce::MouseEvent &event, MidiViewport &viewport)
{
    // No double click action
}

juce::MouseCursor RangeTool::getCursor(MidiViewport &viewport) const { return GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Range, viewport.getCursorScale()); }

void RangeTool::toolActivated(MidiViewport &viewport) { viewport.setMouseCursor(getCursor(viewport)); }

void RangeTool::toolDeactivated(MidiViewport &viewport) { viewport.setMouseCursor(juce::MouseCursor::NormalCursor); }
