/*
  ==============================================================================

    SpinBoxGroup.h
    Created: 26 Jan 2020 8:05:58pm
    Author:  BaraMGB

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SpinBox.h"

//==============================================================================
/*
*/
class SpinBoxGroup    : public Component
                      , public ChangeListener
{
public:
    SpinBoxGroup();
    ~SpinBoxGroup();

    void paint (Graphics& g) override;
    void resized() override;

    void changeListenerCallback(ChangeBroadcaster *source) override;

    void addSpinBox(int digits, int init, int min, int max, String separator, int step = 1);
    bool setValue(int part, int value);
    int getSpinBoxCount();
    bool isDragging();
    int getValue(int part);
    int getDraggedBox() const;

private:
    Array<std::unique_ptr<SpinBox>> m_spinBoxes;
    int m_draggedBox;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpinBoxGroup)
};
