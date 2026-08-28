#include "ApplicationViewState.h"
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

void requireEqual(const juce::String &actual, const juce::String &expected, const char *message)
{
    if (actual != expected)
    {
        std::cerr << "FAILED: " << message << " (expected='" << expected << "' actual='" << actual << "')\n";
        ++failures;
    }
}

void testDefaultMappings()
{
    const auto &mappings = ComputerMidiKeyboardLayout::getDefaultMappings();
    const auto state = ComputerMidiKeyboardLayout::createDefaultState();

    require(mappings.size() == ComputerMidiKeyboardLayout::noteCount, "layout has 25 primary note mappings");
    require(juce::String(mappings.front().noteName) == "C3", "first note is C3");
    require(juce::String(mappings.front().defaultDescription) == "y", "lower C starts on Y");
    require(mappings.front().noteOffset == 0, "lower C offset is 0");
    require(juce::String(mappings[ComputerMidiKeyboardLayout::upperCIndex].noteName) == "C4", "upper C note exists in primary layout");
    require(juce::String(mappings[ComputerMidiKeyboardLayout::upperCIndex].defaultDescription) == "q", "upper C primary key is Q");
    require(mappings[ComputerMidiKeyboardLayout::upperCIndex].noteOffset == 12, "upper C offset is 12");
    require(juce::String(mappings.back().noteName) == "C5", "highest note is C5");
    require(juce::String(mappings.back().defaultDescription) == "i", "top C ends on I");
    require(mappings.back().noteOffset == 24, "top C offset is 24");

    requireEqual(state.primaryKeyDescriptions.front(), ComputerMidiKeyboardLayout::normaliseKeyDescription("y"), "default state stores lower C key");
    requireEqual(state.primaryKeyDescriptions[ComputerMidiKeyboardLayout::upperCIndex], ComputerMidiKeyboardLayout::normaliseKeyDescription("q"), "default state stores upper C key");
    requireEqual(state.upperCAliasDescription, ComputerMidiKeyboardLayout::normaliseKeyDescription(","), "default state stores upper C alias");
}

void testTracktionOctaveLabels()
{
    const auto &mappings = ComputerMidiKeyboardLayout::getDefaultMappings();

    for (const auto &mapping : mappings)
    {
        const auto midiNote = ComputerMidiKeyboardLayout::baseMidiNote + mapping.noteOffset;
        requireEqual(juce::String(mapping.noteName),
                     juce::MidiMessage::getMidiNoteName(midiNote, true, true, 4),
                     "layout labels follow Tracktion middle-C convention");
    }
}

void testValidationAndAliases()
{
    auto state = ComputerMidiKeyboardLayout::createDefaultState();

    require(ComputerMidiKeyboardLayout::validate(state).isEmpty(), "default layout validates");
    require(ComputerMidiKeyboardLayout::isMappedPerformanceKey(juce::KeyPress::createFromDescription("q"), state), "Q is a mapped performance key");
    require(ComputerMidiKeyboardLayout::isMappedPerformanceKey(juce::KeyPress::createFromDescription(","), state), "comma alias is a mapped performance key");
    require(!ComputerMidiKeyboardLayout::isMappedPerformanceKey(juce::KeyPress::createFromDescription("a"), state), "A is not a mapped performance key");
    require(ComputerMidiKeyboardLayout::normaliseKeyDescription("ctrl + a").isEmpty(), "modifier shortcuts are rejected for performance keys");

    state.upperCAliasDescription = "q";
    require(ComputerMidiKeyboardLayout::validate(state).isNotEmpty(), "duplicate alias is rejected");
}

void testKeyboardConfiguration()
{
    juce::MidiKeyboardState state;
    juce::MidiKeyboardComponent keyboard(state, juce::MidiKeyboardComponent::horizontalKeyboard);
    const auto layout = ComputerMidiKeyboardLayout::createDefaultState();
    ComputerMidiKeyboardLayout::applyTo(keyboard, layout);

    require(keyboard.getMidiChannel() == ComputerMidiKeyboardLayout::midiChannel, "keyboard uses MIDI channel 1");

    for (const auto &description : layout.primaryKeyDescriptions)
    {
        const auto key = ComputerMidiKeyboardLayout::keyPressFromDescription(description);
        require(key.isValid() && keyboard.keyPressed(key), "primary mapping is registered");
    }

    require(!keyboard.keyPressed(juce::KeyPress::createFromDescription("a")), "default A mapping is cleared");
    require(!keyboard.keyPressed(juce::KeyPress::createFromDescription(",")), "comma alias is handled outside JUCE key map");
}

void testPersistence()
{
    const auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getChildFile("nextstudio-computer-midi-keyboard-layout-tests.xml");
    tempFile.deleteFile();

    {
        ApplicationViewState appState(tempFile);
        auto state = ComputerMidiKeyboardLayout::createDefaultState();
        state.primaryKeyDescriptions[0] = "a";
        state.primaryKeyDescriptions[1] = {};
        state.upperCAliasDescription = "p";
        ComputerMidiKeyboardLayout::saveTo(appState, state);
        appState.saveState();
    }

    {
        ApplicationViewState appState(tempFile);
        const auto state = ComputerMidiKeyboardLayout::loadFrom(appState);
        requireEqual(state.primaryKeyDescriptions[0], ComputerMidiKeyboardLayout::normaliseKeyDescription("a"), "custom lower C key persists");
        require(state.primaryKeyDescriptions[1].isEmpty(), "cleared mapping persists");
        requireEqual(state.upperCAliasDescription, ComputerMidiKeyboardLayout::normaliseKeyDescription("p"), "custom upper C alias persists");
    }

    tempFile.deleteFile();
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    testDefaultMappings();
    testTracktionOctaveLabels();
    testValidationAndAliases();
    testKeyboardConfiguration();
    testPersistence();

    if (failures != 0)
        return 1;

    std::cout << "Computer MIDI keyboard layout tests passed\n";
    return 0;
}
