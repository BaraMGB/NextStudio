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

#include "LassoSelectionComponent.h"

void LassoSelectionComponent::drawLasso(juce::Graphics& g)
{
    if (!m_gesture.active() || !m_gesture.dragged()) return;
    const auto rect = viewBounds();
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(m_viewBounds);
    g.setColour(juce::Colour(0x22ffffff));
    g.fillRect(rect);
    g.setColour(juce::Colour(0x99ffffff));
    g.drawRect(rect, 1.0f);
}
