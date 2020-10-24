
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "PluginComponent.h"
#include "PluginMenu.h"

namespace te = tracktion_engine;


class PluginRackComponent : public Component,
                             private FlaggedAsyncUpdater,
                             private te::ValueTreeAllEventListener
{
public:
    PluginRackComponent (EditViewState&, te::Track::Ptr);
    ~PluginRackComponent();

    void paint (Graphics& g) override;
    void mouseDown (const MouseEvent& e) override;
    void resized() override;

private:
    void valueTreeChanged() override {}
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override;
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override;

    void handleAsyncUpdate() override;

    void buildPlugins();

    EditViewState& editViewState;
    te::Track::Ptr track;

    TextButton addButton {"+"};
    OwnedArray<PluginComponent> plugins;

    bool updatePlugins = false;
};



class LowerRangeComponent : public Component
                          , public juce::ChangeListener
{
public:
    LowerRangeComponent (EditViewState& evs);
     ~LowerRangeComponent();

    void changeListenerCallback (ChangeBroadcaster*) override;

    void paint (Graphics& g) override;
    void resized () override;

private:

    EditViewState& editViewState;
    OwnedArray<PluginRackComponent> m_footers;

    tracktion_engine::Track * m_pointedTrack;

};
