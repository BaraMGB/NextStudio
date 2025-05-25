
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


/*
  ==============================================================================

    MenuBar.cpp
    Created: 23 Feb 2020 5:36:47pm
    Author:  Zehn

  ==============================================================================
*/

#include "MenuBar.h"
#include "Utilities.h"


void MenuBar::addButton(juce::DrawableButton* button, int toggleGroupId)
{
    m_buttons.add(button);
    m_buttonGaps.set(m_buttons.indexOf(button), m_defaultGap);
    addAndMakeVisible(button);

      if (toggleGroupId > 0)
    {
        button->setClickingTogglesState(true);
        button->setRadioGroupId(toggleGroupId);
    }
}
void MenuBar::setButtonGap(int bg)
{
    for (auto gap = 0; gap < m_buttonGaps.size();gap++)
        if (m_buttonGaps.getReference(gap) == m_defaultGap) {setButtonGap(gap, bg);}
    m_defaultGap = bg;
}
void MenuBar::setButtonGap(int index, int gap)
{
    if (index >= 0 && index < m_buttons.size())
        m_buttonGaps.set(index, gap);
}

void MenuBar::resized()
{
    auto flexAlign = juce::FlexBox::JustifyContent::flexStart;
    auto flexDirection = juce::FlexBox::Direction::row;

    if (m_vertical) {
        flexDirection = juce::FlexBox::Direction::column;
    }

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
    fb.flexDirection = flexDirection;
    fb.flexWrap = juce::FlexBox::Wrap::noWrap;
    fb.justifyContent = flexAlign;
    fb.alignItems = juce::FlexBox::AlignItems::center;

    const auto margin = 7.f;
    const float buttonSize = m_vertical ? getWidth() - (2.0f * getWidth() / margin) : getHeight() - (2.0f * getHeight() / margin);

    const int maxButtons = m_buttons.size() + 1; 

    int availableSpace = m_vertical ? getHeight() : getWidth();
    int totalButtonSize = 0;

    for (int i = 0; i < m_buttons.size(); ++i)
    {
        totalButtonSize += buttonSize + m_buttonGaps[i];
    }

    bool enoughSpace = totalButtonSize <= availableSpace;
    if (m_firstTime)
    {
        m_wasEnoughSpace = !enoughSpace;
        m_firstTime = false;
    }

    if ((!enoughSpace) && (m_wasEnoughSpace == true))
    {
       m_wasEnoughSpace = false;
       
       removeAllChildren(); 
      
       m_popupButton = std::make_unique<juce::DrawableButton>("More...", juce::DrawableButton::ImageOnButtonBackground);
        GUIHelpers::setDrawableOnButton(*m_popupButton, BinaryData::menu_svg, juce::Colours::grey);
       addAndMakeVisible(m_popupButton.get());
      
        juce::FlexItem::Margin margin;
        int gap = m_defaultGap;
        margin.top = gap; 
        margin.bottom = gap;
        margin.right = gap;
        margin.left = gap;
       fb.items.add(juce::FlexItem(buttonSize, buttonSize, *m_popupButton).withMargin(margin));
       fb.performLayout(getLocalBounds());
      
       juce::PopupMenu popupMenu;
       for (int i = 0; i < m_buttons.size(); ++i)
       {
           popupMenu.addItem(i + 1, m_buttons[i]->getButtonText());
       }
      
       m_popupButton->onClick = [this, popupMenu]() mutable {
           int result = popupMenu.show();
           if (result > 0 && result <= m_buttons.size())
           {
               m_buttons[result - 1]->triggerClick();
           }
       };
    }
    else if ((enoughSpace) && m_wasEnoughSpace == false) 
    {
        m_wasEnoughSpace = true;
        removeAllChildren();

        for (int i = 0; i < m_buttons.size(); ++i)
        {
            addAndMakeVisible(*m_buttons[i]);
            juce::FlexItem::Margin margin;
            if (i < m_buttons.size() - 1) {
                int gap = m_buttonGaps[i];
                margin.bottom = m_vertical ? gap : 0.0;
                margin.right = m_vertical ? 0.0 : gap;
            }

            fb.items.add(juce::FlexItem(buttonSize, buttonSize, *m_buttons[i]).withMargin(margin));
        }

        fb.performLayout(getLocalBounds().reduced(m_vertical ? getWidth() / 5.0f : getHeight() / 5.0f).toFloat());
    }
}

juce::Array<juce::DrawableButton*> MenuBar::getButtons()
{
    return m_buttons;
}
