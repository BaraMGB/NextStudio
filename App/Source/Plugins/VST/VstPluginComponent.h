/*
  ==============================================================================

    VstPluginComponent.h
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "UI/Controls/ParameterComponent.h"
#include "Utilities/EditViewState.h"
#include <JuceHeader.h>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

class VstPluginComponent
    : public PluginViewComponent
    , private te::AutomatableParameter::Listener
{
public:
    VstPluginComponent(EditViewState &, te::Plugin::Ptr);
    ~VstPluginComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent &) override {}

    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

    int getNeededWidth() override { return 3; }

private:
    void curveHasChanged(te::AutomatableParameter &) override {}

    void parameterChanged(te::AutomatableParameter &param, float /*newValue*/) override;
    std::unique_ptr<ParameterComponent> m_lastChangedParameterComponent;
    juce::Viewport m_viewPort;
    juce::Component m_pluginListComponent;
    juce::OwnedArray<ParameterComponent> m_parameters;
    te::Plugin::Ptr m_plugin;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VstPluginComponent)
};
