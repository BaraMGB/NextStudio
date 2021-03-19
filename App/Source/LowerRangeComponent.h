#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"
#include "Utilities.h"
#include "PluginComponent.h"
#include "PluginMenu.h"
#include "PianoRollEditorComponent.h"
#include "PluginRackComponent.h"
#include "ClipComponent.h"
#include "MidiClipComponent.h"
#include "TrackHeadComponent.h"
#include "TimelineOverlayComponent.h"


namespace te = tracktion_engine;

class SplitterComponent : public juce::Component
{
public:
    SplitterComponent(EditViewState&);
    void mouseMove(const juce::MouseEvent &event) override;
    void mouseEnter(const juce::MouseEvent &event)override;
    void mouseExit(const juce::MouseEvent &event)override;
    void mouseDown(const juce::MouseEvent &event)override;
    void mouseDrag(const juce::MouseEvent &event)override;
    void mouseUp(const juce::MouseEvent &event)override;
    void  paint(juce::Graphics& g);
private:
    EditViewState & m_editViewState;
    int m_pianorollHeightAtMousedown;
    int m_mousedownPosYatMousdown;
    bool m_isHovering{false};
};

class LowerRangeComponent : public juce::Component
                          , public juce::ChangeListener
                          , public te::ValueTreeAllEventListener
{
public:
    LowerRangeComponent (EditViewState& evs);
     ~LowerRangeComponent() override;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void paint (juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized () override;

    void removePluginRackwithTrack(te::Track::Ptr track);
    void showPluginRack(tracktion_engine::Track::Ptr);
    void showPianoRoll(te::Track::Ptr);
    void hideAll();
    void addPianoRollEditor(PianoRollComponent * pianoroll);
    void addPluginRackComp (PluginRackComponent * pluginrack);

    void clearPluginRacks()
    {
        m_pluginRackComps.clear ();
    }
private:

    void valueTreeChanged() override {}
    void valueTreePropertyChanged (juce::ValueTree&
                                   , const juce::Identifier&) override;
    void valueTreeChildAdded (juce::ValueTree&
                              , juce::ValueTree&) override;
    void valueTreeChildRemoved (juce::ValueTree&
                                , juce::ValueTree&
                                , int) override;
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override;


    EditViewState& m_editViewState;

    juce::OwnedArray<PluginRackComponent> m_pluginRackComps;
    PianoRollComponent m_pianoRollEditor;
    std::unique_ptr<TimelineOverlayComponent> m_timelineOverlay{nullptr};
    SplitterComponent m_splitter;
    const int m_splitterHeight {10};


};
