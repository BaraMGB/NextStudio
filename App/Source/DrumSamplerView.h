#pragma once

#include <JuceHeader.h>

#include "DrumPadComponent.h"
#include "EditViewState.h"
#include "PluginPresetInterface.h"
#include "PluginViewComponent.h"
#include "SoundEditorPanel.h"
#include "Utilities.h"

namespace te = tracktion_engine;
class DrumSamplerView : public PluginViewComponent
{
public:
    DrumSamplerView(EditViewState &evs, te::SamplerPlugin &sampler);
    ~DrumSamplerView() override;

    void paint(juce::Graphics &) override;
    void resized() override;

    int getNeededWidth() override;

    // PluginPresetInterface implementation
    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    te::Edit &m_edit;
    te::SamplerPlugin &m_sampler;

    DrumPadGridComponent m_drumPadComponent;
    SoundEditorPanel m_soundEditorPanel;

    // Drag & Drop state
    struct DragDropState
    {
        bool isDragging = false;
        int sourcePadIndex = -1;
        int draggedSoundIndex = -1;
        juce::Image dragImage;
        juce::Point<int> dragOffset;
    };
    DragDropState m_dragDropState;

    // Drag & Drop methods
    void startDrag(int padIndex, int soundIndex, const juce::MouseEvent &event);
    void continueDrag(const juce::MouseEvent &event);
    void endDrag(const juce::MouseEvent &event);
    void swapSounds(int sourcePad, int targetPad);
    juce::Image createDragImage(int padIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSamplerView)
};
