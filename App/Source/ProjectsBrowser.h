
//
// /*
//  * Copyright 2023 Steffen Baranowsky
//  *
//  * This program is free software: you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License as published by
//  * the Free Software Foundation, either version 3 of the License, or
//  * (at your option) any later version.
//  *
//  * This program is distributed in the hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  * GNU General Public License for more details.
//  *
//  * You should have received a copy of the GNU General Public License
//  * along with this program. If not, see <http://www.gnu.org/licenses/>.
//  */
//
#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "ApplicationViewState.h"
#include "MenuBar.h"
#include "SearchFieldComponent.h"
#include "Utilities.h"
#include "PreviewComponent.h"
#include "Browser_Base.h"

namespace te = tracktion_engine;
class ProjectsBrowserComponent : public BrowserBaseComponent
                               , public juce::ChangeBroadcaster
{
public:
    ProjectsBrowserComponent(EditViewState& evs, ApplicationViewState& avs);

    void paintOverChildren(juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        auto prjButtons = area.removeFromTop(m_projectsMenu.getHeight());
        auto sortbox = area.removeFromTop(m_sortingBox.getHeight());
        auto searchfield = area.removeFromBottom(m_searchField.getHeight());
        auto list = area;

        g.setColour(m_avs.getBorderColour());
        g.drawHorizontalLine(prjButtons.getBottom(), 0, getWidth());
        g.drawHorizontalLine(sortbox.getBottom(), 0, getWidth());
        g.drawHorizontalLine(list.getBottom(), 0, getWidth());
        g.drawHorizontalLine(searchfield.getBottom(), 0, getWidth());
        
    }
    void resized() override;
    juce::var getDragSourceDescription (const juce::SparseSet<int>& /*rowsToDescribe*/) override;

    void paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent &e) override;
    void selectedRowsChanged(int /*lastRowSelected*/) override;


    juce::File m_projectToLoad{};

private:
    juce::DrawableButton m_loadProjectButton
                       , m_saveProjectButton
                       , m_newProjectButton;
    MenuBar             m_projectsMenu;
    void sortList(int selectedID) override;
    EditViewState & m_evs;
    ApplicationViewState& m_avs;

    struct CompareNameForward{
        static int compareElements (const juce::File& first, 
                                              const juce::File& second)
        {   
            return first.getFileName().compareNatural(second.getFileName());
        }
    };

    struct CompareNameBackwards{
        static int compareElements(const juce::File& first, 
                                               const juce::File& second)
        {
            return second.getFileName().compareNatural(first.getFileName());
        }
    };
    void sortByName(juce::Array<juce::File>& list, bool forward);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProjectsBrowserComponent)
};
