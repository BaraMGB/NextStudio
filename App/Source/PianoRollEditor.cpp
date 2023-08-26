
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

#include "PianoRollEditor.h"
#include "KeyboardView.h"
#include "Utilities.h"

PianoRollEditor::PianoRollEditor(EditViewState & evs)
    : m_editViewState(evs)
    , m_toolBar(Alignment::Center)
    , m_timeline (evs, evs.m_pianoX1, evs.m_pianoX2)
    , m_playhead (evs.m_edit, evs, evs.m_pianoX1, evs.m_pianoX2)
    , m_selectionBtn ("select mode", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_drawBtn ("draw mode", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_rangeSelectBtn ("range select mode", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_erasorBtn ("erasor mode", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_splitBtn ("split mode", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
{
    setWantsKeyboardFocus(true);
    evs.m_edit.state.addListener (this);

    addAndMakeVisible (m_timeline);
    addAndMakeVisible (m_playhead);
    m_playhead.setAlwaysOnTop (true);

    GUIHelpers::setDrawableOnButton(m_selectionBtn, BinaryData::select_icon_svg, juce::Colour(0xffffffff));
    m_selectionBtn.setName("select");
    m_selectionBtn.setTooltip(GUIHelpers::translate("select clips or automation points", m_editViewState.m_applicationState));
    m_selectionBtn.addListener(this);

    GUIHelpers::setDrawableOnButton(m_drawBtn, BinaryData::pencil_svg, juce::Colour(0xffffffff));
    m_drawBtn.setName("draw");
    m_drawBtn.setTooltip(GUIHelpers::translate("draw mode", m_editViewState.m_applicationState));
    m_drawBtn.addListener(this);

    GUIHelpers::setDrawableOnButton(m_rangeSelectBtn, BinaryData::select_timerange_svg, juce::Colour(0xffffffff));
    m_rangeSelectBtn.setName("range select");
    m_rangeSelectBtn.setTooltip(GUIHelpers::translate("range select mode", m_editViewState.m_applicationState));
    m_rangeSelectBtn.addListener(this);

    GUIHelpers::setDrawableOnButton(m_erasorBtn, BinaryData::rubber_svg, juce::Colour(0xffffffff));
    m_erasorBtn.setName("erasor");
    m_erasorBtn.setTooltip(GUIHelpers::translate("erasor mode", m_editViewState.m_applicationState));
    m_erasorBtn.addListener(this);

    GUIHelpers::setDrawableOnButton(m_splitBtn, BinaryData::split_svg, juce::Colour(0xffffffff));
    m_splitBtn.setName("split");
    m_splitBtn.setTooltip(GUIHelpers::translate("split clips", m_editViewState.m_applicationState));
    m_splitBtn.addListener(this);

    addAndMakeVisible(m_toolBar);
    m_toolBar.addButton(&m_selectionBtn, 1);
    m_toolBar.addButton(&m_drawBtn, 1);
    m_toolBar.addButton(&m_rangeSelectBtn, 1);
    m_toolBar.addButton(&m_erasorBtn, 1);
    m_toolBar.addButton(&m_splitBtn, 1);
}
PianoRollEditor::~PianoRollEditor()
{
    m_splitBtn.removeListener(this);
    m_erasorBtn.removeListener(this);
    m_rangeSelectBtn.removeListener(this);
    m_drawBtn.removeListener(this);
    m_selectionBtn.removeListener(this);
    m_editViewState.m_edit.state.removeListener (this);
}
void PianoRollEditor::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::pink);
    g.fillAll();

    g.setColour(juce::Colour(0xff181818));
    g.fillRect(getHeaderRect());

    g.setColour(juce::Colour(0xff181818));
    g.fillRect(getFooterRect());

    g.setColour(juce::Colour(0xff272727));
    g.fillRect(getTimeLineRect());
    g.fillRect(getTimelineHelperRect());
    g.fillRect(getKeyboardRect());
    g.fillRect(getMidiEditorRect());
    g.fillRect(getVelocityEditorRect());
    g.fillRect(getParameterToolbarRect());

    g.setColour(juce::Colours::white);
    if (m_pianoRollViewPort == nullptr)
        g.drawText("select a clip for edit midi", getMidiEditorRect(), juce::Justification::centred);
}
void PianoRollEditor::paintOverChildren(juce::Graphics &g)
{
    g.setColour (juce::Colour(0xffffffff));
    const auto snapType = m_timeline.getBestSnapType ();
    const auto snapTypeDesc = m_timeline.getEditViewState ()
                                .getSnapTypeDescription (snapType.level);
    g.drawText (snapTypeDesc
              , getWidth () - 100
              , getHeight () -20
              , 90
              , 20
              , juce::Justification::centredRight);

    g.drawText (m_NoteDescUnderCursor
              , getWidth () - 200
              , getHeight () - 20
              , 90
              , 20
              , juce::Justification::centredLeft);

    g.setColour(juce::Colours::white);
    g.fillRect(getHeaderRect().removeFromBottom(1));
    g.fillRect(getTimeLineRect().removeFromBottom(1));
    g.fillRect(getTimelineHelperRect().removeFromBottom(1));
    g.fillRect(getTimelineHelperRect().removeFromRight(1));
    g.fillRect(getVelocityEditorRect().removeFromTop(1));
    g.fillRect(getParameterToolbarRect().removeFromTop(1));
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

    if (m_keyboard != nullptr)
        m_keyboard->setBounds (keyboard);
    m_timeline.setBounds (timeline);

    if (m_timelineOverlay != nullptr)
        m_timelineOverlay->setBounds (getTimeLineRect().getUnion(getMidiEditorRect()));

    if (m_velocityEditor != nullptr)
        m_velocityEditor->setBounds(getVelocityEditorRect());

    if (m_pianoRollViewPort != nullptr)
        m_pianoRollViewPort->setBounds (getMidiEditorRect());

    m_playhead.setBounds (playhead);
}
juce::Rectangle<int> PianoRollEditor::getPlayHeadRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromLeft(m_editViewState.m_keyboardWidth);
    area.removeFromBottom(getFooterRect().getHeight());
    return area;
}
void PianoRollEditor::mouseMove(const juce::MouseEvent &event)
{
    if (m_pianoRollViewPort)
    {
        m_NoteDescUnderCursor = juce::MidiMessage::getMidiNoteName (
            (int) m_pianoRollViewPort->getNoteNumber(event.y)
            , true
            , true
            , 3);
        repaint();
    }
}


void PianoRollEditor::getAllCommands (juce::Array<juce::CommandID>& commands) 
{

    juce::Array<juce::CommandID> ids {

            KeyPressCommandIDs::deleteSelectedNotes,
            KeyPressCommandIDs::duplicateSelectedNotes,
            KeyPressCommandIDs::nudgeNotesUp,
            KeyPressCommandIDs::nudgeNotesDown,
            KeyPressCommandIDs::nudgeNotesLeft,
            KeyPressCommandIDs::nudgeNotesRight,
            KeyPressCommandIDs::nudgeNotesOctaveUp,
            KeyPressCommandIDs::nudgeNotesOctaveDown,
        };

    commands.addArray(ids);
}


void PianoRollEditor::getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result) 
{

    switch (commandID)
    { 
        case KeyPressCommandIDs::deleteSelectedNotes :
            result.setInfo("delete selected notes", "delete selected", "Notes", 0);
            result.addDefaultKeypress(juce::KeyPress::backspaceKey , 0);
            result.addDefaultKeypress(juce::KeyPress::deleteKey, 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("x").getKeyCode(), juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::duplicateSelectedNotes:
            result.setInfo("duplicate selected Notes", "duplicate selected Notes", "Notes", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("d").getKeyCode(), juce::ModifierKeys::commandModifier);
            break;

        case KeyPressCommandIDs::nudgeNotesUp :

            result.setInfo("move selected Notes up", "move selected notes up", "Notes", 0);
            result.addDefaultKeypress(juce::KeyPress::upKey, 0);
            break;
        case KeyPressCommandIDs::nudgeNotesDown:
            result.setInfo("move selected Notes down", "move selected notes down", "Notes", 0);
            result.addDefaultKeypress(juce::KeyPress::downKey ,0);
            break;
        case KeyPressCommandIDs::nudgeNotesLeft :
            result.setInfo("move selected Notes left", "move selected notes left ", "Notes", 0);
            result.addDefaultKeypress(juce::KeyPress::leftKey ,0);
            break;
        case KeyPressCommandIDs::nudgeNotesRight :
            result.setInfo("move selected Notes right", "move selected notes right", "Notes", 0);
            result.addDefaultKeypress(juce::KeyPress::rightKey ,0);
            break;
        case KeyPressCommandIDs::nudgeNotesOctaveUp :
            result.setInfo("move selected Notes ocatave up", "move selected notes octave up", "Notes", 0);
            result.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::nudgeNotesOctaveDown :
            result.setInfo("move selected Notes octave down", "move selected notes octave down", "Notes", 0);
            result.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::commandModifier);
            break;

        default:
            break;
        }

}

bool PianoRollEditor::perform (const juce::ApplicationCommandTarget::InvocationInfo& info) 
{

    switch (info.commandID)
    { 
        case KeyPressCommandIDs::deleteSelectedNotes:
        {
            if (m_pianoRollViewPort != nullptr)
                m_pianoRollViewPort->deleteSelectedNotes();

            break;
        }
        case KeyPressCommandIDs::duplicateSelectedNotes :
        {   
            if (m_pianoRollViewPort != nullptr)
                m_pianoRollViewPort->duplicateSelectedNotes();

            break;
        }
        case KeyPressCommandIDs::nudgeNotesUp :
        {   
            if (m_pianoRollViewPort != nullptr)
                m_pianoRollViewPort->getSelectedEvents().nudge(m_pianoRollViewPort->getBestSnapType(), 0, 1);

            break;
        }
        case KeyPressCommandIDs::nudgeNotesDown :
        {   
            if (m_pianoRollViewPort != nullptr)
                m_pianoRollViewPort->getSelectedEvents().nudge(m_pianoRollViewPort->getBestSnapType(), 0, -1);
            break;
        }
        case KeyPressCommandIDs::nudgeNotesLeft :
        {   
            if (m_pianoRollViewPort != nullptr)
                m_pianoRollViewPort->getSelectedEvents().nudge(m_pianoRollViewPort->getBestSnapType(), -1, 0);

            break;
        }
        case KeyPressCommandIDs::nudgeNotesRight :
        {   
            if (m_pianoRollViewPort != nullptr)
                m_pianoRollViewPort->getSelectedEvents().nudge(m_pianoRollViewPort->getBestSnapType(), 1, 0);

            break;
        }
        case KeyPressCommandIDs::nudgeNotesOctaveUp :
        {   
            if (m_pianoRollViewPort != nullptr)
                m_pianoRollViewPort->getSelectedEvents().nudge(m_pianoRollViewPort->getBestSnapType(),0, 12);

            break;
        }
        case KeyPressCommandIDs::nudgeNotesOctaveDown :
        {   
            if (m_pianoRollViewPort != nullptr)
                m_pianoRollViewPort->getSelectedEvents().nudge(m_pianoRollViewPort->getBestSnapType(), 0, -12);

            break;
        }
        default:
            return false;
    }
    return true;
}

void PianoRollEditor::buttonClicked(juce::Button* button) 
{
    if (m_pianoRollViewPort == nullptr)
        return;

    if (button == &m_selectionBtn)
    {

        m_pianoRollViewPort->setToolMode(Tool::pointer);
    }
    if (button == &m_drawBtn)
    {
        m_pianoRollViewPort->setToolMode(Tool::draw);
    }
    if (button == &m_rangeSelectBtn)
    {
        m_pianoRollViewPort->setToolMode(Tool::range);
    }
    if (button == &m_erasorBtn)
    {
        m_pianoRollViewPort->setToolMode(Tool::eraser);
    }
    if (button == &m_splitBtn)
    {
        m_pianoRollViewPort->setToolMode(Tool::knife);
    }
    
}

void PianoRollEditor::setTrack(tracktion_engine::Track::Ptr track)
{
    m_pianoRollViewPort = std::make_unique<MidiViewport> (m_editViewState, track);
    addAndMakeVisible (*m_pianoRollViewPort);

    m_timelineOverlay = std::make_unique<TimelineOverlayComponent> (m_editViewState, track, m_timeline);
    addAndMakeVisible (*m_timelineOverlay);

    m_velocityEditor = std::make_unique<VelocityEditor>(m_editViewState, track);
    addAndMakeVisible(*m_velocityEditor);

    m_keyboard = std::make_unique<KeyboardView>(m_editViewState, track);
    addAndMakeVisible (*m_keyboard);
    resized ();
}
void PianoRollEditor::clearTrack()
{
    GUIHelpers::log("track cleared");
    m_timelineOverlay.reset (nullptr);
    m_pianoRollViewPort.reset (nullptr);
    m_velocityEditor.reset(nullptr);
    m_keyboard.reset(nullptr);
    resized ();
}
void PianoRollEditor::valueTreePropertyChanged(
        juce::ValueTree &treeWhosePropertyHasChanged
      , const juce::Identifier &property)
{
    if (treeWhosePropertyHasChanged.hasType(te::IDs::NOTE))
    {
        markAndUpdate(m_updateNoteEditor);
        markAndUpdate(m_updateVelocity);
    }

    if (treeWhosePropertyHasChanged.hasType (IDs::EDITVIEWSTATE))
    {
        if (property == IDs::pianoY1
        ||  property == IDs::pianorollNoteWidth)
        {
            markAndUpdate(m_updateKeyboard);
        }
    }
}
void PianoRollEditor::valueTreeChildAdded(juce::ValueTree&,
                                                   juce::ValueTree& property)
{
    if (te::Clip::isClipState (property))
        markAndUpdate (m_updateClips);
}
void PianoRollEditor::valueTreeChildRemoved(juce::ValueTree& ,
                                                     juce::ValueTree& property,
                                                     int)
{
    if (te::Clip::isClipState (property))
        markAndUpdate (m_updateClips);

    if (m_pianoRollViewPort != nullptr
        && property == m_pianoRollViewPort->getTrack()->state)
            markAndUpdate(m_updateTracks);

}
void PianoRollEditor::handleAsyncUpdate()
{
    if (m_keyboard != nullptr && compareAndReset(m_updateKeyboard))
        m_keyboard->resized();

    if (m_pianoRollViewPort != nullptr && compareAndReset(m_updateNoteEditor))
        m_pianoRollViewPort->repaint();

    if (m_pianoRollViewPort != nullptr && compareAndReset(m_updateVelocity))
        m_velocityEditor->repaint();

    if (m_pianoRollViewPort != nullptr && compareAndReset(m_updateClips))
        m_pianoRollViewPort->updateSelectedEvents();

    if (m_pianoRollViewPort != nullptr && compareAndReset(m_updateTracks))
        clearTrack();
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
    area.reduce(area.getWidth() / 3, 0) ;

    return area;
}
juce::Rectangle<int> PianoRollEditor::getTimeLineRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromLeft(m_editViewState.m_keyboardWidth);
    return area.removeFromTop(m_editViewState.m_timeLineHeight);
}
juce::Rectangle<int> PianoRollEditor::getTimelineHelperRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromRight(getWidth() - m_editViewState.m_keyboardWidth);

    return area.removeFromTop(m_editViewState.m_timeLineHeight);
}
juce::Rectangle<int> PianoRollEditor::getKeyboardRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromTop(m_editViewState.m_timeLineHeight);
    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromBottom(getVelocityEditorRect().getHeight());

    return area.removeFromLeft(m_editViewState.m_keyboardWidth);
}
juce::Rectangle<int> PianoRollEditor::getMidiEditorRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromTop(getTimeLineRect().getHeight());
    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromBottom(getVelocityEditorRect().getHeight());
    return area.removeFromRight(getWidth() - getKeyboardRect().getWidth());
}
juce::Rectangle<int> PianoRollEditor::getParameterToolbarRect()
{
    auto area = getLocalBounds();
    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromRight(getWidth() - m_editViewState.m_keyboardWidth);

    return area.removeFromBottom(getVelocityEditorRect().getHeight());
}
juce::Rectangle<int> PianoRollEditor::getVelocityEditorRect()
{
    auto area = getLocalBounds();

    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromLeft(m_editViewState.m_keyboardWidth);

    int height = getHeight() < 400
        ? getHeight() < 300 ? 0 : getHeight() / 5
        : m_editViewState.m_velocityEditorHeight;

    return area.removeFromBottom(height);
}
juce::Rectangle<int> PianoRollEditor::getFooterRect()
{
    auto area = getLocalBounds();
    return area.removeFromBottom(20);
}
