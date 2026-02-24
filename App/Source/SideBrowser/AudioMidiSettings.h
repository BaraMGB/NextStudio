
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
#include "SideBrowser/PluginBrowser.h"
#include "Utilities/ApplicationViewState.h"
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

class ColourSettingsPanel
    : public juce::Component
    , private juce::ChangeListener
    , public juce::ChangeBroadcaster
{
public:
    class ColourButton : public juce::TextButton
    {
    public:
        ColourButton(const juce::String &name = "ColourButton")
            : juce::TextButton(name)
        {
            setButtonText(name);
        }

        void setCurrentColour(const juce::Colour &colour)
        {
            m_currentColour = colour;
            repaint();
        }

        juce::Colour getCurrentColour() const { return m_currentColour; }

        void paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            auto bounds = getLocalBounds().toFloat();

            g.setColour(m_currentColour);
            g.fillRoundedRectangle(bounds, 4.0f);

            // Draw text
            g.setColour(m_currentColour.contrasting());
            g.setFont(10.0f);
            g.drawFittedText(getButtonText(), getLocalBounds().reduced(2), juce::Justification::centred, 2);

            g.setColour(findColour(juce::TextButton::buttonColourId).contrasting());
            g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

            if (shouldDrawButtonAsDown)
            {
                g.setColour(juce::Colours::black.withAlpha(0.2f));
                g.fillRoundedRectangle(bounds, 4.0f);
            }
            else if (shouldDrawButtonAsHighlighted)
            {
                g.setColour(juce::Colours::white.withAlpha(0.1f));
                g.fillRoundedRectangle(bounds, 4.0f);
            }
        }

    private:
        juce::Colour m_currentColour{juce::Colours::grey};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColourButton)
    };

    struct ColourSetting
    {
        juce::Identifier id;
        juce::String colour;
    };

    ColourSettingsPanel(ApplicationViewState &appState)
        : m_appState(appState),
          m_activeSelector(-1)
    {
        m_colourSettings.add(new ColourSetting{IDs::PrimeColour, appState.m_primeColour});
        m_colourSettings.add(new ColourSetting{IDs::BackgroundColour1, appState.m_guiBackground1});
        m_colourSettings.add(new ColourSetting{IDs::BackgroundColour2, appState.m_guiBackground2});
        m_colourSettings.add(new ColourSetting{IDs::BackgroundColour3, appState.m_guiBackground3});
        m_colourSettings.add(new ColourSetting{IDs::MenuTextColour, appState.m_textColour});
        m_colourSettings.add(new ColourSetting{IDs::MainFrameColour, appState.m_mainFrameColour});
        m_colourSettings.add(new ColourSetting{IDs::BorderColour, appState.m_borderColour});
        m_colourSettings.add(new ColourSetting{IDs::ButtonBackgroundColour, appState.m_buttonBackgroundColour});
        m_colourSettings.add(new ColourSetting{IDs::ButtonTextColour, appState.m_buttonTextColour});
        m_colourSettings.add(new ColourSetting{IDs::timeLineStrokeColour, appState.m_timeLine_strokeColour});
        m_colourSettings.add(new ColourSetting{IDs::timeLineShadowShade, appState.m_timeLine_shadowShade});
        m_colourSettings.add(new ColourSetting{IDs::timeLineTextColour, appState.m_timeLine_textColour});
        m_colourSettings.add(new ColourSetting{IDs::timeLineBackgroundColour, appState.m_timeLine_background});
        m_colourSettings.add(new ColourSetting{IDs::trackBackgroundColour, appState.m_trackBackgroundColour});
        m_colourSettings.add(new ColourSetting{IDs::trackHeaderBackgroundColour, appState.m_trackHeaderBackgroundColour});
        m_colourSettings.add(new ColourSetting{IDs::trackHeaderTextColour, appState.m_trackHeaderTextColour});

        for (int i = 0; i < m_colourSettings.size(); i++)
        {
            auto setting = m_colourSettings[i];

            auto button = std::make_unique<ColourButton>(setting->id.toString());
            button->setCurrentColour(juce::Colour::fromString(juce::String(setting->colour)));
            button->onClick = [this, i]() { showColourSelector(i); };
            addAndMakeVisible(button.get());
            m_colourButtons.add(std::move(button));

            auto selector = std::make_unique<juce::ColourSelector>(juce::ColourSelector::showAlphaChannel | juce::ColourSelector::showColourspace | juce::ColourSelector::showSliders);
            selector->setColour(juce::ColourSelector::ColourIds::backgroundColourId, m_appState.getBackgroundColour1());
            selector->setColour(juce::ColourSelector::ColourIds::labelTextColourId, m_appState.getTextColour());
            selector->setCurrentColour(juce::Colour::fromString(setting->colour), juce::dontSendNotification);
            selector->setName(setting->id.toString());
            selector->addChangeListener(this);
            selector->setVisible(false);
            addAndMakeVisible(selector.get());
            m_selectors.add(std::move(selector));
        }
    }

    void refreshColors()
    {
        m_colourSettings.clear();
        m_colourSettings.add(new ColourSetting{IDs::PrimeColour, m_appState.m_primeColour});
        m_colourSettings.add(new ColourSetting{IDs::BackgroundColour1, m_appState.m_guiBackground1});
        m_colourSettings.add(new ColourSetting{IDs::BackgroundColour2, m_appState.m_guiBackground2});
        m_colourSettings.add(new ColourSetting{IDs::BackgroundColour3, m_appState.m_guiBackground3});
        m_colourSettings.add(new ColourSetting{IDs::MenuTextColour, m_appState.m_textColour});
        m_colourSettings.add(new ColourSetting{IDs::MainFrameColour, m_appState.m_mainFrameColour});
        m_colourSettings.add(new ColourSetting{IDs::BorderColour, m_appState.m_borderColour});
        m_colourSettings.add(new ColourSetting{IDs::ButtonBackgroundColour, m_appState.m_buttonBackgroundColour});
        m_colourSettings.add(new ColourSetting{IDs::ButtonTextColour, m_appState.m_buttonTextColour});
        m_colourSettings.add(new ColourSetting{IDs::timeLineStrokeColour, m_appState.m_timeLine_strokeColour});
        m_colourSettings.add(new ColourSetting{IDs::timeLineShadowShade, m_appState.m_timeLine_shadowShade});
        m_colourSettings.add(new ColourSetting{IDs::timeLineTextColour, m_appState.m_timeLine_textColour});
        m_colourSettings.add(new ColourSetting{IDs::timeLineBackgroundColour, m_appState.m_timeLine_background});
        m_colourSettings.add(new ColourSetting{IDs::trackBackgroundColour, m_appState.m_trackBackgroundColour});
        m_colourSettings.add(new ColourSetting{IDs::trackHeaderBackgroundColour, m_appState.m_trackHeaderBackgroundColour});
        m_colourSettings.add(new ColourSetting{IDs::trackHeaderTextColour, m_appState.m_trackHeaderTextColour});

        for (int i = 0; i < m_colourSettings.size(); i++)
        {
            auto setting = m_colourSettings[i];
            m_colourButtons[i]->setCurrentColour(juce::Colour::fromString(juce::String(setting->colour)));
            m_selectors[i]->setCurrentColour(juce::Colour::fromString(setting->colour), juce::dontSendNotification);
        }
        repaint();
    }

    ~ColourSettingsPanel() override
    {
        for (auto *selector : m_selectors)
        {
            selector->removeChangeListener(this);
        }
    }

    int getPreferredHeight() const
    {
        const int tileSize = 80;
        const int selectorHeight = 250;
        const int columns = juce::jmax(1, getWidth() / tileSize);
        const int rows = (m_colourButtons.size() + columns - 1) / columns;
        const int baseHeight = rows * tileSize + 20;

        return baseHeight + (m_activeSelector >= 0 ? selectorHeight : 0);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int selectorHeight = 250;
        const int tileSize = 80;

        if (m_activeSelector >= 0)
        {
            m_selectors[m_activeSelector]->setBounds(bounds.removeFromTop(selectorHeight).reduced(2));
        }

        bounds.removeFromTop(10); // padding

        const int columns = juce::jmax(1, bounds.getWidth() / tileSize);
        const int actualTileWidth = bounds.getWidth() / columns;
        const int actualTileHeight = tileSize;

        for (int i = 0; i < m_colourButtons.size(); i++)
        {
            const int row = i / columns;
            const int col = i % columns;

            auto tileBounds = juce::Rectangle<int>(col * actualTileWidth, bounds.getY() + row * actualTileHeight, actualTileWidth, actualTileHeight).reduced(2);
            m_colourButtons[i]->setBounds(tileBounds);
        }
    }

    void showColourSelector(int index)
    {
        if (m_activeSelector >= 0 && m_activeSelector < m_selectors.size())
        {
            m_selectors[m_activeSelector]->setVisible(false);
        }

        m_activeSelector = index;
        m_selectors[index]->setVisible(true);

        if (auto parent = getParentComponent())
        {
            resized();
            parent->resized();
        }
    }

private:
    ApplicationViewState &m_appState;
    juce::OwnedArray<ColourSetting> m_colourSettings;
    juce::OwnedArray<ColourButton> m_colourButtons;
    juce::OwnedArray<juce::ColourSelector> m_selectors;
    int m_activeSelector;

    void changeListenerCallback(juce::ChangeBroadcaster *source) override
    {
        if (auto selector = dynamic_cast<juce::ColourSelector *>(source))
        {
            int index = m_selectors.indexOf(selector);
            if (index >= 0)
            {
                colourChanged(index);
                sendChangeMessage();
            }
        }
    }

    void colourChanged(int index)
    {
        juce::Colour newColour = m_selectors[index]->getCurrentColour();
        m_colourSettings[index]->colour = newColour.toString();
        m_colourButtons[index]->setCurrentColour(newColour);

        auto themeState = m_appState.m_applicationStateValueTree.getOrCreateChildWithName(IDs::ThemeState, nullptr);

        themeState.setProperty(m_colourSettings[index]->id, newColour.toString(), nullptr);

        juce::LookAndFeel::getDefaultLookAndFeel().setColour(juce::Label::textColourId, m_appState.getTextColour());

        m_colourButtons[index]->repaint();
    }
};

class GeneralSettings : public juce::Component
{
public:
    explicit GeneralSettings(ApplicationViewState &appState)
        : m_appState(appState)
    {
        m_scaleLabel.setText("Scaling Factor:", juce::dontSendNotification);
        addAndMakeVisible(m_scaleLabel);

        m_scaleSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        m_scaleSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 22);
        m_scaleSlider.setTextValueSuffix("x");
        m_scaleSlider.setRange(0.2, 3.0, 0.01);
        m_scaleSlider.setNumDecimalPlacesToDisplay(2);
        m_scaleSlider.setSliderSnapsToMousePosition(false);
        m_scaleSlider.setMouseDragSensitivity(800);
        m_scaleSlider.setValue(juce::jlimit(0.2f, 3.0f, (float)m_appState.m_appScale.get()), juce::dontSendNotification);
        addAndMakeVisible(m_scaleSlider);
        m_scaleSlider.onValueChange = [this]() { updateScale(); };

        m_mouseScaleLabel.setText("Mouse Cursor Scaling:", juce::dontSendNotification);
        addAndMakeVisible(m_mouseScaleLabel);

        m_mouseScaleEditor.setMultiLine(false);
        m_mouseScaleEditor.setJustification(juce::Justification::centredLeft);
        m_mouseScaleEditor.setText(juce::String(m_appState.m_mouseCursorScale), juce::dontSendNotification);
        addAndMakeVisible(m_mouseScaleEditor);

        m_mouseScaleEditor.onFocusLost = [this]() { updateMouseScale(); };
        m_mouseScaleEditor.onReturnKey = [this]() { updateMouseScale(); };

        m_contentPathLabel.setText("Content Folder:", juce::dontSendNotification);
        addAndMakeVisible(m_contentPathLabel);

        m_changeContentPathButton.onClick = [this]() { chooseContentPath(); };
        addAndMakeVisible(m_changeContentPathButton);

        m_contentPathValue.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(m_contentPathValue);
        updateContentPathLabel();

        m_versionLabel.setText("Version:", juce::dontSendNotification);
        addAndMakeVisible(m_versionLabel);

        if (auto *app = juce::JUCEApplication::getInstance())
            m_versionValue.setText(app->getApplicationVersion(), juce::dontSendNotification);
        else
            m_versionValue.setText("unknown", juce::dontSendNotification);
        m_versionValue.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(m_versionValue);

        m_themeLabel.setText("Theme Colors:", juce::dontSendNotification);
        addAndMakeVisible(m_themeLabel);

        m_themePresetsLabel.setText("Theme Presets:", juce::dontSendNotification);
        addAndMakeVisible(m_themePresetsLabel);

        m_themeCombo.setTextWhenNothingSelected("Select Theme");
        m_themeCombo.onChange = [this] { loadThemeFromCombo(); };
        addAndMakeVisible(m_themeCombo);

        m_saveThemeButton.onClick = [this] { saveTheme(); };
        addAndMakeVisible(m_saveThemeButton);

        m_loadThemeButton.onClick = [this] { loadTheme(); };
        addAndMakeVisible(m_loadThemeButton);

        refreshThemeList();

        m_viewport = std::make_unique<juce::Viewport>();
        addAndMakeVisible(m_viewport.get());

        m_colourSettingsPanel = std::make_unique<ColourSettingsPanel>(appState);
        m_colourSettingsPanel->showColourSelector(0);
        m_viewport->setViewedComponent(m_colourSettingsPanel.get(), false);
    }

    void setOnContentPathChanged(std::function<void()> callback) { m_onContentPathChanged = std::move(callback); }

    void visibilityChanged() override
    {
        if (isVisible())
            refreshThemeList();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int rowHeight = 24;
        const int padding = 10;
        const int scrollbarWidth = m_viewport->getScrollBarThickness();

        auto scaleRow = bounds.removeFromTop(rowHeight);
        m_scaleLabel.setBounds(scaleRow.removeFromLeft(140));
        m_scaleSlider.setBounds(scaleRow.reduced(2));

        auto mouseScaleRow = bounds.removeFromTop(rowHeight);
        m_mouseScaleLabel.setBounds(mouseScaleRow.removeFromLeft(140));
        m_mouseScaleEditor.setBounds(mouseScaleRow.removeFromLeft(100).reduced(2));

        auto contentPathRow = bounds.removeFromTop(rowHeight);
        m_contentPathLabel.setBounds(contentPathRow.removeFromLeft(140));
        m_changeContentPathButton.setBounds(contentPathRow.removeFromLeft(110).reduced(2));

        auto contentPathValueRow = bounds.removeFromTop(rowHeight);
        contentPathValueRow.removeFromLeft(140);
        m_contentPathValue.setBounds(contentPathValueRow.reduced(2));

        auto versionRow = bounds.removeFromTop(rowHeight);
        m_versionLabel.setBounds(versionRow.removeFromLeft(140));
        m_versionValue.setBounds(versionRow.reduced(2));

        bounds.removeFromTop(padding / 2);
        auto themePresetRow1 = bounds.removeFromTop(rowHeight);
        m_themePresetsLabel.setBounds(themePresetRow1.removeFromLeft(120));
        m_themeCombo.setBounds(themePresetRow1.reduced(2));

        auto themePresetRow2 = bounds.removeFromTop(rowHeight);
        m_saveThemeButton.setBounds(themePresetRow2.removeFromLeft(themePresetRow2.getWidth() / 2).reduced(2));
        m_loadThemeButton.setBounds(themePresetRow2.reduced(2));

        bounds.removeFromTop(padding / 2);
        m_themeLabel.setBounds(bounds.removeFromTop(rowHeight));

        bounds.removeFromTop(padding / 2);
        m_viewport->setBounds(bounds);

        const int panelWidth = bounds.getWidth() - scrollbarWidth;
        const int panelHeight = m_colourSettingsPanel->getPreferredHeight();
        m_colourSettingsPanel->setSize(panelWidth, panelHeight);
    }

    ColourSettingsPanel *getColourSettings() { return m_colourSettingsPanel.get(); }

private:
    ApplicationViewState &m_appState;
    juce::Label m_scaleLabel;
    juce::Slider m_scaleSlider;
    juce::Label m_mouseScaleLabel;
    juce::TextEditor m_mouseScaleEditor;
    juce::Label m_contentPathLabel;
    juce::Label m_contentPathValue;
    juce::Label m_versionLabel;
    juce::Label m_versionValue;
    juce::TextButton m_changeContentPathButton{"Change..."};
    juce::Label m_themeLabel;
    juce::Label m_themePresetsLabel;
    juce::ComboBox m_themeCombo;
    juce::TextButton m_saveThemeButton{"Save Theme"};
    juce::TextButton m_loadThemeButton{"Load Theme"};
    std::unique_ptr<juce::Viewport> m_viewport;
    std::unique_ptr<ColourSettingsPanel> m_colourSettingsPanel;
    std::function<void()> m_onContentPathChanged;

    void refreshThemeList()
    {
        m_themeCombo.clear();

        auto themeDir = getThemeDirectory();
        if (!themeDir.exists())
            themeDir.createDirectory();

        juce::Array<juce::File> themeFiles;
        themeDir.findChildFiles(themeFiles, juce::File::findFiles, false, "*.nxttheme");
        themeFiles.sort();

        for (int i = 0; i < themeFiles.size(); ++i)
        {
            m_themeCombo.addItem(themeFiles[i].getFileNameWithoutExtension(), i + 1);
        }
    }

    void loadThemeFromCombo()
    {
        int selectedId = m_themeCombo.getSelectedId();
        if (selectedId <= 0)
            return;

        auto themeDir = getThemeDirectory();
        juce::Array<juce::File> themeFiles;
        themeDir.findChildFiles(themeFiles, juce::File::findFiles, false, "*.nxttheme");
        themeFiles.sort();

        int index = selectedId - 1;
        if (index >= 0 && index < themeFiles.size())
        {
            loadThemeFromFile(themeFiles[index]);
        }
    }

    void saveTheme()
    {
        auto themeDir = getThemeDirectory();
        if (!themeDir.exists())
            themeDir.createDirectory();

        juce::FileChooser fc("Save Theme", themeDir, "*.nxttheme");
        if (fc.browseForFileToSave(true))
        {
            auto file = fc.getResult();
            if (!file.hasFileExtension(".nxttheme"))
                file = file.withFileExtension(".nxttheme");

            auto themeState = m_appState.m_applicationStateValueTree.getChildWithName(IDs::ThemeState);
            if (themeState.isValid())
            {
                if (auto xml = std::unique_ptr<juce::XmlElement>(themeState.createXml()))
                {
                    xml->writeTo(file, {});
                    refreshThemeList();

                    for (int i = 0; i < m_themeCombo.getNumItems(); ++i)
                    {
                        if (m_themeCombo.getItemText(i) == file.getFileNameWithoutExtension())
                        {
                            m_themeCombo.setSelectedId(i + 1, juce::dontSendNotification);
                            break;
                        }
                    }
                }
            }
        }
    }

    void loadTheme()
    {
        auto themeDir = getThemeDirectory();
        juce::FileChooser fc("Load Theme", themeDir, "*.nxttheme");
        if (fc.browseForFileToOpen())
        {
            loadThemeFromFile(fc.getResult());
        }
    }

    void loadThemeFromFile(const juce::File &file)
    {
        if (auto xml = std::unique_ptr<juce::XmlElement>(juce::XmlDocument::parse(file)))
        {
            auto newThemeState = juce::ValueTree::fromXml(*xml);
            if (newThemeState.hasType(IDs::ThemeState))
            {
                auto currentThemeState = m_appState.m_applicationStateValueTree.getOrCreateChildWithName(IDs::ThemeState, nullptr);
                currentThemeState.copyPropertiesFrom(newThemeState, nullptr);

                m_appState.refreshThemeCache();

                if (m_colourSettingsPanel != nullptr)
                {
                    m_colourSettingsPanel->refreshColors();
                    m_colourSettingsPanel->sendChangeMessage();
                }

                resized();

                // Select in combo if it's in the theme directory
                if (file.getParentDirectory() == getThemeDirectory())
                {
                    for (int i = 0; i < m_themeCombo.getNumItems(); ++i)
                    {
                        if (m_themeCombo.getItemText(i) == file.getFileNameWithoutExtension())
                        {
                            m_themeCombo.setSelectedId(i + 1, juce::dontSendNotification);
                            break;
                        }
                    }
                }
                else
                {
                    m_themeCombo.setSelectedId(0, juce::dontSendNotification);
                }
            }
        }
    }

    juce::File getThemeDirectory() { return juce::File(m_appState.m_presetDir.get()).getChildFile("Themes"); }

    static bool ensureDirectory(const juce::File &directory, juce::StringArray &errors)
    {
        if (directory.existsAsFile())
        {
            errors.add("Path exists as file: " + directory.getFullPathName());
            return false;
        }

        if (directory.exists() && directory.isDirectory())
            return true;

        if (!directory.createDirectory() && !(directory.exists() && directory.isDirectory()))
        {
            errors.add("Unable to create directory: " + directory.getFullPathName());
            return false;
        }

        return true;
    }

    static bool ensureContentLayout(const juce::File &root, juce::StringArray &errors)
    {
        if (!ensureDirectory(root, errors))
            return false;

        if (!ensureDirectory(root.getChildFile("Presets"), errors))
            return false;
        if (!ensureDirectory(root.getChildFile("Presets").getChildFile("Themes"), errors))
            return false;
        if (!ensureDirectory(root.getChildFile("Clips"), errors))
            return false;
        if (!ensureDirectory(root.getChildFile("Renders"), errors))
            return false;
        if (!ensureDirectory(root.getChildFile("Samples"), errors))
            return false;
        if (!ensureDirectory(root.getChildFile("Projects"), errors))
            return false;

        return true;
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

        juce::StringArray layoutErrors;
        if (!ensureContentLayout(newRoot, layoutErrors))
        {
            showError(layoutErrors.joinIntoString("\n"));
            return;
        }

        m_appState.setRootFolder(newRoot);
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
        float newMouseScale = m_mouseScaleEditor.getText().getFloatValue();
        if (newMouseScale >= 0.2 && newMouseScale <= 3.0f)
        {
            m_appState.m_mouseCursorScale = newMouseScale;
        }
        else
        {
            m_mouseScaleEditor.setText(juce::String(m_appState.m_mouseCursorScale));
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneralSettings)
};

// ----------------------------------------------------------------

class SettingsView
    : public juce::TabbedComponent
    , public juce::ChangeListener
{
public:
    SettingsView(te::Engine &engine, juce::ApplicationCommandManager &commandManager, ApplicationViewState &appState)
        : juce::TabbedComponent(juce::TabbedButtonBar::Orientation::TabsAtTop),
          m_appState(appState),
          m_commandManager(commandManager),
          m_midiSettings(engine, appState),
          m_generalSettings(appState),
          m_audioSettings(engine.getDeviceManager().deviceManager, 0, 512, 1, 512, false, false, false, false),
          m_keyMappingEditor(*m_commandManager.getKeyMappings(), true),
          m_pluginBrowser(engine, appState)
    {
        setOutline(0);
        m_keyMappingEditor.setColours(appState.getBackgroundColour2(), appState.getTextColour());
        addTab("Audio", appState.getBackgroundColour2(), &m_audioSettings, true);
        addTab("MIDI", appState.getBackgroundColour2(), &m_midiSettings, true);
        addTab("Plugins", appState.getBackgroundColour2(), &m_pluginBrowser, true);
        addTab("General", appState.getBackgroundColour2(), &m_generalSettings, true);
        addTab("Keys", appState.getBackgroundColour2(), &m_keyMappingEditor, true);
        m_generalSettings.getColourSettings()->addChangeListener(this);
    }
    ~SettingsView() override { m_generalSettings.getColourSettings()->removeChangeListener(this); }
    void setOnContentPathChanged(std::function<void()> callback) { m_generalSettings.setOnContentPathChanged(std::move(callback)); }
    void changeListenerCallback(juce::ChangeBroadcaster *source) override
    {
        m_keyMappingEditor.setColours(m_appState.getBackgroundColour2(), m_appState.getTextColour());
        for (int i = 0; i < getNumTabs(); i++)
        {
            setTabBackgroundColour(i, m_appState.getBackgroundColour2());
        }
    }

private:
    ApplicationViewState &m_appState;

    juce::ApplicationCommandManager &m_commandManager;
    MidiSettings m_midiSettings;
    GeneralSettings m_generalSettings;
    juce::AudioDeviceSelectorComponent m_audioSettings;
    juce::KeyMappingEditorComponent m_keyMappingEditor;
    PluginSettings m_pluginBrowser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsView)
};
