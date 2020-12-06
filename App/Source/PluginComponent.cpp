#include "PluginComponent.h"

//==============================================================================
PluginWindowComponent::PluginWindowComponent
    (EditViewState& evs, te::Plugin::Ptr p, juce::Colour tc)
    : editViewState (evs), plugin (p), m_trackColour (tc)
{
    name.setText(plugin->getName(),juce::NotificationType::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(name);
    name.setInterceptsMouseClicks (false, true);

    if (plugin->getPluginType() == "volume")
    {
        m_pluginComponent = std::make_unique<VolumePluginComponent>(evs, p, tc);
    }
    else if (plugin->getPluginType() == "vst")
    {
        m_pluginComponent = std::make_unique<VstPluginComponent>(evs, p, tc);
    }
    else
    {
        m_pluginComponent = std::make_unique<PluginComponent>(evs, p, tc);
    }
    addAndMakeVisible(*m_pluginComponent);
}

PluginWindowComponent::~PluginWindowComponent()
{
    plugin->hideWindowForShutdown ();
}

void PluginWindowComponent::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    g.setColour(juce::Colour(0xff242424));
    auto cornerSize = 10.0f;
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), cornerSize, true);



    g.setColour(plugin->isEnabled () ?
                       m_trackColour : m_trackColour.darker (0.7));

    name.setColour(juce::Label::ColourIds::textColourId,
                   m_trackColour.getBrightness() > 0.8
                                                 ? juce::Colour(0xff000000)
                                                 : juce::Colour(0xffffffff));

    auto header = area.removeFromLeft(m_headerWidth);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true);

    if (m_clickOnHeader)
    {
        g.setColour (juce::Colour(0xffffffff));
        g.drawRect (getLocalBounds ());
    }
}

void PluginWindowComponent::mouseDown (const juce::MouseEvent& e)
{
    if (e.getMouseDownX () < m_headerWidth)
    {
        m_clickOnHeader = true;
        if (e.mods.isRightButtonDown())
        {
            juce::PopupMenu m;
            m.addItem ("Delete", [this] { plugin->deleteFromParent(); });
            m.addItem (plugin->isEnabled () ?
                                  "Disable" : "Enable"
                       , [this] {plugin->setEnabled (!plugin->isEnabled ());});
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
}

void PluginWindowComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (e.getMouseDownX () < m_headerWidth)
    {
        juce::DragAndDropContainer* dragC
                = juce::DragAndDropContainer::findParentDragContainerFor(this);
        if (!dragC->isDragAndDropActive())
        {
            dragC->startDragging(
                        "PluginComp"
                        , this
                        , juce::Image(juce::Image::ARGB,1,1,true)
                        , false);
        }
    }
}

void PluginWindowComponent::mouseUp(const juce::MouseEvent &event)
{
    m_clickOnHeader = false;
    repaint ();
}

void PluginWindowComponent::resized()
{
    auto area = getLocalBounds();
    auto nameLabelRect = juce::Rectangle<int>(area.getX()
                                              , area.getHeight() - m_headerWidth
                                              , area.getHeight()
                                              , m_headerWidth);
    name.setBounds(nameLabelRect);
    name.setTransform(juce::AffineTransform::rotation ( - (juce::MathConstants<float>::halfPi)
                                                 , nameLabelRect.getX() + 10.0
                                                 , nameLabelRect.getY() + 10.0 ));
    area.removeFromLeft(m_headerWidth);
    m_pluginComponent.get()->setBounds(area);
}

PluginComponent::PluginComponent
    (EditViewState& evs, te::Plugin::Ptr p, juce::Colour tc)
    : m_editViewState (evs), m_plugin (p), m_trackColour (tc)
{
}

te::Plugin::Ptr PluginComponent::getPlugin() const
{
    return m_plugin;
}

void PluginComponent::setPlugin(const te::Plugin::Ptr &plugin)
{
    m_plugin = plugin;
}

//------------------------------------------------------------------------------

VolumePluginComponent::VolumePluginComponent
    (EditViewState& evs, te::Plugin::Ptr p, juce::Colour tc)
    : PluginComponent(evs, p, tc)
{
    addAndMakeVisible(m_volumeKnob);
    m_volumeKnob.setRange(0.0f, 3.0f, 0.01f);
    m_volumeKnob.setSkewFactorFromMidPoint(1.0f);
    if (auto volumePlugin
            = dynamic_cast<te::VolumeAndPanPlugin*> (getPlugin().get()))
    {
        m_volumeKnob.getValueObject().referTo(
                    volumePlugin->volume.getPropertyAsValue());
        m_volumeKnob.setValue(volumePlugin->volume);
    }
    m_volumeKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    m_volumeKnob.setTextBoxStyle(juce::Slider::NoTextBox, 0, 0, false);
}

void VolumePluginComponent::resized()
{
    m_volumeKnob.setBounds(getLocalBounds());
}

void VolumePluginComponent::paint(juce::Graphics &g)
{
    g.setColour(juce::Colour(0xff00ff00));
    g.fillAll();
}

// -----------------------------------------------------------------------------

VstPluginComponent::VstPluginComponent
    (EditViewState& evs, te::Plugin::Ptr p, juce::Colour tc)
    : PluginComponent(evs, p, tc)
    , m_lastChangedParameterComponent(nullptr)
{
    if (p)
    {
        for (auto & param : p->getAutomatableParameters())
        {
            if (param)
            {
                param->addListener(this);
                ParameterComponent* parameterComp
                        = new ParameterComponent(*param);
                m_parameterComponents.add(parameterComp);
                parameterComp->addChangeListener(this);
                m_pluginListComponent.addAndMakeVisible(parameterComp);
            }
        }
    }

    addAndMakeVisible(m_viewPort);

    if (p->getAutomatableParameter(0))
    {
        m_lastChangedParameterComponent
                = std::make_unique<ParameterComponent>(
                                            *(p->getAutomatableParameter(0)));
        addAndMakeVisible(*m_lastChangedParameterComponent);
    }
    m_viewPort.setViewedComponent(&m_pluginListComponent, true);
    m_viewPort.setScrollBarsShown(true, false, true, false);
}

VstPluginComponent::~VstPluginComponent()
{
    if (getPlugin())
    {
        for (auto & param : getPlugin()->getAutomatableParameters())
        {
            if (param)
            {
                param->removeListener(this);
            }
        }
    }
}

void VstPluginComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (auto pc = dynamic_cast<ParameterComponent*> (source))
    {
        parameterChanged( pc->getParameter(),0);
    }
}

void VstPluginComponent::parameterChanged(te::AutomatableParameter &ap, float)
{
    m_lastChangedParameterComponent = std::make_unique<ParameterComponent>(ap);
    addAndMakeVisible(*m_lastChangedParameterComponent);

    resized();
}

void VstPluginComponent::resized()
{
    auto area = getLocalBounds();
    const auto widgetHeight = 30;
    m_lastChangedParameterComponent->setBounds(area.removeFromTop(widgetHeight));
    m_viewPort.setBounds(area);
    m_pluginListComponent.setBounds(area.getX()
                                    , area.getY()
                                    , area.getWidth()
                                    ,m_parameterComponents.size() * widgetHeight);

    auto pcb = m_pluginListComponent.getBounds();
    for (auto & pc : m_parameterComponents)
    {
        pc->setBounds(pcb.removeFromTop(widgetHeight));
    }
}
