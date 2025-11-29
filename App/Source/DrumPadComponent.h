#include <JuceHeader.h>

namespace te = tracktion_engine;

class ApplicationViewState;
class DrumPadGridComponent;

// a visual representation of a Pad
class PadComponent : public juce::Component, private juce::Timer
{
public:
    PadComponent(DrumPadGridComponent* parent, int index) : owner(parent), padIndex(index) {}

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    void setText(juce::String text) { m_text = text; }
    void changeColour(juce::Colour colour);
    void triggerVisualFeedback(juce::Colour triggerColour, juce::Colour returnColour);
    void setIsDragTarget(bool isTarget);
    int getPadIndex() const { return padIndex; }

private:
    void timerCallback() override;

    juce::Colour m_colour;
    juce::String m_text;
    juce::Colour m_returnColour { juce::Colours::grey };
    DrumPadGridComponent* owner;
    int padIndex;
    bool m_isDragTarget = false;
    juce::Point<int> m_dragStartPos;
    bool m_isDragging = false;
};

class DrumPadGridComponent : public juce::Component,
                         public juce::ValueTree::Listener,
                         public juce::DragAndDropTarget,
                         private te::PhysicalMidiInputDevice::Listener
{
public:
    DrumPadGridComponent(te::SamplerPlugin&, ApplicationViewState&);
    ~DrumPadGridComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void buttonDown(int padIndex);
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void parentHierarchyChanged() override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;

    // Drag and Drop
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;
    void itemDragEnter (const SourceDetails& dragSourceDetails) override;
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails& dragSourceDetails) override;

    // Pad-to-Pad Drag & Drop
    void startPadDrag(int sourcePadIndex, const juce::MouseEvent& event);
    void continuePadDrag(const juce::MouseEvent& event);
    void endPadDrag(const juce::MouseEvent& event);
    void swapPadSounds(int sourcePad, int targetPad);

    // Accessors for DrumPad
    bool isPadDragging() const { return m_isPadDragging; }
    int getDragSourcePad() const { return m_dragSourcePad; }

    // MIDI Input handling
    void handleIncomingMidiMessage(const juce::MidiMessage& message) override;

    // MIDI Device Management
    void setupMidiInputDevices();
    void cleanupMidiInputDevices();

    // MIDI Processing
    void processMidiForPadLighting(const juce::MidiMessage& message);
    void illuminatePadForNote(int midiNote, float velocity);
    void turnOffPadForNote(int midiNote);
    int getPadIndexForMidiNote(int midiNote);

    int getMidiNoteForPad(int padIndex) const;

    void showPadContextMenu(int padIndex);
    void updatePadNames();
    int getNeededWidth();
    tracktion_engine::Edit* getEdit() { return &m_edit; }
    int getSoundIndexForPad(int padIndex);
    juce::String getMidiNoteNameForPad(int padIndex);
    std::function<void(int)> onPadClicked;

    ApplicationViewState& m_appViewState;
    static constexpr int BASE_MIDI_NOTE = 48; // C3 (MIDI standard)

private:
    void setupNewSample(int soundIndex, const juce::File& file);

    // Helper struct only for swapping tasks, don't persists.
    struct TempSoundState
    {
        juce::String filePath;
        juce::String name;
        float gainDb = 0.0f;
        float pan = 0.0f;
        double start = 0.0;
        double length = 0.0;
        bool openEnded = true;
    };

    TempSoundState getSoundStateFromPlugin(int soundIndex) const;
    void applySoundStateToPlugin(int soundIndex, const TempSoundState& state);

    te::Edit& m_edit;
    te::SamplerPlugin& m_samplerPlugin;
    juce::OwnedArray<PadComponent> m_pads;

    int m_selectedPadIndex = -1;
    int m_draggedOverPad = -1;

    // Pad-to-Pad Drag State
    bool m_isPadDragging = false;
    int m_dragSourcePad = -1;
    int m_dragSourceSoundIndex = -1;
    juce::ComponentDragger m_dragger;
    std::unique_ptr<juce::Component> m_dragImageComponent;

    // MIDI Input Devices
    juce::Array<te::PhysicalMidiInputDevice*> m_connectedMidiDevices;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumPadGridComponent)
};
