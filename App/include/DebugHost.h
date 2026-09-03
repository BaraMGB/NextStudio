#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ApplicationViewState.h"
#include "EditViewState.h"

namespace NextStudio::Debug
{
/** Non-owning adapter to the live application.

    The host owns none of the returned pointers. Every method must be invoked on
    the JUCE message thread; returned pointers are valid only for that call's
    synchronous command execution. Implementations must outlive the controller.
*/
class DebugHost
{
public:
    virtual ~DebugHost() = default;

    virtual bool isDebugMode() const = 0;
    virtual bool isProjectWorkflowActive() const { return false; }
    virtual const ApplicationViewState &getApplicationState() const = 0;
    virtual tracktion_engine::Edit *getCurrentEdit() const = 0;
    virtual EditViewState *getEditViewState() const = 0;
    virtual bool hasEditComponent() const = 0;
    virtual bool hasHeaderComponent() const = 0;
    virtual bool hasLowerRangeComponent() const = 0;
    virtual juce::Rectangle<int> getScreenBounds() const = 0;
    virtual juce::Rectangle<int> getLocalBounds() const = 0;
    virtual juce::Image createSnapshot(const juce::Rectangle<int> &bounds, float scale) const = 0;
    virtual juce::File getDebugArtifactsDirectory() const = 0;

    virtual juce::File writeStateDump() const = 0;
    virtual juce::File captureSnapshot(int maxWidth) const = 0;
    virtual bool play() = 0;
    virtual bool stop() = 0;
    virtual bool showProjectSaveAs() { return false; }
    virtual tracktion_engine::AudioTrack *createAudioTrack(bool midi, const juce::String &name) = 0;
    virtual void requestQuit() = 0;
};
} // namespace NextStudio::Debug
