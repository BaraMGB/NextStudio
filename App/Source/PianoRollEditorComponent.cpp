#include "PianoRollEditorComponent.h"



PianoRollDisplay::PianoRollDisplay(EditViewState & evs
                                   , tracktion_engine::Clip::Ptr clip
                                   , juce::MidiKeyboardComponent & keyboard
                                   , TimeLineComponent & timeline)
    : m_editViewState(evs)
    , m_clip(clip)
    , m_keyboard(keyboard)
    , m_timeline(timeline)
{
}

PianoRollDisplay::~PianoRollDisplay()
{


}

void PianoRollDisplay::paint(juce::Graphics &g)
{
    int y1 = m_editViewState.m_pianoY1;
    int y2 = m_editViewState.m_pianoY2;
    //draw horizontal Lines
    float noteHeight = m_keyboard.getKeyWidth () * 7 / 12;
    float line = getHeight ();
    for (auto i = y1; i <= y2 ; i++)
    {
        line = line - noteHeight  ;
        if (juce::MidiMessage::isMidiNoteBlack (i))
        {
           g.setColour (juce::Colour(0x11ffffff));
        }
        else
        {
            g.setColour (juce::Colour(0x22ffffff));
        }
        juce::Rectangle<float> lineRect = {0.0, line, (float) getWidth (), noteHeight};
        g.fillRect(lineRect.reduced (0, 1));
    }
    g.setColour (juce::Colours::black);
    drawVerticalLines (g);
    if (auto mc = getMidiClip () && getMidiClip ()->getAudioTrack () != nullptr)
    {
        for (auto & trackClip : getMidiClip ()->getAudioTrack ()->getClips ())
        {
            if (auto midiClip = dynamic_cast<te::MidiClip*>(trackClip))
            {
                auto& seq = midiClip->getSequence();
                //draw Notes
                for (auto n : seq.getNotes())
                {
                    auto yOffset = n->getNoteNumber () - y1 + 1;
                    auto noteY = getHeight () - (yOffset * noteHeight);
                    double sBeat = n->getStartBeat() - midiClip->getOffsetInBeats();
                    double eBeat = n->getEndBeat() - midiClip->getOffsetInBeats();

                    auto x1 = m_timeline.beatsToX (sBeat + midiClip->getStartBeat ());
                    auto x2 = m_timeline.beatsToX (eBeat + midiClip->getStartBeat ());

                    if ( sBeat < 0 || eBeat > midiClip->getEndBeat ()
                                              - midiClip->getStartBeat ())
                    {
                        g.setColour (juce::Colours::grey);
                    }
                    else if (n->getColour () == 127)
                    {
                        g.setColour (juce::Colours::green);
                    }
                    else
                    {
                        g.setColour (juce::Colours::white);
                    }
                    juce::Rectangle<float> noteRect
                                (float (x1)     , float (noteY)
                               , float (x2 - x1), float (noteHeight));
                    g.fillRect (noteRect.reduced (1,1));
                }
                //draw ClipRange
                auto clipStartX = m_timeline.beatsToX (midiClip->getStartBeat ());
                auto clipEndX = m_timeline.beatsToX (midiClip->getEndBeat ());
                g.setColour (midiClip->getTrack ()->getColour ());
                g.drawRect (clipStartX  , 0, clipEndX - clipStartX, getHeight ());
                g.setColour (midiClip->getTrack ()->getColour ().withAlpha (0.2f));
                g.fillRect (clipStartX  , 0, clipEndX - clipStartX, getHeight ());
            }
        }
    }
}

void PianoRollDisplay::mouseDown(const juce::MouseEvent &e)
{
    m_clickedNote = getNoteByPos (e.position);
    auto clickedBeat = m_timeline.xToBeats (e.position.x);

    if (!e.mouseWasDraggedSinceMouseDown ())
    {

        if (m_clickedNote)
        {
            if (e.mods.isRightButtonDown ())
            {
                getMidiClip ()->getSequence ()
                        .removeNote (*m_clickedNote
                                     , &m_editViewState.m_edit.getUndoManager ());
                repaint ();
            }
            else
            {
                m_clickOffset = m_clickedNote->getStartBeat () - clickedBeat;
            }
        }
        else if (e.getNumberOfClicks () > 1 || e.mods.isShiftDown ())
        {



            auto beat = clickedBeat
                      - getMidiClip ()->getStartBeat ()
                      + getMidiClip ()->getOffsetInBeats ();

            m_editViewState.m_snapType = 7;
            beat = m_editViewState.getSnapedBeat (beat, true);
            getMidiClip ()->getSequence ().addNote
                    (getNoteNumber (e.position.y)
                     , beat
                     , 0.25
                     , 127
                     , 111
                     , &m_editViewState.m_edit.getUndoManager ());
            repaint();
        }
    }
}

void PianoRollDisplay::mouseDrag(const juce::MouseEvent &e)
{
    if(e.mods.isLeftButtonDown ())
    {
        auto um = &m_editViewState.m_edit.getUndoManager ();
        if (m_clickedNote != nullptr)
        {
            if (m_expandLeft)
            {
                auto oldEndBeat = m_clickedNote->getEndBeat ();
                auto newRawStartBeat = m_timeline.xToBeats (e.position.x)
                        + m_clickOffset;
                auto snapedStart = m_editViewState.getSnapedBeat (newRawStartBeat);
                auto newStart = e.mods.isShiftDown ()
                              ? newRawStartBeat
                              : snapedStart;
                m_clickedNote->setStartAndLength (newStart
                                                  , oldEndBeat - newStart
                                                  , um);
            }
            else if (m_expandRight)
            {
                auto oldStart = m_clickedNote->getStartBeat ();
                auto newEnd = m_timeline.xToBeats (e.position.x)
                            + getMidiClip ()->getOffsetInBeats ()
                            - getMidiClip ()->getStartBeat ()
                            - oldStart;
                auto snapedEnd = m_editViewState.getSnapedBeat (newEnd);
                m_clickedNote->setStartAndLength (oldStart
                                                  , e.mods.isShiftDown ()
                                                  ? newEnd
                                                  : snapedEnd
                                                  , um);
            }
            else
            {
                auto length = m_clickedNote->getLengthBeats ();
                auto newBeat = m_timeline.xToBeats (e.position.x) + m_clickOffset;
                auto snapedBeat = m_editViewState.getSnapedBeat (newBeat);
                m_clickedNote->setStartAndLength (e.mods.isShiftDown ()
                                                  ? newBeat
                                                  : snapedBeat
                                                    , length
                                                    , um);
                m_clickedNote->setNoteNumber (getNoteNumber (e.position.y), um);
            }
        }
        repaint ();
    }
}

void PianoRollDisplay::mouseMove(const juce::MouseEvent &e)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
    for (auto n : getMidiClip ()->getSequence ().getNotes ())
    {
        n->setColour (111, &m_editViewState.m_edit.getUndoManager ());
    }
    if (auto note = getNoteByPos (e.position))
    {
        auto startX = m_timeline.beatsToX (note->getStartBeat ()
                                           + getMidiClip ()->getStartBeat ()
                                         - getMidiClip ()->getOffsetInBeats ()
                                           );
        auto endX = m_timeline.beatsToX (note->getEndBeat ()
                                           + getMidiClip ()->getStartBeat ()
                                         - getMidiClip ()->getOffsetInBeats ()
                                         );
        if (e.position.x < startX + 10)
        {
            setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
            m_expandLeft = true;
            m_expandRight = false;
        }
        else if (e.position.x > endX - 10)
        {
            setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
            m_expandLeft = false;
            m_expandRight = true;
        }
        else
        {
            m_expandLeft = false;
            m_expandRight = false;
        }
        note->setColour (127, &m_editViewState.m_edit.getUndoManager ());
    }
    repaint ();
}

void PianoRollDisplay::mouseExit(const juce::MouseEvent &)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void PianoRollDisplay::mouseUp(const juce::MouseEvent &)
{
}

void PianoRollDisplay::mouseWheelMove(const juce::MouseEvent &event
                                     , const juce::MouseWheelDetails &wheel)
{
    if (event.mods.isShiftDown ())
    {
        auto deltaX1 = event.mods.isCtrlDown () ? wheel.deltaY : -wheel.deltaY;
        auto deltaX2 = -wheel.deltaY;

        m_editViewState.m_pianoX1 =  m_editViewState.m_pianoX1 + deltaX1;
        m_editViewState.m_pianoX2 =  m_editViewState.m_pianoX2 + deltaX2;
    }
    else
    {
        auto deltaY1 = wheel.deltaY >= 0 ? 1 : -1;
        auto deltaY2 = event.mods.isCtrlDown () ? -deltaY1 : deltaY1;

        m_editViewState.m_pianoY1 = juce::jlimit(0
                                                 ,127
                                                 , m_editViewState.m_pianoY1
                                                 + deltaY1);
        m_editViewState.m_pianoY2 = juce::jlimit(m_editViewState.m_pianoY1 + 7
                                                 , 127
                                                 , m_editViewState.m_pianoY2
                                                 + deltaY2);
    }
}

void PianoRollDisplay::drawVerticalLines(juce::Graphics &g)
{
    double x1 = m_editViewState.m_pianoX1;
    double x2 = m_editViewState.m_pianoX2;
    double zoom = x2 - x1;
    int firstBeat = static_cast<int>(x1);
    if(m_timeline.beatsToX(firstBeat) < 0)
    {
        firstBeat++;
    }
    auto pixelPerBeat = getWidth() / zoom;
    for (int beat = firstBeat - 1; beat <= x2; beat++)
    {
        const int BeatX = m_timeline.beatsToX(beat) - 1;
        auto zBars = 16;
        if (zoom < 240)
        {
            zBars /= 2;
        }
        if (zoom < 120)
        {
            zBars /=2;
        }
        if (beat % zBars == 0)
        {
            g.drawLine(BeatX, 0, BeatX, getHeight());
        }
        if (zoom < 60)
        {
            g.drawLine(BeatX,0, BeatX, getHeight());
        }
        if (zoom < 25)
        {
            auto quarterBeat = pixelPerBeat / 4;
            auto i = 1;
            while ( i < 5)
            {
                g.drawLine(BeatX + quarterBeat * i ,0,
                           BeatX + quarterBeat * i ,getHeight());
                i++;
            }
        }
    }
}

int PianoRollDisplay::getNoteNumber(int y)
{
    double noteHeight = m_keyboard.getKeyWidth () * 7 / 12;
    double noteNumb = m_editViewState.m_pianoY1 + ((getHeight () - y )/ noteHeight);
    return noteNumb;
}

tracktion_engine::MidiNote *PianoRollDisplay::getNoteByPos(juce::Point<float> pos)
{
    for (auto note : getMidiClip ()->getSequence ().getNotes ())
    {
        if (note->getNoteNumber () == getNoteNumber (pos.y))
        {
            auto clickedBeat = m_timeline.xToBeats (pos.x)
                    + getMidiClip ()->getOffsetInBeats ();
            auto clipstart = getMidiClip ()->getStartBeat ();
            if ( clickedBeat > note->getStartBeat () + clipstart
             && clickedBeat < note->getEndBeat () + clipstart)
            {
                return note;
            }
        }
    }
    return nullptr;
}


//------------------------------------------------------------------------------

PianoRollComponent::PianoRollComponent(EditViewState & evs
                                       , tracktion_engine::Clip::Ptr clip)
    : m_editViewState(evs)
    , m_clip(clip)
    , m_keyboard (m_editViewState.m_edit.engine.getDeviceManager ().getDefaultMidiInDevice ()->keyboardState
                  , juce::MidiKeyboardComponent::
                    Orientation::verticalKeyboardFacingRight)
    , m_timeline (evs, evs.m_pianoX1, evs.m_pianoX2)
    , m_pianoRoll (evs, clip, m_keyboard, m_timeline)
    , m_playhead (evs.m_edit, evs, evs.m_pianoX1, evs.m_pianoX2)
{
    m_editViewState.m_edit.state.addListener (this);


    m_keyboard.setBlackNoteWidthProportion (0.5);
    m_keyboard.setBlackNoteLengthProportion (0.6);
    m_keyboard.setScrollButtonsVisible (false);

    addAndMakeVisible (m_keyboard);
    addAndMakeVisible (m_timeline);
    addAndMakeVisible (m_pianoRoll);
    addAndMakeVisible (m_playhead);
    m_playhead.setAlwaysOnTop (true);
}

PianoRollComponent::~PianoRollComponent()
{
    m_editViewState.m_edit.engine.getDeviceManager ().getDefaultMidiInDevice ()->keyboardState.removeListener (this);
    m_editViewState.m_edit.state.removeListener (this);
}

void PianoRollComponent::focusLost(juce::Component::FocusChangeType cause)
{
    m_editViewState.m_edit.engine.getDeviceManager ().getDefaultMidiInDevice ()->keyboardState.removeListener (this);
}

void PianoRollComponent::focusGained(juce::Component::FocusChangeType cause)
{
    m_editViewState.m_edit.engine.getDeviceManager ().getDefaultMidiInDevice ()->keyboardState.addListener (this);
}

void PianoRollComponent::resized()
{
    auto area = getLocalBounds ();
    auto keyboard = area.removeFromLeft (50);
    auto timeline = area.removeFromTop (20);

    double firstVisibleNote = m_editViewState.m_pianoY1;
    double lastVisibleNote  = m_editViewState.m_pianoY2;
    double pianoRollNoteWidth = getHeight () / (lastVisibleNote - firstVisibleNote);

    m_keyboard.setKeyWidth (juce::jmax(0.1, pianoRollNoteWidth * 12 / 7));
    m_keyboard.setBounds (0
                          , getHeight () - m_keyboard.getTotalKeyboardWidth ()
                            + (firstVisibleNote * pianoRollNoteWidth)
                          , keyboard.getWidth ()
                          , m_keyboard.getTotalKeyboardWidth ());
    m_timeline.setBounds (timeline);
    m_pianoRoll.setBounds (area);

    m_playhead.setBounds (area);
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
            || i == IDs::pianoY2)
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
//    auto midichannel = getMidiClip ()->getMidiChannel ();
//    getMidiClip ()->getAudioTrack ()->playGuideNote
//                      (midiNoteNumber,midichannel, 127.0 * v, false, true);
}

void PianoRollComponent::handleNoteOff(juce::MidiKeyboardState *
                                       , int /*midiChannel*/
                                       , int /*midiNoteNumber*/
                                       , float)
{
    //getMidiClip ()->getAudioTrack ()->turnOffGuideNotes ();
}

void PianoRollComponent::centerView()
{
    //center view of clip in horizontal
if (auto midiclip = getMidiClip ())
{
    auto width = m_editViewState.m_pianoX2 - m_editViewState.m_pianoX1;
    m_editViewState.m_pianoX1 = juce::jmax(0.0, midiclip->getStartBeat () - 1);
    m_editViewState.m_pianoX2 = m_editViewState.m_pianoX1 + width;
}
    //in vertical
//    auto noteRange = getMidiClip ()->getSequence ().getNoteNumberRange ()
//            .expanded (10);
//    m_editViewState.m_pianoY1 = juce::jmax(0, noteRange.getStart ());
//    m_editViewState.m_pianoY2 = juce::jmin(127, noteRange.getEnd ());
}

juce::MidiKeyboardComponent &PianoRollComponent::getKeyboard()
{
    return m_keyboard;
}

