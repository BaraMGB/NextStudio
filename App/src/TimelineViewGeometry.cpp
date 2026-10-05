#include "TimelineViewGeometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace TimelineViewGeometry
{
double intervalBeats(int level, int beatsPerBar)
{
    static constexpr std::array fractions{1.0 / 960, 2.0 / 960, 5.0 / 960, 1.0 / 64, 1.0 / 32, 1.0 / 16, 1.0 / 8, 1.0 / 4, 1.0 / 2, 1.0};
    static constexpr std::array multiples{1, 2, 4, 8, 16, 64, 128, 256, 1024};
    if (level >= 0 && level < int(fractions.size()))
        return fractions[size_t(level)];
    if (level >= 10 && level <= 18)
        return std::max(1, beatsPerBar) * double(multiples[size_t(level - 10)]);
    return 1.0;
}
int gridLevel(double b, int meter)
{
    if (!std::isfinite(b) || b <= 0)
        return 9;
    for (int level = 3; level < 18; ++level)
        if (intervalBeats(level, meter) / b > 12.0 + 1.0e-9)
            return level;
    return 18;
}
std::optional<NormalizedView> normalize(ViewportContext context, ZoomRequest request)
{
    if (!std::isfinite(context.width) || context.width <= 0 || !std::isfinite(context.rasterScale) || context.rasterScale <= 0 || !std::isfinite(request.beatsPerPixel) || request.beatsPerPixel <= 0 || !std::isfinite(request.anchorBeat) || !std::isfinite(request.anchorX))
        return {};
    const double minB = minimumVisibleBeats / context.width, maxB = maximumVisibleBeats / context.width;
    if (!std::isfinite(minB) || !std::isfinite(maxB) || minB <= 0)
        return {};
    const bool fitting = request.policy == ZoomPolicy::fit;
    // Fits preserve content, even when the next admissible raster scale is
    // wider than the interactive limit. Clamping here can leave no candidate.
    const double wanted = fitting ? std::max(request.beatsPerPixel, minB) : std::clamp(request.beatsPerPixel, minB, maxB);
    double best = 0, bestCost = std::numeric_limits<double>::infinity(), bestN = 0;
    int bestLevel = 9;
    for (int level = 3; level <= 18; ++level)
    {
        const double interval = intervalBeats(level, context.beatsPerBar);
        const double numerator = interval * context.rasterScale;
        double lowN = fitting ? 1.0 : std::max(1.0, std::ceil(numerator / maxB));
        double highN = std::floor(numerator / minB);
        if (level < 18)
            lowN = std::max(lowN, std::floor((12.0 + 1.0e-9) * context.rasterScale) + 1.0);
        if (level > 3)
            highN = std::min(highN, std::floor((12.0 + 1.0e-9) * numerator / intervalBeats(level - 1, context.beatsPerBar)));
        if (fitting)
            highN = std::min(highN, std::floor(numerator / wanted + 1.0e-9));
        if (lowN > highN || !std::isfinite(lowN) || !std::isfinite(highN))
            continue;
        const double ideal = numerator / wanted;
        for (double n : {std::clamp(std::floor(ideal), lowN, highN), std::clamp(std::ceil(ideal), lowN, highN), lowN, highN})
        {
            const double b = numerator / n;
            if (gridLevel(b, context.beatsPerBar) != level || b < minB - 1.0e-14 || (!fitting && b > maxB + 1.0e-10))
                continue;
            if (fitting && b < wanted - 1.0e-14)
                continue;
            const double cost = std::abs(std::log(b / wanted));
            if (cost < bestCost - 1.0e-12 || (std::abs(cost - bestCost) <= 1.0e-12 && b > best))
            {
                best = b;
                bestCost = cost;
                bestLevel = level;
                bestN = n;
            }
        }
    }
    if (best <= 0 && fitting)
    {
        // At extreme extents the coarsest supported interval is less than one
        // physical pixel apart. No integer spacing can preserve the fit; keep
        // the requested linear transform instead of ignoring a finite request.
        best = wanted;
        bestLevel = gridLevel(best, context.beatsPerBar);
        bestN = intervalBeats(bestLevel, context.beatsPerBar) * context.rasterScale / best;
    }
    const double start = request.anchorBeat - request.anchorX * best;
    if (best <= 0 || !std::isfinite(start))
        return {};
    return NormalizedView{best, std::max(0.0, start), intervalBeats(bestLevel, context.beatsPerBar), bestN, bestLevel};
}
double beatToX(double beat, double start, double b) { return (beat - start) / b; }
double xToBeat(double x, double start, double b) { return start + x * b; }
std::vector<Line> lines(double start, double end, double width, double origin, int level, int meter)
{
    std::vector<Line> result;
    if (!std::isfinite(start) || !std::isfinite(end) || !std::isfinite(width) || !std::isfinite(origin) || end <= start || width <= 0)
        return result;
    const double interval = intervalBeats(level, meter);
    const double first = std::ceil(start / interval), last = std::floor(end / interval);
    // Protect malformed input and unsafe float-to-integer conversions.
    constexpr double indexLimit = 9.0e15;
    if (std::abs(first) > indexLimit || std::abs(last) > indexLimit || last - first > 1000000)
        return result;
    if (last < first)
        return result;
    result.reserve(size_t(last - first + 1));
    const double b = (end - start) / width;
    if (!std::isfinite(b) || b <= 0)
        return {};
    for (int64_t i = static_cast<int64_t>(first); i <= static_cast<int64_t>(last); ++i)
    {
        const double beat = double(i) * interval;
        result.push_back({i, beat, origin + beatToX(beat, start, b)});
    }
    return result;
}
double ZoomIntent::multiply(double actual, double factor, ViewportContext context, double seconds)
{
    if (!std::isfinite(actual) || actual <= 0 || !std::isfinite(factor) || factor <= 0 || !std::isfinite(context.width) || context.width <= 0 || !std::isfinite(seconds))
    {
        reset();
        return actual;
    }
    if (m_requested <= 0 || actual != m_lastActual || context != m_context || seconds - m_lastSeconds > 0.35 || seconds < m_lastSeconds)
        m_requested = actual;
    m_context = context;
    m_lastSeconds = seconds;
    m_requested = std::clamp(m_requested * factor, minimumVisibleBeats / context.width, maximumVisibleBeats / context.width);
    return m_requested;
}
} // namespace TimelineViewGeometry
