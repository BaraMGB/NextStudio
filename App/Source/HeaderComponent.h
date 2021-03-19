/*
  ==============================================================================

    HeaderComponent.h
    Created: 7 Jan 2020 8:31:11pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "ApplicationViewState.h"
#include "Utilities.h"
#include "AudioMidiSettings.h"

namespace te = tracktion_engine;

class PositionDisplayComponent  : public juce::Component
{
public:
    PositionDisplayComponent(te::Edit &edit);

    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void resized() override;

    void update();

    double draggedNewTime(int draggedDistance
                          , double timeAtMouseDown
                          , double unitfactor
                          , bool inBeat
                          , int dragfactor=10);
private:
    te::Edit& m_edit;
    juce::Rectangle<int> m_bmpRect
                       , m_sigRect
                       , m_barBeatTickRect
                       , m_timeRect
                       , m_loopInrect
                       , m_loopOutRect;
    juce::Label          m_bpmLabel
                       , m_sigLabel
                       , m_barBeatTickLabel
                       , m_timeLabel
                       , m_loopInLabel
                       , m_loopOutLabel;

    juce::Point<int>     m_mousedownPosition
                       , m_MouseDownScreenPos;

    double               m_mousedownBPM
                       , m_mousedownBarsBeats
                       , m_ppqTimeAtMd
                       , m_mousedownTime
                       , m_mousedownLoopIn
                       , m_mousedownLoopOut
                       , m_newTempo;

    int                  m_mousedownNumerator
                       , m_mousedownDenominator;
};

//------------------------------------------------------------------------------

class HeaderComponent    : public juce::Component
                         , public juce::Button::Listener
                         , public juce::Timer
                         , public juce::ChangeBroadcaster
{
public:
    HeaderComponent(te::Edit &, ApplicationViewState & applicationState);
    ~HeaderComponent();

    void resized() override;
    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;

    juce::File loadingFile() const;

private:
    juce::DrawableButton m_newButton
                       , m_loadButton
                       , m_saveButton
                       , m_pluginsButton
                       , m_stopButton
                       , m_recordButton
                       , m_settingsButton
                       , m_playButton
                       , m_loopButton
                       , m_clickButton;

    te::Edit& m_edit;
    ApplicationViewState& m_applicationState;
    juce::String m_btn_col { "#dbdbdb" };
    juce::Colour m_mainColour{ juce::Colour(0xff57cdff) };
    PositionDisplayComponent m_display;
    juce::File m_loadingFile {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderComponent)
};
