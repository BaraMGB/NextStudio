/*
  ==============================================================================

    ModifierViewComponent.cpp
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "LowerRange/PluginChain/ModifierViewComponent.h"
#include "Utilities/Utilities.h"
#include "LowerRange/PluginChain/RackView.h"

//==============================================================================
ModifierViewComponent::DragHandle::DragHandle() { setMouseCursor(juce::MouseCursor::DraggingHandCursor); }

void ModifierViewComponent::DragHandle::paint(juce::Graphics &g)
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

void ModifierViewComponent::DragHandle::mouseDown(const juce::MouseEvent &e) { toFront(false); }

void ModifierViewComponent::DragHandle::mouseDrag(const juce::MouseEvent &e)
{
    if (auto *dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this))
    {
        if (!dragContainer->isDragAndDropActive())
        {
            dragContainer->startDragging(te::AutomationDragDropTarget::automatableDragString, this, juce::Image(juce::Image::ARGB, 1, 1, true), false);
        }

        if (auto *rackView = findParentComponentOfClass<RackView>())
            rackView->repaint();
    }
}

void ModifierViewComponent::DragHandle::mouseUp(const juce::MouseEvent &e)
{
    GUIHelpers::log("mouseUP");
    getParentComponent()->repaint();

    if (auto *rackView = findParentComponentOfClass<RackView>())
    {
        juce::Component::SafePointer<RackView> safeRackView(rackView);
        juce::MessageManager::callAsync(
            [safeRackView]
            {
                if (safeRackView != nullptr)
                    safeRackView->clearDragSource();
            });
    }
}

void ModifierViewComponent::DragHandle::draggedOntoAutomatableParameterTarget(const te::AutomatableParameter::Ptr &param)
{
    if (m_modifier)
    {
        if (param->getOwnerID() == m_modifier->itemID)
        {
            GUIHelpers::log("Can not connect modifier to its own parameters");
            getParentComponent()->repaint();

            if (auto *rackView = findParentComponentOfClass<RackView>())
            {
                juce::Component::SafePointer<RackView> safeRackView(rackView);
                juce::MessageManager::callAsync(
                    [safeRackView]
                    {
                        if (safeRackView != nullptr)
                            safeRackView->clearDragSource();
                    });
            }
            return;
        }

        // if the droped modifier not belogs to the track, don't insert.
        if (param->getTrack() != te::getTrackContainingModifier(m_modifier->edit, m_modifier))
        {
            if (auto *rackView = findParentComponentOfClass<RackView>())
            {
                juce::Component::SafePointer<RackView> safeRackView(rackView);
                juce::MessageManager::callAsync(
                    [safeRackView]
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
        if (auto *parentComponent = findParentComponentOfClass<ModifierViewComponent>())
        {
            parentComponent->m_listBoxModel.update();
            parentComponent->m_table.updateContent();
        }

        // update RackView painting for removing connection line
        if (auto *rackView = findParentComponentOfClass<RackView>())
        {
            juce::Component::SafePointer<RackView> safeRackView(rackView);
            juce::MessageManager::callAsync(
                [safeRackView]
                {
                    if (safeRackView != nullptr)
                        safeRackView->clearDragSource();
                });
        }
    }
}

int ModifierViewComponent::ConnectedParametersListBoxModel::getNumRows() { return cachedParams.size(); }

void ModifierViewComponent::ConnectedParametersListBoxModel::paintRowBackground(juce::Graphics &g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colours::lightblue);
    else if (rowNumber % 2)
        g.fillAll(juce::Colours::darkgrey.withAlpha(0.5f));
}

void ModifierViewComponent::ConnectedParametersListBoxModel::paintCell(juce::Graphics &g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    g.setColour(rowIsSelected ? juce::Colours::black : juce::Colours::white);

    if (auto param = cachedParams[rowNumber])
    {
        g.setFont(12.0f);
        g.drawText(param->getPluginAndParamName(), 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
}

void ModifierViewComponent::ConnectedParametersListBoxModel::cellClicked(int rowNumber, int columnId, const juce::MouseEvent &e)
{
    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu menu;
        menu.addItem("Remove Connection",
                     [this, rowNumber]
                     {
                         if (auto param = cachedParams[rowNumber])
                         {
                             auto assignments = param->getAssignments();
                             for (auto &assignment : assignments)
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
ModifierViewComponent::ModifierViewComponent(EditViewState &evs, te::Modifier::Ptr m)
    : m_editViewState(evs),
      m_modifier(m),
      m_listBoxModel(m, evs.m_edit, *this)
{
    m_dragHandle.setModifier(m);
    addAndMakeVisible(m_dragHandle);

    m_table.setModel(&m_listBoxModel);
    m_table.getHeader().addColumn("Connected Parameters", 1, 200);
    m_table.setRowHeight(20);
    addAndMakeVisible(m_table);
    if (m)
    {
        for (auto &param : m->getAutomatableParameters())
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

ModifierViewComponent::~ModifierViewComponent() {}

void ModifierViewComponent::removeConnection(int rowIndex)
{
    if (auto param = m_listBoxModel.cachedParams[rowIndex])
    {
        auto assignments = param->getAssignments();
        for (auto &assignment : assignments)
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

void ModifierViewComponent::paint(juce::Graphics &g)
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
LFOModifierComponent::LFOModifierComponent(EditViewState &evs, te::Modifier::Ptr m)
    : ModifierViewComponent(evs, m),
      m_wave(m->getAutomatableParameterByID("wave"), "Wave"),
      m_sync(m->getAutomatableParameterByID("syncType"), "Sync"),
      m_rateType(m->getAutomatableParameterByID("rateType"), "Rate Type"),
      m_bipolar(m->getAutomatableParameterByID("biopolar"), "Bipolar"),
      m_rate(m->getAutomatableParameterByID("rate"), "Rate"),
      m_depth(m->getAutomatableParameterByID("depth"), "Depth"),
      m_phase(m->getAutomatableParameterByID("phase"), "Phase"),
      m_offset(m->getAutomatableParameterByID("offset"), "Offset")
{
    // Clear generic UI
    m_parameters.clear();
    m_paramListComponent.removeAllChildren();
    removeChildComponent(&m_viewPort);
    m_viewPort.setViewedComponent(nullptr, false);

    addAndMakeVisible(m_wave);
    addAndMakeVisible(m_sync);
    addAndMakeVisible(m_rate);
    addAndMakeVisible(m_rateType);
    addAndMakeVisible(m_depth);
    addAndMakeVisible(m_bipolar);
    addAndMakeVisible(m_phase);
    addAndMakeVisible(m_offset);
}

void LFOModifierComponent::paint(juce::Graphics &g)
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
StepModifierComponent::StepDisplay::StepDisplay(te::StepModifier &m)
    : m_modifier(m)
{
    updateSteps();
    startTimerHz(30);
}

void StepModifierComponent::StepDisplay::paint(juce::Graphics &g)
{
    if (m_currentStep >= 0 && m_currentStep < m_sliders.size())
    {
        if (auto *s = m_sliders[m_currentStep])
        {
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.fillRect(s->getBounds());
        }
    }
}

void StepModifierComponent::StepDisplay::resized()
{
    auto area = getLocalBounds();
    int num = m_sliders.size();
    if (num > 0)
    {
        int w = area.getWidth() / num;
        for (int i = 0; i < num; ++i)
        {
            if (auto *s = m_sliders[i])
                s->setBounds(area.removeFromLeft(w).reduced(1, 0));
        }
    }
}

void StepModifierComponent::StepDisplay::sliderValueChanged(juce::Slider *slider)
{
    int index = m_sliders.indexOf(slider);
    if (index >= 0)
        m_modifier.setStep(index, (float)slider->getValue());
}

void StepModifierComponent::StepDisplay::timerCallback()
{
    int newStep = m_modifier.getCurrentStep();
    if (newStep != m_currentStep)
    {
        m_currentStep = newStep;
        repaint();
    }

    // Also check if numSteps has changed to update slider count
    int num = (int)m_modifier.numStepsParam->getCurrentValue();
    if (num != m_sliders.size())
        updateSteps();
}

void StepModifierComponent::StepDisplay::updateSteps()
{
    int num = (int)m_modifier.numStepsParam->getCurrentValue();
    m_sliders.clear();
    for (int i = 0; i < num; ++i)
    {
        auto s = new juce::Slider();
        s->setSliderStyle(juce::Slider::LinearBarVertical);
        s->setRange(-1.0, 1.0);
        s->setValue(m_modifier.getStep(i), juce::dontSendNotification);
        s->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        s->addListener(this);
        addAndMakeVisible(s);
        m_sliders.add(s);
    }
    resized();
}

//==============================================================================
StepModifierComponent::StepModifierComponent(EditViewState &evs, te::Modifier::Ptr m)
    : ModifierViewComponent(evs, m),
      m_sync(m->getAutomatableParameterByID("syncType"), "Sync"),
      m_rateType(m->getAutomatableParameterByID("rateType"), "Rate Type"),
      m_numSteps(m->getAutomatableParameterByID("numSteps"), "Steps"),
      m_rate(m->getAutomatableParameterByID("rate"), "Rate"),
      m_depth(m->getAutomatableParameterByID("depth"), "Depth"),
      m_stepDisplay(*dynamic_cast<te::StepModifier *>(m.get()))
{
    // Clear generic UI
    m_parameters.clear();
    m_paramListComponent.removeAllChildren();
    removeChildComponent(&m_viewPort);
    m_viewPort.setViewedComponent(nullptr, false);

    addAndMakeVisible(m_sync);
    addAndMakeVisible(m_rateType);
    addAndMakeVisible(m_numSteps);
    addAndMakeVisible(m_rate);
    addAndMakeVisible(m_depth);
    addAndMakeVisible(m_stepDisplay);
}

void StepModifierComponent::paint(juce::Graphics &g)
{
    ModifierViewComponent::paint(g);
    auto borderCol = m_editViewState.m_applicationState.getBorderColour();
    auto background2 = m_editViewState.m_applicationState.getBackgroundColour1();

    auto area = getLocalBounds();
    area.removeFromLeft(area.getWidth() / 3);
    auto topPart = area.removeFromTop(area.getHeight() / 2);

    auto comboRect = topPart.removeFromLeft(topPart.getWidth() / 3);
    comboRect.reduce(3, 5);

    g.setColour(background2);
    GUIHelpers::drawRoundedRectWithSide(g, comboRect.toFloat(), 10, true, true, true, true);
    g.setColour(borderCol);
    GUIHelpers::strokeRoundedRectWithSide(g, comboRect.toFloat(), 10, true, true, true, true);

    auto displayRect = area.reduced(3, 5);
    g.setColour(background2);
    GUIHelpers::drawRoundedRectWithSide(g, displayRect.toFloat(), 10, true, true, true, true);
    g.setColour(borderCol);
    GUIHelpers::strokeRoundedRectWithSide(g, displayRect.toFloat(), 10, true, true, true, true);
}

void StepModifierComponent::resized()
{
    auto area = getLocalBounds();

    m_dragHandle.setBounds(area.removeFromTop(20).removeFromRight(20));
    m_dragHandle.toFront(false);

    m_table.setBounds(area.removeFromLeft(area.getWidth() / 3));

    auto controls = area.removeFromTop(area.getHeight() / 2);
    auto comboRect = controls.removeFromLeft(controls.getWidth() / 3);
    comboRect.reduce(0, 5);
    auto comboHeight = comboRect.getHeight() / 2;
    m_sync.setBounds(comboRect.removeFromTop(comboHeight));
    m_rateType.setBounds(comboRect);

    int kw = controls.getWidth() / 3;
    m_numSteps.setBounds(controls.removeFromLeft(kw));
    m_rate.setBounds(controls.removeFromLeft(kw));
    m_depth.setBounds(controls);

    m_stepDisplay.setBounds(area.reduced(5));
}
