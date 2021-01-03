#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"
#include "LevelMeterComponent.h"
#include "PluginRackComponent.h"

namespace te = tracktion_engine;

class TrackHeaderComponent : public juce::Component,
                             private te::ValueTreeAllEventListener,
                             public juce::ChangeBroadcaster
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
    juce::Colour getTrackColour();

    te::Track::Ptr getTrack() const;

    void updateMidiInputs();
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

    bool drawOverlayTrackColour {false},
         m_isResizing {false},
         m_isAboutToResizing {false};
};

//==============================================================================
