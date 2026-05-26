
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
#include "DebugShell.h"
#include "MainComponent.h"
#include "ApplicationViewState.h"
#include "Logging.h"

//==============================================================================
class NextStudioApplication : public juce::JUCEApplication
{
public:
    struct LaunchOptions
    {
        bool debugShell{false};
    };

    //==============================================================================
    NextStudioApplication() {}

    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return false; }

    //==============================================================================
    void initialise(const juce::String &commandLine) override
    {
        m_launchOptions = parseLaunchOptions(commandLine);
        NS_LOG_INFO(app, "Welcome to " + getApplicationName() + " v" + getApplicationVersion());
        mainWindow.reset(new MainWindow(getApplicationName(), m_applicationState, m_launchOptions.debugShell));

        if (m_launchOptions.debugShell)
        {
            if (auto *mainComponent = mainWindow->getMainComponent())
            {
                m_debugShell = std::make_unique<NextStudio::Debug::DebugShell>(*mainComponent);
                m_debugShell->start();
            }
            else
            {
                NS_LOG_ERROR(app, "debug shell requested but MainComponent is unavailable");
            }
        }
    }

    void shutdown() override
    {
        m_debugShell = nullptr;
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        if (mainWindow != nullptr)
        {
            if (auto *mc = dynamic_cast<MainComponent *>(mainWindow->getContentComponent()))
                if (!mc->handleUnsavedEdit())
                    return;
        }

        quit();
    }

    void anotherInstanceStarted(const juce::String & /*commandLine*/) override {}

    static LaunchOptions parseLaunchOptions(const juce::String &commandLine)
    {
        LaunchOptions options;
        auto args = juce::StringArray::fromTokens(commandLine, true);
        args.trim();
        args.removeEmptyStrings();
        options.debugShell = args.contains("--debug-shell");
        return options;
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name, ApplicationViewState &applicationSettings, bool debugShellEnabled)
            : DocumentWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId), DocumentWindow::allButtons),
              m_applicationState(applicationSettings),
              m_debugShellEnabled(debugShellEnabled)
        {
            setUsingNativeTitleBar(true);

#if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
#else

            setBounds(m_applicationState.m_windowXpos, m_applicationState.m_windowYpos, m_applicationState.m_windowWidth, m_applicationState.m_windowHeight);
            setResizable(true, true);
#endif
            auto mc = new MainComponent(m_applicationState, m_debugShellEnabled);
            mc->setSize(m_applicationState.m_windowWidth, m_applicationState.m_windowHeight);
            setContentOwned(mc, true);
            setVisible(true);
        }

        MainComponent *getMainComponent() const
        {
            return dynamic_cast<MainComponent *>(getContentComponent());
        }

        void closeButtonPressed() override
        {
            if (m_debugShellEnabled)
                JUCEApplication::getInstance()->quit();
            else
                JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        ApplicationViewState &m_applicationState;
        bool m_debugShellEnabled{false};
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    ApplicationViewState m_applicationState;
    LaunchOptions m_launchOptions;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<NextStudio::Debug::DebugShell> m_debugShell;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(NextStudioApplication)
