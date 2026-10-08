#pragma once
#include <tracktion_engine/tracktion_engine.h>

namespace ClipGestureLimits
{
namespace tracktion_engine = tracktion::engine;
enum class Kind { move, resizeLeft, resizeRight, stretch };
// Shared preview/commit constraints. Does not snap or mutate the clips.
double constrain(const juce::Array<tracktion_engine::Clip*>&, Kind, double requestedSeconds);
}
