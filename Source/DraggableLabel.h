/*
  ==============================================================================

    DraggableLabel.h
    Created: 14 Dec 2019 5:17:01pm
    Author:  ubu1710

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class DraggableLabel : public Label
{
public:
    DraggableLabel(int value, int minValue, int maxValue, String separator, Justification justification, bool first);
    ~DraggableLabel() override
    {
    }

    void paint(Graphics& g) override;
    void mouseUp(const MouseEvent& /*event*/) override;
    void mouseDrag(const MouseEvent& event) override;
    void resized() override;

    void count(const int step);
    void setValue(const int value);

    inline int getValue()
    {
        return m_value;
    }
    inline int overflowCount()
    {
        return m_overflow;
    }
    inline bool isOverflow()
    {
        return m_overFlowFlag;
    }
    inline void clearOverflowFlag()
    {
        m_overFlowFlag = false;
    }
    inline bool isFirst()
    {
        return m_first;
    }


    int getMinValue() const;

    void setMinFlag(bool minFlag);

private:

    int    m_value,
           m_minValue,
           m_maxValue,
           m_valueTmp,
           m_overflow;
    bool   m_overFlowFlag,
           m_first;
    String m_separator;
    float  m_accuracy;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraggableLabel)
};
