#include "AudioMidiSettings.h"

AudioMidiSettings::AudioMidiSettings(tracktion_engine::Engine &engine) : m_audioSettings(engine.getDeviceManager()
                                                                                         .deviceManager
                                                                                         , 0
                                                                                         , 512
                                                                                         , 1
                                                                                         , 512
                                                                                         , false, false, false, false)
  , m_engine(engine)

{
    addAndMakeVisible (m_audioSettings);
    addAndMakeVisible (m_midiDefaultChooser);
    m_midiDefaultChooser.addListener (this);
    auto& dm = engine.getDeviceManager ();
    for (int i = 0; i < dm.getNumMidiInDevices(); i++)
    {
        if (auto mip = dm.getMidiInDevice (i))
        {
            m_midiDefaultChooser.addItem (mip->getName (), i + 1);
            if (dm.getDefaultMidiInDevice () == mip)
            {
                m_midiDefaultChooser.setSelectedId (i + 1);
            }
        }
    }

}

void AudioMidiSettings::resized()
{
    auto area = getLocalBounds ();
    m_audioSettings.setBounds(area.removeFromTop (getHeight ()/2));
    m_midiDefaultChooser.setBounds (area.removeFromTop (50));
}

void AudioMidiSettings::comboBoxChanged(juce::ComboBox *comboBox)
{
    auto& dm = m_engine.getDeviceManager ();
    dm.setDefaultMidiInDevice (comboBox->getSelectedId () - 1);
    std::cout << " ams: " << dm.getDefaultMidiInDevice ()->getName ()<< std::endl;
}
