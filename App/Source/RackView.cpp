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


#include "RackView.h"
#include "PluginMenu.h"
#include "InstrumentEffectChooser.h"
#include "Utilities.h"


RackView::RackView (EditViewState& evs)
    : m_evs (evs)
{
    addAndMakeVisible(m_nameLabel);
    m_nameLabel.setJustificationType(juce::Justification::centred);
}

RackView::~RackView()
{
    for (auto &b : m_addButtons)
    {
        b->removeListener(this);
    }

    if (m_track)
        m_track->state.removeListener(this);
}

void RackView::paint (juce::Graphics& g)
{
    g.setColour (m_evs.m_applicationState.getBackgroundColour1());
    g.fillRoundedRectangle(getLocalBounds().withTrimmedLeft (2).toFloat(), 10);
    g.setColour(juce::Colours::white);

    auto area = getLocalBounds();

    if (m_isOver)
        g.drawRect(area, 2);

    if (m_track == nullptr)
    {
        g.drawText("select a track for showing rack", getLocalBounds(), juce::Justification::centred);
    }
    else
    {
        auto trackCol = m_track->getColour();
        auto cornerSize = 10.0f;
        auto labelingCol = trackCol.getBrightness() > 0.8f
                 ? juce::Colour(0xff000000)
                 : juce::Colour(0xffffffff);

        m_nameLabel.setColour(juce::Label::ColourIds::textColourId, labelingCol);
        // GUIHelpers::setDrawableOnButton(m_showPluginBtn, BinaryData::expandPluginPlain18_svg ,"#" + labelingCol.toString().substring(2));
        auto header = area.removeFromLeft(HEADERWIDTH);
        g.setColour(trackCol);
        GUIHelpers::drawRoundedRectWithSide(g, header.toFloat(), cornerSize, true, false, true, false);
    };
}

void RackView::paintOverChildren (juce::Graphics& g)
{
    auto* dragC = juce::DragAndDropContainer::findParentDragContainerFor(this);
    if (!dragC || !dragC->isDragAndDropActive())
    {
        m_dragSource = nullptr;
        return;
    }
    if (m_dragSource == nullptr)
        return;

    auto modifier = dynamic_cast<ModifierViewComponent*>(m_dragSource->getParentComponent());
    if (modifier == nullptr)
        return;

    auto mousePos = getMouseXYRelative().toFloat();
    auto sourceBounds = getLocalPoint(m_dragSource, m_dragSource->getLocalBounds().getCentre()).toFloat();

    auto* compUnderMouse = getComponentAt(getMouseXYRelative());
    if (compUnderMouse == this)
        compUnderMouse = nullptr;

    juce::Colour lineColour = m_evs.m_applicationState.getTextColour();

    if (compUnderMouse != nullptr)
    {
        auto* slider = dynamic_cast<AutomatableSliderComponent*>(compUnderMouse);
        if (slider == nullptr)
            slider = compUnderMouse->findParentComponentOfClass<AutomatableSliderComponent>();

        if (slider != nullptr)
        {
            auto param = slider->getAutomatableParameter();
            auto mod = modifier->getModifier();

            if (mod->itemID == param->getOwnerID() || param->getTrack() != te::getTrackContainingModifier (mod->edit, mod))
            {
                lineColour = juce::Colours::grey;
                g.setColour(juce::Colours::grey.withAlpha(0.4f));
                auto bounds = getLocalPoint(slider, juce::Point<int>(0,0));
                auto rect = juce::Rectangle<int>(bounds.getX(), bounds.getY(), slider->getWidth(), slider->getHeight());

                int size = std::min(rect.getWidth(), rect.getHeight());
                rect = rect.withSizeKeepingCentre(size, size);

                g.fillRect(rect);
                g.setColour(juce::Colours::black);
                g.drawLine(rect.getX(), rect.getY(), rect.getBottomRight().getX(), rect.getBottomRight().getY(), 2.f);
                g.drawLine(rect.getTopRight().getX(), rect.getTopRight().getY(), rect.getBottomLeft().getX(), rect.getBottomLeft().getY(), 2.f);
            }
            else
            {
                lineColour = m_evs.m_applicationState.getPrimeColour();
                g.setColour(lineColour);
                auto bounds = getLocalPoint(slider, juce::Point<int>(0,0));
                auto rect = juce::Rectangle<int>(bounds.getX(), bounds.getY(), slider->getWidth(), slider->getHeight());

                int size = std::min(rect.getWidth(), rect.getHeight());
                rect = rect.withSizeKeepingCentre(size, size);

                g.drawRect(rect, 2);
            }
        }
    }

    g.setColour(lineColour);
    g.drawLine(sourceBounds.getX(), sourceBounds.getY(), mousePos.getX(), mousePos.getY(), 2.0f);
}

void RackView::mouseDown (const juce::MouseEvent&)
{
    //editViewState.selectionManager.selectOnly (track.get());
}

void RackView::resized()
{
    auto area = getLocalBounds();
    auto nameLabelRect = juce::Rectangle<int>(area.getX()
                                              , area.getHeight() - HEADERWIDTH 
                                              , area.getHeight()
                                              , HEADERWIDTH);
    m_nameLabel.setBounds(nameLabelRect);
    m_nameLabel.setTransform(juce::AffineTransform::rotation ( - (juce::MathConstants<float>::halfPi)
                                                 , nameLabelRect.getX() + 10.0
                                                 , nameLabelRect.getY() + 10.0 ));
    area.removeFromLeft(HEADERWIDTH);
    area = area.reduced (5);

    for (auto &b : m_addButtons)
    {
        b->removeListener(this);
    }
    m_addButtons.clear();

    if (m_track != nullptr)
    {
        auto firstAdder = std::make_unique<AddButton>(m_track, m_evs.m_applicationState);
        addAndMakeVisible(firstAdder.get());
        firstAdder->addListener(this);
        firstAdder->setButtonText("+");
        firstAdder->setBounds(area.removeFromLeft(15).reduced(0,10));
        m_addButtons.add(std::move(firstAdder));

        for (auto p : m_rackItems)
        {
            area.removeFromLeft (5);
            p->setBounds (area.removeFromLeft((area.getHeight() * p->getNeededWidthFactor()) / 2 ));

            area.removeFromLeft(5);

            auto adder = std::make_unique<AddButton>(m_track, m_evs.m_applicationState);

            if (p->getPlugin())
                adder->setPlugin(p->getPlugin());

            addAndMakeVisible(adder.get());
            adder->setButtonText("+");
            adder->setBounds(area.removeFromLeft(15).reduced(0, 10));
            adder->addListener(this);
            m_addButtons.add(std::move(adder));
        }
        area.removeFromLeft (5);
    }
}

juce::StringArray RackView::getRackOrder() const
{
    auto state = m_evs.getTrackRackViewState(m_track->itemID);
    juce::String orderString = state.getProperty("rackItemOrder").toString();
    juce::StringArray order;
    order.addTokens(orderString, ",", "");
    return order;
}

void RackView::saveRackOrder(const juce::StringArray& order)
{
    auto state = m_evs.getTrackRackViewState(m_track->itemID);
    state.setProperty("rackItemOrder", order.joinIntoString(","), nullptr);
}

static te::Plugin::Ptr getPluginFromList(te::PluginList& list, te::EditItemID id)
{
    for (auto p : list)
        if (p->itemID == id)
            return p;
    return {};
}

static bool isPluginHidden(te::Track& t, te::Plugin* p)
{
    int index = t.pluginList.indexOf(p);
    // Exclude internal hidden plugins (usually at the end: Vol+Pan and LevelMeter)
    return (t.pluginList.size() >= 2 && index >= t.pluginList.size() - 2);
}

int RackView::getPluginIndexForVisualIndex(int visualIndex) const
{
    auto order = getRackOrder();
    int targetPluginIndex = 0;
    for(int i=0; i<visualIndex && i<order.size(); ++i)
    {
        if (getPluginFromList(m_track->pluginList, te::EditItemID::fromVar(order[i])))
            targetPluginIndex++;
    }
    return targetPluginIndex;
}

void RackView::ensureRackOrderConsistency()
{
    auto currentOrder = getRackOrder();
    juce::StringArray newOrder;

    // 1. Keep existing items if they still exist and are not hidden
    for (auto idStr : currentOrder)
    {
        auto id = te::EditItemID::fromVar(idStr);
        if (auto p = getPluginFromList(m_track->pluginList, id))
        {
            if (!isPluginHidden(*m_track, p.get()))
                newOrder.add(idStr);
        }
        else if (auto* ml = m_track->getModifierList())
        {
             if (te::findModifierForID(*ml, id) != nullptr)
             {
                 newOrder.add(idStr);
             }
        }
    }

    // 2. Add any new items not in the list
    for (auto* p : m_track->getAllPlugins())
    {
        if (isPluginHidden(*m_track, p))
            continue;

        if (!newOrder.contains(p->itemID.toString()))
            newOrder.add(p->itemID.toString());
    }

    if (auto* ml = m_track->getModifierList())
    {
        for (auto m : ml->getModifiers())
        {
            if (!newOrder.contains(m->itemID.toString()))
                newOrder.add(m->itemID.toString());
        }
    }

    if (newOrder != currentOrder)
        saveRackOrder(newOrder);
}

void RackView::moveItem(RackItemView* item, int targetIndex)
{
    te::EditItemID id;
    if (item->getPlugin()) id = item->getPlugin()->itemID;
    else if (item->getModifier()) id = item->getModifier()->itemID;

    if (id.isValid())
    {
        auto order = getRackOrder();
        order.removeString(id.toString());

        if (targetIndex >= order.size())
            order.add(id.toString());
        else
            order.insert(targetIndex, id.toString());

        saveRackOrder(order);

        // Sync PluginList order
        if (item->getPlugin())
        {
             int targetPluginIndex = getPluginIndexForVisualIndex(targetIndex);

             m_track->pluginList.insertPlugin(item->getPlugin(), targetPluginIndex, nullptr);
        }

        rebuildView();
    }
}

void RackView::buttonClicked(juce::Button* button)
{
    for (auto &b : m_addButtons)
    {
        if (b == button)
        {
            int visualIndex = m_addButtons.indexOf(b);

            juce::PopupMenu m;
            m.addItem(1, "Plugins...");

            juce::PopupMenu modMenu;
            modMenu.addItem(100, "LFO");
            modMenu.addItem(101, "Envelope Follower");
            modMenu.addItem(102, "Step");
            modMenu.addItem(103, "Random");

            m.addSubMenu("Modifiers", modMenu);

            int result = m.showAt(button);

            if (result == 1)
            {
                if (auto plugin = showMenuAndCreatePlugin (m_track->edit))
                {
                    // Calculate target Plugin List Index based on Rack Order
                    ensureRackOrderConsistency();

                    auto order = getRackOrder();
                    int targetPluginIndex = 0;

                    for(int i=0; i<visualIndex && i<order.size(); ++i)
                    {
                        if (getPluginFromList(m_track->pluginList, te::EditItemID::fromVar(order[i])))
                            targetPluginIndex++;
                    }

                    EngineHelpers::insertPlugin (m_track, plugin, targetPluginIndex);

                    // Update Rack Order
                    order.insert(visualIndex, plugin->itemID.toString());
                    saveRackOrder(order);
                }
            }
            else if (result >= 100)
            {
                 if (auto* ml = m_track->getModifierList())
                 {
                     juce::Identifier id = te::IDs::LFO;
                     if (result == 101) id = te::IDs::ENVELOPEFOLLOWER;
                     if (result == 102) id = te::IDs::STEP;
                     if (result == 103) id = te::IDs::RANDOM;

                     auto mod = ml->insertModifier(juce::ValueTree(id), -1, nullptr);

                     if (mod)
                     {
                         // ensureRackOrderConsistency(); // Don't call here to avoid race/duplication, or handle removal
                         // Better to just manipulate order directly
                         auto order = getRackOrder();

                         // Remove if it was accidentally added by a sync listener (unlikely this fast, but safe)
                         order.removeString(mod->itemID.toString());

                         if (visualIndex >= order.size())
                             order.add(mod->itemID.toString());
                         else
                             order.insert(visualIndex, mod->itemID.toString());

                         saveRackOrder(order);
                     }
                 }
            }

            m_evs.m_selectionManager.selectOnly (m_track);
        }
    }
}

void RackView::setTrack(te::Track::Ptr track)
{
    m_track = track;
    m_track->state.addListener(this);
    m_trackID = m_track->itemID.toString();
    m_nameLabel.setText(m_track->getName(), juce::dontSendNotification);
    rebuildView();
}

void RackView::clearTrack()
{
    if (m_track != nullptr)
        m_track->state.removeListener(this);
    m_track = nullptr;
    m_trackID = "";
    rebuildView();
}

juce::String RackView::getCurrentTrackID()
{
    return m_trackID;
}

juce::OwnedArray<AddButton> & RackView::getAddButtons()
{
    return m_addButtons;
}

juce::OwnedArray<RackItemView> & RackView::getPluginComponents()
{
    return m_rackItems;
}
    
void RackView::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& c)
{
    if (c.hasType (te::IDs::PLUGIN) || te::ModifierList::isModifier(c.getType()))
        markAndUpdate (m_updatePlugins);
}

void RackView::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree& c, int)
{
    if (c.hasType (te::IDs::PLUGIN) || te::ModifierList::isModifier(c.getType()))
        markAndUpdate (m_updatePlugins);
}

void RackView::valueTreeChildOrderChanged (juce::ValueTree& c, int, int)
{
    if (c.hasType (te::IDs::PLUGIN) || te::ModifierList::isModifier(c.getType()))
        markAndUpdate (m_updatePlugins);
}

void RackView::handleAsyncUpdate()
{
    if (compareAndReset (m_updatePlugins))
        rebuildView();
}

void RackView::rebuildView()
{
    m_rackItems.clear();

    if (m_track != nullptr)
    {
        ensureRackOrderConsistency();
        auto order = getRackOrder();

        for (auto idStr : order)
        {
            auto id = te::EditItemID::fromVar(idStr);

            if (auto p = getPluginFromList(m_track->pluginList, id))
            {
                 auto view = std::make_unique<RackItemView> (m_evs, m_track, p);
                 addAndMakeVisible (view.get());
                 m_rackItems.add (std::move(view));
            }
            else if (auto* ml = m_track->getModifierList())
            {
                if (auto m = te::findModifierForID(*ml, id))
                {
                     auto view = std::make_unique<RackItemView> (m_evs, m_track, m);
                     addAndMakeVisible (view.get());
                     m_rackItems.add (std::move(view));
                }
            }
        }
    }

    resized();
}

bool RackView::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry"
     || dragSourceDetails.description == "Instrument or Effect"
     || dragSourceDetails.description == te::AutomationDragDropTarget::automatableDragString)
    {
        return true;
    }
    return false;
}

void RackView::itemDragMove(const SourceDetails& dragSourceDetails) 
{
    // Check if the dragged item is a Plugin component, a list entry, a browser item,
    // or an automatable modifier (which uses automatableDragString).
    // This ensures we accept drops from both standard plugins and Modifiers.
    if (dragSourceDetails.description == "PluginComp"
        || dragSourceDetails.description == "PluginListEntry"
        || dragSourceDetails.description == "Instrument or Effect"
        || dragSourceDetails.description == te::AutomationDragDropTarget::automatableDragString)

    {
        m_isOver = true;
    }

    if (dragSourceDetails.description == te::AutomationDragDropTarget::automatableDragString)
    {
        m_dragSource = dragSourceDetails.sourceComponent.get();
    }
    repaint();
}

void RackView::itemDragExit (const SourceDetails& details) 
{
    m_isOver = false;

    if (details.description != te::AutomationDragDropTarget::automatableDragString)
        m_dragSource = nullptr;

    repaint();
}

void RackView::itemDropped(
    const juce::DragAndDropTarget::SourceDetails& details)
{
    m_dragSource = nullptr;

    te::Track::Ptr track;
    if (m_track != nullptr)
        track = m_track;
    else 
        track = EngineHelpers::addAudioTrack(true, m_evs.m_applicationState.getRandomTrackColour(), m_evs);

    if  (details.description == "PluginListEntry")
        if (auto listbox = dynamic_cast<PluginListbox*>(details.sourceComponent.get ()))
            EngineHelpers::insertPlugin (track, listbox->getSelectedPlugin(m_evs.m_edit));
    if (details.description == "Instrument or Effect")
        if (auto lb = dynamic_cast<InstrumentEffectTable*>(details.sourceComponent.get()))
            EngineHelpers::insertPlugin (track, lb->getSelectedPlugin(m_evs.m_edit));

    m_isOver = false;
    repaint();
}
void AddButton::itemDropped(const SourceDetails& dragSourceDetails) 
{
    if (dragSourceDetails.description == "PluginListEntry")
    {
        if (auto listbox = dynamic_cast<juce::ListBox*>(dragSourceDetails.sourceComponent.get ()))
        {
            if (auto lbm = dynamic_cast<PluginListbox*> (listbox->getModel ()))
            {
                auto pluginRackComp = dynamic_cast<RackView*>(getParentComponent());
                if (pluginRackComp)
                {
                    // Calculate the visual index where the user dropped the item
                    int visualIndex = pluginRackComp->getAddButtons().indexOf(this);

                    // Ensure the internal rack order state matches the current reality before modifying it
                    pluginRackComp->ensureRackOrderConsistency();
                    auto order = pluginRackComp->getRackOrder();

                    int targetPluginIndex = pluginRackComp->getPluginIndexForVisualIndex(visualIndex);

                    auto plugin = lbm->getSelectedPlugin(m_track->edit);
                    if (plugin)
                    {
                        // Insert the plugin into the engine at the calculated index
                        EngineHelpers::insertPlugin (m_track, plugin, targetPluginIndex);

                        // Update the custom rack order to include the new plugin's ID at the correct visual position
                        order.insert(visualIndex, plugin->itemID.toString());
                        pluginRackComp->saveRackOrder(order);
                    }
                }
            }

        }
    }


    if (dragSourceDetails.description == "PluginComp" 
        || dragSourceDetails.description == te::AutomationDragDropTarget::automatableDragString)
    {
        auto pluginRackComp = dynamic_cast<RackView*>(getParentComponent());
        if (pluginRackComp)
        {
            auto* view = dynamic_cast<RackItemView*>(dragSourceDetails.sourceComponent.get());
            if (view)
            {
                int targetIndex = pluginRackComp->getAddButtons().indexOf(this);
                pluginRackComp->moveItem(view, targetIndex);
            }
        }
    }
}
