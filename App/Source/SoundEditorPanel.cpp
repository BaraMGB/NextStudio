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
#include "SoundEditorPanel.h"

SoundEditorPanel::SoundEditorPanel(te::Edit& edit)
    : m_edit(edit)
{
    GUIHelpers::log("SoundEditorPanel: constructor");

    // Create gain slider with range -48.0 to 48.0
    gainValue.setValue(-48.0);
    gainSlider = std::make_unique<NonAutomatableParameterComponent>(
        gainValue, "Gain", -480, 480);
    gainSlider->setSliderRange(-48.0, 48.0, 0.1);
    addAndMakeVisible(*gainSlider);

    // Create pan slider with range -1.0 to 1.0
    panValue.setValue(0.0);
    panSlider = std::make_unique<NonAutomatableParameterComponent>(
        panValue, "Pan", -100, 100);
    panSlider->setSliderRange(-1.0, 1.0, 0.01);
    addAndMakeVisible(*panSlider);

    // Set up callbacks for value changes
    gainValue.addListener(this);
    panValue.addListener(this);

    m_thumbnail = std::make_unique<SampleDisplay>(m_edit.getTransport());
    addAndMakeVisible(*m_thumbnail);

    // Set up callback for marker position changes
    m_thumbnail->onMarkerPositionChanged = [this](double start, double end)
    {
        if (samplerPlugin && soundIndex != -1)
        {
            double length = end - start;
            samplerPlugin->setSoundExcerpt(soundIndex, start, length);
        }
    };

    addAndMakeVisible(openEndedButton);
    openEndedButton.addListener(this);

    addAndMakeVisible(openEndedLabel);
    openEndedLabel.setText("Open Ended", juce::dontSendNotification);
}

SoundEditorPanel::~SoundEditorPanel()
{
    GUIHelpers::log("SoundEditorPanel: destructor");
    gainValue.removeListener(this);
    panValue.removeListener(this);
    openEndedButton.removeListener(this);
}

void SoundEditorPanel::paint(juce::Graphics& g)
{
    if (!samplerPlugin)
    {
        g.setColour(juce::Colours::black);
        g.drawText("No sound selected", getLocalBounds(), juce::Justification::centred);
    }
}

void SoundEditorPanel::resized()
{
    auto bounds = getLocalBounds();

    if (samplerPlugin)
    {
        auto topSliderBounds = bounds.removeFromTop(80);
        gainSlider->setBounds(topSliderBounds.removeFromLeft(topSliderBounds.getWidth() / 3));
        panSlider->setBounds(topSliderBounds.removeFromLeft(topSliderBounds.getWidth() / 2));

        auto openEndedBounds = topSliderBounds;
        openEndedLabel.setBounds(openEndedBounds.removeFromTop(openEndedBounds.getHeight() / 2));
        auto center = openEndedBounds.getCentre();
        openEndedBounds.setWidth(openEndedBounds.getHeight());
        openEndedBounds.setCentre(center);
        openEndedBounds.reduce(openEndedBounds.getHeight() /4, openEndedBounds.getHeight() /4);
        openEndedButton.setBounds(openEndedBounds);

        // Thumbnail takes remaining space (no more range sliders)
        m_thumbnail->setBounds(bounds);

        // Update markers if needed (after layout is complete)
        if (m_markersNeedUpdate && m_thumbnail != nullptr)
        {
            m_thumbnail->refreshMarkers();
            m_markersNeedUpdate = false;
        }
    }
}

void SoundEditorPanel::setSound(te::SamplerPlugin* plugin, int index)
{
    if (plugin != nullptr && index != -1)
    {
        samplerPlugin = plugin;
        soundIndex = index;

        gainValue.setValue(samplerPlugin->getSoundGainDb(soundIndex));
        panValue.setValue(samplerPlugin->getSoundPan(soundIndex));

        auto audioFile = samplerPlugin->getSoundFile(soundIndex);

        if (audioFile.isValid())
        {
            openEndedButton.setToggleState(samplerPlugin->isSoundOpenEnded(soundIndex), juce::dontSendNotification);

            m_thumbnail->setFile(audioFile);
            m_thumbnail->setColour(juce::Colours::blue);

            // Update start/end markers via callback
            double startTime = samplerPlugin->getSoundStartTime(soundIndex);
            double endTime = startTime + samplerPlugin->getSoundLength(soundIndex);
            m_thumbnail->setStartEndPositions(startTime, endTime);
            m_markersNeedUpdate = true;
        }
        else
        {
            m_thumbnail->setFile(te::AudioFile(m_edit.engine, {}));
            m_thumbnail->clearStartEndMarkers();
        }
    }
    else
    {
        samplerPlugin = nullptr;
        soundIndex = -1;
        m_thumbnail->setFile(te::AudioFile(m_edit.engine, {}));
        m_thumbnail->clearStartEndMarkers();
    }

    resized();
    repaint();
}


void SoundEditorPanel::valueChanged(juce::Value& value)
{
    if (samplerPlugin && soundIndex != -1)
    {
        if (value.refersToSameSourceAs(gainValue))
        {
            samplerPlugin->setSoundGains(soundIndex, (float)gainValue.getValue(), samplerPlugin->getSoundPan(soundIndex));
        }
        else if (value.refersToSameSourceAs(panValue))
        {
            samplerPlugin->setSoundGains(soundIndex, samplerPlugin->getSoundGainDb(soundIndex), (float)panValue.getValue());
        }
    }
}

void SoundEditorPanel::buttonClicked(juce::Button* button)
{
    if (samplerPlugin && soundIndex != -1)
    {
        if (button == &openEndedButton)
        {
            samplerPlugin->setSoundOpenEnded(soundIndex, openEndedButton.getToggleState());
        }
    }
}
