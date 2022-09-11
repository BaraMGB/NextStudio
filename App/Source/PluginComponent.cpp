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
    else if (plugin->getPluginType() == "4bandEq")
    {
        m_pluginComponent = std::make_unique<EqPluginComponent>(evs, p);
    }
    else if (plugin->getPluginType() == "delay")
    {
        m_pluginComponent = std::make_unique<DelayPluginComponent>(evs, p);
    }
    else if (plugin->getPluginType() == "lowpass")
    {
        m_pluginComponent = std::make_unique<FilterPluginComponent>(evs, p);
    }
    else
    {
        m_pluginComponent = std::make_unique<VstPluginComponent>(evs, p);
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
    auto cornerSize = 10.0f;
    g.setColour(juce::Colour(0xff242424));
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), cornerSize, true, false, true, false);


    auto trackCol = plugin->isEnabled () ?
                       getTrackColour() : getTrackColour().darker (0.7);

    name.setColour(juce::Label::ColourIds::textColourId,
                   trackCol.getBrightness() > 0.8
                                                 ? juce::Colour(0xff000000)
                                                 : juce::Colour(0xffffffff));

    auto header = area.removeFromLeft(m_headerWidth);
    g.setColour(trackCol);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, false, true, false);

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
FilterPluginComponent::FilterPluginComponent
    (EditViewState& evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    auto um = &evs.m_edit.getUndoManager();

    m_freqPar = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("frequency"), "Freq");
    m_modeLabel.setJustificationType(juce::Justification::centred);


    m_modeButton.onStateChange = [this, um] 
    {
        if (m_modeButton.getToggleState())
            m_plugin->state.setProperty(te::IDs::mode, "highpass", um);
        else
            m_plugin->state.setProperty(te::IDs::mode, "lowpass", um);
        updateLabel(*um);
    };

    addAndMakeVisible(m_modeButton);
    addAndMakeVisible(m_modeLabel);
    addAndMakeVisible(*m_freqPar);
    m_plugin->state.addListener(this);
    updateLabel(*um);
};

void FilterPluginComponent::resized()
{
    auto bounds = getLocalBounds();
    auto h = bounds.getHeight()/12;
    bounds.removeFromTop(h);
    auto modeButtonRect = bounds.removeFromTop(h*3);
    m_modeButton.setBounds(modeButtonRect.reduced(modeButtonRect.getWidth() / 4, modeButtonRect.getHeight() / 4)); 
    m_modeLabel.setBounds(bounds.removeFromTop(h));

    bounds.removeFromTop(h*2);
    m_freqPar->setBounds(bounds.removeFromTop(h*4));
}


void FilterPluginComponent::paint(juce::Graphics &g)
{
}
void FilterPluginComponent::updateLabel (juce::UndoManager& um)
{
    auto mode = m_plugin->state.getPropertyAsValue(
                    te::IDs::mode,&um).toString();
    if (mode == "highpass")
        mode = "Highpass";
    else
        mode = "Lowpass";
    
    m_modeLabel.setText(mode, juce::NotificationType::dontSendNotification); 
}

VolumePluginComponent::VolumePluginComponent
    (EditViewState& evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
        m_panParComp =  std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("pan"), "Pan");
        addAndMakeVisible(*m_panParComp);
        m_volParComp =  std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("volume"), "Vol");
        addAndMakeVisible(*m_volParComp);
        m_plugin->state.addListener(this);

}

void VolumePluginComponent::resized()
{
    auto bounds = getLocalBounds();
    auto h = bounds.getHeight()/12;
    bounds.removeFromTop(h);
    m_volParComp->setBounds(bounds.removeFromTop(h*4));
    bounds.removeFromTop(h*2);
    m_panParComp->setBounds(bounds.removeFromTop(h*4));
}


void VolumePluginComponent::paint(juce::Graphics &g)
{
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
                ParameterComponent* parameterComp = new ParameterComponent(*param);
                param->addListener(this);
                m_parameterComponents.add(parameterComp);
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

void VstPluginComponent::resized()
{
    int scrollPos = m_viewPort.getVerticalScrollBar().getCurrentRangeStart();
    auto area = getLocalBounds();
    const auto widgetHeight = 30;
    if (m_lastChangedParameterComponent)
    {
         m_lastChangedParameterComponent->setBounds(area.removeFromTop(widgetHeight));
    }
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

void VstPluginComponent::parameterChanged (te::AutomatableParameter& param, float /*newValue*/)  
{
    removeChildComponent(m_lastChangedParameterComponent.get());
    m_lastChangedParameterComponent
            = std::make_unique<ParameterComponent>(param);
    addAndMakeVisible(m_lastChangedParameterComponent.get());
    resized();
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
    m_parameterSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    m_parameterSlider.setTextBoxStyle(juce::Slider::NoTextBox, 0, 0, false);
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


