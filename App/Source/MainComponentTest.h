/*
  ==============================================================================

    MainComponentTest.h
    Created: 27 Dec 2019 2:14:32am
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SongEditorComponent.h"
#include "NextLookAndFeel.h"

//==============================================================================
/*
*/
class MainComponent2    : public Component
{
public:
    MainComponent2()
    {
        setLookAndFeel(&m_NextLookAndFeel);
        addAndMakeVisible(m_songEditor);
        
        setSize(1000, 800);
    }

    ~MainComponent2()
    {
        setLookAndFeel(nullptr);
    }

    void paint (Graphics& g) override
    {
        g.setColour(Colour(0xff242424));
        g.fillRect(getBounds());
    }

    void resized() override
    {
        m_songEditor.setBounds(getBounds());
    }

private:
    SongEditorComponent m_songEditor;
    NextLookAndFeel m_NextLookAndFeel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent2)
};
