
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../Utilities/ApplicationViewState.h"

class SetupWizard : public juce::Component
{
public:
    SetupWizard(ApplicationViewState &avs, tracktion::Engine &engine);
    ~SetupWizard() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    bool isFinished() const { return m_finished; }

private:
    static bool ensureDirectory(const juce::File &directory, juce::StringArray &errors);
    static bool ensureWritable(const juce::File &directory, juce::StringArray &errors);
    static bool ensureContentLayout(const juce::File &root, juce::StringArray &errors);
    bool validateAndPrepareContentRoot(const juce::File &root, juce::String &errorMessage) const;
    void showValidationError(const juce::String &message) const;
    void updatePathLabel();
    void updateGuiScale();

    ApplicationViewState &m_avs;
    tracktion::Engine &m_engine;
    bool m_finished = false;

    juce::Label m_titleLabel;
    std::unique_ptr<juce::Drawable> m_logoDrawable;
    juce::Rectangle<int> m_logoBounds;
    juce::Label m_instructionLabel;

    // Path selection
    juce::GroupComponent m_pathGroup;
    juce::Label m_currentPathLabel;
    juce::TextButton m_selectPathButton;

    // Interface setup
    juce::GroupComponent m_interfaceGroup;
    juce::Label m_guiScaleLabel;
    juce::Slider m_guiScaleSlider;

    // Audio setup
    juce::GroupComponent m_audioGroup;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> m_audioSelector;
    std::unique_ptr<juce::Viewport> m_audioViewport;

    juce::TextButton m_finishButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SetupWizard)
};
