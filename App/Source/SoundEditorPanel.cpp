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
#include "ApplicationViewState.h"
#include "Utilities.h"

SoundEditorPanel::SoundEditorPanel(te::SamplerPlugin& plugin, te::Edit& edit, ApplicationViewState& appViewState)
    : m_edit(edit), m_appViewState(appViewState), m_samplerPlugin(plugin)
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

    m_thumbnail = std::make_unique<SampleDisplay>(m_edit.getTransport(), m_appViewState);
    addAndMakeVisible(*m_thumbnail);

    // Set up callback for marker position changes
    m_thumbnail->onMarkerPositionChanged = [this](double start, double end)
    {
        if (soundIndex != -1)
        {
            double length = end - start;
            m_samplerPlugin.setSoundExcerpt(soundIndex, start, length);
        }
    };

    addAndMakeVisible(openEndedButton);
    openEndedButton.addListener(this);
    openEndedButton.setColour(juce::ToggleButton::textColourId, m_appViewState.getTextColour());
    openEndedButton.setColour(juce::ToggleButton::tickColourId, m_appViewState.getPrimeColour());


    addAndMakeVisible(openEndedLabel);
    openEndedLabel.setText("Open Ended", juce::dontSendNotification);
    openEndedLabel.setColour(juce::Label::textColourId, m_appViewState.getTextColour());
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
    g.fillAll(m_appViewState.getBackgroundColour2());
    auto bounds = getLocalBounds();
    bounds.reduce(2, 2);
    g.setColour(m_appViewState.getBorderColour());
    // g.fillRoundedRectangle(bounds.toFloat(), 10);

    auto headerArea = bounds.removeFromTop(20);
    auto waveformArea = bounds.removeFromTop(bounds.getHeight() / 2);
    auto controlsArea = bounds;
    controlsArea.removeFromTop(5);


    GUIHelpers::drawRoundedRectWithSide(g, headerArea.getUnion(waveformArea).toFloat() , 10, true, true, false, false);
    GUIHelpers::drawRoundedRectWithSide(g, controlsArea.toFloat(), 10, true, true, true, true);

    headerArea.reduce(1, 1);
    waveformArea.reduce(1, 1);
    controlsArea.reduce(1, 1);

    auto colour = m_samplerPlugin.getOwnerTrack()->getColour();

    // Draw header
    g.setColour(colour);
    GUIHelpers::drawRoundedRectWithSide(g, headerArea.toFloat(), 6.0f, true, true, false, false);

    auto headerTextColour = colour.getBrightness() > 0.8f
             ? juce::Colour(0xff000000)
             : juce::Colour(0xffffffff);
    g.setColour(headerTextColour);
    g.setFont(14.0f);
    if (m_audioFile != nullptr && m_audioFile->isValid())
    {

        g.drawText(m_audioFile->getFile().getFileName(),
                   headerArea.withTrimmedLeft(10),
                   juce::Justification::centredLeft);

        g.drawText(juce::String(m_audioFile->getLength(), 2) + " s",
                   headerArea.withTrimmedRight(10),
                   juce::Justification::centredRight);
    }
    // Draw waveform background
    g.setColour(m_appViewState.getBackgroundColour1());
    g.fillRect(waveformArea);



    g.setColour(m_appViewState.getBackgroundColour1());
    GUIHelpers::drawRoundedRectWithSide(g, controlsArea.toFloat(), 10.0f, true, true, true, true);
}

void SoundEditorPanel::resized()
{
    auto bounds = getLocalBounds();
    bounds.reduce(3, 3);
    auto headerArea = bounds.removeFromTop(20);

    auto thumbnailArea = bounds.removeFromTop(bounds.getHeight() / 2);
    thumbnailArea.reduce(2, 0);
    m_thumbnail->setBounds(thumbnailArea);

    auto controlsArea = bounds;
    controlsArea.removeFromTop(5);
    controlsArea.reduce(10, 10);

    auto openEndedArea = controlsArea.removeFromRight(controlsArea.getWidth() / 4);
    controlsArea.removeFromRight(controlsArea.getWidth() / 3);
    auto sliderArea = controlsArea;
    // Sliders
    auto gainArea = sliderArea.removeFromLeft(sliderArea.getWidth() / 2);
    auto panArea = sliderArea;
    gainSlider->setBounds(gainArea);
    panSlider->setBounds(panArea);

    // Open Ended
    openEndedLabel.setJustificationType(juce::Justification::centred);
    openEndedLabel.setBounds(openEndedArea.removeFromTop(30));

    auto buttonBounds = openEndedArea;
    auto center = buttonBounds.getCentre();
    auto buttonSize = juce::jmin(buttonBounds.getWidth(), buttonBounds.getHeight()) * 0.5f;
    buttonBounds.setSize(buttonSize, buttonSize);
    buttonBounds.setCentre(center);
    openEndedButton.setBounds(buttonBounds);

    // Update markers if needed (after layout is complete)
    if (m_markersNeedUpdate && m_thumbnail != nullptr)
    {
        m_thumbnail->refreshMarkers();
        m_markersNeedUpdate = false;
    }
}

void SoundEditorPanel::setSound(int index)
{
    if (m_thumbnail != nullptr)
    {
        if (index != -1)
        {
            soundIndex = index;

            gainValue.setValue(m_samplerPlugin.getSoundGainDb(soundIndex));
            panValue.setValue(m_samplerPlugin.getSoundPan(soundIndex));

            auto audioFile = m_samplerPlugin.getSoundFile(soundIndex);

            if (audioFile.isValid())
            {
                m_audioFile = std::make_unique<te::AudioFile>(audioFile);
                openEndedButton.setToggleState(m_samplerPlugin.isSoundOpenEnded(soundIndex), juce::dontSendNotification);

                m_thumbnail->setFile(audioFile);
                m_thumbnail->setColour(m_appViewState.getTextColour());


                // Update start/end markers via callback
                double startTime = m_samplerPlugin.getSoundStartTime(soundIndex);
                double endTime = startTime + m_samplerPlugin.getSoundLength(soundIndex);
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
            soundIndex = -1;
            m_thumbnail->setFile(te::AudioFile(m_edit.engine, {}));
            m_thumbnail->clearStartEndMarkers();
        }
    }
    resized();
    repaint();
}


void SoundEditorPanel::valueChanged(juce::Value& value)
{
    if (soundIndex != -1)
    {
        if (value.refersToSameSourceAs(gainValue))
        {
            m_samplerPlugin.setSoundGains(soundIndex, (float)gainValue.getValue(), m_samplerPlugin.getSoundPan(soundIndex));
        }
        else if (value.refersToSameSourceAs(panValue))
        {
            m_samplerPlugin.setSoundGains(soundIndex, m_samplerPlugin.getSoundGainDb(soundIndex), (float)panValue.getValue());
        }
    }
}

void SoundEditorPanel::buttonClicked(juce::Button* button)
{
    if (soundIndex != -1)
    {
        if (button == &openEndedButton)
        {
            m_samplerPlugin.setSoundOpenEnded(soundIndex, openEndedButton.getToggleState());
        }
    }
}
