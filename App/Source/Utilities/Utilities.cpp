/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#include "Utilities/Utilities.h"

#include "BinaryData.h"
#include "LowerRange/PluginChain/PresetHelpers.h"
#include "Plugins/Arpeggiator/ArpeggiatorPlugin.h"
#include "Plugins/SimpleSynth/SimpleSynthPlugin.h"
#include "Plugins/SpectrumAnalyzer/SpectrumAnalyzerPlugin.h"
#include "Utilities/EditViewState.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_graphics/native/juce_EventTracing.h"
#include "tracktion_core/utilities/tracktion_Time.h"
#include "tracktion_core/utilities/tracktion_TimeRange.h"

void Helpers::addAndMakeVisible(juce::Component &parent, const juce::Array<juce::Component *> &children)
{
    for (auto c : children)
    {
        parent.addAndMakeVisible(c);
        c->setWantsKeyboardFocus(false);
    }
}

juce::String Helpers::getStringOrDefault(const juce::String &stringToTest, const juce::String &stringToReturnIfEmpty) { return stringToTest.isEmpty() ? stringToReturnIfEmpty : stringToTest; }

juce::File Helpers::findRecentEdit(const juce::File &dir)
{
    GUIHelpers::log("Utilities: search for temp file: " + dir.getFullPathName());
    auto files = dir.findChildFiles(juce::File::findFiles, false, "*.nextTemp");
    if (files.size() > 0)
    {
        GUIHelpers::log("Utilities: found at least 1 File");
        files.sort();
        return files.getLast();
    }
    return {};
}
void GUIHelpers::drawTrack(juce::Graphics &g, juce::Component &parent, EditViewState &evs, juce::Rectangle<float> displayedRect, te::ClipTrack::Ptr clipTrack, tracktion::TimeRange etr, bool forDragging)
{
    double x1beats = evs.timeToBeat(etr.getStart().inSeconds());
    double x2beats = evs.timeToBeat(etr.getEnd().inSeconds());

    g.setColour(evs.m_applicationState.getTrackBackgroundColour());
    g.fillRect(displayedRect);

    auto ta = etr.getStart().inSeconds();
    auto te = etr.getEnd().inSeconds();

    auto ba = evs.timeToBeat(ta);
    auto be = evs.timeToBeat(te);

    drawBarsAndBeatLines(g, evs, x1beats, x2beats, displayedRect);

    auto firstIdx = clipTrack->getIndexOfNextTrackItemAt(tracktion::TimePosition::fromSeconds(ta));
    auto lastIdx = clipTrack->getIndexOfNextTrackItemAt(tracktion::TimePosition::fromSeconds(te)) + 1;

    for (auto clipIdx = firstIdx; clipIdx < lastIdx; clipIdx++)
    {
        if (clipIdx < clipTrack->getNumTrackItems())
        {
            auto clip = clipTrack->getTrackItem(clipIdx);

            if (clip->getPosition().time.intersects(etr))
            {
                float x = displayedRect.getX() + evs.timeToX(clip->getPosition().getStart().inSeconds(), displayedRect.getWidth(), x1beats, x2beats);
                float y = displayedRect.getY();
                float w = (displayedRect.getX() + evs.timeToX(clip->getPosition().getEnd().inSeconds(), displayedRect.getWidth(), x1beats, x2beats)) - x;
                float h = displayedRect.getHeight();

                juce::Rectangle<float> clipRect(x, y, w, h);

                auto color = clip->getTrack()->getColour();

                if (forDragging)
                    color = color.withAlpha(0.7f);

                if (auto c = dynamic_cast<te::Clip *>(clip))
                    drawClip(g, parent, evs, clipRect, c, color, displayedRect, x1beats, x2beats);
            }
        }
    }

    g.setColour(juce::Colour(0x60ffffff));
    g.drawLine(displayedRect.getX(), displayedRect.getBottom(), displayedRect.getRight(), displayedRect.getBottom(), 1.0f);
}
void GUIHelpers::drawClip(juce::Graphics &g, juce::Component &parent, EditViewState &evs, juce::Rectangle<float> clipRect, te::Clip *clip, juce::Colour color, juce::Rectangle<float> displayedRect, double x1Beat, double x2beat)
{
    auto isSelected = evs.m_selectionManager.isSelected(clip);
    drawClipBody(g, evs, clip->getName(), clipRect, isSelected, color, displayedRect, x1Beat, x2beat);

    auto header = clipRect.withHeight(static_cast<float>(evs.m_clipHeaderHeight));
    auto contentRect = clipRect.withTrimmedTop(header.getHeight()).reduced(1.0f, 1.0f);

    if (auto wac = dynamic_cast<te::WaveAudioClip *>(clip))
    {
        auto waveformArea = displayedRect.withTrimmedTop(header.getHeight());
        waveformArea.reduce(1, 2);
        if (auto thumb = evs.getOrCreateThumbnail(wac))
            // evs.m_thumbNailManager->drawThumbnail(wac, g, waveformArea, evs.beatToTime(x1Beat), evs.beatToTime(x2beat));
            GUIHelpers::drawWaveform(g, evs, *wac, *thumb, color, contentRect, displayedRect, x1Beat, x2beat);
    }
    else if (auto mc = dynamic_cast<te::MidiClip *>(clip))
    {
        drawMidiClip(g, evs, mc, contentRect, displayedRect, color, x1Beat, x2beat);
    }

    g.setColour(evs.m_applicationState.getTimeLineStrokeColour());
    g.drawRect(clipRect.toNearestInt());

    if (isSelected)
    {
        g.setColour(evs.m_applicationState.getPrimeColour());
        g.drawRect(clipRect.toNearestInt(), 2);
    }
}

void GUIHelpers::drawWaveform(juce::Graphics &g, EditViewState &evs, te::AudioClipBase &c, SimpleThumbnail &thumb, juce::Colour colour, juce::Rectangle<float> clipRect, juce::Rectangle<float> displayedRect, double x1Beat, double x2beat)
{
    if (evs.m_drawWaveforms == false)
        return;

    auto getTimeRangeForDrawing = [](EditViewState &evs, const te::AudioClipBase &clip, const juce::Rectangle<float> clRect, const juce::Rectangle<float> displRect, double x1Beats, double x2Beats) -> tracktion::core::TimeRange
    {
        auto t1 = EngineHelpers::getTimePos(0.0);
        auto t2 = t1 + clip.getPosition().getLength();

        double displStart = evs.beatToTime(x1Beats);
        double displEnd = evs.beatToTime(x2Beats);

        if (clRect.getX() < displRect.getX())
            t1 = t1 + tracktion::TimeDuration::fromSeconds(displStart - clip.getPosition().getStart().inSeconds());

        if (clRect.getRight() > displRect.getRight())
            t2 = t2 - tracktion::TimeDuration::fromSeconds(clip.getPosition().getEnd().inSeconds() - displEnd);

        return {t1, t2};
    };

    auto area = clipRect;
    if (clipRect.getX() < displayedRect.getX())
        area.removeFromLeft(displayedRect.getX() - clipRect.getX());
    if (clipRect.getRight() > displayedRect.getRight())
        area.removeFromRight(clipRect.getRight() - displayedRect.getRight());

    const auto gain = c.getGain();
    const auto pan = thumb.getNumChannels() == 1 ? 0.0f : c.getPan();

    const float pv = pan * gain;
    const float gainL = (gain - pv);
    const float gainR = (gain + pv);

    const bool usesTimeStretchedProxy = c.usesTimeStretchedProxy();

    const auto clipPos = c.getPosition();
    auto offset = clipPos.getOffset();
    auto speedRatio = c.getSpeedRatio();

    g.setColour(colour);

    bool showBothChannels = displayedRect.getHeight() > 100;

    if (usesTimeStretchedProxy)
    {

        // if (!thumb.isOutOfDate())
        {
            drawChannels(g, thumb, area, false, getTimeRangeForDrawing(evs, c, clipRect, displayedRect, x1Beat, x2beat), c.isLeftChannelActive() && showBothChannels, c.isRightChannelActive(), gainL, gainR);
        }
    }
    else if (c.getLoopLength().inSeconds() == 0)
    {
        auto region = getTimeRangeForDrawing(evs, c, clipRect, displayedRect, x1Beat, x2beat);

        auto t1 = EngineHelpers::getTimePos((region.getStart().inSeconds() + offset.inSeconds()) * speedRatio);
        auto t2 = EngineHelpers::getTimePos((region.getEnd().inSeconds() + offset.inSeconds()) * speedRatio);
        bool useHighres = true;
        drawChannels(g, thumb, area, useHighres, {t1, t2}, c.isLeftChannelActive(), c.isRightChannelActive() && showBothChannels, gainL, gainR);
    }
}

void GUIHelpers::drawChannels(juce::Graphics &g, SimpleThumbnail &thumb, juce::Rectangle<float> drawRect, bool useHighRes, tracktion::core::TimeRange time, bool useLeft, bool useRight, float leftGain, float rightGain)
{
    auto area = drawRect;

    thumb.setUseCustomDrawing(true);
    if (useLeft && useRight && thumb.getNumChannels() > 1)
    {
        int channelHeight = area.getHeight() / thumb.getNumChannels();
        for (int channel = 0; channel < thumb.getNumChannels(); channel++)
        {
            thumb.drawChannels(g, area.removeFromTop(channelHeight), time.getStart().inSeconds(), time.getEnd().inSeconds(), channel, channel == 0 ? leftGain : rightGain);
        }
    }
    else if (useLeft)
        thumb.drawChannels(g, area, time.getStart().inSeconds(), time.getEnd().inSeconds(), 0, leftGain);
    else if (useRight)
        thumb.drawChannels(g, area, time.getStart().inSeconds(), time.getEnd().inSeconds(), 1, rightGain);
}

void GUIHelpers::drawClipBody(juce::Graphics &g, EditViewState &evs, juce::String name, juce::Rectangle<float> clipRect, bool isSelected, juce::Colour color, juce::Rectangle<float> displayedRect, double x1Beat, double x2beat)
{
    auto area = clipRect;
    auto header = area.withHeight(static_cast<float>(evs.m_clipHeaderHeight));

    auto clipColor = color;
    auto innerGlow = clipColor.brighter(0.5f);
    auto borderColour = clipColor.darker(0.95f);
    auto backgroundColor = borderColour.withAlpha(0.6f);

    auto labelTextColor = clipColor.getPerceivedBrightness() < 0.5f ? clipColor.withLightness(.75f) : clipColor.darker(.75f);

    float startOffset = displayedRect.getX() - area.getX();
    float endOffset;

    auto clipedClip = area.withLeft(area.getX() + juce::jmax(0.0f, startOffset));
    endOffset = clipedClip.getRight() - displayedRect.getRight();
    clipedClip = clipedClip.withRight(clipedClip.getRight() - juce::jmax(0.0f, endOffset)).reduced(0.0f, 1.0f);
    auto clipedHeader = clipedClip.withHeight(static_cast<float>(evs.m_clipHeaderHeight));

    // Determine if clip extends beyond view boundaries
    float clipExtendsLeft = displayedRect.getX() - area.getX();
    float clipExtendsRight = area.getRight() - displayedRect.getRight();

    g.saveState();
    {
        g.reduceClipRegion(displayedRect.toNearestInt());

        g.setColour(backgroundColor);
        g.fillRect(area.reduced(1.0f, 1.0f));

        g.setColour(innerGlow);

        // Draw borders but avoid edges that extend beyond view
        if (clipExtendsLeft > 0.0)
        {
            // Skip left edge
            g.drawLine(clipedHeader.getX(), clipedHeader.getY(), clipedHeader.getRight(), clipedHeader.getY(), 1.0f);           // Top
            g.drawLine(clipedHeader.getRight(), clipedHeader.getY(), clipedHeader.getRight(), clipedHeader.getBottom(), 1.0f);  // Right
            g.drawLine(clipedHeader.getX(), clipedHeader.getBottom(), clipedHeader.getRight(), clipedHeader.getBottom(), 1.0f); // Bottom

            // Main clip borders
            g.drawLine(clipedClip.getX(), clipedClip.getY(), clipedClip.getRight(), clipedClip.getY(), 1.0f);           // Top
            g.drawLine(clipedClip.getRight(), clipedClip.getY(), clipedClip.getRight(), clipedClip.getBottom(), 1.0f);  // Right
            g.drawLine(clipedClip.getX(), clipedClip.getBottom(), clipedClip.getRight(), clipedClip.getBottom(), 1.0f); // Bottom
            g.setColour(isSelected ? clipColor.interpolatedWith(juce::Colours::blanchedalmond, 0.5f) : clipColor);
            auto chi = clipedHeader.reduced(0.0f, 2.0f);
            chi.removeFromRight(juce::jmin(clipExtendsLeft, 2.0f));
            g.fillRect(chi);
        }
        else if (clipExtendsRight > 0.0)
        {
            // Skip right edge
            g.drawLine(clipedHeader.getX(), clipedHeader.getY(), clipedHeader.getRight(), clipedHeader.getY(), 1.0f);           // Top
            g.drawLine(clipedHeader.getX(), clipedHeader.getY(), clipedHeader.getX(), clipedHeader.getBottom(), 1.0f);          // Left
            g.drawLine(clipedHeader.getX(), clipedHeader.getBottom(), clipedHeader.getRight(), clipedHeader.getBottom(), 1.0f); // Bottom

            // Main clip borders
            g.drawLine(clipedClip.getX(), clipedClip.getY(), clipedClip.getRight(), clipedClip.getY(), 1.0f);           // Top
            g.drawLine(clipedClip.getX(), clipedClip.getY(), clipedClip.getX(), clipedClip.getBottom(), 1.0f);          // Left
            g.drawLine(clipedClip.getX(), clipedClip.getBottom(), clipedClip.getRight(), clipedClip.getBottom(), 1.0f); // Bottom
            g.setColour(isSelected ? clipColor.interpolatedWith(juce::Colours::blanchedalmond, 0.5f) : clipColor);
            auto chi = clipedHeader.reduced(0.0f, 2.0f);
            chi.removeFromLeft(juce::jmin(clipExtendsRight, 2.0f));
            g.fillRect(chi);
        }
        else
        {
            // Full rectangles if clip is completely visible
            g.drawLine(clipedHeader.getX(), clipedHeader.getY(), clipedHeader.getRight(), clipedHeader.getY(), 1.0f);           // Top
            g.drawLine(clipedHeader.getRight(), clipedHeader.getY(), clipedHeader.getRight(), clipedHeader.getBottom(), 1.0f);  // Right
            g.drawLine(clipedHeader.getX(), clipedHeader.getBottom(), clipedHeader.getRight(), clipedHeader.getBottom(), 1.0f); // Bottom
            g.drawLine(clipedHeader.getX(), clipedHeader.getY(), clipedHeader.getX(), clipedHeader.getBottom(), 1.0f);          // Left

            // Main clip borders
            g.drawLine(clipedClip.getX(), clipedClip.getY(), clipedClip.getRight(), clipedClip.getY(), 1.0f);           // Top
            g.drawLine(clipedClip.getX(), clipedClip.getY(), clipedClip.getX(), clipedClip.getBottom(), 1.0f);          // Left
            g.drawLine(clipedClip.getRight(), clipedClip.getY(), clipedClip.getRight(), clipedClip.getBottom(), 1.0f);  // Right
            g.drawLine(clipedClip.getX(), clipedClip.getBottom(), clipedClip.getRight(), clipedClip.getBottom(), 1.0f); // Bottom
            g.setColour(isSelected ? clipColor.interpolatedWith(juce::Colours::blanchedalmond, 0.5f) : clipColor);
            g.fillRect(clipedHeader.reduced(2.0f, 2.0f));
        }
    }
    g.restoreState();

    g.saveState();
    {
        if (clipedHeader.getWidth() <= 0.0f || clipedHeader.getWidth() > 10000.0f)
        {
            g.restoreState();
            return;
        }

        auto textArea = header.reduced(4.0f, 0.0f);
        textArea.removeFromLeft(4.0f);

        auto safeTextArea = textArea;
        safeTextArea.setWidth(juce::jmin(safeTextArea.getWidth(), 10000.0f));

        g.reduceClipRegion(clipedHeader.toNearestInt());

        if (isSelected)
            labelTextColor = juce::Colours::black;

        g.setColour(labelTextColor);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));

        g.drawText(name, safeTextArea, juce::Justification::centredLeft, false);
    }
    g.restoreState();
}

void GUIHelpers::drawMidiClip(juce::Graphics &g, EditViewState &evs, te::MidiClip::Ptr clip, juce::Rectangle<float> clipRect, juce::Rectangle<float> displayedRect, juce::Colour color, double x1Beat, double x2beat)
{
    auto area = clipRect;

    if (clipRect.getX() < displayedRect.getX())
        area = area.withLeft(displayedRect.getX());

    if (clipRect.getRight() > displayedRect.getRight())
        area = area.withRight(displayedRect.getRight());

    auto &seq = clip->getSequence();

    auto range = seq.getNoteNumberRange();
    auto lines = range.getLength();
    auto noteHeight = juce::jmax(1.0f, ((clipRect.getHeight()) / 20.0f));
    auto noteColor = color.withLightness(0.6f);

    for (auto n : seq.getNotes())
    {
        double sBeat = n->getStartBeat().inBeats() - clip->getOffsetInBeats().inBeats();
        double eBeat = n->getEndBeat().inBeats() - clip->getOffsetInBeats().inBeats();
        float y = clipRect.getCentreY();

        if (!range.isEmpty())
            y = juce::jmap((float)n->getNoteNumber(), (float)(range.getStart() + lines), (float)range.getStart(), clipRect.getY() + (noteHeight / 2.0f), clipRect.getY() + clipRect.getHeight() - noteHeight - (noteHeight / 2.0f));

        float startX = evs.beatsToX(sBeat, displayedRect.getWidth(), x1Beat, x2beat);
        float endX = evs.beatsToX(eBeat, displayedRect.getWidth(), x1Beat, x2beat);
        float scrollOffset = evs.beatsToX(0.0, displayedRect.getWidth(), x1Beat, x2beat) * -1.0f;

        float x1 = clipRect.getX() + startX + scrollOffset;
        float x2 = clipRect.getX() + endX + scrollOffset;

        float gap = 2.0f;

        x1 = juce::jmin(juce::jmax(gap, x1), clipRect.getRight() - gap);
        x2 = juce::jmin(juce::jmax(gap, x2), clipRect.getRight() - gap);

        x1 = juce::jmax(area.getX(), x1);
        x2 = juce::jmin(area.getRight(), x2);

        float w = juce::jmax(0.0f, x2 - x1);

        g.setColour(noteColor);
        g.fillRect(x1, y, w, noteHeight);
    }
}

void GUIHelpers::strokeRoundedRectWithSide(juce::Graphics &g, juce::Rectangle<float> area, float cornerSize, bool topLeft, bool topRight, bool bottomLeft, bool bottomRight)
{
    juce::Path p;
    auto x = area.getX();
    auto y = area.getY();
    auto w = area.getWidth();
    auto h = area.getHeight();
    p.addRoundedRectangle(x, y, w, h, cornerSize, cornerSize, topLeft, topRight, bottomLeft, bottomRight);
    auto st = juce::PathStrokeType(1);
    g.strokePath(p, st);
}
void GUIHelpers::drawRoundedRectWithSide(juce::Graphics &g, juce::Rectangle<float> area, float cornerSize, bool topLeft, bool topRight, bool bottomLeft, bool bottomRight)
{
    juce::Path p;
    auto x = area.getX();
    auto y = area.getY();
    auto w = area.getWidth();
    auto h = area.getHeight();
    p.addRoundedRectangle(x, y, w, h, cornerSize, cornerSize, topLeft, topRight, bottomLeft, bottomRight);
    g.fillPath(p);
}

void GUIHelpers::drawFromSvg(juce::Graphics &g, const char *svgbinary, juce::Colour newColour, juce::Rectangle<float> drawRect)
{
    if (auto svg = juce::XmlDocument::parse(svgbinary))
    {
        std::unique_ptr<juce::Drawable> drawable;
        {
            const juce::MessageManagerLock mmLock;
            drawable = juce::Drawable::createFromSVG(*svg);
        }
        if (drawable != nullptr)
        {
            drawable->setTransformToFit(drawRect, juce::RectanglePlacement::centred);
            drawable->replaceColour(juce::Colour(0xff626262), newColour);
            drawable->draw(g, 2.f);
        }
    }
}

void GUIHelpers::setDrawableOnButton(juce::DrawableButton &button, const char *svgbinary, juce::Colour colour)
{
    if (auto drawable = getDrawableFromSvg(svgbinary, colour))
        button.setImages(drawable.get());
}

std::unique_ptr<juce::Drawable> GUIHelpers::getDrawableFromSvg(const char *svgbinary, juce::Colour colour)
{
    if (auto svg = juce::XmlDocument::parse(svgbinary))
    {
        std::unique_ptr<juce::Drawable> drawable;
        {
            const juce::MessageManagerLock mmLock;
            drawable = juce::Drawable::createFromSVG(*svg);
        }

        if (drawable != nullptr)
        {
            drawable->replaceColour(juce::Colour(0xff626262), colour);
        }

        return drawable;
    }

    return nullptr;
}

float GUIHelpers::getScale(const juce::Component &c)
{
#if JUCE_LINUX
    auto rc = c.localAreaToGlobal(c.getLocalBounds());
    auto scale = static_cast<int>(juce::Desktop::getInstance().getDisplays().getDisplayForRect(rc)->scale);
    return static_cast<float>(scale);
#else
    return 1.0f;
#endif
    return 1.0f;
}

juce::MouseCursor GUIHelpers::createCustomMouseCursor(CustomMouseCursor cursorType, float scale)
{

    switch (cursorType)
    {
    case CustomMouseCursor::ShiftLeft:
        return getMouseCursorFromSvg(BinaryData::shiftCursorLeftEdge_svg, {12, 12}, scale);
    case CustomMouseCursor::ShiftRight:
        return getMouseCursorFromSvg(BinaryData::shiftCursorRightEdge_svg, {12, 12}, scale);
    case CustomMouseCursor::TimeShiftRight:
        return getMouseCursorFromSvg(BinaryData::timeShiftCursorRightEdge_svg, {12, 12}, scale);
    case CustomMouseCursor::CurveSteepnes:
        return getMouseCursorFromSvg(BinaryData::curveSteepnessCursor_svg, {12, 12}, scale);
    case CustomMouseCursor::ShiftHand:
        return getMouseCursorFromSvg(BinaryData::shiftHandCursor_svg, {12, 12}, scale);
    case CustomMouseCursor::Draw:
        return getMouseCursorFromSvg(BinaryData::pencil_svg, {1, 24}, scale);
    case CustomMouseCursor::Lasso:
        return juce::MouseCursor::CrosshairCursor;
    case CustomMouseCursor::Range:
        return juce::MouseCursor::IBeamCursor;
    case CustomMouseCursor::Split:
        return getMouseCursorFromSvg(BinaryData::split_svg, {12, 24}, scale);
    case CustomMouseCursor::Erasor:
        return getMouseCursorFromSvg(BinaryData::rubber_svg, {1, 24}, scale);

    default:
        break;
    }

    return juce::MouseCursor();
}

juce::MouseCursor GUIHelpers::getMouseCursorFromPng(const char *png, const int size, juce::Point<int> hotSpot)
{
    juce::MemoryInputStream pngInputStream(png, static_cast<size_t>(size), false);
    juce::Image image = juce::ImageFileFormat::loadFrom(pngInputStream);
    juce::ScaledImage si(image, 2);

    return juce::MouseCursor(si, hotSpot);
}

juce::MouseCursor GUIHelpers::getMouseCursorFromSvg(const char *svgbinary, juce::Point<int> hotSpot, float scale)
{
    auto imageType = juce::Image::ARGB;
    juce::Image image(imageType, 24 * scale, 24 * scale, true);
    juce::Graphics g(image);

    GUIHelpers::drawFromSvg(g, svgbinary, juce::Colours::white, {24 * scale, 24 * scale});

    auto hotX = juce::jlimit(0, static_cast<int>(24 * scale) - 1, static_cast<int>(scale * hotSpot.getX()));
    auto hotY = juce::jlimit(0, static_cast<int>(24 * scale) - 1, static_cast<int>(scale * hotSpot.getY()));

    return juce::MouseCursor(image, hotX, hotY);
}

juce::Image GUIHelpers::drawableToImage(const juce::Drawable &drawable, float targetWidth, float targetHeight)
{
    auto imageType = juce::Image::ARGB;
    juce::Image image(imageType, targetWidth, targetHeight, true);
    juce::Graphics g(image);
    drawable.draw(g, 1.f);

    return image;
}

//--------------------------------------

void GUIHelpers::saveEdit(EditViewState &evs, const juce::File &workDir)
{

    juce::WildcardFileFilter wildcardFilter("*.tracktionedit", juce::String(), "Next Studio Project File");

    juce::FileBrowserComponent browser(juce::FileBrowserComponent::saveMode + juce::FileBrowserComponent::canSelectFiles, workDir, &wildcardFilter, nullptr);

    juce::FileChooserDialogBox dialogBox("Save the project", "Please choose some kind of file that you want to save...", browser, true, juce::Colours::black);

    if (dialogBox.show())
    {
        juce::File selectedFile = browser.getSelectedFile(0).withFileExtension(".tracktionedit");
        evs.m_editName = selectedFile.getFileNameWithoutExtension();

        auto cf = evs.m_edit.editFileRetriever();
        EngineHelpers::refreshRelativePathsToNewEditFile(evs, selectedFile);
        te::EditFileOperations(evs.m_edit).writeToFile(selectedFile, false);
        EngineHelpers::refreshRelativePathsToNewEditFile(evs, cf);
        evs.m_edit.sendSourceFileUpdate();
    }
}

double GUIHelpers::getIntervalBeatsOfSnap(int snapLevel, int numBeatsPerBar)
{
    switch (snapLevel)
    {
    case 0:
        return 1.0 / 960.0;
    case 1:
        return 2.0 / 960.0;
    case 2:
        return 5.0 / 960.0;
    case 3:
        return 1.0 / 64.0;
    case 4:
        return 1.0 / 32.0;
    case 5:
        return 1.0 / 16.0;
    case 6:
        return 1.0 / 8.0;
    case 7:
        return 1.0 / 4.0;
    case 8:
        return 1.0 / 2.0;
    case 9:
        return 1.0;
    case 10:
        return numBeatsPerBar * 1.0;
    case 11:
        return numBeatsPerBar * 2.0;
    case 12:
        return numBeatsPerBar * 4.0;
    case 13:
        return numBeatsPerBar * 8.0;
    case 14:
        return numBeatsPerBar * 16.0;
    case 15:
        return numBeatsPerBar * 64.0;
    case 16:
        return numBeatsPerBar * 128.0;
    case 17:
        return numBeatsPerBar * 256.0;
    case 18:
        return numBeatsPerBar * 1024.0;
    default:
        return 1.0;
    }
}

void GUIHelpers::drawBarsAndBeatLines(juce::Graphics &g, EditViewState &evs, double x1beats, double x2beats, juce::Rectangle<float> boundingRect, bool printDescription)
{
    auto &avs = evs.m_applicationState;
    const auto barColour = avs.getTimeLineStrokeColour().withAlpha(0.4f);
    const auto beatColour = avs.getTimeLineStrokeColour().withAlpha(0.25f);
    const auto fracColour = avs.getTimeLineStrokeColour().withAlpha(0.1f);
    const auto snapLineColour = avs.getTimeLineStrokeColour().withAlpha(0.05f);
    const auto shadowShade = avs.getTimeLineShadowShade();
    const auto textColour = avs.getTimeLineTextColour();
    const auto numBeatsPerBar = static_cast<int>(evs.m_edit.tempoSequence.getTimeSigAt(tracktion::TimePosition::fromSeconds(0)).numerator);

    if (!printDescription)
        drawBarBeatsShadow(g, evs, x1beats, x2beats, boundingRect, shadowShade);

    int snapLevel = juce::jmax(3, evs.getBestSnapType(x1beats, x2beats, boundingRect.getWidth()).getLevel());
    // At 1/16 and finer grids, emphasize quarter-beat landmarks for orientation.
    const bool emphasizeQuarterBeatLines = snapLevel <= 5;
    const auto quarterBeatColour = emphasizeQuarterBeatLines ? avs.getTimeLineStrokeColour().withAlpha(0.2f) : fracColour;
    const auto subDivisionColour = emphasizeQuarterBeatLines ? avs.getTimeLineStrokeColour().withAlpha(0.08f) : fracColour.withAlpha(0.2f);

    double intervalBeats = getIntervalBeatsOfSnap(snapLevel, numBeatsPerBar);

    double startBeat = std::floor(x1beats / intervalBeats) * intervalBeats;
    double endBeat = std::ceil(x2beats / intervalBeats) * intervalBeats;

    double epsilon = 1e-3;

    int labelDetailLevel = 0;
    if (snapLevel >= 7)
        labelDetailLevel = 0;
    else if (snapLevel >= 5)
        labelDetailLevel = 1;
    else
        labelDetailLevel = 2;

    for (double beat = startBeat; beat <= endBeat; beat += intervalBeats)
    {
        float x = boundingRect.getX() + evs.beatsToX(beat, boundingRect.getWidth(), x1beats, x2beats);

        if (x < boundingRect.getX() || x > boundingRect.getRight())
            continue;

        juce::Colour lineColour;
        bool isBarLine = false;
        bool isBeatLine = false;
        bool isQuarterBeatLine = false;

        if (std::abs(std::fmod(beat, numBeatsPerBar)) < epsilon)
        {
            lineColour = barColour;
            isBarLine = true;
        }
        else if (std::abs(std::fmod(beat, 1.0)) < epsilon)
        {
            lineColour = beatColour;
            isBeatLine = true;
        }
        else if (std::abs(std::fmod(beat * 4.0, 1.0)) < epsilon)
        {
            lineColour = quarterBeatColour;
            isQuarterBeatLine = true;
        }
        else
        {
            lineColour = subDivisionColour;
        }

        g.setColour(lineColour);
        g.drawLine(x, boundingRect.getY(), x, boundingRect.getBottom(), 1.0f);

        if (printDescription)
        {
            bool shouldDrawLabel = false;
            juce::String label;

            int barNumber = static_cast<int>(beat / numBeatsPerBar) + 1;
            double beatWithinBar = std::fmod(beat, numBeatsPerBar);

            if (labelDetailLevel == 0 && isBarLine)
            {
                label = juce::String(barNumber);
                shouldDrawLabel = true;
            }
            else if (labelDetailLevel == 1 && (isBarLine || isBeatLine))
            {
                int beatNumber = static_cast<int>(beatWithinBar) + 1;
                label = juce::String(barNumber) + "." + juce::String(beatNumber);
                shouldDrawLabel = true;
            }
            else if (labelDetailLevel == 2)
            {
                int beatNumber = static_cast<int>(beatWithinBar) + 1;
                double fraction = beatWithinBar - static_cast<int>(beatWithinBar);
                int quarterNumber = static_cast<int>(fraction * 4) + 1;
                label = juce::String(barNumber) + "." + juce::String(beatNumber) + "." + juce::String(quarterNumber);
                shouldDrawLabel = isQuarterBeatLine || isBeatLine || isBarLine;
            }

            if (shouldDrawLabel)
            {
                g.setColour(textColour);
                auto textRect = juce::Rectangle<float>(x + 2.0f, boundingRect.getY(), 50.0f, 12.0f);
                g.drawText(label, textRect, juce::Justification::left);
            }
        }
    }
}
void GUIHelpers::drawFakeRoundCorners(juce::Graphics &g, juce::Rectangle<float> bounds, juce::Colour colour, juce::Colour outline, int stroke)
{
    g.setColour(colour);
    const float cornerSize = 10.f;
    auto area = bounds.expanded(1.f);
    auto size = cornerSize;
    juce::Path fakeRoundedCorners;
    fakeRoundedCorners.addRectangle(area);
    fakeRoundedCorners.setUsingNonZeroWinding(false);
    fakeRoundedCorners.addRoundedRectangle(bounds, cornerSize);

    g.fillPath(fakeRoundedCorners);

    g.setColour(outline);
    g.drawRoundedRectangle(bounds, 10 + 1, stroke);
}
void GUIHelpers::printTextAt(juce::Graphics &graphic, juce::Rectangle<float> textRect, const juce::String &text, const juce::Colour &textColour)
{
    graphic.setColour(textColour);
    graphic.drawText("  " + text, textRect, juce::Justification::centredLeft, false);
}

void GUIHelpers::drawRectWithShadow(juce::Graphics &g, juce::Rectangle<float> area, float cornerSize, const juce::Colour &colour, const juce::Colour &shade)
{
    g.setColour(shade);
    g.fillRoundedRectangle(area.translated(2, 2), cornerSize);
    g.setColour(colour);
    g.fillRoundedRectangle(area, cornerSize);
}

void GUIHelpers::drawCircleWithShadow(juce::Graphics &g, juce::Rectangle<float> area, const juce::Colour &colour, const juce::Colour &shade)
{
    g.setColour(shade);
    g.fillEllipse(area.translated(2, 2));
    g.setColour(colour);
    g.fillEllipse(area);
}
void GUIHelpers::drawSnapLines(juce::Graphics &g, const EditViewState &evs, double x1beats, double x2beats, const juce::Rectangle<int> &boundingRect, const juce::Colour &colour)
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

        auto &tempo = evs.m_edit.tempoSequence.getTempoAt(tracktion::BeatPosition::fromBeats(it));
        auto delta = evs.timeToBeat(snapType.getApproxIntervalTime(tempo).inSeconds());
        it = it + delta;
    }
}
void GUIHelpers::drawBarBeatsShadow(juce::Graphics &g, const EditViewState &evs, double x1beats, double x2beats, const juce::Rectangle<float> &boundingRect, const juce::Colour &shade)
{
    const te::TimecodeSnapType &snapType = evs.getBestSnapType(x1beats, x2beats, boundingRect.getWidth());
    int num = evs.m_edit.tempoSequence.getTimeSigAt(tracktion::TimePosition::fromSeconds(0)).numerator;
    int shadowBeatDelta = num * 4;

    if (snapType.getLevel() <= 9)
        shadowBeatDelta = num;
    if (snapType.getLevel() <= 6)
        shadowBeatDelta = 1;

    auto beatIter = static_cast<int>(x1beats);
    while (beatIter % shadowBeatDelta != 0)
        beatIter--;

    while (beatIter <= x2beats)
    {
        if ((beatIter / shadowBeatDelta) % 2 == 0)
        {
            float x = evs.beatsToX(beatIter, boundingRect.getWidth(), x1beats, x2beats);
            float w = evs.beatsToX(beatIter + shadowBeatDelta, boundingRect.getWidth(), x1beats, x2beats) - x;
            juce::Rectangle<float> shadowRect(x + boundingRect.getX(), boundingRect.getY(), w, boundingRect.getHeight());

            if (shadowRect.getX() < boundingRect.getX())
                shadowRect = shadowRect.withLeft(boundingRect.getX());

            if (shadowRect.getRight() > boundingRect.getRight())
                shadowRect = shadowRect.withRight(boundingRect.getRight());

            g.setColour(shade);
            g.fillRect(shadowRect);
        }

        beatIter += shadowBeatDelta;
    }
}

void GUIHelpers::drawLogo(juce::Graphics &g, juce::Colour colour, float scale)
{
    juce::Path logoPath;
    juce::AffineTransform transform;

    // square
    logoPath.startNewSubPath(12, 2);
    logoPath.lineTo(3, 2);
    logoPath.quadraticTo(2, 2, 2, 3);

    logoPath.lineTo(2, 13);
    logoPath.quadraticTo(2, 14, 3, 14);

    logoPath.lineTo(12, 14);
    logoPath.quadraticTo(13, 14, 13, 13);
    logoPath.quadraticTo(9, 8, 13, 3);
    logoPath.quadraticTo(13, 2, 12, 2);
    logoPath.closeSubPath();

    //
    // // Second path
    logoPath.addRoundedRectangle(juce::Rectangle<float>(15, 2, 4, 12), 1.f);
    logoPath.addRoundedRectangle(juce::Rectangle<float>(20, 2, 4, 12), 1.f);
    // //
    // // Triangle
    logoPath.startNewSubPath(26, 3);
    logoPath.quadraticTo(26, 2, 27, 2);
    logoPath.lineTo(37, 7);
    logoPath.quadraticTo(38, 8, 37, 9);
    logoPath.lineTo(27, 14);
    logoPath.quadraticTo(26, 14, 26, 13);
    logoPath.quadraticTo(29, 8, 26, 3);

    logoPath.closeSubPath();

    // Apply transformation and scale
    transform = juce::AffineTransform::scale(scale, scale);
    logoPath.applyTransform(transform);

    // Set the fill color
    g.setColour(colour);

    // Draw the logo path
    g.fillPath(logoPath);
}

juce::String GUIHelpers::translate(juce::String stringToTranslate, ApplicationViewState &avs)
{
    juce::LocalisedStrings translations(avs.getFileToTranslation(), false);
    auto translatedString = translations.translate(stringToTranslate);

    return translatedString;
}

juce::String PlayHeadHelpers::timeToTimecodeString(double seconds)
{
    auto millisecs = juce::roundToInt(seconds * 1000.0);
    auto absMillisecs = std::abs(millisecs);

    return juce::String::formatted("%02d:%02d.%03d",

                                   (absMillisecs / 60000) % 60, (absMillisecs / 1000) % 60, absMillisecs % 1000);
}

juce::StringArray PlayHeadHelpers::getTimeCodeParts(tracktion_engine::Edit &edit, double time)
{
    auto st = tracktion::TimePosition::fromSeconds(time);
    auto timeCode = edit.getTimecodeFormat().getString(edit.tempoSequence, st, false);

    juce::StringArray parts;
    parts.addTokens(timeCode, "|", "");

    return parts;
}
juce::String PlayHeadHelpers::barsBeatsString(tracktion_engine::Edit &edit, double time) { return getTimeCodeParts(edit, time).joinIntoString("."); }

te::AudioTrack::Ptr EngineHelpers::getAudioTrack(te::Track::Ptr track, EditViewState &evs)
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

bool EngineHelpers::renderToNewTrack(EditViewState &evs, juce::Array<tracktion_engine::Track *> tracksToRender, tracktion::TimeRange range)
{
    auto sampleDir = juce::File(evs.m_applicationState.m_samplesDir);
    auto renderFile = sampleDir.getNonexistentChildFile("render", ".wav");

    juce::BigInteger tracksToDo{0};

    auto allTracks = te::getAllTracks(evs.m_edit);

    for (auto *trackToRender : tracksToRender)
    {
        int index = allTracks.indexOf(trackToRender);
        if (index != -1)
            tracksToDo.setBit(index);
    }

    te::Renderer::renderToFile("Render", renderFile, evs.m_edit, range, tracksToDo);

    EngineHelpers::loadAudioFileOnNewTrack(evs, renderFile, juce::Colours::plum, range.getStart().inSeconds());

    return true;
}

bool EngineHelpers::renderCliptoNewTrack(EditViewState &evs, te::Clip::Ptr clip)
{
    auto range = clip->getEditTimeRange();
    auto index = te::getAllTracks(evs.m_edit).indexOf(clip->getTrack());
    auto trackToRender = juce::BigInteger{index};

    auto sampleDir = juce::File(evs.m_applicationState.m_samplesDir);
    auto renderFile = sampleDir.getNonexistentChildFile("render", ".wav");

    te::Renderer::renderToFile("Render", renderFile, evs.m_edit, range, trackToRender);

    EngineHelpers::loadAudioFileOnNewTrack(evs, renderFile, juce::Colours::plum, range.getStart().inSeconds());

    return true;
}

void EngineHelpers::renderEditToFile(EditViewState &evs, juce::File renderFile, tracktion::TimeRange range)
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

    juce::BigInteger tracksToDo{0};

    for (auto i = 0; i < te::getAllTracks(evs.m_edit).size(); ++i)
        tracksToDo.setBit(i);

    te::Renderer::renderToFile("Render", renderFile, evs.m_edit, range, tracksToDo);
}
void EngineHelpers::setMidiInputFocusToSelection(EditViewState &evs)
{
    auto &dm = evs.m_edit.engine.getDeviceManager();
    auto defaultMidi = dm.getDefaultMidiInDevice();
    auto virtualMidi = getVirtualMidiInputDevice(evs.m_edit);

    if (!defaultMidi && !virtualMidi)
        return;

    juce::Array<te::InputDeviceInstance *> midiInputsToModify;
    for (auto instance : evs.m_edit.getAllInputDevices())
    {
        if ((defaultMidi && &instance->getInputDevice() == defaultMidi) || (virtualMidi && &instance->getInputDevice() == virtualMidi))
        {
            midiInputsToModify.add(instance);
        }
    }

    // Identify target tracks from selection
    juce::Array<te::Track *> targetMidiTracks;
    for (auto *track : evs.m_selectionManager.getItemsOfType<te::Track>())
    {
        if (track->isAudioTrack() && track->state.getProperty(IDs::isMidiTrack))
            targetMidiTracks.add(track);
    }

    // If no tracks selected, check for clips
    if (targetMidiTracks.isEmpty())
    {
        for (auto *clip : evs.m_selectionManager.getItemsOfType<te::Clip>())
        {
            if (auto *track = clip->getTrack())
                if (track->isAudioTrack() && track->state.getProperty(IDs::isMidiTrack))
                    targetMidiTracks.addIfNotAlreadyThere(track);
        }
    }

    // CRITICAL: If no new targets identified, we keep the OLD ones to avoid dropouts
    if (targetMidiTracks.isEmpty())
        return;

    // Early exit check: are all midiInputsToModify already pointing exactly to targetMidiTracks?
    bool changeNeeded = false;
    for (auto *instance : midiInputsToModify)
    {
        auto currentTargets = instance->getTargets();

        if (currentTargets.size() != targetMidiTracks.size())
        {
            changeNeeded = true;
            break;
        }

        for (auto *track : targetMidiTracks)
        {
            if (!currentTargets.contains(track->itemID))
            {
                changeNeeded = true;
                break;
            }
        }

        if (changeNeeded)
            break;
    }

    if (!changeNeeded)
        return;

    bool contextReallocNeeded = false;

    // Apply changes robustly: Clear all, then add needed
    for (auto *instance : midiInputsToModify)
    {
        // Ensure monitoring is ON for MIDI devices
        instance->getInputDevice().setMonitorMode(te::InputDevice::MonitorMode::on);

        // 1. Remove ALL current targets
        auto currentTargets = instance->getTargets();
        for (auto targetID : currentTargets)
        {
            [[maybe_unused]] auto result = instance->removeTarget(targetID, &evs.m_edit.getUndoManager());
            contextReallocNeeded = true;
        }

        // 2. Add ALL desired targets
        for (auto *track : targetMidiTracks)
        {
            // Logic to prevent double-triggering:
            // If we are dealing with the Default MIDI Device (e.g. All MIDI Ins),
            // check if the track already has ANOTHER physical input assigned.
            if (defaultMidi && &instance->getInputDevice() == defaultMidi)
            {
                bool hasSpecificInput = false;
                for (auto otherInst : evs.m_edit.getAllInputDevices())
                {
                    if (otherInst == instance)
                        continue;
                    if (virtualMidi && &otherInst->getInputDevice() == virtualMidi)
                        continue;

                    // If another device targets this track, we consider it a specific assignment
                    if (otherInst->getTargets().contains(track->itemID))
                    {
                        hasSpecificInput = true;
                        break;
                    }
                }

                if (hasSpecificInput)
                    continue; // Skip adding default device
            }

            [[maybe_unused]] auto result = instance->setTarget(track->itemID, false, &evs.m_edit.getUndoManager(), 0);
            contextReallocNeeded = true;
        }
    }

    if (contextReallocNeeded)
    {
        GUIHelpers::log("Utilities: setMidiInputFocusToSelection - Targets updated, restarting playback.");
        evs.m_edit.restartPlayback();
    }
}
te::MidiInputDevice *EngineHelpers::getVirtualMidiInputDevice(te::Edit &edit)
{
    auto &dm = edit.engine.getDeviceManager();
    auto name = "virtualMidiIn";

    dm.createVirtualMidiDevice(name);

    for (const auto instance : edit.getAllInputDevices())
    {
        DBG(instance->getInputDevice().getName());

        if (instance->getInputDevice().getDeviceType() == te::InputDevice::virtualMidiDevice && instance->getInputDevice().getName() == name)
            return dynamic_cast<te::MidiInputDevice *>(&instance->getInputDevice());
    }

    return nullptr;
}
tracktion::core::TimePosition EngineHelpers::getTimePos(double t) { return tracktion::core::TimePosition::fromSeconds(t); }
void EngineHelpers::deleteSelectedClips(EditViewState &evs)
{
    for (auto selectedClip : evs.m_selectionManager.getSelectedObjects().getItemsOfType<te::Clip>())
    {
        if (selectedClip->getTrack() != nullptr)
        {
            for (auto ap : selectedClip->getTrack()->getAllAutomatableParams())
            {
                ap->getCurve().removePointsInRegion(selectedClip->getEditTimeRange());
            }

            selectedClip->removeFromParent();
        }
    }
}

bool EngineHelpers::trackWantsClip(const juce::ValueTree state, const te::Track *track)
{
    bool isMidi = state.hasType(te::IDs::MIDICLIP);
    bool isAudio = state.hasType(te::IDs::AUDIOCLIP);

    if (!isMidi && !isAudio)
        return false;

    if (track == nullptr)
        return false;

    if (track->state.getProperty(IDs::isMidiTrack) && isMidi)
        return true;

    if (!track->state.getProperty(IDs::isMidiTrack) && isAudio)
        return true;

    return false;
}

bool EngineHelpers::trackWantsClip(const te::Clip *clip, const te::Track *track)
{
    if (track == nullptr)
        return false;
    if (track->isFolderTrack())
        return false;

    return clip->isMidi() == static_cast<bool>(track->state.getProperty(IDs::isMidiTrack));
}

te::Track *EngineHelpers::getTargetTrack(te::Track *sourceTrack, int verticalOffset)
{
    if (sourceTrack == nullptr)
        return nullptr;

    auto &edit = sourceTrack->edit;
    auto tracks = getSortedTrackList(edit);
    auto targetIdx = tracks.indexOf(sourceTrack) + verticalOffset;
    auto targetTrack = tracks[targetIdx];

    return targetTrack;
}

juce::Array<te::Track *> EngineHelpers::getSortedTrackList(te::Edit &edit)
{
    juce::Array<te::Track *> tracks;

    edit.visitAllTracks(
        [&](te::Track &t)
        {
            if (t.isAutomationTrack() || t.isArrangerTrack() || t.isChordTrack() || t.isMarkerTrack() || t.isTempoTrack() || t.isMasterTrack())
                return true;
            tracks.add(&t);
            return true;
        },
        true);

    return tracks;
}

juce::Array<te::MidiClip *> EngineHelpers::getMidiClipsOfTrack(te::Track &track)
{
    juce::Array<te::MidiClip *> midiClips;

    if (auto at = dynamic_cast<te::AudioTrack *>(&track))
        for (auto c : at->getClips())
            if (auto mc = dynamic_cast<te::MidiClip *>(c))
                midiClips.add(mc);

    return midiClips;
}

double EngineHelpers::getNoteStartBeat(const te::MidiClip *midiClip, const te::MidiNote *n)
{
    auto sBeat = n->getStartBeat() - midiClip->getOffsetInBeats();
    return sBeat.inBeats();
}

double EngineHelpers::getNoteEndBeat(const te::MidiClip *midiClip, const te::MidiNote *n)
{
    auto eBeat = n->getEndBeat() - midiClip->getOffsetInBeats();
    return eBeat.inBeats();
}

bool EngineHelpers::isTrackItemInRange(te::TrackItem *ti, const tracktion::TimeRange &tr) { return ti->getEditTimeRange().intersects(tr); }

juce::ValueTree exportPluginStates(const tracktion::engine::PluginList &pluginList)
{
    juce::ValueTree pluginsTree("Plugins");

    for (auto *plugin : pluginList.getPlugins())
    {
        if (plugin != nullptr)
        {
            // Create a copy of the plugin's state and add it to the tree
            pluginsTree.appendChild(plugin->state.createCopy(), nullptr);
        }
    }

    return pluginsTree;
}

void storePluginStatesAndClear(juce::Array<te::Track *> &involvedTracks, juce::Array<juce::ValueTree> &states, const juce::Array<te::Clip *> &selectedClips, int verticalOffset)
{
    for (const auto &selectedClip : selectedClips)
    {
        if (auto *sourceTrack = selectedClip->getTrack())
        {
            if (auto *targetTrack = EngineHelpers::getTargetTrack(sourceTrack, verticalOffset))
            {
                for (auto *track : {sourceTrack, targetTrack})
                {
                    if (involvedTracks.addIfNotAlreadyThere(track))
                    {
                        juce::ValueTree exportedState = exportPluginStates(track->pluginList);
                        if (!exportedState.isValid())
                        {
                            GUIHelpers::log("Warning: Plugin state export failed for track " + track->getName());
                        }
                        states.add(exportedState);
                        track->pluginList.clear();
                    }
                }
            }
            else
            {
                GUIHelpers::log("Warning: Target track not found for source track " + sourceTrack->getName());
            }
        }
        else
        {
            GUIHelpers::log("Warning: Selected clip has no valid source track!");
        }
    }

    jassert(involvedTracks.size() == states.size());
    GUIHelpers::log("Stored states: " + juce::String(states.size()));
    GUIHelpers::log("Stored tracks: " + juce::String(involvedTracks.size()));
}

void restorePluginStates(const juce::Array<te::Track *> &involvedTracks, const juce::Array<juce::ValueTree> &states)
{
    for (auto i = 0; i < involvedTracks.size(); i++)
    {
        involvedTracks[i]->pluginList.addPluginsFrom(states[i], true, false);
    }

    jassert(involvedTracks.size() == states.size());
    GUIHelpers::log("Restored plugins. Tracks involved: " + juce::String(involvedTracks.size()));
}

void EngineHelpers::moveSelectedClips(bool copy, double timeDelta, int verticalOffset, EditViewState &evs)
{
    // If not copying and no movement occurred, return early to avoid unnecessary processing
    if (!copy && std::abs(timeDelta) < 1.0e-9 && verticalOffset == 0)
        return;

    //---------------------------------------------------
    // when we insert a clip on a track with a plugin with a lot of parameters
    // the needed time is much more higher than a track without a plugin.
    // If you have to duplicate a lot of clips, this could take a lot of time.
    // So this approach removes all plugins and save them in a value tree. After
    // moving or coping the clips, we reinsert the plugins from the state
    // The time is reduced a lot, but I don't know, if this is the best approach.
    //---------------------------------------------------
    bool testOptimisation = false;
    testOptimisation = true;
    //---------------------------------------------------

    if (verticalOffset == 0)
        copyAutomationForSelectedClips(timeDelta, evs.m_selectionManager, copy);

    auto selectedClips = evs.m_selectionManager.getItemsOfType<te::Clip>();
    auto tempPosition = evs.m_edit.getLength().inSeconds() + timeDelta;

    auto involvedTracks = juce::Array<te::Track *>();
    auto states = juce::Array<juce::ValueTree>();
    if (testOptimisation == true)
        storePluginStatesAndClear(involvedTracks, states, selectedClips, verticalOffset);

    juce::Array<te::Clip *> newClips;

    for (auto selectedClip : selectedClips)
    {
        auto targetTrack = getTargetTrack(selectedClip->getTrack(), verticalOffset);

        if (trackWantsClip(selectedClip, targetTrack))
        {
            auto newClip = te::duplicateClip(*selectedClip);
            newClips.add(newClip);
            newClip->setStart(newClip->getPosition().getStart() + tracktion::TimeDuration::fromSeconds(tempPosition), false, true);

            if (!copy)
                selectedClip->removeFromParent();
            else
                evs.m_selectionManager.deselect(selectedClip);
        }
    }

    for (auto newClip : newClips)
    {
        auto pasteTime = newClip->getPosition().getStart().inSeconds() + timeDelta - tempPosition;
        auto targetTrack = EngineHelpers::getTargetTrack(newClip->getTrack(), verticalOffset);

        if (EngineHelpers::trackWantsClip(newClip, targetTrack))
        {
            if (auto tct = dynamic_cast<te::ClipTrack *>(targetTrack))
            {
                tct->deleteRegion({tracktion::TimePosition::fromSeconds(pasteTime), newClip->getPosition().getLength()}, &evs.m_selectionManager);

                if (auto owner = dynamic_cast<te::ClipOwner *>(targetTrack))
                    newClip->moveTo(*owner);
                newClip->setStart(tracktion::TimePosition::fromSeconds(pasteTime), false, true);

                evs.m_selectionManager.addToSelection(newClip);
            }
        }
    }

    if (testOptimisation)
        restorePluginStates(involvedTracks, states);
}

void EngineHelpers::duplicateSelectedClips(EditViewState &evs) { moveSelectedClips(true, getTimeRangeOfSelectedClips(evs).getLength().inSeconds(), 0, evs); }

void EngineHelpers::copyAutomationForSelectedClips(double offset, te::SelectionManager &sm, bool copy)
{
    const auto clipSelection = sm.getSelectedObjects().getItemsOfType<te::Clip>();

    if (clipSelection.size() <= 0)
        return;

    juce::Array<te::TrackAutomationSection> sections;

    for (const auto &selectedClip : clipSelection)
        if (selectedClip->getTrack() != nullptr)
            sections.add(te::TrackAutomationSection(*selectedClip));

    te::moveAutomation(sections, tracktion::TimeDuration::fromSeconds(offset), copy);
}

void EngineHelpers::selectAllClips(te::SelectionManager &sm, te::Edit &edit)
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
void EngineHelpers::selectAllClipsOnTrack(te::SelectionManager &sm, te::AudioTrack &at)
{
    sm.deselectAll();

    for (auto c : at.getClips())
    {
        sm.addToSelection(c);
    }
}
static tracktion::AutomationCurve *getDestCurve(tracktion::Track &t, const tracktion::AutomatableParameter::Ptr &p)
{
    if (p != nullptr)
    {
        if (auto plugin = p->getPlugin())
        {
            auto name = plugin->getName();

            for (auto f : t.getAllPlugins())
                if (f->getName() == name)
                    if (auto param = f->getAutomatableParameter(plugin->indexOfAutomatableParameter(p)))
                        return &param->getCurve();
        }
    }

    return {};
}
static bool mergeInto(const tracktion::TrackAutomationSection &s, juce::Array<tracktion::TrackAutomationSection> &dst)
{
    for (auto &dstSeg : dst)
    {
        if (dstSeg.overlaps(s))
        {
            dstSeg.mergeIn(s);
            return true;
        }
    }

    return false;
}

static void mergeSections(const juce::Array<tracktion::TrackAutomationSection> &src, juce::Array<tracktion::TrackAutomationSection> &dst)
{
    for (const auto &srcSeg : src)
        if (!mergeInto(srcSeg, dst))
            dst.add(srcSeg);
}

void EngineHelpers::moveAutomationOrCopy(const juce::Array<tracktion::TrackAutomationSection> &origSections, tracktion::TimeDuration offset, bool copy)
{
    if (origSections.isEmpty())
        return;

    juce::Array<tracktion::TrackAutomationSection> sections;
    mergeSections(origSections, sections);

    // find all the original curves
    for (auto &&section : sections)
    {
        for (auto &ap : section.activeParameters)
            ap.curve.state = ap.curve.state.createCopy();
    }

    // delete all the old curves
    if (!copy)
    {
        for (auto &section : sections)
        {
            auto sectionTime = section.position;

            for (auto &&activeParam : section.activeParameters)
            {
                auto param = activeParam.param;
                auto &curve = param->getCurve();
                constexpr auto tolerance = tracktion::TimeDuration::fromSeconds(0.0001);

                auto startValue = curve.getValueAt(sectionTime.getStart() - tolerance);
                auto endValue = curve.getValueAt(sectionTime.getEnd() + tolerance);

                auto idx = curve.indexBefore(sectionTime.getEnd() + tolerance);
                auto endCurve = (idx == -1) ? 0.0f : curve.getPointCurve(idx);

                curve.removePointsInRegion(sectionTime.expanded(tolerance));

                if (std::abs(startValue - endValue) < 0.0001f)
                {
                    curve.addPoint(sectionTime.getStart(), startValue, 0.0f);
                    curve.addPoint(sectionTime.getEnd(), endValue, endCurve);
                }
                else if (startValue > endValue)
                {
                    curve.addPoint(sectionTime.getStart(), startValue, 0.0f);
                    curve.addPoint(sectionTime.getStart(), endValue, 0.0f);
                    curve.addPoint(sectionTime.getEnd(), endValue, endCurve);
                }
                else
                {
                    curve.addPoint(sectionTime.getStart(), startValue, 0.0f);
                    curve.addPoint(sectionTime.getEnd(), startValue, 0.0f);
                    curve.addPoint(sectionTime.getEnd(), endValue, endCurve);
                }

                curve.removeRedundantPoints(sectionTime.expanded(tolerance));
            }
        }
    }

    // recreate the curves
    for (auto &section : sections)
    {
        for (auto &activeParam : section.activeParameters)
        {
            auto sectionTime = section.position;

            if (auto dstCurve = (section.src == section.dst) ? &activeParam.param->getCurve() : getDestCurve(*section.dst, activeParam.param))
            {
                constexpr auto errorMargin = tracktion::TimeDuration::fromSeconds(0.0001);

                auto start = sectionTime.getStart();
                auto end = sectionTime.getEnd();
                auto newStart = start + offset;
                auto newEnd = end + offset;

                auto &srcCurve = activeParam.curve;

                auto idx1 = srcCurve.indexBefore(newEnd + errorMargin);
                auto endCurve = idx1 < 0 ? 0 : srcCurve.getPointCurve(idx1);

                auto idx2 = srcCurve.indexBefore(start - errorMargin);
                auto startCurve = idx2 < 0 ? 0 : srcCurve.getPointCurve(idx2);

                auto srcStartVal = srcCurve.getValueAt(start - errorMargin);
                auto srcEndVal = srcCurve.getValueAt(end + errorMargin);

                auto dstStartVal = dstCurve->getValueAt(newStart - errorMargin);
                auto dstEndVal = dstCurve->getValueAt(newEnd + errorMargin);

                tracktion::TimeRange totalRegionWithMargin(newStart - errorMargin, newEnd + errorMargin);
                tracktion::TimeRange startWithMargin(newStart - errorMargin, newStart + errorMargin);
                tracktion::TimeRange endWithMargin(newEnd - errorMargin, newEnd + errorMargin);

                juce::Array<tracktion::AutomationCurve::AutomationPoint> origPoints;

                for (int i = 0; i < srcCurve.getNumPoints(); ++i)
                {
                    auto pt = srcCurve.getPoint(i);

                    if (pt.time >= start - errorMargin && pt.time <= sectionTime.getEnd() + errorMargin)
                        origPoints.add(pt);
                }

                dstCurve->removePointsInRegion(totalRegionWithMargin);

                for (const auto &pt : origPoints)
                    dstCurve->addPoint(pt.time + offset, pt.value, pt.curve);

                auto startPoints = dstCurve->getPointsInRegion(startWithMargin);
                auto endPoints = dstCurve->getPointsInRegion(endWithMargin);

                dstCurve->removePointsInRegion(startWithMargin);
                dstCurve->removePointsInRegion(endWithMargin);

                dstCurve->addPoint(newStart, dstStartVal, startCurve);
                dstCurve->addPoint(newStart, srcStartVal, startCurve);

                for (auto &point : startPoints)
                    dstCurve->addPoint(newStart, point.value, point.curve);

                for (auto &point : endPoints)
                    dstCurve->addPoint(newEnd, point.value, point.curve);

                dstCurve->addPoint(newEnd, srcEndVal, endCurve);
                dstCurve->addPoint(newEnd, dstEndVal, endCurve);

                dstCurve->removeRedundantPoints(totalRegionWithMargin);
            }
        }
    }

    // activate the automation curves on the new tracks
    juce::Array<tracktion::Track *> src, dst;

    for (auto &section : sections)
    {
        if (section.src != section.dst)
        {
            if (!src.contains(section.src.get()))
            {
                src.add(section.src.get());
                dst.add(section.dst.get());
            }
        }
    }

    for (int i = 0; i < src.size(); ++i)
    {
        if (auto ap = src.getUnchecked(i)->getCurrentlyShownAutoParam())
        {
            for (auto p : dst.getUnchecked(i)->getAllAutomatableParams())
            {
                if (p->getPluginAndParamName() == ap->getPluginAndParamName())
                {
                    dst.getUnchecked(i)->setCurrentlyShownAutoParam(p);
                    break;
                }
            }
        }
    }
}

void EngineHelpers::moveAutomation(te::Track *src, te::TrackAutomationSection::ActiveParameters par, tracktion::TimeRange range, double insertTime, bool copy)
{
    te::TrackAutomationSection section;
    section.src = src;
    section.dst = src;
    section.position = range;
    section.activeParameters.add(par);

    juce::Array<te::TrackAutomationSection> secs;
    secs.add(section);
    auto offset = tracktion::TimePosition::fromSeconds(insertTime) - range.getStart();

    te::moveAutomation(secs, offset, copy);
}

te::TrackAutomationSection EngineHelpers::getTrackAutomationSection(te::AutomatableParameter *ap, tracktion::TimeRange tr)
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
void EngineHelpers::resizeSelectedClips(bool fromLeftEdge, double delta, EditViewState &evs)
{
    auto selectedClips = evs.m_selectionManager.getItemsOfType<te::Clip>();
    auto tempPosition = evs.m_edit.getLength().inSeconds() + delta;

    if (fromLeftEdge)
    {
        for (auto sc : selectedClips)
        {
            auto newStart = juce::jmax(sc->getPosition().getStart() - sc->getPosition().getOffset(), sc->getPosition().getStart() + tracktion::TimeDuration::fromSeconds(delta));
            newStart = juce::jmax(tracktion::TimePosition::fromSeconds(0), newStart);
            sc->setStart(newStart, true, false);

            // save clip for damage
            sc->setStart(sc->getPosition().getStart() + tracktion::TimeDuration::fromSeconds(tempPosition), false, true);
        }
    }
    else
    {
        for (auto sc : selectedClips)
        {
            auto newEnd = sc->getPosition().getEnd() + tracktion::TimeDuration::fromSeconds(delta);

            sc->setEnd(newEnd, true);
            // save clip for damage
            sc->setStart(sc->getPosition().getStart() + tracktion::TimeDuration::fromSeconds(tempPosition), false, true);
        }
    }

    for (auto sc : selectedClips)
    {
        if (auto ct = sc->getClipTrack())
        {
            const tracktion::TimeRange range = {sc->getPosition().getStart() - tracktion::TimeDuration::fromSeconds(tempPosition), sc->getPosition().getEnd() - tracktion::TimeDuration::fromSeconds(tempPosition)};
            ct->deleteRegion(range, &evs.m_selectionManager);
        }

        // restore clip
        sc->setStart(sc->getPosition().getStart() - tracktion::TimeDuration::fromSeconds(tempPosition), false, true);
    }
}

tracktion_engine::Project::Ptr EngineHelpers::createTempProject(tracktion_engine::Engine &engine)
{
    auto file = engine.getTemporaryFileManager().getTempDirectory().getChildFile("temp_project").withFileExtension(te::projectFileSuffix);
    te::ProjectManager::TempProject tempProject(engine.getProjectManager(), file, true);
    return tempProject.project;
}

void EngineHelpers::browseForAudioFile(tracktion_engine::Engine &engine, std::function<void(const juce::File &)> fileChosenCallback)
{
    auto fc = std::make_shared<juce::FileChooser>("Please select an audio file to load...", engine.getPropertyStorage().getDefaultLoadSaveDirectory("pitchAndTimeExample"), engine.getAudioFileFormatManager().readFormatManager.getWildcardForAllFormats());

    fc->launchAsync(juce::FileBrowserComponent::openMode + juce::FileBrowserComponent::canSelectFiles,
                    [fc, &engine, callback = std::move(fileChosenCallback)](const juce::FileChooser &)
                    {
                        const auto f = fc->getResult();

                        if (f.existsAsFile())
                            engine.getPropertyStorage().setDefaultLoadSaveDirectory("pitchAndTimeExample", f.getParentDirectory());

                        callback(f);
                    });
}

void EngineHelpers::removeAllClips(tracktion_engine::AudioTrack &track)
{
    const auto &clips = track.getClips();

    for (int i = clips.size(); --i >= 0;)
        clips.getUnchecked(i)->removeFromParent();
}

tracktion_engine::AudioTrack *EngineHelpers::getOrInsertAudioTrackAt(tracktion_engine::Edit &edit, int index)
{
    edit.ensureNumberOfAudioTracks(index + 1);
    return te::getAudioTracks(edit)[index];
}
tracktion_engine::FolderTrack::Ptr EngineHelpers::addFolderTrack(juce::Colour trackColour, EditViewState &evs)
{
    evs.m_edit.ensureMasterTrack();

    auto allTracks = te::getAllTracks(evs.m_edit);
    te::TrackInsertPoint tip{nullptr, nullptr};

    te::Track *lastNonMasterTrack = nullptr;
    for (int i = allTracks.size(); --i >= 0;)
    {
        auto *track = allTracks.getUnchecked(i);
        if (track != nullptr && !track->isMasterTrack())
        {
            lastNonMasterTrack = track;
            break;
        }
    }

    if (lastNonMasterTrack != nullptr)
        tip = te::TrackInsertPoint(*lastNonMasterTrack, true);
    else if (auto *masterTrack = evs.m_edit.getMasterTrack())
        tip = te::TrackInsertPoint{nullptr, masterTrack};

    auto ft = evs.m_edit.insertNewFolderTrack(tip, &evs.m_selectionManager, true);

    ft->state.setProperty(te::IDs::height, (int)evs.m_trackDefaultHeight, nullptr);
    ft->state.setProperty(IDs::isTrackMinimized, false, nullptr);

    ft->state.setProperty(IDs::isMidiTrack, false, &evs.m_edit.getUndoManager());

    juce::String num = juce::String(te::getAudioTracks(evs.m_edit).size());
    ft->setName("Folder " + num);
    ft->setColour(trackColour);
    evs.m_selectionManager.selectOnly(ft);

    evs.m_trackHeightManager->regenerateTrackHeightsFromEdit(evs.m_edit);

    return ft;
}

tracktion_engine::AudioTrack::Ptr EngineHelpers::addAudioTrack(bool isMidiTrack, juce::Colour trackColour, EditViewState &evs)
{
    if (auto track = EngineHelpers::getOrInsertAudioTrackAt(evs.m_edit, te::getAudioTracks(evs.m_edit).size()))
    {
        track->state.setProperty(te::IDs::height, (int)evs.m_trackDefaultHeight, nullptr);
        track->state.setProperty(IDs::isTrackMinimized, false, nullptr);

        track->state.setProperty(IDs::isMidiTrack, isMidiTrack, &evs.m_edit.getUndoManager());

        juce::String num = juce::String(te::getAudioTracks(evs.m_edit).size());
        track->setName(isMidiTrack ? "Instrument " + num : "Wave " + num);
        track->setColour(trackColour);
        evs.m_selectionManager.selectOnly(track);
        evs.m_trackHeightManager->regenerateTrackHeightsFromEdit(evs.m_edit);

        return track;
    }
    return nullptr;
}

tracktion_engine::WaveAudioClip::Ptr EngineHelpers::loadAudioFileOnNewTrack(EditViewState &evs, const juce::File &file, juce::Colour trackColour, double insertTime)
{
    te::AudioFile audioFile(evs.m_edit.engine, file);
    if (audioFile.isValid())
    {
        if (auto track = addAudioTrack(false, trackColour, evs))
        {
            removeAllClips(*track);
            te::ClipPosition pos;
            pos.time = {tracktion::TimePosition::fromSeconds(insertTime), tracktion::TimeDuration::fromSeconds(audioFile.getLength())};
            loadAudioFileToTrack(file, track, pos);
        }
    }
    return {};
}

tracktion_engine::WaveAudioClip::Ptr EngineHelpers::loadAudioFileToTrack(const juce::File &file, te::AudioTrack::Ptr track, te::ClipPosition pos)
{

    if (auto newClip = track->insertWaveClip(file.getFileNameWithoutExtension(), file, pos, true))
    {
        GUIHelpers::log("loading : " + file.getFullPathName());
        newClip->setAutoTempo(false);
        newClip->setAutoPitch(false);
        newClip->setPosition(pos);
        return newClip;
    }

    return {};
}
void EngineHelpers::refreshRelativePathsToNewEditFile(EditViewState &evs, const juce::File &newFile)
{
    for (auto t : te::getAudioTracks(evs.m_edit))
    {
        for (auto c : t->getClips())
        {
            if (c->state.getProperty(te::IDs::source) != "")
            {
                auto source = evs.m_edit.filePathResolver(c->state.getProperty(te::IDs::source));
                auto relPath = source.getRelativePathFrom(newFile.getParentDirectory());

                c->state.setProperty(te::IDs::source, relPath, nullptr);
            }
        }
    }
    evs.m_edit.editFileRetriever = [newFile] { return newFile; };
}

void EngineHelpers::play(EditViewState &evs)
{
    GUIHelpers::log("play");
    auto &transport = evs.m_edit.getTransport();

    if (transport.isPlaying())
        transport.setPosition(tracktion::TimePosition::fromSeconds(static_cast<double>(evs.m_playHeadStartTime)));
    // hack for prevent not playing the first transient of a sample
    // that starts direct on play position
    auto currentPos = transport.getPosition();
    transport.setPosition(tracktion::TimePosition::fromSeconds(0) + evs.m_edit.getLength());
    transport.play(true);
    transport.setPosition(currentPos);
}

void EngineHelpers::stopPlay(EditViewState &evs)
{
    auto &transport = evs.m_edit.getTransport();
    if (!transport.isPlaying())
    {
        evs.m_playHeadStartTime = 0.0;
        transport.setPosition(tracktion::TimePosition::fromSeconds(static_cast<double>(evs.m_playHeadStartTime)));
        evs.setNewStartAndZoom("SongEditor", 0.0);
        transport.stop(false, true);
        GUIHelpers::log("EngineHelpers::stopPlay: stop and Device cleared.");
    }
    else
    {
        transport.stop(false, false);
        transport.setPosition(tracktion::TimePosition::fromSeconds(static_cast<double>(evs.m_playHeadStartTime)));
        GUIHelpers::log("EngineHelpers::stopPlay: stop.");
    }
}
void EngineHelpers::togglePlay(EditViewState &evs)
{
    auto &transport = evs.m_edit.getTransport();

    if (transport.isPlaying())
    {
        transport.stop(false, false);
    }
    else
    {
        evs.m_playHeadStartTime = transport.getPosition().inSeconds();
        EngineHelpers::play(evs);
    }
}

void EngineHelpers::toggleLoop(tracktion_engine::Edit &edit)
{
    auto &transport = edit.getTransport();

    if (transport.looping)
        transport.looping = false;
    else
        transport.looping = true;
}

void EngineHelpers::loopAroundSelection(EditViewState &evs)
{
    auto &transport = evs.m_edit.getTransport();

    transport.setLoopRange(getTimeRangeOfSelectedClips(evs));
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

void EngineHelpers::loopOff(te::Edit &edit)
{
    auto &transport = edit.getTransport();
    transport.looping = false;
}

void EngineHelpers::loopOn(te::Edit &edit)
{
    auto &transport = edit.getTransport();
    transport.looping = true;
}
void EngineHelpers::loopAroundAll(te::Edit &edit)
{
    auto &transport = edit.getTransport();
    transport.setLoopRange({tracktion::TimePosition::fromSeconds(0), edit.getLength()});
    transport.looping = true;
}

void EngineHelpers::toggleSnap(EditViewState &evs)
{

    if (evs.m_snapToGrid)
        evs.m_snapToGrid = false;
    else
        evs.m_snapToGrid = true;
}

void EngineHelpers::toggleMetronome(te::Edit &edit)
{
    GUIHelpers::log("toggle metronome");
    edit.clickTrackEnabled = !edit.clickTrackEnabled;
}

void EngineHelpers::toggleRecord(tracktion_engine::Edit &edit)
{
    auto &transport = edit.getTransport();

    if (transport.isRecording())
        transport.stop(true, false);
    else
        transport.record(false);
}

void EngineHelpers::armTrack(te::AudioTrack &t, bool arm, int position)
{
    auto &edit = t.edit;
    for (auto instance : edit.getAllInputDevices())
    {
        if (te::isOnTargetTrack(*instance, t, position))
        {
            GUIHelpers::log("Utilities.cpp: arm Track");
            instance->setRecordingEnabled(t.itemID, arm);
        }
    }
}

bool EngineHelpers::isTrackArmed(te::AudioTrack &t, int position)
{
    auto &edit = t.edit;
    for (auto instance : edit.getAllInputDevices())
        if (te::isOnTargetTrack(*instance, t, position))
            return instance->isRecordingEnabled(t.itemID);

    return false;
}

bool EngineHelpers::isInputMonitoringEnabled(te::AudioTrack &t, int position)
{
    for (auto instance : t.edit.getAllInputDevices())
        if (te::isOnTargetTrack(*instance, t, position))
            return instance->isLivePlayEnabled(t);

    return false;
}

void EngineHelpers::enableInputMonitoring(te::AudioTrack &t, bool im, int position)
{
    for (auto instance : t.edit.getAllInputDevices())
    {
        if (te::isOnTargetTrack(*instance, t, position))
        {
            instance->getInputDevice().setMonitorMode(im ? te::InputDevice::MonitorMode::on : te::InputDevice::MonitorMode::off);
        }
    }
}

bool EngineHelpers::trackHasInput(te::AudioTrack &t, int position)
{
    auto &edit = t.edit;
    for (auto instance : edit.getAllInputDevices())
        if (te::isOnTargetTrack(*instance, t, position))
            return true;

    return false;
}

std::unique_ptr<juce::KnownPluginList::PluginTree> EngineHelpers::createPluginTree(tracktion_engine::Engine &engine)
{
    auto &list = engine.getPluginManager().knownPluginList;

    if (auto tree = juce::KnownPluginList::createTree(list.getTypes(), juce::KnownPluginList::sortByManufacturer))
    {
        return tree;
    }
    return {};
}

void EngineHelpers::insertPlugin(te::Track::Ptr track, te::Plugin::Ptr plugin, int index)
{
    auto &plugins = track->pluginList;
    if (index == -1)
        index = plugins.size() - 2;
    plugin->state.setProperty(te::IDs::remapOnTempoChange, true, nullptr);
    plugins.insertPlugin(plugin->state, index);
}

// Helper class to bridge te::Plugin to PluginPresetInterface specifically for loading init presets
class InitPresetLoaderAdapter : public PluginPresetInterface
{
public:
    InitPresetLoaderAdapter(te::Plugin::Ptr p, ApplicationViewState &appState)
        : m_plugin(p),
          m_appState(appState)
    {
    }

    juce::ValueTree getPluginState() override { return m_plugin->state.createCopy(); }
    void restorePluginState(const juce::ValueTree &state) override { m_plugin->restorePluginStateFromValueTree(state); }
    juce::ValueTree getFactoryDefaultState() override { return {}; }

    juce::String getPresetSubfolder() const override { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

    juce::String getPluginTypeName() const override
    {
        if (auto *ep = dynamic_cast<te::ExternalPlugin *>(m_plugin.get()))
            return ep->desc.pluginFormatName + juce::String::toHexString(ep->desc.deprecatedUid).toUpperCase();
        return m_plugin->getPluginType();
    }

    ApplicationViewState &getApplicationViewState() override { return m_appState; }

    // Dummies - Not needed for one-shot init loading
    bool getInitialPresetLoaded() override { return false; }
    void setInitialPresetLoaded(bool) override {}
    juce::String getLastLoadedPresetName() override { return {}; }
    void setLastLoadedPresetName(const juce::String &) override {}

private:
    te::Plugin::Ptr m_plugin;
    ApplicationViewState &m_appState;
};

void EngineHelpers::insertPluginWithPreset(EditViewState &evs, te::Track::Ptr track, te::Plugin::Ptr plugin, int index)
{
    if (track == nullptr || plugin == nullptr)
        return;

    auto &plugins = track->pluginList;
    if (index == -1)
        index = plugins.size() - 2; // Mimic insertPlugin behavior (before level meter and volume)

    plugin->state.setProperty(te::IDs::remapOnTempoChange, true, nullptr);

    // Insert and capture the new plugin instance
    auto newPlugin = plugins.insertPlugin(plugin->state, index);

    if (newPlugin)
    {
        InitPresetLoaderAdapter adapter(newPlugin, evs.m_applicationState);
        PresetHelpers::tryLoadInitPreset(adapter);
    }
}

// void GUIHelpers::centerView(EditViewState &evs)
// {
//     if (evs.viewFollowsPos())
//     {
//         auto posBeats = evs.timeToBeat (
//             evs.m_edit.getTransport ().getCurrentPosition ());
//
//         auto x1 = evs.getSongEditorVisibleTimeRange(getWidth()).getStart().inSeconds();
//         auto x2 = evs.getVisibleTimeRange(m_timeLineID, getWidth()).getEnd().inSeconds();
//
//         if (posBeats < x1 || posBeats > x2)
//             moveView(evs, posBeats);
//
//         auto zoom = evs.m_viewX2 - evs.m_viewX1;
//         moveView(evs, juce::jmax((double)evs.m_viewX1, posBeats - zoom/2));
//     }
// }
//
// void GUIHelpers::moveView(EditViewState& evs, double newBeatPos)
// {
//     auto zoom = evs.m_viewX2 - evs.m_viewX1;
//     evs.m_viewX1 = newBeatPos;
//     evs.m_viewX2 = newBeatPos + zoom;
// }

float GUIHelpers::getZoomScaleFactor(int delta, float unitDistance) { return std::pow(2, (float)delta / unitDistance); }
juce::Rectangle<float> GUIHelpers::getSensibleArea(juce::Point<float> p, float w) { return {p.x - (w / 2), p.y - (w / 2), w, w}; }

void GUIHelpers::centerMidiEditorToClip(EditViewState &evs, te::Clip::Ptr c, juce::String timeLineID, int width)
{
    // Horizontal Zoom & Position
    auto clipLen = c->getLengthInBeats().inBeats();
    auto effectiveWidth = (double)juce::jmax(100, width);

    // Fit clip in 80% of width
    double newBeatsPerPixel = clipLen / (effectiveWidth * 0.8);

    // Center the clip
    auto clipStart = c->getStartBeat().inBeats();
    auto viewWidthInBeats = newBeatsPerPixel * effectiveWidth;
    auto startBeat = clipStart - (viewWidthInBeats - clipLen) / 2.0;
    startBeat = juce::jmax(0.0, startBeat);

    evs.setNewStartAndZoom(timeLineID, startBeat, newBeatsPerPixel);

    // Vertical Position (C3 / 60)
    double keyWidth = evs.getViewYScale(timeLineID);
    if (keyWidth <= 1)
    {
        keyWidth = 20.0;
        evs.setViewYScale(timeLineID, keyWidth);
    }

    double height = (double)evs.m_midiEditorHeight;
    double startKey = 61.0 - (height / (2.0 * keyWidth));
    startKey = juce::jmax(0.0, startKey);

    evs.setYScroll(timeLineID, startKey);
}

void GUIHelpers::drawPolyObject(juce::Graphics &g, juce::Rectangle<int> area, int edges, float tilt, float rotation, float radiusFac, float heightFac, float scale)
{
    const float pi = static_cast<float>(3.141592653589793238L);
    auto phi = 0.f + tilt;

    auto xm = area.getWidth() / 2;
    auto rx = (area.getHeight() / 3) * radiusFac * scale;
    auto yRot = juce::jmap(rotation, 0.f, static_cast<float>(rx));

    auto ry = ((area.getHeight() / 3) * scale * radiusFac) - yRot;

    int x = xm + rx * sinf(phi);
    int zLength = ((area.getHeight() - (area.getHeight() - juce::jmap(rotation, 0.f, static_cast<float>(area.getHeight())))) * heightFac) * 2 * scale;

    auto ym = area.getHeight() / 2 + (zLength / 2);
    int y = ym + ry * cosf(phi);

    juce::Path poly;

    while (phi < (2 * pi) + tilt)
    {
        auto oldX = x;
        auto oldY = y;

        phi = phi + (2 * pi / edges);

        x = xm + rx * sinf(phi);
        y = ym + ry * cosf(phi);

        juce::Line<float> zEdge(oldX, oldY - zLength, oldX, oldY);
        juce::Line<float> topEdge(oldX, oldY - zLength, x, y - zLength);
        juce::Line<float> bottomEdge(oldX, oldY, x, y);

        poly.addLineSegment(zEdge, 2);
        poly.addLineSegment(topEdge, 2);
        poly.addLineSegment(bottomEdge, 2);
    }
    auto st = juce::PathStrokeType(2);
    st.setJointStyle(juce::PathStrokeType::JointStyle::beveled);

    g.strokePath(poly, st);
}

void GUIHelpers::drawLogoQuad(juce::Graphics &g, juce::Rectangle<int> area)
{
    const float pi = static_cast<float>(3.141592653589793238L);
    juce::Path path;
    auto roundEdge = 50;

    path.startNewSubPath(area.getX(), area.getY() + roundEdge / 2);
    path.addArc(area.getX(), area.getY(), roundEdge, roundEdge, pi + (pi / 2), 2 * pi);
    path.lineTo(area.getWidth() - roundEdge, area.getY());
    path.addArc(area.getWidth() - roundEdge, area.getY(), roundEdge, roundEdge, pi + (pi / 2) + (pi / 2), 2 * pi + (pi / 2));

    g.strokePath(path, juce::PathStrokeType(2));
}
void EngineHelpers::sortByName(juce::Array<juce::PluginDescription> &list, bool forward)
{

    if (forward)
    {
        CompareNameForward cf;
        list.sort(cf);
    }
    else
    {
        CompareNameBackwards cb;
        list.sort(cb);
    }
}
void EngineHelpers::sortByFormatName(juce::Array<juce::PluginDescription> &list, bool forward)
{
    if (forward)
    {
        CompareFormatForward cf;
        list.sort(cf);
    }
    else
    {
        CompareFormatBackward cb;
        list.sort(cb);
    }
}

juce::PluginDescription EngineHelpers::getPluginDesc(const juce::String &uniqueId, const juce::String &name, juce::String xmlType_, bool isSynth)
{

    auto desc = juce::PluginDescription();

    jassert(xmlType_.isNotEmpty());
    desc.name = name;
    desc.fileOrIdentifier = uniqueId;
    desc.pluginFormatName = (uniqueId.endsWith("_trkbuiltin") || xmlType_ == te::RackInstance::xmlTypeName) ? getInternalPluginFormatName() : juce::String();
    desc.category = xmlType_;
    desc.isInstrument = isSynth;

    return desc;
}

juce::Array<juce::PluginDescription> EngineHelpers::getInternalPlugins()
{
    auto num = 1;

    juce::Array<juce::PluginDescription> list;

    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(te::VolumeAndPanPlugin::getPluginName()), te::VolumeAndPanPlugin::xmlTypeName, false));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(te::EqualiserPlugin::getPluginName()), te::EqualiserPlugin::xmlTypeName, false));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(te::ReverbPlugin::getPluginName()), te::ReverbPlugin::xmlTypeName, false));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(te::DelayPlugin::getPluginName()), te::DelayPlugin::xmlTypeName, false));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(te::ChorusPlugin::getPluginName()), te::ChorusPlugin::xmlTypeName, false));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(te::PhaserPlugin::getPluginName()), te::PhaserPlugin::xmlTypeName, false));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(te::CompressorPlugin::getPluginName()), te::CompressorPlugin::xmlTypeName, false));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(te::PitchShiftPlugin::getPluginName()), te::PitchShiftPlugin::xmlTypeName, false));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(te::LowPassPlugin::getPluginName()), te::LowPassPlugin::xmlTypeName, false));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(te::SamplerPlugin::getPluginName()), te::SamplerPlugin::xmlTypeName, true));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(te::FourOscPlugin::getPluginName()), te::FourOscPlugin::xmlTypeName, true));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(SimpleSynthPlugin::getPluginName()), SimpleSynthPlugin::xmlTypeName, true));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(ArpeggiatorPlugin::getPluginName()), ArpeggiatorPlugin::xmlTypeName, false));
    list.add(getPluginDesc(juce::String(num++) + "_trkbuiltin", TRANS(SpectrumAnalyzerPlugin::getPluginName()), SpectrumAnalyzerPlugin::xmlTypeName, false));
    return list;
}
