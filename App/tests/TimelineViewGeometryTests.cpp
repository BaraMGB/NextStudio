#include "TimelineViewGeometry.h"
#include <cmath>
#include <iostream>
#include <juce_gui_basics/juce_gui_basics.h>
#include <stdexcept>

namespace tg = TimelineViewGeometry;
namespace
{
void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
void near(double actual, double expected, double epsilon = 1.0e-8)
{
    if (!std::isfinite(actual) || !std::isfinite(expected) || std::abs(actual - expected) > epsilon)
    {
        std::cerr << "actual=" << actual << " expected=" << expected << " diff=" << actual - expected << " tolerance=" << epsilon << '\n';
        throw std::runtime_error("coordinate mismatch");
    }
}
void geometryTests()
{
    for (double scale : {1.0, 1.25, 1.5, 2.0})
        for (double pixelsPerBeat : {206.5, 218.5, 186.0, 176.5})
            for (double start : {0.0, 0.137, 1000000.137})
            {
                tg::ViewportContext context{1200.0, scale, 4};
                tg::ZoomRequest request{1.0 / pixelsPerBeat, start + 350.0 / pixelsPerBeat, 350.0};
                auto result = tg::normalize(context, request);
                require(result.has_value(), "normalization failed");
                near(result->intervalBeats * scale / result->beatsPerPixel, result->physicalSpacing);
                require(result->gridLevel == tg::gridLevel(result->beatsPerPixel, 4), "unstable grid level");
                if (result->startBeat > 0)
                    near(tg::beatToX(request.anchorBeat, result->startBeat, result->beatsPerPixel), 350.0, 1.0e-6);
                else
                    require(tg::beatToX(request.anchorBeat, 0, result->beatsPerPixel) <= 350.0 + 1.0e-6, "incorrect beat-zero clamp");
                auto again = tg::normalize(context, {result->beatsPerPixel, result->startBeat, 0.0});
                near(again->beatsPerPixel, result->beatsPerPixel);
                auto lines = tg::lines(result->startBeat, result->startBeat + context.width * result->beatsPerPixel, context.width, 0.0, result->gridLevel, 4);
                require(lines.size() > 2, "missing lines");
                for (size_t i = 1; i < lines.size(); ++i)
                {
                    near((lines[i].x - lines[i - 1].x) * scale, result->physicalSpacing, 1.0e-5);
                    require(lines[i].x > lines[i - 1].x, "unordered grid");
                    near(lines[i].beat, lines[i].index * result->intervalBeats);
                }
            }

    // Dense sweep exercises every interval transition and the zoom limits.
    for (int meter : {3, 4, 5, 7})
        for (double scale : {1.0, 1.25, 1.5, 2.0})
        {
            double last = 0.0;
            for (int i = 0; i <= 6000; ++i)
            {
                const double requested = std::exp(std::log(0.05 / 1200.0) + i / 6000.0 * std::log(100240.0 / 0.05));
                auto r = tg::normalize({1200, scale, meter}, {requested, 1000000, 600});
                require(r.has_value(), "no admissible scale");
                require(r->beatsPerPixel >= last - 1.0e-12, "reversing zoom");
                require(r->beatsPerPixel * 1200 >= 0.05 - 1.0e-10 && r->beatsPerPixel * 1200 <= 100240 + 1.0e-6, "zoom limits");
                near(tg::normalize({1200, scale, meter}, {r->beatsPerPixel, 1000000, 600})->beatsPerPixel, r->beatsPerPixel);
                last = r->beatsPerPixel;
            }
        }
    // Fit must not silently fail near the interactive zoom limit. Its wider
    // raster-aligned candidate may exceed that limit; passive normalization
    // uses the same fit policy for such views.
    for (double width : {600.0, 1200.0, 1360.0})
        for (int meter : {3, 4, 5, 7})
            for (double scale : {1.0, 1.25, 1.5, 2.0})
            {
                double last = 0.0;
                for (int i = 0; i <= 6000; ++i)
                {
                    const double length = std::exp(std::log(0.05) + i / 6000.0 * std::log(100240.0 / 0.05));
                    const double requested = length / width;
                    const tg::ViewportContext context{width, scale, meter};
                    auto result = tg::normalize(context, {requested, 1000000, width / 2, tg::ZoomPolicy::fit});
                    require(result.has_value(), "fit has no admissible scale");
                    require(result->beatsPerPixel >= requested - 1.0e-12, "fit crops requested range");
                    require(result->beatsPerPixel >= last - 1.0e-12, "reversing fit");
                    near(result->physicalSpacing, std::round(result->physicalSpacing));
                    require(result->gridLevel == tg::gridLevel(result->beatsPerPixel, meter), "unstable fit grid level");
                    auto again = tg::normalize(context, {result->beatsPerPixel, result->startBeat, 0, tg::ZoomPolicy::fit});
                    require(again.has_value(), "fit is not restorable");
                    near(again->beatsPerPixel, result->beatsPerPixel);
                    last = result->beatsPerPixel;
                }
            }
    auto clamped = tg::normalize({1000, 1, 4}, {0.01, 0.0, 500});
    near(clamped->startBeat, 0.0);
    auto fit = tg::normalize({1000, 1, 4}, {0.0051, 4.0, 500, tg::ZoomPolicy::fit});
    require(fit.has_value() && fit->beatsPerPixel >= 0.0051, "fit crops content");
    auto oversized = tg::normalize({1200, 1, 4}, {200000.0 / 1200, 200000, 600, tg::ZoomPolicy::fit});
    require(oversized.has_value() && oversized->beatsPerPixel >= 200000.0 / 1200, "fit clamps long range");
    // Beyond the interval table's raster resolution, content visibility takes
    // precedence over integer spacing rather than rejecting a finite fit.
    auto subpixel = tg::normalize({1200, 1, 4}, {1.0e6, 1.0e9, 600, tg::ZoomPolicy::fit});
    require(subpixel.has_value(), "subpixel fit failed");
    near(subpixel->beatsPerPixel, 1.0e6);
    for (double invalid : {0.0, -1.0, double(INFINITY), double(NAN)})
    {
        require(!tg::normalize({invalid, 1, 4}, {0.01, 0, 0}), "invalid width accepted");
        require(!tg::normalize({1000, invalid, 4}, {0.01, 0, 0}), "invalid scale accepted");
        require(!tg::normalize({1000, 1, 4}, {invalid, 0, 0}), "invalid zoom accepted");
    }
    // Float widths and slice origins must not introduce another transform.
    const double b = 1.0 / 224;
    const double start = 0.137;
    const double sliceStart = start + 73.37 * b;
    const double sliceWidth = 250.75;
    const double sliceEnd = sliceStart + sliceWidth * b;
    auto slice = tg::lines(sliceStart, sliceEnd, sliceWidth, 73.37, 5, 4);
    for (const auto &line : slice)
        near(line.x, tg::beatToX(line.beat, start, b));
    for (double beat : {0.25, 0.37, 0.5, 0.78, 4.0})
        near(tg::xToBeat(tg::beatToX(beat, start, b), start, b), beat);
    require(tg::lines(0, 1, 0, 0, 5, 4).empty(), "invalid range accepted");
    const auto negative = tg::lines(-0.137, 1.237, 300.75, 12.5, 5, 4);
    require(!negative.empty() && negative.front().beat < 0, "negative offscreen start lost");
    for (size_t i = 1; i < negative.size(); ++i)
    {
        require(negative[i].index == negative[i - 1].index + 1, "duplicate or skipped index");
        require(negative[i].x > negative[i - 1].x, "negative range unordered");
    }
    require(!tg::normalize({1200, 1, 4}, {50.0, 0, -1.0e308}), "overflowing anchor accepted");

    tg::ZoomIntent intent;
    auto initial = tg::normalize({1200, 1, 4}, {1.0 / 224, 1.0, 0.0});
    const double actual = initial->beatsPerPixel;
    double latest = actual;
    for (int i = 0; i < 20; ++i)
    {
        auto wanted = intent.multiply(latest, 0.997, {1200, 1, 4}, 10.0);
        auto r = tg::normalize({1200, 1, 4}, {wanted, 1, 0});
        intent.applied(r->beatsPerPixel);
        latest = r->beatsPerPixel;
    }
    require(latest < actual, "small gestures stuck");
    // External scale/context changes and gesture timeout discard residuals.
    near(intent.multiply(0.02, 0.9, {1200, 1, 4}, 10.1), 0.018);
    intent.applied(0.018);
    near(intent.multiply(0.018, 0.9, {800, 1, 4}, 10.2), 0.0162);
    intent.applied(0.0162);
    near(intent.multiply(0.0162, 0.9, {800, 1, 4}, 20.0), 0.01458);
}
void rasterTests()
{
    for (double scale : {1.0, 1.25, 1.5, 2.0})
        for (double phase : {0.0, 0.137, 0.5, 0.83})
        {
            auto r = tg::normalize({600, scale, 4}, {1.0 / 218.5, 0, 0});
            juce::Image image(juce::Image::RGB, 1200, 50, true);
            juce::Graphics g(image);
            g.fillAll(juce::Colours::black);
            g.addTransform(juce::AffineTransform::scale(static_cast<float>(scale)));
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            for (int i = 1; i <= 20; ++i)
            {
                const float x = static_cast<float>((i * r->physicalSpacing + phase) / scale);
                g.drawLine(x, 0, x, 20, 1.0f);
            }
            // Compare translated coverage windows, not just total brightness.
            for (int i = 2; i <= 20; ++i)
                for (int offset = -3; offset <= 3; ++offset)
                {
                    const int spacing = static_cast<int>(r->physicalSpacing);
                    auto a = image.getPixelAt(spacing + offset, 10).getRed();
                    auto c = image.getPixelAt(i * spacing + offset, 10).getRed();
                    require(std::abs(int(a) - int(c)) <= 1, "unequal raster coverage");
                }
        }
}
} // namespace
int main()
{
    try
    {
        geometryTests();
        rasterTests();
        std::cout << "Timeline geometry and raster tests passed\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
