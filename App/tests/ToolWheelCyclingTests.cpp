#include "ToolWheelCycling.h"

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
enum class TestTool
{
    pointer,
    draw,
    range,
    eraser,
    knife,
    lasso,
    timestretch
};

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void testMidiOrder()
{
    constexpr std::array order{TestTool::pointer, TestTool::draw, TestTool::range,
                               TestTool::eraser, TestTool::knife, TestTool::lasso};

    require(ToolWheelCycling::cycle(TestTool::pointer, order, 1) == TestTool::draw,
            "MIDI forward cycle failed");
    require(ToolWheelCycling::cycle(TestTool::lasso, order, 1) == TestTool::pointer,
            "MIDI forward wrap failed");
    require(ToolWheelCycling::cycle(TestTool::pointer, order, -1) == TestTool::lasso,
            "MIDI backward wrap failed");
    require(ToolWheelCycling::cycle(TestTool::timestretch, order, 1) == TestTool::pointer,
            "MIDI unknown-tool fallback failed");
}

void testSongOrder()
{
    constexpr std::array order{TestTool::pointer, TestTool::lasso, TestTool::range,
                               TestTool::knife, TestTool::timestretch};

    require(ToolWheelCycling::cycle(TestTool::pointer, order, 1) == TestTool::lasso,
            "Song forward cycle failed");
    require(ToolWheelCycling::cycle(TestTool::timestretch, order, 1) == TestTool::pointer,
            "Song forward wrap failed");
    require(ToolWheelCycling::cycle(TestTool::pointer, order, -1) == TestTool::timestretch,
            "Song backward wrap failed");
    require(ToolWheelCycling::cycle(TestTool::draw, order, -1) == TestTool::timestretch,
            "Song unknown-tool fallback failed");
}

void testSteppedWheel()
{
    ToolWheelCycling::Accumulator accumulator;
    require(accumulator.update(0.01f, false, false, 100) == 1,
            "positive stepped wheel did not advance");
    require(accumulator.update(-0.01f, false, false, 110) == -1,
            "negative stepped wheel did not reverse");
    require(accumulator.update(0.0f, false, false, 120) == 0,
            "zero wheel delta produced a step");
    require(accumulator.update(std::numeric_limits<float>::quiet_NaN(), false, false, 130) == 0,
            "non-finite wheel delta produced a step");
}

void testSmoothWheelAccumulation()
{
    ToolWheelCycling::Accumulator accumulator;
    require(accumulator.update(0.11f, true, false, 100) == 0,
            "small smooth delta stepped too early");
    require(accumulator.update(0.11f, true, false, 110) == 1,
            "accumulated smooth delta did not advance");
    require(accumulator.update(0.8f, true, false, 120) == 1,
            "large smooth event did not produce exactly one step");
    require(accumulator.update(0.01f, true, false, 130) == 0,
            "large smooth event retained extra steps");
}

void testSmoothWheelResetAndInertia()
{
    ToolWheelCycling::Accumulator accumulator;
    require(accumulator.update(0.15f, true, false, 100) == 0,
            "initial smooth delta stepped too early");
    require(accumulator.update(-0.11f, true, false, 110) == 0,
            "direction reversal retained the previous delta");
    require(accumulator.update(-0.11f, true, false, 120) == -1,
            "reversed smooth deltas did not accumulate");

    accumulator.reset();
    require(accumulator.update(0.15f, true, false, 200) == 0,
            "smooth timeout setup stepped too early");
    require(accumulator.update(0.10f, true, false, 500) == 0,
            "smooth delta survived the idle timeout");
    require(accumulator.update(0.3f, true, true, 510) == 0,
            "inertial event produced a step");
}
} // namespace

int main()
{
    try
    {
        testMidiOrder();
        testSongOrder();
        testSteppedWheel();
        testSmoothWheelAccumulation();
        testSmoothWheelResetAndInertia();
        std::cout << "All tool wheel cycling tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Tool wheel cycling test failed: " << error.what() << '\n';
        return 1;
    }
}
