
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "RackView.h"
#include "PluginMenu.h"
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
    g.setColour (juce::Colour(0x181818));
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
        auto firstAdder = new AddButton(m_track);
        m_addButtons.add(firstAdder);
        addAndMakeVisible(*firstAdder);
        firstAdder->addListener(this);
        firstAdder->setButtonText("+");
        firstAdder->setBounds(area.removeFromLeft(15).reduced(0,10));

        for (auto p : m_rackItems)
        {
            area.removeFromLeft (5);
            p->setBounds (area.removeFromLeft((area.getHeight() * p->getNeededWidthFactor()) / 2 ));

            area.removeFromLeft(5);

            auto adder = new AddButton(m_track);
            adder->setPlugin(p->getPlugin());
            m_addButtons.add(adder);
            addAndMakeVisible(*adder);
            adder->setButtonText("+");
            adder->setBounds(area.removeFromLeft(15).reduced(0, 10));
            adder->addListener(this);
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
                    auto p = new RackItemView (m_evs, plugin);
                    addAndMakeVisible (p);
                    m_rackItems.add (p);
                }
            }
        }
    }
    resized();
}

bool RackView::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry")
    {
        return true;
    }
    return false;
}

void RackView::itemDragMove(const SourceDetails& dragSourceDetails) 
{
    if (dragSourceDetails.description == "PluginComp"
        || dragSourceDetails.description == "PluginListEntry")
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
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if(dragSourceDetails.description == "PluginListEntry")
        if (auto listbox = dynamic_cast<PluginListbox*>(
            dragSourceDetails.sourceComponent.get ()))
                EngineHelpers::insertPlugin (m_track, listbox->getSelectedPlugin(m_track->edit));

    m_isOver = false;
    repaint();
}
