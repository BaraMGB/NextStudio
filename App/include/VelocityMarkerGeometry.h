#pragma once
#include <juce_graphics/juce_graphics.h>

// Shared by painted marker heads, point/rectangle hits and musical lasso anchors.
namespace VelocityMarkerGeometry
{
inline int span(int height) { return juce::jmax(1, height - 8); }
inline float markerY(int velocity, int height) { return float(height - 4 - juce::jmap(velocity, 0, 127, 0, span(height))); }
inline double velocityAt(float y, int height) { return (height - 4 - double(y)) * 127.0 / span(height); }
inline float projectVelocity(double velocity, int height) { return float(height - 4 - velocity * span(height) / 127.0); }
} // namespace VelocityMarkerGeometry
