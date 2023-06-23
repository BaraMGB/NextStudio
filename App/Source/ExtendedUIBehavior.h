#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginWindow.h"

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
