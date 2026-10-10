#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>

// Re-evaluate the last raw position on modifier changes, without synthesizing
// pointer movement or losing the original floating-point/down coordinates.
class MouseGestureInput
{
public:
    struct Identity
    {
        juce::Time mouseDownTime;
        int sourceIndex;
        bool matches(const juce::MouseEvent& event) const
        { return mouseDownTime == event.mouseDownTime && sourceIndex == event.source.getIndex(); }
    };
    void remember(const juce::MouseEvent& event)
    {
        if (!belongsToGesture(event))
            m_downScreenPosition = event.eventComponent ? event.eventComponent->localPointToGlobal(event.mouseDownPosition)
                                                         : event.mouseDownPosition;
        m_event.emplace(event);
    }
    void reset() { m_event.reset(); }
    bool belongsToGesture(const juce::MouseEvent& event) const
    {
        const auto key = identity();
        return key && key->matches(event);
    }
    std::optional<Identity> identity() const
    {
        if (!m_event) return {};
        return Identity{m_event->mouseDownTime, m_event->source.getIndex()};
    }
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

// A canceled selection can suppress only its own release, never the next edit.
class MouseGestureCancellation
{
public:
    void cancel(const MouseGestureInput& input)
    {
        if (auto key = input.identity()) m_identity = key;
    }
    bool consume(const juce::MouseEvent& event)
    {
        if (!m_identity || !m_identity->matches(event)) return false;
        m_identity.reset();
        return true;
    }
private:
    std::optional<MouseGestureInput::Identity> m_identity;
};
