#include "PianoRollEditorComponent.h"

void KeyboardView::mouseDown(const juce::MouseEvent& e)
{
    m_pianoStartNoteCached = m_editViewState.m_pianoStartNoteBottom;
    m_pianoRollKeyWidthCached = m_editViewState.m_pianorollNoteWidth;
    m_clickedNote = getNoteNum(e.y);
}

void KeyboardView::mouseDrag(const juce::MouseEvent& e)
{
    auto unitDistance = 50.0f;
    float scaleFactor
        = std::powf (2,(float) e.getDistanceFromDragStartX() / unitDistance);

    auto visibleNotes = (float) (getHeight() / m_pianoRollKeyWidthCached);

    float scaledVisibleNotes = juce::jlimit(0.f , 127.f
                                      , visibleNotes * scaleFactor );

    m_editViewState.m_pianoStartNoteBottom = juce::jmax(0.f,(float) m_clickedNote - ((scaledVisibleNotes * ((float)getHeight() - e.position.y)) / (float) getHeight()));
    float maxKeyHeight = (float) getHeight() / (float)(127.f - m_editViewState.m_pianoStartNoteBottom);
    m_editViewState.m_pianorollNoteWidth = juce::jmax((float)getHeight() / scaledVisibleNotes, maxKeyHeight);
}
//-----------------------------------------------------------------------------------

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
}

void PianoRollEditorComponent::resized()
{
    auto area = getLocalBounds ();
    area.removeFromTop(m_editViewState.m_timeLineHeight);//Header

    area.removeFromBottom(20);//footer

    auto keyboard = area.removeFromLeft(150);
    keyboard.removeFromTop(m_editViewState.m_timeLineHeight);
    auto timeline = area.removeFromTop (m_editViewState.m_timeLineHeight);

    keyboard.removeFromTop(1);

    auto playhead = area.withTrimmedTop ( - m_editViewState.m_timeLineHeight);

    m_keyboard.setBounds (keyboard);

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
        ||  property == IDs::pianorollNoteWidth)
        {
            m_keyboard.resized();
//            m_pianoRollContentComponent->repaint();
            //resized ();
        }
    }
}
void PianoRollEditorComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour(juce::Colour(0xff181818));
    g.fillRect(area.removeFromTop(m_editViewState.m_timeLineHeight));

    g.setColour(juce::Colours::white);
    g.fillRect(area.removeFromTop(1));
    auto footer = area.removeFromBottom(20);
    g.setColour(juce::Colour(0xff181818));
    g.fillRect(footer);

    g.setColour(juce::Colour(0xff272727));
    g.fillRect(area);

    g.setColour(juce::Colours::white);
    area.removeFromTop(m_editViewState.m_timeLineHeight);
    g.fillRect(area.removeFromTop(1));

    g.fillRect(area.removeFromBottom(1));
}
