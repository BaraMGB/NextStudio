#include "SidebarLayout.h"
#include "SplitterCollapseController.h"

#include <iostream>

namespace
{
int failures = 0;

void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        ++failures;
    }
}

void testDragStartingExpanded()
{
    SplitterCollapseController controller;
    controller.beginDrag(false, 230);

    require(!controller.startedCollapsed(), "expanded drag remembers its initial state");
    require(!controller.getCollapsedState(229, false), "expanded panel resists before collapsed position");
    require(controller.getCollapsedState(230, false), "expanded panel collapses at collapsed position");
    require(controller.getCollapsedState(1, true), "collapsed panel remains collapsed while reversing");
    require(!controller.getCollapsedState(0, true), "collapsed panel reopens at original position");
}

void testDragStartingCollapsed()
{
    SplitterCollapseController controller;
    controller.beginDrag(true, 230);

    require(controller.startedCollapsed(), "collapsed drag remembers its initial state");
    require(controller.getCollapsedState(-229, true), "collapsed panel resists before expanded position");
    require(!controller.getCollapsedState(-230, true), "collapsed panel opens at expanded position");
    require(!controller.getCollapsedState(-1, false), "expanded panel remains open while reversing");
    require(controller.getCollapsedState(0, false), "expanded panel collapses at original position");
}

void testZeroTransitionDistance()
{
    SplitterCollapseController controller;
    controller.beginDrag(false, -10);
    require(controller.getCollapsedState(0, false), "negative transition distance is clamped to zero");
}

void testSidebarLayoutPolicy()
{
    require(SidebarLayout::getPreferredWidth(70) == SidebarLayout::defaultExpandedWidth,
            "invalid stored width uses default");
    require(SidebarLayout::getPreferredWidth(600) == 600, "valid stored width is preserved");
    require(SidebarLayout::getTransitionDistance(600, false) == 450, "expanded transition includes resize and resistance");
    require(SidebarLayout::getTransitionDistance(600, true) == 530, "collapsed transition reaches stored expanded width");
    require(SidebarLayout::getResizedWidth(600, -200) == 400, "wide sidebar can shrink continuously");
    require(SidebarLayout::getResizedWidth(600, -500) == 250, "sidebar resize clamps to minimum");
    require(SidebarLayout::getResizedWidth(600, 100) == 700, "sidebar can grow freely");
}
} // namespace

int main()
{
    testDragStartingExpanded();
    testDragStartingCollapsed();
    testZeroTransitionDistance();
    testSidebarLayoutPolicy();

    if (failures != 0)
        return 1;

    std::cout << "Splitter collapse controller tests passed\n";
    return 0;
}
