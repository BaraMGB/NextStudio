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
    , m_levelMeter(at->getLevelMeterPlugin()->measurer)
{
    m_trackName.setText(m_track->getName(), juce::dontSendNotification);
    m_trackName.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_trackName);

    m_volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    m_volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(m_volumeSlider);

    m_panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    m_panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(m_panSlider);

    addAndMakeVisible(m_levelMeter);

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
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setColour (juce::Colours::grey);
    g.drawRect (getLocalBounds(), 1);
}

void MixerChannelStripComponent::resized()
{
    auto area = getLocalBounds();
    m_trackName.setBounds(area.removeFromBottom(20));

    auto buttonArea = area.removeFromBottom(20);
    m_muteButton.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 3));
    m_soloButton.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 2));
    m_armButton.setBounds(buttonArea);

    m_panSlider.setBounds(area.removeFromTop(50));

    m_volumeSlider.setBounds(area.withRight(area.getRight() - 15));

    m_levelMeter.setBounds(area.withLeft(area.getRight() - 15));
}
