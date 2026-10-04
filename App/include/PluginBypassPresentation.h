#pragma once

#include <JuceHeader.h>

// A rendering-only treatment of the complete rack item, including its children.
// This deliberately never calls Component::setEnabled or writes plug-in state.
class PluginBypassEffect final : public juce::ImageEffectFilter
{
public:
    void applyEffect(juce::Image &, juce::Graphics &, float scaleFactor, float alpha) override;
};

class PluginBypassPresentation final : private juce::ValueTree::Listener
{
public:
    PluginBypassPresentation(juce::Component &, juce::ValueTree pluginState);
    ~PluginBypassPresentation() override;

    bool isBypassed() const { return m_bypassed; }

    struct HeaderAreas
    {
        juce::Rectangle<int> title;
        juce::Rectangle<int> badge;
    };
    static HeaderAreas getHeaderAreas(juce::Rectangle<int> header);
    static void paintHeader(juce::Graphics &, juce::Rectangle<int> header, const juce::String &title, juce::Colour titleColour);

private:
    void update();
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeRedirected(juce::ValueTree &) override;

    juce::Component &m_component;
    juce::ValueTree m_state;
    PluginBypassEffect m_effect;
    bool m_bypassed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBypassPresentation)
};
