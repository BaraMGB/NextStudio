#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"
#include "Utilities.h"
#include "PluginComponent.h"
#include "PluginMenu.h"
#include "PianoRollEditor.h"
#include "RackView.h"
#include "TrackHeadComponent.h"
#include "TimelineOverlayComponent.h"


namespace te = tracktion_engine;

class SplitterComponent : public juce::Component
{
public:
    explicit SplitterComponent(EditViewState&);
    void mouseMove(const juce::MouseEvent &event) override;
    void mouseEnter(const juce::MouseEvent &event) override;
    void mouseExit(const juce::MouseEvent &event) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;
    void paint(juce::Graphics& g) override;

private:
    EditViewState & m_editViewState;
    int m_pianorollHeightAtMousedown{};
    double m_cachedPianoNoteNum{};
    bool m_isHovering{false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SplitterComponent)
};

class LowerRangeComponent : public juce::Component
                          , public te::ValueTreeAllEventListener
{
public:
    explicit LowerRangeComponent (EditViewState& evs);
     ~LowerRangeComponent() override;

    void paint (juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized () override;

    void showRackView(const tracktion_engine::Track::Ptr);
    void showPianoRoll(const te::Track::Ptr);

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

    EditViewState& m_evs;

    RackView m_rackView;
    PianoRollEditor m_pianoRollEditor;
    SplitterComponent m_splitter;
    const float m_splitterHeight {10.f};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LowerRangeComponent)
};
