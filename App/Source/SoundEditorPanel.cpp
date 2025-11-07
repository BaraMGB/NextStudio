#include "SoundEditorPanel.h"

SoundEditorPanel::SoundEditorPanel(te::Edit& edit)
    : m_edit(edit)
{
    GUIHelpers::log("SoundEditorPanel: constructor");
    addAndMakeVisible(gainSlider);
    gainSlider.setRange(-48.0, 48.0, 0.1);
    gainSlider.addListener(this);

    addAndMakeVisible(panSlider);
    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.addListener(this);

    addAndMakeVisible(startSlider);
    startSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    startSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    startSlider.addListener(this);

    addAndMakeVisible(endSlider);
    endSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    endSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    endSlider.addListener(this);

    m_thumbnail = std::make_unique<SampleDisplay>(m_edit.getTransport());
    addAndMakeVisible(*m_thumbnail);
}

SoundEditorPanel::~SoundEditorPanel()
{
    GUIHelpers::log("SoundEditorPanel: destructor");
    gainSlider.removeListener(this);
    panSlider.removeListener(this);
    startSlider.removeListener(this);
    endSlider.removeListener(this);
}

void SoundEditorPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::lightgrey);

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
        auto topSliderBounds = bounds.removeFromTop(50);
        gainSlider.setBounds(topSliderBounds.removeFromLeft(topSliderBounds.getWidth() / 2));
        panSlider.setBounds(topSliderBounds);

        auto rangeSliderBounds = bounds.removeFromTop(50);
        startSlider.setBounds(rangeSliderBounds.removeFromLeft(rangeSliderBounds.getWidth() / 2));
        endSlider.setBounds(rangeSliderBounds);

        m_thumbnail->setBounds(bounds);
    }
}

void SoundEditorPanel::setSound(te::SamplerPlugin* plugin, int index)
{
    samplerPlugin = plugin;
    soundIndex = index;

    if (samplerPlugin && soundIndex != -1)
    {
        gainSlider.setValue(samplerPlugin->getSoundGainDb(soundIndex));
        panSlider.setValue(samplerPlugin->getSoundPan(soundIndex));

        auto audioFile = samplerPlugin->getSoundFile(soundIndex);

        if (audioFile.isValid())
        {
            auto fileLength = audioFile.getLengthInSamples() / audioFile.getSampleRate();
            startSlider.setRange(0.0, fileLength, 0.001);
            startSlider.setValue(samplerPlugin->getSoundStartTime(soundIndex));
            endSlider.setRange(0.0, fileLength, 0.001);
            endSlider.setValue(samplerPlugin->getSoundStartTime(soundIndex) + samplerPlugin->getSoundLength(soundIndex));

            m_thumbnail->setFile(audioFile);
            m_thumbnail->setColour(juce::Colours::blue);
        }
        else
        {
            m_thumbnail->setFile(te::AudioFile(m_edit.engine, {}));
        }
    }
    else
    {
        samplerPlugin = nullptr;
        soundIndex = -1;
        m_thumbnail->setFile(te::AudioFile(m_edit.engine, {}));
    }

    resized();
    repaint();
}

void SoundEditorPanel::sliderValueChanged(juce::Slider* slider)
{
    if (samplerPlugin && soundIndex != -1)
    {
        if (slider == &gainSlider)
        {
            samplerPlugin->setSoundGains(soundIndex, (float)gainSlider.getValue(), samplerPlugin->getSoundPan(soundIndex));
        }
        else if (slider == &panSlider)
        {
            samplerPlugin->setSoundGains(soundIndex, samplerPlugin->getSoundGainDb(soundIndex), (float)panSlider.getValue());
        }
        else if (slider == &startSlider || slider == &endSlider)
        {
            auto start = startSlider.getValue();
            auto end = endSlider.getValue();
            if (end < start)
                end = start;
            samplerPlugin->setSoundExcerpt(soundIndex, start, end - start);
        }
    }
}
