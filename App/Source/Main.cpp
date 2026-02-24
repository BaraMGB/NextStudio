
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"
#include "Utilities/ApplicationViewState.h"

//==============================================================================
class NextStudioApplication : public juce::JUCEApplication
{
public:
    //==============================================================================
    NextStudioApplication() {}

    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return false; }

    //==============================================================================
    void initialise(const juce::String & /*commandLine*/) override
    {
        juce::Logger::writeToLog("Welcome to " + getApplicationName() + " v" + getApplicationVersion());
        mainWindow.reset(new MainWindow(getApplicationName(), m_applicationState));
    }

    void shutdown() override { mainWindow = nullptr; }

    void systemRequestedQuit() override { quit(); }

    void anotherInstanceStarted(const juce::String & /*commandLine*/) override {}

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name, ApplicationViewState &applicationSettings)
            : DocumentWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId), DocumentWindow::allButtons),
              m_applicationState(applicationSettings)
        {
            setUsingNativeTitleBar(true);

#if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
#else

            setBounds(m_applicationState.m_windowXpos, m_applicationState.m_windowYpos, m_applicationState.m_windowWidth, m_applicationState.m_windowHeight);
            setResizable(true, true);
#endif
            auto mc = new MainComponent(m_applicationState);
            mc->setSize(m_applicationState.m_windowWidth, m_applicationState.m_windowHeight);
            setContentOwned(mc, true);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            if (auto mc = dynamic_cast<MainComponent *>(getContentComponent()))
            {
                if (mc->handleUnsavedEdit())
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
        ApplicationViewState &m_applicationState;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    ApplicationViewState m_applicationState;
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(NextStudioApplication)
