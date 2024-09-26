
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
    , m_loopBtn("Loop", juce::DrawableButton::ButtonStyle::ImageOnButtonBackground)
{
    m_avs.m_applicationStateValueTree.addListener(this);

    m_isSync = std::make_unique<bool>(true);
    m_volumeSlider = std::make_unique<juce::Slider>();
    m_volumeSlider->setRange(.0f, 1.0f);
    m_volumeSlider->getValueObject().referTo(m_avs.m_previewSliderPos.getPropertyAsValue());
    m_volumeSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    m_volumeSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, false);
    m_volumeSlider->addListener(this);
    m_volumeLabel.setText(GUIHelpers::translate("Volume", m_avs), juce::dontSendNotification);
    addAndMakeVisible(m_volumeSlider.get());

    addAndMakeVisible(m_fileName);
    addAndMakeVisible(m_volumeLabel);
    addAndMakeVisible(m_lenghtLabel);
    m_fileName.setJustificationType(juce::Justification::left);

    Helpers::addAndMakeVisible(*this, {&m_playBtn, &m_stopBtn,&m_loopBtn, &m_syncTempoBtn});

    GUIHelpers::setDrawableOnButton(m_playBtn, BinaryData::play_svg, juce::Colour(0xffffffff));
    GUIHelpers::setDrawableOnButton(m_stopBtn, BinaryData::stop_svg, juce::Colour(0xffffffff));
    GUIHelpers::setDrawableOnButton(m_syncTempoBtn, BinaryData::timeShiftCursor_svg , juce::Colour(0xffffffff));
    GUIHelpers::setDrawableOnButton(m_loopBtn, BinaryData::cached_svg, juce::Colour(0xffffffff));
    m_stopBtn.setTooltip(GUIHelpers::translate("Stop playing", m_avs));
    m_playBtn.setTooltip(GUIHelpers::translate("Play sample", m_avs));
    m_loopBtn.setTooltip(GUIHelpers::translate("Loop", m_avs));
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
    m_loopBtn.onClick = [this]
    {
        m_avs.m_previewLoop = !m_avs.m_previewLoop;
    };
}
SamplePreviewComponent::~SamplePreviewComponent()
{
    m_volumeSlider->removeListener(this);
    m_avs.m_applicationStateValueTree.removeListener(this);
}
void SamplePreviewComponent::paint(juce::Graphics &g) 
{
    auto area = getLocalBounds();

    g.setColour (m_avs.getBorderColour());
    g.fillRect(area.removeFromTop(1));
    g.setColour (m_avs.getMenuBackgroundColour());
    g.fillRect (area);

    g.setColour(m_avs.getBorderColour());
    g.drawHorizontalLine(m_fileName.getBottom(), 0, getWidth());
    g.drawHorizontalLine(m_fileName.getBottom() + 90 + 1, 0, getWidth());
    g.drawHorizontalLine(m_playBtn.getBottom() + 3, 0, getWidth());
    if (m_previewEdit)
    {
        g.setColour(m_avs.getSamplesColour().withAlpha(0.1f));
        g.fillRect(m_thumbnail->getBounds());
    }
}

void SamplePreviewComponent::resized() 
{
    auto area = getLocalBounds();
    area.removeFromTop(2);

    // Set height for file name label
    auto labelHeight = 30;
    auto labelRect = area.removeFromTop(labelHeight);
    m_fileName.setBounds(labelRect.removeFromLeft(150));
    m_lenghtLabel.setBounds(labelRect);

    // Set height for thumbnail
    auto thumbnailHeight = 90;
    auto thumbRect = area.removeFromTop(thumbnailHeight);
    thumbRect.reduce(4, 4);
    if (m_previewEdit)
        m_thumbnail->setBounds(thumbRect);



    // Remaining area for buttons
    auto buttonMenu = area.removeFromTop(40);
    auto buttonWidth = 60; // Fixed button height

    // Define gaps
    auto gapX = 5, gapY = 5;

    buttonMenu.removeFromBottom(5);
    // Set bounds for buttons
    auto play = buttonMenu.removeFromLeft(buttonWidth).reduced(gapX, gapY);
    auto stop = buttonMenu.removeFromLeft(buttonWidth).reduced(gapX, gapY);
    auto loop = buttonMenu.removeFromLeft(buttonWidth).reduced(gapX, gapY);
    // auto sync = buttonMenu.removeFromLeft(buttonHeight).reduced(gapX, gapY);

    // m_syncTempoBtn.setBounds(sync);
    m_loopBtn.setBounds(loop);
    m_stopBtn.setBounds(stop);
    m_playBtn.setBounds(play);

    // Set height for volume slider
    auto volumeSliderHeight = 30;
    auto volumeSliderRect = area.removeFromTop(volumeSliderHeight);
    auto volumeLabelRect = volumeSliderRect.removeFromLeft(60);
    volumeSliderRect = volumeSliderRect.removeFromLeft(100);
    volumeSliderRect.reduce(4, 4);
    m_volumeLabel.setBounds(volumeLabelRect);
    m_volumeSlider->setBounds(volumeSliderRect);
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

    auto lenght = audioFile.getLength();
    auto lenghtStr = juce::String::formatted("%.2f",lenght);

    m_lenghtLabel.setText( lenghtStr + "s", juce::dontSendNotification);
    m_lenghtLabel.setFont(10);
    m_lenghtLabel.setJustificationType(juce::Justification::centredRight);

    m_file = file;
    m_fileName.setText(m_file.getFileName(), juce::sendNotification);
    m_previewEdit = te::Edit::createEditForPreviewingFile (m_engine, file, &m_edit, m_syncTempo, false, m_isSync.get(), juce::ValueTree());

    auto& sliderpos =  m_avs.m_previewSliderPos;
    if (m_previewEdit)
        m_previewEdit->getMasterSliderPosParameter ()->setParameter(sliderpos , juce::dontSendNotification);
    //is needed because, preview edit don't play the attacs of a sample if started. idky
    te::insertSpaceIntoEdit(*m_previewEdit, {tracktion::TimePosition::fromSeconds(0.0), tracktion::TimeDuration::fromSeconds(0.05)});
    m_previewEdit->getTransport().setLoopRange({tracktion::TimePosition::fromSeconds(0.05), tracktion::TimeDuration::fromSeconds(lenght)});
    updateEngineLooping();

    auto colour = *m_isSync ? m_avs.getPrimeColour() : m_avs.getTextColour();
    m_fileName.setColour(juce::Label::textColourId, colour);
    
    m_thumbnail = std::make_unique<SampleView>(m_previewEdit->getTransport ());
    m_thumbnail->setFile (audioFile);
    m_thumbnail->setColour(m_avs.getSamplesColour());
    addAndMakeVisible (*m_thumbnail);
    resized ();

    return true;
}
void SamplePreviewComponent::updateButtonColours()
{
    if (m_previewEdit)
    {
        auto sync = *m_isSync && m_syncTempo;
        auto isPlaying = m_previewEdit->getTransport().isPlaying();
        auto playBtnColour = isPlaying ? juce::Colour(0xff959595) : juce::Colour(0xff474747);
        auto stopBtnColour = isPlaying ? juce::Colour(0xff959515) : juce::Colour(0xff474747);
        auto syncBtnColour = sync ? juce::Colour(0xff959515) : juce::Colour(0xff474747);
        auto loop = m_avs.m_previewLoop ? m_avs.getPrimeColour().withAlpha(.3f) : m_avs.getMainFrameColour();
        // m_stopBtn.setColour(juce::TextButton::buttonColourId,stopBtnColour );
        m_playBtn.setColour(juce::TextButton::buttonColourId, playBtnColour);
        m_syncTempoBtn.setColour(juce::TextButton::buttonColourId, syncBtnColour);
        m_loopBtn.setColour(juce::TextButton::buttonColourId, loop);
    }
}

void SamplePreviewComponent::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i) 
{
    if (i == IDs::PreviewLoop)
        markAndUpdate(m_updateLooping);
}

void SamplePreviewComponent::handleAsyncUpdate () 
{
    if (compareAndReset (m_updateLooping))
    {
        updateEngineLooping();
        updateButtonColours();
    }
}
void SamplePreviewComponent::updateEngineLooping()
{
    if (m_previewEdit)
        m_previewEdit->getTransport().looping = static_cast<bool>(m_avs.m_previewLoop);
}
