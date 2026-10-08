#include "AutomationGestureLimits.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace AutomationGestureLimits
{
double constrain(const juce::Array<Point>& points, double requested)
{
    if (!std::isfinite(requested))
        return 0;
    double lower = -std::numeric_limits<double>::infinity();
    double upper = std::numeric_limits<double>::infinity();
    for (const auto& p : points)
    {
        if (p.parameter == nullptr || !juce::isPositiveAndBelow(p.index, p.parameter->getCurve().getNumPoints()))
            continue;
        const auto selected = [&](int index)
        {
            for (const auto& other : points)
                if (other.parameter == p.parameter && other.index == index)
                    return true;
            return false;
        };
        const auto& curve = p.parameter->getCurve();
        int left = p.index - 1, right = p.index + 1;
        while (left >= 0 && selected(left)) --left;
        while (right < curve.getNumPoints() && selected(right)) ++right;
        const double minimum = left >= 0 ? curve.getPointTime(left).inSeconds() : 0;
        const double maximum = right < curve.getNumPoints() ? curve.getPointTime(right).inSeconds()
                                                            : tracktion_engine::Edit::getMaximumEditEnd().inSeconds();
        lower = std::max(lower, minimum - p.originalTime.inSeconds());
        upper = std::min(upper, maximum - p.originalTime.inSeconds());
    }
    return lower <= upper ? std::clamp(requested, lower, upper) : 0;
}
}
