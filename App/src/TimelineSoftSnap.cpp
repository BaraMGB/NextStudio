#include "TimelineSoftSnap.h"
#include <algorithm>
#include <cmath>

namespace TimelineSoftSnap
{
namespace
{
bool valid(double position, Interval interval)
{
    return std::isfinite(position) && std::isfinite(interval.lower) && std::isfinite(interval.upper)
        && interval.upper > interval.lower && std::isfinite(interval.upper - interval.lower);
}
}
bool isValid(Profile profile)
{
    return std::isfinite(profile.radiusPixels) && profile.radiusPixels >= 0
        && std::isfinite(profile.intervalFraction) && profile.intervalFraction >= 0 && profile.intervalFraction < 0.5;
}
double map(double rawPosition, Interval interval, Profile profile)
{
    if (!valid(rawPosition, interval) || !isValid(profile))
        return rawPosition;
    const auto width = interval.upper - interval.lower;
    const auto radius = std::min(profile.radiusPixels, profile.intervalFraction * width);
    if (rawPosition <= interval.lower + radius)
        return interval.lower;
    if (rawPosition >= interval.upper - radius)
        return interval.upper;
    return interval.lower + (rawPosition - interval.lower - radius) * width / (width - 2 * radius);
}
double inverseAnchor(double displayedPosition, Interval interval, Profile profile)
{
    if (!valid(displayedPosition, interval) || !isValid(profile))
        return displayedPosition;
    if (displayedPosition <= interval.lower)
        return interval.lower;
    if (displayedPosition >= interval.upper)
        return interval.upper;
    const auto width = interval.upper - interval.lower;
    const auto radius = std::min(profile.radiusPixels, profile.intervalFraction * width);
    return interval.lower + radius + (displayedPosition - interval.lower) * (width - 2 * radius) / width;
}
}
