#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include <optional>

namespace MidiNoteGesture
{
namespace tracktion_engine = tracktion::engine;
enum class Kind { move, resizeLeft, resizeRight };
struct Item { const tracktion_engine::MidiClip* clip; const tracktion_engine::MidiNote* note; };
struct Timing { double startBeat, lengthBeats; }; // internal clip-sequence beats
// These production calculations are used by BOTH preview and commit.
double constrain(const juce::Array<Item>&, Kind, double requestedSeconds);
Timing resolve(const Item&, Kind, double effectiveSeconds);
std::optional<double> validSplitBeat(double globalStart, double globalEnd, double proposed);
}
