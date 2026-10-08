#pragma once
#include <tracktion_engine/tracktion_engine.h>
#include <functional>

namespace MidiNoteCreation
{
namespace tracktion_engine = tracktion::engine;
using BeforeRemoval = std::function<void(tracktion_engine::MidiNote*)>;
void clear(const tracktion_engine::MidiClip*, int pitch, const juce::Array<tracktion::BeatRange>&,
           const BeforeRemoval& = {});
tracktion_engine::MidiNote* add(const tracktion_engine::MidiClip*, int pitch, double start, double length, int velocity,
                               const BeforeRemoval&, const std::function<void(double)>& rememberLength);
}
