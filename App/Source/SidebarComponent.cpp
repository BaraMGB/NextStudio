#include "SidebarComponent.h"
#include "RenderDialog.h"
#include "BinaryData.h"
#include "EditViewState.h"
#include "Utilities.h"
#include <memory>


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
    , m_projectsBrowser(m_appState)
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

    m_sampleBrowser.setFileList(juce::File(m_appState.m_samplesDir).findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.wav" ) );
    m_projectsBrowser.setFileList(juce::File(m_appState.m_projectsDir).findChildFiles(juce::File::TypesOfFileToFind::findFiles, true, "*.tracktionedit" ) );
    m_fileListBrowser.setFileList(juce::File(m_appState.m_workDir).findChildFiles(juce::File::TypesOfFileToFind::findFilesAndDirectories , false ) );

    
    setAllVisibleOff();
    m_projectsBrowser.setVisible(true);
}

SidebarComponent::~SidebarComponent()
{
    for (auto b : m_menu.getButtons())
        b->removeListener(this);
}

void SidebarComponent::paint(juce::Graphics& g)
{
    g.fillAll(m_appState.getMenuBackgroundColour());

    g.setColour(m_headerColour);
    auto headerRect = getLocalBounds().removeFromTop(CONTENT_HEADER_HEIGHT);
    auto footerRect = getLocalBounds().removeFromBottom(CONTENT_HEADER_HEIGHT);
    auto colourBulbH = headerRect.removeFromRight(20);
    auto colourBulbF = footerRect.removeFromRight(20);

    g.fillRect(colourBulbH);
    g.fillRect(colourBulbF);
    
    g.setColour(m_appState.getTextColour());
    headerRect.removeFromLeft(m_menu.getWidth());
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

}
void SidebarComponent::paintOverChildren(juce::Graphics& g)
{
    g.setColour(m_appState.getBorderColour());
    g.drawVerticalLine(m_menu.getWidth() -1, 0, getHeight());
    g.drawHorizontalLine(CONTENT_HEADER_HEIGHT, m_menu.getWidth(), getWidth());
    g.drawHorizontalLine(getHeight() - CONTENT_HEADER_HEIGHT, m_menu.getWidth(), getWidth());
    GUIHelpers::drawFakeRoundCorners(g, getLocalBounds().toFloat(), m_appState.getBackgroundColour(),m_appState.getBorderColour());
}

void SidebarComponent::resized() 
{
    auto area = getLocalBounds();

    m_menu.setBounds(area.removeFromLeft(70));
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
            auto preview = area.removeFromBottom(100);
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
            auto preview = area.removeFromBottom(100);
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
    repaint();
}
void SidebarComponent::buttonClicked (juce::Button* button)
{
    std::cout << "button clicked: " << button->getName() << std::endl;
    if (auto db = dynamic_cast<juce::DrawableButton*>(button))
    {
        db->getNormalImage()->replaceColour(juce::Colour(0xffffff), juce::Colours::greenyellow);
    }

    if (button->getName() == "Settings")
    {
        setAllVisibleOff();
        m_settingsView.setVisible(true);
        resized();
    }
    else if (button->getName() == "Instruments")
    {
        setAllVisibleOff();
        m_instrumentList.setVisible(true);
        resized();
    }
    else if (button->getName() == "Samples")
    {
        setAllVisibleOff();
        m_sampleBrowser.setVisible(true);
        m_samplePreview.setVisible(true);
        resized();
    }
    else if (button->getName() == "Projects")
    {
        setAllVisibleOff();
        m_projectsBrowser.setVisible(true);
        resized();
    }
    else if (button->getName() == "Effects")
    {
        setAllVisibleOff();
        m_effectList.setVisible(true);
        resized();
    }
    else if (button->getName() == "Render")
    {
        setAllVisibleOff();
        m_renderComponent = std::make_unique<RenderDialog>(m_evs);
        resized();
    }
    else if (button->getName() == "Home")
    {
        setAllVisibleOff();
        m_fileListBrowser.setVisible(true);
        m_samplePreview.setVisible(true);
        resized();
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
