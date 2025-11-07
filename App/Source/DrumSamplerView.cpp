#include "DrumSamplerView.h"

DrumSamplerView::DrumSamplerView(te::SamplerPlugin& plugin)
    : m_edit(plugin.edit), m_plugin(plugin)
{
    GUIHelpers::log("DrumSamplerView: constructor");

    if (auto sampler = dynamic_cast<te::SamplerPlugin*> (&plugin))
    {
        if (sampler->getNumSounds() == 0)
        {
            for (int i = 0; i < 16; ++i)
                sampler->addSound("", "Empty", 0.0, 0.0, 0.0);
        }
    }
    m_drumPadComponent = std::make_unique<DrumPadComponent>(m_plugin);
    addAndMakeVisible(*m_drumPadComponent);

    m_soundEditorPanel = std::make_unique<SoundEditorPanel>(m_edit);
    addAndMakeVisible(*m_soundEditorPanel);

    m_drumPadComponent->onPadClicked = [this, &plugin](int padIndex)
    {
        int soundIndex = m_drumPadComponent->getSoundIndexForPad(padIndex); 
        m_soundEditorPanel->setSound(&plugin, soundIndex); 
    };

    m_drumPadComponent->buttonDown(15);
}

DrumSamplerView::~DrumSamplerView()
{
    GUIHelpers::log("DrumSamplerView: destructor");
}

void DrumSamplerView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey.darker());
}

void DrumSamplerView::resized()
{
    auto area = getLocalBounds();

    int padWidth = (int)(area.getWidth() * 0.40);

    m_drumPadComponent->setBounds(area.removeFromLeft(padWidth));
    m_soundEditorPanel->setBounds(area);
}

int DrumSamplerView::getNeededWidth() const
{
    return 6; 
}
