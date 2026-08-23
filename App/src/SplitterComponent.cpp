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

#include "SplitterComponent.h"
SplitterComponent::SplitterComponent(bool horizontal)
    : m_isHorizontal(horizontal)
{
}

void SplitterComponent::mouseMove(const juce::MouseEvent &)
{
    setMouseCursor(m_isHorizontal ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::LeftRightResizeCursor);
    m_isHovering = true;
    repaint();
}

void SplitterComponent::mouseEnter(const juce::MouseEvent &) {}

void SplitterComponent::mouseExit(const juce::MouseEvent &)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
    m_isHovering = false;
    repaint();
}

void SplitterComponent::mouseDown(const juce::MouseEvent &event)
{
    m_dragStartScreenPosition = event.getScreenPosition();
    if (onMouseDown)
        onMouseDown();
}

void SplitterComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (!onDrag)
        return;

    const auto screenDistance = event.getScreenPosition() - m_dragStartScreenPosition;
    onDrag(m_isHorizontal ? screenDistance.y : screenDistance.x);
}

void SplitterComponent::mouseUp(const juce::MouseEvent &) {}

void SplitterComponent::paint(juce::Graphics &g)
{
    if (m_isHovering)
    {
        g.setColour(juce::Colour(0xffffffff).withAlpha(0.7f));
        g.fillRect(getLocalBounds());
    }
}
