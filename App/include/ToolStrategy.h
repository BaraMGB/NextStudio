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
#include "Utilities.h"

class MidiViewport;

/**
 * Base interface for all piano roll tools.
 * Implements the Strategy pattern to separate tool-specific logic from MidiViewport.
 */
class ToolStrategy
{
public:
    explicit ToolStrategy(EditViewState &evs)
        : m_evs(evs)
    {
    }
    virtual ~ToolStrategy() = default;

    /**
     * Called when mouse button is pressed in the viewport.
     * @param event The mouse event
     * @param viewport Reference to the MidiViewport
     */
    virtual void mouseDown(const juce::MouseEvent &event, MidiViewport &viewport) = 0;

    /**
     * Called when mouse is dragged in the viewport.
     * @param event The mouse event
     * @param viewport Reference to the MidiViewport
     */
    virtual void mouseDrag(const juce::MouseEvent &event, MidiViewport &viewport) = 0;

    /**
     * Called when mouse button is released in the viewport.
     * @param event The mouse event
     * @param viewport Reference to the MidiViewport
     */
    virtual void mouseUp(const juce::MouseEvent &event, MidiViewport &viewport) = 0;

    /**
     * Called when mouse moves over the viewport (hover effects).
     * @param event The mouse event
     * @param viewport Reference to the MidiViewport
     */
    virtual void mouseMove(const juce::MouseEvent &event, MidiViewport &viewport) {}

    /**
     * Called when mouse is double-clicked in the viewport.
     * @param event The mouse event
     * @param viewport Reference to the MidiViewport
     */
    virtual void mouseDoubleClick(const juce::MouseEvent &event, MidiViewport &viewport) {}

    /**
     * Returns the appropriate cursor for this tool.
     * @return The cursor type to display
     */
    virtual juce::MouseCursor getCursor(MidiViewport &viewport) const = 0;

    /**
     * Called when this tool becomes active.
     * @param viewport Reference to the MidiViewport
     */
    virtual void toolActivated(MidiViewport &viewport) {}

    /**
     * Called when this tool becomes inactive.
     * @param viewport Reference to the MidiViewport
     */
    virtual void toolDeactivated(MidiViewport &viewport) {}

    virtual Tool getToolId() = 0;

protected:
    EditViewState &m_evs;
};

/**
 * Factory class for creating tool instances.
 */
class ToolFactory
{
public:
    static std::unique_ptr<ToolStrategy> createTool(Tool toolType, EditViewState &evs);
};
