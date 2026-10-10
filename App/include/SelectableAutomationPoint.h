#pragma once

#include <tracktion_engine/tracktion_engine.h>

// A gesture snapshot may retain a proxy after its lane removes the unselected
// entry. Retaining the parameter keeps the curve reference valid as well.
struct SelectableAutomationPoint : public tracktion::engine::Selectable, public juce::ReferenceCountedObject
{
    using Ptr = juce::ReferenceCountedObjectPtr<SelectableAutomationPoint>;
    SelectableAutomationPoint(int i, tracktion::engine::AutomationCurve& c)
        : index(i), m_curve(c), pointState(c.state.getChild(i)), parameter(c.getOwnerParameter()) {}
    ~SelectableAutomationPoint() override { notifyListenersOfDeletion(); }
    juce::String getSelectableDescription() override { return "AutomationPoint"; }

    int index = 0;
    tracktion::engine::AutomationCurve& m_curve;
    juce::ValueTree pointState;
    tracktion::engine::AutomatableParameter::Ptr parameter;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelectableAutomationPoint)
};
