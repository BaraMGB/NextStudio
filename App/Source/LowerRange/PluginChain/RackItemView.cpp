/*
  ==============================================================================

    RackItemView.cpp
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "LowerRange/PluginChain/RackItemView.h"
#include "LowerRange/PluginChain/PluginPresetInterface.h"
#include "LowerRange/PluginChain/RackView.h"
#include "Plugins/Arpeggiator/ArpeggiatorPlugin.h"
#include "Plugins/Arpeggiator/ArpeggiatorPluginComponent.h"
#include "Plugins/Compressor/CompressorPluginComponent.h"
#include "Plugins/Delay/DelayPluginComponent.h"
#include "Plugins/Delay/NextDelayPlugin.h"
#include "Plugins/DrumSampler/DrumSamplerView.h"
#include "Plugins/EQ/EqPluginComponent.h"
#include "Plugins/Filter/FilterPluginComponent.h"
#include "Plugins/FourOscPlugin/FourOscPluginComponent.h"
#include "Plugins/Reverb/ReverbPluginComponent.h"
#include "Plugins/SimpleSynth/SimpleSynthPlugin.h"
#include "Plugins/SimpleSynth/SimpleSynthPluginComponent.h"
#include "Plugins/SpectrumAnalyzer/SpectrumAnalyzerPlugin.h"
#include "Plugins/SpectrumAnalyzer/SpectrumAnalyzerPluginComponent.h"
#include "Plugins/VST/VstPluginComponent.h"
#include "Plugins/Volume/VolumePluginComponent.h"
#include "Utilities/Utilities.h"

//==============================================================================
static juce::Identifier getCollapsedStateID(te::EditItemID id)
{
    // Ensure the ID is safe for juce::Identifier
    return "c_" + id.toString().retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_");
}

static bool shouldShowPluginPresetManager(te::Plugin &plugin) { return plugin.isSynth() && plugin.getPluginType() != te::ExternalPlugin::xmlTypeName; }

RackItemView::RackItemView(EditViewState &evs, te::Track::Ptr t, te::Plugin::Ptr p)
    : m_evs(evs),
      m_track(t),
      m_plugin(p),
      m_showPluginBtn("Show Plugin", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
{
    // Load collapsed state
    if (m_track)
    {
        auto state = m_evs.getTrackRackViewState(m_track->itemID);
        m_collapsed = state.getProperty(getCollapsedStateID(m_plugin->itemID), false);
    }

    addAndMakeVisible(m_showPluginBtn);
    m_showPluginBtn.addListener(this);

    name.setText(m_plugin->getName(), juce::NotificationType::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(name);
    name.setInterceptsMouseClicks(false, true);
    GUIHelpers::log("PLUGIN TYPE: ", m_plugin->getPluginType());

    if (m_plugin->getPluginType() == "volume")
    {
        m_pluginComponent = std::make_unique<VolumePluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == "4bandEq")
    {
        m_pluginComponent = std::make_unique<EqPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == "delay" || m_plugin->getPluginType() == NextDelayPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<DelayPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == "lowpass")
    {
        m_pluginComponent = std::make_unique<FilterPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == te::CompressorPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<CompressorPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == te::ReverbPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<ReverbPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == "4osc")
    {
        GUIHelpers::log("4OSC");
        m_pluginComponent = std::make_unique<FourOscPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == SimpleSynthPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<SimpleSynthPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == ArpeggiatorPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<ArpeggiatorPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == SpectrumAnalyzerPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<SpectrumAnalyzerPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == te::SamplerPlugin::xmlTypeName)
    {
        GUIHelpers::log(m_plugin->getPluginType());
        if (auto *sampler = dynamic_cast<te::SamplerPlugin *>(p.get()))
        {
            m_pluginComponent = std::make_unique<DrumSamplerView>(evs, *sampler);
        }
        else
        {
            GUIHelpers::log("ERROR: Plugin claims to be sampler but dynamic_cast failed!");
        }
    }
    else
    {
        m_pluginComponent = std::make_unique<VstPluginComponent>(evs, p);
    }

    if (m_pluginComponent)
        addAndMakeVisible(*m_pluginComponent);

    if (m_plugin && shouldShowPluginPresetManager(*m_plugin))
    {
        if (auto *presetInterface = dynamic_cast<PluginPresetInterface *>(m_pluginComponent.get()))
        {
            m_presetManager = std::make_unique<PresetManagerComponent>(*presetInterface);
            addAndMakeVisible(*m_presetManager);
        }
    }
}

RackItemView::RackItemView(EditViewState &evs, te::Track::Ptr t, te::Modifier::Ptr m)
    : m_evs(evs),
      m_track(t),
      m_modifier(m),
      m_showPluginBtn("Show Modifier", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
{
    // Load collapsed state
    if (m_track)
    {
        auto state = m_evs.getTrackRackViewState(m_track->itemID);
        m_collapsed = state.getProperty(getCollapsedStateID(m_modifier->itemID), false);
    }

    addAndMakeVisible(m_showPluginBtn);
    m_showPluginBtn.addListener(this);

    // Modifiers don't have user-defined names usually, so we use the class name or type
    name.setText(m->getName(), juce::NotificationType::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(name);
    name.setInterceptsMouseClicks(false, true);

    if (dynamic_cast<te::LFOModifier *>(m.get()))
        m_modifierComponent = std::make_unique<LFOModifierComponent>(evs, m);
    else if (dynamic_cast<te::StepModifier *>(m.get()))
        m_modifierComponent = std::make_unique<StepModifierComponent>(evs, m);
    else
        m_modifierComponent = std::make_unique<ModifierViewComponent>(evs, m);

    addAndMakeVisible(*m_modifierComponent);
}

RackItemView::~RackItemView()
{
    if (m_plugin)
        m_plugin->hideWindowForShutdown();
}

void RackItemView::paint(juce::Graphics &g)
{
    auto area = getLocalBounds();
    area.reduce(0, 1);
    auto cornerSize = 7.0f;
    auto backgroundColour = m_evs.m_applicationState.getBackgroundColour2();
    g.setColour(backgroundColour);
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), cornerSize, true, false, true, false);

    auto borderRect = area;

    juce::Colour trackCol;
    if (m_plugin)
        trackCol = m_plugin->isEnabled() ? getTrackColour() : getTrackColour().darker(0.7f);
    else
        trackCol = getTrackColour(); // Modifiers are always enabled effectively or handle it internally

    auto labelingCol = trackCol.getBrightness() > 0.8f ? juce::Colour(0xff000000) : juce::Colour(0xffffffff);

    name.setColour(juce::Label::ColourIds::textColourId, labelingCol);

    GUIHelpers::setDrawableOnButton(m_showPluginBtn, BinaryData::expandPluginPlain18_svg, labelingCol);
    auto header = area.removeFromLeft(m_headerWidth);
    g.setColour(trackCol);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, false, true, false);

    if (m_clickOnHeader)
    {
        g.setColour(juce::Colour(0xffffffff));
        g.drawRect(getLocalBounds());
    }

    g.setColour(m_evs.m_applicationState.getBorderColour());
    GUIHelpers::strokeRoundedRectWithSide(g, borderRect.toFloat(), cornerSize, true, false, true, false);
}

void RackItemView::mouseDown(const juce::MouseEvent &e)
{
    if (e.getMouseDownX() < m_headerWidth)
    {
        m_clickOnHeader = true;
        if (e.mods.isRightButtonDown())
        {
            juce::PopupMenu m;
            if (m_plugin)
            {
                m.addItem("Delete", [this] { m_plugin->deleteFromParent(); });
                m.addItem(m_plugin->isEnabled() ? "Disable" : "Enable", [this] { m_plugin->setEnabled(!m_plugin->isEnabled()); });
            }
            else if (m_modifier)
            {
                m.addItem("Delete", [this] { m_modifier->remove(); });
            }

            juce::Component::SafePointer<RackItemView> safeThis(this);
            m.show();

            if (safeThis == nullptr)
                return;
        }
    }
    else
    {
        m_clickOnHeader = false;
    }
    repaint();
}

void RackItemView::mouseDrag(const juce::MouseEvent &e)
{
    if (e.getMouseDownX() < m_headerWidth)
    {
        juce::DragAndDropContainer *dragC = juce::DragAndDropContainer::findParentDragContainerFor(this);
        if (!dragC->isDragAndDropActive())
        {
            dragC->startDragging("PluginComp", this, juce::Image(juce::Image::ARGB, 1, 1, true), false);
        }
    }
}

void RackItemView::draggedOntoAutomatableParameterTarget(const te::AutomatableParameter::Ptr &param)
{
    if (m_modifier)
    {
        if (param->getTrack() != m_track.get())
            return;

        param->addModifier(*m_modifier, 0.5f, 0.0f, 0.5f);
    }
}

void RackItemView::mouseUp(const juce::MouseEvent &event)
{
    m_clickOnHeader = false;
    repaint();
}

void RackItemView::mouseDoubleClick(const juce::MouseEvent &e)
{
    if (e.getMouseDownX() < m_headerWidth)
    {
        m_collapsed = !m_collapsed;

        // Save collapsed state
        if (m_track)
        {
            te::EditItemID id;
            if (m_plugin)
                id = m_plugin->itemID;
            else if (m_modifier)
                id = m_modifier->itemID;

            if (id.isValid())
            {
                auto state = m_evs.getTrackRackViewState(m_track->itemID);
                state.setProperty(getCollapsedStateID(id), m_collapsed, &m_evs.m_edit.getUndoManager());
            }
        }

        if (auto *rackView = findParentComponentOfClass<RackView>())
            rackView->resized();
    }
}

void RackItemView::resized()
{
    auto area = getLocalBounds();
    juce::Rectangle<int> showButton = {area.getX(), area.getY() + 5, m_headerWidth, m_headerWidth};
    m_showPluginBtn.setBounds(showButton);
    auto nameLabelRect = juce::Rectangle<int>(area.getX(), area.getHeight() - m_headerWidth, area.getHeight(), m_headerWidth);
    name.setBounds(nameLabelRect);
    name.setTransform(juce::AffineTransform::rotation(-(juce::MathConstants<float>::halfPi), nameLabelRect.getX() + 10.0, nameLabelRect.getY() + 10.0));
    area.removeFromLeft(m_headerWidth);
    area.reduce(1, 1);

    if (m_presetManager)
        m_presetManager->setBounds(area.removeFromLeft(130));

    if (m_pluginComponent)
        m_pluginComponent->setBounds(area);
    else if (m_modifierComponent)
        m_modifierComponent->setBounds(area);
}

void RackItemView::buttonClicked(juce::Button *button)
{
    if (button == &m_showPluginBtn)
        if (m_plugin)
            m_plugin->showWindowExplicitly();
}

juce::Colour RackItemView::getTrackColour()
{
    if (m_track)
        return m_track->getColour();
    return juce::Colours::grey;
}

int RackItemView::getNeededWidthFactor()
{
    if (m_pluginComponent)
        return m_pluginComponent->getNeededWidth();
    if (m_modifierComponent)
        return m_modifierComponent->getNeededWidth();
    return 1;
}
