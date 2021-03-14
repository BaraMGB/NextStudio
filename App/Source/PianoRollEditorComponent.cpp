#include "PianoRollEditorComponent.h"

PianoRollComponent::PianoRollComponent(EditViewState & evs)
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
    m_playhead.setAlwaysOnTop (true);
}

PianoRollComponent::~PianoRollComponent()
{
    m_editViewState.m_edit.state.removeListener (this);
    m_keybordstate.removeListener (this);
}

void PianoRollComponent::focusLost(juce::Component::FocusChangeType cause)
{
}

void PianoRollComponent::focusGained(juce::Component::FocusChangeType cause)
{
}

void PianoRollComponent::resized()
{
    auto area = getLocalBounds ();
    auto keyboard = area.removeFromLeft (50);
    auto timeline = area.removeFromTop (50);

    double firstVisibleNote = m_editViewState.m_pianoY1;
    double pianoRollNoteWidth = m_editViewState.m_pianorollNoteWidth;
    m_keyboard.setKeyWidth (juce::jmax(0.1, pianoRollNoteWidth * 12 / 7));
    m_keyboard.setBounds (0
                          , getHeight () - m_keyboard.getTotalKeyboardWidth ()
                            + (firstVisibleNote * pianoRollNoteWidth)
                          , keyboard.getWidth ()
                          , m_keyboard.getTotalKeyboardWidth ());
    m_editViewState.m_pianoY1 =
            juce::jlimit(0
                       , 127 - (int) (getHeight ()
                           / m_editViewState.m_pianorollNoteWidth)
                       , (int) m_editViewState.m_pianoY1);
    m_timeline.setBounds (timeline);
    if (m_pianoRollContentComponent != nullptr)
    {
        m_pianoRollContentComponent->setBounds (area);
        m_pianoRollContentComponent->setKeyWidth (m_keyboard.getKeyWidth ());
    }

    m_playhead.setBounds (area.getUnion (timeline));

}



void PianoRollComponent::handleNoteOn(juce::MidiKeyboardState *
                                      , int /*midiChannel*/
                                      , int midiNoteNumber
                                      , float v)
{
    if (m_pianoRollContentComponent)
    {
        if (auto mc = m_pianoRollContentComponent->getMidiClipsOfTrack ().at (0))
        {
            auto midichannel = mc->getMidiChannel ();
            mc->getAudioTrack ()->playGuideNote
                              (midiNoteNumber,midichannel, 127.0 * v, false, true);


        }

    }

}

void PianoRollComponent::handleNoteOff(juce::MidiKeyboardState *
                                       , int /*midiChannel*/
                                       , int /*midiNoteNumber*/
                                       , float)
{
    if (m_pianoRollContentComponent)
    {
        if (auto mc = m_pianoRollContentComponent->getMidiClipsOfTrack ().at (0))
        {
            auto midichannel = mc->getMidiChannel ();
            mc->getAudioTrack ()->turnOffGuideNotes (midichannel);
        }
    }
}

void PianoRollComponent::setPianoRollClip(std::unique_ptr<PianoRollContentComponent> pianoRollContentComponent)
{
    addAndMakeVisible (*pianoRollContentComponent);


    m_pianoRollContentComponent = std::move (pianoRollContentComponent);
    resized ();
}

void PianoRollComponent::clearPianoRollClip()
{
    m_pianoRollContentComponent.reset (nullptr);
    resized ();
}

void PianoRollComponent::valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property)
{
    if (treeWhosePropertyHasChanged.hasType (IDs::EDITVIEWSTATE))
    {
        if (property == IDs::pianoY1
        ||  property == IDs::pianoY2
        ||  property == IDs::pianorollNoteWidth)
        {
            resized ();
        }
    }
}
