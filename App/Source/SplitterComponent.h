
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"

class SplitterComponent : public juce::Component
{
public:
    explicit SplitterComponent(bool horizontal = true);
    std::function<void()> onMouseDown;
    std::function<void(int)> onDrag; // Add a callback for dragging
    void mouseMove(const juce::MouseEvent &event) override;
    void mouseEnter(const juce::MouseEvent &event) override;
    void mouseExit(const juce::MouseEvent &event) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;
    void paint(juce::Graphics& g) override;

private:
    bool m_isHovering{false};
    bool m_isHorizontal;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SplitterComponent)
};
