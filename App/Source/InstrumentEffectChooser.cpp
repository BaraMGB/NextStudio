#include "InstrumentEffectChooser.h"
#include "PluginMenu.h"
#include "SearchFieldComponent.h"
#include "Utilities.h"

InstrumentEffectListModel::InstrumentEffectListModel(tracktion::Engine &engine, bool isInstrumentList) 
    : m_engine(engine), m_knownPlugins(engine.getPluginManager().knownPluginList)
    , m_isInstrumentList(isInstrumentList)
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
}

void InstrumentEffectListModel::paintRowBackground(juce::Graphics &g, int row, int width, int height, bool rowIsSelected)
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
            g.setColour(juce::Colours::lightgrey);
            g.setFont(juce::Font((float) height * 0.7f, juce::Font::bold));
            g.drawFittedText(text, 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 0.9f);
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

InstrumentEffectTable::InstrumentEffectTable(tracktion::Engine &engine, InstrumentEffectListModel& model)
    : m_engine(engine)
    , m_model(model)
{
    setColour (juce::TableListBox::ColourIds::backgroundColourId
               , juce::Colour(0xff171717));
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
InstrumentEffectChooser::InstrumentEffectChooser(tracktion::Engine &engine, bool isInstrumentList)
    : m_engine(engine)
    , m_isInstrumentList(isInstrumentList)
    , m_model(engine, isInstrumentList)
    , m_listbox(engine, m_model)
{
    addAndMakeVisible(m_listbox);
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
    auto searchField = area.removeFromTop(30);

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
    }

       
    m_listbox.updateContent();
    getParentComponent()->resized();
}
