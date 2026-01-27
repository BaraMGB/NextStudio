/*
  ==============================================================================

    ModifierSidebar.cpp
    Created: 27 Jan 2026
    Author:  Gemini

  ==============================================================================
*/

#include "ModifierSidebar.h"

//==============================================================================
ModifierSidebar::ItemComponent::ItemComponent(ModifierSidebar &o, te::Modifier::Ptr m)
    : owner(o), modifier(m)
{
    addAndMakeVisible(removeButton);
    removeButton.setButtonText("x");
    removeButton.onClick = [this] {
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
    auto bg = juce::Colours::transparentBlack;

    if (m_isSelected)
        bg = owner.m_evs.m_applicationState.getPrimeColour().withAlpha(0.2f);
    else if (isMouseOver())
        bg = juce::Colours::white.withAlpha(0.05f);

    g.fillAll(bg);

    g.setColour(owner.m_evs.m_applicationState.getTextColour());
    g.setFont(14.0f);

    juce::String name = modifier->getName();
    if (name.isEmpty()) {
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

    g.drawText(name, getLocalBounds().reduced(5, 0).withRight(getWidth() - 25), juce::Justification::centredLeft, true);

    g.setColour(juce::Colours::grey.withAlpha(0.3f));
    g.drawRect(getLocalBounds(), 1);
}

void ModifierSidebar::ItemComponent::resized()
{
    removeButton.setBounds(getWidth() - 25, 0, 25, getHeight());
}

void ModifierSidebar::ItemComponent::mouseDown(const juce::MouseEvent &)
{
    owner.setSelectedModifier(modifier);
}

//==============================================================================
ModifierSidebar::ModifierSidebar(EditViewState &evs)
    : m_evs(evs)
{
    addAndMakeVisible(m_viewport);
    m_viewport.setViewedComponent(&m_listContainer, false);
    m_viewport.setScrollBarsShown(true, false, true, false); // vertical scrollbar only

    addAndMakeVisible(m_addButton);
    m_addButton.setButtonText("+");
    m_addButton.onClick = [this] {
        if (!m_track)
            return;

        juce::PopupMenu m;
        m.addItem(1, "LFO");
        m.addItem(2, "Step Sequencer");
        m.addItem(3, "Random");

        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&m_addButton), [this](int result) {
            if (result == 0)
                return;

            if (auto *ml = m_track->getModifierList()) {
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

    if (m_track) {
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

    if (mod) {
        if (currentID == mod->itemID)
            m_evs.setTrackSelectedModifier(m_track->itemID, {}); // Deselect/Toggle
        else
            m_evs.setTrackSelectedModifier(m_track->itemID, mod->itemID);
    }
    else {
        m_evs.setTrackSelectedModifier(m_track->itemID, {});
    }
}

void ModifierSidebar::handleAsyncUpdate()
{
    if (compareAndReset(m_structureChanged)) {
        updateList();
        // updateList also handles selection state init
    }
    else if (compareAndReset(m_selectionChanged)) {
        updateSelectionState();
    }

    // Notify listener (RackView) about selection change if any update happened
    if (onModifierSelected) {
        auto selectedID = getSelectedModifier() ? getSelectedModifier()->itemID : te::EditItemID();
        // We need to resolve the ID to a Ptr
        te::Modifier::Ptr selectedMod;
        if (m_track && selectedID.isValid()) {
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

    if (m_track) {
        if (auto *ml = m_track->getModifierList()) {
            for (auto m : ml->getModifiers()) {
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

    for (auto *item : m_items) {
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

    m_addButton.setBounds(header.removeFromRight(30).reduced(4));

    m_viewport.setBounds(area);

    int itemHeight = 30;
    int totalHeight = m_items.size() * itemHeight;

    m_listContainer.setSize(m_viewport.getWidth(), std::max(m_viewport.getHeight(), totalHeight));

    for (int i = 0; i < m_items.size(); ++i) {
        m_items[i]->setBounds(0, i * itemHeight, m_listContainer.getWidth(), itemHeight);
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
