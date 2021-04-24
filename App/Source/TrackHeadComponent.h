#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"
#include "LevelMeterComponent.h"
#include "PluginRackComponent.h"

namespace te = tracktion_engine;

class TrackHeaderComponent : public juce::Component
                           , private te::ValueTreeAllEventListener
                           , public juce::ChangeBroadcaster
                           , public juce::DragAndDropTarget
                           , public juce::Slider::Listener
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

    void sliderValueChanged(juce::Slider *slider) override;
    void labelTextChanged(juce::Label *labelThatHasChanged);
    void childrenSetVisible(bool v);
private:
    void valueTreeChanged() override {}
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;

    void showPopupMenu(te::AudioTrack* at);
    void deleteTrackFromEdit();

    EditViewState& m_editViewState;
    te::Track::Ptr m_track;
    int m_trackHeightATMouseDown;
    int m_yPosAtMouseDown;

    juce::ValueTree inputsState;
    juce::Label m_trackName;
    juce::ToggleButton m_armButton,
                 m_muteButton,
                 m_soloButton;

    juce::Slider       m_volumeKnob;
    std::unique_ptr<LevelMeterComponent> levelMeterComp;

    juce::Image m_dragImage;
    bool m_isResizing {false},
         m_isAboutToResizing {false},
         m_contentIsOver {false},
         m_trackIsOver {false},
         m_isDragging {false},
         m_isAudioTrack {false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackHeaderComponent)
};
