#include "ThemeSettingsComponent.h"

#include "InitialContentSetup.h"
#include "ThemeHelpers.h"

#include <algorithm>

namespace
{
constexpr int margin = 8;
constexpr int titleHeight = 28;
constexpr int presetHeight = 28;
constexpr int actionHeight = 28;
constexpr int statusHeight = 34;
constexpr int saveRowHeight = 30;
constexpr int groupHeight = 24;
constexpr int colourRowHeight = 32;
constexpr int editorGap = 6;

juce::String displayHex(juce::Colour colour)
{
    return "#" + juce::String::toHexString((int)colour.getRed()).toUpperCase().paddedLeft('0', 2)
           + juce::String::toHexString((int)colour.getGreen()).toUpperCase().paddedLeft('0', 2)
           + juce::String::toHexString((int)colour.getBlue()).toUpperCase().paddedLeft('0', 2);
}
} // namespace

ThemeSettingsComponent::ThemeSettingsComponent(ApplicationViewState &appState)
    : m_appState(appState),
      m_presetModel(juce::File(m_appState.m_presetDir.get()).getChildFile("Themes"), ThemeHelpers::getBuiltInThemeNames())
{
    setWantsKeyboardFocus(true);
    InitialContentSetup::populateBundledContent(juce::File(m_appState.m_workDir.get()));

    m_presetBrowser.onPresetSelected = [this](const juce::String &name) { applyPreset(name); };
    addAndMakeVisible(m_presetBrowser);

    m_saveButton.onClick = [this] { saveActivePreset(); };
    m_saveAsButton.onClick = [this] { beginSaveAs(); };
    addAndMakeVisible(m_saveButton);
    addAndMakeVisible(m_saveAsButton);

    m_statusLabel.setJustificationType(juce::Justification::centredLeft);
    m_statusLabel.setMinimumHorizontalScale(0.8f);
    addAndMakeVisible(m_statusLabel);

    m_saveNameLabel.setText("Name", juce::dontSendNotification);
    m_saveNameLabel.setJustificationType(juce::Justification::centredLeft);
    m_saveNameEditor.setMultiLine(false);
    m_saveNameEditor.setTextToShowWhenEmpty("Theme name", juce::Colours::grey);
    m_saveNameEditor.onReturnKey = [this] { finishSaveAs(); };
    m_saveNameEditor.onEscapeKey = [this] { cancelSaveAs(); };
    m_saveNameEditor.onTextChange = [this]
    {
        m_pendingOverwriteName.clear();
        m_saveNameButton.setButtonText("Save");
    };
    m_saveNameButton.onClick = [this] { finishSaveAs(); };
    m_cancelSaveButton.onClick = [this] { cancelSaveAs(); };
    addChildComponent(m_saveNameLabel);
    addChildComponent(m_saveNameEditor);
    addChildComponent(m_saveNameButton);
    addChildComponent(m_cancelSaveButton);

    m_colourEditor.onColourChanged = [this](juce::Colour colour)
    {
        const auto &definitions = getColourDefinitions();
        if (m_selectedColourIndex < 0 || m_selectedColourIndex >= (int)definitions.size())
            return;

        auto state = getThemeState();
        m_updatingState = true;
        state.setProperty(definitions[(size_t)m_selectedColourIndex].property, colour.toString(), nullptr);
        m_appState.refreshThemeCache();
        m_updatingState = false;
        triggerAsyncUpdate();
    };
    m_colourEditor.onReset = [this]
    {
        const auto &definitions = getColourDefinitions();
        if (m_selectedColourIndex < 0 || m_selectedColourIndex >= (int)definitions.size())
            return;

        const auto &property = definitions[(size_t)m_selectedColourIndex].property;
        const auto colour = getReferenceColour(property);
        m_colourEditor.setCurrentColour(colour, juce::dontSendNotification);
        auto state = getThemeState();
        state.setProperty(property, colour.toString(), nullptr);
        m_appState.refreshThemeCache();
        triggerAsyncUpdate();
    };
    addAndMakeVisible(m_colourEditor);

    auto themeState = getThemeState();
    themeState.addListener(this);
    rebuildPresets();
    updateEditorFromState();
}

ThemeSettingsComponent::~ThemeSettingsComponent()
{
    cancelPendingUpdate();
    getThemeState().removeListener(this);
}

int ThemeSettingsComponent::getPreferredHeight() const
{
    auto height = margin + titleHeight + presetHeight + margin / 2 + m_presetBrowser.getPreferredHeight() + margin / 2 + actionHeight + statusHeight;
    if (m_saveNameEditor.isVisible())
        height += saveRowHeight + margin / 2;
    height += titleHeight + groupHeight * 4 + colourRowHeight * (int)getColourDefinitions().size();
    if (m_selectedColourIndex >= 0)
        height += editorGap + m_colourEditor.getPreferredHeight();
    return height + margin;
}

void ThemeSettingsComponent::refresh()
{
    InitialContentSetup::populateBundledContent(juce::File(m_appState.m_workDir.get()));
    m_presetModel.setDirectory(juce::File(m_appState.m_presetDir.get()).getChildFile("Themes"));
    rebuildPresets();
    updateEditorFromState();
    repaint();
}

void ThemeSettingsComponent::paint(juce::Graphics &g)
{
    const auto text = m_appState.getTextColour();
    const auto border = m_appState.getBorderColour();
    const auto accent = m_appState.getPrimeColour();
    const auto rowBackground = m_appState.getBackgroundColour1();

    g.setColour(text);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("Theme Presets", m_presetTitleBounds, juce::Justification::centredLeft);
    g.drawText("Theme Colors", m_colourTitleBounds, juce::Justification::centredLeft);

    g.setFont(juce::FontOptions(11.0f));
    g.setColour(text.withAlpha(0.72f));
    g.drawText("Drop a .nxttheme file here to import it", m_dropHintBounds, juce::Justification::centredLeft);

    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    for (const auto &[name, bounds] : m_groupBounds)
    {
        g.setColour(text.withAlpha(0.68f));
        g.drawText(name.toUpperCase(), bounds, juce::Justification::centredLeft);
    }

    g.setFont(juce::FontOptions(13.0f));
    const auto &definitions = getColourDefinitions();
    for (size_t i = 0; i < m_colourRowBounds.size(); ++i)
    {
        const auto bounds = m_colourRowBounds[i];
        const auto selected = (int)i == m_selectedColourIndex;

        g.setColour(selected ? accent.withAlpha(0.16f) : rowBackground.withAlpha(0.62f));
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
        g.setColour(selected ? accent : border);
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 4.0f, selected ? 1.5f : 1.0f);

        auto content = bounds.reduced(8, 3);
        auto swatch = content.removeFromRight(26).reduced(2);
        const auto colour = getThemeColour(definitions[i].property);
        g.setColour(colour);
        g.fillRoundedRectangle(swatch.toFloat(), 3.0f);
        g.setColour(border);
        g.drawRoundedRectangle(swatch.toFloat().reduced(0.5f), 3.0f, 1.0f);

        if (getWidth() >= 330)
        {
            auto hexBounds = content.removeFromRight(76);
            g.setColour(text.withAlpha(0.7f));
            g.drawText(displayHex(colour), hexBounds, juce::Justification::centredRight);
        }

        g.setColour(text);
        g.drawFittedText(definitions[i].name, content, juce::Justification::centredLeft, 1);
    }

    if (m_dragActive)
    {
        g.setColour(accent.withAlpha(0.16f));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2.0f), 6.0f);
        g.setColour(accent);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2.0f), 6.0f, 2.0f);
    }
}

void ThemeSettingsComponent::resized()
{
    auto bounds = getLocalBounds().reduced(margin);
    m_presetTitleBounds = bounds.removeFromTop(titleHeight);

    auto quickPresets = bounds.removeFromTop(presetHeight);
    const auto quickButtonWidth = juce::jmax(1, quickPresets.getWidth() / juce::jmax(1, m_quickPresetButtons.size()));
    for (auto *button : m_quickPresetButtons)
        button->setBounds(quickPresets.removeFromLeft(quickButtonWidth).reduced(1));

    bounds.removeFromTop(margin / 2);
    m_presetBrowser.setBounds(bounds.removeFromTop(m_presetBrowser.getPreferredHeight()));
    bounds.removeFromTop(margin / 2);
    auto actions = bounds.removeFromTop(actionHeight);
    const auto actionWidth = juce::jmax(1, actions.getWidth() / 2);
    m_saveButton.setBounds(actions.removeFromLeft(actionWidth).reduced(1));
    m_saveAsButton.setBounds(actions.reduced(1));

    m_statusLabel.setBounds(bounds.removeFromTop(statusHeight));

    if (m_saveNameEditor.isVisible())
    {
        bounds.removeFromTop(margin / 2);
        auto saveRow = bounds.removeFromTop(saveRowHeight);
        m_saveNameLabel.setBounds(saveRow.removeFromLeft(40));
        m_cancelSaveButton.setBounds(saveRow.removeFromRight(58).reduced(1));
        m_saveNameButton.setBounds(saveRow.removeFromRight(68).reduced(1));
        saveRow.removeFromRight(3);
        m_saveNameEditor.setBounds(saveRow.reduced(1));
    }

    m_dropHintBounds = bounds.removeFromTop(titleHeight);
    m_colourTitleBounds = bounds.removeFromTop(titleHeight);
    m_groupBounds.clear();
    m_colourRowBounds.clear();

    juce::String currentGroup;
    const auto &definitions = getColourDefinitions();
    for (size_t i = 0; i < definitions.size(); ++i)
    {
        if (definitions[i].group != currentGroup)
        {
            currentGroup = definitions[i].group;
            m_groupBounds.emplace_back(currentGroup, bounds.removeFromTop(groupHeight));
        }

        m_colourRowBounds.push_back(bounds.removeFromTop(colourRowHeight).reduced(0, 2));
        if ((int)i == m_selectedColourIndex)
        {
            bounds.removeFromTop(editorGap);
            m_colourEditor.setBounds(bounds.removeFromTop(m_colourEditor.getPreferredHeight()));
        }
    }
}

void ThemeSettingsComponent::mouseDown(const juce::MouseEvent &event)
{
    for (size_t i = 0; i < m_colourRowBounds.size(); ++i)
    {
        if (m_colourRowBounds[i].contains(event.getPosition()))
        {
            grabKeyboardFocus();
            selectColour((int)i);
            return;
        }
    }
}

bool ThemeSettingsComponent::keyPressed(const juce::KeyPress &key)
{
    if (key == juce::KeyPress::upKey || key == juce::KeyPress::downKey)
    {
        const auto direction = key == juce::KeyPress::upKey ? -1 : 1;
        selectColour(juce::jlimit(0, (int)getColourDefinitions().size() - 1, m_selectedColourIndex + direction));
        return true;
    }

    if (key == juce::KeyPress::returnKey)
    {
        m_colourEditor.focusHexEditor();
        return true;
    }

    return false;
}

bool ThemeSettingsComponent::isInterestedInFileDrag(const juce::StringArray &files)
{
    return std::any_of(files.begin(), files.end(), [](const juce::String &path)
                       { return juce::File(path).hasFileExtension(".nxttheme"); });
}

void ThemeSettingsComponent::fileDragEnter(const juce::StringArray &, int, int)
{
    m_dragActive = true;
    repaint();
}

void ThemeSettingsComponent::fileDragExit(const juce::StringArray &)
{
    m_dragActive = false;
    repaint();
}

void ThemeSettingsComponent::filesDropped(const juce::StringArray &files, int, int)
{
    m_dragActive = false;
    int imported = 0;
    juce::String lastError;
    ThemePresetModel::Preset lastPreset;

    for (const auto &path : files)
    {
        if (!juce::File(path).hasFileExtension(".nxttheme"))
            continue;

        ThemePresetModel::Preset importedPreset;
        const auto result = m_presetModel.importPreset(juce::File(path), &importedPreset);
        if (result.wasOk())
        {
            ++imported;
            lastPreset = std::move(importedPreset);
        }
        else
        {
            lastError = result.getErrorMessage();
        }
    }

    if (imported > 0)
    {
        rebuildPresets();
        applyPreset(lastPreset.name);
        setStatus(imported == 1 ? "Imported and applied “" + lastPreset.name + "”." : juce::String(imported) + " themes imported; “" + lastPreset.name + "” applied.");
    }
    else
    {
        setStatus(lastError.isNotEmpty() ? lastError : "No valid theme file was dropped.", true);
    }

    repaint();
}

const std::vector<ThemeSettingsComponent::ColourDefinition> &ThemeSettingsComponent::getColourDefinitions()
{
    static const std::vector<ColourDefinition> definitions{
        {IDs::PrimeColour, "Accent", "General"},
        {IDs::BackgroundColour1, "Main background", "General"},
        {IDs::BackgroundColour2, "Secondary background", "General"},
        {IDs::BackgroundColour3, "Panel background", "General"},
        {IDs::MainFrameColour, "Main frame", "General"},
        {IDs::BorderColour, "Border", "General"},
        {IDs::MenuTextColour, "Menu text", "Controls"},
        {IDs::ButtonBackgroundColour, "Button background", "Controls"},
        {IDs::ButtonTextColour, "Button text", "Controls"},
        {IDs::timeLineBackgroundColour, "Background", "Timeline"},
        {IDs::timeLineStrokeColour, "Lines", "Timeline"},
        {IDs::timeLineShadowShade, "Shadow", "Timeline"},
        {IDs::timeLineTextColour, "Text", "Timeline"},
        {IDs::trackBackgroundColour, "Track background", "Tracks"},
        {IDs::trackHeaderBackgroundColour, "Header background", "Tracks"},
        {IDs::trackHeaderTextColour, "Header text", "Tracks"}};
    return definitions;
}

juce::ValueTree ThemeSettingsComponent::getThemeState() const
{
    return m_appState.m_applicationStateValueTree.getOrCreateChildWithName(IDs::ThemeState, nullptr);
}

juce::Colour ThemeSettingsComponent::getThemeColour(const juce::Identifier &property) const
{
    return juce::Colour::fromString(getThemeState()[property].toString()).withAlpha(1.0f);
}

juce::Colour ThemeSettingsComponent::getReferenceColour(const juce::Identifier &property) const
{
    if (m_referenceTheme.isValid() && m_referenceTheme.hasProperty(property))
        return juce::Colour::fromString(m_referenceTheme[property].toString());
    return getThemeColour(property);
}

void ThemeSettingsComponent::rebuildPresets()
{
    m_presets = m_presetModel.getPresets();
    const auto currentState = getThemeState();

    const auto exact = std::find_if(m_presets.begin(), m_presets.end(), [&currentState](const auto &preset)
                                    { return ThemePresetModel::areStatesEquivalent(currentState, preset.state); });
    if (exact != m_presets.end())
    {
        m_activePresetName = exact->name;
        m_referenceTheme = exact->state.createCopy();
    }
    else
    {
        const auto active = std::find_if(m_presets.begin(), m_presets.end(), [this](const auto &preset)
                                         { return preset.name.equalsIgnoreCase(m_activePresetName); });
        if (active != m_presets.end())
        {
            m_referenceTheme = active->state.createCopy();
        }
        else if (!m_presets.empty())
        {
            m_activePresetName = m_presets.front().name;
            m_referenceTheme = m_presets.front().state.createCopy();
        }
    }

    m_quickPresetButtons.clear();
    for (const auto &preset : m_presets)
    {
        if (!preset.builtIn)
            continue;

        auto button = std::make_unique<juce::TextButton>(preset.name);
        button->setClickingTogglesState(false);
        button->setToggleState(preset.name.equalsIgnoreCase(m_activePresetName), juce::dontSendNotification);
        const auto name = preset.name;
        button->onClick = [this, name] { applyPreset(name); };
        addAndMakeVisible(button.get());
        m_quickPresetButtons.add(std::move(button));
    }

    m_presetBrowser.setPresets(m_presets);
    m_presetBrowser.setActivePreset(m_activePresetName);
    updatePresetStatus();
    notifyPreferredHeightChanged();
}

void ThemeSettingsComponent::applyPreset(const juce::String &name)
{
    const auto found = std::find_if(m_presets.begin(), m_presets.end(), [&name](const auto &preset)
                                    { return preset.name.equalsIgnoreCase(name); });
    if (found == m_presets.end())
        return;

    m_updatingState = true;
    if (!m_appState.applyThemeState(found->state))
    {
        m_updatingState = false;
        setStatus("The selected theme could not be applied.", true);
        return;
    }
    m_updatingState = false;
    m_activePresetName = found->name;
    m_referenceTheme = found->state.createCopy();
    rebuildPresets();
    updateEditorFromState();
    repaint();
}

void ThemeSettingsComponent::updatePresetStatus()
{
    const auto found = std::find_if(m_presets.begin(), m_presets.end(), [this](const auto &preset)
                                    { return preset.name.equalsIgnoreCase(m_activePresetName); });
    const auto modified = found == m_presets.end() || !ThemePresetModel::areStatesEquivalent(getThemeState(), found->state);
    setStatus(m_activePresetName.isNotEmpty() ? m_activePresetName + (modified ? " — Modified" : " — Active") : "Custom theme");

    const auto canModifyPreset = found != m_presets.end() && !found->builtIn;
    m_saveButton.setEnabled(canModifyPreset && modified);

    for (auto *button : m_quickPresetButtons)
        button->setToggleState(button->getButtonText().equalsIgnoreCase(m_activePresetName), juce::dontSendNotification);
    m_presetBrowser.setActivePreset(m_activePresetName);
}

void ThemeSettingsComponent::beginSaveAs()
{
    m_pendingOverwriteName.clear();
    m_saveNameButton.setButtonText("Save");
    m_saveNameEditor.setText({}, juce::dontSendNotification);
    m_saveNameLabel.setVisible(true);
    m_saveNameEditor.setVisible(true);
    m_saveNameButton.setVisible(true);
    m_cancelSaveButton.setVisible(true);
    notifyPreferredHeightChanged();
    m_saveNameEditor.grabKeyboardFocus();
}

void ThemeSettingsComponent::finishSaveAs()
{
    const auto name = m_saveNameEditor.getText().trim();
    const auto validation = ThemePresetModel::validateName(name);
    if (validation.failed())
    {
        setStatus(validation.getErrorMessage(), true);
        return;
    }

    const auto existing = std::find_if(m_presets.begin(), m_presets.end(), [&name](const auto &preset)
                                       { return preset.name.equalsIgnoreCase(name); });
    if (existing != m_presets.end() && existing->builtIn)
    {
        setStatus("Built-in themes cannot be overwritten.", true);
        return;
    }

    const auto overwrite = m_pendingOverwriteName.equalsIgnoreCase(name);
    if (existing != m_presets.end() && !overwrite)
    {
        m_pendingOverwriteName = name;
        m_saveNameButton.setButtonText("Overwrite");
        setStatus("“" + existing->name + "” already exists. Confirm overwrite.", true);
        return;
    }

    ThemePresetModel::Preset saved;
    const auto result = m_presetModel.savePreset(name, getThemeState(), overwrite, &saved);
    if (result.failed())
    {
        setStatus(result.getErrorMessage(), true);
        return;
    }

    m_activePresetName = saved.name;
    m_referenceTheme = saved.state.createCopy();
    cancelSaveAs();
    rebuildPresets();
    setStatus("Saved “" + saved.name + "”.");
}

void ThemeSettingsComponent::cancelSaveAs()
{
    m_pendingOverwriteName.clear();
    m_saveNameLabel.setVisible(false);
    m_saveNameEditor.setVisible(false);
    m_saveNameButton.setVisible(false);
    m_cancelSaveButton.setVisible(false);
    m_saveNameButton.setButtonText("Save");
    notifyPreferredHeightChanged();
}

void ThemeSettingsComponent::saveActivePreset()
{
    const auto found = std::find_if(m_presets.begin(), m_presets.end(), [this](const auto &preset)
                                    { return preset.name.equalsIgnoreCase(m_activePresetName); });
    if (found == m_presets.end() || found->builtIn)
        return;

    ThemePresetModel::Preset saved;
    const auto result = m_presetModel.savePreset(found->name, getThemeState(), true, &saved);
    if (result.failed())
    {
        setStatus(result.getErrorMessage(), true);
        return;
    }

    m_referenceTheme = saved.state.createCopy();
    rebuildPresets();
    setStatus("Saved “" + saved.name + "”.");
}

void ThemeSettingsComponent::selectColour(int index)
{
    if (index < 0 || index >= (int)getColourDefinitions().size())
        return;

    m_selectedColourIndex = index;
    updateEditorFromState();
    notifyPreferredHeightChanged();
    scrollEditorIntoView();
    repaint();
}

void ThemeSettingsComponent::updateEditorFromState()
{
    const auto &definitions = getColourDefinitions();
    if (m_selectedColourIndex < 0 || m_selectedColourIndex >= (int)definitions.size())
        return;

    const auto &property = definitions[(size_t)m_selectedColourIndex].property;
    m_colourEditor.setCurrentColour(getThemeColour(property), juce::dontSendNotification);
    m_colourEditor.setReferenceColour(getReferenceColour(property));
    const auto background = m_appState.getBackgroundColour1();
    const auto text = m_appState.getTextColour();
    const auto border = m_appState.getBorderColour();
    const auto accent = m_appState.getPrimeColour();
    m_colourEditor.setThemeColours(background, text, border, accent);
    m_presetBrowser.setThemeColours(background, text, border, accent);
}

void ThemeSettingsComponent::scrollEditorIntoView()
{
    auto *viewport = dynamic_cast<juce::Viewport *>(getParentComponent());
    for (auto *parent = getParentComponent(); parent != nullptr && viewport == nullptr; parent = parent->getParentComponent())
        viewport = dynamic_cast<juce::Viewport *>(parent);

    if (viewport == nullptr || viewport->getViewedComponent() == nullptr)
        return;

    auto *viewed = viewport->getViewedComponent();
    const auto editorArea = viewed->getLocalArea(&m_colourEditor, m_colourEditor.getLocalBounds());
    const auto visibleArea = viewport->getViewArea();
    auto targetY = visibleArea.getY();

    if (editorArea.getBottom() + margin > visibleArea.getBottom())
        targetY = editorArea.getBottom() + margin - visibleArea.getHeight();
    else if (editorArea.getY() - margin < visibleArea.getY())
        targetY = editorArea.getY() - margin;

    if (targetY != visibleArea.getY())
        viewport->setViewPosition(viewport->getViewPositionX(), juce::jmax(0, targetY));
}

void ThemeSettingsComponent::setStatus(juce::String message, bool error)
{
    m_statusLabel.setText(std::move(message), juce::dontSendNotification);
    m_statusLabel.setColour(juce::Label::textColourId, error ? juce::Colours::red : m_appState.getTextColour().withAlpha(0.82f));
}

void ThemeSettingsComponent::notifyPreferredHeightChanged()
{
    if (m_onPreferredHeightChanged)
        m_onPreferredHeightChanged();

    // Rebuilding the quick-preset buttons may not change this component's bounds,
    // so JUCE will not necessarily call resized(). Lay out new children explicitly.
    resized();
}

void ThemeSettingsComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &)
{
    if (!m_updatingState)
        triggerAsyncUpdate();
}

void ThemeSettingsComponent::handleAsyncUpdate()
{
    m_appState.refreshThemeCache();
    updateEditorFromState();
    updatePresetStatus();
    repaint();
}
