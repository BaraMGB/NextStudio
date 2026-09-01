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
    bool isProjectWorkflowActive() const override;
    const ApplicationViewState &getApplicationState() const override;
    tracktion_engine::Edit *getCurrentEdit() const override;
    EditViewState *getEditViewState() const override;
    bool hasEditComponent() const override;
    bool hasHeaderComponent() const override;
    bool hasLowerRangeComponent() const override;
    juce::Rectangle<int> getScreenBounds() const override;
    juce::Rectangle<int> getLocalBounds() const override;
    juce::Image createSnapshot(const juce::Rectangle<int> &bounds, float scale) const override;
    juce::File getDebugArtifactsDirectory() const override;
    juce::File writeStateDump() const override;
    juce::File captureSnapshot(int maxWidth) const override;
    bool play() override;
    bool stop() override;
    bool showProjectSaveAs() override;
    tracktion_engine::AudioTrack *createAudioTrack(bool midi, const juce::String &name) override;
    void requestQuit() override;

private:
    MainComponent &m_mainComponent;
};
} // namespace NextStudio::Debug
