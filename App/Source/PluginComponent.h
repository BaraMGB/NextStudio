#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"

class PluginComponent : public Component
{
public:
    PluginComponent (EditViewState&, te::Plugin::Ptr);
    ~PluginComponent();

    void paint (Graphics& g) override;
    void mouseDown (const MouseEvent& e) override;
    void resized() override;

    int getNeededWidthFactor() { return m_neededWidthFactor;}
    void setNeededWidthFactor(int wf){ m_neededWidthFactor = wf; }

private:
    juce::Label name;
    int m_neededWidthFactor {1};
    EditViewState& editViewState;
    te::Plugin::Ptr plugin;
};
