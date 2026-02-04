#include "Plugins/DrumSampler/DrumSamplerView.h"

DrumSamplerView::DrumSamplerView(EditViewState &evs, te::SamplerPlugin &sampler)
    : PluginViewComponent(evs, sampler),
      m_edit(sampler.edit),
      m_sampler(sampler),
      m_drumPadComponent(sampler, evs.m_applicationState),
      m_soundEditorPanel(sampler, sampler.edit, evs.m_applicationState)
{
    GUIHelpers::log("DrumSamplerView: constructor");

    // Ensure the sampler has the expected 16 sounds if it's currently empty
    if (m_sampler.getNumSounds() == 0)
    {
        GUIHelpers::log("DrumSamplerView: Sampler is empty, applying factory default state.");
        restorePluginState(getFactoryDefaultState());
    }

    addAndMakeVisible(m_drumPadComponent);
    addAndMakeVisible(m_soundEditorPanel);

    m_drumPadComponent.onPadClicked = [this, &sampler](int padIndex)
    {
        int soundIndex = m_drumPadComponent.getSoundIndexForPad(padIndex);
        m_soundEditorPanel.setSound(soundIndex);
    };
}

DrumSamplerView::~DrumSamplerView() { GUIHelpers::log("DrumSamplerView: destructor"); }

void DrumSamplerView::paint(juce::Graphics &g) { g.fillAll(m_editViewState.m_applicationState.getBackgroundColour1().darker()); }

void DrumSamplerView::resized()
{
    auto area = getLocalBounds();

    int padWidth = (int)(area.getWidth() * 0.40);
    m_drumPadComponent.setBounds(area.removeFromLeft(padWidth));
    m_soundEditorPanel.setBounds(area);
}

int DrumSamplerView::getNeededWidth() { return 6; }

// PluginPresetInterface implementation
juce::ValueTree DrumSamplerView::getPluginState() { return m_sampler.state.createCopy(); }

void DrumSamplerView::restorePluginState(const juce::ValueTree &state)
{
    m_sampler.restorePluginStateFromValueTree(state);
    m_drumPadComponent.updatePadNames();
}

juce::String DrumSamplerView::getPresetSubfolder() const { return "DrumSampler"; }

juce::String DrumSamplerView::getPluginTypeName() const { return m_sampler.getPluginType(); }

ApplicationViewState &DrumSamplerView::getApplicationViewState() { return m_editViewState.m_applicationState; }

juce::ValueTree DrumSamplerView::getFactoryDefaultState()
{
    juce::ValueTree defaultState("PLUGIN");
    defaultState.setProperty("type", "sampler", nullptr);

    for (int i = 0; i < 16; ++i)
    {
        juce::ValueTree sound("SOUND");
        sound.setProperty("source", "", nullptr);
        sound.setProperty("name", "Empty", nullptr);
        sound.setProperty("startTime", 0.0, nullptr);
        sound.setProperty("length", 0.0, nullptr);

        int midiNote = DrumPadGridComponent::BASE_MIDI_NOTE + i;
        sound.setProperty("keyNote", midiNote, nullptr);
        sound.setProperty("minNote", midiNote, nullptr);
        sound.setProperty("maxNote", midiNote, nullptr);

        sound.setProperty("gainDb", 0.0, nullptr);
        sound.setProperty("pan", 0.0, nullptr);
        sound.setProperty("openEnded", true, nullptr);
        defaultState.addChild(sound, -1, nullptr);
    }
    return defaultState;
}
