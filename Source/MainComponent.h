/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "HeaderComponent.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class NextLookAndFeel : public LookAndFeel_V4
{
public:
    NextLookAndFeel()
    {
    }
    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
        bool isMouseOverButton, bool isButtonDown) override
    {
        auto buttonArea = button.getLocalBounds();
        auto edge = 1;

      

        g.setColour(Colours::black);
        g.fillRect(buttonArea);
        buttonArea.reduce(edge, edge);
        float gradientContrast = 0.1f; 
        ColourGradient cg(Colour(90,90,90), buttonArea.getX(), buttonArea.getY(),Colour(66,66,66) , buttonArea.getX(),buttonArea.getHeight() , false);
        g.setFillType(cg);
        g.fillRect(buttonArea);

    }

    Font getTextButtonFont(TextButton&, int buttonHeight) override
    {
        return { jmin(10.0f, buttonHeight * 0.6f) };
    }
};

class MainComponent   : public AudioAppComponent
                      , public FileBrowserListener
                      , public Button::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;
private:
    void selectionChanged()                           override {}
    void fileClicked (const File&, const MouseEvent&) override {}
    void fileDoubleClicked(const File&)               override
    {
        auto selectedFile = m_tree.getSelectedFile();
        if (selectedFile.existsAsFile())
        {
        }
    }
    void browserRootChanged(const File&) override {}
    TimeSliceThread m_thread;
    DirectoryContentsList m_dirConList;
    FileTreeComponent m_tree;
    Component m_songEditor;
    StretchableLayoutManager m_stretchableManager;
    StretchableLayoutResizerBar m_resizerBar{ &m_stretchableManager, 1, true };
    HeaderComponent m_header;
    NextLookAndFeel m_nextLookAndFeel;


    const int c_headerHeight = 50;
    const int c_footerHeight = 50;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
