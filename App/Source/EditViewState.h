#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

// sheetcheat for snapTypes
//SnapTypeNumber 0 : 1 tick
//SnapTypeNumber 1 : 2 ticks
//SnapTypeNumber 2 : 5 ticks
//SnapTypeNumber 3 : 1/64 beat
//SnapTypeNumber 4 : 1/32 beat
//SnapTypeNumber 5 : 1/16 beat
//SnapTypeNumber 6 : 1/8 beat
//SnapTypeNumber 7 : 1/4 beat
//SnapTypeNumber 8 : 1/2 beat
//SnapTypeNumber 9 : Beat
//SnapTypeNumber 10 : Bar
//SnapTypeNumber 11 : 2 bars
//SnapTypeNumber 12 : 4 bars
//SnapTypeNumber 13 : 8 bars
//SnapTypeNumber 14 : 16 bars
//SnapTypeNumber 15 : 64 bars
//SnapTypeNumber 16 : 128 bars
//SnapTypeNumber 17 : 256 bars
//SnapTypeNumber 18 : 1024 bars


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
    DECLARE_ID (pianoX1)
    DECLARE_ID (pianoX2)
    DECLARE_ID (pianoY1)
    DECLARE_ID (pianoY2)
    DECLARE_ID (pianorollNoteWidth)
    DECLARE_ID (pianorollHeight)
    DECLARE_ID (snapType)
    DECLARE_ID (drawWaveforms)
    DECLARE_ID (showHeaders)
    DECLARE_ID (showFooters)
    DECLARE_ID (showArranger)
    DECLARE_ID (headerHeight)
    DECLARE_ID (headerWidth)
    DECLARE_ID (isMidiTrack)
    DECLARE_ID (isAutoArmed)
    DECLARE_ID (isPianoRollVisible)
    DECLARE_ID (timeLineHeight)
    DECLARE_ID (lastNoteLenght)
    #undef DECLARE_ID
}

//==============================================================================

class EditViewState
{
public:
    EditViewState (te::Edit& e, te::SelectionManager& s)
        : m_edit (e), m_selectionManager (s)
    {
        m_state = m_edit.state.getOrCreateChildWithName (
                    IDs::EDITVIEWSTATE, nullptr);

        auto um = &m_edit.getUndoManager();

        m_showGlobalTrack.referTo (m_state, IDs::showGlobalTrack, um, false);
        m_showMarkerTrack.referTo (m_state, IDs::showMarkerTrack, um, false);
        m_showChordTrack.referTo (m_state, IDs::showChordTrack, um, false);
        m_showArrangerTrack.referTo (m_state, IDs::showArranger, um, false);
        m_drawWaveforms.referTo (m_state, IDs::drawWaveforms, um, true);
        m_showHeaders.referTo (m_state, IDs::showHeaders, um, true);
        m_showFooters.referTo (m_state, IDs::showFooters, um, false);
        m_showMidiDevices.referTo (m_state, IDs::showMidiDevices, um, false);
        m_showWaveDevices.referTo (m_state, IDs::showWaveDevices, um, true);

        m_isAutoArmed.referTo (m_state, IDs::isAutoArmed, um, true);
        m_headerHeight.referTo(m_state, IDs::headerHeight, um, 50);
        m_headerWidth.referTo(m_state, IDs::headerWidth, um, 310);
        m_viewX1.referTo (m_state, IDs::viewX1, um, 0.0);
        m_viewX2.referTo (m_state, IDs::viewX2, um, 30.0 * 4);
        m_viewY.referTo (m_state, IDs::viewY, um, 0);
        m_pianoX1.referTo (m_state, IDs::pianoX1, um, 0);
        m_pianoX2.referTo (m_state, IDs::pianoX2, um, 4);
        m_pianoY1.referTo (m_state, IDs::pianoY1, um, 24);
        m_pianorollNoteWidth.referTo(m_state, IDs::pianorollNoteWidth, um, 15.0);
        m_isPianoRollVisible.referTo (m_state, IDs::isPianoRollVisible, um, false);
        m_pianorollHeight.referTo (m_state, IDs::pianorollHeight, um, 400);
        m_lastNoteLenght.referTo (m_state, IDs::lastNoteLenght, um, 0);
        m_snapType.referTo(m_state, IDs::snapType, um, 9);

        m_timeLineHeight.referTo(m_state, IDs::timeLineHeight, um, 50);
    }

    int beatsToX (double beats, int width) const
    {
        return juce::roundToInt (((beats - m_viewX1) * width)
                                 / (m_viewX2 - m_viewX1));
    }

    double xToBeats (int x, int width) const
    {
        return (double (x) / width) * (m_viewX2 - m_viewX1) + m_viewX1;
    }

    int timeToX (double time, int width) const
    {
        return beatsToX (timeToBeat (time), width);
    }

    double xToTime(int x, int width) const
    {
        return beatToTime (xToBeats (x, width));
    }

    double beatToTime (double b) const
    {
        auto& ts = m_edit.tempoSequence;
        return ts.beatsToTime (b);
    }

    double timeToBeat (double t) const
    {
        return m_edit.tempoSequence.timeToBeats (t);
    }

    double getSnapedTime (double t, bool downwards = false) const
    {
        auto & transport = m_edit.getTransport ();
        auto & temposequ = m_edit.tempoSequence;
        transport.setSnapType ({te::TimecodeType::barsBeats, m_snapType});
        return downwards
                ? transport.getSnapType ()
                  .roundTimeDown (t, temposequ)
                : transport.getSnapType ()
                  .roundTimeNearest (t, temposequ);
    }

    double getSnapedBeat (double beat, bool downwards = false) const
    {
        return timeToBeat (getSnapedTime (beatToTime (beat), downwards));
    }

    int snapedX (int x, int width)
    {
        auto insertTime = xToTime (x, width);
        auto snapedTime = getSnapedTime (insertTime);
        return timeToX (snapedTime, width);
    }

    te::TimecodeSnapType getBestSnapType(bool forPianoRoll, int width)
    {
        double x1 = forPianoRoll ? m_pianoX1 : m_viewX1;
        double x2 = forPianoRoll ? m_pianoX2 : m_viewX2;
        te::TimecodeSnapType snaptype = m_edit.getTimecodeFormat ()
                .getBestSnapType (
                    m_edit.tempoSequence.getTempoAt (
                        m_edit.getTransport ().getCurrentPosition ())
                    , beatToTime ((x2 - x1)/ width));
        return snaptype;
    }

    juce::String getSnapTypeDescription(int idx)
    {
        tracktion_engine::TempoSetting &tempo = m_edit.tempoSequence.getTempoAt (
                    m_edit.getTransport ().getCurrentPosition ());
        return m_edit.getTimecodeFormat ().getSnapType (idx).getDescription (tempo);
    }

    te::Edit& m_edit;
    te::SelectionManager& m_selectionManager;

    juce::CachedValue<bool> m_showGlobalTrack
                          , m_showMarkerTrack
                          , m_showChordTrack
                          , m_showArrangerTrack
                          , m_drawWaveforms
                          , m_showHeaders
                          , m_showFooters
                          , m_showMidiDevices
                          , m_showWaveDevices
                          , m_isPianoRollVisible
                          , m_isAutoArmed;
    juce::CachedValue<double> m_viewX1
                            , m_viewX2
                            , m_viewY
                            , m_pianoX1
                            , m_pianoX2
                            , m_pianorollNoteWidth
                            , m_lastNoteLenght;
    juce::CachedValue<int> m_pianoY1
                         , m_pianorollHeight;
    juce::CachedValue<int> m_snapType;

    juce::CachedValue<int> m_headerHeight
                         , m_headerWidth
                         , m_timeLineHeight;

    juce::ValueTree m_state;
};
