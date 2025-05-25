
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
    rebuildView();
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
    g.setColour (m_evs.m_applicationState.getMenuBackgroundColour());
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
        auto firstAdder = std::make_unique<AddButton>(m_track);
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

            auto adder = std::make_unique<AddButton>(m_track);
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

void RackView::buttonClicked(juce::Button* button)
{
    for (auto &b : m_addButtons)
    {
        if (b == button)
        {
            if (auto plugin = showMenuAndCreatePlugin (m_track->edit))
                EngineHelpers::insertPlugin (m_track, plugin, m_addButtons.indexOf (b));

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
    if (c.hasType (te::IDs::PLUGIN))
        markAndUpdate (m_updatePlugins);
}

void RackView::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree& c, int)
{
    if (c.hasType (te::IDs::PLUGIN))
        markAndUpdate (m_updatePlugins);
}

void RackView::valueTreeChildOrderChanged (juce::ValueTree& c, int, int)
{
    if (c.hasType (te::IDs::PLUGIN))
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
        if (m_track->pluginList.begin() != nullptr)
        {
            for (auto plugin : m_track->pluginList)
            {
                //don't show the default volume and levelmeter plugin
                if (m_track->pluginList.indexOf(plugin)  < m_track->pluginList.size() - 2 )
                {
                    auto p = std::make_unique<RackItemView> (m_evs, plugin);
                    addAndMakeVisible (p.get());
                    m_rackItems.add (std::move(p));
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
     || dragSourceDetails.description == "Instrument or Effect")
    {
        return true;
    }
    return false;
}

void RackView::itemDragMove(const SourceDetails& dragSourceDetails) 
{
    if (dragSourceDetails.description == "PluginComp"
        || dragSourceDetails.description == "PluginListEntry"
        || dragSourceDetails.description == "Instrument or Effect")

    {
        m_isOver = true;
    }
    repaint();
}

void RackView::itemDragExit (const SourceDetails& /*dragSourceDetails*/) 
{
    m_isOver = false;
    repaint();
}

void RackView::itemDropped(
    const juce::DragAndDropTarget::SourceDetails& details)
{
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
                    EngineHelpers::insertPlugin (m_track,
                                                 lbm->getSelectedPlugin(m_track->edit),
                                                 pluginRackComp->getAddButtons ().indexOf (this));
                }
            }

        }
    }


    if (dragSourceDetails.description == "PluginComp")
    {
        auto pluginRackComp = dynamic_cast<RackView*>(getParentComponent());
        if (pluginRackComp)
        {
            for (auto & pluginComp : pluginRackComp->getPluginComponents())
            {
                if (pluginComp == dragSourceDetails.sourceComponent)
                {
                    auto sourceIndex = m_track->getAllPlugins().indexOf(pluginComp->getPlugin());
                    auto plugToMove = pluginComp->getPlugin();
                    auto targetIndex = pluginRackComp->getAddButtons().indexOf(this);

                    targetIndex = targetIndex > sourceIndex ? targetIndex - 1 : targetIndex;

                    plugToMove->deleteFromParent();
                    m_track->pluginList.insertPlugin(plugToMove, targetIndex,nullptr);
                }
            }
        }
    }
}
