
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
#include "EditViewState.h"
#include "ApplicationViewState.h"
#include "Utilities.h"

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
    juce::KnownPluginList& getKnownPlugins(){return m_knownPlugins;}
private:
    juce::KnownPluginList& m_knownPlugins;
    te::Engine&                           m_engine;
};

// --------------------------------------------------------------------------------------------------------------------------

class PluginListbox : public juce::TableListBox
{
public:
    PluginListbox(te::Engine& engine);

    juce::var getDragSourceDescription(const juce::SparseSet<int>& /*rowsToDescribe*/) override;
    te::Plugin::Ptr getSelectedPlugin(te::Edit& edit);
private:
    te::Engine&                           m_engine;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListbox)
};

// --------------------------------------------------------------------------------------------

class Scanner    : private juce::Timer
                 , public juce::ChangeBroadcaster
{
public:
    Scanner (te::Engine& engine, juce::AudioPluginFormat& format, const juce::StringArray& filesOrIdentifiers,
             juce::PropertiesFile* properties, bool allowPluginsWhichRequireAsynchronousInstantiation, int threads,
             const juce::String& title, const juce::String& text)
        : m_engine(engine),
          formatToScan (format),
          filesOrIdentifiersToScan (filesOrIdentifiers),
          propertiesToUse (properties),
          pathChooserWindow (TRANS ("Select folders to scan..."), juce::String(), juce::MessageBoxIconType::NoIcon),
          progressWindow (title, text, juce::MessageBoxIconType::NoIcon),
          numThreads (threads),
          allowAsync (allowPluginsWhichRequireAsynchronousInstantiation)
    {
        const auto blacklisted = m_engine.getPluginManager().knownPluginList.getBlacklistedFiles();
        initiallyBlacklistedFiles = std::set<juce::String> (blacklisted.begin(), blacklisted.end());

        juce::FileSearchPath path (formatToScan.getDefaultLocationsToSearch());

        // You need to use at least one thread when scanning plug-ins asynchronously
        jassert (! allowAsync || (numThreads > 0));

        // If the filesOrIdentifiersToScan argument isn't empty, we should only scan these
        // If the path is empty, then paths aren't used for this format.
        if (filesOrIdentifiersToScan.isEmpty() && path.getNumPaths() > 0)
        {
           #if ! JUCE_IOS
            if (propertiesToUse != nullptr)
                path = getLastSearchPath (*propertiesToUse, formatToScan);
           #endif

            pathList.setSize (500, 300);
            pathList.setPath (path);

            pathChooserWindow.addCustomComponent (&pathList);
            pathChooserWindow.addButton (TRANS ("Scan"),   1, juce::KeyPress (juce::KeyPress::returnKey));
            pathChooserWindow.addButton (TRANS ("Cancel"), 0, juce::KeyPress (juce::KeyPress::escapeKey));

            pathChooserWindow.enterModalState (true,
                                               juce::ModalCallbackFunction::forComponent (startScanCallback,
                                                                                    &pathChooserWindow, this),
                                               false);
        }
        else
        {
            startScan();
        }
    }

    ~Scanner() override
    {
        if (pool != nullptr)
        {
            pool->removeAllJobs (true, 60000);
            pool.reset();
        }
    }

juce::FileSearchPath getLastSearchPath (juce::PropertiesFile& properties, juce::AudioPluginFormat& format)
{
    auto key = "lastPluginScanPath_" + format.getName();

    if (properties.containsKey (key) && properties.getValue (key, {}).trim().isEmpty())
        properties.removeValue (key);

    return juce::FileSearchPath (properties.getValue (key, format.getDefaultLocationsToSearch().toString()));
}

void setLastSearchPath (juce::PropertiesFile& properties, juce::AudioPluginFormat& format,
                                             const juce::FileSearchPath& newPath)
{
    auto key = "lastPluginScanPath_" + format.getName();

    if (newPath.getNumPaths() == 0)
        properties.removeValue (key);
    else
        properties.setValue (key, newPath.toString());
}
std::set<juce::String> initiallyBlacklistedFiles;
std::unique_ptr<juce::PluginDirectoryScanner> scanner;
private:
te::Engine& m_engine;
juce::AudioPluginFormat& formatToScan;
juce::StringArray filesOrIdentifiersToScan;
juce::PropertiesFile* propertiesToUse;
juce::AlertWindow pathChooserWindow, progressWindow;
juce::FileSearchPathListComponent pathList;
juce::String pluginBeingScanned;
    double progress = 0;
    const int numThreads;
    bool allowAsync, timerReentrancyCheck = false;
    std::atomic<bool> finished { false };
    std::unique_ptr<juce::ThreadPool> pool;
juce::ScopedMessageBox messageBox;

    static void startScanCallback (int result, juce::AlertWindow* alert, Scanner* scanner)
    {
        if (alert != nullptr && scanner != nullptr)
        {
            if (result != 0)
                scanner->warnUserAboutStupidPaths();
            else
                scanner->finishedScan();
        }
    }

    // Try to dissuade people from to scanning their entire C: drive, or other system folders.
    void warnUserAboutStupidPaths()
    {
        for (int i = 0; i < pathList.getPath().getNumPaths(); ++i)
        {
            auto f = pathList.getPath().getRawString (i);

            if (juce::File::isAbsolutePath (f) && isStupidPath (juce::File (f)))
            {
                auto options = juce::MessageBoxOptions::makeOptionsOkCancel (juce::MessageBoxIconType::WarningIcon,
                                                                       TRANS ("Plugin Scanning"),
                                                                       TRANS ("If you choose to scan folders that contain non-plugin files, "
                                                                              "then scanning may take a long time, and can cause crashes when "
                                                                              "attempting to load unsuitable files.")
                                                                         + juce::newLine
                                                                         + TRANS ("Are you sure you want to scan the folder \"XYZ\"?")
                                                                            .replace ("XYZ", f),
                                                                       TRANS ("Scan"));
                messageBox = juce::AlertWindow::showScopedAsync (options, [this] (int result)
                {
                    if (result != 0)
                        startScan();
                    else
                        finishedScan();
                });

                return;
            }
        }

        startScan();
    }

    static bool isStupidPath (const juce::File& f)
    {
    juce::Array<juce::File> roots;
    juce::File::findFileSystemRoots (roots);

        if (roots.contains (f))
            return true;

    juce::File::SpecialLocationType pathsThatWouldBeStupidToScan[]
            = { juce::File::globalApplicationsDirectory,
                juce::File::userHomeDirectory,
                juce::File::userDocumentsDirectory,
                juce::File::userDesktopDirectory,
                juce::File::tempDirectory,
                juce::File::userMusicDirectory,
                juce::File::userMoviesDirectory,
                juce::File::userPicturesDirectory };

        for (auto location : pathsThatWouldBeStupidToScan)
        {
            auto sillyFolder = juce::File::getSpecialLocation (location);

            if (f == sillyFolder || sillyFolder.isAChildOf (f))
                return true;
        }

        return false;
    }

    void startScan()
    {
        pathChooserWindow.setVisible (false);

        scanner.reset (new juce::PluginDirectoryScanner (m_engine.getPluginManager().knownPluginList, formatToScan, pathList.getPath(),
                                                   true, m_engine.getTemporaryFileManager()
                                                     .getTempFile ("PluginScanDeadMansPedal"), allowAsync));

        if (! filesOrIdentifiersToScan.isEmpty())
        {
            scanner->setFilesOrIdentifiersToScan (filesOrIdentifiersToScan);
        }
        else if (propertiesToUse != nullptr)
        {
            GUIHelpers::log("setLastSearchPath");
            setLastSearchPath (*propertiesToUse, formatToScan, pathList.getPath());
            propertiesToUse->saveIfNeeded();
        }

        progressWindow.addButton (TRANS ("Cancel"), 0, juce::KeyPress (juce::KeyPress::escapeKey));
        progressWindow.addProgressBarComponent (progress);
        progressWindow.enterModalState();

        if (numThreads > 0)
        {
            pool.reset (new juce::ThreadPool (numThreads));

            for (int i = numThreads; --i >= 0;)
                pool->addJob (new ScanJob (*this), true);
        }

        startTimer (20);
    }

    void finishedScan()
    {
        sendChangeMessage();
    }

    void timerCallback() override
    {
        if (timerReentrancyCheck)
            return;

        progress = scanner->getProgress();

        if (pool == nullptr)
        {
            const juce::ScopedValueSetter<bool> setter (timerReentrancyCheck, true);

            if (doNextScan())
                startTimer (20);
        }

        if (! progressWindow.isCurrentlyModal())
            finished = true;

        if (finished)
            finishedScan();
        else
            progressWindow.setMessage (TRANS ("Testing") + ":\n\n" + pluginBeingScanned);
    }

    bool doNextScan()
    {
        if (scanner->scanNextFile (true, pluginBeingScanned))
            return true;

        finished = true;
        return false;
    }

    struct ScanJob  : public juce::ThreadPoolJob
    {
        ScanJob (Scanner& s)  : ThreadPoolJob ("pluginscan"), scanner (s) {}

        JobStatus runJob()
        {
            while (scanner.doNextScan() && ! shouldExit())
            {}

            return jobHasFinished;
        }

        Scanner& scanner;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScanJob)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Scanner)
};

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
    void scanFinished (const juce::StringArray& failedFiles,
                                        const std::vector<juce::String>& newBlacklistedFiles)
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
    juce::PopupMenu createOptionsMenu();
    
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
    std::unique_ptr<Scanner> currentScanner;
    te::Engine&                           m_engine;
    juce::TextButton                     m_setupButton;
    juce::ScopedMessageBox m_messageBox;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginBrowser)
};
