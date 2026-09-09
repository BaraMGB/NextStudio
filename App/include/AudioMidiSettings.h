
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#pragma once
#include "PluginBrowser.h"
#include "ApplicationViewState.h"
#include "AudioSettingsComponent.h"
#include "InitialContentSetup.h"
#include "KeyboardSettingsComponent.h"
#include "ThemeSettingsComponent.h"
#include "Utilities.h"
#include "juce_core/juce_core.h"
#include <functional>

namespace te = tracktion_engine;
class MidiSettings
    : public juce::Component
    , public juce::ComboBox::Listener
    , public juce::Button::Listener
{
public:
    explicit MidiSettings(te::Engine &engine, ApplicationViewState &appState);
    ~MidiSettings() override;
    void resized() override;
    void visibilityChanged() override;
    void comboBoxChanged(juce::ComboBox *comboBox) override;
    void buttonClicked(juce::Button *button) override;

private:
    void populateMidiDevices();

    ApplicationViewState &m_appState;
    juce::ToggleButton m_exclusiveMidiFocusButton;
    juce::Label m_exclusiveMidiFocusLabel;
    juce::ComboBox m_midiDefaultChooser;
    juce::Label m_midiDefaultLabel;
    te::Engine &m_engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiSettings)
};

class GeneralSettings : public juce::Component
{
public:
    explicit GeneralSettings(te::Engine &engine, ApplicationViewState &appState)
        : m_engine(engine),
          m_appState(appState),
          m_themeSettings(appState)
    {
        m_scaleLabel.setText("Scaling Factor:", juce::dontSendNotification);
        m_content.addAndMakeVisible(m_scaleLabel);

        m_scaleSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        m_scaleSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 22);
        m_scaleSlider.setTextValueSuffix("x");
        m_scaleSlider.setRange(0.2, 3.0, 0.01);
        m_scaleSlider.setNumDecimalPlacesToDisplay(2);
        m_scaleSlider.setSliderSnapsToMousePosition(false);
        m_scaleSlider.setMouseDragSensitivity(800);
        m_scaleSlider.setValue(juce::jlimit(0.2f, 3.0f, (float)m_appState.m_appScale.get()), juce::dontSendNotification);
        m_scaleSlider.setScrollWheelEnabled(false);
        m_scaleSlider.onValueChange = [this]() { updateScale(); };
        m_content.addAndMakeVisible(m_scaleSlider);

        m_mouseScaleLabel.setText("Mouse Cursor Scaling:", juce::dontSendNotification);
        m_content.addAndMakeVisible(m_mouseScaleLabel);

        m_mouseScaleEditor.setMultiLine(false);
        m_mouseScaleEditor.setJustification(juce::Justification::centredLeft);
        m_mouseScaleEditor.setText(juce::String(m_appState.m_mouseCursorScale), juce::dontSendNotification);
        m_mouseScaleEditor.onFocusLost = [this]() { updateMouseScale(); };
        m_mouseScaleEditor.onReturnKey = [this]() { updateMouseScale(); };
        m_content.addAndMakeVisible(m_mouseScaleEditor);

        m_timeStretchLabel.setText("Time-Stretch Algorithm:", juce::dontSendNotification);
        m_content.addAndMakeVisible(m_timeStretchLabel);

        m_timeStretchCombo.setTextWhenNothingSelected("No algorithm available");
        m_timeStretchCombo.onChange = [this] { updateTimeStretchMode(); };
        m_content.addAndMakeVisible(m_timeStretchCombo);
        refreshTimeStretchModes();

        m_contentPathLabel.setText("Content Folder:", juce::dontSendNotification);
        m_content.addAndMakeVisible(m_contentPathLabel);

        m_changeContentPathButton.onClick = [this]() { chooseContentPath(); };
        m_content.addAndMakeVisible(m_changeContentPathButton);

        m_contentPathValue.setJustificationType(juce::Justification::centredLeft);
        m_content.addAndMakeVisible(m_contentPathValue);
        updateContentPathLabel();

        m_versionLabel.setText("Version:", juce::dontSendNotification);
        m_content.addAndMakeVisible(m_versionLabel);

        if (auto *app = juce::JUCEApplication::getInstance())
            m_versionValue.setText(app->getApplicationVersion(), juce::dontSendNotification);
        else
            m_versionValue.setText("unknown", juce::dontSendNotification);
        m_versionValue.setJustificationType(juce::Justification::centredLeft);
        m_content.addAndMakeVisible(m_versionValue);

        m_themeSettings.setOnPreferredHeightChanged([this] { resized(); });
        m_content.addAndMakeVisible(m_themeSettings);

        m_viewport = std::make_unique<juce::Viewport>();
        addAndMakeVisible(m_viewport.get());
        m_viewport->setViewedComponent(&m_content, false);
        m_viewport->setScrollBarThickness(m_appState.getScrollbarThickness());
        m_viewport->setScrollBarsShown(true, false, true, false);
    }

    ~GeneralSettings() override { m_viewport->setViewedComponent(nullptr, false); }

    void setOnContentPathChanged(std::function<void()> callback) { m_onContentPathChanged = std::move(callback); }
    void refreshThemeFromAppState()
    {
        m_themeSettings.refresh();
        repaint();
    }

    void visibilityChanged() override
    {
        if (isVisible())
        {
            refreshTimeStretchModes();
            m_themeSettings.refresh();
        }
    }

    void resized() override
    {
        m_viewport->setBounds(getLocalBounds());

        constexpr int rowHeight = 24;
        constexpr int padding = 10;
        const int contentWidth = juce::jmax(1, m_viewport->getWidth() - m_viewport->getScrollBarThickness());
        const int controlsHeight = rowHeight * 6 + padding;
        const int themeHeight = m_themeSettings.getPreferredHeight();
        m_content.setSize(contentWidth, juce::jmax(m_viewport->getHeight(), controlsHeight + themeHeight));

        auto bounds = m_content.getLocalBounds();
        auto scaleRow = bounds.removeFromTop(rowHeight);
        m_scaleLabel.setBounds(scaleRow.removeFromLeft(140));
        m_scaleSlider.setBounds(scaleRow.reduced(2));

        auto mouseScaleRow = bounds.removeFromTop(rowHeight);
        m_mouseScaleLabel.setBounds(mouseScaleRow.removeFromLeft(140));
        m_mouseScaleEditor.setBounds(mouseScaleRow.removeFromLeft(100).reduced(2));

        auto timeStretchRow = bounds.removeFromTop(rowHeight);
        m_timeStretchLabel.setBounds(timeStretchRow.removeFromLeft(140));
        m_timeStretchCombo.setBounds(timeStretchRow.reduced(2));

        auto contentPathRow = bounds.removeFromTop(rowHeight);
        m_contentPathLabel.setBounds(contentPathRow.removeFromLeft(140));
        m_changeContentPathButton.setBounds(contentPathRow.removeFromLeft(110).reduced(2));

        auto contentPathValueRow = bounds.removeFromTop(rowHeight);
        contentPathValueRow.removeFromLeft(140);
        m_contentPathValue.setBounds(contentPathValueRow.reduced(2));

        auto versionRow = bounds.removeFromTop(rowHeight);
        m_versionLabel.setBounds(versionRow.removeFromLeft(140));
        m_versionValue.setBounds(versionRow.reduced(2));

        bounds.removeFromTop(padding);
        m_themeSettings.setBounds(bounds.removeFromTop(themeHeight));
    }

private:
    te::Engine &m_engine;
    ApplicationViewState &m_appState;
    juce::Label m_scaleLabel;
    juce::Slider m_scaleSlider;
    juce::Label m_mouseScaleLabel;
    juce::TextEditor m_mouseScaleEditor;
    juce::Label m_timeStretchLabel;
    juce::ComboBox m_timeStretchCombo;
    juce::Label m_contentPathLabel;
    juce::Label m_contentPathValue;
    juce::Label m_versionLabel;
    juce::Label m_versionValue;
    juce::TextButton m_changeContentPathButton{"Change..."};
    juce::Component m_content;
    std::unique_ptr<juce::Viewport> m_viewport;
    ThemeSettingsComponent m_themeSettings;
    std::function<void()> m_onContentPathChanged;

    void refreshTimeStretchModes()
    {
        m_timeStretchCombo.clear(juce::dontSendNotification);

        const auto modeNames = te::TimeStretcher::getPossibleModes(m_engine, true);
        for (int i = 0; i < modeNames.size(); ++i)
            m_timeStretchCombo.addItem(modeNames[i], i + 1);

        const auto defaultModeName = EngineHelpers::getDefaultTimeStretchModeName(m_engine);
        const auto currentModeName = modeNames.contains(juce::String(m_appState.m_timeStretchMode)) ? juce::String(m_appState.m_timeStretchMode) : defaultModeName;

        if (currentModeName.isNotEmpty())
            m_appState.m_timeStretchMode = currentModeName;

        const auto selectedIndex = modeNames.indexOf(currentModeName);
        m_timeStretchCombo.setSelectedId(selectedIndex >= 0 ? selectedIndex + 1 : 0, juce::dontSendNotification);
        m_timeStretchCombo.setEnabled(modeNames.size() > 0);
    }

    void updateTimeStretchMode()
    {
        const auto selectedText = m_timeStretchCombo.getText();
        if (selectedText.isNotEmpty())
            m_appState.m_timeStretchMode = selectedText;
    }

    void showError(const juce::String &message) { juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Content Folder", message); }

    void updateContentPathLabel()
    {
        const auto path = m_appState.m_workDir.get();
        m_contentPathValue.setText(path, juce::dontSendNotification);
        m_contentPathValue.setTooltip(path);
    }

    void chooseContentPath()
    {
        const auto currentRoot = juce::File(m_appState.m_workDir.get());
        juce::FileChooser chooser("Select NextStudio User Folder...", currentRoot, "*");
        if (!chooser.browseForDirectory())
            return;

        const auto newRoot = chooser.getResult();
        if (!newRoot.isDirectory() || newRoot == currentRoot)
            return;

        juce::String errorMessage;
        if (!InitialContentSetup::validateAndPrepareRoot(newRoot, errorMessage))
        {
            showError(errorMessage);
            return;
        }

        m_appState.setRootFolder(newRoot);
        m_themeSettings.refresh();
        m_appState.m_setupComplete = true;
        m_appState.saveState();
        updateContentPathLabel();

        if (m_onContentPathChanged)
            m_onContentPathChanged();
    }

    void updateScale()
    {
        const auto newScale = (float)m_scaleSlider.getValue();
        juce::Desktop::getInstance().setGlobalScaleFactor(newScale);
        m_appState.m_appScale = newScale;
    }

    void updateMouseScale()
    {
        const float newMouseScale = m_mouseScaleEditor.getText().getFloatValue();
        if (newMouseScale >= 0.2f && newMouseScale <= 3.0f)
            m_appState.m_mouseCursorScale = newMouseScale;
        else
            m_mouseScaleEditor.setText(juce::String(m_appState.m_mouseCursorScale));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneralSettings)
};

// ----------------------------------------------------------------

class SettingsView
    : public juce::TabbedComponent
    , private juce::ValueTree::Listener
{
public:
    SettingsView(te::Engine &engine, te::Edit &edit, juce::ApplicationCommandManager &commandManager, ApplicationViewState &appState)
        : juce::TabbedComponent(juce::TabbedButtonBar::Orientation::TabsAtTop),
          m_appState(appState),
          m_commandManager(commandManager),
          m_midiSettings(engine, appState),
          m_generalSettings(engine, appState),
          m_audioSettings(engine, edit, appState),
          m_keyboardSettings(appState, *m_commandManager.getKeyMappings()),
          m_pluginBrowser(engine, appState)
    {
        setOutline(0);
        addTab("Audio", appState.getBackgroundColour2(), &m_audioSettings, true);
        addTab("MIDI", appState.getBackgroundColour2(), &m_midiSettings, true);
        addTab("Plugins", appState.getBackgroundColour2(), &m_pluginBrowser, true);
        addTab("General", appState.getBackgroundColour2(), &m_generalSettings, true);
        addTab("Keys", appState.getBackgroundColour2(), &m_keyboardSettings, true);
        applyThemeToTabs();
        m_appState.m_applicationStateValueTree.getChildWithName(IDs::ThemeState).addListener(this);
    }
    ~SettingsView() override
    {
        m_appState.m_applicationStateValueTree.getChildWithName(IDs::ThemeState).removeListener(this);
    }
    void setOnContentPathChanged(std::function<void()> callback) { m_generalSettings.setOnContentPathChanged(std::move(callback)); }
    void refreshThemeFromAppState()
    {
        m_generalSettings.refreshThemeFromAppState();
        m_pluginBrowser.refreshThemeFromAppState();
        applyThemeToTabs();
    }

private:
    void valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &) override
    {
        if (treeWhosePropertyHasChanged.hasType(IDs::ThemeState))
            applyThemeToTabs();
    }

    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree &) override {}
    void valueTreeRedirected(juce::ValueTree &) override {}

    void applyThemeToTabs()
    {
        setColour(juce::TabbedComponent::backgroundColourId, m_appState.getBackgroundColour2());
        setColour(juce::TabbedComponent::outlineColourId, m_appState.getBorderColour());
        getTabbedButtonBar().setColour(juce::TabbedButtonBar::tabTextColourId, m_appState.getTextColour());
        getTabbedButtonBar().setColour(juce::TabbedButtonBar::frontTextColourId, m_appState.getPrimeColour());

        m_keyboardSettings.refreshThemeFromAppState();

        for (int i = 0; i < getNumTabs(); ++i)
        {
            setTabBackgroundColour(i, m_appState.getBackgroundColour2());

            if (auto *component = getTabContentComponent(i))
            {
                component->sendLookAndFeelChange();
                component->repaint();
            }
        }

        getTabbedButtonBar().repaint();
        repaint();
    }

    ApplicationViewState &m_appState;

    juce::ApplicationCommandManager &m_commandManager;
    MidiSettings m_midiSettings;
    GeneralSettings m_generalSettings;
    AudioSettingsComponent m_audioSettings;
    KeyboardSettingsComponent m_keyboardSettings;
    PluginSettings m_pluginBrowser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsView)
};
