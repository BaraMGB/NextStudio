/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"

//==============================================================================
class NextStudioApplication  : public juce::JUCEApplication
{
public:
    //==============================================================================
    NextStudioApplication() {}

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return false; }

    //==============================================================================
    void initialise (const juce::String& /*commandLine*/) override
    {
        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& /*commandLine*/) override
    {
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name) : DocumentWindow (
                                          name
                                        , juce::Desktop::getInstance()
                                          .getDefaultLookAndFeel()
                                          .findColour (
                                              ResizableWindow::backgroundColourId)
                                        , DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
           #else
            auto settingsFile = juce::File::getSpecialLocation (
                        juce::File::userApplicationDataDirectory)
                        .getChildFile ("NextStudio/AppSettings.xml");
            if (settingsFile.existsAsFile ())
            {
                juce::XmlDocument xmlDoc (settingsFile);
                auto xmlToRead = xmlDoc.getDocumentElement ();
                auto applicationState = juce::ValueTree::fromXml (*xmlToRead);
                setBounds (applicationState.getProperty (IDs::WindowX)
                           , applicationState.getProperty (IDs::WindowY)
                           , applicationState.getProperty (IDs::WindowWidth)
                           , applicationState.getProperty (IDs::WindowHeight));
            }
            else
            {
                centreWithSize (1000, 1600);
            }
            setResizable (true, true);
           #endif
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            if (auto mc = dynamic_cast<MainComponent*>(getContentComponent ()))
            {
                if (mc->handleUnsavedEdit ())
                {
                    JUCEApplication::getInstance()->systemRequestedQuit();
                }
            }
            else
            {
                JUCEApplication::getInstance()->systemRequestedQuit();
            }
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (NextStudioApplication)
