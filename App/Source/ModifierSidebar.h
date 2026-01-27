/*
  ==============================================================================

    ModifierSidebar.h
    Created: 27 Jan 2026
    Author:  Gemini

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class ModifierSidebar : public juce::Component,
                        private te::ValueTreeAllEventListener,
                        private FlaggedAsyncUpdater
{
  public:
    ModifierSidebar(EditViewState &evs);
    ~ModifierSidebar() override;

    void setTrack(te::Track::Ptr track);
    void paint(juce::Graphics &g) override;
    void resized() override;

    void setSelectedModifier(te::Modifier::Ptr mod);
    te::Modifier::Ptr getSelectedModifier()
    {
        if (!m_track)
            return {};
        auto id = m_evs.getTrackSelectedModifier(m_track->itemID);
        if (id.isValid() && m_track->getModifierList())
            return te::findModifierForID(*m_track->getModifierList(), id);
        return {};
    }

    std::function<void(te::Modifier::Ptr)> onModifierSelected;

  private:
    void handleAsyncUpdate() override;
    void updateList();

    // te::ValueTreeAllEventListener
    void valueTreeChanged() override {}
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeParentChanged(juce::ValueTree &) override {}

    EditViewState &m_evs;
    te::Track::Ptr m_track;
    juce::ValueTree m_currentTrackRackState;

    bool m_structureChanged = false;
    bool m_selectionChanged = false;

    juce::TextButton m_addButton{"+"};

    class ItemComponent : public juce::Component
    {
      public:
        ItemComponent(ModifierSidebar &owner, te::Modifier::Ptr mod);
        ~ItemComponent() override;

        void paint(juce::Graphics &g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent &) override;

        void setSelected(bool selected)
        {
            m_isSelected = selected;
            repaint();
        }

        te::Modifier::Ptr modifier;
        ModifierSidebar &owner;
        juce::TextButton removeButton{"x"};
        bool m_isSelected = false;
    };

    juce::Viewport m_viewport;
    juce::Component m_listContainer;
    juce::OwnedArray<ItemComponent> m_items;

    void updateSelectionState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModifierSidebar)
};