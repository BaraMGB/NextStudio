#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"
#include "LevelMeterComponent.h"
#include "PluginRackComponent.h"
#include "AutomatableSliderComponent.h"

namespace te = tracktion_engine;

class AutomationLaneHeaderComponent : public juce::Component
{
public:
    AutomationLaneHeaderComponent(te::AutomatableParameter& ap);
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label m_parameterName;
    juce::Label m_pluginName;
    te::AutomatableParameter& m_automatableParameter;
    int m_heightAtMouseDown = 0, m_mouseDownY = 0;
    bool m_resizing = false, m_hovering = false;
    AutomatableSliderComponent m_slider;

    // MouseListener interface
public:
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag (const juce::MouseEvent &event) override;
    void mouseMove (const juce::MouseEvent &event) override;
    void mouseExit (const juce::MouseEvent &event) override;
    te::AutomatableParameter &automatableParameter() const;
};

class TrackHeaderComponent : public juce::Component
        , private te::ValueTreeAllEventListener
                           , private FlaggedAsyncUpdater
                           , public juce::ChangeBroadcaster
                           , public juce::DragAndDropTarget
                           , public juce::Label::Listener
{
public:
    TrackHeaderComponent (EditViewState&, te::Track::Ptr);
    ~TrackHeaderComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;
    void mouseMove(const juce::MouseEvent &event) override;
    void mouseExit(const juce::MouseEvent &event) override;
    bool keyPressed(const juce::KeyPress &key) override;
    juce::Colour getTrackColour();

    te::Track::Ptr getTrack() const;

    void updateMidiInputs();
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;


    void labelTextChanged(juce::Label *labelThatHasChanged) override;
    void childrenSetVisible(bool v);

private:
    void handleAsyncUpdate();
    void valueTreeChanged() override {}
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeChildAdded(
            juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenAdded);
    void valueTreeChildRemoved(
            juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenRemoved
            , int indexFromWhichChildWasRemoved);

    void showPopupMenu(te::AudioTrack* at);
    void deleteTrackFromEdit();

    EditViewState& m_editViewState;
    te::Track::Ptr m_track;
    int m_trackHeightATMouseDown;
    int m_yPosAtMouseDown;
    double m_currentVolume = 0.0;
    juce::ValueTree inputsState;
    juce::Label m_trackName;
    juce::ToggleButton m_armButton,
                 m_muteButton,
                 m_soloButton;

    std::unique_ptr<AutomatableSliderComponent> m_volumeKnob;
    std::unique_ptr<LevelMeterComponent> levelMeterComp;
    juce::Image m_dragImage;
    bool m_isResizing {false},
         m_isAboutToResizing {false},
         m_contentIsOver {false},
         m_trackIsOver {false},
         m_isDragging {false},
         m_isAudioTrack {false},
         m_updateVolumeKnob {false},
         m_updateAutomationLanes {false},
         m_updateTrackHeight {false},
         m_isMinimized {false};
    void buildAutomationHeader();
    juce::OwnedArray<AutomationLaneHeaderComponent> m_automationHeaders;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackHeaderComponent)
};
