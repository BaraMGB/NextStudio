#include "InstrumentEffectChooser.h"
#include "ApplicationViewState.h"
#include "PluginMenu.h"
#include "SearchFieldComponent.h"
#include "Utilities.h"

InstrumentEffectListModel::InstrumentEffectListModel(tracktion::Engine &engine, bool isInstrumentList, ApplicationViewState& appState) 
    : m_engine(engine), m_knownPlugins(engine.getPluginManager().knownPluginList)
    , m_isInstrumentList(isInstrumentList)
    , m_appState(appState)
{
    updatePluginLists();
}

void InstrumentEffectListModel::updatePluginLists()
{
    m_instruments.clear();
    m_effects.clear();
    

    for (auto& desc : m_knownPlugins.getTypes())
    {
        if (desc.isInstrument)
            m_instruments.add(desc);
        else
            m_effects.add(desc);
    }


    for (auto& desc : EngineHelpers::getInternalPlugins())
    {
        if (desc.isInstrument)
            m_instruments.add(desc);
        else
            m_effects.add(desc);
    }

    filterList();

    if (std::get<column>(m_order) == nameCol)
    {
        EngineHelpers::sortByName(m_instruments, std::get<bool>(m_order));
        EngineHelpers::sortByName(m_effects, std::get<bool>(m_order));
    }
    else if (std::get<column>(m_order) == typeCol)
    {
        EngineHelpers::sortByFormatName(m_instruments, std::get<bool>(m_order));
        EngineHelpers::sortByFormatName(m_effects, std::get<bool>(m_order));
    }

    sendChangeMessage();
}

void InstrumentEffectListModel::paintRowBackground(juce::Graphics &g, int row, int width, int height, bool rowIsSelected)
{
    if (row < 0 || row >= getNumRows())
        return;
    auto bgColour = row % 2 == 0 ? m_appState.getMenuBackgroundColour() : m_appState.getMenuBackgroundColour().brighter(0.05f);
    juce::Rectangle<int> bounds(0, 0, width, height);
    g.setColour(bgColour);
    g.fillRect(bounds);

    if (rowIsSelected)
    {
        g.setColour(juce::Colour(0xff555555));
        g.fillRect(bounds);
    }
}

void InstrumentEffectListModel::paintCell(juce::Graphics &g, int row, int col, int width, int height, bool rowIsSelected)
{
    if (row < 0 || row >= getNumRows())
        return;

    juce::PluginDescription desc = m_isInstrumentList ? m_instruments[row] : m_effects[row];
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
            else if (desc.pluginFormatName == getInternalPluginFormatName())
                icon = GUIHelpers::getDrawableFromSvg(BinaryData::INTIcon_svg, juce::Colours::gold ).release();

            if (icon != nullptr)
            {
                auto padding = 5.f;
                icon->setTransformToFit({static_cast<float>(width)/4 - padding, padding, (static_cast<float>(width)/2.f) + padding, static_cast<float>(height - (padding*2))}, juce::RectanglePlacement::centred);
                icon->draw(g, 1.0f);
                delete icon;
            }

        }
        else
        {
            juce::String preTerm, postTerm;
            int termStartIndex = text.indexOfIgnoreCase(m_searchTerm);
            juce::String searchTerm = text.substring(termStartIndex, termStartIndex + m_searchTerm.length());

            if (termStartIndex != -1 && m_searchTerm.length() > 0)
            {
                preTerm = text.substring(0, termStartIndex);
                postTerm = text.substring(termStartIndex + m_searchTerm.length());
                auto colour = rowIsSelected ? juce::Colours::black : juce::Colours::darkgrey;
                
                g.setColour(colour);
                g.setFont(juce::Font((float) height * 0.7f, juce::Font::bold));
                g.drawFittedText(preTerm, 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 0.9f);

                int preTermWidth = g.getCurrentFont().getStringWidth(preTerm);

                g.setColour(juce::Colours::coral);
                g.drawFittedText(searchTerm, 4 + preTermWidth, 0, width - 6 - preTermWidth, height, juce::Justification::centredLeft, 1, 0.9f);

                int termWidth = g.getCurrentFont().getStringWidth(searchTerm);

                g.setColour(colour);
                g.drawFittedText(postTerm, 4 + preTermWidth + termWidth, 0, width - 6 - preTermWidth - termWidth, height, juce::Justification::centredLeft, 1, 0.9f);
            }
            else
            {
                auto colour = rowIsSelected ? juce::Colours::black : juce::Colours::lightgrey;

                g.setColour(colour);
                g.setFont(juce::Font((float) height * 0.7f, juce::Font::bold));
                g.drawFittedText(text, 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 0.9f);
            }
        }
    }

    g.setColour(juce::Colours::lightgrey.withAlpha(0.3f));
    g.drawHorizontalLine(height - 1, 0, width);
    g.drawVerticalLine(width - 1, 0, height);
}

int InstrumentEffectListModel::getNumRows()
{
    return m_isInstrumentList ? m_instruments.size() : m_effects.size();
     
}

void InstrumentEffectListModel::sortOrderChanged(int newSortColumnId, bool isForwards)
{
    auto& desc = m_isInstrumentList ? m_instruments : m_effects;
    switch (newSortColumnId)
    {
    case typeCol:
            EngineHelpers::sortByFormatName(desc, isForwards);
            m_order = {typeCol, isForwards};
            sendChangeMessage()
            ;break;

    case nameCol:
            EngineHelpers::sortByName(desc, isForwards)
            ; m_order = {nameCol, isForwards};
            sendChangeMessage();
            break;

    default:
            jassertfalse;
            break;
    }
}

// ----------------------------------------------------------------------------------------------

InstrumentEffectTable::InstrumentEffectTable(tracktion::Engine &engine, InstrumentEffectListModel& model, ApplicationViewState& appState)
    : m_engine(engine)
    , m_model(model)
    , m_appState(appState)
{
    setColour (juce::TableListBox::ColourIds::backgroundColourId
               , m_appState.getBackgroundColour());
}

    juce::var InstrumentEffectTable::getDragSourceDescription(const juce::SparseSet<int> &)
{
    return {"Instrument or Effect"};
}

tracktion::Plugin::Ptr InstrumentEffectTable::getSelectedPlugin(tracktion::Edit &edit)
{
    auto selectedRow = getLastRowSelected();

    if (selectedRow < 0)
        return nullptr;

    auto & list = m_model.getPluginList();

    if (selectedRow < list.size())
    {
        if (list[selectedRow].pluginFormatName == getInternalPluginFormatName())
            return edit.getPluginCache().createNewPlugin(list[selectedRow].category, list[selectedRow]);
        else
            return edit.getPluginCache().createNewPlugin(te::ExternalPlugin::xmlTypeName, list[selectedRow]);
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------------

const auto formatWidth = 60;
InstrumentEffectChooser::InstrumentEffectChooser(tracktion::Engine &engine, bool isInstrumentList, ApplicationViewState& appState)
    : m_engine(engine)
    , m_isInstrumentList(isInstrumentList)
    , m_model(engine, isInstrumentList,appState)
    , m_listbox(engine, m_model,appState)
    , m_searchField(appState)
    , m_appState(appState)
{
    addAndMakeVisible(m_listbox);
    m_model.addChangeListener(this);
    m_listbox.setModel(&m_model);
    m_listbox.setRowHeight (20);
    juce::TableHeaderComponent& header = m_listbox.getHeader();
    header.setColour(juce::TableHeaderComponent::ColourIds::backgroundColourId, juce::Colour(0xff171717));
    header.setColour(juce::TableHeaderComponent::ColourIds::textColourId, juce::Colour(0xffffffff));
    header.setColour(juce::TableHeaderComponent::ColourIds::outlineColourId, juce::Colour(0xff888888));
    header.setColour(juce::TableHeaderComponent::ColourIds::highlightColourId, juce::Colour(0xff555555));

    header.addColumn (TRANS ("Format"), 1, formatWidth, formatWidth, formatWidth, juce::TableHeaderComponent::notResizable);
    header.addColumn (TRANS ("Name"), 2, getWidth() - formatWidth + 1, 80,30000, juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortedForwards | juce::TableHeaderComponent::notResizable);

    addAndMakeVisible(m_searchField);
    m_searchField.addChangeListener(this);
}

void InstrumentEffectChooser::resized()
{
    auto area = getLocalBounds();
    auto searchField = area.removeFromBottom(30);

    m_listbox.setBounds(area);
    m_searchField.setBounds(searchField);

    juce::TableHeaderComponent& header = m_listbox.getHeader();
    header.setColumnWidth(InstrumentEffectListModel::nameCol, getWidth() - formatWidth + 1);
}

void InstrumentEffectChooser::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (auto searchbox = dynamic_cast<SearchFieldComponent*>(source))
    {
        m_model.changeSearchTerm(searchbox->getText()) ;
        repaint();
    }

       
    m_listbox.updateContent();
    getParentComponent()->resized();
}
