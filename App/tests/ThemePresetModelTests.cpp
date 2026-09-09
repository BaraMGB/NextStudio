#include "ApplicationViewState.h"
#include "InlineColourEditor.h"
#include "ThemePresetBrowser.h"
#include "ThemePresetModel.h"

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

juce::ValueTree makeTheme(const juce::String &colour)
{
    juce::ValueTree state(IDs::ThemeState);
    state.setProperty(IDs::PrimeColour, colour, nullptr);
    state.setProperty(IDs::BackgroundColour1, "ff101010", nullptr);
    return state;
}

void testNameValidation()
{
    require(ThemePresetModel::validateName("My Theme").wasOk(), "normal preset name is valid");
    require(ThemePresetModel::validateName("").failed(), "empty preset name is rejected");
    require(ThemePresetModel::validateName(" Theme").failed(), "leading whitespace is rejected");
    require(ThemePresetModel::validateName("Theme/Alt").failed(), "path separator is rejected");
}

void testColourConversion()
{
    const auto source = juce::Colour(0x7f123abc);
    require(InlineColourEditor::formatHexColour(source) == "#123ABC", "hex formatting uses RGB notation");

    juce::Colour parsed;
    require(InlineColourEditor::parseHexColour("#A1b2C3", parsed), "six-digit hex color is accepted");
    require(parsed.getRed() == 0xa1 && parsed.getGreen() == 0xb2 && parsed.getBlue() == 0xc3, "hex channels are parsed correctly");
    require(parsed.isOpaque(), "hex parsing always creates an opaque color");
    require(!InlineColourEditor::parseHexColour("#12345", parsed), "short hex color is rejected");
    require(!InlineColourEditor::parseHexColour("#12GG34", parsed), "non-hex characters are rejected");
}

void testStateComparison()
{
    const auto a = makeTheme("ff112233");
    const auto b = makeTheme("ff112233");
    auto different = makeTheme("ff445566");

    require(ThemePresetModel::areStatesEquivalent(a, b), "identical theme states compare equal");
    require(!ThemePresetModel::areStatesEquivalent(a, different), "different theme states do not compare equal");
    different.setProperty("Extra", 1, nullptr);
    require(!ThemePresetModel::areStatesEquivalent(a, different), "extra theme properties are detected");
}

juce::TextEditor *findTextEditor(juce::Component &component)
{
    for (auto *child : component.getChildren())
    {
        if (auto *editor = dynamic_cast<juce::TextEditor *>(child))
            return editor;
        if (auto *editor = findTextEditor(*child))
            return editor;
    }
    return nullptr;
}

void testWheelForwarding()
{
    juce::Component content;
    InlineColourEditor editor;
    juce::Viewport viewport;
    content.addAndMakeVisible(editor);
    content.setSize(280, 900);
    editor.setBounds(0, 300, 280, editor.getPreferredHeight());
    viewport.setSize(300, 180);
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false, true, false);
    viewport.setViewPosition(0, 250);

    auto *textEditor = findTextEditor(editor);
    require(textEditor != nullptr, "inline colour editor exposes an editable hex field");

    if (textEditor != nullptr)
    {
        const auto beforeColour = editor.getCurrentColour();
        const auto beforePosition = viewport.getViewPositionY();
        const auto now = juce::Time::getCurrentTime();
        juce::MouseEvent event(juce::Desktop::getInstance().getMainMouseSource(),
                               {2.0f, 2.0f}, juce::ModifierKeys(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                               textEditor, textEditor, now, {2.0f, 2.0f}, now, 0, false);
        juce::MouseWheelDetails wheel;
        wheel.deltaY = -1.0f;
        textEditor->mouseWheelMove(event, wheel);

        require(viewport.getViewPositionY() > beforePosition, "wheel over hex field scrolls the parent viewport");
        require(editor.getCurrentColour() == beforeColour, "wheel over hex field does not change the colour");
    }

    viewport.setViewedComponent(nullptr, false);
}

void testPresetBrowserScales()
{
    ThemePresetBrowser browser;
    std::vector<ThemePresetModel::Preset> presets;
    presets.push_back({"Dark", {}, makeTheme("ff101010"), true});
    presets.push_back({"Light", {}, makeTheme("fff0f0f0"), true});
    for (int i = 1; i <= 50; ++i)
        presets.push_back({"Theme " + juce::String(i), {}, makeTheme("ff112233"), false});

    const auto preferredHeight = browser.getPreferredHeight();
    browser.setPresets(presets);
    require(browser.getPreferredHeight() == preferredHeight, "preset browser height does not grow with the number of themes");

    auto *list = dynamic_cast<juce::ListBox *>(browser.getChildComponent(2));
    auto *search = dynamic_cast<juce::TextEditor *>(browser.getChildComponent(1));
    require(list != nullptr && list->getListBoxModel() != nullptr, "preset browser exposes its virtualized list");
    require(search != nullptr, "preset browser exposes its search field");

    if (list != nullptr && list->getListBoxModel() != nullptr)
        require(list->getListBoxModel()->getNumRows() == 54, "built-in and custom themes are grouped without creating buttons");

    juce::Component content;
    juce::Viewport outerViewport;
    content.addAndMakeVisible(browser);
    content.setSize(280, 900);
    browser.setBounds(0, 300, 280, browser.getPreferredHeight());
    outerViewport.setSize(300, 180);
    outerViewport.setViewedComponent(&content, false);
    outerViewport.setScrollBarsShown(true, false, true, false);
    outerViewport.setViewPosition(0, 250);

    if (list != nullptr && list->getViewport() != nullptr)
    {
        auto *listViewport = list->getViewport();
        const auto now = juce::Time::getCurrentTime();
        juce::MouseEvent event(juce::Desktop::getInstance().getMainMouseSource(),
                               {2.0f, 2.0f}, juce::ModifierKeys(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                               listViewport, listViewport, now, {2.0f, 2.0f}, now, 0, false);
        juce::MouseWheelDetails wheel;
        wheel.deltaY = -1.0f;
        listViewport->mouseWheelMove(event, wheel);
        require(listViewport->getViewPositionY() > 0, "wheel scrolls the preset list while more rows are available");
        require(outerViewport.getViewPositionY() == 250, "inner preset scrolling does not move the General viewport");

        listViewport->setViewPosition(0, 0);
        wheel.deltaY = 1.0f;
        browser.mouseWheelMove(event, wheel);
        require(outerViewport.getViewPositionY() < 250, "wheel at a preset-list boundary continues scrolling the General viewport");
    }

    if (search != nullptr && list != nullptr && list->getListBoxModel() != nullptr)
    {
        search->setText("Theme 49", false);
        search->onTextChange();
        require(list->getListBoxModel()->getNumRows() == 2, "preset search filters the custom-theme group");
    }

    outerViewport.setViewedComponent(nullptr, false);
}

void testPresetLifecycle()
{
    const auto directory = juce::File::getSpecialLocation(juce::File::tempDirectory)
                               .getChildFile("nextstudio-theme-preset-model-tests");
    directory.deleteRecursively();
    require(directory.createDirectory(), "temporary theme directory is created");

    ThemePresetModel model(directory, {"Dark", "Light"});
    const auto firstState = makeTheme("ff112233");
    ThemePresetModel::Preset saved;

    require(model.savePreset("My Theme", firstState, false, &saved).wasOk(), "custom preset can be saved");
    require(saved.file.existsAsFile(), "saved preset file exists");
    require(model.savePreset("My Theme", firstState, false).failed(), "existing preset needs overwrite confirmation");
    require(model.savePreset("Dark", firstState, true).failed(), "built-in preset cannot be overwritten");

    auto secondState = makeTheme("80445566");
    secondState.setProperty(juce::Identifier("BackgroundColour3"), "7fabcdef", nullptr);
    require(model.savePreset("My Theme", secondState, true).wasOk(), "custom preset can be overwritten");

    auto presets = model.getPresets();
    require(presets.size() == 1, "one custom preset is listed");
    require(presets.front().name == "My Theme", "preset keeps its display name");
    require(presets.front().state[IDs::PrimeColour].toString() == "ff445566", "saved theme colors are normalised to opaque ARGB");
    require(!presets.front().state.hasProperty(juce::Identifier("BackgroundColour3")), "obsolete panel background is removed from saved themes");

    const auto browserSource = directory.getSiblingFile("My Theme.nxttheme");
    browserSource.deleteFile();
    auto browserXml = std::unique_ptr<juce::XmlElement>(secondState.createXml());
    require(browserXml != nullptr && browserXml->writeTo(browserSource, {}), "Home-browser theme fixture is written");

    const auto loadedState = ThemePresetModel::loadThemeState(browserSource);
    require(loadedState.isValid(), "theme state can be loaded directly for browser activation");
    require(loadedState[IDs::PrimeColour].toString() == "ff445566", "browser-loaded theme colors are normalized");
    require(!loadedState.hasProperty(juce::Identifier("BackgroundColour3")), "browser-loaded themes discard obsolete properties");

    browserSource.deleteFile();
    directory.deleteRecursively();
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    testNameValidation();
    testColourConversion();
    testStateComparison();
    testWheelForwarding();
    testPresetBrowserScales();
    testPresetLifecycle();

    if (failures != 0)
        return 1;

    std::cout << "Theme preset model tests passed\n";
    return 0;
}
