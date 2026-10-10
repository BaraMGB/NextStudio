#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SelectionIdentity.h"
#include <algorithm>
#include <cmath>

// Coordinates belong to the source: project beats + pitch, velocity or lane position.
// No view state, model mutation, snapping or selection ownership lives here.
class LassoGesture
{
public:
    void begin(juce::Point<double> anchor) { m_anchor = m_end = anchor; m_active = true; m_dragged = false; }
    void update(juce::Point<double> end) { if (m_active) { m_end = end; m_dragged = true; } }
    void end() { m_active = m_dragged = false; }
    bool active() const { return m_active; }
    bool dragged() const { return m_dragged; }
    juce::Point<double> anchor() const { return m_anchor; }
    juce::Point<double> endPoint() const { return m_end; }
    juce::Rectangle<double> bounds() const { return {m_anchor, m_end}; }
    template <typename Project>
    juce::Rectangle<float> viewBounds(Project project) const { return {project(m_anchor), project(m_end)}; }
    static bool containsCentre(juce::Rectangle<float> bounds, juce::Point<float> point)
    {
        return !bounds.isEmpty() && point.x >= bounds.getX() && point.x <= bounds.getRight()
            && point.y >= bounds.getY() && point.y <= bounds.getBottom();
    }
private:
    juce::Point<double> m_anchor, m_end;
    bool m_active = false, m_dragged = false;
};

// A range is a resolved musical interval and a set of vertical lanes, not a lasso.
// The caller resolves snapping before passing the horizontal coordinate.
class TimeRangeGesture
{
public:
    void begin(double position, double lane) { m_start = m_end = position; m_laneStart = m_laneEnd = lane; m_active = true; }
    void update(double position, double lane) { if (m_active) { m_end = position; m_laneEnd = lane; } }
    void end() { m_active = false; }
    bool active() const { return m_active; }
    double anchor() const { return m_start; }
    juce::Range<double> interval() const { return {std::min(m_start, m_end), std::max(m_start, m_end)}; }
    juce::Range<double> lanes() const { return {std::min(m_laneStart, m_laneEnd), std::max(m_laneStart, m_laneEnd)}; }
private:
    double m_start = 0, m_end = 0, m_laneStart = 0, m_laneEnd = 0;
    bool m_active = false;
};

enum class LassoSelectionMode { replace, add, toggle };
inline LassoSelectionMode lassoSelectionMode(juce::ModifierKeys modifiers)
{
    if (modifiers.isShiftDown()) return LassoSelectionMode::add;
    if (modifiers.isCtrlDown() || modifiers.isCommandDown()) return LassoSelectionMode::toggle;
    return LassoSelectionMode::replace;
}

// Always combine against the gesture-start snapshot, never against last frame's hits.
template <typename Item>
juce::Array<Item> combineLassoSelection(const juce::Array<Item>& original,
                                        const juce::Array<Item>& hits, LassoSelectionMode mode)
{
    const SelectionIdentitySet<Item> originalSet(original.begin(), original.end());
    const SelectionIdentitySet<Item> hitSet(hits.begin(), hits.end());
    SelectionIdentitySet<Item> emitted;
    emitted.reserve(size_t(original.size() + hits.size()));
    juce::Array<Item> result;
    result.ensureStorageAllocated(original.size() + hits.size());
    auto append = [&](const Item& item) { if (emitted.insert(item).second) result.add(item); };
    if (mode != LassoSelectionMode::replace)
        for (const auto& item : original)
            if (mode != LassoSelectionMode::toggle || !hitSet.contains(item)) append(item);
    for (const auto& item : hits)
        if (mode != LassoSelectionMode::toggle || !originalSet.contains(item)) append(item);
    return result;
}
