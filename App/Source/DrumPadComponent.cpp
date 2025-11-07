#include "DrumPadComponent.h"
#include "Utilities.h"
#include "Browser_Base.h"

void DrumPad::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    area.reduce (5, 5);
    g.setColour(m_colour);
    g.fillRoundedRectangle(area, 5);

    // Draw the note name in the top-left corner
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(12.0f);
    g.drawText(owner->getMidiNoteNameForPad(padIndex), area, juce::Justification::topLeft);

    // Draw the sample name in the center
    g.setColour(juce::Colours::black);
    g.setFont(15.0f);
    g.drawText(m_text, area, juce::Justification::centred);
}

void DrumPad::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        owner->showPadContextMenu(padIndex);
    }
    else
    {
        owner->buttonDown(padIndex);
    }
}

void DrumPad::mouseUp(const juce::MouseEvent& e)
{
    if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*owner->getEdit()))
    {
        int soundIndex = owner->getSoundIndexForPad(padIndex);
        int midiNote = owner->BASE_MIDI_NOTE + soundIndex;
        virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOff(1, midiNote, .8f), 0);
    }
}

void DrumPad::changeColour(juce::Colour colour)
{
    m_colour = colour;
    repaint();
}

// -----------------------------------------------------------------------

DrumPadComponent::DrumPadComponent(te::SamplerPlugin& plugin)
    : m_edit(plugin.edit), samplerPlugin(&plugin)
{
    GUIHelpers::log("DrumPadComponent: constructor");

    samplerPlugin->state.addListener(this);

    for (int i = 0; i < 16; ++i)
    {
        auto pad = std::make_unique<DrumPad>(this, i);
        m_pads.add(pad.get());
        pad->changeColour(juce::Colours::grey);
        addAndMakeVisible(pad.release());
    }

    updatePadNames();
}

DrumPadComponent::~DrumPadComponent()
{
    GUIHelpers::log("DrumPadComponent: destructor");
    if (samplerPlugin)
        samplerPlugin->state.removeListener(this);
}

void DrumPadComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void DrumPadComponent::resized()
{
    auto bounds = getLocalBounds();
    auto padWidth = bounds.getWidth() / 4;
    auto padHeight = bounds.getHeight() / 4;

    for (int i = 0; i < m_pads.size(); ++i)
    {
        auto x = (i % 4) * padWidth;
        auto y = (i / 4) * padHeight;
        m_pads[i]->setBounds(x, y, padWidth, padHeight);
    }
}

void DrumPadComponent::buttonDown(int padIndex)
{
    if (onPadClicked)
        onPadClicked(padIndex);

    if (padIndex != -1)
    {
        // Update selection
        if (m_selectedPadIndex != -1 && m_selectedPadIndex < m_pads.size())
            m_pads[m_selectedPadIndex]->changeColour(juce::Colours::grey);

        m_selectedPadIndex = padIndex;
        int soundIndex = getSoundIndexForPad(padIndex);
        m_pads[m_selectedPadIndex]->changeColour(juce::Colours::blue);

        // Always play sound when clicked, regardless of whether it has media
        GUIHelpers::log("DrumPadComponent: Clicked pad " + juce::String(padIndex) + ", soundIndex: " + juce::String(soundIndex));

        // Check if this pad has a valid sound
        if (soundIndex < samplerPlugin->getNumSounds())
        {
            auto soundName = samplerPlugin->getSoundName(soundIndex);
            GUIHelpers::log("DrumPadComponent: Sound at soundIndex " + juce::String(soundIndex) + ": " + soundName);

            if (!soundName.isEmpty() && soundName != "Empty")
            {
                int midiNote = BASE_MIDI_NOTE + soundIndex;

                if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*getEdit()))
                    virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, midiNote, .8f), 0);
                GUIHelpers::log("DrumPadComponent: Playing note " + juce::String(midiNote));
            }
            else
            {
                GUIHelpers::log("DrumPadComponent: No valid sound at soundIndex " + juce::String(soundIndex));
            }
        }
        else
        {
            GUIHelpers::log("DrumPadComponent: Sound index " + juce::String(soundIndex) + " out of range");
        }

        // Visual feedback
        m_pads[padIndex]->changeColour(juce::Colours::lightblue);
        juce::Timer::callAfterDelay(100, [this, padIndex]()
                                    {
                                    if (padIndex == m_selectedPadIndex)
                                    m_pads[padIndex]->changeColour(juce::Colours::blue);
                                    else
                                    m_pads[padIndex]->changeColour(juce::Colours::grey);
                                    });
    }
}

void DrumPadComponent::showPadContextMenu(int padIndex)
{
    GUIHelpers::log("DrumPadComponent: Right click on pad " + juce::String(padIndex));

    juce::PopupMenu menu;
    menu.addItem("Load Sample", [this, padIndex]()
                 {
                 int soundIndex = getSoundIndexForPad(padIndex);
                 GUIHelpers::log("DrumPadComponent: Opening file chooser for pad " + juce::String(padIndex) + " (soundIndex: " + juce::String(soundIndex) + ")");

                 auto fc = std::make_shared<juce::FileChooser>("Select a sample to load",
                                                               juce::File(),
                                                               "*.wav;*.aif;*.aiff");

                 fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                 [this, soundIndex, chooser = fc](const juce::FileChooser& fc)
                                 {
                                 if (fc.getResults().isEmpty()) 
                                 {
                                 GUIHelpers::log("DrumPadComponent: File selection cancelled");
                                 return;
                                 }

                                 auto file = fc.getResult();
                                 GUIHelpers::log("DrumPadComponent: Selected file: " + file.getFullPathName());

                                 try
                                 {
                                 if (!file.existsAsFile())
                                 {
                                 GUIHelpers::log("DrumPadComponent: File does not exist: " + file.getFullPathName());
                                 return;
                                 }

                                 auto filePath = file.getFullPathName();
                                 auto fileName = file.getFileNameWithoutExtension();

                                 if (soundIndex < samplerPlugin->getNumSounds())
                                 {
                                 GUIHelpers::log("DrumPadComponent: Setting soundMedia for existing slot " + juce::String(soundIndex));
                                 samplerPlugin->setSoundMedia(soundIndex, filePath);
                                 samplerPlugin->setSoundName(soundIndex, fileName);

                                 int midiNote = BASE_MIDI_NOTE + soundIndex;
                                 samplerPlugin->setSoundParams(soundIndex, midiNote, midiNote, midiNote);
                                 }

                                 // Debug checks
                                 GUIHelpers::log("DrumPadComponent: getSoundMedia=" + samplerPlugin->getSoundMedia(soundIndex));
                                 auto af = samplerPlugin->getSoundFile(soundIndex);
                                 GUIHelpers::log("DrumPadComponent: AudioFile valid=" + juce::String(af.isValid() ? "true" : "false"));
                                 GUIHelpers::log("DrumPadComponent: getSoundName=" + samplerPlugin->getSoundName(soundIndex));

                                 GUIHelpers::log("DrumPadComponent: Updating pad names");
                                 updatePadNames();
                                 }
                                 catch (const std::exception& e)
                                 {
                                 GUIHelpers::log("DrumPadComponent: Exception while loading sound: " + juce::String(e.what()));
                                 }
                                 catch (...)
                                 {
                                 GUIHelpers::log("DrumPadComponent: Unknown exception while loading sound");
                                 }
                                 });
                 });

    menu.show();
}

void DrumPadComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        auto* pad = dynamic_cast<DrumPad*>(getComponentAt(e.x, e.y));
        if (pad)
        {
            auto padIndex = m_pads.indexOf(pad);
            if (padIndex != -1)
            {
                showPadContextMenu(padIndex);
            }
        }
    }
}

void DrumPadComponent::parentHierarchyChanged()
{
    if (getParentComponent() != nullptr)
    {
        updatePadNames();
    }
}

void DrumPadComponent::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&)
{
    updatePadNames();
}

void DrumPadComponent::updatePadNames()
{
    if (!samplerPlugin)
    {
        GUIHelpers::log("DrumPadComponent: updatePadNames - SamplerPlugin is null!");
        return;
    }

    GUIHelpers::log("DrumPadComponent: updatePadNames - " + juce::String(samplerPlugin->getNumSounds()) + " sounds available");

    for (int i = 0; i < m_pads.size(); ++i)
    {
        int soundIndex = getSoundIndexForPad(i);
        if (soundIndex < samplerPlugin->getNumSounds())
        {
            auto soundName = samplerPlugin->getSoundName(soundIndex);
            if (soundName.isEmpty() || soundName == "Empty")
                m_pads[i]->setText("+");
            else
                m_pads[i]->setText(soundName);
            GUIHelpers::log("DrumPadComponent: Pad " + juce::String(i) + " (sound " + juce::String(soundIndex) + ") name: " + soundName);
        }
        else
        {
            m_pads[i]->setText("+");
        }
    }
}

int DrumPadComponent::getNeededWidth()
{
    return 4;
}

juce::String DrumPadComponent::getMidiNoteNameForPad(int padIndex)
{
    int soundIndex = getSoundIndexForPad(padIndex);
    int midiNote = BASE_MIDI_NOTE + soundIndex;
    return juce::MidiMessage::getMidiNoteName(midiNote, true, true, 3);
}

int DrumPadComponent::getSoundIndexForPad(int padIndex)
{
    const int row = padIndex / 4;
    const int col = padIndex % 4;
    const int mpcRow = 3 - row;
    return mpcRow * 4 + col;
}

//==============================================================================
// Drag and Drop
//==============================================================================

bool DrumPadComponent::isInterestedInDragSource (const SourceDetails& dragSourceDetails)
{
    return dragSourceDetails.description == "SampleBrowser";
}

void DrumPadComponent::itemDragEnter (const SourceDetails& dragSourceDetails)
{
    if (auto* pad = dynamic_cast<DrumPad*>(getComponentAt(dragSourceDetails.localPosition)))
    {
        m_draggedOverPad = m_pads.indexOf(pad);
        pad->changeColour(juce::Colours::orange);
    }
}

void DrumPadComponent::itemDragMove (const SourceDetails& dragSourceDetails)
{
    auto* padUnderCursor = dynamic_cast<DrumPad*>(getComponentAt(dragSourceDetails.localPosition));
    int padIndex = padUnderCursor ? m_pads.indexOf(padUnderCursor) : -1;

    if (padIndex != m_draggedOverPad)
    {
        if (m_draggedOverPad != -1)
        {
            if (m_draggedOverPad == m_selectedPadIndex)
                m_pads[m_draggedOverPad]->changeColour(juce::Colours::blue);
            else
                m_pads[m_draggedOverPad]->changeColour(juce::Colours::grey);
        }

        m_draggedOverPad = padIndex;

        if (m_draggedOverPad != -1)
            m_pads[m_draggedOverPad]->changeColour(juce::Colours::orange);
    }
}

void DrumPadComponent::itemDragExit (const SourceDetails&)
{
    if (m_draggedOverPad != -1)
    {
        if (m_draggedOverPad == m_selectedPadIndex)
            m_pads[m_draggedOverPad]->changeColour(juce::Colours::blue);
        else
            m_pads[m_draggedOverPad]->changeColour(juce::Colours::grey);
        m_draggedOverPad = -1;
    }
}

void DrumPadComponent::itemDropped (const SourceDetails& dragSourceDetails)
{
    juce::File f;
    if (auto* browser = dynamic_cast<BrowserListBox*>(dragSourceDetails.sourceComponent.get()))
        f = browser->getSelectedFile();

    if (f.existsAsFile())
    {
        if (auto* pad = dynamic_cast<DrumPad*>(getComponentAt(dragSourceDetails.localPosition)))
        {
            int padIndex = m_pads.indexOf(pad);
            if (padIndex != -1)
            {
                int soundIndex = getSoundIndexForPad(padIndex);
                GUIHelpers::log("DrumPadComponent: Dropped file " + f.getFileName() + " on pad " + juce::String(padIndex) + " (soundIndex: " + juce::String(soundIndex) + ")");

                samplerPlugin->setSoundMedia(soundIndex, f.getFullPathName());
                samplerPlugin->setSoundName(soundIndex, f.getFileNameWithoutExtension());

                int midiNote = BASE_MIDI_NOTE + soundIndex;
                samplerPlugin->setSoundParams(soundIndex, midiNote, midiNote, midiNote);

                updatePadNames();
            }
        }
    }

    if (m_draggedOverPad != -1)
    {
        if (m_draggedOverPad == m_selectedPadIndex)
            m_pads[m_draggedOverPad]->changeColour(juce::Colours::blue);
        else
            m_pads[m_draggedOverPad]->changeColour(juce::Colours::grey);
        m_draggedOverPad = -1;
    }
}
