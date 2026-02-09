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

#include "LowerRange/Mixer/MixerComponent.h"
#include "LowerRange/Mixer/MixerChannelStripComponent.h"

//==============================================================================
MixerComponent::MixerComponent(EditViewState &evs)
    : m_evs(evs)
{
    addAndMakeVisible(m_viewport);
    m_viewport.setViewedComponent(&m_channelStripContainer, false);
    m_evs.m_edit.state.addListener(this);
    buildChannelStrips();
}

MixerComponent::~MixerComponent() { m_evs.m_edit.state.removeListener(this); }

void MixerComponent::paint(juce::Graphics &g) { g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId)); }

void MixerComponent::resized()
{
    m_viewport.setBounds(getLocalBounds());
    auto channelWidth = 100;
    m_channelStripContainer.setBounds(0, 0, m_channelStrips.size() * channelWidth, getHeight());

    auto area = m_channelStripContainer.getLocalBounds();

    for (auto strip : m_channelStrips)
        strip->setBounds(area.removeFromLeft(channelWidth));
}

void MixerComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) {}

void MixerComponent::valueTreeChildAdded(juce::ValueTree &p, juce::ValueTree &c)
{
    if (p.hasType(te::IDs::EDIT) && c.hasType(te::IDs::TRACK))
        markAndUpdate(m_updateChannelStrips);
}

void MixerComponent::valueTreeChildRemoved(juce::ValueTree &p, juce::ValueTree &c, int)
{
    if (p.hasType(te::IDs::EDIT) && c.hasType(te::IDs::TRACK))
        markAndUpdate(m_updateChannelStrips);
}

void MixerComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int) { markAndUpdate(m_updateChannelStrips); }

void MixerComponent::handleAsyncUpdate()
{
    if (compareAndReset(m_updateChannelStrips))
        buildChannelStrips();
}

void MixerComponent::buildChannelStrips()
{
    m_channelStrips.clear();
    m_channelStripContainer.removeAllChildren();

    for (auto t : te::getAudioTracks(m_evs.m_edit))
    {
        auto strip = new MixerChannelStripComponent(m_evs, t);
        m_channelStrips.add(strip);
        m_channelStripContainer.addAndMakeVisible(strip);
    }

    if (auto *masterTrack = m_evs.m_edit.getMasterTrack())
    {
        auto strip = new MixerChannelStripComponent(m_evs, masterTrack);
        m_channelStrips.add(strip);
        m_channelStripContainer.addAndMakeVisible(strip);
    }
    resized();
}
