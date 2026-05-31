#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ApplicationViewState.h"
#include "EditViewState.h"

class EditComponent;

namespace NextStudio::Debug
{
class DebugHost
{
public:
    virtual ~DebugHost() = default;

    virtual bool isDebugMode() const = 0;
    virtual const ApplicationViewState &getApplicationState() const = 0;
    virtual tracktion_engine::Edit *getCurrentEdit() const = 0;
    virtual EditViewState *getEditViewState() const = 0;
    virtual EditComponent *getEditComponent() const = 0;
    virtual bool hasHeaderComponent() const = 0;
    virtual bool hasLowerRangeComponent() const = 0;
    virtual juce::Rectangle<int> getScreenBounds() const = 0;
    virtual juce::Rectangle<int> getLocalBounds() const = 0;
    virtual juce::Image createSnapshot(const juce::Rectangle<int> &bounds, float scale) const = 0;
    virtual juce::File getDebugArtifactsDirectory() const = 0;
    virtual bool selectTrackByName(const juce::String &trackName) = 0;
    virtual void switchLowerRangeView(LowerRangeView view) = 0;
    virtual void requestQuit() = 0;
};
} // namespace NextStudio::Debug
