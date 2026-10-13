#include "DragEdgeScroll.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void near(double actual, double expected, double tolerance = 1.0e-9)
{
    require(std::abs(actual - expected) <= tolerance, "value mismatch");
}

void testQuietAreaAndDirections()
{
    DragEdgeScroll::Config config;
    near(DragEdgeScroll::velocityForAxis(50.0, 100.0, config), 0.0);
    require(DragEdgeScroll::velocityForAxis(20.0, 100.0, config) < 0.0, "left edge did not scroll left");
    require(DragEdgeScroll::velocityForAxis(80.0, 100.0, config) > 0.0, "right edge did not scroll right");

    const auto velocity = DragEdgeScroll::velocity(10.0, 90.0, 100.0, 100.0, {}, config);
    require(velocity.x < 0.0 && velocity.y > 0.0, "diagonal edge directions are wrong");
}

void testProgressiveAndBoundedSpeed()
{
    DragEdgeScroll::Config config;
    const auto inner = std::abs(DragEdgeScroll::velocityForAxis(30.0, 100.0, config));
    const auto middle = std::abs(DragEdgeScroll::velocityForAxis(20.0, 100.0, config));
    const auto edge = std::abs(DragEdgeScroll::velocityForAxis(0.0, 100.0, config));
    require(inner < middle && middle < edge, "edge speed is not progressive");
    near(edge, config.maximumSpeed);
    near(std::abs(DragEdgeScroll::velocityForAxis(-100.0, 100.0, config)), config.maximumSpeed);
    near(std::abs(DragEdgeScroll::velocityForAxis(200.0, 100.0, config)), config.maximumSpeed);
}

void testAxesAndElapsedTime()
{
    DragEdgeScroll::Config config;
    const auto horizontal = DragEdgeScroll::velocity(0.0, 0.0, 100.0, 100.0, {true, false}, config);
    require(horizontal.x < 0.0 && horizontal.y == 0.0, "disabled vertical axis moved");

    const auto first = DragEdgeScroll::advance({100.0, -50.0}, 0.01, config);
    near(first.x, 1.0);
    near(first.y, -0.5);

    const auto capped = DragEdgeScroll::advance({100.0, -50.0}, 1.0, config);
    near(capped.x, 100.0 * config.maximumStepSeconds);
    near(capped.y, -50.0 * config.maximumStepSeconds);
    near(DragEdgeScroll::advance({100.0, 100.0}, -1.0, config).x, 0.0);
}

void testSmallAndInvalidViewports()
{
    DragEdgeScroll::Config config;
    require(DragEdgeScroll::velocityForAxis(1.0, 10.0, config) < 0.0, "small viewport lost its left edge");
    require(DragEdgeScroll::velocityForAxis(9.0, 10.0, config) > 0.0, "small viewport lost its right edge");
    near(DragEdgeScroll::velocityForAxis(0.0, 0.0, config), 0.0);
    near(DragEdgeScroll::velocityForAxis(std::numeric_limits<double>::quiet_NaN(), 100.0, config), 0.0);
}

void testInteractionClamp()
{
    near(DragEdgeScroll::clampInteractionPosition(-20.0, 100.0), 1.0);
    near(DragEdgeScroll::clampInteractionPosition(40.0, 100.0), 40.0);
    near(DragEdgeScroll::clampInteractionPosition(120.0, 100.0), 99.0);
    near(DragEdgeScroll::clampInteractionPosition(5.0, 6.0, 10.0), 3.0);
}
} // namespace

int main()
{
    try
    {
        testQuietAreaAndDirections();
        testProgressiveAndBoundedSpeed();
        testAxesAndElapsedTime();
        testSmallAndInvalidViewports();
        testInteractionClamp();
        std::cout << "All drag edge scroll tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Drag edge scroll test failed: " << error.what() << '\n';
        return 1;
    }
}
