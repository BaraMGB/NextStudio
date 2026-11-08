#include "PluginWindowTitleBarLayout.h"

#include <iostream>
#include <string>

namespace
{
int failures = 0;

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool overlaps(juce::Rectangle<int> a, juce::Rectangle<int> b)
{
    return a.intersects(b);
}

void checkLayout(juce::Rectangle<int> titleBar, juce::Rectangle<int> closeButton, bool controlsOnLeft)
{
    const auto layout = PluginWindowTitleBarLayout::calculate(titleBar, closeButton);
    require(layout.controlsOnLeft == controlsOnLeft, "control side is detected");
    require(titleBar.contains(layout.pinButton), "pin remains inside the title bar");
    require(titleBar.contains(layout.titleSpace), "title space remains inside the title bar");
    require(!overlaps(layout.pinButton, closeButton), "pin does not overlap the close button");
    require(!overlaps(layout.pinButton, layout.titleSpace), "pin does not overlap the title space");

    if (controlsOnLeft)
        require(layout.pinButton.getX() >= closeButton.getRight(), "left-side pin follows the close button");
    else
        require(layout.pinButton.getRight() <= closeButton.getX(), "right-side pin precedes the close button");
}
} // namespace

int main()
{
    checkLayout({0, 0, 480, 26}, {456, 4, 18, 18}, false);
    checkLayout({0, 0, 480, 26}, {6, 4, 18, 18}, true);
    checkLayout({2, 3, 100, 24}, {80, 6, 18, 18}, false);
    checkLayout({2, 3, 100, 24}, {6, 6, 18, 18}, true);
    checkLayout({0, 0, 320, 38}, {286, 7, 24, 24}, false);

    const auto noClose = PluginWindowTitleBarLayout::calculate({0, 0, 160, 26}, {});
    require(!noClose.controlsOnLeft, "missing close button uses right-side placement");
    require(noClose.pinButton.getRight() <= 154, "fallback pin respects the outer margin");

    const auto empty = PluginWindowTitleBarLayout::calculate({}, {});
    require(empty.pinButton.isEmpty(), "empty title bar produces no pin bounds");
    require(empty.titleSpace.isEmpty(), "empty title bar produces no title space");

    if (failures == 0)
        std::cout << "All plugin-window title-bar layout tests passed.\n";

    return failures == 0 ? 0 : 1;
}
