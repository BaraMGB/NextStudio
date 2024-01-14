#include "PluginBrowser.h"
#include "Utilities.h"

PluginListBoxModel::PluginListBoxModel(tracktion::Engine &engine)
    : m_knownPlugins(engine.getPluginManager().knownPluginList)
    , m_engine(engine)
{
}

void PluginListBoxModel::paintRowBackground(juce::Graphics &g, int row, int width, int height, bool rowIsSelected)
{
    if (row < 0|| row >= getNumRows())
        return;

    juce::Rectangle<int> bounds (0,0, width, height);
    g.setColour(juce::Colour(0xff171717));
    g.fillRect(bounds);

    if (rowIsSelected)
    {
        g.setColour(juce::Colour(0xff555555));
        g.fillRect(bounds);
    }
}

void PluginListBoxModel::paintCell(juce::Graphics &g, int row, int col, int width, int height, bool rowIsSelected)
{
    juce::String text;
    bool isBlacklisted = row >= m_knownPlugins.getNumTypes();

    if (isBlacklisted)
    {
        if (col == nameCol)
            text = m_knownPlugins.getBlacklistedFiles() [row - m_knownPlugins.getNumTypes()];
        else if (col == descCol)
            text = TRANS ("Deactivated after failing to initialise correctly");
    }
    else
    {
        auto desc = m_knownPlugins.getTypes()[row];
        auto type = m_knownPlugins.getTypes()[row];

        switch (col)
        {
            case typeCol:         text = desc.pluginFormatName; break;
            case nameCol:         text = desc.name; break;
            case categoryCol:     text = type.isInstrument ? "Klavier" : "EFFEKT"; break; //desc.category.isNotEmpty() ? desc.category : "-"; break;
            case manufacturerCol: text = desc.manufacturerName; break;
            case descCol:         text = getPluginDescription (desc); break;

            default: jassertfalse; break;
        }
    }
    
    if (text.isNotEmpty())
    {
        auto desc = m_knownPlugins.getTypes()[row];
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
            const auto defaultTextColour = juce::Colours::lightgrey;
            g.setColour (isBlacklisted ? juce::Colours::red
                                       : col == nameCol ? defaultTextColour
                                                        : defaultTextColour.interpolatedWith (juce::Colours::transparentBlack, 0.3f));
            g.setFont (juce::Font ((float) height * 0.7f, juce::Font::bold));
            g.drawFittedText (text, 4, 0, width - 6, height, juce::Justification::centredLeft, 1, 0.9f);
        }
    }
    g.setColour(juce::Colours::lightgrey.withAlpha(0.3f));
    g.drawHorizontalLine(height-1, 0, width);
    g.drawVerticalLine(width-1, 0, height);
}

void PluginListBoxModel::sortOrderChanged (int newSortColumnId, bool isForwards) 
{
    switch (newSortColumnId)
    {
        case typeCol:         m_knownPlugins.sort (juce::KnownPluginList::sortByFormat, isForwards); break;
        case nameCol:         m_knownPlugins.sort (juce::KnownPluginList::sortAlphabetically, isForwards); break;
        case categoryCol:     m_knownPlugins.sort (juce::KnownPluginList::sortByCategory, isForwards); break;
        case manufacturerCol: m_knownPlugins.sort (juce::KnownPluginList::sortByManufacturer, isForwards); break;
        case descCol:         break;

        default: jassertfalse; break;
    }
}
juce::String PluginListBoxModel::getPluginDescription(const juce::PluginDescription &desc)
{
    juce::StringArray items;

    if (desc.descriptiveName != desc.name)
        items.add (desc.descriptiveName);

    items.add (desc.version);

    items.removeEmptyStrings();
    return items.joinIntoString (" - ");
}

//------------------------------------------------------------------------------------------------

PluginListbox::PluginListbox(te::Engine& engine)
    : m_engine(engine)
{
    
    setColour (juce::TableListBox::ColourIds::backgroundColourId
                         , juce::Colour(0xff171717));

}

// ---------------------------------------------------------------------------------------------

juce::var PluginListbox::getDragSourceDescription(const juce::SparseSet<int> &)
{
    return {"PluginListEntry"};
}

tracktion::Plugin::Ptr PluginListbox::getSelectedPlugin(te::Edit& edit)
{
    auto selectedRow = getLastRowSelected();

    if (selectedRow < 0)
        return nullptr;

    auto types = m_engine.getPluginManager().knownPluginList.getTypes();

    if (selectedRow < types.size())
        return edit.getPluginCache().createNewPlugin(te::ExternalPlugin::xmlTypeName, types[selectedRow]);

    return nullptr;
}

// ---------------------------------------------------------------------------------------------

PluginSettings::PluginSettings(te::Engine& engine) 
  : m_engine(engine)
  , m_model(engine)
  , m_listbox(engine)
{
    addAndMakeVisible(m_listbox);
    addAndMakeVisible(m_setupButton);
    juce::TableHeaderComponent& header = m_listbox.getHeader();

    header.addColumn (TRANS ("Format"), 1, 40, 40, 40, juce::TableHeaderComponent::notResizable);
    header.addColumn (TRANS ("Name"), 2, 200, 100, 700, juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortedForwards);
    header.addColumn (TRANS ("Category"), 3, 100, 100, 200);
    header.addColumn (TRANS ("Manufacturer"),4 , 200, 100, 300);
    header.addColumn (TRANS ("Description"), 5, 300, 100, 500, juce::TableHeaderComponent::notSortable);

    m_listbox.setModel(&m_model);
    m_listbox.setRowHeight (20);
    engine.getPluginManager().knownPluginList.addChangeListener(this);
    m_propertiesToUse = te::getApplicationSettings();

    m_setupButton.onClick = [this]
    {
        juce::PopupMenu m;
        m = createOptionsMenu();
        auto opt = juce::PopupMenu::Options().withTargetComponent(m_setupButton);
        m.showMenuAsync(opt);
    };
}

PluginSettings::~PluginSettings()
{
    m_engine.getPluginManager().knownPluginList.removeChangeListener(this);
}
void PluginSettings::resized()
{
    auto area = getLocalBounds();
    auto buttonBar = area.removeFromBottom(20);

    m_setupButton.setBounds(buttonBar.removeFromLeft(30));
    m_listbox.setBounds(area);
}

void PluginSettings::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (auto scanner = dynamic_cast<PluginScanner*>(source))
    {
        const auto blacklisted = m_engine.getPluginManager().knownPluginList.getBlacklistedFiles();
        std::set<juce::String> allBlacklistedFiles (blacklisted.begin(), blacklisted.end());

        std::vector<juce::String> newBlacklistedFiles;
        auto initiallyBlacklistedFiles = scanner->m_initiallyBlacklistedFiles;
        std::set_difference (allBlacklistedFiles.begin(), allBlacklistedFiles.end(),
                             initiallyBlacklistedFiles.begin(), initiallyBlacklistedFiles.end(),
                             std::back_inserter (newBlacklistedFiles));
        scanFinished (scanner->m_dirScanner != nullptr ? scanner->m_dirScanner->getFailedFiles() : juce::StringArray(),
                             newBlacklistedFiles);
    }
    else if (dynamic_cast<juce::KnownPluginList*>(source))
    {
        GUIHelpers::log("Liste changed");
        m_listbox.updateContent();
        getParentComponent()->resized();
    }
}

static bool canShowFolderForPlugin (juce::KnownPluginList& list, int index)
{
    return juce::File::createFileWithoutCheckingPath (list.getTypes()[index].fileOrIdentifier).exists();
}

static void showFolderForPlugin (juce::KnownPluginList& list, int index)
{
    if (canShowFolderForPlugin (list, index))
        juce::File (list.getTypes()[index].fileOrIdentifier).revealToUser();
}
juce::PopupMenu PluginSettings::createOptionsMenu()
{
    juce::PopupMenu menu;
    auto& list = m_engine.getPluginManager().knownPluginList;
    auto& formatManager = m_engine.getPluginManager().pluginFormatManager;
    auto& table = m_listbox;

    menu.addItem (juce::PopupMenu::Item (TRANS ("Clear list"))
                    .setAction ([this]
                    { 
                      auto& list = m_engine.getPluginManager().knownPluginList;
                      list.clear(); 
                    }));

    menu.addSeparator();

    for (auto format : formatManager.getFormats())
        if (format->canScanForPlugins())
            menu.addItem (juce::PopupMenu::Item ("Remove all " + format->getName() + " plug-ins")
                            .setEnabled (! list.getTypesForFormat (*format).isEmpty())
                            .setAction ([this, format]
                                        {
                                            auto& list = m_engine.getPluginManager().knownPluginList;
                                            for (auto& pd : list.getTypesForFormat (*format))
                                                list.removeType (pd);
                                        }));

    menu.addSeparator();

    menu.addItem (juce::PopupMenu::Item (TRANS ("Remove selected plug-in from list"))
                    .setEnabled (table.getNumSelectedRows() > 0)
                    .setAction ([this] { removeSelectedPlugins(); }));

    menu.addItem (juce::PopupMenu::Item (TRANS ("Remove any plug-ins whose files no longer exist"))
                    .setAction ([this] { removeMissingPlugins(); }));

    menu.addSeparator();

    auto selectedRow = table.getSelectedRow();

    menu.addItem (juce::PopupMenu::Item (TRANS ("Show folder containing selected plug-in"))
                    .setEnabled (canShowFolderForPlugin (list, selectedRow))
                    .setAction ([this, selectedRow] 
                                {
                                    auto& list = m_engine.getPluginManager().knownPluginList;
                                    showFolderForPlugin (list, selectedRow); 
                                }));

    menu.addSeparator();

    for (auto format : formatManager.getFormats())
        if (format->canScanForPlugins())
            menu.addItem (juce::PopupMenu::Item ("Scan for new or updated " + format->getName() + " plug-ins")
                            .setAction ([this, format]  { scanFor (*format); }));

    return menu;
}
void PluginSettings::scanFinished (const juce::StringArray& failedFiles, const std::vector<juce::String>& newBlacklistedFiles)
{
    juce::StringArray warnings;

    const auto addWarningText = [&warnings] (const auto& range, const auto& prefix)
    {
        if (range.size() == 0)
            return;

        juce::StringArray names;

        for (auto& f : range)
            names.add (juce::File::createFileWithoutCheckingPath (f).getFileName());

        warnings.add (prefix + ":\n\n" + names.joinIntoString (", "));
    };

    addWarningText (newBlacklistedFiles,  TRANS ("The following files encountered fatal errors during validation"));
    addWarningText (failedFiles,          TRANS ("The following files appeared to be plugin files, but failed to load correctly"));

    currentScanner.reset(); // mustn't delete this before using the failed files array

    if (! warnings.isEmpty())
    {
        auto options = juce::MessageBoxOptions::makeOptionsOk (juce::MessageBoxIconType::InfoIcon,
                                                         TRANS ("Scan complete"),
                                                         warnings.joinIntoString ("\n\n"));
        m_messageBox = juce::AlertWindow::showScopedAsync (options, nullptr);
    }
}
void PluginSettings::removeSelectedPlugins()
{
    auto selected = m_listbox.getSelectedRows();

    for (int i = m_listbox.getNumRows(); --i >= 0;)
        if (selected.contains (i))
            removePluginItem (i);
}

void PluginSettings::removePluginItem (int index)
{
    auto& list = m_engine.getPluginManager().knownPluginList;
    if (index < list.getNumTypes())
        list.removeType (list.getTypes()[index]);
    else
        list.removeFromBlacklist (list.getBlacklistedFiles() [index - list.getNumTypes()]);
}
void PluginSettings::removeMissingPlugins()
{
    auto& list = m_engine.getPluginManager().knownPluginList;
    auto types = list.getTypes();
    auto& formatManager = m_engine.getPluginManager().pluginFormatManager;

    for (int i = types.size(); --i >= 0;)
    {
        auto type = types.getUnchecked (i);

        if (! formatManager.doesPluginStillExist (type))
            list.removeType (type);
    }
}
void PluginSettings::scanFor (juce::AudioPluginFormat& format)
{
    scanFor (format, juce::StringArray());
}

void PluginSettings::scanFor (juce::AudioPluginFormat& format, const juce::StringArray& filesOrIdentifiersToScan)
{
    if (currentScanner != nullptr)
        currentScanner->removeAllChangeListeners();
    currentScanner.reset (new PluginScanner (m_engine, format, filesOrIdentifiersToScan, m_propertiesToUse, m_allowAsync, m_numThreads,
                                       m_dialogTitle.isNotEmpty() ? m_dialogTitle : TRANS ("Scanning for plug-ins..."),
                                       m_dialogText.isNotEmpty()  ? m_dialogText  : TRANS ("Searching for all possible plug-in files...")));
    currentScanner->addChangeListener(this);
}

