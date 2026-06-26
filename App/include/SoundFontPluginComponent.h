#pragma once

#include "PluginViewComponent.h"
#include "SoundFontPlugin.h"
#include "NonAutomatableParameter.h"

class SoundFontPluginComponent
    : public PluginViewComponent
    , private juce::Button::Listener
    , private juce::ComboBox::Listener
    , private juce::Value::Listener
    , private juce::ValueTree::Listener
{
public:
    SoundFontPluginComponent(EditViewState &evs, te::Plugin::Ptr p);
    ~SoundFontPluginComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    int getNeededWidth() override { return 3; }

    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    void buttonClicked(juce::Button *button) override;
    void comboBoxChanged(juce::ComboBox *comboBoxThatHasChanged) override;
    void valueChanged(juce::Value &value) override;

    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree &) override {}

    void refreshFromPlugin();
    void chooseSoundFontFile();

    SoundFontPlugin *m_soundFontPlugin = nullptr;
    bool m_isRefreshingUi = false;

    juce::Value m_gainValue;
    juce::Label m_fileLabel;
    juce::Label m_statusLabel;
    juce::Label m_presetLabel;
    juce::ComboBox m_presetBox;
    juce::TextButton m_loadButton{"Load SF2"};
    juce::TextButton m_panicButton{"Panic"};
    std::unique_ptr<NonAutomatableParameterComponent> m_gainKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFontPluginComponent)
};
