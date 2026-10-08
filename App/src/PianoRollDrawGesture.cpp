#include "PianoRollDrawGesture.h"
#include "PianoRollNoteLength.h"
#include <cmath>

bool PianoRollDrawGesture::begin(double start, double length, double pointerX,
                                const TimelineSnapResolver& resolver, bool bypass)
{
    reset();
    if (!std::isfinite(start) || !std::isfinite(length) || length <= 0 || !resolver.valid())
        return false;
    m_startBeat = start;
    m_endBeat = PianoRollNoteLength::constrainEnd(start, start + length);
    if (!bypass)
        m_endBeat = resolver.endAtOrAfter(m_endBeat, start + PianoRollNoteLength::minimumLengthBeats);
    m_endGesture.begin(m_endBeat, pointerX, resolver);
    m_endGesture.update(pointerX, resolver, bypass);
    m_endGesture.setDisplayedBeat(m_endBeat);
    return active();
}
void PianoRollDrawGesture::update(double pointerX, const TimelineSnapResolver& resolver, bool bypass, bool horizontalDrag)
{
    if (!active())
        return;
    m_dragged = m_dragged || horizontalDrag;
    const auto candidate = m_endGesture.update(pointerX, resolver, bypass);
    if (m_dragged)
        m_endBeat = PianoRollNoteLength::constrainEnd(m_startBeat, candidate);
    m_endGesture.setDisplayedBeat(m_endBeat);
}
