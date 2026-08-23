
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
#include "ApplicationViewState.h"
#include "DebugLaunchDiagnostics.h"
#include "DebugSessionEnvironment.h"
#include "DebugShell.h"
#include "Logging.h"
#include "MainComponent.h"
#include "MainComponentDebugHost.h"
#include "WineRendererFallback.h"

//==============================================================================
class NextStudioApplication : public juce::JUCEApplication
{
public:
    struct LaunchOptions
    {
        bool debugShell{false};
        juce::String debugShellRequestId;
    };

    //==============================================================================
    NextStudioApplication() {}

    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override
    {
        m_launchOptions = parseLaunchOptions(getCommandLineParameters());
        return false;
    }

    //==============================================================================
    void initialise(const juce::String &commandLine) override
    {
        m_fileLogger.reset(juce::FileLogger::createDefaultAppLogger("NextStudio", "NextStudio.log", "NextStudio application log", 512 * 1024));
        if (m_fileLogger != nullptr)
            juce::Logger::setCurrentLogger(m_fileLogger.get());

        m_initialiseEntered = true;
        m_launchOptions = parseLaunchOptions(commandLine);
        NS_LOG_INFO(app, "Welcome to " + getApplicationName() + " v" + getApplicationVersion());
        m_wineRendererFallback.start();

        if (m_launchOptions.debugShell)
        {
            m_debugSessionDirectory = NextStudio::Debug::SessionEnvironment::createDebugSessionTempDirectory();
            if (m_debugSessionDirectory == juce::File())
            {
                NS_LOG_ERROR(app, "failed to create debug-shell session directory");
                quit();
                return;
            }

            const auto workspace = m_debugSessionDirectory.getChildFile("workspace");
            workspace.createDirectory();
            m_applicationState = std::make_unique<ApplicationViewState>(m_debugSessionDirectory.getChildFile("settings/AppSettings.xml"));
            m_applicationState->setRootFolder(workspace);
            m_applicationState->m_setupComplete = true;
            m_applicationState->saveState();
        }
        else
        {
            m_applicationState = std::make_unique<ApplicationViewState>();
        }

        mainWindow.reset(new MainWindow(getApplicationName(), *m_applicationState, m_launchOptions.debugShell, m_debugSessionDirectory, m_wineRendererFallback));

        if (m_launchOptions.debugShell)
        {
            if (auto *mainComponent = mainWindow->getMainComponent())
            {
                m_debugHost = std::make_unique<NextStudio::Debug::MainComponentDebugHost>(*mainComponent);
                m_debugShell = std::make_unique<NextStudio::Debug::DebugShell>(*m_debugHost);
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
        if (!m_initialiseEntered && m_launchOptions.debugShell && m_launchOptions.debugShellRequestId.isNotEmpty())
            NextStudio::Debug::LaunchDiagnostics::recordDebugShellSingleInstanceRejection(getCommandLineParameters(), m_launchOptions.debugShellRequestId);

        m_debugShell = nullptr;
        m_debugHost = nullptr;
        m_wineRendererFallback.stop();
        mainWindow = nullptr;
        m_applicationState = nullptr;

        if (m_debugSessionDirectory != juce::File())
        {
            m_debugSessionDirectory.deleteRecursively();
            m_debugSessionDirectory = juce::File();
        }

        if (juce::Logger::getCurrentLogger() == m_fileLogger.get())
            juce::Logger::setCurrentLogger(nullptr);
        m_fileLogger = nullptr;
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

    void anotherInstanceStarted(const juce::String &commandLine) override
    {
        if (parseLaunchOptions(commandLine).debugShell)
            NS_LOG_WARN(app, "debug shell launch rejected because another NextStudio instance is already running");
    }

    static LaunchOptions parseLaunchOptions(const juce::String &commandLine)
    {
        LaunchOptions options;
        auto args = juce::StringArray::fromTokens(commandLine, true);
        args.trim();
        args.removeEmptyStrings();

        for (const auto &arg : args)
        {
            if (arg == "--debug-shell")
                options.debugShell = true;
            else if (arg.startsWith("--debug-shell-request-id="))
                options.debugShellRequestId = arg.fromFirstOccurrenceOf("=", false, false).trim();
        }

        return options;
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name, ApplicationViewState &applicationSettings, bool debugShellEnabled, const juce::File &debugSessionDirectory, NextStudio::WineRendererFallback &wineRendererFallback)
            : DocumentWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId), DocumentWindow::allButtons),
              m_applicationState(applicationSettings),
              m_debugShellEnabled(debugShellEnabled),
              m_debugSessionDirectory(debugSessionDirectory)
        {
            setUsingNativeTitleBar(true);

#if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
#else
            const juce::Rectangle<int> savedBounds(m_applicationState.m_windowXpos, m_applicationState.m_windowYpos, m_applicationState.m_windowWidth, m_applicationState.m_windowHeight);
            const auto visibleBounds = constrainToCurrentDisplays(savedBounds);
            NS_LOG_INFO(app, "Main window bounds: saved=" + savedBounds.toString() + ", visible=" + visibleBounds.toString());
            setBounds(visibleBounds);
            setResizable(true, true);
#endif
            auto mc = new MainComponent(m_applicationState, wineRendererFallback, m_debugShellEnabled, m_debugSessionDirectory);
            mc->setSize(m_applicationState.m_windowWidth, m_applicationState.m_windowHeight);
            setContentOwned(mc, true);
            wineRendererFallback.applyTo(*this);
            setVisible(true);
        }

        MainComponent *getMainComponent() const { return dynamic_cast<MainComponent *>(getContentComponent()); }

        void closeButtonPressed() override
        {
            if (m_debugShellEnabled)
                JUCEApplication::getInstance()->quit();
            else
                JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        static juce::Rectangle<int> constrainToCurrentDisplays(juce::Rectangle<int> bounds)
        {
            const auto &displays = juce::Desktop::getInstance().getDisplays();
            const auto *display = displays.getDisplayForRect(bounds);

            if (display == nullptr)
                display = displays.getPrimaryDisplay();

            if (display == nullptr || display->userArea.isEmpty())
                return bounds;

            const auto area = display->userArea;
            bounds.setSize(juce::jmin(area.getWidth(), juce::jmax(640, bounds.getWidth())), juce::jmin(area.getHeight(), juce::jmax(480, bounds.getHeight())));

            const auto visiblePart = area.getIntersection(bounds);
            if (visiblePart.getWidth() < 100 || visiblePart.getHeight() < 100)
                return bounds.withCentre(area.getCentre()).constrainedWithin(area);

            return bounds.constrainedWithin(area);
        }

        ApplicationViewState &m_applicationState;
        bool m_debugShellEnabled{false};
        juce::File m_debugSessionDirectory;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<juce::FileLogger> m_fileLogger;
    std::unique_ptr<ApplicationViewState> m_applicationState;
    juce::File m_debugSessionDirectory;
    LaunchOptions m_launchOptions;
    bool m_initialiseEntered{false};
    NextStudio::WineRendererFallback m_wineRendererFallback;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<NextStudio::Debug::DebugHost> m_debugHost;
    std::unique_ptr<NextStudio::Debug::DebugShell> m_debugShell;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(NextStudioApplication)
