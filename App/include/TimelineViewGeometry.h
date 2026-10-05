#pragma once

#include <cstdint>
#include <optional>
#include <vector>

// Rendering/view policy only. Musical snapping remains in Tracktion/TimeLineComponent.
namespace TimelineViewGeometry
{
inline constexpr double minimumVisibleBeats = 0.05;
inline constexpr double maximumVisibleBeats = 100240.0; // Interactive zoom only; fits prioritize content visibility.

struct ViewportContext
{
    double width = 0;
    double rasterScale = 1;
    int beatsPerBar = 4;
    bool operator==(const ViewportContext &) const = default;
};
enum class ZoomPolicy
{
    nearest,
    fit // Conservative; may exceed the interactive upper limit to keep the requested content visible.
};
struct ZoomRequest
{
    double beatsPerPixel;
    double anchorBeat;
    double anchorX;
    ZoomPolicy policy = ZoomPolicy::nearest;
};
struct NormalizedView
{
    double beatsPerPixel;
    double startBeat;
    double intervalBeats;
    double physicalSpacing;
    int gridLevel;
};
struct Line
{
    int64_t index;
    double beat;
    double x;
};

double intervalBeats(int level, int beatsPerBar);
int gridLevel(double beatsPerPixel, int beatsPerBar);
std::optional<NormalizedView> normalize(ViewportContext, ZoomRequest);
double beatToX(double beat, double startBeat, double beatsPerPixel);
double xToBeat(double x, double startBeat, double beatsPerPixel);
std::vector<Line> lines(double startBeat, double endBeat, double width, double originX, int level, int beatsPerBar);

// Retain unnormalized intent, never persist it or put it into edit undo history.
class ZoomIntent
{
public:
    double multiply(double actual, double factor, ViewportContext, double seconds);
    void applied(double actual) { m_lastActual = actual; }
    void reset() { m_requested = 0; }

private:
    double m_requested = 0, m_lastActual = 0, m_lastSeconds = 0;
    ViewportContext m_context;
};
} // namespace TimelineViewGeometry
