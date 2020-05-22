/*
  ==============================================================================

    SongEditorState.h
    Created: 9 Mar 2020 5:17:53pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace IDs
{
    #define DECLARE_ID(name) const juce::Identifier name (#name);
    DECLARE_ID(SONGEDITORSTATE)
    DECLARE_ID(showGlobalTrack)
    DECLARE_ID(showMarkerTrack)
    DECLARE_ID(showChordTrack)
    DECLARE_ID(viewX1)
    DECLARE_ID(viewX2)
    DECLARE_ID(viewY)
    DECLARE_ID(drawWaveforms)
    DECLARE_ID(pixelPerBeat)
#undef DECLARE_ID
}

class SongEditorViewState
{
public:
    SongEditorViewState(tracktion_engine::Edit& edit, tracktion_engine::SelectionManager& selectionManager)
        : m_edit(edit)
        , m_selectionManager(selectionManager)
    {
        m_state = m_edit.state.getOrCreateChildWithName(IDs::SONGEDITORSTATE, nullptr);

        auto um = &edit.getUndoManager();

        m_showGlobalTrack.referTo(m_state, IDs::showGlobalTrack, um, false);
        m_showMarkerTrack.referTo(m_state, IDs::showMarkerTrack, um, false);
        m_showChordTrack.referTo (m_state, IDs::showChordTrack, um, false);
        m_drawWaveforms.referTo  (m_state, IDs::drawWaveforms, um, true);

        m_viewX1.referTo(m_state, IDs::viewX1, um, 0);
        m_viewX2.referTo(m_state, IDs::viewX2, um, 60);
        m_viewY.referTo (m_state, IDs::viewY, um, 0);

        m_pixelPerBeat.referTo(m_state, IDs::pixelPerBeat, um, 140);
    }


    CachedValue<bool>   m_showGlobalTrack,
                        m_showMarkerTrack,
                        m_showChordTrack,
                        m_drawWaveforms;
    CachedValue<double> m_viewX1,
                        m_viewX2,
                        m_viewY,
                        m_pixelPerBeat;

    ValueTree m_state;

    tracktion_engine::Edit& m_edit;
    tracktion_engine::SelectionManager& m_selectionManager;
};