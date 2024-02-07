
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "ApplicationViewState.h"
#include "Utilities.h" 



class SearchFieldComponent : public juce::Component
                            , public juce::ChangeBroadcaster
{
public:

    SearchFieldComponent(ApplicationViewState& appState);

    void resized() override;
    void paint(juce::Graphics& g) override;
    juce::String getText();

private:

    juce::TextEditor m_searchField;
    juce::TextButton m_clearButton;
    juce::Label m_label;

    ApplicationViewState& m_appState;

};
