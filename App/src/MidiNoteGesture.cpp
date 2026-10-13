#include "MidiNoteGesture.h"
#include "PianoRollNoteLength.h"
#include <algorithm>
#include <cmath>

namespace MidiNoteGesture
{
namespace
{
double base(const Item& item) { return item.clip->getStartBeat().inBeats() - item.clip->getOffsetInBeats().inBeats(); }
double time(const Item& item, double beat)
{
    return item.clip->edit.tempoSequence.toTime(tracktion::BeatPosition::fromBeats(beat)).inSeconds();
}
double beat(const Item& item, double seconds)
{
    return item.clip->edit.tempoSequence.toBeats(tracktion::TimePosition::fromSeconds(seconds)).inBeats();
}
}
std::optional<double> validSplitBeat(double start, double end, double proposed)
{
    if (!std::isfinite(start) || !std::isfinite(end) || !std::isfinite(proposed)
        || proposed - start < PianoRollNoteLength::minimumLengthBeats - 1.0e-10
        || end - proposed < PianoRollNoteLength::minimumLengthBeats - 1.0e-10)
        return {};
    return proposed;
}

double edgeTime(const Item& item, Kind kind)
{
    return time(item, base(item) + (kind == Kind::resizeRight ? item.note->getEndBeat().inBeats()
                                                               : item.note->getStartBeat().inBeats()));
}

double constrain(const juce::Array<Item>& items, Kind kind, double requested)
{
    if (!std::isfinite(requested))
        return 0;
    double delta = requested;
    for (const auto& item : items)
    {
        if (item.clip == nullptr || item.note == nullptr)
            continue;
        const double start = base(item) + item.note->getStartBeat().inBeats();
        const double end = start + item.note->getLengthBeats().inBeats();
        if (kind != Kind::resizeRight)
            delta = std::max(delta, time(item, std::max(0.0, base(item))) - time(item, start));
        if (kind == Kind::resizeLeft)
            delta = std::min(delta, time(item, end - PianoRollNoteLength::minimumLengthBeats) - time(item, start));
        else if (kind == Kind::resizeRight)
            delta = std::max(delta, time(item, start + PianoRollNoteLength::minimumLengthBeats) - time(item, end));
    }
    return delta;
}
double constrainViewBeatDelta(const juce::Array<Item>& items, Kind kind, const Item& reference,
                              double currentEdgeBeat, double requestedBeatDelta)
{
    if (reference.clip == nullptr || reference.note == nullptr || !std::isfinite(currentEdgeBeat)
        || !std::isfinite(requestedBeatDelta) || requestedBeatDelta == 0.0)
        return 0.0;

    const double sourceEdgeTime = edgeTime(reference, kind);
    const double requestedSeconds = time(reference, currentEdgeBeat + requestedBeatDelta) - sourceEdgeTime;
    const double constrainedSeconds = constrain(items, kind, requestedSeconds);
    const double allowed = beat(reference, sourceEdgeTime + constrainedSeconds) - currentEdgeBeat;

    return requestedBeatDelta < 0.0 ? std::clamp(allowed, requestedBeatDelta, 0.0)
                                    : std::clamp(allowed, 0.0, requestedBeatDelta);
}
double constrainViewKeyDelta(const juce::Array<Item>& items, int currentNoteDelta, double requestedKeyDelta)
{
    if (!std::isfinite(requestedKeyDelta) || requestedKeyDelta == 0.0)
        return 0.0;

    double allowed = requestedKeyDelta;
    bool foundNote = false;
    for (const auto& item : items)
    {
        if (item.note == nullptr)
            continue;
        foundNote = true;
        const int currentPitch = item.note->getNoteNumber() + currentNoteDelta;
        allowed = std::clamp(allowed, -static_cast<double>(currentPitch),
                             static_cast<double>(127 - currentPitch));
    }

    if (!foundNote)
        return 0.0;
    return requestedKeyDelta < 0.0 ? std::clamp(allowed, requestedKeyDelta, 0.0)
                                   : std::clamp(allowed, 0.0, requestedKeyDelta);
}
Timing resolve(const Item& item, Kind kind, double seconds)
{
    const double start = base(item) + item.note->getStartBeat().inBeats();
    const double end = start + item.note->getLengthBeats().inBeats();
    const double newStart = kind == Kind::resizeRight ? start : beat(item, time(item, start) + seconds);
    const double newEnd = kind == Kind::resizeRight ? beat(item, time(item, end) + seconds) : end;
    return {std::max(0.0, newStart - base(item)), kind == Kind::move ? item.note->getLengthBeats().inBeats()
               : std::max(PianoRollNoteLength::minimumLengthBeats, newEnd - newStart)};
}
}
