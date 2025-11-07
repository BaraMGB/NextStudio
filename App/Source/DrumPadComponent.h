#include <JuceHeader.h>
#include "tracktion_engine/tracktion_engine.h"

namespace te = tracktion_engine;

class DrumPadComponent;

class DrumPad : public juce::Component
{
public:
    DrumPad(DrumPadComponent* parent, int index) : owner(parent), padIndex(index) {}

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setText(juce::String text) { m_text = text; } 
    void changeColour(juce::Colour colour);
private:
    juce::Colour m_colour;
    juce::String m_text;
    DrumPadComponent* owner;
    int padIndex;
};

class DrumPadComponent : public juce::Component,
                         public juce::ValueTree::Listener,
                         public juce::DragAndDropTarget
{
public:
    DrumPadComponent(te::SamplerPlugin&);
    ~DrumPadComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void buttonDown(int padIndex);
    void mouseDown(const juce::MouseEvent& e) override;

    void parentHierarchyChanged() override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;

    // Drag and Drop
    bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;
    void itemDragEnter (const SourceDetails& dragSourceDetails) override;
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails& dragSourceDetails) override;

    void showPadContextMenu(int padIndex);
    void updatePadNames();
    int getNeededWidth();
    tracktion_engine::Edit* getEdit() { return &m_edit; }
    te::SamplerPlugin* getSamplerPlugin() { return samplerPlugin; }
    int getSoundIndexForPad(int padIndex);
    juce::String getMidiNoteNameForPad(int padIndex);
    std::function<void(int)> onPadClicked;

    static constexpr int BASE_MIDI_NOTE = 48;
private:
    te::Edit& m_edit;
    te::SamplerPlugin* samplerPlugin;
    juce::OwnedArray<DrumPad> m_pads;
    int m_selectedPadIndex = -1;
    int m_draggedOverPad = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumPadComponent)
};
