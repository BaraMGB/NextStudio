/*
  ==============================================================================

    PluginChainItemView.cpp
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "PluginChainItemView.h"
#include "PluginChainView.h"
#include "PluginPresetInterface.h"
#include "ArpeggiatorPlugin.h"
#include "ArpeggiatorPluginComponent.h"
#include "ChorusPluginComponent.h"
#include "NextChorusPlugin.h"
#include "CompressorPluginComponent.h"
#include "DelayPluginComponent.h"
#include "NextDelayPlugin.h"
#include "DrumSamplerView.h"
#include "EqPluginComponent.h"
#include "FilterPluginComponent.h"
#include "NextFilterPlugin.h"
#include "PeakLimiterPlugin.h"
#include "PeakLimiterPluginComponent.h"
#include "NextPhaserPlugin.h"
#include "PhaserPluginComponent.h"
#include "ReverbPluginComponent.h"
#include "NextSaturationPlugin.h"
#include "SaturationPluginComponent.h"
#include "SimpleSynthPlugin.h"
#include "SimpleSynthPluginComponent.h"
#include "SoundFontPlugin.h"
#include "SoundFontPluginComponent.h"
#include "SpectrumAnalyzerPlugin.h"
#include "SpectrumAnalyzerPluginComponent.h"
#include "VstPluginComponent.h"
#include "VolumePluginComponent.h"
#include "Browser_Base.h"
#include "Utilities.h"

//==============================================================================
static juce::Identifier getCollapsedStateID(te::EditItemID id)
{
    // Ensure the ID is safe for juce::Identifier
    return "c_" + id.toString().retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_");
}

static bool shouldShowPluginPresetManager(te::Plugin &plugin) { return plugin.isSynth() && plugin.getPluginType() != te::ExternalPlugin::xmlTypeName; }

static juce::File getDraggedBrowserFile(const juce::DragAndDropTarget::SourceDetails &details)
{
    if (auto *browser = dynamic_cast<BrowserListBox *>(details.sourceComponent.get()))
        return browser->getSelectedFile();

    return {};
}

PluginChainItemView::PluginChainItemView(EditViewState &evs, te::Track::Ptr t, te::Plugin::Ptr p)
    : m_evs(evs),
      m_track(t),
      m_plugin(p),
      m_showPluginBtn("Show Plugin", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
{
    // Load collapsed state
    if (m_track)
    {
        auto state = m_evs.getTrackPluginChainViewState(m_track->itemID);
        m_collapsed = state.getProperty(getCollapsedStateID(m_plugin->itemID), false);
    }

    addAndMakeVisible(m_showPluginBtn);
    m_showPluginBtn.addListener(this);

    name.setText(m_plugin->getName(), juce::NotificationType::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(name);
    name.setInterceptsMouseClicks(false, true);
    name.setVisible(false);
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
    else if (m_plugin->getPluginType() == NextFilterPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<FilterPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == te::CompressorPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<CompressorPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == PeakLimiterPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<PeakLimiterPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == te::ReverbPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<ReverbPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == NextChorusPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<ChorusPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == NextPhaserPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<PhaserPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == NextSaturationPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<SaturationPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == SimpleSynthPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<SimpleSynthPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == SoundFontPlugin::xmlTypeName)
    {
        m_pluginComponent = std::make_unique<SoundFontPluginComponent>(evs, p);
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
            auto headerColour = m_track != nullptr ? m_track->getColour() : m_evs.m_applicationState.getPrimeColour();
            m_presetManager = std::make_unique<PresetManagerComponent>(*presetInterface, headerColour);
            addAndMakeVisible(*m_presetManager);
        }
    }
}

PluginChainItemView::PluginChainItemView(EditViewState &evs, te::Track::Ptr t, te::Modifier::Ptr m)
    : m_evs(evs),
      m_track(t),
      m_modifier(m),
      m_showPluginBtn("Show Modifier", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
{
    // Load collapsed state
    if (m_track)
    {
        auto state = m_evs.getTrackPluginChainViewState(m_track->itemID);
        m_collapsed = state.getProperty(getCollapsedStateID(m_modifier->itemID), false);
    }

    addAndMakeVisible(m_showPluginBtn);
    m_showPluginBtn.addListener(this);

    // Modifiers don't have user-defined names usually, so we use the class name or type
    name.setText(m->getName(), juce::NotificationType::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(name);
    name.setInterceptsMouseClicks(false, true);
    name.setVisible(false);

    if (dynamic_cast<te::LFOModifier *>(m.get()))
        m_modifierComponent = std::make_unique<LFOModifierComponent>(evs, m);
    else if (dynamic_cast<te::StepModifier *>(m.get()))
        m_modifierComponent = std::make_unique<StepModifierComponent>(evs, m);
    else
        m_modifierComponent = std::make_unique<ModifierViewComponent>(evs, m);

    addAndMakeVisible(*m_modifierComponent);
}

PluginChainItemView::~PluginChainItemView()
{
    if (m_plugin)
        m_plugin->hideWindowForShutdown();
}

void PluginChainItemView::paint(juce::Graphics &g)
{
    auto area = getLocalBounds();
    area.reduce(0, 1);

    juce::Colour trackCol;
    if (m_plugin)
        trackCol = m_plugin->isEnabled() ? getTrackColour() : getTrackColour().darker(0.7f);
    else
        trackCol = getTrackColour(); // Modifiers are always enabled effectively or handle it internally

    auto labelingCol = trackCol.getBrightness() > 0.8f ? juce::Colour(0xff000000) : juce::Colour(0xffffffff);

    auto title = m_plugin ? m_plugin->getName() : (m_modifier ? m_modifier->getName() : juce::String(""));

    GUIHelpers::drawHeaderBox(g, area.toFloat(), trackCol, m_evs.m_applicationState.getBorderColour(), m_evs.m_applicationState.getBackgroundColour2(), (float)m_headerWidth, GUIHelpers::HeaderPosition::left, title);

    GUIHelpers::setDrawableOnButton(m_showPluginBtn, BinaryData::expandPluginPlain18_svg, labelingCol);

    if (m_clickOnHeader)
    {
        g.setColour(juce::Colour(0xffffffff));
        g.drawRect(getLocalBounds());
    }

    if (m_isFileDropOver)
    {
        g.setColour(getTrackColour().withAlpha(0.2f));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2.0f), 8.0f);
    }
}

bool PluginChainItemView::isInterestedInDragSource(const SourceDetails &dragSourceDetails) { return m_plugin != nullptr && m_plugin->getPluginType() == SoundFontPlugin::xmlTypeName && dragSourceDetails.description == "FileBrowser" && EngineHelpers::isSoundFontFile(getDraggedBrowserFile(dragSourceDetails)); }

void PluginChainItemView::itemDragMove(const SourceDetails &dragSourceDetails)
{
    m_isFileDropOver = isInterestedInDragSource(dragSourceDetails);
    repaint();
}

void PluginChainItemView::itemDragExit(const SourceDetails &)
{
    m_isFileDropOver = false;
    repaint();
}

void PluginChainItemView::itemDropped(const SourceDetails &dragSourceDetails)
{
    m_isFileDropOver = false;

    if (auto *soundFontPlugin = dynamic_cast<SoundFontPlugin *>(m_plugin.get()))
    {
        const auto draggedFile = getDraggedBrowserFile(dragSourceDetails);
        if (EngineHelpers::isSoundFontFile(draggedFile))
            soundFontPlugin->loadSoundFontFile(draggedFile.getFullPathName());
    }

    repaint();
}

void PluginChainItemView::mouseDown(const juce::MouseEvent &e)
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

            juce::Component::SafePointer<PluginChainItemView> safeThis(this);
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

void PluginChainItemView::mouseDrag(const juce::MouseEvent &e)
{
    if (e.getMouseDownX() < m_headerWidth)
    {
        juce::DragAndDropContainer *dragC = juce::DragAndDropContainer::findParentDragContainerFor(this);
        if (!dragC->isDragAndDropActive())
        {
            auto dragImage = createComponentSnapshot(getLocalBounds());
            juce::Point<int> imageOffset(-e.getMouseDownX(), -e.getMouseDownY());
            dragC->startDragging("PluginComp", this, juce::ScaledImage(dragImage), false, &imageOffset, &e.source);
        }
    }
}

void PluginChainItemView::draggedOntoAutomatableParameterTarget(const te::AutomatableParameter::Ptr &param)
{
    if (m_modifier)
    {
        if (param->getTrack() != m_track.get())
            return;

        param->addModifier(*m_modifier, 0.5f, 0.0f, 0.5f);
    }
}

void PluginChainItemView::mouseUp(const juce::MouseEvent &event)
{
    m_clickOnHeader = false;
    repaint();
}

void PluginChainItemView::mouseDoubleClick(const juce::MouseEvent &e)
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
                auto state = m_evs.getTrackPluginChainViewState(m_track->itemID);
                state.setProperty(getCollapsedStateID(id), m_collapsed, nullptr);
            }
        }

        if (auto *rackView = findParentComponentOfClass<PluginChainView>())
            rackView->resized();
    }
}

void PluginChainItemView::resized()
{
    auto area = getLocalBounds();
    juce::Rectangle<int> showButton = {area.getX(), area.getY() + 5, m_headerWidth, m_headerWidth};
    m_showPluginBtn.setBounds(showButton);
    area.removeFromLeft(m_headerWidth);
    area.removeFromRight(m_headerWidth); // protect rounded rect
    area.reduce(2, 2);

    if (m_presetManager)
        m_presetManager->setBounds(area.removeFromLeft(130));

    if (m_pluginComponent)
        m_pluginComponent->setBounds(area);
    else if (m_modifierComponent)
        m_modifierComponent->setBounds(area);
}

void PluginChainItemView::buttonClicked(juce::Button *button)
{
    if (button == &m_showPluginBtn)
        if (m_plugin)
            m_plugin->showWindowExplicitly();
}

juce::Colour PluginChainItemView::getTrackColour()
{
    if (m_track)
        return m_track->getColour();
    return juce::Colours::grey;
}

int PluginChainItemView::getNeededWidthFactor()
{
    if (m_pluginComponent)
        return m_pluginComponent->getNeededWidth();
    if (m_modifierComponent)
        return m_modifierComponent->getNeededWidth();
    return 1;
}
