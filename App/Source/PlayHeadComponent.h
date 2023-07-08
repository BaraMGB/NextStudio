
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"


namespace te = tracktion_engine;

class PlayheadComponent : public juce::Component
                        , private juce::Timer
{
public:
    PlayheadComponent (te::Edit&
                       , EditViewState&
                       , juce::CachedValue<double>& x1
                       , juce::CachedValue<double>& x2);

    void paint (juce::Graphics& g) override;
    bool hitTest (int x, int y) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    bool isPlaying() const;

private:
    void timerCallback() override;

    te::Edit& m_edit;
    EditViewState& m_editViewState;
    juce::CachedValue<double> & m_X1;
    juce::CachedValue<double> & m_X2;

    int m_xPosition = 0;
    bool m_firstTimer = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayheadComponent)
};
