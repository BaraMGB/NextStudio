#pragma once

#include "../JuceLibraryCode/JuceHeader.h"


namespace IDs
{
    #define DECLARE_ID(name)  const juce::Identifier name (#name);
    DECLARE_ID (AppSettings)
    DECLARE_ID (FileBrowser)
    DECLARE_ID (WindowState)
    DECLARE_ID (WorkDIR)
    DECLARE_ID (PresetDIR)
    DECLARE_ID (ClipsDIR)
    DECLARE_ID (SamplesDIR)
    DECLARE_ID (ProjectsDIR)
    DECLARE_ID (Favorites)
    DECLARE_ID (Path)
    DECLARE_ID (red)
    DECLARE_ID (green)
    DECLARE_ID (orange)
    DECLARE_ID (blue)
    DECLARE_ID (violet)
    DECLARE_ID (WindowX)
    DECLARE_ID (WindowY)
    DECLARE_ID (WindowWidth)
    DECLARE_ID (WindowHeight)
    #undef DECLARE_ID
}

enum class PresetTag
{
    Drums
    , Bass
    , Pad
    , Percussion
    , Strings
    , Drone
    , Ambient

};

struct RedFav
{
    RedFav(juce::ValueTree v)
        : state(v)
    {
        jassert(v.hasType(IDs::red));
        fullPath.referTo (state, IDs::Path, nullptr);
    }
    juce::File getFile()
    {
        return juce::File::createFileWithoutCheckingPath (fullPath);
    }
    void setFile(juce::File f)
    {
        fullPath = f.getFullPathName ();
    }
    juce::ValueTree state;
    juce::CachedValue<juce::String> fullPath;
};
class ApplicationViewState
{
public:
    ApplicationViewState()

    {
        auto settingsFile = juce::File::getSpecialLocation (
                    juce::File::userApplicationDataDirectory)
                    .getChildFile ("NextStudio/AppSettings.xml");
        settingsFile.create ();

        juce::XmlDocument xmlDoc (settingsFile);
        auto xmlToRead = xmlDoc.getDocumentElement ();
        if (xmlToRead)
        {
            m_applicationStateValueTree = juce::ValueTree::fromXml (*xmlToRead);
        }
        else
        {
            m_applicationStateValueTree = juce::ValueTree(IDs::AppSettings);
        }
        auto fileBrowser = m_applicationStateValueTree.getOrCreateChildWithName (IDs::FileBrowser, nullptr);

        m_workDir.referTo (fileBrowser, IDs::WorkDIR, nullptr
                            , juce::File::getSpecialLocation (
                                juce::File::userHomeDirectory)
                            .getChildFile ("NextStudio").getFullPathName ());
        m_presetDir.referTo (fileBrowser, IDs::PresetDIR, nullptr
                            , juce::File::getSpecialLocation (
                                juce::File::userHomeDirectory)
                            .getChildFile ("NextStudio/Presets").getFullPathName ());
        m_clipsDir.referTo (fileBrowser, IDs::ClipsDIR, nullptr
                            , juce::File::getSpecialLocation (
                                juce::File::userHomeDirectory)
                            .getChildFile ("NextStudio/Clips").getFullPathName ());
        m_samplesDir.referTo (fileBrowser, IDs::SamplesDIR, nullptr
                            , juce::File::getSpecialLocation (
                                juce::File::userHomeDirectory)
                            .getChildFile ("NextStudio/Samples").getFullPathName ());
        m_projectsDir.referTo (fileBrowser, IDs::ProjectsDIR, nullptr
                            , juce::File::getSpecialLocation (
                                juce::File::userHomeDirectory)
                            .getChildFile ("NextStudio/Projects").getFullPathName ());

        auto redVT = m_applicationStateValueTree.getOrCreateChildWithName (IDs::red, nullptr);
        for (auto i=0; i < redVT.getNumChildren (); i++)
        {
            m_red.add (new RedFav(redVT.getChild (i)));
        }

        auto windowState = m_applicationStateValueTree.getOrCreateChildWithName (IDs::WindowState, nullptr);

        m_windowXpos.referTo (windowState, IDs::WindowX, nullptr, 50);
        m_windowYpos.referTo (windowState, IDs::WindowY, nullptr, 50);
        m_windowWidth.referTo (windowState, IDs::WindowWidth, nullptr, 1600);
        m_windowHeight.referTo (windowState, IDs::WindowHeight, nullptr, 1000);


    }

    void setBounds(juce::Rectangle<int> bounds)
    {
        m_windowXpos = bounds.getX();
        m_windowYpos = bounds.getY();
        m_windowWidth = bounds.getWidth ();
        m_windowHeight = bounds.getHeight ();
    }

    void saveState()
    {
        auto redfav = m_applicationStateValueTree.getOrCreateChildWithName (IDs::red, nullptr);
        redfav.removeAllChildren (nullptr);
        for (auto &redit : m_red)
        {
            redfav.addChild (redit->state, -1, nullptr);
        }

        auto settingsFile = juce::File::getSpecialLocation (
                    juce::File::userApplicationDataDirectory)
                    .getChildFile ("NextStudio/AppSettings.xml");
        settingsFile.create ();
        auto xmlToWrite = m_applicationStateValueTree.createXml ();
        xmlToWrite->writeTo (settingsFile);
    }

    void addFileToFavorites(juce::File file)
    {
        for (auto fav : m_red)
        {
            if (fav->getFile () == file)
            {
                return;
            }
        }
        auto fav = new RedFav(juce::ValueTree (IDs::red));
        fav->setFile (file);
        m_red.add (fav);
        saveState ();
    }

    void removeFileFromFavorite(juce::File file)
    {
        for (auto fav : m_red)
        {
            if (fav->getFile () == file)
            {
                m_red.removeObject (fav);
                saveState ();
            }
        }
    }

    juce::ValueTree m_applicationStateValueTree;
    juce::OwnedArray<RedFav> m_red;

    juce::CachedValue<juce::String> m_workDir,
                                    m_presetDir,
                                    m_clipsDir,
                                    m_samplesDir,
                                    m_projectsDir;
    juce::CachedValue<int>          m_windowXpos,
                                    m_windowYpos,
                                    m_windowWidth,
                                    m_windowHeight;
};
