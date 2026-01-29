
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

#include "../JuceLibraryCode/JuceHeader.h"
#include "ApplicationViewState.h"
#include "NextLookAndFeel.h"
#include "Utilities.h"

namespace te = tracktion_engine;
#include "PluginScanner.h"

class PluginListBoxModel : public juce::TableListBoxModel
{
public:
    enum
    {
        typeCol = 1,
        nameCol = 2,
        categoryCol = 3,
        manufacturerCol = 4,
        descCol = 5
    };

    PluginListBoxModel(te::Engine& engine, ApplicationViewState& appState);

    void paintRowBackground( juce::Graphics &g, int row,int width, int height, bool rowIsSelected) override;
    void paintCell (juce::Graphics& g, int row, int col, int width, int height, bool rowIsSelected) override;

    void sortOrderChanged (int newSortColumnId, bool isForwards) override;
    static juce::String getPluginDescription (const juce::PluginDescription& desc);
    int getNumRows() override {return m_knownPlugins.getTypes().size ();}

    juce::var getDragSourceDescription(const juce::SparseSet<int>& /*rowsToDescribe*/) override;
private:
    juce::KnownPluginList& m_knownPlugins;
    te::Engine& m_engine;
    ApplicationViewState& m_appState;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListBoxModel)
};
//----------------------------------------------------------------------------------------

class PluginListbox : public juce::TableListBox
{
public:
    PluginListbox(te::Engine& engine);

    te::Plugin::Ptr getSelectedPlugin(te::Edit& edit);

private:
    te::Engine& m_engine;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListbox)
};

// --------------------------------------------------------------------------------------------

class PluginSettings : public juce::Component 
                     , public juce::ChangeListener
{
public:
    PluginSettings(te::Engine& engine, ApplicationViewState& appState);
    ~PluginSettings() override;

    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    void scanFor (juce::AudioPluginFormat&);
    void scanFor (juce::AudioPluginFormat&, const juce::StringArray& filesOrIdentifiersToScan);
    juce::PopupMenu createOptionsMenu();
    
    void scanFinished (const juce::StringArray& failedFiles, const std::vector<juce::String>& newBlacklistedFiles);
    void removeSelectedPlugins();
    void removePluginItem (int index);
    
    void removeMissingPlugins();

private:
    PluginListBoxModel m_model;
    PluginListbox m_listbox;
    juce::PropertiesFile* m_propertiesToUse;
    juce::String m_dialogTitle, m_dialogText;
    bool m_allowAsync;
    int m_numThreads;
    std::unique_ptr<PluginScanner> currentScanner;
    te::Engine&                           m_engine;
    juce::TextButton                     m_setupButton;
    juce::ScopedMessageBox m_messageBox;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginSettings)
};



