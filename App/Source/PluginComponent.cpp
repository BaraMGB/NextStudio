#include "PluginComponent.h"

//==============================================================================
PluginComponent::PluginComponent (EditViewState& evs, te::Plugin::Ptr p)
    : editViewState (evs), plugin (p)
{
    name.setText(plugin->getName(),juce::NotificationType::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(name);
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
    if (plugin.getObject()->getOwnerTrack())
    {
        g.setColour(plugin.getObject()->getOwnerTrack()->getColour());
    }
    auto header = area.removeFromLeft(20);
    g.fillRect(header);
}
void PluginComponent::mouseDown (const MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        PopupMenu m;
        m.addItem ("Delete", [this] { plugin->deleteFromParent(); });
        m.show();
    }
    else
    {
        //std::cout << plugin->state.toXmlString() << std::endl;
        plugin->showWindowExplicitly();

    }
}

void PluginComponent::resized()
{
    auto area = getLocalBounds();
    auto nameLabelRect = juce::Rectangle<int>(area.getX(), area.getHeight() - 20, area.getHeight(), 20);
    name.setBounds(nameLabelRect);
    name.setTransform(AffineTransform::rotation (-MathConstants<float>::halfPi,
                                                nameLabelRect.getX() + 10.0 ,
                                                nameLabelRect.getY() + 10.0 ));
}
