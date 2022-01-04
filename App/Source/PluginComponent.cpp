#include "PluginComponent.h"

//==============================================================================
PluginWindowComponent::PluginWindowComponent
    (EditViewState& evs, te::Plugin::Ptr p)
    : editViewState (evs), plugin (p)
{
    name.setText(plugin->getName(),juce::NotificationType::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(name);
    name.setInterceptsMouseClicks (false, true);

    if (plugin->getPluginType() == "volume")
    {
        m_pluginComponent = std::make_unique<VolumePluginComponent>(evs, p);
    }
    else if (plugin->getPluginType() == "vst")
    {
        m_pluginComponent = std::make_unique<VstPluginComponent>(evs, p);
    }
    else
    {
        m_pluginComponent = std::make_unique<PluginViewComponent>(evs, p);
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


    auto trackCol = getTrackColour();
    g.setColour(plugin->isEnabled () ?
                       trackCol : trackCol.darker (0.7));

    name.setColour(juce::Label::ColourIds::textColourId,
                   trackCol.getBrightness() > 0.8
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

juce::Colour PluginWindowComponent::getTrackColour()
{
    if (plugin->getOwnerTrack())
        return plugin->getOwnerTrack()->getColour();
    return juce::Colours::grey;
}

PluginViewComponent::PluginViewComponent
    (EditViewState& evs, te::Plugin::Ptr p)
    : m_editViewState (evs), m_plugin (p)
{
}

te::Plugin::Ptr PluginViewComponent::getPlugin() const
{
    return m_plugin;
}

void PluginViewComponent::setPlugin(const te::Plugin::Ptr &plugin)
{
    m_plugin = plugin;
}

//------------------------------------------------------------------------------

VolumePluginComponent::VolumePluginComponent
    (EditViewState& evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
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
    (EditViewState& evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
    , m_lastChangedParameterComponent(nullptr)
{
    if (p)
    {
        for (auto & param : p->getAutomatableParameters())
        {
            if (param)
            {
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

}

void VstPluginComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (auto pc = dynamic_cast<ParameterComponent*>(source))
    {
        if (pc->getParameter().paramID
                != m_lastChangedParameterComponent->getParameter().paramID)
        {
            removeChildComponent(m_lastChangedParameterComponent.get());
            m_lastChangedParameterComponent
                    = std::make_unique<ParameterComponent>(pc->getParameter());
            addAndMakeVisible(m_lastChangedParameterComponent.get());
            resized();
        }
    }
}

void VstPluginComponent::resized()
{
    int scrollPos = m_viewPort.getVerticalScrollBar().getCurrentRangeStart();
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
    m_viewPort.getVerticalScrollBar().setCurrentRangeStart(scrollPos);
}

//------------------------------------------------------------------------------

ParameterComponent::ParameterComponent(tracktion_engine::AutomatableParameter &ap)
    : m_parameter(ap)
    , m_parameterSlider(ap)
{
    m_parameterName.setText(ap.getParameterName(),
                            juce::NotificationType::dontSendNotification);
    m_parameterName.setInterceptsMouseClicks(false, false);

    m_parameterSlider.setOpaque(false);
    addAndMakeVisible(m_parameterName);
    addAndMakeVisible(m_parameterSlider);
    m_parameterSlider.setRange(0.0f, 3.0f, 0.01f);
    m_parameterSlider.setSkewFactorFromMidPoint(1.0f);
    //m_parameterSlider.setValue(ap.getCurrentValue());
    m_parameterSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    m_parameterSlider.setTextBoxStyle(juce::Slider::NoTextBox, 0, 0, false);
    m_parameterSlider.onValueChange = [this] { sendChangeMessage(); };
}

void ParameterComponent::resized()
{
    auto area = getLocalBounds();

    m_parameterSlider.setBounds(area.removeFromLeft(area.getHeight()));
    m_parameterName.setBounds(area);
}

void ParameterComponent::mouseDown(const juce::MouseEvent &e)
{
    if (e.mods.isLeftButtonDown())
        sendChangeMessage();
}
