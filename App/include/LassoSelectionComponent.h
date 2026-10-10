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

#include "SelectionGestures.h"
#include <functional>
#include <utility>

// Display and geometry only. The source supplies coordinate conversion and hits.
class LassoSelectionComponent
{
public:
    using Projection = std::function<juce::Point<float>(juce::Point<double>)>;
    void setViewBounds(juce::Rectangle<int> bounds) { m_viewBounds = bounds; }
    void setProjection(Projection project) { m_project = std::move(project); }
    void begin(juce::Point<double> anchor) { m_gesture.begin(anchor); }
    void update(juce::Point<double> end) { m_gesture.update(end); }
    void end() { m_gesture.end(); }
    bool active() const { return m_gesture.active(); }
    juce::Rectangle<double> contentBounds() const { return m_gesture.bounds(); }
    juce::Rectangle<float> viewBounds() const { return m_project ? m_gesture.viewBounds(m_project) : juce::Rectangle<float>{}; }
    void drawLasso(juce::Graphics&);
private:
    LassoGesture m_gesture;
    Projection m_project;
    juce::Rectangle<int> m_viewBounds;
};
