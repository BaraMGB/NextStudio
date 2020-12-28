#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"
#include "Utilities.h"
#include "PluginComponent.h"
#include "PluginMenu.h"
#include "PianoRollEditorComponent.h"
#include "PluginRackComponent.h"


namespace te = tracktion_engine;

class LowerRangeComponent : public juce::Component
                          , public juce::ChangeListener
{
public:
    LowerRangeComponent (EditViewState& evs);
     ~LowerRangeComponent();

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void paint (juce::Graphics& g) override;
    void resized () override;

    void showPluginRack(te::Track *);
    void showPianoRoll(tracktion_engine::MidiClip *);

private:

    EditViewState& m_editViewState;

    tracktion_engine::Track::Ptr m_pointedTrack{nullptr};
    te::MidiClip * m_pointedClip{nullptr};
    juce::OwnedArray<PluginRackComponent> m_pluginRackComps;
    std::unique_ptr<PianoRollComponent> m_pianoRoll;
    const int m_splitterHeight {10};

};
