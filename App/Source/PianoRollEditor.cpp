#include "PianoRollEditor.h"
#include "Utilities.h"

PianoRollEditor::PianoRollEditor(EditViewState & evs)
    : m_editViewState(evs)
    , m_keyboard (m_keybordstate, evs)
    , m_timeline (evs, evs.m_pianoX1, evs.m_pianoX2)
    , m_playhead (evs.m_edit, evs, evs.m_pianoX1, evs.m_pianoX2)
{
    m_keybordstate.addListener (this);
    evs.m_edit.state.addListener (this);
    m_keyboard.getKeyboard().setBlackNoteWidthProportion (0.5f);
    m_keyboard.getKeyboard().setBlackNoteLengthProportion (0.6f);
    m_keyboard.getKeyboard().setScrollButtonsVisible (false);

    addAndMakeVisible (m_keyboard);
    addAndMakeVisible (m_timeline);
    addAndMakeVisible (m_playhead);
    m_playhead.setAlwaysOnTop (true);
}
PianoRollEditor::~PianoRollEditor()
{
    m_editViewState.m_edit.state.removeListener (this);
    m_keybordstate.removeListener (this);
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

    m_keyboard.setBounds (keyboard);
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
void PianoRollEditor::handleNoteOn(juce::MidiKeyboardState *
                                      , int /*midiChannel*/
                                      , int midiNoteNumber
                                      , float velocity)
{
    auto& virtMidiInput = EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit.engine);
    virtMidiInput.handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, midiNoteNumber, velocity));
}
void PianoRollEditor::handleNoteOff(juce::MidiKeyboardState *
                                       , int /*midiChannel*/
                                       , int midiNoteNumbr
                                       , float)
{
    auto& virtMidiInput = EngineHelpers::getVirtuelMidiInputDevice(m_editViewState.m_edit.engine);
    virtMidiInput.handleIncomingMidiMessage(juce::MidiMessage::noteOff(0, midiNoteNumbr, 0.8f));
}
void PianoRollEditor::setTrack(tracktion_engine::Track::Ptr track)
{
    m_pianoRollViewPort = std::make_unique<MidiViewport> (m_editViewState, track);
    addAndMakeVisible (*m_pianoRollViewPort);

    m_timelineOverlay = std::make_unique<TimelineOverlayComponent> (m_editViewState, track, m_timeline);
    addAndMakeVisible (*m_timelineOverlay);

    m_velocityEditor = std::make_unique<VelocityEditor>(m_editViewState, track);
    addAndMakeVisible(*m_velocityEditor);

    resized ();
}
void PianoRollEditor::clearTrack()
{
    GUIHelpers::log("track cleared");
    m_timelineOverlay.reset (nullptr);
    m_pianoRollViewPort.reset (nullptr);
    m_velocityEditor.reset(nullptr);
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
    if (compareAndReset(m_updateKeyboard))
        m_keyboard.resized();

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
