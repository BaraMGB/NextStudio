#include "DrumPadComponent.h"
#include "Utilities.h"
#include "Browser_Base.h"

namespace
{
    static constexpr int VISUAL_FEEDBACK_DURATION_MS = 100;
}

void DrumPad::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    area.reduce (5, 5);
    g.setColour(m_colour);
    g.fillRoundedRectangle(area, 5);

    // Draw the note name in the top-left corner
    area.reduce (2, 2);
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
        owner->showPadContextMenu(padIndex);
    else
        owner->buttonDown(padIndex);
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

void DrumPad::triggerVisualFeedback(juce::Colour triggerColour, juce::Colour returnColour)
{
    changeColour(triggerColour);
    m_returnColour = returnColour;
    // Timer runs on the message thread, so this is thread-safe
    startTimer(VISUAL_FEEDBACK_DURATION_MS);
}

void DrumPad::timerCallback()
{
    stopTimer();
    changeColour(m_returnColour);
}

// -----------------------------------------------------------------------

DrumPadComponent::DrumPadComponent(te::SamplerPlugin& plugin)
    : m_edit(plugin.edit)
    , m_samplerPlugin(plugin)
{
    GUIHelpers::log("DrumPadComponent: constructor");

    m_samplerPlugin.state.addListener(this);

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
    m_samplerPlugin.state.removeListener(this);
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
        if (soundIndex < m_samplerPlugin.getNumSounds())
        {
            auto soundName = m_samplerPlugin.getSoundName(soundIndex);
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
        juce::Colour returnColour = (padIndex == m_selectedPadIndex) ? 
                                     juce::Colours::blue : juce::Colours::grey;
        m_pads[padIndex]->triggerVisualFeedback(juce::Colours::lightblue, returnColour);
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

                                 if (soundIndex < m_samplerPlugin.getNumSounds())
                                 {
                                 GUIHelpers::log("DrumPadComponent: Setting soundMedia for existing slot " + juce::String(soundIndex));
                                 m_samplerPlugin.setSoundMedia(soundIndex, filePath);
                                 m_samplerPlugin.setSoundName(soundIndex, fileName);

                                 int midiNote = BASE_MIDI_NOTE + soundIndex;
                                 m_samplerPlugin.setSoundParams(soundIndex, midiNote, midiNote, midiNote);
                                 m_samplerPlugin.setSoundOpenEnded(soundIndex, true);
                                 }

                                 // Debug checks
                                 GUIHelpers::log("DrumPadComponent: getSoundMedia=" + m_samplerPlugin.getSoundMedia(soundIndex));
                                 auto af = m_samplerPlugin.getSoundFile(soundIndex);
                                 GUIHelpers::log("DrumPadComponent: AudioFile valid=" + juce::String(af.isValid() ? "true" : "false"));
                                 GUIHelpers::log("DrumPadComponent: getSoundName=" + m_samplerPlugin.getSoundName(soundIndex));

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
        updatePadNames();
}

void DrumPadComponent::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&)
{
    updatePadNames();
}

void DrumPadComponent::updatePadNames()
{
    GUIHelpers::log("DrumPadComponent: updatePadNames - " + juce::String(m_samplerPlugin.getNumSounds()) + " sounds available");

    for (int i = 0; i < m_pads.size(); ++i)
    {
        int soundIndex = getSoundIndexForPad(i);
        if (soundIndex < m_samplerPlugin.getNumSounds())
        {
            auto soundName = m_samplerPlugin.getSoundName(soundIndex);
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

                m_samplerPlugin.setSoundMedia(soundIndex, f.getFullPathName());
                m_samplerPlugin.setSoundName(soundIndex, f.getFileNameWithoutExtension());

                int midiNote = BASE_MIDI_NOTE + soundIndex;
                m_samplerPlugin.setSoundParams(soundIndex, midiNote, midiNote, midiNote);
                m_samplerPlugin.setSoundOpenEnded(soundIndex, true);

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
