
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

namespace te = tracktion_engine;

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
        m_buttons.clear(false);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff171717));
    }

    void addButton(juce::DrawableButton* button)
    {
        m_buttons.add(button);
        m_buttonGaps.set(m_buttons.indexOf(button), 5);
        addAndMakeVisible(button);
    }

    void setButtonGap(int index, int gap)
    {
        if (index >= 0 && index < m_buttons.size())
            m_buttonGaps.set(index, gap);
    }

    void resized() override
    {
        auto flexAlign = juce::FlexBox::JustifyContent::flexStart;

        switch (m_alignment)
        {
            case Alignment::Left:
                flexAlign = juce::FlexBox::JustifyContent::flexStart;
                break;
            case Alignment::Center:
                flexAlign = juce::FlexBox::JustifyContent::center;
                break;
            case Alignment::Right:
                flexAlign = juce::FlexBox::JustifyContent::flexEnd;
                break;
        }

        juce::FlexBox fb;
        fb.flexWrap = juce::FlexBox::Wrap::noWrap;
        fb.justifyContent = flexAlign;
        fb.alignItems = juce::FlexBox::AlignItems::center;

        const float buttonSize = getHeight() - (2.0f * getHeight() / 5.0f);

        for (int i = 0; i < m_buttons.size(); ++i)
        {
            juce::FlexItem::Margin margin;
            if (i < m_buttons.size() - 1) {
                int gap = m_buttonGaps[i];
                margin.right = gap;
            }

            fb.items.add(juce::FlexItem(buttonSize, buttonSize, *m_buttons[i]).withMargin(margin));
        }

        fb.performLayout(getLocalBounds().reduced(getHeight() / 5.0f).toFloat());
    }

private:

    Alignment m_alignment;
    juce::OwnedArray<juce::DrawableButton> m_buttons;
    juce::Array<int> m_buttonGaps; // Array für Lückenmaße

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MenuBar)
};
