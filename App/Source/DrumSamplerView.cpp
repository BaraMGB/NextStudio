#include "DrumSamplerView.h"

DrumSamplerView::DrumSamplerView(EditViewState& evs, te::Plugin::Ptr plugin)
    : PluginViewComponent(evs, plugin),
    m_edit(plugin->edit), m_plugin(plugin)
{
    GUIHelpers::log("DrumSamplerView: constructor");

    if (auto sampler = dynamic_cast<te::SamplerPlugin*> (plugin.get()))
    {
        if (sampler->getNumSounds() == 0)
        {
            for (int i = 0; i < 16; ++i)
                sampler->addSound("", "Empty", 0.0, 0.0, 0.0);
        }
        m_drumPadComponent = std::make_unique<DrumPadComponent>(*sampler);
        addAndMakeVisible(*m_drumPadComponent);

        m_soundEditorPanel = std::make_unique<SoundEditorPanel>(m_edit);
        addAndMakeVisible(*m_soundEditorPanel);

        m_drumPadComponent->onPadClicked = [this, sampler](int padIndex)
        {
            int soundIndex = m_drumPadComponent->getSoundIndexForPad(padIndex); 
            m_soundEditorPanel->setSound(sampler, soundIndex); 
        };

        m_drumPadComponent->buttonDown(15);
    }
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

int DrumSamplerView::getNeededWidth()
{
    return 6; 
}


// PluginPresetInterface implementation
juce::ValueTree DrumSamplerView::getPluginState()
{
    return m_plugin->state;
}

void DrumSamplerView::restorePluginState(const juce::ValueTree& state)
{
    m_plugin->restorePluginStateFromValueTree(state);
}

juce::String DrumSamplerView::getPresetSubfolder()
{
    return "DrumSampler";
}

juce::String DrumSamplerView::getPluginTypeName()
{
    return m_plugin->getPluginType();
}

ApplicationViewState& DrumSamplerView::getApplicationViewState()
{
    return m_editViewState.m_applicationState;
}
