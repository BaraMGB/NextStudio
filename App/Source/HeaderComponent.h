#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "ApplicationViewState.h"
#include "Utilities.h"
#include "AudioMidiSettings.h"
#include "PositionDisplayComponent.h"

namespace te = tracktion_engine;

class HeaderComponent    : public juce::Component
                         , public juce::Button::Listener
                         , public juce::Timer
                         , public juce::ChangeBroadcaster
{
public:
    HeaderComponent(EditViewState &, ApplicationViewState & applicationState);
    ~HeaderComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;

    juce::File getSelectedFile() const;

    void loopButtonClicked();

private:
    EditViewState& m_editViewState;
    static juce::FlexBox createFlexBox(juce::FlexBox::JustifyContent justify) ;
    int getButtonSize();
    int getGapSize();

    void addButtonsToFlexBox(juce::FlexBox& box,const juce::Array<juce::Component*>& buttons, int width = 0);
    void addFlexBoxToFlexBox(juce::FlexBox& target, const juce::Array<juce::FlexBox*>& items);
    juce::DrawableButton m_newButton
                       , m_loadButton
                       , m_saveButton
                       , m_pluginsButton
                       , m_stopButton
                       , m_recordButton
                       , m_settingsButton
                       , m_playButton
                       , m_loopButton
                       , m_clickButton
                       , m_followPlayheadButton;
    PositionDisplayComponent m_display;

    te::Edit& m_edit;
    ApplicationViewState& m_applicationState;
    juce::String m_btn_col { "#dbdbdb" };

    juce::File m_loadingFile {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderComponent)
};
