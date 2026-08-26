/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class ApplicationViewState;

class RackPanelToggleButton final : public juce::Button
{
public:
    RackPanelToggleButton(juce::String title, ApplicationViewState &appState);

    void setAppearance(bool collapsed, juce::Colour headerColour);
    void paintButton(juce::Graphics &g, bool isMouseOverButton, bool isButtonDown) override;

private:
    juce::String m_title;
    ApplicationViewState &m_appState;
    juce::Colour m_headerColour;
    std::unique_ptr<juce::Drawable> m_collapsedIcon;
    std::unique_ptr<juce::Drawable> m_expandedIcon;
    bool m_collapsed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RackPanelToggleButton)
};
