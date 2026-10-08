#pragma once
#include <tracktion_engine/tracktion_engine.h>

namespace AutomationGestureLimits
{
namespace tracktion_engine = tracktion::engine;
struct Point { tracktion_engine::AutomatableParameter* parameter; int index; tracktion::TimePosition originalTime; };
// Preserve point ordering and selected-point spacing with one feasible delta.
double constrain(const juce::Array<Point>& points, double requestedSeconds);
}
