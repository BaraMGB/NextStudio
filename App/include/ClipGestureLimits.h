#pragma once
#include <functional>
#include <tracktion_engine/tracktion_engine.h>

namespace ClipGestureLimits
{
namespace tracktion_engine = tracktion::engine;
enum class Kind { move, resizeLeft, resizeRight, stretch };
// Shared preview/commit constraints. Does not snap or mutate the clips.
double constrain(const juce::Array<tracktion_engine::Clip*>&, Kind, double requestedSeconds);
// Shared whole-group move/copy feasibility, using the caller's track-type policy.
bool validMoveDestinations(const juce::Array<tracktion_engine::Clip*>&,
                           const juce::Array<tracktion_engine::Track*>& orderedTracks, int verticalOffset,
                           const std::function<bool(const tracktion_engine::Clip*, const tracktion_engine::Track*)>& accepts);
double edgeTime(const tracktion_engine::ClipPosition&, Kind);
tracktion::TimeRange previewRange(const tracktion_engine::ClipPosition&, Kind, double constrainedSeconds);
}
