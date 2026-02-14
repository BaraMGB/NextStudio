/*
  ==============================================================================

    ModifierSidebar.cpp
    Created: 27 Jan 2026
    Author:  Gemini

  ==============================================================================
*/

#include "UI/ModifierSidebar.h"
#include "BinaryData.h"

namespace
{
juce::StringArray getConnectedParameterNames(EditViewState &evs, te::Modifier::Ptr modifier)
{
    juce::StringArray names;

    if (modifier == nullptr)
        return names;

    auto connectedParams = te::getAllParametersBeingModifiedBy(evs.m_edit, *modifier);
    for (auto *param : connectedParams)
    {
        if (param != nullptr)
            names.add(param->getPluginAndParamName());
    }

    return names;
}

te::AutomatableParameter::ModifierAssignment::Ptr getAssignmentForModifier(te::AutomatableParameter::Ptr parameter, te::Modifier::Ptr modifier)
{
    if (parameter == nullptr || modifier == nullptr)
        return {};

    auto assignments = parameter->getAssignments();
    for (auto assignment : assignments)
    {
        if (assignment != nullptr && assignment->isForModifierSource(*modifier))
            return assignment;
    }

    return {};
}

void drawEyeIcon(juce::Graphics &g, juce::Rectangle<float> area, juce::Colour colour, bool enabled)
{
    g.setColour(colour);
    g.drawEllipse(area.reduced(1.0f), 1.0f);

    auto pupil = area.withSizeKeepingCentre(area.getWidth() * 0.3f, area.getHeight() * 0.3f);
    if (enabled)
        g.fillEllipse(pupil);
    else
        g.drawLine(area.getX() + 2.0f, area.getBottom() - 2.0f, area.getRight() - 2.0f, area.getY() + 2.0f, 1.2f);
}

const juce::Identifier kConnEnabled{"nsConnEnabled"};
const juce::Identifier kConnPrevValue{"nsConnPrevValue"};
} // namespace

//==============================================================================
ModifierSidebar::ItemComponent::ItemComponent(ModifierSidebar &o, te::Modifier::Ptr m)
    : owner(o),
      modifier(m)
{
    addAndMakeVisible(removeButton);
    removeButton.setButtonText("x");
    removeButton.onClick = [this]
    {
        if (modifier)
            modifier->remove();
    };

    // Simple look for the remove button
    removeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    removeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
    removeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

ModifierSidebar::ItemComponent::~ItemComponent() {}

void ModifierSidebar::ItemComponent::paint(juce::Graphics &g)
{
    g.fillAll(owner.m_evs.m_applicationState.getBackgroundColour1());

    if (m_isSelected)
    {
        auto trackColour = owner.m_track != nullptr ? owner.m_track->getColour() : owner.m_evs.m_applicationState.getPrimeColour();
        g.fillAll(trackColour.withAlpha(0.2f));
    }

    rebuildConnectionRows();

    auto textArea = getLocalBounds().reduced(5, 0).withLeft(25);
    auto nameArea = textArea.removeFromTop(22);

    g.setColour(owner.m_evs.m_applicationState.getTextColour());
    g.setFont(14.0f);

    juce::String name = modifier->getName();
    if (name.isEmpty())
    {
        // Fallback names based on type
        if (modifier->state.hasType(te::IDs::LFO))
            name = "LFO";
        else if (modifier->state.hasType(te::IDs::STEP))
            name = "Step Seq";
        else if (modifier->state.hasType(te::IDs::RANDOM))
            name = "Random";
        else
            name = "Modifier";
    }

    g.drawText(name, nameArea, juce::Justification::centredLeft, true);

    g.setColour(owner.m_evs.m_applicationState.getTextColour().withAlpha(0.65f));
    g.setFont(11.0f);
    auto connectionTextArea = textArea.withTrimmedLeft(8).withTrimmedRight(2);

    if (m_connectionRows.empty())
    {
        g.drawFittedText("No connected parameters", connectionTextArea.removeFromTop(14), juce::Justification::centredLeft, 1);
    }
    else
    {
        auto iconColour = owner.m_evs.m_applicationState.getTextColour().withAlpha(0.7f);

        for (auto &row : m_connectionRows)
        {
            auto rowRect = connectionTextArea.removeFromTop(14);
            row.rowBounds = rowRect;
            row.eyeBounds = rowRect.removeFromLeft(14).reduced(1);
            row.trashBounds = rowRect.removeFromRight(14).reduced(1);

            drawEyeIcon(g, row.eyeBounds.toFloat(), iconColour, row.enabled);
            GUIHelpers::drawFromSvg(g, BinaryData::trashcan_svg, iconColour, row.trashBounds.toFloat());
            g.drawFittedText(row.name, rowRect.withTrimmedLeft(6).withTrimmedRight(4), juce::Justification::centredLeft, 1);
        }
    }

    g.setColour(juce::Colours::grey.withAlpha(0.3f));
    g.drawRect(getLocalBounds(), 1);
}

void ModifierSidebar::ItemComponent::resized() { removeButton.setBounds(0, 0, 25, 22); }

void ModifierSidebar::ItemComponent::mouseUp(const juce::MouseEvent &e)
{
    for (int i = 0; i < static_cast<int>(m_connectionRows.size()); ++i)
    {
        if (m_connectionRows[(size_t)i].trashBounds.contains(e.getPosition()))
        {
            removeConnection(i);
            owner.repaint();
            return;
        }

        if (m_connectionRows[(size_t)i].eyeBounds.contains(e.getPosition()))
        {
            toggleConnectionEnabled(i);
            owner.repaint();
            return;
        }
    }

    owner.setSelectedModifier(modifier);
}

int ModifierSidebar::ItemComponent::getDesiredHeight()
{
    rebuildConnectionRows();
    const int numLines = juce::jmax(1, static_cast<int>(m_connectionRows.size()));
    return 24 + numLines * 14 + 4;
}

void ModifierSidebar::ItemComponent::rebuildConnectionRows()
{
    m_connectionRows.clear();

    auto names = getConnectedParameterNames(owner.m_evs, modifier);
    for (auto &name : names)
    {
        ConnectionRow row;
        row.name = name;
        m_connectionRows.push_back(row);
    }

    auto connectedParams = te::getAllParametersBeingModifiedBy(owner.m_evs.m_edit, *modifier);
    for (int i = 0; i < connectedParams.size() && i < static_cast<int>(m_connectionRows.size()); ++i)
    {
        auto param = connectedParams[i];
        if (param == nullptr)
            continue;

        auto assignment = getAssignmentForModifier(param, modifier);
        m_connectionRows[(size_t)i].parameter = param;
        m_connectionRows[(size_t)i].assignment = assignment;
        m_connectionRows[(size_t)i].enabled = assignment != nullptr ? static_cast<bool>(assignment->state.getProperty(kConnEnabled, true)) : true;
    }
}

void ModifierSidebar::ItemComponent::toggleConnectionEnabled(int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= static_cast<int>(m_connectionRows.size()))
        return;

    auto &row = m_connectionRows[(size_t)rowIndex];
    if (row.assignment == nullptr)
        return;

    auto currentlyEnabled = row.assignment->state.getProperty(kConnEnabled, true);

    if (currentlyEnabled)
    {
        row.assignment->state.setProperty(kConnPrevValue, row.assignment->value.get(), nullptr);
        row.assignment->value = 0.0f;
        row.assignment->state.setProperty(kConnEnabled, false, nullptr);
        row.enabled = false;
    }
    else
    {
        auto previousValue = static_cast<float>(row.assignment->state.getProperty(kConnPrevValue, 0.5f));
        row.assignment->value = previousValue;
        row.assignment->state.setProperty(kConnEnabled, true, nullptr);
        row.enabled = true;
    }
}

void ModifierSidebar::ItemComponent::removeConnection(int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= static_cast<int>(m_connectionRows.size()))
        return;

    auto &row = m_connectionRows[(size_t)rowIndex];
    if (row.parameter == nullptr || row.assignment == nullptr)
        return;

    row.parameter->removeModifier(*row.assignment);
    rebuildConnectionRows();
}

void ModifierSidebar::ItemComponent::mouseDown(const juce::MouseEvent &) {}

//==============================================================================
ModifierSidebar::ModifierSidebar(EditViewState &evs)
    : m_evs(evs)
{
    addAndMakeVisible(m_viewport);
    m_viewport.setViewedComponent(&m_listContainer, false);
    m_viewport.setScrollBarsShown(true, false, false, false); // vertical scrollbar only

    addAndMakeVisible(m_addButton);
    m_addButton.setButtonText("+");
    m_addButton.onClick = [this]
    {
        if (!m_track)
            return;

        juce::PopupMenu m;
        m.addItem(1, "LFO");
        m.addItem(2, "Step Sequencer");
        m.addItem(3, "Random");

        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&m_addButton),
                        [this](int result)
                        {
                            if (result == 0)
                                return;

                            if (auto *ml = m_track->getModifierList())
                            {
                                juce::Identifier id = te::IDs::LFO;
                                if (result == 2)
                                    id = te::IDs::STEP;
                                else if (result == 3)
                                    id = te::IDs::RANDOM;

                                ml->insertModifier(juce::ValueTree(id), -1, nullptr);
                            }
                        });
    };
}

ModifierSidebar::~ModifierSidebar()
{
    if (m_track)
        m_track->state.removeListener(this);

    if (m_currentTrackRackState.isValid())
        m_currentTrackRackState.removeListener(this);
}

void ModifierSidebar::setTrack(te::Track::Ptr track)
{
    if (m_track == track)
        return;

    // Remove listeners from old objects
    if (m_track)
        m_track->state.removeListener(this);

    m_currentTrackRackState.removeListener(this);

    // Update track
    m_track = track;

    // Setup new state
    juce::ValueTree newRackState;

    if (m_track)
    {
        m_track->state.addListener(this);
        newRackState = m_evs.getTrackRackViewState(m_track->itemID);
    }

    m_currentTrackRackState = newRackState;
    m_currentTrackRackState.addListener(this);

    markAndUpdate(m_structureChanged);
}
void ModifierSidebar::setSelectedModifier(te::Modifier::Ptr mod)
{
    if (!m_track)
        return;

    auto currentID = m_evs.getTrackSelectedModifier(m_track->itemID);

    if (mod)
    {
        if (currentID == mod->itemID)
            m_evs.setTrackSelectedModifier(m_track->itemID, {}); // Deselect/Toggle
        else
            m_evs.setTrackSelectedModifier(m_track->itemID, mod->itemID);
    }
    else
    {
        m_evs.setTrackSelectedModifier(m_track->itemID, {});
    }
}

void ModifierSidebar::handleAsyncUpdate()
{
    if (compareAndReset(m_structureChanged))
    {
        updateList();
        // updateList also handles selection state init
    }
    else if (compareAndReset(m_selectionChanged))
    {
        updateSelectionState();
    }

    // Notify listener (RackView) about selection change if any update happened
    if (onModifierSelected)
    {
        auto selectedID = getSelectedModifier() ? getSelectedModifier()->itemID : te::EditItemID();
        // We need to resolve the ID to a Ptr
        te::Modifier::Ptr selectedMod;
        if (m_track && selectedID.isValid())
        {
            if (auto *ml = m_track->getModifierList())
                selectedMod = te::findModifierForID(*ml, selectedID);
        }
        onModifierSelected(selectedMod);
    }
}

void ModifierSidebar::updateList()
{
    m_items.clear();

    te::EditItemID selectedID;
    if (m_track)
        selectedID = m_evs.getTrackSelectedModifier(m_track->itemID);

    if (m_track)
    {
        if (auto *ml = m_track->getModifierList())
        {
            for (auto m : ml->getModifiers())
            {
                auto *item = new ItemComponent(*this, m);
                if (m->itemID == selectedID)
                    item->setSelected(true);

                m_listContainer.addAndMakeVisible(item);
                m_items.add(item);
            }
        }
    }

    resized(); // Re-layout
    repaint();
}

void ModifierSidebar::updateSelectionState()
{
    te::EditItemID selectedID;
    if (m_track)
        selectedID = m_evs.getTrackSelectedModifier(m_track->itemID);

    for (auto *item : m_items)
    {
        bool shouldBeSelected = (item->modifier && item->modifier->itemID == selectedID);
        if (item->m_isSelected != shouldBeSelected)
            item->setSelected(shouldBeSelected);
    }
}

void ModifierSidebar::paint(juce::Graphics &g)
{
    g.fillAll(m_evs.m_applicationState.getBackgroundColour2());
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds().removeFromTop(30), 1); // Header border
}

void ModifierSidebar::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop(30);

    m_addButton.setBounds(header.removeFromLeft(30).reduced(4));

    m_viewport.setBounds(area);

    int totalHeight = 0;
    for (auto *item : m_items)
        totalHeight += item->getDesiredHeight();

    m_listContainer.setSize(m_viewport.getWidth(), std::max(m_viewport.getHeight(), totalHeight));

    int y = 0;
    for (auto *item : m_items)
    {
        auto itemHeight = item->getDesiredHeight();
        item->setBounds(0, y, m_listContainer.getWidth(), itemHeight);
        y += itemHeight;
    }
}

void ModifierSidebar::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &c)
{
    if (te::ModifierList::isModifier(c.getType()))
        markAndUpdate(m_structureChanged);
}

void ModifierSidebar::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &c, int)
{
    if (te::ModifierList::isModifier(c.getType()))
        markAndUpdate(m_structureChanged);
}

void ModifierSidebar::valueTreeChildOrderChanged(juce::ValueTree &c, int, int)
{
    if (te::ModifierList::isModifier(c.getType()))
        markAndUpdate(m_structureChanged);
}

void ModifierSidebar::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i)
{
    if (v == m_currentTrackRackState && i == IDs::selectedModifier)
        markAndUpdate(m_selectionChanged);
}
