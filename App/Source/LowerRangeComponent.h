#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"
#include "Utilities.h"
#include "PluginComponent.h"
#include "PluginMenu.h"
#include "PianoRollEditorComponent.h"
#include "PluginRackComponent.h"
#include "ClipComponent.h"
#include "TrackHeadComponent.h"


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

    void showPluginRack(tracktion_engine::Track::Ptr);
    void showPianoRoll(te::Clip::Ptr);
    void addPianoRollEditor(PianoRollComponent * pianoroll);
    void addPluginRackComp (PluginRackComponent * pluginrack);


    void clearPianoRolls()
    {
        m_pianoRolls.clear ();
    }
    void clearPluginRacks()
    {
        m_pluginRackComps.clear ();
    }
private:

    EditViewState& m_editViewState;

    tracktion_engine::Track::Ptr m_pointedTrack{nullptr};
    te::Clip::Ptr m_pointedClip{nullptr};
    juce::OwnedArray<PluginRackComponent> m_pluginRackComps;
    juce::OwnedArray<PianoRollComponent> m_pianoRolls;
    const int m_splitterHeight {10};


};
