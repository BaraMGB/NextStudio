#include "LowerRangeLayout.h"
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

void testLowerRangeLayoutPolicy()
{
    constexpr int maximumExpandedHeight = 780;

    require(LowerRangeLayout::getPreferredExpandedHeight(200) == LowerRangeLayout::defaultExpandedHeight,
            "invalid stored piano roll height uses standard expanded height");
    require(LowerRangeLayout::getPreferredExpandedHeight(500) == 500,
            "valid stored piano roll height is preserved");
    require(LowerRangeLayout::getMinimumEditorContainerHeight(50) == 200,
            "song editor timeline boundary defines the minimum editor container height");
    require(LowerRangeLayout::getMaximumExpandedHeight(1000, 50) == maximumExpandedHeight,
            "maximum piano roll height stops at the song editor timeline boundary");
    require(LowerRangeLayout::clampExpandedHeight(900, maximumExpandedHeight) == maximumExpandedHeight,
            "piano roll height is clamped to the maximum expanded height");
    require(LowerRangeLayout::getTransitionDistance(500, false) == 462,
            "expanded piano roll transition includes resize back to the standard height");
    require(LowerRangeLayout::getTransitionDistance(500, true) == 462,
            "collapsed piano roll transition reopens to the stored expanded height");
    require(LowerRangeLayout::getResizedHeight(500, -120, maximumExpandedHeight) == 620,
            "piano roll can continue growing while dragging upward");
    require(LowerRangeLayout::getResizedHeight(500, 120, maximumExpandedHeight) == 380,
            "piano roll can shrink continuously after being enlarged");
    require(LowerRangeLayout::getResizedHeight(500, 200, maximumExpandedHeight) == LowerRangeLayout::defaultExpandedHeight,
            "piano roll shrink clamps to the standard expanded height before collapse");
    require(LowerRangeLayout::getResizedHeight(700, -200, maximumExpandedHeight) == maximumExpandedHeight,
            "piano roll growth clamps at the maximum expanded height");
    require(LowerRangeLayout::getAppliedDragDistance(500, -120, maximumExpandedHeight) == -120,
            "applied piano roll drag matches upward resize distance");
    require(LowerRangeLayout::getAppliedDragDistance(500, 120, maximumExpandedHeight) == 120,
            "applied piano roll drag matches downward resize distance before clamp");
    require(LowerRangeLayout::getAppliedDragDistance(500, 200, maximumExpandedHeight) == 150,
            "applied piano roll drag stops growing once the standard expanded height is reached");
    require(LowerRangeLayout::getAppliedDragDistance(700, -200, maximumExpandedHeight) == -80,
            "applied piano roll drag stops growing once the maximum expanded height is reached");
}

void testLowerRangeExpandedDragSequence()
{
    constexpr int maximumExpandedHeight = 780;

    SplitterCollapseController controller;
    controller.beginDrag(false, LowerRangeLayout::getTransitionDistance(500, false));

    require(!controller.getCollapsedState(149, false), "enlarged piano roll stays expanded while shrinking back toward default height");
    require(LowerRangeLayout::getResizedHeight(500, 149, maximumExpandedHeight) == 351,
            "enlarged piano roll still resizes one pixel above default height");
    require(!controller.getCollapsedState(150, false), "enlarged piano roll does not collapse when it reaches default height");
    require(LowerRangeLayout::getResizedHeight(500, 150, maximumExpandedHeight) == LowerRangeLayout::defaultExpandedHeight,
            "enlarged piano roll reaches the default expanded height before collapse resistance starts");
    require(!controller.getCollapsedState(461, false), "enlarged piano roll remains expanded throughout the collapse resistance zone");
    require(controller.getCollapsedState(462, false), "enlarged piano roll collapses only after the full transition distance is crossed");
}
} // namespace

int main()
{
    testDragStartingExpanded();
    testDragStartingCollapsed();
    testZeroTransitionDistance();
    testSidebarLayoutPolicy();
    testLowerRangeLayoutPolicy();
    testLowerRangeExpandedDragSequence();

    if (failures != 0)
        return 1;

    std::cout << "Splitter collapse controller tests passed\n";
    return 0;
}
