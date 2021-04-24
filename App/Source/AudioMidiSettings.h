#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"


namespace te = tracktion_engine;

class AudioMidiSettings : public juce::Component
                        , public juce::ComboBox::Listener
{
public:

    AudioMidiSettings(te::Engine& engine);

//    void paint (juce::Graphics& g) override;
    void resized () override;
//    void mouseDown (const juce::MouseEvent&) override;
//    void mouseDrag (const juce::MouseEvent &) override;
//    void mouseUp (const juce::MouseEvent &) override;
    void comboBoxChanged(juce::ComboBox *comboBox) override;

protected:

private:

    juce::AudioDeviceSelectorComponent m_audioSettings;
    juce::ComboBox m_midiDefaultChooser;
    te::Engine& m_engine;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioMidiSettings)
};
