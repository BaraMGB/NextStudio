
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
// #include "AudioMidiSettings.h"
// #include "EditViewState.h"
#include "ApplicationViewState.h"
#include "PluginMenu.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace te = tracktion_engine;
class EditViewState;
enum class Tool 
{
    pointer,
    draw,
    range,
    eraser,
    knife,
    lasso,      
    timestretch
};
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
    
    transposeClipDown,
    transposeClipUp,
    reverseClip,

    undo,
    redo,

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
    enum class CustomMouseCursor
{
    ShiftLeft,
    ShiftRight,
    TimeShiftRight,
    CurveSteepnes,
    ShiftHand,
    Draw,
    Range,
    Lasso,
    Split,
    Erasor
};

    float getScale(const juce::Component& c);

    juce::MouseCursor createCustomMouseCursor(CustomMouseCursor cursorType, const juce::Component& c);
    juce::MouseCursor getMouseCursorFromPng(const char* png, const int size, juce::Point<int> hotPoint);
    juce::MouseCursor getMouseCursorFromSvg(const char* svgbinary, juce::Point<int> hitPoint, float scale=1.f);

    std::unique_ptr<juce::Drawable> getDrawableFromSvg(const char* svgbinary, juce::Colour colour);
    void drawFromSvg(juce::Graphics &g, const char* svgbinary, juce::Colour newColour, juce::Rectangle<float> drawRect);
    void setDrawableOnButton(juce::DrawableButton& button, const char* svgbinary, juce::Colour colour);
    juce::Image drawableToImage(const juce::Drawable& drawable, float targetWidth, float targetHeight);

    
    int getTrackHeight(tracktion_engine::Track* track, EditViewState& evs, bool withAutomation=true);

    bool isAutomationVisible(const te::AutomatableParameter& ap);

    float getZoomScaleFactor(int delta, float unitDistance);

    template<typename T>
    void log(T message)
    {
#ifdef DEBUG_OR_RELWITHDEBINFO
        std::cout << juce::Time::getCurrentTime().toString(true, true, true, true)
                  << ": " << message << std::endl;
#endif
    }
    template<typename T>
    void log(const juce::String& d, T message)
    {
#ifdef DEBUG_OR_RELWITHDEBINFO
        std::cout << juce::Time::getCurrentTime().toString(true, true, true, true)
                  << ": " << d << " : "
                  << ": " << message << std::endl;
#endif
    }

    void centerView(EditViewState& evs);

    juce::Rectangle<int> getSensibleArea(juce::Point<int> p, int w);


    void drawTrack(juce::Graphics& g, juce::Component& parent,EditViewState& evs, juce::Rectangle<int> displayedRect, te::ClipTrack::Ptr clipTrack, tracktion::TimeRange etr, bool forDragging=false);
    void drawClip(juce::Graphics& g, juce::Component& parent, EditViewState& evs,juce::Rectangle<int> rect, te::Clip * clip, juce::Colour color, juce::Rectangle<int> displayedRect, double x1Beat, double x2beat);

    void drawClipBody(juce::Graphics& g, EditViewState& evs,juce::String name, juce::Rectangle<int> clipRect,bool isSelected, juce::Colour color, juce::Rectangle<int> displayedRect, double x1Beat, double x2beat);

    void drawMidiClip (juce::Graphics& g, EditViewState& evs,te::MidiClip::Ptr clip, juce::Rectangle<int> clipRect, juce::Rectangle<int> displayedRect, juce::Colour color, double x1Beat, double x2beat);


    void drawWaveform(juce::Graphics& g, EditViewState& evs, te::AudioClipBase& c, te::SmartThumbnail& thumb, juce::Colour colour, juce::Rectangle<int>, juce::Rectangle<int> displayedRect, double x1Beat, double x2beat);
    void drawChannels(juce::Graphics& g, te::SmartThumbnail& thumb, juce::Rectangle<int> area, bool useHighRes, tracktion::core::TimeRange time, bool useLeft, bool useRight, float leftGain, float rightGain);


    void drawAutomationLane (juce::Graphics& g,EditViewState& evs, tracktion::TimeRange drawRange, juce::Rectangle<int> drawRect, te::AutomatableParameter::Ptr ap, bool forDragging=false);

    void drawRoundedRectWithSide(
        juce::Graphics &g
      , juce::Rectangle<float> area
      , float cornerSize
      , bool topLeft 
      , bool topRight
      , bool bottomLeft
      , bool bottomRight);

    void saveEdit(
            EditViewState& evs
          , const juce::File& workDir);

    void drawBarsAndBeatLines(juce::Graphics &g
          , EditViewState &evs
          , double x1beats
          , double x2beats
          , juce::Rectangle<int> boundingRect, bool printDescription=false);
    
    void drawFakeRoundCorners(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour, juce::Colour outline, int stroke=1);
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
    bool renderCliptoNewTrack(EditViewState & evs, te::Clip::Ptr clip);
    bool renderToNewTrack(EditViewState& evs, juce::Array<tracktion_engine::Track*> tracksToRender, tracktion::TimeRange range);

    void updateMidiInputs(EditViewState& evs, te::Track::Ptr track);    
    te::MidiInputDevice& getVirtuelMidiInputDevice(te::Edit& edit);
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

    te::WaveAudioClip::Ptr loadAudioFileToTrack(
            const juce::File& file
          , te::AudioTrack::Ptr track
          , te::ClipPosition pos);

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

    juce::PluginDescription  getPluginDesc(const juce::String& uniqueId
                                            , const juce::String& name,
                                juce::String  xmlType_, bool isSynth);

    juce::Array<juce::PluginDescription> getInternalPlugins();

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
    
    struct CompareNameForward{
        static int compareElements (const juce::PluginDescription& first, 
                                              const juce::PluginDescription& second)
        {   
            return first.name.compareNatural(second.name);
        }
    };

    struct CompareNameBackwards{
        static int compareElements(const juce::PluginDescription& first, 
                                               const juce::PluginDescription& second)
        {
            return second.name.compareNatural(first.name);
        }
    };


    struct CompareFormatForward{
        static int compareElements(const juce::PluginDescription& first,
                                                         const juce::PluginDescription& second)
        {
            return first.pluginFormatName.compareNatural(second.pluginFormatName);   
        }
    };

    struct CompareFormatBackward{
        static int compareElements(const juce::PluginDescription& first,
                                                          const juce::PluginDescription& second)
        {
            return second.pluginFormatName.compareNatural(first.pluginFormatName);
        }
    };

    void sortByFormatName(juce::Array<juce::PluginDescription>& list, bool forward);
    void sortByName(juce::Array<juce::PluginDescription>& list, bool forward);
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

struct SampleView : public juce::Component
{
    explicit SampleView (te::TransportControl& tc);

    void setFile (const te::AudioFile& file);
    void setColour(juce::Colour colour)
    {
        m_colour = colour;
    }
    void paint (juce::Graphics& g) override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Colour m_colour;
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
