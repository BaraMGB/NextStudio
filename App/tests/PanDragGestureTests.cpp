#include "PanDragGesture.h"

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

void testDiagonalPan()
{
    PanDragGesture gesture;
    require(gesture.begin({100.0, 200.0}, {20.0, 40.0}, {0.25, 1.0}, {0.0, 100.0}, {0.0, 200.0}),
            "valid gesture did not begin");

    const auto position = gesture.update({120.0, 185.0});
    require(position.has_value(), "active gesture did not update");
    near(position->x, 15.0);
    near(position->y, 55.0);
}

void testPianoPitchDirectionAndIndependentAxes()
{
    PanDragGesture gesture;
    require(gesture.begin({10.0, 10.0}, {8.0, 30.0}, {0.5, -0.1}, {0.0, 100.0}, {0.0, 120.0}),
            "piano gesture did not begin");

    auto position = gesture.update({10.0, 30.0});
    near(position->x, 8.0);
    near(position->y, 32.0);

    position = gesture.update({4.0, 10.0});
    near(position->x, 11.0);
    near(position->y, 30.0);
}

void testClampingAndReverseMovement()
{
    PanDragGesture gesture;
    require(gesture.begin({50.0, 50.0}, {5.0, 95.0}, {1.0, 1.0}, {0.0, 100.0}, {0.0, 100.0}),
            "bounded gesture did not begin");

    auto position = gesture.update({100.0, 0.0});
    near(position->x, 0.0);
    near(position->y, 100.0);

    position = gesture.update({40.0, 60.0});
    near(position->x, 15.0);
    near(position->y, 85.0);
}

void testInvalidAndCancelledGestures()
{
    PanDragGesture gesture;
    require(!gesture.update({0.0, 0.0}).has_value(), "inactive gesture updated");
    require(!gesture.begin({0.0, 0.0}, {0.0, 0.0}, {1.0, 1.0}, {2.0, 1.0}, {0.0, 1.0}),
            "reversed bounds were accepted");
    require(!gesture.begin({0.0, 0.0}, {0.0, 0.0}, {std::numeric_limits<double>::quiet_NaN(), 1.0},
                           {0.0, 1.0}, {0.0, 1.0}),
            "non-finite scale was accepted");
    require(gesture.begin({0.0, 0.0}, {0.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}),
            "valid gesture did not begin after invalid input");
    gesture.cancel();
    require(!gesture.active() && !gesture.update({1.0, 1.0}).has_value(), "cancelled gesture remained active");
}
} // namespace

int main()
{
    try
    {
        testDiagonalPan();
        testPianoPitchDirectionAndIndependentAxes();
        testClampingAndReverseMovement();
        testInvalidAndCancelledGestures();
        std::cout << "All pan drag gesture tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Pan drag gesture test failed: " << error.what() << '\n';
        return 1;
    }
}
