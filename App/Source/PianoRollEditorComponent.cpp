#include "PianoRollEditorComponent.h"

PianoRollEditorComponent::PianoRollEditorComponent(EditViewState & evs)
    : m_editViewState(evs)
    , m_keyboard (m_keybordstate
                  , juce::MidiKeyboardComponent::
                        Orientation::verticalKeyboardFacingRight)
    , m_timeline (evs, evs.m_pianoX1, evs.m_pianoX2)
    , m_playhead (evs.m_edit, evs, evs.m_pianoX1, evs.m_pianoX2)
{
    m_keybordstate.addListener (this);
    evs.m_edit.state.addListener (this);
    m_keyboard.setBlackNoteWidthProportion (0.5);
    m_keyboard.setBlackNoteLengthProportion (0.6);
    m_keyboard.setScrollButtonsVisible (false);

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

void PianoRollEditorComponent::paintOverChildren(juce::Graphics &g)
{
    //cover left from timeline
    g.setColour (juce::Colour(0xff181818));
    g.fillRect (0, 0
                , m_keyboard.getWidth ()
                , m_timeline.getHeight ());
    g.setColour (juce::Colour(0xff444444));
    g.fillRect (m_keyboard.getWidth ()- 1
                , 0, 1
                , m_timeline.getHeight ());
    //Footer
//    g.setColour(juce::Colour(0xff181818));
//    g.fillRect (0, getHeight() - 20, getWidth (), 20);
//
//    g.setColour (juce::Colour(0xff555555));
//    g.fillRect (m_keyboard.getWidth () - 1
//              , 0
//              , 1
//              , m_editViewState.m_timeLineHeight);

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
}

void PianoRollEditorComponent::resized()
{
    auto area = getLocalBounds ();
    auto timeline = area.removeFromTop (m_editViewState.m_timeLineHeight);
    auto keyboard = area.removeFromLeft (50);
    timeline.removeFromLeft (keyboard.getWidth ());
    auto playhead = area.withTrimmedTop ( - m_editViewState.m_timeLineHeight);
    double firstVisibleNote = m_editViewState.m_pianoStartNoteBottom;
    double pianoRollNoteWidth = m_editViewState.m_pianorollNoteWidth;

    m_keyboard.setKeyWidth (juce::jmax(0.1f, (float) pianoRollNoteWidth * 12 / 7));
    m_keyboard.setBounds (0
                        , getHeight () - (int) m_keyboard.getTotalKeyboardWidth ()
                          + (int) (firstVisibleNote * pianoRollNoteWidth)
                        , keyboard.getWidth ()
                        , (int) m_keyboard.getTotalKeyboardWidth ());

    m_editViewState.m_pianoStartNoteBottom =
            juce::jlimit(0.0
                       , 127.0
                         - (getHeight () / m_editViewState.m_pianorollNoteWidth)
                       , (double) m_editViewState.m_pianoStartNoteBottom);

    m_timeline.setBounds (timeline);
    if (m_timelineOverlay)
    {
        m_timelineOverlay->setBounds (timeline);
    }

    if (m_pianoRollContentComponent)
    {
        m_pianoRollContentComponent->setBounds (area);
    }

    m_playhead.setBounds (playhead);

}

void PianoRollEditorComponent::mouseMove(const juce::MouseEvent &event)
{
    m_NoteDescUnderCursor = juce::MidiMessage::getMidiNoteName (
                m_pianoRollContentComponent->getNoteNumber(event.y)
                , true
                , true
                , 3);
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
    resized ();
}

void PianoRollEditorComponent::clearPianoRollClip()
{
    m_timelineOverlay.reset (nullptr);
    m_pianoRollContentComponent.reset (nullptr);
    resized ();
}

void PianoRollEditorComponent::valueTreePropertyChanged(
        juce::ValueTree &treeWhosePropertyHasChanged
      , const juce::Identifier &property)
{
    if (treeWhosePropertyHasChanged.hasType (IDs::EDITVIEWSTATE))
    {
        if (property == IDs::pianoY1
        ||  property == IDs::pianorollNoteWidth) resized ();
    }
}
