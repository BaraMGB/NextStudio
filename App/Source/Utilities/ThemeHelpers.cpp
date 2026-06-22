#include "ThemeHelpers.h"
#include "ThemePresets.h"

namespace
{
struct BuiltInThemePreset
{
    const char *name;
    const char *fileName;
};

constexpr BuiltInThemePreset builtInThemePresets[]{{"Dark", "Dark.nxttheme"}, {"Light", "Light.nxttheme"}};

const BuiltInThemePreset *findBuiltInThemePreset(const juce::String &themeName)
{
    for (const auto &preset : builtInThemePresets)
        if (themeName.equalsIgnoreCase(preset.name) || themeName.equalsIgnoreCase(preset.fileName) || themeName.equalsIgnoreCase(juce::File(preset.fileName).getFileNameWithoutExtension()))
            return &preset;

    return nullptr;
}

const char *getBuiltInThemeData(const juce::String &fileName, int &dataSize)
{
    for (int i = 0; i < ThemePresets::namedResourceListSize; ++i)
    {
        if (juce::File(ThemePresets::originalFilenames[i]).getFileName().equalsIgnoreCase(fileName))
            return ThemePresets::getNamedResource(ThemePresets::namedResourceList[i], dataSize);
    }

    dataSize = 0;
    return nullptr;
}

bool applyThemeData(ApplicationViewState &appState, const char *data, int dataSize)
{
    if (data == nullptr || dataSize <= 0)
        return false;

    const auto xmlText = juce::String::fromUTF8(data, dataSize);
    auto xml = juce::XmlDocument::parse(xmlText);
    if (xml == nullptr)
        return false;

    return appState.applyThemeState(juce::ValueTree::fromXml(*xml));
}
} // namespace

namespace ThemeHelpers
{
juce::String getDefaultBuiltInThemeName() { return builtInThemePresets[0].name; }

juce::StringArray getBuiltInThemeNames()
{
    juce::StringArray names;
    for (const auto &preset : builtInThemePresets)
        names.add(preset.name);

    return names;
}

bool applyBuiltInTheme(ApplicationViewState &appState, const juce::String &themeName)
{
    const auto *preset = findBuiltInThemePreset(themeName.isNotEmpty() ? themeName : getDefaultBuiltInThemeName());
    if (preset == nullptr)
        preset = &builtInThemePresets[0];

    int dataSize = 0;
    const auto *data = getBuiltInThemeData(preset->fileName, dataSize);
    return applyThemeData(appState, data, dataSize);
}

void applyLookAndFeelColours(juce::LookAndFeel &lookAndFeel, ApplicationViewState &appState)
{
    lookAndFeel.setColour(juce::AlertWindow::backgroundColourId, appState.getBackgroundColour1());
    lookAndFeel.setColour(juce::AlertWindow::textColourId, appState.getTextColour());
    lookAndFeel.setColour(juce::AlertWindow::outlineColourId, appState.getBorderColour());

    lookAndFeel.setColour(juce::Label::textColourId, appState.getTextColour());
    lookAndFeel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    lookAndFeel.setColour(juce::TextButton::buttonColourId, appState.getButtonBackgroundColour());
    lookAndFeel.setColour(juce::TextButton::textColourOffId, appState.getButtonTextColour());
    lookAndFeel.setColour(juce::TextButton::textColourOnId, appState.getButtonTextColour());

    lookAndFeel.setColour(juce::ComboBox::textColourId, appState.getTextColour());
    lookAndFeel.setColour(juce::ComboBox::backgroundColourId, appState.getBackgroundColour1());
    lookAndFeel.setColour(juce::ComboBox::outlineColourId, appState.getBorderColour());
    lookAndFeel.setColour(juce::ComboBox::arrowColourId, appState.getTextColour());
    lookAndFeel.setColour(juce::ComboBox::focusedOutlineColourId, appState.getPrimeColour());

    lookAndFeel.setColour(juce::TextEditor::textColourId, appState.getTextColour());
    lookAndFeel.setColour(juce::TextEditor::backgroundColourId, appState.getBackgroundColour1());
    lookAndFeel.setColour(juce::TextEditor::highlightColourId, appState.getPrimeColour());
    lookAndFeel.setColour(juce::TextEditor::outlineColourId, appState.getBorderColour());
    lookAndFeel.setColour(juce::TextEditor::focusedOutlineColourId, appState.getPrimeColour());
    lookAndFeel.setColour(juce::TextEditor::shadowColourId, appState.getBackgroundColour1().darker(0.3f));

    lookAndFeel.setColour(juce::Slider::textBoxTextColourId, appState.getTextColour());
    lookAndFeel.setColour(juce::Slider::textBoxBackgroundColourId, appState.getBackgroundColour1());
    lookAndFeel.setColour(juce::Slider::textBoxHighlightColourId, appState.getPrimeColour());
    lookAndFeel.setColour(juce::Slider::textBoxOutlineColourId, appState.getBorderColour());

    lookAndFeel.setColour(juce::TabbedButtonBar::tabTextColourId, appState.getTextColour());
    lookAndFeel.setColour(juce::TabbedButtonBar::frontTextColourId, appState.getPrimeColour());
    lookAndFeel.setColour(juce::TabbedButtonBar::tabOutlineColourId, appState.getBorderColour());
    lookAndFeel.setColour(juce::TabbedButtonBar::frontOutlineColourId, appState.getBorderColour());

    lookAndFeel.setColour(juce::TableHeaderComponent::textColourId, appState.getTextColour());
    lookAndFeel.setColour(juce::TableHeaderComponent::backgroundColourId, appState.getBackgroundColour2());
    lookAndFeel.setColour(juce::TableHeaderComponent::outlineColourId, appState.getBorderColour());
    lookAndFeel.setColour(juce::TableHeaderComponent::highlightColourId, appState.getPrimeColour());
    lookAndFeel.setColour(juce::TableListBox::textColourId, appState.getTextColour());
    lookAndFeel.setColour(juce::TableListBox::backgroundColourId, appState.getBackgroundColour2());

    lookAndFeel.setColour(juce::DrawableButton::textColourId, appState.getButtonTextColour());
    lookAndFeel.setColour(juce::DrawableButton::textColourOnId, appState.getButtonTextColour());
    lookAndFeel.setColour(juce::ResizableWindow::backgroundColourId, appState.getBackgroundColour2());

    lookAndFeel.setColour(juce::PopupMenu::backgroundColourId, appState.getBackgroundColour1());
    lookAndFeel.setColour(juce::PopupMenu::textColourId, appState.getTextColour());
    lookAndFeel.setColour(juce::PopupMenu::highlightedBackgroundColourId, appState.getPrimeColour());
    lookAndFeel.setColour(juce::PopupMenu::highlightedTextColourId, appState.getPrimeColour().contrasting(0.8f));

    lookAndFeel.setColour(juce::TooltipWindow::backgroundColourId, appState.getBackgroundColour1());
    lookAndFeel.setColour(juce::TooltipWindow::textColourId, appState.getTextColour());
    lookAndFeel.setColour(juce::TooltipWindow::outlineColourId, appState.getBorderColour());
}

} // namespace ThemeHelpers
