
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
#include "Samples707.h"
#include "Samples808.h"
#include "Samples909.h"
#include "Utilities/ApplicationViewState.h"
#include "MainComponent.h"

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
        auto workingDir = juce::File().createFileWithoutCheckingPath(m_applicationState.m_workDir);
        auto presetDir = juce::File().createFileWithoutCheckingPath(m_applicationState.m_presetDir);
        auto clipsDir = juce::File().createFileWithoutCheckingPath(m_applicationState.m_clipsDir);
        auto rendersDir = juce::File().createFileWithoutCheckingPath(m_applicationState.m_renderDir);
        auto samplesDir = juce::File().createFileWithoutCheckingPath(m_applicationState.m_samplesDir);
        auto projectsDir = juce::File().createFileWithoutCheckingPath(m_applicationState.m_projectsDir);
        workingDir.createDirectory();
        presetDir.createDirectory();
        clipsDir.createDirectory();
        rendersDir.createDirectory();
        samplesDir.createDirectory();
        projectsDir.createDirectory();

        extractSamplesIfNeeded(samplesDir);

        mainWindow.reset(new MainWindow(getApplicationName(), m_applicationState));
    }

    void extractSamplesIfNeeded(const juce::File &samplesDir)
    {
        auto extract = [](const juce::File &targetDir, const char *const *resourceList, const char *const *filenames, int size, auto getResourceFn)
        {
            if (targetDir.exists())
                return;

            targetDir.createDirectory();
            for (int i = 0; i < size; ++i)
            {
                int dataSize = 0;
                if (const char *data = getResourceFn(resourceList[i], dataSize))
                    targetDir.getChildFile(filenames[i]).replaceWithData(data, dataSize);
            }
        };

        extract(samplesDir.getChildFile("707"), Samples707::namedResourceList, Samples707::originalFilenames, Samples707::namedResourceListSize, Samples707::getNamedResource);
        extract(samplesDir.getChildFile("808"), Samples808::namedResourceList, Samples808::originalFilenames, Samples808::namedResourceListSize, Samples808::getNamedResource);
        extract(samplesDir.getChildFile("909"), Samples909::namedResourceList, Samples909::originalFilenames, Samples909::namedResourceListSize, Samples909::getNamedResource);
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
