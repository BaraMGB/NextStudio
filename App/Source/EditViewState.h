#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
namespace te = tracktion_engine;

namespace IDs
{
    #define DECLARE_ID(name)  const juce::Identifier name (#name);
    DECLARE_ID (EDITVIEWSTATE)
    DECLARE_ID (showGlobalTrack)
    DECLARE_ID (showMarkerTrack)
    DECLARE_ID (showChordTrack)
    DECLARE_ID (showMidiDevices)
    DECLARE_ID (showWaveDevices)
    DECLARE_ID (viewX1)
    DECLARE_ID (viewX2)
    DECLARE_ID (viewY)
    DECLARE_ID (snapType)
    DECLARE_ID (drawWaveforms)
    DECLARE_ID (showHeaders)
    DECLARE_ID (showFooters)
    DECLARE_ID (showArranger)
    DECLARE_ID (headerHeight)
    DECLARE_ID (headerWidth)
    #undef DECLARE_ID
}

//==============================================================================
class EditViewState
{
public:
    EditViewState (te::Edit& e, te::SelectionManager& s)
        : edit (e), selectionManager (s)
    {
        state = edit.state.getOrCreateChildWithName (IDs::EDITVIEWSTATE, nullptr);

        auto um = &edit.getUndoManager();

        showGlobalTrack.referTo (state, IDs::showGlobalTrack, um, false);
        showMarkerTrack.referTo (state, IDs::showMarkerTrack, um, false);
        showChordTrack.referTo (state, IDs::showChordTrack, um, false);
        showArrangerTrack.referTo (state, IDs::showArranger, um, false);
        drawWaveforms.referTo (state, IDs::drawWaveforms, um, true);
        showHeaders.referTo (state, IDs::showHeaders, um, true);
        showFooters.referTo (state, IDs::showFooters, um, false);
        showMidiDevices.referTo (state, IDs::showMidiDevices, um, false);
        showWaveDevices.referTo (state, IDs::showWaveDevices, um, true);

        headerHeight.referTo(state, IDs::headerHeight, um, 50);
        headerWidth.referTo(state, IDs::headerWidth, um, 310);
        viewX1.referTo (state, IDs::viewX1, um, 0.0);
        viewX2.referTo (state, IDs::viewX2, um, 30.0 * 4);
        viewY.referTo (state, IDs::viewY, um, 0);
        snapType.referTo(state, IDs::snapType, um, 7);
    }

    int beatsToX (double beats, int width) const
    {
        return juce::roundToInt (((beats - viewX1) * width) / (viewX2 - viewX1));
    }

    double xToBeats (int x, int width) const
    {
        return (double (x) / width) * (viewX2 - viewX1) + viewX1;
    }

    double beatToTime (double b) const
    {
        auto& ts = edit.tempoSequence;
        return ts.beatsToTime (b);
    }

    double timeToBeat (double t) const
    {
        return edit.tempoSequence.timeToBeats (t);
    }

    te::Edit& edit;
    te::SelectionManager& selectionManager;

    juce::CachedValue<bool> showGlobalTrack, showMarkerTrack, showChordTrack, showArrangerTrack,
                      drawWaveforms, showHeaders, showFooters, showMidiDevices, showWaveDevices;

    juce::CachedValue<double> viewX1, viewX2, viewY;
    juce::CachedValue<int> snapType;

    juce::CachedValue<int> headerHeight, headerWidth;

    juce::ValueTree state;
};
