#include "SidebarComponent.h"
#include "BinaryData.h"
#include "Utilities.h"


void SidebarComponent::paint(juce::Graphics& g)
{
    g.fillAll(m_appState.getMenuBackgroundColour());

    g.setColour(m_appState.getTextColour());
    auto headerRect = getLocalBounds().removeFromTop(CONTENT_HEADER_HEIGHT);
    headerRect.removeFromLeft(m_menu.getWidth());
    headerRect.reduce(10, 0);
    g.drawText(m_headerName, headerRect, juce::Justification::centredLeft,false );

    auto iconRect = headerRect.removeFromRight(CONTENT_HEADER_HEIGHT);
    iconRect.reduce(2, 2);

    if (m_instrumentList.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::presetsButton_svg, m_headerColour, iconRect.toFloat());    
    else if (m_effectList.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::pluginsButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_fileBrowser.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::homeButton_svg, m_headerColour, iconRect.toFloat());
    else if (m_settingsView.isVisible())
        GUIHelpers::drawFromSvg(g, BinaryData::settingsButton_svg, m_headerColour, iconRect.toFloat());
}
void SidebarComponent::paintOverChildren(juce::Graphics& g)
{
    g.setColour(m_appState.getBorderColour());
    g.drawVerticalLine(m_menu.getWidth() -1, 0, getHeight());
    g.drawHorizontalLine(CONTENT_HEADER_HEIGHT, m_menu.getWidth(), getWidth());
    GUIHelpers::drawFakeRoundCorners(g, getLocalBounds(), m_appState.getBackgroundColour(),m_appState.getBorderColour());
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
        // m_pluginList.resized();
        resized();
    }
    else if (button->getName() == "Samples")
    {
        setAllVisibleOff();
        // m_pluginList.resized();
        resized();
    }
    else if (button->getName() == "Effects")
    {
        setAllVisibleOff();
        m_effectList.setVisible(true);
        // m_pluginList.resized();
        resized();
    }
    else if (button->getName() == "Home")
    {
        setAllVisibleOff();
        m_fileBrowser.setVisible(true);
        // m_pluginList.resized();
        resized();
    }

}

void SidebarComponent::setAllVisibleOff()
{
    m_settingsView.setVisible(false);
    m_instrumentList.setVisible(false);
    m_effectList.setVisible(false);
    m_fileBrowser.setVisible(false);
}
