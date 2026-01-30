/*
  ==============================================================================

    ArpeggiatorPluginComponent.h
    Created: 26 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "../../Components/AutomatableComboBox.h"
#include "../../Components/AutomatableParameter.h"
#include "../../Components/AutomatableSlider.h"
#include "../../PluginViewComponent.h"
#include "ArpeggiatorPlugin.h"

//==============================================================================
class ArpeggiatorPluginComponent
    : public PluginViewComponent
    , private juce::ValueTree::Listener
{
public:
    ArpeggiatorPluginComponent(EditViewState &evs, te::Plugin::Ptr p);
    ~ArpeggiatorPluginComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    int getNeededWidth() override { return 2; }

    // PluginPresetInterface implementation
    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree &) override {}

    ArpeggiatorPlugin *m_arpeggiator = nullptr;

    std::unique_ptr<AutomatableChoiceComponent> m_modeComp;
    std::unique_ptr<AutomatableChoiceComponent> m_rateComp;
    std::unique_ptr<AutomatableChoiceComponent> m_octaveComp;
    std::unique_ptr<AutomatableParameterComponent> m_gateComp;

    juce::Label m_titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpeggiatorPluginComponent)
};
