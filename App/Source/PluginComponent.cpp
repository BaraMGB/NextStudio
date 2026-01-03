
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/


#include "PluginComponent.h"
#include "Utilities.h"
#include "FourOscPluginComponent.h"
#include "RackView.h"

//==============================================================================
ModifierViewComponent::DragHandle::DragHandle()
{
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void ModifierViewComponent::DragHandle::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2), 2);
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 2, 1);

    // Draw some grip lines
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    auto cx = getWidth() / 2.0f;
    auto cy = getHeight() / 2.0f;
    g.drawLine(cx - 3, cy, cx + 3, cy, 1.0f);
    g.drawLine(cx, cy - 3, cx, cy + 3, 1.0f);
}

void ModifierViewComponent::DragHandle::mouseDown(const juce::MouseEvent& e)
{
    toFront(false);
}

void ModifierViewComponent::DragHandle::mouseDrag(const juce::MouseEvent& e)
{
    if (auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        if (!dragContainer->isDragAndDropActive())
        {
            dragContainer->startDragging(
                te::AutomationDragDropTarget::automatableDragString,
                this,
                juce::Image(juce::Image::ARGB,1,1,true),
                false);
        }

        if (auto* rackView = findParentComponentOfClass<RackView>())
            rackView->repaint();
    }
}

void ModifierViewComponent::DragHandle::mouseUp(const juce::MouseEvent& e)
{
    GUIHelpers::log("mouseUP");
    getParentComponent()->repaint();

    if (auto* rackView = findParentComponentOfClass<RackView>())
    {
        juce::Component::SafePointer<RackView> safeRackView (rackView);
        juce::MessageManager::callAsync ([safeRackView]
        {
            if (safeRackView != nullptr)
                safeRackView->clearDragSource();
        });
    }
}

void ModifierViewComponent::DragHandle::draggedOntoAutomatableParameterTarget (const te::AutomatableParameter::Ptr& param)
{
    if (m_modifier)
    {
        if (param->getOwnerID() == m_modifier->itemID)
        {
            GUIHelpers::log("Can not connect modifier to its own parameters");
            getParentComponent()->repaint();

            if (auto* rackView = findParentComponentOfClass<RackView>())
            {
                juce::Component::SafePointer<RackView> safeRackView (rackView);
                juce::MessageManager::callAsync ([safeRackView]
                                                 {
                                                 if (safeRackView != nullptr)
                                                 safeRackView->clearDragSource();
                                                 });
            }
            return;
        }

        // if the droped modifier not belogs to the track, don't insert.
        if (param->getTrack() != te::getTrackContainingModifier (m_modifier->edit, m_modifier))
        {
            if (auto* rackView = findParentComponentOfClass<RackView>())
            {
                juce::Component::SafePointer<RackView> safeRackView (rackView);
                juce::MessageManager::callAsync ([safeRackView]
                                                 {
                                                 if (safeRackView != nullptr)
                                                 safeRackView->clearDragSource();
                                                 });
            }
            return;
        }

        // Add the modifier to the parameter
        param->addModifier(*m_modifier, 0.5f, 0.0f, 0.5f);

        // update table
        if (auto* parentComponent = findParentComponentOfClass<ModifierViewComponent>())
        {
            parentComponent->m_listBoxModel.update();
            parentComponent->m_table.updateContent();
        }

        // update RackView painting for removing connection line
        if (auto* rackView = findParentComponentOfClass<RackView>())
        {
            juce::Component::SafePointer<RackView> safeRackView (rackView);
            juce::MessageManager::callAsync ([safeRackView]
                                             {
                                             if (safeRackView != nullptr)
                                             safeRackView->clearDragSource();
                                             });
        }
    }
}

int ModifierViewComponent::ConnectedParametersListBoxModel::getNumRows()
{
    return cachedParams.size();
}

void ModifierViewComponent::ConnectedParametersListBoxModel::paintRowBackground (juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll (juce::Colours::lightblue);
    else if (rowNumber % 2)
        g.fillAll (juce::Colours::darkgrey.withAlpha (0.5f));
}

void ModifierViewComponent::ConnectedParametersListBoxModel::paintCell (juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    g.setColour (rowIsSelected ? juce::Colours::black : juce::Colours::white);

    if (auto param = cachedParams[rowNumber])
    {
        g.setFont (12.0f);
        g.drawText (param->getPluginAndParamName(), 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
}

void ModifierViewComponent::ConnectedParametersListBoxModel::cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu menu;
        menu.addItem("Remove Connection", [this, rowNumber] {
            if (auto param = cachedParams[rowNumber])
            {
                auto assignments = param->getAssignments();
                for (auto& assignment : assignments)
                {
                    if (assignment->isForModifierSource(*modifier))
                    {
                        param->removeModifier(*assignment);
                        break;
                    }
                }
                update();
                m_parent.m_table.updateContent();
            }
        });
        menu.show();
    }
}

void ModifierViewComponent::ConnectedParametersListBoxModel::update()
{
    if (modifier)
        cachedParams = te::getAllParametersBeingModifiedBy(edit, *modifier);
}

//==============================================================================
ModifierViewComponent::ModifierViewComponent(EditViewState& evs, te::Modifier::Ptr m)
    : m_editViewState(evs), m_modifier(m), m_listBoxModel(m, evs.m_edit, *this)
{
    m_dragHandle.setModifier(m);
    addAndMakeVisible(m_dragHandle);

    m_table.setModel(&m_listBoxModel);
    m_table.getHeader().addColumn("Connected Parameters", 1, 200);
    m_table.setRowHeight(20);
    addAndMakeVisible(m_table);
    if (m)
    {
        for (auto& param : m->getAutomatableParameters())
        {
            if (param)
            {
                auto parameterComp = std::make_unique<ParameterComponent>(*param, m_editViewState.m_applicationState);
                m_paramListComponent.addAndMakeVisible(parameterComp.get());
                m_parameters.add(std::move(parameterComp));
            }
        }
    }

    addAndMakeVisible(m_viewPort);
    m_viewPort.setViewedComponent(&m_paramListComponent, false);
    m_viewPort.setScrollBarsShown(true, false, true, false);
}

ModifierViewComponent::~ModifierViewComponent()
{
}

void ModifierViewComponent::removeConnection(int rowIndex)
{
    if (auto param = m_listBoxModel.cachedParams[rowIndex])
    {
        auto assignments = param->getAssignments();
        for (auto& assignment : assignments)
        {
            if (assignment->isForModifierSource(*m_modifier))
            {
                param->removeModifier(*assignment);
                break;
            }
        }
        m_listBoxModel.update();
        m_table.updateContent();
    }
}

void ModifierViewComponent::paint(juce::Graphics& g)
{
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour2());
    g.fillAll();
}

void ModifierViewComponent::resized()
{
    int scrollPos = m_viewPort.getVerticalScrollBar().getCurrentRangeStart();
    auto area = getLocalBounds();

    m_dragHandle.setBounds(area.removeFromTop(20).removeFromRight(20));
    m_dragHandle.toFront(false);

    m_table.setBounds(area.removeFromLeft(area.getWidth() / 3));

    m_viewPort.setBounds(area);
    m_viewPort.getVerticalScrollBar().setCurrentRangeStart(scrollPos);
}

//==============================================================================
LFOModifierComponent::LFOModifierComponent(EditViewState& evs, te::Modifier::Ptr m)
    : ModifierViewComponent(evs, m)
    , m_wave (m->getAutomatableParameterByID ("wave"), "Wave")
    , m_sync (m->getAutomatableParameterByID ("syncType"), "Sync")
    , m_rateType (m->getAutomatableParameterByID ("rateType"), "Rate Type")
    , m_bipolar (m->getAutomatableParameterByID ("biopolar"), "Bipolar")
    , m_rate (m->getAutomatableParameterByID ("rate"), "Rate")
    , m_depth (m->getAutomatableParameterByID ("depth"), "Depth")
    , m_phase (m->getAutomatableParameterByID ("phase"), "Phase")
    , m_offset (m->getAutomatableParameterByID ("offset"), "Offset")
{
    // Clear generic UI
    m_parameters.clear();
    m_paramListComponent.removeAllChildren();
    removeChildComponent(&m_viewPort);
    m_viewPort.setViewedComponent(nullptr, false);

    addAndMakeVisible (m_wave);
    addAndMakeVisible (m_sync);
    addAndMakeVisible (m_rate);
    addAndMakeVisible (m_rateType);
    addAndMakeVisible (m_depth);
    addAndMakeVisible (m_bipolar);
    addAndMakeVisible (m_phase);
    addAndMakeVisible (m_offset);
}

void LFOModifierComponent::paint(juce::Graphics& g)
{
    ModifierViewComponent::paint(g);
    auto borderCol = m_editViewState.m_applicationState.getBorderColour();
    auto background2 = m_editViewState.m_applicationState.getBackgroundColour1();

    auto area = getLocalBounds();
    area.removeFromLeft(area.getWidth() / 3);
    auto comboRect = area.removeFromLeft(area.getWidth() / 2);
    comboRect.reduce(3, 5);

    g.setColour(background2);
    GUIHelpers::drawRoundedRectWithSide(g, comboRect.toFloat(), 10, true, true, true, true);
    g.setColour(borderCol);
    GUIHelpers::strokeRoundedRectWithSide(g, comboRect.toFloat(), 10, true, true, true, true);
}

void LFOModifierComponent::resized()
{
    auto area = getLocalBounds();

    m_dragHandle.setBounds(area.removeFromTop(20).removeFromRight(20));
    m_dragHandle.toFront(false);

    m_table.setBounds(area.removeFromLeft(area.getWidth() / 3));

    auto comboRect = area.removeFromLeft(area.getWidth() / 2);
    comboRect.reduce(0, 5);

    auto comboHeight = comboRect.getHeight() / 4;
    m_wave.setBounds(comboRect.removeFromTop(comboHeight));
    m_sync.setBounds(comboRect.removeFromTop(comboHeight));
    m_rateType.setBounds(comboRect.removeFromTop(comboHeight));
    m_bipolar.setBounds(comboRect);


    auto upperKnobs = area.removeFromTop(area.getHeight() / 2);
    auto bottomKnobs = area;
    m_rate.setBounds(upperKnobs.removeFromLeft(upperKnobs.getWidth() / 2));
    m_depth.setBounds(upperKnobs);

    m_phase.setBounds(bottomKnobs.removeFromLeft(bottomKnobs.getWidth() / 2));
    m_offset.setBounds(bottomKnobs);
}

//==============================================================================
RackItemView::RackItemView
    (EditViewState& evs, te::Track::Ptr t, te::Plugin::Ptr p)
    : m_evs (evs), m_track(t), m_plugin (p)
    , m_showPluginBtn( "Show Plugin", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
{
    addAndMakeVisible(m_showPluginBtn);
    m_showPluginBtn.addListener(this);

    name.setText(m_plugin->getName(),juce::NotificationType::dontSendNotification);
    name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(name);
    name.setInterceptsMouseClicks (false, true);
    GUIHelpers::log("PLUGIN TYPE: ",m_plugin->getPluginType());

    if (m_plugin->getPluginType() == "volume")
    {
        m_pluginComponent = std::make_unique<VolumePluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == "4bandEq")
    {
        m_pluginComponent = std::make_unique<EqPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == "delay")
    {
        m_pluginComponent = std::make_unique<DelayPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == "lowpass")
    {
        m_pluginComponent = std::make_unique<FilterPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == "4osc")
    {
        GUIHelpers::log("4OSC");
        m_pluginComponent = std::make_unique<FourOscPluginComponent>(evs, p);
    }
    else if (m_plugin->getPluginType() == te::SamplerPlugin::xmlTypeName)
    {
        GUIHelpers::log(m_plugin->getPluginType());
        if (auto* sampler = dynamic_cast<te::SamplerPlugin*>(p.get()))
        {
            m_pluginComponent = std::make_unique<DrumSamplerView> (evs, *sampler);
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

    if (auto* presetInterface = dynamic_cast<PluginPresetInterface*>(m_pluginComponent.get()))
    {
        m_presetManager = std::make_unique<PresetManagerComponent>(presetInterface);
        addAndMakeVisible(*m_presetManager);
    }
}

RackItemView::RackItemView
    (EditViewState& evs, te::Track::Ptr t, te::Modifier::Ptr m)
    : m_evs (evs), m_track(t), m_modifier (m)
    , m_showPluginBtn( "Show Modifier", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
{
    addAndMakeVisible(m_showPluginBtn);
    m_showPluginBtn.addListener(this);

    // Modifiers don't have user-defined names usually, so we use the class name or type
    name.setText(m->getName(), juce::NotificationType::dontSendNotification); 
    name.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(name);
    name.setInterceptsMouseClicks (false, true);

    if (dynamic_cast<te::LFOModifier*>(m.get()))
        m_modifierComponent = std::make_unique<LFOModifierComponent>(evs, m);
    else
        m_modifierComponent = std::make_unique<ModifierViewComponent>(evs, m);

    addAndMakeVisible(*m_modifierComponent);
}

RackItemView::~RackItemView()
{
    if (m_plugin)
        m_plugin->hideWindowForShutdown ();
}

void RackItemView::paint (juce::Graphics& g)
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
        trackCol = m_plugin->isEnabled () ? getTrackColour() : getTrackColour().darker (0.7f);
    else
        trackCol = getTrackColour(); // Modifiers are always enabled effectively or handle it internally

    auto labelingCol = trackCol.getBrightness() > 0.8f
             ? juce::Colour(0xff000000)
             : juce::Colour(0xffffffff);

    name.setColour(juce::Label::ColourIds::textColourId, labelingCol);

    GUIHelpers::setDrawableOnButton(m_showPluginBtn, BinaryData::expandPluginPlain18_svg ,labelingCol);
    auto header = area.removeFromLeft(m_headerWidth);
    g.setColour(trackCol);
    GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, false, true, false);

    if (m_clickOnHeader)
    {
        g.setColour (juce::Colour(0xffffffff));
        g.drawRect (getLocalBounds ());
    }

    g.setColour(m_evs.m_applicationState.getBorderColour());
    GUIHelpers::strokeRoundedRectWithSide(g, borderRect.toFloat(), cornerSize, true, false, true, false);
}

void RackItemView::mouseDown (const juce::MouseEvent& e)
{
    if (e.getMouseDownX () < m_headerWidth)
    {
        m_clickOnHeader = true;
        if (e.mods.isRightButtonDown())
        {
            juce::PopupMenu m;
            if (m_plugin)
            {
                m.addItem ("Delete", [this] { m_plugin->deleteFromParent(); });
                m.addItem (m_plugin->isEnabled () ? "Disable" : "Enable"
                           , [this] {m_plugin->setEnabled (!m_plugin->isEnabled ());});
            }
            else if (m_modifier)
            {
                m.addItem ("Delete", [this] { m_modifier->remove(); });
            }

            juce::Component::SafePointer<RackItemView> safeThis (this);
            m.show();

            if (safeThis == nullptr)
                return;
        }
    }
    else
    {
        m_clickOnHeader = false;
    }
    repaint ();
}

void RackItemView::mouseDrag(const juce::MouseEvent &e)
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

void RackItemView::draggedOntoAutomatableParameterTarget (const te::AutomatableParameter::Ptr& param)
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
    repaint ();
}

void RackItemView::resized()
{
    auto area = getLocalBounds();
    juce::Rectangle<int> showButton = {area.getX(), area.getY() + 5, m_headerWidth, m_headerWidth};
    m_showPluginBtn.setBounds(showButton);
    auto nameLabelRect = juce::Rectangle<int>(area.getX()
                                              , area.getHeight() - m_headerWidth
                                              , area.getHeight()
                                              , m_headerWidth);
    name.setBounds(nameLabelRect);
    name.setTransform(juce::AffineTransform::rotation ( - (juce::MathConstants<float>::halfPi)
                                                 , nameLabelRect.getX() + 10.0
                                                 , nameLabelRect.getY() + 10.0 ));
    area.removeFromLeft(m_headerWidth);
    area.reduce(1,1);

    if (m_presetManager)
        m_presetManager->setBounds(area.removeFromLeft(130));

    if (m_pluginComponent)
        m_pluginComponent->setBounds(area);
    else if (m_modifierComponent)
        m_modifierComponent->setBounds(area);
}

void RackItemView::buttonClicked(juce::Button* button)
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

juce::ValueTree FilterPluginComponent::getPluginState()
{
    return m_plugin->state.createCopy();
}

juce::ValueTree FilterPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState ("PLUGIN");
    defaultState.setProperty ("type", "lowpass", nullptr);
    defaultState.setProperty (te::IDs::mode, "lowpass", nullptr);
    defaultState.setProperty (te::IDs::frequency, 12000.0, nullptr);
    return defaultState;
}

void FilterPluginComponent::restorePluginState(const juce::ValueTree& state)
{
    m_plugin->restorePluginStateFromValueTree(state);
}

juce::String FilterPluginComponent::getPresetSubfolder() const
{
    return "Filter";
}

juce::String FilterPluginComponent::getPluginTypeName() const
{
    return "lowpass";
}

ApplicationViewState& FilterPluginComponent::getApplicationViewState()
{
    return m_editViewState.m_applicationState;
}



juce::ValueTree EqPluginComponent::getPluginState()
{
    return m_plugin->state.createCopy();
}

juce::ValueTree EqPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState ("PLUGIN");
    defaultState.setProperty ("type", "4bandEq", nullptr);
    return defaultState;
}

void EqPluginComponent::restorePluginState(const juce::ValueTree& state)
{
    m_plugin->restorePluginStateFromValueTree(state);
}

juce::String EqPluginComponent::getPresetSubfolder() const
{
    return "EQ";
}

juce::String EqPluginComponent::getPluginTypeName() const
{
    return "4bandEq";
}

ApplicationViewState& EqPluginComponent::getApplicationViewState()
{
    return m_editViewState.m_applicationState;
}

juce::ValueTree DelayPluginComponent::getPluginState()
{
    return m_plugin->state.createCopy();
}

juce::ValueTree DelayPluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState ("PLUGIN");
    defaultState.setProperty ("type", "delay", nullptr);
    return defaultState;
}

void DelayPluginComponent::restorePluginState(const juce::ValueTree& state)
{
    m_plugin->restorePluginStateFromValueTree(state);
}

juce::String DelayPluginComponent::getPresetSubfolder() const
{
    return "Delay";
}

juce::String DelayPluginComponent::getPluginTypeName() const
{
    return "delay";
}

ApplicationViewState& DelayPluginComponent::getApplicationViewState()
{
    return m_editViewState.m_applicationState;
}

VolumePluginComponent::VolumePluginComponent(EditViewState& evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p)
{
    m_volParComp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("volume"), "Vol");
    addAndMakeVisible(*m_volParComp);
    m_panParComp = std::make_unique<AutomatableParameterComponent>(m_plugin->getAutomatableParameterByID("pan"), "Pan");
    addAndMakeVisible(*m_panParComp);
    m_plugin->state.addListener(this);
}

juce::ValueTree VolumePluginComponent::getPluginState()
{
    return m_plugin->state.createCopy();
}

juce::ValueTree VolumePluginComponent::getFactoryDefaultState()
{
    juce::ValueTree defaultState ("PLUGIN");
    defaultState.setProperty ("type", "volume", nullptr);
    return defaultState;
}

void VolumePluginComponent::restorePluginState(const juce::ValueTree& state)
{
    m_plugin->restorePluginStateFromValueTree(state);
}

juce::String VolumePluginComponent::getPresetSubfolder() const
{
    return "Volume";
}

juce::String VolumePluginComponent::getPluginTypeName() const
{
    return "volume";
}

ApplicationViewState& VolumePluginComponent::getApplicationViewState()
{
    return m_editViewState.m_applicationState;
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
    , m_plugin(p)
{
    if (p)
    {
        for (auto& param : p->getAutomatableParameters())
        {
            if (param)
            {
                param->addListener(this);
                auto parameterComp = std::make_unique<ParameterComponent>(*param, m_editViewState.m_applicationState);
                m_pluginListComponent.addAndMakeVisible(parameterComp.get());
                m_parameters.add(std::move(parameterComp));
            }
        }
    }

    addAndMakeVisible(m_viewPort);

    if (p->getAutomatableParameter(0))
    {
        m_lastChangedParameterComponent
                = std::make_unique<ParameterComponent>(
                                            *(p->getAutomatableParameter(0)), m_editViewState.m_applicationState);
        addAndMakeVisible(*m_lastChangedParameterComponent);
    }
    m_viewPort.setViewedComponent(&m_pluginListComponent, true);
    m_viewPort.setScrollBarsShown(true, false, true, false);
}

VstPluginComponent::~VstPluginComponent()
{
    m_lastChangedParameterComponent.reset();
    if (m_plugin)
    {
        for (auto & param : m_plugin->getAutomatableParameters())
        {
            if (param)
            {
                param->removeListener(this);
            }
        }
    }
}

void VstPluginComponent::paint (juce::Graphics& g) 
{
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour2());
    g.fillAll();
    if (m_lastChangedParameterComponent)
    {
        auto h = m_lastChangedParameterComponent->getHeight();
        g.setColour(juce::Colours::black);
        g.drawLine(0, h, getWidth(), h);
    }
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
                                    ,m_pluginListComponent.getChildren().size() * widgetHeight);

    auto pcb = m_pluginListComponent.getBounds();
    for (auto & pc : m_parameters)
    {
        pc->setBounds(pcb.removeFromTop(widgetHeight));
    }
    m_viewPort.getVerticalScrollBar().setCurrentRangeStart(scrollPos);
}

void VstPluginComponent::parameterChanged (te::AutomatableParameter& param, float) 
{
    if (!m_lastChangedParameterComponent->isDragged() && &m_lastChangedParameterComponent->getParameter() != &param)
    {
        removeChildComponent(m_lastChangedParameterComponent.get());
        m_lastChangedParameterComponent = std::make_unique<ParameterComponent>(param, m_editViewState.m_applicationState);
        addAndMakeVisible(m_lastChangedParameterComponent.get());
        resized();
    }
}

ApplicationViewState& VstPluginComponent::getApplicationViewState()
{
    return m_editViewState.m_applicationState;
}

juce::ValueTree VstPluginComponent::getPluginState()
{
    return m_plugin->state.createCopy();
}

juce::ValueTree VstPluginComponent::getFactoryDefaultState()
{
    if (auto* ep = dynamic_cast<te::ExternalPlugin*>(m_plugin.get()))
    {
        juce::ValueTree state("PLUGIN");
        state.setProperty("type", ep->desc.pluginFormatName + juce::String::toHexString(ep->desc.deprecatedUid).toUpperCase(), nullptr);
        return state;
    }

    return {};
}

void VstPluginComponent::restorePluginState(const juce::ValueTree& state)
{
    m_plugin->restorePluginStateFromValueTree(state);
}

juce::String VstPluginComponent::getPresetSubfolder() const
{
    if (auto* ep = dynamic_cast<te::ExternalPlugin*>(m_plugin.get()))
        if (ep->desc.manufacturerName.isNotEmpty())
            return ep->desc.manufacturerName;

    return "External";
}

juce::String VstPluginComponent::getPluginTypeName() const
{
    if (auto* ep = dynamic_cast<te::ExternalPlugin*>(m_plugin.get()))
        return ep->desc.pluginFormatName + juce::String::toHexString(ep->desc.deprecatedUid).toUpperCase();

    return "unknown";
}

//------------------------------------------------------------------------------

ParameterComponent::ParameterComponent(tracktion_engine::AutomatableParameter &ap, ApplicationViewState& appstate)
    : m_parameter(ap)
    , m_appState(appstate)
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

void ParameterComponent::paint(juce::Graphics& g) 
{
    g.setColour(m_appState.getBackgroundColour1());
    auto area = getLocalBounds();
    area.reduce(2, 2);
    area.removeFromRight(10);
    GUIHelpers::drawRoundedRectWithSide(g, area.toFloat(), 10, true, false, true,  false);
}
void ParameterComponent::resized()
{
    auto area = getLocalBounds();

    m_parameterSlider.setBounds(area.removeFromLeft(area.getHeight()));
    m_parameterName.setBounds(area);
}

void ParameterComponent::mouseDown(const juce::MouseEvent &e)
{
    m_isDragged = true;
}


void ParameterComponent::mouseUp(const juce::MouseEvent& e) 
{
    m_isDragged = false;
}

