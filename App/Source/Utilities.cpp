#include "Utilities.h"
#include "EditViewState.h"
#include "tracktion_core/utilities/tracktion_Time.h"
#include "tracktion_core/utilities/tracktion_TimeRange.h"

void Helpers::addAndMakeVisible(juce::Component &parent, const juce::Array<juce::Component *> &children)
{
    for (auto c : children)
    {
        parent.addAndMakeVisible (c);
        c->setWantsKeyboardFocus(false);
    }
}

juce::String Helpers::getStringOrDefault(const juce::String &stringToTest, const juce::String &stringToReturnIfEmpty)
{
    return stringToTest.isEmpty() ? stringToReturnIfEmpty : stringToTest;
}

juce::File Helpers::findRecentEdit(const juce::File &dir)
{
    auto files = dir.findChildFiles (juce::File::findFiles, false, "*.nextTemp");
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
      , bool topLeft 
      , bool topRight
      , bool bottomLeft
      , bool bottomRight)
{
        juce::Path p;
        auto x = area.getX();
        auto y = area.getY();
        auto w = area.getWidth();
        auto h = area.getHeight();
        p.addRoundedRectangle(x,y,w,h,cornerSize,cornerSize,topLeft, topRight, bottomLeft, bottomRight);
        g.fillPath(p);
   
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

    juce::WildcardFileFilter wildcardFilter ("*.tracktionedit"
                                             , juce::String()
                                             , "Next Studio Project File");

    juce::FileBrowserComponent browser (juce::FileBrowserComponent::saveMode
                                        + juce::FileBrowserComponent::canSelectFiles
                                        , workDir 
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
        evs.m_editName = selectedFile.getFileNameWithoutExtension ();

        auto cf = evs.m_edit.editFileRetriever();
        EngineHelpers::refreshRelativePathsToNewEditFile(evs, selectedFile);
        te::EditFileOperations(evs.m_edit).writeToFile (selectedFile, false);
        EngineHelpers::refreshRelativePathsToNewEditFile(evs, cf);
        evs.m_edit.sendSourceFileUpdate();    
    }
}

void GUIHelpers::drawBarsAndBeatLines(juce::Graphics &g, EditViewState &evs, double x1beats, double x2beats, juce::Rectangle<int> boundingRect, bool printDescription)
{
    const auto BarColour = juce::Colour(0x90ffffff);
    const auto beatColour = juce::Colour(0x60ffffff);
    const auto fracColour = juce::Colour(0x40ffffff);
    const auto snapLineColour = juce::Colour(0x20ffffff);
    const auto shadowShade = juce::Colour(0x50000000);
    const auto textColour = juce::Colour(0xffaaaaaa);
    const auto num = static_cast<int> (
        evs.m_edit.tempoSequence.getTimeSigAt(tracktion::TimePosition::fromSeconds(0)).numerator);

    if (!printDescription)
        drawBarBeatsShadow(g, evs, x1beats, x2beats, boundingRect, shadowShade);

    int snapLevel =
        evs.getBestSnapType(x1beats, x2beats, boundingRect.getWidth()).getLevel();

    if (snapLevel <= 9)
        drawSnapLines(g, evs, x1beats, x2beats, boundingRect, snapLineColour);

    for (int bar  = static_cast<int>(x1beats / num);
             bar <= static_cast<int>(x2beats / num);
             bar++)
    {
        const auto barStartBeat = bar * num;
        const auto barStartX = evs.beatsToX (
            barStartBeat, boundingRect.getWidth(), x1beats, x2beats);

        for (auto b = 0; b < num; b++)
        {
            const auto beatWidth = evs.beatsToX (
                x1beats + 1, boundingRect.getWidth (), x1beats, x2beats);
            const auto beatX = barStartX + beatWidth * b;
            const auto beatRect = juce::Rectangle<float>(
                (float) beatX + boundingRect.getX(), boundingRect.getY(), (float) beatWidth,
                boundingRect.toFloat().getHeight ());
            const auto barString = juce::String(bar + 1);
            const auto beatString = "." + juce::String(b + 1);

            if (b == 0 && printDescription && snapLevel > 5)
                printTextAt(g,
                            beatRect.withWidth(beatRect.getWidth() * 4),
                            barString,
                            textColour);

            if (snapLevel <= 7)
            {

                if (beatX + boundingRect.getX() > boundingRect.getX()
                    && beatX + boundingRect.getX() < boundingRect.getRight())
                {
                    g.setColour(beatColour);
                    g.drawVerticalLine(beatX + boundingRect.getX(), boundingRect.getY(), boundingRect.getBottom());
                }
            }

            if (snapLevel <= 5 && printDescription)
                printTextAt(
                    g, beatRect, barString + beatString, textColour);

            if (snapLevel <= 3)
            {
                const int den = evs.m_edit.tempoSequence.getTimeSigAt(
                        tracktion::TimePosition::fromSeconds(0)).denominator;
                const float frac = beatRect.getWidth() / (16 / den);

                for (auto i = 1; i <= 16 / den; i++)
                {
                    auto fracRect = beatRect.withTrimmedLeft(i * frac);
                    auto fracString = "." + juce::String(i + 1);

                    if (printDescription)
                        printTextAt(g,
                                    fracRect,
                                    barString + beatString + fracString,
                                    textColour);

                    if (fracRect.getX() > boundingRect.getX()
                        && fracRect.getX() < boundingRect.getRight())
                    {
                        g.setColour(fracColour);
                        g.drawVerticalLine(
                        fracRect.getX(),boundingRect.getY(), boundingRect.getBottom());
                    }
                }
            }
        }

        if (barStartX + boundingRect.getX() > boundingRect.getX())
        {
            g.setColour (BarColour);
            g.drawVerticalLine(barStartX + boundingRect.getX(), boundingRect.getY(), boundingRect.getBottom());

        }
    }
}
void GUIHelpers::printTextAt(juce::Graphics& graphic,
                             juce::Rectangle<float> textRect,
                             const juce::String& text,
                             const juce::Colour& textColour)
{
        graphic.setColour(textColour);
        graphic.drawText(
            "  " + text, textRect, juce::Justification::centredLeft, false);
}

void GUIHelpers::drawRectWithShadow(juce::Graphics& g,
                            juce::Rectangle<float> area,
                            float cornerSize,
                            const juce::Colour& colour,
                            const juce::Colour& shade)
{
    g.setColour(shade);
    g.fillRoundedRectangle(area.translated(2, 2), cornerSize);
    g.setColour(colour);
    g.fillRoundedRectangle(area, cornerSize);
}

void GUIHelpers::drawCircleWithShadow(juce::Graphics& g,
                          juce::Rectangle<float> area,
                          const juce::Colour& colour,
                          const juce::Colour& shade)
{
    g.setColour(shade);
    g.fillEllipse(area.translated(2, 2));
    g.setColour(colour);
    g.fillEllipse(area);
}
void GUIHelpers::drawSnapLines(juce::Graphics& g,
                               const EditViewState& evs,
                               double x1beats,
                               double x2beats,
                               const juce::Rectangle<int>& boundingRect,
                               const juce::Colour& colour)
{
    auto snapType = evs.getBestSnapType(x1beats, x2beats, boundingRect.getWidth());
    auto it = 0.0;

    while (it <= x2beats)
    {
        if (it >= x1beats)
        {
            g.setColour(colour);
            int x = boundingRect.getX() + evs.beatsToX(it, boundingRect.getWidth(), x1beats, x2beats);
            g.drawVerticalLine(x, boundingRect.getY(), boundingRect.getBottom());
        }

        auto& tempo = evs.m_edit.tempoSequence.getTempoAt(tracktion::BeatPosition::fromBeats(it));
        auto delta = evs.timeToBeat(snapType.getApproxIntervalTime(tempo).inSeconds());
        it = it + delta;
    }
}

void GUIHelpers::drawBarBeatsShadow(juce::Graphics& g,
                                     const EditViewState& evs,
                                     double x1beats,
                                     double x2beats,
                                     const juce::Rectangle<int>& boundingRect,
                                     const juce::Colour& shade)
{
    const te::TimecodeSnapType& snapType =
        evs.getBestSnapType(x1beats,x2beats, boundingRect.getWidth());
    int num = evs.m_edit.tempoSequence.getTimeSigAt(tracktion::TimePosition::fromSeconds(0)).numerator;
    int shadowBeatDelta = num * 4;
    if (snapType.getLevel() <= 9)
        shadowBeatDelta = num;
    if (snapType.getLevel() <= 4)
        shadowBeatDelta = 1;

    auto beatIter = static_cast<int>(x1beats);
    while(beatIter % shadowBeatDelta != 0)
        beatIter--;

    while (beatIter <= x2beats)
    {
        if ((beatIter/shadowBeatDelta) %2 == 0)
        {
            int x = evs.beatsToX(beatIter, boundingRect.getWidth(), x1beats, x2beats);
            int w = evs.beatsToX(beatIter + shadowBeatDelta, boundingRect.getWidth(), x1beats, x2beats) - x;
            juce::Rectangle<int> shadowRect {x + boundingRect.getX(), boundingRect.getY(), w, boundingRect.getHeight()};

            if (shadowRect.getX() < boundingRect.getX())
                shadowRect.removeFromLeft(boundingRect.getX() - shadowRect.getX());

            if (shadowRect.getRight() > boundingRect.getRight())
                shadowRect.removeFromRight(shadowRect.getRight() - boundingRect.getRight());

            g.setColour(shade);
            g.fillRect(shadowRect);
        }

        beatIter += shadowBeatDelta;
    }
}

void GUIHelpers::drawLogo (juce::Graphics& g, juce::Colour colour, float scale)
    {
        juce::Path logoPath;
        juce::AffineTransform transform;

        // square
        logoPath.startNewSubPath(12 , 2);
        logoPath.lineTo(3, 2);
        logoPath.quadraticTo(2,2, 2,3);

        logoPath.lineTo(2, 13);
        logoPath.quadraticTo(2,14,3,14);

        logoPath.lineTo(12, 14);
        logoPath.quadraticTo(13,14, 13, 13);
        logoPath.quadraticTo(9, 8, 13, 3);
        logoPath.quadraticTo(13,2, 12,2);
        logoPath.closeSubPath();

        // 
        // // Second path
        logoPath.addRoundedRectangle(juce::Rectangle<float>(15,2,4,12), 1.f);
        logoPath.addRoundedRectangle(juce::Rectangle<float>(20,2,4,12), 1.f);
        // //
        // // Triangle
        logoPath.startNewSubPath(26, 3);
        logoPath.quadraticTo(26,2,27,2);
        logoPath.lineTo(37,7);
        logoPath.quadraticTo(38,8,37,9);
        logoPath.lineTo(27,14);
        logoPath.quadraticTo(26,14, 26,13);
        logoPath.quadraticTo(29,8, 26,3);

        logoPath.closeSubPath();

        // Apply transformation and scale
        transform = juce::AffineTransform::scale(scale, scale);
        logoPath.applyTransform(transform);

        // Set the fill color
        g.setColour(colour);

        // Draw the logo path
        g.fillPath(logoPath);
    }

juce::String GUIHelpers::translate (juce::String stringToTranslate, ApplicationViewState& avs)
{
    juce::LocalisedStrings translations(avs.getFileToTranslation(), false);
    auto translatedString = translations.translate(stringToTranslate);

    return translatedString;
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

juce::StringArray PlayHeadHelpers::getTimeCodeParts(
        tracktion_engine::Edit &edit
      , double time)
{
    auto st = tracktion::TimePosition::fromSeconds(time);
    auto timeCode = edit.getTimecodeFormat().getString(edit.tempoSequence, st, false);

    juce::StringArray parts;
    parts.addTokens(timeCode, "|", "");

    return parts;
}
juce::String PlayHeadHelpers::barsBeatsString(
        tracktion_engine::Edit &edit
      , double time)
{
    return getTimeCodeParts(edit, time).joinIntoString(".");
}

te::AudioTrack::Ptr EngineHelpers::getAudioTrack(te::Track::Ptr track, EditViewState& evs)
{
    for (auto at : te::getAudioTracks(evs.m_edit))
    {
        if (at == track)
        {
            return at;
        }
    }
    return nullptr;
}

bool EngineHelpers::renderToNewTrack(EditViewState & evs, juce::Array<tracktion_engine::Track*> tracksToRender, tracktion::TimeRange range)
{
    auto sampleDir = juce::File(evs.m_applicationState.m_samplesDir);
    auto renderFile = sampleDir.getNonexistentChildFile("render", ".wav");
    
    juce::BigInteger tracksToDo{ 0 };

    auto allTracks = te::getAllTracks(evs.m_edit);

    for (auto* trackToRender : tracksToRender)
    {
        int index = allTracks.indexOf(trackToRender);
        if (index != -1) 
            tracksToDo.setBit(index);
    }

    te::Renderer::renderToFile("Render", renderFile, evs.m_edit, range, tracksToDo);

    EngineHelpers::loadAudioFileOnNewTrack(evs, renderFile, juce::Colours::plum, range.getStart().inSeconds());

    return true;
}

void EngineHelpers::renderEditToFile(EditViewState& evs, juce::File renderFile, tracktion::TimeRange range)
{
    if (!renderFile.create())
    {
        juce::Logger::writeToLog("Error: Could not create file. Check permissions.");
        return;
    }
    else
    {
        GUIHelpers::log("File exists");
    }

    if (range == tracktion::TimeRange{})
        range = {tracktion::TimePosition::fromSeconds(0.0), evs.m_edit.getLength()};

    if (te::getAudioTracks(evs.m_edit).size() == 0)
    {
        juce::Logger::writeToLog("Error: The edit contains no tracks.");
        return;
    }

    juce::BigInteger tracksToDo{ 0 };

    for (auto i = 0; i < te::getAllTracks(evs.m_edit).size(); ++i)
        tracksToDo.setBit(i);

    auto result = te::Renderer::renderToFile("Render", renderFile, evs.m_edit, range, tracksToDo);

    if (result)
    {
        juce::Logger::writeToLog("Rendering started successfully.");
    }
    else
    {
        juce::Logger::writeToLog("Error: Failed to start rendering.");
    }
}

void EngineHelpers::updateMidiInputs(EditViewState& evs, te::Track::Ptr track)    
{
    if (auto at = dynamic_cast<te::AudioTrack*>(track.get()))
    {
        if ( at->state.getProperty (IDs::isMidiTrack))
        {
            auto &dm = evs.m_edit.engine.getDeviceManager ();
            for (auto instance: evs.m_edit.getAllInputDevices())
            {
                if (auto midiIn = dynamic_cast<te::MidiInputDevice*>(&instance->getInputDevice ()))
                {

                    if (midiIn == dm.getDefaultMidiInDevice ())
                    {
                        instance->setTargetTrack(*at, 0, true);
                        evs.m_edit.restartPlayback();
                    }
                }
            }
            if (evs.m_isAutoArmed)
            {
                for (auto&i : evs.m_edit.getTrackList ())
                {
                    if (auto audioTrack = dynamic_cast<te::AudioTrack*>(i))
                    {
                        EngineHelpers::armTrack (*audioTrack,false);
                    }
                }
                EngineHelpers::armTrack (*at, true);
            }
        }
    }
}
te::MidiInputDevice& EngineHelpers::getVirtuelMidiInputDevice(te::Engine& engine)
{
    auto& dm = engine.getDeviceManager();
    for (auto i = 0; i < dm.getNumMidiInDevices(); i++)
    {
        auto midiInDev = dm.getMidiInDevice(i);
        if (midiInDev->getName() == "vitualMidiIn")
            return *midiInDev;
    }
    dm.createVirtualMidiDevice("vitualMidiIn");
    return *dm.getMidiInDevice(dm.getNumMidiInDevices() -1);
}
tracktion::core::TimePosition EngineHelpers::getTimePos(double t)
{
    return tracktion::core::TimePosition::fromSeconds(t);
}
void EngineHelpers::deleteSelectedClips(EditViewState &evs)
{
    for (auto selectedClip : evs.m_selectionManager
         .getSelectedObjects ()
         .getItemsOfType<te::Clip>())
    {
        if (selectedClip->getTrack() != nullptr)
        {
            for (auto ap : selectedClip->getTrack ()->getAllAutomatableParams ())
            {
                ap->getCurve ().removePointsInRegion (selectedClip->getEditTimeRange ());
            }

            selectedClip->removeFromParent ();
        }
    }
}

bool EngineHelpers::isTrackShowable(te::Track::Ptr track)
{
    if (track->isChordTrack()
        || track->isTempoTrack()
        || track->isMarkerTrack()
        || track->isArrangerTrack()
        || track->isAutomationTrack()
        || track->isMasterTrack()
    )
    {
        return false;
    }

    return true;
}
    
bool EngineHelpers::trackWantsClip(const te::Clip* clip,
                                    const te::Track* track) 
{
    if (track == nullptr)
        return false;
    if (track->isFolderTrack())
        return false;

    return
        clip->isMidi() ==
        static_cast<bool>(track->state.getProperty( IDs::isMidiTrack));
}

te::Track* EngineHelpers::getTargetTrack(te::Track* sourceTrack, int verticalOffset)
{
    if (sourceTrack == nullptr)
        return nullptr;

    auto &edit = sourceTrack->edit;
    auto tracks = getSortedTrackList(edit);
    auto targetIdx = tracks.indexOf(sourceTrack) + verticalOffset;
    auto targetTrack = tracks[targetIdx];

    return targetTrack;
}

juce::Array<te::Track*> EngineHelpers::getSortedTrackList(te::Edit& edit)
{
    juce::Array<te::Track*> tracks;

    edit.visitAllTracks ([&] (te::Track& t)
        {
            if (t.isAutomationTrack() || t.isArrangerTrack() || t.isChordTrack() || t.isMarkerTrack() || t.isTempoTrack() || t.isMasterTrack())
               return true;
            tracks.add (&t);
            return true; 
        }, true);

    return tracks;
}

bool EngineHelpers::isTrackItemInRange (te::TrackItem* ti,const tracktion::TimeRange& tr)
{
    return ti->getEditTimeRange().intersects(tr);
}
void EngineHelpers::moveSelectedClips(bool copy, double timeDelta, int verticalOffset, EditViewState& evs)
{
    auto selectedClips = evs.m_selectionManager.getItemsOfType<te::Clip>();
    auto tempPosition = evs.m_edit.getLength().inSeconds() * 100;

    if (verticalOffset == 0) EngineHelpers::copyAutomationForSelectedClips(timeDelta, evs.m_selectionManager, copy);

    juce::Array<te::Clip*> copyOfSelectedClips;

    for (auto sc : selectedClips)
    {
        auto targetTrack = EngineHelpers::getTargetTrack(sc->getTrack(), verticalOffset);

        if (EngineHelpers::trackWantsClip(sc, targetTrack))
        {
            auto newClip = te::duplicateClip(*sc);
            copyOfSelectedClips.add(newClip);
            newClip->setStart(newClip->getPosition().getStart() + tracktion::TimeDuration::fromSeconds(tempPosition), false, true);

            if (!copy)
                sc->removeFromParent();
            else
                evs.m_selectionManager.deselect(sc);
        }
    }

    for (auto newClip: copyOfSelectedClips)
    {
        auto pasteTime = newClip->getPosition().getStart().inSeconds() + timeDelta - tempPosition;
        auto targetTrack = EngineHelpers::getTargetTrack(newClip->getTrack(), verticalOffset);
                        
        if (EngineHelpers::trackWantsClip(newClip, targetTrack))
        {
            if (auto tct = dynamic_cast<te::ClipTrack*>(targetTrack))
            {
                tct->deleteRegion({tracktion::TimePosition::fromSeconds(pasteTime),
                                  newClip->getPosition().getLength()},
                                  &evs.m_selectionManager);
    
                if (auto owner = dynamic_cast<te::ClipOwner*>(targetTrack))
                    newClip->moveTo(*owner);
                newClip->setStart(tracktion::TimePosition::fromSeconds(pasteTime), false, true);
    
                evs.m_selectionManager.addToSelection(newClip);
            }
        }
    }

}

void EngineHelpers::duplicateSelectedClips(EditViewState& evs)
{ 
    moveSelectedClips(true, getTimeRangeOfSelectedClips(evs).getLength().inSeconds(), 0, evs);
}

void EngineHelpers::copyAutomationForSelectedClips(double offset
                                                 , te::SelectionManager& sm
                                                 , bool copy)
{
    const auto clipSelection = sm
            .getSelectedObjects ()
            .getItemsOfType<te::Clip>();
    if (clipSelection.size () > 0)
    {
        //collect automation sections
        juce::Array<te::TrackAutomationSection> sections;

        for (const auto& selectedClip : clipSelection)
        {
            if (selectedClip->getTrack() != nullptr)
                sections.add (te::TrackAutomationSection(*selectedClip));
        }

		te::moveAutomation (sections, tracktion::TimeDuration::fromSeconds(offset), copy);
    }
}
void EngineHelpers::selectAllClips(te::SelectionManager& sm, te::Edit& edit)
{
    sm.deselectAll();

    for (auto t : te::getAudioTracks(edit))
    {
        for (auto c : t->getClips())
        {
            sm.addToSelection(c);
        }
    }
}
void EngineHelpers::selectAllClipsOnTrack(te::SelectionManager& sm, te::AudioTrack& at)
{
    sm.deselectAll();

    for (auto c : at.getClips())
    {
        sm.addToSelection(c);
    }
}
static tracktion::AutomationCurve* getDestCurve (tracktion::Track& t, const tracktion::AutomatableParameter::Ptr& p)
{
    if (p != nullptr)
    {
        if (auto plugin = p->getPlugin())
        {
            auto name = plugin->getName();

            for (auto f : t.getAllPlugins())
                if (f->getName() == name)
                    if (auto param = f->getAutomatableParameter (plugin->indexOfAutomatableParameter (p)))
                        return &param->getCurve();
        }
    }

    return {};
}
static bool mergeInto (const tracktion::TrackAutomationSection& s,
                       juce::Array<tracktion::TrackAutomationSection>& dst)
{
    for (auto& dstSeg : dst)
    {
        if (dstSeg.overlaps (s))
        {
            dstSeg.mergeIn (s);
            return true;
        }
    }

    return false;
}

static void mergeSections (const juce::Array<tracktion::TrackAutomationSection>& src,
                           juce::Array<tracktion::TrackAutomationSection>& dst)
{
    for (const auto& srcSeg : src)
        if (! mergeInto (srcSeg, dst))
            dst.add (srcSeg);
}

void EngineHelpers::moveAutomationOrCopy(const juce::Array<tracktion::TrackAutomationSection>& origSections, tracktion::TimeDuration offset, bool copy)
{
    if (origSections.isEmpty())
        return;

    juce::Array<tracktion::TrackAutomationSection> sections;
    mergeSections (origSections, sections);

    // find all the original curves
    for (auto&& section : sections)
    {
        for (auto& ap : section.activeParameters)
            ap.curve.state = ap.curve.state.createCopy();
    }

    // delete all the old curves
    if (! copy)
    {
        for (auto& section : sections)
        {
            auto sectionTime = section.position;

            for (auto&& activeParam : section.activeParameters)
            {
                auto param = activeParam.param;
                auto& curve = param->getCurve();
                constexpr auto tolerance = tracktion::TimeDuration::fromSeconds (0.0001);

                auto startValue = curve.getValueAt (sectionTime.getStart() - tolerance);
                auto endValue   = curve.getValueAt (sectionTime.getEnd()   + tolerance);

                auto idx = curve.indexBefore (sectionTime.getEnd() + tolerance);
                auto endCurve = (idx == -1) ? 0.0f : curve.getPointCurve(idx);

                curve.removePointsInRegion (sectionTime.expanded (tolerance));

                if (std::abs (startValue - endValue) < 0.0001f)
                {
                    curve.addPoint (sectionTime.getStart(), startValue, 0.0f);
                    curve.addPoint (sectionTime.getEnd(), endValue, endCurve);
                }
                else if (startValue > endValue)
                {
                    curve.addPoint (sectionTime.getStart(), startValue, 0.0f);
                    curve.addPoint (sectionTime.getStart(), endValue, 0.0f);
                    curve.addPoint (sectionTime.getEnd(), endValue, endCurve);
                }
                else
                {
                    curve.addPoint (sectionTime.getStart(), startValue, 0.0f);
                    curve.addPoint (sectionTime.getEnd(), startValue, 0.0f);
                    curve.addPoint (sectionTime.getEnd(), endValue, endCurve);
                }

                curve.removeRedundantPoints (sectionTime.expanded (tolerance));
            }
        }
    }

    // recreate the curves
    for (auto& section : sections)
    {
        for (auto& activeParam : section.activeParameters)
        {
            auto sectionTime = section.position;

            if (auto dstCurve = (section.src == section.dst) ? &activeParam.param->getCurve()
                                                             : getDestCurve (*section.dst, activeParam.param))
            {
                constexpr auto errorMargin = tracktion::TimeDuration::fromSeconds (0.0001);

                auto start    = sectionTime.getStart();
                auto end      = sectionTime.getEnd();
                auto newStart = start + offset;
                auto newEnd   = end   + offset;

                auto& srcCurve = activeParam.curve;

                auto idx1 = srcCurve.indexBefore (newEnd + errorMargin);
                auto endCurve = idx1 < 0 ? 0 : srcCurve.getPointCurve (idx1);

                auto idx2 = srcCurve.indexBefore (start - errorMargin);
                auto startCurve = idx2 < 0 ? 0 : srcCurve.getPointCurve (idx2);

                auto srcStartVal = srcCurve.getValueAt (start - errorMargin);
                auto srcEndVal   = srcCurve.getValueAt (end   + errorMargin);

                auto dstStartVal = dstCurve->getValueAt (newStart - errorMargin);
                auto dstEndVal   = dstCurve->getValueAt (newEnd   + errorMargin);

                tracktion::TimeRange totalRegionWithMargin  (newStart - errorMargin, newEnd   + errorMargin);
                tracktion::TimeRange startWithMargin        (newStart - errorMargin, newStart + errorMargin);
                tracktion::TimeRange endWithMargin          (newEnd   - errorMargin, newEnd   + errorMargin);

                juce::Array<tracktion::AutomationCurve::AutomationPoint> origPoints;

                for (int i = 0; i < srcCurve.getNumPoints(); ++i)
                {
                    auto pt = srcCurve.getPoint (i);

                    if (pt.time >= start - errorMargin && pt.time <= sectionTime.getEnd() + errorMargin)
                        origPoints.add (pt);
                }

                dstCurve->removePointsInRegion (totalRegionWithMargin);

                for (const auto& pt : origPoints)
                    dstCurve->addPoint (pt.time + offset, pt.value, pt.curve);

                auto startPoints = dstCurve->getPointsInRegion (startWithMargin);
                auto endPoints   = dstCurve->getPointsInRegion (endWithMargin);

                dstCurve->removePointsInRegion (startWithMargin);
                dstCurve->removePointsInRegion (endWithMargin);

                dstCurve->addPoint (newStart, dstStartVal, startCurve);
                dstCurve->addPoint (newStart, srcStartVal, startCurve);

                for (auto& point : startPoints)
                    dstCurve->addPoint (newStart, point.value, point.curve);

                for (auto& point : endPoints)
                    dstCurve->addPoint (newEnd, point.value, point.curve);

                dstCurve->addPoint (newEnd, srcEndVal, endCurve);
                dstCurve->addPoint (newEnd, dstEndVal, endCurve);

                dstCurve->removeRedundantPoints (totalRegionWithMargin);
            }
        }
    }

    // activate the automation curves on the new tracks
    juce::Array<tracktion::Track*> src, dst;

    for (auto& section : sections)
    {
        if (section.src != section.dst)
        {
            if (! src.contains (section.src.get()))
            {
                src.add (section.src.get());
                dst.add (section.dst.get());
            }
        }
    }

    for (int i = 0; i < src.size(); ++i)
    {
        if (auto ap = src.getUnchecked (i)->getCurrentlyShownAutoParam())
        {
            for (auto p : dst.getUnchecked (i)->getAllAutomatableParams())
            {
                if (p->getPluginAndParamName() == ap->getPluginAndParamName())
                {
                    dst.getUnchecked (i)->setCurrentlyShownAutoParam (p);
                    break;
                }
            }
        }
    }
}



void EngineHelpers::moveAutomation(te::Track* src,te::TrackAutomationSection::ActiveParameters par, tracktion::TimeRange range, double insertTime, bool copy)
{
	te::TrackAutomationSection section;
	section.src = src;
	section.dst = src;
	section.position = range;
	section.activeParameters.add(par);
	
	juce::Array<te::TrackAutomationSection> secs;
	secs.add (section);
	auto offset = tracktion::TimePosition::fromSeconds(insertTime) - range.getStart();

	te::moveAutomation(secs, offset, copy);
}

te::TrackAutomationSection EngineHelpers::getTrackAutomationSection(te::AutomatableParameter* ap, tracktion::TimeRange tr)
{
    te::TrackAutomationSection as;
    as.src = ap->getTrack();
    as.dst = ap->getTrack();
    as.position = tr;
    te::TrackAutomationSection::ActiveParameters par;
    par.param = ap;
    par.curve = ap->getCurve();
    as.activeParameters.add(par);

    return as;
}
void EngineHelpers::resizeSelectedClips(bool fromLeftEdge, double delta, EditViewState & evs)
{
    auto selectedClips = evs.m_selectionManager.getItemsOfType<te::Clip>();
    auto tempPosition = evs.m_edit.getLength().inSeconds() * 100;

    if (fromLeftEdge)
	{
        for (auto sc : selectedClips)
        {
            auto newStart = juce::jmax(sc->getPosition().getStart() - sc->getPosition().getOffset(),
                                       sc->getPosition().getStart() + tracktion::TimeDuration::fromSeconds(delta));
            sc->setStart(newStart, true, false);

			//save clip for damage
            sc->setStart(sc->getPosition().getStart() + tracktion::TimeDuration::fromSeconds(tempPosition), false, true);
        }
	}
    else
    {
        for (auto sc : selectedClips)
        {
            auto newEnd = sc->getPosition().getEnd() + tracktion::TimeDuration::fromSeconds(delta);

            sc->setEnd(newEnd, true);
			//save clip for damage
            sc->setStart(sc->getPosition().getStart() + tracktion::TimeDuration::fromSeconds(tempPosition), false, true);
        }
    }

    for (auto sc : selectedClips)
    {
        if (auto ct = sc->getClipTrack())
        {
            const tracktion::TimeRange range = {sc->getPosition().getStart() - tracktion::TimeDuration::fromSeconds(tempPosition),
                                               sc->getPosition().getEnd() - tracktion::TimeDuration::fromSeconds(tempPosition)};
            ct->deleteRegion(range, &evs.m_selectionManager);
        }

        //restore clip
        sc->setStart(sc->getPosition().getStart() - tracktion::TimeDuration::fromSeconds(tempPosition), false, true);
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
        clips.getUnchecked (i)->removeFromParent();
}

tracktion_engine::AudioTrack *EngineHelpers::getOrInsertAudioTrackAt(
        tracktion_engine::Edit &edit
      , int index)
{
    edit.ensureNumberOfAudioTracks (index + 1);
    return te::getAudioTracks (edit)[index];
}
tracktion_engine::FolderTrack::Ptr EngineHelpers::addFolderTrack(
    juce::Colour trackColour,
    EditViewState &evs)
{
    te::TrackInsertPoint tip (*te::getAllTracks(evs.m_edit).getLast(), false);
    auto ft = evs.m_edit.insertNewFolderTrack(tip, &evs.m_selectionManager, true);

    ft->state.setProperty (te::IDs::height
                             , (int) evs.m_trackDefaultHeight
                             , nullptr);
    ft->state.setProperty (IDs::isTrackMinimized, false, nullptr);

    ft->state.setProperty(  IDs::isMidiTrack
                             , false
                             , &evs.m_edit.getUndoManager());

    juce::String num = juce::String(te::getAudioTracks(evs.m_edit).size());
    ft->setName("Folder " + num);
    ft->setColour(trackColour);
    evs.m_selectionManager.selectOnly(ft);
    return ft;
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
         track->state.setProperty (IDs::isTrackMinimized, false, nullptr);

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

tracktion_engine::WaveAudioClip::Ptr EngineHelpers::loadAudioFileOnNewTrack(
        EditViewState &evs
      , const juce::File &file
      , juce::Colour trackColour
      , double insertTime)
{
    te::AudioFile audioFile (evs.m_edit.engine, file);
    if (audioFile.isValid())
    {
        std::cout << "audio file valid" << std::endl;
        std::cout << "audio file length: " << audioFile.getLength() << std::endl;
        if (auto track = addAudioTrack(false, trackColour, evs))
        {           
            
			removeAllClips (*track);
            te::ClipPosition pos;
            pos.time = {tracktion::TimePosition::fromSeconds(insertTime),  tracktion::TimeDuration::fromSeconds (audioFile.getLength())};
            auto name = file.getFileNameWithoutExtension();
            if (auto newClip = track->insertWaveClip (name, file, pos, true))
            {
				GUIHelpers::log("loading : " + file.getFullPathName ());
				newClip->setAutoTempo(false);
				newClip->setAutoPitch(false);
                newClip->setPosition(pos);
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
                auto source = evs.m_edit.filePathResolver(c->state.getProperty (te::IDs::source));
                auto relPath = source.getRelativePathFrom (newFile.getParentDirectory ());

                c->state.setProperty (te::IDs::source, relPath, nullptr);
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

void EngineHelpers::loopAroundSelection (EditViewState &evs)
{
    auto& transport = evs.m_edit.getTransport ();

    transport.setLoopRange (getTimeRangeOfSelectedClips(evs));
    transport.looping = true;
}

tracktion::TimeRange EngineHelpers::getTimeRangeOfSelectedClips(EditViewState &evs)
{
    if (evs.m_selectionManager.getItemsOfType<te::Clip>().size() > 0)
    {
        auto end = tracktion::TimePosition::fromSeconds(0);
        auto start = end + evs.m_edit.getLength();

        for (auto c : evs.m_selectionManager.getItemsOfType<te::Clip>())
        {
            if (c->getPosition().getStart() < start)
                start = c->getPosition().getStart();
            if (c->getPosition().getEnd() > end)
                end = c->getPosition().getEnd();
        }

        if (start == end)
            return {};
        if (end < start)
            return {};

        return {start, end};
    }
    return {};
}

void EngineHelpers::loopOff (te::Edit& edit)
{
    auto& transport = edit.getTransport ();
    transport.looping = false;
}

void EngineHelpers::loopOn (te::Edit& edit)
{
    auto& transport = edit.getTransport ();
    transport.looping = true;
}
void EngineHelpers::loopAroundAll(te::Edit &edit)
{
    auto& transport = edit.getTransport ();
    transport.setLoopRange ({tracktion::TimePosition::fromSeconds(0), edit.getLength()});
    transport.looping = true;
}

void EngineHelpers::toggleSnap (EditViewState &evs)
{

    if (evs.m_snapToGrid)
        evs.m_snapToGrid = false;
    else
        evs.m_snapToGrid = true;
}

void EngineHelpers::toggleMetronome (te::Edit& edit)
{
    GUIHelpers::log("toggle metronome");
    edit.clickTrackEnabled = !edit.clickTrackEnabled;
}
void EngineHelpers::play (EditViewState &evs)
{
    GUIHelpers::log("play");
    auto& transport = evs.m_edit.getTransport ();
 
    if (transport.isPlaying ())
        transport.setCurrentPosition (evs.m_playHeadStartTime);
    //hack for prevent not playing the first transient of a sample
    //that starts direct on play position
    auto currentPos = transport.getCurrentPosition();
    transport.setCurrentPosition(evs.m_edit.getLength().inSeconds());
    transport.play (true);
    transport.setCurrentPosition(currentPos);
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

bool EngineHelpers::isInputMonitoringEnabled(tracktion_engine::Track &t, int position)
{
    auto& edit = t.edit;
    for (auto instance : edit.getAllInputDevices())
        if (instance->isOnTargetTrack (t, position))
            return instance->getInputDevice().isEndToEndEnabled();

    return false;
}

void EngineHelpers::enableInputMonitoring(
        tracktion_engine::Track &t
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

bool EngineHelpers::trackHasInput(tracktion_engine::Track &t, int position)
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
        smartThumbnail.drawChannels (g, r, { tracktion::TimePosition::fromSeconds(0.0), tracktion::TimeDuration::fromSeconds(smartThumbnail.getTotalLength()) }, 1.0f);
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
    transport.position = tracktion::TimePosition::fromSeconds(transport.getLoopRange().getLength().inSeconds()* proportion);
}

void Thumbnail::mouseUp(const juce::MouseEvent &)
{
    transport.setUserDragging (false);
}

void Thumbnail::updateCursorPosition()
{
    const double loopLength = transport.getLoopRange().getLength().inSeconds();
    const double proportion = loopLength == 0.0 ? 0.0 : transport.getCurrentPosition() / loopLength;

    auto r = getLocalBounds().toFloat();
    const float x = r.getWidth() * float (proportion);
    cursor.setRectangle (r.withWidth (2.0f).withX (x));
}

void EngineHelpers::insertPlugin (te::Track::Ptr track, te::Plugin::Ptr plugin, int index)
{
    auto& plugins = track->pluginList;
    if (index == -1)
        index = plugins.size() - 2;
    plugin->state.setProperty (te::IDs::remapOnTempoChange, true, nullptr);
    plugins.insertPlugin (plugin->state, index);
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
    auto zoom = evs.m_viewX2 - evs.m_viewX1;
    evs.m_viewX1 = newBeatPos;
    evs.m_viewX2 = newBeatPos + zoom;
}

float GUIHelpers::getZoomScaleFactor(int delta, float unitDistance)
{
    return std::pow (2,(float) delta / unitDistance);
}
juce::Rectangle<int> GUIHelpers::getSensibleArea(juce::Point<int> p, int w)
{
    return {p.x - (w/2), p.y - (w/2), w, w};
}
bool GUIHelpers::isAutomationVisible(const te::AutomatableParameter& ap)
{
    return ap.getCurve().getNumPoints() > 0;
}
    
void GUIHelpers::centerMidiEditorToClip(EditViewState& evs, te::Clip::Ptr c)
{
    auto zoom = evs.m_pianoX2 - evs.m_pianoX1;

    evs.m_pianoX1 =
        juce::jmax(0.0, c->getStartBeat ().inBeats() - (zoom /2) + (c->getLengthInBeats ().inBeats()/2));
    evs.m_pianoX2 = evs.m_pianoX1 + zoom;
}

void GUIHelpers::drawPolyObject (juce::Graphics &g, juce::Rectangle<int> area, int edges, float tilt, float rotation,float radiusFac, float heightFac, float scale)
{
    const float pi = static_cast<float> (3.141592653589793238L);
    auto phi = 0.f + tilt;

    auto xm = area.getWidth() / 2;
    auto rx = (area.getHeight() / 3) * radiusFac * scale;
    auto yRot = juce::jmap (rotation, 0.f , static_cast<float>(rx));

    auto ry = ((area.getHeight() / 3) * scale * radiusFac) - yRot;

    int x = xm + rx * sinf(phi);
    int zLength = ((area.getHeight() - (area.getHeight() - juce::jmap(rotation, 0.f, static_cast<float>(area.getHeight())))) * heightFac) * 2 * scale;

    auto ym = area.getHeight() / 2 + (zLength/2);
    int y = ym + ry * cosf(phi);

    juce::Path poly;

    while (phi < (2 * pi) + tilt)
    {
        auto oldX = x;
        auto oldY = y;

        phi = phi + (2 * pi / edges);

        x = xm + rx * sinf(phi);
        y = ym + ry * cosf(phi);

        juce::Line<float> zEdge (oldX, oldY - zLength, oldX, oldY);
        juce::Line<float> topEdge (oldX, oldY - zLength, x, y - zLength);
        juce::Line<float> bottomEdge (oldX, oldY, x, y);

        poly.addLineSegment (zEdge, 2);
        poly.addLineSegment (topEdge, 2);
        poly.addLineSegment (bottomEdge, 2);
    }
    auto st = juce::PathStrokeType(2);
    st.setJointStyle (juce::PathStrokeType::JointStyle::beveled);

    g.strokePath (poly, st);
}

void GUIHelpers::drawLogoQuad (juce::Graphics &g, juce::Rectangle<int> area)
{
    const float pi = static_cast<float> (3.141592653589793238L);
    juce::Path path;
    auto roundEdge = 50;

    path.startNewSubPath (area.getX(), area.getY() + roundEdge/2);
    path.addArc (area.getX() ,area.getY(),roundEdge,roundEdge, pi + (pi/2) , 2*pi );
    path.lineTo (area.getWidth() - roundEdge, area.getY());
    path.addArc (area.getWidth() - roundEdge, area.getY(), roundEdge, roundEdge, pi + (pi/2)+ (pi/2), 2*pi+ (pi/2) );

    g.strokePath (path, juce::PathStrokeType(2));
}
