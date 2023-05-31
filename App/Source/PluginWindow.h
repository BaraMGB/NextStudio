#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

inline bool isDPIAware (te::Plugin&)
{
	// You should keep a DB of if plugins are DPI aware or not and recall that value
	// here. You should let the user toggle the value if the plugin appears tiny
	return true;
}

//==============================================================================
class PluginEditor : public juce::Component
{
public:
    virtual bool allowWindowResizing() = 0;
    virtual juce::ComponentBoundsConstrainer* getBoundsConstrainer() = 0;
};

//==============================================================================
struct AudioProcessorEditorContentComp : public PluginEditor
{
    explicit AudioProcessorEditorContentComp (te::ExternalPlugin& plug) : plugin (plug)
    {
        JUCE_AUTORELEASEPOOL
        {
            if (auto pi = plugin.getAudioPluginInstance())
            {
                editor.reset (pi->createEditorIfNeeded());

                if (editor == nullptr)
                    editor = std::make_unique
                            <juce::GenericAudioProcessorEditor> (*pi);
                addAndMakeVisible (*editor);
            }
        }
        resizeToFitEditor (true);
    }

    bool allowWindowResizing() override { return false; }

    juce::ComponentBoundsConstrainer* getBoundsConstrainer() override
    {
        if (editor == nullptr || allowWindowResizing())
            return {};

        return editor->getConstrainer();
    }

    void resized() override
    {
        if (editor != nullptr)
            editor->setBounds (getLocalBounds());
    }

    void childBoundsChanged (juce::Component* c) override
    {
        if (c == editor.get())
        {
            plugin.edit.pluginChanged (plugin);
            resizeToFitEditor();
        }
    }

    void resizeToFitEditor (bool force = false)
    {
        if (force || ! allowWindowResizing())
            setSize (juce::jmax (8
                           , editor != nullptr
                                    ? editor->getWidth()
                                    : 0)
                   , juce::jmax (8
                           , editor != nullptr
                                    ? editor->getHeight()
                                    : 0));
    }

    te::ExternalPlugin& plugin;
    std::unique_ptr<juce::AudioProcessorEditor> editor;

    AudioProcessorEditorContentComp() = delete;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioProcessorEditorContentComp)
};

//=============================================================================
class PluginWindow : public juce::DocumentWindow
{
public:
    explicit PluginWindow (te::Plugin&);
    ~PluginWindow() override;

    static std::unique_ptr<juce::Component> create (te::Plugin&);

    void show();

    void setEditor (std::unique_ptr<PluginEditor>);
    [[nodiscard]] PluginEditor* getEditor() const         { return editor.get(); }
    
    void recreateEditor();
    void recreateEditorAsync();

private:
    void moved() override;
    void userTriedToCloseWindow() override          { plugin.windowState->closeWindowExplicitly(); }
    void closeButtonPressed() override              { userTriedToCloseWindow(); }
    [[nodiscard]] float getDesktopScaleFactor() const override    { return 1.0f; }

    std::unique_ptr<PluginEditor> createContentComp();

    std::unique_ptr<PluginEditor> editor;
    
    te::Plugin& plugin;
    te::PluginWindowState& windowState;
};

//==============================================================================
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
        TaskRunner runner (t);

         while (runner.isThreadRunning())
             if (! juce::MessageManager::getInstance()->runDispatchLoopUntil (10))
                 break;
    }

    private:

    struct TaskRunner  : public juce::Thread
    {
        TaskRunner (te::ThreadPoolJobWithProgress& t)
            : Thread (t.getJobName()), task (t)
        {
            startThread();
        }

        ~TaskRunner() override
        {
            task.signalJobShouldExit();
            waitForThreadToExit (10000);
        }

        void run() override
        {
            while (! threadShouldExit())
                if (task.runJob() == juce::ThreadPoolJob::jobHasFinished)
                    break;
        }

            te::ThreadPoolJobWithProgress& task;
    };
};
