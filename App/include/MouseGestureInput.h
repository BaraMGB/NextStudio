#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>

// Re-evaluate the last raw position on modifier changes, without synthesizing
// pointer movement or losing the original floating-point/down coordinates.
class MouseGestureInput
{
public:
    void remember(const juce::MouseEvent& event)
    {
        if (!m_event || event.mouseDownTime != m_event->mouseDownTime)
            m_downScreenPosition = event.eventComponent ? event.eventComponent->localPointToGlobal(event.mouseDownPosition)
                                                         : event.mouseDownPosition;
        m_event.emplace(event);
    }
    void reset() { m_event.reset(); }
    std::optional<juce::MouseEvent> withModifiers(juce::ModifierKeys mods) const
    {
        if (!m_event)
            return {};
        const auto& e = *m_event;
        const juce::ModifierKeys merged((mods.getRawFlags() & ~juce::ModifierKeys::allMouseButtonModifiers)
                                      | (e.mods.getRawFlags() & juce::ModifierKeys::allMouseButtonModifiers));
        return juce::MouseEvent(e.source, e.position, merged, e.pressure, e.orientation, e.rotation, e.tiltX, e.tiltY,
                               e.eventComponent, e.originalComponent, juce::Time::getCurrentTime(), e.mouseDownPosition,
                               e.mouseDownTime, e.getNumberOfClicks(), e.mouseWasDraggedSinceMouseDown());
    }
    // Geometry/context replay reads the actual pointer through the NEW component
    // transform. Modifier-only replay above deliberately retains cached coordinates.
    std::optional<juce::MouseEvent> forContext(juce::Component& component, juce::ModifierKeys mods) const
    {
        const auto event = withModifiers(mods);
        if (!event)
            return {};
        const auto& e = *event;
        return juce::MouseEvent(e.source, component.getLocalPoint(nullptr, e.source.getScreenPosition()), e.mods,
            e.pressure, e.orientation, e.rotation, e.tiltX, e.tiltY, &component, e.originalComponent, e.eventTime,
            component.getLocalPoint(nullptr, m_downScreenPosition), e.mouseDownTime, e.getNumberOfClicks(), e.mouseWasDraggedSinceMouseDown());
    }
private:
    juce::Point<float> m_downScreenPosition;
    std::optional<juce::MouseEvent> m_event;
};
