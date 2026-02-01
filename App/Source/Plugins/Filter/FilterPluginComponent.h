/*
  ==============================================================================

    FilterPluginComponent.h
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "Utilities/EditViewState.h"
#include "UI/Controls/AutomatableParameter.h"
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

class FilterPluginComponent
    : public PluginViewComponent
    , private te::ValueTreeAllEventListener
{
public:
    FilterPluginComponent(EditViewState &, te::Plugin::Ptr);
    ~FilterPluginComponent() override { m_plugin->state.removeListener(this); }

    void paint(juce::Graphics &) override;
    void resized() override;
    int getNeededWidth() override { return 2; }

    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    void updateLabel(juce::UndoManager &um);
    void valueTreeChanged() override {}
    void valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i) override
    {
        if (i == te::IDs::frequency)
            m_freqPar->updateLabel();
    }
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}

    std::unique_ptr<AutomatableParameterComponent> m_freqPar;
    juce::ToggleButton m_modeButton;
    juce::Label m_modeLabel;
    
    // Using te::LowPassPlugin pointer might require casting or ensure it's correct type passed
    // But standard PluginViewComponent holds m_plugin as te::Plugin::Ptr
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterPluginComponent)
};
