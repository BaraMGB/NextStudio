/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "ThemePresetModel.h"

#include <functional>
#include <vector>

class ThemePresetBrowser
    : public juce::Component
    , private juce::ListBoxModel
{
public:
    ThemePresetBrowser();
    ~ThemePresetBrowser() override;

    void setPresets(const std::vector<ThemePresetModel::Preset> &presets);
    void setActivePreset(const juce::String &name);
    void setThemeColours(juce::Colour background, juce::Colour text, juce::Colour border, juce::Colour accent);

    int getPreferredHeight() const noexcept { return 194; }

    void paint(juce::Graphics &g) override;
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override;

    std::function<void(const juce::String &)> onPresetSelected;

private:
    class WheelPassingTextEditor : public juce::TextEditor
    {
    public:
        void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override
        {
            juce::Component::mouseWheelMove(event, wheel);
        }
    };

    struct Row
    {
        int presetIndex = -1;
        juce::String heading;
    };

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent &) override;
    void returnKeyPressed(int lastRowSelected) override;
    juce::String getNameForRow(int rowNumber) override;

    void rebuildRows();
    void activateRow(int row);
    int findRowForPreset(const juce::String &name) const;

    std::vector<ThemePresetModel::Preset> m_presets;
    std::vector<Row> m_rows;
    juce::String m_activePreset;
    juce::Colour m_background{0xff202020};
    juce::Colour m_text{juce::Colours::white};
    juce::Colour m_border{juce::Colours::black};
    juce::Colour m_accent{juce::Colours::yellow};

    juce::Label m_titleLabel;
    WheelPassingTextEditor m_searchEditor;
    juce::ListBox m_list{"Theme presets", this};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemePresetBrowser)
};
