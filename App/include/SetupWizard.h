
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginBrowser.h"
#include "ApplicationViewState.h"
#include "InitialContentSetup.h"
#include "ThemeHelpers.h"

class SetupWizard : public juce::Component
{
public:
    SetupWizard(ApplicationViewState &avs, tracktion::Engine &engine);
    ~SetupWizard() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    bool isFinished() const { return m_finished; }

    std::function<void()> onFinished;

private:
    bool validateAndPrepareContentRoot(const juce::File &root, juce::String &errorMessage) const;
    void showValidationError(const juce::String &message);
    void clearValidationError();
    void updatePathLabel();
    void updateGuiScale();
    void applySelectedTheme();

    ApplicationViewState &m_avs;
    tracktion::Engine &m_engine;
    bool m_finished = false;

    juce::Label m_titleLabel;
    std::unique_ptr<juce::Drawable> m_logoDrawable;
    juce::Rectangle<int> m_logoBounds;
    juce::Label m_instructionLabel;

    // Alpha warning
    juce::GroupComponent m_alphaWarningGroup;
    juce::Label m_alphaWarningLabel;

    // Path selection
    juce::GroupComponent m_pathGroup;
    juce::Label m_currentPathLabel;
    juce::TextButton m_selectPathButton;
    std::unique_ptr<juce::FileChooser> m_fileChooser;

    // Interface setup
    juce::GroupComponent m_interfaceGroup;
    juce::Label m_guiScaleLabel;
    juce::Slider m_guiScaleSlider;
    juce::Label m_themeLabel;
    juce::ComboBox m_themeCombo;

    // Plugin setup
    juce::GroupComponent m_pluginGroup;
    PluginSettings m_pluginSettings;

    // Audio setup
    juce::GroupComponent m_audioGroup;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> m_audioSelector;
    std::unique_ptr<juce::Viewport> m_audioViewport;

    juce::Label m_validationLabel;
    juce::TextButton m_finishButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SetupWizard)
};
