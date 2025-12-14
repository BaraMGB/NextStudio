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

#include "MixerChannelStripComponent.h"
#include "Utilities.h"

MixerChannelStripComponent::MixerChannelStripComponent(EditViewState& evs, te::AudioTrack::Ptr at)
    : m_evs(evs)
    , m_track(at)
    , m_volumeSlider(at->getVolumePlugin()->getAutomatableParameterByID("volume"))
    , m_panSlider(at->getVolumePlugin()->getAutomatableParameterByID("pan"))
    , m_levelMeterLeft(at->getLevelMeterPlugin()->measurer, LevelMeterComponent::ChannelType::Left)
    , m_levelMeterRight(at->getLevelMeterPlugin()->measurer, LevelMeterComponent::ChannelType::Right)
{
    m_trackName.setText(m_track->getName(), juce::dontSendNotification);

    auto trackCol = m_track->getColour();
    auto labelingCol = trackCol.getBrightness() > 0.8f
             ? juce::Colour(0xff000000)
             : juce::Colour(0xffffffff);

    m_trackName.setColour(juce::Label::ColourIds::textColourId, labelingCol);
    m_trackName.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_trackName);

    m_volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    m_volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(m_volumeSlider);

    m_panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(m_panSlider);

    addAndMakeVisible(m_levelMeterLeft);
    addAndMakeVisible(m_levelMeterRight);

    m_muteButton.setButtonText("M");
    m_muteButton.onClick = [this] { m_track->setMute(!m_track->isMuted(false)); };
    addAndMakeVisible(m_muteButton);

    m_soloButton.setButtonText("S");
    m_soloButton.onClick = [this] { m_track->setSolo(!m_track->isSolo(false)); };
    addAndMakeVisible(m_soloButton);

    m_armButton.setButtonText("R");
    m_armButton.onClick = [this]
    {
        EngineHelpers::armTrack (*m_track, !EngineHelpers::isTrackArmed (*m_track));
        m_armButton.setToggleState (EngineHelpers::isTrackArmed (*m_track), juce::dontSendNotification);
    };
    addAndMakeVisible(m_armButton);
}

MixerChannelStripComponent::~MixerChannelStripComponent()
{
}

void MixerChannelStripComponent::paint (juce::Graphics& g)
{
    g.fillAll(m_evs.m_applicationState.getBackgroundColour2());
    auto bounds = getLocalBounds();
    bounds.reduce(2, 2);
    g.fillRoundedRectangle(bounds.toFloat(), 10);
    bounds.reduce(1, 1);
    g.setColour(m_evs.m_applicationState.getBackgroundColour1());
    g.fillRoundedRectangle(bounds.toFloat(), 10);

    auto header = bounds.removeFromTop(20);
    g.setColour(m_track->getColour());
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), 9, true, true, false, false);

    // Draw Fader Scale
    if (m_volumeSlider.isVisible())
    {
        auto sliderBounds = m_volumeSlider.getBounds();
        sliderBounds.reduce(sliderBounds.getWidth() / 4, 0);
        float thumbPadding = 13.0f; // Must match LookAndFeel thumb radius/padding
        float trackTop = sliderBounds.getY() + thumbPadding;
        float trackBottom = sliderBounds.getBottom() - thumbPadding;
        float trackHeight = trackBottom - trackTop;

        g.setColour(juce::Colours::grey);
        g.setFont(juce::FontOptions(10.0f));

        static const float dbValues[] = { 6.0f, 3.0f, 0.0f, -6.0f, -12.0f, -24.0f, -36.0f, -60.0f };

        for (auto db : dbValues)
        {
            float val = te::decibelsToVolumeFaderPosition(db);
            float yPos = trackBottom - (val * trackHeight);

            // Draw tick mark overlapping the left meter/slider edge
            // sliderBounds.getX() is the left edge of the slider component.
            float tickX = sliderBounds.getX();
            float tickWidth = 4.f;
            g.fillRect(tickX, yPos, tickWidth, 1.0f);

            // and on the right side of slider component, too
            float tickXRight = sliderBounds.getRight() -tickWidth;
            g.fillRect(tickXRight, yPos, tickWidth, 1.0f);
            // Draw Label to the left of the tick
            // We draw it over the left meter or to its left
            g.drawText(juce::String((int)db), (int)tickX - 22, (int)yPos - 5, 20, 10, juce::Justification::centredRight, false);
        }
    }
}

void MixerChannelStripComponent::resized()
{
    auto area = getLocalBounds();
    area.reduce(3, 3);

    m_trackName.setBounds(area.removeFromTop(20));

    auto buttonArea = area.removeFromTop(20);
    m_muteButton.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 3));
    m_soloButton.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 2));
    m_armButton.setBounds(buttonArea);

    m_panSlider.setBounds(area.removeFromTop(50));

    auto meterWidth = 6;
    m_volumeSlider.setBounds(area);
    area.reduce(area.getWidth()/3, 0);
    m_levelMeterLeft.setBounds(area.removeFromLeft(meterWidth).reduced(0, 13));
    m_levelMeterRight.setBounds(area.removeFromRight(meterWidth).reduced(0, 13));
}
