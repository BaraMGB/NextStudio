#include "ComputerMidiKeyboardLayout.h"

#include <iostream>

namespace
{
int failures = 0;

void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        ++failures;
    }
}

void testPrimaryMappings()
{
    const auto &mappings = ComputerMidiKeyboardLayout::getPrimaryMappings();

    require(mappings.size() == 24, "layout has 24 primary mappings");
    require(juce::String(mappings.front().description) == "y", "lower C starts on Y");
    require(mappings.front().noteOffset == 0, "lower C offset is 0");
    require(juce::String(mappings[11].description) == "m", "lower B ends on M");
    require(mappings[11].noteOffset == 11, "lower B offset is 11");
    require(juce::String(mappings[12].description) == "2", "upper C sharp starts on 2");
    require(mappings[12].noteOffset == 13, "upper C sharp offset skips alias C");
    require(juce::String(mappings.back().description) == "i", "top C ends on I");
    require(mappings.back().noteOffset == 24, "top C offset is 24");
}

void testAliases()
{
    const auto &aliases = ComputerMidiKeyboardLayout::getUpperCAliases();

    require(aliases.size() == 2, "upper C has two aliases");
    require(ComputerMidiKeyboardLayout::isMappedPerformanceKey(aliases[0]), "Q is a mapped performance key");
    require(ComputerMidiKeyboardLayout::isMappedPerformanceKey(aliases[1]), "comma is a mapped performance key");
    require(ComputerMidiKeyboardLayout::isMappedPerformanceKey(juce::KeyPress::createFromDescription("y")), "Y remains mapped");
    require(!ComputerMidiKeyboardLayout::isMappedPerformanceKey(juce::KeyPress::createFromDescription("a")), "A is not a mapped performance key");
}

void testKeyboardConfiguration()
{
    juce::MidiKeyboardState state;
    juce::MidiKeyboardComponent keyboard(state, juce::MidiKeyboardComponent::horizontalKeyboard);
    ComputerMidiKeyboardLayout::applyTo(keyboard);

    require(keyboard.getMidiChannel() == ComputerMidiKeyboardLayout::midiChannel, "keyboard uses MIDI channel 1");

    for (const auto &mapping : ComputerMidiKeyboardLayout::getPrimaryMappings())
        require(keyboard.keyPressed(juce::KeyPress::createFromDescription(mapping.description)), "primary mapping is registered");

    require(!keyboard.keyPressed(juce::KeyPress::createFromDescription("a")), "default A mapping is cleared");
    require(!keyboard.keyPressed(juce::KeyPress::createFromDescription("q")), "Q alias is handled outside JUCE key map");
    require(!keyboard.keyPressed(juce::KeyPress::createFromDescription(",")), "comma alias is handled outside JUCE key map");
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    testPrimaryMappings();
    testAliases();
    testKeyboardConfiguration();

    if (failures != 0)
        return 1;

    std::cout << "Computer MIDI keyboard layout tests passed\n";
    return 0;
}
