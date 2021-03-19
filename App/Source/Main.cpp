/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "ApplicationViewState.h"
#include "MainComponent.h"

//==============================================================================
class NextStudioApplication  : public juce::JUCEApplication
{
public:
    //==============================================================================
    NextStudioApplication(){}

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return false; }

    //==============================================================================
    void initialise (const juce::String& /*commandLine*/) override
    {
        mainWindow.reset (new MainWindow (getApplicationName(), m_applicationState));
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
        MainWindow (juce::String name, ApplicationViewState& applicationSettings) : DocumentWindow (
                                          name
                                        , juce::Desktop::getInstance()
                                          .getDefaultLookAndFeel()
                                          .findColour (
                                              ResizableWindow::backgroundColourId)
                                        , DocumentWindow::allButtons)
                                        , m_applicationState(applicationSettings)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(m_applicationState), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
           #else

            setBounds (m_applicationState.m_windowXpos
                       , m_applicationState.m_windowYpos
                       , m_applicationState.m_windowWidth
                       , m_applicationState.m_windowHeight);
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
        ApplicationViewState& m_applicationState;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    ApplicationViewState m_applicationState;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (NextStudioApplication)
