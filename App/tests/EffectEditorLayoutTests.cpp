#include "EffectEditorLayout.h"
#include "PitchShiftDisplay.h"

#include <array>
#include <cmath>
#include <limits>
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

void testPitchShifterLayout(int width, int height)
{
    const auto layout = EffectEditorLayout::pitchShifter(width, height);
    require(isInside(layout.graph, width, height), "pitch map remains inside the editor");
    require(isInside(layout.parameter, width, height), "pitch parameter remains inside the editor");
    require(!overlaps(layout.graph, layout.parameter), "pitch graph and parameter are disjoint");
    require(layout.graph.width >= 78 && layout.graph.height >= 90, "pitch map preserves readable scale space");
    require(layout.parameter.width >= 78 && layout.parameter.height >= 110, "pitch knob and value remain readable");
    require(layout.graph.width <= 160 && layout.graph.height <= 150, "pitch map growth is bounded");
    require(layout.parameter.height <= 135, "pitch knob growth is bounded");
    require(layout.graph.x == layout.parameter.x && layout.graph.width == layout.parameter.width, "pitch graph and knob align");
    require(std::abs(layout.graph.x - (width - layout.graph.width) / 2) <= 1, "pitch content is horizontally centred");
    require(std::abs(layout.graph.y - (height - (layout.parameter.bottom() - layout.graph.y)) / 2) <= 1, "pitch content is vertically centred");
}

void testPitchShiftPositions()
{
    constexpr float maximum = 24.0f;
    require(PitchShiftDisplay::positionForSemitones(24.0f, maximum) == 0.0f, "positive two octaves is at the top");
    require(PitchShiftDisplay::positionForSemitones(12.0f, maximum) == 0.25f, "positive octave tick aligns");
    require(PitchShiftDisplay::positionForSemitones(0.0f, maximum) == 0.5f, "original pitch is at the centre");
    require(PitchShiftDisplay::positionForSemitones(-12.0f, maximum) == 0.75f, "negative octave tick aligns");
    require(PitchShiftDisplay::positionForSemitones(-24.0f, maximum) == 1.0f, "negative two octaves is at the bottom");
    require(PitchShiftDisplay::positionForSemitones(48.0f, maximum) == 0.0f, "positive visual position is clamped");
    require(PitchShiftDisplay::positionForSemitones(-48.0f, maximum) == 1.0f, "negative visual position is clamped");
    require(std::abs(PitchShiftDisplay::positionForSemitones(0.5f, maximum) - (0.5f - 0.5f / 48.0f)) < 0.00001f, "fractional shift is not rounded");
    require(PitchShiftDisplay::positionForSemitones(0.5f, maximum) < PitchShiftDisplay::positionForSemitones(0.0f, maximum), "fractional positive shift moves upward");
    require(PitchShiftDisplay::positionForSemitones(1.0f, 0.0f) == 0.5f, "invalid range has a neutral position");
    require(PitchShiftDisplay::positionForSemitones(std::numeric_limits<float>::quiet_NaN(), maximum) == 0.5f, "invalid value has a neutral position");
}

void testSnappedPitchDragPositions()
{
    constexpr float maximum = 24.0f;
    constexpr float height = 96.0f; // two pixels per semitone
    require(PitchShiftDisplay::snappedSemitonesForDrag(0.0f, -24.0f, height, maximum) == 12.0f, "upward graph drag raises pitch by whole semitones");
    require(PitchShiftDisplay::snappedSemitonesForDrag(0.0f, 24.0f, height, maximum) == -12.0f, "downward graph drag lowers pitch by whole semitones");
    require(PitchShiftDisplay::snappedSemitonesForDrag(0.0f, -0.9f, height, maximum) == 0.0f, "less than half a positive semitone stays at zero");
    require(PitchShiftDisplay::snappedSemitonesForDrag(0.0f, -1.1f, height, maximum) == 1.0f, "positive drag rounds to nearest semitone");
    require(PitchShiftDisplay::snappedSemitonesForDrag(0.0f, 1.1f, height, maximum) == -1.0f, "negative drag rounds symmetrically");
    require(PitchShiftDisplay::snappedSemitonesForDrag(0.25f, -2.0f, height, maximum) == 1.0f, "fractional starting value snaps only on graph movement");
    require(PitchShiftDisplay::snappedSemitonesForDrag(12.0f, -1000.0f, height, maximum) == 24.0f, "graph drag clamps at the upper limit");
    require(PitchShiftDisplay::snappedSemitonesForDrag(-12.0f, 1000.0f, height, maximum) == -24.0f, "graph drag clamps at the lower limit");
    require(PitchShiftDisplay::snappedSemitonesForDrag(0.25f, 10.0f, 0.0f, maximum) == 0.25f, "invalid drag geometry preserves the value");
    for (int semitones = -24; semitones <= 24; ++semitones)
        require(PitchShiftDisplay::snappedSemitonesForDrag(0.0f, static_cast<float>(-semitones * 2), height, maximum) == static_cast<float>(semitones), "every whole semitone is reachable by dragging");
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
    require(EffectEditorLayout::pitchShifterWidthFactor == 1, "pitch editor requests compact rack width");
    testPitchShifterLayout(86, 240);
    testPitchShifterLayout(110, 290);
    testPitchShifterLayout(200, 360);
    testPitchShiftPositions();
    testSnappedPitchDragPositions();
    for (const int width : {0, 1, 10, 50, 86, 110, 200})
        for (const int height : {0, 1, 10, 50, 100, 240, 360})
        {
            const auto layout = EffectEditorLayout::pitchShifter(width, height);
            require(isInside(layout.graph, width, height) && isInside(layout.parameter, width, height), "small pitch rectangles remain in bounds");
            require(!overlaps(layout.graph, layout.parameter), "small pitch rectangles do not overlap");
        }

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
