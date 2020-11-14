#include "PluginComponent.h"

//==============================================================================
PluginComponent::PluginComponent (EditViewState& evs, te::Plugin::Ptr p, juce::Colour tc)
    : editViewState (evs), plugin (p), m_trackColour (tc)
{
    name.setText(plugin->getName(),juce::NotificationType::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(name);
    name.setInterceptsMouseClicks (false, true);
}

PluginComponent::~PluginComponent()
{
    plugin->hideWindowForShutdown ();
}

void PluginComponent::paint (Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour(Colour(0xff242424));
    auto cornerSize = 10;
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), cornerSize, true);



    g.setColour(plugin->isEnabled () ?
                       m_trackColour : m_trackColour.darker (0.7));

    name.setColour(Label::ColourIds::textColourId, m_trackColour.getBrightness() > 0.8 ?
                                                                    Colour(0xff000000) : Colour(0xffffffff));

    auto header = area.removeFromLeft(m_headerWidth);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true);

    if (m_clickOnHeader)
    {
        g.setColour (juce::Colour(0xffffffff));
        g.drawRect (getLocalBounds ());
    }

//   ****************** Draw shadow Text ****************************************************
    //    auto textRect = getLocalBounds().toFloat();
    //    juce::Image image(Image::ARGB, textRect.getWidth(), textRect.getHeight(), true);
    //    Graphics imageGraph (image);
    //    imageGraph.setColour(Colour(0x00000000));
    //    imageGraph.fillAll();
    //    auto irect = Rectangle<float>(0.0, 0.0, textRect.getWidth(), textRect.getHeight());
    //    imageGraph.setColour(Colour(0xffdddddd));
    //    imageGraph.setFont(16);
    //    imageGraph.drawText("Hello",irect, Justification::centred, false);
    //    DropShadow drops;
    //    drops.colour = Colour(0xff222222);
    //    drops.radius = 1;
    //    drops.drawForImage(g,image);
    //    drops.drawForImage(g,image);
    //    g.drawImage(image,textRect);

}
void PluginComponent::mouseDown (const MouseEvent& e)
{
    if (e.getMouseDownX () < m_headerWidth)
    {
        m_clickOnHeader = true;
        if (e.mods.isRightButtonDown())
        {
            PopupMenu m;
            m.addItem ("Delete", [this] { plugin->deleteFromParent(); });
            m.addItem (plugin->isEnabled () ? "Disable" : "Enable", [this] { plugin->setEnabled (!plugin->isEnabled ());});
            m.show();
        }
        else if(e.getMouseDownY () < m_headerWidth)
        {
            plugin->showWindowExplicitly();
        }
    }
    else
    {
        m_clickOnHeader = false;
    }
    repaint ();
    //std::cout << plugin->state.toXmlString() << std::endl;
}

void PluginComponent::mouseDrag(const MouseEvent &e)
{
    if (e.getMouseDownX () < m_headerWidth)
    {
        DragAndDropContainer* dragC = DragAndDropContainer::findParentDragContainerFor(this);
        if (!dragC->isDragAndDropActive())
        {
            dragC->startDragging("PluginComp", this,juce::Image(Image::ARGB,1,1,true),
                                 false);
        }
    }
}

void PluginComponent::mouseUp(const MouseEvent &event)
{
    m_clickOnHeader = false;
    repaint ();
}
void PluginComponent::resized()
{
    auto area = getLocalBounds();
    auto nameLabelRect = juce::Rectangle<int>(area.getX(),
                                              area.getHeight() - m_headerWidth,
                                              area.getHeight(),
                                              m_headerWidth);
    name.setBounds(nameLabelRect);
    name.setTransform(AffineTransform::rotation (-MathConstants<float>::halfPi,
                                                 nameLabelRect.getX() + 10.0 ,
                                                nameLabelRect.getY() + 10.0 ));
}
