#pragma once

#include "DebugHost.h"

class MainComponent;

namespace NextStudio::Debug
{
class MainComponentDebugHost final : public DebugHost
{
public:
    explicit MainComponentDebugHost(MainComponent &mainComponent);

    bool isDebugMode() const override;
    const ApplicationViewState &getApplicationState() const override;
    tracktion_engine::Edit *getCurrentEdit() const override;
    EditViewState *getEditViewState() const override;
    EditComponent *getEditComponent() const override;
    bool hasHeaderComponent() const override;
    bool hasLowerRangeComponent() const override;
    juce::Rectangle<int> getScreenBounds() const override;
    juce::Rectangle<int> getLocalBounds() const override;
    juce::Image createSnapshot(const juce::Rectangle<int> &bounds, float scale) const override;
    juce::File getDebugArtifactsDirectory() const override;
    bool selectTrackByName(const juce::String &trackName) override;
    void switchLowerRangeView(LowerRangeView view) override;
    void requestQuit() override;

private:
    MainComponent &m_mainComponent;
};
} // namespace NextStudio::Debug
