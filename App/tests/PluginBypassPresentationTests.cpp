#include "PluginBypassPresentation.h"

#include <iostream>

namespace
{
int failures = 0;
#define REQUIRE(condition) \
    do { if (!(condition)) { std::cerr << "FAIL: " << #condition << " (line " << __LINE__ << ")\n"; ++failures; } } while (false)

bool isGray(juce::Colour colour)
{
    return colour.getRed() == colour.getGreen() && colour.getGreen() == colour.getBlue();
}

class ColourPanel : public juce::Component
{
public:
    explicit ColourPanel(juce::Colour colour) : m_colour(colour) {}
    void paint(juce::Graphics &g) override { g.fillAll(m_colour); }
private:
    juce::Colour m_colour;
};

void testFilterPreservesSourceAndAlpha()
{
    PluginBypassEffect effect;
    for (const auto format : {juce::Image::ARGB, juce::Image::RGB})
    {
        juce::Image source(format, 4, 4, true);
        source.setPixelAt(1, 1, juce::Colour(0xff2472c8));
        source.setPixelAt(2, 1, juce::Colour(0x8020a040));
        const auto original = source.getPixelAt(1, 1);
        const auto translucent = source.getPixelAt(2, 1);
        juce::Image output(juce::Image::ARGB, 4, 4, true);
        {
            juce::Graphics g(output);
            effect.applyEffect(source, g, 2.0f, 1.0f);
        }
        REQUIRE(isGray(output.getPixelAt(1, 1)));
        REQUIRE(isGray(output.getPixelAt(2, 1)));
        REQUIRE(output.getPixelAt(1, 1).getAlpha() == original.getAlpha());
        REQUIRE(output.getPixelAt(2, 1).getAlpha() == translucent.getAlpha());
        REQUIRE(source.getPixelAt(1, 1) == original);
        REQUIRE(source.getPixelAt(2, 1) == translucent);
        if (format == juce::Image::ARGB)
            REQUIRE(output.getPixelAt(0, 0).getAlpha() == 0);

        juce::Image faded(juce::Image::ARGB, 4, 4, true);
        juce::Graphics g(faded);
        effect.applyEffect(source, g, 1.0f, 0.5f);
        REQUIRE(faded.getPixelAt(1, 1).getAlpha() >= 127 && faded.getPixelAt(1, 1).getAlpha() <= 128);
        REQUIRE(source.getPixelAt(1, 1) == original);
    }
}

void testWholeSubtreeAndStateTransitions()
{
    // The same controller is used for every rack plug-in role, with no type branches.
    for (const auto *type : {"effect", "midiEffect", "instrument"})
    {
        juce::ValueTree state("PLUGIN");
        state.setProperty("type", type, nullptr);
        state.setProperty("parameter", 0.375, nullptr);
        ColourPanel root(juce::Colours::red), child(juce::Colours::green), grandchild(juce::Colours::blue);
        juce::TextButton button("Still editable");
        root.setSize(100, 100);
        root.setVisible(true);
        root.addAndMakeVisible(child);
        child.setBounds(10, 10, 50, 50);
        child.addAndMakeVisible(grandchild);
        grandchild.setBounds(10, 10, 20, 20);
        root.addAndMakeVisible(button);
        button.setBounds(10, 70, 80, 20);
        const auto initialState = state.createCopy();
        const auto active = root.createComponentSnapshot(root.getLocalBounds());
        {
            PluginBypassPresentation presentation(root, state);
            REQUIRE(!presentation.isBypassed());
            REQUIRE(root.getComponentEffect() == nullptr);
            REQUIRE(!state.hasProperty("enabled"));
            REQUIRE(state.isEquivalentTo(initialState));

            juce::UndoManager undo;
            undo.beginNewTransaction("Bypass");
            state.setProperty("enabled", false, &undo);
            REQUIRE(presentation.isBypassed());
            REQUIRE(root.getComponentEffect() != nullptr);
            REQUIRE(root.isEnabled() && child.isEnabled() && grandchild.isEnabled() && button.isEnabled());
            REQUIRE(root.getComponentAt(juce::Point<int>(40, 80)) == &button);
            REQUIRE(static_cast<double>(state.getProperty("parameter")) == 0.375);
            const auto bypassed = root.createComponentSnapshot(root.getLocalBounds());
            REQUIRE(isGray(bypassed.getPixelAt(5, 5))); // Parent/header/background.
            REQUIRE(isGray(bypassed.getPixelAt(15, 15))); // Embedded GUI.
            REQUIRE(isGray(bypassed.getPixelAt(25, 25))); // Nested graph/control.
            REQUIRE(!isGray(active.getPixelAt(5, 5)) && !isGray(active.getPixelAt(25, 25)));

            REQUIRE(undo.undo());
            REQUIRE(!presentation.isBypassed());
            REQUIRE(root.getComponentEffect() == nullptr);
            const auto restored = root.createComponentSnapshot(root.getLocalBounds());
            REQUIRE(restored.getPixelAt(5, 5) == active.getPixelAt(5, 5));
            REQUIRE(restored.getPixelAt(15, 15) == active.getPixelAt(15, 15));
            REQUIRE(restored.getPixelAt(25, 25) == active.getPixelAt(25, 25));
            REQUIRE(state.isEquivalentTo(initialState));
            REQUIRE(undo.redo());
            REQUIRE(presentation.isBypassed());
            state.setProperty("parameter", 0.625, nullptr);
            REQUIRE(presentation.isBypassed() && root.isEnabled() && button.isEnabled());
            REQUIRE(static_cast<double>(state.getProperty("parameter")) == 0.625);
            state.setProperty("enabled", true, nullptr);
            REQUIRE(!presentation.isBypassed());
            state.setProperty("enabled", false, nullptr);
        }
        REQUIRE(root.getComponentEffect() == nullptr); // No dangling filter after teardown.
        REQUIRE(!static_cast<bool>(state.getProperty("enabled"))); // Teardown never enables the plug-in.
        {
            PluginBypassPresentation initialBypass(root, state);
            REQUIRE(initialBypass.isBypassed());
            REQUIRE(root.getComponentEffect() != nullptr);
        }
    }
}

void testBadgeBoundsAndContrast()
{
    for (const int height : {0, 10, 25, 60, 100, 240, 300})
    {
        const juce::Rectangle<int> header(0, 0, 20, height);
        const auto areas = PluginBypassPresentation::getHeaderAreas(header);
        REQUIRE(areas.title.getX() >= 0 && areas.title.getRight() <= 20);
        REQUIRE(areas.badge.getX() >= 0 && areas.badge.getRight() <= 20);
        REQUIRE(areas.title.getY() >= 0 && areas.title.getBottom() <= height);
        REQUIRE(areas.badge.getY() >= 0 && areas.badge.getBottom() <= height);
        REQUIRE(!areas.title.intersects(areas.badge));
        if (height >= 100)
        {
            REQUIRE(areas.badge.getY() >= 25);
            REQUIRE(areas.badge.getHeight() >= 67);
        }
    }
    juce::Image header(juce::Image::ARGB, 20, 300, true);
    {
        juce::Graphics g(header);
        g.fillAll(juce::Colours::grey);
        PluginBypassPresentation::paintHeader(g, header.getBounds(), "Effect Name", juce::Colours::white);
    }
    const auto badge = PluginBypassPresentation::getHeaderAreas(header.getBounds()).badge;
    int brightPixels = 0, darkPixels = 0;
    for (int y = badge.getY() + 2; y < badge.getBottom() - 2; ++y)
        for (int x = badge.getX() + 2; x < badge.getRight() - 2; ++x)
        {
            const auto pixel = header.getPixelAt(x, y);
            if (pixel.getBrightness() > 0.8f) ++brightPixels;
            if (pixel.getBrightness() < 0.2f) ++darkPixels;
        }
    REQUIRE(brightPixels > 40); // Visible light status lettering, not just desaturation.
    REQUIRE(darkPixels > 200); // Contrasting dark badge background.
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI initialise;
    testFilterPreservesSourceAndAlpha();
    testWholeSubtreeAndStateTransitions();
    testBadgeBoundsAndContrast();
    if (failures == 0)
        std::cout << "Plug-in bypass presentation tests passed\n";
    return failures == 0 ? 0 : 1;
}
