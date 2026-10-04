#pragma once

#include <JuceHeader.h>
#include <tracktion_engine/tracktion_engine.h>

// One graph drag is one automation gesture and one undoable change. The knob
// remains on the existing continuous input path.
class PitchShiftDrag
{
public:
    PitchShiftDrag(tracktion_engine::Plugin::Ptr, juce::UndoManager &);
    ~PitchShiftDrag();

    bool begin();
    void update(float semitones);
    void finish();
    void cancel();

private:
    tracktion_engine::Plugin::Ptr m_plugin;
    tracktion_engine::AutomatableParameter::Ptr m_parameter;
    juce::UndoManager &m_undoManager;
    float m_previousValue = 0.0f;
    bool m_active = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchShiftDrag)
};
