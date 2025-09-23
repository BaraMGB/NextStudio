
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
#include "ApplicationViewState.h"
#include "PluginBrowser.h"
#include "juce_core/juce_core.h"

namespace te = tracktion_engine;
class MidiSettings : public juce::Component
, public juce::ComboBox::Listener, public juce::Button::Listener
{
public:

    explicit MidiSettings(te::Engine& engine, ApplicationViewState& appState);
    ~MidiSettings() override;
    void resized () override;
    void comboBoxChanged(juce::ComboBox *comboBox) override;
    void buttonClicked(juce::Button* button) override;

private:
    ApplicationViewState& m_appState;
    juce::ToggleButton m_exclusiveMidiFocusButton;
    juce::Label m_exclusiveMidiFocusLabel;
    juce::ComboBox m_midiDefaultChooser;
    juce::Label m_midiDefaultLabel;
    te::Engine& m_engine;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSettings)
};

class ColourSettingsPanel : public juce::Component, private juce::ChangeListener, public juce::ChangeBroadcaster
{
public:
    class ColourButton : public juce::TextButton
    {
    public:
        ColourButton(const juce::String& name = "ColourButton")
            : juce::TextButton(name)
        {
            setButtonText("");
        }

        void setCurrentColour(const juce::Colour& colour)
        {
            m_currentColour = colour;
            repaint();
        }

        juce::Colour getCurrentColour() const
        {
            return m_currentColour;
        }

        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            auto bounds = getLocalBounds().toFloat();

            g.setColour(m_currentColour);
            g.fillRoundedRectangle(bounds, 4.0f);

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

    ColourSettingsPanel(ApplicationViewState& appState)
        : m_appState(appState), m_activeSelector(-1)
    {
        m_colourSettings.add(new ColourSetting{ IDs::PrimeColour,                 appState.m_primeColour });
        m_colourSettings.add(new ColourSetting{ IDs::BackgroundColour1,           appState.m_guiBackground1 });
        m_colourSettings.add(new ColourSetting{ IDs::BackgroundColour2,           appState.m_guiBackground2 });
        m_colourSettings.add(new ColourSetting{ IDs::BackgroundColour3,           appState.m_guiBackground3 });
        m_colourSettings.add(new ColourSetting{ IDs::MenuTextColour,              appState.m_textColour });
        m_colourSettings.add(new ColourSetting{ IDs::MainFrameColour,             appState.m_mainFrameColour });
        m_colourSettings.add(new ColourSetting{ IDs::BorderColour,                appState.m_borderColour });
        m_colourSettings.add(new ColourSetting{ IDs::ButtonBackgroundColour,      appState.m_buttonBackgroundColour });
        m_colourSettings.add(new ColourSetting{ IDs::ButtonTextColour,            appState.m_buttonTextColour });
        m_colourSettings.add(new ColourSetting{ IDs::timeLineStrokeColour,        appState.m_timeLine_strokeColour });
        m_colourSettings.add(new ColourSetting{ IDs::timeLineShadowShade,         appState.m_timeLine_shadowShade });
        m_colourSettings.add(new ColourSetting{ IDs::timeLineTextColour,          appState.m_timeLine_textColour });
        m_colourSettings.add(new ColourSetting{ IDs::timeLineBackgroundColour,    appState.m_timeLine_background });
        m_colourSettings.add(new ColourSetting{ IDs::trackBackgroundColour,       appState.m_trackBackgroundColour });
        m_colourSettings.add(new ColourSetting{ IDs::trackHeaderBackgroundColour, appState.m_trackHeaderBackgroundColour });
        m_colourSettings.add(new ColourSetting{ IDs::trackHeaderTextColour,       appState.m_trackHeaderTextColour });

        for (int i = 0; i < m_colourSettings.size(); i++)
        {
            auto setting = m_colourSettings[i];

            auto label = std::make_unique<juce::Label>();
            label->setText(setting->id.toString(), juce::dontSendNotification);
            addAndMakeVisible(*label);
            m_labels.add(std::move(label));

            auto button = std::make_unique<ColourButton>();
            button->setCurrentColour(juce::Colour::fromString(juce::String(setting->colour)));
            button->onClick = [this, i]() { showColourSelector(i); };
            addAndMakeVisible(button.get());
            m_colourButtons.add(std::move(button));

            auto selector = std::make_unique<juce::ColourSelector>(
                juce::ColourSelector::showAlphaChannel |
                juce::ColourSelector::showColourspace |
                juce::ColourSelector::showSliders
            );
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

    ~ColourSettingsPanel() override
    {
        for (auto* selector : m_selectors)
        {
            selector->removeChangeListener(this);
        }
    }

    int getPreferredHeight() const
    {
        const int buttonRowHeight = 30;
        const int selectorHeight = 250;
        const int baseHeight = m_colourSettings.size() * buttonRowHeight + buttonRowHeight;

        return baseHeight + (m_activeSelector >= 0 ? selectorHeight : 0);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int rowHeight = 30;
        const int labelWidth = 200;
        const int buttonWidth = 40;
        const int selectorHeight = 250;

        const auto selectorRect = bounds.removeFromTop(selectorHeight);

        bounds.removeFromTop(rowHeight);

        for (int i = 0; i < m_labels.size(); i++)
        {
            auto row = bounds.removeFromTop(rowHeight);
            m_labels[i]->setBounds(row.removeFromLeft(labelWidth).reduced(2));
            m_colourButtons[i]->setBounds(row.removeFromLeft(buttonWidth).reduced(2));

            if (i == m_activeSelector)
            {
                m_selectors[i]->setBounds(selectorRect);
            }
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
    ApplicationViewState& m_appState;
    juce::OwnedArray<ColourSetting> m_colourSettings;
    juce::OwnedArray<juce::Label> m_labels;
    juce::OwnedArray<ColourButton> m_colourButtons;
    juce::OwnedArray<juce::ColourSelector> m_selectors;
    int m_activeSelector;


    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if (auto selector = dynamic_cast<juce::ColourSelector*>(source))
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

        auto themeState = m_appState.m_applicationStateValueTree
                  .getOrCreateChildWithName(IDs::ThemeState, nullptr);

        themeState.setProperty(m_colourSettings[index]->id, newColour.toString(), nullptr);

        juce::LookAndFeel::getDefaultLookAndFeel().setColour(juce::Label::textColourId, m_appState.getTextColour());

        m_colourButtons[index]->repaint();
    }
};

class GeneralSettings : public juce::Component
{
public:
    GeneralSettings(ApplicationViewState& appState)
    : m_appState(appState)
    {
        m_scaleLabel.setText("Scaling Factor:", juce::dontSendNotification);
        addAndMakeVisible(m_scaleLabel);

        m_scaleEditor.setMultiLine(false);
        m_scaleEditor.setJustification(juce::Justification::centredLeft);
        m_scaleEditor.setText(juce::String(m_appState.m_appScale),
                             juce::dontSendNotification);
        addAndMakeVisible(m_scaleEditor);

        m_scaleEditor.onFocusLost = [this]() { updateScaling(); };
        m_scaleEditor.onReturnKey = [this]() { updateScaling(); };


        m_mouseScaleLabel.setText("Mouse Cursor Scaling:", juce::dontSendNotification);
        addAndMakeVisible(m_mouseScaleLabel);

        m_mouseScaleEditor.setMultiLine(false);
        m_mouseScaleEditor.setJustification(juce::Justification::centredLeft);
        m_mouseScaleEditor.setText(juce::String(m_appState.m_mouseCursorScale),
                             juce::dontSendNotification);
        addAndMakeVisible(m_mouseScaleEditor);

        m_mouseScaleEditor.onFocusLost = [this]() { updateScaling(); };
        m_mouseScaleEditor.onReturnKey = [this]() { updateScaling(); };


        m_themeLabel.setText("Theme Colors:", juce::dontSendNotification);
        addAndMakeVisible(m_themeLabel);

        m_viewport = std::make_unique<juce::Viewport>();
        addAndMakeVisible(m_viewport.get());

        m_colourSettingsPanel = std::make_unique<ColourSettingsPanel>(appState);
        m_colourSettingsPanel->showColourSelector(0);
        m_viewport->setViewedComponent(m_colourSettingsPanel.get(), false);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int rowHeight = 24;
        const int padding = 10;
        const int scrollbarWidth = m_viewport->getScrollBarThickness();

        auto scaleRow = bounds.removeFromTop(rowHeight);
        m_scaleLabel.setBounds(scaleRow.removeFromLeft(140));
        m_scaleEditor.setBounds(scaleRow.removeFromLeft(100).reduced(2));

        auto mouseScaleRow = bounds.removeFromTop(rowHeight);
        m_mouseScaleLabel.setBounds(mouseScaleRow.removeFromLeft(140));
        m_mouseScaleEditor.setBounds(mouseScaleRow.removeFromLeft(100).reduced(2));

        bounds.removeFromTop(padding / 2);
        m_themeLabel.setBounds(bounds.removeFromTop(rowHeight));

        bounds.removeFromTop(padding / 2);
        m_viewport->setBounds(bounds);

        const int panelWidth = bounds.getWidth() - scrollbarWidth;
        const int panelHeight = m_colourSettingsPanel->getPreferredHeight();
        m_colourSettingsPanel->setSize(panelWidth, panelHeight);
    }

    ColourSettingsPanel * getColourSettings()
    {
        return m_colourSettingsPanel.get();
    }

private:
    ApplicationViewState& m_appState;
    juce::Label m_scaleLabel;
    juce::TextEditor m_scaleEditor;
    juce::Label m_mouseScaleLabel;
    juce::TextEditor m_mouseScaleEditor;
    juce::Label m_themeLabel;
    std::unique_ptr<juce::Viewport> m_viewport;
    std::unique_ptr<ColourSettingsPanel> m_colourSettingsPanel;

    void updateScaling()
    {
        float newScale = m_scaleEditor.getText().getFloatValue();
        if (newScale >= 0.2f && newScale <= 3.0f)
        {
            juce::Desktop::getInstance().setGlobalScaleFactor(newScale);
            m_appState.m_appScale = newScale;
        }
        else
        {
            m_scaleEditor.setText(juce::String(juce::Desktop::getInstance().getGlobalScaleFactor()),
                                 juce::dontSendNotification);
        }

        float newMouseScale = m_mouseScaleEditor.getText().getFloatValue();
        if (newMouseScale >= 0.2 && newMouseScale <= 3.0f)
        {
            m_appState.m_mouseCursorScale = newMouseScale;
        }
        else {
            m_mouseScaleEditor.setText(juce::String(m_appState.m_mouseCursorScale));
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneralSettings)
};


// ----------------------------------------------------------------

class SettingsView : public juce::TabbedComponent
                    , public juce::ChangeListener
{
public:
    SettingsView(te::Engine& engine, juce::ApplicationCommandManager& commandManager, ApplicationViewState& appState) : juce::TabbedComponent(juce::TabbedButtonBar::Orientation::TabsAtTop)
        , m_appState(appState)
        , m_commandManager(commandManager)
        , m_midiSettings(engine, appState)
        , m_generalSettings(appState)
        , m_audioSettings(engine.getDeviceManager().deviceManager, 0, 512, 1, 512, false, false, false, false)
        , m_keyMappingEditor(*m_commandManager.getKeyMappings(), true)
        , m_pluginBrowser(engine, appState)
    {
        setOutline(0);
        m_keyMappingEditor.setColours(appState.getBackgroundColour2(), appState.getTextColour());
        addTab("Audio", appState.getBackgroundColour2(), &m_audioSettings, true);
        addTab("MIDI", appState.getBackgroundColour2(), &m_midiSettings, true);
        addTab("Plugins",appState.getBackgroundColour2(), &m_pluginBrowser, true);
        addTab("General",appState.getBackgroundColour2(), &m_generalSettings, true);
        addTab("Keys", appState.getBackgroundColour2(), &m_keyMappingEditor, true);
        m_generalSettings.getColourSettings()->addChangeListener(this);
    }
    ~SettingsView() override
    {
        m_generalSettings.getColourSettings()->removeChangeListener(this);
    }
    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        m_keyMappingEditor.setColours(m_appState.getBackgroundColour2(), m_appState.getTextColour());
        for (int i = 0; i < getNumTabs(); i++)
        {
            setTabBackgroundColour(i, m_appState.getBackgroundColour2());
        }
    }

private:
    ApplicationViewState& m_appState;

    juce::ApplicationCommandManager & m_commandManager;
    MidiSettings m_midiSettings;
    GeneralSettings m_generalSettings;
    juce::AudioDeviceSelectorComponent m_audioSettings;
    juce::KeyMappingEditorComponent m_keyMappingEditor;
    PluginSettings m_pluginBrowser;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsView)
};
