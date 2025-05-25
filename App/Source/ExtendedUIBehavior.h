
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


#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginWindow.h"
#include "Thumbnail.h"

namespace te = tracktion_engine;

class ExtendedUIBehaviour : public te::UIBehaviour
{
public:
    ExtendedUIBehaviour() = default;

    std::unique_ptr<juce::Component> createPluginWindow (te::PluginWindowState& pws) override
    {
        if (auto ws = dynamic_cast<te::Plugin::WindowState*> (&pws))
            return PluginWindow::create (ws->plugin);

        return {};
    }

    void recreatePluginWindowContentAsync (te::Plugin& p) override
    {
        if (auto* w = dynamic_cast<PluginWindow*> (p.windowState->pluginWindow.get()))
            return w->recreateEditorAsync();

        UIBehaviour::recreatePluginWindowContentAsync (p);
    }

    void runTaskWithProgressBar (tracktion_engine::ThreadPoolJobWithProgress& t) override
    {
            double progress{ 0.0 };
            TaskRunner runner(t, progress);

            juce::AlertWindow w("Rendering", {}, juce::AlertWindow::NoIcon);
            w.addProgressBarComponent(progress);
            w.setVisible(true);
            w.setAlwaysOnTop(true);
            w.toFront(true);

            while (runner.isThreadRunning())
                if (!juce::MessageManager::getInstance()->runDispatchLoopUntil(10))
                    break;
    }

    std::unique_ptr<juce::AudioThumbnailBase> createAudioThumbnail (int sourceSamplesPerThumbnailSample,
                                                                            juce::AudioFormatManager& formatManagerToUse,
                                                                            juce::AudioThumbnailCache& cacheToUse) override
    {
        return std::make_unique<Thumbnail> (sourceSamplesPerThumbnailSample,
                                                       formatManagerToUse, cacheToUse);
    }


    private:

    struct TaskRunner  : public juce::Thread
    {
        TaskRunner(te::ThreadPoolJobWithProgress& t, double& prog) : Thread(t.getJobName()), task(t), progress(prog)
        {
            startThread();
        }

        ~TaskRunner()
        {
            task.signalJobShouldExit();
            waitForThreadToExit(10000);
        }


        void run() override
        {
            while (!threadShouldExit())
		    {
			    progress = task.getCurrentTaskProgress();
			    if (task.runJob() == juce::ThreadPoolJob::jobHasFinished)
				    break;
		    }
        }

        te::ThreadPoolJobWithProgress& task;
        double& progress;
    };
};
