#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioMidiSettings.h"
#include "EditViewState.h"

namespace te = tracktion_engine;

enum KeyPressCommandIDs
{
    midiNoteC = 1,
    midiNoteCsharp ,
    midiNoteD,
    midiNoteDsharp ,
    midiNoteE,
    midiNoteF,
    midiNoteFsharp ,
    midiNoteG,
    midiNoteGsharp ,
    midiNoteA,
    midiNoteAsharp ,
    midiNoteB,
    midiNoteUpperC,
    midiNoteUpperCsharp ,
    midiNoteUpperD,
    midiNoteUpperDsharp ,
    midiNoteUpperE,
    midiNoteUpperF,
    midiNoteUpperFsharp ,
    midiNoteUpperG,
    midiNoteUpperGsharp ,
    midiNoteUpperA,
    midiNoteUpperAsharp ,
    midiNoteUpperB,
    midiNoteTopC,


    togglePlay,
    toggleRecord,
    play,
    stop,
    deleteSelectedClips,
    duplicateSelectedClips,
    selectAllClips,
    selectAllTracks,
    selectAllClipsOnTrack,
     
    loopAroundSelection,
    loopOn,
    loopOff,
    loopAroundAll,
    loopToggle,

    toggleSnap,
    toggleMetronome,
    snapToBar,
    snapToBeat,
    snapToGrid,
    snapToTime,
    snapToOff,
    
    deleteSelectedTracks,
    duplicateSelectedTracks,

    renderSelectedTimeRangeToNewTrack,

    deleteSelectedNotes,
    duplicateSelectedNotes,
    nudgeNotesUp,
    nudgeNotesDown,
    nudgeNotesLeft,
    nudgeNotesRight,
    nudgeNotesOctaveUp,
    nudgeNotesOctaveDown,

    debugOutputEdit 
};

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
    bool isAutomationVisible(const te::AutomatableParameter& ap);

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

    void drawClip(juce::Graphics& g,
                  juce::Rectangle<int> rect,
                  te::Clip * clip,
                  juce::Colour color,
                  EditViewState& evs);

    void drawRoundedRectWithSide(
        juce::Graphics &g
      , juce::Rectangle<float> area
      , float cornerSize
      , bool topLeft 
      , bool topRight
      , bool bottomLeft
      , bool bottomRight);

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
    struct SelectedTimeRange 
    {
        juce::Array<te::Track*> selectedTracks;
        juce::Array<te::AutomatableParameter*> selectedAutomations;
        tracktion::TimeRange timeRange;

        tracktion::TimePosition getStart() { return timeRange.getStart(); }
        tracktion::TimeDuration getLength() { return timeRange.getLength(); }
        tracktion::TimePosition getEnd() { return timeRange.getEnd(); }
    };
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
    void drawRectWithShadow(juce::Graphics& g,
                            juce::Rectangle<float> area,
                            float cornerSize,
                            const juce::Colour& colour,
                            const juce::Colour& shade);
    void drawCircleWithShadow(juce::Graphics& g,
                              juce::Rectangle<float> area,
                              const juce::Colour& colour,
                              const juce::Colour& shade);
    
    void drawLogo (juce::Graphics& g, juce::Colour colour, float scale);

     juce::String translate (juce::String stringToTranslate, ApplicationViewState& aps);
}

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

    void renderEditToFile(EditViewState& evs, juce::File renderFile, tracktion::TimeRange range={});
    bool renderToNewTrack(EditViewState& evs, juce::Array<tracktion_engine::AudioTrack*> tracksToRender, tracktion::TimeRange range);

    void updateMidiInputs(EditViewState& evs, te::Track::Ptr track);    
    te::MidiInputDevice& getVirtuelMidiInputDevice(te::Engine& engine);
    tracktion::core::TimePosition getTimePos(double t);
    te::AudioTrack::Ptr getAudioTrack(te::Track::Ptr track, EditViewState& evs);

    bool trackWantsClip(const te::Clip* clip, const te::Track* track);
    te::Track* getTargetTrack(te::Track*, int verticalOffset);
    juce::Array<te::Track*> getSortedTrackList(te::Edit& edit);
    void deleteSelectedClips(EditViewState & evs);

    bool isTrackShowable(te::Track::Ptr track);

    bool isTrackItemInRange (te::TrackItem* ti,const tracktion::TimeRange& tr);
    void moveSelectedClips(bool copy, double timeDelta, int verticalOffset,EditViewState& evs);
    void duplicateSelectedClips(EditViewState& evs);
    void copyAutomationForSelectedClips(double offset
                                                     , te::SelectionManager& sm
                                                     , bool copy);




    void selectAllClips(te::SelectionManager& sm, te::Edit& edit);
    void selectAllClipsOnTrack(te::SelectionManager& sm, te::AudioTrack& at);
    void moveAutomationOrCopy(const juce::Array<te::TrackAutomationSection>& origSections, tracktion::TimeDuration offset, bool copy);
	void moveAutomation(te::Track* src,te::TrackAutomationSection::ActiveParameters par, tracktion::TimeRange range, double insertTime, bool copy);
    
    te::TrackAutomationSection getTrackAutomationSection(te::AutomatableParameter* ap, tracktion::TimeRange tr);

    void resizeSelectedClips(bool fromLeftEdge, double delta, EditViewState & evs);

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


    tracktion::TimeRange getTimeRangeOfSelectedClips(EditViewState &evs);

    void play (EditViewState &evs);
    void pause (EditViewState &evs);
    void rewind(EditViewState &evs);
    void stopPlay(EditViewState &evs);
    void toggleLoop (te::Edit& edit);
    void loopAroundSelection (EditViewState &evs);
    void loopOff (te::Edit& edit);
    void loopOn (te::Edit& edit);
    void loopAroundAll (te::Edit& edit);

    void toggleSnap (EditViewState &evs);
    void toggleMetronome (te::Edit& edit);

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
