
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

#include "PreviewComponent.h"
#include "BinaryData.h"
#include "Utilities.h"

SamplePreviewComponent::SamplePreviewComponent(te::Engine & engine, te::Edit& edit, ApplicationViewState& avs)
    : m_engine(engine)
    , m_edit(edit)
    , m_avs(avs)
    , m_playBtn ("Play/Pause",  juce::DrawableButton::ButtonStyle::ImageOnButtonBackground)
    , m_stopBtn ("Stop", juce::DrawableButton::ButtonStyle::ImageOnButtonBackground)
    , m_syncTempoBtn ("Sync Tempo", juce::DrawableButton::ButtonStyle::ImageOnButtonBackground)
{


    m_isSync = new bool{false};
    m_volumeSlider = std::make_unique<juce::Slider>();
    m_volumeSlider->setRange(-100.0f, 6.0f);
    m_volumeSlider->getValueObject().referTo(m_avs.m_previewSliderPos.getPropertyAsValue());
    m_volumeSlider->setSkewFactorFromMidPoint(1.0f);
    m_volumeSlider->setSliderStyle(juce::Slider::LinearBarVertical);
    m_volumeSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, false);
    addAndMakeVisible(m_volumeSlider.get());

    Helpers::addAndMakeVisible(*this, {&m_playBtn, &m_stopBtn, &m_syncTempoBtn});

    GUIHelpers::setDrawableOnButton(m_playBtn, BinaryData::play_svg, juce::Colour(0xffffffff));
    GUIHelpers::setDrawableOnButton(m_stopBtn, BinaryData::stop_svg, juce::Colour(0xffffffff));
    GUIHelpers::setDrawableOnButton(m_syncTempoBtn, BinaryData::timeShiftCursor_svg , juce::Colour(0xffffffff));
    m_stopBtn.setTooltip(GUIHelpers::translate("Stop playing", m_avs));
    m_playBtn.setTooltip(GUIHelpers::translate("Play sample", m_avs));
    m_syncTempoBtn.setTooltip(GUIHelpers::translate("Sync to song tempo", m_avs));
    m_stopBtn.onClick = [this] 
    {
        if (!m_previewEdit)
            return;

        auto isPlaying = m_previewEdit->getTransport().isPlaying();
        if (isPlaying)
        {
            stop();
        }

        rewind();
        resized();

    };
    m_playBtn.onClick = [this]
    {
        if (!m_previewEdit)
            return;

        auto isPlaying = m_previewEdit->getTransport().isPlaying();
        if (!isPlaying)
        {
            play();
        }
        else
        {
            rewind();
            play();
        }

        resized();
    };
    m_syncTempoBtn.onClick = [this]
    {
        m_syncTempo = !m_syncTempo;
        setFile(m_file); 
        resized();
    };
    addAndMakeVisible(m_fileName);
}

void SamplePreviewComponent::paint(juce::Graphics &g) 
{
    g.setColour (m_avs.getBackgroundColour());
    g.fillRect (getLocalBounds ());
    g.setColour(m_avs.getBorderColour());
    g.drawHorizontalLine(30, 0, getWidth());
    g.drawHorizontalLine(getHeight() - 1, 0, getWidth());

    const int headWidth = 20;
    auto area = getLocalBounds();

    if (m_previewEdit)
    {

        te::AudioFile audioFile (m_previewEdit->engine, m_file);
        if (audioFile.isValid())
        {
            const char* icon = BinaryData::wavetest5_svg;;
            auto colour = m_avs.getPrimeColour();
            GUIHelpers::drawFromSvg (g, icon, colour, {0, 6, 18, 18});
        }
    }

}

void SamplePreviewComponent::resized() 
{
    auto area = getLocalBounds ();
    area.removeFromTop(1);

    auto labelHeight = 30;
    auto thumbnailHeight = (getHeight() - 30) / 2;

    auto thumbRect = area.removeFromTop(thumbnailHeight);
    if (m_previewEdit)
        m_thumbnail->setBounds (thumbRect);
    m_fileName.setBounds(area.removeFromTop(labelHeight));

    auto buttonMenu = area;

    m_volumeSlider->setBounds(buttonMenu.removeFromLeft(buttonMenu.getHeight() ));
    auto buttonwidth = buttonMenu.getHeight() ;

    auto gapX = 1, gapY = 5;
    auto sync = buttonMenu.removeFromLeft (buttonwidth).reduced(gapX, gapY);
    auto stop = buttonMenu.removeFromLeft (buttonwidth).reduced(gapX, gapY);
    auto play = buttonMenu.removeFromLeft (buttonwidth).reduced(gapX, gapY);
    m_syncTempoBtn.setBounds(sync);
    m_stopBtn.setBounds(stop);
    m_playBtn.setBounds(play);

    updateButtonColours();


    repaint();
}

void SamplePreviewComponent::sliderValueChanged(juce::Slider *slider) 
{
    if (slider == m_volumeSlider.get ())
    {
        auto& sliderpos =  m_avs.m_previewSliderPos;
        if (m_previewEdit)
            m_previewEdit->getMasterSliderPosParameter ()->setParameter(sliderpos , juce::dontSendNotification);
    }
}

void SamplePreviewComponent::timerCallback() 
{
    if (m_previewEdit)
    {
        auto& t = m_previewEdit->getTransport();
        if (t.getPosition().inSeconds() > m_previewEdit->getLength().inSeconds())
        {
            t.stop(false, true); 
            rewind();
            resized();
            stopTimer();
        }
        else 
        {
            startTimer(200);
        }
    }
}

void SamplePreviewComponent::play()
{
    if (m_previewEdit)
    {
        auto& ptp = m_previewEdit->getTransport();
        m_previewEdit->dispatchPendingUpdatesSynchronously ();
        ptp.ensureContextAllocated();

        auto sampleLenght = m_previewEdit->getLength().inSeconds() + 0.5;
        ptp.play(false);
        startTimer(200);
        updateButtonColours();
    }
}

void SamplePreviewComponent::stop()
{
    if (m_previewEdit)
    {   
        m_previewEdit->getTransport().stop(false,false, true, false);
        stopTimer();
    }   
}

void SamplePreviewComponent::rewind()
{
    if (m_previewEdit) 
    {
        auto& ptp = m_previewEdit->getTransport();
        ptp.setPosition(tracktion::TimePosition::fromSeconds(0.0));
    }
}
 
bool SamplePreviewComponent::setFile(const juce::File& file)
{
    if (file.isDirectory ())
        return false;

    te::AudioFile audioFile (m_engine, file);
    if (!audioFile.isValid())
        return false;

    float oldVolume = -1;
    if (m_previewEdit)
        oldVolume = m_previewEdit->getMasterVolumePlugin ()->volume;

    m_file = file;
    m_fileName.setText(m_file.getFileName(), juce::sendNotification);
    m_previewEdit = te::Edit::createEditForPreviewingFile (m_engine, file, &m_edit, m_syncTempo, false, m_isSync, juce::ValueTree());

    auto& sliderpos =  m_avs.m_previewSliderPos;
    if (m_previewEdit)
        m_previewEdit->getMasterSliderPosParameter ()->setParameter(sliderpos , juce::dontSendNotification);
    //is needed because, preview edit don't play the attacs of a sample if started. idky
    te::insertSpaceIntoEdit(*m_previewEdit, {tracktion::TimePosition::fromSeconds(0.0), tracktion::TimeDuration::fromSeconds(0.05)});

    auto colour = *m_isSync ? m_avs.getPrimeColour() : m_avs.getTextColour();
    m_fileName.setColour(juce::Label::textColourId, colour);
    
    m_thumbnail = std::make_unique<SampleView>(m_previewEdit->getTransport ());
    m_thumbnail->setFile (audioFile);
    addAndMakeVisible (*m_thumbnail);
    resized ();

    return true;
}
void SamplePreviewComponent::updateButtonColours()
{
    if (m_previewEdit)
    {
        auto isPlaying = m_previewEdit->getTransport().isPlaying();
        auto playBtnColour = isPlaying ? juce::Colour(0xff959595) : juce::Colour(0xff474747);
        auto stopBtnColour = isPlaying ? juce::Colour(0xff959515) : juce::Colour(0xff474747);
        auto syncBtnColour = m_syncTempo ? juce::Colour(0xff959515) : juce::Colour(0xff474747);
        // m_stopBtn.setColour(juce::TextButton::buttonColourId,stopBtnColour );
        m_playBtn.setColour(juce::TextButton::buttonColourId, playBtnColour);
        m_syncTempoBtn.setColour(juce::TextButton::buttonColourId, syncBtnColour);
    }
}
