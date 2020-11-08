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
    g.fillRect(area);

    g.setColour(plugin->isEnabled () ?
                       m_trackColour : m_trackColour.darker (0.7));

    auto header = area.removeFromLeft(m_headerWidth);
    g.fillRect(header);

    if (m_clickOnHeader)
    {
        g.setColour (juce::Colour(0xffffffff));
        g.drawRect (getLocalBounds ());
    }
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
