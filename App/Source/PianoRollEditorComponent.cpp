#include "PianoRollEditorComponent.h"


TimelineOverlayComponent::TimelineOverlayComponent(EditViewState &evs) : m_editViewState (evs)
{
    //setInterceptsMouseClicks (false, true);
}

void TimelineOverlayComponent::paint(juce::Graphics &g)
{
    auto loopRange = m_editViewState.m_edit.getTransport ().getLoopRange ();
    auto loopStartX = timeToX (loopRange.getStart ());
    auto loopEndX = timeToX (loopRange.getEnd ());

    g.setColour (juce::Colours::grey);
    g.fillRect (loopStartX, getHeight () - 10, loopEndX - loopStartX, 10);
}

bool TimelineOverlayComponent::hitTest(int x, int y)
{
    auto loopRange = m_editViewState.m_edit.getTransport ().getLoopRange ();
    auto loopStartX = timeToX (loopRange.getStart ());
    auto loopEndX = timeToX (loopRange.getEnd ());
    auto rect = juce::Rectangle<int>(loopStartX
                                     , getHeight () - 10
                                     , loopEndX - loopStartX
                                     , 10);
    return rect.contains (x, y);
}

void TimelineOverlayComponent::mouseDown(const juce::MouseEvent &e)
{
    std::cout << "clicked!" << std::endl;
    m_loop1AtMousedown = m_editViewState.m_edit.getTransport ()
                                               .getLoopRange ()
                                               .getStart ();
    m_loop2AtMousedown = m_editViewState.m_edit.getTransport ()
                                               .getLoopRange ()
                                               .getEnd ();
}

void TimelineOverlayComponent::mouseDrag(const juce::MouseEvent &e)
{
    auto loopRange = m_editViewState.m_edit.getTransport ().getLoopRange ();
    auto loopStartX = timeToX (loopRange.getStart ());
    auto loopEndX = timeToX (loopRange.getEnd ());
    auto rect = juce::Rectangle<int>(loopStartX
                                     , getHeight () - 10
                                     , loopEndX - loopStartX
                                     , 10);
    if (rect.contains (e.x,e.y))
    {
        //move loopRect
        repaint ();
    }
}




int TimelineOverlayComponent::timeToX(double time)
{
    auto beats = m_editViewState.m_edit.tempoSequence.timeToBeats (time);
    return juce::roundToInt (((beats - m_editViewState.m_pianoX1)
                                  *  getWidth())
                                  / (m_editViewState.m_pianoX2 - m_editViewState.m_pianoX1));
}



//------------------------------------------------------------------------------

PianoRollComponent::PianoRollComponent(EditViewState & evs)
    : m_editViewState(evs)
    , m_keyboard (m_keybordstate
                  , juce::MidiKeyboardComponent::
                    Orientation::verticalKeyboardFacingRight)
    , m_timeline (evs, evs.m_pianoX1, evs.m_pianoX2)
    , m_timelineOverlay (evs)
    , m_playhead (evs.m_edit, evs, evs.m_pianoX1, evs.m_pianoX2)
{
    m_editViewState.m_edit.state.addListener (this);
    m_keybordstate.addListener (this);

    m_keyboard.setBlackNoteWidthProportion (0.5);
    m_keyboard.setBlackNoteLengthProportion (0.6);
    m_keyboard.setScrollButtonsVisible (false);

    addAndMakeVisible (m_keyboard);
    addAndMakeVisible (m_timeline);
    addAndMakeVisible (m_timelineOverlay);
    addAndMakeVisible (m_playhead);
    m_timelineOverlay.setAlwaysOnTop (true);
    m_playhead.setAlwaysOnTop (true);
}

PianoRollComponent::~PianoRollComponent()
{
    m_keybordstate.removeListener (this);
    m_editViewState.m_edit.state.removeListener (this);
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
    m_timelineOverlay.setBounds (timeline);

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
    if (m_pianoRollClip != nullptr)
    {
        m_pianoRollClip->setBounds (area);
        m_pianoRollClip->setKeyWidth (m_keyboard.getKeyWidth ());
    }

    m_playhead.setBounds (area.getUnion (timeline));

}

void PianoRollComponent::valueTreePropertyChanged(juce::ValueTree &v
                                                  , const juce::Identifier &i)
{
    if (v.hasType (tracktion_engine::IDs::MIDICLIP))
    {
        resized ();
        repaint ();
    }
    if (v.hasType (IDs::EDITVIEWSTATE))
    {
        if (i == IDs::pianoX1
            || i == IDs::pianoX2
            || i == IDs::pianoY1
            || i == IDs::pianorollNoteWidth)
        {
            resized ();
            repaint ();
        }
    }
}

void PianoRollComponent::handleNoteOn(juce::MidiKeyboardState *
                                      , int /*midiChannel*/
                                      , int midiNoteNumber
                                      , float v)
{
    if (m_pianoRollClip)
    {
        auto mc = m_pianoRollClip->getDefaulMidiClip ();
        auto midichannel = mc->getMidiChannel ();
        mc->getAudioTrack ()->playGuideNote
                          (midiNoteNumber,midichannel, 127.0 * v, false, true);
    }

}

void PianoRollComponent::handleNoteOff(juce::MidiKeyboardState *
                                       , int /*midiChannel*/
                                       , int /*midiNoteNumber*/
                                       , float)
{
    if (m_pianoRollClip)
    {
        auto mc = m_pianoRollClip->getDefaulMidiClip ();
        mc->getAudioTrack ()->turnOffGuideNotes ();
    }
}

tracktion_engine::Clip::Ptr PianoRollComponent::getClip()
{
    return m_clip;
}

tracktion_engine::MidiClip *PianoRollComponent::getMidiClip()
{
    return dynamic_cast<te::MidiClip*> (m_clip.get());
}

void PianoRollComponent::centerView()
{
    //center view of clip in horizontal
    if (m_pianoRollClip)
    {
        auto mc = m_pianoRollClip->getDefaulMidiClip ();
        auto width = m_editViewState.m_pianoX2 - m_editViewState.m_pianoX1;
        m_editViewState.m_pianoX1 = juce::jmax(0.0, mc->getStartBeat () - 1);
        m_editViewState.m_pianoX2 = m_editViewState.m_pianoX1 + width;
    }
    //in vertical
//    auto noteRange = getMidiClip ()->getSequence ().getNoteNumberRange ()
//            .expanded (10);
//    m_editViewState.m_pianoY1 = juce::jmax(0, noteRange.getStart ());
//    m_editViewState.m_pianoY2 = juce::jmin(127, noteRange.getEnd ());
}

void PianoRollComponent::setPianoRollClip(std::unique_ptr<PianoRollContentComponent> pianoRollClip)
{
    m_pianoRollClip = std::move (pianoRollClip);
    addAndMakeVisible (m_pianoRollClip.get());
    resized ();
}

void PianoRollComponent::clearPianoRollClip()
{
    m_pianoRollClip.reset (nullptr);
    resized ();
}
