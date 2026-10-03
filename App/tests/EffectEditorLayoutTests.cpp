#include "EffectEditorLayout.h"

#include <array>
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

bool isInside(EffectEditorLayout::Rectangle inner, int width, int height)
{
    return inner.x >= 0 && inner.y >= 0 && inner.right() <= width && inner.bottom() <= height;
}

bool overlaps(EffectEditorLayout::Rectangle a, EffectEditorLayout::Rectangle b)
{
    return a.x < b.right() && b.x < a.right() && a.y < b.bottom() && b.y < a.bottom();
}

template <size_t Size>
void requirePairwiseDisjoint(const std::array<EffectEditorLayout::Rectangle, Size> &rectangles, const std::string &name)
{
    for (size_t i = 0; i < rectangles.size(); ++i)
        for (size_t j = i + 1; j < rectangles.size(); ++j)
            require(!overlaps(rectangles[i], rectangles[j]), name + " cells overlap");
}

void testCompressorLayout(int width, int height, int minimumCellWidth, int minimumCellHeight)
{
    const auto layout = EffectEditorLayout::compressor(width, height);

    require(isInside(layout.graph, width, height), "compressor graph remains inside the editor");
    require(layout.graph.width >= 140 && layout.graph.width <= 210, "compressor graph uses its bounded width");

    for (const auto parameter : layout.parameters)
    {
        require(isInside(parameter, width, height), "compressor parameter remains inside the editor");
        require(!overlaps(layout.graph, parameter), "compressor graph and parameters are disjoint");
        require(parameter.width >= minimumCellWidth, "compressor parameter has sufficient width");
        require(parameter.height >= minimumCellHeight, "compressor parameter has sufficient height");
    }

    requirePairwiseDisjoint(layout.parameters, "compressor parameter");

    const std::array footerItems{layout.sidechainLabel, layout.sidechainSource, layout.sidechainTrigger};
    for (const auto item : footerItems)
    {
        require(isInside(item, width, height), "compressor sidechain item remains inside the editor");
        require(!overlaps(layout.graph, item), "compressor graph and sidechain footer are disjoint");
        for (const auto parameter : layout.parameters)
            require(!overlaps(parameter, item), "compressor parameters and sidechain footer are disjoint");
    }
    requirePairwiseDisjoint(footerItems, "compressor sidechain");
}

void testDelayLayout(int width, int height, int minimumGraphWidth, int minimumDetailWidth, int minimumChoiceHeight, int minimumParameterHeight)
{
    const auto layout = EffectEditorLayout::delay(width, height);

    require(isInside(layout.graph, width, height), "delay graph remains inside the editor");
    require(layout.graph.width >= minimumGraphWidth && layout.graph.width <= 210, "delay graph preserves the requested control width");

    for (const auto choice : layout.choices)
    {
        require(isInside(choice, width, height), "delay choice remains inside the editor");
        require(!overlaps(layout.graph, choice), "delay graph and choices are disjoint");
        require(choice.height >= minimumChoiceHeight, "delay choice has sufficient height");
    }

    require(layout.choices[0].width >= 101, "delay Mode choice has enough width for its text");
    require(layout.choices[1].width >= 66, "delay Sync choice has enough width for its text");
    require(layout.choices[2].width >= 71, "delay Division choice has enough width for its text");

    for (const auto parameter : layout.parameters)
    {
        require(isInside(parameter, width, height), "delay parameter remains inside the editor");
        require(!overlaps(layout.graph, parameter), "delay graph and parameters are disjoint");
        require(parameter.height >= minimumParameterHeight, "delay parameter has sufficient height");
    }

    for (size_t i = 3; i < layout.parameters.size(); ++i)
        require(layout.parameters[i].width >= minimumDetailWidth, "delay detail parameter has sufficient width");

    requirePairwiseDisjoint(layout.choices, "delay choice");
    requirePairwiseDisjoint(layout.parameters, "delay parameter");
    for (const auto choice : layout.choices)
        for (const auto parameter : layout.parameters)
            require(!overlaps(choice, parameter), "delay choice and parameter rows are disjoint");
}
} // namespace

int main()
{
    testCompressorLayout(360, 240, 60, 90);
    testCompressorLayout(412, 300, 70, 115);
    testCompressorLayout(600, 360, 115, 140);

    testDelayLayout(360, 240, 98, 58, 60, 78);
    testDelayLayout(412, 300, 150, 58, 64, 106);
    testDelayLayout(600, 360, 210, 90, 64, 136);

    const auto wideCompressor = EffectEditorLayout::compressor(600, 360);
    const auto wideDelay = EffectEditorLayout::delay(600, 360);
    require(wideCompressor.graph.width == 210, "wide compressor graph stops growing at its maximum");
    require(wideDelay.graph.width == 210, "wide delay graph stops growing at its maximum");

    if (failures == 0)
        std::cout << "Effect editor layout tests passed\n";

    return failures == 0 ? 0 : 1;
}
