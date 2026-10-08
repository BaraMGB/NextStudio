#pragma once
#include "TimelineSnapResolver.h"

// Provisional creation only: model mutation belongs to MidiViewport::addNewNote.
class PianoRollDrawGesture
{
public:
    bool begin(double startBeat, double defaultLength, double pointerX,
               const TimelineSnapResolver&, bool bypass);
    void update(double pointerX, const TimelineSnapResolver&, bool bypass, bool horizontalDrag);
    void reset() { m_endGesture.reset(); m_dragged = false; }
    bool active() const { return m_endGesture.active(); }
    const TimelineSnapResult& feedback() const { return m_endGesture.feedback(); }
    double startBeat() const { return m_startBeat; }
    double endBeat() const { return m_endBeat; }
private:
    TimelineMouseGesture m_endGesture;
    double m_startBeat = 0, m_endBeat = 0;
    bool m_dragged = false;
};
