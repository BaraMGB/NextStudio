#include "SidebarComponent.h"
#include "MainComponent.h"
#include "RenderDialog.h"
#include "BinaryData.h"
#include "EditViewState.h"
#include "Utilities.h"


SidebarComponent::SidebarComponent(EditViewState& evs, juce::ApplicationCommandManager& commandManager)
    : m_evs(evs)
    , m_appState(evs.m_applicationState)
    , m_engine(evs.m_edit.engine)
    , m_edit(evs.m_edit)
    , m_commandManager(commandManager)
    , m_menu(m_appState)
    , m_settingsView(m_engine, m_commandManager, m_appState)
    , m_instrumentList(m_engine, true, m_appState)
    , m_effectList(m_engine, false, m_appState)
    , m_samplePreview(m_engine, m_edit, m_appState)
    , m_sampleBrowser(m_appState, m_samplePreview)
    , m_fileListBrowser(m_appState, m_engine, m_samplePreview)
    , m_projectsBrowser(m_evs, m_appState)
{
    addAndMakeVisible(m_menu);
    addChildComponent(m_settingsView);
    addChildComponent(m_instrumentList);
    addChildComponent(m_effectList);
    addChildComponent(m_sampleBrowser);
    addChildComponent(m_samplePreview);
    addChildComponent(m_fileListBrowser);
    addChildComponent(m_projectsBrowser);
    for (auto b : m_menu.getButtons())
        b->addListener(this);

    m_settingsView.setIndent(10);
    m_sampleBrowser.setFileList(findAudioFiles(juce::File(m_appState.m_samplesDir)));
    m_projectsBrowser.setFileList(juce::File(m_appState.m_projectsDir).findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.tracktionedit" ) );
    m_fileListBrowser.setFileList(juce::File(m_appState.m_workDir).findChildFiles(juce::File::TypesOfFileToFind::findFilesAndDirectories , false ) );

    setAllVisibleOff();
    m_projectsBrowser.setVisible(true);
}

SidebarComponent::~SidebarComponent()
{

    if (auto parent = dynamic_cast<MainComponent*>(getParentComponent()))
    {
        m_fileListBrowser.removeChangeListener(parent);
        m_projectsBrowser.removeChangeListener(parent);
    }
    for (auto b : m_menu.getButtons())
        b->removeListener(this);
}

void SidebarComponent::paint(juce::Graphics& g)
{
    auto sideMenu = m_menu.getBounds();
    auto headerRect = getLocalBounds().removeFromTop(CONTENT_HEADER_HEIGHT).withLeft(sideMenu.getWidth());
    auto footerRect = getLocalBounds().removeFromBottom(CONTENT_HEADER_HEIGHT).withLeft(sideMenu.getWidth());
    auto colourBulbH = headerRect.removeFromRight(10);
    auto colourBulbF = footerRect.removeFromRight(10);

    g.setColour(m_appState.getMenuBackgroundColour());
    g.fillRect(headerRect);
    g.fillRect(footerRect);
    g.fillRect(sideMenu);
    g.setColour(m_headerColour);
    g.fillRect(colourBulbH);
    g.fillRect(colourBulbF);

    g.setColour(m_appState.getBorderColour());
    g.drawVerticalLine(sideMenu.getRight() -1, 0, getHeight());
    g.drawHorizontalLine(CONTENT_HEADER_HEIGHT -1, sideMenu.getRight(), getWidth());
    g.drawHorizontalLine(getHeight() - CONTENT_HEADER_HEIGHT, sideMenu.getRight(), getWidth());
    
    g.setColour(m_appState.getTextColour());
    headerRect.reduce(10, 0);
    g.drawText(m_headerName, headerRect, juce::Justification::centredLeft,false );

    auto iconRect = headerRect.removeFromRight(CONTENT_HEADER_HEIGHT);
    iconRect.reduce(2, 2);

    if (m_instrumentList.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::presetsButton_svg, m_headerColour, iconRect.toFloat());    
    else if (m_projectsBrowser.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::projectsButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_sampleBrowser.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::samplesButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_effectList.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::pluginsButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_fileListBrowser.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::homeButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_settingsView.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::settingsButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_renderComponent != nullptr)
        GUIHelpers::drawFromSvg(g, BinaryData::renderButton_svg, m_headerColour, iconRect.toFloat());

}
void SidebarComponent::paintOverChildren(juce::Graphics& g)
{
    GUIHelpers::drawFakeRoundCorners(g, getLocalBounds().toFloat(), m_appState.getMainFrameColour(),m_appState.getBorderColour());
}

void SidebarComponent::resized() 
{
    auto area = getLocalBounds();

    m_menu.setBounds(area.removeFromLeft(70));

    if (m_appState.m_sidebarCollapsed == false)
    {
        area.removeFromTop(CONTENT_HEADER_HEIGHT);
        area.removeFromBottom(CONTENT_HEADER_HEIGHT);

        if (m_settingsView.isVisible())
        {
            m_settingsView.setBounds(area);
            m_headerName = "Settings";
            m_headerColour = m_appState.getSettingsColour();
        }
        else if (m_instrumentList.isVisible())
        {
            m_instrumentList.setBounds(area);
            m_headerName = "Instrument Plugins";
            m_headerColour = m_appState.getInstrumentsColour();
        }
        else if (m_effectList.isVisible())
        {
            m_effectList.setBounds(area);
            m_headerName = "Effect Plugins";
            m_headerColour = m_appState.getEffectsColour();
        }
        else if (m_fileListBrowser.isVisible())
        {
            auto bounds = area;
            if (m_samplePreview.isVisible())
            {
                auto preview = area.removeFromBottom(180);
                m_samplePreview.setBounds(preview);
            }
            m_fileListBrowser.setBounds(area);
            m_headerName = "Home Folder";
            m_headerColour = m_appState.getHomeColour();
        }
        else if (m_sampleBrowser.isVisible())
        {
            auto bounds = area;
            if (m_samplePreview.isVisible())
            {
                auto preview = area.removeFromBottom(180);
                m_samplePreview.setBounds(preview);
            }
            m_sampleBrowser.setBounds(area);
            m_headerName = "Samples";
            m_headerColour = m_appState.getSamplesColour();
        }
        else if (m_projectsBrowser.isVisible())
        {
            auto bounds = area;
            m_projectsBrowser.setBounds(bounds);
            m_headerColour = m_appState.getProjectsColour();
            m_headerName = "Projects";
        }
        else if (m_renderComponent != nullptr) 
        {
            addAndMakeVisible(*m_renderComponent);
            auto bounds = area;
            m_renderComponent->setBounds(bounds);
            m_headerColour = m_appState.getRenderColour();
            m_headerName = "Render";
        }
    }
    repaint();
}
void SidebarComponent::buttonClicked (juce::Button* button)
{

    setAllVisibleOff();
    auto parent = dynamic_cast<MainComponent*>(getParentComponent());
    if (parent)
    {
        if (auto db = dynamic_cast<juce::DrawableButton*>(button))
        {
            db->getNormalImage()->replaceColour(juce::Colour(0xffffff), juce::Colours::greenyellow);
        }

        if (m_lastClickedButton == button->getName())
        {
            if (!m_appState.m_sidebarCollapsed)
                m_cachedSidebarWidth = m_appState.m_sidebarWidth;
            m_appState.m_sidebarCollapsed = !m_appState.m_sidebarCollapsed;
        }
        else {
            m_appState.m_sidebarCollapsed = false;
            if (m_cachedSidebarWidth == 0)
                m_cachedSidebarWidth = m_appState.m_sidebarWidth;
        }

        if (m_appState.m_sidebarCollapsed)
        {
            m_appState.m_sidebarWidth = m_menu.getWidth();
        }
        else 
        {
            m_appState.m_sidebarWidth = m_cachedSidebarWidth;
            if (m_appState.m_sidebarWidth < m_appState.m_minSidebarWidth)
            {
                m_appState.m_sidebarWidth = m_appState.m_minSidebarWidth;
            }

            if (button->getName() == "Settings")
            {
                m_settingsView.setVisible(true);
            }
            else if (button->getName() == "Instruments")
            {
                m_instrumentList.setVisible(true);
            }
            else if (button->getName() == "Samples")
            {
                m_sampleBrowser.setVisible(true);
                m_samplePreview.setVisible(true);
            }
            else if (button->getName() == "Projects")
            {
                m_projectsBrowser.setVisible(true);
            }
            else if (button->getName() == "Effects")
            {
                m_effectList.setVisible(true);
            }
            else if (button->getName() == "Render")
            {
                m_renderComponent = std::make_unique<RenderDialog>(m_evs);
            }
            else if (button->getName() == "Home")
            {
                m_fileListBrowser.setVisible(true);
                m_samplePreview.setVisible(true);
            }
        }
        parent->resized();
    }

    resized();
    m_lastClickedButton = button->getName();

}

void SidebarComponent::updateParentsListener()
{
    if (auto parent = dynamic_cast<MainComponent*>(getParentComponent()))
    {
        m_fileListBrowser.addChangeListener(parent);
        m_projectsBrowser.addChangeListener(parent);
    }
}

void SidebarComponent::setAllVisibleOff()
{
    m_settingsView.setVisible(false);
    m_instrumentList.setVisible(false);
    m_effectList.setVisible(false);
    m_fileListBrowser.setVisible(false);
    m_projectsBrowser.setVisible(false);
    m_sampleBrowser.setVisible(false);
    m_samplePreview.setVisible(false);
    if (m_renderComponent != nullptr)
    {
        m_renderComponent.reset();
    }

}

juce::Array<juce::File> SidebarComponent::findAudioFiles(const juce::File& directory)
{
      juce::Array<juce::File> audioFiles;

    // Check if the provided File object is a directory
    if (!directory.isDirectory())
    {
        // If not, return an empty array
        return audioFiles;
    }

    // Initialize the AudioFormatManager and register basic formats
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    // Define valid audio file extensions
    std::unordered_set<juce::String> validExtensions = { ".wav", ".WAV", ".mp3", ".MP3", ".aiff", ".AIFF", ".flac", ".FLAC", ".ogg", ".OGG", ".m4a", ".M4A" };

    // Find all files in the directory and its subdirectories
    juce::Array<juce::File> allFiles = directory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*");

    // Use a set to track unique files
    std::unordered_set<juce::String> uniqueFiles;

    for (const auto& file : allFiles)
    {
        // Check if the file has a valid audio extension
        if (validExtensions.find(file.getFileExtension()) != validExtensions.end())
        {
            // Attempt to create a reader for the file
            std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
            
            if (reader != nullptr)
            {
                // If reader is valid and file is unique, add to audioFiles
                if (uniqueFiles.find(file.getFullPathName()) == uniqueFiles.end())
                {
                    audioFiles.add(file);
                    uniqueFiles.insert(file.getFullPathName());
                }
            }
        }
    }

    return audioFiles;
}
