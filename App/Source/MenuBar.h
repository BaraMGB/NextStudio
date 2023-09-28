
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

/*
  ==============================================================================

    MenuBar.h
    Created: 23 Feb 2020 5:36:47pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"


enum class Alignment
{
    Left,
    Center,
    Right
};

class MenuBar : public juce::Component
{
public:
    MenuBar(Alignment align = Alignment::Left)
    {
        m_alignment = align;
    }

    ~MenuBar() override
    {
        m_buttons.clear();
    }

    void paint(juce::Graphics& g) override;

    void addButton(juce::DrawableButton* button, int toggleGroupId=0);
    void setButtonGap(int index, int gap);

    void resized() override;

    juce::Array<juce::DrawableButton*> getButtons();

private:

    Alignment m_alignment;
    juce::Array<juce::DrawableButton*> m_buttons;
    juce::Array<int> m_buttonGaps;
    bool m_wasEnoughSpace   {false};
    bool m_firstTime {true};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MenuBar)
};
