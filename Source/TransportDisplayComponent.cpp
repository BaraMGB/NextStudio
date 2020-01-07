/*
  ==============================================================================

    TransportDisplayComponent.cpp
    Created: 7 Jan 2020 8:30:43pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "TransportDisplayComponent.h"

TransportDisplayComponent::TransportDisplayComponent() :
    m_bars(1, 1, 999, ".", Justification::centredRight, 1),
    m_beat(1, 1, 4, ".", Justification::centredRight, 2),
    m_quat(1, 1, 4, ".", Justification::centredRight, 3),
    m_cent(1, 1, 99, "", Justification::centredRight, 4)
{
    addAndMakeVisible(m_bars);
    addAndMakeVisible(m_beat);
    addAndMakeVisible(m_quat);
    addAndMakeVisible(m_cent);

    m_cent.addChangeListener(&m_quat);
    m_quat.addChangeListener(&m_beat);
    m_beat.addChangeListener(&m_bars);

    setSize(600, 400);
}

TransportDisplayComponent::~TransportDisplayComponent()
{
}

void TransportDisplayComponent::paint(Graphics& g)
{
    g.fillAll(Colour(0xff323e44));
}

void TransportDisplayComponent::resized()
{
    FlexBox flexbox{ FlexBox::Direction::row, FlexBox::Wrap::noWrap, FlexBox::AlignContent::center,
        FlexBox::AlignItems::flexStart, FlexBox::JustifyContent::flexStart };

    int w = 35;
    flexbox.items.add(FlexItem(w, getHeight(), m_bars));
    flexbox.items.add(FlexItem(w, getHeight(), m_beat));
    flexbox.items.add(FlexItem(w, getHeight(), m_quat));
    flexbox.items.add(FlexItem(w, getHeight(), m_cent));
    flexbox.performLayout(getLocalBounds());
}


void TransportDisplayComponent::setPosition(double ppqPosition)
{
    int tpqn = 960/* m_edit->ticksPerQuarterNote */;
    m_bars.setValue((static_cast<int>(ppqPosition) / tpqn / 4) + 1);
    m_beat.setValue((static_cast<int>(ppqPosition) / tpqn % 4) + 1);
    m_quat.setValue((static_cast<int>(ppqPosition) / (tpqn / 4) % 4) + 1);
    m_cent.setValue((static_cast<int>(ppqPosition) / ((tpqn / 4) / 100) % 100));
}
