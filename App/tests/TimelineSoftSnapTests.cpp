#include "TimelineSoftSnap.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void near(double a, double b, double tolerance = 1.0e-8) { require(std::abs(a - b) < tolerance, "position mismatch"); }
void run()
{
    near(TimelineSoftSnap::radiusPixels, 18.0);
    near(TimelineSoftSnap::intervalFraction, 0.3);
    // Maintainer feedback: noticeably stronger than the original 6px/20% detents.
    near(TimelineSoftSnap::map(10, {0, 100}), 0);
    near(TimelineSoftSnap::map(90, {0, 100}), 100);
    near(TimelineSoftSnap::map(5.5, {0, 20}), 0);
    near(TimelineSoftSnap::map(14.5, {0, 20}), 20);
    near(TimelineSoftSnap::map(8, {0, 20}), 5);
    near(TimelineSoftSnap::map(12, {0, 20}), 15);
    const auto midi = TimelineSoftSnap::profileForEditor(true);
    const auto song = TimelineSoftSnap::profileForEditor(false);
    near(midi.radiusPixels, 18); near(midi.intervalFraction, .3);
    near(song.radiusPixels, 18); near(song.intervalFraction, .4);
    for (const auto profile : {midi, song})
    {
        near(TimelineSoftSnap::map(18, {0, 100}, profile), 0);
        near(TimelineSoftSnap::map(82, {0, 100}, profile), 100);
        near(TimelineSoftSnap::map(20, {0, 100}, profile), 3.125); // escapable beyond 18px
    }
    near(TimelineSoftSnap::map(3.5, {0, 10}, song), 0); // dense arrangement grid: cap is decisive
    near(TimelineSoftSnap::map(3.5, {0, 10}, midi), 1.25); // narrow-grid cap unchanged
    for (const auto profile : {midi, song})
        for (double origin : {-123.37, 0.0, 123.37, 1.0e6})
            for (double width : {0.01, 1.0, 15.0, 30.0, 100.0, 1234.5})
            {
                const double radius = std::min(profile.radiusPixels, width * profile.intervalFraction);
                const double maxGain = 1 / (1 - 2 * profile.intervalFraction);
                const TimelineSoftSnap::Interval interval{origin, origin + width};
                near(TimelineSoftSnap::map(origin, interval, profile), origin);
                near(TimelineSoftSnap::map(origin + radius, interval, profile), origin);
                near(TimelineSoftSnap::map(origin + width - radius, interval, profile), origin + width);
                near(TimelineSoftSnap::map(origin + width, interval, profile), origin + width);
                double previous = origin;
                for (int i = 0; i <= 10000; ++i)
                {
                    const double x = origin + width * i / 10000.0;
                    const double y = TimelineSoftSnap::map(x, interval, profile);
                    const auto detailed = TimelineSoftSnap::mapDetailed(x, interval, profile);
                    near(detailed.position, y);
                    require(detailed.target.has_value() == (x <= interval.lower + radius || x >= interval.upper - radius),
                            "plateau feedback disagrees with kernel boundaries");
                    if (detailed.target)
                        near(*detailed.target, y);
                    require(y >= previous - 1.0e-9, "non-monotonic curve");
                    require(std::abs(y - x) <= radius + 1.0e-8, "unbounded attraction");
                    require(y - previous <= width / 10000.0 * maxGain + 1.0e-8, "unbounded slope");
                    near(TimelineSoftSnap::map(TimelineSoftSnap::inverseAnchor(y, interval, profile), interval, profile), y);
                    near(y, TimelineSoftSnap::map(x, interval, profile)); // history independent
                    if (profile.radiusPixels == midi.radiusPixels && profile.intervalFraction == midi.intervalFraction)
                        near(y, TimelineSoftSnap::map(x, interval)); // default remains MIDI, not arrangement
                    previous = y;
                }
                for (double boundary : {origin + radius, origin + width - radius})
                {
                    const double epsilon = width * 1.0e-7;
                    require(std::abs(TimelineSoftSnap::map(boundary + epsilon, interval, profile)
                                   - TimelineSoftSnap::map(boundary - epsilon, interval, profile)) < 2 * maxGain * epsilon + 1.0e-8,
                            "discontinuous detent boundary");
                }
                near(TimelineSoftSnap::inverseAnchor(origin, interval, profile), origin);
                near(TimelineSoftSnap::inverseAnchor(origin + width, interval, profile), origin + width);
            }
    for (const auto invalid : {TimelineSoftSnap::Profile{-1, .4}, TimelineSoftSnap::Profile{24, .5},
                               TimelineSoftSnap::Profile{NAN, .3}, TimelineSoftSnap::Profile{12, NAN}})
    {
        require(!TimelineSoftSnap::isValid(invalid), "invalid profile accepted");
        near(TimelineSoftSnap::map(20, {0, 100}, invalid), 20);
        near(TimelineSoftSnap::inverseAnchor(20, {0, 100}, invalid), 20);
    }
    // Adjacent unequal intervals meet at the exact target.
    near(TimelineSoftSnap::map(99.0, {0, 100}), 100);
    near(TimelineSoftSnap::map(100.1, {100, 101}), 100);
    require(TimelineSoftSnap::map(25, {0, 100}) != TimelineSoftSnap::map(TimelineSoftSnap::map(25, {0, 100}), {0, 100}),
            "double application must not be treated as idempotent");
    near(TimelineSoftSnap::map(2, {1, 1}), 2);
    near(TimelineSoftSnap::map(2, {NAN, 10}), 2);
    near(TimelineSoftSnap::inverseAnchor(2, {10, 1}), 2);
}
}
int main()
{
    try { run(); std::cout << "All TimelineSoftSnap tests passed.\n"; return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
