#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"

class PluginComponent : public Component
{
public:
    PluginComponent (EditViewState&, te::Plugin::Ptr, juce::Colour);
    ~PluginComponent();

    void paint (Graphics&) override;
    void mouseDown (const MouseEvent&) override;
    void mouseDrag (const MouseEvent&) override;
    void mouseUp (const MouseEvent&) override;

    void resized() override;

    int getNeededWidthFactor() { return m_neededWidthFactor;}
    void setNeededWidthFactor(int wf){ m_neededWidthFactor = wf; }
    te::Plugin::Ptr getPlugin()
    {
        return plugin;
    }

private:
    juce::Label name;
    int m_headerWidth {20};
    int m_neededWidthFactor {1};
    EditViewState& editViewState;
    te::Plugin::Ptr plugin;
    juce::Colour m_trackColour;

    bool m_clickOnHeader {false};
};
