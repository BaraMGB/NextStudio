#include "PluginScanner.h"


PluginScanner::PluginScanner(tracktion::Engine &en, juce::AudioPluginFormat &format, const juce::StringArray &filesOrIdentifiers, juce::PropertiesFile *properties, bool allowPluginsWhichRequireAsynchronousInstantiation, int threads, const juce::String &title, const juce::String &text)
    : m_engine(en),
      m_formatToScan (format),
      m_filesOrIdentifiersToScan (filesOrIdentifiers),
      m_propertiesToUse (properties),
      m_pathChooserWindow (TRANS ("Select folders to scan..."), juce::String(), juce::MessageBoxIconType::NoIcon),
      m_progressWindow (title, text, juce::MessageBoxIconType::NoIcon),
      m_numThreads (threads),
      m_allowAsync (allowPluginsWhichRequireAsynchronousInstantiation)
{
    const auto blacklisted = m_engine.getPluginManager().knownPluginList.getBlacklistedFiles();
    m_initiallyBlacklistedFiles = std::set<juce::String> (blacklisted.begin(), blacklisted.end());

    juce::FileSearchPath path (m_formatToScan.getDefaultLocationsToSearch());

    // You need to use at least one thread when scanning plug-ins asynchronously
    jassert (! allowAsync || (numThreads > 0));

    // If the filesOrIdentifiersToScan argument isn't empty, we should only scan these
    // If the path is empty, then paths aren't used for this format.
    if (m_filesOrIdentifiersToScan.isEmpty() && path.getNumPaths() > 0)
    {
#if ! JUCE_IOS
        if (m_propertiesToUse != nullptr)
            path = getLastSearchPath (*m_propertiesToUse, m_formatToScan);
#endif

        m_pathList.setSize (500, 300);
        m_pathList.setPath (path);

        m_pathChooserWindow.addCustomComponent (&m_pathList);
        m_pathChooserWindow.addButton (TRANS ("Scan"),   1, juce::KeyPress (juce::KeyPress::returnKey));
        m_pathChooserWindow.addButton (TRANS ("Cancel"), 0, juce::KeyPress (juce::KeyPress::escapeKey));

        m_pathChooserWindow.enterModalState (true, juce::ModalCallbackFunction::forComponent (startScanCallback, &m_pathChooserWindow, this), false);
    }
    else
    {
        startScan();
    }
}

PluginScanner::~PluginScanner()
{
    if (m_threadPool != nullptr)
    {
        m_threadPool->removeAllJobs (true, 60000);
        m_threadPool.reset();
    }
}

juce::FileSearchPath PluginScanner::getLastSearchPath(juce::PropertiesFile &properties, juce::AudioPluginFormat &format)
{
    auto key = "lastPluginScanPath_" + format.getName();

    if (properties.containsKey (key) && properties.getValue (key, {}).trim().isEmpty())
        properties.removeValue (key);

    return juce::FileSearchPath (properties.getValue (key, format.getDefaultLocationsToSearch().toString()));
}

void PluginScanner::setLastSearchPath(juce::PropertiesFile &properties, juce::AudioPluginFormat &format, const juce::FileSearchPath &newPath)
{
    auto key = "lastPluginScanPath_" + format.getName();

    if (newPath.getNumPaths() == 0)
        properties.removeValue (key);
    else
        properties.setValue (key, newPath.toString());
}

void PluginScanner::startScanCallback(int result, juce::AlertWindow *alert, PluginScanner *scanner)
{
    if (alert != nullptr && scanner != nullptr)
    {
        if (result != 0)
            scanner->warnUserAboutStupidPaths();
        else
            scanner->finishedScan();
    }
}

void PluginScanner::warnUserAboutStupidPaths()
{
    for (int i = 0; i < m_pathList.getPath().getNumPaths(); ++i)
    {
        auto f = m_pathList.getPath().getRawString (i);

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
            m_messageBox = juce::AlertWindow::showScopedAsync (options, [this] (int result)
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

bool PluginScanner::isStupidPath(const juce::File &f)
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

void PluginScanner::startScan()
{
    m_pathChooserWindow.setVisible (false);

    m_dirScanner.reset (new juce::PluginDirectoryScanner (m_engine.getPluginManager().knownPluginList, m_formatToScan, m_pathList.getPath(),
                                                       true, m_engine.getTemporaryFileManager()
                                                       .getTempFile ("PluginScanDeadMansPedal"), m_allowAsync));

    if (! m_filesOrIdentifiersToScan.isEmpty())
    {
        m_dirScanner->setFilesOrIdentifiersToScan (m_filesOrIdentifiersToScan);
    }
    else if (m_propertiesToUse != nullptr)
    {
        GUIHelpers::log("setLastSearchPath");
        setLastSearchPath (*m_propertiesToUse, m_formatToScan, m_pathList.getPath());
        m_propertiesToUse->saveIfNeeded();
    }

    m_progressWindow.addButton (TRANS ("Cancel"), 0, juce::KeyPress (juce::KeyPress::escapeKey));
    m_progressWindow.addProgressBarComponent (m_progress);
    m_progressWindow.enterModalState();

    if (m_numThreads > 0)
    {
        m_threadPool.reset (new juce::ThreadPool (m_numThreads));

        for (int i = m_numThreads; --i >= 0;)
            m_threadPool->addJob (new ScanJob (*this), true);
    }

    startTimer (20);
}

void PluginScanner::finishedScan()
{
    sendChangeMessage();
}

void PluginScanner::timerCallback()
{
    if (m_timerReentrancyCheck)
        return;

    m_progress = m_dirScanner->getProgress();

    if (m_threadPool == nullptr)
    {
        const juce::ScopedValueSetter<bool> setter (m_timerReentrancyCheck, true);

        if (doNextScan())
            startTimer (20);
    }

    if (! m_progressWindow.isCurrentlyModal())
        m_finished = true;

    if (m_finished)
        finishedScan();
    else
        m_progressWindow.setMessage (TRANS ("Testing") + ":\n\n" + m_pluginBeingScanned);
}

bool PluginScanner::doNextScan()
{
    if (m_dirScanner->scanNextFile (true, m_pluginBeingScanned))
        return true;

    m_finished = true;
    return false;
}
