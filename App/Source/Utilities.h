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

    int getTrackHeight(tracktion_engine::Track* track, EditViewState& evs, bool withAutomation=true);

    float getZoomScaleFactor(int delta, float unitDistance);

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

    juce::Rectangle<int> getSensibleArea(juce::Point<int> p, int w);

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
    void centerMidiEditorToClip(EditViewState& evs, te::Clip::Ptr c);

    double getSnapBeats(const te::TimecodeSnapType& snapType);
    void drawBarBeatsShadow(juce::Graphics& g,
                             const EditViewState& evs,
                             double x1beats,
                             double x2beats,
                             const juce::Rectangle<int>& boundingRect,
                             const juce::Colour& shade);
    void drawSnapLines(juce::Graphics& g,
                       const EditViewState& evs,
                       double x1beats,
                       double x2beats,
                       const juce::Rectangle<int>& boundingRect,
                       const juce::Colour& colour);
    void drawPolyObject(juce::Graphics& g, juce::Rectangle<int> area, int edges, float tilt, float rotation,float radiusFac, float heightFac, float scale);
    void drawLogoQuad(juce::Graphics&g, juce::Rectangle<int> area);
    void printTextAt(juce::Graphics& graphic,
                     juce::Rectangle<float> textRect,
                     const juce::String& text,
                     const juce::Colour& textColour);
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
    juce::StringArray getTimeCodeParts(te::Edit & edit, double time);
    juce::String barsBeatsString (te::Edit & edit, double time);

    struct TimeCodeStrings{
        explicit TimeCodeStrings(te::Edit & edit)
        {
            auto currenttime = edit.getTransport ().getCurrentPosition ();
            auto tp = tracktion::core::TimePosition::fromSeconds(currenttime);
            bpm = juce::String(edit.tempoSequence.getTempoAt (tp).bpm, 2);
            auto& timesig = edit.tempoSequence.getTimeSigAt (tp);
            signature = juce::String(juce::String(timesig.numerator) + " / "
                                     + juce::String(timesig.denominator));
            time = timeToTimecodeString (currenttime);
            beats = barsBeatsString (edit, currenttime);
            loopIn  = barsBeatsString (edit, edit.getTransport ()
                                       .getLoopRange ().getStart().inSeconds());
            loopOut = barsBeatsString (edit, edit.getTransport ()
                                       .getLoopRange ().getEnd ().inSeconds());
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
    tracktion::core::TimePosition getTimePos(double t);
    te::AudioTrack::Ptr getAudioTrack(te::Track::Ptr track, EditViewState& evs);

    void deleteSelectedClips(EditViewState & evs);

    void copyAutomationForSelectedClips(double offset
                                                     , te::SelectionManager& sm
                                                     , bool copy);

	void moveAutomation(te::Track* src,te::TrackAutomationSection::ActiveParameters par, tracktion::TimeRange range, double insertTime, bool copy);


    te::Project::Ptr createTempProject (te::Engine& engine);

    void browseForAudioFile (
              te::Engine& engine
            , std::function<void (const juce::File&)> fileChosenCallback);

    void removeAllClips (te::AudioTrack& track);

    te::AudioTrack* getOrInsertAudioTrackAt (te::Edit& edit, int index);

    tracktion_engine::FolderTrack::Ptr addFolderTrack(
        juce::Colour trackColour,
        EditViewState &evs);

    tracktion_engine::AudioTrack::Ptr addAudioTrack(
            bool isMidiTrack
          , juce::Colour trackColour
          , EditViewState &evs);

    te::WaveAudioClip::Ptr loadAudioFileOnNewTrack (
            EditViewState& evs
          , const juce::File& file
          , juce::Colour trackColour
          , double insertTime = 0.0);

    void refreshRelativePathsToNewEditFile(EditViewState & evs
                                       , const juce::File& newFile);

    void insertPlugin(te::Track::Ptr track, te::Plugin::Ptr plugin, int index = -1);

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
    bool isInputMonitoringEnabled (te::Track& t, int position = 0);
    void enableInputMonitoring (te::Track& t, bool im, int position = 0);
    bool trackHasInput (te::Track& t, int position = 0);

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
