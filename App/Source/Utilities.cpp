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

void GUIHelpers::log(juce::String message)
{
    std::cout << message << std::endl;
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

void GUIHelpers::drawFromSvg(
        juce::Graphics &g
      , const char *svgbinary
      , juce::String col_hex
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

void GUIHelpers::setDrawableonButton(
        juce::DrawableButton &button
      , const char *svgbinary
      , juce::String col_hex)
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
      , juce::String col_hex
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
        tracktion_engine::Edit &edit
      , juce::File workDir)
{
    auto editfile = te::EditFileOperations(edit).getEditFile ();
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
        te::EditFileOperations(edit).saveAs (selectedFile);
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
    auto ticks = (int)(pos.getBarsBeatsTime ()
                       .getFractionalBeats () * 960 + .5);
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
        selectedClip->removeFromParentTrack ();
    }
}

void EngineHelpers::pasteClipboardToEdit(
        double firstClipTime
      , double clickOffset
      , double insertTime
      , tracktion_engine::Track::Ptr sourceTrack
      , EditViewState &evs
      , bool removeSource
      , bool snap)
{
    auto clipboard = tracktion_engine::Clipboard::getInstance();
    if (clipboard->hasContentWithType<te::Clipboard::Clips>())
    {
        auto clipContent = clipboard
                ->getContentWithType<te::Clipboard::Clips>();

        te::EditInsertPoint insertPoint (evs.m_edit);
        insertPoint.setNextInsertPoint (0, sourceTrack);
        te::Clipboard::ContentType::EditPastingOptions options
                (evs.m_edit, insertPoint, &evs.m_selectionManager);
        const auto xTime = evs.beatToTime(evs.m_viewX1);
        const auto rawTime = juce::jmax(0.0, insertTime - clickOffset + xTime);
        const auto snapedTime = evs.getSnapedTime (rawTime);
        const auto pasteTime = !snap
                ? rawTime - firstClipTime
                : snapedTime - firstClipTime;
        if (removeSource)
        {
            deleteSelectedClips (evs);
        }
        //delete region under pasted clips
        for (auto clip : clipContent->clips)
        {
            auto clipShift = (float) clip.state.getProperty (te::IDs::start) - firstClipTime;
            auto clipInsertTime = snap ? snapedTime + clipShift : rawTime + clipShift;
            auto clipLength = (float) clip.state.getProperty (te::IDs::length);
            auto targetTrack = evs.m_edit.getTrackList ()
                    [sourceTrack->getIndexInEditTrackList () + clip.trackOffset];
            if (auto at = dynamic_cast<te::AudioTrack*>(targetTrack))
            {
                auto editrange = te::EditTimeRange(clipInsertTime
                                                 , clipInsertTime + clipLength);
                at->deleteRegion ( editrange
                                   ,&evs.m_selectionManager);
            }
        }
        options.startTime = pasteTime;
        options.setTransportToEnd = true;

        clipContent->pasteIntoEdit(options);
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
    auto clips = track.getClips();

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

tracktion_engine::WaveAudioClip::Ptr EngineHelpers::loadAudioFileAsClip(
        EditViewState &evs
      , const juce::File &file)
{
    if (auto track = getOrInsertAudioTrackAt (evs.m_edit, tracktion_engine::getAudioTracks(evs.m_edit).size()))
    {
        removeAllClips (*track);
        auto& random = juce::Random::getSystemRandom();
        track->setColour (juce::Colour(random.nextInt (256)
                                       ,random.nextInt (256)
                                       ,random.nextInt (256)));
        te::AudioFile audioFile (evs.m_edit.engine, file);

        if (audioFile.isValid())
        {
            track->deleteRegion ({ 0.0, audioFile.getLength() }
                                 , &evs.m_selectionManager);
            if (auto newClip = track->insertWaveClip (file.getFileNameWithoutExtension(), file,
            { { 0.0, audioFile.getLength() }, 0.0 }, false))
            {
                return newClip;
            }
        }
        std::cout << "loading : " << file.getFullPathName () << std::endl;

    }
    return {};
}

void EngineHelpers::toggleLoop(tracktion_engine::Edit &edit)
{
    auto& transport = edit.getTransport();

    if (transport.looping)
        transport.looping = false;
    else
        transport.looping = true;
}

void EngineHelpers::togglePlay(tracktion_engine::Edit &edit)
{
    auto& transport = edit.getTransport();

    if (transport.isPlaying())
        transport.stop (false, false);
    else
        transport.play (false);
}

void EngineHelpers::toggleRecord(tracktion_engine::Edit &edit)
{
    auto& transport = edit.getTransport();

    if (transport.isRecording())
        transport.stop (true, false);
    else
        transport.record (false);
}

void EngineHelpers::armTrack(
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

    if (auto tree = list.createTree (
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
    const float proportion = e.position.x / getWidth();
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
