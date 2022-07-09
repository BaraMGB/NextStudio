#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ApplicationViewState.h"

namespace te = tracktion_engine;

// sheetcheat for snapTypes
//SnapTypeNumber 0 : 1 tick
//SnapTypeNumber 1 : 2 ticks
//SnapTypeNumber 2 : 5 ticks
//SnapTypeNumber 3 : 1/64 beat
//SnapTypeNumber 4 : 1/32 beat      0.03125
//SnapTypeNumber 5 : 1/16 beat      0.0625
//SnapTypeNumber 6 : 1/8 beat       0.125
//SnapTypeNumber 7 : 1/4 beat       0.25
//SnapTypeNumber 8 : 1/2 beat       0.5
//SnapTypeNumber 9 : Beat           1
//SnapTypeNumber 10 : Bar           4
//SnapTypeNumber 11 : 2 bars        8
//SnapTypeNumber 12 : 4 bars        16
//SnapTypeNumber 13 : 8 bars        32
//SnapTypeNumber 14 : 16 bars       64
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
    DECLARE_ID (showMaster)
    DECLARE_ID (trackMinimized)
    DECLARE_ID (headerHeight)
    DECLARE_ID (headerWidth)
    DECLARE_ID (isMidiTrack)
    DECLARE_ID (isAutoArmed)
    DECLARE_ID (isPianoRollVisible)
    DECLARE_ID (timeLineHeight)
    DECLARE_ID (lastNoteLenght)
    DECLARE_ID (name)
    DECLARE_ID (footerBarHeight)
    DECLARE_ID (folderTrackHeight)
    DECLARE_ID (isTrackMinimized)
    DECLARE_ID (automationFollowsClip)
    DECLARE_ID (playHeadStartTime)
    DECLARE_ID (followsPlayhead)
    DECLARE_ID (timeLineZoomUnit)
    DECLARE_ID (zoomMode)
    DECLARE_ID (velocityEditorHeight)
    DECLARE_ID (isHovered)
    DECLARE_ID (lastVelocity)
    DECLARE_ID (pianoRollKeyboardWidth)
    DECLARE_ID (selected)
	DECLARE_ID (selectedRangeStart)
	DECLARE_ID (selectedRangeEnd)
    #undef DECLARE_ID
}

//==============================================================================

class EditViewState
{
public:
    EditViewState (te::Edit& e, te::SelectionManager& s, ApplicationViewState& avs)
        : m_edit (e), m_selectionManager (s), m_applicationState(avs)
    {
        m_state = m_edit.state.getOrCreateChildWithName (
                    IDs::EDITVIEWSTATE, nullptr);

        auto um = &m_edit.getUndoManager();

        m_showGlobalTrack.referTo (m_state, IDs::showGlobalTrack, um, false);
        m_showMarkerTrack.referTo (m_state, IDs::showMarkerTrack, um, false);
        m_showChordTrack.referTo (m_state, IDs::showChordTrack, um, false);
        m_showArrangerTrack.referTo (m_state, IDs::showArranger, um, false);
        m_showMasterTrack.referTo(m_state, IDs::showMaster, um, false);
        m_drawWaveforms.referTo (m_state, IDs::drawWaveforms, um, false);//true);
        m_showHeaders.referTo (m_state, IDs::showHeaders, um,false);// true);
        m_showFooters.referTo (m_state, IDs::showFooters, um, false);
        m_showMidiDevices.referTo (m_state, IDs::showMidiDevices, um, false);
        m_showWaveDevices.referTo (m_state, IDs::showWaveDevices, um, true);
        m_automationFollowsClip.referTo (m_state, IDs::automationFollowsClip, um, true);

        m_trackHeightMinimized.referTo (m_state, IDs::trackMinimized, um, 30);
        m_isAutoArmed.referTo (m_state, IDs::isAutoArmed, um, true);
        m_trackDefaultHeight.referTo(m_state, IDs::headerHeight, um, 50);
        m_trackHeaderWidth.referTo(m_state, IDs::headerWidth, um, 250);
        m_folderTrackHeight.referTo(m_state, IDs::folderTrackHeight, um, 30);
        m_footerBarHeight.referTo (m_state, IDs::footerBarHeight, um, 20);
        m_viewX1.referTo (m_state, IDs::viewX1, um, 0.0);
        m_viewX2.referTo (m_state, IDs::viewX2, um, 30.0 * 4);
        m_viewY.referTo (m_state, IDs::viewY, um, 0);
        m_pianoX1.referTo (m_state, IDs::pianoX1, um, 0);
        m_pianoX2.referTo (m_state, IDs::pianoX2, um, 4);
        m_pianoStartKey.referTo (m_state, IDs::pianoY1, um, 24);
        m_pianoKeyWidth.referTo(m_state, IDs::pianorollNoteWidth, um, 15.0);
        m_isPianoRollVisible.referTo (m_state, IDs::isPianoRollVisible, um, false);
        m_midiEditorHeight.referTo (m_state, IDs::pianorollHeight, um, 400);
        m_lastNoteLength.referTo (m_state, IDs::lastNoteLenght, um, 0);
        m_snapType.referTo(m_state, IDs::snapType, um, 9);
        m_playHeadStartTime.referTo (m_state, IDs::playHeadStartTime, um, 0.0);
        m_followPlayhead.referTo (m_state, IDs::followsPlayhead, um, true);
        m_timeLineHeight.referTo(m_state, IDs::timeLineHeight, um, 50);
        m_editName.referTo(m_state, IDs::name, um, "unknown");
        m_timeLineZoomUnit.referTo(m_state, IDs::timeLineZoomUnit, um, 50);
        m_zoomMode.referTo(m_state, IDs::zoomMode, um, "B");
        m_velocityEditorHeight.referTo(m_state, IDs::velocityEditorHeight, um, 100);
        m_lastVelocity.referTo(m_state, IDs::lastVelocity, um, 100);
        m_keyboardWidth.referTo(m_state, IDs::pianoRollKeyboardWidth, um, 120);
    }

    float getTimeLineZoomUnit ()
    {
        if(m_zoomMode == "B")
            return m_timeLineZoomUnit;
        return m_timeLineZoomUnit * (-1);
    }

    juce::String getZoomMode ()
    {
        return m_zoomMode;
    }

    [[nodiscard]] int beatsToX (double beats, int width, double x1beats, double x2beats) const
    {
        auto t = beatToTime (beats);
        return timeToX (t, width, x1beats, x2beats);
    }

    [[nodiscard]] double xToBeats (int x, int width, double x1beats, double x2beats) const
    {
        auto t = xToTime (x, width, x1beats, x2beats);
        return timeToBeat (t);
    }

    [[nodiscard]] int timeToX (double time, int width, double x1beats, double x2beats) const
    {
        return juce::roundToIntAccurate (((time - beatToTime (x1beats)) * width)
                           / (beatToTime (x2beats) - beatToTime (x1beats)));
    }

    [[nodiscard]] double xToTime(int x, int width, double x1beats, double x2beats) const
    {
        return (double (x) / width)
                * (beatToTime (x2beats) - beatToTime(x1beats))
                + beatToTime (x1beats);
    }

    [[nodiscard]] double beatToTime (double b) const
    {
        auto bp = tracktion::core::BeatPosition::fromBeats(b);
        auto& ts = m_edit.tempoSequence;
        return ts.toTime(bp).inSeconds();
    }

    [[nodiscard]] double timeToBeat (double t) const
    {
        auto tp = tracktion::core::TimePosition::fromSeconds(t);
        auto& ts = m_edit.tempoSequence;
        return ts.toBeats(tp).inBeats();
    }

    [[nodiscard]] double getSnapedTime (
            double t
          , te::TimecodeSnapType snapType
          , bool downwards = false) const
    {
        auto & transport = m_edit.getTransport ();
        auto & temposequ = m_edit.tempoSequence;

        auto tp = tracktion::core::TimePosition::fromSeconds(t);
        transport.setSnapType ({te::TimecodeType::barsBeats, snapType.getLevel ()});
        return downwards
                ? transport.getSnapType ()
                  .roundTimeDown (tp, temposequ).inSeconds()
                : transport.getSnapType ()
                  .roundTimeNearest (tp, temposequ).inSeconds();
    }

    [[nodiscard]] double getQuantizedBeat(double beat, te::TimecodeSnapType snapType, bool downwards = false) const
    {
        return timeToBeat (getSnapedTime (beatToTime (beat), snapType, downwards));
    }

    [[nodiscard]] int snapedX (int x, int width, te::TimecodeSnapType snapType, double x1beats, double x2beats, bool down=false) const
    {
        auto insertTime = xToTime (x, width, x1beats, x2beats);
        auto snapedTime = getSnapedTime (insertTime, snapType, down);
        return timeToX (snapedTime, width, x1beats, x2beats);
    }

    [[nodiscard]] te::TimecodeSnapType getBestSnapType(double beat1, double beat2, int width) const
    {
        double x1time = beatToTime (beat1);
        double x2time = beatToTime (beat2);

        auto td = tracktion::core::TimeDuration::fromSeconds(x2time - x1time);

        auto pos = m_edit.getTransport ().getPosition();
        te::TimecodeSnapType snaptype = m_edit.getTimecodeFormat()
                .getBestSnapType (
                    m_edit.tempoSequence.getTempoAt (pos)
                    , td / width
                    , false);
        return snaptype;
    }

    [[nodiscard]] juce::String getSnapTypeDescription(int idx) const
    {
        auto tp = m_edit.getTransport ().getPosition (); 
        tracktion_engine::TempoSetting &tempo = m_edit.tempoSequence.getTempoAt (tp);
        return m_edit.getTimecodeFormat ().getSnapType (idx).getDescription (tempo, false);
    }

    [[nodiscard]] double getEndScrollBeat() const
    {
        return timeToBeat ( m_edit.getLength ().inSeconds()) + (480);
    }

    void toggleFollowPlayhead()
    {
        m_followPlayhead = !m_followPlayhead;
    }
    [[nodiscard]] bool viewFollowsPos() const {return m_followPlayhead;}
    te::Edit& m_edit;
    te::SelectionManager& m_selectionManager;

    juce::CachedValue<bool> m_showGlobalTrack
                          , m_showMarkerTrack
                          , m_showChordTrack
                          , m_showArrangerTrack
                          , m_showMasterTrack
                          , m_drawWaveforms
                          , m_showHeaders
                          , m_showFooters
                          , m_showMidiDevices
                          , m_showWaveDevices
                          , m_isPianoRollVisible
                          , m_isAutoArmed
                          , m_automationFollowsClip
                          , m_followPlayhead;
    juce::CachedValue<double> m_viewX1
                            , m_viewX2
                            , m_viewY
                            , m_pianoStartKey, m_pianoX1
                            , m_pianoX2
                            , m_pianoKeyWidth, m_lastNoteLength
                            , m_playHeadStartTime
                            , m_timeLineZoomUnit;
    juce::CachedValue<int> m_midiEditorHeight
                            , m_velocityEditorHeight;
    juce::CachedValue<int> m_snapType;

    juce::CachedValue<int> m_trackHeightMinimized
                         , m_trackDefaultHeight
                         , m_trackHeaderWidth
                         , m_timeLineHeight
                         , m_folderTrackHeight
                         , m_footerBarHeight
                         , m_lastVelocity
                         , m_keyboardWidth;

    juce::CachedValue<juce::String> m_editName
                                    , m_zoomMode;
    juce::ValueTree m_state;
    ApplicationViewState& m_applicationState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditViewState)
};
