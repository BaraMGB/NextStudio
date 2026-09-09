/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "ApplicationViewState.h"
#include "InlineColourEditor.h"
#include "ThemePresetBrowser.h"
#include "ThemePresetModel.h"

#include <functional>
#include <vector>

class ThemeSettingsComponent
    : public juce::Component
    , public juce::FileDragAndDropTarget
    , private juce::ValueTree::Listener
    , private juce::AsyncUpdater
{
public:
    explicit ThemeSettingsComponent(ApplicationViewState &appState);
    ~ThemeSettingsComponent() override;

    int getPreferredHeight() const;
    void refresh();
    void setOnPreferredHeightChanged(std::function<void()> callback) { m_onPreferredHeightChanged = std::move(callback); }

    void paint(juce::Graphics &graphics) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &event) override;
    bool keyPressed(const juce::KeyPress &key) override;

    bool isInterestedInFileDrag(const juce::StringArray &files) override;
    void fileDragEnter(const juce::StringArray &, int, int) override;
    void fileDragExit(const juce::StringArray &) override;
    void filesDropped(const juce::StringArray &files, int, int) override;

private:
    struct ColourDefinition
    {
        juce::Identifier property;
        juce::String name;
        juce::String group;
    };

    static const std::vector<ColourDefinition> &getColourDefinitions();
    juce::ValueTree getThemeState() const;
    juce::Colour getThemeColour(const juce::Identifier &property) const;
    juce::Colour getReferenceColour(const juce::Identifier &property) const;

    void rebuildPresets();
    void applyPreset(const juce::String &name);
    void updatePresetStatus();
    void beginSaveAs();
    void finishSaveAs();
    void cancelSaveAs();
    void saveActivePreset();
    void selectColour(int index);
    void updateEditorFromState();
    void scrollEditorIntoView();
    void setStatus(juce::String message, bool error = false);
    void notifyPreferredHeightChanged();

    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree &) override {}
    void valueTreeRedirected(juce::ValueTree &) override {}
    void handleAsyncUpdate() override;

    ApplicationViewState &m_appState;
    ThemePresetModel m_presetModel;
    std::vector<ThemePresetModel::Preset> m_presets;
    juce::OwnedArray<juce::TextButton> m_quickPresetButtons;
    juce::String m_activePresetName;
    juce::ValueTree m_referenceTheme;
    int m_selectedColourIndex = 0;
    bool m_updatingState = false;
    bool m_dragActive = false;
    juce::String m_pendingOverwriteName;

    ThemePresetBrowser m_presetBrowser;
    juce::TextButton m_saveButton{"Save"};
    juce::TextButton m_saveAsButton{"Save As"};
    juce::Label m_statusLabel;
    juce::Label m_saveNameLabel;
    juce::TextEditor m_saveNameEditor;
    juce::TextButton m_saveNameButton{"Save"};
    juce::TextButton m_cancelSaveButton{"Cancel"};
    InlineColourEditor m_colourEditor;

    juce::Rectangle<int> m_presetTitleBounds;
    juce::Rectangle<int> m_dropHintBounds;
    juce::Rectangle<int> m_colourTitleBounds;
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> m_groupBounds;
    std::vector<juce::Rectangle<int>> m_colourRowBounds;
    std::function<void()> m_onPreferredHeightChanged;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeSettingsComponent)
};
