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

    // Highlight if drag target
    if (m_isDragTarget)
    {
        g.setColour(juce::Colours::orange.brighter());
        g.drawRoundedRectangle(area, 5, 3);
    }

    g.setColour(m_colour);
    g.fillRoundedRectangle(area, 5);

    // Draw the note name in the top-left corner
    area.reduce (2, 2);
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(12.0f);
    g.drawText(owner->getMidiNoteNameForPad(padIndex), area, juce::Justification::topLeft);

    // Draw the sample name in the center
    g.setColour(juce::Colours::black);
    g.setFont(9.0f);
    g.drawText(m_text, area, juce::Justification::centred);
}

void DrumPad::mouseDown(const juce::MouseEvent& e)
{
    m_dragStartPos = e.getMouseDownPosition();
    m_isDragging = false;

    if (e.mods.isRightButtonDown())
        owner->showPadContextMenu(padIndex);
    else
        owner->buttonDown(padIndex);
}

void DrumPad::mouseDrag(const juce::MouseEvent& e)
{
    if (!e.mods.isLeftButtonDown())
        return;

    auto dragDistance = e.getDistanceFromDragStart();
    if (!m_isDragging && dragDistance > 10)
    {
        m_isDragging = true;
        owner->startPadDrag(padIndex, e);
    }

    if (m_isDragging)
    {
        owner->continuePadDrag(e);
    }
}

void DrumPad::mouseUp(const juce::MouseEvent& e)
{
    if (m_isDragging)
    {
        owner->endPadDrag(e);
        m_isDragging = false;
    }
    else
    {
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*owner->getEdit()))
        {
            int soundIndex = owner->getSoundIndexForPad(padIndex);

            // Get the actual MIDI note assigned to this sound (not calculated from soundIndex)
            int midiNote = owner->getSoundKeyNote(soundIndex);

            // If keyNote is not set (default -1), fall back to the pad's default note
            if (midiNote < 0)
            {
                midiNote = owner->BASE_MIDI_NOTE + padIndex;
                GUIHelpers::log("DrumPad::mouseUp: Using fallback MIDI note " + juce::String(midiNote) + " for padIndex " + juce::String(padIndex));
            }
            else
            {
                GUIHelpers::log("DrumPad::mouseUp: Using stored MIDI note " + juce::String(midiNote) + " for soundIndex " + juce::String(soundIndex));
            }

            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOff(1, midiNote, .8f), 0);
        }
    }
}

void DrumPad::mouseEnter(const juce::MouseEvent& e)
{
    if (owner->isPadDragging() && padIndex != owner->getDragSourcePad())
    {
        setIsDragTarget(true);
    }
}

void DrumPad::mouseExit(const juce::MouseEvent& e)
{
    setIsDragTarget(false);
}

void DrumPad::setIsDragTarget(bool isTarget)
{
    if (m_isDragTarget != isTarget)
    {
        m_isDragTarget = isTarget;
        repaint();
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

    // Initialize MIDI Input Devices
    setupMidiInputDevices();

    updatePadNames();
}

DrumPadComponent::~DrumPadComponent()
{
    GUIHelpers::log("DrumPadComponent: destructor");

    // Cleanup MIDI Input Devices
    cleanupMidiInputDevices();

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
        auto y = (3 - (i / 4)) * padHeight; // Invert y-axis for bottom-up layout
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
                // Get the actual MIDI note assigned to this sound (not calculated from soundIndex)
                int midiNote = m_samplerPlugin.getKeyNote(soundIndex);

                // If keyNote is not set (default -1), fall back to the pad's default note
                if (midiNote < 0)
                {
                    midiNote = BASE_MIDI_NOTE + padIndex;
                    GUIHelpers::log("DrumPadComponent: Using fallback MIDI note " + juce::String(midiNote) + " for padIndex " + juce::String(padIndex));
                }
                else
                {
                    GUIHelpers::log("DrumPadComponent: Using stored MIDI note " + juce::String(midiNote) + " for soundIndex " + juce::String(soundIndex));
                }

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
                                         setupNewSample(soundIndex, file);
                                     });
                     });
    menu.addSeparator();

    menu.addItem("Clear", [this, padIndex]()
                 {
                 int soundIndex = getSoundIndexForPad(padIndex);
                 GUIHelpers::log("DrumPadComponent: Clearing pad " + juce::String(padIndex) + " (soundIndex: " + juce::String(soundIndex) + ")");

                 if (soundIndex < m_samplerPlugin.getNumSounds())
                 {
                     // Clear the sound by setting empty media and name
                     m_samplerPlugin.setSoundMedia(soundIndex, "");
                     m_samplerPlugin.setSoundName(soundIndex, "Empty");

                     // Reset MIDI parameters to defaults
                     int midiNote = BASE_MIDI_NOTE + soundIndex;
                     m_samplerPlugin.setSoundParams(soundIndex, midiNote, midiNote, midiNote);
                     m_samplerPlugin.setSoundOpenEnded(soundIndex, true);

                     GUIHelpers::log("DrumPadComponent: Cleared sound at index " + juce::String(soundIndex));
                     updatePadNames();

                     // Repaint the pad to show the updated text immediately
                     if (padIndex >= 0 && padIndex < m_pads.size())
                         m_pads[padIndex]->repaint();
                 }
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
                showPadContextMenu(padIndex);
        }
    }
}

void DrumPadComponent::mouseDrag(const juce::MouseEvent& e)
{
    // Handle drag image movement if we're dragging
    if (m_isPadDragging && m_dragImageComponent)
    {
        auto pos = getLocalPoint(nullptr, e.getScreenPosition());
        m_dragImageComponent->setCentrePosition(pos.x, pos.y);
    }
}

void DrumPadComponent::mouseUp(const juce::MouseEvent& e)
{
    // This will be handled by the individual pad's mouseUp
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
    const int midiNote = BASE_MIDI_NOTE + padIndex;
    return juce::MidiMessage::getMidiNoteName(midiNote, true, true, 3);
}

int DrumPadComponent::getSoundIndexForPad(int padIndex)
{
    // With the new resized logic, padIndex and soundIndex are now the same.
    // Pad 0 (bottom-left) corresponds to sound 0.
    return padIndex;
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
                setupNewSample(soundIndex, f);
            }
        }
    }

    if (m_draggedOverPad != -1)
    {
        if (m_draggedOverPad == m_selectedPadIndex)
        {    
            m_pads[m_draggedOverPad]->changeColour(juce::Colours::blue);
        }
        else
        {
            m_pads[m_draggedOverPad]->changeColour(juce::Colours::grey);
        }
        m_draggedOverPad = -1;
    }
}

//==============================================================================
// Pad-to-Pad Drag & Drop Implementation
//==============================================================================

void DrumPadComponent::startPadDrag(int sourcePadIndex, const juce::MouseEvent& event)
{
    if (sourcePadIndex < 0 || sourcePadIndex >= m_pads.size())
        return;

    int soundIndex = getSoundIndexForPad(sourcePadIndex);
    if (soundIndex >= m_samplerPlugin.getNumSounds())
        return;

    auto soundName = m_samplerPlugin.getSoundName(soundIndex);
    if (soundName.isEmpty() || soundName == "Empty")
        return; // Don't drag empty pads

    GUIHelpers::log("DrumPadComponent: Starting drag from pad " + juce::String(sourcePadIndex) +
                   " (sound: " + soundName + ")");

    m_isPadDragging = true;
    m_dragSourcePad = sourcePadIndex;
    m_dragSourceSoundIndex = soundIndex;

    // Create drag image component with visual representation
    struct DragImagePainter : public juce::Component
    {
        int sourcePadIndex;
        juce::String soundName;

        DragImagePainter(int pad, const juce::String& name)
            : sourcePadIndex(pad), soundName(name)
        {
            setSize(50, 50);
            setAlwaysOnTop(true);
            setOpaque(false);
        }

        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            area.reduce (5, 5);

            // Semi-transparent background
            g.setColour(juce::Colours::black.withAlpha(0.7f));
            g.fillRoundedRectangle(area, 8);

            // Border
            g.setColour(juce::Colours::orange);
            g.drawRoundedRectangle(area, 8, 2);

            // Pad info
            area.reduce (5, 5);
            g.setColour(juce::Colours::white);
            g.setFont(12.0f);
            g.drawText("Pad " + juce::String(sourcePadIndex), area.removeFromTop(20), juce::Justification::centred);

            g.setFont(14.0f);
            g.drawText(soundName, area, juce::Justification::centred);
        }
    };

    m_dragImageComponent = std::make_unique<DragImagePainter>(sourcePadIndex, soundName);

    // Add to parent component for proper layering
    if (auto parent = getParentComponent())
    {
        parent->addAndMakeVisible(m_dragImageComponent.get());
    }
    else
    {
        addAndMakeVisible(m_dragImageComponent.get());
    }

    // Position the drag image
    auto pos = getLocalPoint(nullptr, event.getScreenPosition());
    m_dragImageComponent->setCentrePosition(pos.x, pos.y);

    // Set cursor
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);

    // Visual feedback on source pad
    m_pads[sourcePadIndex]->changeColour(juce::Colours::orange.withAlpha(0.5f));
}

void DrumPadComponent::continuePadDrag(const juce::MouseEvent& event)
{
    if (!m_isPadDragging || !m_dragImageComponent)
        return;

    // Update drag image position
    auto pos = getLocalPoint(nullptr, event.getScreenPosition());
    m_dragImageComponent->setCentrePosition(pos.x, pos.y);

    // Check which pad we're over - convert to global coordinates first
    auto globalPos = event.getScreenPosition();
    auto localPos = getLocalPoint(nullptr, globalPos);

    // Find which pad contains this point
    int targetPadIndex = -1;
    for (int i = 0; i < m_pads.size(); ++i)
    {
        if (m_pads[i]->getBounds().contains(localPos))
        {
            targetPadIndex = i;
            break;
        }
    }

    // Update drag target states
    for (int i = 0; i < m_pads.size(); ++i)
    {
        bool shouldBeTarget = (i == targetPadIndex && i != m_dragSourcePad);
        m_pads[i]->setIsDragTarget(shouldBeTarget);
    }
}

void DrumPadComponent::endPadDrag(const juce::MouseEvent& event)
{
    if (!m_isPadDragging)
        return;

    GUIHelpers::log("DrumPadComponent: Ending drag from pad " + juce::String(m_dragSourcePad));

    // Check which pad we're dropping on - use same logic as continuePadDrag
    auto globalPos = event.getScreenPosition();
    auto localPos = getLocalPoint(nullptr, globalPos);

    int targetPadIndex = -1;
    for (int i = 0; i < m_pads.size(); ++i)
    {
        if (m_pads[i]->getBounds().contains(localPos))
        {
            targetPadIndex = i;
            break;
        }
    }

    if (targetPadIndex >= 0 && targetPadIndex != m_dragSourcePad)
    {
        GUIHelpers::log("DrumPadComponent: Dropping on pad " + juce::String(targetPadIndex));
        swapPadSounds(m_dragSourcePad, targetPadIndex);

        // Visual feedback on successful drop
        m_pads[targetPadIndex]->triggerVisualFeedback(juce::Colours::green,
                                                     targetPadIndex == m_selectedPadIndex ?
                                                     juce::Colours::blue : juce::Colours::grey);
        buttonDown(targetPadIndex);
    }

    // Restore source pad color
    if (m_dragSourcePad >= 0 && m_dragSourcePad < m_pads.size())
    {
        juce::Colour sourceColor = (m_dragSourcePad == m_selectedPadIndex) ?
                                   juce::Colours::blue : juce::Colours::grey;
        m_pads[m_dragSourcePad]->changeColour(sourceColor);
    }

    // Clean up drag state
    m_isPadDragging = false;
    m_dragSourcePad = -1;
    m_dragSourceSoundIndex = -1;

    // Remove drag image with fade effect
    if (m_dragImageComponent)
    {
        // Simple fade out effect - just remove immediately for now
        if (auto parent = m_dragImageComponent->getParentComponent())
            parent->removeChildComponent(m_dragImageComponent.get());

        m_dragImageComponent.reset();
    }

    // Clear all drag target states
    for (int i = 0; i < m_pads.size(); ++i)
    {
        m_pads[i]->setIsDragTarget(false);
    }

    // Reset cursor
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void DrumPadComponent::swapPadSounds(int sourcePad, int targetPad)
{
    if (sourcePad < 0 || sourcePad >= m_pads.size() || 
        targetPad < 0 || targetPad >= m_pads.size())
        return;

    int sourceSoundIndex = getSoundIndexForPad(sourcePad);
    int targetSoundIndex = getSoundIndexForPad(targetPad);

    if (sourceSoundIndex >= m_samplerPlugin.getNumSounds() || 
        targetSoundIndex >= m_samplerPlugin.getNumSounds())
        return;

    GUIHelpers::log("DrumPadComponent: Swapping sounds between pad " + juce::String(sourcePad) +
                   " (sound " + juce::String(sourceSoundIndex) + ") and pad " + juce::String(targetPad) +
                   " (sound " + juce::String(targetSoundIndex) + ")");

    // Get all data from source and target sounds
    auto sourceData = getPadSoundData(sourceSoundIndex);
    auto targetData = getPadSoundData(targetSoundIndex);

    // Apply swapped data
    applyPadSoundData(targetSoundIndex, sourceData);
    applyPadSoundData(sourceSoundIndex, targetData);

    // MIDI parameters are tied to the pad index, not the sound data, so we re-apply them
    const int targetMidiNote = BASE_MIDI_NOTE + targetSoundIndex;
    m_samplerPlugin.setSoundParams(targetSoundIndex, targetMidiNote, targetMidiNote, targetMidiNote);

    const int sourceMidiNote = BASE_MIDI_NOTE + sourceSoundIndex;
    m_samplerPlugin.setSoundParams(sourceSoundIndex, sourceMidiNote, sourceMidiNote, sourceMidiNote);

    GUIHelpers::log("DrumPadComponent: Sound swap - sourceSound: " + juce::String(sourceSoundIndex) +
                   " -> new midiNote: " + juce::String(sourceMidiNote) +
                   ", targetSound: " + juce::String(targetSoundIndex) +
                   " -> new midiNote: " + juce::String(targetMidiNote));

    // Update UI
    updatePadNames();

    GUIHelpers::log("DrumPadComponent: Sound swap completed");
}

//==============================================================================
// MIDI Input Device Management
//==============================================================================

void DrumPadComponent::setupMidiInputDevices()
{
    GUIHelpers::log("DrumPadComponent: Setting up MIDI input devices");

    auto& deviceManager = m_edit.engine.getDeviceManager();

    // Rescan for MIDI devices to ensure we have the latest list
    deviceManager.rescanMidiDeviceList();

    auto midiDevices = deviceManager.getMidiInDevices();
    GUIHelpers::log("DrumPadComponent: Found " + juce::String(midiDevices.size()) + " MIDI input devices");

    for (int i = 0; i < midiDevices.size(); ++i)
    {
        auto& device = midiDevices[i];
        GUIHelpers::log("DrumPadComponent: Device " + juce::String(i) + ": " + device->getName() +
                       " (Type: " + juce::String(device->getDeviceType()) + ")");

        if (auto physicalDevice = dynamic_cast<te::PhysicalMidiInputDevice*>(device.get()))
        {
            GUIHelpers::log("DrumPadComponent: Found physical MIDI device: " + physicalDevice->getName());

            // Enable the device and add as listener
            physicalDevice->setEnabled(true);
            physicalDevice->addListener(this);
            m_connectedMidiDevices.add(physicalDevice);

            // Try to open the device
            auto error = physicalDevice->openDevice();
            if (!error.isEmpty())
            {
                GUIHelpers::log("DrumPadComponent: Error opening MIDI device " +
                               physicalDevice->getName() + ": " + error);
            }
            else
            {
                GUIHelpers::log("DrumPadComponent: Successfully connected to MIDI device: " +
                               physicalDevice->getName());
            }
        }
        else
        {
            GUIHelpers::log("DrumPadComponent: Device " + device->getName() + " is not a physical MIDI device");
        }
    }

    if (m_connectedMidiDevices.isEmpty())
    {
        GUIHelpers::log("DrumPadComponent: No physical MIDI devices found - pad lighting will only work with virtual MIDI");
    }
    else
    {
        GUIHelpers::log("DrumPadComponent: Connected to " + juce::String(m_connectedMidiDevices.size()) + " MIDI devices");
    }
}

void DrumPadComponent::cleanupMidiInputDevices()
{
    GUIHelpers::log("DrumPadComponent: Cleaning up MIDI input devices");

    for (auto* device : m_connectedMidiDevices)
    {
        if (device != nullptr)
        {
            device->removeListener(this);
            GUIHelpers::log("DrumPadComponent: Disconnected from MIDI device: " + device->getName());
        }
    }

    m_connectedMidiDevices.clear();
}

//==============================================================================
// MIDI Input Callback Implementation
//==============================================================================

void DrumPadComponent::handleIncomingMidiMessage(const juce::MidiMessage& message)
{
    // This callback is called from the Tracktion Engine MIDI thread
    // We need to safely forward this to the UI thread

    if (message.isNoteOn())
    {
        GUIHelpers::log("DrumPadComponent: MIDI Note ON received - Note: " +
                       juce::String(message.getNoteNumber()) +
                       ", Velocity: " + juce::String(message.getFloatVelocity()) +
                       ", Channel: " + juce::String(message.getChannel()));
    }
    else if (message.isNoteOff())
    {
        GUIHelpers::log("DrumPadComponent: MIDI Note OFF received - Note: " +
                       juce::String(message.getNoteNumber()) +
                       ", Channel: " + juce::String(message.getChannel()));
    }
    else
    {
        GUIHelpers::log("DrumPadComponent: Other MIDI message received: " + message.getDescription());
    }

    juce::MessageManager::callAsync([this, message]() {
        processMidiForPadLighting(message);
    });
}

void DrumPadComponent::processMidiForPadLighting(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        int noteNumber = message.getNoteNumber();
        float velocity = message.getFloatVelocity();

        GUIHelpers::log("DrumPadComponent: Processing Note ON - Note: " + juce::String(noteNumber) +
                       ", Velocity: " + juce::String(velocity));

        int padIndex = getPadIndexForMidiNote(noteNumber);
        GUIHelpers::log("DrumPadComponent: Note " + juce::String(noteNumber) +
                       " maps to pad index " + juce::String(padIndex));

        if (padIndex >= 0 && padIndex < m_pads.size())
        {
            GUIHelpers::log("DrumPadComponent: Illuminating pad " + juce::String(padIndex));
            illuminatePadForNote(noteNumber, velocity);
        }
        else
        {
            GUIHelpers::log("DrumPadComponent: No valid pad for note " + juce::String(noteNumber));
        }
    }
    else if (message.isNoteOff())
    {
        int noteNumber = message.getNoteNumber();

        GUIHelpers::log("DrumPadComponent: Processing Note OFF - Note: " + juce::String(noteNumber));

        int padIndex = getPadIndexForMidiNote(noteNumber);
        GUIHelpers::log("DrumPadComponent: Note " + juce::String(noteNumber) +
                       " maps to pad index " + juce::String(padIndex));

        if (padIndex >= 0 && padIndex < m_pads.size())
        {
            GUIHelpers::log("DrumPadComponent: Turning off pad " + juce::String(padIndex));
            turnOffPadForNote(noteNumber);
        }
        else
        {
            GUIHelpers::log("DrumPadComponent: No valid pad for note " + juce::String(noteNumber));
        }
    }
}

void DrumPadComponent::illuminatePadForNote(int midiNote, float velocity)
{
    int padIndex = getPadIndexForMidiNote(midiNote);

    if (padIndex >= 0 && padIndex < m_pads.size())
    {
        // Calculate color based on velocity (darker for softer, brighter for louder)
        juce::Colour baseColor = juce::Colours::orange;
        float brightness = juce::jlimit(0.3f, 1.0f, 0.3f + velocity * 0.7f);
        juce::Colour velocityColor = baseColor.withBrightness(brightness);

        // Get the return color (selected pad stays blue, others grey)
        juce::Colour returnColor = (padIndex == m_selectedPadIndex) ?
                                   juce::Colours::blue : juce::Colours::grey;

        // Trigger visual feedback with velocity-based color
        m_pads[padIndex]->triggerVisualFeedback(velocityColor, returnColor);

        GUIHelpers::log("DrumPadComponent: Illuminated pad " + juce::String(padIndex) +
                       " for MIDI note " + juce::String(midiNote) +
                       " with velocity " + juce::String(velocity));
    }
    else
    {
        GUIHelpers::log("DrumPadComponent: No pad found for MIDI note " + juce::String(midiNote));
    }
}

void DrumPadComponent::turnOffPadForNote(int midiNote)
{
    int padIndex = getPadIndexForMidiNote(midiNote);

    if (padIndex >= 0 && padIndex < m_pads.size())
    {
        // Return to normal color
        juce::Colour returnColor = (padIndex == m_selectedPadIndex) ?
                                   juce::Colours::blue : juce::Colours::grey;
        m_pads[padIndex]->changeColour(returnColor);

        GUIHelpers::log("DrumPadComponent: Turned off pad " + juce::String(padIndex) +
                       " for MIDI note " + juce::String(midiNote));
    }
}

int DrumPadComponent::getPadIndexForMidiNote(int midiNote)
{
    // Only check for exact keyNote matches - this is the correct behavior for drum pads
    for (int soundIndex = 0; soundIndex < m_samplerPlugin.getNumSounds(); ++soundIndex)
    {
        int keyNote = m_samplerPlugin.getKeyNote(soundIndex);

        // Check if this sound has a valid keyNote assigned (not default -1)
        if (keyNote >= 0 && keyNote == midiNote)
        {
            GUIHelpers::log("DrumPadComponent: Found exact match - MIDI note " + juce::String(midiNote) +
                           " -> sound index " + juce::String(soundIndex));
            return getPadIndexForSound(soundIndex);
        }
    }

    // For drum pads, we should NOT use range-based matching or wide fallbacks
    // Only use the standard mapping if no custom keyNotes are set at all
    bool hasCustomKeyNotes = false;
    for (int soundIndex = 0; soundIndex < m_samplerPlugin.getNumSounds(); ++soundIndex)
    {
        if (m_samplerPlugin.getKeyNote(soundIndex) >= 0)
        {
            hasCustomKeyNotes = true;
            break;
        }
    }

    // Always use default mapping for pads without custom keyNotes
    // This ensures pads light up even when no sound is loaded
    if (midiNote >= BASE_MIDI_NOTE && midiNote < BASE_MIDI_NOTE + 16)
    {
        int soundIndex = midiNote - BASE_MIDI_NOTE;

        // Check if this pad has a custom keyNote - if so, don't use default mapping
        if (soundIndex < m_samplerPlugin.getNumSounds())
        {
            int customKeyNote = m_samplerPlugin.getKeyNote(soundIndex);
            if (customKeyNote >= 0)
            {
                GUIHelpers::log("DrumPadComponent: Pad " + juce::String(soundIndex) +
                               " has custom keyNote " + juce::String(customKeyNote) +
                               " - skipping default mapping for MIDI note " + juce::String(midiNote));
                return -1; // Don't use default mapping for pads with custom keyNotes
            }
        }

        GUIHelpers::log("DrumPadComponent: Using default mapping - MIDI note " + juce::String(midiNote) +
                       " -> sound index " + juce::String(soundIndex));
        return getPadIndexForSound(soundIndex);
    }

    GUIHelpers::log("DrumPadComponent: No pad found for MIDI note " + juce::String(midiNote));
    return -1; // No pad found for this note
}

int DrumPadComponent::getPadIndexForSound(int soundIndex)
{
    // With the corrected resized() logic, padIndex and soundIndex are now the same.
    return soundIndex;
}

//==============================================================================
// Private Helper Functions
//==============================================================================

void DrumPadComponent::setupNewSample(int soundIndex, const juce::File& file)
{
    if (soundIndex < 0 || soundIndex >= m_samplerPlugin.getNumSounds())
    {
        GUIHelpers::log("DrumPadComponent::setupNewSample: Invalid soundIndex " + juce::String(soundIndex));
        return;
    }

    if (!file.existsAsFile())
    {
        GUIHelpers::log("DrumPadComponent::setupNewSample: File does not exist: " + file.getFullPathName());
        return;
    }

    auto filePath = file.getFullPathName();
    auto fileName = file.getFileNameWithoutExtension();

    GUIHelpers::log("DrumPadComponent::setupNewSample: Setting up sound " + juce::String(soundIndex) + " with file " + fileName);

    m_samplerPlugin.setSoundMedia(soundIndex, filePath);
    m_samplerPlugin.setSoundName(soundIndex, fileName);

    // Set marker positions to the full length of the audio file
    auto audioFile = m_samplerPlugin.getSoundFile(soundIndex);
    if (audioFile.isValid())
    {
        double fileLength = audioFile.getLengthInSamples() / audioFile.getSampleRate();
        m_samplerPlugin.setSoundExcerpt(soundIndex, 0.0, fileLength);
    }

    // Set MIDI parameters - for drum pads, each sound should only respond to its exact note
    const int midiNote = BASE_MIDI_NOTE + soundIndex;

    GUIHelpers::log("DrumPadComponent: setupNewSample - soundIndex: " + juce::String(soundIndex) +
                   ", midiNote: " + juce::String(midiNote));

    m_samplerPlugin.setSoundParams(soundIndex, midiNote, midiNote, midiNote);  // keyNote=minNote=maxNote
    m_samplerPlugin.setSoundOpenEnded(soundIndex, true);

    updatePadNames();
}

DrumPadComponent::PadSoundData DrumPadComponent::getPadSoundData(int soundIndex) const
{
    PadSoundData data;
    if (soundIndex >= 0 && soundIndex < m_samplerPlugin.getNumSounds())
    {
        data.media = m_samplerPlugin.getSoundMedia(soundIndex);
        data.name = m_samplerPlugin.getSoundName(soundIndex);
        data.gainDb = m_samplerPlugin.getSoundGainDb(soundIndex);
        data.pan = m_samplerPlugin.getSoundPan(soundIndex);
        data.startTime = m_samplerPlugin.getSoundStartTime(soundIndex);
        data.length = m_samplerPlugin.getSoundLength(soundIndex);
        data.openEnded = m_samplerPlugin.isSoundOpenEnded(soundIndex);
    }
    return data;
}

void DrumPadComponent::applyPadSoundData(int soundIndex, const PadSoundData& data)
{
    if (soundIndex >= 0 && soundIndex < m_samplerPlugin.getNumSounds())
    {
        m_samplerPlugin.setSoundMedia(soundIndex, data.media);
        m_samplerPlugin.setSoundName(soundIndex, data.name);
        m_samplerPlugin.setSoundGains(soundIndex, data.gainDb, data.pan);
        m_samplerPlugin.setSoundExcerpt(soundIndex, data.startTime, data.length);
        m_samplerPlugin.setSoundOpenEnded(soundIndex, data.openEnded);
    }
}
