#include "PluginBypassPresentation.h"

namespace
{
const juce::Identifier enabledProperty("enabled");

void drawVerticalText(juce::Graphics &g, juce::Rectangle<int> bounds, const juce::String &text, float fontHeight)
{
    if (bounds.isEmpty())
        return;

    juce::Graphics::ScopedSaveState saveState(g);
    const auto centre = bounds.toFloat().getCentre();
    g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi, centre.x, centre.y));
    const auto textArea = juce::Rectangle<float>(centre.x - bounds.getHeight() * 0.5f,
                                                centre.y - bounds.getWidth() * 0.5f,
                                                static_cast<float>(bounds.getHeight()),
                                                static_cast<float>(bounds.getWidth()));
    g.setFont(juce::FontOptions(fontHeight, juce::Font::bold));
    g.drawFittedText(text, textArea.toNearestInt(), juce::Justification::centred, 1);
}
} // namespace

void PluginBypassEffect::applyEffect(juce::Image &source, juce::Graphics &destination, float /*scaleFactor*/, float alpha)
{
    // JUCE has already rendered the whole subtree at the physical pixel scale.
    // Never desaturate the shared source or any image owned by a child editor.
    auto grayscale = source.createCopy();
    grayscale.desaturate();
    juce::Graphics::ScopedSaveState saveState(destination);
    destination.setOpacity(alpha);
    destination.drawImageAt(grayscale, 0, 0);
}

PluginBypassPresentation::PluginBypassPresentation(juce::Component &component, juce::ValueTree pluginState)
    : m_component(component), m_state(std::move(pluginState))
{
    jassert(m_component.getComponentEffect() == nullptr);
    m_state.addListener(this);
    update();
}

PluginBypassPresentation::~PluginBypassPresentation()
{
    m_state.removeListener(this);
    if (m_component.getComponentEffect() == &m_effect)
        m_component.setComponentEffect(nullptr);
}

void PluginBypassPresentation::update()
{
    // Read the property itself, so notification ordering relative to the engine's
    // CachedValue does not delay the visual transition, including undo/redo.
    const bool bypassed = !static_cast<bool>(m_state.getProperty(enabledProperty, true));
    if (bypassed != m_bypassed)
    {
        m_bypassed = bypassed;
        m_component.setComponentEffect(m_bypassed ? &m_effect : nullptr);
        m_component.repaint();
    }
}

void PluginBypassPresentation::valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &property)
{
    if (tree == m_state && property == enabledProperty)
        update();
}

void PluginBypassPresentation::valueTreeRedirected(juce::ValueTree &) { update(); }

PluginBypassPresentation::HeaderAreas PluginBypassPresentation::getHeaderAreas(juce::Rectangle<int> header)
{
    header.removeFromTop(juce::jmin(25, header.getHeight())); // Keep the editor-open button clear.
    header = header.reduced(juce::jmin(2, header.getWidth() / 2), juce::jmin(4, header.getHeight() / 2));
    const auto badge = header.removeFromBottom(juce::jmin(84, header.getHeight()));
    const auto title = header.reduced(0, juce::jmin(4, header.getHeight() / 2));
    return {title, badge};
}

void PluginBypassPresentation::paintHeader(juce::Graphics &g, juce::Rectangle<int> header, const juce::String &title, juce::Colour titleColour)
{
    const auto areas = getHeaderAreas(header);
    g.setColour(titleColour);
    drawVerticalText(g, areas.title, title, 12.0f);

    if (!areas.badge.isEmpty())
    {
        g.setColour(juce::Colour(0xff181818));
        g.fillRoundedRectangle(areas.badge.toFloat(), 3.0f);
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.drawRoundedRectangle(areas.badge.toFloat().reduced(0.5f), 3.0f, 1.0f);
        g.setColour(juce::Colours::white);
        drawVerticalText(g, areas.badge, TRANS("BYPASSED"), 10.0f);
    }
}
