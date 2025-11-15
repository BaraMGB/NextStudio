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
    g.setFont(15.0f);
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

            // If keyNote is not set (default -1), fall back to pad-based calculation
            if (midiNote < 0)
            {
                midiNote = owner->BASE_MIDI_NOTE + soundIndex;
                GUIHelpers::log("DrumPad::mouseUp: Using fallback MIDI note " + juce::String(midiNote) + " for soundIndex " + juce::String(soundIndex));
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
                // Get the actual MIDI note assigned to this sound (not calculated from soundIndex)
                int midiNote = m_samplerPlugin.getKeyNote(soundIndex);

                // If keyNote is not set (default -1), fall back to pad-based calculation
                if (midiNote < 0)
                {
                    midiNote = BASE_MIDI_NOTE + soundIndex;
                    GUIHelpers::log("DrumPadComponent: Using fallback MIDI note " + juce::String(midiNote) + " for soundIndex " + juce::String(soundIndex));
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
    int soundIndex = getSoundIndexForPad(padIndex);
    int midiNote = BASE_MIDI_NOTE + soundIndex;
    return juce::MidiMessage::getMidiNoteName(midiNote, true, true, 3);
}

int DrumPadComponent::getSoundIndexForPad(int padIndex)
{
    // MPC-style layout: bottom row (pads 0-3) = sounds 0-3, top row (pads 12-15) = sounds 12-15
    // This matches traditional MPC drum pad layout
    const int row = padIndex / 4;
    const int col = padIndex % 4;
    const int mpcRow = 3 - row;  // MPC Layout: bottom row is 0, top row is 3
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

    auto sourceMedia = m_samplerPlugin.getSoundMedia(sourceSoundIndex);
    auto sourceName = m_samplerPlugin.getSoundName(sourceSoundIndex);
    auto sourceGain = m_samplerPlugin.getSoundGainDb(sourceSoundIndex);
    auto sourcePan = m_samplerPlugin.getSoundPan(sourceSoundIndex);
    auto sourceKeyNote = m_samplerPlugin.getKeyNote(sourceSoundIndex);
    auto sourceMinNote = m_samplerPlugin.getMinKey(sourceSoundIndex);
    auto sourceMaxNote = m_samplerPlugin.getMaxKey(sourceSoundIndex);

    auto targetMedia = m_samplerPlugin.getSoundMedia(targetSoundIndex);
    auto targetName = m_samplerPlugin.getSoundName(targetSoundIndex);
    auto targetGain = m_samplerPlugin.getSoundGainDb(targetSoundIndex);
    auto targetPan = m_samplerPlugin.getSoundPan(targetSoundIndex);
    auto targetKeyNote = m_samplerPlugin.getKeyNote(targetSoundIndex);
    auto targetMinNote = m_samplerPlugin.getMinKey(targetSoundIndex);
    auto targetMaxNote = m_samplerPlugin.getMaxKey(targetSoundIndex);

    // Swap source sound data to target
    m_samplerPlugin.setSoundMedia(targetSoundIndex, sourceMedia);
    m_samplerPlugin.setSoundName(targetSoundIndex, sourceName);
    m_samplerPlugin.setSoundGains(targetSoundIndex, sourceGain, sourcePan);

    // Calculate MIDI notes for the NEW sound indices (not the old ones!)
    int targetMidiNote = BASE_MIDI_NOTE + targetSoundIndex;
    int sourceMidiNote = BASE_MIDI_NOTE + sourceSoundIndex;

    // Set MIDI parameters for target sound (now has source's data)
    m_samplerPlugin.setSoundParams(targetSoundIndex, targetMidiNote, targetMidiNote, targetMidiNote);

    // Swap target sound data to source
    m_samplerPlugin.setSoundMedia(sourceSoundIndex, targetMedia);
    m_samplerPlugin.setSoundName(sourceSoundIndex, targetName);
    m_samplerPlugin.setSoundGains(sourceSoundIndex, targetGain, targetPan);

    // Set MIDI parameters for source sound (now has target's data)
    m_samplerPlugin.setSoundParams(sourceSoundIndex, sourceMidiNote, sourceMidiNote, sourceMidiNote);

    // Update pad names
    updatePadNames();

    GUIHelpers::log("DrumPadComponent: Sound swap completed");
}
