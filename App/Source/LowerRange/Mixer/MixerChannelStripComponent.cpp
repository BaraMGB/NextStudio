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

#include "LowerRange/Mixer/MixerChannelStripComponent.h"
#include "Utilities/Utilities.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace
{
te::LevelMeterPlugin *findLevelMeterPlugin(te::Track &track)
{
    for (auto *plugin : track.pluginList)
        if (auto *level = dynamic_cast<te::LevelMeterPlugin *>(plugin))
            return level;

    return nullptr;
}
} // namespace

MixerChannelStripComponent::MixerChannelStripComponent(EditViewState &evs, te::Track::Ptr track)
    : m_evs(evs),
      m_track(track),
      m_volumeSlider(m_evs.m_edit.getMasterSliderPosParameter()),
      m_panSlider(m_evs.m_edit.getMasterPanParameter())
{
    m_isMasterTrack = m_track != nullptr && m_track->isMasterTrack();
    m_trackName.setText(m_track->getName(), juce::dontSendNotification);

    auto trackCol = m_track->getColour();
    auto labelingCol = trackCol.getBrightness() > 0.8f ? juce::Colour(0xff000000) : juce::Colour(0xffffffff);

    m_trackName.setColour(juce::Label::ColourIds::textColourId, labelingCol);
    m_trackName.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_trackName);

    m_volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    m_volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(m_volumeSlider);

    m_panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(m_panSlider);

    // Initialize meters
    updateComponentsFromTrack();

    m_muteButton.setButtonText("M");
    m_muteButton.onClick = [this]
    {
        if (m_isMasterTrack)
        {
            if (auto masterVol = m_track->edit.getMasterVolumePlugin())
            {
                masterVol->muteOrUnmute();
                m_muteButton.setToggleState(masterVol->getSliderPos() <= 0.0f, juce::dontSendNotification);
            }
        }
        else
        {
            m_track->setMute(!m_track->isMuted(false));
        }
    };
    addAndMakeVisible(m_muteButton);

    m_soloButton.setButtonText("S");
    m_soloButton.onClick = [this] { m_track->setSolo(!m_track->isSolo(false)); };
    addAndMakeVisible(m_soloButton);

    m_armButton.setButtonText("R");
    m_armButton.onClick = [this]
    {
        if (auto at = dynamic_cast<te::AudioTrack *>(m_track.get()))
        {
            EngineHelpers::armTrack(*at, !EngineHelpers::isTrackArmed(*at));
            m_armButton.setToggleState(EngineHelpers::isTrackArmed(*at), juce::dontSendNotification);
        }
    };
    addAndMakeVisible(m_armButton);

    if (m_isMasterTrack)
    {
        m_soloButton.setVisible(false);
        m_soloButton.setEnabled(false);
        m_armButton.setVisible(false);
        m_armButton.setEnabled(false);
    }
    else if (!m_track->isAudioTrack())
    {
        m_armButton.setVisible(false);
        m_armButton.setEnabled(false);
    }

    m_track->state.addListener(this);
}

MixerChannelStripComponent::~MixerChannelStripComponent() { m_track->state.removeListener(this); }

void MixerChannelStripComponent::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) { updateComponentsFromTrack(); }

void MixerChannelStripComponent::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) { updateComponentsFromTrack(); }

void MixerChannelStripComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int) { updateComponentsFromTrack(); }

void MixerChannelStripComponent::updateComponentsFromTrack()
{
    if (m_track == nullptr)
        return;

    m_levelMeterLeft.reset();
    m_levelMeterRight.reset();

    // Re-bind sliders
    if (m_isMasterTrack)
    {
        m_volumeSlider.setParameter(m_track->edit.getMasterSliderPosParameter());
        m_panSlider.setParameter(m_track->edit.getMasterPanParameter());

        if (auto masterVol = m_track->edit.getMasterVolumePlugin())
            m_muteButton.setToggleState(masterVol->getSliderPos() <= 0.0f, juce::dontSendNotification);

        m_levelMeterLeft = std::make_unique<LevelMeterComponent>(
            [this]() -> te::LevelMeasurer *
            {
                if (auto epc = m_track->edit.getTransport().getCurrentPlaybackContext())
                    return &epc->masterLevels;

                return nullptr;
            },
            LevelMeterComponent::ChannelType::Left);
        m_levelMeterRight = std::make_unique<LevelMeterComponent>(
            [this]() -> te::LevelMeasurer *
            {
                if (auto epc = m_track->edit.getTransport().getCurrentPlaybackContext())
                    return &epc->masterLevels;

                return nullptr;
            },
            LevelMeterComponent::ChannelType::Right);
        addAndMakeVisible(*m_levelMeterLeft);
        addAndMakeVisible(*m_levelMeterRight);
    }
    else if (auto at = dynamic_cast<te::AudioTrack *>(m_track.get()))
    {
        if (auto volPlugin = at->getVolumePlugin())
        {
            m_volumeSlider.setParameter(volPlugin->getAutomatableParameterByID("volume"));
            m_panSlider.setParameter(volPlugin->getAutomatableParameterByID("pan"));
        }

        if (auto levelPlugin = at->getLevelMeterPlugin())
        {
            m_levelMeterLeft = std::make_unique<LevelMeterComponent>(levelPlugin->measurer, LevelMeterComponent::ChannelType::Left);
            m_levelMeterRight = std::make_unique<LevelMeterComponent>(levelPlugin->measurer, LevelMeterComponent::ChannelType::Right);

            addAndMakeVisible(*m_levelMeterLeft);
            addAndMakeVisible(*m_levelMeterRight);
        }

        m_muteButton.setToggleState(m_track->isMuted(false), juce::dontSendNotification);
        m_soloButton.setToggleState(m_track->isSolo(false), juce::dontSendNotification);
        m_armButton.setToggleState(EngineHelpers::isTrackArmed(*at), juce::dontSendNotification);
    }
    else if (auto ft = dynamic_cast<te::FolderTrack *>(m_track.get()))
    {
        if (auto volPlugin = ft->getVolumePlugin())
        {
            m_volumeSlider.setParameter(volPlugin->getAutomatableParameterByID("volume"));
            m_panSlider.setParameter(volPlugin->getAutomatableParameterByID("pan"));
        }

        if (auto levelPlugin = findLevelMeterPlugin(*ft))
        {
            m_levelMeterLeft = std::make_unique<LevelMeterComponent>(levelPlugin->measurer, LevelMeterComponent::ChannelType::Left);
            m_levelMeterRight = std::make_unique<LevelMeterComponent>(levelPlugin->measurer, LevelMeterComponent::ChannelType::Right);

            addAndMakeVisible(*m_levelMeterLeft);
            addAndMakeVisible(*m_levelMeterRight);
        }

        m_muteButton.setToggleState(m_track->isMuted(false), juce::dontSendNotification);
        m_soloButton.setToggleState(m_track->isSolo(false), juce::dontSendNotification);
    }

    resized();
}

void MixerChannelStripComponent::paint(juce::Graphics &g)
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

        static const float dbValues[] = {6.0f, 3.0f, 0.0f, -6.0f, -12.0f, -24.0f, -36.0f, -60.0f};

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
            float tickXRight = sliderBounds.getRight() - tickWidth;
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

    auto headerArea = area.removeFromTop(20);

    // Track Name takes full width now
    m_trackName.setBounds(headerArea);

    auto buttonArea = area.removeFromTop(20);
    m_muteButton.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 3));
    m_soloButton.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 2));
    m_armButton.setBounds(buttonArea);

    m_panSlider.setBounds(area.removeFromTop(50));

    auto meterWidth = 6;
    m_volumeSlider.setBounds(area);
    area.reduce(area.getWidth() / 3, 0);

    if (m_levelMeterLeft)
        m_levelMeterLeft->setBounds(area.removeFromLeft(meterWidth).reduced(0, 13));
    if (m_levelMeterRight)
        m_levelMeterRight->setBounds(area.removeFromRight(meterWidth).reduced(0, 13));
}
