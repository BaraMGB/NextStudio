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

#include "DrawTool.h"
#include "../Utilities.h"

void DrawTool::mouseDown(const juce::MouseEvent& event, MidiViewport& viewport)
{
    // Delegate to MidiViewport to start the drawing process
    viewport.startNoteDraw(event);
}

void DrawTool::mouseDrag(const juce::MouseEvent& event, MidiViewport& viewport)
{
    viewport.setSnap(true);
    if (event.mods.isShiftDown())
        viewport.setSnap(false);
    // Delegate to MidiViewport to update the drawing process
    viewport.updateNoteDraw(event);
}

void DrawTool::mouseUp(const juce::MouseEvent& event, MidiViewport& viewport)
{
    // Delegate to MidiViewport to finalize the note
    viewport.endNoteDraw();
}

void DrawTool::mouseMove(const juce::MouseEvent& event, MidiViewport& viewport)
{
    // Update cursor based on context
    if (viewport.getClipAt(event.x))
    {
        viewport.setMouseCursor(getCursor());
    }
    else
    {
        viewport.setMouseCursor(juce::MouseCursor::NoCursor);
    }
}

void DrawTool::mouseDoubleClick(const juce::MouseEvent& event, MidiViewport& viewport)
{
    // A double-click could create a note with a default length.
    // For now, we can treat it like a single click-and-release.
    viewport.startNoteDraw(event);
    viewport.endNoteDraw();
}

juce::MouseCursor DrawTool::getCursor() const
{
    return juce::MouseCursor::CrosshairCursor;
}

void DrawTool::toolActivated(MidiViewport& viewport)
{
    GUIHelpers::log("DrawTool activated");
    viewport.setMouseCursor(getCursor());
}

void DrawTool::toolDeactivated(MidiViewport& viewport)
{
    GUIHelpers::log("DrawTool deactivated");

    // Ensure any pending drawing operation is cancelled
    viewport.cancelNoteDraw();

    viewport.setMouseCursor(juce::MouseCursor::NormalCursor);
}
