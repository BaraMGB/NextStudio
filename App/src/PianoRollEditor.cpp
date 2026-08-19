
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

#include "PianoRollEditor.h"
#include "KeyboardView.h"
#include "Utilities.h"

PianoRollEditor::PianoRollEditor(EditViewState &evs)
    : m_editViewState(evs),
      m_timeLine(evs, "PianoRoll", true),
      m_notePropertiesBar(evs),
      m_playhead(evs.m_edit, evs, m_timeLine),
      m_toolBar(Alignment::Center),
      m_selectionBtn("select mode", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize),
      m_drawBtn("draw mode", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize),
      m_rangeSelectBtn("range select mode", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize),
      m_erasorBtn("erasor mode", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize),
      m_splitBtn("split mode", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize),
      m_lassoBtn("lasso mode", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
{
    setWantsKeyboardFocus(true);
    evs.m_edit.state.addListener(this);
    evs.m_applicationState.m_applicationStateValueTree.addListener(this);
    evs.m_selectionManager.addChangeListener(this);
    m_midiKeyChangeDispatcher->listeners.add(this);

    m_notePropertiesBar.setSelectionProvider([this]
    {
        juce::Array<std::pair<te::MidiClip *, te::MidiNote *>> selection;
        if (m_pianoRollViewPort == nullptr)
            return selection;

        const auto &clips = m_pianoRollViewPort->getCachedMidiClips();
        for (auto *note : m_pianoRollViewPort->getSelectedNotes())
        {
            for (auto *clip : clips)
            {
                if (clip != nullptr && clip->getSequence().getNotes().contains(note))
                {
                    selection.add({clip, note});
                    break;
                }
            }
        }
        return selection;
    });
    m_notePropertiesBar.setEditHandlers(
        [this](const juce::Array<MidiNotePropertyEdit> &preview)
        {
            if (m_pianoRollViewPort != nullptr)
                m_pianoRollViewPort->setNotePropertyPreview(preview);
            if (m_velocityEditor != nullptr)
                m_velocityEditor->setNotePropertyPreview(preview);
        },
        [this](NotePropertiesBar::Property property, const juce::Array<MidiNotePropertyEdit> &edits)
        {
            if (m_pianoRollViewPort != nullptr)
                m_pianoRollViewPort->commitNotePropertyEdit(edits, property != NotePropertiesBar::Property::velocity);
            if (m_velocityEditor != nullptr)
                m_velocityEditor->repaint();
        });
    m_notePropertiesBar.setTimingStepProvider([this]
    {
        return m_timeLine.isSnappingEnabled()
                   ? m_timeLine.getSnapIntervalBeats()
                   : 1.0 / te::Edit::ticksPerQuarterNote;
    });

    addAndMakeVisible(m_notePropertiesBar);
    addAndMakeVisible(m_timeLine);
    addAndMakeVisible(m_playhead);
    addAndMakeVisible(m_horizontalScrollBar);
    m_playhead.setAlwaysOnTop(true);
    m_horizontalScrollBar.setAlwaysOnTop(true);
    m_horizontalScrollBar.setAutoHide(false);
    m_horizontalScrollBar.addListener(this);
    m_horizontalScrollBar.setVisible(false);

    m_selectionBtn.setName("select");
    m_selectionBtn.setTooltip(GUIHelpers::translate("select clips or automation points", m_editViewState.m_applicationState));
    m_selectionBtn.addListener(this);

    m_drawBtn.setName("draw");
    m_drawBtn.setTooltip(GUIHelpers::translate("draw mode", m_editViewState.m_applicationState));
    m_drawBtn.addListener(this);

    m_rangeSelectBtn.setName("range select");
    m_rangeSelectBtn.setTooltip(GUIHelpers::translate("range select mode", m_editViewState.m_applicationState));
    m_rangeSelectBtn.addListener(this);

    m_erasorBtn.setName("erasor");
    m_erasorBtn.setTooltip(GUIHelpers::translate("erasor mode", m_editViewState.m_applicationState));
    m_erasorBtn.addListener(this);

    m_splitBtn.setName("split");
    m_splitBtn.setTooltip(GUIHelpers::translate("split clips", m_editViewState.m_applicationState));
    m_splitBtn.addListener(this);

    m_lassoBtn.setName("lasso");
    m_lassoBtn.setTooltip(GUIHelpers::translate("lasso select", m_editViewState.m_applicationState));
    m_lassoBtn.addListener(this);

    addAndMakeVisible(m_toolBar);
    m_toolBar.addButton(&m_selectionBtn, 1);
    m_toolBar.addButton(&m_drawBtn, 1);
    m_toolBar.addButton(&m_rangeSelectBtn, 1);
    m_toolBar.addButton(&m_erasorBtn, 1);
    m_toolBar.addButton(&m_splitBtn, 1);
    m_toolBar.addButton(&m_lassoBtn, 1);

    updateButtonColour();
}
PianoRollEditor::~PianoRollEditor()
{
    m_midiKeyChangeDispatcher->listeners.remove(this);
    m_splitBtn.removeListener(this);
    m_erasorBtn.removeListener(this);
    m_rangeSelectBtn.removeListener(this);
    m_drawBtn.removeListener(this);
    m_selectionBtn.removeListener(this);
    m_lassoBtn.removeListener(this);
    m_horizontalScrollBar.removeListener(this);
    m_editViewState.m_selectionManager.removeChangeListener(this);
    m_editViewState.m_applicationState.m_applicationStateValueTree.removeListener(this);
    m_editViewState.m_edit.state.removeListener(this);
}
void PianoRollEditor::paint(juce::Graphics &g)
{
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour1());
    g.fillAll();

    g.setColour(m_editViewState.m_applicationState.getBackgroundColour2());
    g.fillRect(getNotePropertiesRect());
    g.fillRect(getTimeLineRect());
    g.fillRect(getTimelineHelperRect());
    g.fillRect(getKeyboardRect());
    g.fillRect(getMidiEditorRect());
    g.fillRect(getVelocityEditorRect());
    g.fillRect(getParameterToolbarRect());
    g.fillRect(getHorizontalScrollbarSpacerRect());
    g.fillRect(getHorizontalScrollbarRect());

    g.setColour(juce::Colours::white);
    if (m_pianoRollViewPort == nullptr)
        g.drawText("select a clip for edit midi", getMidiEditorRect(), juce::Justification::centred);
}
void PianoRollEditor::paintOverChildren(juce::Graphics &g)
{
    g.setColour(juce::Colour(0xffffffff));

    const auto snapMode = static_cast<PianoRollSnapMode>(static_cast<int>(m_editViewState.m_pianoRollSnapMode));
    juce::String snapDescription;
    if (snapMode == PianoRollSnapMode::off)
    {
        snapDescription = "Off";
    }
    else if (snapMode == PianoRollSnapMode::fixed)
    {
        snapDescription = "1/" + juce::String(juce::jmax(1, static_cast<int>(m_editViewState.m_pianoRollSnapDenominator)));
    }
    else
    {
        const auto snapType = m_timeLine.getBestSnapType();
        snapDescription = "Adaptive: " + m_timeLine.getEditViewState().getSnapTypeDescription(snapType.level);
    }

    auto footerContent = getFooterRect().reduced(10, 0);
    auto snapBounds = footerContent.removeFromRight(juce::jmin(180, footerContent.getWidth()));
    g.drawFittedText(snapDescription, snapBounds, juce::Justification::centredRight, 1);

    auto noteBounds = footerContent.removeFromRight(juce::jmin(100, footerContent.getWidth()));
    g.drawText(m_NoteDescUnderCursor, noteBounds, juce::Justification::centredLeft);

    g.setColour(m_editViewState.m_applicationState.getBorderColour());
    g.fillRect(getHeaderRect().removeFromBottom(1));
    g.fillRect(getNotePropertiesRect().removeFromBottom(1));
    g.fillRect(getTimeLineRect().removeFromBottom(1));
    g.fillRect(getTimelineHelperRect().removeFromBottom(1));
    g.fillRect(getTimelineHelperRect().removeFromRight(1));
    g.fillRect(getVelocityEditorRect().removeFromTop(1));
    g.fillRect(getParameterToolbarRect().removeFromTop(1));
    g.fillRect(getHorizontalScrollbarSpacerRect().removeFromTop(1));
    g.fillRect(getHorizontalScrollbarRect().removeFromTop(1));
    g.fillRect(getFooterRect().removeFromTop(1));
    g.fillRect(getParameterToolbarRect().removeFromRight(1));
}
void PianoRollEditor::resized()
{
    auto keyboard = getKeyboardRect();
    auto timeline = getTimeLineRect();
    auto playhead = getPlayHeadRect();
    auto toolbar = getToolBarRect();

    m_toolBar.setBounds(toolbar);
    m_notePropertiesBar.setBounds(getNotePropertiesRect());

    if (m_keyboard != nullptr)
        m_keyboard->setBounds(keyboard);
    m_timeLine.setBounds(timeline);

    if (m_timelineOverlay != nullptr)
        m_timelineOverlay->setBounds(getTimeLineRect().getUnion(getMidiEditorRect()));

    if (m_velocityEditor != nullptr)
        m_velocityEditor->setBounds(getVelocityEditorRect());

    if (m_pianoRollViewPort != nullptr)
        m_pianoRollViewPort->setBounds(getMidiEditorRect());

    m_playhead.setBounds(playhead);
    m_horizontalScrollBar.setBounds(getHorizontalScrollbarRect());
    updateHorizontalScrollBar();
}

void PianoRollEditor::updateHorizontalScrollBar()
{
    const auto timelineWidth = m_timeLine.getWidth();

    if (timelineWidth <= 0)
        return;

    const auto visibleBeatRange = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), timelineWidth);

    m_horizontalScrollBar.setRangeLimits({0.0, m_editViewState.getEndScrollBeat()}, juce::dontSendNotification);
    m_horizontalScrollBar.setCurrentRange({visibleBeatRange.getStart().inBeats(), visibleBeatRange.getEnd().inBeats()}, juce::dontSendNotification);
}

int PianoRollEditor::getScrollbarThickness() const
{
    return m_editViewState.m_applicationState.getScrollbarThickness();
}

void PianoRollEditor::scrollBarMoved(juce::ScrollBar *scrollBarThatHasMoved, double newRangeStart)
{
    if (scrollBarThatHasMoved != &m_horizontalScrollBar)
        return;

    m_editViewState.setNewStartAndZoom(m_timeLine.getTimeLineID(), newRangeStart);
}

juce::Rectangle<int> PianoRollEditor::getPlayHeadRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromTop(getNotePropertiesRect().getHeight());
    area.removeFromLeft(m_editViewState.m_keyboardWidth);
    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromBottom(getScrollbarThickness());
    return area;
}
bool PianoRollEditor::keyPressed(const juce::KeyPress &key)
{
    if (m_pianoRollViewPort != nullptr && m_pianoRollViewPort->hasPendingPaste())
    {
        if (key.getKeyCode() == juce::KeyPress::returnKey)
            return m_pianoRollViewPort->confirmPendingPaste(true);

        if (key.getKeyCode() == juce::KeyPress::escapeKey)
            return m_pianoRollViewPort->cancelPendingPaste();
    }

    return juce::Component::keyPressed(key);
}

bool PianoRollEditor::confirmPendingPasteIfActive()
{
    return m_pianoRollViewPort != nullptr && m_pianoRollViewPort->confirmPendingPaste(true);
}

void PianoRollEditor::getAllCommands(juce::Array<juce::CommandID> &commands)
{

    juce::Array<juce::CommandID> ids{

        KeyPressCommandIDs::deleteSelectedNotes, KeyPressCommandIDs::duplicateSelectedNotes,
        KeyPressCommandIDs::copySelectedNotes, KeyPressCommandIDs::pasteNotesInPlace,
        KeyPressCommandIDs::confirmPendingPaste, KeyPressCommandIDs::cancelPendingPaste,
        KeyPressCommandIDs::nudgeNotesUp, KeyPressCommandIDs::nudgeNotesDown,
        KeyPressCommandIDs::nudgeNotesLeft, KeyPressCommandIDs::nudgeNotesRight,
        KeyPressCommandIDs::nudgeNotesOctaveUp, KeyPressCommandIDs::nudgeNotesOctaveDown,
    };

    commands.addArray(ids);
}

void PianoRollEditor::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo &result)
{

    switch (commandID)
    {
    case KeyPressCommandIDs::deleteSelectedNotes:
        result.setInfo("delete selected notes", "delete selected", "Notes", 0);
        result.addDefaultKeypress(juce::KeyPress::backspaceKey, 0);
        result.addDefaultKeypress(juce::KeyPress::deleteKey, 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("x").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    case KeyPressCommandIDs::duplicateSelectedNotes:
        result.setInfo("duplicate selected Notes", "duplicate selected Notes", "Notes", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("d").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    case KeyPressCommandIDs::copySelectedNotes:
        result.setInfo("copy selected Notes", "copy selected Notes", "Notes", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("c").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    case KeyPressCommandIDs::pasteNotesInPlace:
        result.setInfo("paste Notes in place", "paste Notes in place", "Notes", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("v").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    case KeyPressCommandIDs::confirmPendingPaste:
        result.setInfo("confirm pending MIDI paste", "confirm pending MIDI paste", "Notes", 0);
        break;
    case KeyPressCommandIDs::cancelPendingPaste:
        result.setInfo("cancel pending MIDI paste", "cancel pending MIDI paste", "Notes", 0);
        result.addDefaultKeypress(juce::KeyPress::escapeKey, 0);
        break;

    case KeyPressCommandIDs::nudgeNotesUp:

        result.setInfo("move selected Notes up", "move selected notes up", "Notes", 0);
        result.addDefaultKeypress(juce::KeyPress::upKey, 0);
        break;
    case KeyPressCommandIDs::nudgeNotesDown:
        result.setInfo("move selected Notes down", "move selected notes down", "Notes", 0);
        result.addDefaultKeypress(juce::KeyPress::downKey, 0);
        break;
    case KeyPressCommandIDs::nudgeNotesLeft:
        result.setInfo("move selected Notes left", "move selected notes left ", "Notes", 0);
        result.addDefaultKeypress(juce::KeyPress::leftKey, 0);
        break;
    case KeyPressCommandIDs::nudgeNotesRight:
        result.setInfo("move selected Notes right", "move selected notes right", "Notes", 0);
        result.addDefaultKeypress(juce::KeyPress::rightKey, 0);
        break;
    case KeyPressCommandIDs::nudgeNotesOctaveUp:
        result.setInfo("move selected Notes ocatave up", "move selected notes octave up", "Notes", 0);
        result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::commandModifier);
        break;
    case KeyPressCommandIDs::nudgeNotesOctaveDown:
        result.setInfo("move selected Notes octave down", "move selected notes octave down", "Notes", 0);
        result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::commandModifier);
        break;

    default:
        break;
    }
}

bool PianoRollEditor::perform(const juce::ApplicationCommandTarget::InvocationInfo &info)
{

    switch (info.commandID)
    {
    case KeyPressCommandIDs::deleteSelectedNotes:
    {
        if (m_pianoRollViewPort != nullptr)
        {
            if (!m_pianoRollViewPort->cancelPendingPaste())
                m_pianoRollViewPort->deleteSelectedNotes();
        }

        break;
    }
    case KeyPressCommandIDs::duplicateSelectedNotes:
    {
        if (m_pianoRollViewPort != nullptr)
            m_pianoRollViewPort->duplicateSelectedNotes();

        break;
    }
    case KeyPressCommandIDs::copySelectedNotes:
    {
        if (m_pianoRollViewPort != nullptr)
        {
            auto clipboard = m_pianoRollViewPort->copySelectedNotesToClipboard();
            if (!clipboard.isEmpty())
                m_midiClipboard = std::move(clipboard);
        }
        break;
    }
    case KeyPressCommandIDs::pasteNotesInPlace:
    {
        if (m_pianoRollViewPort != nullptr && m_pianoRollViewPort->beginPendingPaste(m_midiClipboard))
            grabKeyboardFocus();
        break;
    }
    case KeyPressCommandIDs::confirmPendingPaste:
    {
        confirmPendingPasteIfActive();
        break;
    }
    case KeyPressCommandIDs::cancelPendingPaste:
    {
        if (m_pianoRollViewPort != nullptr)
            m_pianoRollViewPort->cancelPendingPaste();
        break;
    }
    case KeyPressCommandIDs::nudgeNotesUp:
    {
        if (m_pianoRollViewPort != nullptr &&
            !m_pianoRollViewPort->nudgePendingPaste(0, 1))
            m_pianoRollViewPort->nudgeSelectedNotes(0, 1);

        break;
    }
    case KeyPressCommandIDs::nudgeNotesDown:
    {
        if (m_pianoRollViewPort != nullptr &&
            !m_pianoRollViewPort->nudgePendingPaste(0, -1))
            m_pianoRollViewPort->nudgeSelectedNotes(0, -1);
        break;
    }
    case KeyPressCommandIDs::nudgeNotesLeft:
    {
        if (m_pianoRollViewPort != nullptr &&
            !m_pianoRollViewPort->nudgePendingPaste(-1, 0))
            m_pianoRollViewPort->nudgeSelectedNotes(-1, 0);

        break;
    }
    case KeyPressCommandIDs::nudgeNotesRight:
    {
        if (m_pianoRollViewPort != nullptr &&
            !m_pianoRollViewPort->nudgePendingPaste(1, 0))
            m_pianoRollViewPort->nudgeSelectedNotes(1, 0);

        break;
    }
    case KeyPressCommandIDs::nudgeNotesOctaveUp:
    {
        if (m_pianoRollViewPort != nullptr &&
            !m_pianoRollViewPort->nudgePendingPaste(0, 12))
            m_pianoRollViewPort->nudgeSelectedNotes(0, 12);

        break;
    }
    case KeyPressCommandIDs::nudgeNotesOctaveDown:
    {
        if (m_pianoRollViewPort != nullptr &&
            !m_pianoRollViewPort->nudgePendingPaste(0, -12))
            m_pianoRollViewPort->nudgeSelectedNotes(0, -12);

        break;
    }
    default:
        return false;
    }
    return true;
}

void PianoRollEditor::updateButtonColour()
{
    m_notePropertiesBar.updateColours();
    auto buttonColour = m_editViewState.m_applicationState.getButtonTextColour();
    GUIHelpers::setDrawableOnButton(m_selectionBtn, BinaryData::select_icon_svg, buttonColour);
    GUIHelpers::setDrawableOnButton(m_drawBtn, BinaryData::pencil_svg, buttonColour);
    GUIHelpers::setDrawableOnButton(m_rangeSelectBtn, BinaryData::select_timerange_svg, buttonColour);
    GUIHelpers::setDrawableOnButton(m_erasorBtn, BinaryData::rubber_svg, buttonColour);
    GUIHelpers::setDrawableOnButton(m_splitBtn, BinaryData::split_svg, buttonColour);
    GUIHelpers::setDrawableOnButton(m_lassoBtn, BinaryData::rubberband_svg, buttonColour);
}
void PianoRollEditor::buttonClicked(juce::Button *button)
{
    if (m_pianoRollViewPort == nullptr)
        return;

    if (button == &m_selectionBtn)
    {

        m_pianoRollViewPort->setTool(Tool::pointer);
    }
    if (button == &m_drawBtn)
    {
        m_pianoRollViewPort->setTool(Tool::draw);
    }
    if (button == &m_rangeSelectBtn)
    {
        m_pianoRollViewPort->setTool(Tool::range);
    }
    if (button == &m_erasorBtn)
    {
        m_pianoRollViewPort->setTool(Tool::eraser);
    }
    if (button == &m_splitBtn)
    {
        m_pianoRollViewPort->setTool(Tool::knife);
    }
    if (button == &m_lassoBtn)
    {
        m_pianoRollViewPort->setTool(Tool::lasso);
    }
}

void PianoRollEditor::setTrack(tracktion_engine::Track::Ptr track, bool forceRefresh)
{
    if (track == nullptr)
    {
        if (m_pianoRollViewPort != nullptr)
            clearTrack();
        return;
    }

    if (!forceRefresh && m_pianoRollViewPort != nullptr && m_pianoRollViewPort->getTrack() == track)
        return;

    if (m_pianoRollViewPort != nullptr)
        clearTrack();

    auto sanitizedID = "ID" + track->itemID.toString().removeCharacters("{}-");
    m_timeLine.setTimeLineID(sanitizedID);
    m_pianoRollViewPort = std::make_unique<MidiViewport>(m_editViewState, track, m_timeLine);
    m_pianoRollViewPort->setNoteUnderMouseHandler([this](std::optional<int> note)
    {
        m_NoteDescUnderCursor = note.has_value()
                                    ? juce::MidiMessage::getMidiNoteName(*note, true, true, 3)
                                    : juce::String{};
        repaint(getFooterRect());
    });
    addAndMakeVisible(*m_pianoRollViewPort);
    m_pianoRollViewPort->addChangeListener(this);
    m_horizontalScrollBar.setVisible(true);

    m_timelineOverlay = std::make_unique<TimelineOverlayComponent>(m_editViewState, track, m_timeLine);
    addAndMakeVisible(*m_timelineOverlay);

    m_velocityEditor = std::make_unique<VelocityEditor>(m_editViewState, track, m_timeLine.getTimeLineID());
    addAndMakeVisible(*m_velocityEditor);

    m_keyboard = std::make_unique<KeyboardView>(m_editViewState, m_timeLine.getTimeLineID());
    addAndMakeVisible(*m_keyboard);
    m_keyboard->setOnKeyClicked([this](int midiNoteNumber, bool addToSelection) { handleKeyboardKeyClick(midiNoteNumber, addToSelection); });
    resized();
}
void PianoRollEditor::clearTrack()
{
    m_pianoRollViewPort->finishPendingPasteOnDeselect();
    m_editViewState.m_selectionManager.deselect(&m_pianoRollViewPort->getSelectedEvents());
    m_timelineOverlay.reset(nullptr);
    m_pianoRollViewPort->removeChangeListener(this);
    m_pianoRollViewPort.reset(nullptr);
    m_NoteDescUnderCursor.clear();
    repaint(getFooterRect());
    m_velocityEditor.reset(nullptr);
    m_keyboard.reset(nullptr);
    m_horizontalScrollBar.setVisible(false);
    m_notePropertiesBar.clearSelection();
    resized();
}
void PianoRollEditor::valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property)
{
    if (treeWhosePropertyHasChanged.hasType(te::IDs::NOTE) || treeWhosePropertyHasChanged.hasType(m_timeLine.getTimeLineID()))
    {
        markAndUpdate(m_updateNoteEditor);
        markAndUpdate(m_updateVelocity);
        markAndUpdate(m_updateNoteProperties);
    }

    if (treeWhosePropertyHasChanged.hasType(m_timeLine.getTimeLineID()))
    {
        markAndUpdate(m_updateKeyboard);
        markAndUpdate(m_updateHorizontalScrollbar);
    }

    if (property == IDs::pianoRollSnapMode || property == IDs::pianoRollSnapDenominator)
        repaint(getFooterRect());

    if (treeWhosePropertyHasChanged.hasType(IDs::ThemeState))
    {
        markAndUpdate(m_updateButtonColour);
    }
}
void PianoRollEditor::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &property)
{
    if (te::Clip::isClipState(property))
        markAndUpdate(m_updateClips);
}
void PianoRollEditor::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &property, int)
{
    if (te::Clip::isClipState(property))
        markAndUpdate(m_updateClips);

    if (property.hasType(te::IDs::NOTE))
        markAndUpdate(m_updateNoteProperties);

    if (m_pianoRollViewPort != nullptr && property == m_pianoRollViewPort->getTrack()->state)
        markAndUpdate(m_updateTracks);
}
void PianoRollEditor::handleAsyncUpdate()
{
    if (compareAndReset(m_updateButtonColour))
    {
        updateButtonColour();
    }
    if (m_keyboard != nullptr && compareAndReset(m_updateKeyboard))
        m_keyboard->resized();

    if (m_pianoRollViewPort != nullptr && compareAndReset(m_updateNoteEditor))
    {
        m_pianoRollViewPort->refreshNoteUnderMouse();
        m_pianoRollViewPort->repaint();
        m_timeLine.repaint();
        repaint(getFooterRect());
    }

    if (m_velocityEditor != nullptr && compareAndReset(m_updateVelocity))
        m_velocityEditor->repaint();

    if (compareAndReset(m_updateNoteProperties))
        m_notePropertiesBar.refreshFromSelection();

    if (compareAndReset(m_updateHorizontalScrollbar))
        updateHorizontalScrollBar();

    if (m_pianoRollViewPort != nullptr && compareAndReset(m_updateClips))
        m_pianoRollViewPort->updateSelectedEvents();

    if (m_pianoRollViewPort != nullptr && compareAndReset(m_updateTracks))
        clearTrack();
}

void PianoRollEditor::handleKeyboardKeyClick(int midiNoteNumber, bool addToSelection)
{
    if (m_pianoRollViewPort == nullptr)
        return;

    m_pianoRollViewPort->finishPendingPasteOnDeselect();
    selectNotesOfKeyInCurrentClip(midiNoteNumber, addToSelection);
}

void PianoRollEditor::midiKeyStateChanged(te::AudioTrack *track, const juce::Array<int> &notesOn,
                                          const juce::Array<int> &, const juce::Array<int> &notesOff)
{
    if (m_keyboard != nullptr && m_pianoRollViewPort != nullptr &&
        m_pianoRollViewPort->getTrack().get() == track)
        m_keyboard->setMidiNotesDown(notesOn, notesOff);
}

juce::Array<te::MidiClip *> PianoRollEditor::getSelectedMidiClipsOnTrack() const
{
    juce::Array<te::MidiClip *> clips;
    if (m_pianoRollViewPort == nullptr)
        return clips;

    const auto track = m_pianoRollViewPort->getTrack();
    for (auto *clip : m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
    {
        if (auto *midiClip = dynamic_cast<te::MidiClip *>(clip))
        {
            if (track != nullptr && midiClip->getTrack() == track.get())
                clips.add(midiClip);
        }
    }

    return clips;
}

bool PianoRollEditor::hasSelectedNotesOfKey(const juce::Array<te::MidiClip *> &clips, int midiNoteNumber, te::SelectedMidiEvents &selectedEvents) const
{
    for (auto *clip : clips)
    {
        for (auto *note : clip->getSequence().getNotes())
        {
            if (note->getNoteNumber() != midiNoteNumber)
                continue;

            if (selectedEvents.isSelected(note))
                return true;
        }
    }

    return false;
}

void PianoRollEditor::removeSelectedNotesOfKey(const juce::Array<te::MidiClip *> &clips, int midiNoteNumber, te::SelectedMidiEvents &selectedEvents)
{
    for (auto *clip : clips)
    {
        for (auto *note : clip->getSequence().getNotes())
        {
            if (note->getNoteNumber() != midiNoteNumber)
                continue;

            if (selectedEvents.isSelected(note))
                selectedEvents.removeSelectedEvent(note);
        }
    }
}

std::pair<te::MidiClip *, te::MidiNote *> PianoRollEditor::selectNotesOfKey(const juce::Array<te::MidiClip *> &clips, int midiNoteNumber, bool addToSelection)
{
    bool add = addToSelection;
    te::MidiNote *firstSelectedNote = nullptr;
    te::MidiClip *firstSelectedClip = nullptr;

    for (auto *clip : clips)
    {
        for (auto *note : clip->getSequence().getNotes())
        {
            if (note->getNoteNumber() != midiNoteNumber)
                continue;

            if (firstSelectedNote == nullptr)
            {
                firstSelectedNote = note;
                firstSelectedClip = clip;
            }

            m_pianoRollViewPort->setNoteSelected(note, add);
            add = true;
        }
    }

    return {firstSelectedClip, firstSelectedNote};
}

void PianoRollEditor::selectNotesOfKeyInCurrentClip(int midiNoteNumber, bool addToSelection)
{
    if (m_pianoRollViewPort == nullptr)
        return;

    const auto clips = getSelectedMidiClipsOnTrack();
    if (clips.isEmpty())
        return;

    auto &selectedEvents = m_pianoRollViewPort->getSelectedEvents();

    // Shift-click toggles this pitch: if any are selected, remove them all.
    if (addToSelection && hasSelectedNotesOfKey(clips, midiNoteNumber, selectedEvents))
    {
        removeSelectedNotesOfKey(clips, midiNoteNumber, selectedEvents);
        m_pianoRollViewPort->setClickedNote(nullptr);
        m_pianoRollViewPort->repaint();
        return;
    }

    if (!addToSelection)
        m_pianoRollViewPort->unselectAll();

    const auto selection = selectNotesOfKey(clips, midiNoteNumber, addToSelection);
    if (selection.first != nullptr)
        m_pianoRollViewPort->setClickedClip(selection.first);
    m_pianoRollViewPort->setClickedNote(selection.second);
    m_pianoRollViewPort->repaint();
}
juce::Rectangle<int> PianoRollEditor::getHeaderRect()
{
    auto area = getLocalBounds();
    return area.removeFromTop(m_editViewState.m_timeLineHeight);
}
juce::Rectangle<int> PianoRollEditor::getToolBarRect()
{
    auto area = getLocalBounds();
    area = area.removeFromTop(m_editViewState.m_timeLineHeight);
    area.reduce(area.getWidth() / 3, 0);

    return area;
}
juce::Rectangle<int> PianoRollEditor::getNotePropertiesRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    return area.removeFromTop(30);
}

juce::Rectangle<int> PianoRollEditor::getTimeLineRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromTop(getNotePropertiesRect().getHeight());
    area.removeFromLeft(m_editViewState.m_keyboardWidth);
    return area.removeFromTop(m_editViewState.m_timeLineHeight);
}
juce::Rectangle<int> PianoRollEditor::getTimelineHelperRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromTop(getNotePropertiesRect().getHeight());
    area.removeFromRight(getWidth() - m_editViewState.m_keyboardWidth);

    return area.removeFromTop(m_editViewState.m_timeLineHeight);
}
juce::Rectangle<int> PianoRollEditor::getKeyboardRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromTop(getNotePropertiesRect().getHeight());
    area.removeFromTop(m_editViewState.m_timeLineHeight);
    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromBottom(getScrollbarThickness());
    area.removeFromBottom(getVelocityEditorRect().getHeight());

    return area.removeFromLeft(m_editViewState.m_keyboardWidth);
}
juce::Rectangle<int> PianoRollEditor::getMidiEditorRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromTop(getNotePropertiesRect().getHeight());
    area.removeFromTop(getTimeLineRect().getHeight());
    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromBottom(getScrollbarThickness());
    area.removeFromBottom(getVelocityEditorRect().getHeight());
    return area.removeFromRight(getWidth() - getKeyboardRect().getWidth());
}
juce::Rectangle<int> PianoRollEditor::getParameterToolbarRect()
{
    auto area = getLocalBounds();
    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromBottom(getScrollbarThickness());
    area.removeFromRight(getWidth() - m_editViewState.m_keyboardWidth);

    return area.removeFromBottom(getVelocityEditorRect().getHeight());
}
juce::Rectangle<int> PianoRollEditor::getVelocityEditorRect()
{
    auto area = getLocalBounds();

    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromBottom(getScrollbarThickness());
    area.removeFromLeft(m_editViewState.m_keyboardWidth);

    int height = getHeight() < 400 ? getHeight() < 300 ? 0 : getHeight() / 5 : m_editViewState.m_velocityEditorHeight;

    return area.removeFromBottom(height);
}

juce::Rectangle<int> PianoRollEditor::getHorizontalScrollbarRect()
{
    auto area = getLocalBounds();
    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromLeft(m_editViewState.m_keyboardWidth);
    return area.removeFromBottom(getScrollbarThickness());
}

juce::Rectangle<int> PianoRollEditor::getHorizontalScrollbarSpacerRect()
{
    auto area = getLocalBounds();
    area.removeFromBottom(getFooterRect().getHeight());
    return area.removeFromBottom(getScrollbarThickness()).removeFromLeft(m_editViewState.m_keyboardWidth);
}

juce::Rectangle<int> PianoRollEditor::getFooterRect()
{
    auto area = getLocalBounds();
    return area.removeFromBottom(20);
}
