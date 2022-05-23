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
    DECLARE_ID (FavoriteTypes)
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
    DECLARE_ID (FolderTrackIndent)
    DECLARE_ID (ThemeState)
    DECLARE_ID (PrimeColour)
    DECLARE_ID (BackgroundColour)
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

struct Favorite
{
    Favorite(juce::Identifier tag, juce::ValueTree v)
        : m_tag (std::move(tag))
        , m_state(std::move(v))
    {
        fullPath.referTo (m_state, IDs::Path, nullptr);
    }
    [[nodiscard]] juce::File getFile() const
    {
        return juce::File::createFileWithoutCheckingPath (fullPath);
    }
    void setFile(const juce::File& f)
    {
        fullPath = f.getFullPathName ();
    }
    juce::Identifier m_tag;
    juce::ValueTree m_state;
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
        auto fileBrowser = m_applicationStateValueTree.getOrCreateChildWithName (
                    IDs::FileBrowser, nullptr);


        m_workDir.referTo (fileBrowser, IDs::WorkDIR, nullptr
                            , juce::File::getSpecialLocation (
                                juce::File::userHomeDirectory)
                            .getChildFile ("NextStudio/").getFullPathName ());
        fileBrowser.setProperty (IDs::WorkDIR,juce::var(m_workDir),nullptr);
        m_presetDir.referTo (fileBrowser, IDs::PresetDIR, nullptr
                            , juce::File::getSpecialLocation (
                                juce::File::userHomeDirectory)
                            .getChildFile ("NextStudio/Presets/").getFullPathName ());
        fileBrowser.setProperty (IDs::PresetDIR,juce::var(m_presetDir),nullptr);
        m_clipsDir.referTo (fileBrowser, IDs::ClipsDIR, nullptr
                            , juce::File::getSpecialLocation (
                                juce::File::userHomeDirectory)
                            .getChildFile ("NextStudio/Clips/").getFullPathName ());
        fileBrowser.setProperty (IDs::ClipsDIR,juce::var(m_clipsDir),nullptr);
        m_samplesDir.referTo (fileBrowser, IDs::SamplesDIR, nullptr
                            , juce::File::getSpecialLocation (
                                juce::File::userHomeDirectory)
                            .getChildFile ("NextStudio/Samples/").getFullPathName ());
        fileBrowser.setProperty (IDs::SamplesDIR,juce::var(m_samplesDir),nullptr);
        m_projectsDir.referTo (fileBrowser, IDs::ProjectsDIR, nullptr
                            , juce::File::getSpecialLocation (
                                juce::File::userHomeDirectory)
                            .getChildFile ("NextStudio/Projects/").getFullPathName ());
        fileBrowser.setProperty (IDs::ProjectsDIR,juce::var(m_projectsDir),nullptr);

        auto favorites = m_applicationStateValueTree
                .getOrCreateChildWithName (IDs::Favorites, nullptr);

        for (auto i=0; i < favorites.getNumChildren (); i++)
        {
            m_favorites.add (new Favorite(favorites.getChild (i).getType (), favorites.getChild (i)));
        }

        auto windowState = m_applicationStateValueTree
                .getOrCreateChildWithName (IDs::WindowState, nullptr);

        m_windowXpos.referTo (windowState, IDs::WindowX, nullptr, 50);
        m_windowYpos.referTo (windowState, IDs::WindowY, nullptr, 50);
        m_windowWidth.referTo (windowState, IDs::WindowWidth, nullptr, 1600);
        m_windowHeight.referTo (windowState, IDs::WindowHeight, nullptr, 1000);

        auto themeState = m_applicationStateValueTree
                .getOrCreateChildWithName(IDs::ThemeState, nullptr);
        m_folderTrackIndent.referTo (themeState, IDs::FolderTrackIndent, nullptr, 10);

		
		m_primeColour.referTo (themeState, IDs::PrimeColour, nullptr, juce::Colour(0xffffff00).toString());
		m_backgroundColour.referTo (themeState, IDs::BackgroundColour, nullptr, juce::Colour(0xff181818).toString());
    }

	juce::Colour getPrimeColour()
	{
		return juce::Colour::fromString(juce::String(m_primeColour));
	}
	juce::Colour getBackgroundColour()
	{
		return juce::Colour::fromString(juce::String(m_backgroundColour));
    }
    void addFavoriteType(const juce::Identifier& type)
    {
        auto favoriteTypes = m_applicationStateValueTree
                .getOrCreateChildWithName (IDs::FavoriteTypes, nullptr);
        favoriteTypes.getOrCreateChildWithName (type, nullptr);
    }

    [[nodiscard]] juce::Array<juce::Identifier> getFavoriteTypeList() const
    {
        juce::Array<juce::Identifier> result;
        auto favoriteTypes = m_applicationStateValueTree
                .getChildWithName (IDs::FavoriteTypes);
        if (!favoriteTypes.isValid ())
        {
            return result;
        }
        for (auto i = 0; i < favoriteTypes.getNumChildren (); i++)
        {
            result.add (favoriteTypes.getChild (i).getType ());
        }
        return result;
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
        auto favoritesState = m_applicationStateValueTree.getOrCreateChildWithName (IDs::Favorites, nullptr);
        favoritesState.removeAllChildren (nullptr);
        for (auto favEntry : m_favorites)
        {
            favoritesState.addChild (favEntry->m_state, -1, nullptr);
        }
        auto fileBrowser = m_applicationStateValueTree.getOrCreateChildWithName (
                    IDs::FileBrowser, nullptr);


        auto settingsFile = juce::File::getSpecialLocation (
                    juce::File::userApplicationDataDirectory)
                    .getChildFile ("NextStudio/AppSettings.xml");
        settingsFile.create ();
        auto xmlToWrite = m_applicationStateValueTree.createXml ();
        if (xmlToWrite->writeTo (settingsFile))
        {
            std::cout << "settings written to: " + settingsFile.getFullPathName () << std::endl;
        }
        else
        {
            std::cout << "couldn't write to: " + settingsFile.getFullPathName () << std::endl;
        }
    }

    void addFileToFavorites(const juce::Identifier& tag, const juce::File& file)
    {
        for (auto favEntry : m_favorites)
        {
            if (favEntry->getFile () == file && favEntry->m_tag == tag)
            {
                return;
            }
        }
        auto fav = new Favorite(tag, juce::ValueTree(tag));
        fav->setFile (file);
        m_favorites.add (fav);
        saveState ();
    }

    juce::Array<juce::File> removeFileFromFavorite(const juce::Identifier& tag
                                                   ,const juce::File& file)
    {
        for (auto fav : m_favorites)
        {
            if (fav->getFile () == file && fav->m_tag == tag)
            {
                m_favorites.removeObject (fav);
                saveState ();
            }
        }
        juce::Array<juce::File> currentFileList;
        for (auto fav : m_favorites)
        {
            if (fav->m_tag == tag)
            {
                currentFileList.add (fav->getFile ());
            }
        }
        return currentFileList;
    }

    juce::Colour getRandomTrackColour()
    {
        auto rdm = juce::Random::getSystemRandom().nextInt(m_trackColours.size());
        return m_trackColours[rdm];
    }
    juce::ValueTree m_applicationStateValueTree;
    juce::OwnedArray<Favorite> m_favorites;
    juce::Array<juce::Colour> m_trackColours
    {
           juce::Colour(0xff1dd13d), juce::Colour(0xff008CDC),juce::Colour(0xffFFAD00), juce::Colour(0xffFF3E5A), juce::Colour(0xffC766FF),
           juce::Colour(0xff356800), juce::Colour(0xff054D77), juce::Colour(0xff9A6C0B),juce::Colour(0xff862835), juce::Colour(0xff5A1582),
           juce::Colour(0xffFFF800), juce::Colour(0xff84E185), juce::Colour(0xffEC610F),juce::Colour(0xffD6438A),juce::Colour(0xff0053FF),
           juce::Colour(0xffD3CF4F), juce::Colour(0xff5D937F),juce::Colour(0xffA27956),juce::Colour(0xffAA7A99), juce::Colour(0xff3A5BA1)
    };

    juce::CachedValue<juce::String> m_workDir,
                                    m_presetDir,
                                    m_clipsDir,
                                    m_samplesDir,
                                    m_projectsDir,
                                    m_backgroundColour,
									m_primeColour;
    juce::CachedValue<int>          m_windowXpos,
                                    m_windowYpos,
                                    m_windowWidth,
                                    m_windowHeight,
                                    m_folderTrackIndent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ApplicationViewState)
};
