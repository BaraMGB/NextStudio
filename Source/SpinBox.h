/*
  ==============================================================================

    SpinBox.h
    Created: 26 Jan 2020 8:05:27pm
    Author:  BaraMGB

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class SpinBox    : public Component
                 , public ChangeBroadcaster
{
public:
    SpinBox(int digits, int init, int min, int max, String separator, int step = 1);


    ~SpinBox();


    void mouseEnter(const MouseEvent &event) override;
    void mouseDown(const MouseEvent &event) override;
    void mouseDrag(const MouseEvent &event) override;
    void mouseUp(const MouseEvent &event) override;
    void mouseExit(const MouseEvent &event) override;

    void paint (Graphics& g) override;
    void resized() override;

    void setValue(int value);
    int getValue();

    bool overrun() const;
    bool underrun() const;

    int getMin() const;
    int getMax() const;
    int getStep() const;
    void setTextColour(const Colour &textColour);

    bool isDragging() const;


private:
    int m_digits, m_value, m_min, m_max, m_step;

    String m_separator;
    bool m_overrun, m_underrun, m_dragging;
    Point<int> m_mouseDown;
    Colour m_textColour;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpinBox)
};
