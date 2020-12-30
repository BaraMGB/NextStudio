/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2018
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

//==============================================================================
namespace Helpers
{
    static inline void addAndMakeVisible (juce::Component& parent
                                          , const juce::Array<juce::Component*>& children)
    {
        for (auto c : children)
            parent.addAndMakeVisible (c);
    }

    static inline juce::String getStringOrDefault (
              const juce::String& stringToTest
            , const juce::String& stringToReturnIfEmpty)
    {
        return stringToTest.isEmpty() ? stringToReturnIfEmpty : stringToTest;
    }
    
    static inline juce::File findRecentEdit (const juce::File& dir)
    {
        auto files = dir.findChildFiles (juce::File::findFiles, false, "*.tracktionedit");
        
        if (files.size() > 0)
        {
            files.sort();
            return files.getLast();
        }
        
        return {};
    }
}

namespace GUIHelpers
{

inline void drawRoundedRectWithSide(
          juce::Graphics & g
        , juce::Rectangle<float> area
        , float cornerSize
        , bool left)
{
    g.fillRoundedRectangle(area, cornerSize);
    auto rightRect = area.withTrimmedLeft(area.getWidth()/2);
    g.fillRect(rightRect);
}


inline void changeColor(
          juce::XmlElement& xml
        , juce::String inputColour
        , juce::String color_hex)
{
    forEachXmlChildElement(xml, xmlnode)
    {
        if (xmlnode->hasAttribute ("fill"))
        {
            if (xmlnode->getStringAttribute ("fill") == inputColour)
            {
                xmlnode->setAttribute ("fill", color_hex);
            }
        }
    }
}
inline void drawFromSvg(juce::Graphics &g, const char* svgbinary,juce::String col_hex,int w,int h)
{
    if (auto svg = juce::XmlDocument::parse (svgbinary))
    {

        std::unique_ptr<juce::Drawable> drawable;
        GUIHelpers::changeColor (*svg, "#626262", col_hex);
        {
            const juce::MessageManagerLock mmLock;
            drawable = juce::Drawable::createFromSVG (*svg);
            drawable->setTransformToFit (
                          juce::Rectangle<float> (0.0f, 0.0f, float (w), float (h))
                        , juce::RectanglePlacement::centred);
            drawable->draw (g, 1.f);
        }

    }
}

inline void setDrawableonButton(juce::DrawableButton& button, const char* svgbinary,juce::String col_hex)
{
    if (auto svg = juce::XmlDocument::parse (svgbinary))
    {
        std::unique_ptr<juce::Drawable> drawable;
        GUIHelpers::changeColor (*svg, "#626262", col_hex);
        {
            const juce::MessageManagerLock mmLock;
            drawable = juce::Drawable::createFromSVG (*svg);
        }
        button.setImages (drawable.get ());
    }

}
inline juce::Image getImageFromSvg(const char* svgbinary,juce::String col_hex,int w,int h)
{
    juce::Image image (juce::Image::RGB, w, h, true);
    juce::Graphics g (image);
    drawFromSvg (g,svgbinary, col_hex, w, h);
    return image;
}



}
//==============================================================================
namespace PlayHeadHelpers
{
    // Quick-and-dirty function to format a timecode string
    static inline juce::String timeToTimecodeString (double seconds)
    {
        auto millisecs = juce::roundToInt (seconds * 1000.0);
        auto absMillisecs = std::abs (millisecs);

        return juce::String::formatted ("%02d:%02d.%03d",

                                  (absMillisecs / 60000) % 60,
                                  (absMillisecs / 1000)  % 60,
                                  absMillisecs % 1000);
    }

    // Quick-and-dirty function to format a bars/beats string
    static inline juce::String quarterNotePositionToBarsBeatsString (double quarterNotes, int numerator, int denominator)
    {
        if (numerator == 0 || denominator == 0)
            return "1.1.000";

        auto quarterNotesPerBar = (numerator * 4 / denominator);
        auto beats  = (fmod (quarterNotes, quarterNotesPerBar) / quarterNotesPerBar) * numerator;

        auto bar    = ((int) quarterNotes) / quarterNotesPerBar + 1;
        auto beat   = ((int) beats) + 1;
        auto ticks  = ((int) (fmod (beats, 1.0) * 960.0 + 0.5));

        return juce::String::formatted ("%d.%d.%03d", bar, beat, ticks);
    }

    struct TimeCodeStrings{
        TimeCodeStrings(const juce::AudioPlayHead::CurrentPositionInfo& pos)
        {
            bpm = juce::String(pos.bpm,2);
            signature = juce::String(juce::String(pos.timeSigNumerator) + "/" + juce::String(pos.timeSigDenominator));
            time = timeToTimecodeString (pos.timeInSeconds);
            beats = quarterNotePositionToBarsBeatsString (pos.ppqPosition,
                                                          pos.timeSigNumerator,
                                                          pos.timeSigDenominator);
            loopIn = quarterNotePositionToBarsBeatsString (pos.ppqLoopStart,
                                                           pos.timeSigNumerator,
                                                           pos.timeSigDenominator);
            loopOut = quarterNotePositionToBarsBeatsString (pos.ppqLoopEnd,
                                                          pos.timeSigNumerator,
                                                          pos.timeSigDenominator);
        }
        juce::String bpm,
                     signature,
                     time,
                     beats,
                     loopIn,
                     loopOut;
    };

    // Returns a textual description of a CurrentPositionInfo
    static inline juce::String getTimecodeDisplay (const juce::AudioPlayHead::CurrentPositionInfo& pos)
    {
        juce::MemoryOutputStream displayText;

        displayText << juce::String (pos.bpm, 2) << " bpm, "
                    << pos.timeSigNumerator << '/' << pos.timeSigDenominator
                    << "  -  " << timeToTimecodeString (pos.timeInSeconds)
                    << "  -  " << quarterNotePositionToBarsBeatsString (pos.ppqPosition,
                                                                        pos.timeSigNumerator,
                                                                        pos.timeSigDenominator);

        if (pos.isRecording)
            displayText << "  (recording)";
        else if (pos.isPlaying)
            displayText << "  (playing)";
        else
            displayText << "  (stopped)";

        return displayText.toString();
    }
}

//==============================================================================
namespace EngineHelpers
{
    inline te::Project::Ptr createTempProject (te::Engine& engine)
    {
        auto file = engine.getTemporaryFileManager()
                .getTempDirectory().getChildFile ("temp_project")
                .withFileExtension (te::projectFileSuffix);
        te::ProjectManager::TempProject tempProject (engine.getProjectManager()
                                                     , file
                                                     , true);
        return tempProject.project;
    }

    inline void showAudioDeviceSettings (te::Engine& engine)
    {
//        auto chacheDir = engine.getPropertyStorage ().getAppPrefsFolder ();
//        auto setupFile = chacheDir.getChildFile ("Settings.xml");
//        if (setupFile.exists ())
//        {
//            setupFile.deleteFile ();
//        }

        juce::DialogWindow::LaunchOptions o;
        o.dialogTitle = TRANS("Audio Settings");
        o.dialogBackgroundColour = juce::LookAndFeel::getDefaultLookAndFeel()
                .findColour (juce::ResizableWindow::backgroundColourId);
        o.content.setOwned (new juce::AudioDeviceSelectorComponent (
                                engine.getDeviceManager().deviceManager
                                , 0, 512, 1, 512, true, true, true, true));
        o.content->setSize (400, 600);
        o.launchAsync ();
    }

    inline void browseForAudioFile (
              te::Engine& engine
            , std::function<void (const juce::File&)> fileChosenCallback)
    {
        auto fc = std::make_shared<juce::FileChooser> (
                    "Please select an audio file to load..."
                    , engine.getPropertyStorage()
                        .getDefaultLoadSaveDirectory ("pitchAndTimeExample")
                    , engine.getAudioFileFormatManager()
                        .readFormatManager.getWildcardForAllFormats());

        fc->launchAsync (juce::FileBrowserComponent::openMode
                         + juce::FileBrowserComponent::canSelectFiles
                         , [fc, &engine, callback = std::move (fileChosenCallback)]
                                (const juce::FileChooser&)
                            {
                                const auto f = fc->getResult();

                                if (f.existsAsFile())
                                    engine.getPropertyStorage()
                                        .setDefaultLoadSaveDirectory (
                                           "pitchAndTimeExample"
                                           , f.getParentDirectory());

                                callback (f);
                            });
    }

    inline void removeAllClips (te::AudioTrack& track)
    {
        auto clips = track.getClips();

        for (int i = clips.size(); --i >= 0;)
            clips.getUnchecked (i)->removeFromParentTrack();
    }
    
    inline te::AudioTrack* getOrInsertAudioTrackAt (te::Edit& edit, int index)
    {
        edit.ensureNumberOfAudioTracks (index + 1);
        return te::getAudioTracks (edit)[index];
    }

    inline te::WaveAudioClip::Ptr loadAudioFileAsClip (te::Edit& edit, const juce::File& file)
    {
        if (auto track = getOrInsertAudioTrackAt (edit, tracktion_engine::getAudioTracks(edit).size()))
        {
            removeAllClips (*track);

            // Add a new clip to this track
            te::AudioFile audioFile (edit.engine, file);

            if (audioFile.isValid())
                if (auto newClip = track->insertWaveClip (file.getFileNameWithoutExtension(), file,
                                                          { { 0.0, audioFile.getLength() }, 0.0 }, false))
                    return newClip;
        }

        return {};
    }

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

    inline void toggleLoop (te::Edit& edit)
    {
        auto& transport = edit.getTransport();

        if (transport.looping)
            transport.looping = false;
        else
            transport.looping = true;
    }

    inline void togglePlay (te::Edit& edit)
    {
        auto& transport = edit.getTransport();

        if (transport.isPlaying())
            transport.stop (false, false);
        else
            transport.play (false);
    }
    
    inline void toggleRecord (te::Edit& edit)
    {
        auto& transport = edit.getTransport();
        
        if (transport.isRecording())
            transport.stop (true, false);
        else
            transport.record (false);
    }
    
    inline void armTrack (te::AudioTrack& t, bool arm, int position = 0)
    {
        auto& edit = t.edit;
        for (auto instance : edit.getAllInputDevices())
            if (instance->isOnTargetTrack (t, position))
                instance->setRecordingEnabled (t, arm);
    }
    
    inline bool isTrackArmed (te::AudioTrack& t, int position = 0)
    {
        auto& edit = t.edit;
        for (auto instance : edit.getAllInputDevices())
            if (instance->isOnTargetTrack (t, position))
                return instance->isRecordingEnabled (t);
        
        return false;
    }
    
    inline bool isInputMonitoringEnabled (te::AudioTrack& t, int position = 0)
    {
        auto& edit = t.edit;
        for (auto instance : edit.getAllInputDevices())
            if (instance->isOnTargetTrack (t, position))
                return instance->getInputDevice().isEndToEndEnabled();
        
        return false;
    }
    
    inline void enableInputMonitoring (te::AudioTrack& t, bool im, int position = 0)
    {
        if (isInputMonitoringEnabled (t, position) != im)
        {
            auto& edit = t.edit;
            for (auto instance : edit.getAllInputDevices())
                if (instance->isOnTargetTrack (t, position))
                    instance->getInputDevice().flipEndToEnd();
        }
    }
    
    inline bool trackHasInput (te::AudioTrack& t, int position = 0)
    {
        auto& edit = t.edit;
        for (auto instance : edit.getAllInputDevices())
            if (instance->isOnTargetTrack (t, position))
                return true;
        
        return false;
    }

    inline std::unique_ptr<juce::KnownPluginList::PluginTree> createPluginTree (te::Engine& engine)
    {
        auto& list = engine.getPluginManager().knownPluginList;

        if (auto tree = list.createTree (list.getTypes(), juce::KnownPluginList::sortByManufacturer))
            return tree;

        return {};
    }

}

//==============================================================================
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

//==============================================================================
struct Thumbnail    : public juce::Component
{
    Thumbnail (te::TransportControl& tc)
        : transport (tc)
    {
        cursorUpdater.setCallback ([this]
                                   {
                                       updateCursorPosition();

                                       if (smartThumbnail.isGeneratingProxy() || smartThumbnail.isOutOfDate())
                                           repaint();
                                   });
        cursor.setFill (findColour (juce::Label::textColourId));
        addAndMakeVisible (cursor);
    }

    void setFile (const te::AudioFile& file)
    {
        smartThumbnail.setNewFile (file);
        cursorUpdater.startTimerHz (25);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        const auto colour = findColour (juce::Label::textColourId);

        if (smartThumbnail.isGeneratingProxy())
        {
            g.setColour (colour.withMultipliedBrightness (0.9f));
            g.drawText ("Creating proxy: "
                        + juce::String (juce::roundToInt (
                              smartThumbnail.getProxyProgress() * 100.0f))
                        + "%"
                        , r
                        , juce::Justification::centred);

        }
        else
        {
            const float brightness = smartThumbnail.isOutOfDate() ? 0.4f : 0.66f;
            g.setColour (colour.withMultipliedBrightness (brightness));
            smartThumbnail.drawChannels (g, r, true, { 0.0, smartThumbnail.getTotalLength() }, 1.0f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        transport.setUserDragging (true);
        mouseDrag (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        jassert (getWidth() > 0);
        const float proportion = e.position.x / getWidth();
        transport.position = proportion * transport.getLoopRange().getLength();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        transport.setUserDragging (false);
    }

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

    void updateCursorPosition()
    {
        const double loopLength = transport.getLoopRange().getLength();
        const double proportion = loopLength == 0.0 ? 0.0 : transport.getCurrentPosition() / loopLength;

        auto r = getLocalBounds().toFloat();
        const float x = r.getWidth() * float (proportion);
        cursor.setRectangle (r.withWidth (2.0f).withX (x));
    }
};
