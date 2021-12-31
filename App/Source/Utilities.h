/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com
*/

#pragma once


#include <utility>

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

    template<typename T>
    static T invert(T value)
    {
        return value * (-1);
    }
}



namespace GUIHelpers
{
    template<typename T>
    void log(T message)
    {
        std::cout << juce::Time::getCurrentTime().toString(true, true, true, true)
                  << ": " << message << std::endl;
    }
    template<typename T>
    void log(const juce::String& d, T message)
    {
        std::cout << juce::Time::getCurrentTime().toString(true, true, true, true)
                  << ": " << d << " : "
                  << ": " << message << std::endl;
    }

    void centerView(EditViewState& evs);

    void drawRoundedRectWithSide(
          juce::Graphics & g
        , juce::Rectangle<float> area
        , float cornerSize
        , bool left);

    void changeColor(
          juce::XmlElement& xml
        , const juce::String& inputColour
        , const juce::String& color_hex);

    void drawFromSvg(
            juce::Graphics &g
          , const char* svgbinary
          ,const juce::String& col_hex
          ,juce::Rectangle<float> drawRect);

    void setDrawableOnButton(
            juce::DrawableButton& button
          , const char* svgbinary
          ,const juce::String& col_hex);

    juce::Image getImageFromSvg(
            const char* svgbinary
          , const juce::String& col_hex
          , int w
          , int h);

    void saveEdit(
            EditViewState& evs
          , const juce::File& workDir);

    void drawBarsAndBeatLines(juce::Graphics &g
          , EditViewState &evs
          , double x1beats
          , double x2beats
          , juce::Rectangle<int> boundingRect, bool printDescription=false);
    void moveView(EditViewState& evs, double newBeatPos);
    }

    class DelayedOneShotLambda : public juce::Timer
    {
    public:
        DelayedOneShotLambda(int ms, std::function<void()> fn)
        : m_func(std::move(fn))
        {
            startTimer(ms);
        }
        ~DelayedOneShotLambda() override { stopTimer(); }

        void timerCallback() override
        {
            auto f = m_func;
            delete this;
            f();
        }
    private:
        std::function<void()> m_func;
    };

namespace PlayHeadHelpers
{
    juce::String timeToTimecodeString (double seconds);
    juce::String barsBeatsString (te::Edit & edit, double time);

    struct TimeCodeStrings{
        explicit TimeCodeStrings(te::Edit & edit)
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

    void duplicateSelectedClips (tracktion_engine::Edit &edit
                               , te::SelectionManager& selectionManager
                               , bool withAutomation);

    void pasteClipboardToEdit(
            double pasteTime
          , double firstClipTime
          , const tracktion_engine::Track::Ptr& destinationTrack
          , EditViewState &evs
          , bool removeSource);

    void copyAutomationForSelectedClips(double offset
                                                     , te::SelectionManager& sm
                                                     , bool copy);

    te::Project::Ptr createTempProject (te::Engine& engine);

    void browseForAudioFile (
              te::Engine& engine
            , std::function<void (const juce::File&)> fileChosenCallback);

    void removeAllClips (te::AudioTrack& track);

    te::AudioTrack* getOrInsertAudioTrackAt (te::Edit& edit, int index);

    tracktion_engine::AudioTrack::Ptr addAudioTrack(
            bool isMidiTrack
          , juce::Colour trackColour
          , EditViewState &evs);

    te::WaveAudioClip::Ptr loadAudioFileAsClip (
            EditViewState& evs
          , const juce::File& file
          , juce::Colour trackColour
          , double insertTime = 0.0);

    void refreshRelativePathsToNewEditFile(EditViewState & evs
                                       , const juce::File& newFile);

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


    void play (EditViewState &evs);
    void pause (EditViewState &evs);
    void rewind(EditViewState &evs);
    void stopPlay(EditViewState &evs);
    void toggleLoop (te::Edit& edit);
    void togglePlay (EditViewState &evs);
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

    static bool compareAndReset (bool& flag) noexcept
    {
        if (! flag)
            return false;

        flag = false;
        return true;
    }
};

struct Thumbnail : public juce::Component
{
    explicit Thumbnail (te::TransportControl& tc);

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
