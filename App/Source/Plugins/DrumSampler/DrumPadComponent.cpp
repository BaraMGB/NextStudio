#include "Plugins/DrumSampler/DrumPadComponent.h"
#include "Utilities/ApplicationViewState.h"
#include "SideBrowser/Browser_Base.h"
#include "Utilities/Utilities.h"

namespace
{
static constexpr int VISUAL_FEEDBACK_DURATION_MS = 100;
}

void PadComponent::paint(juce::Graphics &g)
{
    auto area = getLocalBounds().toFloat();
    area.reduce(5, 5);

    // Highlight if drag target
    if (m_isDragTarget)
    {
        g.setColour(owner->m_appViewState.getPrimeColour());
        g.drawRoundedRectangle(area, 5, 3);
    }

    g.setColour(owner->m_appViewState.getBorderColour());
    g.fillRoundedRectangle(area, 5);
    area.reduce(1, 1);
    g.setColour(m_colour);
    g.fillRoundedRectangle(area, 5);

    // Draw the note name in the top-left corner
    area.reduce(2, 2);
    g.setColour(m_colour.getBrightness() > 0.5f ? juce::Colours::black : owner->m_appViewState.getButtonTextColour());
    g.setFont(12.0f);
    g.drawText(owner->getMidiNoteNameForPad(padIndex), area, juce::Justification::topLeft);

    // Draw the sample name in the center
    g.setFont(7.0f);
    g.drawText(m_text, area, juce::Justification::centred);
}

void PadComponent::mouseDown(const juce::MouseEvent &e)
{
    m_dragStartPos = e.getMouseDownPosition();
    m_isDragging = false;

    if (e.mods.isRightButtonDown())
        owner->showPadContextMenu(padIndex);
    else
        owner->buttonDown(padIndex);
}

void PadComponent::mouseDrag(const juce::MouseEvent &e)
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

void PadComponent::mouseUp(const juce::MouseEvent &e)
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
            // Always use the pad's fixed midiNote - simplified logic!
            int midiNote = owner->getMidiNoteForPad(padIndex);

            GUIHelpers::log("DrumPad::mouseUp: Using fixed MIDI note " + juce::String(midiNote) + " for padIndex " + juce::String(padIndex));

            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOff(1, midiNote, .8f), 0);
        }
    }
}

void PadComponent::mouseEnter(const juce::MouseEvent &e)
{
    if (owner->isPadDragging() && padIndex != owner->getDragSourcePad())
    {
        setIsDragTarget(true);
    }
}

void PadComponent::mouseExit(const juce::MouseEvent &e) { setIsDragTarget(false); }

void PadComponent::setIsDragTarget(bool isTarget)
{
    if (m_isDragTarget != isTarget)
    {
        m_isDragTarget = isTarget;
        repaint();
    }
}

void PadComponent::changeColour(juce::Colour colour)
{
    m_colour = colour;
    repaint();
}

void PadComponent::triggerVisualFeedback(juce::Colour triggerColour, juce::Colour returnColour)
{
    changeColour(triggerColour);
    m_returnColour = returnColour;
    // Timer runs on the message thread, so this is thread-safe
    startTimer(VISUAL_FEEDBACK_DURATION_MS);
}

void PadComponent::timerCallback()
{
    stopTimer();
    changeColour(m_returnColour);
}

// -----------------------------------------------------------------------

DrumPadGridComponent::DrumPadGridComponent(te::SamplerPlugin &plugin, ApplicationViewState &appViewState)
    : m_edit(plugin.edit),
      m_samplerPlugin(plugin),
      m_appViewState(appViewState)
{
    GUIHelpers::log("DrumPadComponent: constructor");

    m_samplerPlugin.state.addListener(this);

    for (int i = 0; i < 16; ++i)
    {
        auto pad = std::make_unique<PadComponent>(this, i);
        m_pads.add(pad.get());
        pad->changeColour(m_appViewState.getButtonBackgroundColour());
        addAndMakeVisible(pad.release());
    }

    setupMidiInputDevices();

    updatePadNames();
}

DrumPadGridComponent::~DrumPadGridComponent()
{
    GUIHelpers::log("DrumPadComponent: destructor");

    cleanupMidiInputDevices();

    m_samplerPlugin.state.removeListener(this);
}

void DrumPadGridComponent::paint(juce::Graphics &g)
{
    g.fillAll(m_appViewState.getBackgroundColour2());
    auto bounds = getLocalBounds();
    bounds.reduce(2, 2);
    g.fillRoundedRectangle(bounds.toFloat(), 10);
    bounds.reduce(1, 1);
    g.setColour(m_appViewState.getBackgroundColour1());
    g.fillRoundedRectangle(bounds.toFloat(), 10);
}

void DrumPadGridComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.reduce(3, 3); // Outer margin

    // 1. Determine the maximum possible size for a pad
    // by taking the smaller dimension of the available space.
    // This keeps the pads square.
    // (If you want rectangular pads, use getWidth()/4 and getHeight()/4 separately)
    auto minDimension = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto padSize = minDimension / 4;

    // 2. Calculate the total size of the 4x4 grid
    auto totalGridSize = padSize * 4;

    // 3. Calculate the start offset to center the grid:
    // (Available Space - Used Space) / 2
    auto xOffset = bounds.getX() + (bounds.getWidth() - totalGridSize) / 2;
    auto yOffset = bounds.getY() + (bounds.getHeight() - totalGridSize) / 2;

    for (int i = 0; i < m_pads.size(); ++i)
    {
        auto col = i % 4;
        auto row = 3 - (i / 4); // Invert y-axis for bottom-up layout

        // 4. Calculate coordinates: Start Offset + (Column/Row * Size)
        auto x = xOffset + (col * padSize);
        auto y = yOffset + (row * padSize);

        m_pads[i]->setBounds(x, y, padSize, padSize);
    }
}

void DrumPadGridComponent::buttonDown(int padIndex)
{
    if (onPadClicked)
        onPadClicked(padIndex);

    if (padIndex != -1)
    {
        // Update selection
        if (m_selectedPadIndex != -1 && m_selectedPadIndex < m_pads.size())
            m_pads[m_selectedPadIndex]->changeColour(m_appViewState.getButtonBackgroundColour());

        m_selectedPadIndex = padIndex;
        int soundIndex = getSoundIndexForPad(padIndex);
        m_pads[m_selectedPadIndex]->changeColour(m_appViewState.getPrimeColour());

        // Always play sound when clicked, regardless of whether it has media
        GUIHelpers::log("DrumPadComponent: Clicked pad " + juce::String(padIndex) + ", soundIndex: " + juce::String(soundIndex));

        // Check if this pad has a valid sound
        if (soundIndex < m_samplerPlugin.getNumSounds())
        {
            auto soundName = m_samplerPlugin.getSoundName(soundIndex);
            GUIHelpers::log("DrumPadComponent: Sound at soundIndex " + juce::String(soundIndex) + ": " + soundName);

            if (!soundName.isEmpty() && soundName != "Empty")
            {
                // Always use the pad's fixed midiNote - no more complex fallback logic!
                int midiNote = getMidiNoteForPad(padIndex);

                GUIHelpers::log("DrumPadComponent: Using fixed MIDI note " + juce::String(midiNote) + " for padIndex " + juce::String(padIndex));

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
        auto noteNum = getMidiNoteForPad(padIndex);
        illuminatePadForNote(noteNum, 100);
    }
}

void DrumPadGridComponent::showPadContextMenu(int padIndex)
{
    GUIHelpers::log("DrumPadComponent: Right click on pad " + juce::String(padIndex));

    juce::PopupMenu menu;
    menu.addItem("Load Sample",
                 [this, padIndex]()
                 {
                     int soundIndex = getSoundIndexForPad(padIndex);
                     GUIHelpers::log("DrumPadComponent: Opening file chooser for pad " + juce::String(padIndex) + " (soundIndex: " + juce::String(soundIndex) + ")");

                     auto fc = std::make_shared<juce::FileChooser>("Select a sample to load", juce::File(), "*.wav;*.aif;*.aiff");

                     juce::Component::SafePointer<DrumPadGridComponent> safeThis(this);

                     fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                     [safeThis, soundIndex, chooser = fc](const juce::FileChooser &fc)
                                     {
                                         if (safeThis == nullptr)
                                             return;

                                         if (fc.getResults().isEmpty())
                                         {
                                             GUIHelpers::log("DrumPadComponent: File selection cancelled");
                                             return;
                                         }

                                         auto file = fc.getResult();

                                         safeThis->setupNewSample(soundIndex, file);
                                     });
                 });
    menu.addSeparator();

    menu.addItem("Clear",
                 [this, padIndex]()
                 {
                     int soundIndex = getSoundIndexForPad(padIndex);
                     GUIHelpers::log("DrumPadComponent: Clearing pad " + juce::String(padIndex) + " (soundIndex: " + juce::String(soundIndex) + ")");

                     if (soundIndex < m_samplerPlugin.getNumSounds())
                     {
                         // Clear the sound by setting empty media and name
                         m_samplerPlugin.setSoundMedia(soundIndex, "");
                         m_samplerPlugin.setSoundName(soundIndex, "Empty");

                         // Reset MIDI parameters to pad's fixed midiNote
                         int padMidiNote = getMidiNoteForPad(soundIndex);
                         m_samplerPlugin.setSoundParams(soundIndex, padMidiNote, padMidiNote, padMidiNote);
                         m_samplerPlugin.setSoundOpenEnded(soundIndex, true);

                         GUIHelpers::log("DrumPadComponent: Cleared sound at index " + juce::String(soundIndex) + " (pad midiNote: " + juce::String(padMidiNote) + ")");

                         updatePadNames();

                         // Repaint the pad to show the updated text immediately
                         if (padIndex >= 0 && padIndex < m_pads.size())
                             m_pads[padIndex]->repaint();
                     }
                 });

    menu.show();
}

void DrumPadGridComponent::mouseDown(const juce::MouseEvent &e)
{
    if (e.mods.isRightButtonDown())
    {
        auto *pad = dynamic_cast<PadComponent *>(getComponentAt(e.x, e.y));
        if (pad)
        {
            auto padIndex = m_pads.indexOf(pad);
            if (padIndex != -1)
                showPadContextMenu(padIndex);
        }
    }
}

void DrumPadGridComponent::mouseDrag(const juce::MouseEvent &e)
{
    // Handle drag image movement if we're dragging
    if (m_isPadDragging && m_dragImageComponent)
    {
        auto pos = getLocalPoint(nullptr, e.getScreenPosition());
        m_dragImageComponent->setCentrePosition(pos.x, pos.y);
    }
}

void DrumPadGridComponent::mouseUp(const juce::MouseEvent &e)
{
    // This will be handled by the individual pad's mouseUp
}

void DrumPadGridComponent::parentHierarchyChanged()
{
    if (getParentComponent() != nullptr)
        updatePadNames();
}

void DrumPadGridComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) { updatePadNames(); }

void DrumPadGridComponent::updatePadNames()
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

int DrumPadGridComponent::getNeededWidth() { return 4; }

juce::String DrumPadGridComponent::getMidiNoteNameForPad(int padIndex)
{
    int midiNote = getMidiNoteForPad(padIndex);
    if (midiNote >= 0)
    {
        // Use octave 4 as base to show C3 for MIDI note 48 (standard drum notation)
        return juce::MidiMessage::getMidiNoteName(midiNote, true, true, 4);
    }
    return "?";
}

int DrumPadGridComponent::getSoundIndexForPad(int padIndex)
{
    // With the new resized logic, padIndex and soundIndex are now the same.
    // Pad 0 (bottom-left) corresponds to sound 0.
    return padIndex;
}

//==============================================================================
// Drag and Drop
//==============================================================================

bool DrumPadGridComponent::isInterestedInDragSource(const SourceDetails &dragSourceDetails)
{
    return dragSourceDetails.description == "SampleBrowser" || dragSourceDetails.description == "FileBrowser";
}

void DrumPadGridComponent::itemDragEnter(const SourceDetails &dragSourceDetails)
{
    if (auto *pad = dynamic_cast<PadComponent *>(getComponentAt(dragSourceDetails.localPosition)))
    {
        m_draggedOverPad = m_pads.indexOf(pad);
        pad->changeColour(m_appViewState.getPrimeColour());
    }
}

void DrumPadGridComponent::itemDragMove(const SourceDetails &dragSourceDetails)
{
    auto *padUnderCursor = dynamic_cast<PadComponent *>(getComponentAt(dragSourceDetails.localPosition));
    int padIndex = padUnderCursor ? m_pads.indexOf(padUnderCursor) : -1;

    if (padIndex != m_draggedOverPad)
    {
        if (m_draggedOverPad != -1)
        {
            if (m_draggedOverPad == m_selectedPadIndex)
                m_pads[m_draggedOverPad]->changeColour(m_appViewState.getPrimeColour());
            else
                m_pads[m_draggedOverPad]->changeColour(m_appViewState.getButtonBackgroundColour());
        }

        m_draggedOverPad = padIndex;

        if (m_draggedOverPad != -1)
            m_pads[m_draggedOverPad]->changeColour(m_appViewState.getRenderColour());
    }
}

void DrumPadGridComponent::itemDragExit(const SourceDetails &)
{
    if (m_draggedOverPad != -1)
    {
        if (m_draggedOverPad == m_selectedPadIndex)
            m_pads[m_draggedOverPad]->changeColour(m_appViewState.getPrimeColour());
        else
            m_pads[m_draggedOverPad]->changeColour(m_appViewState.getButtonBackgroundColour());
        m_draggedOverPad = -1;
    }
}

void DrumPadGridComponent::itemDropped(const SourceDetails &dragSourceDetails)
{
    juce::File f;
    if (auto *browser = dynamic_cast<BrowserListBox *>(dragSourceDetails.sourceComponent.get()))
        f = browser->getSelectedFile();

    if (f.existsAsFile())
    {
        if (auto *pad = dynamic_cast<PadComponent *>(getComponentAt(dragSourceDetails.localPosition)))
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
            m_pads[m_draggedOverPad]->changeColour(m_appViewState.getPrimeColour());
        else
            m_pads[m_draggedOverPad]->changeColour(m_appViewState.getButtonBackgroundColour());

        m_draggedOverPad = -1;
    }
}

//==============================================================================
// Pad-to-Pad Drag & Drop Implementation
//==============================================================================

void DrumPadGridComponent::startPadDrag(int sourcePadIndex, const juce::MouseEvent &event)
{
    if (sourcePadIndex < 0 || sourcePadIndex >= m_pads.size())
        return;

    int soundIndex = getSoundIndexForPad(sourcePadIndex);
    if (soundIndex >= m_samplerPlugin.getNumSounds())
        return;

    auto soundName = m_samplerPlugin.getSoundName(soundIndex);

    // Don't drag empty pads
    if (soundName.isEmpty() || soundName == "Empty")
        return;

    GUIHelpers::log("DrumPadComponent: Starting drag from pad " + juce::String(sourcePadIndex) + " (sound: " + soundName + ")");

    m_isPadDragging = true;
    m_dragSourcePad = sourcePadIndex;
    m_dragSourceSoundIndex = soundIndex;

    // Create drag image component with visual representation
    struct DragImagePainter : public juce::Component
    {
        int sourcePadIndex;
        juce::String soundName;
        ApplicationViewState &m_appViewState;

        DragImagePainter(int pad, const juce::String &name, ApplicationViewState &appViewState)
            : sourcePadIndex(pad),
              soundName(name),
              m_appViewState(appViewState)
        {
            setSize(50, 50);
            setAlwaysOnTop(true);
            setOpaque(false);
        }

        void paint(juce::Graphics &g) override
        {
            auto area = getLocalBounds().toFloat();
            area.reduce(5, 5);

            // Semi-transparent background
            g.setColour(m_appViewState.getButtonBackgroundColour().withAlpha(0.7f));
            g.fillRoundedRectangle(area, 8);

            // Border
            g.setColour(m_appViewState.getPrimeColour());
            g.drawRoundedRectangle(area, 8, 2);

            // Pad info
            area.reduce(5, 5);
            g.setColour(m_appViewState.getTextColour());
            g.setFont(12.0f);
            g.drawText("Pad " + juce::String(sourcePadIndex), area.removeFromTop(20), juce::Justification::centred);

            g.setFont(14.0f);
            g.drawText(soundName, area, juce::Justification::centred);
        }
    };

    m_dragImageComponent = std::make_unique<DragImagePainter>(sourcePadIndex, soundName, m_appViewState);

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
    m_pads[sourcePadIndex]->changeColour(m_appViewState.getPrimeColour().withAlpha(0.5f));
}

void DrumPadGridComponent::continuePadDrag(const juce::MouseEvent &event)
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

void DrumPadGridComponent::endPadDrag(const juce::MouseEvent &event)
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
        m_pads[targetPadIndex]->triggerVisualFeedback(m_appViewState.getButtonTextColour(), targetPadIndex == m_selectedPadIndex ? m_appViewState.getPrimeColour() : m_appViewState.getButtonBackgroundColour());
        buttonDown(targetPadIndex);
    }

    // Restore source pad color
    if (m_dragSourcePad >= 0 && m_dragSourcePad < m_pads.size())
    {
        juce::Colour sourceColor = (m_dragSourcePad == m_selectedPadIndex) ? m_appViewState.getPrimeColour() : m_appViewState.getButtonBackgroundColour();
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

void DrumPadGridComponent::swapPadSounds(int sourcePad, int targetPad)
{
    // Validation
    if (sourcePad < 0 || sourcePad >= 16 || targetPad < 0 || targetPad >= 16)
        return;

    int sourceSoundIdx = getSoundIndexForPad(sourcePad);
    int targetSoundIdx = getSoundIndexForPad(targetPad);

    GUIHelpers::log("DrumPadComponent: Swapping sounds " + juce::String(sourceSoundIdx) + " <-> " + juce::String(targetSoundIdx));

    // 1. Read status directly from the plugin (Single Source of Truth)
    auto stateA = getSoundStateFromPlugin(sourceSoundIdx);
    auto stateB = getSoundStateFromPlugin(targetSoundIdx);

    // 2. Apply crosswise
    applySoundStateToPlugin(sourceSoundIdx, stateB);
    applySoundStateToPlugin(targetSoundIdx, stateA);

    // 3. Repair MIDI Notes (Fixed Note Logic)
    // No matter what we swapped, Pad X must always remain Note Y

    int noteA = getMidiNoteForPad(sourcePad);
    m_samplerPlugin.setSoundParams(sourceSoundIdx, noteA, noteA, noteA);

    int noteB = getMidiNoteForPad(targetPad);
    m_samplerPlugin.setSoundParams(targetSoundIdx, noteB, noteB, noteB);

    // 4. UI Update
    updatePadNames();

    // Repaint specific pads for immediate feedback
    if (sourcePad < m_pads.size())
        m_pads[sourcePad]->repaint();
    if (targetPad < m_pads.size())
        m_pads[targetPad]->repaint();
}

//==============================================================================
// MIDI Input Device Management
//==============================================================================

void DrumPadGridComponent::setupMidiInputDevices()
{
    GUIHelpers::log("DrumPadComponent: Setting up MIDI input devices");

    auto &deviceManager = m_edit.engine.getDeviceManager();

    // Rescan for MIDI devices to ensure we have the latest list
    deviceManager.rescanMidiDeviceList();

    auto midiDevices = deviceManager.getMidiInDevices();
    GUIHelpers::log("DrumPadComponent: Found " + juce::String(midiDevices.size()) + " MIDI input devices");

    for (int i = 0; i < midiDevices.size(); ++i)
    {
        auto &device = midiDevices[i];
        GUIHelpers::log("DrumPadComponent: Device " + juce::String(i) + ": " + device->getName() + " (Type: " + juce::String(device->getDeviceType()) + ")");

        if (auto physicalDevice = dynamic_cast<te::PhysicalMidiInputDevice *>(device.get()))
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
                GUIHelpers::log("DrumPadComponent: Error opening MIDI device " + physicalDevice->getName() + ": " + error);
            }
            else
            {
                GUIHelpers::log("DrumPadComponent: Successfully connected to MIDI device: " + physicalDevice->getName());
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

void DrumPadGridComponent::cleanupMidiInputDevices()
{
    GUIHelpers::log("DrumPadComponent: Cleaning up MIDI input devices");

    for (auto *device : m_connectedMidiDevices)
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

void DrumPadGridComponent::handleIncomingMidiMessage(const juce::MidiMessage &message)
{
    // on message thread we can log and light up the pad
    juce::MessageManager::callAsync(
        [this, message]()
        {
            if (message.isNoteOn())
            {
                GUIHelpers::log("MIDI Note ON: " + juce::String(message.getNoteNumber()));
            }

            processMidiForPadLighting(message);
        });
}

void DrumPadGridComponent::processMidiForPadLighting(const juce::MidiMessage &message)
{
    if (message.isNoteOn())
    {
        int noteNumber = message.getNoteNumber();
        float velocity = message.getFloatVelocity();

        int padIndex = getPadIndexForMidiNote(noteNumber);

        if (padIndex >= 0 && padIndex < m_pads.size())
        {
            illuminatePadForNote(noteNumber, velocity);
        }
    }
    else if (message.isNoteOff())
    {
        int noteNumber = message.getNoteNumber();
        int padIndex = getPadIndexForMidiNote(noteNumber);

        if (padIndex >= 0 && padIndex < m_pads.size())
        {
            turnOffPadForNote(noteNumber);
        }
    }
}

void DrumPadGridComponent::illuminatePadForNote(int midiNote, float velocity)
{
    int padIndex = getPadIndexForMidiNote(midiNote);

    if (padIndex >= 0 && padIndex < m_pads.size())
    {
        // Calculate color based on velocity (darker for softer, brighter for louder)
        juce::Colour baseColor = m_appViewState.getPrimeColour().brighter(0.8f);
        float brightness = juce::jlimit(0.3f, 1.0f, 0.3f + velocity * 0.7f);
        juce::Colour velocityColor = baseColor.withBrightness(brightness);

        // Get the return color (selected pad stays blue, others grey)
        juce::Colour returnColor = (padIndex == m_selectedPadIndex) ? m_appViewState.getPrimeColour() : m_appViewState.getButtonBackgroundColour();

        // Trigger visual feedback with velocity-based color
        m_pads[padIndex]->triggerVisualFeedback(velocityColor, returnColor);
    }
}

void DrumPadGridComponent::turnOffPadForNote(int midiNote)
{
    int padIndex = getPadIndexForMidiNote(midiNote);

    if (padIndex >= 0 && padIndex < m_pads.size())
    {
        // Return to normal color
        juce::Colour returnColor = (padIndex == m_selectedPadIndex) ? m_appViewState.getPrimeColour() : m_appViewState.getButtonBackgroundColour();
        m_pads[padIndex]->changeColour(returnColor);
    }
}

int DrumPadGridComponent::getPadIndexForMidiNote(int midiNote)
{
    // Direct calculation: Pad 0 = MIDI 48, Pad 1 = MIDI 49, etc.
    // No more complex mapping table needed!
    if (midiNote >= BASE_MIDI_NOTE && midiNote < BASE_MIDI_NOTE + 16)
    {
        int padIndex = midiNote - BASE_MIDI_NOTE;
        GUIHelpers::log("DrumPadComponent: Direct mapping - MIDI note " + juce::String(midiNote) + " -> pad index " + juce::String(padIndex));
        return padIndex;
    }

    GUIHelpers::log("DrumPadComponent: MIDI note " + juce::String(midiNote) + " out of range");
    return -1; // No pad found for this note
}

//==============================================================================
// Private Helper Functions
//==============================================================================

void DrumPadGridComponent::setupNewSample(int soundIndex, const juce::File &file)
{
    if (soundIndex < 0)
        return;

    // Ensure we have enough sound slots in the sampler
    while (m_samplerPlugin.getNumSounds() <= soundIndex)
    {
        GUIHelpers::log("DrumPadComponent: Adding empty sound slot at index " + juce::String(m_samplerPlugin.getNumSounds()));
        m_samplerPlugin.addSound({}, "Empty", 0, 0, 0);
    }

    if (!file.existsAsFile())
        return;

    auto filePath = file.getFullPathName();
    auto fileName = file.getFileNameWithoutExtension();

    m_samplerPlugin.setSoundMedia(soundIndex, filePath);
    m_samplerPlugin.setSoundName(soundIndex, fileName);

    // Length logic
    auto audioFile = m_samplerPlugin.getSoundFile(soundIndex);
    if (audioFile.isValid())
    {
        double fileLength = audioFile.getLengthInSamples() / audioFile.getSampleRate();
        m_samplerPlugin.setSoundExcerpt(soundIndex, 0.0, fileLength);
    }

    int padMidiNote = getMidiNoteForPad(soundIndex); // soundIndex == padIndex
    m_samplerPlugin.setSoundParams(soundIndex, padMidiNote, padMidiNote, padMidiNote);
    m_samplerPlugin.setSoundOpenEnded(soundIndex, true);
}

int DrumPadGridComponent::getMidiNoteForPad(int padIndex) const
{
    if (padIndex >= 0 && padIndex < 16)
    {
        return BASE_MIDI_NOTE + padIndex;
    }
    return -1;
}

DrumPadGridComponent::TempSoundState DrumPadGridComponent::getSoundStateFromPlugin(int soundIndex) const
{
    TempSoundState s;
    if (soundIndex >= 0 && soundIndex < m_samplerPlugin.getNumSounds())
    {
        s.filePath = m_samplerPlugin.getSoundMedia(soundIndex);
        s.name = m_samplerPlugin.getSoundName(soundIndex);
        s.gainDb = m_samplerPlugin.getSoundGainDb(soundIndex);
        s.pan = m_samplerPlugin.getSoundPan(soundIndex);
        s.start = m_samplerPlugin.getSoundStartTime(soundIndex);
        s.length = m_samplerPlugin.getSoundLength(soundIndex);
        s.openEnded = m_samplerPlugin.isSoundOpenEnded(soundIndex);
    }
    return s;
}

void DrumPadGridComponent::applySoundStateToPlugin(int soundIndex, const TempSoundState &state)
{
    if (soundIndex >= 0 && soundIndex < m_samplerPlugin.getNumSounds())
    {
        m_samplerPlugin.setSoundMedia(soundIndex, state.filePath);
        m_samplerPlugin.setSoundName(soundIndex, state.name);
        m_samplerPlugin.setSoundGains(soundIndex, state.gainDb, state.pan);
        m_samplerPlugin.setSoundExcerpt(soundIndex, state.start, state.length);
        m_samplerPlugin.setSoundOpenEnded(soundIndex, state.openEnded);
    }
}
