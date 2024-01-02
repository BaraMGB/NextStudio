
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

    PluginListBoxModel(te::Engine& engine);

    void paintRowBackground( juce::Graphics &g, int row,int width, int height, bool rowIsSelected) override;
    void paintCell (juce::Graphics& g, int row, int col, int width, int height, bool rowIsSelected) override;

    void sortOrderChanged (int newSortColumnId, bool isForwards) override;
    static juce::String getPluginDescription (const juce::PluginDescription& desc);
    int getNumRows() override {return m_knownPlugins.getTypes().size ();}

private:
    juce::KnownPluginList& m_knownPlugins;
    te::Engine& m_engine;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListBoxModel)
};
class InstrumentListBoxModel : public juce::TableListBoxModel
                             , public juce::ChangeBroadcaster
{
public:
    enum
    {
        typeCol = 1,
        nameCol = 2,
    };

    InstrumentListBoxModel(te::Engine& engine) : m_engine(engine), m_knownPlugins(engine.getPluginManager().knownPluginList)
    {
        updateInstrumentList();
    }

    void updateInstrumentList()
    {           
        m_instruments.clear();
        for (auto& desc : m_knownPlugins.getTypes())    
        {
            if (desc.isInstrument)
                m_instruments.add(desc);
        }
    }
    void paintRowBackground(juce::Graphics& g, int row, int width, int height, bool rowIsSelected) override
    {
        if (row < 0 || row >= getNumRows())
            return;

        juce::Rectangle<int> bounds(0, 0, width, height);
        g.setColour(juce::Colour(0xff171717));
        g.fillRect(bounds);

        if (rowIsSelected)
        {
            g.setColour(juce::Colour(0xff555555));
            g.fillRect(bounds);
        }
    }

    void paintCell(juce::Graphics& g, int row, int col, int width, int height, bool rowIsSelected) override
    {
        if (row < 0 || row >= getNumRows())
            return;

        juce::PluginDescription desc = m_instruments[row];
        juce::String text;
        bool isBlacklisted = m_knownPlugins.getBlacklistedFiles().contains(desc.fileOrIdentifier);

        if (isBlacklisted)
        {
            if (col == nameCol)
                text = desc.fileOrIdentifier;
            else if (col == typeCol)
                text = TRANS("Deactivated after failing to initialise correctly");
        }
        else
        {
            switch (col)
            {
                case typeCol:         text = desc.pluginFormatName; break;
                case nameCol:         text = desc.name; break;
                default: jassertfalse; break;
            }
        }

        if (text.isNotEmpty())
        {
            if (col == typeCol)
            {
                juce::Drawable* icon = nullptr;
                if (desc.pluginFormatName == "VST3")
                    icon = GUIHelpers::getDrawableFromSvg(BinaryData::vst3Icon_svg, juce::Colours::yellowgreen).release();
                else if (desc.pluginFormatName == "LADSPA")
                    icon = GUIHelpers::getDrawableFromSvg(BinaryData::ladspaIcon_svg, juce::Colours::coral).release();
                else if (desc.pluginFormatName == "LA2")
                    icon = GUIHelpers::getDrawableFromSvg(BinaryData::la2Icon_svg, juce::Colours::cornflowerblue).release();
                else if (desc.pluginFormatName == "AudioUnit")
                    icon = GUIHelpers::getDrawableFromSvg(BinaryData::AUIcon_svg, juce::Colours::wheat).release();

                if (icon != nullptr)
                {
                    icon->setTransformToFit({static_cast<float>(width)/4, 0.f, static_cast<float>(width)/2.f, static_cast<float>(height)}, juce::RectanglePlacement::centred);
                    icon->draw(g, 1.0f);
                    delete icon;
                }

            }
            else 
            {
                g.setColour(juce::Colours::lightgrey);
                g.setFont(juce::Font((float) height * 0.7f, juce::Font::bold));
                g.drawFittedText(text, 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 0.9f);
            }
        }

        g.setColour(juce::Colours::lightgrey.withAlpha(0.3f));
        g.drawHorizontalLine(height - 1, 0, width);
        g.drawVerticalLine(width - 1, 0, height);
    }

    int getNumRows() override
    {
        return m_instruments.size();
    }

    void sortOrderChanged (int newSortColumnId, bool isForwards) override 
    {
        switch (newSortColumnId)
        {
            case typeCol:         sortByFormatName(isForwards); break;
            case nameCol:         sortByName(isForwards); break;

            default: jassertfalse; break;
        }
    }
private:
    juce::KnownPluginList& m_knownPlugins;
    te::Engine& m_engine;
    juce::Array<juce::PluginDescription> m_instruments;


    struct CompareNameForward{
        static int compareElements (const juce::PluginDescription& first, 
                                              const juce::PluginDescription& second)
        {   
            return first.name.compareNatural(second.name);
        }
    };

    struct CompareNameBackwards{
        static int compareElements(const juce::PluginDescription& first, 
                                               const juce::PluginDescription& second)
        {
            return second.name.compareNatural(first.name);
        }
    };

    void sortByName(bool forward)
    {
        
        if (forward)
        {
            CompareNameForward cf;
            m_instruments.sort(cf);
        }
        else
        {
            CompareNameBackwards cb;
            m_instruments.sort(cb);
        }
    }

    struct CompareFormatForward{
        static int compareElements(const juce::PluginDescription& first,
                                                         const juce::PluginDescription& second)
        {
            return first.pluginFormatName.compareNatural(second.pluginFormatName);   
        }
    };

    struct CompareFormatBackward{
        static int compareElements(const juce::PluginDescription& first,
                                                          const juce::PluginDescription& second)
        {
            return second.pluginFormatName.compareNatural(first.pluginFormatName);
        }
    };

    void sortByFormatName(bool forward)
    {
        if (forward)
        {
            CompareFormatForward cf;
            m_instruments.sort(cf);
        }
        else
        {
            CompareFormatBackward cb;
            m_instruments.sort(cb);
        }
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentListBoxModel)
};
//----------------------------------------------------------------------------------------

class PluginListbox : public juce::TableListBox
{
public:
    PluginListbox(te::Engine& engine);

    juce::var getDragSourceDescription(const juce::SparseSet<int>& /*rowsToDescribe*/) override;
    te::Plugin::Ptr getSelectedPlugin(te::Edit& edit);

private:
    bool isInstrumentList()
    {
        auto model = getModel();
        return dynamic_cast<InstrumentListBoxModel*>(model) != nullptr;
    }
    te::Engine& m_engine;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListbox)
};

// --------------------------------------------------------------------------------------------

class PluginBrowser : public juce::Component 
                     , public juce::ChangeListener
{
public:
    PluginBrowser(te::Engine& engine);
    ~PluginBrowser() override;

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
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginBrowser)
};



class InstrumentBrowser : public juce::Component 
                     , public juce::ChangeListener
{
public:
    InstrumentBrowser(te::Engine& engine)
    : m_engine(engine)
    , m_model(engine)
    , m_listbox(engine)
    {
        addAndMakeVisible(m_listbox);
        m_listbox.setModel(&m_model);
        m_listbox.setRowHeight (20);
        juce::TableHeaderComponent& header = m_listbox.getHeader();

        header.addColumn (TRANS ("Format"), 1, 40, 40, 40, juce::TableHeaderComponent::notResizable);
        header.addColumn (TRANS ("Name"), 2, 200, 100, 700, juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortedForwards);
    }
    ~InstrumentBrowser() override
    {}

    void resized() override
    {
        m_listbox.setBounds(getLocalBounds());
    }
    void changeListenerCallback(juce::ChangeBroadcaster *source) override
    {
        m_listbox.updateContent();
        getParentComponent()->resized();
    }


private:
    InstrumentListBoxModel m_model;
    PluginListbox m_listbox;
    te::Engine&                           m_engine;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstrumentBrowser)
};
