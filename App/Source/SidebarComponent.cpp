#include "SidebarComponent.h"
#include "Utilities.h"


void SidebarComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour(m_appState.getBackgroundColour());
    g.fillRect(area);
    g.setColour(juce::Colours::lightgrey);
    g.drawVerticalLine(m_menu.getWidth(), 0, getHeight());
}

void SidebarComponent::paintOverChildren(juce::Graphics& g)
{
    GUIHelpers::drawFakeRoundCorners(g, getLocalBounds());
}

void SidebarComponent::buttonClicked (juce::Button* button)
{
    std::cout << "button clicked: " << button->getName() << std::endl;
    if (auto db = dynamic_cast<juce::DrawableButton*>(button))
    {
        db->getNormalImage()->replaceColour(juce::Colour(0xffffff), juce::Colours::greenyellow);
    }
}
