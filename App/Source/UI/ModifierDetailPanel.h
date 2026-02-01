/*
  ==============================================================================

    ModifierDetailPanel.h
    Created: 27 Jan 2026
    Author:  Gemini

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Utilities/EditViewState.h"
#include "Plugins/PluginComponent.h"

namespace te = tracktion_engine;

class ModifierDetailPanel : public juce::Component
{
public:
    ModifierDetailPanel(EditViewState &evs);
    ~ModifierDetailPanel() override;

    void setModifier(te::Modifier::Ptr mod);
    void paint(juce::Graphics &g) override;
    void resized() override;

private:
    EditViewState &m_evs;
    std::unique_ptr<juce::Component> m_view;
    juce::Label m_placeholderLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModifierDetailPanel)
};
