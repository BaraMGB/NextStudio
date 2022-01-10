#include "PianoRollEditorComponent.h"

PianoRollEditorComponent::PianoRollEditorComponent(EditViewState & evs)
    : m_editViewState(evs)
    , m_keyboard (m_keybordstate, evs)
    , m_timeline (evs, evs.m_pianoX1, evs.m_pianoX2)
    , m_playhead (evs.m_edit, evs, evs.m_pianoX1, evs.m_pianoX2)
{
    m_keybordstate.addListener (this);
    evs.m_edit.state.addListener (this);
    m_keyboard.getKeyboard().setBlackNoteWidthProportion (0.5);
    m_keyboard.getKeyboard().setBlackNoteLengthProportion (0.6);
    m_keyboard.getKeyboard().setScrollButtonsVisible (false);

    addAndMakeVisible (m_keyboard);
    addAndMakeVisible (m_timeline);
    addAndMakeVisible (m_playhead);
    m_timeline.setAlwaysOnTop (true);
    m_playhead.setAlwaysOnTop (true);
}
PianoRollEditorComponent::~PianoRollEditorComponent()
{
    m_editViewState.m_edit.state.removeListener (this);
    m_keybordstate.removeListener (this);
}
void PianoRollEditorComponent::paint(juce::Graphics& g)
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
}
void PianoRollEditorComponent::paintOverChildren(juce::Graphics &g)
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
void PianoRollEditorComponent::resized()
{

    auto keyboard = getKeyboardRect();
    auto timeline = getTimeLineRect();
    auto playhead = getPlayHeadRect();

    m_keyboard.setBounds (keyboard);
    m_timeline.setBounds (timeline);

    if (m_timelineOverlay)
    {
        m_timelineOverlay->setBounds (timeline);
    }

    if (m_velocityEditor)
    {
        m_velocityEditor->setBounds(getVelocityEditorRect());
    }

    if (m_pianoRollContentComponent)
    {
        m_pianoRollContentComponent->setBounds (getMidiEditorRect());
    }

    m_playhead.setBounds (playhead);

}
juce::Rectangle<int> PianoRollEditorComponent::getPlayHeadRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromLeft(m_editViewState.m_keyboardWidth);
    area.removeFromBottom(getFooterRect().getHeight());
    return area;
}
void PianoRollEditorComponent::mouseMove(const juce::MouseEvent &event)
{
    m_NoteDescUnderCursor = juce::MidiMessage::getMidiNoteName (
                (int) m_pianoRollContentComponent->getNoteNumber(event.y)
                , true
                , true
                , 3);
    repaint();
}
void PianoRollEditorComponent::handleNoteOn(juce::MidiKeyboardState *
                                      , int /*midiChannel*/
                                      , int midiNoteNumber
                                      , float velocity)
{
    if (m_pianoRollContentComponent)
    {
        if (auto mc = m_pianoRollContentComponent->getMidiClipsOfTrack ().at (0))
        {
            auto ch = mc->getMidiChannel ();
            mc->getAudioTrack ()->playGuideNote
                              (midiNoteNumber
                             , ch
                             , (int) (127 * velocity)
                             , false
                             , true);
        }
    }
}
void PianoRollEditorComponent::handleNoteOff(juce::MidiKeyboardState *
                                       , int /*midiChannel*/
                                       , int /*midiNoteNumber*/
                                       , float)
{
    if (m_pianoRollContentComponent)
    {
        if (auto mc = m_pianoRollContentComponent->getMidiClipsOfTrack ().at (0))
        {
            auto ch = mc->getMidiChannel ();
            mc->getAudioTrack ()->turnOffGuideNotes (ch);
        }
    }
}
void PianoRollEditorComponent::setTrack(const tracktion_engine::Track::Ptr& track)
{
    m_pianoRollContentComponent = std::make_unique<PianoRollContentComponent>(m_editViewState, track);
    addAndMakeVisible (*m_pianoRollContentComponent);

    m_timelineOverlay = std::make_unique<TimelineOverlayComponent>
            (m_editViewState
           , m_pianoRollContentComponent->getTrack ()
           , m_timeline);
    m_timelineOverlay->setAlwaysOnTop (true);
    addAndMakeVisible (*m_timelineOverlay);

    m_velocityEditor = std::make_unique<VelocityEditor>(
        m_editViewState, m_pianoRollContentComponent->getTrack());
    addAndMakeVisible(*m_velocityEditor);

    resized ();
}
void PianoRollEditorComponent::clearTrack()
{
    m_timelineOverlay.reset (nullptr);
    m_pianoRollContentComponent.reset (nullptr);
    m_velocityEditor.reset(nullptr);
    resized ();
}
void PianoRollEditorComponent::valueTreePropertyChanged(
        juce::ValueTree &treeWhosePropertyHasChanged
      , const juce::Identifier &property)
{
    if (treeWhosePropertyHasChanged.hasType(te::IDs::NOTE))
    {
        if (property == te::IDs::v)
        {
            markAndUpdate(m_updateNoteEditor);
        }
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
void PianoRollEditorComponent::handleAsyncUpdate()
{
    if (m_updateKeyboard)
    {
        m_keyboard.resized();
    }

    if (m_updateNoteEditor)
    {
        m_pianoRollContentComponent->repaint();
    }

    if (m_updateVelocity)
    {
        m_velocityEditor->repaint();
    }
}
juce::Rectangle<int> PianoRollEditorComponent::getHeaderRect()
{
    auto area = getLocalBounds();
    return area.removeFromTop(m_editViewState.m_timeLineHeight);
}
juce::Rectangle<int> PianoRollEditorComponent::getTimeLineRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromLeft(m_editViewState.m_keyboardWidth);
    return area.removeFromTop(m_editViewState.m_timeLineHeight);
}
juce::Rectangle<int> PianoRollEditorComponent::getTimelineHelperRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromRight(getWidth() - m_editViewState.m_keyboardWidth);

    return area.removeFromTop(m_editViewState.m_timeLineHeight);
}
juce::Rectangle<int> PianoRollEditorComponent::getKeyboardRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromTop(m_editViewState.m_timeLineHeight);
    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromBottom(getVelocityEditorRect().getHeight());

    return area.removeFromLeft(m_editViewState.m_keyboardWidth);
}
juce::Rectangle<int> PianoRollEditorComponent::getMidiEditorRect()
{
    auto area = getLocalBounds();
    area.removeFromTop(getHeaderRect().getHeight());
    area.removeFromTop(getTimeLineRect().getHeight());
    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromBottom(getVelocityEditorRect().getHeight());
    return area.removeFromRight(getWidth() - getKeyboardRect().getWidth());
}
juce::Rectangle<int> PianoRollEditorComponent::getParameterToolbarRect()
{
    auto area = getLocalBounds();
    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromRight(getWidth() - m_editViewState.m_keyboardWidth);

    return area.removeFromBottom(m_editViewState.m_velocityEditorHeight);
}
juce::Rectangle<int> PianoRollEditorComponent::getVelocityEditorRect()
{
    auto area = getLocalBounds();

    area.removeFromBottom(getFooterRect().getHeight());
    area.removeFromLeft(m_editViewState.m_keyboardWidth);

    return area.removeFromBottom(m_editViewState.m_velocityEditorHeight);
}
juce::Rectangle<int> PianoRollEditorComponent::getFooterRect()
{
    auto area = getLocalBounds();
    return area.removeFromBottom(20);
}
