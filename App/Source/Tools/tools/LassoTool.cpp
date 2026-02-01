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

#include "Tools/tools/LassoTool.h"

void LassoTool::mouseDown(const juce::MouseEvent &event, MidiViewport &viewport) { viewport.startLasso(event, false); }

void LassoTool::mouseDrag(const juce::MouseEvent &event, MidiViewport &viewport) { viewport.updateLasso(event); }

void LassoTool::mouseUp(const juce::MouseEvent &event, MidiViewport &viewport)
{
    viewport.stopLasso();

    // Switch back to PointerTool after lasso selection is complete
    viewport.setTool(Tool::pointer);
}

void LassoTool::mouseMove(const juce::MouseEvent &event, MidiViewport &viewport) { viewport.setMouseCursor(getCursor(viewport)); }

void LassoTool::mouseDoubleClick(const juce::MouseEvent &event, MidiViewport &viewport)
{
    // No double click action
}

juce::MouseCursor LassoTool::getCursor(MidiViewport &viewport) const { return GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Lasso, viewport.getCursorScale()); }

void LassoTool::toolActivated(MidiViewport &viewport) { viewport.setMouseCursor(getCursor(viewport)); }

void LassoTool::toolDeactivated(MidiViewport &viewport) { viewport.setMouseCursor(juce::MouseCursor::NormalCursor); }
