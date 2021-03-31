/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioMidiSettings.h"

namespace te = tracktion_engine;


namespace Helpers
{
    void addAndMakeVisible (
            juce::Component& parent
            , const juce::Array<juce::Component*>& children);

    juce::String getStringOrDefault (
              const juce::String& stringToTest
            , const juce::String& stringToReturnIfEmpty);

    juce::File findRecentEdit (const juce::File& dir);
}

namespace GUIHelpers
{
    void log(juce::String message);
    void drawRoundedRectWithSide(
          juce::Graphics & g
        , juce::Rectangle<float> area
        , float cornerSize
        , bool left);

    void changeColor(
          juce::XmlElement& xml
        , juce::String inputColour
        , juce::String color_hex);

    void drawFromSvg(
            juce::Graphics &g
          , const char* svgbinary
          ,juce::String col_hex
          ,juce::Rectangle<float> drawRect);

    void setDrawableonButton(
            juce::DrawableButton& button
          , const char* svgbinary
          ,juce::String col_hex);

    juce::Image getImageFromSvg(
            const char* svgbinary
          , juce::String col_hex
          , int w
          , int h);

    void saveEdit(
            EditViewState& evs
          , juce::File workDir);
}

namespace PlayHeadHelpers
{
    juce::String timeToTimecodeString (double seconds);
    juce::String barsBeatsString (te::Edit & edit, double time);

    struct TimeCodeStrings{
        TimeCodeStrings(te::Edit & edit)
        {
            auto currenttime = edit.getTransport ().getCurrentPosition ();
            bpm = juce::String(edit.tempoSequence.getTempoAt (currenttime).bpm, 2);
            auto& timesig = edit.tempoSequence.getTimeSigAt (currenttime);
            signature = juce::String(juce::String(timesig.numerator) + " / "
                                     + juce::String(timesig.denominator));
            time = timeToTimecodeString (currenttime);
            beats = barsBeatsString (edit, currenttime);
            loopIn  = barsBeatsString (edit, edit.getTransport ()
                                       .getLoopRange ().getStart ());
            loopOut = barsBeatsString (edit, edit.getTransport ()
                                       .getLoopRange ().getEnd ());
        }

        juce::String bpm,
        signature,
        time,
        beats,
        loopIn,
        loopOut;
    };
}

namespace EngineHelpers
{
    void deleteSelectedClips(EditViewState & evs);

    void pasteClipboardToEdit (double firstClipTime
                                    , double clickOffset
                                    , double insertTime
                                    , te::Track::Ptr sourceTrack
                                    , EditViewState& evs
                                    , bool removeSource
                                    , bool snap);

    te::Project::Ptr createTempProject (te::Engine& engine);

    void browseForAudioFile (
              te::Engine& engine
            , std::function<void (const juce::File&)> fileChosenCallback);

    void removeAllClips (te::AudioTrack& track);

    te::AudioTrack* getOrInsertAudioTrackAt (te::Edit& edit, int index);

    te::WaveAudioClip::Ptr loadAudioFileAsClip (
            EditViewState& evs
          , const juce::File& file);

    void refreshRelativePathstoNewEditFile(EditViewState & evs
                                       , juce::File newFile);

    template<typename ClipType>
    typename ClipType::Ptr loopAroundClip (ClipType& clip)
    {
        auto& transport = clip.edit.getTransport();
        transport.setLoopRange (clip.getEditTimeRange());
        transport.looping = true;
        transport.position = 0.0;
        transport.play (false);

        return clip;
    }

    void toggleLoop (te::Edit& edit);
    void togglePlay (te::Edit& edit);
    void toggleRecord (te::Edit& edit);
    void armTrack (te::AudioTrack& t, bool arm, int position = 0);
    bool isTrackArmed (te::AudioTrack& t, int position = 0);
    bool isInputMonitoringEnabled (te::AudioTrack& t, int position = 0);
    void enableInputMonitoring (te::AudioTrack& t, bool im, int position = 0);
    bool trackHasInput (te::AudioTrack& t, int position = 0);
    std::unique_ptr<juce::KnownPluginList::PluginTree> createPluginTree (
            te::Engine& engine);
}

class FlaggedAsyncUpdater : public juce::AsyncUpdater
{
public:
    //==============================================================================
    void markAndUpdate (bool& flag)     { flag = true; triggerAsyncUpdate(); }

    bool compareAndReset (bool& flag) noexcept
    {
        if (! flag)
            return false;

        flag = false;
        return true;
    }
};

struct Thumbnail : public juce::Component
{
    Thumbnail (te::TransportControl& tc);

    void setFile (const te::AudioFile& file);
    void paint (juce::Graphics& g) override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    te::TransportControl& transport;
    te::SmartThumbnail smartThumbnail {
        transport.engine
        , te::AudioFile (transport.engine)
        , *this
        , nullptr
    };
    juce::DrawableRectangle cursor;
    te::LambdaTimer cursorUpdater;

    void updateCursorPosition();
};
