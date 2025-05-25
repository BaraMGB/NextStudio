
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


#include "LevelMeterComponent.h"


LevelMeterComponent::LevelMeterComponent (te::LevelMeasurer &lm)
    : m_levelMeasurer (lm)
{
    setOpaque (true);
    m_levelMeasurer.addClient(m_levelClient);
    startTimerHz(30);
}

LevelMeterComponent::~LevelMeterComponent()
{
    m_levelMeasurer.removeClient(m_levelClient);
    stopTimer();
}

void LevelMeterComponent::paint(juce::Graphics &g)
{
    g.setColour (juce::Colours::black);
    g.fillRect (getLocalBounds ());
    const double meterHeight{ double(getHeight()) };
    const double meterWidth{ double(getWidth()) };
    const double offSet{ fabs(RANGEMINdB) };
    const double scaleFactor{ meterHeight / (RANGEMAXdB + offSet) };

    // draw meter Gain bar
    auto lineAtNullDb = float(meterHeight - (offSet * scaleFactor));
    auto displayBarHeightLeft
            = ((m_currentLeveldBLeft + offSet) * scaleFactor);
    auto displayBarHeightRight
            = ((m_currentLeveldBRight + offSet) * scaleFactor);

    if (float(meterHeight - displayBarHeightLeft) <= lineAtNullDb
            || float(meterHeight - displayBarHeightRight) <= lineAtNullDb)
    {
        g.setColour (juce::Colours::red);
    }
    else
    {
        g.setColour(juce::Colour(0xff74bb00));
    }

    if (displayBarHeightLeft > 0 || displayBarHeightRight > 0)
    {
        g.fillRect(0.0f
                   , float(meterHeight - displayBarHeightLeft)
                   , float(meterWidth * 0.45)
                   , juce::jmax(0.0f, float(displayBarHeightLeft)));
        g.fillRect(float(meterWidth * 0.55)
                   , float(meterHeight - displayBarHeightRight)
                   , float(meterWidth)
                   , juce::jmax(0.0f, float(displayBarHeightRight)));
    }
}

void LevelMeterComponent::timerCallback()
{
    m_prevLeveldBLeft = m_currentLeveldBLeft;
    m_prevLeveldBRight = m_currentLeveldBRight;
    m_currentLeveldBLeft = m_levelClient.getAndClearAudioLevel(0).dB;
    m_currentLeveldBRight = m_levelClient.getAndClearAudioLevel(1).dB;

    // Now we give the level bar fading charcteristics.
    // And, the below coversions, decibelsToGain and gainToDecibels,
    // take care of 0dB, which will never fade!...but a gain of 1.0 (0dB) will.

    const auto prevLevelLeft{
        juce::Decibels::decibelsToGain(m_prevLeveldBLeft) };
    const auto prevLevelRight{
        juce::Decibels::decibelsToGain(m_prevLeveldBRight) };

    if (m_prevLeveldBLeft > m_currentLeveldBLeft)
    {
        m_currentLeveldBLeft
                = juce::Decibels::gainToDecibels(prevLevelLeft * 0.94);
    }

    if (m_prevLeveldBRight > m_currentLeveldBRight)
    {
        m_currentLeveldBRight
                = juce::Decibels::gainToDecibels(prevLevelRight * 0.94);
    }

    // the test below may save some unnecessary paints
    if(m_currentLeveldBLeft != m_prevLeveldBLeft
     || m_currentLeveldBRight != prevLevelRight)
    {
        repaint();
    }
}
