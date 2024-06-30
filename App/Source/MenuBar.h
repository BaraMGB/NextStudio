
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
    MenuBar(Alignment align = Alignment::Left, bool vert = false)
    {
        m_alignment = align;
        m_vertical = vert;
    }

    ~MenuBar() override
    {
        m_buttons.clear();
    }


    void addButton(juce::DrawableButton* button, int toggleGroupId=0);
    void setButtonGap(int bg);
    void setButtonGap(int index, int gap);

    void resized() override;

    juce::Array<juce::DrawableButton*> getButtons();

private:

    Alignment m_alignment;
    juce::Array<juce::DrawableButton*> m_buttons;
    std::unique_ptr<juce::DrawableButton> m_popupButton;
    juce::Array<int> m_buttonGaps;
    bool m_wasEnoughSpace   {false};
    bool m_firstTime {true};
    bool m_vertical {false};
    int m_defaultGap {5};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MenuBar)
};
