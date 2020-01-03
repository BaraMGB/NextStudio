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
                     , public ChangeBroadcaster
                     , public ChangeListener
{
public:
    DraggableLabel(int value, int minValue, int maxValue, String separator, Justification justification, int position=-1);
    ~DraggableLabel() override
    {
    }

    void paint(Graphics& g) override;
    void mouseDrag(const MouseEvent& event) override;
    void resized() override;

    void changeListenerCallback(ChangeBroadcaster *source) override;

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



    void setDownStop(bool downStop);


    int getMinValue() const;

private:

    int    m_value,
           m_minValue,
           m_maxValue,
           m_valueTmp,
           m_overflow;
    bool   m_downStop;
    int    m_position;

    String m_separator;
    float  m_accuracy;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraggableLabel)
};
