#include "HeaderLayout.h"
#include "PositionDisplayInfoState.h"
#include "PositionDisplayLayout.h"
#include <iostream>

namespace
{
int failures = 0;
#define CHECK(condition) do { if (!(condition)) { std::cerr << __LINE__ << ": " #condition "\n"; ++failures; } } while (false)

void feedback()
{
    PositionDisplayInfoState state;
    CHECK(!state.isSavedVisible() && !state.isPending());
    CHECK(!state.expire(100000));
    state.saved(100);
    CHECK(state.isSavedVisible() && !state.isPending());
    CHECK(!state.expire(3099));
    CHECK(state.expire(3100));
    CHECK(!state.isSavedVisible());

    state.saved(4000);
    state.saved(5000);
    CHECK(!state.expire(7000)); // A delayed callback cannot hide a newer save.
    CHECK(state.isSavedVisible());
    CHECK(state.expire(8000));

    state.setBlocked(true, 9000);
    state.saved(9100);
    CHECK(state.isPending() && !state.isSavedVisible());
    CHECK(!state.expire(20000)); // Save As time does not consume display time.
    state.setBlocked(true, 21000);
    CHECK(state.isPending());
    state.setBlocked(false, 22000);
    CHECK(state.isSavedVisible() && !state.isPending());
    state.setBlocked(false, 24000);
    CHECK(!state.expire(24999));
    CHECK(state.expire(25000));

    state.saved(26000);
    state.setBlocked(true, 27000); // A new workflow discards old success.
    CHECK(!state.isSavedVisible() && !state.isPending());
    state.setBlocked(false, 28000); // Cancellation cannot resurrect it.
    CHECK(!state.isSavedVisible());

    state.setBlocked(true, 29000);
    state.saved(30000);
    state.clear(); // Project replacement/teardown clears pending success.
    state.setBlocked(false, 31000);
    CHECK(!state.isSavedVisible() && !state.isPending());
}

void positionDisplayLayout()
{
    const auto checkInsideAndSeparate = [] (const PositionDisplayLayout::Layout &layout,
                                            juce::Rectangle<int> bounds)
    {
        for (const auto rectangle : {layout.info, layout.left, layout.center, layout.right})
        {
            CHECK(rectangle.getWidth() >= 0 && rectangle.getHeight() >= 0);
            CHECK(bounds.contains(rectangle));
        }
        CHECK(!layout.info.intersects(layout.center));
        CHECK(!layout.left.intersects(layout.center));
        CHECK(!layout.right.intersects(layout.center));
    };

    const auto full = PositionDisplayLayout::calculate({0, 0, 586, 46}, 90, 105);
    CHECK(full.mode == PositionDisplayLayout::Mode::full);
    CHECK(full.info.getWidth() == 180);
    CHECK(full.showsLabels() && full.showsSideFields());
    CHECK(full.center.getWidth() >= 130);
    checkInsideAndSeparate(full, {0, 0, 586, 46});

    // A 1200-pixel application window with the expanded sidebar leaves roughly
    // 770 pixels for the header and 476 for the Position Display.
    const auto narrowHeader = HeaderLayout::calculate(770, 60, 600);
    const auto compact = PositionDisplayLayout::calculate(narrowHeader.positionDisplay.withPosition(0, 0), 70, 85);
    CHECK(narrowHeader.positionDisplay.getWidth() == 476);
    CHECK(compact.mode == PositionDisplayLayout::Mode::compact);
    CHECK(compact.info.getWidth() == 120);
    CHECK(!compact.showsLabels() && compact.showsSideFields());
    CHECK(compact.center.getWidth() >= 140);
    checkInsideAndSeparate(compact, narrowHeader.positionDisplay.withPosition(0, 0));

    // At smaller supported sizes the side fields yield to position/time, but
    // save status remains present instead of collapsing to zero width.
    const auto smallHeader = HeaderLayout::calculate(470, 60, 600);
    const auto minimal = PositionDisplayLayout::calculate(smallHeader.positionDisplay.withPosition(0, 0), 70, 85);
    CHECK(smallHeader.positionDisplay.getWidth() == 176);
    CHECK(minimal.mode == PositionDisplayLayout::Mode::minimal);
    CHECK(!minimal.showsLabels() && !minimal.showsSideFields());
    CHECK(minimal.info.getWidth() >= 80);
    CHECK(minimal.center.getWidth() >= 64);
    CHECK(minimal.left.isEmpty() && minimal.right.isEmpty());
    checkInsideAndSeparate(minimal, smallHeader.positionDisplay.withPosition(0, 0));

    const auto tiny = PositionDisplayLayout::calculate({0, 0, 60, 46}, 70, 85);
    CHECK(!tiny.info.isEmpty());
    CHECK(tiny.center.isEmpty());
    checkInsideAndSeparate(tiny, {0, 0, 60, 46});
}

void layout()
{
    for (int height : {0, 30, 60, 90, 120})
        for (int width = 0; width <= 2400; ++width)
        {
            const auto geometry = HeaderLayout::calculate(width, height, 600);
            const juce::Rectangle<int> bounds(0, 0, width, height);
            const auto inside = [&bounds](juce::Rectangle<int> rectangle)
            {
                return rectangle.getWidth() >= 0 && rectangle.getHeight() >= 0
                    && rectangle.getX() >= 0 && rectangle.getY() >= 0
                    && rectangle.getRight() <= bounds.getRight()
                    && rectangle.getBottom() <= bounds.getBottom();
            };
            CHECK(inside(geometry.positionDisplay));
            for (size_t i = 0; i < geometry.buttons.size(); ++i)
            {
                CHECK(inside(geometry.buttons[i]));
                CHECK(!geometry.buttons[i].intersects(geometry.positionDisplay));
                for (size_t j = 0; j < i; ++j)
                    CHECK(!geometry.buttons[i].intersects(geometry.buttons[j]));
            }
            CHECK(std::abs(geometry.positionDisplay.getCentreX() - width / 2) <= 1);
        }

    for (int width : {900, 1170, 1270, 1500})
    {
        const auto geometry = HeaderLayout::calculate(width, 60, 600);
        CHECK(geometry.positionDisplay.getWidth() >= 580); // Outer header gaps.
        for (const auto &button : geometry.buttons)
            CHECK(button.getWidth() >= 20 && button.getHeight() == 46);
    }

    const auto normal = HeaderLayout::calculate(1170, 60, 600);
    const auto afterStatusChange = HeaderLayout::calculate(1170, 60, 600);
    CHECK(normal.positionDisplay == afterStatusChange.positionDisplay);
    CHECK(normal.buttons == afterStatusChange.buttons);
}
}

int main()
{
    feedback();
    positionDisplayLayout();
    layout();
    if (failures == 0)
        std::cout << "Header presentation tests passed\n";
    return failures == 0 ? 0 : 1;
}
