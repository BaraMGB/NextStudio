#include "SidebarComponent.h"
#include "Utilities.h"


void SidebarComponent::paintOverChildren(juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour(juce::Colours::lightgrey);
    g.drawVerticalLine(m_menu.getWidth() -1, 0, getHeight());
    GUIHelpers::drawFakeRoundCorners(g, getLocalBounds());
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
    m_fileBrowser.setVisible(false);
}
