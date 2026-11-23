#include "NextLookAndFeel.h"

namespace
{
using Cursor = juce::MouseCursor;
struct CursorArtwork
{
    Cursor::StandardCursorType role;
    const char *svg;
    juce::Point<int> hotspot {12, 12};
};

const CursorArtwork artwork[] = {
    {Cursor::NormalCursor, BinaryData::cursorArrow_svg, {3, 2}},
    {Cursor::WaitCursor, BinaryData::cursorArrow_svg, {3, 2}},
    {Cursor::IBeamCursor, BinaryData::cursorText_svg},
    {Cursor::CrosshairCursor, BinaryData::cursorCrosshair_svg},
    {Cursor::CopyingCursor, BinaryData::shiftHandCursor_svg},
    {Cursor::PointingHandCursor, BinaryData::cursorArrow_svg, {3, 2}},
    {Cursor::DraggingHandCursor, BinaryData::shiftHandCursor_svg},
    {Cursor::LeftRightResizeCursor, BinaryData::cursorHorizontal_svg},
    {Cursor::UpDownResizeCursor, BinaryData::cursorVertical_svg},
    {Cursor::UpDownLeftRightResizeCursor, BinaryData::shiftHandCursor_svg},
    {Cursor::TopEdgeResizeCursor, BinaryData::cursorEdgeTop_svg},
    {Cursor::BottomEdgeResizeCursor, BinaryData::cursorEdgeBottom_svg},
    {Cursor::LeftEdgeResizeCursor, BinaryData::cursorEdgeLeft_svg},
    {Cursor::RightEdgeResizeCursor, BinaryData::cursorEdgeRight_svg},
    {Cursor::TopLeftCornerResizeCursor, BinaryData::cursorDiagonalDown_svg},
    {Cursor::BottomRightCornerResizeCursor, BinaryData::cursorDiagonalDown_svg},
    {Cursor::TopRightCornerResizeCursor, BinaryData::cursorDiagonalUp_svg},
    {Cursor::BottomLeftCornerResizeCursor, BinaryData::cursorDiagonalUp_svg},
};
static_assert(std::size(artwork) + 2 == Cursor::NumStandardCursorTypes); // Parent and NoCursor have no artwork.
}

juce::MouseCursor NextLookAndFeel::getMouseCursorFor(juce::Component &component)
{
    // Resolve inheritance before translating roles, including those requested by JUCE widgets.
    auto cursor = LookAndFeel_V4::getMouseCursorFor(component);
    if (cursor == Cursor::ParentCursor)
        cursor = Cursor::NormalCursor;

    for (const auto &entry : artwork)
    {
        if (cursor != entry.role)
            continue;

        const auto scale = juce::jlimit(0.2f, 3.f, m_appState.m_mouseCursorScale.get());
        if (scale != m_cursorCacheScale)
        {
            m_cursorCache.fill(std::nullopt);
            m_cursorCacheScale = scale;
        }
        auto &cached = m_cursorCache[entry.role];
        if (!cached)
            cached = GUIHelpers::getMouseCursorFromSvg(entry.svg, entry.hotspot, scale);
        return *cached;
    }
    return cursor; // Preserve intentional hiding and existing custom tool cursors.
}
