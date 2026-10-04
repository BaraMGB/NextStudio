#pragma once

#include <algorithm>
#include <cmath>

namespace PitchShiftDisplay
{
// Top is positive pitch, bottom is negative pitch. Do not quantise fractional shifts.
[[nodiscard]] inline float positionForSemitones(float semitones, float maximumSemitones)
{
    if (!std::isfinite(semitones) || !std::isfinite(maximumSemitones) || maximumSemitones <= 0.0f)
        return 0.5f;

    return 0.5f - std::clamp(semitones / maximumSemitones, -1.0f, 1.0f) * 0.5f;
}
// Relative vertical drag keeps the grab offset, but only this input path snaps.
[[nodiscard]] inline float snappedSemitonesForDrag(float startSemitones, float verticalDelta, float scaleHeight, float maximumSemitones)
{
    if (!std::isfinite(startSemitones) || !std::isfinite(verticalDelta) || !std::isfinite(scaleHeight)
        || !std::isfinite(maximumSemitones) || scaleHeight <= 0.0f || maximumSemitones <= 0.0f)
        return startSemitones;

    const auto value = startSemitones - (verticalDelta / scaleHeight) * (2.0f * maximumSemitones);
    return std::clamp(std::round(value), -maximumSemitones, maximumSemitones);
}
} // namespace PitchShiftDisplay
