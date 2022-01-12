#include "Utilities.h"

void Helpers::addAndMakeVisible(juce::Component &parent, const juce::Array<juce::Component *> &children)
{
    for (auto c : children)
        parent.addAndMakeVisible (c);
}

juce::String Helpers::getStringOrDefault(const juce::String &stringToTest, const juce::String &stringToReturnIfEmpty)
{
    return stringToTest.isEmpty() ? stringToReturnIfEmpty : stringToTest;
}

juce::File Helpers::findRecentEdit(const juce::File &dir)
{
    auto files = dir.findChildFiles (juce::File::findFiles, false, "*.tracktionedit");
    if (files.size() > 0)
    {
        files.sort();
        return files.getLast();
    }
    return {};
}

void GUIHelpers::drawRoundedRectWithSide(
        juce::Graphics &g
      , juce::Rectangle<float> area
      , float cornerSize
      , bool left)
{
    g.fillRoundedRectangle(area, cornerSize);
    auto rightRect = area.withTrimmedLeft(area.getWidth()/2);
    g.fillRect(rightRect);
}

void GUIHelpers::changeColor(
        juce::XmlElement &xml
      , const juce::String& inputColour
      , const juce::String& color_hex)
{
    forEachXmlChildElement(xml, xmlnode)
    {
        if (xmlnode->hasAttribute ("fill"))
        {
            if (xmlnode->getStringAttribute ("fill") == inputColour)
            {
                xmlnode->setAttribute ("fill", color_hex);
            }
            if (xmlnode->hasAttribute ("style"))
            {
               juce::String att = xmlnode->getStringAttribute ("style");
               xmlnode->setAttribute (
                         "style"
                       , att.replaceFirstOccurrenceOf (inputColour, color_hex));
            }
        }
        if (xmlnode->hasAttribute ("stroke"))
        {
            if (xmlnode->getStringAttribute ("stroke") == inputColour)
            {
                xmlnode->setAttribute ("stroke", color_hex);
            }
        }
    }
}

void GUIHelpers::drawFromSvg(
        juce::Graphics &g
      , const char *svgbinary
      , const juce::String& col_hex
      , juce::Rectangle<float> drawRect)
{
    if (auto svg = juce::XmlDocument::parse (svgbinary))
    {
        std::unique_ptr<juce::Drawable> drawable;
        GUIHelpers::changeColor (*svg, "#626262", col_hex);
        {
            const juce::MessageManagerLock mmLock;
            drawable = juce::Drawable::createFromSVG (*svg);
            drawable->setTransformToFit (drawRect
                                         , juce::RectanglePlacement::centred);
            drawable->draw (g, 1.f);
        }
    }
}

void GUIHelpers::setDrawableOnButton(
        juce::DrawableButton &button
      , const char *svgbinary
      , const juce::String& col_hex)
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

juce::Image GUIHelpers::getImageFromSvg(
        const char *svgbinary
      , const juce::String& col_hex
      , int w
      , int h)
{
    juce::Image image (juce::Image::RGB, w, h, true);
    juce::Graphics g (image);
    drawFromSvg (
                g
              , svgbinary
              , col_hex
              , { 0.0, 0.0
                , (float) w, (float) h });
    return image;
}

void GUIHelpers::saveEdit(
        EditViewState& evs
      , const juce::File& workDir)
{

    auto editfile = te::EditFileOperations(evs.m_edit).getEditFile ();
    auto file = editfile.getFileName () != "Untitled.tracktionedit"
            ? editfile
            : workDir;
    juce::WildcardFileFilter wildcardFilter ("*.tracktionedit"
                                             , juce::String()
                                             , "Next Studio Project File");

    juce::FileBrowserComponent browser (juce::FileBrowserComponent::saveMode
                                        + juce::FileBrowserComponent::canSelectFiles
                                        , file
                                        , &wildcardFilter
                                        , nullptr);

    juce::FileChooserDialogBox dialogBox ("Save the project",
                                          "Please choose some kind of file that you want to save...",
                                          browser,
                                          true,
                                          juce::Colours::black);

    if (dialogBox.show())
    {
        juce::File selectedFile = browser.getSelectedFile (0)
                .withFileExtension (".tracktionedit");
        EngineHelpers::refreshRelativePathsToNewEditFile(evs, selectedFile);
        evs.m_editName = selectedFile.getFileNameWithoutExtension ();
        te::EditFileOperations(evs.m_edit).writeToFile (selectedFile, false);
    }
}

void GUIHelpers::drawBarsAndBeatLines(juce::Graphics &g, EditViewState &evs, double x1beats, double x2beats, juce::Rectangle<int> boundingRect, bool printDescription)
{

    auto snaptype = evs.getBestSnapType (
                x1beats, x2beats, boundingRect.getWidth ());
    te::TimecodeDisplayIterator iterator (
                evs.m_edit
                , evs.beatToTime (x1beats -1)
                , snaptype
                , false);

    auto t = iterator.next ();
    while (t <= evs.beatToTime (x2beats))
    {
        const auto x = evs.timeToX (t, boundingRect.getWidth (), x1beats, x2beats);
        auto barsBeats = evs.m_edit
                .tempoSequence.timeToBarsBeats (t);


        int barsNum = barsBeats.bars + 1;
        int beatsNum = barsBeats.getFractionalBeats () > 0.9999 ? barsBeats.getWholeBeats () + 2 : barsBeats.getWholeBeats () + 1;
        if (barsBeats.beats > 3.9999)
        {
            barsNum++;
            beatsNum = 1;
        }
        auto beatLenght = evs.beatsToX (x1beats + 1, boundingRect.getWidth (), x1beats, x2beats);

        auto printText = [](juce::Graphics& graphic, juce::Rectangle<float> textRect, const juce::String& text)
        {
            graphic.setColour (juce::Colour(0x70ffffff));
            graphic.drawText ("  " + text
                      , textRect
                      , juce::Justification::centredLeft
                      , false);
        };

        if (printDescription && ((barsBeats.beats < 0.0001) || (barsBeats.beats > 3.9999)))
        {
            auto barRect = juce::Rectangle<float>(
                (float) x
                , 0.f
                , (float) beatLenght * 4.f
                , boundingRect.toFloat().getHeight ());
            if (snaptype.level > 6)
                printText(g, barRect, juce::String(barsNum));
            else
                printText(g, barRect, juce::String(barsNum) + "." + juce::String(beatsNum));
            g.setColour (juce::Colour(0x90ffffff));
        }
        else if (barsBeats.getFractionalBeats () < 0.0001
                 || barsBeats.getFractionalBeats () > 0.9999)
        {
            auto beatRect = juce::Rectangle<float>(
                (float) x
                , 0.f
                , (float) beatLenght
                , boundingRect.toFloat().getHeight ());

            if (snaptype.level < 7 && printDescription)
                printText(g, beatRect, juce::String(barsNum) + "." + juce::String(beatsNum));

            g.setColour (juce::Colour(0x40ffffff));
        }
        else
        {
            g.setColour (juce::Colour(0x10ffffff));
        }
        g.fillRect (x, 0, 1, boundingRect.getHeight());

        t = iterator.next();
    }
}


juce::String PlayHeadHelpers::timeToTimecodeString(double seconds)
{
    auto millisecs = juce::roundToInt (seconds * 1000.0);
    auto absMillisecs = std::abs (millisecs);

    return juce::String::formatted ("%02d:%02d.%03d",

                                    (absMillisecs / 60000) % 60,
                                    (absMillisecs / 1000)  % 60,
                                    absMillisecs % 1000);
}

juce::String PlayHeadHelpers::barsBeatsString(
        tracktion_engine::Edit &edit
      , double time)
{
    te::TempoSequencePosition pos(edit.tempoSequence);
    pos.setTime(time);
    auto bars = pos.getBarsBeatsTime ().bars + 1;
    auto beat = (int)pos.getBarsBeatsTime ().beats + 1;
    auto ticks = juce::roundToIntAccurate(pos.getBarsBeatsTime ()
                       .getFractionalBeats () * 960);
    if (ticks == 960) {ticks = 0;}

    return juce::String(bars) + "."
            + juce::String(beat) + "."
            + juce::String::formatted("%03d", ticks);
}

void EngineHelpers::deleteSelectedClips(EditViewState &evs)
{
    for (auto selectedClip : evs.m_selectionManager
         .getSelectedObjects ()
         .getItemsOfType<te::Clip>())
    {
        for (auto ap : selectedClip->getTrack ()->getAllAutomatableParams ())
        {
            ap->getCurve ().removePointsInRegion (selectedClip->getEditTimeRange ());
        }
        selectedClip->removeFromParentTrack ();
    }
}

void EngineHelpers::copyAutomationForSelectedClips(double offset
                                                 , te::SelectionManager& sm
                                                 , bool copy)
{
    auto clipSelection = sm
            .getSelectedObjects ()
            .getItemsOfType<te::Clip>();
    if (clipSelection.size () > 0)
    {
        //collect automation sections
        juce::Array<te::TrackAutomationSection> sections;

        for (auto& selectedClip : clipSelection)
        {
            sections.add (te::TrackAutomationSection(*selectedClip));
        }
            te::moveAutomation (  sections
                                , offset
                                , copy);
    }
}

void EngineHelpers::pasteClipboardToEdit(
        double pasteTime
      , double firstClipTime
      , const tracktion_engine::Track::Ptr& destinationTrack
      , EditViewState &evs
      , bool removeSource)
{
    if (destinationTrack)
    {
        auto clipboard = tracktion_engine::Clipboard::getInstance();
        if (clipboard->hasContentWithType<te::Clipboard::Clips>())
        {
            auto clipContent = clipboard
                    ->getContentWithType<te::Clipboard::Clips>();

            te::EditInsertPoint insertPoint (evs.m_edit);
            insertPoint.setNextInsertPoint (0, destinationTrack);
            te::Clipboard::ContentType::EditPastingOptions options
                    (evs.m_edit, insertPoint, &evs.m_selectionManager);

            if (removeSource)
            {
                deleteSelectedClips (evs);

            }
            //delete region under pasted clips
            for (const auto& clip : clipContent->clips)
            {
                auto clipShift = (float) clip.state.getProperty (te::IDs::start);
                auto clipInsertTime = pasteTime + clipShift;
                auto clipLength = (float) clip.state.getProperty (te::IDs::length);
                if (auto targetTrack = evs.m_edit.getTrackList ()
                        [destinationTrack->getIndexInEditTrackList () + clip.trackOffset])
                {
                    if (auto at = dynamic_cast<te::AudioTrack*>(targetTrack))
                    {
                        auto range = te::EditTimeRange(clipInsertTime
                                                         , clipInsertTime + clipLength);
                        at->deleteRegion (range,&evs.m_selectionManager);
                    }
                }
            }
            options.startTime = pasteTime;
            options.setTransportToEnd = true;

            clipContent->pasteIntoEdit(options);
        }
    }
}

tracktion_engine::Project::Ptr EngineHelpers::createTempProject(
        tracktion_engine::Engine &engine)
{
    auto file = engine.getTemporaryFileManager()
            .getTempDirectory().getChildFile ("temp_project")
            .withFileExtension (te::projectFileSuffix);
    te::ProjectManager::TempProject tempProject (engine.getProjectManager()
                                                 , file
                                                 , true);
    return tempProject.project;
}

void EngineHelpers::browseForAudioFile(
        tracktion_engine::Engine &engine
        , std::function<void (const juce::File &)> fileChosenCallback)
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

void EngineHelpers::removeAllClips(tracktion_engine::AudioTrack &track)
{
    const auto& clips = track.getClips();

    for (int i = clips.size(); --i >= 0;)
        clips.getUnchecked (i)->removeFromParentTrack();
}

tracktion_engine::AudioTrack *EngineHelpers::getOrInsertAudioTrackAt(
        tracktion_engine::Edit &edit
      , int index)
{
    edit.ensureNumberOfAudioTracks (index + 1);
    return te::getAudioTracks (edit)[index];
}

tracktion_engine::AudioTrack::Ptr EngineHelpers::addAudioTrack(
        bool isMidiTrack
      , juce::Colour trackColour
      , EditViewState &evs)
{
    if (auto track = EngineHelpers::getOrInsertAudioTrackAt (
            evs.m_edit, te::getAudioTracks(evs.m_edit).size()))
    {
         track->state.setProperty (te::IDs::height
                                 , (int) evs.m_trackDefaultHeight
                                 , nullptr);
         track->state.setProperty (IDs::isTrackMinimized, true, nullptr);

         track->state.setProperty(  IDs::isMidiTrack
                                  , isMidiTrack
                                  , &evs.m_edit.getUndoManager());

         juce::String num = juce::String(te::getAudioTracks(evs.m_edit).size());
         track->setName(isMidiTrack ? "Instrument " + num : "Wave " + num);
         track->setColour(trackColour);
         evs.m_selectionManager.selectOnly(track);
         return track;
    }
    return nullptr;
}

tracktion_engine::WaveAudioClip::Ptr EngineHelpers::loadAudioFileAsClip(
        EditViewState &evs
      , const juce::File &file
      , juce::Colour trackColour
      , double insertTime)
{
    te::AudioFile audioFile (evs.m_edit.engine, file);
    if (audioFile.isValid())
    {
        if (auto track = addAudioTrack(false, trackColour, evs))
        {
            removeAllClips (*track);


                if (auto newClip = track->insertWaveClip (
                            file.getFileNameWithoutExtension(), file,
                { { insertTime, insertTime + audioFile.getLength() }, 0.0 }, true))
                {
                    GUIHelpers::log("loading : " + file.getFullPathName ());
                    return newClip;
                }
        }
    }
    return {};
}

void EngineHelpers::refreshRelativePathsToNewEditFile(
        EditViewState & evs, const juce::File& newFile)
{
    for (auto t : te::getAudioTracks (evs.m_edit))
    {
        for (auto c : t->getClips ())
        {
            if (c->state.getProperty (te::IDs::source) != "")
            {
                auto source = evs.m_edit.filePathResolver(
                            c->state.getProperty (te::IDs::source));
                c->state.setProperty (
                      te::IDs::source
                    , source.getRelativePathFrom (newFile.getParentDirectory ())
                    , nullptr);
            }
        }
    }
    evs.m_edit.editFileRetriever = [newFile] {return newFile;};
}

void EngineHelpers::rewind(EditViewState &evs)
{
    auto& transport = evs.m_edit.getTransport ();

    evs.m_playHeadStartTime = 0.0;
    transport.setCurrentPosition(evs.m_playHeadStartTime);
    GUIHelpers::centerView(evs);
}

void EngineHelpers::stopPlay(EditViewState &evs)
{
    auto& transport = evs.m_edit.getTransport ();
    if (!transport.isPlaying())
    {
        evs.m_playHeadStartTime = 0.0;
        transport.setCurrentPosition(evs.m_playHeadStartTime);
    }
    else
    {
        transport.stop(false, false, true, true);
        transport.setCurrentPosition(evs.m_playHeadStartTime);
    }
    GUIHelpers::centerView(evs);
}

void EngineHelpers::toggleLoop (tracktion_engine::Edit &edit)
{
    auto& transport = edit.getTransport ();

    if (transport.looping)
        transport.looping = false;
    else
        transport.looping = true;
}

void EngineHelpers::play (EditViewState &evs)
{
    auto& transport = evs.m_edit.getTransport ();
    if (transport.isPlaying ())
        transport.setCurrentPosition (evs.m_playHeadStartTime);
    transport.play (true);
}

void EngineHelpers::pause (EditViewState &evs)
{
    auto& transport = evs.m_edit.getTransport ();
    if (transport.isPlaying ())
    {
        transport.stop(false, false, true, false);
    }
}

void EngineHelpers::togglePlay(EditViewState& evs)
{
    auto& transport = evs.m_edit.getTransport ();

    if (transport.isPlaying ())
    {
        transport.stop (false, false);
    }
    else
    {
        evs.m_playHeadStartTime = transport.getCurrentPosition () ;
        EngineHelpers::play(evs);
    }
}

void EngineHelpers::toggleRecord (tracktion_engine::Edit &edit)
{
    auto& transport = edit.getTransport ();

    if (transport.isRecording ())
        transport.stop (true, false);
    else
        transport.record (false);
}

void EngineHelpers::armTrack (
        tracktion_engine::AudioTrack &t
      , bool arm
      , int position)
{
    auto& edit = t.edit;
    for (auto instance : edit.getAllInputDevices())
        if (instance->isOnTargetTrack (t, position))
            instance->setRecordingEnabled (t, arm);
}

bool EngineHelpers::isTrackArmed(tracktion_engine::AudioTrack &t, int position)
{
    auto& edit = t.edit;
    for (auto instance : edit.getAllInputDevices())
        if (instance->isOnTargetTrack (t, position))
            return instance->isRecordingEnabled (t);

    return false;
}

bool EngineHelpers::isInputMonitoringEnabled(tracktion_engine::AudioTrack &t, int position)
{
    auto& edit = t.edit;
    for (auto instance : edit.getAllInputDevices())
        if (instance->isOnTargetTrack (t, position))
            return instance->getInputDevice().isEndToEndEnabled();

    return false;
}

void EngineHelpers::enableInputMonitoring(
        tracktion_engine::AudioTrack &t
      , bool im, int position)
{
    if (isInputMonitoringEnabled (t, position) != im)
    {
        auto& edit = t.edit;
        for (auto instance : edit.getAllInputDevices())
            if (instance->isOnTargetTrack (t, position))
                instance->getInputDevice().flipEndToEnd();
    }
}

bool EngineHelpers::trackHasInput(tracktion_engine::AudioTrack &t, int position)
{
    auto& edit = t.edit;
    for (auto instance : edit.getAllInputDevices())
        if (instance->isOnTargetTrack (t, position))
            return true;

    return false;
}

std::unique_ptr<juce::KnownPluginList::PluginTree> EngineHelpers::createPluginTree(
        tracktion_engine::Engine &engine)
{
    auto& list = engine.getPluginManager().knownPluginList;

    if (auto tree = juce::KnownPluginList::createTree (
                list.getTypes()
              , juce::KnownPluginList::sortByManufacturer))
    {
        return tree;
    }
    return {};
}

Thumbnail::Thumbnail(tracktion_engine::TransportControl &tc)
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

void Thumbnail::setFile(const tracktion_engine::AudioFile &file)
{
    smartThumbnail.setNewFile (file);
    cursorUpdater.startTimerHz (25);
    repaint();
}

void Thumbnail::paint(juce::Graphics &g)
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

void Thumbnail::mouseDown(const juce::MouseEvent &e)
{
    transport.setUserDragging (true);
    mouseDrag (e);
}

void Thumbnail::mouseDrag(const juce::MouseEvent &e)
{
    jassert (getWidth() > 0);
    const float proportion = (float) e.position.x / (float) getWidth();
    transport.position = proportion * transport.getLoopRange().getLength();
}

void Thumbnail::mouseUp(const juce::MouseEvent &)
{
    transport.setUserDragging (false);
}

void Thumbnail::updateCursorPosition()
{
    const double loopLength = transport.getLoopRange().getLength();
    const double proportion = loopLength == 0.0 ? 0.0 : transport.getCurrentPosition() / loopLength;

    auto r = getLocalBounds().toFloat();
    const float x = r.getWidth() * float (proportion);
    cursor.setRectangle (r.withWidth (2.0f).withX (x));
}

void EngineHelpers::duplicateSelectedClips(
        te::Edit& edit
      , te::SelectionManager& selectionManager
      , bool withAutomation)
{
    auto clipSelection = selectionManager
            .getSelectedObjects ()
            .getItemsOfType<te::Clip>();
    if (clipSelection.size () > 0)
    {
        auto selectionRange = te::getTimeRangeForSelectedItems (clipSelection);
        edit.getTransport ().setCurrentPosition (selectionRange.end);

        //collect automation sections
        juce::Array<te::TrackAutomationSection> sections;

        for (auto& selectedClip : clipSelection)
        {
            sections.add (te::TrackAutomationSection(*selectedClip));
            //delete destination region
            if (auto at = dynamic_cast<te::AudioTrack*>(selectedClip->getClipTrack ()))
            {
                auto clipstart = selectedClip->getEditTimeRange ().start
                                  - selectionRange.start;
                te::EditTimeRange targetRange = {selectionRange.end + clipstart
                                      , selectionRange.end + clipstart
                                                           + selectedClip
                                        ->getEditTimeRange ().getLength ()};
                at->deleteRegion (targetRange, &selectionManager);
            }
            te::duplicateClip (*selectedClip);
        }
        if (withAutomation)
        {
            te::moveAutomation (  sections
                                , selectionRange.getLength ()
                                , true);//<-copy
        }
        //now move all selected clips. the duplicated clips remain
        te::moveSelectedClips (
                    clipSelection
                  , edit
                  , te::MoveClipAction::moveStartToCursor
                  , false);
    }
}

void GUIHelpers::centerView(EditViewState &evs)
{
    if (evs.viewFollowsPos())
    {
        auto posBeats = evs.timeToBeat (
            evs.m_edit.getTransport ().getCurrentPosition ());

        if (posBeats < evs.m_viewX1 || posBeats > evs.m_viewX2)
            moveView(evs, posBeats);

        auto zoom = evs.m_viewX2 - evs.m_viewX1;
        moveView(evs, juce::jmax((double)evs.m_viewX1, posBeats - zoom/2));
    }
}

void GUIHelpers::moveView(EditViewState& evs, double newBeatPos)
{
    auto delta = evs.m_viewX1 - newBeatPos;
    auto zoom = evs.m_viewX2 - evs.m_viewX1;
    evs.m_viewX1 = evs.m_viewX1 - delta;
    evs.m_viewX2 = evs.m_viewX1 + zoom;
}

float GUIHelpers::getScaleFactor(int delta, float unitDistance)
{
    return std::powf (2,(float) delta / unitDistance);
}
juce::Rectangle<int> GUIHelpers::getSensibleArea(juce::Point<int> p, int w)
{
    return {p.x - (w/2), p.y - (w/2), w, w};
}
int GUIHelpers::getTrackHeight(tracktion_engine::AudioTrack* track, EditViewState& evs)
{
    bool isMinimized = (bool) track->state.getProperty(IDs::isTrackMinimized);
    auto trackHeight =
            isMinimized
            ? evs.m_trackHeightMinimized
            : (int) track->state.getProperty(tracktion_engine::IDs::height, 0);

    if (!isMinimized)
    {
        for (auto apEditItems: track->getAllAutomatableEditItems())
        {
            for (auto ap: apEditItems->getAutomatableParameters())
            {
                if (ap->getCurve().getNumPoints() > 0)
                {
                    int height = ap->getCurve().state.getProperty(
                            tracktion_engine::IDs::height, 0);
                    trackHeight += height;
                }
            }
        }
    }

    return trackHeight;
}
