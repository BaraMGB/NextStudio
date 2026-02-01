
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
// #include "Utilities/ApplicationViewState.h"
#include "Utilities/Utilities.h"

namespace te = tracktion_engine;

class PositionDisplayComponent : public juce::Component
{
public:
    explicit PositionDisplayComponent(te::Edit &edit);

    void paint(juce::Graphics &) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;

    void update();

private:
    te::Edit &m_edit;
    juce::Rectangle<int> m_bmpRect, m_sigRect, m_barBeatTickRect, m_timeRect, m_loopInrect, m_loopOutRect;
    juce::Label m_bpmLabel, m_sigLabel, m_barBeatTickLabel, m_timeLabel, m_loopInLabel, m_loopOutLabel;

    juce::Point<int> m_mousedownPosition;

    double m_mousedownBPM{};
    tracktion::TimePosition m_mousedownTime;
    tracktion::BeatPosition m_mousedownBeatPosition, m_mousedownLoopIn, m_mousedownLoopOut;

    int m_mousedownNumerator{}, m_mousedownDenominator{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PositionDisplayComponent)
};
